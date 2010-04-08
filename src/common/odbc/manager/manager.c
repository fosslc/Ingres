/*
** Copyright (c) 2004, 2009 Ingres Corporation 
*/ 

#include <compat.h>
#include <gl.h>
#include <me.h>
#include <mu.h>
#include <st.h>
#include <tr.h>
#include <qu.h>
#include <sql.h>                    /* ODBC Core definitions */
#include <sqlext.h>                 /* ODBC extensions */
#include <sqlstate.h>
#include <iiodbc.h>
#include <odbccfg.h>
#include <idmseini.h>
#include "odbclocal.h"

/* 
** Name: manager.c - ODBC driver manager functions for ODBC CLI.
** 
** Description: 
** 		This file defines: 
** 
**              SQLDrivers - List driver definitions.
**              SQLDataSources - List data source definitions.
**              SQLConfigDataSource - Modify data source definitions.
**              ConfigDSN - Modify data source definitions.
**              SQLInstallerError - Installer error handler.
** 
** History: 
**   14-jun-04 (loera01)
**      Created.
**   17-feb-05 (Ralph.Loen@ca.com)  SIR 113913
**      Added SQLConfigDataSource(), ConfigDSN(), SQLDataSources(), and
**      SQLInstallerError().
**   14-Jul-2005 (hanje04)
**	Move defns. of driver functions here from common!odbc!hdr iiodbc.h
**	as GLOBALDEF's because Mac OSX doesn't allow multiple symbol defines.
**   19-jan-2006 (loera01) SIR 115615
**      Default vendor attribute is now "Ingres".
**   19-June-2006 (Ralph Loen) SIRS 115708 and 112260
**      SQLGetFunctions() now includes SQLExtendedFetch() and SQLDescribeParam()
**      as supported functions.
**   03-Jul-2006 (Ralph Loen) SIR 116326
**      Add IIBrowseConnectW() external function definition.
**   14-Jul-2006 (Ralph Loen) SIR 116385
**      Add support for SQLTablePrivileges().
**   21-jul-06 (Ralph Loen)  SIR 116410
**      Add support for SQLColumnPrivilegesW().
**   26-Feb-1007 (Ralph Loen) 
**      Cleaned up #ifndef NT_GENERIC.
**   23-Mar-2009 (Ralph Loen) Bug 121838
**      Moved SQLGetFunctions() to info.c.
*/ 

IIODBC_STATE IIodbc_state[] =
{
    { "E0", "Unallocated environment" },
    { "E1", "Allocated environment, unallocated connection" },
    { "E2", "Allocated environment, allocated connection" },
    { "C0", "Unallocated environment, unallocated connection" },
    { "C1", "Allocated environment, unallocated connection" },
    { "C2", "Allocated environment, allocated connection" },
    { "C3", "Connection function needs data" },
    { "C4", "Connected connection" },
    { "C5", "Connected connection, allocated statement" },
    { "C6", "Connected connection, transaction in progress." },
    { "S0", "Unallocated statement" },
    { "S1", "Allocated statement." },
    { "S2", "Prepared statement. No result set will be created." },
    { "S3", "Prepared statement. A result set will be created." },
    { "S4", "Statement executed and no result set was created." },
    { "S5", "Statement executed, result set was created. " },
    { "S6", "Cursor positioned with SQLFetch or SQLFetchScroll." },
    { "S7", "Cursor positioned with SQLExtendedFetch." },
    { "S8", "Function needs data. SQLParamData has not been called." },
    { "S9", "Function needs data. SQLPutData has not been called." },
    { "S10", "Function needs data. SQLPutData has been called." },
    { "S11", "Still Executing." },
    { "S12", "Asynchronous execution cancelled" },
    { "D0", "Unallocated descriptor" },
    { "D1", "Allocated descriptor" }
};

/*
**  Ingres ODBC driver function entry points.
*/

GLOBALDEF IIODBC_DRIVER_FN IIAllocConnect;
GLOBALDEF IIODBC_DRIVER_FN IIAllocEnv;
GLOBALDEF IIODBC_DRIVER_FN IIAllocStmt;
GLOBALDEF IIODBC_DRIVER_FN IIBindCol;
GLOBALDEF IIODBC_DRIVER_FN IICancel;
GLOBALDEF IIODBC_DRIVER_FN IIColAttribute;
GLOBALDEF IIODBC_DRIVER_FN IIConnect;
GLOBALDEF IIODBC_DRIVER_FN IIDescribeCol;
GLOBALDEF IIODBC_DRIVER_FN IIDisconnect;
GLOBALDEF IIODBC_DRIVER_FN IIError;
GLOBALDEF IIODBC_DRIVER_FN IIExecDirect;
GLOBALDEF IIODBC_DRIVER_FN IIExecute;
GLOBALDEF IIODBC_DRIVER_FN IIFetch;
GLOBALDEF IIODBC_DRIVER_FN IIFreeConnect;
GLOBALDEF IIODBC_DRIVER_FN IIFreeEnv;
GLOBALDEF IIODBC_DRIVER_FN IIFreeStmt;
GLOBALDEF IIODBC_DRIVER_FN IIGetCursorName;
GLOBALDEF IIODBC_DRIVER_FN IINumResultCols;
GLOBALDEF IIODBC_DRIVER_FN IIPrepare;
GLOBALDEF IIODBC_DRIVER_FN IIRowCount;
GLOBALDEF IIODBC_DRIVER_FN IISetCursorName;
GLOBALDEF IIODBC_DRIVER_FN IISetParam;
GLOBALDEF IIODBC_DRIVER_FN IITransact;
GLOBALDEF IIODBC_DRIVER_FN IIColAttributes;
GLOBALDEF IIODBC_DRIVER_FN IITablePrivileges;
GLOBALDEF IIODBC_DRIVER_FN IIBindParam;
GLOBALDEF IIODBC_DRIVER_FN IIColumns;
GLOBALDEF IIODBC_DRIVER_FN IIDriverConnect;
GLOBALDEF IIODBC_DRIVER_FN IIGetConnectOption;
GLOBALDEF IIODBC_DRIVER_FN IIGetData;
GLOBALDEF IIODBC_DRIVER_FN IIGetInfo;
GLOBALDEF IIODBC_DRIVER_FN IIGetStmtOption;
GLOBALDEF IIODBC_DRIVER_FN IIGetTypeInfo;
GLOBALDEF IIODBC_DRIVER_FN IIParamData;
GLOBALDEF IIODBC_DRIVER_FN IIPutData;
GLOBALDEF IIODBC_DRIVER_FN IISetConnectOption;
GLOBALDEF IIODBC_DRIVER_FN IISetStmtOption;
GLOBALDEF IIODBC_DRIVER_FN IISpecialColumns;
GLOBALDEF IIODBC_DRIVER_FN IIStatistics;
GLOBALDEF IIODBC_DRIVER_FN IITables;
GLOBALDEF IIODBC_DRIVER_FN IIBrowseConnect;
GLOBALDEF IIODBC_DRIVER_FN IIColumnPrivileges;
GLOBALDEF IIODBC_DRIVER_FN IIDescribeParam;
GLOBALDEF IIODBC_DRIVER_FN IIExtendedFetch;
GLOBALDEF IIODBC_DRIVER_FN IIForeignKeys;
GLOBALDEF IIODBC_DRIVER_FN IIMoreResults;
GLOBALDEF IIODBC_DRIVER_FN IINativeSql;
GLOBALDEF IIODBC_DRIVER_FN IINumParams;
GLOBALDEF IIODBC_DRIVER_FN IIParamOptions;
GLOBALDEF IIODBC_DRIVER_FN IIPrimaryKeys;
GLOBALDEF IIODBC_DRIVER_FN IIProcedureColumns;
GLOBALDEF IIODBC_DRIVER_FN IIProcedures;
GLOBALDEF IIODBC_DRIVER_FN IISetPos;
GLOBALDEF IIODBC_DRIVER_FN IISetScrollOptions;
GLOBALDEF IIODBC_DRIVER_FN IIBindParameter;
GLOBALDEF IIODBC_DRIVER_FN IIAllocHandle;
GLOBALDEF IIODBC_DRIVER_FN IICloseCursor;
GLOBALDEF IIODBC_DRIVER_FN IICopyDesc;
GLOBALDEF IIODBC_DRIVER_FN IIEndTran;
GLOBALDEF IIODBC_DRIVER_FN IIFetchScroll;
GLOBALDEF IIODBC_DRIVER_FN IIFreeHandle;
GLOBALDEF IIODBC_DRIVER_FN IIGetConnectAttr;
GLOBALDEF IIODBC_DRIVER_FN IIGetDescField;
GLOBALDEF IIODBC_DRIVER_FN IIGetDescRec;
GLOBALDEF IIODBC_DRIVER_FN IIGetDiagField;
GLOBALDEF IIODBC_DRIVER_FN IIGetDiagRec;
GLOBALDEF IIODBC_DRIVER_FN IIGetEnvAttr;
GLOBALDEF IIODBC_DRIVER_FN IIGetStmtAttr;
GLOBALDEF IIODBC_DRIVER_FN IISetConnectAttr;
GLOBALDEF IIODBC_DRIVER_FN IISetDescField;
GLOBALDEF IIODBC_DRIVER_FN IISetDescRec;
GLOBALDEF IIODBC_DRIVER_FN IISetEnvAttr;
GLOBALDEF IIODBC_DRIVER_FN IISetStmtAttr;
GLOBALDEF IIODBC_DRIVER_FN IIColAttributeW;
GLOBALDEF IIODBC_DRIVER_FN IIColAttributesW;
GLOBALDEF IIODBC_DRIVER_FN IIConnectW;
GLOBALDEF IIODBC_DRIVER_FN IIDescribeColW;
GLOBALDEF IIODBC_DRIVER_FN IIErrorW;
GLOBALDEF IIODBC_DRIVER_FN IIGetDiagRecW;
GLOBALDEF IIODBC_DRIVER_FN IIGetDiagFieldW;
GLOBALDEF IIODBC_DRIVER_FN IIExecDirectW;
GLOBALDEF IIODBC_DRIVER_FN IIGetStmtAttrW;
GLOBALDEF IIODBC_DRIVER_FN IIDriverConnectW;
GLOBALDEF IIODBC_DRIVER_FN IISetConnectAttrW;
GLOBALDEF IIODBC_DRIVER_FN IIGetCursorNameW;
GLOBALDEF IIODBC_DRIVER_FN IIPrepareW;
GLOBALDEF IIODBC_DRIVER_FN IISetStmtAttrW;
GLOBALDEF IIODBC_DRIVER_FN IISetCursorNameW;
GLOBALDEF IIODBC_DRIVER_FN IIGetConnectOptionW;
GLOBALDEF IIODBC_DRIVER_FN IIGetInfoW;
GLOBALDEF IIODBC_DRIVER_FN IIColumnsW;
GLOBALDEF IIODBC_DRIVER_FN IIGetTypeInfoW;
GLOBALDEF IIODBC_DRIVER_FN IISetConnectOptionW;
GLOBALDEF IIODBC_DRIVER_FN IISpecialColumnsW;
GLOBALDEF IIODBC_DRIVER_FN IIStatisticsW;
GLOBALDEF IIODBC_DRIVER_FN IITablesW;
GLOBALDEF IIODBC_DRIVER_FN IIBrowseConnectW;
GLOBALDEF IIODBC_DRIVER_FN IIForeignKeysW;
GLOBALDEF IIODBC_DRIVER_FN IINativeSqlW;
GLOBALDEF IIODBC_DRIVER_FN IIPrimaryKeysW;
GLOBALDEF IIODBC_DRIVER_FN IIProcedureColumnsW;
GLOBALDEF IIODBC_DRIVER_FN IIProceduresW;
GLOBALDEF IIODBC_DRIVER_FN IISetDescFieldW;
GLOBALDEF IIODBC_DRIVER_FN IIGetDescFieldW;
GLOBALDEF IIODBC_DRIVER_FN IIGetDescRecW;
GLOBALDEF IIODBC_DRIVER_FN IIGetConnectAttrW;
GLOBALDEF IIODBC_DRIVER_FN IIGetPrivateProfileString;
GLOBALDEF IIODBC_DRIVER_FN IITraceInit;
GLOBALDEF IIODBC_DRIVER_FN IITablePrivilegesW;
GLOBALDEF IIODBC_DRIVER_FN IIColumnPrivilegesW;
GLOBALDEF IIODBC_DRIVER_FN IIGetFunctions;

