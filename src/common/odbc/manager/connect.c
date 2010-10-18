/*
/*
** Copyright (c) 2010 Ingres Corporation 
*/ 

#include <compat.h>
#include <gl.h>
#include <ci.h>
#include <cm.h>
#include <cs.h>
#include <dl.h>
#include <er.h>
#include <gc.h>
#include <lo.h>
#include <me.h>
#include <mh.h>
#include <mu.h>
#include <nm.h>
#include <pm.h>
#include <qu.h>
#include <st.h>
#include <tm.h>
#include <tr.h>
#include <sql.h>                    /* ODBC Core definitions */
#include <sqlext.h>                 /* ODBC extensions */
#include <iiapi.h>
#include <sqlstate.h>
#include <odbccfg.h>
#include <iiodbc.h>
#include <iiodbcfn.h>
#include <sqlca.h>
#include <idmseini.h>
#include "odbclocal.h"
#include "tracefn.h"

/* 
** Name: connect.c - Connection functions for the ODBC CLI
** 
** Description: 
** 		This file defines: 
** 
**              SQLAllocHandle - Allocate a handle.
**              SQLFreeHandle - Free a handle.
**              SQLAllocConnect - Allocate a connection handle.
**              SQLAllocEnv - Allocate an environment handle.
**              SQLConnect - Connect using an ODBC data source.
**              SQLDisconnect - Disconnect an ODBC client.
**              SQLDriverConnect - Connect using an ODBC connection string. 
**              SQLFreeConnect - Free connection resources.
**              SQLFreeEnv - Free environment resources.
**              SQLBrowseConnect - Discover data source connection attributes.
**              SQLGetPrivateProfileString - Get a configuration attribute.
** 
** History: 
**   14-jun-04 (loera01)
**     Created.
**   17-feb-05 (Ralph.Loen@ca.com)  SIR 113913
**     In IIodbc_freeEnv(), free DSN and driver queue if allocated.
**   09-jun-05 (loera01) Bug 114663
**     In SQLConnect(), invoke releaseLock() before calling insertErrorMsg().
**     In SQLDriverConnect(), add applyLock() before SQLGetProfileString() and
**     releaseLock() before calling insertErrorMsg().
**   15-Jul-2005 (hanje04)
**	Include iiodbcfn.h and tracefn.h which are the new home for the 
**	ODBC CLI function pointer definitions.
**   18-Jun-2006 (Ralph Loen) SIR 116260
**      Added IIDescribeParam() to the DLload() stack to support 
**      SQLDescribeParam().
**   19-June-2006 (Ralph Loen) SIR 115708
**      Added support for SQLExtendedFetch().
**   03-Jul-2006 (Ralph Loen) SIR 116326
**      Add support for SQLBrowseConnect().
**   07-Jul-2006 (Ralph Loen) SIR 116326
**      In SQLBrowseConnect(), make sure releaseLock() is invoked before 
**      returning.
**   14-Jul-2006 (Ralph Loen) SIR 116385
**      Add support for SQLTablePrivilegesW().
**   21-jul-06 (Ralph Loen)  SIR 116410
**      Add support for SQLColumnPrivilegesW().
**   10-Jan-2007 (Ralph Loen) Bug 117459 
**      In SQLGetPrivateProfileString(), check for non-null value of
**      "altPath" pointer instead of assuming it's referenced.
**   26-Feb-2007 (Ralph Loen) SIR 117786
**      Add support for ODBC connection pooling.
**   13-Mar-2007 (Ralph Loen) SIR 117786
**      Tightened treatment of connection states.  Added misplaced code for
**      non-Windows environments.  
**   16-May-2007 (Ralph Loen) SIR 117786
**      IIodbc_freePoolHandle() was made obsolete, since CLI handles now
**      swap driver handles if a match is found in the pool, instead of
**      freeing the current CLI connection handle.  Added IIodbc_encode()
**      and IIodbc_decode() to encrypt connection handles when they are
**      stored in the pool, and decrypted when fetched from the pool.
**   30-Jan-2008 (Ralph Loen) SIR 119723
**      Added support for SQLSetPos() in the driver.
**   17-June-2008 (Ralph Loen) Bug 120516
**      In IIodbc_allocEnv(), allocate the environment handle before the 
**      possibility of filling the error header of the environment handle 
**      with error information.
**   17-June-2008 (Ralph Loen) Bug 120506
**      Renamed DRV_STR_CAIPT_TR_FILE as DRV_STR_ALT_TRACE_FILE.
**   23-March-2009 (Ralph Loen) Bug 121838
**      Add support for SQLGetFunctions().
**   03-Sep-2010 (Ralph Loen) Bug 124348
**      Replaced SQLINTEGER, SQLUINTEGER and SQLPOINTER arguments with
**      SQLLEN, SQLULEN and SQLLEN * for compatibility with 64-bit
**      platforms.
**   08-Oct-2010 (Ralph Loen) Bug 124578
**      Supply a placeholder string for the output string argument in
**      SQLBrowseConnect() if the argument is NULL.
**   11-Oct-2010 (Ralph Loen) Bug 124578
**      Clean up treatment of placeholder variables outLen and outStr
**      in SQLBrowseConnect().
*/ 

#ifndef NT_GENERIC
char *getMgrInfo(bool *sysIniDefined);
#endif /* NT_GENERIC */

GLOBALDEF BOOL block_init = FALSE;
GLOBALDEF IIODBC_CB IIodbc_cb;
GLOBALDEF char cli_state[MAX_STATES][MAX_STRLEN] =
{
    "E0", "E1", "E2", "C0", "C1", "C2", "C3", "C4", "C5", "C6", "S0", "S1", "S2", "S3", "S4", 
    "S5", "S6", "S7", "S8", "S9", "S10", "S11", "S12", "D0", "D1" 
};

char *driver_syms[] =
{
    "SQLAllocConnect",
    "SQLAllocEnv",
    "SQLAllocStmt",
    "SQLBindCol",
    "SQLCancel",
    "SQLColAttribute",
    "SQLConnect",
    "SQLDescribeCol",
    "SQLDisconnect",
    "SQLError",
    "SQLExecDirect",
    "SQLExecute",
    "SQLExtendedFetch",
    "SQLFetch",
    "SQLFreeConnect",
    "SQLFreeEnv",
    "SQLFreeStmt",
    "SQLGetCursorName",
    "SQLNumResultCols",
    "SQLPrepare",
    "SQLRowCount",
    "SQLSetCursorName",
    "SQLSetParam",
    "SQLTransact",
    "SQLColAttributes",
    "SQLTablePrivileges",
    "SQLBindParam",
    "SQLColumns",
    "SQLDriverConnect",
    "SQLGetConnectOption",
    "SQLGetData",
    "SQLGetInfo",
    "SQLGetStmtOption",
    "SQLGetTypeInfo",
    "SQLParamData",
    "SQLPutData",
    "SQLSetConnectOption",
    "SQLSetStmtOption",
    "SQLSpecialColumns",
    "SQLStatistics",
    "SQLTables",
    "SQLBrowseConnect",
    "SQLColumnPrivileges",
    "SQLDescribeParam",
    "SQLForeignKeys",
    "SQLMoreResults",
    "SQLNativeSql",
    "SQLNumParams",
    "SQLParamOptions",
    "SQLPrimaryKeys",
    "SQLProcedureColumns",
    "SQLProcedures",
    "SQLSetPos",
    "SQLSetScrollOptions",
    "SQLBindParameter",
    "SQLAllocHandle",
    "SQLCloseCursor",
    "SQLCopyDesc",
    "SQLEndTran",
    "SQLFetchScroll",
    "SQLFreeHandle",
    "SQLGetConnectAttr",
    "SQLGetDescField",
    "SQLGetDescRec",
    "SQLGetDiagField",
    "SQLGetDiagRec",
    "SQLGetEnvAttr",
    "SQLGetStmtAttr",
    "SQLSetConnectAttr",
    "SQLSetDescField",
    "SQLSetDescRec",
    "SQLSetEnvAttr",
    "SQLSetStmtAttr",
    "SQLColAttributeW",
    "SQLColAttributesW",
    "SQLBrowseConnectW",
    "SQLConnectW",
    "SQLDescribeColW",
    "SQLErrorW",
    "SQLGetDiagRecW",
    "SQLGetDiagFieldW",
    "SQLExecDirectW",
    "SQLGetStmtAttrW",
    "SQLDriverConnectW",
    "SQLSetConnectAttrW",
    "SQLGetCursorNameW",
    "SQLPrepareW",
    "SQLSetStmtAttrW",
    "SQLSetCursorNameW",
    "SQLGetConnectOptionW",
    "SQLGetInfoW",
    "SQLColumnsW",
    "SQLGetTypeInfoW",
    "SQLSetConnectOptionW",
    "SQLSpecialColumnsW",
    "SQLStatisticsW",
    "SQLTablesW",
    "SQLForeignKeysW",
    "SQLNativeSqlW",
    "SQLPrimaryKeysW",
    "SQLProcedureColumnsW",
    "SQLProceduresW",
    "SQLSetDescFieldW",
    "SQLGetDescFieldW",
    "SQLGetDescRecW",
    "SQLGetConnectAttrW",
    "SQLTablePrivilegesW",
    "SQLColumnPrivilegesW",
    "SQLSetPos",
    "SQLGetFunctions",
    NULL
};

i2 cbDriver_syms = (sizeof(driver_syms) / sizeof(driver_syms[0])) - 1;
i2 dsnToken(char *dsnStr);

/*
** The following strings have no counterpart in the ODBC configuration API,
** but are valid strings for SQLDriverConnect().
*/
#define UID_STR "uid"
#define PWD_STR "pwd"
#define DB_STR "db"
#define DATASOURCENAME_STR "datasourcename"
#define LOGONID_STR "logonid"
#define SERVERNAME "servername"
#define SRVR_STR "srvr"

/*
** The following definitions are used for encryption and decription.
*/
#define CRYPT_SIZE              8
#define CRYPT_DELIM             '*'
#define GCS_MAX_STR             128
#define GCS_MAX_BUF             255
#define ENC_KEY_LEN     (CRYPT_SIZE * 2)
static  char    *cset =
"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";


VOID CIsetkey(PTR key_str, CI_KS KS);
BOOL IIodbc_insertPoolElement( pDBC pdbc, const i2 pool_type);

