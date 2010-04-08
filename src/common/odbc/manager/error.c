/*
** Copyright (c) 2004, 2009 Ingres Corporation 
*/ 

#include <compat.h>
#include <er.h>
#include <me.h>
#include <mu.h>
#include <qu.h>
#include <st.h>
#include <sql.h>                    /* ODBC Core definitions */
#include <sqlext.h>                 /* ODBC extensions */
#ifdef NT_GENERIC
#include <odbcinst.h>
#endif
#include <sqlstate.h>
#include <erodbc.h>
#include <iiodbc.h>
#include <iiodbcfn.h>
#include "odbclocal.h"
#include "tracefn.h"

/* 
** Name: error.c - Error handling routines for the ODBC CLI
** 
** Description: 
** 		This file defines: 
** 
**              SQLError - Get error information (ODBC 1.0 version).
**              SQLGetDiagField - Get error field.
**              SQLGetDiagRec - Get error information.
** 
** History: 
**   14-jun-04 (loera01)
**       Created. 
**   04-Oct-04 (hweho01)
**       Avoid compiler error on AIX platform, put include
**       files of sql.h and sqlext.h sqlstate.h after st.h.
**   17-Feb-05 (Ralph.Loen@ca.com) Sir 113913
**       Added error table for SQLInstallerError().
**   15-Jul-2005 (hanje04)
**	Include iiodbcfn.h and tracefn.h which are the new home for the 
**	ODBC CLI function pointer definitions.
**   18-Jan-2006 (loera01) Sir 115615
**      Re-formatted error messages for Ingres Corp.
**   23-Mar-2009 (Ralph Loen) Bug 121838
**      Added support for HY095 sqlstate.
*/ 

/*
**  Driver message SQL STATE table:
*/
typedef struct
{
    u_i4    err;                      /* internal error code */
    char    szSqlState[6];            /* return value */
    u_i4    msgID;                    /* message string resource id */
} SQLSTATE_ENTRY;

