/*
** Copyright (c) 1987, 2008 Ingres Corporation
** All Rights Reserved.
*/

# include <compat.h>
# ifdef OS_THREADS_USED
# include <gl.h>
# include <st.h>
# include <clconfig.h>
# include <systypes.h>
# include <fdset.h>
# include <clsigs.h>
# include <iicommon.h>
# include <cm.h>
# include <cs.h>
# include <er.h>
# include <ex.h>
# include <me.h>
# include <pc.h>
# include <lk.h>
# include <tm.h>
# include <tr.h>
# include <nm.h>
# include <me.h>
# include <si.h>
# include <cv.h>
# include <sp.h>
# include <mo.h>


# include <csinternal.h>
# include <csev.h>
# include <cssminfo.h>
# include <rusage.h>
# include <clpoll.h>
# include <machconf.h>
# include <pwd.h>

# include "csmtmgmt.h"

# include "csmtlocal.h"

# include "csmtsampler.h"

/*
**
**  Name: CSMTMONITOR.C - CS Monitor functions for OS threads.
**
**  Description:
**      This module contains those routines which provide support for
**	CS monitoring under OS threads.
**
**	    CSMTmonitor		- main routine.
**
[@func_list@]...
**
**  History:
**      22-jan-2002 (devjo01)
**          Added this banner.  See individual functions for older
**          histories.
**      23-apr-2002 (horda03) Bug 107642
**          When displaying mutexes via the showmutex command, don't bother
**          checking that the address of the mutex specified by the user matches
**          the address of the mutex when it was initialised. For shared process
**          mutexes, the mutexes may resided in different parts of the process
**          memory space.
**	04-sep-2002 (devjo01)
**	    - Add processing of "registered" foreign diagnostics to
**	      OS threaded version of iimonitor.
**	    - Use function address set in Cs_srv_block.cs_format_lkkey
**	      to format LK_LOCK_KEY sync objects.
**	14-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	18-Jun-2005 (hanje04)
**	    Replace GLOBALDEF with GLOBALDEF for CsSamplerBlkPtr to stop
**	    compiler error on MAC OSX.
**      28-Apr-2005 (hanal04) Bug 114424 INGSRV3277
**          Tru64 does not hold the thread ID at the start of the cs_thread_id
**          structure. ifdef and explicitly reference the __sequence field.
**	25-Jan-2006 (bonro01)
**	    Lock SCB chain during "stacks" and "show all sessions"
**	    commands.
**	31-Jan-2007 (kiria01) b117581
**	    Use global cs_mask_names and cs_state_names.
**	09-dec-2008 (joea)
**	    Exclude Ex_core_enabled on VMS.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	11-Nov-2010 (kschendel) SIR 124685
**	    Delete CS_SYSTEM ref, defined in csinternal.h.
**	    Normalize trailing \n, which TRformat suppresses.  Gak.
*/


#if !defined(VMS)
GLOBALREF i4                    Ex_core_enabled;
#endif

/*
** SAMPLER globals and externs
*/
GLOBALREF CS_SEMAPHORE    	Cs_known_list_sem;
GLOBALDEF CS_SYNCH		hCsSamplerSem ZERO_FILL;
GLOBALREF CSSAMPLERBLKPTR	CsSamplerBlkPtr;

# if defined(DCE_THREADS)
GLOBALREF CS_THREAD_ID Cs_thread_id_init;
# endif /* DCE_THREADS */

static CS_THREAD_ID  CsSamplerThreadId ZERO_FILL;
static bool	     SamplerInit = FALSE;

static STATUS StartSampler(
                char    *command,
		TR_OUTPUT_FCN *output_fcn);

static STATUS StopSampler(
		TR_OUTPUT_FCN *output_fcn);

static STATUS ShowSampler(
		TR_OUTPUT_FCN *output_fcn);

static void show_sessions(
    char    *command,
    TR_OUTPUT_FCN *output_fcn,
    i4	    formatted,
    i4	    powerful);

static void test_segv(char *command, TR_OUTPUT_FCN *fcn, i4 depth );

static void format_sessions(char *command, TR_OUTPUT_FCN *fcn, i4 powerful);


