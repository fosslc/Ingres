/*
** Copyright (c) 1982, 2008 Ingres Corporation
*/
 
# include	<compat.h>
# include       <gl.h>
# include	<descrip>
# include	<efndef.h>
# include	<iledef.h>
# include	<iodef.h>
# include	<iosbdef.h>
# include	<exhdef.h>
# include	<jpidef.h>
# include	<ssdef>
# include	<lib$routines.h>
# include	<starlet.h>
# include	<vmstypes.h>
# include	<cm.h>
# include	<ex.h>
# include	<nm.h>
# include	<lo.h>
# include	<me.h>
# include	<st.h>
# include	<pc.h>
# include	<si.h>
# include	<er.h>
# include	"pclocal.h"
# include       <astjacket.h>

/**
** Name:	pccmdline.c -	Compatibility Library Process Control
**					Execute Command-Line Module.
** Description:
**	Contains the routines used to execute command-lines defined as part
**	of the Process Control (PC) module of the CL.  Defines:
**
**	PCcmdline()	execute a command-line.
**
**	CL local:
**
**	PCcmdnointr()	execute a command-line without interrupt handling.
**	PCcmdintr()	execute a command-line with interrupt handling.
**	PCcmdnoout()	execute a command-line with no output.
**	PCcmdut()	execute a UT executable command.
**	PCcmdcloseout() close output log file
**
** History:		
**	NOTE JUMBLED CHRONOLOGICAL ORDER OF HISTORY:
**	17-dec-92 (sandyd)
**		Removed references to deceased functions - INtoignore() and 
**		IN_kill_me().  The old 5.0 stuff has now been cleaned out of
**		the CL.
**	4-feb-92 (mgw) (42114)
**		Add capability to turn off no-output property of PCcmdnoout()
**		if II_UT_TRACE is turned on, and direct the output from
**		that call to the trace file.
**	6/91 (Mike S)
**		Release ^C AST for PCcmdline entry point.
**		Don't re-establish AST if it wasn't established to begin with.
**		Put PCsubproc flags back into this file.
**	06-mar-1991 (rogerk)
**		Changed size of 'ttname', which is a static defined buffer
**		to hold the terminal name, from 16 to MAX_LOC.  When the
**		terminal was really defined to be a command file (for instance
**		if the process was a batch job), and the command file name
**		exceeded 16 characters, we would write over the stack.
**	04-mar-1991 (sylviap) 
**		Changed PCsubproc so it is no longer a static.  UTprint now 
**		calls PCsubproc.  Moved the PCsubproc "modes" out to pc.h --
**		now accessible to UTprint.
**
**	Revision 6.4  90/03/14  sylviap
**	Passing a CL_ERR_DESC into PCcmdline().  Changing interface of
**	PCcmdintr() to be consistent with PCcmdline().
**	90/02/13  Added parameters to PCcmdline().
**
**	Revision 6.3  90/03  wong
**	Moved 'STlength()' call into 'PCfewrite()' and 'PCiwrite()' and removed
**	numerous unnecessary 'STcopy()' calls, instead passing strings directly
**	into 'PCiwrite()'.
**
**	17-apr-1985 (fhc) -- added communication with IN
**		for process timeout handling
**
**	03-jul-86 (mgw) - Bug # 9704
**		Send NOINTR flag to PCcmdut() so the frontend
**		called can handle it's own interrupts.
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	4-sept-95 (Nick Fletcher)
**	    fixed a double byte bug whereby the CLI was coverting the 
**	    parameters passed to front end programs to lowercase and failing
**	    to convert double-byte characters correctly.
**      02-oct-1996 (rosga02)
**          Bug 74980: add processing for VMS 6.0 captive accounts,
**          to allow spawning to be /TRUSTED if II_SPAWN_TRUSTED
**          is set.
**	1-jul-1997(angusm)
**	    Add getprocdets() to check MODE, TERMINAL settings for
**	    this process: need to handle reset of SYS$INPUT etc
**	    differently for NETWORK processes.
**      01-jul-1998 (kinte01)
**          Changed PC_NOWAIT to PC_NO_WAIT which is the way it is defined
**	    in the Unix CL
**      09-nov-1999 (kinte01)
**         Rename EX_CONTINUE to EX_CONTINUES per AIX 4.3 porting change
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes and
**	   external function references
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	29-aug-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**	27-jul-2004 (abbjo03)
**	    Rename pc.h to pclocal.h to avoid conflict with CL hdr!hdr file.
**	18-jun-2008 (joea)
**	    Fix PCsubproc so it doesn't attempt to write to a potential
**	    cmd string literal.  Change PChandler to deal with both user
**	    interrupts and actual exceptions.
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
**	29-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
**	11-nov-2008 (joea)
**	    Use CL_CLEAR_ERR to initialize CL_ERR_DESC.
**      18-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**      22-dec-2008 (stegr01)
**          Itanium VMS port
**      06-Dec-2010 (horda03) SIR 124685
**          Fix VMS build problems, 
**/

/* Note:  DCLCOMLINE is the size of the mailbox buffer used to communicate with
** the DCL subprocess and with other INGRES front-ends.  Its value is hard-coded
** in "cl_vms!clf!misc!frontmain.mar" and cannot be changed to remain backwards
** compatabile with ABF applications, which link "frontmain.obj" directly.
*/
# define	DCLMAX		1024
# define	DCLCOMLINE	128

