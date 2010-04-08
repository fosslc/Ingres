/*
** Copyright (c) 1988, 2004 Ingres Corporation
*/

# include <compat.h>
# include <sp.h>
# include <gl.h>
# include <iicommon.h>
# include <cm.h>
# include <cs.h>
# include <er.h>
# include <ex.h>
# include <lo.h>
# include <me.h>
# include <sp.h>	/* needed by mo.h for splay trees */
# include <mo.h>	/* Managed Objects */
# include <pc.h>
# include <lk.h>
# include <st.h>
# include <tr.h>
# include <cv.h>
# include <pc.h>
# include <csinternal.h>
# include "cslocal.h"
# include "cssampler.h"

/*
** The following lock type definitions are to be used as the first element
** of a lock key of type LK_AUDIT.
** This is taken from front/st/ipm/dba.h
*/
# define        SXAS_LOCK               1
# define        SXAP_LOCK               2
# define        SXLC_LOCK               3       /* SXL cache lock */

/*
** These are sub-lock id types used for the SXAS state lock
*/
# define        SXAS_STATE_LOCK         1       /* Main state lock */
# define        SXAS_SAVE_LOCK          2       /* Save lock */
 
/*
** These are sub-lock id types for the SXAP physical lock
*/
# define        SXAP_SHMCTL     1
# define        SXAP_ACCESS     2
# define        SXAP_QUEUE      3

GLOBALREF CS_SYSTEM		Cs_srv_block;
GLOBALREF HANDLE		hCsSamplerSem;
GLOBALREF CSSAMPLERBLKPTR	CsSamplerBlkPtr;
GLOBALREF char 			CSsamp_index_name[];
GLOBALREF bool			CSsamp_stopping;

FUNC_EXTERN VOID CS_dump_stack();
FUNC_EXTERN CS_SCB *CS_find_scb(CS_SID sid);

HANDLE	hCsSamplerThread = NULL;
DWORD	CsSamplerThreadId = 0;

static void show_sessions(
		char	*command,
		VOID	(*ouput_fcn)(),
		i4 	powerful);

static void format_sessions(
		char	*command,
		VOID	(*output_fcn)(),
		i4 	powerful);

static STATUS StartSampler(
		char    *command,
		VOID	(*output_fcn)() );

static STATUS StopSampler(
		VOID	(*output_fcn)() );

static STATUS ShowSampler(
		VOID	(*output_fcn)() );

static void FormatLockName( LK_LOCK_KEY * lockkeyptr,
				char * name);


/*
**  Name: CSmonitor  - Implement the monitor task
**
**  Description:
**	This routine is called ala the regular thread processing routine.
**	It parses its input, decides what to do, and returns the output.
**
**  Inputs:
**	mode		Mode of operation CS_INPUT, _OUPUT, ...
**	scb		Sessions control block to monitor
**	*command	Text of the command
**	powerful	Is this user powerful
**	output_fcn	Function to call to perform the output.
**			This routine will be called as
**			    (*output_fcn)(newline_present, length, buffer)
**			where buffer is the length character
**			output string, and newline_present
**			indicates whether a newline needs to
**			be added to the end of the string.
**
**  Outputs:
**	next_mode	As above.
**
**	Returns:
**	    OK
**
**	Exceptions:
**	    none
**
**  Side Effects:
**	none
**
**  History:
**      04-aug-95 (sarjo01)
**	    Increased size of buf[] to 255, was getting overrun by
**          "show server" command output; for "show server" added to format
**	    string dummy values for quantums, ready mask, etc. to satisfy
**	    IPM, which takes apart output field-by-field (and dies if
**	    something is missing).
**	    NOTE: this new rev (5) actually undoes everything changed to
**	    create rev 4.
**	08-aug-1995 (canor01)
**	    cs_state values were not being being reported correctly
**	09-aug-95 (shero03)
**	    use csinternal rather than csintern
**	10-aug-95 (tutto01)
**	    STscompare for 'show server' was incorrect, so this command
**	    never worked.
**	21-aug-1995 (shero03)
**	    Avoid formatting the main-job task
**	08-sep-1995 (shero03)
**	    Add the NT sampler thread
**	    Support start sampling, show sampling, and stop sampling
**	20-sep-1995 (shero03)
**	    Change the sampling report to show tenths of a percent and 
**	    use absolute percentages instead of relative percentages.
**	10-jan-1996 (canor01)
**	    Show name of mutex when in CS_MUTEX state.
**	08-feb-1996 (canor01)
**	    Show LKEVENT, LGEVENT and LOG waits in sampler output.
**	29-apr-1996 (canor01)
**	    When removing a session, report the SID, not the scb address.
**	23-sep-1996 (canor01)
**	    Move global data definitions to csdata.c.
**	12-feb-1997 (canor01)
**	    Fix two instances of calls to UnlockSamplerBlk() that were
**	    passing a pointer to a handle instead of the handle.
**	16-Oct-1997 (shero03)
**	    Synched the sampler code with csmtmonitor & change 432396.
**	22-Oct-1997 (shero03)
**	    Don't get the time if there is no output function
**      7-jan-98 (stephenb)
**          Format the output for "show sessions" better and add the 
**          active users highwater mark in preparation for user based licensing
**      8-jan-98 (stephenb)
**          switch high water mark in above chage to appear after max active
**          so that IPM is not confused, also break up long displya line
**          to pervent piccolo from baulking.
**	08-Jul-1998 (shero03)
**	    Add new thread types for the sampler.
** 	01-Dec-1998 (wonst02)
** 	    Attach/detach the sampler block Managed Object.
** 	07-Jan-1999 (wonst02)
** 	    Add new event types: CS_LIO_MASK (Log I/O), et al.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	24-aug-1999 (somsa01)
**	    Moved the calls to MOattach() and MOdetach() that were in
**	    StartSampler() and StopSampler() to the actual Sampler thread
**	    routine. In this way, the MO semaphores would not be locked by
**	    this current thread (deadlock).
**	15-dec-1999 (somsa01 for jenj02, part II)
**	    Standardize cs_self = (CS_SID *)scb, session id's are always
**	    a pointer to the thread's SCB, never OS thread_id, which is
**	    contained in cs_thread_id.
**	    Added sampler display of server CPU time and wait counts.
**	    Added "show <...> sessions stats" to act as a visual aid
**	    to see what, if any, progress the sessions are
**	    making in their quest.
**	10-jan-2000 (somsa01)
**	    Added more statistics printout. Also, print out the interval
**	    on fresh sampler startup. Also added the "showmutex", "debug",
**	    and "stacks" commands.
**	24-jul-2000 (somsa01 for jenjo02)
**	    Removed cs_sem_init_addr from CS_SEMAPHORE, replaced cs_sid with
**	    cs_thread_id for OS threads. Show mutex owner thread_id when
**	    displaying mutex info.
**	    When displaying session ids for OS threads, append ":thread_id".
**	10-oct-2000 (somsa01 for jenjo02)
**	    Sampler improvements: break out event wait percentages
**	    by thread type, show DB id in lock id, track and
**	    display I/O and transaction rates, fix mutex and lock
**	    wait logic. Lump all non-user lock waits into one bucket;
**	    "system" threads should never wait on locks.
**	13-oct-2001 (somsa01)
**	    For LP64, use CVahxi64().
**	23-jan-2004 (somsa01 for jenjo02)
**	    Resolve long-standing inconsistencies and inaccuracies with
**	    "cs_num_active" by computing it only when needed by MO
**	    or IIMONITOR. Added MO methods, external functions
**	    CS_num_active(), CS_hwm_active(), callable from IIMONITOR,
**	    e.g. Works for both MT and Ingres-threaded servers.
**	03-feb-2004 (somsa01 for hanch04)
**	    Add more help, suspend, resume, closed, open.
**	03-feb-2004 (somsa01 for jenjo02)
**	    Added "kill query" to help syntax, SIR 110141.
**	14-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	20-Jul-2004 (lakvi01)
**		SIR# 112703, Cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	18-Jul-2005 (drivi01)
**	    Modified routines for format_sessions to print out 
**	    scan_scb->cs_thread_id instead of an_scb->cs_thread_id
**	    b/c an_scb will be equal to NULL in most cases causing
**	    these routines to catch exception.
**	08-Feb-2007 (kiria01) b117581
**	    Use global cs_mask_names and cs_state_names.
**      23-Apr-2009 (hanal04) Bug 132714
**          Update iimonitor help output to include SAMPLING commands.
**      23-Sep-2009 (hanal04) Bug 115316
**          Added "SHOW SERVER CAPABILITIES".
**      29-Jan-2010 (smeke01) Bug 123192
**          For 'show smstats' and 'show mutex', display u_i4 values with
**          format specifier %10u rather than %8d.
*/
/* ARGSUSED */
static          STATUS
mntr_handler(EX_ARGS * args)
{
	return (EXDECLARE);
}

