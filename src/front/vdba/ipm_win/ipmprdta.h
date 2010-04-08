/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : ipmprdta.h, Header File 
**    Project  : CA-OpenIngres/Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : The implementation of serialization of IPM Page (modeless dialog) 
**
** History:
**
** xx-Apr-1997 (uk$so01)
**    Created
** 19-May-2003 (uk$so01)
**    SIR #110269, Add Network trafic monitoring.
*/

#ifndef IPMPRDTA_HEADER
#define IPMPRDTA_HEADER

#include "repitem.h"
#include "repevent.h"
#include "collidta.h"
#include "repinteg.h" // CaReplicIntegrityRegTableList
#include "vnodedml.h"

typedef struct tagLOGINFOSUMMARY
{
	LOGSUMMARYDATAMIN logStruct0;
	LOGSUMMARYDATAMIN logStruct1;
	int   nUse;   // 0: Startup is used; 1: Now is used.
	TCHAR tchszStartTime [64];

} LOGINFOSUMMARY, FAR* LPLOGINFOSUMMARY;

typedef struct tagLOCKINFOSUMMARY
{
	LOCKSUMMARYDATAMIN locStruct0;
	LOCKSUMMARYDATAMIN locStruct1;
	int   nUse;   // 0: Startup is used; 1: Now is used.
	TCHAR tchszStartTime [64];
} LOCKINFOSUMMARY, FAR* LPLOCKINFOSUMMARY;


class CuIpmPropertyData: public CObject
{
	DECLARE_SERIAL (CuIpmPropertyData)
public:
	CuIpmPropertyData();
	CuIpmPropertyData(LPCTSTR lpszClassName);

	virtual ~CuIpmPropertyData(){};

	virtual void NotifyLoad (CWnd* pDlg);  
		// NotifyLoad() Send message WM_USER_IPMPAGE_LOADING to the Window 'pDlg'
		// WPARAM: The class name of the Sender.
		// LPARAM: The data structure depending on the class name of the Sender.
	virtual void Serialize (CArchive& ar);
	CString& GetClassName() {return m_strClassName;}
protected:
	CString m_strClassName;
};

//
// DETAIL: Active User
class CuIpmPropertyDataDetailActiveUser: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataDetailActiveUser)
public:
	CuIpmPropertyDataDetailActiveUser();
	virtual ~CuIpmPropertyDataDetailActiveUser(){};

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
};


//
// DETAIL: Database
class CuIpmPropertyDataDetailDatabase: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataDetailDatabase)
public:
	CuIpmPropertyDataDetailDatabase();
	virtual ~CuIpmPropertyDataDetailDatabase(){};

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
};


//
// DETAIL: Lock
class CuIpmPropertyDataDetailLock: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataDetailLock)
public:
	CuIpmPropertyDataDetailLock ();
	CuIpmPropertyDataDetailLock (LPLOCKDATAMAX pData);
	virtual ~CuIpmPropertyDataDetailLock(){};

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize	(CArchive& ar);
	
	
	LOCKDATAMAX m_dlgData;
};

//
// DETAIL: Lock List
class CuIpmPropertyDataDetailLockList: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataDetailLockList)
public:
	CuIpmPropertyDataDetailLockList ();
	CuIpmPropertyDataDetailLockList (LPLOCKLISTDATAMAX pData);
	virtual ~CuIpmPropertyDataDetailLockList(){};

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize(CArchive& ar);
	
	
	LOCKLISTDATAMAX m_dlgData;
};

//
// DETAIL: Process
class CuIpmPropertyDataDetailProcess: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataDetailProcess)
public:
	CuIpmPropertyDataDetailProcess ();
	CuIpmPropertyDataDetailProcess (LPLOGPROCESSDATAMAX pData);
	virtual ~CuIpmPropertyDataDetailProcess(){};

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	
	LOGPROCESSDATAMAX m_dlgData;
};

