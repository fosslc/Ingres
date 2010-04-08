/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : logadata.h , Header File 
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Log Analyser Data
**
** History:
**
** 17-Dec-1999 (uk$so01)
**    Created
** 23-Apr-2001 (uk$so01)
**    Bug #104487, 
**    When creating the temporary table, make sure to use the exact type based on its length.
**    If the type is integer and its length is 2 then use smallint or int2.
**    If the type is float and its length is 4 then use float(4).
** 07-Jun-2002 (uk$so01)
**    SIR #107951, Display date format according to the LOCALE.
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
** 01-Dec-2004 (uk$so01)
**    IJA BUG #113551
**    Ija should internally specify the fixed date format expected by auditdb 
**    command independently of the LOCALE setting.
**/

#if !defined(LOGADATA_HEADER)
#define LOGADATA_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "dmlbase.h"
#include "dmldbase.h"
#include "dmltable.h"
#include "dmlrule.h"
#include "dmlproc.h"
#include "rcdepend.h"

//
// Type of Transaction:
typedef enum 
{
	T_UNKNOWN = 0, 
	T_DELETE, 
	T_INSERT, 
	T_BEFOREUPDATE, 
	T_AFTERUPDATE,
	T_CREATETABLE,
	T_ALTERTABLE,
	T_DROPTABLE,
	T_RELOCATE,
	T_MODIFY,
	T_INDEX
} TxType;

BOOL isUndoRedoable(TxType txtyp);

class CaEllapsedTime: public CObject
{
public:
	CaEllapsedTime(){m_bError = FALSE;};
	CaEllapsedTime(LPCTSTR lpSinceString, LPCTSTR lpAfterString);
	~CaEllapsedTime(){};
	void GetDisplayString(CString& csTempDisplay);
	void Set(LPCTSTR lpSinceString, LPCTSTR lpAfterString);
	int Compare(CaEllapsedTime& Ellapsed);
protected:
	BOOL ReadStringAndFillVariable(LPCTSTR lpCurString, BOOL bStart);
	CTimeSpan m_TimeSpanWithNoCs;
	CTime m_ct1;
	CTime m_ct2;
	int m_ics;
	int m_icsEnd;
	int m_icsStart;
	BOOL m_bError;
};


//
// Detail of a Database Transaction:
//
class CaBaseTransactionItemData: public CObject
{
public:
	CaBaseTransactionItemData();
	virtual ~CaBaseTransactionItemData();

#ifdef SIMULATION
	void SetTransactionID(LPCTSTR ID){m_strTransactionID = ID;}
	CString GetTransactionID(){return m_strTransactionID;}
#endif
	
	void SetAction (TxType nType){m_nTxType = nType;}
	TxType GetAction(){return m_nTxType;}

	CString GetUser(){return m_strUser;}
	CString GetTable(){return m_strTable;}
	CString GetTableOwner(){return m_strTableOwner;}
	CString GetData();
	CStringList& GetListData(){return m_strlistData;}
	CString GetLsn ();
	BOOL GetCommit (){return m_bCommitted;}
	BOOL GetJournal(){return m_bJournaled;}
	BOOL GetRollback2SavePoint(){return m_bRollback2SavePoint;}

	void SetUser (LPCTSTR lpszUser){m_strUser = lpszUser;}
	void SetTable (LPCTSTR lpszTable){m_strTable = lpszTable;}
	void SetTableOwner (LPCTSTR lpszTableOwner){m_strTableOwner = lpszTableOwner;}
	void SetData (LPCTSTR lpszData){m_strData = lpszData;}
#ifdef SIMULATION
	void SetLsn  (LPCTSTR lpszLsn){m_strLSN = lpszLsn;}
#endif
	void SetCommit (BOOL bSet){m_bCommitted = bSet;}
	void SetJournal(BOOL bSet){m_bJournaled = bSet;}
	void SetRollback2SavePoint(BOOL bSet){m_bRollback2SavePoint = bSet;}

