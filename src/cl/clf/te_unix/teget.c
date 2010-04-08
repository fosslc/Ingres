/*
**
**	TEget.c
**
**	Copyright (c) 1986, 2004 Ingres Corporation
**	All rights reserved.
*/

/*
** Name:	TEget - Get a character
**
** Description:
**	Return the next character in the lookahead buffer or go
**	ask the O/S for the next character.  Return EOF when EOF
**	is encountered.  Note that the implementation does not
**	require the use of a lookahead buffer.
**
**	If the first parameter is greater than zero and there is
**	a need to ask the O/S for a character, then only wait
**	the number of seconds as specified in the first parameter.
**	A timeout has occurred if no characters are returned within
**	the timeout period (as specified by the first parameter).
**	For the timeout case, set the second parameter to TRUE to
**	let caller know that a timeout has occurred and return NULL.
**
**	In all other cases, the second parameter will be set to FALSE.
**	and the (first) character received from the lookahead buffer
**	or O/S will be returned.
**
**	If the first parameter is zero or negative, or in environments
**	that do not support input timeout, TEget will simply wait forever
**	for user input (as it currently does).
**
**	When obtaining characters from a file (as is the case when
**	running in testing mode), TEget will ignore timeout.
**
**	(Sigh) This used to be a macro that called getc().  getc() doesn't
**	distinguish between a read that was interrupted by a signal, hence
**	failed with EINTR, and end-of-file; it returns EOF in either case.
**	So we implement our own.  Another function call, another layer
**	of buffering.
**
**
** Inputs:
**	seconds		Timeout period in wallclock seconds.
**
** Outputs:
**	Returns:
**		char	Character obtained from lookahead buffer or O/S.
**			EOF when EOF encountered.
**			TE_TIMEDOUT when a timeout occurs.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	XX/XX/XX (XXX) - Initial version.
**	12/01/88 (dkh) - Added support for timeout.
**	08-may-89 (russ)
**		Remove itimer in favor of alarm(), since we're only using
**		timeouts in seconds anyway.  Also allow for the use of
**		signal() if sigvec is not available.  Also removing unused
**		variable timeoutsecs.
**	23-may-89 (mca)
**		Added support for reading from a TCFILE (for SEP test tool.)
**	29-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Added sigaction code;
**		Added machine-specific code for sqs_us5 to handle ^Z.
**      10-sep-1990 (jonb)
**          Integrated ingres6301 change 280140 (jkb):
**              undefine xCL_011_USE_SIGVEC for sqs_us5 so SYSV signal
**              semantics are used.  This is necessary for the alarm call
**              to work properly.
**              (Backout the icanon change described above and) use UCB ioctl
**              semantics to get job control.  [ The aforementioned icanon
**              change was never in so I can't back it out ... jonb ]
**              Turning on and off icanon around the read cause a race
**              condition in which characters may get lost.  This caused
**              function keys to not work in some cases
**	    Further note from jonb: this change also backs out the fake
**	    job control for sqs_us5.  
**	10-sep-1990 (jonb)
**	    Replace line deleted in last change.
**	11/08/90 (dkh) - Changed input buffers to u_char to better support
**			 8 bit input.
**	01-Oct-91 (Sanjive)
**	    Integrate fix to alarmhdlr() from ingres6302p.
**      	18-Apr-1991 (Sanjive)
**          	Integrated fix to alarmhdlr() from 6202u (21-aug-90 (chandra)).
**    24-feb-92 (jillb)
**        Added dg8_us5 to ifdef for restoring original signal mask.  On
**        dg it must be done manually.
**  20-feb-92 (aking)
**      Added a check for bu2_us5 and bu3_us5 for Bull's DPX/2 system in the
**      alarmhdlr() function - Bull uses sigaction, but does not have the
**      structure containing the member 'uc_sigmask'.
**    16-dec-92 (bconnell)
**      	Added odt_us5 to those platforms that use SIGACTION
**	12/30/92 (mikem)
**	    su4_us5 port.  Replaced ?? and ??? in comments with XX XXX to
**	    eliminate compiler warning "trigraph sequence replaced".
**
**	STILL NEED TO HANDLE READING INPUT FROM FILES (FOR TESTING).
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	19-may-94 (twai/kirke)
**	    Merged changes from ingres63p.
**      13-oct-94 (canor01)
**          AIX 4.1-specific usage of sigsetmask() function
**	22-jun-95 (allmi01)
**	    Added dgi_us5 support for DG-UX on Intel based platforms following dg8_us5.
**    14-jul-95 (morayf)
**          Added SCO sos_us5 to those platforms that use SIGACTION.
**	16-jan-1996 (toumi01; from 1.1 axp_osf port) (schte01)
**	    Added axp_osf to test to retore signal mask.
**	12-jan-1998 (fucch01)
**	    Re-ordered includes to deal with compile errors on sgi_us5.
**	    Moved clconfig.h and clsigs.h up so they're right after compat.h
**	    and added systypes.h to get rid of a new error that came up.
**	18-feb-1998 (toumi01)
**	    Add Linux (lnx_us5) to list of platforms resetting the signal mask
**	    with sigemptyset, sigaddset, & sigprocmask.
**      19-Oct-1998 (muhpa01)
**          bug 93041 (hpb_us5) - Add hpb_us5 to list of platforms which use
**          sigemptyset, sigaddset, & sigprocmask to reset the signal mask.
**          This was causing forms runtime timeout failure on HP-UX 11.00 64-bit
**      22-Jun-1999 (podni01) Ignore sigaction for rmx_us5 because Siemens
**          v5.44 sigaction() routine only works for synchronous signals,
**          use signal instead.
**      10-may-1999 (walro03)
**          Remove obsolete version string bu2_us5, bu3_us5, odt_us5, sqs_us5.
**	01-jul-1999 (toumi01)
**		Move IIte_fpin = stdin init to run time, avoiding compile error
**		under Linux glibc 2.1: "initializer element is not constant".
**		The GNU glibc 2.1 FAQ claims that "a strict reading of ISO C"
**		does not allow constructs like "static FILE *foo = stdin" for
**		stdin/stdout/stderr.
**      03-jul-99 (podni01)
**          Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**	06-oct-1999 (toumi01)
**	    Change Linux config string from lnx_us5 to int_lnx.
**      13-oct-1999 (hweho01)
**          Added ris_u64 to list of platforms which use sigsetmask() function.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**      10-Jan-2000 (hanal04) Bug 99965 INGSRV1090.
**          Correct rux_us5 changes that were applied using the rmx_us5
**          label.
**	14-Jun-2000 (hanje04)
**	    Add ibm_lnx to list of platforms resetting the signal mask
**	    with sigemptyset, sigaddset, & sigprocmask.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-Sep-2000 (hanje04)
**	    Add Alpha Linux (axp_lnx) to list of platforms resetting the signal **	    mask with sigemptyset, sigaddset, & sigprocmask.
**	23-jul-2001 (stephenb)
**	    Add support for i64_aix
**	17-aug-2001 (somsa01 for fanra01)
**	    SIR 104700
**	    Add handling for the TEstreamEnabled case for direct stream I/O.
**	04-Dec-2001 (hanje04)
**	    IA64 Linux added to signal mask resetting change.
**      08-Oct-2002 (hanje04)
**          As part of AMD x86-64 Linux port, replace individual Linux
**          defines with generic LNX define where apropriate
**      08-Oct-2002 (hweho01)
**          For rs4_us5, use xCL_068_USE_SIGACTION on AIX 5.1.
**	19-Feb-2003 (hanje04)
**	    Added missing \ from bad cross of AMD x86-64 changes.
**      18-Apr-2005 (hanje04)
**          Add support for Max OS X (mac_osx)).
**          Based on initial changes by utlia01 and monda07.
**	14-Feb-2008 (hanje04)
**	    SIR S119978
**	    Update includes for renamed header
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/

# include	<compat.h>
# include 	<systypes.h>
# include	<clconfig.h>
# include	<clsigs.h>
# include	<gl.h>
# include	<si.h>
# include	<te.h>
# include	<tc.h>
# include	<errno.h>
# include	<setjmp.h>

# define	TEBUFFER	32	/* amount we buffer btwn. syscalls */
# define	TIMEEXPR	2000	/* indicates a timeout occurred */

