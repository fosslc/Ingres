/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dmlsynon.cpp: implementation for the CaSynonym, CaFolderSynonym class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Synonym object and folder
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

#include "stdafx.h"
#include "trfsynon.h"
#include "ingobdml.h"
#include "constdef.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Object: Synonym:
// ****************
IMPLEMENT_SERIAL (CtrfSynonym, CtrfItemData, 1)
CString CtrfSynonym::GetDisplayedItem(int nMode)
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
// Folder of Synonym:
// ******************
IMPLEMENT_SERIAL (CtrfFolderSynonym, CtrfItemData, 1)
CtrfFolderSynonym::CtrfFolderSynonym(): CtrfItemData(_T("Synonyms"), O_FOLDER_SYNONYM)
{
	m_EmptyNode.SetDisplayedItem (_T("<No Synonyms>"));
	m_EmptyNode.SetBackParent(this);
	m_treeCtrlData.SetImage (IM_FOLDER_EMPTY, IM_FOLDER_EMPTY);
}

CtrfFolderSynonym::~CtrfFolderSynonym()
{
	while (!m_listObject.IsEmpty())
		delete m_listObject.RemoveHead();
}

UINT CtrfFolderSynonym::GetFolderFlag()
{
	return FOLDER_SYNONYM;
}

CaLLQueryInfo* CtrfFolderSynonym::GetQueryInfo(CaLLQueryInfo* pData)
{
	if (!pData)
	{
		CaLLQueryInfo info;
		info.SetObjectType (OBT_SYNONYM);
		return m_pBackParent? m_pBackParent->GetQueryInfo(&info): NULL;
	}
	else
	{
		return m_pBackParent? m_pBackParent->GetQueryInfo(pData): NULL;
	}
}

void CtrfFolderSynonym::SetImageID(CaDisplayInfo* pDisplayInfo)
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

CtrfItemData* CtrfFolderSynonym::SearchObject (CaSynonym* pObj, BOOL bMatchOwner)
{
	POSITION pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfSynonym* pData = (CtrfSynonym*)m_listObject.GetNext (pos);
		CaSynonym& obj = pData->GetObject();
		if (pObj && pObj->GetName().CompareNoCase(obj.GetName()) == 0 )
		{
			if (!bMatchOwner)
				return pData;
			else
			if (pObj->GetOwner().CompareNoCase (obj.GetOwner()) == 0)
				return pData;
		}
	}
	return NULL;
}


void CtrfFolderSynonym::Expand  (CTreeCtrl* pTree, HTREEITEM hExpand)
{
	TRACE0 ("CtrfFolderSynonym::Expand \n");
	BOOL bAlreadyExpanded = m_treeCtrlData.IsAlreadyExpanded();
	CtrfItemData::Expand(pTree, hExpand);
	if (!bAlreadyExpanded)
	{
		CaRefreshTreeInfo info;
		info.SetInfo (0);
		RefreshData (pTree, hExpand, &info);
	}
}


BOOL CtrfFolderSynonym::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
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
		CaSynonym* pNew = (CaSynonym*)lNew.RemoveHead();
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
		CtrfSynonym* pNewObject = new CtrfSynonym (pNew);
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