/*{
** Name: CSMTmonitor	- Implement the monitor task
**
** Description:
**	This routine is called ala the regular thread processing routine.
**	It parses its input, decides what to do, and returns the output.
**
** Inputs:
**	mode				Mode of operation
**					    CS_INPUT, _OUPUT, ...
**	scb				Sessions control block to monitor
**	*command			Text of the command
**	powerful			Is this user powerful
**	output_fcn			Function to call to perform the output.
**					This routine will be called as
**					    (*output_fcn)(NULL,
**							    length,
**							    buffer)
**					where buffer is the length character
**					output string
**
** Outputs:
**	next_mode			As above.
**	Returns:
**	    OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	13-jan-1987 (fred)
**	    Rudimentary version created.
**	26-jan-1988 (fred)
**	    Added output function to solve overflow bugs.
**	30-sep-1988 (rogerk)
**	    Added alter session command to alter session characteristics.
**	    Added alter server command to alter server characteristics.
**	28-Jan-1991 (anton)
**	    Added CS_CNDWAIT state
**	12-feb-1991 (mikem)
**	    On systems which support it, made the debug option print a hex
**	    stack trace of the thread (called "CS_dump_stack()").
**	25-sep-1991 (mikem) integrated following change: 18-aug-1991 (ralph)
**          CSmonitor() - Drive CSattn() when IIMONITOR REMOVE
**	23-Oct-1992 (daveb)
**	    cast some calls correctly; sids are nats, really.
**	23-nov-1992 (bryanp)
**	    Given all the new special threads in the server nowadays, the
**	    standard show sessions command is overwhelmed with lots of useless
**	    session information. To "improve" matters, this change changes the
**	    syntax to:
**		show [user/all/special] sessions
**		format [user/all/special] sessions
**	    where the default for user/all/special is user. Also, I'm making
**	    it illegal to remove a special thread until someone can prove to
**	    me why we should allow it (typically, the effect is that the server
**	    crashes).
**	26-apr-1993 (fredv)
**	    Moved <clconfig.h>, <systypes.h>, <clsigs.h> up to avoid
**		redefine of EXCONTINUE by AIX's system header context.h.
**		The same change was done in 6.4 but didn't make into 6.5.
**	24-may-1993 (bryanp)
**	    Added semaphore name to "show mutex" command.
**	8-jun-93 (ed)
**	    Change scb parameter to match spec
**	30-jun-93 (shailaja)
**	    Fixed compiler warnings. 
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	16-aug-93 (ed)
**	    add <st.h>
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**	23-aug-1993 (andys)
**	    In CSmonitor, check the return value of CSremove before calling
**	    CSattn. [bug 45020] [was 28-aug-92 (andys)]
**	1-Sep-93 (seiwald)
**	    CS option revamp: csdbms.h gone.
**      01-sep-93 (smc)
**          Commented lint pointer truncation warning, cast sids to CS_SID.
**	20-sep-93 (mikem)
**	    Added support to "show smstats" for new semaphore contention
**	    statistics fields.  Also changed calls to CVahxl() to be type
**	    correct.
**      31-dec-1993 (andys)
**          Add new command "stacks" which prints out all session stacks.
**	    Add extra "verbose" parameter to calls to CS_dump_stack().
**	06-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	14-oct-93 (swm)
**	    Bug #56445
**	    Assign sem or cnd pointers to new cs_sync_obj (synchronisation
**	    object) rather than overloading cs_memory which is a i4;
**	    pointers do not fit into i4s on all machines!
**	    Also, now that semaphores are exclusive, remove unnecessary
**	    code that negates the sem address.
**	    Also, since semaphores are exclusive eliminate the test for a
**	    negated semaphore address in TRdisplays.
**	    Changed displays of cs_self to use %p rather than %x since a
**	    CS_SID is the same size as a pointer.
**	02-dec-93 (swm)
**	    Bug #58641
**	    Changed CSmonitor() and format_sessions() to use CVaxptr() to
**	    convert hex strings representing pointers to pointers, rather
**	    CVahxl().
**	31-jan-1994 (bryanp) B57296
**	    Fixed show mutex command to handle unnamed semaphores properly.
**	31-jan-1994 (bryanp) B56960
**	    LRC requests that "special" should be "system"
**      15-feb-1994 (andyw)
**          Modify HELP context. Display new commands with the exceptions of
**          CRASH SERVER,SHOW MUTEX,DEBUG & STACKS. Bug ref. 56720
**	15-may-1995 (harpa06)
**	    Added "SHOW SERVER LISTEN" and "SHOW SERVER SHUTDOWN" to the help
**	    Bug #68690
**	15-May-1995 (jenjo02)
**	    Changed semaphore "naps" to "redispatches" to reflect new
**	    spin algorithms.
**	19-sep-1995 (nick)
**	    New parameter to CS_dump_stack().
**	01-Dec-1995 (jenjo02)
**	    Added 2 new wait states, "CS_LKEVENT_MASK" and CS_LGEVENT_MASK"
**	    to statistically distinguish these types from other LK/LG
**	    waits.
**	05-dec-95 (hanch04)
**	    bug 73016, Add more help, suspend, resume, closed, open
**	03-jun-1996 (canor01)
**	    Add "sampler" code from the NT CS for versions that support
**	    operating-system threads. Clean up references to session ids-- 
**	    they should refer to CS_SID's, not (CS_SCB *)'s.
**	13-Aug-1996 (jenjo02)
**	    Modified "show sm statistics" to match thread's version of
**	    CS_SEMAPHORE. Rewrote "showmutex".
**	16-sep-1996 (canor01)
**	    When printing sampler info on mutexes, show the mutex id
**	    (actually its address) for use by "show mutex".
**	18-sep-1996 (canor01)
**	    Sampler thread can show number of user sessions and high-water
**	    mark for number of user sessions.  Suppress statistics on
**	    both the sampler thread and any monitor thread.
**	11-nov-1996 (canor01)
**	    Protect cs_num_sessions and cs_num_active from simultaneous
**	    access.
**	16-Dec-1996 (shero03)
**	    Add Lock Wait Analysis section
**      18-feb-1997 (hanch04)
**          As part of merging of Ingres-threaded and OS-threaded servers,
**          rename file to csmtmonitor.c and moved all calls to CSMT...
**      07-mar-1997 (canor01)
**          Make all functions dummies unless OS_THREADS_USED is defined.
**	14-mar-1997 (hanch04)
**	    Removed extra endif
**	11-apr-1997 (canor01)
**	    Make CS_tls statistics dependent on inclusion of local tls
**	    functions.
**	06-Jun-1997 (shero03)
**	    Print out the top Mutex and Lock entries.
**	29-Jun-1997 (muhpa01)
**	    Added POSIX_DRAFT_4 changes for clearing/testing of 
**	    CsSamplerThreadId.
**	14-Oct-1997 (shero03)
**	    Print out the rest of the Mutex and Lock entries in one bucket.
**	03-nov-1997 (canor01)
**	    CS_create_thread now takes a stack address as a parameter.
**	10-Dec-1997 (jenjo02)
**	    Added number of cross-process mutex spins and thread yields.
**	15-Dec-1997 (muhpa01)
**	    Reset the SamplerInit flag in StopSampler in order to force
**	    initialization of hCsSamplerSem when sampling is started again.
**	    This in necessary on platforms which require explicit mutex
**	    initialization, such as HP.  Also, issue CS_thread_detach for
**	    hp8_us5 - this is needed to reclaim thread resources from the
**	    terminated sampler thread.
**      7-jan-98 (stephenb)
**          Format the output for "show sessions" better and add the 
**          active users highwater mark in preparation for user based licensing
**      8-jan-98 (stephenb)
**          switch high water mark in above chage to appear after max active
**          so that IPM is not confused, also break up long displya line
**          to pervent piccolo from baulking.
**	11-Mar-1998 (jenjo02)
**	    MAXTHREADS changed to MAXSAMPTHREADS to avoid conflict with
**	    sys/threads.h. Added support for Factotum threads.
**	02-Jun-1998 (jenjo02)
**          Show session id (cs_self), not &scb, in "Session <sid> is an internal task"
**          messages consistently.
**	24-Sep-1998 (jenjo02)
**	    Standardize cs_self = (CS_SID *)scb, session id's are always 
**	    a pointer to the thread's SCB, never OS thread_id, which is 
**	    contained in cs_thread_id.
**	    Eliminate cs_cnt_mutex, hold Cs_known_list_sem instead while adjusting
**	    session counts.
**	    Added sampler display of server CPU time and wait counts.
**	    Added "show <...> sessions stats" to act as a visual aid
**	    to see what, if any, progress the sessions are
**	    making in their quest.
**      5-nov-98 (shust01)
**          Added internal option memorydbg on|off, to 'color' memory for diagnostic
**          purposes.  Also added core generation option to enable a core file to
**          be generated when a crash occurs. enablecore | disablecore.
**	16-Nov-1998 (jenjo02)
**	    Cleaned up "sessions stats" to show only non-zero counts.
**	    New event wait masks added for Log I/O, I/O read/write.
**	    Incorporated server wait statistics into Event Wait
**	    Analysis graph.
**	02-Dec-1998 (muhpa01)
**	    Removed code for POSIX_DRAFT_4 for HP - obsolete
**	21-jan-1999 (canor01)
**	    Remove erglf.h.
**	01-Feb-1999 (jenjo02)
**	    Repaired computation of log/lock event wait times.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	14-apr-99 (devjo01)
**	    Add code to allow easy test of SEGV/SIGBUS stack trace.
**	    New "test11" command provided, and the frame for other
**	    testxx commands put in place.
**	15-Nov-1999 (jenjo02)
**	    Removed cs_sem_init_addr from CS_SEMAPHORE, replaced cs_sid with
**	    cs_thread_id for OS threads. Show mutex owner thread_id when
**	    displaying mutex info.
**	    When displaying session ids for OS threads, append ":thread_id".
**      08-Dec-1999 (podni01)  
**          Put back all POSIX_DRAFT_4 related changes; replaced POSIX_DRAFT_4
**          with DCE_THREADS to reflect the fact that Siemens (rux_us5) needs 
**          this code for some reason removed all over the place.
**      08-Mar-2000 (hanal04) Bug 100771 INGSRV 1125
**          Modified devjo01's exception handler for show mutex to use
**          the new CS_null_handler() which does not log the SIGSEGV
**          or stack dump to the error log.
**      22-Nov-2000 (hanal04) Bug 103286 INGSRV 1321
**          Added exception handler for SIGSEGV in CSMT_find_scb()
**          When "format" is not followed by a session id (or an
**          invalid session id is supplied).
**      25-Feb-2002 (hanal04) Bug 105444 INGSRV 1563
**          Prevent looping in dirty scans of the scb list by
**          checking the cs_tag. An invalid SCB will have an invalid
**          cs_next pointer and never hit the loops exit condition
**          (scb == Cs_srv_block.cs_known_list).
**	04-Apr-2000 (jenjo02)
**	    Added getrusage stats to sampler output, when available.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2000 (jenjo02)
**	    Sampler improvements: break out event wait percentages
**	    by thread type, show DB id in lock id, track and
**	    display I/O and transaction rates, fix mutex and lock
**	    wait logic. Lump all non-user lock waits into one bucket;
**	    "system" threads should never wait on locks.
**	04-Oct-2000 (jenjo02)
**	    Deleted FUNC_EXTERN of CSMT_find_scb(), now prototyped
**	    in cs.h.
**	14-mar-2001 (somsa01)
**	    Added HP-UX equivalents for suspending/resuming a session
**	    thread.
**	22-Feb-2002 (devjo01)
**	    Use CS_verify_sid to parse and validate
**	    user passed SID ident.
**	22-May-2002 (jenjo02)
**	    Resolve long-standing inconsistencies and inaccuracies with
**	    "cs_num_active" by computing it only when needed by MO
**	    or IIMONITOR. Added MO methods, external functions
**	    CS_num_active(), CS_hwm_active(), callable from IIMONITOR,
**	    e.g. Works for both MT and Ingres-threaded servers.
**      23-apr-2002 (hordao3) Bug 107642
**          Don't check memory address of mutex when doing a showmutex. The
**          address of the mutex when it was initialised may be different to
**          the address specified, as the address depends on where the 
**          shared memory is mapped into a process's memory.
**	04-sep-2002 (devjo01)
**	    Add processing of "registered" foreign diagnostics to
**	    OS threaded version of iimonitor.
**	31-oct-2002 (somsa01)
**	    In StartSampler(), pass Cs_srv_block.cs_stksize to
**	    CS_create_thread() rather than a hard limit of 4096 as the
**	    stack size. Also, on HP-UX, pass in cs_stkcache to
**	    CS_create_thread().
**	03-Dec-2002 (hanje04)
**	    In StartSampler() the Linux pre compiler doesn't like the #ifdef
**	    being used to call the CS_create_thread macro with a extra 
**	    parameter for HP. Add a completely separate call as for DCE_THREADS
**	24-Apr-2003 (jenjo02)
**	    Added "kill query" to help syntax, SIR 110141.
**	18-Jun-2003 (hanje04)
**	    Remove ZERO_FILL from GLOBALDEF of CsSamplerBlkPtr because it 
**	    conflicts with the GLOBALDEF in csmonitor.c which is also 
**	    ZERO_FILL'd
**	27-Oct-2006 (smeke01) Bug 116848
**	    Corrected the display of formatted show sessions so that it
**	    correctly displays the admin thread details. Also corrected typo 
**	    "avaiable" and made all displays of the  word "session" start 
**	    with an uppercase 'S' for consistency.
**	05-Nov-2006 (smeke01) Bug 119411
**	    On int.lnx thread-ids are unsigned longs, so I removed i4 coercion  
**	    and changed display format from %d to %ul for this platform.
**	    Introduced a conditionally-defined format string TIDFMT to make 
**	    this easier in future. 
**      23-Apr-2009 (hanal04) Bug 132714
**          Update iimonitor help output to include SAMPLING commands.
**      23-Sep-2009 (hanal04) Bug 115316
**          Added "SHOW SERVER CAPABILITIES".
**	29-Jan-2010 (smeke01) Bug 123192
**	    For 'show smstats' and 'show mutex', display u_i4 values with 
**	    format specifier %10u rather than %8d. 
*/
STATUS
CSMTmonitor(i4 mode, CS_SCB *temp_scb, i4 *nmode, char *command, i4 powerful, TR_OUTPUT_FCN *output_fcn)
{
    STATUS		status;
    i4			count;
    char		buf[81];
    char		tidbuf[32];
    EX_CONTEXT		ex_context;
    CS_SCB		*an_scb;
    CS_SCB		*scan_scb;
    CS_SEMAPHORE	*mutex;
    CS_MNTR_SCB		*scb;
    PTR			ptr_tmp;
    i4			badcmnd = 0;
    EX_CONTEXT          ex_context1;

    temp_scb->cs_thread_type = CS_MONITOR;
    scb = (CS_MNTR_SCB *) temp_scb;
    *nmode = CS_EXCHANGE;
    if (EXdeclare(cs_handler, &ex_context))
    {
	EXdelete();
	return(FAIL);
    }

    switch (mode)
    {
	case CS_INITIATE:
	    *nmode = CS_INPUT;
	case CS_TERMINATE:
	    break;

	case CS_INPUT:
	    CVlower(command);
	    if (STscompare("breakpoint", 0,
			command, 0) == 0)
	    {
		CS_breakpoint();
	    }
	    else if (STscompare("stopserverconditional", 0,
			command, 0) == 0)
	    {
		if (!powerful)
		{
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
				"Superuser status required to stop servers");
		}
		else
		{
		    status = CSMTterminate(CS_COND_CLOSE, &count);
		    if (status)
		    {
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
				"Error stopping server, %d. sessions remaining",
					count);
		    }
		}
	    }
	    else if (STscompare("stopserver", 0,
			command, 0) == 0)
	    {
		if (!powerful)
		{
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
				"Superuser status required to stop servers");
		}
		else
		{
		    status = CSMTterminate(CS_KILL, &count);
		    if (status)
		    {
			TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
				"Server will stop. %d. sessions aborted",
					count);
		    }
		}
	    }
	    else if (STscompare("crashserver", 0,
			command, 0) == 0)
	    {
		if (!powerful)
		{
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
				"Superuser status required to stop servers");
		}
		else
		{
		    status = CSMTterminate(CS_CRASH, &count);
		}
	    }
	    else if (STscompare("setserver shut", 0,
			command, 0) == 0)
	    {
		if (!powerful)
		{
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
				"Superuser status required to stop servers");
		}
		else
		{
		    status = CSMTterminate(CS_CLOSE, &count);
		    if (status)
		    {
			TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
				"Server will stop. %d. sessions remaining",
					count);
		    }
		}
	    }
	    else if (STscompare("showsessions", 0, command, 0) == 0 ||
		     STscompare("showusersessions", 0, command, 0) == 0 ||
		     STscompare("showallsessions", 0, command, 0) == 0 ||
		     STscompare("showsystemsessions", 0, command, 0) == 0)
	    {
		show_sessions(command, output_fcn, 0, powerful);
	    }
	    else if (STscompare("showsessionsformatted", 0, command, 0) == 0 ||
		     STscompare("showusersessionsformatted",
							0, command, 0) == 0 ||
		     STscompare("showallsessionsformatted",
							0, command, 0) == 0 ||
		     STscompare("showsystemsessionsformatted",
							0, command, 0) == 0)
	    {
		show_sessions(command, output_fcn, 1, powerful);
	    }
	    else if (STscompare("showsessionsstats", 0, command, 0) == 0 ||
		     STscompare("showusersessionsstats",
							0, command, 0) == 0 ||
		     STscompare("showallsessionsstats",
							0, command, 0) == 0 ||
		     STscompare("showsystemsessionsstats",
							0, command, 0) == 0)
	    {
		show_sessions(command, output_fcn, 2, powerful);
	    }
	    else if ((STzapblank(command, command))
			&& STscompare("showmutex", 9, command, 9) == 0)
	    {
               /* lint truncation warning if size of ptr > i4, 
		  but code valid */
		if ((CVaxptr(command + 9, &ptr_tmp)) || 
		    (ptr_tmp  < (PTR)0x2000))
		{
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"Invalid mutex id %p", ptr_tmp);
		    break;
		}
                else
                {
                    mutex = (CS_SEMAPHORE *) ptr_tmp;
                }

                if (EXdeclare(CS_null_handler, &ex_context1))
                {
		   TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"mutex id %p invalid address", mutex);
                   EXdelete();
		   break;
		}

		if ( mutex->cs_sem_scribble_check != CS_SEM_LOOKS_GOOD &&
		     mutex->cs_sem_scribble_check != CS_SEM_WAS_REMOVED)
		{
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"Structure at %p does not look like a mutex",
				mutex);
                    EXdelete();
		    break;
		}

		if ( mutex->cs_value )
		{
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"Mutex at %p: Name: %s, EXCL owner: (tid: %s, pid: %d)",
				mutex,
		    mutex->cs_sem_name[0] ? mutex->cs_sem_name : "(UN-NAMED)",
#if defined(axp_osf)
			    STprintf(tidbuf, TIDFMT, mutex->cs_thread_id->__sequence),
#else
			    STprintf(tidbuf, TIDFMT, mutex->cs_thread_id),
#endif
				mutex->cs_pid);
		}
		else if ( mutex->cs_count )
		{
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"Mutex at %p: Name: %s, SHARE owner: (tid: %s, pid: %d), count: %d",
				mutex,
		    mutex->cs_sem_name[0] ? mutex->cs_sem_name : "(UN-NAMED)",
#if defined(axp_osf)
			    STprintf(tidbuf, TIDFMT, mutex->cs_thread_id->__sequence),
#else
			    STprintf(tidbuf, TIDFMT, mutex->cs_thread_id),
#endif
				mutex->cs_pid,
				mutex->cs_count);
		}
		else
		{
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"Mutex at %p: Name: %s, is currently unowned",
				mutex,
		    mutex->cs_sem_name[0] ? mutex->cs_sem_name : "(UN-NAMED)");
		}
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		"    Shared: %10u Collisions: %10u Hwm: %10u \n      Excl: %10u Collisions: %10u",
				mutex->cs_smstatistics.cs_sms_count,
				mutex->cs_smstatistics.cs_smsx_count,
				mutex->cs_smstatistics.cs_sms_hwm,
				mutex->cs_smstatistics.cs_smx_count,
				mutex->cs_smstatistics.cs_smxx_count);
                EXdelete();
		if (mutex->cs_type == CS_SEM_MULTI)
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		    "     Spins: %10u     Yields: %10u",
				mutex->cs_smstatistics.cs_smcl_count,
				mutex->cs_smstatistics.cs_smmp_count);
	    }
	    else if (STscompare("showserver", 0, command, 0) == 0)
	    {
		i4 num_sessions, num_active;

		num_sessions = Cs_srv_block.cs_num_sessions;
		num_active = CS_num_active();
                TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
                    "\n\tServer: %s\n \nConnected Sessions (includes \
system threads)\n \n\tCurrent:\t%d\n\tLimit:\t\t%d\n \nActive \
Sessions\n \n\tCurrent\t\t%d\n\tLimit\t\t%d\n\tHigh \
Water\t%d\n \nrdy mask %x state: %x\nidle quantums %d./%d. (%d.%%)",
                    Cs_srv_block.cs_name,
                    num_sessions, Cs_srv_block.cs_max_sessions,
                    num_active, Cs_srv_block.cs_max_active,
                    Cs_srv_block.cs_hwm_active,
                    Cs_srv_block.cs_ready_mask,
                    Cs_srv_block.cs_state,
                    Cs_srv_block.cs_idle_time,
                    Cs_srv_block.cs_quantums,
                    (Cs_srv_block.cs_quantums ?
                        ((Cs_srv_block.cs_idle_time *
                            100)/Cs_srv_block.cs_quantums) : 100));
	    }
	    else if (STscompare("showsmstats", 11, command, 11) == 0)
	    {
		char		place[1024];
		char		*buffer = place;
		
		STprintf(buffer, "Semaphore Statistics:\n");
		buffer += STlength(buffer);
		STprintf(buffer, "--------- -----------\n");
		buffer += STlength(buffer);
		STprintf(buffer, "    Single process  Shared: %10u  Collisions: %10u\n",
			    Cs_srv_block.cs_smstatistics.cs_smss_count,
			    Cs_srv_block.cs_smstatistics.cs_smssx_count);
		buffer += STlength(buffer);
		STprintf(buffer, "                      Excl: %10u  Collisions: %10u\n",
			    Cs_srv_block.cs_smstatistics.cs_smsx_count,
			    Cs_srv_block.cs_smstatistics.cs_smsxx_count);
		buffer += STlength(buffer);
		STprintf(buffer, "     Multi process  Shared: %10u  Collisions: %10u\n",
			    Cs_srv_block.cs_smstatistics.cs_smms_count,
			    Cs_srv_block.cs_smstatistics.cs_smmsx_count);
		buffer += STlength(buffer);
		STprintf(buffer, "                      Excl: %10u  Collisions: %10u\n",
			    Cs_srv_block.cs_smstatistics.cs_smmx_count,
			    Cs_srv_block.cs_smstatistics.cs_smmxx_count);
		buffer += STlength(buffer);
		STprintf(buffer, "                     Spins: %10u      Yields: %10u\n",
			    Cs_srv_block.cs_smstatistics.cs_smcl_count,
			    Cs_srv_block.cs_smstatistics.cs_smmp_count);
		buffer += STlength(buffer);
		STprintf(buffer, "--------- -----------\n");
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1, place);
	    }
	    else if (STscompare("clearsmstats", 12, command, 12) == 0)
	    {
		Cs_srv_block.cs_smstatistics.cs_smss_count = 0;
		Cs_srv_block.cs_smstatistics.cs_smssx_count = 0;
		Cs_srv_block.cs_smstatistics.cs_smsx_count = 0;
		Cs_srv_block.cs_smstatistics.cs_smsxx_count = 0;
		Cs_srv_block.cs_smstatistics.cs_smms_count = 0;
		Cs_srv_block.cs_smstatistics.cs_smmsx_count = 0;
		Cs_srv_block.cs_smstatistics.cs_smmx_count = 0;
		Cs_srv_block.cs_smstatistics.cs_smmxx_count = 0;
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		    "Semaphore statistics cleared");
		TRdisplay("=========\n***** WARNING: Semaphore statistics cleared at %@ by %24s\n=========\n",
		    scb->csm_scb.cs_username);
	    }
	    else if (STscompare("format", 6, command, 6) == 0)
	    {
		format_sessions(command, output_fcn, powerful);
	    }
            else if ((STscompare("startsampling", 13, command, 13) == 0))
            {
                if (!powerful)
                {
                    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
                        "Superuser status required to start sampling.");
                }
                else
                {
                    status = StartSampler(command, output_fcn);
                    if (status)
                    {
                        TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
                               "Sampling failed to start.");
                    }
                }
            }
            else if ((STscompare("stopsampling", 0, command, 0) == 0))
            {
                if (!powerful)
                {
                    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
                        "Superuser status required to stop sampling.");
                }
                else
                {
                    status = StopSampler(output_fcn);
                    if (status)
                    {
                        TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
                               "Sampling failed to stop.");
                    }
                }
            }
            else if ((STscompare("showsampling", 0, command, 0) == 0))
            {
                if (!powerful)
                {
                    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
                        "Superuser status required to show sampling.");
                }
                else
                {
                    status = ShowSampler(output_fcn);
                    if (status)
                    {
                        TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
                               "Show sampling failed.");
                    }
                }
            }
	    else if (STscompare("remove", 6, command, 6) == 0)
	    {
		if ( NULL == ( an_scb = CS_verify_sid("remove", command,
				output_fcn, 1) ) )
		{
		    break;
		}
		else
		{
		    if ((MEcmp(an_scb->cs_username,
				scb->csm_scb.cs_username,
				sizeof(scb->csm_scb.cs_username))) && !powerful)
		    {
			TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
				"Superuser or owner status required to stop sessions");
		    }
		    else
		    {
			if (CSMTremove((CS_SID)an_scb->cs_self) == OK)
			{
		            TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			        "Session %p removed", an_scb->cs_self);
                            CSMTattn(CS_REMOVE_EVENT, (CS_SID)an_scb->cs_self);
			}
			else
			{
		            TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			        "Session %p not removed", an_scb->cs_self);
			}
		    }
		}
	    }
	    else if (STscompare("suspend", 7, command, 7) == 0)
	    {
		if ( NULL == ( an_scb = CS_verify_sid("suspend", command,
				output_fcn, 1) ) )
		    break;

		if (an_scb->cs_state != CS_COMPUTABLE)
		{
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"Session %p is not computable -- not suspended",
			    an_scb->cs_self);
		}
		else
		{
		    if ((MEcmp(an_scb->cs_username,
				scb->csm_scb.cs_username,
				sizeof(scb->csm_scb.cs_username))) && !powerful)
		    {
			TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
				"Superuser or owner status required to suspend sessions");
		    }
		    else
		    {
			an_scb->cs_state = CS_UWAIT;
# ifdef sparc_sol
			thr_suspend( an_scb->cs_thread_id );
# elif defined(thr_hpux)
			pthread_suspend( an_scb->cs_thread_id );
# endif
			TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			    "Session %p has been indefinitely suspended",
				an_scb->cs_self);
		    }
		}
	    }
	    else if (STscompare("resume", 6, command, 6) == 0)
	    {
		if ( NULL == ( an_scb = CS_verify_sid("resume", command,
				output_fcn, 1) ) )
		    break;
		
		if (an_scb->cs_state != CS_UWAIT)
		{
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"Session %p was not suspended",
			    an_scb->cs_self);
		}
		else
		{
		    if (!powerful)
		    {
			TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
				"Superuser status required to resume sessions");
		    }
		    else
		    {
			an_scb->cs_state = CS_COMPUTABLE;
# ifdef sparc_sol
			thr_continue( an_scb->cs_thread_id );
# elif defined(thr_hpux)
			pthread_continue( an_scb->cs_thread_id );
# endif /* su4_us5 */
			TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			    "Session %p has been resumed",
				an_scb->cs_self);
		    }
		}
	    }
	    else if (STscompare("debug", 5, command, 5) == 0)
	    {
		if ( NULL == ( an_scb = CS_verify_sid("debug", command,
				output_fcn, 0) ) )
		    break;

		{
		    char	buffer[1048];
		    CS_fmt_scb(an_scb,
				sizeof(buffer),
				buffer);
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1, buffer);
		}

		CSMTget_scb(&scan_scb);
		if (scan_scb != an_scb)
			CS_dump_stack(an_scb, NULL, NULL, output_fcn, FALSE);
		else
			CS_dump_stack(NULL, NULL, NULL, output_fcn, FALSE);
	    }
            else if (STscompare("stacks", 6, command, 6) == 0)
            {
	        CSp_semaphore(0, &Cs_known_list_sem);
                for (scan_scb = Cs_srv_block.cs_known_list->cs_next;
                    scan_scb && scan_scb != Cs_srv_block.cs_known_list;
                    scan_scb = scan_scb->cs_next)
                {
                        if(scan_scb->cs_tag != CS_TAG)
        {
                            TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
                                ">>>>> Session list has changed. Exiting stacks. <<<<<");
                            break;
                        }

                        TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
                                "\n---\nStack dump for session %p",
                                scan_scb->cs_self);
			CSMTget_scb(&an_scb);
                        if (an_scb != scan_scb)
                                CS_dump_stack(scan_scb, NULL, NULL, output_fcn, FALSE);
                        else
                                CS_dump_stack(NULL, NULL, NULL, output_fcn, FALSE);
                }
	        CSv_semaphore(&Cs_known_list_sem);
            }