/*
** All 5 entry points call PCsubproc with an argument that tells
** what to do with interrupts and with I/O.
*/
# define        NOINTR          01      /* We won't interrupt the subprocess */
# define        NOINPUT         010     /* Disable terminal input */
# define        NOOUTPUT        020     /* Disable standard output and error */
# define        LONGCMD         0100    /* Send command input via mailbox */
# define        NOCTRLCTRAP     0200
                                /* Disable ^C trap while suproc is active */

static char			statname[33];
static char			mbxname[33];
static char			pname[33];
static char			ttname[MAX_LOC+1];
static struct dsc$descriptor_s	pdesc;
static struct dsc$descriptor_s	mbx;
static struct dsc$descriptor_s	status;
static II_VMS_CHANNEL		mbxchan = 0;
static II_VMS_CHANNEL		statchan;
static PID			pid;
static II_VMS_EF_NUMBER		peflag;
static int			deltim[2];	/* Used as a QUAD Word */
static int			timerid;
static bool			incom = FALSE;
static bool			PCcatch;
static bool			log_open = FALSE;
static $EXHDEF			pcexitblk;

static const
	char	_TurnOffMsg[]	= 	"$ set message/nofac/noid/nosev/notext",
		_TurnOnMsg[]	= 	"$ set message/fac/id/sev/text",
		_DeassignOutput[] =	"$ deassign SYS$OUTPUT",
		_DeassignError[] =	"$ deassign SYS$ERROR",
		_DisableInput[]	= 	"$ deassign SYS$INPUT",
		_DisableCmd[]	= 	"$ deassign SYS$COMMAND",
		_DisableOutput[]= 	"$ define/nolog SYS$OUTPUT NLA0:",
		_DisableError[]	= 	"$ define/nolog SYS$ERROR NLA0:",
		_RedirectOutput[]= 	"$ define/nolog SYS$OUTPUT %s",
		_ErrorToOutput[] = 	"$ define/nolog SYS$ERROR SYS$OUTPUT";

static const
	char	*modes[] = { "OTHER"  ,
			     "NETWORK" ,
			     "BATCH",
			     "INTERACTIVE" };

STATUS	PCsubproc();
STATUS	PCcmdcloseout();

static STATUS	PCbatch();
static STATUS	PCiwrite(II_VMS_CHANNEL mbx, char *buf);
static int	PCitimer();
static int	write();
static int	read(II_VMS_CHANNEL chan, char *buf, int *size);
static int	trnlog();
static int	mailbox();
static int	PCexith();
static STATUS	getprocdets();	
static STATUS	PCfewrite();
static EX	PChandler(EX_ARGS *exc_args);


FUNC_EXTERN STATUS CVahxl();
FUNC_EXTERN void IInotrap_ctrlc();
FUNC_EXTERN void IItrap_ctrlc();
FUNC_EXTERN bool IIctrlc_is_trapped();
FUNC_EXTERN void UTopentrace();
FUNC_EXTERN void UTlogtrace();

/*
** Invoke a system command.  The command is sent through a mailbox
** to a DCL that has been spawned, and is lying around.  If the DCL process
** isn't needed for 15 minutes, it is killed, and we'll start up another the
** next time it's needed.
**
** The exit handler PCexith insures that the subprocess is stopped (i.e.
** killed) when the front-end program exits.
**
** There are five entry points for some VMS reasons.
**
** PCcmdline is used to run commands that catch ^C and handle
** it properly.  When this type of program is run, nothing is
** done with ^C.
**
** PCcmdnointr is an internal CL entry point very similar to PCcmdline.
**
** PCcmdintr is used to run commands that probably don't catch
** ^C.  These are things like systems commands such as the linker
** and compilers.  In this case, PCsubproc catches ^C and causes
** the program being run in the subprocess to exit.
**
** PCcmdnoout is used to run commands which don't want output
** displayed to the user.  Things like LOpurge use this entry point.
**
** PCcmdut is used to run commands passed in from UTexe.  These
** commands may be longer than DCL's 256 character limit.
**
** PCcmdline is the only user level entry point.  The other three are
** VMS CL internal routines.
**
*/

/*
** History:
**	13-feb-90 (sylviap)  Added parameters to PCcmdline.
**	14-mar-90 (sylviap)  Passing a CL_ERR_DESC into PCcmdline().
**	11-aug-93 (ed) changed to match prototype
*/
STATUS
PCcmdline( 
LOCATION	*shell,	/* unused */
char		*cmd,
i4		wait,
LOCATION	*log,
CL_ERR_DESC	*error)
{
	STATUS status;

	CL_CLEAR_ERR(error);
	if ( wait == PC_NO_WAIT )
		return PCbatch(shell, cmd, log, error);
	else
	{
		/* Don't interrupt the subprocess; let it handle ^C */
		status = PCsubproc(shell, cmd, NOINTR|NOCTRLCTRAP, log, 
				   FALSE, error);
		incom = FALSE;
		return status;
	}
}

/*
** History:
**	14-mar-90 (sylviap)  Changing interface to be consistent with PCcmdline.
**	21-mar-1990 (jhw)  VMS utilities (compilers, etc.) output identically
**		to SYS$ERROR and SYS$OUTPUT so only enable SYS$ERROR.
*/
STATUS
PCcmdintr(
char		*shell,	/* unused */
char		*cmd,
i4		wait,
LOCATION	*log,
bool		append,
CL_ERR_DESC	*error)
{
	STATUS status;

	CL_CLEAR_ERR(error);
	/* We'll kill the subprocess if we detect ^C */
	status = PCsubproc(shell, cmd, 0, log, append, error);
	incom = FALSE;
	return status;
}

