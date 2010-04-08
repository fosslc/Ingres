/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vsdtree.cpp : main tree of the left pane
**    Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Tree of ingres object (starting from database)
**
** History:
**
** 26-Aug-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    created
** 22-Apr-2003 (schph01)
**    SIR 107523 Add Sequence Object
** 06-Jan-2004 (schph01)
**    SIR #111462, Put string into resource files
** 19-Jan-2004 (schph01)
**    SIR #111462, Put string into resource files
** 18-Aug-2004 (uk$so01)
**    BUG #112841 / ISSUE #13623161, Allow user to cancel the Comparison
**    operation.
** 29-Sep-2004 (uk$so01)
**    BUG #113119, Add readlock mode in the session management and
**    optimize the display.
** 22-Nov-2004 (uk$so01)
**    BUG #113505 / #ISSUE 13755286, Fix the incorrect specified database
**    when trying to query the rules of two databases.
*/

#include "stdafx.h"
#include "vsdtree.h"
#include "constdef.h"
#include "usrexcep.h"
#include "ingobdml.h"
#include "vsddml.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static void FolderSetImageID(CtrfItemData* pItem)
{
	CaVsdItemData* pUserData = (CaVsdItemData*)pItem->GetUserData();
	ASSERT(pUserData);
	if (!pUserData)
		return;
	if (pUserData->IsDiscarded())
	{
		pItem->GetTreeCtrlData().SetImage (IM_VSDTRFd, IM_VSDTRFd);
	}
	else
	{
		BOOL bHasDiscardInSubBranches = FALSE;
		TCHAR ch = pUserData->HasDifferences();
		VSD_HasDiscadItem(pItem, bHasDiscardInSubBranches, TRUE);
		if (bHasDiscardInSubBranches)
		{
			switch (ch)
			{
			case _T('='):
				pItem->GetTreeCtrlData().SetImage (IM_VSDTRFd_FOLDER_CLOSE_GG, IM_VSDTRFd_FOLDER_OPEN_GG);
				break;
			default:
				pItem->GetTreeCtrlData().SetImage (IM_VSDTRFd_FOLDER_CLOSE_YY, IM_VSDTRFd_FOLDER_OPEN_YY);
				break;
			}
		}
		else
		{
			switch (ch)
			{
			case _T('='):
				pItem->GetTreeCtrlData().SetImage (IM_VSDTRF_FOLDER_CLOSE_GG, IM_VSDTRF_FOLDER_OPEN_GG);
				break;
			default:
				pItem->GetTreeCtrlData().SetImage (IM_VSDTRF_FOLDER_CLOSE_YY, IM_VSDTRF_FOLDER_OPEN_YY);
				break;
			}
		}
	}
}

//
// Object: CaVsdRefreshTreeInfo 
// ************************************************************************************************
CString CaVsdRefreshTreeInfo::GetNode1()
{
	if (m_bLoadSchema1)
		return m_LoadedSchema1.GetNode();
	else
		return m_strNode1;
}

CString CaVsdRefreshTreeInfo::GetNode2()
{
	if (m_bLoadSchema2)
		return m_LoadedSchema2.GetNode();
	else
		return m_strNode2;
	return m_strNode2;
}

CString CaVsdRefreshTreeInfo::GetDatabase1()
{
	if (m_bInstallation)
		return m_strDatabase1;
	if (m_bLoadSchema1)
		return m_LoadedSchema1.GetDatabase();
	else
		return m_strDatabase1;
}

CString CaVsdRefreshTreeInfo::GetDatabase2()
{
	if (m_bInstallation)
		return m_strDatabase2;

	if (m_bLoadSchema2)
		return m_LoadedSchema2.GetDatabase();
	else
		return m_strDatabase2;
}

CString CaVsdRefreshTreeInfo::GetSchema1()
{
	if (m_bLoadSchema1)
		return m_LoadedSchema1.GetSchema();
	else
		return m_strSchema1;
}

CString CaVsdRefreshTreeInfo::GetSchema2()
{
	if (m_bLoadSchema2)
		return m_LoadedSchema2.GetSchema();
	else
		return m_strSchema2;
}

void CaVsdRefreshTreeInfo::ShowAnimateTextInfo(LPCTSTR lpszText)
{
	if (m_hWndAnimateDlg)
	{
		LPTSTR lpszMsg = new TCHAR[_tcslen(lpszText) + 1];
		lstrcpy (lpszMsg, lpszText);
		::PostMessage (m_hWndAnimateDlg, WM_EXECUTE_TASK, W_EXTRA_TEXTINFO, (LPARAM)lpszMsg);
	}
}

//
// Object: CaVsdItemData 
// ************************************************************************************************
CaVsdItemData::CaVsdItemData(CtrfItemData* pBackParent)
{
	m_tchszDifference = _T('\0');
	m_bMultiple = FALSE;
	m_strOwner2 = _T("");
	m_bDiscarded = FALSE;
	VsdFolder(pBackParent);
}

CaVsdItemData::~CaVsdItemData()
{
	while (!m_listDifference.IsEmpty())
		delete m_listDifference.RemoveHead();
}



TCHAR CaVsdItemData::HasDifferences (BOOL bIncludeSubFolder)
{
	ASSERT(m_pBackObject);
	if (!m_pBackObject)
		return _T('=');
	if (!m_pBackObject->DisplayItem() || IsDiscarded())
		return _T('=');

	switch (m_tchszDifference)
	{
	case _T('+'):
	case _T('-'):
		return m_tchszDifference;
	default:
		if (!m_listDifference.IsEmpty())
			return _T('!');
		break;
	}

	if (bIncludeSubFolder)
	{
		CTypedPtrList< CObList, CtrfItemData* >& ls = m_pBackObject->GetListObject();
		POSITION pos = ls.GetHeadPosition();
		while (pos != NULL)
		{
			CtrfItemData* ptrfObj = ls.GetNext(pos);
			if (!ptrfObj->DisplayItem())
				continue;
			CaVsdItemData* pObj = (CaVsdItemData*)ptrfObj->GetUserData();
			ASSERT(pObj);
			if (pObj)
			{
				if (pObj->IsDiscarded())
					continue;
				if (pObj->HasDifferences(bIncludeSubFolder) != _T('='))
					return _T('!');
			}
		}
	}

	return _T('=');
}

//
// Object: Security Alarm 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdAlarm, CtrfAlarm, 1)
BOOL CaVsdAlarm::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE, bOk2 = FALSE;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
/*UKS
	try
	{
*/
		CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
		ASSERT (pDisplayInfo);
		if (!pDisplayInfo)
			return FALSE;

		if (GetDifference() == _T('*') || GetDifference() == _T('#'))
		{
			//
			// Compare Detail info of the two objects:
			CaAlarm& obj = GetObject();
			CString strMsg;
			strMsg.Format (IDS_COMPARE_SECURITY, (LPCTSTR)obj.GetName(), (LPCTSTR)obj.GetName());/* _T("compare security alarm %s <--> %s \n")*/
			TRACE0 (strMsg);

			CaVsdFolderAlarm* pBackParent = (CaVsdFolderAlarm*)m_pBackParent;
			CaVsdItemData* pVsdParent = pBackParent->GetVsdParentItem();
			CmtSessionManager* pSessionManager = GetSessionManager();
			CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
			CaLLQueryInfo queryInfo2 (*pQueryInfo);
			queryInfo2.SetObjectType(OBT_ALARM);
			queryInfo2.SetNode(pRefreshInfo->GetNode2());
			queryInfo2.SetDatabase(pRefreshInfo->GetDatabase2());
			queryInfo2.SetItem2(pQueryInfo->GetItem2(), pVsdParent->GetOwner2());

			switch (pQueryInfo->GetSubObjectType())
			{
			case OBT_INSTALLATION:
				//strMsg.Format (_T("Installation Alarm %d"), obj.GetSecurityNumber());
				//pRefreshInfo->ShowAnimateTextInfo(strMsg);
				break;
			case OBT_DATABASE:
				//strMsg.Format (_T("Alarm: %s / %d"), (LPCTSTR)pQueryInfo->GetDatabase(), obj.GetSecurityNumber());
				//pRefreshInfo->ShowAnimateTextInfo(strMsg);
				break;
			case OBT_TABLE:
				//strMsg.Format (
				//    _T("Alarm: %s / %s %d"), 
				//    (LPCTSTR)pQueryInfo->GetDatabase(), 
				//    (LPCTSTR)pQueryInfo->GetItem2(), 
				//    obj.GetSecurityNumber());
				//pRefreshInfo->ShowAnimateTextInfo(strMsg);
				break;
			default:
				break;
			}

			CaAlarm to2;
			CaDBObject* pDt1 = &obj;
			CaDBObject* pDt2 = &to2;
			to2.SetItem(obj.GetName(), GetOwner2());
			to2.SetSecurityNumber(obj.GetSecurityNumber());

			if (pRefreshInfo->IsLoadSchema2())
				bOk2 = VSD_llQueryDetailObject(&queryInfo2, pRefreshInfo->GetLoadedSchema2(), pDt2);
			else
				bOk2  = INGRESII_llQueryDetailObject (&queryInfo2, &to2, pSessionManager);

			if (bOk2 && pDt1 && pDt2)
			{
				VSD_CompareAlarm (this, (CaAlarm*)pDt1, (CaAlarm*)pDt2);
			}
			if (pRefreshInfo->IsLoadSchema2() && pDt2)
				delete pDt2;

		}

		POSITION pos = m_listObject.GetHeadPosition();
		while (pos != NULL)
		{
			CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
			if (pRefreshInfo->IsInterrupted())
				break;
			ptrfObj->RefreshData(pTree, hItem, pInfo);
		}
/*UKS
	}
	catch (...)
	{

	}
*/
	return TRUE;
}

void CaVsdAlarm::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	int nImage = -1;
	CtrfAlarm::SetImageID(pDisplayInfo);
	if (IsDiscarded())
	{
		GetTreeCtrlData().SetImage (IM_VSDTRFd, IM_VSDTRFd);
	}
	else
	{
		BOOL bHasDiscardInSubBranches = FALSE;
		TCHAR ch = HasDifferences ();
		CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
		int nSubObjectType = pQueryInfo->GetSubObjectType();

		VSD_HasDiscadItem(this, bHasDiscardInSubBranches, TRUE);
		if (bHasDiscardInSubBranches)
		{
			switch (ch)
			{
			case _T('+'):
				switch (nSubObjectType)
				{
				case OBT_INSTALLATION:
					nImage = IM_VSDTRFd_AxINSTALLATION_GR;
					break;
				case OBT_DATABASE:
					nImage = IM_VSDTRFd_AxDATABASE_GR;
					break;
				case OBT_TABLE:
					nImage = IM_VSDTRFd_AxTABLE_GR;
					break;
				default:
					break;
				}
				GetTreeCtrlData().SetImage (nImage, nImage);
				break;
			case _T('-'):
				switch (nSubObjectType)
				{
				case OBT_INSTALLATION:
					nImage = IM_VSDTRFd_AxINSTALLATION_RG;
					break;
				case OBT_DATABASE:
					nImage = IM_VSDTRFd_AxDATABASE_RG;
					break;
				case OBT_TABLE:
					nImage = IM_VSDTRFd_AxTABLE_RG;
					break;
				default:
					break;
				}
				GetTreeCtrlData().SetImage (nImage, nImage);
				break;
			case _T('!'):
				switch (nSubObjectType)
				{
				case OBT_INSTALLATION:
					nImage = IM_VSDTRFd_AxINSTALLATION_YY;
					break;
				case OBT_DATABASE:
					nImage = IM_VSDTRFd_AxDATABASE_YY;
					break;
				case OBT_TABLE:
					nImage = IM_VSDTRFd_AxTABLE_YY;
					break;
				default:
					break;
				}
				GetTreeCtrlData().SetImage (nImage, nImage);
				break;
			default:
				switch (nSubObjectType)
				{
				case OBT_INSTALLATION:
					nImage = IM_VSDTRFd_AxINSTALLATION_GG;
					break;
				case OBT_DATABASE:
					nImage = IM_VSDTRFd_AxDATABASE_GG;
					break;
				case OBT_TABLE:
					nImage = IM_VSDTRFd_AxTABLE_GG;
					break;
				default:
					break;
				}
				GetTreeCtrlData().SetImage (nImage, nImage);
				break;
			}
		}
		else
		{
			switch (ch)
			{
			case _T('+'):
				switch (nSubObjectType)
				{
				case OBT_INSTALLATION:
					nImage = IM_VSDTRF_AxINSTALLATION_GR;
					break;
				case OBT_DATABASE:
					nImage = IM_VSDTRF_AxDATABASE_GR;
					break;
				case OBT_TABLE:
					nImage = IM_VSDTRF_AxTABLE_GR;
					break;
				default:
					break;
				}
				GetTreeCtrlData().SetImage (nImage, nImage);
				break;
			case _T('-'):
				switch (nSubObjectType)
				{
				case OBT_INSTALLATION:
					nImage = IM_VSDTRF_AxINSTALLATION_RG;
					break;
				case OBT_DATABASE:
					nImage = IM_VSDTRF_AxDATABASE_RG;
					break;
				case OBT_TABLE:
					nImage = IM_VSDTRF_AxTABLE_RG;
					break;
				default:
					break;
				}
				GetTreeCtrlData().SetImage (nImage, nImage);
				break;
			case _T('!'):
				switch (nSubObjectType)
				{
				case OBT_INSTALLATION:
					nImage = IM_VSDTRF_AxINSTALLATION_YY;
					break;
				case OBT_DATABASE:
					nImage = IM_VSDTRF_AxDATABASE_YY;
					break;
				case OBT_TABLE:
					nImage = IM_VSDTRF_AxTABLE_YY;
					break;
				default:
					break;
				}
				GetTreeCtrlData().SetImage (nImage, nImage);
				break;
			default:
				switch (nSubObjectType)
				{
				case OBT_INSTALLATION:
					nImage = IM_VSDTRF_AxINSTALLATION_GG;
					break;
				case OBT_DATABASE:
					nImage = IM_VSDTRF_AxDATABASE_GG;
					break;
				case OBT_TABLE:
					nImage = IM_VSDTRF_AxTABLE_GG;
					break;
				default:
					break;
				}
				GetTreeCtrlData().SetImage (nImage, nImage);
				break;
			}
		}
	}
}

#if defined (_DEBUG) && defined(_SHOWDIFF_SYMBOL)
CString CaVsdAlarm::GetDisplayedItem(int nMode)
{
	CString strFmt;
	strFmt.Format(_T("{%c %d}   "), GetDifference(), IsMultiple());
	strFmt+= CtrfAlarm::GetDisplayedItem(nMode);
	return strFmt;
}
#endif

//
// Object: Folder of Security Alarm 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdFolderAlarm, CtrfFolderAlarm, 1)
void CaVsdFolderAlarm::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfFolderAlarm::SetImageID(pDisplayInfo);
	FolderSetImageID(this);
}


BOOL CaVsdFolderAlarm::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE, bOk2;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
	CTypedPtrList< CObList, CaDBObject* > lNew, lNew2;
	CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
	ASSERT (pQueryInfo);
	if (!pQueryInfo)
		return FALSE;

	CaVsdItemData* pVsdParent = (CaVsdItemData*)m_pBackParent->GetUserData();
	CaLLQueryInfo queryInfo1(*pQueryInfo);
	CaLLQueryInfo queryInfo2(*pQueryInfo);
	queryInfo1.SetNode(pRefreshInfo->GetNode1());
	queryInfo1.SetDatabase(pRefreshInfo->GetDatabase1());
	queryInfo2.SetNode(pRefreshInfo->GetNode2());
	queryInfo2.SetDatabase(pRefreshInfo->GetDatabase2());
	if (pVsdParent) // NULL: if (root is the parent)
		queryInfo2.SetItem2(queryInfo1.GetItem2(), pVsdParent->GetOwner2());

	CmtSessionManager* pSessionManager = GetSessionManager();
	ASSERT (pSessionManager);
	if (!pSessionManager)
		return FALSE;
	//CString csTemp;
	//csTemp.LoadString(IDS_MSG_WAIT_SECURITY_ALARMS);
	//pRefreshInfo->ShowAnimateTextInfo(csTemp);//_T("List of Security Alarms...")

	if (pRefreshInfo->IsLoadSchema1())
		bOk = VSD_llQueryObject (&queryInfo1, pRefreshInfo->GetLoadedSchema1(), lNew);
	else
		bOk = INGRESII_llQueryObject (&queryInfo1, lNew, pSessionManager);

	if (pRefreshInfo->IsLoadSchema2())
		bOk2 = VSD_llQueryObject (&queryInfo2, pRefreshInfo->GetLoadedSchema2(), lNew2);
	else
		bOk2 = INGRESII_llQueryObject (&queryInfo2, lNew2, pSessionManager);

	if (!(bOk && bOk2))
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
		//
		// New object that is not in the old list, add it to the list:
		CaVsdAlarm* pNewObject = new CaVsdAlarm (pNew);
		pNewObject->SetBackParent (this);
		pNewObject->GetDifference() = _T('+'); // Mark as exist in Installation 1 ( temprary but not in Installation 2)
		m_listObject.AddTail (pNewObject);
		delete pNew;
	}

	//
	// Merging from the Schema2's objects:
	CtrfItemData* pExist = NULL;
	CaVsdFolderAlarm fdObject;
	CTypedPtrList< CObList, CtrfItemData* >& ls2 = fdObject.GetListObject();
	while (!lNew2.IsEmpty())
	{
		CaAlarm* pNew = (CaAlarm*)lNew2.RemoveHead();
		//
		// New object that is not in the old list, add it to the list:
		CaVsdAlarm* pNewObject = new CaVsdAlarm (pNew);
		pNewObject->SetBackParent (this);
		ls2.AddTail (pNewObject);
		delete pNew;
	}

	while (!ls2.IsEmpty())
	{
		BOOL b2Add = TRUE;
		CaVsdAlarm* pNew = (CaVsdAlarm*)ls2.RemoveHead();
		CaAlarm& obj = pNew->GetObject();
		CtrfItemData* pExist = SearchObject(&obj);
		if (pExist != NULL)
		{
			CaVsdAlarm* pCur = (CaVsdAlarm*)pExist;
			TCHAR& tchDiff = pCur->GetDifference();
			tchDiff = _T('#'); // Mark to compare the detail of grantee
			pCur->SetOwner2(obj.GetOwner());
			delete pNew;
			b2Add = FALSE;
		}

		if (b2Add)
		{
			//
			// New object from database 2:
			pNew->SetBackParent (this);
			pNew->GetDifference() = _T('-'); // Exist in Database2 but not in Database1
			m_listObject.AddTail (pNew);
		}
	}
	
	//
	// Refresh Sub-Branches
	pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
		if (pRefreshInfo->IsInterrupted())
			break;
		ptrfObj->RefreshData(pTree, hItem, pInfo);
	}

	return TRUE;
}

//
// Object: Grantee 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdGrantee, CtrfGrantee, 1)
BOOL CaVsdGrantee::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE, bOk2 = FALSE;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
/*UKS
	try
	{
*/
		CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
		ASSERT (pDisplayInfo);
		if (!pDisplayInfo)
			return FALSE;

		if (GetDifference() == _T('*') || GetDifference() == _T('#'))
		{
			//
			// Compare Detail info of the two objects:
			CaGrantee& obj = GetObject();
			CString strMsg;
			strMsg.Format (IDS_COMPARE_GRANTEE , (LPCTSTR)GetDisplayedItem(), (LPCTSTR)GetDisplayedItem());/*_T("compare grantee %s <--> %s \n")*/
			TRACE0 (strMsg);
			//pRefreshInfo->ShowAnimateTextInfo(strMsg);

			CaVsdFolderGrantee* pBackParent = (CaVsdFolderGrantee*)m_pBackParent;
			CaVsdItemData* pVsdParent = pBackParent->GetVsdParentItem();
			CmtSessionManager* pSessionManager = GetSessionManager();
			CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
			CaLLQueryInfo queryInfo2 (*pQueryInfo);
			queryInfo2.SetObjectType(OBT_GRANTEE);
			queryInfo2.SetNode(pRefreshInfo->GetNode2());
			queryInfo2.SetDatabase(pRefreshInfo->GetDatabase2());
			queryInfo2.SetItem2(pQueryInfo->GetItem2(), pVsdParent->GetOwner2());

			switch (pQueryInfo->GetSubObjectType())
			{
			case OBT_INSTALLATION:
				//strMsg.Format (_T("Grantee: %s"), (LPCTSTR)obj.GetName());
				//pRefreshInfo->ShowAnimateTextInfo(strMsg);
				break;
			case OBT_DATABASE:
				//strMsg.Format (_T("Grantee: %s / %s"), (LPCTSTR)pQueryInfo->GetDatabase(), (LPCTSTR)GetDisplayedItem());
				//pRefreshInfo->ShowAnimateTextInfo(strMsg);
				break;
			case OBT_TABLE:
			case OBT_VIEW:
			case OBT_PROCEDURE:
			case OBT_SEQUENCE:
			case OBT_DBEVENT:
				//strMsg.Format (
				//	_T("Grantee: %s / %s /%s"), 
				//	(LPCTSTR)pQueryInfo->GetDatabase(), 
				//	(LPCTSTR)pQueryInfo->GetItem2(), 
				//	(LPCTSTR)GetDisplayedItem());
				//pRefreshInfo->ShowAnimateTextInfo(strMsg);
				break;
			default:
				break;
			}

			CaGrantee to2;
			CaDBObject* pDt1 = &obj;
			CaDBObject* pDt2 = &to2;
			to2.SetItem(obj.GetName(), _T(""));
			to2.SetIdentity(obj.GetIdentity());

			if (pRefreshInfo->IsLoadSchema2())
				bOk2 = VSD_llQueryDetailObject(&queryInfo2, pRefreshInfo->GetLoadedSchema2(), pDt2);
			else
				bOk2  = INGRESII_llQueryDetailObject (&queryInfo2, &to2, pSessionManager);

			if (bOk2 && pDt1 && pDt2)
			{
				VSD_CompareGrantee (this, (CaGrantee*)pDt1, (CaGrantee*)pDt2);
			}
			if (pRefreshInfo->IsLoadSchema2() && pDt2)
				delete pDt2;
		}

		POSITION pos = m_listObject.GetHeadPosition();
		while (pos != NULL)
		{
			CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
			if (pRefreshInfo->IsInterrupted())
				break;
			ptrfObj->RefreshData(pTree, hItem, pInfo);
		}
/*UKS
	}
	catch (...)
	{

	}
*/
	return TRUE;
}

void CaVsdGrantee::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	int nImage = -1;
	CtrfGrantee::SetImageID(pDisplayInfo);
	CaGrantee& grantee = GetObject();
	int nID = grantee.GetIdentity();
	if (IsDiscarded())
	{
		GetTreeCtrlData().SetImage (IM_VSDTRFd, IM_VSDTRFd);
	}
	else
	{
		BOOL bHasDiscardInSubBranches = FALSE;
		TCHAR ch = HasDifferences ();
		CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
		int nSubObjectType = pQueryInfo->GetSubObjectType();

		VSD_HasDiscadItem(this, bHasDiscardInSubBranches, TRUE);
		if (bHasDiscardInSubBranches)
		{
			switch (ch)
			{
			case _T('+'):
				switch (nSubObjectType)
				{
				case OBT_INSTALLATION:
					nImage = IM_VSDTRFd_GxINSTALLATION_GR;
					break;
				case OBT_DATABASE:
					nImage = IM_VSDTRFd_GxDATABASE_GR;
					break;
				case OBT_TABLE:
					nImage = IM_VSDTRFd_GxTABLE_GR;
					break;
				case OBT_VIEW:
					nImage = IM_VSDTRFd_GxVIEW_GR;
					break;
				case OBT_PROCEDURE:
					nImage = IM_VSDTRFd_GxPROCEDURE_GR;
					break;
				case OBT_SEQUENCE:
					nImage = IM_VSDTRFd_GxSEQUENCE_GR;
					break;
				case OBT_DBEVENT:
					nImage = IM_VSDTRFd_GxDBEVENT_GR;
					break;
				default:
					break;
				}
				GetTreeCtrlData().SetImage (nImage, nImage);
				break;
			case _T('-'):
				switch (nSubObjectType)
				{
				case OBT_INSTALLATION:
					nImage = IM_VSDTRFd_GxINSTALLATION_RG;
					break;
				case OBT_DATABASE:
					nImage = IM_VSDTRFd_GxDATABASE_RG;
					break;
				case OBT_TABLE:
					nImage = IM_VSDTRFd_GxTABLE_RG;
					break;
				case OBT_VIEW:
					nImage = IM_VSDTRFd_GxVIEW_RG;
					break;
				case OBT_PROCEDURE:
					nImage = IM_VSDTRFd_GxPROCEDURE_RG;
					break;
				case OBT_SEQUENCE:
					nImage = IM_VSDTRFd_GxSEQUENCE_RG;
					break;
				case OBT_DBEVENT:
					nImage = IM_VSDTRFd_GxDBEVENT_RG;
					break;
				default:
					break;
				}
				GetTreeCtrlData().SetImage (nImage, nImage);
				break;
			case _T('!'):
				switch (nSubObjectType)
				{
				case OBT_INSTALLATION:
					nImage = IM_VSDTRFd_GxINSTALLATION_YY;
					break;
				case OBT_DATABASE:
					nImage = IM_VSDTRFd_GxDATABASE_YY;
					break;
				case OBT_TABLE:
					nImage = IM_VSDTRFd_GxTABLE_YY;
					break;
				case OBT_VIEW:
					nImage = IM_VSDTRFd_GxVIEW_YY;
					break;
				case OBT_PROCEDURE:
					nImage = IM_VSDTRFd_GxPROCEDURE_YY;
					break;
				case OBT_SEQUENCE:
					nImage = IM_VSDTRFd_GxSEQUENCE_YY;
					break;
				case OBT_DBEVENT:
					nImage = IM_VSDTRFd_GxDBEVENT_YY;
					break;
				default:
					break;
				}
				GetTreeCtrlData().SetImage (nImage, nImage);
				break;
			default:
				switch (nSubObjectType)
				{
				case OBT_INSTALLATION:
					nImage = IM_VSDTRFd_GxINSTALLATION_GG;
					break;
				case OBT_DATABASE:
					nImage = IM_VSDTRFd_GxDATABASE_GG;
					break;
				case OBT_TABLE:
					nImage = IM_VSDTRFd_GxTABLE_GG;
					break;
				case OBT_VIEW:
					nImage = IM_VSDTRFd_GxVIEW_GG;
					break;
				case OBT_PROCEDURE:
					nImage = IM_VSDTRFd_GxPROCEDURE_GG;
					break;
				case OBT_SEQUENCE:
					nImage = IM_VSDTRFd_GxSEQUENCE_GG;
					break;
				case OBT_DBEVENT:
					nImage = IM_VSDTRFd_GxDBEVENT_GG;
					break;
				default:
					break;
				}
				GetTreeCtrlData().SetImage (nImage, nImage);
				break;
			}
		}
		else
		{
			switch (ch)
			{
			case _T('+'):
				switch (nSubObjectType)
				{
				case OBT_INSTALLATION:
					nImage = IM_VSDTRF_GxINSTALLATION_GR;
					break;
				case OBT_DATABASE:
					nImage = IM_VSDTRF_GxDATABASE_GR;
					break;
				case OBT_TABLE:
					nImage = IM_VSDTRF_GxTABLE_GR;
					break;
				case OBT_VIEW:
					nImage = IM_VSDTRF_GxVIEW_GR;
					break;
				case OBT_PROCEDURE:
					nImage = IM_VSDTRF_GxPROCEDURE_GR;
					break;
				case OBT_SEQUENCE:
					nImage = IM_VSDTRF_GxSEQUENCE_GR;
					break;
				case OBT_DBEVENT:
					nImage = IM_VSDTRF_GxDBEVENT_GR;
					break;
				default:
					break;
				}
				GetTreeCtrlData().SetImage (nImage, nImage);
				break;
			case _T('-'):
				switch (nSubObjectType)
				{
				case OBT_INSTALLATION:
					nImage = IM_VSDTRF_GxINSTALLATION_RG;
					break;
				case OBT_DATABASE:
					nImage = IM_VSDTRF_GxDATABASE_RG;
					break;
				case OBT_TABLE:
					nImage = IM_VSDTRF_GxTABLE_RG;
					break;
				case OBT_VIEW:
					nImage = IM_VSDTRF_GxVIEW_RG;
					break;
				case OBT_PROCEDURE:
					nImage = IM_VSDTRF_GxPROCEDURE_RG;
					break;
				case OBT_SEQUENCE:
					nImage = IM_VSDTRF_GxSEQUENCE_RG;
					break;
				case OBT_DBEVENT:
					nImage = IM_VSDTRF_GxDBEVENT_RG;
					break;
				default:
					break;
				}
				GetTreeCtrlData().SetImage (nImage, nImage);
				break;
			case _T('!'):
				switch (nSubObjectType)
				{
				case OBT_INSTALLATION:
					nImage = IM_VSDTRF_GxINSTALLATION_YY;
					break;
				case OBT_DATABASE:
					nImage = IM_VSDTRF_GxDATABASE_YY;
					break;
				case OBT_TABLE:
					nImage = IM_VSDTRF_GxTABLE_YY;
					break;
				case OBT_VIEW:
					nImage = IM_VSDTRF_GxVIEW_YY;
					break;
				case OBT_PROCEDURE:
					nImage = IM_VSDTRF_GxPROCEDURE_YY;
					break;
				case OBT_SEQUENCE:
					nImage = IM_VSDTRF_GxSEQUENCE_YY;
					break;
				case OBT_DBEVENT:
					nImage = IM_VSDTRF_GxDBEVENT_YY;
					break;
				default:
					break;
				}
				GetTreeCtrlData().SetImage (nImage, nImage);
				break;
			default:
				switch (nSubObjectType)
				{
				case OBT_INSTALLATION:
					nImage = IM_VSDTRF_GxINSTALLATION_GG;
					break;
				case OBT_DATABASE:
					nImage = IM_VSDTRF_GxDATABASE_GG;
					break;
				case OBT_TABLE:
					nImage = IM_VSDTRF_GxTABLE_GG;
					break;
				case OBT_VIEW:
					nImage = IM_VSDTRF_GxVIEW_GG;
					break;
				case OBT_PROCEDURE:
					nImage = IM_VSDTRF_GxPROCEDURE_GG;
					break;
				case OBT_SEQUENCE:
					nImage = IM_VSDTRF_GxSEQUENCE_GG;
					break;
				case OBT_DBEVENT:
					nImage = IM_VSDTRF_GxDBEVENT_GG;
					break;
				default:
					break;
				}
				GetTreeCtrlData().SetImage (nImage, nImage);
				break;
			}
		}
	}
}

