/*
** Copyright (c) 2010 Ingres Corporation 
*/ 

#include <compat.h>
#include <gc.h>
#include <me.h>
#include <mu.h>
#include <qu.h>
#include <st.h>
#include <tr.h>
#include <sql.h>                    /* ODBC Core definitions */
#include <sqlext.h>                 /* ODBC extensions */
#include <iiapi.h>
#include <erodbc.h>
#include <iiodbc.h>
#include <iiodbcfn.h>
#include <idmseini.h>
#include <odbccfg.h>
#include <sqlca.h>
#include <idmsxsq.h>                /* control block typedefs */
#include <idmsutil.h>               /* utility stuff */
#include "odbclocal.h"
#include "tracefn.h"

/* 
** Name: unicode.c - Unicode version of ODBC functions for the ODBC CLI.
** 
** Description: 
** 		This file defines: 
** 
**              SQLColAttributeW - Get connection attributes.
**              SQLConnectW - Connect to the database via DSN.
**              SQLDescribeColW - Get column attributes.
**              SQLErrorW - Get error information (ODBC 2.0 version).
**              SQLExecDirectW - Execute a query directly.
**              SQLGetConnectAttrW - Get column attributes.
**              SQLGetCursorNameW - Get cursor name.
**              SQLSetDescFieldW - Set descriptor column attributes.
**              SQLGetDescFieldW - Get descriptor column attributes.
**              SQLGetDescRecW - Get descriptor of a row.
**              SQLGetDiagFieldW - Get error items. 
**              SQLGetDiagRecW - Get error information.
**              SQLPrepareW - Prepare a query.
**              SQLSetConnectAttrW - Get connection attributes.
**              SQLSetCursorNameW - Set a cursor name.
**              SQLColumnsW - Get column metadata.
**              SQLGetConnectOptionsW - Get connection attributes (2.0 vers).
**              SQLGetInfoW - Get driver information.
**              SQLGetTypeInfoW - Get data type information.
**              SQLSetConnectOptionsW - Set connection attributes (2.0 vers).
**              SQLSpecialColumnsW - Get unique column information.
**              SQLStatisticsW - Get table statictics.
**              SQLTablesW - Get table information.
**              SQLDriverConnectW - Connect via connection string.
**              SQLGetStmtAttrW - Get statement attributes.
**              SQLSetStmtAttrW - Set statement attributes.
**              SQLNativeSqlW - Return modified query string.
**              SQLPrimaryKeysW - Return primary keys in a table.
**              SQLProcedureColumnsW - Get info on db procedure parameters.
**              SQLProceduresW - Get info about database procedures.
**              SQLForeignKeysW - List foreign keys in a table.
**              SQLTablePrivilegesW - List table privileges.
**              SQLDriversW - List driver attributes.
**
** History: 
**   14-jun-04 (loera01)
**     Created.
**   15-Jul-2005 (hanje04)
**	Include iiodbcfn.h and tracefn.h which are the new home for the 
**	ODBC CLI function pointer definitions.
**   03-Jul-2006 (Ralph Loen) SIR 116326
**       Add support for SQLBrowseConnectW().
**   07-Jul-2006 (Ralph Loen) SIR 116326
**      In SQLBrowseConnectW(), make sure releaseLock() is invoked before
**      returning.
**   14-Jul-2006 (Ralph Loen) SIR 116385
**      Add support for SQLTablePrivilegesW()
**   26-Feb-1007 (Ralph Loen) SIR 117786
**      Add support for ODBC connection pooling.
**   13-Mar-2007 (Ralph Loen) SIR 117786
**      Tightened up treatement of ODBC connection states.  Replaced
**      Windows-dependent MultiByteToWideChar() and WideCharToMultiByte()
**      with API-dependent ConvertWCharToChar() and ConvertCharToWChar().
**   21-May-2007 (Ralph Loen) SIR 117786
**      For SQLDriverConnectW() and SQLDriverConnect(), convert the 
**      connection information to ASCII before storing in the connection
**      pool--if connection pooling is enabled.
**   03-Sep-2010 (Ralph Loen) Bug 124348
**      Replaced SQLINTEGER, SQLUINTEGER and SQLPOINTER arguments with
**      SQLLEN, SQLULEN and SQLLEN * for compatibility with 64-bit
**      platforms.
**   06-Sep-2010 (Ralph Loen) Bug 124348
**      Added version of SQLColAttribute() dependent on _WIN64 macro for
**      compatibility with MS implementation.
**   07-Oct-2010 (Ralph Loen) Bug 124570
**      Clone revised treatment of ConvertUCS2ToUCS4() and ConvertUCS4ToUCS2()
**      from the driver.  This addresses potential byte-alignment issues on
**      64-bit platforms.
*/ 

/*
** Name: 	SQLColAttributeW - Get column attributes
** 
** Description: 
**	        SQLColAttributeW returns descriptor information for a 
**              column in a result set. 
**
** Inputs:
**              StatementHandle - Statement handle.
**              ColumnNumber - The record number in the row descriptor.
**              FieldIdentifier - Field type to be returned.
**              BufferLength - Maximum length of returned attribute.
**
** Outputs: 
**              ValuePtrParm - Returned field attribute.
**              StringLengthPtr - Returned attribute length.
**              pfDesc - Returned column attribute.
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

#ifdef _WIN64
SQLRETURN SQL_API SQLColAttributeW(
    SQLHSTMT         hstmt,
    SQLUSMALLINT     ColumnNumber,
    SQLUSMALLINT     FieldIdentifier,
    SQLPOINTER       ValuePtr,
    SQLSMALLINT      BufferLength,       /*   count of bytes */
    SQLSMALLINT     *StringLengthPtr,    /* ->count of bytes */
    SQLLEN          *NumericAttributePtr) 
#else
SQLRETURN SQL_API SQLColAttributeW(
    SQLHSTMT         hstmt,
    SQLUSMALLINT     ColumnNumber,
    SQLUSMALLINT     FieldIdentifier,
    SQLPOINTER       ValuePtr,
    SQLSMALLINT      BufferLength,       /*   count of bytes */
    SQLSMALLINT     *StringLengthPtr,    /* ->count of bytes */
    SQLPOINTER       NumericAttributePtr) 