/*
** History:
**	10-apr-1990 (Mike S) Created
*/
STATUS
PCcmdnointr(
char		*shell,	/* unused */
char		*cmd,
i4		wait,
LOCATION	*log,
bool		append,
CL_ERR_DESC	*error)
{
	STATUS status;

	CL_CLEAR_ERR(error);
	/* We'll let the subprocess detect ^C */
	status = PCsubproc(shell, cmd, NOINTR|NOCTRLCTRAP, log, append, error);
	incom = FALSE;
	return status;
}

STATUS
PCcmdnoout(
char	*shell,	/* unused */
char	*cmd)
{
	STATUS status;
	CL_ERR_DESC	err_code;
	FILE		*tf = NULL;
	LOCATION	tmploc;
	LOCATION	trcloc;
	char		trcbuf[MAX_LOC+1];

	UTopentrace(&tf);	/* open trace file if II_UT_TRACE is set */

	if (tf)	 /* If II_UT_TRACE is on, turn on output to the trace file */
	{
		SIfprintf(tf, "%s\n", cmd);	/* log the command */
		(void)SIclose(tf);

		/* create a temp output file */
		(void)NMloc(TEMP, PATH, (char *)NULL, &tmploc);
		(void)LOuniq("pc", "out", &tmploc);
		(void)LOcopy(&tmploc, trcbuf, &trcloc);

		/* don't disable input or output to the subprocess */
		status = PCsubproc(shell, cmd, 0, &trcloc, TRUE, &err_code);

		/* copy error output to II_UT_TRACE and clean up */
		PCcmdcloseout(FALSE);
		UTlogtrace(&trcloc, status, &err_code);
		(void)LOdelete(&trcloc);
	}
	else
	{
		/* Disable all input and output to the subprocess */
		status = PCsubproc(shell, cmd, (NOINPUT|NOOUTPUT),
				(LOCATION *)NULL, FALSE, &err_code);
	}
	incom = FALSE;
	return status;
}

STATUS
PCcmdut(
char		*cmd,
i4		wait,
LOCATION	*log,
CL_ERR_DESC	*error)
{
	STATUS status;

	CL_CLEAR_ERR(error);
	if ( wait == PC_NO_WAIT )
		return PCbatch((LOCATION *)NULL, cmd, log, error);
	else
	{
		int	flags2 = LONGCMD | NOINTR | NOCTRLCTRAP;
                char	mode[MAX_LOC];
		char	us[MAX_LOC];

		MEfill(sizeof(mode), '\0', mode);
		MEfill(sizeof(us), '\0', us);
		status = getprocdets(mode, us);
		if (status != OK)
			return(status);
		if (STcompare(mode, ERx("NETWORK")) == 0)
			flags2 |= NOINPUT;
		/* Send command via mailbox; let the subprocess handle ^C */
		status = PCsubproc( (LOCATION *)NULL, cmd, 
				     flags2,
				     log, FALSE, error
		);
		incom = FALSE;
        	return status;
	}
}

/*
** Name:	PCbatch() -	Execute Command-line in Batch.
**
** Description:
**	Executes a command-line as a detached sub-process (i.e., in batch.)  It
**	does so by writing the command-line to a temporary command file (which
**	deletes itself after execution) and then submiting the command file as
**	a batch job.
**
** Inputs:
**	shell	{char *}  The command interpreter (unused.)
**	cmd	{char *}  The command-line.	
**	log	{LOCATION *}  The output log location.
**
** Output:
**	error	{CL_ERR_DESC *}  The CL error description.
**
** Returns:
**	{STATUS}
**
** History:
**	02/90 (sylviap) -- Originally written in "ut!utexe.c".
**	03/90 (jhw) -- Moved here as 'PCbatch()'.
**      22-Sept-2003 (ashco01)
**            Problem: INGSRV#2092, Bug: 109579
**            PCbatch() is sometimes called with a NULL logfile
**            name, this causes LOfroms() to subsequently fail with 
**            E_CL1102_LO_FR_BAD_SYN. Added check for logfile name
**            and now compose dcl 'submit' statement accordingly.
**            
*/
static STATUS
PCbatch(
char		*shell,	/* unused */
char		*cmd,
LOCATION	*log,
CL_ERR_DESC	*error)
{
	STATUS		stat;
	FILE            *tfile;
	char            *cmdfile;
	char            *logfile;
	char            buf[MAX_LOC + 1];
	char		submit[MAX_LOC * 2 + 21 + 1];
	LOCATION        tloc;

	CL_CLEAR_ERR(error);
	/* Create a command file */
	if ( (stat = NMloc(TEMP, PATH, NULL, &tloc)) != OK
		|| (stat = LOuniq ("pc","tmp", &tloc)) != OK
		|| (LOtos(&tloc, &cmdfile),
			(stat = SIopen(&tloc, "w",&tfile)) != OK) )
	{
		return error->error = stat;
	}
	SIfprintf(tfile,"$ %s\n", cmd);
	SIfprintf(tfile,"$ delete %s;*\n", cmdfile);
	SIclose(tfile);

	/* Let subprocess handle ^C */
        /* logfile location name may be NULL, omit in this case */
        if (log == NULL ) 
		stat = PCsubproc( shell,
		    STprintf(submit, "submit/nolog %s",cmdfile),
		    NOINTR, (LOCATION *)NULL, FALSE, error);
	else
        {
		LOtos(log, &logfile);
		stat = PCsubproc( shell,
		    STprintf(submit, "submit/log=%s/noprinter %s",
			      logfile, cmdfile),
		    NOINTR, (LOCATION *)NULL, FALSE, error);
        }
	incom = FALSE;
	return stat;
}

