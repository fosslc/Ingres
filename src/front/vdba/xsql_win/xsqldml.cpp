/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xipmdml.cpp, implementation file 
**    Project  : IPM
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Stand alone Ingres Performance Monitor.
**
** History:
**
** 10-Dec-2001 (uk$so01)
**    Created
** 03-Oct-2003 (uk$so01)
**    SIR #106648, Vdba-Split, Additional fix for GATEWAY Enhancement 
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 20-Apr-2004 (uk$so01)
**    BUG #112171 / ISSUE 13368379.
**    In the vdbasql query text area, type the statement create role R1; 
**    and then Click on GO. 
**    Click any combobox (not node and server), vdbasql will Hang.
*/

#include "stdafx.h"
#include "xsql.h"
#include "xsqldoc.h"
#include "ingobdml.h"
#include "xsqldml.h"
#include "constdef.h"
#include "usrexcep.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL DML_QueryNode(CdSqlQuery* pDoc, CTypedPtrList< CObList, CaDBObject* >& listObj)
{
	BOOL bOK = (VNODE_QueryNode (listObj) == NOERROR)? TRUE: FALSE;
	return bOK;
}


BOOL DML_QueryServer(CdSqlQuery* pDoc, CaNode* pNode, CTypedPtrList< CObList, CaDBObject* >& listObj)
{
	CaNode node (pNode->GetName());
	CString strConnectionString = pDoc->GetConnectInfo();
	if (!strConnectionString.IsEmpty())
		node.SetName(strConnectionString);
	BOOL bOK = (VNODE_QueryServer (&node, listObj) == NOERROR)? TRUE: FALSE;
	return bOK;
}

BOOL DML_QueryUser(CdSqlQuery* pDoc, CTypedPtrList< CObList, CaDBObject* >& listObj)
{
	BOOL bOK = FALSE;
	CaLLQueryInfo queryInfo (OBT_USER, pDoc->GetNode(), _T("iidbdb"));

	CString strDefault;
	CString strNode = pDoc->GetNode();
	CString strServer = pDoc->GetServer();
	CString strUser = pDoc->GetUser();

	strDefault.LoadString(IDS_DEFAULT_SERVER);
	if (strDefault.CompareNoCase(strServer) == 0)
		strServer = _T("");
	strDefault.LoadString(IDS_DEFAULT_USER);
	if (strDefault.CompareNoCase(strUser) == 0)
		strUser = pDoc->GetDefaultConnectedUser();
	queryInfo.SetServerClass(strServer);

	CaSessionManager& sessionMgr = theApp.GetSessionManager();
	SETLOCKMODE lockmode;
	memset (&lockmode, 0, sizeof(lockmode));
	lockmode.nReadLock = LM_NOLOCK;
	CaSession session;
	session.SetFlag(queryInfo.GetFlag());
	session.SetNode(queryInfo.GetNode());
	session.SetDatabase(queryInfo.GetDatabase());
	session.SetUser(queryInfo.GetUser());
	session.SetServerClass(queryInfo.GetServerClass());
	session.SetOptions(queryInfo.GetOptions());
	session.SetDescription(sessionMgr.GetDescription());
	CaSessionUsage useSession (&sessionMgr, &session);
	CaSession* pSession = useSession.GetCurrentSession();
	if (pSession)
		pSession->SetLockMode(&lockmode);
	bOK = INGRESII_llQueryObject2 (&queryInfo, listObj, pSession);

	return bOK;
}

#if defined (_OPTION_GROUPxROLE)
BOOL DML_QueryGroup(CdSqlQuery* pDoc, CTypedPtrList< CObList, CaDBObject* >& listObj)
{
	BOOL bOK = FALSE;
	CaLLQueryInfo queryInfo (OBT_GROUP, pDoc->GetNode(), _T("iidbdb"));

	CString strDefault;
	CString strNode = pDoc->GetNode();
	CString strServer = pDoc->GetServer();
	CString strUser = pDoc->GetUser();

	strDefault.LoadString(IDS_DEFAULT_SERVER);
	if (strDefault.CompareNoCase(strServer) == 0)
		strServer = _T("");
	strDefault.LoadString(IDS_DEFAULT_USER);
	if (strDefault.CompareNoCase(strUser) == 0)
		strUser = pDoc->GetDefaultConnectedUser();
	queryInfo.SetServerClass(strServer);

	CaSessionManager& sessionMgr = theApp.GetSessionManager();
	SETLOCKMODE lockmode;
	memset (&lockmode, 0, sizeof(lockmode));
	lockmode.nReadLock = LM_NOLOCK;
	CaSession session;
	session.SetFlag(queryInfo.GetFlag());
	session.SetNode(queryInfo.GetNode());
	session.SetDatabase(queryInfo.GetDatabase());
	session.SetUser(queryInfo.GetUser());
	session.SetServerClass(queryInfo.GetServerClass());
	session.SetOptions(queryInfo.GetOptions());
	session.SetDescription(sessionMgr.GetDescription());
	CaSessionUsage useSession (&sessionMgr, &session);
	CaSession* pSession = useSession.GetCurrentSession();
	if (pSession)
		pSession->SetLockMode(&lockmode);

	bOK = INGRESII_llQueryObject2 (&queryInfo, listObj, pSession);

	return bOK;
}