STATUS
CSmonitor(i4     mode,
          CS_SCB *scb,
          i4     *nmode,
          char   *command,
          i4     powerful,
          VOID   (*output_fcn) ())
{
	STATUS		status;
	i4		count;
	OFFSET_TYPE	session;
	i4 		lencommand;
	char		buf[255];
	char		*ptr;
	EX_CONTEXT	ex_context;
	CS_SCB		*an_scb;
	CS_SCB		*scan_scb;
	CS_SEMAPHORE	*mutex;
	PTR		ptr_tmp;

	*nmode = CS_EXCHANGE;

	/* zap off any trailing cr chars */

	ptr = command + strlen(command) - 1;
	if (*ptr == '\r')
	    *ptr = '\0';

	if (EXdeclare(mntr_handler, &ex_context))
	{
	    EXdelete();
	    return (FAIL);
	}

	switch (mode)
	{
	    case CS_INITIATE:
	    	*nmode = CS_INPUT;

	    case CS_TERMINATE:
	    	break;

	    case CS_INPUT:
	    	CVlower(command);
	        STzapblank(command, command);
		lencommand = (i4)strlen(command);


		if ( (lencommand >= 4) &&
		     (STbcompare("help", 4, command, 4, FALSE) == 0) )
		{
		    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf) - 1,
		    "Server:\n    SET SERVER [SHUT | CLOSED | OPEN]\n\
    STOP SERVER\n    \
SHOW SERVER [LISTEN | SHUTDOWN | CAPABILITIES]\n\
    START SAMPLING [milli second interval]\n\
    STOP SAMPLING\n\
    SHOW SAMPLING\n");
		    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf) - 1,
		    "Session:\n    SHOW [USER] SESSIONS [FORMATTED | STATS]\n\
    SHOW SYSTEM SESSIONS [FORMATTED | STATS]\n\
    SHOW ALL SESSIONS [FORMATTED | STATS]\n\
    FORMAT [ALL | sid]\n    REMOVE [sid]\n    SUSPEND [sid]\n\
    RESUME [sid]\n\nQuery:\n    KILL [sid]\n\nOthers:\n    QUIT\n\n");
		}
		else if ( (lencommand >= 21) &&
			   (STbcompare("stopserverconditional", 21, command, 21, FALSE) == 0))
		{
		    if (!powerful)
		    {
		        TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		    	         "Superuser status required to stop servers", 0L);
		    }
		    else {
		        StopSampler(NULL);
		        status = CSterminate(CS_COND_CLOSE, &count);
		        if (status)
			{
		    	    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		    	    	"Error stopping server, %d. sessions remaining",
		    	    	 count);
		    	}
		    }
	       	}

		else if ( (lencommand >= 10) &&
			  (STbcompare("stopserver", 10, command, 10, FALSE) == 0) )
		{
		    if (!powerful)
		    {
			TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
			    "Superuser status required to stop servers", 0L);
		    }
		    else
		    {
		        StopSampler(NULL);
			status = CSterminate(CS_KILL, &count);
			if (status)
			{
			    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf) - 1,
			  	"Server will stop. %d. sessions aborted",
			         count);
			}
		    }
		}

		else if ( (lencommand >= 12) &&
			  (STbcompare("setservershut", 12, command, 12, FALSE) == 0) )
	 	{
		    if (!powerful)
		    {
		    	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		    	    "Superuser status required to stop servers", 0L);
		    }
		    else {
		        StopSampler(NULL);
		    	status = CSterminate(CS_CLOSE, &count);
		    	if (status) {
		    	    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf) - 1,
			    	   "Server will stop. %d. sessions remaining",
			    	    count);
			}
		    }
		}


		else if ( (lencommand >= 12) &&
			  (STbcompare("showsessions", 12, command, 12, FALSE) == 0 ||
			   STbcompare("showallsessions", 15, command, 15, FALSE) == 0 ||
			   STbcompare("showusersessions", 16, command, 16, FALSE) == 0 ||
			   STbcompare("showsystemsessions", 18, command, 18, FALSE) == 0) )
	    	{
		    show_sessions(command, output_fcn, powerful);
		}
			
		else if ( (lencommand >= 9) &&
			  (STbcompare("showmutex", 9, command, 9, FALSE) == 0) )
		{
		    if ((CVaxptr(command + 9, &ptr_tmp)) ||
			(ptr_tmp  < (PTR)0x2000))
		    {
			TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
				 "Invalid mutex id %p", ptr_tmp);
			break;
		    }
		    else
			mutex = (CS_SEMAPHORE *) ptr_tmp;

		    if (mutex->cs_sem_scribble_check != CS_SEM_LOOKS_GOOD &&
			mutex->cs_sem_scribble_check != CS_SEM_WAS_REMOVED)
		    {
			TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
				 "Structure at %p does not look like a mutex\n",
				 mutex);
			break;
		    }

		    if ( mutex->cs_value )
		    {
			TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
		"Mutex at %p: Name: %s, EXCL owner: (tid: %x, pid: %d)\n",
				 mutex,
				 mutex->cs_sem_name[0] ? mutex->cs_sem_name :
					"(UN-NAMED)",
				 mutex->cs_thread_id,
				 mutex->cs_pid,
				 mutex->cs_count);
		    }
		    else if ( mutex->cs_count )
		    {
			TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
		"Mutex at %p: Name: %s, SHARE owner: (tid: %x, pid: %d)\n",
				 mutex,
				 mutex->cs_sem_name[0] ? mutex->cs_sem_name :
					"(UN-NAMED)",
				 mutex->cs_thread_id,
				 mutex->cs_pid,
				 mutex->cs_count);
		    }
		    else
		    {
			TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
				"Mutex at %p: Name: %s, is currently unowned\n",
				mutex,
				mutex->cs_sem_name[0] ? mutex->cs_sem_name :
					"(UN-NAMED)");
		    }

		    TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
			"    Shared: %10u Collisions: %10u Hwm: %10u \n      Excl: %8d Collisions: %10u \n",
			     mutex->cs_smstatistics.cs_sms_count,
			     mutex->cs_smstatistics.cs_smsx_count,
			     mutex->cs_smstatistics.cs_sms_hwm,
			     mutex->cs_smstatistics.cs_smx_count,
			     mutex->cs_smstatistics.cs_smxx_count);
		}
		else if ( (lencommand >= 10) &&
			  (STbcompare("showserver", 10, command, 10, FALSE) == 0) )
		{
                TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
                    "\r\n\tServer: %s\r\n\r\nConnected Sessions (includes \
system threads)\r\n\r\n\tCurrent:\t%d\r\n\tLimit:\t\t%d\r\n\r\nActive \
Sessions\r\n\r\n\tCurrent\t\t%d\r\n\tLimit\t\t%d\r\n\tHigh \
Water\t%d\r\n\r\nrdy mask %x state: %x\r\nidle quantums %d./%d. (%d.%%)",
                    Cs_srv_block.cs_name,
                    Cs_srv_block.cs_num_sessions, Cs_srv_block.cs_max_sessions,
                    CS_num_active(), Cs_srv_block.cs_max_active,
                    Cs_srv_block.cs_hwm_active,
                    Cs_srv_block.cs_ready_mask,
                    Cs_srv_block.cs_state,
                    0,
                    0,
                    0);
		}
  
		else if ( (lencommand >= 11) &&
			  (STbcompare("showsmstats", 11, command, 11, FALSE) == 0) )
		{
		    char	place[1024];
		    char	*buffer = place;

                    STprintf(buffer, "Semaphore Statistics:\r\n");
                    buffer += STlength(buffer);
                    STprintf(buffer, "--------- -----------\r\n");
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
                    STprintf(buffer, "--------- -----------\r\n");
                    TRformat(output_fcn, 1, buf, sizeof(buf) - 1, place);

		    break;
		}

		else if ( (lencommand >= 12) &&
			  (STbcompare("clearsmstats", 12, command, 12, FALSE) == 0) )
		{
		    if (!powerful)
		    {
		    	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf) - 1,
		    		"Superuser status required to clear semaphore statistics", 0L);
		    	break;
		    }
                    Cs_srv_block.cs_smstatistics.cs_smss_count = 0;
                    Cs_srv_block.cs_smstatistics.cs_smssx_count = 0;
                    Cs_srv_block.cs_smstatistics.cs_smsx_count = 0;
                    Cs_srv_block.cs_smstatistics.cs_smsxx_count = 0;
                    Cs_srv_block.cs_smstatistics.cs_smms_count = 0;
                    Cs_srv_block.cs_smstatistics.cs_smmsx_count = 0;
                    Cs_srv_block.cs_smstatistics.cs_smmx_count = 0;
                    Cs_srv_block.cs_smstatistics.cs_smmxx_count = 0;
		    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf) - 1,
		    		"Semaphore statistics cleared\n", 0L);
		    TRdisplay("========\n***** WARNING: Semaphore statistics cleared at %@ by %24s\n=======\n",
		    	scb->cs_username);
		    break;
		}
  
		else if ( (lencommand >= 6) &&
			  (STbcompare("format", 6, command, 6, FALSE) == 0) )
		{
		    format_sessions(command, output_fcn, powerful);
		}

		else if ( (lencommand >= 12) &&
			  (STbcompare("startsampling", 12, command, 12, FALSE) == 0) )
		{
		    if (!powerful)
		    {
			TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
			    "Superuser status required to start sampling.", 0L);
		    }
		    else
		    {
			status = StartSampler(command, output_fcn);
		    	if (status)
		    	{
		    	    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf) - 1,
			    	   "Sampling failed to start.");
			}
		    }
		}

 		else if ( (lencommand >= 11) &&
 			  (STbcompare("stopsampling", 11, command, 11, FALSE) == 0) )
		{
		    if (!powerful)
		    {
			TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
			    "Superuser status required to stop sampling.", 0L);
		    }
		    else
		    {
			status = StopSampler(output_fcn);
		    	if (status)
		    	{
		    	    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf) - 1,
			    	   "Sampling failed to stop.", 0L);
			}
		    }
		}

		else if ( (lencommand >= 10) &&
			  (STbcompare("showsampling", 10, command, 10, FALSE) == 0) )
		{
		    if (!powerful)
		    {
			TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
			    "Superuser status required to show sampling.", 0L);
		    }
		    else
		    {	
		        status = ShowSampler(output_fcn);
		        if (status)
		        {
		       	     TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf) - 1,
		       	   	"Show sampling failed.", 0);
		        }
		    }
		}

		else if ( (lencommand >= 10) &&
			  (STbcompare("zapsession", 10, command, 10, FALSE) == 0) )
		{
		    if (!powerful)
		    {
		    	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		    	  "Superuser status required to zap sessions", 0L);
		    	break;
		    }
