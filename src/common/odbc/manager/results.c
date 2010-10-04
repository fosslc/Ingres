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
** Name: results.c - Result set functions for the ODBC CLI.
** 
** Description: 
** 		This file defines: 
** 
**              SQLBindCol - Bind a column to a variable.
**              SQLDescribeCol - Return result set column attributes.
**              SQLFetch - Return a row of data.
**              SQLFetchScroll - Return a row via the Cursor Library.
**              SQLGetData - Get a result set column.
**              SQLMoreResults - Determine whether more results are avaialble.
**              SQLNumResultCols - Get number of result columns.
**              SQLNumParams - Get number of parameters.
**              SQLRowCount - Get rows affected by a query.
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
**   13-Mar-2007 (Ralph Loen) SIR 117786
**      Tightened up treatement of ODBC connection states.
**   24-Nov-2009 (Ralph Loen) Bug 122956
**      In SQLFetchScroll(), return an error with sqlstate HY106 (invalid
**      argument) if the FetchOrientation argument is invalid.
**   03-Sep-2010 (Ralph Loen) Bug 124348
**      Replaced SQLINTEGER, SQLUINTEGER and SQLPOINTER arguments with
**      SQLLEN, SQLULEN and SQLLEN * for compatibility with 64-bit
**      platforms.
*/ 

/*
** Name: 	SQLBindCol - Bind a variable to a tuple descriptor.
** 
** Description: 
**              SQLBindCol binds application data buffers to columns in 
**              the result set.
**
** Inputs:
**              hstmt - Statement handle.
**              icol - Column sequence.
**              fCType - C data type.
**              rgbValue - Data to be fetched.
**              cbValueMax - Maximum length of the data.
**              pcbValue - "Or" indicator.
**
** Outputs: 
**              pcbValue - May return data length.
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

RETCODE SQL_API SQLBindCol(
    SQLHSTMT      hstmt,        /* StatementHandle  */
    UWORD         icol,         /* ColumnNumber     */
    SWORD         fCType,       /* TargetType       */
    SQLPOINTER    rgbValue,     /* TargetValuePtr   */
    SQLLEN        cbValueMax,   /* BufferLength     */
    SQLLEN        *pcbValue)    /* StrLen_or_IndPtr */
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLBindCol(hstmt, icol, fCType,
        rgbValue, cbValueMax, pcbValue), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc =  IIBindCol(
        pstmt->hdr.driverHandle,        
        icol,         
        fCType,       
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
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_SUCCESS));
    return SQL_SUCCESS;

}

