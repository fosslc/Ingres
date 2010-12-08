/*
**Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <st.h>
# include <clconfig.h>
# include <systypes.h>
# include <fdset.h>
# include <clsigs.h>
# include <cs.h>
# include <er.h>
# include <ex.h>
# include <me.h>
# include <pc.h>
# include <tm.h>
# include <tr.h>
# include <nm.h>
# include <me.h>
# include <si.h>
# include <cv.h>
# include <sp.h>
# include <mo.h>
# include <lk.h>


# include <csinternal.h>
# include <csev.h>
# include <cssminfo.h>
# include <rusage.h>
# include <clpoll.h>
# include <machconf.h>
# include <pwd.h>

# include "csmgmt.h"

# include "cslocal.h"
# include "cssampler.h"

/*
**
**  Name: CSMONITOR.C - CS Monitor functions.
**
**  Description:
**      This module contains those routines which provide support for
**	CS monitoring under INTERNAL threads, plus support functions
**	used both by INTERNAL and OS threads.
**
**	    IICSmonitor		- main function.
**	    CS_null_handler	- No-op exception handler to catch bad input.
**	    CS_verify_sid	- Parse and verify user provided SID.
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
**	15-aug-2002 (devjo01)
**	    Use %p when displaying session ID for "stack".
**	04-sep-2002 (devjo01)
**	    Add diagnostic registration stuff.
**	31-Jan-2007 (kiria01) b117581
**	    Use global cs_mask_names and cs_state_names.
**	11-Nov-2010 (kschendel) SIR 124685
**	    Delete CS_SYSTEM ref, get from csinternal.
**	    Fix confusion re output-fcn, it's a TR_OUTPUT_FCN not something
**	    else.  Drop the goofy add-a-newline arg.  TRformat always drops
**	    trailing and doubled newlines, normalize the calls here.
*/


GLOBALREF i4                    Ex_core_enabled;

GLOBALREF CS_SEMAPHORE		Cs_known_list_sem;

/*
** SAMPLER globals and externs
*/
GLOBALDEF CSSAMPLERBLKPTR	CsSamplerBlkPtr ZERO_FILL;

static STATUS StartSampler(
                char    *command,
                TR_OUTPUT_FCN	*output_fcn );

static STATUS StopSampler(
                TR_OUTPUT_FCN	*output_fcn );

static STATUS ShowSampler(
                TR_OUTPUT_FCN	*output_fcn );

static void show_sessions(
    char    *command,
    TR_OUTPUT_FCN *output_fcn,
    i4	    formatted,
    i4	    powerful);

static void test_segv(char *command, TR_OUTPUT_FCN *output_fcn, i4 depth );

static void format_sessions(char *command, TR_OUTPUT_FCN *output_fcn, i4 powerful);

/*
** Name: CS_null_handler	- No-op exception handler.
**
** Description:
**	This handler catches exceptions presumably triggered by bad
**	user provided address values.   It performs no error reporting,
**	but simply returns EXDECLARE to allow handle declarer to 
**	process error.
**
** Inputs:
**	exargs		EX arguments (this is an EX handler)
** Outputs:
**	Returns:
**	    EXDECLARE
**
** Side Effects:
**	    none.
**
** History:
**	22-jan-2002 (devjo01)
**	    Moved and renamed.  Was csmt_monitor_handler in csmtmonitor.c.
*/
STATUS
CS_null_handler(EX_ARGS *exargs)
{
    return(EXDECLARE);
} /* CS_null_handler */


/*{
** Name: CS_verify_sid - Parse & verify user provided SID.
**
** Description:
**	This routine extracts a SID from a command string, then
**	converts it to a CS_SCB *, while guarding against a bad
**	address by establishing a local exception handler.
**
** Inputs:
**	action				Action to be performed on target.
**	command				command string prefixed by "action",
**					and containing SID as next field.
**	output_fcn			Function to call to perform the output.
**	nointernal			If non-zero, disallow action against
**					certain internal sessions.
**
** Outputs:
**	none
**	Returns:
**	    Pointer to SCB or NULL.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	22-jan-2002 (devjo01)
**	    Initial version.
*/
CS_SCB	*
CS_verify_sid( char *action, char *command, TR_OUTPUT_FCN *output_fcn,
	i4 nointernal )
{
    EX_CONTEXT		 ex_context;
    CS_SCB		*scb = NULL;
    PTR			 ptr_tmp;
    char		 buf[80];

    STzapblank(command, command);
    if (OK == CVaxptr(command + STlength(action), &ptr_tmp))
    {
	/* Install handler to catch bad user SID ID */
	if (OK == EXdeclare(CS_null_handler, &ex_context))
	    scb = CS_find_scb( (CS_SID)ptr_tmp );
	EXdelete();
    }

    if ( NULL == scb )
    {
	TRformat(output_fcn, NULL, buf, sizeof(buf) - 1, "Invalid session id");
    }
    else if ( nointernal &&
	      ( (scb == (CS_SCB *)&Cs_idle_scb) ||
	        (scb == (CS_SCB *)&Cs_admin_scb) ) )
    {
	TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		"Cannot %s internal session %p.", action, scb->cs_self);
	scb = NULL;
    }
    return scb;
} /* CS_verify_sid */



