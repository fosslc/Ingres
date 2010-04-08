/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ijatree.cpp , Implementation File 
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Manipulation tree data of the left pane
**
** History:
**
** 14-Dec-1999 (uk$so01)
**    Created
** 21-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management.
** 01-Jul-2003 (uk$so01)
**    BUG #110506/INGDBA231 .After refreshing the list of nodes, all nodes have
**    to be passed to the ijactrl.ocx because its list of old nodes has been 
**    made empty.
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
** 30-Oct-2003 (noifr01)
**    trailing } character had been lost, probably due to previous cross-
**    integration
** 16-Dec-2003 (schph01)
**    SIR #111462, Put string into resource files
** 22-Nov-2004 (schph01)
**    BUG #113511 replace CompareNoCase function by CollateNoCase
**    according to the LC_COLLATE category setting of the locale code
**    page currently in use.
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "ijatree.h"
#include "ijadml.h"
#include "mainfrm.h"
#include "viewrigh.h"
#include "vnodedml.h"

//
// If this constant 'MULTIPLE_TABLEOWNER' is defined, then multiple tables in the database are
// displayed with OWNER prefixed !!
// #define MULTIPLE_TABLEOWNER

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class  CaPathDescription
{
public:
	CaPathDescription();
	~CaPathDescription(){}

	int m_nLevel;
	CString m_strP1;
	CString m_strP2;
	CString m_strP3;
};

CaPathDescription::CaPathDescription()
{
	m_nLevel= 0;
	m_strP1 = _T("");
	m_strP2 = _T("");
	m_strP3 = _T("");
}

static CaTreeCtrlData* IJA_FindTreePath (CTreeCtrl* pTree, CaPathDescription* pDcs)
{
	CaIjaTreeItemData* pFound = NULL;
	HTREEITEM hItem = pTree->GetRootItem();
	hItem = pTree->GetNextItem(hItem, TVGN_CHILD);
	int nLevel = pDcs->m_nLevel;
	int nCurLevel = 1;
	if (hItem)
	{
		while (hItem && nCurLevel <= nLevel)
		{
			CaIjaTreeItemData* pItem = (CaIjaTreeItemData*)pTree->GetItemData(hItem);
			switch (nCurLevel)
			{
			case 1:
				if (!pDcs->m_strP1.IsEmpty() && pDcs->m_strP1.CompareNoCase (pItem->GetItem()) == 0)
				{
					pFound = pItem;
					if (nCurLevel == nLevel)
						return &(pFound->GetTreeCtrlData());
					if (nCurLevel < nLevel)
					{
						hItem = pTree->GetNextItem(hItem, TVGN_CHILD);
						nCurLevel++;
						break;
					}
					break;
				}
				else
				{
					hItem = pTree->GetNextItem(hItem, TVGN_NEXT);
				}
				break;
			case 2:
				if (!pDcs->m_strP2.IsEmpty() && pDcs->m_strP2.CompareNoCase (pItem->GetItem()) == 0)
				{
					pFound = pItem;
					if (nCurLevel == nLevel)
						return &(pFound->GetTreeCtrlData());
					if (nCurLevel < nLevel)
					{
						hItem = pTree->GetNextItem(hItem, TVGN_CHILD);
						nCurLevel++;
						break;
					}
					pFound = pItem;
					break;
				}
				else
				{
					hItem = pTree->GetNextItem(hItem, TVGN_NEXT);
				}
				break;
			default:
				pFound = NULL;
				break;
			}
		}
	}
	return NULL;
}

static CaIjaTreeItemData* IJA_FindItem (CTypedPtrList<CObList, CaIjaTreeItemData*>& listItem, CaIjaTreeItemData* pItem)
{
	POSITION pos = listItem.GetHeadPosition();
	while (pos != NULL)
	{
		CaIjaTreeItemData* pExit = listItem.GetNext (pos);
		if (pExit->GetItem().CompareNoCase (pItem->GetItem()) == 0 && pExit->GetOwner().CompareNoCase (pItem->GetOwner()) == 0)
			return pExit;
	}
	return NULL;
}


