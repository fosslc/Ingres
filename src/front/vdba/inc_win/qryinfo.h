/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qryinfo.h: interface for the CaLLAddAlterDrop, 
**               CaLowLevelQueryInfomation class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Access low lewel data (Ingres Database)
**
** History:
**
** 23-Dec-1999 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 28-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM 
**    Handle autocommit ON/OFF
** 21-Oct-2002 (uk$so01)
**    BUG/SIR #106156 Manually integrate the change #453850 into ingres30
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 29-Sep-2004 (uk$so01)
**    BUG #113119, Add readlock mode in the session management
**/

#if !defined(QUERYINFO_HEADER)
#define QUERYINFO_HEADER
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
class CaSessionManager;

//
// CLASS: CaConnectInfo
// ************************************************************************************************
class CaConnectInfo: public CObject
{
	DECLARE_SERIAL (CaConnectInfo)
public:
	CaConnectInfo();
	CaConnectInfo(LPCTSTR lpszNode, LPCTSTR lpszDatabase, LPCTSTR lpszDBOwner = NULL);
	virtual ~CaConnectInfo(){}
	//
	// Manually manage the object version:
	// When you detect that you must manage the newer version (class members have been changed),
	// the derived class should override GetObjectVersion() and return
	// the value N same as IMPLEMENT_SERIAL (CaXyz, XXX, N).
	virtual UINT GetObjectVersion(){return 1;}

	virtual void Serialize (CArchive& ar);
	CaConnectInfo(const CaConnectInfo& c);
	CaConnectInfo operator = (const CaConnectInfo& c);
	void Copy (const CaConnectInfo& c);
	BOOL Matched (const CaConnectInfo& c);

	CString GetUser() const {return m_strUser;}
	CString GetNode() const {return m_strNode;}
	CString GetDatabase() const {return m_strDatabase;}
	CString GetDatabaseOwner() const {return m_strDatabaseOwner;}
	CString GetOptions () const {return m_strOptions;}
	CString GetServerClass () const {return m_strServerClass;}
	UINT GetFlag() const {return  m_nFlag;}
	BOOL GetIndependent() const {return m_bIndependent;}
	UINT GetLockMode() const {return  m_nSessionLock;}


	void SetUser(LPCTSTR lpszText){m_strUser = lpszText;}
	void SetNode(LPCTSTR lpszText){m_strNode = lpszText;}
	void SetDatabase(LPCTSTR lpszText, LPCTSTR lpszOwner = NULL){m_strDatabase = lpszText; m_strDatabaseOwner = lpszOwner;}
	void SetOptions (LPCTSTR lpszText){m_strOptions = lpszText;}
	void SetServerClass (LPCTSTR lpszText){m_strServerClass = lpszText;}
	void SetFlag(UINT nFlag){m_nFlag = nFlag;}
	void SetIndependent(BOOL bSet){m_bIndependent = bSet;}
	void SetLockMode(UINT nLock){m_nSessionLock = nLock;}

protected:

	CString m_strNode;          // Virtual Node Name
	CString m_strDatabase;      // Database Name
	CString m_strDatabaseOwner; // Reserved
	//
	// The flags parameter to the Connection (EXEC SQL CONNECT ...):
	CString m_strUser;          // User name, for connect ... identified by m_strUser
	CString m_strServerClass;
	CString m_strOptions;       // -u or -R flag ex: -u<username> -R<roleid/password>
	UINT    m_nFlag;            // DBFLAG_STARNATIVE ...
	BOOL    m_bIndependent;     // Connect independent session
	UINT    m_nSessionLock;     // 0=ignore, LM_NOLOCK, LM_SHARED, LM_EXCLUSIVE, ...
};