#if !defined(VMS)
            else if (STscompare("enablecore", 10, command, 10) == 0)
            {
                Ex_core_enabled = 1;
            }
            else if (STscompare("disablecore", 11, command, 11) == 0)
            {
                Ex_core_enabled = 0;
            }
#endif
	    else if (STscompare("help", 4, command, 4) == 0)
	    {
                TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
                    "Server:\n     SET SERVER [SHUT | CLOSED | OPEN]\n \
    STOP SERVER\n     SHOW SERVER [LISTEN | SHUTDOWN | CAPABILITIES]\n     START SAMPLING [milli second interval]\n     STOP SAMPLING\n     SHOW SAMPLING\n\n");
                TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
                    "Session:\n     SHOW [USER] SESSIONS [FORMATTED | STATS]\n \
    SHOW SYSTEM SESSIONS [FORMATTED | STATS]\n \
    SHOW ALL SESSIONS [FORMATTED | STATS]\n \
    FORMAT [ALL | sid]\n     REMOVE [sid]\n     SUSPEND [sid]\n \
    RESUME [sid]\n\nQuery:\n     KILL [sid]\n\nOthers:\n     QUIT\n\n"
                        );
	    }
	    else if (STscompare("test", 4, command, 4) == 0)
	    {
		i4	test;

		if ( OK != CVan(command+4,&test) )
		{
		    badcmnd = 1;
		    break;
		}
		switch ( test )
		{
		case 11:  /* Test stack backtrace by forcing a SEGV */
		    test_segv(command,output_fcn,0);
		    break;
		default:
		    badcmnd = 1;
		    break;
		}	
	    }
	    else if ( OK != CSmon_try_registered_diags( output_fcn, command ) )
	    {
		badcmnd = 1;
	    }
	    break;

	case CS_OUTPUT:
	    break;
    }
    if ( badcmnd )
    {
	TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		"Illegal command");
    }
    EXdelete();
    return(OK);
}

