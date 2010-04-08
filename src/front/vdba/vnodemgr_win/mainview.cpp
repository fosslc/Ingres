/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : mainview.cpp, Implementation File  
*
**    Project  : Virtual Node Manager.      
**    Author   : UK Sotheavut, Detail implementation. 
** 
**    Purpose  : Main View (Frame, View, Doc design). Tree View of Node Data
**
**    HISTORY:
**    uk$so01:  (24-jan-2000, Sir #100102)
**               Add the "Performance Monitor" command in toolbar and menu
**    27-01-2000 (noifr01)
**     (bug 100187) VNODE_RunVdbaSql() was called instead of VNODE_RunVdbaIpm() for
**     opening a monitor window against the local node
**    14-may-2001 (zhayu01) SIR 104724
**     Change "Node test is successful" to "Server test was successful".
**    28-Nov-2000 (schph01)
**     (SIR 102945) Grayed menu "Add Installation Password..." when the selection
**     on tree is on another branch that "Nodes" and "(Local)".
**    22-may-2001 (zhayu01) SIR 104751 for EDBC
**		1. Check login in the OnRButtonDown() also.
**		2. Add CheckLogin() and use it when necessary.
**		3. Test the connection first when expanding a server node and do not continue
**		   if the test fails.
**    18-jul-2001 (zhayu01) SIR 105279 for EDBC
**		Display the menu with every item grayed out except delete if the login failed 
**		or cancelled in the OnRButtonDown().
**    15-aug-2001 (zhayu01) Additions to SIR 105279 for EDBC
**		Un-grayed out the server test menu item in addition to the delete.
** 04-Apr-2003 (uk$so01)
**    SIR #106648, Additional change, invoke vdbamon and vdbasql instead of 
**    vdba in context.
** 18-Sep-2003 (uk$so01)
**    SIR #106648, Integrate the libingll.lib and cleanup codes.
** 10-Oct-2003 (schph01)
**    SIR #109864, Add functions to manage the menu that launch ingnet with the
**    -vnode option.
** 14-Nov-2003 (uk$so01)
**    BUG #110983, If GCA Protocol Level = 50 then use MIB Objects 
**    to query the server classes.
** 10-Mar-2010(drivi01) SIR 123397
**    Update the routines to return error message when Test connection
**    fails to provide more information to the user.
*/


#include "stdafx.h"
#include "vnodemgr.h"
#include "maindoc.h"
#include "mainview.h"
#include "resource.h"
#include "vnodedml.h"
extern "C"
{
STATUS INGRESII_ERLookup(int msgid, char** buf, int buf_len);
}
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#ifdef	EDBC
BOOL CheckLogin(CString strNode);
#endif

/////////////////////////////////////////////////////////////////////////////
// CvMainView

IMPLEMENT_DYNCREATE(CvMainView, CTreeView)

BEGIN_MESSAGE_MAP(CvMainView, CTreeView)
	//{{AFX_MSG_MAP(CvMainView)
#if defined(EDBC)
	ON_COMMAND(ID_SERVER_ADD, OnNodeAdd)
	ON_COMMAND(ID_SERVER_ALTER, OnNodeAlter)
	ON_COMMAND(ID_SERVER_DROP, OnNodeDrop)
#else
	ON_COMMAND(ID_NODE_ADD, OnNodeAdd)
	ON_COMMAND(ID_NODE_ALTER, OnNodeAlter)
	ON_COMMAND(ID_NODE_DROP, OnNodeDrop)
#endif
	ON_COMMAND(ID_NODE_REFRESH, OnNodeRefresh)
	ON_COMMAND(ID_NODE_SQLTEST, OnNodeSqltest)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED, OnItemexpanded)
	ON_WM_RBUTTONDOWN()
#if defined(EDBC)
	ON_UPDATE_COMMAND_UI(ID_SERVER_ADD, OnUpdateNodeAdd)
	ON_UPDATE_COMMAND_UI(ID_SERVER_ALTER, OnUpdateNodeAlter)
	ON_UPDATE_COMMAND_UI(ID_SERVER_DROP, OnUpdateNodeDrop)
#else
	ON_UPDATE_COMMAND_UI(ID_NODE_ADD, OnUpdateNodeAdd)
	ON_UPDATE_COMMAND_UI(ID_NODE_ALTER, OnUpdateNodeAlter)
	ON_UPDATE_COMMAND_UI(ID_NODE_DROP, OnUpdateNodeDrop)
