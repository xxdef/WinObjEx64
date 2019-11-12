/*******************************************************************************
*
*  (C) COPYRIGHT AUTHORS, 2015 - 2019
*
*  TITLE:       ABOUTDLG.C
*
*  VERSION:     1.82
*
*  DATE:        08 Nov 2019
*
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
* ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
* TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
*******************************************************************************/
#include "global.h"
#include "msvcver.h"

#undef _WINE_NB_DEBUG

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
    ULONG    returnLength;
    NTSTATUS status;
    HANDLE   hImage;
    WCHAR    szBuffer[MAX_PATH];

    PCHAR    wine_ver, wine_str;

    SYSTEM_BOOT_ENVIRONMENT_INFORMATION sbei;
    SYSTEM_VHD_BOOT_INFORMATION *psvbi;

    SetDlgItemText(hwndDlg, ID_ABOUT_PROGRAM, PROFRAM_NAME_AND_TITLE);

    rtl_swprintf_s(szBuffer, 100, TEXT("%lu.%lu.%lu"),
        PROGRAM_MAJOR_VERSION,
        PROGRAM_MINOR_VERSION,
        PROGRAM_REVISION_NUMBER);

    SetDlgItemText(hwndDlg, ID_ABOUT_BUILDINFO, szBuffer);

    //
    // Set dialog icon.
    //
    hImage = LoadImage(g_WinObj.hInstance, MAKEINTRESOURCE(IDI_ICON_MAIN), IMAGE_ICON, 48, 48, LR_SHARED);
    if (hImage) {
        SendMessage(GetDlgItem(hwndDlg, ID_ABOUT_ICON), STM_SETIMAGE, IMAGE_ICON, (LPARAM)hImage);
        DestroyIcon((HICON)hImage);
    }

    //
    // Remove class icon if any.
    //
    SetClassLongPtr(hwndDlg, GCLP_HICON, (LONG_PTR)NULL);

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
    MultiByteToWideChar(CP_ACP, 0, __DATE__, (INT)_strlen_a(__DATE__), _strend(szBuffer), 40);
    _strcat(szBuffer, TEXT(" "));
    MultiByteToWideChar(CP_ACP, 0, __TIME__, (INT)_strlen_a(__TIME__), _strend(szBuffer), 40);
    SetDlgItemText(hwndDlg, ID_ABOUT_BUILDDATE, szBuffer);

#ifdef _WINE_NB_DEBUG
    g_WinObj.IsWine = TRUE;
    wine_ver = "4.9";
#endif

    //
    // Fill OS name.
    //
    RtlSecureZeroMemory(szBuffer, sizeof(szBuffer));
    if (g_WinObj.IsWine) {
        _strcpy(szBuffer, TEXT("Reported as "));
    }

    rtl_swprintf_s(_strend(szBuffer), 100, TEXT("Windows NT %1u.%1u (build %u"),
        g_WinObj.osver.dwMajorVersion, g_WinObj.osver.dwMinorVersion, g_WinObj.osver.dwBuildNumber);
    if (g_WinObj.osver.szCSDVersion[0]) {
        _strcat(szBuffer, TEXT(", "));
        _strcat(szBuffer, g_WinObj.osver.szCSDVersion);
    }
    _strcat(szBuffer, TEXT(")"));

    //
    // Fill boot options.
    //   
    if (g_WinObj.IsWine) {
#ifndef _WINE_NB_DEBUG
        wine_ver = (PCHAR)wine_get_version();
#endif
        wine_str = (PCHAR)supHeapAlloc(_strlen_a(wine_ver) + MAX_PATH);
        if (wine_str) {
            _strcpy_a(wine_str, "Wine ");
            _strcat_a(wine_str, wine_ver);
            _strcat_a(wine_str, " emulation");
            SetDlgItemTextA(hwndDlg, ID_ABOUT_OSNAME, wine_str);
            supHeapFree(wine_str);
        }
        SetDlgItemText(hwndDlg, ID_ABOUT_ADVINFO, szBuffer);

    }
    else {
        SetDlgItemText(hwndDlg, ID_ABOUT_OSNAME, szBuffer);

        RtlSecureZeroMemory(&sbei, sizeof(sbei));
        RtlSecureZeroMemory(&szBuffer, sizeof(szBuffer));

        //
        // Query KD debugger enabled.
        //
        if (kdIsDebugBoot()) {
            _strcpy(szBuffer, TEXT("Debug, "));
        }

        //
        // Query VHD boot state if possible.
        //
        psvbi = (SYSTEM_VHD_BOOT_INFORMATION*)supHeapAlloc(PAGE_SIZE);
        if (psvbi) {
            status = NtQuerySystemInformation(SystemVhdBootInformation, psvbi, PAGE_SIZE, &returnLength);
            if (NT_SUCCESS(status)) {
                if (psvbi->OsDiskIsVhd) {
                    _strcat(szBuffer, TEXT("VHD, "));
                }
            }
            supHeapFree(psvbi);
        }

        //
        // Query firmware mode and SecureBoot state for UEFI.
        //
        status = NtQuerySystemInformation(SystemBootEnvironmentInformation, &sbei, sizeof(sbei), &returnLength);
        if (NT_SUCCESS(status)) {

            if (sbei.FirmwareType == FirmwareTypeUefi) {
                _strcat(szBuffer, TEXT("UEFI"));
            }
            else {
                if (sbei.FirmwareType == FirmwareTypeBios) {
                    _strcat(szBuffer, TEXT("BIOS"));
                }
                else {
                    _strcat(szBuffer, TEXT("Unknown"));
                }
            }

            if (sbei.FirmwareType == FirmwareTypeUefi) {
                bSecureBoot = FALSE;
                if (supQuerySecureBootState(&bSecureBoot)) {
                    _strcat(szBuffer, TEXT(" with"));
                    if (bSecureBoot == FALSE) {
                        _strcat(szBuffer, TEXT("out"));
                    }
                    _strcat(szBuffer, TEXT(" SecureBoot"));
                }
                g_kdctx.IsSecureBoot = bSecureBoot;
            }
        }
        else {
            _strcpy(szBuffer, TEXT("Unknown"));
        }
        SetDlgItemText(hwndDlg, ID_ABOUT_ADVINFO, szBuffer);
    }

    SetFocus(GetDlgItem(hwndDlg, IDOK));
}