	//
	// For the Display only:
	CString GetStrTransactionID();
	virtual CString GetStrAction();

	int  GetImageId();
	BOOL IsBeginEnd(){return m_bBeginEnd;}
	void SetBegin (){m_bBeginEnd = TRUE; m_bBegin = TRUE;}
	void SetEnd (){m_bBeginEnd = TRUE; m_bBegin = FALSE;}
	//
	// LSN End is only for the sort order for Recovery/Redo purpose:
	void SetLsnEnd(LPCTSTR lpszLsn){m_strLSNEnd = lpszLsn;}
	void SetLsnEndPerValues(unsigned long lhigh, unsigned long llow){m_lLsnHigh_EndTrans = lhigh; m_lLsnLow_EndTrans = llow;}
	BOOL isLSNEndhigher( CaBaseTransactionItemData *p1) {if (m_lLsnHigh_EndTrans > p1->m_lLsnHigh_EndTrans) return TRUE; if (m_lLsnHigh_EndTrans<p1->m_lLsnHigh_EndTrans) return FALSE; return (m_lLsnLow_EndTrans >p1->m_lLsnLow_EndTrans);}

	void SetTransactionID (unsigned long lHight, unsigned long lLow){m_lTranIdHigh = lHight; m_lTranIdLow = lLow;}
	void SetLsn (unsigned long lHight, unsigned long lLow){m_lLsnHigh = lHight; m_lLsnLow = lLow;}
	void SetTid(long lTID) { m_ltid = lTID; }
	void GetTransactionID( unsigned long & lHigh, unsigned long & lLow){lHigh= m_lTranIdHigh; lLow=m_lTranIdLow;}
	void GetLsn(unsigned long & lHigh, unsigned long & lLow){lHigh= m_lLsnHigh; lLow=m_lLsnLow;}
	BOOL isLSNhigher( CaBaseTransactionItemData *p1) {if (m_lLsnHigh > p1->m_lLsnHigh) return TRUE; if (m_lLsnHigh<p1->m_lLsnHigh) return FALSE; return (m_lLsnLow >p1->m_lLsnLow);}
	long GetTid() { return m_ltid;}
	//
	// LSN End is only for the sort order for Recovery/Redo purpose:
	CString GetLsnEnd(){return m_strLSNEnd;}
	void GetLsnEndPerValues(unsigned long& lHigh, unsigned long& lLow){lHigh= m_lLsnHigh_EndTrans; lLow=m_lLsnLow_EndTrans;}

	void SetMsgTidChanged(BOOL bDisplayed) {m_bMsgTidModLaterDisplayed = bDisplayed;}
	void SetMsgTblChanged(BOOL bDisplayed) {m_bMsgTblModLaterDisplayed = bDisplayed;}
	BOOL HasMsgTidChangedBeenDisplayed() {return m_bMsgTidModLaterDisplayed;}
	BOOL HasMsgTblChangedBeenDisplayed() {return m_bMsgTblModLaterDisplayed;}

protected:
	TxType  m_nTxType;
#ifdef SIMULATION
	CString m_strTransactionID;
#endif
	CString m_strLSN;
	CString m_strUser;
	CString m_strTable;
	CString m_strTableOwner;
	CString m_strData;
	CString m_strLSNEnd;

	CStringList m_strlistData;
	BOOL m_bCommitted; // Committed (TRUE), Rollback(FALSE)
	BOOL m_bJournaled; // Journal (TRUE), Non-Journal(FALSE)
	BOOL m_bRollback2SavePoint; // Partial rollback to save point (TRUE)

	BOOL m_bBeginEnd;  // This item is for the special Transaction (Begin, End);
	BOOL m_bBegin;     // Only if m_bBeginEnd = TRUE, in that case: TRUE->BEGIN, FALSE->END

