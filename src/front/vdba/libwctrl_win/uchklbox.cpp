/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : uchklbox.cpp, Implementation file
**    Project  : Prototype
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Owner draw LISTBOX to have a check list box.
**
** History:
** xx-Jan-1996 (uk$so01)
**    created.
** 10-Jul-2001 (uk$so01)
**    Get this file from IVM and put it in the library libwctrl.lib
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 28-Feb-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
**    And integrate sam modif on 01-nov-2001 (somsa01) Cleaned up 64-bit compiler warnings.
** 27-Jun-2003 (schph01)
**    (bug 110493) CuCheckListBox::OnLButtonDown() method, when the
**     calculated index is bigger than the number of items in the list box,
**     nothing to do.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "uchklbox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define CACHKPLATFORMW95    1
#define CACHKPLATFORMWNT    2
#define XMARGIN             1   // Margin  
//
// For Window 95
//
enum 
{
	IMAGE_95NOCHECK,            // Item is not checked
	IMAGE_95CHECK,              // Item is checked (Window 95 look)
	IMAGE_95CHECKGREY,          // Item is checked and grey (Window 95 look)    
	IMAGE_95NOCHECKGREY,        // Item is not checked and grey (Window NT & 95 look small)
	IMAGE_95NTCHECK,            // Item is checked (Window NT look small)
	IMAGE_95NTCHECKBOLD,        // Item is checked (Triple lines cross small) 
	IMAGE_95NTCHECKBOLDGREY,    // Item is checked and grey (Triple lines cross small)
	IMAGE_95NTCHECKGREY         // Item is checked and grey (Window NT small)
};
//
// For Window NT and Window 3.1
//
enum
{
	IMAGE_NTNOCHECK,            // Item is not checked
	IMAGE_NTCHECK,              // Item is checked
	IMAGE_NT3STATE,             // Item is marked as 3-states
	IMAGE_NTNOCHECKGREY,        // Item is not checked and grey
	IMAGE_NTCHECKGREY           // Item is checked and grey
};


class CuCheckListBoxItemData
{
public:
	CuCheckListBoxItemData();
	CuCheckListBoxItemData(BOOL bEnable, BOOL bCheck, BOOL bUseColor = FALSE, COLORREF colorref = NULL, DWORD dwData = 0);
	DWORD_PTR    m_dwUserData;
	BOOL         m_bEnable;
	BOOL         m_bChecked;
	BOOL         m_bUseColor;
	COLORREF     m_cColorRef;
};

CuCheckListBoxItemData::CuCheckListBoxItemData()
{
	m_dwUserData = (DWORD)0;
	m_bEnable    = TRUE;
	m_bChecked   = FALSE;
	m_bUseColor  = FALSE;
	m_cColorRef  = NULL;
}

CuCheckListBoxItemData::CuCheckListBoxItemData(BOOL bEnable, BOOL bCheck, BOOL bUseColor, COLORREF colorref, DWORD dwData)
{
	m_dwUserData = (DWORD)dwData;
	m_bEnable    = bEnable;
	m_bChecked   = bCheck;
	m_bUseColor  = bUseColor;
	m_cColorRef  = colorref;
}

/////////////////////////////////////////////////////////////////////////////
// CuCheckListBox

CuCheckListBox::CuCheckListBox()
{
	CBitmap bitmap;
	BITMAP  bm;
	VERIFY (bitmap.LoadBitmap (IDB_CHECKLISTBOX_95));
	bitmap.GetObject (sizeof(BITMAP), &bm);
	m_sizeCheck.cx = bm.bmWidth / 4;
	m_sizeCheck.cy = bm.bmHeight;
	m_hBitmapCheck = (HBITMAP)bitmap.Detach();
	m_bEnable      = TRUE; // The list box is enabled.
	m_bSelectDisableItem = FALSE;
}

CuCheckListBox::~CuCheckListBox()
{

}



BEGIN_MESSAGE_MAP(CuCheckListBox, CListBox)
	//{{AFX_MSG_MAP(CuCheckListBox)
	ON_WM_LBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_WM_CREATE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_SETFONT, OnSetFont)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuCheckListBox message handlers

BOOL CuCheckListBox::Create (DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID) 
{
	if (!(dwStyle & LBS_OWNERDRAWFIXED))
		dwStyle |= LBS_OWNERDRAWFIXED;
	if ((dwStyle & LBS_OWNERDRAWVARIABLE))
		dwStyle &= ~LBS_OWNERDRAWVARIABLE;
	
	return CListBox::Create (dwStyle, rect, pParentWnd, nID);
}