static HTREEITEM IJA_TreeAddItem (LPCTSTR lpszItem, CTreeCtrl* pTree, HTREEITEM hParent, HTREEITEM hInsertAfter, int nImage, DWORD dwData)
{
	TV_INSERTSTRUCT tvins; 
	memset (&tvins, 0, sizeof (tvins));
	tvins.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	//
	// Set the text of the item. 
	tvins.item.pszText = (LPTSTR)lpszItem; 
	tvins.item.cchTextMax = _tcslen(lpszItem)+ sizeof (TCHAR); 
	
	tvins.item.iImage = nImage;
	tvins.item.iSelectedImage = nImage; 
	tvins.hInsertAfter = hInsertAfter;
	tvins.hParent = hParent;
	
	HTREEITEM hItem = pTree->InsertItem(&tvins);
	if (hItem)
		pTree->SetItemData (hItem, dwData);
	return hItem; 
}

void CaIjaTreeItemData::Display (CTreeCtrl* pTree, HTREEITEM hParent)
{
	HTREEITEM hItem = m_treeCtrlData.GetTreeItem();
	CString strDisplay = m_strItem;
	if (hItem == NULL)
	{
		hItem = IJA_TreeAddItem (strDisplay, pTree, hParent, TVI_SORT, m_treeCtrlData.GetImage(), (DWORD)this);
		m_treeCtrlData.SetTreeItem(hItem);
	}
	else
	{
		pTree->SetItemText (hItem, strDisplay);
	}
}




CaIjaTable::CaIjaTable():CaIjaTreeItemData()
{
	m_treeCtrlData.SetImage (IM_FOLDER_TABLE, IM_FOLDER_TABLE);
}

CaIjaTable::CaIjaTable(LPCTSTR lpszItem, LPCTSTR lpszOwner):CaIjaTreeItemData(lpszItem, lpszOwner)
{
	m_treeCtrlData.SetImage (IM_FOLDER_TABLE, IM_FOLDER_TABLE);
}

void CaIjaTable::Display (CTreeCtrl* pTree, HTREEITEM hParent)
{
	HTREEITEM hItem = m_treeCtrlData.GetTreeItem();
	BOOL bMultiple = FALSE;
	CString strDisplay;
#if defined (MULTIPLE_TABLEOWNER)
	//
	// Find if this Item exists in the Tree:
	if (pTree->ItemHasChildren(hParent))
	{
		HTREEITEM hT1 = pTree->GetChildItem(hParent);
		while (hT1)
		{
			CaIjaTreeItemData* pData = (CaIjaTreeItemData*)pTree->GetItemData(hT1);
			if (pData && pData != this)
			{
				if (pData->GetItem().CompareNoCase(m_strItem) == 0)
				{
					if (!pData->GetOwner().IsEmpty())
						AfxFormatString2 (strDisplay, IDS_OWNERxITEM, (LPCTSTR)pData->GetOwner(), (LPCTSTR)pData->GetItem());
					else
						strDisplay = pData->GetItem();
					pTree->SetItemText (hT1, strDisplay);
					bMultiple = TRUE;
					break;
				}
			}
			hT1 = pTree->GetNextItem (hT1, TVGN_NEXT);
		}
	}
	
	if (bMultiple)
		AfxFormatString2 (strDisplay, IDS_OWNERxITEM, (LPCTSTR)m_strOwner, (LPCTSTR)m_strItem);
	else
		strDisplay = m_strItem;

	if (hItem == NULL)
	{
		hItem = IJA_TreeAddItem (strDisplay, pTree, hParent, TVI_SORT, m_treeCtrlData.GetImage(), (DWORD)this);
		m_treeCtrlData.SetTreeItem(hItem);
		GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_EXIST);
	}
	else
	{
		pTree->SetItemText (hItem, strDisplay);
	}
