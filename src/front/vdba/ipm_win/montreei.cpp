/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : montreei.cpp : implementation file
**    Project  : CA-OpenIngres/Monitoring.
**    Author   : Emmanuel Blattes
**    Purpose  : Classes for the ICE specific branches in the monitor tree control 
**
** History:
**
** 11-Sep-1998 (Emmanuel Blattes)
**    Created
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "montreei.h" 
#include "ipmdisp.h" // Definition of CuPageInformation
#include "ipmdoc.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// CLASS: CuTMIceServer
// ************************************************************************************************
IMPLEMENT_SERIAL (CuTMIceServer, CTreeItem, 5)
CuTMIceServer::CuTMIceServer (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_SERVER, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	// FNN REQUEST Nov 6, 98: always show all items whatever server accepts to be closed/opened or not
	SetContextMenuId(IDR_POPUP_IPM_SERVER);
	InitializeImageId(IDB_TM_SERVER);
}


BOOL CuTMIceServer::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();

	CuTMIceConnUsrStatic    *pBranch1 = new CuTMIceConnUsrStatic(pGlobalData, GetPTreeItemData());
	CuTMIceActUsrStatic     *pBranch2 = new CuTMIceActUsrStatic(pGlobalData, GetPTreeItemData());
	CuTMIceFIAllStatic      *pBranch3 = new CuTMIceFIAllStatic(pGlobalData, GetPTreeItemData());
	CuTMIceHttpConnStatic   *pBranch4 = new CuTMIceHttpConnStatic(pGlobalData, GetPTreeItemData());
	CuTMIceDbConnStatic     *pBranch5 = new CuTMIceDbConnStatic(pGlobalData, GetPTreeItemData());
	CuTMIceAllTransacStatic *pBranch6 = new CuTMIceAllTransacStatic(pGlobalData, GetPTreeItemData());
	CuTMIceAllCursorStatic  *pBranch7 = new CuTMIceAllCursorStatic(pGlobalData, GetPTreeItemData());

	pBranch1->CreateTreeLine (hParentItem);
	pBranch2->CreateTreeLine (hParentItem);
	pBranch3->CreateTreeLine (hParentItem);
	pBranch4->CreateTreeLine (hParentItem);
	pBranch5->CreateTreeLine (hParentItem);
	pBranch6->CreateTreeLine (hParentItem);
	pBranch7->CreateTreeLine (hParentItem);

	return TRUE;
}

CuTMIceServer::CuTMIceServer()
{
	m_pPageInfo = NULL;
}

UINT CuTMIceServer::GetCustomImageId()
{
	LPSERVERDATAMIN lps =(LPSERVERDATAMIN)GetPTreeItemData()->GetDataPtr();
	return GetServerImageId(lps);
}

BOOL CuTMIceServer::IsEnabled(UINT idMenu)
{
	switch (idMenu) {
		case IDM_IPMBAR_SHUTDOWN:   // KillShutdown
			if (IsNoItem() || IsErrorItem())
				return FALSE;
			return TRUE;
		default:
			return CTreeItem::IsEnabled(idMenu);
	}
	return FALSE;   // security if return forgotten in switch
}

BOOL CuTMIceServer::KillShutdown()
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
		bRet = IPM_ShutDownServer(pDoc, pServer, bSoft);
	}
	return bRet;
}


