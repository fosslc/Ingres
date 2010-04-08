/*
** Copyright (c) 2004 Ingres Corporation 
*/ 

#include <compat.h>
#include <gl.h>
#include <cv.h>
#include <dl.h>
#include <mu.h>
#include <er.h>
#include <lo.h>
#include <me.h>
#include <mu.h>
#include <nm.h>
#include <pc.h>
#include <pm.h>
#include <st.h>
#include <qu.h>
#include <tr.h>
#include <sql.h>
#include <sqlext.h>
#include <sqlca.h>
#include <idmseini.h>
#include <odbccfg.h>

#include <iiodbc.h>
#include <iiodbcfn.h>
#include "odbclocal.h"
#include "tracefn.h"

/* 
** Name: trace.c - Internal trace functions.
** 
** Description: 
** 		This file defines: 
** 
**	        IIodbc_initTrace - Initialize tracing.
**	        IIodbc_termTrace - Shutdown tracing.
** 
** History: 
**      23-apr-2004 (loera01)
**          Adapted from IIodbc_trace routines.
**      14-apr-2004 (loera01)
**          Ported to VMS.
**      29-jun-2004 (loera01)
**          Ported load of tracing module to non-Windows platforms.
**      03-sep-2004 (loera01)
**          In IIodbc_initTrace(), return safely if tracing is enabled and 
**          errors occur with locating or loading the trace library.
**   15-Jul-2005 (hanje04)
**	Include iiodbcfn.h and tracefn.h which are the new home for the 
**	ODBC CLI function pointer definitions.
**   17-June-2008 (Ralph Loen) Bug 120506
**      Renamed DRV_STR_CAIPT_TR_FILE as DRV_STR_ALT_TRACE_FILE.
**	24-Aug-2009 (kschendel) 121804
**	    Need pm.h for PMhost.
**   03-Dec-2009 (Ralph Loen) 
**      Add function templates for GetODBCTrace( ) and isCLI();
*/

/*
** Name: 	IIodbc_initTrace - Initialize ODBC tracing
** 
** Description: 
**              IIodbc_initTrace initializes CLI tracing.
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
** 
**              Updates global ODBC CLI control block.
**
** History: 
**    14-jun-04 (loera01)
**          Created.
**      03-sep-2004 (loera01)
**          Return safely if tracing is enabled and errors occur with
**          locating or loading the trace library.
*/ 


