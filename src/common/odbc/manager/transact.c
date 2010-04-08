/*
** Copyright (c) 2004, 2007 Ingres Corporation 
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
** Name: transact.c - transaction functions for ODBC CLI.
** 
** Description: 
** 		This file defines: 
** 
**              SQLEndTran - End transacton.
**              SQLTransact - Perform transaction.
** 
** History: 
**   14-jun-04 (loera01)
**      Created. 
**   15-Jul-2005 (hanje04)
**	Include iiodbcfn.h and tracefn.h which are the new home for the 
**	ODBC CLI function pointer definitions.
**   26-Feb-1007 (Ralph Loen) 
**      Cleaned up some inconsistencies in locking.
**   03-Dec-2009 (Ralph Loen) Bug 123003
**      In SQLTransact(), remove code that returned an invalid handle error 
**      if both henv and hdbc were set.  This is perfectly valid. 
** 
*/ 

/*
** Name: 	SQLEndTran - End a transaction
** 
** Description: 
**              SQLEndTran requests a commit or rollback operation for all 
**              active operations on all statements associated with a 
**              connection. SQLEndTran can also request that a commit or 
**              rollback operation be performed for all connections 
**              associated with an environment.
**
** Inputs:
**              HandleType - Environment or connection handle type.
**              Handle - Environment or connection handle.
**              CompletionType - Commit or rollback.
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
**    04-Oct-04 (hweho01)
**      Avoid compiler error on AIX platform, put include
**      files of sql.h and sqlext.h after qu.h.
*/ 

SQLRETURN  SQL_API SQLEndTran (
    SQLSMALLINT HandleType,
    SQLHANDLE   Handle,
    SQLSMALLINT CompletionType)
{
    RETCODE rc, traceRet = 1;
    pENV penv;
    pDBC pdbc;
    pSTMT pstmt;
    IIODBC_HDR *hdr;
    IIODBC_ERROR_HDR *errHdr;
    QUEUE *sq, *cq;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLEndTran(HandleType, Handle,
        CompletionType), traceRet);

    switch (HandleType)
    {
    case SQL_HANDLE_ENV:
        penv = (pENV)Handle;
        hdr = &penv->hdr;
        errHdr = &penv->errHdr;

        if (validHandle(penv, SQL_HANDLE_ENV) != SQL_SUCCESS)
        {
            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, 
                SQL_INVALID_HANDLE));
            return SQL_INVALID_HANDLE;
        }

        resetErrorBuff(penv, SQL_HANDLE_ENV);

        rc =  IIEndTran(
            SQL_HANDLE_ENV,
            penv->hdr.driverHandle,
            CompletionType);
    
        if (rc != SQL_SUCCESS)
        {
            applyLock(SQL_HANDLE_ENV, penv); 
            hdr->driverError = TRUE;
            errHdr->rc = rc;
            releaseLock(SQL_HANDLE_ENV, penv);
        }
        else if (SQL_SUCCEEDED(rc))
        {
            for (cq = penv->dbc_q.q_prev;
                cq != &penv->dbc_q; cq = cq->q_prev)
            {
                pdbc = (pDBC)cq;
                applyLock(SQL_HANDLE_DBC, pdbc);
                if (pdbc->stmt_count > 0)
                    pdbc->hdr.state = C5;
                else
                    pdbc->hdr.state = C4;
                releaseLock(SQL_HANDLE_DBC, pdbc);
                for (sq = pdbc->stmt_q.q_prev;
                    sq != &pdbc->stmt_q; sq = sq->q_prev)
                {
                    pstmt = (pSTMT)sq;
                    applyLock(SQL_HANDLE_STMT, pstmt);
                    if (!pdbc->autocommit)
                        pstmt->hdr.state = S1;

                    if (!pstmt->prepared)
                    {
                        if (pstmt->select || pstmt->execproc)
                            pstmt->hdr.state = S3;
                        else
                           pstmt->hdr.state = S2;
                    }
                    else
                        pstmt->hdr.state = S4;
                    releaseLock(SQL_HANDLE_STMT, pstmt);
                }
            }
        }
        break;

    case SQL_HANDLE_DBC:
        pdbc = (pDBC)Handle;
        hdr = &pdbc->hdr;
        errHdr = &pdbc->errHdr;

        if (validHandle(pdbc, SQL_HANDLE_DBC) != SQL_SUCCESS)
        {
            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, 
                SQL_INVALID_HANDLE));
            return SQL_INVALID_HANDLE;
        }

        resetErrorBuff(pdbc, SQL_HANDLE_DBC);

        rc =  IIEndTran(
            HandleType,
            pdbc->hdr.driverHandle,
            CompletionType);
    
        if (rc != SQL_SUCCESS)
        {
            applyLock(SQL_HANDLE_DBC, pdbc);
            hdr->driverError = TRUE;
            errHdr->rc = rc;
            releaseLock(SQL_HANDLE_DBC, pdbc);
        }
        else if (SQL_SUCCEEDED(rc))
        {
            applyLock(SQL_HANDLE_DBC, pdbc);
            if (pdbc->stmt_count > 0)
                pdbc->hdr.state = C5;
            else
                pdbc->hdr.state = C4;
            releaseLock(SQL_HANDLE_DBC, pdbc);
            
            for (cq = pdbc->stmt_q.q_prev;
                cq != &pdbc->stmt_q; cq = cq->q_prev)
            {
                pstmt = (pSTMT)cq;
                applyLock(SQL_HANDLE_STMT, pstmt);
                if (!pdbc->autocommit)
                    pstmt->hdr.state = S1;
                if (!pstmt->prepared)
                {
                    if (pstmt->select || pstmt->execproc)
                        pstmt->hdr.state = S3;
                    else
                       pstmt->hdr.state = S2;
                }
                else
                    pstmt->hdr.state = S4;
	           releaseLock(SQL_HANDLE_STMT, pstmt); 
            }
        }
        releaseLock(SQL_HANDLE_DBC, pdbc);
        break;

    default:
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
        break;
    }
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLTransact - End a transaction (ODBC 1.0 version) 
** 
** Description: 
**              SQLTransact issues a rollback or commit query.
**
** Inputs:
**              henv - Environment handle or NULL.
**              hdbc - Connection handle or NULL.
**              fType - Completion type: rollback or commit.
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
**   14-jun-04 (loera01)
**      Created.
**   03-Dec-2009 (Ralph Loen) Bug 123003
**      Remove code that returned an invalid handle error if both
**      henv and hdbc were set.  This is perfectly valid. 
*/ 