/*{
** Name: CSmonitor	- Implement the monitor task
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
**					output string.
**					Although a standard "output function"
**					accepts a pointer arg, we don't use it.
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
**      18-feb-1997 (hanch04)
**          As part of merging of Ingres-threaded and OS-threaded servers,
** 	    check for cs_mt.
**	7-jan-98 (stephenb)
**	    Format the output for "show sessions" better and add the 
**	    active users highwater mark in preparation for user based licensing
**	8-jan-98 (stephenb)
**	    switch high water mark in above chage to appear after max active
**	    so that IPM is not confused, also break up long displya line
**	    to pervent piccolo from baulking.
**	02-Jun-1998 (jenjo02)
**	    Show session id (cs_self), not &scb, in "Session <sid> is an internal task"
**	    messages consistently.
**	05-Oct-1998 (jenjo02)
**	    Added "show <...> sessions stats" to act as a visual aid
**	    to see what, if any, progress the sessions are
**	    making in their quest.
**	    Added server CPU time and counts of waits by type to the
**	    sampler output.
**	16-Nov-1998 (jenjo02)
**	    Cleaned up "sessions stats" to show only non-zero counts.
**	    New event wait masks added for Log I/O, I/O read/write.
**	    Incorporated server wait statistics into Event Wait
**	    Analysis graph.
**	21-jan-1999 (canor01)
**	    Remove erglf.h
**	01-Feb-1999 (jenjo02)
**	    Repaired computation of log/lock event wait times.
**	25-May-1999 (shero03)
**	    Add new facilities: URS, ICE, and CUF
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	14-apr-99 (devjo01)
**	    Add code to allow easy test of SEGV/SIGBUS stack trace.
**	    New "test11" command provided, and the frame for other
**	    testxx commands put in place.
**	03-jul-99 (devjo01)
**          Show CS_DIZZY_MASK bit is set when displaying a session.
**	    Include new SHOW MUTEX logic coded by wanya01, plus hooks
**	    to enable core generation. 
**	15-Nov-1999 (jenjo02)
**	    Removed cs_sem_init_addr from CS_SEMAPHORE.
**      08-Dec-1999 (hanal04) Bug 95607 INGSRV 736
**          Exclusively held cross process semaphores do not have cs_value
**          set. Check cs_pid for CS_SEM_MULTI semaphores in the SHOW MUTEX
**          command code.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Oct-2000 (jenjo02)
**	    Renamed to IICSmonitor, removed cs_mt test.
**	    Deleted FUNC_EXTERN for CS_find_scb(), which is
**	    prototyped in cslocal.h
**	22-Feb-2002 (devjo01)
**	    Use CS_verify_sid to parse and validate
**	    user passed SID ident.
**	19-Apr-2002 (devjo01)
**	    Consistently use %d for pid, and %p for session ID.
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
**	15-aug-2002 (devjo01)
**	    Use %p when displaying session ID for "stack".
**	24-Apr-2003 (jenjo02)
**	    Added "kill query" to help syntax, SIR 110141.
**	04-sep-2002 (devjo01)
**	    Allow other facilities to register their own debug
**	    facilities.  Added IICSmon_register, IICSmon_deregister. 
**	04-Oct-2007 (bonro01)
**	    Fix SEGV from "debug <sid>" command.
**      23-Apr-2009 (hanal04) Bug 132714
**          Update iimonitor help output to include SAMPLING commands.
**      23-Sep-2009 (hanal04) Bug 115316
**          Added "SHOW SERVER CAPABILITIES".
**	29-Jan-2010 (smeke01) Bug 123192
**	    For 'show smstats' and 'show mutex', display u_i4 values with 
**	    format specifier %10u rather than %8d. 
**	16-Nov-2010 (kschendel) SIR 124685
**	    Low level routine (CS_dump_stack) needs to be able to really use
**	    the arg param to an output_fcn, so we need to stop with the
**	    misuse of that param as a newline flag.  The effect trickles all
**	    the way up.
*/
STATUS
IICSmonitor(i4 mode, CS_SCB *temp_scb, i4 *nmode,
	char *command, i4 powerful, TR_OUTPUT_FCN *output_fcn)
{
    STATUS		status;
    i4			count;
    char		buf[81];
    EX_CONTEXT		ex_context;
    EX_CONTEXT          ex_context1;
    CS_SCB		*an_scb;
    CS_SCB		*scan_scb;
    CS_SEMAPHORE	*mutex;
    CS_MNTR_SCB		*scb;
    PTR			ptr_tmp;
    char                *temp ;
    i4			badcmnd = 0;

    temp_scb->cs_thread_type = CS_MONITOR;
    scb = (CS_MNTR_SCB *) temp_scb;
    *nmode = CS_EXCHANGE;
    if (EXdeclare(CS_null_handler, &ex_context))
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
		    status = CSterminate(CS_COND_CLOSE, &count);
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
		    status = CSterminate(CS_KILL, &count);
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
		    status = CSterminate(CS_CRASH, &count);
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
		    status = CSterminate(CS_CLOSE, &count);
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
		    break;
		}

                /* Include SEM_MULTI exclusive holders.
                */
                if ( mutex->cs_value ||
                     (mutex->cs_pid && (mutex->cs_type == CS_SEM_MULTI)))
		{
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"Mutex at %p: Name: %s, EXCL owner: (sid: %p, pid: %d) ",
				mutex,
		    mutex->cs_sem_name[0] ? mutex->cs_sem_name : "(UN-NAMED)",
				mutex->cs_owner,
				mutex->cs_pid);
		}
		else if ( mutex->cs_count )
		{
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"Mutex at %p: Name: %s, SHARE owner: (sid: %p, pid: %d), count: %d",
				mutex,
		    mutex->cs_sem_name[0] ? mutex->cs_sem_name : "(UN-NAMED)",
				mutex->cs_owner,
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
                EXdelete();
	    }
	    else if (STscompare("showserver", 0, command, 0) == 0)
	    {
		i4 num_sessions, num_active;

		num_sessions = Cs_srv_block.cs_num_sessions;
		/* Compute both cs_num_active, cs_hwm_active */
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
                STprintf(buffer, "    Exclusive requests:   %10u   Shared:  %10u   Multi-Process:  %10u\n",
                            Cs_srv_block.cs_smstatistics.cs_smx_count,
                            Cs_srv_block.cs_smstatistics.cs_sms_count,
                            Cs_srv_block.cs_smstatistics.cs_smc_count);
                STprintf(buffer, "    Exclusive collisions: %10u   Shared:  %10u   Multi-Process:  %10u\n",
                            Cs_srv_block.cs_smstatistics.cs_smxx_count,
                            Cs_srv_block.cs_smstatistics.cs_smcx_count);
                STprintf(buffer, "    Multi-Process Wait loops: %10u\n",
                            Cs_srv_block.cs_smstatistics.cs_smcl_count);
                STprintf(buffer, "    Multi-Process single-processor redispatches: %10u\n",
                            Cs_srv_block.cs_smstatistics.cs_smsp_count);
                STprintf(buffer, "    Multi-Process multi-processor redispatches: %10u\n",
                            Cs_srv_block.cs_smstatistics.cs_smmp_count);
                buffer += STlength(buffer);
                STprintf(buffer, "    Multi-Process non-server naps: %10u\n",
                            Cs_srv_block.cs_smstatistics.cs_smnonserver_count);
		buffer += STlength(buffer);
		STprintf(buffer, "--------- -----------\n");
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1, place);
	    }
	    else if (STscompare("clearsmstats", 12, command, 12) == 0)
	    {
                Cs_srv_block.cs_smstatistics.cs_smx_count = 0;
                Cs_srv_block.cs_smstatistics.cs_sms_count = 0;
                Cs_srv_block.cs_smstatistics.cs_smxx_count = 0;
                Cs_srv_block.cs_smstatistics.cs_smsx_count = 0;
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
		if ( NULL != ( an_scb =
		      CS_verify_sid("remove", command, output_fcn, 1) ) )
		{
		    if ((MEcmp(an_scb->cs_username,
				scb->csm_scb.cs_username,
				sizeof(scb->csm_scb.cs_username))) && !powerful)
		    {
			TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		      "Superuser or owner status required to remove sessions");
		    }
		    else
		    {
			if (CSremove((CS_SID)an_scb->cs_self) == OK)
			{
		            TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			        "Session %p removed", an_scb->cs_self);
                            CSattn(CS_REMOVE_EVENT, (CS_SID)an_scb->cs_self);
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
		if ( NULL == ( an_scb =
		      CS_verify_sid("suspend", command, output_fcn, 1) ) )
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
			TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			    "Session %p has been indefinitely suspended",
				an_scb->cs_self);
		    }
		}
	    }
	    else if (STscompare("resume", 6, command, 6) == 0)
	    {
		if ( NULL == ( an_scb =
		      CS_verify_sid("resume", command, output_fcn, 1) ) )
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
			TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			    "Session %p has been resumed",
				an_scb->cs_self);
		    }
		}
	    }
	    else if (STscompare("debug", 5, command, 5) == 0)
	    {
		if ( NULL == ( an_scb =
		      CS_verify_sid("debug", command, output_fcn, 0) ) )
		    break;

		{
		    char	buffer[1048];
		    CS_fmt_scb(an_scb,
				sizeof(buffer),
				buffer);
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1, buffer);
		}

		CSget_scb(&scan_scb);
		if (scan_scb != an_scb)
			CS_dump_stack(an_scb, NULL, NULL, output_fcn, FALSE);
		else
			CS_dump_stack(NULL, NULL, NULL, output_fcn, FALSE);
	    }
            else if (STscompare("stacks", 6, command, 6) == 0)
            {
		CSget_scb(&an_scb);
                for (scan_scb = Cs_srv_block.cs_known_list->cs_next;
                    scan_scb && scan_scb != Cs_srv_block.cs_known_list;
                    scan_scb = scan_scb->cs_next)
                {
			TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			    "\n---\nStack dump for session %p (%24s)",
			    scan_scb->cs_self, scan_scb->cs_username);
                        if (an_scb != scan_scb)
                                CS_dump_stack(scan_scb, NULL, NULL, output_fcn, FALSE);
                        else
                                CS_dump_stack(NULL, NULL, NULL, output_fcn, FALSE);
                }
            }
            else if (STscompare("enablecore", 10, command, 10) == 0)
            {
                Ex_core_enabled = 1;
            }
            else if (STscompare("disablecore", 11, command, 11) == 0)
            {
                Ex_core_enabled = 0;
            }
            else if (STscompare("testcore", 8, command, 11) == 0)
            {
              temp = (char *)1 ;
              *temp='a';
            }
	    else if (STscompare("setpriority", 11, command, 11) == 0)
	    {
		i4 priority;
                
		if ( (OK != CVan(command+11, &priority)) || 
		     (OK != CSaltr_session((CS_SID)0, CS_AS_PRIORITY, 
			    (PTR)&priority)) )	
		{
		     TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"Failed");
		}
            }
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
    RESUME [sid]\n\n");
                TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
                    "Query:\n     KILL [sid]\n\n");
                TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		    "Others:\n     QUIT\n\n");
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
**	05-Oct-1998 (jenjo02)
**	    Added "show <...> sessions stats" to act as a visual aid
**	    to see what, if any, progress the sessions are
**	    making in their quest. 
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

