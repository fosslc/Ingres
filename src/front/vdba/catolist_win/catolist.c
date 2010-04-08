/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Name: catolist.cpp
**
**  Description:
**
**  History:
**	03-may-1996 (fanra01)
**	    Created.
**	14-Feb-2000 (somsa01)
**	    Corrected typecast in SelectItem().
**	02-feb-2004 (somsa01)
**	    Removed inclusion of ctl3d.h.
**	07-May-2009 (drivi01)
**	    In effort to port to Visual Studio 2008
**	    clean up the precedence warnings.
*/

#include "dll.h"

#define _DEV
#include "catolist.h"

#ifndef WIN32
#define _USE_CTL3D
#endif

#define TEXT_LEFT_MARGIN	3
#define TEXT_VERT_MARGIN	2

#define SELECTION_SPACER	1
#define BOX_LEFT_MARGIN		1
#define BOX_VERT_MARGIN		1

static BOOL OnNCCreate(HWND hwnd, LPCREATESTRUCT lpCreate);
static BOOL OnCreate (HWND hwnd, LPCREATESTRUCT lpCreate);
static void OnNCDestroy(HWND hwnd);
static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT FAR* lpDrawItem);
static void OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT FAR* lpMeasureItem);
static void OnDeleteItem(HWND hwnd, const DELETEITEMSTRUCT FAR* lpDeleteItem);
static void OnSetFocus(HWND hwnd, HWND hwndOldFocus);
static void OnKillFocus(HWND hwnd, HWND hwndNewFocus);
static void OnSize(HWND hwnd, UINT state, int cx, int cy);

static void LBOnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
static void LBOnChar(HWND hwnd, UINT ch, int cRepeat);

static LONG TranslateToLBStyles(LONG style);
static void HandleFocusState(const DRAWITEMSTRUCT FAR * lpDrawItem);
static void HandleSelectionState(const DRAWITEMSTRUCT FAR * lpDrawItem);
static int GetCheckBoxWidth (const DRAWITEMSTRUCT FAR* lpDrawItem);
static int GetTextLeftMargin(const DRAWITEMSTRUCT FAR * lpDrawItem);
static void DrawCheckBox(const DRAWITEMSTRUCT FAR* lpDrawItem);
static LRESULT HandleCATOLISTMessages(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK __export SubListBoxWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static void ClearFocusRect(HWND hwnd, const DRAWITEMSTRUCT FAR * lpDrawItem);
static BOOL SelectItem(HWND hwnd, int nIdx);

LRESULT WINAPI __export LBDefWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

/*
 * CATOLISTWndProc
 *
 * Purpose:
 *  Window Procedure for the CATOLIST custom control.  Handles all
 *  messages like WM_PAINT just as a normal application window would.
 *  Any message not processed here should go to DefWindowProc.
 *
 * Parameters:
 *  hwnd            HWND handle to the control window.
 *  message         UINT message identifier.
 *  wParam          WPARAM parameter of the message.
 *  lParam          LPARAM parameter of the message.
 *
 * Return Value:
 *  LRESULT         Varies with the message.
 *
 */

LRESULT CALLBACK __export CATOLISTWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_NCCREATE:
			return HANDLE_WM_NCCREATE(hwnd, wParam, lParam, OnNCCreate);

		case WM_CREATE:
			return HANDLE_WM_CREATE(hwnd, wParam, lParam, OnCreate);

		case WM_DRAWITEM:
			return HANDLE_WM_DRAWITEM(hwnd, wParam, lParam, OnDrawItem);

		HANDLE_MSG(hwnd, WM_NCDESTROY, OnNCDestroy);
		HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
		HANDLE_MSG(hwnd, WM_MEASUREITEM, OnMeasureItem);
		HANDLE_MSG(hwnd, WM_DELETEITEM, OnDeleteItem);
		HANDLE_MSG(hwnd, WM_SETFOCUS, OnSetFocus);
		HANDLE_MSG(hwnd, WM_KILLFOCUS, OnKillFocus);
		HANDLE_MSG(hwnd, WM_SIZE, OnSize);

		default:
		{
			if (message > WM_USER)
				return HandleCATOLISTMessages(hwnd, message, wParam, lParam);

			return DefWindowProc(hwnd, message, wParam, lParam);
		}
	}

	return 0L;
}


