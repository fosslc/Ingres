/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dmlvnode.cpp: implementation of the CtrfNode, CtrfNodeXxx, 
**               CtrfFolderCtrfNode, CtrfFolderCtrfNodeXxx class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Node object and folder
**
** History:
**
** 11-Feb-1998 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
**/

#include "stdafx.h"
#include "constdef.h"
#include "trfvnode.h" // Have forward declaration of class CaDatabase, CaTable, ...
#include "vnodedml.h"
#include "usrexcep.h"
#include <winsock2.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Object: Node Attribute
// ************************************************************************************************
IMPLEMENT_SERIAL (CtrfNodeAttribute, CtrfItemData, 1)


//
// Folder of Attribute:
// ************************************************************************************************
IMPLEMENT_SERIAL (CtrfFolderNodeAttribute, CtrfItemData, 1)
CtrfFolderNodeAttribute::CtrfFolderNodeAttribute(): CtrfItemData(_T("Attributes"), O_FOLDER_NODE_ATTRIBUTE)
{
	m_EmptyNode.SetDisplayedItem (_T("<No Attributes>"));
	m_EmptyNode.SetBackParent (this);
	m_treeCtrlData.SetImage (IM_FOLDER_EMPTY, IM_FOLDER_EMPTY);
}

CtrfFolderNodeAttribute::~CtrfFolderNodeAttribute()
{
	while (!m_listObject.IsEmpty())
		delete m_listObject.RemoveHead();
}


CtrfItemData* CtrfFolderNodeAttribute::SearchObject (CaNodeAttribute* pObj)
{
	POSITION pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfNodeAttribute* pData = (CtrfNodeAttribute*)m_listObject.GetNext (pos);
		CaNodeAttribute& obj = pData->GetObject();
		if (pObj && pObj->GetName().CompareNoCase(obj.GetName()) == 0 )
		{
			return pData;
		}
	}
	return NULL;
}

void CtrfFolderNodeAttribute::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	int nImage, nImageExpand;
	int nEmptyImage, nEmptyImageExpand;

	pDisplayInfo->GetEmptyImage  (nEmptyImage, nEmptyImageExpand);
	pDisplayInfo->GetFolderImage (nImage, nImageExpand);
	m_treeCtrlData.SetImage (nImage, nImageExpand);
	
	CtrfItemData* pEmptyNode = GetEmptyNode();
	if (pEmptyNode)
	{
		pEmptyNode->GetTreeCtrlData().SetImage (nEmptyImage, nEmptyImageExpand);
	}
}



void CtrfFolderNodeAttribute::Expand  (CTreeCtrl* pTree, HTREEITEM hExpand)
{
	TRACE0 ("CtrfFolderNodeAttribute::Expand \n");
	BOOL bAlreadyExpanded = m_treeCtrlData.IsAlreadyExpanded();
	CtrfItemData::Expand(pTree, hExpand);
	if (!bAlreadyExpanded)
	{
		CaRefreshTreeInfo info;
		info.SetInfo (0);
		RefreshData (pTree, hExpand, &info);
	}
}

