/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : wnfixedw.cpp : implementation file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Ruler control that allow user to click on the ruler to divide
**               the columns:
**
** History:
**
** 29-Dec-2000 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

#include "stdafx.h"
#include "wnfixedw.h"
#include "svriia.h"
#include "colorind.h" // UxCreateFont
//#define _HEADER

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
IMPLEMENT_DYNCREATE( CuWndFixedWidth, CWnd )

static int StretchWidth(int nWidth, int m_nMeasure)
{
	return 0; //((m_nWidth/m_nMeasure)+1)*m_nMeasure;
}


CuWndFixedWidth::CuWndFixedWidth()
{
	UxCreateFont (m_font, _T("Courier"), 10);
	m_nCurrentHScrollPos = 0;
	m_nCurrentVScrollPos = 0;
	m_bReadOnly = FALSE;
	m_sizeChar.cx = 9;
	m_pArrayLine = NULL;
	m_nItemsPerPage = 0;
	m_nMaxLen = 0;
	m_nMinLen = 0;
	m_bTrackPos = FALSE;
	m_scrollInfo.cbSize = sizeof (m_scrollInfo);
	m_pointInfo = CPoint(0,0);
	m_bDrawInfo = FALSE;

	//
	// Drag & Drop Divider:
	m_bDragging = FALSE;
	m_pHeaderMarker = NULL;
	m_pXorPen = new CPen (PS_SOLID|PS_INSIDEFRAME, 1, RGB (0xC080, 0xC080, 0xC080));
	m_strMsgScrollTo.LoadString (IDS_SCROLL_TOLINE);

}

CuWndFixedWidth::~CuWndFixedWidth()
{
	if (m_pXorPen)
		delete m_pXorPen;
}

void CuWndFixedWidth::Cleanup()
{
	CuStaticRuler* pRuler =(CuStaticRuler*)&m_ruler;
	if (!pRuler)
		return;
	pRuler->Cleanup();
	CuListBoxFixedWidth& listBox = GetListBox();
	listBox.ResetContent();
	ResizeControls();
}


BEGIN_MESSAGE_MAP(CuWndFixedWidth, CWnd)
	//{{AFX_MSG_MAP(CuWndFixedWidth)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


int CuWndFixedWidth::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	CClientDC dc (this);
	CFont* pOldFont = dc.SelectObject (&m_font);
	m_sizeChar = dc.GetTextExtent (_T("x"), 1);

	CRect r (lpCreateStruct->x, lpCreateStruct->y, lpCreateStruct->x+ lpCreateStruct->cx, lpCreateStruct->y + lpCreateStruct->cy);
	if (m_bReadOnly)
		m_ruler.SetBodyColor (RGB(192, 192, 192));

	m_ruler.Create    (NULL, WS_CHILD, r, this, 1001);
#ifdef _HEADER
	m_header.Create   (WS_CHILD|WS_VISIBLE|HDS_DRAGDROP|HDS_FULLDRAG |HDS_HOTTRACK |CCS_NOPARENTALIGN |CCS_NODIVIDER/*|HDS_BUTTONS*/|HDS_HORZ, r, this, 1003);
#endif
	m_listbox.Create  (WS_CHILD|LBS_NOINTEGRALHEIGHT|WS_VSCROLL, r, this, 1004);
	
	m_horzScrollBar.Create(WS_CHILD | WS_VISIBLE | /*WS_DISABLED |*/ SBS_HORZ | SBS_TOPALIGN, r, this, 1005);
	m_horzScrollBar.SetWindowPos (&wndTop, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);

	m_vertScrollBar.Create(WS_CHILD | WS_VISIBLE | /*WS_DISABLED |*/ SBS_VERT | SBS_RIGHTALIGN, r, this, 1006);
	m_vertScrollBar.SetWindowPos (&wndTop, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
	
	
	m_ruler.SetFont(&m_font);

#ifdef _HEADER
	m_header.SetFont(&m_font);
#endif
	m_listbox.SetFont(&m_font);
	m_listbox.SetRuler (&m_ruler);

#ifdef _HEADER
	m_header.EnableWindow (FALSE);
#endif

	m_horzScrollBar.SetScrollPos (0);
	m_vertScrollBar.SetScrollPos (0);


	return 0;
}