/*
** Name: 	SQLDescribCol - Get column information
** 
** Description: 
**              SQLDescribeCol returns the result descriptor-column name, 
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

RETCODE SQL_API SQLDescribeCol(
    SQLHSTMT     hstmt,
    UWORD        icol,
    UCHAR      * szColName,
    SWORD        cbColNameMax,
    SWORD      * pcbColName,
    SWORD      * pfSqlType,
    SQLULEN    * pcbColDef,
    SWORD      * pibScale,
    SWORD      * pfNullable)
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLDescribeCol(hstmt, icol, 
        szColName, cbColNameMax, pcbColName, pfSqlType, pcbColDef,
        pibScale, pfNullable), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    if (cbColNameMax <= 0)
        cbColNameMax = SQL_MAX_MESSAGE_LENGTH;

    rc =  IIDescribeCol( pstmt->hdr.driverHandle,
        icol,
        szColName,
        cbColNameMax,
        pcbColName,
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
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLFetch - Fetch a row of data
** 
** Description: 
**              SQLFetch fetches the next rowset of data from the result 
**              set and returns data for all bound columns.
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
**              May produce a result set.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

RETCODE SQL_API SQLFetch(
    SQLHSTMT hstmt)
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLFetch( hstmt), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc =  IIFetch( pstmt->hdr.driverHandle);

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
** Name: 	SQLFetchScroll - Fetch a rowset
** 
** Description: 
**              SQLFetchScroll fetches the specified rowset of data from 
**              the result set and returns data for all bound columns. 
**              Rowsets can be specified at an absolute or relative position. 
**
** Inputs:
**              hstmt - Statement handle.
**              FetchOrientation - Cursor direction.
**              FetchOffset - Row number.
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
SQLRETURN SQL_API SQLFetchScroll(
    SQLHSTMT     hstmt,
    SQLSMALLINT  FetchOrientation,
    SQLLEN       FetchOffset)
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLFetchScroll(hstmt, 
        FetchOrientation, FetchOffset), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    switch (FetchOrientation)
    {
        case SQL_FETCH_NEXT:
        case SQL_FETCH_FIRST:
        case SQL_FETCH_LAST:
        case SQL_FETCH_PRIOR:
        case SQL_FETCH_ABSOLUTE:
        case SQL_FETCH_RELATIVE:
            break;

        default:
        {
            insertErrorMsg((SQLHANDLE)pstmt, SQL_HANDLE_STMT, SQL_HY106,
                SQL_ERROR, NULL, 0);
            return SQL_ERROR;
            break;
        }
    }

    rc =  IIFetchScroll( 
        pstmt->hdr.driverHandle,
        FetchOrientation,
        FetchOffset);

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
** Name: 	SQLGetData - Fetch a single column
** 
** Description: 
**              SQLGetData retrieves data for a single column in the result 
**              set. It can be called multiple times to retrieve 
**              variable-length data in parts.
** Inputs:
**              hstmt - Statement handle.
**              icol - Column sequence.
**              fCType - C type of the column.
**              cbValueMax - Maximum length of the data.
**              pcbValue - "Or" indicator.
**
** Outputs: 
**              rgbValue - Data value.
**              pcbValue - Length of the data.
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
RETCODE SQL_API SQLGetData(
    SQLHSTMT     hstmt,
    UWORD        icol,
    SWORD        fCType,
    SQLPOINTER   rgbValue,
    SQLLEN       cbValueMax,
    SQLLEN       *pcbValue)
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLGetData(hstmt, icol, fCType,
        rgbValue, cbValueMax, pcbValue), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);
    rc =  IIGetData( 
        pstmt->hdr.driverHandle,
        icol,
        fCType,
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
** Name: 	SQLMoreResults - Check for pending result set
** 
** Description: 
**              SQLMoreResults determines whether more results are available 
**              on a statement containing SELECT, UPDATE, INSERT, or DELETE 
**              statements and, if so, initializes processing for those results.
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

RETCODE SQL_API SQLMoreResults (
    SQLHSTMT hstmt)
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;
    pDBC pdbc;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLMoreResults(hstmt), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);
    rc =  IIMoreResults( 
        pstmt->hdr.driverHandle);
     
    if (rc == SQL_NO_DATA_FOUND && (pstmt->hdr.state == S4))
    {
        if (!pstmt->select || !pstmt->execproc)
            pstmt->hdr.state = S1;
        else
            pstmt->hdr.state = S2; 
    }
    else if (SQL_SUCCEEDED(rc))
    {
        pdbc = pstmt->pdbc;
        if (pstmt->hdr.state >= S5 && pstmt->hdr.state <= S7)
            if (pstmt->cursor_open)
                pstmt->hdr.state = S5;
	    else
                pstmt->hdr.state = S4;
    }

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
** Name: 	SQLNumResultCols - Get number of columns in result set.
** 
** Description: 
**              SQLNumResultCols returns the number of columns in a result 
**              set.
**
** Inputs:
**              hstmt - Statement handle.
**
** Outputs: 
**              pccol - Number of columns.
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
RETCODE SQL_API SQLNumResultCols(
    SQLHSTMT    hstmt,
    SWORD     * pccol)
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLNumResultCols(hstmt, pccol),
         traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc =  IINumResultCols( 
        pstmt->hdr.driverHandle,
        pccol);
     
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
** Name: 	SQLNumParams - Get number of parameters.
** 
** Description: 
**              SQLNumParams returns the number of parameters in an SQL 
**              statement.
**
** Inputs:
**              hstmt - Statement handle.
**
** Outputs: 
**              pcpar - Parameter count.
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

RETCODE SQL_API SQLNumParams(
    SQLHSTMT    hstmt,
    SWORD     * pcpar)
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;


    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLNumParams(hstmt, pcpar), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc =  IINumParams( 
        pstmt->hdr.driverHandle,
        pcpar);
     
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
** Name: 	SQLRowCount - Get number of rows in a result set.
** 
** Description: 
**              SQLRowCount returns the number of rows affected by an 
**              UPDATE, INSERT, or DELETE statement.
**
** Inputs:
**              hstmt - Statement handle.
**
** Outputs: 
**              pcrow - Number of rows. 
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
RETCODE SQL_API SQLRowCount(
    SQLHSTMT     hstmt,
    SQLLEN       * pcrow)
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLRowCount(hstmt, pcrow), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc =  IIRowCount( 
        pstmt->hdr.driverHandle,
        pcrow);
     
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