/*
** Name:	PCsubproc() -	Execute Command-Line using Sub-process.
**
** Description:
**	Executes a command-line by sending it to a command execution
**	sub-process.  This handles interrupts and I/O redirection as
**	appropriate.
**
** History:
**	03/90 (jhw) -- Added support for output redirection.
*/

/*
** Sub-process set-up commands.
**
**	These commands set-up the DCL session for the command execution
**	sub-process.  In addition, the terminal logical (TT) is defined
**	and the command status channel opened using dynamic statements in
**	the set-up code in 'PCsubproc()'.
**
**	Note:  Sub-process default has SYS$OUTPUT and SYS$ERROR set to the
**	null device (NLA0:) so spurious errors are not displayed to the user.
*/

static char	*_setup[] = {
			"$ set :=",
			"$ set noon",
			/*
			** BUG 5732 -- Undefine symbols used here so users
			** definitions don't get in the way.
			*/
			"$ deassign :=",
			"$ define :=",
			/*
			** BUG 3605 -- Several fixes to solve problems with more
			** than one subprocess stacked up.  The most important
			** is not to use DEFINE/USER when using spawn.  Any
			** logical names that need to be defined by PCsubproc()
			** are first deassigned (in the context of a "set
			** message/no ..." so that error messages are not
			** printed) before being defined so that spurious error
			** messages about redefinition (which can't be hidden)
			** are not produced. 
			*/
			/* $ set message/nofac/noid/nosev/notext */ _TurnOffMsg,
			/* $ deassign SYS$OUTPUT */ _DeassignOutput,
			/* $ deassign SYS$ERROR */ _DeassignError,
			/* $ deassign SYS$INPUT */ _DisableInput,
			"$ deassign TT",
			/*
			** BUG 5743 -- Do the same to sys$command.
			** This allows SET HOST to work.
			*/
			/* $ deassign SYS$COMMAND */ _DisableCmd,
			/* $ set message/fac/id/sev/text */ _TurnOnMsg,
			/*
			** BUG 4970 -- Spell out the word DEFINE in case
			** the user has a symbol defined for DEF.
			*/
			/* $ define SYS$OUTPUT NLA0: */ _DisableOutput,
			/* $ define SYS$ERROR NLA0: */ _DisableError,
			/* Dynamic set-up for TT and command status channel
			** shown here for completeness; see below ...
			"$ open/write rbox <statname>"
			"$ define TT <ttname>"
			*/
			NULL
};

