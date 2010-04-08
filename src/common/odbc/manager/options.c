/*
** Copyright (c) 2004, 2007 Ingres Corporation 
*/ 
#include <compat.h>
#include <cs.h>
#include <me.h>
#include <mu.h>
#include <pc.h>
#include <qu.h>
#include <si.h>
#include <st.h>
#include <tm.h>
#include <tr.h>
#include <thcl.h>
#include <sql.h>                    /* ODBC Core definitions */
#include <sqlext.h>                 /* ODBC extensions */
#include <iiodbc.h>
#include <iiodbcfn.h>
#include "odbclocal.h"
#include "tracefn.h"

void IIodbc_timeOutThread();

/* 
** Name: options.c - Get or set attributes for ODBC CLI. 
** 
** Description: 
** 		This file defines: 
** 
**              SQLGetConnectAttr - Get connect attributes.
**              SQLGetConnectOption - Get connect attributes (ODBC 1.0 version).
**              SQLGetEnvAttr  - Get connection attributes.
**              SQLGetStmtAttr - Set statement attributes.
**              SQLGetStmtOption - Set statement attributes.
**              SQLSetConnectAttr - Set statement attributes.
**              SQLSetConnectOption - Set statement attributes (1.0 version).
**              SQLSetEnvAttr - Set statement attributes.
**              SQLSetStmtAttr - Set statement attributes.
**              SQLSetStmtOption - Set statement attributes (ODBC 1.0 version).
** 
** History: 
**   14-jun-04 (loera01)
**     Created.
**   04-Oct-04 (hweho01)
**     Avoid compiler error on AIX platform, put include
**     files of sql.h and sqlext.h after st.h.
**   15-Jul-2005 (hanje04)
**     Include iiodbcfn.h and tracefn.h which are the new home for the 
**     ODBC CLI function pointer definitions.
**   19-June-2006 (Ralph Loen) 
**     In SQLSetStmtOption(), call the ODBC driver with the ODBC driver
**     handle, not the CLI statement handle.
**   26-Feb-1007 (Ralph Loen) SIR 117786
**     Added suppport for ODBC connection pooling. 
**   13-Mar-2007 (Ralph Loen) SIR 117786
**     Added IIodbc_createThread() (temporarily) and IIodbc_timeOutThread()
**     to support ODBC connection pool time outs.
**   05-Apr-2007 (Ralph Loen) SIR 117786
**     Added int_lnx pre-compiler definition to that non-Windows code works only
**     for Intel Linux platforms.  This is a temporary change. 
**   21-May-2007 (Ralph Loen) SIR 117786
**     Obsoleted IIodbc_freePoolHandle().  Instead, when a connection handle
**     is fetched from the pool, swap the current driver handle with the
**     driver handle from the pool.  Connection handles are freed from
**     SQLFreeConnect() or SQLFreeHandle, without finessing due to the
**     presence of ODBC connection pooling.
**   21-May-2007 (Ralph Loen) SIR 117786
**     Added new TH module in the CL to handle client threading.
**     IIodbc_createPoolThread() calls TH_createThread(), which has been
**     normalized for various thread API's in Unix, Linux, Windows and
**     VMS.
**   01-Aug-2007 (rapma01)
**     Added cast of pValue to ULONG* in SQLSetConnectAttr()
**     due to compiler errors on AIX
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**  04-Dec-2009 (Ralph Loen) Bug 123012
**      In SQLSetConnectAttr(), treat pValue as an integer, rather than a 
**      pointer to an integer, if the attribute is SQL_ATTR_AUTOCOMMIT.
*/ 

