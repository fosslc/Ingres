/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : find.c
//
//    Dialog box for Searching an object
//
********************************************************************/

#include "dll.h"
#include <commdlg.h>
#include "dbadlg1.h"
#include "dlgres.h"

typedef struct tagTOOLSFINDPROP
{
	UINT uWinFindMessage;
	UINT uFindMessage;
}	TOOLSFINDPROP, FAR * LPTOOLSFINDPROP;

char szToolsFindProp[] = "TOOLSFINDPROP";
char szFRStruct[] = "FRSTRUCT";

#define LM_DESTROY		WM_USER + 1

LRESULT CALLBACK __export DlgFindWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy(HWND hwnd);

BOOL WINAPI __export DlgFind (HWND hwndOwner, LPTOOLSFIND lpToolsFind)
/*
	Function:
		Creates a modeless dialog box that can be used to perform search
		operations.

	Parameters:
		hwndOwner		- Handle to the owner window
		lpToolsFind		- Points to structure containing information used to 
							- initialise the dialog. The dialog uses the same
							- structure to return the result of the dialog.

	Returns:
		TRUE if successful.

	Note:
		Since the dialog created is modeless,  the IsDialogMessage function
		should be called in the applications main message loop.

		The application is notified of any action in the dialog via a
		message whose id is returned in the uMessage structure element.
		The application should therefore check for this message in its
		window procedure.

		The wParam parameter of the message contains the notification flags.

		The TOOLSFIND structure that is used to initialise the dialog should
		not be destroyed until the dialog is dismissed.	

		When the application receives the FIND_DIALOGTERM notification, it
		should tidy up any memory associated with the dialog and end its
		check for modeless dialogs in its message loop.
*/
{
	HWND hwndHidden;
	WNDCLASS wc;
	char szClass[100];
	HLOCAL hfr;
	FINDREPLACE FAR * lpfr;

	if (!IsWindow(hwndOwner) || !lpToolsFind)
	{
		ASSERT(NULL);
		return FALSE;
	}

	if (!lpToolsFind->lpszFindWhat || lpToolsFind->wFindWhatLen == 0)
	{
		ASSERT(NULL);
		return FALSE;
	}

	hfr = LocalAlloc(LHND, sizeof(FINDREPLACE));
	lpfr = hfr ? (FINDREPLACE FAR *)LocalLock(hfr) : NULL;
	if (!lpfr)
	{
		if (hfr)
			LocalFree(hfr);
		return FALSE;
	}

	// Create a hidden window to deal with all the Windows message processing
	// required by this common dialog.  We can then send our own notification
	// messages to the application and hide Windows specific tasks.

	if (LoadString(hResource, IDS_FINDTEXT_WNDCLASS, szClass, sizeof(szClass)))
	{
		// Create the class for the hidden window if it does not already
		// exist.

		if (!GetClassInfo(hResource, szClass, &wc))
		{
			ZEROINIT(wc);

			wc.lpfnWndProc = DlgFindWndProc;
			wc.hInstance = hInst;
			wc.lpszClassName = szClass;

			if (!RegisterClass(&wc))
				return FALSE;
		}
	}
	else
		return FALSE;

	// Create the hidden window

	hwndHidden = CreateWindow (szClass,
										"",
										WS_POPUP,
										0, 0, 0, 0,
										hwndOwner,
										NULL,
										hInst,
										NULL);

	if (hwndHidden == NULL)
		return FALSE;

	SetProp(hwndHidden, szFRStruct, hfr);

	_fmemset(lpfr, 0, sizeof(FINDREPLACE));

	lpfr->lStructSize = sizeof(FINDREPLACE);
	lpfr->hwndOwner = hwndHidden;
	lpfr->lpstrFindWhat = lpToolsFind->lpszFindWhat;
	lpfr->wFindWhatLen = lpToolsFind->wFindWhatLen;
	lpfr->Flags = FR_HIDEUPDOWN;

	if ((lpToolsFind->hdlg = FindText(lpfr)) != NULL)
	{
		char szMessage[100];
		HLOCAL hmem = LocalAlloc(LHND, sizeof(TOOLSFINDPROP));
		LPTOOLSFINDPROP lpprop = hmem ? (LPTOOLSFINDPROP) LocalLock(hmem) : NULL;

		if (lpprop)
		{
			LoadString(hResource, IDS_FINDTEXT_MESSAGE, szMessage, sizeof(szMessage));
			lpprop->uWinFindMessage = RegisterWindowMessage(FINDMSGSTRING);
			lpprop->uFindMessage = lpToolsFind->uMessage = RegisterWindowMessage(szMessage);
			LocalUnlock(hmem);
			SetProp(hwndHidden, szToolsFindProp, hmem);
		}
		else
		{
			if (hmem)
				LocalFree(hmem);
			DestroyWindow(lpToolsFind->hdlg);
			DestroyWindow(hwndHidden);
			lpToolsFind->hdlg = NULL;
		}
	}

	return (lpToolsFind->hdlg == NULL ? FALSE : TRUE);
}


LRESULT CALLBACK __export DlgFindWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HLOCAL hmem = (HLOCAL)GetProp(hwnd, szToolsFindProp);
	LPTOOLSFINDPROP lpprop = hmem ? (LPTOOLSFINDPROP)LocalLock(hmem): NULL;

	if (lpprop && (message == lpprop->uWinFindMessage))
	{
		HWND hwndApp = GetParent(hwnd);
		FINDREPLACE FAR * lpfr = (FINDREPLACE FAR *)lParam;
		WPARAM wFlags = 0;

		if (lpfr->Flags & FR_DIALOGTERM)
			wFlags |= FIND_DIALOGTERM;

		if (lpfr->Flags & FR_FINDNEXT)
			wFlags |= FIND_FINDNEXT;

		if (lpfr->Flags & FR_WHOLEWORD)
			wFlags |= FIND_WHOLEWORD;

		if (lpfr->Flags & FR_MATCHCASE)
			wFlags |= FIND_MATCHCASE;

		// Notify the application
		SendMessage(hwndApp, lpprop->uFindMessage, wFlags, 0L);

		LocalUnlock(hmem);

		if (lpfr->Flags & FR_DIALOGTERM)
			PostMessage(hwnd, LM_DESTROY, 0, 0L);

		return 0;
	}

	if (lpprop)
		LocalUnlock(hmem);

	switch (message)
	{
		HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);

		case LM_DESTROY:
			DestroyWindow(hwnd);
			break;

		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
	}

	return 0L;
}


static void OnDestroy(HWND hwnd)
{
	HLOCAL hmem;

	hmem = GetProp(hwnd, szToolsFindProp);
	LocalFree(hmem);
	RemoveProp(hwnd, szToolsFindProp);

	hmem = GetProp(hwnd, szFRStruct);
	LocalUnlock(hmem);
	LocalFree(hmem);
	RemoveProp(hwnd, szFRStruct);
}