#if defined(LP64)
		    if (CVahxi64(command + 6, &session))
#else
		    if (CVahxl(command + 6, (ULONG *) & session))
#endif  /* LP64 */
		    {
		    	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		    		"Invalid session id", 0L);
		    	break;
		    }
		    if ((session == CS_ADDER_ID) ||
                        (an_scb = CS_find_scb(session)) == 0)
		    {
		    	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		    		"Invalid session id", 0L);
			break;
		    }
		    TerminateThread((HANDLE) an_scb->cs_thread_handle, 0);
		    CloseHandle((HANDLE) an_scb->cs_thread_handle);
	        }

		else if ( (lencommand >= 6) &&
			  (STbcompare("remove", 6, command, 6, FALSE) == 0) )
		{
		    if (!powerful)
		    {
		    	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		    		"Superuser status required to remove sessions", 0L);
		    	break;
		    }
#if defined(LP64)
		    if (CVahxi64(command + 6, &session))
#else
		    if (CVahxl(command + 6, (ULONG *) & session))
#endif  /* LP64 */
		    {
		    	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		         	"Invalid session id", 0L);
			break;
		    }
		    if ((session == CS_ADDER_ID) ||
                        (an_scb = CS_find_scb(session)) == 0)
		    {
		    	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		    		"Invalid session id", 0L);
			break;
		    }
		    else
		    {
		    	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		    	    	"Session %x removed", an_scb->cs_self);
		    	CSattn(CS_REMOVE_EVENT, an_scb->cs_self);
		    	CSremove(an_scb->cs_self);
		    }
		}

		else if ( (lencommand >= 7) &&
			  (STbcompare("suspend", 7, command, 7, FALSE) == 0) )
		{
		    if (!powerful)
		    {
		    	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		    		"Superuser status required to suspend sessions", 0L);
		    	break;
		    }
#if defined(LP64)
		    if (CVahxi64(command + 7, &session))
#else
		    if (CVahxl(command + 7, (ULONG *) & session))
#endif  /* LP64 */
		    {
		    	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		        	"Invalid session id", 0L);
			break;
		    }
		    if ((session == CS_ADDER_ID) ||
                        (an_scb = CS_find_scb(session)) == 0)
		    {
		    	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		    		"Invalid session id", 0L);
		    	break;
		    }
		    else if (an_scb->cs_state != CS_COMPUTABLE)
		    {
		    	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		    		"Session %x is not computable -- not suspended",
		    	 	an_scb->cs_self);
		    }
		    else
		    {
		        an_scb->cs_state = CS_UWAIT;
		    	SuspendThread((HANDLE) an_scb->cs_thread_handle);
		    	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		    		"Session %x has been indefinitely suspended",
		    	 	an_scb->cs_self);
		    }
		}

		else if ( (lencommand >= 6) &&
			  (STbcompare("resume", 6, command, 6, FALSE) == 0) )
		{
		    if (!powerful)
		    {
		    	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		    		"Superuser status required to resume sessions", 0L);
		    	break;
		    }
#if defined(LP64)
		    if (CVahxi64(command + 6, &session))
#else
		    if (CVahxl(command + 6, (ULONG *) & session))
#endif  /* LP64 */
		    {
		    	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		    		"Invalid session id", 0L);
		    	break;
		    }
		    if ((an_scb = CS_find_scb(session)) == 0) {
		    	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		    		"Invalid session id", 0L);
		    }
		    else if (an_scb->cs_state != CS_UWAIT) {
		    	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		    		"Session %x was not suspended",
		    	 	an_scb->cs_self);
		    }
		    else {
		    	an_scb->cs_state = CS_COMPUTABLE;
		    	ResumeThread((HANDLE) an_scb->cs_thread_handle);
		    	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		    	    	"Session %x has been resumed",
		    	 	an_scb->cs_self);
		    }
		}
		else if ( (lencommand >= 5) &&
			  (STbcompare("debug", 5, command, 5, FALSE) == 0) )
		{
		    STzapblank(command, command);
		    if (CVaxptr(command + 5, &ptr_tmp))
		    {
			TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
				 "Invalid session id");
			break;
		    }
		    if ((an_scb = CS_find_scb((CS_SID) ptr_tmp)) == 0)
		    {
			TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
				 "Invalid session id");
			break;
		    }
		    else
		    {
			char	buffer[1048];

			CS_fmt_scb(an_scb, sizeof(buffer), buffer);
			TRformat(output_fcn, 1, buf, sizeof(buf) - 1, buffer);
		    }

		    CSget_scb(&scan_scb);
		    if (scan_scb != an_scb)
			CS_dump_stack(an_scb, 0, output_fcn, FALSE);
		    else
			CS_dump_stack(0, 0, output_fcn, FALSE);
		}
		else if ( (lencommand >= 6) &&
			  (STbcompare("stacks", 6, command, 6, FALSE) == 0) )
		{
		    for (scan_scb = Cs_srv_block.cs_known_list->cs_next;
			 scan_scb && scan_scb != Cs_srv_block.cs_known_list;
			 scan_scb = scan_scb->cs_next)
		    {
			TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
				 "\n---\nStack dump for session %x\n",
				 scan_scb->cs_self);
			CSget_scb(&an_scb);
			if (an_scb != scan_scb)
			    CS_dump_stack(scan_scb, 0, output_fcn, FALSE);
			else
			    CS_dump_stack(0, 0, output_fcn, FALSE);
		    }
		}
		else if ( (lencommand >= 11) &&
			  (STbcompare("crashserver", 11, command, 11, FALSE) == 0) )
		{
		    if (!powerful)
		    {
			TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
			    "Superuser status required to stop servers", 0L);
		    }
		    else {
		        StopSampler(NULL);
		    	status = CSterminate(CS_CRASH, &count);
		    }
		}

/*  
        	else if (STbcompare("showmemstats", 11, command, 0, FALSE) == 0)
        	{
        	     TRformat(output_fcn, (i4 *)1, buf, sizeof(buf) - 1,
    		     	 "MEreq: %d (alloc=%d) MEget: %d (alloc=%d) MEfree: %d MEf_pg: %d\n",
            	         get_MEreqmem_calls(), get_MEreqmem_memory_used(),
            	         get_MEget_pages_calls(), get_MEget_pages_memory_used(),
            		 get_MEfree_calls(), get_MEfree_pages_calls() );
        	     TRformat(output_fcn, (i4 *)1, buf, sizeof(buf) -1,
            	         "MEsbrk: %d (alloc=%d)\n",
            		 get_MEsbrk_calls(), get_MEsbrk_memory_used() );
        		 TRformat(output_fcn, (i4 *)1, buf, sizeof(buf) -1,
            		 "Stack: %d\n", Cs_srv_block.cs_stk_count);
        	}

*/

		else
		{
		    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf) - 1,
		    	"Illegal command\n", 0L);
		}
		break;
  
	    case CS_OUTPUT:
	        break;
	}

	EXdelete();
	return (OK);

}

/****************************************************************************
**
** Name: show_sessions - Implement the various show sessions 
**
** Description:
**      This is a local routine that takes care of displaying the session
**      information.                                                    
**
** Inputs:
**  *command        Text of the command
**			show sessions [formatted]
**			show all sessions [formatted] 
**			show user sessions [formatted] 	
**			show system sessions [formatted]
**
**  powerful        Is this user powerful
**  output_fcn      Function to call to perform the output.
**                  This routine will be called as
**                      (*output_fcn)(newline_present, length, buffer)
**                  where buffer is the length character
**                  output string, and newline_present
**                  indicates whether a newline needs to
**                  be added to the end of the string.
**
**  Returns:
**      OK
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
**	15-dec-1999 (somsa01)
**	    Added printout of session statistics.
**	10-jan-2000 (somsa01)
**	    Added more statistics printout.
**	01-jul-2008 (smeke01)
**	    Corrected typo in "show user sessions" option that 
**	    prevented the "stats" option from working.
**	09-Feb-2009 (smeke01) b119586
**	    Used new function CSMT_get_thread_stats() to get CPU 
**	    stats output to work without the overhead of having 
**	    session_accounting turned on. 
**
****************************************************************************/