#endif
	ON_UPDATE_COMMAND_UI(ID_NODE_SQLTEST, OnUpdateNodeSqltest)
	ON_COMMAND(ID_NODE_ADD_INSTALLATION, OnNodeAddInstallation)
#if defined(EDBC)
	ON_COMMAND(ID_SERVER_TEST, OnNodeTest)
	ON_UPDATE_COMMAND_UI(ID_SERVER_TEST, OnUpdateNodeTest)
#else
	ON_COMMAND(ID_NODE_TEST, OnNodeTest)
	ON_UPDATE_COMMAND_UI(ID_NODE_TEST, OnUpdateNodeTest)
#endif
	ON_COMMAND(ID_NODE_DOM, OnNodeDom)
	ON_UPDATE_COMMAND_UI(ID_NODE_DOM, OnUpdateNodeDom)
	ON_COMMAND(ID_NODE_MONITOR, OnNodeMonitor)
	ON_UPDATE_COMMAND_UI(ID_NODE_MONITOR, OnUpdateNodeMonitor)
	ON_UPDATE_COMMAND_UI(ID_NODE_ADD_INSTALLATION, OnUpdateNodeAddInstallation)
	ON_COMMAND(ID_NODE_EDIT_VIEW, OnNodeEditView)
	ON_UPDATE_COMMAND_UI(ID_NODE_EDIT_VIEW, OnUpdateNodeEditView)
	ON_COMMAND(ID_MANAGE_LOCALVNODE, OnManageLocalvnode)
	ON_UPDATE_COMMAND_UI(ID_MANAGE_LOCALVNODE, OnUpdateManageLocalvnode)
	ON_COMMAND(ID_POPUP_NODE_EDIT_VIEW, OnPopupNodeEditView)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CTreeView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CTreeView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CTreeView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvMainView construction/destruction

CvMainView::CvMainView()
{
	m_bInitialize = FALSE;
}

CvMainView::~CvMainView()
{
}

BOOL CvMainView::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS|TVS_SHOWSELALWAYS;
	return CTreeView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CvMainView drawing

void CvMainView::OnDraw(CDC* pDC)
{
	CdMainDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
}

