/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** CLSIGS.H - include UNIX signal definitions.
**
** Not done directly to hide the SIGCLD/SIGCHLD difference.
**
** History:
**	27-Oct-1988 (daveb)
**	    Created.
**	17-may-90 (blaise)
**	    Integrated changes from 61 and ug:
**		Force an error if <clconfig.h> is not included;
**		Add CLSIGS_H_INCLUDED 'gate' to prevent multiple inclusion of
**		this file;
**		Add <systypes.h> for dr6_us5;
**		Include <ucontext.h> and <siginfo.h> if they exist, for POSIX
**		signal handlers;
**		Add macros to determine how the signal parameters are defined
**		(reference or value);
**		Add special case to enable dg8_us5 to use these macros.
**	30-may-90 (blaise)
**	    Add "define SIGLD SIGCHLD" (the converse was already defined) to
**	    ensure that the BSD signal and its SysV equivalent are both
**	    defined.
**	21-mar-91 (seng)
**	    Add <systypes.h> to clear <types.h> on AIX 3.1.
**	02-jul-91 (johnr)
**	    Add <systypes.h> to clear <types.h> on hp3_us5.
**	12-feb-93 (swm)
**	    Add axp_osf typedefs for EX_SIGCODE and EX_SIGCONTEXT.
**	01-feb-95 (medji01)
**	    Fix for bug 62988. Change SysV definition for SIGCLD from
**	    SIGHCLD to SIGCHLD.          
**      09-feb-95 (chech02)
**          Added rs4_us5 for AIX 4.1.
**      30-Nov-94 (nive/walro03)
**          For dg8_us5, added inclusion of systypes.h and chnaged the defines
**          for EX_SIGCONTEXT and EX_SIGCODE during OpenINGRES 1.1 port.
**	20-mar-95 (smiba01)
**	    Added support for dr6_ues based on dr6_us5
**	22-jun-95 (allmi01)
**	    Added support for dgi_us5 )DG-UX on Intel based platforms) following dg8_us5.
**	21-jul-95 (morayf)
**	    Added necessary ucontext-related header file for sos_us5.
**  12-nov-96 (kinpa04)
**		Added alternate handler parameters declarations fo those
**		machines that set SA_SIGINFO on sigaction. eg: axp_osf
**	05-Dec-1997 (allmi01)
**	    Added sgi_us5 specific #defines and #undefs to get sigcontext
**	    struct definitions from the sgi system header sys/siginfo.h.
**	10-Aug-1998 (kosma01)
**	    Remove sgi_us5 specific #defines and #undefs, for sigcontext
**	    struct definitions. Other changes to the compiler command
**	    made them obsolete.
**      06-May-1999 (allmi01)
**          Added and include for sys/sigaction.h for rmx_us5.
**      10-may-1999 (walro03)
**          Remove obsolete version string dr6_ues, sqs_us5.
**      03-jul-1999 (podni01)
**          Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**      16-jul-1999 (hweho01)
**          Included systypes.h for ris_u64 (AIX 64-bit platform). 
**	    
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**      10-Jan-2000 (hanal04) Bug 99965 INGSRV1090.
**          Correct rux_us5 changes that were applied using the rmx_us5
**          label.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-jul-2001 (stephenb)
**	    Add support for i64_aix
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
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/
# ifndef CLCONFIG_H_INCLUDED
        # error "didn't include clconfig.h before clsigs.h"
# endif

# ifndef CLSIGS_H_INCLUDED
# define CLSIGS_H_INCLUDED
/*
** include <systypes.h> to prevent types problems with signals
*/
#if defined(dr6_us5) || defined(hp3_us5) ||\
    defined(any_aix) || defined(dg8_us5) || defined(dgi_us5)
# include <systypes.h>
#endif /* dr6_us5 aix hp3_us5 dg8_us5 dgi_us5 */

/*
** include signal.h first. 
** It is needed by ucontext.h in on some platforms 
*/
# include   <signal.h>

