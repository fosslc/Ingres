/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
** Source   : ipmdml.cpp, Implementation file
** Project  : Ingres II/ (Vdba Monitoring)
** Author   : UK Sotheavut (uk$so01)
** Purpose  : Data manipulation: access the lowlevel data through 
**            Library or COM module.
** History:
**	12-Nov-2001 (uk$so01)
**	    Created
**	17-Jul-2003 (uk$so01)
**	    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**	    have the descriptions.
**	02-feb-20043 (somsa01)
**	    Updated to format some UCHAR's into CString before using them
**	    in string concatenation statements.
**	15-Mar-2004 (schph01)
**	    (SIR #110013) Handle the DAS Server
**  30-Apr-2004 (schph01)
**      (BUG 112243) Enable to launch the IPM_OpenCloseServer() function
** 19-Nov-2004 (uk$so01)
**    BUG #113500 / ISSUE #13755297 (Vdba / monitor, references such as background 
**    refresh, or the number of sessions, are not taken into account in vdbamon nor 
**    in the VDBA Monitor functionality)
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "repevent.h"
#include "ipmdml.h"
#include "ipmdoc.h"
#include "ipmframe.h"
#include "repevent.h"
#include "monrepli.h"
#include "regsvice.h"
#include "xdlgacco.h"
#include "constdef.h"
#include "taskanim.h"
#include "runnode.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int CaSwitchSessionThread::In()
{
	if (m_bFirstEnable)
		return DBAThreadEnableCurrentSession();
	else
		return DBAThreadDisableCurrentSession();
}

int CaSwitchSessionThread::Out()
{
	if (m_nOut)
		return RES_SUCCESS;
	if (m_bFirstEnable)
		return DBAThreadDisableCurrentSession();
	else
		return DBAThreadEnableCurrentSession();
}


void MessageWithHistoryButton( CString& strMsg)
{
	if (strMsg.IsEmpty())
		return;
	AfxMessageBox(strMsg);
}

void ManageError4SendEvent(LPERRORPARAMETERS lpErr)
{
	CString strMsg;

	switch (lpErr->iErrorID)
	{
	case RES_E_MAX_CONN_SEND_EVENT:
		strMsg.Format(IDS_F_MAX_CONN_SEND_EVENT,lpErr->tcParam1,lpErr->tcParam2);
		break;
	case RES_E_DBA_OWNER_EVENT:
		strMsg.Format(IDS_F_DBA_OWNER_EVENT,lpErr->tcParam2,lpErr->tcParam1,lpErr->tcParam2);
		break;
	case RES_E_OPENING_SESSION:
		strMsg.Format(IDS_F_OPENING_SESSION,lpErr->tcParam1);
		break;
	case RES_E_SEND_EXECUTE_IMMEDIAT:
		strMsg.Format(IDS_E_SEND_EXECUTE_IMMEDIAT,lpErr->iParam1);
		break;
	default:
		strMsg.Format(_T("Error while issuing event.")); // IDS_E_ISSUING_EVENT
		break;
	}
	MessageWithHistoryButton(strMsg);
	//AfxMessageBox(strMsg); 
}

CaIpmQueryInfo::CaIpmQueryInfo(CdIpmDoc* pDoc, int nObjType, LPIPMUPDATEPARAMS pUps)
    :CaLLQueryInfo(), m_pIpmDoc(pDoc), m_pUps(pUps)
{
	m_bReportError = TRUE;
	SetObjectType(nObjType);
	m_nRegSize = 0;
	m_bAllocated = FALSE;
	m_pStruct = NULL;
}

CaIpmQueryInfo::CaIpmQueryInfo(CdIpmDoc* pDoc, int nObjType, LPIPMUPDATEPARAMS pUps, LPVOID lpStruct, int nStructSize)
    :CaLLQueryInfo(), m_pIpmDoc(pDoc), m_pUps(pUps)
{
	m_bReportError = TRUE;
	SetObjectType(nObjType);
	m_nRegSize = nStructSize;
	m_bAllocated = FALSE;
	m_pStruct = lpStruct;
	if (!lpStruct)
	{
		m_pStruct = new char [nStructSize];
		m_bAllocated = TRUE;
	}
}


CaIpmQueryInfo::~CaIpmQueryInfo()
{
	if (m_bAllocated && m_pStruct)
		delete m_pStruct;
}


BOOL IPM_GetLockSummaryInfo(CaIpmQueryInfo* pQueryInfo, LPLOCKSUMMARYDATAMIN presult, LPLOCKSUMMARYDATAMIN pstart, BOOL bSinceStartUp, BOOL bNowAction)
{
	BOOL bOK = FALSE;
	ASSERT(pQueryInfo && pQueryInfo->GetDocument());
	if (!pQueryInfo)
		return FALSE;
	if (!pQueryInfo->GetDocument())
		return FALSE;

	CdIpmDoc* pDoc = pQueryInfo->GetDocument();
	int nHNode = pDoc->GetNodeHandle();
	int nRes = MonGetLockSummaryInfo(nHNode, presult, pstart, bSinceStartUp, bNowAction);
	return (nRes == RES_ERR)? FALSE: TRUE;

	return bOK;
}

BOOL IPM_GetLogSummaryInfo (CaIpmQueryInfo* pQueryInfo, LPLOGSUMMARYDATAMIN presult, LPLOGSUMMARYDATAMIN pstart, BOOL bSinceStartUp, BOOL bNowAction)
{
	BOOL bOK = FALSE;
	ASSERT(pQueryInfo && pQueryInfo->GetDocument());
	if (!pQueryInfo)
		return FALSE;
	if (!pQueryInfo->GetDocument())
		return FALSE;

	CdIpmDoc* pDoc = pQueryInfo->GetDocument();
	int nHNode = pDoc->GetNodeHandle();
	int nRes = MonGetLogSummaryInfo(nHNode, presult, pstart, bSinceStartUp, bNowAction);
	return (nRes == RES_ERR)? FALSE: TRUE;

	return bOK;
}

BOOL IPM_llQueryDetailInfo (CaIpmQueryInfo* pQueryInfo, LPVOID lpInfoStruct)
{
	BOOL bOK = TRUE;
	int nRes = 0;
	int nObjectType = pQueryInfo->GetObjectType();
	CdIpmDoc* pIpmDoc = pQueryInfo->GetDocument();
	ASSERT(pIpmDoc);
	if (!pIpmDoc)
		return FALSE;

	switch(nObjectType)
	{
	case OT_MON_SESSION:
	case OT_MON_LOCKLIST:
	case OT_MON_LOCK:
	case OT_MON_LOGHEADER:
	case OT_MON_LOGPROCESS:
	case OTLL_MON_RESOURCE:
	case OT_MON_SERVER:
	case OT_MON_TRANSACTION:
		nRes = MonGetDetailInfo(pIpmDoc->GetNodeHandle(), lpInfoStruct, nObjectType, 0);
		bOK = (nRes == RES_SUCCESS);
		break;
	default:
		//
		// New type ??? 
		ASSERT(FALSE);
		break;
	}
	return bOK;
}


BOOL IPM_llQueryInfo (CaIpmQueryInfo* pQueryInfo, CPtrList& listInfoStruct)
{
	UCHAR buf      [MAXOBJECTNAME];
	UCHAR buffilter[MAXOBJECTNAME];
	UCHAR extradata[EXTRADATASIZE];
	LPUCHAR aparents[3];
	BOOL bOK = TRUE;
	int nRes = 0;
	CdIpmDoc* pIpmDoc = NULL;
	IPMUPDATEPARAMS* pUps = NULL;
	LPVOID lpDataStruct = NULL;
	int nStructSize = 0;
	int nNodeHandle = 0;
	int nObjectType = pQueryInfo->GetObjectType();

	switch(nObjectType)
	{
	case OT_REPLIC_REGCOLS_V11:
		{
			UCHAR bufparenttbl[MAXOBJECTNAME];
			aparents[0]= (LPUCHAR)(LPCTSTR)pQueryInfo->GetDatabase ();
			aparents[1] = StringWithOwner((unsigned char *)(LPCTSTR)pQueryInfo->GetItem2(),
			                              (unsigned char *)(LPCTSTR)pQueryInfo->GetItem2Owner(), bufparenttbl);
			aparents[2]='\0';

			nRes = DBAGetFirstObject ((UCHAR *)(LPCTSTR)pQueryInfo->GetNode(),
			                          nObjectType,
			                          2,
			                          aparents,
			                          TRUE,
			                          buf,
			                          buffilter,extradata);
			if (nRes != RES_ENDOFDATA && nRes != RES_SUCCESS)
			{
				pQueryInfo->GetException().SetErrorCode(nRes);
				return FALSE;
			}
			while (nRes == RES_SUCCESS)
			{
				CaReplicCommon *pItemCol = new CaReplicCommon((LPTSTR)buf,getint(extradata+STEPSMALLOBJ));
				listInfoStruct.AddTail(pItemCol);

				nRes = DBAGetNextObject(buf,buffilter,extradata);
			}
		}
		break;
	case OT_REPLIC_REGTBLS_V11:
		{
			aparents[0]= (LPUCHAR)(LPCTSTR)pQueryInfo->GetDatabase ();
			aparents[1]='\0';
			aparents[2]='\0';

			DD_REGISTERED_TABLES RegTmp;
			nRes = DBAGetFirstObject ((LPUCHAR)(LPCTSTR)pQueryInfo->GetNode(),
			                          OT_REPLIC_REGTBLS_V11,
			                          1,
			                          aparents,
			                          TRUE,
			                          (unsigned char *)&RegTmp,
			                          NULL,NULL);
			if (nRes != RES_ENDOFDATA && nRes != RES_SUCCESS)
			{
				pQueryInfo->GetException().SetErrorCode(nRes);
				return FALSE;
			}
			while (nRes == RES_SUCCESS) 
			{
				DD_REGISTERED_TABLES* pObj = new DD_REGISTERED_TABLES;
				memcpy (pObj, &RegTmp, sizeof(DD_REGISTERED_TABLES));
				listInfoStruct.AddTail(pObj);
				nRes = DBAGetNextObject((unsigned char *)&RegTmp,NULL,NULL);
			}
		}
		break;
	case OT_MON_REPLIC_CDDS_ASSIGN:
		{
			REPLICCDDSASSIGNDATAMIN Cddsdata;
			REPLICSERVERDATAMIN RepSvrdta  = *((LPREPLICSERVERDATAMIN)pQueryInfo->GetUpdateParam()->pStruct);
			CString strNode = pQueryInfo->GetNode();

			nRes = DBAGetFirstObject( (LPUCHAR)NULL,
			                           OT_MON_REPLIC_CDDS_ASSIGN,
			                           0,
			                           (LPUCHAR*)&RepSvrdta,
			                           FALSE,(LPUCHAR)&Cddsdata,
			                           NULL,
			                           NULL);
			if (nRes != RES_ENDOFDATA && nRes != RES_SUCCESS)
			{
				pQueryInfo->GetException().SetErrorCode(nRes);
				return FALSE;
			}

			while (nRes == RES_SUCCESS) 
			{
				REPLICCDDSASSIGNDATAMIN* pObj = new REPLICCDDSASSIGNDATAMIN;
				memcpy (pObj, &Cddsdata, sizeof(REPLICCDDSASSIGNDATAMIN));
				listInfoStruct.AddTail(pObj);

				nRes = DBAGetNextObject((LPUCHAR)&Cddsdata, NULL, NULL);
			}
		}
		break;
	case OT_REPLIC_CONNECTION_V11:
		{
		CStringList* pObj = NULL;
		CString Tempo;
		aparents[0]= (LPUCHAR)(LPCTSTR)pQueryInfo->GetDatabase ();
		aparents[1]='\0';
		aparents[2]='\0';

		nRes = DBAGetFirstObject ((LPUCHAR)(LPCTSTR)pQueryInfo->GetNode(),
		                           OT_REPLIC_CONNECTION_V11,
		                           1,
		                           aparents,
		                           TRUE,
		                           buf,
		                           buffilter,extradata);
		if (nRes != RES_ENDOFDATA && nRes != RES_SUCCESS)
		{
			pQueryInfo->GetException().SetErrorCode(nRes);
			return FALSE;
		}
		while ( nRes == RES_SUCCESS )
		{
			if (!pObj)
				pObj = new CStringList();
			Tempo.Empty();
			Tempo.Format(_T("%d %s::%s"),getint(extradata),buf,buffilter);
			// Add in the first combo the local Database only.
			if ( getint(extradata+(2*STEPSMALLOBJ)) == 1)
				pObj->AddHead(Tempo); // the first item is the local database.
			else
				pObj->AddTail(Tempo);
			nRes = DBAGetNextObject(buf,buffilter,extradata);
		}
		if (pObj)
			listInfoStruct.AddTail(pObj);
		}
		break;
	default:
		pIpmDoc = pQueryInfo->GetDocument();
		ASSERT(pIpmDoc);
		if (!pIpmDoc)
			return FALSE;
		pUps = pQueryInfo->GetUpdateParam();
		lpDataStruct = pQueryInfo->GetStruct();
		nStructSize = pQueryInfo->GetStructSize();
		nNodeHandle = pIpmDoc->GetNodeHandle();
		nRes = GetFirstMonInfo (nNodeHandle, pUps->nType, pUps->pStruct, nObjectType, lpDataStruct, pUps->pSFilter);
		switch (nRes)
		{
		case RES_ERR:
		case RES_TIMEOUT:
		case RES_NOGRANT:
		case RES_NOT_OI:
		case RES_CANNOT_SOLVE:
		case RES_MULTIPLE_GRANTS:
			pQueryInfo->GetException().SetErrorCode(nRes);
			return FALSE;
		default:
			break;
		}

		while (nRes == RES_SUCCESS) 
		{
			LPVOID pObj = new char [nStructSize];
			memcpy (pObj, lpDataStruct, nStructSize);
			listInfoStruct.AddTail(pObj);
			nRes = GetNextMonInfo(lpDataStruct);
		}
		break;
	}

	bOK = (nRes == RES_SUCCESS || nRes == RES_ENDOFDATA); 

	return bOK;
}

