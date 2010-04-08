#ifndef _CATOLIST_H
#define _CATOLIST_H

#ifndef _INC_WINDOWS
#include <windows.h>
#endif

#ifdef WIN32
    #ifndef _DEV
        #define LIBAPI _declspec(dllimport)
    #else
        #define LIBAPI _declspec(dllexport)
    #endif
#else   // WIN32
    #define LIBAPI __export
#endif  // WIN32

#ifdef WIN32
    LIBAPI BOOL CATOListDummy(void);
#else
    BOOL WINAPI LIBAPI CATOListDummy (void);
#endif

// CATOLIST Styles

#define CALBS_NOTIFY	           0x0001L
#define CALBS_SORT	           0x0002L
#define CALBS_NOREDRAW	        0x0004L
#define CALBS_USETABSTOPS       0x0008L
#define CALBS_WANTKEYBOARDINPUT 0x0010L
#define CALBS_3D                0x0100L
#if (WINVER >= 0x030a)
#define CALBS_DISABLENOSCROLL   0x0020L
#endif  /* WINVER >= 0x030a */

// ********* CATOLIST messages **********

#define CALB_ADDSTRING            (WM_USER+1)
// wParam : not used
// lParam : (LPARAM)(LPCSTR) address of string to add
// Returns: zero based index to string. LB_ERR if error occurs, LB_ERRSPACE
//          if there is insufficient space to add the string

#define CALB_INSERTSTRING         (WM_USER+2)
// wParam : (WPARAM) item index
// lParam : (LPARAM)(LPCSTR) address of string to insert
// Returns: zero based index to string. LB_ERR if error occurs, LB_ERRSPACE
//          if there is insufficient space to insert the string

#define CALB_DELETESTRING         (WM_USER+3)
// wParam : (WPARAM) item index of string to delete
// lParam : not used
// Returns: count of items remaining in the list. LB_ERR if index is invalid

#define CALB_RESETCONTENT         (WM_USER+5)
// wParam : not used
// lParam : not used
// Returns: none

#define CALB_SETSEL               (WM_USER+6)
// wParam : (WPARAM)(BOOL) selection flag
// lParam : (LPARAM)(UINT) item index
// Returns: LB_ERR if error occurs

#define CALB_SETCURSEL	          (WM_USER+7)
// wParam : (WPARAM) item index
// lParam : not used
// Returns: LB_ERR if error occurs

#define CALB_GETSEL               (WM_USER+8)
// wParam : (WPARAM) item index
// lParam : not used
// Returns: state of check box in item. LB_ERR if error occurs

#define CALB_GETCURSEL	          (WM_USER+9)
// wParam : not used
// lParam : not used
// Returns: zero based index of selected item. LB_ERR if an error occurs

#define CALB_GETTEXT	             (WM_USER+10)
// wParam : (WPARAM) index
// lParam : (LPARAM)(LPCSTR) address of buffer
// Returns:

#define CALB_GETTEXTLEN	          (WM_USER+11)
// wParam : (WPARAM) item index
// lParam : not used
// Returns:

#define CALB_GETCOUNT	          (WM_USER+12)
// wParam : not used
// lParam : not used
// Returns:

#define CALB_SELECTSTRING         (WM_USER+13)
// wParam : (WPARAM) index start
// lParam : (LPARAM)(LPCSTR) address of search string
// Returns:

#define CALB_GETTOPINDEX	       (WM_USER+15)
// wParam : not used
// lParam : not used
// Returns:

#define CALB_FINDSTRING	          (WM_USER+16)
// wParam : (WPARAM) index start
// lParam : (LPARAM)(LPCSTR) address of search string
// Returns:

#define CALB_GETSELCOUNT	       (WM_USER+17)
// wParam : not used
// lParam : not used
// Returns:	count of items with checked check boxes. LB_ERR if error.

#define CALB_GETSELITEMS	       (WM_USER+18)
// wParam : (WPARAM) maximum number of items
// lParam : (LPARAM) (int FAR *) address of buffer
// Returns:

#define CALB_SETTABSTOPS          (WM_USER+19)
// wParam : (WPARAM) number of tab stops
// lParam : (LPARAM)(int FAR *) address of tab stop array
// Returns:

#define CALB_GETHORIZONTALEXTENT  (WM_USER+20)
// wParam : not used
// lParam : not used
// Returns:

#define CALB_SETHORIZONTALEXTENT  (WM_USER+21)
// wParam : (WPARAM) horizontal scroll width
// lParam : not used
// Returns:

