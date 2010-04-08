/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : montree.cpp : implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : Emmanuel Blattes
**    Purpose  : Classes for monitoring tree.
**
** History:
**
** 20-Feb-1997 (Emmanuel Blattes)
**    Created
** 03-Feb-2000 (noifr01)
**    (SIR 100331) manage the (new) RMCMD monitor server type
** 02-Aug-2001 (noifr01)
**    (sir 105275) manage the JDBC server type
** 15-Mar-2004 (schph01)
**    (SIR #110013) Handle the DAS Server
** 30-Apr-2004 (schph01)
**    (BUG 112243) Add function to gray/ungray the remove/shutdown button and
**    menu item.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "montree.h"  // General branches
#include "montreei.h" // Ice branches
#include "treerepl.h" // Replication branches
#include "dgipmc02.h"
#include "vtreeglb.h"
#include "ipmdoc.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



IMPLEMENT_SERIAL (CuTMServerStatic, CTreeItem, 2)
IMPLEMENT_SERIAL (CuTMServer, CTreeItem, 2)
// Added Emb 18/06 : same as server, but not expandable,
// and only one tab "detail" on right pane
IMPLEMENT_SERIAL (CuTMServerNotExpandable, CTreeItem, 2)

CuTMServerStatic::CuTMServerStatic (CTreeGlobalData* pTreeGlobalData)
    :CTreeItem (pTreeGlobalData, 0, IDS_TM_SERVER, SUBBRANCH_KIND_OBJ, IDB_TM_SERVER, NULL, OT_MON_SERVER, IDS_TM_SERVER_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMServerStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CTreeItem *pItem;

	// Emb July 3, 97: take "error" branch in account
	if (!pItemData) {
		pItem = new CuTMServer (GetPTreeGlobData(), pItemData);
		return pItem;
	}

	LPSERVERDATAMIN lps =(LPSERVERDATAMIN)pItemData->GetDataPtr();
	ASSERT (lps);

	switch (lps->servertype) 
	{
	case SVR_TYPE_STAR:
	case SVR_TYPE_RECOVERY:
	case SVR_TYPE_DBMS:
		pItem = new CuTMServer (GetPTreeGlobData(), pItemData);
		return pItem;
	case SVR_TYPE_ICE:
		pItem = new CuTMIceServer (GetPTreeGlobData(), pItemData);
		return pItem;
	case SVR_TYPE_RMCMD: /* RMCMD server branch not expandable */
	case SVR_TYPE_JDBC:  /* JDBC  server branch not expandable */
	case SVR_TYPE_DASVR:  /* DASVR  server branch not expandable */
	default:
		pItem = new CuTMServerNotExpandable (GetPTreeGlobData(), pItemData);
		return pItem;
	}

	ASSERT (FALSE);
	return 0;
}

CuTMServerStatic::CuTMServerStatic()
{
}

CuPageInformation* CuTMServerStatic::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = { IDS_TAB_STATIC_SERVER, };
	UINT nDlgID [1] = { IDD_IPMPAGE_STATIC_SERVERS, };
	UINT nIDTitle = IDS_TAB_STATIC_SERVER_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMServerStatic"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

CuTMServer::CuTMServer (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, OT_MON_SERVER, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	// FNN REQUEST Nov 6, 98: always show all items whatever server accepts to be closed/opened or not
	SetContextMenuId(IDR_POPUP_IPM_SERVER);
	InitializeImageId(IDB_TM_SERVER);
}


CDialog* CuTMServer::GetPage (CWnd* pParent)
{
	return NULL;
}

//
// We must know all information how to display 
// the property of this object: how many pages, and what they are ...
CuPageInformation* CuTMServer::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [5] = 
	{
		IDS_TAB_DETAIL, 
		IDS_TAB_SESSION, 
		IDS_TAB_LOCKLIST, 
		IDS_TAB_LOCK, 
		IDS_TAB_TRANSACTION
	};
	UINT nDlgID [5] = 
	{
		IDD_IPMDETAIL_SERVER, 
		IDD_IPMPAGE_SESSIONS, 
		IDD_IPMPAGE_LOCKLISTS, 
		IDD_IPMPAGE_LOCKS, 
		IDD_IPMPAGE_TRANSACTIONS, 
	};
	UINT nIDTitle = IDS_TAB_SERVER_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMServer"), 5, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}


BOOL CuTMServer::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();

	CuTMSessionStatic   *pBranch1 = new CuTMSessionStatic(pGlobalData, GetPTreeItemData());
	CuTMLocklistStatic  *pBranch2 = new CuTMLocklistStatic(pGlobalData, GetPTreeItemData());
	CuTMLockStatic      *pBranch3 = new CuTMLockStatic(pGlobalData, GetPTreeItemData());
	CuTMTransacStatic   *pBranch4 = new CuTMTransacStatic(pGlobalData, GetPTreeItemData());
	CuTMDbStatic        *pBranch5 = new CuTMDbStatic(pGlobalData, GetPTreeItemData());
	CuTMTblStatic       *pBranch6 = new CuTMTblStatic(pGlobalData, GetPTreeItemData());
	CuTMPgStatic        *pBranch7 = new CuTMPgStatic(pGlobalData, GetPTreeItemData());
	CuTMOthStatic       *pBranch8 = new CuTMOthStatic(pGlobalData, GetPTreeItemData());

	pBranch1->CreateTreeLine (hParentItem);
	pBranch2->CreateTreeLine (hParentItem);
	pBranch3->CreateTreeLine (hParentItem);
	pBranch4->CreateTreeLine (hParentItem);
	pBranch5->CreateTreeLine (hParentItem);
	pBranch6->CreateTreeLine (hParentItem);
	pBranch7->CreateTreeLine (hParentItem);
	pBranch8->CreateTreeLine (hParentItem);

	return TRUE;
}

CuTMServer::CuTMServer()
{
	m_pPageInfo = NULL;
}

UINT CuTMServer::GetCustomImageId()
{
	LPSERVERDATAMIN lps =(LPSERVERDATAMIN)GetPTreeItemData()->GetDataPtr();
	return GetServerImageId(lps);
}

static BOOL NEAR DoesCurrentStateFitWithMenuId(LPSERVERDATAMIN lps, UINT idMenu)
{
	ASSERT ((int)idMenu == IDM_IPMBAR_CLOSESVR || (int)idMenu == IDM_IPMBAR_OPENSVR);

	CString csCurrentState = lps->listen_state;
	if (csCurrentState == _T("OPEN")) {
		if ((int)idMenu == IDM_IPMBAR_CLOSESVR)
			return TRUE;
		else if ((int)idMenu == IDM_IPMBAR_OPENSVR)
			return FALSE;
		else {
			ASSERT (FALSE);
			return FALSE;
		}
	}
	else {
		if ((int)idMenu == IDM_IPMBAR_CLOSESVR)
			return TRUE;
		else if ((int)idMenu == IDM_IPMBAR_OPENSVR)
			return FALSE;
		else {
			ASSERT (FALSE);
			return FALSE;
		}
	}
}

BOOL CuTMServer::IsEnabled(UINT idMenu)
{
	LPSERVERDATAMIN lps = 0;
	switch (idMenu) 
	{
	case IDM_IPMBAR_SHUTDOWN:   // KillShutdown
		if (IsNoItem() || IsErrorItem())
			return FALSE;
		lps =(LPSERVERDATAMIN)GetPTreeItemData()->GetDataPtr();
		ASSERT (lps);
		if (!IsShutdownServerType(lps))
			return FALSE;
		return TRUE;

	case IDM_IPMBAR_CLOSESVR:   // Close server
	case IDM_IPMBAR_OPENSVR:    // Open server
		if (IsNoItem() || IsErrorItem())
			return FALSE;
		lps =(LPSERVERDATAMIN)GetPTreeItemData()->GetDataPtr();
		ASSERT (lps);
		if (!IsShutdownServerType(lps))
			return FALSE;
		return DoesCurrentStateFitWithMenuId(lps, idMenu);
		break;

	default:
		return CTreeItem::IsEnabled(idMenu);
	}
	return FALSE;   // security if return forgotten in switch
}

BOOL CuTMServer::KillShutdown()
{
	BOOL bRet = FALSE;
	BOOL bSoft = TRUE;
	CdIpmDoc* pDoc = GetPTreeGlobData()->GetDocument();
	CaConnectInfo& connectInfo = pDoc->GetConnectionInfo();
	CString strNode = connectInfo.GetNode();
	// get info about server
	LPSERVERDATAMIN lps =(LPSERVERDATAMIN)GetPTreeItemData()->GetDataPtr();
	TCHAR szSvrName[100];

	CTreeCtrl* pTreeCtrl = GetPTreeGlobData()->GetPTree();
	IPM_GetInfoName(GetPTreeGlobData()->GetDocument(), OT_MON_SERVER, lps, szSvrName, sizeof(szSvrName));

	// Fnn request June 2, 97:
	// always soft ---> replaced by "simple" message box
	CString confTxt;
	confTxt.Format(IDS_SHUTDOWN_SOFT, szSvrName);
	CString caption;
	caption.Format(IDS_SHUTDOWNSRV_TITLE, szSvrName, (LPCTSTR)strNode);
	int ret;
	ret = MessageBox(GetFocus(), confTxt, caption, MB_YESNO | MB_ICONQUESTION);
	if (ret == IDNO)
		return FALSE;
	bSoft = TRUE;


	// confirm
	CString confirmTxt;
	if (bSoft)
		confirmTxt.Format(IDS_SHUTDOWN_CONFIRM_SOFT, szSvrName);
	else
		confirmTxt.Format(IDS_SHUTDOWN_CONFIRM_HARD, szSvrName);
	ret = MessageBox(GetFocus(), confirmTxt, caption, MB_YESNO | MB_ICONQUESTION);
	if (ret == IDYES) 
	{
		LPSERVERDATAMIN pServer =(LPSERVERDATAMIN)GetPTreeItemData()->GetDataPtr();
		bRet = IPM_ShutDownServer (pDoc, pServer, bSoft);
	}
	return bRet;
}

BOOL CuTMServer::CloseServer()
{
	int ret;
	CString confirmTxt,caption;
	TCHAR szSvrName[100];
	BOOL bOpen = FALSE;
	CdIpmDoc* pDoc = GetPTreeGlobData()->GetDocument();
	CaConnectInfo& connectInfo = pDoc->GetConnectionInfo();
	CString strNode = connectInfo.GetNode();

	// get info about server
	LPSERVERDATAMIN lps =(LPSERVERDATAMIN)GetPTreeItemData()->GetDataPtr();
	CTreeCtrl* pTreeCtrl = GetPTreeGlobData()->GetPTree();
	IPM_GetInfoName(GetPTreeGlobData()->GetDocument(), OT_MON_SERVER, lps, szSvrName, sizeof(szSvrName));

	caption.Format(IDS_E_TITLE_CLOSE_SVR, szSvrName, (LPCTSTR)strNode);//"Close Server %s on %s"
	confirmTxt.Format(IDS_A_CLOSE_SVR, szSvrName);//"Close Server %s (Last Confirmation)?"
	ret = MessageBox(GetFocus(), confirmTxt, caption, MB_YESNO | MB_ICONQUESTION);
	if (ret == IDNO)
		return FALSE;

	BOOL bOK = IPM_OpenCloseServer(pDoc, lps, bOpen);
	if (bOK)
	{
		pDoc->UpdateDisplay();
		return TRUE;
	}
	return FALSE;
}

BOOL CuTMServer::OpenServer()
{
	int ret;
	TCHAR szSvrName[100];
	CString confirmTxt,caption;
	BOOL bOpen = TRUE;
	CdIpmDoc* pDoc = GetPTreeGlobData()->GetDocument();
	CaConnectInfo& connectInfo = pDoc->GetConnectionInfo();
	CString strNode = connectInfo.GetNode();

	// get info about server
	LPSERVERDATAMIN lps =(LPSERVERDATAMIN)GetPTreeItemData()->GetDataPtr();
	CTreeCtrl* pTreeCtrl = GetPTreeGlobData()->GetPTree();
	IPM_GetInfoName(GetPTreeGlobData()->GetDocument(), OT_MON_SERVER, lps, szSvrName, sizeof(szSvrName));


	caption.Format(IDS_TITLE_OPEN_SVR, szSvrName, (LPCTSTR)strNode);//"Open Server %s on %s"
	confirmTxt.Format(IDS_A_OPEN_SVR, szSvrName);//"Open Server %s ?"
	ret = MessageBox(GetFocus(), confirmTxt, caption, MB_YESNO | MB_ICONQUESTION);
	if (ret == IDNO)
		return FALSE;

	BOOL bOK = IPM_OpenCloseServer(pDoc, lps, bOpen);
	if (bOK)
	{
		pDoc->UpdateDisplay();
		return TRUE;
	}

	return FALSE;
}

CuTMServerNotExpandable::CuTMServerNotExpandable (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, OT_MON_SERVER, SUBBRANCH_KIND_NONE, FALSE, pItemData)
{
	// FNN REQUEST Nov 6, 98: always show all items whatever server accepts to be closed/opened or not
	SetContextMenuId(IDR_POPUP_IPM_SERVER);
	InitializeImageId(IDB_TM_SERVER);
}


CDialog* CuTMServerNotExpandable::GetPage (CWnd* pParent)
{
	return NULL;
}

//
// We must know all information how to display 
// the property of this object: how many pages, and what they are ...
CuPageInformation* CuTMServerNotExpandable::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = 
	{
		IDS_TAB_DETAIL
	};
	UINT nDlgID [1] = 
	{
		IDD_IPMDETAIL_SERVER
	};
	UINT nIDTitle = IDS_TAB_SERVER_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMServerNotExpandable"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}


CuTMServerNotExpandable::CuTMServerNotExpandable()
{
	m_pPageInfo = NULL;
}

UINT CuTMServerNotExpandable::GetCustomImageId()
{
	LPSERVERDATAMIN lps =(LPSERVERDATAMIN)GetPTreeItemData()->GetDataPtr();
	return GetServerImageId(lps);
}

BOOL CuTMServerNotExpandable::IsEnabled(UINT idMenu)
{
	LPSERVERDATAMIN lps =(LPSERVERDATAMIN)GetPTreeItemData()->GetDataPtr();

	switch (idMenu) {
		case IDM_IPMBAR_SHUTDOWN:   // KillShutdown
			if (IsNoItem() || IsErrorItem())
				return FALSE;
			ASSERT (lps);
			if (!IsShutdownServerType(lps))
				return FALSE;
			return TRUE;

		default:
			return CTreeItem::IsEnabled(idMenu);
	}
	return FALSE;   // security if return forgotten in switch
}

BOOL CuTMServerNotExpandable::KillShutdown()
{
	BOOL bRet = FALSE;
	BOOL bSoft = TRUE;

	CdIpmDoc* pDoc = GetPTreeGlobData()->GetDocument();
	CaConnectInfo& connectInfo = pDoc->GetConnectionInfo();
	CString strNode = connectInfo.GetNode();

	// get info about server
	LPSERVERDATAMIN lps =(LPSERVERDATAMIN)GetPTreeItemData()->GetDataPtr();
	TCHAR szSvrName[100];
	CTreeCtrl* pTreeCtrl = GetPTreeGlobData()->GetPTree();
	IPM_GetInfoName(GetPTreeGlobData()->GetDocument(), OT_MON_SERVER, lps, szSvrName, sizeof(szSvrName));

	// Fnn request June 2, 97:
	// always soft ---> replaced by "simple" message box
	CString confTxt;
	confTxt.Format(IDS_SHUTDOWN_SOFT, szSvrName);
	CString caption;
	caption.Format(IDS_SHUTDOWNSRV_TITLE, szSvrName, (LPCTSTR)strNode);
	int ret;
	ret = MessageBox(GetFocus(), confTxt, caption, MB_YESNO | MB_ICONQUESTION);
	if (ret == IDNO)
		return FALSE;
	bSoft = TRUE;

	// confirm
	CString confirmTxt;
	if (bSoft)
		confirmTxt.Format(IDS_SHUTDOWN_CONFIRM_SOFT, szSvrName);
	else
		confirmTxt.Format(IDS_SHUTDOWN_CONFIRM_HARD, szSvrName);
	ret = MessageBox(GetFocus(), confirmTxt, caption, MB_YESNO | MB_ICONQUESTION);
	if (ret == IDYES)
	{
		LPSERVERDATAMIN pServer =(LPSERVERDATAMIN)GetPTreeItemData()->GetDataPtr();
		bRet = IPM_ShutDownServer (pDoc, pServer, bSoft);
	}
	return bRet;
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMSessionStatic, CTreeItem, 2)
IMPLEMENT_SERIAL (CuTMSession, CTreeItem, 1)