/*
** Name: 	SQLAllocHandle - Allocate a handle.
** 
** Description: 
**              SQLAllocHandle allocates an environment, connection, 
**              statement, or descriptor handle.
**
** Inputs:
**              HandleType - Handle type (env, dbc, stmt or desc).
**              InputHandle - Root handle.
**
** Outputs: 
**              OutputHandle - Allocated handle.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_SUCCESS_WITH_INFO
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

SQLRETURN  SQL_API SQLAllocHandle (
    SQLSMALLINT HandleType,
    SQLHANDLE   InputHandle,
    SQLHANDLE   *OutputHandle)
{
    RETCODE rc, traceRet = 1;
    pENV penv;
    pDBC pdbc;
    pSTMT pstmt;

    switch (HandleType)
    {
    case SQL_HANDLE_ENV:

        rc = IIodbc_allocEnv((pENV *)OutputHandle);
         
        ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLAllocHandle(HandleType, 
            InputHandle, OutputHandle), traceRet);

        if (rc == SQL_SUCCESS && IIAllocHandle)
        {
            penv = (pENV)*OutputHandle;  

            rc = IIAllocHandle(
                SQL_HANDLE_ENV,
                NULL,
                &penv->hdr.driverHandle);
            
            applyLock(SQL_HANDLE_ENV, penv);
            if (rc != SQL_SUCCESS)
            {
                penv->hdr.driverError = TRUE;
                penv->errHdr.rc = rc;
            }
            else
                penv->hdr.state = E1;
            releaseLock(SQL_HANDLE_ENV, penv);
         }   
         break;

    case SQL_HANDLE_DBC:

         ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLAllocHandle(HandleType, 
             InputHandle, OutputHandle),traceRet);

         rc = IIodbc_allocDbc((pENV)InputHandle, (pDBC *)OutputHandle);

        if (rc == SQL_SUCCESS && IIAllocHandle)
        {
            pdbc = (pDBC)*OutputHandle;  
            penv = (pENV)InputHandle;
    
            rc = IIAllocHandle(
                SQL_HANDLE_DBC,
                penv->hdr.driverHandle,
                &pdbc->hdr.driverHandle);
    
            applyLock(SQL_HANDLE_DBC, pdbc);
            if (rc != SQL_SUCCESS)
            {
                pdbc->hdr.driverError = TRUE;
                pdbc->errHdr.rc = rc;
            } 
            else 
            {
                pdbc->hdr.state = C2;
                applyLock(SQL_HANDLE_ENV, penv);
                penv->hdr.state = E2;
                releaseLock(SQL_HANDLE_ENV, penv);
            }
            releaseLock(SQL_HANDLE_DBC, pdbc);
        }   
        break;

    case SQL_HANDLE_STMT:
        ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLAllocHandle(HandleType, 
            InputHandle, OutputHandle), traceRet);

        rc = IIodbc_allocStmt((pDBC)InputHandle, 
            (pSTMT *)OutputHandle);

        if (rc == SQL_SUCCESS)
        {
            pstmt = (pSTMT)*OutputHandle;  
            pdbc = (pDBC)InputHandle;

            rc = IIAllocHandle(
                SQL_HANDLE_STMT,
                pdbc->hdr.driverHandle,
                &pstmt->hdr.driverHandle);

            applyLock(SQL_HANDLE_STMT, pstmt);
            if (rc != SQL_SUCCESS)
            {
                pstmt->hdr.driverError = TRUE;
                pstmt->errHdr.rc = rc;
            }
            else
                pstmt->hdr.state = S1;
            releaseLock(SQL_HANDLE_STMT, pstmt);	
        }   
        break;

    case SQL_HANDLE_DESC:

        ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLAllocHandle(HandleType, 
            InputHandle, OutputHandle), traceRet);

        rc = IIodbc_allocDesc((pDBC)InputHandle, 
            (SQLHDESC *)OutputHandle);
        break;
    }
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLFreeHandle - Free a handle.
** 
** Description: 
**              SQLFreeHandle frees resouces associated with an
**              environment, connection, statement, or descriptor handle.
**
** Inputs:
**              HandleType - Handle type (env, dbc, stmt or desc).
**              Handle - Handle to be freed.
**
** Outputs: 
**              None.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

SQLRETURN  SQL_API SQLFreeHandle (
    SQLSMALLINT HandleType,
    SQLHANDLE   Handle)
{
    RETCODE rc = SQL_SUCCESS;
    pENV penv = NULL;
    pDBC pdbc = NULL;
    pSTMT pstmt = NULL;
    pDESC pdesc = NULL;
    QUEUE *q;
    IIODBC_HDR *hdr;
    IIODBC_ERROR_HDR *errHdr;
    RETCODE traceRet = 1;
    QUEUE *pool;
    SQLINTEGER *count;
    
    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLFreeHandle(HandleType, Handle),
       traceRet);

    if (!Handle)
    {
        return SQL_INVALID_HANDLE;
    }

    if (HandleType != SQL_HANDLE_DESC)
    {
        if (validHandle(Handle, HandleType) != SQL_SUCCESS)
        {
            rc = SQL_INVALID_HANDLE;
            goto routine_exit;
        }
        resetErrorBuff(Handle, HandleType);
    }
    else
    {
        pdesc = getDescHandle(Handle);
        if (pdesc)
        {
            if (validHandle(pdesc, HandleType) != SQL_SUCCESS)
            {
                 return SQL_INVALID_HANDLE;
            }
            resetErrorBuff(pdesc, HandleType);
        }
    }

    switch (HandleType)
    {
        case SQL_HANDLE_ENV:  
        penv = (pENV)Handle;
        hdr = &penv->hdr;
        errHdr = &penv->errHdr;
        if (penv->pooling == DRIVER_POOL)
        {
            pool = &IIodbc_cb.pool_q;
            count = &IIodbc_cb.pool_count;
        }
        else if (penv->pooling == HENV_POOL)
        {
            pool = &penv->pool_q;
            count = &penv->pool_count;
        }
        
        /*
        ** Disconnect and free any pooled connections.
        */        
        if (penv->pooling == HENV_POOL || ( penv->pooling == DRIVER_POOL &&
            IIodbc_cb.env_count == 0))
        {
            for (q = pool->q_prev; q != pool; q = pool->q_prev)
            {
                pdbc = (pDBC)((pPOOL)q)->pdbc;
                rc = IIDisconnect(pdbc->hdr.driverHandle);
                rc = IIFreeConnect(pdbc->hdr.driverHandle);
                if (penv->pooling == HENV_POOL)
                    applyLock(SQL_HANDLE_ENV, penv);
                else
                    applyLock(SQL_HANDLE_IIODBC_CB, NULL);
                QUremove(q);
                *count -= 1;
                if (penv->pooling == HENV_POOL)
                    releaseLock(SQL_HANDLE_ENV, penv);
                else
                    releaseLock(SQL_HANDLE_IIODBC_CB, NULL);
                MEfree((PTR)q);
                ODBC_EXEC_TRACE(ODBC_EX_TRACE)
                    ("SQLFreeHandle: pool count is %d\n",*count);
            } /* for (q = pool->q_prev; q != pool; q = pool->q_prev) */
        } /* if (penv->pooling == HENV_POOL || ( penv->pooling == DRIVER_POOL &&
            IIodbc_cb.env_count == 0)) */

        if (penv->pooling == HENV_POOL || !penv->pooling)
        {
            rc = IIFreeHandle(SQL_HANDLE_ENV, penv->hdr.driverHandle);
            applyLock(SQL_HANDLE_ENV, penv);
            if (SQL_SUCCEEDED (rc) && penv->hdr.state == E1)
                penv->hdr.state = E0;
            else if (rc != SQL_SUCCESS)
            {
                hdr->driverError = TRUE;
                errHdr->rc = rc;
            }
            releaseLock(SQL_HANDLE_ENV, penv);
        }
        
        if (penv->pooling == DRIVER_POOL)
        {
            applyLock(SQL_HANDLE_IIODBC_CB, NULL);
            IIodbc_cb.env_count--;
            releaseLock(SQL_HANDLE_IIODBC_CB, NULL);
            if (IIodbc_cb.env_count == 0)
                IIodbc_freeEnvHandles();
            goto routine_exit;
        }
        break;

    case SQL_HANDLE_DBC:
        pdbc = (pDBC)Handle;
        penv = (pENV)pdbc->penv;
        hdr = &pdbc->hdr;
        errHdr = &pdbc->errHdr;
        if (pdbc->hdr.driverHandle)
        {
            rc = IIFreeHandle(
                SQL_HANDLE_DBC,
                pdbc->hdr.driverHandle);
            
            applyLock(SQL_HANDLE_DBC, pdbc);
            if (SQL_SUCCEEDED (rc))
                pdbc->hdr.state = C1;
            else if (rc != SQL_SUCCESS)
            {
                hdr->driverError = TRUE;
                errHdr->rc = rc;
            }
            releaseLock(SQL_HANDLE_DBC, pdbc);
        }
        break;

    case SQL_HANDLE_STMT:
        pstmt = (pSTMT)Handle;
        pdbc = pstmt->pdbc;
        hdr = &pstmt->hdr;
        errHdr = &pstmt->errHdr;
        if (pstmt->hdr.driverHandle)
        {
            rc = IIFreeHandle(
                SQL_HANDLE_STMT,
                pstmt->hdr.driverHandle);
        
            applyLock(HandleType, Handle);
            if (rc != SQL_SUCCESS)
            {
                hdr->driverError = TRUE;
                errHdr->rc = rc;
            }
            else
            {
                applyLock(SQL_HANDLE_DBC, pdbc);
                if (!pdbc->stmt_count && pdbc->hdr.state != C6)
                    pdbc->hdr.state = C4;
                releaseLock(SQL_HANDLE_DBC, pdbc);
            }
            releaseLock(SQL_HANDLE_STMT, pstmt);
        }
        break;

    case SQL_HANDLE_DESC:
        rc = IIFreeHandle(
            HandleType,
            Handle);

        if (pdesc && rc != SQL_SUCCESS)
        {
            applyLock(SQL_HANDLE_DESC, pdesc);
            pdesc->hdr.driverError = TRUE;
            pdesc->errHdr.rc = rc; 
            releaseLock(SQL_HANDLE_DESC, pdesc);	
        }
        break;
    }

    if (!SQL_SUCCEEDED(rc))
        goto routine_exit;
    else
    {
        if (!IIodbc_freeHandle(HandleType, (void *)Handle))
            rc = SQL_INVALID_HANDLE;
        else
            rc = SQL_SUCCESS;
    }
    
routine_exit:
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLAllocConnect - Allocate a connection handle.
** 
** Description: 
**              See above.
**
** Inputs:
**              henv - Environment handle.
**
** Outputs: 
**              hdbc - Connection handle.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

RETCODE SQL_API SQLAllocConnect(
    SQLHENV    henv,
    SQLHDBC  * phdbc)
{   
    RETCODE rc, traceRet = 1;
    pENV penv = henv;
    pDBC pdbc;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLAllocConnect(penv, phdbc),
        traceRet);

    rc = IIodbc_allocDbc(penv, (pDBC *)phdbc); 

    if (rc == SQL_SUCCESS && IIAllocConnect)
    {
        pdbc = *phdbc;

        rc = IIAllocConnect(
            penv->hdr.driverHandle,
            &pdbc->hdr.driverHandle);

        applyLock(SQL_HANDLE_DBC, pdbc);
        applyLock(SQL_HANDLE_ENV, penv);
        if (rc != SQL_SUCCESS)
        {
            penv->hdr.driverError = TRUE;
            penv->errHdr.rc = rc;
        }
        else
        {
            pdbc->hdr.state = C2;
            penv->hdr.state = E2;
        }
        releaseLock(SQL_HANDLE_ENV, penv);
        releaseLock(SQL_HANDLE_DBC, pdbc);
    }

    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLAllocEnv - Allocate an environment handle.
** 
** Description: 
**              See above.
**
** Inputs:
**              None.
**
** Outputs: 
**              henv - Envionment handle.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 


RETCODE SQL_API SQLAllocEnv(
    SQLHENV     * phenv)
{
    RETCODE rc, traceRet = 1;
    pENV penv;

    rc = IIodbc_allocEnv((pENV *)phenv);

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLAllocEnv(phenv), traceRet);
  
    if (rc == SQL_SUCCESS && IIAllocEnv)
    {
        penv = *phenv;

        rc = IIAllocEnv(
            &penv->hdr.driverHandle);

        applyLock(SQL_HANDLE_ENV, penv);
        if (rc != SQL_SUCCESS)
        {
            penv->hdr.driverError = TRUE;
            penv->errHdr.rc = rc;
        }
        else
            penv->hdr.state = E1;
        releaseLock(SQL_HANDLE_ENV, penv);
    }
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLConnect - Connect to database.
** 
** Description: 
**              SQLConnect connects to the database via a pre-defined
**              data source definition.  If the driver is not loaded,
**              the ODBC CLI searches for, and loads, the driver library,
**              and maps the underlying ODBC functions.
**
** Inputs:
**              hdbc  - Connection handle.
**              szDSN - Data source.
**              cbDSN - Length of data source string.
**              szUID - User name.
**              cbUID - Length of user name string.
**              szPWD - Password.
**              cbPWD - Length of password string.
**
** Outputs: 
**              None.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_SUCCESS_WITH_INFO
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
**              If SQLSetConnectAttr() SQLSetConnectOption(), SQLSetEnvAttr(),
**              or SQLSetEnvOption() were called prior, and the driver was
**              not loaded, the ODBC CLI invokes these functions retroactively.
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
**    26-Feb-07 (Ralph Loen) SIR 117786
**      If connection pooling is enabled, search for a matching connection
**      in the pool before invoking the driver version of SQLConnect().
*/ 

RETCODE SQL_API SQLConnect(
    SQLHDBC     hdbc,
    UCHAR     * szDSN,
    SWORD       cbDSN,
    UCHAR     * szUID,
    SWORD       cbUID,
    UCHAR     * szPWD,
    SWORD       cbPWD)
{
    RETCODE rc = SQL_SUCCESS, traceRet = 1, ret;
    pDBC pdbc = (pDBC)hdbc, pdbcMatch=NULL;
    pENV penv = pdbc->penv;
    char database[OCFG_MAX_STRLEN];
    char opwd[OCFG_MAX_STRLEN*2];
    STATUS status;
    pDBC handle;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLConnect(hdbc, szDSN, cbDSN, 
        szUID, cbUID, szPWD, cbPWD), traceRet);

    if (!hdbc)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    if (pdbc->hdr.type != SQL_HANDLE_DBC)
    {
        rc = SQL_INVALID_HANDLE;
        goto routine_exit;
    }

    ODBC_EXEC_TRACE(ODBC_EX_TRACE)
        ("SQLConnect entry: state is %s\n", cli_state[pdbc->hdr.state]);
    
    resetErrorBuff(hdbc, SQL_HANDLE_DBC);

    applyLock(SQL_HANDLE_DBC, pdbc);
    if (!SQLGetPrivateProfileString ((char *)szDSN, KEY_DATABASE, "", database,
        sizeof(database),ODBC_INI))
    {
        releaseLock(SQL_HANDLE_DBC, pdbc);
        insertErrorMsg(pdbc, SQL_HANDLE_DBC, SQL_IM002,
            SQL_ERROR, NULL, 0);
        rc = SQL_ERROR;
        goto routine_exit;
    }

    /*
    ** If pooling is enabled, store the SQLConnect() arguments if non-empty.
    */
    if (penv->pooling)
    {
        if ((cbDSN > 0 || cbDSN == SQL_NTS) && szDSN)
            pdbc->dsn = STalloc((char *)szDSN);
        if ((cbUID > 0 || cbUID == SQL_NTS) && szUID)
            pdbc->uid = STalloc((char *)szUID);
        if ((cbPWD > 0 || cbPWD == SQL_NTS) && szPWD)
        {
            status = IIodbc_encode((char *)szPWD, IIodbc_cb.hostname, opwd);
            pdbc->pwd = STalloc(opwd);
        }
    }
    releaseLock(SQL_HANDLE_DBC, pdbc);

    /*
    ** Load the driver if not currently loaded.
    */
    if (!IIConnect)
    {
        rc = IIodbc_connectPhase1(pdbc);
        if (rc == SQL_SUCCESS)
            rc = IIodbc_connectPhase2(pdbc);
        if (!SQL_SUCCEEDED(rc))
            goto routine_exit;
    }
    /*
    **  If pooling is enabled, see if the connection pool has a matching
    **  connection handle.  If so, use the pooled handle instead of the
    **  allocated handle.
    */
    else if (penv->pooling == HENV_POOL)
    {
        ret = IIodbc_fetchPoolElement ( pdbc, HENV_POOL, &pdbcMatch );
        if (pdbc == pdbcMatch)
        {
            ODBC_EXEC_TRACE(ODBC_EX_TRACE)
                ("SQLConnect: not calling driver, duplicate conn handle %p\n",
                    pdbc);
            goto routine_exit;
        }
        else if (ret == TRUE)
        {
            applyLock(SQL_HANDLE_DBC, pdbc);
            handle = pdbc->hdr.driverHandle;
            pdbc->hdr.driverHandle = pdbcMatch->hdr.driverHandle;
            pdbcMatch->hdr.driverHandle = handle;
            pdbc->hdr.state = C4;
            pdbcMatch->hdr.state = C2;
            releaseLock(SQL_HANDLE_DBC, pdbc);
            goto routine_exit;
        }
    }    
    else if (penv->pooling == DRIVER_POOL)
    {
        ret = IIodbc_fetchPoolElement ( pdbc, DRIVER_POOL, &pdbcMatch );
        if (pdbc == pdbcMatch)
        {
            ODBC_EXEC_TRACE(ODBC_EX_TRACE)
                ("SQLConnect: not calling driver, duplicate conn handle %p\n",
                    pdbc);
            goto routine_exit;
        }
        else if (ret == TRUE)
        {
            ODBC_EXEC_TRACE(ODBC_EX_TRACE)
                ("%p uses handle %p from the pool\n",pdbc,
                pdbcMatch->hdr.driverHandle);
            applyLock(SQL_HANDLE_DBC, pdbc);
            handle = pdbc->hdr.driverHandle;
            pdbc->hdr.driverHandle = pdbcMatch->hdr.driverHandle;
            pdbcMatch->hdr.driverHandle = handle;
            pdbc->hdr.state = C4;
            pdbcMatch->hdr.state = C2;
            releaseLock(SQL_HANDLE_DBC, pdbc);
            goto routine_exit;
        }
    }    
    
    rc = IIConnect(
        pdbc->hdr.driverHandle,
        szDSN,
        cbDSN,
        szUID,
        cbUID,
        szPWD,
        cbPWD);

    applyLock(SQL_HANDLE_DBC, pdbc);
    if (rc != SQL_SUCCESS)
    {
        if (rc != SQL_SUCCESS_WITH_INFO)
            pdbc->hdr.state = C2;
        pdbc->hdr.driverError = TRUE;
        pdbc->errHdr.rc = rc;
    }
    else
    {
        pdbc->hdr.state = C4;
    }
    releaseLock(SQL_HANDLE_DBC, pdbc);
        
routine_exit:
    ODBC_EXEC_TRACE(ODBC_EX_TRACE)
        ("SQLConnect exit: state is %s\n", cli_state[pdbc->hdr.state]);
 
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLDisconnect - Disconnect from the database.
** 
** Description: 
**              See above.
**
** Inputs:
**              hdbc - Connection handle.
**
** Outputs: 
**              None.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_SUCCESS_WITH_INFO
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
**   26-Feb-1007 (Ralph Loen) SIR 117786
**      If connection pooling is enabled, create an IIODBC_POOL object
**      and insert into the ODBC connection pool instead of disconnecting.
*/ 

