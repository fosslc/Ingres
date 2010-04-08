/*
**Copyright (c) 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <er.h>
#include    <ex.h>
#include    <pc.h>
#include    <sl.h>
#include    <nm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <lk.h>
#include    <cx.h>
#include    <lkdef.h>
#include    <lgkdef.h>
#include    <st.h>
#include    <tr.h>
/***** FOLLOWING LINES FOR TRACING LK MUTEX/UNMUTEX PROBLEMS: *****/
#ifdef LK_MUTEX_TRACE
#include    <cv.h>
#include    <me.h>
#include    <tm.h>
typedef struct
{
    char	semname[CS_SEM_NAME_LEN];	/* Name of semaphore */
    TM_STAMP	timestamp;	/* Time of mutex/unmutex call */
    i4	sem_pid;	/* Process ID */
    CS_SID	sem_sess;	/* Session ID (from linked scb) */
#if defined(VMS) && defined(ALPHA)
    i4	sem_test_set;	/* Interlocked test/set field for MULTIs */
#endif
    i2		sem_count;	/* Sem count */
    i2		sem_value;	/* Sem value */
    char	sem_type;	/* Sem type (SINGLE/MULTI) */
    char	call_type;	/* LK Mutex/unmutex call type mask: */
#define LKMUTEX_TRACE_MUTEX 0x01	/* 1=Mutex call; 0=Unmutex call */
#define LKMUTEX_TRACE_AFTER 0x02	/* 1=After CSp/v call; 0=Before call */
#define LKMUTEX_TRACE_EXCL  0x04	/* 1=Exclusive mode; 0=Shared or n/a */
#define LKMUTEX_TRACE_DUMP  0x08	/* 1=Entry has dump of preceding area*/
#define LKMUTEX_TRACE_PRINT 0x80	/* Print traces after error */
    union
    {
	char	pre_dump[12];	/* Dump of 12 bytes preceding semaphore */
	struct	{
	    long  pre_dump_l1;
	    long  pre_dump_l2;
	    long  pre_dump_l3;
		} pdl;
    } pd;
} LKMUTEX_TRACE_ENTRY;
static i4  LKmutex_trace_sw = 0; /* Controls activation/use of LK Mutex trace:
                                 **  =0: never checked logical;
                                 **  <0: logical missing, tracing off;
                                 **  >0: logical found, tracing on;
				 **  incremented after trace print, up to 5.
                                 */
/* Size of circular LK Mutex trace stack; from logical or by default: */
static i4  LKmutex_trace_size;
/* Current LK Mutex trace stack entry (1 - LKmutex_trace_size): */
static i4  lkmt_entry_num;
/* Top and Current LK Mutex trace stack entry pointers: */
static LKMUTEX_TRACE_ENTRY  *lkmt_top_p, *lkmt_entry_p;
/* LK Mutex tracing function: */
static VOID LKmutex_trace(CS_SEMAPHORE*, i4, char);
#endif

