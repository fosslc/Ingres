/*
** Copyright (c) 1988, 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sp.h>
#include    <cs.h>
#include    <gc.h>
#include    <er.h>
#include    <ex.h>
#include    <lo.h>
#include    <me.h>
#include    <nm.h>
#include    <st.h>
#include    <tr.h>
#include    <pc.h>
#include    <csinternal.h>
#include    <signal.h>
#include    <clconfig.h>
#include    <mo.h>
#include    <meprivate.h>
#include    "csmgmt.h"
#include    "cslocal.h"

#include    <imagehlp.h>


/*
**  Name: CSHL.C - Control System "High Level" Functions
**
**  Description:
**      This module contains those routines which provide system dispatching
**      services to the DBMS (only, at the moment).  "High level" refers to
**      those routines which can be implemented in C as opposed to macro.
**
**          CS_xchg_thread() - Pick next task and return it.
**          CS_toq_scan() - Scan timeout queue.
**          CS_setup() - Run the thread thru its appropriate
**          functions based on the modes set
**          by the mainline code.
**          CS_find_scb() - Find the SCB belonging to a particular
**              thread id.
**          CS_dump_scb() - Dump the high level scb contents.
**          CS_rcv_request() - Prime system to receive a request
**              for new thread addition.
**          CS_del_thread() - Final processing for deletion of a thread.
**          CS_admin_task() - Thread addition and deletion thread.
**          CS_exit_handler() - Final CS server processing.
**
**  History:
**	25-jun-95 (emmag & canor01)
**	    Modified behaviour of CS_rcv_request for our 'new & improved'
**	    async gc.
**	25-jun-95 (emmag)
**	    In CS_setup we need to check for status of E_CS0008_INTERRUPTED
**	    which is returned from CSsuspend.  Not checking for it will
**	    result in us calling scc_send() an incorrect number of times,
**	    and next time we get to CS_setup we'd call SCF and get an 
**	    inconsisten message queue.  (Bug 62372)
**	11-jul-1995 (shero03,wonst02)
**	    Provide a way to end the admin_task loop
**	13-jul-1995 (shero03)
**	    Make CSterminate(KILL) the last thing to happen
**	11-Aug-1995 (shero03)
**	    Remove the reference to SCB + x70
**	24-aug-1995 (shero03)
**	    Attach SCBs to MO.
**      12-Dec-95 (fanra01)
**          Extracted data for DLLs on windows NT.
**      16-Jan-1996 (jenjo02)
**          Wait reason for CSsuspend() no longer defaults to CS_LOG_MASK,
**          which must now be explicitly stated.
**	01-feb-1996 (canor01)
**	    Change the admin thread to use an ASYNC listen for compatibility
**	    with STAR.
**	08-feb-1996 (canor01)
**	    Specify a particular thread to use a different startup routine
**	    (CS_prf_setup() instead of CS_setup()) so it can be tracked by 
**	    the Microsoft profiler.
**	08-feb-1996 (canor01)
**	    CS_swuser() calls Sleep(0) to give up the rest of its scheduled
**	    timeslice to another thread with equal or higher priority.
**	21-feb-1996 (canor01)
**	    Remove unused CsThreadsRunningEvent.
**	04-mar-1996 (canor01)
**	    Write session_accounting to application event log.
**	19-mar-1996 (canor01)
**	    Change method of obtaining thread-local storage for pSCB
**	    from "__declspec(thread)" to "TlsAlloc()".  We can have
**	    up to 64 of these structures. Right now the only other one
**	    is in EX.  This should be replaced with a generic TL
**	    facility later.
**	19-apr-1996 (canor01)
**	    Trap access violations in the thread and terminate only the
**	    thread, not the entire process.
**	25-apr-1996 (canor01)
**	    If a thread has been removed, allow it to break out of
**	    the send/receive loop.
**	23-may-1996 (canor01)
**	    Instead of simply terminating the thread, pass access 
**	    violations along to EX to let any registered handler do 
**	    what needs to be done.
**      16-sep-1996 (canor01)
**          If the server state is CS_TERMINATING, do not attempt to
**          receive another connection.
**	25-mar-1997 (canor01)
**	    Free any allocated thread-local storage on thread exit.
**	08-apr-97 (mcgem01)
**	    Changed CA-OpenIngres to OpenIngres.
**	22-apr-1997 (canor01)
**	    To make sure the listen thread has all the synchronization 
**	    objects it needs, make a call to GCinitiate() in CS_admin_task().
**      02-may-1997 (canor01)
**          Sessions created with the CS_CLEANUP_MASK will be shut
**          down before any other system threads.  When they exit, they
**          must call CSterminate() to allow the normal system threads
**          to exit.
**	15-may-97 (mcgem01)
**	    Clean up compiler warnings.  Add missing include files, and 
**	    add two parameters missing from call to GCinitiate. 
**	2-jul-1997 (canor01)
**	    Add preliminary code for CS_dump_stack().
**	7-nov-1997 (canor01)
**	    Changes for handling fatal errors: 1) don't overflow a local
**	    variable on the stack when dumping the stack on an exception;
**	    2) differentiate between "server closed" and "too many users";
**	    3) check for transitional state between closed and shutdown.
**	12-feb-1998 (canor01)
**	    Close cs_di_event handle when thread exits.
**      13-Feb-98 (fanra01)
**          Made changes to move attch of scb to MO just before we start
**          running.
**          This is so the SID is initialised to be used as the instance.
**      31-Mar-1998 (jenjo02)
**          Added *thread_id output parm to CSadd_thread() prototype.
**	22-jun-1998 (canor01)
**	    Protect access to session count fields.
**	26-jun-1998 (canor01)
**	    In CS_del_thread, session count mutex was being unlocked, but
**	    was not locked.
**	01-jul-1998 (canor01)
**	    In CS_del_thread, remove extra decrement of session count to
**	    bring into sync with Unix version.
**	27-jul-1998 (rajus01)
**	    Removed unused symbols( NUM_EVENTS, LISTEN, SHUTDOWN, GCC_COMPLETE,
**	    SYNC_EVENT ) defined for asynchrous events.
**      08-oct-1998 (canor01 for jenjo02)
**          Eliminate cs_cnt_mutex, adjust counts while holding
**	    Cs_known_list_sem instead.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	03-sep-1999 (somsa01)
**	    In CS_del_thread(), moved CS_detach_scb() call to RIGHT AFTER
**	    removing the scb from the known list, protecting it with the
**	    Cs_known_list_sem semaphore. In this way, we eliminate a hole
**	    whereby, while a session is exiting, another session accessing
**	    MO objects will not obtain information about the exiting session
**	    (and its scb) before detaching its scb from MO but AFTER its
**	    physical scb was taken off the scb list. This caused SEGVs in MO
**	    routines due to the fact that they obtained null SCB pointers
**	    and tried to dereference them.
**	15-nov-1999 (somsa01)
**	    Changed EXCONTINUE to EXCONTINUES.
**	15-dec-1999 (somsa01 for jenjo02, part II)
**	    Standardize cs_self = (CS_SID *)scb, session id's are always
**	    a pointer to the thread's SCB, never OS thread_id, which is
**	    contained in cs_thread_id. This eliminates the need to
**	    mutex and walk the known SCB list to find an SCB by sid
**	    (CSresume(), e.g.).
**	10-jan-2000 (somsa01)
**	    Removed scb printout in CS_del_thread(), as it is the same as
**	    the sid. Also, added setting of CS_BIOR_MASK and CS_BIOW_MASK,
**	    and added CS_checktime(). Added CS_fmt_scb().
**	24-jul-2000 (somsa01 for jenjo02)
**	    Append thread_id to session_id.
**	19-sep-2000 (somsa01)
**	    In CS_dump_stack for NT, changed u_longnat to u_i4.
**	10-oct-2000 (somsa01)
**	    Deleted cs_bio_done, cs_bio_time, cs_bio_waits, cs_dio_done,
**	    cs_dio_time, cs_dio_waits from Cs_srv_block, cs_dio, cs_bio
**	    from CS_SCB. Kept the MO objects for them and added MO
**	    methods to compute their values.
**	13-may-2002 (somsa01)
**	    In CS_dump_stack(), initialize cp to bp[1] before starting our
**	    stack walk.
**	14-mar-2003 (somsa01)
**	    In CS_del_thread(), call EX_TLSCleanup() and NMgtTLSCleanup()
**	    to clean up NM and EX TLS allocations, respectively.
**	23-jan-2004 (somsa01 for jenjo02)
**	    Resolve long-standing inconsistencies and inaccuracies with
**	    "cs_num_active" by computing it only when needed by MO
**	    or IIMONITOR. Added MO methods, external functions
**	    CS_num_active(), CS_hwm_active(), callable from IIMONITOR,
**	    e.g. Works for both MT and Ingres-threaded servers.
**	28-jan-2004 (fanra01)
**	    Added CSdump_stack().
**	22-jul-2004 (somsa01)
**	    Removed unnecessary include of erglf.h.
**      23-Jul-2004 (lakvi01)
**              SIR#112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	11-Nov-2010 (kschendel) SIR 124685
**	    Delete CS_SYSTEM ref, is in csinternal.  Fix prototypes.
*/

