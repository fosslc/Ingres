/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : lmsgstat.cpp, Implementation file
**    Project  : Visual Manager.
**    Author   : UK Sotheavut
**    Purpose  : Owner draw LISTCTRL that can draw 3D horizontal bar in the column
**
**  History:
**
** 07-Jul-1999 (uk$so01)
**    created
** 12-Fev-2001 (noifr01)
**    bug 102974 (manage multiline errlog.log messages)
** 14-Aug-2001 (uk$so01)
**    SIR #105384 (Adjust the header width)
** 27-Feb-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
** 06-Mar-2003 (uk$so01)
**    SIR #109773, Add property frame for displaying the description 
**    of ingres messages.
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "lmsgstat.h"
#include "ivmdml.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const int R_FACE_PROJECT_WIDTH = 5;
static const int U_FACE_PROJECT_WIDTH = 5;

CuListCtrlMessageStatistic::CuListCtrlMessageStatistic():CuListCtrl()
{
	SetLineSeperator(FALSE);
	m_nMaxCountGroup = 0;

	m_crFaceFront = m_colorHighLight;
	m_crFaceUpper = GetSysColor (COLOR_GRAYTEXT);
	m_crFaceRight = GetSysColor (COLOR_3DLIGHT);
	m_colorFaceSelected = RGB(0, 200, 0);
	m_strLongMessage = _T("");
	m_strLongCount = _T("");
	m_nMaxMessageWidth = 0;
	m_nMaxCountWidth = 0;
}

CuListCtrlMessageStatistic::~CuListCtrlMessageStatistic()
{
}

UINT CuListCtrlMessageStatistic::GetColumnJustify(int nCol)
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

BEGIN_MESSAGE_MAP(CuListCtrlMessageStatistic, CuListCtrl)
	//{{AFX_MSG_MAP(CuListCtrlMessageStatistic)
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
// CuListCtrlMessageStatistic message handlers


BOOL CuListCtrlMessageStatistic::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult) 
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


void CuListCtrlMessageStatistic::DrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CString  pszItem;
	CString  strItem;
	COLORREF colorTextOld;
	COLORREF colorBkOld;
	
	CDC* pDC = CDC::FromHandle (lpDrawItemStruct->hDC);
	if ((int)lpDrawItemStruct->itemID < 0)
		return;
	int  nItem     = lpDrawItemStruct->itemID;
	BOOL bFocus    = GetFocus() == this;
	BOOL bSelected = (bFocus || (GetStyle() & LVS_SHOWSELALWAYS)) && (GetItemState(nItem, LVIS_SELECTED) & LVIS_SELECTED);
	CaMessageStatisticItemData* pItemData = (CaMessageStatisticItemData*)lpDrawItemStruct->itemData;
	//
	// Must have item data:
	ASSERT (pItemData);
	if (!pItemData)
		return;

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
	if (bSelected)
	{
		colorTextOld = pDC->SetTextColor (m_colorHighLightText);
		colorBkOld   = pDC->SetBkColor   (m_colorHighLight);
		pDC->FillRect (r, &brushSelect);
		m_crFaceFront = m_colorFaceSelected;
	}
	else
	{
		m_crFaceFront = m_colorHighLight;
		colorTextOld = pDC->SetTextColor (m_colorText);
		pDC->FillRect (r, &brushNormal);
	}
	pDC->FillRect (r, &brushNormal);

	//
	// Caculate the header width in pixel for ajusting header when user double clicks on the column divider: 
	if (m_nMaxMessageWidth == 0)
	{
		CSize s;
		GetTextExtentPoint32(pDC->m_hDC, m_strLongMessage, m_strLongMessage.GetLength(), &s);
		m_nMaxMessageWidth = s.cx;
	}
	if (m_nMaxCountWidth == 0)
	{
		CSize s;
		GetTextExtentPoint32(pDC->m_hDC, m_strLongCount, m_strLongCount.GetLength(), &s);
		m_nMaxCountWidth = s.cx;
	}

	if (bSelected)
	{
		colorTextOld = pDC->SetTextColor (m_colorHighLightText);
		colorBkOld   = pDC->SetBkColor   (m_colorHighLight);
		pDC->FillRect (r, &brushSelect);
	}

	//
	// Draw the Main Item "Message":
	GetItemRect (nItem, rcItem, LVIR_LABEL);
	rcLabel = rcItem;
	rcLabel.left  += OFFSET_FIRST;
	rcLabel.right -= OFFSET_FIRST;
	strItem = IVM_IngresGenericMessage (pItemData->GetCode(), pItemData->GetTitle());
	pszItem = MakeShortString (pDC, strItem, rcItem.right - rcItem.left, 2*OFFSET_OTHER);
	UINT nJustify = GetColumnJustify (STATITEM_MESSAGE);
	pDC->DrawText (pszItem, rcLabel, nJustify|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);

	CPen  penGray (PS_SOLID, 1, m_colorLine);
	CPen* pOldPen = pDC->SelectObject (&penGray);
	if (m_bLineSeperator)
	{
		pDC->MoveTo (rcItem.right-2, rcItem.top);
		pDC->LineTo (rcItem.right-2, rcItem.bottom);
	}

	//
	// Draw the item: Count:
	nJustify = GetColumnJustify (STATITEM_COUNT);
	rcItem.left   = rcItem.right;
	rcItem.right += GetColumnWidth(STATITEM_COUNT);
	if (pItemData->GetCountExtra()==0)
		strItem.Format (_T("%d"), pItemData->GetCount());
	else
		strItem.Format (_T("%d(+%d)"), pItemData->GetCount(), pItemData->GetCountExtra());
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

	if (m_bLineSeperator)
	{
		pDC->MoveTo (rcItem.right-1, rcItem.top);
		pDC->LineTo (rcItem.right-1, rcItem.bottom);
	}
	//
	// Draw the Graph: 
	nJustify = GetColumnJustify (STATITEM_GRAPH);
	rcItem.left   = rcItem.right;
	rcItem.right += GetColumnWidth(STATITEM_GRAPH);
	if (GetColumnWidth(2) > 10)
		DrawBar(pDC, rcItem, pItemData);
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

void CuListCtrlMessageStatistic::DrawBar(CDC* pDC, CRect r, CaMessageStatisticItemData* pData)
{
	CBrush* pOldBrush = NULL;
	CPen*   pOldPen   = NULL;
	CPoint p [4];
	double dWidth = (double)GetColumnWidth(STATITEM_GRAPH) - (double)U_FACE_PROJECT_WIDTH - 2.0;
	double dp = (double)pData->GetCount() / (double)m_nMaxCountGroup;
	dWidth = dWidth*dp;
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


void CuListCtrlMessageStatistic::OnDividerdblclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMHEADER pHdr = (LPNMHEADER)pNMHDR;
	*pResult = 0;
	if (pHdr->hdr.idFrom == 0 && (pHdr->hdr.code == HDN_DIVIDERDBLCLICKA || pHdr->hdr.code == HDN_DIVIDERDBLCLICKW))
	{

		switch (pHdr->iItem)
		{
		case 0: // Message Text
			if (m_nMaxMessageWidth > 0)
				SetColumnWidth (pHdr->iItem, m_nMaxMessageWidth +40);
			break;
		case 1: // count 
			if (m_nMaxCountWidth > 0)
				SetColumnWidth (pHdr->iItem, m_nMaxCountWidth +40);
			break;
		case 2: // graph (auto adjust)
			break;
		default:
			break;
		}
	}
}
