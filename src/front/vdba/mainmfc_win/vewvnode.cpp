/*****************************************************************************************
//                                                                                      //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                      //
//    Source   : VewVNode.cpp,Implementation file, MDI View of CFrmVNodeMDIChild        //
//                                                                                      //
//                                                                                      //
//    Project  : CA-OpenIngres/Monitoring.                                              //
//    Author   : UK Sotheavut                                                           //
//                                                                                      //
//                                                                                      //
//    Purpose  : Virtual Node Dialog Bar Resizable becomes a MDI Child when             //
//               it is not a Docking View.                                              //
*****************************************************************************************/
#include "stdafx.h"
#include "mainmfc.h"
#include "mainfrm.h"
#include "vewvnode.h"
#include "docvnode.h"
#include "frmvnode.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CViewVNodeMDIChild

IMPLEMENT_DYNCREATE(CViewVNodeMDIChild, CView)

CViewVNodeMDIChild::CViewVNodeMDIChild()
{
    m_pTree = NULL;
}

CViewVNodeMDIChild::~CViewVNodeMDIChild()
{
}

CTreeItemNodes* CViewVNodeMDIChild::GetSelectedVirtNodeItem()
{
    HTREEITEM hItem = m_pTree->GetSelectedItem();
    if (!hItem)
        return NULL;
    CTreeItemNodes *pItem = (CTreeItemNodes*)m_pTree->GetItemData(hItem);
    ASSERT(pItem != NULL);
    return pItem;
}

BOOL CViewVNodeMDIChild::UpdateNodeDlgItemButton (UINT idMenu)
{
    CTreeItemNodes *pItem = GetSelectedVirtNodeItem();
    if (!pItem)
        return FALSE;
    return pItem->IsEnabled (idMenu);
}



BEGIN_MESSAGE_MAP(CViewVNodeMDIChild, CView)
	//{{AFX_MSG_MAP(CViewVNodeMDIChild)
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_UPDATE_COMMAND_UI(IDM_VNODEBAR01, OnUpdateVnodebar01)
	ON_UPDATE_COMMAND_UI(IDM_VNODEBAR02, OnUpdateVnodebar02)
	ON_UPDATE_COMMAND_UI(IDM_VNODEBAR03, OnUpdateVnodebar03)
	ON_UPDATE_COMMAND_UI(IDM_VNODEBAR04, OnUpdateVnodebar04)
	ON_UPDATE_COMMAND_UI(IDM_VNODEBAR05, OnUpdateVnodebar05)
	ON_UPDATE_COMMAND_UI(IDM_VNODEBAR06, OnUpdateVnodebar06)
	ON_UPDATE_COMMAND_UI(IDM_VNODEBAR07, OnUpdateVnodebar07)
	ON_UPDATE_COMMAND_UI(IDM_VNODEBAR08, OnUpdateVnodebar08)
	ON_UPDATE_COMMAND_UI(IDM_VNODEBAR09, OnUpdateVnodebar09)
	ON_UPDATE_COMMAND_UI(IDM_VNODEBAR10, OnUpdateVnodebar10)
	ON_UPDATE_COMMAND_UI(IDM_VNODEBAR11, OnUpdateVnodebar11)
	ON_UPDATE_COMMAND_UI(IDM_VNODEBARSC, OnUpdateVnodebarscratch)
	ON_COMMAND(IDM_VNODEBAR01, OnVnodebar01)
	ON_COMMAND(IDM_VNODEBAR02, OnVnodebar02)
	ON_COMMAND(IDM_VNODEBAR03, OnVnodebar03)
	ON_COMMAND(IDM_VNODEBAR04, OnVnodebar04)
	ON_COMMAND(IDM_VNODEBAR05, OnVnodebar05)
	ON_COMMAND(IDM_VNODEBAR06, OnVnodebar06)
	ON_COMMAND(IDM_VNODEBAR07, OnVnodebar07)
	ON_COMMAND(IDM_VNODEBAR08, OnVnodebar08)
	ON_COMMAND(IDM_VNODEBAR09, OnVnodebar09)
	ON_COMMAND(IDM_VNODEBAR10, OnVnodebar10)
	ON_COMMAND(IDM_VNODEBAR11, OnVnodebar11)
	ON_COMMAND(IDM_VNODEBARSC, OnVnodebarscratch)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CViewVNodeMDIChild drawing

void CViewVNodeMDIChild::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CViewVNodeMDIChild diagnostics

#ifdef _DEBUG
void CViewVNodeMDIChild::AssertValid() const
{
	CView::AssertValid();
}

void CViewVNodeMDIChild::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CViewVNodeMDIChild message handlers

void CViewVNodeMDIChild::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
    if (!m_pTree)
        return;
    CRect r;
    GetClientRect (r);
    m_pTree->MoveWindow (r);
    m_pTree->ShowWindow (SW_SHOW);
}

int CViewVNodeMDIChild::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
  
    CMainFrame* mainWndPtr = (CMainFrame*)AfxGetMainWnd();
    ASSERT_VALID (mainWndPtr);
    m_pTree = mainWndPtr->GetVNodeTreeCtrl();
    ASSERT (m_pTree != NULL);
    m_pTree->SetParent (this);
    CRect r;
    GetClientRect (r);
    m_pTree->MoveWindow (r);
    m_pTree->ShowWindow (SW_SHOW);
	return 0;
}

void CViewVNodeMDIChild::OnDestroy() 
{
    CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
    if (m_pTree)
    {
        m_pTree->ShowWindow (SW_HIDE);
        if (m_pTree->GetParent() == this) m_pTree->SetParent (pMain->GetVNodeDlgBar());
    }
    pMain->SetNodesVisibilityFlag(FALSE);
    CView::OnDestroy();
}