#endif
{
    pSTMT pstmt = (pSTMT)hstmt;
    RETCODE rc, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLColAttributeW(hstmt,ColumnNumber,
        FieldIdentifier, ValuePtr, BufferLength, StringLengthPtr, 
        NumericAttributePtr), traceRet);

    if (validHandle((SQLHANDLE)pstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff((SQLHANDLE)pstmt, SQL_HANDLE_STMT);

    rc = IIColAttributeW(pstmt->hdr.driverHandle,
        ColumnNumber,
        FieldIdentifier,
        ValuePtr,
        BufferLength,
        StringLengthPtr,
        NumericAttributePtr);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_STMT, pstmt);
    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLConnectW - Connect to database.
** 
** Description: 
**              SQLConnectW connects to the database via a pre-defined
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
*/ 

SQLRETURN SQL_API SQLConnectW(
    SQLHDBC          hdbc,
    SQLWCHAR        *szWideDSN,
    SQLSMALLINT      cbWideDSN,      /*   count of chars */
    SQLWCHAR        *szWideUID,
    SQLSMALLINT      cbWideUID,      /*   count of chars */
    SQLWCHAR        *szWidePWD,
    SQLSMALLINT      cbWidePWD)      /*   count of chars */
{
    RETCODE rc = SQL_SUCCESS, ret, traceRet = 1;
    pDBC pdbc = (pDBC)hdbc, pdbcMatch=NULL;
    pENV penv = (pENV)pdbc->penv;
    char pWork[OCFG_MAX_STRLEN];
    char opwd[OCFG_MAX_STRLEN];
    char database[OCFG_MAX_STRLEN];
    char hostname[HOSTNAME_LEN];
    STATUS status;
    pDBC handle;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLConnectW(hdbc, szWideDSN,
        cbWideDSN, szWideUID, cbWideUID, szWidePWD, cbWidePWD), traceRet);

    if (!hdbc)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    if (pdbc->hdr.type != SQL_HANDLE_DBC)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    ODBC_EXEC_TRACE(ODBC_EX_TRACE)
        ("SQLConnectW entry: state is %s\n", cli_state[pdbc->hdr.state]);
    
    resetErrorBuff(hdbc, SQL_HANDLE_DBC);

    applyLock(SQL_HANDLE_DBC, pdbc);
    if (szWideDSN)
    {
        ret = ConvertWCharToChar( szWideDSN, cbWideDSN, pWork, 
            OCFG_MAX_STRLEN, NULL, NULL, NULL);
        if (!SQL_SUCCEEDED(ret))
        {
            insertErrorMsg(pdbc, SQL_HANDLE_DBC, SQL_HY001,
                SQL_ERROR, NULL, 0);
            rc = SQL_ERROR;
            releaseLock(SQL_HANDLE_DBC, pdbc);
            goto routine_exit;
        }

        if (!SQLGetPrivateProfileString ((char *)pWork, KEY_DATABASE, 
            "", database, sizeof(database),ODBC_INI))
        {
            insertErrorMsg(pdbc, SQL_HANDLE_DBC, SQL_IM002,
                SQL_ERROR, NULL, 0);
            rc = SQL_ERROR;
            releaseLock(SQL_HANDLE_DBC, pdbc);
            goto routine_exit;
        }
    }

    if (penv->pooling)
    {
        if (cbWideDSN > 0 || cbWideDSN == SQL_NTS)
            pdbc->dsn = STalloc(pWork);
        if (szWideUID)
        {
            ret = ConvertWCharToChar( szWideUID, cbWideUID, pWork, 
                OCFG_MAX_STRLEN, NULL, NULL, NULL);
             if (!SQL_SUCCEEDED(ret))
            {
                insertErrorMsg(pdbc, SQL_HANDLE_DBC, SQL_HY001,
                    SQL_ERROR, NULL, 0);
                rc = SQL_ERROR;
                releaseLock(SQL_HANDLE_DBC, pdbc);
                goto routine_exit;
            }
            if (cbWideUID > 0 || cbWideUID == SQL_NTS)
                pdbc->uid = STalloc(pWork);
        }
        if (szWidePWD)
        {
            ret = ConvertWCharToChar( szWidePWD, cbWidePWD, pWork, 
                OCFG_MAX_STRLEN, NULL, NULL, NULL);
            if (!SQL_SUCCEEDED(ret))
            {
                insertErrorMsg(pdbc, SQL_HANDLE_DBC, SQL_HY001,
                    SQL_ERROR, NULL, 0);
                rc = SQL_ERROR;
                releaseLock(SQL_HANDLE_DBC, pdbc);
                goto routine_exit;
            }
            if (cbWidePWD > 0 || cbWidePWD == SQL_NTS)
            {
                GChostname(hostname, sizeof(hostname));
                status = IIodbc_encode(pWork, hostname, opwd);
                pdbc->pwd = STalloc(opwd);
            }
        }
    }
    releaseLock(SQL_HANDLE_DBC, pdbc);

    if (!IIConnectW)
    {
        rc = IIodbc_connectPhase1(pdbc);
        if (rc == SQL_SUCCESS)
        {
            applyLock(SQL_HANDLE_DBC, pdbc);
            pdbc->hdr.state = C3;
            releaseLock(SQL_HANDLE_DBC, pdbc);
            rc = IIodbc_connectPhase2(pdbc);
            if (!SQL_SUCCEEDED(rc))
                goto routine_exit;
        }
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
                ("SQLConnectW: not calling driver, duplicate conn handle %p\n",
                    pdbc);
            goto routine_exit;
        }
        else if (ret == TRUE)
        {
            applyLock(SQL_HANDLE_DBC, pdbc);
            handle = pdbc->hdr.driverHandle;
            pdbc->hdr.driverHandle = pdbcMatch->hdr.driverHandle;
            pdbcMatch->hdr.driverHandle = pdbc->hdr.driverHandle;
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
                ("SQLConnectW: not calling driver, duplicate conn handle %p\n",
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
    rc = IIConnectW(
        pdbc->hdr.driverHandle,
        szWideDSN,
        cbWideDSN,
        szWideUID,
        cbWideUID,
        szWidePWD,
        cbWidePWD);

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
    ODBC_EXEC_TRACE(ODBC_EX_TRACE)("SQLConnectW exit: state is %s\n",
        cli_state[pdbc->hdr.state]);

    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLDescribColW - Get column information
** 
** Description: 
**              SQLDescribeColW returns the result descriptor-column name, 
**              type, column size, decimal digits, and nullability-for one 
**              column in the result set.
**
** Inputs:
**              hstmt - Statement handle.
**              icol - Column sequence.
**              cbColNameMax - Maximum length of szColName.
**
** Outputs: 
**              szColName - Column name.
**              pcbColName - Actual length of szColName.
**              pfSqlType - SQL type of the column.
**              pcbColDef - Precision.
**              pibScale - Scale.
**              pfNullable - Whether or not the column is nullable.
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

SQLRETURN SQL_API SQLDescribeColW(
    SQLHSTMT         hstmt,
    SQLUSMALLINT     icol,
    SQLWCHAR        *szWideColName,
    SQLSMALLINT      cbWideColNameMax,   /*   count of chars */
    SQLSMALLINT     *pcbWideColName,     /* ->count of chars */
    SQLSMALLINT     *pfSqlType,
    SQLULEN         *pcbColDef,          /* ->ColumnSize in chars */
    SQLSMALLINT     *pibScale,
    SQLSMALLINT     *pfNullable)
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    if (cbWideColNameMax <= 0)
        cbWideColNameMax = SQL_MAX_MESSAGE_LENGTH;

    rc =  IIDescribeColW( pstmt->hdr.driverHandle,
        icol,
        szWideColName,
        cbWideColNameMax,
        pcbWideColName,
        pfSqlType,
        pcbColDef,
        pibScale,
        pfNullable);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_STMT, pstmt);
    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLErrorW - Return error information.
** 
** Description: 
**              SQLErrorW returns the next error information in the error queue.
**
** Inputs:
**              penv - Environment block or NULL.
**              pdbc - Connection block or NULL.
**              pstmt - Statement block or NULL.
** 
** Outputs: 
**              szSqlState - Location to return SQLSTATE.
**              pfNativeError - Location to return native error code.
**              szErrorMsg - Location to return error message text.
**              cbErrorMsgMax - Length of error message buffer.
**              pcbErrorMsg - Length of error message returned.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_SUCCESS_WITH_INFO
**              SQL_NO_DATA
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

SQLRETURN SQL_API SQLErrorW(
    SQLHENV          henv,
    SQLHDBC          hdbc,
    SQLHSTMT         hstmt,
    SQLWCHAR        *szSqlState,
    SQLINTEGER      *pfNativeError,
    SQLWCHAR        *szErrorMsg,
    SQLSMALLINT      cbErrorMsgMax, /* ->count of chars */
    SQLSMALLINT     *pcbErrorMsg)   /* ->count of chars */
{
    pENV penv = henv;
    pDBC pdbc = hdbc;
    pSTMT pstmt = hstmt;
    char *msgFormat = "[CA][Ingres][Ingres ODBC CLI] %s";
    char msg[SQL_MAX_MESSAGE_LENGTH];
    IIODBC_ERROR *err;
    static RecNumber = 0;
    BOOL driverError = FALSE;
    RETCODE traceRet = 1;

    RecNumber++;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLErrorW(henv, hdbc, hstmt,
        szSqlState, pfNativeError, szErrorMsg, cbErrorMsgMax, pcbErrorMsg),
        traceRet);

    if (RecNumber > IIODBC_MAX_ERROR_ROWS)
    {
         ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_NO_DATA));
         return SQL_NO_DATA;
    }

    if (!henv && !hdbc && !hstmt)
    {
         ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_NO_DATA));
         return SQL_INVALID_HANDLE;
    }

    if (penv && validHandle(penv, SQL_HANDLE_ENV) != SQL_SUCCESS)
    {
         ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
         return SQL_INVALID_HANDLE;
    }

    if (pdbc && validHandle(pdbc, SQL_HANDLE_DBC) != SQL_SUCCESS)
    {
         ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
         return SQL_INVALID_HANDLE;
    }
    if (pstmt && validHandle(pstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
         ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
         return SQL_INVALID_HANDLE;
    } 

    if (penv)
    {
        err = &penv->errHdr.err[0];
        if (penv->errHdr.error_count && 
            err[RecNumber - 1].messageText[0])
        {
            if (penv->errHdr.error_count < RecNumber)
            {
                ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_NO_DATA));
                return SQL_NO_DATA;
            }
            if (szErrorMsg)
            {
                STprintf(msg, msgFormat, err[RecNumber - 1].messageText);
                ConvertCharToWChar( err[RecNumber - 1].messageText, SQL_NTS,
                    szErrorMsg, SQL_MAX_MESSAGE_LENGTH, pcbErrorMsg, NULL, NULL);
            }
            if (szSqlState)
                ConvertCharToWChar( err[RecNumber - 1].sqlState, SQL_NTS,
                    szSqlState, SQL_MAX_MESSAGE_LENGTH, NULL, NULL, NULL);
            if (pfNativeError)
                *pfNativeError = 0;
            RecNumber++;
            ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_SUCCESS));
            return SQL_SUCCESS;
        }
             
        if (penv->hdr.driverError)
            driverError = TRUE;
    }
    if (pdbc) 
    {        
        err = &pdbc->errHdr.err[0];
        if (pdbc->errHdr.error_count && 
            err[RecNumber - 1].messageText[0])
        {
            if (pdbc->errHdr.error_count < RecNumber)
            {
                ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_NO_DATA));
                return SQL_NO_DATA;
            }
            if (szErrorMsg)
            {
                STprintf(msg, msgFormat, err[RecNumber - 1].messageText);
                ConvertCharToWChar( err[RecNumber - 1].messageText, SQL_NTS,
                    szErrorMsg, SQL_MAX_MESSAGE_LENGTH, pcbErrorMsg, NULL, 
                        NULL);
            }
            if (szSqlState)
                ConvertCharToWChar( err[RecNumber - 1].sqlState, SQL_NTS,
                    szSqlState, SQL_MAX_MESSAGE_LENGTH, NULL, NULL, NULL);
            if (pfNativeError)
                *pfNativeError = 0;
            RecNumber++;
            ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_SUCCESS));
            return SQL_SUCCESS;
        }
        if (pdbc->hdr.driverError)
            driverError = TRUE;
    }

    if (pstmt) 
    {       
        err = &pstmt->errHdr.err[0];
        if (pstmt->errHdr.error_count && 
            err[RecNumber - 1].messageText[0])
        {
            if (pstmt->errHdr.error_count < RecNumber)
            {
                ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_NO_DATA));
                return SQL_NO_DATA;
            }
            if (szErrorMsg)
            {
                STprintf(msg, msgFormat, err[RecNumber - 1].messageText);
                ConvertCharToWChar( err[RecNumber - 1].messageText, SQL_NTS,
                    szErrorMsg, SQL_MAX_MESSAGE_LENGTH, pcbErrorMsg, NULL, NULL);
            }
            if (szSqlState)
                ConvertCharToWChar( err[RecNumber - 1].sqlState, SQL_NTS,
                    szSqlState, SQL_MAX_MESSAGE_LENGTH, NULL, NULL, NULL);
             if (pfNativeError)
                *pfNativeError = 0;
             RecNumber++;
             ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_SUCCESS));
             return SQL_SUCCESS;
        }

        if (pstmt->hdr.driverError)
            driverError = TRUE;
    }

    if (driverError)
    {
        return IIError(
            henv ? penv->hdr.driverHandle : NULL,
            hdbc ? pdbc->hdr.driverHandle : NULL,
            hstmt ? pstmt->hdr.driverHandle : NULL,
            szSqlState,
            pfNativeError,
            szErrorMsg,
            cbErrorMsgMax,
            pcbErrorMsg);
    }

    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_NO_DATA));
    return SQL_NO_DATA;

}

/*
** Name: 	SQLExecDirectW - Execute a statement without a prior prepare
** 
** Description: 
**              SQLExecDirectW executes a preparable statement, using the 
**              current values of the parameter marker variables if any 
**              parameters exist in the statement. 
**
** Inputs:
**              hstmt - Statement handle.
**              szWideSqlStr - Query string.
**              cbWideSqlStr - Length of query string.
**
**
** Outputs: 
**              None.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_SUCCESS_WITH_INFO
**              SQL_NEED_DATA
**              SQL_NO_DATA
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

SQLRETURN SQL_API SQLExecDirectW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szWideSqlStr,
    SQLINTEGER       cbWideSqlStr)     /*   count of chars */
{
    RETCODE rc, traceRet = 1;

    pSTMT pstmt = (pSTMT)hstmt;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLExecDirect(hstmt, szWideSqlStr,
         cbWideSqlStr), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    if (!szWideSqlStr)
    {
        insertErrorMsg(pstmt, SQL_HANDLE_STMT, SQL_HY009,
            SQL_ERROR, NULL, 0);
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_ERROR));
        return SQL_ERROR;
    }

    rc = IIExecDirectW( pstmt->hdr.driverHandle,
        szWideSqlStr,
        cbWideSqlStr);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_STMT, pstmt);
    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLGetConnectAttr - Get connection attribute.
** 
** Description: 
**              SQLGetConnectAttr returns the current setting of a 
**              connection attribute.
**
** Inputs:
**              hdbc - Connection handle.
**              fOption - Requested connection attribute.
**              BufferLength - Maximum attribute buffer length.
**
**
** Outputs: 
**              pvParamParameter - Attribute value.
**              pStringLength - Actual attribute buffer length.
**
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

SQLRETURN SQL_API SQLGetConnectAttrW(
    SQLHDBC          hdbc,
    SQLINTEGER       fAttribute,
    SQLPOINTER       rgbValue,
    SQLINTEGER       cbValueMax,    /*   count of bytes */
    SQLINTEGER      *pcbValue)      /* ->count of bytes */
{

    RETCODE rc, traceRet = 1;
    pDBC pdbc = (pDBC)hdbc;
    SQLUINTEGER *pvParam = (SQLUINTEGER *)rgbValue;
    SQLINTEGER   StringLength = sizeof(SQLUINTEGER); 

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLGetConnectAttr(hdbc,
         fAttribute, rgbValue, cbValueMax, pcbValue), traceRet);

    if (!hdbc)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    if ( !pdbc->hdr.driverHandle )
    {
        if (fAttribute == SQL_ATTR_LOGIN_TIMEOUT)
        {
            *pvParam = 15;
            *pcbValue = StringLength;
            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_SUCCESS));
            return SQL_SUCCESS;
        }
    }
    else
    {
        resetErrorBuff(pdbc, SQL_HANDLE_DBC);

        rc = IIGetConnectAttrW(
            pdbc->hdr.driverHandle,
            fAttribute,
            rgbValue,
            cbValueMax,
            pcbValue);
    }

    applyLock(SQL_HANDLE_DBC, pdbc);
    if (rc != SQL_SUCCESS)
    {
        pdbc->hdr.driverError = TRUE;
        pdbc->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_DBC, pdbc);

    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLGetCursorNameW - Get cursor name.
** 
** Description: 
**              SQLGetCursorNameW returns the current cursor name.
**
** Inputs:
**              hstmt - Statement handle.
**              cbCursorMax - Maximum cursor buffer length.
**
**
** Outputs: 
**              szCursor - Attribute value.
**              pcbCursor - Actual attribute buffer length.
**
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

SQLRETURN SQL_API SQLGetCursorNameW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szCursor,
    SQLSMALLINT      cbCursorMax,   /*   count of chars; bytes in SQLGetCursorNameA */
    SQLSMALLINT     *pcbCursor)     /* ->count of chars */
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;


    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLGetCursorName(pstmt, szCursor,
            cbCursorMax, pcbCursor), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc =  IIGetCursorNameW(
        pstmt->hdr.driverHandle,
        szCursor,
        cbCursorMax,
        pcbCursor);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
    }
    releaseLock(SQL_HANDLE_STMT, pstmt);
    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLSetDescFieldW - Set a descriptor field.
** 
** Description: 
**              SQLSetDescFieldW sets the value of a single field of a 
**              descriptor record.
**
** Inputs:
**              DescriptorHandle - Descriptor handle.
**              RecNumber - Descriptor record number.
**              FieldIdentifier - Descriptor field to be set.
**              rgbValue - Descriptor field buffer
**              cbValue - Length of descriptor field buffer.
**
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
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

