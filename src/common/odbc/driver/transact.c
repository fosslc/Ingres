/*
**  (C) Copyright 1992,2004 Ingres Corporation
*/

#include <compat.h>
#ifndef VMS
#include <systypes.h>
#endif
#include <sql.h>                    /* ODBC Core definitions */
#include <sqlext.h>                 /* ODBC extensions */
#include <sqlstate.h>               /* ODBC driver sqlstate codes */
#include <sqlcmd.h>

#include <st.h> 
#include <tr.h>

#include <sqlca.h>
#include <idmsxsq.h>                /* control block typedefs */
#include <idmsutil.h>               /* utility stuff */
#include "caidoopt.h"              /* connect options */
#include "idmsoinf.h"               /* information definitions */
#include "idmsoids.h"               /* string ids */
#include <idmseini.h>               /* ini file keys */
#include "idmsodbc.h"               /* ODBC driver definitions             */

/*
//  TRANSACT.C
//
//  Process transaction for ODBC driver.
//
//  Change History
//  --------------
//
//  date        programmer          description
//
//  12/10/1992  Dave Ross           Initial coding
//   3/14/1995  Dave Ross           ODBC 2.0 upgrade...
//  02/07/1997  Dave Thole          Moved connH/tranH to SESS
//  03/24/1997  Jean Teng           Closed open stmts before commit
//  04/16/1997  Jean Teng           SetTransaction ...
//  04/29/1997  Jean Teng           bug fix release number 
//  04/30/1997  Jean Teng           SetTransaction ... if in transaction return success 
//  05/09/1997  Dave Thole          SetTransaction only if INGRES server class
//  05/20/1997  Jean Teng           don't SetTransaction if not Ingres
//  07/10/1997  Dave Thole          Break up SetTransaction "READ ONLY" and "ISOLATION"
//  07/10/1997  Dave Thole          Back off prior change and simply insert a comma.
//  11/11/1997  Dave Thole          Documentation clean-up
//  12/04/1997  Jean Teng           convert c run-time functions to CL 
//  02/26/1998  Dave Thole          Turn on autocommit=on only after SetTransaction
//  03/18/1998  Dave Thole          Turn on autocommit=on for non-Ingres also
//  10/26/1998  Dave Thole          Added support for SQL_ATTR_ENLIST_IN_DTC
//  01/20/1999  Dave Thole          Add AnyIngresCursorOpen, SQLTransact_InternalCall.
//  02/10/1999  Dave Thole          Use connection's SQLCA for commit/rollback errors
//  03/15/1999  Dave Thole          Port to UNIX.
//  11/11/1999  Dave Thole          Avoid duplicate SET TRANSACTION statements.
//  03/13/2000  Dave Thole          Add support for Ingres Read Only.
//  01/11/2001  Dave Thole          Integrate MTS/DTC support into 2.8
**	29-oct-2001 (thoda04)
**	    Corrections to 64-bit compiler warnings change.
**	07-feb-2002 (weife01)
**	    Bug#106831: Make read-only driver back compatible to
**	    allow updating in procedure as per document.
**      20-apr-2002 (loera01) Bug 107536
**          For AnyIngresCursorOpen(), be sure to check internal opened statements 
**          as well as ordinary opened cursors.
**	18-apr-2002 (weife01) Bug #106831
**          Add option in advanced tab of ODBC admin which allow the procedure 
**          update with odbc read only driver 
**      30-sep-2002 (loera01) Bug 108831
**          SetTransaction() constructs a "SET TRANSACTION..." query to the
**          dbms.  Since the intention is for the SET command to persist,
**          change this to "SET SESSION".
**      11-oct-2002 (loera01) Bug 108819
**          The CloseCursors() routine should not reset the status blocks
**          for all statement handles.  Reset only for open statements
**          with API cursors opened.  
**      12-oct-2002 (loera01)
**          Remove C++ style comments from CloseCursor().
**      14-feb-2003 (loera01) SIR 109643
**          Minimize platform-dependency in code.  Make backward-compatible
**          with earlier Ingres versions.
**     22-sep-2003 (loera01)
**          Added internal ODBC tracing.
**     17-oct-2003 (loera01)
**          Changed "IIsyc_" routines to "odbc_".
**     13-nov-2003 (loera01)
**          Removed obsolete arguments from SqlToIngresAPI().
**     20-nov-2003 (loera01)
**          Removed unused SQL_CB_PRESERVE.  Changed SQL_CB_DELETE
**          to SQL_AUTOCOMMIT_OFF and SQL_CB_CLOSE to    
**          SQL_AUTOCOMMIT_ON.
**	23-apr-2004 (somsa01)
**	    Include systypes.h to avoid compile errors on HP.
**     14-Sep-2004 (loera01) Bug 113042
**          In SetTransaction(), executed "SET SESSION" query outside of
**          the ODBC query state machine.  Otherwise, the state machine
**          advances as according to flags set from incompatible states.
**     15-nov-2004 (Ralph.Loen@ca.com) Bug 113462
**          Use IItrace global block to handle tracing.
*/


