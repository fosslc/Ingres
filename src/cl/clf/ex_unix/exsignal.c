/*
**Copyright (c) 2004 Ingres Corporation
*/
# include 	<compat.h>
# include 	<gl.h>
# include	<systypes.h>
# include	<clconfig.h>
# include	<clsigs.h>
# if defined (sgi_us5)
# include	<sys/fpu.h>
# endif
# include	<ex.h>
# include 	<er.h>
# include 	<si.h>
# include	"exi.h"
# include	"exerr.h"
# include       <evset.h>

# include	<stdarg.h>

/*
** exsignal.c
**
**	History:
**		written 3/10/83 (jrc)
 * Revision 1.2  88/09/28  14:10:22  root
 * sc
 *
 * Revision 1.1  88/09/21  21:13:57  root
 * This is the new stuff.  We go dancing in.
 *
 * Revision 1.1  88/08/05  13:36:24  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 *
**      Revision 1.7  88/06/09  13:41:32  anton
**      added escape to true unix signals
**
**      Revision 1.6  88/04/07  15:19:50  anton
**      more jupiter changes
**
**      Revision 1.5  88/03/04  12:27:01  anton
**      do bsd signaling right
**      no more backend
**
**		Revision 30.3  86/06/04  19:54:06  jas
**		Map SIGALRM to new exception EXTIMEOUT
**		(local to UNIX, used in KN).
**
**		Revision 1.3  86/04/25  16:57:29  daveb
**		Add void casts for 4.2 code to pass lint.
**
**		Revision 1.2  86/04/25  14:56:40  daveb
**		Save original signal state when
**		setting up our exception system.  If one is
**		later received, and we don't have a handler
**		in place (or resignal through all of our
**		handlers), reinstall the original and resignal.
**
**		Revision 1.2  86/02/25  21:18:17  perform
**		include IN.h for INlogintr()
**
**	17-Oct-1988 (daveb)
**		Change to use new ifdefs (use_sigvec)
**	6-Jan-1989 (daveb)
**		Do not allow raising an interrupt within an interrupt.  This
**		prevents some possible re-entrancy problems.  
**
**		Correct comment about calling user signal handlers.  SV
**		and BSD behavior is the same, and has been for some time.
**	26-may-89 (daveb)
**		silence warnings by declaring handles TYPESIG instead of int.
**	01-May-1990 (Kimman)
**	    On the ds3_ulx (should apply to any other MIPS RISC boxes), integer
**	    divided by zero generates SIGTRAP not SIGFPE. Also, pc isn't
**	    increment after the exception. Added code to trap the signal for
**	    ds3_ulx and advance the pc to skip the exception instruction.
**      01-May-89 (kimman)
**              No longer catch SIGC[H]LD as it's done in ingresug.
**	16-may-90 (blaise)
**	    Integrated changes from ug:
**		Added function descriptions;
**		Silence some warnings by using TYPESIG instead of the often
**		incorrect int;
**		Simplify i_EXcatch by passing three arguments for all signal-
**		generated exceptions;
**		Handle "hard" case remapping;
**		Set traps for all signals up to MAXSIG, rather than just the
**		first 16;
**		Added hp8_us5 specific routine to allow a floating point
**		handler to return to the instruction after a trap;
**		Added dg8_us5 specific routine to decode SIGFPE faults;
**		Added knowledge of sigaction (POSIX equivalent of sigvec);
**		Changed i_EXcatch() declaration to use <clsigs.h> macros;
**		Remove references to dead variables i_EXigint and i_EXighup.
**		Added new signals: for example SIGXCPU, SIGXFSZ, SIGPWR
**		SIGUSR1, SIGUSR2, SIGSYSV;
**		Added machine-specific code for pyr_u42, pyr_us5, hp8_us5,
**		rtp_us5, ps2_us5, dg8_us5.
**	21-may-90 (blaise)
**	    Removed incorrect terminating colon from "# ifdef SIGXFSZ:".
**	21-may-90 (blaise)
**	    Removed duplicate SIGSTOP, and added missing commas, in
**	    initialisation of sigs_to_catch array.
**	21-may-90 (blaise)
**	    Added missing "struct sigvec new, old" declaration.
**	23-may-90 (rog)
**	    EXds_kybd_intrpt() and EXen_kybd_intrpt() were turned into
**		no-ops in ex.h, so they needed to be removed from this file.
**	4-sep-90 (jkb)
**	   restore original signal mask for sqs_ptx
**	6-Sep-90 (daveb)
**	    Remove traps of SIGTSTP, STOP, TTIN TTOU; they shouldn't be 
**	    mapped into exceptions.  Their SIG_DFL action is harmless and
**	    there is no way for mainline to handle them.  The 
**	    result was that sending ^Z to the terminal monitor exited
**	    with an "unknown exception" messsage.  This is not what
**	    was desired.  The error was introduced by overzealous
**	    integration or the 6.2 changes; this change should be taken
**	    by all 6.3/02 ports.
**	4-oct-90 (jkb)
**	   restore original signal mask for sqs_ptx correctly, add code to
**	   clear floating point stack after exception
**      08-jan-91 (jkb)
**              Remove seq_fpe function for sqs_us5.  All floating point
**              errors are now hard errors so it is no longer needed.
**	25-mar-91 (kirke)
**	    Added #include <systypes.h> because HP-UX's signal.h includes
**	    sys/types.h.  Removed sqs_us5 code duplicated in clsigs.h.
**	27-mar-91 (rudyw)
**	    ifdef a section of code dealing with "Fake" values which causes
**	    redefinition problems for ODT compiler.
**	15-apr-91 (vijay)
**	    Removed SIGDANGER from the fatal signal list.
**	07-apr-1992 (mgw) Bug #42098
**	    We shouldn't be catching signals whose SIG_DFL action is
**	    harmless as stated below. This includes SIGPOLL so delete it
**	    from sigs_to_catch[]. This will fix bug 42098 where calling
**	    emacs from our tools was crashing.
**	06-jul-92 (swm)
**	    Bug #36524. Integration of Paul Andrews (pauland) fix: stop
**	    recursive processing of SIGHUP to prevent front end processes
**	    writing to a dead terminal growing in size.
**	05-mar-93 (mikem)
**	    su4_us5 port.  Added special handling for integer overflow to allow
**	    continues after the signal.  Default signal handling would not 
**	    advance the PC after the integer overflow, as is done automatically
**	    for floating point exceptions.
**	16-mar-93 (swm)
**	    On axp_osf integer division by zero causes SIGTRAP rather than
**	    SIGFPE. Added code to return EXINTDIV exception when SIGTRAP is
**	    caught.
**      16-mar-93 (smc)
**          Added axp_osf to the list of platforms that don't have a
**          signal context containing uc_sigmask.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	23-Jul-1993 (daveb)
**		handler return is STATUS, not EX, or i4.	    
**	26-Jul-1993 (rog)
**	    Added i_EXrestoreorigsig() as a companion to i_EXsetothersig().
**	10-feb-1994 (ajc)
**	    Added hp8_bls entries based on hp8*.	
**	21-mar-94 (dave)
**		Added support for new entry point EXsetclient().
**	21-mar-94 (peterk)
**		case statement needs non-empty code on HP c89 compiler;
**		move continue outside of #ifdef; identify "dkh" as "dave"
**		in history comments.
**	13-oct-94 (canor01)
**	    AIX 4.1 sigset_t is a structure for sigsetmask()
**      7-apr-95 (abowler)
**	    Fixes for 55538 and 52338 taken from 6.4 (nick):-
**          Under AIX, SIGLOST is mapped to SIGIOT/SIGABORT and therefore
**          we don't want to catch it. Also redefined setsig to EXsetsig.
**      30-Nov-94 (nive/walro03)
**        dg8_us5 uses sigaction now instead of sigvec.  Therefore all of
**        the dg8_us5 specific sigvec code has been removed
**	28-Apr-95 (georgeb)
**	    Added ifndef for usl_us5 to prevent the use of NSIG.
**      1-may-95 (wadag01)
**	    Aligned with 6.4/05:
**          Added odt_us5 to those platforms which have sigaction,
**          but the signal handler is not BSD like.
**      14-jul-95 (morayf)
**          Added sos_us5 to those platforms which have sigaction.
**	19-sep-95 (nick)
**	    Changes to support stack overflow guard pages.
**	16-jan-1996 (toumi01; from 1.1 axp_osf port) (schte01)
**	    Changed type of arguments to long for axp_osf to match EX_ARGS
**	    *exarg_array, signal and code (see excl.h). Changed arp to ptr
**	    to long; changed argarray to array of longs.
**	23-jan-1996 (sweeney)
**	    Tab {} delimited code block in to avoid confusing ctags,
**	    which otherwise thinks the } on an empty line is the end
**	    of the function i_EXcatch().
**	08-dec-95 (morayf)
**	    Added SNI changes for RMx00 Pyramid clones (rmx_us5).
**	12-nov-96 (kinpa04)
**		Extended exception handling improvements regarding alternate
**		stack and context passing to axp_osf
**	29-jul-1997 (walro03)
**	    Correct compile errors on Tandem NonStop (ts2_us5).  NSIG was
**	    defined in signal.h as MAXSIG+1, then we defined MAXSIG to be NSIG.
**      18-aug-1997 (hweho01)
**          Renamed EX_OK to EX_OK_DGI for dgi_us5, because  
**          it is defined in /usr/include/sys/_int_unistd.h.   
**	13-jan-1998 (muhpa01)
**	    Add hp8_us5 to list of platforms resetting the signal mask
**	    with sigemptyset, sigaddset, & sigprocmask
**      18-feb-98 (toumi01)
**	    Add Linux (lnx_us5) to list of platforms resetting the signal mask
**	    with sigemptyset, sigaddset, & sigprocmask.
**	    #ifdef SIGSYS (doesn't exist on Linux).
**	13-apr-1998 (muhpa01)
**	    Added changes for HP-UX 11.0 32-bit, hpb_us5: changes for
**	    sigaction, create & use an alternate stack, & don't call
**	    the adjust_fpe() routine.
**	28-jul-1998 (devjo01)
**	    Do not catch SIGLOST if it is just an alias for SIGABRT.
**      08-aug-1998 (hweho01)
**          Renamed EX_OK to EX_OK_DG8 for dg8_us5, because
**          it is defined in /usr/include/sys/_int_unistd.h.
**	09-oct-1998 (schte01)
**	    Changed EXaltstack for axp_osf to use sigaltstack instead of
**	    sigstack, the latter is soon to be obsolete.          
**      13-jan-1999 (hweho01)
**          Changed EX_OK to EX_OK1 for all platforms.
**	18-jan-1999 (muhpa01)
**	    Don't call adjust_fpe() for hp8_us5.
**	23-Feb-1999 (kosma01)
**	    IRIX 6.x default behaviour is to NOT generate SIGFPE.
**	    set_fpc_csr causes it to generate SIGFPE.
**	    Returning from signal handler will loop without pc adjustment.
**	    Add sgi_us5_adjust_pc to handle that circumstance.
**      10-may-1999 (walro03)
**          Remove obsolete version string bu2_us5, bu3_us5, odt_us5, pyr_u42,
**          rtp_us5, ps2_us5, sqs_us5.
**      22-jun-1999 (popri01)
**          For Siemens (rmx_us5), revert sigaction to signal because
**          the threaded version of sigaction does not support 
**          asynchronous signals, including the SIGUSR2 "wake-up"
**          that Ingres employs.  In addition, the Siemens implementation
**          of POSIX threads reserves and uses SIGVTALRM as a thread
**          time-slicing mechanism. So we must ignore it.
**      03-jul-1999 (podni01)
**          Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**      02-Aug-1999 (hweho01)
**          Changed EXCONTINUE to EXCONTINUES to avoid compile error of
**          redefinition on AIX 4.3, it is used in <sys/context.h>.
**      10-Jan-2000 (hanal04) Bug 99965 INGSRV1090.
**          Correct rux_us5 changes that were applied using the rmx_us5
**          label.
**	14-jun-2000 (hanje04)
**	    Added ibm_lnx to list of platforms resetting the signal mask
**          with sigemptyset, sigaddset, & sigprocmask.
**          #ifdef SIGSYS (doesn't exist on Linux).
**	21-jun-2000 (hanje04)
**	    Move #include <varargs.h> from end of include list to above 
**	    #include <ex.h>. On ibm_lnx va_start was being resolved from
**	    stdarg.h and not varargs.h, causing compile error.
**	21-dec-2000 (toumi01)
**	    Make the move of #include <varargs.h> conditional on ibm_lnx,
**	    which is the platform that needed the change.  Moving the
**	    include to the above location breaks Linux builds on Intel and
**	    on Alpha (Red Hat 7.0), and who knows where else.
**	27-Dec-2000 (jenjo02)
**	    i_EXtop() now returns **EX_CONTEXT rather than *EX_CONTEXT,
**	    i_EXpop() takes **EX_CONTEXT as parameter. For OS threads,
**	    this reduces the number of ME_tls_get() calls from 3 to 1.
**	30-Jan-2001 (hanje04)
**	    Added Alpha Linux to conditional positioning of varargs.h for
**	    S390 Linux.
**	21-Feb-2001 (hanje04)
**	    Removed extra '#include varargs.h" added by above Alpha Linux 
**	    change.
**	23-jul-2001 (stephenb)
**	    Add support for i64_aix
**	22-aug-2001 (toumi01)
**	    For i64_aix use xCL_068_USE_SIGACTION
**      08-Oct-2002 (hanje04)
**          As part of AMD x86-64 Linux port, replace individual Linux
**          defines with generic LNX define where apropriate.
**	08-Oct-2002 (hweho01)
**	    For rs4_us5, use xCL_068_USE_SIGACTION on AIX 5.1. 
**	01-Jul-2003 (wanfr01) 
**	    Bug 85386, INGSRV 25
**	    Enabled Alternate stack for dgi_us5 so large in clauses won't
**	    kill the server.
**      07-July-2003 (hanje04)
**          BUG 110542 - Linux Only
**          Enable use of alternate stack for SIGNAL handler in the event
**          of SIGSEGV SIGILL and SIGBUS. pthread libraries do not like
**          the alternate stack to be malloc'd so declare it on IICSdispatch
**          stack for INTERNAL THREADS and localy for each thread in CSMT_setup
**          for OS threads.
**      27-Oct-2003 (hanje04)
**          BUG 110542 - Linux Only - Ammendment
**          Minor changes to Linux altstack code to get it working on S390
**          Linux as well. Reset SIGMASK and SA_ONSTACK before returning from
**	    the SIGNAL handler.
**      14-Jan-2004 (hanje04)
**          Change functions using old, single argument va_start (in
**          varargs.h), to use newer (ANSI) 2 argument version (in stdargs.h).
**          This is because the older version is no longer implemented in
**          version 3.3.1 of GCC.
**	21-Sep-2004 (schka24)
**	    Don't catch sigsegv or sigbus if II_SEGV_COREDUMP is defined.
**	    This allows a simple, on-the-fly first cut at investigating
**	    front- or back-end segv's.
**      18-Apr-2005 (hanje04)
**          Add support for Max OS X (mac_osx)).
**          Based on initial changes by utlia01 and monda07.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/

/* Should be in stdlib.h, but easy / portable to define here: */