//
// DETAIL: Resource
class CuIpmPropertyDataDetailResource: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataDetailResource)
public:
	CuIpmPropertyDataDetailResource ();
	CuIpmPropertyDataDetailResource (LPRESOURCEDATAMAX pData);
	virtual ~CuIpmPropertyDataDetailResource(){};

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize(CArchive& ar);
	
	
	RESOURCEDATAMAX m_dlgData;
};

//
// DETAIL: CaIpmPropertyNetTraficData
class CaIpmPropertyNetTraficData: public CObject
{
	DECLARE_SERIAL (CaIpmPropertyNetTraficData)
public:
	CaIpmPropertyNetTraficData();
	virtual ~CaIpmPropertyNetTraficData(){}
	virtual void Serialize (CArchive& ar);

	CaNetTraficInfo m_TraficInfo;

	BOOL m_bStartup;
	int  m_nCounter; 
	CString m_strTimeStamp;
	CString m_strTotalPacketIn;
	CString m_strTotalDataIn;
	CString m_strTotalPacketOut;
	CString m_strTotalDataOut;
	CString m_strCurrentPacketIn;
	CString m_strCurrentDataIn;
	CString m_strCurrentPacketOut;
	CString m_strCurrentDataOut;
	CString m_strAveragePacketIn;
	CString m_strAverageDataIn;
	CString m_strAveragePacketOut;
	CString m_strAverageDataOut;
	CString m_strPeekPacketIn;
	CString m_strPeekDataIn;
	CString m_strPeekPacketOut;
	CString m_strPeekDataOut;
	CString m_strDate1PacketIn;
	CString m_strDate2PacketIn;
	CString m_strDate1DataIn;
	CString m_strDate2DataIn;
	CString m_strDate1PacketOut;
	CString m_strDate2PacketOut;
	CString m_strDate1DataOut;
	CString m_strDate2DataOut;
};


//
// DETAIL: Server
class CuIpmPropertyDataDetailServer: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataDetailServer)
public:
	CuIpmPropertyDataDetailServer ();
	CuIpmPropertyDataDetailServer (LPSERVERDATAMAX pData);
	virtual ~CuIpmPropertyDataDetailServer(){};

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	SERVERDATAMAX m_dlgData;
	CaIpmPropertyNetTraficData m_netTraficData;
};


//
// DETAIL: Session
class CuIpmPropertyDataDetailSession: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataDetailSession)
public:
	CuIpmPropertyDataDetailSession ();
	CuIpmPropertyDataDetailSession (LPSESSIONDATAMAX pData);
	virtual ~CuIpmPropertyDataDetailSession(){};

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize	(CArchive& ar);
	
	SESSIONDATAMAX m_dlgData;
};


//
// DETAIL: Transaction
class CuIpmPropertyDataDetailTransaction: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataDetailTransaction)
public:
	CuIpmPropertyDataDetailTransaction ();
	CuIpmPropertyDataDetailTransaction (LPLOGTRANSACTDATAMAX pData);
	virtual ~CuIpmPropertyDataDetailTransaction(){};

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize(CArchive& ar);
	
	LOGTRANSACTDATAMAX m_dlgData;
};


//
// PAGE: Active Databases
//		 tuple = (DB, Status, TXCnt, Begin, End, Read, Write)
//		 It must containt the list of tuples.
// *********************************************************
class CuDataTupleActiveDatabase: public CObject
{
	DECLARE_SERIAL (CuDataTupleActiveDatabase)
public:
	CuDataTupleActiveDatabase();
	CuDataTupleActiveDatabase(
		LPCTSTR lpszDatabase,
		LPCTSTR lpszStatus, 
		LPCTSTR lpszTXCnt, 
		LPCTSTR lpszBegin,
		LPCTSTR lpszEnd,
		LPCTSTR lpszRead,
		LPCTSTR lpszWrite);
	virtual ~CuDataTupleActiveDatabase(){}
	CString m_strDatabase ; 
	CString m_strStatus;
	CString m_strTXCnt;
	CString m_strBegin;   
	CString m_strEnd; 
	CString m_strRead; 
	CString m_strWrite;   

	virtual void Serialize (CArchive& ar);
};