void CuWndFixedWidth::ResizeControls()
{
	CRect rTemp, r;
	GetClientRect (r);
	rTemp = r;
	if (!IsWindow(m_listbox.m_hWnd))
		return;

	//
	// Resize the ruler:
	rTemp.bottom = r.top + 30;
	m_ruler.MoveWindow (rTemp);
#ifdef _HEADER
	rTemp.top    =  rTemp.bottom +1;
	rTemp.bottom =  rTemp.top + 18;
	m_header.MoveWindow (rTemp);
#endif
	//
	// Resize the listBox:
	rTemp.left   = r.left;
	rTemp.top    = rTemp.bottom + 1;
	rTemp.bottom = r.bottom - GetSystemMetrics(SM_CYHSCROLL);
	rTemp.right  = r.right  - GetSystemMetrics(SM_CXVSCROLL);
	m_listbox.MoveWindow (rTemp);

	//
	// Resize the vertical scrollbar:
	rTemp.right  = r.right;
	rTemp.left   = r.right  - GetSystemMetrics(SM_CXVSCROLL);
	m_vertScrollBar.MoveWindow (rTemp);

	//
	// Resize the horizontal scrollbar:
	rTemp.top    = rTemp.bottom+1;
	rTemp.left   = r.left;
	rTemp.bottom = r.bottom;
	m_horzScrollBar.MoveWindow (rTemp);

	m_ruler.ShowWindow(SW_SHOW);

#ifdef _HEADER
	m_header.ShowWindow(SW_SHOW);
#endif
	m_listbox.ShowWindow(SW_SHOW);
	m_horzScrollBar.ShowWindow(SW_SHOW);
}



void CuWndFixedWidth::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
	ResizeControls();

	CRect r, rClient;
	m_listbox.GetClientRect (rClient);
	if (m_listbox.GetCount() == 0)
	{
		m_listbox.AddString (_T(""));
		m_listbox.GetItemRect (0, r);
		m_listbox.ResetContent();
	}
	else
		m_listbox.GetItemRect (0, r);

	m_nItemsPerPage = rClient.Height()/r.Height();
	m_listbox.GetWindowRect (m_rectListBox);
	ScreenToClient(m_rectListBox);
}

void CuWndFixedWidth::AddDivider (CPoint p)
{
	p.x = m_sizeChar.cx*p.x;
	CuStaticRuler* pRuler =(CuStaticRuler*)&m_ruler;
	if (!pRuler)
		return;
	pRuler->AddHeaderMarker (p);
}

int CuWndFixedWidth::GetColumnDividerCount()
{
	CuStaticRuler* pRuler =(CuStaticRuler*)&m_ruler;
	if (!pRuler)
		return 0;
	int nCount = pRuler->GetCount();
	return nCount;
}

inline static int DividerPosCompare (const void* e1, const void* e2)
{
	int o1 = *((UINT*)e1);
	int o2 = *((UINT*)e2);
	return (o1 > o2)? 1: (o1 < o2)? -1: 0;
}

void CuWndFixedWidth::GetColumnPositions(int*& pArrayPosition, int& nSize)
{
	pArrayPosition = NULL;

	nSize = GetColumnDividerCount();
	if (nSize == 0)
		return;

	CuStaticRuler* pRuler =(CuStaticRuler*)&m_ruler;
	CArray< CPoint,CPoint > pointArray;
	int nOffsetOrigin = pRuler->GetPointArray (pointArray);
	ASSERT (nSize == pointArray.GetSize());
	if (nSize != pointArray.GetSize())
		return;
	pArrayPosition = new int [nSize];

	for (int i=0; i<nSize; i++)
	{
		pArrayPosition[i] = pointArray.GetAt(i).x/m_sizeChar.cx;
	}

	qsort((void*)pArrayPosition, (size_t)nSize, (size_t)sizeof(int), DividerPosCompare);
}

