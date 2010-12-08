/*
** Copyright (c) 1987, 2009 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <cs.h>
# include <er.h>
# include <ex.h>
# include <pc.h>
# include <lk.h>
# include <tm.h>
# include <tr.h>
# include <nm.h>
# include <st.h>
# include <me.h>
# include <si.h>
# include <cv.h>
# include <sp.h>
# include <mo.h>

# include <exhdef.h>
# include <csinternal.h>

# include "csmgmt.h"

# include "cslocal.h"

# include "cssampler.h"

# include <ssdef.h>
# include <lib$routines>
# include <starlet.h>

# include <astjacket.h>

GLOBALREF CS_SEMAPHORE		Cs_known_list_sem;
/*
** SAMPLER globals and externs
*/
GLOBALDEF CSSAMPLERBLKPTR       CsSamplerBlkPtr ZERO_FILL;

static bool          SamplerInit = FALSE;

static STATUS StartSampler(
                char    *command,
		TR_OUTPUT_FCN *output_fcn );

static STATUS StopSampler(
		TR_OUTPUT_FCN *output_fcn );

static STATUS ShowSampler(
		TR_OUTPUT_FCN *output_fcn );

static void show_mutex(
    TR_OUTPUT_FCN *output_fcn,
    CS_SEMAPHORE *mutex );
    
static int show_sessions(
    char    *command,
    TR_OUTPUT_FCN *output_fcn,
    i4	    formatted);

static void format_sessions(char *command, TR_OUTPUT_FCN *, i4 powerful);
static STATUS mntr_handler(EX_ARGS *exargs);

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
CS_null_handler(
EX_ARGS	    *exargs)
{
    return EXDECLARE;
}

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
CS_verify_sid(
  char 	*action, 
  char  *command,
  TR_OUTPUT_FCN *output_fcn,
  i4	nointernal)
{
    EX_CONTEXT		 ex_context;
    CS_SCB		*scb = NULL;
    PTR			 ptr_tmp;
    char		 buf[80];

    STzapblank(command, command);
    if (CVaxptr(command + STlength(action), &ptr_tmp) == OK)
    {
	/* Install handler to catch bad user SID ID */
	if (EXdeclare(CS_null_handler, &ex_context) == OK)
	    scb = CS_find_scb((CS_SID)ptr_tmp);
	EXdelete();
    }

    if (scb == NULL)
    {
	TRformat(output_fcn, NULL, buf, sizeof(buf) - 1, "Invalid session id");
    }
    else if (nointernal && ((scb == (CS_SCB *)&Cs_idle_scb) ||
	    (scb == (CS_SCB *)&Cs_admin_scb)))
    {
	TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
		"Cannot %s internal session %p.", action, scb->cs_self);
	scb = NULL;
    }
    return scb;
}