CString CaVsdGrantee::GetDisplayedItem(int nMode)
{
	CString strFmt;
	TCHAR c = _T('P');
	CaGrantee& obj = GetObject();

	switch (obj.GetIdentity())
	{
	case OBT_USER:
		c = _T('U');
		break;
	case OBT_GROUP:
		c = _T('G');
		break;
	case OBT_ROLE:
		c = _T('R');
		break;
	default:
		break;
	}
#if defined (_DEBUG) && defined(_SHOWDIFF_SYMBOL)
	strFmt.Format(_T("{[ID= %c] %c %d}   "), c, GetDifference(), IsMultiple());
	strFmt+= CtrfGrantee::GetDisplayedItem(nMode);
#else
	if (c == _T('P'))
		return _T("(public)");
	else
		return  CtrfGrantee::GetDisplayedItem(nMode);
#endif
	return strFmt;
}

//
// Object: Folder of Grantee 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdFolderGrantee, CtrfFolderGrantee, 1)
void CaVsdFolderGrantee::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfFolderGrantee::SetImageID(pDisplayInfo);
	FolderSetImageID(this);
}


BOOL CaVsdFolderGrantee::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE, bOk2;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
	CTypedPtrList< CObList, CaDBObject* > lNew, lNew2;
	CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
	ASSERT (pQueryInfo);
	if (!pQueryInfo)
		return FALSE;
	
	CaVsdItemData* pVsdParent = GetVsdParentItem();
	CaLLQueryInfo queryInfo1(*pQueryInfo);
	CaLLQueryInfo queryInfo2(*pQueryInfo);
	queryInfo1.SetNode(pRefreshInfo->GetNode1());
	queryInfo1.SetDatabase(pRefreshInfo->GetDatabase1());
	queryInfo2.SetNode(pRefreshInfo->GetNode2());
	queryInfo2.SetDatabase(pRefreshInfo->GetDatabase2());
	if (pVsdParent) // NULL: if (root is the parent)
		queryInfo2.SetItem2(queryInfo1.GetItem2(), pVsdParent->GetOwner2());

	CmtSessionManager* pSessionManager = GetSessionManager();
	ASSERT (pSessionManager);
	if (!pSessionManager)
		return FALSE;

	//CString csTemp;
	//csTemp.LoadString(IDS_MSG_WAIT_GRANTEES);
	//pRefreshInfo->ShowAnimateTextInfo(csTemp);//_T("List of Grantees...")

	if (pRefreshInfo->IsLoadSchema1())
		bOk = VSD_llQueryObject (&queryInfo1, pRefreshInfo->GetLoadedSchema1(), lNew);
	else
		bOk = INGRESII_llQueryObject (&queryInfo1, lNew, pSessionManager);

	if (pRefreshInfo->IsLoadSchema2())
		bOk2 = VSD_llQueryObject (&queryInfo2, pRefreshInfo->GetLoadedSchema2(), lNew2);
	else
		bOk2 = INGRESII_llQueryObject (&queryInfo2, lNew2, pSessionManager);

	if (!(bOk && bOk2))
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
		//
		// New object that is not in the old list, add it to the list:
		CaVsdGrantee* pNewObject = new CaVsdGrantee (pNew);
		pNewObject->SetBackParent (this);
		pNewObject->GetDifference() = _T('+'); // Mark as exist in Installation 1 ( temprary but not in Installation 2)
		m_listObject.AddTail (pNewObject);
		delete pNew;
	}

	//
	// Merging from the Schema2's objects:
	
	CtrfItemData* pExist = NULL;
	CaVsdFolderGrantee fdObject;
	CTypedPtrList< CObList, CtrfItemData* >& ls2 = fdObject.GetListObject();
	while (!lNew2.IsEmpty())
	{
		CaGrantee* pNew = (CaGrantee*)lNew2.RemoveHead();
		//
		// New object that is not in the old list, add it to the list:
		CaVsdGrantee* pNewObject = new CaVsdGrantee (pNew);
		pNewObject->SetBackParent (this);
		ls2.AddTail (pNewObject);
		delete pNew;
	}

	while (!ls2.IsEmpty())
	{
		BOOL b2Add = TRUE;
		CaVsdGrantee* pNew = (CaVsdGrantee*)ls2.RemoveHead();
		CaGrantee& obj = pNew->GetObject();
		CtrfItemData* pExist = SearchObject(&obj);
		if (pExist != NULL)
		{
			CaVsdGrantee* pCur = (CaVsdGrantee*)pExist;
			TCHAR& tchDiff = pCur->GetDifference();
			tchDiff = _T('#'); // Mark to compare the detail of grantee
			delete pNew;
			b2Add = FALSE;
		}

		if (b2Add)
		{
			//
			// New object from database 2:
			pNew->SetBackParent (this);
			pNew->GetDifference() = _T('-'); // Exist in Database2 but not in Database1
			m_listObject.AddTail (pNew);
		}
	}
	
	//
	// Refresh Sub-Branches
	pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
		if (pRefreshInfo->IsInterrupted())
			break;
		ptrfObj->RefreshData(pTree, hItem, pInfo);
	}

	return TRUE;
}


//
// Object: Table 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdTable, CtrfTable, 1)
void CaVsdTable::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfTable::SetImageID(pDisplayInfo);
	CaTable& table = GetObject();
	int nSubTableType = table.GetFlag();
	if (IsDiscarded())
	{
		GetTreeCtrlData().SetImage (IM_VSDTRFd, IM_VSDTRFd);
	}
	else
	{
		BOOL bHasDiscardInSubBranches = FALSE;
		TCHAR ch = HasDifferences ();
		VSD_HasDiscadItem(this, bHasDiscardInSubBranches, TRUE);
		if (bHasDiscardInSubBranches)
		{
			switch (ch)
			{
			case _T('+'):
				if (nSubTableType == OBJTYPE_STARNATIVE)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_TABLE_NATIVE_GR, IM_VSDTRFd_TABLE_NATIVE_GR);
				else if (nSubTableType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_TABLE_LINK_GR, IM_VSDTRFd_TABLE_LINK_GR);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRFd_TABLE_GR, IM_VSDTRFd_TABLE_GR);
				break;
			case _T('-'):
				if (nSubTableType == OBJTYPE_STARNATIVE)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_TABLE_NATIVE_RG, IM_VSDTRFd_TABLE_NATIVE_RG);
				else if (nSubTableType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_TABLE_LINK_RG, IM_VSDTRFd_TABLE_LINK_RG);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRFd_TABLE_RG, IM_VSDTRFd_TABLE_RG);
				break;
			case _T('!'):
				if (nSubTableType == OBJTYPE_STARNATIVE)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_TABLE_NATIVE_YY, IM_VSDTRFd_TABLE_NATIVE_YY);
				else if (nSubTableType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_TABLE_LINK_YY, IM_VSDTRFd_TABLE_LINK_YY);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRFd_TABLE_YY, IM_VSDTRFd_TABLE_YY);
				break;
			default:
				if (nSubTableType == OBJTYPE_STARNATIVE)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_TABLE_NATIVE_GG, IM_VSDTRFd_TABLE_NATIVE_GG);
				else if (nSubTableType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_TABLE_LINK_GG, IM_VSDTRFd_TABLE_LINK_GG);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRFd_TABLE_GG, IM_VSDTRFd_TABLE_GG);
				break;
			}
		}
		else
		{
			switch (ch)
			{
			case _T('+'):
				if (nSubTableType == OBJTYPE_STARNATIVE)
					GetTreeCtrlData().SetImage (IM_VSDTRF_TABLE_NATIVE_GR, IM_VSDTRF_TABLE_NATIVE_GR);
				else if (nSubTableType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRF_TABLE_LINK_GR, IM_VSDTRF_TABLE_LINK_GR);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRF_TABLE_GR, IM_VSDTRF_TABLE_GR);
				break;
			case _T('-'):
				if (nSubTableType == OBJTYPE_STARNATIVE)
					GetTreeCtrlData().SetImage (IM_VSDTRF_TABLE_NATIVE_RG, IM_VSDTRF_TABLE_NATIVE_RG);
				else if (nSubTableType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRF_TABLE_LINK_RG, IM_VSDTRF_TABLE_LINK_RG);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRF_TABLE_RG, IM_VSDTRF_TABLE_RG);
				break;
			case _T('!'):
				if (nSubTableType == OBJTYPE_STARNATIVE)
					GetTreeCtrlData().SetImage (IM_VSDTRF_TABLE_NATIVE_YY, IM_VSDTRF_TABLE_NATIVE_YY);
				else if (nSubTableType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRF_TABLE_LINK_YY, IM_VSDTRF_TABLE_LINK_YY);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRF_TABLE_YY, IM_VSDTRF_TABLE_YY);
				break;
			default:
				if (nSubTableType == OBJTYPE_STARNATIVE)
					GetTreeCtrlData().SetImage (IM_VSDTRF_TABLE_NATIVE_GG, IM_VSDTRF_TABLE_NATIVE_GG);
				else if (nSubTableType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRF_TABLE_LINK_GG, IM_VSDTRF_TABLE_LINK_GG);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRF_TABLE_GG, IM_VSDTRF_TABLE_GG);
				break;
			}
		}
	}
}


BOOL CaVsdTable::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE, bOk2 = FALSE;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;

	CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
	ASSERT (pDisplayInfo);
	if (!pDisplayInfo)
		return FALSE;

	if (GetDifference() == _T('*') || GetDifference() == _T('#'))
	{
		//
		// Compare Detail info of the two objects:
		CaTable& obj = GetObject();
		CString strMsg;
		strMsg.Format (IDS_COMPARE_TBL, obj.GetOwner(), obj.GetName(), GetOwner2(), obj.GetName()); /* _T("compare table [%s].%s <--> [%s].%s \n")*/
		TRACE0 (strMsg);

		CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
		CaLLQueryInfo queryInfo1 (*pQueryInfo);
		CaLLQueryInfo queryInfo2 (*pQueryInfo);
		queryInfo1.SetObjectType(OBT_TABLE);
		queryInfo2.SetObjectType(OBT_TABLE);
		queryInfo1.SetNode(pRefreshInfo->GetNode1());
		queryInfo1.SetDatabase(pRefreshInfo->GetDatabase1());
		queryInfo1.SetFlag(pRefreshInfo->GetFlag());
		queryInfo2.SetNode(pRefreshInfo->GetNode2());
		queryInfo2.SetDatabase(pRefreshInfo->GetDatabase2());
		queryInfo2.SetItem2(obj.GetName(), GetOwner2());
		queryInfo2.SetFlag(pRefreshInfo->GetFlag());

		strMsg.Format (
		    _T("Table: %s / %s <--> %s / %s"),
		    (LPCTSTR)queryInfo1.GetDatabase(), (LPCTSTR)obj.GetName(),
		    (LPCTSTR)queryInfo2.GetDatabase(), (LPCTSTR)obj.GetName());
		pRefreshInfo->ShowAnimateTextInfo(strMsg);

		CmtSessionManager* pSessionManager = GetSessionManager();
		CaTableDetail to1, to2;
		CaDBObject* pDt1 = &to1;
		CaDBObject* pDt2 = &to2;

		to1.SetQueryFlag(DTQUERY_ALL);
		to1.SetItem(obj.GetName(), obj.GetOwner());
		to2.SetQueryFlag(DTQUERY_ALL);
		to2.SetItem(obj.GetName(), GetOwner2());
		if (pRefreshInfo->IsLoadSchema1())
			bOk = VSD_llQueryDetailObject(&queryInfo1, pRefreshInfo->GetLoadedSchema1(), pDt1);
		else
			bOk  = INGRESII_llQueryDetailObject (&queryInfo1, &to1, pSessionManager);

		if (pRefreshInfo->IsLoadSchema2())
			bOk2 = VSD_llQueryDetailObject(&queryInfo2, pRefreshInfo->GetLoadedSchema2(), pDt2);
		else
			bOk2  = INGRESII_llQueryDetailObject (&queryInfo2, &to2, pSessionManager);

		if (bOk && bOk2 && pDt1 && pDt2)
		{
			VSD_CompareTable (this, (CaTableDetail*)pDt1, (CaTableDetail*)pDt2);
		}
		if (pRefreshInfo->IsLoadSchema1() && pDt1)
			delete pDt1;
		if (pRefreshInfo->IsLoadSchema2() && pDt2)
			delete pDt2;
	}

	POSITION pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
		if (pRefreshInfo->IsInterrupted())
			break;

		ptrfObj->RefreshData(pTree, hItem, pInfo);
	}

	return TRUE;
}

void CaVsdTable::Initialize()
{
	CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
	ASSERT (pDisplayInfo);
	if (!pDisplayInfo)
		return;
	UINT nNodeFlag = pDisplayInfo->TableChildrenFolderGetFlag();
	if (m_bInitialized)
		return;

	if ( GetFlag() & DBFLAG_STARNATIVE )
	{
		CaTable& table = GetObject();
		if (table.GetFlag() == OBJTYPE_STARNATIVE)
			return;
		//
		// Folder Index:
		CaVsdFolderIndex* pFolder1 = new CaVsdFolderIndex();
		pFolder1->SetBackParent (this);
		m_listObject.AddTail(pFolder1);
		m_pFolderIndex = pFolder1;
	}
	else
	{
		//
		// Folder Index:
		CaVsdFolderIndex* pFolder1 = new CaVsdFolderIndex();
		pFolder1->SetBackParent (this);
		m_listObject.AddTail(pFolder1);
		m_pFolderIndex = pFolder1;
		//
		// Folder Rule:
		CaVsdFolderRule* pFolder2 = new CaVsdFolderRule();
		pFolder2->SetBackParent (this);
		m_listObject.AddTail(pFolder2);
		m_pFolderRule = pFolder2;
		//
		// Folder Integrity:
		CaVsdFolderIntegrity* pFolder3 = new CaVsdFolderIntegrity();
		pFolder3->SetBackParent (this);
		m_listObject.AddTail(pFolder3);
		m_pFolderIntegrity = pFolder3;
		//
		// Folder Grantee:
		CaVsdFolderGrantee* pFolder4 = new CaVsdFolderGrantee(OBT_TABLE);
		pFolder4->SetBackParent (this);
		m_listObject.AddTail(pFolder4);
		//
		// Folder Security Alarm:
		CaVsdFolderAlarm* pFolder5 = new CaVsdFolderAlarm(OBT_TABLE);
		pFolder5->SetBackParent (this);
		m_listObject.AddTail(pFolder5);
		}
	m_bInitialized = TRUE;
}

CString CaVsdTable::GetDisplayedItem(int nMode)
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
		CString strOwner2 = GetOwner2();
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

		if (GetDifference() == _T('*') || GetDifference() == _T('#'))
		{
			if (!strOwner2.IsEmpty() && strOwner2.CompareNoCase(m_object.GetOwner()) != 0)
			{
				strDisplay = m_object.GetName();
#if defined (_MULTIPLE_OWNERS_PREFIXED)
				CString strOwners;
				strOwners.Format(_T("%s/%s"), (LPCTSTR)m_object.GetOwner(), (LPCTSTR)strOwner2);

				switch (nMode)
				{
				case 0
					strDisplay.Format (strOWNERxITEM, (LPCTSTR)strOwners, (LPCTSTR)m_object.GetName());
					break;
				case 1
					strDisplay.Format (strOWNERxITEM, (LPCTSTR)m_object.GetName(), (LPCTSTR)strOwners);
					break;
				default:
					ASSERT(FALSE);
					break;
				}
#endif
			}
		}
#if defined (_DEBUG) && defined(_SHOWDIFF_SYMBOL)
		CString strFmt;
		strFmt.Format(_T("{%c %d}   "), GetDifference(), IsMultiple());
		strFmt+= strDisplay;
		return strFmt;
#endif
		return strDisplay;
	}
	return CtrfTable::GetDisplayedItem(nMode);
}

//
// Object: Folder of Table 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdFolderTable, CtrfFolderTable, 1)
void CaVsdFolderTable::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfFolderTable::SetImageID(pDisplayInfo);
	FolderSetImageID(this);
}

BOOL CaVsdFolderTable::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE, bOk2;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
	CTypedPtrList< CObList, CaDBObject* > lNew, lNew2;
	CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
	ASSERT (pQueryInfo);
	if (!pQueryInfo)
		return FALSE;

	CaLLQueryInfo queryInfo1(*pQueryInfo);
	CaLLQueryInfo queryInfo2(*pQueryInfo);
	queryInfo1.SetNode(pRefreshInfo->GetNode1());
	queryInfo1.SetDatabase(pRefreshInfo->GetDatabase1());
	queryInfo1.SetFlag(pRefreshInfo->GetFlag());
	queryInfo2.SetNode(pRefreshInfo->GetNode2());
	queryInfo2.SetDatabase(pRefreshInfo->GetDatabase2());
	queryInfo2.SetFlag(pRefreshInfo->GetFlag());

	CmtSessionManager* pSessionManager = GetSessionManager();
	ASSERT (pSessionManager);
	if (!pSessionManager)
		return FALSE;
	CString csTemp;
	csTemp.LoadString(IDS_MSG_WAIT_TABLES);
	pRefreshInfo->ShowAnimateTextInfo(csTemp);//_T("List of Tables...")

	if (pRefreshInfo->IsLoadSchema1())
		bOk = VSD_llQueryObject (&queryInfo1, pRefreshInfo->GetLoadedSchema1(), lNew);
	else
		bOk = INGRESII_llQueryObject (&queryInfo1, lNew, pSessionManager);

	if (pRefreshInfo->IsLoadSchema2())
		bOk2 = VSD_llQueryObject (&queryInfo2, pRefreshInfo->GetLoadedSchema2(), lNew2);
	else
		bOk2 = INGRESII_llQueryObject (&queryInfo2, lNew2, pSessionManager);

	if (!(bOk && bOk2))
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
	// Filter objects:
	CString strSchema1 = pRefreshInfo->GetSchema1();
	if (!strSchema1.IsEmpty())
	{
		pos = lNew.GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaTable* pNew = (CaTable*)lNew.GetNext(pos);
			if (pNew->GetOwner().CompareNoCase(strSchema1) != 0)
			{
				lNew.RemoveAt(p);
				delete pNew;
			}
		}
	}
	CString strSchema2 = pRefreshInfo->GetSchema2();
	if (!strSchema2.IsEmpty())
	{
		pos = lNew2.GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaTable* pNew = (CaTable*)lNew2.GetNext(pos);
			if (pNew->GetOwner().CompareNoCase(strSchema2) != 0)
			{
				lNew2.RemoveAt(p);
				delete pNew;
			}
		}
	}

	//
	// Add new Objects:
	while (!lNew.IsEmpty())
	{
		CaTable* pNew = (CaTable*)lNew.RemoveHead();
		//
		// Match only object name and ignore the owner:
		CtrfItemData* pExist = SearchObject(pNew, FALSE);
		if (pExist != NULL)
		{
			//
			// There exists the same object with different owner (multiple object)
			pExist->GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_EXIST);
			((CaVsdTable*)pExist)->SetMultiple();
		}

		//
		// New object that is not in the old list, add it to the list:
		CaVsdTable* pNewObject = new CaVsdTable (pNew);
		pNewObject->SetBackParent (this);
		pNewObject->GetDifference() = _T('+'); // Mark as exist in Database1 ( temprary but not in Database2)
		if (pExist)
			pNewObject->SetMultiple();
		m_listObject.AddTail (pNewObject);

		delete pNew;
	}

	//
	// Merging from the Schema2's objects:
	if (!strSchema1.IsEmpty() && !strSchema2.IsEmpty())
	{
		while (!lNew2.IsEmpty())
		{
			CaTable* pNew = (CaTable*)lNew2.RemoveHead();
			CtrfItemData* pExist = SearchObject(pNew, FALSE);
			if (pExist != NULL)
			{
				CaVsdTable* pCur = (CaVsdTable*)pExist;
				TCHAR& tchDiff = pCur->GetDifference();
				tchDiff = _T('*'); // Mark to compare the detail of table
				pCur->SetFlag(pRefreshInfo->GetFlag());
				pCur->Initialize();
				pCur->SetOwner2(pNew->GetOwner());
				delete pNew;
			}
			else
			{
				//
				// New object from database 2:
				CaVsdTable* pNewObject = new CaVsdTable (pNew);
				pNewObject->SetBackParent (this);
				pNewObject->GetDifference() = _T('-'); // Exist in Database2 but not in Database1
				m_listObject.AddTail (pNewObject);
				delete pNew;
			}
		}
	}
	else
	{
		CtrfItemData* pExist = NULL;
		CaVsdFolderTable fdObject;
		CTypedPtrList< CObList, CtrfItemData* >& ls2 = fdObject.GetListObject();
		while (!lNew2.IsEmpty())
		{
			CaTable* pNew = (CaTable*)lNew2.RemoveHead();
			if (strSchema2.IsEmpty())
			{
				//
				// Match only object name and ignore the owner:
				pExist = fdObject.SearchObject(pNew, FALSE);
				if (pExist != NULL)
				{
					((CaVsdTable*)pExist)->SetMultiple();
				}
			}

			//
			// New object that is not in the old list, add it to the list:
			CaVsdTable* pNewObject = new CaVsdTable (pNew);
			pNewObject->SetBackParent (this);
			if (pExist)
				pNewObject->SetMultiple();
			ls2.AddTail (pNewObject);
			delete pNew;
		}

		while (!ls2.IsEmpty())
		{
			BOOL b2Add = TRUE;
			CaVsdTable* pNew = (CaVsdTable*)ls2.RemoveHead();
			CaTable& obj = pNew->GetObject();
			CtrfItemData* pExist = SearchObject(&obj);
			if (pExist != NULL)
			{
				CaVsdTable* pCur = (CaVsdTable*)pExist;
				TCHAR& tchDiff = pCur->GetDifference();
				tchDiff = _T('#'); // Mark to compare the detail of table
				pCur->SetOwner2(obj.GetOwner());
				pCur->SetFlag(pRefreshInfo->GetFlag());
				pCur->Initialize();
				delete pNew;
				b2Add = FALSE;
			}
			else
			{
				if (pRefreshInfo->IsIgnoreOwner() && !pNew->IsMultiple())
				{
					CtrfItemData* pExist = SearchObject(&obj, FALSE);
					if (pExist != NULL)
					{
						CaVsdTable* pCur = (CaVsdTable*)pExist;
						if (!pCur->IsMultiple())
						{
							TCHAR& tchDiff = pCur->GetDifference();
							tchDiff = _T('*'); // Mark to compare the detail of table
							pCur->SetOwner2(obj.GetOwner());
							pCur->SetFlag(pRefreshInfo->GetFlag());
							pCur->Initialize();
							delete pNew;
							b2Add = FALSE;
						}
					}
				}
			}

			if (b2Add)
			{
				//
				// New object from database 2:
				pNew->SetBackParent (this);
				pNew->GetDifference() = _T('-'); // Exist in Database2 but not in Database1
				m_listObject.AddTail (pNew);
			}
		}
	}
	//
	// Refresh Sub-Branches
	pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
		if (pRefreshInfo->IsInterrupted())
			break;
		ptrfObj->RefreshData(pTree, hItem, pInfo);
	}

	return TRUE;
}



// Object: View 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdView, CtrfView, 1)
void CaVsdView::Initialize()
{
	CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
	ASSERT (pDisplayInfo);
	if (!pDisplayInfo)
		return;
	if (m_bInitView)
		return;
	if ( GetFlag() & DBFLAG_STARNATIVE )
		return;

	//
	// Folder Grantee:
	CaVsdFolderGrantee* pFolder = new CaVsdFolderGrantee(OBT_VIEW);
	pFolder->SetBackParent (this);
	m_listObject.AddTail(pFolder);

	m_bInitView = TRUE;
}

void CaVsdView::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfView::SetImageID(pDisplayInfo);
	CaView& view = GetObject();
	int nSubTableType = view.GetFlag();

	if (IsDiscarded())
	{
		GetTreeCtrlData().SetImage (IM_VSDTRFd, IM_VSDTRFd);
	}
	else
	{
		BOOL bHasDiscardInSubBranches = FALSE;
		TCHAR ch = HasDifferences ();
		VSD_HasDiscadItem(this, bHasDiscardInSubBranches, TRUE);
		if (bHasDiscardInSubBranches)
		{
			switch (ch)
			{
			case _T('+'):
				if (nSubTableType == OBJTYPE_STARNATIVE)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_VIEW_NATIVE_GR, IM_VSDTRFd_VIEW_NATIVE_GR);
				else if (nSubTableType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_VIEW_LINK_GR, IM_VSDTRFd_VIEW_LINK_GR);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRFd_VIEW_GR, IM_VSDTRFd_VIEW_GR);
				break;
			case _T('-'):
				if (nSubTableType == OBJTYPE_STARNATIVE)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_VIEW_NATIVE_RG, IM_VSDTRFd_VIEW_NATIVE_RG );
				else if (nSubTableType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_VIEW_LINK_RG, IM_VSDTRFd_VIEW_LINK_RG );
				else
					GetTreeCtrlData().SetImage (IM_VSDTRFd_VIEW_RG, IM_VSDTRFd_VIEW_RG);
				break;
			case _T('!'):
				if (nSubTableType == OBJTYPE_STARNATIVE)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_VIEW_NATIVE_YY, IM_VSDTRFd_VIEW_NATIVE_YY);
				else if (nSubTableType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_VIEW_LINK_YY, IM_VSDTRFd_VIEW_LINK_YY);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRFd_VIEW_YY, IM_VSDTRFd_VIEW_YY);
				break;
			default:
				if (nSubTableType == OBJTYPE_STARNATIVE)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_VIEW_NATIVE_GG, IM_VSDTRFd_VIEW_NATIVE_GG);
				else if (nSubTableType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_VIEW_LINK_GG, IM_VSDTRFd_VIEW_LINK_GG);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRFd_VIEW_GG, IM_VSDTRFd_VIEW_GG);
				break;
			}
		}
		else
		{
			switch (ch)
			{
			case _T('+'):
				if (nSubTableType == OBJTYPE_STARNATIVE)
					GetTreeCtrlData().SetImage (IM_VSDTRF_VIEW_NATIVE_GR, IM_VSDTRF_VIEW_NATIVE_GR);
				else if (nSubTableType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRF_VIEW_LINK_GR, IM_VSDTRF_VIEW_LINK_GR);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRF_VIEW_GR, IM_VSDTRF_VIEW_GR);
				break;
			case _T('-'):
				if (nSubTableType == OBJTYPE_STARNATIVE)
					GetTreeCtrlData().SetImage (IM_VSDTRF_VIEW_NATIVE_RG, IM_VSDTRF_VIEW_NATIVE_RG);
				else if (nSubTableType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRF_VIEW_LINK_RG, IM_VSDTRF_VIEW_LINK_RG);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRF_VIEW_RG, IM_VSDTRF_VIEW_RG);
				break;
			case _T('!'):
				if (nSubTableType == OBJTYPE_STARNATIVE)
					GetTreeCtrlData().SetImage (IM_VSDTRF_VIEW_NATIVE_YY, IM_VSDTRF_VIEW_NATIVE_YY);
				else if (nSubTableType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_VIEW_LINK_YY, IM_VSDTRFd_VIEW_LINK_YY);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRF_VIEW_YY, IM_VSDTRF_VIEW_YY);
				break;
			default:
				if (nSubTableType == OBJTYPE_STARNATIVE)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_VIEW_NATIVE_GG, IM_VSDTRFd_VIEW_NATIVE_GG);
				else if (nSubTableType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRF_VIEW_LINK_GG, IM_VSDTRF_VIEW_LINK_GG);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRF_VIEW_GG, IM_VSDTRF_VIEW_GG);
				break;
			}
		}
	}
}

