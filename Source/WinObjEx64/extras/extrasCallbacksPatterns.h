/*******************************************************************************
*
*  (C) COPYRIGHT AUTHORS, 2019
*
*  TITLE:       EXTRASCALLBACKSPATTERNS.H
*
*  VERSION:     1.82
*
*  DATE:        18 Nov 2019
*
*  Header with search patterns used by Callbacks dialog routines.
*
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
* ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
* TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
*******************************************************************************/

#pragma once

//
// PsAltSystemCallHandlers
//
#define MAX_ALT_SYSTEM_CALL_HANDLERS 2

BYTE PsAltSystemCallHandlersPattern[] = {
    0x4C, 0x8D, 0x35
};
