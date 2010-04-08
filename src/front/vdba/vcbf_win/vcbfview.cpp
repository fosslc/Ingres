/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Project : Ingres Configuration Manager
**
**  Source   : vcbfview.cpp
**
**  implementation of the CVcbfView class
**
**  22-Dec-2000 (noifr01)
**   (SIR 103548) appended the installation ID in the application title.
** 05-Apr-2001 (uk$so01)
**    (SIR 103548) only allow one vcbf per installation ID
** 03-Jun-2003 (uk$so01)
**    SIR #110344, Show the Raw Log information on the Unix Operating System.
*/

#include "stdafx.h"
#include "vcbf.h"
#include "vcbfdoc.h"
#include "vcbfview.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVcbfView

IMPLEMENT_DYNCREATE(CVcbfView, CView)

BEGIN_MESSAGE_MAP(CVcbfView, CView)
	//{{AFX_MSG_MAP(CVcbfView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVcbfView construction/destruction

CVcbfView::CVcbfView()
{

}

CVcbfView::~CVcbfView()
{
}

BOOL CVcbfView::PreCreateWindow(CREATESTRUCT& cs)
{
//	cs.style|=WS_CLIPCHILDREN;
	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CVcbfView drawing

void CVcbfView::OnDraw(CDC* pDC)
{
	CVcbfDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CString strTitle;
#if defined (EDBC)
	strTitle.LoadString (IDS_APP_EDBC_TITLE);
#else
	strTitle.LoadString (AFX_IDS_APP_TITLE);
#endif
	strTitle+=theApp.GetCurrentInstallationID();
	AfxGetMainWnd()->SetWindowText((LPCTSTR)strTitle);
}

/////////////////////////////////////////////////////////////////////////////
// CVcbfView printing

BOOL CVcbfView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CVcbfView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CVcbfView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CVcbfView diagnostics

#ifdef _DEBUG
void CVcbfView::AssertValid() const
{
	CView::AssertValid();
}

void CVcbfView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CVcbfDoc* CVcbfView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CVcbfDoc)));
	return (CVcbfDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CVcbfView message handlers

int CVcbfView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	m_pMainTabDlg = new CMainTabDlg(this);
	m_pMainTabDlg->Create(IDD_MAINTAB_DLG,this);
	m_pMainTabDlg->ShowWindow(SW_SHOW);
	return 0;
}

void CVcbfView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	
	if (m_pMainTabDlg && IsWindow(m_pMainTabDlg->m_hWnd)) {
		CRect r;
		GetClientRect(r);
		m_pMainTabDlg->MoveWindow(r);
	}
}

void CVcbfView::OnInitialUpdate() 
{
	CView::OnInitialUpdate();
	CMainFrame* pFrame = (CMainFrame*)GetParentFrame();
	ASSERT (pFrame);
	if (pFrame)
		pFrame->m_pVcbfView = this;
}