void IPM_SetSessionStart(long nStart)
{
	LIBMON_SetSessionStart(nStart);
}

void IPM_SetSessionDescription(LPCTSTR lpszSessionDescription)
{
	LIBMON_SetSessionDescription(lpszSessionDescription);
}

void IPM_PropertiesChange(WPARAM wParam, LPARAM lParam)
{
	UINT nMask = (UINT)wParam;
	if ((nMask & IPMMASK_BKREFRESH) || (nMask & IPMMASK_TIMEOUT))
	{
		CaIpmProperty* pProperty = (CaIpmProperty*)lParam;
		if (!pProperty)
			return;
		if (nMask & IPMMASK_BKREFRESH)
		{
			FREQUENCY fq;
			memset (&fq, 0, sizeof(fq));
			fq.count = (int)pProperty->GetRefreshFrequency();
			fq.unit  = pProperty->GetUnit();
			fq.unit++; // pProperty->GetUnit() return  0 ...6 to be equivalent 1=FREQ_UNIT_SECONDS ... 7=FREQ_UNIT_YEARS
			LIBMON_SetRefreshFrequency(&fq);
		}
		if (nMask & IPMMASK_TIMEOUT)
		{
			LIBMON_SetSessionTimeOut (pProperty->GetTimeout());
		}
	}
	if (nMask & IPMMASK_MAXSESSION)
	{
		CaIpmProperty* pProperty = (CaIpmProperty*)lParam;
		if (!pProperty)
			return;
		LIBMON_SetMaxSessions(pProperty->GetMaxSession());
	}
}


BOOL IPM_Initiate(CdIpmDoc* pDoc)
{
	CaConnectInfo& connectInfo = pDoc->GetConnectionInfo();
	CString strNode = connectInfo.GetNode();
	CString strServer = connectInfo.GetServerClass();
	CString strUser= connectInfo.GetUser();

	//
	// Prefix server
	if (!strServer.IsEmpty())
	{
		CString strFmt;
		strFmt.Format (_T(" (/%s)"), (LPCTSTR)strServer);
		strNode += strFmt;
	}
	//
	// Prefix user
	if (!strUser.IsEmpty())
	{
		CString strFmt;
		strFmt.Format (_T(" (user:%s)"), (LPCTSTR)strUser);
		strNode += strFmt;
	}

	int nRes = LIBMON_CheckConnection ((LPUCHAR)(LPCTSTR)strNode);
	if (nRes != RES_SUCCESS)
		return FALSE;
	int nDbmsVersion = GetOIVers();
	if (nDbmsVersion == OIVERS_NOTOI)
	{
		AfxMessageBox (IDS_MSG_IPM_UNAVAILABLE);
		return FALSE;
	}
	nRes = LIBMON_OpenNodeStruct((LPUCHAR)(LPCTSTR)strNode);
	if (nRes == -1)
		return FALSE;
	
	pDoc->SetDbmsVersion(nDbmsVersion);
	pDoc->SetNodeHandle(nRes);
	return TRUE;
}

BOOL IPM_TerminateDocument(CdIpmDoc* pDoc)
{
	BOOL bOK = TRUE;
	int nNodeHandle = pDoc->GetNodeHandle();
	if (nNodeHandle != -1)
		bOK = LIBMON_CloseNodeStruct (nNodeHandle);
	return bOK;
}

BOOL IPM_Initialize(HANDLE& hEventDataChange)
{
	BOOL bOK = TaskAnimateInitialize();
	if (!bOK)
	{
		CString strDll= strDll = _T("tkanimat.dll");
		CString strMsg;
#if defined (MAINWIN)
	#if defined (hpb_us5)
		strDll = _T("libtkanimat.sl");
	#else
		strDll = _T("libtkanimat.so");
	#endif
#endif
		AfxFormatString1 (strMsg, IDS_FAIL_TO_LOAD_DLL, strDll);
		AfxMessageBox (strMsg, MB_ICONHAND|MB_OK);
		return FALSE;
	}
	bOK =initNodeStruct();
	if (bOK)
	{
		hEventDataChange = LIBMON_CreateEventDataChange();
	}

	return bOK ;
}

BOOL IPM_Exit(HANDLE& hEventDataChange)
{
	BOOL bOK = EndNodeStruct();
	if (hEventDataChange)
	{
		CloseHandle(hEventDataChange);
	}
	return bOK;
}

BOOL IPM_SetDbmsVersion(int nDbmsVersion)
{
	SetOIVers(nDbmsVersion);
	return TRUE;
}


BOOL IPM_GetInfoName(CdIpmDoc* pDoc, int nObjectType, LPVOID pData, LPTSTR lpszbufferName, int nBufferSize)
{
	BOOL bOK = TRUE;
	ASSERT(pDoc);
	if (!pDoc)
		return FALSE;
	int nHNode = pDoc->GetNodeHandle();
	GetMonInfoName(nHNode, nObjectType, pData, lpszbufferName, nBufferSize);
	return bOK;
}


BOOL IPM_GetDatabaseOwner(CaIpmQueryInfo* pQueryInfo, CString& strDatabaseOwner)
{
	int ires,irestype, nNodeHandle;
	UCHAR buf [EXTRADATASIZE];
	UCHAR extradata [EXTRADATASIZE];
	UCHAR bufowner[MAXOBJECTNAME];
	TCHAR VnodeName[256];
	LPUCHAR parentstrings [MAXPLEVEL];
	strDatabaseOwner.Empty();
	_tcscpy(VnodeName ,(LPCTSTR)pQueryInfo->GetNode());

	// get DBOwner
	nNodeHandle = LIBMON_OpenNodeStruct((LPUCHAR)VnodeName);
	if (nNodeHandle<0)
	{
		//"No node handle availaible.No assignement display."
		AfxMessageBox( IDS_E_HANDLE_AVALAIBLE, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
		return FALSE;
	}
	ires= DOMGetFirstObject  ( nNodeHandle, OT_DATABASE, 0, NULL, TRUE,NULL,buf,bufowner,extradata);
	if (ires!=RES_SUCCESS && ires!=RES_ENDOFDATA)
	{
		LIBMON_CloseNodeStruct(nNodeHandle);
		return FALSE;
	}
	parentstrings[0]=(LPUCHAR)(LPCTSTR)pQueryInfo->GetDatabase();
	parentstrings[1]=NULL;

	ires = DOMGetObjectLimitedInfo(
		nNodeHandle,
		parentstrings[0],
		(UCHAR *)"",
		OT_DATABASE,
		0,
		parentstrings,
		TRUE,
		&irestype,
		buf,
		bufowner,
		extradata);

	LIBMON_CloseNodeStruct(nNodeHandle);
	if (ires != RES_SUCCESS) 
	{
		//"get database owner failed.No assignement display."
		AfxMessageBox( IDS_E_DB_OWNER, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
		return FALSE;
	}

	strDatabaseOwner = (LPCTSTR)bufowner;
	return TRUE;
}



BOOL IPM_StartReplicationServer(CdIpmDoc* pIpmDoc, REPLICSERVERDATAMIN* pSvrDta, CString& strOutput)
{
	int ires, nNodeHandle;
	UCHAR buf[300];
	CString csOwner;
	CString LocalDBNodeAndUser;
	try
	{
		if (lstrcmp((LPCTSTR)pSvrDta->LocalDBNode, (LPCTSTR)pSvrDta->RunNode) == 0)
		{
			CaIpmQueryInfo queryInfo (NULL, -1);
			queryInfo.SetNode((LPCTSTR)pSvrDta->LocalDBNode);
			queryInfo.SetDatabase((LPCTSTR)pSvrDta->LocalDBName);

			BOOL bOK = IPM_GetDatabaseOwner(&queryInfo, csOwner);
			if (!bOK)
				return FALSE;
			LocalDBNodeAndUser.Empty();
			LocalDBNodeAndUser = pSvrDta->LocalDBNode;
			LocalDBNodeAndUser = LocalDBNodeAndUser + LPUSERPREFIXINNODENAME + csOwner + LPUSERSUFFIXINNODENAME;
		}
		else {
			LocalDBNodeAndUser.Empty();
			LocalDBNodeAndUser = pSvrDta->RunNode;
		}

		nNodeHandle  = LIBMON_OpenNodeStruct ((LPUCHAR)(LPCTSTR)LocalDBNodeAndUser);
		if (nNodeHandle == -1) 
		{
			CString strMsg, strMsg2;
			strMsg.LoadString (IDS_MAX_NB_CONNECT);
			strMsg2.LoadString(IDS_E_EVENT_LAUNCH);
			strMsg += strMsg2;
			AfxMessageBox(strMsg, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
			return FALSE;
		}
		
		// Temporary for activate a session
		ires = DOMGetFirstObject (nNodeHandle, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL);
		CString AllLines;

		ires = StartReplicServer (nNodeHandle, pSvrDta);
		if (ires == RES_ERR)
			AllLines.LoadString(IDS_E_START_REPLIC_SVR);//_T("Error while starting the Replication Server.\r\n")
		else 
		{
			//_T("To view the replicat.log file, please press\r\n"
			//"the \"View Log\" button when the status indicator\r\n"
			//"has been refreshed\r\n");
			AllLines.LoadString(IDS_E_VIEW_REPLIC_LOG);
		}

		LIBMON_CloseNodeStruct (nNodeHandle);
		if (!AllLines.IsEmpty())
		{
			strOutput = AllLines;
		}

		return TRUE;
	}
	catch (CeException e)
	{
		switch (e.GetErrorCode())
		{
		case E_SERVICE_CONNECT:
			AfxMessageBox (IDS_E_CONNECT_SERVICE);
			break;
		case E_SERVICE_STATUS:
			AfxMessageBox (IDS_E_SERVICE_STATUS);
			break;
		case E_SERVICE_NAME:
			AfxMessageBox (IDS_E_GET_SERVICE_NAME);
			break;
		case E_SERVICE_CONFIG:
			AfxMessageBox (IDS_E_CHANGE_SERVICE_CONF);
			break;
		default:
			ASSERT(FALSE);
			break;
		}
	}
	catch (...)
	{

	}
	return FALSE;
}


BOOL IPM_StopReplicationServer(CdIpmDoc* pIpmDoc, REPLICSERVERDATAMIN* pSvrDta, CString& strOutput)
{
	int ires,nNodeHandle;
	UCHAR buf[300];
	CString LocalDBNodeAndUser;
	CWaitCursor hourglass;
	CString csTempOwner;

	if (lstrcmp((LPCTSTR)pSvrDta->LocalDBNode,(LPCTSTR)pSvrDta->RunNode) == 0) 
	{
		CaIpmQueryInfo queryInfo (NULL, -1);
		queryInfo.SetNode((LPCTSTR)pSvrDta->LocalDBNode);
		queryInfo.SetDatabase((LPCTSTR)pSvrDta->LocalDBName);

		BOOL bOK = IPM_GetDatabaseOwner(&queryInfo, csTempOwner);
		if (!bOK)
			return FALSE;

		LocalDBNodeAndUser.Empty();
		LocalDBNodeAndUser = pSvrDta->LocalDBNode;
		LocalDBNodeAndUser = LocalDBNodeAndUser + LPUSERPREFIXINNODENAME + csTempOwner + LPUSERSUFFIXINNODENAME;
	}
	else {
		LocalDBNodeAndUser.Empty();
		LocalDBNodeAndUser = pSvrDta->RunNode;
	}
	nNodeHandle  = LIBMON_OpenNodeStruct ((LPUCHAR)(LPCTSTR)LocalDBNodeAndUser);
	if (nNodeHandle == -1) {
		//"No more available internal node handle. Cannot launch DB Event."
		CString strMsg, strMsg2;
		strMsg.LoadString (IDS_MAX_NB_CONNECT);
		strMsg2.LoadString(IDS_E_EVENT_LAUNCH);
		strMsg += strMsg2;

		AfxMessageBox(strMsg , MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
		return FALSE;
	}
	
	// Temporary for activate a session
	ires = DOMGetFirstObject (nNodeHandle, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL);
	
	StopReplicServer (nNodeHandle, pSvrDta);
	LIBMON_CloseNodeStruct (nNodeHandle);
	
	CString AllLines;
	AllLines.LoadString(IDS_E_VIEW_REPLIC_LOG);
	//_T("To view the replicat.log file, please press\r\n"
	//"the \"View Log\" button when the status indicator\r\n"
	//"has been refreshed\r\n");
	
	if (!AllLines.IsEmpty())
	{
		strOutput = AllLines;
	}

	return TRUE;
}

BOOL IPM_ViewReplicationServerLog(REPLICSERVERDATAMIN* pSvrDta, CString& strOutput)
{
	int ires,nNodeHandle;
	UCHAR buf[300];
	CString AllLines;
	CString LocalDBNodeAndUser;
	CString csOwner;

	if (lstrcmp((LPCTSTR)pSvrDta->LocalDBNode, (LPCTSTR)pSvrDta->RunNode) == 0)
	{
		CaIpmQueryInfo queryInfo (NULL, -1);
		queryInfo.SetNode((LPCTSTR)pSvrDta->LocalDBNode);
		queryInfo.SetDatabase((LPCTSTR)pSvrDta->LocalDBName);

		BOOL bOK = IPM_GetDatabaseOwner(&queryInfo, csOwner);
		if (!bOK)
			return FALSE;

		LocalDBNodeAndUser.Empty();
		LocalDBNodeAndUser = (LPCTSTR)pSvrDta->LocalDBNode;
		LocalDBNodeAndUser = LocalDBNodeAndUser + LPUSERPREFIXINNODENAME + csOwner + LPUSERSUFFIXINNODENAME;
	}
	else {
		LocalDBNodeAndUser.Empty();
		LocalDBNodeAndUser = (LPCTSTR)pSvrDta->RunNode;
	}

	CWaitCursor hourglass;
	nNodeHandle  = LIBMON_OpenNodeStruct ((LPUCHAR)(LPCTSTR)LocalDBNodeAndUser);
	if (nNodeHandle == -1)
	{
		CString strMsg, strMsg2;
		strMsg.LoadString (IDS_MAX_NB_CONNECT);
		strMsg2.LoadString(IDS_E_EVENT_LAUNCH);
		strMsg += strMsg2;
		AfxMessageBox(strMsg, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
		return FALSE;
	}
	// Temporary for activate a session
	ires = DOMGetFirstObject (nNodeHandle, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL);

	GetReplicStartLog(pSvrDta, &AllLines);
	if (!AllLines.IsEmpty())
	{
		strOutput = (LPCTSTR)AllLines;
	}
	LIBMON_CloseNodeStruct (nNodeHandle);

	return TRUE;
}

void IPM_FreeMemory(TCHAR **strCur)
{
	if (*strCur != NULL)
	{
		delete *strCur;
		*strCur = NULL;
	}
	return;
}

BOOL IPM_AllocatString(TCHAR **strCur, UINT iIdsString)
{
	CString strTmp;
	int iNbChar;

	strTmp.LoadString( iIdsString );
	if (!strTmp.IsEmpty())
	{
		iNbChar = strTmp.GetLength()+1;
		*strCur = new char [iNbChar];
		if (strCur)
			lstrcpy(*strCur, strTmp);
		else
		{
			AfxMessageBox (_T("Allocation memory failed"));
			return FALSE;
		}
	}
	else
	{
		strCur = NULL;
		AfxMessageBox (_T("String ID unknown. LoadString Failed"));
		return FALSE;
	}
	return TRUE;
}

BOOL IPM_LoadInputString( LPCHKINPUTSTRING  lpInStrFormat)
{
	CString strTmp;

	if ( !IPM_AllocatString(&lpInStrFormat->lpUnexpectedRow         , IDS_E_UNEXPECTED_ROW) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpGetDbName             , IDS_E_GET_DB_NAME) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpIngresReplic          , IDS_I_INGRES_REPLIC) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpDistribConfig         , IDS_I_DISTRIB_CONFIG) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpLocDb                 , IDS_I_LOC_DB) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpDistribDDpathTbl      , IDS_E_DISTRIB_DD_PATH_TBL) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpPropagationPath       , IDS_E_PROPAGATION_PATH) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpOpenLocalCursor       , IDS_E_OPEN_LOCAL_CURSOR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpFetchingDB            , IDS_E_FETCHING_DB) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpCloseLocCursor        , IDS_E_CLOSE_LOC_CURSOR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpLocalRollback         , IDS_E_LOCAL_ROLLBACK) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpLocalCommit           , IDS_E_LOCAL_COMMIT) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpReleaseSession        , IDS_ERR_RELEASE_SESSION) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpCloseTempFile         , IDS_ERR_CLOSE_TEMP_FILE) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpReportDB              , IDS_I_REPORT_DB) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpMaxConnection         , IDS_E_MAX_CONNECTION) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpProhibits             , IDS_E_PROHIBITS) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpNoProblem             , IDS_I_NO_PROBLEM) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpNoRelease             , IDS_ERR_RELEASE_SESSION) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpErrorConnecting       , IDS_E_CONNECTING) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpDBAName               , IDS_F_DBA_NAME) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpReplicObjectInDB      , IDS_E_REPLIC_OBJECT_IN_DB) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpReplicObjectExist     , IDS_E_REPLIC_OBJECT_EXIST) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpOpenCursor            , IDS_E_OPEN_CURSOR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpFetching              , IDS_E_FETCHING) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpCheckingDDDB          , IDS_E_CHECKING_DD_DB) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->DDDatabasesError        , IDS_E_DD_DATABASES_ERROR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpRecordDB              , IDS_E_RECORD_DB) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpDbName                , IDS_F_DB_NAME) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpDbNoAndVnode          , IDS_E_DBNO_AND_VNODE) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpDbNoAndDBMS           , IDS_E_DBNO_AND_DBMS) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpCloseCursorDDDB       , IDS_E_CLOSE_CURSOR_DD_DB) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpOpenCurDDreg          , IDS_E_OPEN_CUR_DD_REG) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpFetchDDReg            , IDS_E_FETCH_DD_REG) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpCheckDDReg            , IDS_E_CHECK_DD_REG) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpDDRegTblError         , IDS_E_DD_REG_TBL_ERROR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpRecordNoNotFound      , IDS_E_RECORD_NO_NOT_FOUND) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpSupportTblNotCreate   , IDS_E_SUPPORT_TBL_NOT_CREATE) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpColumnRegistred       , IDS_E_COLUMN_REGISTRED) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpTblNameError          , IDS_E_TBL_NAME_ERROR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpTblOwnerError         , IDS_E_TBL_OWNER_ERROR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpDBCddsOpenCursor      , IDS_E_DB_CDDS_OPEN_CURSOR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpDDRegistCloseCursor   , IDS_E_DD_REGIST_CLOSE_CURSOR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpDDRegistColOpenCursor , IDS_E_DD_REGIST_COL_OPEN_CURSOR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpDDRegistColFetchCursor, IDS_E_DD_REGIST_COL_FETCH_CURSOR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpDDRegistColChecking   , IDS_E_DD_REGIST_COL_CHECKING) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpDDRegistColError      , IDS_E_DD_REGIST_COL_ERROR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpRecordTable           , IDS_E_RECORD_TABLE) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpColumnSequence        , IDS_E_COLUMN_SEQUENCE) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpRegistColCloseCursor  , IDS_E_REGIST_COL_CLOSE_CURSOR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpKeySequenceError      , IDS_E_KEY_SEQUENCE_ERROR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpCddsOpenCursor        , IDS_E_CDDS_OPEN_CURSOR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpCddsFetchCursor       , IDS_E_CDDS_FETCH_CURSOR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpCddsChecking          , IDS_E_CDDS_CHECKING))
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpCddsCheckError        , IDS_E_CDDS_CHECK_ERROR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpCddsNoFound           , IDS_E_CDDS_NO_FOUND) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpCddsName              , IDS_E_CDDS_NAME) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpCddsCollision         , IDS_E_CDDS_COLLISION) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpCddsErrormode         , IDS_E_CDDS_ERRORMODE) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpDDCddsCloseCursor     , IDS_E_DD_CDDS_CLOSE_CURSOR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpCheckDBCddsOpenCursor , IDS_E_CHECK_DB_CDDS_OPEN_CURSOR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpCddsFetchingCursor    , IDS_E_CDDS_FETCHING_CURSOR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpCddsCheckingDBno      , IDS_E_CDDS_CHECKING_DB_NO) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpCddsError             , IDS_E_CDDS_ERROR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpCddsDBno              , IDS_E_CDDS_DB_NO) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpCddsTargetType        , IDS_E_CDDS_TARGET_TYPE) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpCddsCloseCursor       , IDS_E_CDDS_CLOSE_CURSOR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpPathOpenCursor        , IDS_E_PATH_OPEN_CURSOR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpPathFetchingCursor    , IDS_E_PATH_FETCHING_CURSOR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpDDPathTbl             , IDS_E_DD_PATH_TBL) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpDDPathError           , IDS_E_DD_PATH_ERROR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpRecordCddsNo          , IDS_E_RECORD_CDDS_NO) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpCddsLocalNo           , IDS_E_CDDS_LOCAL_NO) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpClosingCursor         , IDS_E_CLOSING_CURSOR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpSettingSession        , IDS_E_SETTING_SESSION) )
		return FALSE;

	return TRUE;
}