STATUS
PCsubproc(
char		*shell,	/* unused */
char		*cmd,	/* command to execute */
int		flags,	/* Interrupt I/O redirection flags */
LOCATION	*log,	/* output log file (NULL for terminal) */
bool		append, /* Append to currently open file? */
CL_ERR_DESC	*error)	/* Error return */
{	
	int		rval;
	int		rstat;
	int		size;
	char		*logfile;
	int		flag = 1;
	char		buf[DCLMAX];
	EX_CONTEXT	excontext;
	bool		reset_ctrlc;

	char		lc_cmd[DCLMAX];
	char		*cmd_ptr;
	char		*cptr;
	char		*lc_cptr = lc_cmd;
	char		quoted_flag = FALSE;
	char		in_param = FALSE;
	char		is_param_specifier = FALSE;
	char		prev_was_white = FALSE;
	char            *cp;	
        EX              ex;

	CL_CLEAR_ERR(error);
	if (log != 0)
		LOtos(log, &logfile);
	/*
	** Note that interrupts are disabled while the process is starting up.
	** Once a command is running interrupts are caught or not depending
	** on the value in 'flags'.  If they are not caught, they are delivered
	** to the subprocess, which acts on them as appropriate.  If the sub-
	** process exits, then the input call below returns, and interrupts are
	** enabled before returning.  If they are caught, then the command is
	** exited with a force exit.
	*/
	EXinterrupt(EX_OFF);

	/*
	** Note:  If the command-line is NULL or empty,
	** then this is a call to an interactive shell.
	*/
	if ( cmd == NULL || *cmd == EOS )
	{ /* call interactive DCL session */
		/* Terminate any pre-existing command execution sub-process and
		** then redefine all the input and output to be to the terminal.
		** Then execute a sub-process with an interactive DCL session.
		** Afterwards, the sub-process will be logged out by the user.
		*/
		if ( mbxchan != 0 )
		{
			PCcleanup(ex);
		}
		flag = 0;

		/*
		** bug 74980 - allow spawns to be considered 'trusted'
		*/
		NMgtAt("II_SPAWN_TRUSTED", &cp);
		if (cp != NULL && *cp != '\0')
			flag |= CLI$M_TRUSTED;

		if (EXdeclare(PChandler, &excontext))
		{
			rval = FAIL;
		}
		else
		{
			EXinterrupt(EX_ON);
			IInotrap_ctrlc();	/* Release our ^C AST */
			rval = lib$spawn(0, 0, 0, &flag);
			IItrap_ctrlc();	/* Resume our ^C AST */
			rval = (rval & 01 ? OK : rval);
		}
		EXdelete();
		return rval;
	}

	/* Command lines are executed by a semi-permanent DCL sub-process and
	** communication carried out between this process and its child
	** through two mail boxes, one to write commands to the sub-process
	** and one to read command status back.  Note that most errors detected
	** here will terminate the sub-process.
	*/

	PCcatch = !(flags & NOINTR);
	incom = TRUE;

	if ( mbxchan == 0 )
	{ /* open mail boxes and start sub-process */
		struct dsc$descriptor_s	desc;
		struct dsc$descriptor_s	tt;

		desc.dsc$b_class = DSC$K_CLASS_S;
		desc.dsc$b_dtype = DSC$K_DTYPE_T;
		if ( statchan == 0 )
		{ /* initialize descriptors */
			PID	pid;
			char	*cp;

			PCpid(&pid);
			/* command mailbox */
			(void)STprintf(mbxname, "II_PC_MBX_%x", pid);
			/* command status mailbox */
			(void)STprintf(statname, "II_PC_STATUS_%x", pid);
			/* process name */
			(void)STprintf(pname, "II_PC_%x", pid);
			if (!((rval = lib$get_ef(&peflag)) & 01))
				goto errreturn;
			if (!((rval = sys$gettim(deltim)) & 01))
				goto errreturn;
			timerid = deltim[0];
			NMgtAt("II_PC_TIMEOUT", &cp);
			if (cp != NULL && *cp != '\0')
				desc.dsc$a_pointer = cp;
			else
				desc.dsc$a_pointer = "0000 00:15:00.00";
			desc.dsc$w_length = STlength(desc.dsc$a_pointer);
			if (!((rval = sys$bintim(&desc, deltim)) & 01))
				goto errreturn;
			/* Command status mailbox descriptor */
			status.dsc$b_class = DSC$K_CLASS_S;
			status.dsc$b_dtype = DSC$K_DTYPE_T;
			status.dsc$w_length = STlength(statname);
			status.dsc$a_pointer = statname;
			/* Command mailbox descriptor */
			mbx.dsc$b_class = DSC$K_CLASS_S;
			mbx.dsc$b_dtype = DSC$K_DTYPE_T;
			mbx.dsc$w_length = STlength(mbxname);
			mbx.dsc$a_pointer = mbxname;
			/* Process name descriptor */
			pdesc.dsc$b_class = DSC$K_CLASS_S;
			pdesc.dsc$b_dtype = DSC$K_DTYPE_T;
			pdesc.dsc$w_length = STlength(pname);
			pdesc.dsc$a_pointer = pname;
			/* Terminate any sub-process */
			sys$delprc(0, &pdesc);
			pcexitblk.exh$gl_link = 0;
			pcexitblk.exh$g_func = PCexith;
			pcexitblk.exh$l_argcount = 1;
			pcexitblk.exh$gl_value = &(pcexitblk.exh$l_status);
			sys$dclexh(&pcexitblk);

			/* Get the terminal name for this process */
			ttname[0] = 'T'; ttname[1] = 'T', ttname[2] = '\0';
			tt.dsc$b_class = DSC$K_CLASS_S;
			tt.dsc$b_dtype = DSC$K_DTYPE_T;
			tt.dsc$w_length = 2;
			tt.dsc$a_pointer = ttname;
			trnlog(&tt);
			for ( cp = ttname ; *cp != '\0' && *cp == '_' ; ++cp )
				--tt.dsc$w_length;
			if (cp != ttname)
				STcopy(cp, ttname);
		}
		/* Open command mailbox */
		if ((rval = mailbox(&mbx, &mbxchan)))
			goto errreturn;
		/* Open command status mailbox */
		if ((rval = mailbox(&status, &statchan)))
			goto errreturn;
		if (!((rval = sys$setimr(EFN$C_ENF, deltim, PCitimer, timerid, 0))
				& 01))
			goto errreturn;
		/*
		** BUG 3695
		** If called from a program being run in a batch job, then the
		** output must be a terminal PPF.  TT is always guaranteed to be
		** a terminal PPF.  In a batch job it points to NLA0:
		*/
		/*
		** bug 74980 - allow spawns to be considered 'trusted'
		*/
		NMgtAt("II_SPAWN_TRUSTED", &cp);
		if (cp != NULL && *cp != '\0')
			flag |= CLI$M_TRUSTED;

		if ((rval = lib$spawn(0, &mbx, 0, &flag, &pdesc, &pid, 0, &peflag)) & 01)
		{ /* initialize DCL sub-process */
			char	**cpp;

			/* set-up DCL sub-process session */

			for ( cpp = _setup ; *cpp != NULL ; ++cpp )
				(void)PCiwrite(mbxchan, *cpp);

			/* Open command status channel */
			(void)PCiwrite( mbxchan,
				STprintf(buf, "$ open/write rbox %s", statname)
			);
			/* Define terminal logical */
			(void)PCiwrite( mbxchan,
				STprintf(buf, "$ define TT %s", ttname)
			);
		}
		else
		{
			goto errreturn;
		}
	} /* end start-up code */

	/*
	** Set-up input, output, and error output for the sub-process.
	**
	** By our convention, all input and output channels for the command
	** execution sub-process are disabled.  For input channels, this is
	** effected by deassigning them, and for the output channels, by
	** assigning them to the null device.  To re-enable, the former are
	** assigned to the terminal, and the latter are deassigned, which
	** automatically re-enables output to the terminal.
	*/
	if ((flags & NOOUTPUT) != 0)
	{
	  	/* 
		** Disable output.  It's already disabled, unless we have an
		** open log file.
		*/
		if (log_open)
			PCcmdcloseout(FALSE);
	}
	else
	{ /* enable output ... */
		if ( log != NULL )
		{ /* redirect both output and error to log file ... */

			/* 
			** If we have an open log, and we're appending, leave
			** things as they are.
			*/
			if (!(append && log_open))
			{
			
				/* Redirect SYS$OUTPUT to the log file, and 
				** SYS$ERROR to SYS$OUTPUT.  This will merge 
				** both streams into the log file.	
				*/
				(void)PCiwrite(mbxchan, _DeassignOutput);
				(void)PCiwrite(mbxchan, _DeassignError);
				(void)PCiwrite( mbxchan,
					 STprintf(buf, _RedirectOutput, logfile)
				);
				(void)PCiwrite( mbxchan, _ErrorToOutput);
				log_open = TRUE;
			}
		}
		else
		{ /* let both error and output go to the terminal ... */

			/* This disables the output to the null device,
			** and by default re-enables output to the
			** terminal:
			**	$ deassign SYS$OUTPUT
			**	$ deassign SYS$ERROR
			*/
			(void)PCiwrite(mbxchan, _DeassignOutput);
			(void)PCiwrite(mbxchan, _DeassignError);
		}
	}

	if (!(flags & NOINPUT))
	{ /* enable input from terminal */
		/* Assign input channels to the terminal */
		(void)PCiwrite( mbxchan,
				STprintf(buf, "$ define SYS$INPUT %s", ttname)
		);
		(void)PCiwrite( mbxchan,
				STprintf(buf, "$ define SYS$COMMAND %s", ttname)
		);
	}

	/* Execute the command-line */

	buf[0] = '$';
	buf[1] = ' ';
	if ( EXdeclare(PChandler, &excontext) != OK )
	{
		EXinterrupt(EX_OFF);
	}

        /*
	** Double Byte Bug Fix:
	** When the argument is passed into the INGRES front end 
	** program, the command line interpreter converts the 
	** arguements into lowercase. We do want a lowercase conversion
	** to take place, but we want a proper double byte conversion
	** since the CLI's conversion will mess up double byte chars.
	** Therefore do double-byte lower case conversion on all chars except
	** the parameter type specifiers and then enclose
	** each of the parameters in quotes so that the CLI will not do
	** lower case conversion.
	*/
#ifdef VMS

#ifdef DOUBLEBYTE
	for ( cptr = cmd ; *cptr != EOS ; CMnext(cptr), CMnext(lc_cptr) )
	{
		bool do_lower = FALSE;

		if( !is_param_specifier )
		{
			if( !quoted_flag )
			    do_lower = TRUE;
		}
		else
		{
			is_param_specifier = FALSE;
		}
		
		if( *cptr == '"' )
		{
                	quoted_flag = !quoted_flag;
			prev_was_white = FALSE;
		}
        	else if( *cptr == '-' && prev_was_white )  /* Start of parameter? */
		{
			is_param_specifier = TRUE;
			if( !quoted_flag ) /* Not already quoted ? */
			{
				in_param = TRUE;
				*lc_cptr = '"';
				lc_cptr++;
			}
			prev_was_white = FALSE;
		}
		else if( CMwhite(cptr) )
		{
			prev_was_white = TRUE;
			if( in_param )
			{
				in_param = FALSE;
				*lc_cptr = '"';
				lc_cptr++;
			}
		}
		else
		{
			prev_was_white = FALSE;
		}
		if (do_lower)
		    CMtolower(cptr, lc_cptr);
		else
		    CMcpychar(cptr, lc_cptr);
	}
	if( in_param )
	{
		*lc_cptr++ = '"';
	}
	*lc_cptr = EOS;
        cmd_ptr = lc_cmd;
#else
        cmd_ptr = cmd;
#endif  /* DOUBLEBYTE */

#else
        cmd_ptr = cmd;
#endif  /* VMS */

	EXinterrupt(EX_ON);
	if ((flags & LONGCMD) && STlength(cmd_ptr) > DCLCOMLINE-2)
	{ /* extra long Front-End command */
		char	*cp;
		int	len;

		/* Front-End commands that exceed the command mailbox buffer
		** size are executed by sending the command-line minus the
		** command through the mailbox after the command is executed
		** with the /EXTRA parameter specifying the mailbox.
		**
		** All INGRES Front-End programs have an entry point defined
		** in "clf!misc!frontstart.c", which reads the command line
		** either from its arguments, or if the /EXTRA parameter was
		** specified, from the mailbox.  Command-lines written in this
		** way need not be concerned with continuation characters.
		*/


		/* Separate command from rest of the command line */
		for ( cp = cmd_ptr ; !CMwhite(cp) ; CMnext(cp) )
			;
		len = cp - cmd_ptr;

		/* Execute command with /EXTRA parameter */
		(void)STlcopy( cmd_ptr, buf + 2, len );
		(void)STprintf(buf + 2 + len, "/EXTRA=%s", mbxname);
		(void)PCiwrite(mbxchan, buf);


		/* Send down rest of command line */
		if ( PCfewrite(mbxchan, cp) != OK )
			goto errreturn;
	}
	else
	{
		STlcopy(cmd_ptr, &(buf[2]), DCLMAX-3); 
		if ( PCiwrite(mbxchan, buf) != OK ) 
			goto errreturn;
	}

	/* Get command status.
	**
	** Read status right away to avoid possibility of mailbox overflow
	** and deadlock between parent and sub processes. (agh)
	*/
	reset_ctrlc = IIctrlc_is_trapped() && (flags & NOCTRLCTRAP != 0);
	if (reset_ctrlc)
		IInotrap_ctrlc();	/* Release our ^C AST */
	(void)PCiwrite(mbxchan, "$ write rbox $status");
	size = DCLCOMLINE;
	rstat = read(statchan, buf, &size);
	if (reset_ctrlc)
		IItrap_ctrlc();		/* Resume our ^C AST */
	if (rstat != OK)
	{
		rval = rstat;
		goto errreturn;
	}
	buf[size] = '\0';
	CVahxl(&buf[2], &rstat);

	/*
	** Disable all standard input and output channels, again.
	**
	** BUG 4069 -- Must turn messages off again so that spurious error
	** messages (e.g., "No logical name match") do not appear:
	**	$ set message/nofac/noid/nosev/notext
	*/
	(void)PCiwrite(mbxchan, _TurnOffMsg);
	if (!(flags & NOINPUT))
	{ /* disable input ... */
		/*	$ deassign SYS$INPUT	*/
		(void)PCiwrite(mbxchan, _DisableInput);
		/*	$ deassign SYS$COMMAND	*/
		(void)PCiwrite(mbxchan, _DisableCmd);
	}
	if (!(flags & NOOUTPUT))
	{ 
		/* If we're appending to a file, don't close it yet */
		if (!(append && log_open))
			PCcmdcloseout(TRUE);
	}

	/*	$ set message/fac/id/sev/text	*/
	(void)PCiwrite(mbxchan, _TurnOnMsg);

	EXdelete();
	EXinterrupt(EX_OFF);

	if (!((rval = sys$cantim(timerid, 0)) & 01))
		goto errreturn;
	if (!((rval = sys$setimr(EFN$C_ENF, deltim, PCitimer, timerid, 0)) & 01))
		goto errreturn;
	rval = rstat & 01 ? OK : rstat;
	EXinterrupt(EX_ON);
	return rval;

	/*
	** Bad error.  Cancel timer, free event flags.
	*/
   errreturn:
	PCcleanup(ex);
	EXinterrupt(EX_ON);
	return error->error = rval;
}