CuPageInformation* CuTMIceServer::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [8] = 
	{
		IDS_TAB_DETAIL,
		IDS_TAB_ICE_CONNECTEDUSER,
		IDS_TAB_ICE_ACTIVEUSER,
		IDS_TAB_ICE_CACHEPAGE,
		IDS_TAB_ICE_HTTP_SERVER,
		IDS_TAB_ICE_DATABASECONNECTION,
		IDS_TAB_ICE_TRANSACTION,
		IDS_TAB_ICE_CURSOR
	};
	UINT nDlgID [8] = 
	{
		IDD_IPMDETAIL_SERVER,
		IDD_IPMICEPAGE_CONNECTEDUSER,
		IDD_IPMICEPAGE_ACTIVEUSER,
		IDD_IPMICEPAGE_CACHEPAGE,
		IDD_IPMICEPAGE_HTTPSERVERCONNECTION,
		IDD_IPMICEPAGE_DATABASECONNECTION,
		IDD_IPMICEPAGE_TRANSACTION,
		IDD_IPMICEPAGE_CURSOR
	};
	UINT nIDTitle = IDS_TAB_SERVER_TITLE;
	m_pPageInfo = new CuPageInformation (_T("CuTMIceServer"), 8, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

//
// CLASS: CuTMIceActUsrStatic, CuTMIceActUsr
// ************************************************************************************************
IMPLEMENT_SERIAL (CuTMIceActUsrStatic, CTreeItem, 1)
IMPLEMENT_SERIAL (CuTMIceActUsr, CTreeItem, 1)

CuTMIceActUsrStatic::CuTMIceActUsrStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_SERVER, IDS_TM_ICE_ACTIVE_USER, SUBBRANCH_KIND_OBJ, IDB_TM_ICE_ACTIVE_USER, pItemData, OT_MON_ICE_ACTIVE_USER, IDS_TM_ICE_ACTIVE_USER_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMIceActUsrStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMIceActUsr* pBranch = new CuTMIceActUsr (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMIceActUsrStatic::CuTMIceActUsrStatic()
{
}


CuPageInformation* CuTMIceActUsrStatic::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = {IDS_TAB_ICE_ACTIVEUSER};
	UINT nDlgID [1] = {IDD_IPMICEPAGE_ACTIVEUSER};
	UINT nIDTitle = IDS_TAB_ICE_TITLE_STATIC_ACTIVEUSER;
	m_pPageInfo = new CuPageInformation (_T("CuTMIceActUsrStatic"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}


CuTMIceActUsr::CuTMIceActUsr (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_ICE_ACTIVE_USER, SUBBRANCH_KIND_NONE, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);
	InitializeImageId(IDB_TM_ICE_ACTIVE_USER);
}

CuTMIceActUsr::CuTMIceActUsr()
{
}

CuPageInformation* CuTMIceActUsr::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = {IDS_TAB_DETAIL};
	UINT nDlgID [1] = {IDD_IPMICEDETAIL_ACTIVEUSER};
	UINT nIDTitle = IDS_TAB_ICE_TITLE_ACTIVEUSER;
	m_pPageInfo = new CuPageInformation (_T("CuTMIceActUsr"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

//
// CLASS: CuTMIceHttpConnStatic, CuTMIceHttpConn
// ************************************************************************************************
IMPLEMENT_SERIAL (CuTMIceHttpConnStatic, CTreeItem, 1)
IMPLEMENT_SERIAL (CuTMIceHttpConn, CTreeItem, 1)

CuTMIceHttpConnStatic::CuTMIceHttpConnStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_SERVER, IDS_TM_ICE_HTTP_CONNECTION, SUBBRANCH_KIND_OBJ, IDB_TM_ICE_HTTP_CONNECTION, pItemData, OT_MON_SESSION, IDS_TM_ICE_HTTP_CONNECTION_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMIceHttpConnStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMIceHttpConn* pBranch = new CuTMIceHttpConn (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMIceHttpConnStatic::CuTMIceHttpConnStatic()
{
}

CuPageInformation* CuTMIceHttpConnStatic::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = {IDS_TAB_ICE_HTTP_SERVER};
	UINT nDlgID [1] = {IDD_IPMICEPAGE_HTTPSERVERCONNECTION};
	UINT nIDTitle = IDS_TAB_ICE_TITLE_STATIC_HTTP_SERVER;
	m_pPageInfo = new CuPageInformation (_T("CuTMIceHttpConnStatic"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

CuTMIceHttpConn::CuTMIceHttpConn (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_SESSION, SUBBRANCH_KIND_NONE, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);
	InitializeImageId(IDB_TM_ICE_HTTP_CONNECTION);
}

CuTMIceHttpConn::CuTMIceHttpConn()
{
}

CuPageInformation* CuTMIceHttpConn::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = {IDS_TAB_DETAIL};
	UINT nDlgID [1] = {IDD_IPMICEDETAIL_HTTPSERVERCONNECTION};
	UINT nIDTitle = IDS_TAB_ICE_TITLE_HTTP_SERVER;
	m_pPageInfo = new CuPageInformation (_T("CuTMIceHttpConn"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

//
// CLASS: CuTMIceDbConnStatic, CuTMIceDbConn
// ************************************************************************************************
IMPLEMENT_SERIAL (CuTMIceDbConnStatic, CTreeItem, 1)
IMPLEMENT_SERIAL (CuTMIceDbConn, CTreeItem, 1)

CuTMIceDbConnStatic::CuTMIceDbConnStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_SERVER, IDS_TM_ICE_DB_CONNECTION, SUBBRANCH_KIND_OBJ, IDB_TM_ICE_DB_CONNECTION, pItemData, OT_MON_ICE_DB_CONNECTION, IDS_TM_ICE_DB_CONNECTION_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMIceDbConnStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMIceDbConn* pBranch = new CuTMIceDbConn (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMIceDbConnStatic::CuTMIceDbConnStatic()
{
}

CuPageInformation* CuTMIceDbConnStatic::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = {IDS_TAB_ICE_DATABASECONNECTION};
	UINT nDlgID [1] = {IDD_IPMICEPAGE_DATABASECONNECTION};
	UINT nIDTitle = IDS_TAB_ICE_TITLE_STATIC_DATABASECONNECTION;
	m_pPageInfo = new CuPageInformation (_T("CuTMIceDbConnStatic"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

CuTMIceDbConn::CuTMIceDbConn (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_ICE_DB_CONNECTION, SUBBRANCH_KIND_NONE, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);
	InitializeImageId(IDB_TM_ICE_DB_CONNECTION);
}

CuTMIceDbConn::CuTMIceDbConn()
{
}

CuPageInformation* CuTMIceDbConn::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = {IDS_TAB_DETAIL};
	UINT nDlgID [1] = {IDD_IPMICEDETAIL_DATABASECONNECTION};
	UINT nIDTitle = IDS_TAB_ICE_TITLE_DATABASECONNECTION;
	m_pPageInfo = new CuPageInformation (_T("CuTMIceDbConn"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

//
// CLASS: CuTMIceFIAllStatic, CuTMIceFIAll (File info for All)
// ************************************************************************************************
IMPLEMENT_SERIAL (CuTMIceFIAllStatic, CTreeItem, 1)
IMPLEMENT_SERIAL (CuTMIceFIAll, CTreeItem, 1)
CuTMIceFIAllStatic::CuTMIceFIAllStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_SERVER, IDS_TM_ICE_FILEINFO_ALL, SUBBRANCH_KIND_OBJ, IDB_TM_ICE_DB_CONNECTION, pItemData, OT_MON_ICE_FILEINFO, IDS_TM_ICE_FILEINFO_ALL_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMIceFIAllStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMIceFIAll* pBranch = new CuTMIceFIAll (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMIceFIAllStatic::CuTMIceFIAllStatic()
{
}

CuPageInformation* CuTMIceFIAllStatic::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = {IDS_TAB_ICE_CACHEPAGE};
	UINT nDlgID [1] = {IDD_IPMICEPAGE_CACHEPAGE};
	UINT nIDTitle = IDS_TAB_ICE_TITLE_STATIC_CACHEPAGE;
	m_pPageInfo = new CuPageInformation (_T("CuTMIceFIAllStatic"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

CuTMIceFIAll::CuTMIceFIAll (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_ICE_FILEINFO, SUBBRANCH_KIND_NONE, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);
	InitializeImageId(IDB_TM_ICE_FILEINFO_ALL);
}

CuTMIceFIAll::CuTMIceFIAll()
{
}

CuPageInformation* CuTMIceFIAll::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = {IDS_TAB_DETAIL};
	UINT nDlgID [1] = {IDD_IPMICEDETAIL_CACHEPAGE};
	UINT nIDTitle = IDS_TAB_ICE_TITLE_CACHEPAGE;
	m_pPageInfo = new CuPageInformation (_T("CuTMIceFIAll"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

//
// CLASS: CuTMIceConnUsr, CuTMIceConnUsr
// ************************************************************************************************
IMPLEMENT_SERIAL (CuTMIceConnUsrStatic, CTreeItem, 1)
IMPLEMENT_SERIAL (CuTMIceConnUsr, CTreeItem, 2)

CuTMIceConnUsrStatic::CuTMIceConnUsrStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_SERVER, IDS_TM_ICE_CONNECTED_USER, SUBBRANCH_KIND_OBJ, IDB_TM_ICE_CONNECTED_USER, pItemData, OT_MON_ICE_CONNECTED_USER, IDS_TM_ICE_CONNECTED_USER_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMIceConnUsrStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMIceConnUsr* pBranch = new CuTMIceConnUsr (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMIceConnUsrStatic::CuTMIceConnUsrStatic()
{
}

CuPageInformation* CuTMIceConnUsrStatic::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = {IDS_TAB_ICE_CONNECTEDUSER};
	UINT nDlgID [1] = {IDD_IPMICEPAGE_CONNECTEDUSER};
	UINT nIDTitle = IDS_TAB_ICE_TITLE_STATIC_CONNECTEDUSER;
	m_pPageInfo = new CuPageInformation (_T("CuTMIceConnUsrStatic"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

CuTMIceConnUsr::CuTMIceConnUsr (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_ICE_CONNECTED_USER, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);
	InitializeImageId(IDB_TM_ICE_CONNECTED_USER);
}

CuTMIceConnUsr::CuTMIceConnUsr()
{
}

BOOL CuTMIceConnUsr::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();
	CuTMIceTransacStatic      *pBranch1 = new CuTMIceTransacStatic (pGlobalData, GetPTreeItemData());
	CuTMIceFIUserStatic       *pBranch2 = new CuTMIceFIUserStatic (pGlobalData, GetPTreeItemData());
	pBranch1->CreateTreeLine (hParentItem);
	pBranch2->CreateTreeLine (hParentItem);
	return TRUE;
}

CuPageInformation* CuTMIceConnUsr::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = {IDS_TAB_DETAIL};
	UINT nDlgID [1] = {IDD_IPMICEDETAIL_CONNECTEDUSER};
	UINT nIDTitle = IDS_TAB_ICE_TITLE_CONNECTEDUSER;
	m_pPageInfo = new CuPageInformation (_T("CuTMIceConnUsr"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

//
// CLASS: CuTMIceTransacStatic, CuTMIceTransac
// ************************************************************************************************
IMPLEMENT_SERIAL (CuTMIceTransacStatic, CTreeItem, 1)
IMPLEMENT_SERIAL (CuTMIceTransac, CTreeItem, 1)

CuTMIceTransacStatic::CuTMIceTransacStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_ICE_CONNECTED_USER, IDS_TM_ICE_TRANSACTION, SUBBRANCH_KIND_OBJ, IDB_TM_ICE_TRANSACTION, pItemData, OT_MON_ICE_TRANSACTION, IDS_TM_ICE_TRANSACTION_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMIceTransacStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMIceTransac* pBranch = new CuTMIceTransac (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMIceTransacStatic::CuTMIceTransacStatic()
{
}

CuPageInformation* CuTMIceTransacStatic::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = {IDS_TAB_DETAIL};
	UINT nDlgID [1] = {IDD_IPMICEDETAIL_TRANSACTION};
	UINT nIDTitle = IDS_TAB_ICE_TITLE_STATIC_TRANSACTION;
	m_pPageInfo = new CuPageInformation (_T("CuTMIceTransacStatic"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

CuTMIceTransac::CuTMIceTransac (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_ICE_TRANSACTION, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);
	InitializeImageId(IDB_TM_ICE_TRANSACTION);
}

CuTMIceTransac::CuTMIceTransac()
{
}

BOOL CuTMIceTransac::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();
	CuTMIceCursorStatic      *pBranch1 = new CuTMIceCursorStatic (pGlobalData, GetPTreeItemData());
	pBranch1->CreateTreeLine (hParentItem);
	return TRUE;
}

CuPageInformation* CuTMIceTransac::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = {IDS_TAB_ICE_TRANSACTION};
	UINT nDlgID [1] = {IDD_IPMICEPAGE_TRANSACTION};
	UINT nIDTitle = IDS_TAB_ICE_TITLE_TRANSACTION;
	m_pPageInfo = new CuPageInformation (_T("CuTMIceTransac"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

//
// CLASS: CuTMIceCursorStatic, CuTMIceCursor
// ************************************************************************************************
IMPLEMENT_SERIAL (CuTMIceCursorStatic, CTreeItem, 1)
IMPLEMENT_SERIAL (CuTMIceCursor, CTreeItem, 1)

CuTMIceCursorStatic::CuTMIceCursorStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_ICE_TRANSACTION, IDS_TM_ICE_CURSOR, SUBBRANCH_KIND_OBJ, IDB_TM_ICE_CURSOR, pItemData, OT_MON_ICE_CURSOR, IDS_TM_ICE_CURSOR_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMIceCursorStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMIceCursor* pBranch = new CuTMIceCursor (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMIceCursorStatic::CuTMIceCursorStatic()
{
}

CuPageInformation* CuTMIceCursorStatic::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = {IDS_TAB_ICE_CURSOR};
	UINT nDlgID [1] = {IDD_IPMICEPAGE_CURSOR};
	UINT nIDTitle = IDS_TAB_ICE_TITLE_STATIC_CURSOR;
	m_pPageInfo = new CuPageInformation (_T("CuTMIceCursorStatic"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

CuTMIceCursor::CuTMIceCursor (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_ICE_CURSOR, SUBBRANCH_KIND_NONE, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);
	InitializeImageId(IDB_TM_ICE_CURSOR);
}

CuTMIceCursor::CuTMIceCursor()
{
}

CuPageInformation* CuTMIceCursor::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = {IDS_TAB_DETAIL};
	UINT nDlgID [1] = {IDD_IPMICEDETAIL_CURSOR};
	UINT nIDTitle = IDS_TAB_ICE_TITLE_CURSOR;
	m_pPageInfo = new CuPageInformation (_T("CuTMIceCursor"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

//
// CLASS: CuTMIceAllTransacStatic, CuTMIceAllTransac
// ************************************************************************************************
IMPLEMENT_SERIAL (CuTMIceAllTransacStatic, CTreeItem, 1)
IMPLEMENT_SERIAL (CuTMIceAllTransac, CTreeItem, 1)

CuTMIceAllTransacStatic::CuTMIceAllTransacStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_SERVER, IDS_TM_ICE_ALLTRANSACTION, SUBBRANCH_KIND_OBJ, IDB_TM_ICE_ALLTRANSACTION, pItemData, OT_MON_ICE_TRANSACTION, IDS_TM_ICE_ALLTRANSACTION_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMIceAllTransacStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMIceAllTransac* pBranch = new CuTMIceAllTransac (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMIceAllTransacStatic::CuTMIceAllTransacStatic()
{
}

CuPageInformation* CuTMIceAllTransacStatic::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = {IDS_TAB_ICE_TRANSACTION};
	UINT nDlgID [1] = {IDD_IPMICEPAGE_TRANSACTION};
	UINT nIDTitle = IDS_TAB_ICE_TITLE_STATIC_CURSOR;
	m_pPageInfo = new CuPageInformation (_T("CuTMIceAllTransacStatic"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

CuTMIceAllTransac::CuTMIceAllTransac (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_ICE_TRANSACTION, SUBBRANCH_KIND_STATIC, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);
	InitializeImageId(IDB_TM_ICE_ALLTRANSACTION);
}

CuTMIceAllTransac::CuTMIceAllTransac()
{
}

BOOL CuTMIceAllTransac::CreateStaticSubBranches (HTREEITEM hParentItem)
{
	CTreeGlobalData* pGlobalData = GetPTreeGlobData();
	CuTMIceCursorStatic      *pBranch1 = new CuTMIceCursorStatic (pGlobalData, GetPTreeItemData());
	pBranch1->CreateTreeLine (hParentItem);
	return TRUE;
}

CuPageInformation* CuTMIceAllTransac::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = {IDS_TAB_DETAIL};
	UINT nDlgID [1] = {IDD_IPMICEDETAIL_TRANSACTION};
	UINT nIDTitle = IDS_TAB_ICE_TITLE_TRANSACTION;
	m_pPageInfo = new CuPageInformation (_T("CuTMIceAllTransac"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

//
// CLASS: CuTMIceFIUserStatic, CuTMIceFIUser (File info for a Connected user)
// ************************************************************************************************
IMPLEMENT_SERIAL (CuTMIceFIUserStatic, CTreeItem, 3)
IMPLEMENT_SERIAL (CuTMIceFIUser, CTreeItem, 2)

CuTMIceFIUserStatic::CuTMIceFIUserStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_ICE_CONNECTED_USER, IDS_TM_ICE_FILEINFO_USER, SUBBRANCH_KIND_OBJ, IDB_TM_ICE_FILEINFO_USER, pItemData, OT_MON_ICE_FILEINFO, IDS_TM_ICE_FILEINFO_USER_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMIceFIUserStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMIceFIUser* pBranch = new CuTMIceFIUser (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMIceFIUserStatic::CuTMIceFIUserStatic()
{
}

CuPageInformation* CuTMIceFIUserStatic::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = {IDS_TAB_ICE_CACHEPAGE};
	UINT nDlgID [1] = {IDD_IPMICEPAGE_CACHEPAGE};
	UINT nIDTitle = IDS_TAB_ICE_TITLE_STATIC_CACHEPAGE;
	m_pPageInfo = new CuPageInformation (_T("CuTMIceFIUserStatic"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

CuTMIceFIUser::CuTMIceFIUser (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_ICE_FILEINFO, SUBBRANCH_KIND_NONE, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);
	InitializeImageId(IDB_TM_ICE_FILEINFO_USER);
}

CuTMIceFIUser::CuTMIceFIUser()
{
}

CuPageInformation* CuTMIceFIUser::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = {IDS_TAB_DETAIL};
	UINT nDlgID [1] = {IDD_IPMICEDETAIL_CACHEPAGE};
	UINT nIDTitle = IDS_TAB_ICE_TITLE_CACHEPAGE;
	m_pPageInfo = new CuPageInformation (_T("CuTMIceFIUser"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

//
// CLASS: CuTMIceAllCursorStatic, CuTMIceAllCursor 
// ************************************************************************************************
IMPLEMENT_SERIAL (CuTMIceAllCursorStatic, CTreeItem, 1)
IMPLEMENT_SERIAL (CuTMIceAllCursor, CTreeItem, 1)
CuTMIceAllCursorStatic::CuTMIceAllCursorStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_SERVER, IDS_TM_ICE_ALLCURSOR, SUBBRANCH_KIND_OBJ, IDB_TM_ICE_ALLCURSOR, pItemData, OT_MON_ICE_CURSOR, IDS_TM_ICE_ALLCURSOR_NONE)
{
	SetContextMenuId(IDR_POPUP_IPM);
}

CTreeItem* CuTMIceAllCursorStatic::AllocateChildItem (CTreeItemData* pItemData)
{
	CuTMIceAllCursor* pBranch = new CuTMIceAllCursor (GetPTreeGlobData(), pItemData);
	return pBranch;
}

CuTMIceAllCursorStatic::CuTMIceAllCursorStatic()
{
}

CuPageInformation* CuTMIceAllCursorStatic::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = {IDS_TAB_ICE_CURSOR};
	UINT nDlgID [1] = {IDD_IPMICEPAGE_CURSOR};
	UINT nIDTitle = IDS_TAB_ICE_TITLE_STATIC_CURSOR;
	m_pPageInfo = new CuPageInformation (_T("CuTMIceAllCursorStatic"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}

CuTMIceAllCursor::CuTMIceAllCursor (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
	:CTreeItem (pTreeGlobalData, OT_MON_ICE_CURSOR, SUBBRANCH_KIND_NONE, FALSE, pItemData)
{
	SetContextMenuId(IDR_POPUP_IPM);
	InitializeImageId(IDB_TM_ICE_ALLCURSOR);
}

CuTMIceAllCursor::CuTMIceAllCursor()
{
}

CuPageInformation* CuTMIceAllCursor::GetPageInformation ()
{
	if (m_pPageInfo)
		return m_pPageInfo;
	UINT nTabID [1] = {IDS_TAB_DETAIL};
	UINT nDlgID [1] = {IDD_IPMICEDETAIL_CURSOR};
	UINT nIDTitle = IDS_TAB_ICE_TITLE_CURSOR;
	m_pPageInfo = new CuPageInformation (_T("CuTMIceAllCursor"), 1, nTabID, nDlgID, nIDTitle);
	return m_pPageInfo;
}