CuTMSessionStatic::CuTMSessionStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, OT_MON_SERVER, IDS_TM_SESSION, SUBBRANCH_KIND_OBJ, IDB_TM_SESSION, pItemData, OT_MON_SESSION, IDS_TM_SESSION_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMSessionStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMSession* pBranch = new CuTMSession (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMSessionStatic::CuTMSessionStatic()
{
}

BOOL CuTMSessionStatic::FilterApplies(FilterCause because)
{
	switch (because) {
		case FILTER_INTERNAL_SESSIONS:
			return TRUE;
		default:
			return CTreeItem::FilterApplies(because);
	}
	ASSERT (FALSE);
}

CuPageInformation* CuTMSessionStatic::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = { IDS_TAB_SESSION, };
	UINT nDlgID [1] = { IDD_IPMPAGE_SESSIONS, };
	UINT nIDTitle = IDS_TAB_STATIC_SESS_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMSessionStatic"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}


CuTMSession::CuTMSession (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, OT_MON_SESSION, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);
	InitializeImageId(IDB_TM_SESSION);
}

CuTMSession::CuTMSession()
{
}

BOOL CuTMSession::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();

	CuTMSessLocklistStatic  *pBranch1 = new CuTMSessLocklistStatic(pGlobalData, GetPTreeItemData());
	CuTMSessLockStatic      *pBranch2 = new CuTMSessLockStatic(pGlobalData, GetPTreeItemData());
	CuTMSessTransacStatic   *pBranch3 = new CuTMSessTransacStatic(pGlobalData, GetPTreeItemData());
	CuTMSessDbStatic        *pBranch4 = new CuTMSessDbStatic(pGlobalData, GetPTreeItemData());

	pBranch1->CreateTreeLine (hParentItem);
	pBranch2->CreateTreeLine (hParentItem);
	pBranch3->CreateTreeLine (hParentItem);
	pBranch4->CreateTreeLine (hParentItem);

	// new as of March 21, 97
	CuTMSessTblStatic       *pBranch5 = new CuTMSessTblStatic(pGlobalData, GetPTreeItemData());
	CuTMSessPgStatic        *pBranch6 = new CuTMSessPgStatic(pGlobalData, GetPTreeItemData());
	CuTMSessOthStatic       *pBranch7 = new CuTMSessOthStatic(pGlobalData, GetPTreeItemData());
	pBranch5->CreateTreeLine (hParentItem);
	pBranch6->CreateTreeLine (hParentItem);
	pBranch7->CreateTreeLine (hParentItem);

	// New as of May 18, 98
	CuTMSessBlockingSessionStatic *pBranch8 = new CuTMSessBlockingSessionStatic(pGlobalData, GetPTreeItemData());
	pBranch8->CreateTreeLine (hParentItem);

	return TRUE;
}

//
// We must know all information how to display 
// the property of this object: how many pages, and what they are ...
CuPageInformation* CuTMSession::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [5] = 
	{
		IDS_TAB_DETAIL, 
		IDS_TAB_CLIENT,
		IDS_TAB_LOCKLIST, 
		IDS_TAB_LOCK, 
		IDS_TAB_TRANSACTION
	};
	UINT nDlgID [5] = 
	{
		IDD_IPMDETAIL_SESSION, 
		IDD_IPMPAGE_CLIENT,
		IDD_IPMPAGE_LOCKLISTS, 
		IDD_IPMPAGE_LOCKS, 
		IDD_IPMPAGE_TRANSACTIONS, 
	};
	UINT nIDTitle = IDS_TAB_SESSION_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMSession"), 5, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}


UINT CuTMSession::GetCustomImageId()
{
	LPSESSIONDATAMIN pSession =(LPSESSIONDATAMIN)GetPTreeItemData()->GetDataPtr();
	ASSERT (pSession);
	if (pSession->locklist_wait_id)
		return IDB_TM_SESSION_BLOCKED;
	else
		return IDB_TM_SESSION;
}

BOOL CuTMSession::IsEnabled(UINT idMenu)
{
	LPSESSIONDATAMIN pSession = 0;
	switch (idMenu) {
		case IDM_IPMBAR_SHUTDOWN:   // KillShutdown
		case IDM_IPMBAR_CLOSESVR:
			if (IsNoItem() || IsErrorItem())
				return FALSE;
			pSession = (LPSESSIONDATAMIN)GetPTreeItemData()->GetDataPtr();
			ASSERT (pSession);
			if (!IsShutdownServerType(&(pSession->sesssvrdata)))
				return FALSE;

			//For internal session no menu "remove /shutdown" available
			if (IsInternalSession(pSession))
				return FALSE;
			return TRUE;
		default:
			return CTreeItem::IsEnabled(idMenu);
	}
	return FALSE;   // security if return forgotten in switch
}

BOOL CuTMSession::KillShutdown()
{
	BOOL bRet = FALSE;
	CdIpmDoc* pDoc = GetPTreeGlobData()->GetDocument();
	CaConnectInfo& connectInfo = pDoc->GetConnectionInfo();
	CString strNode = connectInfo.GetNode();

	// get info about session
	LPSESSIONDATAMIN pSession =(LPSESSIONDATAMIN)GetPTreeItemData()->GetDataPtr();
	TCHAR szSessName[100];
	CTreeCtrl* pTreeCtrl = GetPTreeGlobData()->GetPTree();
	IPM_GetInfoName(GetPTreeGlobData()->GetDocument(), OT_MON_SESSION, pSession, szSessName, sizeof(szSessName));

	// confirm or cancel
	CString capt;
	capt.Format(IDS_KILLSESSION_CONFIRM_CAPT, (LPCTSTR)strNode);
	CString txt;
	txt.Format(IDS_KILLSESSION_CONFIRM_TXT, szSessName);
	int ret = MessageBox(GetFocus(), txt, capt, MB_YESNO | MB_ICONQUESTION);
	if (ret == IDYES) 
	{
		bRet = IPM_RemoveSession(pDoc, pSession);
	}
	return bRet;
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMSessLocklistStatic, CTreeItem, 1)

CuTMSessLocklistStatic::CuTMSessLocklistStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, OT_MON_SESSION, IDS_TM_SESS_LOCKLIST, SUBBRANCH_KIND_OBJ, IDB_TM_SESS_LOCKLIST, pItemData, OT_MON_LOCKLIST, IDS_TM_SESS_LOCKLIST_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMSessLocklistStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMLocklist* pBranch = new CuTMLocklist (GetPTreeGlobData(), pItemData);
	return pBranch;
}

BOOL CuTMSessLocklistStatic::FilterApplies(FilterCause because)
{
	switch (because) {
		case FILTER_SYSTEM_LOCK_LISTS:
			return TRUE;
		default:
			return CTreeItem::FilterApplies(because);
	}
	ASSERT (FALSE);
}

CuTMSessLocklistStatic::CuTMSessLocklistStatic()
{
}



/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMLocklistLockStatic, CTreeItem, 1)

CuTMLocklistLockStatic::CuTMLocklistLockStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, OT_MON_LOCKLIST, IDS_TM_SESS_LOCKLIST_LOCK, SUBBRANCH_KIND_OBJ, IDB_TM_SESS_LOCKLIST_LOCK, pItemData, OT_MON_LOCK, IDS_TM_SESS_LOCKLIST_LOCK_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMLocklistLockStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMLock* pBranch = new CuTMLock (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMLocklistLockStatic::CuTMLocklistLockStatic()
{
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMLockResourceStatic, CTreeItem, 1)
IMPLEMENT_SERIAL (CuTMLockResource, CTreeItem, 1)

CuTMLockResourceStatic::CuTMLockResourceStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, OT_MON_LOCK, IDS_TM_LOCK_RESOURCE, SUBBRANCH_KIND_OBJ, IDB_TM_LOCK_RESOURCE, pItemData, OTLL_MON_RESOURCE, IDS_TM_LOCK_RESOURCE_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMLockResourceStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMLockResource* pBranch = new CuTMLockResource (GetPTreeGlobData(), pItemData);
	return pBranch;
}

BOOL CuTMLockResourceStatic::FilterApplies(FilterCause because)
{
	switch (because) {
		case FILTER_RESOURCE_TYPE:
		case FILTER_NULL_RESOURCES:
			return TRUE;
		default:
			return CTreeItem::FilterApplies(because);
	}
	ASSERT (FALSE);
}

CuTMLockResourceStatic::CuTMLockResourceStatic()
{
}

CuTMLockResource::CuTMLockResource (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, OTLL_MON_RESOURCE, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetCompleteType(OT_TOBESOLVED);   // Base Type for subbranches
	SetContextMenuId(IDR_POPUP_IPM);
	InitializeImageId(IDB_TM_LOCK_RESOURCE);
}

CuTMLockResource::CuTMLockResource()
{
}

UINT CuTMLockResource::GetCustomImageId()
{
	LPRESOURCEDATAMIN lpr =(LPRESOURCEDATAMIN)GetPTreeItemData()->GetDataPtr();
	return GetResourceImageId(lpr);
}

BOOL CuTMLockResource::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();

	// checks on completed type
	ASSERT (IsTypeCompleted());
	int completeType = GetCompleteType();
	ASSERT (completeType == OT_MON_RES_DB    ||
	        completeType == OT_MON_RES_TABLE ||
	        completeType == OT_MON_RES_PAGE  ||
	        completeType == OT_MON_RES_OTHER);

	if (completeType == OT_MON_RES_DB) {
		CuTMResourceDbLockStatic    *pBranch1 = new CuTMResourceDbLockStatic(pGlobalData, GetPTreeItemData());
		pBranch1->CreateTreeLine (hParentItem);
	}
	else if (completeType == OT_MON_RES_TABLE) {
		CuTMResourceTblLockStatic    *pBranch1 = new CuTMResourceTblLockStatic(pGlobalData, GetPTreeItemData());
		pBranch1->CreateTreeLine (hParentItem);
	}
	else if (completeType == OT_MON_RES_PAGE) {
		CuTMResourcePgLockStatic    *pBranch1 = new CuTMResourcePgLockStatic(pGlobalData, GetPTreeItemData());
		pBranch1->CreateTreeLine (hParentItem);
	}
	else if (completeType == OT_MON_RES_OTHER) {
		CuTMResourceOthLockStatic    *pBranch1 = new CuTMResourceOthLockStatic(pGlobalData, GetPTreeItemData());
		pBranch1->CreateTreeLine (hParentItem);
	}
	else {
		ASSERT (FALSE);
		return FALSE;
	}

	return TRUE;
}


//
// Emb April 4, 97: Duplicated from CuTMLockinfoResource::GetPageInformation()
// Changed April 22 at Fnn request: only 2 tabs (detail and locks)
CuPageInformation* CuTMLockResource::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [2] = 
	{
		IDS_TAB_DETAIL, 
		IDS_TAB_LOCK,
	};
	UINT nDlgID [2] = 
	{
		IDD_IPMDETAIL_RESOURCE, 
		IDD_IPMPAGE_LOCKS,
	};
	UINT nIDTitle = IDS_TAB_RESOURCE_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMLockResource"), 2, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}


/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMLocklistBllockStatic, CTreeItem, 1)
IMPLEMENT_SERIAL (CuTMLocklistBllock, CTreeItem, 1)

CuTMLocklistBllockStatic::CuTMLocklistBllockStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, OT_MON_LOCKLIST, IDS_TM_SESS_LOCKLIST_BL_LOCK, SUBBRANCH_KIND_OBJ, IDB_TM_SESS_LOCKLIST_BL_LOCK, pItemData, OT_MON_LOCKLIST_BL_LOCK, IDS_TM_SESS_LOCKLIST_BL_LOCK_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMLocklistBllockStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMLocklistBllock* pBranch = new CuTMLocklistBllock (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMLocklistBllockStatic::CuTMLocklistBllockStatic()
{
}

CuTMLocklistBllock::CuTMLocklistBllock (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, OT_MON_LOCKLIST_BL_LOCK, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetCompleteType(OT_MON_LOCK);   // Base Type for subbranches
	SetContextMenuId(IDR_POPUP_IPM);
	InitializeImageId(IDB_TM_SESS_LOCKLIST_BL_LOCK);
}

CuTMLocklistBllock::CuTMLocklistBllock()
{
}

UINT CuTMLocklistBllock::GetCustomImageId()
{
	// masqued as of 13/05 to keep the "lock" icon
	return GetImageId();
}

BOOL CuTMLocklistBllock::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();

	// checks on completed type
	ASSERT (IsTypeCompleted());
	ASSERT (GetCompleteType() == OT_MON_LOCK);

	CuTMLockResourceStatic    *pBranch1 = new CuTMLockResourceStatic(pGlobalData, GetPTreeItemData());
	CuTMLockLocklistStatic    *pBranch2 = new CuTMLockLocklistStatic(pGlobalData, GetPTreeItemData());
	CuTMLockSessionStatic     *pBranch3 = new CuTMLockSessionStatic(pGlobalData, GetPTreeItemData());
	CuTMLockServerStatic      *pBranch4 = new CuTMLockServerStatic(pGlobalData, GetPTreeItemData());

	pBranch1->CreateTreeLine (hParentItem);
	pBranch2->CreateTreeLine (hParentItem);
	pBranch3->CreateTreeLine (hParentItem);
	pBranch4->CreateTreeLine (hParentItem);

	return TRUE;
}

//
// We must know all information how to display 
// the property of this object: how many pages, and what they are ...
CuPageInformation* CuTMLocklistBllock::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = {IDS_TAB_DETAIL};
	UINT nDlgID [1] = {IDD_IPMDETAIL_LOCK};
	UINT nIDTitle = IDS_TAB_LOCK_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMLocklistBllock"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMSessLockStatic, CTreeItem, 1)

CuTMSessLockStatic::CuTMSessLockStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, OT_MON_SESSION, IDS_TM_SESS_LOCK, SUBBRANCH_KIND_OBJ, IDB_TM_SESS_LOCK, pItemData, OT_MON_LOCK, IDS_TM_SESS_LOCK_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMSessLockStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMLock* pBranch = new CuTMLock (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMSessLockStatic::CuTMSessLockStatic()
{
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMSessTransacStatic, CTreeItem, 1)

CuTMSessTransacStatic::CuTMSessTransacStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, OT_MON_SESSION, IDS_TM_SESS_TRANSAC, SUBBRANCH_KIND_OBJ, IDB_TM_SESS_TRANSAC, pItemData, OT_MON_TRANSACTION, IDS_TM_SESS_TRANSAC_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMSessTransacStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMTransac* pBranch = new CuTMTransac (GetPTreeGlobData(), pItemData);
	return pBranch;
}

