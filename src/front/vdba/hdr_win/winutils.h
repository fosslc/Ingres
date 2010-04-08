/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project : CA/OpenIngres Visual DBA
//
//    Source : winutils.h
//    Set of utility functions specific for windows
//
********************************************************************/

// Dialog boxes utility functions
void CenterDialog(HWND hDlg);
BOOL HDLGSetProp(HWND hDlg,LPVOID lpData);
LPVOID HDLGGetProp(HWND hDlg);
VOID HDLGUnlockProp(HWND hDlg);
HANDLE HDLGRemoveProp(HWND hDlg);
long LaunchIngstartDialogBox(HWND hWnd);

// Hour glass functions
void ShowHourGlass(void);
void RemoveHourGlass(void);

// Icons and bitmaps caches functions
BOOL    IconCacheInit(VOID);
HICON   IconCacheLoadIcon(WORD idIcon);
VOID    IconCacheFree(VOID);

BOOL    BitmapCacheInit(VOID);
HBITMAP BitmapCacheLoadBitmap(WORD idBitmap);
VOID    BitmapCacheFree(VOID);
VOID    ErrorMsgBox(UINT msgId);


// resources functions

// No checks for this function  !!!! You have to be sure before using it
extern LPSTR LoadStr(UINT ui,LPSTR lp);
extern VOID  PostMessageToTrees(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
// international functions
BOOL NanGetWindowsDate(LPSTR lpstrDate);
BOOL NanGetWindowsTime(LPSTR lpstrTime);

// container utility functions
int CalculateComboHeight(HWND hwndCntr);