#else
	if (hItem == NULL)
	{
		if (!m_strItemOwner.IsEmpty())
			AfxFormatString2 (strDisplay, IDS_OWNERxITEM, (LPCTSTR)m_strItemOwner, (LPCTSTR)m_strItem);
		else
			strDisplay = m_strItem;
		hItem = IJA_TreeAddItem (strDisplay, pTree, hParent, TVI_LAST, m_treeCtrlData.GetImage(), (DWORD)this);
		m_treeCtrlData.SetTreeItem(hItem);
	}
#endif
}




CaIjaDatabase::CaIjaDatabase():CaIjaTreeItemData(), m_EmptyTable(_T("<No Tables>"))
{
	CString csItem;
	csItem.LoadString(IDS_NO_TABLES);
	m_EmptyTable.SetItem(csItem);
	m_treeCtrlData.SetImage (IM_FOLDER_DATABASE, IM_FOLDER_DATABASE);
	m_EmptyTable.GetTreeCtrlData().SetImage (IM_FOLDER_EMPTY, IM_FOLDER_EMPTY);
	m_pBackParent = NULL;
	m_nStar = OBJTYPE_NOTSTAR;
}

CaIjaDatabase::CaIjaDatabase(LPCTSTR lpszItem, LPCTSTR lpszOwner):CaIjaTreeItemData(lpszItem, lpszOwner), m_EmptyTable(_T("<No Tables>")) 
{
	CString csItem;
	csItem.LoadString(IDS_NO_TABLES);
	m_EmptyTable.SetItem(csItem);
	m_treeCtrlData.SetImage (IM_FOLDER_DATABASE, IM_FOLDER_DATABASE);
	m_EmptyTable.GetTreeCtrlData().SetImage (IM_FOLDER_EMPTY, IM_FOLDER_EMPTY);
	m_pBackParent = NULL;
	m_nStar = OBJTYPE_NOTSTAR;
}

void CaIjaDatabase::SetStar(int nStar)
{
	m_nStar = nStar;
	switch (m_nStar)
	{
	case OBJTYPE_STARNATIVE:
		m_treeCtrlData.SetImage (IM_FOLDER_DATABASE_STAR, IM_FOLDER_DATABASE_STAR);
		break;
	case OBJTYPE_STARLINK:
		m_treeCtrlData.SetImage (IM_FOLDER_DATABASE_STAR_LINK, IM_FOLDER_DATABASE_STAR_LINK);
		break;
	default:
		break;
	}
}


CaIjaDatabase::~CaIjaDatabase()
{
	while (!m_listTable.IsEmpty())
		delete m_listTable.RemoveHead();
}

void CaIjaDatabase::Expand (CTreeCtrl* pTree, HTREEITEM hExpand, CaIjaTreeData* pTreeData)
{
	BOOL bAlreadyExpanded = m_treeCtrlData.IsAlreadyExpanded();
	CaIjaTreeItemData::Expand(pTree, hExpand, pTreeData);
	if (!bAlreadyExpanded)
	{
		CString csItem;
		csItem.LoadString(IDS_NO_TABLES);
		m_EmptyTable.SetItem(csItem);
		CaRefreshTreeInfo info;
		info.SetInfo (0);
		RefreshData (pTree, hExpand, &info);
	}
}

