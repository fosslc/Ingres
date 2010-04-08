/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : treerepl.cpp, implementation file (copy from montree4.cpp)
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Classes for replication branches
**
** History:
**
** 12-Nov-2001 (uk$so01)
**    Created
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "treerepl.h"
#include "ipmdisp.h"    // CuPageInformation

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// CLASS: CuTMReplAllDbStatic
// ************************************************************************************************
IMPLEMENT_SERIAL (CuTMReplAllDbStatic, CTreeItem, 1)
CuTMReplAllDbStatic::CuTMReplAllDbStatic (CTreeGlobalData* pTreeGlobalData)
	:CTreeItem (pTreeGlobalData, 0, IDS_TM_REPLIC_ALLDB, SUBBRANCH_KIND_OBJ, IDB_TM_REPLIC_ALLDB, NULL, OT_DATABASE, IDS_TM_REPLIC_ALLDB_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMReplAllDbStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMReplAllDb* pBranch = new CuTMReplAllDb (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMReplAllDbStatic::CuTMReplAllDbStatic()
{
}

CuPageInformation* CuTMReplAllDbStatic::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = { IDS_TAB_STATIC_REPLICATION, };
	UINT nDlgID [1] = { IDD_IPMPAGE_STATIC_REPLICATIONS, };
	UINT nIDTitle = IDS_TAB_STATIC_REPLICATION_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMReplAllDbStatic"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

//
// CLASS: CuTMReplAllDb
// ************************************************************************************************
IMPLEMENT_SERIAL (CuTMReplAllDb, CTreeItem, 1)
CuTMReplAllDb::CuTMReplAllDb (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_DATABASE, SUBBRANCH_KIND_OBJ, FALSE, pItemData, OT_MON_REPLIC_SERVER, IDS_TM_REPLIC_SERVER_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
	InitializeImageId(IDB_TM_REPLIC_ALLDB);
}

CTreeItem* CuTMReplAllDb::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMReplServer* pBranch = new CuTMReplServer (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMReplAllDb::CuTMReplAllDb()
{
}

UINT CuTMReplAllDb::GetCustomImageId()
{
	// no old code
	LPRESOURCEDATAMIN lpr =(LPRESOURCEDATAMIN)GetPTreeItemData()->GetDataPtr();
	return GetDbTypeImageId(lpr);
}

CuPageInformation* CuTMReplAllDb::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [6] = 
	{
		IDS_TAB_REPSTATIC_ACTIVITY,
		IDS_TAB_REPSTATIC_SERVER,
		IDS_TAB_REPSTATIC_RAISE_EVENT,
		IDS_TAB_REPSTATIC_COLLISION,
		IDS_TAB_REPSTATIC_INTEGRITY,
		IDS_TAB_REPSTATIC_DISTRIB
	};
	UINT nDlgID [6] = 
	{
		IDD_REPSTATIC_PAGE_ACTIVITY,
		IDD_REPSTATIC_PAGE_SERVER,
		IDD_REPSTATIC_PAGE_RAISEEVENT,
		IDD_REPSTATIC_PAGE_COLLISION,
		IDD_REPSTATIC_PAGE_INTEGRITY,
		IDD_REPSTATIC_PAGE_DISTRIB
	};
	UINT nIDTitle = IDS_REPSTATIC_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMReplAllDb"), 6, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

void CuTMReplAllDb::UpdateDataWhenSelChange()
{
	LPRESOURCEDATAMIN pSvrDta = GetPTreeItemData()? (LPRESOURCEDATAMIN)GetPTreeItemData()->GetDataPtr(): NULL;
	if (pSvrDta)
	{
		pSvrDta->m_bRefresh = TRUE;
	}
}

CString CuTMReplAllDb::GetRightPaneTitle()
{
	// get info about current database
	LPRESOURCEDATAMIN lps =(LPRESOURCEDATAMIN)GetPTreeItemData()->GetDataPtr();
	TCHAR szDbName[100];
	CTreeCtrl* pTreeCtrl = GetPTreeGlobData()->GetPTree();
	IPM_GetInfoName(GetPTreeGlobData()->GetDocument(), OT_DATABASE, lps, szDbName, sizeof(szDbName));

	CString csDbName = CString (_T("on Database ")) + (LPCTSTR)szDbName;
	return csDbName;
}

BOOL CuTMReplAllDb::ItemDisplaysNoPage()
{
	// get info about current database
	LPRESOURCEDATAMIN lps =(LPRESOURCEDATAMIN)GetPTreeItemData()->GetDataPtr();
	TCHAR szDbName[100];
	CTreeCtrl* pTreeCtrl = GetPTreeGlobData()->GetPTree();
	IPM_GetInfoName(GetPTreeGlobData()->GetDocument(), OT_DATABASE, lps, szDbName, sizeof(szDbName));

	// call for replicator status
	if (IPM_IsReplicatorInstalled(GetPTreeGlobData()->GetDocument(), szDbName))
		return FALSE;
	else
		return TRUE;
}


//
// CLASS: CuTMReplServer
// ************************************************************************************************
IMPLEMENT_SERIAL (CuTMReplServer, CTreeItem, 1)
CuTMReplServer::CuTMReplServer (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_REPLIC_SERVER, SUBBRANCH_KIND_NONE, FALSE, pItemData)
{
	// Specific Context menu id, replaces standard line "SetContextMenuId(IDR_POPUP_IPM);"
	SetContextMenuId(IDR_POPUP_IPM_REPLICSVR);
	InitializeImageId(IDB_TM_REPLIC_SERVER);
}

CuTMReplServer::CuTMReplServer()
{
}

UINT CuTMReplServer::GetCustomImageId()
{
	LPREPLICSERVERDATAMIN lps =(LPREPLICSERVERDATAMIN)GetPTreeItemData()->GetDataPtr();
	switch (lps->startstatus) {
		case REPSVR_UNKNOWNSTATUS:
			return IDB_TM_REPLIC_SERVER;
		case REPSVR_STARTED:
			return IDB_TM_REPLIC_SERVER_STARTED;
		case REPSVR_STOPPED:
			return IDB_TM_REPLIC_SERVER_STOPPED;
		case REPSVR_ERROR:
			return IDB_TM_REPLIC_SERVER_ERROR;
		default:
			ASSERT (FALSE);
			return IDB_TM_REPLIC_SERVER_ERROR;
	}
}


CuPageInformation* CuTMReplServer::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [4] = 
	{
		IDS_TAB_REPSERVER_STATUS,
		IDS_TAB_REPSERVER_STARTUP_SETTING,
		IDS_TAB_REPSERVER_RAISE_EVENT,
		IDS_TAB_REPSERVER_ASSIGNMENT
	};
	UINT nDlgID [4] = 
	{
		IDD_REPSERVER_PAGE_STATUS,
		IDD_REPSERVER_PAGE_STARTSETTING,
		IDD_REPSERVER_PAGE_RAISEEVENT,
		IDD_REPSERVER_PAGE_ASSIGNMENT
	};
	UINT nIDTitle = IDS_REPSERVER_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMReplServer"), 4, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

void CuTMReplServer::UpdateDataWhenSelChange()
{
	LPREPLICSERVERDATAMIN pSvrDta = GetPTreeItemData()? (LPREPLICSERVERDATAMIN)GetPTreeItemData()->GetDataPtr(): NULL;
	if (pSvrDta)
	{
		pSvrDta->m_bRefresh = TRUE;
	}
}

BOOL CuTMReplServer::IsEnabled(UINT idMenu)
{
	switch (idMenu) {
		case IDM_IPMBAR_REPLICSVR_CHANGERUNNODE:
			if (IsNoItem() || IsErrorItem())
				return FALSE;
			return TRUE;
		default:
			return CTreeItem::IsEnabled(idMenu);
	}
	return FALSE;   // security if return forgotten in switch
}


//
// CLASS: CuTMDbReplStatic
// ************************************************************************************************
IMPLEMENT_SERIAL (CuTMDbReplStatic, CTreeItem, 1)

CuTMDbReplStatic::CuTMDbReplStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_DATABASE, IDS_TM_REPLIC_REPLIC, SUBBRANCH_KIND_OBJ, FALSE, pItemData, OT_MON_REPLIC_SERVER, IDS_TM_REPLIC_SERVER_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMDbReplStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMReplServer* pBranch = new CuTMReplServer (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMDbReplStatic::CuTMDbReplStatic()
{
}

CuPageInformation* CuTMDbReplStatic::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [6] = 
	{
		IDS_TAB_REPSTATIC_ACTIVITY,
		IDS_TAB_REPSTATIC_SERVER,
		IDS_TAB_REPSTATIC_RAISE_EVENT,
		IDS_TAB_REPSTATIC_COLLISION,
		IDS_TAB_REPSTATIC_INTEGRITY,
		IDS_TAB_REPSTATIC_DISTRIB
	};
	UINT nDlgID [6] = 
	{
		IDD_REPSTATIC_PAGE_ACTIVITY,
		IDD_REPSTATIC_PAGE_SERVER,
		IDD_REPSTATIC_PAGE_RAISEEVENT,
		IDD_REPSTATIC_PAGE_COLLISION,
		IDD_REPSTATIC_PAGE_INTEGRITY,
		IDD_REPSTATIC_PAGE_DISTRIB
	};
	UINT nIDTitle = IDS_REPSTATIC_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMDbReplStatic"), 6, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

void CuTMDbReplStatic::UpdateDataWhenSelChange()
{
	LPRESOURCEDATAMIN pSvrDta = GetPTreeItemData()? (LPRESOURCEDATAMIN)GetPTreeItemData()->GetDataPtr(): NULL;
	if (pSvrDta)
	{
		pSvrDta->m_bRefresh = TRUE;
	}
}

CString CuTMDbReplStatic::GetRightPaneTitle()
{
	// get info about current database
	LPRESOURCEDATAMIN lps =(LPRESOURCEDATAMIN)GetPTreeItemData()->GetDataPtr();
	TCHAR szDbName[100];
	CTreeCtrl* pTreeCtrl = GetPTreeGlobData()->GetPTree();
	IPM_GetInfoName(GetPTreeGlobData()->GetDocument(), OT_DATABASE, lps, szDbName, sizeof(szDbName));

	CString csDbName = CString (_T("on Database ")) + (LPCTSTR)szDbName;
	return csDbName;
}

BOOL CuTMDbReplStatic::ItemDisplaysNoPage()
{
	// get info about current database
	LPRESOURCEDATAMIN lps =(LPRESOURCEDATAMIN)GetPTreeItemData()->GetDataPtr();
	CdIpmDoc* pDoc = GetPTreeGlobData()->GetDocument();
	TCHAR szDbName[100];
	CTreeCtrl* pTreeCtrl = GetPTreeGlobData()->GetPTree();
	IPM_GetInfoName(pDoc, OT_DATABASE, lps, szDbName, sizeof(szDbName));

	// call for replicator status
	if (IPM_IsReplicatorInstalled(pDoc, szDbName))
		return FALSE;
	else
		return TRUE;
}

CString CuTMDbReplStatic::GetDBName()
{
	ASSERT (HasReplicMonitor());

	// get info about current database
	LPRESOURCEDATAMIN lps =(LPRESOURCEDATAMIN)GetPTreeItemData()->GetDataPtr();
	TCHAR szDbName[100];
	CTreeCtrl* pTreeCtrl = GetPTreeGlobData()->GetPTree();
	IPM_GetInfoName(GetPTreeGlobData()->GetDocument(), OT_DATABASE, lps, szDbName, sizeof(szDbName));

	CString csDbName = szDbName;
	return csDbName;
}

