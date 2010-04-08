/*docstrt----------------------------------------------------------*
 * File:    SUBEDIT.C
 *
 * Description: Functions needed to create an edit control which
 *				restricts the type of input.
 *
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 *
 * Author   Date        Change description
 * -------- ----------- ------------------------------------------*
 * RPB      22-Aug-1993 Created
 * RAW      23-Jan-1995 Added CSC_UNDERSCORE
 * RAW      27-Jan-1995 Use Windows functions to determine if char numeric etc...
 *docend----------------------------------------------------------*/

#define OEMRESOURCE
 
#include <windows.h>
#include <windowsx.h>

#include <string.h>
#include <stdlib.h>

#include "port.h"
#include "subedit.h"
#include "dll.h"

static void OnChar(HWND hwnd, TCHAR ch, int cRepeat);
LRESULT WINAPI __export CommonSubEditCntlProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

/*docstrt----------------------------------------------------------*
 * Function: SubEditCntlProc
 *
 * Description: New window proc for the subclassed edit control  
 *
 * Prototype: LONG FAR PASCAL _export SubEditCntlProc( HWND hWnd,
 *					WORD wMsg, WORD wParam, LONG lParam)
 *
 * Parameter 1- Handle of edit control 
 *           2- Message number
 *			 3- Word of parms
 *			 4- Long word of parms
 *
 * Returns:  Long value
 *           
 *docend----------------------------------------------------------*/

LRESULT WINAPI __export SubEditCntlProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
        HANDLE_MSG(hwnd, WM_CHAR, OnChar);
	}
	return CommonSubEditCntlProc(hwnd, message, wParam, lParam);
}

LRESULT WINAPI __export CommonSubEditCntlProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HGLOBAL			hMem;
	LPSUBCLASSINFO	lpInfo;
	WNDPROC			lpOldProc=NULL;

	hMem=GetProp(hwnd, SUBEDITPROP);
	if (hMem)
	{
		lpInfo=(LPSUBCLASSINFO)GlobalLock(hMem);
        lpOldProc = lpInfo->lpFn;
        GlobalUnlock(hMem);
        return CallWindowProc(lpOldProc, hwnd, message, wParam, lParam);
    }

    return 0;
}

static void OnChar(HWND hwnd, TCHAR ch, int cRepeat)
{
	HGLOBAL			hMem;
	LPSUBCLASSINFO	lpInfo;
	WNDPROC			lpOldProc=NULL;
    BOOL bShift = GetKeyState(VK_SHIFT);
#ifdef DOUBLEBYTE
	static	BOOL	bLastCharDBCSLead=FALSE;
	BOOL	bThisCharDBCSValid=FALSE;
#endif

	hMem=GetProp(hwnd, SUBEDITPROP);
	if (!hMem)
        return;

	lpInfo=(LPSUBCLASSINFO)GlobalLock(hMem);
    if (!lpInfo)
        return;

    lpOldProc=lpInfo->lpFn;
	GlobalUnlock(hMem);

    if (!lpOldProc)
        return;

#ifdef DOUBLEBYTE
	if (bLastCharDBCSLead)
	{
		bLastCharDBCSLead=FALSE;
		bThisCharDBCSValid=TRUE;
	} else
		bThisCharDBCSValid=bLastCharDBCSLead=IsDBCSLeadByte(ch);
#endif

	if (ch == VK_BACK)
    {
        FORWARD_WM_CHAR(hwnd, ch, cRepeat, CommonSubEditCntlProc);
        return;
    }

	if (IsCharAlphaNumeric(ch) && !IsCharAlpha(ch)
		&& lpInfo->wType & CSC_NUMERIC)
    {
        FORWARD_WM_CHAR(hwnd, ch, cRepeat, CommonSubEditCntlProc);
        return;
    }

#ifdef DOUBLEBYTE
	if ((IsCharAlpha(ch) || bThisCharDBCSValid) && (lpInfo->wType & CSC_ALPHA))
#else
	if (IsCharAlpha(ch) && (lpInfo->wType & CSC_ALPHA))
#endif
    {
        FORWARD_WM_CHAR(hwnd, ch, cRepeat, CommonSubEditCntlProc);
        return;
    }

	if ((lpInfo->wType & CSC_NATIONAL) &&
		(ch == 0x0040 || ch == 0x0024 ||
		ch == 0x0023))
    {
        FORWARD_WM_CHAR(hwnd, ch, cRepeat, CommonSubEditCntlProc);
        return;
    }

	if (lpInfo->wType & CSC_SPACE && ch == VK_SPACE)
    {
        FORWARD_WM_CHAR(hwnd, ch, cRepeat, CommonSubEditCntlProc);
        return;
    }

	if ((lpInfo->wType & CSC_QUOTES) &&
		(ch == 0x0027 || ch == 0x0022))
    {
        FORWARD_WM_CHAR(hwnd, ch, cRepeat, CommonSubEditCntlProc);
        return;
    }

	if ((lpInfo->wType & CSC_UNDERSCORE)
		&& (ch == 0x005f && bShift))
    {
        FORWARD_WM_CHAR(hwnd, ch, cRepeat, CommonSubEditCntlProc);
        return;
    }

	if ((lpInfo->wType & CSC_SLASH)
		&& (ch == 0x002f && bShift))
    {
        FORWARD_WM_CHAR(hwnd, ch, cRepeat, CommonSubEditCntlProc);
        return;
    }

	if ((lpInfo->wType & CSC_COLON)
		&& (ch == 0x003A && bShift))
    {
        FORWARD_WM_CHAR(hwnd, ch, cRepeat, CommonSubEditCntlProc);
        return;
    }

	if ((lpInfo->wType & CSC_BACKSLASH)
		&& (ch == 0x005C && bShift))
    {
        FORWARD_WM_CHAR(hwnd, ch, cRepeat, CommonSubEditCntlProc);
        return;
    }
}