static void
show_sessions(
    char    *command,
    TR_OUTPUT_FCN *output_fcn,
    i4	    format_option,
    i4	    powerful)
{
    CS_SCB	*an_scb;
    char	buf[100];
    char        tbuf[64];
    i4		show_user;
    i4		show_system;
    i4		formatted = (format_option == 1) ? 1 : 0;
    i4		stats = (format_option == 2) ? 1 : 0;

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
    else    /* show all sessions or show all sessions formatted */
    {
	show_user = 1;
	show_system = 1;
    }

    for (an_scb = Cs_srv_block.cs_known_list->cs_next;
	an_scb && an_scb != Cs_srv_block.cs_known_list;
	an_scb = an_scb->cs_next)
    {
	if (show_system == 0 && 
            an_scb->cs_thread_type != CS_USER_THREAD &&
            an_scb->cs_thread_type != CS_MONITOR)
	    continue;

	if (show_user == 0 && 
	    (an_scb->cs_thread_type == CS_USER_THREAD ||
	     an_scb->cs_thread_type == CS_MONITOR))
	    continue;
	
	if (stats)
	{
	    char	tbuf[128];
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
	    if (an_scb->cs_cputime)
	        STprintf(tbuf, "CPU %d", an_scb->cs_cputime * 10),
		STcat(sbuf,tbuf);

	    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
	    	"Session %p (%24s) %s",
		an_scb->cs_self,
		an_scb->cs_username,
		sbuf);
	}
	else switch (an_scb->cs_state)
	{
	    case CS_COMPUTABLE:
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		    "Session %p (%24s) cs_state: %w (%d) cs_mask: %v",
			    an_scb->cs_self,
			    an_scb->cs_username,
			    cs_state_names,
			    an_scb->cs_state,
			    an_scb->cs_priority,
			    cs_mask_names,
			    an_scb->cs_mask); 
		if (an_scb->cs_mask & CS_MUTEX_MASK && 
		    an_scb->cs_sync_obj)
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"        Mutex: %s  ((%s) %p)",
			((CS_SEMAPHORE *)an_scb->cs_sync_obj)->cs_sem_name[0] ?
			((CS_SEMAPHORE *)an_scb->cs_sync_obj)->cs_sem_name :
			"(UN-NAMED)",
			"x",
			an_scb->cs_sync_obj);
		break;

	    case CS_FREE:
	    case CS_STACK_WAIT:
	    case CS_UWAIT:
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		    "Session %p (%24s) cs_state: %w cs_mask: %v",
			    an_scb->cs_self,
			    an_scb->cs_username,
			    cs_state_names,
			    an_scb->cs_state,
			    cs_mask_names,
			    an_scb->cs_mask); 
		break;

	    case CS_EVENT_WAIT: 
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		    "session %p (%24s) cs_state: %w (%s) cs_mask: %v",
			    an_scb->cs_self,
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
			    an_scb->cs_mask); 
		break;
		
	    case CS_MUTEX:
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		    "Session %p (%24s) cs_state: %w ((%s) %p) cs_mask: %v",
			    an_scb->cs_self,
			    an_scb->cs_username,
			    cs_state_names,
			    an_scb->cs_state,
			    "x",
			    an_scb->cs_sync_obj,
			    cs_mask_names,
			    an_scb->cs_mask); 
		if (an_scb->cs_sync_obj)
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"        Mutex: %s",
			((CS_SEMAPHORE *)an_scb->cs_sync_obj)->cs_sem_name[0] ?
			((CS_SEMAPHORE *)an_scb->cs_sync_obj)->cs_sem_name :
			"(UN-NAMED)");
		break;
		
	    case CS_CNDWAIT:
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		    "Session %p (%24s) cs_state: %w (%p) cs_mask: %v",
			    an_scb->cs_self,
			    an_scb->cs_username,
			    cs_state_names,
			    an_scb->cs_state,
			    an_scb->cs_sync_obj,
			    cs_mask_names,
			    an_scb->cs_mask); 
		break;
		
	    default:

		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		    "Session %p (%24s) cs_state: <invalid> %x",
		    an_scb->cs_self,
		    an_scb->cs_username,
		    an_scb->cs_state);
		break;

	}
	if (formatted)
	{
	    if (an_scb->cs_mask & CS_MNTR_MASK)
	    {
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		    "Session %p is a monitor task, owner is pid %d",
			an_scb->cs_self,
			an_scb->cs_ppid);
	    }
	    else if ((an_scb == &Cs_idle_scb)
			|| (an_scb == (CS_SCB *)&Cs_admin_scb))
	    {
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		"Session %p is an internal task; no info available",
			an_scb->cs_self);
	    }
	    else
	    {
		(*Cs_srv_block.cs_format)(an_scb,
					    command,
					    powerful, 1);
	    }
	}
    }

    return;
}