/*
** History:
**	31-jan-1994 (bryanp) B56960
**	    LRC requests that "special" should be "system"
**      08-Mar-2000 (hanal04) Bug 100771 INGSRV 1125
**          Add exceptional handler when formatting session info
**          as the SCB may be in the process of being removed.
**      25-Feb-2002 (hanal04) Bug 105444 INGSRV 1563
**          Prevent looping in dirty scans of the scb list by
**          checking the cs_tag. An invalid SCB will have an invalid
**          cs_next pointer and never hit the loops exit condition
**          (scb == Cs_srv_block.cs_known_list).
**      19-aug-2002 (devjo01)
**          Format target LKEVENT block if LKEVENT waits. 
**	04-sep-2002 (devjo01)
**	    Use cs_format_lkkey to display blocking lock.
*/
static char *
format_lkevent( CS_SCB *scb, char *buffer )
{
    STcopy("LKEVENT", buffer);
    if ( scb->cs_sync_obj )
    {
        STprintf( buffer + STlength(buffer), "(%x,%x,%x)",
                  ((LK_EVENT*)(scb->cs_sync_obj))->type_high,
                  ((LK_EVENT*)(scb->cs_sync_obj))->type_low,
                  ((LK_EVENT*)(scb->cs_sync_obj))->value );
    }
    return buffer;
}

static char *
format_lock( CS_SCB *scb, char *buffer )
{
    char	keybuf[128];

    STcopy("LOCK", buffer);
    if ( Cs_srv_block.cs_format_lkkey && scb->cs_sync_obj )
    {
        STprintf( buffer + STlength(buffer), "(%s)",
	  (*Cs_srv_block.cs_format_lkkey)( (LK_LOCK_KEY *)(scb->cs_sync_obj),
	  keybuf ) );
    }
    return buffer;
}

/*
** History:
**	07-Oct-2008 (smeke01) b120907
**	    Show OS thread id for those platforms that support it (see csnormal.h).
**	09-Feb-2009 (smeke01) b119586
**	    Use new function CSMT_get_thread_stats() to get CPU
**	    stats output to work without the overhead of having 
**	    session_accounting turned on. 
*/
static void
show_sessions(
    char    *command,
    TR_OUTPUT_FCN *output_fcn,
    i4	    format_option,
    i4	    powerful)
{
    CS_SCB	*an_scb;
    char	buf[132];
    char	tbuf[128];
    char	tidbuf[32];
    i4		show_user;
    i4		show_system;
    EX_CONTEXT  ex;
    i4		formatted = (format_option == 1) ? 1 : 0;
    i4		stats = (format_option == 2) ? 1 : 0;
    char	szOStid[sizeof(" OS_tid: ") + MAX_UI8_DIGITS + 1];
    CS_THREAD_STATS_CB	thread_stats_cb;

    if (STscompare("showsessions", 0, command, 0) == 0 ||
	STscompare("showusersessions", 0, command, 0) == 0 ||
	STscompare("showsessionsformatted", 0, command, 0) == 0 ||
	STscompare("showusersessionsformatted", 0, command, 0) == 0 ||
	STscompare("showsessionsstats", 0, command, 0) == 0 ||
	STscompare("showusersessionsstats", 0, command, 0) == 0)
    {
	show_user = 1;
	show_system = 0;
    }
    else if (STscompare("showsystemsessions", 0, command, 0) == 0 ||
	     STscompare("showsystemsessionsformatted", 0, command, 0) == 0 ||
	     STscompare("showsystemsessionsstats", 0, command, 0) == 0)
    {
	show_user = 0;
	show_system = 1;
    }
    else    /* show all sessions or show all sessions formatted 
	       or show all sessions stats */
    {
	show_user = 1;
	show_system = 1;
    }

    CSp_semaphore(0, &Cs_known_list_sem);

    for (an_scb = Cs_srv_block.cs_known_list->cs_next;
	an_scb && an_scb != Cs_srv_block.cs_known_list;
	an_scb = an_scb->cs_next)
    {
	if (an_scb->cs_os_tid != 0)
	{
	    STcopy(" OS_tid: ", szOStid); 
	    if (sizeof(an_scb->cs_os_tid) <= 4)
		CVula(an_scb->cs_os_tid, (char *)(szOStid + sizeof(" OS_tid: ") - 1));
	    else
		CVula8(an_scb->cs_os_tid, (char *)(szOStid + sizeof(" OS_tid: ") - 1));
	}
	else
	{
	    szOStid[0] = EOS;
	}

        if(an_scb->cs_tag != CS_TAG)
        {
            TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
                ">>>>> Session list has changed. Exiting show sessions. <<<<<");
            break;
        }

	if (show_system == 0 && 
            an_scb->cs_thread_type != CS_USER_THREAD &&
            an_scb->cs_thread_type != CS_MONITOR)
	    continue;

	if (show_user == 0 && 
	    (an_scb->cs_thread_type == CS_USER_THREAD ||
	     an_scb->cs_thread_type == CS_MONITOR))
	    continue;
	
        if (EXdeclare(CS_null_handler, &ex) != OK)
        {
            EXdelete();
            TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
                ">>>>> (SCB %p) Session being removed <<<<<", an_scb);
            continue;
        }

	if (stats)
	{
	    char	tbuf[20];
	    char	sbuf[132];

	    sbuf[0] = '\0';

	    if (an_scb->cs_bior)
	        STprintf(tbuf, "BIOR %d ", an_scb->cs_bior),
		STcat(sbuf,tbuf);
	    if (an_scb->cs_biow)
	        STprintf(tbuf, "BIOW %d ", an_scb->cs_biow),
		STcat(sbuf,tbuf);
	    if (an_scb->cs_dior)
	        STprintf(tbuf, "DIOR %d ", an_scb->cs_dior),
		STcat(sbuf,tbuf);
	    if (an_scb->cs_diow)
	        STprintf(tbuf, "DIOW %d ", an_scb->cs_diow),
		STcat(sbuf,tbuf);
	    if (an_scb->cs_lior)
	        STprintf(tbuf, "LIOR %d ", an_scb->cs_lior),
		STcat(sbuf,tbuf);
	    if (an_scb->cs_liow)
	        STprintf(tbuf, "LIOW %d ", an_scb->cs_liow),
		STcat(sbuf,tbuf);
	    if (an_scb->cs_locks)
	        STprintf(tbuf, "Lock %d ", an_scb->cs_locks),
		STcat(sbuf,tbuf);
	    if (an_scb->cs_lkevent)
	        STprintf(tbuf, "LKEvnt %d ", an_scb->cs_lkevent),
		STcat(sbuf,tbuf);
	    if (an_scb->cs_logs)
	        STprintf(tbuf, "Log %d ", an_scb->cs_logs),
		STcat(sbuf,tbuf);
	    if (an_scb->cs_lgevent)
	        STprintf(tbuf, "LGEvnt %d ", an_scb->cs_lgevent),
		STcat(sbuf,tbuf);

	    if (an_scb->cs_os_tid != 0)
	    {
		thread_stats_cb.cs_os_pid = an_scb->cs_os_pid;
		thread_stats_cb.cs_os_tid = an_scb->cs_os_tid;
		thread_stats_cb.cs_thread_id = an_scb->cs_thread_id; 
		thread_stats_cb.cs_stats_flag = CS_THREAD_STATS_CPU; 
		if (CSMT_get_thread_stats(&thread_stats_cb) == OK)
		{ 
		    STprintf(tbuf, "CPU u:%d s:%d", thread_stats_cb.cs_usercpu, thread_stats_cb.cs_systemcpu);
		    STcat(sbuf,tbuf);
		}
	    }
	    else
	    if (an_scb->cs_cputime)
	        STprintf(tbuf, "CPU %d", an_scb->cs_cputime * 10),
		STcat(sbuf,tbuf);

	    if (an_scb->cs_os_tid != 0 )
	        STcat(sbuf, szOStid);

	    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
	    	"Session %p:%s (%24s) %s",
		an_scb->cs_self,
#if defined(axp_osf)
		STprintf(tidbuf, TIDFMT, an_scb->cs_thread_id->__sequence),
#else
		STprintf(tidbuf, TIDFMT, an_scb->cs_thread_id),
#endif
		an_scb->cs_username,
		sbuf);
	}
	else switch (an_scb->cs_state)
	{
	    case CS_FREE:
	    case CS_COMPUTABLE:
	    case CS_STACK_WAIT:
	    case CS_UWAIT:
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		    "Session %p:%s (%24s) cs_state: %w cs_mask: %v%s",
			    an_scb->cs_self,
#if defined(axp_osf)
			    STprintf(tidbuf, TIDFMT, an_scb->cs_thread_id->__sequence),
#else
			    STprintf(tidbuf, TIDFMT, an_scb->cs_thread_id),
#endif
			    an_scb->cs_username,
			    cs_state_names,
			    an_scb->cs_state,
			    cs_mask_names,
			    an_scb->cs_mask,
			    szOStid);
		if (an_scb->cs_mask & CS_MUTEX_MASK && 
		    an_scb->cs_sync_obj)
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"        Mutex: %s",
			((CS_SEMAPHORE *)an_scb->cs_sync_obj)->cs_sem_name[0] ?
			((CS_SEMAPHORE *)an_scb->cs_sync_obj)->cs_sem_name :
			"(UN-NAMED)");
		break;

	    case CS_EVENT_WAIT: 
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		    "Session %p:%s (%24s) cs_state: %w (%s) cs_mask: %v%s",
			    an_scb->cs_self,
#if defined(axp_osf)
			    STprintf(tidbuf, TIDFMT, an_scb->cs_thread_id->__sequence),
#else
			    STprintf(tidbuf, TIDFMT, an_scb->cs_thread_id),
#endif
			    an_scb->cs_username,
			    cs_state_names,
			    an_scb->cs_state,
			    (an_scb->cs_memory & CS_DIO_MASK ?
				 (an_scb->cs_memory & CS_IOR_MASK ?
				    "DIOR" : "DIOW") :
				an_scb->cs_memory & CS_BIO_MASK ?
				 (an_scb->cs_memory & CS_IOR_MASK ?
				    "BIOR" : "BIOW") :
				an_scb->cs_memory & CS_LIO_MASK ?
				 (an_scb->cs_memory & CS_IOR_MASK ?
				    "LIOR" : "LIOW") :
				an_scb->cs_memory & CS_LOCK_MASK ?
				    format_lock( an_scb, tbuf ) :
				an_scb->cs_memory & CS_LOG_MASK ?
				    "LOG" :
				an_scb->cs_memory & CS_LKEVENT_MASK ?
                                    format_lkevent( an_scb, tbuf ) :
				an_scb->cs_memory & CS_LGEVENT_MASK ?
				    "LGEVENT" :
				an_scb->cs_memory & CS_TIMEOUT_MASK ?
				    "TIME"    :
				    "<any>"),
			    cs_mask_names,
			    an_scb->cs_mask,
			    szOStid);
		break;
		
	    case CS_MUTEX:
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		    "Session %p:%s (%24s) cs_state: %w ((%s) %p) cs_mask: %v%s",
			    an_scb->cs_self,