void CvMainView::OnInitialUpdate()
{
	CTreeView::OnInitialUpdate();
	CdMainDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	//
	// This application maintains only a single document, so
	// we can give it to be accessed as a global:
	if (pDoc)
		theApp.m_pDocument = pDoc;

	if (!m_bInitialize)
	{
		CTreeCtrl& cTree = GetTreeCtrl();
		m_ImageList.Create (IDB_NODETREE, 16, 0, RGB(255,0,255));
		cTree.SetImageList (&m_ImageList, TVSIL_NORMAL);

		InitializeData();
		m_bInitialize = TRUE;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CvMainView printing

BOOL CvMainView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CvMainView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CvMainView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CvMainView diagnostics

#ifdef _DEBUG
void CvMainView::AssertValid() const
{
	CTreeView::AssertValid();
}

void CvMainView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}

CdMainDoc* CvMainView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CdMainDoc)));
	return (CdMainDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvMainView message handlers

void CvMainView::InitializeData()
{
	CWaitCursor doWaitCursor;
	CTreeCtrl& cTree = GetTreeCtrl();
	CdMainDoc* pDoc = (CdMainDoc*)GetDocument();
	if (!pDoc)
		return;
	CaTreeNodeFolder& vnodeData = pDoc->GetDataVNode();
	vnodeData.Refresh();
	vnodeData.Display(&cTree);
}

void CvMainView::OnNodeAdd() 
{
	CWaitCursor doWaitCursor;
	CTreeCtrl& cTree = GetTreeCtrl();
	HTREEITEM hSelected = cTree.GetSelectedItem();
	if (!hSelected)
		return;
	CaVNodeTreeItemData* pItem = (CaVNodeTreeItemData*)cTree.GetItemData (hSelected);
	if (!pItem)
		return;
	BOOL bAdd = pItem->Add();
	if (bAdd)
	{
		//
		// Refresh Data:
		OnNodeRefresh();
	}
}

void CvMainView::OnNodeAlter() 
{
	CWaitCursor doWaitCursor;
	CTreeCtrl& cTree = GetTreeCtrl();
	HTREEITEM hSelected = cTree.GetSelectedItem();
	if (!hSelected)
		return;
	CaVNodeTreeItemData* pItem = (CaVNodeTreeItemData*)cTree.GetItemData (hSelected);
	if (!pItem)
		return;
	BOOL bAlter = pItem->Alter();
	if (bAlter)
	{
		//
		// Refresh Data:
		OnNodeRefresh();
	}
}

void CvMainView::OnNodeDrop() 
{
	CWaitCursor doWaitCursor;
	CTreeCtrl& cTree = GetTreeCtrl();
	HTREEITEM hSelected = cTree.GetSelectedItem();
	if (!hSelected)
		return;
	CaVNodeTreeItemData* pItem = (CaVNodeTreeItemData*)cTree.GetItemData (hSelected);
	if (!pItem)
		return;
	if (pItem->Drop())
		OnNodeRefresh();
}

void CvMainView::OnNodeRefresh() 
{
	CWaitCursor doWaitCursor;
	CdMainDoc* pDoc = (CdMainDoc*)GetDocument();
	if (!pDoc)
		return;
	CTreeCtrl& cTree = GetTreeCtrl();
	pDoc->GetDataVNode().Refresh();       // Data
	pDoc->GetDataVNode().Display(&cTree); // Visual Rendering
}

void CvMainView::OnNodeSqltest() 
{
	BOOL bExecOK = FALSE;
	BOOL bAnable = FALSE;
	CTreeCtrl& cTree = GetTreeCtrl();
	HTREEITEM hSel = cTree.GetSelectedItem();
	if (hSel)
	{
		CaVNodeTreeItemData* pItem = (CaVNodeTreeItemData*)cTree.GetItemData (hSel);
		if (!pItem)
			return;
		CString strNode = pItem->GetName();
		CString strNode1 = strNode;
		if (strNode.Find(_T(' ')) != -1) 
			strNode1.Format (_T("\"%s\""), (LPCTSTR)strNode);
		strNode = strNode1;

		if (pItem->GetNodeType() != NODExSERVER)
		{
#ifdef	EDBC
			if (!CheckLogin(strNode))
				return;
#endif
			bExecOK = VNODE_RunVdbaSql(strNode);
		}
		else
		{
			if (pItem->GetNodeType() == NODExSERVER)
			{
				CaIngnetNodeServer* pServer = (CaIngnetNodeServer*)pItem;
				strNode = pServer->GetNodeName();
				strNode1 = strNode;
				if (strNode1.Find(_T(' ')) != -1) 
					strNode1.Format (_T("\"%s\""), (LPCTSTR)strNode);

				strNode.Format (_T("%s /%s"), (LPCTSTR)strNode1, (LPCTSTR)pServer->GetName());
				bExecOK = VNODE_RunVdbaSql(strNode);
			}
		}
		if(!bExecOK)
		{
			CString strMsg = _T("Cannot run SQL Test Window.\n");
			TRACE0(strMsg);
		}
	}
}

void CvMainView::OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CWaitCursor doWaitCursor;
	int nImage = -1;
	*pResult = 0;
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	HTREEITEM hExpand = pNMTreeView->itemNew.hItem;
	CTreeCtrl& cTree = GetTreeCtrl();

	CaVNodeTreeItemData* pItem = (CaVNodeTreeItemData*)cTree.GetItemData (hExpand);
	if (!pItem)
		return;
	
	CaTreeNodeInformation& nodeInfo = pItem->GetNodeInformation();
	if (pNMTreeView->action & TVE_COLLAPSE)
	{
		//
		// Collapseing:
		pItem->OnExpandBranch (&cTree, hExpand, FALSE);
	}
	else
	if (pNMTreeView->action & TVE_EXPAND)
	{
		//
		// Expanding:
#ifdef	EDBC
		/*
		** Test connection before continue if it is a server node and
		** do not expand if the test fails.
		*/
		NODETYPE nodetype = pItem->GetNodeType();
		if (nodetype == NODExNORMAL)
		{
			CString strNode = pItem->GetName();
			if (!CheckLogin(strNode))
			{
				/*
				** Collapse this node so that we do not expose any server objects.
				*/
				cTree.Expand(hExpand, TVE_COLLAPSE);
				return;
			}
			else
			{
				STATUS bSuccess = INGRESII_NodeCheckConnection(strNode);
				if (bSuccess)
				{
					/*
					** Collapse this node so that we do not expose any server objects.
					*/
					cTree.Expand(hExpand, TVE_COLLAPSE);
					CString strMsg;
					strMsg.Format(_T("Failed to connect to server %s."), (LPCTSTR)strNode);
					AfxMessageBox(strMsg, MB_OK | MB_ICONEXCLAMATION);
					return;
				}
			}
		}
#endif
		pItem->OnExpandBranch (&cTree, hExpand, TRUE);
	}
}