static STATUS CS_del_thread(CS_SCB *);
int IICSExceptFilter( EXCEPTION_POINTERS* ep );

/******************************************************************************
** externs
******************************************************************************/

GLOBALREF CS_SCB		Cs_main_scb;
GLOBALREF CS_SEMAPHORE	Cs_known_list_sem;
GLOBALREF HANDLE		CsMultiTaskEvent;
GLOBALREF HANDLE		hAsyncEvents[];
GLOBALREF HANDLE		hSignalShutdown;

FACILITYDEF DWORD TlsIdxSCB = 0;
GLOBALREF i4     		Cs_lastquant ;
static void dump_extended();


/*
** Global string expansions for CS values
**
** The following macros expand the strings for such as CS_STATE_ENUM and
** CS_MASK_MASKS to generate the global *_names strings. Additionally the values
** used are checked for their order and for their being no gaps - either of
** which would cause a mismatch when displayed from such as TRdisplay.
**
** History:
**	08-Feb-2007 (kiria01) b117581
**	    Build the cs_state_names and cs_mask_names from the table macroes
**	    and check their consistancy.
**	09-Apr-2007 (drivi01) b117581
**	    Fix change #485729, replace "GLOBALREF const" with GLOBALCONSTREF and
**	    "GLOBALDEF const" with GLOBALCONSTDEF for cs_state_names and 
**	    cs_mask_names to prevent compile problems of c++ files. 
*/
#define _DEFINESEND

/*
** Define the comma separated lists of names as global strings
*/
#define _DEFINE(s,v) #s ","
GLOBALCONSTDEF char cs_state_names[] = CS_STATE_ENUMS ;
GLOBALCONSTDEF char cs_mask_names[] = CS_MASK_MASKS ;
#undef _DEFINE

/*
** Check that there are no gaps in the numbers.
** To do this we OR the bit values and check that it
** is a number of form 2**n -1. (2**n has lower n-1 bits clear
** bug 2**n -1 has n-1 lower bits set so ANDing these needs
** to yield 0)
*/

/* Check for gaps in cs_state_enums */
#define _DEFINE(s,v) (1<<v)|
#if ((CS_STATE_ENUMS 0) & ((CS_STATE_ENUMS 0)+1))!=0
# error "cs_state_enums are not contiguous"
#endif
#undef _DEFINE

/* Check for gaps in cs_mask_masks */
#define _DEFINE(s,v) |v
#if ((0 CS_MASK_MASKS) & ((0 CS_MASK_MASKS)+1))!=0
# error "cs_mask_masks are not contiguous"
#endif
#undef _DEFINE

/* Check that the values are ordered */
#define _DEFINE(s,v) >= v || v
#if ((-1) CS_STATE_ENUMS == 0)
# error "cs_state_enums are not ordered properly"
#endif
#if ((-1) CS_MASK_MASKS == 0)
# error "cs_mask_masks are not ordered properly"
#endif
#undef _DEFINE
#undef _DEFINESEND

/*
** Other defines
*/

/* number of 100-nano-seconds in 1 second */
#define         HNANO   10000000

/* number of 100-nano-seconds in 1 milli-second */
#define         HNANO_PER_MILLI 10000

/* number of milli-seconds in 1 second */
#define         MILLI   1000
/* number of seconds in the lo word  */
#define         SECONDS_PER_DWORD 0xFFFFFFFF / HNANO_PER_MILLI



/******************************************************************************
**
** Name: CS_setup() - Place bottom level calls to indicated procedures
**
** Description:
**      This routine is called after the initial stack layout is set up.
**      It simply sits in a loop and calls the indicated routine(s) back
**      to perform their actions.
**
**  If the thread being called is the admin or idle threads, then special
**  action is taken.
*
*
** Inputs:
**      none
**
** Outputs:
**  none
**
**  Returns:
**      Never (on purpose).
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
**	24-jul-2000 (somsa01 for jenjo02)
**	    Append thread_id to session_id.
**
******************************************************************************/
STATUS
cs_handler(
	   EX_ARGS * exargs)
{
	char            buf[1024];
	int             i;
	CS_SCB		*scb;

	CSget_scb(&scb);

	if (exargs->exarg_num == EXKILL || exargs->exarg_num == EXINTR) {
		i = 0;
		CSterminate(CS_KILL, &i);
		return EXCONTINUES;
	}

	if (exargs->exarg_num == EX_UNWIND)
		return (EXDECLARE);

	if (!EXsys_report(exargs, buf)) {
		STprintf(buf,
		         "Exception is %x (%d.)",
		         exargs->exarg_num,
		         exargs->exarg_num);
		for (i = 0; i < exargs->exarg_count; i++)
			STprintf(buf + STlength(buf),
			         ",arg[%d]=%x",
			         i,
			         exargs->exarg_array[i]);
	}

	TRdisplay("cs_handler (session %p:%d): %t\n",
		  scb->cs_self, scb->cs_thread_id,
		  STlength(buf), buf);
	return (EXDECLARE);
}