BOOL CtrfFolderNodeAttribute::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE;
	CaLLQueryInfo queryInfo(OBT_VNODE_ATTRIBUTE, NULL);
	//
	// Refresh only the branch has already been expanded:
	if (pInfo && pInfo->GetAction() == CaRefreshTreeInfo::ACTION_REFRESH && !GetTreeCtrlData().IsAlreadyExpanded())
		return TRUE;
	CTypedPtrList< CObList, CaDBObject* > lNew;
	CaLLQueryInfo* pQueryInfo = GetQueryInfo(&queryInfo);
	ASSERT (pQueryInfo);

	//
	// Query list of Objects:
	if (GetPfnCOMQueryObject())
	{
		bOk  = GetPfnCOMQueryObject()(GetAptAccess(), pQueryInfo, lNew);
	}
	else
	{
		CtrfItemData* pBackParent = (CtrfItemData*)GetBackParent();
		while (pBackParent && pBackParent->GetTreeItemType() != O_NODE)
		{
			pBackParent = (CtrfItemData*)pBackParent->GetBackParent();
		}

		ASSERT (pBackParent);
		if (pBackParent)
		{
			CtrfNode* pNodeParent = (CtrfNode*)pBackParent;
			CaNode& node = pNodeParent->GetObject();

			if (GetPfnUserQueryObject())
				bOk  = GetPfnUserQueryObject()(pQueryInfo, lNew, NULL);
			else
				bOk = (VNODE_QueryAttribute (&node, lNew) == NOERROR)? TRUE: FALSE;
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
		CaNodeAttribute* pNew = (CaNodeAttribute*)lNew.RemoveHead();
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
		CtrfNodeAttribute* pNewObject = new CtrfNodeAttribute (pNew);
		pNewObject->SetBackParent (this);
		pNewObject->Initialize();
		m_listObject.AddTail (pNewObject);

		delete pNew;
	}

	//
	// Refresh Sub-Branches ?
	Display (pTree, hItem);
	
	return TRUE;
}



//
// Object: Node Login
// ******************
IMPLEMENT_SERIAL (CtrfNodeLogin, CtrfItemData, 1)


//
// Folder of Login:
// ************************************************************************************************
IMPLEMENT_SERIAL (CtrfFolderNodeLogin, CtrfItemData, 1)
CtrfFolderNodeLogin::CtrfFolderNodeLogin(): CtrfItemData(_T("Logins"), O_FOLDER_NODE_LOGIN)
{
	m_EmptyNode.SetDisplayedItem (_T("<No Logins>"));
	m_EmptyNode.SetBackParent (this);
	m_treeCtrlData.SetImage (IM_FOLDER_EMPTY, IM_FOLDER_EMPTY);
}

CtrfFolderNodeLogin::~CtrfFolderNodeLogin()
{
	while (!m_listObject.IsEmpty())
		delete m_listObject.RemoveHead();
}

CtrfItemData* CtrfFolderNodeLogin::SearchObject (CaNodeLogin* pObj)
{
	POSITION pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfNodeLogin* pData = (CtrfNodeLogin*)m_listObject.GetNext (pos);
		CaNodeLogin& obj = pData->GetObject();
		if (pObj && pObj->GetName().CompareNoCase(obj.GetName()) == 0 )
		{
			return pData;
		}
	}
	return NULL;
}

void CtrfFolderNodeLogin::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	int nImage, nImageExpand;
	int nEmptyImage, nEmptyImageExpand;

	pDisplayInfo->GetEmptyImage  (nEmptyImage, nEmptyImageExpand);
	pDisplayInfo->GetFolderImage (nImage, nImageExpand);
	m_treeCtrlData.SetImage (nImage, nImageExpand);
	
	CtrfItemData* pEmptyNode = GetEmptyNode();
	if (pEmptyNode)
	{
		pEmptyNode->GetTreeCtrlData().SetImage (nEmptyImage, nEmptyImageExpand);
	}
}



void CtrfFolderNodeLogin::Expand  (CTreeCtrl* pTree, HTREEITEM hExpand)
{
	TRACE0 ("CtrfFolderNodeLogin::Expand \n");
	BOOL bAlreadyExpanded = m_treeCtrlData.IsAlreadyExpanded();
	CtrfItemData::Expand(pTree, hExpand);
	if (!bAlreadyExpanded)
	{
		CaRefreshTreeInfo info;
		info.SetInfo (0);
		RefreshData (pTree, hExpand, &info);
	}
}