/*
** History:
**	31-jan-1994 (bryanp) B56960
**	    LRC requests that "special" should be "system"
*/
static void
format_sessions(char *command, TR_OUTPUT_FCN *output_fcn, i4 powerful)
{
    i4		show_user = 0;
    i4		show_system = 0;
    CS_SCB	*scan_scb;
    CS_SCB	*an_scb;
    char	buf[100];
    i4		found_scb = 0;

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
    for (scan_scb = Cs_srv_block.cs_known_list->cs_next;
	scan_scb && scan_scb != Cs_srv_block.cs_known_list;
	scan_scb = scan_scb->cs_next)
    {
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
		    "Session %p is a monitor task, owner is pid %d",
			scan_scb->cs_self,
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
		found_scb = 1;
		break;	/* we only wanted to format one SCB */
	    }
	}
    }
    if (an_scb != 0 && found_scb == 0)
    {
	TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
	    "Invalid session id");
    }
    CSv_semaphore(&Cs_known_list_sem);
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
**  History:
**       7-May-2009 (hanal04) Bug 122034
**          Adjust TRformat of sampling interval to handle 100.00 which
**          is valid.
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

	    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	   		"The sampling interval is now %6.2f seconds",
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
	CsSamplerBlkPtr->interval = interval;

	/* Get and save the current Server CPU usage */
	CS_update_cpu((i4 *)0, (i4 *)0);
	CsSamplerBlkPtr->startcpu = Cs_srv_block.cs_cpu;

	/* Set the starting wait counts */
	MEcopy((char*)&Cs_srv_block.cs_wtstatistics,
		sizeof(CsSamplerBlkPtr->waits),
		    (char*)&CsSamplerBlkPtr->waits);

	/*
	** Unlock the sampler block.
	** Unleash the sampler
	** return
	*/

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
**	05-Oct-1998 (jenjo02)
**	    Added server CPU time and counts of waits by type to the
**	    sampler output.
**	    Changed MAXTHREADS to MAXSAMPTHREADS.
**
****************************************************************************/

