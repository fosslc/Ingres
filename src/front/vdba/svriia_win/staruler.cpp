/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : staruler.cpp : implementation file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Static Control that looks like a ruler
**
** History:
**
** 19-Dec-2000 (uk$so01)
**    Created
** 31-Oct-2001 (hanje04)
**    Replaced out of date 'CPoint& name=' constructs with CPoint name() as
**    they was causing problems on Linux.
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#include "stdafx.h"
#include "staruler.h"
#include "colorind.h" // UxCreateFont

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CuStaticRuler

CuStaticRuler::CuStaticRuler()
{
	m_nLeftMagin = 0;
	m_nOffsetOrigin = 0;
	m_rgbBody = RGB( 10, 192, 192);
	m_rgbGrid = RGB(255, 255, 255);
}

CuStaticRuler::~CuStaticRuler()
{
	while (!m_listHeader.IsEmpty())
		delete m_listHeader.RemoveHead();
}


BEGIN_MESSAGE_MAP(CuStaticRuler, CStatic)
	//{{AFX_MSG_MAP(CuStaticRuler)
	ON_WM_PAINT()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuStaticRuler message handlers

void CuStaticRuler::OnPaint() 
{
	CPaintDC dc(this);
	DrawRuler(&dc);

	CaHeaderMarker* pHeader = NULL;
	POSITION pos = m_listHeader.GetHeadPosition();
	while (pos != NULL)
	{
		pHeader = m_listHeader.GetNext (pos);
		DrawHeaderMarker(&dc, pHeader);
	}
}

int CuStaticRuler::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CStatic::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

int CuStaticRuler::GetPointArray (CArray< CPoint,CPoint >& pointArray)
{
	CaHeaderMarker* pHeader = NULL;
	POSITION pos = m_listHeader.GetHeadPosition();
	while (pos != NULL)
	{
		pHeader = m_listHeader.GetNext (pos);
		CPoint p = pHeader->GetPoint();
		pointArray.Add (p);
	}
	return (m_nOffsetOrigin + m_nLeftMagin);
}


void CuStaticRuler::OffsetOrigin(int nOffset)
{
	CPoint p (nOffset, 0);
	m_nOffsetOrigin = p.x;
	Invalidate();
}


void CuStaticRuler::DrawRuler(CDC* pDC)
{
	SIZE size;
	CRect r;
	CFont* pFont = GetFont();
	CFont* pOldFond= NULL;
	CPen* pOldP = NULL;
	CBrush* pOldBrush = NULL;
	int nBkOldMode = -1;
	GetClientRect (r);

	int nYpos = r.top + (int)((double)r.Height()/1.2);
	CBrush rb (m_rgbBody);
	pOldBrush = pDC->SelectObject (&rb);
	pDC->FillRect (r, &rb);
	pDC->SelectObject (pOldBrush);

	CPen p1(PS_SOLID, 1, m_rgbGrid);
	CPen p2(PS_SOLID, 2, m_rgbGrid);
	CPen p3(PS_SOLID, 3, m_rgbGrid);
	pOldP = pDC->SelectObject (&p1);
	pDC->MoveTo(r.left,  nYpos);
	pDC->LineTo(r.right, nYpos);

	pOldFond = pDC->SelectObject (pFont);
	GetTextExtentPoint32 (pDC->m_hDC, _T("8"), 1, &size);

	int nStep = 0, nCount = m_nOffsetOrigin/9;
	while (nStep < r.right)
	{
		pDC->MoveTo(nStep, nYpos);
		pDC->LineTo(nStep, nYpos - 3);

		if ((nCount%5) == 0)
		{
			pDC->SelectObject (&p2);
			pDC->MoveTo(nStep+m_nLeftMagin, nYpos);
			pDC->LineTo(nStep+m_nLeftMagin, nYpos - 3);
			pDC->SelectObject (&p1);
		}

		if ((nCount%10) == 0)
		{
			pDC->SelectObject (&p3);
			pDC->MoveTo(nStep+m_nLeftMagin, nYpos);
			pDC->LineTo(nStep+m_nLeftMagin, nYpos - 4);
			pDC->SelectObject (&p1);

			if (nCount > 0)
			{
				SIZE s;
				CString strText;
				strText.Format(_T("%d"), nCount);
				CRect rcInit = r;
				GetTextExtentPoint32 (pDC->m_hDC, strText, strText.GetLength(), &s);

				rcInit.left  = nStep - s.cx/2;
				rcInit.right = rcInit.left + s.cx;
				rcInit.bottom= nYpos - 6;
				rcInit.top   = rcInit.bottom - size.cy;

				nBkOldMode = pDC->SetBkMode(TRANSPARENT);
				pDC->DrawText(strText,-1, rcInit, DT_SINGLELINE|DT_CENTER);
				pDC->SetBkMode(nBkOldMode);
			}
		}

		nStep+= size.cx;
		nCount++;
	}

	pDC->SelectObject (pOldP);
	pDC->SelectObject (pOldFond);
}



