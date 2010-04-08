/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : viewrigh.cpp : implementation file
**    Project  : INGRESII / Visual Schema Diff Control (vsda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : The right pane for the Vsda Control Window.
**               It is responsible for displaying the Modeless Dialog of the Tree Item.
**
** History:
**
** 26-Aug-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
*/
#include "stdafx.h"
#include "vsda.h"
#include "viewrigh.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CvSdaRight, CView)

CvSdaRight::CvSdaRight()
{
	m_pPageList = NULL;
}

CvSdaRight::~CvSdaRight()
{
}


BEGIN_MESSAGE_MAP(CvSdaRight, CView)
	//{{AFX_MSG_MAP(CvSdaRight)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_UPDATEDATA, OnUpdateData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvSdaRight drawing

void CvSdaRight::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CvSdaRight diagnostics

#ifdef _DEBUG
void CvSdaRight::AssertValid() const
{
	CView::AssertValid();
}

void CvSdaRight::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvSdaRight message handlers

int CvSdaRight::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	m_pPageList = new CuDlgPageDifferentList();
	m_pPageList->Create(IDD_DIFFERENT_LIST, this);
	m_pPageList->ShowWindow(SW_SHOW);
	
	return 0;
}

void CvSdaRight::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	if (m_pPageList && IsWindow (m_pPageList->m_hWnd))
	{
		CRect r;
		GetClientRect (r);
		m_pPageList->MoveWindow(r);
	}
}

BOOL CvSdaRight::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.style |= WS_CLIPCHILDREN;
	
	return CView::PreCreateWindow(cs);
}

long CvSdaRight::OnUpdateData(WPARAM wParam, LPARAM lParam)
{
	if (m_pPageList && IsWindow (m_pPageList->m_hWnd))
	{
		m_pPageList->SendMessage (WMUSRMSG_UPDATEDATA, wParam, lParam);
	}
	return 0;
}