static const SQLSTATE_ENTRY errTable[MAX_SQLSTATE_TABLE] =
{
    SQL_01000, "01000", 0,
    SQL_01004, "01004", F_OD0072_IDS_ERR_TRUNCATION,
    SQL_01S02, "01S02", F_OD0057_IDS_ERR_OPTION_CHANGE,
    SQL_01S05, "01S05", F_OD0030_IDS_ERR_CANCEL_FREE,
    SQL_07001, "07001", F_OD0059_IDS_ERR_PARM_LIST,
    SQL_07006, "07006", F_OD0073_IDS_ERR_TYPE_ATTRIBUT,
    SQL_07009, "07009", F_OD0027_IDS_ERR_ARGUMENT,  
    SQL_22001, "22001", F_OD0071_IDS_ERR_TRUNC_STRING,
    SQL_22002, "22002", F_OD0050_IDS_ERR_NOINDICATOR,
    SQL_22003, "22003", F_OD0062_IDS_ERR_RANGE,   /* num value out of range */
    SQL_22008, "22008", F_OD0037_IDS_ERR_DATE_OVERFLOW,
    SQL_22018, "22018", F_OD0028_IDS_ERR_ASSIGNMENT,
    SQL_22026, "22026", F_OD0049_IDS_ERR_MISMATCH,
    SQL_23000, "23000", F_OD0032_IDS_ERR_CONSTRAINT,
    SQL_24000, "24000", F_OD0036_IDS_ERR_CURSOR_STATE,
    SQL_25000, "25000", F_OD0067_IDS_ERR_STATE,
    SQL_34000, "34000", F_OD0034_IDS_ERR_CURSOR_INVALI,
    SQL_3C000, "3C000", F_OD0033_IDS_ERR_CURSOR_DUP,
    SQL_42000, "42000", 0,
    SQL_IM001, "IM001", F_OD0077_IDS_ERR_NOT_SUPPORTED,
    SQL_IM002, "IM002", F_OD0156_IDS_ERR_DATA_SOURCE,
    SQL_IM003, "IM003", F_OD0101_IDS_LOADLIB_ERROR,
    SQL_IM007, "IM007", F_OD0053_IDS_ERR_NOPROMPT,
    SQL_IM009, "IM009", F_OD0070_IDS_ERR_TRANSDLL,
    SQL_HY000, "HY000", 0,
    SQL_HY001, "HY001", F_OD0158_IDS_ERR_MEM_ALLOC,
    SQL_HY004, "HY004", F_OD0074_IDS_ERR_TYPE_INVALID,
    SQL_HY007, "HY007", F_OD0066_IDS_ERR_SEQUENCE, /* stmt is not prepared*/
    SQL_HY008, "HY008", F_OD0029_IDS_ERR_CANCELED, /* operation canceled */
    SQL_HY009, "HY009", F_OD0066_IDS_ERR_SEQUENCE,  /* function seq error*/
    SQL_HY010, "HY010", F_OD0066_IDS_ERR_SEQUENCE,  /* function seq error*/
    SQL_HY011, "HY011", F_OD0047_IDS_ERR_INVALID_NOW, /* cannot be set now */
    SQL_HY013, "HY013", F_OD0159_IDS_ERR_MEM_MGT, /* memory mgt error */
    SQL_HY015, "HY015", F_OD0035_IDS_ERR_CURSOR_NONE,   /* no cursor name */
    SQL_HY016, "HY016", 0,
    SQL_HY019, "HY019", F_OD0062_IDS_ERR_RANGE,  /* data sent in pieces*/
    SQL_HY021, "HY021", F_OD0039_IDS_ERR_DESCRIPTOR,   
    SQL_HY024, "HY024", F_OD0027_IDS_ERR_ARGUMENT,
    SQL_HY090, "HY090", F_OD0048_IDS_ERR_LENGTH,  /* invalid str or buf len */
    SQL_HY091, "HY091", F_OD0039_IDS_ERR_DESCRIPTOR, /* invalid desc field id */
    SQL_HY092, "HY092", F_OD0056_IDS_ERR_OPTION,   /* invalid attr id */
    SQL_S1094, "S1094", F_OD0064_IDS_ERR_SCALE,   /* invalid scale (2.x only) */
    SQL_HY096, "HY096", F_OD0045_IDS_ERR_INFO_TYPE,  /* invalid info type */
    SQL_HY103, "HY103", F_OD0157_IDS_ERR_RETV_CODE, /* inv retrieval code */
    SQL_HY104, "HY104", F_OD0061_IDS_ERR_PRECISION,  /* invalid precision */
    SQL_HY105, "HY105", F_OD0060_IDS_ERR_PARM_TYPE,  /* invalid param type */
    SQL_HY106, "HY106", F_OD0027_IDS_ERR_ARGUMENT,   
    SQL_HYC00, "HYC00", F_OD0054_IDS_ERR_NOT_CAPABLE,  /* opt not implemented */
    SQL_S1DE0, "S1DE0", F_OD0042_IDS_ERR_EXEC_DATA,
    };

/*
**  Installer error table.
*/
typedef struct
{
    u_i4    err;                      /* internal error code */
    u_i4    odbcErr;                  /* ODBC error code */
    u_i4    msgID;                    /* message string resource id */
} INSTALLER_ENTRY;

static const INSTALLER_ENTRY instTable[MAX_INSTALLER_TABLE] =
{
    II_GENERAL_ERR, ODBC_ERROR_GENERAL_ERR, 0,
    II_INVALID_HWND, ODBC_ERROR_INVALID_HWND, F_OD0160_IDS_ERR_INVALID_HWND,
    II_INVALID_REQUEST_TYPE, ODBC_ERROR_INVALID_REQUEST_TYPE, 
        F_OD0161_IDS_ERR_REQUEST_TYPE,
    II_INVALID_NAME, ODBC_ERROR_INVALID_NAME, F_OD0162_IDS_ERR_INVALID_NAME,
    II_INVALID_KEYWORD_VALUE, ODBC_ERROR_INVALID_KEYWORD_VALUE, 
        F_OD0163_IDS_ERR_KEYWORD,
    II_REQUEST_FAILED, ODBC_ERROR_REQUEST_FAILED, 
        F_OD0164_IDS_ERR_REQ_FAILED,
    II_LOAD_LIB_FAILED, ODBC_ERROR_LOAD_LIB_FAILED, 
        F_OD0165_IDS_ERR_LOAD_LIB,
    II_OUT_OF_MEM, ODBC_ERROR_OUT_OF_MEM, 
        F_OD0166_IDS_ERR_OUT_OF_MEM
}; 