BOOL CaVsdView::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE;
	BOOL bOk2= FALSE;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
/*UKS
	try
	{
*/
		CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
		ASSERT (pDisplayInfo);
		if (!pDisplayInfo)
			return FALSE;

		if (GetDifference() == _T('*') || GetDifference() == _T('#'))
		{
			//
			// Compare Detail info of the two objects:
			CaView& obj = GetObject();
			CString strMsg;
			strMsg.Format (IDS_COMPARE_VIEW, obj.GetOwner(), obj.GetName(), GetOwner2(), obj.GetName());//_T("compare view [%s].%s <--> [%s].%s \n")
			TRACE0 (strMsg);

			CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
			CaLLQueryInfo qry1 (*pQueryInfo);
			CaLLQueryInfo qry2 (*pQueryInfo);
			qry1.SetObjectType(OBT_VIEW);
			qry2.SetObjectType(OBT_VIEW);
			qry1.SetNode(pRefreshInfo->GetNode1());
			qry1.SetDatabase(pRefreshInfo->GetDatabase1());
			qry1.SetFlag(pRefreshInfo->GetFlag());
			qry2.SetNode(pRefreshInfo->GetNode2());
			qry2.SetDatabase(pRefreshInfo->GetDatabase2());
			qry2.SetFlag(pRefreshInfo->GetFlag());

			strMsg.Format (
			    _T("View: %s / %s <--> %s / %s"),
			    (LPCTSTR)qry1.GetDatabase(), (LPCTSTR)obj.GetName(),
			    (LPCTSTR)qry2.GetDatabase(), (LPCTSTR)obj.GetName());
			pRefreshInfo->ShowAnimateTextInfo(strMsg);

			CmtSessionManager* pSessionManager = GetSessionManager();
			CaViewDetail to1, to2;
			CaDBObject* pDt1 = &to1;
			CaDBObject* pDt2 = &to2;

			to1.SetItem(obj.GetName(), obj.GetOwner());
			to2.SetItem(obj.GetName(), GetOwner2());
			if (pRefreshInfo->IsLoadSchema1())
				bOk = VSD_llQueryDetailObject(&qry1, pRefreshInfo->GetLoadedSchema1(), pDt1);
			else
				bOk  = INGRESII_llQueryDetailObject (&qry1, &to1, pSessionManager);

			if (pRefreshInfo->IsLoadSchema2())
				bOk2 = VSD_llQueryDetailObject(&qry2, pRefreshInfo->GetLoadedSchema2(), pDt2);
			else
				bOk2  = INGRESII_llQueryDetailObject (&qry2, &to2, pSessionManager);

			if (bOk && bOk2 && pDt1 && pDt2)
			{
				VSD_CompareView (this, (CaViewDetail*)pDt1, (CaViewDetail*)pDt2);
			}
			if (pRefreshInfo->IsLoadSchema1() && pDt1)
				delete pDt1;
			if (pRefreshInfo->IsLoadSchema2() && pDt2)
				delete pDt2;
		}

		POSITION pos = m_listObject.GetHeadPosition();
		while (pos != NULL)
		{
			CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
			if (pRefreshInfo->IsInterrupted())
				break;
			ptrfObj->RefreshData(pTree, hItem, pInfo);
		}

/*UKS
	}
	catch (...)
	{

	}
*/
	return TRUE;
}

CString CaVsdView::GetDisplayedItem(int nMode)
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
		CString strOwner2 = GetOwner2();
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

		if (GetDifference() == _T('*') || GetDifference() == _T('#'))
		{
			if (!strOwner2.IsEmpty() && strOwner2.CompareNoCase(m_object.GetOwner()) != 0)
			{
				strDisplay = m_object.GetName();
#if defined (_MULTIPLE_OWNERS_PREFIXED)
				CString strOwners;
				strOwners.Format(_T("%s/%s"), (LPCTSTR)m_object.GetOwner(), (LPCTSTR)strOwner2);

				switch (nMode)
				{
				case 0
					strDisplay.Format (strOWNERxITEM, (LPCTSTR)strOwners, (LPCTSTR)m_object.GetName());
					break;
				case 1
					strDisplay.Format (strOWNERxITEM, (LPCTSTR)m_object.GetName(), (LPCTSTR)strOwners);
					break;
				default:
					ASSERT(FALSE);
					break;
				}
#endif
			}
		}
#if defined (_DEBUG) && defined(_SHOWDIFF_SYMBOL)
		CString strFmt;
		strFmt.Format(_T("{%c %d}   "), GetDifference(), IsMultiple());
		strFmt+= strDisplay;
		return strFmt;
#endif
		return strDisplay;
	}
	return CtrfView::GetDisplayedItem(nMode);
}

//
// Object: Folder of View 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdFolderView, CtrfFolderView, 1)
void CaVsdFolderView::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfFolderView::SetImageID(pDisplayInfo);
	FolderSetImageID(this);
}

BOOL CaVsdFolderView::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE, bOk2;
	CTypedPtrList< CObList, CaDBObject* > lNew, lNew2;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
	CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
	ASSERT (pQueryInfo);
	if (!pQueryInfo)
		return FALSE;
	CaLLQueryInfo queryInfo1(*pQueryInfo);
	CaLLQueryInfo queryInfo2(*pQueryInfo);
	queryInfo1.SetNode(pRefreshInfo->GetNode1());
	queryInfo1.SetDatabase(pRefreshInfo->GetDatabase1());
	queryInfo1.SetFlag(pRefreshInfo->GetFlag());
	queryInfo2.SetNode(pRefreshInfo->GetNode2());
	queryInfo2.SetDatabase(pRefreshInfo->GetDatabase2());
	queryInfo2.SetFlag(pRefreshInfo->GetFlag());
	CmtSessionManager* pSessionManager = GetSessionManager();
	ASSERT (pSessionManager);
	if (!pSessionManager)
		return FALSE;

	CString csTemp;
	csTemp.LoadString(IDS_MSG_WAIT_VIEWS);
	pRefreshInfo->ShowAnimateTextInfo(csTemp);//_T("List of Views...")

	if (pRefreshInfo->IsLoadSchema1())
		bOk = VSD_llQueryObject (&queryInfo1, pRefreshInfo->GetLoadedSchema1(), lNew);
	else
		bOk = INGRESII_llQueryObject (&queryInfo1, lNew, pSessionManager);

	if (pRefreshInfo->IsLoadSchema2())
		bOk2 = VSD_llQueryObject (&queryInfo2, pRefreshInfo->GetLoadedSchema2(), lNew2);
	else
		bOk2 = INGRESII_llQueryObject (&queryInfo2, lNew2, pSessionManager);

	if (!(bOk && bOk2))
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
	// Filter objects:
	CString strSchema1 = pRefreshInfo->GetSchema1();
	if (!strSchema1.IsEmpty())
	{
		pos = lNew.GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaView* pNew = (CaView*)lNew.GetNext(pos);
			if (pNew->GetOwner().CompareNoCase(strSchema1) != 0)
			{
				lNew.RemoveAt(p);
				delete pNew;
			}
		}
	}
	CString strSchema2 = pRefreshInfo->GetSchema2();
	if (!strSchema2.IsEmpty())
	{
		pos = lNew2.GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaView* pNew = (CaView*)lNew2.GetNext(pos);
			if (pNew->GetOwner().CompareNoCase(strSchema2) != 0)
			{
				lNew2.RemoveAt(p);
				delete pNew;
			}
		}
	}
	
	//
	// Add new Objects:
	while (!lNew.IsEmpty())
	{
		CaView* pNew = (CaView*)lNew.RemoveHead();
		//
		// Match only object name and ignore the owner:
		CtrfItemData* pExist = SearchObject(pNew, FALSE);
		if (pExist != NULL)
		{
			pExist->GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_EXIST);
			((CaVsdView*)pExist)->SetMultiple();
		}

		//
		// New object that is not in the old list, add it to the list:
		CaVsdView* pNewObject = new CaVsdView (pNew);
		pNewObject->SetBackParent (this);
		pNewObject->GetDifference() = _T('+'); // Mark as exist in Database1 ( temprary but not in Database2)
		if (pExist)
			pNewObject->SetMultiple();
		m_listObject.AddTail (pNewObject);

		delete pNew;
	}

	//
	// Merging from the Schema2's objects:
	if (!strSchema1.IsEmpty() && !strSchema2.IsEmpty())
	{
		while (!lNew2.IsEmpty())
		{
			CaView* pNew = (CaView*)lNew2.RemoveHead();
			CtrfItemData* pExist = SearchObject(pNew, FALSE);
			if (pExist != NULL)
			{
				CaVsdView* pCur = (CaVsdView*)pExist;
				TCHAR& tchDiff = pCur->GetDifference();
				tchDiff = _T('*'); // Mark to compare the detail of object
				pCur->SetOwner2(pNew->GetOwner());
				pCur->SetFlag(pRefreshInfo->GetFlag());
				pCur->Initialize();
				delete pNew;
			}
			else
			{
				//
				// New object from database 2:
				CaVsdView* pNewObject = new CaVsdView (pNew);
				pNewObject->SetBackParent (this);
				pNewObject->GetDifference() = _T('-'); // Exist in Database2 but not in Database1
				m_listObject.AddTail (pNewObject);
				delete pNew;
			}
		}
	}
	else
	{
		CtrfItemData* pExist = NULL;
		CaVsdFolderView fdObject;
		CTypedPtrList< CObList, CtrfItemData* >& ls2 = fdObject.GetListObject();
		while (!lNew2.IsEmpty())
		{
			CaView* pNew = (CaView*)lNew2.RemoveHead();
			if (strSchema2.IsEmpty())
			{
				//
				// Match only object name and ignore the owner:
				pExist = fdObject.SearchObject(pNew, FALSE);
				if (pExist != NULL)
				{
					((CaVsdView*)pExist)->SetMultiple();
				}
			}

			//
			// New object that is not in the old list, add it to the list:
			CaVsdView* pNewObject = new CaVsdView (pNew);
			pNewObject->SetBackParent (this);
			if (pExist)
				pNewObject->SetMultiple();
			ls2.AddTail (pNewObject);
			delete pNew;
		}

		while (!ls2.IsEmpty())
		{
			BOOL b2Add = TRUE;
			CaVsdView* pNew = (CaVsdView*)ls2.RemoveHead();
			CaView& obj = pNew->GetObject();
			CtrfItemData* pExist = SearchObject(&obj);
			if (pExist != NULL)
			{
				CaVsdView* pCur = (CaVsdView*)pExist;
				TCHAR& tchDiff = pCur->GetDifference();
				tchDiff = _T('#'); // Mark to compare the detail of object
				pCur->SetOwner2(obj.GetOwner());
				pCur->SetFlag(pRefreshInfo->GetFlag());
				pCur->Initialize();
				delete pNew;
				b2Add = FALSE;
			}
			else
			{
				if (pRefreshInfo->IsIgnoreOwner() && !pNew->IsMultiple())
				{
					CtrfItemData* pExist = SearchObject(&obj, FALSE);
					if (pExist != NULL)
					{
						CaVsdView* pCur = (CaVsdView*)pExist;
						if (!pCur->IsMultiple())
						{
							TCHAR& tchDiff = pCur->GetDifference();
							tchDiff = _T('*'); // Mark to compare the detail of object
							pCur->SetOwner2(obj.GetOwner());
							pCur->SetFlag(pRefreshInfo->GetFlag());
							pCur->Initialize();
							delete pNew;
							b2Add = FALSE;
						}
					}
				}
			}

			if (b2Add)
			{
				//
				// New object from database 2:
				pNew->SetBackParent (this);
				pNew->GetDifference() = _T('-'); // Exist in Database2 but not in Database1
				m_listObject.AddTail (pNew);
			}
		}
	}
	//
	// Refresh Sub-Branches ?
	pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
		if (pRefreshInfo->IsInterrupted())
			break;
		ptrfObj->RefreshData(pTree, hItem, pInfo);
	}

	return TRUE;
}


//
// Object: Procedure 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdProcedure, CtrfProcedure, 1)
void CaVsdProcedure::Initialize()
{
	CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
	ASSERT (pDisplayInfo);
	if (!pDisplayInfo)
		return;
	if (m_bInitProcedure)
		return;
	if ( GetFlag() & DBFLAG_STARNATIVE )
		return;
	//
	// Folder Grantee:
	CaVsdFolderGrantee* pFolder = new CaVsdFolderGrantee(OBT_PROCEDURE);
	pFolder->SetBackParent (this);
	m_listObject.AddTail(pFolder);
	
	m_bInitProcedure = TRUE;
}

void CaVsdProcedure::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfProcedure::SetImageID(pDisplayInfo);
	CaProcedure& procedure = GetObject();
	int nProcType = procedure.GetFlag();

	if (IsDiscarded())
	{
		GetTreeCtrlData().SetImage (IM_VSDTRFd, IM_VSDTRFd);
	}
	else
	{
		BOOL bHasDiscardInSubBranches = FALSE;
		TCHAR ch = HasDifferences ();
		VSD_HasDiscadItem(this, bHasDiscardInSubBranches, TRUE);
		if (bHasDiscardInSubBranches)
		{
			switch (ch)
			{
			case _T('+'):
				if (nProcType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_PROCEDURE_LINK_GR, IM_VSDTRFd_PROCEDURE_LINK_GR);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRFd_PROCEDURE_GR, IM_VSDTRFd_PROCEDURE_GR);
				break;
			case _T('-'):
				if (nProcType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_PROCEDURE_LINK_RG, IM_VSDTRFd_PROCEDURE_LINK_RG);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRFd_PROCEDURE_RG, IM_VSDTRFd_PROCEDURE_RG);
				break;
			case _T('!'):
				if (nProcType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_PROCEDURE_LINK_YY, IM_VSDTRFd_PROCEDURE_LINK_YY);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRFd_PROCEDURE_YY, IM_VSDTRFd_PROCEDURE_YY);
				break;
			default:
				if (nProcType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_PROCEDURE_LINK_GG, IM_VSDTRFd_PROCEDURE_LINK_GG);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRFd_PROCEDURE_GG, IM_VSDTRFd_PROCEDURE_GG);
				break;
			}
		}
		else
		{
			switch (ch)
			{
			case _T('+'):
				if (nProcType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRF_PROCEDURE_LINK_GR, IM_VSDTRF_PROCEDURE_LINK_GR);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRF_PROCEDURE_GR, IM_VSDTRF_PROCEDURE_GR);
				break;
			case _T('-'):
				if (nProcType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRF_PROCEDURE_LINK_RG, IM_VSDTRF_PROCEDURE_LINK_RG);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRF_PROCEDURE_RG, IM_VSDTRF_PROCEDURE_RG);
				break;
			case _T('!'):
				if (nProcType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRF_PROCEDURE_LINK_YY, IM_VSDTRF_PROCEDURE_LINK_YY);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRF_PROCEDURE_YY, IM_VSDTRF_PROCEDURE_YY);
				break;
			default:
				if (nProcType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_PROCEDURE_LINK_GG, IM_VSDTRFd_PROCEDURE_LINK_GG);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRF_PROCEDURE_GG, IM_VSDTRF_PROCEDURE_GG);
				break;
			}
		}
	}
}

#if defined (_DEBUG) && defined(_SHOWDIFF_SYMBOL)
CString CaVsdProcedure::GetDisplayedItem(int nMode)
{
	CString strFmt;
	strFmt.Format(_T("{%c %d}   "), GetDifference(), IsMultiple());
	strFmt+= CtrfProcedure::GetDisplayedItem(nMode);
	return strFmt;
}
#endif

BOOL CaVsdProcedure::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE;
	BOOL bOk2= FALSE;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
/*UKS
	try
	{
*/
		CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
		ASSERT (pDisplayInfo);
		if (!pDisplayInfo)
			return FALSE;

		if (GetDifference() == _T('*') || GetDifference() == _T('#'))
		{
			//
			// Compare Detail info of the two objects:
			CaProcedure& obj = GetObject();
			CString strMsg;
			strMsg.Format (IDS_COMPARE_PROC, obj.GetOwner(), obj.GetName(), GetOwner2(), obj.GetName());/*_T("compare procedure [%s].%s <--> [%s].%s \n")*/
			TRACE0 (strMsg);

			CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
			CaLLQueryInfo qry1 (*pQueryInfo);
			CaLLQueryInfo qry2 (*pQueryInfo);
			qry1.SetObjectType(OBT_PROCEDURE);
			qry2.SetObjectType(OBT_PROCEDURE);
			qry1.SetNode(pRefreshInfo->GetNode1());
			qry1.SetDatabase(pRefreshInfo->GetDatabase1());
			qry1.SetFlag(pRefreshInfo->GetFlag());
			qry2.SetNode(pRefreshInfo->GetNode2());
			qry2.SetDatabase(pRefreshInfo->GetDatabase2());
			qry2.SetFlag(pRefreshInfo->GetFlag());

			 strMsg.Format (
			    _T("Procedure: %s / %s <--> %s / %s"),
			    (LPCTSTR)qry1.GetDatabase(), (LPCTSTR)obj.GetName(),
			    (LPCTSTR)qry2.GetDatabase(), (LPCTSTR)obj.GetName());
			pRefreshInfo->ShowAnimateTextInfo(strMsg);

			CmtSessionManager* pSessionManager = GetSessionManager();
			CaProcedureDetail to1, to2;
			CaDBObject* pDt1 = &to1;
			CaDBObject* pDt2 = &to2;

			to1.SetItem(obj.GetName(), obj.GetOwner());
			to2.SetItem(obj.GetName(), GetOwner2());
			if (pRefreshInfo->IsLoadSchema1())
				bOk = VSD_llQueryDetailObject(&qry1, pRefreshInfo->GetLoadedSchema1(), pDt1);
			else
				bOk  = INGRESII_llQueryDetailObject (&qry1, &to1, pSessionManager);

			if (pRefreshInfo->IsLoadSchema2())
				bOk2 = VSD_llQueryDetailObject(&qry2, pRefreshInfo->GetLoadedSchema2(), pDt2);
			else
				bOk2  = INGRESII_llQueryDetailObject (&qry2, &to2, pSessionManager);
			if (bOk && bOk2 && pDt1 && pDt2)
			{
				VSD_CompareProcedure (this, (CaProcedureDetail*)pDt1, (CaProcedureDetail*)pDt2);
			}
			if (pRefreshInfo->IsLoadSchema1() && pDt1)
				delete pDt1;
			if (pRefreshInfo->IsLoadSchema2() && pDt2)
				delete pDt2;
		}
	//
	// Refresh Sub-Branches
	POSITION pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
		if (pRefreshInfo->IsInterrupted())
			break;
		ptrfObj->RefreshData(pTree, hItem, pInfo);
	}

/*UKS
	}
	catch (...)
	{

	}
*/
	return TRUE;
}

//
// Object: Folder of Procedure 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdFolderProcedure, CtrfFolderProcedure, 1)
void CaVsdFolderProcedure::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfFolderProcedure::SetImageID(pDisplayInfo);
	FolderSetImageID(this);
}


BOOL CaVsdFolderProcedure::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE, bOk2;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
	CTypedPtrList< CObList, CaDBObject* > lNew, lNew2;
	CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
	ASSERT (pQueryInfo);
	if (!pQueryInfo)
		return FALSE;
	CaLLQueryInfo queryInfo1(*pQueryInfo);
	CaLLQueryInfo queryInfo2(*pQueryInfo);
	queryInfo1.SetNode(pRefreshInfo->GetNode1());
	queryInfo1.SetDatabase(pRefreshInfo->GetDatabase1());
	queryInfo1.SetFlag(pRefreshInfo->GetFlag());
	queryInfo2.SetNode(pRefreshInfo->GetNode2());
	queryInfo2.SetDatabase(pRefreshInfo->GetDatabase2());
	queryInfo2.SetFlag(pRefreshInfo->GetFlag());
	CmtSessionManager* pSessionManager = GetSessionManager();
	ASSERT (pSessionManager);
	if (!pSessionManager)
		return FALSE;

	CString csTemp;
	csTemp.LoadString(IDS_MSG_WAIT_PROCEDURES);
	pRefreshInfo->ShowAnimateTextInfo(csTemp);//_T("List of Procedures...")

	if (pRefreshInfo->IsLoadSchema1())
		bOk = VSD_llQueryObject (&queryInfo1, pRefreshInfo->GetLoadedSchema1(), lNew);
	else
		bOk = INGRESII_llQueryObject (&queryInfo1, lNew, pSessionManager);

	if (pRefreshInfo->IsLoadSchema2())
		bOk2 = VSD_llQueryObject (&queryInfo2, pRefreshInfo->GetLoadedSchema2(), lNew2);
	else
		bOk2 = INGRESII_llQueryObject (&queryInfo2, lNew2, pSessionManager);

	if (!(bOk && bOk2))
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
	// Filter objects:
	CString strSchema1 = pRefreshInfo->GetSchema1();
	if (!strSchema1.IsEmpty())
	{
		pos = lNew.GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaProcedure* pNew = (CaProcedure*)lNew.GetNext(pos);
			if (pNew->GetOwner().CompareNoCase(strSchema1) != 0)
			{
				lNew.RemoveAt(p);
				delete pNew;
			}
		}
	}
	CString strSchema2 = pRefreshInfo->GetSchema2();
	if (!strSchema2.IsEmpty())
	{
		pos = lNew2.GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaProcedure* pNew = (CaProcedure*)lNew2.GetNext(pos);
			if (pNew->GetOwner().CompareNoCase(strSchema2) != 0)
			{
				lNew2.RemoveAt(p);
				delete pNew;
			}
		}
	}

	//
	// Add new Objects:
	while (!lNew.IsEmpty())
	{
		CaProcedure* pNew = (CaProcedure*)lNew.RemoveHead();
		//
		// Match only object name and ignore the owner:
		CtrfItemData* pExist = SearchObject(pNew, FALSE);
		if (pExist != NULL)
		{
			//
			// There exists the same object with different owner (multiple object)
			pExist->GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_EXIST);
			((CaVsdProcedure*)pExist)->SetMultiple();
		}

		//
		// New object that is not in the old list, add it to the list:
		CaVsdProcedure* pNewObject = new CaVsdProcedure (pNew);
		pNewObject->SetBackParent (this);
		pNewObject->GetDifference() = _T('+'); // Mark as exist in Database1 ( temprary but not in Database2)
		if (pExist)
			pNewObject->SetMultiple();
		m_listObject.AddTail (pNewObject);

		delete pNew;
	}

	//
	// Merging from the Schema2's objects:
	if (!strSchema1.IsEmpty() && !strSchema2.IsEmpty())
	{
		while (!lNew2.IsEmpty())
		{
			CaProcedure* pNew = (CaProcedure*)lNew2.RemoveHead();
			CtrfItemData* pExist = SearchObject(pNew, FALSE);
			if (pExist != NULL)
			{
				CaVsdProcedure* pCur = (CaVsdProcedure*)pExist;
				//
				// Mark to compare the detail of object
				pCur->GetDifference() = _T('*');
				pCur->SetOwner2(pNew->GetOwner());
				pCur->SetFlag(pRefreshInfo->GetFlag());
				pCur->Initialize();
				delete pNew;
			}
			else
			{
				//
				// New object from database 2:
				CaVsdProcedure* pNewObject = new CaVsdProcedure (pNew);
				pNewObject->SetBackParent (this);
				pNewObject->GetDifference() = _T('-'); // Exist in Database2 but not in Database1
				m_listObject.AddTail (pNewObject);
				delete pNew;
			}
		}
	}
	else
	{
		CtrfItemData* pExist = NULL;
		CaVsdFolderProcedure fdObject;
		CTypedPtrList< CObList, CtrfItemData* >& ls2 = fdObject.GetListObject();
		while (!lNew2.IsEmpty())
		{
			CaProcedure* pNew = (CaProcedure*)lNew2.RemoveHead();
			if (strSchema2.IsEmpty())
			{
				//
				// Match only object name and ignore the owner:
				pExist = fdObject.SearchObject(pNew, FALSE);
				if (pExist != NULL)
				{
					((CaVsdProcedure*)pExist)->SetMultiple();
				}
			}

			//
			// New object that is not in the old list, add it to the list:
			CaVsdProcedure* pNewObject = new CaVsdProcedure (pNew);
			pNewObject->SetBackParent (this);
			if (pExist)
				pNewObject->SetMultiple();
			ls2.AddTail (pNewObject);
			delete pNew;
		}

		while (!ls2.IsEmpty())
		{
			BOOL b2Add = TRUE;
			CaVsdProcedure* pNew = (CaVsdProcedure*)ls2.RemoveHead();
			CaProcedure& obj = pNew->GetObject();
			CtrfItemData* pExist = SearchObject(&obj);
			if (pExist != NULL)
			{
				CaVsdProcedure* pCur = (CaVsdProcedure*)pExist;
				TCHAR& tchDiff = pCur->GetDifference();
				tchDiff = _T('#'); // Mark to compare the detail of object
				pCur->SetOwner2(obj.GetOwner());
				pCur->SetFlag(pRefreshInfo->GetFlag());
				pCur->Initialize();
				delete pNew;
				b2Add = FALSE;
			}
			else
			{
				if (pRefreshInfo->IsIgnoreOwner() && !pNew->IsMultiple())
				{
					CtrfItemData* pExist = SearchObject(&obj, FALSE);
					if (pExist != NULL)
					{
						CaVsdProcedure* pCur = (CaVsdProcedure*)pExist;
						if (!pCur->IsMultiple())
						{
							TCHAR& tchDiff = pCur->GetDifference();
							tchDiff = _T('*'); // Mark to compare the detail of object
							pCur->SetOwner2(obj.GetOwner());
							pCur->Initialize();
							delete pNew;
							b2Add = FALSE;
						}
					}
				}
			}

			if (b2Add)
			{
				//
				// New object from database 2:
				pNew->SetBackParent (this);
				pNew->GetDifference() = _T('-'); // Exist in Database2 but not in Database1
				m_listObject.AddTail (pNew);
			}
		}
	}
	//
	// Refresh Sub-Branches
	pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
		if (pRefreshInfo->IsInterrupted())
			break;
		ptrfObj->RefreshData(pTree, hItem, pInfo);
	}

	return TRUE;
}

//
// Object: Folder of Sequence 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdFolderSequence, CtrfFolderSequence, 1)
void CaVsdFolderSequence::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfFolderSequence::SetImageID(pDisplayInfo);
	FolderSetImageID(this);
}


