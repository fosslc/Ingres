/*
**  (C) Copyright 2000, 2004 Ingres Corporation
*/

#include <compat.h>
#ifndef VMS
#include <systypes.h>
#endif
#include <sql.h>                    /* ODBC Core definitions */
#include <sqlext.h>                 /* ODBC extensions */
#include <sqlstate.h>               /* ODBC driver sqlstate codes */
#include <sqlcmd.h>
#include <cs.h>
#include <cv.h>
#include <me.h>
#include <st.h> 
#include <er.h>
#include <erodbc.h>

#include <sqlca.h>
#include <idmsxsq.h>                /* control block typedefs */
#include <idmsutil.h>               /* utility stuff */
#include "caidoopt.h"              /* connect options */
#include "idmsoinf.h"               /* information definitions */
#include "idmsoids.h"               /* string ids */
#include <idmseini.h>               /* ini file keys */
#include "idmsodbc.h"               /* ODBC driver definitions             */

/*
**  Module Name: LOCK.C
**
**  Description: Locking routines
**
**  Change History
**  --------------
**
**  date        programmer          description
**
**  06/21/2000  Dave Thole          Split from dllmai32.c into own module
**      14-feb-2003 (loera01) SIR 109643
**          Minimize platform-dependency in code.  Make backward-compatible
**          with earlier Ingres versions.
**	23-apr-2004 (somsa01)
**	    Include systypes.h to avoid compile errors on HP.
**     15-nov-2004 (Ralph.Loen@ca.com) Bug 113462
**          Use IItrace global block to handle tracing.
**          Removed unsed code block that invokes LockEnv().  
*/

/*
** Definition of all global variables owned by this file.
*/
ODBC_CRITICAL_SECTION IIODcsEnv ZERO_FILL;  /* Use to single thread driver */
BOOL                  IIODcsEnvSetup=FALSE; /* need to init IIODcsEnv */
#ifdef NT_GENERIC
ODBC_CRITICAL_SECTION csLibqxa ZERO_FILL;   /* Use to single thread calls to
                                               Ingres XA routines for Windows
                                               since they are not thread=safe yet */
#endif

/*
** Definition of static variables and forward static functions.
*/
static CS_THREAD_ID  thread_id_zero  ZERO_FILL;


#if !defined(_WIN32)  ||  defined(DEBUG_ODBCCS)
#ifdef OS_THREADS_USED

/*
**  ODBCInitializeCriticalSection
**
**  Simply initialize the mutex.
**
**  On entry: sptr-->ODBC_CRITICAL_SECTION
**
**  Returns:  nothing
**
*/
void IIODcsin_ODBCInitializeCriticalSection(ODBC_CRITICAL_SECTION *sptr)
   {
    CS_synch_init(&sptr->mutex);
   }


/*
**  ODBCDeleteCriticalSection
**
**  Destroy the mutex.
**
**  On entry: sptr-->ODBC_CRITICAL_SECTION
**
**  Returns:  nothing
**
*/
void IIODcsde_ODBCDeleteCriticalSection(ODBC_CRITICAL_SECTION *sptr)
   {
    CS_synch_destroy(&sptr->mutex);
   }


/*
**  ODBCEnterCriticalSection
**
**  Enter single threaded critical section of code using mutexes.
**  If the the thread is the one already holding the lock, then it
**  is recursing and don't try to re-acquire the lock.
**
**  On entry: sptr-->ODBC_CRITICAL_SECTION
**
**  Returns:  nothing
**
*/
void IIODcsen_ODBCEnterCriticalSection(ODBC_CRITICAL_SECTION *sptr)
   {
#ifdef DCE_THREADS
    if (CS_synch_trylock(&sptr->mutex)==0)
#else
    if (CS_synch_trylock(&sptr->mutex))
#endif
       { /* lock blocked; check if we already hold lock */
         if (!CS_thread_id_equal(sptr->threadid, CS_get_thread_id()))
            { CS_synch_lock(&sptr->mutex);  /* wait on the lock */
              sptr->usagecount=0;
            }
       }
    /* we got the lock or we already had the lock in a recursive call */
    CS_thread_id_assign(sptr->threadid, CS_get_thread_id());
    sptr->usagecount++;
   }


/*
**  ODBCLeaveCriticalSection
**
**  Leave single threaded critical section of code
**
**  On entry: sptr-->ODBC_CRITICAL_SECTION
**
**  Returns:  nothing
**
*/
void IIODcslv_ODBCLeaveCriticalSection(ODBC_CRITICAL_SECTION *sptr)
   {
#if defined(_WIN32)  &&  defined(DEBUG_ODBCCS)
    if (sptr->usagecount <= 0)
        MessageBox(NULL, "Use count went negative", 
                 "ODBCLeaveCriticalSection Internal Error", MB_ICONSTOP|MB_OK);
#endif

    if (sptr->usagecount)   /* safety check to make sure usage count not < 0 */
       {sptr->usagecount--; /* decrement recusrsive call usage count */
        if (sptr->usagecount==0) /* if recusrsive call usage count down to 0 */
           {
            CS_thread_id_assign(sptr->threadid, thread_id_zero); /*clear id */
            CS_synch_unlock(&sptr->mutex);              /* unlock the mutex */
           }
       }
   }
#endif /* OS_THREADS_USED */
#endif /* !defined(_WIN32)  &&  !defined(DEBUG_ODBCCS) */