SQLRETURN  SQL_API SQLSetDescFieldW(
    SQLHDESC         DescriptorHandle,
    SQLSMALLINT      RecNumber,
    SQLSMALLINT      FieldIdentifier,
    SQLPOINTER       rgbValue,
    SQLINTEGER       cbValue)     /* count of bytes */
{
    RETCODE rc, traceRet = 1;
    pDESC pdesc;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLSetDescFieldW(DescriptorHandle,
        RecNumber, FieldIdentifier, rgbValue, cbValue), traceRet);

    if (!DescriptorHandle)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    rc = IISetDescFieldW(DescriptorHandle,
       RecNumber,
       FieldIdentifier,
       rgbValue,
       cbValue);

    if (rc != SQL_SUCCESS )
    {
        pdesc = getDescHandle(DescriptorHandle);
        applyLock(SQL_HANDLE_DESC, pdesc);
        if (pdesc != NULL)
        {
            pdesc->hdr.driverError = TRUE;
            pdesc->errHdr.rc = rc;
        }
        releaseLock(SQL_HANDLE_DESC, pdesc);
    }

    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}

SQLRETURN SQL_API SQLGetDescFieldW(
    SQLHDESC         hdesc,
    SQLSMALLINT      iRecord,
    SQLSMALLINT      iField,
    SQLPOINTER       rgbValue,
    SQLINTEGER       cbValueMax,    /*   count of bytes */
    SQLINTEGER      *pcbValue)      /* ->count of bytes */
{
    RETCODE rc, traceRet = 1;
    pDESC pdesc;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLGetDescFieldW(hdesc, iRecord,
        iField, rgbValue, cbValueMax, pcbValue), traceRet);

    if (!hdesc)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE)); 
        return SQL_INVALID_HANDLE;
    }

    rc = IIGetDescFieldW(hdesc,
       iRecord,
       iField,
       rgbValue,
       cbValueMax,
       pcbValue);

    if (rc != SQL_SUCCESS )
    {
        pdesc = getDescHandle(hdesc);
        if (pdesc != NULL)
        {
            pdesc->hdr.driverError = TRUE;
            pdesc->errHdr.rc = rc;
        }
    }
    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLGetDescRecW - Return descriptor metadata
** 
** Description: 
**              SQLGetDescRecW returns the current settings or values of 
**              multiple fields of a descriptor record. The fields returned 
**              describe the name, data type, and storage of column or 
**              parameter data.
** Inputs:
**              DescriptorHandle - Descriptor handle.
**              RecNumber - Descriptor record number.
**              BufferLength - Maximum length of the Name buffer.
**
** Outputs: 
**              Name - SQL_DESC_NAME field buffer.
**              StringLengthPtr - Length of the Name Buffer.
**              TypePtr - SQL_DESC_TYPE field buffer.
**              SubTypePtr - SQL_DESC_DATETIME_INTERVAL_CODE field buffer.
**              LengthPtr - SQL_DESC_OCTET_LENGTH field buffer.
**              PrecisionPtr - SQL_DESC_PRECISION field buffer.
**              ScalePtr - SQL_DESC_SCALE field buffer.
**              NullablePtr - SQL_DESC_NULLABLE field buffer.
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

SQLRETURN SQL_API SQLGetDescRecW(
    SQLHDESC         hdesc,
    SQLSMALLINT      iRecord,
    SQLWCHAR        *szWideColName,
    SQLSMALLINT      cbWideColNameMax, /*   count of chars; bytes in SQLGetDescRecA */
    SQLSMALLINT     *pcbWideColName,   /* ->count of chars */
    SQLSMALLINT     *pfType,
    SQLSMALLINT     *pfSubType,
    SQLLEN          *pLength,
    SQLSMALLINT     *pPrecision, 
    SQLSMALLINT     *pScale,
    SQLSMALLINT     *pNullable)
{
    RETCODE rc, traceRet = 1;
    pDESC pdesc;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLGetDescRecW(hdesc, iRecord,
        szWideColName, cbWideColNameMax, pcbWideColName, pfType, pfSubType,
        pLength, pPrecision, pScale, pNullable), traceRet);

    if (!hdesc)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    rc = IIGetDescRecW(hdesc,
       iRecord,
	   szWideColName,
	   cbWideColNameMax,
	   pcbWideColName,
	   pfType,
	   pfSubType,
	   pLength,
	   pPrecision,
	   pScale,
	   pNullable);

    if (rc != SQL_SUCCESS )
    {
        pdesc = getDescHandle(hdesc);
        if (pdesc != NULL)
        {
            pdesc->hdr.driverError = TRUE;
            pdesc->errHdr.rc = rc;
        }
    }
    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLGetDiagFieldW - Get error diagnostic
** 
** Description: 
**              SQLGetDiagFieldW returns the current value of a field of a 
**              record of the diagnostic data structure (associated with a 
**              specified handle) that contains error, warning, and status 
**              information.
**
** Inputs:
**              HandleType - Handle type.
**              Handle - Env, dbc, stmt, or desc handle.
**              RecNumber - Record in the error queue to return.
**              DiagIdentifier - Indicates field diagnostic.
**              BufferLength - Max length of diagnostic buffer.
**
** Outputs: 
**              pDiagInfoParm - Diagnostic information buffer.
**              pStringLength - Length of diagnostic buffer.
**
**
** Returns: 
**              SQL_SUCCESS
**              SQL_SUCCESS_WITH_INFO
**              SQL_NO_DATA
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

SQLRETURN SQL_API SQLGetDiagFieldW(
    SQLSMALLINT      fHandleType,
    SQLHANDLE        Handle,
    SQLSMALLINT      iRecord,
    SQLSMALLINT      fDiagField,
    SQLPOINTER       rgbDiagInfo,
    SQLSMALLINT      cbDiagInfoMax,  /*   count of bytes */
    SQLSMALLINT     *pcbDiagInfo)    /* ->count of bytes */
{
    pENV penv = NULL;
    pDBC pdbc = NULL;
    pSTMT pstmt = NULL;
    pDESC pdesc = NULL;
    SQLHANDLE drvHandle = NULL;
    BOOL mgrError = FALSE;
    BOOL driverError = FALSE;
    i4 rowCount = 0;
    IIODBC_ERROR *err;
    RETCODE rc = SQL_SUCCESS, retCode = SQL_SUCCESS, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLGetDiagFieldW(fHandleType,
        Handle, iRecord, fDiagField, rgbDiagInfo, cbDiagInfoMax,
        pcbDiagInfo), traceRet);
    
    *(char *)rgbDiagInfo = '\0';
    *pcbDiagInfo = 0;
    if (iRecord > IIODBC_MAX_ERROR_ROWS)
        return SQL_NO_DATA;

    switch (fHandleType)
    {
    case SQL_HANDLE_ENV:
        penv = (pENV)Handle;
        if (penv && penv->hdr.type == SQL_HANDLE_ENV) 
        {        
            if (penv->hdr.driverError)
                driverError = TRUE;
            if (penv->errHdr.error_count)
                mgrError = TRUE;
            err = &penv->errHdr.err[0];
            drvHandle = (SQLHANDLE)penv->hdr.driverHandle;
            retCode = penv->errHdr.rc;
            rowCount = penv->errHdr.error_count;
        }
		else
            return SQL_INVALID_HANDLE;
        break;

    case SQL_HANDLE_DBC:
        pdbc = (pDBC)Handle;
        if (pdbc && pdbc->hdr.type == SQL_HANDLE_DBC) 
        {
            if (pdbc->hdr.driverError)
               driverError = TRUE;
            if (pdbc->errHdr.error_count)
                mgrError = TRUE;
            drvHandle = (SQLHANDLE)pdbc->hdr.driverHandle;
            err = &pdbc->errHdr.err[0];
            retCode = pdbc->errHdr.rc;
            rowCount = pdbc->errHdr.error_count;
        }
        else
            return SQL_INVALID_HANDLE;
        break;

    case SQL_HANDLE_STMT:
        pstmt = (pSTMT)Handle;

        if (pstmt && pstmt->hdr.type == SQL_HANDLE_STMT)
        {
            if (pstmt->hdr.driverError)
                driverError = TRUE;
            if (pstmt->errHdr.error_count)
                mgrError = TRUE;
            drvHandle = (SQLHANDLE)pstmt->hdr.driverHandle;
            err = &pstmt->errHdr.err[0];
            retCode = pstmt->errHdr.rc;
            rowCount = pstmt->errHdr.error_count;
        }
        else
             return SQL_INVALID_HANDLE;
        break;

    case SQL_HANDLE_DESC:
        if (Handle)
        {
            drvHandle = Handle;
            pdesc = getDescHandle(Handle);
            if (pdesc)
            {
                if (pdesc->hdr.driverError)
                    driverError = TRUE;

                if (pdesc->errHdr.error_count)
		            mgrError = TRUE;

		        retCode = pdesc->errHdr.rc;
                err = &pdesc->errHdr.err[0];
                rowCount = pdesc->errHdr.error_count;
            }
            else
                rc = IIGetDiagFieldW(
                    fHandleType,
                    Handle,
                    iRecord,
                    fDiagField,
                    rgbDiagInfo,
                    cbDiagInfoMax,
                    pcbDiagInfo);
        }
        else
        {
           ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, 
                SQL_INVALID_HANDLE));
            return SQL_INVALID_HANDLE;
        }
        break;
    }

    /*
    ** Return code diag is implemented solely in the Driver Manager.
    */
    if (fDiagField == SQL_DIAG_RETURNCODE)
    {
        *(RETCODE *)rgbDiagInfo = retCode;
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_SUCCESS));
        return SQL_SUCCESS;
    }

    if (mgrError)
    {
        switch (fDiagField)
		{
        case SQL_DIAG_NUMBER:
            break;

        case SQL_DIAG_SQLSTATE:
            STcopy((char *)err[iRecord - 1].sqlState,
                (char *)rgbDiagInfo);
            ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, 
                SQL_SUCCESS));
            return SQL_SUCCESS;
            break;

        case SQL_DIAG_CLASS_ORIGIN:
            if (err[iRecord - 1].sqlState[0] == 'I' &&
                err[iRecord - 1].sqlState[1] == 'M')
                STcopy("ODBC 3.0",(char *)rgbDiagInfo);
            else
                STcopy("ISO 9075",(char *)rgbDiagInfo);
            ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, 
                SQL_SUCCESS));
            return SQL_SUCCESS;
	    break;

        case SQL_DIAG_NATIVE:
                *(i4 *)rgbDiagInfo = 0;
                ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_SUCCESS));
                return SQL_SUCCESS;
            break;

        case SQL_DIAG_MESSAGE_TEXT:
            STcopy(err[iRecord -1].messageText,
                (char *)rgbDiagInfo);
            ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_SUCCESS));
            return SQL_SUCCESS;
            break;

        case SQL_DIAG_SUBCLASS_ORIGIN:

        {
            i2 k;
            BOOL supported = FALSE;
            static char *states[] = {"01S00","01S01","01S02","01S06","01S07",
               "07S01","08S01","21S01","21S02","25S01",
               "25S02","25S03","42S01","42S02","45S01",
               "42S12","42S21","42S22","HY095","HY097",
               "HY098","HY099","HY100","HY101","HY105",
               "HY107","HY109","HY110","HY111","HYT00","HYT01",
               "IM001","IM002","IM003","IM004","IM005",
               "IM006","IM007","IM008","IM010","IM011","IM012"
                                };
            for (k = 0; k < sizeof(states) / sizeof(states[0]); k++)
            {
                if (!STcompare(err[iRecord - 1].sqlState, states[k]))
                    supported = TRUE;
	    }
	    if (supported)
	       STcopy("ODBC 3.0",(char *)rgbDiagInfo);
            else
                STcopy("ISO 9075",(char *)rgbDiagInfo);
            ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_SUCCESS));
            return SQL_SUCCESS;

            break;
        }
        case SQL_DIAG_CURSOR_ROW_COUNT:
        case SQL_DIAG_DYNAMIC_FUNCTION :
        case SQL_DIAG_DYNAMIC_FUNCTION_CODE :
        case SQL_DIAG_COLUMN_NUMBER:
        case SQL_DIAG_CONNECTION_NAME:
        case SQL_DIAG_ROW_COUNT:
        case SQL_DIAG_ROW_NUMBER:
        case SQL_DIAG_SERVER_NAME:
 
            driverError = TRUE;
            break;
        }
    }

    if (driverError && drvHandle)
        rc = IIGetDiagFieldW (fHandleType, drvHandle, iRecord,
            fDiagField, rgbDiagInfo, cbDiagInfoMax,  
            pcbDiagInfo);

    if (fDiagField == SQL_DIAG_NUMBER)
    {
        if (driverError && drvHandle && rc == SQL_SUCCESS)
           rowCount += *(i4 *)rgbDiagInfo;

        *(i4 *)rgbDiagInfo = rowCount;
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_SUCCESS));
        return SQL_SUCCESS;
    }
    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLGetDiagRecW - Get error information
