/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qupview.cpp, Implementation file
**    Project  : CA-OpenIngres Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Upper view of the sql frame. It contains a serie of static control 
**               to display time taken to execute the queries.
**
** History:
**
** xx-Oct-1997 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "qupview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CvSqlQueryUpperView, CView)

BEGIN_MESSAGE_MAP(CvSqlQueryUpperView, CView)
	//{{AFX_MSG_MAP(CvSqlQueryUpperView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvSqlQueryUpperView construction/destruction

CvSqlQueryUpperView::CvSqlQueryUpperView()
{
	m_pStatementDlg = NULL;
}

CvSqlQueryUpperView::~CvSqlQueryUpperView()
{
}

BOOL CvSqlQueryUpperView::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= WS_CLIPCHILDREN;
	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CvSqlQueryUpperView drawing

void CvSqlQueryUpperView::OnDraw(CDC* pDC)
{
	CdSqlQueryRichEditDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CvSqlQueryUpperView printing

BOOL CvSqlQueryUpperView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CvSqlQueryUpperView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CvSqlQueryUpperView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CvSqlQueryUpperView diagnostics

#ifdef _DEBUG
void CvSqlQueryUpperView::AssertValid() const
{
	CView::AssertValid();
}

void CvSqlQueryUpperView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CdSqlQueryRichEditDoc* CvSqlQueryUpperView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CdSqlQueryRichEditDoc)));
	return (CdSqlQueryRichEditDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvSqlQueryUpperView message handlers

int CvSqlQueryUpperView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	try
	{
		CRect r;
		m_pStatementDlg = new CuDlgSqlQueryStatement(this);
		m_pStatementDlg->Create (IDD_SQLQUERY_STATEMENT, this);
		m_pStatementDlg->ShowWindow (SW_SHOW);
	}
	catch (...)
	{
		AfxMessageBox (IDS_MSG_FAIL_2_CREATE_SQL_PANE, MB_ICONEXCLAMATION|MB_OK);
		m_pStatementDlg = NULL;
	}
	return 0;
}

void CvSqlQueryUpperView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	if (!m_pStatementDlg)
		return;
	if (!IsWindow (m_pStatementDlg->m_hWnd))
		return;
	CRect r, rDlg;
	GetClientRect (rDlg);
	m_pStatementDlg->GetWindowRect (r);
	ScreenToClient (r);
	r.right = rDlg.right;
	m_pStatementDlg->MoveWindow (r);
}

void CvSqlQueryUpperView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	CdSqlQueryRichEditDoc* pDoc = GetDocument();
	if (pHint && pHint == pDoc)
	{
		if (pDoc->IsLoadedDoc())
		{
			m_pStatementDlg->LoadDocument();
		}
	}
}