int CuCheckListBox::GetBitmapCheck (int index)
{
	CuCheckListBoxItemData* pItemData = (CuCheckListBoxItemData*)CListBox::GetItemData (index);    
	ASSERT (pItemData != NULL);
	//
	// For Window 95
	//
	if (pItemData->m_bEnable && !pItemData->m_bChecked)
		return IMAGE_95NOCHECK;
	else
	if (pItemData->m_bEnable && pItemData->m_bChecked)
		return IMAGE_95CHECK;
	else
	if (!pItemData->m_bEnable && pItemData->m_bChecked)
		return IMAGE_95CHECKGREY;
	else
	if (!pItemData->m_bEnable && !pItemData->m_bChecked)
		return IMAGE_95NOCHECKGREY;
	else
		return IMAGE_95NOCHECK;
}

int CuCheckListBox::CalcMinimumItemHeight()
{
	int nResult;
	int cyText;

	if ((GetStyle() & (LBS_HASSTRINGS|LBS_OWNERDRAWFIXED)) == (LBS_HASSTRINGS|LBS_OWNERDRAWFIXED))
	{
		CClientDC dc(this);
		CFont* pOldFont = dc.SelectObject(GetFont());
		TEXTMETRIC tm;
		VERIFY (dc.GetTextMetrics ( &tm ));
		dc.SelectObject(pOldFont);
		cyText  = tm.tmHeight;
		nResult = max (m_sizeCheck.cy+2, cyText);
	}
	else
	{
		nResult = m_sizeCheck.cy + 2;
	}
	return nResult;
}

int CuCheckListBox::CheckFromPoint(CPoint point, BOOL& bInCheck)
{
	bInCheck    = FALSE;
	int nIndex  = -1;

	if ((GetStyle() & LBS_OWNERDRAWFIXED) == LBS_OWNERDRAWFIXED)
	{
		//
		// Optimized case for ownerdraw fixed, single column
		int cyItem = GetItemHeight(0);
		if (point.y < cyItem * GetCount())
		{
			nIndex = GetTopIndex() + point.y / cyItem;
			if (point.x < m_sizeCheck.cx + 2)
				bInCheck = TRUE;;
		}
	}
	return nIndex;
}

void CuCheckListBox::InvalidateCheck(int nIndex)
{
	CRect rect;
	GetItemRect (nIndex, rect);
	rect.right = rect.left + m_sizeCheck.cx + 2;
	InvalidateRect(rect, FALSE);
}

BOOL CuCheckListBox::IsItemEnabled (int iIndex)
{
	CuCheckListBoxItemData* pItemData = (CuCheckListBoxItemData*)CListBox::GetItemData (iIndex);	
	ASSERT (pItemData != NULL);
	return pItemData->m_bEnable ? TRUE: FALSE;
}


BOOL CuCheckListBox::EnableItem (int iIndex, BOOL bEnabled)
{
	CRect rect;
	CuCheckListBoxItemData* pItemData = (CuCheckListBoxItemData*)CListBox::GetItemData (iIndex);	
	ASSERT (pItemData != NULL);
	pItemData->m_bEnable = bEnabled;
	GetItemRect (iIndex, rect);
	InvalidateRect (rect, FALSE);
	return TRUE;
}

BOOL CuCheckListBox::SetItemColor  (int iIndex, COLORREF colorref)
{
	CRect rect;
	CuCheckListBoxItemData* pItemData = (CuCheckListBoxItemData*)CListBox::GetItemData (iIndex);	
	ASSERT (pItemData != NULL);
	if (colorref != NULL)
	{
		pItemData->m_bUseColor = TRUE;
		pItemData->m_cColorRef = colorref;
	}
	else
	{
		pItemData->m_bUseColor = FALSE;
		pItemData->m_cColorRef = NULL;
	}
	GetItemRect (iIndex, rect);
	InvalidateRect (rect, FALSE);
	return TRUE;
}

int CuCheckListBox::SetCheck (int iIndex, BOOL bCheck)
{
	CuCheckListBoxItemData* pItemData = (CuCheckListBoxItemData*)CListBox::GetItemData (iIndex);	
	ASSERT (pItemData != NULL);
	pItemData->m_bChecked = bCheck;
	InvalidateCheck (iIndex);
	return iIndex; 
}

int CuCheckListBox::SetCheck (LPCTSTR lpszItem, BOOL bCheck)
{
	if (!lpszItem)
		return LB_ERR;
	int nIndex = FindStringExact (-1, lpszItem);
	if (nIndex != LB_ERR)
		return SetCheck (nIndex, bCheck);
	return LB_ERR;
}