	//
	// Internal Data:
	unsigned long m_lLsnHigh;
	unsigned long m_lLsnLow;
	unsigned long m_lTranIdHigh;
	unsigned long m_lTranIdLow;

	unsigned long m_lLsnHigh_EndTrans;
	unsigned long m_lLsnLow_EndTrans;


	BOOL m_bMsgTidModLaterDisplayed;
	BOOL m_bMsgTblModLaterDisplayed;

	long m_ltid;
};





//
// Detail of a Database Transaction:
//
// TRANS, USER, ACTION, TABLE, DATA.
// 
class CaDatabaseTransactionItemDataDetail: public CaBaseTransactionItemData
{
public:
	CaDatabaseTransactionItemDataDetail():CaBaseTransactionItemData()
	{
		m_strTxTimeBegin = _T("");
		m_strTxTimeEnd = _T("");
	}
	virtual ~CaDatabaseTransactionItemDataDetail(){}
	int Compare (CaDatabaseTransactionItemDataDetail* pItem, int nColumn);
	
	virtual CString GetStrAction();
	void SetTransactionBegin(LPCTSTR lpszTime){ASSERT(m_bBeginEnd); m_strTxTimeBegin = lpszTime;}
	void SetTransactionEnd(LPCTSTR lpszTime){ASSERT(m_bBeginEnd); m_strTxTimeEnd = lpszTime;}
	CString GetTransactionBegin(){ASSERT(m_bBeginEnd); return m_strTxTimeBegin;}
	CString GetTransactionEnd(){ASSERT(m_bBeginEnd); return m_strTxTimeEnd;}

protected:
	CString m_strTxTimeBegin;
	CString m_strTxTimeEnd;

};



class CaDatabaseTransactionItemData: public CObject
{
public:
	CaDatabaseTransactionItemData();
	virtual ~CaDatabaseTransactionItemData();

	void SetTransactionID(LPCTSTR ID){m_strTransactionID = ID;}
	CString GetTransactionID(){return m_strTransactionID;}

	void SetCountInsert(long nCount){m_lCountInsert = nCount;}
	void SetCountUpdate(long nCount){m_lCountUpdate = nCount;}
	void SetCountDelete(long nCount){m_lCountDelete = nCount;}
	void SetCountTotal (long nCount){m_lCountTotal = nCount;}
	long GetCountInsert(){return m_lCountInsert;}
	long GetCountUpdate(){return m_lCountUpdate;}
	long GetCountDelete(){return m_lCountDelete;}
	long GetCountTotal (){return m_lCountTotal;}


	void SetUser(LPCTSTR lpszUser){m_strUser = lpszUser;}
	void SetBegin(LPCTSTR lpszDate){m_strBegin = lpszDate;}
	void SetEnd(LPCTSTR lpszDate){m_strEnd = lpszDate;}
	void SetDuration(LPCTSTR lpszDuration){m_strDuration = lpszDuration;}
	void SetEllapsed(LPCTSTR lpSinceString, LPCTSTR lpAfterString) {m_EllapsedTime.Set(lpSinceString,lpAfterString);}
	void SetCommit (BOOL bSet){m_bCommitted = bSet;}
	void SetJournal(BOOL bSet){m_bJournaled = bSet;}
	void SetLsnBegin(LPCTSTR lpszLsn){m_strLSNBegin = lpszLsn;}
	void SetLsnEnd(LPCTSTR lpszLsn){m_strLSNEnd = lpszLsn;}
	void SetRollback2SavePoint(BOOL bSet){m_bRollback2SavePoint = bSet;}

	CString GetUser(){return m_strUser;}
	CString GetBegin(){return m_strBegin;}
	CString GetEnd(){return m_strEnd;}
	CString GetBeginLocale();
	CString GetEndLocale();
	CString GetDuration();
	CaEllapsedTime & GetEllapsed() {return m_EllapsedTime;}
	BOOL GetCommit (){return m_bCommitted;}
	BOOL GetJournal(){return m_bJournaled;}
	BOOL GetRollback2SavePoint(){return m_bRollback2SavePoint;}

