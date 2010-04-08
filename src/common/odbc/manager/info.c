/*
** Copyright (c) 2004, 2009 Ingres Corporation 
*/ 

#include <compat.h>
#include <mu.h>
#include <qu.h>
#include <sql.h>                    /* ODBC Core definitions */
#include <sqlext.h>                 /* ODBC extensions */
#include <iiodbc.h>
#include <iiodbcfn.h>
#include <idmseini.h>
#include "odbclocal.h"
#include "tracefn.h"

/* 
** Name: info.c - Get ODBC driver metadata for ODBC CLI.
** 
** Description: 
** 		This file defines: 
** 
**              SQLGetInfo - Get driver information.
**              SQLGetTypeInfo - Get information for individual data types.
**              SQLGetFunctions - Get information on functions supported.
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
**   23-Mar-09 (Ralph Loen) Bug 121838) 
**      Add SQLGetFunctions(), moved from manager.c.  SQLGetFunctions()
**      now calls the ODBC driver instead of providing defaults.
*/ 

/*
** Name: 	SQLGetInfo - Get general information
** 
** Description: 
**              SQLGetInfo returns general information about the driver and 
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
**    23-mar-09 (Ralph Loen) Bug 121838
**      Added SQLGetFunctions().
**
*/ 
RETCODE SQL_API SQLGetInfo(
    SQLHDBC hdbc,
    UWORD   fInfoType,
    SQLPOINTER  rgbInfoValueParameter,
    SWORD   cbInfoValueMax,
    SWORD     *pcbInfoValue)
{
    pDBC pdbc = (pDBC)hdbc;
    RETCODE rc, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLGetInfo(hdbc, fInfoType,
        rgbInfoValueParameter, cbInfoValueMax, 
        pcbInfoValue), traceRet);   

    if (validHandle(hdbc, SQL_HANDLE_DBC) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn( traceRet, SQL_INVALID_HANDLE)); 
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hdbc, SQL_HANDLE_DBC);

    rc = IIGetInfo(pdbc->hdr.driverHandle,
        fInfoType,
        rgbInfoValueParameter,
        cbInfoValueMax,
        pcbInfoValue);
   
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
** Name: 	SQLGetTypeInfo - Get information about a data type.
** 
** Description: 
**              SQLGetTypeInfo returns information about data types 
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

RETCODE SQL_API SQLGetTypeInfo(
    SQLHSTMT  hstmt,
    SWORD     fSqlType)
{
    pSTMT pstmt = (pSTMT)hstmt;
    RETCODE rc, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLGetTypeInfo(hstmt, fSqlType),
        traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc = IIGetTypeInfo(pstmt->hdr.driverHandle,
        fSqlType);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
        pstmt->hdr.state = S1;
    }
    else if (SQL_SUCCEEDED(rc))
        pstmt->hdr.state = S5;

    releaseLock(SQL_HANDLE_STMT, pstmt);
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLGetFunctions - Discover driver functions
** 
** Description: 
**              SQLGetFunctions returns information about whether a driver 
**              supports a specific ODBC function. 
**
** Inputs:
**              ConnectionHandle - Connection handle.
**              FunctionId - ODBC API function code.
**
** Outputs: 
**              Supported - Points to a single boolean if FunctionId  
**              describes a particular function.  Points to a 100-element
**              array of SQLUSMALLINT if FunctionId is SQL_API_ALL_FUNCTIONS.  
**              Points to a 250-element array of SQLUSMALLINT if FunctionId
**              is SQL_API_ODBC3_ALL_FUNCTIONS.  
**              The calling application allocates the return array.
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
**    23-mar-09 (Ralph Loen) Bug 121838 
**      Now calls driver instead of returning static data.
*/ 
SQLRETURN  SQL_API SQLGetFunctions(
    SQLHDBC hdbc,
    SQLUSMALLINT FunctionId, 
    SQLUSMALLINT *Supported)
{
    RETCODE rc, traceRet = 1;
    pDBC pdbc = (pDBC)hdbc;
    BOOL funcIdError = FALSE;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLGetFunctions(hdbc,
        FunctionId, Supported ), traceRet);

    if (validHandle(hdbc, SQL_HANDLE_DBC) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hdbc, SQL_HANDLE_DBC);

    if ( FunctionId < SQL_API_ALL_FUNCTIONS ) /* Value 0 */
        funcIdError = TRUE;
    else if ( FunctionId > SQL_API_MAX_LEVEL_3_VALUE ) /* Value 1050 */
        funcIdError = TRUE;
    else if ( FunctionId < SQL_API_ODBC3_ALL_FUNCTIONS &&  /* Value 999 */
        FunctionId > SQL_API_MAX_LEVEL_2_VALUE ) /* Value 100 */
        funcIdError = TRUE;
    
    if ( funcIdError )  
    {
         insertErrorMsg((SQLHANDLE)pdbc, SQL_HANDLE_DBC, SQL_HY095,
             SQL_ERROR, "Function type out of range", 0);
         ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_ERROR));
         return SQL_ERROR;
    }

    rc = IIGetFunctions( pdbc->hdr.driverHandle, 
                    FunctionId,
                    Supported);
    
    applyLock(SQL_HANDLE_DBC, pdbc);
    if (rc != SQL_SUCCESS)
    {
        pdbc->hdr.driverError = TRUE;
        pdbc->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_DBC, pdbc);

    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_SUCCESS));
    return SQL_SUCCESS;
}
