/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : comsessi.cpp: Implementation of the CmtSession, CmtSessionManager classes.
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : SQL Session management
**
** History:
**
** 12-Oct-2000 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
**/

#include "stdafx.h"
#include "comsessi.h"
#include "comllses.h"
#include "constdef.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CmtSession::CmtSession():CaSession()
{

}

CmtSession::~CmtSession()
{
}

CmtSession::CmtSession(const CmtSession& c)
{
	Copy (c);
}

CmtSession::CmtSession(const CaSession& c)
{
	CaSession::Copy (c);
}


BOOL CmtSession::Connect()
{
	CmtMutexReadWrite mAccess (this);

	m_bConnected = CaSession::Connect ();
	m_bReleased = FALSE;

	return m_bConnected;
}

BOOL CmtSession::Disconnect()
{
	BOOL bOk = FALSE;
	CmtMutexReadWrite mAccess (this);

	bOk = CaSession::Disconnect();

	return bOk;
}

BOOL CmtSession::Release(SessionReleaseMode nReleaseMode)
{
	CmtMutexReadWrite mAccess (this);

	BOOL bOk = FALSE;
	bOk = CaSession::Release(nReleaseMode);

	return bOk;
}


BOOL CmtSession::Activate()
{
	CmtMutexReadWrite mAccess (this);

	BOOL bOk = CaSession::Activate();

	return bOk;
}

BOOL CmtSession::IsReleased()
{
	CmtMutexReadWrite mAccess (this);
	return m_bReleased;
}


CmtSession CmtSession::operator = (const CmtSession& c)
{
	Copy (c);
	return *this;
}

BOOL CmtSession::operator ==(const CmtSession& c)
{
	return CaSession::operator == (c);
}

void CmtSession::Copy (const CmtSession& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	CaSession::Copy(c);
}


//
// SESSION MANAGER:
// *******************************************************************
CmtSessionManager::CmtSessionManager():CaSessionManager()
{

}

CmtSessionManager::~CmtSessionManager()
{
	CmtMutexReadWrite mAccess (this);
	Cleanup();
	mAccess.UnLock();
}


BOOL CmtSessionManager::Disconnect(CaSession* pInfo, BOOL bCommit)
{
	CmtMutexReadWrite mAccess (this);
	return CaSessionManager::Disconnect(pInfo, bCommit);
}


CaSession* CmtSessionManager::GetSession(CaSession* pInfo)
{
	CmtMutexReadWrite mAccess (this);
	if (mAccess.IsLocked())
	{
		return CaSessionManager::GetSession (pInfo);
	}
	else
	{
		//
		// Dead lock ?
		return NULL;
	}
	return NULL;
}


CaSession* CmtSessionManager::SearchSession(CaSession* pInfo)
{
	return CaSessionManager::SearchSession(pInfo);
}


