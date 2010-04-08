/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : sessimgr.cpp: implementation of the CaSession, CaSessionManager classes.
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : SQL Session management
**
** History:
**
** 12-Oct-2000 (uk$so01)
**    Created
** 19-Sep-2001 (uk$so01)
**    BUG #105759 (Win98 only). Exit iia in the Step 2, the Disconnect
**    session did not return.
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
** 17-Mar-2003 (schph01)
**    Use /STAR for connection only on Database Distributed.
** 03-Oct-2003 (uk$so01)
**    SIR #106648, Vdba-Split, Additional fix for GATEWAY Enhancement 
** 23-Jan-2004 (schph01)
**    (sir 104378) detect version 3 of Ingres, for managing
**    new features provided in this version. replaced references
**    to 2.65 with refereces to 3  in #define definitions for
**    better readability in the future
** 29-Sep-2004 (uk$so01)
**    BUG #113119, Add readlock mode in the session management
** 05-Oct-2004 (uk$so01)
**    BUG #113185
**    Manage the Prompt for password for the Ingres DBMS user that 
**    has been created with the password when connecting to the DBMS.
**/

#include "stdafx.h"
#include "sessimgr.h"
#include "constdef.h"
#include "ingrsess.h"
#include "usrexcep.h"
#include "ingobdml.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// CLASS: CaSession
// ************************************************************************************************
CaSession::CaSession():CaConnectInfo()
{
	Init();
}

void CaSession::Init()
{
	m_nSessionNumber = 0;
	m_bConnected = FALSE;
	m_bReleased  = TRUE;
	m_nIngresVersion = INGRESVERS_NOTOI;
	m_nDbmsType = DBMS_UNKNOWN;

	m_strDesciption = _T("");
	m_bCommitted = FALSE;
}

CaSession::~CaSession()
{
	if (m_bIndependent && m_bConnected)
		Disconnect();
}

CaSession::CaSession(const CaSession& c)
{
	Copy (c);
}

CaSession::CaSession(CaConnectInfo* pInfo):CaConnectInfo()
{
	Init();
	CaConnectInfo::Copy((const CaConnectInfo&)(*pInfo));
}


CString CaSession::GetConnection()
{
	CString strSession = _T("");
	BOOL bNode = FALSE;
	if (m_strDatabase.IsEmpty())
		m_strDatabase = _T("iidbdb");
	if (m_strNode.IsEmpty() || m_strNode.CompareNoCase(_T("(local)")) == 0)
		strSession = m_strDatabase;
	else
		strSession.Format (_T("%s::%s"), (LPCTSTR)m_strNode, (LPCTSTR)m_strDatabase);
	if (!m_strServerClass.IsEmpty())
	{
		strSession += _T("/");
		strSession += m_strServerClass;
	}
	//
	// Check if it is a star database:
	UINT nFlag = GetFlag();
	if ((nFlag & DBFLAG_STARNATIVE))
		strSession += _T("/STAR");

	return strSession;
}

BOOL CaSession::Connect()
{
	ASSERT(!m_bConnected);
	if (m_bConnected)
		return FALSE;

	m_bConnected = INGRESII_llConnectSession (this);
	m_bReleased = FALSE;
	return m_bConnected;
}

BOOL CaSession::Disconnect()
{
	if (!m_bConnected)
		return FALSE;
	if (INGRESII_llDisconnectSession (this))
	{
		m_bConnected = FALSE;
		m_bReleased  = TRUE;
		return TRUE;
	}

	return FALSE;
}

BOOL CaSession::Release(SessionReleaseMode nReleaseMode)
{
	BOOL bOk = FALSE;
	if (m_bConnected)
	{
		if (GetIndependent())
		{
			bOk = INGRESII_llDisconnectSession(this, nReleaseMode);
			if (bOk)
			{
				m_bConnected = FALSE;
				m_bReleased  = TRUE;
			}
		}
		else
		{
			m_bReleased = INGRESII_llReleaseSession(this, nReleaseMode);
		}
	}

	return bOk;
}


BOOL CaSession::SetSessionNone()
{
	return INGRESII_llSetSessionNone(this);
}