BOOL CuTMSessTransacStatic::FilterApplies(FilterCause because)
{
	switch (because) {
		case FILTER_INACTIVE_TRANSACTIONS:
			return TRUE;
		default:
			return CTreeItem::FilterApplies(because);
	}
	ASSERT (FALSE);
}


CuTMSessTransacStatic::CuTMSessTransacStatic()
{
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMSessDbStatic, CTreeItem, 1)
IMPLEMENT_SERIAL (CuTMSessDb, CTreeItem, 1)

CuTMSessDbStatic::CuTMSessDbStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, OT_MON_SESSION, IDS_TM_SESS_DB, SUBBRANCH_KIND_OBJ, IDB_TM_SESS_DB, pItemData, OT_MON_RES_DB, IDS_TM_SESS_DB_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMSessDbStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMSessDb* pBranch = new CuTMSessDb (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMSessDbStatic::CuTMSessDbStatic()
{
}

CuTMSessDb::CuTMSessDb (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, OT_MON_RES_DB, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetCompleteType(OT_DATABASE);
	SetContextMenuId(IDR_POPUP_IPM);
	InitializeImageId(IDB_TM_SESS_DB);
}

CuTMSessDb::CuTMSessDb()
{
}

UINT CuTMSessDb::GetCustomImageId()
{
	LPRESOURCEDATAMIN lpr =(LPRESOURCEDATAMIN)GetPTreeItemData()->GetDataPtr();
	return GetDbTypeImageId(lpr);
}

BOOL CuTMSessDb::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();
	// checks on completed type
	ASSERT (IsTypeCompleted());
	ASSERT (GetCompleteType() == OT_DATABASE);

	// Masqued April 14, 97 for fnn : CuTMDbAllTblStatic      *pBranch1 = new CuTMDbAllTblStatic      (pGlobalData, GetPTreeItemData());
	CuTMDbLocklistStatic    *pBranch2 = new CuTMDbLocklistStatic    (pGlobalData, GetPTreeItemData());
	CuTMDbLockStatic        *pBranch3 = new CuTMDbLockStatic        (pGlobalData, GetPTreeItemData());
	CuTMDbLockedTblStatic   *pBranch4 = new CuTMDbLockedTblStatic   (pGlobalData, GetPTreeItemData());
	CuTMDbLockedPgStatic    *pBranch4b= new CuTMDbLockedPgStatic    (pGlobalData, GetPTreeItemData());
	CuTMDbLockedOtherStatic *pBranch5 = new CuTMDbLockedOtherStatic (pGlobalData, GetPTreeItemData());
	CuTMDbServerStatic      *pBranch6 = new CuTMDbServerStatic      (pGlobalData, GetPTreeItemData());
	CuTMDbTransacStatic     *pBranch7 = new CuTMDbTransacStatic     (pGlobalData, GetPTreeItemData());
	// masqued April 15 for fnn: CuTMDbLockOnDbStatic    *pBranch8 = new CuTMDbLockOnDbStatic    (pGlobalData, GetPTreeItemData());
	CuTMDbSessionStatic     *pBranch9 = new CuTMDbSessionStatic     (pGlobalData, GetPTreeItemData());
	CuTMDbReplStatic        *pBranch10 = new CuTMDbReplStatic        (pGlobalData, GetPTreeItemData());

	// Masqued April 14, 97 for fnn : pBranch1->CreateTreeLine (hParentItem);
	pBranch2->CreateTreeLine (hParentItem);
	pBranch3->CreateTreeLine (hParentItem);
	pBranch4->CreateTreeLine (hParentItem);
	pBranch4b->CreateTreeLine (hParentItem);
	pBranch5->CreateTreeLine (hParentItem);
	pBranch6->CreateTreeLine (hParentItem);
	pBranch7->CreateTreeLine (hParentItem);
	// masqued April 15 for fnn: pBranch8->CreateTreeLine (hParentItem);
	pBranch9->CreateTreeLine (hParentItem);
	pBranch10->CreateTreeLine (hParentItem);


	return TRUE;
}


CuPageInformation* CuTMSessDb::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [4] = 
	{
		IDS_TAB_SESSION,
		IDS_TAB_LOCKLIST,
		IDS_TAB_LOCK,
		IDS_TAB_TRANSACTION,
	};
	UINT nDlgID [4] = 
	{
		IDD_IPMPAGE_SESSIONS,
		IDD_IPMPAGE_LOCKLISTS,
		IDD_IPMPAGE_LOCKS,
		IDD_IPMPAGE_TRANSACTIONS,
	};
	UINT nIDTitle = IDS_TAB_DATABASE_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMSessDb"), 4, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}


/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMLocklistStatic, CTreeItem, 2)
IMPLEMENT_SERIAL (CuTMLocklist, CTreeItem, 1)

CuTMLocklistStatic::CuTMLocklistStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, OT_MON_SERVER, IDS_TM_LOCKLIST, SUBBRANCH_KIND_OBJ, IDB_TM_LOCKLIST, pItemData, OT_MON_LOCKLIST, IDS_TM_LOCKLIST_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMLocklistStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMLocklist* pBranch = new CuTMLocklist (GetPTreeGlobData(), pItemData);
	return pBranch;
}

BOOL CuTMLocklistStatic::FilterApplies(FilterCause because)
{
	switch (because) {
		case FILTER_SYSTEM_LOCK_LISTS:
			return TRUE;
		default:
			return CTreeItem::FilterApplies(because);
	}
	ASSERT (FALSE);
}

CuTMLocklistStatic::CuTMLocklistStatic()
{
}

CuPageInformation* CuTMLocklistStatic::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = { IDS_TAB_LOCKLIST, };
	UINT nDlgID [1] = { IDD_IPMPAGE_LOCKLISTS, };
	UINT nIDTitle = IDS_TAB_STATIC_LOCKLIST_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMLocklistStatic"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

CuTMLocklist::CuTMLocklist (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, OT_MON_LOCKLIST, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);
	InitializeImageId(IDB_TM_LOCKLIST);
}

CuTMLocklist::CuTMLocklist()
{
}

UINT CuTMLocklist::GetCustomImageId()
{
	LPLOCKLISTDATAMIN lpl =(LPLOCKLISTDATAMIN)GetPTreeItemData()->GetDataPtr();
	return GetLocklistImageId(lpl);
}

BOOL CuTMLocklist::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();

	CuTMLocklistLockStatic    *pBranch1 = new CuTMLocklistLockStatic(pGlobalData, GetPTreeItemData());
	CuTMLocklistBllockStatic  *pBranch2 = new CuTMLocklistBllockStatic(pGlobalData, GetPTreeItemData());
	// Request Fnn 27/03 : always session for a locklist
	CuTMLocklistSessionStatic *pBranch3 = new CuTMLocklistSessionStatic(pGlobalData, GetPTreeItemData());
	// New as of May 14, 98: blocking session
	CuTMBlockingSessionStatic   *pBranchB = new CuTMBlockingSessionStatic(pGlobalData, GetPTreeItemData());

	pBranch1->CreateTreeLine (hParentItem);
	pBranch2->CreateTreeLine (hParentItem);
	pBranch3->CreateTreeLine (hParentItem);
	pBranchB->CreateTreeLine (hParentItem);

	return TRUE;
}

//
// We must know all information how to display 
// the property of this object: how many pages, and what they are ...
CuPageInformation* CuTMLocklist::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [2] = 
	{
		IDS_TAB_DETAIL, 
		IDS_TAB_LOCK
	};
	UINT nDlgID [2] = 
	{
		IDD_IPMDETAIL_LOCKLIST, 
		IDD_IPMPAGE_LOCKS
	};
	UINT nIDTitle = IDS_TAB_LOCKLIST_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMLocklist"), 2, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}


/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMLockStatic, CTreeItem, 2)
IMPLEMENT_SERIAL (CuTMLock, CTreeItem, 1)

CuTMLockStatic::CuTMLockStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, OT_MON_SERVER, IDS_TM_LOCK, SUBBRANCH_KIND_OBJ, IDB_TM_LOCK, pItemData, OT_MON_LOCK, IDS_TM_LOCK_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMLockStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMLock* pBranch = new CuTMLock (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMLockStatic::CuTMLockStatic()
{
}

CuPageInformation* CuTMLockStatic::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = { IDS_TAB_LOCK, };
	UINT nDlgID [1] = { IDD_IPMPAGE_LOCKS, };
	UINT nIDTitle = IDS_TAB_STATIC_LOCK_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMLockStatic"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

CuTMLock::CuTMLock (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, OT_MON_LOCK, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);
	InitializeImageId(IDB_TM_LOCK);
}

CuTMLock::CuTMLock()
{
}

UINT CuTMLock::GetCustomImageId()
{
	// masqued as of 13/05 to keep the "lock" icon
	return GetImageId();
}

BOOL CuTMLock::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();
	// Request Fnn 27/03 : always 4 branches for a lock
	CuTMLockResourceStatic    *pBranch1 = new CuTMLockResourceStatic(pGlobalData, GetPTreeItemData());
	CuTMLockLocklistStatic    *pBranch2 = new CuTMLockLocklistStatic(pGlobalData, GetPTreeItemData());
	CuTMLockSessionStatic     *pBranch3 = new CuTMLockSessionStatic(pGlobalData, GetPTreeItemData());
	CuTMLockServerStatic      *pBranch4 = new CuTMLockServerStatic(pGlobalData, GetPTreeItemData());

	pBranch1->CreateTreeLine (hParentItem);
	pBranch2->CreateTreeLine (hParentItem);
	pBranch3->CreateTreeLine (hParentItem);
	pBranch4->CreateTreeLine (hParentItem);

	return TRUE;
}

//
// We must know all information how to display 
// the property of this object: how many pages, and what they are ...
CuPageInformation* CuTMLock::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = {IDS_TAB_DETAIL};
	UINT nDlgID [1] = {IDD_IPMDETAIL_LOCK};
	UINT nIDTitle = IDS_TAB_LOCK_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMLock"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMTransacStatic, CTreeItem, 2)
IMPLEMENT_SERIAL (CuTMTransac, CTreeItem, 1)

CuTMTransacStatic::CuTMTransacStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, OT_MON_SERVER, IDS_TM_TRANSAC, SUBBRANCH_KIND_OBJ, IDB_TM_TRANSAC, pItemData, OT_MON_TRANSACTION, IDS_TM_TRANSAC_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMTransacStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMTransac* pBranch = new CuTMTransac (GetPTreeGlobData(), pItemData);
	return pBranch;
}

BOOL CuTMTransacStatic::FilterApplies(FilterCause because)
{
	switch (because) {
		case FILTER_INACTIVE_TRANSACTIONS:
			return TRUE;
		default:
			return CTreeItem::FilterApplies(because);
	}
	ASSERT (FALSE);
}

CuTMTransacStatic::CuTMTransacStatic()
{
}

CuPageInformation* CuTMTransacStatic::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = { IDS_TAB_TRANSACTION, };
	UINT nDlgID [1] = { IDD_IPMPAGE_TRANSACTIONS, };
	UINT nIDTitle = IDS_TAB_STATIC_TRANSAC_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMTransacStatic"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

CuTMTransac::CuTMTransac (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, OT_MON_TRANSACTION, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);
	InitializeImageId(IDB_TM_TRANSAC);
}

CuTMTransac::CuTMTransac()
{
}

BOOL CuTMTransac::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();

	// April 1, 97: always 5 branches for transaction
	CuTMTransacLocklistStatic    *pBranch1 = new CuTMTransacLocklistStatic(pGlobalData, GetPTreeItemData());
	CuTMTransacLockStatic        *pBranch2 = new CuTMTransacLockStatic(pGlobalData, GetPTreeItemData());
	CuTMTransacResourceStatic    *pBranch3 = new CuTMTransacResourceStatic(pGlobalData, GetPTreeItemData());
	CuTMTransacSessionStatic     *pBranch4 = new CuTMTransacSessionStatic(pGlobalData, GetPTreeItemData());
	CuTMTransacServerStatic      *pBranch5 = new CuTMTransacServerStatic(pGlobalData, GetPTreeItemData());

	pBranch1->CreateTreeLine (hParentItem);
	pBranch2->CreateTreeLine (hParentItem);
	pBranch3->CreateTreeLine (hParentItem);
	pBranch4->CreateTreeLine (hParentItem);
	pBranch5->CreateTreeLine (hParentItem);

	return TRUE;
}

//
// We must know all information how to display 
// the property of this object: how many pages, and what they are ...
CuPageInformation* CuTMTransac::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [3] = 
	{
		IDS_TAB_DETAIL, 
		IDS_TAB_LOCKLIST, 
		IDS_TAB_LOCK
	};
	UINT nDlgID [3] = 
	{
		IDD_IPMDETAIL_TRANSACTION, 
		IDD_IPMPAGE_LOCKLISTS, 
		IDD_IPMPAGE_LOCKS
	};
	UINT nIDTitle = IDS_TAB_TRANSACTION_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMTransac"), 3, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}
/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMDbStatic, CTreeItem, 1)
IMPLEMENT_SERIAL (CuTMDb, CTreeItem, 1)

CuTMDbStatic::CuTMDbStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, OT_MON_SERVER, IDS_TM_DB, SUBBRANCH_KIND_OBJ, IDB_TM_DB, pItemData, OT_MON_RES_DB, IDS_TM_DB_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMDbStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	 CuTMDb* pBranch = new CuTMDb (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMDbStatic::CuTMDbStatic()
{
}

CuTMDb::CuTMDb (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, OT_MON_RES_DB, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetCompleteType(OT_DATABASE);
	SetContextMenuId(IDR_POPUP_IPM);
	InitializeImageId(IDB_TM_DB);
}

CuTMDb::CuTMDb()
{
}

UINT CuTMDb::GetCustomImageId()
{
	// old code : return IDB_TM_LOCK_TYPE_DB;
	LPRESOURCEDATAMIN lpr =(LPRESOURCEDATAMIN)GetPTreeItemData()->GetDataPtr();
	return GetDbTypeImageId(lpr);
}

BOOL CuTMDb::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();

	// checks on completed type
	ASSERT (IsTypeCompleted());
	ASSERT (GetCompleteType() == OT_DATABASE);

	// Masqued April 14, 97 for fnn : CuTMDbAllTblStatic      *pBranch1 = new CuTMDbAllTblStatic      (pGlobalData, GetPTreeItemData());
	CuTMDbLocklistStatic    *pBranch2 = new CuTMDbLocklistStatic    (pGlobalData, GetPTreeItemData());
	CuTMDbLockStatic        *pBranch3 = new CuTMDbLockStatic        (pGlobalData, GetPTreeItemData());
	CuTMDbLockedTblStatic   *pBranch4 = new CuTMDbLockedTblStatic   (pGlobalData, GetPTreeItemData());
	CuTMDbLockedPgStatic    *pBranch4b= new CuTMDbLockedPgStatic    (pGlobalData, GetPTreeItemData());
	CuTMDbLockedOtherStatic *pBranch5 = new CuTMDbLockedOtherStatic (pGlobalData, GetPTreeItemData());
	CuTMDbServerStatic      *pBranch6 = new CuTMDbServerStatic      (pGlobalData, GetPTreeItemData());
	CuTMDbTransacStatic     *pBranch7 = new CuTMDbTransacStatic     (pGlobalData, GetPTreeItemData());
	// masqued April 15 for fnn: CuTMDbLockOnDbStatic    *pBranch8 = new CuTMDbLockOnDbStatic    (pGlobalData, GetPTreeItemData());
	CuTMDbSessionStatic     *pBranch9 = new CuTMDbSessionStatic     (pGlobalData, GetPTreeItemData());
	CuTMDbReplStatic        *pBranch10 = new CuTMDbReplStatic        (pGlobalData, GetPTreeItemData());

	// Masqued April 14, 97 for fnn : pBranch1->CreateTreeLine (hParentItem);
	pBranch2->CreateTreeLine (hParentItem);
	pBranch3->CreateTreeLine (hParentItem);
	pBranch4->CreateTreeLine (hParentItem);
	pBranch4b->CreateTreeLine (hParentItem);
	pBranch5->CreateTreeLine (hParentItem);
	pBranch6->CreateTreeLine (hParentItem);
	pBranch7->CreateTreeLine (hParentItem);
	// masqued April 15 for fnn: pBranch8->CreateTreeLine (hParentItem);
	pBranch9->CreateTreeLine (hParentItem);
	pBranch10->CreateTreeLine (hParentItem);

	return TRUE;
}