char *trace_syms[] = { 
    "TraceReturn",
/*    "TraceOpenLogFile",
    "TraceCloseLogFile",
    "TraceVersion",
*/
    "TraceSQLAllocConnect",
    "TraceSQLAllocEnv",
    "TraceSQLAllocStmt",
    "TraceSQLBindCol",
    "TraceSQLCancel",
    "TraceSQLColAttributes",
    "TraceSQLColAttributesW",
    "TraceSQLConnect",
    "TraceSQLConnectW",
    "TraceSQLDescribeCol",
    "TraceSQLDescribeColW",
    "TraceSQLDisconnect",
    "TraceSQLError",
    "TraceSQLErrorW",
    "TraceSQLExecDirect",
    "TraceSQLExecDirectW",
    "TraceSQLExecute",
    "TraceSQLFetch",
    "TraceSQLFreeConnect",
    "TraceSQLFreeEnv",
    "TraceSQLFreeStmt",
    "TraceSQLGetCursorName",
    "TraceSQLGetCursorNameW",
    "TraceSQLNumResultCols",
    "TraceSQLPrepare",
    "TraceSQLPrepareW",
    "TraceSQLRowCount",
    "TraceSQLSetCursorName",
    "TraceSQLSetCursorNameW",
    "TraceSQLSetParam",
    "TraceSQLTransact",
    "TraceSQLColumns",
    "TraceSQLColumnsW",
    "TraceSQLDriverConnect",
    "TraceSQLDriverConnectW",
    "TraceSQLGetConnectOption",
    "TraceSQLGetConnectOptionW",
    "TraceSQLGetData",
    "TraceSQLGetFunctions",
    "TraceSQLGetInfo",
    "TraceSQLGetInfoW",
    "TraceSQLGetStmtOption",
    "TraceSQLGetTypeInfo",
    "TraceSQLGetTypeInfoW",
    "TraceSQLParamData",
    "TraceSQLPutData",
    "TraceSQLSetConnectOption",
    "TraceSQLSetConnectOptionW",
    "TraceSQLSetStmtOption",
    "TraceSQLSpecialColumns",
    "TraceSQLSpecialColumnsW",
    "TraceSQLStatistics",
    "TraceSQLStatisticsW",
    "TraceSQLTables",
    "TraceSQLTablesW",
    "TraceSQLBrowseConnect",
    "TraceSQLBrowseConnectW",
    "TraceSQLColumnPrivileges",
    "TraceSQLColumnPrivilegesW",
    "TraceSQLDataSources",
    "TraceSQLDataSourcesW",
    "TraceSQLDescribeParam",
    "TraceSQLExtendedFetch",
    "TraceSQLForeignKeys",
    "TraceSQLForeignKeysW",
    "TraceSQLMoreResults",
    "TraceSQLNativeSql",
    "TraceSQLNativeSqlW",
    "TraceSQLNumParams",
    "TraceSQLParamOptions",
    "TraceSQLPrimaryKeys",
    "TraceSQLPrimaryKeysW",
    "TraceSQLProcedureColumns",
    "TraceSQLProcedureColumnsW",
    "TraceSQLProcedures",
    "TraceSQLProceduresW",
    "TraceSQLSetPos",
    "TraceSQLSetScrollOptions",
    "TraceSQLTablePrivileges",
    "TraceSQLTablePrivilegesW",
    "TraceSQLDrivers",
    "TraceSQLDriversW",
    "TraceSQLBindParameter",
    "TraceSQLAllocHandle",
    "TraceSQLBindParam",
    "TraceSQLCloseCursor",
    "TraceSQLColAttribute",
    "TraceSQLColAttributeW",
    "TraceSQLCopyDesc",
    "TraceSQLEndTran",
    "TraceSQLFetchScroll",
    "TraceSQLFreeHandle",
    "TraceSQLGetConnectAttr",
    "TraceSQLGetConnectAttrW",
    "TraceSQLGetDescField",
    "TraceSQLGetDescFieldW",
    "TraceSQLGetDescRec",
    "TraceSQLGetDescRecW",
    "TraceSQLGetDiagField",
    "TraceSQLGetDiagFieldW",
    "TraceSQLGetDiagRec",
    "TraceSQLGetDiagRecW",
    "TraceSQLGetEnvAttr",
    "TraceSQLGetStmtAttr",
    "TraceSQLGetStmtAttrW",
    "TraceSQLSetConnectAttr",
    "TraceSQLSetConnectAttrW",
    "TraceSQLSetDescField",
    "TraceSQLSetDescFieldW",
    "TraceSQLSetDescRec",
    "TraceSQLSetEnvAttr",
    "TraceSQLSetStmtAttr",
    "TraceSQLSetStmtAttrW",
    "TraceSQLAllocHandleStd",
    "TraceSQLAllocHandleStdW",
    "TraceSQLBulkOperations",
    NULL
};

static i2 is_cli = 0;
i4 GetODBCTrace( );
BOOL isCLI();

/*
** Name: IIodbc_initTrace
**
** Description:
**     This function initializes the tracing capability in API.
**
** Return value:
**     none
**
** Re-eneventcy:
**	This function does not update shared memory.
**
** History:
**     23-apr-2004 (loera01)
**        Created.
*/

