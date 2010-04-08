/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vrepcort.cpp, Implementation file
**    Project  : INGRESII / Monitoring.
**    Author   : UK Sotheavut
**    Purpose  : The view containing the modeless dialog to display the conflict rows 
**
** History:
**
** xx-May-1998 (uk$so01)
**    Created
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "repcodoc.h"
#include "vrepcort.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CvReplicationPageCollisionViewRight, CView)

CvReplicationPageCollisionViewRight::CvReplicationPageCollisionViewRight()
{
	m_pDlg = NULL;
	m_pDlg2= NULL;
	m_bTransaction = TRUE;
}

CvReplicationPageCollisionViewRight::~CvReplicationPageCollisionViewRight()
{
}


BEGIN_MESSAGE_MAP(CvReplicationPageCollisionViewRight, CView)
	//{{AFX_MSG_MAP(CvReplicationPageCollisionViewRight)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_CHANGE_SETTING,   OnPropertiesChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvReplicationPageCollisionViewRight drawing

void CvReplicationPageCollisionViewRight::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CvReplicationPageCollisionViewRight diagnostics

#ifdef _DEBUG
void CvReplicationPageCollisionViewRight::AssertValid() const
{
	CView::AssertValid();
}

void CvReplicationPageCollisionViewRight::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvReplicationPageCollisionViewRight message handlers

int CvReplicationPageCollisionViewRight::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	try
	{
		m_pDlg = new CuDlgReplicationPageCollisionRight(this);
		m_pDlg->Create (IDD_REPCOLLISION_PAGE_RIGHT, this);
		m_pDlg2= new CuDlgReplicationPageCollisionRight2(this);
		m_pDlg2->Create (IDD_REPCOLLISION_PAGE_RIGHT2, this);
	}
	catch (...)
	{
		TRACE0 ("CvReplicationPageCollisionViewRight::Cannot create dialog for displaying the conflict rows\n");
		m_pDlg = NULL;
	}

	return 0;
}

void CvReplicationPageCollisionViewRight::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	if (m_pDlg && IsWindow (m_pDlg->m_hWnd))
	{
		CRect r;
		GetClientRect (r);
		m_pDlg->MoveWindow (r);
		if (!m_bTransaction)
		{
			if (m_pDlg2 && IsWindow (m_pDlg2->m_hWnd))
				m_pDlg2->ShowWindow (SW_HIDE);
			m_pDlg->ShowWindow (SW_SHOW);
		}
	}
	if (m_pDlg2 && IsWindow (m_pDlg2->m_hWnd))
	{
		CRect r;
		GetClientRect (r);
		m_pDlg2->MoveWindow (r);
		if (m_bTransaction)
		{
			if (m_pDlg && IsWindow (m_pDlg->m_hWnd))
				m_pDlg->ShowWindow (SW_HIDE);
			m_pDlg2->ShowWindow (SW_SHOW);
		}
	}
}


LONG CvReplicationPageCollisionViewRight::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	HTREEITEM hSelected = (HTREEITEM)wParam;
	CaCollisionItem* pItem = (CaCollisionItem*)lParam;
	if (!pItem)
	{
		if (m_pDlg && IsWindow (m_pDlg->m_hWnd))
			m_pDlg->ShowWindow (SW_HIDE);
		m_pDlg2->DisplayData (NULL);
		m_pDlg2->ShowWindow (SW_SHOW);
		return 0;
	}

	m_bTransaction = pItem->IsTransaction();
	if (m_bTransaction)
	{
		if (m_pDlg && IsWindow (m_pDlg->m_hWnd))
			m_pDlg->ShowWindow (SW_HIDE);
		m_pDlg2->DisplayData ((CaCollisionTransaction*)pItem);
		m_pDlg2->ShowWindow (SW_SHOW);
	}
	else
	{
		if (m_pDlg2 && IsWindow (m_pDlg2->m_hWnd))
			m_pDlg2->ShowWindow (SW_HIDE);
		m_pDlg->DisplayData ((CaCollisionSequence*)pItem);
		m_pDlg->ShowWindow (SW_SHOW);
	}

	return 0;
}

long CvReplicationPageCollisionViewRight::OnPropertiesChange(WPARAM wParam, LPARAM lParam)
{
	if (m_pDlg && IsWindow (m_pDlg->m_hWnd))
		PropertyChange(&(m_pDlg->m_cListCtrl), wParam, lParam);
	return 0;
}