	CTypedPtrList < CObList, CaDatabaseTransactionItemDataDetail* >& GetDetailTransaction(){return m_listDetailTransaction;}

	//
	// For the Display only:
	CString GetStrTransactionID();
	CString GetStrCountInsert();
	CString GetStrCountUpdate();
	CString GetStrCountDelete();
	CString GetStrCountTotal();
	CString GetLsnBegin(){return m_strLSNBegin;}
	CString GetLsnEnd(){return m_strLSNEnd;}
	int GetImageId();
	int Compare (CaDatabaseTransactionItemData* pItem, int nColumn);

	CaDatabaseTransactionItemDataDetail& GetBeginTransaction(){return m_beginTransaction;}
	CaDatabaseTransactionItemDataDetail& GetEndTransaction(){return m_endTransaction;}


protected:
	CString m_strUser;
	CString m_strBegin;
	CString m_strEnd;
	CString m_strDuration;
	CaEllapsedTime m_EllapsedTime;
	CString m_strTransactionID;

	long m_lCountInsert;
	long m_lCountUpdate;
	long m_lCountDelete;
	long m_lCountTotal;
	BOOL m_bCommitted; // Committed (TRUE), Rollback(FALSE)
	BOOL m_bJournaled; // Journal (TRUE), Non-Journal(FALSE)
	BOOL m_bRollback2SavePoint; // Partial rollback to save point (TRUE)

	CString m_strLSNBegin;
	CString m_strLSNEnd;

	//
	// The main transaction of Database contains the Detail of Transactions:
	CTypedPtrList < CObList, CaDatabaseTransactionItemDataDetail* > m_listDetailTransaction;
	CaDatabaseTransactionItemDataDetail m_beginTransaction;
	CaDatabaseTransactionItemDataDetail m_endTransaction;
};



//
// Table Transaction:
//
// TRANS, DATE, USER, ACTION (operation), NAME.
// 
class CaTableTransactionItemData: public CaBaseTransactionItemData
{
public:
	CaTableTransactionItemData();
	virtual ~CaTableTransactionItemData(){}

	void SetOperation(TxType nType){SetAction(nType);}
	void SetDate(LPCTSTR lpszDate){m_strDate = lpszDate;}

	CString GetDate(){return m_strDate;}
	CString GetDateLocale();

	void SetName(LPCTSTR lpszName){m_strName = lpszName;} // Simulation only
	CString GetName(){return m_strName;}                  // Simulation only
	int Compare (CaTableTransactionItemData* pItem, int nColumn);

	//
	// For the Display only:
	CString GetStrOperation(){return GetStrAction();}


protected:
	CString m_strDate;
	CString m_strName;

};


class CaQueryTransactionInfo: public CObject
{
public:
	CaQueryTransactionInfo();
	CaQueryTransactionInfo(const CaQueryTransactionInfo& c);
	virtual ~CaQueryTransactionInfo(){}
	CaQueryTransactionInfo operator = (const CaQueryTransactionInfo& c);

	void SetNode (LPCTSTR lpszNode){m_strNode = lpszNode;}
	CString GetNode(){ return m_strNode;}

	void SetDatabase (LPCTSTR lpszDatabase, LPCTSTR lpszDatabaseOwner);
	void SetTable    (LPCTSTR lpszTable, LPCTSTR lpszTableOwner);
	void SetBeforeAfter (LPCTSTR lpszBefore, LPCTSTR lpszAfter){m_strBefore = lpszBefore; m_strAfter = lpszAfter;}
	void SetUser(LPCTSTR lpszUser){m_strUser = lpszUser;};
	void SetInconsistent(BOOL bInconsistent){m_bInconsistent = bInconsistent;}
	void SetWait(BOOL bWait){m_bWait = bWait;}
	void SetCheckPointNumber (int nNo){m_nCheckPointNumber = nNo;}
	void SetAfterCheckPoint (BOOL bAfterChk) {m_bAfterCheckPoint = bAfterChk;}
	void SetAbortedTransaction (BOOL bSet) {m_bAbortedTransaction = bSet;}
	void SetCommittedTransaction (BOOL bSet) {m_bCommittedTransaction = bSet;}
	void SetConnectedUser(LPCTSTR lpszConnectedUser){m_strConnectedUser = lpszConnectedUser;}

