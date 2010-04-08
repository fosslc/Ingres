/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ieatree.cpp: Implementation of the Ingres Objects and Folders 
**    Project  : IMPORT & EXPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Hierarchy of Ingres Objects.
**
** History:
**
** 12-Dec-2000 (uk$so01)
**    Created
** 24-Jul-2001 (uk$so01)
**    SIR #105322, Do not show the item "<new table>" when invoking
**    in context to select the specific table given in the IIASTRUCT::wczTable parameter.
** 30-Jan-2002 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 14-Oct-2004 (uk$so01)
**    BUG #113226 / ISSUE 13720088: The DBMS connections should not be in the cache
**    for IIA & IEA.
** 22-Nov-2004 (schph01)
**    BUG #113511 replaceCompareNoCase function by CollateNoCase
**    according to the LC_COLLATE category setting of the locale code
**    page currently in  use.
**/

#include "stdafx.h"
#include "ieatree.h"
#include "constdef.h"
#include "vnodedml.h"
#include "ingobdml.h"
#include "rcdepend.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#if defined (_SORT_TREEITEM)
inline static int CALLBACK compareDBObject (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CtrfItemData* o1 = (CtrfItemData*)lParam1;
	CtrfItemData* o2 = (CtrfItemData*)lParam2;
	if (o1 && o2)
	{
		if (o1->GetTreeItemType() != o2->GetTreeItemType())
		{
			if (o1->IsFolder() && !o2->IsFolder())
				return -1;
			if (o2->IsFolder() && !o1->IsFolder())
				return 1;
		}

		CaDBObject* ob1 = o1->GetDBObject();
		CaDBObject* ob2 = o2->GetDBObject();
		if (ob1 && ob2)
		{
			return ob1->GetName().CollateNoCase(ob2->GetName());
		}
		else
			return 0;
	}
	else
		return 0;
}

inline static int CALLBACK compareTable (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	BOOL bGroup = (BOOL)lParamSort;
	CaIeaTable* o1 = (CaIeaTable*)lParam1;
	CaIeaTable* o2 = (CaIeaTable*)lParam2;
	if (o1 && o2)
	{
		if (o1->GetTreeItemType() != o2->GetTreeItemType())
		{
			if (o1->GetTreeItemType() == O_BASE_ITEM)
				return -1;
			if (o2->GetTreeItemType() == O_BASE_ITEM)
				return 1;
		}

		CaTable& t1 = o1->GetObject();
		CaTable& t2 = o2->GetObject();
		if (t1.GetOwner().IsEmpty())
			return -1;
		if (t2.GetOwner().IsEmpty())
			return 1;
		if (bGroup)
		{
			CString strItem1 = t1.GetOwner() + t1.GetName();
			CString strItem2 = t2.GetOwner() + t2.GetName();

			return strItem1.CollateNoCase(strItem2);
		}
		else
			return t1.GetName().CollateNoCase(t2.GetName());
	}
	else
		return 0;
}
#endif


//
// Object: Table 
// **************
IMPLEMENT_SERIAL (CaIeaTable, CtrfFolderTable, 1)



//
// Object: Folder of Tables
// ************************************************************************************************
IMPLEMENT_SERIAL (CaIeaFolderTable, CtrfTable, 1)
void CaIeaFolderTable::Expand (CTreeCtrl* pTree, HTREEITEM hExpand)
{

}