/*
**  ErrUnlockDbc
**
**  Set error and unlock connection.
**
**  On entry: LPDBC-->connection block
**
**  Returns:  return code from ErrState.
*/
RETCODE FASTCALL ErrUnlockDbc (UINT err, LPDBC pdbc)
{
    RETCODE rc;

    rc = ErrState (err, pdbc);
    UnlockDbc (pdbc);
    return rc;
}

/*
**  ErrUnlockDesc
**
**  Set error in DESC and unlock connection.
**
**  On entry: LPDESC-->descriptor block
**
**  Returns:  return code from ErrState.
*/
RETCODE FASTCALL ErrUnlockDesc (UINT err, LPDESC pdesc)
{
    RETCODE rc;

    rc = ErrState (err, pdesc);
    UnlockDesc (pdesc);
    return rc;
}

/*
**  ErrUnlockStmt
**
**  Set error and unlock statement.
**
**  On entry: LPSTMT-->statement block
**
**  Returns:  return code from ErrState.
*/
RETCODE FASTCALL ErrUnlockStmt (UINT err, LPSTMT pstmt)
{
    RETCODE rc;

    rc = ErrState (err, pstmt);
    UnlockStmt (pstmt);
    return rc;
}

/*
**  LockEnv
**
**  Lock entire environment for ODBC session.
**
**  On entry: LPENV-->environment block
**
**  Returns:  nothing
*/
void LockEnv (LPENV penv)
{
    if (IIODcsEnvSetup == FALSE)
       {ODBCInitializeCriticalSection (&IIODcsEnv);
        IIODcsEnvSetup = TRUE;
       }

    ODBCEnterCriticalSection (&IIODcsEnv);     

    return;
}

/*
**  LockDbc
**
**  Lock connection block.
**
**  On entry: LPDBC-->connection block
**
**  Returns:  TRUE if DBC found in ENV-->DBC
**            FALSE otherwise  
*/
BOOL LockDbc (LPDBC pdbc)
{
/*  LPDBC p;   */
    BOOL  rc = FALSE;
    
    if (!pdbc)
        return FALSE;

#ifdef _WIN32
    __try
#endif
    {
        if (memcmp(pdbc->szEyeCatcher, "DBC*", 4) != 0)  /* basic check */
            pdbc = NULL;
    }
#ifdef _WIN32
    __except (GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION)
    {
        pdbc = NULL;
    }
#endif

    if (!pdbc)       /* return "invalid handle" if bad DBC */
       return FALSE;

    rc = LockDbc_Trusted(pdbc);  /* lock the DBC */

    return rc;
}

/*
**  LockDbc_Trusted
**
**  Lock connection block while trusting the DBC pointer.
**  Called only by driver routines that have already 
**  validated the DBC pointer and do not want to risk
**  waiting on a ENV lock while already holding the DBC lock
**  leading to deadlock.
**
**  On entry: LPDBC-->connection block
**
**  Returns:  TRUE always  
*/
BOOL LockDbc_Trusted (LPDBC pdbc)
{
    ODBCEnterCriticalSection (&pdbc->csDbc);

    return TRUE;
}

/*
**  LockDesc
**
**  Lock connection block that owns descriptor.
**
**  On entry: LPDESC-->statement block
**
**  Returns:  TRUE if STMT found and lock DBC
**            FALSE otherwise  
*/
BOOL LockDesc (LPDESC pdesc)
{
    LPDBC  pdbc;
    
    /*
    **  Get alleged DBC owner:
    */
#ifdef _WIN32
    __try
#endif
    {
        pdbc = pdesc->pdbc;
    }
#ifdef _WIN32
    __except (GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION)
    {
        pdbc = NULL;
    }
#endif

    /*
    **  Lock the DBC of the descriptor:
    */
    if (LockDbc (pdbc))
    {
        if (STequal ("DESC*", pdesc->szEyeCatcher))
        {
            return TRUE;
        }
        UnlockDbc (pdbc);
    }

    return FALSE;
}

/*
**  LockStmt
**
**  Lock connection block that owns statement.
**
**  On entry: LPSTMT-->statement block
**
**  Returns:  TRUE if STMT found and lock DBC
**            FALSE otherwise  
*/
BOOL LockStmt (LPSTMT pstmt)
{
    LPDBC  pdbc;
    LPSTMT p;
    
    /*
    **  Get alleged DBC owner:
    */
#ifdef _WIN32
    __try
#endif
    {
        pdbc = pstmt->pdbcOwner;
    }
#ifdef _WIN32
    __except (GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION)
    {
        pdbc = NULL;
    }
#endif

    /*
    **  Lock the DBC, and make sure the STMT was
    **  not freed while we were waiting for the lock:
    */
    if (LockDbc (pdbc))
    {
        for (p = pdbc->pstmtFirst; p; p = p->pstmtNext)
        {
            if (p == pstmt)
            {
                return TRUE;
            }
        }
        UnlockDbc (pdbc);
    }

    return FALSE;
}

/*
**  UnlockDbc
**
**  Unock connection block.
**
**  On entry: LPDBC-->connection block
**
**  Returns:  nothing
*/
void FASTCALL UnlockDbc (LPDBC pdbc)
{
    ODBCLeaveCriticalSection (&pdbc->csDbc);
    return;
}

/*
**  UnlockEnv
**
**  Unlock entire environment for ODBC session.
**
**  On entry: LPENV-->environment block
**
**  Returns:  nothing
*/
void FASTCALL UnlockEnv (LPENV penv)
{
    ODBCLeaveCriticalSection (&IIODcsEnv);
    return;
}