BOOL CaIjaDatabase::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	CTypedPtrList<CObList, CaIjaTreeItemData*> listTable;
	CaIjaNode* pNode = (CaIjaNode*)m_pBackParent;
	if (!pNode)
		return FALSE;
	BOOL bError = FALSE;
	try
	{
		//
		// Refresh only the branch has already been expanded:
		if (!GetTreeCtrlData().IsAlreadyExpanded())
			return TRUE;

		if (!IJA_QueryTable (pNode->GetItem(), m_strItem, listTable, m_nStar))
			return FALSE;
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason());
		bError = TRUE;
	}
	catch (...)
	{
		AfxMessageBox (_T("System error::IJA_QueryTable, failed to query tables."));
		bError = TRUE;
	}

	if (bError)
	{
		CString csItem;
		csItem.LoadString(IDS_DATA_UNAVAILABLE);
		m_EmptyTable.SetItem(csItem);
		if (pTree && hItem)
			Display (pTree, m_treeCtrlData.GetTreeItem());
		return FALSE;
	}

	CTypedPtrList<CObList, CaIjaTreeItemData*>& listOldTable = m_listTable;
	//
	// Mark all old tables to be deleted:
	POSITION pos = listOldTable.GetHeadPosition();
	while (pos != NULL)
	{
		CaIjaTreeItemData* pItem = listOldTable.GetNext (pos);
		pItem->GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_DELETE);
	}
	//
	// Add new Tables:
	pos = listTable.GetHeadPosition();
	while (pos != NULL)
	{
		CaIjaTreeItemData* pItem = listTable.GetNext (pos);
		CaIjaTreeItemData* pExist = IJA_FindItem (listOldTable, pItem);
		if (pExist)
		{
			pExist->GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_EXIST);
			delete pItem;
			continue;
		}

		pItem->GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_NEW);
		pItem->SetBackParent (this);
		listOldTable.AddTail (pItem);
	}
	//
	// Update display the branches of path <Node>::<Database>:
	if (pTree && hItem)
		Display (pTree, pTree->GetParentItem(hItem));
	return TRUE;
}

void CaIjaDatabase::Display (CTreeCtrl* pTree, HTREEITEM hParent)
{
	HTREEITEM hItem = m_treeCtrlData.GetTreeItem();
	//
	// Create this tree item of database:
	if (hItem == NULL)
	{
		CString strDisplay = m_strItem;
		hItem = IJA_TreeAddItem (strDisplay, pTree, hParent, TVI_SORT, m_treeCtrlData.GetImage(), (DWORD)this);
		m_treeCtrlData.SetTreeItem (hItem);
		GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_EXIST);
	}

	CaIjaTreeItemData* pData = NULL;
	POSITION p, pos = m_listTable.GetHeadPosition();
	//
	// Delete all items if their states are <ITEM_DELETE>
	while (pos != NULL)
	{
		p = pos;
		pData = m_listTable.GetNext (pos);
		hItem = pData->GetTreeCtrlData().GetTreeItem();
		if (pData->GetTreeCtrlData().GetState() == CaTreeCtrlData::ITEM_DELETE)
		{
			if (hItem)
				pTree->DeleteItem (hItem);
			pData->GetTreeCtrlData().SetTreeItem(NULL);
			m_listTable.RemoveAt (p);
			delete pData;
		}
	}

	//
	// Display the rest of items if their states are <ITEM_NEW>:
	pos = m_listTable.GetHeadPosition();
	if (pos == NULL)
	{
		m_EmptyTable.Display (pTree, m_treeCtrlData.GetTreeItem());
		return;
	}
	else
	{
		CaTreeCtrlData& emptyDta = m_EmptyTable.GetTreeCtrlData();
		if (emptyDta.GetTreeItem())
			pTree->DeleteItem (emptyDta.GetTreeItem());
		emptyDta.SetTreeItem(NULL);
	}

	while (pos != NULL)
	{
		pData = m_listTable.GetNext (pos);
		pData->Display (pTree, m_treeCtrlData.GetTreeItem());
	}
}





CaIjaNode::CaIjaNode():CaIjaTreeItemData(), m_EmptyDatabase(_T("<No Databases>"))
{
	CString csItem;
	csItem.LoadString(IDS_NO_DATABASES);
	m_EmptyDatabase.SetItem(csItem);
	m_treeCtrlData.SetImage (IM_FOLDER_NODE_NORMAL, IM_FOLDER_NODE_NORMAL);
	m_EmptyDatabase.GetTreeCtrlData().SetImage (IM_FOLDER_EMPTY, IM_FOLDER_EMPTY);

	m_bLocal = FALSE;
	m_pBackParent = NULL;
}