/*{
** Name: IICSmonitor	- Implement the monitor task
**
** Description:
**      This routine is called ala the regular thread processing routine. 
**      It parses its input, decides what to do, and returns the output.
**
** Inputs:
**      mode                            Mode of operation
**					    CS_INPUT, _OUPUT, ...
**      scb                             Sessions control block to monitor
**      *command			Text of the command
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
**      next_mode                       As above.
**	Returns:
**	    OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      13-jan-1987 (fred)
**          Rudimentary version created.
**      26-jan-1988 (fred)
**          Added output function to solve overflow bugs.
**	09-oct-90 (ralph)
**	    6.3->6.5 merge:
**	    23-apr-90 (bryanp)
**		Added support for CS_GWFIO_MASK used to indicate Gateway wait.
**      18-aug-91 (ralph)
**          CSmonitor() - Drive CSattn() when IIMONITOR REMOVE
**	11-nov-1992 (bryanp)
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
**	24-may-1993 (bryanp)
**	    Repair formatting problem in showsmstats.
**      8-jun-93 (ed)
**          change parm definition for scb_temp to match CL spec
**      16-jul-93 (ed)
**	    added gl.h
**	23-aug-1993 (andys)
**	    In "breakpoint" command do a lib$signal(ss$_debug) to force
**	    us into the debugger. 
**	    Put these mysterious "[@history_line@]" things back at the end of
**	    history.
**          In CSmonitor, check the return value of CSremove before calling
**          CSattn. [bug 45020] [was 28-aug-92]
**	20-oct-1993 (lauraw)
**	    Removed include csdbms.h.
**	31-jan-1994 (bryanp) B57296
**	    Handle unnamed semaphores in the "show mutex" command.
**	31-jan-1994 (bryanp) B56960
**	    LRC: "special" should be "system".
**	23-feb-95 (wolf, from canor01)
**	    B64236: Update the help text to show other options.
**      13-jun-95 (albany, from wolf, for harpa06)
**	    Added "SHOW SERVER LISTEN" and "SHOW SERVER SHUTDOWN" to the help
**          display.  (68690)
**	27-jul-1995 (dougb)
**	    Add "plus" versions of most "show" commands -- to output just a bit
**	    more information.  (These are not the defaults to avoid testing
**	    differences.)  Also, add new "show internal sessions" commands.
**	    Some of the information output by these new commands may not be
**	    suitable for customers.  Therefore, they are not documented
**	    anywhere but in this file!
**	    In case CSterminate() returns a "real" error, display any
**	    unexpected status.
**	 4-aug-1995 (dougb)
**	    Correct problem with parsing 'show system sessions' (et cetera)
**	    introduced in my last change.  Also, remove unecessary
**	    STzapblank() calls in CSmonitor() -- after it's already been done.
**      06-dec-1995 (whitfield)
**          Add "stacksplus" and "debugplus" to cause verbose variant of
**          "stacks" and "debug" commands.
**	24-jan-1996 (dougb) bug 71590
**	    Add xDEBUG-only commands which may be used to cause run-time
**	    exceptions which might reach the VMS last-chance handler.
**	19-feb-1996 (duursma)
**	    Partial integration of UNIX change 423116.
**	19-aug-1997 (teresak)
**	    Integration of sampler code for UNIX
**	20-feb-1998 (kinte01)
**	   Cross-integrate Unix change 433682
**         7-jan-98 (stephenb)
**           Format the output for "show sessions" better and add the
**           active users highwater mark in preparation for user based licensing
**	21-jan-1999 (canor01)
**	    Remove erglf.h.
**      19-may-99 (kinte01)
**          Added casts to quite compiler warnings & change nat to i4
**          where appropriate
**	11-apr-2000 (devjo01)
**	    Add ability to "register" debug/diag functions with IIMONITOR.
**	15-apr-2000 (devjo01)
**	    Cross change 442838 from 1.2.   General cleanup.   Make show_mutex
**	    handle a bad address.
**	19-jul-2000 (kinte01)
**	    Correct prototype definitions by adding missing includes and
**	    external function references
**	19-Jul-2000 (kinte01)
**	   extraneous \ found in TRformat statement. reported by 6.2 compiler
**      23-Nov-2000 (horda03)
**          Sampling code was not completely ported to VMS. Adding the code
**          that identifies that user has issued a Sampling command, and to
**          invoke the necessary function (direct port from Unix).
**
**          CSSAMPLING accvio's due to differences in use of cs_memory
**          and cs_sync_obj on VMS to UNIX. Bug 56445 introduced the
**          field cs_sync_obj in CS_SCB to hold the semaphore ptr being
**          addressed in CS routine rather than the misuse of cs_memory.
**          (102059)
**	01-dec-2000	(kinte01)
**		Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**		from VMS CL as the use is no longer allowed
**	07-feb-01 (kinte01)
**	    Add casts to quiet compiler warnings
**	11-apr-2002 (kinte01)
**	    More compiler warning cleanup
**	28-aug-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP and use default
**	    /STANDARD compiler option (rather than VAXC).
**	23-oct-2003 (devjo01)
**	    Hand cross germane 103715 changes from UNIX CL.
**	    (Diagnostic registration stuff.)
**	23-oct-2003 (kinte01)
**	    Add missing lk.h header
**	28-jun-2005 (devjo01)
**	    Correct show_mutex for CS_SEM_MULTI mutexes.
**	08-Feb-2007 (kiria01) b117581
**	    Use global cs_mask_names and cs_state_names.
**	14-oct-2008 (joea)
**	    Integrate 16-nov-1998 change from unix version.  Add Log I/O,
**	    I/O read/write counts. Incorporate server wait statistics into
**	    Event Wait Analysis graph.
**	21-nov-2008 (joea)
**	    Renamed to IICSmonitor to support both OS and internal threads.
**	01-dec-2008 (joea)
**	    Remove prototype for CS_find_scb.
**	02-dec-2008 (joea)
**	    Rename csmon_try_registered_diags to CSmon_try_registered_diags
**	    and make it visible externally.  In format_sessions, use
**	    semaphores instead of disabling/enabling ASTs.
**	09-dec-2008 (joea)
**	    Use CS_null_handler ported from Unix.
**      23-Apr-2009 (hanal04) Bug 132714
**          Update iimonitor help output to include SAMPLING commands.
**      24-apr-2009 (stegr01)
**          Itanium VMS port.
**      23-Sep-2009 (hanal04) Bug 115316
**          Added "SHOW SERVER CAPABILITIES".
**	15-Nov-2010 (kschendel) SIR 124685
**	    Prototype / include fixes.
**      16-Nov-2010 (horda03) b124691
**          Inside a threaded server ASTs should be disabled.
**      06-Dec-2010 (horda03) SIR 124685
**          Fix VMS build problems, 
**      07-Dec-2010 (horda03) SIR 124685
**          Fix another VMS build problems
*/
STATUS
IICSmonitor(
i4	mode,
CS_SCB	*scb_temp,
i4	*nmode,
char	*command,
i4	powerful,
TR_OUTPUT_FCN *output_fcn)
{
    STATUS              status;
    i4			count;
    char		buf[81];
    EX_CONTEXT		ex_context;
    CS_SCB		*an_scb;
    CS_SCB		*scan_scb;
    CS_MNTR_SCB		*scb;
    CS_SEMAPHORE	*mutex;
    i4                 format = 1, verbose = 0;
    i4                 asts_enabled;

    scb_temp->cs_thread_type = CS_MONITOR;
    scb = (CS_MNTR_SCB *)scb_temp;
    *nmode = CS_EXCHANGE;

    asts_enabled = (sys$setast(0) == SS$_WASSET);

    if (EXdeclare(CS_null_handler, &ex_context))
    {
	if (asts_enabled) sys$setast(1);
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
# ifdef xDEBUG
	    if (( 0 !=
		 ( verbose = !STscompare( "corruptback", 0, command, 0 ))
		 )
		|| ( 0 == STscompare( "corrupt", 0, command, 0 )))
	    {
		static VOID fill_fill( bool back );

		/*
		** EXdeclare() above is irrelevant, this should corrupt
		** stack from this frame up- or down-wards.  We'll never get
		** back here since fill_fill() or a routine it calls will
		** attempt to return to an address looking a lot like NULL.
		*/
		TRdisplay( "About to corrupt the stack %swards.\n",
			  ( verbose ? "up" : "down" ));
		TRformat( output_fcn, NULL, buf, sizeof( buf ) - 1,
			 "About to corrupt the stack %swards.",
			 ( verbose ? "up" : "down" ));

		fill_fill( verbose );

		TRdisplay( "Unexpected return from fill_fill().\n" );
		TRformat( output_fcn, NULL, buf, sizeof( buf ) - 1,
			 "Unexpected return from fill_fill()." );
	    }
	    else if ( 0 == STscompare( "handle", 0, command, 0 ))
	    {
		static STATUS bad_handler( EX_ARGS *exargs );
		f8	zero;
		f8	one;
		f8	res;

		/*
		** Use a handler which won't be very useful.  We're attempting
		** to see how exceptions in the user handler routines are
		** handled.  So far, things are just handled further up the
		** stack.  There's 2 EXdeclare()'s of cs_handler() above this
		** call frame (by CS_setup() and CSdispatch()).  In addition,
		** frontstart.c on VMS establishes a VMS error handler which
		** exits the image cleanly when it is reached.  The last-chance
		** handler never seems to be called.
		*/
		EXdelete();
		EXdeclare( bad_handler, &ex_context );

		TRdisplay( "Bad handler installed, causing FPDIV error.\n" );
		TRformat( output_fcn, NULL, buf, sizeof( buf ) - 1,
			 "Bad handler installed, causing FPDIV error." );

		/* Cause a run-time exception -- FPDIV. */
		zero = 0.0;
		one = 1.0;
		res = one / zero;
	    }
	    else if ( 0 == STscompare( "recurse", 0, command, 0 ))
	    {
		volatile char string [4096] = { '\0' };

		/*
		** Work 'til we hit the guard page.  On VMS, this will
		** get us to the last-chance exception handler immediately.
		** On Unix systems which have "real" stack protection, this
		** won't even be a thread-fatal exception:  mntr_handler()
		** will be called on an alteri4e stack and processing will
		** continue in the EXdeclare() block above.
		*/
		TRdisplay( "We have recursed %d times.\n", powerful );
		TRformat( output_fcn, NULL, buf, sizeof( buf ) - 1,
			 "We have recursed %d times.", powerful );

		STcopy( command, string );
		status = CSmonitor( mode, scb_temp, nmode, string,
				   powerful + 1, output_fcn );
	    }
# endif /* xDEBUG */
	    if (STscompare("breakpoint", 0,
			command, 0) == 0)
	    {
#ifdef xDEBUG
		lib$signal(SS$_DEBUG);
#endif
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
		    if ( E_CS0003_NOT_QUIESCENT == status )
		    {
			TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
				"Error stopping server, %d. sessions remaining",
					count);
		    }
		    else if ( status )
		    {
			TRformat( output_fcn, NULL, buf, sizeof buf - 1,
			       "CSmonitor(): Error 0x%x while attempting stop",
				 status );
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
		    if ( E_CS0003_NOT_QUIESCENT == status )
		    {
			TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
				"Server will stop. %d. sessions aborted",
					count);
		    }
		    else if ( status )
		    {
			TRformat( output_fcn, NULL, buf, sizeof buf - 1,
			       "CSmonitor(): Error 0x%x while attempting stop",
				 status );
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
	    else if (STscompare("setservershut", 0,
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
		    if ( E_CS0003_NOT_QUIESCENT == status )
		    {
			TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
				"Server will stop. %d. sessions remaining",
					count);
		    }
		    else if ( status )
		    {
			TRformat( output_fcn, NULL, buf, sizeof buf - 1,
			       "CSmonitor(): Error 0x%x while attempting shut",
				 status );
		    }
		}
	    }
	    else if ( show_sessions( command, output_fcn, powerful ) )
	    {
		/* If non-zero return, SHOW SESSION command processed*/;
	    }
	    else if ( STscompare("showmutex", 9, command, 9) == 0)
	    {
		if ((CVaxptr(command + 9, (PTR *)&mutex)) || (mutex < (PTR)0x200))
		{
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"Invalid mutex id %p", mutex);
		    break;
		}
  		show_mutex(output_fcn,mutex);
	    }
	    else if ( STscompare("showserver", 0, command, 0) == 0
		     || 0 == STscompare( "showserverplus", 0, command, 0 ))
	    {
		if ( '\0' == command[10] )
		{
		    /*
		    ** Regular version of command, don't change output from
		    ** earlier versions
		    */
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
                    "\n\tServer: %s\n \nConnected Sessions (includes \
system threads)\n \n\tCurrent:\t%d\n\tLimit:\t\t%d\n \nActive \
Sessions\n \n\tCurrent\t\t%d\n\tLimit\t\t%d\n\tHigh \
Water\t%d\n \nrdy mask %x state: %x\nidle quantums %d./%d. (%d.%%)",
		    Cs_srv_block.cs_name,
		    Cs_srv_block.cs_num_sessions, Cs_srv_block.cs_max_sessions,
                    Cs_srv_block.cs_num_active, Cs_srv_block.cs_max_active,
                    Cs_srv_block.cs_hwm_active,
		    Cs_srv_block.cs_ready_mask,
		    Cs_srv_block.cs_state,
		    Cs_srv_block.cs_idle_time,
		    Cs_srv_block.cs_quantums,
		    (Cs_srv_block.cs_quantums ?
			((Cs_srv_block.cs_idle_time *
			    100)/Cs_srv_block.cs_quantums) : 100));
		}
		else
		{
		    /*
		    ** Plus version of command, add meaning of the cs_state
		    ** and output the current cs_mask.
		    */
                TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
                    "\n\tServer: %s\n \nConnected Sessions (includes \
system threads)\n \n\tCurrent:\t%d\n\tLimit:\t\t%d\n \nActive \
Sessions\n \n\tCurrent\t\t%d\n\tLimit\t\t%d\n\tHigh \
Water\t%d\n \nrdy mask %x state: %x(%w)\nidle quantums %d./%d. (%d.%%) \
\ncs_mask: %x (%v)",
                    Cs_srv_block.cs_name,
                    Cs_srv_block.cs_num_sessions, Cs_srv_block.cs_max_sessions,
                    Cs_srv_block.cs_num_active, Cs_srv_block.cs_max_active,
                    Cs_srv_block.cs_hwm_active,
                    Cs_srv_block.cs_ready_mask,
                    Cs_srv_block.cs_state,
                             "CS_UNINIT,CS_INITIALIZING,CS_PROCESSING,\
CS_IDLING,CS_CLOSING,CS_TERMINATING,CS_ERROR,CS_REPENTING,CS_SWITCHING",
                             ( CS_SWITCHING == Cs_srv_block.cs_state
                              ? 0x08 : ( Cs_srv_block.cs_state / 0x10 )),
                    Cs_srv_block.cs_idle_time,
                    Cs_srv_block.cs_quantums,
                    (Cs_srv_block.cs_quantums ?
                        ((Cs_srv_block.cs_idle_time *
                            100)/Cs_srv_block.cs_quantums) : 100),
                             Cs_srv_block.cs_mask,
                            "CS_NOSLICE,CS_ADD,CS_SHUTDOWN,CS_FINALSHUT,\
CS_ACCNTING,CS_CPUSTAT,CS_REPENTING,CS_LONGQUANTUM",
                             Cs_srv_block.cs_mask );
		}
                    if (Cs_srv_block.cs_gpercent)
                    {
                        if (Cs_srv_block.cs_gpriority)
                        {
                            TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
"Govern Usage: %d.%%, Repents at priority: %d., currently %s",
                                Cs_srv_block.cs_gpercent,
                                Cs_srv_block.cs_gpriority,
                                (Cs_srv_block.cs_mask & CS_REPENTING_MASK
                                        ? "Repenting" : "Available"));
                        }
                        else
                        {
                            TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
                                "Govern Usage: %d.%%, currently %s",
                                Cs_srv_block.cs_gpercent,
                                (Cs_srv_block.cs_mask & CS_REPENTING_MASK
                                        ? "Repenting" : "Available"));
                        }
                    }
	    }
	    else if (STscompare("showsmstats", 11, command, 11) == 0)
	    {
		char		place[1024];
		char		*buffer = place;

		STprintf(buffer, "Semaphore Statistics:\n");
		buffer += STlength(buffer);
		STprintf(buffer, "--------- -----------\n");
		buffer += STlength(buffer);
		STprintf(buffer, "    Exclusive requests:   %8d.  Shared:  %8d.  Multi-Process:  %8d.\n",
			    Cs_srv_block.cs_smstatistics.cs_smx_count,
			    Cs_srv_block.cs_smstatistics.cs_sms_count,
			    Cs_srv_block.cs_smstatistics.cs_smc_count);
		buffer += STlength(buffer);
		STprintf(buffer, "    Exclusive collisions: %8d.  Shared:  %8d.  Multi_process:  %8d.\n",
			    Cs_srv_block.cs_smstatistics.cs_smxx_count,
			    Cs_srv_block.cs_smstatistics.cs_smsx_count,
			    Cs_srv_block.cs_smstatistics.cs_smcx_count);
		buffer += STlength(buffer);
		STprintf(buffer, "    Multi-Process Wait loops: %8d.\n",
			    Cs_srv_block.cs_smstatistics.cs_smcl_count);
		buffer += STlength(buffer);
		STprintf(buffer, "    Multi-Process Wait sleeps: %8d.\n",
			    Cs_srv_block.cs_smstatistics.cs_smcs_count);
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
		TRdisplay("=========\n***** WARNING: Semaphore statistics cleared at %@ by %12s\n=========\n",
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
		an_scb = CS_verify_sid("remove", command, output_fcn, 1);
		if (an_scb != NULL)
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
			if (CSremove(an_scb) == OK)
                        {
				TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
                                       "Session %p removed", an_scb);
				CSattn(CS_REMOVE_EVENT, an_scb);
			}
			else
			{
				TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
                                        "Session %p not removed", an_scb);
                        }
		    }
		}
	    }
	    else if (STscompare("suspend", 7, command, 7) == 0)
	    {
		an_scb = CS_verify_sid("suspend", command, output_fcn, 1);
		if (an_scb != NULL)
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
				an_scb);
		    }
		}		    		
	    }
	    else if (STscompare("resume", 6, command, 6) == 0)
	    {
		an_scb = CS_verify_sid("resume", command, output_fcn, 1);
		if (an_scb == NULL)
		    break;
		if (an_scb->cs_state != CS_UWAIT)
		{
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"Session %p was not suspended",
 			    an_scb);
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
				an_scb);
		    }
		}		    		
	    }
	    else if (( 0 !=
		      ( verbose = !STscompare( "debugplus", 9, command, 9 ))
		      )
		     || ( 0 == STscompare( "debug", 5, command, 5 )))
	    {
		char *whatcmd = "debug";
		if (verbose)
		    whatcmd = "debugplus";
		an_scb = CS_verify_sid(whatcmd, command, output_fcn, 1);
		if (an_scb == NULL)
		{
		    char	buffer[1048];

		    CS_fmt_scb( an_scb,
			       sizeof(buffer),
			       buffer );
		    TRformat( output_fcn, NULL, buf, sizeof(buf) - 1,
			     "%t", STlength(buffer), buffer );
		}
		if ( Cs_srv_block.cs_current == an_scb )
		    CS_dump_stack( 0, NULL, NULL, output_fcn, verbose );
		else
		    CS_dump_stack( an_scb, NULL, NULL, output_fcn, verbose );
	    }
            else if (( 0 !=
		      ( verbose = !STscompare( "stacksplus", 10, command, 10 ))
		      )
		     || ( 0 == STscompare( "stacks", 6, command, 6 )))
            {
                for ( scan_scb = Cs_srv_block.cs_known_list->cs_next;
		     scan_scb && scan_scb != Cs_srv_block.cs_known_list;
		     scan_scb = scan_scb->cs_next )
                {
		    TRformat( output_fcn, NULL, buf, sizeof(buf) - 1,
			     "\n---\nStack dump for session %p",
			     scan_scb );

		    if ( Cs_srv_block.cs_current == scan_scb )
			CS_dump_stack( 0, NULL, NULL, output_fcn, verbose );
		    else
			CS_dump_stack( scan_scb, NULL, NULL, output_fcn, verbose );
                }
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
                    "Server:\n     SET SERVER SHUT\n     STOP SERVER\n \
    SHOW SERVER [LISTEN | SHUTDOWN | CAPABILITIES]\n     START SAMPLING [milli second interval]\n     STOP SAMPLING\n     SHOW SAMPLING\n\n");
                TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
                    "Session:\n     SHOW [USER] SESSIONS [FORMATTED]\n \
    SHOW SYSTEM SESSIONS [FORMATTED]\n     SHOW ALL SESSIONS [FORMATTED]\n \
    FORMAT [ALL | sid]\n     REMOVE [sid]\n     \nOthers:\n     QUIT\n\n");
            }
	    else if (STscompare("usage", 5, command, 5) == 0)
	    {
		i4		number;
		
		if ((command[5] == 0) || CVal(command + 5, &number))
		{
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"Invalid usage parameter");
		    break;
		}
		else if ((number < 0) || (number > 99))
		{
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"Usage percentage outside range 0 - 99");
		    break;
		}
		else if (Cs_srv_block.cs_mask & CS_NOSLICE_MASK)
		{
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"Usage valid only with quantums");
		    break;
		}
		else if (Cs_srv_block.cs_mask & CS_LONGQUANTUM_MASK)
		{
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"Usage valid only with quantums <= 00:00:01.00");
		    break;
		}
		else if (!powerful)
		{
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"Superuser required to alter usage.");
		}
		else
		{
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"Server usage changed from %d. to %d.",
			Cs_srv_block.cs_gpercent,
			number);
		    Cs_srv_block.cs_gpercent = number;
		}
	    }
	    else if (STscompare("repentat", 8, command, 8) == 0)
	    {
		i4		number;
		
		if ((command[8] == 0) || CVal(command + 8, &number))
		{
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"Invalid repent priority");
		    break;
		}
		else if ((number < 0) || (number > 15))
		{
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"Repent priority outside range 0 - 15");
		    break;
		}
		else if (!powerful)
		{
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"Superuser required to alter repent method.");
		}
		else if (number != 0)
		{
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"Server repent priority changed from %d. to %d.",
			Cs_srv_block.cs_gpriority,
			number);
		    Cs_srv_block.cs_gpriority = number;
		}
		else
		{
		    /*
		    ** Changing to different repent method.  We must assure
		    ** that we are at the non-repent priority to do this
		    */
		    
		    TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"Server repent priority changed from %d. to %d.",
			Cs_srv_block.cs_gpriority,
			number);
		    Cs_srv_block.cs_gpriority = number;
		    status = sys$setpri(0, 0,
				    Cs_srv_block.cs_norm_priority,
				    0, 0, 0);
		    Cs_srv_block.cs_mask &= ~CS_REPENTING_MASK;
		}
	    }
	    else if ( OK != CSmon_try_registered_diags( output_fcn, command ) )
	    {
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
			"Illegal command");
	    }
	    break;

    	case CS_OUTPUT:
	    break;
    }
    EXdelete();

    if (asts_enabled) sys$setast(1);
    return(OK);
}