RETCODE SQL_API SQLDisconnect(
    SQLHDBC hdbc)
{
    pDBC pdbc = (pDBC)hdbc;
    pENV penv = pdbc->penv;
    RETCODE rc = SQL_SUCCESS, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLDisconnect(hdbc), traceRet);
    if (validHandle(hdbc, SQL_HANDLE_DBC) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        rc = SQL_INVALID_HANDLE;
        goto exit_routine;
    }

	ODBC_EXEC_TRACE(ODBC_EX_TRACE) ("SQLDisconnect entry: state is %s\n", 
            cli_state[pdbc->hdr.state]);
    resetErrorBuff(hdbc, SQL_HANDLE_DBC);

    /*
    **  If connection pooling is enabled, don't disconnect here.  Instead,
    **  insert the connection handle into the connection pool.
    **  Dead (previously failed) connections are not entered into the
    **  pool.
    */
    if (penv->pooling == HENV_POOL && pdbc->hdr.state > C3)
    {
        if (pdbc->hdr.state == C6)
        {
            insertErrorMsg(hdbc, SQL_HANDLE_DBC, SQL_25000, SQL_ERROR, NULL, 0);
            rc = SQL_ERROR;
            goto exit_routine;
        }
        ODBC_EXEC_TRACE(ODBC_EX_TRACE)
           ("Inserting handle %p in the pool\n",pdbc);

        IIodbc_insertPoolElement(pdbc, HENV_POOL);
    }
    else if (penv->pooling == DRIVER_POOL && pdbc->hdr.state > C3)
    {
        if (pdbc->hdr.state == C6)
        {
            insertErrorMsg(hdbc, SQL_HANDLE_DBC, SQL_25000, SQL_ERROR, NULL, 0);
            rc = SQL_ERROR;
            goto exit_routine;
        }
        ODBC_EXEC_TRACE(ODBC_EX_TRACE)
            ("Inserting handle %p in the pool\n",pdbc);

        IIodbc_insertPoolElement(pdbc, DRIVER_POOL);
    }
    else if (!penv->pooling)
    {
        rc = IIDisconnect(pdbc->hdr.driverHandle);
        applyLock(SQL_HANDLE_DBC, pdbc);
        if (rc != SQL_SUCCESS)
        {
            pdbc->hdr.driverError = TRUE;
            pdbc->errHdr.rc = rc;
        }
        else if (SQL_SUCCEEDED(rc) )
        {
            QUEUE *q;
            pDESC pdesc;
            pSTMT pstmt;

            if (pdbc->hdr.state != C6) /* if transaction not in progress */
                pdbc->hdr.state = C2;
            /*
            ** IIodbc_freeHandle() removes the statement handle from pdbc.
            */
            for (q = pdbc->stmt_q.q_prev;
                q != &pdbc->stmt_q; q = pdbc->stmt_q.q_prev)
            {
               pstmt = (pSTMT)q;
               IIodbc_freeHandle(SQL_HANDLE_STMT, (void *)pstmt);
            }
            /*
            ** IIodbc_freeHandle() removes the desc handle from IIodbc_cb.
            */
            for (q = IIodbc_cb.desc_q.q_prev; q != &IIodbc_cb.desc_q; 
                q = IIodbc_cb.desc_q.q_prev)
            {
                pdesc = (pDESC)q;
                if (pdesc->pdbc == pdbc)
                {
                    IIodbc_freeHandle(SQL_HANDLE_DESC, (void *)pdesc);
                    IIodbc_freeHandle(SQL_HANDLE_DESC, (void*)pdesc);
                }
            }
            releaseLock(SQL_HANDLE_DBC, pdbc);
        } /* if (rc != SQL_SUCCESS) */
    } /* if (penv->pooling == HENV_POOL && pdbc->hdr.state > C3 */

exit_routine:
    ODBC_EXEC_TRACE(ODBC_EX_TRACE)
        ("SQLDisconnect exit: state is %s\n", cli_state[pdbc->hdr.state]);
 
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLDriverConnect - Connect to database.
** 
** Description: 
**              SQLDriverConnect connects to the database via a connection
**              string.  If the driver is not loaded, the ODBC CLI searches 
**              for, and loads, the driver library, and maps the underlying 
**              ODBC functions.
**
** Inputs:
**              hdbc - Connection handle.
**              hwnd - Window handle or NULL.
**              szConnStrIn - Connection string.
**              cbConnStrIn - Length of connection string.
**              cbConnStrOutMax - Max length of output (modified) conn string.
**              fDriverCompletion - Whether to prompt for more info (Windows).
**
** Outputs: 
**              szConnStrOut - Actual connection string passed to the dbms.
**              pcbConnStrOut - Returned length of connection string.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_SUCCESS_WITH_INFO
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
**   26-Feb-1007 (Ralph Loen) SIR 117786
**      If connection pooling is enabled, store the connection string
**      attributes in the connection handle.  Check for a matching
**      connection handle and use the pool handle for connecting if
**      a match is found.
*/ 


RETCODE SQL_API SQLDriverConnect(
    SQLHDBC     hdbc,
    SQLHWND     hwnd,
    UCHAR     * szConnStrIn,
    SWORD       cbConnStrIn,
    UCHAR     * szConnStrOut,
    SWORD       cbConnStrOutMax,
    SWORD     * pcbConnStrOut,
    UWORD       fDriverCompletion)
{
    RETCODE rc = SQL_SUCCESS, ret, traceRet = 1;
    pDBC pdbc = (pDBC)hdbc, pdbcMatch=NULL;
    pENV penv = pdbc->penv;
    char *p = NULL, *term = NULL;
    char pWork[800];
    SQLINTEGER cb;
    pDBC handle;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLDriverConnect(hdbc, hwnd, 
        szConnStrIn, cbConnStrIn, szConnStrOut, cbConnStrOutMax, 
        pcbConnStrOut, fDriverCompletion), traceRet);

    if (!hdbc)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    if (pdbc->hdr.type != SQL_HANDLE_DBC)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    ODBC_EXEC_TRACE(ODBC_EX_TRACE) ("SQLDriverConnect entry: state is %s\n",
        cli_state[pdbc->hdr.state]);
    
    resetErrorBuff(hdbc, SQL_HANDLE_DBC);

    if (cbConnStrIn > 0)
        cb = cbConnStrIn;
    else if (cbConnStrIn == SQL_NTS)
        cb = (SQLINTEGER)STlength((char*)szConnStrIn);
    
    STlcopy((char *)szConnStrIn, pWork, cb);

    if (penv->pooling)
    {
        pdbc->driverConnect = TRUE;
        IIodbc_formatConnHandle(pWork, pdbc);
    }
            
    if (!IIDriverConnect)
    {
        rc = IIodbc_connectPhase1(pdbc);
        if (rc == SQL_SUCCESS)
        {
            applyLock(SQL_HANDLE_DBC, pdbc);
            releaseLock(SQL_HANDLE_DBC, pdbc);
            rc = IIodbc_connectPhase2(pdbc);
        }
        if (!SQL_SUCCEEDED(rc))
            goto routine_exit;
    }
    
    /*
    **  If pooling is enabled, see if the connection pool has a matching
    **  connection handle.  If so, use the pooled handle instead of the
    **  allocated handle.
    */
    else if (penv->pooling == HENV_POOL)
    {
        ret = IIodbc_fetchPoolElement ( pdbc, HENV_POOL, &pdbcMatch );
        if (pdbc == pdbcMatch)
        {
            ODBC_EXEC_TRACE(ODBC_EX_TRACE)
                ("SQLDriverConnect: not calling driver, duplicate conn handle %p\n",
                    pdbc);
            goto routine_exit;
        }
        else if (ret == TRUE)
        {
            applyLock(SQL_HANDLE_DBC, pdbc);
            handle = pdbc->hdr.driverHandle;
            pdbc->hdr.driverHandle = pdbcMatch->hdr.driverHandle;
            pdbcMatch->hdr.driverHandle = handle;
            pdbc->hdr.state = C4;
            pdbcMatch->hdr.state = C2;
            releaseLock(SQL_HANDLE_DBC, pdbc);
            goto routine_exit;
        }
    }    
    else if (penv->pooling == DRIVER_POOL)
    {
        ret = IIodbc_fetchPoolElement ( pdbc, DRIVER_POOL, &pdbcMatch );
        if (pdbc == pdbcMatch)
        {
            ODBC_EXEC_TRACE(ODBC_EX_TRACE)
                ("SQLDriverConnect: not calling driver, duplicate conn handle %p\n",
                    pdbc);
            goto routine_exit;
        }
        else if (ret == TRUE)
        {
            applyLock(SQL_HANDLE_DBC, pdbc);
            handle = pdbc->hdr.driverHandle;
            pdbc->hdr.driverHandle = pdbcMatch->hdr.driverHandle;
            pdbcMatch->hdr.driverHandle = handle;
            pdbc->hdr.state = C4;
            pdbcMatch->hdr.state = C2;
            releaseLock(SQL_HANDLE_DBC, pdbc);
            goto routine_exit;
        }
    }    

    rc = IIDriverConnect(
        pdbc->hdr.driverHandle,
        hwnd,
        szConnStrIn,
        cbConnStrIn,
        szConnStrOut,
        cbConnStrOutMax,
        pcbConnStrOut,
        fDriverCompletion);
        
    applyLock(SQL_HANDLE_DBC, pdbc);
    if (rc != SQL_SUCCESS)
    {
       if (rc != SQL_SUCCESS_WITH_INFO)
           pdbc->hdr.state = C2;
        pdbc->hdr.driverError = TRUE;
        pdbc->errHdr.rc = rc;
    }
    else 
       pdbc->hdr.state = C4;
    releaseLock(SQL_HANDLE_DBC, pdbc);
    
routine_exit:
    ODBC_EXEC_TRACE(ODBC_EX_TRACE) ("SQLDriverConnect exit: state is %s\n",
        cli_state[pdbc->hdr.state]);
    
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLFreeConnect - Free a connection handle.
** 
** Description: 
**              See above.
**
** Inputs:
**              hdbc - Connection handle.
**
** Outputs: 
**              None.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
**   26-Feb-1007 (Ralph Loen) SIR 117786
**      Do not free the connection handle if connection pooling is enabled.
*/ 

RETCODE SQL_API SQLFreeConnect(
    SQLHDBC  hdbc)
{
    pDBC pdbc = (pDBC)hdbc;
    pENV penv = (pENV)pdbc->penv;
    RETCODE rc, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLFreeConnect(hdbc), traceRet);

    if (!pdbc)
    {
        rc = SQL_INVALID_HANDLE;
        goto routine_exit;
    }

    if (pdbc->hdr.type != SQL_HANDLE_DBC)
    {
        rc = SQL_INVALID_HANDLE;
        goto routine_exit;
    }
    
    resetErrorBuff(hdbc, SQL_HANDLE_DBC);

    if ( pdbc->hdr.driverHandle)
    {
        rc = IIFreeConnect(pdbc->hdr.driverHandle);
        if (rc != SQL_SUCCESS)
        {
            applyLock(SQL_HANDLE_DBC, pdbc);
            pdbc->hdr.driverError = TRUE;
            pdbc->errHdr.rc = rc;
            releaseLock(SQL_HANDLE_DBC, pdbc);
            goto routine_exit; 
        }
        if (SQL_SUCCEEDED (rc))
        {
            applyLock(SQL_HANDLE_DBC, pdbc);
            pdbc->hdr.state = C1;
            releaseLock(SQL_HANDLE_DBC, pdbc);
        }
    }

    IIodbc_freeHandle( SQL_HANDLE_DBC, (void *)pdbc); 

routine_exit:
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLFreeEnv - Free an environment handle.
** 
** Description: 
**              See above.
**
** Inputs:
**              henv - Environment handle.
**
** Outputs: 
**              None.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
**   26-Feb-1007 (Ralph Loen) SIR 117786
**      Do not free the environment handle if driver pooling is 
**      enabled and the environment handle count is greater than 1.
*/ 

RETCODE SQL_API SQLFreeEnv(
    SQLHENV henv)
{
    pENV penv = (pENV)henv;
    pDBC pdbc;
    RETCODE rc, traceRet = 1;
    QUEUE *q, *pool;
    SQLINTEGER *count;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE,IITraceSQLFreeEnv(henv), traceRet);

    if (!penv)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    if (penv->hdr.type != SQL_HANDLE_ENV)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(henv, SQL_HANDLE_ENV);

    if (penv->pooling == DRIVER_POOL)
    {
        pool = &IIodbc_cb.pool_q;
        count = &IIodbc_cb.pool_count;
    }
    else if (penv->pooling == HENV_POOL)
    {
        pool = &penv->pool_q;
        count = &penv->pool_count;
    }
    
    if (penv->hdr.driverHandle)
    {
        /*
        ** Disconnect and free any pooled connections.
        */
        if (penv->pooling && IIodbc_cb.env_count == 0)
        {
            for (q = pool->q_prev; q != pool; q = pool->q_prev)  
            {
                pdbc = (pDBC)((pPOOL)q)->pdbc;
                rc = IIDisconnect(pdbc->hdr.driverHandle);
                rc = IIFreeConnect(pdbc->hdr.driverHandle);
                QUremove(q);
                MEfree((PTR)q);
                *count -= 1;
                ODBC_EXEC_TRACE(ODBC_EX_TRACE)
                    ("SQLFreeHandle: pool count is %d\n",*count);
            }
        }
        if (penv->pooling == HENV_POOL || !penv->pooling)
        {    
            rc = IIFreeEnv(penv->hdr.driverHandle);
            applyLock(SQL_HANDLE_ENV, penv);
            if (rc != SQL_SUCCESS)
            {
                penv->hdr.driverError = TRUE;
                penv->errHdr.rc = rc;
                return rc;
            }
            else if (SQL_SUCCEEDED (rc) && penv->hdr.state == E1)
                penv->hdr.state = E0;
            releaseLock(SQL_HANDLE_ENV, penv);
        }
        if (penv->pooling == DRIVER_POOL)
        {
            applyLock(SQL_HANDLE_IIODBC_CB, NULL);
            IIodbc_cb.env_count--;
            if (IIodbc_cb.env_count == 0)
                IIodbc_freeEnvHandles();
            releaseLock(SQL_HANDLE_IIODBC_CB, NULL);
            return SQL_SUCCESS;
        }
    }

    IIodbc_freeHandle(SQL_HANDLE_ENV,(void *)penv);
 
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));

    ODBC_TERMTRACE();

    return SQL_SUCCESS;
}