	void GetDatabase (CString& strDatabase, CString& strDatabaseOwner);
	void GetTable    (CString& strTable, CString& strTableOwner);
	void GetBeforeAfter (CString& strBefore, CString& strAfter){strBefore = m_strBefore; strAfter = m_strAfter;}
	BOOL GetInconsistent (){return m_bInconsistent;}
	BOOL GetWait (){return m_bWait;}
	int  GetCheckPointNumber (){return m_nCheckPointNumber;}
	BOOL GetAfterCheckPoint () {return m_bAfterCheckPoint;}
	BOOL GetAbortedTransaction() {return m_bAbortedTransaction;}
	BOOL GetCommittedTransaction() {return m_bCommittedTransaction;}
	CString GetUser(){return m_strUser;}
	CString GetConnectedUser(){return m_strConnectedUser;}

protected:
	CString m_strNode;
	CString m_strDatabase;
	CString m_strDatabaseOwner;
	CString m_strTable;
	CString m_strTableOwner;
	CString m_strConnectedUser;

	//
	// Filter parameters:
	CString m_strBefore;
	CString m_strAfter;
	CString m_strUser;
	
	BOOL m_bInconsistent;
	BOOL m_bWait;
	int  m_nCheckPointNumber;
	BOOL m_bAfterCheckPoint;
	BOOL m_bAbortedTransaction;
	BOOL m_bCommittedTransaction;

	void Copy (const CaQueryTransactionInfo& c);
};



class CaIjaTable: public CaTable
{
public:
	CaIjaTable():CaTable(){}
	CaIjaTable(LPCTSTR lpszItem, LPCTSTR lpszItemOwner, BOOL bSystem = FALSE)
		:CaTable(lpszItem, lpszItemOwner, bSystem){}
	~CaIjaTable()
	{
		while (!m_listColumn.IsEmpty())
			delete m_listColumn.RemoveHead();
	};


	CTypedPtrList<CObList, CaColumn*> m_listColumn;
};


class CaIjaRule: public CaRuleDetail
{
public:
	CaIjaRule():CaRuleDetail(){}
	CaIjaRule(LPCTSTR lpszName, LPCTSTR lpszOwner, BOOL bSystem = FALSE): CaRuleDetail(lpszName, lpszOwner, bSystem){}
	
	virtual ~CaIjaRule(){}

public:
	CaProcedureDetail m_procedure;

};


//
// This enumeration is shared by two image lists: IDB_IJAIMAGELIST and IDB_IJAIMAGELIST2
// It could be for one big image list, but for the reason that if we want to have the 
// distinct image for the same image index then two lists are better.
// Another reason is that IDB_IJAIMAGELIST is always the subset of IDB_IJAIMAGELIST2
enum
{
	//
	// For image list IDB_IJAIMAGELIST + IDB_IJAIMAGELIST2
	IM_COMMIT_JOURNAL = 0,
	IM_COMMIT_JOURNAL_NO,
	IM_ROLLBACK_JOURNAL,
	IM_ROLLBACK_JOURNAL_NO,
	// 
	// For the partial rollback to save point:
	IM_COMMIT_JOURNAL_ROLLBACK_2_SAVEPOINT,
	IM_COMMIT_JOURNAL_NO_ROLLBACK_2_SAVEPOINT,
	IM_ROLLBACK_JOURNAL_ROLLBACK_2_SAVEPOINT,
	IM_ROLLBACK_JOURNAL_NO_ROLLBACK_2_SAVEPOINT,