//
// We must know all information how to display 
// the property of this object: how many pages, and what they are ...
CuPageInformation* CuTMDb::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [4] = 
	{
		IDS_TAB_SESSION,
		IDS_TAB_LOCKLIST,
		IDS_TAB_LOCK,
		IDS_TAB_TRANSACTION,
	};
	UINT nDlgID [4] = 
	{
		IDD_IPMPAGE_SESSIONS,
		IDD_IPMPAGE_LOCKLISTS,
		IDD_IPMPAGE_LOCKS,
		IDD_IPMPAGE_TRANSACTIONS,
	};
	UINT nIDTitle = IDS_TAB_DATABASE_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMDb"), 4, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

/* ------------------------------------------MONTREE2------------------------------------*/
IMPLEMENT_SERIAL (CuTMLoginfoStatic, CTreeItem, 1)

CuTMLoginfoStatic::CuTMLoginfoStatic (CTreeGlobalData* pTreeGlobalData)
    :CTreeItem (pTreeGlobalData, 0, IDS_TM_LOGINFO, SUBBRANCH_KIND_STATIC, IDB_TM_LOGINFO, 0)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CuTMLoginfoStatic::CuTMLoginfoStatic()
{
}

BOOL CuTMLoginfoStatic::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();

	CuTMLogProcessStatic  *pBranch1 = new CuTMLogProcessStatic(pGlobalData, GetPTreeItemData());
	CuTMLogDbStatic       *pBranch2 = new CuTMLogDbStatic(pGlobalData, GetPTreeItemData());
	CuTMLogTransacStatic  *pBranch3 = new CuTMLogTransacStatic(pGlobalData, GetPTreeItemData());

	pBranch1->CreateTreeLine (hParentItem);
	pBranch2->CreateTreeLine (hParentItem);
	pBranch3->CreateTreeLine (hParentItem);

	return TRUE;
}

//
// We must know all information how to display 
// the property of this object: how many pages, and what they are ...
CuPageInformation* CuTMLoginfoStatic::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [6] = 
	{
		IDS_TAB_SUMMARY, 
		IDS_TAB_HEADER,
		IDS_TAB_LOGFILE,
		IDS_TAB_PROCESS,
		IDS_TAB_ACTIVEDB,
		IDS_TAB_TRANSACTION
	};
	UINT nDlgID [6] = 
	{
		IDD_IPMDETAIL_LOGINFO, 
		IDD_IPMPAGE_HEADER,
		IDD_IPMPAGE_LOGFILE,
		IDD_IPMPAGE_PROCESSES,
		IDD_IPMPAGE_ACTIVEDB,
		IDD_IPMPAGE_TRANSACTIONS
	};
	UINT nIDTitle = IDS_TAB_LOGINFO_TITLE;

	m_pPageInfo = new CuPageInformation (_T("CuTMLoginfoStatic"), 6, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMLogProcessStatic, CTreeItem, 2)
IMPLEMENT_SERIAL (CuTMLogProcess, CTreeItem, 1)

CuTMLogProcessStatic::CuTMLogProcessStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, 0, IDS_TM_LOGPROCESS, SUBBRANCH_KIND_OBJ, IDB_TM_LOGPROCESS, pItemData, OT_MON_LOGPROCESS, IDS_TM_LOGPROCESS_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMLogProcessStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMLogProcess* pBranch = new CuTMLogProcess (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMLogProcessStatic::CuTMLogProcessStatic()
{
}

CuPageInformation* CuTMLogProcessStatic::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = { IDS_TAB_PROCESS, };
	UINT nDlgID [1] = { IDD_IPMPAGE_PROCESSES, };
	UINT nIDTitle = IDS_TAB_STATIC_PROCESS_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMLogProcessStatic"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

CuTMLogProcess::CuTMLogProcess (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, OT_MON_LOGPROCESS, SUBBRANCH_KIND_NONE, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);
	InitializeImageId(IDB_TM_LOGPROCESS);
}

CuTMLogProcess::CuTMLogProcess()
{
}

//
// We must know all information how to display 
// the property of this object: how many pages, and what they are ...
CuPageInformation* CuTMLogProcess::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = {IDS_TAB_DETAIL};
	UINT nDlgID [1] = {IDD_IPMDETAIL_PROCESS};
	UINT nIDTitle = IDS_TAB_PROCESS_TITLE;

	m_pPageInfo = new CuPageInformation (_T("CuTMLogProcess"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}
/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMLogDbStatic, CTreeItem, 2)
IMPLEMENT_SERIAL (CuTMLogDb, CTreeItem, 1)

CuTMLogDbStatic::CuTMLogDbStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, 0, IDS_TM_LOGDB, SUBBRANCH_KIND_OBJ, IDB_TM_LOGDB, pItemData, OT_MON_LOGDATABASE, IDS_TM_LOGDB_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMLogDbStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMLogDb* pBranch = new CuTMLogDb (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMLogDbStatic::CuTMLogDbStatic()
{
}

CuPageInformation* CuTMLogDbStatic::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = { IDS_TAB_ACTIVEDB, };
	UINT nDlgID [1] = { IDD_IPMPAGE_ACTIVEDB, };
	UINT nIDTitle = IDS_TAB_STATIC_ACTIVEDB_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMLogDbStatic"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

CuTMLogDb::CuTMLogDb (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, OT_MON_LOGDATABASE, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);
	InitializeImageId(IDB_TM_LOGDB);
}

CuTMLogDb::CuTMLogDb()
{
}

UINT CuTMLogDb::GetCustomImageId()
{
	// old code : return IDB_TM_LOCK_TYPE_DB;
	LPLOGDBDATAMIN lpldb =(LPLOGDBDATAMIN)GetPTreeItemData()->GetDataPtr();
	return GetDbTypeImageId(lpldb);
}

BOOL CuTMLogDb::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();

	// Masqued April 14, 97 for fnn : CuTMLogDbAllTblStatic      *pBranch1 = new CuTMLogDbAllTblStatic      (pGlobalData, GetPTreeItemData());
	CuTMLogDbLocklistStatic    *pBranch2 = new CuTMLogDbLocklistStatic    (pGlobalData, GetPTreeItemData());
	CuTMLogDbLockStatic        *pBranch3 = new CuTMLogDbLockStatic        (pGlobalData, GetPTreeItemData());
	CuTMLogDbLockedTblStatic   *pBranch4 = new CuTMLogDbLockedTblStatic   (pGlobalData, GetPTreeItemData());
	CuTMLogDbLockedPgStatic    *pBranch4b= new CuTMLogDbLockedPgStatic    (pGlobalData, GetPTreeItemData());
	CuTMLogDbLockedOtherStatic *pBranch5 = new CuTMLogDbLockedOtherStatic (pGlobalData, GetPTreeItemData());
	CuTMLogDbServerStatic      *pBranch6 = new CuTMLogDbServerStatic      (pGlobalData, GetPTreeItemData());
	CuTMLogDbTransacStatic     *pBranch7 = new CuTMLogDbTransacStatic     (pGlobalData, GetPTreeItemData());
	// Masqued April 15, 97 for fnn : CuTMLogDbLockOnDbStatic    *pBranch8 = new CuTMLogDbLockOnDbStatic    (pGlobalData, GetPTreeItemData());
	CuTMLogDbSessionStatic     *pBranch9 = new CuTMLogDbSessionStatic     (pGlobalData, GetPTreeItemData());

	// Masqued April 14, 97 for fnn : pBranch1->CreateTreeLine (hParentItem);
	pBranch2->CreateTreeLine (hParentItem);
	pBranch3->CreateTreeLine (hParentItem);
	pBranch4->CreateTreeLine (hParentItem);
	pBranch4b->CreateTreeLine (hParentItem);
	pBranch5->CreateTreeLine (hParentItem);
	pBranch6->CreateTreeLine (hParentItem);
	pBranch7->CreateTreeLine (hParentItem);
	// Masqued April 15, 97 for fnn : pBranch8->CreateTreeLine (hParentItem);
	pBranch9->CreateTreeLine (hParentItem);

	return TRUE;
}



CuPageInformation* CuTMLogDb::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [4] = 
	{
		IDS_TAB_SESSION,
		IDS_TAB_LOCKLIST,
		IDS_TAB_LOCK,
		IDS_TAB_TRANSACTION,
	};
	UINT nDlgID [4] = 
	{
		IDD_IPMPAGE_SESSIONS,
		IDD_IPMPAGE_LOCKLISTS,
		IDD_IPMPAGE_LOCKS,
		IDD_IPMPAGE_TRANSACTIONS,
	};
	UINT nIDTitle = IDS_TAB_DATABASE_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMLogDb"), 4, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMLogTransacStatic, CTreeItem, 2)

CuTMLogTransacStatic::CuTMLogTransacStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
    :CTreeItem (pTreeGlobalData, 0, IDS_TM_LOGTRANSAC, SUBBRANCH_KIND_OBJ, IDB_TM_LOGTRANSAC, pItemData, OT_MON_TRANSACTION, IDS_TM_LOGTRANSAC_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMLogTransacStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMTransac* pBranch = new CuTMTransac (GetPTreeGlobData(), pItemData);
	return pBranch;
}

BOOL CuTMLogTransacStatic::FilterApplies(FilterCause because)
{
	switch (because) {
		case FILTER_INACTIVE_TRANSACTIONS:
			return TRUE;
		default:
			return CTreeItem::FilterApplies(because);
	}
	ASSERT (FALSE);
}

CuTMLogTransacStatic::CuTMLogTransacStatic()
{
}

CuPageInformation* CuTMLogTransacStatic::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = { IDS_TAB_TRANSACTION, };
	UINT nDlgID [1] = { IDD_IPMPAGE_TRANSACTIONS, };
	UINT nIDTitle = IDS_TAB_STATIC_LOGTRANSAC_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMLogTransacStatic"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMAllDbStatic, CTreeItem, 2)
IMPLEMENT_SERIAL (CuTMAllDb, CTreeItem, 1)

CuTMAllDbStatic::CuTMAllDbStatic (CTreeGlobalData* pTreeGlobalData)
	:CTreeItem (pTreeGlobalData, 0, IDS_TM_ALLDB, SUBBRANCH_KIND_OBJ, IDB_TM_ALLDB, NULL, OT_DATABASE, IDS_TM_ALLDB_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMAllDbStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMAllDb* pBranch = new CuTMAllDb (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMAllDbStatic::CuTMAllDbStatic()
{
}

CuPageInformation* CuTMAllDbStatic::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = { IDS_TAB_STATIC_DATABASE, };
	UINT nDlgID [1] = { IDD_IPMPAGE_STATIC_DATABASES, };
	UINT nIDTitle = IDS_TAB_STATIC_DATABASE_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMAllDbStatic"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

CuTMAllDb::CuTMAllDb (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_DATABASE, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);
	InitializeImageId(IDB_TM_ALLDB);
}

CuTMAllDb::CuTMAllDb()
{
}

UINT CuTMAllDb::GetCustomImageId()
{
	// old code : return IDB_TM_LOCK_TYPE_DB;
	LPRESOURCEDATAMIN lpr =(LPRESOURCEDATAMIN)GetPTreeItemData()->GetDataPtr();
	return GetDbTypeImageId(lpr);
}

BOOL CuTMAllDb::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();

	// Masqued April 14, 97 for fnn : CuTMDbAllTblStatic      *pBranch1 = new CuTMDbAllTblStatic      (pGlobalData, GetPTreeItemData());
	CuTMDbLocklistStatic    *pBranch2 = new CuTMDbLocklistStatic    (pGlobalData, GetPTreeItemData());
	CuTMDbLockStatic        *pBranch3 = new CuTMDbLockStatic        (pGlobalData, GetPTreeItemData());
	CuTMDbLockedTblStatic   *pBranch4 = new CuTMDbLockedTblStatic   (pGlobalData, GetPTreeItemData());
	CuTMDbLockedPgStatic    *pBranch4b= new CuTMDbLockedPgStatic    (pGlobalData, GetPTreeItemData());
	CuTMDbLockedOtherStatic *pBranch5 = new CuTMDbLockedOtherStatic (pGlobalData, GetPTreeItemData());
	CuTMDbServerStatic      *pBranch6 = new CuTMDbServerStatic      (pGlobalData, GetPTreeItemData());
	CuTMDbTransacStatic     *pBranch7 = new CuTMDbTransacStatic     (pGlobalData, GetPTreeItemData());
	// masqued April 15 for fnn: CuTMDbLockOnDbStatic    *pBranch8 = new CuTMDbLockOnDbStatic    (pGlobalData, GetPTreeItemData());
	CuTMDbSessionStatic     *pBranch9 = new CuTMDbSessionStatic     (pGlobalData, GetPTreeItemData());
	CuTMDbReplStatic        *pBranch10 = new CuTMDbReplStatic        (pGlobalData, GetPTreeItemData());

	// Masqued April 14, 97 for fnn : pBranch1->CreateTreeLine (hParentItem);
	pBranch2->CreateTreeLine (hParentItem);
	pBranch3->CreateTreeLine (hParentItem);
	pBranch4->CreateTreeLine (hParentItem);
	pBranch4b->CreateTreeLine (hParentItem);
	pBranch5->CreateTreeLine (hParentItem);
	pBranch6->CreateTreeLine (hParentItem);
	pBranch7->CreateTreeLine (hParentItem);
	// masqued April 15 for fnn: pBranch8->CreateTreeLine (hParentItem);
	pBranch9->CreateTreeLine (hParentItem);
	pBranch10->CreateTreeLine (hParentItem);

	return TRUE;
}



CuPageInformation* CuTMAllDb::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [4] = 
	{
		IDS_TAB_SESSION,
		IDS_TAB_LOCKLIST,
		IDS_TAB_LOCK,
		IDS_TAB_TRANSACTION,
	};
	UINT nDlgID [4] = 
	{
		IDD_IPMPAGE_SESSIONS,
		IDD_IPMPAGE_LOCKLISTS,
		IDD_IPMPAGE_LOCKS,
		IDD_IPMPAGE_TRANSACTIONS,
	};
	UINT nIDTitle = IDS_TAB_DATABASE_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMAllDb"), 4, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}


IMPLEMENT_SERIAL (CuTMDbAllTbl, CTreeItem, 1)
CuTMDbAllTbl::CuTMDbAllTbl (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_TABLE, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetCompleteType(OT_MON_RES_TABLE);   // Base Type for subbranches
	SetContextMenuId(IDR_POPUP_IPM);;
	InitializeImageId(IDB_TM_DB_ALLTBL);
}

CuTMDbAllTbl::CuTMDbAllTbl()
{
}

BOOL CuTMDbAllTbl::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();
	// checks on completed type
	ASSERT (IsTypeCompleted());
	ASSERT (GetCompleteType() == OT_MON_RES_TABLE);

	CuTMResourceTblLockStatic *pBranch1 = new CuTMResourceTblLockStatic(pGlobalData, GetPTreeItemData());
	pBranch1->CreateTreeLine (hParentItem);
	return TRUE;
}


/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMDbLocklistStatic, CTreeItem, 1)

