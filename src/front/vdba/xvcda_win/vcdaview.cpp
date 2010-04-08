/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vcdaview.cpp : implementation of the CvCda class
**    Project  : VCDA (Container)
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : frame/view/doc architecture (view)
**
** History:
**
** 01-Oct-2002 (uk$so01)
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
**    Created
** 23-Sep-2004 (uk$so01)
**    BUG #113114 
**    Force the SendMessageToDescendants(WM_SYSCOLORCHANGE) to reflect 
**    immediately the system color changed.
**/

#include "stdafx.h"
#include "vcda.h"
#include "vcdadoc.h"
#include "vcdaview.h"
#include ".\vcdaview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CvCda

IMPLEMENT_DYNCREATE(CvCda, CView)

BEGIN_MESSAGE_MAP(CvCda, CView)
	//{{AFX_MSG_MAP(CvCda)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvCda construction/destruction

CvCda::CvCda()
{
	m_pDlgMain = NULL;
}

CvCda::~CvCda()
{
}

BOOL CvCda::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= WS_CLIPCHILDREN;
	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CvCda drawing

void CvCda::OnDraw(CDC* pDC)
{
	CdCda* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CString strTitle; 
	strTitle.LoadString (AFX_IDS_APP_TITLE);
	strTitle += INGRESII_QueryInstallationID();

	AfxGetMainWnd()->SetWindowText((LPCTSTR)strTitle);
}

/////////////////////////////////////////////////////////////////////////////
// CvCda diagnostics

#ifdef _DEBUG
void CvCda::AssertValid() const
{
	CView::AssertValid();
}

void CvCda::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CdCda* CvCda::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CdCda)));
	return (CdCda*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvCda message handlers

int CvCda::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	m_pDlgMain = new CuDlgMain(this);
	if (m_pDlgMain)
	{
		m_pDlgMain->Create (IDD_MAIN, this);
		m_pDlgMain->ShowWindow (SW_SHOW);
	}
	return 0;
}

void CvCda::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	
	if (m_pDlgMain && IsWindow (m_pDlgMain->m_hWnd))
	{
		CRect r;
		GetClientRect (r);
		m_pDlgMain->MoveWindow(r);
	}
}

void CvCda::OnSysColorChange()
{
	CView::OnSysColorChange();
	if (m_pDlgMain && IsWindow (m_pDlgMain->m_hWnd))
		m_pDlgMain->SendMessageToDescendants(WM_SYSCOLORCHANGE);
}