static void
show_sessions(
	char	*command,
	VOID	(*output_fcn) (),
	i4 	powerful)
{
	CS_SCB	*an_scb;
	char	buf[132];
	i4 	show_user = 0;
	i4 	show_system = 0;
	i4 	show_format = 0;	 
	i4 	lencommand;
	i4	show_stats = 0;
	CS_THREAD_STATS_CB	thread_stats_cb;
	STATUS  status;

	lencommand = STlength(command);
	if (STbcompare("showsessions", 12, command, 12, FALSE) == 0)
	{
	    show_user = 1;
	    if (lencommand > 12)
	    {
		if (STbcompare("formatted",9, command+12, 9, FALSE) == 0)
		    show_format = 1;
		else if (STbcompare("stats", 5, command+12, 5, FALSE) == 0)
		    show_stats = 1;
	    }
	}

	else if (STbcompare("showallsessions", 15, command, 15, FALSE)  == 0)
	{
	    show_user = 1;
	    show_system = 1;
	    if (lencommand > 15)
	    {
		if (STbcompare("formatted",9, command+15, 9, FALSE) == 0)
		    show_format = 1;
		else if (STbcompare("stats", 5, command+15, 5, FALSE) == 0)
		    show_stats = 1;
	    }
	}

	else if (STbcompare("showusersessions", 16, command, 16, FALSE)  == 0)
	{
	    show_user = 1;
	    if (lencommand > 16)
	    {
		if (STbcompare("formatted",9, command+16, 9, FALSE) == 0)
		    show_format = 1;
		else if (STbcompare("stats", 5, command+16, 5, FALSE) == 0)
		    show_stats = 1;
	    }
	}
	else if (STbcompare("showsystemsessions", 18, command, 18, FALSE) == 0)
        {
	    show_system = 1;
    	    if (lencommand > 18)
	    {
		if (STbcompare("formatted",9, command+18, 9, FALSE) == 0)
		    show_format = 1;
		else if (STbcompare("stats", 5, command+18, 5, FALSE) == 0)
		    show_stats = 1;
	    }
	}
		
	for (an_scb = Cs_srv_block.cs_known_list->cs_next;
	     an_scb && an_scb != Cs_srv_block.cs_known_list;
	     an_scb = an_scb->cs_next)
	{

	    if (show_user == 0 && an_scb->cs_thread_type == CS_USER_THREAD)
	        continue;

	    if (show_system == 0 && an_scb->cs_thread_type != CS_USER_THREAD)
	        continue;

	    if (show_stats)
	    {
		char	tbuf[20], sbuf[132];

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

		thread_stats_cb.cs_thread_handle = an_scb->cs_thread_handle; 
		thread_stats_cb.cs_stats_flag = CS_THREAD_STATS_CPU; 
		if (CSMT_get_thread_stats(&thread_stats_cb) == OK)
		{ 
		    STprintf(tbuf, "CPU u:%d s:%d", thread_stats_cb.cs_usercpu, thread_stats_cb.cs_systemcpu);
		    STcat(sbuf,tbuf);
		}

		TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
		    "Session %p:%d (%24s) %s\n",
		    an_scb->cs_self,
		    an_scb->cs_thread_id,
		    an_scb->cs_username,
		    sbuf);
	    }
	    else switch (an_scb->cs_state)
	    {
		case CS_FREE:
		case CS_COMPUTABLE:
		case CS_STACK_WAIT:
		case CS_UWAIT:
		    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf) - 1,
		    	"Session %p:%d (%24s) cs_state: %w (%s) cs_mask: %v",
		    	an_scb->cs_self,
			an_scb->cs_thread_id,
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
				"LOCK" :
			    an_scb->cs_memory & CS_LOG_MASK ?
				"LOG" :
			    an_scb->cs_memory & CS_LKEVENT_MASK ?
				"LKEVENT" :
			    an_scb->cs_memory & CS_LGEVENT_MASK ?
				"LGEVENT" :
			    an_scb->cs_memory & CS_TIMEOUT_MASK ?
				"TIME"    :
				"<any>"),
			cs_mask_names,
		    	an_scb->cs_mask);
		    break;

		case CS_EVENT_WAIT:
		    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf) - 1,
		    	"Session %p:%d (%24s) cs_state: %w (%s) cs_mask: %v",
		    	an_scb->cs_self,
			an_scb->cs_thread_id,
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
				"LOCK" :
			    an_scb->cs_memory & CS_LOG_MASK ?
				"LOG" :
			    an_scb->cs_memory & CS_LKEVENT_MASK ?
				"LKEVENT" :
			    an_scb->cs_memory & CS_LGEVENT_MASK ?
				"LGEVENT" :
			    an_scb->cs_memory & CS_TIMEOUT_MASK ?
				"TIME"    :
				"<any>"),
			cs_mask_names,
		    	an_scb->cs_mask);
		    break;

		case CS_MUTEX:
		    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf) - 1,
			"Session %p:%d (%24s) cs_state: %w ((%s) %p) cs_mask: %v",
			an_scb->cs_self,
			an_scb->cs_thread_id,
			an_scb->cs_username,
			cs_state_names,
			an_scb->cs_state,
			"x",
			an_scb->cs_sync_obj,
			((CS_SEMAPHORE*)(an_scb->cs_sync_obj))->cs_sem_name,
			cs_mask_names,
			an_scb->cs_mask);
		    if (an_scb->cs_sync_obj)
		    {
			TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
			"        Mutex: %s",
			((CS_SEMAPHORE *)an_scb->cs_sync_obj)->cs_sem_name[0] ?
			((CS_SEMAPHORE *)an_scb->cs_sync_obj)->cs_sem_name :
			"(UN-NAMED)");
		    }
		    break;

		case CS_CNDWAIT:
		    TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
		    	"Session %p:%d (%24s) cs_state: %w (%p) cs_mask: %v",
		    	an_scb->cs_self,
			an_scb->cs_thread_id,
		    	an_scb->cs_username,
		    	cs_state_names,
		    	an_scb->cs_state,
		    	an_scb->cs_sync_obj,
			cs_mask_names,
		    	an_scb->cs_mask);
		    break;

		default:

		    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf) - 1,
		    	"session %p (%24s) cs_state: <invalid> %x",
		    	an_scb->cs_self,
		    	an_scb->cs_username,
		    	an_scb->cs_state);
		    break;
	    }

	    if (show_format)
	    {
	        if (an_scb->cs_mask & CS_MNTR_MASK)
	        {
	            TRformat(output_fcn, 1, buf, sizeof(buf) - 1,
			 "Session %p:%d is a monitor task; no information is available",
			 an_scb, an_scb->cs_thread_id);
	        }
		else if (an_scb->cs_self == CS_ADDER_ID)
		    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		        "Session %p:%d is an internal task; no information is available",
			an_scb->cs_self, an_scb->cs_thread_id);
	        else
	        {
	 	    status = (*Cs_srv_block.cs_format) (an_scb, command, powerful);
		    if (status)
			TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
			"Session %p:%d does not have any displayable information",
			an_scb->cs_self, an_scb->cs_thread_id);
	        }
	    }			
	}

	return;
}

/****************************************************************************
**
** Name: format_sessions - Implement the various format commands
**
** Description:
**      This is a local routine that takes care of displaying the formatted
**      information.                                                    
**
** Inputs:
**  scb             Sessions control block to monitor
**  *command        Text of the command
**  powerful        Is this user powerful
**  output_fcn      Function to call to perform the output.
**                  This routine will be called as
**                      (*output_fcn)(newline_present, length, buffer)
**                  where buffer is the length character
**                  output string, and newline_present
**                  indicates whether a newline needs to
**                  be added to the end of the string.
**
**  Returns:
**      OK
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
**	06-oct-1995 (canor01)
**	    scs_format (*Cs_srv_block.cs_format) requires four parameters to
**	    correctly format the information.
**
****************************************************************************/