BOOL CtrfFolderNodeLogin::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE;
	CaLLQueryInfo queryInfo(OBT_VNODE_LOGINPSW, NULL);
	CTypedPtrList< CObList, CaDBObject* > lNew;
	//
	// Refresh only the branch has already been expanded:
	if (pInfo && pInfo->GetAction() == CaRefreshTreeInfo::ACTION_REFRESH && !GetTreeCtrlData().IsAlreadyExpanded())
		return TRUE;
	CaLLQueryInfo* pQueryInfo = GetQueryInfo(&queryInfo);
	ASSERT (pQueryInfo);

	//
	// Query list of Objects:
	if (GetPfnCOMQueryObject())
	{
		bOk  = GetPfnCOMQueryObject()(GetAptAccess(), pQueryInfo, lNew);
	}
	else
	{
		CtrfItemData* pBackParent = (CtrfItemData*)GetBackParent();
		while (pBackParent && pBackParent->GetTreeItemType() != O_NODE)
		{
			pBackParent = (CtrfItemData*)pBackParent->GetBackParent();
		}

		ASSERT (pBackParent);
		if (pBackParent)
		{
			CtrfNode* pNodeParent = (CtrfNode*)pBackParent;
			CaNode& node = pNodeParent->GetObject();

			if (GetPfnUserQueryObject())
				bOk  = GetPfnUserQueryObject()(pQueryInfo, lNew, NULL);
			else
				bOk = (VNODE_QueryLogin (&node, lNew) == NOERROR)? TRUE: FALSE;
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
		CaNodeLogin* pNew = (CaNodeLogin*)lNew.RemoveHead();
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
		CtrfNodeLogin* pNewObject = new CtrfNodeLogin (pNew);
		pNewObject->SetBackParent (this);
		pNewObject->Initialize();
		m_listObject.AddTail (pNewObject);

		delete pNew;
	}

	//
	// Refresh Sub-Branches ?
	Display (pTree, hItem);
	
	return TRUE;
}


//
// Object: Node Connection Data
// ****************************
IMPLEMENT_SERIAL (CtrfNodeConnectData, CtrfItemData, 1)


//
// Folder of ConnectData:
// ************************************************************************************************
IMPLEMENT_SERIAL (CtrfFolderNodeConnectData, CtrfItemData, 1)
CtrfFolderNodeConnectData::CtrfFolderNodeConnectData(): CtrfItemData(_T("Connection Data"), O_FOLDER_NODE_CONNECTION)
{
	m_EmptyNode.SetDisplayedItem (_T("<No Connections>"));
	m_EmptyNode.SetBackParent (this);
	m_treeCtrlData.SetImage (IM_FOLDER_EMPTY, IM_FOLDER_EMPTY);
}

CtrfFolderNodeConnectData::~CtrfFolderNodeConnectData()
{
	while (!m_listObject.IsEmpty())
		delete m_listObject.RemoveHead();
}


CtrfItemData* CtrfFolderNodeConnectData::SearchObject (CaNodeConnectData* pObj)
{
	POSITION pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfNodeConnectData* pData = (CtrfNodeConnectData*)m_listObject.GetNext (pos);
		CaNodeConnectData& obj = pData->GetObject();
		if (pObj && pObj->GetName().CompareNoCase(obj.GetName()) == 0 )
		{
			return pData;
		}
	}
	return NULL;
}

void CtrfFolderNodeConnectData::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	int nImage, nImageExpand;
	int nEmptyImage, nEmptyImageExpand;

	pDisplayInfo->GetEmptyImage  (nEmptyImage, nEmptyImageExpand);
	pDisplayInfo->GetFolderImage (nImage, nImageExpand);
	m_treeCtrlData.SetImage (nImage, nImageExpand);
	
	CtrfItemData* pEmptyNode = GetEmptyNode();
	if (pEmptyNode)
	{
		pEmptyNode->GetTreeCtrlData().SetImage (nEmptyImage, nEmptyImageExpand);
	}
}



void CtrfFolderNodeConnectData::Expand  (CTreeCtrl* pTree, HTREEITEM hExpand)
{
	TRACE0 ("CtrfFolderNodeConnectData::Expand \n");
	BOOL bAlreadyExpanded = m_treeCtrlData.IsAlreadyExpanded();
	CtrfItemData::Expand(pTree, hExpand);
	if (!bAlreadyExpanded)
	{
		CaRefreshTreeInfo info;
		info.SetInfo (0);
		RefreshData (pTree, hExpand, &info);
	}
}