BOOL CaSession::Activate()
{
	BOOL bOk = FALSE;
	ASSERT(m_nSessionNumber > 0 && m_bConnected);
	if (m_nSessionNumber > 0 && m_bConnected)
		bOk = INGRESII_llActivateSession (this);
	if (bOk)
		m_bReleased = FALSE;

	return bOk;
}


CaSession CaSession::operator = (const CaSession& c)
{
	Copy (c);
	return *this;
}

BOOL CaSession::operator ==(const CaSession& c)
{
	if (!CaConnectInfo::Matched((const CaConnectInfo&)c))
		return FALSE;
	return TRUE;
}

void CaSession::Copy (const CaSession& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	CaConnectInfo::Copy(c);
	m_nSessionNumber = c.m_nSessionNumber;
	m_strDesciption  = c.m_strDesciption;
	//
	// Do not affect the m_bConnected, private usage only !
	m_bConnected = FALSE;
	m_nSessionNumber = 0;
	m_bReleased = FALSE;
	m_nDbmsType = DBMS_UNKNOWN;
	m_nIngresVersion = -1;
	m_bCommitted = FALSE;
}


void CaSession::SetLockMode (SETLOCKMODE* pLockModeInfo)
{
	ASSERT (m_bConnected); // The session must be connected first
	if(!m_bConnected)
		return;
	INGRESII_llSetLockMode(this, pLockModeInfo);
}

void CaSession::SetAutoCommit (BOOL bSet)
{
	INGRESII_llSetSql(this, SET_AUTOCOMMIT, bSet);
}

void CaSession::SetOptimizeOnly (BOOL bSet)
{
	INGRESII_llSetSql(this, SET_OPTIMIZEONLY, bSet);
}


void CaSession::SetQep (BOOL bSet)
{
	INGRESII_llSetSql(this, SET_QEP, bSet);
}

void CaSession::SetDescription(LPCTSTR lpszDescription)
{
	m_strDesciption = lpszDescription;
	if(m_bConnected)
		INGRESII_llSetSql(this, SET_DESCRIPTION, TRUE);
}

void CaSession::Commit()
{
	INGRESII_llCommitSession (this, TRUE);
	m_bCommitted = TRUE;
}

void CaSession::Rollback()
{
	INGRESII_llCommitSession (this, FALSE);
	m_bCommitted = TRUE; // Session has been rollbacked (nothing to commit)
}

BOOL CaSession::GetAutoCommit()
{
	BOOL bSet = INGRESII_llGetAutoCommit (this);
	return bSet;
}


BOOL CaSession::IsCommitted()
{
	if (GetVersion() >= INGRESVERS_30)
	{
		if (m_bConnected)
			Activate();
		else
			return TRUE;
		CString strBuff = INGRESII_llDBMSInfo(_T("transaction_state"));
		if (strBuff.IsEmpty())
			return TRUE;
		return (strBuff[0] == _T('0'))? TRUE: FALSE;
	}
	else
	{
		return m_bCommitted;
	}
}


//
// CLASS: CaSessionManager
// ************************************************************************************************
CaSessionManager::CaSessionManager()
{
	m_nSessionStart = 1;
	//
	// Max session in the cache, session number [m_nSessionStart; m_nSessionStart+16]
	m_nMaxSession = 16;
	m_nIndependent= m_nMaxSession+1;
	m_listObject.SetSize (m_nMaxSession, 5);
	for (int i=0; i<m_nMaxSession; i++)
		m_listObject[i] = NULL;
	m_strSessionDescription = _T("");
}

void CaSessionManager::Cleanup()
{
	CaSession* pObj = NULL;
	int nSize = m_listObject.GetSize();

	for (int i=0; i<nSize; i++)
	{
		pObj = m_listObject[i];
		try
		{
			if (pObj)
				pObj->Disconnect();
			m_listObject[i] = NULL;
		}
		catch(...)
		{
			TRACE1("Disconnect session failed: %s\n", (LPCTSTR)pObj->GetConnection());
		}
		//
		// Continue to proceed anyhow !
		if (pObj)
			delete pObj;
	}
}

CaSessionManager::~CaSessionManager()
{
	Cleanup();
	m_listObject.RemoveAll();
}



