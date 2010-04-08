/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vwdtdiff.cpp : implementation file
**    Project  : INGRESII / Visual Schema Diff Control (vsda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : View for detail difference page
**
** History:
**
** 10-Dec-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
** 17-May-2004 (uk$so01)
**    SIR #109220, ISSUE 13407277: additional change to put the header
**    in the pop-up for an item's detail of difference.
*/

#include "stdafx.h"
#include "vsda.h"
#include "vwdtdiff.h"
#include ".\vwdtdiff.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CvDifferenceDetail, CView)

CvDifferenceDetail::CvDifferenceDetail()
{
	m_pDetailPage = NULL;
}

CvDifferenceDetail::~CvDifferenceDetail()
{
}


BEGIN_MESSAGE_MAP(CvDifferenceDetail, CView)
	//{{AFX_MSG_MAP(CvDifferenceDetail)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvDifferenceDetail drawing

void CvDifferenceDetail::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CvDifferenceDetail diagnostics

#ifdef _DEBUG
void CvDifferenceDetail::AssertValid() const
{
	CView::AssertValid();
}

void CvDifferenceDetail::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvDifferenceDetail message handlers

BOOL CvDifferenceDetail::PreCreateWindow(CREATESTRUCT& cs) 
{
	return CView::PreCreateWindow(cs);
}

void CvDifferenceDetail::OnInitialUpdate() 
{
	CView::OnInitialUpdate();
}

int CvDifferenceDetail::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	m_pDetailPage = new CuDlgDifferenceDetailPage();
	m_pDetailPage->Create(IDD_DIFFERENCE_DETAIL_PAGE, this);
	m_pDetailPage->ShowWindow(SW_SHOW);
	return 0;
}

void CvDifferenceDetail::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	if (m_pDetailPage && IsWindow(m_pDetailPage->m_hWnd))
	{
		CRect r;
		GetClientRect (r);
		m_pDetailPage->MoveWindow(r);
	}
}