static BOOL OnCreate (HWND hwnd, LPCREATESTRUCT lpCreate)
{
	LPCATOLIST lpList;
	HWND hwndListBox;
	LONG style;

	lpList = (LPCATOLIST)GetWindowLong(hwnd, GWL_LPLIST);

	// Our associate is the parent by default.
	lpList->hWndAssociate = GetParent(hwnd);

	// Copy styles
	lpList->style = lpCreate->style;

	ShowWindow(hwnd, SW_SHOWNORMAL);

	style = TranslateToLBStyles (lpList->style);

	// Maintain the WS_TABSTOP style if set
	if ((lpList->style & WS_TABSTOP) != 0)
		style |= WS_TABSTOP;
	// Maintain the WS_GROUP style if set
	if ((lpList->style & WS_GROUP) != 0)
		style |= WS_GROUP;

	hwndListBox = CreateWindow (	"LISTBOX",
											"",
											style | LBS_OWNERDRAWFIXED |
											LBS_NOINTEGRALHEIGHT | LBS_HASSTRINGS |
											WS_VISIBLE | WS_VSCROLL | WS_BORDER |
											WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
											2,
											2,
											lpCreate->cx - 4,
											lpCreate->cy - 4,
											hwnd,
											(HMENU)ID_LISTBOX,
											hinst,
											NULL);

	if (!IsWindow(hwndListBox))
		return FALSE;

#ifdef _USE_CTL3D
	Ctl3dSubclassCtl(hwndListBox);
#endif

	lpList->wndprcListBox = SubclassWindow(hwndListBox, SubListBoxWndProc);

	lpList->hwndList = hwndListBox;
	lpList->hpenbrColour = GetSysColor(COLOR_WINDOW);
	lpList->hpenBackground = CreatePen(PS_SOLID, 1, lpList->hpenbrColour);

	{
		// Set the listbox font the same as the dialogs font.

		HFONT hfontDlg = GetWindowFont(GetParent(hwnd));
		SetWindowFont(hwndListBox, hfontDlg, TRUE);
	}

	return TRUE;
}


static BOOL OnNCCreate(HWND hwnd, LPCREATESTRUCT lpCreate)
{
	LPCATOLIST lpList;

	if ((lpList = (LPCATOLIST)GlobalAllocPtr(GHND, sizeof(CATOLIST))) == NULL)
		return FALSE;

	SetWindowLong(hwnd, GWL_LPLIST, (LONG)lpList);

	return TRUE;
}


static void OnNCDestroy(HWND hwnd)
{
	LPCATOLIST lpList;
	lpList = (LPCATOLIST)GetWindowLong(hwnd, GWL_LPLIST);

#if	0
	// Since the list box is a child window it has already
	// been destroyed by this point.
	SubclassWindow(lpList->hwndList, lpList->wndprcListBox);

#ifdef _USE_CTL3D
	Ctl3dUnsubclassCtl(lpList->hwndList);
#endif
#endif

#ifndef WIN32
	if (IsGDIObject(lpList->hpenBackground))
#endif
		DeleteObject(lpList->hpenBackground);

	GlobalFreePtr (lpList);
}


static void OnSetFocus(HWND hwnd, HWND hwndOldFocus)
{
	LPCATOLIST lpList;
	lpList = (LPCATOLIST)GetWindowLong(hwnd, GWL_LPLIST);

	SetFocus(lpList->hwndList);
}


static void OnKillFocus(HWND hwnd, HWND hwndNewFocus)
{
	LPCATOLIST lpList;
	lpList = (LPCATOLIST)GetWindowLong(hwnd, GWL_LPLIST);

	SendMessage(lpList->hwndList, WM_KILLFOCUS, (WPARAM)hwndNewFocus, 0L);
}


