/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**  Project : CA/OpenIngres Visual DBA
**
**  Source : print.c
**  from vo - manages print
**
**  Author : Unknown
**
**  History:
**	14-Feb-2000 (somsa01)
**	    Include windows.h and windowsx.h, removed typedefs.h .
** 05-Aug-2003 (uk$so01)
**    SIR #106648, Remove printer setting
**/

#define SYS_WIN16
#define WIN32_LEAN_AND_MEAN
//#define STRICT

#include <stdlib.h>    // Moved Emb before aspen.h
#include <windows.h>
#include <windowsx.h>

#include "aspen.h"

#include        "nanmem.e"
#include        "nanact.e"
// Masqued Emb 8/3/95: #include        "nanerr.e"
#include        "global.h"

#include        <commdlg.h>
#include        <dlgs.h>
#include        <drivinit.h>
#include        <memory.h>
#include        <ctype.h>                 
#include        <string.h>
#include        <malloc.h>

#include        "resource.h"  // Added Emb 8/3/95
#include        "stddlg.e"
#include        "print.e"
// Masqued Emb 8/3/95: #include        "help.h"
#include        <time.h>

#include "port.h"

// TO BE FINISHED: the following typedef is in dba.h but there may be some
// interferences with other includes for this source

typedef HWND DISPWND;

#include "esltools.h"  // Added Emb 8/3/95
#include "winutils.h"  // Added Emb 2/5/95 for NanGetWindowsDate/Time

//
// Emb added 9/3/95
//
#include "main.h"
#undef APP_INST
#define APP_INST()   hInst
#undef MALLOCSTR
#define MALLOCSTR(x) ESL_AllocMem(x+1)
#undef FREE
#define FREE(x) ESL_FreeMem(x)

#define MAX_INPUT_LEN   (5)

#define DRIVERNAMELEN           ((USHORT) 40)

static  HDC             hDCCurrentPrinter;
static  HFONT           hFontPrinter;
static  UINT            uiYPageSize=0;  //      in 1/100 of unit, e.g. 1 cm = 100 
static  UINT            uiXPageSize=0;  //      in 1/100 of unit
static  INT             iXRatio;
static  INT             iYRatio;
static  INT             iTopFree;
static  INT             iBottomFree;
static  INT             iLeftFree;
static  INT             iRightFree;

static  UINT            uiFontHeight;
static  INT             iYPos;
static  UINT            uiTabWidth;
static  UINT            uiPrinterTabs;
static  BOOL            fPrintAbort;
static  ABORTPROC       lpfnAbort;
static  DLGPROC         lpfnPDlg;
static  HWND            hwndOwner;
static  HWND            hwndPDlg;
static  BOOL            fPrintHeader;
static  LPSTR           lpstrHeaderFileString;
static  UINT            uiPageNr;
static  LPSTR           lpstrPrintTitle;
static  FARPROC         lpfnOldEdit;
static  RECT            rect;
static  HGLOBAL         hDevMode;   // Emb 14/3: removed NULL (guaranteed)
static  HGLOBAL         hDevNames;  // Emb 14/3: removed NULL (guaranteed)

// Masqued Emb 9/3/95: UINT PASCAL round(double d);  // from round.asm

BOOL WINAPI  PrintPageSetupProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI  PrintEditSub(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static VOID _printLine(LPSTR lpstrBuffer, UINT uiLen);
static VOID _printNewPage(VOID);
static VOID _printFootnote(VOID);

static VOID _PrintPageSetValues(HWND hwnd, UINT uiID, UINT uiValue);
static VOID _PrintPageAddValues(HWND hwnd, UINT uiEditId, INT iAdd);
static UINT _PrintPageGetValues(HWND hwnd, UINT uiID);
static VOID _PrintGetPageDim(LPUINT lpuiWidth, LPUINT lpuiHeight);
static HDC  _GetTempDC(VOID);

static BOOL PrintOnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void PrintOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static HBRUSH PrintOnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type);
static BOOL PageOnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void PageOnDestroy(HWND hwnd);
static void PageOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static void PageOnVScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos);

typedef struct tagPAGEDWLSTRUCT
{
    LPPAGESETUP             lpPageSetup;
    FARPROC                 lpfnEditSub;
} PAGEDWLSTRUCT, FAR * LPPAGEDWLSTRUCT;



