/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : lvleft.cpp, Implementation file                                       //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : View (left-pane of the page Configure)                                //
//               See the CONCEPT.DOC for more detail                                   //
****************************************************************************************/

#include "stdafx.h"
#include "vcbf.h"
#include "lvleft.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfListViewLeft

IMPLEMENT_DYNCREATE(CConfListViewLeft, CView)

CConfListViewLeft::CConfListViewLeft()
{
	m_pDialog = NULL;
}

CConfListViewLeft::~CConfListViewLeft()
{
}


BEGIN_MESSAGE_MAP(CConfListViewLeft, CView)
	//{{AFX_MSG_MAP(CConfListViewLeft)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfListViewLeft drawing

void CConfListViewLeft::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CConfListViewLeft diagnostics

#ifdef _DEBUG
void CConfListViewLeft::AssertValid() const
{
	CView::AssertValid();
}

void CConfListViewLeft::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CConfListViewLeft message handlers

int CConfListViewLeft::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	// store object pointer for use in CMainFrame::OnClose()
	CVcbfApp* pApp = (CVcbfApp*) AfxGetApp();
	ASSERT (pApp->m_pConfListViewLeft == 0);
	pApp->m_pConfListViewLeft = this;

	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	m_pDialog = new CConfLeftDlg (this);
	m_pDialog->Create (IDD_CONFIG_LEFT, this);
	return 0;
}

void CConfListViewLeft::OnDestroy() 
{
	CView::OnDestroy();
	
	// TODO: Add your message handler code here
	
}

void CConfListViewLeft::OnInitialUpdate() 
{
	CView::OnInitialUpdate();
	
	// TODO: Add your specialized code here and/or call the base class
	
}

void CConfListViewLeft::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	if (m_pDialog && IsWindow (m_pDialog->m_hWnd))
	{
		CRect r;
		GetClientRect (r);
		m_pDialog->MoveWindow (r);
		m_pDialog->ShowWindow (SW_SHOW);
	}
}