/*
** Name: 	SQLGetConnectAttr - Get connection attribute.
** 
** Description: 
**              SQLGetConnectAttr returns the current setting of a 
**              connection attribute.
**
** Inputs:
**              hdbc - Connection handle.
**              fOption - Requested connection attribute.
**              BufferLength - Maximum attribute buffer length.
**
**
** Outputs: 
**              pvParamParameter - Attribute value.
**              pStringLength - Actual attribute buffer length.
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

RETCODE SQL_API SQLGetConnectAttr(
    SQLHDBC     hdbc,
    SQLINTEGER  fOption,
    SQLPOINTER  pvParamParameter,
    SQLINTEGER  BufferLength,
    SQLINTEGER *pStringLength)
{
    RETCODE rc = SQL_SUCCESS, traceRet = 1;
    pDBC pdbc = (pDBC)hdbc;
    SQLUINTEGER *pvParam = (SQLUINTEGER *)pvParamParameter;
    SQLINTEGER   StringLength = sizeof(SQLUINTEGER); 

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLGetConnectAttr(hdbc,
         fOption, pvParamParameter, BufferLength, pStringLength), traceRet);

    if (!hdbc)
    {
        rc = SQL_INVALID_HANDLE;
        goto exit_routine;
    }
    
    if ( !pdbc->hdr.driverHandle )
    {
        if (fOption == SQL_ATTR_LOGIN_TIMEOUT)
        {
            *pvParam = 15;
            *pStringLength = StringLength;
            rc = SQL_SUCCESS;
            goto exit_routine;
        }
    }
    else if (fOption == SQL_ATTR_CONNECTION_DEAD)
    {
        if (pdbc->hdr.state < C4)
            *pvParam = SQL_CD_TRUE;
        else
            *pvParam = SQL_CD_FALSE;
        rc =  SQL_SUCCESS;
        goto exit_routine;
    }
    else
    {
        resetErrorBuff(pdbc, SQL_HANDLE_DBC);
        rc = IIGetConnectAttr(
            pdbc->hdr.driverHandle,
            fOption,
            pvParamParameter,
            BufferLength,
            pStringLength);
    }

    applyLock(SQL_HANDLE_DBC, pdbc);
    if (rc != SQL_SUCCESS)
    {
        pdbc->hdr.driverError = TRUE;
        pdbc->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_DBC, pdbc);

exit_routine:
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLGetConnectOption - Get connection attribute (ODBC 1.0)
** 
** Description: 
**              SQLGetConnectOption returns the current setting of the
**              connection attribute.
**
** Inputs:
**              hdbc - Connection handle.
**              fOption - Connection attribute.
**
**
** Outputs: 
**              pvParam - Setting of the connection attribute.
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

RETCODE SQL_API SQLGetConnectOption(
    SQLHDBC    hdbc,
    UWORD      fOption,
    SQLPOINTER pvParam)
{
    RETCODE rc, traceRet = 1;
    pDBC pdbc = (pDBC)hdbc;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLGetConnectOption(hdbc, fOption,
       pvParam), traceRet);

    if (!pdbc)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(hdbc, SQL_HANDLE_DBC);

    rc = IIGetConnectOption(
        pdbc->hdr.driverHandle,
        fOption,
        pvParam);

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
** Name: 	SQLGetEnvAttr - Get environment attribute.
** 
** Description: 
**              SQLGetEnvAttr returns the current setting of an
**              environment attribute.
**
** Inputs:
**              EnvironmentHandle - Environment handle.
**              Attribute - Requested attribute.
**              BufferLength - Maximum length of the attribute.
**
** Outputs: 
**              ValuePtr - Value of the attribute.
**              pStringLength - Actual length of the attribute.
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
**   26-Feb-1007 (Ralph Loen) SIR 117786
**      Return settings of SQL_ATTR_CONNECTION_POOLING and
**      SQL_ATTR_CP_MATCH.
*/ 