/*
**  ODBC trace function entry points.
*/

GLOBALDEF IIODBC_DRIVER_FN IITraceReturn;
/*IIODBC_DRIVER_FN TraceOpenLogFile;
GLOBALDEF IIODBC_DRIVER_FN TraceCloseLogFile;
GLOBALDEF IIODBC_DRIVER_FN TraceVersion;
*/
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLAllocConnect;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLAllocEnv;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLAllocStmt;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLBindCol;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLCancel;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLColAttributes;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLColAttributesW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLConnect;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLConnectW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLDescribeCol;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLDescribeColW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLDisconnect;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLError;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLErrorW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLExecDirect;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLExecDirectW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLExecute;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLFetch;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLFreeConnect;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLFreeEnv;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLFreeStmt;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLGetCursorName;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLGetCursorNameW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLNumResultCols;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLPrepare;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLPrepareW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLRowCount;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLSetCursorName;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLSetCursorNameW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLSetParam;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLTransact;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLColumns;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLColumnsW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLDriverConnect;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLDriverConnectW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLGetConnectOption;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLGetConnectOptionW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLGetData;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLGetFunctions;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLGetInfo;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLGetInfoW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLGetStmtOption;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLGetTypeInfo;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLGetTypeInfoW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLParamData;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLPutData;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLSetConnectOption;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLSetConnectOptionW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLSetStmtOption;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLSpecialColumns;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLSpecialColumnsW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLStatistics;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLStatisticsW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLTables;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLTablesW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLBrowseConnect;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLBrowseConnectW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLColumnPrivileges;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLColumnPrivilegesW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLDataSources;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLDataSourcesW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLDescribeParam;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLExtendedFetch;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLForeignKeys;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLForeignKeysW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLMoreResults;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLNativeSql;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLNativeSqlW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLNumParams;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLParamOptions;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLPrimaryKeys;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLPrimaryKeysW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLProcedureColumns;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLProcedureColumnsW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLProcedures;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLProceduresW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLSetPos;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLSetScrollOptions;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLTablePrivileges;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLTablePrivilegesW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLDrivers;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLDriversW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLBindParameter;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLAllocHandle;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLBindParam;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLCloseCursor;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLColAttribute;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLColAttributeW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLCopyDesc;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLEndTran;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLFetchScroll;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLFreeHandle;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLGetConnectAttr;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLGetConnectAttrW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLGetDescField;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLGetDescFieldW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLGetDescRec;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLGetDescRecW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLGetDiagField;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLGetDiagFieldW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLGetDiagRec;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLGetDiagRecW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLGetEnvAttr;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLGetStmtAttr;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLGetStmtAttrW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLSetConnectAttr;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLSetConnectAttrW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLSetDescField;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLSetDescFieldW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLSetDescRec;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLSetEnvAttr;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLSetStmtAttr;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLSetStmtAttrW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLAllocHandleStd;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLAllocHandleStdW;
GLOBALDEF IIODBC_DRIVER_FN IITraceSQLBulkOperations;

/*
** Name: 	SQLDrivers - Return driver attributes
** 
** Description: 
**              SQLDrivers lists driver descriptions and driver 
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

SQLRETURN SQL_API SQLDrivers(
    SQLHENV          henv,
    SQLUSMALLINT     fDirection,
    SQLCHAR        *szDriverDesc,
    SQLSMALLINT      cbDriverDescMax,
    SQLSMALLINT     *pcbDriverDesc,
    SQLCHAR        *szDriverAttributes,
    SQLSMALLINT      cbDrvrAttrMax,
    SQLSMALLINT     *pcbDrvrAttr)
{   
    RETCODE traceRet = 1;

    pENV penv = (pENV)henv;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLDrivers(henv, fDirection,
        szDriverDesc, cbDriverDescMax, pcbDriverDesc, szDriverAttributes,
        cbDrvrAttrMax, pcbDrvrAttr), traceRet);

    if (validHandle(henv, SQL_HANDLE_ENV) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

/*
    resetErrorBuff(henv, SQL_HANDLE_ENV);

    insertErrorMsg(henv, SQL_HANDLE_ENV, SQL_IM001,
        SQL_ERROR, NULL, 0);

    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_ERROR));

    return SQL_ERROR;
*/
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_SUCCESS));
    return SQL_SUCCESS;

}

/*
** Name: 	SQLDataSources - Get data source information
** 
** Description: 
**              SQLDataSources returns information about a data source. 
**
** Inputs:
**              EnvironmentHandle - Environment handle.
**              Direction - Fetch direction.
**              BufferLength1 - Maximum length of data source name buffer.
**              BufferLength2 - Maximum length of description buffer.
**             
**
** Outputs: 
**              ServerName - Data source name.
**              NameLength1 - Actual length of data source name buffer.
**              Description - Driver description.
**              NameLength2 - Actual length of description buffer.
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
SQLRETURN  SQL_API SQLDataSources(
    SQLHENV EnvironmentHandle,
    SQLUSMALLINT Direction, 
    SQLCHAR *ServerName,
    SQLSMALLINT BufferLength1, 
    SQLSMALLINT *NameLength1,
    SQLCHAR *Description, 
    SQLSMALLINT BufferLength2,
    SQLSMALLINT *NameLength2)
{
    pENV penv = (pENV)EnvironmentHandle;
    bool sysIniDefined = FALSE;
#ifndef NT_GENERIC
    char *altPath = getMgrInfo(&sysIniDefined);
    PTR dsnH;
    PTR drvH=IIodbc_cb.drvConfigHandle;
    i4 count;
    char dsnname[OCFG_MAX_STRLEN];
    char dsndescript[OCFG_MAX_STRLEN];
    i4 i,j,attrCount;
    char **dsnList;
    ATTR_ID dsnAttrList[DSN_ATTR_MAX];
    ATTR_ID *attrList[DSN_ATTR_MAX];
#endif /* NT_GENERIC */
    STATUS status = OK; 
    RETCODE traceRet = 1;
    RETCODE rc = SQL_SUCCESS;
    static bool getSys = TRUE;
    static bool getUsr = TRUE;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLDataSources(EnvironmentHandle,
        Direction, ServerName, BufferLength1, NameLength1, Description,
        BufferLength2, NameLength2), traceRet);

    if (!EnvironmentHandle)
    {
        rc = SQL_INVALID_HANDLE;
        return rc;
    }