class CuIpmPropertyDataPageActiveDatabases: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataPageActiveDatabases)
public:
	CuIpmPropertyDataPageActiveDatabases();
	virtual ~CuIpmPropertyDataPageActiveDatabases();

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);

	CTypedPtrList<CObList, CuDataTupleActiveDatabase*> m_listTuple;
};


//
// PAGE: Client
class CuIpmPropertyDataPageClient: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataPageClient)
public:
	CuIpmPropertyDataPageClient ();
	CuIpmPropertyDataPageClient (LPSESSIONDATAMAX pData);
	virtual ~CuIpmPropertyDataPageClient(){};

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize(CArchive& ar);
	
	SESSIONDATAMAX m_dlgData;
};

//
// PAGE: Databases
class CuIpmPropertyDataPageDatabases: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataPageDatabases)
public:
	CuIpmPropertyDataPageDatabases();
	virtual ~CuIpmPropertyDataPageDatabases(){};

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
};

//
// PAGE: Headers
// It is a form view !!!
class CuIpmPropertyDataPageHeaders: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataPageHeaders)
public:
	CuIpmPropertyDataPageHeaders();
	CuIpmPropertyDataPageHeaders(LPLOGHEADERDATAMIN pData);
	virtual ~CuIpmPropertyDataPageHeaders(){};

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	LOGHEADERDATAMIN m_dlgData;
};

//
// PAGE: LogFile
class CaPieInfoData;
class CuIpmPropertyDataPageLogFile: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataPageLogFile)
public:
	CuIpmPropertyDataPageLogFile();
	virtual ~CuIpmPropertyDataPageLogFile(){};

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	CString m_strStatus;
	CaPieInfoData* m_pPieInfo;  // Do not delete this member.
};

//
// PAGE: Lock Lists
//		 tuple = (ID, Session, Count, Logical, MaxL, Status)
//		 It must containt the list of tuples.
// *********************************************************
class CuDataTupleLockList: public CObject
{
	DECLARE_SERIAL (CuDataTupleLockList)
public:
	CuDataTupleLockList();
	CuDataTupleLockList(
		LPCTSTR lpszID,
		LPCTSTR lpszSession, 
		LPCTSTR lpszCount, 
		LPCTSTR lpszLogical,
		LPCTSTR lpszMaxL,
		LPCTSTR lpszStatus);
	virtual ~CuDataTupleLockList(){}
	CString m_strID ;
	CString m_strSession;
	CString m_strCount;
	CString m_strLogical;
	CString m_strMaxL;
	CString m_strStatus;   

	virtual void Serialize (CArchive& ar);
};

class CuIpmPropertyDataPageLockLists: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataPageLockLists)
public:
	CuIpmPropertyDataPageLockLists();
	virtual ~CuIpmPropertyDataPageLockLists();

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	CTypedPtrList<CObList, CuDataTupleLockList*> m_listTuple;
};


//
// PAGE: Lock Resources
//		 tuple = (ID, GR, CV, LockType, DB, Table, Page)
//		 It must containt the list of tuples.
// ***************************************************************
class CuDataTupleResource: public CObject
{
	DECLARE_SERIAL (CuDataTupleResource)
public:
	CuDataTupleResource();
	CuDataTupleResource(
		LPCTSTR lpszID,
		LPCTSTR lpszGR, 
		LPCTSTR lpszCV, 
		LPCTSTR lpszLockType,
		LPCTSTR lpszDB,
		LPCTSTR lpszTable,
		LPCTSTR lpszPage);
	virtual ~CuDataTupleResource(){}
	CString m_strID ;
	CString m_strGR;
	CString m_strCV;
	CString m_strLockType; 
	CString m_strDB;  
	CString m_strTable;
	CString m_strPage;

	virtual void Serialize (CArchive& ar);
};

class CuIpmPropertyDataPageLockResources: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataPageLockResources)
public:
	CuIpmPropertyDataPageLockResources();
	virtual ~CuIpmPropertyDataPageLockResources();

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	CTypedPtrList<CObList, CuDataTupleResource*> m_listTuple;
};