static void OnSize(HWND hwnd, UINT state, int cx, int cy)
{
	LPCATOLIST lpList;
	lpList = (LPCATOLIST)GetWindowLong(hwnd, GWL_LPLIST);

	SendMessage(lpList->hwndList, WM_SIZE, (WPARAM)state, MAKELPARAM(cx, cy));
}


static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	LPCATOLIST lpList = (LPCATOLIST)GetWindowLong(hwnd, GWL_LPLIST);
	UINT uNotify;

	switch (id)
	{
		case ID_LISTBOX:
		{
			switch (codeNotify)
			{
				case (UINT)LBN_ERRSPACE:
					uNotify = (UINT)CALBN_ERRSPACE;
					break;

				case LBN_SELCHANGE:
					uNotify = CALBN_SELCHANGE;
					break;

				case LBN_DBLCLK:
					uNotify = CALBN_DBLCLK;
					break;

				case LBN_SELCANCEL:
					uNotify = CALBN_SELCANCEL;
					break;

				case LBN_SETFOCUS:
					uNotify = CALBN_SETFOCUS;
					break;

				case LBN_KILLFOCUS:
					uNotify = CALBN_KILLFOCUS;
					break;

				default:
					return;
			}

			ForwardCALBNotification(lpList->hWndAssociate, hwnd, uNotify);
			break;
		}
	}
}


static BOOL OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT FAR* lpDrawItem)
{
	char szText[256];

	// Check for items in the list box.
	if (lpDrawItem->itemID == -1)
	{
		HandleFocusState(lpDrawItem);
		return TRUE;
	}

	switch (lpDrawItem->itemAction)
	{
		case ODA_DRAWENTIRE:
		{
			BOOL bItemSelected = lpDrawItem->itemState & ODS_SELECTED ? TRUE : FALSE;
			BOOL bItemHasFocus = lpDrawItem->itemState & ODS_FOCUS ? TRUE : FALSE;

			ClearFocusRect(hwnd, lpDrawItem);

			DrawCheckBox(lpDrawItem);
	
			if (ListBox_GetText(lpDrawItem->hwndItem, lpDrawItem->itemID, szText) != LB_ERR)
			{
				int nTextMargin = GetTextLeftMargin(lpDrawItem);

				TextOut(	lpDrawItem->hDC,
							lpDrawItem->rcItem.left + nTextMargin,
							lpDrawItem->rcItem.top + 1,
							szText,
							lstrlen(szText));
			}

			if (bItemSelected)
				HandleSelectionState(lpDrawItem);

			if (bItemHasFocus)
				HandleFocusState(lpDrawItem);

			break;
		}

		case ODA_SELECT:
		{
			HandleSelectionState(lpDrawItem);
			break;
		}

		case ODA_FOCUS:
		{
			HandleFocusState(lpDrawItem);
			break;
		}

		default:
			return FALSE;
	}

	return TRUE;
}


static void OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT FAR* lpMeasureItem)
{
	HWND hdlg = GetParent(GetParent(hwnd));
	HDC hdc = GetDC(hdlg);
	TEXTMETRIC tm;

	GetTextMetrics(hdc, &tm);
	lpMeasureItem->itemHeight = tm.tmHeight - 1;
	ReleaseDC(hdlg, hdc);
}


static void OnDeleteItem(HWND hwnd, const DELETEITEMSTRUCT FAR* lpDeleteItem)
{
	LPCATOLIST lpList = (LPCATOLIST)GetWindowLong(hwnd, GWL_LPLIST);
	HLOCAL hmem = (HLOCAL)ListBox_GetItemData(lpList->hwndList, lpDeleteItem->itemID);
	LocalFree(hmem);
}


static void HandleSelectionState(const DRAWITEMSTRUCT FAR * lpDrawItem)
{
	RECT rect;

	rect = lpDrawItem->rcItem;
	rect.left = GetTextLeftMargin(lpDrawItem) - SELECTION_SPACER;

	InvertRect(lpDrawItem->hDC, &rect);
}