BOOL CtrfFolderNodeConnectData::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE;
	CaLLQueryInfo queryInfo(OBT_VNODE_CONNECTIONDATA, NULL);
	CTypedPtrList< CObList, CaDBObject* > lNew;
	//
	// Refresh only the branch has already been expanded:
	if (pInfo && pInfo->GetAction() == CaRefreshTreeInfo::ACTION_REFRESH && !GetTreeCtrlData().IsAlreadyExpanded())
		return TRUE;
	CaLLQueryInfo* pQueryInfo = GetQueryInfo(&queryInfo);
	ASSERT (pQueryInfo);

	//
	// Query list of Objects:
	if (GetPfnCOMQueryObject())
	{
		bOk  = GetPfnCOMQueryObject()(GetAptAccess(), pQueryInfo, lNew);
	}
	else
	{
		CtrfItemData* pBackParent = (CtrfItemData*)GetBackParent();
		while (pBackParent && pBackParent->GetTreeItemType() != O_NODE)
		{
			pBackParent = (CtrfItemData*)pBackParent->GetBackParent();
		}

		ASSERT (pBackParent);
		if (pBackParent)
		{
			CtrfNode* pNodeParent = (CtrfNode*)pBackParent;
			CaNode& node = pNodeParent->GetObject();

			if (GetPfnUserQueryObject())
				bOk  = GetPfnUserQueryObject()(pQueryInfo, lNew, NULL);
			else
				bOk = (VNODE_QueryConnection (&node, lNew) == NOERROR)? TRUE: FALSE;
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
		CaNodeConnectData* pNew = (CaNodeConnectData*)lNew.RemoveHead();
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
		CtrfNodeConnectData* pNewObject = new CtrfNodeConnectData (pNew);
		pNewObject->SetBackParent (this);
		pNewObject->Initialize();
		m_listObject.AddTail (pNewObject);

		delete pNew;
	}

	//
	// Refresh Sub-Branches ?
	Display (pTree, hItem);
	
	return TRUE;
}


//
// Object: Node Server
// ************************************************************************************************
IMPLEMENT_SERIAL (CtrfNodeServer, CtrfItemData, 1)
void CtrfNodeServer::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	int nImage, nImageExpand;

	pDisplayInfo->GetNodeServerImage (nImage, nImageExpand);
	m_treeCtrlData.SetImage (nImage, nImageExpand);
}

void CtrfNodeServer::Initialize()
{
	CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
	ASSERT (pDisplayInfo);
	if (!pDisplayInfo)
		return;
	UINT nServerFlag = pDisplayInfo->ServerChildrenFolderGetFlag();

	//
	// Folder Table:
	if (nServerFlag & FOLDER_DATABASE)
	{
		CtrfFolderDatabase* pFolder = new CtrfFolderDatabase();
		pFolder->SetBackParent(this);
		m_listObject.AddTail (pFolder);
	}
}



void CtrfNodeServer::Expand  (CTreeCtrl* pTree, HTREEITEM hExpand)
{
	CtrfItemData::Expand(pTree, hExpand);
	TRACE0("CtrfNodeServer::Expand \n");
}

CaLLQueryInfo* CtrfNodeServer::GetQueryInfo(CaLLQueryInfo* pData)
{
	if (!pData)
	{
		CaLLQueryInfo info;
		info.SetServerClass(m_object.GetName());
		return m_pBackParent? m_pBackParent->GetQueryInfo(&info): NULL;
	}
	else
	{
		pData->SetServerClass(m_object.GetName());
		return m_pBackParent? m_pBackParent->GetQueryInfo(pData): NULL;
	}
}


//
// Folder of Server:
// ************************************************************************************************
IMPLEMENT_SERIAL (CtrfFolderNodeServer, CtrfItemData, 1)
CtrfFolderNodeServer::CtrfFolderNodeServer(): CtrfItemData(_T("Servers"), O_FOLDER_NODE_SERVER)
{
	m_EmptyNode.SetDisplayedItem (_T("<No Servers>"));
	m_EmptyNode.SetBackParent (this);
	m_treeCtrlData.SetImage (IM_FOLDER_EMPTY, IM_FOLDER_EMPTY);
}