extern char *getenv();


# if defined(pyr_us5) || defined(sgi_us5)

/*
** Even though the default signal behavior is bad, we can adjust the
** signal return to the next instruction, so don't make these "hard" cases.
*/
# undef xCL_046_IDIV_HARD
# undef xCL_049_FDIV_HARD
# undef xCL_052_IOVF_HARD
# undef xCL_055_FOVF_HARD
# undef xCL_058_FUNF_HARD

# endif

/* On some machines special code can be run to turn an integer divide by zero
** into a recoverable signal event.
*/

# if defined(xCL_OVERRIDE_IDIV_HARD)
# undef xCL_046_IDIV_HARD
# endif

/*
** Remap exceptions from which we may not return to be "hard" cases.
*/
# ifndef xCL_011_USE_SIGVEC
#	if defined(xCL_046_IDIV_HARD) || defined(xCL_049_FDIV_HARD) \
		|| defined(xCL_052_IOVF_HARD) || defined(xCL_058_FUNF_HARD)
#		undef EXINTDIV
#		undef EXFLTDIV
#		undef EXINTOVF
#		undef EXFLTOVF
#		undef EXFLTUND
#		define EXINTDIV EXHINTDIV
#		define EXFLTDIV EXHFLTDIV
#		define EXINTOVF EXHINTOVF
#		define EXFLTOVF EXHFLTOVF
#		define EXFLTUND EXHFLTUND
#	endif
# else
#	ifdef xCL_046_IDIV_HARD
#		undef EXINTDIV
#		define EXINTDIV	EXHINTDIV
#	endif
#	ifdef xCL_049_FDIV_HARD
#		undef EXFLTDIV
#		define EXFLTDIV	EXHFLTDIV
#	endif
#	ifdef xCL_052_IOVF_HARD
#		undef EXINTOVF
#		define EXINTOVF	EXHINTOVF
#	endif
#	ifdef xCL_055_FOVF_HARD
#		undef EXFLTOVF
#		define EXFLTOVF	EXHFLTOVF
#	endif
#	ifdef xCL_058_FUNF_HARD
#		undef EXFLTUND
#		define EXFLTUND	EXHFLTUND
#	endif
# endif /* SIGVEC */