CuTMDbLocklistStatic::CuTMDbLocklistStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_DATABASE, IDS_TM_DB_LOCKLIST, SUBBRANCH_KIND_OBJ, IDB_TM_DB_LOCKLIST, pItemData, OT_MON_LOCKLIST, IDS_TM_DB_LOCKLIST_NONE)
{
	 SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMDbLocklistStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMLocklist* pBranch = new CuTMLocklist (GetPTreeGlobData(), pItemData);
	return pBranch;
}

BOOL CuTMDbLocklistStatic::FilterApplies(FilterCause because)
{
	switch (because) {
		case FILTER_SYSTEM_LOCK_LISTS:
		return TRUE;
		default:
		return CTreeItem::FilterApplies(because);
	}
	ASSERT (FALSE);
}

CuTMDbLocklistStatic::CuTMDbLocklistStatic()
{
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMDbLockStatic, CTreeItem, 1)

CuTMDbLockStatic::CuTMDbLockStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_DATABASE, IDS_TM_DB_LOCK, SUBBRANCH_KIND_OBJ, IDB_TM_DB_LOCK, pItemData, OT_MON_LOCK, IDS_TM_DB_LOCK_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMDbLockStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMLock* pBranch = new CuTMLock (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMDbLockStatic::CuTMDbLockStatic()
{
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMDbLockedTblStatic, CTreeItem, 1)
IMPLEMENT_SERIAL (CuTMDbLockedTbl, CTreeItem, 1)

CuTMDbLockedTblStatic::CuTMDbLockedTblStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_DATABASE, IDS_TM_DB_LOCKEDTBL, SUBBRANCH_KIND_OBJ, IDB_TM_DB_LOCKEDTBL, pItemData, OT_MON_RES_TABLE, IDS_TM_DB_LOCKEDTBL_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMDbLockedTblStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMDbLockedTbl* pBranch = new CuTMDbLockedTbl (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMDbLockedTblStatic::CuTMDbLockedTblStatic()
{
}

CuTMDbLockedTbl::CuTMDbLockedTbl (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_RES_TABLE, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);;
	InitializeImageId(IDB_TM_DB_LOCKEDTBL);
}

CuTMDbLockedTbl::CuTMDbLockedTbl()
{
}

BOOL CuTMDbLockedTbl::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();

	CuTMResourceTblLockStatic *pBranch1 = new CuTMResourceTblLockStatic(pGlobalData, GetPTreeItemData());
	pBranch1->CreateTreeLine (hParentItem);
	return TRUE;
}


//
// Emb April 4, 97: Duplicated from CuTMLockinfoResource::GetPageInformation()
// Changed April 22 at Fnn request: only 2 tabs (detail and locks)
CuPageInformation* CuTMDbLockedTbl::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [2] = 
	{
		IDS_TAB_DETAIL, 
		IDS_TAB_LOCK,
	};
	UINT nDlgID [2] = 
	{
		IDD_IPMDETAIL_RESOURCE, 
		IDD_IPMPAGE_LOCKS,
	};
	UINT nIDTitle = IDS_TAB_RESOURCE_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMDbLockedTbl"), 2, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}


/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMDbLockedPgStatic, CTreeItem, 1)
IMPLEMENT_SERIAL (CuTMDbLockedPg, CTreeItem, 1)

CuTMDbLockedPgStatic::CuTMDbLockedPgStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_DATABASE, IDS_TM_DB_LOCKEDPAGE, SUBBRANCH_KIND_OBJ, IDB_TM_DB_LOCKEDPAGE, pItemData, OT_MON_RES_PAGE, IDS_TM_DB_LOCKEDPAGE_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMDbLockedPgStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMDbLockedPg* pBranch = new CuTMDbLockedPg (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMDbLockedPgStatic::CuTMDbLockedPgStatic()
{
}

CuTMDbLockedPg::CuTMDbLockedPg (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_RES_PAGE, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);;
	InitializeImageId(IDB_TM_DB_LOCKEDPAGE);
}

CuTMDbLockedPg::CuTMDbLockedPg()
{
}

BOOL CuTMDbLockedPg::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();

	CuTMResourcePgLockStatic *pBranch1 = new CuTMResourcePgLockStatic(pGlobalData, GetPTreeItemData());
	pBranch1->CreateTreeLine (hParentItem);

	return TRUE;
}


//
// Emb April 4, 97: Duplicated from CuTMLockinfoResource::GetPageInformation()
// Changed April 22 at Fnn request: only 2 tabs (detail and locks)
CuPageInformation* CuTMDbLockedPg::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [2] = 
	{
		IDS_TAB_DETAIL, 
		IDS_TAB_LOCK,
	};
	UINT nDlgID [2] = 
	{
		IDD_IPMDETAIL_RESOURCE, 
		IDD_IPMPAGE_LOCKS,
	};
	UINT nIDTitle = IDS_TAB_RESOURCE_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMDbLockedPg"), 2, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}


/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMDbLockedOtherStatic, CTreeItem, 1)
IMPLEMENT_SERIAL (CuTMDbLockedOther, CTreeItem, 1)

CuTMDbLockedOtherStatic::CuTMDbLockedOtherStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_DATABASE, IDS_TM_DB_LOCKEDOTHER, SUBBRANCH_KIND_OBJ, IDB_TM_DB_LOCKEDOTHER, pItemData, OT_MON_RES_OTHER, IDS_TM_DB_LOCKEDOTHER_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMDbLockedOtherStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMDbLockedOther* pBranch = new CuTMDbLockedOther (GetPTreeGlobData(), pItemData);
	return pBranch;
}

BOOL CuTMDbLockedOtherStatic::FilterApplies(FilterCause because)
{
	switch (because) {
		case FILTER_RESOURCE_TYPE:
		case FILTER_NULL_RESOURCES:
			return TRUE;
		default:
			return CTreeItem::FilterApplies(because);
	}
	ASSERT (FALSE);
}

CuTMDbLockedOtherStatic::CuTMDbLockedOtherStatic()
{
}

CuTMDbLockedOther::CuTMDbLockedOther (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_RES_OTHER, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);;
	InitializeImageId(IDB_TM_DB_LOCKEDOTHER);
}

CuTMDbLockedOther::CuTMDbLockedOther()
{
}

BOOL CuTMDbLockedOther::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();

	CuTMResourceOthLockStatic *pBranch1 = new CuTMResourceOthLockStatic(pGlobalData, GetPTreeItemData());
	pBranch1->CreateTreeLine (hParentItem);
	return TRUE;
}


//
// Emb April 4, 97: Duplicated from CuTMLockinfoResource::GetPageInformation()
// Changed April 22 at Fnn request: only 2 tabs (detail and locks)
CuPageInformation* CuTMDbLockedOther::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [2] = 
	{
		IDS_TAB_DETAIL, 
		IDS_TAB_LOCK,
	};
	UINT nDlgID [2] = 
	{
		IDD_IPMDETAIL_RESOURCE, 
		IDD_IPMPAGE_LOCKS,
	};
	UINT nIDTitle = IDS_TAB_RESOURCE_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMDbLockedOther"), 2, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}


/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMDbServerStatic, CTreeItem, 1)

CuTMDbServerStatic::CuTMDbServerStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_DATABASE, IDS_TM_DB_SERVER, SUBBRANCH_KIND_OBJ, IDB_TM_DB_SERVER, pItemData, OT_MON_SERVER, IDS_TM_DB_SERVER_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMDbServerStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CTreeItem *pItem;

	// Emb July 3, 97: take "error" branch in account
	if (!pItemData) {
		pItem = new CuTMServer (GetPTreeGlobData(), pItemData);
		return pItem;
	}

	LPSERVERDATAMIN lps =(LPSERVERDATAMIN)pItemData->GetDataPtr();
	ASSERT (lps);
	switch (lps->servertype) {
		case SVR_TYPE_STAR:
		case SVR_TYPE_RECOVERY:
		case SVR_TYPE_DBMS:
			pItem = new CuTMServer (GetPTreeGlobData(), pItemData);
			return pItem;
		case SVR_TYPE_ICE:
			pItem = new CuTMIceServer (GetPTreeGlobData(), pItemData);
			return pItem;
		default:
			// Should never happend ---> debug assertion
			ASSERT (FALSE);
			pItem = new CuTMServerNotExpandable (GetPTreeGlobData(), pItemData);
			return pItem;
	}

	ASSERT (FALSE);
	return 0;
}

CuTMDbServerStatic::CuTMDbServerStatic()
{
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMDbTransacStatic, CTreeItem, 1)

CuTMDbTransacStatic::CuTMDbTransacStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_DATABASE, IDS_TM_DB_TRANSAC, SUBBRANCH_KIND_OBJ, IDB_TM_DB_TRANSAC, pItemData, OT_MON_TRANSACTION, IDS_TM_DB_TRANSAC_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMDbTransacStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMTransac* pBranch = new CuTMTransac (GetPTreeGlobData(), pItemData);
	return pBranch;
}

BOOL CuTMDbTransacStatic::FilterApplies(FilterCause because)
{
	switch (because) {
		case FILTER_INACTIVE_TRANSACTIONS:
			return TRUE;
		default:
			return CTreeItem::FilterApplies(because);
	}
	ASSERT (FALSE);
}

CuTMDbTransacStatic::CuTMDbTransacStatic()
{
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMDbLockOnDbStatic, CTreeItem, 1)

CuTMDbLockOnDbStatic::CuTMDbLockOnDbStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_RES_DB, IDS_TM_DB_LOCKONDB, SUBBRANCH_KIND_OBJ, IDB_TM_DB_LOCKONDB, pItemData, OT_MON_LOCK, IDS_TM_DB_LOCKONDB_NONE)
{
	// Note : replaced OT_DATABASE with OT_MON_RES_DB at Fnn request 21/3/97
	ASSERT (IPM_StructSize(OT_DATABASE) == IPM_StructSize(OT_MON_RES_DB));
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMDbLockOnDbStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMLock* pBranch = new CuTMLock (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMDbLockOnDbStatic::CuTMDbLockOnDbStatic()
{
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMDbSessionStatic, CTreeItem, 1)

CuTMDbSessionStatic::CuTMDbSessionStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_DATABASE, IDS_TM_DB_SESSION, SUBBRANCH_KIND_OBJ, IDB_TM_DB_SESSION, pItemData, OT_MON_SESSION, IDS_TM_DB_SESSION_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMDbSessionStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMSession* pBranch = new CuTMSession (GetPTreeGlobData(), pItemData);
	return pBranch;
}

BOOL CuTMDbSessionStatic::FilterApplies(FilterCause because)
{
	switch (because) {
		case FILTER_INTERNAL_SESSIONS:
			return TRUE;
		default:
			return CTreeItem::FilterApplies(because);
	}
	ASSERT (FALSE);
}

CuTMDbSessionStatic::CuTMDbSessionStatic()
{
}


/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMActiveUsrStatic, CTreeItem, 1)
IMPLEMENT_SERIAL (CuTMActiveUsr, CTreeItem, 1)

CuTMActiveUsrStatic::CuTMActiveUsrStatic (CTreeGlobalData* pTreeGlobalData)
	:CTreeItem (pTreeGlobalData, 0, IDS_TM_ACTUSR, SUBBRANCH_KIND_OBJ, IDB_TM_ACTUSR, NULL, OT_MON_ACTIVE_USER, IDS_TM_ACTUSR_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMActiveUsrStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMActiveUsr* pBranch = new CuTMActiveUsr (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMActiveUsrStatic::CuTMActiveUsrStatic()
{
}

CuPageInformation* CuTMActiveUsrStatic::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = { IDS_TAB_STATIC_ACTIVEUSER, };
	UINT nDlgID [1] = { IDD_IPMPAGE_STATIC_ACTIVEUSERS, };
	UINT nIDTitle = IDS_TAB_STATIC_ACTIVEUSER_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMActiveUsrStatic"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

CuTMActiveUsr::CuTMActiveUsr (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_ACTIVE_USER, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);;
	InitializeImageId(IDB_TM_ACTUSR);
}

CuTMActiveUsr::CuTMActiveUsr()
{
}

BOOL CuTMActiveUsr::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();

	CuTMUsrSessStatic    *pBranch1 = new CuTMUsrSessStatic(pGlobalData, GetPTreeItemData());
	pBranch1->CreateTreeLine (hParentItem);

	return TRUE;
}

//
// We must know all information how to display 
// the property of this object: how many pages, and what they are ...

// Modified April 14, 97 at fnn request : only one tab "sessions"
CuPageInformation* CuTMActiveUsr::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = { IDS_TAB_SESSION };
	UINT nDlgID [2] = { IDD_IPMPAGE_SESSIONS };
	UINT nIDTitle = IDS_TAB_ACTIVEUSER_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMActiveUsr"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMUsrSessStatic, CTreeItem, 1)

CuTMUsrSessStatic::CuTMUsrSessStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_ACTIVE_USER, IDS_TM_USRSESS, SUBBRANCH_KIND_OBJ, IDB_TM_USRSESS, pItemData, OT_MON_SESSION, IDS_TM_USRSESS_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMUsrSessStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMSession* pBranch = new CuTMSession (GetPTreeGlobData(), pItemData);
	return pBranch;
}

BOOL CuTMUsrSessStatic::FilterApplies(FilterCause because)
{
	switch (because) {
		case FILTER_INTERNAL_SESSIONS:
			return TRUE;
		default:
			return CTreeItem::FilterApplies(because);
	}
	ASSERT (FALSE);
}

CuTMUsrSessStatic::CuTMUsrSessStatic()
{
}

/* --------------------------------------MONTREE3---------------------------------------*/
IMPLEMENT_SERIAL (CuTMLockLocklistStatic, CTreeItem, 1)

CuTMLockLocklistStatic::CuTMLockLocklistStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_LOCK, IDS_TM_LOCK_LOCKLIST, SUBBRANCH_KIND_OBJ, IDB_TM_LOCK_LOCKLIST, pItemData, OT_MON_LOCKLIST, IDS_TM_LOCK_LOCKLIST_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMLockLocklistStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMLocklist* pBranch = new CuTMLocklist (GetPTreeGlobData(), pItemData);
	return pBranch;
}

BOOL CuTMLockLocklistStatic::FilterApplies(FilterCause because)
{
	switch (because) {
		case FILTER_SYSTEM_LOCK_LISTS:
			return TRUE;
		default:
			return CTreeItem::FilterApplies(because);
	}
	ASSERT (FALSE);
}

CuTMLockLocklistStatic::CuTMLockLocklistStatic()
{
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMLockSessionStatic, CTreeItem, 1)

CuTMLockSessionStatic::CuTMLockSessionStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_LOCK, IDS_TM_LOCK_SESSION, SUBBRANCH_KIND_OBJ, IDB_TM_LOCK_SESSION, pItemData, OT_MON_SESSION, IDS_TM_LOCK_SESSION_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMLockSessionStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMSession* pBranch = new CuTMSession (GetPTreeGlobData(), pItemData);
	return pBranch;
}

BOOL CuTMLockSessionStatic::FilterApplies(FilterCause because)
{
	switch (because) {
		case FILTER_INTERNAL_SESSIONS:
			return TRUE;
		default:
			return CTreeItem::FilterApplies(because);
	}
	ASSERT (FALSE);
}

CuTMLockSessionStatic::CuTMLockSessionStatic()
{
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMLockServerStatic, CTreeItem, 1)