void IIodbc_initTrace( )
{
    char	*retVal;
    STATUS	status;
    CL_ERR_DESC	err_code;
    char traceValue[OCFG_MAX_STRLEN]; 
    char *winFile = "CAIIODTR";
    char *traceFile;
    LOCATION loc;
    CL_ERR_DESC clerr;
    PTR dlHandle;

    NMgtAt( "II_ODBC_TRACE", &retVal );

    if ( retVal  &&  *retVal ) 
        CVal( retVal, &IIodbc_cb.odbc_trace_level );

    retVal = NULL;

    NMgtAt( "II_ODBC_LOG", &retVal );

    if ( retVal  &&  *retVal ) 
    {
        IIodbc_cb.odbc_trace_file = (char *)
           MEreqmem( 0, STlength(retVal) + 16, FALSE, &status );
        if (IIodbc_cb.odbc_trace_file)
        {
            STcopy( retVal, IIodbc_cb.odbc_trace_file );
        }
    }

    if (!IIodbc_cb.odbc_trace_level)
    {
        if (SQLGetPrivateProfileString (DRV_STR_ODBC, DRV_STR_TRACE, "", 
	    traceValue, 6, ODBCINST_INI) )
        {
            CVupper(traceValue);
            if (traceValue[0] == '1' || traceValue[0] == 'T'
	        || traceValue[0] == 'Y')
                IIodbc_cb.odbc_trace_level = 1;
        }
    }	   
    if (! IIodbc_cb.odbc_trace_file && IIodbc_cb.odbc_trace_level)
    {
        if (SQLGetPrivateProfileString (DRV_STR_ODBC, DRV_STR_TRACE_FILE, "", 
            traceValue, OCFG_MAX_STRLEN, ODBCINST_INI) )
        {
            IIodbc_cb.odbc_trace_file = (char *)
                MEreqmem( 0, STlength(traceValue) + 16, FALSE, &status );
            STcopy( traceValue, IIodbc_cb.odbc_trace_file );
        }	   
        else if (SQLGetPrivateProfileString (DRV_STR_ODBC, 
	    DRV_STR_ALT_TRACE_FILE, "", traceValue, OCFG_MAX_STRLEN, 
	    ODBCINST_INI) )
        {
            IIodbc_cb.odbc_trace_file = (char *)
                MEreqmem( 0, STlength(traceValue) + 16, FALSE, &status );
            STcopy( traceValue, IIodbc_cb.odbc_trace_file );
        }	   
    }

    /*
    **  Load the CLI tracing DLL or shared library.  Note that the trace
    **  code is not currently openSource.  The CLI trace binary must be
    **  downloaded from CA's openSource site.
    */
    if (IIodbc_cb.odbc_trace_file && IIodbc_cb.odbc_trace_level)
    {
#ifdef NT_GENERIC
        traceFile = winFile;
#else
        PMinit();
        status = PMload( NULL, NULL );
        if (status != OK)
            goto errorReturn;

        PMsetDefault( 0, SystemCfgPrefix );
        PMsetDefault( 1, PMhost() );
        PMsetDefault( 2, ERx("odbc") );
        status = PMget( ERx("!.trace_module_name"), &traceFile );
        if (status != OK)
            goto errorReturn;
#endif
        status = DLload( NULL, traceFile, trace_syms, &dlHandle, &clerr );
        if (status != OK)
            goto errorReturn;
    }
    else
    {
        goto errorReturn;
    }
    bind_sym( &dlHandle, "TraceReturn", &IITraceReturn );
/*    
    bind_sym( &dlHandle, "TraceOpenLogFile", &TraceOpenLogFile );
    bind_sym( &dlHandle, "TraceCloseLogFile", &TraceCloseLogFile );
    bind_sym( &dlHandle, "TraceVersion", &TraceVersion );
*/
    bind_sym( &dlHandle, "TraceSQLAllocConnect", &IITraceSQLAllocConnect );
    bind_sym( &dlHandle, "TraceSQLAllocEnv", &IITraceSQLAllocEnv );
    bind_sym( &dlHandle, "TraceSQLAllocStmt",&IITraceSQLAllocStmt );
    bind_sym( &dlHandle, "TraceSQLBindCol", &IITraceSQLBindCol );
    bind_sym( &dlHandle, "TraceSQLCancel", &IITraceSQLCancel );
    bind_sym( &dlHandle, "TraceSQLColAttributes", &IITraceSQLColAttributes );
    bind_sym( &dlHandle, "TraceSQLColAttributesW", &IITraceSQLColAttributesW );
    bind_sym( &dlHandle, "TraceSQLConnect", &IITraceSQLConnect );
    bind_sym( &dlHandle, "TraceSQLConnectW", &IITraceSQLConnectW );
    bind_sym( &dlHandle, "TraceSQLDescribeCol", &IITraceSQLDescribeCol );
    bind_sym( &dlHandle, "TraceSQLDescribeColW", &IITraceSQLDescribeColW );
    bind_sym( &dlHandle, "TraceSQLDisconnect", &IITraceSQLDisconnect );
    bind_sym( &dlHandle, "TraceSQLError", &IITraceSQLError );
    bind_sym( &dlHandle, "TraceSQLErrorW", &IITraceSQLErrorW );
    bind_sym( &dlHandle, "TraceSQLExecDirect", &IITraceSQLExecDirect );
    bind_sym( &dlHandle, "TraceSQLExecDirectW", &IITraceSQLExecDirectW );
    bind_sym( &dlHandle, "TraceSQLExecute", &IITraceSQLExecute );
    bind_sym( &dlHandle, "TraceSQLFetch", &IITraceSQLFetch );
    bind_sym( &dlHandle, "TraceSQLFreeConnect", &IITraceSQLFreeConnect );
    bind_sym( &dlHandle, "TraceSQLFreeEnv", &IITraceSQLFreeEnv );
    bind_sym( &dlHandle, "TraceSQLFreeStmt", &IITraceSQLFreeStmt );
    bind_sym( &dlHandle, "TraceSQLGetCursorName", &IITraceSQLGetCursorName );
    bind_sym( &dlHandle, "TraceSQLGetCursorNameW", &IITraceSQLGetCursorNameW );
    bind_sym( &dlHandle, "TraceSQLNumResultCols", &IITraceSQLNumResultCols );
    bind_sym( &dlHandle, "TraceSQLPrepare", &IITraceSQLPrepare );
    bind_sym( &dlHandle, "TraceSQLPrepareW", &IITraceSQLPrepareW );
    bind_sym( &dlHandle, "TraceSQLRowCount", &IITraceSQLRowCount );
    bind_sym( &dlHandle, "TraceSQLSetCursorName", &IITraceSQLSetCursorName );
    bind_sym( &dlHandle, "TraceSQLSetCursorNameW", &IITraceSQLSetCursorNameW );
    bind_sym( &dlHandle, "TraceSQLSetParam", &IITraceSQLSetParam );
    bind_sym( &dlHandle, "TraceSQLTransact", &IITraceSQLTransact);
    bind_sym( &dlHandle, "TraceSQLColumns", &IITraceSQLColumns );
    bind_sym( &dlHandle, "TraceSQLColumnsW", &IITraceSQLColumnsW );
    bind_sym( &dlHandle, "TraceSQLDriverConnect", &IITraceSQLDriverConnect );
    bind_sym( &dlHandle, "TraceSQLDriverConnectW", &IITraceSQLDriverConnectW );
    bind_sym( &dlHandle, "TraceSQLGetConnectOption", &IITraceSQLGetConnectOption );
    bind_sym( &dlHandle, "TraceSQLGetConnectOptionW", 
        &IITraceSQLGetConnectOptionW );
    bind_sym( &dlHandle, "TraceSQLGetData", &IITraceSQLGetData );
    bind_sym( &dlHandle, "TraceSQLGetFunctions", &IITraceSQLGetFunctions );
    bind_sym( &dlHandle, "TraceSQLGetInfo", &IITraceSQLGetInfo );
    bind_sym( &dlHandle, "TraceSQLGetInfoW", &IITraceSQLGetInfoW );
    bind_sym( &dlHandle, "TraceSQLGetStmtOption", &IITraceSQLGetStmtOption );
    bind_sym( &dlHandle, "TraceSQLGetTypeInfo", &IITraceSQLGetTypeInfo );
    bind_sym( &dlHandle, "TraceSQLGetTypeInfoW", &IITraceSQLGetTypeInfo );
    bind_sym( &dlHandle, "TraceSQLParamData", &IITraceSQLParamData );
    bind_sym( &dlHandle, "TraceSQLPutData", &IITraceSQLPutData );
    bind_sym( &dlHandle, "TraceSQLSetConnectOption", &IITraceSQLSetConnectOption );
    bind_sym( &dlHandle, "TraceSQLSetConnectOptionW", 
        &IITraceSQLSetConnectOptionW );
    bind_sym( &dlHandle, "TraceSQLSetStmtOption", &IITraceSQLSetStmtOption );
    bind_sym( &dlHandle, "TraceSQLSpecialColumns", &IITraceSQLSpecialColumns );
    bind_sym( &dlHandle, "TraceSQLSpecialColumnsW", &IITraceSQLSpecialColumnsW );
    bind_sym( &dlHandle, "TraceSQLStatistics", &IITraceSQLStatistics );
    bind_sym( &dlHandle, "TraceSQLStatisticsW", &IITraceSQLStatisticsW );
    bind_sym( &dlHandle, "TraceSQLTables", &IITraceSQLTables );
    bind_sym( &dlHandle, "TraceSQLTablesW", &IITraceSQLTablesW );
    bind_sym( &dlHandle, "TraceSQLBrowseConnect", &IITraceSQLBrowseConnect );
    bind_sym( &dlHandle, "TraceSQLBrowseConnectW", &IITraceSQLBrowseConnectW );
    bind_sym( &dlHandle, "TraceSQLColumnPrivileges", &IITraceSQLColumnPrivileges );
    bind_sym( &dlHandle, "TraceSQLColumnPrivilegesW", 
        &IITraceSQLColumnPrivilegesW );
    bind_sym( &dlHandle, "TraceSQLDataSources", &IITraceSQLDataSources );
    bind_sym( &dlHandle, "TraceSQLDataSourcesW", &IITraceSQLDataSourcesW );
    bind_sym( &dlHandle, "TraceSQLDescribeParam", &IITraceSQLDescribeParam );
    bind_sym( &dlHandle, "TraceSQLExtendedFetch", &IITraceSQLExtendedFetch );
    bind_sym( &dlHandle, "TraceSQLForeignKeys", &IITraceSQLForeignKeys );
    bind_sym( &dlHandle, "TraceSQLForeignKeysW", &IITraceSQLForeignKeysW );
    bind_sym( &dlHandle, "TraceSQLMoreResults", &IITraceSQLMoreResults );
    bind_sym( &dlHandle, "TraceSQLNativeSql", &IITraceSQLNativeSql );
    bind_sym( &dlHandle, "TraceSQLNativeSqlW", &IITraceSQLNativeSqlW );
    bind_sym( &dlHandle, "TraceSQLNumParams", &IITraceSQLNumParams );
    bind_sym( &dlHandle, "TraceSQLParamOptions", &IITraceSQLParamOptions );
    bind_sym( &dlHandle, "TraceSQLPrimaryKeys", &IITraceSQLPrimaryKeys );
    bind_sym( &dlHandle, "TraceSQLPrimaryKeysW", &IITraceSQLPrimaryKeysW );
    bind_sym( &dlHandle, "TraceSQLProcedureColumns", &IITraceSQLProcedureColumns );
    bind_sym( &dlHandle, "TraceSQLProcedureColumnsW", 
        &IITraceSQLProcedureColumnsW );
    bind_sym( &dlHandle, "TraceSQLProcedures", &IITraceSQLProcedures );
    bind_sym( &dlHandle, "TraceSQLProceduresW", &IITraceSQLProcedures );
    bind_sym( &dlHandle, "TraceSQLSetPos", &IITraceSQLSetPos );
    bind_sym( &dlHandle, "TraceSQLSetScrollOptions", &IITraceSQLSetScrollOptions );
    bind_sym( &dlHandle, "TraceSQLTablePrivileges", &IITraceSQLTablePrivileges );
    bind_sym( &dlHandle, "TraceSQLTablePrivilegesW", &IITraceSQLTablePrivilegesW );
    bind_sym( &dlHandle, "TraceSQLDrivers", &IITraceSQLDrivers );
    bind_sym( &dlHandle, "TraceSQLDriversW", &IITraceSQLDriversW );
    bind_sym( &dlHandle, "TraceSQLBindParameter", &IITraceSQLBindParameter );
    bind_sym( &dlHandle, "TraceSQLAllocHandle", &IITraceSQLAllocHandle );
    bind_sym( &dlHandle, "TraceSQLBindParam", &IITraceSQLBindParam );
    bind_sym( &dlHandle, "TraceSQLCloseCursor", &IITraceSQLCloseCursor );
    bind_sym( &dlHandle, "TraceSQLColAttribute", &IITraceSQLColAttribute );
    bind_sym( &dlHandle, "TraceSQLColAttributeW", &IITraceSQLColAttributeW );
    bind_sym( &dlHandle, "TraceSQLCopyDesc", &IITraceSQLCopyDesc );
    bind_sym( &dlHandle, "TraceSQLEndTran", &IITraceSQLEndTran );
    bind_sym( &dlHandle, "TraceSQLFetchScroll", &IITraceSQLFetchScroll );
    bind_sym( &dlHandle, "TraceSQLFreeHandle", &IITraceSQLFreeHandle );
    bind_sym( &dlHandle, "TraceSQLGetConnectAttr", &IITraceSQLGetConnectAttr );
    bind_sym( &dlHandle, "TraceSQLGetConnectAttrW", &IITraceSQLGetConnectAttrW );
    bind_sym( &dlHandle, "TraceSQLGetDescField", &IITraceSQLGetDescField );
    bind_sym( &dlHandle, "TraceSQLGetDescFieldW", &IITraceSQLGetDescField );
    bind_sym( &dlHandle, "TraceSQLGetDescRec", &IITraceSQLGetDescRec );
    bind_sym( &dlHandle, "TraceSQLGetDescRecW", &IITraceSQLGetDescRecW );
    bind_sym( &dlHandle, "TraceSQLGetDiagField", &IITraceSQLGetDiagField );
    bind_sym( &dlHandle, "TraceSQLGetDiagFieldW", &IITraceSQLGetDiagFieldW );
    bind_sym( &dlHandle, "TraceSQLGetDiagRec", &IITraceSQLGetDiagRec );
    bind_sym( &dlHandle, "TraceSQLGetDiagRecW", &IITraceSQLGetDiagRecW );
    bind_sym( &dlHandle, "TraceSQLGetEnvAttr", &IITraceSQLGetEnvAttr );
    bind_sym( &dlHandle, "TraceSQLGetStmtAttr", &IITraceSQLGetStmtAttr );
    bind_sym( &dlHandle, "TraceSQLGetStmtAttrW", &IITraceSQLGetStmtAttrW );
    bind_sym( &dlHandle, "TraceSQLSetConnectAttr", &IITraceSQLSetConnectAttr );
    bind_sym( &dlHandle, "TraceSQLSetConnectAttrW", &IITraceSQLSetConnectAttrW );
    bind_sym( &dlHandle, "TraceSQLSetDescField", &IITraceSQLSetDescField );
    bind_sym( &dlHandle, "TraceSQLSetDescFieldW", &IITraceSQLSetDescFieldW );
    bind_sym( &dlHandle, "TraceSQLSetDescRec", &IITraceSQLSetDescRec );
    bind_sym( &dlHandle, "TraceSQLSetEnvAttr", &IITraceSQLSetEnvAttr );
    bind_sym( &dlHandle, "TraceSQLSetStmtAttr", &IITraceSQLSetStmtAttr );
    bind_sym( &dlHandle, "TraceSQLSetStmtAttrW", &IITraceSQLSetStmtAttrW );
    bind_sym( &dlHandle, "TraceSQLAllocHandleStd", &IITraceSQLAllocHandleStd );
    bind_sym( &dlHandle, "TraceSQLAllocHandleStdW", &IITraceSQLAllocHandleStdW );
    bind_sym( &dlHandle, "TraceSQLBulkOperations", &IITraceSQLBulkOperations );

#ifndef NT_GENERIC
    bind_sym( &dlHandle, "TraceInit", &IITraceInit );
    
    status = IITraceInit();
    if (status != OK)
        goto errorReturn;
#endif /* NT_GENERIC */

    if (IIodbc_cb.odbc_trace_file && IIodbc_cb.odbc_trace_level)
        TRset_file( TR_F_OPEN, IIodbc_cb.odbc_trace_file, 
            STlength( IIodbc_cb.odbc_trace_file ), &err_code );
    return;

errorReturn:
    if (IIodbc_cb.odbc_trace_file)
    {
        MEfree(IIodbc_cb.odbc_trace_file);
        IIodbc_cb.odbc_trace_file = NULL;
    }
    IIodbc_cb.odbc_trace_level = 0;
    return;
}