# if !defined(usl_us5) && !defined(ts2_us5)
# ifdef MAXSIG
# undef MAXSIG
# endif
# define MAXSIG		NSIG	/* must be >= highest signal number web
				** catch, + 1; be sure it's always big
				** enough!
				*/
# endif /* usl_us5 ts2_us5 */
 
void sgi_us5_adjust_pc(struct sigcontext *scp);
/*
** Keep track of what signal actions are already in place.
** If an exception occurs that we do not handle (no handler in
** place, or last handler returns EXRESIGNAL), take this action.
*/
TYPESIG (*orig_handler[ MAXSIG ])();

/*
** These are the signals we are interested in mapping to exceptions.
** You should catch ALL signals that would terminate the program except
** SIGIOT (SIGABORT).
**
** You should NOT catch signals whose SIG_DFL action is harmless.
**
** There had better be a case in i_EXcatch for each of these, and a 
** decode case in exsysrep.c.
*/ 
int sigs_to_catch[] = 
{
	SIGHUP,
	SIGINT,
	SIGQUIT,
	SIGILL,
# ifdef SIGDANGER
	SIGDANGER,
# endif
# ifdef SIGEMT
	SIGEMT,
# endif
	SIGALRM,
	SIGBUS,
	SIGFPE,
# ifdef SIGLOST
# if	SIGLOST != SIGABRT
	SIGLOST,
# endif
# endif
	SIGPIPE,
# ifdef SIGPROF
	SIGPROF,
# endif
# ifdef SIGPWR
	SIGPWR,
# endif
	SIGSEGV,
# ifdef SIGSYS
	SIGSYS,
# endif
# ifdef SIGSYSV
	SIGSYSV,
# endif
	SIGTERM,
# ifdef SIGTRAP
	SIGTRAP,
# endif
# ifdef SIGUSR1
	SIGUSR1,
# endif
# ifdef SIGUSR2
	SIGUSR2,
# endif
# ifdef SIGVPROF
	SIGVPROF,
# endif
# ifdef SIGVTALRM
	SIGVTALRM,
# endif
# ifdef SIGXCPU
	SIGXCPU,
# endif
# ifdef SIGXFSZ
	SIGXFSZ,
# endif
      -1		/* terminates the list */
};


/*
** Counts of "queued" signals while EXinterrupt is blocking them.
*/
extern i4	EXsigints;
extern i4	EXsigquits;

/*
** This is mainly for iifsgw.
*/
bool    IN_in_fsgw = FALSE;

/*
** Flags that we are already processing these signals, to avoid recursion
*/
static i4	Inhup = 0;
static i4	Inint = 0;
static i4	Inquit = 0;

/*
**  Set the default client type to be an user application.
*/
static i4	client_type = EX_USER_APPLICATION;
static bool	ex_initialized = FALSE;

/*
** Forward reference
*/

TYPESIG i_EXcatch();

#ifdef sparc_sol
static void su4_us5_adj_fpe(
    int                     signum,
    register siginfo_t      *code_ptr,
    register ucontext_t     *usr_ptr);
#endif

/*
**      Name:
**              EXsetsig()
**
**      Function:
**              sets up a handler for a signal, using eithe sigvec or signal
**              as appropriate.
**
**      Inputs:
**              signum                  The signal of interest
**              handler                 A pointer to function of TYPESIG
**                                      to be used as the new handler.
**
**
**      Returns:
**              A pointer to the  TYPESIG handler previously installed.
**
**      Side Effects:
**              Changes the process's signal state.
**
**	History:
**
**	16-may-90 (blaise)
**	    Integrated ingresug changes:
**		Added knowledge of sigaction (POSIX equivalent of sigvec).
**	05-mar-93 (mikem)
**	    su4_us5 port.  Added the SA_SIGINFO flag to the sigaction call
**	    so that on integer and floating point exceptions we can look at
**	    the siginfo_t structure to determine cause of the exception.  This
**	    is used by the su4_us5_adj_fpe() routine to handle integer overflow.
**	29-mar-93 (swm)
**	    Workaround Operating System bug on DEC OSF/1 V1.2 (Rev. 10)
**	    in which the exception code for integer arithmetic exceptions
**	    is not passed to a signal handler. The workaround is to get
**	    the required value from the sc_traparg_a0 element of the
**	    sigcontext structure.
**	    Also, made previous change, to add the SA_SIGINFO flag to the
**	    sigaction call, conditional on xCL_072_SIGINFO_H_EXISTS.
**	29-jun-95 (forky01)
**	    For cases when xCL_072_SIGINFO_H_EXISTS was not defined, the
**	    sigaction.sa_flags was left uninitialized and used whatever
**	    was on the stack at the time for the local defined structure.
**	    Add #else condition to init sa_flags=0.  Also change #ifdef to
**	    be based on existence of SA_SIGINFO.  Fixes (69583).
**	19-sep-95 (nick)
**	    Use exception stacks for various exception stacks - need to
**	    do this in conjunction with guard pages.
**      07-July-2003 (hanje04)
**          BUG 110542 - Linux Only
**          Enable use of alternate stack for SIGNAL handler in the event
**          of SIGSEGV SIGILL and SIGBUS. pthread libraries do not like
**          the alternate stack to be malloc'd so declare it on IICSdispatch
**          stack for INTERNAL THREADS and localy for each thread in CSMT_setup
**          for OS threads.
**      18-Apr-2005 (hanje04)
**          Add support for Max OS X (mac_osx)).
**          Based on initial changes by utlia01 and monda07.
**      13-jan-2004 (wanfr01)
**          Bug 64899, INGSRV87
**          Enabled Alternate stack for AIX so large in clauses won't
**          kill the server
*/

TYPESIG
(*EXsetsig( signum, handler ))()

int signum;
TYPESIG (*handler)();

