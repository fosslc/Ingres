//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
// PAN/LCM source control information:
//
//  Archive name    ^[Archive:g:\vo4clip\include\arch\print.e]
//  Modifier        ^[Author:mulwi01(1000)]
//  Base rev        ^[BaseRev:1.0]
//  Latest rev      ^[Revision:2.0]
//  Latest rev time ^[Timestamp:Mon Oct 03 17:16:18 1994]
//  Check in time   ^[Date:Tue Oct 04 10:05:02 1994]
//  Change log      ^[Mods:// ]
// 
// Mon Oct 03 17:16:18 1994, Rev 2.0, mulwi01(1000)
//  401@WM003 add PrintInitSelect
// 
//
//
//////////////////////////////////////////////////////////////////////////////
//
//  Taken from VO March 8, 95 -  adapted by Emb - seach for "Emb" comments
//

#include "port.h"

//
//      struct used for printsetpage
//
typedef struct _PAGESETUP
{
	UINT    uiTop;                                  //      top offset...   those values are always in 1/100 of unit
	UINT    uiLeft;
	UINT    uiBottom;
	UINT    uiRight;
	BOOL    fCmOrInches;                    //      if cm the TRUE, else FALSE
}       PAGESETUP, FAR * LPPAGESETUP;

#define PAGE_TOP(p)                     ((p)->uiTop)
#define PAGE_LEFT(p)            ((p)->uiLeft)
#define PAGE_BOTTOM(p)          ((p)->uiBottom)
#define PAGE_RIGHT(p)           ((p)->uiRight)
#define PAGE_CM(p)                      ((p)->fCmOrInches)

BOOL    APIFUNC PrintSetup(HWND hwnd);
HDC	APIFUNC PrintInit(HWND hwnd, LPSTR lpstrTitle); //	calls the printdlg function and returns the hDCPrinter
HDC	APIFUNC PrintInitSelect(HWND hwnd, LPSTR lpstrTitle, LPLONG pFlag); //401@WM003 allow selection
RETCODE APIFUNC PrintExit(HWND hwnd);                                   //      frees all that was related to the printjob
RETCODE APIFUNC PrintSetFont(LPFONTINFO lpFontInfo);    //      sets the current font for textoutput
RETCODE APIFUNC PrintSetTabs(UINT uiTabs);                      //      sets the current tabwidth char per tab
RETCODE APIFUNC PrintEndJob(VOID);
RETCODE APIFUNC PrintStartJob(LPSTR lpstrDocName, LPSTR lpstrFileName);
RETCODE APIFUNC PrintEject();


//              the PrintSetPage is just setting the page values
//              variables are used to init and to return
//              units in cm/zoll
RETCODE APIFUNC PrintSetPage(LPPAGESETUP lpPageSetup);

//              the PrintQueryPage is opening the page setup dialog
//              variables are used to init and to return
//              units in cm/zoll
RETCODE APIFUNC PrintQueryPage(HWND hwndParent, LPPAGESETUP lpPageSetup);

RETCODE APIFUNC PrintLine(LPSTR lpstrBuffer);
RETCODE APIFUNC PrintSetHeaderFileName(LPSTR lpstrHeaderName);          //      sets the name of file for current header 
RETCODE APIFUNC PrintSetHeader(BOOL     fHeader);                                       //      turns header printing on or off


LRESULT WINAPI PASCAL  __export PrintHookProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL    WINAPI  __export PrintDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
int     WINAPI  __export PrintAbortProc(HDC hdc, WORD reserved );



#define PRINT_JOBABORTED                        ((RETCODE) 1)

#define PRINT_HEADERSIZE                        ((UINT) 200)            //      max size of chars for complete header