/**
**
**  Name: LKMUTEX.C - Implements the mutex functions of the logging system
**
**  Description:
**	This module contains the code which implements the mutual exclusion
**	primitives used by LK.
**	
**	    LK_imutex  -- Initialize the LKD mutex
**	    LK_mutex   -- lock down the LKD and all its sub-structures.
**	    LK_unmutex -- unlock the LK_mutex lock.
**	    LK_get_critical_sems -- get all the semaphores
**	    LK_rel_critical_sems -- release them
**
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	13-oct-1992 (bryanp)
**	    Temporarily remove the EXinterrupt() calls since they goof up VMS.
**	    In particular, EXinterrrupt disables AST's, and that causes all
**	    sorts of grief (for example, CSp_semaphore doesn't work right).
**	23-Oct-1992 (daveb)
**	    name the semaphore too.
**	16-feb-1993 (bryanp)
**	    6.5 Cluster Project: experiment with disabling AST's during LK code.
**	24-may-1993 (bryanp)
**	    Only the CSP needs to disable AST's. Other processes need not.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	23-may-1994 (bryanp) B58498
**	    After releasing mutex, check to see if an interrupt was
**		received; if so, exit gracefully.
**	17-nov-1994 (medji01)
**	    Mutex Granularity Project
**		- removed unused sys_err parm from function definitions
**		- added mutex ptr field to function definitions, this
**		  will be used in CSx_semaphore calls in place of the 
**		  hardcoded reference to the LKD mutex.
**		- moved LKD ptr initialization to VMS-dependent code
**		- added mutex name parm to LK_imutex
**		- removed LKD ptr initialization where unreferenced
**	26-Apr-1995 (jenjo02)
**	    Combined CSi|n_semaphore functions calls into single
**	    CSw_semaphore call.
**	06-Dec-1995 (jenjo02)
**	    Added CSa_semaphore() to LK_imutex().
**	    Removed most of non-platform-dependent semaphore
**	    debugging code.
**	19-Dec-1995 (jenjo02)
**	    Added new parm to LK_mutex(), the mode in which the
**	    semaphore is to be acquired, SHARE, or EXCL.
**	14-mar-96 (nick)
**	    Add LK_get_critical_sems() and LK_rel_critical_sems().
**	28-Mar-1996 (jenjo02)
**	    Revised Nick's list of which semaphores to acquire in
**	    LK_get|rel_critical_sems().
**	14-May-1996 (jenjo02)
**	    Added TRdisplay to identify CSp|v_semaphore() failure.
**	21-aug-1996 (boama01)
**	    Changes to correct probs with semaphores on Alpha/VMS:
**	    - Chgd LK_mutex() to disable ASTs even when this is NOT the CSP;
**	      the CSP is protected since all lock rqsts are completed under
**	      the AST-fired LK_mbx_complete() fn, but other servers (DBMS, RCP)
**	      are NOT protected from interruption and need this to prevent use
**	      of LKD areas while the mutex is held (single thread LKD mutexes).
**	    - Chgd LK_unmutex() similarly, and to log errors before re-enabling
**	      ASTs.  Hopefully all this will prevent DMA00A errors on slower
**	      VMS machines.
**	    - Added extra log messaging for details of semaphores that fail.
**	    - Added LK_mutex/unmutex() trace facility to further help diagnose
**	      mutex errors (CL250A, DMA00A). These under control of the
**	      LK_MUTEX_TRACE preproc symbol (see generic!back!dmf!hdr LKDEF.H).
**	    - Added new cs_test_set fld in Alpha semaphores to mssgs, traces.
**	08-Oct_1996 (jenjo02)
**	    In LK_get_critical_sems/LK_rel_critical_sems, call CSp|v_semaphore
**	    directly instead of using LK_mutex|unmutex to avoid cluttering
**	    the log with misleading semaphore errors.
**	10-Apr-1997 (jenjo02)
**	    Fixed typo in TRdisplay, should be CSv_sem... not CSp_sem ...
**	    Removed LK_rmutex(), no longer used.
**	02-Feb-2000 (jenjo02)
**	    Inline CSp|v_semaphore calls. LK_mutex, LK_unmutex are
**	    now macros calling LK_mutex_fcn(), LK_unmutex_fcn() iff
**	    a semaphore error occurs.
**	    Added mutex name, module name and line number to DMA009, DMA00A
**	    messages to aid debugging.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-apr-2001 (devjo01)
**	    s103715 (Portable cluster support)
**	24-oct-2003 (devjo01)
**	    with the Portable cluster project LK_disable_ASTs and 
**	    LK_enable_ASTS are no longer applicable
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
*/

static i4	LK_interrupt_received = 0;