{
# ifdef	xCL_011_USE_SIGVEC
    struct sigvec new, old;

    new.sv_handler = handler;
    new.sv_mask = new.sv_onstack = 0;

# ifdef SV_ONSTACK
#  if defined (xCL_077_BSD_MMAP) || defined (any_aix)
#   if defined(su4_u42) || defined (any_aix)
    if ((signum == SIGSEGV) || (signum == SIGBUS) || (signum == SIGILL))
	new.sv_onstack |= SV_ONSTACK;
#   endif /* su4_u42 */
#  endif /* ifdef xCL_077_BSD_MMAP */
# endif /* SV_ONSTACK */

    (void) sigvec( signum, &new, &old );
    return( old.sv_handler );
# else
# ifdef xCL_068_USE_SIGACTION
    struct sigaction new, old;

# if defined(thr_hpux) || defined(LNX) || defined(OSX)
    new.sa_sigaction = handler;
# else
    new.sa_handler = handler;
# endif /* hpb_us5 */
    sigemptyset(&(new.sa_mask));

    new.sa_flags = 0;

# ifdef SA_SIGINFO
    /*
    ** give us three args to the signal handler to provide context
    ** information ( we already expect three args for all signal suites )
    */
    new.sa_flags |= SA_SIGINFO;
# endif /* SA_SIGINFO */

# ifdef SA_ONSTACK
# if defined (xCL_077_BSD_MMAP) || defined(any_aix)
    /*
    ** catch some signals on an alternate stack
    */
#   if defined(sparc_sol) || defined(axp_osf) || defined(thr_hpux) || defined(dgi_us5) || \
        defined(any_aix)
    if ((signum == SIGSEGV) || (signum == SIGBUS) || (signum == SIGILL))
	new.sa_flags |= SA_ONSTACK;
#   elif defined(LNX) || defined(OSX)
    if ((signum == SIGSEGV) || (signum == SIGBUS) || (signum == SIGILL))
        new.sa_flags =  new.sa_flags | SA_ONSTACK | SA_RESTART | SA_NODEFER;
#   endif /* su4_us5 axp_osf */
#  endif /* ifdef xCL_077_BSD_MMAP */
# endif /* SA_ONSTACK */

#if defined(rux_us5)
    old.sa_handler=signal( signum, new.sa_handler );
#else
    (void) sigaction( signum, &new, &old );
#endif /* rux_us5*/
# if defined(thr_hpux) || defined(LNX) || defined(OSX)
    return( old.sa_sigaction );
# else
    return( old.sa_handler );
# endif /* hpb_us5 */
# else
    return(signal(signum,handler));
# endif	/* xCL_068_USE_SIGACTION */
# endif	/* xCL_011_USE_SIGVEC */
}

/*
**	Name:
**		EXestablish()
**
**	Function:
**		Set catches for signals from UNIX. This is called
**              from the i_EXsetup the first time that is called
**              out of the EXdeclare() macro.
**
**	Side Effects:
**		Changes for signals set up, saves the original handlers.
**
**	History:
**
**	16-may-90 (blaise)
**	    Integrated changes from ug:
**		Added function description above;
**		Set traps for all signals up to MAXSIG, rather than just the
**		first 16;
**		Remove references to dead variables i_EXigint and i_EXighup.
**       7-Oct-2003 (hanal04) Bug 110889 INGSRV2521
**          Moved the call to SIG_IGN (ignore) SIGINTs for EX_INGRES_DBMS
**          types from CS[MT]initiate().
**	21-Sep-2004 (schka24)
**	    Don't catch segv or sigbus if II_SEGV_COREDUMP defined.
*/

i4
i_EXestablish()
{
    char *segv_dump;	/* Don't-catch env var */
    i4	i;		/* array index */
    i4  s;              /* signal of interest */

    if (!ex_initialized)
    {
	ex_initialized = TRUE;
	if( client_type == EX_INGRES_DBMS)
	{
	    EXsetsig(SIGINT, SIG_IGN);
	}
    }

    /* See if don't-catch var is defined (env var only, not ing var) */
    segv_dump = getenv("II_SEGV_COREDUMP");

    /* loop through the array of signals we want to map to exceptions */

    for (i = 0; (s = sigs_to_catch[ i ]) > 0 ; i++)
    {
	/* Don't catch segv, bus error if trying to get coredump */
	if (segv_dump != NULL
	  && (s == SIGBUS || s == SIGSEGV))
	    continue;

	/*
	**  Since SIGBUS, SIGSEGV, SIGFPE ans SIGPIPE are
	**  always trapped by everyone, we don't have to deal with them
	**  specially below.
	*/
	if (client_type != EX_INGRES_DBMS)
	{
	    switch(s)
	    {
	    case SIGHUP:
	    case SIGINT:
	    case SIGQUIT:
	    case SIGTERM:
		/*
		**  If the client is a tool, then break out
		**  so it can be trapped.  Otherwise, skip it
		**  for user apps.
		*/
		if (client_type == EX_INGRES_TOOL)
		{
		    break;
		}
		else
		{
		    continue;
		}

	    case SIGILL:
# ifdef SIGDANGER
	    case SIGDANGER:
# endif
# ifdef SIGEMT
	    case SIGEMT:
# endif
	    case SIGALRM:
# ifdef SIGLOST
	    case SIGLOST:
# endif
# ifdef SIGPROF
	    case SIGPROF:
# endif
# ifdef SIGPWR
	    case SIGPWR:
# endif
# ifdef SIGSYS
	    case SIGSYS:
# endif
# ifdef SIGSYSV
	    case SIGSYSV:
# endif
# ifdef SIGTRAP
	    case SIGTRAP:
# endif
# ifdef SIGUSR1
	    case SIGUSR1:
# endif
# ifdef SIGUSR2
	    case SIGUSR2:
# endif
# ifdef SIGVPROF
	    case SIGVPROF:
# endif
# ifdef SIGVTALRM
	    case SIGVTALRM:
# endif
# ifdef SIGXCPU
	    case SIGXCPU:
# endif
# ifdef SIGXFSZ
	    case SIGXFSZ:
# endif
		continue;
	    }
	}
# if defined(sgi_us5)
    /*
    **  This little snippet of code convinces IRIX 6.x
    **  to start trapping SIGFPEs.
    */
    if ( s == SIGFPE )
    {
        int fpc_csr_prev;
        union fpc_csr flp;
        flp.fc_word = get_fpc_csr();
        flp.fc_struct.en_invalid = 0;
        flp.fc_struct.en_divide0 = 1;
        flp.fc_struct.en_overflow = 1;
        flp.fc_struct.en_underflow = 1;
        flp.fc_struct.en_inexact = 0;
        fpc_csr_prev = set_fpc_csr(flp.fc_word) ;
    }
# endif /* sgi_us5 */


        if ((orig_handler[s] = EXsetsig(s, i_EXcatch)) == SIG_IGN)
        {
            /* If we were ignoring it, continue to do so for these.
            ** This is so background jobs don't suddenly start to
            ** pickup keyboard generated signals that were nohupped.
            */
            switch (s)
            {
            case SIGINT:
            case SIGHUP:
            case SIGQUIT:
#if defined(rux_us5)
            case SIGVTALRM:
#endif
                (void) EXsetsig(s, SIG_IGN);
            }
        }
    }
    return( s );        /* we aren't void, so return something */
}

/*
**      Name:
**              i_EXsetsig
**
**      Function:
**              Installs the EX signal handler i_EXcatch() for a signal.
**
**      Inputs:
**              signum                  The signal of interest.
**
**      Returns:
**              none.
**
**      Side Effects:
**              Changes the process's signal state, saves the old handler
**              in the global orig_handler[] table.
**
**	History:
**
**	16-may-90 (blaise)
**	    Integrated changes from ug:
**		Added function description above.
*/

i_EXsetsig(signum)
int	signum;
{
    orig_handler[signum] = EXsetsig(signum, i_EXcatch);
}

/*
**      Name:
**              i_EXsetothersig
**
**      Function:
**              Installs a signal handler for a signal, saving previous.
**              Used internally by the CL.
**
**      Inputs:
**              signum                  The signal of interest.
**              func                    the new handler to use.
**
**      Returns:
**              none.
**
**      Side Effects:
**              Changes the process's signal state, saves the old handler
**              in the global orig_handler[] table.
**
**	16-may-90 (blaise)
**	    Integrated changes from ug:
**		Added function description above.
*/

i_EXsetothersig(signum, func)
int	signum;
TYPESIG	(*func)();
{
    orig_handler[signum] = EXsetsig(signum, func);
}

/*
**      Name:
**              i_EXrestoreorigsig
**
**      Function:
**		Restores the original signal handler for a signal from
**		the orig_handler arrray.
**              Used internally by the CL.
**
**      Inputs:
**              signum                  The signal of interest.
**
**      Returns:
**              none.
**
**      Side Effects:
**              Changes the process's signal state.
**
**	26-jul-1993 (rog)
**	    Created.
*/

