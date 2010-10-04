/*
** Copyright (c) 2010 Ingres Corporation 
*/ 

#include <compat.h>
#include <cm.h>
#include <sql.h>                    /* ODBC Core definitions */
#include <sqlext.h>                 /* ODBC extensions */
#include <mu.h>
#include <qu.h>
#include <iiodbc.h>
#include <iiodbcfn.h>
#include "odbclocal.h"
#include "tracefn.h"

/* 
** Name: prepare.c - query preparation functions for the ODBC CLI.
** 
** Description: 
** 		This file defines: 
** 
**              SQLAllocStmt - Allocate a statement handle.
**              SQLBindParameter - Bind a parameter to a variable.
**              SQLBindParam - Bind a parameter to a variable (ISO version).
**              SQLCloseCursor - Close a cursor.
**              SQLFreeStmt - Free statement resources.
**              SQLGetCursorName - Get cursor name.
**              SQLNativeSql - Return modified SQL query string.    	
**              SQLParamOptions - Set query parameters (ODBC 1.0 version).
**              SQLPrepare - Prepare a query.
**              SQLSetCursorName - Set cursor name.
**              SQLSetParam - Bind parameter to a variable (ODBC 1.0 version).
** 
** History: 
**   14-jun-04 (loera01)
**     Created. 
**   15-Jul-2005 (hanje04)
**	Include iiodbcfn.h and tracefn.h which are the new home for the 
**	ODBC CLI function pointer definitions.
**   18-Jun-2006 (Ralph Loen) SIR 116260
**      Moved SQLDescribeParam() to execute.c to match driver code. 
**   13-Mar-2007 (Ralph Loen) SIR 117786
**      Tightened up treatement of ODBC connection states.  Replaced
**      call to SQLFreeStmt() with a call to the driver. 
**   03-Sep-2010 (Ralph Loen) Bug 124348
**      Replaced SQLINTEGER, SQLUINTEGER and SQLPOINTER arguments with
**      SQLLEN, SQLULEN and SQLLEN * for compatibility with 64-bit
**      platforms.
*/ 

/*
** Name: 	SQLAllocStmt - Allocate a statement handle.
** 
** Description: 
**              SQLAllocStmt allocates a statement handle.
**
** Inputs:
**              hdbc - Connection handle.
**
** Outputs: 
**              phstmt - Allocated statement handle.
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

SQLRETURN SQL_API SQLAllocStmt(
    SQLHDBC     hdbc,
    SQLHSTMT  * phstmt)
{
    pSTMT pstmt;
    pDBC pdbc = (pDBC)hdbc;
    RETCODE rc, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLAllocStmt(hdbc, phstmt), traceRet);
    rc = IIodbc_allocStmt(pdbc, (pSTMT *)phstmt); 

    if (rc == SQL_SUCCESS)
    {
        pstmt = (pSTMT) *phstmt;

        rc = IIAllocStmt(
            pdbc->hdr.driverHandle,
            &pstmt->hdr.driverHandle);


        applyLock(SQL_HANDLE_STMT, pstmt);
        if (rc != SQL_SUCCESS)
        {
            pdbc->hdr.driverError = TRUE;
            pdbc->errHdr.rc = rc;
	}
        else
        {
            pstmt->hdr.state = S1;
            pdbc->hdr.state = C5;
        }
        releaseLock(SQL_HANDLE_STMT, pstmt);
    }
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLBindParameter - Bind a variable to a parameter.
** 
** Description: 
**              SQLBindParameter binds a buffer to a parameter marker in 
**              an SQL statement. 
**
** Inputs:
**              hstmt - Statement handle.
**              ipar - Parameter sequence.
**              fParamType - parameter type.
**              fCType - C data type.
**              fSqlType - ODBC (SQL) data type.
**              cbColDef - Precision.
**              ibScale - Scale.
**              rgbValue - Data buffer.
**              cbValueMax  - Maximum length of data buffer.
**              pcbValue - "Or" indicator.
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

SQLRETURN SQL_API SQLBindParameter(
    SQLHSTMT      hstmt,        /* StatementHandle  */
    UWORD         ipar,         /* ParameterNumber  */
    SWORD         fParamType,   /* InputOutputType  */
    SWORD         fCType,       /* ValueType        */
    SWORD         fSqlType,     /* ParameterType    */
    SQLULEN       cbColDef,     /* ColumnSize       */
    SWORD         ibScale,      /* DecimalDigits    */
    SQLPOINTER    rgbValue,     /* ParameterValuePtr*/
    SQLLEN        cbValueMax,   /* BufferLength     */
    SQLLEN       *pcbValue)    /* StrLen_or_IndPtr */
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLBindParameter(hstmt, ipar, 
        fParamType, fCType, fSqlType, cbColDef, ibScale, rgbValue,
        cbValueMax, pcbValue), traceRet);

    rc =  IIBindParameter(
        pstmt->hdr.driverHandle,
        ipar,        
        fParamType, 
        fCType,    
        fSqlType, 
        cbColDef,
        ibScale,
        rgbValue,     
        cbValueMax,  
        pcbValue);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_STMT, pstmt);

    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLBindParam - Bind a variable to a parameter (deprecated)