void IPM_ChkFreeInputString( LPCHKINPUTSTRING  lpInStrFormat)
{
	IPM_FreeMemory(&lpInStrFormat->lpUnexpectedRow);
	IPM_FreeMemory(&lpInStrFormat->lpGetDbName);
	IPM_FreeMemory(&lpInStrFormat->lpIngresReplic);
	IPM_FreeMemory(&lpInStrFormat->lpDistribConfig);
	IPM_FreeMemory(&lpInStrFormat->lpLocDb);
	IPM_FreeMemory(&lpInStrFormat->lpDistribDDpathTbl);
	IPM_FreeMemory(&lpInStrFormat->lpPropagationPath);
	IPM_FreeMemory(&lpInStrFormat->lpOpenLocalCursor);
	IPM_FreeMemory(&lpInStrFormat->lpFetchingDB);
	IPM_FreeMemory(&lpInStrFormat->lpCloseLocCursor);
	IPM_FreeMemory(&lpInStrFormat->lpLocalRollback);
	IPM_FreeMemory(&lpInStrFormat->lpLocalCommit);
	IPM_FreeMemory(&lpInStrFormat->lpReleaseSession);
	IPM_FreeMemory(&lpInStrFormat->lpCloseTempFile);
	IPM_FreeMemory(&lpInStrFormat->lpReportDB);
	IPM_FreeMemory(&lpInStrFormat->lpMaxConnection);
	IPM_FreeMemory(&lpInStrFormat->lpProhibits);
	IPM_FreeMemory(&lpInStrFormat->lpNoProblem);
	IPM_FreeMemory(&lpInStrFormat->lpNoRelease);
	IPM_FreeMemory(&lpInStrFormat->lpErrorConnecting);
	IPM_FreeMemory(&lpInStrFormat->lpDBAName);
	IPM_FreeMemory(&lpInStrFormat->lpReplicObjectInDB);
	IPM_FreeMemory(&lpInStrFormat->lpReplicObjectExist);
	IPM_FreeMemory(&lpInStrFormat->lpOpenCursor);
	IPM_FreeMemory(&lpInStrFormat->lpFetching);
	IPM_FreeMemory(&lpInStrFormat->lpCheckingDDDB);
	IPM_FreeMemory(&lpInStrFormat->DDDatabasesError);
	IPM_FreeMemory(&lpInStrFormat->lpRecordDB);
	IPM_FreeMemory(&lpInStrFormat->lpDbName);
	IPM_FreeMemory(&lpInStrFormat->lpDbNoAndVnode);
	IPM_FreeMemory(&lpInStrFormat->lpDbNoAndDBMS);
	IPM_FreeMemory(&lpInStrFormat->lpCloseCursorDDDB);
	IPM_FreeMemory(&lpInStrFormat->lpOpenCurDDreg);
	IPM_FreeMemory(&lpInStrFormat->lpFetchDDReg);
	IPM_FreeMemory(&lpInStrFormat->lpCheckDDReg);
	IPM_FreeMemory(&lpInStrFormat->lpDDRegTblError);
	IPM_FreeMemory(&lpInStrFormat->lpRecordNoNotFound);
	IPM_FreeMemory(&lpInStrFormat->lpSupportTblNotCreate);
	IPM_FreeMemory(&lpInStrFormat->lpColumnRegistred);
	IPM_FreeMemory(&lpInStrFormat->lpTblNameError);
	IPM_FreeMemory(&lpInStrFormat->lpTblOwnerError);
	IPM_FreeMemory(&lpInStrFormat->lpDBCddsOpenCursor);
	IPM_FreeMemory(&lpInStrFormat->lpDDRegistCloseCursor);
	IPM_FreeMemory(&lpInStrFormat->lpDDRegistColOpenCursor);
	IPM_FreeMemory(&lpInStrFormat->lpDDRegistColFetchCursor);
	IPM_FreeMemory(&lpInStrFormat->lpDDRegistColChecking);
	IPM_FreeMemory(&lpInStrFormat->lpDDRegistColError);
	IPM_FreeMemory(&lpInStrFormat->lpRecordTable);
	IPM_FreeMemory(&lpInStrFormat->lpColumnSequence);
	IPM_FreeMemory(&lpInStrFormat->lpRegistColCloseCursor);
	IPM_FreeMemory(&lpInStrFormat->lpKeySequenceError);
	IPM_FreeMemory(&lpInStrFormat->lpCddsOpenCursor);
	IPM_FreeMemory(&lpInStrFormat->lpCddsFetchCursor);
	IPM_FreeMemory(&lpInStrFormat->lpCddsChecking);
	IPM_FreeMemory(&lpInStrFormat->lpCddsCheckError);
	IPM_FreeMemory(&lpInStrFormat->lpCddsNoFound);
	IPM_FreeMemory(&lpInStrFormat->lpCddsName);
	IPM_FreeMemory(&lpInStrFormat->lpCddsCollision);
	IPM_FreeMemory(&lpInStrFormat->lpCddsErrormode);
	IPM_FreeMemory(&lpInStrFormat->lpDDCddsCloseCursor);
	IPM_FreeMemory(&lpInStrFormat->lpCheckDBCddsOpenCursor);
	IPM_FreeMemory(&lpInStrFormat->lpCddsFetchingCursor);
	IPM_FreeMemory(&lpInStrFormat->lpCddsCheckingDBno);
	IPM_FreeMemory(&lpInStrFormat->lpCddsError);
	IPM_FreeMemory(&lpInStrFormat->lpCddsDBno);
	IPM_FreeMemory(&lpInStrFormat->lpCddsTargetType);
	IPM_FreeMemory(&lpInStrFormat->lpCddsCloseCursor);
	IPM_FreeMemory(&lpInStrFormat->lpPathOpenCursor);
	IPM_FreeMemory(&lpInStrFormat->lpPathFetchingCursor);
	IPM_FreeMemory(&lpInStrFormat->lpDDPathTbl);
	IPM_FreeMemory(&lpInStrFormat->lpDDPathError);
	IPM_FreeMemory(&lpInStrFormat->lpRecordCddsNo);
	IPM_FreeMemory(&lpInStrFormat->lpCddsLocalNo);
	IPM_FreeMemory(&lpInStrFormat->lpClosingCursor);
	IPM_FreeMemory(&lpInStrFormat->lpSettingSession);
}