BOOL CaIeaFolderTable::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE;
	CTypedPtrList< CObList, CaDBObject* > lNew;

	try
	{
		CaLLQueryInfo info;
		info.SetObjectType(OBT_TABLE);
		CaLLQueryInfo* pQueryInfo = GetQueryInfo(&info);
		ASSERT (pQueryInfo);
		if (!pQueryInfo)
			return FALSE;
		pQueryInfo->SetIndependent(TRUE);
		if (GetPfnCOMQueryObject())
			bOk  = GetPfnCOMQueryObject()(GetAptAccess(), pQueryInfo, lNew);
		else
		{
			CmtSessionManager* pSessionManager = GetSessionManager();
			ASSERT (pSessionManager);
			if (!pSessionManager)
				return FALSE;

			if (GetPfnUserQueryObject())
				bOk = GetPfnUserQueryObject()(pQueryInfo, lNew, pSessionManager);
			else
			{
				bOk = INGRESII_llQueryObject (pQueryInfo, lNew, pSessionManager);
				if (bOk)
				{
					CaTable* pDummyTable = new CaTable (_T("<new table>"), _T(""));
					lNew.AddHead(pDummyTable);
				}
			}
		}
		if (!bOk)
			return FALSE;

		//
		// Mark all old object as being deleted:
		CtrfItemData* pObj = NULL;
		POSITION p = NULL, pos = m_listObject.GetHeadPosition();
		while (pos != NULL)
		{
			pObj = m_listObject.GetNext (pos);
			pObj->GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_DELETE);
		}

		//
		// Add new Objects:
		while (!lNew.IsEmpty())
		{
			CaTable* pNew = (CaTable*)lNew.RemoveHead();
			CtrfItemData* pExist = SearchObject(pNew);

			//
			// The new queried object already exists in the old list, we destroy it !
			if (pExist != NULL)
			{
				pExist->GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_EXIST);
				delete pNew;
				continue;
			}

			//
			// New object that is not in the old list, add it to the list:
			CaIeaTable* pNewObject = new CaIeaTable (pNew);
			pNewObject->SetBackParent (this);
			pNewObject->Initialize();
			m_listObject.AddTail (pNewObject);

			delete pNew;
		}

		//
		// Refresh Sub-Branches ?

		Display (pTree, hItem);

#if defined (_SORT_TREEITEM)
		TVSORTCB sortcb;
		memset (&sortcb, 0, sizeof(sortcb));
		sortcb.hParent = hItem;
		sortcb.lParam = (BOOL)TRUE; // Group by owner first
		sortcb.lpfnCompare = compareTable;
		pTree->SortChildrenCB(&sortcb);
#endif
	}
	catch (CeSqlException e)
	{
		m_bError = TRUE;
		AfxMessageBox (e.GetReason());
	}
	catch (...)
	{
		m_bError = TRUE;
	}

	if (m_bError)
	{
		HTREEITEM hDelete = NULL;
		CtrfItemData* pObj = NULL;
		POSITION  pos = m_listObject.GetHeadPosition();
		while (pos != NULL)
		{
			pObj = m_listObject.GetNext (pos);
			hDelete = pObj->GetTreeCtrlData().GetTreeItem();
			if (hDelete)
			{
				pTree->DeleteItem(hDelete);
				pObj->GetTreeCtrlData().SetTreeItem(NULL);
			}
		}
		while (!m_listObject.IsEmpty())
			delete m_listObject.RemoveHead();
		throw (int)0;
	}

	return bOk;
}




//
// Object: Database 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaIeaDatabase, CtrfDatabase, 1)
void CaIeaDatabase::Initialize()
{
	if (m_b4Import)
	{
		//
		// Folder Table:
		CaIeaFolderTable* pFolderTable = new CaIeaFolderTable();
		pFolderTable->SetBackParent (this);
		m_listObject.AddTail(pFolderTable);

		CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
		ASSERT (pDisplayInfo);
		int nEmptyImage, nEmptyImageExpand;
		if (pDisplayInfo)
		{
			pDisplayInfo->GetEmptyImage  (nEmptyImage, nEmptyImageExpand);
			m_EmptyNode.GetTreeCtrlData().SetImage (nEmptyImage, nEmptyImageExpand);
		}
	}
}

void CaIeaDatabase::Expand  (CTreeCtrl* pTree, HTREEITEM hExpand)
{
	BOOL bAlreadyExpanded = m_treeCtrlData.IsAlreadyExpanded();
	CtrfItemData::Expand(pTree, hExpand);
	if (!bAlreadyExpanded)
	{
		CaRefreshTreeInfo info;
		info.SetInfo (0);
		RefreshData (pTree, hExpand, &info);
	}
}

