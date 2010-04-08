/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : comsessi.h: interface for the CmtSession, CmtSessionManager classes.
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


#if !defined (COMSESSION_MANAGER_HEADER)
#define COMSESSION_MANAGER_HEADER
#include "combase.h"
#include "sessimgr.h"

//
// General Connect to DBMS server:
class CmtSession : public CaSession, public CmtProtect
{
public:
	CmtSession();
	CmtSession(const CmtSession& c);
	CmtSession(const CaSession&  c);
	virtual ~CmtSession();
	
	CmtSession operator = (const CmtSession& c);
	BOOL operator ==(const CmtSession& c);

	virtual BOOL Connect();
	virtual BOOL Disconnect();
	virtual BOOL Release(SessionReleaseMode nReleaseMode = SESSION_COMMIT);
	virtual BOOL Activate();
	virtual BOOL IsReleased();

protected:
	void Copy (const CmtSession& c);

};


//
// Session Manager
// ***************
class CmtSessionManager : public CaSessionManager, public CmtProtect
{
public:
	CmtSessionManager();
	virtual ~CmtSessionManager();

	virtual CaSession* GetSession(CaSession* pInfo);
	BOOL Disconnect(CaSession* pInfo, BOOL bCommit=TRUE);
	void Cleanup(){CaSessionManager::Cleanup();}

protected:
	virtual CaSession* SearchSession(CaSession* pInfo);

};



#endif // SESSION_MANAGER_HEADER