/*
** Name: 	SQLError - Return error information.
** 
** Description: 
**              SQLError returns the next error information in the error queue.
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
RETCODE SQL_API SQLError(
    SQLHENV      henv,
    SQLHDBC      hdbc,
    SQLHSTMT     hstmt,
    SQLCHAR     *szSqlStateParm,
    SDWORD      *pfNativeError,
    SQLCHAR     *szErrorMsgParm,
    SWORD        cbErrorMsgMax,
    SWORD       *pcbErrorMsg)
{
    pENV penv = (pENV)henv;
    pDBC pdbc = (pDBC)hdbc;
    pSTMT pstmt = (pSTMT)hstmt;
    char *msgFormat = "[Ingres][Ingres ODBC CLI] %s";
    char msg[SQL_MAX_MESSAGE_LENGTH];
    IIODBC_ERROR *err;
    static RecNumber = 0;
    BOOL driverError = FALSE;
    RETCODE rc = SQL_NO_DATA;
    RETCODE traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLError(henv, hdbc, hstmt,
        szSqlStateParm, pfNativeError, szErrorMsgParm, 
        cbErrorMsgMax, pcbErrorMsg), traceRet );

    if (RecNumber > IIODBC_MAX_ERROR_ROWS)
    {
         ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_NO_DATA));
         return SQL_NO_DATA;
    }

    if (!henv && !hdbc && !hstmt)
    {
         ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
         return SQL_INVALID_HANDLE;
    }

    if (penv)
    {
        if (penv->hdr.driverError)
            driverError = TRUE;
        err = &penv->errHdr.err[0];
        if (penv->errHdr.error_count && 
            err[RecNumber].messageText[0])
        {
            if (penv->errHdr.error_count < RecNumber + 1)
            {
                RecNumber = 0;
                if (!driverError)
                {
                    ODBC_TRACE(ODBC_TR_TRACE, 
                         IITraceReturn(traceRet, SQL_NO_DATA));
                    return SQL_NO_DATA;
                }
            }

            if (szErrorMsgParm)
            {
                STprintf(msg, msgFormat, err[RecNumber].messageText); 
                STcopy(msg, (char *)szErrorMsgParm);
            }
            if (szSqlStateParm)
                STcopy(err[RecNumber].sqlState, (char *)szSqlStateParm);             
            if (pcbErrorMsg)
                *pcbErrorMsg = STlength(msg);
            if (pfNativeError)
                *pfNativeError = 0;
            RecNumber++;
            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_SUCCESS));
            return SQL_SUCCESS;
        }
    }
    if (pdbc) 
    {        
        if (pdbc->hdr.driverError)
            driverError = TRUE;
        err = &pdbc->errHdr.err[0];
        if (pdbc->errHdr.error_count && 
            err[RecNumber].messageText[0])
        {
            if (pdbc->errHdr.error_count < RecNumber + 1)
            {
                RecNumber = 0;
                if (!driverError)
                {
                     ODBC_TRACE(ODBC_TR_TRACE, 
                         IITraceReturn(traceRet, SQL_NO_DATA));
                     return SQL_NO_DATA;
                }
            }

            if (szErrorMsgParm)
            {
                STprintf(msg, msgFormat, 
                    err[RecNumber].messageText); 
                STcopy(msg, (char *)szErrorMsgParm);
            }
            if (szSqlStateParm)
                STcopy(err[RecNumber].sqlState, (char *)szSqlStateParm);             
            if (pcbErrorMsg)
                *pcbErrorMsg = STlength(msg);
            if (pfNativeError)
                *pfNativeError = 0;
            RecNumber++;
            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_SUCCESS));
            return SQL_SUCCESS;
        }
    }

    if (pstmt) 
    {       
        if (pstmt->hdr.driverError)
            driverError = TRUE;
        err = &pstmt->errHdr.err[0];
        if (pstmt->errHdr.error_count && 
            err[RecNumber].messageText[0])
        {
            if (pdbc->errHdr.error_count < RecNumber + 1)
            {
                RecNumber = 0;
                if (driverError)
                {
                    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, 
                        SQL_NO_DATA));
                    return SQL_NO_DATA;
                }
            }

            if (szErrorMsgParm)
            {
                STprintf(msg, msgFormat, err[RecNumber].messageText); 
                STcopy(msg, (char *)szErrorMsgParm);
            }
            if (szSqlStateParm)
                STcopy(err[RecNumber].sqlState, (char *)szSqlStateParm);             
            if (pcbErrorMsg)
                *pcbErrorMsg = STlength(msg);
             if (pfNativeError)
                *pfNativeError = 0;
             RecNumber++;
             ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_SUCCESS));
             return SQL_SUCCESS;
        }
    }

    if (driverError)
    {
        rc = IIError(
            henv ? penv->hdr.driverHandle : NULL,
            hdbc ? pdbc->hdr.driverHandle : NULL,
            hstmt ? pstmt->hdr.driverHandle : NULL,
            szSqlStateParm,
            pfNativeError,
            szErrorMsgParm,
            cbErrorMsgMax,
            pcbErrorMsg);
    }

    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLGetDiagField - Get error diagnostic
** 
** Description: 
**              SQLGetDiagField returns the current value of a field of a 
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
SQLRETURN  SQL_API SQLGetDiagField (
    SQLSMALLINT  HandleType,
    SQLHANDLE    Handle,
    SQLSMALLINT  RecNumber,
    SQLSMALLINT  DiagIdentifier,
    SQLPOINTER   pDiagInfoParm,
    SQLSMALLINT  BufferLength,
    SQLSMALLINT *pStringLength)
{
    static i2 rec = 0;

    pENV penv = NULL;
    pDBC pdbc = NULL;
    pSTMT pstmt = NULL;
    pDESC pdesc = NULL;
    SQLHANDLE drvHandle = NULL;
    BOOL mgrError = FALSE;
    BOOL driverError = FALSE;
    i4 rowCount = 0;
    IIODBC_ERROR *err;
    RETCODE rc = SQL_NO_DATA, retCode = SQL_SUCCESS;
    RETCODE traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLGetDiagField(HandleType, Handle,
        RecNumber,DiagIdentifier, pDiagInfoParm, BufferLength, 
        pStringLength), traceRet );

    if (pDiagInfoParm)
        *(char *)pDiagInfoParm = '\0';
    if (pStringLength)
        *pStringLength = 0;

    if (RecNumber > IIODBC_MAX_ERROR_ROWS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_NO_DATA));
        return SQL_NO_DATA;
    }

    switch (HandleType)
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
        {
            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, 
                SQL_INVALID_HANDLE));
            return SQL_INVALID_HANDLE;
        }
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
        {
            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, 
                SQL_INVALID_HANDLE));
            return SQL_INVALID_HANDLE;
        }
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
        {
            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, 
                SQL_INVALID_HANDLE));
            return SQL_INVALID_HANDLE;
        }
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
                rc = IIGetDiagField (HandleType, Handle, RecNumber,
                    DiagIdentifier, pDiagInfoParm, BufferLength,
                    pStringLength);
        }
        else
        {
            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, 
                SQL_INVALID_HANDLE));
            return SQL_INVALID_HANDLE;
        }
        break;
    }

    /*
    ** Return code diag is implemented solely in the Driver Manager.
    */
    if (DiagIdentifier == SQL_DIAG_RETURNCODE)
    {
        *(RETCODE *)pDiagInfoParm = retCode;
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_SUCCESS));
        return SQL_SUCCESS;
    }

    if (mgrError)
    {
        switch (DiagIdentifier)
		{
        case SQL_DIAG_NUMBER:
            break;

        case SQL_DIAG_SQLSTATE:
            STcopy((char *)err[RecNumber - 1].sqlState,
                (char *)pDiagInfoParm);
            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_SUCCESS));
            return SQL_SUCCESS;
            break;

        case SQL_DIAG_CLASS_ORIGIN:
            if (err[RecNumber - 1].sqlState[0] == 'I' &&
                err[RecNumber - 1].sqlState[1] == 'M')
                STcopy("ODBC 3.0",(char *)pDiagInfoParm);
            else
                STcopy("ISO 9075",(char *)pDiagInfoParm);

            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_SUCCESS));
            return SQL_SUCCESS;
            break;

        case SQL_DIAG_NATIVE:
            *(i4 *)pDiagInfoParm = 0;
            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_SUCCESS));
            return SQL_SUCCESS;
            break;

        case SQL_DIAG_MESSAGE_TEXT:
            STcopy(err[RecNumber -1].messageText,
                (char *)pDiagInfoParm);
            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_SUCCESS));
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
               "IM001","eM002","IM003","IM004","IM005",
               "IM006","IM007","IM008","IM010","IM011","IM012"
                                };
            for (k = 0; k < sizeof(states) / sizeof(states[0]); k++)
            {
                if (!STcompare(err[RecNumber - 1].sqlState, states[k]))
                    supported = TRUE;
	    }
	    if (supported)
	       STcopy("ODBC 3.0",(char *)pDiagInfoParm);
            else
                STcopy("ISO 9075",(char *)pDiagInfoParm);
            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_SUCCESS));
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
        rc = IIGetDiagField (HandleType, drvHandle, RecNumber,
            DiagIdentifier, pDiagInfoParm, BufferLength,
            pStringLength);

    if (DiagIdentifier == SQL_DIAG_NUMBER)
    {
        if (driverError && drvHandle && rc == SQL_SUCCESS)
           rowCount += *(i4 *)pDiagInfoParm;

        *(i4 *)pDiagInfoParm = rowCount;
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_SUCCESS));
        return SQL_SUCCESS;
    }
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
    
}