/*
** Not currently supported in the Windows version of the CLI.
*/
#ifdef NT_GENERIC
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
    return SQL_SUCCESS;
#else /* NT_GENERIC */

    resetErrorBuff(EnvironmentHandle, SQL_HANDLE_ENV);

    switch (Direction)
    {
        case SQL_FETCH_FIRST:
            getSys = TRUE;
            getUsr = TRUE;
            penv->usrDsnCurCount = 0;
            penv->sysDsnCurCount = 0;
            break;
        case SQL_FETCH_FIRST_USER:
            getSys = FALSE;
            getUsr = TRUE;
            penv->usrDsnCurCount = 0;
            break;
        case SQL_FETCH_FIRST_SYSTEM:
            getSys = TRUE;
            getUsr = FALSE;
            penv->sysDsnCurCount = 0;
            break;
        case SQL_FETCH_NEXT:
            break;
        default:
            insertErrorMsg(penv, SQL_HANDLE_ENV, SQL_HY103,
                SQL_ERROR, NULL, 0);
            return SQL_ERROR;
    }
    /*
    ** If no prior invocations of SQLGetData(), initialize the DSN queue.
    */
    if (!penv->usrDsnCount && !penv->sysDsnCount)
    {
        /*
        ** Open the odbcinst.ini file and initialize the driver handle.
        */
        if (altPath && *altPath && !sysIniDefined)
            openConfig(drvH, OCFG_ALT_PATH, altPath, &dsnH, &status);
        else
            openConfig(drvH, OCFG_SYS_PATH, NULL, &dsnH, &status);
        if (status != OK)
        {
            insertErrorMsg(penv, SQL_HANDLE_ENV, SQL_HY000,
                SQL_ERROR, "Cannot open system dsn configuration file", status);
            rc = SQL_ERROR;
            goto exit_module;
        }
        /* 
        ** Get system-level DSN definitions first.
        */
        count = listConfig(dsnH, (i4)0, NULL, &status);
        if (status != OK) 
        {
            insertErrorMsg(penv, SQL_HANDLE_ENV, SQL_HY000,
                SQL_ERROR, "Cannot get count of system DSN names", status);
            rc = SQL_ERROR;
            goto exit_module;
        }
        if (count)
        {
            penv->sysDsnCount = count;
            dsnList = (char **)MEreqmem( (u_i2) 0, 
                (u_i4)(count * sizeof(char *)), TRUE, (STATUS *) NULL);
            if (!dsnList)
            {
                insertErrorMsg(penv, SQL_HANDLE_ENV, SQL_HY001,
                    SQL_ERROR, NULL, 0);
                rc = SQL_ERROR;
                goto exit_module;
            }
            for (i=0;i<count;i++)
            {
                dsnList[i] = (char *)MEreqmem( (u_i2) 0, 
                    (u_i4)OCFG_MAX_STRLEN, TRUE, (STATUS *) NULL);
                if (!dsnList[i])
                {
                    insertErrorMsg(penv, SQL_HANDLE_ENV, SQL_HY001,
                        SQL_ERROR, NULL, 0);
                    rc = SQL_ERROR;
                    goto exit_module;
                }
            }
            penv->sysDsnList = (IIODBC_DSN_LIST *)MEreqmem( (u_i2) 0, 
                (u_i4)(count * sizeof(IIODBC_DSN_LIST)), TRUE, 
                    (STATUS *) NULL);
            if (!penv->sysDsnList)
            {
                insertErrorMsg(penv, SQL_HANDLE_ENV, SQL_HY001,
                    SQL_ERROR, NULL, 0);
                rc = SQL_ERROR;
                goto exit_module;
            }
            listConfig( dsnH, count, dsnList, &status );
            if (status != OK) 
            {
                insertErrorMsg(penv, SQL_HANDLE_ENV, SQL_HY000,
                    SQL_ERROR, "Cannot get system DSN data", status);
                rc = SQL_ERROR;
                goto exit_module;
            }

            for (i=0;i<count;i++)
            {
                STcopy(dsnList[i],penv->sysDsnList[i].dsnName);
                attrCount = getConfigEntry( dsnH, dsnList[i], 0, NULL, 
                    &status );
                if (status != OK) 
                {
                    insertErrorMsg(penv, SQL_HANDLE_ENV, SQL_HY000,
                        SQL_ERROR, "Cannot get count of system DSN entries",
                        status);
                    rc = SQL_ERROR;
                    goto exit_module;
                }
                if (attrCount)
                {
                    for (j = 0; j < attrCount; j++)
                    {
                        attrList[j] = &dsnAttrList[j];
                        dsnAttrList[j].value = (char *)MEreqmem( (u_i2) 0, 
                            (u_i4)OCFG_MAX_STRLEN, TRUE, (STATUS *) NULL);
                        if (!dsnAttrList[j].value)
                        {
                            insertErrorMsg(penv, SQL_HANDLE_ENV, SQL_HY001,
                                SQL_ERROR, NULL, 0);
                            rc = SQL_ERROR;
                            goto exit_module;
                        }
                    }
                    getConfigEntry( dsnH, dsnList[i], attrCount, attrList, 
                        &status );
                    if (status != OK)
                    {
                        insertErrorMsg(penv, SQL_HANDLE_ENV, SQL_HY000,
                            SQL_ERROR, "Cannot get system DSN entries", status);
                        rc = SQL_ERROR;
                        goto exit_module;
                    }

                    for (j=0;j<attrCount;j++)
                    {
                        if (dsnAttrList[j].id == DSN_ATTR_DRIVER)
                        {
                            STcopy(dsnAttrList[j].value,
                                penv->sysDsnList[i].dsnDescription);
                            break;
                        }
                    }
                    for (j = 0; j < attrCount; j++)
                        MEfree((PTR)dsnAttrList[j].value);
                } /* if (attrCount) */
            }  /* for (i=0;i<count;i++) */  
            for (i = 0; i < count; i++)
                MEfree((PTR)dsnList[i]);
            MEfree((PTR)dsnList);
        }  /* if (count) */
        closeConfig(dsnH, &status);

        /*
        ** Now get the user DSN's.
        */
        openConfig(drvH, OCFG_USR_PATH, NULL, &dsnH, &status);
        if (status != OK)
        {
            insertErrorMsg(penv, SQL_HANDLE_ENV, SQL_HY000,
                SQL_ERROR, "Cannot open user dsn configuration file", status);
            rc = SQL_ERROR;
            goto exit_module;
        }
        count = listConfig(dsnH, (i4)0, NULL, &status);
        if (status != OK)
        {
            insertErrorMsg(penv, SQL_HANDLE_ENV, SQL_HY000,
                SQL_ERROR, "Cannot get count of user DSN names", status);
            rc = SQL_ERROR;
            goto exit_module;
        }
        if (count)
        {
            penv->usrDsnCount = count;
            dsnList = (char **)MEreqmem( (u_i2) 0, 
                (u_i4)(count * sizeof(char *)), TRUE, (STATUS *) NULL);
            if (!dsnList)
            {
                insertErrorMsg(penv, SQL_HANDLE_ENV, SQL_HY001,
                    SQL_ERROR, NULL, 0);
                rc = SQL_ERROR;
                goto exit_module;
            }
            for (i=0;i<count;i++)
            {
                dsnList[i] = (char *)MEreqmem( (u_i2) 0, 
                    (u_i4)OCFG_MAX_STRLEN, TRUE, (STATUS *) NULL);
                if (!dsnList[i])
                {
                    insertErrorMsg(penv, SQL_HANDLE_ENV, SQL_HY001,
                        SQL_ERROR, NULL, 0);
                    rc = SQL_ERROR;
                    goto exit_module;
                }
            }
            penv->usrDsnList = (IIODBC_DSN_LIST *)MEreqmem( (u_i2) 0, 
                (u_i4)(count * sizeof(IIODBC_DSN_LIST)), TRUE, 
                (STATUS *) NULL);
            if (!penv->usrDsnList)
            {
                insertErrorMsg(penv, SQL_HANDLE_ENV, SQL_HY001,
                    SQL_ERROR, NULL, 0);
                rc = SQL_ERROR;
                goto exit_module;
            }
            listConfig( dsnH, count, dsnList, &status );
            if (status != OK) 
            {
                insertErrorMsg(penv, SQL_HANDLE_ENV, SQL_HY000,
                    SQL_ERROR, "Cannot get user DSN data", status);
                rc = SQL_ERROR;
                goto exit_module;
            }

            for (i=0;i<count;i++)
            {
                STcopy(dsnList[i],penv->usrDsnList[i].dsnName);
                attrCount = getConfigEntry( dsnH, dsnList[i], 0, NULL, 
                    &status );
                if (status != OK)
                {
                    insertErrorMsg(penv, SQL_HANDLE_ENV, SQL_HY000,
                        SQL_ERROR, "Cannot get count of user DSN entries",
                        status);
                    rc = SQL_ERROR;
                    goto exit_module;
                }
                if (attrCount)
                {
                    for (j = 0; j < attrCount; j++)
                    {
                        attrList[j] = &dsnAttrList[j];
                        dsnAttrList[j].value = (char *)MEreqmem( (u_i2) 0, 
                            (u_i4)OCFG_MAX_STRLEN, TRUE, (STATUS *) NULL);
                        if (!dsnAttrList[j].value)
                        {
                            insertErrorMsg(penv, SQL_HANDLE_ENV, SQL_HY001,
                                SQL_ERROR, NULL, 0);
                            rc = SQL_ERROR;
                            goto exit_module;
                        }
                    }
                    getConfigEntry( dsnH, dsnList[i], attrCount, attrList, 
                        &status );
                    if (status != OK)
                    {
                        insertErrorMsg(penv, SQL_HANDLE_ENV, SQL_HY000,
                            SQL_ERROR, "Cannot get user DSN entries", status);
                        rc = SQL_ERROR;
                        goto exit_module;
                    }

                    for (j=0;j<attrCount;j++)
                    {
                        if (dsnAttrList[j].id == DSN_ATTR_DRIVER)
                        {
                            STcopy(dsnAttrList[j].value,
                                penv->usrDsnList[i].dsnDescription);
                            break;
                        }
                    }
                    for (j = 0; j < attrCount; j++)
                        MEfree((PTR)dsnAttrList[j].value);
                } /* if (attrCount) */
            }  /* for (i=0;i<count;i++) */  
            for (i = 0; i < count; i++)
                MEfree((PTR)dsnList[i]);
            MEfree((PTR)dsnList);
        }  /* if (count) */
        closeConfig(dsnH, &status);
    } /* if (!penv->usrDsnCount && !penv->sysDsnCount) */

    if (getSys && getUsr)
    {
        if (penv->sysDsnCurCount >= penv->sysDsnCount && 
           penv->usrDsnCurCount >= penv->usrDsnCount) 
        {
            rc = SQL_NO_DATA;
            goto exit_module;
        }
    }
       
    if (getSys)
    {
        if (penv->sysDsnCurCount >= penv->sysDsnCount)
        {
            if (!getUsr)
            {
                rc = SQL_NO_DATA;
                goto exit_module;
            }
            else
                getSys = FALSE;
        }
        else
        {
            STcopy(penv->sysDsnList[penv->sysDsnCurCount].dsnName,ServerName);
            *NameLength1 = 
                STlength(penv->sysDsnList[penv->sysDsnCurCount].dsnName);

            STcopy(penv->sysDsnList[penv->sysDsnCurCount].dsnDescription,
                Description);
            *NameLength2 = 
                STlength(penv->sysDsnList[penv->sysDsnCurCount].dsnDescription);
            penv->sysDsnCurCount++;
            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_SUCCESS));
            rc =  SQL_SUCCESS;
            goto exit_module;
        }
    }
    if (getUsr)
    {
        if ((penv->usrDsnCurCount) >= penv->usrDsnCount)
        {
            if (!getSys)
            {
                rc = SQL_NO_DATA;
                goto exit_module;
            }
            else
                getUsr = FALSE;
        }
        else
        {
            STcopy(penv->usrDsnList[penv->usrDsnCurCount].dsnName,ServerName);
            *NameLength1 = 
                 STlength(penv->usrDsnList[penv->usrDsnCurCount].dsnName);

            STcopy(penv->usrDsnList[penv->usrDsnCurCount].dsnDescription,
                Description);
            *NameLength2 = 
                STlength(penv->usrDsnList[penv->usrDsnCurCount].dsnDescription);
            penv->usrDsnCurCount++;
        }
    } /* if (getUsr) */