BOOL IPM_CheckDistributionConfig(CdIpmDoc* pDoc, RESOURCEDATAMIN* pSvrDta, CString& strOutput)
{
	TCHAR* tmp = NULL;
	strOutput = _T("");

	TCHAR tchszDbName[100];
	BOOL bOK = IPM_GetInfoName(pDoc, OT_DATABASE, pSvrDta, tchszDbName, sizeof(tchszDbName));
	if (bOK)
	{
		CHKINPUTSTRING  InputStringFormat;
		ERRORPARAMETERS ErrorParameters;
		CString strMsg,strMsg2,csMessage;
		memset(&InputStringFormat, NULL, sizeof(CHKINPUTSTRING));
		memset(&ErrorParameters, NULL, sizeof(ERRORPARAMETERS));

		if (!IPM_LoadInputString(&InputStringFormat))
		{
			IPM_ChkFreeInputString(&InputStringFormat);
			return FALSE; // memory allocated error or string ID unknown.
		}
		tmp = LIBMON_check_distrib_config(pDoc->GetNodeHandle(), tchszDbName,
		                                  &InputStringFormat, &ErrorParameters);
		if ( ErrorParameters.iErrorID & MASK_RES_E_READ_TEMPORARY_FILE )
		{
			strMsg.LoadString(IDS_ERR_READ_TEMPORARY_FILE);
			ErrorParameters.iErrorID &= ~MASK_RES_E_RELEASE_SESSION;
		}

		switch (ErrorParameters.iErrorID)
		{
		case RES_E_OPEN_REPORT:
			strMsg2.Format(IDS_F_OPEN_REPORT,ErrorParameters.tcParam1);
			break;
		case RES_E_OPENING_SESSION:
			strMsg2.Format(IDS_F_OPENING_SESSION,ErrorParameters.tcParam1);
			break;
		}

		if (!strMsg2.IsEmpty())
		{
			if (!strMsg.IsEmpty())
				csMessage = strMsg + _T("\n") + strMsg2;
			else
				csMessage = strMsg2;

			AfxMessageBox(csMessage);
		}
		else if (!strMsg.IsEmpty())
			AfxMessageBox(strMsg);

		if (tmp != NULL){
			strOutput = (LPTSTR)tmp;
			LIBMON_Vdba_FreeMemory(&tmp);
		}
		IPM_ChkFreeInputString(&InputStringFormat);
	}
	return TRUE;
}

BOOL ManageError4collision(LPERRORPARAMETERS lpLstError)
{
	CString csMsg;
	BOOL bMessageWithHistoryButton = FALSE;
	BOOL bContinueDisplayCollision = FALSE;
	
	switch (lpLstError->iErrorID)
	{
	case RES_E_OPENING_SESSION:
		csMsg.Format(IDS_F_OPENING_SESSION,lpLstError->tcParam1);
		break;
	case RES_E_CANNOT_ALLOCATE_MEMORY:
		csMsg = _T("Cannot allocate memory.");
		break;
	case RES_E_OPENING_CURSOR:
		csMsg.Format(IDS_ERR_OPENING_CURSOR);
		bMessageWithHistoryButton = TRUE;
		break;
	case RES_E_MAX_CONNECTIONS:
		csMsg.LoadString(IDS_ERR_MAX_CONNECTIONS);
		break;
	case RES_E_FETCHING_TARGET_DB:
		csMsg.LoadString(IDS_ERR_FETCHING_TARGET_DB);
		bMessageWithHistoryButton = TRUE;
		break;
	case RES_E_COUNT_CONNECTIONS:
		csMsg.LoadString(IDS_ERR_COUNT_CONNECTIONS);
		bMessageWithHistoryButton = TRUE;
		break;
	case RES_E_SELECT_FAILED:
		csMsg.LoadString(IDS_ERR_SELECT_FAILED);
		bMessageWithHistoryButton = TRUE;
		break;
	case RES_E_F_FETCHING_DATA:
		csMsg.Format(IDS_F_FETCHING_DATA, lpLstError->iParam1);
		bMessageWithHistoryButton = TRUE;
		break;
	case RES_E_SQL:
		csMsg.Format(IDS_ERR_SQLERR, lpLstError->iParam1);
		bMessageWithHistoryButton = TRUE;
		break;
	case RES_E_TBL_NUMBER:
		csMsg.Format(IDS_ERR_TBL_NUMBER, lpLstError->iParam1);
		break;
	case RES_E_F_RELATIONSHIP:
		csMsg.Format(IDS_F_RELATIONSHIP, lpLstError->iParam1);
		break;
	case RES_E_NO_COLUMN_REGISTRATION:
		csMsg.Format(IDS_F_NO_COLUMN_REGISTRATION, lpLstError->tcParam1,lpLstError->tcParam2);
		break;
	case RES_E_BLANK_TABLE_NAME:
		csMsg.LoadString(IDS_ERR_BLANK_TABLE_NAME);
		break;
	case RES_E_F_TBL_NUMBER:
		csMsg.Format(IDS_F_TBL_NUMBER, lpLstError->iParam1);
		break;
	case RES_E_F_INVALID_OBJECT:
		csMsg.Format(IDS_F_INVALID_OBJECT, lpLstError->iParam1);
		break;
	case RES_E_INCONSISTENT_COUNT:
		csMsg.Format(IDS_ERR_INCONSISTENT_COUNT, lpLstError->tcParam1);
		break;
	case RES_E_F_GETTING_COLUMNS:
		bMessageWithHistoryButton = TRUE;
		csMsg.Format(IDS_F_GETTING_COLUMNS, lpLstError->iParam1,lpLstError->tcParam1,lpLstError->tcParam1);
		break;
	case RES_E_F_REGISTRED_COL:
		csMsg.Format(IDS_F_REGISTRED_COL, lpLstError->tcParam1,lpLstError->tcParam1);
		break;
	case RES_E_IMMEDIATE_EXECUTION:
		bMessageWithHistoryButton = TRUE;
		csMsg.Format(IDS_ERR_IMMEDIATE_EXECUTION, lpLstError->iParam1);
		break;
	case RES_E_FLOAT_EXEC_IMMEDIATE:
		csMsg.Format(IDS_F_FLOAT_EXEC_IMMEDIATE, lpLstError->iParam1);
		bMessageWithHistoryButton = TRUE;
		break;
	case RES_E_CHAR_EXECUTE_IMMEDIATE:
		csMsg.Format(IDS_ERR_CHAR_EXECUTE_IMMEDIATE, lpLstError->iParam1);
		bMessageWithHistoryButton = TRUE;
		break;
	case RES_E_F_COLUMN_INFO:
		csMsg.Format(IDS_F_COLUMN_INFO, lpLstError->tcParam1);
		bMessageWithHistoryButton = TRUE;
		break;
	default:
		csMsg.Format(_T("Collision Error Number: %d, not managed"),lpLstError->iErrorID);
		bMessageWithHistoryButton = TRUE;
		break;
	}

	if (bMessageWithHistoryButton)
		MessageWithHistoryButton(csMsg);
	else
		AfxMessageBox(csMsg);

	return bContinueDisplayCollision;
}


BOOL IPM_RetrieveCollision(CdIpmDoc* pIpmDoc, RESOURCEDATAMIN* pSvrDta, CaCollision* pCollisionData)
{
	//
	// get info the name of the current database
	char szDbName[100];
	LPRETVISUALINFO lpTmpCol;
	RETVISUALINFO RetInfo;
	ERRORPARAMETERS LstError;
	LPSTOREVISUALINFO lpVisualInfo;

	GetMonInfoName(pIpmDoc->GetNodeHandle(), OT_DATABASE, pSvrDta, szDbName, sizeof(szDbName));

	RetInfo.lpRetVisualSeq = NULL;
	RetInfo.iNumVisualSequence = 0;
	LIBMON_VdbaColl_Retrieve_Collision(pIpmDoc->GetNodeHandle() ,szDbName , &RetInfo, &LstError);

	if (LstError.iErrorID != RES_SUCCESS)
	{
		LIBMON_VdbaCollFreeMemory(&RetInfo);
		ManageError4collision(&LstError);
		return FALSE;
	}
	if (RetInfo.lpRetVisualSeq == NULL)
		return TRUE;

	lpTmpCol = &RetInfo;
	lpVisualInfo = lpTmpCol->lpRetVisualSeq;

	for (int i = 0 ; i<lpTmpCol->iNumVisualSequence; i++,*lpVisualInfo++)
	{
		if (&(lpVisualInfo->VisualSeq) != NULL )
		{
			CaCollisionSequence* pNewSequence;
			pNewSequence = new CaCollisionSequence (lpVisualInfo->VisualSeq.sequence_no);
			switch (lpVisualInfo->VisualSeq.type)
			{
				case TX_INSERT:
					pNewSequence->m_strCollisionType = _T("Insert");
					break;
				case TX_UPDATE:
					pNewSequence->m_strCollisionType = _T("Update");
					break;
				case TX_DELETE:
					pNewSequence->m_strCollisionType = _T("Delete");
					break;
				default:
					pNewSequence->m_strCollisionType = _T("Unknown");
			}
			pNewSequence->m_strTable           = lpVisualInfo->VisualSeq.table_name;
			pNewSequence->m_strTableOwner      = lpVisualInfo->VisualSeq.table_owner;
			pNewSequence->m_nTransaction       = lpVisualInfo->VisualSeq.transaction_id;
			pNewSequence->m_nCDDS              = lpVisualInfo->VisualSeq.cdds_no;
			pNewSequence->m_strLocalDatabase   = lpVisualInfo->VisualSeq.Localdb;
			pNewSequence->m_strSourceDatabase  = lpVisualInfo->VisualSeq.Sourcedb;
			pNewSequence->m_strTargetDatabase  = lpVisualInfo->VisualSeq.Targetdb;
			pNewSequence->m_strSourceVNode     = lpVisualInfo->VisualSeq.SourceVnode;
			pNewSequence->m_strTargetVNode     = lpVisualInfo->VisualSeq.TargetVnode;
			pNewSequence->m_strLocalVNode      = lpVisualInfo->VisualSeq.LocalVnode;
			pNewSequence->m_nPrevTransaction   = lpVisualInfo->VisualSeq.old_transaction_id;
			pNewSequence->m_strSourceTransTime = lpVisualInfo->VisualSeq.SourceTransTime;
			pNewSequence->m_strTargetTransTime = lpVisualInfo->VisualSeq.TargetTransTime;
			pNewSequence->m_nSourceDB          = lpVisualInfo->VisualSeq.nSourceDB;
			pNewSequence->m_nSequenceNo        = lpVisualInfo->VisualSeq.sequence_no;
			pNewSequence->m_nTblNo             = lpVisualInfo->VisualSeq.nTblNo;
			pNewSequence->m_nSvrTargetType     = lpVisualInfo->VisualSeq.nSvrTargetType;

			if (lpVisualInfo->VisualSeq.SourceResolvedCollision == -1)
				pNewSequence->SetImageType (CaCollisionItem::PREVAIL_SOURCE);
			else if (lpVisualInfo->VisualSeq.SourceResolvedCollision == -2)
				pNewSequence->SetImageType (CaCollisionItem::PREVAIL_TARGET);
			else
				pNewSequence->SetImageType (CaCollisionItem::PREVAIL_UNRESOLVED);

			int column_number;
			BOOL bConflicted;
			VISUALCOL *pTempVCol ;

			for (	column_number = 0, pTempVCol = (VISUALCOL *)lpVisualInfo->VisualCol;column_number < lpVisualInfo->iNumVisualColumns;
					column_number++, *pTempVCol++)
			{
				if (lstrcmp(pTempVCol->dataColSource,pTempVCol->dataColTarget)!=0)
					bConflicted = TRUE;
				else
					bConflicted = FALSE;

				pNewSequence->AddColumn ( pTempVCol->column_name,
				                          pTempVCol->dataColSource,
				                          pTempVCol->dataColTarget,
				                          pTempVCol->key_sequence,
				                          bConflicted);
			}
			pCollisionData->AddCollisionSequence (lpVisualInfo->VisualSeq.transaction_id, pNewSequence);
		}
	}

	// Free Memory
	LIBMON_VdbaCollFreeMemory(&RetInfo);

	return TRUE;
}

BOOL IPM_SendRaisedEvents(CdIpmDoc* pDoc, LPREPLICSERVERDATAMIN pSvrDta, CaReplicationRaiseDbevent* Event)
{
	int iret;
	CString svr_No;

	ERRORPARAMETERS lpErr;
	svr_No.Format(_T("%d"),pSvrDta->serverno);
	iret = LIBMON_SendEvent((char *)(LPCTSTR)pSvrDta->LocalDBNode,
	                        (char *)(LPCTSTR)pSvrDta->LocalDBName,
	                        (char *)(LPCTSTR)Event->GetEvent(),
	                        (char *)(LPCTSTR)Event->GetServerFlag(),
	                        (char *)(LPCTSTR)svr_No, &lpErr);
	if (lpErr.iErrorID != RES_SUCCESS)
		ManageError4SendEvent(&lpErr);
	else
		AfxMessageBox(_T("Event issued.")); //IDS_E_EVENT_ISSUED _T("Event issued.")
	return TRUE;
}