static STATUS
ShowSampler(
	TR_OUTPUT_FCN *output_fcn )

{
	char	timestr[50];
	char	buf[100];
	/*f8	perct[max(MAXSAMPTHREADS,MAXMUTEXES+1)];*/
	f8	*perct;
	i4	samples;
	i4	i, j, temp;
	i4	threshold;
	i4	totusersamples = 0;
	i4	totsyssamples = 0;
	i4	totmutexes;
	STATUS  status = OK;
		
	if (CsSamplerBlkPtr == NULL)
	{
	    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	   	     "There is no sampling now");
	    return(FAIL);
	}

	/*
	** Lock the Sampler Block
	*/
	if (status != OK)
	    return(status);

	if (CsSamplerBlkPtr == NULL)
	{
	    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	   	     "There is no sampling now");

	    return(FAIL);
	}

        perct = (f8*)MEreqmem( 0, max(MAXSAMPTHREADS,MAXMUTEXES+1)*sizeof(f8),TRUE,NULL);


	/*
	** Print the time
	*/
	TMstr(&CsSamplerBlkPtr->starttime, timestr);
	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
		"Started at %s.",
		timestr);

	TMnow(&CsSamplerBlkPtr->stoptime);
	TMstr(&CsSamplerBlkPtr->stoptime, timestr);

	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
		"Current Time %s.",
		timestr);

	for (i = -1; i < MAXSAMPTHREADS; i++)
	{
	     if (i == 0)
	         totusersamples += CsSamplerBlkPtr->Thread[i].numthreadsamples;
	     else
	         totsyssamples += CsSamplerBlkPtr->Thread[i].numthreadsamples;

	}

	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
		"Total sample intervals %d.",
		CsSamplerBlkPtr->numsamples);

	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
		"Total User Thread samples %d.",
		totusersamples);

	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
		"Total System Thread samples %d",
		totsyssamples);

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
		"Thread Name          Computable  EventWait  MutexWait   Misc");

	for (i = -1; i < MAXSAMPTHREADS; i++)
	{
	     samples = CsSamplerBlkPtr->Thread[i].numthreadsamples;
	     if (samples > 0)
	     {
		for (j = 0, perct[0] = 0.0; j < MAXSTATES; j++)
		{
		    if (j > 3)  
	    		perct[0] += (100.0 * CsSamplerBlkPtr->Thread[i].state[j] )
	    				/ (f8) samples;
		    else
    	     	        perct[j] = (100.0 * CsSamplerBlkPtr->Thread[i].state[j] )
    	     	        		/ (f8) samples;
		}

		TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
			"%20w %10.1f  %9.1f  %9.1f  %5.1f",
			"AdminThread,UserThread,MonitorThread,FastCommitThread,\
WriteBehindThread,ServerInitThread,EventThread,2PCThread,RecoveryThread,LogWriter,\
CheckDeadThread,GroupCommitThread,ForceAbortThread,AuditThread,CPTimerThread,CheckTermThread,\
SecauditWriter,WriteAlongThread,ReadAheadThread,ReplicatorQueueMgr,\
LKCallbackThread,LKDeadlockThread,SamplerThread,SortThread,FactotumThread,LicenseThread,Invalid",
			i+1,
			perct[1], perct[2], perct[3], perct[0]);
	    }
	}

	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
		"------------------------------------------------------------");

	/*
	** For user/system threads, tell how much computable time was spent in each facility
	*/

	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
		"Computability Analysis");

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
	    	    if (j == 0)
    	     	        perct[0] = (100.0 * CsSamplerBlkPtr->Thread[j].facility[i] )
    	     	       	        	/ (f8) totusersamples;
		    else
		    {
		     	perct[1] += (100.0 * CsSamplerBlkPtr->Thread[j].facility[i] )
   	     	       			/ (f8) totsyssamples;
		    }
		}
	    }

	    if (perct[0] > 0 || perct[1] > 0)
	    {

	        TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
			"%8w    %6.1f  %6.1f",
			"<None>,CLF,ADF,DMF,OPF,PSF,QEF,QSF,RDF,SCF,ULF,DUF,GCF,RQF,TPF,GWF,SXF,URS,ICE,CUF",
			i,
			perct[0], perct[1]);
	    }
	}
	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
		"--------------------------");

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
		"Event Wait Analysis");

	    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "           DIOR    DIOW    LIOR    LIOW    BIOR    BIOW     Log    Lock  LGEvnt  LKEvnt   Other");
	    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "Waits  %8d%8d%8d%8d%8d%8d%8d%8d%8d%8d%8d",
	    Cs_srv_block.cs_wtstatistics.cs_dior_done - 
		CsSamplerBlkPtr->waits.cs_dior_done,
	    Cs_srv_block.cs_wtstatistics.cs_diow_done - 
		CsSamplerBlkPtr->waits.cs_diow_done,
	    Cs_srv_block.cs_wtstatistics.cs_lior_done - 
		CsSamplerBlkPtr->waits.cs_lior_done,
	    Cs_srv_block.cs_wtstatistics.cs_liow_done - 
		CsSamplerBlkPtr->waits.cs_liow_done,
	    Cs_srv_block.cs_wtstatistics.cs_bior_done - 
		CsSamplerBlkPtr->waits.cs_bior_done,
	    Cs_srv_block.cs_wtstatistics.cs_biow_done - 
		CsSamplerBlkPtr->waits.cs_biow_done,
	    Cs_srv_block.cs_wtstatistics.cs_lg_done - 
		CsSamplerBlkPtr->waits.cs_lg_done,
	    Cs_srv_block.cs_wtstatistics.cs_lk_done - 
		CsSamplerBlkPtr->waits.cs_lk_done,
	    Cs_srv_block.cs_wtstatistics.cs_lge_done - 
		CsSamplerBlkPtr->waits.cs_lge_done,
	    Cs_srv_block.cs_wtstatistics.cs_lke_done - 
		CsSamplerBlkPtr->waits.cs_lke_done,
	    (Cs_srv_block.cs_wtstatistics.cs_tm_waits -
		CsSamplerBlkPtr->waits.cs_tm_waits) +
		(Cs_srv_block.cs_wtstatistics.cs_event_wait -
		    CsSamplerBlkPtr->waits.cs_event_wait));
			
	    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
	    "Time(ms)  %5d%8d%8d%8d%8d%8d%8d%8d%8d%8d%8d",
		(dior_waits) ? dior_time / dior_waits : 0,
		(diow_waits) ? diow_time / diow_waits : 0,
		(lior_waits) ? lior_time / lior_waits : 0,
		(liow_waits) ? liow_time / liow_waits : 0,
		(bior_waits) ? bior_time / bior_waits : 0,
		(biow_waits) ? biow_time / biow_waits : 0,
		(lg_waits)  ? lg_time  / lg_waits   : 0,
		(lk_waits)  ? lk_time  / lk_waits   : 0,
		(lge_waits) ? lge_time / lge_waits  : 0,
		(lke_waits) ? lke_time / lke_waits  : 0,
		0);

	    samples = CsSamplerBlkPtr->numsyseventsamples;
	    if (samples > 0)
	    {

	        for (j = 0; j < MAXEVENTS + 1; j++)
	        {
		    perct[j] = (100.0 * CsSamplerBlkPtr->sysevent[j] )
				/ (f8) totsyssamples;
	        }

	        TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
		"%8s%7.1f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f",
		"System", perct[0], 
		perct[1], perct[2], perct[3], perct[4], perct[5], perct[6],
		perct[7], perct[8], perct[9]);
	    }  /* System Events */
		
	    samples = CsSamplerBlkPtr->numusereventsamples;
	    if (samples > 0)
	    {

	        for (j = 0; j < MAXEVENTS + 1; j++)
	        {
		    perct[j] = (100.0 * CsSamplerBlkPtr->userevent[j] )
				/ (f8) totusersamples;
	        }

	        TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
		"%8s%7.1f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f %7.1f",
		"User", perct[0], 
		perct[1], perct[2], perct[3], perct[4], perct[5], perct[6],
		perct[7], perct[8], perct[9]);

	    }  /* User Events */

	    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
    "-----------------------------------------------------------------------------------------------");
	}  /* either System or User Events */

	/*
	** For each Mutex Wait, tell how much time was spent waiting on each.
	*/	
	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
		"Mutex Wait Analysis");

	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
		"Mutex Name                       ID        Admin  User  Fast  Write   Log  Group");

	samples = CsSamplerBlkPtr->nummutexsamples;
	threshold = (samples + 50) / 1000;	/* only look at mutexes > .1% */
	if (samples > 0)
	{
	    for (i = 0; i < MAXMUTEXES+1; i++)
	    {
		if (CsSamplerBlkPtr->Mutex[i].name[0] == EOS)
		    continue;

		for (j = -1, totmutexes = 0; j < MAXSAMPTHREADS; j++)
		{
		    temp = CsSamplerBlkPtr->Mutex[i].count[j];
		    if (temp > 0)
		    {
	                perct[j+1] = (100.0 * temp)
           		 / (f8) CsSamplerBlkPtr->Thread[j].numthreadsamples;
		        totmutexes += temp;
		    }
		    else
		    {
		        perct[j+1] = 0.0;
		    }
		}

	        if (totmutexes > threshold)
		{
		    TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
			"%32s %p %5.1f  %5.1f %5.1f  %5.1f %5.1f  %5.1f",
			CsSamplerBlkPtr->Mutex[i].name,
			CsSamplerBlkPtr->Mutex[i].id,
			perct[0], perct[1], perct[3], perct[4], perct[9], perct[11]);
		}
	    }

	}  /* Mutex Events */

        TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
		"--------------------------------------------------------------------------------");

	/*
	** For each Condition Wait, tell how much time was spent waiting on each.
	*/	

	MEfree((PTR)perct);

	/* 
	** Unlock the Sampler Block
	** return
	*/

	return (status);
}  /* ShowSampler */