/*
** Name: 	SQLBrowseConnect - Perform connection attribute discovery
** 
** Description: 
**              Discover connection attributes based on input connection 
**              string.
**
** Inputs:
**              hdbc - Connection handle.
**              szConnStrIn - Attribute string.
**              cbConnStrIn - Length of attribute string.
**              cbConnStrOutMax - Maximum length of returned attribute string.
**
** Outputs: 
**              szConnStrOut - Returned attribute.
**              pcbConnStrOut - Length of returned attribute string.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_SUCCESS_WITH_INFO
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
**    08-Oct-2010 (Ralph Loen) Bug 124578
**      Supply a placeholder string for the output string argument
**      if the argument is NULL; this allows SQLBrowseConnect() to
**      return SQL_NEED_DATA on first invocation and attempt a
**      connection on the second invocation.
**    11-Oct-2010 (Ralph Loen) Bug 124578
**      Clean up treatment of placeholder variables outLen and outStr
**      in SQLBrowseConnectW().
*/ 

SQLRETURN SQL_API SQLBrowseConnect(
    SQLHDBC            hdbc,
    SQLCHAR            *szConnStrIn,
    SQLSMALLINT        cbConnStrIn,
    SQLCHAR            *szConnStrOut,
    SQLSMALLINT        cbConnStrOutMax,
    SQLSMALLINT       *pcbConnStrOut)
{
    SQLCHAR *outStr;
    SQLSMALLINT outLen = MAX_CONNSTR_OUTLEN;
    BOOL allocStr = FALSE;
    pDBC pdbc = (pDBC)hdbc;
    RETCODE rc = SQL_SUCCESS, traceRet = 1;
    

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLBrowseConnect(hdbc, szConnStrIn, 
         cbConnStrIn, szConnStrOut, cbConnStrOutMax, pcbConnStrOut),
         traceRet);

    if (!hdbc)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    if (pdbc->hdr.type != SQL_HANDLE_DBC)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    /*
    ** If the application supplied no output string, supply a placeholder.
    */
    if (!szConnStrOut)
    {
        outStr = (SQLCHAR *) MEreqmem(0, outLen, TRUE, NULL);
        allocStr = TRUE;
    }
    else
    {
        outStr = szConnStrOut;
        outLen = cbConnStrOutMax;
    }

    resetErrorBuff(hdbc, SQL_HANDLE_DBC);

    if (!IIBrowseConnect)
    {
        rc = IIodbc_connectPhase1(pdbc);
    }
    if (rc == SQL_SUCCESS)
    {
        rc = IIBrowseConnect(pdbc->hdr.driverHandle,
            szConnStrIn,
            cbConnStrIn,
            outStr,
            outLen,
            pcbConnStrOut);

    }
    applyLock(SQL_HANDLE_DBC, pdbc);
    if (rc != SQL_SUCCESS && rc != SQL_NEED_DATA)
    {
        pdbc->hdr.driverError = TRUE;
        pdbc->errHdr.rc = rc;
    }
    else if (SQL_SUCCEEDED(rc))
        pdbc->hdr.state = C4;

    releaseLock(SQL_HANDLE_DBC, pdbc);

    if (allocStr)
    {
        MEfree((PTR)outStr);
        szConnStrOut = NULL;
    }

    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));

    return rc;
}

/*
** Name: 	SQLGetPrivateProfileString - Get configuration info
** 
** Description: 
**              SQLGetPrivateProfileString gets a list of names of values 
**              or data corresponding to a value of the system information.
** Inputs:
**              szSection - Configuration file section to be searched.
**              szEntry - Section key.
**              szDefault - Default value.
**              cbBuffer - Size of return buffer.
**              szFile - Configuration file (registry) name.
** Outputs: 
**              szBuffer - Return buffer.
** Returns: 
**              Number of characters read.
**
** Exceptions: 
**              N/A 
** 
** Side Effects: 
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 
#ifndef NT_GENERIC

SQLINTEGER SQL_API SQLGetPrivateProfileString(
    const char *szSection,
    const char *szEntry,
    const char *szDefault,
    char *szBuffer,
    SQLLEN cbBuffer,
    const char *szFile)
{
    i4 i,j,count = 0;
    STATUS status = OK;
    ATTR_NAME **attrName;
    bool sysIniDefined = FALSE;
    char *altPath = getMgrInfo(&sysIniDefined);
    PTR dsnH;
    PTR drvH = IIodbc_cb.drvConfigHandle;

    bool systemPath = FALSE;
    bool traceOn = FALSE;
    char tracePath[OCFG_MAX_STRLEN];

    /*
    ** If odbcinst.ini (system path), first see if tracing is requested.
    ** Otherwise retrieve attribute for other types of entries.
    */
    if ( !STbcompare(ODBCINST_INI, 0, szFile, 0, TRUE ) )
    {
        systemPath = TRUE;
        if (!STbcompare(DRV_STR_ODBC, 0, szSection, 0, TRUE) )
        {
            traceOn = getTrace(drvH, tracePath, &status); 
            if (!STbcompare(DRV_STR_TRACE, 0, szEntry, 0, TRUE) )
            {
                if (traceOn)
                    STcopy("Y", szBuffer);
                else
                    STcopy("N", szBuffer);
                return 1;
            }
            if (!STbcompare(DRV_STR_TRACE_FILE, 0, szEntry, 0, TRUE) )
            {
                if (traceOn)
                    STcopy(tracePath, szBuffer);
                else
                    STcopy("\0", szBuffer);
                return STlength(tracePath);
            }
            else if (!STbcompare(DRV_STR_ALT_TRACE_FILE, 0, szEntry, 0, TRUE) )
            {
                if (traceOn)
                    STcopy(tracePath, szBuffer);
                else
                    STcopy("\0", szBuffer);
                return STlength(tracePath);
            }
        }
    } 
    /*
    **  Try the user path, then either the alt or system path.
    */
    openConfig(drvH, OCFG_USR_PATH, NULL, &dsnH, &status);

    if (status == OK)
    {
        count = getEntryAttribute( dsnH, (char *)szSection, 0, NULL, &status );

        if (status != OK || !count)
        {
            closeConfig(dsnH, &status);
        }
    }

    if (!count)
    {
        if (altPath && !sysIniDefined)
            openConfig(drvH, OCFG_ALT_PATH, altPath, &dsnH, &status);
        else
            openConfig(drvH, OCFG_SYS_PATH, NULL, &dsnH, &status);

        if (status != OK)
            return 0;

        count = getEntryAttribute( dsnH, (char *)szSection, 0, NULL, &status );
        if (status != OK)
        {
            closeConfig(dsnH, &status);
            return 0;
        }
    }

    if (count)
    {
        attrName = (ATTR_NAME **)MEreqmem( (u_i2) 0, 
            (u_i4)(count * sizeof(ATTR_NAME *)), TRUE, (STATUS *) NULL);
        for (j = 0; j < count; j++)
        {
            attrName[j] = (ATTR_NAME *)MEreqmem( (u_i2) 0, 
                (u_i4)(sizeof(ATTR_NAME)), TRUE, (STATUS *) NULL);
            attrName[j]->name = (char *) MEreqmem( (u_i2) 0, 
                (u_i4)OCFG_MAX_STRLEN, TRUE, (STATUS *) NULL);
            attrName[j]->value = (char *) MEreqmem( (u_i2) 0, 
                (u_i4)OCFG_MAX_STRLEN, TRUE, (STATUS *) NULL);
        }
        getEntryAttribute( dsnH, (char *)szSection, count, attrName, 
            &status );
        if (status != OK)
        {
            for (i = 0; i < count; i++)
            {
                MEfree((PTR)attrName[i]->name);
                MEfree((PTR)attrName[i]->value);
                MEfree((PTR)attrName[i]);
            }
            MEfree((PTR)attrName);
            closeConfig(dsnH, &status);
            return 0;
        }
        for (j = 0; j < count; j++)
        {
            if ( !STbcompare(attrName[j]->name, 0, szEntry, 0, TRUE ) )
            {
                STcopy(attrName[j]->value, szBuffer);
                for (i = 0; i < count; i++)
                {
                    MEfree((PTR)attrName[i]->name);
                    MEfree((PTR)attrName[i]->value);
                    MEfree((PTR)attrName[i]);
                }
                MEfree((PTR)attrName);
                closeConfig(dsnH, &status);
                return (STlength(szBuffer));
            }
        }
    }
    for (i = 0; i < count; i++)
    {
        MEfree((PTR)attrName[i]->name);
        MEfree((PTR)attrName[i]->value);
        MEfree((PTR)attrName[i]);
    }
    if (count)
        MEfree((PTR)attrName);
    closeConfig(dsnH, &status);

    return 0;
}
#else

SQLINTEGER SQL_API SQLGetPrivateProfileString(
    const char *szSection,
    const char *szEntry,
    const char *szDefault,
    char *szBuffer,
    SQLLEN cbBuffer,
    const char *szFile)
{
    i4 lRet;
    HKEY hKey;
    i4 dwBufLen=256;
    char *userFormat = "Software\\ODBC\\ODBC.INI\\%s";
    char *systemFormat = "SOFTWARE\\ODBC\\ODBC.INI\\%s";
    char keyStr[256];

    /* 
    **  Try the user-level first, then system.
    */
    STprintf(keyStr, userFormat, szSection);
    lRet = RegOpenKeyEx( HKEY_CURRENT_USER, keyStr,
        0, KEY_QUERY_VALUE, &hKey );
   
    if( lRet != ERROR_SUCCESS )
    {
        STprintf(keyStr, systemFormat, szSection);
        lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
            keyStr, 0, KEY_QUERY_VALUE, &hKey);
    }
    if (lRet != ERROR_SUCCESS)
        return 0;

    lRet = RegQueryValueEx( hKey, szEntry,
       NULL, NULL, (LPBYTE) szBuffer, &dwBufLen);
    RegCloseKey( hKey );
    if (lRet == ERROR_SUCCESS)
        return ((SQLINTEGER)STlength(szBuffer));
    return 0;
}
#endif /* ifndef NT_GENERIC */

/*
** Name: 	IIodbc_connectPhase2 - Second phase of database connection.
** 
** Description: 
**              IIodbc_connectPhase2() reads the CLI cache for requests to
**              set connection and environment options prior to loading the
**              driver.  It executes the requests as they are found in the
**              cache--not necessarily in the order in which they were invoked
**              in the calling application.
**
** Inputs:
**              pdbc - Connection handle.
**
** Outputs: 
**              None.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 
RETCODE IIodbc_connectPhase2( pDBC pdbc )
{
    pENV penv = pdbc->penv; 
    QUEUE *q;

    RETCODE rc = SQL_SUCCESS;
    IISETENVATTR_PARAM *iiSetEnvAttrParam;
    IISETCONNECTOPTION_PARAM *iiSetConnectOptionParam;
    IISETCONNECTOPTIONW_PARAM *iiSetConnectOptionWParam;
    IISETCONNECTATTR_PARAM *iiSetConnectAttrParam;
    IISETCONNECTATTRW_PARAM *iiSetConnectAttrWParam;
    if (penv->hdr.driverHandle)
    {
        if (penv->setEnvAttr_count)
        {
            /*
            ** SQLSetEnvAttr was called before loading the driver.
            ** Execute now with saved parameters.
            */
            for (q = penv->setEnvAttr_q.q_prev; 
                q != &penv->setEnvAttr_q, penv->setEnvAttr_count; 
                q = penv->setEnvAttr_q.q_prev)
            {
                iiSetEnvAttrParam = (IISETENVATTR_PARAM *)q; 
                iiSetEnvAttrParam->EnvironmentHandle = 
                    (SQLHENV)penv->hdr.driverHandle;
                
                rc = IISetEnvAttr (
                    iiSetEnvAttrParam->EnvironmentHandle,
                    iiSetEnvAttrParam->Attribute,
                    iiSetEnvAttrParam->ValuePtr,
                    iiSetEnvAttrParam->StringLength);

                QUremove((QUEUE *)iiSetEnvAttrParam);
                MEfree((PTR)iiSetEnvAttrParam);
                penv->setEnvAttr_count--;
                if (rc != SQL_SUCCESS)
                {
                    penv->hdr.driverError = TRUE;
                    penv->errHdr.rc = rc;
                    return rc;
                }
            }
        }
        if (pdbc->hdr.driverHandle)
        {
            if (pdbc->setConnectAttr_count)
            {
                /*
                ** SQLSetConnectAttr was called before loading the driver.
                ** Execute now with saved parameters.
                */
                for (q = pdbc->setConnectAttr_q.q_prev; 
                    q != &pdbc->setConnectAttr_q; q = pdbc->setConnectAttr_q.q_prev)
                {
                    iiSetConnectAttrParam = (IISETCONNECTATTR_PARAM *)q; 
                    iiSetConnectAttrParam->ConnectionHandle = 
                        (SQLHDBC)pdbc->hdr.driverHandle;
                  
                    rc = IISetConnectAttr (
                        iiSetConnectAttrParam->ConnectionHandle,
                        iiSetConnectAttrParam->Attribute,
                        iiSetConnectAttrParam->Value,
                        iiSetConnectAttrParam->StringLength);
    
                    QUremove((QUEUE *)iiSetConnectAttrParam);
                    pdbc->setConnectAttr_count--;
                    if (rc != SQL_SUCCESS)
                    {
                        pdbc->hdr.driverError = TRUE;
                        pdbc->errHdr.rc = rc;
                    }
                    MEfree((PTR)iiSetConnectAttrParam); 
                    if (!pdbc->setConnectAttr_count)
                        break;
                }
            }
            if (pdbc->setConnectAttrW_count)
            {
                /*
                ** SQLSetConnectAttrW was called before loading the driver.
                ** Execute now with saved parameters.
                */
                for (q = pdbc->setConnectAttrW_q.q_prev; 
                    q != &pdbc->setConnectAttrW_q; q = pdbc->setConnectAttrW_q.q_prev)
                {
                    iiSetConnectAttrWParam = (IISETCONNECTATTRW_PARAM *)q; 
                    iiSetConnectAttrWParam->hdbc = 
                        (SQLHDBC)pdbc->hdr.driverHandle;
                  
                    rc = IISetConnectAttrW (
                        iiSetConnectAttrWParam->hdbc,
                        iiSetConnectAttrWParam->fAttribute,
                        iiSetConnectAttrWParam->rgbValue,
                        iiSetConnectAttrWParam->cbValue);
    
                    QUremove((QUEUE *)iiSetConnectAttrWParam);
                    pdbc->setConnectAttrW_count--;
                    if (rc != SQL_SUCCESS)
                    {
                        pdbc->hdr.driverError = TRUE;
                        pdbc->errHdr.rc = rc;
                    }

                    MEfree((PTR)iiSetConnectAttrWParam); 
                    if (!pdbc->setConnectAttrW_count)
                        break;
                }
            }

            if (pdbc->setConnectOption_count)
            {
                /*
                ** SQLSetConnectOption was called before loading the driver.
                ** Execute now with saved parameters.
                */
                 for (q = pdbc->setConnectOption_q.q_prev; 
                    q != &pdbc->setConnectOption_q; q = pdbc->setConnectOption_q.q_prev)
                {
                    iiSetConnectOptionParam = (IISETCONNECTOPTION_PARAM *)q; 
                    iiSetConnectOptionParam->ConnectionHandle = 
                        (SQLHDBC)pdbc->hdr.driverHandle;
                  
                    rc = IISetConnectOption (
                        iiSetConnectOptionParam->ConnectionHandle,
                        iiSetConnectOptionParam->Option,
                        iiSetConnectOptionParam->Value);
    
                    QUremove((QUEUE *)iiSetConnectOptionParam);
                    pdbc->setConnectOption_count--;
                    if (rc != SQL_SUCCESS)
                    {
                        pdbc->hdr.driverError = TRUE;
                        pdbc->errHdr.rc = rc;
                    }
                    MEfree((PTR)iiSetConnectOptionParam);
                    if (!pdbc->setConnectOption_count)
                        break;
                }
            }

            if (pdbc->setConnectOptionW_count)
            {
                /*
                ** SQLSetConnectOptionW was called before loading the driver.
                ** Execute now with saved parameters.
                */
                for (q = pdbc->setConnectOptionW_q.q_prev; 
                    q != &pdbc->setConnectOptionW_q; q = pdbc->setConnectOptionW_q.q_prev)
                {
                    iiSetConnectOptionWParam = (IISETCONNECTOPTIONW_PARAM *)q; 
                    iiSetConnectOptionWParam->ConnectionHandle = 
                        (SQLHDBC)pdbc->hdr.driverHandle;
                  
                    rc = IISetConnectOptionW (
                        iiSetConnectOptionWParam->ConnectionHandle,
                        iiSetConnectOptionWParam->Option,
                        iiSetConnectOptionWParam->Value);
    
                    QUremove((QUEUE *)iiSetConnectOptionWParam);
                    pdbc->setConnectOptionW_count--;
                    if (rc != SQL_SUCCESS)
                    {
                        pdbc->hdr.driverError = TRUE;
                        pdbc->errHdr.rc = rc;
                    }
    
                    MEfree((PTR)iiSetConnectOptionWParam);
                    if (!pdbc->setConnectOptionW_count)
                        break;
                }
            }
        }
    }

    if (SQL_SUCCEEDED(rc))
        pdbc->hdr.state = C3;

    return rc;
}