** 
** Description: 
**              SQLBindParam binds a buffer to a parameter marker in 
**              an SQL statement.   
**              The CLI simply invokes SQLBindParameter from the driver
**              when SQLBindParam is called.
**
** Inputs:
**              StatementHandle - Statement handle.
**              ParameterNumber - Parameter sequence.
**              ValueType - C data type.
**              ParameterType - ODBC (SQL) data type.
**              LengthPrecision - Precision.
**              ibScale - Scale.
**              ParameterValue - Data buffer.
**              StrLen_or_Ind - "Or" indicator.
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


SQLRETURN  SQL_API SQLBindParam(
    SQLHSTMT StatementHandle,
    SQLUSMALLINT ParameterNumber,
    SQLSMALLINT ValueType,
    SQLSMALLINT ParameterType, 
    SQLULEN     LengthPrecision,
    SQLSMALLINT ParameterScale, 
    SQLPOINTER ParameterValue,
    SQLLEN     *StrLen_or_Ind)
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)StatementHandle;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLBindParam(StatementHandle,
        ParameterNumber, ValueType, ParameterType, LengthPrecision,
        ParameterScale, ParameterValue, StrLen_or_Ind), traceRet);

    rc =  IIBindParameter(
        pstmt->hdr.driverHandle,
        ParameterNumber,
        SQL_PARAM_OUTPUT,
        ValueType,
        ParameterType,
        LengthPrecision,
        ParameterScale,
        ParameterValue,
        0,
        StrLen_or_Ind);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }

    releaseLock(SQL_HANDLE_STMT, pstmt);
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLCloseCursor - Close a cursor.
** 
** Description: 
**              SQLCloseCursor closes a cursor that has been opened on a 
**              statement and discards pending results.
**
** Inputs:
**              StatementHandle.
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
SQLRETURN  SQL_API SQLCloseCursor (
    SQLHSTMT StatementHandle)
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)StatementHandle;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLCloseCursor(StatementHandle), 
         traceRet);

    if (validHandle((SQLHANDLE)pstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff((SQLHANDLE)pstmt, SQL_HANDLE_STMT);

    rc =  IICloseCursor(
        pstmt->hdr.driverHandle);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }
    else if (SQL_SUCCEEDED(rc))
    {
        if (pstmt->prepared)
            pstmt->hdr.state = S3;
        else
            pstmt->hdr.state = S1;
    }

    releaseLock(SQL_HANDLE_STMT, pstmt);
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLFreeStmt - Free a statement handle
** 
** Description: 
**              SQLFreeStmt stops processing associated with a specific 
**              statement, closes any open cursors associated with the 
**              statement, discards pending results, or, optionally, frees 
**              all resources associated with the statement handle.
**
** Inputs:
**              hstmt - Statement handle.
**              fOption - Operation to be performed.
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
SQLRETURN SQL_API SQLFreeStmt(
    SQLHSTMT hstmt,
    UWORD   fOption)
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;
    pDBC pdbc = pstmt->pdbc;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLFreeStmt(hstmt, fOption), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        rc = SQL_INVALID_HANDLE;
        goto exit_routine;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    if (fOption == SQL_DROP)
    {
        rc = IIFreeHandle(
            SQL_HANDLE_STMT,
            pstmt->hdr.driverHandle);
    }
    else
        rc = IIFreeStmt(
            pstmt->hdr.driverHandle,
            fOption);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
        goto exit_routine;
    }
    if (pstmt->hdr.state >= S4 &&
        pstmt->hdr.state <= S7)
    if (pstmt->prepared)
        pstmt->hdr.state = S2;
    else
        pstmt->hdr.state = S1;
    releaseLock(SQL_HANDLE_STMT, pstmt);

    if (fOption == SQL_DROP)
    {
        if (!IIodbc_freeHandle(SQL_HANDLE_STMT, (void *)pstmt))
            rc = SQL_INVALID_HANDLE;
        else
        {
            applyLock(SQL_HANDLE_DBC, pdbc);
            if (!pdbc->stmt_count && pdbc->hdr.state != C6)
                pdbc->hdr.state = C4;
            releaseLock(SQL_HANDLE_DBC, pdbc);
            rc = SQL_SUCCESS;
        } 
    }