//401@WM003
/**********************************
	RETCODE APIFUNC PrintInitSelect
	Description:
	   init the printer system, calling printdlg, using selection option
	Access to Globals:

***********************************/
HDC APIFUNC PrintInitSelect(HWND hwnd, LPSTR lpstrTitle, LPLONG pSelectFlag )
{

  PRINTDLG        pd;
  LPDEVMODE       lpDevMode = NULL;
  LPDEVNAMES      lpDevNames;
  LPSTR           lpszDriverName;
  LPSTR           lpszDeviceName;
  LPSTR           lpszPortName;
  TEXTMETRIC      textMetric;
  FARPROC         lpfnHookProc;
  INT             iPixel;

  _fmemset(&pd, 0, sizeof(PRINTDLG));

  lpstrPrintTitle = (LPSTR) MALLOCSTR(STRINGTABLE_LEN);

  lstrcpy(lpstrPrintTitle, lpstrTitle);

  pd.lStructSize = sizeof(PRINTDLG);
  pd.hwndOwner = hwnd;

  /* print all */ 
  // Masqued Emb 8/3/95 : removed PD_ENABLEPRINTTEMPLATE and PD_ENABLEPRINTHOOK
  // Old code : pd.Flags = PD_NOSELECTION | PD_USEDEVMODECOPIES | PD_RETURNDC | PD_NOPAGENUMS | PD_ENABLEPRINTHOOK | PD_ENABLEPRINTTEMPLATE;/* | PD_SHOWHELP;*/ //361@001 JFS
  pd.Flags = PD_NOSELECTION | PD_USEDEVMODECOPIES | PD_RETURNDC | PD_NOPAGENUMS;

  //401@WM003 allow selection?
  if ( pSelectFlag != NULL && *pSelectFlag != 0 )
    pd.Flags &= ~PD_NOSELECTION;

  lpfnHookProc = (FARPROC) MakeProcInstance ((FARPROC) PrintHookProc, APP_INST());

  pd.lpfnPrintHook = 0; // Masqued Emb 8/3/95 :  ElpfnHookProc;
  pd.lpPrintTemplateName = 0; // Masqued Emb 8/3/95 : MAKEINTRESOURCE(INITPRINTDLG); // 373@001 ER
  pd.hInstance = APP_INST();
  hwndOwner = hwnd;

  if (hDevNames)
  {    
	pd.hDevNames = hDevNames;
  }
  if (hDevMode)
  {
	pd.hDevMode = hDevMode; 
  }
  /* Initialize PRINTDLG structure */

  //360@0001 MS 03/28/94                      
  // Masqued Emb 8/3/95: HelpPushDialogNo(PRINT_BOX);
  if (PrintDlg(&pd) != 0) 
  {
	if (pd.hDC == (HDC) 0)
	{
	  if (pd.hDevNames)
	  {
		lpDevNames = (LPDEVNAMES)GlobalLock(pd.hDevNames);
		lpszDriverName = (LPSTR)lpDevNames + lpDevNames->wDriverOffset;
		lpszDeviceName = (LPSTR)lpDevNames + lpDevNames->wDeviceOffset;
		lpszPortName   = (LPSTR)lpDevNames + lpDevNames->wOutputOffset;
		GlobalUnlock(pd.hDevNames);

		if (pd.hDevMode)
		{
		  lpDevMode = (LPDEVMODE)GlobalLock(pd.hDevMode);
		}

		pd.hDC = CreateDC(lpszDriverName, lpszDeviceName, lpszPortName, (const DEVMODE *)lpDevMode);

		if (pd.hDevMode && lpDevMode)
		{
		  GlobalUnlock(pd.hDevMode);
		}
		GlobalFree(pd.hDevNames);       
	  }
	  if (pd.hDevMode)
	  {               

		GlobalFree(pd.hDevMode);
	  }
	}

	//401@WM003 need selection?
	if ( pSelectFlag != NULL )
	  if ( pd.Flags & PD_SELECTION )
	    *pSelectFlag = TRUE;
	  else
	    *pSelectFlag = FALSE;

  // Emb 9/3/95 : moved in the block of the if(PrintDlg) otherwise erratic!
  if (pd.hDevMode != hDevMode) 
  {
	hDevMode = pd.hDevMode;
  }
  if (pd.hDevNames != hDevNames) 
  {
	hDevNames = pd.hDevNames;
  }

  }
  //360@0001 MS 03/28/94                        
  // Masqued Emb 8/3/95: HelpPopDialogNo();

  FreeProcInstance((FARPROC) lpfnHookProc);

  hDCCurrentPrinter = pd.hDC;

  if (hDCCurrentPrinter)
  {
	SetMapMode(hDCCurrentPrinter, MM_LOMETRIC);
	SetBkMode(hDCCurrentPrinter, TRANSPARENT);
	uiYPageSize = 10 * GetDeviceCaps (hDCCurrentPrinter, VERTSIZE);         // in measurement
	uiXPageSize = 10 * GetDeviceCaps (hDCCurrentPrinter, HORZSIZE);

	iPixel = GetDeviceCaps(hDCCurrentPrinter, VERTRES);             //      pixel
	iYRatio = (INT) iPixel / uiYPageSize;                                   //      pixel per 0.1 mm

	iPixel  = GetDeviceCaps(hDCCurrentPrinter, HORZRES);    //pixel
	iXRatio = (INT) iPixel/ uiXPageSize;                            //pixel per 0.1 mm
	GetTextMetrics(hDCCurrentPrinter, &textMetric);
	hFontPrinter = (HFONT) 0;
	uiPrinterTabs = 4;
	uiTabWidth = 4 * textMetric.tmAveCharWidth;
	uiFontHeight = textMetric.tmHeight + textMetric.tmInternalLeading;
	fPrintHeader = FALSE;
	lpstrHeaderFileString = NULL;

  }

  SaveDC(pd.hDC);

  return (pd.hDC);

}       //      end of PrintInit