/*docstrt----------------------------------------------------------*
 * Function: SubClassEditCntl
 *
 * Description: Subclasses an edit control to limit text input to
 *				a range of values
 *
 * Prototype: WNDPROC SubClassEditCntl( HWND hDlg, int nDlgItem, WORD wType)
 *
 * Parameter 1- Handle of dialog 
 *           2- Control id number
 *			 3- Word indicating allowable character set
 *
 * Returns:  WNDPROC of the original window procedure.
 *           
 *docend----------------------------------------------------------*/

WNDPROC SubClassEditCntl( HWND hDlg, int nDlgItem, WORD wType)
{
	HGLOBAL			hMem;
	HWND			hEdit;
	LPSUBCLASSINFO	lpInfo;
	WNDPROC			lpOldProc=NULL;

	hEdit=GetDlgItem(hDlg, nDlgItem);

	hMem=GlobalAlloc(GHND, sizeof(SUBCLASSINFO));
	if (hMem)
	{
		lpInfo=(LPSUBCLASSINFO)GlobalLock(hMem);
		if (lpInfo)
		{
			SetProp(hEdit, SUBEDITPROP, hMem);
			lpInfo->wType=wType;
			/* subclass Edit control */
			lpInfo->lpFn= (WNDPROC)SetWindowLong(hEdit,
				GWL_WNDPROC,
				(LONG) MakeProcInstance((FARPROC)SubEditCntlProc,
				GetWindowInstance(hDlg)));
			lpOldProc=lpInfo->lpFn;
			GlobalUnlock(hMem);
		}
	}

	return(lpOldProc);
}



/*docstrt----------------------------------------------------------*
 * Function: ChangeEditCntlType
 *
 * Description: Changes the allowable character sets
 *
 * Prototype: void ChangeEditCntlType( HWND hDlg, int nDlgItem, WORD wType)
 *
 * Parameter 1- Handle of dialog 
 *           2- Control id number
 *			 3- Word indicating allowable character set
 *
 * Returns:  None.
 *           
 *docend----------------------------------------------------------*/

void ChangeEditCntlType( HWND hDlg, int nDlgItem, WORD wType)
{
	HGLOBAL		hMem;
	HWND			hEdit;
	LPSUBCLASSINFO	lpInfo;

	hEdit=GetDlgItem(hDlg, nDlgItem);

	hMem=GetProp(hEdit, SUBEDITPROP);
	if (hMem)
	{
		lpInfo=(LPSUBCLASSINFO)GlobalLock(hMem);
		if (lpInfo)
		{
			lpInfo->wType=wType;
			GlobalUnlock(hMem);
		}
	}
}


/*docstrt----------------------------------------------------------*
 * Function: ResetSubClassEditCntl
 *
 * Description: Reset the Subclassing of an edit control
 *
 * Prototype: void ResetSubClassEditCntl(HWND hDlg, int nDlgItem)
 *
 * Parameter 1- Handle of dialog window
 * 			 2- Control id of edit control
 *
 * Returns:  WNDPROC of the original window procedure.
 *           
 *docend----------------------------------------------------------*/

void ResetSubClassEditCntl(HWND hDlg, int nDlgItem)
{
	HGLOBAL		hMem;
	HWND			hEdit;
	LPSUBCLASSINFO	lpInfo;
	WNDPROC 		lpProc;

	/* Get handle of the edit control */
	hEdit=GetDlgItem(hDlg, nDlgItem);

	hMem=GetProp(hEdit, SUBEDITPROP);
	if (hMem)
	{
		lpInfo=(LPSUBCLASSINFO)GlobalLock(hMem);
		if (lpInfo)
		{
			/* Re-establish old proc and clean up instance */
    		lpProc=(WNDPROC)SetWindowLong(hEdit, GWL_WNDPROC,
				(DWORD) lpInfo->lpFn);
			FreeProcInstance((FARPROC)lpProc);
			GlobalUnlock(hMem);
		}
		GlobalFree(hMem);
		RemoveProp(hEdit, SUBEDITPROP);
	}
}