//
// PAGE: Locks
//		 tuple = (ID, RQ, State, LockType, DB, Table, Page)
//		 It must containt the list of tuples.
// ***************************************************************
class CuDataTupleLock: public CObject
{
	DECLARE_SERIAL (CuDataTupleLock)
public:
	CuDataTupleLock();
	CuDataTupleLock(
		LPCTSTR lpszID,
		LPCTSTR lpszRQ, 
		LPCTSTR lpszState, 
		LPCTSTR lpszLockType,
		LPCTSTR lpszDB,
		LPCTSTR lpszTable,
		LPCTSTR lpszPage);
	virtual ~CuDataTupleLock(){}
	CString m_strID ;
	CString m_strRQ;
	CString m_strState;
	CString m_strLockType; 
	CString m_strDB;  
	CString m_strTable;
	CString m_strPage;

	virtual void Serialize (CArchive& ar);
};

class CuIpmPropertyDataPageLocks: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataPageLocks)
public:
	CuIpmPropertyDataPageLocks();
	virtual ~CuIpmPropertyDataPageLocks();

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	CTypedPtrList<CObList, CuDataTupleLock*> m_listTuple;
};


//
// PAGE: Lock Tables
//		 tuple = (ID, GR, CV, Table)
//		 It must containt the list of tuples.
// ***************************************************************
class CuDataTupleLockTable: public CObject
{
	DECLARE_SERIAL (CuDataTupleLockTable)
public:
	CuDataTupleLockTable();
	CuDataTupleLockTable(
		LPCTSTR lpszID,
		LPCTSTR lpszGR, 
		LPCTSTR lpszCV, 
		LPCTSTR lpszTable);
	virtual ~CuDataTupleLockTable(){}
	CString m_strID ;
	CString m_strGR;
	CString m_strCV;
	CString m_strTable;

	virtual void Serialize (CArchive& ar);
};

class CuIpmPropertyDataPageLockTables: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataPageLockTables)
public:
	CuIpmPropertyDataPageLockTables();
	virtual ~CuIpmPropertyDataPageLockTables();

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	CTypedPtrList<CObList, CuDataTupleLockTable*> m_listTuple;
};


//
// PAGE: Processes
//		 tuple = (ID, PID, Name, TTY, Database, State, Facil, Query)
//		 It must containt the list of tuples.
// *****************************************************************
class CuDataTupleProcess: public CObject
{
	DECLARE_SERIAL (CuDataTupleProcess)
public:
	CuDataTupleProcess();
	CuDataTupleProcess(
		LPCTSTR lpszID,
		LPCTSTR lpszPID, 
		LPCTSTR lpszType, 
		LPCTSTR lpszOpenDB,
		LPCTSTR lpszWrite,
		LPCTSTR lpszForce,
		LPCTSTR lpszWait,
		LPCTSTR lpszBegin,
		LPCTSTR lpszEnd);
	virtual ~CuDataTupleProcess(){};
	CString m_strID ;
	CString m_strPID;
	CString m_strType;
	CString m_strOpenDB;
	CString m_strWrite;
	CString m_strForce;
	CString m_strWait;
	CString m_strBegin;
	CString m_strEnd;

	virtual void Serialize (CArchive& ar);
};

class CuIpmPropertyDataPageProcesses: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataPageProcesses)
public:
	CuIpmPropertyDataPageProcesses();
	virtual ~CuIpmPropertyDataPageProcesses();

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	CTypedPtrList<CObList, CuDataTupleProcess*> m_listTuple;
};


//
// PAGE: Sessions
//		 tuple = (ID, Name, TTY, Database, State, Facil, Query)
//		 It must containt the list of tuples.
// *****************************************************************
class CuDataTupleSession: public CObject
{
	DECLARE_SERIAL (CuDataTupleSession)
public:
	CuDataTupleSession();
	CuDataTupleSession(
		LPCTSTR lpszID,
		LPCTSTR lpszName, 
		LPCTSTR lpszTTY,
		LPCTSTR lpszDatabase,
		LPCTSTR lpszState,
		LPCTSTR lpszFacil,
		LPCTSTR lpszQuery);
	virtual ~CuDataTupleSession(){};
	CString m_strID ;
	CString m_strName;
	CString m_strTTY;
	CString m_strDatabase;
	CString m_strState;
	CString m_strFacil;
	CString m_strQuery;

