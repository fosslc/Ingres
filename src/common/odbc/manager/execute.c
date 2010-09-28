/*
** Copyright (c) 2010 Ingres Corporation 
*/ 

#include <compat.h>
#include <mu.h>
#include <qu.h>
#include <sql.h>                    /* ODBC Core definitions */
#include <sqlext.h>                 /* ODBC extensions */
#include <iiodbc.h>
#include <iiodbcfn.h>
#include "odbclocal.h"
#include "tracefn.h"

/* 
** Name: execute.c - Query execution functions for the ODBC CLI.
** 
** Description: 
** 		This file defines: 
** 
**              SQLCancel - Cancel a query.
**              SQLExecute - Execute a prepared query.
**              SQLExecDirect - Execute a query directly.
**              SQLParamData - Get execution data for segmented inserts.
**              SQLPutData - Insert column data.
**              SQLDescribeParam - Describe input parameters.
** 
** History: 
**   14-jun-04 (loera01)
**     Created. 
**   04-Oct-04 (hweho01)
**     Avoid compiler error on AIX platform, put include
**     files of sql.h and sqlext.h after qu.h.
**   15-Jul-2005 (hanje04)
**	Include iiodbcfn.h and tracefn.h which are the new home for the 
**	ODBC CLI function pointer definitions.
**   18-Jun-2006 (Ralph Loen) SIR 116260
**      Added SQLDescribeParam().
**   13-Mar-2007 (Ralph Loen) SIR 117786
**      Tightened up treatement of ODBC connection states.
**   03-Sep-2010 (Ralph Loen) Bug 124348
**      Replaced SQLINTEGER, SQLUINTEGER and SQLPOINTER arguments with
**      SQLLEN, SQLULEN and SQLLEN * for compatibility with 64-bit
**      platforms.
*/ 