VOID
CS_setup(void *parm)
{
    STATUS     (*rtn) ();
    STATUS     status;
    EX_CONTEXT excontext;
    CS_SCB     **ppSCB;
    CS_SCB     *pSCB;

    /* trap the really bad exceptions for this thread only */
    __try
    {
	
	/* make sure thread-local storage index is initialized */
        if ( TlsIdxSCB == 0 )
	{
            TlsIdxSCB = TlsAlloc();
	    TlsSetValue( TlsIdxSCB, NULL );
	}
        ppSCB = TlsGetValue( TlsIdxSCB );
        if (ppSCB == NULL)
        {
            ppSCB = (CS_SCB **) MEreqmem(0,sizeof(CS_SCB **),TRUE,NULL);
            TlsSetValue( TlsIdxSCB, ppSCB );
        }

        *ppSCB = (CS_SCB *) parm;

	pSCB = *ppSCB;

	/*
	 * It may not yet be safe to begin. In particular, if this thread
	 * was added by the server startup routine running out of
	 * CSdispatch, then CSdispatch may not  have completed its work, so
	 * we will wait for CSdispatch to authorize us to proceed by
	 * waiting on the semaphore which it will post when it is  ready.
	 * The infinite wait time is debatable, but it's not clear that we
	 * can decide on a non-infinite timeout...
	 */

	WaitForSingleObject(CsMultiTaskEvent,
	                    INFINITE);

	if (EXdeclare(cs_handler, &excontext)) {
		pSCB->cs_mode = CS_TERMINATE;
		if (pSCB->cs_mask & CS_FATAL_MASK) {

			(*Cs_srv_block.cs_elog) (E_CS00FF_FATAL_ERROR, 0, 0);

                        CSp_semaphore(1, &Cs_known_list_sem);
			--Cs_srv_block.cs_num_sessions;

			if (pSCB->cs_thread_type == CS_USER_THREAD)
				Cs_srv_block.cs_user_sessions--;
                        CSv_semaphore(&Cs_known_list_sem);

			ExitThread(0);
		} else {
			(*Cs_srv_block.cs_elog) (E_CS0014_ESCAPED_EXCPTN, 0, 0);
		}
		pSCB->cs_mask |= CS_FATAL_MASK;
	}

	/*
	 * Set session control block pointer to the server control block.
	 * This field was not set at thread initialization because the
	 * value may not have been known.
	 */

	pSCB->cs_svcb = Cs_srv_block.cs_svcb;

        if ( Cs_srv_block.cs_scbattach )
            (*Cs_srv_block.cs_scbattach)( pSCB );

	for (;;) {
		switch (pSCB->cs_mode) {
		case CS_INITIATE:
		case CS_TERMINATE:
		case CS_INPUT:
		case CS_OUTPUT:
		case CS_READ:
			rtn = Cs_srv_block.cs_process;
			break;

		default:
			(*Cs_srv_block.cs_elog) (E_CS000C_INVALID_MODE, 0, 0);
			pSCB->cs_mode = CS_TERMINATE;
			continue;
		}

		if (pSCB->cs_mode == CS_INPUT) {

			WaitForSingleObject(pSCB->cs_access_sem,
			                    INFINITE);
			pSCB->cs_state   = CS_EVENT_WAIT;
			pSCB->cs_mask   |= CS_INTERRUPT_MASK;
			pSCB->cs_memory |= CS_BIOR_MASK;
			ReleaseMutex(pSCB->cs_access_sem);

			status = (*Cs_srv_block.cs_read) (pSCB, 0);

			if (status == OK)
				status = CSsuspend(CS_INTERRUPT_MASK |
				                    CS_BIOR_MASK,
				                   0,
				                   0);
			else
			{
			    WaitForSingleObject(pSCB->cs_access_sem, INFINITE);
			    pSCB->cs_state   = CS_COMPUTABLE;
			    pSCB->cs_mask   &= ~CS_INTERRUPT_MASK;
			    pSCB->cs_memory &= ~CS_BIOR_MASK;
			    ReleaseMutex(pSCB->cs_access_sem);
			}

			if ((status) &&
			    (status != E_CS0008_INTERRUPTED) &&
			    (status != E_CS001E_SYNC_COMPLETION)) {
				pSCB->cs_mode = CS_TERMINATE;
			}

		} else if (pSCB->cs_mode == CS_READ) {

			WaitForSingleObject(pSCB->cs_access_sem,
			                    INFINITE);
			pSCB->cs_mode = CS_INPUT;
			pSCB->cs_mask &= ~(CS_EDONE_MASK |
			                  CS_IRCV_MASK |
			                  CS_TO_MASK |
			                  CS_ABORTED_MASK |
			                  CS_INTERRUPT_MASK |
			                  CS_TIMEOUT_MASK);
			pSCB->cs_memory = 0;
			ReleaseMutex(pSCB->cs_access_sem);

		}

		status = (*rtn) (pSCB->cs_mode, pSCB, &pSCB->cs_nmode);

		if (status)
			pSCB->cs_nmode = CS_TERMINATE;
		if (pSCB->cs_mode == CS_TERMINATE)
			break;

		if ((pSCB->cs_nmode == CS_OUTPUT) ||
		    (pSCB->cs_nmode == CS_EXCHANGE)) {
			do {
				WaitForSingleObject(pSCB->cs_access_sem,
						    INFINITE);
				pSCB->cs_state   = CS_EVENT_WAIT;
				pSCB->cs_mask   |= CS_INTERRUPT_MASK;
				pSCB->cs_memory |= CS_BIOW_MASK;
				ReleaseMutex(pSCB->cs_access_sem);
				
				status = (*Cs_srv_block.cs_write) (pSCB, 0);
				if (status == OK)
					status = CSsuspend(CS_INTERRUPT_MASK |
					                   CS_BIOW_MASK,
					                   0,
					                   0);
				else
				{
				    WaitForSingleObject(pSCB->cs_access_sem,
							INFINITE);
				    pSCB->cs_state   = CS_COMPUTABLE;
				    pSCB->cs_mask   &= ~CS_INTERRUPT_MASK;
				    pSCB->cs_memory &= ~CS_BIOW_MASK;
				    ReleaseMutex(pSCB->cs_access_sem);
				}
			} while ( ((status == OK) || 
				  (status == E_CS0008_INTERRUPTED)) &&
				  (pSCB->cs_mask & CS_DEAD_MASK) == 0);

			if ((status != OK) &&
			    (status != E_CS0008_INTERRUPTED) &&
			    (status != E_CS001D_NO_MORE_WRITES)) {
				pSCB->cs_nmode = CS_TERMINATE;
			}
		}

		if ((pSCB->cs_mask & CS_DEAD_MASK) == 0) {
			switch (pSCB->cs_nmode) {
			case CS_INITIATE:
			case CS_INPUT:
			case CS_OUTPUT:
			case CS_TERMINATE:
				pSCB->cs_mode = pSCB->cs_nmode;
				break;

			case CS_EXCHANGE:
				pSCB->cs_mode = CS_INPUT;
				break;

			default:
				(*Cs_srv_block.cs_elog) (E_CS000D_INVALID_RTN_MODE, 0, 0);
				pSCB->cs_mode = CS_TERMINATE;
				break;
			}
		} else {
			pSCB->cs_mode = CS_TERMINATE;
		}

	}


        if ( Cs_srv_block.cs_scbdetach )
            (*Cs_srv_block.cs_scbdetach)( pSCB );

	CS_del_thread(pSCB);

	ExitThread(0);
    }
    __except ( IICSExceptFilter( GetExceptionInformation() ) )
    {
        ; /* should not get here */
    }
}
int
IICSExceptFilter( EXCEPTION_POINTERS* ep )
{
    EXCEPTION_RECORD *exrp = ep->ExceptionRecord;
    CONTEXT *ctxtp = ep->ContextRecord;
    
    switch ( exrp->ExceptionCode )
    {
        case EXCEPTION_ACCESS_VIOLATION :
    	    EXsignal( EXSEGVIO, 3, (i4)exrp->ExceptionCode, exrp, ctxtp );
	    return ( EXCEPTION_CONTINUE_EXECUTION );

	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
    	    EXsignal( EXBNDVIO, 3, (i4)exrp->ExceptionCode, exrp, ctxtp );
	    return ( EXCEPTION_CONTINUE_EXECUTION );

	case EXCEPTION_DATATYPE_MISALIGNMENT:
    	    EXsignal( EXBUSERR, 3, (i4)exrp->ExceptionCode, exrp, ctxtp );
	    return ( EXCEPTION_CONTINUE_EXECUTION );

	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    	    EXsignal( EXFLTDIV, 3, (i4)exrp->ExceptionCode, exrp, ctxtp );
	    return ( EXCEPTION_CONTINUE_EXECUTION );

	case EXCEPTION_FLT_OVERFLOW:
    	    EXsignal( EXFLTOVF, 3, (i4)exrp->ExceptionCode, exrp, ctxtp );
	    return ( EXCEPTION_CONTINUE_EXECUTION );

	case EXCEPTION_FLT_UNDERFLOW:
    	    EXsignal( EXFLTUND, 3, (i4)exrp->ExceptionCode, exrp, ctxtp );
	    return ( EXCEPTION_CONTINUE_EXECUTION );

	case EXCEPTION_FLT_DENORMAL_OPERAND:
	case EXCEPTION_FLT_INEXACT_RESULT:
	case EXCEPTION_FLT_INVALID_OPERATION:
	case EXCEPTION_FLT_STACK_CHECK:
    	    EXsignal( EXFLOAT, 3, (i4)exrp->ExceptionCode, exrp, ctxtp );
	    return ( EXCEPTION_CONTINUE_EXECUTION );

	case EXCEPTION_ILLEGAL_INSTRUCTION:
    	    EXsignal( EXOPCOD, 3, (i4)exrp->ExceptionCode, exrp, ctxtp );
	    return ( EXCEPTION_CONTINUE_EXECUTION );

	case EXCEPTION_IN_PAGE_ERROR:
    	    EXsignal( EXUNKNOWN, 3, (i4)exrp->ExceptionCode, exrp, ctxtp );
	    return ( EXCEPTION_CONTINUE_EXECUTION );

	case EXCEPTION_INT_DIVIDE_BY_ZERO:
    	    EXsignal( EXINTDIV, 3, (i4)exrp->ExceptionCode, exrp, ctxtp );
	    return ( EXCEPTION_CONTINUE_EXECUTION );

	case EXCEPTION_INT_OVERFLOW:
    	    EXsignal( EXINTOVF, 3, (i4)exrp->ExceptionCode, exrp, ctxtp );
	    return ( EXCEPTION_CONTINUE_EXECUTION );

	case 0xc0000008: /* bad handle */
	    return ( EXCEPTION_CONTINUE_EXECUTION );
    
       default:
    	    EXsignal( EXUNKNOWN, 3, (i4)exrp->ExceptionCode, exrp, ctxtp );
	    return ( EXCEPTION_CONTINUE_SEARCH );
    }
}

