/*
** Copyright 1992, 2004 Ingres Corporation
*/

#include <compat.h>
#include <stdarg.h>
#ifndef VMS
#include <systypes.h>
#endif
#include <sql.h>                    /* ODBC Core definitions */
#include <sqlext.h>                 /* ODBC extensions */
#include <sqlstate.h>               /* ODBC driver sqlstate codes */
#include <sqlcmd.h>

#include <cv.h> 
#include <st.h> 
#include <tr.h>
#include <me.h>
#include <cm.h>
#include <er.h>

#include <iiapi.h>
#include <erodbc.h>

#include <sqlca.h>
#include <idmsxsq.h>                /* control block typedefs */
#include <idmsutil.h>               /* utility stuff */
#include "caidoopt.h"              /* connect options */
#include "idmsoinf.h"               /* information definitions */
#include "idmsoids.h"               /* string ids */
#include <idmseini.h>               /* ini file keys */
#include "idmsodbc.h"               /* ODBC driver definitions             */

#include <idmsutil.h>               /* utility routines */
#include <idmsocvt.h>               /* ODBC conversion routines */

/*
** Name: ERROR.C
**
** Description:
**	Error routines for ODBC driver.
**
** History:
**	01-dec-1992 (rosda01)
**	    Initial coding
**	01-feb-1993 (rosda01)
**	    Conversion routines
**	03-may-1993 (rosda01)
**	    Split from ORES.
**	14-mar-1995 (rosda01)
**	    ODBC 2.0 upgrade...
**	21-jun-1996 (rosda01)
**	    Win32 port
**	02-may-1997 (thoda04)
**	    Cleanup for 16-bit compiles
**	12-may-1997 (thoda04)
**	    Added IM001 "Driver does not support this function"
**	01-jul-1997 (thoda04)
**	    Bulletproof ErrGetSqlcaMessage's pMessage
**	02-jul-1997 (tenge01)
**	    set pMessage in ErrGetSqlcaMessage 
**	07-jul-1997 (tenge01)
**	    set SQLPTR to null after utfree 
**	03-nov-1997 (tenge01)
**	    1. ErrGetSqlcaMessage, the native error code was in SQLERC
**	       not SQLCODE
**	    2. ErrSetSqlca, remove IDMS_STATE related code
**	04-dec-1997 (tenge01)
**	    conver C run-time functions to CL
**	27-feb-1998 (thoda04)
**	    Call GetTls for thread local storage
**	06-jul-1998 (thoda04)
**	    Chop driver name prefix only if "CA-"
**	28-oct-1998 (thoda04)
**	    Pick up correct ids field for err msg
**	17-feb-1999 (thoda04)
**	    Ported to UNIX
**	25-mar-1999 (thoda04)
**	    Updates for other product packaging
**	07-nov-2000 (thoda04)
**	    Convert nat's to i4
**	19-jun-2001 (thoda04)
**	    return null terminator at end of SQLSTATE
**	12-jul-2001 (somsa01)
**	    Cleaned up compiler warnings.
**	05-sep-2001 (thoda04)
**	    UnlockDbc missing from SQLGetDiagField.
**	29-oct-2001 (thoda04)
**	    Corrections to 64-bit compiler warnings change.
**	11-dec-2001 (thoda04)
**	    Additional 64-bit changes for overloaded attributes.
**      22-aug-2002 (loera01) Bug 108364
**          Return an error for GetChar() and GetCharAndLength() if presented
**          with a buffer length of zero or less.
**      09-sep-2002 (loera01) Bug 108679
**          Make sure SQLGetDiagField_internalCall() calls GetChar() with a 
**          valid maximum buffer length, else the fix for bug 108364 above 
**          will reject the call and return an error.
**      14-feb-2003 (loera01) SIR 109643
**          Minimize platform-dependency in code.  Make backward-compatible
**          with earlier Ingres versions.
**     25-jun-2003 (loera01) Bug 110448
**          In SQLGetDiagField_InternalCall(), SQLGetDiagRec_InternalCall(),
**          and GetDiagRec() do not attempt to retrieve error messages if the 
**          message buffer argument from SQLGetDiagRec() or SQLGetDiagField()
**          is set to zero.  The Driver Manager may be interested only in
**          retrieving the size of the error message.
**     17-oct-2003 (loera01)
**          Fixed unreferenced variable error.
**     20-nov-2003 (loera01)
**          Cast sqlcode as RETCODE.
**     04-dec-2003 (loera01)
**          Changed "STMT_" command flags to "CMD_".
**     18-dec-2003 (loera01)
**          Check penv before reporting status.
**	23-apr-2004 (somsa01)
**	    Include systypes.h to avoid compile errors on HP.
**	11-Jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**     27-Oct-2004 (Ralph.Loen@ca.com)
**          In ErrState(), made trace message legible.
**     15-nov-2004 (Ralph.Loen@ca.com) Bug 113462
**          Use IItrace global block to handle tracing.
**     02-dec-2004 (loera01) Bug 115582
**          Added SQL_01S07 (fractional truncation) error.
**     18-jan-2006 (loera01) SIR 115615
**          Re-formatted error messages for Ingres Corp.
**    30-Jan-2008 (Ralph Loen) SIR 119723
**          Added F_OD0170_IDS_ERR_ROW_RANGE.
**    22-Sep-2009 (hanje04) 
**	    Define null terminator, nt to pass to CMcpchar() instead of
**	    literal "\0" because it causes relocation errors when linking
**	    64bit intel OSX
**    09-Mar-2010 (Ralph Loen) Bug 123392
**          Add F_OD0171_IDS_ERR_NULL_PTR and SQLSTATE HY009 to the
**          error table in ErrState().
**     23-Apr-2010 (Ralph Loen) Bug 123629
**          Add SQLSTATE SQL_01S00.
**    12-Aug-2010 (Ralph Loen) Bug 124235
**          Remove limit of SQL_MAX_MESSAGE_LENGTH for error messages.
**          The SQLERM field of SQLCA_MSG_TYPE is now a dynamic pointer,
**          with a new field, SQLLEN, to track the message length size.
**          ErrInsertSqlcaMessage() allocates the pointer; 
**          FreeSqlcaMsgBuffers() and PopSqlcMsg() free the pointer.
**          A new function, ErrGetSqlcaMessageLen(), returns the 
**          error message length for functions that need to allocate
**          workspace buffers.
*/

/*
**  Internal function prototypes:
*/
static RETCODE             ErrGetDriverMessage(
    LPCSTR       szDriver,
    LPSTR        pMessage,
    CHAR       * szErrorMsg,
    SWORD        cbErrorMsgMax,
    SWORD      * pcbErrorMsg,
    CHAR       * rgbWork);

static RETCODE             ErrGetSqlcaMessage(
    LPCSTR       szDriver,
    SQLSMALLINT  RecNumber,
    LPSQLCA      psqlca,
    CHAR       * pSqlState,
    CHAR       * szErrorMsg,
    SWORD        cbErrorMsgMax,
    SWORD      * pcbErrorMsg,
    SDWORD     * pfNativeError,
    SQLINTEGER     * irowCurrent,    /* Current row of rowset in error*/
    SQLINTEGER     * icolCurrent,    /* Current col of row in error*/
    CHAR       * rgbWork);
    
static WORD             ErrGetSqlcaMessageLen(
    SQLSMALLINT  RecNumber,
    LPSQLCA      psqlca);

static SQLRETURN  GetDiagRec ( 
    SQLSMALLINT  HandleType,
    SQLHANDLE    Handle,
    SQLSMALLINT  RecNumber,
    CHAR        *pSqlStateParm,
    SQLINTEGER  *pNativeError,
    CHAR        *pMessageTextParm,
    SQLSMALLINT  BufferLength,
    SQLSMALLINT *pTextLength,
    SQLINTEGER      *pirowCurrent,
    SQLINTEGER      *picolCurrent);

static VOID  GetDiagFieldDynamicFunction(
    LPSTMT       pstmt,
    CHAR        *szValue,
    i4          *code);

static void Map3xSqlstateTo2x (LPENV penv, char * szState);
static const char nt[] = "\0";

/*
**  SQLError
**
**  Returns the next SQL error information.
**
**  On entry: penv         -->environment block or NULL.
**            pdbc         -->connection  block or NULL.
**            pstmt        -->statement   block or NULL.
**            szSqlState   -->location to return SQLSTATE.
**            pfNativeError-->location to return native error code.
**            szErrorMsg   -->location to return error message text.
**            cbErrorMsgMax = length of error message buffer.
**            pcbErrorMsg   = length of error message returned.
**
**  On exit:  what you would expect from the above...
**
**  Returns:  SQL_SUCCESS
**            SQL_SUCCESS_WITH_INFO
**            SQL_NO_DATA_FOUND
**            SQL_ERROR
**            SQL_INVALID_HANDLE
*/

RETCODE SQL_API SQLError(
    SQLHENV      henv,
    SQLHDBC      hdbc,
    SQLHSTMT     hstmt,
    SQLCHAR     *szSqlStateParm,
    SDWORD      *pfNativeError,
    SQLCHAR     *szErrorMsgParm,
    SWORD        cbErrorMsgMax,
    SWORD      * pcbErrorMsg)
{
    return SQLError_InternalCall(
                 henv,
                 hdbc,
                 hstmt,
                 szSqlStateParm,
                 pfNativeError,
                 szErrorMsgParm,
                 cbErrorMsgMax,
                 pcbErrorMsg);
}