BOOL CaIeaDatabase::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE;
	try
	{
		m_bError = FALSE;
		CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
		ASSERT (pDisplayInfo);
		if (!pDisplayInfo)
			return FALSE;

		//
		// Delete the dummy item <No Tables>:
		CtrfItemData* pEmptyNode = GetEmptyNode();
		if (pEmptyNode)
		{
			CaTreeCtrlData& emptyDta = pEmptyNode->GetTreeCtrlData();
			if (emptyDta.GetTreeItem())
				pTree->DeleteItem (emptyDta.GetTreeItem());
			emptyDta.SetTreeItem(NULL);
		}

		POSITION pos = m_listObject.GetHeadPosition();
		while (pos != NULL)
		{
			CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
			if (!ptrfObj->IsDisplayFolder(pDisplayInfo))
			{
				CaRefreshTreeInfo refresInfo;
				ptrfObj->RefreshData(pTree, hItem, &refresInfo);
			}
		}

		//
		// Display the dummy Item <No Tables>:
		if (pEmptyNode && !pTree->ItemHasChildren(hItem))
		{
			CtrfItemData::DisplayEmptyOrErrorNode (pEmptyNode, pTree, hItem);
		}
	}
	catch (CeSqlException e)
	{
		m_bError = TRUE;
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		m_bError = TRUE;
	}

	if (m_bError)
	{
		//
		// Display the dummy <Data Unavailable>:
		CtrfItemData* pEmptyNode = GetEmptyNode();
		if (pEmptyNode  && !pTree->ItemHasChildren(hItem))
		{
			CtrfItemData::DisplayEmptyOrErrorNode (pEmptyNode, pTree, hItem);
			CaTreeCtrlData& emptyDta = pEmptyNode->GetTreeCtrlData();
			HTREEITEM hDummy = emptyDta.GetTreeItem();
			CString strError = pEmptyNode->GetErrorString();
			if (hDummy)
				pTree->SetItemText (hDummy, strError);
		}
	}
	return TRUE;
}


void CaIeaDatabase::Display (CTreeCtrl* pTree, HTREEITEM hParent)
{
	if (m_b4Import)
	{
		CtrfDatabase::Display(pTree, hParent);
		return;
	}

	CaDisplayInfo* pDisplayInfo = m_pBackParent? m_pBackParent->GetDisplayInfo(): NULL;
	BOOL bDisplayFolder = TRUE;
	HTREEITEM hItem = m_treeCtrlData.GetTreeItem();
	CtrfItemData* pData = NULL;
	//
	// pDisplayInfo must be accessible, it must be in the root parent of 
	// the three hierarchy !
	ASSERT(pDisplayInfo);
	if (!pDisplayInfo)
		return;

	//
	// Create this tree item:
	if (!hItem)
	{
		int nImage;
		CString strDisplay = GetDisplayedItem();

		SetImageID(pDisplayInfo);
		nImage = m_treeCtrlData.GetImage();
		hItem = CtrfItemData::TreeAddItem (strDisplay, pTree, hParent, TVI_LAST, nImage, (DWORD)this);
		m_treeCtrlData.SetTreeItem (hItem);
		m_treeCtrlData.SetState(CaTreeCtrlData::ITEM_EXIST);
		
	}
}

//
// Object: Folder of Databases
// ************************************************************************************************
IMPLEMENT_SERIAL (CaIeaFolderDatabase, CtrfFolderDatabase, 1)
void CaIeaFolderDatabase::Expand  (CTreeCtrl* pTree, HTREEITEM hExpand)
{

}


BOOL CaIeaFolderDatabase::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE;
	CTypedPtrList< CObList, CaDBObject* > lNew;
	try
	{
		CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
		ASSERT (pQueryInfo);
		if (!pQueryInfo)
			return FALSE;
		pQueryInfo->SetIncludeSystemObjects(FALSE);
		pQueryInfo->SetIndependent(TRUE);

		if (GetPfnCOMQueryObject())
			bOk  = GetPfnCOMQueryObject()(GetAptAccess(), pQueryInfo, lNew);
		else
		{
			CmtSessionManager* pSessionManager = GetSessionManager();
			ASSERT (pSessionManager);
			if (!pSessionManager)
				return FALSE;

			if (GetPfnUserQueryObject())
				bOk = GetPfnUserQueryObject()(pQueryInfo, lNew, pSessionManager);
			else
				bOk = INGRESII_llQueryObject (pQueryInfo, lNew, pSessionManager);
		}
		if (!bOk)
			return FALSE;

		//
		// Mark all old object as being deleted:
		CtrfItemData* pObj = NULL;
		POSITION p = NULL, pos = m_listObject.GetHeadPosition();
		while (pos != NULL)
		{
			pObj = m_listObject.GetNext (pos);
			pObj->GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_DELETE);
		}

		//
		// Add new Objects:
		while (!lNew.IsEmpty())
		{
			CaDatabase* pNew = (CaDatabase*)lNew.RemoveHead();
			CtrfItemData* pExist = SearchObject(pNew);

			//
			// The new queried object already exists in the old list, we destroy it !
			if (pExist != NULL)
			{
				pExist->GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_EXIST);
				delete pNew;
				continue;
			}

			//
			// New object that is not in the old list, add it to the list:
			CaIeaDatabase* pNewObject = new CaIeaDatabase (pNew);
			pNewObject->SetBackParent (this);
			pNewObject->ImportMode(m_b4Import);
			pNewObject->Initialize();
			m_listObject.AddTail (pNewObject);

			delete pNew;
		}

		//
		// Refresh Sub-Branches ?

		Display (pTree, hItem);