BOOL CuCheckListBox::EnableWindow  (BOOL bEnable)
{
	CRect r;
	CListBox::EnableWindow (bEnable);
	m_bEnable = bEnable;
	return TRUE;
	GetClientRect  (r);
	InvalidateRect (r, FALSE);
};


BOOL CuCheckListBox::ResetContent ()
{
	CuCheckListBoxItemData* pItemData = NULL;
	int i, nCount = CListBox::GetCount();
	
	for (i=0; i<nCount; i++)
	{
		pItemData = (CuCheckListBoxItemData*)CListBox::GetItemData (i);
		if (pItemData) delete pItemData;
	}
	CListBox::ResetContent();
	return TRUE;
}

int CuCheckListBox::DeleteString (int iIndex)
{
	ASSERT (iIndex != LB_ERR);
	if (iIndex == LB_ERR)
		return LB_ERR;
	CuCheckListBoxItemData* pItemData = (CuCheckListBoxItemData*)CListBox::GetItemData (iIndex);
	int nRet = CListBox::DeleteString(iIndex);
	if (nRet != LB_ERR)
	{
		if (pItemData)
			delete pItemData;
	}
	return nRet;
}



BOOL CuCheckListBox::GetCheck (int iIndex)
{
	CuCheckListBoxItemData* pItemData = NULL;
	pItemData = (CuCheckListBoxItemData*)CListBox::GetItemData (iIndex);
	ASSERT (pItemData);
	return pItemData->m_bChecked;
}

int CuCheckListBox::AddString  (LPCTSTR lpszText)
{
	int index = CListBox::AddString (lpszText);
	if (index != LB_ERR)
	{
		CuCheckListBoxItemData* pItemData;
		pItemData = new CuCheckListBoxItemData (TRUE, FALSE);
		CListBox::SetItemData (index, (DWORD_PTR)pItemData);
	}
	return index;
}

int CuCheckListBox::AddString  (LPCTSTR lpszText, COLORREF cColor)
{
	//
	// TODO: Implement this functionality !!
	ASSERT (FALSE);
	return -1;
}


int CuCheckListBox::InsertString (int iIndex, LPCTSTR lpszText)
{
	int index = CListBox::InsertString (iIndex, lpszText);
	if (index != LB_ERR)
	{
		CuCheckListBoxItemData* pItemData;
		pItemData = new CuCheckListBoxItemData (TRUE, FALSE);
		CListBox::SetItemData (index, (DWORD_PTR)pItemData);
	}
	return index;
}

	
void CuCheckListBox::PreDrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	DRAWITEMSTRUCT drawItem;
	memcpy (&drawItem, lpDrawItemStruct, sizeof(DRAWITEMSTRUCT));
	BOOL bPreDraw = ((LONG)drawItem.itemID >= 0) && ((drawItem.itemAction & (ODA_DRAWENTIRE|ODA_SELECT)) != 0);
	
	if (bPreDraw)
	{
		int  cyItem= GetItemHeight  (drawItem.itemID);
		CDC* pDC   = CDC::FromHandle(drawItem.hDC);
		//
		// Determine the background color for the current item.
		COLORREF newBkColor = GetSysColor (COLOR_WINDOW);
		BOOL bDisabled = !m_bEnable || !IsItemEnabled (drawItem.itemID);
		if ((drawItem.itemState & ODS_SELECTED) && !bDisabled)
			newBkColor = GetSysColor (COLOR_HIGHLIGHT);
		COLORREF oldBkColor = pDC->SetBkColor (newBkColor);
		//
		// Draw the bitmap (check/uncheck bitmap)
		//
		CDC bitmapDC;
		if (bitmapDC.CreateCompatibleDC(pDC))
		{
			int nBitmap = GetBitmapCheck (drawItem.itemID);
			HBITMAP hOldBitmap = (HBITMAP)::SelectObject(bitmapDC.m_hDC, m_hBitmapCheck);
			// 
			// Determine the rectangle for drawing the check bitmap
			//
			CRect rectCheck = drawItem.rcItem;
			int y = abs((drawItem.rcItem.bottom - drawItem.rcItem.top - cyItem)) / 2;
			rectCheck.left += 1;
			rectCheck.top  += y+2;
			rectCheck.right = rectCheck.left + m_sizeCheck.cx;
			rectCheck.bottom= rectCheck.top  + m_sizeCheck.cy;
	
			CRect rectItem = drawItem.rcItem;
			rectItem.right = rectItem.left + m_sizeCheck.cx + 2;
			CBrush brush   (GetSysColor (COLOR_WINDOW));
			//
			// Erase the background for drawing the bitmap.
			pDC->FillRect(rectItem, &brush);
			pDC->BitBlt (
				rectCheck.left, 
				rectCheck.top,
				m_sizeCheck.cx, 
				m_sizeCheck.cy, 
				&bitmapDC,
				m_sizeCheck.cx  * nBitmap, 
				0, 
				SRCCOPY);
			::SelectObject (bitmapDC.m_hDC, hOldBitmap);
		}
		pDC->SetBkColor (oldBkColor);
		//
		// Call DrawItem the item text.
		drawItem.rcItem.left = drawItem.rcItem.left + m_sizeCheck.cx + 2;
		DrawItem(&drawItem);
	}
}

