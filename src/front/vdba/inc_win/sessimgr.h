/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sessimgr.h: interface for the CaSession, CaSessionManager classes.
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : SQL Session management
**
** History:
**
** 12-Oct-2000 (uk$so01)
**    Created
** 10-Oct-2001 (uk$so01)
**    SIR #106000. CaSessionManager::Cleanup() should not call 
**    m_listObject.RemoveAll(), bexause it removes the entry of sessions.
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 28-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM 
**    Handle autocommit ON/OFF
** 29-Apr-2003 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM 
**    Handle new Ingres Version 2.65
** 23-Jan-2004 (schph01)
**    (sir 104378) detect version 3 of Ingres, for managing
**    new features provided in this version. replaced references
**    to 2.65 with refereces to 3  in #define definitions for
**    better readability in the future
** 22-Sep-2004 (uk$so01)
**    BUG #113104 / ISSUE 13690527, Add the functionality
**    to allow vdba to request the IPM.OCX and SQLQUERY.OCX
**    to know if there is an SQL session through a given vnode
**    or through a given vnode and a database.
** 29-Sep-2004 (uk$so01)
**    BUG #113119, Add readlock mode in the session management
** 05-Oct-2004 (uk$so01)
**    BUG #113185
**    Manage the Prompt for password for the Ingres DBMS user that 
**    has been created with the password when connecting to the DBMS.
** 02-Feb-2006 (drivi01)
**    BUG 115691
**    Keep VDBA tools at the same version as DBMS server.  Update VDBA
**    to 9.0 level versioning. Make sure that 9.0 version string is
**    correctly interpreted by visual tools and VDBA internal version
**    is set properly upon that interpretation.
**/


#if !defined (SESSION_MANAGER_HEADER)
#define SESSION_MANAGER_HEADER
#include "qryinfo.h"

//
// Define the constant name differenct from VDBA to avoid
// conflicts when using this file within the VDBA:
#define INGRESVERS_NOTOI  0
#define INGRESVERS_12     (INGRESVERS_NOTOI + 1)
#define INGRESVERS_20     (INGRESVERS_NOTOI + 2)
#define INGRESVERS_25     (INGRESVERS_NOTOI + 3)
#define INGRESVERS_26     (INGRESVERS_NOTOI + 4)
#define INGRESVERS_30     (INGRESVERS_NOTOI + 5)
#define INGRESVERS_90     (INGRESVERS_NOTOI + 6)

#define DBMS_UNKNOWN      0
#define DBMS_INGRESII     1

//
// SET LOCKMODE:
// *************
#define LM_SESSION         -1
#define LM_SYSTEM          -2
#define LM_PAGE             1
#define LM_TABLE            2
#define LM_ROW              3
#define LM_NOLOCK           4
#define LM_SHARED           5
#define LM_EXCLUSIVE        7
typedef struct tagSETLOCKMODE
{
	TCHAR tchszTable[256]; // can be empty
	int   nLevel;           // 0 or n (n must be LM_PAGE, LM_TABLE, LM_SESSION, LM_SYSTEM, LM_ROW)
	int   nReadLock;        // 0 or n (n must be LM_NOLOCK, LM_SHARED, LM_EXCLUSIVE, LM_SESSION, LM_SYSTEM)
	int   nMaxLock;         // 0 or n (n must be any value > 0 or LM_SESSION, LM_SYSTEM)
	int   nTimeOut;         // 0 or n (n must be any value > 0 or LM_SESSION, LM_SYSTEM)
} SETLOCKMODE;
#define SET_AUTOCOMMIT      8
#define SET_OPTIMIZEONLY    9
#define SET_QEP            10
#define SET_DESCRIPTION    11

//
// CLASS: CaSession
// ************************************************************************************************
typedef enum 
{
	SESSION_NONE = 0,
	SESSION_COMMIT,
	SESSION_ROLLBACK
} SessionReleaseMode;

class CaSession : public CaConnectInfo
{
public:
	CaSession();
	CaSession(const CaSession& c);
	CaSession(CaConnectInfo* pInfo);
	void Init();
	virtual ~CaSession();
	
	CaSession operator = (const CaSession& c);
	BOOL operator ==(const CaSession& c);

	void SetSessionNum (int nVal){m_nSessionNumber = nVal;}
	int  GetSessionNum (){return m_nSessionNumber;}
	BOOL SetSessionNone();
	int  GetVersion(){return m_nIngresVersion;} // INGRESVERS_XX above
	void SetVersion(int nVer){m_nIngresVersion = nVer;}
	CString GetConnection();
	CString GetDescription(){return m_strDesciption;}
	BOOL IsCommitted();
	void SetCommitted(BOOL bSet){m_bCommitted = bSet;} 
	BOOL GetAutoCommit();
	int  GetDbmsType(){return m_nDbmsType;}
	void SetDbmsType(int nType){m_nDbmsType = nType;}