BOOL IPM_QueryRaisedEvents(CdIpmDoc* pDoc, RESOURCEDATAMIN* pSvrDta, CaReplicRaiseDbeventList& EventsList)
{
	CString cDefDbName;
	int irestype,iret,NodeHdl;
	UCHAR buf[EXTRADATASIZE];
	UCHAR extradata[EXTRADATASIZE];
	UCHAR DBAUsernameOntarget[MAXOBJECTNAME];
	LPUCHAR parentstrings [MAXPLEVEL];

	// Get current vnode name
	LPUCHAR vnodeName = (LPUCHAR)GetVirtNodeName ( pDoc->GetNodeHandle() );
	
	// get name of the current database
	char szDbName[100];
	GetMonInfoName(pDoc->GetNodeHandle(), OT_DATABASE, pSvrDta, szDbName, sizeof(szDbName));
	//
	//Get DBA user name for this database
	parentstrings[0]=(LPUCHAR)szDbName;
	parentstrings[1]=NULL;
	memset (DBAUsernameOntarget,'\0',sizeof(DBAUsernameOntarget));
	iret = DOMGetObjectLimitedInfo(	 pDoc->GetNodeHandle(),
									parentstrings [0],
									(UCHAR *)"",
									OT_DATABASE,
									0,
									parentstrings,
									TRUE,
									&irestype,
									buf,
									DBAUsernameOntarget,
									extradata
								);
	if (iret != RES_SUCCESS)
		return FALSE;

	CStringList AllServer;
	cDefDbName.Format(_T("%s::%s"),vnodeName, szDbName);

	IPM_GetAllReplicationServerStarted( pDoc, pSvrDta, &AllServer);
	
	while (!EventsList.IsEmpty())
		delete EventsList.RemoveHead();

	CString csVnodeAndUsers;
	UCHAR szCurVnode[MAXOBJECTNAME];
	_tcscpy((LPTSTR)szCurVnode, (LPCTSTR)vnodeName);
	RemoveConnectUserFromString(szCurVnode);

	csVnodeAndUsers.Format(_T("%s%s%s%s"),
		szCurVnode,
		LPUSERPREFIXINNODENAME,
		DBAUsernameOntarget,LPUSERSUFFIXINNODENAME);

	NodeHdl = LIBMON_OpenNodeStruct((LPUCHAR)(LPCTSTR)csVnodeAndUsers);
	if (NodeHdl == -1) 
		return FALSE;
	EventsList.DefineAllDbeventWithDefaultValues(NodeHdl , cDefDbName,-1); 
	LIBMON_CloseNodeStruct (NodeHdl);

	return TRUE;
}

//void GetAllReplicationServerStarted(int hdl ,CString LocalVnode, LPRESOURCEDATAMIN pStruct, CStringList *pcslSrvStarted);
BOOL IPM_GetAllReplicationServerStarted(CdIpmDoc* pDoc, RESOURCEDATAMIN* pStruct, CStringList* pcslSrvStarted)
{
	REPLICSERVERDATAMIN pstructReq;
	pcslSrvStarted->RemoveAll();

	int retval = GetFirstMonInfo(pDoc->GetNodeHandle() , OT_DATABASE, pStruct,  OT_MON_REPLIC_SERVER, &pstructReq, NULL);
	while (retval == RES_SUCCESS) {
		char	name[MAXMONINFONAME];
		name[0] = '\0';
		GetMonInfoName(pDoc->GetNodeHandle() , OT_MON_REPLIC_SERVER, &pstructReq, name, sizeof(name));
		ASSERT (name[0]);
		if (*name != '\0')
			pcslSrvStarted->AddHead( name );
		retval = GetNextMonInfo(&pstructReq);
	}
	// Get if Vnode exist for send event
	POSITION pos,PosCur;
	pos = pcslSrvStarted->GetHeadPosition();
	while (pos != NULL) {
		CString strVName,strNodeName;
		PosCur = pos;
		strVName = pcslSrvStarted->GetNext(pos);
		strNodeName  = strVName.SpanExcluding(_T(":"));
		VerifyAndUpdateVnodeName (&strNodeName);
		if (strNodeName.IsEmpty()) {
		   pcslSrvStarted->RemoveAt(PosCur);
		}
	}
	return TRUE;
}


BOOL IPM_SendEventToServers(CdIpmDoc* pIpmDoc,CString& strEventFlag,CString& strServerFlag,CPtrList& listInfoStruct)
{
	int iret;
	CString SvrNo;
	LPREPLICSERVERDATAMIN dta;
	bool bOneEventFailed = FALSE;
	bool bOneEventSend = FALSE;
	ERRORPARAMETERS lpErr;

	POSITION pos;
	pos = listInfoStruct.GetHeadPosition();
	while (pos != NULL) {
		dta = (LPREPLICSERVERDATAMIN)listInfoStruct.GetNext(pos);
		SvrNo.Format(_T("%d"),dta->serverno);
		iret = LIBMON_SendEvent( (char *)(LPCTSTR)dta->LocalDBNode,
		                         (char *)(LPCTSTR)dta->LocalDBName,
		                         (char *)(LPCTSTR)strEventFlag,
		                         (char *)(LPCTSTR)strServerFlag,
		                         (char *)(LPCTSTR)SvrNo,&lpErr);
		if (lpErr.iErrorID != RES_SUCCESS) {
			ManageError4SendEvent(&lpErr);
			bOneEventFailed = TRUE;
/*
			char name[MAXMONINFONAME];
			CString csMessage;
		
			GetMonInfoName(m_CurNodeHandle, OT_MON_REPLIC_SERVER, dta, name, sizeof(name));
			//"Error while issuing event on %s."
			csMessage.Format(IDS_E_ISSUING_ERROR,name);
			MessageWithHistoryButton(m_hWnd, csMessage);
*/
		}
		else
		{
			if (iret == RES_SUCCESS)
				bOneEventSend=TRUE;
		}
	}
	if (!bOneEventFailed && bOneEventSend)
		AfxMessageBox(IDS_EVENT_ISSUED);//_T("Event(s) issued.")

	return TRUE;
}

BOOL IPM_LoadInputString( LPTBLINPUTSTRING  lpInStrFormat)
{
	CString strTmp;

	if ( !IPM_AllocatString(&lpInStrFormat->lpCheckUniqueKeys        , IDS_E_CHECK_UNIQUE_KEYS_DBMS_ERROR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpLookingUPColumns       , IDS_E_LOOKING_UP_COLUMNS) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpIngresReplic           , IDS_I_INGRES_REPLIC) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpTblIntegReport         , IDS_E_TBL_INTEG_REPORT) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpForTbl                 , IDS_I_FOR_TBL) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpIandtbl                , IDS_I_AND_TBL) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpIRegKeyColumns         , IDS_I_REG_KEY_COLUMNS) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpIUnique                , IDS_I_UNIQUE) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpRowOnlyIn              , IDS_ROW_ONLY_IN) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpTblIntegRowDifferences , IDS_TBL_INTEG_ROW_DIFFERENCES) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpTblIntegSourceDB       , IDS_TBL_INTEG_SOURCE_DB) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpTblIntegDBTransac      , IDS_TBL_INTEG_DB_TRANSAC) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpCheckIndexesOpenCursor , IDS_E_CHECK_INDEXES_OPEN_CURSOR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpFetchIndex             , IDS_E_FETCH_INDEX) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpResTblIntegError       , IDS_E_TBL_INTEG_ERROR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpInvalidDB              ,IDS_E_INVALID_DB) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpTblIntegTargetType     , IDS_E_TBL_INTEG_TARGET_TYPE) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpDbUnprotreadonly       , IDS_E_DB_UNPROTREADONLY) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpMaxNumConnection       , IDS_E_MAX_NUM_CONNECTION) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpConnectDb              , IDS_E_CONNECT_DB) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpReplicObjectInDB       , IDS_E_REPLIC_OBJECT_IN_DB) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpInformationMissing     , IDS_E_INFO_MISSING) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpUnBoundedDta           , IDS_E_UNBOUNDED_DTA) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpColumnInfo             , IDS_F_COLUMN_INFO) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpTblIntegNoReplicCol    , IDS_E_TBL_INTEG_NO_REPLIC_COL) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpKeyColFound            , IDS_E_KEY_COL_FOUND) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpTblIntegProbConfig     , IDS_E_TBL_INTEG_PROB_CONFIG) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpDescribeStatement      , IDS_E_DESCRIBE_STATEMENT) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpTblIntegOpenCursor     , IDS_E_TBL_INTEG_OPEN_CURSOR) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpTblIntegFetch          , IDS_TBL_INTEG_FETCH_INTEG) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpTblIntegExecImm        , IDS_E_TBL_INTEG_EXEC_IMM) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpTblIntegTblDef         , IDS_TBL_INTEG_TBL_DEF) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpTblIntegNoDifferences  , IDS_TBL_INTEG_NO_DIFFERENCES) )
		return FALSE;
	if ( !IPM_AllocatString(&lpInStrFormat->lpReplicObjectExist      , IDS_E_REPLIC_OBJECT_EXIST) )
		return FALSE;

	return TRUE;
}

void IPM_TblFreeInputString( LPTBLINPUTSTRING  lpInStrFormat)
{
	IPM_FreeMemory(&lpInStrFormat->lpCheckUniqueKeys        );
	IPM_FreeMemory(&lpInStrFormat->lpLookingUPColumns       );
	IPM_FreeMemory(&lpInStrFormat->lpIngresReplic           );
	IPM_FreeMemory(&lpInStrFormat->lpTblIntegReport         );
	IPM_FreeMemory(&lpInStrFormat->lpForTbl                 );
	IPM_FreeMemory(&lpInStrFormat->lpIandtbl                );
	IPM_FreeMemory(&lpInStrFormat->lpIRegKeyColumns         );
	IPM_FreeMemory(&lpInStrFormat->lpIUnique                );
	IPM_FreeMemory(&lpInStrFormat->lpRowOnlyIn              );
	IPM_FreeMemory(&lpInStrFormat->lpTblIntegRowDifferences );
	IPM_FreeMemory(&lpInStrFormat->lpTblIntegSourceDB       );
	IPM_FreeMemory(&lpInStrFormat->lpTblIntegDBTransac      );
	IPM_FreeMemory(&lpInStrFormat->lpCheckIndexesOpenCursor );
	IPM_FreeMemory(&lpInStrFormat->lpFetchIndex             );
	IPM_FreeMemory(&lpInStrFormat->lpResTblIntegError       );
	IPM_FreeMemory(&lpInStrFormat->lpInvalidDB              );
	IPM_FreeMemory(&lpInStrFormat->lpTblIntegTargetType     );
	IPM_FreeMemory(&lpInStrFormat->lpDbUnprotreadonly       );
	IPM_FreeMemory(&lpInStrFormat->lpMaxNumConnection       );
	IPM_FreeMemory(&lpInStrFormat->lpConnectDb              );
	IPM_FreeMemory(&lpInStrFormat->lpReplicObjectInDB       );
	IPM_FreeMemory(&lpInStrFormat->lpInformationMissing     );
	IPM_FreeMemory(&lpInStrFormat->lpUnBoundedDta           );
	IPM_FreeMemory(&lpInStrFormat->lpColumnInfo             );
	IPM_FreeMemory(&lpInStrFormat->lpTblIntegNoReplicCol    );
	IPM_FreeMemory(&lpInStrFormat->lpKeyColFound            );
	IPM_FreeMemory(&lpInStrFormat->lpTblIntegProbConfig     );
	IPM_FreeMemory(&lpInStrFormat->lpDescribeStatement      );
	IPM_FreeMemory(&lpInStrFormat->lpTblIntegOpenCursor     );
	IPM_FreeMemory(&lpInStrFormat->lpTblIntegFetch          );
	IPM_FreeMemory(&lpInStrFormat->lpTblIntegExecImm        );
	IPM_FreeMemory(&lpInStrFormat->lpTblIntegTblDef         );
	IPM_FreeMemory(&lpInStrFormat->lpTblIntegNoDifferences  );
	IPM_FreeMemory(&lpInStrFormat->lpReplicObjectExist      );
}