/******************************************************************************
**
** Name: CS_find_scb    - Find the scb given the session id
**
** Description:
**
** Inputs:
**      sid                             The session id to find
**
** Outputs:
**  Returns:
**      the SCB in question, or 0 if non available
**  Exceptions:
**
**
** Side Effects:
**      none
**
** History:
**	15-dec-1999 (somsa01 for jenjo02, part II)
**	    Standardize cs_self = (CS_SID *)scb, session id's are always
**	    a pointer to the thread's SCB, never OS thread_id, which is
**	    contained in cs_thread_id. This eliminates the need to walk
**	    the known SCB list to find an SCB by sid.
**
******************************************************************************/
CS_SCB *
CS_find_scb(CS_SID sid)
{
    CS_SCB *scb;

    /* If CS_ADDER_ID, return the static Admin thread SCB */
    if (sid == CS_ADDER_ID)
	return((CS_SCB *)&Cs_main_scb);
    else if ((scb = (CS_SCB *)sid) && scb->cs_type == CS_SCB_TYPE)
	return(scb);
    else
	return(CS_NULL_ID);
}

/******************************************************************************
**
** Name: CS_dump_scb    - Display the contents of an scb
**
** Description:
**      This routine simply displays the scb via TRdisplay().  It is intended
**      primarily for interactive debugging.
**
** Inputs:
**      sid  sid of scb in question
**      scb  scb in question.  Either will do If scb is given, it is used.
**
** Outputs:
**  Returns:
**      VOID
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
******************************************************************************/
static VOID
CS_dump_scb(CS_SID sid,
            CS_SCB *scb,
	    PTR output_arg,
	    TR_OUTPUT_FCN *output_fcn)
{
    char        buf[128];

	if (scb == 0) {
		scb = CS_find_scb(sid);
		if (scb == 0) {
			TRdisplay("No SCB found for sid %x%< (%d.)\n", sid);
			return;
		}
	}
	TRformat(output_fcn,output_arg,buf,sizeof(buf)-1,"\n>>>CS_SCB found at %x<<<\n\n", scb);
	TRformat(output_fcn,output_arg,buf,sizeof(buf)-1,"cs_next: %x\tcs_prev: %x\n", scb->cs_next, scb->cs_prev);
	TRformat(output_fcn,output_arg,buf,sizeof(buf)-1,"cs_length: %d.\tcs_type: %x\n", scb->cs_length, scb->cs_type);
	TRformat(output_fcn,output_arg,buf,sizeof(buf)-1,"cs_self: %x%< (%d.)\n", scb->cs_self);
	TRformat(output_fcn,output_arg,buf,sizeof(buf)-1,"cs_stk_size: %x%< (%d.)\n", scb->cs_stk_size);
	TRformat(output_fcn,output_arg,buf,sizeof(buf)-1,"cs_state: %w %<(%x)\tcs_mask: %v %<(%x)\n",
            cs_state_names,
            scb->cs_state,
            cs_mask_names,
            scb->cs_mask);
	TRformat(output_fcn,output_arg,buf,sizeof(buf)-1,"cs_mode: %w%<(%x)\tcs_nmode: %w%<(%x)\n",
	 "<invalid>,CS_INITIATE,CS_INPUT,CS_OUTPUT,CS_TERMINATE,CS_EXCHANGE",
		  scb->cs_mode,
	 "<invalid>,CS_INITIATE,CS_INPUT,CS_OUTPUT,CS_TERMINATE,CS_EXCHANGE",
		  scb->cs_nmode);
	TRformat(output_fcn,output_arg,buf,sizeof(buf)-1,"cs_thread_type: %w%<(%x)\n",
"CS_NORMAL,CS_MONITOR,CS_FAST_COMMIT,CS_WRITE_BEHIND,CS_SERVER_INIT,CS_EVENT,\
CS_2PC,CS_RECOVERY,CS_LOGWRITER,CS_CHECK_DEAD,CS_GROUP_COMMIT,CS_FORCE_ABORT,\
CS_AUDIT,CS_CP_TIMER,CS_CHECK_TERM,CS_SECAUD_WRITER,CS_WRITE_ALONG,CS_READ_AHEAD,\
CS_INTRNL_THREAD,CS_USER_THREAD", scb->cs_thread_type);
	TRformat(output_fcn,output_arg,buf,sizeof(buf)-1,"cs_username: %32.32s\n",scb->cs_username);
	TRformat(output_fcn,output_arg,buf,sizeof(buf)-1,"cs_sem_count: %x\n", scb->cs_sem_count);
}