/*
** Name: 	bind_sym - Bind (map) ODBC functions to the CLI.
** 
** Description: 
**              bind_sym() binds a driver function entry point to a
**              callable address in the CLI.
**
** Inputs:
**              dlHandle - Load handle obtained from DLload().
**              driverSym - Driver function name.
**
** Outputs: 
**              fcn - Process address of the driver function.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_SUCCESS_WITH_INFO
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 
void bind_sym(PTR *dlHandle, char *driverSym, IIODBC_DRIVER_FN *fcn)
{
    CL_ERR_DESC err;
    STATUS status = OK;
    IIODBC_DRIVER_FN boundFn;    

    status = DLbind(*dlHandle, driverSym, 
        (PTR *)&boundFn, &err);
    if (status == OK)
        *fcn = boundFn;
    else
        *fcn = NULL;
}

/*
** Name: 	IIodbc_freeHandle - Free a handle (internal)
** 
** Description: 
**              IIodbc_freeHandle() frees the CLI resources associated with
**              the provided handle.
**
** Inputs:
**              HandleType - Handle type: env, dbc, stmt, or desc.
**              handle - The CLI handle.
**
** Outputs: 
**              None.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

BOOL IIodbc_freeHandle (SQLSMALLINT HandleType, void * handle)
{
    pENV penv = NULL;
    pDBC pdbc = NULL;
    pSTMT pstmt = NULL;
    pDESC pdesc = NULL;
    STATUS status = OK;
    BOOL ret = TRUE;
    QUEUE *pool_q,*q;
    i4 *count;
    pDBC pool_pdbc;

    ODBC_EXEC_TRACE(ODBC_EX_TRACE)
        ("IIodbc_freeHandle entry with handle type %d and handle %p\n",
            HandleType, handle);
    if (!handle)
    {
        ret = TRUE;
        goto exit_routine;
    }

    switch (HandleType)
    {
    case SQL_HANDLE_ENV:
        penv = (pENV)handle;
        /*
        ** Environment handles are not freed with driver pooling unless
        ** the environment handle count is 1.  In that case, driver_pooling
        ** is set to FALSE before calling IIodbc_freeHandle().
        */
        applyLock(SQL_HANDLE_IIODBC_CB, NULL);
        if (penv->pooling == DRIVER_POOL)
        {
            ret = SQL_SUCCESS;
            IIodbc_cb.env_count--;
            releaseLock(SQL_HANDLE_IIODBC_CB, NULL);
	        goto exit_routine;
        }
#ifndef NT_GENERIC
        if (!IIodbc_cb.env_count)
        {
            closeConfig(IIodbc_cb.drvConfigHandle, &status);
            IIodbc_cb.drvConfigHandle = 0;
        }
        if (penv->usrDsnCount)
            MEfree((PTR)penv->usrDsnList);
        if (penv->sysDsnCount)
            MEfree((PTR)penv->sysDsnList);
#endif
        QUremove((QUEUE *)penv);
        
        if (penv->pooling == HENV_POOL)
            IIodbc_cb.env_count--;

        releaseLock(SQL_HANDLE_IIODBC_CB, NULL);

        deleteLock(SQL_HANDLE_ENV, penv);
        if (MEfree((PTR)penv) != OK)
        {
            ret = FALSE;
            goto exit_routine;
        }
        break;

    case SQL_HANDLE_DBC:
        pdbc = (pDBC)handle;
        penv = pdbc->penv;

        if (penv->pooling == DRIVER_POOL)
        {
            pool_q = &IIodbc_cb.pool_q;
            count = &IIodbc_cb.pool_count;
        }
        else if (penv->pooling == HENV_POOL)
        {
            pool_q = &penv->pool_q;
            count = &penv->pool_count;
        }

        applyLock(SQL_HANDLE_ENV, penv);
        penv->dbc_count--;
        QUremove((QUEUE *)pdbc);
        releaseLock(SQL_HANDLE_ENV, penv);

        /*
        **  If pooling is enabled, and the connection handle is in the pool, 
        **  remove it.
        */
        if (penv->pooling == HENV_POOL || penv->pooling == DRIVER_POOL)
        {
            applyLock(SQL_HANDLE_ENV, pdbc);
            for( q = pool_q->q_prev; q != pool_q; q = q->q_prev)
            {
                pool_pdbc = (pDBC)((pPOOL)q)->pdbc;
                if (pdbc != pool_pdbc)
                    continue;
                ODBC_EXEC_TRACE(ODBC_EX_TRACE) ("IIodbc_freeHandle removes pdbc %p from the pool\n", pdbc);
                QUremove(q);
                MEfree((PTR)q);
                *count -= 1; 
                break;
            }

            releaseLock(SQL_HANDLE_ENV, pdbc);
        } /* if (count) */
        deleteLock(SQL_HANDLE_DBC, pdbc);
        if (MEfree((PTR)pdbc) != OK)
        {
            ret = FALSE;
            goto exit_routine;
        }
        break;

    case SQL_HANDLE_STMT:
        pstmt = (pSTMT)handle;
        applyLock(SQL_HANDLE_DBC,pdbc);
        pdbc = pstmt->pdbc;
        pdbc->stmt_count--;
        QUremove((QUEUE *)pstmt);
        releaseLock(SQL_HANDLE_DBC, pdbc);
        deleteLock(SQL_HANDLE_STMT, pstmt);
        if (MEfree((PTR)pstmt) != OK)
        {
            ret = FALSE;
            goto exit_routine;
        }
        break;

    case SQL_HANDLE_DESC:
        pdesc = getDescHandle(handle);
        if (pdesc)
        {
            applyLock(SQL_HANDLE_IIODBC_CB, NULL);
            IIodbc_cb.desc_count--;
            QUremove((QUEUE *)pdesc);
            releaseLock(SQL_HANDLE_IIODBC_CB, NULL);

            deleteLock(SQL_HANDLE_DESC, pdesc);
            if (MEfree((PTR)pdesc) != OK)
            {
                ret = FALSE;
                goto exit_routine;
            }
        }
        break;

    default:
        ret = FALSE;
        break;
    }

exit_routine:
    ODBC_EXEC_TRACE(ODBC_EX_TRACE) ("IIodbc_freeHandle exit\n");
    return ret;
}

/*
** Name: 	IIodbc_allocEnv - Allocate a environment handle (internal).
** 
** Description: 
**              IIodbc_allocEnv() allocates the environment handle used by
**              the CLI and is the handle presented to ODBC applications.
**
** Inputs:
**              None.
**
** Outputs: 
**              ppenv - CLI environment handle.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
**   26-Feb-1007 (Ralph Loen) SIR 117786
**      If connection pooling is enabled, initialize the pool queue
**      and set the corresponding flags in the environment handle.
**   17-June-2008 (Ralph Loen) Bug 120516
**      Allocate the environment handle before the possibility of filling the 
**      error header of the environment handle with error information.
**       
*/ 


RETCODE IIodbc_allocEnv(pENV *ppenv)
{
    RETCODE rc = SQL_SUCCESS;
    pENV penv = NULL;

#ifndef NT_GENERIC
    bool sysIniDefined = FALSE;
    char altPath[OCFG_MAX_STRLEN]="\0";
    char *tmpPath = getMgrInfo(&sysIniDefined);
#endif /* NT_GENERIC */
    STATUS status = OK;

    if (!ppenv)
        return SQL_INVALID_HANDLE;

    if (!block_init)
    {
        IIodbc_initControlBlock();
        block_init = TRUE;
    }
    penv = (pENV)MEreqmem(0, sizeof(IIODBC_ENV), TRUE, NULL);
    *ppenv = penv;
    
 #ifndef NT_GENERIC
    if (!IIodbc_cb.env_count)
    {
        if (tmpPath && *tmpPath)
            STcopy(tmpPath, altPath);

        if (*altPath && !sysIniDefined)
        {
            openConfig(NULL, OCFG_ALT_PATH, altPath, 
                (PTR *)&IIodbc_cb.drvConfigHandle, &status); 
        }
        else
        {
            openConfig(NULL, OCFG_SYS_PATH, NULL, 
                (PTR *)&IIodbc_cb.drvConfigHandle, &status);
        }

        if (status != OK)
        {
            insertErrorMsg(penv, SQL_HANDLE_ENV, SQL_HY000,
                SQL_SUCCESS_WITH_INFO, 
                "Error opening driver manager configuration file", 0);
        } 
    }
#endif /* NT_GENERIC */ 
    
    applyLock(SQL_HANDLE_IIODBC_CB, NULL);
    QUinsert((QUEUE *)penv, &IIodbc_cb.env_q);
    releaseLock(SQL_HANDLE_IIODBC_CB, NULL);
    if ( !initLock(SQL_HANDLE_ENV, penv))
    {
        insertErrorMsg(penv, SQL_HANDLE_ENV, SQL_HY000,
            SQL_ERROR, "Error initializing environment semaphore", 0);
        return SQL_ERROR;
    }
    applyLock(SQL_HANDLE_ENV, penv);
    QUinit(&penv->dbc_q);
    QUinit(&penv->setEnvAttr_q);
    penv->hdr.type = SQL_HANDLE_ENV;
    penv->hdr.state = E0;
    penv->dbc_count = 0;
    penv->errHdr.rc = SQL_SUCCESS;
    if (IIodbc_cb.pooling == HENV_POOL)
    {
        ODBC_EXEC_TRACE(ODBC_TR_TRACE)("IIodbc_allocEnv: henv pool initialized\n");
        penv->pooling = HENV_POOL;
        QUinit(&penv->pool_q);
    }
    else if (IIodbc_cb.pooling == DRIVER_POOL)
        penv->pooling = DRIVER_POOL;
	releaseLock(SQL_HANDLE_ENV, penv);
    applyLock(SQL_HANDLE_IIODBC_CB, NULL);
    IIodbc_cb.env_count++;  
    releaseLock(SQL_HANDLE_IIODBC_CB, NULL);

    return rc;
}

