/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : lbfixedw.cpp : implementation file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Listbox control (owner drawn that allows to paint the divider
**               of column on it (fixed width control associated with ruler)
**
** History:
**
** 19-Dec-2000 (uk$so01)
**    Created
**/


#include "stdafx.h"
#include "lbfixedw.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuListBoxFixedWidth::CuListBoxFixedWidth()
{
	m_pRuler  = NULL;
}

CuListBoxFixedWidth::~CuListBoxFixedWidth()
{
}


BEGIN_MESSAGE_MAP(CuListBoxFixedWidth, CListBox)
	//{{AFX_MSG_MAP(CuListBoxFixedWidth)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



BOOL CuListBoxFixedWidth::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!(cs.style & LBS_OWNERDRAWFIXED))
		cs.style |= LBS_OWNERDRAWFIXED;
	if (!(cs.style & LBS_HASSTRINGS))
		cs.style |= LBS_HASSTRINGS;
	return CListBox::PreCreateWindow(cs);
}


BOOL CuListBoxFixedWidth::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult) 
{
	switch (message)
	{
	case WM_DRAWITEM:
		ASSERT(pLResult == NULL); // no return value expected
		PreDrawItem((LPDRAWITEMSTRUCT)lParam);
		break;
	case WM_MEASUREITEM:
		ASSERT(pLResult == NULL);// no return value expected
		break;
	default:
		return CListBox::OnChildNotify(message, wParam, lParam, pLResult);
	}
	return TRUE;
}


void CuListBoxFixedWidth::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
	//
	// You must override DrawItem and MeasureItem for LBS_OWNERDRAWVARIABLE
	//
	ASSERT((GetStyle() & (LBS_OWNERDRAWFIXED|LBS_HASSTRINGS)) == (LBS_OWNERDRAWFIXED|LBS_HASSTRINGS));	
}

void CuListBoxFixedWidth::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
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
		//
		// Determine the text color
		newTextColor = GetSysColor(COLOR_WINDOWTEXT);
		oldTextColor = pDC->SetTextColor(newTextColor);

		newBkColor = GetSysColor(COLOR_WINDOW);
		oldBkColor = pDC->SetBkColor(newBkColor);
		//
		// Determine the background color
		if (newTextColor == newBkColor)
			newTextColor = RGB(0xC0, 0xC0, 0xC0);
		
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

	//
	// Draw column width separators:
	if (m_pRuler && IsWindow (m_pRuler->m_hWnd))
	{
		CArray< CPoint,CPoint > pointArray;
		int nOffsetOrigin = m_pRuler->GetPointArray (pointArray);
		int nBkOldMode = pDC->SetBkMode(TRANSPARENT);
		CPen p (PS_SOLID, 1, RGB(255, 0, 0));
		CPen* pOldP = pDC->SelectObject (&p);
		CRect rView;
		GetClientRect(rView);
		int nSize = pointArray.GetSize();
		for (int i=0; i<nSize; i++)
		{
			CPoint pt = pointArray[i];
			pDC->MoveTo(pt.x+1 , lpDrawItemStruct->rcItem.top);
			pDC->LineTo(pt.x+1 , lpDrawItemStruct->rcItem.bottom);
		}

		pDC->SelectObject (pOldP);
		pointArray.RemoveAll();
		pDC->SetBkMode(nBkOldMode);
	}
}


int CuListBoxFixedWidth::CalcMinimumItemHeight()
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
		nResult = cyText;
	}

	return nResult;
}

void CuListBoxFixedWidth::PreMeasureItem (LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	int cyItem = CalcMinimumItemHeight();
	MEASUREITEMSTRUCT measureItem;
	memcpy (&measureItem, lpMeasureItemStruct, sizeof(MEASUREITEMSTRUCT));
	measureItem.itemHeight = cyItem;
	measureItem.itemWidth  = (UINT)-1;
	MeasureItem(&measureItem);
}


void CuListBoxFixedWidth::PreDrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct)
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
		//
		// Call DrawItem the item text.
		//drawItem.rcItem.left = drawItem.rcItem.left + m_sizeCheck.cx + 2;
		DrawItem(&drawItem);
	}
}


void CuListBoxFixedWidth::OnPaint() 
{
	CListBox::OnPaint();
}