RETCODE SQL_API SQLError_InternalCall(
    SQLHENV      henv,
    SQLHDBC      hdbc,
    SQLHSTMT     hstmt,
    SQLCHAR     *szSqlStateParm,
    SDWORD      *pfNativeError,
    SQLCHAR     *szErrorMsgParm,
    SWORD        cbErrorMsgMax,
    SWORD      * pcbErrorMsg)
{
    LPENV        penv  = (LPENV) henv;
    LPDBC        pdbc  = (LPDBC) hdbc;
    LPSTMT       pstmt = (LPSTMT)hstmt;
    CHAR       * szSqlState = (CHAR *)szSqlStateParm;
    CHAR       * szErrorMsg = (CHAR *)szErrorMsgParm;
    RETCODE      rc;

    if (pstmt)
    {
        if (!LockStmt (pstmt)) return SQL_INVALID_HANDLE;
    }
    else if (pdbc)
    {
        if (!LockDbc (pdbc)) return SQL_INVALID_HANDLE;
    }
    else if (penv)
    {
        LockEnv (penv);
    }

    TraceOdbc (SQL_API_SQLERROR, penv, pdbc, pstmt,
         szSqlState, pfNativeError, szErrorMsg, cbErrorMsgMax, pcbErrorMsg);

    rc = SQL_NO_DATA_FOUND;

    /*
    **  Return error for STMT handle:
    */
    if (pstmt != SQL_NULL_HSTMT)
    {
     rc = SQLGetDiagRec_InternalCall(SQL_HANDLE_STMT, (SQLHANDLE)pstmt, 1,
                        szSqlStateParm,
                        pfNativeError,
                        szErrorMsgParm,
                        cbErrorMsgMax,
                        pcbErrorMsg);
     ErrResetStmt(pstmt);   /* for testing only return 1 msg */
    }
    else
    /*
    **  Return error for DBC handle:
    */
    if (pdbc != SQL_NULL_HDBC)
    {
     rc = SQLGetDiagRec_InternalCall(SQL_HANDLE_DBC, (SQLHANDLE)pdbc, 1,
                        szSqlStateParm,
                        pfNativeError,
                        szErrorMsgParm,
                        cbErrorMsgMax,
                        pcbErrorMsg);
     ErrResetDbc(pdbc);   /* for testing only return 1 msg */
    }
    else
    /*
    **  Return error for ENV handle:
    */
    if (penv != SQL_NULL_HENV)
    {
     rc = SQLGetDiagRec_InternalCall(SQL_HANDLE_ENV, (SQLHANDLE)penv, 1,
                        szSqlStateParm,
                        pfNativeError,
                        szErrorMsgParm,
                        cbErrorMsgMax,
                        pcbErrorMsg);
     ErrResetEnv(penv);   /* for testing only return 1 msg */
    }

    if      (pstmt) 
        UnlockDbc (pstmt->pdbcOwner); 
    else if (pdbc)
        UnlockDbc (pdbc); 
    else if (penv)
        UnlockEnv (penv);

    return rc;
}


/*
**  SQLGetDiagField
**
**  SQLGetDiagField returns the current value of a field of a record of the
**                  diagnostic data structure (associated with a specified
**                  handle) that contains error, warning, and status information.
**
**  On entry:
**    HandleType [Input]
**      The type of handle to be allocated by SQLAllocHandle.
**      Must be one of the following values:
**        SQL_HANDLE_ENV
**        SQL_HANDLE_DBC
**        SQL_HANDLE_STMT
**        SQL_HANDLE_DESC
**    Handle [Input]
**    RecNumber [Input]
**    DiagIdentifier [Input]
**      Indicates the field of the diagnostic whose value is to be returned.
**    pDiagInfo [Output]
**      Pointer to a buffer in which to return the diagnostic information.
**      The data type depends on the value of DiagIdentifier.
**    BufferLength [Input]
**    pStringLength [Output]
**
**  Returns:
**    SQL_SUCCESS
**    SQL_SUCCESS_WITH_INFO
**    SQL_NO_DATA
**    SQL_INVALID_HANDLE
**    SQL_ERROR
**
*/
SQLRETURN  SQL_API SQLGetDiagField (
    SQLSMALLINT  HandleType,
    SQLHANDLE    Handle,
    SQLSMALLINT  RecNumber,
    SQLSMALLINT  DiagIdentifier,
    SQLPOINTER   pDiagInfoParm,
    SQLSMALLINT  BufferLength,
    SQLSMALLINT *pStringLength)
{
    return SQLGetDiagField_InternalCall (
                 HandleType,
                 Handle,
                 RecNumber,
                 DiagIdentifier,
                 pDiagInfoParm,
                 BufferLength,
                 pStringLength);
}

SQLRETURN  SQL_API SQLGetDiagField_InternalCall (
    SQLSMALLINT  HandleType,
    SQLHANDLE    Handle,
    SQLSMALLINT  RecNumber,
    SQLSMALLINT  DiagIdentifier,
    SQLPOINTER   pDiagInfoParm,
    SQLSMALLINT  BufferLength,
    SQLSMALLINT *pStringLength)
{
    LPENV        penv  = NULL;
    LPDBC        pdbc  = NULL;
    LPDESC       pdesc = NULL;
    LPSTMT       pstmt = NULL;
    CHAR       * pDiagInfo     = (CHAR *)   pDiagInfoParm;
    i4         * pDiagInfoi4   = (i4 *)     pDiagInfoParm;
    SQLINTEGER     * pDiagInfolen  = (SQLINTEGER *) pDiagInfoParm;
    SQLUINTEGER    * pDiagInfoulen = (SQLUINTEGER *)pDiagInfoParm;
    RETCODE      rc = SQL_SUCCESS;
    SQLCA_TYPE * psqlca;
    i4           i4temp;
    SQLINTEGER       lentemp;
    CHAR         szFunction[32];
    CHAR         SqlState[SQL_SQLSTATE_SIZE+1]="00000";
    SQLINTEGER   NativeError = 0;
    CHAR       * p;
    SQLSMALLINT  stringLength;

    switch (HandleType)
    {
    case  SQL_HANDLE_ENV:
       {
        penv  = (LPENV) Handle;
        LockEnv (penv);
        break;
       }

    case  SQL_HANDLE_DBC:
       {
        pdbc  = (LPDBC) Handle;
        if (!LockDbc (pdbc)) return SQL_INVALID_HANDLE;

        psqlca = &pdbc->sqlca;
        ErrFlushErrorsToSQLCA(pdbc);
           /* Flush DBC errors out of DBC block and into SQLCA error list */
        break;
       }

    case  SQL_HANDLE_STMT:
       {
        pstmt = (LPSTMT)Handle;
        if (!LockStmt (pstmt)) return SQL_INVALID_HANDLE;
        pdbc  = pstmt->pdbcOwner;

        psqlca = &pstmt->sqlca;
        ErrFlushErrorsToSQLCA(pstmt);
           /* Flush STMT errors out of STMT block and into SQLCA error list */
        break;
       }

    case  SQL_HANDLE_DESC:
       {
        pdesc = (LPDESC)Handle;
        if (!LockDesc (pdesc)) return SQL_INVALID_HANDLE;
        pdbc  = pdesc->pdbc;
        psqlca = &pdesc->sqlca;
        break;
       }

    default:
                               return SQL_INVALID_HANDLE;
    }    /* end switch HandleType */


    if (psqlca)
        stringLength = ErrGetSqlcaMessageLen (
                RecNumber, psqlca);
    else
        stringLength = SQL_MAX_MESSAGE_LENGTH;

    switch(DiagIdentifier)
    {
    case  SQL_DIAG_CURSOR_ROW_COUNT:   /* count of rows in cursor */
        /* An exact row count is never supported for a dynamic cursor.
           For other types of cursors, the driver can support either
           exact (SQL_CA2_CRC_EXACT) or approximate row counts 
           (SQL_CA2_CRC_APPROXIMATE), but not both. If the driver
           supports neither exact nor approximate row counts for a specific
           cursor type, the SQL_DIAG_CURSOR_ROW_COUNT field contains the
           number of rows that have been fetched so far. */
       {
        if (HandleType == SQL_HANDLE_STMT  &&
               pstmt->fCommand == CMD_INSERT  ||
               pstmt->fCommand == CMD_UPDATE  ||
               pstmt->fCommand == CMD_DELETE  )
            i4temp = pstmt->sqlca.SQLNRP;
        else
            i4temp = pstmt->crow; /*  number of rows fetched so far */
#ifdef _WIN64
        *pDiagInfolen = i4temp;
#else
        I4ASSIGN_MACRO(i4temp, *pDiagInfolen);
#endif
        break;
       }

    case  SQL_DIAG_DYNAMIC_FUNCTION:   /* SQL statement executed */
       {
        GetDiagFieldDynamicFunction(pstmt, szFunction, &i4temp);
        if (pDiagInfo)
            rc = GetChar (NULL, szFunction, pDiagInfo, BufferLength, pStringLength);
        break;
       }

    case  SQL_DIAG_DYNAMIC_FUNCTION_CODE:  /* code of SQL statement executed */
       {
        GetDiagFieldDynamicFunction(pstmt, szFunction, &i4temp);
        I4ASSIGN_MACRO(i4temp, *pDiagInfoi4);
        break;
       }

    case  SQL_DIAG_NUMBER:             /* number of status records */
       {
        if (HandleType == SQL_HANDLE_ENV)           /* return ENV error count*/
            i4temp = *penv->szMessage ? 1 : 0;
        else
            i4temp = psqlca->SQLMCT;    /* return DBC, STMT. DESC error count*/
        I4ASSIGN_MACRO(i4temp, *pDiagInfoi4);
        break;
       }

    case  SQL_DIAG_RETURNCODE:         /* return code returned by function*/
       {
        break;                           /*(implemented by Driver Manager)*/
       }

    case  SQL_DIAG_ROW_COUNT:  /* number of rows affected by ins, del, or upd */
       {
        if (HandleType == SQL_HANDLE_STMT)
            i4temp = pstmt->sqlca.SQLNRP;
        else
            i4temp = -1;
#ifdef _WIN64
        *pDiagInfolen = i4temp;
#else
        I4ASSIGN_MACRO(i4temp, *pDiagInfolen);
#endif
        break;
       }

    case  SQL_DIAG_CLASS_ORIGIN:       /* document owner of SQLSTATE class*/
       {
        rc =    GetDiagRec(HandleType,
                           Handle,
                           RecNumber,
                           &SqlState[0],
                           NULL,
                           NULL,
                           stringLength,
                           NULL,
                           NULL,
                           NULL); 

        if (rc != SQL_SUCCESS)
            break;

        if (MEcmp(SqlState, "IM", 2) == 0)   
            p = "ODBC 3.0";
        else
            p = "ISO 9075";
        
        if (pStringLength && !*pStringLength)
            *pStringLength = STlength(p) + 1;
        
        if (pDiagInfo)
            rc = GetChar (NULL, p, pDiagInfo, BufferLength, pStringLength);
        break;
       }

    case  SQL_DIAG_COLUMN_NUMBER:      /* column or parameter number in err*/
       {
        lentemp = SQL_COLUMN_NUMBER_UNKNOWN;
        rc =    GetDiagRec(HandleType,
                           Handle,
                           RecNumber,
                           NULL,
                           NULL,
                           NULL,
                           stringLength,
                           NULL,
                           NULL,
                           &lentemp);
        if (rc != SQL_SUCCESS)
            break;

#ifdef _WIN64
        *pDiagInfolen = lentemp;
#else
        I4ASSIGN_MACRO(lentemp, *pDiagInfolen);
#endif
        break;
       }

    case  SQL_DIAG_CONNECTION_NAME:    /* connection name (driver defined) */
       {                               /*   just return zero-length string */
        if (pDiagInfo)
            rc = GetChar (NULL, "", pDiagInfo, BufferLength, pStringLength);
        break;
       }

    case  SQL_DIAG_MESSAGE_TEXT:       /* error/warning message text */
       {
        rc =    GetDiagRec(HandleType,
                           Handle,
                           RecNumber,
                           NULL,
                           NULL,
                           pDiagInfo,
                           stringLength,
                           NULL,
                           NULL,
                           NULL);
        break;
       }

    case  SQL_DIAG_NATIVE:             /* native error code */
       {
        rc =    GetDiagRec(HandleType,
                           Handle,
                           RecNumber,
                           NULL,
                           &NativeError,
                           NULL,
                           stringLength,
                           NULL,
                           NULL,
                           NULL);
        if (rc != SQL_SUCCESS)
            break;
        I4ASSIGN_MACRO(NativeError, *pDiagInfoi4);
        break;
       }

    case  SQL_DIAG_ROW_NUMBER:         /* row or parameter that is in error */
       {
        lentemp = SQL_ROW_NUMBER_UNKNOWN;
        rc =    GetDiagRec(HandleType,
                           Handle,
                           RecNumber,
                           NULL,
                           NULL,
                           NULL,
                           stringLength,
                           NULL,
                           &lentemp,
                           NULL);
        if (rc != SQL_SUCCESS)
            break;

#ifdef _WIN64
        *pDiagInfolen = lentemp;
#else
        I4ASSIGN_MACRO(lentemp, *pDiagInfolen);
#endif
        break;
       }

    case  SQL_DIAG_SERVER_NAME:        /* data source name */
       {                 /* must be same as SQLGetInfo(SQL_DATA_SOURCE_NAME) */
        if (!pdbc)   break;              /* safety check */
        if (pDiagInfo)
            rc = GetChar (NULL, pdbc->szDSN, pDiagInfo, BufferLength, 
                                                          pStringLength);
        break;
       }

    case  SQL_DIAG_SQLSTATE:           /* return code returned by function*/
       {
        rc =    GetDiagRec(HandleType,
                           Handle,
                           RecNumber,
                           &SqlState[0],
                           NULL,
                           NULL,
                           stringLength,
                           NULL,
                           NULL,
                           NULL);
        if (rc != SQL_SUCCESS  &&  rc != SQL_SUCCESS_WITH_INFO)
            break;
        if (pDiagInfo)
            rc = GetChar (NULL, SqlState, pDiagInfo, BufferLength, pStringLength);
        break;
       }

    case  SQL_DIAG_SUBCLASS_ORIGIN:    /* document owner of SQLSTATE subclass*/
       {static CHAR *states[] = {"01S00","01S01","01S02","01S06","01S07",
                                 "07S01","08S01","21S01","21S02","25S01",
                                 "25S02","25S03","42S01","42S02","45S01",
                                 "42S12","42S21","42S22","HY095","HY097",
                                 "HY098","HY099","HY100","HY101","HY105",
                                 "HY107","HY109","HY110","HY111","HYT00","HYT01",
                                 "IM001","IM002","IM003","IM004","IM005",
                                 "IM006","IM007","IM008","IM010","IM011","IM012"
                                };
        i2   i;

        rc =    GetDiagRec(HandleType,
                           Handle,
                           RecNumber,
                           &SqlState[0],
                           NULL,
                           NULL,
                           stringLength,
                           NULL,
                           NULL,
                           NULL);

        if (rc != SQL_SUCCESS)
            break;

        p = "ISO 9075";         /* if not in list, then return "ISO 9075" */
        for (i=0; i < sizeof(states)/sizeof(states[0]); i++)
            if (MEcmp(SqlState, states[i], 5) == 0)
               {p = "ODBC 3.0";     /* if in list. then return "ODBC 3.0" */
                break;
               }
        if (pStringLength && !*pStringLength)
            *pStringLength = STlength(p) + 1; 
        if (pDiagInfo)
            rc = GetChar (NULL, p, pDiagInfo, BufferLength, pStringLength);
        break;
       }

    }    /* end switch DiagIdentifier */

    if (HandleType == SQL_HANDLE_ENV)
         UnlockEnv (penv); 
    else UnlockDbc (pdbc); 

    return(rc);
}