BOOL CaVsdFolderSequence::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE, bOk2;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
	CTypedPtrList< CObList, CaDBObject* > lNew, lNew2;
	CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
	ASSERT (pQueryInfo);
	if (!pQueryInfo)
		return FALSE;
	CaLLQueryInfo queryInfo1(*pQueryInfo);
	CaLLQueryInfo queryInfo2(*pQueryInfo);
	queryInfo1.SetNode(pRefreshInfo->GetNode1());
	queryInfo1.SetDatabase(pRefreshInfo->GetDatabase1());
	queryInfo1.SetFlag(pRefreshInfo->GetFlag());
	queryInfo2.SetNode(pRefreshInfo->GetNode2());
	queryInfo2.SetDatabase(pRefreshInfo->GetDatabase2());
	queryInfo2.SetFlag(pRefreshInfo->GetFlag());
	CmtSessionManager* pSessionManager = GetSessionManager();
	ASSERT (pSessionManager);
	if (!pSessionManager)
		return FALSE;

	//CString csTemp;
	//csTemp.LoadString(IDS_MSG_WAIT_SEQUENCES);
	//pRefreshInfo->ShowAnimateTextInfo(csTemp);//_T("List of Sequences...")

	if (pRefreshInfo->IsLoadSchema1())
		bOk = VSD_llQueryObject (&queryInfo1, pRefreshInfo->GetLoadedSchema1(), lNew);
	else
		bOk = INGRESII_llQueryObject (&queryInfo1, lNew, pSessionManager);

	if (pRefreshInfo->IsLoadSchema2())
		bOk2 = VSD_llQueryObject (&queryInfo2, pRefreshInfo->GetLoadedSchema2(), lNew2);
	else
		bOk2 = INGRESII_llQueryObject (&queryInfo2, lNew2, pSessionManager);

	if (!(bOk && bOk2))
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
	// Filter objects:
	CString strSchema1 = pRefreshInfo->GetSchema1();
	if (!strSchema1.IsEmpty())
	{
		pos = lNew.GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaSequence* pNew = (CaSequence*)lNew.GetNext(pos);
			if (pNew->GetOwner().CompareNoCase(strSchema1) != 0)
			{
				lNew.RemoveAt(p);
				delete pNew;
			}
		}
	}
	CString strSchema2 = pRefreshInfo->GetSchema2();
	if (!strSchema2.IsEmpty())
	{
		pos = lNew2.GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaSequence* pNew = (CaSequence*)lNew2.GetNext(pos);
			if (pNew->GetOwner().CompareNoCase(strSchema2) != 0)
			{
				lNew2.RemoveAt(p);
				delete pNew;
			}
		}
	}

	//
	// Add new Objects:
	while (!lNew.IsEmpty())
	{
		CaSequence* pNew = (CaSequence*)lNew.RemoveHead();
		//
		// Match only object name and ignore the owner:
		CtrfItemData* pExist = SearchObject(pNew, FALSE);
		if (pExist != NULL)
		{
			//
			// There exists the same object with different owner (multiple object)
			pExist->GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_EXIST);
			((CaVsdSequence*)pExist)->SetMultiple();
		}

		//
		// New object that is not in the old list, add it to the list:
		CaVsdSequence* pNewObject = new CaVsdSequence (pNew);
		pNewObject->SetBackParent (this);
		pNewObject->GetDifference() = _T('+'); // Mark as exist in Database1 ( temprary but not in Database2)
		if (pExist)
			pNewObject->SetMultiple();
		m_listObject.AddTail (pNewObject);

		delete pNew;
	}

	//
	// Merging from the Schema2's objects:
	if (!strSchema1.IsEmpty() && !strSchema2.IsEmpty())
	{
		while (!lNew2.IsEmpty())
		{
			CaSequence* pNew = (CaSequence*)lNew2.RemoveHead();
			CtrfItemData* pExist = SearchObject(pNew, FALSE);
			if (pExist != NULL)
			{
				CaVsdSequence* pCur = (CaVsdSequence*)pExist;
				//
				// Mark to compare the detail of object
				pCur->GetDifference() = _T('*');
				pCur->SetOwner2(pNew->GetOwner());
				pCur->SetFlag(pRefreshInfo->GetFlag());
				pCur->Initialize();
				delete pNew;
			}
			else
			{
				//
				// New object from database 2:
				CaVsdSequence* pNewObject = new CaVsdSequence (pNew);
				pNewObject->SetBackParent (this);
				pNewObject->GetDifference() = _T('-'); // Exist in Database2 but not in Database1
				m_listObject.AddTail (pNewObject);
				delete pNew;
			}
		}
	}
	else
	{
		CtrfItemData* pExist = NULL;
		CaVsdFolderSequence fdObject;
		CTypedPtrList< CObList, CtrfItemData* >& ls2 = fdObject.GetListObject();
		while (!lNew2.IsEmpty())
		{
			CaSequence* pNew = (CaSequence*)lNew2.RemoveHead();
			if (strSchema2.IsEmpty())
			{
				//
				// Match only object name and ignore the owner:
				pExist = fdObject.SearchObject(pNew, FALSE);
				if (pExist != NULL)
				{
					((CaVsdSequence*)pExist)->SetMultiple();
				}
			}

			//
			// New object that is not in the old list, add it to the list:
			CaVsdSequence* pNewObject = new CaVsdSequence (pNew);
			pNewObject->SetBackParent (this);
			if (pExist)
				pNewObject->SetMultiple();
			ls2.AddTail (pNewObject);
			delete pNew;
		}

		while (!ls2.IsEmpty())
		{
			BOOL b2Add = TRUE;
			CaVsdSequence* pNew = (CaVsdSequence*)ls2.RemoveHead();
			CaSequence& obj = pNew->GetObject();
			CtrfItemData* pExist = SearchObject(&obj);
			if (pExist != NULL)
			{
				CaVsdSequence* pCur = (CaVsdSequence*)pExist;
				TCHAR& tchDiff = pCur->GetDifference();
				tchDiff = _T('#'); // Mark to compare the detail of object
				pCur->SetOwner2(obj.GetOwner());
				pCur->SetFlag(pRefreshInfo->GetFlag());
				pCur->Initialize();
				delete pNew;
				b2Add = FALSE;
			}
			else
			{
				if (pRefreshInfo->IsIgnoreOwner() && !pNew->IsMultiple())
				{
					CtrfItemData* pExist = SearchObject(&obj, FALSE);
					if (pExist != NULL)
					{
						CaVsdSequence* pCur = (CaVsdSequence*)pExist;
						if (!pCur->IsMultiple())
						{
							TCHAR& tchDiff = pCur->GetDifference();
							tchDiff = _T('*'); // Mark to compare the detail of object
							pCur->SetOwner2(obj.GetOwner());
							pCur->Initialize();
							delete pNew;
							b2Add = FALSE;
						}
					}
				}
			}

			if (b2Add)
			{
				//
				// New object from database 2:
				pNew->SetBackParent (this);
				pNew->GetDifference() = _T('-'); // Exist in Database2 but not in Database1
				m_listObject.AddTail (pNew);
			}
		}
	}
	//
	// Refresh Sub-Branches
	pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
		if (pRefreshInfo->IsInterrupted())
			break;
		ptrfObj->RefreshData(pTree, hItem, pInfo);
	}

	return TRUE;
}

//
// Object: Procedure 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdSequence, CtrfSequence, 1)
void CaVsdSequence::Initialize()
{
	CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
	ASSERT (pDisplayInfo);
	if (!pDisplayInfo)
		return;
	if (m_bInitSequence)
		return;
	if ( GetFlag() & DBFLAG_STARNATIVE )
		return;
	//
	// Folder Grantee:
	CaVsdFolderGrantee* pFolder = new CaVsdFolderGrantee(OBT_SEQUENCE);
	pFolder->SetBackParent (this);
	m_listObject.AddTail(pFolder);
	
	m_bInitSequence = TRUE;
}

void CaVsdSequence::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfSequence::SetImageID(pDisplayInfo);

	if (IsDiscarded())
	{
		GetTreeCtrlData().SetImage (IM_VSDTRFd, IM_VSDTRFd);
	}
	else
	{
		BOOL bHasDiscardInSubBranches = FALSE;
		TCHAR ch = HasDifferences ();
		VSD_HasDiscadItem(this, bHasDiscardInSubBranches, TRUE);
		if (bHasDiscardInSubBranches)
		{
			switch (ch)
			{
			case _T('+'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_SEQUENCE_GR, IM_VSDTRFd_SEQUENCE_GR);
				break;
			case _T('-'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_SEQUENCE_RG, IM_VSDTRFd_SEQUENCE_RG);
				break;
			case _T('!'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_SEQUENCE_YY, IM_VSDTRFd_SEQUENCE_YY);
				break;
			default:
				GetTreeCtrlData().SetImage (IM_VSDTRFd_SEQUENCE_GG, IM_VSDTRFd_SEQUENCE_GG);
				break;
			}
		}
		else
		{
			switch (ch)
			{
			case _T('+'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_SEQUENCE_GR, IM_VSDTRF_SEQUENCE_GR);
				break;
			case _T('-'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_SEQUENCE_RG, IM_VSDTRF_SEQUENCE_RG);
				break;
			case _T('!'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_SEQUENCE_YY, IM_VSDTRF_SEQUENCE_YY);
				break;
			default:
				GetTreeCtrlData().SetImage (IM_VSDTRF_SEQUENCE_GG, IM_VSDTRF_SEQUENCE_GG);
				break;
			}
		}
	}
}

#if defined (_DEBUG) && defined(_SHOWDIFF_SYMBOL)
CString CaVsdSequence::GetDisplayedItem(int nMode)
{
	CString strFmt;
	strFmt.Format(_T("{%c %d}   "), GetDifference(), IsMultiple());
	strFmt+= CtrfSequence::GetDisplayedItem(nMode);
	return strFmt;
}
#endif

BOOL CaVsdSequence::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE;
	BOOL bOk2= FALSE;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
/*UKS
	try
	{
*/
		CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
		ASSERT (pDisplayInfo);
		if (!pDisplayInfo)
			return FALSE;

		if (GetDifference() == _T('*') || GetDifference() == _T('#'))
		{
			//
			// Compare Detail info of the two objects:
			CaSequence& obj = GetObject();
			CString strMsg;
			strMsg.Format (IDS_COMPARE_SEQ, obj.GetOwner(), obj.GetName(), GetOwner2(), obj.GetName());/*_T("compare sequence [%s].%s <--> [%s].%s \n")*/
			TRACE0 (strMsg);

			CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
			CaLLQueryInfo qry1 (*pQueryInfo);
			CaLLQueryInfo qry2 (*pQueryInfo);
			qry1.SetObjectType(OBT_SEQUENCE);
			qry2.SetObjectType(OBT_SEQUENCE);
			qry1.SetNode(pRefreshInfo->GetNode1());
			qry1.SetDatabase(pRefreshInfo->GetDatabase1());
			qry1.SetFlag(pRefreshInfo->GetFlag());
			qry2.SetNode(pRefreshInfo->GetNode2());
			qry2.SetDatabase(pRefreshInfo->GetDatabase2());
			qry2.SetFlag(pRefreshInfo->GetFlag());

			//strMsg.Format (
			//    _T("Sequence: %s / %s <--> %s / %s"),
			//    (LPCTSTR)qry1.GetDatabase(), (LPCTSTR)obj.GetName(),
			//    (LPCTSTR)qry2.GetDatabase(), (LPCTSTR)obj.GetName());
			//pRefreshInfo->ShowAnimateTextInfo(strMsg);

			CmtSessionManager* pSessionManager = GetSessionManager();
			CaSequenceDetail to1, to2;
			CaDBObject* pDt1 = &to1;
			CaDBObject* pDt2 = &to2;

			to1.SetItem(obj.GetName(), obj.GetOwner());
			to2.SetItem(obj.GetName(), GetOwner2());
			if (pRefreshInfo->IsLoadSchema1())
				bOk = VSD_llQueryDetailObject(&qry1, pRefreshInfo->GetLoadedSchema1(), pDt1);
			else
				bOk  = INGRESII_llQueryDetailObject (&qry1, &to1, pSessionManager);

			if (pRefreshInfo->IsLoadSchema2())
				bOk2 = VSD_llQueryDetailObject(&qry2, pRefreshInfo->GetLoadedSchema2(), pDt2);
			else
				bOk2  = INGRESII_llQueryDetailObject (&qry2, &to2, pSessionManager);
			if (bOk && bOk2 && pDt1 && pDt2)
			{
				VSD_CompareSequence (this, (CaSequenceDetail*)pDt1, (CaSequenceDetail*)pDt2);
			}
			if (pRefreshInfo->IsLoadSchema1() && pDt1)
				delete pDt1;
			if (pRefreshInfo->IsLoadSchema2() && pDt2)
				delete pDt2;
		}
	//
	// Refresh Sub-Branches
	POSITION pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
		if (pRefreshInfo->IsInterrupted())
			break;
		ptrfObj->RefreshData(pTree, hItem, pInfo);
	}

/*UKS
	}
	catch (...)
	{

	}
*/
	return TRUE;
}

//
// Object: DBEvent:
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdDBEvent, CtrfDBEvent, 1)
void CaVsdDBEvent::Initialize()
{
	CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
	ASSERT (pDisplayInfo);
	if (!pDisplayInfo)
		return;
	if (m_bInitDBEvent)
		return;
	//
	// Folder Grantee:
	CaVsdFolderGrantee* pFolder = new CaVsdFolderGrantee(OBT_DBEVENT);
	pFolder->SetBackParent (this);
	m_listObject.AddTail(pFolder);

	m_bInitDBEvent = TRUE;
}

void CaVsdDBEvent::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfDBEvent::SetImageID(pDisplayInfo);
	if (IsDiscarded())
	{
		GetTreeCtrlData().SetImage (IM_VSDTRFd, IM_VSDTRFd);
	}
	else
	{
		BOOL bHasDiscardInSubBranches = FALSE;
		TCHAR ch = HasDifferences ();
		VSD_HasDiscadItem(this, bHasDiscardInSubBranches, TRUE);
		if (bHasDiscardInSubBranches)
		{
			switch (ch)
			{
			case _T('+'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_DBEVENT_GR, IM_VSDTRFd_DBEVENT_GR);
				break;
			case _T('-'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_DBEVENT_RG, IM_VSDTRFd_DBEVENT_RG);
				break;
			case _T('!'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_DBEVENT_YY, IM_VSDTRFd_DBEVENT_YY);
				break;
			default:
				GetTreeCtrlData().SetImage (IM_VSDTRFd_DBEVENT_GG, IM_VSDTRFd_DBEVENT_GG);
				break;
			}
		}
		else
		{
			switch (ch)
			{
			case _T('+'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_DBEVENT_GR, IM_VSDTRF_DBEVENT_GR);
				break;
			case _T('-'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_DBEVENT_RG, IM_VSDTRF_DBEVENT_RG);
				break;
			case _T('!'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_DBEVENT_YY, IM_VSDTRF_DBEVENT_YY);
				break;
			default:
				GetTreeCtrlData().SetImage (IM_VSDTRF_DBEVENT_GG, IM_VSDTRF_DBEVENT_GG);
				break;
			}
		}
	}
}


#if defined (_DEBUG) && defined(_SHOWDIFF_SYMBOL)
CString CaVsdDBEvent::GetDisplayedItem(int nMode)
{
	CString strFmt;
	strFmt.Format(_T("{%c %d}   "), GetDifference(), IsMultiple());
	strFmt+= CtrfDBEvent::GetDisplayedItem(nMode);
	return strFmt;
}
#endif

BOOL CaVsdDBEvent::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE;
	BOOL bOk2= FALSE;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
/*UKS
	try
	{
*/
		CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
		ASSERT (pDisplayInfo);
		if (!pDisplayInfo)
			return FALSE;

		if (GetDifference() == _T('*') || GetDifference() == _T('#'))
		{
			//
			// Compare Detail info of the two objects:
			CaDBEvent& obj = GetObject();
			CString strMsg;
			strMsg.Format (IDS_COMPARE_DBEVEVNT, obj.GetOwner(), obj.GetName(), GetOwner2(), obj.GetName());/*_T("compare dbevent [%s].%s <--> [%s].%s \n")*/
			TRACE0 (strMsg);

			CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
			CaLLQueryInfo qry1 (*pQueryInfo);
			CaLLQueryInfo qry2 (*pQueryInfo);
			qry1.SetObjectType(OBT_DBEVENT);
			qry2.SetObjectType(OBT_DBEVENT);
			qry1.SetNode(pRefreshInfo->GetNode1());
			qry1.SetDatabase(pRefreshInfo->GetDatabase1());
			qry2.SetNode(pRefreshInfo->GetNode2());
			qry2.SetDatabase(pRefreshInfo->GetDatabase2());

			strMsg.Format (
			    _T("Database Event: %s / %s <--> %s / %s"),
			    (LPCTSTR)qry1.GetDatabase(), (LPCTSTR)obj.GetName(),
			    (LPCTSTR)qry2.GetDatabase(), (LPCTSTR)obj.GetName());
			pRefreshInfo->ShowAnimateTextInfo(strMsg);

			CmtSessionManager* pSessionManager = GetSessionManager();
			CaDBEventDetail to1, to2;
			CaDBObject* pDt1 = &to1;
			CaDBObject* pDt2 = &to2;

			to1.SetItem(obj.GetName(), obj.GetOwner());
			to2.SetItem(obj.GetName(), GetOwner2());
			if (pRefreshInfo->IsLoadSchema1())
				bOk = VSD_llQueryDetailObject(&qry1, pRefreshInfo->GetLoadedSchema1(), pDt1);
			else
				bOk  = INGRESII_llQueryDetailObject (&qry1, &to1, pSessionManager);

			if (pRefreshInfo->IsLoadSchema2())
				bOk2 = VSD_llQueryDetailObject(&qry2, pRefreshInfo->GetLoadedSchema2(), pDt2);
			else
				bOk2  = INGRESII_llQueryDetailObject (&qry2, &to2, pSessionManager);
			if (bOk && bOk2 && pDt1 && pDt2)
			{
				VSD_CompareDBEvent (this, (CaDBEventDetail*)pDt1, (CaDBEventDetail*)pDt2);
			}
			if (pRefreshInfo->IsLoadSchema1() && pDt1)
				delete pDt1;
			if (pRefreshInfo->IsLoadSchema2() && pDt2)
				delete pDt2;
		}

		POSITION pos = m_listObject.GetHeadPosition();
		while (pos != NULL)
		{
			CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
			if (pRefreshInfo->IsInterrupted())
				break;
			ptrfObj->RefreshData(pTree, hItem, pInfo);
		}

/*UKS
	}
	catch (...)
	{

	}
*/
	return TRUE;
}


//
// Folder of DBEvent:
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdFolderDBEvent, CtrfFolderDBEvent, 1)
void CaVsdFolderDBEvent::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfFolderDBEvent::SetImageID(pDisplayInfo);
	FolderSetImageID(this);
}


BOOL CaVsdFolderDBEvent::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE, bOk2;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
	CTypedPtrList< CObList, CaDBObject* > lNew, lNew2;
	CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
	ASSERT (pQueryInfo);
	if (!pQueryInfo)
		return FALSE;
	CaLLQueryInfo queryInfo1(*pQueryInfo);
	CaLLQueryInfo queryInfo2(*pQueryInfo);
	queryInfo1.SetNode(pRefreshInfo->GetNode1());
	queryInfo1.SetDatabase(pRefreshInfo->GetDatabase1());
	queryInfo2.SetNode(pRefreshInfo->GetNode2());
	queryInfo2.SetDatabase(pRefreshInfo->GetDatabase2());
	CmtSessionManager* pSessionManager = GetSessionManager();
	ASSERT (pSessionManager);
	if (!pSessionManager)
		return FALSE;
	CString csTemp;
	csTemp.LoadString(IDS_MSG_WAIT_DB_EVENTS);
	pRefreshInfo->ShowAnimateTextInfo(csTemp);//_T("List of Darabase Events...")

	if (pRefreshInfo->IsLoadSchema1())
		bOk = VSD_llQueryObject (&queryInfo1, pRefreshInfo->GetLoadedSchema1(), lNew);
	else
		bOk = INGRESII_llQueryObject (&queryInfo1, lNew, pSessionManager);

	if (pRefreshInfo->IsLoadSchema2())
		bOk2 = VSD_llQueryObject (&queryInfo2, pRefreshInfo->GetLoadedSchema2(), lNew2);
	else
		bOk2 = INGRESII_llQueryObject (&queryInfo2, lNew2, pSessionManager);

	if (!(bOk && bOk2))
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
	// Filter objects:
	CString strSchema1 = pRefreshInfo->GetSchema1();
	if (!strSchema1.IsEmpty())
	{
		pos = lNew.GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaDBEvent* pNew = (CaDBEvent*)lNew.GetNext(pos);
			if (pNew->GetOwner().CompareNoCase(strSchema1) != 0)
			{
				lNew.RemoveAt(p);
				delete pNew;
			}
		}
	}
	CString strSchema2 = pRefreshInfo->GetSchema2();
	if (!strSchema2.IsEmpty())
	{
		pos = lNew2.GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaDBEvent* pNew = (CaDBEvent*)lNew2.GetNext(pos);
			if (pNew->GetOwner().CompareNoCase(strSchema2) != 0)
			{
				lNew2.RemoveAt(p);
				delete pNew;
			}
		}
	}

	//
	// Add new Objects:
	while (!lNew.IsEmpty())
	{
		CaDBEvent* pNew = (CaDBEvent*)lNew.RemoveHead();
		//
		// Match only object name and ignore the owner:
		CtrfItemData* pExist = SearchObject(pNew, FALSE);
		if (pExist != NULL)
		{
			//
			// There exists the same object with different owner (multiple object)
			pExist->GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_EXIST);
			((CaVsdDBEvent*)pExist)->SetMultiple();
		}

		//
		// New object that is not in the old list, add it to the list:
		CaVsdDBEvent* pNewObject = new CaVsdDBEvent (pNew);
		pNewObject->SetBackParent (this);
		pNewObject->GetDifference() = _T('+'); // Mark as exist in Database1 ( temprary but not in Database2)
		if (pExist)
			pNewObject->SetMultiple();
		m_listObject.AddTail (pNewObject);

		delete pNew;
	}

	//
	// Merging from the Schema2's objects:
	if (!strSchema1.IsEmpty() && !strSchema2.IsEmpty())
	{
		while (!lNew2.IsEmpty())
		{
			CaDBEvent* pNew = (CaDBEvent*)lNew2.RemoveHead();
			CtrfItemData* pExist = SearchObject(pNew, FALSE);
			if (pExist != NULL)
			{
				CaVsdDBEvent* pCur = (CaVsdDBEvent*)pExist;
				TCHAR& tchDiff = pCur->GetDifference();
				tchDiff = _T('*'); // Mark to compare the detail of object
				pCur->SetOwner2(pNew->GetOwner());
				pCur->Initialize();
				delete pNew;
			}
			else
			{
				//
				// New object from database 2:
				CaVsdDBEvent* pNewObject = new CaVsdDBEvent (pNew);
				pNewObject->SetBackParent (this);
				pNewObject->GetDifference() = _T('-'); // Exist in Database2 but not in Database1
				m_listObject.AddTail (pNewObject);
				delete pNew;
			}
		}
	}
	else
	{
		CtrfItemData* pExist = NULL;
		CaVsdFolderDBEvent fdObject;
		CTypedPtrList< CObList, CtrfItemData* >& ls2 = fdObject.GetListObject();
		while (!lNew2.IsEmpty())
		{
			CaDBEvent* pNew = (CaDBEvent*)lNew2.RemoveHead();
			if (strSchema2.IsEmpty())
			{
				//
				// Match only object name and ignore the owner:
				pExist = fdObject.SearchObject(pNew, FALSE);
				if (pExist != NULL)
				{
					((CaVsdDBEvent*)pExist)->SetMultiple();
				}
			}

			//
			// New object that is not in the old list, add it to the list:
			CaVsdDBEvent* pNewObject = new CaVsdDBEvent (pNew);
			pNewObject->SetBackParent (this);
			if (pExist)
				pNewObject->SetMultiple();
			ls2.AddTail (pNewObject);
			delete pNew;
		}

		while (!ls2.IsEmpty())
		{
			BOOL b2Add = TRUE;
			CaVsdDBEvent* pNew = (CaVsdDBEvent*)ls2.RemoveHead();
			CaDBEvent& obj = pNew->GetObject();
			CtrfItemData* pExist = SearchObject(&obj);
			if (pExist != NULL)
			{
				CaVsdDBEvent* pCur = (CaVsdDBEvent*)pExist;
				TCHAR& tchDiff = pCur->GetDifference();
				tchDiff = _T('#'); // Mark to compare the detail of object
				pCur->SetOwner2(obj.GetOwner());
				pCur->Initialize();
				delete pNew;
				b2Add = FALSE;
			}
			else
			{
				if (pRefreshInfo->IsIgnoreOwner() && !pNew->IsMultiple())
				{
					CtrfItemData* pExist = SearchObject(&obj, FALSE);
					if (pExist != NULL)
					{
						CaVsdDBEvent* pCur = (CaVsdDBEvent*)pExist;
						if (!pCur->IsMultiple())
						{
							TCHAR& tchDiff = pCur->GetDifference();
							tchDiff = _T('*'); // Mark to compare the detail of object
							pCur->SetOwner2(obj.GetOwner());
							pCur->Initialize();
							delete pNew;
							b2Add = FALSE;
						}
					}
				}
			}

			if (b2Add)
			{
				//
				// New object from database 2:
				pNew->SetBackParent (this);
				pNew->GetDifference() = _T('-'); // Exist in Database2 but not in Database1
				m_listObject.AddTail (pNew);
			}
		}
	}
	//
	// Refresh Sub-Branches
	pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
		if (pRefreshInfo->IsInterrupted())
			break;
		ptrfObj->RefreshData(pTree, hItem, pInfo);
	}

	return TRUE;
}


//
// Object: Synonym
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdSynonym, CtrfSynonym, 1)
void CaVsdSynonym::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfSynonym::SetImageID(pDisplayInfo);
	if (IsDiscarded())
	{
		GetTreeCtrlData().SetImage (IM_VSDTRFd, IM_VSDTRFd);
	}
	else
	{
		BOOL bHasDiscardInSubBranches = FALSE;
		TCHAR ch = HasDifferences ();
		VSD_HasDiscadItem(this, bHasDiscardInSubBranches, TRUE);
		if (bHasDiscardInSubBranches)
		{
			switch (ch)
			{
			case _T('+'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_SYNONYM_GR, IM_VSDTRFd_SYNONYM_GR);
				break;
			case _T('-'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_SYNONYM_RG, IM_VSDTRFd_SYNONYM_RG);
				break;
			case _T('!'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_SYNONYM_YY, IM_VSDTRFd_SYNONYM_YY);
				break;
			default:
				GetTreeCtrlData().SetImage (IM_VSDTRFd_SYNONYM_GG, IM_VSDTRFd_SYNONYM_GG);
				break;
			}
		}
		else
		{
			switch (ch)
			{
			case _T('+'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_SYNONYM_GR, IM_VSDTRF_SYNONYM_GR);
				break;
			case _T('-'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_SYNONYM_RG, IM_VSDTRF_SYNONYM_RG);
				break;
			case _T('!'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_SYNONYM_YY, IM_VSDTRF_SYNONYM_YY);
				break;
			default:
				GetTreeCtrlData().SetImage (IM_VSDTRF_SYNONYM_GG, IM_VSDTRF_SYNONYM_GG);
				break;
			}
		}
	}
}

#if defined (_DEBUG) && defined(_SHOWDIFF_SYMBOL)
CString CaVsdSynonym::GetDisplayedItem(int nMode)
{
	CString strFmt;
	strFmt.Format(_T("{%c %d}   "), GetDifference(), IsMultiple());
	strFmt+= CtrfSynonym::GetDisplayedItem(nMode);
	return strFmt;
}
#endif

BOOL CaVsdSynonym::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE;
	BOOL bOk2= FALSE;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