static void HandleFocusState(const DRAWITEMSTRUCT FAR * lpDrawItem)
{
	RECT rect;

	rect = lpDrawItem->rcItem;
	rect.left = GetTextLeftMargin(lpDrawItem) - SELECTION_SPACER;

	DrawFocusRect(lpDrawItem->hDC, &rect);
}


static void ClearFocusRect(HWND hwnd, const DRAWITEMSTRUCT FAR * lpDrawItem)
/*
	Function:
		Clears the focus rectangle by drawing a rectangle of the background
		colour on top of it.

	Parameters:
		hwnd			- Handle of the control window
		lpDrawItem	- Item drawing info structure.

	Returns:
		None.

	Note:
		The reason this function was implemented was to cope with unwanted
		focus rectangles being drawn. The bug was as follows:
			1. Bring up the Create Database dialog in CA-OpenINGRES Tools Tester
			2. Set the focus to the first item in the products list.
			3. Cover the dialog with Program Manager.
			4. Bring the dialog back to the front.
		After following these steps the focus rectangle would not be visible
		since we had received too many HandleFocusState messages.
*/
{
	RECT rect;
	LPCATOLIST lpList = (LPCATOLIST)GetWindowLong(hwnd, GWL_LPLIST);
	HPEN hpenOld;

	// Since we are not a top level window we do not receive the
	// WM_SYSCOLORCHANGE message, therefore we must check for ourself
	// to see if the current pen is still using the correct background
	// colour and the user hasnt changed it on us.

	if (lpList->hpenbrColour != GetSysColor(COLOR_WINDOW))
	{
		DeleteObject(lpList->hpenBackground);
		lpList->hpenbrColour = GetSysColor(COLOR_WINDOW);
		lpList->hpenBackground = CreatePen(PS_SOLID, 1, lpList->hpenbrColour);
	}

	hpenOld = SelectObject(lpDrawItem->hDC, lpList->hpenBackground);

	rect = lpDrawItem->rcItem;
	rect.left = GetTextLeftMargin(lpDrawItem) - SELECTION_SPACER;

	// Top line
	MoveToEx(lpDrawItem->hDC, rect.left, rect.top, NULL);
	LineTo(lpDrawItem->hDC, rect.right - 1, rect.top);

	// Right side line
	MoveToEx(lpDrawItem->hDC, rect.right - 1, rect.top, NULL);
	LineTo(lpDrawItem->hDC, rect.right - 1, rect.bottom - 1);

	// Bottom line
	MoveToEx(lpDrawItem->hDC, rect.left, rect.bottom - 1, NULL);
	LineTo(lpDrawItem->hDC, rect.right - 1, rect.bottom - 1);

	// Left side line
	MoveToEx(lpDrawItem->hDC, rect.left, rect.top, NULL);
	LineTo(lpDrawItem->hDC, rect.left, rect.bottom - 1);

	SelectObject(lpDrawItem->hDC, hpenOld);
}


static LONG TranslateToLBStyles(LONG style)
{
	LONG lRet = 0;

	if (style & CALBS_NOTIFY)
		lRet |= LBS_NOTIFY;

	if (style & CALBS_SORT)
		lRet |= LBS_SORT;

	if (style & CALBS_NOREDRAW)
		lRet |= LBS_NOREDRAW;

	if (style & CALBS_USETABSTOPS)
		lRet |= LBS_USETABSTOPS;

	if (style & CALBS_WANTKEYBOARDINPUT)
		lRet |= LBS_WANTKEYBOARDINPUT;

#if (WINVER >= 0x030a)
	if (style & CALBS_DISABLENOSCROLL)
		lRet |= LBS_DISABLENOSCROLL;
#endif  /* WINVER >= 0x030a */

	return lRet;
}


static int GetCheckBoxWidth (const DRAWITEMSTRUCT FAR* lpDrawItem)
{
	int nWidth;

	nWidth = lpDrawItem->rcItem.bottom - lpDrawItem->rcItem.top;
	nWidth -= 2 * BOX_VERT_MARGIN;

	return (nWidth > 0 ? nWidth : 2);
}