static void
format_sessions(
	char    *command,
	VOID    (*output_fcn) (),
	i4 	powerful)
{
	CS_SCB		*an_scb;
	CS_SCB  	*scan_scb;
	char		buf[100];
	i4 		show_user = 0;
	i4 		show_system = 0;
	i4 		found_scb = 0;
	OFFSET_TYPE 	session;
	STATUS		status;
		
	/*
	** Syntax:
	**      format xxxxxxxx			- format the indicated session
	**	format all			- format ALL sessions
	**	format all sessions		- ditto
	**	format user			- format USER sessions
	**	format user sessions		- ditto
	**	format system			- format system sessions
	**	format system sessions		- ditto
	*/

	if (STlength(command) == 6)
	{
	    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf) - 1,
			"Invalid session id", 0L);
	    return;
	}

	if (STbcompare("all", 3, command+6, 3, FALSE) == 0)
	{
	    an_scb = 0;
	    show_user = 1;
	    show_system = 1;
	}	
	else if (STbcompare("users", 4, command+6, 4, FALSE) == 0 ||
	         STbcompare("sessions", 8, command+6, 8, FALSE) == 0)
	{
	    an_scb = 0;
	    show_user = 1;
	}
	else if (STbcompare("system", 6, command+6, 6, FALSE) == 0)
	{
	    an_scb = 0;
	    show_system = 1;
	}
	else
	{
#if defined(LP64)
	    if (CVahxi64(command + 6, &session))
#else
	    if (CVahxl(command + 6, &session))
#endif  /* LP64 */
	    {
	        TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf) - 1,
			"Invalid session id", 0L);
		return;
	    }
	    else
	    {
		if ((an_scb = CS_find_scb(session)) == 0)
		{
		    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf) - 1,
		         "Invalid session id", 0L);
		    return;
	        }
	    }

	}
		
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
		    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf) - 1,
		       "Session %p:%d is a monitor task; no information is available",
		        scan_scb->cs_self, scan_scb->cs_thread_id);
		}
		else if (scan_scb->cs_self == CS_ADDER_ID)
		{
		    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf) - 1,
			"Session %p:%d is an internal task; no information is available",
			scan_scb->cs_self, scan_scb->cs_thread_id);
		}
		else
		{
	 	    status = (*Cs_srv_block.cs_format) (scan_scb, 
							command, 
							powerful, 0);
		    if (status)
			TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
			"Session %p:%d does not have any displayable information",
			scan_scb->cs_self, scan_scb->cs_thread_id);
		}
	    }
	}

	return;
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
**                      (*output_fcn)(newline_present, length, buffer)
**                  where buffer is the length character
**                  output string, and newline_present
**                  indicates whether a newline needs to
**                  be added to the end of the string.
**
**  Returns:
**      OK
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
**	24-aug-1999 (somsa01)
**	    Moved the call to MOattach() to the actual Sampler thread
**	    routine. In this way, the MO semaphores would not be locked by
**	    this current thread (deadlock).
**	15-dec-1999 (somsa01)
**	    Added initialization of Server CPU usage and wait counts.
**	10-jan-2000 (somsa01)
**	    Print out the sampler interval on fresh startup. Also, if
**	    CsSamplerBlkPtr->shutdown is set, release hCsSamplerSem and
**	    try again until CsSamplerBlkPtr is NULL.
**       7-May-2009 (hanal04) Bug 122034
**          Adjust TRformat of sampling interval to handle 100.00 which
**          is valid.
**
****************************************************************************/

static STATUS
StartSampler(
	char    *command,
	VOID    (*output_fcn) () )

{
	char	buf[100];
	i4 	interval;
	SYSTIME	time;
	char	timestr[50];
	STATUS  status = OK;
	void	CS_sampler();

	/*
	** Syntax:
	**      start sampling xxxxxxxx		
	**	(xxxxxxx - the interval between samplings in milli seconds)
	**	(xxxxxxx >= 100 and <= 100000 .1 seconds - 100 seconds)
	**	(if xxxxxxxxx is ommitted, then assume 1000 or 1 second.)
	*/
	if (CVal(command + 13, &interval))
	{
	    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
	   		"The sampling interval must be between 100 and 100000 (milliseconds)", 0L);
	    return(FAIL);
	}
	if (interval == 0)
	    interval = 1000;	/* default sampling interval is 1 second */

	if ((interval < 100) || (interval > 100000))
	{
	    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
	    	"The sampling interval must be between 100 and 100000 (milliseconds)", 0L);
	    return(FAIL);
	}
	
	/*
	** Lock the sampler block. If the shutdown flag is set, unlock
	** the sampler block, wait a little bit, then try again.
	*/
	for (;;)
	{
	    status = LockSamplerBlk(&hCsSamplerSem);
	    if (status != OK)
		return(status);
	    else
	    {
		if (CsSamplerBlkPtr && CsSamplerBlkPtr->shutdown)
		{
		    i4	sleeptime = CsSamplerBlkPtr->interval;

		    UnlockSamplerBlk(hCsSamplerSem);
		    PCsleep(sleeptime);
		    continue;
		}
		else
		    break;
	    }
	}

	/*
	** If the sampler is already running,
	** 	Update the sampling interval and reporting period,
	** 	Unlock the sampler block,
	** 	Return.
	*/ 
	if (CsSamplerBlkPtr != NULL)
	{
	    CsSamplerBlkPtr->interval = interval;
	    status = UnlockSamplerBlk(hCsSamplerSem);

	    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
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
	CsSamplerBlkPtr->stoptime = time;
	CsSamplerBlkPtr->interval = interval;

	/* Get and save the current Server CPU usage */
	CsSamplerBlkPtr->startcpu = TMcpu();

	/* Set the starting wait/time counts */
	MEcopy( (char*)&Cs_srv_block.cs_wtstatistics,
		sizeof(CsSamplerBlkPtr->waits),
		(char*)&CsSamplerBlkPtr->waits );

	hCsSamplerThread = CreateThread(
					NULL,				/* Security Attribute */
					4096,				/* stack size */
					(LPTHREAD_START_ROUTINE) CS_sampler,
					CsSamplerBlkPtr,		/* Parm */
					CREATE_SUSPENDED,		/* Don't start yet */
					&CsSamplerThreadId);		/* Return the id */
	if (hCsSamplerThread == NULL)
	{
	    status = GetLastError();
	    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		"The sampler thread failed to start. Status = %d",
		status);
	}
	else
	{
	    if (!SetThreadPriority(hCsSamplerThread, THREAD_PRIORITY_TIME_CRITICAL))
	    {
	    	status = GetLastError();
	    }
	} 
	
	/*
	** Unlock the sampler block.
	** Unleash the sampler
	** return
	*/
	status = UnlockSamplerBlk(hCsSamplerSem);

	if (hCsSamplerThread == NULL)	/* if no thread then just return */ 
	    return(FAIL);

	if (!ResumeThread(hCsSamplerThread))	/* Start the Sampler going */
	    status = GetLastError();

	TMstr(&time, timestr);
	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		"Sampling is started. %s",
		timestr);
	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		 "The sampling interval is now %6.2f second(s)",
		 interval/1000.0);

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
**                      (*output_fcn)(newline_present, length, buffer)
**                  where buffer is the length character
**                  output string, and newline_present
**                  indicates whether a newline needs to
**                  be added to the end of the string.
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
** History:
**	24-aug-1999 (somsa01)
**	    Moved the call to MOdetach() to the actual Sampler thread
**	    routine. In this way, the MO semaphores would not be locked by
**	    this current thread (deadlock). Also, the Sampler thread now
**	    also frees up the CsSamplerBlkPtr and the hCsSamplerSem.
**
****************************************************************************/

static STATUS
StopSampler(
	VOID    (*output_fcn) () )

{
	SYSTIME	time;
	char	timestr[50];
	char	buf[100];
	STATUS  status = OK;


	if (CsSamplerBlkPtr == NULL)
	{
	    if (output_fcn != NULL)
	        TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
	   	     "There is no sampling now", 0L);
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
	** Set the stop time and the shutdown flag
	** Unlock the Sampler Block
	** Close the handle for the Sampler Thread and Sampler Semaphore
	** Return.
	*/
	status = LockSamplerBlk(&hCsSamplerSem);
	if (status != OK)
	    return(status);

	if (!output_fcn)
	    TMnow(&CsSamplerBlkPtr->stoptime);

	time = CsSamplerBlkPtr->stoptime;
	CsSamplerBlkPtr->shutdown = TRUE;

	status = UnlockSamplerBlk(hCsSamplerSem);

	CloseHandle(hCsSamplerThread);
	hCsSamplerThread = NULL;

	if (output_fcn != NULL)
	{
	   TMstr(&time, timestr);
	   TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
	     	"Sampling is stopped. %s",
		timestr);
	}

	return (status);

}  /* StopSampler */

/*
**  Name: ShowSampler - Show the Sampler Data
**
**  Description:
**	This is a local routine that presents the Sampler's data in a human
**	readable form, it the Sampler is running.
** 
**  Inputs:
**	output_fcn	Function to call to perform the output.
**			This routine will be called as
**			    (*output_fcn)(newline_present, length, buffer)
**			where buffer is the length character
**			output string, and newline_present
**			indicates whether a newline needs to
**			be added to the end of the string.
**
**	Returns:
**	    OK
**
**	Exceptions:
**	    none
**
**  Side Effects:
**	none
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
** 	07-Jan-1999 (wonst02)
** 	    Add new event types: CS_LIO_MASK (Log I/O), et al.
**	24-May-1999 (shero03)
**	    Add URS and ICE facilities;    
**	15-dec-1999 (somsa01 for jenjo02, part II)
**	    Added sampler display of server CPU time and wait counts.
**	    Helps to see if threads are making progress or are "stuck".
**	10-jan-2000 (somsa01)
**	    Added more statistics printout. Also, CPU time is in
**	    milliseconds, so changed to properly display in seconds.
**	10-oct-2000 (somsa01 for jenjo02)
**	    Sampler improvements: break out event wait percentages
**	    by thread type, show DB id in lock id, track and
**	    display I/O and transaction rates, fix mutex and lock
**	    wait logic. Lump all non-user lock waits into one bucket;
**	    "system" threads should never wait on locks.
**	23-jan-2004 (somsa01 for jenjo02)
**	    Resolve long-standing inconsistencies and inaccuracies with
**	    "cs_num_active" by computing it only when needed by MO
**	    or IIMONITOR. Added MO methods, external functions
**	    CS_num_active(), CS_hwm_active(), callable from IIMONITOR,
**	    e.g. Works for both MT and Ingres-threaded servers.
*/