/*
**  SQLGetDiagRec
**
**  SQLGetDiagRec returns the current values of multiple fields of a 
**                diagnostic record that contains error, warning, and status
**                information. Unlike SQLGetDiagField, which returns one
**                diagnostic field per call, SQLGetDiagRec returns several
**                commonly used fields of a diagnostic record, including the
**                SQLSTATE, the native error code, and the diagnostic
**                message text.
**
**  On entry:
**    HandleType [Input]
**      The type of handle to be allocated by SQLAllocHandle.
**      Must be one of the following values:
**        SQL_HANDLE_ENV
**        SQL_HANDLE_DBC
**        SQL_HANDLE_STMT
**        SQL_HANDLE_DESC
**    Handle [Input]
**    RecNumber [Input]
**    pSqlState [Output]
**    pNativeError [Output]
**      Pointer to a buffer in which to return the native error code,
**      specific to the data source. This information is contained in the
**      SQL_DIAG_NATIVE diagnostic field.
**    pMessageText [Output]
**      Pointer to a buffer in which to return the diag message text string.
**    BufferLength [Input]
**    pTextLength  [Output]
**
**  Returns:
**    SQL_SUCCESS
**    SQL_SUCCESS_WITH_INFO
**    SQL_NO_DATA
**    SQL_INVALID_HANDLE
**    SQL_ERROR
**
*/
SQLRETURN  SQL_API SQLGetDiagRec ( 
    SQLSMALLINT  HandleType,
    SQLHANDLE    Handle,
    SQLSMALLINT  RecNumber,
    SQLCHAR     *pSqlStateParm,
    SQLINTEGER  *pNativeError,
    SQLCHAR     *pMessageTextParm,
    SQLSMALLINT  BufferLength,
    SQLSMALLINT *pTextLength)
{
    return SQLGetDiagRec_InternalCall ( 
                 HandleType,
                 Handle,
                 RecNumber,
                 pSqlStateParm,
                 pNativeError,
                 pMessageTextParm,
                 BufferLength,
                 pTextLength);
}

SQLRETURN  SQL_API SQLGetDiagRec_InternalCall ( 
    SQLSMALLINT  HandleType,
    SQLHANDLE    Handle,
    SQLSMALLINT  RecNumber,
    SQLCHAR     *pSqlStateParm,
    SQLINTEGER  *pNativeError,
    SQLCHAR     *pMessageTextParm,
    SQLSMALLINT  BufferLength,
    SQLSMALLINT *pTextLength)
{
    SQLRETURN rc;

    rc = GetDiagRec(HandleType,
                    Handle,
                    RecNumber,
                    (char*)pSqlStateParm,
                    pNativeError,
                    (char*)pMessageTextParm,
                    BufferLength,
                    pTextLength,
                    NULL,   /* no current row of rowset in error */
                    NULL);  /* no current col of row in error */
    return rc;
}