CuTMLockServerStatic::CuTMLockServerStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_LOCK, IDS_TM_LOCK_SERVER, SUBBRANCH_KIND_OBJ, IDB_TM_LOCK_SERVER, pItemData, OT_MON_SERVER, IDS_TM_LOCK_SERVER_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMLockServerStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CTreeItem *pItem;

	// Emb July 3, 97: take "error" branch in account
	if (!pItemData) {
		pItem = new CuTMServer (GetPTreeGlobData(), pItemData);
		return pItem;
	}

	LPSERVERDATAMIN lps =(LPSERVERDATAMIN)pItemData->GetDataPtr();
	ASSERT (lps);
	switch (lps->servertype) {
		case SVR_TYPE_STAR:
		case SVR_TYPE_RECOVERY:
		case SVR_TYPE_DBMS:
			pItem = new CuTMServer (GetPTreeGlobData(), pItemData);
			return pItem;
		case SVR_TYPE_ICE:
			pItem = new CuTMIceServer (GetPTreeGlobData(), pItemData);
			return pItem;
		default:
			// Should never happend ---> debug assertion
			ASSERT (FALSE);
			pItem = new CuTMServerNotExpandable (GetPTreeGlobData(), pItemData);
			return pItem;
	}

	ASSERT (FALSE);
	return 0;
}

CuTMLockServerStatic::CuTMLockServerStatic()
{
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMTransacLocklistStatic, CTreeItem, 1)

CuTMTransacLocklistStatic::CuTMTransacLocklistStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_TRANSACTION, IDS_TM_TRANSAC_LOCKLIST, SUBBRANCH_KIND_OBJ, IDB_TM_TRANSAC_LOCKLIST, pItemData, OT_MON_LOCKLIST, IDS_TM_TRANSAC_LOCKLIST_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMTransacLocklistStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMLocklist* pBranch = new CuTMLocklist (GetPTreeGlobData(), pItemData);
	return pBranch;
}

BOOL CuTMTransacLocklistStatic::FilterApplies(FilterCause because)
{
	switch (because) {
		case FILTER_SYSTEM_LOCK_LISTS:
			return TRUE;
		default:
			return CTreeItem::FilterApplies(because);
	}
	ASSERT (FALSE);
}

CuTMTransacLocklistStatic::CuTMTransacLocklistStatic()
{
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMTransacLockStatic, CTreeItem, 1)

CuTMTransacLockStatic::CuTMTransacLockStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_TRANSACTION, IDS_TM_TRANSAC_LOCK, SUBBRANCH_KIND_OBJ, IDB_TM_TRANSAC_LOCK, pItemData, OT_MON_LOCK, IDS_TM_TRANSAC_LOCK_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMTransacLockStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMLock* pBranch = new CuTMLock (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMTransacLockStatic::CuTMTransacLockStatic()
{
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMTransacResourceStatic, CTreeItem, 1)

CuTMTransacResourceStatic::CuTMTransacResourceStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_TRANSACTION, IDS_TM_TRANSAC_RESOURCE, SUBBRANCH_KIND_STATIC, IDB_TM_TRANSAC_RESOURCE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CuTMTransacResourceStatic::CuTMTransacResourceStatic()
{
}

BOOL CuTMTransacResourceStatic::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();

	CuTMTransacDbStatic     *pBranch1 = new CuTMTransacDbStatic(pGlobalData, GetPTreeItemData());
	CuTMTransacTblStatic    *pBranch2 = new CuTMTransacTblStatic(pGlobalData, GetPTreeItemData());
	CuTMTransacPgStatic     *pBranch3 = new CuTMTransacPgStatic(pGlobalData, GetPTreeItemData());
	CuTMTransacOtherStatic  *pBranch4 = new CuTMTransacOtherStatic(pGlobalData, GetPTreeItemData());

	pBranch1->CreateTreeLine (hParentItem);
	pBranch2->CreateTreeLine (hParentItem);
	pBranch3->CreateTreeLine (hParentItem);
	pBranch4->CreateTreeLine (hParentItem);

	return TRUE;
}

BOOL CuTMTransacResourceStatic::FilterApplies(FilterCause because)
{
	switch (because) {
		case FILTER_RESOURCE_TYPE:
		case FILTER_NULL_RESOURCES:
			return TRUE;
		default:
			return CTreeItem::FilterApplies(because);
	}
	ASSERT (FALSE);
}


/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMTransacDbStatic, CTreeItem, 1)
IMPLEMENT_SERIAL (CuTMTransacDb, CTreeItem, 1)

CuTMTransacDbStatic::CuTMTransacDbStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_TRANSACTION, IDS_TM_TRANSAC_DB, SUBBRANCH_KIND_OBJ, IDB_TM_TRANSAC_DB, pItemData, OT_MON_RES_DB, IDS_TM_TRANSAC_DB_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMTransacDbStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMTransacDb* pBranch = new CuTMTransacDb (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMTransacDbStatic::CuTMTransacDbStatic()
{
}

CuTMTransacDb::CuTMTransacDb (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_RES_DB, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetCompleteType(OT_DATABASE);   // Base Type for subbranches
	SetContextMenuId(IDR_POPUP_IPM);;
	InitializeImageId(IDB_TM_TRANSAC_DB);
}

CuTMTransacDb::CuTMTransacDb()
{
}

UINT CuTMTransacDb::GetCustomImageId()
{
	// old code : return IDB_TM_LOCK_TYPE_DB;
	LPRESOURCEDATAMIN lpr =(LPRESOURCEDATAMIN)GetPTreeItemData()->GetDataPtr();
	return GetDbTypeImageId(lpr);
}

BOOL CuTMTransacDb::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();

	// checks on completed type
	ASSERT (IsTypeCompleted());
	ASSERT (GetCompleteType() == OT_DATABASE);

	// Masqued April 14, 97 for fnn : CuTMDbAllTblStatic      *pBranch1 = new CuTMDbAllTblStatic      (pGlobalData, GetPTreeItemData());
	CuTMDbLocklistStatic    *pBranch2 = new CuTMDbLocklistStatic    (pGlobalData, GetPTreeItemData());
	CuTMDbLockStatic        *pBranch3 = new CuTMDbLockStatic        (pGlobalData, GetPTreeItemData());
	CuTMDbLockedTblStatic   *pBranch4 = new CuTMDbLockedTblStatic   (pGlobalData, GetPTreeItemData());
	CuTMDbLockedPgStatic    *pBranch4b= new CuTMDbLockedPgStatic    (pGlobalData, GetPTreeItemData());
	CuTMDbLockedOtherStatic *pBranch5 = new CuTMDbLockedOtherStatic (pGlobalData, GetPTreeItemData());
	CuTMDbServerStatic      *pBranch6 = new CuTMDbServerStatic      (pGlobalData, GetPTreeItemData());
	CuTMDbTransacStatic     *pBranch7 = new CuTMDbTransacStatic     (pGlobalData, GetPTreeItemData());
	// masqued April 15 for fnn: CuTMDbLockOnDbStatic    *pBranch8 = new CuTMDbLockOnDbStatic    (pGlobalData, GetPTreeItemData());
	CuTMDbSessionStatic     *pBranch9 = new CuTMDbSessionStatic     (pGlobalData, GetPTreeItemData());
	CuTMDbReplStatic        *pBranch10 = new CuTMDbReplStatic        (pGlobalData, GetPTreeItemData());

	// Masqued April 14, 97 for fnn : pBranch1->CreateTreeLine (hParentItem);
	pBranch2->CreateTreeLine (hParentItem);
	pBranch3->CreateTreeLine (hParentItem);
	pBranch4->CreateTreeLine (hParentItem);
	pBranch4b->CreateTreeLine (hParentItem);
	pBranch5->CreateTreeLine (hParentItem);
	pBranch6->CreateTreeLine (hParentItem);
	pBranch7->CreateTreeLine (hParentItem);
	// masqued April 15 for fnn: pBranch8->CreateTreeLine (hParentItem);
	pBranch9->CreateTreeLine (hParentItem);
	pBranch10->CreateTreeLine (hParentItem);

	return TRUE;
}


CuPageInformation* CuTMTransacDb::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [4] = 
	{
		IDS_TAB_SESSION,
		IDS_TAB_LOCKLIST,
		IDS_TAB_LOCK,
		IDS_TAB_TRANSACTION,
	};
	UINT nDlgID [4] = 
	{
		IDD_IPMPAGE_SESSIONS,
		IDD_IPMPAGE_LOCKLISTS,
		IDD_IPMPAGE_LOCKS,
		IDD_IPMPAGE_TRANSACTIONS,
	};
	UINT nIDTitle = IDS_TAB_DATABASE_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMTransacDb"), 4, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}


/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMTransacOtherStatic, CTreeItem, 1)
IMPLEMENT_SERIAL (CuTMTransacOther, CTreeItem, 1)

CuTMTransacOtherStatic::CuTMTransacOtherStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_TRANSACTION, IDS_TM_TRANSAC_OTHER, SUBBRANCH_KIND_OBJ, IDB_TM_TRANSAC_OTHER, pItemData, OT_MON_RES_OTHER, IDS_TM_TRANSAC_OTHER_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMTransacOtherStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMTransacOther* pBranch = new CuTMTransacOther (GetPTreeGlobData(), pItemData);
	return pBranch;
}

BOOL CuTMTransacOtherStatic::FilterApplies(FilterCause because)
{
	switch (because) {
		case FILTER_RESOURCE_TYPE:
		case FILTER_NULL_RESOURCES:
			return TRUE;
		default:
			return CTreeItem::FilterApplies(because);
	}
	ASSERT (FALSE);
}

CuTMTransacOtherStatic::CuTMTransacOtherStatic()
{
}

CuTMTransacOther::CuTMTransacOther (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_RES_OTHER, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);;
	InitializeImageId(IDB_TM_TRANSAC_OTHER);
}

CuTMTransacOther::CuTMTransacOther()
{
}

BOOL CuTMTransacOther::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();
	CuTMResourceOthLockStatic    *pBranch1 = new CuTMResourceOthLockStatic(pGlobalData, GetPTreeItemData());
	pBranch1->CreateTreeLine (hParentItem);
	return TRUE;
}


/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMTransacSessionStatic, CTreeItem, 1)

CuTMTransacSessionStatic::CuTMTransacSessionStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_TRANSACTION, IDS_TM_TRANSAC_SESSION, SUBBRANCH_KIND_OBJ, IDB_TM_TRANSAC_SESSION, pItemData, OT_MON_SESSION, IDS_TM_TRANSAC_SESSION_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMTransacSessionStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMSession* pBranch = new CuTMSession(GetPTreeGlobData(), pItemData);
	return pBranch;
}

BOOL CuTMTransacSessionStatic::FilterApplies(FilterCause because)
{
	switch (because) {
		case FILTER_INTERNAL_SESSIONS:
			return TRUE;
		default:
			return CTreeItem::FilterApplies(because);
	}
	ASSERT (FALSE);
}

CuTMTransacSessionStatic::CuTMTransacSessionStatic()
{
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMTransacServerStatic, CTreeItem, 1)

CuTMTransacServerStatic::CuTMTransacServerStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_TRANSACTION, IDS_TM_TRANSAC_SERVER, SUBBRANCH_KIND_OBJ, IDB_TM_TRANSAC_SERVER, pItemData, OT_MON_SERVER, IDS_TM_TRANSAC_SERVER_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMTransacServerStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CTreeItem *pItem;

	// Emb July 3, 97: take "error" branch in account
	if (!pItemData) {
		pItem = new CuTMServer (GetPTreeGlobData(), pItemData);
		return pItem;
	}

	LPSERVERDATAMIN lps =(LPSERVERDATAMIN)pItemData->GetDataPtr();
	ASSERT (lps);
	switch (lps->servertype) {
		case SVR_TYPE_STAR:
		case SVR_TYPE_RECOVERY:
		case SVR_TYPE_DBMS:
			pItem = new CuTMServer (GetPTreeGlobData(), pItemData);
			return pItem;
		case SVR_TYPE_ICE:
			pItem = new CuTMIceServer (GetPTreeGlobData(), pItemData);
			return pItem;
		default:
			// Should never happend ---> debug assertion
			ASSERT (FALSE);
			pItem = new CuTMServerNotExpandable (GetPTreeGlobData(), pItemData);
			return pItem;
	}

	ASSERT (FALSE);
	return 0;
}

CuTMTransacServerStatic::CuTMTransacServerStatic()
{
}



/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMTransacTblStatic, CTreeItem, 1)
IMPLEMENT_SERIAL (CuTMTransacTbl, CTreeItem, 1)

CuTMTransacTblStatic::CuTMTransacTblStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_TRANSACTION, IDS_TM_TRANSAC_TBL, SUBBRANCH_KIND_OBJ, IDB_TM_TRANSAC_TBL, pItemData, OT_MON_RES_TABLE, IDS_TM_TRANSAC_TBL_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMTransacTblStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMTransacTbl* pBranch = new CuTMTransacTbl (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMTransacTblStatic::CuTMTransacTblStatic()
{
}

CuTMTransacTbl::CuTMTransacTbl (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_RES_TABLE, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);;
	InitializeImageId(IDB_TM_TRANSAC_TBL);
}

CuTMTransacTbl::CuTMTransacTbl()
{
}

BOOL CuTMTransacTbl::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();
	CuTMResourceTblLockStatic    *pBranch1 = new CuTMResourceTblLockStatic(pGlobalData, GetPTreeItemData());
	pBranch1->CreateTreeLine (hParentItem);
	return TRUE;
}


/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMTransacPgStatic, CTreeItem, 1)
IMPLEMENT_SERIAL (CuTMTransacPg, CTreeItem, 1)

CuTMTransacPgStatic::CuTMTransacPgStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_TRANSACTION, IDS_TM_TRANSAC_PAGE, SUBBRANCH_KIND_OBJ, IDB_TM_TRANSAC_PAGE, pItemData, OT_MON_RES_PAGE, IDS_TM_TRANSAC_PAGE_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMTransacPgStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMTransacPg* pBranch = new CuTMTransacPg (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMTransacPgStatic::CuTMTransacPgStatic()
{
}

CuTMTransacPg::CuTMTransacPg (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_RES_PAGE, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);;
	InitializeImageId(IDB_TM_TRANSAC_PAGE);
}

CuTMTransacPg::CuTMTransacPg()
{
}

BOOL CuTMTransacPg::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();
	CuTMResourcePgLockStatic    *pBranch1 = new CuTMResourcePgLockStatic(pGlobalData, GetPTreeItemData());
	pBranch1->CreateTreeLine (hParentItem);
	return TRUE;
}




/* ------------------------------------------------------------------------------*/

/* New as of May 14, 98: blocking session for a lock */

IMPLEMENT_SERIAL (CuTMBlockingSessionStatic, CTreeItem, 1)
IMPLEMENT_SERIAL (CuTMBlockingSession, CTreeItem, 1)