//
// CLASS: CaLLQueryInfo
// ************************************************************************************************
class CaLLQueryInfo: public CaConnectInfo
{
	DECLARE_SERIAL (CaLLQueryInfo)
public:
	enum {FETCH_USER = 0, FETCH_SYSTEM, FETCH_ALL};
	CaLLQueryInfo();
	CaLLQueryInfo(int nObjType, LPCTSTR lpszNode, LPCTSTR lpszDatabase = NULL);
	CaLLQueryInfo(const CaLLQueryInfo& c);
	CaLLQueryInfo operator = (const CaLLQueryInfo& c);
	virtual ~CaLLQueryInfo(){}
	virtual void Serialize (CArchive& ar);
	void Initialize ();

	CString GetConnectionString();
	int  GetFetchObjects(){return m_nFetch;}
	BOOL GetIncludeSystemObjects(){return (m_nFetch == FETCH_ALL);}
	int  GetObjectType(){return m_nTypeObject;}
	int  GetSubObjectType(){return m_nSubObjectType;}

	CString GetItem1(){return GetDatabase();}
	CString GetItem1Owner(){return m_strDatabaseOwner;}
	CString GetItem2(){return m_strItem2;}
	CString GetItem2Owner(){return m_strItem2Owner;}

	void SetFetchObjects(int nFetch){m_nFetch = nFetch;}
	void SetIncludeSystemObjects(BOOL bSet){m_nFetch = bSet? FETCH_ALL: FETCH_USER;}
	void SetObjectType(int nType){m_nTypeObject = nType;}
	void SetSubObjectType(int nType){m_nSubObjectType = nType;}
	void SetItem1(LPCTSTR lpszItem1, LPCTSTR lpszItem1Owner = NULL){SetDatabase(lpszItem1, lpszItem1Owner);}
	void SetItem2(LPCTSTR lpszItem2, LPCTSTR lpszItem2Owner){m_strItem2 = lpszItem2; m_strItem2Owner = lpszItem2Owner;}
	void SetItem3(LPCTSTR lpszItem3, LPCTSTR lpszItem3Owner){m_strItem3 = lpszItem3; m_strItem3Owner = lpszItem3Owner;}

	//
	// Data:
	int  m_nTypeObject;      // OBT_XX (Ex: OBT_VIRTNODE, OBT_USER, OBT_DATABASE, ...)
	CString m_strItem2;      // Usually a Table Name
	CString m_strItem2Owner; // Usually a Table Owner
	CString m_strItem3;      // Ex: Child of item2. Ex Index
	CString m_strItem3Owner;

protected:
	int m_nSubObjectType;    // Must be used when querying grantees, security alarms.
	int m_nFetch;
	void Copy (const CaLLQueryInfo& c);
};



//
// CLASS: CaLLAddAlterDrop
// ************************************************************************************************
class CaLLAddAlterDrop: public CaConnectInfo
{
	DECLARE_SERIAL (CaLLAddAlterDrop)
public:
	CaLLAddAlterDrop():CaConnectInfo()
	{
		m_strStatement = _T("");
		m_bCommit = TRUE;
		m_lAffectedRows = -1;
	}
	CaLLAddAlterDrop(LPCTSTR lpszNode, LPCTSTR lpszDatabase, LPCTSTR lpszStatement, BOOL bSessionIndependent = FALSE):
		CaConnectInfo(lpszNode, lpszDatabase, _T("")), m_strStatement(lpszStatement)
	{
		SetIndependent(bSessionIndependent);
		m_bCommit = TRUE;
		m_lAffectedRows = -1;
	}

	virtual ~CaLLAddAlterDrop(){}
	virtual void Serialize (CArchive& ar);

	void SetStatement(LPCTSTR lpszStatement){m_strStatement= lpszStatement;}
	CString GetStatement(){return m_strStatement;}
	BOOL ExecuteStatement(CaSessionManager* pSessionManager);
	void SetCommitInfo(BOOL bSet){m_bCommit = bSet;}
	BOOL GetCommitInfo(){return m_bCommit;}
	void SetAffectedRows(long lVal){m_lAffectedRows = lVal;}
	long GetAffectedRows(){return m_lAffectedRows;}
protected:
	CString m_strStatement;
	BOOL m_bCommit; // If TRUE instrcut the user that the session must be committed.
	long m_lAffectedRows;
};




#endif // !defined(QUERYINFO_HEADER)