** 
** Description: 
**              SQLGetDiagRecW returns the current values of multiple fields 
**              of a diagnostic record that contains error, warning, and 
**              status information.
**
** Inputs:
**              HandleType - Handle type.
**              Handle - env, dbc, stmt, or desc handle.
**              RecNumber - Row number of the error message queue.
**              BufferLength - Maximum length of message text buffer.
**
** Outputs: 
**              pSqlStateParm - SQLSTATE buffer.
**              pNativeError - Native error number buffer.
**              pMessageTextParm - Message text buffer.
**              pTextLength - Lengthof message text buffer.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_SUCCESS_WITH_INFO
**              SQL_NO_DATA
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

SQLRETURN SQL_API SQLGetDiagRecW(
    SQLSMALLINT      HandleType,
    SQLHANDLE        Handle,
    SQLSMALLINT      RecNumber,
    SQLWCHAR        *pSqlStateParm,
    SQLINTEGER      *pNativeError,
    SQLWCHAR        *pMessageTextParm,
    SQLSMALLINT      BufferLength,  /*   count of chars */
    SQLSMALLINT     *pTextLength)    /* ->count of chars */
{
    pENV penv = NULL;
    pDBC pdbc = NULL;
    pSTMT pstmt = NULL;
    pDESC pdesc = NULL;
    SQLHANDLE drvHandle = NULL;
    BOOL driverError = FALSE;
    char msgFormat[26] = "[CA][Ingres ODBC CLI] %s";
    char msg[SQL_MAX_MESSAGE_LENGTH];
    IIODBC_ERROR *err;
    RETCODE traceRet = 1;

     
    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLGetDiagRecW(Handle, RecNumber,
        pSqlStateParm, pNativeError, pMessageTextParm, BufferLength, 
        pTextLength), traceRet);

    if (RecNumber > IIODBC_MAX_ERROR_ROWS)
    {
         ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
         return SQL_NO_DATA;
    }

    if (RecNumber < 1)
    {
         ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_ERROR));
         return SQL_ERROR;
    }

    switch (HandleType)
    {
    case SQL_HANDLE_ENV:
        penv = (pENV)Handle;
        if (penv && penv->hdr.type == SQL_HANDLE_ENV) 
        { 
            err = &penv->errHdr.err[0];
            if (penv->errHdr.error_count && 
                err[RecNumber - 1].messageText[0])
            {
                if (pMessageTextParm)
                {
                    STprintf(msg, msgFormat, err[RecNumber - 1].messageText); 
                    ConvertCharToWChar( err[RecNumber - 1].messageText, SQL_NTS,
                        pMessageTextParm, SQL_MAX_MESSAGE_LENGTH, 
                        pTextLength, NULL, NULL);
                }
                if (pSqlStateParm)
                {
                    ConvertCharToWChar( err[RecNumber - 1].sqlState, SQL_NTS,
                        pSqlStateParm, SQL_MAX_MESSAGE_LENGTH, NULL, NULL, NULL);
                }    
                if (pNativeError)
                    *pNativeError = 0;
                ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, 
                    SQL_SUCCESS));
                return SQL_SUCCESS;
            }

            if (penv->hdr.driverError)
               driverError = TRUE;
            drvHandle = (SQLHANDLE)penv->hdr.driverHandle;
        }
        else
        {
            ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, 
                SQL_INVALID_HANDLE));
            return SQL_INVALID_HANDLE;
        }

        break;

    case SQL_HANDLE_DBC:
        pdbc = (pDBC)Handle;
        if (pdbc && pdbc->hdr.type == SQL_HANDLE_DBC) 
        {        
            err = &pdbc->errHdr.err[0];
            if (pdbc->errHdr.error_count && 
                err[RecNumber - 1].messageText[0])
            {
                if (pMessageTextParm)
                {
                    STprintf(msg, msgFormat, err[RecNumber - 1].messageText); 
                    ConvertCharToWChar( err[RecNumber - 1].messageText, SQL_NTS,
                        pMessageTextParm, SQL_MAX_MESSAGE_LENGTH, 
                        pTextLength, NULL, NULL);
                }
                if (pSqlStateParm)
                {
                    ConvertCharToWChar( err[RecNumber - 1].sqlState, SQL_NTS,
                        pSqlStateParm, SQL_MAX_MESSAGE_LENGTH, NULL, NULL, NULL);
                }    
                if (pNativeError)
                    *pNativeError = 0;
                ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_SUCCESS));
                return SQL_SUCCESS;
            }

            if (pdbc->hdr.driverError)
               driverError = TRUE;
            drvHandle = (SQLHANDLE)pdbc->hdr.driverHandle;
        }
        else
        {
            ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, 
                SQL_INVALID_HANDLE));
            return SQL_INVALID_HANDLE;
        }

        break;

    case SQL_HANDLE_STMT:
        pstmt = (pSTMT)Handle;

        if (pstmt && pstmt->hdr.type == SQL_HANDLE_STMT) 
        {       
            err = &pstmt->errHdr.err[0];
            if (pstmt->errHdr.error_count && 
                err[RecNumber - 1].messageText[0])
            {
                if (pMessageTextParm)
                {
                    STprintf(msg, msgFormat, err[RecNumber - 1].messageText); 
                    ConvertCharToWChar( err[RecNumber - 1].messageText, SQL_NTS,
                        pMessageTextParm, SQL_MAX_MESSAGE_LENGTH, 
                        pTextLength, NULL, NULL);
                }
                if (pSqlStateParm)
                {
                    ConvertCharToWChar( err[RecNumber - 1].sqlState, SQL_NTS,
                        pSqlStateParm, SQL_MAX_MESSAGE_LENGTH, NULL, NULL, NULL);
                }    

                 if (pNativeError)
                    *pNativeError = 0;
                 ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, 
                     SQL_SUCCESS));
                 return SQL_SUCCESS;
            }

            if (pstmt->hdr.driverError)
               driverError = TRUE;
            drvHandle = (SQLHANDLE)pstmt->hdr.driverHandle;
        }
        else
        {
            ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, 
                SQL_INVALID_HANDLE));
            return SQL_INVALID_HANDLE;
        }
        break;

    case SQL_HANDLE_DESC:
    
        if (Handle) 
        {
            pdesc = getDescHandle(Handle);
            if (pdesc)
            {
                err = &pdesc->errHdr.err[0];
                if (pdesc->errHdr.error_count && 
                    err[RecNumber - 1].messageText[0])
                {
                    if (pMessageTextParm)
                    {
                        STprintf(msg, msgFormat, err[RecNumber - 1].messageText); 
                        ConvertCharToWChar( err[RecNumber - 1].messageText, SQL_NTS,
                            pMessageTextParm, SQL_MAX_MESSAGE_LENGTH, 
                            pTextLength, NULL, NULL);
                    }
                    if (pSqlStateParm)
                    {
                        ConvertCharToWChar( err[RecNumber - 1].sqlState, SQL_NTS,
                            pSqlStateParm, SQL_MAX_MESSAGE_LENGTH, NULL, NULL, NULL);
                    }    
    
                    if (pNativeError)
                        *pNativeError = 0;
                    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, 
                        SQL_SUCCESS));
                    return SQL_SUCCESS;
                } /* if (pdesc->errHdr.error_count && 
                    err[RecNumber - 1].messageText[0]) */
            } /* if (pdesc) */

            if (pdesc->hdr.driverError)
               driverError = TRUE;
            drvHandle = Handle;
        } /* if (Handle) */
        else
        {
            ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, 
                SQL_INVALID_HANDLE));
            return SQL_INVALID_HANDLE;
        }
       
        break;
    } /* switch(HandleType) */
 
    if (driverError)
        return IIGetDiagRec ( HandleType, drvHandle, RecNumber,
            pSqlStateParm, pNativeError, pMessageTextParm,
            BufferLength, pTextLength);

    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_ERROR));
    return SQL_ERROR;
}

/*
** Name: 	SQLPrepareW - Prepare a query
** 
** Description: 
**              SQLPrepareW prepares an SQL string for execution.
**
** Inputs:
**              hstmt - Statement handle.
**              szSqlStr - Query string.
**              cbSqlStr - Length of query string.
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
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

SQLRETURN SQL_API SQLPrepareW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szWideSqlStr,
    SQLINTEGER       cbWideSqlStr)   /*   count of chars */
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLPrepareW(hstmt, szWideSqlStr,
         cbWideSqlStr), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    if (!szWideSqlStr)
    {
        insertErrorMsg(pstmt, SQL_HANDLE_STMT, SQL_HY009,
            SQL_ERROR, NULL, 0);
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_ERROR));
        return SQL_ERROR;
    }

    rc = IIPrepareW( pstmt->hdr.driverHandle,
        szWideSqlStr,
        cbWideSqlStr);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_STMT, pstmt);
    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLSetConnectAttrW - Set connection attribute.
** 
** Description: 
**              SQLSetConnectAttrW sets attributes that govern aspects of 
**              connections.
**
** Inputs:
**              hdbc - Connection handle.
**              fOption - Requested connection attribute.
**              pValue - Attribute value.
**              StringLength - Attribute buffer length.
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
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

SQLRETURN SQL_API SQLSetConnectAttrW(
    SQLHDBC          hdbc,
    SQLINTEGER       fAttribute,
    SQLPOINTER       rgbValue,
    SQLINTEGER       cbValue)        /*   count of bytes */
{
    RETCODE rc, traceRet = 1;
    pDBC pdbc = (pDBC)hdbc;
    IISETCONNECTATTRW_PARAM *iiSetConnectAttrWParam;

    
    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLSetConnectAttrW(hdbc,
         fAttribute, rgbValue, cbValue), traceRet);

    if (!hdbc)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }
    /*
    **  If the driver isn't loaded yet, set a placeholder to do this 
    **  later.
    */
    if ( !IISetConnectAttrW )
    {
        iiSetConnectAttrWParam = 
            (IISETCONNECTATTRW_PARAM *)MEreqmem(0, 
            sizeof(IISETCONNECTATTRW_PARAM), TRUE, NULL);
        QUinsert((QUEUE *)iiSetConnectAttrWParam, &pdbc->setConnectAttr_q);
 		iiSetConnectAttrWParam->hdbc = hdbc;
		iiSetConnectAttrWParam->fAttribute = fAttribute;
        iiSetConnectAttrWParam->rgbValue = rgbValue;
		iiSetConnectAttrWParam->cbValue = cbValue;
		pdbc->setConnectAttrW_count++;

        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_SUCCESS));
 
        return SQL_SUCCESS;
    }
    else
    {
        resetErrorBuff( pdbc, SQL_HANDLE_DBC );
        rc = IISetConnectAttr(
            pdbc->hdr.driverHandle,
            fAttribute,
            rgbValue,
            cbValue);

        applyLock(SQL_HANDLE_DBC, pdbc);
        if (rc != SQL_SUCCESS)
        {
            pdbc->hdr.driverError = TRUE;
            pdbc->errHdr.rc = rc;
        }
        releaseLock(SQL_HANDLE_DBC, pdbc);
    }
    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}
/*
** Name: 	SQLSetCursorNameW - Set cursor name.
** 
** Description: 
**              SQLSetCursorNameW associates a cursor name with an active 
**              statement. 
**
** Inputs:
**              hstmt - Statement handle.
**              szCursorParameter - Cursor name.
**              cbCursor - length of cursor name buffer.
**
** Outputs: 
**
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

SQLRETURN SQL_API SQLSetCursorNameW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szCursor,
    SQLSMALLINT      cbCursor)       /*   count of chars */
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLSetCursorNameW(hstmt,
       szCursor, cbCursor), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));  
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc =  IISetCursorNameW(
        pstmt->hdr.driverHandle,
        szCursor,
        cbCursor);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_STMT, pstmt);
    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLColumnsW - Return list of columns in a table.
