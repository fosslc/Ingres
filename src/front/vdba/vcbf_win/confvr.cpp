/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : confvr.cpp, Implementation file                                       //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : View (right-pane of the page Configure)                               //
//               See the CONCEPT.DOC for more detail                                   //
****************************************************************************************/
#include "stdafx.h"
#include "vcbf.h"
#include "confvr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfViewRight

IMPLEMENT_DYNCREATE(CConfViewRight, CView)

CConfViewRight::CConfViewRight()
{
	m_pConfigRightDlg = NULL;
}

CConfViewRight::~CConfViewRight()
{
}


BEGIN_MESSAGE_MAP(CConfViewRight, CView)
	//{{AFX_MSG_MAP(CConfViewRight)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfViewRight drawing

void CConfViewRight::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CConfViewRight diagnostics

#ifdef _DEBUG
void CConfViewRight::AssertValid() const
{
	CView::AssertValid();
}

void CConfViewRight::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CConfViewRight message handlers

void CConfViewRight::OnInitialUpdate() 
{
	CView::OnInitialUpdate();
	
	// TODO: Add your specialized code here and/or call the base class
	
}

int CConfViewRight::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	m_pConfigRightDlg = new CConfRightDlg (this);
	m_pConfigRightDlg->Create (IDD_CONFIG_RIGHT, this);
	return 0;
}

void CConfViewRight::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	if (m_pConfigRightDlg && IsWindow (m_pConfigRightDlg->m_hWnd))
	{
		CRect r;
		GetClientRect (r);
		m_pConfigRightDlg->MoveWindow (r);
		m_pConfigRightDlg->ShowWindow (SW_SHOW);
	}
}

BOOL CConfViewRight::PreCreateWindow(CREATESTRUCT& cs) 
{
	return CView::PreCreateWindow(cs);
}


BOOL CConfViewRight::DisplayPage (CuPageInformation* pPageInfo)
{
	m_pConfigRightDlg->DisplayPage (pPageInfo);
	return TRUE;
}
