/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vsdaview.cpp : implementation of the CvSda class
**    Project  : INGRES II/ Visual Schema Diff Control (vdda.exe).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Frame/View/Doc Architecture (view)
**
** History:
**
** 23-Sep-2002 (uk$so01)
**    Created
** 23-Sep-2004 (uk$so01)
**    BUG #113114 
**    Force the SendMessageToDescendants(WM_SYSCOLORCHANGE) to reflect 
**    immediately the system color changed.
** 12-Oct-2004 (uk$so01)
**    BUG #113215 / ISSUE 13720075,
**    F1-key should invoke always the topic Overview if no comparison's result.
**    Otherwise F1-key should invoke always the ocx.s help.
*/

#include "stdafx.h"
#include "vsda.h"
#include "vsdadoc.h"
#include "ingobdml.h"
#include ".\vsdaview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CvSda, CView)

BEGIN_MESSAGE_MAP(CvSda, CView)
	//{{AFX_MSG_MAP(CvSda)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvSda construction/destruction

CvSda::CvSda()
{
	m_pDlgMain = NULL;
}

CvSda::~CvSda()
{
}

BOOL CvSda::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= WS_CLIPCHILDREN;
	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CvSda drawing

void CvSda::OnDraw(CDC* pDC)
{
	CdSda* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CString strTitle; 
	strTitle.LoadString (AFX_IDS_APP_TITLE);
	strTitle += INGRESII_QueryInstallationID();

	AfxGetMainWnd()->SetWindowText((LPCTSTR)strTitle);}

/////////////////////////////////////////////////////////////////////////////
// CvSda diagnostics

#ifdef _DEBUG
void CvSda::AssertValid() const
{
	CView::AssertValid();
}

void CvSda::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CdSda* CvSda::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CdSda)));
	return (CdSda*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvSda message handlers

int CvSda::OnCreate(LPCREATESTRUCT lpCreateStruct) 
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

void CvSda::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);

	if (m_pDlgMain && IsWindow (m_pDlgMain->m_hWnd))
	{
		CRect r;
		GetClientRect (r);
		m_pDlgMain->MoveWindow(r);
	}
}

CuVddaControl* CvSda::GetVsdaControl()
{
	if (!m_pDlgMain)
		return NULL;
	return &(m_pDlgMain->m_vsdaCtrl);
}


void CvSda::OnSysColorChange()
{
	CView::OnSysColorChange();
	CuVsda* pControl = GetVsdaControl();
	if (pControl && IsWindow (pControl->m_hWnd))
		pControl->SendMessageToDescendants(WM_SYSCOLORCHANGE);
}