	//
	// For image list IDB_IJAIMAGELIST2
	IM_TX_BEGIN_COMMIT_JOURNAL,
	IM_TX_BEGIN_COMMIT_JOURNAL_NO,
	IM_TX_BEGIN_ROLLBACK_JOURNAL,
	IM_TX_BEGIN_ROLLBACK_JOURNAL_NO,
	IM_TX_END_COMMIT_JOURNAL,
	IM_TX_END_COMMIT_JOURNAL_NO,
	IM_TX_END_ROLLBACK_JOURNAL,
	IM_TX_END_ROLLBACK_JOURNAL_NO,

	// 
	// For the partial rollback to save point:
	IM_TX_BEGIN_COMMIT_JOURNAL_ROLLBACK_2_SAVEPOINT,
	IM_TX_BEGIN_COMMIT_JOURNAL_NO_ROLLBACK_2_SAVEPOINT,
	IM_TX_BEGIN_ROLLBACK_JOURNAL_ROLLBACK_2_SAVEPOINT,
	IM_TX_BEGIN_ROLLBACK_JOURNAL_NO_ROLLBACK_2_SAVEPOINT,
	IM_TX_END_COMMIT_JOURNAL_ROLLBACK_2_SAVEPOINT,
	IM_TX_END_COMMIT_JOURNAL_NO_ROLLBACK_2_SAVEPOINT,
	IM_TX_END_ROLLBACK_JOURNAL_ROLLBACK_2_SAVEPOINT,
	IM_TX_END_ROLLBACK_JOURNAL_NO_ROLLBACK_2_SAVEPOINT

};

//
// Header Indicator:
#define IM_HEADER_UP    2
#define IM_HEADER_DOWN  3

class CaTemporarySession;
class CaRecoverRedoInformation
{
public:
	CaRecoverRedoInformation(CaTemporarySession* pSession)
	{
		m_pQueryTransactionInfo = NULL;
		m_pListTransaction = NULL;
		m_bScanJournal = FALSE;
		m_bNoRule = FALSE;
		m_bImpersonateUser = FALSE;
		m_bErrorOnAffectedRowCount = TRUE;
		m_pSession = pSession;
	}
	~CaRecoverRedoInformation();
	void DestroyTemporaryTable();
	
	CaQueryTransactionInfo* m_pQueryTransactionInfo;    // Query Info, (Database, node, ...
	CTypedPtrList<CObList, CaIjaTable*> m_listTable;    // List of table and its columns;
	CTypedPtrList<CObList, CaDBObject*> m_listTempTable;// List of temporary tables
	CStringList m_listTempFile;                         // List of temporary Files
	CTypedPtrList < CObList, CaBaseTransactionItemData* >* m_pListTransaction; // List of transaction
	BOOL m_bScanJournal;
	BOOL m_bNoRule;
	BOOL m_bImpersonateUser;
	BOOL m_bErrorOnAffectedRowCount;

	CaTemporarySession* m_pSession;
};

int  IJA_CompareDate  (LPCTSTR lpszDate1, LPCTSTR lpszDate2, BOOL bFromIngresColumn = FALSE);

//
// dateFormat: the enum value of II_DATE_FORMAT
// lpszDate  : the current output date due to the II_DATE_FORMAT
// strConvertDate: converted date to the unique format yyyy-mm-dd [HH:MM:SS]
void IJA_MakeDateStr (II_DATEFORMAT dateFormat, LPCTSTR lpszDate, CString& strConvertDate);
//
// Construct the MFC CTime from format of current auditdb: dd-mmm-yyyy HH:MM:SS.xx
// where xx is the /100 of second:
CTime IJA_GetCTime(CString& strIjaDate);
// Convert date to the expected formatby auditdb:
CString toAuditdbInputDate(CTime& t);

#endif // LOGADATA_HEADER