#if defined (_SORT_TREEITEM)
		TVSORTCB sortcb;
		memset (&sortcb, 0, sizeof(sortcb));
		sortcb.hParent = hItem;
		sortcb.lParam = 0;
		sortcb.lpfnCompare = compareDBObject;
		pTree->SortChildrenCB(&sortcb);
#endif
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{

	}

	return TRUE;
}


//
// Object: Node Server
// ************************************************************************************************
IMPLEMENT_SERIAL (CaIeaNodeServer, CtrfNodeServer, 1)
void CaIeaNodeServer::Initialize()
{
	//
	// Folder Database:
	CaIeaFolderDatabase* pFolderDatabase = new CaIeaFolderDatabase();
	pFolderDatabase->SetBackParent (this);
	pFolderDatabase->ImportMode(m_b4Import);
	m_listObject.AddTail(pFolderDatabase);

	CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
	ASSERT (pDisplayInfo);
	int nEmptyImage, nEmptyImageExpand;
	if (pDisplayInfo)
	{
		pDisplayInfo->GetEmptyImage  (nEmptyImage, nEmptyImageExpand);
		m_EmptyNode.GetTreeCtrlData().SetImage (nEmptyImage, nEmptyImageExpand);
	}
}


void CaIeaNodeServer::Expand  (CTreeCtrl* pTree, HTREEITEM hExpand)
{
	BOOL bAlreadyExpanded = m_treeCtrlData.IsAlreadyExpanded();
	CtrfItemData::Expand(pTree, hExpand);
	if (!bAlreadyExpanded)
	{
		CaRefreshTreeInfo info;
		info.SetInfo (0);
		RefreshData (pTree, hExpand, &info);
	}
}


BOOL CaIeaNodeServer::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bError = FALSE;

	try
	{
		CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
		ASSERT (pDisplayInfo);
		if (!pDisplayInfo)
			return FALSE;

		//
		// Delete the dummy item <No Databases>:
		CtrfItemData* pEmptyNode = GetEmptyNode();
		if (pEmptyNode)
		{
			CaTreeCtrlData& emptyDta = pEmptyNode->GetTreeCtrlData();
			if (emptyDta.GetTreeItem())
				pTree->DeleteItem (emptyDta.GetTreeItem());
			emptyDta.SetTreeItem(NULL);
		}

		POSITION pos = m_listObject.GetHeadPosition();
		while (pos != NULL)
		{
			CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
			if (!ptrfObj->IsDisplayFolder(pDisplayInfo))
			{
				CaRefreshTreeInfo refresInfo;
				ptrfObj->RefreshData(pTree, hItem, &refresInfo);
			}
		}
		//
		// Display the dummy Item <No Tables>:
		if (pEmptyNode && !pTree->ItemHasChildren(hItem))
		{
			CtrfItemData::DisplayEmptyOrErrorNode (pEmptyNode, pTree, hItem);
		}
	}
	catch (CeNodeException e)
	{
		m_bError = TRUE;
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		m_bError = TRUE;
	}

	if (m_bError)
	{
		//
		// Display the dummy <Data Unavailable>:
		CtrfItemData* pEmptyNode = GetEmptyNode();
		if (pEmptyNode  && !pTree->ItemHasChildren(hItem))
		{
			CtrfItemData::DisplayEmptyOrErrorNode (pEmptyNode, pTree, hItem);
			CaTreeCtrlData& emptyDta = pEmptyNode->GetTreeCtrlData();
			HTREEITEM hDummy = emptyDta.GetTreeItem();
			CString strError = pEmptyNode->GetErrorString();
			if (hDummy)
				pTree->SetItemText (hDummy, strError);
		}
	}
	
	return TRUE;
}




