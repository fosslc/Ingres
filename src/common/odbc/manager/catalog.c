/*
** Copyright (c) 2004, 2006 Ingres Corporation 
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
** Name: catalog.c - ODBC CLI catalog functions
** 
** Description: 
** 		This file defines: 
** 
** 	        SQLColumns - Get column metadata.
**              SQLForeignKeys - Get metadata on foreign keys.
**              SQLPrimaryKeys - Get metadata on primary keys.
**              SQLProcedures - Get metadata on database procedures.
**              SQLProcedureColumns - Get metadata on dbproc parameters.
**              SQLSpecialColumns - Get metadata unique to a row.
**              SQLStatistics - Get static and index metadata for a row.
**              SQLTables - Get table metadata.	
**              SQLTablePrivileges - Get list of table privileges.
**              SQLColumnPrivileges - Get list of column privileges.
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
**   14-Jul-2006 (Ralph Loen) SIR 116385
**      Add support for SQLTablePrivileges().
**   21-jul-06 (Ralph Loen)  SIR 116410
**      Add support for SQLColumnPrivileges().
** 
*/ 

/*
** Name: 	SQLColumns - Return list of columns in a table.
** 
** Description: 
**		SQLColumns returns the list of column names in specified 
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

RETCODE SQL_API SQLColumns(
    SQLHSTMT    hstmt,
    UCHAR  * szTableQualifier,
    SWORD       cbTableQualifier,
    UCHAR  * szTableOwner,
    SWORD       cbTableOwner,
    UCHAR  * szTableName,
    SWORD       cbTableName,
    UCHAR  * szColumnName,
    SWORD       cbColumnName)
{  
    pSTMT pstmt = (pSTMT)hstmt;
    RETCODE rc, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLColumns(hstmt, szTableQualifier,
       cbTableQualifier, szTableOwner, cbTableOwner, szTableName,
       cbTableName, szColumnName, cbColumnName), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc = IIColumns(pstmt->hdr.driverHandle,
        szTableQualifier,
        cbTableQualifier,
        szTableOwner,
        cbTableOwner,
        szTableName,
        cbTableName,
        szColumnName,
        cbColumnName);

    applyLock(SQL_HANDLE_STMT, pstmt);

    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
        pstmt->hdr.state = S1;
    }
    else if (SQL_SUCCEEDED(rc))
        pstmt->hdr.state = S5;
    releaseLock( SQL_HANDLE_STMT, pstmt );
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLForeignKeys - Return list of foreign keys in a table.
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

RETCODE SQL_API SQLForeignKeys(
    SQLHSTMT    hstmt,
    UCHAR  * szPkTableQualifier,    SWORD  cbPkTableQualifier,
    UCHAR  * szPkTableOwner,        SWORD  cbPkTableOwner,
    UCHAR  * szPkTableName,         SWORD  cbPkTableName,
    UCHAR  * szFkTableQualifier,    SWORD  cbFkTableQualifier,
    UCHAR  * szFkTableOwner,        SWORD  cbFkTableOwner,
    UCHAR  * szFkTableName,         SWORD  cbFkTableName)
{   
    pSTMT pstmt = (pSTMT)hstmt;
    RETCODE rc, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLForeignKeys(
         hstmt, szPkTableQualifier,
         cbPkTableQualifier, szPkTableOwner, cbPkTableOwner, szPkTableName, 
         cbPkTableName, szFkTableQualifier, cbFkTableQualifier,
         szFkTableOwner, cbFkTableOwner, szFkTableName, cbFkTableName), 
         traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc = IIForeignKeys(pstmt->hdr.driverHandle,
       szPkTableQualifier,    
       cbPkTableQualifier,
       szPkTableOwner,        
       cbPkTableOwner,
       szPkTableName,         
       cbPkTableName,
       szFkTableQualifier,    
       cbFkTableQualifier,
       szFkTableOwner,        
       cbFkTableOwner,
       szFkTableName,         
       cbFkTableName);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
        pstmt->hdr.state = S1;
    }
    else if (SQL_SUCCEEDED(rc))
        pstmt->hdr.state = S5;
    releaseLock( SQL_HANDLE_STMT, pstmt );
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLPrimaryKeys - Return list of primary keys in a table.
** 
** Description: 
**              Returns a list of primary keys in the specified table.
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

RETCODE SQL_API SQLPrimaryKeys(
    SQLHSTMT    hstmt,
    UCHAR  * szTableQualifier,    SWORD  cbTableQualifier,
    UCHAR  * szTableOwner,        SWORD  cbTableOwner,
    UCHAR  * szTableName,         SWORD  cbTableName)
{   
    pSTMT pstmt = (pSTMT)hstmt;
    RETCODE rc, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLPrimaryKeys(hstmt, szTableQualifier,
        cbTableQualifier, szTableOwner, cbTableOwner, szTableName, 
        cbTableName), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc = IIPrimaryKeys(pstmt->hdr.driverHandle,
       szTableQualifier,    
       cbTableQualifier,
       szTableOwner,        
       cbTableOwner,
       szTableName,         
       cbTableName);
    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
        pstmt->hdr.state = S1;
    }
    else if (SQL_SUCCEEDED(rc))
        pstmt->hdr.state = S5;
    releaseLock( SQL_HANDLE_STMT, pstmt );
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLProcedures - Return list of database procedures.
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

RETCODE SQL_API SQLProcedures(
    SQLHSTMT    hstmt,
    UCHAR  * szProcQualifier,    SWORD  cbProcQualifier,
    UCHAR  * szProcOwner,        SWORD  cbProcOwner,
    UCHAR  * szProcName,         SWORD  cbProcName)
{   
    pSTMT pstmt = (pSTMT)hstmt;
    RETCODE rc, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLProcedures(hstmt, szProcQualifier,
         cbProcQualifier, szProcOwner, cbProcOwner, szProcName, cbProcName),
         traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc = IIProcedures(pstmt->hdr.driverHandle,
        szProcQualifier,    
        cbProcQualifier,
        szProcOwner,        
        cbProcOwner,
        szProcName,         
        cbProcName);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
        pstmt->hdr.state = S1;
    }
    else if (SQL_SUCCEEDED(rc))
        pstmt->hdr.state = S5;
    releaseLock( SQL_HANDLE_STMT, pstmt );
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLProcedureColumns - Return list of database procedures.
** 
** Description: 
**              SQLProcedureColumns returns the list of input and output 
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


RETCODE SQL_API SQLProcedureColumns(
    SQLHSTMT    hstmt,
    UCHAR  * szProcQualifier,    SWORD  cbProcQualifier,
    UCHAR  * szProcOwner,        SWORD  cbProcOwner,
    UCHAR  * szProcName,         SWORD  cbProcName,
    UCHAR  * szColumnName,       SWORD  cbColumnName)
{   
    pSTMT pstmt = (pSTMT)hstmt;
    RETCODE rc, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLProcedureColumns(hstmt,
         szProcQualifier, cbProcQualifier, szProcOwner, cbProcOwner, 
         szProcName, cbProcName, szColumnName, cbColumnName), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc = IIProcedureColumns(pstmt->hdr.driverHandle,
        szProcQualifier,    
        cbProcQualifier,
        szProcOwner,        
        cbProcOwner,
        szProcName,         
        cbProcName,
        szColumnName,       
        cbColumnName);
    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
        pstmt->hdr.state = S1;
    }
    else if (SQL_SUCCEEDED(rc))
        pstmt->hdr.state = S5;
    releaseLock( SQL_HANDLE_STMT, pstmt );
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLSpecialColumns - Return list of database procedures.
** 
** Description: 
**              SQLSpecialColumns retrieves the following information about 
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

RETCODE SQL_API SQLSpecialColumns(
    SQLHSTMT    hstmt,
    UWORD       fColType,
    UCHAR  * szTableQualifier,
    SWORD       cbTableQualifier,
    UCHAR  * szTableOwner,
    SWORD       cbTableOwner,
    UCHAR  * szTableName,
    SWORD       cbTableName,
    UWORD       fScope,
    UWORD       fNullable)
{   
    pSTMT pstmt = (pSTMT)hstmt;
    RETCODE rc, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLSpecialColumns( hstmt, fColType,
        szTableQualifier, cbTableQualifier, szTableOwner, cbTableOwner,
        szTableName, cbTableName, fScope, fNullable), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc = IISpecialColumns(pstmt->hdr.driverHandle,
        fColType,
        szTableQualifier,
        cbTableQualifier,
        szTableOwner,
        cbTableOwner,
        szTableName,
        cbTableName,
        fScope,
        fNullable);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
        pstmt->hdr.state = S1;
    }
    else if (SQL_SUCCEEDED(rc))
        pstmt->hdr.state = S5;  
    releaseLock( SQL_HANDLE_STMT, pstmt );
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLStatistics - Return table statistics.
** 
** Description: 
**              SQLStatistics retrieves a list of statistics about a single 
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

RETCODE SQL_API SQLStatistics(
    SQLHSTMT    hstmt,
    UCHAR  * szTableQualifier,
    SWORD  cbTableQualifier,
    UCHAR  * szTableOwner,
    SWORD  cbTableOwner,
    UCHAR  * szTableName,
    SWORD  cbTableName,
    UWORD  fUnique,
    UWORD  fAccuracy)
{   
    pSTMT pstmt = (pSTMT)hstmt;
    RETCODE rc, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLStatistics(hstmt, szTableQualifier,
        cbTableQualifier, szTableOwner, cbTableOwner, szTableName,
        cbTableName, fUnique, fAccuracy), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc = IIStatistics(pstmt->hdr.driverHandle,
        szTableQualifier,
        cbTableQualifier,
        szTableOwner,
        cbTableOwner,
        szTableName,
        cbTableName,
        fUnique,
        fAccuracy);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
        pstmt->hdr.state = S1;
    }
    else if (SQL_SUCCEEDED(rc))
        pstmt->hdr.state = S5;
    releaseLock( SQL_HANDLE_STMT, pstmt );
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLTables - Return table metadata.
** 
** Description: 
**              SQLTables returns the list of table, catalog, or schema 
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

RETCODE SQL_API SQLTables(
    SQLHSTMT    hstmt,
    UCHAR  * szTableQualifier,
    SWORD       cbTableQualifier,
    UCHAR  * szTableOwner,
    SWORD       cbTableOwner,
    UCHAR  * szTableName,
    SWORD       cbTableName,
    UCHAR  * szTableType,
    SWORD       cbTableType)
{   
    pSTMT pstmt = (pSTMT)hstmt;
    RETCODE rc, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLTables(hstmt, szTableQualifier,
         cbTableQualifier, szTableOwner, cbTableOwner, 
         szTableName, cbTableName, szTableType, cbTableType), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc = IITables(pstmt->hdr.driverHandle,
       szTableQualifier,
       cbTableQualifier,
       szTableOwner,
       cbTableOwner,
       szTableName,
       cbTableName,
       szTableType,
       cbTableType);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
        pstmt->hdr.state = S1;
    }
    else if (SQL_SUCCEEDED(rc))
        pstmt->hdr.state = S5;
    releaseLock( SQL_HANDLE_STMT, pstmt );
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLTablePrivileges - Return table columns and privileges.
** 
** Description: 
**              SQLTablePrivileges returns a list of tables and the 
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


SQLRETURN SQL_API SQLTablePrivileges(
    SQLHSTMT         hstmt,
    SQLCHAR        *szCatalogName,
    SQLSMALLINT      cbCatalogName,
    SQLCHAR        *szSchemaName,
    SQLSMALLINT      cbSchemaName,
    SQLCHAR        *szTableName,
    SQLSMALLINT      cbTableName)
{   
    pSTMT pstmt = (pSTMT)hstmt;
    RETCODE rc, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLTablePrivileges(hstmt, 
         szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, 
         szTableName, cbTableName), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc = IITablePrivileges(
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
        pstmt->hdr.state = S1;
    }
    else if (SQL_SUCCEEDED(rc))
        pstmt->hdr.state = S5;
    releaseLock( SQL_HANDLE_STMT, pstmt );
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLColumnPrivileges - Return table columns and privileges.
** 
** Description: 
**              SQLColumnPrivileges returns a list of columns and 
**              associated privileges for the specified table.
** Inputs:
**              hstmt                   StatementHandle.
**              szCatalogName           Catalog name.
**              cbCatalogName           Length of szCatalogName. 
**              szSchemaName            Schema name.
**              cbSchemaName            Length of szSchemaName. 
**              szTableName             Table name. 
**              cbTableName             Length of TableName. 
**              szColumnName            String search pattern for column names. 
**              cbColumnName            Length of szColumnName. 
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

SQLRETURN SQL_API SQLColumnPrivileges(
    SQLHSTMT           hstmt,
    SQLCHAR               *szCatalogName,
    SQLSMALLINT        cbCatalogName,
    SQLCHAR               *szSchemaName,
    SQLSMALLINT        cbSchemaName,
    SQLCHAR               *szTableName,
    SQLSMALLINT        cbTableName,
    SQLCHAR               *szColumnName,
    SQLSMALLINT        cbColumnName)
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLColumnPrivileges(hstmt,
        szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
        szTableName, cbTableName, szColumnName, cbColumnName), traceRet);

    if (validHandle(hstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    rc =  IIColumnPrivileges(
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
