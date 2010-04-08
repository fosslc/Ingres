/*
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : cachevi1.cpp, Implementation file
**
**
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut
**
**
**    Purpose  : Left pane of DBMS Cache page
**
**  History:
**	01-nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings / errors.
*/

#include "stdafx.h"
#include "vcbf.h"
#include "cachevi1.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CvDbmsCacheViewLeft

IMPLEMENT_DYNCREATE(CvDbmsCacheViewLeft, CView)

CvDbmsCacheViewLeft::CvDbmsCacheViewLeft()
{
	m_pDlgCacheList = NULL;
}

CvDbmsCacheViewLeft::~CvDbmsCacheViewLeft()
{
}


BEGIN_MESSAGE_MAP(CvDbmsCacheViewLeft, CView)
	//{{AFX_MSG_MAP(CvDbmsCacheViewLeft)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_UPDATING, OnUpdateData)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON1, OnButton1)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON2, OnButton2)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON3, OnButton3)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvDbmsCacheViewLeft drawing

void CvDbmsCacheViewLeft::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CvDbmsCacheViewLeft diagnostics

#ifdef _DEBUG
void CvDbmsCacheViewLeft::AssertValid() const
{
	CView::AssertValid();
}

void CvDbmsCacheViewLeft::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvDbmsCacheViewLeft message handlers

int CvDbmsCacheViewLeft::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	m_pDlgCacheList = new CuDlgCacheList (this);
	m_pDlgCacheList->Create (IDD_CACHE_LIST, this);
	return 0;
}

void CvDbmsCacheViewLeft::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	if (m_pDlgCacheList && IsWindow (m_pDlgCacheList->m_hWnd))
	{
		CRect r;
		GetClientRect (r);
		m_pDlgCacheList->MoveWindow (r);
		m_pDlgCacheList->ShowWindow (SW_SHOW);
	}
}

LONG CvDbmsCacheViewLeft::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CvDbmsCacheViewLeft::OnUpdateData...\n");
	if (m_pDlgCacheList)
		m_pDlgCacheList->SendMessage (WMUSRMSG_CBF_PAGE_UPDATING, 0, 0);
	return 0L;
}

LONG CvDbmsCacheViewLeft::OnButton1 (UINT wParam, LONG lParam)
{
	return 0L;
}

LONG CvDbmsCacheViewLeft::OnButton2 (UINT wParam, LONG lParam)
{
	return 0L;
}

LONG CvDbmsCacheViewLeft::OnButton3 (UINT wParam, LONG lParam)
{
	return 0L;
}
