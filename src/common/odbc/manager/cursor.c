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
** Name: cursor.c - Cursor library routines for the ODBC CLI
** 
** Description: 
** 		This file defines: 
** 
**              SQLExtendedFetch - Fetch a row via the cursor library. 
**              SQLSetPos - Set cursor position.
**              SQLSetScrollOptions - Define cursor result set.
** 
** History: 
**  14-jun-04 (loera01)
**    Created.
**  04-Oct-04 (hweho01)
**    Avoid compiler error on AIX platform, put include
**    files of sql.h and sqlext.h after qu.h.
**   15-Jul-2005 (hanje04)
**	Include iiodbcfn.h and tracefn.h which are the new home for the 
**	ODBC CLI function pointer definitions.
**   19-June-2006 (Ralph Loen) SIR 115708
**      Added support for SQLExtendedFetch().
**   30-Jan-2008 (Ralph Loen) SIR 119723
**      Activated support for SQLSetPos().
**   07-Feb-2008 (Ralph Loen) SIR 119723
**      Activated support for SQLSetScrollOptions().
**   03-Sep-2010 (Ralph Loen) Bug 124348
**      Replaced SQLINTEGER, SQLUINTEGER and SQLPOINTER arguments with
**      SQLLEN, SQLULEN and SQLLEN * for compatibility with 64-bit
**      platforms.
*/ 

/*
** Name: 	SQLExtendedFetch - Fetch a rowset of data.
** 
** Description: 
**              SQLExtendedFetch fetches the specified rowset of data from 
**              the result set and returns data for all bound columns.
**              This deprecated function is currently unsupported.
**
** Inputs:
**              hstmt - Statement handle.
**              fFetchType - Cursor direction.
**              irow - Row number (row offset) to fetch.
**
**
** Outputs: 
**              pcrow - Number of rows fetched.
**              rgfRowStatus - Row status array.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_SUCCESS_WITH_INFO
**              SQL_ERROR
**              SQL_NO_DATA
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
SQLRETURN SQL_API SQLExtendedFetch(
    SQLHSTMT           hstmt,
    SQLUSMALLINT       fFetchType,
    SQLLEN          irow,
    SQLULEN           *pcrow,
    SQLUSMALLINT       *rgfRowStatus)

{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLExtendedFetch(hstmt, 
        fFetchType, irow, pcrow, rgfRowStatus), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc =  IIExtendedFetch( 
        pstmt->hdr.driverHandle,
        fFetchType,
        irow,
        pcrow,
        rgfRowStatus);

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
** Name: 	SQLSetPos - Set cursor position
** 
** Description: 
**              SQLSetPos sets the cursor position in a rowset and allows 
**              an application to refresh data in the rowset or to update 
**              or delete data in the result set.  Currently not supported.
**
** Inputs:
**              hstmt - Statement handle.
**              irow - Row number.
**              fOption - Operation to perform.
**              fLock - How to lock the row after performing the operation.
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
**    30-jan-08 (Ralph Loen) SIR 119723
**      Activated for new driver capability.
**
*/ 
SQLRETURN SQL_API SQLSetPos(
    SQLHSTMT           hstmt,
    SQLUSMALLINT      irow,
    SQLUSMALLINT       fOption,
    SQLUSMALLINT       fLock)
{
    pSTMT pstmt = (pSTMT)hstmt;
    RETCODE traceRet = 1, rc = SQL_SUCCESS;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLSetPos( hstmt, irow, fOption,
       fLock), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc =  IISetPos(
        pstmt->hdr.driverHandle,
        irow,
        fOption,
        fLock);

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
** Name: 	SQLSetScrollOptions - Set cursor scroll options.
** 
** Description: 
**              SQLSetScrollOptions() specifies the cursor type and
**              concurrency for SQLFetchScroll() and SQLFetchExtended().
**
** Inputs:
**             hstmt - Statement handle.
**             fConcurrency - Cursor direction.
**             crowKeyset - Key set size.
**             crowRowset - Row set size.
**
** Outputs: 
**             None.
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
**    07-feb-08 (Ralph Loen)
**      Activated.
*/ 
SQLRETURN SQL_API SQLSetScrollOptions(
    SQLHSTMT           hstmt,
    SQLUSMALLINT       fConcurrency,
    SQLLEN             crowKeyset,
    SQLUSMALLINT       crowRowset)
{
    pSTMT pstmt = (pSTMT)hstmt;
    RETCODE traceRet = 1, rc = SQL_SUCCESS;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLSetScrollOptions(hstmt,
         fConcurrency, crowKeyset, crowRowset), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc = IISetScrollOptions(
        pstmt->hdr.driverHandle,
        fConcurrency,
        crowKeyset,
        crowRowset);

    if (rc == SQL_ERROR)
    {
        applyLock(SQL_HANDLE_STMT, pstmt);
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
        releaseLock(SQL_HANDLE_STMT, pstmt);
    }
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}