void CuWndFixedWidth::OnLButtonDown(UINT nFlags, CPoint point) 
{
	if (m_bReadOnly)
		return;

	CWnd::OnLButtonDown(nFlags, point);
	CuStaticRuler* pRuler =(CuStaticRuler*)&m_ruler;
	ASSERT(pRuler);
	if (!pRuler)
		return;

	CClientDC dc(this);
	dc.DPtoLP(&point);
	int nScrollPos = m_horzScrollBar.GetScrollPos ();
	point.x += nScrollPos;
	if ((point.x / m_sizeChar.cx) == 0)
		return; // Require at least one character to put a divider

	if (!m_horzScrollBar.IsWindowEnabled())
	{
		//
		// Click on the horizontal scrollbar:
		CRect r;
		m_horzScrollBar.GetWindowRect(r);
		ScreenToClient(r);
		if (point.y > r.top && point.y < r.bottom)
			return;
	}

	if ((point.x / m_sizeChar.cx) < m_nMaxLen)
	{
		CPoint pt = pRuler->AddHeaderMarker (point);
		//if ((pt.x / m_sizeChar.cx) >= m_nMinLen)
		//	pRuler->RemoveHeaderMarker(pt);
		m_listbox.Invalidate();
	}
	
	m_pMove1 = CPoint (0,0);
	m_pMove2 = CPoint (0,0);

	m_bDragging = TRUE;
	m_pHeaderMarker = pRuler->GetHeaderMarkerAt(point);
	SetCapture();
}

void CuWndFixedWidth::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	ReleaseCapture();
	m_pHeaderMarker = NULL;
	if (m_bReadOnly)
		return;

	CuStaticRuler* pRuler =(CuStaticRuler*)&m_ruler;
	ASSERT(pRuler);
	if (!pRuler)
		return;

	CClientDC dc(this);
	dc.DPtoLP(&point);
	int nScrollPos = m_horzScrollBar.GetScrollPos ();
	point.x += nScrollPos;

	if ((point.x / m_sizeChar.cx) == 0)
		return; // Require at least one character to put a divider
	if (!m_horzScrollBar.IsWindowEnabled())
	{
		//
		// Click on the horizontal scrollbar:
		CRect r;
		m_horzScrollBar.GetWindowRect(r);
		ScreenToClient(r);
		if (point.y > r.top && point.y < r.bottom)
			return;
	}

	pRuler->RemoveHeaderMarker(point);
	pRuler->Invalidate();

	m_listbox.Invalidate();
	CWnd::OnLButtonDblClk(nFlags, point);
}

void CuWndFixedWidth::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	if(pScrollBar != &m_horzScrollBar)
	{
		CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
		return;
	}

	int nPageWidth = 200;
	int nCurPos = m_horzScrollBar.GetScrollPos();
	int nPrevPos = nCurPos;
	switch(nSBCode)
	{
	case SB_LEFT:
		m_nCurrentHScrollPos = 0;
		break;
	case SB_RIGHT:
		m_nCurrentHScrollPos = m_horzScrollBar.GetScrollLimit()-1;
		break;
	case SB_LINELEFT:
		m_nCurrentHScrollPos = max(m_nCurrentHScrollPos-m_sizeChar.cx, 0);
		break;
	case SB_LINERIGHT:
		m_nCurrentHScrollPos = min(m_nCurrentHScrollPos+m_sizeChar.cx, m_horzScrollBar.GetScrollLimit()-1);
		break;
	case SB_PAGELEFT:
		m_nCurrentHScrollPos = max(m_nCurrentHScrollPos-nPageWidth, 0);
		break;
	case SB_PAGERIGHT:
		m_nCurrentHScrollPos = min(m_nCurrentHScrollPos + nPageWidth, m_horzScrollBar.GetScrollLimit()-1);
		break;
	
	case SB_THUMBTRACK:
	case SB_THUMBPOSITION:
		if(nPos==0)
			m_nCurrentHScrollPos = 0;
		else
			m_nCurrentHScrollPos = nPos; //min(StretchWidth(nPos, m_sizeChar.cx), m_horzScrollBar.GetScrollLimit()-1);

		break;
	}
	// 6 is Microsoft's step in a CListCtrl for example
	CWnd::OnHScroll(nSBCode, m_nCurrentHScrollPos, pScrollBar);

	m_horzScrollBar.SetScrollPos(m_nCurrentHScrollPos);
	//m_tree.m_nOffset = -m_nCurPos;

	// smoothly scroll the tree control
	{
//		CRect m_scrollRect;
/*
		m_ruler.ScrollWindow(nPrevPos - m_nCurrentHScrollPos, 0);
#ifdef _HEADER
		m_header.ScrollWindow(nPrevPos - m_nCurrentHScrollPos, 0);
#endif
		m_listbox.ScrollWindow(nPrevPos - m_nCurrentHScrollPos, 0);
*/
	}

	CRect r;
	GetClientRect (r);
	int nWewWidtht = r.Width();
	m_ruler.GetWindowRect (r);
	ScreenToClient (r);
	m_ruler.SetWindowPos(&wndTop, - m_nCurrentHScrollPos, 0, nWewWidtht + m_nCurrentHScrollPos, r.Height(), SWP_SHOWWINDOW);
