/*******************************************************************************
*
*  (C) COPYRIGHT AUTHORS, 2019 - 2022
*
*  TITLE:       PROPTOKEN.C
*
*  VERSION:     1.94
*
*  DATE:        31 May 2022
*
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
* ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
* TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
*******************************************************************************/
#include "global.h"
#include "propDlg.h"

HWND g_hwndTokenPageList;
INT g_lvTokenPageSelectedItem;
INT g_lvTokenPageColumnHit;

#define T_TOKEN_PROP_CID_PID    TEXT("propTokenPid")
#define T_TOKEN_PROP_CID_TID    TEXT("propTokenTid")
#define T_TOKEN_PROP_TYPE       TEXT("propTokenType")


/*
* TokenPageShowError
*
* Purpose:
*
* Hide all windows for given hwnd and display error text with custom text if specified.
*
*/
VOID TokenPageShowError(
    _In_ HWND hwndDlg,
    _In_opt_ LPWSTR lpMessageText
)
{
    ENUMCHILDWNDDATA ChildWndData;

    if (GetWindowRect(hwndDlg, &ChildWndData.Rect)) {
        ChildWndData.nCmdShow = SW_HIDE;
        EnumChildWindows(hwndDlg, supCallbackShowChildWindow, (LPARAM)&ChildWndData);
    }

    if (lpMessageText) {
        SetDlgItemText(hwndDlg, IDC_TOKEN_ERROR, lpMessageText);
    }
    ShowWindow(GetDlgItem(hwndDlg, IDC_TOKEN_ERROR), SW_SHOW);
}

/*
* TokenPageInitControls
*
* Purpose:
*
* Initialize page controls.
*
*/
VOID TokenPageInitControls(
    _In_ HWND hwndDlg,
    _In_ BOOLEAN IsAppContainer
)
{
    LVGROUP lvg;

    g_hwndTokenPageList = GetDlgItem(hwndDlg, IDC_TOKEN_PRIVLIST);
    g_lvTokenPageSelectedItem = -1;
    g_lvTokenPageColumnHit = -1;

    //
    // Set listview style flags and theme.
    //
    supSetListViewSettings(g_hwndTokenPageList,
        LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP,
        FALSE,
        TRUE,
        NULL,
        0);

    SendMessage(g_hwndTokenPageList, LVM_ENABLEGROUPVIEW, 1, 0);

    supAddListViewColumn(g_hwndTokenPageList, 0, 0, 0,
        I_IMAGENONE,
        LVCFMT_LEFT,
        TEXT("Name"), 400);

    supAddListViewColumn(g_hwndTokenPageList, 1, 1, 1,
        I_IMAGENONE,
        LVCFMT_LEFT,
        TEXT("Status"), 150);

    RtlSecureZeroMemory(&lvg, sizeof(lvg));
    lvg.cbSize = sizeof(LVGROUP);
    lvg.mask = LVGF_HEADER | LVGF_ALIGN | LVGF_GROUPID;
    lvg.uAlign = LVGA_HEADER_LEFT;

    lvg.pszHeader = TEXT("Privileges");
    lvg.cchHeader = (INT)_strlen(lvg.pszHeader);
    lvg.iGroupId = 0;
    SendMessage(g_hwndTokenPageList, LVM_INSERTGROUP, (WPARAM)0, (LPARAM)&lvg);

    lvg.pszHeader = TEXT("Groups");
    lvg.cchHeader = (INT)_strlen(lvg.pszHeader);
    lvg.iGroupId = 1;
    SendMessage(g_hwndTokenPageList, LVM_INSERTGROUP, (WPARAM)1, (LPARAM)&lvg);

    if (IsAppContainer) {
        lvg.pszHeader = TEXT("Capabilities");
        lvg.cchHeader = (INT)_strlen(lvg.pszHeader);
        lvg.iGroupId = 2;
        SendMessage(g_hwndTokenPageList, LVM_INSERTGROUP, (WPARAM)2, (LPARAM)&lvg);
    }

    SetDlgItemText(hwndDlg, IDC_TOKEN_USER, T_CannotQuery);
    SetDlgItemText(hwndDlg, IDC_TOKEN_SID, T_CannotQuery);
    SetDlgItemText(hwndDlg, IDC_TOKEN_APPCONTAINER, T_CannotQuery);
}