void CvMainView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	BOOL bAddEnabled = FALSE;
	BOOL bAlterEnabled = FALSE;
	BOOL bDropEnabled = FALSE;
	BOOL bSqlEnabled = FALSE;
	BOOL bTestNodeEnabled = FALSE;
	BOOL bIPMEnabled = FALSE;
	BOOL bAddInstallPasswordEnabled = FALSE;
	BOOL bManageLocalVnodeEnabled = FALSE;

	CTreeView::OnRButtonDown(nFlags, point);
	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_NODE_POPUP));

	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);
	CWnd* pWndPopupOwner = this;
	//
	// Menu handlers are coded in this source:
	// So do not use the following statement:
	// while (pWndPopupOwner->GetStyle() & WS_CHILD)
	//     pWndPopupOwner = pWndPopupOwner->GetParent();
	//
	CPoint p = point;
	ClientToScreen(&p);

	CaVNodeTreeItemData* pItem = NULL;
	CTreeCtrl& cTree = GetTreeCtrl();
	UINT nFlag = TVHT_ONITEM;
	HTREEITEM hHit = cTree.HitTest (point, &nFlag);
	if (hHit)
		cTree.SelectItem (hHit);
	HTREEITEM hSel = cTree.GetSelectedItem();
	if (hSel)
	{
		pItem = (CaVNodeTreeItemData*)cTree.GetItemData (hSel);
		if (pItem)
		{
#ifdef EDBC
			NODETYPE nodetype = pItem->GetNodeType();
			if (nodetype == NODExNORMAL)
			{
				CTypedPtrList<CObList, CaVNodeTreeItemData*> listLogin;
				POSITION pos;
				BOOL bSave = TRUE;
				CString Login;
				CaIngnetNodeLogin* pLogin = NULL;

				VNODE_QueryLogin (pItem->GetName(), listLogin);
				pos = listLogin.GetHeadPosition();
				if (pos != NULL)
				{
					pLogin = (CaIngnetNodeLogin*)listLogin.GetNext (pos);
					bSave = pLogin->IsSave();
					Login = pLogin->GetName ();
				}
				if (!bSave && Login.IsEmpty())
				{
					/*
					** Prompt the user to enter user name and password 
					** if they have not been entered already.
					*/
					if (!pLogin->Enter())
					{
						/*
						** The login failed or cancelled, display the menu with
						** only drop and server test enabled.
						*/
						menu.EnableMenuItem (ID_SERVER_ADD, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
						menu.EnableMenuItem (ID_SERVER_ALTER, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
						menu.EnableMenuItem (ID_SERVER_DROP, MF_BYCOMMAND|MF_ENABLED);
						menu.EnableMenuItem (ID_NODE_SQLTEST, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
						menu.EnableMenuItem (ID_NODE_DOM, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
						menu.EnableMenuItem (ID_NODE_REFRESH, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
						menu.DeleteMenu (ID_NODE_ADD_INSTALLATION, MF_BYCOMMAND);
						menu.DeleteMenu (ID_NODE_MONITOR, MF_BYCOMMAND);
						pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, p.x, p.y, pWndPopupOwner);
						return;
					}
				}
			}
#endif
			bAddEnabled = pItem->IsAddEnabled();
			bAlterEnabled = pItem->IsAlterEnabled();
			bDropEnabled = pItem->IsDropEnabled();
			if (!theApp.m_pNodeAccess->IsVnodeOptionDefined())
			{
				bSqlEnabled = pItem->IsSQLEnabled();
				bTestNodeEnabled = pItem->IsTestNodeEnabled();
				bIPMEnabled = pItem->IsIPMEnabled();
				if (pItem->GetNodeType() == NODExNORMAL)
					bManageLocalVnodeEnabled = TRUE;
			}

			bAddInstallPasswordEnabled = pItem->IsAddInstallationEnabled();
#if defined (EDBC)
			//
			// Installation password.
			if (bAddInstallPasswordEnabled)
				menu.EnableMenuItem (ID_NODE_ADD_INSTALLATION, MF_BYCOMMAND|MF_ENABLED);
			else
				menu.EnableMenuItem (ID_NODE_ADD_INSTALLATION, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
			//
			// Add ...
			if (bAddEnabled)
				menu.EnableMenuItem (ID_SERVER_ADD, MF_BYCOMMAND|MF_ENABLED);
			else
				menu.EnableMenuItem (ID_SERVER_ADD, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
			//
			// Alter ...
			if (bAlterEnabled)
				menu.EnableMenuItem (ID_SERVER_ALTER, MF_BYCOMMAND|MF_ENABLED);
			else
				menu.EnableMenuItem (ID_SERVER_ALTER, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
			//
			// Drop ...
			if (bDropEnabled)
				menu.EnableMenuItem (ID_SERVER_DROP, MF_BYCOMMAND|MF_ENABLED);
			else
				menu.EnableMenuItem (ID_SERVER_DROP, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
			//
			// SQL Test ...(+DOM, + Test Node)
			if (bSqlEnabled)
			{
				menu.EnableMenuItem (ID_NODE_SQLTEST, MF_BYCOMMAND|MF_ENABLED);
				menu.EnableMenuItem (ID_NODE_DOM, MF_BYCOMMAND|MF_ENABLED);
				menu.EnableMenuItem (ID_SERVER_TEST, MF_BYCOMMAND|MF_ENABLED);
			}
			else
			{
				menu.EnableMenuItem (ID_NODE_SQLTEST, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
				menu.EnableMenuItem (ID_NODE_DOM, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
				menu.EnableMenuItem (ID_SERVER_TEST, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
			}
			//
			// Test Node...
			if (bTestNodeEnabled)
				menu.EnableMenuItem (ID_SERVER_TEST, MF_BYCOMMAND|MF_ENABLED);
			else
				menu.EnableMenuItem (ID_SERVER_TEST, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
			//
			// Performance Monitor:
			if (bIPMEnabled)
				menu.EnableMenuItem (ID_NODE_MONITOR, MF_BYCOMMAND|MF_ENABLED);
			else
				menu.EnableMenuItem (ID_NODE_MONITOR, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
#else
			//
			// Installation password.
			if (bAddInstallPasswordEnabled)
				menu.EnableMenuItem (ID_NODE_ADD_INSTALLATION, MF_BYCOMMAND|MF_ENABLED);
			else
				menu.EnableMenuItem (ID_NODE_ADD_INSTALLATION, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
			//
			// Add ...
			if (bAddEnabled)
				menu.EnableMenuItem (ID_NODE_ADD, MF_BYCOMMAND|MF_ENABLED);
			else
				menu.EnableMenuItem (ID_NODE_ADD, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
			//
			// Alter ...
			if (bAlterEnabled)
				menu.EnableMenuItem (ID_NODE_ALTER, MF_BYCOMMAND|MF_ENABLED);
			else
				menu.EnableMenuItem (ID_NODE_ALTER, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
			//
			// Drop ...
			if (bDropEnabled)
				menu.EnableMenuItem (ID_NODE_DROP, MF_BYCOMMAND|MF_ENABLED);
			else
				menu.EnableMenuItem (ID_NODE_DROP, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
			//
			// SQL Test ...(+DOM, + Test Node)
			if (bSqlEnabled)
			{
				menu.EnableMenuItem (ID_NODE_SQLTEST, MF_BYCOMMAND|MF_ENABLED);
				menu.EnableMenuItem (ID_NODE_DOM, MF_BYCOMMAND|MF_ENABLED);
				menu.EnableMenuItem (ID_NODE_TEST, MF_BYCOMMAND|MF_ENABLED);
			}
			else
			{
				menu.EnableMenuItem (ID_NODE_SQLTEST, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
				menu.EnableMenuItem (ID_NODE_DOM, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
				menu.EnableMenuItem (ID_NODE_TEST, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
			}
			//
			// Test Node...
			if (bTestNodeEnabled)
				menu.EnableMenuItem (ID_NODE_TEST, MF_BYCOMMAND|MF_ENABLED);
			else
				menu.EnableMenuItem (ID_NODE_TEST, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
			//
			// Performance Monitor:
			if (bIPMEnabled)
				menu.EnableMenuItem (ID_NODE_MONITOR, MF_BYCOMMAND|MF_ENABLED);
			else
				menu.EnableMenuItem (ID_NODE_MONITOR, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
			//
			// Manage local vnode.
			if (bManageLocalVnodeEnabled)
				menu.EnableMenuItem (ID_POPUP_NODE_EDIT_VIEW, MF_BYCOMMAND|MF_ENABLED);
			else
				menu.EnableMenuItem (ID_POPUP_NODE_EDIT_VIEW, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);

#endif
		}
	}
#if defined (EDBC)
	menu.DeleteMenu (ID_NODE_ADD_INSTALLATION, MF_BYCOMMAND);
	menu.DeleteMenu (ID_NODE_MONITOR, MF_BYCOMMAND);
#endif
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, p.x, p.y, pWndPopupOwner);
}


void CvMainView::OnUpdateNodeAdd(CCmdUI* pCmdUI) 
{
	BOOL bAnable = FALSE;
	CTreeCtrl& cTree = GetTreeCtrl();
	HTREEITEM hSel = cTree.GetSelectedItem();
	if (hSel)
	{
		CaVNodeTreeItemData* pItem = (CaVNodeTreeItemData*)cTree.GetItemData (hSel);
		if (pItem)
			bAnable = pItem->IsAddEnabled();
	}
	pCmdUI->Enable (bAnable);
}

void CvMainView::OnUpdateNodeAlter(CCmdUI* pCmdUI) 
{
	BOOL bAnable = FALSE;
	CTreeCtrl& cTree = GetTreeCtrl();
	HTREEITEM hSel = cTree.GetSelectedItem();
	if (hSel)
	{
		CaVNodeTreeItemData* pItem = (CaVNodeTreeItemData*)cTree.GetItemData (hSel);
		if (pItem)
			bAnable = pItem->IsAlterEnabled();
	}
	pCmdUI->Enable (bAnable);
}

void CvMainView::OnUpdateNodeDrop(CCmdUI* pCmdUI) 
{
	BOOL bAnable = FALSE;
	CTreeCtrl& cTree = GetTreeCtrl();
	HTREEITEM hSel = cTree.GetSelectedItem();
	if (hSel)
	{
		CaVNodeTreeItemData* pItem = (CaVNodeTreeItemData*)cTree.GetItemData (hSel);
		if (pItem)
			bAnable = pItem->IsDropEnabled();
	}
	pCmdUI->Enable (bAnable);
}

void CvMainView::OnUpdateNodeSqltest(CCmdUI* pCmdUI) 
{
	BOOL bAnable = FALSE;
	if ( !theApp.m_pNodeAccess->IsVnodeOptionDefined() )
	{
		CTreeCtrl& cTree = GetTreeCtrl();
		HTREEITEM hSel = cTree.GetSelectedItem();
		if (hSel)
		{
			CaVNodeTreeItemData* pItem = (CaVNodeTreeItemData*)cTree.GetItemData (hSel);
			if (pItem)
				bAnable = pItem->IsSQLEnabled();
		}
	}
	pCmdUI->Enable (bAnable);
}

void CvMainView::OnNodeAddInstallation() 
{
	CWaitCursor doWaitCursor;
	CTreeCtrl& cTree = GetTreeCtrl();
	HTREEITEM hSelected = cTree.GetSelectedItem();
	if (!hSelected)
		return;
	CaVNodeTreeItemData* pItem = (CaVNodeTreeItemData*)cTree.GetItemData (hSelected);
	if (!pItem)
		return;
	BOOL bAdd = pItem->AddInstallation();
	if (bAdd)
	{
		//
		// Refresh Data:
		OnNodeRefresh();
	}
}

void CvMainView::OnNodeTest() 
{
	CWaitCursor doWaitCursor;
	char buf[ER_MAX_LEN];
	char* pBuf = buf;
	int buf_len = sizeof(buf);

	CTreeCtrl& cTree = GetTreeCtrl();
	HTREEITEM hSelected = cTree.GetSelectedItem();
	if (!hSelected)
		return;
	CaVNodeTreeItemData* pItem = (CaVNodeTreeItemData*)cTree.GetItemData (hSelected);
	if (!pItem)
		return;
	CString strNode = pItem->GetName();

#ifdef	EDBC
	if (!CheckLogin(strNode))
		return;

#endif
	STATUS nodeStatus = INGRESII_NodeCheckConnection(strNode);
	CString strMsg;
	if (nodeStatus == OK)
#if defined (EDBC)
		strMsg.Format(_T("Server test on %s was successful"), (LPCTSTR)strNode);
#else
		strMsg.Format(_T("Node test on %s is successful"), (LPCTSTR)strNode);
#endif
	else
	{
		INGRESII_ERLookup(nodeStatus, &pBuf, buf_len);
		strMsg.Format(_T("Node test on %s has failed"), (LPCTSTR)strNode);
		if (*buf != '\0')
		{
			strMsg+=" with error:\n\n\n\"";
			strMsg+=buf; strMsg+="\"";
		}
	}
	AfxMessageBox(strMsg, MB_OK | MB_ICONEXCLAMATION);
}

void CvMainView::OnUpdateNodeTest(CCmdUI* pCmdUI) 
{
	BOOL bAnable = FALSE;
	if ( !theApp.m_pNodeAccess->IsVnodeOptionDefined() )
	{
		CTreeCtrl& cTree = GetTreeCtrl();
		HTREEITEM hSel = cTree.GetSelectedItem();
		if (hSel)
		{
			CaVNodeTreeItemData* pItem = (CaVNodeTreeItemData*)cTree.GetItemData (hSel);
			if (pItem)
				bAnable = pItem->IsTestNodeEnabled();
		}
	}
	pCmdUI->Enable (bAnable);
}

void CvMainView::OnNodeDom() 
{
	BOOL bAnable = FALSE;
	BOOL bExecOK = FALSE;
	CTreeCtrl& cTree = GetTreeCtrl();
	HTREEITEM hSel = cTree.GetSelectedItem();
	if (hSel)
	{
		CaVNodeTreeItemData* pItem = (CaVNodeTreeItemData*)cTree.GetItemData (hSel);
		if (!pItem)
			return;
		CString strNode = pItem->GetName();
		CString strNode1 = strNode;
		if (strNode.Find(_T(' ')) != -1) 
			strNode1.Format (_T("\"%s\""), (LPCTSTR)strNode);
		strNode = strNode1;


		if (pItem->GetNodeType() == NODExLOCAL)
			bExecOK = VNODE_RunVdbaDom(_T(""));
		else
		if (pItem->GetNodeType() != NODExSERVER)
		{
#ifdef	EDBC
			if (!CheckLogin(strNode))
				return;
#endif
			bExecOK = VNODE_RunVdbaDom(strNode);
		}
		else
		{
			if (pItem->GetNodeType() == NODExSERVER)
			{
				CaIngnetNodeServer* pServer = (CaIngnetNodeServer*)pItem;
				if (pServer->IsLocal())
				{
					strNode.Format (_T(" /%s"), (LPCTSTR)pServer->GetName());
					bExecOK = VNODE_RunVdbaDom(strNode);
				}
				else
				{
					strNode = pServer->GetNodeName();
					strNode1 = strNode;
					if (strNode1.Find(_T(' ')) != -1) 
						strNode1.Format (_T("\"%s\""), (LPCTSTR)strNode);

					strNode.Format (_T("%s /%s"), (LPCTSTR)strNode1, (LPCTSTR)pServer->GetName());
					bExecOK = VNODE_RunVdbaDom(strNode);
				}
			}
		}
		if(!bExecOK)
		{
			CString strMsg = _T("Cannot run Database Object Management (DOM) Window.\n");
			TRACE0(strMsg);
		}
	}
}

void CvMainView::OnUpdateNodeDom(CCmdUI* pCmdUI) 
{
	BOOL bAnable = FALSE;
	if ( !theApp.m_pNodeAccess->IsVnodeOptionDefined() )
	{
		CTreeCtrl& cTree = GetTreeCtrl();
		HTREEITEM hSel = cTree.GetSelectedItem();
		if (hSel)
		{
			CaVNodeTreeItemData* pItem = (CaVNodeTreeItemData*)cTree.GetItemData (hSel);
			if (pItem)
				bAnable = pItem->IsDOMEnabled();
		}
	}
	pCmdUI->Enable (bAnable);
}

void CvMainView::OnNodeMonitor() 
{
	BOOL bAnable = FALSE;
	BOOL bExecOK = FALSE;
	CTreeCtrl& cTree = GetTreeCtrl();
	HTREEITEM hSel = cTree.GetSelectedItem();
	if (hSel)
	{
		CaVNodeTreeItemData* pItem = (CaVNodeTreeItemData*)cTree.GetItemData (hSel);
		if (!pItem)
			return;
		CString strNode = pItem->GetName();
		CString strNode1 = strNode;
		if (strNode.Find(_T(' ')) != -1) 
			strNode1.Format (_T("\"%s\""), (LPCTSTR)strNode);
		strNode = strNode1;

		if (pItem->GetNodeType() != NODExSERVER)
		{
			bExecOK = VNODE_RunVdbaIpm(strNode);
		}
		else
		{
			if (pItem->GetNodeType() == NODExSERVER)
			{
				CaIngnetNodeServer* pServer = (CaIngnetNodeServer*)pItem;
				strNode = pServer->GetNodeName();
				strNode1 = strNode;
				if (strNode1.Find(_T(' ')) != -1) 
					strNode1.Format (_T("\"%s\""), (LPCTSTR)strNode);

				strNode.Format (_T("%s /%s"), (LPCTSTR)strNode1, (LPCTSTR)pServer->GetName());
				bExecOK = VNODE_RunVdbaIpm(strNode);
			}
		}
		if(!bExecOK)
		{
			CString strMsg = _T("Cannot run Performance Monitor Window.\n");
			TRACE0(strMsg);
		}
	}
}

void CvMainView::OnUpdateNodeMonitor(CCmdUI* pCmdUI) 
{
	BOOL bAnable = FALSE;
	if ( !theApp.m_pNodeAccess->IsVnodeOptionDefined() )
	{
		CTreeCtrl& cTree = GetTreeCtrl();
		HTREEITEM hSel = cTree.GetSelectedItem();
		if (hSel)
		{
			CaVNodeTreeItemData* pItem = (CaVNodeTreeItemData*)cTree.GetItemData (hSel);
			if (pItem)
				bAnable = pItem->IsIPMEnabled();
		}
	}
	pCmdUI->Enable (bAnable);
}

void CvMainView::OnUpdateNodeAddInstallation(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	CTreeCtrl& cTree = GetTreeCtrl();
	HTREEITEM hSel = cTree.GetSelectedItem();
	if (hSel)
	{
		CaVNodeTreeItemData* pItem = (CaVNodeTreeItemData*)cTree.GetItemData (hSel);
		switch (pItem->GetNodeType())
		{
		case NODExLOCAL:
			if (!HasInstallationPassword(FALSE))
				bEnable = TRUE;
			break;
		case NODExFOLDER:
			if ( pItem->GetName().IsEmpty() && !HasInstallationPassword(FALSE))
				bEnable = TRUE;
			break;
		
		}
	}
	pCmdUI->Enable (bEnable);
}


#ifdef	EDBC
BOOL CheckLogin(CString strNode)
{
	CTypedPtrList<CObList, CaVNodeTreeItemData*> listLogin;
	POSITION pos;
	BOOL bSave = TRUE;
	CString Login;
	CaIngnetNodeLogin* pLogin = NULL;

	VNODE_QueryLogin (strNode, listLogin);
	pos = listLogin.GetHeadPosition();
	if (pos != NULL)
	{
		pLogin = (CaIngnetNodeLogin*)listLogin.GetNext (pos);
		bSave = pLogin->IsSave();
		Login = pLogin->GetName ();
	}
	if (!bSave && Login.IsEmpty())
	{
		/*
		** Prompt the user to enter user name and password 
		** if they have not been entered already.
		*/
		if (!pLogin->Enter())
			return FALSE;
	}
	return TRUE;
}
#endif

void CvMainView::OnNodeEditView() 
{
	BOOL bExecOK = FALSE;
	CTreeCtrl& cTree = GetTreeCtrl();
	HTREEITEM hSel = cTree.GetSelectedItem();
	if (hSel)
	{
		CaVNodeTreeItemData* pItem = (CaVNodeTreeItemData*)cTree.GetItemData (hSel);
		if (!pItem)
			return;
		if (pItem->GetNodeType() != NODExNORMAL)
			return;
		CString strNode;
		strNode = INGRESII_llQuoteIfNeeded(pItem->GetName());
		bExecOK = VNODE_RunIngNetLocalVnode(strNode);
		if(!bExecOK)
		{
			CString strMsg = _T("Cannot run Ingnet -vnode Window.\n");
			TRACE0(strMsg);
		}
	}
}

void CvMainView::OnUpdateNodeEditView(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	if ( !theApp.m_pNodeAccess->IsVnodeOptionDefined() ) // -vnode option not used at startup
	{
		CTreeCtrl& cTree = GetTreeCtrl();
		HTREEITEM hSel = cTree.GetSelectedItem();
		if (hSel)
		{
			CaVNodeTreeItemData* pItem = (CaVNodeTreeItemData*)cTree.GetItemData (hSel);
			if (pItem->GetNodeType() == NODExNORMAL)
				bEnable = TRUE;
		}
	}
	pCmdUI->Enable (bEnable);
}

void CvMainView::OnManageLocalvnode() 
{
	OnNodeEditView();
}

void CvMainView::OnUpdateManageLocalvnode(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	if ( !theApp.m_pNodeAccess->IsVnodeOptionDefined() ) // -vnode option not used at startup
	{
		CTreeCtrl& cTree = GetTreeCtrl();
		HTREEITEM hSel = cTree.GetSelectedItem();
		if (hSel)
		{
			CaVNodeTreeItemData* pItem = (CaVNodeTreeItemData*)cTree.GetItemData (hSel);
			if (pItem->GetNodeType() == NODExNORMAL)
				bEnable = TRUE;
		}
	}
	pCmdUI->Enable (bEnable);
}

void CvMainView::OnPopupNodeEditView() 
{
	OnNodeEditView();
}