** 
** Description: 
**		SQLColumnsW returns the list of column names in specified 
**              tables.  The driver returns this information as a result set 
**              on the specified statement handle.
** 
** Inputs:
**              hstmt              Statement control block.
**              szTableQualifier   Ignored.
**              cbTableQualifier   Ignored.
**              szTableOwner       Owner (schema) name.
**              cbTableOwner       Length of owner name.
**              szTableName        Table name.
**              cbTableName        Length of table name.
**              szColumnName       Column name.
**              cbColumnName       Length of column name.
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
** 
**              Generates a result set.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

SQLRETURN SQL_API SQLColumnsW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szCatalogName,
    SQLSMALLINT      cbCatalogName,
    SQLWCHAR        *szSchemaName,
    SQLSMALLINT      cbSchemaName,
    SQLWCHAR        *szTableName,
    SQLSMALLINT      cbTableName,
    SQLWCHAR        *szColumnName,
    SQLSMALLINT      cbColumnName)
{
    pSTMT pstmt = (pSTMT)hstmt;
    RETCODE rc, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLColumnsW( hstmt, szCatalogName, 
        cbCatalogName, szSchemaName, cbSchemaName, szTableName, 
        cbTableName, szColumnName, cbColumnName), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc = IIColumnsW(pstmt->hdr.driverHandle,
        szCatalogName,
        cbCatalogName,
        szSchemaName,
        cbSchemaName,
        szTableName,
        cbTableName,
        szColumnName,
        cbColumnName);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_STMT, pstmt);
    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLGetConnectOptionW - Get connection attribute (ODBC 1.0)
** 
** Description: 
**              SQLGetConnectOptionW returns the current setting of the
**              connection attribute.
**
** Inputs:
**              hdbc - Connection handle.
**              fOption - Connection attribute.
**
** Outputs: 
**              pvParam - Setting of the connection attribute.
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

RETCODE SQL_API SQLGetConnectOptionW(
    SQLHDBC    hdbc,
    SQLUSMALLINT fOption,
    SQLPOINTER pvParam)
{
    RETCODE rc, traceRet = 1;
    pDBC pdbc = (pDBC)hdbc;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLGetConnectOptionW(hdbc, fOption,
        pvParam), traceRet);

    if (!hdbc)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hdbc, SQL_HANDLE_DBC);

    rc = IIGetConnectOptionW(
        pdbc->hdr.driverHandle,
        fOption,
        pvParam);

    applyLock(SQL_HANDLE_DBC, pdbc);
    if (rc != SQL_SUCCESS)
    {
        pdbc->hdr.driverError = TRUE;
        pdbc->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_DBC, pdbc);

    return rc;
}

/*
** Name: 	SQLGetInfoW - Get general information
** 
** Description: 
**              SQLGetInfoW returns general information about the driver and 
**              data source associated with a connection.
**
** Inputs:
**              hdbc - Connection handle.
**              fInfoType - Type of data to retrieve.
**              cbInfoValueMax - Maximum length of the info buffer.
**              pcbInfoValue - Actual length of the info buffer.
**              
**
** Outputs: 
**              rgbInfoValueParameter - Information buffer.
**
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

SQLRETURN SQL_API SQLGetInfoW(
    SQLHDBC          hdbc,
    SQLUSMALLINT     fInfoType,
    SQLPOINTER       rgbInfoValue,
    SQLSMALLINT      cbInfoValueMax, /*   count of bytes */
    SQLSMALLINT     *pcbInfoValue)   /* ->count of bytes */
{
    pDBC pdbc = (pDBC)hdbc;
    RETCODE rc,traceRet = 1;


    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLGetInfoW(hdbc, fInfoType,
        rgbInfoValue, cbInfoValueMax, pcbInfoValue), traceRet);

    if (validHandle(hdbc, SQL_HANDLE_DBC) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hdbc, SQL_HANDLE_DBC);

    rc = IIGetInfoW(pdbc->hdr.driverHandle,
        fInfoType,
        rgbInfoValue,
        cbInfoValueMax,
        pcbInfoValue);

    applyLock(SQL_HANDLE_DBC, pdbc);
    if (rc != SQL_SUCCESS)
    {
        pdbc->hdr.driverError = TRUE;
        pdbc->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_DBC, pdbc);
    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;

}

/*
** Name: 	SQLGetTypeInfoW - Get information about a data type.
** 
** Description: 
**              SQLGetTypeInfoW returns information about data types 
**              supported by the data source. The driver returns the 
**              information in the form of an SQL result set.
**
** Inputs:
**              hstmt - Statement handle.
**              fSqlType - Data type.
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
** 
**              Generates a result set.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

SQLRETURN SQL_API SQLGetTypeInfoW(
    SQLHSTMT         hstmt,
    SQLSMALLINT      DataType)
{
    pSTMT pstmt = (pSTMT)hstmt;
    RETCODE rc, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLGetTypeInfoW(hstmt, DataType),
         traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    applyLock(SQL_HANDLE_STMT, pstmt);
    rc = IIGetTypeInfoW(pstmt->hdr.driverHandle,
        DataType);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_STMT, pstmt);
    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_SUCCESS));
    return rc;
}

/*
** Name: 	SQLSetConnectOptionW - Set connection attribute (ODBC 1.0)
** 
** Description: 
**              SQLSetConnectOptionW sets attributes that govern aspects of 
**              connections.
** Inputs:
**              hdbc - Connection handle.
**              fOption - Connection attribute.
**              vParam - Setting of the connection attribute.
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
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

RETCODE SQL_API SQLSetConnectOptionW(
    SQLHDBC    hdbc,
    UWORD      fOption,
    SQLULEN    vParam)
{
    RETCODE rc, traceRet = 1;
    pDBC pdbc = (pDBC)hdbc;
    IISETCONNECTOPTIONW_PARAM *iiSetConnectOptionWParam;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLSetConnectOptionW(hdbc,
        fOption, vParam), traceRet);

    if (!hdbc)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }
    
    /*
    **  If the driver isn't loaded yet, set a placeholder to do this 
    **  later.
    */
    if ( !IISetConnectOptionW )
    {
        iiSetConnectOptionWParam = 
            (IISETCONNECTOPTIONW_PARAM *)MEreqmem(0, 
            sizeof(IISETCONNECTOPTIONW_PARAM), TRUE, NULL);
        QUinsert((QUEUE *)iiSetConnectOptionWParam, &pdbc->setConnectOption_q);
        iiSetConnectOptionWParam->ConnectionHandle = hdbc;
		iiSetConnectOptionWParam->Option = fOption;
		iiSetConnectOptionWParam->Value = vParam;
		pdbc->setConnectOptionW_count++;
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_SUCCESS));
        return SQL_SUCCESS;
    }
    else
    {
        resetErrorBuff(pdbc, SQL_HANDLE_DBC);

        rc = IISetConnectOptionW(
            pdbc->hdr.driverHandle,
            fOption,
            vParam);

        applyLock(SQL_HANDLE_DBC, pdbc);
        if (rc != SQL_SUCCESS)
        {
            pdbc->hdr.driverError = TRUE;
            pdbc->errHdr.rc = rc;
        }
        else if (fOption == SQL_ATTR_AUTOCOMMIT)
            pdbc->autocommit = TRUE;
        releaseLock(SQL_HANDLE_DBC, pdbc);
    }

    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLSpecialColumnsW - Return list of database procedures.
** 
** Description: 
**              SQLSpecialColumnsW retrieves the following information about 
**              columns within a specified table: 
**              1.  The optimal set of columns that uniquely identifies a 
**                  row in the table. 
**              2.  Columns that are automatically updated when any value 
**                  in the row is updated by a transaction. 
**
** Inputs:
**            hstmt              Statement control block.
**            fColType           Type of column to return.
**            szTableQualifier   Ignored.
**            cbTableQualifier   Ignored.
**            szTableOwner       Owner (schema) name.
**            cbTableOwner       Length of owner name.
**            szTableName        Table name.
**            cbTableName        Length of table name.
**            fScope             Minimum scope of rowid.
**            fNullable          Whether columns can be nullable.
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
** 
**              Generates a result set.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

SQLRETURN SQL_API SQLSpecialColumnsW(
    SQLHSTMT         hstmt,
    SQLUSMALLINT     fColType,
    SQLWCHAR        *szCatalogName,
    SQLSMALLINT      cbCatalogName,
    SQLWCHAR        *szSchemaName,
    SQLSMALLINT      cbSchemaName,
    SQLWCHAR        *szTableName,
    SQLSMALLINT      cbTableName,
    SQLUSMALLINT     fScope,
    SQLUSMALLINT     fNullable)
{
    pSTMT pstmt = (pSTMT)hstmt;
    RETCODE rc, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLSpecialColumnsW(hstmt, fColType,
        szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
        szTableName, cbTableName, fScope, fNullable), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
        return SQL_INVALID_HANDLE;

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc = IISpecialColumnsW(pstmt->hdr.driverHandle,
        fColType,
        szCatalogName,
        cbCatalogName,
        szSchemaName,
        cbSchemaName,
        szTableName,
        cbTableName,
        fScope,
        fNullable);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_STMT, pstmt);

    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;

}

/*
** Name: 	SQLStatisticsW - Return table statistics.
** 
** Description: 
**              SQLStatisticsW retrieves a list of statistics about a single 
**              table and the indexes associated with the table.
**
** Inputs:
**            hstmt              Statement control block.
**            szTableQualifier   Ignored.
**            cbTableQualifier   Ignored.
**            szTableOwner       Owner (schema) name.
**            cbTableOwner       Length of owner name.
**            szTableName        Table name.
**            cbTableName        Length of table name.
**            fUnique            Type of index.
**            fAccuracy          Importannce of cardinality.
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
** 
**              Generates a result set.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

SQLRETURN SQL_API SQLStatisticsW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szCatalogName,
    SQLSMALLINT      cbCatalogName,
    SQLWCHAR        *szSchemaName,
    SQLSMALLINT      cbSchemaName,
    SQLWCHAR        *szTableName,
    SQLSMALLINT      cbTableName,
    SQLUSMALLINT     fUnique,
    SQLUSMALLINT     fAccuracy)
{
    pSTMT pstmt = (pSTMT)hstmt;
    RETCODE rc, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLStatisticsW(hstmt,
         szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
         szTableName, cbTableName, fUnique, fAccuracy), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc = IIStatisticsW(pstmt->hdr.driverHandle,
        szCatalogName,
        cbCatalogName,
        szSchemaName,
        cbSchemaName,
        szTableName,
        cbTableName,
        fUnique,
        fAccuracy);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->errHdr.rc = rc;
        pstmt->hdr.driverError = TRUE;
    }
    releaseLock(SQL_HANDLE_STMT, pstmt);
    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLTablesW - Return table metadata.
** 
** Description: 
**              SQLTablesW returns the list of table, catalog, or schema 
**              names, and table types, stored in a specific data source.
**
** Inputs:
**            hstmt              statement control block.
**            szTableQualifier   qualifier name.
**            cbTableQualifier   length of qualifier name.
**            szTableOwner       owner (schema) name.
**            cbTableOwner       length of owner name.
**            szTableName        table name.
**            cbTableName        length of table name.
**            szTableType        table type.
**            cbTableType        length of table type.
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
** 
**              Generates a result set.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

SQLRETURN SQL_API SQLTablesW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szCatalogName,
    SQLSMALLINT      cbCatalogName,
    SQLWCHAR        *szSchemaName,
    SQLSMALLINT      cbSchemaName,
    SQLWCHAR        *szTableName,
    SQLSMALLINT      cbTableName,
    SQLWCHAR        *szTableType,
    SQLSMALLINT      cbTableType)
{
    pSTMT pstmt = (pSTMT)hstmt;
    RETCODE rc, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLTablesW( hstmt, szCatalogName,
        cbCatalogName, szSchemaName, cbSchemaName, szTableName, 
        cbTableName, szTableType, cbTableType), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc = IITablesW(pstmt->hdr.driverHandle,
       szCatalogName,
       cbCatalogName,
       szSchemaName,
       cbSchemaName,
       szTableName,
       cbTableName,
       szTableType,
       cbTableType);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_STMT, pstmt);
    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLDriverConnectW - Connect to database.
** 
** Description: 
**              SQLDriverConnectW connects to the database via a connection
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
**              If connection pooling is enables, may remove a 
**              connection handle from the pool queue and
**              free the connection handle.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
**   26-Feb-1007 (Ralph Loen) SIR 117786
**      If connection pooling is enabled: (1) store the connection attributes 
**      in the connection handle; (2) check the connection pool and use the
**      connection pool handle if a match is found.
*/ 