	virtual BOOL Connect();
	virtual BOOL Disconnect();
	virtual BOOL Release(SessionReleaseMode nReleaseMode = SESSION_COMMIT);
	virtual BOOL Activate();
	virtual BOOL IsReleased(){return m_bReleased;}
	virtual BOOL IsConnected(){return m_bConnected;}

	void SetLockMode (SETLOCKMODE* pLockModeInfo);
	void SetAutoCommit (BOOL bSet);
	void SetOptimizeOnly (BOOL bSet);
	void SetQep (BOOL bSet);
	void SetDescription(LPCTSTR lpszDescription);
	void Commit();
	void Rollback();

protected:
	int  m_nSessionNumber;
	BOOL m_bConnected;
	BOOL m_bReleased;
	int  m_nDbmsType; // DBMS_UNKNOWN, DBMS_INGRESII, 
	void Copy (const CaSession& c);
private:
	//
	// This member is set when cretating connection by using EXEC SQL SELECT DBMSINFO('_version')
	int  m_nIngresVersion;
	CString m_strDesciption;
	BOOL m_bCommitted;
};


//
// CLASS: CaSessionManager (Cache of Session)
// ************************************************************************************************
class CaSessionManager : public CObject
{
public:
	CaSessionManager();
	virtual ~CaSessionManager();
	long GetSessionStart(){return m_nSessionStart;}
	void SetSessionStart(long nSessionStart){m_nSessionStart = nSessionStart;}

	virtual CaSession* GetSession(CaSession* pInfo);
	virtual BOOL Disconnect(CaSession* pInfo, BOOL bCommit=TRUE);
	void Cleanup();
	void SetDescription(LPCTSTR lpszDescription){m_strSessionDescription = lpszDescription;}
	CString GetDescription(){return m_strSessionDescription;}
	CTypedPtrArray< CObArray, CaSession* >& GetListSessions(){return m_listObject;}

protected:
	virtual CaSession* SearchSession(CaSession* pInfo);

	CTypedPtrArray< CObArray, CaSession* > m_listObject;
	long m_nMaxSession;
	long m_nIndependent;

	long m_nSessionStart;
	CString m_strSessionDescription;
};


//
// CLASS: CaSessionUsage
// Use session (with cache of session ability):
// ************************************************************************************************
class CaSessionUsage
{
public:
	CaSessionUsage(CaSessionManager* pSessionManager, CaSession* pInfo, SessionReleaseMode nType =  SESSION_ROLLBACK)
		: m_pSessionManager(pSessionManager)
	{
		m_pSession = m_pSessionManager->GetSession(pInfo);
		m_bStatus = m_pSession? TRUE: FALSE;
		m_bReleased = FALSE;
		m_nReleaseMode = nType;
	}
	~CaSessionUsage()
	{
		Release(m_nReleaseMode);
		if (m_pSession && m_pSession->GetIndependent())
			delete m_pSession;
	}
	BOOL IsOK(){return (m_bStatus && m_pSession);}

	void Release(SessionReleaseMode nReleaseMode = SESSION_COMMIT)
	{
		if (!IsOK() || m_bReleased)
			return;
		BOOL bIndependent = m_pSession->GetIndependent();
		if (m_pSession->Release(nReleaseMode))
		{
			m_bStatus = FALSE;
		}
		if (bIndependent)
		{
			m_pSessionManager->Disconnect (m_pSession, nReleaseMode);
			m_pSession = NULL;
		}
		m_bReleased = TRUE;
	}
	CaSession* GetCurrentSession(){return m_pSession;}

private:
	SessionReleaseMode m_nReleaseMode;
	BOOL m_bReleased;
	BOOL m_bStatus;
	CaSession* m_pSession;
	CaSessionManager* m_pSessionManager;
};


//
// Manage the Prompt for password for the Ingres DBMS user that created with
// password.
// I personally prefer to manage these codes in a seperate new DLL called 
// IIDBPROM.DLL that is isolated and can be used by any application that required this
// functionality.
typedef void (CALLBACK* PfnPasswordPrompt)(char* mesg, int noecho, char* reply, int replen);
//
// Setup the HANDLER which is called by the INGRESII_llSetPrompt in 
// TKSPLASH.DLL.
void INGRESII_llInitPasswordPromptHandler(PfnPasswordPrompt pfnPrompt);
//
// The application (usually an executable .exe will call this function
// at the initialization time: it will then invoke the DLL TKSPLASH.DLL
// by calling the export funtion INGRESII_llSetPrompt.
// INGRESII_llSetPrompt will then call INGRESII_llInitPasswordPromptHandler defined
// in this module to setup the HANDLER (passed through the argument) which is 
// defined in TKSPLASH.DLL
BOOL INGRESII_llSetPasswordPrompt();


#endif // SESSION_MANAGER_HEADER