/*UKS
	try
	{
*/
		CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
		ASSERT (pDisplayInfo);
		if (!pDisplayInfo)
			return FALSE;

		if (GetDifference() == _T('*') || GetDifference() == _T('#'))
		{
			//
			// Compare Detail info of the two objects:
			CaSynonym& obj = GetObject();
			CString strMsg;
			strMsg.Format (IDS_COMPARE_SYNONYM, obj.GetOwner(), obj.GetName(), GetOwner2(), obj.GetName());/*_T("compare synonym [%s].%s <--> [%s].%s \n")*/
			TRACE0 (strMsg);

			CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
			CaLLQueryInfo qry1 (*pQueryInfo);
			CaLLQueryInfo qry2 (*pQueryInfo);
			qry1.SetObjectType(OBT_SYNONYM);
			qry2.SetObjectType(OBT_SYNONYM);
			qry1.SetNode(pRefreshInfo->GetNode1());
			qry1.SetDatabase(pRefreshInfo->GetDatabase1());
			qry2.SetNode(pRefreshInfo->GetNode2());
			qry2.SetDatabase(pRefreshInfo->GetDatabase2());

			//strMsg.Format (
			//    _T("Synonym: %s / %s <--> %s / %s"),
			//    (LPCTSTR)qry1.GetDatabase(), (LPCTSTR)obj.GetName(),
			//    (LPCTSTR)qry2.GetDatabase(), (LPCTSTR)obj.GetName());
			//pRefreshInfo->ShowAnimateTextInfo(strMsg);

			CmtSessionManager* pSessionManager = GetSessionManager();
			CaSynonymDetail to1, to2;
			CaDBObject* pDt1 = &to1;
			CaDBObject* pDt2 = &to2;

			to1.SetItem(obj.GetName(), obj.GetOwner());
			to2.SetItem(obj.GetName(), GetOwner2());
			if (pRefreshInfo->IsLoadSchema1())
				bOk = VSD_llQueryDetailObject(&qry1, pRefreshInfo->GetLoadedSchema1(), pDt1);
			else
				bOk  = INGRESII_llQueryDetailObject (&qry1, &to1, pSessionManager);

			if (pRefreshInfo->IsLoadSchema2())
				bOk2 = VSD_llQueryDetailObject(&qry2, pRefreshInfo->GetLoadedSchema2(), pDt2);
			else
				bOk2  = INGRESII_llQueryDetailObject (&qry2, &to2, pSessionManager);
			if (bOk && bOk2 && pDt1 && pDt2)
			{
				VSD_CompareSynonym (this, (CaSynonymDetail*)pDt1, (CaSynonymDetail*)pDt2);
			}
			if (pRefreshInfo->IsLoadSchema1() && pDt1)
				delete pDt1;
			if (pRefreshInfo->IsLoadSchema2() && pDt2)
				delete pDt2;
		}
/*UKS
	}
	catch (...)
	{

	}
*/
	return TRUE;
}

//
// Folder of Synonym
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdFolderSynonym, CtrfFolderSynonym, 1)
void CaVsdFolderSynonym::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfFolderSynonym::SetImageID(pDisplayInfo);
	FolderSetImageID(this);
}

BOOL CaVsdFolderSynonym::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE, bOk2;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
	CTypedPtrList< CObList, CaDBObject* > lNew, lNew2;
	CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
	ASSERT (pQueryInfo);
	if (!pQueryInfo)
		return FALSE;
	CaLLQueryInfo queryInfo1(*pQueryInfo);
	CaLLQueryInfo queryInfo2(*pQueryInfo);
	queryInfo1.SetNode(pRefreshInfo->GetNode1());
	queryInfo1.SetDatabase(pRefreshInfo->GetDatabase1());
	queryInfo2.SetNode(pRefreshInfo->GetNode2());
	queryInfo2.SetDatabase(pRefreshInfo->GetDatabase2());
	CmtSessionManager* pSessionManager = GetSessionManager();
	ASSERT (pSessionManager);
	if (!pSessionManager)
		return FALSE;
	//CString csTemp;
	//csTemp.LoadString(IDS_MSG_WAIT_SYNONYMS);
	//pRefreshInfo->ShowAnimateTextInfo(csTemp);//_T("List of Synonyms...")

	if (pRefreshInfo->IsLoadSchema1())
		bOk = VSD_llQueryObject (&queryInfo1, pRefreshInfo->GetLoadedSchema1(), lNew);
	else
		bOk = INGRESII_llQueryObject (&queryInfo1, lNew, pSessionManager);

	if (pRefreshInfo->IsLoadSchema2())
		bOk2 = VSD_llQueryObject (&queryInfo2, pRefreshInfo->GetLoadedSchema2(), lNew2);
	else
		bOk2 = INGRESII_llQueryObject (&queryInfo2, lNew2, pSessionManager);

	if (!(bOk && bOk2))
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
	// Filter objects:
	CString strSchema1 = pRefreshInfo->GetSchema1();
	if (!strSchema1.IsEmpty())
	{
		pos = lNew.GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaSynonym* pNew = (CaSynonym*)lNew.GetNext(pos);
			if (pNew->GetOwner().CompareNoCase(strSchema1) != 0)
			{
				lNew.RemoveAt(p);
				delete pNew;
			}
		}
	}
	CString strSchema2 = pRefreshInfo->GetSchema2();
	if (!strSchema2.IsEmpty())
	{
		pos = lNew2.GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaSynonym* pNew = (CaSynonym*)lNew2.GetNext(pos);
			if (pNew->GetOwner().CompareNoCase(strSchema2) != 0)
			{
				lNew2.RemoveAt(p);
				delete pNew;
			}
		}
	}

	//
	// Add new Objects:
	while (!lNew.IsEmpty())
	{
		CaSynonym* pNew = (CaSynonym*)lNew.RemoveHead();
		//
		// Match only object name and ignore the owner:
		CtrfItemData* pExist = SearchObject(pNew, FALSE);
		if (pExist != NULL)
		{
			//
			// There exists the same object with different owner (multiple object)
			pExist->GetTreeCtrlData().SetState (CaTreeCtrlData::ITEM_EXIST);
			((CaVsdSynonym*)pExist)->SetMultiple();
		}

		//
		// New object that is not in the old list, add it to the list:
		CaVsdSynonym* pNewObject = new CaVsdSynonym (pNew);
		pNewObject->SetBackParent (this);
		pNewObject->Initialize();
		pNewObject->GetDifference() = _T('+'); // Mark as exist in Database1 ( temprary but not in Database2)
		if (pExist)
			pNewObject->SetMultiple();
		m_listObject.AddTail (pNewObject);

		delete pNew;
	}

	//
	// Merging from the Schema2's objects:
	if (!strSchema1.IsEmpty() && !strSchema2.IsEmpty())
	{
		while (!lNew2.IsEmpty())
		{
			CaSynonym* pNew = (CaSynonym*)lNew2.RemoveHead();
			CtrfItemData* pExist = SearchObject(pNew, FALSE);
			if (pExist != NULL)
			{
				CaVsdSynonym* pCur = (CaVsdSynonym*)pExist;
				TCHAR& tchDiff = pCur->GetDifference();
				tchDiff = _T('*'); // Mark to compare the detail of table
				pCur->SetOwner2(pNew->GetOwner());
				delete pNew;
			}
			else
			{
				//
				// New object from database 2:
				CaVsdSynonym* pNewObject = new CaVsdSynonym (pNew);
				pNewObject->SetBackParent (this);
				pNewObject->Initialize();
				pNewObject->GetDifference() = _T('-'); // Exist in Database2 but not in Database1
				m_listObject.AddTail (pNewObject);
				delete pNew;
			}
		}
	}
	else
	{
		CtrfItemData* pExist = NULL;
		CaVsdFolderSynonym fdObject;
		CTypedPtrList< CObList, CtrfItemData* >& ls2 = fdObject.GetListObject();
		while (!lNew2.IsEmpty())
		{
			CaSynonym* pNew = (CaSynonym*)lNew2.RemoveHead();
			if (strSchema2.IsEmpty())
			{
				//
				// Match only object name and ignore the owner:
				pExist = fdObject.SearchObject(pNew, FALSE);
				if (pExist != NULL)
				{
					((CaVsdSynonym*)pExist)->SetMultiple();
				}
			}

			//
			// New object that is not in the old list, add it to the list:
			CaVsdSynonym* pNewObject = new CaVsdSynonym (pNew);
			pNewObject->SetBackParent (this);
			pNewObject->Initialize();
			if (pExist)
				pNewObject->SetMultiple();
			ls2.AddTail (pNewObject);
			delete pNew;
		}

		while (!ls2.IsEmpty())
		{
			BOOL b2Add = TRUE;
			CaVsdSynonym* pNew = (CaVsdSynonym*)ls2.RemoveHead();
			CaSynonym& obj = pNew->GetObject();
			CtrfItemData* pExist = SearchObject(&obj);
			if (pExist != NULL)
			{
				CaVsdSynonym* pCur = (CaVsdSynonym*)pExist;
				TCHAR& tchDiff = pCur->GetDifference();
				tchDiff = _T('#'); // Mark to compare the detail of table
				pCur->SetOwner2(obj.GetOwner());
				delete pNew;
				b2Add = FALSE;
			}
			else
			{
				if (pRefreshInfo->IsIgnoreOwner() && !pNew->IsMultiple())
				{
					CtrfItemData* pExist = SearchObject(&obj, FALSE);
					if (pExist != NULL)
					{
						CaVsdSynonym* pCur = (CaVsdSynonym*)pExist;
						if (!pCur->IsMultiple())
						{
							TCHAR& tchDiff = pCur->GetDifference();
							tchDiff = _T('*'); // Mark to compare the detail of table
							pCur->SetOwner2(obj.GetOwner());
							delete pNew;
							b2Add = FALSE;
						}
					}
				}
			}

			if (b2Add)
			{
				//
				// New object from database 2:
				pNew->SetBackParent (this);
				pNew->Initialize();
				pNew->GetDifference() = _T('-'); // Exist in Database2 but not in Database1
				m_listObject.AddTail (pNew);
			}
		}
	}
	//
	// Refresh Sub-Branches
	pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
		if (pRefreshInfo->IsInterrupted())
			break;
		ptrfObj->RefreshData(pTree, hItem, pInfo);
	}

	return TRUE;
}

//
// Object: Index
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdIndex, CtrfIndex, 1)
void CaVsdIndex::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfIndex::SetImageID(pDisplayInfo);
	CaIndex& Index = GetObject();
	int nSubTableType = Index.GetFlag();

	if (IsDiscarded())
	{
		GetTreeCtrlData().SetImage (IM_VSDTRFd, IM_VSDTRFd);
	}
	else
	{
		BOOL bHasDiscardInSubBranches = FALSE;
		TCHAR ch = HasDifferences ();
		VSD_HasDiscadItem(this, bHasDiscardInSubBranches, TRUE);
		if (bHasDiscardInSubBranches)
		{
			switch (ch)
			{
			case _T('+'):
				if (nSubTableType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_INDEX_LINK_GR, IM_VSDTRFd_INDEX_LINK_GR);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRFd_INDEX_GR, IM_VSDTRFd_INDEX_GR);
				break;
			case _T('-'):
				if (nSubTableType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_INDEX_LINK_RG, IM_VSDTRFd_INDEX_LINK_RG);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRFd_INDEX_RG, IM_VSDTRFd_INDEX_RG);
				break;
			case _T('!'):
				if (nSubTableType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_INDEX_LINK_YY, IM_VSDTRFd_INDEX_LINK_YY);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRFd_INDEX_YY, IM_VSDTRFd_INDEX_YY);
				break;
			default:
				if (nSubTableType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_INDEX_LINK_GG, IM_VSDTRFd_INDEX_LINK_GG);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRFd_INDEX_GG, IM_VSDTRFd_INDEX_GG);
				break;
			}
		}
		else
		{
			switch (ch)
			{
			case _T('+'):
				if (nSubTableType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRF_INDEX_LINK_GR, IM_VSDTRF_INDEX_LINK_GR);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRF_INDEX_GR, IM_VSDTRF_INDEX_GR);
				break;
			case _T('-'):
				if (nSubTableType == OBJTYPE_STARLINK)
				GetTreeCtrlData().SetImage (IM_VSDTRF_INDEX_LINK_RG, IM_VSDTRF_INDEX_LINK_RG);
				else
				GetTreeCtrlData().SetImage (IM_VSDTRF_INDEX_RG, IM_VSDTRF_INDEX_RG);
				break;
			case _T('!'):
				if (nSubTableType == OBJTYPE_STARLINK)
				GetTreeCtrlData().SetImage (IM_VSDTRF_INDEX_LINK_YY, IM_VSDTRF_INDEX_LINK_YY);
				else
				GetTreeCtrlData().SetImage (IM_VSDTRF_INDEX_YY, IM_VSDTRF_INDEX_YY);
				break;
			default:
				if (nSubTableType == OBJTYPE_STARLINK)
				GetTreeCtrlData().SetImage (IM_VSDTRFd_INDEX_LINK_GG, IM_VSDTRFd_INDEX_LINK_GG);
				else
				GetTreeCtrlData().SetImage (IM_VSDTRF_INDEX_GG, IM_VSDTRF_INDEX_GG);
				break;
			}
		}
	}
}

CString CaVsdIndex::GetDisplayedItem(int nMode)
{
	CString strFullName = _T("");
	switch (nMode)
	{
	case 0:
		return GetObject().GetName();
		break;
	case 1:
		if (m_pBackParent)
		{
			CaVsdTable* pTable = (CaVsdTable*)m_pBackParent->GetBackParent();
			if (pTable)
			{
				strFullName.Format(_T("%s / %s"), pTable->GetDisplayedItem(nMode), CtrfIndex::GetDisplayedItem());
			}
#if defined (_DEBUG) && defined(_SHOWDIFF_SYMBOL)
			CString strFmt;
			strFmt.Format(_T("%c  "), GetDifference());
			strFmt+= strFullName;;
			return strFmt;
#endif
		}
		return strFullName;
		break;
	default:
		ASSERT (FALSE);
		return _T("???");
	}
}

CString CaVsdIndex::GetName()
{
	if (m_pBackParent)
	{
		CaVsdTable* pTable = (CaVsdTable*)m_pBackParent->GetBackParent();
		if (pTable)
		{
			CString strFullName;
			strFullName.Format(_T("%s / %s"), (LPCTSTR)pTable->GetDisplayedItem(), (LPCTSTR)GetObject().GetName());
			return strFullName;
		}
	}

	return GetObject().GetName();
}

CString CaVsdIndex::GetOwner()
{
	return GetObject().GetOwner();
}

BOOL CaVsdIndex::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE, bOk2 = FALSE;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
/*UKS
	try
	{
*/
		CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
		ASSERT (pDisplayInfo);
		if (!pDisplayInfo)
			return FALSE;

		if (GetDifference() == _T('*') || GetDifference() == _T('#'))
		{
			//
			// Compare Detail info of the two objects:
			CaIndex& obj = GetObject();
			CString strMsg;
			strMsg.Format (IDS_COMPARE_INDEX, obj.GetOwner(), obj.GetName(), GetOwner2(), obj.GetName());/*_T("compare index [%s].%s <--> [%s].%s \n")*/
			TRACE0 (strMsg);

			CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
			CaLLQueryInfo qry1 (*pQueryInfo);
			CaLLQueryInfo qry2 (*pQueryInfo);
			qry1.SetObjectType(OBT_INDEX);
			qry2.SetObjectType(OBT_INDEX);
			qry1.SetNode(pRefreshInfo->GetNode1());
			qry1.SetDatabase(pRefreshInfo->GetDatabase1());
			qry1.SetFlag(pRefreshInfo->GetFlag());
			qry2.SetNode(pRefreshInfo->GetNode2());
			qry2.SetDatabase(pRefreshInfo->GetDatabase2());
			qry2.SetFlag(pRefreshInfo->GetFlag());

			if (m_pBackParent)
			{
				CaVsdTable* pTable = (CaVsdTable*)m_pBackParent->GetBackParent();
				if (pTable)
				{
					CaTable& tb = pTable->GetObject();
					qry2.SetItem2(tb.GetName(), pTable->GetOwner2());
				}
			}

			//strMsg.Format (_T("Index: %s / %s /%s <--> %s / %s /%s"),
			//    (LPCTSTR)qry1.GetDatabase(),
			//    (LPCTSTR)qry1.GetItem2(),
			//    (LPCTSTR)obj.GetName(),
			//    (LPCTSTR)qry2.GetDatabase(),
			//    (LPCTSTR)qry2.GetItem2(),
			//    (LPCTSTR)obj.GetName());
			//pRefreshInfo->ShowAnimateTextInfo(strMsg);

			CmtSessionManager* pSessionManager = GetSessionManager();
			CaIndexDetail to1, to2;
			CaDBObject* pDt1 = &to1;
			CaDBObject* pDt2 = &to2;

			to1.SetItem(obj.GetName(), obj.GetOwner());
			to2.SetItem(obj.GetName(), GetOwner2());
			if (pRefreshInfo->IsLoadSchema1())
				bOk = VSD_llQueryDetailObject(&qry1, pRefreshInfo->GetLoadedSchema1(), pDt1);
			else
				bOk  = INGRESII_llQueryDetailObject (&qry1, &to1, pSessionManager);

			if (pRefreshInfo->IsLoadSchema2())
				bOk2 = VSD_llQueryDetailObject(&qry2, pRefreshInfo->GetLoadedSchema2(), pDt2);
			else
				bOk2  = INGRESII_llQueryDetailObject (&qry2, &to2, pSessionManager);
			if (bOk && bOk2 && pDt1 && pDt2)
			{
				VSD_CompareIndex (this, (CaIndexDetail*)pDt1, (CaIndexDetail*)pDt2);
			}
			if (pRefreshInfo->IsLoadSchema1() && pDt1)
				delete pDt1;
			if (pRefreshInfo->IsLoadSchema2() && pDt2)
				delete pDt2;
		}
/*UKS
	}
	catch (...)
	{

	}
*/
	return TRUE;
}


//
// Object: Folder of Index 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdFolderIndex, CtrfFolderIndex, 1)
void CaVsdFolderIndex::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfFolderIndex::SetImageID(pDisplayInfo);
	FolderSetImageID(this);
}

BOOL CaVsdFolderIndex::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	CString csTemp;
	BOOL bOk = FALSE, bOk2 = FALSE;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
	CTypedPtrList< CObList, CaDBObject* > lNew, lNew2;
	CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
	ASSERT (pQueryInfo);
	if (!pQueryInfo)
		return FALSE;
	CaVsdTable* pTable = (CaVsdTable*)m_pBackParent;
	CaLLQueryInfo queryInfo1(*pQueryInfo);
	CaLLQueryInfo queryInfo2(*pQueryInfo);
	queryInfo1.SetNode(pRefreshInfo->GetNode1());
	queryInfo1.SetDatabase(pRefreshInfo->GetDatabase1());
	queryInfo1.SetItem2(pQueryInfo->GetItem2(), pTable->GetOwner());
	queryInfo1.SetFlag(pRefreshInfo->GetFlag());
	queryInfo2.SetNode(pRefreshInfo->GetNode2());
	queryInfo2.SetDatabase(pRefreshInfo->GetDatabase2());
	queryInfo2.SetItem2(pQueryInfo->GetItem2(), pTable->GetOwner2());
	queryInfo2.SetFlag(pRefreshInfo->GetFlag());
	TCHAR& tchDifference = pTable->GetDifference();
	if (tchDifference == _T('-') || tchDifference == _T('+')) 
	{
		csTemp.LoadString(IDS_NOT_FETCHED);
		m_EmptyNode.SetDisplayedItem (csTemp);//_T("<Not fetched for comparing>")
		return TRUE;
	}
	CmtSessionManager* pSessionManager = GetSessionManager();
	ASSERT (pSessionManager);
	if (!pSessionManager)
		return FALSE;

	//csTemp.LoadString(IDS_MSG_WAIT_INDICES);
	//pRefreshInfo->ShowAnimateTextInfo(csTemp);//_T("List of Indices...")

	if (pRefreshInfo->IsLoadSchema1())
		bOk = VSD_llQueryObject (&queryInfo1, pRefreshInfo->GetLoadedSchema1(), lNew);
	else
		bOk = INGRESII_llQueryObject (&queryInfo1, lNew, pSessionManager);

	if (pRefreshInfo->IsLoadSchema2())
		bOk2 = VSD_llQueryObject (&queryInfo2, pRefreshInfo->GetLoadedSchema2(), lNew2);
	else
		bOk2 = INGRESII_llQueryObject (&queryInfo2, lNew2, pSessionManager);

	if (!bOk || !bOk2)
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
	// Filter objects:
	CString strSchema1 = pRefreshInfo->GetSchema1();
	if (!strSchema1.IsEmpty())
	{
		pos = lNew.GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaIndex* pNew = (CaIndex*)lNew.GetNext(pos);
			if (pNew->GetOwner().CompareNoCase(strSchema1) != 0)
			{
				lNew.RemoveAt(p);
				delete pNew;
			}
		}
	}
	CString strSchema2 = pRefreshInfo->GetSchema2();
	if (!strSchema2.IsEmpty())
	{
		pos = lNew2.GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaIndex* pNew = (CaIndex*)lNew2.GetNext(pos);
			if (pNew->GetOwner().CompareNoCase(strSchema2) != 0)
			{
				lNew2.RemoveAt(p);
				delete pNew;
			}
		}
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
		CaVsdIndex* pNewObject = new CaVsdIndex (pNew);
		pNewObject->SetBackParent (this);
		pNewObject->Initialize();
		pNewObject->GetDifference() = _T('+'); // Mark as exist in Database1 (temprary but not in Database2)
		m_listObject.AddTail (pNewObject);

		delete pNew;
	}

	//
	// Merging from the Schema2's objects:
/*
	if (!strSchema1.IsEmpty() && !strSchema2.IsEmpty())
	{
*/
		while (!lNew2.IsEmpty())
		{
			CaIndex* pNew = (CaIndex*)lNew2.RemoveHead();
			CtrfItemData* pExist = SearchObject(pNew);
			if (pExist != NULL)
			{
				CaVsdIndex* pCur = (CaVsdIndex*)pExist;
				TCHAR& tchDiff = pCur->GetDifference();
				tchDiff = _T('*'); // Mark to compare the detail of table
				pCur->SetOwner2(pNew->GetOwner());
				delete pNew;
			}
			else
			{
				//
				// New object from database 2:
				CaVsdIndex* pNewObject = new CaVsdIndex (pNew);
				pNewObject->SetBackParent (this);
				pNewObject->Initialize();
				pNewObject->GetDifference() = _T('-'); // Exist in Database2 but not in Database1
				m_listObject.AddTail (pNewObject);
				delete pNew;
			}
		}
/*
	}
	else
	{
		while (!lNew2.IsEmpty())
		{
			CaIndex* pNew = (CaIndex*)lNew2.RemoveHead();
			CtrfItemData* pExist = SearchObject(pNew);
			if (pExist != NULL)
			{
				CaVsdIndex* pCur = (CaVsdIndex*)pExist;
				TCHAR& tchDiff = pCur->GetDifference();
				tchDiff = _T('#'); // Mark to compare the detail of table
				pCur->SetOwner2(pNew->GetOwner());
			}
			else
			{
				if (pRefreshInfo->IsIgnoreOwner())
				{

				}
				else
				{
					//
					// New object from database 2:
					CaVsdIndex* pNewObject = new CaVsdIndex (pNew);
					pNewObject->SetBackParent (this);
					pNewObject->Initialize();
					pNewObject->GetDifference() = _T('-'); // Exist in Database2 but not in Database1
					m_listObject.AddTail (pNewObject);
				}
			}

			delete pNew;
		}
	}
*/
	//
	// Refresh Sub-Branches ?
	pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
		if (pRefreshInfo->IsInterrupted())
			break;
		ptrfObj->RefreshData(pTree, hItem, pInfo);
	}

	return TRUE;
}


//
// Object: RULE
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdRule, CtrfRule, 1)
void CaVsdRule::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfRule::SetImageID(pDisplayInfo);
	if (IsDiscarded())
	{
		GetTreeCtrlData().SetImage (IM_VSDTRFd, IM_VSDTRFd);
	}
	else
	{
		BOOL bHasDiscardInSubBranches = FALSE;
		TCHAR ch = HasDifferences ();
		VSD_HasDiscadItem(this, bHasDiscardInSubBranches, TRUE);
		if (bHasDiscardInSubBranches)
		{
			switch (ch)
			{
			case _T('+'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_RULE_GR, IM_VSDTRFd_RULE_GR);
				break;
			case _T('-'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_RULE_RG, IM_VSDTRFd_RULE_RG);
				break;
			case _T('!'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_RULE_YY, IM_VSDTRFd_RULE_YY);
				break;
			default:
				GetTreeCtrlData().SetImage (IM_VSDTRFd_RULE_GG, IM_VSDTRFd_RULE_GG);
				break;
			}
		}
		else
		{
			switch (ch)
			{
			case _T('+'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_RULE_GR, IM_VSDTRF_RULE_GR);
				break;
			case _T('-'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_RULE_RG, IM_VSDTRF_RULE_RG);
				break;
			case _T('!'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_RULE_YY, IM_VSDTRF_RULE_YY);
				break;
			default:
				GetTreeCtrlData().SetImage (IM_VSDTRF_RULE_GG, IM_VSDTRF_RULE_GG);
				break;
			}
		}
	}
}

CString CaVsdRule::GetDisplayedItem(int nMode)
{
	CString strFullName = _T("");
	switch (nMode)
	{
	case 0:
		return GetObject().GetName();
		break;
	case 1:
		if (m_pBackParent)
		{
			CaVsdTable* pTable = (CaVsdTable*)m_pBackParent->GetBackParent();
			if (pTable)
			{
				strFullName.Format(_T("%s / %s"), pTable->GetDisplayedItem(nMode), CtrfRule::GetDisplayedItem());
			}
#if defined (_DEBUG) && defined(_SHOWDIFF_SYMBOL)
			CString strFmt;
			strFmt.Format(_T("%c  "), GetDifference());
			strFmt+= strFullName;;
			return strFmt;
#endif
		}
		return strFullName;
		break;
	default:
		ASSERT (FALSE);
		return _T("???");
	}
}

CString CaVsdRule::GetName()
{
	if (m_pBackParent)
	{
		CaVsdTable* pTable = (CaVsdTable*)m_pBackParent->GetBackParent();
		if (pTable)
		{
			CString strFullName;
			strFullName.Format(_T("%s / %s"), (LPCTSTR)pTable->GetDisplayedItem(), (LPCTSTR)GetObject().GetName());
			return strFullName;
		}
	}

	return GetObject().GetName();
}

CString CaVsdRule::GetOwner()
{
	return GetObject().GetOwner();
}

BOOL CaVsdRule::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE, bOk2 = FALSE;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
/*UKS
	try
	{
*/
		CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
		ASSERT (pDisplayInfo);
		if (!pDisplayInfo)
			return FALSE;

		if (GetDifference() == _T('*') || GetDifference() == _T('#'))
		{
			//
			// Compare Detail info of the two objects:
			CaRule& obj = GetObject();
			CString strMsg;
			strMsg.Format (IDS_COMPARE_RULE, obj.GetOwner(), obj.GetName(), GetOwner2(), obj.GetName());/*_T("compare rule [%s].%s <--> [%s].%s \n")*/
			TRACE0 (strMsg);

			CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
			CaLLQueryInfo qry1 (*pQueryInfo);
			CaLLQueryInfo qry2 (*pQueryInfo);
			qry1.SetObjectType(OBT_RULE);
			qry2.SetObjectType(OBT_RULE);
			qry1.SetNode(pRefreshInfo->GetNode1());
			qry1.SetDatabase(pRefreshInfo->GetDatabase1());
			qry2.SetNode(pRefreshInfo->GetNode2());
			qry2.SetDatabase(pRefreshInfo->GetDatabase2());
			
			if (m_pBackParent)
			{
				CaVsdTable* pTable = (CaVsdTable*)m_pBackParent->GetBackParent();
				if (pTable)
				{
					CaTable& tb = pTable->GetObject();
					qry2.SetItem2(tb.GetName(), pTable->GetOwner2());
				}
			}

			//strMsg.Format (
			//    _T("Rule: %s / %s /%s <--> %s / %s /%s"),
			//    (LPCTSTR)qry1.GetDatabase(),
			//    (LPCTSTR)qry1.GetItem2(),
			//    (LPCTSTR)obj.GetName(),
			//    (LPCTSTR)qry2.GetDatabase(),
			//    (LPCTSTR)qry2.GetItem2(),
			//    (LPCTSTR)obj.GetName());
			//pRefreshInfo->ShowAnimateTextInfo(strMsg);

			CmtSessionManager* pSessionManager = GetSessionManager();
			CaRuleDetail to1, to2;
			CaDBObject* pDt1 = &to1;
			CaDBObject* pDt2 = &to2;

			to1.SetItem(obj.GetName(), obj.GetOwner());
			to2.SetItem(obj.GetName(), GetOwner2());
			if (pRefreshInfo->IsLoadSchema1())
				bOk = VSD_llQueryDetailObject(&qry1, pRefreshInfo->GetLoadedSchema1(), pDt1);
			else
				bOk  = INGRESII_llQueryDetailObject (&qry1, &to1, pSessionManager);

			if (pRefreshInfo->IsLoadSchema2())
				bOk2 = VSD_llQueryDetailObject(&qry2, pRefreshInfo->GetLoadedSchema2(), pDt2);
			else
				bOk2  = INGRESII_llQueryDetailObject (&qry2, &to2, pSessionManager);
			if (bOk && bOk2 && pDt1 && pDt2)
			{
				VSD_CompareRule (this, (CaRuleDetail*)pDt1, (CaRuleDetail*)pDt2);
			}
			if (pRefreshInfo->IsLoadSchema1() && pDt1)
				delete pDt1;
			if (pRefreshInfo->IsLoadSchema2() && pDt2)
				delete pDt2;
		}