/*{
** Name: LK_mutex_fcn()	- Get mutex on LK objects.
**
** Description:
**      Get mutex on LK objects.
**
** Inputs:
**	mode				SEM_SHARE or
**					SEM_EXCL
**	lmutex				LK_SEMAPHORE to latch in that mode
**	file				name of source module making the
**					call
**	line				line number within that module
**
** Outputs:
**	none.
**
**	Returns:
**	    OK				- success
**	    other non-zero return	- failure.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	16-feb-1993 (bryanp)
**	    6.5 Cluster Project: experiment with disabling AST's during LK code.
**	24-may-1993 (bryanp)
**	    Only the CSP needs to disable AST's. Other processes need not.
**	    Furthermore, the CSP only needs to actively disable AST's when it
**	    is calling LK routines at non-AST level. When it is already running
**	    at AST level the setast calls aren't needed.
**	17-nov-1994 (medji01)
**	    Mutex Granularity Project
**		- removed sys_err parm
**		- added mutex pointer parm
**		- moved LKD ptr initialization to VMS-dependent code
**	19-Dec-1995 (jenjo02)
**	    Added "mode", share or exclusive.
**	14-May-1996 (jenjo02)
**	    Added TRdisplay to identify CSp|v_semaphore() failure.
**	21-aug-1996 (boama01)
**	    Chgd LK_mutex() to disable ASTs even when this is NOT the CSP
**	      on a VMS machine; the CSP is protected since all lock rqsts are
**	      completed under the AST-fired LK_mbx_complete() fn, but other
**	      servers (DBMS, RCP) are NOT protected from interruption and need
**	      this to prevent use of LKD areas while the mutex is held
**	      (single thread LKD mutexes).
**	    Also chgd to re-enable ASTs if a CSp_semaphore() error occurs.
**	    Also added extra log messaging for internals of semaphore that
**	      failed (directly accessed).
**	    Also added calls to LKmutex_trace() when tracing symbol is defined.
**	02-Feb-2000 (jenjo02)
**	    Renamed from LK_mutex, added "file" and "line" parms.
**	    Modified E_DMA009 to show mutex name, file, and line,
**	    removed now-redundant TRdisplays.
**	02-Mar-2007 (jonj)
**	    Don't dereference cs_owner in "extra messaging";
**	    it may well be a session in another process.
**	14-Nov-2007 (jonj)
**	    Don't know why, exactly, but ule_format() in the 
**	    "extra" VMS stuff sometimes causes an ACCVIO, use
**	    ERlog instead.
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
**	08-Jan-2008 (jonj)
**	    Changed prototype to pass pointer to LK_SEMAPHORE instead of
**	    CS_SEMAPHORE. When LK_MUTEX_DEBUG is defined in lkdef.h,
**	    then LK_SEMAPHORE will be a structure containing a CS_SEMAPHORE,
**	    file/line of last successful mutex/unmutex.
**	    
*/
STATUS
LK_mutex_fcn(i4 mode,
	LK_SEMAPHORE *lmutex, 
	char	*filename,
	i4 	 linenum) 
{
    CL_ERR_DESC	sys_err;
    char 	xtra_msg[256];
    STATUS	status;
    CS_SEMAPHORE *amutex;

#ifdef LK_MUTEX_DEBUG
    amutex = &lmutex->mutex;
#else
    amutex = lmutex;
#endif

#ifdef LGLK_NO_INTERRUPTS
    EXinterrupt(EX_OFF);
#endif


#ifdef LK_MUTEX_TRACE
    LKmutex_trace(amutex, mode, (char)LKMUTEX_TRACE_MUTEX);
#endif
    status = CSp_semaphore(mode, amutex);
#ifdef LK_MUTEX_TRACE
    LKmutex_trace(amutex, mode,
	(char)(LKMUTEX_TRACE_MUTEX | LKMUTEX_TRACE_AFTER));
#endif

    if (status)
    {
	i4	err_code;
	CS_SEM_STATS sstat;

#ifdef LGLK_NO_INTERRUPTS
	EXinterrupt(EX_ON);
#endif

	/* Try to get the name of the failed semaphore */
	if ( CSs_semaphore(CS_INIT_SEM_STATS, amutex, &sstat, sizeof(sstat)) )
	    STprintf(sstat.name, "-unknown-");

	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);

	uleFormat(NULL, E_DMA009_LK_MUTEX, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
		    0, sstat.name,
		    0, filename,
		    0, linenum);

#ifdef LK_MUTEX_DEBUG
	STprintf(xtra_msg, "Last mutexed by %s:%d\n",
		    lmutex->file, lmutex->line);
	ERlog(xtra_msg, STlength(xtra_msg), &sys_err);
#endif

#if defined(VMS) && defined(ALPHA)
	/* Extra messaging of semaphore content for problem analysis */
	STprintf(xtra_msg,
    "Semaphore: \"%.*s\"   Value: %d   Count: %d  session: %x   pid: %x\
Test_set: %d\n",
	  CS_SEM_NAME_LEN, amutex->cs_sem_name,
	  amutex->cs_value,
	  amutex->cs_count,
	  amutex->cs_owner,
	  amutex->cs_pid,
	  amutex->cs_test_set);
	ERlog(xtra_msg, STlength(xtra_msg), &sys_err);
#endif
#ifdef LK_MUTEX_TRACE
	/*
	** If a Mutex trace being done, print out the trace entries up to
	** the point of error.
	*/
	LKmutex_trace(amutex, mode, (char)LKMUTEX_TRACE_PRINT);
#endif
    }
#ifdef LK_MUTEX_DEBUG
    else
    {
	/* Record source of last successful LK_mutex */
        lmutex->file = filename;
	lmutex->line = linenum;
    }
#endif

    return(status);
}