CtrfFolderNodeServer::~CtrfFolderNodeServer()
{
	while (!m_listObject.IsEmpty())
		delete m_listObject.RemoveHead();
}

void CtrfFolderNodeServer::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	int nImage, nImageExpand;
	int nEmptyImage, nEmptyImageExpand;

	pDisplayInfo->GetEmptyImage  (nEmptyImage, nEmptyImageExpand);
	pDisplayInfo->GetFolderImage (nImage, nImageExpand);
	m_treeCtrlData.SetImage (nImage, nImageExpand);
	
	CtrfItemData* pEmptyNode = GetEmptyNode();
	if (pEmptyNode)
	{
		pEmptyNode->GetTreeCtrlData().SetImage (nEmptyImage, nEmptyImageExpand);
	}
}



CtrfItemData* CtrfFolderNodeServer::SearchObject (CaNodeServer* pObj)
{
	POSITION pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfNodeServer* pData = (CtrfNodeServer*)m_listObject.GetNext (pos);
		CaNodeServer& obj = pData->GetObject();
		if (pObj && pObj->GetName().CompareNoCase(obj.GetName()) == 0 )
		{
			return pData;
		}
	}
	return NULL;
}


void CtrfFolderNodeServer::Expand  (CTreeCtrl* pTree, HTREEITEM hExpand)
{
	TRACE0 ("CtrfFolderNodeServer::Expand \n");
	BOOL bAlreadyExpanded = m_treeCtrlData.IsAlreadyExpanded();
	CtrfItemData::Expand(pTree, hExpand);
	if (!bAlreadyExpanded)
	{
		CaRefreshTreeInfo info;
		info.SetInfo (0);
		RefreshData (pTree, hExpand, &info);
	}
}

BOOL CtrfFolderNodeServer::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE;
	CaLLQueryInfo queryInfo(OBT_VNODE_SERVERCLASS, NULL);
	CTypedPtrList< CObList, CaDBObject* > lNew;
	//
	// Refresh only the branch has already been expanded:
	if (pInfo && pInfo->GetAction() == CaRefreshTreeInfo::ACTION_REFRESH && !GetTreeCtrlData().IsAlreadyExpanded())
		return TRUE;
	CaLLQueryInfo* pQueryInfo = GetQueryInfo(&queryInfo);
	ASSERT (pQueryInfo);

	//
	// Query list of Objects:
	if (GetPfnCOMQueryObject())
	{
		bOk  = GetPfnCOMQueryObject()(GetAptAccess(), pQueryInfo, lNew);
	}
	else
	{
		CaLLQueryInfo* pQueryInfo = GetQueryInfo(&queryInfo);
		ASSERT (pQueryInfo);
		CtrfNode* pBackParent = (CtrfNode*)GetBackParent();
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
		CtrfNodeServer* pNewObject = new CtrfNodeServer (pNew);
		pNewObject->SetBackParent (this);
		pNewObject->Initialize();
		m_listObject.AddTail (pNewObject);

		delete pNew;
	}

	//
	// Refresh Sub-Branches ?
	Display (pTree, hItem);
	
	return TRUE;
}



//
// Folder of Node Advanced Parameter:
// ************************************************************************************************
IMPLEMENT_SERIAL (CtrfFolderNodeAdvance, CtrfItemData, 1)
CtrfFolderNodeAdvance::CtrfFolderNodeAdvance(): CtrfItemData(_T("Advanced Node Parameters"), O_FOLDER_NODE_ADVANCE)
{
	m_treeCtrlData.SetImage (IM_FOLDER_EMPTY, IM_FOLDER_EMPTY);

	CtrfFolderNodeLogin* pFolderNodeLogin= new CtrfFolderNodeLogin();
	pFolderNodeLogin->SetBackParent (this);
	m_listObject.AddTail(pFolderNodeLogin);

	CtrfFolderNodeConnectData* pFolderNodeConn= new CtrfFolderNodeConnectData();
	pFolderNodeConn->SetBackParent (this);
	m_listObject.AddTail(pFolderNodeConn);

	CtrfFolderNodeAttribute* pFolderNodeAttr= new CtrfFolderNodeAttribute();
	pFolderNodeAttr->SetBackParent (this);
	m_listObject.AddTail(pFolderNodeAttr);

}