static STATUS
mntr_handler( EX_ARGS *exargs )
{
# ifdef xDEBUG
    char	buf [256];
    CL_SYS_ERR	error;

    if ( !EXsys_report( exargs, buf ))
	STprintf( buf, "Exception is %x (%d.)",
		 exargs->exarg_num, exargs->exarg_num );
    TRdisplay( "mntr_handler(): %s\n", buf );
    ERsend( ER_ERROR_MSG, buf, STlength(buf), &error );
# endif /* xDEBUG */

    return(EXDECLARE);
}

# ifdef xDEBUG
#  pragma noinline( fill_fill )
static VOID fill_fill( bool back )
{
    /*
    ** This routine is fairly simple and should have a minimal call frame.
    ** Writing 50 bytes into memory up to this variable (filling the 50
    ** bytes *above* string with 0's) should prevent our return attempt.
    ** Writing 50 bytes after this variable (into the 50 bytes starting
    ** 50 bytes *below* string) should prevent MEfill()'s return attempt.
    ** ??? Note:  This routine must not be inlined by the compiler for any
    ** ??? of this to work (must be a stack procedure).
    */
    char	string [8];

    MEfill( 50, '\0', string + ( back ? sizeof string : -50 ));

    if ( back )
	TRdisplay( "Stack should now be corrupt, unable to return.\n" );
    else
	TRdisplay( "Unexpected return from MEfill().\n" );
}