/*
** include <siginfo.h> and <ucontext.h> if they exist; these provide
** alternatives to the sigcontext pointer and code integer in sigvec-style
** signal handlers but are not available for all boxes with sigaction
*/
# ifdef xCL_072_SIGINFO_H_EXISTS
# if defined(rux_us5)
# include <sys/sigaction.h>
# endif
# include <siginfo.h>
# endif /* xCL_072_SIGINFO_H_EXISTS */
# ifdef xCL_071_UCONTEXT_H_EXISTS
# if defined(sos_us5)
# include <signal.h>
# include <sys/regset.h>
# endif /* sos_us5 */
# include <ucontext.h>
# endif /* xCL_071_UCONTEXT_H_EXISTS */


/*
** Make sure both SIGCHLD (BSD signal) and SIGCLD (SysV equivalent) are
** defined
*/

# ifndef SIGCHLD	/* BSD version */
# define SIGCHLD	SIGCLD	/* SV-BSD difference */
# endif

# ifndef SIGCLD		/* SysV version */
# define SIGCLD		SIGCHLD
# endif

# if defined(xCL_011_USE_SIGVEC) && !defined(sigmask)
# define sigmask( m ) 	( 1 << ( (m) - 1 ) )
# endif


/* need to prepend name on sequent system 5 */

# define	sigvec_func	sigvec

/*
** setup the correct signal parameters
**
** A signal handler can be called with the following:
** 
** 1) handler(signum, sigcode, sigcontext)
**    int	signum;
**    int	sigcode;
**    struct sigcontext *sigcontext;
**
** 2) handler(signum, sigcode, sigcontext)
**    int	signum;
**    siginfo_t sigcode;
**    ucontext_t sigcontext;
**
** 3) handler(signum, sigcode, sigcontext)
**    int	signum;
**    int	sigcode;
**    int	sigcontext;
**
** 4) handler(signum, sigcode, sigcontext)
**    int	signum;
**    siginfo_t 	sigcode;
**    struct sigcontext 	*sigcontext;
**
** Provide a way to hide all these differences from the source code.
**
** handler(signum, sigcode, sigcontext)
** int	signum;
** EX_SIGCODE	SIGCODE(sigcode);
** EX_SIGCONTEXT SIGCONTEXT(sigcontext);
**
*/

#if defined(dg8_us5) || defined(dgi_us5) || defined(LNX) || \
    defined(OSX)
# define gotit
# define EX_SIGCODE siginfo_t
# define EX_SIGCONTEXT ucontext_t
# define SIGCODE(code) *code
# define SIGCONTEXT(context) *context
#endif /*dg8_us5 dgi_us5 */

# ifdef axp_osf
# define gotit
typedef siginfo_t		EX_SIGCODE;
typedef	struct sigcontext EX_SIGCONTEXT;
# define SIGCODE(code)	*code
# define SIGCONTEXT(context) *context
# endif /* axp_osf */

/* Default cases down here */

#ifndef gotit

#ifdef	xCL_011_USE_SIGVEC
# define gotit
typedef i4		EX_SIGCODE;
typedef	struct sigcontext EX_SIGCONTEXT;
# define SIGCODE(code)	code
# define SIGCONTEXT(context) *context
#endif	/* xCL_011_USE_SIGVEC */

#if defined(xCL_072_SIGINFO_H_EXISTS) && defined(xCL_071_UCONTEXT_H_EXISTS)
# define gotit
typedef siginfo_t	EX_SIGCODE;
typedef ucontext_t	EX_SIGCONTEXT;
# define SIGCODE(code)	*code
# define SIGCONTEXT(context) *context
#endif	/* xCL_072_SIGINFO_H_EXISTS && xCL_071_UCONTEXT_H_EXISTS */

#endif	/* gotit */

#ifndef	gotit
typedef	i4	EX_SIGCODE;
typedef i4	EX_SIGCONTEXT;
# define SIGCODE(code)	code
# define SIGCONTEXT(context) context
#endif	/* gotit */


# endif /* CLSIGS_H_INCLUDED */