/********************************** 
	RETCODE APIFUNC PrintInit(HWND hwnd)    
	Description:
		init the printer system, calling printdlg               
	Access to Globals:

***********************************/
HDC APIFUNC PrintInit(HWND hwnd, LPSTR lpstrTitle)
{

  //401@WM003 just call PrintInitSelect with null parm
  return( PrintInitSelect( hwnd, lpstrTitle, NULL ) );

}       //      end of PrintInit



/********************************** 
	RETCODE APIFUNC PrintSetHeaderFileName(LPSTR lpstrHeaderName)
	Description:
		sets the string to be printed as file name
	Access to Globals:

***********************************/
RETCODE APIFUNC PrintSetHeaderFileName(LPSTR lpstrHeaderName)
{
  RETCODE rc;
  INT             iLen;

  iLen = lstrlen(lpstrHeaderName);

  if (lpstrHeaderFileString != NULL)
  {
	FREE(lpstrHeaderFileString);
	lpstrHeaderFileString = NULL;   // Emb added 9/3/95
  }
  lpstrHeaderFileString = (LPSTR) MALLOCSTR(iLen);
  lstrcpy(lpstrHeaderFileString, lpstrHeaderName);

  rc  = SUCCESS;

  return (rc);

}       //      end of PrintSetHeaderFileName(lpstr lpstrHeaderName)

/********************************** 
	RETCODE APIFUNC PrintSetHeader(BOOL fHeader)
	Description:
		turns header printing on or off
		
	Access to Globals:

***********************************/
RETCODE APIFUNC PrintSetHeader(BOOL     fHeader)
{
  fPrintHeader = fHeader;

  return (SUCCESS);

}       //      end of RETCODE PrintSetHeader(BOOL fHeader)

/********************************** 
	RETCODE APIFUNC PrintEnd(HWND hwnd, HDC hDCPrinter);    //      frees all that was related to the printjob
	Description:
		exits the printer system
	Access to Globals:
		hDCCurrentPrinter

***********************************/
RETCODE APIFUNC PrintExit(HWND hwnd)
{
  RETCODE rc;

  rc = SUCCESS;

  if (hDCCurrentPrinter)
  {
	RestoreDC(hDCCurrentPrinter, -1);
	DeleteDC(hDCCurrentPrinter);
	hDCCurrentPrinter = (HDC) 0;
  }

  if (hFontPrinter)
  {
	DeleteObject(hFontPrinter);
  }

  if (lpstrHeaderFileString)
  {
	FREE(lpstrHeaderFileString);
	lpstrHeaderFileString = NULL;   // Emb added 9/3/95
  }

  if (lpstrPrintTitle)            // Emb added 9/3/95
  {
  FREE(lpstrPrintTitle);
  lpstrPrintTitle = NULL;         // Emb added 9/3/95
  }

  return (rc);

}               // end of PrintEnd(HWND hwnd, HDC hDCPrinter)

/********************************** 
	RETCODE APIFUNC PrintLine(LPSTR lpstrBuffer)

	Description:
		prints a buffer in a line using current font
	Access to Globals:
		hDCCurrentPrinter

***********************************/
RETCODE APIFUNC PrintLine(LPSTR lpstrBuffer)
{

  RETCODE         rc;
  UINT            uiWidth;
  UINT            uiLen;
  UINT            uiCounter;
  UINT            uiComplete;
  BOOL            fDone;
  LPSTR           lpstrStart;

  rc = FAILURE;

  uiLen = lstrlen(lpstrBuffer);
  fDone = FALSE;

  // SetMapMode(hDCCurrentPrinter, MM_LOMETRIC);

  uiWidth = LOWORD(GetTabbedTextExtent(hDCCurrentPrinter, lpstrBuffer, uiLen, 1, &uiTabWidth));
  if (fPrintAbort)
  {
	goto getOut;
  }

  if (uiWidth > uiXPageSize)
  {
	lpstrStart = lpstrBuffer;

	uiComplete = 0;
	while (!fDone)
	{
	  uiWidth = 0;
	  uiCounter = 0;

	  while (uiWidth < uiXPageSize && (uiComplete+uiCounter <= uiLen))
	  {
		uiCounter++;
		uiWidth = LOWORD(GetTabbedTextExtent(hDCCurrentPrinter, lpstrStart, uiCounter, 1, &uiTabWidth));
	  }
	  if (fPrintAbort)
	  {
		goto getOut;
	  }

	  uiCounter--;
	  _printLine(lpstrStart, uiCounter);
	  uiComplete += uiCounter;
	  if (uiComplete < uiLen)
	  {
		lpstrStart = (LPSTR) (lpstrStart+uiCounter);
	  }
	  else
	  {
		fDone = TRUE;
		rc = SUCCESS;
	  }
	}
  }
  else
  {
	_printLine(lpstrBuffer, lstrlen(lpstrBuffer));
	rc = SUCCESS;
  }

  getOut: 

  return (rc);

}       //      end of RETCODE APIFUNC PrintLine(LPSTR lpstrBuffer)