/*
** Name: 	IIodbc_allocDbc - Allocate a connection handle (internal).
** 
** Description: 
**              IIodbc_allocDbc() allocates the connection handle used by
**              the CLI and is the handle presented to ODBC applications.
**
** Inputs:
**              penv - CLI environment handle.
**
** Outputs: 
**              ppdbc - CLI connection handle.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

RETCODE IIodbc_allocDbc(pENV penv, pDBC *ppdbc)
{
    RETCODE rc = SQL_SUCCESS;
    pDBC pdbc = NULL;

    if (!penv)
        return SQL_INVALID_HANDLE;
    
    if (penv->hdr.type != SQL_HANDLE_ENV)
        return SQL_INVALID_HANDLE;

    resetErrorBuff(penv, SQL_HANDLE_ENV);

    if (!ppdbc)
    {
        insertErrorMsg(penv, SQL_HANDLE_ENV, SQL_HY009,
            SQL_ERROR, NULL, 0);
        return SQL_ERROR;
    }

    pdbc = (pDBC)MEreqmem(0, sizeof(IIODBC_DBC), TRUE, NULL);
    pdbc->penv = penv;
    *ppdbc = pdbc;
    penv->dbc_count++;

    pdbc->hdr.type = SQL_HANDLE_DBC;
    pdbc->hdr.state = C1;
    pdbc->stmt_count = 0;
    pdbc->autocommit = TRUE;
    if (!initLock( SQL_HANDLE_DBC, pdbc))
    {
        insertErrorMsg(pdbc, SQL_HANDLE_DBC, SQL_HY000,
            SQL_ERROR, "Error initializing connection semaphore", 0);
        return SQL_ERROR;
    }
    pdbc->errHdr.rc = SQL_SUCCESS;

    QUinsert((QUEUE *)pdbc, &penv->dbc_q);

    QUinit(&pdbc->setConnectAttr_q);
    QUinit(&pdbc->setConnectAttr_q);
    QUinit(&pdbc->setConnectOption_q);
    QUinit(&pdbc->getConnectAttr_q);
    QUinit(&pdbc->getConnectOption_q);
    QUinit(&pdbc->stmt_q);
    return rc;
}

/*
** Name: 	IIodbc_allocStmt - Allocate a statement handle (internal).
** 
** Description: 
**              IIodbc_allocStmt() allocates the statement handle used by
**              the CLI and is the handle presented to ODBC applications.
**
** Inputs:
**              pdbc - CLI connection handle.
**
** Outputs: 
**              ppstmt - CLI statement handle.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

RETCODE IIodbc_allocStmt(pDBC pdbc, pSTMT *ppstmt)
{
    pSTMT pstmt = NULL;
    RETCODE rc = SQL_SUCCESS;
    SQLHDESC hdesc = NULL;

    if (validHandle(pdbc, SQL_HANDLE_DBC) != SQL_SUCCESS) 
        return SQL_INVALID_HANDLE;

    resetErrorBuff(pdbc, SQL_HANDLE_DBC);

    if (!ppstmt)
    {
        insertErrorMsg(pdbc, SQL_HANDLE_DBC, SQL_HY009,
            SQL_ERROR, NULL, 0);
        return SQL_ERROR;
    }
    pstmt = (pSTMT)MEreqmem(0, sizeof(IIODBC_STMT), TRUE, NULL);
    pstmt->pdbc = pdbc;
    *ppstmt = pstmt;
    pdbc->stmt_count++;

    pstmt->hdr.type = SQL_HANDLE_STMT;
    pstmt->hdr.state = S1;
    if (!initLock( SQL_HANDLE_STMT, pstmt))
    {
        insertErrorMsg(pstmt, SQL_HANDLE_STMT, SQL_HY000,
            SQL_ERROR, "Error initializing statement semaphore", 0);
        return SQL_ERROR;
    }
    pstmt->errHdr.rc = SQL_SUCCESS;
 
    QUinsert((QUEUE *)pstmt, &pdbc->stmt_q);

    return rc;
}

/*
** Name: 	IIodbc_allocDesc - Allocate a descriptor handle (internal).
** 
** Description: 
**              IIodbc_allocDesc() allocates the descriptor handle used by
**              the CLI.  Most applications, however, use the descriptor
**              handle provided by the ODBC driver.
**
** Inputs:
**              pdbc - CLI connection handle.
**
** Outputs: 
**              ppdesc - CLI descriptor handle.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions: 
**              N/A 
** 
** Side Effects: 
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

RETCODE IIodbc_allocDesc(pDBC pdbc, SQLHDESC *ppdesc)
{
    RETCODE rc = SQL_SUCCESS;
    pDESC pdesc;

    if (validHandle(pdbc, SQL_HANDLE_DBC) != SQL_SUCCESS)
        return SQL_INVALID_HANDLE;

	resetErrorBuff(pdbc, SQL_HANDLE_DBC);

    if (!ppdesc)
    {
        insertErrorMsg(pdbc, SQL_HANDLE_DBC, SQL_HY009,
            SQL_ERROR, NULL, 0);
        return SQL_ERROR;
    }

    rc = IIAllocHandle(SQL_HANDLE_DESC,
        pdbc->hdr.driverHandle, ppdesc);

    if (rc == SQL_SUCCESS)
    {
        pdesc = (pDESC)MEreqmem(0, sizeof(IIODBC_DESC), TRUE, NULL);
        pdesc->pdbc = pdbc;
        pdesc->hdr.type = SQL_HANDLE_DESC;
        pdesc->hdr.state = D1;
        pdesc->hdr.driverHandle = *ppdesc;
        IIodbc_cb.desc_count++;
        QUinsert((QUEUE *)pdesc, &IIodbc_cb.desc_q);
        if (!initLock( SQL_HANDLE_DESC, pdesc))
        {
            insertErrorMsg(pdbc, SQL_HANDLE_DESC, SQL_HY000,
                SQL_ERROR, "Error initializing descriptor semaphore", 0);
            return SQL_ERROR;
        }
        pdesc->errHdr.rc = SQL_SUCCESS;
    }

    return rc;
}

/*
** Name: 	IIodbc_connectPhase1 - First phase of database connection.
** 
** Description: 
**              This is one of the most important functions of the CLI.  
**              IIodbc_connectPhase1() loads the driver library and
**              maps the underlying ODBC functions.  If multiple connection
**              handle allocations were requested prior to the load, they
**              are allocated at this time.
**
** Inputs:
**              pdbc - Connection handle.
**
** Outputs: 
**              None.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

RETCODE IIodbc_connectPhase1(pDBC pdbc)
{
    PTR dlHandle;
    LOCATION    loc, tmploc;
    CL_ERR_DESC clerr;
    STATUS      status;
    char locbuf[MAX_LOC];
    pENV penv;
    RETCODE rc, traceRet = 1;
    char *driverFile;
    char *winFile = "CAIIOD35";

#ifndef NT_GENERIC
    if ((status = NMloc(LIB, PATH, NULL, &tmploc)) != OK)
#else
    if ((status = NMloc(BIN, PATH, NULL, &tmploc)) != OK)
#endif
    {
        SETCLERR(&clerr, status, 0);
        return(SQL_ERROR);
    }
    LOcopy(&tmploc, locbuf, &loc);
    
    /*
    **  At this point, "loc" points to the location where DL modules
    **  reside.
    */
#ifndef NT_GENERIC
    PMinit();
    status = PMload( NULL, NULL );
    if (status != OK)
    {
        insertErrorMsg(pdbc, SQL_HANDLE_DBC, SQL_IM003,
            SQL_ERROR, NULL, status);
        return( SQL_ERROR );
    }

    PMsetDefault( 0, SystemCfgPrefix );
    PMsetDefault( 1, PMhost() );
    PMsetDefault( 2, ERx("odbc") );
    status = PMget( ERx("!.module_name"), &driverFile );
    if (status != OK)
    {
        insertErrorMsg(pdbc, SQL_HANDLE_DBC, SQL_IM003,
            SQL_ERROR, NULL, status);
        return( SQL_ERROR );
    }
#else
    driverFile = winFile;
#endif /* NT_GENERIC */

    status = DLload( NULL, driverFile, driver_syms, 
                &dlHandle, &clerr );

    if ( status != OK )
    {
        insertErrorMsg(pdbc, SQL_HANDLE_DBC, SQL_IM003,
            SQL_ERROR, NULL, status);
        return SQL_ERROR;
    }

    bind_sym( &dlHandle, "SQLAllocEnv", &IIAllocEnv);
    bind_sym( &dlHandle, "SQLAllocConnect", &IIAllocConnect );
    bind_sym( &dlHandle, "SQLAllocEnv", &IIAllocEnv );
    bind_sym( &dlHandle, "SQLAllocStmt", &IIAllocStmt );
    bind_sym( &dlHandle, "SQLBindCol", &IIBindCol );
    bind_sym( &dlHandle, "SQLCancel", &IICancel );
    bind_sym( &dlHandle, "SQLColAttribute", &IIColAttribute );
    bind_sym( &dlHandle, "SQLConnect", &IIConnect );
    bind_sym( &dlHandle, "SQLDescribeCol", &IIDescribeCol );
    bind_sym( &dlHandle, "SQLDisconnect", &IIDisconnect );
    bind_sym( &dlHandle, "SQLError", &IIError );
    bind_sym( &dlHandle, "SQLExecDirect", &IIExecDirect );
    bind_sym( &dlHandle, "SQLExecute", &IIExecute );
    bind_sym( &dlHandle, "SQLFetch", &IIFetch );
    bind_sym( &dlHandle, "SQLFreeConnect", &IIFreeConnect );
    bind_sym( &dlHandle, "SQLFreeEnv", &IIFreeEnv );
    bind_sym( &dlHandle, "SQLFreeStmt", &IIFreeStmt );
    bind_sym( &dlHandle, "SQLGetCursorName", &IIGetCursorName );
    bind_sym( &dlHandle, "SQLNumResultCols", &IINumResultCols );
    bind_sym( &dlHandle, "SQLPrepare", &IIPrepare );
    bind_sym( &dlHandle, "SQLRowCount", &IIRowCount );
    bind_sym( &dlHandle, "SQLSetCursorName", &IISetCursorName );
    bind_sym( &dlHandle, "SQLSetParam", &IISetParam );
    bind_sym( &dlHandle, "SQLTransact", &IITransact );
    bind_sym( &dlHandle, "SQLColAttributes", &IIColAttributes );
    bind_sym( &dlHandle, "SQLTablePrivileges", &IITablePrivileges );
    bind_sym( &dlHandle, "SQLBindParam", &IIBindParam );
    bind_sym( &dlHandle, "SQLColumns", &IIColumns );
    bind_sym( &dlHandle, "SQLDriverConnect", &IIDriverConnect );
    bind_sym( &dlHandle, "SQLGetConnectOption", &IIGetConnectOption );
    bind_sym( &dlHandle, "SQLGetData", &IIGetData );
    bind_sym( &dlHandle, "SQLGetInfo", &IIGetInfo );
    bind_sym( &dlHandle, "SQLGetStmtOption", &IIGetStmtOption );
    bind_sym( &dlHandle, "SQLGetTypeInfo", &IIGetTypeInfo );
    bind_sym( &dlHandle, "SQLParamData", &IIParamData );
    bind_sym( &dlHandle, "SQLPutData", &IIPutData );
    bind_sym( &dlHandle, "SQLSetConnectOption", &IISetConnectOption );
    bind_sym( &dlHandle, "SQLSetStmtOption", &IISetStmtOption );
    bind_sym( &dlHandle, "SQLSpecialColumns", &IISpecialColumns );
    bind_sym( &dlHandle, "SQLStatistics", &IIStatistics );
    bind_sym( &dlHandle, "SQLTables", &IITables );
    bind_sym( &dlHandle, "SQLBrowseConnect", &IIBrowseConnect );
    bind_sym( &dlHandle, "SQLColumnPrivileges", &IIColumnPrivileges );
    bind_sym( &dlHandle, "SQLDescribeParam", &IIDescribeParam );
    bind_sym( &dlHandle, "SQLExtendedFetch", &IIExtendedFetch );
    bind_sym( &dlHandle, "SQLForeignKeys", &IIForeignKeys );
    bind_sym( &dlHandle, "SQLMoreResults", &IIMoreResults );
    bind_sym( &dlHandle, "SQLNativeSql", &IINativeSql );
    bind_sym( &dlHandle, "SQLNumParams", &IINumParams );
    bind_sym( &dlHandle, "SQLParamOptions", &IIParamOptions );
    bind_sym( &dlHandle, "SQLPrimaryKeys", &IIPrimaryKeys );
    bind_sym( &dlHandle, "SQLProcedureColumns", &IIProcedureColumns );
    bind_sym( &dlHandle, "SQLProcedures", &IIProcedures );
    bind_sym( &dlHandle, "SQLSetPos", &IISetPos );
    bind_sym( &dlHandle, "SQLSetScrollOptions", &IISetScrollOptions );
    bind_sym( &dlHandle, "SQLBindParameter", &IIBindParameter );
    bind_sym( &dlHandle, "SQLAllocHandle", &IIAllocHandle );
    bind_sym( &dlHandle, "SQLCloseCursor", &IICloseCursor );
    bind_sym( &dlHandle, "SQLCopyDesc", &IICopyDesc );
    bind_sym( &dlHandle, "SQLEndTran", &IIEndTran );
    bind_sym( &dlHandle, "SQLFetchScroll", &IIFetchScroll );
    bind_sym( &dlHandle, "SQLFreeHandle", &IIFreeHandle );
    bind_sym( &dlHandle, "SQLGetConnectAttr", &IIGetConnectAttr );
    bind_sym( &dlHandle, "SQLGetDescField", &IIGetDescField );
    bind_sym( &dlHandle, "SQLGetDescRec", &IIGetDescRec );
    bind_sym( &dlHandle, "SQLGetDiagField", &IIGetDiagField );
    bind_sym( &dlHandle, "SQLGetDiagRec", &IIGetDiagRec );
    bind_sym( &dlHandle, "SQLGetEnvAttr", &IIGetEnvAttr );
    bind_sym( &dlHandle, "SQLGetStmtAttr", &IIGetStmtAttr );
    bind_sym( &dlHandle, "SQLSetConnectAttr", &IISetConnectAttr );
    bind_sym( &dlHandle, "SQLSetDescField", &IISetDescField );
    bind_sym( &dlHandle, "SQLSetDescRec", &IISetDescRec );
    bind_sym( &dlHandle, "SQLSetEnvAttr", &IISetEnvAttr );
    bind_sym( &dlHandle, "SQLSetStmtAttr", &IISetStmtAttr );
    bind_sym( &dlHandle, "SQLColAttributeW", &IIColAttributeW );
    bind_sym( &dlHandle, "SQLColAttributesW", &IIColAttributesW );
    bind_sym( &dlHandle, "SQLBrowseConnectW", &IIBrowseConnectW );
    bind_sym( &dlHandle, "SQLConnectW", &IIConnectW );
    bind_sym( &dlHandle, "SQLDescribeColW", &IIDescribeColW );
    bind_sym( &dlHandle, "SQLErrorW", &IIErrorW );
    bind_sym( &dlHandle, "SQLGetDiagRecW", &IIGetDiagRecW );
    bind_sym( &dlHandle, "SQLGetDiagFieldW", &IIGetDiagFieldW );
    bind_sym( &dlHandle, "SQLExecDirectW", &IIExecDirectW );
    bind_sym( &dlHandle, "SQLGetStmtAttrW", &IIGetStmtAttrW );
    bind_sym( &dlHandle, "SQLDriverConnectW", &IIDriverConnectW );
    bind_sym( &dlHandle, "SQLSetConnectAttrW", &IISetConnectAttrW );
    bind_sym( &dlHandle, "SQLGetCursorNameW", &IIGetCursorNameW );
    bind_sym( &dlHandle, "SQLPrepareW", &IIPrepareW );
    bind_sym( &dlHandle, "SQLSetStmtAttrW", &IISetStmtAttrW );
    bind_sym( &dlHandle, "SQLSetCursorNameW", &IISetCursorNameW );
    bind_sym( &dlHandle, "SQLGetConnectOptionW", &IIGetConnectOptionW );
    bind_sym( &dlHandle, "SQLGetInfoW", &IIGetInfoW );
    bind_sym( &dlHandle, "SQLColumnsW", &IIColumnsW );
    bind_sym( &dlHandle, "SQLGetTypeInfoW", &IIGetTypeInfoW );
    bind_sym( &dlHandle, "SQLSetConnectOptionW", &IISetConnectOptionW );
    bind_sym( &dlHandle, "SQLSpecialColumnsW", &IISpecialColumnsW );
    bind_sym( &dlHandle, "SQLStatisticsW", &IIStatisticsW );
    bind_sym( &dlHandle, "SQLTablesW", &IITablesW );
    bind_sym( &dlHandle, "SQLForeignKeysW", &IIForeignKeysW );
    bind_sym( &dlHandle, "SQLNativeSqlW", &IINativeSqlW );
    bind_sym( &dlHandle, "SQLPrimaryKeysW", &IIPrimaryKeysW );
    bind_sym( &dlHandle, "SQLProcedureColumnsW", &IIProcedureColumnsW );
    bind_sym( &dlHandle, "SQLProceduresW", &IIProceduresW );
    bind_sym( &dlHandle, "SQLSetDescFieldW", &IISetDescFieldW );
    bind_sym( &dlHandle, "SQLGetDescFieldW", &IIGetDescFieldW );
    bind_sym( &dlHandle, "SQLGetDescRecW", &IIGetDescRecW );
    bind_sym( &dlHandle, "SQLGetConnectAttrW", &IIGetConnectAttrW );
    bind_sym( &dlHandle, "SQLTablePrivilegesW", &IITablePrivilegesW );
    bind_sym( &dlHandle, "SQLColumnPrivilegesW", &IIColumnPrivilegesW );
    bind_sym( &dlHandle, "SQLSetPos", &IISetPos );
    bind_sym( &dlHandle, "SQLGetFunctions", &IIGetFunctions );

    if (IIAllocEnv)
    {
        penv = pdbc->penv;
        rc = IIAllocEnv(&penv->hdr.driverHandle);
        if (rc != SQL_SUCCESS)
        {
            penv->hdr.driverError = TRUE;
            penv->errHdr.rc = rc;
            return rc;
        }
    }
    else
    {
        return SQL_ERROR;
    }

    if (rc == SQL_SUCCESS)
    {
        QUEUE *q;

        if (IIAllocConnect)
        {
            pDBC pdbc1;

            /*
            ** First allocate the known connect handle, then 
            ** see if others were allocated prior to the load
            ** and allocate them as well.
            */
            rc = IIAllocConnect( penv->hdr.driverHandle, 
                &pdbc->hdr.driverHandle);
            if (rc != SQL_SUCCESS)
            {
                pdbc->hdr.driverError = TRUE;
                pdbc->errHdr.rc = rc;
                return rc;
            }
            else
            {
                pdbc->hdr.state = C2;
                penv->hdr.state = E2;
            }

            for (q = penv->dbc_q.q_prev; 
                q != &penv->dbc_q; q = q->q_prev)
            {
                pdbc1 = (pDBC)q;
                if (pdbc1 != pdbc)
                {
                    rc = IIAllocConnect( penv->hdr.driverHandle, 
                        &pdbc1->hdr.driverHandle);
                    if (rc != SQL_SUCCESS)
                    {
                        pdbc1->hdr.driverError = TRUE;
                        pdbc1->errHdr.rc = rc;
                        return rc;
                    }
                    else
                    {
                        pdbc->hdr.state = C2;
                        penv->hdr.state = E2;
                    }

                }
            }
        }
        else
        {
            return SQL_ERROR;
        }
    }
    return SQL_SUCCESS;
}