/*UKS
	}
	catch (...)
	{

	}
*/
	return TRUE;
}


//
// Folder of Rule:
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdFolderRule, CtrfFolderRule, 1)
void CaVsdFolderRule::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfFolderRule::SetImageID(pDisplayInfo);
	FolderSetImageID(this);
}

BOOL CaVsdFolderRule::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	CString csTemp;
	BOOL bOk = FALSE, bOk2 = FALSE;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
	CTypedPtrList< CObList, CaDBObject* > lNew, lNew2;
	CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
	ASSERT (pQueryInfo);
	if (!pQueryInfo)
		return FALSE;
	CaVsdTable* pTable = (CaVsdTable*)m_pBackParent;
	CaLLQueryInfo queryInfo1(*pQueryInfo);
	CaLLQueryInfo queryInfo2(*pQueryInfo);
	queryInfo1.SetNode(pRefreshInfo->GetNode1()); 
	queryInfo1.SetDatabase(pRefreshInfo->GetDatabase1()); 
	queryInfo1.SetItem2(pQueryInfo->GetItem2(), pTable->GetOwner2());
	queryInfo2.SetNode(pRefreshInfo->GetNode2()); 
	queryInfo2.SetDatabase(pRefreshInfo->GetDatabase2()); 
	queryInfo2.SetItem2(pQueryInfo->GetItem2(), pTable->GetOwner2());
	TCHAR& tchDifference = pTable->GetDifference();
	if (tchDifference == _T('-') || tchDifference == _T('+')) 
	{
		csTemp.LoadString(IDS_NOT_FETCHED);
		m_EmptyNode.SetDisplayedItem (csTemp);//_T("<Not fetched for comparing>")
		return TRUE;
	}
	CmtSessionManager* pSessionManager = GetSessionManager();
	ASSERT (pSessionManager);
	if (!pSessionManager)
		return FALSE;

	//csTemp.LoadString(IDS_MSG_WAIT_RULES);
	//pRefreshInfo->ShowAnimateTextInfo(csTemp);//_T("List of Rules...")

	if (pRefreshInfo->IsLoadSchema1())
		bOk = VSD_llQueryObject (&queryInfo1, pRefreshInfo->GetLoadedSchema1(), lNew);
	else
		bOk = INGRESII_llQueryObject (&queryInfo1, lNew, pSessionManager);

	if (pRefreshInfo->IsLoadSchema2())
		bOk2 = VSD_llQueryObject (&queryInfo2, pRefreshInfo->GetLoadedSchema2(), lNew2);
	else
		bOk2 = INGRESII_llQueryObject (&queryInfo2, lNew2, pSessionManager);

	if (!bOk || !bOk2)
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
	// Filter objects:
	CString strSchema1 = pRefreshInfo->GetSchema1();
	if (!strSchema1.IsEmpty())
	{
		pos = lNew.GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaRule* pNew = (CaRule*)lNew.GetNext(pos);
			if (pNew->GetOwner().CompareNoCase(strSchema1) != 0)
			{
				lNew.RemoveAt(p);
				delete pNew;
			}
		}
	}
	CString strSchema2 = pRefreshInfo->GetSchema2();
	if (!strSchema2.IsEmpty())
	{
		pos = lNew2.GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaRule* pNew = (CaRule*)lNew2.GetNext(pos);
			if (pNew->GetOwner().CompareNoCase(strSchema2) != 0)
			{
				lNew2.RemoveAt(p);
				delete pNew;
			}
		}
	}
	//
	// Add new Objects:
	while (!lNew.IsEmpty())
	{
		CaRule* pNew = (CaRule*)lNew.RemoveHead();
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
		CaVsdRule* pNewObject = new CaVsdRule (pNew);
		pNewObject->SetBackParent (this);
		pNewObject->Initialize();
		pNewObject->GetDifference() = _T('+'); // Mark as exist in Database1 ( temprary but not in Database2)
		m_listObject.AddTail (pNewObject);

		delete pNew;
	}

	//
	// Merging from the Schema2's objects:
	while (!lNew2.IsEmpty())
	{
		CaRule* pNew = (CaRule*)lNew2.RemoveHead();
		CtrfItemData* pExist = SearchObject(pNew);
		if (pExist != NULL)
		{
			CaVsdRule* pCur = (CaVsdRule*)pExist;
			TCHAR& tchDiff = pCur->GetDifference();
			tchDiff = _T('*'); // Mark to compare the detail of table
			pCur->SetOwner2(pNew->GetOwner());
			delete pNew;
		}
		else
		{
			//
			// New object from database 2:
			CaVsdRule* pNewObject = new CaVsdRule (pNew);
			pNewObject->SetBackParent (this);
			pNewObject->Initialize();
			pNewObject->GetDifference() = _T('-'); // Exist in Database2 but not in Database1
			m_listObject.AddTail (pNewObject);
			delete pNew;
		}
	}
	//
	// Refresh Sub-Branches ?
	pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
		if (pRefreshInfo->IsInterrupted())
			break;
		ptrfObj->RefreshData(pTree, hItem, pInfo);
	}

	return TRUE;
}

//
// Object: Integrity
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdIntegrity, CtrfIntegrity, 1)
void CaVsdIntegrity::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfIntegrity::SetImageID(pDisplayInfo);
	if (IsDiscarded())
	{
		GetTreeCtrlData().SetImage (IM_VSDTRFd, IM_VSDTRFd);
	}
	else
	{
		BOOL bHasDiscardInSubBranches = FALSE;
		TCHAR ch = HasDifferences ();
		VSD_HasDiscadItem(this, bHasDiscardInSubBranches, TRUE);
		if (bHasDiscardInSubBranches)
		{
			switch (ch)
			{
			case _T('+'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_INTEGRITY_GR, IM_VSDTRFd_INTEGRITY_GR);
				break;
			case _T('-'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_INTEGRITY_RG, IM_VSDTRFd_INTEGRITY_RG);
				break;
			case _T('!'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_INTEGRITY_YY, IM_VSDTRFd_INTEGRITY_YY);
				break;
			default:
				GetTreeCtrlData().SetImage (IM_VSDTRFd_INTEGRITY_GG, IM_VSDTRFd_INTEGRITY_GG);
				break;
			}
		}
		else
		{
			switch (ch)
			{
			case _T('+'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_INTEGRITY_GR, IM_VSDTRF_INTEGRITY_GR);
				break;
			case _T('-'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_INTEGRITY_RG, IM_VSDTRF_INTEGRITY_RG);
				break;
			case _T('!'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_INTEGRITY_YY, IM_VSDTRF_INTEGRITY_YY);
				break;
			default:
				GetTreeCtrlData().SetImage (IM_VSDTRF_INTEGRITY_GG, IM_VSDTRF_INTEGRITY_GG);
				break;
			}
		}
	}
}

CString CaVsdIntegrity::GetDisplayedItem(int nMode)
{
	CString strFullName = _T("");
	switch (nMode)
	{
	case 0:
		return CtrfIntegrity::GetDisplayedItem(nMode);
		break;
	case 1:
		if (m_pBackParent)
		{
			CaVsdTable* pTable = (CaVsdTable*)m_pBackParent->GetBackParent();
			if (pTable)
			{
				strFullName.Format(_T("%s / %s"), pTable->GetDisplayedItem(nMode), CtrfIntegrity::GetDisplayedItem());
			}
#if defined (_DEBUG) && defined(_SHOWDIFF_SYMBOL)
			CString strFmt;
			strFmt.Format(_T("%c  "), GetDifference());
			strFmt+= strFullName;;
			return strFmt;
#endif
		}
		return strFullName;
		break;
	default:
		ASSERT (FALSE);
		return _T("???");
	}
}

CString CaVsdIntegrity::GetName()
{
	if (m_pBackParent)
	{
		CaVsdTable* pTable = (CaVsdTable*)m_pBackParent->GetBackParent();
		if (pTable)
		{
			CString strFullName;
			strFullName.Format(_T("%s / %d"), (LPCTSTR)pTable->GetDisplayedItem(), GetObject().GetNumber());
			return strFullName;
		}
	}

	return GetObject().GetName();
}

CString CaVsdIntegrity::GetOwner()
{
	return GetObject().GetOwner();
}

BOOL CaVsdIntegrity::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE, bOk2 = FALSE;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
/*UKS
	try
	{
*/
		CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
		ASSERT (pDisplayInfo);
		if (!pDisplayInfo)
			return FALSE;

		if (GetDifference() == _T('*') || GetDifference() == _T('#'))
		{
			//
			// Compare Detail info of the two objects:
			CaIntegrity& obj = GetObject();
			CString strMsg;
			strMsg.Format (IDS_COMPARE_INTEGRITY, obj.GetOwner(), obj.GetName(), GetOwner2(), obj.GetName());/*_T("compare integrity [%s].%s <--> [%s].%s \n")*/
			TRACE0 (strMsg);

			CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
			CaLLQueryInfo qry1 (*pQueryInfo);
			CaLLQueryInfo qry2 (*pQueryInfo);
			qry1.SetObjectType(OBT_INTEGRITY);
			qry2.SetObjectType(OBT_INTEGRITY);
			qry1.SetNode(pRefreshInfo->GetNode1());
			qry1.SetDatabase(pRefreshInfo->GetDatabase1());
			qry2.SetNode(pRefreshInfo->GetNode2());
			qry2.SetDatabase(pRefreshInfo->GetDatabase2());
			
			if (m_pBackParent)
			{
				CaVsdTable* pTable = (CaVsdTable*)m_pBackParent->GetBackParent();
				if (pTable)
				{
					CaTable& tb = pTable->GetObject();
					qry2.SetItem2(tb.GetName(), pTable->GetOwner2());
				}
			}

			//strMsg.Format (
			//    _T("Integrity: %s / %s /%d <--> %s / %s /%d"),
			//    (LPCTSTR)qry1.GetDatabase(),
			//    (LPCTSTR)qry1.GetItem2(),
			//    obj.GetNumber(),
			//    (LPCTSTR)qry2.GetDatabase(),
			//    (LPCTSTR)qry2.GetItem2(),
			//    obj.GetNumber());
			//pRefreshInfo->ShowAnimateTextInfo(strMsg);

			CmtSessionManager* pSessionManager = GetSessionManager();
			CaIntegrityDetail to1, to2;
			CaDBObject* pDt1 = &to1;
			CaDBObject* pDt2 = &to2;

			to1.SetItem(obj.GetName(), obj.GetOwner());
			to1.SetNumber (obj.GetNumber());
			to2.SetItem(obj.GetName(), GetOwner2());
			to2.SetNumber (obj.GetNumber());
			if (pRefreshInfo->IsLoadSchema1())
				bOk = VSD_llQueryDetailObject(&qry1, pRefreshInfo->GetLoadedSchema1(), pDt1);
			else
				bOk  = INGRESII_llQueryDetailObject (&qry1, &to1, pSessionManager);

			if (pRefreshInfo->IsLoadSchema2())
				bOk2 = VSD_llQueryDetailObject(&qry2, pRefreshInfo->GetLoadedSchema2(), pDt2);
			else
				bOk2  = INGRESII_llQueryDetailObject (&qry2, &to2, pSessionManager);
			if (bOk && bOk2 && pDt1 && pDt2)
			{
				VSD_CompareIntegrity (this, (CaIntegrityDetail*)pDt1, (CaIntegrityDetail*)pDt2);
			}
			if (pRefreshInfo->IsLoadSchema1() && pDt1)
				delete pDt1;
			if (pRefreshInfo->IsLoadSchema2() && pDt2)
				delete pDt2;
		}
/*UKS
	}
	catch (...)
	{

	}
*/
	return TRUE;
}


//
// Folder of Integrity:
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdFolderIntegrity, CtrfFolderIntegrity, 1)
void CaVsdFolderIntegrity::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfFolderIntegrity::SetImageID(pDisplayInfo);
	FolderSetImageID(this);
}

BOOL CaVsdFolderIntegrity::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	CString csTemp;
	BOOL bOk = FALSE, bOk2 = FALSE;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
	CTypedPtrList< CObList, CaDBObject* > lNew, lNew2;
	CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
	ASSERT (pQueryInfo);
	if (!pQueryInfo)
		return FALSE;
	CaVsdTable* pTable = (CaVsdTable*)m_pBackParent;
	CaLLQueryInfo queryInfo1(*pQueryInfo);
	CaLLQueryInfo queryInfo2(*pQueryInfo);
	queryInfo1.SetNode(pRefreshInfo->GetNode1()); 
	queryInfo1.SetDatabase(pRefreshInfo->GetDatabase1()); 
	queryInfo1.SetItem2(pQueryInfo->GetItem2(), pTable->GetOwner2());
	queryInfo2.SetNode(pRefreshInfo->GetNode2()); 
	queryInfo2.SetDatabase(pRefreshInfo->GetDatabase2());
	queryInfo2.SetItem2(pQueryInfo->GetItem2(), pTable->GetOwner2());
	TCHAR& tchDifference = pTable->GetDifference();
	if (tchDifference == _T('-') || tchDifference == _T('+')) 
	{
		csTemp.LoadString(IDS_NOT_FETCHED);
		m_EmptyNode.SetDisplayedItem (csTemp);//_T("<Not fetched for comparing>")
		return TRUE;
	}
	CmtSessionManager* pSessionManager = GetSessionManager();
	ASSERT (pSessionManager);
	if (!pSessionManager)
		return FALSE;

	//csTemp.LoadString(IDS_MSG_WAIT_INTEGRITIES);
	//pRefreshInfo->ShowAnimateTextInfo(csTemp);//_T("List of Integrities...")

	if (pRefreshInfo->IsLoadSchema1())
		bOk = VSD_llQueryObject (&queryInfo1, pRefreshInfo->GetLoadedSchema1(), lNew);
	else
		bOk = INGRESII_llQueryObject (&queryInfo1, lNew, pSessionManager);

	if (pRefreshInfo->IsLoadSchema2())
		bOk2 = VSD_llQueryObject (&queryInfo2, pRefreshInfo->GetLoadedSchema2(), lNew2);
	else
		bOk2 = INGRESII_llQueryObject (&queryInfo2, lNew2, pSessionManager);

	if (!bOk || !bOk2)
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
/*
	//
	// Filter objects:
	CString strSchema1 = pRefreshInfo->GetSchema1();
	if (!strSchema1.IsEmpty())
	{
		pos = lNew.GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaIntegrity* pNew = (CaIntegrity*)lNew.GetNext(pos);
			if (pNew->GetOwner().CompareNoCase(strSchema1) != 0)
			{
				lNew.RemoveAt(p);
				delete pNew;
			}
		}
	}
	CString strSchema2 = pRefreshInfo->GetSchema2();
	if (!strSchema2.IsEmpty())
	{
		pos = lNew2.GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaIntegrity* pNew = (CaIntegrity*)lNew2.GetNext(pos);
			if (pNew->GetOwner().CompareNoCase(strSchema2) != 0)
			{
				lNew2.RemoveAt(p);
				delete pNew;
			}
		}
	}
*/
	//
	// Add new Objects:
	while (!lNew.IsEmpty())
	{
		CaIntegrity* pNew = (CaIntegrity*)lNew.RemoveHead();
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
		CaVsdIntegrity* pNewObject = new CaVsdIntegrity (pNew);
		pNewObject->SetBackParent (this);
		pNewObject->Initialize();
		pNewObject->GetDifference() = _T('+'); // Mark as exist in Database1 ( temprary but not in Database2)
		m_listObject.AddTail (pNewObject);

		delete pNew;
	}

	//
	// Merging from the Schema2's objects:
	while (!lNew2.IsEmpty())
	{
		CaIntegrity* pNew = (CaIntegrity*)lNew2.RemoveHead();
		CtrfItemData* pExist = SearchObject(pNew);
		if (pExist != NULL)
		{
			CaVsdIntegrity* pCur = (CaVsdIntegrity*)pExist;
			TCHAR& tchDiff = pCur->GetDifference();
			tchDiff = _T('*'); // Mark to compare the detail of table
			pCur->SetOwner2(pNew->GetOwner());
			delete pNew;
		}
		else
		{
			//
			// New object from database 2:
			CaVsdIntegrity* pNewObject = new CaVsdIntegrity (pNew);
			pNewObject->SetBackParent (this);
			pNewObject->Initialize();
			pNewObject->GetDifference() = _T('-'); // Exist in Database2 but not in Database1
			m_listObject.AddTail (pNewObject);
			delete pNew;
		}
	}
	//
	// Refresh Sub-Branches ?
	pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
		if (pRefreshInfo->IsInterrupted())
			break;
		ptrfObj->RefreshData(pTree, hItem, pInfo);
	}

	return TRUE;
}


//
// Object: Database 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdDatabase, CtrfDatabase, 1)
CaVsdDatabase::~CaVsdDatabase()
{
	Cleanup();
}

void CaVsdDatabase::Cleanup()
{
	while (!m_listObject.IsEmpty())
		delete m_listObject.RemoveHead();
	while (!m_listDifference.IsEmpty())
		delete m_listDifference.RemoveHead();
	GetTreeCtrlData().SetTreeItem(NULL);
}

CString CaVsdDatabase::GetDisplayedItem(int nMode)
{
	return m_bInitialized? m_strDisplayItem: GetObject().GetName();
}

void CaVsdDatabase::Initialize()
{
	if (m_bInitialized)
		return;
	CaDatabase& pDb = GetObject();
	int nDbFlag = pDb.GetStar();

	m_strDisplayItem = GetObject().GetName();
	//
	// Mark to compare database of schema1 with database of schema2
	GetDifference() = _T('*');

	if ( nDbFlag == OBJTYPE_STARNATIVE)
	{
		// Folder Table:
		CaVsdFolderTable* pFolder1 = new CaVsdFolderTable();
		pFolder1->SetBackParent(this);
		m_listObject.AddTail (pFolder1);
		//
		// Folder View:
		CaVsdFolderView* pFolder2 = new CaVsdFolderView();
		pFolder2->SetBackParent(this);
		m_listObject.AddTail (pFolder2);
		//
		// Folder Procedure:
		CaVsdFolderProcedure* pFolder3 = new CaVsdFolderProcedure();
		pFolder3->SetBackParent(this);
		m_listObject.AddTail (pFolder3);
	}
	else
	{
		//
		// Folder Table:
		CaVsdFolderTable* pFolder1 = new CaVsdFolderTable();
		pFolder1->SetBackParent(this);
		m_listObject.AddTail (pFolder1);
		//
		// Folder View:
		CaVsdFolderView* pFolder2 = new CaVsdFolderView();
		pFolder2->SetBackParent(this);
		m_listObject.AddTail (pFolder2);
		//
		// Folder Procedure:
		CaVsdFolderProcedure* pFolder3 = new CaVsdFolderProcedure();
		pFolder3->SetBackParent(this);
		m_listObject.AddTail (pFolder3);
		//
		// Folder DBEvent:
		CaVsdFolderDBEvent* pFolder4 = new CaVsdFolderDBEvent();
		pFolder4->SetBackParent(this);
		m_listObject.AddTail (pFolder4);
		//
		// Folder Synonym:
		CaVsdFolderSynonym* pFolder5 = new CaVsdFolderSynonym();
		pFolder5->SetBackParent(this);
		m_listObject.AddTail (pFolder5);
		//
		// Folder Grantee:
		CaVsdFolderGrantee* pFolder6 = new CaVsdFolderGrantee(OBT_DATABASE);
		pFolder6->SetBackParent (this);
		m_listObject.AddTail(pFolder6);
		//
		// Folder Security Alarm:
		CaVsdFolderAlarm* pFolder7 = new CaVsdFolderAlarm(OBT_DATABASE);
		pFolder7->SetBackParent (this);
		m_listObject.AddTail(pFolder7);
		//
		// Folder Sequence:
		CaVsdFolderSequence* pFolder8 = new CaVsdFolderSequence();
		pFolder8->SetBackParent(this);
		m_listObject.AddTail (pFolder8);
	}
	m_bInitialized = TRUE;
}

BOOL CaVsdDatabase::Populate (CaVsdRefreshTreeInfo* pPopulateData)
{
	BOOL bInterrupted = FALSE;

	CString s1 = pPopulateData->GetSchema1();
	CString s2 = pPopulateData->GetSchema2();
	if (!s1.IsEmpty() && !s2.IsEmpty())
	{
		m_strDisplayItem.Format (_T("Database Schema: [%s / %s]"), (LPCTSTR)s1, (LPCTSTR)s2);
	}
	else
	{
		m_strDisplayItem = _T("Database");
	}

	RefreshData(NULL, NULL, pPopulateData);
	bInterrupted = pPopulateData->IsInterrupted();

	pPopulateData->Close();
	return bInterrupted? FALSE: TRUE;
}


void CaVsdDatabase::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CaDatabase& db = GetObject();
	int nDBType = db.GetStar();

	CtrfDatabase::SetImageID(pDisplayInfo);
	if (IsDiscarded())
	{
		GetTreeCtrlData().SetImage (IM_VSDTRFd, IM_VSDTRFd);
	}
	else
	{
		BOOL bHasDiscardInSubBranches = FALSE;
		TCHAR ch = HasDifferences ();
		VSD_HasDiscadItem(this, bHasDiscardInSubBranches, TRUE);
		if (bHasDiscardInSubBranches)
		{
			switch (ch)
			{
			case _T('+'):
				if (nDBType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_DATABASE_DISTRIBUTED_GR, IM_VSDTRFd_DATABASE_DISTRIBUTED_GR);
				else if (nDBType == OBJTYPE_STARNATIVE)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_DATABASE_COORDINATOR_GR, IM_VSDTRFd_DATABASE_COORDINATOR_GR);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRFd_DATABASE_GR, IM_VSDTRFd_DATABASE_GR);
				break;
			case _T('-'):
				if (nDBType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_DATABASE_DISTRIBUTED_RG, IM_VSDTRFd_DATABASE_DISTRIBUTED_RG);
				else if (nDBType == OBJTYPE_STARNATIVE)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_DATABASE_COORDINATOR_RG, IM_VSDTRFd_DATABASE_COORDINATOR_RG);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRFd_DATABASE_RG, IM_VSDTRFd_DATABASE_RG);
				break;
			case _T('!'):
				if (nDBType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_DATABASE_DISTRIBUTED_YY, IM_VSDTRFd_DATABASE_DISTRIBUTED_YY);
				else if (nDBType == OBJTYPE_STARNATIVE)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_DATABASE_COORDINATOR_YY, IM_VSDTRFd_DATABASE_COORDINATOR_YY);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRFd_DATABASE_YY, IM_VSDTRFd_DATABASE_YY);
				break;
			default:
				if (nDBType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_DATABASE_DISTRIBUTED_GG, IM_VSDTRFd_DATABASE_DISTRIBUTED_GG);
				else if (nDBType == OBJTYPE_STARNATIVE)
					GetTreeCtrlData().SetImage (IM_VSDTRFd_DATABASE_COORDINATOR_GG, IM_VSDTRFd_DATABASE_COORDINATOR_GG);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRFd_DATABASE_GG, IM_VSDTRFd_DATABASE_GG);
				break;
			}
		}
		else
		{
			switch (ch)
			{
			case _T('+'):
				if (nDBType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRF_DATABASE_DISTRIBUTED_GR, IM_VSDTRF_DATABASE_DISTRIBUTED_GR);
				else if (nDBType == OBJTYPE_STARNATIVE)
					GetTreeCtrlData().SetImage (IM_VSDTRF_DATABASE_COORDINATOR_GR, IM_VSDTRF_DATABASE_COORDINATOR_GR);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRF_DATABASE_GR, IM_VSDTRF_DATABASE_GR);
				break;
			case _T('-'):
				if (nDBType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRF_DATABASE_DISTRIBUTED_RG, IM_VSDTRF_DATABASE_DISTRIBUTED_RG);
				else if (nDBType == OBJTYPE_STARNATIVE)
					GetTreeCtrlData().SetImage (IM_VSDTRF_DATABASE_COORDINATOR_RG, IM_VSDTRF_DATABASE_COORDINATOR_RG);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRF_DATABASE_RG, IM_VSDTRF_DATABASE_RG);
				break;
			case _T('!'):
				if (nDBType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRF_DATABASE_DISTRIBUTED_YY, IM_VSDTRF_DATABASE_DISTRIBUTED_YY);
				else if (nDBType == OBJTYPE_STARNATIVE)
					GetTreeCtrlData().SetImage (IM_VSDTRF_DATABASE_COORDINATOR_YY, IM_VSDTRF_DATABASE_COORDINATOR_YY);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRF_DATABASE_YY, IM_VSDTRF_DATABASE_YY);
				break;
			default:
				if (nDBType == OBJTYPE_STARLINK)
					GetTreeCtrlData().SetImage (IM_VSDTRF_DATABASE_DISTRIBUTED_GG, IM_VSDTRF_DATABASE_DISTRIBUTED_GG);
				else if (nDBType == OBJTYPE_STARNATIVE)
					GetTreeCtrlData().SetImage (IM_VSDTRF_DATABASE_COORDINATOR_GG, IM_VSDTRF_DATABASE_COORDINATOR_GG);
				else
					GetTreeCtrlData().SetImage (IM_VSDTRF_DATABASE_GG, IM_VSDTRF_DATABASE_GG);
				break;
			}
		}
	}
}


BOOL CaVsdDatabase::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE, bOk2 = FALSE;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
/*UKS
	try
	{
*/
		CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
		ASSERT (pDisplayInfo);
		if (!pDisplayInfo)
			return FALSE;

		if (GetDifference() == _T('*') || GetDifference() == _T('#'))
		{
			//
			// Compare Detail info of the two objects:
			CaDatabase& obj = GetObject();
			CString strDatabase2 = pRefreshInfo->IsInstallation()? obj.GetName(): pRefreshInfo->GetDatabase2();

			CString strMsg;
			strMsg.Format (IDS_COMPARE_DB, obj.GetOwner(), obj.GetName(), GetOwner2(), strDatabase2);/* _T("compare database [%s].%s <--> [%s].%s \n")*/
			TRACE0 (strMsg);

			CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
			CaLLQueryInfo qry1 (*pQueryInfo);
			CaLLQueryInfo qry2 (*pQueryInfo);
			qry1.SetObjectType(OBT_DATABASE);
			qry2.SetObjectType(OBT_DATABASE);
			qry1.SetNode(pRefreshInfo->GetNode1());
			qry1.SetDatabase(_T("iidbdb"));
			qry2.SetNode(pRefreshInfo->GetNode2());
			qry2.SetDatabase(_T("iidbdb"));

			strMsg.Format (_T("Database: %s <--> %s"), (LPCTSTR)obj.GetName(), (LPCTSTR)strDatabase2);
			pRefreshInfo->ShowAnimateTextInfo(strMsg);


			CmtSessionManager* pSessionManager = GetSessionManager();
			CaDatabaseDetail to1, to2;
			to1.SetItem(obj.GetName(), obj.GetOwner());
			to2.SetItem(strDatabase2, GetOwner2());
			CaDBObject* pDt1 = &to1;
			CaDBObject* pDt2 = &to2;

			if (pRefreshInfo->IsLoadSchema1())
				bOk = VSD_llQueryDetailObject(&qry1, pRefreshInfo->GetLoadedSchema1(), pDt1);
			else
				bOk  = INGRESII_llQueryDetailObject (&qry1, &to1, pSessionManager);

			if (pRefreshInfo->IsLoadSchema2())
				bOk2 = VSD_llQueryDetailObject(&qry2, pRefreshInfo->GetLoadedSchema2(), pDt2);
			else
				bOk2  = INGRESII_llQueryDetailObject (&qry2, &to2, pSessionManager);

			if (bOk && bOk2 && pDt1 && pDt2)
			{
				VSD_CompareDatabase (this, (CaDatabaseDetail*)pDt1, (CaDatabaseDetail*)pDt2);
			}
			if (pRefreshInfo->IsLoadSchema1() && pDt1)
				delete pDt1;
			if (pRefreshInfo->IsLoadSchema2() && pDt2)
				delete pDt2;

			CString strSavedDb1 = pRefreshInfo->GetDatabase1();
			CString strSavedDb2 = pRefreshInfo->GetDatabase2();
			pRefreshInfo->SetDatabase(obj.GetName(), strDatabase2);
			UINT iOldFlag = pRefreshInfo->GetFlag();
			pRefreshInfo->SetFlag(iOldFlag | obj.GetService());
			POSITION pos = m_listObject.GetHeadPosition();
			while (pos != NULL)
			{
				CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
				if (pRefreshInfo->IsInterrupted())
					break;
				ptrfObj->RefreshData(pTree, hItem, pRefreshInfo);
			}
			pRefreshInfo->SetDatabase(strSavedDb1, strSavedDb2);
			pRefreshInfo->SetFlag(iOldFlag);
		}
