/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dmlindex.cpp: implementation for the CtrfIndex, CtrfFolderIndex class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Index object and folder
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

#include "stdafx.h"
#include "trfindex.h"
#include "constdef.h"
#include "ingobdml.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Object: Index 
// *************
IMPLEMENT_SERIAL (CtrfIndex, CtrfItemData, 1)
CString CtrfIndex::GetDisplayedItem(int nMode)
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



//
// Folder of Index:
// ****************
IMPLEMENT_SERIAL (CtrfFolderIndex, CtrfItemData, 1)
CtrfFolderIndex::CtrfFolderIndex(): CtrfItemData(_T("Indexes"), O_FOLDER_INDEX)
{
	m_EmptyNode.SetDisplayedItem (_T("<No Indexes>"));
	m_EmptyNode.SetBackParent(this);
	m_treeCtrlData.SetImage (IM_FOLDER_EMPTY, IM_FOLDER_EMPTY);
}

CtrfFolderIndex::~CtrfFolderIndex()
{
	while (!m_listObject.IsEmpty())
		delete m_listObject.RemoveHead();
}

UINT CtrfFolderIndex::GetFolderFlag()
{
	return FOLDER_INDEX;
}

CaLLQueryInfo* CtrfFolderIndex::GetQueryInfo(CaLLQueryInfo* pData)
{
	if (!pData)
	{
		CaLLQueryInfo info;
		info.SetObjectType (OBT_INDEX);

		return m_pBackParent? m_pBackParent->GetQueryInfo(&info): NULL;
	}
	else
	{
		return m_pBackParent? m_pBackParent->GetQueryInfo(pData): NULL;
	}
}

void CtrfFolderIndex::SetImageID(CaDisplayInfo* pDisplayInfo)
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


CtrfItemData* CtrfFolderIndex::SearchObject(CaIndex* pObj)
{
	POSITION pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfIndex* ptrfObj = (CtrfIndex*)m_listObject.GetNext (pos);
		CaIndex& object = ptrfObj->GetObject();
		if (pObj && pObj->GetName().CompareNoCase (object.GetName()) == 0)
			return ptrfObj;
	}
	return NULL;
}




void CtrfFolderIndex::Expand  (CTreeCtrl* pTree, HTREEITEM hExpand)
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


BOOL CtrfFolderIndex::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
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
		CaIndex* pNew = (CaIndex*)lNew.RemoveHead();
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
		CtrfIndex* pNewObject = new CtrfIndex (pNew);
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


