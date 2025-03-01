/*******************************************************************************
*
*  (C) COPYRIGHT AUTHORS, 2015 - 2022
*
*  TITLE:       ABOUTDLG.C
*
*  VERSION:     1.94
*
*  DATE:        04 Jun 2022
*
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
* ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
* TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
*******************************************************************************/
#include "global.h"
#include "msvcver.h"
#include "winedebug.h"

#define T_ABOUTDLG_ICON_PROP TEXT("aboutDlgIcon")

/*
* AboutDialogInit
*
* Purpose:
*
* Displays program version and system information
*
*/
VOID AboutDialogInit(
    HWND hwndDlg
)
{
    BOOLEAN  bSecureBoot = FALSE;
    BOOLEAN  bHVCIEnabled = FALSE, bHVCIStrict = FALSE, bHVCIIUMEnabled = FALSE;
    HANDLE   hImage;
    WCHAR    szBuffer[MAX_PATH];

    PCHAR    wine_ver, wine_str;

    FIRMWARE_TYPE firmwareType;

    SetDlgItemText(hwndDlg, ID_ABOUT_PROGRAM, PROFRAM_NAME_AND_TITLE);

    RtlStringCchPrintfSecure(szBuffer,
        MAX_PATH,
        TEXT("%lu.%lu.%lu"),
        PROGRAM_MAJOR_VERSION,
        PROGRAM_MINOR_VERSION,
        PROGRAM_REVISION_NUMBER);

    SetDlgItemText(hwndDlg, ID_ABOUT_BUILDINFO, szBuffer);

    //
    // Set dialog icon.
    //
    hImage = LoadImage(g_WinObj.hInstance,
        MAKEINTRESOURCE(IDI_ICON_MAIN),
        IMAGE_ICON,
        48, 48,
        0);

    if (hImage) {

        SendDlgItemMessage(hwndDlg, ID_ABOUT_ICON,
            STM_SETIMAGE, IMAGE_ICON, (LPARAM)hImage);

        SetProp(hwndDlg, T_ABOUTDLG_ICON_PROP, hImage);
    }

    //
    // Set compiler version and name.
    //
    RtlSecureZeroMemory(szBuffer, sizeof(szBuffer));
    _strcpy(szBuffer, VC_VER);
    if (szBuffer[0] == 0) {
        _strcpy(szBuffer, TEXT("MSVC ("));
        ultostr(_MSC_FULL_VER, _strend(szBuffer));
        _strcat(szBuffer, TEXT(")"));
    }
#if defined(__cplusplus)
    _strcat(szBuffer, TEXT(" compiled as C++"));
#else
    _strcat(szBuffer, TEXT(" compiled as C"));
#endif

    SetDlgItemText(hwndDlg, ID_ABOUT_COMPILERINFO, szBuffer);

    //
    // Set build date and time.
    //
    RtlSecureZeroMemory(szBuffer, sizeof(szBuffer));
    MultiByteToWideChar(CP_ACP, 0, __DATE__, (INT)_strlen_a(__DATE__), szBuffer, 40);
    _strcat(szBuffer, TEXT(" "));
    MultiByteToWideChar(CP_ACP, 0, __TIME__, (INT)_strlen_a(__TIME__), _strend(szBuffer), 40);
    SetDlgItemText(hwndDlg, ID_ABOUT_BUILDDATE, szBuffer);

    //
    // Fill OS name.
    //
    RtlSecureZeroMemory(szBuffer, sizeof(szBuffer));
    if (g_WinObj.IsWine) {
        _strcpy(szBuffer, TEXT("Reported as "));
    }

    RtlStringCchPrintfSecure(_strend(szBuffer),
        100,
        TEXT("Windows NT %1u.%1u (build %u"),
        g_WinObj.osver.dwMajorVersion,
        g_WinObj.osver.dwMinorVersion,
        g_WinObj.osver.dwBuildNumber);

    if (g_WinObj.osver.szCSDVersion[0]) {
        _strcat(szBuffer, TEXT(", "));
        _strcat(szBuffer, g_WinObj.osver.szCSDVersion);
    }
    _strcat(szBuffer, TEXT(")"));

    //
    // Fill boot options.
    //   
    if (g_WinObj.IsWine) {
        wine_ver = (PCHAR)wine_get_version();
        wine_str = (PCHAR)supHeapAlloc(_strlen_a(wine_ver) + MAX_PATH);
        if (wine_str) {
            _strcpy_a(wine_str, "Wine ");
            _strcat_a(wine_str, wine_ver);
            SetDlgItemTextA(hwndDlg, ID_ABOUT_OSNAME, wine_str);
            supHeapFree(wine_str);
        }
        SetDlgItemText(hwndDlg, ID_ABOUT_ADVINFO, szBuffer);

    }
    else {
        SetDlgItemText(hwndDlg, ID_ABOUT_OSNAME, szBuffer);

        RtlSecureZeroMemory(&szBuffer, sizeof(szBuffer));

        //
        // Query KD debugger enabled.
        //
        if (ntsupIsKdEnabled(NULL, NULL)) {
            _strcpy(szBuffer, TEXT("Debug, "));
        }

        //
        // Query VHD boot state if possible.
        //
        if (g_kdctx.IsOsDiskVhd) {
            _strcat(szBuffer, TEXT("VHD, "));
        }

        //
        // Query firmware mode and SecureBoot state for UEFI.
        //
        firmwareType = g_kdctx.Data->FirmwareType;

        if (firmwareType == FirmwareTypeUnknown) {

            _strcpy(szBuffer, T_Unknown);

        }
        else {

            if (firmwareType == FirmwareTypeUefi) {
                _strcat(szBuffer, TEXT("UEFI"));
            }
            else {
                if (firmwareType == FirmwareTypeBios) {
                    _strcat(szBuffer, TEXT("BIOS"));
                }
                else {
                    _strcat(szBuffer, TEXT("Unknown"));
                }
            }

            if (firmwareType == FirmwareTypeUefi) {
                bSecureBoot = FALSE;
                if (supQuerySecureBootState(&bSecureBoot)) {
                    _strcat(szBuffer, TEXT(" with"));
                    if (bSecureBoot == FALSE) {
                        _strcat(szBuffer, TEXT("out"));
                    }
                    _strcat(szBuffer, TEXT(" SecureBoot"));
                }
                g_kdctx.IsSecureBoot = bSecureBoot;

                if (supQueryHVCIState(&bHVCIEnabled, &bHVCIStrict, &bHVCIIUMEnabled)) {
                    if (bHVCIEnabled) {
                        _strcat(szBuffer, TEXT(", HVCI"));
                        if (bHVCIStrict)
                            _strcat(szBuffer, TEXT(" (strict)"));
                        if (bHVCIIUMEnabled)
                            _strcat(szBuffer, TEXT(", IUM"));
                    }
                }
            }
        }

        SetDlgItemText(hwndDlg, ID_ABOUT_ADVINFO, szBuffer);
    }

    SetFocus(GetDlgItem(hwndDlg, IDOK));
}

