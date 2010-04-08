/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : VwDbeP02.cpp, Implementation File   (View of MDI Child Frame)         //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut.                                                         //
//                                                                                     //
//                                                                                     //
//    Purpose  : The right pane for the DBEvent Trace.                                 //
//               It contains the Modeless Dialog (Header and ListCtrl)                 //
****************************************************************************************/
#include "stdafx.h"
#include "mainmfc.h"
#include "vwdbep02.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDbeventView2

IMPLEMENT_DYNCREATE(CDbeventView2, CView)

CDbeventView2::CDbeventView2()
{
    m_pDlgRaisedDBEvent = NULL;
}

CDbeventView2::~CDbeventView2()
{
}


BEGIN_MESSAGE_MAP(CDbeventView2, CView)
    //{{AFX_MSG_MAP(CDbeventView2)
    ON_WM_SIZE()
    ON_WM_CREATE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDbeventView2 drawing

void CDbeventView2::OnDraw(CDC* pDC)
{
    CDocument* pDoc = GetDocument();
    // TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CDbeventView2 diagnostics

#ifdef _DEBUG
void CDbeventView2::AssertValid() const
{
    CView::AssertValid();
}

void CDbeventView2::Dump(CDumpContext& dc) const
{
    CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CDbeventView2 message handlers

void CDbeventView2::OnInitialUpdate() 
{
    CView::OnInitialUpdate();
    
    // TODO: Add your specialized code here and/or call the base class
    
}

void CDbeventView2::OnSize(UINT nType, int cx, int cy) 
{
    CView::OnSize(nType, cx, cy);
    if (!m_pDlgRaisedDBEvent || (m_pDlgRaisedDBEvent && !m_pDlgRaisedDBEvent->m_hWnd))
        return;
    CRect r;
    GetClientRect (r);
    m_pDlgRaisedDBEvent->MoveWindow (r);
}

int CDbeventView2::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    if (CView::OnCreate(lpCreateStruct) == -1)
        return -1;
    m_pDlgRaisedDBEvent = new CuDlgDBEventPane02 (this);
    m_pDlgRaisedDBEvent->Create (IDD_DBEVENTPANE2, this);
    m_pDlgRaisedDBEvent->ShowWindow (SW_SHOW);
    return 0;
}

BOOL CDbeventView2::PreCreateWindow(CREATESTRUCT& cs) 
{
    cs.style |= WS_CLIPCHILDREN;
    return CView::PreCreateWindow(cs);
}

void CDbeventView2::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	//
    // Notified from the Outside that the DBEvent should refresh its data
    // when lHint is not 0
}