static int GetTextLeftMargin(const DRAWITEMSTRUCT FAR * lpDrawItem)
{
	return (BOX_LEFT_MARGIN + GetCheckBoxWidth(lpDrawItem) + TEXT_LEFT_MARGIN + SELECTION_SPACER);
}


static void DrawCheckBox(const DRAWITEMSTRUCT FAR* lpDrawItem)
{
	RECT rect;
	int nWidth = GetCheckBoxWidth(lpDrawItem);
	HLOCAL hmem = (HLOCAL)lpDrawItem->itemData;
	LPITEMDATA lpdata = hmem ? (LPITEMDATA)LocalLock(hmem) : NULL;

	if (!lpdata)
	{
		ASSERT(NULL);
		return;
	}

	rect.left   = lpDrawItem->rcItem.left + BOX_LEFT_MARGIN;
	rect.top    = lpDrawItem->rcItem.top + BOX_VERT_MARGIN;
	rect.right  = lpDrawItem->rcItem.left + BOX_LEFT_MARGIN + nWidth;
	rect.bottom = lpDrawItem->rcItem.top + BOX_VERT_MARGIN + nWidth;

	Rectangle(	lpDrawItem->hDC,
					rect.left,
					rect.top,
					rect.right + 1,
					rect.bottom);

	if (lpdata->bSelected)
	{
		MoveToEx(lpDrawItem->hDC, rect.left, rect.top, NULL);
		LineTo(lpDrawItem->hDC, rect.right, rect.bottom);

		MoveToEx(lpDrawItem->hDC, rect.right, rect.top, NULL);
		LineTo(lpDrawItem->hDC, rect.left, rect.bottom);
	}

	LocalUnlock(hmem);
}