static RETCODE SQL_API SQLTransact_Common(LPENV penv, LPDBC pdbc, UWORD fType);


/*
**  SQLEndTran
**
**  SQLEndTran requests a commit or rollback operation for all active 
**  operations on all statements associated with a connection. SQLEndTran 
**  can also request that a commit or rollback operation be performed for 
**  all connections associated with an environment.
**
**  On entry:
**    HandleType [Input]
**       Handle type identifier: 
**         SQL_HANDLE_ENV if Handle is an environment handle, or 
**         SQL_HANDLE_DBC if Handle is a connection handle.
**    Handle [Input]
**       The handle, of the type indicated by HandleType, indicating the 
**       scope of the transaction.
**    CompletionType [Input]
**       SQL_COMMIT
**       SQL_ROLLBACK
**
**  Returns:
**    SQL_SUCCESS
**    SQL_SUCCESS_WITH_INFO
**    SQL_INVALID_HANDLE
**    SQL_ERROR
**
*/
SQLRETURN  SQL_API SQLEndTran (
    SQLSMALLINT HandleType,
    SQLHANDLE   Handle,
    SQLSMALLINT CompletionType)
{
    SQLRETURN   rc = SQL_SUCCESS;
    LPENV       penv;
    LPDBC       pdbc;

    switch(HandleType)
       {
        case SQL_HANDLE_ENV:

            penv = (LPENV)Handle;
            ErrResetEnv (penv);
            /*
            **  Make sure ENV-->DBC chain stays valid while do this
            */
            LockEnv (NULL);  /* lock the whole environment */

            /*
            **  Loop through all DBCs
            */
            for (pdbc = penv->pdbcFirst; pdbc; pdbc = pdbc->pdbcNext)
            {
                rc = SQLTransact_InternalCall(SQL_NULL_HENV,
                                              pdbc, CompletionType);
                if (rc == SQL_ERROR)
                    break;
            }  /* end for loop through ENV-->DBC chain */

            UnlockEnv (NULL);  /* unlock the environment */
            return rc;

        case SQL_HANDLE_DBC:

            pdbc = (LPDBC)Handle;

            if (!LockDbc (pdbc))
                return SQL_INVALID_HANDLE;

            rc = SQLTransact_Common(SQL_NULL_HENV,
                                    pdbc, CompletionType);
       }  /* end switch(HandleType) */

    return(rc);
}


/*
//  SQLTransact
//
//  Control an SQL transaction.
//
//  On entry: penv -->environment block (in driver manager).
//            pdbc -->connection block.
//            fType = transaction operation type.
//
//  On exit:  Transaction is committed or rolled back.
//            See following notes for state of any cursors
//            and prepared statements.
//
//  Returns:  SQL_SUCCESS
//            SQL_SUCCESS_WITH_INFO
//            SQL_ERROR
//            SQL_INVALID_HANDLE (driver manager)
//
//  ODBC defines three levels of cursor commit behavior and three independent
//  levels of cursor rollback behavior:
//
//  SQL_CB_DELETE   Cursors are closed. Prepared statements are deleted.
//  SQL_CB_CLOSE    Cursors are closed. Prepared statements can be reexecuted.
//  SQL_CB_PRESERVE Cursors remain open and at their current row. Prepared
//                  statements can be reexecuted.
//
//
*/
RETCODE SQL_API SQLTransact(
    SQLHENV henv,
    SQLHDBC hdbc,
    UWORD fType)
{
    if (!LockDbc ((LPDBC)hdbc)) 
        return SQL_INVALID_HANDLE;
    return(SQLTransact_Common((LPENV)henv, (LPDBC)hdbc, fType));
}