#ifdef _HEADER
	m_header.GetWindowRect (r);
	ScreenToClient (r);
	m_header.SetWindowPos(&wndTop, - m_nCurrentHScrollPos, r.top, nWewWidtht + m_nCurrentHScrollPos, r.Height(), SWP_SHOWWINDOW);
#endif
	m_listbox.GetWindowRect (r);
	ScreenToClient (r);
	int nLbWidth = nWewWidtht - GetSystemMetrics(SM_CXVSCROLL);
	m_listbox.SetWindowPos(&wndTop, - m_nCurrentHScrollPos, r.top, nLbWidth + m_nCurrentHScrollPos, r.Height(), SWP_SHOWWINDOW);

	m_ruler.Invalidate();
}


void CuWndFixedWidth::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	if(pScrollBar != &m_vertScrollBar)
	{
		CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
		return;
	}

	int nPageWidth = (m_nItemsPerPage == 0)? 10: m_nItemsPerPage;
	int nCurPos = m_vertScrollBar.GetScrollPos();
	int nPrevPos = nCurPos;
	switch(nSBCode)
	{
	case SB_TOP:
		m_nCurrentVScrollPos = 0;
		break;
	case SB_BOTTOM:
		m_nCurrentVScrollPos = m_vertScrollBar.GetScrollLimit()-1;
		break;
	case SB_LINEUP:
		if (m_nCurrentVScrollPos == 0)
			break;
		m_nCurrentVScrollPos--;
		m_vertScrollBar.SetScrollPos(m_nCurrentVScrollPos);
		ScrollLineUp();
		break;
	case SB_LINEDOWN:
		if (m_nCurrentVScrollPos == (m_vertScrollBar.GetScrollLimit()-1))
			break;
		m_nCurrentVScrollPos++;
		m_vertScrollBar.SetScrollPos(m_nCurrentVScrollPos);
		ScrollLineDown();
		break;
	case SB_PAGEDOWN:
		if (m_nCurrentVScrollPos == (m_vertScrollBar.GetScrollLimit()-1))
			return;
		m_nCurrentVScrollPos += m_nItemsPerPage;
		if (m_nCurrentVScrollPos >= m_vertScrollBar.GetScrollLimit())
			m_nCurrentVScrollPos = m_vertScrollBar.GetScrollLimit()-1;
		m_vertScrollBar.SetScrollPos(m_nCurrentVScrollPos);
		ScrollPageDown();
		break;
	case SB_PAGEUP:
		if (m_nCurrentVScrollPos == 0)
			return;
		m_nCurrentVScrollPos -= m_nItemsPerPage;
		if (m_nCurrentVScrollPos < 0)
			m_nCurrentVScrollPos = 0;
		m_vertScrollBar.SetScrollPos(m_nCurrentVScrollPos);
		ScrollPageUp();
		break;
	case SB_THUMBTRACK:
		m_vertScrollBar.GetScrollInfo(&m_scrollInfo);
		m_nCurrentVScrollPos = m_scrollInfo.nTrackPos;
		m_bTrackPos = TRUE;
		DrawThumbTrackInfo(m_nCurrentVScrollPos);
		break;
	case SB_THUMBPOSITION:
		m_vertScrollBar.GetScrollInfo(&m_scrollInfo);
		m_nCurrentVScrollPos = m_scrollInfo.nTrackPos;
		m_bTrackPos = TRUE;
		break;
	case SB_ENDSCROLL:
		m_vertScrollBar.SetScrollPos(m_nCurrentVScrollPos);
		if (m_bTrackPos)
		{
			DrawThumbTrackInfo(m_nCurrentVScrollPos, TRUE); // Erase
			Display(m_nCurrentVScrollPos);
		}
		m_bTrackPos = FALSE;
		break;
	}
	m_vertScrollBar.SetScrollPos(m_nCurrentVScrollPos);
}