/*
**  GetDiagRec
**
**  GetDiagRec returns the current values of multiple fields of a 
**                diagnostic record that contains error, warning, and status
**                information. Unlike SQLGetDiagField, which returns one
**                diagnostic field per call, SQLGetDiagRec returns several
**                commonly used fields of a diagnostic record, including the
**                SQLSTATE, the native error code, and the diagnostic
**                message text.
**
**  On entry:
**    HandleType [Input]
**      The type of handle to be allocated by SQLAllocHandle.
**      Must be one of the following values:
**        SQL_HANDLE_ENV
**        SQL_HANDLE_DBC
**        SQL_HANDLE_STMT
**        SQL_HANDLE_DESC
**    Handle [Input]
**    RecNumber [Input]
**    pSqlState [Output]
**    pNativeError [Output]
**      Pointer to a buffer in which to return the native error code,
**      specific to the data source. This information is contained in the
**      SQL_DIAG_NATIVE diagnostic field.
**    pMessageText [Output]
**      Pointer to a buffer in which to return the diag message text string.
**    BufferLength [Input]
**    pTextLength  [Output]
**    pirowCurrent [Output]      Current row of rowset in error
**    picolCurrent [Output]      Current column of rowset in error
**
**  Returns:
**    SQL_SUCCESS
**    SQL_SUCCESS_WITH_INFO
**    SQL_NO_DATA
**    SQL_INVALID_HANDLE
**    SQL_ERROR
**
*/
static SQLRETURN  GetDiagRec (
    SQLSMALLINT  HandleType,
    SQLHANDLE    Handle,
    SQLSMALLINT  RecNumber,
    CHAR        *pSqlState,
    SQLINTEGER  *pNativeError,
    CHAR        *pMessageText,
    SQLSMALLINT  BufferLength,
    SQLSMALLINT *pTextLength,
    SQLINTEGER      *pirowCurrent,
    SQLINTEGER      *picolCurrent)
{
    LPENV        penv;
    LPDBC        pdbc  = NULL;
    LPDESC       pdesc;
    LPSTMT       pstmt;
    RETCODE      rc;
    CHAR         *rgbWork;
    LPCSTR       szDriver;
    SQLCA_TYPE * psqlca;

    SQLSMALLINT len = SQL_MAX_MESSAGE_LENGTH;

    switch (HandleType)
    {
    case  SQL_HANDLE_ENV:
        penv  = (LPENV) Handle;
        if (!penv)
             return SQL_INVALID_HANDLE;
        LockEnv (penv);
        break;

    case  SQL_HANDLE_DBC:
        pdbc  = (LPDBC) Handle;
        if (!LockDbc (pdbc)) return SQL_INVALID_HANDLE;

        psqlca = &pdbc->sqlca;
        ErrFlushErrorsToSQLCA(pdbc);
           /* Flush DBC errors out of DBC block and into SQLCA error list */
        break;

    case  SQL_HANDLE_STMT:
        pstmt = (LPSTMT)Handle;
        if (!LockStmt (pstmt)) return SQL_INVALID_HANDLE;
        pdbc  = pstmt->pdbcOwner;

        psqlca = &pstmt->sqlca;
        ErrFlushErrorsToSQLCA(pstmt);
           /* Flush STMT errors out of STMT block and into SQLCA error list */
        break;

    case  SQL_HANDLE_DESC:
        pdesc = (LPDESC)Handle;
        if (!LockDesc (pdesc)) return SQL_INVALID_HANDLE;
        pdbc  = pdesc->pdbc;
        psqlca = &pdesc->sqlca;
        break;

    default:
        return SQL_INVALID_HANDLE;
    }    /* end switch HandleType */

    /*
    **  Assume no errors to report
    */
    if (pSqlState)
        MEcopy ("00000", 6, pSqlState);  /* SQL_SQLSTATE_SIZE + null term */
    if (pNativeError)
        *pNativeError = 0;
    if (pMessageText && BufferLength)
        CMcpychar(nt,pMessageText);

    switch ((SQLSMALLINT)pTextLength)
    {
    case SQL_IS_UINTEGER:
    case SQL_IS_INTEGER:
    case SQL_IS_USMALLINT:
    case SQL_IS_SMALLINT:
    break;

    default:
    if (pTextLength)
        len = *pTextLength = ErrGetSqlcaMessageLen (
            RecNumber, psqlca);
        len += 50;  /* Allow for error msg prefix */
            break;
    }

    rgbWork = MEreqmem(0, len, TRUE, NULL);
    
    rc = SQL_NO_DATA;

    if (HandleType == SQL_HANDLE_ENV)     /* return ENV errors */
    {
        if (RecNumber > 1)
            return SQL_NO_DATA;
        if (pSqlState)    /* return SqlState to application */
            MEcopy (penv->szSqlState, 5, pSqlState);
        szDriver = "";  /* Don't know driver yet for ENV errors... */
        rc = ErrGetDriverMessage (szDriver, penv->szMessage, 
                pMessageText, BufferLength, pTextLength, rgbWork);
        UnlockEnv (penv);
        return rc;
    }

                               /* return DBC, STMT, DESC errors */
    szDriver = pdbc->szDriver;
    rc = ErrGetSqlcaMessage (
                szDriver, RecNumber, psqlca, pSqlState,
                pMessageText, BufferLength, pTextLength, pNativeError,
                pirowCurrent, picolCurrent, rgbWork);

    if (pSqlState  &&  (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO))
        Map3xSqlstateTo2x (pdbc->penvOwner, pSqlState);

    MEfree((PTR)rgbWork);
    UnlockDbc (pdbc); 

    return rc;
}


/*
**  ErrFlushErrorsToSQLCA
**
**  Flush STMT errors out of STMT block and into SQLCA error list.
**
**  On entry: szDriver     -->driver name, "Ingres", ...
**            pMessage     -->message in ENV, DBC, or STMT.
**            szErrorMsg   -->location to return error message text.
**            cbErrorMsgMax = length of error message buffer.
**            pcbErrorMsg  -->location to return length of error message.
**            rgbWork      -->buffer to build message in.
**
**  On exit:  Message is returned to ODBC user
**            and driver message is cleared.
**
**  Returns:  SQL_SUCESS            message returned
**            SQL_SUCESS_WITH_INFO  message truncated
**            SQL_NO_DATA_FOUND     no message to return
*/

void ErrFlushErrorsToSQLCA(
    LPVOID       lpv)
{
    UDWORD       cv;
    LPSTMT       pstmt = NULL;
    SQLCA      * psqlca;                 /* SQL Communication area */

    /*
    **  Figure out whether a connection or statement error:
    */
    if      (STcompare (((LPSTMT)lpv)->szEyeCatcher, "STMT*") == 0)
    {
        psqlca       = &(pstmt=(LPSTMT)lpv)->sqlca;
    }
    else if (STcompare (((LPDBC)lpv)->szEyeCatcher, "DBC*") == 0)
    {
        psqlca       = &((LPDBC)lpv)->sqlca;
    }
    else return;   /* should never happen but let's bulletproof */
    /*
    **  Flush stmt conversion errors and warnings to SQLCA error list
    */
    if (pstmt  &&  pstmt->fCvtError)
        while ((cv = ErrSetCvtMessage(pstmt)) != CVT_SUCCESS)
           pstmt->fCvtError &= ~cv;  /* turn off the convert error bit list */

    /*
    **  Add dummy message for DML returning SQLCODE = 100:
    */
        if (psqlca->SQLCODE == 100)
        {
            ErrState (SQL_01000, lpv, F_OD0051_IDS_ERR_NOMATCH);
            psqlca->SQLCODE = 0;
        }

    return;
}

/*
**  ErrGetDriverMessage
**
**  Return error message from ENV, DBC, or STMT.
**
**  On entry: szDriver     -->driver name, "Ingres", ...
**            pMessage     -->message in ENV, DBC, or STMT.
**            szErrorMsg   -->location to return error message text.
**            cbErrorMsgMax = length of error message buffer.
**            pcbErrorMsg  -->location to return length of error message.
**            rgbWork      -->buffer to build message in.
**
**  On exit:  Message is returned to ODBC user
**            and driver message is cleared.
**
**  Returns:  SQL_SUCESS            message returned
**            SQL_SUCESS_WITH_INFO  message truncated
**            SQL_NO_DATA_FOUND     no message to return
*/

static RETCODE             ErrGetDriverMessage(
    LPCSTR       szDriver,
    LPSTR        pMessage,
    CHAR       * szErrorMsg,
    SWORD        cbErrorMsgMax,
    SWORD      * pcbErrorMsg,
    CHAR       * rgbWork)
{
    RETCODE rc;
    char         msgprefix[100];

    if (*pMessage == 0)
        return SQL_NO_DATA_FOUND;

    if (MEcmp(szDriver,"CA-",3)==0  ||  MEcmp(szDriver,"CA ",3)==0)
        szDriver += 3;   /* skip past "CA-" prefix of driver name */
    STcopy(ERget(F_OD0107_OPING_ODBC_DRIVER), msgprefix);
    if (MEcmp(msgprefix, "***", 3)==0)  /* if msg not available */
       STcopy("[Ingres][%s ODBC Driver]", msgprefix);

    STprintf (rgbWork, msgprefix, szDriver);
    STcat (rgbWork, pMessage);

    if (szErrorMsg)
        rc = GetChar (NULL, rgbWork, szErrorMsg, cbErrorMsgMax, pcbErrorMsg);

    return rc;
}


/*
**  ErrGetSqlcaMessage
**
**  Return error message from SQLCA in DBC or STMT.
**
**  On entry: szDriver     -->Driver name ("Ingres", etc.)
**            RecNumber     = record number desired
**            psqlca       -->SQLCA.
**            ppMessage    -->-->next message in SQLCA.
**            szErrorMsg   -->location to return error message text.
**            cbErrorMsgMax = length of error message buffer.
**            pcbErrorMsg  -->location to return length of error message.
**            rgbWork      -->buffer to build message in.
**
**  On exit:  Message is returned to user and
**            next message, if any, is queued up.
**
**  Returns:  SQL_SUCESS            message returned
**            SQL_SUCESS_WITH_INFO  message truncated
**            SQL_NO_DATA_FOUND     no message to return
*/

