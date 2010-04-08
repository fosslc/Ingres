/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : trfdbase.cpp: implementation for the CtrfDatabase, CtrfFolderDatabase class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Database object and folder
**
** History:
**
** 05-Feb-1998 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

#include "stdafx.h"
#include "trfdbase.h"
#include "trfgrant.h"
#include "ingobdml.h"
#include "constdef.h"
#include "usrexcep.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Object: Database 
// ************************************************************************************************
IMPLEMENT_SERIAL (CtrfDatabase, CtrfItemData, 1)
CtrfDatabase::~CtrfDatabase()
{
}


void CtrfDatabase::Initialize()
{
	CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
	ASSERT (pDisplayInfo);
	if (!pDisplayInfo)
		return;
	UINT nNodeFlag = pDisplayInfo->DBChildrenFolderGetFlag();

	//
	// Folder Table:
	if (nNodeFlag & FOLDER_TABLE)
	{
		CtrfFolderTable* pFolder = new CtrfFolderTable();
		pFolder->SetBackParent(this);
		m_listObject.AddTail (pFolder);
	}

	//
	// Folder View:
	if (nNodeFlag & FOLDER_VIEW)
	{
		CtrfFolderView* pFolder = new CtrfFolderView();
		pFolder->SetBackParent(this);
		m_listObject.AddTail (pFolder);
	}

	//
	// Folder Procedure:
	if (nNodeFlag & FOLDER_PROCEDURE)
	{
		CtrfFolderProcedure* pFolder = new CtrfFolderProcedure();
		pFolder->SetBackParent(this);
		m_listObject.AddTail (pFolder);
	}

	//
	// Folder DBEvent:
	if (nNodeFlag & FOLDER_DBEVENT)
	{
		CtrfFolderDBEvent* pFolder = new CtrfFolderDBEvent();
		pFolder->SetBackParent(this);
		m_listObject.AddTail (pFolder);
	}

	//
	// Folder Synonym:
	if (nNodeFlag & FOLDER_SYNONYM)
	{
		CtrfFolderSynonym* pFolder = new CtrfFolderSynonym();
		pFolder->SetBackParent(this);
		m_listObject.AddTail (pFolder);
	}

	//
	// Folder Grantee:
	if (nNodeFlag & FOLDER_GRANTEEDATABASE)
	{
		CtrfFolderGrantee* pFolder = new CtrfFolderGrantee(OBT_DATABASE);
		pFolder->SetBackParent(this);
		pFolder->SetDisplayGranteePrivileges(TRUE);
		m_listObject.AddTail (pFolder);
	}

	//
	// Folder Security Alarm:
	if (nNodeFlag & FOLDER_ALARMDATABASE)
	{
		CtrfFolderAlarm* pFolder = new CtrfFolderAlarm(OBT_DATABASE);
		pFolder->SetBackParent (this);
		m_listObject.AddTail(pFolder);
	}
}

CtrfItemData* CtrfDatabase::GetSubFolder(int nFolderType)
{
	POSITION pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* pFolder = m_listObject.GetNext (pos);
		if (pFolder->GetTreeItemType()== nFolderType)
			return pFolder;
	}
	return NULL;
}


void CtrfDatabase::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	int nImage, nImageExpand;

	pDisplayInfo->GetDatabaseImage  (nImage, nImageExpand);
	m_treeCtrlData.SetImage (nImage, nImageExpand);
}

CaLLQueryInfo* CtrfDatabase::GetQueryInfo(CaLLQueryInfo* pData)
{
	if (!pData)
	{
		CaLLQueryInfo info;
		info.SetDatabase (m_object.GetName());
		return m_pBackParent? m_pBackParent->GetQueryInfo(&info): NULL;
	}
	else
	{
		pData->SetDatabase (m_object.GetName());
		return m_pBackParent? m_pBackParent->GetQueryInfo(pData): NULL;
	}
}


UINT CtrfDatabase::GetDisplayObjectFlag(CaDisplayInfo* pDisplayInfo)
{
	return pDisplayInfo->DBChildrenFolderGetFlag();
}


void CtrfDatabase::Expand  (CTreeCtrl* pTree, HTREEITEM hExpand)
{
	CtrfItemData::Expand(pTree, hExpand);
}

BOOL CtrfDatabase::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE;
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
	catch (...)
	{

	}
	return TRUE;
}


//
// Folder of Database:
// ************************************************************************************************
IMPLEMENT_SERIAL (CtrfFolderDatabase, CtrfItemData, 1)
CtrfFolderDatabase::CtrfFolderDatabase(): CtrfItemData(_T("Databases"), O_FOLDER_DATABASE)
{
	m_EmptyNode.SetDisplayedItem (_T("<No Databases>"));
	m_EmptyNode.SetBackParent(this);
	m_treeCtrlData.SetImage (IM_FOLDER_EMPTY, IM_FOLDER_EMPTY);
}

CtrfFolderDatabase::~CtrfFolderDatabase()
{
}


CaLLQueryInfo* CtrfFolderDatabase::GetQueryInfo(CaLLQueryInfo* pData)
{
	if (!pData)
	{
		CaLLQueryInfo info;
		info.SetObjectType (OBT_DATABASE);
		return m_pBackParent? m_pBackParent->GetQueryInfo(&info): NULL;
	}
	else
	{
		return m_pBackParent? m_pBackParent->GetQueryInfo(pData): NULL;
	}
}



CtrfDatabase* CtrfFolderDatabase::SearchObject(CaDatabase* pObj)
{
	POSITION pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfDatabase* ptrfObj = (CtrfDatabase*)m_listObject.GetNext (pos);
		CaDatabase& database = ptrfObj->GetObject();
		if (pObj && pObj->GetName().CompareNoCase (database.GetName()) == 0)
			return ptrfObj;
	}
	return NULL;
}

void CtrfFolderDatabase::SetImageID(CaDisplayInfo* pDisplayInfo)
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


void CtrfFolderDatabase::Expand  (CTreeCtrl* pTree, HTREEITEM hExpand)
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




BOOL CtrfFolderDatabase::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE;
	CTypedPtrList< CObList, CaDBObject* > lNew;
	//
	// Refresh only the branch has already been expanded:
	if (pInfo && pInfo->GetAction() == CaRefreshTreeInfo::ACTION_REFRESH && !GetTreeCtrlData().IsAlreadyExpanded())
		return TRUE;
	CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
	ASSERT (pQueryInfo);
	if (!pQueryInfo)
		return FALSE;

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
		CtrfDatabase* pNewObject = new CtrfDatabase (pNew);
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