CuTMBlockingSessionStatic::CuTMBlockingSessionStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_LOCKLIST, IDS_TM_BLOCKING_SESSION, SUBBRANCH_KIND_OBJ, IDB_TM_SESSION, pItemData, OT_MON_BLOCKING_SESSION, IDS_TM_BLOCKING_SESSION_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMBlockingSessionStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMBlockingSession* pBranch = new CuTMBlockingSession (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMBlockingSessionStatic::CuTMBlockingSessionStatic()
{
}

BOOL CuTMBlockingSessionStatic::FilterApplies(FilterCause because)
{
	return CTreeItem::FilterApplies(because);
}


CuTMBlockingSession::CuTMBlockingSession (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_BLOCKING_SESSION, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetCompleteType(OT_MON_SESSION);    // Base Type for subbranches
	SetContextMenuId(IDR_POPUP_IPM);
	InitializeImageId(IDB_TM_SESSION);
}

CuTMBlockingSession::CuTMBlockingSession()
{
}

BOOL CuTMBlockingSession::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();

	CuTMSessLocklistStatic  *pBranch1 = new CuTMSessLocklistStatic(pGlobalData, GetPTreeItemData());
	CuTMSessLockStatic      *pBranch2 = new CuTMSessLockStatic(pGlobalData, GetPTreeItemData());
	CuTMSessTransacStatic   *pBranch3 = new CuTMSessTransacStatic(pGlobalData, GetPTreeItemData());
	CuTMSessDbStatic        *pBranch4 = new CuTMSessDbStatic(pGlobalData, GetPTreeItemData());

	pBranch1->CreateTreeLine (hParentItem);
	pBranch2->CreateTreeLine (hParentItem);
	pBranch3->CreateTreeLine (hParentItem);
	pBranch4->CreateTreeLine (hParentItem);

	// new as of March 21, 97
	CuTMSessTblStatic       *pBranch5 = new CuTMSessTblStatic(pGlobalData, GetPTreeItemData());
	CuTMSessPgStatic        *pBranch6 = new CuTMSessPgStatic(pGlobalData, GetPTreeItemData());
	CuTMSessOthStatic       *pBranch7 = new CuTMSessOthStatic(pGlobalData, GetPTreeItemData());
	pBranch5->CreateTreeLine (hParentItem);
	pBranch6->CreateTreeLine (hParentItem);
	pBranch7->CreateTreeLine (hParentItem);

	// New as of May 18, 98
	CuTMSessBlockingSessionStatic *pBranch8 = new CuTMSessBlockingSessionStatic(pGlobalData, GetPTreeItemData());
	pBranch8->CreateTreeLine (hParentItem);

	return TRUE;
}

//
// We must know all information how to display 
// the property of this object: how many pages, and what they are ...
CuPageInformation* CuTMBlockingSession::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [5] = 
	{
		IDS_TAB_DETAIL, 
		IDS_TAB_CLIENT,
		IDS_TAB_LOCKLIST, 
		IDS_TAB_LOCK, 
		IDS_TAB_TRANSACTION
	};
	UINT nDlgID [5] = 
	{
		IDD_IPMDETAIL_SESSION, 
		IDD_IPMPAGE_CLIENT,
		IDD_IPMPAGE_LOCKLISTS, 
		IDD_IPMPAGE_LOCKS, 
		IDD_IPMPAGE_TRANSACTIONS, 
	};
	UINT nIDTitle = IDS_TAB_SESSION_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMBlockingSession"), 5, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}


BOOL CuTMBlockingSession::IsEnabled(UINT idMenu)
{
	LPSESSIONDATAMIN pSession = 0;
	switch (idMenu) {
		case IDM_IPMBAR_SHUTDOWN:   // KillShutdown
		case IDM_IPMBAR_CLOSESVR:
			if (IsNoItem() || IsErrorItem())
			return FALSE;
			pSession =(LPSESSIONDATAMIN)GetPTreeItemData()->GetDataPtr();
			ASSERT (pSession);
			if (!IsShutdownServerType(&(pSession->sesssvrdata)))
				return FALSE;

			//For internal session no menu "remove /shutdown" available
			if (IsInternalSession(pSession))
				return FALSE;

			return TRUE;
		default:
			return CTreeItem::IsEnabled(idMenu);
	}
	return FALSE;   // security if return forgotten in switch
}

UINT CuTMBlockingSession::GetCustomImageId()
{
	LPSESSIONDATAMIN pSession =(LPSESSIONDATAMIN)GetPTreeItemData()->GetDataPtr();
	ASSERT (pSession);
	if (pSession->locklist_wait_id)
		return IDB_TM_SESSION_BLOCKED;
	else
		return IDB_TM_SESSION;
}

BOOL CuTMBlockingSession::KillShutdown()
{
	BOOL bRet = FALSE;
	CdIpmDoc* pDoc = GetPTreeGlobData()->GetDocument();
	CaConnectInfo& connectInfo = pDoc->GetConnectionInfo();
	CString strNode = connectInfo.GetNode();

	// get info about session
	LPSESSIONDATAMIN pSession =(LPSESSIONDATAMIN)GetPTreeItemData()->GetDataPtr();
	TCHAR szSessName[100];
	CTreeCtrl* pTreeCtrl = GetPTreeGlobData()->GetPTree();
	IPM_GetInfoName(GetPTreeGlobData()->GetDocument(), OT_MON_SESSION, pSession, szSessName, sizeof(szSessName));

	// confirm or cancel
	CString capt;
	capt.Format(IDS_KILLSESSION_CONFIRM_CAPT, (LPCTSTR)strNode);
	CString txt;
	txt.Format(IDS_KILLSESSION_CONFIRM_TXT, szSessName);
	int ret = MessageBox(GetFocus(), txt, capt, MB_YESNO | MB_ICONQUESTION);
	if (ret == IDYES)
	{
		bRet= IPM_RemoveSession(pDoc, pSession);
	}
	return bRet;
}

/* ------------------------------------------------------------------------------*/

/* New as of May 18, 98: blocking session for a session */

IMPLEMENT_SERIAL (CuTMSessBlockingSessionStatic, CTreeItem, 1)
IMPLEMENT_SERIAL (CuTMSessBlockingSession, CTreeItem, 1)

CuTMSessBlockingSessionStatic::CuTMSessBlockingSessionStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_SESSION, IDS_TM_BLOCKING_SESSION, SUBBRANCH_KIND_OBJ, IDB_TM_SESSION, pItemData, OT_MON_BLOCKING_SESSION, IDS_TM_BLOCKING_SESSION_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMSessBlockingSessionStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMSessBlockingSession* pBranch = new CuTMSessBlockingSession (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMSessBlockingSessionStatic::CuTMSessBlockingSessionStatic()
{
}

BOOL CuTMSessBlockingSessionStatic::FilterApplies(FilterCause because)
{
	return CTreeItem::FilterApplies(because);
}


CuTMSessBlockingSession::CuTMSessBlockingSession (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_BLOCKING_SESSION, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetCompleteType(OT_MON_SESSION);    // Base Type for subbranches
	SetContextMenuId(IDR_POPUP_IPM);
	InitializeImageId(IDB_TM_SESSION);
}

CuTMSessBlockingSession::CuTMSessBlockingSession()
{
}

BOOL CuTMSessBlockingSession::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();

	CuTMSessLocklistStatic  *pBranch1 = new CuTMSessLocklistStatic(pGlobalData, GetPTreeItemData());
	CuTMSessLockStatic      *pBranch2 = new CuTMSessLockStatic(pGlobalData, GetPTreeItemData());
	CuTMSessTransacStatic   *pBranch3 = new CuTMSessTransacStatic(pGlobalData, GetPTreeItemData());
	CuTMSessDbStatic        *pBranch4 = new CuTMSessDbStatic(pGlobalData, GetPTreeItemData());

	pBranch1->CreateTreeLine (hParentItem);
	pBranch2->CreateTreeLine (hParentItem);
	pBranch3->CreateTreeLine (hParentItem);
	pBranch4->CreateTreeLine (hParentItem);

	// new as of March 21, 97
	CuTMSessTblStatic       *pBranch5 = new CuTMSessTblStatic(pGlobalData, GetPTreeItemData());
	CuTMSessPgStatic        *pBranch6 = new CuTMSessPgStatic(pGlobalData, GetPTreeItemData());
	CuTMSessOthStatic       *pBranch7 = new CuTMSessOthStatic(pGlobalData, GetPTreeItemData());
	pBranch5->CreateTreeLine (hParentItem);
	pBranch6->CreateTreeLine (hParentItem);
	pBranch7->CreateTreeLine (hParentItem);

	// New as of May 18, 98
	CuTMSessBlockingSessionStatic *pBranch8 = new CuTMSessBlockingSessionStatic(pGlobalData, GetPTreeItemData());
	pBranch8->CreateTreeLine (hParentItem);

	return TRUE;
}

//
// We must know all information how to display 
// the property of this object: how many pages, and what they are ...
CuPageInformation* CuTMSessBlockingSession::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [5] = 
	{
		IDS_TAB_DETAIL, 
		IDS_TAB_CLIENT,
		IDS_TAB_LOCKLIST, 
		IDS_TAB_LOCK, 
		IDS_TAB_TRANSACTION
	};
	UINT nDlgID [5] = 
	{
		IDD_IPMDETAIL_SESSION, 
		IDD_IPMPAGE_CLIENT,
		IDD_IPMPAGE_LOCKLISTS, 
		IDD_IPMPAGE_LOCKS, 
		IDD_IPMPAGE_TRANSACTIONS, 
	};
	UINT nIDTitle = IDS_TAB_SESSION_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMSessBlockingSession"), 5, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}


BOOL CuTMSessBlockingSession::IsEnabled(UINT idMenu)
{
	LPSESSIONDATAMIN pSession = 0;
	switch (idMenu) {
		case IDM_IPMBAR_SHUTDOWN:   // KillShutdown
		case IDM_IPMBAR_CLOSESVR:
			if (IsNoItem() || IsErrorItem())
				return FALSE;
			pSession =(LPSESSIONDATAMIN)GetPTreeItemData()->GetDataPtr();
			ASSERT (pSession);
			if (!IsShutdownServerType(&(pSession->sesssvrdata)))
				return FALSE;
			return TRUE;
		default:
			return CTreeItem::IsEnabled(idMenu);
	}
	return FALSE;   // security if return forgotten in switch
}

UINT CuTMSessBlockingSession::GetCustomImageId()
{
	LPSESSIONDATAMIN pSession =(LPSESSIONDATAMIN)GetPTreeItemData()->GetDataPtr();
	ASSERT (pSession);
	if (pSession->locklist_wait_id)
		return IDB_TM_SESSION_BLOCKED;
	else
		return IDB_TM_SESSION;
}

BOOL CuTMSessBlockingSession::KillShutdown()
{
	BOOL bRet = FALSE;
	CdIpmDoc* pDoc = GetPTreeGlobData()->GetDocument();
	CaConnectInfo& connectInfo = pDoc->GetConnectionInfo();
	CString strNode = connectInfo.GetNode();

	// get info about session
	LPSESSIONDATAMIN pSession =(LPSESSIONDATAMIN)GetPTreeItemData()->GetDataPtr();
	TCHAR szSessName[100];
	CTreeCtrl* pTreeCtrl = GetPTreeGlobData()->GetPTree();
	IPM_GetInfoName(GetPTreeGlobData()->GetDocument(), OT_MON_SESSION, pSession, szSessName, sizeof(szSessName));

	// confirm or cancel
	CString capt;
	capt.Format(IDS_KILLSESSION_CONFIRM_CAPT, (LPCTSTR)strNode);
	CString txt;
	txt.Format(IDS_KILLSESSION_CONFIRM_TXT, szSessName);
	int ret = MessageBox(GetFocus(), txt, capt, MB_YESNO | MB_ICONQUESTION);
	if (ret == IDYES)
	{
		bRet = IPM_RemoveSession(pDoc, pSession);
	}
	return bRet;
}



/* ------------------------------------MONTREE4------------------------------------------*/

IMPLEMENT_SERIAL (CuTMLockinfoStatic, CTreeItem, 1)

CuTMLockinfoStatic::CuTMLockinfoStatic (CTreeGlobalData* pTreeGlobalData)
	:CTreeItem (pTreeGlobalData, 0, IDS_TM_LOCKINFO, SUBBRANCH_KIND_STATIC, IDB_TM_LOCKINFO, 0)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CuTMLockinfoStatic::CuTMLockinfoStatic()
{
}

BOOL CuTMLockinfoStatic::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();

	CuTMLockinfoLocklistStatic  *pBranch1 = new CuTMLockinfoLocklistStatic(pGlobalData, GetPTreeItemData());
	// Fnn request April 17, 97: replace resource branch with 4 specialized branches
	//CuTMLockinfoResourceStatic  *pBranch2 = new CuTMLockinfoResourceStatic(pGlobalData, GetPTreeItemData());
	CuTMLockinfoResDbStatic     *pBranch3 = new CuTMLockinfoResDbStatic(pGlobalData, GetPTreeItemData());
	CuTMLockinfoResTblStatic    *pBranch4 = new CuTMLockinfoResTblStatic(pGlobalData, GetPTreeItemData());
	CuTMLockinfoResPgStatic     *pBranch5 = new CuTMLockinfoResPgStatic(pGlobalData, GetPTreeItemData());
	CuTMLockinfoResOthStatic    *pBranch6 = new CuTMLockinfoResOthStatic(pGlobalData, GetPTreeItemData());

	pBranch1->CreateTreeLine (hParentItem);
	//pBranch2->CreateTreeLine (hParentItem);
	pBranch3->CreateTreeLine (hParentItem);
	pBranch4->CreateTreeLine (hParentItem);
	pBranch5->CreateTreeLine (hParentItem);
	pBranch6->CreateTreeLine (hParentItem);

	return TRUE;
}


/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMLockinfoLocklistStatic, CTreeItem, 1)