CaIjaNode::CaIjaNode(LPCTSTR lpszItem):CaIjaTreeItemData(lpszItem),  m_EmptyDatabase(_T("<No Databases>"))
{
	CString csItem;
	csItem.LoadString(IDS_NO_DATABASES);
	m_EmptyDatabase.SetItem(csItem);
	m_treeCtrlData.SetImage (IM_FOLDER_NODE_NORMAL, IM_FOLDER_NODE_NORMAL);
	m_EmptyDatabase.GetTreeCtrlData().SetImage (IM_FOLDER_EMPTY, IM_FOLDER_EMPTY);

	m_bLocal = FALSE;
	m_pBackParent = NULL;
}



CaIjaNode::~CaIjaNode()
{
	while (!m_listDatabase.IsEmpty())
		delete m_listDatabase.RemoveHead();
}

void CaIjaNode::Display (CTreeCtrl* pTree, HTREEITEM hParent)
{
	HTREEITEM hItem = m_treeCtrlData.GetTreeItem();
	//
	// Create this tree item of Node:
	if (hItem == NULL)
	{
		CString strDisplay = m_strItem;
		hItem = IJA_TreeAddItem (strDisplay, pTree, hParent, TVI_SORT, m_treeCtrlData.GetImage(), (DWORD)this);
		m_treeCtrlData.SetTreeItem (hItem);
		GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_EXIST);
	}

	CaIjaTreeItemData* pData = NULL;
	POSITION p, pos = m_listDatabase.GetHeadPosition();
	//
	// Delete all items if their states are <ITEM_DELETE>
	while (pos != NULL)
	{
		p = pos;
		pData = m_listDatabase.GetNext (pos);
		hItem = pData->GetTreeCtrlData().GetTreeItem();
		if (pData->GetTreeCtrlData().GetState() == CaTreeCtrlData::ITEM_DELETE)
		{
			if (hItem)
				pTree->DeleteItem (hItem);
			pData->GetTreeCtrlData().SetTreeItem(NULL);
			m_listDatabase.RemoveAt (p);
			delete pData;
		}
	}

	//
	// Display the rest of items if their states are <ITEM_NEW>:
	pos = m_listDatabase.GetHeadPosition();
	if (pos == NULL)
	{
		m_EmptyDatabase.Display (pTree, m_treeCtrlData.GetTreeItem());
		return;
	}
	else
	{
		CaTreeCtrlData& emptyDta = m_EmptyDatabase.GetTreeCtrlData();
		if (emptyDta.GetTreeItem())
			pTree->DeleteItem (emptyDta.GetTreeItem());
		emptyDta.SetTreeItem(NULL);
	}

	while (pos != NULL)
	{
		pData = m_listDatabase.GetNext (pos);
		pData->Display (pTree, m_treeCtrlData.GetTreeItem());
	}
}

void CaIjaNode::Expand  (CTreeCtrl* pTree, HTREEITEM hExpand, CaIjaTreeData* pTreeData)
{
	BOOL bAlreadyExpanded = m_treeCtrlData.IsAlreadyExpanded();
	CaIjaTreeItemData::Expand(pTree, hExpand, pTreeData);
	if (!bAlreadyExpanded)
	{
		CString csItem;
		csItem.LoadString(IDS_NO_DATABASES);
		m_EmptyDatabase.SetItem(csItem);
		CaRefreshTreeInfo info;
		info.SetInfo (0);
		RefreshData (pTree, hExpand, &info);
	}
}

