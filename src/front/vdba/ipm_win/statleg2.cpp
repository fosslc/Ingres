/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : statleg2.cpp, Implementation file
**    Project  : CA-OpenIngres/Monitoring
**    Author   : Emmanuel Blattes
**    Purpose  : Owner draw listbox derived from CuStatisticBarLegendList2
**               having a different background color compatible with dialog box
**
** History:
**
** xx-Apr-1997 (Emmanuel Blattes)
**    Created
*/


#include "stdafx.h"
#include "statleg2.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#if !defined (STATFRM_HEADER)
CuStatisticBarLegendList::CuStatisticBarLegendList()
{
	m_nXHatchWidth  = 4;
	m_nXHatchHeight = 6;
	m_bHatch = FALSE;
	m_nRectColorWidth = 20;
	m_colorText  = ::GetSysColor (COLOR_WINDOWTEXT);
	m_colorTextBk = ::GetSysColor (COLOR_WINDOW);
}


CuStatisticBarLegendList::~CuStatisticBarLegendList()
{
}

int CuStatisticBarLegendList::AddItem (LPCTSTR lpszItem, COLORREF color)
{
	int index = AddString (lpszItem);
	if (index != LB_ERR)
		SetItemData (index, (DWORD)color);
	return index;
}

int CuStatisticBarLegendList::AddItem2(LPCTSTR lpszItem, DWORD dwItemData)
{
	m_bHatch = TRUE;
	int index = AddString (lpszItem);
	if (index != LB_ERR)
		SetItemData (index, dwItemData);
	return index;
}

BEGIN_MESSAGE_MAP(CuStatisticBarLegendList, CListBox)
	//{{AFX_MSG_MAP(CuStatisticBarLegendList)
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuStatisticBarLegendList message handlers

BOOL CuStatisticBarLegendList::PreCreateWindow(CREATESTRUCT& cs) 
{
	if ((cs.style & LBS_OWNERDRAWFIXED) == 0)
		cs.style |= LBS_OWNERDRAWFIXED;
	if ((cs.style & LBS_MULTICOLUMN) == 0)
		cs.style |= LBS_MULTICOLUMN;
	if ((cs.style & LBS_HASSTRINGS) == 0)
		cs.style |= LBS_HASSTRINGS;
	return CListBox::PreCreateWindow(cs);
}

void CuStatisticBarLegendList::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
	lpMeasureItemStruct->itemHeight = 20;
}

#define X_LEFTMARGIN 2
#define X_SEPERATOR  4

void CuStatisticBarLegendList::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	COLORREF cr = NULL;
	CaBarPieItem* pData = NULL;

	int nPreviousBkMode = 0;
	if ((int)lpDrawItemStruct->itemID < 0)
		return;
	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	if (m_bHatch)
	{
		pData = (CaBarPieItem*)lpDrawItemStruct->itemData;
		ASSERT (pData);
		if (!pData)
			return;
		cr = pData->m_colorRGB;
	}
	else
		cr = (COLORREF)lpDrawItemStruct->itemData; // RGB in item data

	CBrush brush(m_colorTextBk);
	pDC->FillRect (&lpDrawItemStruct->rcItem, &brush);
	
	CRect r = lpDrawItemStruct->rcItem;
	r.top    = r.top + 1;
	r.bottom = r.bottom - 1;
	r.left   = r.left + X_LEFTMARGIN;
	r.right  = r.left + m_nRectColorWidth;
	
	CBrush brush2(cr);
	pDC->FillRect (r, &brush2);
	if (m_bHatch && pData && pData->m_bHatch)
	{
		CRect rh = r;
		CBrush br(pData->m_colorHatch);
		//
		// Draw two rectangles:
	
		// upper:
		rh.left    = rh.right - m_nXHatchWidth - 4;
		rh.right   = rh.right - 2;
		rh.top     = rh.top    + 2;
		rh.bottom  = rh.top + m_nXHatchHeight;
		pDC->FillRect (rh, &br);
	
		// lower:
		rh = r;
		rh.left    = rh.right - m_nXHatchWidth - 4;
		rh.right   = rh.right - 2;
		rh.bottom  = rh.bottom - 1;
		rh.top     = rh.bottom - m_nXHatchHeight; 
		pDC->FillRect (rh, &br);
	}

	CRect rcText = lpDrawItemStruct->rcItem;
	CString strItem;
	GetText (lpDrawItemStruct->itemID, strItem);
	rcText.left = r.right + X_SEPERATOR;
	nPreviousBkMode = pDC->SetBkMode (TRANSPARENT);
	pDC->DrawText (strItem, rcText, DT_LEFT|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
	pDC->SetBkMode (nPreviousBkMode);
}


void CuStatisticBarLegendList::OnLButtonDown(UINT nFlags, CPoint point) 
{
	//
	// Just swallow the message WM_LBUTTONDOWN
	return;
}

void CuStatisticBarLegendList::OnSysColorChange() 
{
	CListBox::OnSysColorChange();
	m_colorText = ::GetSysColor (COLOR_WINDOWTEXT);
	m_colorTextBk = ::GetSysColor (COLOR_WINDOW);
}
#endif // STATFRM_HEADER


CuStatisticBarLegendList2::CuStatisticBarLegendList2()
  :CuStatisticBarLegendList()
{
	m_colorTextBk       = ::GetSysColor (COLOR_BTNFACE);
}


CuStatisticBarLegendList2::~CuStatisticBarLegendList2()
{
}

BEGIN_MESSAGE_MAP(CuStatisticBarLegendList2, CuStatisticBarLegendList)
	//{{AFX_MSG_MAP(CuStatisticBarLegendList2)
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP

	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuStatisticBarLegendList2 message handlers

void CuStatisticBarLegendList2::OnSysColorChange() 
{
	CuStatisticBarLegendList::OnSysColorChange();
	m_colorTextBk       = ::GetSysColor (COLOR_BTNFACE);
}

BOOL CuStatisticBarLegendList2::OnEraseBkgnd(CDC* pDC) 
{
	COLORREF colorBk = ::GetSysColor (COLOR_BTNFACE);
	CBrush bkBrush;
	bkBrush.CreateSolidBrush(colorBk);
	CRect rect;
	GetClientRect(&rect);
	pDC->FillRect(&rect, &bkBrush);

	return TRUE;    // I have processed!
}