BOOL CViewVNodeMDIChild::PreCreateWindow(CREATESTRUCT& cs) 
{
    cs.style |= WS_CLIPCHILDREN;
	return CView::PreCreateWindow(cs);
}

void CViewVNodeMDIChild::OnInitialUpdate() 
{
	CView::OnInitialUpdate();
	
  CDocVNodeMDIChild* pDoc = (CDocVNodeMDIChild*)GetDocument();
  ASSERT(pDoc);
  pDoc->SetTitle ((LPCTSTR)"Virtual Nodes");
  // No toolbar to set the caption on!

  if (pDoc->IsLoadedDoc()) {
    // frame window placement
    CFrmVNodeMDIChild* pFrame = (CFrmVNodeMDIChild*)GetParent();  // 1 level since no splitter wnd
    BOOL bResult = pFrame->SetWindowPlacement(pDoc->GetWPLJ());
    ASSERT (bResult);

  }
}

void CViewVNodeMDIChild::OnUpdateVnodebar01(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable (UpdateNodeDlgItemButton(pCmdUI->m_nID));
}

void CViewVNodeMDIChild::OnUpdateVnodebar02(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable (UpdateNodeDlgItemButton(pCmdUI->m_nID));
}

void CViewVNodeMDIChild::OnUpdateVnodebar03(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable (UpdateNodeDlgItemButton(pCmdUI->m_nID));
}

void CViewVNodeMDIChild::OnUpdateVnodebar04(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable (UpdateNodeDlgItemButton(pCmdUI->m_nID));
}

void CViewVNodeMDIChild::OnUpdateVnodebar05(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable (UpdateNodeDlgItemButton(pCmdUI->m_nID));
}

void CViewVNodeMDIChild::OnUpdateVnodebar06(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable (UpdateNodeDlgItemButton(pCmdUI->m_nID));
}

void CViewVNodeMDIChild::OnUpdateVnodebar07(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable (UpdateNodeDlgItemButton(pCmdUI->m_nID));
}

void CViewVNodeMDIChild::OnUpdateVnodebar08(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable (UpdateNodeDlgItemButton(pCmdUI->m_nID));
}

void CViewVNodeMDIChild::OnUpdateVnodebar09(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable (UpdateNodeDlgItemButton(pCmdUI->m_nID));
}

void CViewVNodeMDIChild::OnUpdateVnodebar10(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable (UpdateNodeDlgItemButton(pCmdUI->m_nID));
}

void CViewVNodeMDIChild::OnUpdateVnodebar11(CCmdUI* pCmdUI) 
{
  //TRACE0 ("CViewVNodeMDIChild::OnUpdateVnodebar11()...\n");
	pCmdUI->Enable (TRUE);
}

void CViewVNodeMDIChild::OnVnodebar01() 
{
    CTreeItemNodes* pItem = GetSelectedVirtNodeItem();
    if (pItem)
        pItem->TreeConnect();
}

void CViewVNodeMDIChild::OnVnodebar02() 
{
    CTreeItemNodes* pItem = GetSelectedVirtNodeItem();
    if (pItem)
        pItem->TreeSqlTest();
}

void CViewVNodeMDIChild::OnVnodebar03() 
{
    CTreeItemNodes* pItem = GetSelectedVirtNodeItem();
    if (pItem)
        pItem->TreeMonitor();
}

void CViewVNodeMDIChild::OnVnodebar04() 
{
    CTreeItemNodes* pItem = GetSelectedVirtNodeItem();
    if (pItem)
        pItem->TreeDbevent();
}

void CViewVNodeMDIChild::OnVnodebar05() 
{
    CTreeItemNodes* pItem = GetSelectedVirtNodeItem();
    if (pItem)
        pItem->TreeDisconnect();
}

void CViewVNodeMDIChild::OnVnodebar06() 
{
    CTreeItemNodes* pItem = GetSelectedVirtNodeItem();
    if (pItem)
        pItem->TreeCloseWin();
}

void CViewVNodeMDIChild::OnVnodebar07() 
{
    CTreeItemNodes* pItem = GetSelectedVirtNodeItem();
    if (pItem)
        pItem->TreeActivateWin();
}

void CViewVNodeMDIChild::OnVnodebar08() 
{
    CTreeItemNodes* pItem = GetSelectedVirtNodeItem();
    if (pItem)
        pItem->TreeAdd();
}

void CViewVNodeMDIChild::OnVnodebar09() 
{
    CTreeItemNodes* pItem = GetSelectedVirtNodeItem();
    if (pItem)
        pItem->TreeAlter();
}

void CViewVNodeMDIChild::OnVnodebar10() 
{
    CTreeItemNodes* pItem = GetSelectedVirtNodeItem();
    if (pItem)
        pItem->TreeDrop();
}

void CViewVNodeMDIChild::OnVnodebar11() 
{
    TRACE0 ("CViewVNodeMDIChild::OnVnodebar11()...\n");

    // Added Emb 26/05 : did nothing in mdi mode...
    CWaitCursor hourGlass;
    UpdateMonInfo(0, 0, NULL, OT_NODE);
    UpdateNodeDisplay();
}

void CViewVNodeMDIChild::OnUpdateVnodebarscratch(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable (UpdateNodeDlgItemButton(pCmdUI->m_nID));
}

void CViewVNodeMDIChild::OnVnodebarscratch() 
{
    CTreeItemNodes* pItem = GetSelectedVirtNodeItem();
    if (pItem)
        pItem->TreeScratchpad();
}