static RETCODE SQL_API SQLTransact_Common(
    LPENV penv,
    LPDBC pdbc,
    UWORD fType)
{
    RETCODE rc;
    LPSESS  psess;
    LPSTMT  pstmt;
    BOOL    fCommitCat = TRUE;
    BOOL    fCommitSql = TRUE;

    TRACE_INTL(pdbc,"SQLTransact_common");

    if (IItrace.fTrace >= ODBC_EX_TRACE)
        TraceOdbc (SQL_API_SQLTRANSACT, penv, pdbc, fType);

    ErrResetDbc (pdbc);        /* clear any errors on DBC */

    /*
    **  Free up catalog session as much as we can:
    */
    psess = &pdbc->sessCat;
    pdbc->sqb.pSession= psess;
    pdbc->sqb.pSqlca  = &pdbc->sqlca;       /* use connection's SQLCA for SQL calls */

    if (psess->fStatus & SESS_CONNECTED)
    {
        if (fCommitCat && psess->fStatus & SESS_STARTED) /* if cat transaction started */
        {
            pdbc->sqb.Options = SQB_SUSPEND;

			/* close all stmts otherwise API would return error on commit */
			for (pstmt = pdbc->pstmtFirst; pstmt; pstmt = pstmt->pstmtNext)
			{
				if ((pstmt->stmtHandle) && (pstmt->fStatus & STMT_CATALOG))
				{
					pdbc->sqb.pSqlca = &pstmt->sqlca;
					pdbc->sqb.pStmt = pstmt;
					SqlClose (&pdbc->sqb);
					pstmt->fStatus &= ~(STMT_OPEN + STMT_PREPARED + 
                                                    STMT_API_CURSOR_OPENED); 
					ResetStmt(pstmt, pdbc->fCommit == SQL_AUTOCOMMIT_OFF);
                         /* clear STMT state and free/keep buffers */
				}
			}

			pdbc->sqb.pSession= psess;
            pdbc->sqb.pSqlca  = &pdbc->sqlca; 
            SqlCommit (&pdbc->sqb);

            psess->fStatus &= ~SESS_STARTED;  /* no cat transaction active */
            rc = SQLCA_ERROR (pdbc, psess);
            if (rc == SQL_SUCCESS)
                psess->fStatus |= SESS_SUSPENDED;
        }
        else if (!(psess->fStatus & SESS_SUSPENDED))
        {
            SqlSuspend (&pdbc->sqb);

            rc = SQLCA_ERROR (pdbc, psess);
            if (rc == SQL_SUCCESS)
                psess->fStatus |= SESS_SUSPENDED;
        }
    }

    /*
    **  Make main session changes stick or wash them off:
    */
    psess = &pdbc->sessMain;
    pdbc->sqb.pSession= psess;
    pdbc->sqb.Options = SQB_SUSPEND;
	
	/* close all stmts otherwise API would return error on commit */
	for (pstmt = pdbc->pstmtFirst; pstmt; pstmt = pstmt->pstmtNext)
	{
		if ((pstmt->stmtHandle) && !(pstmt->fStatus & STMT_CATALOG))
		{
			pdbc->sqb.pSqlca = &pstmt->sqlca;
			pdbc->sqb.pStmt = pstmt;
			SqlClose (&pdbc->sqb);
			pstmt->fStatus &= ~(STMT_OPEN + STMT_PREPARED +
                                            STMT_API_CURSOR_OPENED); 
			ResetStmt(pstmt, pdbc->fCommit == SQL_AUTOCOMMIT_OFF);
                 /* clear STMT state and free/keep buffers */
		}
	}

    if (fType == SQL_COMMIT)
    {
        pdbc->sqb.pSqlca  = &pdbc->sqlca; /* use connection's SQLCA for errors */
        if (fCommitSql)
        {
            SqlCommit (&pdbc->sqb);
            psess->fStatus &= ~SESS_STARTED;/* no main transaction active */
        }
    }
    else if (fType == SQL_ROLLBACK)
    {
        pdbc->sqb.pSqlca  = &pdbc->sqlca; /* use connection's SQLCA for errors */
        SqlRollback (&pdbc->sqb);
        psess->fStatus &= ~SESS_STARTED;
    }
    psess->fStatus &= ~SESS_COMMIT_NEEDED;
    rc = SQLCA_ERROR (pdbc, psess);
    if (rc == SQL_SUCCESS)
        psess->fStatus |= SESS_SUSPENDED;

    /*
    **  Close any "open" cursors closed by commit:
    */
    CloseCursors (pdbc);
    
    UnlockDbc (pdbc);
    return rc;
}

/*
**  SQLTransact_InternalCall
**
**  Lock the DBC without locking the ENV and then call SQLTransact().
**
**  On entry: pdbc -->connection block;
**
**  Returns:  SQL status of SQLTransact
*/
RETCODE SQL_API SQLTransact_InternalCall(LPENV penv, LPDBC pdbc, UWORD fType)
{
    if (!LockDbc_Trusted (pdbc)) return SQL_INVALID_HANDLE;
    return(SQLTransact_Common(penv, pdbc, fType));
}