#if defined(axp_osf)
			    STprintf(tidbuf, TIDFMT, an_scb->cs_thread_id->__sequence),
#else
			    STprintf(tidbuf, TIDFMT, an_scb->cs_thread_id),
#endif
			    an_scb->cs_username,
			    cs_state_names,
			    an_scb->cs_state,
			    "x",
			    an_scb->cs_sync_obj,
			    cs_mask_names,
			    an_scb->cs_mask,
			    szOStid);
		if (an_scb->cs_sync_obj)
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"        Mutex: %s",
			((CS_SEMAPHORE *)an_scb->cs_sync_obj)->cs_sem_name[0] ?
			((CS_SEMAPHORE *)an_scb->cs_sync_obj)->cs_sem_name :
			"(UN-NAMED)");
		break;
		
	    case CS_CNDWAIT:
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		    "Session %p:%s (%24s) cs_state: %w (%p) cs_mask: %v%s",
			    an_scb->cs_self,
#if defined(axp_osf)
			    STprintf(tidbuf, TIDFMT, an_scb->cs_thread_id->__sequence),
#else
			    STprintf(tidbuf, TIDFMT, an_scb->cs_thread_id),
#endif
			    an_scb->cs_username,
			    cs_state_names,
			    an_scb->cs_state,
			    an_scb->cs_sync_obj,
			    cs_mask_names,
			    an_scb->cs_mask,
			    szOStid);
		break;
		
	    default:

		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		    "Session %p:%s (%24s) cs_state: <invalid> %x%s",
		    an_scb->cs_self,
#if defined(axp_osf)
		    STprintf(tidbuf, TIDFMT, an_scb->cs_thread_id->__sequence),
#else
		    STprintf(tidbuf, TIDFMT, an_scb->cs_thread_id),
#endif
		    an_scb->cs_username,
		    an_scb->cs_state,
		    szOStid);
		break;

	}
	if (formatted)
	{
	    if (an_scb->cs_mask & CS_MNTR_MASK)
	    {
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		    "Session %p:%s is a monitor task, owner is pid %d",
			an_scb->cs_self,
#if defined(axp_osf)
			STprintf(tidbuf, TIDFMT, an_scb->cs_thread_id->__sequence),
#else
			STprintf(tidbuf, TIDFMT, an_scb->cs_thread_id),
#endif
			an_scb->cs_ppid);
	    }
	    else if ((an_scb == &Cs_idle_scb)
			|| (an_scb == (CS_SCB *)&Cs_admin_scb))
	    {
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		"Session %p:%s is an internal task; no info available",
			an_scb->cs_self,
#if defined(axp_osf)
			STprintf(tidbuf, TIDFMT, an_scb->cs_thread_id->__sequence));
#else
			STprintf(tidbuf, TIDFMT, an_scb->cs_thread_id));
#endif
	    }
	    else
	    {
		(*Cs_srv_block.cs_format)(an_scb,
					    command,
					    powerful, 1);
	    }
	}
        EXdelete();
    }
    CSv_semaphore(&Cs_known_list_sem);

    return;
}

/*
** History:
**	31-jan-1994 (bryanp) B56960
**	    LRC requests that "special" should be "system"
**      08-Mar-2000 (hanal04) Bug 100771 INGSRV 1125
**          Add exceptional handler when formatting session info
**          as the SCB may be in the process of being removed.
**      22-Nov-2000 (hanal04) Bug 103286 INGSRV 1321
**          Added exception handler for SIGSEGV in CSMT_find_scb()
**          When "format" is not followed by a session id (or an
**          invalid session id is supplied).
**      25-Feb-2002 (hanal04) Bug 105444 INGSRV 1563
**          Prevent looping in dirty scans of the scb list by
**          checking the cs_tag. An invalid SCB will have an invalid
**          cs_next pointer and never hit the loops exit condition
**          (scb == Cs_srv_block.cs_known_list).
**      19-Sep-2002 (hanal04) Bug 108741 INGSRV 1883
**          Ensure the Cs_known_list_sem mutex is released
**          if an exception occurs in any of the called functions.
**          Failure to do so prevents the server from accepting
**          any new connections.
*/
static void
format_sessions(char *command, TR_OUTPUT_FCN *output_fcn, i4 powerful)
{
    i4		show_user = 0;
    i4		show_system = 0;
    CS_SCB	*scan_scb;
    CS_SCB	*an_scb;
    char	buf[100];
    char	tidbuf[32];
    EX_CONTEXT	ex_context;

    STzapblank(command, command);

    /*
    ** Syntax:
    **	    format xxxxxxxx	    - format the indicated session
    **	    format all		    - format ALL sessions
    **	    format all sessions	    - ditto
    **	    format user		    - format user sessions
    **	    format user sessions    - same
    **	    format system	    - format system sessions
    **	    format system sessions - same
    */
    if (STscompare("all", 3, command+6, 3) == 0)
    {
	an_scb = 0;
	show_user = 1;
	show_system = 1;
    }
    else if (STscompare("user", 4, command+6, 4) == 0 ||
	     STscompare("sessions", 8, command+6, 8) == 0)
    {
	an_scb = 0;
	show_user = 1;
	show_system = 0;
    }
    else if (STscompare("system", 6, command+6, 6) == 0)
    {
	an_scb = 0;
	show_user = 0;
	show_system = 1;
    }
    else
    {
	if ( NULL == ( an_scb =
		       CS_verify_sid("format", command, output_fcn, 0) ) )
	    return;
    }
    CSp_semaphore(0, &Cs_known_list_sem);

    if (EXdeclare(cs_handler, &ex_context))
    {
       CSv_semaphore(&Cs_known_list_sem);
       TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
            "Server Exception reported. Aborting previous command.");
       EXdelete();
       return;
    }

    for (scan_scb = Cs_srv_block.cs_known_list->cs_next;
	scan_scb && scan_scb != Cs_srv_block.cs_known_list;
	scan_scb = scan_scb->cs_next)
    {
        if(scan_scb->cs_tag != CS_TAG)
        {
            TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
                ">>>>> Session list has changed. Exiting format sessions. <<<<<");
            break;
        }

	if (an_scb == 0 && show_user == 0 &&
			scan_scb->cs_thread_type == CS_USER_THREAD)
	    continue;
	if (an_scb == 0 && show_system == 0 &&
			scan_scb->cs_thread_type != CS_USER_THREAD)
	    continue;

	if (an_scb == 0 || (scan_scb == an_scb))
	{
	    if (scan_scb->cs_mask & CS_MNTR_MASK)
	    {
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		    "Session %p:%s is a monitor task, owner is pid %d",
			scan_scb->cs_self,
#if defined(axp_osf)
			STprintf(tidbuf, TIDFMT, an_scb->cs_thread_id->__sequence),
#else
			STprintf(tidbuf, TIDFMT, an_scb->cs_thread_id),
#endif
			scan_scb->cs_ppid);
	    }
	    else if ((scan_scb == &Cs_idle_scb)
			|| (scan_scb == (CS_SCB *)&Cs_admin_scb))
	    {
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		"Session %p is an internal task; no info available",
			scan_scb->cs_self);
	    }
	    else
	    {
                
		(*Cs_srv_block.cs_format)(scan_scb,
					    command,
					    powerful, 0);
	    }
	    if (an_scb)
	    {
		an_scb = NULL;
		break;	/* we only wanted to format one SCB */
	    }
	}
    }
    CSv_semaphore(&Cs_known_list_sem);
    EXdelete();
    if (an_scb)
    {
	TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
	    "Invalid session id");
    }
}

/*
** History:
**	14-apr-1999 (devjo01
**	    New function to provide convenient way to test SEGV stack trace.
*/
static void 
test_segv(
	char *command,
	TR_OUTPUT_FCN *output_fcn,
	i4 depth
)
{
    char		buf[81];
    EX_CONTEXT		ex_context;
    int	*intp;

    switch ( depth )
    {
    case 0:	/* Top level call, set up */
	TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
	   "Start SEGV test" );
        if (EXdeclare(cs_handler, &ex_context))
        {
	    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
	    "Exception caught" );
	}
	else
	{
 	    test_segv("1st",output_fcn,depth+1);
	}
	TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
	   "End SEGV test" );
	EXdelete();
	break;
    case 3:	/* Force SEGV here */
	intp = (int*)0x3; 
	*intp = 0; /* SEGV or BUS depending on alignment requirements */
	break;
    default:	/* Just call recursively to provide call frames to dump */
	test_segv("dunsel",output_fcn,depth+1);
	break;
    }
} /* test_segv */

/****************************************************************************
**
** Name: StartSampler - Startup the Sampler Thread
**
** Description:
**      This is a local routine that starts the Sampler thread, if is not already
**      running.  .                                                    
**
** Inputs:
**  *command        Text of the command
**  output_fcn      Function to call to perform the output.
**                  This routine will be called as
**                      (*output_fcn)(NULL, length, buffer)
**                  where buffer is the length character
**                  output string
**
**  Returns:
**      OK
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
****************************************************************************/

static STATUS
StartSampler(
	char    *command,
	TR_OUTPUT_FCN *output_fcn )

{
	char	buf[100];
	i4	interval;
	SYSTIME	time;
	char	timestr[50];
	CS_SCB	*an_scb;
	STATUS  status = OK;

	/*
	** Syntax:
	**      start sampling xxxxxxxx		
	**	(xxxxxxx - the interval between samplings in milli seconds)
	**	(xxxxxxx >= 100 and <= 100000 .1 seconds - 100 seconds)
	**	(if xxxxxxxxx is ommitted, then assume 1000 or 1 second.)
	*/
	if (CVal(command + 13, &interval))
	{
	    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	   		"The sampling interval must be between 100 and 100000 milliseconds");
	    return(FAIL);
	}
	if (interval == 0)
	    interval = 1000;	/* default sampling interval is 1 second */

	if ((interval < 100) || (interval > 100000))
	{
	    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	   		"The sampling interval must be between 100 and 100000 milliseconds");
	    return(FAIL);
	}

	/*
	** Lock the sampler block
	*/
	status = LockSamplerBlk(&hCsSamplerSem);
	if (status != OK)
	    return(status);

	/*
	** If the sampler is already running,
	** Update the interval.
	** Unlock the sampler block.
	** Return.
	*/ 
	if (CsSamplerBlkPtr != NULL)
	{
	    CsSamplerBlkPtr->interval = interval;
	    status = UnlockSamplerBlk(&hCsSamplerSem);

	    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	   		"The sampling interval is now %4.2f seconds",
	   		 interval/1000.0);
	    return(OK);
	}


	/*
	** Obtain the sampler block.
	** Initialize the sampler block (including the interval, start time)
	** Create the sampler thread at a bit higher priority.
	*/
	
	CsSamplerBlkPtr = (CSSAMPLERBLKPTR)MEreqmem(0,
	                                    sizeof(CSSAMPLERBLK), TRUE, NULL);
	if (CsSamplerBlkPtr == NULL)
	    return(FAIL);

	TMnow(&time);
	CsSamplerBlkPtr->starttime = time;
	CsSamplerBlkPtr->stoptime = time;
	CsSamplerBlkPtr->interval = interval;

	/* Get and save the current Server CPU usage */
	CS_update_cpu((i4 *)0, (i4 *)0);
	CsSamplerBlkPtr->startcpu = Cs_srv_block.cs_cpu;

	/* Set the starting wait/time counts */
	MEcopy((char*)&Cs_srv_block.cs_wtstatistics, 
		sizeof(CsSamplerBlkPtr->waits),
		    (char*)&CsSamplerBlkPtr->waits);