/*{
** Name: LK_unmutex_fcn()	- release an LK mutex.
**
** Description:
**	Release an LK mutex.
**
** Inputs:
**	amutex				semaphore to release
**	file				name of source module making the
**					call
**	line				line number within that module
**
** Outputs:
**	none.
**
**	Returns:
**	    OK		success
**	    !OK		fail
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	16-feb-1993 (bryanp)
**	    6.5 Cluster Project: experiment with disabling AST's during LK code.
**	24-may-1993 (bryanp)
**	    Only the CSP needs to disable AST's. Other processes need not.
**	23-may-1994 (bryanp) B58498
**	    After releasing mutex, check to see if an interrupt was
**		received; if so, exit gracefully.
**	17-nov-1994 (medji01)
**	    Mutex Granularity Project
**		- removed sys_err parm
**		- added mutex pointer parm
**		- moved LKD ptr initialization to VMS-dependent code
**	14-May-1996 (jenjo02)
**	    Added TRdisplay to identify CSp|v_semaphore() failure.
**	21-aug-1996 (boama01)
**	    Chgd LK_unmutex() to re-enable ASTs even when this is NOT the CSP
**	      on a VMS machine; see LK_mutex() above for details.
**	    Also moved logging of errors directly after CSv_semaphore() call,
**	      and BEFORE LK_enable_ast() call.
**	    Also added extra log messaging for internals of semaphore that
**	      failed (directly accessed).
**	    Added calls to LKmutex_trace() when tracing symbol is defined.
**	10-Apr-1997 (jenjo02)
**	    Fixed typo in TRdisplay, should be CSv_sem... not CSp_sem ...
**	02-Feb-2000 (jenjo02)
**	    Renamed from LK_unmutex, added "file" and "line" parms.
**	    Modified E_DMA00A to show mutex name, file, and line,
**	    removed now-redundant TRdisplays.
**	02-Mar-2007 (jonj)
**	    Don't dereference cs_owner in "extra messaging";
**	    it may well be a session in another process.
**	14-Nov-2007 (jonj)
**	    Don't know why, exactly, but uleFormat(NULL, ) in the 
**	    "extra" VMS stuff sometimes causes an ACCVIO, use
**	    ERlog instead.
**	08-Jan-2008 (jonj)
**	    Changed prototype to pass pointer to LK_SEMAPHORE instead of
**	    CS_SEMAPHORE. When LK_MUTEX_DEBUG is defined in lkdef.h,
**	    then LK_SEMAPHORE will be a structure containing a CS_SEMAPHORE,
**	    file/line of last successful mutex/unmutex.
*/
STATUS
LK_unmutex_fcn(LK_SEMAPHORE *lmutex,
	char	*filename,
	i4	linenum)
{
    CL_ERR_DESC	sys_err;
    char 	xtra_msg[100];
    STATUS      status;
    CS_SEMAPHORE *amutex;

#ifdef LK_MUTEX_DEBUG
    amutex = &lmutex->mutex;
#else
    amutex = lmutex;
#endif

#ifdef LK_MUTEX_TRACE
    LKmutex_trace(amutex, (i4)0, (char)0);
#endif
    status = CSv_semaphore(amutex);
#ifdef LK_MUTEX_TRACE
    LKmutex_trace(amutex, (i4)0, (char)LKMUTEX_TRACE_AFTER);
#endif

    if (status)
    {
	i4	err_code;
	CS_SEM_STATS sstat;

	/* Try to get the name of the failed semaphore */
	if ( CSs_semaphore(CS_INIT_SEM_STATS, amutex, &sstat, sizeof(sstat)) )
	    STprintf(sstat.name, "-unknown-");
	
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);

	uleFormat(NULL, E_DMA00A_LK_UNMUTEX, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
		    0, sstat.name,
		    0, filename,
		    0, linenum);

#ifdef LK_MUTEX_DEBUG
	STprintf(xtra_msg, "Last unmutexed by %s:%d\n",
		    lmutex->file, lmutex->line);
	ERlog(xtra_msg, STlength(xtra_msg), &sys_err);
#endif

#if defined(VMS) && defined(ALPHA)
	/* Extra messaging of semaphore content for problem analysis */
	{
	    STprintf(xtra_msg,
	"Semaphore: \"%.*s\"   Value: %d   Count: %d  session: %x   pid: %x\
  Test_set: %d\n",
	      CS_SEM_NAME_LEN, amutex->cs_sem_name,
	      amutex->cs_value,
	      amutex->cs_count,
	      amutex->cs_owner,
	      amutex->cs_pid,
	      amutex->cs_test_set);
	    ERlog(xtra_msg, STlength(xtra_msg), &sys_err);
	}
#endif
#ifdef LK_MUTEX_TRACE
	/*
	** If a Mutex trace being done, print out the trace entries up to
	** the point of error.
	*/
	LKmutex_trace(amutex, (i4)0, (char)LKMUTEX_TRACE_PRINT);
#endif
    }
#ifdef LK_MUTEX_DEBUG
    else
    {
	/* Record source of last successful LK_unmutex */
        lmutex->file = filename;
	lmutex->line = linenum;
    }
#endif

#ifdef LGLK_NO_INTERRUPTS
    EXinterrupt(EX_ON);