//
// Object: Folder of Node Server
// ************************************************************************************************
IMPLEMENT_SERIAL (CaIeaFolderNodeServer, CtrfFolderNodeServer, 1)
void CaIeaFolderNodeServer::Expand  (CTreeCtrl* pTree, HTREEITEM hExpand)
{
	BOOL bAlreadyExpanded = m_treeCtrlData.IsAlreadyExpanded();
	CtrfItemData::Expand(pTree, hExpand);
	if (!bAlreadyExpanded)
	{
		CaRefreshTreeInfo info;
		info.SetInfo (0);
		RefreshData (pTree, hExpand, &info);
	}
}

BOOL CaIeaFolderNodeServer::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE;
	CaLLQueryInfo queryInfo;
	queryInfo.SetObjectType(OBT_VNODE_SERVERCLASS);

	CTypedPtrList< CObList, CaDBObject* > lNew;
	try
	{
		//
		// Query list of Objects:
		if (GetPfnCOMQueryObject())
		{
			CaLLQueryInfo* pQueryInfo = GetQueryInfo(&queryInfo);
			ASSERT (pQueryInfo);
			bOk  = GetPfnCOMQueryObject()(GetAptAccess(), pQueryInfo, lNew);
		}
		else
		{
			CaLLQueryInfo* pQueryInfo = GetQueryInfo(&queryInfo);
			ASSERT (pQueryInfo);
			CaIeaNode* pBackParent = (CaIeaNode*)GetBackParent();
			ASSERT (pBackParent);
			if (pBackParent)
			{
				CaNode& node = pBackParent->GetObject();

				if (GetPfnUserQueryObject())
					bOk  = GetPfnUserQueryObject()(pQueryInfo, lNew, NULL);
				else
					bOk = (VNODE_QueryServer (&node, lNew) == NOERROR)? TRUE: FALSE;
			}
		}
		if (!bOk)
			return FALSE;

		//
		// Mark all old object as being deleted:
		CtrfItemData* pObj = NULL;
		POSITION p = NULL, pos = m_listObject.GetHeadPosition();
		while (pos != NULL)
		{
			pObj = m_listObject.GetNext (pos);
			pObj->GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_DELETE);
		}

		//
		// Add new Objects:
		while (!lNew.IsEmpty())
		{
			CaNodeServer* pNew = (CaNodeServer*)lNew.RemoveHead();
			CtrfItemData* pExist = SearchObject(pNew);

			//
			// The new queried object already exists in the old list, we destroy it !
			if (pExist != NULL)
			{
				pExist->GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_EXIST);
				delete pNew;
				continue;
			}

			//
			// New object that is not in the old list, add it to the list:
			CaIeaNodeServer* pNewObject = new CaIeaNodeServer (pNew);
			pNewObject->SetBackParent (this);
			pNewObject->ImportMode(m_b4Import);
			pNewObject->Initialize();
			m_listObject.AddTail (pNewObject);

			delete pNew;
		}

		//
		// Refresh Sub-Branches ?

		Display (pTree, hItem);
	}
	catch (CeNodeException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		return FALSE;
	}

	return TRUE;
}


//
// Object: Node 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaIeaNode, CtrfNode, 1)
void CaIeaNode::Initialize()
{
	while (!m_listObject.IsEmpty())
		delete m_listObject.RemoveHead();
	//
	// Folder Database:
	CaIeaFolderDatabase* pFolderDatabase = new CaIeaFolderDatabase();
	pFolderDatabase->SetBackParent (this);
	pFolderDatabase->ImportMode(m_b4Import);
	m_listObject.AddTail(pFolderDatabase);

	//
	// Folder Server:
	CaIeaFolderNodeServer* pFolderServer = new CaIeaFolderNodeServer();
	pFolderServer->SetBackParent (this);
	pFolderServer->ImportMode(m_b4Import);
	m_listObject.AddTail(pFolderServer);
}

void CaIeaNode::Expand  (CTreeCtrl* pTree, HTREEITEM hExpand)
{
	BOOL bAlreadyExpanded = m_treeCtrlData.IsAlreadyExpanded();
	CtrfItemData::Expand(pTree, hExpand);
	if (!bAlreadyExpanded)
	{
		CtrfItemData::Expand(pTree, hExpand);
		CaRefreshTreeInfo info;
		info.SetInfo (0);
		RefreshData (pTree, hExpand, &info);
	}
}