BOOL DML_QueryRole(CdSqlQuery* pDoc, CTypedPtrList< CObList, CaDBObject* >& listObj)
{
	BOOL bOK = FALSE;
	CaLLQueryInfo queryInfo (OBT_ROLE, pDoc->GetNode(), _T("iidbdb"));
	queryInfo.SetDatabase("iidbdb");

	CString strDefault;
	CString strNode = pDoc->GetNode();
	CString strServer = pDoc->GetServer();
	CString strUser = pDoc->GetUser();

	strDefault.LoadString(IDS_DEFAULT_SERVER);
	if (strDefault.CompareNoCase(strServer) == 0)
		strServer = _T("");
	strDefault.LoadString(IDS_DEFAULT_USER);
	if (strDefault.CompareNoCase(strUser) == 0)
		strUser = pDoc->GetDefaultConnectedUser();
	queryInfo.SetServerClass(strServer);

	SETLOCKMODE lockmode;
	memset (&lockmode, 0, sizeof(lockmode));
	lockmode.nReadLock = LM_NOLOCK;
	CaSessionManager& sessionMgr = theApp.GetSessionManager();
	CaSession session;
	session.SetFlag(queryInfo.GetFlag());
	session.SetNode(queryInfo.GetNode());
	session.SetDatabase(queryInfo.GetDatabase());
	session.SetUser(queryInfo.GetUser());
	session.SetServerClass(queryInfo.GetServerClass());
	session.SetOptions(queryInfo.GetOptions());
	session.SetDescription(sessionMgr.GetDescription());
	CaSessionUsage useSession (&sessionMgr, &session);
	CaSession* pSession = useSession.GetCurrentSession();
	if (pSession)
		pSession->SetLockMode(&lockmode);
	bOK = INGRESII_llQueryObject2 (&queryInfo, listObj, pSession);

	return bOK;
}
#endif


BOOL DML_QueryDatabase(CdSqlQuery* pDoc, CTypedPtrList< CObList, CaDBObject* >& listObj, int& nSessionVersion)
{
	BOOL bOK = FALSE;
	CString strNode = pDoc->GetNode();
	CString strConnectionString = pDoc->GetConnectInfo();
	if (!strConnectionString.IsEmpty())
		strNode = strConnectionString;
	CaLLQueryInfo queryInfo (OBT_DATABASE, strNode, _T("iidbdb"));
	queryInfo.SetFetchObjects (CaLLQueryInfo::FETCH_ALL);

	CString strDefault;
	CString strServer = pDoc->GetServer();
	CString strUser = pDoc->GetUser();

	strDefault.LoadString(IDS_DEFAULT_SERVER);
	if (strDefault.CompareNoCase(strServer) == 0)
		strServer = _T("");
	strDefault.LoadString(IDS_DEFAULT_USER);
	if (strDefault.CompareNoCase(strUser) == 0)
		strUser = pDoc->GetDefaultConnectedUser();
	queryInfo.SetServerClass(strServer);
	queryInfo.SetUser(strUser);

	SETLOCKMODE lockmode;
	memset (&lockmode, 0, sizeof(lockmode));
	lockmode.nReadLock = LM_NOLOCK;
	CaSessionManager& sessionMgr = theApp.GetSessionManager();
	CaSession session;
	session.SetFlag(queryInfo.GetFlag());
	session.SetNode(queryInfo.GetNode());
	session.SetDatabase(queryInfo.GetDatabase());
	session.SetUser(queryInfo.GetUser());
	session.SetServerClass(queryInfo.GetServerClass());
	session.SetOptions(queryInfo.GetOptions());
	session.SetDescription(sessionMgr.GetDescription());
	CaSessionUsage useSession (&sessionMgr, &session);
	CaSession* pSession = useSession.GetCurrentSession();
	if (pSession)
	{
		pSession->SetLockMode(&lockmode);
		nSessionVersion = pSession->GetVersion();
		bOK = INGRESII_llQueryObject2 (&queryInfo, listObj, pSession);
	}

	return bOK;
}