SQLRETURN  SQL_API SQLGetEnvAttr (
    SQLHENV     EnvironmentHandle,
    SQLINTEGER  Attribute,
    SQLPOINTER  ValuePtr,
    SQLINTEGER  BufferLength,
    SQLINTEGER *pStringLength)
{
    RETCODE rc = SQL_SUCCESS, traceRet = 1;
    i4 *pi4 = (i4 *)ValuePtr;
    pENV penv = (pENV)EnvironmentHandle;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLGetEnvAttr(EnvironmentHandle,
         Attribute, ValuePtr, BufferLength, pStringLength), traceRet);

    if (!EnvironmentHandle)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        rc = SQL_INVALID_HANDLE;
        goto routine_exit;
    }
    if ( !IIGetEnvAttr )
    {
        /*
        ** Without a loaded driver, we can only report on the attributes
        ** supported in the driver manager.  Pooling attributes are only
        ** reported in the driver manager anyway.
        */
        if (Attribute == SQL_ATTR_ODBC_VERSION)
        {
            if (penv->envAttrValue)
                *pi4 = (i4)(SCALARP)penv->envAttrValue;
            else
                *pi4 = (i4)SQL_OV_ODBC2;
        }
        else if (Attribute == SQL_ATTR_CONNECTION_POOLING)
        {
            if (penv->pooling == DRIVER_POOL)
                *pi4 = (i4)SQL_CP_ONE_PER_DRIVER;
            else if (penv->pooling == HENV_POOL)
                *pi4 = (i4)SQL_CP_ONE_PER_HENV;
            else
                *pi4 = (i4)SQL_CP_OFF;
        }
        else if (Attribute == SQL_ATTR_CP_MATCH)
        {
            if (penv->relaxed_match)
                *pi4 = (i4)SQL_CP_RELAXED_MATCH;
            else
                *pi4 = (i4)SQL_CP_STRICT_MATCH;
        }
        goto routine_exit;
    } /* if ( !IIGetEnvAttr ) */
    
    if (Attribute == SQL_ATTR_CONNECTION_POOLING)
    {
        rc = SQL_SUCCESS;
        if (penv->pooling == DRIVER_POOL)
            *pi4 = (i4)SQL_CP_ONE_PER_DRIVER;
        else if (penv->pooling == HENV_POOL)
            *pi4 = (i4)SQL_CP_ONE_PER_HENV;
        else
            *pi4 = (i4)SQL_CP_OFF;
        goto routine_exit;
    }
    else if (Attribute == SQL_ATTR_CP_MATCH)
    {
        if (penv->relaxed_match)
            *pi4 = (i4)SQL_CP_RELAXED_MATCH;
        else
            *pi4 = (i4)SQL_CP_STRICT_MATCH;
        goto routine_exit;
    }
    else if (Attribute == SQL_ATTR_OUTPUT_NTS)
    {
        *pi4 = (i4)SQL_TRUE;
        goto routine_exit;
    }

    resetErrorBuff(penv, SQL_HANDLE_ENV);

    rc = IIGetEnvAttr (
        penv->hdr.driverHandle,
        Attribute,
        ValuePtr,
        BufferLength,
        pStringLength);

    if (rc != SQL_SUCCESS)
    {
        applyLock(SQL_HANDLE_ENV, penv);
        penv->hdr.driverError = TRUE;
        penv->errHdr.rc = rc;
        releaseLock(SQL_HANDLE_ENV, penv);
    }
 
    /*
    ** Assume ODBC version 2 if the driver returns an unspecified response.
    */
    if (Attribute == SQL_ATTR_ODBC_VERSION && 
        (*(i4 *)ValuePtr !=  SQL_OV_ODBC2 && *(i4 *)ValuePtr != SQL_OV_ODBC3))
        *pi4 = (i4)SQL_OV_ODBC2;

routine_exit:
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLGetStmtAttr - Get statement attribute.
** 
** Description: 
**              SQLGetStmtAttr returns the current setting of a
**              statement attribute.
**
** Inputs:
**              StatementHandle - Statement handle.
**              Attribute - Requested statement attribute.
**              BufferLength - Maximum length of the attribute.
**
** Outputs: 
**              ValuePtr - Value of the attribute.
**              pStringLength - Actual length of the attribute.
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