exit_routine:
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLGetCursorName - Get cursor name
** 
** Description: 
**              SQLGetCursorName gets the cursor name associated with the
**              select query.
**
** Inputs:
**
**              hstmt - Statement handle.
**              cbCursorMax - Maximum length of cursor name buffer.
**
** Outputs: 
**              szCursor - Cursor name.
**              pcbCursor - Actual length of cursor name buffer.
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

SQLRETURN SQL_API SQLGetCursorName(
    SQLHSTMT      hstmt,
    UCHAR       * szCursor,
    SWORD         cbCursorMax,
    SWORD       * pcbCursor)
{   
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLGetCursorName(hstmt, szCursor,
        cbCursorMax, pcbCursor), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc =  IIGetCursorName(
        pstmt->hdr.driverHandle,
        szCursor,
        cbCursorMax,
        pcbCursor);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_STMT, pstmt);

    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLNativeSql - Get native SQL query string.
** 
** Description: 
**              SQLNativeSql returns the SQL string as modified by the driver. 
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

SQLRETURN SQL_API SQLNativeSql(
    SQLHDBC     hdbc,
    SQLCHAR   * InStatementText,
    SQLINTEGER  TextLength1,
    SQLCHAR   * OutStatementText,
    SQLINTEGER  BufferLength,
    SQLINTEGER* pcbValue)
{
    RETCODE rc, traceRet = 1;
    pDBC pdbc = (pDBC)hdbc;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLNativeSql(hdbc, InStatementText,
        TextLength1, OutStatementText, BufferLength, pcbValue), traceRet);

    if (!BufferLength)
        BufferLength = SQL_MAX_MESSAGE_LENGTH;

    if (validHandle(hdbc, SQL_HANDLE_DBC) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hdbc, SQL_HANDLE_DBC);

    rc =  IINativeSql(
        pdbc->hdr.driverHandle,
        InStatementText,
        TextLength1,
        OutStatementText,
        BufferLength,
        pcbValue);

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
** Name: 	SQLParamOptions - Set parameter array
** 
** Description: 
**              SQLParamOptions allows an application to specify multiple 
**              values for the set of parameters assigned by SQLBindParameter.
**
** Inputs:
**              hstmt - Statement handle.
**              crow - Number of rows.
**              pirow - Row buffer.
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

SQLRETURN SQL_API SQLParamOptions(
    SQLHSTMT     hstmt,
    UDWORD       crow,
    UDWORD     * pirow)
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLParamOptions (hstmt, crow, pirow),
        traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc =  IIParamOptions(
        pstmt->hdr.driverHandle,
        crow,
        pirow);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_STMT, pstmt);

    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLPrepare - Prepare a query
** 
** Description: 
**              SQLPrepare prepares an SQL string for execution.
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