/*
** Name:	PCiwrite() -	Write DCL Command to MailBox.
**
** Description:
**	Write command-line to sub-process through the mail box.  Since the mail-
**	box buffer is limited in size to DCLCOMLINE (128) bytes, this routine
**	must break up the command-line into smaller lines appending the DCL
**	continuation character to each line.
**
**	Note because of the mailbox buffer size limitation, quoted (") string
**	parameters are limited in size to DCLCOMLINE - 2 - 1 (125) bytes (two
**	positions for the quotes, themselves, and one for the continuation
**	character.)  
**
**	Errors from this are generally ignored, except when the above string
**	parameter limit is exceeded.  Since the command should not be executed
**	if this occurs, the sub-process should be terminated to ensure that it
**	does not execute.
**
** History:
**	20-mar-90 (jhw) -- Get size using 'STlength()' here.  Also, modified
**	"break-line" algorithm to place continuation character (-) anywhere
**	but in a quoted (") string.  This corrects both #20035 and #20204.
*/
static STATUS
PCiwrite(
II_VMS_CHANNEL	mbx,
char	*buf)
{
	int	size = STlength(buf);
	int	rval;
	char	line[DCLCOMLINE];

# ifdef LOG_MBX_CMDS
	SIprintf("mbx: %s\n", buf);
	SIflush(stdout);
# endif

	/* Break up command into blocks to be written to the mail box */

	while (size > DCLCOMLINE)
	{
		char	quote;
		char	*cp;
		char	*brk;
		int	len;

		MEcopy(buf, DCLCOMLINE, line);
		/*
		** The command is too long to pass in one write.  A hyphen (-)
		** has to be put in to make DCL expect more input.  Find a
		** convenient place for the hyphen, that is everywhere but
		** within a quoted (") string.
		*/
		quote = '\0';
		brk = NULL;
		for ( cp = line ; cp <= &line[DCLCOMLINE - 1] ; CMnext(cp) )
		{
			if ( quote == '\0' )
			{ /* not in quoted string */
				brk = cp;
				if ( *cp == '"' )
					quote = *cp;
			}
			else if ( *cp == quote )
			{ /* end quote */
				quote = '\0';
			}
		}

		if ( brk == line )
		{ /* quoted string too large */
			return E_CL_MASK + E_PC_MASK + 1;	/* fatal */
		}

		*brk = '-';
		len = brk - line;
		/* Write all of line upto and including "-" */
		if ( (rval = write(mbxchan, line, len + 1)) != OK )
			return rval;

		buf += len;
		size -= len;
	} /* end while */

	if ((rval = write(mbxchan, buf, size)))
		return rval;

	return OK;
}