/*UKS
	}
	catch (...)
	{

	}
*/
	return TRUE;
}


//
// Folder of Database:
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdFolderDatabase, CtrfFolderDatabase, 1)
void CaVsdFolderDatabase::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfFolderDatabase::SetImageID(pDisplayInfo);
	FolderSetImageID(this);
}

BOOL CaVsdFolderDatabase::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE, bOk2;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
	CTypedPtrList< CObList, CaDBObject* > lNew, lNew2;
	CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
	ASSERT (pQueryInfo);
	if (!pQueryInfo)
		return FALSE;

	CaLLQueryInfo queryInfo1(*pQueryInfo);
	CaLLQueryInfo queryInfo2(*pQueryInfo);
	queryInfo1.SetNode(pRefreshInfo->GetNode1());
	queryInfo1.SetDatabase(_T("iidbdb"));
	queryInfo2.SetNode(pRefreshInfo->GetNode2());
	queryInfo2.SetDatabase(_T("iidbdb"));

	CmtSessionManager* pSessionManager = GetSessionManager();
	ASSERT (pSessionManager);
	if (!pSessionManager)
		return FALSE;

	CString csTemp;
	csTemp.LoadString(IDS_MSG_WAIT_DB);
	pRefreshInfo->ShowAnimateTextInfo(csTemp);//_T("List of Databases...")

	if (pRefreshInfo->IsLoadSchema1())
		bOk = VSD_llQueryObject (&queryInfo1, pRefreshInfo->GetLoadedSchema1(), lNew);
	else
		bOk = INGRESII_llQueryObject (&queryInfo1, lNew, pSessionManager);

	if (pRefreshInfo->IsLoadSchema2())
		bOk2 = VSD_llQueryObject (&queryInfo2, pRefreshInfo->GetLoadedSchema2(), lNew2);
	else
		bOk2 = INGRESII_llQueryObject (&queryInfo2, lNew2, pSessionManager);

	if (!(bOk && bOk2))
		return FALSE;
	POSITION p = NULL, pos = NULL;
#if defined (_IGNORE_STAR_DATABASE)
	pos = lNew.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		CaDatabase* pDb = (CaDatabase*)lNew.GetNext(pos);
		if (pDb->GetStar() != OBJTYPE_NOTSTAR)
		{
			lNew.RemoveAt(p);
			delete pDb;
		}
	}
#endif
	//
	// Mark all old object as being deleted:
	CtrfItemData* pObj = NULL;
	p = NULL;
	pos = m_listObject.GetHeadPosition();
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
		//
		// New object that is not in the old list, add it to the list:
		CaVsdDatabase* pNewObject = new CaVsdDatabase (pNew);
		pNewObject->SetBackParent (this);
		pNewObject->GetDifference() = _T('+'); // Mark as exist in Installation 1 ( temprary but not in Installation 2)
		m_listObject.AddTail (pNewObject);
		delete pNew;
	}

	//
	// Merging from the Schema2's objects:
	while (!lNew2.IsEmpty())
	{
		CaDatabase* pNew = (CaDatabase*)lNew2.RemoveHead();
		CtrfItemData* pExist = SearchObject(pNew);
		if (pExist != NULL)
		{
			CaVsdDatabase* pCur = (CaVsdDatabase*)pExist;
			TCHAR& tchDiff = pCur->GetDifference();
			tchDiff = _T('*'); // Mark to compare the detail of table
			pCur->Initialize();
			pCur->SetOwner2(pNew->GetOwner());
			delete pNew;
		}
		else
		{
			//
			// New object from database 2:
			CaVsdDatabase* pNewObject = new CaVsdDatabase (pNew);
			pNewObject->SetBackParent (this);
			pNewObject->GetDifference() = _T('-'); // Exist in Database2 but not in Database1
			m_listObject.AddTail (pNewObject);
			delete pNew;
		}
	}
	//
	// Refresh Sub-Branches
	pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
		if (pRefreshInfo->IsInterrupted())
			break;
		ptrfObj->RefreshData(pTree, hItem, pInfo);
	}

	return TRUE;
}


//
// Object: User 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdUser, CtrfUser, 1)
BOOL CaVsdUser::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE, bOk2 = FALSE;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
/*UKS
	try
	{
*/
		CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
		ASSERT (pDisplayInfo);
		if (!pDisplayInfo)
			return FALSE;

		if (GetDifference() == _T('*') || GetDifference() == _T('#'))
		{
			//
			// Compare Detail info of the two objects:
			CaUser& obj = GetObject();
			CString strMsg;
			strMsg.Format (IDS_COMPARE_USER, (LPCTSTR)obj.GetName(), (LPCTSTR)obj.GetName());/*_T("compare user %s <--> %s \n")*/
			TRACE0 (strMsg);

			CmtSessionManager* pSessionManager = GetSessionManager();
			CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
			CaLLQueryInfo queryInfo1 (*pQueryInfo);
			CaLLQueryInfo queryInfo2 (*pQueryInfo);
			queryInfo1.SetObjectType(OBT_USER);
			queryInfo2.SetObjectType(OBT_USER);
			queryInfo1.SetNode(pRefreshInfo->GetNode1());
			queryInfo1.SetDatabase(_T("iidbdb"));
			queryInfo2.SetNode(pRefreshInfo->GetNode2());
			queryInfo2.SetDatabase(_T("iidbdb"));

			strMsg.Format (_T("User: %s"), (LPCTSTR)obj.GetName());
			pRefreshInfo->ShowAnimateTextInfo(strMsg);

			CaUserDetail to1, to2;
			CaDBObject* pDt1 = &to1;
			CaDBObject* pDt2 = &to2;
			to1.SetItem(obj.GetName(), _T(""));
			to2.SetItem(obj.GetName(), _T(""));
			if (pRefreshInfo->IsLoadSchema1())
				bOk = VSD_llQueryDetailObject(&queryInfo1, pRefreshInfo->GetLoadedSchema1(), pDt1);
			else
				bOk  = INGRESII_llQueryDetailObject (&queryInfo1, &to1, pSessionManager);

			if (pRefreshInfo->IsLoadSchema2())
				bOk2 = VSD_llQueryDetailObject(&queryInfo2, pRefreshInfo->GetLoadedSchema2(), pDt2);
			else
				bOk2  = INGRESII_llQueryDetailObject (&queryInfo2, &to2, pSessionManager);

			if (bOk && bOk2 && pDt1 && pDt2)
			{
				VSD_CompareUser (this, (CaUserDetail*)pDt1, (CaUserDetail*)pDt2);
			}
			if (pRefreshInfo->IsLoadSchema1() && pDt1)
				delete pDt1;
			if (pRefreshInfo->IsLoadSchema2() && pDt2)
				delete pDt2;
		}

		POSITION pos = m_listObject.GetHeadPosition();
		while (pos != NULL)
		{
			CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
			if (pRefreshInfo->IsInterrupted())
				break;
			ptrfObj->RefreshData(pTree, hItem, pInfo);
		}
/*UKS
	}
	catch (...)
	{

	}
*/
	return TRUE;
}

void CaVsdUser::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfUser::SetImageID(pDisplayInfo);
	if (IsDiscarded())
	{
		GetTreeCtrlData().SetImage (IM_VSDTRFd, IM_VSDTRFd);
	}
	else
	{
		BOOL bHasDiscardInSubBranches = FALSE;
		TCHAR ch = HasDifferences ();
		VSD_HasDiscadItem(this, bHasDiscardInSubBranches, TRUE);
		if (bHasDiscardInSubBranches)
		{
			switch (ch)
			{
			case _T('+'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_USER_GR, IM_VSDTRFd_USER_GR);
				break;
			case _T('-'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_USER_RG, IM_VSDTRFd_USER_RG);
				break;
			case _T('!'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_USER_YY, IM_VSDTRFd_USER_YY);
				break;
			default:
				GetTreeCtrlData().SetImage (IM_VSDTRFd_USER_GG, IM_VSDTRFd_USER_GG);
				break;
			}
		}
		else
		{
			switch (ch)
			{
			case _T('+'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_USER_GR, IM_VSDTRF_USER_GR);
				break;
			case _T('-'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_USER_RG, IM_VSDTRF_USER_RG);
				break;
			case _T('!'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_USER_YY, IM_VSDTRF_USER_YY);
				break;
			default:
				GetTreeCtrlData().SetImage (IM_VSDTRF_USER_GG, IM_VSDTRF_USER_GG);
				break;
			}
		}
	}
}


#if defined (_DEBUG) && defined(_SHOWDIFF_SYMBOL)
CString CaVsdUser::GetDisplayedItem(int nMode)
{
	CString strFmt;
	strFmt.Format(_T("{%c %d}   "), GetDifference(), IsMultiple());
	strFmt+= CtrfUser::GetDisplayedItem(nMode);
	return strFmt;
}
#endif

//
// Object: Folder of User 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdFolderUser, CtrfFolderUser, 1)
void CaVsdFolderUser::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfFolderUser::SetImageID(pDisplayInfo);
	FolderSetImageID(this);
}

BOOL CaVsdFolderUser::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE, bOk2;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
	CTypedPtrList< CObList, CaDBObject* > lNew, lNew2;
	CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
	ASSERT (pQueryInfo);
	if (!pQueryInfo)
		return FALSE;

	CaLLQueryInfo queryInfo1(*pQueryInfo);
	CaLLQueryInfo queryInfo2(*pQueryInfo);
	queryInfo1.SetNode(pRefreshInfo->GetNode1());
	queryInfo2.SetNode(pRefreshInfo->GetNode2());
	queryInfo1.SetDatabase(_T("iidbdb"));
	queryInfo2.SetDatabase(_T("iidbdb"));

	CmtSessionManager* pSessionManager = GetSessionManager();
	ASSERT (pSessionManager);
	if (!pSessionManager)
		return FALSE;

	CString csTemp;
	csTemp.LoadString(IDS_MSG_WAIT_USERS);
	pRefreshInfo->ShowAnimateTextInfo(csTemp);//_T("List of Users...")

	if (pRefreshInfo->IsLoadSchema1())
		bOk = VSD_llQueryObject (&queryInfo1, pRefreshInfo->GetLoadedSchema1(), lNew);
	else
		bOk = INGRESII_llQueryObject (&queryInfo1, lNew, pSessionManager);

	if (pRefreshInfo->IsLoadSchema2())
		bOk2 = VSD_llQueryObject (&queryInfo2, pRefreshInfo->GetLoadedSchema2(), lNew2);
	else
		bOk2 = INGRESII_llQueryObject (&queryInfo2, lNew2, pSessionManager);

	if (!(bOk && bOk2))
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
		CaUser* pNew = (CaUser*)lNew.RemoveHead();
		//
		// New object that is not in the old list, add it to the list:
		CaVsdUser* pNewObject = new CaVsdUser (pNew);
		pNewObject->SetBackParent (this);
		pNewObject->GetDifference() = _T('+'); // Mark as exist in Installation 1 ( temprary but not in Installation 2)
		m_listObject.AddTail (pNewObject);

		delete pNew;
	}

	//
	// Merging from the Schema2's objects:
	CtrfItemData* pExist = NULL;
	CaVsdFolderUser fdObject;
	CTypedPtrList< CObList, CtrfItemData* >& ls2 = fdObject.GetListObject();
	while (!lNew2.IsEmpty())
	{
		CaUser* pNew = (CaUser*)lNew2.RemoveHead();
		//
		// New object that is not in the old list, add it to the list:
		CaVsdUser* pNewObject = new CaVsdUser (pNew);
		pNewObject->SetBackParent (this);
		if (pExist)
			pNewObject->SetMultiple();
		ls2.AddTail (pNewObject);
		delete pNew;
	}

	while (!ls2.IsEmpty())
	{
		BOOL b2Add = TRUE;
		CaVsdUser* pNew = (CaVsdUser*)ls2.RemoveHead();
		CaUser& obj = pNew->GetObject();
		CtrfItemData* pExist = SearchObject(&obj);
		if (pExist != NULL)
		{
			CaVsdUser* pCur = (CaVsdUser*)pExist;
			TCHAR& tchDiff = pCur->GetDifference();
			tchDiff = _T('#'); // Mark to compare the detail of table
			pCur->Initialize();
			delete pNew;
			b2Add = FALSE;
		}
	
		if (b2Add)
		{
			//
			// New object from database 2:
			pNew->SetBackParent (this);
			pNew->GetDifference() = _T('-'); // Exist in Database2 but not in Database1
			m_listObject.AddTail (pNew);
		}
	}
	
	//
	// Refresh Sub-Branches
	pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
		if (pRefreshInfo->IsInterrupted())
			break;
		ptrfObj->RefreshData(pTree, hItem, pInfo);
	}

	return TRUE;
}


//
// Object: Group 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdGroup, CtrfGroup, 1)
BOOL CaVsdGroup::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE, bOk2 = FALSE;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
/*UKS
	try
	{
*/
		CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
		ASSERT (pDisplayInfo);
		if (!pDisplayInfo)
			return FALSE;

		if (GetDifference() == _T('*') || GetDifference() == _T('#'))
		{
			//
			// Compare Detail info of the two objects:
			CaGroup& obj = GetObject();
			CString strMsg;
			strMsg.Format (IDS_COMPARE_GROUP, (LPCTSTR)obj.GetName(), (LPCTSTR)obj.GetName());/*_T("compare group %s <--> %s \n")*/
			TRACE0 (strMsg);

			CmtSessionManager* pSessionManager = GetSessionManager();
			CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
			CaLLQueryInfo queryInfo1 (*pQueryInfo);
			CaLLQueryInfo queryInfo2 (*pQueryInfo);
			queryInfo1.SetObjectType(OBT_GROUP);
			queryInfo2.SetObjectType(OBT_GROUP);
			queryInfo1.SetNode(pRefreshInfo->GetNode1());
			queryInfo1.SetDatabase(_T("iidbdb"));
			queryInfo2.SetNode(pRefreshInfo->GetNode2());
			queryInfo2.SetDatabase(_T("iidbdb"));

			strMsg.Format (_T("Group: %s"), (LPCTSTR)obj.GetName());
			pRefreshInfo->ShowAnimateTextInfo(strMsg);

			CaGroupDetail to1, to2;
			CaDBObject* pDt1 = &to1;
			CaDBObject* pDt2 = &to2;
			to1.SetItem(obj.GetName(), _T(""));
			to2.SetItem(obj.GetName(), _T(""));
			if (pRefreshInfo->IsLoadSchema1())
				bOk = VSD_llQueryDetailObject(&queryInfo1, pRefreshInfo->GetLoadedSchema1(), pDt1);
			else
				bOk  = INGRESII_llQueryDetailObject (&queryInfo1, &to1, pSessionManager);

			if (pRefreshInfo->IsLoadSchema2())
				bOk2 = VSD_llQueryDetailObject(&queryInfo2, pRefreshInfo->GetLoadedSchema2(), pDt2);
			else
				bOk2  = INGRESII_llQueryDetailObject (&queryInfo2, &to2, pSessionManager);

			if (bOk && bOk2 && pDt1 && pDt2)
			{
				VSD_CompareGroup (this, (CaGroupDetail*)pDt1, (CaGroupDetail*)pDt2);
			}
			if (pRefreshInfo->IsLoadSchema1() && pDt1)
				delete pDt1;
			if (pRefreshInfo->IsLoadSchema2() && pDt2)
				delete pDt2;
		}

		POSITION pos = m_listObject.GetHeadPosition();
		while (pos != NULL)
		{
			CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
			if (pRefreshInfo->IsInterrupted())
				break;
			ptrfObj->RefreshData(pTree, hItem, pInfo);
		}
/*UKS
	}
	catch (...)
	{

	}
*/
	return TRUE;
}

void CaVsdGroup::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfGroup::SetImageID(pDisplayInfo);
	if (IsDiscarded())
	{
		GetTreeCtrlData().SetImage (IM_VSDTRFd, IM_VSDTRFd);
	}
	else
	{
		BOOL bHasDiscardInSubBranches = FALSE;
		TCHAR ch = HasDifferences ();
		VSD_HasDiscadItem(this, bHasDiscardInSubBranches, TRUE);
		if (bHasDiscardInSubBranches)
		{
			switch (ch)
			{
			case _T('+'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_GROUP_GR, IM_VSDTRFd_GROUP_GR);
				break;
			case _T('-'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_GROUP_RG, IM_VSDTRFd_GROUP_RG);
				break;
			case _T('!'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_GROUP_YY, IM_VSDTRFd_GROUP_YY);
				break;
			default:
				GetTreeCtrlData().SetImage (IM_VSDTRFd_GROUP_GG, IM_VSDTRFd_GROUP_GG);
				break;
			}
		}
		else
		{
			switch (ch)
			{
			case _T('+'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_GROUP_GR, IM_VSDTRF_GROUP_GR);
				break;
			case _T('-'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_GROUP_RG, IM_VSDTRF_GROUP_RG);
				break;
			case _T('!'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_GROUP_YY, IM_VSDTRF_GROUP_YY);
				break;
			default:
				GetTreeCtrlData().SetImage (IM_VSDTRF_GROUP_GG, IM_VSDTRF_GROUP_GG);
				break;
			}
		}
	}
}

#if defined (_DEBUG) && defined(_SHOWDIFF_SYMBOL)
CString CaVsdGroup::GetDisplayedItem(int nMode)
{
	CString strFmt;
	strFmt.Format(_T("{%c %d}   "), GetDifference(), IsMultiple());
	strFmt+= CtrfGroup::GetDisplayedItem(nMode);
	return strFmt;
}
#endif

//
// Object: Folder of Group 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdFolderGroup, CtrfFolderGroup, 1)
void CaVsdFolderGroup::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfFolderGroup::SetImageID(pDisplayInfo);
	FolderSetImageID(this);
}

BOOL CaVsdFolderGroup::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE, bOk2;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
	CTypedPtrList< CObList, CaDBObject* > lNew, lNew2;
	CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
	ASSERT (pQueryInfo);
	if (!pQueryInfo)
		return FALSE;

	CaLLQueryInfo queryInfo1(*pQueryInfo);
	CaLLQueryInfo queryInfo2(*pQueryInfo);
	queryInfo1.SetNode(pRefreshInfo->GetNode1());
	queryInfo2.SetNode(pRefreshInfo->GetNode2());
	queryInfo1.SetDatabase(_T("iidbdb"));
	queryInfo2.SetDatabase(_T("iidbdb"));

	CmtSessionManager* pSessionManager = GetSessionManager();
	ASSERT (pSessionManager);
	if (!pSessionManager)
		return FALSE;

	CString csTemp;
	csTemp.LoadString(IDS_MSG_WAIT_GROUPS);
	pRefreshInfo->ShowAnimateTextInfo(csTemp);//_T("List of Groups...")

	if (pRefreshInfo->IsLoadSchema1())
		bOk = VSD_llQueryObject (&queryInfo1, pRefreshInfo->GetLoadedSchema1(), lNew);
	else
		bOk = INGRESII_llQueryObject (&queryInfo1, lNew, pSessionManager);

	if (pRefreshInfo->IsLoadSchema2())
		bOk2 = VSD_llQueryObject (&queryInfo2, pRefreshInfo->GetLoadedSchema2(), lNew2);
	else
		bOk2 = INGRESII_llQueryObject (&queryInfo2, lNew2, pSessionManager);

	if (!(bOk && bOk2))
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
		CaGroup* pNew = (CaGroup*)lNew.RemoveHead();
		//
		// New object that is not in the old list, add it to the list:
		CaVsdGroup* pNewObject = new CaVsdGroup (pNew);
		pNewObject->SetBackParent (this);
		pNewObject->GetDifference() = _T('+'); // Mark as exist in Installation 1 ( temprary but not in Installation 2)
		m_listObject.AddTail (pNewObject);

		delete pNew;
	}

	//
	// Merging from the Schema2's objects:
	CtrfItemData* pExist = NULL;
	CaVsdFolderGroup fdObject;
	CTypedPtrList< CObList, CtrfItemData* >& ls2 = fdObject.GetListObject();
	while (!lNew2.IsEmpty())
	{
		CaGroup* pNew = (CaGroup*)lNew2.RemoveHead();
		//
		// New object that is not in the old list, add it to the list:
		CaVsdGroup* pNewObject = new CaVsdGroup (pNew);
		pNewObject->SetBackParent (this);
		ls2.AddTail (pNewObject);
		delete pNew;
	}

	while (!ls2.IsEmpty())
	{
		BOOL b2Add = TRUE;
		CaVsdGroup* pNew = (CaVsdGroup*)ls2.RemoveHead();
		CaGroup& obj = pNew->GetObject();
		CtrfItemData* pExist = SearchObject(&obj);
		if (pExist != NULL)
		{
			CaVsdGroup* pCur = (CaVsdGroup*)pExist;
			TCHAR& tchDiff = pCur->GetDifference();
			tchDiff = _T('#'); // Mark to compare the detail of table
			pCur->SetOwner2(obj.GetOwner());
			pCur->Initialize();
			delete pNew;
			b2Add = FALSE;
		}

		if (b2Add)
		{
			//
			// New object from database 2:
			pNew->SetBackParent (this);
			pNew->GetDifference() = _T('-'); // Exist in Database2 but not in Database1
			m_listObject.AddTail (pNew);
		}
	}

	//
	// Refresh Sub-Branches
	pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
		if (pRefreshInfo->IsInterrupted())
			break;
		ptrfObj->RefreshData(pTree, hItem, pInfo);
	}

	return TRUE;
}

//
// Object: Role 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdRole, CtrfRole, 1)
BOOL CaVsdRole::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE, bOk2 = FALSE;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
/*UKS
	try
	{
*/
		CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
		ASSERT (pDisplayInfo);
		if (!pDisplayInfo)
			return FALSE;

		if (GetDifference() == _T('*') || GetDifference() == _T('#'))
		{
			//
			// Compare Detail info of the two objects:
			CaRole& obj = GetObject();
			CString strMsg;
			strMsg.Format (IDS_COMPARE_ROLE, (LPCTSTR)obj.GetName(), (LPCTSTR)obj.GetName());/*_T("compare role %s <--> %s \n")*/
			TRACE0 (strMsg);

			CmtSessionManager* pSessionManager = GetSessionManager();
			CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
			CaLLQueryInfo queryInfo1 (*pQueryInfo);
			CaLLQueryInfo queryInfo2 (*pQueryInfo);
			queryInfo1.SetObjectType(OBT_ROLE);
			queryInfo2.SetObjectType(OBT_ROLE);
			queryInfo1.SetNode(pRefreshInfo->GetNode1());
			queryInfo1.SetDatabase(_T("iidbdb"));
			queryInfo2.SetNode(pRefreshInfo->GetNode2());
			queryInfo2.SetDatabase(_T("iidbdb"));

			strMsg.Format (_T("Role: %s"), (LPCTSTR)obj.GetName());
			pRefreshInfo->ShowAnimateTextInfo(strMsg);

			CaRoleDetail to1, to2;
			CaDBObject* pDt1 = &to1;
			CaDBObject* pDt2 = &to2;
			to1.SetItem(obj.GetName(), _T(""));
			to2.SetItem(obj.GetName(), _T(""));
			if (pRefreshInfo->IsLoadSchema1())
				bOk = VSD_llQueryDetailObject(&queryInfo1, pRefreshInfo->GetLoadedSchema1(), pDt1);
			else
				bOk  = INGRESII_llQueryDetailObject (&queryInfo1, &to1, pSessionManager);

			if (pRefreshInfo->IsLoadSchema2())
				bOk2 = VSD_llQueryDetailObject(&queryInfo2, pRefreshInfo->GetLoadedSchema2(), pDt2);
			else
				bOk2  = INGRESII_llQueryDetailObject (&queryInfo2, &to2, pSessionManager);

			if (bOk && bOk2 && pDt1 && pDt2)
			{
				VSD_CompareRole (this, (CaRoleDetail*)pDt1, (CaRoleDetail*)pDt2);
			}
			if (pRefreshInfo->IsLoadSchema1() && pDt1)
				delete pDt1;
			if (pRefreshInfo->IsLoadSchema2() && pDt2)
				delete pDt2;
		}

		POSITION pos = m_listObject.GetHeadPosition();
		while (pos != NULL)
		{
			CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
			if (pRefreshInfo->IsInterrupted())
				break;
			ptrfObj->RefreshData(pTree, hItem, pInfo);
		}
/*UKS
	}
	catch (...)
	{

	}
*/
	return TRUE;
}

void CaVsdRole::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfRole::SetImageID(pDisplayInfo);
	if (IsDiscarded())
	{
		GetTreeCtrlData().SetImage (IM_VSDTRFd, IM_VSDTRFd);
	}
	else
	{
		BOOL bHasDiscardInSubBranches = FALSE;
		TCHAR ch = HasDifferences ();
		VSD_HasDiscadItem(this, bHasDiscardInSubBranches, TRUE);
		if (bHasDiscardInSubBranches)
		{
			switch (ch)
			{
			case _T('+'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_ROLE_GR, IM_VSDTRFd_ROLE_GR);
				break;
			case _T('-'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_ROLE_RG, IM_VSDTRFd_ROLE_RG);
				break;
			case _T('!'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_ROLE_YY, IM_VSDTRFd_ROLE_YY);
				break;
			default:
				GetTreeCtrlData().SetImage (IM_VSDTRFd_ROLE_GG, IM_VSDTRFd_ROLE_GG);
				break;
			}
		}
		else
		{
			switch (ch)
			{
			case _T('+'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_ROLE_GR, IM_VSDTRF_ROLE_GR);
				break;
			case _T('-'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_ROLE_RG, IM_VSDTRF_ROLE_RG);
				break;
			case _T('!'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_ROLE_YY, IM_VSDTRF_ROLE_YY);
				break;
			default:
				GetTreeCtrlData().SetImage (IM_VSDTRF_ROLE_GG, IM_VSDTRF_ROLE_GG);
				break;
			}
		}
	}
}

#if defined (_DEBUG) && defined(_SHOWDIFF_SYMBOL)
CString CaVsdRole::GetDisplayedItem(int nMode)
{
	CString strFmt;
	strFmt.Format(_T("{%c %d}   "), GetDifference(), IsMultiple());
	strFmt+= CtrfRole::GetDisplayedItem(nMode);
	return strFmt;
}
#endif


//
// Object: Folder of Role 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdFolderRole, CtrfFolderRole, 1)
void CaVsdFolderRole::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfFolderRole::SetImageID(pDisplayInfo);
	FolderSetImageID(this);
}