/****************************************************************\
**								**
** I I M O N I T O R    R E G I S T R A T I O N    S T U F F	**
**								**
\****************************************************************/

# define	CSMON_MAX_REGS		128

typedef struct _CSMON_REGISTRATION CSMON_REGISTRATION;

struct _CSMON_REGISTRATION
{
    char	*prefix;
    i4		(*entryfcn)(TR_OUTPUT_FCN *, char *);
};

static i4		  CSmon_Registration_Count = 0;

static CSMON_REGISTRATION CSmon_Registrations[CSMON_MAX_REGS]	ZERO_FILL;

/****************************************************************************
**
** Name: csmon_lookup_entry
**
** Description:
**      Internal utility function which encapsulates search routine
**      used to determine which if any of the registered routines
**	"matches" the command string.
**
**	Since command string passed has been packed by STzapblank(),
**	we can't parse out individual words, so instead, command
**	string is checked against all registered prefixes, and the 
**	longest prefix matched is selected.
**
** Inputs:
**
**	command - pointer to packed lower case command string.
**
**  Returns:
**      Pointer to matching registration, or NULL if no matches.
**
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
**  History:
**	04-sep-2002 (devjo01)
**	    Created.
**
****************************************************************************/

static CSMON_REGISTRATION *
csmon_lookup_entry( char *command )
{
    CSMON_REGISTRATION	*registration;
    i4			 index, length, bestindex, bestlength;

    bestlength = 0;

    for ( registration = CSmon_Registrations, index = 0;
	  index < CSmon_Registration_Count; 
	  registration++, index++ )
    {
	length = STlength(registration->prefix);
	if ( 0 == MEcmp(command, registration->prefix, length) )
	{
	    if ( length > bestlength )
	    {
		bestindex = index;
		bestlength = length;
	    }
	}
    }
    if ( bestlength > 0 )
    {
	return CSmon_Registrations + bestindex;
    }
    return NULL;
}