SQLRETURN SQL_API SQLDriverConnectW(
    SQLHDBC          hdbc,
    SQLHWND          hwnd,
    SQLWCHAR        *szWideConnStrIn,
    SQLSMALLINT      cbWideConnStrIn,     /*   count of chars; bytes in SQLDriverConnectA */
    SQLWCHAR        *szWideConnStrOut,
    SQLSMALLINT      cbWideConnStrOutMax, /*   count of chars; bytes in SQLDriverConnectA */
    SQLSMALLINT     *pcbWideConnStrOut,   /* ->count of chars */
    SQLUSMALLINT     fDriverCompletion)
{
    RETCODE rc = SQL_SUCCESS, ret, traceRet = 1;
    pDBC pdbc = (pDBC)hdbc, pdbcMatch=NULL;
    pENV penv = (pENV)pdbc->penv;
    SQLCHAR pWork[800];
    pDBC handle;
    
    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLDriverConnectW(hdbc,
        hwnd, szWideConnStrIn, cbWideConnStrIn, szWideConnStrOut,
        cbWideConnStrOutMax, pcbWideConnStrOut, fDriverCompletion), traceRet);

    if (!hdbc)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    if (pdbc->hdr.type != SQL_HANDLE_DBC)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    ODBC_EXEC_TRACE(ODBC_EX_TRACE)("SQLDriverConnectW entry: state is %s\n",
        cli_state[pdbc->hdr.state]);

    resetErrorBuff(hdbc, SQL_HANDLE_DBC);
    
    ret = ConvertWCharToChar( szWideConnStrIn, cbWideConnStrIn, (char *)pWork, 
        OCFG_MAX_STRLEN, NULL, NULL, NULL);

    if (!SQL_SUCCEEDED(ret))
    {
        applyLock(SQL_HANDLE_DBC, pdbc);
        insertErrorMsg(hdbc, SQL_HANDLE_DBC, SQL_HY001,
           SQL_ERROR, F_OD0015_IDS_WORKAREA, 0);
        releaseLock(SQL_HANDLE_DBC, pdbc);
        rc = SQL_ERROR;
        goto routine_exit;
    }

    if (penv->pooling)
    {
        pdbc->driverConnect = TRUE;
        IIodbc_formatConnHandle((char *)pWork, pdbc);
    }
    
    penv = pdbc->penv;
    if (!IIDriverConnectW)
    {
        rc = IIodbc_connectPhase1(pdbc);
        if (rc == SQL_SUCCESS)
        {
            applyLock(SQL_HANDLE_DBC, pdbc);
            pdbc->hdr.state = C3;
            releaseLock(SQL_HANDLE_DBC, pdbc);
            rc = IIodbc_connectPhase2(pdbc);
            if (!SQL_SUCCEEDED(rc))
                goto routine_exit;
        }
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
                ("SQLDriverConnectW: not calling driver, duplicate conn handle %p\n",
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
                ("SQLDriverConnectW: not calling driver, duplicate conn handle %p\n",
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

    rc = IIDriverConnectW(
        pdbc->hdr.driverHandle,
        hwnd,
        szWideConnStrIn,
        cbWideConnStrIn,
        szWideConnStrOut,
        cbWideConnStrOutMax,
        pcbWideConnStrOut,
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
    {
        pdbc->hdr.state = C4;
    }
    releaseLock(SQL_HANDLE_DBC, pdbc);
    
routine_exit:
    ODBC_EXEC_TRACE(ODBC_EX_TRACE)("SQLDriverConnectW exit: state is %s\n",
        cli_state[pdbc->hdr.state]);

    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLGetStmtAttrW - Get statement attribute.
** 
** Description: 
**              SQLGetStmtAttrW returns the current setting of a
**              statement attribute.
**
** Inputs:
**              StatementHandle - Statement handle.
**              Attribute - Requested statement attribute.
**              BufferLength - Maximum length of the attribute.
**
** Outputs: 
**              ValuePtr - Value of the attribute.
**              pStringLength - Actual length of the attribute.
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

SQLRETURN SQL_API SQLGetStmtAttrW(
    SQLHSTMT         hstmt,
    SQLINTEGER       fAttribute,
    SQLPOINTER       rgbValue,
    SQLINTEGER       cbValueMax,      /*   count of bytes */
    SQLINTEGER      *pcbValue)        /* ->count of bytes */
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLGetStmtAttrW(hstmt,
        fAttribute, rgbValue, cbValueMax, pcbValue), traceRet);

    if (validHandle( pstmt, SQL_HANDLE_STMT ) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    if (pstmt->hdr.driverHandle)
    {
        resetErrorBuff(pstmt, SQL_HANDLE_STMT);
        rc = IIGetStmtAttrW (
            pstmt->hdr.driverHandle,
            fAttribute,
            rgbValue,
            cbValueMax,
            pcbValue);
        if (rc != SQL_SUCCESS)
        {
            pstmt->hdr.driverError = TRUE;
            pstmt->errHdr.rc = rc;
        }
    }
    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLSetStmtAttr - Get statement attribute.
** 
** Description: 
**              SQLSetStmtAttr returns the current setting of an
**              statement attribute.
**
** Inputs:
**              hstmt - Environment handle.
**              Attribute - Requested statement attribute.
**              ValuePtr - Value of the attribute.
**              StringLength - Length of the attribute.
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
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

SQLRETURN SQL_API SQLSetStmtAttrW(
    SQLHSTMT         hstmt,
    SQLINTEGER       fAttribute,
    SQLPOINTER       rgbValue,
    SQLINTEGER       cbValueMax)
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;
    
    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLGetStmtAttrW(hstmt,
         fAttribute, rgbValue, cbValueMax), traceRet);

    if (validHandle(pstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    switch (fAttribute)
    {
    case SQL_ATTR_USE_BOOKMARKS: 
    case SQL_ATTR_CONCURRENCY: 
    case SQL_ATTR_CURSOR_TYPE:
    case SQL_ATTR_SIMULATE_CURSOR: 
        if (pstmt->hdr.state >= S3)
        {
            ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_ERROR));
            return SQL_ERROR;
        }
        break;
    }
     
    rc = IISetStmtAttrW(
        pstmt->hdr.driverHandle,
        fAttribute,
        rgbValue,
        cbValueMax);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_STMT, pstmt);

    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLNativeSqlW - Get native SQL query string.
** 
** Description: 
**              SQLNativeSqlW returns the SQL string as modified by the driver. 
**
** Inputs:
**              hdbc - Connection handle.
**              InStatementText - Application query text.
**              TextLength1 - Length of application query text.
**              BufferLength - Maximum length of modified query text buffer.
**
**
** Outputs: 
**              OutStatementText - Modified query text.
**              pcbValue - Actual length of modified query text buffer.
**
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

SQLRETURN SQL_API SQLNativeSqlW(
    SQLHDBC          hdbc,
    SQLWCHAR        *szWideSqlStrIn,
    SQLINTEGER       cbWideSqlStrIn,      /*   count of chars */
    SQLWCHAR        *szWideSqlStrOut,
    SQLINTEGER       cbWideSqlStrMax,     /*   count of chars; bytes in SQLNativeSqlA */
    SQLINTEGER      *pcbWideSqlStr)       /* ->count of chars */
{
    RETCODE rc, traceRet = 1;
    pDBC pdbc = (pDBC)hdbc;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLNativeSqlW(hdbc, szWideSqlStrIn,
       cbWideSqlStrIn, szWideSqlStrOut, cbWideSqlStrMax, pcbWideSqlStr), 
       traceRet);

    if (!cbWideSqlStrMax)
        cbWideSqlStrMax = SQL_MAX_MESSAGE_LENGTH;

    if (validHandle(hdbc, SQL_HANDLE_DBC) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hdbc, SQL_HANDLE_DBC);

    rc =  IINativeSqlW(
        pdbc->hdr.driverHandle,
        szWideSqlStrIn,
        cbWideSqlStrIn,
        szWideSqlStrOut,
        cbWideSqlStrMax,
        pcbWideSqlStr);

    applyLock(SQL_HANDLE_DBC, pdbc);
    if (rc != SQL_SUCCESS)
    {
        pdbc->hdr.driverError = TRUE;
        pdbc->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_DBC, pdbc);
    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLPrimaryKeysW - Return list of primary keys in a table.
** 
** Description: 
**              SQLPrimaryKeysW returns a list of primary keys in the 
**              specified table.
**
** Inputs:
**            hstmt              Statement control block.
**            szTableQualifier   Ignored.
**            cbTableQualifier   Ignored.
**            szTableOwner       Owner (schema) name.
**            cbTableOwner       Length of owner name.
**            szTableName        Table name.
**            cbTableName        Length of table name.
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
** 
**              Generates a result set.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

SQLRETURN SQL_API SQLPrimaryKeysW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szCatalogName,
    SQLSMALLINT      cbCatalogName,   /*   count of chars */
    SQLWCHAR        *szSchemaName,
    SQLSMALLINT      cbSchemaName,    /*   count of chars */
    SQLWCHAR        *szTableName,
    SQLSMALLINT      cbTableName)     /*   count of chars */
{
    pSTMT pstmt = (pSTMT)hstmt;
    RETCODE rc, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLPrimaryKeysW( hstmt, 
        szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, 
        szTableName, cbTableName), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc = IIPrimaryKeysW(pstmt->hdr.driverHandle,
       szCatalogName,    
       cbCatalogName,
       szSchemaName,        
       cbSchemaName,
       szTableName,         
       cbTableName);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_STMT, pstmt);
    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;

}

/*
** Name: 	SQLProcedureColumnsW - Return list of database procedures.
** 
** Description: 
**              SQLProcedureColumnsW returns the list of input and output 
**              parameters, as well as the columns that make up the result 
**              set for the specified procedures. The driver returns the 
**              information as a result set on the specified statement.
**
** Inputs:
**            hstmt             Statement control block.
**            szProcQualifier   Qualifier name.   (ignored)
**            cbProcQualifier   Length of qualifier name.
**            szProcOwner       Owner (schema) name search string.
**            cbProcOwner       Length of owner name.
**            szProcName        Table name search string.
**            cbProcName        Length of table name.
**            szColumnName      Column name search string.
**            cbColumnName      Length of column name.
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
** 
**              Generates a result set.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

SQLRETURN SQL_API SQLProcedureColumnsW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szCatalogName,
    SQLSMALLINT      cbCatalogName,
    SQLWCHAR        *szSchemaName,
    SQLSMALLINT      cbSchemaName,
    SQLWCHAR        *szProcName,
    SQLSMALLINT      cbProcName,
    SQLWCHAR        *szColumnName,
    SQLSMALLINT      cbColumnName)
{
    pSTMT pstmt = (pSTMT)hstmt;
    RETCODE rc, traceRet;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLProcedureColumnsW( hstmt, 
        szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, 
        szProcName, cbProcName, szColumnName, cbColumnName), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    { 
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE)); 
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc = IIProcedureColumnsW(pstmt->hdr.driverHandle,
        szCatalogName,    
        cbCatalogName,
        szSchemaName,        
        cbSchemaName,
        szProcName,         
        cbProcName,
        szColumnName,       
        cbColumnName);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_STMT, pstmt);
    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;

}

/*
** Name: 	SQLProceduresW - Return list of database procedures.
** 
** Description: 
**              Returns a list of database procedures in a data source.
**
** Inputs:
**            hstmt             Statement control block.
**            szProcQualifier   Qualifier name.   (ignored)
**            cbProcQualifier   Length of qualifier name.
**            szProcOwner       Owner (schema) name search string.
**            cbProcOwner       Length of owner name.
**            szProcName        Table name search string.
**            cbProcName        Length of table name.
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
** 
**              Generates a result set.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

SQLRETURN SQL_API SQLProceduresW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szCatalogName,
    SQLSMALLINT      cbCatalogName,   /*   count of chars; bytes in SQLProceduresA */
    SQLWCHAR        *szSchemaName,
    SQLSMALLINT      cbSchemaName,    /*   count of chars; bytes in SQLProceduresA */
    SQLWCHAR        *szProcName,
    SQLSMALLINT      cbProcName)      /*   count of chars; bytes in SQLProceduresA */
{
    pSTMT pstmt = (pSTMT)hstmt;
    RETCODE rc, traceRet;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLProceduresW( hstmt,
        szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,  szProcName,
        cbProcName), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc = IIProceduresW(pstmt->hdr.driverHandle,
        szCatalogName,    
        cbCatalogName,
        szSchemaName,        
        cbSchemaName,
        szProcName,         
        cbProcName);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_STMT, pstmt);
    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
    return rc;
}