#define CALB_SETCOLUMNWIDTH       (WM_USER+22)
// wParam : (WPARAM) column width
// lParam : not used
// Returns:

#define CALB_SETTOPINDEX	       (WM_USER+24)
// wParam : (WPARAM) item index
// lParam : not used
// Returns:

#define CALB_GETITEMRECT	       (WM_USER+25)
// wParam : (WPARAM) item index
// lParam : (LPARAM)(RECT FAR *) address of RECT structure
// Returns:

#define CALB_GETITEMDATA          (WM_USER+26)
// wParam : (WPARAM) item index
// lParam : not used
// Returns:

#define CALB_SETITEMDATA          (WM_USER+27)
// wParam : (WPARAM) item index
// lParam : (LPARAM) value to associate with item
// Returns:

#define CALB_SELITEMRANGE         (WM_USER+28)
// wParam : (WPARAM)(BOOL) selection flag
// lParam : MAKELPARAM(wFirst, wLast) first and last items
// Returns:

#if (WINVER >= 0x030a)

#define CALB_SETITEMHEIGHT        (WM_USER+33)
// wParam : (WPARAM) item index
// lParam : MAKELPARAM(cyItem ,0) item height
// Returns:

#define CALB_GETITEMHEIGHT        (WM_USER+34)
// wParam : (WPARAM) item index
// lParam : not used
// Returns:

#define CALB_FINDSTRINGEXACT      (WM_USER+35)
// wParam : (WPARAM) item before start of search
// lParam : (LPARAM)(LPCSTR) address of search string
// Returns:

#endif  /* WINVER >= 0x030a */

// CATOLIST notification codes

#define CALBN_ERRSPACE	    (-2)
#define CALBN_SELCHANGE	      1
#define CALBN_DBLCLK	         2
#define CALBN_SELCANCEL       3
#define CALBN_SETFOCUS        4
#define CALBN_KILLFOCUS       5
#define CALBN_CHECKCHANGE     6     // Check box selection changed

// CATOLIST message return values

#define CALB_OKAY 	    0
#define CALB_ERR		  (-1)
#define CALB_ERRSPACE  (-2)

/****** CATOLIST control message APIs *****************************************/

#define CAListBox_Enable(hwndCtl, fEnable)            EnableWindow((hwndCtl), (fEnable))

#define CAListBox_GetCount(hwndCtl)                   ((int)(DWORD)SendMessage((hwndCtl), CALB_GETCOUNT, 0, 0L))
#define CAListBox_ResetContent(hwndCtl)               ((BOOL)(DWORD)SendMessage((hwndCtl), CALB_RESETCONTENT, 0, 0L))

#define CAListBox_AddString(hwndCtl, lpsz)            ((int)(DWORD)SendMessage((hwndCtl), CALB_ADDSTRING, 0, (LPARAM)(LPCSTR)(lpsz)))
#define CAListBox_InsertString(hwndCtl, index, lpsz)  ((int)(DWORD)SendMessage((hwndCtl), CALB_INSERTSTRING, (WPARAM)(int)(index), (LPARAM)(LPCSTR)(lpsz)))
#define CAListBox_DeleteString(hwndCtl, index)        ((int)(DWORD)SendMessage((hwndCtl), CALB_DELETESTRING, (WPARAM)(int)(index), 0L))

#define CAListBox_GetTextLen(hwndCtl, index)          ((int)(DWORD)SendMessage((hwndCtl), CALB_GETTEXTLEN, (WPARAM)(int)(index), 0L))
#define CAListBox_GetText(hwndCtl, index, lpszBuffer)  ((int)(DWORD)SendMessage((hwndCtl), CALB_GETTEXT, (WPARAM)(int)(index), (LPARAM)(LPCSTR)(lpszBuffer)))

#define CAListBox_GetItemData(hwndCtl, index)         ((LRESULT)(DWORD)SendMessage((hwndCtl), CALB_GETITEMDATA, (WPARAM)(int)(index), 0L))
#define CAListBox_SetItemData(hwndCtl, index, data)   ((int)(DWORD)SendMessage((hwndCtl), CALB_SETITEMDATA, (WPARAM)(int)(index), (LPARAM)(data)))