/******************************************************************************
**
** Name: CS_rcv_request - Prime system to receive a request for new session
**
** Description:
**  This routine is called to queue a request to the mainline code for
**  a new thread.  It is primarily a common interface point.
**
** Inputs:
**  None.
** Outputs:
**  None.
**  Returns:
**      VOID
**  Exceptions:
**      none
**
** Side Effects:
**      A new session is created.
**
** History:
**	10-jan-2000 (somsa01)
**	    Added setting of CS_BIOR_MASK.
**	18-Dec-2006 (wonca01) BUG 117262
**	    Change cs_elog() parameter from STATUS to
**	    CL_ERR_DESC in order for a valid error code to 
**	    reported.
**
******************************************************************************/
static STATUS
CS_rcv_request(void)
{
	STATUS status;
	CL_ERR_DESC *err_code;
	CS_SCB *scb;

	CSget_scb(&scb);

	WaitForSingleObject(scb->cs_access_sem, INFINITE);
	scb->cs_state   = CS_EVENT_WAIT;
	scb->cs_mask   |= CS_INTERRUPT_MASK;
	scb->cs_memory |= CS_BIOR_MASK;
	ReleaseMutex(scb->cs_access_sem);

	status = Cs_srv_block.cs_saddr(Cs_srv_block.cs_crb_buf, 0);

	WaitForSingleObject(scb->cs_access_sem, INFINITE);
	scb->cs_state   = CS_COMPUTABLE;
	scb->cs_mask   &= ~CS_INTERRUPT_MASK;
	scb->cs_memory &= ~CS_BIOR_MASK;
	ReleaseMutex(scb->cs_access_sem);

	if (status) 
	{
		SETWIN32ERR ( err_code, status, ER_create);
	    (*Cs_srv_block.cs_elog) (E_CS0011_BAD_CNX_STATUS, err_code, 0);
            MEfill( sizeof(Cs_srv_block.cs_crb_buf), 0, Cs_srv_block.cs_crb_buf );
	    return (FAIL);
	} 
	else
	{
            status = CSsuspend( CS_INTERRUPT_MASK | CS_BIOR_MASK, 0, 0 );
            if (Cs_srv_block.cs_mask & CS_SHUTDOWN_MASK)
	    {
                if ((Cs_srv_block.cs_mask & CS_FINALSHUT_MASK) == 0)
                {
                    CSterminate(CS_START_CLOSE, 0);
                    CS_swuser();
                }
                /* give other threads a chance to shut down cleanly */
                if ( Cs_srv_block.cs_num_sessions > 1 )
                    Sleep(2000);

                status = OK;
	    }
            return (status);
	}
}