static LRESULT HandleCATOLISTMessages(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LPITEMDATA lpdata;
	LPCATOLIST lpList;
	lpList = (LPCATOLIST)GetWindowLong(hwnd, GWL_LPLIST);

	switch (message)
	{
		case CALB_ADDSTRING:
		{
			LPSTR lpszString = (LPSTR) lParam;
			HLOCAL hmem = LocalAlloc(LHND, sizeof(ITEMDATA));
			int nIdx;

			if (hmem == NULL)
			{
				ForwardCALBNotification(lpList->hWndAssociate, hwnd, CALBN_ERRSPACE);
				return CALB_ERRSPACE;
			}

			nIdx = ListBox_AddString(lpList->hwndList, lpszString);
			if (nIdx >= 0)
				ListBox_SetItemData(lpList->hwndList, nIdx, (UINT)(HLOCAL)hmem);
			else
				LocalFree(hmem);

			return nIdx;
		}

		case CALB_INSERTSTRING:
		{
			int nItemIndex = (int)wParam;
			LPSTR lpszString = (LPSTR) lParam;
			HLOCAL hmem = LocalAlloc(LHND, sizeof(ITEMDATA));
			int nIdx;

			if (hmem == NULL)
			{
				ForwardCALBNotification(lpList->hWndAssociate, hwnd, CALBN_ERRSPACE);
				return CALB_ERRSPACE;
			}

			nIdx = ListBox_InsertString(lpList->hwndList, nItemIndex, lpszString);
			if (nIdx >= 0)
				ListBox_SetItemData(lpList->hwndList, nIdx, (UINT)(HLOCAL)hmem);
			else
				LocalFree(hmem);

			return nIdx;
		}

		case CALB_DELETESTRING:
			return ListBox_DeleteString(lpList->hwndList, wParam);

		case CALB_RESETCONTENT:
			return ListBox_ResetContent(lpList->hwndList);

		case CALB_SETSEL:
		{
			BOOL bSelected = (BOOL)wParam;
			UINT uIdx, uIdxStart, uIdxEnd;
			HLOCAL hmem;
            RECT rect;

            if (lParam == -1)
            {
                uIdxStart = 0;
                uIdxEnd = ListBox_GetCount(lpList->hwndList) - 1;
            }
            else
            {
                uIdxStart = (UINT) lParam;
                uIdxEnd = (UINT) lParam;
            }

			for (uIdx = uIdxStart; uIdx <= uIdxEnd; uIdx++)
            {
			    hmem = (HLOCAL)ListBox_GetItemData(lpList->hwndList, uIdx);
			    if (hmem == (HLOCAL)LB_ERR)
				    return CALB_ERR;
			    lpdata = (LPITEMDATA)LocalLock(hmem);
			    lpdata->bSelected = (bSelected != 0 ? TRUE : FALSE);
			    LocalUnlock(hmem);
      	        ListBox_GetItemRect(lpList->hwndList, (int)uIdx, &rect);
      	        rect.top += BOX_VERT_MARGIN;
      	        rect.bottom -= BOX_VERT_MARGIN - 1;
                rect.left = BOX_LEFT_MARGIN;
      	        rect.right = rect.left + (rect.bottom - rect.top);
      	        InvalidateRect(lpList->hwndList, &rect, TRUE);
            }
			return CALB_OKAY;
		}

		case CALB_GETSEL:
		{
			BOOL bSelected;
			int nItemIndex = (int)wParam;
			HLOCAL hmem = (HLOCAL)ListBox_GetItemData(lpList->hwndList, nItemIndex);

			if (hmem == (HLOCAL)LB_ERR)
				return CALB_ERR;

			lpdata = (LPITEMDATA)LocalLock(hmem);
			bSelected = lpdata->bSelected;
			LocalUnlock(hmem);
			return bSelected;
		}

		case CALB_SETCURSEL:
			return ListBox_SetCurSel(lpList->hwndList, wParam);

		case CALB_GETCURSEL:
			return ListBox_GetCurSel(lpList->hwndList);

		case CALB_GETTEXT:
			return ListBox_GetText(lpList->hwndList, wParam, lParam);

		case CALB_GETTEXTLEN:
			return ListBox_GetTextLen(lpList->hwndList, wParam);

		case CALB_GETCOUNT:
			return ListBox_GetCount(lpList->hwndList);

		case CALB_SELECTSTRING:
		{
			int nItemIndex = ListBox_SelectString(lpList->hwndList, wParam, lParam);
			if (nItemIndex < 0)
				return nItemIndex;
			return CAListBox_SetSel(hwnd, TRUE, nItemIndex);
		}

#if	0
		// !! No need to invest time in this one right now.
		// !! Maybe future release.
		case CALB_DIR:
			return ListBox_Dir(lpList->hwndList, wParam, lParam);
#endif

		case CALB_GETTOPINDEX:
			return ListBox_GetTopIndex(lpList->hwndList);

		case CALB_FINDSTRING:
			return ListBox_FindString(lpList->hwndList, wParam, lParam);

		case CALB_GETSELCOUNT:
		{
			int nItemCount = ListBox_GetCount(lpList->hwndList);
			int nItemIndex;
			int nSelectedCount = 0;

			// Check for an error
			if (nItemCount < 0)
				return nItemCount;

			for (nItemIndex = 0; nItemIndex < nItemCount; nItemIndex++)
			{
				HLOCAL hmem = (HLOCAL)ListBox_GetItemData(lpList->hwndList, nItemIndex);

				if (hmem == (HLOCAL)LB_ERR)
					return CALB_ERR;

				lpdata = (LPITEMDATA)LocalLock(hmem);
				if (lpdata->bSelected)
					nSelectedCount++;
				LocalUnlock(hmem);
			}
			return nSelectedCount;
		}

		case CALB_GETSELITEMS:
		{
			int nMax = (int)wParam;
			int FAR * lpbuf = (int FAR *)lParam;
			int nItemIndex;
			int nItemCount = ListBox_GetCount(lpList->hwndList);
			int nSelectedCount = 0;

			// Check for an error
			if (nItemCount < 0)
				return nItemCount;

			for (nItemIndex = 0; nItemIndex < nItemCount; nItemIndex++)
			{
				HLOCAL hmem = (HLOCAL)ListBox_GetItemData(lpList->hwndList, nItemIndex);

				if (hmem == (HLOCAL)LB_ERR)
					return CALB_ERR;

				if (nSelectedCount >= nMax)
					break;

				lpdata = (LPITEMDATA)LocalLock(hmem);
				if (lpdata->bSelected)
				{
					*lpbuf = nItemIndex;
					lpbuf++;
					nSelectedCount++;
				}
				LocalUnlock(hmem);
			}
			return CALB_OKAY;
		}

		case CALB_SETTABSTOPS:
			return ListBox_SetTabStops(lpList->hwndList, wParam, lParam);

		case CALB_GETHORIZONTALEXTENT:
			return ListBox_GetHorizontalExtent(lpList->hwndList);

		case CALB_SETHORIZONTALEXTENT:
			ListBox_SetHorizontalExtent(lpList->hwndList, wParam);
			break;

		case CALB_SETCOLUMNWIDTH:
			ListBox_SetColumnWidth(lpList->hwndList, wParam);
			break;

		case CALB_SETTOPINDEX:
			return ListBox_SetTopIndex(lpList->hwndList, wParam);

		case CALB_GETITEMRECT:
			return ListBox_GetItemRect(lpList->hwndList, wParam, lParam);

		case CALB_GETITEMDATA:
		{
			int nItemIndex = (int)wParam;
			HLOCAL hmem = (HLOCAL)ListBox_GetItemData(lpList->hwndList, nItemIndex);
			LPARAM lUser;

			if (hmem == (HLOCAL)LB_ERR)
				return CALB_ERR;

			lpdata = (LPITEMDATA)LocalLock(hmem);
			lUser = lpdata->lParam;
			LocalUnlock(hmem);

			return lUser;
		}

		case CALB_SETITEMDATA:
		{
			int nItemIndex = (int)wParam;
			HLOCAL hmem = (HLOCAL)ListBox_GetItemData(lpList->hwndList, nItemIndex);

			if (hmem == (HLOCAL)LB_ERR)
				return CALB_ERR;

			lpdata = (LPITEMDATA)LocalLock(hmem);
			lpdata->lParam = lParam;
			LocalUnlock(hmem);

			return CALB_OKAY;
		}

		case CALB_SELITEMRANGE:
		{
			BOOL bSelection = (BOOL) wParam;
			int nFirst = (int)LOWORD(lParam);
			int nLast = (int)HIWORD(lParam);
			int nItemIndex;

			ASSERT(nFirst <= nLast);

			for (nItemIndex = nFirst; nItemIndex <= nLast; nItemIndex++)
			{
				int nError = CAListBox_SetSel(hwnd, bSelection, nItemIndex);
				if (nError == CALB_ERR)
					return CALB_ERR;
			}

			return CALB_OKAY;
		}

#if (WINVER >= 0x030a)
		case CALB_SETITEMHEIGHT:
			return ListBox_SetItemHeight(lpList->hwndList, wParam, lParam);

		case CALB_GETITEMHEIGHT:
			return ListBox_GetItemHeight(lpList->hwndList, wParam);

		case CALB_FINDSTRINGEXACT:
			return ListBox_FindStringExact(lpList->hwndList, wParam, lParam);
#endif  /* WINVER >= 0x030a */

		default:
			ASSERT(NULL);
	}

	return 0L;
}


