/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : viewleft.cpp , Implementation File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : MFC (Frame View Doc) Tree View of Node, Database, Table
**
** History:
**
** 22-Dec-1999 (uk$so01)
**    Created
** 14-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management and put string into the resource files
** 21-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management.
** 26-Jul-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management. Replace the ID_HELP_GENERAL by 0 (Help Finder)
**    Appearantly, ID_HELP_GENERAL = 1000 is not defined in the ija.hlp.
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
** 16-Dec-2003 (schph01)
**    SIR #111462 Removed obsolete code that was including hardcoded strings
**    (rather than putting the string into resources)
** 05-Mar-2010 (drivi01)
**    Add error when user attempts to expand or view a vectorwise database.
**    VectorWise is not journaled and therefore shouldn't allow users
**    to view journals.
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "viewleft.h"
#include "viewrigh.h"
#include "mainfrm.h"
#include "ijadml.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// REMOVE THIS DEFINE WHEN AUDITDB ACCEPT the table of the form OWNER.TABLE. 
//
// Workaround: actually, auditdb does not accept the option -table = owrner.table.
// So if there are mutiple table names in the same database, then we do not allow
// to select this table:
//#define WORK_AROUND_MULIPLEOWNER

/////////////////////////////////////////////////////////////////////////////
// CvViewLeft

IMPLEMENT_DYNCREATE(CvViewLeft, CTreeView)

CvViewLeft::CvViewLeft()
{
	m_bInitialize = FALSE;
	m_hHitRButton = NULL;
}

CvViewLeft::~CvViewLeft()
{
}


BEGIN_MESSAGE_MAP(CvViewLeft, CTreeView)
	//{{AFX_MSG_MAP(CvViewLeft)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED, OnItemexpanded)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	ON_WM_RBUTTONDOWN()
	ON_COMMAND(IDM_REFRESH_CURRENT, OnRefreshCurrent)
	ON_COMMAND(IDM_REFRESH_CURRENT_SUB, OnRefreshCurrentSub)
	ON_NOTIFY_REFLECT(TVN_SELCHANGING, OnSelchanging)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnItemexpanding)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvViewLeft drawing

void CvViewLeft::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
}

/////////////////////////////////////////////////////////////////////////////
// CvViewLeft diagnostics

#ifdef _DEBUG
void CvViewLeft::AssertValid() const
{
	CTreeView::AssertValid();
}

void CvViewLeft::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvViewLeft message handlers

void CvViewLeft::OnInitialUpdate() 
{
	CTreeView::OnInitialUpdate();
	
	if (!m_bInitialize)
	{
		CTreeCtrl& cTree = GetTreeCtrl();
		m_ImageList.Create (IDB_INGRESOBJECT16x16, 16, 0, RGB(255,0,255));
		cTree.SetImageList (&m_ImageList, TVSIL_NORMAL);

		if (theApp.m_cmdLine.m_nArgs == 0 || theApp.m_cmdLine.m_nArgs == -1)
			m_treeData.Display (&cTree);
		else
		{
			//
			// Display the special tree according to the input command line:
			m_treeData.SetDisplaySpecial(theApp.m_cmdLine.m_nArgs);
			switch (theApp.m_cmdLine.m_nArgs)
			{
			case 1: // Node level
				{
					CaIjaNode& node = m_treeData.GetSpecialRootNode();
					node.SetItem (theApp.m_cmdLine.m_strNode);
					node.LocalNode ();
					node.Display (&cTree, cTree.GetRootItem());
				}
				break;

			case 2: // Database level
				{
					CaIjaNode& node = m_treeData.GetSpecialRootNode();
					node.SetItem (theApp.m_cmdLine.m_strNode);
					node.LocalNode ();

					CaIjaDatabase& database = m_treeData.GetSpecialRootDatabase();
					database.SetItem(theApp.m_cmdLine.m_strDatabase);
					database.Display (&cTree, cTree.GetRootItem());
				}
				break;

			case 3: // Table level
				{
					CaIjaNode& node = m_treeData.GetSpecialRootNode();
					node.SetItem (theApp.m_cmdLine.m_strNode);
					node.LocalNode ();

					CaIjaDatabase& database = m_treeData.GetSpecialRootDatabase();
					database.SetItem(theApp.m_cmdLine.m_strDatabase);

					CaIjaTable& table = m_treeData.GetSpecialRootTable();
					table.SetItem     (theApp.m_cmdLine.m_strTable);
					table.SetOwner    (theApp.m_cmdLine.m_strTableOwner);
					table.Display (&cTree, cTree.GetRootItem());
				}
				break;

			default:
				ASSERT (FALSE);
				break;
			}
		}

		m_treeData.SortFolder (&cTree);
		HTREEITEM hRoot = cTree.GetRootItem();
		if (hRoot)
			cTree.SelectItem (hRoot);
		m_bInitialize = TRUE;
	}
}