SQLRETURN SQL_API SQLPrepare(
    SQLHSTMT      hstmt,
    UCHAR       * szSqlStr,
    SDWORD        cbSqlStr)
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;
    pDBC pdbc;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLPrepare(hstmt, szSqlStr, 
         cbSqlStr), traceRet);

    if (validHandle((SQLHANDLE)pstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff((SQLHANDLE)pstmt, SQL_HANDLE_STMT);
    pdbc = pstmt->pdbc;
 
	if (!szSqlStr)
    {
        insertErrorMsg((SQLHANDLE)pstmt, SQL_HANDLE_STMT, SQL_HY009,
            SQL_ERROR, NULL, 0);
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_ERROR));
        return SQL_ERROR;
    }

    rc = IIPrepare( pstmt->hdr.driverHandle,
        szSqlStr,
        cbSqlStr);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }

    if (SQL_SUCCEEDED(rc))
    {
       if (!pdbc->autocommit)
            pdbc->hdr.state = C6;
       if (pstmt->hdr.state == S1)
            if (pstmt->select || pstmt->execproc)
                pstmt->hdr.state = S3;
            else
                pstmt->hdr.state = S2;
    }
    else
        pstmt->hdr.state = S1;
    releaseLock(SQL_HANDLE_STMT, pstmt);

    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}
	
/*
** Name: 	SQLSetCursorName - Set cursor name.
** 
** Description: 
**              SQLSetCursorName associates a cursor name with an active 
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

SQLRETURN SQL_API SQLSetCursorName(
    SQLHSTMT      hstmt,
    UCHAR       * szCursorParameter,
    SWORD         cbCursor)
{   
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE,  IITraceSQLSetCursorName(hstmt, 
         szCursorParameter, cbCursor), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc =  IISetCursorName(
        pstmt->hdr.driverHandle,
        szCursorParameter,
        cbCursor);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }
    else if (SQL_SUCCEEDED(rc))
    {
        pstmt->prepared = TRUE;
    }
    releaseLock(SQL_HANDLE_STMT, pstmt);
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLSetParam - ODBC 2.0 version of SQLBindParameter
** 
** Description: 
**              SQLSetParam performs the same function as SQLBindParameter.
**              This function is not supported in the CLI.
** Inputs:
**              StatementHandle - Statement handle.
**              ParameterNumber - Parameter sequence.
**              ValueType - C type of the parameter.
**              ParameterType - SQL type of the parameter.
**              LengthPrecision - Precision.
**              ParameterScale - Scale.
**              ParameterValue - Variable to bind.
**              StrLen_or_Ind - Parameter disposition.
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

SQLRETURN  SQL_API SQLSetParam(
    SQLHSTMT StatementHandle,
    SQLUSMALLINT ParameterNumber, 
    SQLSMALLINT ValueType,
    SQLSMALLINT ParameterType, 
    SQLULEN LengthPrecision,
    SQLSMALLINT ParameterScale, 
    SQLPOINTER ParameterValue,
    SQLLEN *StrLen_or_Ind)
{
    pSTMT pstmt = (pSTMT)StatementHandle;
    RETCODE traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLSetParam(StatementHandle,
        ParameterNumber, ValueType, ParameterType, LengthPrecision,
        ParameterScale, ParameterValue, StrLen_or_Ind), traceRet);

    if (validHandle((SQLHANDLE)pstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff((SQLHANDLE)pstmt, SQL_HANDLE_STMT);

    insertErrorMsg((SQLHANDLE)pstmt, SQL_HANDLE_STMT, 
        SQL_IM001, SQL_ERROR, NULL, 0);
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_ERROR));
    return SQL_ERROR;
}

/*
** Name: 	ScanPastSpaces - Scan past spaces, if any.
** 
** Description: 
**              See above.
**
** Inputs:
**              str - String to scan.
**
** Outputs: 
**              None.
**
** Returns: 
**              Next non-space character in the string.
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
**    31-jul-97 (thoda04)
**      Created.
*/ 

char * ScanPastSpaces(char * str)
{
   while (CMspace(str))  CMnext(str);       /* scan past blanks */
   return(str);
}