/*
** Name: 	SQLForeignKeysW - Return list of foreign keys in a table.
** 
** Description: 
**              Returns a list of foreign keys in the specified table 
**              (columns in the specified table that refer to primary keys 
**              in other tables),
**                             - or -
**              returns a list of foreign keys in other tables that refer to 
**              the primary key in the specified table.
**
** Inputs:
**              hstmt                Statement control block.
**              szPkTableQualifier   Ignored.
**              cbPkTableQualifier   Ignored.
**              szPkTableOwner       Primary key owner (schema) name.
**              cbPkTableOwner       Length of primary key owner name.
**              szPkTableName        Primary key table name.
**              cbPkTableName        Length of primary key table name.
**              szFkTableQualifier   Ignored.
**              cbFkTableQualifier   Ignored.
**              szFkTableOwner       Foreign key owner (schema) name.
**              cbFkTableOwner       Length of foreign key owner name.
**              szFkTableName        Foreign key table name.
**              cbFkTableName        Length of foreign key table name.
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
** 
**              Generates a result set.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

SQLRETURN SQL_API SQLForeignKeysW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szPkCatalogName,
    SQLSMALLINT      cbPkCatalogName, /*   count of chars; bytes in SQLForeignKe
ysA */
    SQLWCHAR        *szPkSchemaName,
    SQLSMALLINT      cbPkSchemaName,  /*   count of chars; bytes in SQLForeignKe
ysA */
    SQLWCHAR        *szPkTableName,
    SQLSMALLINT      cbPkTableName,
    SQLWCHAR        *szFkCatalogName,
    SQLSMALLINT      cbFkCatalogName,
    SQLWCHAR        *szFkSchemaName,
    SQLSMALLINT      cbFkSchemaName,
    SQLWCHAR        *szFkTableName,
    SQLSMALLINT      cbFkTableName)
{
    pSTMT pstmt = (pSTMT)hstmt;
    RETCODE rc, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLForeignKeysW(hstmt, 
        szPkCatalogName, cbPkCatalogName, szPkSchemaName, cbPkSchemaName,
        szPkTableName, cbPkTableName, szFkCatalogName, cbFkCatalogName,
        szFkSchemaName, cbFkSchemaName, szFkTableName, cbFkTableName), 
        traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc = IIForeignKeysW(pstmt->hdr.driverHandle,
       szPkTableName,    
       cbPkTableName,
       szPkCatalogName,        
       cbPkCatalogName,
       szPkSchemaName,         
       cbPkSchemaName,
       szFkTableName,         
       cbFkTableName);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_STMT, pstmt);
    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;

}

/*
** Name: 	SQLTablePrivilegesW - Return table columns and privileges.
** 
** Description: 
**              SQLTablePrivilegesW returns a list of tables and the 
**              privileges associated with each table. 
** 
** Inputs:
**              hstmt                   StatementHandle.
**              szCatalogName           Catalog name.
**              cbCatalogName           Length of szCatalogName. 
**              szSchemaName            Schema name.
**              cbSchemaName            Length of szSchemaName. 
**              szTableName             Table name. 
**              cbTableName             Length of TableName. 
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
** 
**              Generates a result set.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

SQLRETURN SQL_API SQLTablePrivilegesW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szCatalogName,
    SQLSMALLINT      cbCatalogName,
    SQLWCHAR        *szSchemaName,
    SQLSMALLINT      cbSchemaName,
    SQLWCHAR        *szTableName,
    SQLSMALLINT      cbTableName)
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLTablePrivilegesW(hstmt,
        szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
        szTableName, cbTableName), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc =  IITablePrivilegesW(
        pstmt->hdr.driverHandle,
        szCatalogName,
        cbCatalogName,
        szSchemaName,
        cbSchemaName,
        szTableName,
        cbTableName);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_STMT, pstmt);
    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}

SQLRETURN SQL_API SQLColumnPrivilegesW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szCatalogName,
    SQLSMALLINT      cbCatalogName,
    SQLWCHAR        *szSchemaName,
    SQLSMALLINT      cbSchemaName,
    SQLWCHAR        *szTableName,
    SQLSMALLINT      cbTableName,
    SQLWCHAR        *szColumnName,
    SQLSMALLINT      cbColumnName)
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLColumnPrivilegesW(hstmt,
        szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
        szTableName, cbTableName, szColumnName, cbColumnName), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc =  IIColumnPrivilegesW(
        pstmt->hdr.driverHandle,
        szCatalogName,
        cbCatalogName,
        szSchemaName,
        cbSchemaName,
        szTableName,
        cbTableName,
        szColumnName,
        cbColumnName);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_STMT, pstmt);
    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLDriversW - Return driver attributes
** 
** Description: 
**              SQLDriversW lists driver descriptions and driver 
**              attribute keywords.  Currently unsupported.
**
** Inputs:
**              henv - Environment handle.
**              fDirection - Fetch direction.
**              cbDriverDescMax - Max length of driver description buffer.
**              cbDrvrAttrMax - Max length of driver attribute buffer.
**
** Outputs: 
**              szDriverDesc - Driver description.
**              pcbDriverDesc - Actual length of driver description buffer.
**              szDriverAttributes - Driver attribute value pairs.
**              pcbDrvrAttr - Actual length of driver attribute buffer.
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

SQLRETURN SQL_API SQLDriversW(
    SQLHENV          henv,
    SQLUSMALLINT     fDirection,
    SQLWCHAR        *szDriverDesc,
    SQLSMALLINT      cbDriverDescMax,
    SQLSMALLINT     *pcbDriverDesc,
    SQLWCHAR        *szDriverAttributes,
    SQLSMALLINT      cbDrvrAttrMax,
    SQLSMALLINT     *pcbDrvrAttr)
{
    pENV penv = (pENV)henv;
    RETCODE traceRet =  1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLDriversW(henv, szDriverDesc,
         cbDriverDescMax, pcbDriverDesc, szDriverAttributes, cbDrvrAttrMax,
         pcbDrvrAttr), traceRet);

    if (validHandle(henv, SQL_HANDLE_ENV) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(henv, SQL_HANDLE_ENV);

    insertErrorMsg(henv, SQL_HANDLE_ENV, SQL_IM001,
        SQL_ERROR, NULL, 0);

    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_ERROR));
    return SQL_ERROR;
}

/*
** Name: 	SQLBrowseConnectW - Perform connection attribute discovery
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
*/ 

SQLRETURN SQL_API SQLBrowseConnectW(
    SQLHDBC            hdbc,
    SQLWCHAR        *szConnStrIn,
    SQLSMALLINT        cbConnStrIn,
    SQLWCHAR        *szConnStrOut,
    SQLSMALLINT        cbConnStrOutMax,
    SQLSMALLINT    *pcbConnStrOut)
{
    pDBC pdbc = (pDBC)hdbc;
    RETCODE rc = SQL_SUCCESS, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLBrowseConnectW(hdbc, szConnStrIn, 
         cbConnStrIn, szConnStrOut, cbConnStrOutMax, pcbConnStrOut),
         traceRet);

    if (!hdbc)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        rc = SQL_INVALID_HANDLE;
        goto exit_routine;
    }

    if (pdbc->hdr.type != SQL_HANDLE_DBC)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        rc = SQL_INVALID_HANDLE;
        goto exit_routine;
    }

    resetErrorBuff(hdbc, SQL_HANDLE_DBC);

    rc = IIodbc_connectPhase1(pdbc);
    if (rc == SQL_SUCCESS)
    {
        pdbc->hdr.state = C3;
        rc = IIodbc_connectPhase2(pdbc);
        if (SQL_SUCCEEDED(rc))
        {
            rc = IIBrowseConnectW(pdbc->hdr.driverHandle,
                szConnStrIn,
                cbConnStrIn,
                szConnStrOut,
                cbConnStrOutMax,
                pcbConnStrOut);
        }
    }
    applyLock(SQL_HANDLE_DBC, pdbc);
    if (rc != SQL_SUCCESS)
    {
        pdbc->hdr.state = C2;
        pdbc->hdr.driverError = TRUE;
        pdbc->errHdr.rc = rc;
    }
    else if (SQL_SUCCEEDED(rc))
        pdbc->hdr.state = C4;

    releaseLock(SQL_HANDLE_DBC, pdbc);

exit_routine:
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_ERROR));
    return rc;
}

/*
**  ConvertCharToWChar
**
**  Convert a character string to wide (Unicode) string, possibly truncated.
**
**  On entry: szValue      -->ANSI string to convert from.
**            cbValue       = length of input buffer or SQL_NTS if null-terminated.
**            rgbWideValue -->where to return wide (Unicode) string.
**            cbWideValueMax= length (in SQLWCHAR chars) of output buffer.
**            pcbValue      = where to return length (in SQLWCHAR chars) of string
**                            (not counting the null-termination character).
**            pdwValue      = where to return length (in SQLWCHAR chars) of string
**                            (not counting the null-termination character).
**            plenValue     = where to return length (in SQLWCHAR chars) of string
**                            (not counting the null-termination character).
**
**  On exit:  Data is copied to buffer, possibly truncated, null terminated.
**
**  Returns:  SQL_SUCCESS           String returned successfully.
**            SQL_ERROR             Unsuccessful conversion.   
**            SQL_SUCCESS_WITH_INFO String truncation.   
**  History:   
**   26-Feb-2007 (Ralph Loen) SIR 117786   
**      Created from common!odbc!driver unicode.c. 
**   07-Oct-2010 (Ralph Loen) Bug 124570
**      Fill error message block upon errors.
*/
RETCODE ConvertCharToWChar(
    char     * szValue,
    SQLINTEGER     cbValue,
    SQLWCHAR * rgbWideValue,
    SQLINTEGER     cbWideValueMax,
    SWORD    * pcbValue,
    SDWORD   * pdwValue,
    SQLINTEGER   * plenValue)
{
    SQLWCHAR *szWideValue = NULL;
    SQLINTEGER     cbWideValue = 0;
    RETCODE    rc    = SQL_SUCCESS;
    IIAPI_CONVERTPARM   cv;
    BOOL isUcs4 = (sizeof(SQLWCHAR) == 4 ? TRUE : FALSE);
    i2 *ucs2buf = NULL;
    i2 nt = 0;
    i4 nt4 = 0;

    if (cbValue == SQL_NTS)              /* if length NTS, get length */
    {
        if (szValue != NULL)
            cbValue = (SQLINTEGER)STlength(szValue);
        else
            cbValue = 0;
    }

    if (szValue == NULL  ||  cbValue <= 0)  /* if no input string, */
                                            /* return 0 now*/
    {
        /* 
        ** Room in buffer for null-term? 
        */
        if (rgbWideValue  &&  cbWideValueMax>0) 
            *rgbWideValue = 0;
        if (pcbValue)
            *pcbValue = 0;
        if (pdwValue)
            *pdwValue = 0;
        if (plenValue)
            *plenValue = 0;
        return SQL_SUCCESS;
    }

    if (isUcs4)       
    {
        cbWideValue = cbValue * 2;
        ucs2buf = (i2 *)MEreqmem(0, cbWideValue+8, TRUE, NULL);
        if (ucs2buf == NULL)  /* no memory!? */
            return SQL_ERROR;
    }
    else
    {
        cbWideValue = cbValue * sizeof(SQLWCHAR);
        szWideValue = (SQLWCHAR *)MEreqmem(0, cbWideValue+8, TRUE, NULL);
        if (szWideValue == NULL)  /* no memory!? */
            return SQL_ERROR;
    }

    /* szWideValue -> target buffer to hold Unicode string
    cbWideValue  = target buffer length in bytes */
    
    cv.cv_srcDesc.ds_dataType   = IIAPI_CHA_TYPE;
    cv.cv_srcDesc.ds_nullable   = FALSE;
    cv.cv_srcDesc.ds_length     = (II_UINT2) cbValue;
    cv.cv_srcDesc.ds_precision  = 0;
    cv.cv_srcDesc.ds_scale      = 0;
    cv.cv_srcDesc.ds_columnType = IIAPI_COL_TUPLE;
    cv.cv_srcDesc.ds_columnName = NULL;
    cv.cv_srcValue.dv_null      = FALSE;
    cv.cv_srcValue.dv_length    = (II_UINT2) cbValue;
    cv.cv_srcValue.dv_value     = szValue;

    cv.cv_dstDesc.ds_dataType   = IIAPI_NCHA_TYPE;
    cv.cv_dstDesc.ds_nullable   = FALSE;
    cv.cv_dstDesc.ds_length     = (II_UINT2)cbWideValue;
    cv.cv_dstDesc.ds_precision  = 0;
    cv.cv_dstDesc.ds_scale      = 0;
    cv.cv_dstDesc.ds_columnType = IIAPI_COL_TUPLE;
    cv.cv_dstDesc.ds_columnName = NULL;
    cv.cv_dstValue.dv_null      = FALSE;
    cv.cv_dstValue.dv_length    = (II_UINT2)cbWideValue;
    if (isUcs4)
        cv.cv_dstValue.dv_value     = ucs2buf;
    else
        cv.cv_dstValue.dv_value     = szWideValue;

    IIapi_convertData(&cv);  
    if (cv.cv_status != IIAPI_ST_SUCCESS)
    {
        if (ucs2buf)
            MEfree((PTR)ucs2buf);
        if (szWideValue)
            MEfree((PTR)szWideValue);
        return SQL_ERROR;
    }

    /* 
    ** szWideValue -> Unicode string
    ** cbWideValue  = Unicode string length in characters 
    */

    if (pcbValue)   /* if SWORD destination count field present */
       *pcbValue = (SWORD)cbValue;
                                 /* return length in SQLWCHAR chars 
                                    excluding null-term char*/
    if (pdwValue)   /* if SDWORD destination count field present */
       *pdwValue = (SDWORD)cbValue;
                                 /* return length in SQLWCHAR chars 
                                    excluding null-term char*/
    if (plenValue)  /* if SQLINTEGER destination count field present */
       *plenValue = cbValue;
                                 /* return length in SQLWCHAR chars 
                                    excluding null-term char*/

    if (rgbWideValue)   /* if destination buffer present */
    {
        if (cbValue < cbWideValueMax) /* fits well into the target buffer*/
        {
            if (isUcs4)
            {
                ConvertUCS2ToUCS4(ucs2buf, rgbWideValue, cbValue);
                /* Null terminate. */
                I4ASSIGN_MACRO(nt4,*(rgbWideValue+cbValue));
            }
            else
            {
                memcpy (rgbWideValue, szWideValue, cbWideValue);
                *(rgbWideValue+cbValue) = nt;  /* null terminate */
            }
        }
        else
        {   /* need to truncate some */
            if (cbWideValueMax > 0)
            {
                cbValue = cbWideValueMax - 1;
                if (isUcs4)
                {
                    ConvertUCS2ToUCS4(ucs2buf, rgbWideValue, cbValue);
                    /* Null terminate. */
                    I4ASSIGN_MACRO(nt4,*(rgbWideValue+cbValue));
                }
                else
                {
                    memcpy (rgbWideValue, szWideValue, 
                        cbValue*sizeof(SQLWCHAR));
                    *(rgbWideValue + cbValue) = nt;
                }
            }
            rc = SQL_SUCCESS_WITH_INFO;
        }
    }
    if (szWideValue)
        MEfree((PTR)szWideValue);      /* free work area for work string */

    return rc;
}


