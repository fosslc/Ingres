/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : trfgrant.cpp: Implementation for the CtrfGrantee, CtrfFolderGrantee class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Grantee object and folder
**
** History:
**
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**    Created
**/

#include "stdafx.h"
#include "trfgrant.h"
#include "ingobdml.h"
#include "constdef.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//
// Object: CtrfGrantPrivilege
// ************************************************************************************************
IMPLEMENT_SERIAL (CtrfGrantPrivilege, CtrfItemData, 1)


//
// Object: CtrfGrantee
// ************************************************************************************************
IMPLEMENT_SERIAL (CtrfGrantee, CtrfItemData, 1)
void CtrfGrantee::Initialize()
{
	CaGrantee& grantee = GetObject();
	CTypedPtrList < CObList, CaPrivilegeItem* >& lp = grantee.GetListPrivilege();
	POSITION pos = lp.GetHeadPosition();
	while (pos != NULL)
	{
		CaPrivilegeItem* pObj = lp.GetNext(pos);
		CtrfGrantPrivilege* pPrivilege = new CtrfGrantPrivilege(pObj);
		pPrivilege->SetBackParent(this);
		m_listObject.AddTail(pPrivilege);
	}
}


//
// Object: Folder of Grantee
// ************************************************************************************************
IMPLEMENT_SERIAL (CtrfFolderGrantee, CtrfItemData, 1)
CtrfFolderGrantee::CtrfFolderGrantee(): CtrfItemData(_T("Grantees"), O_FOLDER_GRANTEE)
{
	m_bDisplayGranteePrivilege = FALSE;
}

CtrfFolderGrantee::CtrfFolderGrantee(int nObjectParentType): CtrfItemData(_T("Grantees"), O_FOLDER_GRANTEE)
{
	m_EmptyNode.SetDisplayedItem (_T("<No Grantees>"));
	m_EmptyNode.SetBackParent(this);
	m_treeCtrlData.SetImage (IM_FOLDER_EMPTY, IM_FOLDER_EMPTY);
	m_nObjectParentType = nObjectParentType;
	m_bDisplayGranteePrivilege = FALSE;
}

CtrfFolderGrantee::~CtrfFolderGrantee()
{
}

UINT CtrfFolderGrantee::GetFolderFlag()
{
	return 0; //FOLDER_GRANTEE;
}


CaLLQueryInfo* CtrfFolderGrantee::GetQueryInfo(CaLLQueryInfo* pData)
{
	if (!pData)
	{
		CaLLQueryInfo info;
		info.SetObjectType (OBT_GRANTEE);
		info.SetSubObjectType (m_nObjectParentType);
		return m_pBackParent? m_pBackParent->GetQueryInfo(&info): NULL;
	}
	else
	{
		pData->SetObjectType(OBT_GRANTEE);
		pData->SetSubObjectType(m_nObjectParentType);
		return m_pBackParent? m_pBackParent->GetQueryInfo(pData): NULL;
	}
}

void CtrfFolderGrantee::SetImageID(CaDisplayInfo* pDisplayInfo)
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

CtrfItemData* CtrfFolderGrantee::SearchObject(CaGrantee* pObj)
{
	POSITION pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfGrantee* pData = (CtrfGrantee*)m_listObject.GetNext (pos);
		CaGrantee& obj = pData->GetObject();
		if (pObj && pObj->GetName().CompareNoCase(obj.GetName()) == 0 )
		{
			return pData;
		}
	}
	return NULL;
}



void CtrfFolderGrantee::Expand  (CTreeCtrl* pTree, HTREEITEM hExpand)
{
	TRACE0 ("CtrfFolderGrantee::Expand \n");
	BOOL bAlreadyExpanded = m_treeCtrlData.IsAlreadyExpanded();
	CtrfItemData::Expand(pTree, hExpand);
	if (!bAlreadyExpanded)
	{
		CaRefreshTreeInfo info;
		info.SetInfo (0);
		RefreshData (pTree, hExpand, &info);
	}
}


BOOL CtrfFolderGrantee::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
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
		CaGrantee* pNew = (CaGrantee*)lNew.RemoveHead();
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
		CtrfGrantee* pNewObject = new CtrfGrantee (pNew);
		pNewObject->SetBackParent (this);
		if (m_bDisplayGranteePrivilege)
			pNewObject->Initialize();
		m_listObject.AddTail (pNewObject);

		delete pNew;
	}

	//
	// Refresh Sub-Branches ?
	Display (pTree, hItem);
	return TRUE;
}