LRESULT CALLBACK __export SubListBoxWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
      HANDLE_MSG(hwnd, WM_LBUTTONDOWN, LBOnLButtonDown);
      HANDLE_MSG(hwnd, WM_LBUTTONDBLCLK, LBOnLButtonDown);
      HANDLE_MSG(hwnd, WM_CHAR, LBOnChar);

      default:
         return LBDefWndProc(hwnd, message, wParam, lParam);
	}
}


static void LBOnChar(HWND hwnd, UINT ch, int cRepeat)
{
	switch (ch)
	{
		case VK_SPACE:
		{
			// Allow the space key to toggle the item check box

         int nCount = ListBox_GetCount(hwnd);
         if (nCount != LB_ERR)
         {
   			int nIdx = ListBox_GetCurSel(hwnd);
            HWND hwndCtl = GetParent(hwnd);
         	LPCATOLIST lpList;

	   		SelectItem(hwnd, nIdx);

         	lpList = (LPCATOLIST)GetWindowLong(hwndCtl, GWL_LPLIST);
            ASSERT(lpList);
            ForwardCALBNotification(lpList->hWndAssociate, hwndCtl, CALBN_CHECKCHANGE);
         }

         break;
		}

		default:
		{
			// Search for the next item that begins with this character.

			int nIdx = ListBox_GetCurSel(hwnd);
			char szFormat[] = "%c";
			char szSearch[sizeof(szFormat) + 1];

			wsprintf(szSearch, szFormat, ch);
			ListBox_SelectString(hwnd, nIdx, szSearch);
		}
	}
}