/*
* TokenPageListAdd
*
* Purpose:
*
* Add item to page listview.
*
*/
VOID TokenPageListAdd(
    _In_ INT GroupIndex,
    _In_ LPWSTR lpName,
    _In_ LPWSTR lpStatus
)
{
    INT nIndex;
    LVITEM lvitem;

    RtlSecureZeroMemory(&lvitem, sizeof(lvitem));
    lvitem.mask = LVIF_TEXT | LVIF_GROUPID;
    lvitem.pszText = lpName;
    lvitem.iItem = MAXINT;
    lvitem.iGroupId = GroupIndex;
    nIndex = ListView_InsertItem(g_hwndTokenPageList, &lvitem);

    lvitem.mask = LVIF_TEXT;
    lvitem.iSubItem = 1;
    lvitem.pszText = lpStatus;
    lvitem.iItem = nIndex;
    ListView_SetItem(g_hwndTokenPageList, &lvitem);
}

/*
* TokenPageListInfo
*
* Purpose:
*
* Query and list token information.
*
*/
VOID TokenPageListInfo(
    _In_ PROP_OBJECT_INFO* Context,
    _In_ HWND hwndDlg
)
{
    BOOLEAN IsAppContainer = FALSE;
    ULONG i, cchName, r;
    NTSTATUS Status;
    LPWSTR ErrMsg = NULL, ElementName, UserAndDomain, pString;
    HANDLE ObjectHandle = NULL;
    HANDLE TokenHandle = NULL;
    ACCESS_MASK DesiredAccessLv1, DesiredAccessLv2;

    PTOKEN_PRIVILEGES pTokenPrivs;
    PTOKEN_USER pTokenUser;
    PTOKEN_MANDATORY_LABEL pTokenIntegrity;
    PTOKEN_GROUPS pTokenGroups;
    PTOKEN_APPCONTAINER_INFORMATION pTokenAppContainer;
    TOKEN_ELEVATION TokenElv;
    TOKEN_STATISTICS TokenStats;

    WCHAR szBuffer[MAX_PATH], szPrivName[MAX_PATH + 1];

    if (Context->TypeIndex == ObjectTypeProcess) {
        DesiredAccessLv1 = PROCESS_QUERY_INFORMATION;
        DesiredAccessLv2 = PROCESS_QUERY_LIMITED_INFORMATION;
    }
    else {
        DesiredAccessLv1 = THREAD_QUERY_INFORMATION;
        DesiredAccessLv2 = THREAD_QUERY_LIMITED_INFORMATION;
    }

    if (!propOpenCurrentObject(Context, &ObjectHandle, MAXIMUM_ALLOWED)) {
        if (!propOpenCurrentObject(Context, &ObjectHandle, DesiredAccessLv1)) {
            propOpenCurrentObject(Context, &ObjectHandle, DesiredAccessLv2);
        }
    }

    if (ObjectHandle == NULL) {
        TokenPageShowError(hwndDlg, NULL);
        return;
    }

    if (Context->TypeIndex == ObjectTypeProcess) {

        Status = supOpenProcessTokenEx(ObjectHandle, &TokenHandle);
        if (!NT_SUCCESS(Status))
            Status = NtOpenProcessToken(ObjectHandle, TOKEN_QUERY, &TokenHandle);

    }
    else {
        Status = NtOpenThreadToken(ObjectHandle, TOKEN_QUERY, TRUE, &TokenHandle);
    }

    if (NT_SUCCESS(Status) && TokenHandle != NULL) {


        i = 0;
        if (NT_SUCCESS(NtQueryInformationToken(TokenHandle, TokenIsAppContainer, (PVOID)&i, sizeof(ULONG), &r))) {
            IsAppContainer = (i > 0);
        }

        TokenPageInitControls(hwndDlg, IsAppContainer);

        //
        // List token privileges.
        //
        pTokenPrivs = (PTOKEN_PRIVILEGES)supGetTokenInfo(TokenHandle, TokenPrivileges, NULL);
        if (pTokenPrivs) {

            for (i = 0; i < pTokenPrivs->PrivilegeCount; i++) {

                //
                // Output privilege flags like Process Explorer.
                //
                szPrivName[0] = 0;
                cchName = MAX_PATH;
                if (LookupPrivilegeName(NULL, &pTokenPrivs->Privileges[i].Luid,
                    szPrivName, &cchName))
                {
                    ElementName = TEXT("Disabled");
                    if (pTokenPrivs->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED) {
                        ElementName = TEXT("Enabled");
                    }

                    _strcpy(szBuffer, ElementName);

                    if (pTokenPrivs->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT) {
                        _strcat(szBuffer, TEXT(", Default Enabled"));
                    }

                    TokenPageListAdd(0, szPrivName, szBuffer);
                }

            }

            supHeapFree(pTokenPrivs);
        }

        //
        // List token groups.
        //
        pTokenGroups = (PTOKEN_GROUPS)supGetTokenInfo(TokenHandle, TokenGroups, NULL);
        if (pTokenGroups) {

            for (i = 0; i < pTokenGroups->GroupCount; i++) {

                UserAndDomain = NULL;
                if (supLookupSidUserAndDomain(pTokenGroups->Groups[i].Sid, &UserAndDomain)) {

                    r = pTokenGroups->Groups[i].Attributes;
                    pString = NULL;
                    szBuffer[0] = 0;
                    if (r & SE_GROUP_USE_FOR_DENY_ONLY)
                        pString = _strcpy(szBuffer, TEXT("Deny"));

                    if (r & SE_GROUP_RESOURCE) {
                        if (pString)
                            _strcat(szBuffer, TEXT(", "));
                        pString = _strcat(szBuffer, TEXT("Domain-Local"));
                    }

                    if ((r & SE_GROUP_MANDATORY) && (!(r & SE_GROUP_OWNER))) {
                        if (pString)
                            _strcat(szBuffer, TEXT(", "));
                        pString = _strcat(szBuffer, TEXT("Mandatory"));
                    }
                    if (r & SE_GROUP_OWNER) {
                        if (pString)
                            _strcat(szBuffer, TEXT(", "));
                        pString = _strcat(szBuffer, TEXT("Owner"));
                    }
                    if (r & SE_GROUP_INTEGRITY) {
                        if (pString)
                            _strcat(szBuffer, TEXT(", "));
                        ElementName = TEXT("Integrity");
                        if (!(r & SE_GROUP_INTEGRITY_ENABLED)) {
                            ElementName = TEXT("DesktopIntegrity");
                        }
                        _strcat(szBuffer, ElementName);
                    }

                    TokenPageListAdd(1, UserAndDomain, szBuffer);

                    supHeapFree(UserAndDomain);
                }

            }

            supHeapFree(pTokenGroups);
        }

        //
        // Token elevated.
        //
        if (NT_SUCCESS(NtQueryInformationToken(TokenHandle, TokenElevation,
            (PVOID)&TokenElv, sizeof(TokenElv), &r)))
        {
            ElementName = (TokenElv.TokenIsElevated > 0) ? TEXT("Yes") : TEXT("No");
            SetDlgItemText(hwndDlg, IDC_TOKEN_ELEVATED, ElementName);
        }

        //
        // Token virtualization.
        //
        i = 0;
        if (NT_SUCCESS(NtQueryInformationToken(TokenHandle, TokenVirtualizationAllowed,
            (PVOID)&i, sizeof(i), &r)))
        {
            if (i > 0) {
                i = 0;
                if (NT_SUCCESS(NtQueryInformationToken(TokenHandle, TokenVirtualizationEnabled,
                    (PVOID)&i, sizeof(i), &r)))
                {
                    ElementName = (i > 0) ? TEXT("Yes") : TEXT("No");
                    SetDlgItemText(hwndDlg, IDC_TOKEN_VIRTUALIZED, ElementName);
                }
            }
        }
        else {
            SetDlgItemText(hwndDlg, IDC_TOKEN_VIRTUALIZED, TEXT("Not allowed"));
        }

        //
        // Token integrity level.
        //
        pTokenIntegrity = (PTOKEN_MANDATORY_LABEL)supGetTokenInfo(TokenHandle, TokenIntegrityLevel, NULL);
        if (pTokenIntegrity) {
            i = *RtlSubAuthoritySid(pTokenIntegrity->Label.Sid,
                (DWORD)(UCHAR)(*RtlSubAuthorityCountSid(pTokenIntegrity->Label.Sid) - 1));
            ElementName = supIntegrityToString(i);
            SetDlgItemText(hwndDlg, IDC_TOKEN_INTEGRITYLEVEL, ElementName);
            supHeapFree(pTokenIntegrity);
        }

        //
        // Token session id.
        //
        i = 0;
        if (NT_SUCCESS(NtQueryInformationToken(TokenHandle, TokenSessionId,
            (PVOID)&i, sizeof(i), &r)))
        {
            szBuffer[0] = 0;
            ultostr(i, szBuffer);
            SetDlgItemText(hwndDlg, IDC_TOKEN_SESSION, szBuffer);
        }

        //
        // Token user.
        //
        pTokenUser = (PTOKEN_USER)supGetTokenInfo(TokenHandle, TokenUser, NULL);
        if (pTokenUser) {
            ElementName = NULL;
            if (ConvertSidToStringSid(pTokenUser->User.Sid, &ElementName)) {
                SetDlgItemText(hwndDlg, IDC_TOKEN_SID, ElementName);
                LocalFree(ElementName);
            }

            ElementName = NULL;
            if (supLookupSidUserAndDomain(pTokenUser->User.Sid, &ElementName)) {
                SetDlgItemText(hwndDlg, IDC_TOKEN_USER, ElementName);
                supHeapFree(ElementName);
            }
            supHeapFree(pTokenUser);
        }

        //
        // AppContainer related.
        //
        if (IsAppContainer) {

            //
            // Token AppContainer SID.
            //
            pTokenAppContainer = (PTOKEN_APPCONTAINER_INFORMATION)supGetTokenInfo(TokenHandle, TokenAppContainerSid, NULL);
            if (pTokenAppContainer) {
                ElementName = NULL;
                if (pTokenAppContainer->TokenAppContainer) {
                    if (ConvertSidToStringSid(pTokenAppContainer->TokenAppContainer, &ElementName)) {
                        SetDlgItemText(hwndDlg, IDC_TOKEN_APPCONTAINER, ElementName);
                        LocalFree(ElementName);
                    }
                }
                supHeapFree(pTokenAppContainer);
            }


            pTokenGroups = (PTOKEN_GROUPS)supGetTokenInfo(TokenHandle, TokenCapabilities, NULL);
            if (pTokenGroups) {

                for (i = 0; i < pTokenGroups->GroupCount; i++) {
                    if (pTokenGroups->Groups[i].Sid) {
                        ElementName = NULL;
                        if (ConvertSidToStringSid(pTokenGroups->Groups[i].Sid, &ElementName)) {
                            TokenPageListAdd(2, ElementName, TEXT("Capabilities"));
                            LocalFree(ElementName);
                        }
                    }
                }
                supHeapFree(pTokenGroups);
            }
        }
        //
        // UIAccess
        //
        i = 0;
        if (NT_SUCCESS(NtQueryInformationToken(TokenHandle, TokenUIAccess,
            (PVOID)&i, sizeof(i), &r)))
        {
            ElementName = (i > 0) ? TEXT("Yes") : TEXT("No");
            SetDlgItemText(hwndDlg, IDC_TOKEN_UIACCESS, ElementName);
        }

        if (NT_SUCCESS(NtQueryInformationToken(TokenHandle, TokenStatistics,
            (PVOID)&TokenStats, sizeof(TOKEN_STATISTICS), &r)))
        {
            szBuffer[0] = 0;
            RtlStringCchPrintfSecure(szBuffer, MAX_PATH, L"0x%x-%x$", 
                TokenStats.AuthenticationId.HighPart,
                TokenStats.AuthenticationId.LowPart);
            SetDlgItemText(hwndDlg, IDC_TOKEN_AUTHID, szBuffer);
        }

        NtClose(TokenHandle);
    }
    else {
        if (Status == STATUS_NO_TOKEN)
            ErrMsg = TEXT("Token doesn't exist, thread is not impersonating a client.");

        TokenPageShowError(hwndDlg, ErrMsg);
    }
    NtClose(ObjectHandle);
}

