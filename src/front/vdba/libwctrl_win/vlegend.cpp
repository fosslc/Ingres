/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vlegend.cpp : implementation file
**    Project  : VDBA
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Legend of pie/chart control
**
** History:
**
** 03-Dec-2001 (uk$so01)
**    Created
*/

#include "stdafx.h"
#include "vlegend.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CvLegend, CView)

CvLegend::CvLegend()
{
}

CvLegend::~CvLegend()
{
}


BEGIN_MESSAGE_MAP(CvLegend, CView)
	//{{AFX_MSG_MAP(CvLegend)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvLegend drawing

void CvLegend::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CvLegend diagnostics

#ifdef _DEBUG
void CvLegend::AssertValid() const
{
	CView::AssertValid();
}

void CvLegend::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvLegend message handlers

int CvLegend::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	CRect r;
	GetClientRect (r);
	UINT nStyle = WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_HSCROLL|LBS_OWNERDRAWFIXED|LBS_HASSTRINGS;
	m_legendListBox.Create (nStyle, r, this, -1);
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	if (hFont == NULL)
		hFont = (HFONT)GetStockObject(ANSI_VAR_FONT);
	m_legendListBox.SendMessage(WM_SETFONT, (WPARAM)hFont);
	return 0;
}

void CvLegend::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	if (IsWindow (m_legendListBox.m_hWnd))
	{
		CRect r;
		GetClientRect (r);
		m_legendListBox.MoveWindow (r);
	}
}