i_EXrestoreorigsig(signum)
int	signum;
{
    (void) EXsetsig(signum, orig_handler[signum]);
}

/*
**      Name:
**              i_EXcatch()
**
**      Function:
**              The function installed by EX to handle the UNIX signals of
**              interest contained in sigs_to_catch[].  Signals mentioned
**              here, but not in sigs_to_catch will never be caught.
**
**      Inputs:
**              signum                  The signal of interest
**              code                    The sigvec code
**              scp                     the sigvec context pointer
**
**              Code and scp will not exist on non-sigvec systems.
**
**      Returns:
**              none.
**
**      Side Effects:
**              Drives the exception system, or leaves flags for
**		interrupts and quits of interest to EXinterrupt.
**
**	16-may-90 (blaise)
**	    Integrated changes from ug:
**		Added function description above;
**		Simplified function by passing three arguments for all signal-
**		generated exceptions;
**		Added global variable IN_in_fsgw for special iifsgw case;
**		Added cases for new signals: for example SIGXCPU, SIGXFSZ,
**		SIGUSR1, SIGUSR2, SIGSYSV;
**		Added new FPE codes: for example FPE_FLTOVF_EXC,
**		FPE_INVALID_EXC, FPE_INEXACT_EXC;
**		Added machine-specific code for pyr, hp8, rtp, ps2.
**	4-sep-90 (jkb)
**	   restore original signal mask for sqs_ptx
**	4-oct-90 (jkb)
**	   restore original signal mask for sqs_ptx correctly, add code to
**	   clear floating point stack after exception
**      8-oct-90 (chsieh)
**   		jkb's change same as ours. Added ifdef bu2_us5 or bu3_us5 
**		to sqs_ptx to handle the cases where  UNIX has sigaction, 
**		but the signal handler is not BSD like.
**              The process context is not passed as argument to the handler.
**            	The previous interrupt mask is not accessable. 
**              However, all it tries to do is re-enable that "signum"  
**		interrupt.
**	22-sep-93 (smc)
**	    Changed cast of scp in EXsignal call to be portable PTR.
**	10-feb-94 (ajc)
**	    Added hp8_bls based on hp8*
**	23-jan-1996 (sweeney)
**	    Tab {} delimited code block in to avoid confusing ctags,
**	    which otherwise thinks the } on an empty line is the end
**	    of the function i_EXcatch().
**	12-nov-96 (kinpa04)
**		For axp_osf, add si_code to place code value gotten from scp.
**		code should be siginfo but in some cases it is null which
**		necessitates the use of the code from scp.
**	02-jul-1998 (kosma01 for fucch01)
**		For sgi_us5, if POSIX is defined then SIGTRAP is not defined.
**		Since sgi_us5 is a POSIX thread platform...
**	06-oct-1999 (toumi01)
**	    Change Linux config string from lnx_us5 to int_lnx.
**      07-Sep-2000 (hanje04)
**	    Add axp_lnx (Alpha Linux) to list of platforms resetting the
**	    signal mask with sigemptyset, sigaddset, & sigprocmask.
**	    #ifdef SIGSYS (doesn't exist on Linux).
**	12-Sep-2000 (hanje04)
**	    Moved '#include varargs.h' above si.h so that 'va_start' is 
**	    correctly from stdargs.h and not varargs.h on Alpha Linux.
**	04-Dec-2001 (hanje04)
**	    Add support for IA64 Linux (i64_lnx)
**      07-July-2003 (hanje04)
**          BUG 110542 - Linux Only
**          Enable use of alternate stack for SIGNAL handler in the event
**          of SIGSEGV SIGILL and SIGBUS. pthread libraries do not like
**          the alternate stack to be malloc'd so declare it on IICSdispatch
**          stack for INTERNAL THREADS and localy for each thread in CSMT_setup
**          for OS threads.
**      18-Apr-2005 (hanje04)
**          Add support for Max OS X (mac_osx)).
**          Based on initial changes by utlia01 and monda07.
**      29-Sep-2005 (hweho01)
**          Correct the typo error caused by the previous change.     
*/

/*ARGSUSED*/
TYPESIG
i_EXcatch(signum, code, scp)