#endif


    if (LK_interrupt_received)
    {
	LKintrack();
	LGintrack();
	PCexit(FAIL);
    }

    return(status);
}

/*{
** Name: LK_imutex()	- Initialize the LK mutex.
**
** Description:
**	Release LK mutex.  Currently LK uses a semaphore called the
**	LKD mutex shared by all LK code.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**	    OK		success
**	    !OK		fail
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	23-Oct-1992 (daveb)
**	    name the semaphore too.
**	17-nov-1994 (medji01)
**	    Mutex Granularity Project
**		- removed sys_err parm
**		- added mutex pointer and mutex name parms
**		- removed LKD ptr initialization
**	26-Apr-1995 (jenjo02)
**	    Combined CSi|n_semaphore functions calls into single
**	    CSw_semaphore call.
**	06-Dec-1995 (jenjo02)
**	    Added CSa_semaphore() to LK_imutex().
**	08-Jan-2008 (jonj)
**	    Changed prototype to pass pointer to LK_SEMAPHORE instead of
**	    CS_SEMAPHORE. When LK_MUTEX_DEBUG is defined in lkdef.h,
**	    then LK_SEMAPHORE will be a structure containing a CS_SEMAPHORE,
**	    file/line of last successful mutex/unmutex.
*/
STATUS
LK_imutex(LK_SEMAPHORE *lmutex, char *mutex_name)
{
    STATUS      status;
    i4	err_code;
    CS_SEMAPHORE *amutex;

#ifdef LK_MUTEX_DEBUG
    amutex = &lmutex->mutex;
    lmutex->file = NULL;
    lmutex->line = 0;
#else
    amutex = lmutex;
#endif

    status = CSw_semaphore(amutex, CS_SEM_MULTI, mutex_name);

    if (status)
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	uleFormat(NULL, E_DMA00B_LK_IMUTEX, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
    }
    else
    {
	(void) CSa_semaphore(amutex);
    }
    return(status);
}

void
LKintrcvd(void)
{
    LK_interrupt_received = 1;
}
void
LKintrack(void)
{
    LK_interrupt_received = 0;
}

/*{
** Name: LK_get_critical_sems()	- obtain ALL the LK semaphores 
**
** Description:
**	Obtain all the necessary LK semaphores to ensure nothing else
**	can touch LK.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**	    OK		success
**	    !OK		fail
**
** History:
**	14-mar-96 (nick)
**	    Created.
**	28-Mar-1996 (jenjo02)
**	    Changed Nick's semaphore list in LK_get|rel_critical_sems()
**	    to a more protective selection.
**	08-Oct_1996 (jenjo02)
**	    In LK_get_critical_sems/LK_rel_critical_sems, call CSp|v_semaphore
**	    directly instead of using LK_mutex|unmutex to avoid cluttering
**	    the log with misleading semaphore errors.
**	07-Jun-2007 (jonj)
**	    Get all RSB mutexes, too.
**	14-Nov-2007 (jonj)
**	    Ooh, bad idea, sometimes causes mutex deadlocks during recovery.
*/
STATUS
LK_get_critical_sems(void)
{
    STATUS	status;
    LKD		*lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    SIZE_TYPE	*lbk_table;
    SIZE_TYPE	*rbk_table;
    LLB		*llb;
    RSB		*rsb;
    i4		i;

    /*
    ** Lock lkd_lbk_mutex to prevent LLBs from
    ** being added or removed.
    */
#ifdef LK_MUTEX_DEBUG
    (VOID)CSp_semaphore(SEM_EXCL, &lkd->lkd_lbk_mutex.mutex);
#else
    (VOID)CSp_semaphore(SEM_EXCL, &lkd->lkd_lbk_mutex);
#endif

    /*
    ** Lock every LLB, ignore p_sem errors.
    */
    lbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_lbk_table);
    for (i = 1; i <= lkd->lkd_lbk_count; i++)
    {
	llb = (LLB *)LGK_PTR_FROM_OFFSET(lbk_table[i]);

	if (llb->llb_type == LLB_TYPE)
#ifdef LK_MUTEX_DEBUG
	    (VOID)CSp_semaphore(SEM_EXCL, &llb->llb_mutex.mutex);
#else
	    (VOID)CSp_semaphore(SEM_EXCL, &llb->llb_mutex);
#endif
    }

    return(OK);
}
/*{
** Name: LK_rel_critical_sems()	- release all LK semaphores
**
** Description:
**	Release all semaphores obtained by LK_get_critical_sems().
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**	    OK		success
**	    !OK		fail
**
** History:
**	14-mar-96 (nick)
**	    Created.
**	28-Mar-1996 (jenjo02)
**	    Changed Nick's semaphore list in LK_get|rel_critical_sems()
**	    to a more protective selection.
**	08-Oct_1996 (jenjo02)
**	    In LK_get_critical_sems/LK_rel_critical_sems, call CSp|v_semaphore
**	    directly instead of using LK_mutex|unmutex to avoid cluttering
**	    the log with misleading semaphore errors.
**	07-Jun-2007 (jonj)
**	    Release all RSB mutexes, too.
**	14-Nov-2007 (jonj)
**	    Ooh, bad idea, sometimes causes mutex deadlocks during recovery.
*/
STATUS
LK_rel_critical_sems(void)
{
    STATUS	status = OK;
    LKD		*lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    SIZE_TYPE	*lbk_table;
    SIZE_TYPE	*rbk_table;
    LLB		*llb;
    RSB		*rsb;
    i4		i;

    /*
    ** Unlock every LLB
    */
    lbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_lbk_table);
    for (i = 1; i <= lkd->lkd_lbk_count; i++)
    {
	llb = (LLB *)LGK_PTR_FROM_OFFSET(lbk_table[i]);

	if (llb->llb_type == LLB_TYPE)
#ifdef LK_MUTEX_DEBUG
	    (VOID)CSv_semaphore(&llb->llb_mutex.mutex);
#else
	    (VOID)CSv_semaphore(&llb->llb_mutex);
#endif
    }

    /*
    ** Unlock lkd_lbk_mutex.
    */