SQLRETURN  SQL_API SQLGetStmtAttr (
    SQLHSTMT     StatementHandle,
    SQLINTEGER   Attribute,
    SQLPOINTER   ValuePtr,
    SQLINTEGER   BufferLength,
    SQLINTEGER  *pStringLength)
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)StatementHandle;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLGetStmtAttr(StatementHandle,
        Attribute, ValuePtr, BufferLength, pStringLength), traceRet);

    if (pstmt->hdr.driverHandle)
    {
        resetErrorBuff(pstmt, SQL_HANDLE_STMT);
        rc = IIGetStmtAttr (
            pstmt->hdr.driverHandle,
            Attribute,
            ValuePtr,
            BufferLength,
            pStringLength);

        applyLock(SQL_HANDLE_STMT, pstmt);
        if (rc != SQL_SUCCESS)
        {
            pstmt->hdr.driverError = TRUE;
            pstmt->errHdr.rc = rc;
        }
        releaseLock(SQL_HANDLE_STMT, pstmt);
    }
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLGetStmtOption - Get statement attribute (ODBC 1.0).
** 
** Description: 
**              SQLGetStmtOption returns the current setting of a
**              statement attribute.
**
** Inputs:
**              hstmt - Statement handle.
**              fOption - Requested statement attribute.
**
** Outputs: 
**              pvParamParameter - Value of the attribute.
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

RETCODE SQL_API SQLGetStmtOption(
    SQLHSTMT     hstmt,
    UWORD        fOption,
    SQLPOINTER   pvParamParameter)
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;
    
    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLGetStmtOption(hstmt, fOption,
        pvParamParameter), traceRet);

    if (pstmt->hdr.driverHandle)
    {
        resetErrorBuff(pstmt, SQL_HANDLE_STMT);

        rc = IIGetStmtOption(
            pstmt->hdr.driverHandle,
            fOption,
            pvParamParameter);

        applyLock(SQL_HANDLE_STMT, pstmt);
        if (rc != SQL_SUCCESS)
        {
            pstmt->hdr.driverError = TRUE;
            pstmt->errHdr.rc = rc;
        }
        releaseLock(SQL_HANDLE_STMT, pstmt);
    }
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLSetConnectAttr - Set connection attribute.
** 
** Description: 
**              SQLSetConnectAttr sets attributes that govern aspects of 
**              connections.
**
** Inputs:
**              hdbc - Connection handle.
**              fOption - Requested connection attribute.
**              pValue - Attribute value.
**              StringLength - Attribute buffer length.
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
**  14-jun-04 (loera01)
**      Created.
**  04-Dec-2009 (Ralph Loen) Bug 123012
**      Treat fOption as an integer, rather than a pointer to an integer,
**      if the attribute is SQL_ATTR_AUTOCOMMIT.    
*/ 

RETCODE SQL_API SQLSetConnectAttr(
    SQLHDBC     hdbc,
    SQLINTEGER  fOption,
    SQLPOINTER  pValue,
    SQLINTEGER  StringLength)
{
    RETCODE rc, traceRet = 1;
    pDBC pdbc = (pDBC)hdbc;
    IISETCONNECTATTR_PARAM *iiSetConnectAttrParam;
    
    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLSetConnectAttr(hdbc, fOption,
       pValue, StringLength), traceRet);
    
    if (!hdbc)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    /*
    **  If the driver isn't loaded yet, set a placeholder to do this 
    **  later.
    */
    if (!IISetConnectAttr)
    {
        iiSetConnectAttrParam = 
            (IISETCONNECTATTR_PARAM *)MEreqmem(0, 
            sizeof(IISETCONNECTATTR_PARAM), TRUE, NULL);
        QUinsert((QUEUE *)iiSetConnectAttrParam, &pdbc->setConnectAttr_q);
        iiSetConnectAttrParam->ConnectionHandle = hdbc;
        iiSetConnectAttrParam->Attribute = fOption;
        iiSetConnectAttrParam->Value = pValue;
        iiSetConnectAttrParam->StringLength = StringLength;
        pdbc->setConnectAttr_count++;
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_SUCCESS));
        return SQL_SUCCESS;
    }
    else
    {
        resetErrorBuff( pdbc, SQL_HANDLE_DBC );
        rc = IISetConnectAttr(
            pdbc->hdr.driverHandle,
            fOption,
            pValue,
            StringLength);

        applyLock(SQL_HANDLE_DBC, pdbc);
        if (rc != SQL_SUCCESS)
        {
            pdbc->errHdr.rc = rc;
            pdbc->hdr.driverError = TRUE;
        }
        else if (fOption == SQL_ATTR_AUTOCOMMIT && StringLength == SQL_IS_INTEGER)
        {
            if (((ULONG)pValue) == SQL_AUTOCOMMIT_OFF)
                pdbc->autocommit = FALSE;
            else if (((ULONG)pValue) == SQL_AUTOCOMMIT_ON)
                pdbc->autocommit = TRUE;
        }
        releaseLock(SQL_HANDLE_DBC, pdbc);
    }
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLSetConnectOption - Set connection attribute (ODBC 1.0)
** 
** Description: 
**              SQLSetConnectOption sets attributes that govern aspects of 
**              connections.
** Inputs:
**              hdbc - Connection handle.
**              fOption - Connection attribute.
**              vParam - Setting of the connection attribute.
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


