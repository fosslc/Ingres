/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : ingrsess.h: header file 
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Low lewel functions
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
** 28-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM 
**    Handle autocommit ON/OFF
** 05-Oct-2004 (uk$so01)
**    BUG #113185
**    Manage the Prompt for password for the Ingres DBMS user that 
**    has been created with the password when connecting to the DBMS.
**/

#if !defined(INGRSESS_HEADER)
#define INGRSESS_HEADER

#include "sessimgr.h"
#include "ingobdml.h"


BOOL INGRESII_llConnectSession   (CaSession* pInfo);
BOOL INGRESII_llReleaseSession   (CaSession* pInfo, SessionReleaseMode nReleaseMode = SESSION_COMMIT);
BOOL INGRESII_llDisconnectSession(CaSession* pInfo, SessionReleaseMode nReleaseMode = SESSION_ROLLBACK);
BOOL INGRESII_llActivateSession  (CaSession* pInfo);
BOOL INGRESII_llSetSessionNone   (CaSession* pInfo);
BOOL INGRESII_llSetLockMode (CaSession* pInfo, SETLOCKMODE* pLockModeInfo);
BOOL INGRESII_llSetSql(CaSession* pInfo, int nSetWhat, BOOL bSet);
BOOL INGRESII_llCommitSession (CaSession* pInfo, BOOL bCommit); // bCommit = TRUE-> Commit, FALSE -> Rollback
BOOL INGRESII_llGetAutoCommit (CaSession* pInfo);
BOOL INGRESII_llIsSessionCommitted (CaSession* pInfo);
void INGRESII_llSetPasswordPromptHandler(PfnIISQLHandler handler);

#endif // INGRSESS_HEADER