/*
* AboutDialogOnNotify
*
* Purpose:
*
* WM_NOTIFY handler.
*
*/
VOID AboutDialogOnNotify(
    _In_ HWND   hwndDlg,
    _In_ LPARAM lParam
)
{
    PNMLINK pNMLink;
    LITEM item;

    switch (((LPNMHDR)lParam)->code) {
    case NM_CLICK:
    case NM_RETURN:

        pNMLink = (PNMLINK)lParam;
        item = pNMLink->item;
        if ((((LPNMHDR)lParam)->hwndFrom == GetDlgItem(hwndDlg, IDC_ABOUT_SYSLINK))
            && (item.iLink == 0))
        {
            supShellExecInExplorerProcess(item.szUrl);
        }

        break;
    }
}

/*
* AboutDialogProc
*
* Purpose:
*
* About Dialog Window Procedure
*
* During WM_INITDIALOG centers window and initializes system info
*
*/
INT_PTR CALLBACK AboutDialogProc(
    _In_ HWND   hwndDlg,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
)
{
    HANDLE hIcon;

    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg) {

    case WM_INITDIALOG:
        supCenterWindow(hwndDlg);
        AboutDialogInit(hwndDlg);
        break;

    case WM_NOTIFY:
        AboutDialogOnNotify(hwndDlg, lParam);
        break;

    case WM_COMMAND:

        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDOK:
        case IDCANCEL:
            hIcon = RemoveProp(hwndDlg, T_ABOUTDLG_ICON_PROP);
            if (hIcon) {
                DestroyIcon((HICON)hIcon);
            }
            return EndDialog(hwndDlg, S_OK);
        }
    }
    return 0;
}