	virtual void Serialize (CArchive& ar);
};

class CuIpmPropertyDataPageSessions: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataPageSessions)
public:
	CuIpmPropertyDataPageSessions();
	virtual ~CuIpmPropertyDataPageSessions();

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	CTypedPtrList<CObList, CuDataTupleSession*> m_listTuple;
};


//
// PAGE: Transactions
//		 tuple = (ExternalTXID, Database, Session, Status, Write, Split)
//		 It must containt the list of tuples.
// *********************************************************************
class CuDataTupleTransaction: public CObject
{
	DECLARE_SERIAL (CuDataTupleTransaction)
public:
	CuDataTupleTransaction();
	CuDataTupleTransaction(
		LPCTSTR lpszExternalTXID,
		LPCTSTR lpszDatabase, 
		LPCTSTR lpszSession,
		LPCTSTR lpszStatus,
		LPCTSTR lpszWrite,
		LPCTSTR lpszSplit);
	virtual ~CuDataTupleTransaction(){};
	CString m_strExternalTXID ; 
	CString m_strDatabase;
	CString m_strSession;
	CString m_strStatus;
	CString m_strWrite;
	CString m_strSplit;

	virtual void Serialize (CArchive& ar);
};

class CuIpmPropertyDataPageTransactions: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataPageTransactions)
public:
	CuIpmPropertyDataPageTransactions();
	virtual ~CuIpmPropertyDataPageTransactions();

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	CTypedPtrList<CObList, CuDataTupleTransaction*> m_listTuple;
};


//
// SUMMARY: LockInfo
class CuIpmPropertyDataSummaryLockInfo: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataSummaryLockInfo)
public:
	CuIpmPropertyDataSummaryLockInfo();
	CuIpmPropertyDataSummaryLockInfo(LPLOCKINFOSUMMARY pData);
	virtual ~CuIpmPropertyDataSummaryLockInfo(){};

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);

	LOCKINFOSUMMARY m_dlgData;
};


//
// SUMMARY: LogInfo
class CuIpmPropertyDataSummaryLogInfo: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataSummaryLogInfo)
public:
	CuIpmPropertyDataSummaryLogInfo();
	CuIpmPropertyDataSummaryLogInfo(LPLOGINFOSUMMARY pData);
	virtual ~CuIpmPropertyDataSummaryLogInfo(){};

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);

	LOGINFOSUMMARY m_dlgData;
};


//
// Replication Data
// *****************************************************************



//
// SERVER::Startup Setting:
class CaReplicationServerDataPageStartupSetting: public CuIpmPropertyData
{
	DECLARE_SERIAL (CaReplicationServerDataPageStartupSetting)
public:
	CaReplicationServerDataPageStartupSetting();
	virtual ~CaReplicationServerDataPageStartupSetting()
	{
		if (!m_listItems.IsEmpty())
			m_listItems.RemoveAll();
	};

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	// 
	// Data member;
	CaReplicationItemList m_listItems;
	CArray < int, int > m_cxHeader;
	CSize m_scrollPos;

};


//
// SERVER::Raise Event:
class CaReplicationServerDataPageRaiseEvent: public CuIpmPropertyData
{
	DECLARE_SERIAL (CaReplicationServerDataPageRaiseEvent)
public:
	CaReplicationServerDataPageRaiseEvent();
	virtual ~CaReplicationServerDataPageRaiseEvent();
	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	// 
	// Data member;
	CaReplicRaiseDbeventList m_listEvent;
	CArray < int, int > m_cxHeader;
	CSize m_scrollPos;

};


//
// SERVER::Assignment:
class CaReplicationServerDataPageAssignment: public CuIpmPropertyData
{
	DECLARE_SERIAL (CaReplicationServerDataPageAssignment)
public:
	CaReplicationServerDataPageAssignment();
	virtual ~CaReplicationServerDataPageAssignment();
	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	// 
	// Data member;
	CTypedPtrList<CObList, CStringList*> m_listTuple;
	CArray < int, int > m_cxHeader;
	CSize m_scrollPos;
};


