/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : lstbstat.cpp, Implementation file 
**    Project  : CA-OpenIngres/Vdba
**    Author   : UK Sotheavut
**    Purpose  : Owner draw LISTCTRL that can draw 3D horizontal bar in the column 
**
** History after 01-01-2000
**
** xx-Mar-1998 (uk$so01)
**    created.
** 28-Jan-2000 (noifr01 for uk$so01)
**    (bug 100200) a build warning that appeared only in release mode was resulting in the
**    bug. Initialized the rcItem variable , that was not initialized previously
** 04-Jun-2002 (uk$so01)
**    SIR #105384 Adjust the header witdth when double-ckicking on the divider
** 28-Mar-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "lstbstat.h"
#include "tbstatis.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const int R_FACE_PROJECT_WIDTH = 5;
static const int U_FACE_PROJECT_WIDTH = 5;

CuListCtrlTableStatistic::CuListCtrlTableStatistic():CuListCtrl()
{
	SetLineSeperator(FALSE);
	m_dMaxPercentage = 0.0;
	m_dMaxRepetitionFactor = 0.0;

	m_crFaceFront = m_colorHighLight;
	m_crFaceUpper = GetSysColor (COLOR_GRAYTEXT);
	m_crFaceRight = GetSysColor (COLOR_3DLIGHT);
}

CuListCtrlTableStatistic::~CuListCtrlTableStatistic()
{
}

UINT CuListCtrlTableStatistic::GetColumnJustify(int nCol)
{
	LV_COLUMN lvc; 
	UINT nJustify = DT_LEFT;
	memset (&lvc, 0, sizeof (lvc));
	lvc.mask = LVCF_FMT | LVCF_WIDTH;
	if (CListCtrl::GetColumn (nCol, &lvc))
	{
		switch(lvc.fmt & LVCFMT_JUSTIFYMASK)
		{
		case LVCFMT_RIGHT:
			nJustify = DT_RIGHT;
			break;
		case LVCFMT_CENTER:
			nJustify = DT_CENTER;
			break;
		default:
			nJustify = DT_LEFT;
			break;
		}
	}
	return nJustify;
}

BEGIN_MESSAGE_MAP(CuListCtrlTableStatistic, CuListCtrl)
	//{{AFX_MSG_MAP(CuListCtrlTableStatistic)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
	//
	// HDN_DIVIDERDBLCLICK is mapped to iether HDN_DIVIDERDBLCLICKA or HDN_DIVIDERDBLCLICKW
	// but the header always sends -325 which is HDN_DIVIDERDBLCLICKW (I don't know if this is a bug).
	// So that's why I map HDN_DIVIDERDBLCLICKA and HDN_DIVIDERDBLCLICKW to the same handler OnDividerdblclick():
	ON_NOTIFY(HDN_DIVIDERDBLCLICKA, 0, OnDividerdblclick)
	ON_NOTIFY(HDN_DIVIDERDBLCLICKW, 0, OnDividerdblclick)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuListCtrlTableStatistic message handlers


BOOL CuListCtrlTableStatistic::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult) 
{
	switch (message)
	{
	case WM_DRAWITEM:
		ASSERT(pLResult == NULL);    // No return value expected
		DrawItem ((LPDRAWITEMSTRUCT)lParam);
		break;
	default:
		return CListCtrl::OnChildNotify(message, wParam, lParam, pLResult);
	}
	return TRUE;
}