#ifdef LK_MUTEX_DEBUG
    (VOID)CSv_semaphore(&lkd->lkd_lbk_mutex.mutex);
#else
    (VOID)CSv_semaphore(&lkd->lkd_lbk_mutex);
#endif
    return(OK);
}

#ifdef LK_MUTEX_TRACE
/*{
** Name: LKmutex_trace()	- Trace mutex/unmutex calls and results.
**
** Description:
**      This routine maintains a circular trace queue of entries in which
**	LK_mutex() and LK_unmutex() calls are traced.  The routine is
**	ordinarily called to write a trace entry to the queue, but can also be
**	called to dump the queue to the Ingres error log (see below).
**
**	Note that this routine (and calls to it) will only be compiled if the
**	LK_MUTEX_TRACE switch is defined in generic!back!dmf!hdr LKDEF.H.
**	In addition, tracing will only occur if the II_LKMUTEX_TRACE logical
**	is defined at runtime.  If the value of this logical is a number
**	between 6 and 1000, this value is used as the number of entries
**	allocated in the queue.  Otherwise, a default number of entries is used.
**
**	A series of static variables are used to maintain trace status,
**	queue position, etc.  When the trace is first activated, the circular
**	trace queue is allocated dynamically.  Since these are all static
**	elements, a different queue is maintained for each server with which
**	these LK routines are linked (e.g., DBMS, RCP, etc.).  When printed,
**	the trace output is identified by process ID, and is relevant only
**	for the server executing in that process.
**
** Inputs:
**	amutex				semaphore to trace
**	mode				SEM_SHARE or SEM_EXCL on mutex call;
**					 ignored on an unmutex call
**	trace_type			character bit flag, identifying
**					 mutex/unmutex, before/after
**					 CSx_semaphore() call, or special
**					 trace print function
**
** Outputs:
**	none.
**
**	Returns:
**	    None.
**
** History:
**	21-aug-1996 (boama01)
**	    Created for tracing CL250A and DMA00A problems. Includes new
**	    cs_test_set fld in Alpha semaphores.
*/
static VOID
LKmutex_trace(CS_SEMAPHORE *mutex,
		i4 mode,
		char trace_type)
{

    /* Immediately check to see if tracing is disabled; if so, exit */
    if (LKmutex_trace_sw < 0)
	return;
    /* Also check to see if too many trace prints caused its disablement */
    if (LKmutex_trace_sw > 5)
	return;

    /* If a trace print request, send trace entry contents to error log */
    if (trace_type & (char)LKMUTEX_TRACE_PRINT)
    {
	char	line[133], fmtd_stamp[TM_SIZE_STAMP];
	TM_STAMP	time_now;
	CL_ERR_DESC	sys_err;
	i4	i, loopback, error;
	LKMUTEX_TRACE_ENTRY	*entry;

	/*
	** Print a header identifying the process whose queue is being
	** printed.  Use uleFormat(NULL, ) here to get node/server/session/time
	** header info in message, but use ERlog() in subsequent lines
	** to omit this info and keep line short.
	*/
	TMget_stamp(&time_now);
	TMstamp_str(&time_now, fmtd_stamp);
	STprintf(line, "%s  LKMutex Trace follows:", fmtd_stamp);
	uleFormat(NULL, 0, (CL_ERR_DESC *)NULL, ULE_MESSAGE, NULL,
	    line, STlength(line), (i4 *)NULL, &error, 0);
	STprintf(line,
"LK MUTEX/UNMUTEX TRACE CONTENTS FOR PROCESS %x FROM BAD STATUS",
	    LGK_my_pid);
	ERlog(line, STlength(line), &sys_err);
	STprintf(line,
#if defined(VMS) && defined(ALPHA)
"Seq.  %-*s  Call       CS* call  %-20s  LKtype  SemType  Process   Session\
   Count  Value  Tst/Set",
#else
"Seq.  %-*s  Call       CS* call  %-20s  LKtype  SemType  Process   Session\
   Count  Value",
#endif
	    TM_SIZE_STAMP, "Timestamp", "Semaphore");
	ERlog(line, STlength(line), &sys_err);

	/*
	** If no trace entries ever written to queue, the first entry still has
	** its pre-initialized value.  If so, print a message and exit.
	*/
	if (!(LKmutex_trace_sw) || !(lkmt_top_p->semname[0]))
	{
	    STprintf(line,
		"** NO TRACE ENTRIES WRITTEN IN INTERNAL TRACE QUEUE **");
	    ERlog(line, STlength(line), &sys_err);
	    return;
	}

	/*
	** Print each entry, starting w. oldest and wrapping around to latest.
	** If we never wrapped around the queue, the "next" queue entry still
	** has its pre-initialized value.  If so, simply print from the first
	** entry through the last one written.
	*/
	if (!(lkmt_entry_p->semname[0]))
	{
	    entry = lkmt_top_p;
	    loopback = 1;
	}
	else
	{
	    entry = lkmt_entry_p;
	    loopback = 0;
	}
	i = lkmt_entry_num;
	while (!((entry == lkmt_entry_p) && (loopback)))
	{
	    TMstamp_str(&entry->timestamp, fmtd_stamp);
	    STprintf(line,
#if defined(VMS) && defined(ALPHA)
		"%4d  %-*s  %s  %s  %-20s  %s  %s  %8x  %8x  %5d  %5d  %5d",
#else
		"%4d  %-*s  %s  %s  %-20s  %s  %s  %8x  %8x  %5d  %5d",
#endif
		i, TM_SIZE_STAMP, fmtd_stamp,
		(entry->call_type & (char)LKMUTEX_TRACE_MUTEX)
		    ? "LKmutex  " : "LKunmutex",
		(entry->call_type & (char)LKMUTEX_TRACE_AFTER)
		    ? "After   " : "Before  ",
		entry->semname,
		(entry->call_type & (char)LKMUTEX_TRACE_EXCL)
		    ? " EXCL " : "  -   ",
		(entry->sem_type) ? "MULTI  " : "SINGLE ",
		entry->sem_pid, entry->sem_sess,
#if defined(VMS) && defined(ALPHA)
		entry->sem_count, entry->sem_value, entry->sem_test_set);
#else
		entry->sem_count, entry->sem_value);
#endif
	    ERlog(line, STlength(line), &sys_err);
	    if (lkmt_entry_p->call_type & (char)LKMUTEX_TRACE_DUMP)
	    {
		STprintf(line,
		"                   12 bytes preceding semaphore: %8x %8x %8x",
		    lkmt_entry_p->pd.pdl.pre_dump_l1,
		    lkmt_entry_p->pd.pdl.pre_dump_l2,
		    lkmt_entry_p->pd.pdl.pre_dump_l3);
		ERlog(line, STlength(line), &sys_err);
	    }
	    if (i == LKmutex_trace_size)
	    {
		entry = lkmt_top_p;
		i = loopback = 1;
	    }
	    else
	    {
		entry++;
		i++;
	    }
	} /* end while */

	/*
	** Tally the number of trace prints produced; if 5 have already been
	** printed, disable all tracing from here on.  This limits the
	** instances of "runaway" tracing from recursive mutex errors that may
	** be encountered by server recovery or rundown.
	*/
	LKmutex_trace_sw++;
	if (LKmutex_trace_sw > 5)
	{
	    STprintf(line,
"**** LK MUTEX/UNMUTEX TRACING HAS BEEN DISABLED AFTER 5 PRINTS! ****");
	    ERlog(line, STlength(line), &sys_err);
	    /* Might as well release the trace entry memory as well */
	    (void)MEfree(lkmt_top_p);
	}

	return;
    }

    /* Here we are asked to trace; first see if tracing was ever activated */
    if (LKmutex_trace_sw == 0)
    {
	char	*trace_env;
	i4	trace_size;
	STATUS	status;
	LKD	*lkd = (LKD *)LGK_base.lgk_lkd_ptr;

	/*
	** ALWAYS disable tracing for CSP; it single-threads LK mutex calls
	** since it single-threads lock requests thru AST delivery, so we
	** never need to worry about CSP mutexes. (???)
	*/
	if (LGK_my_pid == lkd->lkd_csp_id)
	{
	    LKmutex_trace_sw = -1;
	    return;
	}

	/* To activate trace, check for environment variable/logical: */
	NMgtAt("II_LKMUTEX_TRACE", &trace_env);
	if (trace_env && *trace_env)
	{
	    /* Trace activates; see if user specified valid trace queue size */
	    status = CVan(trace_env, &trace_size);
	    if (!(status) && trace_size > 5 && trace_size < 1001)
		LKmutex_trace_size = trace_size;
	    else
		/* Invalid or missing size; use default queue size */
		LKmutex_trace_size = 300;

	    /* Allocate and init the trace queue in number of entries */
	    trace_size = LKmutex_trace_size * sizeof(LKMUTEX_TRACE_ENTRY);
	    lkmt_top_p = MEreqmem((u_i4)0, trace_size, TRUE, status);
	    if (status)
	    {
		i4 err_code;
		char line[100];
		STprintf(line,
"%@ LKmutex_trace: failed to allocate %d bytes for trace queue; status was:\n",
		    trace_size);
		uleFormat(NULL, 0, (CL_ERR_DESC *)NULL, ULE_MESSAGE, NULL,
		    line, STlength(line), (i4 *)NULL, &err_code, 0);
		uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
		/* Set switch to disable tracing */
		LKmutex_trace_sw = -1;
		return;
	    }

	    /* Initialize the queue ptr and seq.no. variables */
	    lkmt_entry_p = lkmt_top_p;
	    lkmt_entry_num = 1;
	    LKmutex_trace_sw = 1;
	}
	else
	{
	    /* Env. variable/logical not found; indicate no tracing and exit */
	    LKmutex_trace_sw = -1;
	    return;
	}
    } /* end if(trace_sw == 0) */

    /*
    ** Here we must write a new entry into the trace queue.  The pointer and
    ** sequence no. should already be set for the next (new) entry.
    */
    MEcopy(mutex->cs_sem_name, CS_SEM_NAME_LEN, lkmt_entry_p->semname);
    TMget_stamp(&lkmt_entry_p->timestamp);
    /*
    ** Note: it's possible to use semaphore contents to get flds from scb:
    **    lkmt_entry_p->sem_pid = mutex->cs_owner->cs_pid;
    **    lkmt_entry_p->sem_sess = mutex->cs_owner->cs_self;
    ** However, this REQUIRES that the sem not be touched between the
    ** CSx_semaphore() call and here, a condition that may not be guaranteed
    ** (trace is used to find sem update errors, y'know!).  So simply trace
    ** the contents of the sem without actually using them:
    */
    lkmt_entry_p->sem_pid = mutex->cs_pid;
    lkmt_entry_p->sem_sess = mutex->cs_owner;
    lkmt_entry_p->sem_count = mutex->cs_count;
    lkmt_entry_p->sem_value = mutex->cs_value;