/********************************** 
	VOID    _printLine(LPSTR lpstrBuffer)

	Description:
		prints a buffer in a line using current font
	Access to Globals:
		hDCCurrentPrinter

***********************************/
static VOID _printLine(LPSTR lpstrBuffer, UINT uiLen)
{
  LONG lRet;

  // SetMapMode(hDCCurrentPrinter, MM_LOMETRIC);

  if (iYPos == 0)
  {
	_printNewPage();

  }
  if (hFontPrinter)
  {

	SelectObject(hDCCurrentPrinter, hFontPrinter);
  }
  lRet = TabbedTextOut(hDCCurrentPrinter,0,  (INT) -iYPos, lpstrBuffer, uiLen, 1, &uiTabWidth, 0);
  iYPos += (uiFontHeight);
  //   if (iYPos+uiFontHeight+uiFontHeight > uiYPageSize)
	if (iYPos+(uiFontHeight*3) > uiYPageSize)
	{
	  iYPos += uiFontHeight;
	  _printFootnote();
	  EndPage(hDCCurrentPrinter);
	  iYPos = 0;
	}

  return;

}       //      end of static VOID _printline
/*------------------------------------------------------------------*/
static VOID _printFootnote()
{
  CHAR    achHeader[PRINT_HEADERSIZE];
  CHAR    achDate[20];
  CHAR    achTime[20];
  INT     iXPos;

  NanGetWindowsDate(achDate);
  NanGetWindowsTime(achTime);
  lstrcpy(achHeader, achDate);
  lstrcat(achHeader, " "); 
  lstrcat(achHeader, achTime);
  wsprintf(achDate, "%u", uiPageNr);
  iXPos = uiXPageSize - LOWORD(GetTextExtent(hDCCurrentPrinter, "9999", 4));
  iYPos = uiYPageSize - (2 * uiFontHeight);
  TabbedTextOut(hDCCurrentPrinter,0,  -iYPos, achHeader, lstrlen(achHeader), 1, &uiTabWidth, 0);
  TextOut(hDCCurrentPrinter, iXPos, -iYPos,achDate, lstrlen(achDate));
  return;
}       

/********************************** 
	static VOID _printNewPage()

	Description:
		starts a new page, prints header if fPrintHeader=TRUE
	Access to Globals:
		hDCCurrentPrinter

***********************************/
static VOID _printNewPage()
{
  CHAR    achHeader[PRINT_HEADERSIZE] = "\0";
  //   CHAR    achDate[20];
  INT             iXPos;

  StartPage(hDCCurrentPrinter);
  if (fPrintHeader)
  {
	SetMapMode(hDCCurrentPrinter, MM_LOMETRIC);
	//   NanGetWindowsDate(achDate);
	//   lstrcpy(achHeader, achDate);

	if (lpstrHeaderFileString)
	{
	  //  lstrcat(achHeader, "\t\t");
	  //  lstrcat(achHeader, lpstrHeaderFileString);
	  lstrcpy(achHeader, lpstrHeaderFileString);
	}

	//   wsprintf(achDate, "%u", uiPageNr);
	if (hFontPrinter)
	{
	  SelectObject(hDCCurrentPrinter, hFontPrinter);
	}
	iXPos = uiXPageSize - LOWORD(GetTextExtent(hDCCurrentPrinter, "9999", 4));
	TabbedTextOut(hDCCurrentPrinter,0, -iYPos, achHeader, lstrlen(achHeader), 1, &uiTabWidth, 0);
	//   TextOut(hDCCurrentPrinter, iXPos, -iYPos,achDate, lstrlen(achDate));
	iYPos += (uiFontHeight+uiFontHeight);
  }     
  uiPageNr++;
  return;

}       // end of static VOID _printNewPage()

/********************************** 
	RETCODE APIFUNC PrintStartJob()

	Description:
		starts a printjob 
	Access to Globals:
		hDCCurrentPrinter

***********************************/
RETCODE APIFUNC PrintStartJob(LPSTR lpstrDocName, LPSTR lpstrFileName)
{
  DOCINFO         docInfo;
  RETCODE         rc;

  rc = FAILURE;

  if (hDCCurrentPrinter)
  {
   memset (&docInfo, 0, sizeof(docInfo));

	docInfo.cbSize = sizeof(DOCINFO);
	docInfo.lpszDocName = lpstrDocName;
	docInfo.lpszOutput = lpstrFileName;

	fPrintAbort = FALSE;

	/* Make instances of the Abort proc. and the Print dialog function */
	lpfnAbort = (ABORTPROC) MakeProcInstance ((FARPROC) PrintAbortProc, APP_INST());
	lpfnPDlg  = (DLGPROC) MakeProcInstance ((FARPROC) PrintDlgProc, APP_INST());

	/* Disable the main application window and create the Cancel dialog */
	EnableWindow (hwndOwner, FALSE);
	hwndPDlg = CreateDialog (APP_INST(), MAKEINTRESOURCE(ABORTPRINTDIALOG), hwndOwner, lpfnPDlg); // 373@001 ER
	ShowWindow (hwndPDlg, SW_SHOW);
	UpdateWindow (hwndPDlg);

	SetAbortProc(hDCCurrentPrinter, lpfnAbort);             

	if (StartDoc(hDCCurrentPrinter, &docInfo) > 0)
	{
	  iYPos = 0;
	  uiPageNr = 0;
	  rc = SUCCESS;
	}

  }       
  return (rc);

}       //      end of RETCODE APIFUNC PrintStartJob(LPSTR lpstrDocName, LPSTR lpstrFileName)

