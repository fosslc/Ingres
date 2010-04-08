/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
** Source   : ipmdml.h, Header File 
** Project  : Ingres II/ (Vdba Monitoring)
** Author   : UK Sotheavut (uk$so01)
** Purpose  : Data manipulation: access the lowlevel data through 
**            Library or COM module.
** History:
**
** 12-Nov-2001 (uk$so01)
**    Created
*/

#if !defined (_IPMDML_HEADER)
#define _IPMDML_HEADER
#include "qryinfo.h" 
#include "usrexcep.h"
#include "taskanim.h"

class CdIpmDoc;
class CaReplicRaiseDbeventList;
class CaReplicationRaiseDbevent;
class CaCollisionItem;
class CaCollision;
class CaReplicIntegrityRegTable;

class CaSwitchSessionThread
{
public:
	CaSwitchSessionThread(BOOL bFirstEnable): m_bFirstEnable(bFirstEnable)
	{
		In();
		m_nOut = FALSE;
	}
	~CaSwitchSessionThread()
	{
		if (m_nOut)
			return;
		Out();
	}

	int In();
	int Out();


private:
	BOOL m_nOut;
	BOOL m_bFirstEnable;
};


class CeIpmException: public CeSqlException
{
public:
	CeIpmException(): CeSqlException() {SetErrorCode(-1);}
	CeIpmException(LPCTSTR lpszReason): CeSqlException(lpszReason){SetErrorCode(-1);}
	~CeIpmException(){};
};


class CaIpmQueryInfo: public CaLLQueryInfo
{
public:
	CaIpmQueryInfo(CdIpmDoc* pDoc, int nObjType, LPIPMUPDATEPARAMS pUps = NULL);
	CaIpmQueryInfo(CdIpmDoc* pDoc, int nObjType, LPIPMUPDATEPARAMS pUps, LPVOID lpStruct, int nStructSize);
	~CaIpmQueryInfo();

	CdIpmDoc* GetDocument(){return m_pIpmDoc;}
	IPMUPDATEPARAMS* GetUpdateParam() {return m_pUps;}
	LPVOID GetStruct(){return m_pStruct;}

	int GetRegObjectType(){return GetObjectType();}
	int GetStructSize(){return m_nRegSize;}
	CeIpmException& GetException(){return m_mException;}
	void SetReportError(BOOL bSet){m_bReportError = bSet;}
	BOOL IsReportError(){return m_bReportError;}

protected:
	CdIpmDoc* m_pIpmDoc;
	LPIPMUPDATEPARAMS m_pUps;
	LPVOID m_pStruct;
	int    m_nRegSize;
private:
	BOOL m_bAllocated;
	BOOL m_bReportError; // By default = TRUE; If FALSE It is up to the caller to report errors
	CeIpmException m_mException;
};


void IPM_SetSessionStart(long nStart);
void IPM_SetSessionDescription(LPCTSTR lpszSessionDescription);
void IPM_PropertiesChange(WPARAM wParam, LPARAM lParam);
BOOL IPM_Initiate(CdIpmDoc* pDoc);
BOOL IPM_TerminateDocument(CdIpmDoc* pDoc);
BOOL IPM_Initialize(HANDLE& hEventDataChange);
BOOL IPM_Exit(HANDLE& hEventDataChange);


BOOL IPM_GetLockSummaryInfo(CaIpmQueryInfo* pQueryInfo, LPLOCKSUMMARYDATAMIN presult, LPLOCKSUMMARYDATAMIN pstart, BOOL bSinceStartUp, BOOL bNowAction);
BOOL IPM_GetLogSummaryInfo (CaIpmQueryInfo* pQueryInfo, LPLOGSUMMARYDATAMIN presult, LPLOGSUMMARYDATAMIN pstart, BOOL bSinceStartUp, BOOL bNowAction);
BOOL IPM_llQueryDetailInfo (CaIpmQueryInfo* pQueryInfo, LPVOID lpInfoStruct);
BOOL IPM_llQueryInfo (CaIpmQueryInfo* pQueryInfo, CPtrList& listInfoStruct);
BOOL IPM_SetDbmsVersion(int nDbmsVersion);
BOOL IPM_GetInfoName(CdIpmDoc* pDoc, int nObjectType, LPVOID pData, LPTSTR lpszbufferName, int nBufferSize);
BOOL IPM_ReplicationGetQueues(CdIpmDoc* pIpmDoc, RESOURCEDATAMIN* pSvrDta, int& nInputQueue, int& nDistributionQueue);
BOOL IPM_GetDatabaseOwner(CaIpmQueryInfo* pQueryInfo, CString& strDatabaseOwner);
BOOL IPM_StartReplicationServer(CdIpmDoc* pIpmDoc, REPLICSERVERDATAMIN* pSvrDta, CString& strOutput);
BOOL IPM_StopReplicationServer(CdIpmDoc* pIpmDoc, REPLICSERVERDATAMIN* pSvrDta, CString& strOutput);
BOOL IPM_ViewReplicationServerLog(REPLICSERVERDATAMIN* pSvrDta, CString& strOutput);
BOOL IPM_CheckDistributionConfig(CdIpmDoc* pDoc, RESOURCEDATAMIN* pSvrDta, CString& strOutput);
BOOL IPM_RetrieveCollision(CdIpmDoc* pIpmDoc, RESOURCEDATAMIN* pSvrDta, CaCollision* pCollisionData);
BOOL IPM_QueryRaisedEvents(CdIpmDoc* pDoc, RESOURCEDATAMIN* pSvrDta, CaReplicRaiseDbeventList& EventsList);
BOOL IPM_GetAllReplicationServerStarted(CdIpmDoc* pDoc, RESOURCEDATAMIN* pStruct, CStringList* pcslSrvStarted);
BOOL IPM_SendEventToServers(CdIpmDoc* pIpmDoc,CString& strEventFlag,CString& strServerFlag,CPtrList& listInfoStruct);
BOOL IPM_QueryIntegrityOutput (CdIpmDoc* pIpmDoc, CaReplicIntegrityRegTable *pItem, int iDb1, int iDb2,int iCddsNo, TCHAR *tcTimeBegin, TCHAR *tcTimeEnd, TCHAR *tcColOrder, CString& strOutput);
BOOL IPM_SendRaisedEvents(CdIpmDoc* pDoc, LPREPLICSERVERDATAMIN pSvrDta, CaReplicationRaiseDbevent* Event);