BOOL CaIjaNode::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	CTypedPtrList<CObList, CaIjaTreeItemData*> listDatabase;
	BOOL bError = FALSE;
	try
	{
		//
		// Refresh only the branch has already been expanded:
		if (!GetTreeCtrlData().IsAlreadyExpanded())
			return TRUE;

		if (!IJA_QueryDatabase (m_strItem, listDatabase))
			return FALSE;
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason());
		bError = TRUE;
	}
	catch (...)
	{
		AfxMessageBox (_T("System error::IJA_QueryDatabase, failed to query databases."));
		bError = TRUE;
	}
	if (bError)
	{
		CString csItem;
		csItem.LoadString(IDS_DATA_UNAVAILABLE);
		m_EmptyDatabase.SetItem(csItem);
		if (pTree && hItem)
			Display (pTree, m_treeCtrlData.GetTreeItem());
		return FALSE;
	}


	CTypedPtrList<CObList, CaIjaTreeItemData*>& listOldDatabase = m_listDatabase;
	//
	// Mark all old databases to be deleted:
	POSITION pos = listOldDatabase.GetHeadPosition();
	while (pos != NULL)
	{
		CaIjaTreeItemData* pItem = listOldDatabase.GetNext (pos);
		pItem->GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_DELETE);
	}
	//
	// Add new Databases:
	pos = listDatabase.GetHeadPosition();
	while (pos != NULL)
	{
		CaIjaTreeItemData* pItem = listDatabase.GetNext (pos);
		CaIjaTreeItemData* pExist = IJA_FindItem (listOldDatabase, pItem);
		if (pExist)
		{
			pExist->GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_EXIST);
			delete pItem;
			continue;
		}

		pItem->GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_NEW);
		pItem->SetBackParent (this);
		listOldDatabase.AddTail (pItem);
	}

	if (pInfo->m_nLevel == 1 || pInfo->m_nLevel == -1)
	{
		CaRefreshTreeInfo info;
		pos = listOldDatabase.GetHeadPosition();
		while (pos != NULL)
		{
			CaIjaTreeItemData* pItem = listOldDatabase.GetNext (pos);
			if (pItem->GetTreeCtrlData().GetState () == CaTreeCtrlData::ITEM_DELETE)
				continue;
			info.SetInfo (pInfo->m_nLevel);
			pItem->RefreshData(NULL, NULL, &info);
		}
	}
	//
	// Update display the branches of path <Node>::<Database>:
	if (pTree && hItem)
		Display (pTree, pTree->GetParentItem(hItem));
	return TRUE;
}



CaIjaTreeData::CaIjaTreeData(): CaIjaTreeItemData(_T("<Nodes>")), m_EmptyNode(_T("<No Nodes>"))
{
	CString csItem;
	csItem.LoadString(IDS_NO_NODES);
	m_EmptyNode.SetItem(csItem);

	m_strItem.LoadString(IDS_ROOT_NODE);

	m_EmptyNode.GetTreeCtrlData().SetImage(IM_FOLDER_EMPTY, IM_FOLDER_EMPTY);
	m_treeCtrlData.SetImage (IM_FOLDER_CLOSED, IM_FOLDER_OPENED);

	m_databaseRoot.SetBackParent (&m_nodeRoot);
	m_tableRoot.SetBackParent(&m_databaseRoot);
	m_nDisplaySpecial = 0;
}


CaIjaTreeData::~CaIjaTreeData()
{
	while (!m_listNode.IsEmpty())
		delete m_listNode.RemoveHead();
}

void CaIjaTreeData::Expand (CTreeCtrl* pTree, HTREEITEM hExpand, CaIjaTreeData* pTreeData)
{
	BOOL bAlreadyExpanded = m_treeCtrlData.IsAlreadyExpanded();
	CaIjaTreeItemData::Expand(pTree, hExpand, pTreeData);
	pTree->SetItemImage(m_treeCtrlData.GetTreeItem(), m_treeCtrlData.GetImageExpanded(), m_treeCtrlData.GetImageExpanded());
	if (!bAlreadyExpanded)
	{
		CString csItem;
		csItem.LoadString(IDS_NO_NODES);
		m_EmptyNode.SetItem(csItem);
		CaRefreshTreeInfo info;
		info.SetInfo (0);
		RefreshData(pTree, hExpand, &info);
	}
}