BOOL CaVsdFolderRole::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE, bOk2;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
	CTypedPtrList< CObList, CaDBObject* > lNew, lNew2;
	CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
	ASSERT (pQueryInfo);
	if (!pQueryInfo)
		return FALSE;

	CaLLQueryInfo queryInfo1(*pQueryInfo);
	CaLLQueryInfo queryInfo2(*pQueryInfo);
	queryInfo1.SetNode(pRefreshInfo->GetNode1());
	queryInfo2.SetNode(pRefreshInfo->GetNode2());
	queryInfo1.SetDatabase(_T("iidbdb"));
	queryInfo2.SetDatabase(_T("iidbdb"));

	CmtSessionManager* pSessionManager = GetSessionManager();
	ASSERT (pSessionManager);
	if (!pSessionManager)
		return FALSE;

	CString csTemp;
	csTemp.LoadString(IDS_MSG_WAIT_ROLES);
	pRefreshInfo->ShowAnimateTextInfo(csTemp);//_T("List of Roles...")

	if (pRefreshInfo->IsLoadSchema1())
		bOk = VSD_llQueryObject (&queryInfo1, pRefreshInfo->GetLoadedSchema1(), lNew);
	else
		bOk = INGRESII_llQueryObject (&queryInfo1, lNew, pSessionManager);

	if (pRefreshInfo->IsLoadSchema2())
		bOk2 = VSD_llQueryObject (&queryInfo2, pRefreshInfo->GetLoadedSchema2(), lNew2);
	else
		bOk2 = INGRESII_llQueryObject (&queryInfo2, lNew2, pSessionManager);

	if (!(bOk && bOk2))
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
		CaRole* pNew = (CaRole*)lNew.RemoveHead();
		//
		// New object that is not in the old list, add it to the list:
		CaVsdRole* pNewObject = new CaVsdRole (pNew);
		pNewObject->SetBackParent (this);
		pNewObject->GetDifference() = _T('+'); // Mark as exist in Installation 1 ( temprary but not in Installation 2)
		m_listObject.AddTail (pNewObject);
		delete pNew;
	}

	//
	// Merging from the Schema2's objects:

	
	CtrfItemData* pExist = NULL;
	CaVsdFolderRole fdObject;
	CTypedPtrList< CObList, CtrfItemData* >& ls2 = fdObject.GetListObject();
	while (!lNew2.IsEmpty())
	{
		CaRole* pNew = (CaRole*)lNew2.RemoveHead();

		//
		// New object that is not in the old list, add it to the list:
		CaVsdRole* pNewObject = new CaVsdRole (pNew);
		pNewObject->SetBackParent (this);
		ls2.AddTail (pNewObject);
		delete pNew;
	}

	while (!ls2.IsEmpty())
	{
		BOOL b2Add = TRUE;
		CaVsdRole* pNew = (CaVsdRole*)ls2.RemoveHead();
		CaRole& obj = pNew->GetObject();
		CtrfItemData* pExist = SearchObject(&obj);
		if (pExist != NULL)
		{
			CaVsdRole* pCur = (CaVsdRole*)pExist;
			TCHAR& tchDiff = pCur->GetDifference();
			tchDiff = _T('#'); // Mark to compare the detail of table
			pCur->SetOwner2(obj.GetOwner());
			pCur->Initialize();
			delete pNew;
			b2Add = FALSE;
		}

		if (b2Add)
		{
			//
			// New object from database 2:
			pNew->SetBackParent (this);
			pNew->GetDifference() = _T('-'); // Exist in Database2 but not in Database1
			m_listObject.AddTail (pNew);
		}
	}
	
	//
	// Refresh Sub-Branches
	pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
		if (pRefreshInfo->IsInterrupted())
			break;
		ptrfObj->RefreshData(pTree, hItem, pInfo);
	}

	return TRUE;
}



//
// Object: Profile 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdProfile, CtrfProfile, 1)
BOOL CaVsdProfile::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE, bOk2 = FALSE;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
/*UKS
	try
	{
*/
		CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
		ASSERT (pDisplayInfo);
		if (!pDisplayInfo)
			return FALSE;

		if (GetDifference() == _T('*') || GetDifference() == _T('#'))
		{
			//
			// Compare Detail info of the two objects:
			CaProfile& obj = GetObject();
			CString strMsg;
			strMsg.Format (IDS_COMPARE_PROFILE, (LPCTSTR)obj.GetName(), (LPCTSTR)obj.GetName());/*_T("compare profile %s <--> %s \n")*/
			TRACE0 (strMsg);

			CmtSessionManager* pSessionManager = GetSessionManager();
			CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
			CaLLQueryInfo queryInfo1 (*pQueryInfo);
			CaLLQueryInfo queryInfo2 (*pQueryInfo);
			queryInfo1.SetObjectType(OBT_PROFILE);
			queryInfo2.SetObjectType(OBT_PROFILE);
			queryInfo1.SetNode(pRefreshInfo->GetNode1());
			queryInfo1.SetDatabase(_T("iidbdb"));
			queryInfo2.SetNode(pRefreshInfo->GetNode2());
			queryInfo2.SetDatabase(_T("iidbdb"));

			strMsg.Format (_T("Profile: %s"), (LPCTSTR)obj.GetName());
			pRefreshInfo->ShowAnimateTextInfo(strMsg);

			CaProfileDetail to1, to2;
			CaDBObject* pDt1 = &to1;
			CaDBObject* pDt2 = &to2;
			to1.SetItem(obj.GetName(), _T(""));
			to2.SetItem(obj.GetName(), _T(""));
			if (pRefreshInfo->IsLoadSchema1())
				bOk = VSD_llQueryDetailObject(&queryInfo1, pRefreshInfo->GetLoadedSchema1(), pDt1);
			else
				bOk  = INGRESII_llQueryDetailObject (&queryInfo1, &to1, pSessionManager);

			if (pRefreshInfo->IsLoadSchema2())
				bOk2 = VSD_llQueryDetailObject(&queryInfo2, pRefreshInfo->GetLoadedSchema2(), pDt2);
			else
				bOk2  = INGRESII_llQueryDetailObject (&queryInfo2, &to2, pSessionManager);

			if (bOk && bOk2 && pDt1 && pDt2)
			{
				VSD_CompareProfile (this, (CaProfileDetail*)pDt1, (CaProfileDetail*)pDt2);
			}
			if (pRefreshInfo->IsLoadSchema1() && pDt1)
				delete pDt1;
			if (pRefreshInfo->IsLoadSchema2() && pDt2)
				delete pDt2;
		}

		POSITION pos = m_listObject.GetHeadPosition();
		while (pos != NULL)
		{
			CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
			if (pRefreshInfo->IsInterrupted())
				break;
			ptrfObj->RefreshData(pTree, hItem, pInfo);
		}
/*UKS
	}
	catch (...)
	{

	}
*/
	return TRUE;
}

void CaVsdProfile::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfProfile::SetImageID(pDisplayInfo);
	if (IsDiscarded())
	{
		GetTreeCtrlData().SetImage (IM_VSDTRFd, IM_VSDTRFd);
	}
	else
	{
		BOOL bHasDiscardInSubBranches = FALSE;
		TCHAR ch = HasDifferences ();
		VSD_HasDiscadItem(this, bHasDiscardInSubBranches, TRUE);
		if (bHasDiscardInSubBranches)
		{
			switch (ch)
			{
			case _T('+'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_PROFILE_GR, IM_VSDTRFd_PROFILE_GR);
				break;
			case _T('-'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_PROFILE_RG, IM_VSDTRFd_PROFILE_RG);
				break;
			case _T('!'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_PROFILE_YY, IM_VSDTRFd_PROFILE_YY);
				break;
			default:
				GetTreeCtrlData().SetImage (IM_VSDTRFd_PROFILE_GG, IM_VSDTRFd_PROFILE_GG);
				break;
			}
		}
		else
		{
			switch (ch)
			{
			case _T('+'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_PROFILE_GR, IM_VSDTRF_PROFILE_GR);
				break;
			case _T('-'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_PROFILE_RG, IM_VSDTRF_PROFILE_RG);
				break;
			case _T('!'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_PROFILE_YY, IM_VSDTRF_PROFILE_YY);
				break;
			default:
				GetTreeCtrlData().SetImage (IM_VSDTRF_PROFILE_GG, IM_VSDTRF_PROFILE_GG);
				break;
			}
		}
	}
}

#if defined (_DEBUG) && defined(_SHOWDIFF_SYMBOL)
CString CaVsdProfile::GetDisplayedItem(int nMode)
{
	CString strFmt;
	strFmt.Format(_T("{%c %d}   "), GetDifference(), IsMultiple());
	strFmt+= CtrfProfile::GetDisplayedItem(nMode);
	return strFmt;
}
#endif

//
// Object: Folder of Profile 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdFolderProfile, CtrfFolderProfile, 1)
void CaVsdFolderProfile::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfFolderProfile::SetImageID(pDisplayInfo);
	FolderSetImageID(this);
}

BOOL CaVsdFolderProfile::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE, bOk2;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
	CTypedPtrList< CObList, CaDBObject* > lNew, lNew2;
	CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
	ASSERT (pQueryInfo);
	if (!pQueryInfo)
		return FALSE;

	CaLLQueryInfo queryInfo1(*pQueryInfo);
	CaLLQueryInfo queryInfo2(*pQueryInfo);
	queryInfo1.SetNode(pRefreshInfo->GetNode1());
	queryInfo2.SetNode(pRefreshInfo->GetNode2());
	queryInfo1.SetDatabase(_T("iidbdb"));
	queryInfo2.SetDatabase(_T("iidbdb"));

	CmtSessionManager* pSessionManager = GetSessionManager();
	ASSERT (pSessionManager);
	if (!pSessionManager)
		return FALSE;

	CString csTemp;
	csTemp.LoadString(IDS_MSG_WAIT_PROFILES);
	pRefreshInfo->ShowAnimateTextInfo(csTemp);//_T("List of Profiles...")

	if (pRefreshInfo->IsLoadSchema1())
		bOk = VSD_llQueryObject (&queryInfo1, pRefreshInfo->GetLoadedSchema1(), lNew);
	else
		bOk = INGRESII_llQueryObject (&queryInfo1, lNew, pSessionManager);

	if (pRefreshInfo->IsLoadSchema2())
		bOk2 = VSD_llQueryObject (&queryInfo2, pRefreshInfo->GetLoadedSchema2(), lNew2);
	else
		bOk2 = INGRESII_llQueryObject (&queryInfo2, lNew2, pSessionManager);

	if (!(bOk && bOk2))
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
		CaProfile* pNew = (CaProfile*)lNew.RemoveHead();
		//
		// New object that is not in the old list, add it to the list:
		CaVsdProfile* pNewObject = new CaVsdProfile (pNew);
		pNewObject->SetBackParent (this);
		pNewObject->GetDifference() = _T('+'); // Mark as exist in Installation 1 ( temprary but not in Installation 2)
		m_listObject.AddTail (pNewObject);
		delete pNew;
	}

	//
	// Merging from the Schema2's objects:
	CtrfItemData* pExist = NULL;
	CaVsdFolderProfile fdObject;
	CTypedPtrList< CObList, CtrfItemData* >& ls2 = fdObject.GetListObject();
	while (!lNew2.IsEmpty())
	{
		CaProfile* pNew = (CaProfile*)lNew2.RemoveHead();
		//
		// New object that is not in the old list, add it to the list:
		CaVsdProfile* pNewObject = new CaVsdProfile (pNew);
		pNewObject->SetBackParent (this);
		ls2.AddTail (pNewObject);
		delete pNew;
	}

	while (!ls2.IsEmpty())
	{
		BOOL b2Add = TRUE;
		CaVsdProfile* pNew = (CaVsdProfile*)ls2.RemoveHead();
		CaProfile& obj = pNew->GetObject();
		CtrfItemData* pExist = SearchObject(&obj);
		if (pExist != NULL)
		{
			CaVsdProfile* pCur = (CaVsdProfile*)pExist;
			TCHAR& tchDiff = pCur->GetDifference();
			tchDiff = _T('#'); // Mark to compare the detail of table
			pCur->SetOwner2(obj.GetOwner());
			pCur->Initialize();
			delete pNew;
			b2Add = FALSE;
		}

		if (b2Add)
		{
			//
			// New object from database 2:
			pNew->SetBackParent (this);
			pNew->GetDifference() = _T('-'); // Exist in Database2 but not in Database1
			m_listObject.AddTail (pNew);
		}
	}

	//
	// Refresh Sub-Branches
	pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
		if (pRefreshInfo->IsInterrupted())
			break;
		ptrfObj->RefreshData(pTree, hItem, pInfo);
	}

	return TRUE;
}

//
// Object: Location 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdLocation, CtrfLocation, 1)
BOOL CaVsdLocation::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE, bOk2 = FALSE;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
/*UKS
	try
	{
*/
		CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
		ASSERT (pDisplayInfo);
		if (!pDisplayInfo)
			return FALSE;

		if (GetDifference() == _T('*') || GetDifference() == _T('#'))
		{
			//
			// Compare Detail info of the two objects:
			CaLocation& obj = GetObject();
			CString strMsg;
			strMsg.Format (IDS_COMPARE_LOC, (LPCTSTR)obj.GetName(), (LPCTSTR)obj.GetName());/* _T("compare location %s <--> %s \n")*/
			TRACE0 (strMsg);

			CmtSessionManager* pSessionManager = GetSessionManager();
			CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
			CaLLQueryInfo queryInfo1 (*pQueryInfo);
			CaLLQueryInfo queryInfo2 (*pQueryInfo);
			queryInfo1.SetObjectType(OBT_LOCATION);
			queryInfo2.SetObjectType(OBT_LOCATION);
			queryInfo1.SetNode(pRefreshInfo->GetNode1());
			queryInfo1.SetDatabase(_T("iidbdb"));
			queryInfo2.SetNode(pRefreshInfo->GetNode2());
			queryInfo2.SetDatabase(_T("iidbdb"));

			strMsg.Format (_T("Location: %s"), (LPCTSTR)obj.GetName());
			pRefreshInfo->ShowAnimateTextInfo(strMsg);

			CaLocationDetail to1, to2;
			CaDBObject* pDt1 = &to1;
			CaDBObject* pDt2 = &to2;
			to1.SetItem(obj.GetName(), _T(""));
			to2.SetItem(obj.GetName(), _T(""));
			if (pRefreshInfo->IsLoadSchema1())
				bOk = VSD_llQueryDetailObject(&queryInfo1, pRefreshInfo->GetLoadedSchema1(), pDt1);
			else
				bOk  = INGRESII_llQueryDetailObject (&queryInfo1, &to1, pSessionManager);

			if (pRefreshInfo->IsLoadSchema2())
				bOk2 = VSD_llQueryDetailObject(&queryInfo2, pRefreshInfo->GetLoadedSchema2(), pDt2);
			else
				bOk2  = INGRESII_llQueryDetailObject (&queryInfo2, &to2, pSessionManager);

			if (bOk && bOk2 && pDt1 && pDt2)
			{
				VSD_CompareLocation (this, (CaLocationDetail*)pDt1, (CaLocationDetail*)pDt2);
			}
			if (pRefreshInfo->IsLoadSchema1() && pDt1)
				delete pDt1;
			if (pRefreshInfo->IsLoadSchema2() && pDt2)
				delete pDt2;
		}

		POSITION pos = m_listObject.GetHeadPosition();
		while (pos != NULL)
		{
			CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
			if (pRefreshInfo->IsInterrupted())
				break;
			ptrfObj->RefreshData(pTree, hItem, pInfo);
		}
/*UKS
	}
	catch (...)
	{

	}
*/
	return TRUE;
}

void CaVsdLocation::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfLocation::SetImageID(pDisplayInfo);
	if (IsDiscarded())
	{
		GetTreeCtrlData().SetImage (IM_VSDTRFd, IM_VSDTRFd);
	}
	else
	{
		BOOL bHasDiscardInSubBranches = FALSE;
		TCHAR ch = HasDifferences ();
		VSD_HasDiscadItem(this, bHasDiscardInSubBranches, TRUE);
		if (bHasDiscardInSubBranches)
		{
			switch (ch)
			{
			case _T('+'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_LOCATION_GR, IM_VSDTRFd_LOCATION_GR);
				break;
			case _T('-'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_LOCATION_RG, IM_VSDTRFd_LOCATION_RG);
				break;
			case _T('!'):
				GetTreeCtrlData().SetImage (IM_VSDTRFd_LOCATION_YY, IM_VSDTRFd_LOCATION_YY);
				break;
			default:
				GetTreeCtrlData().SetImage (IM_VSDTRFd_LOCATION_GG, IM_VSDTRFd_LOCATION_GG);
				break;
			}
		}
		else
		{
			switch (ch)
			{
			case _T('+'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_LOCATION_GR, IM_VSDTRF_LOCATION_GR);
				break;
			case _T('-'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_LOCATION_RG, IM_VSDTRF_LOCATION_RG);
				break;
			case _T('!'):
				GetTreeCtrlData().SetImage (IM_VSDTRF_LOCATION_YY, IM_VSDTRF_LOCATION_YY);
				break;
			default:
				GetTreeCtrlData().SetImage (IM_VSDTRF_LOCATION_GG, IM_VSDTRF_LOCATION_GG);
				break;
			}
		}
	}
}

#if defined (_DEBUG) && defined(_SHOWDIFF_SYMBOL)
CString CaVsdLocation::GetDisplayedItem(int nMode)
{
	CString strFmt;
	strFmt.Format(_T("{%c %d}   "), GetDifference(), IsMultiple());
	strFmt+= CtrfLocation::GetDisplayedItem(nMode);
	return strFmt;
}
#endif

//
// Object: Folder of Location 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdFolderLocation, CtrfFolderLocation, 1)
void CaVsdFolderLocation::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	CtrfFolderLocation::SetImageID(pDisplayInfo);
	FolderSetImageID(this);
}

BOOL CaVsdFolderLocation::RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo)
{
	BOOL bOk = FALSE, bOk2;
	CaVsdRefreshTreeInfo* pRefreshInfo = (CaVsdRefreshTreeInfo*)pInfo;
	CTypedPtrList< CObList, CaDBObject* > lNew, lNew2;
	CaLLQueryInfo* pQueryInfo = GetQueryInfo(NULL);
	ASSERT (pQueryInfo);
	if (!pQueryInfo)
		return FALSE;

	CaLLQueryInfo queryInfo1(*pQueryInfo);
	CaLLQueryInfo queryInfo2(*pQueryInfo);
	queryInfo1.SetNode(pRefreshInfo->GetNode1());
	queryInfo2.SetNode(pRefreshInfo->GetNode2());
	queryInfo1.SetDatabase(_T("iidbdb"));
	queryInfo2.SetDatabase(_T("iidbdb"));

	CmtSessionManager* pSessionManager = GetSessionManager();
	ASSERT (pSessionManager);
	if (!pSessionManager)
		return FALSE;

	CString csTemp;
	csTemp.LoadString(IDS_MSG_WAIT_LOCATIONS);
	pRefreshInfo->ShowAnimateTextInfo(csTemp);//_T("List of Locations...")

	if (pRefreshInfo->IsLoadSchema1())
		bOk = VSD_llQueryObject (&queryInfo1, pRefreshInfo->GetLoadedSchema1(), lNew);
	else
		bOk = INGRESII_llQueryObject (&queryInfo1, lNew, pSessionManager);

	if (pRefreshInfo->IsLoadSchema2())
		bOk2 = VSD_llQueryObject (&queryInfo2, pRefreshInfo->GetLoadedSchema2(), lNew2);
	else
		bOk2 = INGRESII_llQueryObject (&queryInfo2, lNew2, pSessionManager);

	if (!(bOk && bOk2))
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
		CaLocation* pNew = (CaLocation*)lNew.RemoveHead();
		//
		// New object that is not in the old list, add it to the list:
		CaVsdLocation* pNewObject = new CaVsdLocation (pNew);
		pNewObject->SetBackParent (this);
		pNewObject->GetDifference() = _T('+'); // Mark as exist in Installation 1 ( temprary but not in Installation 2)
		m_listObject.AddTail (pNewObject);
		delete pNew;
	}

	//
	// Merging from the Schema2's objects:
	
	CtrfItemData* pExist = NULL;
	CaVsdFolderLocation fdObject;
	CTypedPtrList< CObList, CtrfItemData* >& ls2 = fdObject.GetListObject();
	while (!lNew2.IsEmpty())
	{
		CaLocation* pNew = (CaLocation*)lNew2.RemoveHead();
		//
		// New object that is not in the old list, add it to the list:
		CaVsdLocation* pNewObject = new CaVsdLocation (pNew);
		pNewObject->SetBackParent (this);
		ls2.AddTail (pNewObject);
		delete pNew;
	}

	while (!ls2.IsEmpty())
	{
		BOOL b2Add = TRUE;
		CaVsdLocation* pNew = (CaVsdLocation*)ls2.RemoveHead();
		CaLocation& obj = pNew->GetObject();
		CtrfItemData* pExist = SearchObject(&obj);
		if (pExist != NULL)
		{
			CaVsdLocation* pCur = (CaVsdLocation*)pExist;
			TCHAR& tchDiff = pCur->GetDifference();
			tchDiff = _T('#'); // Mark to compare the detail of table
			pCur->SetOwner2(obj.GetOwner());
			pCur->Initialize();
			delete pNew;
			b2Add = FALSE;
		}

		if (b2Add)
		{
			//
			// New object from database 2:
			pNew->SetBackParent (this);
			pNew->GetDifference() = _T('-'); // Exist in Database2 but not in Database1
			m_listObject.AddTail (pNew);
		}
	}
	
	//
	// Refresh Sub-Branches
	pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
		if (pRefreshInfo->IsInterrupted())
			break;
		ptrfObj->RefreshData(pTree, hItem, pInfo);
	}

	return TRUE;
}

IMPLEMENT_SERIAL (CaVsdRoot, CtrfItemData, 1)
CaVsdRoot::~CaVsdRoot()
{
	Cleanup();
}

void CaVsdRoot::Cleanup()
{
	while (!m_listObject.IsEmpty())
		delete m_listObject.RemoveHead();
	while (!m_listDifference.IsEmpty())
		delete m_listDifference.RemoveHead();
	GetTreeCtrlData().SetTreeItem(NULL);
}

void CaVsdRoot::Initialize(BOOL bInstallation)
{
	CaDisplayInfo* pDisplayInfo = GetDisplayInfo();
#if defined (_DISPLAY_OWNER_OBJECT)
	CString strFormat;
	strFormat.LoadString(IDS_OWNERxITEM);
	pDisplayInfo->SetOwnerPrefixedFormat(strFormat);
#endif
	//
	// Mark to compare database of schema1 with database of schema2
	GetDifference() = _T('*');
	pDisplayInfo->SetEmptyImage (IM_VSDTRF_EMPTY, IM_VSDTRF_EMPTY);
	//
	// Query the system objects ?
	m_queryInfo.SetFetchObjects (CaLLQueryInfo::FETCH_USER);
	if (bInstallation)
	{
		//
		// Folder Group:
		CaVsdFolderGroup* pFolder1 = new CaVsdFolderGroup();
		pFolder1->SetBackParent(this);
		m_listObject.AddTail (pFolder1);
		//
		// Folder Role:
		CaVsdFolderRole* pFolder2 = new CaVsdFolderRole();
		pFolder2->SetBackParent(this);
		m_listObject.AddTail (pFolder2);
		//
		// Folder Profile:
		CaVsdFolderProfile* pFolder3 = new CaVsdFolderProfile();
		pFolder3->SetBackParent(this);
		m_listObject.AddTail (pFolder3);
		//
		// Folder Users:
		CaVsdFolderUser* pFolder4 = new CaVsdFolderUser();
		pFolder4->SetBackParent(this);
		m_listObject.AddTail (pFolder4);
		//
		// Folder Location:
		CaVsdFolderLocation* pFolder5 = new CaVsdFolderLocation();
		pFolder5->SetBackParent(this);
		m_listObject.AddTail (pFolder5);
		//
		// Folder Grantee:
		CaVsdFolderGrantee* pFolder6 = new CaVsdFolderGrantee(OBT_INSTALLATION);
		pFolder6->SetDisplayedItem(_T("Installation Grantees"));
		pFolder6->SetBackParent(this);
		m_listObject.AddTail (pFolder6);
		//
		// Folder Security Alarm:
		CaVsdFolderAlarm* pFolder7 = new CaVsdFolderAlarm(OBT_INSTALLATION);
		pFolder7->SetDisplayedItem(_T("Installation Security Alarms"));
		pFolder7->SetBackParent(this);
		m_listObject.AddTail (pFolder7);
	}
	//
	// Folder Database:
	CaVsdFolderDatabase* pFolder6 = new CaVsdFolderDatabase();
	pFolder6->SetBackParent(this);
	m_listObject.AddTail (pFolder6);
}


CaVsdDatabase* CaVsdRoot::SetSchemaDatabase(LPCTSTR lpszNatabase)
{
	POSITION pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
		if (ptrfObj->IsFolder() && ptrfObj->GetTreeItemType() == O_FOLDER_DATABASE)
		{
			CTypedPtrList< CObList, CtrfItemData* >& listObject = ptrfObj->GetListObject();
			CaDatabase database (lpszNatabase, _T(""));
			CaVsdDatabase* pVsdDatabase = new CaVsdDatabase(&database);
			pVsdDatabase->SetBackParent(ptrfObj);
			listObject.AddTail(pVsdDatabase);
			return pVsdDatabase;
		}
	}
	return NULL;
}


BOOL CaVsdRoot::Populate (CaVsdRefreshTreeInfo* pPopulateData)
{
	BOOL bInterrupted = FALSE;
/*UKS
	try
	{
*/
		if (pPopulateData->IsLoadSchema1())
			(pPopulateData->GetLoadedSchema1())->LoadSchema();
		if (pPopulateData->IsLoadSchema2())
			(pPopulateData->GetLoadedSchema2())->LoadSchema();

		m_strDisplayItem = _T("Installation"); //.Format (_T("Installation: [%s and %s]"), (LPCTSTR)pPopulateData->GetDatabase1(), (LPCTSTR)pPopulateData->GetDatabase2());
		m_queryInfo.SetNode(pPopulateData->GetNode1());
		m_queryInfo.SetItem1(pPopulateData->GetDatabase1(), NULL);

		POSITION pos = m_listObject.GetHeadPosition();
		while (pos != NULL)
		{
			CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
			bInterrupted = pPopulateData->IsInterrupted();
			if (bInterrupted)
				break;
			ptrfObj->RefreshData(NULL, NULL, pPopulateData);
			bInterrupted = pPopulateData->IsInterrupted();
			if (bInterrupted)
				break;
		}
/*UKS
	}
	catch (...)
	{

	}
*/
	pPopulateData->Close();
	return bInterrupted? FALSE: TRUE;
}

void CaVsdRoot::SetImageID(CaDisplayInfo* pDisplayInfo)
{
	if (IsDiscarded())
	{
		GetTreeCtrlData().SetImage (IM_VSDTRFd, IM_VSDTRFd);
	}
	else
	{
		BOOL bHasDiscardInSubBranches = FALSE;
		TCHAR ch = HasDifferences ();
		VSD_HasDiscadItem(this, bHasDiscardInSubBranches, TRUE);
		if (bHasDiscardInSubBranches)
		{
			if (ch == _T('='))
				GetTreeCtrlData().SetImage (IM_VSDTRFd_INSTALLATION_GG, IM_VSDTRFd_INSTALLATION_GG);
			else
				GetTreeCtrlData().SetImage (IM_VSDTRFd_INSTALLATION_YY, IM_VSDTRFd_INSTALLATION_YY);
		}
		else
		{
			if (ch == _T('='))
				GetTreeCtrlData().SetImage (IM_VSDTRF_INSTALLATION_GG, IM_VSDTRF_INSTALLATION_GG);
			else
				GetTreeCtrlData().SetImage (IM_VSDTRF_INSTALLATION_YY, IM_VSDTRF_INSTALLATION_YY);
		}
	}
}

CaVsdDatabase* CaVsdRoot::FindFirstDatabase()
{
	POSITION pos = m_listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CtrfItemData* ptrfObj = m_listObject.GetNext(pos);
		if (ptrfObj->IsFolder() && ptrfObj->GetTreeItemType() == O_FOLDER_DATABASE)
		{
			CTypedPtrList< CObList, CtrfItemData* >& listObject = ptrfObj->GetListObject();
			CaVsdDatabase* pVsdDatabase = (CaVsdDatabase*)listObject.GetHead();
			return pVsdDatabase;
		}
	}
	return NULL;
}