RETCODE SQL_API SQLSetConnectOption(
    SQLHDBC    hdbc,
    UWORD      fOption,
    SQLUINTEGER vParam)
{
    RETCODE rc, traceRet = 1;
    pDBC pdbc = (pDBC)hdbc;
    IISETCONNECTOPTION_PARAM *iiSetConnectOptionParam;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLSetConnectOption(hdbc,
        fOption, vParam), traceRet);

    if (!hdbc)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }
    /*
    **  If the driver isn't loaded yet, set a placeholder to do this 
    **  later.
    */
    if ( !IISetConnectOption )
    {
        iiSetConnectOptionParam = 
            (IISETCONNECTOPTION_PARAM *)MEreqmem(0, 
            sizeof(IISETCONNECTOPTION_PARAM), TRUE, NULL);
        QUinsert((QUEUE *)iiSetConnectOptionParam, &pdbc->setConnectOption_q);
        iiSetConnectOptionParam->ConnectionHandle = hdbc;
        iiSetConnectOptionParam->Option = fOption;
        iiSetConnectOptionParam->Value = vParam;
        pdbc->setConnectOption_count++;
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_SUCCESS));
        return SQL_SUCCESS;
    }
    else
    {
        resetErrorBuff(pdbc, SQL_HANDLE_DBC);

        rc = IISetConnectOption(
            pdbc->hdr.driverHandle,
            fOption,
            vParam);

        applyLock(SQL_HANDLE_DBC, pdbc);
        if (rc != SQL_SUCCESS)
        {
            pdbc->hdr.driverError = TRUE;
            pdbc->errHdr.rc = rc;
        }
        else if (fOption == SQL_ATTR_AUTOCOMMIT)
            pdbc->autocommit = TRUE;
        releaseLock(SQL_HANDLE_DBC, pdbc);
    }

    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLSetEnvAttr - Get environment attribute.
** 
** Description: 
**              SQLSetStmtAttr returns the current setting of an
**              environment attribute.
**
** Inputs:
**              EnvironmentHandle - Environment handle.
**              Attribute - Requested statement attribute.
**              ValuePtr - Value of the attribute.
**              StringLength - Length of the attribute.
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
**   26-Feb-1007 (Ralph Loen) SIR 117786
**      If connection pooling is specified: (1) initialize the IIodbc_cb
**      control block if required; (2) Initialize the connection pool;
**      (3) Mark relaxed criteria if requested.
*/ 