/*
** Name: 	IIodbc_insertPoolElement
** 
** Description: 
**              Inserts a connection handle into a driver or environment
**              connection pool.
**
** Inputs:      
**              pdbc        CLI connection handle to be inserted.
**              pool_type   HENV_POOL or DRIVER_POOL.              
**
** Outputs: 
**              None.
**
** Returns: 
**              SQL_SUCCESS
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
**              Adds a pool member to the driver or environment connection
**              pool queue.
**
** History: 
**    27-Jan-07 (Ralph Loen) SIR 117786
**      Created.
*/ 
BOOL IIodbc_insertPoolElement( pDBC pdbc, const i2 pool_type)
{
    i2 handle_type;
    pPOOL pool;
    QUEUE *pool_q;
    SQLHANDLE handle;
    pENV penv = pdbc->penv;
    SQLINTEGER *count;
    
    ODBC_EXEC_TRACE(ODBC_IN_TRACE) ("IIodbc_insertPoolElement entry\n");

    switch (pool_type)
    {
    case HENV_POOL:
        handle_type = SQL_HANDLE_ENV;
        pool_q = &penv->pool_q;
        handle = penv;
        count = &penv->pool_count;
        break;
    
    case DRIVER_POOL:
        handle_type = SQL_HANDLE_IIODBC_CB;
        pool_q = &IIodbc_cb.pool_q;
        handle = (SQLHANDLE)&IIodbc_cb;
        count = &IIodbc_cb.pool_count;
        break;
    }
    pool = ( pPOOL )MEreqmem(0, sizeof(IIODBC_POOL), TRUE, NULL);
    TMnow(&pdbc->expire_time);
	pdbc->expire_time.TM_secs += 1;
    ODBC_EXEC_TRACE(ODBC_EX_TRACE)
        ("%p inserted into the pool with expire_time %d\n",pdbc,
              pdbc->expire_time);
    pool->pdbc = (PTR)pdbc;
    applyLock(handle_type, handle);
    QUinsert((QUEUE *)pool, pool_q);
    *count += 1;
    releaseLock(handle_type, handle);
    ODBC_EXEC_TRACE(ODBC_EX_TRACE) 
        ("IIodbc_insertPoolElement: exit pool count is %d\n",*count);
    return TRUE;
}

/*
** Name: 	IIodbc_fetchPoolElement
** 
** Description: 
**              Fetches a connection handle from the connection pool
**              if the match criteria are met.  
**
** Inputs:      
**              pdbc        The current connection handle.
**              pool_type   HENV_POOL or DRIVER_POOL.
** Outputs: 
**              pdbcMatch   Matching pool handle, if found.
**
** Returns: 
**              SQL_SUCCESS if a match is found.
**              SQL_ERROR if a match is not found.
** 
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
**              Removes and frees a pool handle, and the underlying
**              CLI connection handle, if a match is found.
**
** History: 
**    27-Jan-07 (Ralph Loen) SIR 117786
**      Created.
*/ 
BOOL IIodbc_fetchPoolElement (pDBC pdbc, const i2 pool_type,
    pDBC *pdbcMatch)
{
    QUEUE *pool_q, *q;
    SQLHANDLE handle;
    i2 handle_type;
    pPOOL pool;
    pDBC pool_pdbc;
    pENV penv = pdbc->penv;
    SQLINTEGER *count;
    char pool_pwd[OCFG_MAX_STRLEN] = "\0";
    char pwd[OCFG_MAX_STRLEN] = "\0";
    BOOL rc;
    STATUS status;
    TM_STAMP stamp;
    char stampStr[TM_SIZE_STAMP];
   
    *pdbcMatch = NULL;
    TMget_stamp(&stamp);
    TMstamp_str(&stamp, stampStr);
    ODBC_EXEC_TRACE(ODBC_IN_TRACE) ("IIodbc_fetchPoolElement entry at %s\n",
        stampStr);
    switch (pool_type)
    {
    case HENV_POOL:
        handle_type = SQL_HANDLE_ENV;
        pool_q = &penv->pool_q;
        handle = penv;
        count = &penv->pool_count;
        break;
    
    case DRIVER_POOL:
        handle_type = SQL_HANDLE_IIODBC_CB;
        pool_q = &IIodbc_cb.pool_q;
        handle = (SQLHANDLE)NULL;
        count = &IIodbc_cb.pool_count;
        break;
    }
   
    applyLock(handle_type, handle);
    /*
    ** Check if the attributes match.  Return TRUE and remove the handle 
    ** from the pool if a match is found.
    */
    for( q = pool_q->q_prev; q != pool_q; q = q->q_prev )
    {
        pool_pdbc = (pDBC)((pPOOL)q)->pdbc;
        if (!IIodbc_connStringMatches(pool_pdbc->dsn,pdbc->dsn))
            continue;
        if (!IIodbc_connStringMatches(pool_pdbc->uid,pdbc->uid))
            continue;
        
        if (pool_pdbc->pwd && pdbc->pwd)
        {
            status = IIodbc_decode(pdbc->pwd, IIodbc_cb.hostname, pwd);
            status = IIodbc_decode(pool_pdbc->pwd, IIodbc_cb.hostname, 
                pool_pwd);
            if (!IIodbc_connStringMatches(pool_pwd, pwd))
                continue;
        }
        else if (!IIodbc_connStringMatches(pool_pdbc->pwd,pdbc->pwd))
            continue;

        if (pdbc->driverConnect && pool_pdbc->driverConnect)
        {
            if (!IIodbc_connStringMatches(pool_pdbc->database,pdbc->database))
                continue;
            if (!IIodbc_connStringMatches(pool_pdbc->server_type,
                pdbc->server_type))
                continue;
            if (!IIodbc_connStringMatches(pool_pdbc->vnode,pdbc->vnode))
                continue;                
            if (!IIodbc_connStringMatches(pool_pdbc->driver,pdbc->driver))
                continue;
            if (!IIodbc_connStringMatches(pool_pdbc->group,pdbc->group))
                continue;
            if (!IIodbc_connStringMatches(pool_pdbc->role_name,pdbc->role_name))
                continue;
            if (!IIodbc_connStringMatches(pool_pdbc->role_pwd,pdbc->role_pwd))
                continue;
            if (!IIodbc_connStringMatches(pool_pdbc->dbms_pwd,pdbc->dbms_pwd))
                continue;
            if (!IIodbc_connStringMatches(pool_pdbc->with_option,pdbc->with_option))
                continue;
            if (!penv->relaxed_match)
            {
                if (pdbc->blank_date != pool_pdbc->blank_date)
                    continue;
                if (pdbc->date_1582 != pool_pdbc->date_1582)
                    continue;                    
                if (pdbc->cat_connect != pool_pdbc->cat_connect)
                    continue;
                if (pdbc->numeric_overflow != pool_pdbc->numeric_overflow)
                    continue;
                if (pdbc->cat_schema_null != pool_pdbc->cat_schema_null)
                    continue;
                if (pdbc->select_loops != pool_pdbc->select_loops)
                    continue;                    
            }
        }        
        pool = (pPOOL)q;
        *pdbcMatch = (pDBC)pool->pdbc;
        QUremove(q);
        MEfree((PTR)q);
        *count -= 1;
        ODBC_EXEC_TRACE(ODBC_EX_TRACE)
            ("IIodbc_fetchPoolElement: Fetched handle %p, pool count is %d\n",
            *pdbcMatch, *count);
        rc =  TRUE;
        goto routine_exit;
    } /* for( q = pool_q->q_prev; q != pool_q; q = q->q_prev ) */
    rc = FALSE;

routine_exit:
    releaseLock(handle_type, handle);
    TMget_stamp(&stamp);
    TMstamp_str(&stamp, stampStr);
    ODBC_EXEC_TRACE(ODBC_IN_TRACE) 
        ("IIodbc_fetchPoolElement returns %s at %s\n", rc ? "TRUE" : "FALSE",
            stampStr);
    return rc;
}

/*
** Name: 	IIodbc_freeEnvHandles
** 
** Description: 
**              Frees all allocated environment handles in the
**              IIodbc_cb control block.  
**
** Inputs:      
**              None.
**
** Outputs: 
**              None.
**
** Returns: 
**              TRUE if succeeded.
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
**              Removes all environment handles from the IIodbc_cb queue.
**
** History: 
**    27-Jan-07 (Ralph Loen) SIR 117786
**      Created.
*/ 
BOOL IIodbc_freeEnvHandles ()
{
    QUEUE *q;
    pENV penv;
    RETCODE rc;

    ODBC_EXEC_TRACE(ODBC_IN_TRACE) ("IIodbc_freeEnvHandles entry\n");

    for (q = IIodbc_cb.env_q.q_prev; q != &IIodbc_cb.env_q; 
        q = IIodbc_cb.env_q.q_prev)
    {
        penv = (pENV)q;
        rc = IIFreeHandle(SQL_HANDLE_ENV, penv->hdr.driverHandle);
        
        applyLock(SQL_HANDLE_ENV, penv);
        if (SQL_SUCCEEDED (rc) && penv->hdr.state == E1)
            penv->hdr.state = E0;
        else if (rc != SQL_SUCCESS)
        {
            penv->hdr.driverError = TRUE;
            penv->errHdr.rc = rc;
        }
        releaseLock(SQL_HANDLE_ENV, penv);
        /*
        ** Inovcation of IIodbc_freeHandle() removes the environment handle 
        ** from the linked list in IIodbc_cb.
        */
        penv->pooling = NO_POOL;
        IIodbc_freeHandle(SQL_HANDLE_ENV, penv);
    }
    ODBC_EXEC_TRACE(ODBC_IN_TRACE) ("IIodbc_freeEnvHandles exit\n");
    return TRUE;
}

/*
** Name: 	IIodbc_connStringMatches
** 
** Description: 
**              Determines whether two connection strings match
**              according to ODBC criteria.  For the ODBC, null and
**              empty strings match.
**
** Inputs:      
**              str1    first string to compare.
**              str2    second string to compare.
**
** Outputs: 
**              None.
**
** Returns: 
**              TRUE if a match is found.
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
**              None.
**
** History: 
**    27-Jan-07 (Ralph Loen) SIR 117786
**      Created.
*/ 

BOOL IIodbc_connStringMatches(char *str1, char *str2)
{
    BOOL ret;

    ODBC_EXEC_TRACE(ODBC_IN_TRACE) 
        ("IIodbc_connStringMatches(%s, %s) entry\n", 
		str1 && *str1 ? str1 : "<null>", str2 && *str2 ? str2 : "<null>");

    if (str1 == str2)
    {
        ret = TRUE;
        goto exit_routine;
    }
    if (!str1 && !str2)
    {
        ret = TRUE;
        goto exit_routine;
    }
    else if (str1 && !str2)
    {
        ret = FALSE;
        goto exit_routine;
    }
    else if (str2 && !str1)
    {
        ret = FALSE;
        goto exit_routine;
    }
    else if (!str1 && CMcmpcase(str2,"\0"))
    {
        ret = TRUE;
        goto exit_routine;
    }
    else if (!str2 && CMcmpcase(str1,"\0"))
    {
        ret = TRUE;
        goto exit_routine;
    }
    else if (STbcompare( str1, 0, str2, 0, TRUE ))
    {
        ret = FALSE;
        goto exit_routine;
    }
    ret = TRUE;

exit_routine:

    ODBC_EXEC_TRACE(ODBC_IN_TRACE) 
        ("IIodbc_connStringMatches returns %s\n", ret ? "TRUE" : "FALSE");
    return ret;
}

/*
** Name: 	IIodbc_formatConnHandle
** 
** Description: 
**              Stores connection string attributes in the connection
**              handle.
** Inputs:      
**              szConnStrIn     Connection string.
**
** Outputs: 
**              pdbc            CLI connection handle.
**
** Returns: 
**              TRUE if succeeded.
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
**              None.
**
** History: 
**    27-Jan-07 (Ralph Loen) SIR 117786
**      Created.
*/ 

