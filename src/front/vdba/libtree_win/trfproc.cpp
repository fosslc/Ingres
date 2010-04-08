/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dmlproc.h: interface for the CtrfProcedure, CtrfFolderProcedure class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Procedure object and folder
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#include "stdafx.h"
#include "trfproc.h"
#include "trfgrant.h"
#include "ingobdml.h"
#include "constdef.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//
// Object: Procedure 
// ************************************************************************************************
IMPLEMENT_SERIAL (CtrfProcedure, CtrfItemData, 1)
CString CtrfProcedure::GetDisplayedItem(int nMode)
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

void CtrfProcedure::Initialize()
{
	CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
	//
	// pDisplayInfo must be accessible, it must be in the root parent of 
	// the three hierarchy !
	ASSERT(pDisplayInfo);
	if (!pDisplayInfo)
		return;
	UINT nNodeFlag = pDisplayInfo->ProcedureChildrenFolderGetFlag();

	//
	// Folder Grantee:
	if (nNodeFlag & FOLDER_GRANTEEPROCEDURE)
	{
		CtrfFolderGrantee* pFolder = new CtrfFolderGrantee(OBT_PROCEDURE);
		pFolder->SetBackParent(this);
		pFolder->SetDisplayGranteePrivileges(TRUE);
		m_listObject.AddTail (pFolder);
	}
}

CaLLQueryInfo* CtrfProcedure::GetQueryInfo(CaLLQueryInfo* pData)
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


//
// Folder of Procedure:
// ************************************************************************************************
IMPLEMENT_SERIAL (CtrfFolderProcedure, CtrfItemData, 1)
CtrfFolderProcedure::CtrfFolderProcedure(): CtrfItemData(_T("Procedures"), O_FOLDER_PROCEDURE)
{
	m_EmptyNode.SetDisplayedItem (_T("<No Procedures>"));
	m_EmptyNode.SetBackParent(this);
	m_treeCtrlData.SetImage (IM_FOLDER_EMPTY, IM_FOLDER_EMPTY);
}

CtrfFolderProcedure::~CtrfFolderProcedure()
{
	while (!m_listObject.IsEmpty())
		delete m_listObject.RemoveHead();
}

UINT CtrfFolderProcedure::GetFolderFlag()
{
	return FOLDER_PROCEDURE;
}

CaLLQueryInfo* CtrfFolderProcedure::GetQueryInfo(CaLLQueryInfo* pData)
{
	if (!pData)
	{
		CaLLQueryInfo info;
		info.SetObjectType (OBT_PROCEDURE);
		return m_pBackParent? m_pBackParent->GetQueryInfo(&info): NULL;
	}
	else
	{
		return m_pBackParent? m_pBackParent->GetQueryInfo(pData): NULL;
	}
}

void CtrfFolderProcedure::SetImageID(CaDisplayInfo* pDisplayInfo)
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

CtrfItemData* CtrfFolderProcedure::SearchObject (CaProcedure* pObj, BOOL bMatchOwner)
{
	POSITION pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfProcedure* pData = (CtrfProcedure*)m_listObject.GetNext (pos);
		CaProcedure& obj = pData->GetObject();
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


void CtrfFolderProcedure::Expand  (CTreeCtrl* pTree, HTREEITEM hExpand)
{
	TRACE0 ("CtrfFolderProcedure::Expand \n");
	BOOL bAlreadyExpanded = m_treeCtrlData.IsAlreadyExpanded();
	CtrfItemData::Expand(pTree, hExpand);
	if (!bAlreadyExpanded)
	{
		CaRefreshTreeInfo info;
		info.SetInfo (0);
		RefreshData (pTree, hExpand, &info);
	}
}

BOOL CtrfFolderProcedure::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
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
		CaProcedure* pNew = (CaProcedure*)lNew.RemoveHead();
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
		CtrfProcedure* pNewObject = new CtrfProcedure (pNew);
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