VOID GlobalsAppendText(
    _In_ HWND hwndEdit,
    _In_ LPWSTR lpText)
{
    int idx = GetWindowTextLength(hwndEdit);
    SendMessage(hwndEdit, EM_SETSEL, (WPARAM)idx, (LPARAM)idx);
    SendMessage(hwndEdit, EM_REPLACESEL, 0, (LPARAM)lpText);
}

/*
* AboutDialogCollectGlobals
*
* Purpose:
*
* Build globals list (g_kdctx + g_WinObj).
*
*/
VOID AboutDialogCollectGlobals(
    _In_ HWND hwndDlg,
    _In_ HWND hwndParent
)
{
    BOOLEAN bCustomSignersAllowed;

    WCHAR szBuffer[MAX_PATH * 4];
    WCHAR szTemp[MAX_PATH];

    SYSTEM_INFO SystemInfo;
    HKEY hKey;
    DWORD dwType, cbData, dwValue;

    HWND hwndEdit = GetDlgItem(hwndDlg, IDC_GLOBALS);


    RtlSecureZeroMemory(szBuffer, sizeof(szBuffer));
    RtlSecureZeroMemory(szTemp, sizeof(szTemp));

    //
    // Generic environment information.
    //
    rtl_swprintf_s(szBuffer, 100, TEXT("Windows Object Explorer 64\t\t:\t%lu.%lu.%lu\r\n"),
        PROGRAM_MAJOR_VERSION,
        PROGRAM_MINOR_VERSION,
        PROGRAM_REVISION_NUMBER);
    GlobalsAppendText(hwndEdit, szBuffer);

    GetDlgItemText(hwndParent, ID_ABOUT_OSNAME, szTemp, MAX_PATH);

    rtl_swprintf_s(szBuffer,
        MAX_PATH * 2,
        TEXT("Operation System\t\t\t:\t%s\r\n"),
        szTemp);
    GlobalsAppendText(hwndEdit, szBuffer);

    if (ERROR_SUCCESS == RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
        0,
        KEY_QUERY_VALUE,
        &hKey))
    {
        RtlSecureZeroMemory(szTemp, sizeof(szTemp));
        _strcpy(szBuffer, TEXT("Processor\t\t\t\t:\t"));

        cbData = 128;
        dwType = REG_NONE;
        if (ERROR_SUCCESS == RegQueryValueEx(
            hKey,
            TEXT("Identifier"),
            NULL,
            &dwType,
            (LPBYTE)&szTemp,
            &cbData))
        {
            _strcat(szBuffer, szTemp);
        }

        _strcat(szBuffer, TEXT(", "));

        cbData = 128;
        dwType = REG_NONE;
        if (ERROR_SUCCESS == RegQueryValueEx(
            hKey,
            TEXT("VendorIdentifier"),
            NULL,
            &dwType,
            (LPBYTE)&szTemp,
            &cbData))
        {
            _strcat(szBuffer, szTemp);
        }

        _strcat(szBuffer, TEXT(", "));

        cbData = sizeof(DWORD);
        dwType = REG_NONE;
        dwValue = 0;
        if (ERROR_SUCCESS == RegQueryValueEx(
            hKey,
            TEXT("~MHz"),
            NULL,
            &dwType,
            (LPBYTE)&dwValue,
            &cbData))
        {
            szTemp[0] = L'~';
            szTemp[1] = 0;
            ultostr(dwValue, &szTemp[1]);
            _strcat(szTemp, TEXT("MHz"));
            _strcat(szBuffer, szTemp);
        }

        _strcat(szBuffer, TEXT("\r\n"));
        GlobalsAppendText(hwndEdit, szBuffer);

        RegCloseKey(hKey);
    }

    GetSystemInfo(&SystemInfo);

    rtl_swprintf_s(szBuffer,
        MAX_PATH * 2,
        TEXT("Number of Processors\t\t:\t%lu, Mask 0x%08lX\r\n"),
        SystemInfo.dwNumberOfProcessors,
        SystemInfo.dwActiveProcessorMask);
    GlobalsAppendText(hwndEdit, szBuffer);


    //
    // List g_kdctx.
    //
    GlobalsAppendText(hwndEdit, TEXT("\r\ng_kdctx variables\r\n"));

    rtl_swprintf_s(szBuffer,
        MAX_PATH,
        TEXT("IsSecureBoot\t\t\t:\t%lu\r\n"),
        g_kdctx.IsSecureBoot);
    GlobalsAppendText(hwndEdit, szBuffer);

    rtl_swprintf_s(szBuffer,
        MAX_PATH,
        TEXT("drvOpenLoadStatus\t\t:\t%lu"),
        g_kdctx.drvOpenLoadStatus);

    if (g_kdctx.drvOpenLoadStatus == 0) {
        _strcat(szBuffer, TEXT(" (reported as OK)"));
    }
    _strcat(szBuffer, TEXT("\r\n"));
    GlobalsAppendText(hwndEdit, szBuffer);

    rtl_swprintf_s(szBuffer,
        MAX_PATH,
        TEXT("IsFullAdmin\t\t\t:\t%lu\r\n"),
        g_kdctx.IsFullAdmin);
    GlobalsAppendText(hwndEdit, szBuffer);

    rtl_swprintf_s(szBuffer,
        MAX_PATH,
        TEXT("IsOurLoad\t\t\t:\t%lu\r\n"),
        g_kdctx.IsOurLoad);
    GlobalsAppendText(hwndEdit, szBuffer);

    rtl_swprintf_s(szBuffer,
        MAX_PATH,
        TEXT("DirectoryRootAddress\t\t:\t0x%p\r\n"),
        g_kdctx.DirectoryRootAddress);
    GlobalsAppendText(hwndEdit, szBuffer);

    rtl_swprintf_s(szBuffer,
        MAX_PATH,
        TEXT("DirectoryTypeIndex\t\t:\t%lu\r\n"),
        g_kdctx.DirectoryTypeIndex);
    GlobalsAppendText(hwndEdit, szBuffer);

    rtl_swprintf_s(szBuffer,
        MAX_PATH,
        TEXT("hDevice\t\t\t\t:\t%llu\r\n"),
        g_kdctx.hDevice);
    GlobalsAppendText(hwndEdit, szBuffer);

    rtl_swprintf_s(szBuffer,
        MAX_PATH,
        TEXT("IopInvalidDeviceRequest\t\t:\t0x%p\r\n"),
        g_kdctx.IopInvalidDeviceRequest);
    GlobalsAppendText(hwndEdit, szBuffer);

    rtl_swprintf_s(szBuffer,
        MAX_PATH,
        TEXT("KiServiceLimit\t\t\t:\t%lX\r\n"),
        g_kdctx.KiServiceLimit);
    GlobalsAppendText(hwndEdit, szBuffer);

    rtl_swprintf_s(szBuffer,
        MAX_PATH,
        TEXT("KiServiceTableAddress\t\t:\t0x%p\r\n"),
        g_kdctx.KiServiceTableAddress);
    GlobalsAppendText(hwndEdit, szBuffer);

    rtl_swprintf_s(szBuffer,
        MAX_PATH,
        TEXT("NtOsBase\t\t\t\t:\t0x%p\r\n"),
        g_kdctx.NtOsBase);
    GlobalsAppendText(hwndEdit, szBuffer);

    rtl_swprintf_s(szBuffer,
        MAX_PATH,
        TEXT("NtOsSize\t\t\t\t:\t0x%lX\r\n"),
        g_kdctx.NtOsSize);
    GlobalsAppendText(hwndEdit, szBuffer);

    rtl_swprintf_s(szBuffer,
        MAX_PATH,
        TEXT("NtOsImageMap\t\t\t:\t0x%p\r\n"),
        g_kdctx.NtOsImageMap);
    GlobalsAppendText(hwndEdit, szBuffer);

    rtl_swprintf_s(szBuffer,
        MAX_PATH,
        TEXT("ObHeaderCookie\t\t\t:\t0x%lX\r\n"),
        g_kdctx.ObHeaderCookie);
    GlobalsAppendText(hwndEdit, szBuffer);

    rtl_swprintf_s(szBuffer,
        MAX_PATH,
        TEXT("PrivateNamespaceLookupTable\t:\t0x%p\r\n"),
        g_kdctx.PrivateNamespaceLookupTable);
    GlobalsAppendText(hwndEdit, szBuffer);

    rtl_swprintf_s(szBuffer,
        MAX_PATH,
        TEXT("SystemRangeStart\t\t\t:\t0x%llX\r\n"),
        g_kdctx.SystemRangeStart);
    GlobalsAppendText(hwndEdit, szBuffer);

    //
    // List g_WinObj (UI specific).
    //
    GlobalsAppendText(hwndEdit, TEXT("\r\ng_WinObj variables\r\n"));

    rtl_swprintf_s(szBuffer,
        MAX_PATH,
        TEXT("EnableExperimentalFeatures\t\t:\t%lu\r\n"),
        g_WinObj.EnableExperimentalFeatures);
    GlobalsAppendText(hwndEdit, szBuffer);

    rtl_swprintf_s(szBuffer,
        MAX_PATH,
        TEXT("IsWine\t\t\t\t:\t%lu\r\n"),
        g_WinObj.IsWine);
    GlobalsAppendText(hwndEdit, szBuffer);

    //
    // List other data.
    //
    GlobalsAppendText(hwndEdit, TEXT("\r\nOther data\r\n"));

    if (NT_SUCCESS(supCICustomKernelSignersAllowed(&bCustomSignersAllowed))) {

        rtl_swprintf_s(szBuffer,
            MAX_PATH,
            TEXT("Licensed for Custom Kernel Signers\t:\t%lu\r\n"),
            bCustomSignersAllowed);
        GlobalsAppendText(hwndEdit, szBuffer);
    }

    rtl_swprintf_s(szBuffer,
        MAX_PATH,
        TEXT("DPI Value\t\t\t\t:\t%lu"),
        supGetDPIValue(NULL));
    GlobalsAppendText(hwndEdit, szBuffer);

    SetFocus(hwndEdit);
}


/*
* GlobalsWindowProc
*
* Purpose:
*
* Globals dialog window procedure.
*
*/
LRESULT CALLBACK GlobalsWindowProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
)
{
    HWND hwndParent = (HWND)lParam;

    switch (uMsg) {
    case WM_INITDIALOG:

        AboutDialogCollectGlobals(hwnd, hwndParent);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            return EndDialog(hwnd, S_OK);
            break;
        default:
            break;
        }

    default:
        break;
    }
    return 0;
}

/*
* AboutDialogProc
*
* Purpose:
*
* About Dialog Window Dialog Procedure
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
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg) {

    case WM_INITDIALOG:
        supCenterWindow(hwndDlg);
        AboutDialogInit(hwndDlg);
        break;

    case WM_COMMAND:

        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            return EndDialog(hwndDlg, S_OK);
            break;
        case IDC_ABOUT_GLOBALS:

            DialogBoxParam(g_WinObj.hInstance,
                MAKEINTRESOURCE(IDD_DIALOG_GLOBALS),
                hwndDlg,
                (DLGPROC)&GlobalsWindowProc,
                (LPARAM)hwndDlg);
            break;
        default:
            break;
        }

    default:
        break;
    }
    return 0;
}