exit_module:
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
#endif /* NT_GENERIC */
}

/*
** Name: 	SQLConfigDataSource - Add, delete, or modify an ODBC data
**              source definition.
** 
** Description: 
**              SQLConfigDataSource() adds, deletes, or modifies ODBC data
**              sources.  Note that the CLI currently doesn't need to 
**              dynamically load ConfigDSN() as indicated in the specs,
**              since it's dedicated to the Ingres ODBC driver at present.  
**
** Inputs:
**              hwndParent - Parent window.  Ignored on non-Windows.
**              fRequest - Type of request.
**              lpszDriver - Driver name.
**              lpszAttributes - Attribute string.
**
** Outputs: 
**              None.
**
** Returns: 
**
**              ODBC_ERROR_GENERAL_ERR
**              ODBC_ERROR_INVALID_HWND
**              ODBC_ERROR_INVALID_REQUEST_TYPE
**              ODBC_ERROR_INVALID_NAME
**              ODBC_ERROR_INVALID_KEYWORD_VALUE
**              ODBC_ERROR_REQUEST_FAILED
**              ODBC_ERROR_LOAD_LIBRARY_FAILED
**              ODBC_ERROR_OUT_OF_MEM 
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
** 
**              If successful, SQLConfigDataSource() will modify an ODBC
**              configuration file.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

SQLRETURN SQL_API SQLConfigDataSource(
    SQLHWND       hwndParent,
    SQLSMALLINT   fRequest,
    SQLCHAR       *lpszDriver,
    SQLCHAR       *lpszAttributes)
{

/*
** Not currently supported in the Windows version of the CLI.
*/
#ifdef NT_GENERIC
    return SQL_SUCCESS;
#else 
    /*
    ** Normally SQLConfigDataSource() loads ConfigDSN() dynamically, but the
    ** CLI can simply call it presently.
    */
    return ConfigDSN(hwndParent,
                     fRequest, 
                     lpszDriver,
                     lpszAttributes);
#endif /* NT_GENERIC */
}

