/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dmlalarm.cpp: implementation of the CtrfAlarm, CtrfFolderAlarm class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Alarm object and Folder
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

#include "stdafx.h"
#include "trfvnode.h"
#include "trfalarm.h"
#include "usrexcep.h"
#include "constdef.h"
#include "ingobdml.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Object: Alarm
// ************************************************************************************************
IMPLEMENT_SERIAL (CtrfAlarm, CtrfItemData, 1)
CString CtrfAlarm::GetDisplayedItem(int nMode)
{
	CString strDisplay;
	CaAlarm& alarm = GetObject();
	CString strAuth = (alarm.GetSubjectType() == _T('P'))? _T("(public)"): alarm.GetSecurityUser();
	strDisplay.Format(
		_T("(%d / %s) %s"), 
		(LPCTSTR)alarm.GetSecurityNumber(), 
		(LPCTSTR)alarm.GetName(), 
		(LPCTSTR)strAuth);
	return strDisplay;
}




//
// Folder of Alarm:
// ************************************************************************************************
IMPLEMENT_SERIAL (CtrfFolderAlarm, CtrfItemData, 1)
CtrfFolderAlarm::CtrfFolderAlarm(): CtrfItemData(_T("Security Alarms"), O_FOLDER_ALARM)
{
	m_EmptyNode.SetDisplayedItem (_T("<No Alarms>"));
	m_EmptyNode.SetBackParent(this);
	m_treeCtrlData.SetImage (IM_FOLDER_EMPTY, IM_FOLDER_EMPTY);
	m_nObjectParentType = OBT_TABLE;
}

CtrfFolderAlarm::CtrfFolderAlarm(int nObjectParentType): CtrfItemData(_T("Security Alarms"), O_FOLDER_ALARM)
{
	m_EmptyNode.SetDisplayedItem (_T("<No Alarms>"));
	m_EmptyNode.SetBackParent(this);
	m_treeCtrlData.SetImage (IM_FOLDER_EMPTY, IM_FOLDER_EMPTY);
	m_nObjectParentType = nObjectParentType;
}

CtrfFolderAlarm::~CtrfFolderAlarm()
{
}


UINT CtrfFolderAlarm::GetFolderFlag()
{
	return FOLDER_COLUMN;
}

CaLLQueryInfo* CtrfFolderAlarm::GetQueryInfo(CaLLQueryInfo* pData)
{
	if (!pData)
	{
		CaLLQueryInfo info;
		info.SetObjectType (OBT_ALARM);
		info.SetSubObjectType (m_nObjectParentType);
		return m_pBackParent? m_pBackParent->GetQueryInfo(&info): NULL;
	}
	else
	{
		pData->SetObjectType (OBT_ALARM);
		pData->SetSubObjectType(m_nObjectParentType);
		return m_pBackParent? m_pBackParent->GetQueryInfo(pData): NULL;
	}
}

CtrfItemData* CtrfFolderAlarm::SearchObject(CaAlarm* pObj)
{
	POSITION pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfAlarm* ptrfObj = (CtrfAlarm*)m_listObject.GetNext (pos);
		CaAlarm& object = ptrfObj->GetObject();
		if (pObj && 
#if defined (_MATCH_ALARM_NAME)
		    pObj->GetName().CompareNoCase (object.GetName()) == 0 &&
#endif
		    pObj->GetSecurityNumber() == object.GetSecurityNumber())
			return ptrfObj;
	}
	return NULL;
}


void CtrfFolderAlarm::SetImageID(CaDisplayInfo* pDisplayInfo)
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


void CtrfFolderAlarm::Expand  (CTreeCtrl* pTree, HTREEITEM hExpand)
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

BOOL CtrfFolderAlarm::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
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
		CaAlarm* pNew = (CaAlarm*)lNew.RemoveHead();
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
		CtrfAlarm* pNewObject = new CtrfAlarm (pNew);
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