static RETCODE             ErrGetSqlcaMessage(
    LPCSTR       szDriver,
    SQLSMALLINT  RecNumber,
    LPSQLCA      psqlca,
    CHAR       * pSqlState,
    CHAR       * szErrorMsg,
    SWORD        cbErrorMsgMax,
    SWORD      * pcbErrorMsg,
    SDWORD     * pfNativeError,
    SQLINTEGER     * pirowCurrent,   /* Current row of rowset in error*/
    SQLINTEGER     * picolCurrent,   /* Current col of rowset in error*/
    CHAR       * rgbWork)
{
    RETCODE      rc = SQL_SUCCESS;
    i4           cb;
    OFFSET_TYPE  cbId;
    LPSQLCAMSG   psqlcamsg;
    CHAR       * pMessage;
    char         msgprefix[100];

    if (RecNumber < 1  ||  RecNumber > psqlca->SQLMCT)
        return SQL_NO_DATA_FOUND;

    for (psqlcamsg = psqlca->SQLPTR; psqlcamsg != NULL;
         psqlcamsg = psqlcamsg->SQLPTR)
        if (--RecNumber == 0)
           break;

    if (!psqlcamsg)
        return SQL_NO_DATA_FOUND;

    /*
    **  Extract next message from SQLCA message buffer, if any.
    **  If no message text, build one here.
    **  Return message and point to next, if any and not truncated.
    */
    if (MEcmp(szDriver,"CA-",3)==0  ||  MEcmp(szDriver,"CA ",3)==0)
        szDriver += 3;   /* skip past "CA-" prefix of driver name */

    if (psqlcamsg->isaDriverMsg)
    {
        STcopy(ERget(F_OD0107_OPING_ODBC_DRIVER), msgprefix);
        if (MEcmp(msgprefix, "***", 3)==0)  /* if msg not available */
           STcopy("[Ingres][%s ODBC Driver]", msgprefix);
        STprintf (rgbWork, msgprefix, szDriver);
    }
    else
    {
        STcopy(ERget(F_OD0108_OPING_ODBC_DRIVER1), msgprefix);
        if (MEcmp(msgprefix, "***", 3)==0)  /* if msg not available */
           STcopy("[Ingres][%s ODBC Driver][%s]", msgprefix);
        STprintf (rgbWork,msgprefix, szDriver, szDriver);
    }

    cbId = (OFFSET_TYPE)STlength (rgbWork);

    pMessage = psqlcamsg->SQLERM;        /* pMessage-> len byte and message   */
    cb = psqlcamsg->SQLLEN;              /* cb = msg len;  pMessage-> message */
    if (cb == 0)                         /* if no msg test for some reason */
    {                                 /*    add some to indicate none   */
        STprintf (&rgbWork[cbId], ERget(F_OD0052_IDS_ERR_NOMESSAGE), 
                                  psqlcamsg->SQLERC);
    }
    else                                 /* tack on the msg text to the prefix */
    {
        MEcopy (pMessage, cb, &rgbWork[cbId]);  /* copy msg after [Ingres][...] prefix*/
        rgbWork[cbId + cb] = '\0';              /* null term the full message     */
    }
    if (szErrorMsg)
        rc = GetChar (NULL, rgbWork, szErrorMsg, cbErrorMsgMax, pcbErrorMsg);
                                            /* return full message to user    */
   else
       rgbWork[0] = '0';

    /*
    **  Return SQLSTATE and Extended Reason Code
    */
    if (pSqlState)
        MEcopy(psqlcamsg->SQLSTATE, SQL_SQLSTATE_SIZE, pSqlState);

    if (pfNativeError)
       *pfNativeError = psqlcamsg->SQLERC;

    if (pirowCurrent)
       *pirowCurrent  = psqlcamsg->irowCurrent ? psqlcamsg->irowCurrent :
                                                 SQL_ROW_NUMBER_UNKNOWN;

    if (picolCurrent)
       *picolCurrent  = psqlcamsg->icolCurrent ? psqlcamsg->icolCurrent :
                                                 SQL_COLUMN_NUMBER_UNKNOWN;

    return rc;
}

/*
**  FreeSqlcaMsgBuffers 
*/
void FreeSqlcaMsgBuffers(LPSQLCAMSG * lpsqlcamsg)
{
	LPSQLCAMSG lpsqlcamsg_crnt=*lpsqlcamsg;
	LPSQLCAMSG lpsqlcamsg_next;

	while (lpsqlcamsg_crnt)
	{
		lpsqlcamsg_next = lpsqlcamsg_crnt->SQLPTR;
                if (lpsqlcamsg_crnt->SQLERM)
                    MEfree((PTR)lpsqlcamsg_crnt->SQLERM);
		MEfree((PTR)lpsqlcamsg_crnt);
		lpsqlcamsg_crnt = lpsqlcamsg_next;
	}
	*lpsqlcamsg = NULL;
	return;
}

/*
** Name: ErrGetMessageLen
**
** Description:
**      Return the length of the diagnostic message according to the 
**      record number.
**
** Input:                  RecordNumber - diagnostic record number.
**                         psqlca - pointer to the SQLCA message block.
**
** Output:                 None.
**
** Return value:           Message length if SQLLEN is set in the message
**                         block, otherwise SQL_MAX_MESSAGE_LENGTH.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** History:
**   12-Aug-2010 (Ralph Loen) Bug 124235
**      Created.
*/

static WORD             ErrGetSqlcaMessageLen(
    SQLSMALLINT  RecNumber,
    LPSQLCA      psqlca)
{
    LPSQLCAMSG   psqlcamsg;

    if (RecNumber < 1  ||  RecNumber > psqlca->SQLMCT)
        return SQL_MAX_MESSAGE_LENGTH;

    for (psqlcamsg = psqlca->SQLPTR; psqlcamsg != NULL;
         psqlcamsg = psqlcamsg->SQLPTR)
        if (--RecNumber == 0)
           break;

    if (!psqlcamsg)
        return SQL_MAX_MESSAGE_LENGTH;

    if (psqlcamsg->isaDriverMsg)
    {
        return SQL_MAX_MESSAGE_LENGTH;
    }

    return (psqlcamsg->SQLLEN ? psqlcamsg->SQLLEN : SQL_MAX_MESSAGE_LENGTH);
}

/*
**  ErrResetDbc
**
**  Reset connection error status.
**
**  On entry: pdbc-->DBC block.
**
**  On exit:  SQLSTATE and error message cleared in DBC block.
**
**  Returns:  nothing.
*/

void ErrResetDbc(
    LPDBC   pdbc)
{
    STcopy ("00000", pdbc->szSqlState );
    pdbc->szMessage[0]  = 0;
    pdbc->sqlca.SQLCODE = 0;
    pdbc->sqlca.SQLERC  = 0;
    pdbc->sqlca.SQLMCT  = 0;
    pdbc->pMessage  = NULL;
    FreeSqlcaMsgBuffers(&(pdbc->sqlca.SQLPTR));
    return;
}


/*
**  ErrResetDesc
**
**  Reset descriptor error status.
**
**  On entry: pdesc-->DESC block.
**
**  On exit:  SQLSTATE and error message cleared in DESC block.
**
**  Returns:  nothing.
*/

void ErrResetDesc(
    LPDESC   pdesc)
{
    STcopy ("00000", pdesc->szSqlState );
    pdesc->szMessage[0]  = 0;
    pdesc->sqlca.SQLCODE = 0;
    pdesc->sqlca.SQLERC  = 0;
    pdesc->sqlca.SQLMCT  = 0;
    FreeSqlcaMsgBuffers(&(pdesc->sqlca.SQLPTR));
    return;
}


/*
**  ErrResetEnv
**
**  Reset environment error status.
**
**  On entry: penv-->ENV block.
**
**  On exit:  SQLSTATE and error message cleared in ENV block.
**
**  Returns:  nothing.
*/

void ErrResetEnv(
    LPENV   penv)
{
    STcopy ("00000", penv->szSqlState);
    penv->szMessage[0] = 0;
    return;
}


/*
**  ErrResetStmt
**
**  Reset statement error status if not an internal call.
**
**  On entry: pstmt->STMT block.
**
**  On exit:  SQLSTATE, error messages, error flags, and SQLCA
**            cleared in STMT block.
**
**  Returns:  nothing.
*/

void ErrResetStmt(
    LPSTMT  pstmt)
{
    if ((pstmt->fStatus & STMT_INTERNAL) == 0)
    {
        STcopy ("00000", pstmt->szSqlState);
        pstmt->szMessage[0]  = 0;
        pstmt->szError[0]    = 0;
        pstmt->fCvtError     = 0;
        pstmt->sqlca.SQLCODE = 0;
        pstmt->sqlca.SQLERC  = 0;
        pstmt->sqlca.SQLMCT  = 0;
        pstmt->pMessage = NULL;
        pstmt->irowCurrent = 0;   /* row in rowset in error if problem */
        pstmt->icolCurrent = 0;   /* column in error if problem */
    }
    FreeSqlcaMsgBuffers(&(pstmt->sqlca.SQLPTR));
    return;
}


/*
**  ErrSetCvtMessage
**
**  Insert SQLSTATE and message for column conversion error:
**
**  On entry: pstmt   -->STMT block.
**
**  On exit:  SQLSTATE and error message set in stmt block.
**
**  Returns:  CVT bit flag return code.
*/

UDWORD ErrSetCvtMessage(
    LPSTMT        pstmt)
{
    if (pstmt->fCvtError & CVT_NOT_CAPABLE)
    {
        ErrState (SQL_HYC00, pstmt);  /* optional feature not implemented */
        return CVT_NOT_CAPABLE;
    }
    else
    if (pstmt->fCvtError & CVT_INVALID)
    {
        ErrState (SQL_07006, pstmt);  /*restricted data type attribute vilation*/
        return CVT_INVALID;
    }
    else
    if (pstmt->fCvtError & CVT_ERROR)
    {
        ErrState (SQL_22018, pstmt);  /* invalid character value for cast */
        return CVT_ERROR;
    }
    else
    if (pstmt->fCvtError & CVT_OUT_OF_RANGE)
    {
        ErrState (SQL_22003, pstmt);  /* numeric value out of range */
        return CVT_OUT_OF_RANGE;
    }
    else
    if (pstmt->fCvtError & CVT_DATE_OVERFLOW)
    {
        ErrState (SQL_22008, pstmt);  /* datetime field overflow */
        return CVT_DATE_OVERFLOW;
    }
    else
    if (pstmt->fCvtError & CVT_STRING_TRUNC)
    {
        ErrState (SQL_22001, pstmt);
        return CVT_STRING_TRUNC;
    }
    else
    if (pstmt->fCvtError & CVT_TRUNCATION)
    {
        ErrState (SQL_01004, pstmt);
        return CVT_TRUNCATION;
    }
    else
    if (pstmt->fCvtError & CVT_FRAC_TRUNC)
    {
        ErrState (SQL_01S07,pstmt);
        return CVT_FRAC_TRUNC;
    }
    else
        return CVT_SUCCESS;
}


/*
**  ErrSetSqlca
**
**  ErrSetSqlca is usually called as part of SQLCA_ERROR macro
**  Set error info from SQLCA into DBC or STMT.
**  Update session status if an error.
**
**  On entry: psqlca    -->SQLCA.
**            szSqlState-->location to return SQLSTATE.
**            ppMessage -->location to return first message in SQLCA, if any.
**            psess     -->session block containing status
**
**  On exit:  Message count and pointer set.
**
**  Returns:  SQL_SUCCESS
**            SQL_SUCCESS_WITH_INFO
**            SQL_ERROR
*/

RETCODE ErrSetSqlca(
    LPSQLCA       psqlca,
    CHAR        * szSqlState,
    LPSTR       * pMessage,
    LPSESS        psess)
{
    /*
    **  Assume no message, and we're done if success:
    */
    *pMessage = NULL;
    if (psqlca->SQLCODE == 0)
        return SQL_SUCCESS;

    /*
    **  Point to first message, if any:
    */
    if (psqlca->SQLMCT > 0)
	{
            if (psqlca->SQLPTR)
                *pMessage = psqlca->SQLPTR->SQLERM;
	}

    STcopy (psqlca->SQLSTATE, szSqlState);
    if (psqlca->SQLCODE < 0)
        return SQL_ERROR;

    return SQL_SUCCESS_WITH_INFO;
}