#define CAListBox_FindString(hwndCtl, indexStart, lpszFind) ((int)(DWORD)SendMessage((hwndCtl), CALB_FINDSTRING, (WPARAM)(int)(indexStart), (LPARAM)(LPCSTR)(lpszFind)))
#define CAListBox_FindItemData(hwndCtl, indexStart, data) ((int)(DWORD)SendMessage((hwndCtl), CALB_FINDSTRING, (WPARAM)(int)(indexStart), (LPARAM)(data)))

#define CAListBox_SetSel(hwndCtl, fSelect, index)     ((int)(DWORD)SendMessage((hwndCtl), CALB_SETSEL, (WPARAM)(BOOL)(fSelect), (LPARAM)(UINT)index))
#define CAListBox_SelItemRange(hwndCtl, fSelect, first, last)    ((int)(DWORD)SendMessage((hwndCtl), CALB_SELITEMRANGE, (WPARAM)(BOOL)(fSelect), MAKELPARAM((first), (last))))

#define CAListBox_GetCurSel(hwndCtl)                  ((int)(DWORD)SendMessage((hwndCtl), CALB_GETCURSEL, 0, 0L))
#define CAListBox_SetCurSel(hwndCtl, index)           ((int)(DWORD)SendMessage((hwndCtl), CALB_SETCURSEL, (WPARAM)(int)(index), 0L))

#define CAListBox_SelectString(hwndCtl, indexStart, lpszFind) ((int)(DWORD)SendMessage((hwndCtl), CALB_SELECTSTRING, (WPARAM)(int)(indexStart), (LPARAM)(LPCSTR)(lpszFind)))

#define CAListBox_GetSel(hwndCtl, index)              ((int)(DWORD)SendMessage((hwndCtl), CALB_GETSEL, (WPARAM)(int)(index), 0L))
#define CAListBox_GetSelCount(hwndCtl)                ((int)(DWORD)SendMessage((hwndCtl), CALB_GETSELCOUNT, 0, 0L))
#define CAListBox_GetTopIndex(hwndCtl)                ((int)(DWORD)SendMessage((hwndCtl), CALB_GETTOPINDEX, 0, 0L))
#define CAListBox_GetSelItems(hwndCtl, cItems, lpItems) ((int)(DWORD)SendMessage((hwndCtl), CALB_GETSELITEMS, (WPARAM)(int)(cItems), (LPARAM)(int FAR*)(lpItems)))

#define CAListBox_SetTopIndex(hwndCtl, indexTop)      ((int)(DWORD)SendMessage((hwndCtl), CALB_SETTOPINDEX, (WPARAM)(int)(indexTop), 0L))

#define CAListBox_SetColumnWidth(hwndCtl, cxColumn)   ((void)SendMessage((hwndCtl), CALB_SETCOLUMNWIDTH, (WPARAM)(int)(cxColumn), 0L))
#define CAListBox_GetHorizontalExtent(hwndCtl)        ((int)(DWORD)SendMessage((hwndCtl), CALB_GETHORIZONTALEXTENT, 0, 0L))
#define CAListBox_SetHorizontalExtent(hwndCtl, cxExtent)     ((void)SendMessage((hwndCtl), CALB_SETHORIZONTALEXTENT, (WPARAM)(int)(cxExtent), 0L))

#define CAListBox_SetTabStops(hwndCtl, cTabs, lpTabs) ((BOOL)(DWORD)SendMessage((hwndCtl), CALB_SETTABSTOPS, (WPARAM)(int)(cTabs), (LPARAM)(int FAR*)(lpTabs)))

#define CAListBox_GetItemRect(hwndCtl, index, lprc)   ((int)(DWORD)SendMessage((hwndCtl), CALB_GETITEMRECT, (WPARAM)(int)(index), (LPARAM)(RECT FAR*)(lprc)))

#if (WINVER >= 0x030a)
#define CAListBox_FindStringExact(hwndCtl, indexStart, lpszFind) ((int)(DWORD)SendMessage((hwndCtl), CALB_FINDSTRINGEXACT, (WPARAM)(int)(indexStart), (LPARAM)(LPCSTR)(lpszFind)))

#define CAListBox_SetItemHeight(hwndCtl, index, cy)   ((int)(DWORD)SendMessage((hwndCtl), CALB_SETITEMHEIGHT, (WPARAM)(int)(index), MAKELPARAM((cy), 0)))
#define CAListBox_GetItemHeight(hwndCtl, index)       ((int)(DWORD)SendMessage((hwndCtl), CALB_GETITEMHEIGHT, (WPARAM)(int)(index), 0L))
#endif  /* WINVER >= 0x030a */

#endif	// _CATOLIST_H