static STATUS
bad_handler( EX_ARGS *exargs )
{
    char	buf [256];
    CL_SYS_ERR	error;
    char	*ptr = NULL;

    if ( !EXsys_report( exargs, buf ))
	STprintf( buf, "Exception is %x (%d.)", exargs->exarg_num,
		 exargs->exarg_num );
    TRdisplay( "bad_handler(): %s\n", buf );
    ERsend( ER_ERROR_MSG, buf, STlength(buf), &error );

    /* Cause another exception -- ACCVIO. */
    *ptr = 'f';

    return ( EXDECLARE );
}
# endif /* xDEBUG */


static void
show_mutex(
    TR_OUTPUT_FCN *output_fcn,
    CS_SEMAPHORE *mutex )
{
    EX_CONTEXT	ex_context;
    CS_SCB *an_scb;
    char buf[128];

    if ( EXdeclare( mntr_handler, &ex_context ) )
    {
	TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
	 "Invalid mutex address (%p)", mutex );
    }
    else
    {
        if ( (mutex->cs_type == CS_SEM_MULTI) && (!mutex->cs_count) )
	{
            TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
	     "Mutex at %x: Name: %s, owner: %d:%x",
		    mutex,
             mutex->cs_sem_name[0] ? mutex->cs_sem_name : "(UN-NAMED)",
		mutex->cs_pid,
		mutex->cs_owner);
	}
        else if (((mutex->cs_type == CS_SEM_SINGLE) && (mutex->cs_value)) )
        {
            TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
	     "Mutex at %x: Name: %s, owner: %d:%x, list is %x (%d.), ",
		    mutex,
             mutex->cs_sem_name[0] ? mutex->cs_sem_name : "(UN-NAMED)",
		mutex->cs_pid,
		mutex->cs_owner,
		mutex->cs_next,
		mutex->cs_next ? mutex->cs_next->cs_sem_count : 0);
	}
	else
	{
            TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
   "(Currently shared) Mutex at %x: Name: %s, owner: <many>, list is %x (%d.), ",
		mutex,
                mutex->cs_sem_name[0] ? mutex->cs_sem_name : "(UN-NAMED)",
		mutex->cs_next,
		mutex->cs_next ? mutex->cs_next->cs_sem_count : 0);
	}
	if (mutex->cs_type == CS_SEM_SINGLE &&
            NULL != mutex->cs_next)
	{
            for (an_scb = mutex->cs_next->cs_sm_next;
	         an_scb;
	         an_scb = an_scb->cs_sm_next)
            {
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
                        "%x (%d.), ",
                        an_scb, an_scb->cs_sem_count);
	    }
            TRformat(output_fcn, NULL, buf, sizeof(buf) - 1, "\n");
        }
    }
    EXdelete();
}   /* show_mutex */