void ManageError4Integrity(LPERRORPARAMETERS lpErrParam)
{
	CString strMsg,strMsgTmp;
	BOOL bMessageWithHistoryButton = FALSE;

	if (lpErrParam->iErrorID == RES_SUCCESS)
		return;

	if (lpErrParam->iErrorID & MASK_RES_E_ACTIVATE_SESSION)
	{
		strMsg.LoadString( IDS_ERR_ACTIVATE_SESSION );
		lpErrParam->iErrorID &= ~MASK_RES_E_ACTIVATE_SESSION;
	}
	if (lpErrParam->iErrorID & MASK_RES_E_RELEASE_SESSION)
	{
		if(!strMsg.IsEmpty())
		{
			strMsgTmp.LoadString(IDS_ERR_RELEASE_SESSION);
			strMsg += strMsgTmp;
		}
		else
			strMsg.LoadString(IDS_ERR_RELEASE_SESSION);
		lpErrParam->iErrorID &= ~MASK_RES_E_RELEASE_SESSION;
	}
	if (lpErrParam->iErrorID & MASK_RES_E_READ_TEMPORARY_FILE)
	{
		if(!strMsg.IsEmpty())
		{
			strMsgTmp.LoadString(IDS_ERR_READ_TEMPORARY_FILE);
			strMsg += strMsgTmp;
		}
		else
			strMsg.LoadString(IDS_ERR_READ_TEMPORARY_FILE);
		lpErrParam->iErrorID &= ~MASK_RES_E_READ_TEMPORARY_FILE;
	}

	strMsgTmp.Empty();
	switch(lpErrParam->iErrorID)
	{
	case RES_E_SQL:
		strMsgTmp.Format(IDS_ERR_SQLERR, lpErrParam->iParam1);
		bMessageWithHistoryButton = TRUE;
		break;
	case RES_E_TBL_NUMBER:
		strMsgTmp.Format(IDS_ERR_TBL_NUMBER, lpErrParam->iParam1);
		break;
	case RES_E_CANNOT_ALLOCATE_MEMORY:
		strMsgTmp = _T("Cannot allocate memory.");
		break;
	case RES_E_INCONSISTENT_COUNT:
		strMsgTmp.Format(IDS_ERR_INCONSISTENT_COUNT, lpErrParam->tcParam1);
		break;
	case RES_E_ACTIV_SESSION:
		strMsgTmp.Format(IDS_E_ACTIV_SESSION, lpErrParam->iParam1);
		break;
	default:
		strMsgTmp.Format(_T("Integrity Error Number: %d, not managed"),lpErrParam->iErrorID);
		bMessageWithHistoryButton = TRUE;
		break;
	}

	if(!strMsgTmp.IsEmpty())
		strMsg += strMsgTmp;

	if (bMessageWithHistoryButton)
		MessageWithHistoryButton(strMsg);
	else
		AfxMessageBox(strMsg);
}
BOOL IPM_QueryIntegrityOutput (CdIpmDoc* pIpmDoc, CaReplicIntegrityRegTable *pItem,
                               int iDb1, int iDb2,int iCddsNo,TCHAR *tcTimeBegin,
                               TCHAR *tcTimeEnd,TCHAR *tcColOrder, CString& strOutput)
{
	CString strMsg,strMsgTmp;
	char *IntegrityInfo = NULL;
	TBLINPUTSTRING  InputStringFormat;
	ERRORPARAMETERS ErrorParameters;
	memset(&ErrorParameters, NULL, sizeof(ERRORPARAMETERS));
	strMsg.Empty();
	strMsgTmp.Empty();

	memset(&InputStringFormat, NULL, sizeof(TBLINPUTSTRING));
	if (!IPM_LoadInputString(&InputStringFormat))
	{
		IPM_TblFreeInputString(&InputStringFormat);
		return FALSE; // memory allocated error or string ID unknown.
	}

	IntegrityInfo = LIBMON_table_integrity( pItem->GetNodeHandle(),
	                                        (LPTSTR)(LPCTSTR)pItem->GetDbName(),
	                                        iDb1,iDb2, pItem->GetTable_no(),
	                                        iCddsNo , tcTimeBegin, tcTimeEnd, 
	                                        tcColOrder,&InputStringFormat,&ErrorParameters);
	ManageError4Integrity(&ErrorParameters);
	if (IntegrityInfo != NULL)  {  
		strOutput = IntegrityInfo;
		LIBMON_Vdba_FreeMemory(&IntegrityInfo);
	}
	else
		strOutput.Empty();
	IPM_TblFreeInputString(&InputStringFormat);

	return TRUE;
}


BOOL IPM_GetCddsNumber(int iHandle, CString& strDataBaseName, CStringList& listCdds)
{
	int     ires,index;
	char    buf [MAXOBJECTNAME*2];
	char    buffilter [MAXOBJECTNAME];
	LPUCHAR parentstrings [MAXPLEVEL];
	CString csTempo;

	index = strDataBaseName.Find(_T("::"));
	if (index != -1)
		csTempo = strDataBaseName.Mid(index+2);
	else 
		csTempo = strDataBaseName;

	parentstrings [0] = (LPUCHAR)(LPCTSTR)csTempo;
	parentstrings [1] = NULL;
	ires = DOMGetFirstObject (
		iHandle, 
		OT_REPLIC_CDDS, 
		1,
		parentstrings, 
		FALSE,
		NULL, 
		(LPUCHAR)buf,
		NULL, 
		NULL);

	while (ires == RES_SUCCESS)
	{
		listCdds.AddTail((LPCTSTR)buf);
		ires    = DOMGetNextObject ((LPUCHAR)buf, (LPUCHAR)buffilter, NULL);
	}

	return TRUE;
}

BOOL IPM_GetDatabaseConnection(int iHandle, CString& strDataBaseName, CStringList& listConnection)
{
	int     ires,index;
	char    buf [MAXOBJECTNAME*2];
	char    buffilter [MAXOBJECTNAME*2];
	char    bufOwner  [MAXOBJECTNAME];
	LPUCHAR parentstrings [MAXPLEVEL];
	CString csTempo;

	index = strDataBaseName.Find(_T("::"));
	if (index != -1)
		csTempo = strDataBaseName.Mid(index+2);
	else 
		csTempo = strDataBaseName;

	parentstrings [0] = (LPUCHAR)(LPCTSTR)csTempo;
	parentstrings [1] = NULL;
	ires = DOMGetFirstObject (
		iHandle, 
		OT_REPLIC_CONNECTION, 
		1,
		parentstrings, 
		FALSE, 
		NULL, 
		(LPUCHAR)buf, 
		(LPUCHAR)bufOwner ,
		(LPUCHAR)buffilter );

	while (ires == RES_SUCCESS)
	{
		listConnection.AddTail(buf);
		ires  = DOMGetNextObject ((LPUCHAR)buf, (LPUCHAR)bufOwner ,(LPUCHAR)buffilter );
	}

	return TRUE;
}

BOOL IPM_IsReplicatorInstalled(CdIpmDoc* pIpmDoc, LPCTSTR lpszDatabase)
{
	return MonIsReplicatorInstalled(pIpmDoc->GetNodeHandle(), lpszDatabase);
}

BOOL IPM_ShutDownServer (CdIpmDoc* pIpmDoc, SERVERDATAMIN* pServer, BOOL bSoft)
{
	int ret = MonShutDownSvr(pIpmDoc->GetNodeHandle(), pServer, bSoft);
	return (ret == RES_SUCCESS)? TRUE: FALSE;
}


BOOL IPM_OpenCloseServer(CdIpmDoc* pIpmDoc, SERVERDATAMIN* pServer, BOOL bOpen)
{
	int ret = MonOpenCloseSvr(pIpmDoc->GetNodeHandle(), pServer, bOpen);
	IPMUPDATEPARAMS ups;
	memset (&ups, 0, sizeof(ups));
	ups.nType = 0;
	ups.pStruct = NULL;
	CaIpmQueryInfo queryInfo (pIpmDoc, OT_MON_ALL, &ups);
	IPM_UpdateInfo(&queryInfo);

	return (ret == RES_SUCCESS)? TRUE: FALSE;
}

BOOL IPM_RemoveSession(CdIpmDoc* pIpmDoc, SESSIONDATAMIN* pSession)
{
	int ret = MonRemoveSession(pIpmDoc->GetNodeHandle(), pSession);
	return (ret == RES_SUCCESS)? TRUE: FALSE;
}

int IPM_StructSize(int iobjecttypeReq)
{
	return GetMonInfoStructSize(iobjecttypeReq);
}

BOOL IPM_FitWithFilter (CdIpmDoc* pDoc, int iobjtype, LPVOID pStructReq, LPSFILTER pfilter)
{
	// hNode not used in MonXyz() function
	ASSERT(pDoc);
	return MonFitWithFilter(iobjtype, pStructReq, pfilter);
}

BOOL IPM_CompleteStruct(CdIpmDoc* pIpmDoc, void* pstruct, int* poldtype, int* pnewtype)
{
	int iRes = MonCompleteStruct(pIpmDoc->GetNodeHandle(), pstruct, poldtype, pnewtype);
	return (iRes == RES_SUCCESS)? TRUE: FALSE;
}

int IPM_CompareInfo(int iobjecttype, void* pstruct1, void* pstruct2)
{
	return CompareMonInfo(iobjecttype, pstruct1, pstruct2);
}


BOOL IPM_PingAll(CStringList& listItem)
{
	TCHAR szVnodeName[175];//MAX_VNODE_NAME
	TCHAR szDBName[MAXDBNAME];
	CString strItem;
	ERRORPARAMETERS structErr;

	POSITION pos = listItem.GetHeadPosition();
	while (pos != NULL)
	{
		strItem = listItem.GetNext(pos);
		int nRes = GetNodeAndDB((LPUCHAR)szVnodeName, (LPUCHAR)szDBName, (LPUCHAR)(LPCTSTR)strItem);
		if (nRes == RES_SUCCESS) 
		{
			nRes = LIBMON_SendEvent(szVnodeName, szDBName, _T("DD_PING"), NULL, _T(""),&structErr);
			if (structErr.iErrorID != RES_SUCCESS)
				ManageError4SendEvent(&structErr);
				//AfxMessageBox(IDS_E_PING_FAILED);//"Ping server failed."
		}
	}
	return TRUE;
}

BOOL IPM_GetRelatedInfo (CdIpmDoc* pIpmDoc, int iobjecttypeSrc, LPVOID pStruct, int iobjecttypeDest, LPVOID pStructDest)
{
	int nres = MonGetRelatedInfo (pIpmDoc->GetNodeHandle(), iobjecttypeSrc, pStruct, iobjecttypeDest, pStructDest);
	return (nres == RES_SUCCESS)? TRUE: FALSE;
}

BOOL IPM_UpdateInfo(CaIpmQueryInfo* pInfo)
{
	CdIpmDoc* pIpmDoc = pInfo->GetDocument();
	IPMUPDATEPARAMS* pUps = pInfo->GetUpdateParam();
	int iobjecttypeParent = pUps? pUps->nType: 0;
	LPVOID pstructParent  = pUps? pUps->pStruct: NULL;
	int iobjecttypeReq    = pInfo->GetRegObjectType();
	UpdateMonInfo(pIpmDoc->GetNodeHandle(), iobjecttypeParent, pstructParent, iobjecttypeReq);

	return TRUE;
}

BOOL IPM_NeedBkRefresh(CdIpmDoc* pIpmDoc)
{
	return LIBMON_NeedBkRefresh (pIpmDoc->GetNodeHandle()) ;
}


BOOL IPM_ReplicationGetQueues(CdIpmDoc* pIpmDoc, RESOURCEDATAMIN* pSvrDta, int& nInputQueue, int& nDistributionQueue)
{
	int iRes = 0;
	int nIQ = 0, nDQ = 0;
	TCHAR tchszDbName[100];
	BOOL bOK = IPM_GetInfoName(pIpmDoc, OT_DATABASE, pSvrDta, tchszDbName, sizeof(tchszDbName));
	if (bOK)
	{
		iRes = LIBMON_ReplMonGetQueues(pIpmDoc->GetNodeHandle(), (LPUCHAR)tchszDbName, &nIQ, &nDQ);
	}
	nInputQueue = nIQ;
	nDistributionQueue = nDQ;

	return (iRes == RES_SUCCESS)? TRUE: FALSE;
}

BOOL IPM_InitializeReplicator(CdIpmDoc* pIpmDoc, LPCTSTR lpszDatabase)
{
	ASSERT (pIpmDoc->GetNodeHandle() != -1);
	ASSERT (pIpmDoc->GetNodeHandleReplication() == -1);
	int nhReplMonHandle = 0;
	int res = DBEReplMonInit(pIpmDoc->GetNodeHandle(), (LPUCHAR)lpszDatabase, &nhReplMonHandle, pIpmDoc);
	if (res != RES_SUCCESS)
	{
		pIpmDoc->SetNodeHandleReplication(-1);
		return FALSE;
	}

	pIpmDoc->SetNodeHandleReplication(nhReplMonHandle);
	return TRUE;
}

BOOL IPM_TerminateReplicator(CdIpmDoc* pIpmDoc)
{
	int nhReplMonHandle = pIpmDoc->GetNodeHandleReplication();
	if (nhReplMonHandle != -1) 
	{
		int res = DBEReplMonTerminate(nhReplMonHandle, pIpmDoc);
		ASSERT (res == RES_SUCCESS);
		if (res == RES_SUCCESS)
		{
			pIpmDoc->SetNodeHandleReplication(-1);
			return TRUE;
		}
	}

	return FALSE;
}


BOOL IPM_ReplicationServerChangeRunNode(CdIpmDoc* pIpmDoc)
{
	CTreeItem* pItem = pIpmDoc->GetSelectedTreeItem();
	ASSERT (pItem);
	if (!pItem)
		return FALSE;
	ASSERT (pItem->GetType() == OT_MON_REPLIC_SERVER);
	if (pItem->GetType() != OT_MON_REPLIC_SERVER)
		return FALSE;

	LPREPLICSERVERDATAMIN lpData = (LPREPLICSERVERDATAMIN)pItem->GetPTreeItemData()->GetDataPtr();
	ASSERT (lpData);
	if (!lpData)
		return FALSE;
	if (lpData->startstatus == REPSVR_STARTED)
	{
		TCHAR tchszBuf[100];
		CString cMess;
		BOOL bOK = IPM_GetInfoName(pIpmDoc, OT_MON_REPLIC_SERVER, lpData, tchszBuf,sizeof(tchszBuf));
		cMess.Format(IDS_STOP_SVR_REPLIC, tchszBuf); //"Please Stop the Replication Server: %s\n"
		AfxMessageBox(cMess);                   //"before Changing the Run Node."
		return FALSE;
	}
	CxDlgReplicationRunNode dlg (pIpmDoc);
	memmove(&(dlg.m_ReplicSvrData), lpData, sizeof(REPLICSERVERDATAMIN));
	dlg.DoModal();

	IPMUPDATEPARAMS ups;
	memset (&ups, 0, sizeof(ups));
	ups.nType = 0;
	ups.pStruct = NULL;
	CaIpmQueryInfo queryInfo (pIpmDoc, OT_MON_ALL, &ups);
	IPM_UpdateInfo(&queryInfo);
	pIpmDoc->UpdateDisplay();

	return TRUE;
}