int signum;
EX_SIGCODE SIGCODE(code);
EX_SIGCONTEXT SIGCONTEXT(scp);
{
    EX e;
# ifdef axp_osf
	long	si_code;
# endif

# ifdef xCL_011_USE_SIGVEC
    /*
    ** Must restore the original signal mask.  The kernel blocked
    ** the current signal on entry to the handler; but if we _longjmp
    ** out of the handler, the kernel will not get a chance to restore
    ** the original mask for us.
    */

# if defined(ris_u64)
    /* sc_mask is a sigset_t, which is a structure of two groups of 32-bit
    ** integers for AIX 4.1.  we just need the lower 32 for this function.
    */
    (void)sigsetmask(scp->sc_mask.losigs);
# else
    (void)sigsetmask(scp->sc_mask);
# endif
# else
# if defined(xCL_068_USE_SIGACTION)
     /*
     ** restore original signal mask (see above comment for sigvec)
      */
# if defined(sqs_ptx) || defined(axp_osf) || defined(sos_us5) || \
     defined(any_hpux) || defined(LNX) || defined(any_aix) || \
     defined (dgi_us5) || defined(OSX)
    {
         sigset_t mask;

         sigemptyset(&mask);
         sigaddset(&mask,signum);
         sigprocmask(SIG_UNBLOCK, &mask, (sigset_t *)0);
    }
# else
     sigprocmask(SIG_SETMASK, &(scp->uc_sigmask), (sigset_t *)0);
# endif /* sqs_ptx */


# else

#if !defined(sos_us5) && !defined(sgi_us5)
    /* Fakes for passing to exception handlers */
    int code;
    int scp;
#endif

    /* Notorious race condition here! If same signal happens before
       we replant ourselves, we can get blown out of the water.*/
    (void) EXsetsig( signum, i_EXcatch );

    /* provide some value to exception handler */
# if !defined(sgi_us5)
    code = scp = signum;
# endif
# endif /* xCL_068_USE_SIGACTION */
# endif	/* xCL_011_USE_SIGVEC */

# ifdef axp_osf
/*
**	    Workaround Operating System bug on DEC OSF/1 V1.2 (Rev. 10)
**	    in which the exception code for integer arithmetic exceptions
**	    is not passed to a signal handler. The workaround is to get
**	    the required value from the sc_traparg_a0 element of the
**	    sigcontext structure.
*/
    si_code = scp->sc_traparg_a0;
# endif /* axp_osf */

    switch (signum)
    {
      case SIGHUP:
	if (Inhup)		/* avoid recursion */
	{
		return;
	}
	e = EXHANGUP;
	break;

      case SIGINT:
	/*
	** If SIGINT and SIGQUIT and keyboard interrupts are currently
	** masked (EXintr_count > 0), just count the signal.
	** The exception will be raised when keyboard interrupts
	** are unmasked with EX_ON, EX_DELIVER or EX_RESET.
	*/
	if ( EXintr_count > 0 )		/* log ints while blocked */
	{
	    EXsigints++;
	    return;
	}
	else if( Inint )		/* avoid recursion */
	{
	    return;
	}
	e = EXINTR;
	break;

      case SIGQUIT:
	if ( EXintr_count > 0 )		/* log quits while blocked */
	{
	    EXsigquits++;
	    return;
	}
	else if( Inquit )		/* avoid recursion */
	{
	    return;
	}
	e = EXQUIT;
	break;


/*
** We don't map anything to EXRESVOP or EXBUSERR, because many mainline
** handlers only call EXsysreport when the exception is EXSEGVIO.  So
** We dump all the fatal signals there instead.  Were we really going
** to do something different for these exceptions than for a SEGV?
**      e = EXRESVOP;
**      break;
**      e = EXBUSERR;
**      break;
*/

      case SIGFPE:

	/*
        ** On machines where possible and needed, adjust the signal return
        ** pc so we don't land on the same instruction and loop.  If
        ** you can do this, be sure not to turn on the "hard" versions
        ** of the exceptions.
        */

# if defined(rmx_us5) || defined(rux_us5)
        /* scp->uc_mcontext.mc_pc += 4; */
        scp->uc_mcontext.gpregs[CXT_EPC] += 4;
        e = EXINTDIV;
# endif /* rmx_us5 */


# if defined(pyr_us5)
        adjust_pc(scp);
# endif

# if defined(sgi_us5)
	sgi_us5_adjust_pc(scp);
# endif

# if defined(sparc_sol)
	su4_us5_adj_fpe(signum, code, scp);
# endif

# if defined(hp8_bls)
        adjust_fpe(signum, code, scp);
# define FPE_INTDIV_EXC 13   /* Should be defined for hp but isn't */
# endif

# ifdef	xCL_011_USE_SIGVEC
	switch (code)
	{
# ifdef FPE_INTDIV_TRAP
	case FPE_INTDIV_TRAP:
	    e = EXINTDIV;
	    break;
# endif
# ifdef FPE_INTDIV_EXC
        case FPE_INTDIV_EXC:
            e = EXINTDIV;
            break;
# endif
# ifdef FPE_INTOVF_TRAP
	case FPE_INTOVF_TRAP:
	    e = EXINTOVF;
	    break;
# endif
# ifdef FPE_INTOVF_EXC
        case FPE_INTOVF_EXC:
            e = EXINTOVF;
            break;
# endif
# ifdef FPE_FLTDIV_TRAP
	case FPE_FLTDIV_TRAP:
	    e = EXFLTDIV;
	    break;
# endif
# ifdef FPE_FLTDIV_EXC
        case FPE_FLTDIV_EXC:
            e = EXFLTDIV;
            break;
# endif
# ifdef FPE_FLTUND_TRAP
	case FPE_FLTUND_TRAP:
	    e = EXFLTUND;
	    break;
# endif
# ifdef FPE_FLTUND_EXC
        case FPE_FLTUND_EXC:
            e = EXFLTUND;
            break;
# endif
	/* These default to EXFLTOVF */
# ifdef FPE_FLTOVF_TRAP
	case FPE_FLTOVF_TRAP:
# endif
# ifdef FPE_FLTOVF_EXC
        case FPE_FLTOVF_EXC:
# endif
# ifdef FPE_FLTNAN_TRAP
	case FPE_FLTNAN_TRAP:
# endif
# ifdef FPE_FLTINVALID_EXC
        case FPE_FLTINVALID_EXC:
# endif
# ifdef FPE_FLTINEXACT_EXC
        case FPE_FLTINEXACT_EXC:
# endif
	default:
	    e = EXFLTOVF;
	}

# else	/* xCL_011_USE_SIGVEC */

	e = EXFLTOVF;		/* Should be EXFLOAT, but mainline
                                   handlers don't know about that one. */
# endif	/* xCL_011_USE_SIGVEC */
	break;

# ifdef SIGTRAP
      case SIGTRAP:
	    e = EXTRACE;
# if defined(rmx_us5) || defined(rux_us5)
        /* scp->uc_mcontext.mc_pc += 4; */
        scp->uc_mcontext.gpregs[CXT_EPC] += 4;
        e = EXINTDIV;
# endif /* rmx_us5 */

# ifdef ds3_ulx
	    scp->sc_pc += 4;  /* advance the pc to next instruction
				 (integer division takes 4 bytes.) */
	    e = EXINTDIV;
# endif
# ifdef axp_osf
            e = EXINTDIV;
# endif
	break;

# endif /* SIGTRAP */
      /*
      ** Map any fatal but otherwise uninteresting signals to EXSEGVIO,
      ** so naive mainline code that only calls EXsysreport on that exception
      ** will pick it up and log a meaningful message.
      */

      case SIGBUS:
# ifdef SIGDANGER
      /*
      ** Sigdanger is not fatal on most systems. If it is so on yours,
      ** ifdef it for your box. Else, we ignore it.
      */
# endif
# ifdef SIGEMT
      case SIGEMT:
# endif
      case SIGILL:
# ifndef ris_us5
# ifdef SIGLOST
      case SIGLOST:
# endif
# endif /* ris_us5 */
# ifdef SIGPROF
      case SIGPROF:
# endif
# ifdef SIGPWR
        case SIGPWR:
# endif
      case SIGSEGV:
# ifdef SIGSYS
      case SIGSYS:
# endif
# ifdef SIGSYSV
        case SIGSYSV:
# endif
# ifdef SIGUSR1
      case SIGUSR1:
# endif
# ifdef SIGUSR2
      case SIGUSR2:
# endif
# ifdef SIGVTALRM
      case SIGVTALRM:
# endif
# ifdef SIGXCPU
      case SIGXCPU:
# endif
# ifdef SIGXFSZ
      case SIGXFSZ:
# endif
	e = EXSEGVIO;
	break;

      case SIGALRM:
        e = EXTIMEOUT;
	break;

      case SIGPIPE:
	if (IN_in_fsgw)
            exit(0);
        e = EXCOMMFAIL;
	break;

      case SIGTERM:
	if (IN_in_fsgw)
            e = EXBREAKQUERY;
        else
            e = EXKILL;
	break;

      default:  /* not a signal we handle */
        return;
    }

    /* EXsignal and EXsys_report count on there being 3 args if this is
       an exception raised by a signal.  This distinguishes them from ones
       raised by software, which will have no arguments */

# if defined(axp_osf)
    EXsignal( e, 3, (long)signum, (long)si_code, (PTR)scp );
# else
    EXsignal( e, 3, (long)signum, (long)code, (PTR)scp );
# endif
}

/*
** Function:	EXsignal()
**
** Inputs:
**              ex              exception to raise
**              N               number of arguments following.
**              args ...        variable number of arguments to the exception.
**
** Example:
**	EXsignal(ex, N, arg1, arg2, ... ,argN)
**		EX	ex;
**		i4	N;
**		i4	arg1,arg2,...argN;
**
**		Raises the exception ex with the N arguments arg1..argN.
**
** Returns:
**	Nothing.
**
** History
**	6-Jan-1989 (daveb)
**		Add gates to prevent recursive interrupts and quits.
**		Remove ancient irrelevant history.
**	16-may-90 (blaise)
**	    Integrated changes from ug:
**		Removed the variable parent, which didn't do anything;
**		Check whether it was a signal before driving default signal
**		handler.
**	23-Jul-1993 (daveb)
**		handler return is STATUS, not EX, or i4.	    
**      22-Sep-1993 (smc)
**	 	Bug #56446
**		Altered special three parameter 'system exception'
**              case to place data into extended portable EX_ARGS.
**	19-sep-1995 (nick)
**		Deal with exception stacks and restore state.
**      14-Mch-1996 (prida01)
**              Add dump on fatal call to diagnostics
**	27-Dec-2000 (jenjo02)
**	    i_EXtop() now returns **EX_CONTEXT rather than *EX_CONTEXT,
**	    i_EXpop() takes **EX_CONTEXT as parameter. For OS threads,
**	    this reduces the number of ME_tls_get() calls from 3 to 1.
**	03-Oct-2005 (toumi01) BUG 115098
**	  Avoid trying to run non-existent exception handlers. A problem
**	  showed up with this running an esql/c program with a bad value
**	  for II_SYSTEM.
**	27-Oct-2005 (hanje04)
**	    BUG 114921
**	    As SIGHUP isn't always fatal, reset Inhup when we're done.
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
*/


# define 	ERR_EXIT	-1