/*
* TokenPageShowAdvancedProperties
*
* Purpose:
*
* Show properties of selected token object.
*
*/
VOID TokenPageShowAdvancedProperties(
    _In_ HWND hwndDlg)
{
    OBJECT_ATTRIBUTES ObjectAttributes = RTL_INIT_OBJECT_ATTRIBUTES((PUNICODE_STRING)NULL, 0);
    PROP_UNNAMED_OBJECT_INFO TokenObject;
    PROP_DIALOG_CREATE_SETTINGS propSettings;

    LPWSTR FormatStringTokenProcess = TEXT("Process Token, PID:%llu");
    LPWSTR FormatStringTokenThread = TEXT("Thread Token, PID:%llu, TID:%llu");

    HANDLE TokenHandle = NULL;
    WCHAR szFakeName[MAX_PATH + 1];

    //
    // Only one token properties dialog at the same time allowed.
    //
    ENSURE_DIALOG_UNIQUE(g_PsTokenWindow);

    RtlSecureZeroMemory(&TokenObject, sizeof(PROP_UNNAMED_OBJECT_INFO));

    TokenObject.ClientId.UniqueProcess =
        GetProp(hwndDlg, T_TOKEN_PROP_CID_PID);

    TokenObject.ClientId.UniqueThread =
        GetProp(hwndDlg, T_TOKEN_PROP_CID_TID);

    TokenObject.IsThreadToken =
        (BOOL)HandleToULong(GetProp(hwndDlg, T_TOKEN_PROP_TYPE));

    RtlSecureZeroMemory(szFakeName, sizeof(szFakeName));

    if (NT_SUCCESS(supOpenTokenByParam(&TokenObject.ClientId,
        &ObjectAttributes,
        TOKEN_QUERY,
        TokenObject.IsThreadToken,
        &TokenHandle)))
    {
        supQueryObjectFromHandle(TokenHandle, &TokenObject.ObjectAddress, NULL);
        NtClose(TokenHandle);
    }

    RtlSecureZeroMemory(&propSettings, sizeof(propSettings));

    if (TokenObject.IsThreadToken) {

        RtlStringCchPrintfSecure(szFakeName,
            MAX_PATH,
            FormatStringTokenThread,
            TokenObject.ClientId.UniqueProcess,
            TokenObject.ClientId.UniqueThread);

    }
    else {

        RtlStringCchPrintfSecure(szFakeName,
            MAX_PATH,
            FormatStringTokenProcess,
            TokenObject.ClientId.UniqueProcess);

    }

    propSettings.hwndParent = hwndDlg;
    propSettings.lpObjectName = szFakeName;
    propSettings.lpObjectType = OBTYPE_NAME_TOKEN;
    propSettings.UnnamedObject = &TokenObject;

    propCreateDialog(&propSettings);
}