SQLRETURN  SQL_API SQLSetEnvAttr (
    SQLHENV    EnvironmentHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER StringLength)
{
    RETCODE rc, traceRet = 1;
    pENV penv = (pENV)EnvironmentHandle;
    IISETENVATTR_PARAM *iiSetEnvAttrParam;
    i4 value = (i4)(SCALARP)ValuePtr;

    if (block_init)
        ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLSetEnvAttr(EnvironmentHandle,
            Attribute, ValuePtr, StringLength), traceRet);

    /*
    ** Connection pooling is set in the CLI before any drivers are loaded.
    */
    if (!EnvironmentHandle)
    {
        if (Attribute != SQL_ATTR_CONNECTION_POOLING)
        {
            rc = SQL_INVALID_HANDLE;
            goto exit_routine;
        }
        if (!block_init)
        {
            /*
            ** Initialize the CLI control block if connection pooling is 
            ** specified.
            */
            if (value == SQL_CP_ONE_PER_DRIVER || value == SQL_CP_ONE_PER_HENV)
            {
                IIodbc_initControlBlock();
                if (IIodbc_cb.timeout != -1)
                    IIodbc_createPoolThread();
                block_init = TRUE;
            }
        }
        
        if (value == SQL_CP_ONE_PER_DRIVER)
        {
            ODBC_EXEC_TRACE(ODBC_TR_TRACE)
                ("SQLSetEnvAttr: driver pool initialized\n");
            IIodbc_cb.pooling = DRIVER_POOL;
            QUinit(&IIodbc_cb.pool_q);
        }
        else if (value == SQL_CP_ONE_PER_HENV)
        {
            IIodbc_cb.pooling = HENV_POOL;
        }
        else if (value != SQL_CP_OFF)
        {
            /* 
            ** If the pooling arguments are invalid, only SQL_INVALID_HANDLE 
            ** can be returned because no environment handle exists.
            */
            rc = SQL_INVALID_HANDLE;
            goto exit_routine;
        } /* if (!initPool) */
        rc = SQL_SUCCESS;
        goto exit_routine;
    } /* if (!EnvironmentHandle) */
    
    
    if (Attribute == SQL_ATTR_CP_MATCH && value == SQL_CP_RELAXED_MATCH)
        penv->relaxed_match = TRUE;
    else
        penv->relaxed_match = FALSE;

    /*
    **  If the driver isn't loaded yet, set a placeholder to do this 
    **  later.
    */
    if ( !IISetEnvAttr )
    {
        if (Attribute == SQL_ATTR_ODBC_VERSION)
            penv->envAttrValue = ValuePtr;
        iiSetEnvAttrParam = 
        (IISETENVATTR_PARAM *)MEreqmem(0, sizeof(IISETENVATTR_PARAM), 
            TRUE, NULL);
        penv->setEnvAttr_count++;
        iiSetEnvAttrParam->Attribute = Attribute;
        iiSetEnvAttrParam->EnvironmentHandle = EnvironmentHandle;
        iiSetEnvAttrParam->ValuePtr = ValuePtr;
        iiSetEnvAttrParam->StringLength = StringLength;
        QUinsert((QUEUE *)iiSetEnvAttrParam, &penv->setEnvAttr_q);
        rc = SQL_SUCCESS;
        goto exit_routine;
    }

    if (penv->hdr.state < E2)
    {
        resetErrorBuff(penv, SQL_HANDLE_ENV);

        rc = IISetEnvAttr (
            penv->hdr.driverHandle,
            Attribute,
            ValuePtr,
            StringLength);
    }
    else
        rc = SQL_ERROR;

    applyLock(SQL_HANDLE_ENV, penv);
    if (rc != SQL_SUCCESS)
    {
        penv->hdr.driverError = TRUE;
        penv->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_ENV, penv);
    