/*VARARGS2*/
VOID
EXsignal(EX ex, i4 N, ...)
{
	register long	*arp;

	EX_CONTEXT	*env,
			*exc,
			**excp;
	long		argarr[EXMAXARG];
	EX_ARGS		exarg;
	char		errmsg[ER_MAX_LEN];
	STATUS		handle_ret;
	i4		sig;

	va_list		args;

	exarg.exarg_count = N;
	exarg.exarg_num   = ex;
	exarg.exarg_array = argarr;

	/* Don't allow recursive interrupts.  It's OK if an Inxxx
	** variable goes > 1, since we will forcibly set it to zero before
	** we are done processing the one that matters.  We must be sure
	** to reset them, or we will lock out subsequent interrupts!
	*/
	if( (ex == EXINTR && Inint++) || (ex == EXQUIT && Inquit++) ||
	    (ex == EXHANGUP && Inhup++) )
	    return;

	/*
	** We first collect up the arguments into the standard
	** exception argument stack.  This is declared locally
	** because exceptions can be signaled during an exception.
	** 3 args is a special case indicating a system exception
	** has occured, the data is then written into additional 
	** elements of the EX_ARGS structure
	*/

	arp = argarr;
	va_start( args, N );
	if ( N < 3 )
		while( N-- )
			*arp++ = va_arg( args, long );
	else
	{
		exarg.signal = va_arg( args, long );
		exarg.code = va_arg( args, long );
		exarg.scp = va_arg( args, PTR );
	}
	va_end( args );

	/*
	** We are now ready to begin running handlers.
	** This loop does not pop exceptions off the stack.
	*/

	excp = i_EXtop();
	env = excp ? *(excp) : NULL;
	for (; env; env = i_EXnext(env))
	{
		if (env->handler_address == 0)
			continue;

		handle_ret = (*(env->handler_address))(&exarg);

		switch (handle_ret)
		{
		  case EXRESIGNAL:
			/* run higher level handlers */
			continue;

		  case EXDECLARE:
			if (ex != EX_PSF_LJMP)
				EXdump(EV_DUMP_ON_FATAL,0);
			/* long jump back to the declaration ("unwind") */
			break;

		  case EXCONTINUES:
			/* return running no more handlers */
			Inint = Inquit = Inhup = 0;
			return;

		  default:
			SIfprintf(stderr,
				  "invalid handler return--%x\n",
				  handle_ret);

			PCexit(ERR_EXIT);
		}

		break;
	}

	/*
	** If no handler and it is EXEXIT, bail out immediately with
	** status 0.  This doesn't call PCexit to run PCatexit queued
	** routines.  Maybe it should.
	**
	** If no handler and it is a UNIX signal, reinstall the user's
	** original handler and resignal.  Exit if it returns.
	*/
	if (!env)
	{
	    /*
	    ** No more handlers.  Take the original handler's action, if any.
	    */
	    switch( ex )
	    {
	      case EXEXIT:
			exit( 0 );	/* Not PCexit? */
			break;
	      case EXHANGUP:
	      case EXINTR:
	      case EXQUIT:
	      case EXRESVOP:
	      case EXBUSERR:
	      case EXINTDIV:
	      case EXFLTDIV:
	      case EXFLTOVF:
	      case EXSEGVIO:
	      case EXTIMEOUT:
	      case EXCOMMFAIL:
	      case EXUNKNOWN:
	      case EXBREAKQUERY:
	      case EXCHLDDIED:

		if( exarg.exarg_count == 3 )
		{
		    /*
		    ** This exception was generated by a UNIX signal.
		    ** The single argument is the signal number that
		    ** caused the exception.
		    */
		    sig = exarg.signal;
		    (void) EXsetsig( sig, orig_handler[ sig ] );
		    (void) kill( getpid(), sig );
		    (void) EXsetsig( sig, i_EXcatch );
		    break;
		}
		/* else fall through ... */

	      default:
		EXsignal( EXEXIT, 0, 0 );
		abort();	/* "Can't get here" */
	    }
	    Inint = Inquit = Inhup = 0;
	    return;
	}

	/*
	** If we get here then an unwind is in progress.
	**
	** We have to step down the runtime stack popping
	** each scope we encounter.  Before the scope is popped
	** we must call any exception bound to the stack
	**
	** Each exception on the except stack must be <= to
	** current runtime scope because of the loop
	** done at the start of the routine.
	*/
	exarg.exarg_num   = EX_UNWIND;
	exarg.exarg_count = 0;

	for (exc = *(excp = i_EXtop()); env != exc ; exc = i_EXnext(exc))
	{
		if ( exc->handler_address )
		{
		    (void) (*(exc->handler_address))(&exarg);

		    i_EXpop(excp);
		}
	}

	/*
	** The handlers have been run.  Time to do the return to
	** the appropriate scope.
	*/
	Inint = Inquit = 0;
#if defined sqs_ptx
        /*
        ** This is unforunately necessary to pop values that cause FPE's
        ** off the 387 co-processor stack. Without this FPE's will collect
        ** on the stack causing it to overflow.
       */
        asm("fclex");
        asm("ffree %st(0)");
        asm("ffree %st(1)");
        asm("ffree %st(2)");
        asm("ffree %st(3)");
        asm("ffree %st(4)");
        asm("ffree %st(5)");
        asm("ffree %st(6)");
        asm("ffree %st(7)");
#endif /* sqs_ptx */
# ifdef xCL_077_BSD_MMAP
# if defined(sparc_sol) || defined(thr_hpux)
	{
	    ucontext_t excontext;

	    /* try to reset the onstack status */
	    if (getcontext(&excontext) == 0)
		if (excontext.uc_stack.ss_flags & SA_ONSTACK)
		{
		    excontext.uc_stack.ss_flags &= ~SA_ONSTACK;
		    (void) setcontext(&excontext);
		}
	}
# endif /* su4_us5 */
# if defined(LNX)
	{
	    stack_t exstack;

	    if ( sigaltstack( (stack_t *)0, &exstack) == 0 )
	    {
	        if ( exstack.ss_flags & SA_ONSTACK )
	        {
		    sigset_t mask;
		
		    sigemptyset(&mask);
		    sigprocmask(SIG_UNBLOCK, &mask, (sigset_t *)0);
		    exstack.ss_flags &= ~SA_ONSTACK;
		    sigaltstack( &exstack, (stack_t *)0 );
	        }
	    }
	}
# endif
# if defined(su4_u42) || defined(axp_osf)
	{
	    struct sigstack exstack;

# if defined(axp_osf)
	    if (sigstack(&exstack, &exstack) == 0)
# else
	    if (sigstack((struct sigstack *)0, &exstack) == 0)
# endif
		if (exstack.ss_onstack)
		{
# if defined(axp_osf)
         sigset_t mask;

         sigemptyset(&mask);
         sigprocmask(SIG_UNBLOCK, &mask, (sigset_t *)0);
# endif
		    exstack.ss_onstack = 0;
		    (void) sigstack(&exstack, (struct sigstack *)0);
		}
	}
# endif /* su4_u42 axp_osf */
# endif /* ifdef xCL_077_BSD_MMAP */
	i_EXreturn(env, EXDECLARE);

	/* "should never get here" -- i_EXreturn shouldn't return */
	return;
}



/*{
** Name: adjust_pc()      - Adjust the signal return pc
**
** Description:
**      Adjusts the signal return pc to point to the instruction following
**	the one that caused the signal.  This function is necessary to
**	prevent	infinite loops caused by exception signal handler returns
**	that assume the handler will correct the cause of the exception
**	and the instruction should be retried.
**
** Outputs:
**      scp			Pointer to the signal context.
**	    .pc			Address of instruction after instruction
**				causing exception.
**
**      Returns:
**	    none
**
**      Exceptions:
**          none
**
** Side Effects:
**	Modifying the pc within the signal context may cause the signal
**	handler to return to a different instruction than it otherwise
**	might.
**
** History:
**	16-may-90 (blaise)
**	    Added this function as part of the integration of ingresug changes
**	    (function was created 12-jun-89 by arana)
*/

# if defined(pyr_us5)
# include <systypes.h>
# include <disasm.h>

VOID
adjust_pc(scp)
struct sigcontext *scp;
{
    char buf[MINDISZ];

    /* disasm is a Pyramid library routine */
    scp->sc_pc = disasm((unsigned long)scp->sc_pc, buf);
}