/*
**  ErrSetToken
**
**  Save token causing syntax error in STMT.
**
**  Called after PREPARE or EXECUTE IMMEDIATE.
**
**  On entry: pstmt   -->STMT block.
**            szSqlStr-->syntax string from caller.
**
**  On exit:  Syntax error is saved, if any.
**
**  Returns:  SQL_SUCCESS
**            SQL_ERROR
*/

VOID ErrSetToken(
    LPSTMT        pstmt,
    CHAR       *  szSqlStr,
    SDWORD        cbSqlStr)
{
    CHAR      * sz;
    size_t      cb;

    TRACE_INTL (pstmt, "ErrSetToken");

    MEfill (sizeof (pstmt->szError), 0, pstmt->szError);

    if (pstmt->sqlca.SQLERC < 6000 || pstmt->sqlca.SQLERC > 6999)
        pstmt->sqlca.SQLSER = 0;
    else
    {
        if (pstmt->sqlca.SQLSER > cbSqlStr)
            pstmt->sqlca.SQLSER = cbSqlStr;

        /*
        **  Stash what we think is the token in error:
        */
        if (pstmt->sqlca.SQLSER > 0)
        {
            sz = szSqlStr + pstmt->sqlca.SQLSER - 1;
            cb = UMIN (cbSqlStr - pstmt->sqlca.SQLSER + 1, sizeof (pstmt->szError) - 1);
            STlcopy (sz, pstmt->szError, cb);
        }
    }
    return;
}


/*
**  ErrState
**
**  Set SQLSTATE and error message in DBC or STMT
**
**  On entry: err = error code (index into STATE table)
**            lpv-->ENV, DBC, or STMT
**            ...   optional parms
**
**  Returns:  SQL_ERROR
**            SQL_SUCCESS_WITH_INFO
*/
RETCODE ErrState (UINT err, LPVOID lpv, ...)
{
    DECLTEMP(rgbTemp[LEN_TEMP]) /* Win32 only */
    UWORD       fTrace = 0;
    CHAR      * szSqlState;
    CHAR      * szMessage;
    va_list     marker;
    UINT        ids, u0, u1;
    LPSTR       lpsz;
    HINSTANCE   hinst;
    int         i  = 0;
    RETCODE     rc = SQL_ERROR;
    CHAR      * szM;
    char        szTemp[MSGLEN];
    SQLCA      *psqlca;                 /* SQL Communication area */
    /*
    **  Driver message SQL STATE table:
    */
    static const struct
    {
        UINT    err;                    /* internal error code */
        LPCSTR  sz;                     /* return value */
        UINT    ids;                    /* message string resource id */
    }
    STATE[] =
    {
        SQL_01000, "01000", 0,
        SQL_01004, "01004", F_OD0072_IDS_ERR_TRUNCATION,
        SQL_01S00, "01S00", F_OD0163_IDS_ERR_KEYWORD,
        SQL_01S02, "01S02", F_OD0057_IDS_ERR_OPTION_CHANGE,
        SQL_01S05, "01S05", F_OD0030_IDS_ERR_CANCEL_FREE,
        SQL_01S07, "01S07", F_OD0167_IDS_ERR_FRAC_TRUNC,
        SQL_07001, "07001", F_OD0059_IDS_ERR_PARM_LIST,
        SQL_07006, "07006", F_OD0073_IDS_ERR_TYPE_ATTRIBUT,
        SQL_22001, "22001", F_OD0071_IDS_ERR_TRUNC_STRING,
        SQL_22002, "22002", F_OD0050_IDS_ERR_NOINDICATOR,
        SQL_22003, "22003", F_OD0062_IDS_ERR_RANGE,         /* numeric value out of range */
        SQL_22018, "22018", F_OD0028_IDS_ERR_ASSIGNMENT,
        SQL_22008, "22008", F_OD0037_IDS_ERR_DATE_OVERFLOW,
        SQL_22026, "22026", F_OD0049_IDS_ERR_MISMATCH,
        SQL_23000, "23000", F_OD0032_IDS_ERR_CONSTRAINT,
        SQL_24000, "24000", F_OD0036_IDS_ERR_CURSOR_STATE,
        SQL_25000, "25000", F_OD0067_IDS_ERR_STATE,
        SQL_34000, "34000", F_OD0034_IDS_ERR_CURSOR_INVALI,
        SQL_3C000, "3C000", F_OD0033_IDS_ERR_CURSOR_DUP,
        SQL_42000, "42000", 0,
        SQL_IM001, "IM001", F_OD0077_IDS_ERR_NOT_SUPPORTED,
        SQL_IM007, "IM007", F_OD0053_IDS_ERR_NOPROMPT,
        SQL_IM009, "IM009", F_OD0070_IDS_ERR_TRANSDLL,
        SQL_HY000, "HY000", 0,
        SQL_HY001, "HY001", F_OD0000_IDS_ERR_STORAGE,
        SQL_07009, "07009", F_OD0031_IDS_ERR_COLUMN,        /* invalid descriptor index */
        SQL_HY004, "HY004", F_OD0074_IDS_ERR_TYPE_INVALID,
        SQL_HY007, "HY007", F_OD0066_IDS_ERR_SEQUENCE,      /* ?associated stmt is not prepared*/
        SQL_HY008, "HY008", F_OD0029_IDS_ERR_CANCELED,      /* operation canceled */
        SQL_HY009, "HY009", F_OD0171_IDS_ERR_NULL_PTR, /* Inv use null ptr */
        SQL_HY019, "HY019", F_OD0062_IDS_ERR_RANGE,  /* ?non-char and non-binary data sent in pieces*/
        SQL_HY024, "HY024", F_OD0027_IDS_ERR_ARGUMENT,
        SQL_HY010, "HY010", F_OD0066_IDS_ERR_SEQUENCE,      /* function sequence error*/
        SQL_HY011, "HY011", F_OD0047_IDS_ERR_INVALID_NOW,   /* attribute cannot be set now */
        SQL_HY015, "HY015", F_OD0035_IDS_ERR_CURSOR_NONE,   /* no cursor name available */
        SQL_HY090, "HY090", F_OD0048_IDS_ERR_LENGTH,        /* invalid string or buffer length */
        SQL_HY091, "HY091", F_OD0039_IDS_ERR_DESCRIPTOR,    /* invalid descriptor field identifier */
        SQL_HY092, "HY092", F_OD0056_IDS_ERR_OPTION,        /* invalid attribute/option identifier*/
        SQL_S1094, "S1094", F_OD0064_IDS_ERR_SCALE,         /* invalid scale (2.x only) */
        SQL_HY096, "HY096", F_OD0045_IDS_ERR_INFO_TYPE,     /* invalid information type */
        SQL_HY104, "HY104", F_OD0061_IDS_ERR_PRECISION,     /* invalid precision or scale */
        SQL_HY105, "HY105", F_OD0060_IDS_ERR_PARM_TYPE,     /* invalid parameter type */
        SQL_HYC00, "HYC00", F_OD0054_IDS_ERR_NOT_CAPABLE,   /* optional feature not implemented */
        SQL_S1DE0, "S1DE0", F_OD0042_IDS_ERR_EXEC_DATA,
        SQL_HY021, "HY021", F_OD0039_IDS_ERR_DESCRIPTOR,   /* ???? temp ????*/
        SQL_HY106, "HY106", F_OD0027_IDS_ERR_ARGUMENT,   /* ???? temp ???? Fetch type out of range*/
        SQL_07009, "07009", F_OD0027_IDS_ERR_ARGUMENT,   /* ???? temp ???? Invalid descriptor index*/
        SQL_HY107, "HY107",  F_OD0170_IDS_ERR_ROW_RANGE,
        0,         "HY000", F_OD0046_IDS_ERR_INVALID
    };

    /*
    **  Figure out whether a connection or statement error:
    */
    if (STcompare (((LPSTMT)lpv)->szEyeCatcher, "STMT*") == 0)
    {
        fTrace      =  IItrace.fTrace;
        szSqlState  =  ((LPSTMT)lpv)->szSqlState;
        szMessage   =  ((LPSTMT)lpv)->szMessage;
        psqlca      = &((LPSTMT)lpv)->sqlca;
        psqlca->irowCurrent =
                       (DWORD)((LPSTMT)lpv)->irowCurrent; /* Current row of rowset*/
        psqlca->icolCurrent =
                       ((LPSTMT)lpv)->icolCurrent; /* Current col of row*/
    }
    else if (STcompare (((LPDBC)lpv)->szEyeCatcher, "DBC*") == 0)
    {
        fTrace      =  IItrace.fTrace;
        szSqlState  =  ((LPDBC)lpv)->szSqlState;
        szMessage   =  ((LPDBC)lpv)->szMessage;
        psqlca      = &((LPDBC)lpv)->sqlca;
    }
    else if (STcompare (((LPDBC)lpv)->szEyeCatcher, "ENV*") == 0)
    {
        fTrace      =  IItrace.fTrace;
        szSqlState  =  ((LPENV)lpv)->szSqlState;
        szMessage   =  ((LPENV)lpv)->szMessage;
        psqlca      =  NULL;
    }
    else if (STcompare (((LPDESC)lpv)->szEyeCatcher, "DESC*") == 0)
    {
        szSqlState  =  ((LPDESC)lpv)->szSqlState;
        szMessage   =  ((LPDESC)lpv)->szMessage;
        psqlca      = &((LPDESC)lpv)->sqlca;
    }
    else
    {
        TRdisplay (ERROR_MSG, ERget(F_OD0109_INVALID_ADDRESS));
        return SQL_ERROR;
    }

    /*
    **  Find SQLSTATE in table:
    */
    while (STATE[i].err && STATE[i].err != err)
        i++;
    STcopy (STATE[i].sz, szSqlState);

    /*
    **  Something for bugs:
    */
    STprintf (rgbTemp, "ErrState (%.8lx, %s)\n", lpv, szSqlState);
    if (fTrace >= ODBC_IN_TRACE)
        TRdisplay(rgbTemp);

    /*
    **  Error specific processing:
    */
    va_start (marker, lpv);
    switch (STATE[i].err)
    {
    case SQL_01000:                     /* information */

        ids = va_arg (marker, UINT);    /* text from caller only */
        /*LoadString (hModule, ids, szMessage, MSGLEN); */
        szM = ERget(ids); 
        STcopy(szM,szMessage); 
        rc = SQL_SUCCESS_WITH_INFO;
        break;

    case SQL_01004:                     /* truncation warning */
    case SQL_01S02:                     /* option value changed */
    case SQL_01S05:                     /* Cancel = Free (Close) */
    case SQL_01S07:

        /*LoadString (hModule, STATE[i].ids, szMessage, MSGLEN); */
        szM = ERget(STATE[i].ids);
        STcopy(szM,szMessage);
        rc = SQL_SUCCESS_WITH_INFO;
        break;

    case SQL_42000:                     /* syntax or security */

        ids = va_arg (marker, UINT);    /* text from caller only */
        /*LoadString (hModule, ids, szMessage, MSGLEN); */
        szM = ERget(ids); 
        STcopy(szM,szMessage);
        break;

    case SQL_HY000:                     /* general error */

        ids = va_arg (marker, UINT);    /* text from caller only */
        switch (ids)
        {
        case F_OD0063_IDS_ERR_RESOURCE:          /* user defined resource */

            u0 = va_arg (marker, UINT); /* resource id */
            u1 = va_arg (marker, UINT); /* resource type */
            STprintf (szMessage, ERget(ids), u0, u1);
            break;

        default:

            /*LoadString (hModule, ids, szMessage, MSGLEN); */
            szM = ERget(ids); 
            STcopy(szM,szMessage);
            break;
        }
        break;

    case SQL_HY001:                     /* storage */

        ids = va_arg (marker, UINT);    /* parm from caller */
        /*LoadString (hModule, ids, rgbTemp, MSGLEN); */
        szM =  ERget((ER_MSGID) STATE[i].ids);
        STcopy(szM,szTemp);
        szM =  ERget((ER_MSGID) ids);
        STprintf (szMessage, szTemp, szM);
        break;

    case SQL_IM009:                     /* translation dll load error */

        lpsz  = va_arg (marker, LPSTR);
        hinst = va_arg (marker, HINSTANCE);
        STprintf (szMessage, ERget(STATE[i].ids), lpsz, hinst);
        break;

    case 0:                             /* not found */

        STprintf (szMessage, ERget(STATE[i].ids), err);
        break;

    default:                            /* text from table */

        /*LoadString (hModule, STATE[i].ids, szMessage, MSGLEN); */
        szM = ERget(STATE[i].ids); 
        STcopy(szM,szMessage);
        break;
    }
    va_end (marker);

    if (psqlca)   /* if sqlca is present, insert onto message list */
       {
         RETCODE sqlcode;
         long sqlerc;
         sqlcode = psqlca->SQLCODE;   /* save sqlcode */
         sqlerc  = psqlca->SQLERC;    /* save sqlerc */
         psqlca->SQLCODE = rc;        /* use inherent code of state */
         if (psqlca->SQLCODE == 100)  /* if SQL_NO_DATA */
             psqlca->SQLERC  =  100;  /*    then set erc to 100 */
         else
             psqlca->SQLERC  = 0;     /* no extended reason code */
         InsertSqlcaMsg(psqlca, szMessage, szSqlState, TRUE);
         psqlca->SQLERC =  sqlerc;    /* restore sqlerc */
         psqlca->SQLCODE = sqlcode;   /* restore sqlcode */
       }

    return rc;
}