/********************************** 
	RETCODE APIFUNC PrintSetPage(UINT iTop, UINT uiLeft, UINT uiBottom, UINT uiRight);
	Description:
		sets the page borders
	Access to Globals:
		hDCCurrentPrinter

***********************************/
RETCODE APIFUNC PrintSetPage(LPPAGESETUP lpPageSetup)
{
  RETCODE rc;
  rc = FAILURE;

  return (rc);
}

/********************************** 
	RETCODE APIFUNC PrintQueryPage(LPPAGESETUP lpPageSetup);
	Description:
		opens the print page setup dialog
	Access to Globals:
***********************************/
RETCODE APIFUNC PrintQueryPage(HWND hwndParent, LPPAGESETUP lpPageSetup)
{
  RETCODE         rc;
  DLGPROC         lpfnDlg;

  rc = FAILURE;

  lpfnDlg = (DLGPROC) MakeProcInstance((FARPROC) PrintPageSetupProc, APP_INST());
  // 373@001 ER
  if (DialogBoxParam(APP_INST(), MAKEINTRESOURCE(PRINTPAGESETUP), hwndParent, lpfnDlg, (LPARAM) lpPageSetup))
  {
	rc = SUCCESS;

  }

  FreeProcInstance ((FARPROC) lpfnDlg);
  return (rc);
}

/********************************** 
	RETCODE APIFUNC PrintSetTabWidth
	Description:
		sets the tabwidth
	Access to Globals:
		hDCCurrentPrinter

***********************************/
RETCODE APIFUNC PrintSetTabs(UINT uiTabs)
{
  TEXTMETRIC      textMetric;

  uiPrinterTabs = uiTabs;
  if (hDCCurrentPrinter)
  {
	if (hFontPrinter)
	{
	  SelectObject(hDCCurrentPrinter, hFontPrinter);
	}
	// SetMapMode(hDCCurrentPrinter, MM_LOMETRIC);
	GetTextMetrics(hDCCurrentPrinter, &textMetric);
	uiTabWidth = uiPrinterTabs * textMetric.tmAveCharWidth;
  }

  return (SUCCESS);

}       // end of RETCODE       PrintSetTabWidth(UINT uiWidth)

/********************************** 
	RETCODE APIFUNC PrintEndJob(VOID)
	Description:
		ends a printjob 
	Access to Globals:
		hDCCurrentPrinter

***********************************/
RETCODE APIFUNC PrintEndJob(VOID)
{
  RETCODE         rc;

  rc = FAILURE;

  if (hDCCurrentPrinter)
  {
	if (fPrintAbort)
	{
	  AbortDoc(hDCCurrentPrinter);
	  rc = PRINT_JOBABORTED;
	}
	else
	{
	  _printFootnote();
	  EndPage(hDCCurrentPrinter);
	  EndDoc(hDCCurrentPrinter);
	  rc = SUCCESS;
	}
	EnableWindow (hwndOwner, TRUE);
	DestroyWindow (hwndPDlg);
	FreeProcInstance ((FARPROC) lpfnPDlg);
	FreeProcInstance ((FARPROC) lpfnAbort);

  }       
  return (rc);

}       //      end of RETCODE APIFUNC PrintEndJob(VOID)

/********************************** 
	RETCODE APIFUNC PrintSetFont(LPFONTINFO lpFontInfo)

	Description:
		sets the current text font
	Access to Globals:
		hDCCurrentPrinter

***********************************/
RETCODE APIFUNC PrintSetFont(LPFONTINFO lpFontInfo)
{
  RETCODE rc;
  LOGFONT logFont;
  HFONT   hOldFont;
  TEXTMETRIC      textMetric;                 
  int iMM, iPix;

  rc = FAILURE;

  if (hDCCurrentPrinter)
  {
	hOldFont = hFontPrinter;

	SetMapMode(hDCCurrentPrinter, MM_LOMETRIC);

	//      @MS0001 - Convert from points to .1 of MM                                
	CreateLogFont(lpFontInfo, &logFont, hDCCurrentPrinter);
	iMM = GetDeviceCaps(hDCCurrentPrinter, VERTSIZE);
	iPix = GetDeviceCaps(hDCCurrentPrinter, VERTRES);
	logFont.lfHeight = (int) (((float) lpFontInfo->iPoints*(float)GetDeviceCaps(hDCCurrentPrinter, LOGPIXELSY))/ (float) 72.0);
	logFont.lfHeight = (int) (10.0*((float) logFont.lfHeight*((float) ((float) iMM/ (float) iPix))));
	hFontPrinter = CreateFontIndirect(&logFont);
	SelectObject(hDCCurrentPrinter, hFontPrinter);

	GetTextMetrics(hDCCurrentPrinter, &textMetric);
	uiTabWidth = uiPrinterTabs * textMetric.tmAveCharWidth;

	uiFontHeight = textMetric.tmHeight + textMetric.tmInternalLeading;
	rc = SUCCESS;

	if (hOldFont) 
	{
	  DeleteObject(hOldFont);
	}
  }

  return (rc);

}       //      end of  RETCODE APIFUNC PrintSetFont(HFONT hFont)

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : AbortProc()                                                *
 *                                                                          *
 *  PURPOSE    : To be called by GDI print code to check for user abort.    *
 *                                                                          *
 ****************************************************************************/