/* readinst -	Called by disasm.
**		Returns the word at the address pointed to by addr.
*/		
unsigned long
readinst(addr)
unsigned long *addr;
{
    return(*addr);
}

/* constout -	Called by disasm.
**		Converts a numeric value to a character string, placing
**		it at the location pointed to by cp, and returning a
**		pointer to the byte following the character string.
*/		
char *
constout(val, cp)
unsigned long	val;
char		*cp;
{
    /* Don't need to convert the numeric value, so just return the pointer. */
    return(cp);
}

/* dispout -	Called by disasm.
**		Same as constout except the numeric value denotes a
**		displacement.
*/		
char *
dispout(val, cp)
unsigned long	val;
char		*cp;
{
    /* Don't need to convert the numeric value, so just return the pointer. */
    return(cp);
}
# endif

# if defined(sgi_us5)
/* 
**  sgi_us4_adjust_pc adjust the program count so that we can return without
**  repeating the trap.
*/
void
sgi_us5_adjust_pc(struct sigcontext *scp)
{
/*  Is the exception instruction in a branch delay slot ? */
    if ( !(scp->sc_cause & SC_CAUSE_BD) ) 
      /*
      ** It is not in the branch delay slot, 
      ** the next executable instruction is 4 bytes ahead 
      */
      scp->sc_pc += 4;
    else
      /* It is in the branch delay slot 
      ** the branch instruction points to the next  
      ** executable instruction. */
      {
         scp->sc_pc += 4;
      }
}
# endif /* sgi_us5 sgi_adjust_pc */

# if defined(hp8_bls)
adjust_fpe(sig, code, scp)
/******************************************************************
adjust_fpe fixes the float pt unit so we can return without repeating the trap
   (In this case, we clear the "T-bit" of the floating pt unit)
**************************************************************************/
/* Note: when code == 13, then the trap was generated by an integer overflow*/
/*       or by an integer divide by zero.                                   */
    int sig, code;
    struct sigcontext *scp;
    {

    /* if this is actually an integer trap, then skip the instruction*/
    if (code == 13)
        scp->sc_psw |= 0x200000;  /* set the "nullify" bit */
    
    /* otherwise, it is a floating pt exception.  Clear the T-bit */
    else
        scp->sc_sl.sl_ss.ss_frstat &= ~FP_TBIT;
    }
# endif /* hp8_bls */

#ifdef sparc_sol
/*{
** Name: su4_us5_adj_fpe()   - recover gracefully from integer divide by 0.
**
** Description:
**	Under solaris 2.1 without this change an integer divide by zero will
**      not advance the PC after the signal is handled.  Under previous sun
**	releases this was done.  This code restores the previous behavior (ie.
**	an exception is raised, the pc is advanced past the instruction, and
**	0 is left as the result).  This behavior is the same as the default
**	floating point over/underflow behavior.
**
**	With this change applications on sun that depended on queries which
**	generate integer divide by zero resulting in 0 (run_all being one such
**	application), should continue to "work" in the same way across releases.
**
**	This function only handles FPE_INTDIV as all other SIGFPE signals are 
**	handled as expected automatically by the system as part of the default.
**
** Inputs:
**	signum				signal number
**	code_ptr			pointer to info to decode sig meaning.
**	usr_ptr				pointer to user context
**
** Outputs:
**	none.
**
**      Returns:
**	    void.
**
** History:
**      01-mar-93 (mikem)
**          Created.
*/
static void
su4_us5_adj_fpe(
int			signum,
register siginfo_t	*code_ptr,
register ucontext_t	*usr_ptr)
{
    register int	*rp;

    if (code_ptr->si_code == FPE_INTDIV)
    {
	rp = usr_ptr->uc_mcontext.gregs;

	/* set result to be 0 */
	rp[REG_O0] = 0;

	/* bump the PC by 4 to get past instruction which caused exception */
	rp[REG_PC] = rp[REG_nPC];
	rp[REG_nPC] += 4;
    }
}
#endif


/*{
** Name:	EXsetclient - Set client information for the EX subsystem
**
** Description:
**	This routine informs the EX subsystem as to the type of client
**	that it is dealing with.  The client type information is
**	important in that it allows EX to modify its behavior dependent
**	on the client
**
**	For user applications, EX will only intercept access violation and
**	floating point exceptions.  This becomes the default behavior for EX.
**	It is up to the user application to deal with any other exceptions.
**
**	If the client is the Ingres DBMS, then EX will need to intercept
**	any exceptions that will cause a process to exit (current behavior).
**
**	In the case of Ingres Tools, EX will also intercept user generated
**	exit exceptions (such as interrupt) in addition to the access
**	violation and floating point exceptions.
**
** Inputs:
**	EX_INGRES_DBMS		Client is the Ingres DBMS or any application
**				that desires the trap all behavior.
**
**	EX_INGRES_TOOL		Client is one of the Ingres Tools.
**
**	EX_USER_APPLICATION	Client is a user written application.  This
**				is actually unnecessary since this is the
**				default.  It is defined here just for
**				completeness.
**
** Outputs:
**
**	Returns:
**		EX_OK			If everything succeeded.
**		EXSETCLIENT_LATE	If the EX subsystem has already
**					been initialized when this is called.
**		EXSETCLIENT_BADCLEINT	If an unknown client was passed in.
**
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	21-mar-94 (dave) - Initial version.
**	25-mar-94 (dave) - Added error return status if the subsystem
**			  has already been initialized.
*/
STATUS
EXsetclient(i4 client)
{
	if (!ex_initialized)
	{
	    switch (client)
	    {
	      case EX_INGRES_DBMS:
	      case EX_INGRES_TOOL:
	      case EX_USER_APPLICATION:
	        client_type = client;
		break;

	      default:
		return(EXSETCLIENT_BADCLIENT);
		break;
	    }
	}
	else
	{
	    return(EXSETCLIENT_LATE);
	}
	return(EX_OK1);
}

/*{
** Name: EXalt_stack()      - Define alternate stack to kernel
**
** Description:
**
** Inputs:
**	ex_stack	- stack address
**	s_size		- stack size
**	
**	
** Outputs:
**
**      Returns:
**	    none
**
**      Exceptions:
**          none
**
** History:
**	19-sep-95 (nick)
**	    Created.
**      07-July-2003 (hanje04)
**          BUG 110542 - Linux Only
**          Enable use of alternate stack for SIGNAL handler in the event
**          of SIGSEGV SIGILL and SIGBUS. pthread libraries do not like
**          the alternate stack to be malloc'd so declare it on IICSdispatch
**          stack for INTERNAL THREADS and localy for each thread in CSMT_setup
**          for OS threads.
**      18-Apr-2005 (hanje04)
**          Add support for Max OS X (mac_osx)).
**          Based on initial changes by utlia01 and monda07.
**      13-jan-2004 (wanfr01)
**          Bug 64899, INGSRV87
**          Enabled Alternate stack for AIX so large in clauses won't
**          kill the server
*/
STATUS
EXaltstack(PTR ex_stack, i4  s_size)
{
# if defined(sparc_sol) || defined(thr_hpux) || defined(axp_osf) || \
     defined(LNX) || defined (dgi_us5) || defined(OSX) || \
     defined(any_aix)
    stack_t nexstack;

    nexstack.ss_size = s_size;
    nexstack.ss_flags = 0;
# if defined(axp_osf)
    nexstack.ss_sp = ex_stack + s_size - 8; /* stack grows down */
# else 
    nexstack.ss_sp = ex_stack;
# endif /* axp_osf */

    if (sigaltstack(&nexstack, (stack_t *)0) < 0)
	return(FAIL);
# endif /* su4_us5 */
# if defined(su4_u42) 
    struct sigstack nexstack;
    int ststat;

    /* 
    **  stack grows down on su4_u42 and sigstack() requires the 
    **  address at the end of the alloc'd area.  
    */
    nexstack.ss_sp = ex_stack + s_size - 8;
    nexstack.ss_onstack = 0;

    if (sigstack(&nexstack, (struct sigstack *)0) < 0)
	return(FAIL);
# endif /* su4_u42 */
    return(OK);
}