extern	FILE	*IIte_fpin;
extern	i4	errno;

GLOBALREF bool	TEstreamEnabled;

/* New vars for use with reading from a TCFILE. (mca) */
extern int	IIte_use_tc;
extern TCFILE	*IItc_in;

static	jmp_buf	context = {0};

# ifdef xCL_011_USE_SIGVEC
static	struct	sigvec		newvec = {0};

static	bool	firsttime = TRUE;
# endif

# ifdef xCL_068_USE_SIGACTION
static  struct  sigaction               newaction;

static  bool    firsttime = TRUE;
# endif

static TYPESIG
alarmhdlr(signum, code, scp)
int signum;
EX_SIGCODE SIGCODE(code);
EX_SIGCONTEXT SIGCONTEXT(scp);
{

# ifdef xCL_011_USE_SIGVEC
    /*
    ** Must restore the original signal mask.  The kernel blocked
    ** the current signal (SIGALRM) on entry to the handler, but we are
    ** about to longjmp out of the handler.
    */
# if  defined(ris_u64)
    /* sc_maks is a sigset_t, which in AIX 4.1 is a structure of two
    ** 32-bit integers.  We only need to reference the lower 32 bits here.
    */
    (void)sigsetmask(scp->sc_mask.losigs);
# else
    (void)sigsetmask(scp->sc_mask);
# endif /* ris_u64 */

# else
# if defined(xCL_068_USE_SIGACTION)
    /*
     ** restore original signal mask.
     */
# if defined(dol_us5) || defined(sqs_ptx) || defined(dg8_us5) \
  || defined(dgi_us5) || defined(sos_us5) || defined(axp_osf) \
  || defined(thr_hpux) || defined(any_aix) || defined(LNX) \
  || defined(OSX)
	{
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask,signum);
	sigprocmask(SIG_UNBLOCK, &mask, (sigset_t *)0);
	}