RETCODE SQL_API SQLTransact(
    SQLHENV henv,
    SQLHDBC hdbc,
    UWORD fType)
{
    RETCODE rc, traceRet = 1;
    pENV penv = NULL;
    pDBC pdbc = NULL;
	pSTMT pstmt = NULL;
    QUEUE *cq, *sq;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLTransact(henv, hdbc, fType),
       traceRet);

    if (!henv && !hdbc)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, 
            SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    if (henv)
    {
        if (validHandle(henv, SQL_HANDLE_ENV) != SQL_SUCCESS)
        {
            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, 
                SQL_INVALID_HANDLE));
            return SQL_INVALID_HANDLE;
        }
        else
        {
            resetErrorBuff(henv, SQL_HANDLE_ENV);
            penv = (pENV)henv;
            rc =  IITransact(
                penv->hdr.driverHandle,
                NULL,
                fType);

            applyLock(SQL_HANDLE_ENV, penv);
            if (rc != SQL_SUCCESS)
            {
                 penv->hdr.driverError = TRUE;
                 penv->errHdr.rc = rc;
            }
            else if (SQL_SUCCEEDED(rc))
            {
                for (cq = penv->dbc_q.q_prev;
                    cq != &penv->dbc_q; cq = cq->q_prev)
                {
                    pdbc = (pDBC)cq;
                    applyLock(SQL_HANDLE_DBC, pdbc);
                    if (pdbc->stmt_count > 1)
                        pdbc->hdr.state = C5;
                    else
                        pdbc->hdr.state = C4;
                    releaseLock(SQL_HANDLE_DBC, pdbc);
                    for (sq = pdbc->stmt_q.q_prev;
                        sq != &pdbc->stmt_q; sq = sq->q_prev)
                    {
                        pstmt = (pSTMT)sq;
                        applyLock(SQL_HANDLE_STMT, pstmt);
                        if (!pdbc->autocommit)
                            pstmt->hdr.state = S1;
    
                        if (!pstmt->prepared)
                        {
                            if (pstmt->select || pstmt->execproc)
                                pstmt->hdr.state = S3;
                            else
                               pstmt->hdr.state = S2;
                        }
                        else
                            pstmt->hdr.state = S4;
                        releaseLock(SQL_HANDLE_STMT, pstmt);
                    } /* for (sq = pdbc->stmt_q.q_prev;
                        sq != &pdbc->stmt_q; sq = sq->q_prev) */
                } /* for (cq = penv->dbc_q.q_prev;
                    cq != &penv->dbc_q; cq = cq->q_prev) */
            } /* if (SQL_SUCCEEDED(rc) */
        } /* if (validHandle(henv, SQL_HANDLE_ENV) != SQL_SUCCESS) */
        releaseLock(SQL_HANDLE_ENV, penv);
    }  /* if (henv) */
    
    if (hdbc)
    {
        pdbc = (pDBC)hdbc;
        if (validHandle(hdbc, SQL_HANDLE_DBC) != SQL_SUCCESS)
        {
            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, 
                SQL_INVALID_HANDLE));
            return SQL_INVALID_HANDLE;
        }
        else
        {
            resetErrorBuff(hdbc, SQL_HANDLE_DBC);
            rc =  IITransact(
                NULL,
                pdbc->hdr.driverHandle,
                fType);

            applyLock(SQL_HANDLE_DBC, pdbc);
            if (rc != SQL_SUCCESS)
            {
                 pdbc->hdr.driverError = TRUE;
                 pdbc->errHdr.rc = rc;
            }
            else if (SQL_SUCCEEDED(rc))
            { 
                if (pdbc->stmt_count > 1)
                    pdbc->hdr.state = C5;
                else
                    pdbc->hdr.state = C5;
            }
            releaseLock(SQL_HANDLE_DBC, pdbc);
        }
    } /* if (hdbc) */
    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}