/*
** History:
**	31-jan-1994 (bryanp) B56960
**	    LRC: "special" should be "system".
**	27-jul-1995 (dougb)
**	    Add "plus" versions of most "show" commands -- to output just a bit
**	    more information.  (These are not the defaults to avoid testing
**	    differences.)  Also, add new "show internal sessions" commands.
**	    Some of the information output by these new commands may not be
**	    suitable for customers.  Therefore, they are not documented
**	    anywhere but in this file!
**	 4-aug-1995 (dougb)
**	    Correct problem with parsing 'show system sessions' (et cetera)
**	    introduced in my last change.  Required no spaces within or
**	    between the first 2 tokens of the command.
**	15-apr-2000 (devjo01)
**	    Crossed improved session display from 1.2.   Cleaned up how
**	    command string was parsed.   Allowable syntax is:
**	    SHOW [ALL | SYSTEM | USER | INTERNAL] SESSIONS [FORMATTED] [PLUS]
**	23-oct-2003 (devjo01)
**	    Added format_lkevent, and format_lock to bring improved UNIX
**	    diagnostics into VMS.
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

static int
show_sessions(
    char    *command,
    TR_OUTPUT_FCN *output_fcn,
    i4	    powerful)
{
    CS_SCB	*an_scb = Cs_srv_block.cs_known_list->cs_next;
    char	buf[100];
    char        tbuf[64];
    i4		show_user 	= 1;	/* Only user sessions by default */
    i4		show_system 	= 0;
    i4		show_internal 	= 0;
    i4		formatted	= 0;
    i4		plus		= 0;

    STzapblank( command, command );
    if ( STscompare( "show", 4, command, 4 ) )
    {
	/* Not a SHOW ... command */
        return 0;
    }
    command += 4;

    if ( 0 == STscompare( "all", 3, command, 3 ) )
    {
	/*
	** To avoid comparison failures, "show all" does *not* output
	** information about the internal threads.
	*/
	show_system = 1;
	command += 3;
    }
    else if ( 0 == STscompare( "system", 6, command, 6 ) )
    {
	show_system = 1;
	show_user = 0;
	command += 6;
    }
    else if ( 0 == STscompare( "user", 4, command, 4 ) )
    {
        command += 4;
    }
    else if ( 0 == STscompare( "internal", 8, command, 8 ) )
    {
	/*
	** Since the Admin and Repent threads are not on the cs_known_list
	** list of well-known threads, we start the following loop with the
	** Admin thread.  These two threads are linked (forward only) to the
	** Idle thread.
	** ??? Outputting information about *all* threads or system and
	** ??? internal threads would not be complicated under this scheme.
	** ??? That has not yet been implemented.
	*/
	an_scb = (CS_SCB *)&Cs_admin_scb;
	show_user = 0;
	show_internal = 1;
	command += 8;
    }

    if ( STscompare( "sessions", 8, command, 8 ) )
    {
	/* Not a SHOW [ALL | SYSTEM | USER | INTERNAL ] SESSIONS command */
	return 0;
    }
    command += 8;

    if ( 0 == STscompare( "formatted", 9, command, 9 ) )
    {
	formatted = 1;
	command += 9;
    }

    if ( 0 == STscompare( "plus", 4, command, 4 ) )
    {
	plus = 1;
	command += 4;
    }

    if ( *command )
    {
	/* Extra characters at end / bad syntax */
	return 0;
    }

    for ( ;
	 an_scb && an_scb != Cs_srv_block.cs_known_list;
	 an_scb = an_scb->cs_next )
    {
	if (show_system == 0 && an_scb->cs_thread_type != CS_USER_THREAD
	    && !( show_internal && CS_INTRNL_THREAD == an_scb->cs_thread_type))
	    continue;

	if (show_user == 0 && an_scb->cs_thread_type == CS_USER_THREAD)
	    continue;

	switch (an_scb->cs_state)
	{
	    case CS_COMPUTABLE:
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		    "Session %x (%24s) cs_state: %w (%d) cs_mask: %v",
			    an_scb->cs_self,
			    an_scb->cs_username,
			    cs_state_names,
			    an_scb->cs_state,
			    an_scb->cs_priority,
			    cs_mask_names,
			    an_scb->cs_mask); 
		if ( plus && (an_scb->cs_mask & CS_MUTEX_MASK) )
		{
		    show_mutex(output_fcn,(CS_SEMAPHORE*)(an_scb->cs_sync_obj));
		}
		break;

	    case CS_FREE:
	    case CS_STACK_WAIT:
	    case CS_UWAIT:
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		    "Session %x (%24s) cs_state: %w cs_mask: %v",
			    an_scb->cs_self,
			    an_scb->cs_username,
			    cs_state_names,
			    an_scb->cs_state,
			    cs_mask_names,
			    an_scb->cs_mask); 
		break;

	    case CS_EVENT_WAIT: 
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		    "session %x (%24s) cs_state: %w (%s) cs_mask: %v",
			    an_scb->cs_self,
			    an_scb->cs_username,
			    cs_state_names,
			    an_scb->cs_state,
			    (an_scb->cs_memory & CS_DIO_MASK ?
				"DIO" :
				an_scb->cs_memory & CS_BIO_MASK ?
				    "BIO" :
				an_scb->cs_memory & CS_LOCK_MASK ?
				    format_lock( an_scb, tbuf ) :
				an_scb->cs_memory & CS_GWFIO_MASK ?
				    "GWF-IO" :
				an_scb->cs_memory & CS_LOG_MASK ?
				    "LOG-IO" :
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
		    "Session %x (%24s) cs_state: %w ((%s) %x) cs_mask: %v",
			    an_scb->cs_self,
			    an_scb->cs_username,
			    cs_state_names,
			    an_scb->cs_state,
			    ((an_scb->cs_mask & CS_SMUTEX_MASK) ? "s" : "x"),
			    an_scb->cs_sync_obj,
			    cs_mask_names,
			    an_scb->cs_mask);
		if ( plus ) 
		    show_mutex(output_fcn,(CS_SEMAPHORE*)(an_scb->cs_sync_obj));
		break;
		
	    case CS_CNDWAIT:
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		    "Session %x (%24s) cs_state: %w (%x) cs_mask: %v",
			    an_scb->cs_self,
			    an_scb->cs_username,
			    cs_state_names,
			    an_scb->cs_state,
			    an_scb->cs_memory,
			    cs_mask_names,
			    an_scb->cs_mask); 
		break;
		
	    default:

		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		    "Session %x (%24s) cs_state: <invalid> %x",
		    an_scb->cs_self,
		    an_scb->cs_username,
		    an_scb->cs_state);
		break;

	}

	if ( plus )
	{
	    TRformat( output_fcn, NULL, buf, sizeof(buf) - 1,
		     "cs_priority: %d., cs_sem_count: %d.",
		     an_scb->cs_priority, an_scb->cs_sem_count );
	}

	if (formatted)
	{
	    if (an_scb->cs_mask & CS_MNTR_MASK)
	    {
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		    "Session %x is a monitor task, owner is pid %x",
			an_scb,
			an_scb->cs_ppid);
	    }
	    else if ((an_scb == &Cs_idle_scb)
			|| (an_scb == (CS_SCB *)&Cs_admin_scb)
			|| (an_scb == &Cs_repent_scb))
	    {
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		"Session %x is an internal task; no info available",
			an_scb);
	    }
	    else
	    {
		(*Cs_srv_block.cs_format)(an_scb,
					    command,
					    powerful, 1);
	    }
	}
    }

    return 1;
}