BOOL IIodbc_formatConnHandle(char *szConnStrIn, pDBC pdbc)
{
    char *p, *p1;
    i2 len;
    char opwd[OCFG_MAX_STRLEN*2];
    STATUS status;

    ODBC_EXEC_TRACE(ODBC_IN_TRACE) 
        ("IIodbc_formatConnHandle(%s, %p) entry\n", szConnStrIn, pdbc);

    applyLock(SQL_HANDLE_DBC, pdbc);
    p = strtok(szConnStrIn,";");
    /* 
    ** Parse each token in the connection string and store any
    ** recognized attributes in the connection handle.
    */
    while (p)
    {
        while (CMwhite(p)) CMnext(p);
        len = (i2)STlength(DSN_STR_DSN);    
        if (!STbcompare (p, len, (char *) DSN_STR_DSN, len, TRUE))
        {  
            p1 = STstrindex(p, "=", 0, TRUE);
            CMnext(p1);
            pdbc->dsn = STalloc(p1);
            goto parseNext;
        }        
        len = (i2)STlength(DSN_STR_DATABASE);
        if (!STbcompare(p, len, (char *) DSN_STR_DATABASE, len, TRUE))
        {
            p1 = STstrindex(p, "=", 0, TRUE);
            CMnext(p1);
            pdbc->database = STalloc(p1);
            goto parseNext;
        }
        len = (i2)STlength(DSN_STR_SERVER_TYPE);        
        if (!STbcompare (p, len, (char *) DSN_STR_SERVER_TYPE, 
            len, TRUE))
        {
            p1 = STstrindex(p, "=", 0, TRUE);
            CMnext(p1);
            pdbc->server_type = STalloc(p1);
            goto parseNext;            
        }
        len = (i2)STlength(DSN_STR_SERVER);
        if (!STbcompare (p, (i2)len, (char *) DSN_STR_SERVER, 
            len, TRUE))
        {
            p1 = STstrindex(p, "=", 0, TRUE);
            CMnext(p1);
            pdbc->vnode = STalloc(p1);
            goto parseNext;            
        }
        len = (i2)STlength(UID_STR);
        if (!STbcompare (p, len, UID_STR, len, TRUE))
        {
            p1 = STstrindex(p, "=", 0, TRUE);
            CMnext(p1);
            pdbc->uid = STalloc(p1);
            goto parseNext;            
        }
        len = (i2)STlength(PWD_STR);
        if (!STbcompare (p, len, PWD_STR, len, TRUE))
        {
            p1 = STstrindex(p, "=", 0, TRUE);
            CMnext(p1);
            status = IIodbc_encode(p1, IIodbc_cb.hostname, opwd);
            pdbc->pwd = STalloc(opwd);
            goto parseNext;            
        }
        len = (i2)STlength(DSN_STR_DRIVER);
        if (!STbcompare (p, len, DSN_STR_DRIVER, len, TRUE))
        {
            p1 = STstrindex(p, "=", 0, TRUE);
            CMnext(p1);
            pdbc->driver = STalloc(p1);
            goto parseNext;            
        }
        len = (i2)STlength(DSN_STR_ROLE_NAME);
        if (!STbcompare (p, len, DSN_STR_ROLE_NAME, len, TRUE))
        {
            p1 = STstrindex(p, "=", 0, TRUE);
            CMnext(p1);
            pdbc->role_name = STalloc(p1);
            goto parseNext;            
        }
        len = (i2)STlength(DSN_STR_ROLE_PWD);
        if (!STbcompare (p, len, DSN_STR_ROLE_PWD, len, TRUE))
        {
            p1 = STstrindex(p, "=", 0, TRUE);
            CMnext(p1);
            pdbc->role_pwd = STalloc(p1);
            goto parseNext;            
        }
        len = (i2)STlength(DSN_STR_GROUP);
        if (!STbcompare (p, len, DSN_STR_GROUP, len, TRUE))
        {
            p1 = STstrindex(p, "=", 0, TRUE);
            CMnext(p1);
            pdbc->group = STalloc(p1);
            goto parseNext;            
        }
        len = (i2)STlength(DSN_STR_DBMS_PWD);
        if (!STbcompare (p, len, DSN_STR_DBMS_PWD, len, TRUE))
        {
            p1 = STstrindex(p, "=", 0, TRUE);
            CMnext(p1);
            pdbc->dbms_pwd = STalloc(p1);
            goto parseNext;            
        }
        len = (i2)STlength( DSN_STR_WITH_OPTION);
        if (!STbcompare (p, len,  DSN_STR_WITH_OPTION, len, TRUE))
        {
            p1 = STstrindex(p, "=", 0, TRUE);
            CMnext(p1);
            pdbc->with_option = STalloc(p1);
            goto parseNext;            
        }
        len = (i2)STlength(DB_STR);
        if (!STbcompare (p, (i2)len, (char *) DB_STR, 
            len, TRUE))
        {
            p1 = STstrindex(p, "=", 0, TRUE);
            CMnext(p1);
            pdbc->database = STalloc(p1);
            goto parseNext;            
        }
        len = (i2)STlength(DATASOURCENAME_STR);
        if (!STbcompare (p, (i2)len, (char *) DATASOURCENAME_STR, 
            len, TRUE))
        {
            p1 = STstrindex(p, "=", 0, TRUE);
            CMnext(p1);
            pdbc->dsn = STalloc(p1);
            goto parseNext;            
        }
        len = (i2)STlength(LOGONID_STR);
        if (!STbcompare (p, (i2)len, (char *) LOGONID_STR, 
            len, TRUE))
        {
            p1 = STstrindex(p, "=", 0, TRUE);
            CMnext(p1);
            pdbc->uid = STalloc(p1);
            goto parseNext;            
        }
        len = (i2)STlength(SRVR_STR);
        if (!STbcompare (p, (i2)len, (char *) SRVR_STR, 
            len, TRUE))
        {
            p1 = STstrindex(p, "=", 0, TRUE);
            CMnext(p1);
            pdbc->vnode = STalloc(p1);
            goto parseNext;            
        }
        if (!pdbc->penv->relaxed_match)
        {
            len = (i2)STlength(DSN_STR_SELECT_LOOPS);
            if (!STbcompare (p, len,  DSN_STR_SELECT_LOOPS, len, TRUE))
            {
                p1 = STstrindex(p, "=", 0, TRUE);
                CMnext(p1);
                if (CMcmpnocase(p,"y"))
                    pdbc->select_loops = TRUE;
                goto parseNext;            
            }
            len = (i2)STlength(DSN_STR_BLANK_DATE);
            if (!STbcompare (p, len,  DSN_STR_BLANK_DATE, len, TRUE))
            {
                p1 = STstrindex(p, "=", 0, TRUE);
                CMnext(p1);
                if (CMcmpnocase(p,"y"))
                    pdbc->blank_date = TRUE;
                goto parseNext;
            }                
            len = (i2)STlength(DSN_STR_DATE_1582);
            if (!STbcompare (p, len,  DSN_STR_DATE_1582, len, TRUE))
            {
                p1 = STstrindex(p, "=", 0, TRUE);
                CMnext(p1);
                if (CMcmpnocase(p,"y"))
                    pdbc->date_1582 = TRUE;
                goto parseNext;            
            }
            len = (i2)STlength(DSN_STR_NUMERIC_OVERFLOW);
            if (!STbcompare (p, len,  DSN_STR_NUMERIC_OVERFLOW, len, TRUE))
            {
                p1 = STstrindex(p, "=", 0, TRUE);
                CMnext(p1);
                if (CMcmpnocase(p,"y"))
                    pdbc->numeric_overflow = TRUE;
                goto parseNext;            
            }
            len = (i2)STlength(DSN_STR_CAT_SCHEMA_NULL);
            if (!STbcompare (p, len,  DSN_STR_CAT_SCHEMA_NULL, len, TRUE))
            {
                p1 = STstrindex(p, "=", 0, TRUE);
                CMnext(p1);
                if (CMcmpnocase(p,"y"))
                    pdbc->cat_schema_null = TRUE;
                goto parseNext;            
            }
        }
parseNext:
        p = strtok(NULL, ";");
    }
    
    releaseLock(SQL_HANDLE_DBC, pdbc);    
    ODBC_EXEC_TRACE(ODBC_IN_TRACE) ("IIodbc_formatConnHandle exit\n");
    return TRUE;    
}

/*
** Name: 	IIodbc_initControlBlock
** 
** Description: 
**              Initializes the ODBC CLI control block and other
**              global CLI environment variables.
**
** Inputs:      
**              None.
**
** Outputs: 
**              None.
**
** Returns: 
**              void.
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
**              Fills the global IIodbc_cb structure with zeros.  Initializes
**              semaphores and default linkes lists.  Initializes the API.
**
** History: 
**    27-Jan-07 (Ralph Loen) SIR 117786
**      Created.
*/ 
void IIodbc_initControlBlock()
{
    IIAPI_INITPARM  initParm;    
#ifndef NT_GENERIC
    bool sysIniDefined = FALSE;
    char altPath[OCFG_MAX_STRLEN]="\0";
    char *tmpPath = getMgrInfo(&sysIniDefined);
    STATUS status;
#endif /* NT_GENERIC */

    /*
    ** Initialize Ingres API
    */
    initParm.in_timeout = -1;

# if defined(IIAPI_VERSION_5)
    initParm.in_version = IIAPI_VERSION_5;
# elif defined(IIAPI_VERSION_4)
    initParm.in_version = IIAPI_VERSION_4;
# elif defined(IIAPI_VERSION_3)
    initParm.in_version = IIAPI_VERSION_3;
# endif
    IIapi_initialize (&initParm);
    initLock(SQL_HANDLE_IIODBC_CB, NULL );
    MEfill(sizeof(IIODBC_CB), 0, &IIodbc_cb);
    QUinit(&IIodbc_cb.env_q);
    QUinit(&IIodbc_cb.desc_q);
    setCLI();
#ifndef NT_GENERIC
        if (tmpPath && *tmpPath)
            STcopy(tmpPath, altPath);

        if (*altPath && !sysIniDefined)
        {
            openConfig(NULL, OCFG_ALT_PATH, altPath, 
                (PTR *)&IIodbc_cb.drvConfigHandle, &status); 
        }
        else
        {
            openConfig(NULL, OCFG_SYS_PATH, NULL, 
                (PTR *)&IIodbc_cb.drvConfigHandle, &status);
        }
    IIodbc_cb.timeout = getTimeout(IIodbc_cb.drvConfigHandle);
#else
    IIodbc_cb.timeout = 1;
#endif /* NT_GENERIC */ 
    ODBC_INITTRACE();
    GChostname(IIodbc_cb.hostname, sizeof(IIodbc_cb.hostname));
    ODBC_EXEC_TRACE(ODBC_IN_TRACE) ("IIodbc_initControlBlock exit\n");
    return;
}
/*
** Name: IIodbc_encode
**
** Description:
**  Encrypts a string using key provided.  Encrypted
**  value is returned in text format.  Input string
**  lengths should not exceed GCS_MAX_STR.  Output
**  buffer must be at least GCS_MAX_BUF.
**
** Input:
**  str	    Value to be encrypted.
**  key	    Key for encryption.
**
** Output:
**  buff    Encrypted value in text format.
**
** Returns:
**  i4  Length of encrypted value.
**
** History:
**  01-Mar-07 (Ralph Loen) SIR 117786
**      Cloned from gcs_encode() written by Gordy Thorpe.
*/

i4 IIodbc_encode( char *str, char *key, char *buff )
{
    u_i1    kbuff[ ENC_KEY_LEN + 1 ];
    u_i1    ibuff[ GCS_MAX_BUF + 1 ];
    u_i1    tmp[ GCS_MAX_BUF + 1 ];
    CI_KS   ksch;
    char    *src;
    i4      len, csize = (i4)STlength( cset );
    bool    done;

    /*
    ** Generate a ENC_KEY_LEN size encryption 
    ** key from key provided.  The input key 
    ** will be duplicated or truncated as needed.  
    ** ENC_KEY_LEN must be a multiple of CRYPT_SIZE.
    */
    for( src = key, len = 0; len < ENC_KEY_LEN; len++ )
    {
        if ( ! *src )  src = key;
        kbuff[ len ] = *src++;
    }

    kbuff[ len ] = EOS;
    CIsetkey( (PTR)kbuff, ksch );

    /*
    ** The string to be encrypted must be a multiple of
    ** CRYPT_SIZE in length for the block encryption
    ** algorithm used by CIencode().  To provide some
    ** randomness in the output, the first character
    ** or each CRYPT_SIZE block is assigned a random
    ** value.  The remainder of the block is filled
    ** from the string to be encrypted.  If the final
    ** block is not filled, random values are used as
    ** padding.  A fixed delimiter separates the 
    ** original string and the padding (the delimiter 
    ** must always be present).
    */
    for( done = FALSE, len = 0; ! done  &&  len < (sizeof(ibuff) - 1); )
    {
        /*
        ** First character in each encryption block is random.
        */
        ibuff[ len++ ] = cset[ (i4)(MHrand() * csize) % csize ];

        /*
        ** Load string into remainder of encryption block.
        */
        while( *str  &&  len % CRYPT_SIZE )  ibuff[ len++ ] = *str++;

        /*
        ** If encryption block not filled, fill with random padding.
        */
        if ( len % CRYPT_SIZE )
        {
            /*
            ** Padding begins with fixed delimiter.
            */
            ibuff[ len++ ] = CRYPT_DELIM;
            done = TRUE;	/* Only done when delimiter appended */
    
            /*
            ** Fill encryption block with random padding.
            */
            while( len % CRYPT_SIZE )
                ibuff[ len++ ] = cset[ (i4)(MHrand() * csize) % csize ];
        }
    }

    /*
    ** Encrypt and convert to text.
    */
    ibuff[ len ] = EOS;
    CIencode( (PTR)ibuff, len, ksch, (PTR)tmp );
    CItotext( (uchar *)tmp, len, (PTR)buff );

    return( (i4)STlength( buff ) );
}

/*
** Name: IIodbc_decode
**
** Description:
**  Decrypt a string encrypted by IIodbc_encode().
**  Encrypted lengths should not exceed GCS_MAX_BUF.
**  Decrypted lengths should not exceed GCS_MAX_STR.
**
** Input:
**  str	    Encrypted value to be decrypted.
**  key	    Key for decryption.
**
** Output:
**  buff    Decrypted value.
**
** Returns:
**  i4      length of decrypted value.
**
** History:
**  01-Mar-07 (Ralph Loen) SIR 117786
**      Cloned from gcs_decode() written by Gordy Thorpe.
*/

i4 IIodbc_decode( char *str, char *key, char *buff )
{
    u_i1    kbuff[ ENC_KEY_LEN + 1 ];
    u_i1    obuff[ GCS_MAX_BUF + 1 ];
    u_i1    tmp[ GCS_MAX_BUF + 1 ];
    CI_KS   ksch;
    char    *src;
    i4      len;

    /*
    ** Generate a ENC_KEY_LEN size encryption 
    ** key from key provided.  The input key 
    ** will be duplicated or truncated as needed.
    ** ENC_KEY_LEN must be a multiple of CRYPT_SIZE.
    */
    for( src = key, len = 0; len < ENC_KEY_LEN; len++ )
    {
        if ( ! *src )  src = key;
        kbuff[ len ] = *src++;
    }

    kbuff[ len ] = EOS;
    CIsetkey( (PTR)kbuff, ksch);
    CItobin( (PTR)str, &len, (u_char *)tmp );

    /*
    ** Encrypted string should already be
    ** the correct size, but ensure that
    ** it really is a multiple of CRYPT_SIZE.
    */
    while( len % CRYPT_SIZE )  tmp[ len++ ] = ' ';
    tmp[ len ] = EOS;
    CIdecode( (PTR)tmp, len, ksch, (PTR)obuff );

    /*
    ** Remove padding and delimiter 
    ** from last encryption block.
    */
    while( len  &&  obuff[ --len ] != CRYPT_DELIM );
    obuff[ len ] = EOS;

    /*
    ** Extract original string skipping first
    ** character in each encryption block.
    */
    for( str = buff, len = 0; obuff[ len ]; len++ )
    if ( len % CRYPT_SIZE )  *str++ = obuff[ len ];

    *str = EOS;

    return( (i4) (str - buff ));
}
