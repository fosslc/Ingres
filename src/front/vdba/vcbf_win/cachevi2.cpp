/*
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : cachevi2.cpp, Implementation file
**
**
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut
**
**
**    Purpose  : Right pane of DBMS Cache page
**
**  History:
**	01-nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings / errors.
*/

#include "stdafx.h"
#include "vcbf.h"
#include "cachevi2.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CvDbmsCacheViewRight

IMPLEMENT_DYNCREATE(CvDbmsCacheViewRight, CView)

CvDbmsCacheViewRight::CvDbmsCacheViewRight()
{
	m_pDlgCacheTab = NULL;
}

CvDbmsCacheViewRight::~CvDbmsCacheViewRight()
{
}


BEGIN_MESSAGE_MAP(CvDbmsCacheViewRight, CView)
	//{{AFX_MSG_MAP(CvDbmsCacheViewRight)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_UPDATING, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_VALIDATE, OnValidateData)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON1, OnButton1)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON2, OnButton2)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON3, OnButton3)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvDbmsCacheViewRight drawing

void CvDbmsCacheViewRight::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CvDbmsCacheViewRight diagnostics

#ifdef _DEBUG
void CvDbmsCacheViewRight::AssertValid() const
{
	CView::AssertValid();
}

void CvDbmsCacheViewRight::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvDbmsCacheViewRight message handlers

int CvDbmsCacheViewRight::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	m_pDlgCacheTab = new CuDlgCacheTab (this);
	m_pDlgCacheTab->Create (IDD_CACHE_TAB, this);
	return 0;
}

void CvDbmsCacheViewRight::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	if (m_pDlgCacheTab && IsWindow (m_pDlgCacheTab->m_hWnd))
	{
		CRect r;
		GetClientRect (r);
		m_pDlgCacheTab->MoveWindow (r);
		m_pDlgCacheTab->ShowWindow (SW_SHOW);
	}
}

LONG CvDbmsCacheViewRight::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CvDbmsCacheViewRight::OnUpdateData...\n");
	if (m_pDlgCacheTab)
		m_pDlgCacheTab->SendMessage (WMUSRMSG_CBF_PAGE_UPDATING, 0, 0);
	return 0L;
}

LONG CvDbmsCacheViewRight::OnValidateData (WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CvDbmsCacheViewRight::OnValidateData...\n");
	if (m_pDlgCacheTab)
		m_pDlgCacheTab->SendMessage (WMUSRMSG_CBF_PAGE_VALIDATE, 0, 0);
	return 0L;
}

LONG CvDbmsCacheViewRight::OnButton1 (UINT wParam, LONG lParam)
{
	TRACE0 ("CvDbmsCacheViewRight::OnButton1...\n");
	if (m_pDlgCacheTab)
		m_pDlgCacheTab->SendMessage (WM_CONFIGRIGHT_DLG_BUTTON1, 0, 0);
	return 0L;
}

LONG CvDbmsCacheViewRight::OnButton2 (UINT wParam, LONG lParam)
{
	TRACE0 ("CvDbmsCacheViewRight::OnButton2...\n");
	if (m_pDlgCacheTab)
		m_pDlgCacheTab->SendMessage (WM_CONFIGRIGHT_DLG_BUTTON2, 0, 0);
	return 0L;
}

LONG CvDbmsCacheViewRight::OnButton3 (UINT wParam, LONG lParam)
{
	TRACE0 ("CvDbmsCacheViewRight::OnButton3...\n");
	if (m_pDlgCacheTab)
		m_pDlgCacheTab->SendMessage (WM_CONFIGRIGHT_DLG_BUTTON3, 0, 0);
	return 0L;
}