/*
* TokenPageHandlePopup
*
* Purpose:
*
* Token page list popup construction.
*
*/
VOID TokenPageHandlePopup(
    _In_ HWND hwndDlg,
    _In_ LPPOINT lpPoint,
    _In_ PVOID lpUserParam
)
{
    HMENU hMenu;

    UNREFERENCED_PARAMETER(lpUserParam);

    hMenu = CreatePopupMenu();
    if (hMenu) {

        if (supListViewAddCopyValueItem(hMenu,
            g_hwndTokenPageList,
            ID_OBJECT_COPY,
            0,
            lpPoint,
            &g_lvTokenPageSelectedItem,
            &g_lvTokenPageColumnHit))
        {
            TrackPopupMenu(hMenu,
                TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                lpPoint->x,
                lpPoint->y,
                0,
                hwndDlg,
                NULL);
        }
        DestroyMenu(hMenu);
    }
}

/*
* TokenPageDialogOnCommand
*
* Purpose:
*
* Token page WM_COMMAND handler.
*
*/
INT_PTR TokenPageDialogOnCommand(
    _In_ HWND hwndDlg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
)
{
    INT_PTR Result = 0;

    UNREFERENCED_PARAMETER(lParam);

    switch (GET_WM_COMMAND_ID(wParam, lParam)) {
    case ID_OBJECT_COPY:

        supListViewCopyItemValueToClipboard(g_hwndTokenPageList,
            g_lvTokenPageSelectedItem,
            g_lvTokenPageColumnHit);
        
        Result = 1;
        break;
    case IDC_TOKEN_ADVANCED:
        TokenPageShowAdvancedProperties(hwndDlg);
        Result = 1;
        break;
    default:
        break;
    }

    return Result;
}