/*
** Name: IIodbc_termTrace
**
** Description:
**     This function shuts down the tracing capability in API.
**
** Return value:
**     none
**
** Re-eneventcy:
**	This function does not update shared memory.
**
** History:
**     23-apr-2004 (loera01)
**        Created.
*/

void IIodbc_termTrace( )
{
    CL_ERR_DESC	    err_code;
    
	if ( IIodbc_cb.odbc_trace_file )
	    TRset_file( TR_F_CLOSE, IIodbc_cb.odbc_trace_file, 
			STlength( IIodbc_cb.odbc_trace_file ), &err_code );

	IIodbc_cb.odbc_trace_level = 0;
	IIodbc_cb.odbc_trace_file = NULL;
  
    return;
}

/*
** Name: GetODBCTrace
**
** Description:
**     This function returns the current trace setting, or -1 if no setting.
**
** Return value:
**     Current trace setting or -1.
**
** Re-eneventcy:
**	This function does not update shared memory.
**
** History:
**     25-jun-2004 (loera01)
**        Created.
*/

i4 GetODBCTrace( )
{
    static i4 trace_level = 0;

    if (trace_level)
        return trace_level;

    if (IIodbc_cb.odbc_trace_level)
        trace_level = IIodbc_cb.odbc_trace_level;
    else
        trace_level = -1;

    return trace_level;
}

/*
** Name: 	GetCLI - Get CLI static variable.
** 
** Description: 
**              GetCLI sets a static variable so that the tracing module
**              can detece whether or not it has been called from the CLI.
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
** 
**              Updates global ODBC CLI control block.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 
BOOL isCLI()
{
    return is_cli;
}

/*
** Name: 	SetCLI - Set CLI static variable.
** 
** Description: 
**              SetCLI sets a static variable so that the tracing module
**              can detece whether or not it has been called from the CLI.
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
** 
**              Updates global ODBC CLI control block.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 
void setCLI()
{
    is_cli = 1;
}