/*
**  GetChar
**
**  Return a character string, possibly truncated.
**
**  On entry: pstmt     -->statement block or NULL.
**            szValue   -->string to copy.
**            rgbValue  -->where to return string.
**            cbValueMax = length of output buffer.
**            pcbValue   = where to return length of string.
**
**  On exit:  Data is copied to buffer, possibly truncated, null terminated.
**
**  Returns:  SQL_SUCCESS           String returned successfully.
**            SQL_SUCCESS_WITH_INFO String truncated, buffer too short.
**
**  Note:     Do not use to directly return length to caller if
**            a double word is requred for pcbValue (e.g SQLGetData).
**            If DWORD lengths needed then use GetCharAndLength().
*/
RETCODE GetChar(
    LPVOID        lpv,
    CHAR      *   szValue,
    CHAR      *   rgbValue,
    SWORD         cbValueMax,
    SWORD      *  pcbValue)
{
    RETCODE rc = SQL_SUCCESS;
    OFFSET_TYPE cb;

    if (cbValueMax <= 0)    /* Zero or nonsense buffer length not allowed */
    {
        if (lpv) ErrState (SQL_22026, lpv);
        return SQL_ERROR;
    }
    if (szValue == NULL)  /* if NULL, return empty string */
        szValue = "";

    cb = (OFFSET_TYPE)STlength (szValue);

    if (pcbValue)
        *pcbValue = (SWORD)cb;

    if (rgbValue)
    {
        if (cb < cbValueMax)
            STcopy (szValue, rgbValue);
        else
        {
            STlcopy (szValue, rgbValue, cbValueMax - 1);
            rc = SQL_SUCCESS_WITH_INFO;
            if (lpv) ErrState (SQL_01004, lpv);
        }
    }
    return rc;
}

/*
**  GetCharAndLength
**
**  Return a character string, possibly truncated and 4 byte length.
**
**  On entry: lpv       -->statement or descriptor block or NULL.
**            szSource  -->string to copy.  If NULL, return empty string.
**            szTarget  -->where to return string or NULL.
**            cbValueMax = length of target buffer (including null term char).
**            pcbValue   = where to return length of string excluding null term.
**
**  On exit:  Data is copied to buffer, possibly truncated, null terminated.
**
**  Returns:  SQL_SUCCESS           String returned successfully.
**            SQL_SUCCESS_WITH_INFO String truncated, buffer too short.
**
**  Note:     Same as GetChar() funtion but with a DWORD lengths.
**            Called by SQLGetDescField.
*/
RETCODE GetCharAndLength(
    LPVOID        lpv,
    CHAR         *szSource,
    CHAR         *szTarget,
    SQLINTEGER    cbValueMax,
    SQLINTEGER   *pcbValue)
{
    RETCODE       rc = SQL_SUCCESS;
    SQLINTEGER    cb;

    if (cbValueMax <= 0)    /* Zero or nonsense buffer length not allowed */
    {
        if (lpv) ErrState (SQL_22026, lpv);
        return SQL_ERROR;
    }
    if (szSource == NULL)  /* if NULL, return empty string */
        szSource = "";

    cb = (SQLINTEGER)STlength (szSource);

    if (pcbValue)
        *pcbValue = cb; /* return length of untruncated string minus null term */
    if (szTarget)
    {
        if (cb < cbValueMax)
            STcopy (szSource, szTarget);
        else
        {
            STlcopy (szSource, szTarget, cbValueMax - 1);
            rc = SQL_SUCCESS_WITH_INFO;
            if (lpv) ErrState (SQL_01004, lpv);
        }
    }

    return rc;
}