/*
**  CloseCursors
**
**  Mark all cursors on a connection as closed and not prepared.
**  Only called after a commit is issued when commit behavior is not preserve.
**
**  On entry: pdbc -->connection block;
**
**  Returns:  Nothing
*/
VOID CloseCursors (
    LPDBC   pdbc)
{
    LPSTMT  pstmt;

    TRACE_INTL(pdbc,"CloseCursors");
    for (pstmt = pdbc->pstmtFirst; pstmt; pstmt = pstmt->pstmtNext)
    {
        if (pdbc->fCommit == SQL_AUTOCOMMIT_OFF)
        {
            pstmt->fStatus = 0;             /* off all status flags */
            FreeStmtBuffers (pstmt);        /* can't use these any more */
            FreeSqlStr (pstmt);             /* return to allocated state */
        }
        else if (pstmt->fStatus & STMT_OPEN &&
            (pstmt->fStatus & STMT_API_CURSOR_OPENED ||
             pstmt->fStatus & STMT_INTERNAL))
        {
            pstmt->fStatus &= STMT_RESET;
        }
    }
    return;
}


/*
**  CursorOpen
**
**  Check if any (other) cursor is open on connection.
**
**  On entry: pdbc     -->connection block;
**            fCatalog  = catalog session statement flag
**                        STMT_CATALOG to check for catalog cursors only
**                        0            to check on caller cursors only
**
**  Returns:  TRUE if a cursor is open
**            FALSE if not
*/
BOOL CursorOpen (
    LPDBC   pdbc,
    UWORD   fCatalog)
{
    LPSTMT  pstmt;

    TRACE_INTL(pdbc,"CursorOpen");
    for (pstmt = pdbc->pstmtFirst; pstmt; pstmt = pstmt->pstmtNext)
    {
        if (pstmt->fStatus & STMT_OPEN)  /* if an opened result set */
        {
            if (fCatalog)
            {
                if (pstmt->fStatus & STMT_CATALOG && !(pstmt->fStatus & STMT_CLOSED))
                    return TRUE;
            }
            else
            {
                if (!(pstmt->fStatus & (STMT_CATALOG | STMT_CONST | STMT_CACHE_GET | STMT_CLOSED)))
                    return TRUE;
            }
        }
    }
    return FALSE;
}

/*
**  AnyIngresCursorOpen
**
**  Check if any Ingres API cursor is open on connection.
**  Used to determine if statement can be committed
**  (Ingres doesn't auto-commit until cursors are closed and
**  if the driver is simulating auto-commit then the driver
**  needs to be consistent with that behaviour).
**
**  On entry: pdbc     -->connection block;
**
**  Returns:  TRUE if an Ingres API cursor is open
**            FALSE if not
*/
BOOL AnyIngresCursorOpen (
    LPDBC   pdbc)
{
    LPSTMT  pstmt;

    TRACE_INTL(pdbc,"AnyIngresCursorOpen");
    for (pstmt = pdbc->pstmtFirst; pstmt; pstmt = pstmt->pstmtNext)
    {
         if (pstmt->fStatus & STMT_OPEN &&
             (pstmt->fStatus & STMT_API_CURSOR_OPENED ||
             pstmt->fStatus & STMT_INTERNAL))
             return(TRUE); 
    }
    return FALSE;
}


/*
**  PreparedStmt
**
**  Check if a PREPARED statement exists on the connection:
**
**  On entry: pdbc-->connection block;
**
**  Returns:  TRUE   if one exists that was not prepared by SQLExecDirect
**            FALSE  otherwise
*/
BOOL PreparedStmt (LPDBC pdbc)
{
    LPSTMT pstmt;

    TRACE_INTL(pdbc,"PreparedStmt");
    for (pstmt = pdbc->pstmtFirst; pstmt; pstmt = pstmt->pstmtNext)
        if ((pstmt->fStatus & (STMT_PREPARED | STMT_DIRECT)) == STMT_PREPARED)
            return TRUE;

    return FALSE;
}


/*
**  SetTransaction
**
**  Issue a SET TRANSACTION command:
**
**  On entry: pdbc-->connection block.
**
**  On exit:  Transaction access and or isolation is set.
**
**  Returns:  SQL_SUCCESS
**            SQL_SUCCESS_WITH_INFO
**            SQL_ERROR
*/