CaSession* CaSessionManager::GetSession(CaSession* pInfo)
{
	//
	// First of all, check if the session number exceeds the limit.
	// If so, try to destroy those extra sessions:
	CaSession* pObj = NULL;
	int i, nSize = m_listObject.GetSize();
	if (nSize > m_nMaxSession)
	{
		for (i=(nSize -1); i>=m_nMaxSession; i--)
		{
			pObj = m_listObject[i];
			if (pObj && pObj->IsReleased())
			{
				m_listObject[i] = NULL;
				pObj->Disconnect();
				delete pObj;
				pObj = NULL;
			}
		}
	}

	//
	// Search if there is an available session:
	pObj = SearchSession(pInfo);
	if (pObj)
	{
		if (pObj->IsConnected())
			pObj->Activate();
		else
			pObj->Connect();
		return pObj;
	}
	else
	{
		//
		// No session available in the cache and there is no more entry for the new session !
		// All session are used!
		int nExtra = nSize;
		for (i = m_nMaxSession; i < nSize; i++)
		{
			pObj = m_listObject[i];
			if (!pObj)
			{
				nExtra = i;
				break;
			}
		}

		pObj = new CaSession (*pInfo);
		if (nExtra >= nSize)
		{
			nExtra = m_listObject.Add(pObj);
			pObj->SetSessionNum (nExtra + m_nSessionStart);
		}
		else
		{
			pObj->SetSessionNum (nExtra + m_nSessionStart);
			m_listObject.SetAt(nExtra, pObj);
		}
		pObj->Connect();
		return pObj;
	}

	return NULL;
}

BOOL CaSessionManager::Disconnect(CaSession* pInfo, BOOL bCommit)
{
	CaSession* pObj = NULL;
	int i, nSize = m_listObject.GetSize();

	for (i=0; i<nSize; i++)
	{
		pObj = m_listObject[i];
		if (pObj && pInfo == pObj)
		{
			m_listObject[i] = NULL;
			pObj->Disconnect();
			delete pObj;
			return TRUE;
		}
	}
	return FALSE;
}


CaSession* CaSessionManager::SearchSession(CaSession* pInfo)
{
	CaSession* pObj = NULL;
	int i, nAvail = -1;
	for (i=0; i<m_nMaxSession; i++)
	{
		pObj = m_listObject[i];
		if (!pObj && nAvail == -1)
		{
			nAvail = i;
		}

		if (pObj && pObj->IsReleased() && (*pObj == *pInfo))
			return pObj;
	}

	//
	// Create a new session:
	// Find the first released session and disconnect it.
	for (i=0; i<m_nMaxSession && nAvail ==-1; i++)
	{
		pObj = m_listObject[i];
		if (!pObj)
		{
			nAvail = i;
			break;
		}

		if (pObj && pObj->IsReleased())
		{
			//
			// Disconnected the unused session to free the slot:
			nAvail = i;
			pObj->Disconnect();
			delete pObj;
			m_listObject[i] = NULL;
			break;
		}
	}

	if (nAvail != -1)
	{
		try
		{
			pObj = new CaSession (*pInfo);
			pObj->SetSessionNum (nAvail + m_nSessionStart);
			pObj->Connect();
			m_listObject[nAvail] = pObj;
			return pObj;
		}
		catch(CeSqlException e)
		{
			if (pObj)
				delete pObj;
			throw e;
		}
	}

	return NULL;
}

void INGRESII_llInitPasswordPromptHandler(PfnPasswordPrompt pfnPrompt)
{
	INGRESII_llSetPasswordPromptHandler((PfnIISQLHandler)pfnPrompt);
}


BOOL INGRESII_llSetPasswordPrompt()
{
	BOOL bOK = TRUE;
	CString strDllName = _T("tksplash.dll");
#if defined (MAINWIN)
	#if defined (hpb_us5)
	strDllName  = _T("libtksplash.sl");
	#else
	strDllName  = _T("libtksplash.so");
	#endif
#endif
	HINSTANCE hLib = LoadLibrary(strDllName);
	if (hLib < (HINSTANCE)HINSTANCE_ERROR)
		bOK = FALSE;
	if (bOK)
	{
		void (PASCAL *lpDllEntryPoint)();
		(FARPROC&)lpDllEntryPoint = GetProcAddress (hLib, "INGRESII_llSetPrompt");
		if (lpDllEntryPoint == NULL)
			bOK = FALSE;
		else
		{
			(*lpDllEntryPoint)();
		}
	}

	return bOK;
}