/*
** Name: 	SQLGetDiagRec - Get error information
** 
** Description: 
**              SQLGetDiagRec returns the current values of multiple fields 
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
    pENV penv = NULL;
    pDBC pdbc = NULL;
    pSTMT pstmt = NULL;
    pDESC pdesc = NULL;
    SQLHANDLE drvHandle = NULL;
    BOOL driverError = FALSE;
    char *msgFormat = "[Ingres][Ingres ODBC CLI] %s";
    char msg[SQL_MAX_MESSAGE_LENGTH];
    IIODBC_ERROR *err;
    RETCODE rc=SQL_NO_DATA;
    RETCODE traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLGetDiagRec(HandleType, 
         Handle, RecNumber, pSqlStateParm, pNativeError, pMessageTextParm,
         BufferLength, pTextLength), traceRet );

    if (RecNumber > IIODBC_MAX_ERROR_ROWS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_NO_DATA));
        return SQL_NO_DATA;
    }

    if (RecNumber < 1)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_ERROR));
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
                    STcopy(msg, (char *)pMessageTextParm);
                }
                if (pSqlStateParm)
                    STcopy(err[RecNumber - 1].sqlState, (char *)pSqlStateParm);             
                if (pTextLength)
                    *pTextLength = STlength(msg);
                if (pNativeError)
                    *pNativeError = 0;
                ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_SUCCESS));
                return SQL_SUCCESS;
            }
            else if (!penv->hdr.driverError)
	    {
		ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_NO_DATA));
                return SQL_NO_DATA;
	    }

            if (penv->hdr.driverError)
               driverError = TRUE;
            drvHandle = (SQLHANDLE)penv->hdr.driverHandle;
        }
        else
        {
            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, 
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
                    STprintf(msg, msgFormat, 
                        err[RecNumber - 1].messageText); 
                    STcopy(msg, (char *)pMessageTextParm);
                }
                if (pSqlStateParm)
                    STcopy(err[RecNumber - 1].sqlState, (char *)pSqlStateParm);             
                if (pTextLength)
                    *pTextLength = STlength(msg);
                if (pNativeError)
                    *pNativeError = 0;
                ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_SUCCESS));
                return SQL_SUCCESS;
            }
            else if (!pdbc->hdr.driverError)
            {
	        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_NO_DATA));
	        return SQL_NO_DATA;
            }

            if (pdbc->hdr.driverError)
               driverError = TRUE;
            drvHandle = (SQLHANDLE)pdbc->hdr.driverHandle;
        }
        else
        {
            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, 
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
                    STcopy(msg, (char *)pMessageTextParm);
                }
                if (pSqlStateParm)
                    STcopy(err[RecNumber - 1].sqlState, (char *)pSqlStateParm);             
			    if (pTextLength)
                    *pTextLength = STlength(msg);
                 if (pNativeError)
                    *pNativeError = 0;
                 ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_SUCCESS));
                 return SQL_SUCCESS;
            }
            else if (!pstmt->hdr.driverError)
            {
	        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_NO_DATA));
                return SQL_NO_DATA;
            }

            if (pstmt->hdr.driverError)
               driverError = TRUE;
            drvHandle = (SQLHANDLE)pstmt->hdr.driverHandle;
        }
        else
        {
            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, 
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
                        STprintf(msg, msgFormat, 
                            err[RecNumber - 1].messageText); 
                        STcopy(msg, (char *)pMessageTextParm);
					}
                    if (pSqlStateParm)
                        STcopy(err[RecNumber - 1].sqlState, 
                            (char *)pSqlStateParm);             
                    if (pTextLength)
                        *pTextLength = STlength(msg);
                    if (pNativeError)
                        *pNativeError = 0;
                    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, 
                        SQL_SUCCESS));
                    return SQL_SUCCESS;
                }
                else if (!pdesc->hdr.driverError)
                {
	            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, 
                        SQL_NO_DATA));
		    return SQL_NO_DATA;
                }
                if (pdesc->hdr.driverError)
                    driverError = TRUE;
            }
            drvHandle = Handle;
        }
        else
        {
            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, 
                SQL_INVALID_HANDLE));
            return SQL_INVALID_HANDLE;
        }
       
        break;
    }
 
    if (driverError)
        rc = IIGetDiagRec ( HandleType, drvHandle, RecNumber,
            pSqlStateParm, pNativeError, pMessageTextParm,
            BufferLength, pTextLength);

    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	validHandle - Validate handle.
** 
** Description: 
**              validHandle checks the input handle to verify that the
**              handle is of the type specified, is not NULL, and has an
**              associated driver handle.
**
** Inputs:
**              handle - Env, dbc, stmt, or desc handle.
**              handleType - handle type.
**
** Outputs: 
**
**
** Returns: 
**              SQL_SUCCESS
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

RETCODE validHandle(SQLHANDLE handle, i2 handleType)
{
    pENV penv;
    pDBC pdbc;
    pSTMT pstmt;
    pDESC pdesc;

    if (!handle)
        return SQL_INVALID_HANDLE;

    switch (handleType)
    {
    case SQL_HANDLE_ENV:
        penv = (pENV)handle;
        if (penv->hdr.type != handleType)
            return SQL_INVALID_HANDLE;
        if (!penv->hdr.driverHandle)
            return SQL_INVALID_HANDLE;
        break;

	case SQL_HANDLE_DBC:
        pdbc = (pDBC)handle;
        if (pdbc->hdr.type != handleType)
            return SQL_INVALID_HANDLE;
        if (!pdbc->hdr.driverHandle)
            return SQL_INVALID_HANDLE;
        break;

    case SQL_HANDLE_STMT:
        pstmt = (pSTMT)handle;
        if (pstmt->hdr.type != handleType)
            return SQL_INVALID_HANDLE;
        if (!pstmt->hdr.driverHandle)
            return SQL_INVALID_HANDLE;
        break;

    case SQL_HANDLE_DESC:
        pdesc = (pDESC)handle;
        if (pdesc->hdr.type != handleType)
            return SQL_INVALID_HANDLE;
        if (!pdesc->hdr.driverHandle)
            return SQL_INVALID_HANDLE;
        break;
    }
    return SQL_SUCCESS;
}

/*
** Name: 	insertErrorMsg - Insert error information
** 
** Description: 
**              insertErrorMsg inserts CLI errors in the CLI error message
**              queue.
**
** Inputs:
**              handle - env, dbc, stmt, or desc handle.
**              handleType - Handle type.
**              sqlState - SQLSTATE of the error.
**              rc - Return code of the error.
**              errMsg - Error message.
**              status - Error message ID.
**
** Outputs: 
**              None.
**
** Returns: 
**              TRUE if successful.
**              FALSE if not.
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

BOOL insertErrorMsg(SQLHANDLE handle, i2 handleType, i2 internalErr, RETCODE rc,
    char *errMsg, STATUS status)
{
    pENV penv;
    pDBC pdbc;
    pSTMT pstmt;
    pDESC pdesc;
    pINST pinst = &IIodbc_cb.inst;
    IIODBC_ERROR *err;
    char msg[SQL_MAX_MESSAGE_LENGTH];
    i4 rec = 0;

    if (internalErr < 0 && internalErr > MAX_SQLSTATE_TABLE)
        return FALSE;

    if (handleType == SQL_HANDLE_DESC && (pdesc = getDescHandle(handle)))
        applyLock(SQL_HANDLE_DESC, pdesc);
    else
        applyLock(handleType, handle);

    switch (handleType)
    {
    case SQL_HANDLE_ENV:
        penv = (pENV)handle;
        err = &penv->errHdr.err[0];
        penv->errHdr.error_count++;
        rec = penv->errHdr.error_count;
        penv->errHdr.rc = rc;
        break; 

    case SQL_HANDLE_DBC:
        pdbc = (pDBC)handle;
        err = &pdbc->errHdr.err[0];
        pdbc->errHdr.error_count++;
        rec = pdbc->errHdr.error_count;
        pdbc->errHdr.rc = rc;
        break;

    case SQL_HANDLE_STMT:
        pstmt = (pSTMT)handle;
        err = &pstmt->errHdr.err[0];
        pstmt->errHdr.error_count++;
        rec = pstmt->errHdr.error_count;
        pstmt->errHdr.rc = rc;
        break;

    case SQL_HANDLE_DESC:
	if (!pdesc)
            return FALSE; /* No externally allocated */
		                  /* descriptor */
        err = &pdesc->errHdr.err[0];
        pdesc->errHdr.error_count++;
        rec = pdesc->errHdr.error_count;
        pdesc->errHdr.rc = rc;
        break;

    case SQL_HANDLE_INSTALLER:
        err = &pinst->errHdr.err[0];
        pinst->errHdr.error_count++;
        rec = pinst->errHdr.error_count;
        pinst->errHdr.rc = rc;
        break;

    } /* switch (handleType) */

    if (rec >= IIODBC_MAX_ERROR_ROWS)
        return FALSE;

    if (handleType == SQL_HANDLE_INSTALLER)
    switch (internalErr)
    {
    case II_GENERAL_ERR:
        STcopy(errMsg, err[rec - 1].messageText);
        return TRUE;
        break;
    default:
        STcopy(ERget(instTable[internalErr].msgID), err[rec - 1].messageText);
        return TRUE;
        break;	 
    }
    switch (internalErr)
    {
    case SQL_01000:
    case SQL_42000:
    case SQL_HY000:
    case SQL_HY095:
    case SQL_HY016:
        STcopy(errMsg, err[rec - 1].messageText);
        break;
    case SQL_IM003:
        STprintf(msg, ERget(errTable[internalErr].msgID), status);
        STcopy(msg, err[rec - 1].messageText); 
        break;
    default:
        STcopy(ERget(errTable[internalErr].msgID), err[rec - 1].messageText);
        break;	 
    }

    STcopy(errTable[internalErr].szSqlState, err[rec - 1].sqlState); 

    releaseLock(handleType, handle);

    return TRUE;
}