void CuCheckListBox::PreMeasureItem (LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	int cyItem = CalcMinimumItemHeight();
	MEASUREITEMSTRUCT measureItem;
	memcpy (&measureItem, lpMeasureItemStruct, sizeof(MEASUREITEMSTRUCT));
	measureItem.itemHeight = cyItem;
	measureItem.itemWidth  = (UINT)-1;
	MeasureItem(&measureItem);
}


void CuCheckListBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	//
	// You must override DrawItem and MeasureItem for LBS_OWNERDRAWVARIABLE
	//
	COLORREF newTextColor;
	COLORREF oldTextColor;
	COLORREF newBkColor;
	COLORREF oldBkColor;
	ASSERT((GetStyle() & (LBS_OWNERDRAWFIXED|LBS_HASSTRINGS)) == (LBS_OWNERDRAWFIXED|LBS_HASSTRINGS));
	CDC* pDC = CDC::FromHandle (lpDrawItemStruct->hDC);
	BOOL bPreDraw = ((LONG)lpDrawItemStruct->itemID >= 0) && ((lpDrawItemStruct->itemAction & (ODA_DRAWENTIRE|ODA_SELECT)) != 0);
	if (bPreDraw)
	{
		int  cyItem    = GetItemHeight (lpDrawItemStruct->itemID);
		BOOL bDisabled = !m_bEnable || !IsItemEnabled (lpDrawItemStruct->itemID);
		//
		// Determine the text color
		CuCheckListBoxItemData* pItemData = (CuCheckListBoxItemData*)CListBox::GetItemData (lpDrawItemStruct->itemID);
		ASSERT (pItemData);
		if (pItemData->m_bUseColor)
			newTextColor = pItemData->m_cColorRef;
		else
			newTextColor = bDisabled ? RGB(0x80, 0x80, 0x80): GetSysColor(COLOR_WINDOWTEXT); 
		oldTextColor = pDC->SetTextColor(newTextColor);

		newBkColor = GetSysColor(COLOR_WINDOW);
		oldBkColor = pDC->SetBkColor(newBkColor);
		//
		// Determine the background color
		if (newTextColor == newBkColor)
			newTextColor = RGB(0xC0, 0xC0, 0xC0);  
		
		if ((!bDisabled||m_bSelectDisableItem) && ((lpDrawItemStruct->itemState & ODS_SELECTED) != 0))
		{
			pDC->SetTextColor (GetSysColor(COLOR_HIGHLIGHTTEXT));
			pDC->SetBkColor (GetSysColor(COLOR_HIGHLIGHT));
		}
		//
		// Draw the string in the list box.
		int y;
		CString strText;
		CListBox::GetText (lpDrawItemStruct->itemID, strText);
		y = (lpDrawItemStruct->rcItem.bottom - lpDrawItemStruct->rcItem.top - cyItem) / 2;
		pDC->ExtTextOut (
			lpDrawItemStruct->rcItem.left,
			lpDrawItemStruct->rcItem.top + y,
			ETO_OPAQUE, 
			&(lpDrawItemStruct->rcItem), 
			strText, 
			strText.GetLength(), 
			NULL);
		pDC->SetTextColor (oldTextColor);
		pDC->SetBkColor   (oldBkColor);
	}
	if ((lpDrawItemStruct->itemAction & ODA_FOCUS) != 0)
		pDC->DrawFocusRect (&(lpDrawItemStruct->rcItem));
}

void CuCheckListBox::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
	//
	// You must override DrawItem and MeasureItem for LBS_OWNERDRAWVARIABLE
	//
	ASSERT((GetStyle() & (LBS_OWNERDRAWFIXED|LBS_HASSTRINGS)) == (LBS_OWNERDRAWFIXED|LBS_HASSTRINGS));
}