//
// SERVER::Status:
class CaReplicationServerDataPageStatus: public CuIpmPropertyData
{
	DECLARE_SERIAL (CaReplicationServerDataPageStatus)
public:
	CaReplicationServerDataPageStatus();
	virtual ~CaReplicationServerDataPageStatus();
	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	// 
	// Data member;
	REPLICSERVERDATAMIN m_dlgData;
	CString m_strStartLogFile;
};

//
// REPLICATION STATIC::Activity:
class CaReplicationStaticDataPageActivity: public CuIpmPropertyData
{
	DECLARE_SERIAL (CaReplicationStaticDataPageActivity)
public:
	CaReplicationStaticDataPageActivity();
	virtual ~CaReplicationStaticDataPageActivity();
	virtual void NotifyLoad (CWnd* pDlg);
	virtual void Serialize (CArchive& ar);
	
	// 
	// Data member;
	int m_nCurrentPage;
	CString m_strInputQueue;
	CString m_strDistributionQueue;
	CString m_strStartingTime;

	CTypedPtrList<CObList, CStringList*> m_listSummary;
	CArray < int, int > m_cxSummary;
	CSize m_scrollSummary;
	//
	// Per Database:
	CTypedPtrList<CObList, CStringList*> m_listDatabaseOutgoing;
	CTypedPtrList<CObList, CStringList*> m_listDatabaseIncoming;
	CTypedPtrList<CObList, CStringList*> m_listDatabaseTotal;
	CArray < int, int > m_cxDatabaseOutgoing;
	CArray < int, int > m_cxDatabaseIncoming;
	CArray < int, int > m_cxDatabaseTotal	;
	CSize m_scrollDatabaseOutgoing;
	CSize m_scrollDatabaseIncoming;
	CSize m_scrollDatabaseTotal;

	//
	// Per Table:
	CTypedPtrList<CObList, CStringList*> m_listTableOutgoing;
	CTypedPtrList<CObList, CStringList*> m_listTableIncoming;
	CTypedPtrList<CObList, CStringList*> m_listTableTotal;
	CArray < int, int > m_cxTableOutgoing;
	CArray < int, int > m_cxTableIncoming;
	CArray < int, int > m_cxTableTotal	 ;
	CSize m_scrollTableOutgoing;
	CSize m_scrollTableIncoming;
	CSize m_scrollTableTotal;
};

//
// REPLICATION STATIC::Integerity:
class CaReplicationStaticDataPageIntegrity: public CuIpmPropertyData
{
	DECLARE_SERIAL (CaReplicationStaticDataPageIntegrity)
public:
	CaReplicationStaticDataPageIntegrity();
	virtual ~CaReplicationStaticDataPageIntegrity();
	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	// 
	// Data members
	//

	// tables combo
	CaReplicIntegrityRegTableList m_RegTableList;
	CString m_strComboTable;

	// cdds combo
	CaReplicCommonList m_CDDSList;
	/* FINIR : itemdata */
	CString m_strComboCdds;

	// Databases combos
	CaReplicCommonList m_DbList1;
	CaReplicCommonList m_DbList2;
	/* FINIR : itemdata */
	CString m_strComboDatabase1;
	CString m_strComboDatabase2;
	
	// Columns list
	CaReplicCommonList m_ColList;

	// edit fields
	CString m_strTimeBegin;
	CString m_strTimeEnd;
	CString m_strResult; // Edit4
};

//
// REPLICATION STATIC::Server:
class CaReplicationStaticDataPageServer: public CuIpmPropertyData
{
	DECLARE_SERIAL (CaReplicationStaticDataPageServer)
public:
	CaReplicationStaticDataPageServer();
	virtual ~CaReplicationStaticDataPageServer();
	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	// 
	// Data member;
	CTypedPtrList<CObList, CStringList*> m_listTuple;
	CArray < int, int > m_cxHeader;
	CSize m_scrollPos;

};