static STATUS
ShowSampler(
	VOID    (*output_fcn) () )

{
# define MAXITERATIONS (max(max(MAXEVENTS + 1,MAXSAMPTHREADS+1),MAXSTATES))
# define NUMTOP 10
	
	char	timestr[50];
	char	buf[100];
	char	lockname[100];
	f8	perct[MAXITERATIONS];
	f8	topval[NUMTOP];
	i4 	topindx[NUMTOP];
	i4 	restsamples;
	i4 	restvals;
	i4 	samples;
	i4 	seconds;
	i4 	i, j, k, temp;
	f8	perct_mutex, perct_lock;
	STATUS  status = OK;
	i4	cputime;
		

	if (CsSamplerBlkPtr == NULL)
	{
	    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
	   	     "There is no sampling now", 0L);

	    status = UnlockSamplerBlk(hCsSamplerSem);
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
	    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
	   	     "There is no sampling now", 0L);

	    status = UnlockSamplerBlk(hCsSamplerSem);
	    return(FAIL);
	}

	/*
	** Print the start time and sampling interval
	*/
	TMstr(&CsSamplerBlkPtr->starttime, timestr);
	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		"Started at %s, sampling interval %dms.",
		timestr, CsSamplerBlkPtr->interval);

	TMnow(&CsSamplerBlkPtr->stoptime);
	TMstr(&CsSamplerBlkPtr->stoptime, timestr);

	seconds = CsSamplerBlkPtr->stoptime.TM_secs - 
		  CsSamplerBlkPtr->starttime.TM_secs;

	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		"Current Time %s.",
		timestr);

	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		"Total sample intervals %d.",
		CsSamplerBlkPtr->numsamples);

	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		"Total User Thread samples %d.",
		CsSamplerBlkPtr->totusersamples);

	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		"Total System Thread samples %d",
		CsSamplerBlkPtr->totsyssamples);

	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		"Total Active User Threads %d, Active High Water Mark %d",
		CS_num_active(),
		Cs_srv_block.cs_hwm_active);

	cputime = TMcpu();
	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		"Server CPU seconds %d.%d",
		(cputime - CsSamplerBlkPtr->startcpu) / 1000,
		(cputime - CsSamplerBlkPtr->startcpu) % 1000);

	/* 
	** For each thread type tell how much time was spent in each state
	*/

	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		"Thread State Analysis:            ----------- EventWait ------------", 0L);
	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		"Thread Name          Computable   BIO   DIO   LIO   LOG  LOCK  Other  MutexWait  Other", 0L);

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
			    perct[j] = (100.0 *
					CsSamplerBlkPtr->Thread[i].state[j])
					/ (f8) samples;
			    break;
			default:
			    perct[0] += (100.0 *
					CsSamplerBlkPtr->Thread[i].state[j])
					/ (f8) samples;
		    }
		}

		perct_mutex += perct[CS_MUTEX];
		perct_lock  +=
			(100.0 * CsSamplerBlkPtr->Thread[i].evwait[EV_LOCK])
					/ (f8) samples;

		TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
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

	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		"--------------------------------------------------------------------------------------", 0L);

	/*
	** For user/system threads, tell how much computable time was
	** spent in each facility
	*/

	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1, "\
Computing Analysis:\n\
Facility      User  System", 0L);

	for (i = 0; i < MAXFACS; i++)
	{

	    for (j = -1, perct[1] = 0.0; j < MAXSAMPTHREADS; j++)
	    {

		/* Only look at the number of computable samples */
	        samples = CsSamplerBlkPtr->Thread[j].state[1];
		if (samples > 0)
		{
	    	    if (j == 0)
		    {
    	     	        perct[0] = (100.0 *
				    CsSamplerBlkPtr->Thread[j].facility[i]) /
				    (f8) CsSamplerBlkPtr->totusersamples;
		    }
		    else
		    {
		     	perct[1] += (100.0 *
				    CsSamplerBlkPtr->Thread[j].facility[i]) /
   	     	       		    (f8) CsSamplerBlkPtr->totsyssamples;
		    }
		}
	    }

	    if ((perct[0] > 0) | (perct[1] > 0))
	    {

	        TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
			"%8w    %6.1f  %6.1f",
"<None>,CLF,ADF,DMF,OPF,PSF,QEF,QSF,RDF,SCF,ULF,DUF,GCF,RQF,TPF,GWF,SXF,URS,ICE",
			i,
			perct[0], perct[1]);
	    }
	}
	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1, "\
--------------------------", 0L);

	/*
	** For each Mutex Wait, tell how much time was spent waiting on each.
	** Only look at mutexes if mutex wait time is more than 1%.
	** First sort the top 10 mutexes.
	*/	
	
	if (perct_mutex > 1.0)
	{

	    for (k = 0; k < NUMTOP; k++)
		topval[k] = 0.0;
	    restvals = 0;
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

	    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1, "\
Top Mutexes Analysis:\n\
Mutex Name                       ID        User  Fast  Write  Log  Group  Sort  Other", 
	    0L);

	    for (k = 0; k < NUMTOP - 1; k++)
	    {
		if (topval[k] == 0)
		    break;

		i = topindx[k];

		for (j = -1, perct[0] = 0.0; j < MAXSAMPTHREADS; j++)
		{
		    temp = CsSamplerBlkPtr->Mutex[i].count[j];
		    if (temp > 0)
		    {
			perct_mutex -= (100.0 * temp) /
			(f8) CsSamplerBlkPtr->Thread[j].numthreadsamples;
		    }
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
				perct[j+1] = (100.0 * temp) /
				(f8) CsSamplerBlkPtr->Thread[j].numthreadsamples;
			    }
			    else
				perct[j+1] = 0.0;
			    break;
			default:
			    if (temp > 0)
				perct[0] += (100.0 * temp) /
				(f8) CsSamplerBlkPtr->Thread[j].numthreadsamples;
		    }
		}

	        TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
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

	    if (restsamples)
	       TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
			"The other %d mutex(es) accounted for %.1f%%",
			restsamples, perct_mutex); 

	    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		"--------------------------------------------------------------------------------------", 0L);

	}  /* Mutex Waits */
	else
	{
	    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		"Mutex Waits are less than 1%%", 0L);
	    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		"----------------------------", 0L);
	}

	/*
	** For each Lock Wait, tell how much time was spent waiting on each.
	** Only look at locks if lock wait time is > 1%.
	** First sort the top 10 locks.
	*/	

	if (perct_lock > 1.0)
	{
	    
	    for (k = 0; k < NUMTOP; k++)
		topval[k] = 0.0;
	    restvals = 0;
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

	    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		"Top Locks Analysis:", 0L);

	    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		"Resource Type, Name                                                User System", 0L);
	    
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
		    if (temp > 0)
		    {
			perct_lock -= (100.0 * temp) /
			(f8) CsSamplerBlkPtr->Thread[j].numthreadsamples;

			if ( j == CS_NORMAL )
			{
			    perct[0] = (100.0 * temp) /
			(f8) CsSamplerBlkPtr->Thread[j].numthreadsamples;
			}
			else
			{
			    perct[1] += (100.0 * temp) /
			(f8) CsSamplerBlkPtr->Thread[j].numthreadsamples;
			}
		    }
		}

		FormatLockName(&CsSamplerBlkPtr->Lock[i].lk_key, lockname);
		TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
			"%64s  %5.1f  %5.1f",
			lockname, perct[0], perct[1]);
	    }

	    if (restsamples)
	       TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
			"The other %d lock(s) accounted for %.1f%% ",
			restsamples, perct_lock);

	    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		"------------------------------------------------------------------------------", 0L);


	}  /* Lock Waits */
	else
	{
	    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		"Lock Waits are less than 1%%", 0L);
	    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		"---------------------------", 0L);
	}

	/*
	** For all Events tell how much time was spent waiting on each type
	*/
	if (CsSamplerBlkPtr->numsyseventsamples +
	    CsSamplerBlkPtr->numusereventsamples > 0)
	{
	    u_i4 bior_waits = Cs_srv_block.cs_wtstatistics.cs_bior_waits -
				CsSamplerBlkPtr->waits.cs_bior_waits;
	    u_i4 bior_time  = Cs_srv_block.cs_wtstatistics.cs_bior_time -
				CsSamplerBlkPtr->waits.cs_bior_time;
	    u_i4 biow_waits = Cs_srv_block.cs_wtstatistics.cs_biow_waits -
				CsSamplerBlkPtr->waits.cs_biow_waits;
	    u_i4 biow_time  = Cs_srv_block.cs_wtstatistics.cs_biow_time -
				CsSamplerBlkPtr->waits.cs_biow_time;
	    u_i4 dior_waits = Cs_srv_block.cs_wtstatistics.cs_dior_waits -
				CsSamplerBlkPtr->waits.cs_dior_waits;
	    u_i4 dior_time  = Cs_srv_block.cs_wtstatistics.cs_dior_time -
				CsSamplerBlkPtr->waits.cs_dior_time;
	    u_i4 diow_waits = Cs_srv_block.cs_wtstatistics.cs_diow_waits -
				CsSamplerBlkPtr->waits.cs_diow_waits;
	    u_i4 diow_time  = Cs_srv_block.cs_wtstatistics.cs_diow_time -
				CsSamplerBlkPtr->waits.cs_diow_time;
	    u_i4 lior_waits = Cs_srv_block.cs_wtstatistics.cs_lior_waits -
				CsSamplerBlkPtr->waits.cs_lior_waits;
	    u_i4 lior_time  = Cs_srv_block.cs_wtstatistics.cs_lior_time -
				CsSamplerBlkPtr->waits.cs_lior_time;
	    u_i4 liow_waits = Cs_srv_block.cs_wtstatistics.cs_liow_waits -
				CsSamplerBlkPtr->waits.cs_liow_waits;
	    u_i4 liow_time  = Cs_srv_block.cs_wtstatistics.cs_liow_time -
				CsSamplerBlkPtr->waits.cs_liow_time;
	    u_i4 lg_waits  = Cs_srv_block.cs_wtstatistics.cs_lg_waits -
				CsSamplerBlkPtr->waits.cs_lg_waits;
	    u_i4 lg_time   = Cs_srv_block.cs_wtstatistics.cs_lg_time -
				CsSamplerBlkPtr->waits.cs_lg_time;
	    u_i4 lge_waits = Cs_srv_block.cs_wtstatistics.cs_lge_waits -
				CsSamplerBlkPtr->waits.cs_lge_waits;
	    u_i4 lge_time  = Cs_srv_block.cs_wtstatistics.cs_lge_time -
				CsSamplerBlkPtr->waits.cs_lge_time;
	    u_i4 lk_waits  = Cs_srv_block.cs_wtstatistics.cs_lk_waits -
				CsSamplerBlkPtr->waits.cs_lk_waits;
	    u_i4 lk_time   = Cs_srv_block.cs_wtstatistics.cs_lk_time -
				CsSamplerBlkPtr->waits.cs_lk_time;
	    u_i4 lke_waits = Cs_srv_block.cs_wtstatistics.cs_lke_waits -
				CsSamplerBlkPtr->waits.cs_lke_waits;
	    u_i4 lke_time  = Cs_srv_block.cs_wtstatistics.cs_lke_time -
				CsSamplerBlkPtr->waits.cs_lke_time;

	    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		"Event Wait Analysis:", 0L);

	    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
	    "           DIOR   DIOW  LIOR   LIOW   BIOR   BIOW   Log  Lock LGEVT LKEVT", 0L);

	    TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
	    "Time(ms)   %4d   %4d  %4d   %4d   %4d   %4d  %4d  %4d  %4d  %4d",
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
		    perct[j] = (100.0 * CsSamplerBlkPtr->sysevent[j]) /
				(f8) CsSamplerBlkPtr->totsyssamples;
		}

		TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		"  System  %5.1f  %5.1f %5.1f  %5.1f  %5.1f  %5.1f %5.1f %5.1f %5.1f %5.1f",
		perct[0],
		perct[1], perct[2], perct[3], perct[4], perct[5], perct[6],
		perct[7], perct[8], perct[9]);
	    } /* System Events */

	    samples = CsSamplerBlkPtr->numusereventsamples;
	    if (samples > 0)
	    {
		for (j = 0; j < MAXEVENTS + 1; j++)
		{
		    perct[j] = (100.0 * CsSamplerBlkPtr->userevent[j]) /
				(f8) CsSamplerBlkPtr->totusersamples;
		}

		TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
		"    User  %5.1f  %5.1f %5.1f  %5.1f  %5.1f  %5.1f %5.1f %5.1f %5.1f %5.1f",
		perct[0],
		perct[1], perct[2], perct[3], perct[4], perct[5], perct[6],
		perct[7], perct[8], perct[9]);
	    }  /* User Events */

            TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
	    "-------------------------------------------------------------------------", 0L);
	}  /* either System or User Events */

	/* I/O and Transaction rates */

	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
	    "I/O and Transaction Rates/Sec:", 0L);
	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
	    "          DIOR....kb  DIOW....kb  LIOR....kb  LIOW....kb  BIOR  BIOW   Txn", 0L);

	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
	    " Current %5d %5d %5d %5d %5d %5d %5d %5d %5d %5d %5d",
		CsSamplerBlkPtr->dior[CURR], CsSamplerBlkPtr->diork[CURR],
		CsSamplerBlkPtr->diow[CURR], CsSamplerBlkPtr->diowk[CURR],
		CsSamplerBlkPtr->lior[CURR], CsSamplerBlkPtr->liork[CURR],
		CsSamplerBlkPtr->liow[CURR], CsSamplerBlkPtr->liowk[CURR],
		CsSamplerBlkPtr->bior[CURR], CsSamplerBlkPtr->biow[CURR],
		CsSamplerBlkPtr->txn[CURR]);
	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
	    "    Peak %5d %5d %5d %5d %5d %5d %5d %5d %5d %5d %5d",
		CsSamplerBlkPtr->dior[PEAK], CsSamplerBlkPtr->diork[PEAK],
		CsSamplerBlkPtr->diow[PEAK], CsSamplerBlkPtr->diowk[PEAK],
		CsSamplerBlkPtr->lior[PEAK], CsSamplerBlkPtr->liork[PEAK],
		CsSamplerBlkPtr->liow[PEAK], CsSamplerBlkPtr->liowk[PEAK],
		CsSamplerBlkPtr->bior[PEAK], CsSamplerBlkPtr->biow[PEAK],
		CsSamplerBlkPtr->txn[PEAK]);

	TRformat(output_fcn, (i4 *) 1, buf, sizeof(buf)-1,
	    "--------------------------------------------------------------------------", 0L);

	/* 
	** Unlock the Sampler Block
	** return
	*/
	status = UnlockSamplerBlk(hCsSamplerSem);

	return (status);
}  /* ShowSampler */