/*
** Name: 	SQLCancel - Cancel a query
** 
** Description: 
**              SQLCancel cancels the processing on a statement.
**
** Inputs:
**              hstmt - Statement handle.
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
RETCODE SQL_API SQLCancel(
    SQLHSTMT  hstmt)
{
    RETCODE rc;
    pSTMT pstmt = (pSTMT)hstmt;
    pDBC pdbc;
    RETCODE traceRet = 1;


    ODBC_TRACE_ENTRY(ODBC_TR_TRACE,IITraceSQLCancel( hstmt), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);
    pdbc = pstmt->pdbc;

    rc = IICancel(pstmt->hdr.driverHandle);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }
    else if (SQL_SUCCEEDED(rc))
    {
        if (pstmt->prepared)
            if (pstmt->select || pstmt->execproc)
                pstmt->hdr.state = S3;
            else
                pstmt->hdr.state = S2;
        else
            pstmt->hdr.state = S1;			
    }

    releaseLock(SQL_HANDLE_STMT, pstmt);
    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLExecute - Execute a prepared query
** 
** Description: 
**              SQLExecute executes a prepared statement, using the current 
**              values of the parameter marker variables if any parameter 
**              markers exist in the statement.
**
** Inputs:
**              hstmt - Statement handle.
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
**              May create a result set.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

RETCODE SQL_API SQLExecute(
    SQLHSTMT  hstmt)
{
    pSTMT pstmt = (pSTMT)hstmt;
    pDBC pdbc;
    RETCODE rc, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLExecute(hstmt), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);
    pdbc = pstmt->pdbc;

    rc = IIExecute(pstmt->hdr.driverHandle);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc == SQL_NEED_DATA)
       pstmt->hdr.state = S8;
    else if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }
    else if (SQL_SUCCEEDED(rc))
    {
        if (!pdbc->autocommit)
            pdbc->hdr.state = C6;
        else
            pdbc->hdr.state = C5;

        if (pstmt->select || pstmt->execproc)
            pstmt->hdr.state = S5;
        else
            pstmt->hdr.state = S4;
    }

    releaseLock(SQL_HANDLE_STMT, pstmt);
    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLExecDirect - Execute a statement without a prior prepare
** 
** Description: 
**              SQLExecDirect executes a preparable statement, using the 
**              current values of the parameter marker variables if any 
**              parameters exist in the statement. 
**
** Inputs:
**              hstmt - Statement handle.
**              szSqlStr - Query string.
**              cbSqlStr - Length of query string.
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
RETCODE SQL_API SQLExecDirect(
    SQLHSTMT    hstmt,
    UCHAR     * szSqlStr,
    SDWORD      cbSqlStr)
{
    RETCODE rc, traceRet = 1;

    pSTMT pstmt = (pSTMT)hstmt;
    pDBC pdbc;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLExecDirect(hstmt, szSqlStr,
        cbSqlStr), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);
    pdbc = pstmt->pdbc;
    pstmt->prepared = FALSE;

    if (!szSqlStr)
    {
        insertErrorMsg(pstmt, SQL_HANDLE_STMT, SQL_HY009,
            SQL_ERROR, NULL, 0);
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_ERROR));
        return SQL_ERROR;
    }

    rc = IIExecDirect( pstmt->hdr.driverHandle,
        szSqlStr,
        cbSqlStr);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc == SQL_NEED_DATA)
       pstmt->hdr.state = S8;
    else if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }
    else if (SQL_SUCCEEDED(rc))
    {
        if (!pdbc->autocommit)
            pdbc->hdr.state = C6;
        else
            pdbc->hdr.state = C5;

        if (pstmt->select || pstmt->execproc)
            pstmt->hdr.state = S5;
        else
            pstmt->hdr.state = S4;
    }

    releaseLock(SQL_HANDLE_STMT, pstmt);
    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLParamData - Supply parameter data
** 
** Description: 
**              SQLParamData is used in conjunction with SQLPutData to 
**              supply parameter data at statement execution time.
**
** Inputs:
**              hstmt - Statement handle.
**              prgbValueParameter - Pointer to bound column.
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
RETCODE SQL_API SQLParamData(
    SQLHSTMT  hstmt,
    SQLPOINTER *prgbValueParameter)
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLParamData(hstmt, 
         prgbValueParameter), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc =  IIParamData(
        pstmt->hdr.driverHandle,
        prgbValueParameter);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }

    if (pstmt->hdr.state == S8 || pstmt->hdr.state == S10)
    {
        if (rc == SQL_ERROR)
            if (pstmt->prepared)
                pstmt->hdr.state = S2;
            else
                pstmt->hdr.state = S1;
        else if (rc == SQL_NEED_DATA)
            pstmt->hdr.state = S9;
    }
 
    releaseLock(SQL_HANDLE_STMT, pstmt);
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLPutData - Insert column data
** 
** Description: 
**              SQLPutData allows an application to send data for a 
**              parameter or column to the driver at statement execution time. 
**
** Inputs:
**              hstmt - Statement handle.
**              DataPtr - Column or parameter data.
**              cbValue - Length of column or parameter data.
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
RETCODE SQL_API SQLPutData(
    SQLHSTMT    hstmt,
    SQLPOINTER  DataPtr,
    SQLLEN      cbValue)
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLPutData(hstmt, DataPtr, cbValue),
         traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(pstmt, SQL_HANDLE_STMT);

    rc = IIPutData(
        pstmt->hdr.driverHandle,
        DataPtr,
        cbValue);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }

    if (pstmt->hdr.state == S9)
    {
        if (rc == SQL_ERROR)
        {
            if (pstmt->prepared && pstmt->cursor_open)
                pstmt->hdr.state = S3;
            else if (pstmt->prepared && !pstmt->cursor_open)
                pstmt->hdr.state = S2;
             else
                pstmt->hdr.state = S1;
        }
    } 
    else if (pstmt->hdr.state == S8)
    {
        if (rc == SQL_ERROR)
        {
            if (pstmt->prepared && pstmt->cursor_open)
                pstmt->hdr.state = S3;
            else if (pstmt->prepared && !pstmt->cursor_open)
                pstmt->hdr.state = S2;
            else
                pstmt->hdr.state = S1;
        }
        else if (SQL_SUCCEEDED(rc))
            pstmt->hdr.state = S10;
    }

    releaseLock(SQL_HANDLE_STMT, pstmt);
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLDescribeParam - Get parameter information
** 
** Description: 
**              SQLDescribeParam returns the description of a parameter 
**              marker associated with a prepared SQL statement.
**
** Inputs:
**              hstmt - Statement handle.
**              ipar - Parameter sequence.
**
** Outputs: 
**              pfSqlType - SQL type of the parameter.
**              pcbParamDef - Precision.
**              pibScale - Scale.
**              pfNullable - Whether or not the parameter is nullable.
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
**    16-jun-06 (loera01)
**      Created.
*/ 

SQLRETURN SQL_API SQLDescribeParam(
    SQLHSTMT           hstmt,
    SQLUSMALLINT       ipar,
    SQLSMALLINT        *pfSqlType,
    SQLUINTEGER        *pcbParamDef,
    SQLSMALLINT        *pibScale,
    SQLSMALLINT        *pfNullable)
{
    pSTMT pstmt = (pSTMT)hstmt;
    RETCODE rc, traceRet = 1;
 
    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLDescribeParam(hstmt, ipar,
         pfSqlType, pcbParamDef, pibScale, pfNullable), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(pstmt, SQL_HANDLE_STMT);

    rc = IIDescribeParam(
        pstmt->hdr.driverHandle,
            ipar,
            pfSqlType,
            pcbParamDef,
            pibScale,
            pfNullable);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }

    if (pstmt->hdr.state == S9)
    {
        if (rc == SQL_ERROR)
        {
            if (pstmt->prepared && pstmt->cursor_open)
                pstmt->hdr.state = S3;
            else if (pstmt->prepared && !pstmt->cursor_open)
                pstmt->hdr.state = S2;
             else
                pstmt->hdr.state = S1;
        }
    } 
    else if (pstmt->hdr.state == S8)
    {
        if (rc == SQL_ERROR)
        {
            if (pstmt->prepared && pstmt->cursor_open)
                pstmt->hdr.state = S3;
            else if (pstmt->prepared && !pstmt->cursor_open)
                pstmt->hdr.state = S2;
            else
                pstmt->hdr.state = S1;
        }
        else if (SQL_SUCCEEDED(rc))
            pstmt->hdr.state = S10;
    }

    releaseLock(SQL_HANDLE_STMT, pstmt);
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}