BOOL CuCheckListBox::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult) 
{
	switch (message)
	{
	case WM_DRAWITEM:
		ASSERT(pLResult == NULL);       // no return value expected
		PreDrawItem((LPDRAWITEMSTRUCT)lParam);
		break;
	case WM_MEASUREITEM:
		ASSERT(pLResult == NULL);       // no return value expected
		PreMeasureItem((LPMEASUREITEMSTRUCT)lParam);
		break;
	default:
		return CListBox::OnChildNotify(message, wParam, lParam, pLResult);
	}
	return TRUE;
}

void CuCheckListBox::PreSubclassWindow() 
{
	CListBox::PreSubclassWindow();
	ASSERT(GetStyle() & (LBS_OWNERDRAWFIXED|LBS_HASSTRINGS));
}

void CuCheckListBox::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CuCheckListBoxItemData* pItemData;
	SetFocus();
	//
	// Determine where the click is
	BOOL bInCheck;
	int nIndex = CheckFromPoint(point, bInCheck);
	if (nIndex == -1)
		return;
	if (nIndex > (GetCount()-1))
		return;
	BOOL bEnable = IsItemEnabled (nIndex);
	if (!bEnable && !m_bSelectDisableItem)
		return;
	//
	// Toggle the check
	if (bInCheck)
	{
		CWnd*	pParent = GetParent();
		ASSERT_VALID(pParent);
		pItemData = (CuCheckListBoxItemData*)CListBox::GetItemData (nIndex);
		pItemData->m_bChecked = !pItemData->m_bChecked;
		InvalidateCheck (nIndex);
		CListBox::OnLButtonDown(nFlags, point);
		pParent->SendMessage (WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), CLBN_CHKCHANGE), (LPARAM)m_hWnd);
		return;
	}
	CListBox::OnLButtonDown(nFlags, point);
}

void CuCheckListBox::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	CuCheckListBoxItemData* pItemData;
	if (nChar != VK_SPACE)
	{
		CListBox::OnKeyDown(nChar, nRepCnt, nFlags);
		return;
	}

	int nIndex = GetCaretIndex();
	CWnd* pParent = GetParent();
	ASSERT_VALID(pParent);
	if (nIndex != LB_ERR && IsItemEnabled(nIndex))
	{
		pItemData = (CuCheckListBoxItemData*)CListBox::GetItemData (nIndex);
		ASSERT (pItemData);
		pItemData->m_bChecked = !pItemData->m_bChecked;
		InvalidateCheck (nIndex);
		pParent->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), CLBN_CHKCHANGE), (LPARAM)m_hWnd);
	}
	CListBox::OnKeyDown(nChar, nRepCnt, nFlags);
}

int CuCheckListBox::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CListBox::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	// TODO: Add your specialized creation code here
	
	return 0;
}

void CuCheckListBox::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	
	CListBox::OnLButtonDblClk(nFlags, point);
}




void CuCheckListBox::OnDestroy() 
{
	CuCheckListBoxItemData* pItemData = NULL;
	int i, nCount = CListBox::GetCount();

	for (i=0; i<nCount; i++)
	{
		pItemData = (CuCheckListBoxItemData*)CListBox::GetItemData (i);
		if (pItemData) delete pItemData;
	}
	CListBox::OnDestroy();
}

LRESULT CuCheckListBox::OnSetFont(WPARAM , LPARAM)
{
	Default();

	if ((GetStyle() & (LBS_OWNERDRAWFIXED | LBS_HASSTRINGS)) == (LBS_OWNERDRAWFIXED | LBS_HASSTRINGS))
		SetItemHeight(0, CalcMinimumItemHeight());
	return 0;
}


int CuCheckListBox::SetItemData (int iIndex, DWORD_PTR dwItemData)
{
	CuCheckListBoxItemData* pItemData = NULL;
	pItemData = (CuCheckListBoxItemData*)CListBox::GetItemData (iIndex);
	if (pItemData) 
	{
		pItemData->m_dwUserData = dwItemData;
	}
	return iIndex;
}

DWORD_PTR CuCheckListBox::GetItemData (int iIndex)
{
	CuCheckListBoxItemData* pItemData = NULL;
	pItemData = (CuCheckListBoxItemData*)CListBox::GetItemData (iIndex);
	if (pItemData) 
	{
		return (DWORD_PTR)pItemData->m_dwUserData;
	}
	return (DWORD_PTR)0;
}

int CuCheckListBox::GetCheckCount()
{
	int nCounter = 0;
	int i, nCount = GetCount();
	for (i=0; i<nCount; i++)
	{
		if (GetCheck(i) == TRUE)
			nCounter++;
	}
	return nCounter;
}