BOOL CvViewLeft::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.style |= TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS|TVS_SHOWSELALWAYS;
	return CTreeView::PreCreateWindow(cs);
}

void CvViewLeft::OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	*pResult = 0;
	HTREEITEM hExpand = pNMTreeView->itemNew.hItem;
	CTreeCtrl& cTree = GetTreeCtrl();
	try
	{
		CaIjaTreeItemData* pItem = (CaIjaTreeItemData*)cTree.GetItemData (hExpand);
		if (!pItem)
			return;
		if (pNMTreeView->action & TVE_COLLAPSE)
		{
			//
			// Collapseing:
			pItem->Collapse (&cTree, hExpand, &m_treeData);
		}
		else
		if (pNMTreeView->action & TVE_EXPAND)
		{
			//
			// Expanding:
			CWaitCursor doWaitCursor;
			pItem->Expand (&cTree, hExpand, &m_treeData);
			TV_SORTCB sortcb;

			sortcb.hParent = hExpand;
			sortcb.lpfnCompare = CaIjaTreeData::fnCompareComponent;
			sortcb.lParam = NULL;
			cTree.SortChildrenCB (&sortcb);
		}
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason());
	}
	catch(...)
	{
		TRACE0 ("System error: CvViewLeft::OnItemexpanded\n");
	}
}

void CvViewLeft::SetPaneDatabase(CTreeCtrl* pTree, HTREEITEM hSelected, CaIjaTreeItemData* pItem)
{
	CfMainFrame* pm = (CfMainFrame*)theApp.m_pMainWnd;
	ASSERT (pm);
	if (!pm)
		return;
	CvViewRight* pV = (CvViewRight*)pm->GetRightView();
	ASSERT (pV);
	if (!pV)
		return;
	if (!pV->m_pCtrl)
		return;
	if (!IsWindow (pV->m_pCtrl->m_hWnd))
		return;

	CaIjaNode* pParentItem = (CaIjaNode*)pItem->GetBackParent();
	if (pParentItem)
	{
		ASSERT (pParentItem->GetItemType() == ITEM_NODE);
		if (pParentItem->GetItemType() == ITEM_NODE)
		{
			pV->m_pCtrl->SetMode (0);
			CTypedPtrList<CObList, CaUser*> listUser;
			if (IJA_QueryUser (pParentItem->GetItem(), listUser))
			{
				POSITION pos = listUser.GetHeadPosition();
				pV->m_pCtrl->SetAllUserString(theApp.m_strAllUser);
				pV->m_pCtrl->CleanUser();
				pV->m_pCtrl->AddUser (theApp.m_strAllUser);
				pV->m_pCtrl->SetUserPos (0);

				while (pos != NULL)
				{
					CaUser* pUser = listUser.GetNext (pos);
					pV->m_pCtrl->AddUser (pUser->GetItem());
					delete pUser;
				}
			}

			pV->m_pCtrl->RefreshPaneDatabase(pParentItem->GetItem(), pItem->GetItem(), pItem->GetOwner());
		}
	}
}