int WINAPI PrintAbortProc (HDC hdc, WORD reserved )
{
  MSG msg;

  /* Allow other apps to run, or get abort messages */
  while (!fPrintAbort && PeekMessage (&msg, ZERO, ZERO, ZERO, TRUE))
  {
	if (!hwndPDlg || !IsDialogMessage (hwndPDlg, &msg))
	{
	  TranslateMessage (&msg);
	  DispatchMessage  (&msg);
	}
  }
  return !fPrintAbort;
}       //      end of int FAR PASCAL AbortProc (HDC hdc, WORD reserved )

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : PrintDlgProc ()                                            *
 *                                                                          *
 *  PURPOSE    : Dialog function for the print cancel dialog box.           *
 *                                                                          *
 *  RETURNS    : TRUE  - OK to abort/ not OK to abort                       *
 *               FALSE - otherwise.                                         *
 *                                                                          *
 ****************************************************************************/
BOOL WINAPI __export PrintDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    HANDLE_MSG(hwnd, WM_INITDIALOG, PrintOnInitDialog);
    HANDLE_MSG(hwnd, WM_COMMAND, PrintOnCommand);
#ifdef  WIN32
    HANDLE_MSG(hwnd, WM_CTLCOLORSTATIC, PrintOnCtlColor);
#else
    HANDLE_MSG(hwnd, WM_CTLCOLOR, PrintOnCtlColor);
#endif
  }
  return FALSE;
}       // end of PrintDlgProc(HWND hwnd, WORD msg, WORD wParam, LONG lParam)


static BOOL PrintOnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	SetDlgItemText(hwnd, IDD_PABORT_TEXT, lpstrPrintTitle);
	return (FALSE);
}

static void PrintOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	if (id == IDD_PABORT_CANCEL)
	{
	  /* abort printing if the only button gets hit */
	  fPrintAbort = TRUE;
	}
}

static HBRUSH PrintOnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type)
{
    switch (type)
    {
	case CTLCOLOR_STATIC:
	{
	  if (hwndChild==GetDlgItem(hwnd, IDD_PABORT_TEXT))
	  {
		int iLen=(int) SendDlgItemMessage(hwnd, IDD_PABORT_TEXT,WM_GETTEXTLENGTH,0,0L);
		if (iLen)
		{
		  LPSTR lpTxt=(LPSTR) GlobalAllocPtr(GHND,iLen+1);
		  if (lpTxt)
		  {
			if (SendDlgItemMessage(hwnd, IDD_PABORT_TEXT,WM_GETTEXTLENGTH,iLen+1,(LPARAM)(LPVOID) lpTxt))
			{
			  RECT rc;
			  GetClientRect(GetDlgItem(hwnd, IDD_PABORT_TEXT),&rc);
			  DrawText(hdc,lpTxt,iLen,&rc,DT_CENTER|DT_NOPREFIX|DT_VCENTER|DT_WORDBREAK);
			}
		   GlobalFreePtr(lpTxt);
		  }
		}
	  }
	  break;
	}
    }

    return (HBRUSH)NULL;
}

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : PrintDlgProc ()                                            *
 *                                                                          *
 *  PURPOSE    : Dialog function for the print cancel dialog box.           *
 *                                                                          *
 *  RETURNS    : TRUE  - OK to abort/ not OK to abort                       *
 *               FALSE - otherwise.                                         *
 *                                                                          *
 ****************************************************************************/
LRESULT WINAPI PASCAL __export PrintHookProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
  case WM_INITDIALOG:
	if (lpstrPrintTitle)
	{
	  SetWindowText(hwnd, lpstrPrintTitle);
	}
	ShowWindow(GetDlgItem(hwnd,rad3 ), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, stc2), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, stc3), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, edt1), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, edt2), SW_HIDE);
	return (FALSE);
  }
  return FALSE;
}       // end of PrintHookProc(HWND hwnd, WORD msg, WORD wParam, LONG lParam)

/********************************** 
	BOOL WINAPI  PrintPageSetupProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	Description:
		dialog proc for pagesetup dialog 
	Access to Globals:
		

***********************************/
BOOL WINAPI PrintPageSetupProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    HANDLE_MSG(hwnd, WM_COMMAND, PageOnCommand);
    HANDLE_MSG(hwnd, WM_INITDIALOG, PageOnInitDialog);
    HANDLE_MSG(hwnd, WM_DESTROY, PageOnDestroy);
    HANDLE_MSG(hwnd, WM_VSCROLL, PageOnVScroll);
  }       //      end of message switch

  return FALSE;
}       //      end of PrintPageSetupProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

