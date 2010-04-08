/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dmltable.cpp: Implementation for the CtrfFolderTable, CtrfFolderTable class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Table object and folder
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
**/

#include "stdafx.h"
#include "trftable.h"
#include "trfgrant.h"
#include "trfalarm.h"
#include "ingobdml.h"
#include "constdef.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//
// Object: Table 
// ************************************************************************************************
IMPLEMENT_SERIAL (CtrfTable, CtrfItemData, 1)
CtrfTable::~CtrfTable()
{

}

void CtrfTable::Initialize()
{
	CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
	ASSERT (pDisplayInfo);
	if (!pDisplayInfo)
		return;
	UINT nNodeFlag = pDisplayInfo->TableChildrenFolderGetFlag();

	//
	// Folder Column:
	if (nNodeFlag & FOLDER_COLUMN)
	{
		CtrfFolderColumn* pFolder = new CtrfFolderColumn();
		pFolder->SetBackParent (this);
		m_listObject.AddTail(pFolder);
		m_pFolderColumn = pFolder;
	}

	//
	// Folder Index:
	if (nNodeFlag & FOLDER_INDEX)
	{
		CtrfFolderIndex* pFolder = new CtrfFolderIndex();
		pFolder->SetBackParent (this);
		m_listObject.AddTail(pFolder);
		m_pFolderIndex = pFolder;
	}

	//
	// Folder Rule:
	if (nNodeFlag & FOLDER_RULE)
	{
		CtrfFolderRule* pFolder = new CtrfFolderRule();
		pFolder->SetBackParent (this);
		m_listObject.AddTail(pFolder);
		m_pFolderRule = pFolder;
	}

	//
	// Folder Integrity:
	if (nNodeFlag & FOLDER_INTEGRITY)
	{
		CtrfFolderIntegrity* pFolder = new CtrfFolderIntegrity();
		pFolder->SetBackParent (this);
		m_listObject.AddTail(pFolder);
		m_pFolderIntegrity = pFolder;
	}

	//
	// Folder Grantee:
	if (nNodeFlag & FOLDER_GRANTEETABLE)
	{
		CtrfFolderGrantee* pFolder = new CtrfFolderGrantee(OBT_TABLE);
		pFolder->SetBackParent (this);
		pFolder->SetDisplayGranteePrivileges(TRUE);
		m_listObject.AddTail(pFolder);
		m_pFolderGrantee = pFolder;
	}

	//
	// Folder Security Alarm:
	if (nNodeFlag & FOLDER_ALARMTABLE)
	{
		CtrfFolderAlarm* pFolder = new CtrfFolderAlarm(OBT_TABLE);
		pFolder->SetBackParent (this);
		m_listObject.AddTail(pFolder);
		m_pFolderAlarm = pFolder;
	}
}


void CtrfTable::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	int nImage, nImageExpand;

	pDisplayInfo->GetTableImage (nImage, nImageExpand);
	m_treeCtrlData.SetImage (nImage, nImageExpand);
}


CaLLQueryInfo* CtrfTable::GetQueryInfo(CaLLQueryInfo* pData)
{
	if (!pData)
	{
		CaLLQueryInfo info;
		info.SetItem2(m_object.GetName(), m_object.GetOwner());
		return m_pBackParent? m_pBackParent->GetQueryInfo(&info): NULL;
	}
	else
	{
		pData->SetItem2(m_object.GetName(), m_object.GetOwner());
		return m_pBackParent? m_pBackParent->GetQueryInfo(pData): NULL;
	}
}


CString CtrfTable::GetDisplayedItem(int nMode)
{
	CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
	//
	// pDisplayInfo must be accessible, it must be in the root parent of 
	// the three hierarchy !
	ASSERT(pDisplayInfo);
	if (!pDisplayInfo)
		return m_object.GetName();
	CString strOWNERxITEM = pDisplayInfo->GetOwnerPrefixedFormat();
	if (!strOWNERxITEM.IsEmpty() && !m_object.GetOwner().IsEmpty())
	{
		CString strDisplay;
		switch (nMode)
		{
		case 0:
			strDisplay.Format (strOWNERxITEM, (LPCTSTR)m_object.GetOwner(), (LPCTSTR)m_object.GetName());
			break;
		case 1:
			strDisplay.Format (strOWNERxITEM, (LPCTSTR)m_object.GetName(), (LPCTSTR)m_object.GetOwner());
			break;
		default:
			ASSERT(FALSE);
			break;
		}
		return strDisplay;
	}

	return m_object.GetName();
}





void CtrfTable::Expand  (CTreeCtrl* pTree, HTREEITEM hExpand)
{
	CtrfItemData::Expand(pTree, hExpand);
	TRACE0("CtrfFolderTable::Expand \n");
}

BOOL CtrfTable::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
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
// Folder of Table:
// ************************************************************************************************
IMPLEMENT_SERIAL (CtrfFolderTable, CtrfItemData, 1)
CtrfFolderTable::CtrfFolderTable(): CtrfItemData(_T("Tables"), O_FOLDER_TABLE)
{
	m_EmptyNode.SetDisplayedItem (_T("<No Tables>"));
	m_EmptyNode.SetBackParent (this);
	m_treeCtrlData.SetImage (IM_FOLDER_EMPTY, IM_FOLDER_EMPTY);
}

CtrfFolderTable::~CtrfFolderTable()
{
}

void CtrfFolderTable::SetImageID(CaDisplayInfo* pDisplayInfo)
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

CaLLQueryInfo* CtrfFolderTable::GetQueryInfo(CaLLQueryInfo* pData)
{
	if (!pData)
	{
		CaLLQueryInfo info;
		info.SetObjectType(OBT_TABLE);
		return m_pBackParent? m_pBackParent->GetQueryInfo(&info): NULL;
	}
	else
	{
		return m_pBackParent? m_pBackParent->GetQueryInfo(pData): NULL;
	}
}

CtrfItemData* CtrfFolderTable::SearchObject (CaTable* pObj, BOOL bMatchOwner)
{
	POSITION pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfTable* ptrfObj = (CtrfTable*)m_listObject.GetNext (pos);
		CaTable& obj = ptrfObj->GetObject();
		if (pObj && pObj->GetName().CompareNoCase  (obj.GetName())  == 0)
		{
			if (!bMatchOwner)
				return ptrfObj;
			else
			if (pObj->GetOwner().CompareNoCase (obj.GetOwner()) == 0)
				return ptrfObj;
		}
	}
	return NULL;
}



void CtrfFolderTable::Expand  (CTreeCtrl* pTree, HTREEITEM hExpand)
{
	TRACE0 ("CtrfFolderTable::Expand \n");

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


BOOL CtrfFolderTable::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
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
		CtrfTable* pNewObject = new CtrfTable (pNew);
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