void CvViewLeft::SetPaneTable(CTreeCtrl* pTree, HTREEITEM hSelected, CaIjaTreeItemData* pItem)
{
	CfMainFrame* pm = (CfMainFrame*)theApp.m_pMainWnd;
	ASSERT (pm);
	if (!pm)
		return;
	CvViewRight* pV =  (CvViewRight*)pm->GetRightView();
	ASSERT (pV);
	if (!pV)
		return;
	if (!pV->m_pCtrl)
		return;
	if (!IsWindow (pV->m_pCtrl->m_hWnd))
		return;

	CString strDatabase = _T(""); 
	CaIjaDatabase* pParentItem1 = (CaIjaDatabase*)pItem->GetBackParent();
	if (pParentItem1)
	{
		ASSERT (pParentItem1->GetItemType() == ITEM_DATABASE);
		if (pParentItem1->GetItemType() == ITEM_DATABASE)
		{
			strDatabase = pParentItem1->GetItem();
		}
	}
	if (strDatabase.IsEmpty())
		return;

	CaIjaNode* pParentItem2 = (CaIjaNode*)pParentItem1->GetBackParent();
	if (pParentItem2)
	{
		ASSERT (pParentItem2->GetItemType() == ITEM_NODE);
		if (pParentItem2->GetItemType() == ITEM_NODE)
		{
			pV->m_pCtrl->SetMode (1);
			CTypedPtrList<CObList, CaUser*> listUser;
			if (IJA_QueryUser (pParentItem2->GetItem(), listUser))
			{
				POSITION pos = listUser.GetHeadPosition();
				pV->m_pCtrl->SetAllUserString(theApp.m_strAllUser);
				pV->m_pCtrl->CleanUser();
				pV->m_pCtrl->AddUser (theApp.m_strAllUser);
				pV->m_pCtrl->SetUserPos (0);
				while (pos != NULL)
				{
					CaUser* pUser = listUser.GetNext (pos);
					pV->m_pCtrl->AddUser (pUser->GetItem());
					delete pUser;
				}
			}
			pV->m_pCtrl->RefreshPaneTable(pParentItem2->GetItem(), strDatabase, pParentItem1->GetOwner(), pItem->GetItem(), pItem->GetOwner());
		}
	}
}

void CvViewLeft::SetPaneNull(CTreeCtrl* pTree, HTREEITEM hSelected, CaIjaTreeItemData* pItem)
{
	CfMainFrame* pm = (CfMainFrame*)theApp.m_pMainWnd;
	ASSERT (pm);
	if (!pm)
		return;
	CvViewRight* pV = (CvViewRight*)pm->GetRightView();
	ASSERT (pV);
	if (!pV)
		return;
	if (!pV->m_pCtrl)
		return;
	if (!IsWindow (pV->m_pCtrl->m_hWnd))
		return;
	pV->m_pCtrl->SetMode (-1);
}


void CvViewLeft::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CWaitCursor doWaitCursor;
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	*pResult = 0;

	HTREEITEM hItem = pNMTreeView->itemNew.hItem;
	CTreeCtrl& cTree = GetTreeCtrl();
	try
	{
		CaIjaTreeItemData* pItem = (CaIjaTreeItemData*)cTree.GetItemData (hItem);
		if (!pItem)
			return;

		if (hItem != NULL)
		{
			int nType = pItem->GetItemType();
			switch (nType)
			{
			case ITEM_DATABASE:
				SetPaneDatabase(&cTree, hItem, pItem);
				break;
			case ITEM_TABLE:
				SetPaneTable(&cTree, hItem, pItem);
				break;
			default:
				SetPaneNull(&cTree, hItem, pItem);
				break;
			}
			CfMainFrame* pMainFrame = (CfMainFrame*)GetParentFrame();
			if (pMainFrame)
			{
				CuUntrackableSplitterWnd& splitterwnd = pMainFrame->GetSplitterWnd();
				splitterwnd.SetActivePane( 0, 1);
			}
		}
	}
	catch(...)
	{
		TRACE0 ("System error: CvViewLeft::OnSelchanged\n");
	}
}

void CvViewLeft::OnRButtonDown(UINT nFlags, CPoint point) 
{
	CTreeView::OnRButtonDown(nFlags, point);
	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_TREELEFT_POPUP));

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

	CaIjaTreeItemData* pItem = NULL;
	CTreeCtrl& cTree = GetTreeCtrl();
	UINT nFlag = TVHT_ONITEM;
	m_hHitRButton = cTree.HitTest (point, &nFlag);
	if (m_hHitRButton)
	{
		pItem = (CaIjaTreeItemData*)cTree.GetItemData (m_hHitRButton);
		if (pItem)
		{
			BOOL bRefreshCurrentBranch = pItem->IsRefreshCurrentBranchEnabled();
			BOOL bRefreshCurrentSubBranch = pItem->IsRefreshCurrentSubBranchEnabled();
			//
			// Current Branch:
			if (bRefreshCurrentBranch)
				menu.EnableMenuItem (IDM_REFRESH_CURRENT, MF_BYCOMMAND|MF_ENABLED);
			else
				menu.EnableMenuItem (IDM_REFRESH_CURRENT, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
			//
			// Current And Sub-Branch:
			if (bRefreshCurrentSubBranch)
				menu.EnableMenuItem (IDM_REFRESH_CURRENT_SUB, MF_BYCOMMAND|MF_ENABLED);
			else
				menu.EnableMenuItem (IDM_REFRESH_CURRENT_SUB, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
		}
	}
	if (m_hHitRButton)
		cTree.SelectItem (m_hHitRButton);
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, p.x, p.y, pWndPopupOwner);
}