BOOL CaIjaTreeData::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bError = FALSE;
	CTypedPtrList<CObList, CaIjaTreeItemData*> listNode;
	try
	{
		//
		// Refresh only the branch has already been expanded:
		if (!GetTreeCtrlData().IsAlreadyExpanded())
			return TRUE;

		if (!IJA_QueryNode (listNode))
			return FALSE;
	}
	catch (CeSqlException e)
	{
		if (!INGRESII_IsRunning())
		{
			CString strMsg;
			strMsg.LoadString(IDS_INGRES_NOT_RUNNING);
			AfxMessageBox (strMsg);
		}
		else
			AfxMessageBox (e.GetReason());
		bError = TRUE;
	}
	catch (...)
	{
		AfxMessageBox (_T("System error::IJA_QueryNode, failed to query nodes."));
		bError = TRUE;
	}
	if (bError)
	{
		CString csItem;
		csItem.LoadString(IDS_DATA_UNAVAILABLE);
		m_EmptyNode.SetItem(csItem);
		if (pTree && hItem)
			Display (pTree, NULL);
		return FALSE;
	}


	CfMainFrame* pm = (CfMainFrame*)theApp.m_pMainWnd;
	ASSERT (pm);
	if (!pm)
		return FALSE;
	CvViewRight* pV = (CvViewRight*)pm->GetRightView();
	ASSERT (pV);
	if (!pV)
		return FALSE;
	BOOL bAddNode = FALSE;
	if (pV->m_pCtrl && IsWindow (pV->m_pCtrl->m_hWnd))
	{
		pV->m_pCtrl->CleanNode();
		bAddNode = TRUE;
	}
	//
	// Mark all old Node to be deleted:
	POSITION pos = m_listNode.GetHeadPosition();
	while (pos != NULL)
	{
		CaIjaTreeItemData* pItem = m_listNode.GetNext (pos);
		pItem->GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_DELETE);
	}
	//
	// Add new Node:
	pos = listNode.GetHeadPosition();
	while (pos != NULL)
	{
		CaIjaTreeItemData* pItem = listNode.GetNext (pos);
		CaIjaTreeItemData* pExist = IJA_FindItem (m_listNode, pItem);
		if (pExist)
		{
			pExist->GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_EXIST);
			delete pItem;
			continue;
		}

		pItem->GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_NEW);
		m_listNode.AddTail (pItem);
	}

	if (pInfo->m_nLevel == 1 || pInfo->m_nLevel == -1)
	{
		pos = m_listNode.GetHeadPosition();
		while (pos != NULL)
		{
			CaIjaTreeItemData* pItem = m_listNode.GetNext (pos);
			if (pItem->GetTreeCtrlData().GetState () == CaTreeCtrlData::ITEM_DELETE)
				continue;
			CaRefreshTreeInfo info;
			if (pInfo->m_nLevel == 1)
				info.SetInfo (0);
			else
				info.SetInfo (-1);
			pItem->RefreshData (NULL, NULL, &info);
		}
	}

	//
	// Update display of all nodes:
	if (pTree && hItem)
		Display (pTree, NULL);
	if (bAddNode)
	{
		pos = m_listNode.GetHeadPosition();
		while (pos != NULL)
		{
			CaIjaTreeItemData* pItem = m_listNode.GetNext (pos);
			pV->m_pCtrl->AddNode(pItem->GetItem());
		}
	}

	return TRUE;
}


void CaIjaTreeData::Collapse(CTreeCtrl* pTree, HTREEITEM hExpand, CaIjaTreeData* pTreeData)
{
	pTree->SetItemImage(m_treeCtrlData.GetTreeItem(), m_treeCtrlData.GetImage(), m_treeCtrlData.GetImage());
	CaIjaTreeItemData::Expand(pTree, hExpand, pTreeData);
}