RETCODE SetTransaction (
    LPDBC pdbc,
    LPSESS psess)
{
    CHAR    sz[80];
    unsigned short fOptions;
    LPSQB   psqb = &pdbc->sqb;
    BOOL    fSession, fSet;
    BOOL    fCatalog   = (psess == &pdbc->sessCat);
    UDWORD  fIsolation = (fCatalog) ? SQL_TXN_READ_UNCOMMITTED : pdbc->fIsolation;
    RETCODE rc         = SQL_SUCCESS;

    TRACE_INTL (pdbc, "SetTransaction");
    /*
    **  Use the main session unless told otherwise:
    */
    if (psess == NULL)
        psess = &pdbc->sessMain;
    /*
    **  Issue command only if needed.
    **  If we issued a SET SESSION, remember it so we don't do it again.
    **  This does not start a transaction, but SET TRANSACTION does.
    **  (If not INGRES server, don't it.  DESKTOP gets 50005 gateway err)
    */
    if (!isServerClassINGRES(pdbc))
        goto SetTransactionAutocommitON;

    /*
    **  If a transaction is already started or a SET SESSION was issued
    **  with transaction options we're done.
    */
    if (psess->fStatus & (SESS_STARTED | SESS_SESSION_SET))
        goto SetTransactionAutocommitON;

    /*
    **  cannot issue the set command if within a transaction.
    */
    if (psess->tranHandle != NULL)
        return SQL_SUCCESS;

    /* set session is not standard as per dilma04 */
    if (pdbc->fDriver == DBC_INGRES && pdbc->fRelease >= fReleaseRELEASE20)
    {
        strcpy (sz, "SET SESSION ");
        fSession = TRUE;
        fSet     = TRUE;

        if (isDriverReadOnly(pdbc) && (pdbc->fOptions & 
                    OPT_ALLOW_DBPROC_UPDATE))
            strcat (sz, "READ WRITE ");
        else if (fCatalog || pdbc->fOptions & OPT_READONLY)
            strcat (sz, "READ ONLY ");
        else 
            strcat (sz, "READ WRITE ");

		if (fSet || fIsolation != pdbc->fIsolationDBMS)
		{
			switch (pdbc->fIsolation)
			{
			case SQL_TXN_READ_UNCOMMITTED:

				STcat (sz, ", ISOLATION LEVEL READ UNCOMMITTED");
				break;

			case SQL_TXN_READ_COMMITTED:

				STcat (sz, ", ISOLATION LEVEL READ COMMITTED");
				break;

			case SQL_TXN_REPEATABLE_READ:

				STcat (sz, ", ISOLATION LEVEL REPEATABLE READ");
				break;
		
			case SQL_TXN_SERIALIZABLE:

				STcat (sz, ", ISOLATION LEVEL SERIALIZABLE");
				break;

			}
		}
	} /* end if (pdbc->fDriver == DBC_INGRES && pdbc->fRelease >= fReleaseRELEASE20) */
	else
	{   /* release is < 2.0 */
		if (fIsolation != pdbc->fIsolationDBMS) 
		{
			fSession = TRUE;
            fSet     = TRUE;
	        if (fIsolation == SQL_TXN_READ_UNCOMMITTED)
			    STcopy("SET LOCKMODE SESSION WHERE READLOCK=NOLOCK", sz);
			else
   			    STcopy("SET LOCKMODE SESSION WHERE READLOCK=SHARED", sz);
		}
		else
			fSet = FALSE;
	}

    if (fSet)
    {
        LPSTMT      pstmt = psqb->pStmt;

        psqb->pSqlca  = &pdbc->sqlca;
        psqb->pSession= psess;
        fOptions = psqb->Options;
        psqb->Options = 0;
        if (!odbc_query( psess, pstmt, IIAPI_QT_QUERY, sz))
            rc = SQLCA_ERROR (pdbc, psess);
        else
            odbc_close(pstmt);
            
        if (rc == SQL_SUCCESS)
            psess->fStatus |= (fSession) ? SESS_SESSION_SET : SESS_STARTED;

        psess->fStatus &= ~SESS_SUSPENDED;
        SqlCommit(pdbc->psqb);
        if (pdbc->fOptions & OPT_AUTOCOMMIT  &&  psess->tranHandle == NULL)
           psqb->pSqlca->SQLCODE = odbc_AutoCommit(psqb,TRUE);
        psqb->pSession = &pdbc->sessMain;
        psqb->Options = fOptions;
        if (fCatalog)  /* if catalog connection, don't fall through and */ 
           return rc;  /* upset the main connection with autocommit yet */
    }

SetTransactionAutocommitON:
    /* set auto commit=on if necessary now that the user has had a 
       chance to set the transaction isolation */
    if (pdbc->fOptions & OPT_AUTOCOMMIT  &&  psess->tranHandle == NULL)
       psqb->pSqlca->SQLCODE = odbc_AutoCommit(psqb,TRUE);

    return rc;
}