void CvViewLeft::RefreshData(RefreshType Mode)
{
	CWaitCursor doWaitCursor;
	CaRefreshTreeInfo info;
	CaIjaTreeItemData* pItem = NULL;
	CTreeCtrl& cTree = GetTreeCtrl();
	HTREEITEM hSel = cTree.GetSelectedItem();
	if (!hSel)
		return;
	pItem = (CaIjaTreeItemData*)cTree.GetItemData (hSel);
	if (!pItem)
		return;
	int nType = pItem->GetItemType();

	switch (Mode)
	{
	case REFRESH_CURRENT:
		info.SetInfo (0);
		pItem->RefreshData (&cTree, hSel, &info);
		break;
	case REFRESH_CURRENT_SUB:
		info.SetInfo (1);
		pItem->RefreshData (&cTree, hSel, &info);
		break;
	case REFRESH_CURRENT_ALL:
		info.SetInfo (-1);
		pItem->RefreshData (&cTree, hSel, &info);
		break;
	default:
		break;
	}
	m_treeData.SortFolder (&cTree);
}

void CvViewLeft::OnRefreshCurrent() 
{
	RefreshData(REFRESH_CURRENT);
}

void CvViewLeft::OnRefreshCurrentSub() 
{
	RefreshData(REFRESH_CURRENT_SUB);
}



void CvViewLeft::OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	HTREEITEM hItem = pNMTreeView->itemNew.hItem;
	CTreeCtrl& cTree = GetTreeCtrl();
	*pResult = 0;

	try
	{
		CaIjaTreeItemData* pItem = (CaIjaTreeItemData*)cTree.GetItemData (hItem);
		if (!pItem)
			return;
		HTREEITEM hParent = cTree.GetParentItem(hItem);

		if (hItem != NULL)
		{
			int nType = pItem->GetItemType();
			switch (nType)
			{
			case ITEM_DATABASE:
				if (((CaIjaDatabase*)pItem)->GetStar() == OBJTYPE_STARLINK)
				{
					AfxMessageBox (theApp.m_strMsgErrorDBStar, MB_ICONEXCLAMATION|MB_OK);
					*pResult = 1;
				}
				if (((CaIjaDatabase *)pItem)->GetVW())
				{
					AfxMessageBox(IDS_NOJNL_VW, MB_ICONEXCLAMATION|MB_OK);
					*pResult = 1;
				}
				break;
			default:
				break;
			}
		}
	}
	catch(...)
	{
		TRACE0 ("System error: CvViewLeft::OnSelchanged\n");
	}
}

void CvViewLeft::OnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	*pResult = 0;

	HTREEITEM hExpand = pNMTreeView->itemNew.hItem;
	CTreeCtrl& cTree = GetTreeCtrl();
	try
	{
		CaIjaTreeItemData* pItem = (CaIjaTreeItemData*)cTree.GetItemData (hExpand);
		if (!pItem)
			return;
		if (pNMTreeView->action & TVE_EXPAND)
		{
			if (pItem->GetItemType() == ITEM_DATABASE)
			{
				if (((CaIjaDatabase*)pItem)->GetStar() == OBJTYPE_STARLINK )
				{
					AfxMessageBox (theApp.m_strMsgErrorDBStar, MB_ICONEXCLAMATION|MB_OK);
					*pResult = 1;
				}
				if (((CaIjaDatabase *)pItem)->GetVW())
				{
					AfxMessageBox(IDS_NOJNL_VW, MB_ICONEXCLAMATION|MB_OK);
					*pResult = 1;
				}
			}
		}
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason());
	}
	catch(...)
	{
		TRACE0 ("System error: CvViewLeft::OnItemexpanded\n");
	}

}

void CvViewLeft::ShowHelp()
{
	CfMainFrame* pm = (CfMainFrame*)theApp.m_pMainWnd;
	ASSERT (pm);
	if (!pm)
		return;
	CvViewRight* pV =  (CvViewRight*)pm->GetRightView();
	ASSERT (pV);
	if (!pV)
		return;
	if (!pV->m_pCtrl)
		return;
	if (!IsWindow (pV->m_pCtrl->m_hWnd))
		return;
	long lHelpID = pV->m_pCtrl->ShowHelp();
	if (lHelpID == -1)
	{
		//
		// When there is neither database nor table has been selected:
		APP_HelpInfo (m_hWnd, 0);
	}
}