BOOL IPM_ReplicationRunNode(CdIpmDoc* pIpmDoc, REPLICSERVERDATAMIN& rsd, CString& NewRunNode)
{
	int ires;
	BOOL bNewRunNodeAvailable = FALSE;
	REPLICSERVERDATAMIN ReplicSvrDtaNewRunNode;
	REPLICSERVERDATAMIN ReplicSvrDta;
	RESOURCEDATAMIN  ResDtaMin;

	RAISEDREPLICDBE dbe;
	DBEScanDBEvents(NULL,&dbe);

	memmove(&ReplicSvrDtaNewRunNode, &rsd, sizeof(REPLICSERVERDATAMIN));


	if (!NewRunNode.IsEmpty()) {
		if ( NewRunNode != CString(rsd.RunNode) ) {
			CString csLocal;
			csLocal.LoadString(IDS_I_LOCALNODE);
			if ( NewRunNode.Find(csLocal) != -1) {
				CString cTempo, cWithoutLocal,cT1 = ") ";
				int index;
				cTempo = NewRunNode;
				index = cTempo.Find( cT1 );
				cWithoutLocal = cTempo.Right((cTempo.GetLength() - (index + cT1.GetLength())));
				lstrcpy ((char *)ReplicSvrDtaNewRunNode.RunNode,cWithoutLocal);
			}
			else 
				lstrcpy ((char *)ReplicSvrDtaNewRunNode.RunNode,NewRunNode);

			FillResStructFromDB(&ResDtaMin, rsd.ParentDataBaseName);

			ires=GetFirstMonInfo(pIpmDoc->GetNodeHandle(),OT_DATABASE,&ResDtaMin,
								 OT_MON_REPLIC_SERVER,(void * )&ReplicSvrDta,NULL);
			while (ires==RES_SUCCESS) {
				if (lstrcmpi((char *)ReplicSvrDtaNewRunNode.RunNode,(char *)ReplicSvrDta.RunNode) == 0 &&
					ReplicSvrDtaNewRunNode.serverno == ReplicSvrDta.serverno && 
					ReplicSvrDtaNewRunNode.localdb != ReplicSvrDta.localdb ) {
						//"Error: Two servers cannot run on the same node, "
						//"with the same number, and different local Databases."
						AfxMessageBox(IDS_E_RUN_SERVER_REPLIC);
						bNewRunNodeAvailable = FALSE;
						break;
				}
				if ( (lstrcmpi((char *)ReplicSvrDtaNewRunNode.RunNode,(char *)ReplicSvrDta.RunNode) == 0 &&
					 ReplicSvrDtaNewRunNode.serverno == ReplicSvrDta.serverno &&
					 ReplicSvrDtaNewRunNode.localdb == ReplicSvrDta.localdb &&
					 ReplicSvrDta.startstatus == REPSVR_STARTED) ||
					 (lstrcmpi((char *)rsd.RunNode,(char *)ReplicSvrDta.RunNode) == 0 &&
					 rsd.serverno == ReplicSvrDta.serverno &&
					 rsd.localdb == ReplicSvrDta.localdb &&
					 ReplicSvrDta.startstatus == REPSVR_STARTED) ) {
					CString cMess;
					char buf[100];
					GetMonInfoName(pIpmDoc->GetNodeHandle(), OT_MON_REPLIC_SERVER,
								   &ReplicSvrDta, buf,sizeof(buf));
					//"Please Stop the Replication Server: %s\n"
					//"before Changing the Run Node."
					cMess.Format(IDS_F_STOP_REPLIC_SVR,buf);
					AfxMessageBox(cMess);
					bNewRunNodeAvailable = FALSE;
					break;
				}
				else
					bNewRunNodeAvailable = TRUE;
				ires=GetNextMonInfo((void * )&ReplicSvrDta);
			}
		}
		
		if (bNewRunNodeAvailable) {

			ires = LIBMON_FilledSpecialRunNode(pIpmDoc->GetNodeHandle(),
			                                   &ResDtaMin,
			                                   &rsd,
			                                   &ReplicSvrDtaNewRunNode);
			if (ires != RES_SUCCESS) {
				//_T("Cannot update Replicator System Catalog.")
				AfxMessageBox(IDS_E_REPLIC_UPDATE_FAILED);
			}
			else {
				if (GetOIVers() == OIVERS_12 || GetOIVers() == OIVERS_20)
					//"Don't forget to rebuild the Replication Server(s) (from a DOM window) "
					AfxMessageBox(IDS_I_DONT_FORGET_REBUILD);
			}
				
		}
		
	}
	return TRUE;
}


BOOL IPM_GetRaisedDBEvents(CdIpmDoc* pIpmDoc, RAISEDREPLICDBE* pRaisedDBEvent, BOOL& bHasEvent, BOOL& bUpdateTree)
{
	int nRes = DBEScanDBEvents(pIpmDoc,pRaisedDBEvent);
	bHasEvent = FALSE;
	bUpdateTree = FALSE;
	if (nRes == RES_ERR)
		return FALSE;
	if (nRes & MASK_DBEEVENTTHERE)
		bHasEvent = TRUE;
	if (nRes & MASK_NEED_REFRESH_MONTREE)
		bUpdateTree = TRUE;
	return TRUE;
}


BOOL IPM_IsFirstAccessDataAfterLoad(CdIpmDoc* pDoc)
{
	if (pDoc)
		return isMonSpecialState (pDoc->GetNodeHandle());
	return FALSE;
}