CtrfFolderNodeAdvance::~CtrfFolderNodeAdvance()
{

}




void CtrfFolderNodeAdvance::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	int nImage, nImageExpand;
	int nEmptyImage, nEmptyImageExpand;

	pDisplayInfo->GetEmptyImage  (nEmptyImage, nEmptyImageExpand);
	pDisplayInfo->GetFolderImage (nImage, nImageExpand);
	m_treeCtrlData.SetImage (nImage, nImageExpand);
}


void CtrfFolderNodeAdvance::Expand  (CTreeCtrl* pTree, HTREEITEM hExpand)
{
	CtrfItemData::Expand(pTree, hExpand);
	TRACE0 ("CtrfFolderNodeAdvance::Expand \n");
}

BOOL CtrfFolderNodeAdvance::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	CaRefreshTreeInfo info;
	info.SetInfo (0);

	m_folderNodeLogin->RefreshData (pTree, hItem, &info);
	m_folderNodeConnectData->RefreshData (pTree, hItem, &info);
	m_folderNodeAttribute->RefreshData (pTree, hItem, &info);

	return TRUE;
}






//
// Object: Node 
// ************************************************************************************************
IMPLEMENT_SERIAL (CtrfNode, CtrfItemData, 1)
void CtrfNode::Initialize()
{
	CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
	ASSERT (pDisplayInfo);
	if (!pDisplayInfo)
		return;
	UINT nNodeFlag = pDisplayInfo->NodeChildrenFolderGetFlag();

	//
	// Folder Database:
	if (nNodeFlag & FOLDER_DATABASE)
	{
		CtrfFolderDatabase* pFolder = new CtrfFolderDatabase();
		pFolder->SetBackParent (this);
		m_listObject.AddTail(pFolder);
	}

	//
	// Folder Profile:
	if (nNodeFlag & FOLDER_PROFILE)
	{
		CtrfFolderProfile* pFolder = new CtrfFolderProfile();
		pFolder->SetBackParent (this);
		m_listObject.AddTail(pFolder);
	}
	//
	// Folder User:
	if (nNodeFlag & FOLDER_USER)
	{
		CtrfFolderUser* pFolder = new CtrfFolderUser();
		pFolder->SetBackParent (this);
		m_listObject.AddTail(pFolder);
	}
	//
	// Folder Group:
	if (nNodeFlag & FOLDER_GROUP)
	{
		CtrfFolderGroup* pFolder = new CtrfFolderGroup();
		pFolder->SetBackParent (this);
		m_listObject.AddTail(pFolder);
	}
	//
	// Folder Role:
	if (nNodeFlag & FOLDER_ROLE)
	{
		CtrfFolderRole* pFolder = new CtrfFolderRole();
		pFolder->SetBackParent (this);
		m_listObject.AddTail(pFolder);
	}
	//
	// Folder Location:
	if (nNodeFlag & FOLDER_DATABASE)
	{
		CtrfFolderLocation* pFolder = new CtrfFolderLocation();
		pFolder->SetBackParent (this);
		m_listObject.AddTail(pFolder);
	}

	
	
	//
	// Folder Server:
	if (nNodeFlag & FOLDER_SERVER)
	{
		CtrfFolderNodeServer* pFolderServer = new CtrfFolderNodeServer();
		pFolderServer->SetBackParent (this);
		m_listObject.AddTail(pFolderServer);
	}

	//
	// Local node cannot have the Advance Node Parameters:
	if (!m_object.IsLocalNode() && (nNodeFlag & FOLDER_ADVANCE))
	{
		CtrfFolderNodeAdvance* pFolderNodeAdvance = new CtrfFolderNodeAdvance();
		pFolderNodeAdvance->SetBackParent (this);
		m_listObject.AddTail(pFolderNodeAdvance);
	}
}