/*
**  Name: CS_del_thread  - Delete the thread.
**
**  Description:
**	This routine removes and traces of the thread from existence in the
**	system.  This amounts, primarily, to deallocating memory.
**
**	If the server is keeping accounting records for each session then
**	it logs the accounting message indicating the CPU utilization for this
**	thread.
**
**  Inputs:
**	scb	scb to delete
**
**  Outputs:
**	none
**
**	Returns:
**	    STATUS
**
**	Exceptions:
**	    none
**
**  Side Effects:
**      none
**
**  History:
**	25-mar-1997 (canor01)
**	    Free any allocated thread-local storage on thread exit.
**	25-mar-1997 (canor01)
**	    Free any allocated thread-local storage on thread exit.
**      02-may-1997 (canor01)
**          Sessions created with the CS_CLEANUP_MASK will be shut
**          down before any other system threads.  When they exit, they
**          must call CSterminate() to allow the normal system threads
**          to exit.
**	19-aug-1997 (canor01)
**	    Remove second closing of cs_thread_handle.
**	03-sep-1999 (somsa01)
**	    Moved CS_detach_scb() call to RIGHT AFTER removing the scb
**	    from the known list, protecting it with the Cs_known_list_sem
**	    semaphore. In this way, we eliminate a hole whereby, while a
**	    session is exiting, another session accessing MO objects will
**	    not obtain information about the exiting session (and its scb)
**	    before detaching its scb from MO but AFTER its physical scb was
**	    taken off the scb list. This caused SEGVs in MO routines due to
**	    the fact that they obtained null SCB pointers and tried to
**	    dereference them.
**	15-dec-1999 (somsa01)
**	    Print out the thread id as well as the session id.
**	10-jan-2000 (somsa01)
**	    Removed scb printout as it is the same as the sid.
**	14-mar-2003 (somsa01)
**	    Call EX_TLSCleanup() and NMgtTLSCleanup() to clean up
**	    NM and EX TLS allocations, respectively.
**	23-jan-2004 (somas01)
**	    Removed messing with cs_num_active.
*/
static STATUS
CS_del_thread(CS_SCB *scb)
{
	char	acc_rec[2048];
	HANDLE	hEventLog;
	STATUS	status;

	CSp_semaphore(1, &Cs_known_list_sem);     /* exclusive */
	scb->cs_next->cs_prev = scb->cs_prev;
	scb->cs_prev->cs_next = scb->cs_next;

	/*  Remove from known thread tree */
	CS_detach_scb(scb);

	CSv_semaphore(&Cs_known_list_sem);

	CloseHandle(scb->cs_access_sem);
	CloseHandle(scb->cs_event_sem);
	CloseHandle(scb->cs_di_event);
	CloseHandle(scb->cs_thread_handle);

        /* 
	** Decrement session count.  
	** If user thread decrement user session count 
	*/ 
        CSp_semaphore(1, &Cs_known_list_sem);
        Cs_srv_block.cs_num_sessions--;
        if (scb->cs_thread_type == CS_USER_THREAD ||
            scb->cs_thread_type == CS_MONITOR)
        {
            Cs_srv_block.cs_user_sessions--;
        }
        CSv_semaphore(&Cs_known_list_sem);

	TRdisplay("CS_del_thread: sid %x, tid %d\n",
	          (i4 ) scb->cs_self,
	          scb->cs_thread_id);

	/* log the session if session accounting enabled */
	if ((Cs_srv_block.cs_mask & CS_ACCNTING_MASK) &&
	    (scb->cs_thread_type == CS_USER_THREAD))
	{        
		FILETIME        kerneltime;
		FILETIME        usertime;
		FILETIME        dummy;
		i4              kernelcpu;
		i4              usercpu;
		char		eventsource[80];

		if (! GetThreadTimes(scb->cs_thread_handle,
		                &dummy, &dummy,
		                &kerneltime, &usertime) )
		{
		    status = GetLastError();
		}

		usercpu = usertime.dwHighDateTime * SECONDS_PER_DWORD * MILLI;
		usercpu += usertime.dwLowDateTime / HNANO_PER_MILLI;

		kernelcpu = kerneltime.dwHighDateTime * SECONDS_PER_DWORD * MILLI;
		kernelcpu += kerneltime.dwLowDateTime / HNANO_PER_MILLI;

		usercpu += kernelcpu;
		scb->cs_cputime = usercpu;

		STprintf( acc_rec, "%sDBMS Session:  User: \'%32s\'  Connect time: %ld sec CPU time: %ld  BIO count: %ld  DIO count: %ld",
		          SystemVarPrefix, 
			  scb->cs_username, 
		          TMsecs() - scb->cs_connect,
		          scb->cs_cputime,
		          scb->cs_bior + scb->cs_biow,
		          scb->cs_dior + scb->cs_diow );

		/* open local machine, application log */
		STprintf( eventsource, "%s/NT", SystemProductName );
		hEventLog = RegisterEventSource( NULL, eventsource );
		if ( hEventLog == NULL )
		{
			status = GetLastError();
		}
		else
		{
			char	*acc_rec_ptr = acc_rec;
			char	**acc_rec_ptr_ptr = &acc_rec_ptr;
	
			if ( ReportEvent( hEventLog,
				  	EVENTLOG_INFORMATION_TYPE,
				  	(WORD) 0,      /* event category */
				  	(DWORD) 2002,  /* event identifier */
				  	(PSID) NULL,
				  	(WORD) 1,      /* number of strings */
				  	(DWORD) 0,
				  	acc_rec_ptr_ptr,
				  	NULL ) == FALSE)
				status = GetLastError();
			DeregisterEventSource( hEventLog );
		}
	}

        /* if we're a cleanup thread, let the other system threads die too */
        if ( scb->cs_cs_mask & CS_CLEANUP_MASK )
	{
	    i4  active_count = 0;
            CSterminate(CS_CLOSE, &active_count);
	}

	Cs_srv_block.cs_scbdealloc(scb);

	/* Free memory allocated in thread-local storage */
	EX_TLSCleanup();
	NMgtTLSCleanup();
	ME_tls_destroyall( &status );
	ME_tls_destroy( TlsIdxSCB, &status );

	/* Decrement session count. */

	if (Cs_srv_block.cs_num_sessions <= 0)
	{
	    /*
	     * If this is the last server session -- signal main task to quit.
	     */
	    SetEvent(hSignalShutdown);
	}

	return (OK);
}

/******************************************************************************
**
** Name: cs_admin_task  - Perform administrative functions
**
** Description:
**  This routine performs the administrative functions in the server which
**  must be performed in thread context.  These include the functions of
**  adding threads, deleting threads, and performing accounting functions.
**
** Inputs:
**      mode        Ignored.
**      scb         the scb for the admin thread
**      next_mode   ignored.
**      io_area     ignored.
**
**      These ignored parms are included so that this thread looks
**      like all others.
**
** Outputs:
**  Returns:
**      E_DB_OK
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
**      16-sep-1996 (canor01)
**          If the server state is CS_TERMINATING, do not attempt to
**          receive another connection.
**
******************************************************************************/
STATUS
CS_admin_task(void)
{
	CS_SCB *scb;

	CSget_scb(&scb);

	GCinitiate(NULL, NULL);/* make sure GC initializes for listen thread */

	for (;;)
        {

            if ( Cs_srv_block.cs_state == CS_TERMINATING )
                return OK;

	    if (CS_rcv_request())
		continue;

	    if ((Cs_srv_block.cs_mask & CS_SHUTDOWN_MASK) &&
		(Cs_srv_block.cs_mask & CS_FINALSHUT_MASK) &&
		(Cs_srv_block.cs_num_sessions == 0))
	    {
		return OK;
	    }

	    if (((Cs_srv_block.cs_num_sessions + 1) >
	          Cs_srv_block.cs_max_sessions) ||
	         (Cs_srv_block.cs_mask & CS_SHUTDOWN_MASK)) {
		if ( Cs_srv_block.cs_mask & CS_SHUTDOWN_MASK )
		    Cs_srv_block.cs_reject(Cs_srv_block.cs_crb_buf, 
					   E_CS0047_SHUT_STATE);
	        else
	            Cs_srv_block.cs_reject(Cs_srv_block.cs_crb_buf, 1);
	    }
	    else {
		CSadd_thread(0, Cs_srv_block.cs_crb_buf, 0, NULL, NULL);
	    }

	}
	return FAIL;
}

/*{
** Name: CS_fmt_scb	- Format debugging scb
**
** Description:
**	This routine formats the interesting information from the
**	session control block passed in.  This routine is called by
**	the monitor task to aid in debugging.
**
** Inputs:
**	scb		The scb to format
**	iosize		Size of area to fill
**	area		ptr to area to fill
**
** Outputs:
**	none
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	none
**
** History:
**	10-jan-2000 (somsa01)
**	    Created, but not much to print out here right now.
**	24-jul-2000 (somsa01 for jenjo02)
**	    Append thread_id to session_id.
*/
VOID
CS_fmt_scb(CS_SCB *scb, i4 iosize, char *area)
{
    i4		length = 0;
    char	*buf = area;
    CS_STK_CB	*stk_hdr;

    if (iosize < 512)
    {
	STprintf(buf, "Insufficient space\r\n");
	return;
    }

    STprintf(buf, "---Dump of scb for session %p:%d---\r\n",
	     scb, scb->cs_thread_id);
    buf += STlength(buf);
    STprintf(buf, "cs_next: %p\tcs_prev: %p\r\n",
	     scb->cs_next, scb->cs_prev);
    buf += STlength(buf);
    STprintf(buf, "cs_length(%d.+): %d.\tcs_type(%x): %x\r\n",
	     sizeof(CS_SCB), scb->cs_length, CS_SCB_TYPE, scb->cs_type);
    buf += STlength(buf);
    STprintf(buf, "cs_tag: %4s\tcs_self: %p\r\n",
	     &scb->cs_tag, scb->cs_self);
    buf += STlength(buf);
}