# else
     sigprocmask(SIG_SETMASK, &(scp->uc_sigmask), (sigset_t *)0);
# endif /* dol_us5 */

# else
# if defined(xCL_086_USE_SIGSET)
     	/*
     	** Released SIGALRM from blocked signals.
     	*/
     	(void) sigrelse(SIGALRM);

# endif /* xCL_086_USE_SIGSET */
# endif /* xCL_068_USE_SIGACTION */
# endif /* not xCL_011_USE_SIGVEC */

    longjmp(context, TIMEEXPR);
	 /*NOTREACHED*/
}


i4
TEget(seconds)
i4	seconds;
{
    static u_char buffer[TEBUFFER];
    static u_char *nxtvalid;
    static i4  bufcount = 0;
# ifdef xCL_011_USE_SIGVEC
    struct	sigvec		oldvec;
# else
# ifdef xCL_068_USE_SIGACTION
# if defined(rux_us5)
        TYPESIG (*oldvec)();
# else
        struct     sigaction               oldaction;
# endif
# endif	/* xCL_068_USE_SIGACTION */
# endif	/* xCL_011_USE_SIGVEC */
    register i4  c;
    char *cp;

    if (TEstreamEnabled)
    {
	if (seconds > 0)
	    return(TE_TIMEDOUT);
	else
	    return((char)getchar());
    }

    /* If we're in TCFILE mode, read from the input TCFILE. */
    if (IIte_use_tc)
	return(TCgetc(IItc_in, seconds) );

    /*
    **  Need to ask UNIX for some characters.
    **  If timeout is set, then set alarm before
    **  doing "read" system call.  If we return due
    **  to a the alarm going off then a timeout has
    **  occured.  Return TE_TIMEDOUT in this case.
    */
    while ( bufcount <= 0 )
    {
	nxtvalid = buffer;

	/*
	**  Set alarm.
	*/
	if (seconds > 0)
	{
# ifdef xCL_011_USE_SIGVEC
	    if (firsttime)
	    {
		newvec.sv_handler = alarmhdlr;
		newvec.sv_mask = 0;
		newvec.sv_onstack = 0;

		firsttime = FALSE;
	    }
# endif
# ifdef xCL_068_USE_SIGACTION
            if (firsttime)
            {
                newaction.sa_handler = alarmhdlr;
                sigemptyset(&(newaction.sa_mask));
                newaction.sa_flags = 0;

                firsttime = FALSE;
            }
# endif

	    /*
	    **  If we are returning here due to alarm
	    **  going off, then a timeout has occured.
	    */
	    if (setjmp(context) == TIMEEXPR)
	    {
# ifdef xCL_011_USE_SIGVEC
		sigvec(SIGALRM, &oldvec, NULL);
# else
# ifdef xCL_068_USE_SIGACTION
#  if defined(rux_us5)
                signal(SIGALRM, oldvec);
#  else
                sigaction(SIGALRM, &oldaction, (struct sigaction *)0);
#  endif
# endif	/* xCL_068_USE_SIGACTION */
# endif	/* xCL_011_USE_SIGVEC */
		return(TE_TIMEDOUT);
	    }

	    /*
	    **  Set signal handler to capture alarm and
	    **  then set the timer.
	    */
# ifdef xCL_011_USE_SIGVEC
	    sigvec(SIGALRM, &newvec, &oldvec);
# else
# ifdef xCL_068_USE_SIGACTION
#  if defined(rux_us5)
            oldvec = signal(SIGALRM, alarmhdlr);
#  else
            sigaction(SIGALRM, &newaction, &oldaction);
#  endif
# endif	/* xCL_068_USE_SIGACTION */
# endif	/* xCL_011_USE_SIGVEC */
	    alarm(seconds);
	}

	if (IIte_fpin == NULL)
		IIte_fpin = stdin;
	bufcount = read( fileno(IIte_fpin), buffer, TEBUFFER );

	if (seconds > 0)
	{
	    /*
	    **  Got here before alarm went off, reset the
	    **  timer.  Note that we may be here due to
	    **  the read having been interrupted.  This
	    **  should be OK.
	    */
	    alarm(0);
	}

	if ( bufcount == 0 || (bufcount < 0 && errno != EINTR) )
	    return( EOF );
    }
    bufcount--;
    c = *nxtvalid++;
    return( c );
}