/*
* TokenPageDialogOnInit
*
* Purpose:
*
* Token page WM_INITDIALOG handler.
*
*/
INT_PTR TokenPageDialogOnInit(
    _In_ HWND hwndDlg,
    _In_ LPARAM lParam)
{
    PROPSHEETPAGE* pSheet = NULL;
    PROP_OBJECT_INFO* Context = NULL;

    pSheet = (PROPSHEETPAGE*)lParam;
    if (pSheet) {
        Context = (PROP_OBJECT_INFO*)pSheet->lParam;

        //
        // Remember client id.
        //
        SetProp(hwndDlg,
            T_TOKEN_PROP_CID_PID,
            Context->UnnamedObjectInfo.ClientId.UniqueProcess);

        SetProp(hwndDlg,
            T_TOKEN_PROP_CID_TID,
            Context->UnnamedObjectInfo.ClientId.UniqueThread);

        SetProp(hwndDlg,
            T_TOKEN_PROP_TYPE,
            UlongToHandle((ULONG)(Context->TypeDescription->Index == ObjectTypeThread)));

        //
        // Show token summary information.
        //
        TokenPageListInfo(Context, hwndDlg);
    }

    return 1;
}

/*
* TokenPageDialogProc
*
* Purpose:
*
* Token page for Process/Thread object type.
*
* WM_INITDIALOG - Initialize listview, set window prop with context,
* collect token info and fill list.
*
*/
INT_PTR CALLBACK TokenPageDialogProc(
    _In_  HWND hwndDlg,
    _In_  UINT uMsg,
    _In_  WPARAM wParam,
    _In_  LPARAM lParam
)
{
    switch (uMsg) {

    case WM_CONTEXTMENU:

        supHandleContextMenuMsgForListView(hwndDlg,
            wParam,
            lParam,
            g_hwndTokenPageList,
            (pfnPopupMenuHandler)TokenPageHandlePopup,
            NULL);

        break;

    case WM_COMMAND:
        return TokenPageDialogOnCommand(hwndDlg, wParam, lParam);

    case WM_INITDIALOG:
        return TokenPageDialogOnInit(hwndDlg, lParam);

    case WM_DESTROY:
        RemoveProp(hwndDlg, T_TOKEN_PROP_CID_PID);
        RemoveProp(hwndDlg, T_TOKEN_PROP_CID_TID);
        RemoveProp(hwndDlg, T_TOKEN_PROP_TYPE);
        break;

    default:
        return 0;
    }
    return 1;
}