CtrfNode::~CtrfNode()
{
}

CaLLQueryInfo* CtrfNode::GetQueryInfo(CaLLQueryInfo* pData)
{
	if (!pData)
	{
		CaLLQueryInfo info;
		info.SetNode(m_object.GetName());
		return m_pBackParent? m_pBackParent->GetQueryInfo(&info): NULL;
	}
	else
	{
		pData->SetNode(m_object.GetName());
		return m_pBackParent? m_pBackParent->GetQueryInfo(pData): NULL;
	}
}

void CtrfNode::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	int nImage, nImageExpand;
	if (m_object.IsLocalNode())
		pDisplayInfo->GetNodeLocalImage  (nImage, nImageExpand);
	else
		pDisplayInfo->GetNodeImage  (nImage, nImageExpand);

	m_treeCtrlData.SetImage (nImage, nImageExpand);
}

UINT CtrfNode::GetDisplayObjectFlag(CaDisplayInfo* pDisplayInfo)
{
	return  pDisplayInfo->NodeChildrenFolderGetFlag();
}





void CtrfNode::Expand  (CTreeCtrl* pTree, HTREEITEM hExpand)
{
	BOOL bAlreadyExpanded = m_treeCtrlData.IsAlreadyExpanded();
	CtrfItemData::Expand(pTree, hExpand);
	if (bAlreadyExpanded)
		return;
	CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
	//
	// pDisplayInfo must be accessible, it must be in the root parent of 
	// the three hierarchy !
	ASSERT(pDisplayInfo);
	if (!pDisplayInfo)
		return;
	
	TRACE0("CtrfNode::Expand \n");
}




//
// Object: Folder of Node
// ************************************************************************************************
IMPLEMENT_SERIAL (CtrfFolderNode, CtrfItemData, 1)
CtrfFolderNode::CtrfFolderNode(): CtrfItemData(_T("<Nodes>"), O_FOLDER_NODE)
{
	m_EmptyNode.SetBackParent (this);
	m_pAptAccess = NULL;
	m_pfnCOMQueryObject = NULL;
	m_pfnUserQueryObject = NULL;
	m_pSessionManager = NULL;
}
	
CtrfFolderNode::~CtrfFolderNode()
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

void CtrfFolderNode::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	int nImage, nImageExpand;
	int nEmptyImage, nEmptyImageExpand;

	pDisplayInfo->GetEmptyImage  (nEmptyImage, nEmptyImageExpand);
	pDisplayInfo->GetFolderImage (nImage, nImageExpand);
	m_treeCtrlData.SetImage (nImage, nImageExpand);
	
	CtrfItemData* pEmptyNode = GetEmptyNode();
	if (pEmptyNode)
	{
		pEmptyNode->GetTreeCtrlData().SetImage (nEmptyImage, nEmptyImageExpand);
	}
}



//
// You must called initialize() befor starting to use cache !
BOOL CtrfFolderNode::Initialize()
{
	//
	// Query List of Nodes;
	CTypedPtrList< CObList, CaDBObject* > lNew;
	HRESULT hError = VNODE_QueryNode (lNew);
	if (hError != NOERROR)
	{
		//
		// TODO:HANDLE ERRROR
		return FALSE;
	}

	while (!lNew.IsEmpty())
	{
		CaNode* pObj = (CaNode*)lNew.RemoveHead();

		//
		// New object that is not in the old list, add it to the list:
		CtrfNode* pNewObject = new CtrfNode (pObj);

		pNewObject->SetBackParent (this);
		m_listObject.AddTail (pNewObject);

		delete pObj;
	}

	return TRUE;
}