exit_routine:
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}
/*
** Name: 	SQLSetStmtAttr - Set statement attribute.
** 
** Description: 
**              SQLSetStmtAttr sets attributes related to a statement.
**
** Inputs:
**              hstmt - Statement handle.
**              fOption - Requested statement attribute.
**              ValuePtr - Value of the attribute.
**              StringLength - Length of the attribute.
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

RETCODE SQL_API SQLSetStmtAttr(
    SQLHSTMT   hstmt,
    SQLINTEGER fOption,
    SQLPOINTER ValuePtr,
    SQLINTEGER StringLength)
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;
    
    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLSetStmtAttr(hstmt, fOption,
        ValuePtr, StringLength), traceRet);

    if (validHandle(pstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }
    else
        resetErrorBuff(hstmt, SQL_HANDLE_STMT);

    switch (fOption)
    {
    case SQL_ATTR_USE_BOOKMARKS: 
    case SQL_ATTR_CONCURRENCY: 
    case SQL_ATTR_CURSOR_TYPE:
    case SQL_ATTR_SIMULATE_CURSOR: 
        if (pstmt->hdr.state >= S3)
        {
            rc = SQL_ERROR;
            {
                ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
                return rc;
            }
        }
        break;
    }
     
    rc = IISetStmtAttr(
        pstmt->hdr.driverHandle,
        fOption,
        ValuePtr,
        StringLength);

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
** Name: 	SQLSetStmtOption - Set statement attribute (ODBC 1.0).
** 
** Description: 
**              SQLSetStmtOption sets a connection attribute.
**
** Inputs:
**              hstmt - Statement handle.
**              fOption - Requested statement attribute.
**              vParam - Value of the attribute.
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


RETCODE SQL_API SQLSetStmtOption(
    SQLHSTMT hstmt,
    SQLUSMALLINT fOption,
    SQLUINTEGER  vParam)
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)hstmt;
    
    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLSetStmtOption( hstmt, fOption,
        vParam), traceRet);

    if (validHandle(pstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }
    else
    {
        resetErrorBuff(hstmt, SQL_HANDLE_STMT);

        rc = IISetStmtOption(
            pstmt->hdr.driverHandle,
            fOption,
            vParam);

        applyLock(SQL_HANDLE_STMT, pstmt);
        if (rc != SQL_SUCCESS)
        {
            pstmt->hdr.driverError = TRUE;
            pstmt->errHdr.rc = rc;
        }
        releaseLock(SQL_HANDLE_STMT, pstmt);
    }
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}
/*
** Name: 	IIodbc_createPoolThread - Create a pooling thread.
** 
** Description: 
**              This routine is a temporary submission.  IIodbc_createThread()
**              will be replaced by the new compatlib routine 
**              THcreate_clientThread() that will reside in the new "TH" 
**              sub-facility.
**
** Inputs:
**              None.
**
** Outputs: 
**              None.
**
** Returns: 
**              TRUE if successful; FALSE otherwise 
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
**    13-Mar-07 (Ralph Loen) SIR 117786
**      Created.
*/ 

BOOL IIodbc_createPoolThread()
{
    TH_THREAD_ID threadID ZERO_FILL;
    STATUS status=OK;

    TH_create_thread( 32768,               /* stack size */
        NULL,                              /* stack addr */
        IIodbc_timeOutThread,              /* init routine */
        (void *)NULL,                      /* parm */
        &threadID,                         /* thread id */
        TH_DETACHED,                       /* release resource on xit */
        &status );
    if (status != OK)
    {
        SIprintf("NO POOLING THREAD CREATED\n");
        return FALSE;
    }
    return TRUE;
}

/*
** Name: 	IIodbc_timeOutThread - ODBC pool time-out threadr
**              
** 
** Description: 
**              IIodbc_timeOutThread() searches the ODBC connection pool
**              every thirty seconds and checks for expired connections.
**              If a connection has expired, the connection will be
**              aborted and the connection handle will be removed from the
**              ODBC connection pool.
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
**              Connection handles may be removed from the pool, and
**              the connection will be aborted.
**
** History: 
**    13-Mar-07 (Ralph Loen) SIR 117786
**      Created.
*/ 