/*
**  ConvertWCharToChar
**
**  Convert a wide (Unicode) string to character string, possibly truncated.
**
**  On entry: szWideValue  -->Wide (Unicode) string to convert.
**            cbWideValue   = length in char of input buffer
**                               or SQL_NTS if null-terminated.
**            rgbValue     -->where to return null-terminated ANSI string.
**            cbValueMax    = length of output buffer.
**            pcbValue      = where to return length (SWORD) of ANSI string
**                            (not couting the null-termination character).
**            pdwValue      = where to return length (SDWORD) of ANSI string
**                            (not couting the null-termination character).
**            plenValue     = where to return length (SQLINTEGER) of ANSI string
**                            (not couting the null-termination character).
**
**  On exit:  Data is copied to buffer, possibly truncated, null terminated.
**
**  Returns:  SQL_SUCCESS           String returned successfully.
**            SQL_ERROR             Unsuccessful conversion.   
**            SQL_SUCCESS_WITH_INFO String truncation.   
**  History:   
**   26-Feb-2007 (Ralph Loen) SIR 117786   
**      Created from common!odbc!driver unicode.c. 
**   07-Oct-2010 (Ralph Loen) Bug 124570
**      Fill error message block upon errors.
*/
RETCODE ConvertWCharToChar(
    SQLWCHAR * szWideValue,
    SQLINTEGER     cbWideValue,
    char     * rgbValue,
    SQLINTEGER     cbValueMax,
    SWORD    * pcbValue,
    SDWORD   * pdwValue,
    SQLINTEGER   * plenValue)
{
    u_i2     cbValue;
    char     * szValue = NULL, *szData = NULL;
    RETCODE    rc = SQL_SUCCESS;
    IIAPI_CONVERTPARM   cv;
    BOOL isUcs4 = (sizeof(SQLWCHAR) == 4 ? TRUE : FALSE);
    u_i2 *ucs2buf = NULL;
 
    if (cbWideValue == SQL_NTS)              /* if length NTS, get length */
    {
        if (szWideValue)
            cbWideValue = (SQLINTEGER)wcslen(szWideValue); /* length in chars */
        else
            cbWideValue = 0;
    }

    if (szWideValue == NULL  ||  rgbValue == NULL  ||
        cbWideValue <= 0)  /* if no input string, return 0 now*/
    {
        if (rgbValue  &&  cbValueMax>0)  /* room in buffer? */
            *rgbValue = '\0';
        if (pcbValue)
            *pcbValue = 0;
        if (pdwValue)
            *pdwValue = 0;
        if (plenValue)
            *plenValue = 0;
        return SQL_SUCCESS;
    }

    if (isUcs4)
    {
        ucs2buf = (u_i2 *)MEreqmem(0, ((cbWideValue+1)*sizeof(u_i2)), 
            TRUE, NULL);
        if (ucs2buf == NULL)  /* no memory!? */
            return SQL_ERROR;

        ConvertUCS4ToUCS2((u_i4*)szWideValue, (u_i2*)ucs2buf, cbWideValue);
    }

    cbWideValue *= sizeof(i2); 

    szValue = MEreqmem(0, cbValueMax+2, TRUE, NULL);

    if (szValue == NULL)  /* no memory!? */
        return SQL_ERROR;

    /* 
    ** szValue -> target buffer to hold character string
    ** cbValue  = target buffer length in bytes 
    */

    if (cbWideValue)     /* convert the Unicode string to character */
    {
        cv.cv_srcDesc.ds_dataType   = IIAPI_NCHA_TYPE;
        cv.cv_srcDesc.ds_nullable   = FALSE;
        cv.cv_srcDesc.ds_length     = (II_UINT2) cbWideValue;
        cv.cv_srcDesc.ds_precision  = 0;
        cv.cv_srcDesc.ds_scale      = 0;
        cv.cv_srcDesc.ds_columnType = IIAPI_COL_TUPLE;
        cv.cv_srcDesc.ds_columnName = NULL;
        cv.cv_srcValue.dv_null      = FALSE;
        if (isUcs4)
            cv.cv_srcValue.dv_value     = ucs2buf;
        else
            cv.cv_srcValue.dv_value     = szWideValue;
        cv.cv_srcValue.dv_length    = (II_UINT2) cbWideValue;
        cv.cv_dstDesc.ds_dataType   = IIAPI_VCH_TYPE;
        cv.cv_dstDesc.ds_nullable   = FALSE;
        cv.cv_dstDesc.ds_length     = (II_UINT2)cbValueMax+2;
        cv.cv_dstDesc.ds_precision  = 0;
        cv.cv_dstDesc.ds_scale      = 0;
        cv.cv_dstDesc.ds_columnType = IIAPI_COL_TUPLE;
        cv.cv_dstDesc.ds_columnName = NULL;
        cv.cv_dstValue.dv_null      = FALSE;
        cv.cv_dstValue.dv_length    = (II_UINT2)cbValueMax+2;
        cv.cv_dstValue.dv_value     = szValue;

        IIapi_convertData(&cv);  
        if (cv.cv_status != IIAPI_ST_SUCCESS)
        {
            if (szValue)
                MEfree((PTR)szValue);
            if (ucs2buf)
                MEfree((PTR)ucs2buf);
            return SQL_ERROR;
        }
    }

    cbValue = *(u_i2 *)szValue;
    szData = szValue + sizeof(u_i2);
    
    /* 
    ** szData -> character string
    ** cbValue  = character string length in bytes 
    */

    if (pcbValue)   /* if destination count field (SWORD) present */
       *pcbValue = (SWORD)cbValue;
                    /* number of character excluding null-term char*/
    if (pdwValue)   /* if destination count field (SDWORD) present */
       *pdwValue = (SDWORD)cbValue;
                    /* number of character excluding null-term char*/
    if (plenValue)  /* if destination count field (SQLINTEGER) present */
       *plenValue = cbValue;
                    /* number of character excluding null-term char*/

    if (rgbValue)   /* if destination buffer present */
    {
        if (cbValue < cbValueMax) /* fits well into the target buffer*/
        {
            memcpy (rgbValue, szData, cbValue);
            *(rgbValue+cbValue)=0;  /* null terminate */
        }
        else
        {   /* need to truncate some */
            if (cbValueMax > 0)
            {
                cbValue = (u_i2)cbValueMax - 1;
                memcpy (rgbValue, szData, cbValue);
                *(rgbValue + cbValue) = '\0';
            }
            rc = SQL_SUCCESS_WITH_INFO;
        }
    }

    if (szValue)
        MEfree((PTR)szValue); /* free work area for work string */

    if (ucs2buf)
        MEfree((PTR)ucs2buf);

    return rc;
}

/*
**  ConvertUCS2ToUCS4
**
**  Convert a character string to wide (Unicode) string, possibly truncated.
**
**  On entry: p2  -->UCS2 Unicode string
**            p4  -->UCS4 Unicode string
**            len  = length of string in characters.
**
**  On exit:  Data is copied to buffer, possibly truncated, null terminated.
**
**  Returns:  SQL_SUCCESS           String returned successfully.
*/
RETCODE ConvertUCS2ToUCS4(
    u_i2     * p2,
    u_i4     * p4,
    SQLINTEGER     len)
{
    SQLINTEGER     i;
    u_i4       sizeofSQLWCHAR = sizeof(SQLWCHAR);
    u_i2       tmp2;
    u_i4       tmp4;

    if (sizeofSQLWCHAR != 4)  /* return if not needed */
        return SQL_SUCCESS;

    p2 = p2 + len - 1;    /* assume that p2 and p4 point to same buffer */
    p4 = p4 + len - 1;    /* and process right to left */

    for (i=0; i < len; i++, p2--, p4--)
    {
        I2ASSIGN_MACRO(*p2, tmp2);
        tmp4 = (u_i4)tmp2;
        I4ASSIGN_MACRO(tmp4, *p4);
    }

    return SQL_SUCCESS;
}


/*
**  ConvertUCS4ToUCS2
**
**  Convert a character string to wide (Unicode) string, possibly truncated.
**
**  On entry: p4  -->UCS4 Unicode string
**            p2  -->UCS2 Unicode string
**            len  = length of string in characters.
**
**  On exit:  Data is copied to buffer, possibly truncated, null terminated.
**
**  Returns:  SQL_SUCCESS           String returned successfully.
*/
RETCODE ConvertUCS4ToUCS2(
    u_i4     * p4,
    u_i2     * p2,
    SQLINTEGER     len)
{
    SQLINTEGER     i;
    u_i4       sizeofSQLWCHAR = sizeof(SQLWCHAR);
    u_i4       tmp4;
    u_i2       tmp2;

    char *p = (char *)p2;

    if (sizeofSQLWCHAR != 4)  /* return if not needed */
        return SQL_SUCCESS;

    /* assume that p2 and p4 point to same buffer */
    /* and process left to right */

    for (i=0; i < len; i++, p2++, p4++)
    {
        I4ASSIGN_MACRO(*p4, tmp4); 
        tmp2 = (u_i2)tmp4;
        I2ASSIGN_MACRO(tmp2, *p2);
    }

    return SQL_SUCCESS;
}