BOOL CaIeaNode::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bError = FALSE;

	try
	{
		CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
		ASSERT (pDisplayInfo);
		if (!pDisplayInfo)
			return FALSE;

		POSITION pos = m_listObject.GetHeadPosition();
		while (pos != NULL)
		{
			CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
			if (!ptrfObj->IsDisplayFolder(pDisplayInfo))
			{
				CaRefreshTreeInfo refresInfo;
				ptrfObj->RefreshData(pTree, hItem, &refresInfo);
			}
		}
	}
	catch (CeNodeException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{

	}
	return TRUE;
}


//
// Object: Iea Tree (Import & Export Assistant Tree)
// ************************************************************************************************
IMPLEMENT_SERIAL (CaIeaTree, CtrfFolderNode, 1)
CaIeaTree::CaIeaTree():CtrfFolderNode()
{
	SetSessionManager(&theApp.m_sessionManager);
}

CaIeaTree::~CaIeaTree()
{
	CtrfItemData* pObj = NULL;
	POSITION p, pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		pObj = m_listObject.GetNext(pos);
		m_listObject.RemoveAt(p);
		delete pObj;
	}
}

BOOL CaIeaTree::Initialize()
{
	CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
#if defined (_DISPLAY_OWNER_OBJECT)
	CString strFormat;
	strFormat.LoadString(IDS_OWNERxITEM);
	pDisplayInfo->SetOwnerPrefixedFormat(strFormat);
#endif

	pDisplayInfo->SetDisplayObjectInFolder (O_NODE, FALSE);       // Do not display child items in Folder Nodes:
	pDisplayInfo->SetDisplayObjectInFolder (O_DATABASE, FALSE);   // Do not display child items in Folder Databases:
	//
	// Do not display Table Forlder:
	pDisplayInfo->SetDisplayObjectInFolder (O_TABLE, FALSE);      // Do not display child items in Folder Tables:

	pDisplayInfo->NodeChildrenFolderAddFlag(FOLDER_SERVER);
	pDisplayInfo->NodeChildrenFolderAddFlag(FOLDER_DATABASE);

	m_queryInfo.SetIncludeSystemObjects (FALSE);

	return TRUE;
}


void CaIeaTree::Expand  (CTreeCtrl* pTree, HTREEITEM hExpand)
{

}

BOOL CaIeaTree::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE;
	//
	// List of CaNode:
	CTypedPtrList< CObList, CaDBObject* > lNew;
	try
	{
		//
		// Query list of nodes:
		CaLLQueryInfo queryInfo;
		queryInfo.SetObjectType(OBT_VIRTNODE);
		CaLLQueryInfo* pQueryInfo = GetQueryInfo(&queryInfo);
		ASSERT (pQueryInfo);
		if (GetPfnCOMQueryObject())
			bOk  = GetPfnCOMQueryObject()(GetAptAccess(), pQueryInfo, lNew);
		else
		{
			if (GetPfnUserQueryObject())
				bOk  = GetPfnUserQueryObject()(pQueryInfo, lNew, NULL);
			else
				bOk = (VNODE_QueryNode (lNew) == NOERROR)? TRUE: FALSE;
		}
		if (!bOk)
			return FALSE;

		//
		// Mark all old object as being deleted:
		CtrfItemData* pObj = NULL;
		POSITION p = NULL, pos = m_listObject.GetHeadPosition();
		while (pos != NULL)
		{
			pObj = m_listObject.GetNext (pos);
			pObj->GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_DELETE);
		}

		//
		// Add new Objects:
		while (!lNew.IsEmpty())
		{
			CaNode* pNew = (CaNode*)lNew.RemoveHead();
			CtrfItemData* pExist = SearchObject(pNew);

			//
			// The new queried object already exists in the old list, we destroy it !
			if (pExist != NULL)
			{
				pExist->GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_EXIST);
				delete pNew;
				continue;
			}

			//
			// New object that is not in the old list, add it to the list:
			CaIeaNode* pNewObject = new CaIeaNode (pNew);
			pNewObject->SetBackParent (this);
			pNewObject->Initialize();
			m_listObject.AddTail (pNewObject);

			delete pNew;
		}

		//
		// Refresh Sub-Branches ?

		Display (pTree, hItem);

#if defined (_SORT_TREEITEM)
		TVSORTCB sortcb;
		memset (&sortcb, 0, sizeof(sortcb));
		sortcb.hParent = hItem;
		sortcb.lParam = 0;
		sortcb.lpfnCompare = compareDBObject;
		pTree->SortChildrenCB(&sortcb);