void CtrfFolderNode::Display (CTreeCtrl* pTree,  HTREEITEM hParent)
{
	if (!hParent)
		hParent = pTree->GetRootItem();

	BOOL bDisplayObjectInFolder = m_displayInfo.IsDisplayObjectInFolder (O_NODE);
	HTREEITEM hItem = m_treeCtrlData.GetTreeItem();
	CtrfItemData* pData = NULL;
	POSITION p, pos = m_listObject.GetHeadPosition();

	//
	// Create this tree item:
	if (!hItem)
	{
		if (bDisplayObjectInFolder )
		{
			int nImage, nImageExpand;
			m_displayInfo.GetFolderImage (nImage, nImageExpand);
			m_treeCtrlData.SetImage (nImage, nImageExpand);

			CString strDisplay = GetDisplayedItem();
			hItem = CtrfItemData::TreeAddItem (strDisplay, pTree, hParent, TVI_LAST, nImage, (DWORD)this);
			m_treeCtrlData.SetTreeItem (hItem);
		}
		else
		{
			m_treeCtrlData.SetTreeItem (NULL);
			hItem = hParent;
		}
	}
	//
	// Display the list of Objects:
	if (!m_bError)
	{
		//
		// Delete the dummy item:
		CaTreeCtrlData& emptyDta = m_EmptyNode.GetTreeCtrlData();
		if (emptyDta.GetTreeItem())
			pTree->DeleteItem (emptyDta.GetTreeItem());
		emptyDta.SetTreeItem(NULL);

		pos = m_listObject.GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			pData = m_listObject.GetNext (pos);
			if (pData->GetTreeCtrlData().GetState() != CaTreeCtrlData::ITEM_DELETE)
			{
				pData->Display (pTree, hItem);
			}
			else
			{
				//
				// Delete all items if their states are <ITEM_DELETE>
				HTREEITEM hDeleteItem = pData->GetTreeCtrlData().GetTreeItem();
				if (hDeleteItem)
					pTree->DeleteItem (hDeleteItem);

				pData->GetTreeCtrlData().SetTreeItem(NULL);
				m_listObject.RemoveAt (p);
				delete pData;
			}
		}

		//
		// Display the dummy item <No Objects>:
		pos = m_listObject.GetHeadPosition();
		if (pos == NULL && bDisplayObjectInFolder)
		{
			m_EmptyNode.Display (pTree, hItem);
		}
	}
	else
	if (bDisplayObjectInFolder)
	{
		//
		// Display the dummy <Data Unavailable>:
		m_EmptyNode.Display (pTree, hItem);
		CaTreeCtrlData& emptyDta = m_EmptyNode.GetTreeCtrlData();
		HTREEITEM hDummy = emptyDta.GetTreeItem();
		CString strError = m_EmptyNode.GetErrorString();

		if (hDummy)
			pTree->SetItemText (hDummy, strError);
	}
}

void CtrfFolderNode::Expand  (CTreeCtrl* pTree, HTREEITEM hExpand)
{
	CtrfItemData::Expand  (pTree, hExpand);
}

BOOL CtrfFolderNode::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE;
	//
	// List of CaNode:
	CTypedPtrList< CObList, CaDBObject* > lNew;
	//
	// Refresh only the branch has already been expanded:
	if (pInfo && pInfo->GetAction() == CaRefreshTreeInfo::ACTION_REFRESH && !GetTreeCtrlData().IsAlreadyExpanded())
		return TRUE;
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
		CtrfNode* pNewObject = new CtrfNode (pNew);
		pNewObject->SetBackParent (this);
		pNewObject->Initialize();
		m_listObject.AddTail (pNewObject);

		delete pNew;
	}

	//
	// Refresh Sub-Branches ?

	Display (pTree, hItem);
	return TRUE;
}


CtrfItemData* CtrfFolderNode::SearchObject (CaNode* pObj)
{
	POSITION pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfNode* ptrfObj = (CtrfNode*)m_listObject.GetNext (pos);
		CaNode& obj = ptrfObj->GetObject();
		if (pObj && pObj->GetName().CompareNoCase (obj.GetName()) == 0)
			return ptrfObj;
	}
	return NULL;
}