/*
** Name: 	resetErrorBuff - Reset error buffer
** 
** Description: 
**              resetErrorBuff clears error information for the error queue.
**
** Inputs:
**              handle - env, dbc, stmt or desc handle.
**
** Outputs: 
**              None.
**
** Returns: 
**              TRUE if successful.
**              FALSE if not.
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

BOOL resetErrorBuff( SQLHANDLE handle, i2 handleType ) 
{
    pENV penv = NULL;
    pDBC pdbc = NULL;
    pSTMT pstmt = NULL;
    pDESC pdesc = NULL;
    pINST pinst = &IIodbc_cb.inst;
    IIODBC_ERROR_HDR *errHdr;

    if (!handle)
        return FALSE;

    if (handleType == SQL_HANDLE_INSTALLER)
        applyLock(SQL_HANDLE_IIODBC_CB, handle);
    else
        applyLock(handleType, handle);

    switch (handleType)
    {
    case SQL_HANDLE_ENV:
        penv = (pENV)handle;
        penv->hdr.driverError = FALSE;
        errHdr = &penv->errHdr;
        break;

    case SQL_HANDLE_DBC:
        pdbc = (pDBC)handle;
        pdbc->hdr.driverError = FALSE;
        errHdr = &pdbc->errHdr;
        break;

    case SQL_HANDLE_STMT:
        pstmt = (pSTMT)handle;
        pstmt->hdr.driverError = FALSE;
        errHdr = &pstmt->errHdr;
        break;

    case SQL_HANDLE_DESC:
        pdesc = (pDESC)handle;
        pdesc->hdr.driverError = FALSE;
        errHdr = &pdesc->errHdr;
        break;

    case SQL_HANDLE_INSTALLER:
        errHdr = &pinst->errHdr;
        errHdr->error_count = 1;
        break;
 
    default:
        return FALSE;
        break;
    }

    if (errHdr->error_count)
        MEfill(sizeof(IIODBC_ERROR_HDR), 0, errHdr);

    errHdr->rc = SQL_SUCCESS;

    if (handleType == SQL_HANDLE_INSTALLER)
        releaseLock(SQL_HANDLE_IIODBC_CB, handle);
    else
        releaseLock(handleType, handle);

    return TRUE;
}