void IIodbc_timeOutThread()
{
    QUEUE *q, *p, *pq;
    pENV penv;
    pDBC pdbc;
    RETCODE rc;
    SYSTIME expire_time;
    TM_STAMP stamp;
    char stampStr[TM_SIZE_STAMP];

    ODBC_EXEC_TRACE(ODBC_EX_TRACE)
        ("IIodbc_timeOutThread() with timeout %d\n", IIodbc_cb.timeout);
    while(TRUE)
    {
        /*
        ** Search the pool every thirty seconds and check for expired
        ** connections.  Force disconnect and free from the pool
        ** if the connection handle has passed the expiration time.  
        ** Note that the CLI and driver connection handles are not
        ** freed.  
        */
        PCsleep(30000);
        TMget_stamp(&stamp);
        TMstamp_str(&stamp, stampStr);
        ODBC_EXEC_TRACE(ODBC_EX_TRACE)
            ("IIodbc_timeOutThread woke up at %s\n", stampStr);
        TMnow(&expire_time);
        
        if (IIodbc_cb.pooling == DRIVER_POOL)
        {
            applyLock(SQL_HANDLE_IIODBC_CB, NULL);
            for (q = IIodbc_cb.pool_q.q_prev; q!= &IIodbc_cb.pool_q;
                q = p)
            {
                ODBC_EXEC_TRACE(ODBC_EX_TRACE)
                    ("IIodbc_timeOutThread: driver pool count is %d\n", 
                    IIodbc_cb.pool_count);
                p = q->q_prev;
                pdbc = (pDBC)((pPOOL)q)->pdbc;
                ODBC_EXEC_TRACE(ODBC_EX_TRACE)
                    ("IIodbc_timeOutThread: found conn handle %p with time diff %d\n",pdbc,
                    expire_time.TM_secs - pdbc->expire_time.TM_secs);
                if (expire_time.TM_secs - pdbc->expire_time.TM_secs > 
                    IIodbc_cb.timeout)
                {
                    /*
                    **  Note that the connection handle is not freed, only
                    **  removed from the pool. 
                    */
                    ODBC_EXEC_TRACE(ODBC_EX_TRACE)
                        ("IIodbc_timeOutThread: EXPIRED.  pdbc time is %d vs %d\n",
                        pdbc->expire_time.TM_secs, expire_time.TM_secs);
                    rc = IIDisconnect(pdbc->hdr.driverHandle);
                    QUremove(q);
                    MEfree((PTR)q);
                    IIodbc_cb.pool_count -= 1;
                    ODBC_EXEC_TRACE(ODBC_EX_TRACE)
                        ("new pool count is %d\n",IIodbc_cb.pool_count);
                    applyLock(SQL_HANDLE_DBC, pdbc);
                    pdbc->hdr.state = C2;
                    releaseLock(SQL_HANDLE_DBC, pdbc);
                }
            }
            releaseLock(SQL_HANDLE_IIODBC_CB, NULL);
        }
        else
        {
            for (q = IIodbc_cb.env_q.q_prev; q!= &IIodbc_cb.env_q;
                q = q->q_prev)
            {
                penv = (pENV)q;
                TRdisplay("Found env handle %p\n",penv);
                applyLock(SQL_HANDLE_ENV, penv);
                for (pq = penv->pool_q.q_prev; pq != &penv->pool_q;
                    pq = p)
                {
                    ODBC_EXEC_TRACE(ODBC_EX_TRACE)
                        ("Env pool count is %d\n",penv->pool_count);
                    p = q->q_prev;
                    pdbc = (pDBC)((pPOOL)pq)->pdbc;
                    ODBC_EXEC_TRACE(ODBC_EX_TRACE)
                        ("IIodbc_TimeOutThread: Found conn handle %p with time diff %d\n",pdbc,
                        expire_time.TM_secs - pdbc->expire_time.TM_secs);
                    if (expire_time.TM_secs - pdbc->expire_time.TM_secs
                        > 1)
                    {
                        ODBC_EXEC_TRACE(ODBC_EX_TRACE)
                            ("IIodbc_timeOutThread: EXPIRED.  pdbc time is %d vs %d\n",
                            pdbc->expire_time.TM_secs,   
                            expire_time.TM_secs);
                        rc = IIDisconnect(pdbc->hdr.driverHandle);
                        QUremove(q);
                        MEfree((PTR)q);
                        penv->pool_count -= 1;
                        ODBC_EXEC_TRACE(ODBC_EX_TRACE)
                            ("IIodbc_timeOutThread: new pool count is %d\n",
                            penv->pool_count);
                    }
                }
                releaseLock(SQL_HANDLE_ENV, penv);
            } /* for (q = IIodbc_cb.env_q.q_prev; q!= &IIodbc_cb.env_q;
                q = q->q_prev) */
        } /* if (IIodbc_cb.pooling == DRIVER_POOL) */
    } /* while (TRUE) */
}