# if defined(DCE_THREADS)
	CS_create_thread( Cs_srv_block.cs_stksize, /* stack size */
                          CSMT_sampler,            /* init routine */
                          CsSamplerBlkPtr,         /* parm */
                          &CsSamplerThreadId,      /* thread id */
                          CS_JOINABLE,             /* must wait for it */
                          &status );
# elif defined(any_hpux)
	CS_create_thread( Cs_srv_block.cs_stksize, /* stack size */
			  NULL,                    /* stack addr */
			  CSMT_sampler,		   /* init routine */
			  CsSamplerBlkPtr,	   /* parm */
			  &CsSamplerThreadId, 	   /* thread id */
			  CS_JOINABLE,		   /* must wait for it */
                          Cs_srv_block.cs_stkcache,
			  &status );
# else
	CS_create_thread( Cs_srv_block.cs_stksize, /* stack size */
			  NULL,                    /* stack addr */
			  CSMT_sampler,		   /* init routine */
			  CsSamplerBlkPtr,	   /* parm */
			  &CsSamplerThreadId, 	   /* thread id */
			  CS_JOINABLE,		   /* must wait for it */
			  &status );
# endif
	if (status)
	{
	    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
		"The sampler thread failed to start. Status = %d",
		status);
# if defined(DCE_THREADS)
            CS_thread_id_assign( CsSamplerThreadId, Cs_thread_id_init );
# else
            CsSamplerThreadId = 0;
# endif
	}
	else
	{
	    /* increase the thread's priority to maximum */
	    CS_thread_setprio( CsSamplerThreadId, CS_LIM_PRIORITY, &status );
	} 

# ifndef xCL_094_TLS_EXISTS
	/*
	** Because certain CS_tls hash table counts are cumulative, get
	** a starting count.
	*/
	CSMTp_semaphore( 0, &Cs_known_list_sem );

	/* Loop thru all the SCBs in the server */
	for (an_scb = Cs_srv_block.cs_known_list->cs_next;
    	     an_scb && an_scb != Cs_srv_block.cs_known_list;
	     an_scb = an_scb->cs_next)
	{
	    CSMT_tls_measure(an_scb->cs_thread_id, &CsSamplerBlkPtr->numtlsslots, 
		&CsSamplerBlkPtr->numtlsprobes, &CsSamplerBlkPtr->numtlsreads, 
		&CsSamplerBlkPtr->numtlsdirty, &CsSamplerBlkPtr->numtlswrites );
    	} /* for */

	CsSamplerBlkPtr->numtlsslots = 0;
	CsSamplerBlkPtr->numtlsprobes = 0;
	CsSamplerBlkPtr->numtlsreads *= -1;
	CsSamplerBlkPtr->numtlsdirty *= -1;
	CsSamplerBlkPtr->numtlswrites *= -1;

	CSMTv_semaphore( &Cs_known_list_sem );

# endif /* xCL_094_TLS_EXISTS */

#ifdef xCL_003_GETRUSAGE_EXISTS
	/* Initialize process's resource usage baseline */
	getrusage(RUSAGE_SELF, &CsSamplerBlkPtr->ruse);
#endif /* xCL_003_GETRUSAGE_EXISTS */

	/*
	** Unlock the sampler block.
	** Unleash the sampler
	** return
	*/
	status = UnlockSamplerBlk(&hCsSamplerSem);

# if defined(DCE_THREADS)
        if (CS_thread_id_equal(CsSamplerThreadId, Cs_thread_id_init ))
# else
        if (CsSamplerThreadId == 0)     /* if no thread then just return */
# endif /* DCE_THREADS */
	    return(FAIL);

	TMstr(&time, timestr);
	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
		"Sampling is started. %s",
		timestr);

	return(status);

} /* StartSampler */

/****************************************************************************
**
** Name: StopSampler - Stop the Sampler Thread
**
** Description:
**      This is a local routine that if the Sampler thread is running
**	print out the results then causes the Sampler thread to end.
**
** Inputs:
**  output_fcn      Function to call to perform the output.
**                  This routine will be called as
**                      (*output_fcn)(NULL, length, buffer)
**                  where buffer is the length character
**                  output string
**
**		    This may be NULL
**
**  Returns:
**      OK
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
****************************************************************************/

static STATUS
StopSampler(
	TR_OUTPUT_FCN *output_fcn )

{
	SYSTIME	time;
	char	timestr[50];
	char	buf[100];
	STATUS  status = OK;


	if (CsSamplerBlkPtr == NULL)
	{
	    if (output_fcn != NULL)
	        TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	   	     "There is no sampling now");
	    return(status);
	}
	/*
	** Print the report.
	*/
	if (output_fcn != NULL)
	{
	    status = ShowSampler(output_fcn);
 	    if (status != OK)
	        return(status);
	}

	/*
	** Lock the Sampler Block
	** Release the Sampler Block.
	** Null the Sampler Block Ptr.
	** Unlock the Sampler Block
	** Close the handle for the Sampler Thread and Sampler Semaphore
	** Return.
	*/
	status = LockSamplerBlk(&hCsSamplerSem);
	if (status != OK)
	    return(status);
	
	/*
	** If the sampler is not running return.
	*/
	if (CsSamplerBlkPtr == NULL)
	{
	    TMnow(&time);
	}
	else
	{
	    time = CsSamplerBlkPtr->stoptime;

	    MEfree((PTR)CsSamplerBlkPtr);
     
     	    CsSamplerBlkPtr = NULL;
	}

	status = UnlockSamplerBlk(&hCsSamplerSem);

	CS_join_thread( CsSamplerThreadId,NULL );

	CS_synch_destroy( &hCsSamplerSem );

	SamplerInit = FALSE;

	TMstr(&time, timestr);
	if (output_fcn != NULL)
	    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	     	"Sampling is stopped. %s",
		timestr);

	return (status);

}  /* StopSampler */

/****************************************************************************
**
** Name: ShowSampler - Show the Sampler Data
**
** Description:
**      This is a local routine that presents the Sampler's data in a human
**      readable form, it the Sampler is running.                                                   
**
** Inputs:
**  output_fcn      Function to call to perform the output.
**                  This routine will be called as
**                      (*output_fcn)(NULL, length, buffer)
**                  where buffer is the length character
**                  output string
**
**  Returns:
**      OK
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
**  History:
**      27-sep-1995 (shero03)
**	    Show system mutex information by thread for the interesting threads
**	18-sep-1996 (canor01)
**	    Sampler thread can show number of user sessions and high-water
**	    mark for number of user sessions.
**	19-sep-1996 (canor01)
**	    Identify Replicator Queue thread.
**	15-nov-1996 (canor01)
**	    Identify LK Callback thread.
**	21-Nov-1996 (jenjo02)
**	    Identify Deadlock Detector thread.
**	16-Dec-1996 (shero03)
**	    Add Lock Wait Analysis section
**	31-Jan-1997 (wonst02)
**	    Add CS_tls hash table section.
**	11-apr-1997 (canor01)
**	    Make CS_tls statistics dependent on inclusion of local tls
**	    functions.
**	06-Jun-1997 (shero03)
**	    Print out the top Mutex and Lock entries.
**	14-Oct-1997 (shero03)
**	    Print out the rest of the Mutex and Lock entries in one bucket.
**	30-dec-1997 (canor01)
**	    perct array must hold MAXTHREADS+1.
**	08-Jul-1998 (shero03)
**	    Add License thread
**	25-Sep-1998 (jenjo02)
**	    Added sampler display of server CPU time and wait counts.
**	    Helps to see if threads are making progress or are "stuck".
**	04-Apr-2000 (jenjo02)
**	    Added getrusage stats to sampler output, when available.
**	29-Sep-2000 (jenjo02)
**	    Sampler improvements: break out event wait percentages
**	    by thread type, show DB id in lock id, track and
**	    display I/O and transaction rates, fix mutex and lock
**	    wait logic. Lump all non-user lock waits into one bucket;
**	    "system" threads should never wait on locks.
**
****************************************************************************/

static STATUS
ShowSampler(
	TR_OUTPUT_FCN *output_fcn )

{
# define MAXITERATIONS (max(max(MAXEVENTS + 1,MAXSAMPTHREADS+1),MAXSTATES))
# define NUMTOP 10
	
    char	timestr[50];
    char	buf[132];
    char	lockname[256];
    f8	perct[MAXITERATIONS];
    f8	topval[NUMTOP];
    i4	topindx[NUMTOP];
    i4	restsamples;
    i4	samples, totsamples;
    i4	seconds;
    i4	i, j, k, temp;
    f8	perct_mutex, perct_lock;
    STATUS  status = OK;

#ifdef xCL_003_GETRUSAGE_EXISTS
    struct rusage ru;
    struct rusage *rs;
#endif /* xCL_003_GETRUSAGE_EXISTS */
	    

    if (CsSamplerBlkPtr == NULL)
    {
	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
		 "There is no sampling now");
	return(FAIL);
    }

    /*
    ** Lock the Sampler Block
    */
    status = LockSamplerBlk(&hCsSamplerSem);
    if (status != OK)
	return(status);

    if (CsSamplerBlkPtr == NULL)
    {
	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
		 "There is no sampling now");

	status = UnlockSamplerBlk(&hCsSamplerSem);
	return(FAIL);
    }

#ifdef xCL_003_GETRUSAGE_EXISTS
    /* Get server resource usage since sampling started */
    getrusage(RUSAGE_SELF, &ru);