#if defined(VMS) && defined(ALPHA)
    lkmt_entry_p->sem_test_set = mutex->cs_test_set;
#endif
    lkmt_entry_p->sem_type = mutex->cs_type;
    lkmt_entry_p->call_type = trace_type;
    if (mode == SEM_EXCL)
	lkmt_entry_p->call_type |= (char)LKMUTEX_TRACE_EXCL;
    /* Dump area preceding sem if a lbk, rbk or sbk mutex sem */
    if ((mutex->cs_sem_name[4] == 'b') && (mutex->cs_sem_name[5] == 'k')
      && ((mutex->cs_sem_name[3] == 'r') || (mutex->cs_sem_name[3] == 's')
      || (mutex->cs_sem_name[3] == 'l')))
    {
	lkmt_entry_p->call_type |= (char)LKMUTEX_TRACE_DUMP;
	MEcopy((char*)((char*)(mutex)-12), 12, lkmt_entry_p->pd.pre_dump);
    }

    /* Advance ptr/seq. no. to next entry in queue; may wrap around to top */
    if (lkmt_entry_num == LKmutex_trace_size)
    {
	lkmt_entry_p = lkmt_top_p;
	lkmt_entry_num = 1;
    }
    else
    {
	lkmt_entry_p++;
	lkmt_entry_num++;
    }

    return;
}
#endif