static void LBOnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	int nHeight;
	int nIdxTop = ListBox_GetTopIndex(hwnd);
	int nCount = ListBox_GetCount(hwnd);
	int nOffset;
   BOOL bNotify = FALSE;

	// Check we have some items

	if (nIdxTop >= 0)
   {
   	// Determine the index of the item we are on

   	nHeight = ListBox_GetItemHeight(hwnd, 0);
	   nOffset = y / nHeight;

   	// Check we are within a valid item range.  If so select the item.

   	if (nIdxTop + nOffset < nCount)
      {
        // Did we click in the checkbox area???
        BOOL bInCheckbox = FALSE;
        RECT rect;
        POINT pt;

        ListBox_GetItemRect(hwnd, nIdxTop + nOffset, &rect);
        rect.top += BOX_VERT_MARGIN;
        rect.bottom -= BOX_VERT_MARGIN - 1;
        rect.left = BOX_LEFT_MARGIN;
        rect.right = rect.left + (rect.bottom - rect.top);

        pt.x = x;
        pt.y = y;

        if (PtInRect(&rect, pt))
          bInCheckbox = TRUE;

        if (bInCheckbox)
        {
          SelectItem(hwnd, nIdxTop + nOffset);
          bNotify = TRUE;
        }
      }
   }

   FORWARD_WM_LBUTTONDOWN(hwnd, fDoubleClick, x, y, keyFlags, LBDefWndProc);

   if (bNotify)
   {
      HWND hwndCtl = GetParent(hwnd);
   	LPCATOLIST lpList = (LPCATOLIST)GetWindowLong(hwndCtl, GWL_LPLIST);
      ASSERT(lpList);
   	ForwardCALBNotification(lpList->hWndAssociate, hwndCtl, CALBN_CHECKCHANGE);
   }
}


static BOOL SelectItem(HWND hwnd, int nIdx)
{
	HLOCAL hmem = (HLOCAL)ListBox_GetItemData(hwnd, nIdx);
	RECT rect;
	LPITEMDATA lpdata;

	// Check for error
	if (hmem < (HLOCAL)0)
		return FALSE;

	// Toggle the item selection

	lpdata = (LPITEMDATA)LocalLock(hmem);
	if (lpdata)
		lpdata->bSelected = !lpdata->bSelected;
	LocalUnlock(hmem);

	// Invalidate the check box so it gets repainted

	ListBox_GetItemRect(hwnd, nIdx, &rect);

	rect.top += BOX_VERT_MARGIN;
	rect.bottom -= BOX_VERT_MARGIN - 1;
	rect.left = BOX_LEFT_MARGIN;
	rect.right = rect.left + (rect.bottom - rect.top);
	InvalidateRect(hwnd, &rect, TRUE);

	return TRUE;
}

#ifdef WIN32
LIBAPI BOOL CATOListDummy(void)
#else
BOOL WINAPI LIBAPI CATOListDummy (void)
#endif
{
	return TRUE;
}


LRESULT WINAPI __export LBDefWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LPCATOLIST lpList = (LPCATOLIST)GetWindowLong(GetParent(hwnd), GWL_LPLIST);

	return CallWindowProc(lpList->wndprcListBox, hwnd, message, wParam, lParam);
}