#endif /* xCL_003_GETRUSAGE_EXISTS */

    /*
    ** Print the start time and sampling interval
    */
    TMstr(&CsSamplerBlkPtr->starttime, timestr);
    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "Started at %s, sampling interval %dms.",
	    timestr, CsSamplerBlkPtr->interval);

    TMnow(&CsSamplerBlkPtr->stoptime);
    TMstr(&CsSamplerBlkPtr->stoptime, timestr);

    seconds = CsSamplerBlkPtr->stoptime.TM_secs - 
	      CsSamplerBlkPtr->starttime.TM_secs;

    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "Current Time %s.",
	    timestr);

    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "Total sample intervals %d.",
	    CsSamplerBlkPtr->numsamples);

    totsamples = CsSamplerBlkPtr->totusersamples
	       + CsSamplerBlkPtr->totsyssamples;

    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "Total User Thread samples %d.",
	    CsSamplerBlkPtr->totusersamples);

    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "Total System Thread samples %d",
	    CsSamplerBlkPtr->totsyssamples);

    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "Total Active User Threads %d, Active High Water Mark %d",
	    CS_num_active(),
	    Cs_srv_block.cs_hwm_active);

    /* Update Server cpu time so we have something to show */
    CS_update_cpu((i4 *)0, (i4 *)0);
    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "Server CPU seconds %d.%d",
	    (Cs_srv_block.cs_cpu - CsSamplerBlkPtr->startcpu) / 100,
	    (Cs_srv_block.cs_cpu - CsSamplerBlkPtr->startcpu) % 100);

    /* 
    ** For each thread type tell how much time was spent in each state
    */

    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "Thread State Analysis:            ----------- EventWait ------------");
    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "Thread Name          Computable   BIO   DIO   LIO   LOG  LOCK  Other  MutexWait  Other");

    perct_mutex = perct_lock = 0.0;
    
    for (i = -1; i < MAXSAMPTHREADS; i++)
    {
	 samples = CsSamplerBlkPtr->Thread[i].numthreadsamples;
	 if (samples > 0)
	 {
	    for (j = 0, perct[0] = 0.0; j < MAXSTATES; j++)
	    {
		switch ( j )
		{
		    case CS_COMPUTABLE:
		    case CS_EVENT_WAIT:
		    case CS_MUTEX:
			perct[j] = (100.0 * CsSamplerBlkPtr->Thread[i].state[j] )
				    / (f8) samples;
			break;
		    default:
			perct[0] += (100.0 * CsSamplerBlkPtr->Thread[i].state[j] )
				    / (f8) samples;
		}
	    }

	    perct_mutex += perct[CS_MUTEX];
	    perct_lock  += 
		    (100.0 * CsSamplerBlkPtr->Thread[i].evwait[EV_LOCK])
				    / (f8) samples;

	    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
		    "%20w %10.1f %5.1f %5.1f %5.1f %5.1f %5.1f %6.1f %10.1f %6.1f",
		    "AdminThread,UserThread,MonitorThread,FastCommitThread,\
WriteBehindThread,ServerInitThread,EventThread,2PCThread,RecoveryThread,LogWriter,\
CheckDeadThread,GroupCommitThread,ForceAbortThread,AuditThread,CPTimerThread,CheckTermThread,\
SecauditWriter,WriteAlongThread,ReadAheadThread,ReplicatorQueueMgr,\
LKCallbackThread,LKDeadlockThread,SamplerThread,SortThread,FactotumThread,LicenseThread,Invalid",
		    i+1,
		    perct[CS_COMPUTABLE], 
		    (100.0 * CsSamplerBlkPtr->Thread[i].evwait[EV_BIO])
				    / (f8) samples,
		    (100.0 * CsSamplerBlkPtr->Thread[i].evwait[EV_DIO])
				    / (f8) samples,
		    (100.0 * CsSamplerBlkPtr->Thread[i].evwait[EV_LIO])
				    / (f8) samples,
		    (100.0 * CsSamplerBlkPtr->Thread[i].evwait[EV_LOG])
				    / (f8) samples,
		    (100.0 * CsSamplerBlkPtr->Thread[i].evwait[EV_LOCK])
				    / (f8) samples,
		    perct[CS_EVENT_WAIT],
		    perct[CS_MUTEX], 
		    perct[0]);
	}
    }

    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "--------------------------------------------------------------------------------------");

    /*
    ** For user/system threads, tell how much computable time was spent in each facility
    */

    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "Computing Analysis:");

    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "Facility      User  System");

    for (i = 0; i < MAXFACS; i++)
    {

	for (j = -1, perct[1] = 0.0; j < MAXSAMPTHREADS; j++)
	{

	    /* Only look at the number of computable samples */
	    samples = CsSamplerBlkPtr->Thread[j].state[1];
	    if (samples > 0)
	    {
		if (j == CS_NORMAL)
		    perct[0] = (100.0 * CsSamplerBlkPtr->Thread[j].facility[i] )
				    / (f8) CsSamplerBlkPtr->totusersamples;
		else
		{
		    perct[1] += (100.0 * CsSamplerBlkPtr->Thread[j].facility[i] )
				    / (f8) CsSamplerBlkPtr->totsyssamples;
		}
	    }
	}

	if (perct[0] > 0 || perct[1] > 0)
	{

	    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
		    "%8w    %6.1f  %6.1f",
		    "<None>,CLF,ADF,DMF,OPF,PSF,QEF,QSF,RDF,SCF,ULF,DUF,GCF,RQF,TPF,GWF,SXF",
		    i,
		    perct[0], perct[1]);
	}
    }
    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "--------------------------");

    /*
    ** For each Mutex Wait, tell how much time was spent waiting on each.
    ** Only look at mutexes if mutex wait time is more than 1%.
    ** First sort the top 10 mutexes.
    */	
    
    if ( perct_mutex > 1.0 )
    {
	for (k = 0; k < NUMTOP; k++)
	    topval[k] = 0.0;
	restsamples = 0;

	for (i = 0; i < MAXMUTEXES+1; i++)
	{
	    if (CsSamplerBlkPtr->Mutex[i].name[0] == EOS)
		continue;

	    for (j = -1, temp = 0; j < MAXSAMPTHREADS; j++)
		temp += CsSamplerBlkPtr->Mutex[i].count[j];

	    if (temp == 0)
		continue;
	    
	    if (temp > topval[NUMTOP-1])	/* is this in top 10 ? */
	    {
		for (k = 0; k < NUMTOP-2; k++)
		{
		    if (temp > topval[k])
		    {
			/* Add the last one to the "rest" */
			if (topval[NUMTOP-1] > 0.0)
			   restsamples++;

			/* Any values below here? */
			if ((k < NUMTOP - 1) &&
			    (topval[k] > 0.0))
			{
			    /* Yes, shift lower ones down */
			    for (j = NUMTOP - 1; j >= k; j--)
			    {
				topval[j] = topval[j-1];
				topindx[j] = topindx[j-1];
			    }
			}
			break;
		    }
		}

	      /* Save this value in top 10 */
	      topval[k] = temp;
	      topindx[k] = i;
	    }
	    else
	    {
	      /* Save this with the "rest" */
	      restsamples++;
	    }
	}

	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "Top Mutexes Analysis:");

	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "Mutex Name                       ID        User  Fast  Write   Log  Group  Sort  Other");

	for (k = 0; k < NUMTOP - 1; k++)
	{
	    if (topval[k] == 0)
		break;

	    i = topindx[k];

	    for (j = -1, perct[0] = 0.0; j < MAXSAMPTHREADS; j++)
	    {
		temp = CsSamplerBlkPtr->Mutex[i].count[j];
		if ( temp > 0 )
		    perct_mutex -= (100.0 * temp)
			 / (f8) CsSamplerBlkPtr->Thread[j].numthreadsamples;
		switch (j)
		{
		    case CS_NORMAL:
		    case CS_FAST_COMMIT:
		    case CS_WRITE_BEHIND:
		    case CS_LOGWRITER:
		    case CS_GROUP_COMMIT:
		    case CS_SORT:
			if (temp > 0)
			{
			    perct[j+1] = (100.0 * temp)
			     / (f8) CsSamplerBlkPtr->Thread[j].numthreadsamples;
			}
			else
			    perct[j+1] = 0.0;
			break;
		    default:
			if (temp > 0)
			    perct[0] += (100.0 * temp)
			     / (f8) CsSamplerBlkPtr->Thread[j].numthreadsamples;
		}
	    }

	    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
		    "%32s %p %5.1f %5.1f %6.1f %5.1f %6.1f %5.1f %6.1f",
		    CsSamplerBlkPtr->Mutex[i].name,
		    CsSamplerBlkPtr->Mutex[i].id,
		    perct[CS_NORMAL+1], 
		    perct[CS_FAST_COMMIT+1], 
		    perct[CS_WRITE_BEHIND+1], 
		    perct[CS_LOGWRITER+1], 
		    perct[CS_GROUP_COMMIT+1], 
		    perct[CS_SORT+1],
		    perct[0]);
	}

	if ( restsamples )
	   TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
		    "The other %d mutex(es) accounted for %.1f%%",
		    restsamples, perct_mutex );

	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "--------------------------------------------------------------------------------------");

    }  /* Mutex Waits */
    else
    {
	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "Mutex Waits are less than 1%%");
	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "----------------------------");
    }

    /*
    ** For each Lock Wait, tell how much time was spent waiting on each.
    ** Only look at locks if lock wait time is > 1%.
    ** First sort the top 10 locks.
    */	
    
    if ( perct_lock > 1.0 )
    {
	for (k = 0; k < NUMTOP; k++)
	    topval[k] = 0.0;
	restsamples = 0;

	for (i = 0; i < MAXLOCKS+1; i++)
	{

	    for (j = -1, temp = 0; j < MAXSAMPTHREADS; j++)
		temp += CsSamplerBlkPtr->Lock[i].count[j];
		
	    if (temp == 0)
		continue;
	    
	    if (temp > topval[NUMTOP-1])	/* is this in top 10 ? */
	    {
		for (k = 0; k < NUMTOP-2; k++)
		{
		    if (temp > topval[k])
		    {
			/* Add the last one to the "rest" */
			if (topval[NUMTOP-1] > 0.0)
			    restsamples++;

			/* Any values below here? */
			if ((k < NUMTOP - 1) &&
			    (topval[k] > 0.0))
			{
			    /* Yes, shift lower ones down */

			    for (j = NUMTOP - 1; j >= k; j--)
			    {
				topval[j] = topval[j-1];
				topindx[j] = topindx[j-1];
			    }
			}
		    break;
		    }
		}

		/* Save this value in top 10 */
		topval[k] = temp;
		topindx[k] = i;
	    }
	    else
	    {
	      /* Save this with the "rest" */
	      restsamples++;
	    }
	}	 
	
	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "Top Locks Analysis:");
	
	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "Resource Type, Name                                                User System");

	for (k = 0; k < NUMTOP - 1; k++)
	{
	    if (topval[k] == 0)
		break;

	    i = topindx[k];

	    /* perct[0] holds "user" pct, [1] "system" */
	    perct[0] = perct[1] = 0.0;

	    for (j = -1; j < MAXSAMPTHREADS; j++)
	    {
		temp = CsSamplerBlkPtr->Lock[i].count[j];
		if ( temp > 0 )
		{
		    perct_lock -= (100.0 * temp)
			 / (f8) CsSamplerBlkPtr->Thread[j].numthreadsamples;

		    if ( j == CS_NORMAL )
			perct[0] = (100.0 * temp)
			 / (f8) CsSamplerBlkPtr->Thread[j].numthreadsamples;
		    else
			perct[1] += (100.0 * temp)
			 / (f8) CsSamplerBlkPtr->Thread[j].numthreadsamples;
		}
	    }

	    (void) (*Cs_srv_block.cs_format_lkkey)(&CsSamplerBlkPtr->Lock[i].lk_key,
	      lockname);
	    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
		"%64s  %5.1f  %5.1f",
		lockname, perct[0], perct[1]);
	}

	if ( restsamples )
	   TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
		    "The other %d lock(s) accounted for %.1f%%",
		    restsamples, perct_lock);

	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "------------------------------------------------------------------------------");


    }  /* Lock Waits */
    else
    {
	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "Lock Waits are less than 1%%");
	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "---------------------------");
    }


    /*
    ** For all System Events tell how much time was spent waiting on each type
    */
    if (CsSamplerBlkPtr->numsyseventsamples +
	CsSamplerBlkPtr->numusereventsamples > 0)
    {
	u_i4	bior_waits = Cs_srv_block.cs_wtstatistics.cs_bior_waits - 
				    CsSamplerBlkPtr->waits.cs_bior_waits;
	u_i4	bior_time  = Cs_srv_block.cs_wtstatistics.cs_bior_time - 
				    CsSamplerBlkPtr->waits.cs_bior_time;
	u_i4	biow_waits = Cs_srv_block.cs_wtstatistics.cs_biow_waits - 
				    CsSamplerBlkPtr->waits.cs_biow_waits;
	u_i4	biow_time  = Cs_srv_block.cs_wtstatistics.cs_biow_time - 
				    CsSamplerBlkPtr->waits.cs_biow_time;
	u_i4	dior_waits = Cs_srv_block.cs_wtstatistics.cs_dior_waits - 
				    CsSamplerBlkPtr->waits.cs_dior_waits;
	u_i4	dior_time  = Cs_srv_block.cs_wtstatistics.cs_dior_time - 
				    CsSamplerBlkPtr->waits.cs_dior_time;
	u_i4	diow_waits = Cs_srv_block.cs_wtstatistics.cs_diow_waits - 
				    CsSamplerBlkPtr->waits.cs_diow_waits;
	u_i4	diow_time  = Cs_srv_block.cs_wtstatistics.cs_diow_time - 
				    CsSamplerBlkPtr->waits.cs_diow_time;
	u_i4	lior_waits = Cs_srv_block.cs_wtstatistics.cs_lior_waits - 
				    CsSamplerBlkPtr->waits.cs_lior_waits;
	u_i4	lior_time  = Cs_srv_block.cs_wtstatistics.cs_lior_time - 
				    CsSamplerBlkPtr->waits.cs_lior_time;
	u_i4	liow_waits = Cs_srv_block.cs_wtstatistics.cs_liow_waits - 
				    CsSamplerBlkPtr->waits.cs_liow_waits;
	u_i4	liow_time  = Cs_srv_block.cs_wtstatistics.cs_liow_time - 
				    CsSamplerBlkPtr->waits.cs_liow_time;
	u_i4	lg_waits  = Cs_srv_block.cs_wtstatistics.cs_lg_waits - 
				    CsSamplerBlkPtr->waits.cs_lg_waits;
	u_i4	lg_time   = Cs_srv_block.cs_wtstatistics.cs_lg_time - 
				    CsSamplerBlkPtr->waits.cs_lg_time;
	u_i4	lge_waits = Cs_srv_block.cs_wtstatistics.cs_lge_waits - 
				    CsSamplerBlkPtr->waits.cs_lge_waits;
	u_i4	lge_time  = Cs_srv_block.cs_wtstatistics.cs_lge_time - 
				    CsSamplerBlkPtr->waits.cs_lge_time;
	u_i4	lk_waits  = Cs_srv_block.cs_wtstatistics.cs_lk_waits - 
				    CsSamplerBlkPtr->waits.cs_lk_waits;
	u_i4	lk_time   = Cs_srv_block.cs_wtstatistics.cs_lk_time - 
				    CsSamplerBlkPtr->waits.cs_lk_time;
	u_i4	lke_waits = Cs_srv_block.cs_wtstatistics.cs_lke_waits - 
				    CsSamplerBlkPtr->waits.cs_lke_waits;
	u_i4	lke_time  = Cs_srv_block.cs_wtstatistics.cs_lke_time - 
				    CsSamplerBlkPtr->waits.cs_lke_time;

	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "Event Wait Analysis:");

	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	"          DIOR  DIOW  LIOR  LIOW  BIOR  BIOW   Log  Lock LGEvnt LKEvnt");