void CaIjaTreeData::Display (CTreeCtrl* pTree, HTREEITEM hParent)
{
	if (m_nDisplaySpecial == 0)
	{
		HTREEITEM hItem = m_treeCtrlData.GetTreeItem();
		//
		// Create this tree item of Root : <Nodes>:
		if (hItem == NULL)
		{
			CString strDisplay = m_strItem;
			hItem = IJA_TreeAddItem (strDisplay, pTree, pTree->GetRootItem(), TVI_SORT, m_treeCtrlData.GetImage(), (DWORD)this);
			m_treeCtrlData.SetTreeItem (hItem);
		}

		CaIjaTreeItemData* pData = NULL;
		POSITION p, pos = m_listNode.GetHeadPosition();
		//
		// Delete all items if their states are <ITEM_DELETE>
		while (pos != NULL)
		{
			p = pos;
			pData = m_listNode.GetNext (pos);
			hItem = pData->GetTreeCtrlData().GetTreeItem();
			if (pData->GetTreeCtrlData().GetState() == CaTreeCtrlData::ITEM_DELETE)
			{
				if (hItem)
					pTree->DeleteItem (hItem);
				pData->GetTreeCtrlData().SetTreeItem(NULL);
				m_listNode.RemoveAt (p);
				delete pData;
			}
		}
		//
		// Display the rest of items if their states are <ITEM_NEW>:
		pos = m_listNode.GetHeadPosition();
		if (pos == NULL)
		{
			m_EmptyNode.Display (pTree, m_treeCtrlData.GetTreeItem());
			return;
		}
		else
		{
			CaTreeCtrlData& emptyDta = m_EmptyNode.GetTreeCtrlData();
			if (emptyDta.GetTreeItem())
				pTree->DeleteItem (emptyDta.GetTreeItem());
			emptyDta.SetTreeItem(NULL);
		}

		while (pos != NULL)
		{
			pData = m_listNode.GetNext (pos);
			pData->Display (pTree, m_treeCtrlData.GetTreeItem());
		}
	}
	else
	{
		switch (m_nDisplaySpecial)
		{
		case 1: // Node level
			m_nodeRoot.Display (pTree, hParent);
			break;

		case 2: // Database level
			m_databaseRoot.Display (pTree, hParent);
			break;

		case 3: // Table level
			m_tableRoot.Display (pTree, hParent);
			break;

		default:
			ASSERT (FALSE);
			break;
		}
	}
}

int CALLBACK CaIjaTreeData::fnCompareComponent(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CaIjaTreeItemData* pComponent1 = (CaIjaTreeItemData*)lParam1;
	CaIjaTreeItemData* pComponent2 = (CaIjaTreeItemData*)lParam2;
	if (pComponent1 == NULL || pComponent2 == NULL)
		return 0;
	return (pComponent1->GetItem().CollateNoCase (pComponent2->GetItem()) >= 0 )? 1: -1;
}

void CaIjaTreeData::SortFolder(CTreeCtrl* pTree)
{
	TV_SORTCB sortcb;

	sortcb.hParent = pTree->GetRootItem();
	sortcb.lpfnCompare = CaIjaTreeData::fnCompareComponent;
	sortcb.lParam = NULL;
	if (!m_listNode.IsEmpty())
	{
		POSITION pos1 = m_listNode.GetHeadPosition();
		while (pos1 != NULL)
		{
			CaIjaNode* pNode = (CaIjaNode*)m_listNode.GetNext (pos1);
			sortcb.hParent = pNode->GetTreeCtrlData().GetTreeItem();
			if (sortcb.hParent)
			{
				pTree->SortChildrenCB (&sortcb);

				CTypedPtrList < CObList, CaIjaTreeItemData* >& ldb = pNode->GetListDatabase();
				POSITION pos2 = ldb.GetHeadPosition();
				while (pos2 != NULL)
				{
					CaIjaTreeItemData* pDatabase = ldb.GetNext (pos2);
					sortcb.hParent = pDatabase->GetTreeCtrlData().GetTreeItem();
					pTree->SortChildrenCB (&sortcb);
				}
			}
		}
	}
}