/****************************************************************************
**
** Name: IICSmon_register
**
** Description:
**      Hook in diagnostic code for another facility.  This allows
**      access through iimonitor to diagnostic code which must access
**	data and structures private to that facility.  All CS needs
**	to know, is a unique prefix for command strings to be passed
**	to the foreign diagnostic facility, and an entry point functions.
**
**	Entry point functions is passed the output function pointer,
**	and the command string.   Entry point function should return
**	OK unless printing of standard CSmonitor bad syntax message
**	is desired.
**
**	No attempt is made to protect against concurrent registrations,
**	since it is anticipated that registrations will be infrequent,
**	and typically done during single threaded intialization of the
**	server.
**
** Inputs:
**
**	prefix	- pointer to character string which should uniquely
**		  distinguish commands to be routed to registering
**		  diagnostic.  Prefix must be lower case, and cannot
**		  contain a space.
**
**	entry_fcn - Address of function which will be passed the
**		  following two parameters if "prefix" is matched.
**
**	  output_fcn      Function to call to perform the output.
**	                  This routine will be called as
**                      (*output_fcn)(NULL, length, buffer)
**	                  where buffer is the length character
**	                  output string
**
**	  command	  Pointer to buffer holding passed command.
**
**  Returns:
**      OK	- Diag added or altered.
**	FAIL	- Array full.
**
**  Exceptions:
**      none
**
** Side Effects:
**      One registration slot consumed per successful new registation.
**
**  History:
**	04-sep-2002 (devjo01)
**	    Created.
**	11-Jan-2008 (jonj)
**	    Upped CSMON_MAX_REGS from 8 to 128
**
****************************************************************************/