/****************************************************************************
**
** Name: FormatLockName - Format a lock-key into a more readable form.
**
** Description:
**      Depending on the type of lock build a string
**      with the database name/id, table id & idx, page id
**	A '0' type indicates a non-named lock.                             
**	This code is modeled after LKLSTDET which lists the locks in IPM
**
** Inputs:
**	Lock-key
** Outputs:
**	Update area pointed to by nameptr with a meaningful string
**  Returns:
**      none
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
**  History:
**      18-dec-1996 (shero03)
**	    Original (copied from printlockdet in LKLSTDET.QSC)
**
****************************************************************************/

static void
FormatLockName( LK_LOCK_KEY *lockkeyptr,
		char * nameptr)

{
	char	tempbuf[LK_KEY_LENGTH+1];	
	char	tblname[17];
	char	ownername[9];
	char	type[12];
	char	*dbn;

	do 	/* something to break out of */	
	{
	    /* Construct the type */
	    switch (lockkeyptr->lk_type)
	    {
	    	case 0:
	    	    STcopy("<n/a>", type);
		    break;
		case LK_DATABASE:
		    STcopy("DATABASE", type);
		    break;
		case LK_SV_DATABASE:
		    STcopy("SV_DATABASE", type);
		    break;
		case LK_OWNER_ID:
		    STcopy("OWNER_ID", type);
		    break;
		case LK_JOURNAL:
		    STcopy("JOURNAL", type);
		    break;
		case LK_DB_TEMP_ID:
		    STcopy("DB_TEMP_ID", type);
		    break;
		case LK_CONFIG:
		    STcopy("CONFIG", type);
		    break;
		case LK_OPEN_DB:
		    STcopy("OPEN_DB", type);
		    break;
		case LK_CKP_CLUSTER:
		    STcopy("CKP_CLUSTER", type);
		    break;
		case LK_BM_DATABASE:
		    STcopy("BM_DATABASE", type);
		    break;
		case LK_CONTROL:
		    STcopy("CONTROL", type);
		    break;
		case LK_TABLE:
		    STcopy("TABLE", type);
		    break;
		case LK_BM_TABLE:
		    STcopy("BM_TABLE", type);
		    break;
		case LK_SV_TABLE:
		    STcopy("SV_TABLE", type);
		    break;
		case LK_EXTEND_FILE:
		    STcopy("EXTEND_FILE", type);
		    break;
		case LK_TBL_CONTROL:
		    STcopy("TBL_CONTROL", type);
		    break;
		case LK_PAGE:
		    STcopy("PAGE", type);
		    break;
		case LK_BM_PAGE:
		    STcopy("BM_PAGE", type);
		    break;
		case LK_PH_PAGE:
		    STcopy("PH_PAGE", type);
		    break;
		case LK_SS_EVENT:
		    STcopy("SS_EVENT", type);
		    break;
		case LK_EVCONNECT:
		    STcopy("EVCONNECT", type);
		    break;
		case LK_CKP_TXN:
		    STcopy("CKP_TXN", type);
		    break;
		case LK_AUDIT:
		    STcopy("AUDIT", type);
		    break;
		case LK_ROW:
		    STcopy("ROW", type);
		    break;
		case LK_CREATE_TABLE:
		    STcopy("CREATE_TBL", type);
		    break;
		case LK_CKP_DB:
		    STcopy("CKP_DB", type);
		    break;
	       case LK_BM_LOCK:
		    STcopy("BM_LOCK", type);
		    break;
	       case LK_VAL_LOCK:
		    STcopy("VAL_LOCK", type);
		    break;
	       default:
		    type[0] = EOS;
	    }  /* switch on lock type */
	    
	    switch (lockkeyptr->lk_type)
	    {
	    	case 0:
	    	    STprintf(nameptr,"%s",type);
		    break;
		case LK_DATABASE:
		case LK_SV_DATABASE:
		case LK_OWNER_ID:
		case LK_JOURNAL:
		case LK_DB_TEMP_ID:
		case LK_CONFIG:
		case LK_OPEN_DB:
		case LK_CKP_CLUSTER:
		    STcopy((char*)&lockkeyptr->lk_key1, tempbuf);
		    dbn = tempbuf;
		    if (!CMnmstart(tempbuf))
		    	*tempbuf = EOS;
		    else
		    {	/* set a EOS at the first invalid character */
		    	for (CMnext(dbn); *dbn != EOS; CMnext(dbn))
			{
			    if (!CMnmchar(dbn))
			    {
			    	*dbn = EOS;    
				break;
			    }
			}  /* loop through the name */
		    }
		    STprintf(nameptr, "%s,%s", type, tempbuf);
		    break;

		case LK_BM_DATABASE:
		    STprintf(nameptr, "%s,DB=%d", type, lockkeyptr->lk_key1);
		    break;

		case LK_CONTROL:
		    MEcopy((char*)&lockkeyptr->lk_key1, LK_KEY_LENGTH,
		    	  tempbuf);
		    tempbuf[LK_KEY_LENGTH+1] = EOS;
		    STprintf(nameptr, "%s,SYS CONTROL=%s", type, tempbuf);
		    break;

		case LK_TABLE:
		case LK_BM_TABLE:
		case LK_SV_TABLE:
		case LK_EXTEND_FILE:
		case LK_TBL_CONTROL:
		    STprintf(nameptr, "%s,DB=%x,TABLE=[%d,%d]", type,
				lockkeyptr->lk_key1,
		    		lockkeyptr->lk_key2,
		    	 	lockkeyptr->lk_key3);
		    break;

		case LK_PAGE:
		case LK_BM_PAGE:
		case LK_PH_PAGE:
  		    STprintf(nameptr, "%s,DB=%x,TABLE=[%d,%d],PAGE=%d", type,
			lockkeyptr->lk_key1,
		    	lockkeyptr->lk_key2,
			lockkeyptr->lk_key3, lockkeyptr->lk_key4);
		    break;

		case LK_SS_EVENT:
		    STprintf(nameptr, "%s,SERVER=<%x,%x,%x>", type,
		    	lockkeyptr->lk_key1, lockkeyptr->lk_key2,
			lockkeyptr->lk_key3);
		    break;

		case LK_EVCONNECT:
		    STprintf(nameptr, "%s,SERVER=%x", type,
		    	lockkeyptr->lk_key1);
		    break;

		case LK_CKP_TXN:
		    if ((lockkeyptr->lk_key1 == 0) ||
		    	(lockkeyptr->lk_key3 == 0))
		    {
		        tblname[0] = EOS;
			ownername[0] = EOS;
		    }
		    else
		    {
		        MEcopy((char*)&lockkeyptr->lk_key1, 8,
		    	 	ownername);
		        MEcopy((char*)&lockkeyptr->lk_key3, 16,	
		    	      	tblname);
			ownername[8] = EOS;
			tblname[16] = EOS;
		    }
		    STprintf(tempbuf, "%s-%s", tblname, ownername);
		    STzapblank(tempbuf, tempbuf);
		    STprintf(nameptr, "%s,NAME='%s'", type, tempbuf);
		    
		    break;

		case LK_AUDIT:
		    switch (lockkeyptr->lk_key1)
		    {
		    	case SXAS_LOCK:
		    	    if (lockkeyptr->lk_key2 == SXAS_STATE_LOCK)
			    	STcopy("AUDIT,State(primary)", nameptr);
			    else if (lockkeyptr->lk_key2 == SXAS_SAVE_LOCK)
			    	STcopy("AUDIT,State(save)", nameptr);
			    else 
			    	STprintf(nameptr, "AUDIT,State(oper=%d)",
					lockkeyptr->lk_key2);
			    break;
			case SXAP_LOCK:	
			    if (lockkeyptr->lk_key2 == SXAP_SHMCTL)
			    	STprintf(nameptr,
					"AUDIT,Physical_layer(shmctl,node=<%x>)",
					lockkeyptr->lk_key3);
			    else if (lockkeyptr->lk_key2 == SXAP_ACCESS)
			    	STcopy("AUDIT,Physical_layer(access)", nameptr);
			    else if (lockkeyptr->lk_key2 == SXAP_QUEUE)
			    	STprintf(nameptr,
					"AUDIT,Physical_layer(queue,node=<%x>)",
					lockkeyptr->lk_key3);
			    else 
			    	STprintf(nameptr, "AUDIT,Physical_layer(oper=%d)",
					lockkeyptr->lk_key2);
			    break;
	 
			case SXLC_LOCK:
			    STprintf(nameptr, "AUDIT,Label_cache");
			    break;

			default:
			    STprintf(nameptr, "AUDIT,TYPE=%d,OPER=%d",
			    	lockkeyptr->lk_key1,
				lockkeyptr->lk_key2);
		    }
		    break;

		case LK_ROW:
  		    STprintf(nameptr, "%s,DB=%x,TABLE=[%d,%d],PAGE=%d,ROW=%d", type,
			lockkeyptr->lk_key1,
		    	lockkeyptr->lk_key2,
			lockkeyptr->lk_key3, lockkeyptr->lk_key4,
		    	lockkeyptr->lk_key5);
		    break;

		case LK_CREATE_TABLE:
		case LK_CKP_DB:
		    if ((lockkeyptr->lk_key4 == 0) ||
		    	(lockkeyptr->lk_key2 == 0))
		    {
		        tblname[0] = EOS;
			ownername[0] = EOS;
		    }
		    else
		    {
		        MEcopy((char*)&lockkeyptr->lk_key2, 8,
		    	      	ownername);
		        MEcopy((char*)&lockkeyptr->lk_key4, 16,
		    	  	tblname);
			ownername[8] = EOS;
			tblname[16] = EOS;
		    }
		    STprintf(tempbuf, "%s-%s", tblname, ownername);
		    STzapblank(tempbuf, tempbuf);
		    STprintf(nameptr, "%s,DB=%x,NAME='%s'", type,
		    	lockkeyptr->lk_key1, tempbuf);
    		    break;

	       case LK_BM_LOCK:
		    MEcopy((char*)&lockkeyptr->lk_key1, 8,
		    	      	tempbuf);
		    tempbuf[8] = EOS;
		    STprintf(nameptr, "%s,BUFMGR='%s'", type, tempbuf);
		    break;

	       case LK_VAL_LOCK:
  		    STprintf(nameptr, "%s,DB=%x,TABLE=[%d,%d],VALUE=<%d,%d,%d>", 
		    	type,
			lockkeyptr->lk_key1,
			lockkeyptr->lk_key2, lockkeyptr->lk_key3, 
			lockkeyptr->lk_key4, lockkeyptr->lk_key5, 
			lockkeyptr->lk_key6);
		    break;

	       default:
		    STprintf(nameptr, "%d,<%x,%x,%x,%x,%x,%x>",
		    	lockkeyptr->lk_type, lockkeyptr->lk_key1,
		    	lockkeyptr->lk_key2, lockkeyptr->lk_key3,
		    	lockkeyptr->lk_key4, lockkeyptr->lk_key5,
			lockkeyptr->lk_key6);
	    }  /* switch on lock type */
	
	} while (0);	/* do only once	*/
} /* FormatLockName */

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
** History:
**	25-aug-1999 (somsa01)
**	    If CSsamp_stopping is set, then wait for the Sampler thread to
**	    initialize *hpLockSem back to NULL.
**
****************************************************************************/

STATUS
LockSamplerBlk( HANDLE *hpLockSem )

{
	STATUS  status = OK;
	DWORD	dwWaitResult;
	
	/*
	** If the semaphore is unitialize, then create a new one with us
	** owning it. If a Sampler thread is currently shutting down,
	** wait for it to finish.
	*/	
	if (CSsamp_stopping)
	{
	    while (*hpLockSem != NULL);
	    CSsamp_stopping = FALSE;
	}

	if (*hpLockSem == NULL)
	{	
	    *hpLockSem = CreateSemaphore( NULL, 0, 1, NULL);
	    if (*hpLockSem == NULL)
	        status = GetLastError();

 	    return(status);
	}

	/*
	** Wait for the semaphore, if some thread has it
	*/
	dwWaitResult = WaitForSingleObject( *hpLockSem, INFINITE);
	if (dwWaitResult == WAIT_OBJECT_0)
	    return(OK);
	else
	    status = dwWaitResult;

	return (status);

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
UnlockSamplerBlk( HANDLE hLockSem )

{
	STATUS  status = OK;
		
	if (! ReleaseSemaphore( hLockSem, 1, NULL) )
	    status = GetLastError();
		     
	return (status);

} /* UnlockSamplerBlk */