/*
** History:
**	31-jan-1994 (bryanp) B56960
**	    LRC: "special" should be "system".
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
    else if (CVahxl(command + 6, (u_i4 *)&an_scb))
    {
	TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
	    "Invalid session id");
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
		    "Session %x is a monitor task, owner is pid %x",
			scan_scb,
			scan_scb->cs_ppid);
	    }
	    else if ((scan_scb == &Cs_idle_scb)
			|| (scan_scb == (CS_SCB *)&Cs_admin_scb)
			|| (scan_scb == &Cs_repent_scb))
	    {
		TRformat(output_fcn, NULL, buf, sizeof(buf) - 1,
		"Session %x is an internal task; no info available",
			scan_scb);
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
 	CL_ERR_DESC errdesc;
# if 0
        GCA_LS_PARMS        local_crb;
# endif


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
**
****************************************************************************/

static STATUS
ShowSampler(
	TR_OUTPUT_FCN *output_fcn )

{
	char	timestr[50];
	char	buf[100];
	/*f8	perct[max(MAXTHREADS,MAXMUTEXES+1)];*/
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

        perct = (f8*)MEreqmem( 0, max(MAXTHREADS,MAXMUTEXES+1)*sizeof(f8),TRUE,NULL);


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

	for (i = -1; i < MAXTHREADS; i++)
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
		Cs_srv_block.cs_num_active,
		Cs_srv_block.cs_hwm_active);
	/* 
	** For each thread type tell how much time was spent in each state
	*/

	TRformat(output_fcn, NULL, buf, sizeof(buf)-1,
		"Thread Name          Computable  EventWait  MutexWait   Misc");

	for (i = -1; i < MAXTHREADS; i++)
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
LKCallbackThread,LKDeadlockThread,SamplerThread,SortThread,FactotumThread,Invalid",
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

	    for (j = -1, perct[1] = 0.0; j < MAXTHREADS; j++)
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

	    if (perct[0] > 0 | perct[1] > 0)
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
		for (j = 0; j < MAXEVENTS; j++)
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

		for (j = -1, totmutexes = 0; j < MAXTHREADS; j++)
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
    i4		(*entryfcn)(i4 (*)(PTR, i4, char *), char *);
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
**                      (*output_fcn)(newline_present, length, buffer)
**	                  where buffer is the length character
**	                  output string, and newline_present
**	                  indicates whether a newline needs to
**	                  be added to the end of the string.
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