/*
** History:
**	20-mar-90 (jhw) -- Get size using 'STlength()' here.
*/
static STATUS
PCfewrite(
int	mbx,
char	*buf)
{
	int	size = STlength(buf);
	int	rval;
	char	line[DCLCOMLINE];

	while (size >= DCLCOMLINE)
	{
		if ((rval = write(mbxchan, buf, DCLCOMLINE)))
			return rval;
		buf += DCLCOMLINE;
		size -= DCLCOMLINE;
	}
	/*
	** We are writing to FRONTMAIN, which uses SIread() to read from a
	** mailbox.  When SIread() reads from a mailbox it always tries to read
	** as much as asked for.   This means we must always write a fixed size
	** record, and the NUL in the string will get sent over too.  FRONTMAIN
	** will look for the NUL to decide if it should quit.
	*/
	MEfill(DCLCOMLINE, '\0', line);
	if ( size > 0 )
		MEcopy(buf, size, line);
	if ((rval = write(mbxchan, line, DCLCOMLINE)))
		return rval;
	return OK;
}

/*
** We shouldn't get an interrupt while the subprocess is
** running, but if we do reset the timer. Otherwise,
** timer has expired.  In that case, delete the subprocess.
*/
static
PCitimer()
{
        EX ex;

	sys$setast(0);
	if (incom)
	{
		sys$setimr(EFN$C_ENF, deltim, PCitimer, timerid, 0);
	}
	else
	{
		PCcleanup(ex);
	}
	sys$setast(1);
}

