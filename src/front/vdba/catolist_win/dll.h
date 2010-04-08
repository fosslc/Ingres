#ifndef _DLL_H
#define _DLL_H

#define STRICT
#include <windows.h>
#include <windowsx.h>

#include "assert.h"

#ifdef NDEBUG
	#define ASSERT(exp) ((void)0)
#else
	#define ASSERT(exp) assert(exp)
#endif

typedef struct tagCATOLIST
{
	HWND        hWndAssociate;  //Associate window handle
	LONG       	style;        //Copy of GetWindowLong(hWnd, GWL_STYLE)
	HWND			hwndList;
	WNDPROC		wndprcListBox;
	HPEN			hpenBackground;
	COLORREF		hpenbrColour;	// Colour of the current background pen
} CATOLIST, FAR * LPCATOLIST;

typedef struct tagITEMDATA
{
	BOOL bSelected;
	LPARAM lParam;	// User data.
} ITEMDATA, FAR * LPITEMDATA;

// Listbox window ID
#define ID_LISTBOX	1

//Offsets to use with GetWindowLong
#define GWL_LPLIST    0

//Extra bytes for the window if the size of a local handle.
#define CBWINDOWEXTRA       sizeof(LPCATOLIST)

//Extra Class bytes.
#define CBCLASSEXTRA        0

extern HINSTANCE	hinst;

#ifdef WIN32
#define __export

#define	ForwardCALBNotification(hwndAssociate, hwnd, uNotify)\
				SendMessage((HWND)hwndAssociate, WM_COMMAND, MAKEWPARAM((UINT)GetWindowID(hwnd), (UINT)uNotify), (LPARAM)(HWND)hwnd);
#else
#define	ForwardCALBNotification(hwndAssociate, hwnd, uNotify)\
				SendMessage((HWND)hwndAssociate, WM_COMMAND, (WPARAM)GetWindowID(hwnd), MAKELPARAM((HWND)hwnd, (UINT)uNotify));
#endif

LRESULT CALLBACK __export CATOLISTWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

#endif		// _DLL_H