/******************************************************************************
**
**
**
******************************************************************************/
VOID
CS_breakpoint()
{
}

void
CS_swuser()
{
	Sleep(0);	/* relinquish remainder of thread's timeslice */
}

VOID
CS_prf_setup(void *parm)
{

	printf("\nCS_prf_setup invoked for profiling init\n");

	CS_setup( parm );

	return;
}

/*{
** Name: CS_default_output_fcn	- default output function for 
**                                CS_dump_stack.
**
** Description:
**      This routine logs messages to the errlog.log
**
** Inputs:
**      arg1	   - ptr to argument list.
**      msg_len    - length of buffer.
**      msg_buffer - ptr to message buffer.
**      
** Outputs:
**      Returns:
**          Doesn't
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      22-Oct-2002 (hanal04) Bug 108986
**          Created.
*/
static void
CS_default_output_fcn( PTR arg1, i4 msg_len, char *msg_buffer)
{
 CL_ERR_DESC err_code;
 
 ERsend (ER_ERROR_MSG, msg_buffer, msg_len, &err_code);
}
 


/*{
** Name: CS_dump_stack()        - dump out stack trace.
**
** Description:
**      This routine calls a system specific routine which prints out a stack
**      trace of the given session.  This may not be possible to do on all
**      systems so it is not mandatory that this routine be implemented.
**      This routine is meant as a debugging aide and is not necessary for
**      the proper functioning of the server.  Providing an implementation
**      of this function will make diagnosis of AV's in the server much easier
**      in the field.
**
**      Currently this routine is called in two places.  It is called by
**      the EXsys_report() and it is called by the iimonitor "debug" command.
**
**      If no implementation of this routine exists for a given platform,
**      calling this routine will silently result in no work done (ie. the
**      function will simply return doing nothing).
**
** Inputs:
**      scb                             session id of stack to dumped.  If the
**                                      value input is NULL then the current
**                                      stack is assumed.
**	output_arg			Passed to output_fcn
**      output_fcn                      Function to call to perform the output.
**                                      This routine will be called as
**                                          (*output_fcn)(output_arg,
**                                                          length,
**                                                          buffer)
**                                      where buffer is the length character
**                                      output string
**      verbose                         Should we print some information about
**                                      where this diagnostic is coming from?
**
**
** Outputs:
**      none.
**
**      Returns:
**          VOID
**
** History:
**      12-feb-91 (mikem)
**          Created.
**      31-dec-1993 (andys)
**          Add verbose argument.
**      19-sep-1995
**          Added stack_p parameter and add dr6_us5 to the list.
**	10-jan-2000 (somsa01)
**	    ctxtp may be NULL.
**	19-sep-2000 (somsa01)
**	    Changed u_longnat to u_i4.
**	13-may-2002 (somsa01)
**	    Initialize cp to bp[1] before starting our stack walk.
*/
VOID
CS_dump_stack( CS_SCB *scb, void *ctxarg, PTR output_arg,
	TR_OUTPUT_FCN *output_fcn, i4  verbose )
{
# ifdef NT_INTEL
    CONTEXT	*ctxtp = (CONTEXT *) ctxarg;
    u_i4   	**stack_p = NULL;
    char        buf[128];
    char	parmbuf[1024];
    int		parmlen;
    char	numbuf[20];
    char        name[GCC_L_PORT];
    char        verbose_string[80];
    u_i4        **sp, *getsp();
    u_i4        **bp;
    u_i4        pc;
    u_i4        parmcount;
    i4          stack_depth = 100;
    CS_SCB      *cur_scb;
    PID         pid;            /* our process id                           */
    unsigned short retinst;
    unsigned char  *cp;
    u_int       i;
    u_i4        *funcentry;
    char        funcstr[128];
    char	curstr[128];
    char	dllstr[128];
    u_i4        funcoffset;
    bool        tfstatus;
    bool	havesymbols = TRUE;
    DWORD       displacement;
    struct _xsymbol {
                IMAGEHLP_SYMBOL symbol;
		char xname[256];
    } xsymbol;
    IMAGEHLP_MODULE module;
    HANDLE      hCurProc;
    DWORD       status;
    DWORD	baseaddr;

    if (output_fcn == NULL)
    {
	output_arg = NULL;
	output_fcn = CS_default_output_fcn;
    }

    if (ctxtp)
	stack_p = (u_i4 **)ctxtp->Esp;

    if (scb)
    {
	CS_dump_scb(0, scb, output_arg, output_fcn);
	return;
    }
    else
    {
        /*
        ** No passed scb: use the passed stack pointer if it exists or
        ** get the fp from the stack and use the current scb for termination.
        */
	if ( !stack_p )
	    return;
        CSget_scb(&cur_scb);
        sp = stack_p;
	CS_dump_scb(0, cur_scb, output_arg, output_fcn);
    }

    module.SizeOfStruct = sizeof(IMAGEHLP_MODULE);
    xsymbol.symbol.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
    xsymbol.symbol.MaxNameLength = sizeof(xsymbol.xname);
    hCurProc = GetCurrentProcess();
    tfstatus = SymInitialize( hCurProc, NULL, TRUE );
    if ( tfstatus != TRUE )
    {
	status = GetLastError();
	havesymbols = FALSE;
    }

    if (verbose)
    {
        STlcopy(Cs_srv_block.cs_name,
                name,
                sizeof(Cs_srv_block.cs_name));
 
        PCpid(&pid);
        STprintf(verbose_string, "Stack dmp name %s  pid %d session %x: ",
		name, pid, scb ? scb->cs_self : (cur_scb?cur_scb->cs_self:GetCurrentThreadId()));
    }
    else
    {
        verbose_string[0] = EOS;
    }

    /* in case stack is corrupt, only print 100 levels so that we don't
    ** loop here forever.
    */
    pc = ctxtp->Eip;
    bp = (u_i4      **) ctxtp->Ebp;

    TRformat(output_fcn,output_arg,buf,sizeof(buf)-1,"-----------------------------------\n");
    tfstatus = SymGetSymFromAddr( hCurProc, (DWORD)ctxtp->Eip,
	                          &displacement, &xsymbol.symbol );
    if ( tfstatus == TRUE )
    {
	STprintf( funcstr, "%s+%x (%08x)", xsymbol.symbol.Name, displacement, ctxtp->Eip );
    }
    else
    {
	STprintf( funcstr, "%08x", ctxtp->Eip );
    }

    TRformat(output_fcn,output_arg,buf,sizeof(buf)-1,"Stack trace beginning at %s\n", funcstr);

    cp = (unsigned char *) bp[1];
    while ((stack_depth-- > 0) && bp && bp[1])
    {
 
	     retinst = *((unsigned short *) bp[1]);
	     if ( retinst == 0xC483 )
	     {
		cp = (unsigned char *) bp[1];
		parmcount = * ( cp + 2 ) / sizeof(unsigned char *);
	     }
	     else
	     {
		parmcount = 0;
	     }
	     parmbuf[0] = EOS;
	     parmlen = 0;
	     for ( i = 0; i < parmcount; i++ )
	     {
		STprintf( numbuf, " %08x", bp[2+i] );
		STcat( parmbuf, numbuf );
		/* check that we have room in the buffer for another parm */
		parmlen += 9;
		if ( parmlen >= sizeof(parmbuf)-10 )
		    break;
	     }
             if ( *(cp - 5) == 0xE8 )
             {
	        funcoffset = *((u_i4     *)(cp - 4));
		funcentry = (u_i4      *) (cp + funcoffset);
		if ( *((unsigned short *)funcentry) == 0x25FF ) /* in DLL */
		{
		    funcentry = (u_i4      *) (((char *)funcentry) + sizeof(unsigned short));
		    funcentry = *((u_i4      **) *funcentry);
		}
                tfstatus = SymGetSymFromAddr( hCurProc, (DWORD)funcentry,
			                      &displacement, &xsymbol.symbol );
		if ( tfstatus == TRUE )
		    STprintf( funcstr, "%s/%08x", xsymbol.symbol.Name, funcentry );
		else
		    STprintf( funcstr, "%08x", funcentry );
		baseaddr = SymGetModuleBase( hCurProc, (DWORD)funcentry );
                tfstatus = SymGetModuleInfo( hCurProc, (DWORD)funcentry, &module );
                if ( tfstatus == TRUE )
		    STprintf( dllstr, "(%s,Base:%x)", module.ModuleName, baseaddr );
		else
		    dllstr[0] = EOS;
	     }
	     else
	     {
		dllstr[0] = EOS;
		STprintf( funcstr, "????????" );
	     }
             tfstatus = SymGetSymFromAddr( hCurProc, (DWORD)bp[1],
 		                           &displacement, &xsymbol.symbol );
	     if ( tfstatus == TRUE )
		STcopy( xsymbol.symbol.Name, curstr );
	     else
		STprintf( curstr, "%08x", bp[1] );
	     TRformat( output_fcn, output_arg, buf, sizeof(buf) - 1,
		       "%s%s: %s%s(%s )\n",
		       verbose_string,
                       curstr, dllstr, funcstr, parmbuf );
 
             bp = (u_i4 **) bp[0];
    }

    if (havesymbols)
	SymCleanup( hCurProc );

# endif /* NT_INTEL */
    return;
}