/*
**  GetDiagFieldDynamicFunction
**
**  Return function name and function code for statement
**
**  On entry: pstmt     -->statement block
**            szValue   -->where to return function name
**            code      -->where to return function code
**
**  On exit:  function name and function code returned
**
**  Returns:  nothing
**
*/
static VOID  GetDiagFieldDynamicFunction(
    LPSTMT       pstmt,
    CHAR        *szValue,
    i4          *code)
{
    u_i2        len1, len2;
    CHAR       *p;
    CHAR       *q;
    CHAR        sz[32] = "";

    *szValue = '\0';                  /* default to unknown */
    *code    =   SQL_DIAG_UNKNOWN_STATEMENT;

    if (!pstmt)    /* safety check */
        return;

    if (pstmt->fCommand == CMD_SELECT)                      /* SELECT */
       {if (isApiCursorNeeded(pstmt))
           {
             STcopy("SELECT CURSOR", szValue);
             *code = SQL_DIAG_SELECT_CURSOR;
           }
        else
           {
             STcopy("SELECT", szValue);
             *code = SQL_DIAG_UNKNOWN_STATEMENT; /* no code for vanilla select*/
           }
        return;
       }
    if (pstmt->fCommand == CMD_INSERT)                      /* INSERT */
       { STcopy("INSERT", szValue);
         *code = SQL_DIAG_INSERT;
         return;
       }
    if (pstmt->fCommand == CMD_DELETE)                      /* DELETE */
       { if (pstmt->fStatus & STMT_WHERE_CURRENT_OF_CURSOR)
            {
             STcopy("DYNAMIC DELETE CURSOR", szValue);
             *code = SQL_DIAG_DYNAMIC_DELETE_CURSOR;
            }
         else
            {
             STcopy("DELETE WHERE", szValue);
             *code = SQL_DIAG_DELETE_WHERE;
            }
         return;
       }
    if (pstmt->fCommand == CMD_UPDATE)                      /* UPDATE */
       { if (pstmt->fStatus & STMT_WHERE_CURRENT_OF_CURSOR)
            {
             STcopy("DYNAMIC UPDATE CURSOR", szValue);
             *code = SQL_DIAG_DYNAMIC_UPDATE_CURSOR;
            }
         else
            {
             STcopy("UPDATE WHERE", szValue);
             *code = SQL_DIAG_UPDATE_WHERE;
            }
         return;
       }
    if (pstmt->fCommand == CMD_EXECPROC)                    /* CALL */
       { STcopy("CALL", szValue);
         *code = SQL_DIAG_CALL;
         return;
       }

    if (pstmt->szSqlStr == NULL  ||
        *pstmt->szSqlStr == '\0')     /* if we don't have a string */
        return;                       /* return unknown */

    p = ScanPastSpaces(pstmt->szSqlStr);  /* scan the 1st two words of stmt */
    q = ScanRegularIdentifier(p);
    len1 = (u_i2)(q-p);                        /* length of 1st word */
    if (len1 == 0)   return;
    MEcopy(p, len1, sz);               /* build word in sz   */

    CVupper(sz);                  /* normalize case for the keyword compare */

    if (STequal(sz, "GRANT"))                                /* GRANT */
       { STcopy("GRANT", szValue);
         *code = SQL_DIAG_GRANT;
         return;
       }
    if (STequal(sz, "REVOKE"))                               /* REVOKE */
       { STcopy("REVOKE", szValue);
         *code = SQL_DIAG_REVOKE;
         return;
       }

    if (!STequal(sz, "CREATE")  &&  /* we don't recognize command */
        !STequal(sz, "DROP")    &&  /*   if not one of these      */
        !STequal(sz, "ALTER"))
            return;                 /*   then just return now */

    p = ScanPastSpaces(q);             /* scan the 2nd word of stmt */
    q = ScanRegularIdentifier(p);
    len2 = (u_i2)(q-p);                        /* length of 2nd word */

    if (len2 == 0  ||  (len1 + len2) > 30)  return;

    sz[len1] = ' ';
    MEcopy(p, len2, sz+len1+1);        /* build 2nd word in sz   */
    CVupper(sz+len1+1);                /* normalize for the keyword compare */

    if (STequal(sz, "ALTER TABLE"))                          /* ALTER TABLE */
       { STcopy("ALTER TABLE", szValue);
         *code = SQL_DIAG_ALTER_TABLE;
         return;
       }
    if (STequal(sz, "CREATE INDEX"))                         /* CREATE INDEX */
       { STcopy("CREATE INDEX", szValue);
         *code = SQL_DIAG_CREATE_INDEX;
         return;
       }
    if (STequal(sz, "CREATE TABLE"))                         /* CREATE TABLE */
       { STcopy("CREATE TABLE", szValue);
         *code = SQL_DIAG_CREATE_TABLE;
         return;
       }
    if (STequal(sz, "CREATE VIEW"))                          /* CREATE VIEW */
       { STcopy("CREATE VIEW", szValue);
         *code = SQL_DIAG_CREATE_VIEW;
         return;
       }
    if (STequal(sz, "DROP INDEX"))                           /* DROP INDEX */
       { STcopy("DROP INDEX", szValue);
         *code = SQL_DIAG_DROP_INDEX;
         return;
       }
    if (STequal(sz, "DROP SCHEMA"))                          /* DROP SCHEMA */
       { STcopy("DROP SCHEMA", szValue);
         *code = SQL_DIAG_DROP_SCHEMA;
         return;
       }
    if (STequal(sz, "DROP TABLE"))                           /* DROP TABLE */
       { STcopy("ALTER DROP", szValue);
         *code = SQL_DIAG_DROP_TABLE;
         return;
       }
    if (STequal(sz, "DROP VIEW"))                            /* DROP VIEW */
       { STcopy("DROP VIEW", szValue);
         *code = SQL_DIAG_DROP_VIEW;
         return;
       }

    return;
}

/*
** Name: InsertSqlcaMsg 
**
** Description:
**      Insert error or information message into the SQLCA message list.
**
*/
II_VOID InsertSqlcaMsg(LPSQLCA psqlca, II_CHAR *pMsg,
    II_CHAR * pSqlState, BOOL isaDriverMsg)
{
    WORD len;
    LPSQLCAMSG lpsqlcamsg_next;
    LPSQLCAMSG lpsqlcamsg_temp;

    len = (int)STlength(pMsg);

    lpsqlcamsg_next = (LPSQLCAMSG) MEreqmem(0,sizeof(SQLCA_MSG_TYPE),TRUE,NULL);
	lpsqlcamsg_next->SQLERM = (char *) MEreqmem(0,len+1,TRUE,NULL);
    if (lpsqlcamsg_next && lpsqlcamsg_next->SQLERM)
    {
        psqlca->SQLMCT += 1;    /* increment message count */
        lpsqlcamsg_next->SQLCODE = psqlca->SQLCODE;
        lpsqlcamsg_next->SQLERC  = psqlca->SQLERC;
        lpsqlcamsg_next->irowCurrent  = psqlca->irowCurrent;  /* row in error */
        lpsqlcamsg_next->icolCurrent  = psqlca->icolCurrent;  /* col in error */
        lpsqlcamsg_next->isaDriverMsg  = isaDriverMsg;
        MEcopy(pSqlState, sizeof(lpsqlcamsg_next->SQLSTATE),
               lpsqlcamsg_next->SQLSTATE);

        lpsqlcamsg_next->SQLLEN = len;
        STlcopy(pMsg, lpsqlcamsg_next->SQLERM, len); 
        lpsqlcamsg_next->SQLPTR = NULL;
        
        /* put the new msg ptr in sqlca base */
        if (!psqlca->SQLPTR) 
            psqlca->SQLPTR = lpsqlcamsg_next;
        else
        {
            lpsqlcamsg_temp = psqlca->SQLPTR;
            while(lpsqlcamsg_temp->SQLPTR)
                lpsqlcamsg_temp = lpsqlcamsg_temp->SQLPTR;
            lpsqlcamsg_temp->SQLPTR = lpsqlcamsg_next;
        }
    }
    return; 
}

/*
** Name: PopSqlcaMsg 
**
** Description:
**      Pop the last message of the  SQLCA block.
**      Restore the SQLCODE, SQLERC, SQLSTATE to the new msg state.
**
*/
VOID PopSqlcaMsg(LPSQLCA psqlca)
{
    LPSQLCAMSG pmsg;
    LPSQLCAMSG priormsg;

          /* strip the last message and set SQLCODE, SQLERC, SQLSTATE */
    if (psqlca->SQLMCT > 0)
    {
        pmsg = psqlca->SQLPTR;      /* pmsg -> first message */
        if (psqlca->SQLMCT == 1)    /* if only one message on chain */
        {
           psqlca->SQLPTR = NULL; /*     mark the chain as empty */
           psqlca->SQLCODE = SQL_SUCCESS;
           psqlca->SQLERC  = 0;
           MEcopy("00000", sizeof(psqlca->SQLSTATE),psqlca->SQLSTATE);
        }
        else  /* find the last and 2nd last messages */
        { 
            while (pmsg->SQLPTR)
            {
                priormsg = pmsg;  /* -> message before pmsg */
                pmsg     = pmsg->SQLPTR;
            }
           priormsg->SQLPTR = NULL; /* make the 2nd last the last msg */
           psqlca->SQLCODE = priormsg->SQLCODE;  /* set the SQLCA state */
           psqlca->SQLERC  = priormsg->SQLERC;
           MEcopy(priormsg->SQLSTATE,sizeof(psqlca->SQLSTATE),
                    psqlca->SQLSTATE);
        }
        MEfree((PTR)pmsg->SQLERM);  /* free the last message in the chain */
        MEfree((PTR)pmsg);  /* free the last message block in the chain */
        psqlca->SQLMCT--;
    }  /* end if (psqlca->SQLMCT > 0) */
}

static void Map3xSqlstateTo2x (LPENV penv, char * szState)
{
    typedef struct tagSqlstateMap
    { 
        char * szState3x;
        char * szState2x;
    }  SqlstateMap;

    static const SqlstateMap sqlstatemap[] =
    {
/*     3.x      2.x     */
    {"07005", "24000"}, /* invalid cursor state */
    {"07009", "S1002"}, /* invalid descriptor index */
    {"22007", "22008"}, /* invalid datetime format */
    {"22018", "22005"}, /* data conversion error in assignment */
    {"HY000", "S1000"}, /* general error */
    {"HY001", "S1001"}, /* memory allocation error */
    {"HY003", "S1003"}, /* invalid application buffer type */
    {"HY004", "S1004"}, /* invalid SQL data type */
    {"HY007", "S1010"}, /* associated stmt is not prepared;     see also HY010*/
    {"HY008", "S1008"}, /* operation canceled */
    {"HY009", "S1009"}, /* invalid use of null pointer;         see also HY024,HY092*/
    {"HY010", "S1010"}, /* function sequence error;             see also HY007*/
    {"HY011", "S1011"}, /* attribute cannot be set now */
    {"HY015", "S1015"}, /* no cursor name available */
    {"HY019", "22003"}, /* Non-char and non-binary data sent in pieces*/
    {"HY024", "S1009"}, /* invalid attribute value;             see also HY009,HY092*/
    {"HY092", "S1009"}, /* invalid attribute/option identifier; see also HY009,HY024*/
    {"HY090", "S1090"}, /* invalid string or buffer length */
    {"HY091", "S1091"}, /* invalid descriptor field identifier */
    {"HY092", "S1092"}, /* invalid attribute/option identifier; AMBIGUOUS*/
    {"HY096", "S1096"}, /* invalid information type */
    {"HY104", "S1104"}, /* invalid precision or scale */
    {"HY105", "S1105"}, /* invalid parameter type */
    {"HYC00", "S1C00"}  /* optional feature not implemented */
    };
#define sizeofSqlstateMap sizeof(sqlstatemap)/sizeof(sqlstatemap[0])

    int i;

    if (szState == NULL)                   /* return if no SQLSTATE */
        return;

    if (memcmp(szState, "00000", 5) == 0)  /* skip search if "00000" */
        return;

    if (penv->ODBCVersion < SQL_OV_ODBC3)    /* if 2.x or before */
    {
        for (i=0; i < sizeofSqlstateMap; i++)
        {
            if (memcmp(szState, sqlstatemap[i].szState3x, SQL_SQLSTATE_SIZE) == 0)
            {
                memcpy(szState, sqlstatemap[i].szState2x, SQL_SQLSTATE_SIZE);
                break;
            }
        }  /* end for loop */
    }
    else                                      /* 3.x or later*/
    {
        if (memcmp(szState, "S1094", 5) == 0)  /* map 2.x "invalid scale" */
            memcpy(szState, "HY104", 5);       /* to 3.x "invalid p or s" */
    }

    return;
}

