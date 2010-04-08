/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : cbfprdta.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager (Origin IPM Project)                 //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : The implementation of serialization of CBF Page (modeless dialog)     //
****************************************************************************************/
#ifndef CBFPRDTA_HEADER
#define CBFPRDTA_HEADER


class CuCbfPropertyData: public CObject
{
    DECLARE_SERIAL (CuCbfPropertyData)
public:
    CuCbfPropertyData();
    CuCbfPropertyData(LPCTSTR lpszClassName);

    virtual ~CuCbfPropertyData(){};

    virtual void NotifyLoad (CWnd* pDlg);  
        // NotifyLoad() Send message WM_USER_IPMPAGE_LOADING to the Window 'pDlg'
        // WPARAM: The class name of the Sender.
        // LPARAM: The data structure depending on the class name of the Sender.
    virtual void Serialize (CArchive& ar);
    CString& GetClassName() {return m_strClassName;}
protected:
    CString m_strClassName;
};

/*
//
// DETAIL: Active User
class CuCbfPropertyDataDetailActiveUser: public CuCbfPropertyData
{
    DECLARE_SERIAL (CuCbfPropertyDataDetailActiveUser)
public:
    CuCbfPropertyDataDetailActiveUser();
    virtual ~CuCbfPropertyDataDetailActiveUser(){};

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);
};


//
// DETAIL: Database
class CuCbfPropertyDataDetailDatabase: public CuCbfPropertyData
{
    DECLARE_SERIAL (CuCbfPropertyDataDetailDatabase)
public:
    CuCbfPropertyDataDetailDatabase();
    virtual ~CuCbfPropertyDataDetailDatabase(){};

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);
};

//
// DETAIL: Location
class CuCbfPropertyDataDetailLocation: public CuCbfPropertyData
{
    DECLARE_SERIAL (CuCbfPropertyDataDetailLocation)
public:
    CuCbfPropertyDataDetailLocation();
    virtual ~CuCbfPropertyDataDetailLocation(){};

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

    CuPieInfoData* m_pPieInfo;    // Do not delete this member.
};

//
// DETAIL: Lock
class CuCbfPropertyDataDetailLock: public CuCbfPropertyData
{
    DECLARE_SERIAL (CuCbfPropertyDataDetailLock)
public:
    CuCbfPropertyDataDetailLock ();
    CuCbfPropertyDataDetailLock (LPLOCKDATAMAX pData);
    virtual ~CuCbfPropertyDataDetailLock(){};

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize  (CArchive& ar);
    
    
    LOCKDATAMAX m_dlgData;
};

//
// DETAIL: Lock List
class CuCbfPropertyDataDetailLockList: public CuCbfPropertyData
{
    DECLARE_SERIAL (CuCbfPropertyDataDetailLockList)
public:
    CuCbfPropertyDataDetailLockList ();
    CuCbfPropertyDataDetailLockList (LPLOCKLISTDATAMAX pData);
    virtual ~CuCbfPropertyDataDetailLockList(){};

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize  (CArchive& ar);
    
    
    LOCKLISTDATAMAX m_dlgData;
};

//
// DETAIL: Process
class CuCbfPropertyDataDetailProcess: public CuCbfPropertyData
{
    DECLARE_SERIAL (CuCbfPropertyDataDetailProcess)
public:
    CuCbfPropertyDataDetailProcess ();
    CuCbfPropertyDataDetailProcess (LPLOGPROCESSDATAMAX pData);
    virtual ~CuCbfPropertyDataDetailProcess(){};

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);
    
    
    LOGPROCESSDATAMAX m_dlgData;
};

//
// DETAIL: Resource
class CuCbfPropertyDataDetailResource: public CuCbfPropertyData
{
    DECLARE_SERIAL (CuCbfPropertyDataDetailResource)
public:
    CuCbfPropertyDataDetailResource ();
    CuCbfPropertyDataDetailResource (LPRESOURCEDATAMAX pData);
    virtual ~CuCbfPropertyDataDetailResource(){};

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize  (CArchive& ar);
    
    
    RESOURCEDATAMAX m_dlgData;
};

//
// DETAIL: Server
class CuCbfPropertyDataDetailServer: public CuCbfPropertyData
{
    DECLARE_SERIAL (CuCbfPropertyDataDetailServer)
public:
    CuCbfPropertyDataDetailServer ();
    CuCbfPropertyDataDetailServer (LPSERVERDATAMAX pData);
    virtual ~CuCbfPropertyDataDetailServer(){};

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);
    
    SERVERDATAMAX m_dlgData;
};


//
// DETAIL: Session
class CuCbfPropertyDataDetailSession: public CuCbfPropertyData
{
    DECLARE_SERIAL (CuCbfPropertyDataDetailSession)
public:
    CuCbfPropertyDataDetailSession ();
    CuCbfPropertyDataDetailSession (LPSESSIONDATAMAX pData);
    virtual ~CuCbfPropertyDataDetailSession(){};

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize  (CArchive& ar);
    
    SESSIONDATAMAX m_dlgData;
};


//
// DETAIL: Transaction
class CuCbfPropertyDataDetailTransaction: public CuCbfPropertyData
{
    DECLARE_SERIAL (CuCbfPropertyDataDetailTransaction)
public:
    CuCbfPropertyDataDetailTransaction ();
    CuCbfPropertyDataDetailTransaction (LPLOGTRANSACTDATAMAX pData);
    virtual ~CuCbfPropertyDataDetailTransaction(){};

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize  (CArchive& ar);
    
    LOGTRANSACTDATAMAX m_dlgData;
};


//
// PAGE: Active Databases
//       tuple = (DB, Status, TXCnt, Begin, End, Read, Write)
//       It must containt the list of tuples.
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

class CuCbfPropertyDataPageActiveDatabases: public CuCbfPropertyData
{
    DECLARE_SERIAL (CuCbfPropertyDataPageActiveDatabases)
public:
    CuCbfPropertyDataPageActiveDatabases();
    virtual ~CuCbfPropertyDataPageActiveDatabases();

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

    CTypedPtrList<CObList, CuDataTupleActiveDatabase*> m_listTuple;
};


//
// PAGE: Client
class CuCbfPropertyDataPageClient: public CuCbfPropertyData
{
    DECLARE_SERIAL (CuCbfPropertyDataPageClient)
public:
    CuCbfPropertyDataPageClient ();
    CuCbfPropertyDataPageClient (LPSESSIONDATAMAX pData);
    virtual ~CuCbfPropertyDataPageClient(){};

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize  (CArchive& ar);
    
    SESSIONDATAMAX m_dlgData;
};

//
// PAGE: Databases
class CuCbfPropertyDataPageDatabases: public CuCbfPropertyData
{
    DECLARE_SERIAL (CuCbfPropertyDataPageDatabases)
public:
    CuCbfPropertyDataPageDatabases();
    virtual ~CuCbfPropertyDataPageDatabases(){};

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);
    
};

//
// PAGE: Headers
// It is a form view !!!
class CuCbfPropertyDataPageHeaders: public CuCbfPropertyData
{
    DECLARE_SERIAL (CuCbfPropertyDataPageHeaders)
public:
    CuCbfPropertyDataPageHeaders();
    CuCbfPropertyDataPageHeaders(LPLOGHEADERDATAMIN pData);
    virtual ~CuCbfPropertyDataPageHeaders(){};

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);
    
    LOGHEADERDATAMIN m_dlgData;
};


//
// PAGE: Location (Page 2, Page1 is call Detail)
class CuCbfPropertyDataPageLocation: public CuCbfPropertyData
{
    DECLARE_SERIAL (CuCbfPropertyDataPageLocation)
public:
    CuCbfPropertyDataPageLocation();
    virtual ~CuCbfPropertyDataPageLocation(){};

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);
    
    CuPieInfoData* m_pPieInfo;    // Do not delete this member.
};

//
// PAGE: Location Usage (Page 3)
class CuCbfPropertyDataPageLocationUsage: public CuCbfPropertyData
{
    DECLARE_SERIAL (CuCbfPropertyDataPageLocationUsage)
public:
    CuCbfPropertyDataPageLocationUsage();
    CuCbfPropertyDataPageLocationUsage(LPLOCATIONPARAMS pData);
    virtual ~CuCbfPropertyDataPageLocationUsage(){};

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);
    
    LOCATIONPARAMS m_dlgData;
};



//
// PAGE: Lock Lists
//       tuple = (ID, Session, Count, Logical, MaxL, Status)
//       It must containt the list of tuples.
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

class CuCbfPropertyDataPageLockLists: public CuCbfPropertyData
{
    DECLARE_SERIAL (CuCbfPropertyDataPageLockLists)
public:
    CuCbfPropertyDataPageLockLists();
    virtual ~CuCbfPropertyDataPageLockLists();

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);
    
    CTypedPtrList<CObList, CuDataTupleLockList*> m_listTuple;
};


//
// PAGE: Lock Resources
//       tuple = (ID, GR, CV, LockType, DB, Table, Page)
//       It must containt the list of tuples.
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

class CuCbfPropertyDataPageLockResources: public CuCbfPropertyData
{
    DECLARE_SERIAL (CuCbfPropertyDataPageLockResources)
public:
    CuCbfPropertyDataPageLockResources();
    virtual ~CuCbfPropertyDataPageLockResources();

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);
    
    CTypedPtrList<CObList, CuDataTupleResource*> m_listTuple;
};

//
// PAGE: Locks
//       tuple = (ID, RQ, State, LockType, DB, Table, Page)
//       It must containt the list of tuples.
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

class CuCbfPropertyDataPageLocks: public CuCbfPropertyData
{
    DECLARE_SERIAL (CuCbfPropertyDataPageLocks)
public:
    CuCbfPropertyDataPageLocks();
    virtual ~CuCbfPropertyDataPageLocks();

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);
    
    CTypedPtrList<CObList, CuDataTupleLock*> m_listTuple;
};


//
// PAGE: Lock Tables
//       tuple = (ID, GR, CV, Table)
//       It must containt the list of tuples.
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

class CuCbfPropertyDataPageLockTables: public CuCbfPropertyData
{
    DECLARE_SERIAL (CuCbfPropertyDataPageLockTables)
public:
    CuCbfPropertyDataPageLockTables();
    virtual ~CuCbfPropertyDataPageLockTables();

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);
    
    CTypedPtrList<CObList, CuDataTupleLockTable*> m_listTuple;
};


//
// PAGE: Processes
//       tuple = (ID, PID, Name, TTY, Database, State, Facil, Query)
//       It must containt the list of tuples.
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

class CuCbfPropertyDataPageProcesses: public CuCbfPropertyData
{
    DECLARE_SERIAL (CuCbfPropertyDataPageProcesses)
public:
    CuCbfPropertyDataPageProcesses();
    virtual ~CuCbfPropertyDataPageProcesses();

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);
    
    CTypedPtrList<CObList, CuDataTupleProcess*> m_listTuple;
};


//
// PAGE: Sessions
//       tuple = (ID, Name, TTY, Database, State, Facil, Query)
//       It must containt the list of tuples.
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

class CuCbfPropertyDataPageSessions: public CuCbfPropertyData
{
    DECLARE_SERIAL (CuCbfPropertyDataPageSessions)
public:
    CuCbfPropertyDataPageSessions();
    virtual ~CuCbfPropertyDataPageSessions();

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);
    
    CTypedPtrList<CObList, CuDataTupleSession*> m_listTuple;
};


//
// PAGE: Transactions
//       tuple = (ExternalTXID, Database, Session, Status, Write, Split)
//       It must containt the list of tuples.
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

class CuCbfPropertyDataPageTransactions: public CuCbfPropertyData
{
    DECLARE_SERIAL (CuCbfPropertyDataPageTransactions)
public:
    CuCbfPropertyDataPageTransactions();
    virtual ~CuCbfPropertyDataPageTransactions();

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);
    
    CTypedPtrList<CObList, CuDataTupleTransaction*> m_listTuple;
};

//
// SUMMARY: Locations
class CuCbfPropertyDataSummaryLocations: public CuCbfPropertyData
{
    DECLARE_SERIAL (CuCbfPropertyDataSummaryLocations)
public:
    CuCbfPropertyDataSummaryLocations();
    virtual ~CuCbfPropertyDataSummaryLocations(){};

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);
    
    UINT m_nBarWidth;
    UINT m_nUnit;
    CuDiskInfoData* m_pDiskInfo;    // Do not delete this member.
};

//
// SUMMARY: LockInfo
class CuCbfPropertyDataSummaryLockInfo: public CuCbfPropertyData
{
    DECLARE_SERIAL (CuCbfPropertyDataSummaryLockInfo)
public:
    CuCbfPropertyDataSummaryLockInfo();
    CuCbfPropertyDataSummaryLockInfo(LPLOCKINFOSUMMARY pData);
    virtual ~CuCbfPropertyDataSummaryLockInfo(){};

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

    LOCKINFOSUMMARY m_dlgData;
};


//
// SUMMARY: LogInfo
class CuCbfPropertyDataSummaryLogInfo: public CuCbfPropertyData
{
    DECLARE_SERIAL (CuCbfPropertyDataSummaryLogInfo)
public:
    CuCbfPropertyDataSummaryLogInfo();
    CuCbfPropertyDataSummaryLogInfo(LPLOGINFOSUMMARY pData);
    virtual ~CuCbfPropertyDataSummaryLogInfo(){};

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

    LOGINFOSUMMARY m_dlgData;
};

*/


#endif