void CuStaticRuler::DrawHeaderMarker(CDC* pDC, CaHeaderMarker* pMarker)
{
	CRect r, rx;
	CPoint p = pMarker->GetPoint();
	CPen* pOldP = NULL;
	CPen p1(PS_SOLID, 1, RGB(0, 0, 0));
	GetClientRect (r);
	int nYpos = r.top + (int)((double)r.Height()/1.5);
	pOldP = pDC->SelectObject (&p1);

	pDC->MoveTo(p.x+m_nLeftMagin  - m_nOffsetOrigin, nYpos +1);
	pDC->LineTo(p.x+m_nLeftMagin  - m_nOffsetOrigin, r.bottom-1);
	pDC->MoveTo(p.x+m_nLeftMagin+1- m_nOffsetOrigin, nYpos +1);
	pDC->LineTo(p.x+m_nLeftMagin+1- m_nOffsetOrigin, r.bottom-1);

}

void CuStaticRuler::AdjustPoint (CPoint& point)
{
	SIZE size;
	CClientDC dc(this);
	CFont* pFont = GetFont();
	CFont* pOldFond = dc.SelectObject (pFont);
	GetTextExtentPoint32 (dc.m_hDC, _T("8"), 1, &size);

	int nMid = size.cx / 2;
	int nRes = point.x % size.cx;
	point.x -= nRes;

	if (nRes > nMid)
		point.x += size.cx;
}

CPoint CuStaticRuler::ChangePoint(CaHeaderMarker* pHeader, CPoint newPoint)
{
	CPoint point = newPoint;
	AdjustPoint(point);
	pHeader->SetPoint (point);
	return point;
}



BOOL CuStaticRuler::ExistHeaderMarker(CPoint& point)
{
	CPoint p = point;
	AdjustPoint (p);
	CaHeaderMarker* pHeader = NULL;
	POSITION pos = m_listHeader.GetHeadPosition();
	while (pos != NULL)
	{
		pHeader = m_listHeader.GetNext (pos);
		CPoint c = pHeader->GetPoint();
		if (c.x == p.x)
			return TRUE;
	}

	return FALSE;
}


CaHeaderMarker* CuStaticRuler::GetHeaderMarkerAt(CPoint point)
{
	AdjustPoint(point);

	CaHeaderMarker* pHeader = NULL;
	POSITION pos = m_listHeader.GetHeadPosition();
	while (pos != NULL)
	{
		pHeader = m_listHeader.GetNext (pos);
		CPoint c(pHeader->GetPoint());
		if (c.x == point.x)
		{
			return pHeader;
		}
	}
	return NULL;
}


CPoint CuStaticRuler::AddHeaderMarker(CPoint p)
{
	CPoint point = p;
	if (!ExistHeaderMarker(point))
	{
		point = p;
		AdjustPoint(point);
		CaHeaderMarker* pHeader = new CaHeaderMarker(point);
		m_listHeader.AddTail (pHeader);

		CClientDC dc(this);
		DrawHeaderMarker(&dc, pHeader);
	}

	return point;
}

CPoint CuStaticRuler::RemoveHeaderMarker(CPoint p)
{
	CPoint point = p;
	if (ExistHeaderMarker(point))
	{
		point = p;
		AdjustPoint(point);

		CaHeaderMarker* pHeader = NULL;
		POSITION px, pos = m_listHeader.GetHeadPosition();
		while (pos != NULL)
		{
			px = pos;
			pHeader = m_listHeader.GetNext (pos);
			CPoint c = pHeader->GetPoint();
			if (c.x == point.x)
			{
				m_listHeader.RemoveAt (px);
				delete pHeader;
				Invalidate();
				return c;
			}
		}
	}
	Invalidate();
	return point;
}