//
// REPLICATION STATIC::Distrib:
class CaReplicationStaticDataPageDistrib: public CuIpmPropertyData
{
	DECLARE_SERIAL (CaReplicationStaticDataPageDistrib)
public:
	CaReplicationStaticDataPageDistrib();
	virtual ~CaReplicationStaticDataPageDistrib();
	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	// 
	// Data member;
	CString m_strText;
};


//
// REPLICATION STATIC::Collision:
class CaReplicationStaticDataPageCollision: public CuIpmPropertyData
{
	DECLARE_SERIAL (CaReplicationStaticDataPageCollision)
public:
	CaReplicationStaticDataPageCollision();
	virtual ~CaReplicationStaticDataPageCollision();
	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	// 
	// Data member;
	CaCollision* m_pData;
};

//
// PAGE: Static SERVERS
// *****************************************************************
class CuDataTupleServer: public CObject
{
	DECLARE_SERIAL (CuDataTupleServer)
public:
	CuDataTupleServer();
	CuDataTupleServer(
		LPCTSTR lpszName,
		LPCTSTR lpszListenAddress,
		LPCTSTR lpszClass,
		LPCTSTR lpszDbList,
		int imageIndex);
	virtual ~CuDataTupleServer(){};
	CString m_strName;
	CString m_strListenAddress;
	CString m_strClass;
	CString m_strDbList;
	int m_imageIndex;

	virtual void Serialize (CArchive& ar);
};

class CuIpmPropertyDataPageServers: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataPageServers)
public:
	CuIpmPropertyDataPageServers();
	virtual ~CuIpmPropertyDataPageServers();

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	CTypedPtrList<CObList, CuDataTupleServer*> m_listTuple;
};


//
// PAGE: Static ACTIVE USERS
// *****************************************************************
class CuDataTupleActiveuser: public CObject
{
	DECLARE_SERIAL (CuDataTupleActiveuser)
public:
	CuDataTupleActiveuser();
	CuDataTupleActiveuser(LPCTSTR lpszName);
	virtual ~CuDataTupleActiveuser(){};
	CString m_strName;

	virtual void Serialize (CArchive& ar);
};

class CuIpmPropertyDataPageActiveusers: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataPageActiveusers)
public:
	CuIpmPropertyDataPageActiveusers();
	virtual ~CuIpmPropertyDataPageActiveusers();

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	CTypedPtrList<CObList, CuDataTupleActiveuser*> m_listTuple;
};

//
// PAGE: Static DATABASES
// *****************************************************************
class CuDataTupleDatabase: public CObject
{
	DECLARE_SERIAL (CuDataTupleDatabase)
public:
	CuDataTupleDatabase();
	CuDataTupleDatabase(LPCTSTR lpszName, int imageIndex);
	virtual ~CuDataTupleDatabase(){};
	CString m_strName;
	int m_imageIndex;

	virtual void Serialize (CArchive& ar);
};

class CuIpmPropertyDataPageStaticDatabases: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataPageStaticDatabases)
public:
	CuIpmPropertyDataPageStaticDatabases();
	virtual ~CuIpmPropertyDataPageStaticDatabases();

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	CTypedPtrList<CObList, CuDataTupleDatabase*> m_listTuple;
};

//
// PAGE: Static REPLICATION
// *****************************************************************
class CuDataTupleReplication: public CObject
{
	DECLARE_SERIAL (CuDataTupleReplication)
public:
	CuDataTupleReplication();
	CuDataTupleReplication(LPCTSTR lpszName, int imageIndex);
	virtual ~CuDataTupleReplication(){};
	CString m_strName;
	int m_imageIndex;

	virtual void Serialize (CArchive& ar);
};

class CuIpmPropertyDataPageStaticReplications: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuIpmPropertyDataPageStaticReplications)
public:
	CuIpmPropertyDataPageStaticReplications();
	virtual ~CuIpmPropertyDataPageStaticReplications();

	virtual void NotifyLoad (CWnd* pDlg);  
	virtual void Serialize (CArchive& ar);
	
	CTypedPtrList<CObList, CuDataTupleReplication*> m_listTuple;
};

#endif // IPMPRDTA_HEADER
