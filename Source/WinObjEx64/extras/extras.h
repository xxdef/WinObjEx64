/*******************************************************************************
*
*  (C) COPYRIGHT AUTHORS, 2015 - 2022
*
*  TITLE:       EXTRAS.H
*
*  VERSION:     1.94
*
*  DATE:        07 Jun 2022
*
*  Common header file for Extras dialogs.
*
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
* ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
* TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
*******************************************************************************/
#pragma once

typedef struct _EXTRASCONTEXT {
    HWND hwndDlg;
    HWND ListView;
    HWND TreeList;
    HWND StatusBar;
    HIMAGELIST ImageList;
    INT lvColumnToSort;
    INT lvColumnCount;
    INT lvColumnHit;
    INT lvItemHit;
    INT tlSubItemHit;
    BOOL bInverseSort;
    union {
        ULONG_PTR Reserved;
        ULONG_PTR DialogMode;
    };
    HICON ObjectIcon;
    HICON DialogIcon;
} EXTRASCONTEXT, *PEXTRASCONTEXT;

typedef struct _EXTRASCALLBACK {
    ULONG_PTR lParam;
    ULONG_PTR Value;
} EXTRASCALLBACK, *PEXTRASCALLBACK;

typedef enum _IPC_DLG_MODE {
    IpcModeNamedPipes = 0,
    IpcModeMailSlots = 1,
    IpcMaxMode = 2
} IPC_DLG_MODE;

typedef enum _DRIVERS_DLG_MODE {
    DrvModeNormal = 0,
    DrvModeUnloaded = 1,
    DrvModeMax = 2
} DRIVERS_DLG_MODE;

typedef enum _SSDT_DLG_MODE {
    SST_Ntos = 0,
    SST_Win32k = 1,
    SST_Max = 2
} SSDT_DLG_MODE;

typedef INT(CALLBACK *DlgCompareFunction)(
    _In_ LPARAM lParam1,
    _In_ LPARAM lParam2,
    _In_ LPARAM lParamSort
    );

typedef BOOL(CALLBACK *CustomNotifyFunction)(
    _In_ LPNMLISTVIEW nhdr,
    _In_ EXTRASCONTEXT *Context,
    _In_opt_ PVOID Parameter
    );

VOID extrasSimpleListResize(
    _In_ HWND hwndDlg);

VOID extrasSetDlgIcon(
    _In_ EXTRASCONTEXT* Context);

VOID extrasRemoveDlgIcon(
    _In_ EXTRASCONTEXT* Context);

VOID extrasShowDialogById(
    _In_ WORD DialogId);

VOID extrasHandleSettingsChange(
    EXTRASCONTEXT* Context);