static int
read(
II_VMS_CHANNEL	chan,
char	*buf,
int	*size)
{
	int	rval;
	IOSB	iosb;

        rval = sys$qiow(EFN$C_ENF, chan, IO$_READVBLK, &iosb, 0, 0, buf, *size,
			0, 0, 0, 0);
	if (rval & 1)
	    rval = iosb.iosb$w_status;
	if (!(rval & 1))
	    return rval;
	*size = iosb.iosb$w_bcnt;
	return OK;
}

static int
write(
II_VMS_CHANNEL	*chan,
char	*buf,
int     size)
{
	int	rval;
        IOSB    iosb;

	if ( size > 0 )
	{
	    rval = sys$qiow(EFN$C_ENF, chan, IO$_WRITEVBLK|IO$M_NOW, &iosb, 0,
			    0, buf, size, 0, 0, 0, 0);
	    if (rval & 1)
		rval = iosb.iosb$w_status;
	    if (!(rval & 1))
		return rval;
 	}
       return OK;
}

static int
mailbox(
struct dsc$descriptor_s	*name,
II_VMS_CHANNEL		*chan)
{
	int	rval;

	if (!((rval = sys$crembx(0, chan, DCLCOMLINE, 0, 0, 0, name)) & 01))
		return rval;
	return trnlog(name);
}

static int
trnlog(
struct dsc$descriptor_s	*name)
{
	short			len;
	int			rval;
	struct dsc$descriptor_s	xlate;
	char			buf[256];

	xlate.dsc$a_pointer = buf;
	xlate.dsc$w_length = sizeof(buf);
	xlate.dsc$b_class = DSC$K_CLASS_S;
	xlate.dsc$b_dtype = DSC$K_DTYPE_T;
	if (!((rval = sys$trnlog(name, &len, &xlate, 0, 0, 0)) & 01))
		return rval;
	name->dsc$w_length = STlcopy(buf, name->dsc$a_pointer, len);

	return OK;
}

static EX
PChandler(EX_ARGS *exc_args)
{
    if (EXmatch(exc_args->exarg_num, 1, EXINTR))
    {
	if (PCcatch)
		sys$forcex(0, &pdesc, SS$_ABORT);

	return EX_CONTINUE;
    }
    else
        return EX_RESIGNAL;
}

static
PCexith()
{
	sys$delprc(0, &pdesc);
	incom = FALSE;
	log_open = FALSE;
	mbxchan = 0;
}

/*
** Description:
**	Close mail box and terminate sub-process.
*/
void
PCcleanup(EX except)
{
	sys$cantim(timerid, 0);
	sys$dassgn(statchan);
	sys$dassgn(mbxchan);
	if (sys$delprc(0, &pdesc) != SS$_NONEXPR)
		sys$waitfr(peflag);
	lib$free_ef(&peflag);
	sys$canexh(&pcexitblk);
	statchan = 0;
	mbxchan = 0;
	incom = FALSE;
	log_open = FALSE;
}

/*
** Description:
**	Close the output file, and redirect output and error output to the
**	null device.
*/
STATUS
PCcmdcloseout(
bool	msgs_off)	/* Are messages already turned off ? */
{
	char	buf[DCLMAX];
	int	size = DCLCOMLINE;

	/*
	** Disable output and error output.  It's important to deassign
	** the logicals before redefining them; that's when the log
	** file gets closed.
	*/
	if (!msgs_off)
		(void)PCiwrite(mbxchan, _TurnOffMsg);

	(void)PCiwrite(mbxchan, _DeassignOutput);
	(void)PCiwrite(mbxchan, _DeassignError);
	(void)PCiwrite(mbxchan, _DisableOutput);
	(void)PCiwrite(mbxchan, _DisableError);

	(void)PCiwrite(mbxchan, "$ write rbox $status");
	(void)read(statchan, buf, &size);

	if (!msgs_off)
		(void)PCiwrite(mbxchan, _TurnOnMsg);

	log_open = FALSE;
	return OK;
}
/*
** Description: Get process' TERMINAL and MODE setting
**              to set settings specially for UTExe calls
**              from within NETWORK processes (e.g. HTTPD servers)
*/

static 
STATUS	getprocdets(char *mode, char *us)
{
	ILE3	itemlist[3];
	STATUS	PCstatus;
	int	modeval;
	IOSB	iosb;
	
	itemlist[0].ile3$w_length = sizeof(modeval);
	itemlist[0].ile3$w_code = JPI$_MODE;
	itemlist[0].ile3$ps_bufaddr = &modeval;
	itemlist[0].ile3$ps_retlen_addr = NULL;

	itemlist[1].ile3$w_length = MAX_LOC;
	itemlist[1].ile3$w_code = JPI$_USERNAME;
	itemlist[1].ile3$ps_bufaddr = us;
	itemlist[1].ile3$ps_retlen_addr = NULL;

	itemlist[2].ile3$w_length = NULL;
	itemlist[2].ile3$w_code = NULL;
	itemlist[2].ile3$ps_bufaddr = NULL;
	itemlist[2].ile3$ps_retlen_addr = NULL;

	PCstatus = sys$getjpiw(EFN$C_ENF, 0, 0, &itemlist, &iosb, 0, 0);
	if (PCstatus & 1)
	    PCstatus = iosb.iosb$w_status;
	if (PCstatus == SS$_NORMAL)
	{
		STcopy(modes[modeval], mode);
		return OK;
	
	}
	else
		return PCstatus;
}