BOOL IPM_GetCddsNumber(int iHandle, CString& strDataBaseName, CStringList& listCdds);
BOOL IPM_GetDatabaseConnection(int iHandle, CString& strDataBaseName, CStringList& listConnection);
BOOL IPM_IsReplicatorInstalled(CdIpmDoc* pIpmDoc, LPCTSTR lpszDatabase);
BOOL IPM_ShutDownServer (CdIpmDoc* pIpmDoc, SERVERDATAMIN* pServer, BOOL bSoft);
BOOL IPM_OpenCloseServer(CdIpmDoc* pIpmDoc, SERVERDATAMIN* pServer, BOOL bOpen);
BOOL IPM_RemoveSession(CdIpmDoc* pIpmDoc, SESSIONDATAMIN* pSession);
BOOL IPM_FitWithFilter (CdIpmDoc* pDoc, int iobjtype, LPVOID pStructReq, LPSFILTER pfilter);
BOOL IPM_CompleteStruct(CdIpmDoc* pIpmDoc, void* pstruct, int* poldtype, int* pnewtype);
BOOL IPM_PingAll(CStringList& listItem);
BOOL IPM_GetRelatedInfo (CdIpmDoc* pIpmDoc, int iobjecttypeSrc, LPVOID pStruct, int iobjecttypeDest, LPVOID pStructDest);
int  IPM_StructSize(int iobjecttypeReq);
int  IPM_CompareInfo(int iobjecttype, void* pstruct1, void* pstruct2);
BOOL IPM_UpdateInfo(CaIpmQueryInfo* pInfo);
BOOL IPM_InitializeReplicator(CdIpmDoc* pIpmDoc, LPCTSTR lpszDatabase);
BOOL IPM_TerminateReplicator(CdIpmDoc* pIpmDoc);
BOOL IPM_ReplicationServerChangeRunNode(CdIpmDoc* pIpmDoc);
BOOL IPM_ReplicationRunNode(CdIpmDoc* pIpmDoc, REPLICSERVERDATAMIN& rsd, CString& NewRunNode);
BOOL IPM_ResolveCurrentTransaction(CdIpmDoc* pIpmDoc, CaCollisionItem* pItem);
BOOL IPM_GetRaisedDBEvents(CdIpmDoc* pIpmDoc, RAISEDREPLICDBE* pRaisedDBEvent, BOOL& bHasEvent, BOOL& bUpdateTree);
BOOL IPM_IsFirstAccessDataAfterLoad(CdIpmDoc* pDoc);
void IPM_SetFirstAccessDataAfterLoad(CdIpmDoc* pDoc, BOOL bFirstAccess);

// public utilities
UINT GetServerImageId  (LPSERVERDATAMIN lps);
UINT GetLockImageId    (LPLOCKDATAMIN lpl);
UINT GetResourceImageId(LPRESOURCEDATAMIN lpr);
UINT GetDbTypeImageId  (LPLOGDBDATAMIN lpldb);
UINT GetDbTypeImageId  (LPRESOURCEDATAMIN lpr);
UINT GetLocklistImageId(LPLOCKLISTDATAMIN lpl);

TCHAR* IPM_GetLockTableNameStr(LPLOCKDATAMIN lplock, TCHAR* bufres, int bufsize);
TCHAR* IPM_GetLockPageNameStr (LPLOCKDATAMIN lplock, TCHAR* bufres);
TCHAR* IPM_GetResTableNameStr(LPRESOURCEDATAMIN lpres, TCHAR* bufres, int bufsize);
TCHAR* IPM_GetResPageNameStr (LPRESOURCEDATAMIN lpres, TCHAR* bufres);
TCHAR* IPM_GetTransactionString(int hdl, long tx_high, long tx_low, TCHAR* bufres, int bufsize);

BOOL MonIsReplicatorInstalled(int hNode, LPCTSTR dbName);
int  MonGetReplicatorVersion(int hNode, LPCTSTR dbName);
CString IPM_BuildFullNodeName(LPNODESVRCLASSDATAMIN pServerDataMin);
CString IPM_BuildFullNodeName(LPNODEUSERDATAMIN pUserDataMin);

BOOL IPM_GetSvrPageEvents(CdIpmDoc* pIpmDoc,CaReplicRaiseDbeventList &EventsList,LPREPLICSERVERDATAMIN pserverdatamin);
BOOL IPM_NeedBkRefresh(CdIpmDoc* pIpmDoc);
BOOL IPM_SerializeChache(CArchive& ar);
void IPM_SetNormalState(CdIpmDoc* pIpmDoc);
void IPM_ErrorLogFile(LPCTSTR lpszFullFileName);


#endif // _IPMDML_HEADER