/*
** Name: 	ConfigDSN - Add, delete, or modify an ODBC data
**              source definition.
** 
** Description: 
**              ConfigDSN() adds, deletes, or modifies ODBC data
**              sources.  Note that the CLI currently doesn't need to 
**              dynamically load ConfigDSN() as indicated in the specs,
**              since it's dedicated to the Ingres ODBC driver at present.  
**
** Inputs:
**              hwndParent - Parent window.  Ignored on non-Windows.
**              fRequest - Type of request.
**              lpszDriver - Driver name.
**              lpszAttributes - Attribute string.
**
** Outputs: 
**              None.
**
** Returns: 
**
**              ODBC_ERROR_GENERAL_ERR
**              ODBC_ERROR_INVALID_HWND
**              ODBC_ERROR_INVALID_REQUEST_TYPE
**              ODBC_ERROR_INVALID_NAME
**              ODBC_ERROR_INVALID_KEYWORD_VALUE
**              ODBC_ERROR_REQUEST_FAILED
**              ODBC_ERROR_LOAD_LIBRARY_FAILED
**              ODBC_ERROR_OUT_OF_MEM 
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
** 
**              If successful, ConfigDSN() will modify an ODBC
**              configuration file.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

#ifndef NT_GENERIC

SQLRETURN SQL_API ConfigDSN(
    SQLHWND       hwndParent,
    SQLSMALLINT   fRequest,
    SQLCHAR       *lpszDriver,
    SQLCHAR       *lpszAttributes)
{
    bool sysIniDefined = FALSE;
    char *altPath = getMgrInfo(&sysIniDefined);
    PTR dsnH = NULL;
    PTR drvH=IIodbc_cb.drvConfigHandle;
    i4 count;
    char dsnname[OCFG_MAX_STRLEN] = "\0";
    char dsndescript[OCFG_MAX_STRLEN];
    i4 i,j,attrCount,argCount=0,checkCount=0;
    char **drvList;
    char **dsnList;
    ATTR_ID drvAttrList[DSN_ATTR_MAX];
    ATTR_ID dsnAttrList[DSN_ATTR_MAX]; 
    ATTR_ID *attrList[DSN_ATTR_MAX];
    ATTR_ID argAttrList[DSN_ATTR_MAX];
    STATUS status = OK; 
    RETCODE traceRet = 1;
    static bool wantSys = FALSE;
    pENV penv = NULL;
    SQLCHAR *sz;
    pINST pinst = &IIodbc_cb.inst;
    i4 attrTable[] = 
    {
        DSN_ATTR_DRIVER,
        DSN_ATTR_DESCRIPTION,
        DSN_ATTR_VENDOR,
        DSN_ATTR_DRIVER_TYPE,
        DSN_ATTR_SERVER,
        DSN_ATTR_DATABASE,
        DSN_ATTR_SERVER_TYPE,
        DSN_ATTR_PROMPT_UID_PWD,
        DSN_ATTR_WITH_OPTION,
        DSN_ATTR_ROLE_NAME,
        DSN_ATTR_ROLE_PWD,
        DSN_ATTR_DISABLE_CAT_UNDERSCORE,
        DSN_ATTR_ALLOW_PROCEDURE_UPDATE,
        DSN_ATTR_USE_SYS_TABLES,
        DSN_ATTR_BLANK_DATE,
        DSN_ATTR_DATE_1582,
        DSN_ATTR_CAT_CONNECT,
        DSN_ATTR_NUMERIC_OVERFLOW,
        DSN_ATTR_SUPPORT_II_DECIMAL,
        DSN_ATTR_CAT_SCHEMA_NULL,
        DSN_ATTR_READ_ONLY,
        DSN_ATTR_SELECT_LOOPS,
        DSN_ATTR_CONVERT_3_PART
    };
    i4 attrTblCount = sizeof(attrTable) / sizeof(i4);

    resetErrorBuff(pinst, SQL_HANDLE_INSTALLER);

    switch (fRequest)
    {
        case (ODBC_ADD_DSN):
        case (ODBC_CONFIG_DSN):
        case (ODBC_REMOVE_DSN):
            break;
        case (ODBC_ADD_SYS_DSN):
        case (ODBC_CONFIG_SYS_DSN):
        case (ODBC_REMOVE_SYS_DSN):
            wantSys = TRUE;
            break;
        case (ODBC_REMOVE_DEFAULT_DSN):
            goto exit_OK;
            break;
        default:
            insertErrorMsg(pinst, SQL_HANDLE_INSTALLER, II_INVALID_REQUEST_TYPE,
                ODBC_ERROR_INVALID_REQUEST_TYPE, NULL, 0);
            goto error_exit;
    }

    for (j = 0; j < DSN_ATTR_MAX; j++)
    {
        argAttrList[j].value = (char *)MEreqmem( (u_i2) 0, 
            (u_i4)OCFG_MAX_STRLEN, TRUE, (STATUS *) NULL);
        if (!argAttrList[j].value)
        {
            insertErrorMsg(pinst, SQL_HANDLE_INSTALLER, II_OUT_OF_MEM,
                ODBC_ERROR_OUT_OF_MEM, NULL, 0);
            goto error_exit;
        }
    }
    /*
    ** Collect attributes from caller.
    */
    for (sz = (char *)lpszAttributes; sz && *sz; sz += STlength(sz) + 1)
    {
        if (STbcompare((char *)DSN_STR_DSN, STlength(DSN_STR_DSN), 
            sz, STlength(DSN_STR_DSN), TRUE) == 0 )
        {
            STcopy(sz + STlength(DSN_STR_DSN)+1, dsnname);
            checkCount--;
        }
        else if (STbcompare((char *)DSN_STR_DRIVER, 
            STlength(DSN_STR_DRIVER), sz, 
            STlength(DSN_STR_DRIVER), TRUE) == 0 )
        {
            argAttrList[argCount].id = DSN_ATTR_DRIVER;
            STcopy(sz + STlength(DSN_STR_DRIVER)+1, 
                argAttrList[argCount].value);
            argCount++;
        }
        else if (STbcompare((char *)DSN_STR_SELECT_LOOPS, 
            STlength(DSN_STR_SELECT_LOOPS), sz, 
            STlength(DSN_STR_SELECT_LOOPS), TRUE) == 0 )
        {
            argAttrList[argCount].id = DSN_ATTR_SELECT_LOOPS;
            STcopy(sz + STlength(DSN_STR_SELECT_LOOPS)+1, 
                argAttrList[argCount].value);
            CVupper(argAttrList[argCount].value);
            argCount++;
        }
        else if (STbcompare((char *)DSN_STR_DESCRIPTION, 
            STlength(DSN_STR_DESCRIPTION), sz, 
            STlength(DSN_STR_DESCRIPTION), TRUE) == 0 )
        {
            argAttrList[argCount].id = DSN_ATTR_DESCRIPTION;
            STcopy(sz + STlength(DSN_STR_DESCRIPTION)+1, 
                argAttrList[argCount].value);
            argCount++;
        }
        else if (STbcompare((char *)DSN_STR_DRIVER_TYPE, 
            STlength(DSN_STR_DRIVER_TYPE), sz, 
            STlength(DSN_STR_DRIVER_TYPE), TRUE) == 0 )
        {
            argAttrList[argCount].id = DSN_ATTR_DRIVER_TYPE;
            STcopy(sz + STlength(DSN_STR_DRIVER_TYPE)+1, 
                argAttrList[argCount].value);
            argCount++;
        }
        else if (STbcompare((char *)DSN_STR_SERVER, 
            STlength(DSN_STR_SERVER), sz, 
            STlength(DSN_STR_SERVER), TRUE) == 0  &&
            STbcompare((char *)DSN_STR_SERVER_TYPE, 
            STlength(DSN_STR_SERVER_TYPE), sz, 
            STlength(DSN_STR_SERVER_TYPE), TRUE))
        {
            argAttrList[argCount].id = DSN_ATTR_SERVER;
            STcopy(sz + STlength(DSN_STR_SERVER)+1, 
                argAttrList[argCount].value);
            argCount++;
        }
        else if (STbcompare((char *)DSN_STR_DATABASE, 
            STlength(DSN_STR_DATABASE), sz, 
            STlength(DSN_STR_DATABASE), TRUE) == 0 )
        {
            argAttrList[argCount].id = DSN_ATTR_DATABASE;
            STcopy(sz + STlength(DSN_STR_DATABASE)+1, 
                argAttrList[argCount].value);
            argCount++;
        }
        else if (STbcompare((char *)DSN_STR_SERVER_TYPE, 
            STlength(DSN_STR_SERVER_TYPE), sz, 
            STlength(DSN_STR_SERVER_TYPE), TRUE) == 0 )
        {
            argAttrList[argCount].id = DSN_ATTR_SERVER_TYPE;
            STcopy(sz + STlength(DSN_STR_SERVER_TYPE)+1, 
                argAttrList[argCount].value);
            argCount++;
        }
        else if (STbcompare((char *)DSN_STR_PROMPT_UID_PWD, 
            STlength(DSN_STR_PROMPT_UID_PWD), sz, 
            STlength(DSN_STR_PROMPT_UID_PWD), TRUE) == 0 )
        {
            argAttrList[argCount].id = DSN_ATTR_PROMPT_UID_PWD;
            STcopy(sz + STlength(DSN_STR_PROMPT_UID_PWD)+1, 
                argAttrList[argCount].value);
            CVupper(argAttrList[argCount].value);
            argCount++;
        }
        else if (STbcompare((char *)DSN_STR_WITH_OPTION, 
            STlength(DSN_STR_WITH_OPTION), sz, 
            STlength(DSN_STR_WITH_OPTION), TRUE) == 0 )
        {
            argAttrList[argCount].id = DSN_ATTR_WITH_OPTION;
            STcopy(sz + STlength(DSN_STR_WITH_OPTION)+1, 
                argAttrList[argCount].value);
            argCount++;
        }
        else if (STbcompare((char *)DSN_STR_ROLE_NAME, 
            STlength(DSN_STR_ROLE_NAME), sz, 
            STlength(DSN_STR_ROLE_NAME), TRUE) == 0 )
        {
            argAttrList[argCount].id = DSN_ATTR_ROLE_NAME;
            STcopy(sz + STlength(DSN_STR_ROLE_NAME)+1, 
                argAttrList[argCount].value);
            argCount++;
        }
        else if (STbcompare((char *)DSN_STR_ROLE_PWD, 
            STlength(DSN_STR_ROLE_PWD), sz, 
            STlength(DSN_STR_ROLE_PWD), TRUE) == 0 )
        {
            argAttrList[argCount].id = DSN_ATTR_ROLE_PWD;
            STcopy(sz + STlength(DSN_STR_ROLE_PWD)+1, 
                argAttrList[argCount].value);
            argCount++;
        }
        else if (STbcompare((char *)DSN_STR_DISABLE_CAT_UNDERSCORE, 
            STlength(DSN_STR_DISABLE_CAT_UNDERSCORE), sz, 
            STlength(DSN_STR_DISABLE_CAT_UNDERSCORE), TRUE) == 0 )
        {
            argAttrList[argCount].id = DSN_ATTR_DISABLE_CAT_UNDERSCORE;
            STcopy(sz + STlength(DSN_STR_DISABLE_CAT_UNDERSCORE)+1, 
                argAttrList[argCount].value);
            CVupper(argAttrList[argCount].value);
            argCount++;
        }
        else if (STbcompare((char *)DSN_STR_ALLOW_PROCEDURE_UPDATE, 
            STlength(DSN_STR_ALLOW_PROCEDURE_UPDATE), sz, 
            STlength(DSN_STR_ALLOW_PROCEDURE_UPDATE), TRUE) == 0 )
        {
            argAttrList[argCount].id = DSN_ATTR_ALLOW_PROCEDURE_UPDATE;
            STcopy(sz + STlength(DSN_STR_ALLOW_PROCEDURE_UPDATE)+1, 
                argAttrList[argCount].value);
            CVupper(argAttrList[argCount].value);
            argCount++;
        }
        else if (STbcompare((char *)DSN_STR_USE_SYS_TABLES, 
            STlength(DSN_STR_USE_SYS_TABLES), sz, 
            STlength(DSN_STR_USE_SYS_TABLES), TRUE) == 0 )
        {
            argAttrList[argCount].id = DSN_ATTR_USE_SYS_TABLES;
            STcopy(sz + STlength(DSN_STR_USE_SYS_TABLES)+1, 
                argAttrList[argCount].value);
            CVupper(argAttrList[argCount].value);
            argCount++;
        }
        else if (STbcompare((char *)DSN_STR_BLANK_DATE, 
            STlength(DSN_STR_BLANK_DATE), sz, 
            STlength(DSN_STR_BLANK_DATE), TRUE) == 0 )
        {
            argAttrList[argCount].id = DSN_ATTR_BLANK_DATE;
            STcopy(sz + STlength(DSN_STR_BLANK_DATE)+1, 
                argAttrList[argCount].value);
            CVupper(argAttrList[argCount].value);
            argCount++;
        }
        else if (STbcompare((char *)DSN_STR_DATE_1582, 
            STlength(DSN_STR_DATE_1582), sz, 
            STlength(DSN_STR_DATE_1582), TRUE) == 0 )
        {
            argAttrList[argCount].id = DSN_ATTR_DATE_1582;
            STcopy(sz + STlength(DSN_STR_DATE_1582)+1, 
                argAttrList[argCount].value);
            CVupper(argAttrList[argCount].value);
            argCount++;
        }
        else if (STbcompare((char *)DSN_STR_CAT_CONNECT, 
            STlength(DSN_STR_CAT_CONNECT), sz, 
            STlength(DSN_STR_CAT_CONNECT), TRUE) == 0 )
        {
            argAttrList[argCount].id = DSN_ATTR_CAT_CONNECT;
            STcopy(sz + STlength(DSN_STR_CAT_CONNECT)+1, 
                argAttrList[argCount].value);
            CVupper(argAttrList[argCount].value);
            argCount++;
        }
        else if (STbcompare((char *)DSN_STR_NUMERIC_OVERFLOW, 
            STlength(DSN_STR_NUMERIC_OVERFLOW), sz, 
            STlength(DSN_STR_NUMERIC_OVERFLOW), TRUE) == 0 )
        {
            argAttrList[argCount].id = DSN_ATTR_NUMERIC_OVERFLOW;
            STcopy(sz + STlength(DSN_STR_NUMERIC_OVERFLOW)+1, 
                argAttrList[argCount].value);
            CVupper(argAttrList[argCount].value);
            argCount++;
        }
        else if (STbcompare((char *)DSN_STR_SUPPORT_II_DECIMAL, 
            STlength(DSN_STR_SUPPORT_II_DECIMAL), sz, 
            STlength(DSN_STR_SUPPORT_II_DECIMAL), TRUE) == 0 )
        {
            argAttrList[argCount].id = DSN_ATTR_SUPPORT_II_DECIMAL;
            STcopy(sz + STlength(DSN_STR_SUPPORT_II_DECIMAL)+1, 
                argAttrList[argCount].value);
            CVupper(argAttrList[argCount].value);
            argCount++;
        }
        else if (STbcompare((char *)DSN_STR_CAT_SCHEMA_NULL, 
            STlength(DSN_STR_CAT_SCHEMA_NULL), sz, 
            STlength(DSN_STR_CAT_SCHEMA_NULL), TRUE) == 0 )
        {
            argAttrList[argCount].id = DSN_ATTR_CAT_SCHEMA_NULL;
            STcopy(sz + STlength(DSN_STR_CAT_SCHEMA_NULL)+1, 
                argAttrList[argCount].value);
            CVupper(argAttrList[argCount].value);
            argCount++;
        }
        else if (STbcompare((char *)DSN_STR_READ_ONLY, 
            STlength(DSN_STR_READ_ONLY), sz, 
            STlength(DSN_STR_READ_ONLY), TRUE) == 0 )
        {
            argAttrList[argCount].id = DSN_ATTR_READ_ONLY;
            STcopy(sz + STlength(DSN_STR_READ_ONLY)+1, 
                argAttrList[argCount].value);
            CVupper(argAttrList[argCount].value);
            argCount++;
        }
        else if (STbcompare((char *)DSN_STR_CONVERT_3_PART, 
            STlength(DSN_STR_CONVERT_3_PART), sz, 
            STlength(DSN_STR_CONVERT_3_PART), TRUE) == 0 )
        {
            argAttrList[argCount].id = DSN_ATTR_CONVERT_3_PART;
            STcopy(sz + STlength(DSN_STR_CONVERT_3_PART)+1, 
                argAttrList[argCount].value);
            CVupper(argAttrList[argCount].value);
            argCount++;
        }
        if (checkCount+1 != argCount)
        {
            /*
            ** Didn't pick up any recognized attributes.  Syntax error.
            */
            insertErrorMsg(pinst, SQL_HANDLE_INSTALLER, 
                II_INVALID_KEYWORD_VALUE, ODBC_ERROR_INVALID_KEYWORD_VALUE, 
                NULL, 0);
            goto error_exit;
        }
        checkCount = argCount;
    } /* for (sz = (char *)lpszAttributes; sz && *sz;  */

    if (!dsnname[0])
    {
        insertErrorMsg(pinst, SQL_HANDLE_INSTALLER, II_GENERAL_ERR,
            ODBC_ERROR_GENERAL_ERR, "No DSN attribute specified", 0);
        goto error_exit;
    }

    if (!argCount && fRequest != ODBC_ADD_DSN && fRequest != ODBC_ADD_SYS_DSN)
    {
        insertErrorMsg(pinst, SQL_HANDLE_INSTALLER, II_GENERAL_ERR,
            ODBC_ERROR_GENERAL_ERR, "No attributes specified", 0);
        goto error_exit;
    }
     
    if (altPath && *altPath && !sysIniDefined)
        openConfig(NULL, OCFG_ALT_PATH, altPath, &drvH, &status);
    else
        openConfig(NULL, OCFG_SYS_PATH, NULL, &drvH, &status);
    if (status != OK)
    {
        insertErrorMsg(pinst, SQL_HANDLE_INSTALLER, II_GENERAL_ERR,
            ODBC_ERROR_GENERAL_ERR, "Cannot initialize driver data", status);
        goto error_exit;
    }
    
    /* 
    ** Get driver definitions.
    */
    count = listConfig(drvH, (i4)0, NULL, &status);
    if (status != OK) 
    {
        insertErrorMsg(pinst, SQL_HANDLE_INSTALLER, II_GENERAL_ERR,
            ODBC_ERROR_GENERAL_ERR, "Cannot get count of driver names", status);
        goto error_exit;
    }
    if (count)
    {
        drvList = (char **)MEreqmem( (u_i2) 0, 
            (u_i4)(count * sizeof(char *)), TRUE, (STATUS *) NULL);
        if (!drvList)
        {
            insertErrorMsg(pinst, SQL_HANDLE_INSTALLER, II_OUT_OF_MEM,
                ODBC_ERROR_OUT_OF_MEM, NULL, 0);
            goto error_exit;
        }
        for (i=0;i<count;i++)
        {
            drvList[i] = (char *)MEreqmem( (u_i2) 0, 
                (u_i4)OCFG_MAX_STRLEN, TRUE, (STATUS *) NULL);
            if (!drvList[i])
            {
                insertErrorMsg(pinst, SQL_HANDLE_INSTALLER, II_OUT_OF_MEM,
                    ODBC_ERROR_OUT_OF_MEM, NULL, 0);
                goto error_exit;
            }
        }
        listConfig( drvH, count, drvList, &status );
        if (status != OK) 
        {
            insertErrorMsg(pinst, SQL_HANDLE_INSTALLER, II_GENERAL_ERR,
                ODBC_ERROR_GENERAL_ERR, "Cannot get driver data", status);
            goto error_exit;
        }
    } /* if (count) */

    for (i = 0; i < count; i++)
    {
        if (!STcompare(lpszDriver, drvList[i]))
           break;
    }
    if (i == count)
    {
        insertErrorMsg(pinst, SQL_HANDLE_INSTALLER, II_INVALID_NAME,
            ODBC_ERROR_INVALID_NAME, 0, status);
        goto error_exit;
    }
    for (i = 0; i < count; i++)
        MEfree((PTR)drvList[i]);
    MEfree((PTR)drvList); 
    if (wantSys)
    {
        if (altPath && *altPath && !sysIniDefined)
            openConfig(drvH, OCFG_ALT_PATH, altPath, &dsnH, &status);
        else
            openConfig(drvH, OCFG_SYS_PATH, NULL, &dsnH, &status);
    }
    else
        openConfig(drvH, OCFG_USR_PATH, NULL, &dsnH, &status);

    if (status != OK)
    {
        insertErrorMsg(pinst, SQL_HANDLE_INSTALLER, II_GENERAL_ERR,
            ODBC_ERROR_GENERAL_ERR, "Cannot read DSN data", status);
        goto error_exit;
    }
    /* 
    ** Get DSN definitions.
    */
    switch (fRequest)
    {
    case ODBC_ADD_DSN:
    case ODBC_ADD_SYS_DSN:
        for (j = 0; j < attrTblCount; j++)
        {
            attrList[j] = &dsnAttrList[j];
            dsnAttrList[j].id = attrTable[j];
            dsnAttrList[j].value = (char *)MEreqmem( (u_i2) 0, 
                (u_i4)OCFG_MAX_STRLEN, TRUE, (STATUS *) NULL);
            if (!dsnAttrList[j].value)
            {
                insertErrorMsg(pinst, SQL_HANDLE_INSTALLER, II_OUT_OF_MEM,
                    ODBC_ERROR_OUT_OF_MEM, NULL, 0);
                goto error_exit;
            }
        }
        /*
        ** First supply any requested attributes.
        */
        for (i=0;i<argCount;i++)
        {
            for (j=0;j<attrTblCount;j++)
            {
                if (dsnAttrList[j].id == argAttrList[i].id)
                    STcopy(argAttrList[i].value, dsnAttrList[j].value); 
            } /* for (j = 0; j < attrCount; j++) */
        } /* for (i=0;i<argCount;i++) */
        /*
        ** Add default values for missing attributes.
        */
        for (j=0;j<attrTblCount;j++)
        {
            switch (dsnAttrList[j].id)
            {
            case DSN_ATTR_DATABASE:
                if (!dsnAttrList[j].value[0])
                {
                    insertErrorMsg(pinst, SQL_HANDLE_INSTALLER, II_GENERAL_ERR,
                        ODBC_ERROR_GENERAL_ERR, "Database attribute required", 
                        status);
                    goto error_exit;
                }
            case DSN_ATTR_VENDOR:
                if (!dsnAttrList[j].value[0])
                    STcopy(DSN_STR_INGRES_CORP, dsnAttrList[j].value);
                break;
            case DSN_ATTR_DRIVER:
                if (!dsnAttrList[j].value[0])
                    STcopy(DSN_STR_INGRES, dsnAttrList[j].value);
                break;
            case DSN_ATTR_DRIVER_TYPE:
                if (!dsnAttrList[j].value[0])
                    STcopy(DSN_STR_INGRES, dsnAttrList[j].value);
                break;
            case DSN_ATTR_SERVER:
                if (!dsnAttrList[j].value[0])
                    STcopy(DSN_STR_LOCAL, dsnAttrList[j].value);
                break;
            case DSN_ATTR_SERVER_TYPE:
                if (!dsnAttrList[j].value[0])
                    STcopy(DSN_STR_INGRES, dsnAttrList[j].value);
                break;
            case DSN_ATTR_DISABLE_CAT_UNDERSCORE:
            case DSN_ATTR_ALLOW_PROCEDURE_UPDATE:
            case DSN_ATTR_USE_SYS_TABLES:
            case DSN_ATTR_BLANK_DATE:
            case DSN_ATTR_DATE_1582:
            case DSN_ATTR_CAT_CONNECT:
            case DSN_ATTR_NUMERIC_OVERFLOW:
            case DSN_ATTR_SUPPORT_II_DECIMAL:
            case DSN_ATTR_CAT_SCHEMA_NULL:
            case DSN_ATTR_READ_ONLY:
            case DSN_ATTR_SELECT_LOOPS:
            case DSN_ATTR_CONVERT_3_PART:
                if (!dsnAttrList[j].value[0])
                    STcopy(DSN_STR_N, dsnAttrList[j].value);
                break;
            default:
                break;
            }
        } /* for (j = 0; j < attrCount; j++) */
        setConfigEntry( dsnH, dsnname, attrTblCount, attrList, &status );
        if (status != OK) 
        {
            insertErrorMsg(pinst, SQL_HANDLE_INSTALLER, II_GENERAL_ERR,
                ODBC_ERROR_GENERAL_ERR, "Cannot add DSN entry", status);
            goto error_exit;
        }
        for (j = 0; j < attrTblCount; j++)
            MEfree((PTR)dsnAttrList[j].value);
        break;
    case ODBC_REMOVE_DSN:
    case ODBC_REMOVE_SYS_DSN:
    case ODBC_CONFIG_DSN:
    case ODBC_CONFIG_SYS_DSN:
        count = listConfig(dsnH, (i4)0, NULL, &status);
        if (status != OK) 
        {
            insertErrorMsg(pinst, SQL_HANDLE_INSTALLER, II_GENERAL_ERR,
                ODBC_ERROR_GENERAL_ERR, "Cannot get count of DSN names", 
                status);
            goto error_exit;
        }
        if (!count)
        {
            insertErrorMsg(pinst, SQL_HANDLE_INSTALLER, II_GENERAL_ERR,
                ODBC_ERROR_GENERAL_ERR, "No DSN data to modify", status);
            goto error_exit;
        }
        else
        {
            dsnList = (char **)MEreqmem( (u_i2) 0, 
                (u_i4)(count * sizeof(char *)), TRUE, (STATUS *) NULL);
            if (!dsnList)
            {
                insertErrorMsg(pinst, SQL_HANDLE_INSTALLER, II_OUT_OF_MEM,
                    ODBC_ERROR_OUT_OF_MEM, NULL, 0);
                goto error_exit;
            }
            for (i=0;i<count;i++)
            {
                dsnList[i] = (char *)MEreqmem( (u_i2) 0, 
                    (u_i4)OCFG_MAX_STRLEN, TRUE, (STATUS *) NULL);
                if (!dsnList[i])
                {
                    insertErrorMsg(pinst, SQL_HANDLE_INSTALLER, II_OUT_OF_MEM,
                        ODBC_ERROR_OUT_OF_MEM, NULL, 0);
                    goto error_exit;
                }
            }
            listConfig( dsnH, count, dsnList, &status );
            if (status != OK) 
            {
                insertErrorMsg(pinst, SQL_HANDLE_INSTALLER, II_GENERAL_ERR,
                    ODBC_ERROR_GENERAL_ERR, "Cannot get DSN data", status);
                goto error_exit;
            }
    
            attrCount = getConfigEntry( dsnH, dsnname, 0, NULL, &status );
            if (status != OK) 
            {
                insertErrorMsg(pinst, SQL_HANDLE_INSTALLER, II_GENERAL_ERR,
                    ODBC_ERROR_GENERAL_ERR, "Cannot locate DSN definition",
                    status);
                goto error_exit;
            }
            if (fRequest == ODBC_REMOVE_DSN || fRequest == ODBC_REMOVE_SYS_DSN)
            {
                delConfigEntry( dsnH, dsnname, &status );
                if (status != OK) 
                {
                    insertErrorMsg(pinst, SQL_HANDLE_INSTALLER, II_GENERAL_ERR,
                        ODBC_ERROR_GENERAL_ERR, "Cannot delete DSN entry", 
                        status);
                    goto error_exit;
                }
                else
                    goto exit_OK;
            }
            if (attrCount)
            {
                for (j = 0; j < attrCount; j++)
                {
                    attrList[j] = &dsnAttrList[j];
                    dsnAttrList[j].value = (char *)MEreqmem( (u_i2) 0, 
                        (u_i4)OCFG_MAX_STRLEN, TRUE, (STATUS *) NULL);
                    if (!dsnAttrList[j].value)
                    {
                        insertErrorMsg(pinst, SQL_HANDLE_INSTALLER, 
                            II_OUT_OF_MEM, ODBC_ERROR_OUT_OF_MEM, NULL, 0);
                        goto error_exit;
                    }
                }
                getConfigEntry( dsnH, dsnname, attrCount, attrList, 
                    &status );
                if (status != OK)
                {
                    insertErrorMsg(pinst, SQL_HANDLE_INSTALLER, II_GENERAL_ERR,
                        ODBC_ERROR_GENERAL_ERR, "Cannot get DSN entries", 
                        status);
                    goto error_exit;
                }
                for (i=0;i<argCount;i++)
                {
                    for (j=0;j<attrCount;j++)
                    {
                        if (dsnAttrList[j].id == argAttrList[i].id)
                            STcopy(argAttrList[i].value, dsnAttrList[j].value); 
                    } /* for (j = 0; j < attrCount; j++) */
                    MEfree(argAttrList[i].value);
                } /* for (i=0;i<argCount;i++) */
                setConfigEntry( dsnH, dsnname, attrCount,
                    attrList, &status );
                if (status != OK) 
                {
                    insertErrorMsg(pinst, SQL_HANDLE_INSTALLER, II_GENERAL_ERR,
                        ODBC_ERROR_GENERAL_ERR, "Cannot modify DSN entry", 
                        status);
                    goto error_exit;
                }
                for (j = 0; j < attrCount; j++)
                    MEfree((PTR)dsnAttrList[j].value);
            } /* if (attrCount */
            else
            {
                insertErrorMsg(pinst, SQL_HANDLE_INSTALLER, II_GENERAL_ERR,
                    ODBC_ERROR_GENERAL_ERR, "No attributes to modify", status);
            }
            for (i = 0; i < count; i++)
                MEfree((PTR)dsnList[i]);
            MEfree((PTR)dsnList);
        }  /* if (!count) */
    } /* switch (fRequest) */
    goto exit_OK;