void CuListCtrlTableStatistic::DrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CString  pszItem;
	CString  strItem;
	COLORREF colorTextOld;
	
	CDC* pDC = CDC::FromHandle (lpDrawItemStruct->hDC);
	if ((int)lpDrawItemStruct->itemID < 0)
		return;
	int  nItem     = lpDrawItemStruct->itemID;
	BOOL bFocus    = GetFocus() == this;
	CaTableStatisticItem* pItemData = (CaTableStatisticItem*)lpDrawItemStruct->itemData;
	
	CRect rcItem;
	CRect rcAllLabels;
	CRect rcLabel;
	GetItemRect (nItem, rcAllLabels, LVIR_BOUNDS);
	GetItemRect (nItem, rcLabel,     LVIR_LABEL);
	rcAllLabels.left = rcLabel.left;

	if (m_bFullRowSelected)
	{
		if (rcAllLabels.right < m_cxClient)
			rcAllLabels.right = m_cxClient;
	}
	CRect r = rcAllLabels;
	r.top++;
	CBrush brushSelect(m_colorHighLight);
	CBrush brushNormal(m_colorTextBk);
	colorTextOld = pDC->SetTextColor (m_colorText);
	pDC->FillRect (r, &brushNormal);

	rcItem = rcLabel;
	strItem.Format (_T("%d"), nItem+1);
	pszItem = MakeShortString (pDC, strItem, rcItem.right - rcItem.left, 2*OFFSET_OTHER);
	//
	// Draw the Main Item Label #
	GetItemRect (nItem, rcItem, LVIR_LABEL);
	rcLabel = rcItem;
	rcLabel.left  += OFFSET_FIRST;
	rcLabel.right -= OFFSET_FIRST;

	UINT nJustify = GetColumnJustify (STATITEM_NUM);
	pDC->DrawText (pszItem, rcLabel, nJustify|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);

	CPen  penGray (PS_SOLID, 1, m_colorLine);
	CPen* pOldPen = pDC->SelectObject (&penGray);
	if (m_bLineSeperator)
	{
		pDC->MoveTo (rcItem.right-2, rcItem.top);
		pDC->LineTo (rcItem.right-2, rcItem.bottom);
	}

	//
	// Draw the item: Value:
	nJustify = GetColumnJustify (STATITEM_VALUE);
	rcItem.left   = rcItem.right;
	rcItem.right += GetColumnWidth(STATITEM_VALUE);
	strItem = pItemData->m_strValue;
	pszItem = MakeShortString (pDC, strItem, rcItem.right - rcItem.left, 2*OFFSET_OTHER);
	if (strItem.GetLength() > 0) 
	{
		rcLabel = rcItem;
		rcLabel.left  += OFFSET_FIRST;
		rcLabel.right -= OFFSET_FIRST;
		pDC->DrawText (pszItem, rcLabel, nJustify|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
	}
	if (m_bLineSeperator)
	{
		pDC->MoveTo (rcItem.right-1, rcItem.top);
		pDC->LineTo (rcItem.right-1, rcItem.bottom);
	}
	//
	// Draw the item: %:
	nJustify = GetColumnJustify (STATITEM_PERCENTAGE);
	rcItem.left   = rcItem.right;
	rcItem.right += GetColumnWidth(STATITEM_PERCENTAGE);
	strItem.Format (_T("%0.2f"), pItemData->m_dPercentage);
	pszItem = MakeShortString (pDC, strItem, rcItem.right - rcItem.left, 2*OFFSET_OTHER);
	if (strItem.GetLength() > 0) 
	{
		rcLabel = rcItem;
		rcLabel.left  += OFFSET_FIRST;
		rcLabel.right -= OFFSET_FIRST;
		pDC->DrawText (pszItem, rcLabel, nJustify|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
	}
	if (m_bLineSeperator)
	{
		pDC->MoveTo (rcItem.right-1, rcItem.top);
		pDC->LineTo (rcItem.right-1, rcItem.bottom);
	}
	//
	// Draw the Graph: 
	nJustify = GetColumnJustify (STATITEM_GRAPH1);
	rcItem.left   = rcItem.right;
	rcItem.right += GetColumnWidth(STATITEM_GRAPH1);
	if (GetColumnWidth(3) > 10)
		DrawBar(pDC, rcItem, pItemData, FALSE);
	if (m_bLineSeperator)
	{
		pDC->MoveTo (rcItem.right-1, rcItem.top);
		pDC->LineTo (rcItem.right-1, rcItem.bottom);
	}
	//
	// Draw the Rep Factor: 
	nJustify = GetColumnJustify (STATITEM_REPFACTOR);
	rcItem.left   = rcItem.right;
	rcItem.right += GetColumnWidth(STATITEM_REPFACTOR);
	strItem.Format (_T("%0.2f"), pItemData->m_dRepetitionFactor);
	pszItem = MakeShortString (pDC, strItem, rcItem.right - rcItem.left, 2*OFFSET_OTHER);
	if (strItem.GetLength() > 0) 
	{
		rcLabel = rcItem;
		rcLabel.left  += OFFSET_FIRST;
		rcLabel.right -= OFFSET_FIRST;
		pDC->DrawText (pszItem, rcLabel, nJustify|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
	}
	if (m_bLineSeperator)
	{
		pDC->MoveTo (rcItem.right-1, rcItem.top);
		pDC->LineTo (rcItem.right-1, rcItem.bottom);
	}
	//
	// Draw the Graph: 
	nJustify = GetColumnJustify (STATITEM_GRAPH2);
	rcItem.left   = rcItem.right;
	rcItem.right += GetColumnWidth(STATITEM_GRAPH2);
	if (GetColumnWidth(5) > 10)
		DrawBar(pDC, rcItem, pItemData, TRUE);
	if (m_bLineSeperator)
	{
		pDC->MoveTo (rcItem.right-1, rcItem.top);
		pDC->LineTo (rcItem.right-1, rcItem.bottom);
	}
	if (m_bLineSeperator)
	{
		pDC->MoveTo (rcAllLabels.left,  rcAllLabels.top);
		pDC->LineTo (rcAllLabels.right, rcAllLabels.top);
		pDC->MoveTo (rcAllLabels.left,  rcAllLabels.bottom);
		pDC->LineTo (rcAllLabels.right, rcAllLabels.bottom);
	}
	pDC->SelectObject (pOldPen);
	pDC->SetTextColor (colorTextOld);
}

double CuListCtrlTableStatistic::GetPercentUnit()
{
	double dW = (double)GetColumnWidth(3) - (double)R_FACE_PROJECT_WIDTH - 4.0;
	return (dW / m_dMaxPercentage);
}

double CuListCtrlTableStatistic::GetRepFactorUnit()
{
	double dW = (double)GetColumnWidth(5) - (double)R_FACE_PROJECT_WIDTH - 4.0;
	return (dW / m_dMaxRepetitionFactor);
}

void CuListCtrlTableStatistic::DrawBar(CDC* pDC, CRect r, CaTableStatisticItem* pData, BOOL bRep)
{
	CBrush* pOldBrush = NULL;
	CPen*   pOldPen   = NULL;
	CPoint p [4];
	double dWidth = 0.0;
	dWidth = bRep? pData->m_dRepetitionFactor*GetRepFactorUnit(): pData->m_dPercentage*GetPercentUnit();
	r.top   += 1;
	r.bottom-= 1;
	r.left  += OFFSET_FIRST;
	//
	// Draw the front face:
	CRect rcFront = r;
	CBrush brushFront(m_crFaceFront);
	rcFront.top  += (U_FACE_PROJECT_WIDTH + 2);
	rcFront.right = rcFront.left + (int)dWidth;
	pDC->FillRect (rcFront, &brushFront);
	//
	// Draw the upper face:
	CBrush  brushUpper(m_crFaceUpper);
	CPen    penColorUpper(PS_SOLID, 1, m_crFaceUpper);
	pOldBrush = pDC->SelectObject (&brushUpper);
	pOldPen   = pDC->SelectObject (&penColorUpper); 
	p[0] = CPoint (rcFront.left, rcFront.top-1);
	p[1] = CPoint (rcFront.left +R_FACE_PROJECT_WIDTH, rcFront.top-1-U_FACE_PROJECT_WIDTH);
	p[2] = CPoint (rcFront.right+R_FACE_PROJECT_WIDTH, rcFront.top-1-U_FACE_PROJECT_WIDTH);
	p[3] = CPoint (rcFront.right, rcFront.top-1);
	pDC->Polygon (p, 4);
	//
	// Draw the right face:
	CBrush  brushRight(m_crFaceRight);
	CPen    penColorRight(PS_SOLID, 1, m_crFaceRight);
	pDC->SelectObject (&brushRight);
	pDC->SelectObject (&penColorRight);
	
	p[0] = CPoint (rcFront.right, rcFront.top-1);
	p[1] = CPoint (rcFront.right+R_FACE_PROJECT_WIDTH, rcFront.top-1-U_FACE_PROJECT_WIDTH);
	p[2] = CPoint (rcFront.right+R_FACE_PROJECT_WIDTH, rcFront.bottom-1-U_FACE_PROJECT_WIDTH);
	p[3] = CPoint (rcFront.right, rcFront.bottom-1);
	pDC->Polygon (p, 4);

	pDC->SelectObject (pOldBrush);
	pDC->SelectObject (pOldPen); 
}


void CuListCtrlTableStatistic::OnDividerdblclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMHEADER pHdr = (LPNMHEADER)pNMHDR;
	*pResult = 0;
	if (pHdr->hdr.idFrom == 0 && (pHdr->hdr.code == HDN_DIVIDERDBLCLICKA || pHdr->hdr.code == HDN_DIVIDERDBLCLICKW))
	{
		int i, nMax [6];
		int nCount = GetItemCount();
		CPaintDC dc (this);
		CString strItem;
		CSize s;

		strItem.Format (_T("%d"), nCount);
		GetTextExtentPoint32(dc.m_hDC, strItem, strItem.GetLength(), &s);
		nMax[0] = s.cx;
		nMax[1] = 0;
		strItem = _T("88.88");
		GetTextExtentPoint32(dc.m_hDC, strItem, strItem.GetLength(), &s);
		nMax[2] = s.cx;
		nMax[3] = 0;
		nMax[4] = s.cx;
		nMax[5] = 0;


		switch (pHdr->iItem)
		{
		case 0: // # 
		case 1: // Value 
		case 2: // count 
		case 3: // graph (auto adjust)
		case 4: // rep 
		case 5: // graph (auto adjust)
			for (i=0; i<nCount; i++)
			{
				CaTableStatisticItem* pItemData = (CaTableStatisticItem*)GetItemData(i);
				GetTextExtentPoint32(dc.m_hDC, pItemData->m_strValue, pItemData->m_strValue.GetLength(), &s);
				nMax[1] = max (nMax[1], s.cx);
			}
			break;
		default:
			break;
		}

		int nPad = 20;
		switch (pHdr->iItem)
		{
		case 0: // # 
		case 1: // Value 
		case 2: // count 
		case 4: // rep 
			SetColumnWidth (pHdr->iItem, nMax[pHdr->iItem] + nPad);
			break;
		default:
			break;
		}
	}
}