void IPM_SetFirstAccessDataAfterLoad(CdIpmDoc* pDoc, BOOL bFirstAccess)
{
	if (bFirstAccess == FALSE)
	{
		SetMonNormalState (pDoc->GetNodeHandle());
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
// Utilities 

TCHAR* IPM_GetLockTableNameStr(LPLOCKDATAMIN lplock, TCHAR* bufres, int bufsize)
{
	return GetLockTableNameStr(lplock, bufres, bufsize);
}

TCHAR* IPM_GetLockPageNameStr (LPLOCKDATAMIN lplock, TCHAR* bufres)
{
	return GetLockPageNameStr (lplock, bufres);
}

TCHAR* IPM_GetResTableNameStr(LPRESOURCEDATAMIN lpres, TCHAR* bufres, int bufsize)
{
	return GetResTableNameStr(lpres, bufres, bufsize);
}

TCHAR* IPM_GetResPageNameStr (LPRESOURCEDATAMIN lpres, TCHAR* bufres)
{
	return GetResPageNameStr (lpres, bufres);
}

TCHAR* IPM_GetTransactionString(int hdl, long tx_high, long tx_low, TCHAR* bufres, int bufsize)
{
	return GetTransactionString(hdl, tx_high, tx_low, bufres, bufsize);
}


UINT GetServerImageId(LPSERVERDATAMIN lps)
{
	switch (lps->servertype) 
	{
	case SVR_TYPE_DBMS:
		return IDB_TM_SERVER_TYPE_INGRES;
	case SVR_TYPE_GCN:
		return IDB_TM_SERVER_TYPE_NAME;
	case SVR_TYPE_NET:
		return IDB_TM_SERVER_TYPE_NET;
	case SVR_TYPE_STAR:
		return IDB_TM_SERVER_TYPE_STAR;
	case SVR_TYPE_RECOVERY:
		return IDB_TM_SERVER_TYPE_RECOVERY;
	case SVR_TYPE_ICE:
		return IDB_TM_SERVER_TYPE_ICE;
	case SVR_TYPE_RMCMD:
		return IDB_TM_SERVER_TYPE_RMCMD;
	case SVR_TYPE_JDBC:
	case SVR_TYPE_DASVR:
		return IDB_TM_SERVER_TYPE_JDBC;
	case SVR_TYPE_OTHER:
	default:
		return IDB_TM_SERVER_TYPE_OTHER;
	}
	ASSERT (FALSE);
	return -1;
}


UINT GetLockImageId(LPLOCKDATAMIN lpl)
{
	switch(lpl->locktype) 
	{
	case LOCK_TYPE_DB:
		return IDB_TM_LOCK_TYPE_DB;
	case LOCK_TYPE_TABLE:
		return IDB_TM_LOCK_TYPE_TABLE;
	case LOCK_TYPE_PAGE:
		return IDB_TM_LOCK_TYPE_PAGE;
	case LOCK_TYPE_OTHER:
	default:
		return IDB_TM_LOCK_TYPE_OTHER;
	}
	ASSERT (FALSE);
	return -1;
}

UINT GetResourceImageId(LPRESOURCEDATAMIN lpr)
{
	LOCKDATAMIN ldm;

	// duplicate from dbaginfo.sc line 1400
	switch (lpr->resource_type) 
	{
	case 1:
		ldm.locktype = LOCK_TYPE_DB;
		break;
	case 2:
		ldm.locktype = LOCK_TYPE_TABLE;
		break;
	case 3:
		ldm.locktype = LOCK_TYPE_PAGE;
		break;
	default :
		ldm.locktype = LOCK_TYPE_OTHER;
	}
	UINT uiLockImageId = GetLockImageId(&ldm);

	// complimentary custom for databases sub-types
	if (lpr->resource_type == 1)
		uiLockImageId = GetDbTypeImageId(lpr);

	return uiLockImageId;
}

UINT GetLocklistImageId(LPLOCKLISTDATAMIN lpl)
{
	if (lpl->locklist_wait_id != 0L)
		return IDB_TM_LOCKLIST_BLOCKED_YES;
	else
		return IDB_TM_LOCKLIST_BLOCKED_NO;
}

UINT GetDbTypeImageId(LPRESOURCEDATAMIN lpr)
{
	switch (lpr->dbtype) 
	{
	case DBTYPE_REGULAR:
		return IDB_TM_DB_TYPE_REGULAR;
	case DBTYPE_DISTRIBUTED:
		return IDB_TM_DB_TYPE_DISTRIBUTED;
	case DBTYPE_COORDINATOR:
		return IDB_TM_DB_TYPE_COORDINATOR;
	case DBTYPE_ERROR:
		return IDB_TM_DB_TYPE_ERROR;
	default:
		ASSERT (FALSE); // Unexpected ---> trapped in debug mode
		return IDB_TM_DB_TYPE_REGULAR;  // for release mode: does not 'catch the eye' of the user
	}
	ASSERT (FALSE);
	return -1;
}

UINT GetDbTypeImageId(LPLOGDBDATAMIN lpldb)
{
	switch (lpldb->dbtype)
	{
	case DBTYPE_REGULAR:
		return IDB_TM_DB_TYPE_REGULAR;
	case DBTYPE_DISTRIBUTED:
		return IDB_TM_DB_TYPE_DISTRIBUTED;
	case DBTYPE_COORDINATOR:
		return IDB_TM_DB_TYPE_COORDINATOR;
	case DBTYPE_ERROR:
		return IDB_TM_DB_TYPE_ERROR;
	default:
		ASSERT (FALSE); // Unexpected ---> trapped in debug mode
		return IDB_TM_DB_TYPE_REGULAR;  // for release mode: does not 'catch the eye' of the user
	}
	ASSERT (FALSE);
	return -1;
}


int  MonGetReplicatorVersion(int hNode, LPCTSTR dbName)
{
	int     replicVer;
	char    szOwnerName[MAXOBJECTNAME];
	char    dummyBuf[MAXOBJECTNAME];
	LPUCHAR aDummyParents[MAXPLEVEL];
	int     dummyType;

	// 1) get ownername for database
	aDummyParents[0] = aDummyParents[1] = aDummyParents[2] = NULL;
	int result = DOMGetObjectLimitedInfo(hNode,
		(LPUCHAR)dbName,          // db name
		(LPUCHAR)"",              // parent
		OT_DATABASE,              // type
		0,                        // parent level
		aDummyParents,            // dummy parents table
		TRUE,                     // accept system object (hypotetical replication on Coordinator Database...)
		&dummyType,               // result type
		(LPUCHAR)dummyBuf,        // result objname
		(LPUCHAR)szOwnerName,     // result owner
		(LPUCHAR)dummyBuf);       // result extradata
	ASSERT (result != RES_ERR);
	if (result == RES_ERR)
		return REPLIC_NOINSTALL;

	// 2) get version number
	// can be REPLIC_V10, REPLIC_V105, REPLIC_V11 or REPLIC_NOINSTALL
	replicVer = GetReplicInstallStatus(hNode, (LPUCHAR)(LPCTSTR)dbName, (LPUCHAR)szOwnerName);

	// 3) return value
	return replicVer;
}

BOOL MonIsReplicatorInstalled(int hNode, LPCTSTR dbName)
{
	int replicVer = MonGetReplicatorVersion(hNode, dbName);
	if (replicVer == REPLIC_NOINSTALL || replicVer==REPLIC_VER_ERR)
		return FALSE;
	else
		return TRUE;
}

//
// Public Utility functions
//

CString IPM_BuildFullNodeName(LPNODESVRCLASSDATAMIN pServerDataMin)
{
	CString name(pServerDataMin->NodeName);
	name += LPCLASSPREFIXINNODENAME;
	name += CString(pServerDataMin->ServerClass);
	name += LPCLASSSUFFIXINNODENAME;
	return name;
}

CString IPM_BuildFullNodeName(LPNODEUSERDATAMIN pUserDataMin)
{
	CString name(pUserDataMin->NodeName);

	if (pUserDataMin->ServerClass[0] != '\0') {
		name += LPCLASSPREFIXINNODENAME;
		name += CString(pUserDataMin->ServerClass);
		name += LPCLASSSUFFIXINNODENAME;
	}

	name += LPUSERPREFIXINNODENAME;
	name += CString(pUserDataMin->User);
	name += LPUSERSUFFIXINNODENAME;
	return name;
}

BOOL IPM_GetSvrPageEvents(CdIpmDoc* pIpmDoc,CaReplicRaiseDbeventList &EventsList,LPREPLICSERVERDATAMIN pserverdatamin)
{
	CString cDefDbName,csDBAOwner,csVnodeAndUsers;
	int ires,nNodeHandle,NodeHdl,irestype;
	UCHAR DBAUsernameOntarget[MAXOBJECTNAME];
	UCHAR buf[EXTRADATASIZE];
	UCHAR extradata[EXTRADATASIZE];
	LPUCHAR parentstrings [MAXPLEVEL];

	NodeHdl  = LIBMON_OpenNodeStruct (pserverdatamin->LocalDBNode);
	if (NodeHdl == -1)
		return FALSE;
	
	// Temporary for activate a session
	ires = DOMGetFirstObject (NodeHdl, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL);
	cDefDbName.Format(_T("%s::%s"),pserverdatamin->LocalDBNode,pserverdatamin->LocalDBName);
	//
	//Get DBA user name for this database
	parentstrings[0]=pserverdatamin->LocalDBName;
	parentstrings[1]=NULL;
	memset (DBAUsernameOntarget,'\0',sizeof(DBAUsernameOntarget));
	ires = DOMGetObjectLimitedInfo(	NodeHdl,
									parentstrings [0],
									(UCHAR *)"",
									OT_DATABASE,
									0,
									parentstrings,
									TRUE,
									&irestype,
									buf,
									DBAUsernameOntarget,
									extradata
								);
	if (ires != RES_SUCCESS) {
		LIBMON_CloseNodeStruct(NodeHdl);
		return FALSE;
	}

	LIBMON_CloseNodeStruct(NodeHdl);

	csDBAOwner=DBAUsernameOntarget;
	csVnodeAndUsers.Format(
		_T("%s%s%s%s"),
		pserverdatamin->LocalDBNode,
		LPUSERPREFIXINNODENAME,
		(LPCTSTR)csDBAOwner,
		LPUSERSUFFIXINNODENAME);


	nNodeHandle  = LIBMON_OpenNodeStruct ((LPUCHAR)(LPCTSTR)csVnodeAndUsers);
	if (nNodeHandle == -1)
		return FALSE;

	// Temporary for activate a session
	ires = DOMGetFirstObject (nNodeHandle, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL);
	cDefDbName.Format(_T("%s::%s"),pserverdatamin->LocalDBNode, pserverdatamin->LocalDBName);
	
	while (!EventsList.IsEmpty())
		delete EventsList.RemoveHead();

	EventsList.DefineAllDbeventWithDefaultValues( nNodeHandle, cDefDbName,pserverdatamin->serverno); 

	LIBMON_CloseNodeStruct (nNodeHandle);

	return TRUE;
}

int GetDbaOwner(const CString csVnode,const CString csDatabase ,CString & csDbaOwnerLocal)
{
	UCHAR DBAUsername[MAXOBJECTNAME],buf[MAXOBJECTNAME],extradata[EXTRADATASIZE];
	LPUCHAR parentstrings [MAXPLEVEL];
	int iret,irestype,hdlNode;
	CString strMsg1,strMsg2;
	UCHAR udummy[] = "";
	parentstrings [0] = (LPUCHAR)(LPCTSTR)csDatabase;
	parentstrings [1] = NULL;

	hdlNode = LIBMON_OpenNodeStruct ((LPUCHAR)(LPCTSTR)csVnode);
	if (hdlNode<0) {
		strMsg1.LoadString( IDS_MAX_NB_CONNECT );
		//strMsg1 += strMsg2.LoadString( IDS_E_UPDATE_COLLISION );      // " - Cannot update collision(s)."
		strMsg1 = _T(" - Cannot update collision(s).");
		AfxMessageBox (strMsg1);
		return RES_ERR;
	}
	DOMGetFirstObject (hdlNode, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL);

	//Get DBA user name for this database
	iret = DOMGetObjectLimitedInfo(hdlNode,
							parentstrings [0],
							udummy,
							OT_DATABASE,
							0,
							parentstrings,
							TRUE,
							&irestype,
							buf,
							DBAUsername,
							extradata
							);
	LIBMON_CloseNodeStruct(hdlNode);
	if (iret != RES_SUCCESS) {
		//strMsg1.Format(IDS_F_COLLISION_DBA_OWNER,(LPCTSTR)csDatabase);
		//strMsg1 += strMsg2.LoadString(IDS_E_UPDATE_COLLISION);      // " - Cannot update collision(s)."
		strMsg1 = _T(" - Cannot update collision(s).");
		AfxMessageBox (strMsg1);
		return RES_ERR;
	}
	csDbaOwnerLocal = DBAUsername;
	return RES_SUCCESS;
}

BOOL IPM_ResolveCurrentTransaction(CdIpmDoc* pIpmDoc, CaCollisionItem* pItem)
{
	int hdlLocal  =-1;
	int hdlTarget =-1;
	int ires1,ires2,ires3;
	UCHAR buf[EXTRADATASIZE],nodeNameNoUser[MAXOBJECTNAME];
	UCHAR szConnectNameTarget[2*MAXOBJECTNAME],szConnectNameLocal[2*MAXOBJECTNAME];
	int iSessionLoc,iSessionTar,nResolveType;
	CString csNodeUserTarget,csDbaTarget,csNodeUserLocal,csDbaLocal;
	CaCollisionSequence* pSeq = NULL;
	CaCollisionTransaction* pTrans = (CaCollisionTransaction*)pItem;
	CTypedPtrList<CObList, CaCollisionSequence*>& listSeq = pTrans->GetListCollisionSequence();

	POSITION pos = listSeq.GetHeadPosition();
	ASSERT(pos);
	pSeq = listSeq.GetNext (pos);
	// OpenNodeStruct() on LocalVnode and TargetVnode
	ASSERT(!pSeq->m_strTargetVNode.IsEmpty());
	ASSERT(!pSeq->m_strLocalVNode.IsEmpty());
	// Format string with vnode name and dba name.
	if ( GetDbaOwner(pSeq->m_strLocalVNode ,
		             pSeq->m_strLocalDatabase ,csDbaLocal) == RES_ERR)
		return FALSE;

		lstrcpy((LPSTR)nodeNameNoUser,(LPCTSTR)pSeq->m_strLocalVNode);
		RemoveConnectUserFromString(nodeNameNoUser);
		csNodeUserLocal  = nodeNameNoUser;
		csNodeUserLocal += LPUSERPREFIXINNODENAME;
		csNodeUserLocal += csDbaLocal;
		csNodeUserLocal += LPUSERSUFFIXINNODENAME;

		// Format string with vnode name and dba name.
		if ( GetDbaOwner(pSeq->m_strTargetVNode ,
		                 pSeq->m_strTargetDatabase , csDbaTarget) == RES_ERR )
			return FALSE;
		csNodeUserTarget  = pSeq->m_strTargetVNode;
		csNodeUserTarget += LPUSERPREFIXINNODENAME;
		csNodeUserTarget += csDbaTarget;
		csNodeUserTarget += LPUSERSUFFIXINNODENAME;

		if (csNodeUserLocal != csNodeUserTarget)
		{
			hdlLocal  = LIBMON_OpenNodeStruct ((LPUCHAR)(LPCTSTR)csNodeUserLocal);
			hdlTarget = LIBMON_OpenNodeStruct ((LPUCHAR)(LPCTSTR)csNodeUserTarget);
			if (hdlLocal<0 || hdlTarget<0)
			{
				CString strMsg;
				strMsg.LoadString(IDS_MAX_NB_CONNECT);  // _T("Maximum number of connections has been reached"
				//strMsg += VDBA_MfcResourceString (IDS_E_UPDATE_COLLISION);      // " - Cannot update collision(s)."
				strMsg += _T(" - Cannot update collision(s).");
				AfxMessageBox (strMsg);
				if (hdlLocal>=0)
					LIBMON_CloseNodeStruct(hdlLocal);
				if (hdlTarget>=0)
					LIBMON_CloseNodeStruct(hdlTarget);
				return FALSE;
			}
			DOMGetFirstObject ( hdlLocal,
			                    OT_DATABASE, 0, NULL,
			                    FALSE, NULL, buf, NULL, NULL);
			DOMGetFirstObject ( hdlTarget,
			                    OT_DATABASE, 0, NULL,
			                    FALSE, NULL, buf, NULL, NULL);
		}
		else
		{
			hdlLocal  = LIBMON_OpenNodeStruct ((LPUCHAR)(LPCTSTR)csNodeUserLocal);
			if (hdlLocal<0)
			{
				CString strMsg,strMsg2;
				strMsg.LoadString(IDS_MAX_NB_CONNECT);  // _T("Maximum number of connections has been reached"
				//strMsg += strMsg2.Loadstring(IDS_E_UPDATE_COLLISION);      // " - Cannot update collision(s)."
				strMsg += _T(" - Cannot update collision(s).");
				AfxMessageBox (strMsg);
				return FALSE;
			}
			DOMGetFirstObject ( hdlLocal,
			                    OT_DATABASE, 0, NULL,
			                    FALSE, NULL, buf, NULL, NULL);
		}

		wsprintf((char *)szConnectNameTarget,"%s::%s",csNodeUserTarget,pSeq->m_strTargetDatabase);
		wsprintf((char *)szConnectNameLocal ,"%s::%s",csNodeUserLocal ,pSeq->m_strLocalDatabase);
		ires1 = Getsession(szConnectNameLocal,SESSION_TYPE_CACHEREADLOCK, &iSessionLoc);
		ires2 = Getsession(szConnectNameTarget,SESSION_TYPE_CACHEREADLOCK, &iSessionTar);
		if (ires1!=RES_SUCCESS || ires2!=RES_SUCCESS)
		{
			AfxMessageBox(IDS_E_GET_SESSION);
			//MessageWithHistoryButton(m_hWnd,VDBA_MfcResourceString ());//"Connection failed."
			if (ires1 == RES_SUCCESS)
				ReleaseSession(iSessionLoc,RELEASE_ROLLBACK);
			if (ires2 == RES_SUCCESS)
				ReleaseSession(iSessionTar,RELEASE_ROLLBACK);
			if (hdlLocal>=0)
				LIBMON_CloseNodeStruct(hdlLocal);
			if (hdlTarget>=0)
				LIBMON_CloseNodeStruct(hdlTarget);
			return FALSE;
		}
		pos = listSeq.GetHeadPosition();
		while (pos != NULL)
		{
			pSeq = listSeq.GetNext (pos);
			if ( pSeq->GetResolvedType() == CaCollisionItem::PREVAIL_SOURCE)
				nResolveType = -1;
			else if ( pSeq->GetResolvedType() == CaCollisionItem::PREVAIL_TARGET)
				nResolveType = -2;
			else
				break;
			ERRORPARAMETERS ErrorParam;
			ActivateSession(iSessionLoc);
			ires1 = LIBMON_UpdateTable4ResolveCollision(nResolveType ,
			                                            pSeq->m_nSourceDB,
			                                            pSeq->m_nTransaction,
			                                            pSeq->m_nSequenceNo ,
			                                            (LPSTR)(LPCTSTR)pSeq->m_strTable,
			                                            pSeq->m_nTblNo,&ErrorParam);
			if ( ires1 == RES_ERR)
				break;
			ActivateSession(iSessionTar);
			if ( pSeq->GetResolvedType() == CaCollisionItem::PREVAIL_SOURCE)
				nResolveType = -2;
			else if ( pSeq->GetResolvedType() == CaCollisionItem::PREVAIL_TARGET)
				nResolveType = -1;
			else
				break;
			ires2 = LIBMON_UpdateTable4ResolveCollision(nResolveType ,
			                                            pSeq->m_nSourceDB,
			                                            pSeq->m_nTransaction,
			                                            pSeq->m_nSequenceNo ,
			                                            (LPSTR)(LPCTSTR)pSeq->m_strTable,
			                                            pSeq->m_nTblNo,&ErrorParam);
			if ( ires2 == RES_ERR)
				break;
			pSeq->SetResolved();
		}
		ActivateSession(iSessionLoc);
		if (ires1 == RES_ERR)
			ReleaseSession(iSessionLoc,RELEASE_ROLLBACK);
		else
			ReleaseSession(iSessionLoc,RELEASE_COMMIT);
		ActivateSession(iSessionTar);
		if (ires2 == RES_ERR)
			ReleaseSession(iSessionTar,RELEASE_ROLLBACK);
		else
			ReleaseSession(iSessionTar,RELEASE_COMMIT);

		if (hdlLocal>=0)
			LIBMON_CloseNodeStruct(hdlLocal);
		if (hdlTarget>=0)
			LIBMON_CloseNodeStruct(hdlTarget);

		ERRORPARAMETERS ErrorParam;
		ires3 = LIBMON_SendEvent( (LPSTR)nodeNameNoUser,
		                          (LPSTR)(LPCTSTR)pSeq->m_strLocalDatabase,
		                          "dd_go_server","","",&ErrorParam);
		if (ires1 == RES_ERR || ires2 == RES_ERR || ires3 == RES_ERR)
		{
			pTrans->SetResolved(FALSE);
			return FALSE;
		}
		else
			pTrans->SetResolved();
	return TRUE;
}

BOOL IPM_SerializeChache(CArchive& ar)
{
	int nLen;
	if (ar.IsStoring())
	{
		void* pCachedata;
		int ires = LIBMON_SaveCache(&pCachedata, &nLen);
		if (ires !=RES_SUCCESS)
		{
			ar << 0;
			return TRUE;
		}
		else
		{
			ar << nLen;
			ar.Write ((const void*)pCachedata, nLen);
			LIBMON_FreeSaveCacheBuffer(pCachedata);
		}
	}
	else
	{
		ar >> nLen;
		if (nLen > 0)
		{
			BYTE* pBuffer = new BYTE[nLen];
			UINT nRead = ar.Read ((void*)pBuffer, nLen);
			ASSERT(nRead == (UINT)nLen);
			int ires = LIBMON_LoadCache(pBuffer, nLen);
			delete pBuffer;
		}
	}
	return TRUE;
}

void IPM_SetNormalState(CdIpmDoc* pIpmDoc)
{
	//
	// Mandatory BEFORE UpdateMonDisplay() otherwise stack overflow
	SetMonNormalState(pIpmDoc->GetNodeHandle());
}

void IPM_ErrorLogFile(LPCTSTR lpszFullFileName)
{
	LIBMON_SetTemporaryFileName((LPTSTR)(LPCTSTR) lpszFullFileName);
}