error_exit:
    if (dsnH)
        closeConfig(dsnH, &status);
    if (drvH)
        closeConfig(drvH, &status);
    return FALSE;

exit_OK:
    if (dsnH)
        closeConfig(dsnH, &status);
    if (drvH)
        closeConfig(drvH, &status);
    return TRUE;
}
#endif /* ifndef NT_GENERIC */

SQLRETURN SQL_API SQLInstallerError(
    SQLSMALLINT iError,
    SQLINTEGER *pfErrorCode,
    SQLCHAR *lpszErrorMsg,
    SQLSMALLINT cbErrorMsgMax,
    SQLSMALLINT *pcbErrorMsg)
{
    pINST pinst = &IIodbc_cb.inst;

    if (iError != 1)
         return SQL_NO_DATA;

    if (pinst->errHdr.error_count == 0)
         return SQL_NO_DATA;

    applyLock(SQL_HANDLE_IIODBC_CB, pinst);
    *pfErrorCode = pinst->errHdr.rc;
    STcopy(pinst->errHdr.err[0].messageText, lpszErrorMsg);
    *pcbErrorMsg = STlength(lpszErrorMsg); 
    releaseLock(SQL_HANDLE_IIODBC_CB, pinst);
    return SQL_SUCCESS;
}

/*
** Name: 	initLock - Initialize semaphore.
** 
** Description: 
**              initLock initializes a semaphore mutex assciated with the
**              provided handle.
**
** Inputs:
**              handle - CLI input handle.
**              handleType - env, dbc, stmt, or desc.
**
** Outputs: 
**              None.
**
** Returns: 
**              TRUE if successful, FALSE otherwise.
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
BOOL initLock( i2 handleType, SQLHANDLE handle )
{
    pENV penv;
    pDBC pdbc;
    pSTMT pstmt;
    pDESC pdesc;

    if (!handle && handleType != SQL_HANDLE_IIODBC_CB)
        return FALSE;

    switch (handleType)
    {
        case SQL_HANDLE_IIODBC_CB:
            if (MUi_semaphore( &IIodbc_cb.sem ) != OK)
                return FALSE; 
            break;

        case SQL_HANDLE_ENV:
            penv = handle;
            if (MUi_semaphore( &penv->hdr.sem ) != OK)
                return FALSE; 
            break;

        case SQL_HANDLE_DBC:
            pdbc = handle;
            if (MUi_semaphore( &pdbc->hdr.sem ) != OK)
                return FALSE; 
            break;

        case SQL_HANDLE_STMT:
            pstmt = handle;
            if (MUi_semaphore( &pstmt->hdr.sem ) != OK)
                return FALSE; 
            break;

        case SQL_HANDLE_DESC:
            pdesc = handle;
            if (MUi_semaphore( &pdesc->hdr.sem ) != OK)
                return FALSE; 
            break;

        default:
            return FALSE;

    }
    return TRUE;
}

/*
** Name: 	applyLock - Assert a semaphore.
** 
** Description: 
**              applyLock holds a semaphore mutex assciated with the
**              provided handle.
**
** Inputs:
**              handle - CLI input handle.
**              handleType - env, dbc, stmt, or desc.
**
** Outputs: 
**              None.
**
** Returns: 
**              TRUE if successful, FALSE otherwise.
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

BOOL applyLock( i2 handleType, SQLHANDLE handle )
{
    pENV penv;
    pDBC pdbc;
    pSTMT pstmt;
    pDESC pdesc;

   if (!handle && handleType != SQL_HANDLE_IIODBC_CB)
        return FALSE;

    switch (handleType)
    {
        case SQL_HANDLE_IIODBC_CB:
            if (MUp_semaphore( &IIodbc_cb.sem ) != OK)
                return FALSE; 
            break;

        case SQL_HANDLE_ENV:
            penv = handle;
            if (MUp_semaphore( &penv->hdr.sem ) != OK)
                return FALSE; 
            break;

        case SQL_HANDLE_DBC:
            pdbc = handle;
            if (MUp_semaphore( &pdbc->hdr.sem ) != OK)
                return FALSE; 
            break;

        case SQL_HANDLE_STMT:
            pstmt = handle;
            if (MUp_semaphore( &pstmt->hdr.sem ) != OK)
                return FALSE; 
            break;

        case SQL_HANDLE_DESC:
            pdesc = handle;
            if (MUp_semaphore( &pdesc->hdr.sem ) != OK)
                return FALSE; 
            break;

        default:
            return FALSE;

    }
    return TRUE;
}

/*
** Name: 	releaseLock - Release a semaphore.
** 
** Description: 
**              releaseLock releases a semaphore mutex assciated with the
**              provided handle.
**
** Inputs:
**              handle - CLI input handle.
**              handleType - env, dbc, stmt, or desc.
**
** Outputs: 
**              None.
**
** Returns: 
**              TRUE if successful, FALSE otherwise.
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

BOOL releaseLock( i2 handleType, SQLHANDLE handle ) 
{
    pENV penv;
    pDBC pdbc;
    pSTMT pstmt;
    pDESC pdesc;

    if (!handle && handleType != SQL_HANDLE_IIODBC_CB)
        return FALSE;

    switch (handleType)
    {
        case SQL_HANDLE_IIODBC_CB:
            if (MUv_semaphore( &IIodbc_cb.sem ) != OK)
                return FALSE; 
            break;

        case SQL_HANDLE_ENV:
            penv = handle;
            if (MUv_semaphore( &penv->hdr.sem ) != OK)
                return FALSE; 
            break;

        case SQL_HANDLE_DBC:
            pdbc = handle;
            if (MUv_semaphore( &pdbc->hdr.sem ) != OK)
                return FALSE; 
            break;

        case SQL_HANDLE_STMT:
            pstmt = handle;
            if (MUv_semaphore( &pstmt->hdr.sem ) != OK)
                return FALSE; 
            break;

        case SQL_HANDLE_DESC:
            pdesc = handle;
            if (MUv_semaphore( &pdesc->hdr.sem ) != OK)
                return FALSE; 
            break;

        default:
            return FALSE;

    }
    return TRUE;
}

/*
** Name: 	deleteLock - Remove a semaphore.
** 
** Description: 
**              deleteLock removes a semaphore mutex assciated with the
**              provided handle.
**
** Inputs:
**              handle - CLI input handle.
**              handleType - env, dbc, stmt, or desc.
**
** Outputs: 
**              None.
**
** Returns: 
**              TRUE if successful, FALSE otherwise.
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

BOOL deleteLock( i2 handleType, SQLHANDLE handle )
{
    pENV penv;
    pDBC pdbc;
    pSTMT pstmt;
    pDESC pdesc;

    if (!handle && handleType != SQL_HANDLE_IIODBC_CB)
        return FALSE;

    switch (handleType)
    {
        case SQL_HANDLE_IIODBC_CB:
            if (MUr_semaphore( &IIodbc_cb.sem ) != OK)
                return FALSE; 
            break;

        case SQL_HANDLE_ENV:
            penv = handle;
            if (MUr_semaphore( &penv->hdr.sem ) != OK)
                return FALSE; 
            break;

        case SQL_HANDLE_DBC:
            pdbc = handle;
            if (MUr_semaphore( &pdbc->hdr.sem ) != OK)
                return FALSE; 
            break;

        case SQL_HANDLE_STMT:
            pstmt = handle;
            if (MUr_semaphore( &pstmt->hdr.sem ) != OK)
                return FALSE; 
            break;

        case SQL_HANDLE_DESC:
            pdesc = handle;
            if (MUr_semaphore( &pdesc->hdr.sem ) != OK)
                return FALSE; 
            break;

        default:
            return FALSE;

    }
    return TRUE;
}