static BOOL PageOnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
  UINT                    uiWidth;
  UINT                    uiHeight;
  LPPAGEDWLSTRUCT lpdata;

  lpdata = (LPPAGEDWLSTRUCT)ESL_AllocMem(sizeof(PAGEDWLSTRUCT));
  if (!lpdata)
  {
    EndDialog(hwnd, -1);
    return FALSE;
  }
  else
  {
    SetWindowLong(hwnd, DWL_USER, (long)lpdata);
  }

	lpdata->lpPageSetup = (LPPAGESETUP) lParam;

	_PrintPageSetValues(hwnd, IDD_PSETUP_TOPEDIT, PAGE_TOP(lpdata->lpPageSetup));
	_PrintPageSetValues(hwnd, IDD_PSETUP_LEFTEDIT, PAGE_LEFT(lpdata->lpPageSetup));
	_PrintPageSetValues(hwnd, IDD_PSETUP_RIGHTEDIT, PAGE_RIGHT(lpdata->lpPageSetup));
	_PrintPageSetValues(hwnd, IDD_PSETUP_BOTTOMEDIT, PAGE_BOTTOM(lpdata->lpPageSetup));

	_PrintGetPageDim(&uiWidth, &uiHeight);

	_PrintPageSetValues(hwnd, IDD_PSETUP_HEIGHT, uiHeight);
	_PrintPageSetValues(hwnd, IDD_PSETUP_WIDTH, uiWidth);

	CheckDlgButton(hwnd, IDD_PSETUP_CM, PAGE_CM(lpdata->lpPageSetup));
	CheckDlgButton(hwnd, IDD_PSETUP_INCH, !PAGE_CM(lpdata->lpPageSetup));

	lpdata->lpfnEditSub = MakeProcInstance((FARPROC) PrintEditSub, APP_INST());
	lpfnOldEdit = (FARPROC)SetWindowLong(GetDlgItem(hwnd, IDD_PSETUP_TOPEDIT),GWL_WNDPROC,(LONG) lpdata->lpfnEditSub);
	lpfnOldEdit = (FARPROC)SetWindowLong(GetDlgItem(hwnd, IDD_PSETUP_LEFTEDIT),GWL_WNDPROC,(LONG) lpdata->lpfnEditSub);
	lpfnOldEdit = (FARPROC)SetWindowLong(GetDlgItem(hwnd, IDD_PSETUP_BOTTOMEDIT),GWL_WNDPROC,(LONG) lpdata->lpfnEditSub);
	lpfnOldEdit = (FARPROC)SetWindowLong(GetDlgItem(hwnd, IDD_PSETUP_RIGHTEDIT),GWL_WNDPROC,(LONG) lpdata->lpfnEditSub);
	return (FALSE);
}

static void PageOnDestroy(HWND hwnd)
{
    LPPAGEDWLSTRUCT lpdata = (LPPAGEDWLSTRUCT)GetWindowLong(hwnd, DWL_USER);
	SetWindowLong(GetDlgItem(hwnd, IDD_PSETUP_TOPEDIT),GWL_WNDPROC,(LONG) lpfnOldEdit);
	SetWindowLong(GetDlgItem(hwnd, IDD_PSETUP_LEFTEDIT),GWL_WNDPROC,(LONG) lpfnOldEdit);
	SetWindowLong(GetDlgItem(hwnd, IDD_PSETUP_BOTTOMEDIT),GWL_WNDPROC,(LONG) lpfnOldEdit);
	SetWindowLong(GetDlgItem(hwnd, IDD_PSETUP_RIGHTEDIT),GWL_WNDPROC,(LONG) lpfnOldEdit);
	FreeProcInstance(lpdata->lpfnEditSub);
    ESL_FreeMem(lpdata);
}

static void PageOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{
	case IDOK:
	case IDD_PSETUP_OKAY:
	  EndDialog(hwnd, TRUE);
	  break;

	case IDCANCEL:
	case IDD_PSETUP_CANCEL:
	  EndDialog(hwnd, FALSE);
	  break;

	}
}

static void PageOnVScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
{
  INT                             iAdd;
  INT                             iEditId;

	switch (code)
	{
	case SB_LINEUP:
	  iAdd = 1;
	  break;
	case SB_LINEDOWN:
	  iAdd = -1;
	  break;

	default:
	  iAdd = 0;
	}

	if (iAdd != 0)
	{
	  switch (GetDlgCtrlID(hwndCtl))
	  {
	  case IDD_PSETUP_TOPSCROLL:
		iEditId = IDD_PSETUP_TOPEDIT;
		break;

	  case IDD_PSETUP_LEFTSCROLL:
		iEditId = IDD_PSETUP_LEFTEDIT;
		break;

	  case IDD_PSETUP_BOTTOMSCROLL:
		iEditId = IDD_PSETUP_BOTTOMEDIT;
		break;

	  case IDD_PSETUP_RIGHTSCROLL:
		iEditId = IDD_PSETUP_RIGHTEDIT;
		break;

	  }
	  _PrintPageAddValues(hwnd, iEditId, iAdd);

	}       //      iAdd != 0
}


/********************************** 
	static VOID     _PrintPageSetValues(HWND hwnd, UINT uiID, UINT uiValue)

	Description:
		sets the double like string in the dialog
	Access to Globals:
		

***********************************/
static VOID     _PrintPageSetValues(HWND hwnd, UINT uiID, UINT uiValue)
{
  INT             iHun, iTen, iOne;
  CHAR    achValues[MAX_INPUT_LEN];

  iHun = (INT) (uiValue / 100); 
  iTen = (INT) ((uiValue - (iHun*100)) / 10);
  iOne = (INT) (uiValue - (iHun*100) - (iTen*10));
  wsprintf(achValues, "%d.%d%d", iHun, iTen, iOne);
  SetDlgItemText(hwnd, uiID, achValues);

  return;
}