CuTMLockinfoLocklistStatic::CuTMLockinfoLocklistStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, 0, IDS_TM_LOCKINFO_LOCKLIST, SUBBRANCH_KIND_OBJ, IDB_TM_LOCKLIST, pItemData, OT_MON_LOCKLIST, IDS_TM_LOCKINFO_LOCKLIST_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMLockinfoLocklistStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMLocklist* pBranch = new CuTMLocklist (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMLockinfoLocklistStatic::CuTMLockinfoLocklistStatic()
{
}

BOOL CuTMLockinfoLocklistStatic::FilterApplies(FilterCause because)
{
	switch (because) {
		case FILTER_SYSTEM_LOCK_LISTS:
			return TRUE;
		default:
			return CTreeItem::FilterApplies(because);
	}
	ASSERT (FALSE);
}

//
// We must know all information how to display 
// the property of this object: how many pages, and what they are ...
CuPageInformation* CuTMLockinfoStatic::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [3] = 
	{
		IDS_TAB_SUMMARY,
		IDS_TAB_LOCKLIST,
		IDS_TAB_LOCKRESOURCE
	};
	UINT nDlgID [3] = 
	{
		IDD_IPMDETAIL_LOCKINFO, 
		IDD_IPMPAGE_LOCKLISTS,
		IDD_IPMPAGE_LOCKEDRESOURCES
	};
	UINT nIDTitle = IDS_TAB_LOCKINFO_TITLE;

	m_pPageInfo = new CuPageInformation (_T("CuTMLockinfoStatic"), 3, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}


/* ------------------------------------------------------------------------------*/


IMPLEMENT_SERIAL (CuTMResourceDbLockStatic, CTreeItem, 1)


CuTMResourceDbLockStatic::CuTMResourceDbLockStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_RES_DB, IDS_TM_RES_DBLOCK, SUBBRANCH_KIND_OBJ, IDB_TM_LOCK, pItemData, OT_MON_LOCK, IDS_TM_RES_DBLOCK_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMResourceDbLockStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMLock* pBranch = new CuTMLock (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMResourceDbLockStatic::CuTMResourceDbLockStatic()
{
}


/* ------------------------------------------------------------------------------*/


IMPLEMENT_SERIAL (CuTMResourceTblLockStatic, CTreeItem, 1)

CuTMResourceTblLockStatic::CuTMResourceTblLockStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_RES_TABLE, IDS_TM_RES_TBLLOCK, SUBBRANCH_KIND_OBJ, IDB_TM_LOCK, pItemData, OT_MON_LOCK, IDS_TM_RES_TBLLOCK_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMResourceTblLockStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMLock* pBranch = new CuTMLock (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMResourceTblLockStatic::CuTMResourceTblLockStatic()
{
}

/* ------------------------------------------------------------------------------*/


IMPLEMENT_SERIAL (CuTMResourcePgLockStatic, CTreeItem, 1)

CuTMResourcePgLockStatic::CuTMResourcePgLockStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_RES_PAGE, IDS_TM_RES_PGLOCK, SUBBRANCH_KIND_OBJ, IDB_TM_LOCK, pItemData, OT_MON_LOCK, IDS_TM_RES_PGLOCK_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMResourcePgLockStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMLock* pBranch = new CuTMLock (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMResourcePgLockStatic::CuTMResourcePgLockStatic()
{
}


/* ------------------------------------------------------------------------------*/


IMPLEMENT_SERIAL (CuTMResourceOthLockStatic, CTreeItem, 1)

CuTMResourceOthLockStatic::CuTMResourceOthLockStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_RES_OTHER, IDS_TM_RES_OTHLOCK, SUBBRANCH_KIND_OBJ, IDB_TM_LOCK, pItemData, OT_MON_LOCK, IDS_TM_RES_OTHLOCK_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMResourceOthLockStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMLock* pBranch = new CuTMLock (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMResourceOthLockStatic::CuTMResourceOthLockStatic()
{
}

/* ------------------------------------------------------------------------------*/


IMPLEMENT_SERIAL (CuTMSessTblStatic, CTreeItem, 1)

CuTMSessTblStatic::CuTMSessTblStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_SESSION, IDS_TM_SESS_TBL, SUBBRANCH_KIND_OBJ, IDB_TM_TABLE, pItemData, OT_MON_RES_TABLE, IDS_TM_SESS_TBL_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMSessTblStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMTbl* pBranch = new CuTMTbl (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMSessTblStatic::CuTMSessTblStatic()
{
}

/* ------------------------------------------------------------------------------*/


IMPLEMENT_SERIAL (CuTMSessPgStatic, CTreeItem, 1)

CuTMSessPgStatic::CuTMSessPgStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_SESSION, IDS_TM_SESS_PAGE, SUBBRANCH_KIND_OBJ, IDB_TM_PAGE, pItemData, OT_MON_RES_PAGE, IDS_TM_SESS_PAGE_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMSessPgStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMPg* pBranch = new CuTMPg (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMSessPgStatic::CuTMSessPgStatic()
{
}


/* ------------------------------------------------------------------------------*/


IMPLEMENT_SERIAL (CuTMSessOthStatic, CTreeItem, 1)

CuTMSessOthStatic::CuTMSessOthStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_SESSION, IDS_TM_SESS_OTHER, SUBBRANCH_KIND_OBJ, IDB_TM_OTHER, pItemData, OT_MON_RES_OTHER, IDS_TM_SESS_OTHER_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMSessOthStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMOth* pBranch = new CuTMOth (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMSessOthStatic::CuTMSessOthStatic()
{
}

BOOL CuTMSessOthStatic::FilterApplies(FilterCause because)
{
	switch (because) {
		case FILTER_RESOURCE_TYPE:
		case FILTER_NULL_RESOURCES:
			return TRUE;
		default:
			return CTreeItem::FilterApplies(because);
	}
	ASSERT (FALSE);
}

/* ------------------------------------------------------------------------------*/


IMPLEMENT_SERIAL (CuTMTblStatic, CTreeItem, 1)
IMPLEMENT_SERIAL (CuTMTbl, CTreeItem, 1)

CuTMTblStatic::CuTMTblStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_SERVER, IDS_TM_TBL, SUBBRANCH_KIND_OBJ, IDB_TM_TABLE, pItemData, OT_MON_RES_TABLE, IDS_TM_TBL_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMTblStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMTbl* pBranch = new CuTMTbl (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMTblStatic::CuTMTblStatic()
{
}

CuTMTbl::CuTMTbl (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_RES_TABLE, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);;
	InitializeImageId(IDB_TM_TABLE);
}

CuTMTbl::CuTMTbl()
{
}

BOOL CuTMTbl::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();
	CuTMResourceTblLockStatic *pBranch1 = new CuTMResourceTblLockStatic(pGlobalData, GetPTreeItemData());
	pBranch1->CreateTreeLine (hParentItem);
	return TRUE;
}

//
// Emb April 4, 97: Duplicated from CuTMLockinfoResource::GetPageInformation()
// Changed April 22 at Fnn request: only 2 tabs (detail and locks)
CuPageInformation* CuTMTbl::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [2] = 
	{
		IDS_TAB_DETAIL, 
		IDS_TAB_LOCK,
	};
	UINT nDlgID [2] = 
	{
		IDD_IPMDETAIL_RESOURCE, 
		IDD_IPMPAGE_LOCKS,
	};
	UINT nIDTitle = IDS_TAB_RESOURCE_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMTbl"), 2, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}


/* ------------------------------------------------------------------------------*/


IMPLEMENT_SERIAL (CuTMPgStatic, CTreeItem, 1)
IMPLEMENT_SERIAL (CuTMPg, CTreeItem, 1)

CuTMPgStatic::CuTMPgStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_SERVER, IDS_TM_PAGE, SUBBRANCH_KIND_OBJ, IDB_TM_PAGE, pItemData, OT_MON_RES_PAGE, IDS_TM_PAGE_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMPgStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMPg* pBranch = new CuTMPg (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMPgStatic::CuTMPgStatic()
{
}

CuTMPg::CuTMPg (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_RES_PAGE, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);;
	InitializeImageId(IDB_TM_PAGE);
}

CuTMPg::CuTMPg()
{
}

BOOL CuTMPg::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();
	CuTMResourcePgLockStatic *pBranch1 = new CuTMResourcePgLockStatic(pGlobalData, GetPTreeItemData());
	pBranch1->CreateTreeLine (hParentItem);
	return TRUE;
}


//
// Emb April 4, 97: Duplicated from CuTMLockinfoResource::GetPageInformation()
// Changed April 22 at Fnn request: only 2 tabs (detail and locks)
CuPageInformation* CuTMPg::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [2] = 
	{
		IDS_TAB_DETAIL, 
		IDS_TAB_LOCK,
	};
	UINT nDlgID [2] = 
	{
		IDD_IPMDETAIL_RESOURCE, 
		IDD_IPMPAGE_LOCKS,
	};
	UINT nIDTitle = IDS_TAB_RESOURCE_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMPg"), 2, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}


/* ------------------------------------------------------------------------------*/


IMPLEMENT_SERIAL (CuTMOthStatic, CTreeItem, 1)
IMPLEMENT_SERIAL (CuTMOth, CTreeItem, 1)

CuTMOthStatic::CuTMOthStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_SERVER, IDS_TM_OTHER, SUBBRANCH_KIND_OBJ, IDB_TM_OTHER, pItemData, OT_MON_RES_OTHER, IDS_TM_OTHER_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMOthStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMOth* pBranch = new CuTMOth (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMOthStatic::CuTMOthStatic()
{
}

BOOL CuTMOthStatic::FilterApplies(FilterCause because)
{
	switch (because) {
		case FILTER_RESOURCE_TYPE:
		case FILTER_NULL_RESOURCES:
			return TRUE;
		default:
			return CTreeItem::FilterApplies(because);
	}
	ASSERT (FALSE);
}

CuTMOth::CuTMOth (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_RES_OTHER, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);;
	InitializeImageId(IDB_TM_OTHER);
}

CuTMOth::CuTMOth()
{
}

BOOL CuTMOth::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();
	CuTMResourceOthLockStatic *pBranch1 = new CuTMResourceOthLockStatic(pGlobalData, GetPTreeItemData());
	pBranch1->CreateTreeLine (hParentItem);
	return TRUE;
}


//
// Emb April 4, 97: Duplicated from CuTMLockinfoResource::GetPageInformation()
// Changed April 22 at Fnn request: only 2 tabs (detail and locks)
CuPageInformation* CuTMOth::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [2] = 
	{
		IDS_TAB_DETAIL, 
		IDS_TAB_LOCK,
	};
	UINT nDlgID [2] = 
	{
		IDD_IPMDETAIL_RESOURCE, 
		IDD_IPMPAGE_LOCKS,
	};
	UINT nIDTitle = IDS_TAB_RESOURCE_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMOth"), 2, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}


/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMLogDbLocklistStatic, CuTMDbLocklistStatic, 1)

CuTMLogDbLocklistStatic::CuTMLogDbLocklistStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CuTMDbLocklistStatic(pTreeGlobalData, pItemData)
{
	SetType(OT_MON_LOGDATABASE);    // since derived from CuTMDbLocklistStatic
	SetCompleteType(GetType());     // propagate the right type
	SetContextMenuId(IDR_POPUP_IPM);;
}


CuTMLogDbLocklistStatic::CuTMLogDbLocklistStatic()
{
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMLogDbLockStatic, CuTMDbLockStatic, 1)

CuTMLogDbLockStatic::CuTMLogDbLockStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CuTMDbLockStatic(pTreeGlobalData, pItemData)
{
	SetType(OT_MON_LOGDATABASE);    // since derived from CuTMDbLockStatic
	SetCompleteType(GetType());     // propagate the right type
	SetContextMenuId(IDR_POPUP_IPM);;
}

CuTMLogDbLockStatic::CuTMLogDbLockStatic()
{
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMLogDbLockedTblStatic, CuTMDbLockedTblStatic, 1)

CuTMLogDbLockedTblStatic::CuTMLogDbLockedTblStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CuTMDbLockedTblStatic(pTreeGlobalData, pItemData)
{
	SetType(OT_MON_LOGDATABASE);    // since derived from CuTMDbLockedTblStatic
	SetCompleteType(GetType());     // propagate the right type
	SetContextMenuId(IDR_POPUP_IPM);;
}

CuTMLogDbLockedTblStatic::CuTMLogDbLockedTblStatic ()
{
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMLogDbLockedPgStatic, CuTMDbLockedPgStatic, 1)

CuTMLogDbLockedPgStatic::CuTMLogDbLockedPgStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CuTMDbLockedPgStatic(pTreeGlobalData, pItemData)
{
	SetType(OT_MON_LOGDATABASE);    // since derived from CuTMDbLockedPgStatic
	SetCompleteType(GetType());     // propagate the right type
	SetContextMenuId(IDR_POPUP_IPM);;
}

CuTMLogDbLockedPgStatic::CuTMLogDbLockedPgStatic()
{
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMLogDbLockedOtherStatic, CuTMDbLockedOtherStatic, 1)

CuTMLogDbLockedOtherStatic::CuTMLogDbLockedOtherStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CuTMDbLockedOtherStatic(pTreeGlobalData, pItemData)
{
	SetType(OT_MON_LOGDATABASE);    // since derived from CuTMDbLockedOtherStatic
	SetCompleteType(GetType());     // propagate the right type
	SetContextMenuId(IDR_POPUP_IPM);;
}

CuTMLogDbLockedOtherStatic::CuTMLogDbLockedOtherStatic()
{
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMLogDbServerStatic, CuTMDbServerStatic, 1)

CuTMLogDbServerStatic::CuTMLogDbServerStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CuTMDbServerStatic(pTreeGlobalData, pItemData)
{
	SetType(OT_MON_LOGDATABASE);    // since derived from CuTMDbServerStatic
	SetCompleteType(GetType());     // propagate the right type
	SetContextMenuId(IDR_POPUP_IPM);;
}

CuTMLogDbServerStatic::CuTMLogDbServerStatic()
{
}


/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMLogDbTransacStatic, CuTMDbTransacStatic, 1)

CuTMLogDbTransacStatic::CuTMLogDbTransacStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CuTMDbTransacStatic(pTreeGlobalData, pItemData)
{
	SetType(OT_MON_LOGDATABASE);    // since derived from CuTMDbTransacStatic
	SetCompleteType(GetType());     // propagate the right type
	SetContextMenuId(IDR_POPUP_IPM);;
}

CuTMLogDbTransacStatic::CuTMLogDbTransacStatic()
{
}


/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMLogDbSessionStatic, CuTMDbSessionStatic, 1)

CuTMLogDbSessionStatic::CuTMLogDbSessionStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CuTMDbSessionStatic(pTreeGlobalData, pItemData)
{
	SetType(OT_MON_LOGDATABASE);    // since derived from CuTMDbSessionStatic
	SetCompleteType(GetType());     // propagate the right type
	SetContextMenuId(IDR_POPUP_IPM);;
}

CuTMLogDbSessionStatic::CuTMLogDbSessionStatic()
{
}


/* ------------------------------------------------------------------------------*/


IMPLEMENT_SERIAL (CuTMLocklistSessionStatic, CTreeItem, 1)

CuTMLocklistSessionStatic::CuTMLocklistSessionStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_LOCKLIST, IDS_TM_LOCKLIST_SESSION, SUBBRANCH_KIND_OBJ, IDB_TM_SESSION, pItemData, OT_MON_SESSION, IDS_TM_LOCKLIST_SESSION_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMLocklistSessionStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMSession* pBranch = new CuTMSession (GetPTreeGlobData(), pItemData);
	return pBranch;
}

BOOL CuTMLocklistSessionStatic::FilterApplies(FilterCause because)
{
	switch (because) 
	{
	case FILTER_INTERNAL_SESSIONS:
		return TRUE;
	default:
		return CTreeItem::FilterApplies(because);
	}
	ASSERT (FALSE);
}

CuTMLocklistSessionStatic::CuTMLocklistSessionStatic()
{
}


/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMLockinfoResDbStatic, CTreeItem, 1)

CuTMLockinfoResDbStatic::CuTMLockinfoResDbStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, 0, IDS_TM_LOCKINFO_RESDB, SUBBRANCH_KIND_OBJ, IDB_TM_LOCKEDDB, pItemData, OT_MON_RES_DB, IDS_TM_LOCKINFO_RESDB_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMLockinfoResDbStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMDb* pBranch = new CuTMDb (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMLockinfoResDbStatic::CuTMLockinfoResDbStatic()
{
}


/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMLockinfoResTblStatic, CTreeItem, 1)

CuTMLockinfoResTblStatic::CuTMLockinfoResTblStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, 0, IDS_TM_LOCKINFO_RESTBL, SUBBRANCH_KIND_OBJ, IDB_TM_LOCKEDTBL, pItemData, OT_MON_RES_TABLE, IDS_TM_LOCKINFO_RESTBL_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMLockinfoResTblStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMTbl* pBranch = new CuTMTbl (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMLockinfoResTblStatic::CuTMLockinfoResTblStatic()
{
}


/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMLockinfoResPgStatic, CTreeItem, 1)

CuTMLockinfoResPgStatic::CuTMLockinfoResPgStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, 0, IDS_TM_LOCKINFO_RESPG, SUBBRANCH_KIND_OBJ, IDB_TM_LOCKEDPAGE, pItemData, OT_MON_RES_PAGE, IDS_TM_LOCKINFO_RESPG_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMLockinfoResPgStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMPg* pBranch = new CuTMPg (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMLockinfoResPgStatic::CuTMLockinfoResPgStatic()
{
}

/* ------------------------------------------------------------------------------*/

IMPLEMENT_SERIAL (CuTMLockinfoResOthStatic, CTreeItem, 1)

CuTMLockinfoResOthStatic::CuTMLockinfoResOthStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, 0, IDS_TM_LOCKINFO_RESOTH, SUBBRANCH_KIND_OBJ, IDB_TM_LOCKEDOTHER, pItemData, OT_MON_RES_OTHER, IDS_TM_LOCKINFO_RESOTH_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);;
}

CTreeItem* CuTMLockinfoResOthStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMOth* pBranch = new CuTMOth (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMLockinfoResOthStatic::CuTMLockinfoResOthStatic()
{
}

BOOL CuTMLockinfoResOthStatic::FilterApplies(FilterCause because)
{
	switch (because)
	{
	case FILTER_RESOURCE_TYPE:
	case FILTER_NULL_RESOURCES:
		return TRUE;
	default:
		return CTreeItem::FilterApplies(because);
	}
	ASSERT (FALSE);
}