#if 0
static void
dump_extended( CS_SCB *scb, CONTEXT *ctxtp )
{
    HANDLE     hCurProc;
    STACKFRAME stackframe;
    bool       tfstatus;
    DWORD      status;

    tfstatus = StackWalk( IMAGE_FILE_MACHINE_I386,
	                  hCurProc,
                          scb->cs_thread_handle,
			  &stackframe,
			  ctxtp,
			  NULL,
			  SymFunctionTableAccess,
			  SymGetModuleBase,
			  NULL );
    if ( tfstatus != TRUE )
    {
	status = GetLastError();
    }
    else
    {
	;
    }
}
#endif


/*{
** Name: CS_checktime()
**
** Description:
**	Returns current time.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
**	Returns:
**	    i4
**
** History:
**	10-jan-2000 (somsa01)
**	    Created.
*/
i4
CS_checktime()
{
    HRSYSTIME hrtime;

    TMhrnow(&hrtime);
    return (hrtime.tv_sec * 1000) + (hrtime.tv_nsec / NANO_PER_MILLI);
}

/*{
** Name: CSdump_stack()       - dump out stack trace.
**
** Description:
**    Externally visible way to get a stack trace
**    of the caller's session.
**
** Inputs:
**    none
**
** Outputs:
**    none.
**
**    Returns:
**        VOID
**
** History:
**    25-Apr-2003 (jenjo02) SIR 111028
**        Invented
*/
VOID
CSdump_stack( void )
{
    /* Dump this thread's stack, non-verbose */
    CS_dump_stack(NULL, NULL, NULL, NULL,0);
}

/*{
** Name:	CSMT_get_thread_stats
**
** Description:
**	Obtain the stats for the thread identified in the 
**	supplied parameter block. 
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	pTSCB		Pointer to CS_TSCB parameter block
**	    pTSCB->cs_thread_handle - handle of thread 
**
** Outputs:
**	    ** If CS_THREAD_STATS_CPU is set in pTSCB->cs_stats_flag:
**	    pTSCB->cs_usercpu - user cpu used by thread
**	    pTSCB->cs_systemcpu - system cpu used by thread
**
** Returns:
** 	FAIL or OK as appropriate.
**
** History:
**	09-Feb-2009 (smeke01) b119586
**	    Copied mutatis mutandis from Linux/Solaris version.
*/
STATUS
CSMT_get_thread_stats(CS_THREAD_STATS_CB *pTSCB)
{
	FILETIME	kerneltime;
	FILETIME	usertime;
	FILETIME	dummy;
	i4     		kernelcpu;
	i4     		usercpu;

	/*
	** Check that we're being asked to do something
	** we know how to do. At the time of writing we
	** only know how to do thread cpu stats.
	*/
	if (pTSCB->cs_stats_flag != CS_THREAD_STATS_CPU)
	{
	    TRdisplay("%@ CSMT_get_thread_stats(): unknown stats request via flag value:%x\n", pTSCB->cs_stats_flag);
	    return FAIL;
	}

	if (! GetThreadTimes(pTSCB->cs_thread_handle,
			&dummy, &dummy,
			&kerneltime, &usertime) )
	{
	    DWORD error = GetLastError();
	    TRdisplay("%@ CSMT_get_thread_stats(): GetThreadTimes() failed. GetLastError() returned (%d)\n", error);
	    return FAIL;
	}

	usercpu = usertime.dwHighDateTime * SECONDS_PER_DWORD * MILLI;
	usercpu += usertime.dwLowDateTime / HNANO_PER_MILLI;

	kernelcpu = kerneltime.dwHighDateTime * SECONDS_PER_DWORD * MILLI;
	kernelcpu += kerneltime.dwLowDateTime / HNANO_PER_MILLI;

	pTSCB->cs_usercpu = usercpu;
	pTSCB->cs_systemcpu = kernelcpu;

	return OK;
}