void CuWndFixedWidth::ScrollLineUp()
{
	Display(m_nCurrentVScrollPos);
}

void CuWndFixedWidth::ScrollLineDown()
{
	Display(m_nCurrentVScrollPos);
}

void CuWndFixedWidth::ScrollPageUp()
{
	Display(m_nCurrentVScrollPos);
}

void CuWndFixedWidth::ScrollPageDown()
{
	Display(m_nCurrentVScrollPos);
}


void CuWndFixedWidth::Display(int nPos)
{
	if (!m_pArrayLine)
		return;
	int i, nSize = m_pArrayLine->GetSize();
	m_vertScrollBar.SetScrollRange (0, nSize-m_nStartIndex-m_nItemsPerPage);
	m_listbox.ResetContent();

	nPos = nPos + m_nStartIndex;
	i=0;
	while (i<m_nItemsPerPage && nPos <nSize)
	{
		CString strLine = m_pArrayLine->GetAt (nPos);
		m_listbox.AddString(strLine);

		nPos++;
		i++;
	}
}

void CuWndFixedWidth::SetData(CStringArray& arrayLine, int nStartIndex)
{
	//
	// Must call the create mumber first:
	ASSERT (IsWindow(m_vertScrollBar.m_hWnd) && IsWindow(m_listbox.m_hWnd));
	if (!(IsWindow(m_vertScrollBar.m_hWnd) || IsWindow(m_listbox.m_hWnd)))
		return;

	m_nStartIndex = nStartIndex;
	m_pArrayLine = &arrayLine;

	m_nMaxLen = 0;
	m_nMinLen = INT_MAX;
	int i, nSize = m_pArrayLine->GetSize();
	for (i=m_nStartIndex; i<nSize; i++)
	{
		CString strLine = m_pArrayLine->GetAt(i);
		int nLen = strLine.GetLength();
		//
		// Max String Length:
		if (nLen > m_nMaxLen)
			m_nMaxLen = nLen;

		//
		// Min String Length:
		if (nLen < m_nMinLen)
			m_nMinLen = nLen;
	}

	CRect r;
	m_listbox.GetClientRect (r);
	if ((m_nMaxLen*m_sizeChar.cx) <= r.Width())
	{
		GetHorzScrollBar().EnableWindow (FALSE);
	}
	else
	{
		GetHorzScrollBar().EnableWindow (TRUE);
	}
	m_horzScrollBar.SetScrollRange(0, (m_nMaxLen*m_sizeChar.cx) - r.Width() + m_sizeChar.cx);


	if ((nSize - m_nStartIndex) > m_nItemsPerPage)
	{
		m_vertScrollBar.EnableWindow (TRUE);
	}
	else
	{
		m_vertScrollBar.EnableWindow (FALSE);
	}
}

void CuWndFixedWidth::DrawThumbTrackInfo (int nPos, BOOL bErase)
{
	CClientDC dc (this);
	CFont* pFont = m_listbox.GetFont();
	CFont* pOldFond = dc.SelectObject (pFont);
	
	if (!m_bDrawInfo)
	{
		GetCursorPos(&m_pointInfo);
		ScreenToClient (&m_pointInfo);
		m_pointInfo.x -= 20;
	}

	int nSize = m_pArrayLine->GetSize();
	CString strSize;
	strSize.Format (m_strMsgScrollTo, nSize, nSize);

	CRect r;
	CString strMsg;
	strMsg.Format (m_strMsgScrollTo, nPos+1, m_pArrayLine->GetSize());
	CSize txSize = dc.GetTextExtent ((LPCTSTR)strSize);
	
	r.top    = m_pointInfo.y;
	r.left   = m_pointInfo.x - txSize.cx - 10;
	r.right  = m_pointInfo.x;
	r.bottom = r.top + txSize.cy + 4;

	COLORREF oldTextBkColor = dc.SetBkColor (RGB(244, 245, 200));
	CBrush br (RGB(244, 245, 200));
	dc.FillRect(r, &br);
	if (bErase)
	{
		Invalidate();
		m_bDrawInfo = FALSE;
	}
	else
	{
		dc.DrawText (strMsg, r, DT_CENTER|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER|DT_NOCLIP);
		m_bDrawInfo = TRUE;
	}
	dc.SetBkColor (oldTextBkColor);
	dc.SelectObject (pOldFond);
}