#endif
	}
	catch (CeNodeException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
		return FALSE;
	}
	catch (...)
	{
		return FALSE;
	}
	return TRUE;
}




//
// Object: Export Tree (Export Assistant Tree)
// ************************************************************************************************
IMPLEMENT_SERIAL (CaIexportTree, CtrfFolderNode, 1)
CaIexportTree::CaIexportTree():CtrfFolderNode()
{
	SetSessionManager(&theApp.m_sessionManager);
}

CaIexportTree::~CaIexportTree()
{
	CtrfItemData* pObj = NULL;
	POSITION p, pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		pObj = m_listObject.GetNext(pos);
		m_listObject.RemoveAt(p);
		delete pObj;
	}
}

BOOL CaIexportTree::Initialize()
{
	CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
#if defined (_DISPLAY_OWNER_OBJECT)
	CString strFormat;
	strFormat.LoadString(IDS_OWNERxITEM);
	pDisplayInfo->SetOwnerPrefixedFormat(strFormat);
#endif

	pDisplayInfo->SetDisplayObjectInFolder (O_NODE, FALSE);       // Do not display child items in Folder Nodes:
	pDisplayInfo->SetDisplayObjectInFolder (O_DATABASE, FALSE);   // Do not display child items in Folder Databases:
	//
	// Do not display Table Forlder:
	pDisplayInfo->SetDisplayObjectInFolder (O_TABLE, FALSE);      // Do not display child items in Folder Tables:

	pDisplayInfo->NodeChildrenFolderAddFlag(FOLDER_SERVER);
	pDisplayInfo->NodeChildrenFolderAddFlag(FOLDER_DATABASE);

	m_queryInfo.SetIncludeSystemObjects (FALSE);

	return TRUE;
}


void CaIexportTree::Expand  (CTreeCtrl* pTree, HTREEITEM hExpand)
{

}

BOOL CaIexportTree::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE;
	//
	// List of CaNode:
	CTypedPtrList< CObList, CaDBObject* > lNew;
	try
	{
		//
		// Query list of nodes:
		CaLLQueryInfo queryInfo;
		queryInfo.SetObjectType(OBT_VIRTNODE);
		CaLLQueryInfo* pQueryInfo = GetQueryInfo(&queryInfo);
		ASSERT (pQueryInfo);
		if (GetPfnCOMQueryObject())
			bOk  = GetPfnCOMQueryObject()(GetAptAccess(), pQueryInfo, lNew);
		else
		{
			if (GetPfnUserQueryObject())
				bOk  = GetPfnUserQueryObject()(pQueryInfo, lNew, NULL);
			else
				bOk = (VNODE_QueryNode (lNew) == NOERROR)? TRUE: FALSE;
		}
		if (!bOk)
			return FALSE;

		//
		// Mark all old object as being deleted:
		CtrfItemData* pObj = NULL;
		POSITION p = NULL, pos = m_listObject.GetHeadPosition();
		while (pos != NULL)
		{
			pObj = m_listObject.GetNext (pos);
			pObj->GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_DELETE);
		}

		//
		// Add new Objects:
		while (!lNew.IsEmpty())
		{
			CaNode* pNew = (CaNode*)lNew.RemoveHead();
			CtrfItemData* pExist = SearchObject(pNew);

			//
			// The new queried object already exists in the old list, we destroy it !
			if (pExist != NULL)
			{
				pExist->GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_EXIST);
				delete pNew;
				continue;
			}

			//
			// New object that is not in the old list, add it to the list:
			CaIeaNode* pNewObject = new CaIeaNode (pNew);
			pNewObject->SetBackParent (this);
			pNewObject->ImportMode(FALSE);
			pNewObject->Initialize();
			m_listObject.AddTail (pNewObject);

			delete pNew;
		}

		//
		// Refresh Sub-Branches ?

		Display (pTree, hItem);

#if defined (_SORT_TREEITEM)
		TVSORTCB sortcb;
		memset (&sortcb, 0, sizeof(sortcb));
		sortcb.hParent = hItem;
		sortcb.lParam = 0;
		sortcb.lpfnCompare = compareDBObject;
		pTree->SortChildrenCB(&sortcb);
#endif
	}
	catch (CeNodeException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
		return FALSE;
	}
	catch (...)
	{
		return FALSE;
	}
	return TRUE;
}