/********************************** 
	static VOID     _PrintPageAddValues(HWND hwnd, UINT uiID, INT iAdd)

	Description:
		adds a unit to the double like string in the dialog
	Access to Globals:
		

***********************************/
static VOID _PrintPageAddValues(HWND hwnd, UINT uiEditId, INT iAdd)
{
  UINT    uiCurrent;

  uiCurrent = _PrintPageGetValues(hwnd, uiEditId);
  if (!(uiCurrent == 0 && iAdd < 0) || (uiCurrent > 0))
  {
	uiCurrent += iAdd;
	_PrintPageSetValues(hwnd, uiEditId, uiCurrent);
  }

  return;
}

/********************************** 
	static UINT     _PrintPageGetValues(HWND hwnd, UINT uiID)

	Description:
		gets the current value of a field in the dialog
	Access to Globals:
		

***********************************/
static UINT     _PrintPageGetValues(HWND hwnd, UINT uiID)
{
  UINT    uiResult;
  CHAR    achValues[MAX_INPUT_LEN];       

  if (GetDlgItemText(hwnd, uiID, achValues, MAX_INPUT_LEN) > 0)
  {
  // Changed Emb 9/3/95: uiResult = (UINT) round(100 * atof(achValues));
	uiResult = (UINT) (100.0 * atof(achValues));
  }               
  else
  {
	uiResult = 0;
  }

  return (uiResult);
}

/********************************** 
	LRESULT WINAPI  PrintEditSub

	Description:
		subclass function for numeric input boxes
	Access to Globals:
***********************************/
LRESULT WINAPI PrintEditSub(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  CHAR    achValue[MAX_INPUT_LEN];
  BOOL    fDecPoint;
  SHORT   sCount;
  BOOL    fRet;

  fRet = TRUE;

  switch (msg)
  {
  case WM_CHAR:
	GetWindowText(hwnd, achValue, MAX_INPUT_LEN);
	sCount = 0;
	fDecPoint = FALSE;
	while (achValue[sCount])
	{
	  if (achValue[sCount] == '.' || achValue[sCount] == ',')
	  {
		fDecPoint = TRUE;
	  }
	  sCount++;
	}
	switch ((CHAR) wParam)
	{
	case VK_RETURN:
	case VK_BACK:
	  break;

	case '.':
	case ',':
	  if (fDecPoint)
	  {
		fRet = FALSE;
	  }
	  break;

	default:
	  fRet = isdigit((CHAR) wParam);
	  break;
	}
	break;
  }
  if (fRet)
  {
  // Emb 14/3 : manage STRICT define
  #ifdef STRICT
	return  (CallWindowProc((WNDPROC) lpfnOldEdit,hwnd,msg,wParam,lParam));
  #else
	return  (CallWindowProc((FARPROC) lpfnOldEdit,hwnd,msg,wParam,lParam));
  #endif
  }
  else
  {
	MessageBeep(MB_ICONINFORMATION);
  }
  return (0L);

}

/********************************** 
	static VOID _PrintGetPageDim(LPUINT lpuiWidth, LPUINT lpuiHeight);

	Description:
		returns current page dimemsions 
	Access to Globals:
***********************************/
static VOID _PrintGetPageDim(LPUINT lpuiWidth, LPUINT lpuiHeight)
  {
  HDC             hDCTemp;

  *lpuiHeight = 0;
  *lpuiWidth      = 0;

  if (hDCCurrentPrinter)
  {
	*lpuiHeight =   uiYPageSize;
	*lpuiWidth =    uiXPageSize;
  }
  else
  {
	hDCTemp = _GetTempDC(); 
	if (hDCTemp)
	{
	  SetMapMode(hDCTemp, MM_LOMETRIC);
	  *lpuiHeight = 10 * GetDeviceCaps (hDCTemp, VERTSIZE);
	  *lpuiWidth = 10 * GetDeviceCaps (hDCTemp, HORZSIZE);
	  DeleteDC(hDCTemp);
	}
  }
  return;
}

/********************************** 
	static HDC      _GetTempDC(VOID)

	Description:
		returns a temp DC for the printer, if PrintInit was not called
	Access to Globals:
***********************************/
static HDC      _GetTempDC(VOID)
{
  CHAR    achPrinter[STRINGTABLE_LEN+1];
  LPSTR   lpstrDevice;
  LPSTR   lpstrDriver;
  LPSTR   lpstrOutput;

  GetProfileString("windows", "device", ",,,", achPrinter, STRINGTABLE_LEN);
  if ((lpstrDevice = strtok(achPrinter,",")) &&
	(lpstrDriver = strtok(NULL      ,",")) &&
	(lpstrOutput = strtok(NULL      ,",")))
  {
	return (CreateDC(lpstrDriver, lpstrDevice, lpstrOutput, NULL));
  }
  return ((HDC) 0);
}

RETCODE APIFUNC PrintEject()
{   
  _printFootnote();
  iYPos = 0;
  EndPage(hDCCurrentPrinter);
  _printNewPage();
  return TRUE;
}       