void CuWndFixedWidth::OnMouseMove(UINT nFlags, CPoint point) 
{
	CWnd::OnMouseMove(nFlags, point);
	if (m_bReadOnly)
		return;

	CuStaticRuler* pRuler =(CuStaticRuler*)&m_ruler;
	ASSERT(pRuler);
	if (!pRuler)
		return;

	CClientDC dc(this);
	dc.DPtoLP(&point);
	int nScrollPos = m_horzScrollBar.GetScrollPos ();
	point.x += nScrollPos;
	if (!m_pHeaderMarker)
	{
		if (pRuler->ExistHeaderMarker (point))
		{
			::SetCursor (theApp.LoadCursor(AFX_IDC_MOVE4WAY));
		}
		else
			::SetCursor (::LoadCursor(NULL, IDC_ARROW));
	}
	else
	if (m_bDragging && m_pHeaderMarker)
	{
		::SetCursor (theApp.LoadCursor(AFX_IDC_MOVE4WAY));
		CPoint pNewPoin = pRuler->ChangePoint(m_pHeaderMarker, point);
		CPen* pOldPen = dc.SelectObject (m_pXorPen);
		int nOldROP2  = dc.SetROP2 (R2_XORPEN);
	
		if (m_pMove1.x != 0 && m_pMove2.x != 0 &&  m_pMove1.y != 0 && m_pMove2.y != 0)
		{
			dc.MoveTo (m_pMove1.x, m_pMove1.y);
			dc.LineTo (m_pMove2.x, m_pMove2.y);
		}

		pRuler->Invalidate();

		m_pMove1 = pNewPoin;
		m_pMove2 = pNewPoin;
		m_pMove1.x -= nScrollPos;
		m_pMove2.x -= nScrollPos;

		m_pMove1.y = m_rectListBox.top;
		m_pMove2.y = m_rectListBox.bottom;
		dc.MoveTo (m_pMove1.x, m_pMove1.y);
		dc.LineTo (m_pMove2.x, m_pMove2.y);

		dc.SelectObject (pOldPen);
		dc.SetROP2 (nOldROP2);
	}
	else
		::SetCursor (::LoadCursor(NULL, IDC_ARROW));
}


void CuWndFixedWidth::OnLButtonUp(UINT nFlags, CPoint point) 
{
	CWnd::OnLButtonUp(nFlags, point);
	ReleaseCapture();

	CuStaticRuler* pRuler =(CuStaticRuler*)&m_ruler;
	ASSERT(pRuler);
	if (pRuler && m_pHeaderMarker && m_bDragging)
	{
		CRect r;
		GetClientRect(r);
		if (point.x < r.left || point.x > r.right )
		{
			CPoint pt = m_pHeaderMarker->GetPoint();
			pRuler->RemoveHeaderMarker(pt);
		}
		else
		{
			CClientDC dc(this);
			dc.DPtoLP(&point);
			int nScrollPos = m_horzScrollBar.GetScrollPos ();
			point.x += nScrollPos;
			if ((point.x / m_sizeChar.cx) == 0)
			{
				//
				// Require at least one character to put a divider
				CPoint pt = m_pHeaderMarker->GetPoint();
				pRuler->RemoveHeaderMarker(pt);
			}
		}
	}

	m_pHeaderMarker = NULL;
	m_bDragging = FALSE;
	m_pMove1 = CPoint (0,0);
	m_pMove2 = CPoint (0,0);
	m_listbox.Invalidate();
	m_vertScrollBar.Invalidate();
}