STATUS
IICSmon_register( char *prefix, i4 (*entry_fcn)(TR_OUTPUT_FCN *, char *) )
{
    CSMON_REGISTRATION	*registration;

    if ( CSMON_MAX_REGS == CSmon_Registration_Count )
    {
	return FAIL;
    }
    if ( NULL == ( registration = csmon_lookup_entry( prefix ) ) )
    {
	registration = CSmon_Registrations + CSmon_Registration_Count++;
	registration->prefix = STalloc(prefix);
    }
    registration->entryfcn = entry_fcn;
    return OK;
}


/****************************************************************************
**
** Name: IICSmon_deregister
**
** Description:
**      Remove an entry point previously registered with IICSmon_register.
**
** Inputs:
**	prefix	- pointer to character string which uniquely
**		  distinguishes diagnostic to be deregistered.
**
** Returns:
**      OK	- Diag removed.
**	FAIL	- Prefix not found.
**
**  Exceptions:
**      none
**
** Side Effects:
**      Minor memory leak since STalloc'ed mem is not scavenged.
**
**  History:
**	04-sep-2002 (devjo01)
**	    Created.
**
****************************************************************************/

STATUS
IICSmon_deregister( char *prefix )
{
    CSMON_REGISTRATION	*registration, *new_end;
    i4			 bytestomove;

    if ( NULL == ( registration = csmon_lookup_entry( prefix ) ) )
    {
	return FAIL;
    }

    CSmon_Registration_Count--;
    new_end = CSmon_Registrations + CSmon_Registration_Count;

    if ( registration != new_end )
    {
	bytestomove = (PTR)new_end - (PTR)registration;
	MEmove( (u_i2)bytestomove, (PTR)(registration + 1), '\0',
		(u_i2)bytestomove, (PTR)registration );
    }
    return OK;
}

/****************************************************************************
**
** Name: CSmon_try_registered_diags
**
** Description:
**      Routined used by OS & INTERNAL versions of IIMONITOR to check
**	an otherwise unrecognized command string against registered
**	diagnostics.
**
** Inputs:
**	output_fcn -      Function to call to perform the output.
**	                  This routine will be called as
**                      (*output_fcn)(NULL, length, buffer)
**	                  where buffer is the length character
**	                  output string
**
**	command	   -      String to be checked against registered diagnostics.
**
** Returns:
**      OK	- Diag Found and successfully invoked.
**	FAIL	- Prefix not found.
**
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
**  History:
**	04-sep-2002 (devjo01)
**	    Created.
**
****************************************************************************/

i4
CSmon_try_registered_diags( TR_OUTPUT_FCN *output_fcn, char *command )
{
    CSMON_REGISTRATION	*registration;

    if ( NULL == ( registration = csmon_lookup_entry( command ) ) )
    {
	return FAIL;
    }
    return ( (*registration->entryfcn)( output_fcn, command ) );
}
