/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : viewrigh.h, implementation File
**    Project  : INGRESII / Monitoring.
**    Author   : UK Sotheavut (old code from ipmview2.cpp)
**    Purpose  : The right pane for the Monitor Window.
**               It is responsible for displaying the Modeless Dialog of the Tree Item.
**
** History:
**
** 12-Nov-2001 (uk$so01)
**    Created
** 19-May-2003 (uk$so01)
**    SIR #110269, Add Network trafic monitoring.
** 07-Apr-2004 (uk$so01)
**    BUG #112117 / ISSUE 13349347 (The filter check box "Internal Sessions",..
**    does not get refresh imediately).
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "vtree.h"
#include "ipmframe.h"
#include "ipmdoc.h"
#include "viewrigh.h"
#include "dgipmc02.h"
#include "viewleft.h"
#include "montree.h"   // General Monitor Tree Items
#include "treerepl.h"  // Replication Tree Items
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CvIpmRight, CView)

CvIpmRight::CvIpmRight()
{
	m_pIpmTabDialog = NULL;
}

CvIpmRight::~CvIpmRight()
{
}


BEGIN_MESSAGE_MAP(CvIpmRight, CView)
	//{{AFX_MSG_MAP(CvIpmRight)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvIpmRight drawing

void CvIpmRight::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CvIpmRight diagnostics

#ifdef _DEBUG
void CvIpmRight::AssertValid() const
{
	CView::AssertValid();
}

void CvIpmRight::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvIpmRight message handlers

int CvIpmRight::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	CdIpmDoc* pDoc = (CdIpmDoc*)GetDocument();
	ASSERT (pDoc);
	if (pDoc && pDoc->GetShowTree() == 0)
	{
		UINT nStyle = WS_CHILD | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS;
		m_treeCtrl.SetIpmDoc(pDoc);
		m_treeCtrl.Create(nStyle, CRect (0, 0, 10, 10), this, -1);
		pDoc->GetPTreeGD()->SetPTree(&m_treeCtrl);
	}

	return 0;
}

void CvIpmRight::OnInitialUpdate() 
{
	CRect r;
	CView::OnInitialUpdate();
	//
	// Create a Modeless Dialog that is child and occupied the view's client area.
	// 
	try
	{
		CdIpmDoc* pDoc = (CdIpmDoc*)GetDocument();
		ASSERT (pDoc);
		m_pIpmTabDialog = new CuDlgIpmTabCtrl (this);
		if (!m_pIpmTabDialog->Create (IDD_IPMCAT02, this))
			AfxThrowResourceException();
		pDoc->SetTabDialog (m_pIpmTabDialog);

	}
	catch (...)
	{
		throw;
	}
	GetClientRect (r);
	m_pIpmTabDialog->MoveWindow (r);
	m_pIpmTabDialog->ShowWindow (SW_SHOW);
}

void CvIpmRight::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	if (!(m_pIpmTabDialog && IsWindow (m_pIpmTabDialog->m_hWnd)))
		return;
	CRect r;
	GetClientRect (r);
	m_pIpmTabDialog->MoveWindow (r);
}

BOOL CvIpmRight::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.style |= WS_CLIPCHILDREN;
	return CView::PreCreateWindow(cs);
}

void CvIpmRight::DisplayProperty (CuPageInformation* pPageInfo)
{
	if (m_pIpmTabDialog)
		m_pIpmTabDialog->DisplayPage (pPageInfo);
}

void CvIpmRight::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	BOOL bRegularStaticSet = TRUE;
	CdIpmDoc* pDoc = (CdIpmDoc*)GetDocument();
	ASSERT (pDoc);
	CaIpmProperty& property = pDoc->GetProperty();
	int nHint = (int)lHint;
	switch (nHint)
	{
	case 1: 
		// Invoked by CdIpmDoc::Initiate()
		// Create regular static set, if needed
		if (bRegularStaticSet && pDoc->GetShowTree() == 0) 
		{
			CuTMServerStatic    *pItem1 = new CuTMServerStatic   (pDoc->GetPTreeGD());
			CuTMLockinfoStatic  *pItem3 = new CuTMLockinfoStatic (pDoc->GetPTreeGD());
			CuTMLoginfoStatic   *pItem4 = new CuTMLoginfoStatic  (pDoc->GetPTreeGD());
			CuTMAllDbStatic     *pItem5 = new CuTMAllDbStatic    (pDoc->GetPTreeGD());
			CuTMActiveUsrStatic *pItem6 = new CuTMActiveUsrStatic(pDoc->GetPTreeGD());
			CuTMReplAllDbStatic *pItem7 = new CuTMReplAllDbStatic(pDoc->GetPTreeGD());

			HTREEITEM hItem1 = pItem1->CreateTreeLine();
			HTREEITEM hItem2 = pItem3->CreateTreeLine();
			HTREEITEM hItem3 = pItem4->CreateTreeLine();
			HTREEITEM hItem4 = pItem5->CreateTreeLine();
			HTREEITEM hItem5 = pItem6->CreateTreeLine();
			HTREEITEM hItem6 = pItem7->CreateTreeLine();
			ASSERT (hItem1 && hItem2 && hItem3 && hItem4 && hItem5 && hItem6);
			m_treeCtrl.SelectItem(hItem1);
			if (m_pIpmTabDialog)
				m_pIpmTabDialog->SendMessage (WM_USER_IPMPAGE_UPDATEING, (WPARAM)pDoc, (LPARAM)lHint);
		}
		break;
	default:
		if (m_pIpmTabDialog)
			m_pIpmTabDialog->SendMessage (WM_USER_IPMPAGE_UPDATEING, (WPARAM)pDoc, (LPARAM)lHint);
		break;
	}
}

void CvIpmRight::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	if (m_pIpmTabDialog && !bActivate)
		m_pIpmTabDialog->SendMessage (WM_LAYOUTEDIT_HIDE_PROPERTY, (WPARAM)0, (LPARAM)0);
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

void CvIpmRight::OnDestroy() 
{
	CdIpmDoc* pDoc = (CdIpmDoc*)GetDocument();
	ASSERT (pDoc);
	if (pDoc && pDoc->GetShowTree() == 0)
		pDoc->GetPTreeGD()->FreeAllTreeItemsData();

	CView::OnDestroy();
}