/*      "Time(ms)  1000  1000  1000  1000  1000  1000  1000  1000   1000   1000 */
/*      "  System 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0  100.0  100.0 */
		    
	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	"Time(ms)  %4d  %4d  %4d  %4d  %4d  %4d  %4d  %4d   %4d   %4d",
	    (dior_waits) ? dior_time / dior_waits : 0,
	    (diow_waits) ? diow_time / diow_waits : 0,
	    (lior_waits) ? lior_time / lior_waits : 0,
	    (liow_waits) ? liow_time / liow_waits : 0,
	    (bior_waits) ? bior_time / bior_waits : 0,
	    (biow_waits) ? biow_time / biow_waits : 0,
	    (lg_waits)  ? lg_time  / lg_waits   : 0,
	    (lk_waits)  ? lk_time  / lk_waits   : 0,
	    (lge_waits) ? lge_time / lge_waits  : 0,
	    (lke_waits) ? lke_time / lke_waits  : 0);

	samples = CsSamplerBlkPtr->numsyseventsamples;
	if (samples > 0)
	{

	    for (j = 0; j < MAXEVENTS + 1; j++)
	    {
		perct[j] = (100.0 * CsSamplerBlkPtr->sysevent[j] )
			    / (f8) CsSamplerBlkPtr->totsyssamples;
	    }

	    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "  System %5.1f %5.1f %5.1f %5.1f %5.1f %5.1f %5.1f %5.1f  %5.1f  %5.1f",
	    perct[0], 
	    perct[1], perct[2], perct[3], perct[4], perct[5], perct[6],
	    perct[7], perct[8], perct[9]);
	}  /* System Events */
	    
	samples = CsSamplerBlkPtr->numusereventsamples;
	if (samples > 0)
	{

	    for (j = 0; j < MAXEVENTS + 1; j++)
	    {
		perct[j] = (100.0 * CsSamplerBlkPtr->userevent[j] )
			    / (f8) CsSamplerBlkPtr->totusersamples;
	    }

	    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "    User %5.1f %5.1f %5.1f %5.1f %5.1f %5.1f %5.1f %5.1f  %5.1f  %5.1f",
	    perct[0], 
	    perct[1], perct[2], perct[3], perct[4], perct[5], perct[6],
	    perct[7], perct[8], perct[9]);

	}  /* User Events */

	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
        "----------------------------------------------------------------------");
    }  /* either System or User Events */
    
	
    /* I/O and Transaction rates */

    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	"I/O and Transaction Rates/Sec:");
    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	"          DIOR....kb  DIOW....kb  LIOR....kb  LIOW....kb  BIOR  BIOW   Txn");
/*        Current 10000 10000 10000 10000 10000 10000 10000 10000 10000 10000 10000 */
    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	" Current %5d %5d %5d %5d %5d %5d %5d %5d %5d %5d %5d",
	    CsSamplerBlkPtr->dior[CURR], CsSamplerBlkPtr->diork[CURR],
	    CsSamplerBlkPtr->diow[CURR], CsSamplerBlkPtr->diowk[CURR],
	    CsSamplerBlkPtr->lior[CURR], CsSamplerBlkPtr->liork[CURR],
	    CsSamplerBlkPtr->liow[CURR], CsSamplerBlkPtr->liowk[CURR],
	    CsSamplerBlkPtr->bior[CURR], CsSamplerBlkPtr->biow[CURR],
	    CsSamplerBlkPtr->txn[CURR]);
    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	"    Peak %5d %5d %5d %5d %5d %5d %5d %5d %5d %5d %5d",
	    CsSamplerBlkPtr->dior[PEAK], CsSamplerBlkPtr->diork[PEAK],
	    CsSamplerBlkPtr->diow[PEAK], CsSamplerBlkPtr->diowk[PEAK],
	    CsSamplerBlkPtr->lior[PEAK], CsSamplerBlkPtr->liork[PEAK],
	    CsSamplerBlkPtr->liow[PEAK], CsSamplerBlkPtr->liowk[PEAK],
	    CsSamplerBlkPtr->bior[PEAK], CsSamplerBlkPtr->biow[PEAK],
	    CsSamplerBlkPtr->txn[PEAK]);

    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	"--------------------------------------------------------------------------");

    
# ifndef xCL_094_TLS_EXISTS
    /*
    ** Hash table analysis for CS tls hash table
    */
    samples = CsSamplerBlkPtr->numtlssamples;
    if (samples > 0)
    {
	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "CS TLS Hash Table Analysis:");
	perct[0] = (f8) CsSamplerBlkPtr->numtlsslots 
		 / (f8) CsSamplerBlkPtr->numsamples;
	perct[1] = (f8) CsSamplerBlkPtr->numtlsprobes / (f8) samples;
	perct[2] = (100.0 * CsSamplerBlkPtr->numtlsdirty) / (f8) samples;
	perct[3] = (f8) CsSamplerBlkPtr->numtlsreads / (f8) seconds;
	perct[4] = (f8) CsSamplerBlkPtr->numtlswrites / (f8) seconds;
	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
"Avg. slots used %6.1f   Avg. reads per sec. %6.0f    Pct. dirty reads %5.3f",
		    perct[0], perct[3], perct[2]);
	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
"Avg. hash probes %5.3f   Avg. writes per sec. %5.3f",
		    perct[1], perct[4]);
    }  /* CS tls hash samples */
    else
	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "No CS TLS Samples");

    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
"-----------------------------------------------------------------------------");
# endif /* xCL_094_TLS_EXISTS */

#ifdef xCL_003_GETRUSAGE_EXISTS
    rs = &CsSamplerBlkPtr->ruse;

    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "getrusage() measurements of this process since sampling started:");

    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
    "    UTIME    STIME  MAXRSS   IDRSS  MINFLT  MAJFLT   NSWAP INBLOCK OUBLOCK");
    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
    "%9.3f%9.3f%8d%8d%8d%8d%8d%8d%8d",
	(f8)(((f8)((ru.ru_utime.tv_sec - rs->ru_utime.tv_sec) * MICRO_PER_SEC)
	 + (f8)(ru.ru_utime.tv_usec - rs->ru_utime.tv_usec))
	 / (f8)MICRO_PER_SEC),
	(f8)(((f8)((ru.ru_stime.tv_sec - rs->ru_stime.tv_sec) * MICRO_PER_SEC)
	 + (f8)(ru.ru_stime.tv_usec - rs->ru_stime.tv_usec))
	 / (f8)MICRO_PER_SEC),
	ru.ru_maxrss,
	ru.ru_idrss,
	ru.ru_minflt - rs->ru_minflt,
	ru.ru_majflt - rs->ru_majflt,
	ru.ru_nswap  - rs->ru_nswap,
	ru.ru_inblock - rs->ru_inblock,
	ru.ru_oublock - rs->ru_oublock);

    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
    "   MSGSND   MSGRCV   NSIGS   NVCSW  NIVCSW");
    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
    " %8d %8d%8d%8d%8d",
	ru.ru_msgsnd - rs->ru_msgsnd,
	ru.ru_msgrcv - rs->ru_msgrcv,
	ru.ru_nsignals - rs->ru_nsignals,
	ru.ru_nvcsw - rs->ru_nvcsw,
	ru.ru_nivcsw - rs->ru_nivcsw);

    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
"--------------------------------------------------------------------------");
#endif /* xCL_003_GETRUSAGE_EXISTS */

    /* 
    ** Unlock the Sampler Block
    ** return
    */
    status = UnlockSamplerBlk(&hCsSamplerSem);

    return (status);
}  /* ShowSampler */

/****************************************************************************
**
** Name: LockSamplerBlk - Lock the Sampler Block so no other threads can read or write it.
**
** Description:
**      This is a local cs routine that locks the Sampler's data so there
**      are no other reader or writers.                                                   
**	If the lock has not been established, then build one.
**	Otherwise, wait for anyone who has the lock. If noone then it is a short wait.
**
** Inputs:
**
**  Returns:
**      OK
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
****************************************************************************/

STATUS
LockSamplerBlk( CS_SYNCH *hpLockSem )

{

	/*
	** If the semaphore is unitialized, then create a new one with us owning it.
	*/	
	if (! SamplerInit)
	{	
	    SamplerInit = TRUE;
	    CS_synch_init( hpLockSem );
	}

	/*
	** Wait for the semaphore, if some thread has it
	*/
	CS_synch_lock( hpLockSem );

	return (OK);

} /* LockSamplerBlk */


/****************************************************************************
**
** Name: UnlockSamplerBlk - Unlock the Sampler Block so other threads can read or write it.
**
** Description:
**      This is a local cs routine that unlocks the Sampler's data so the other
**      threads can access it.                                                   
**
** Inputs:
**
**  Returns:
**      OK
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
****************************************************************************/

STATUS
UnlockSamplerBlk( CS_SYNCH *hpLockSem )

{
	STATUS  status = OK;
		
	CS_synch_unlock( hpLockSem );

	return (status);

} /* UnlockSamplerBlk */

# endif /* OS_THREADS_USED */
