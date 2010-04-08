/*
** History:
**	06-Feb-91	(dufour)
**		Added support for HP-UX (hp8_us5).
**	17-dec-91 (francel/kirke)
**		Added support for hp3_us5.
**	29-apr-92 (purusho)
**		Made some amd_us5 specific changes
**	18-Aug-92 (GordonW)
**		re-vamped the ACintChild and ACkillChild macros
**		to work a bit better.
**		For HP machines parameter to sigblock/seigsetmask is long.
**      21-aug-92 (kevinm)
**              On dg8_us5 use _BSD_SIGNAL_FLAVOR so achilles doesn't exit on 
**		SIG ALARM during sigpause.
**      30-dec-92 (rkumar)
**              included ftime.h for sqs_ptx. Don't define ACunblock for
**              sqs_ptx(please see accompat.c for work around).
**              Added sqs_ptx for hp8_us5 etc., to get ACinitRandom 
**              defines.
**      28-jan-93 (rkumar)
**              Removed nonportable ACunblock define from my earlier
**              change.
**	7-Aug-93 (jeffr)
**		Added new VMS specific stuff to handle Mailboxes for Version 2
**      7-Aug-93 (jeffr)
**		Changed Log formatting to be the same as Unix
**	20-Dec-94 (GordonW)
**		cleanup.
**	27-Dec-94 (GordonW)
**		finx su4_u42/su4_us5 usage of time/ftime.
**		Added externs for time(), and ftime().
**	22-jun-95 (allmi01)
**		Added dgi_us5 support for DG-UX on Intel based platforms following dg8_us5.
**	12-Jan-96  (bocpa01)
**		Added support for NT (NT_GENERIC).
**	27-feb-96 (hopgi01)
**		Added support for sui_us5, following su4_us4
**	02-feb-1997 (kamal03)
**		Splitted multiline #if defined... statement into multiple
**		#if ... lines, because VAX C precompiler doesn't honor 
**		continuation in this case.
**	29-May-97 (cohmi01)
**	  	Correct definition of ACgetEvents() for sun-sol so it doesn't
**		keep getting EINVAL and spin on the 'while' loop. 0 is an
**		invalid parm for sigpause on sun-sol, even though ok on HP.
**      29-aug-97 (musro02)
**              Removed include ftime.h for sqs_ptx.
**              As of DYNIX 4.0 V4.2.0 sys/timeb.h is available.
**	12-mar-98 (fucch01)
**		Add include of time.h for sgi_us5, since we need to use
**		time function in lieu of ftime, since it doesn't exist
**		on sgi.  Added sgi_us5 to platforms defining ACftime as
**		time.  Also, had to add typedef of union wait cut
**		from wait.h to get accompat.c to compile.  
**	23-Mar-1999 (kosma01)
**		Remove typedef of union wait cut from wait.h on sgi_us5.
**		Changes to other compiler #defines in mkdefault took care of it.
**	10-may-1999 (walro03)
**		Remove obsolete version string amd_us5.
**      14-May-1999 (allmi01)
**              Allow rmx_us5 to use rand() instead of random() removing
**              BSD ucb requirement.
**      03-jul-1999 (podni01)
**              Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**      22-Nov-1999 (hweho01)
**              Added support for ris_u64 (AIX 64-bit platform).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-Mar-2005 (bonro01)
**	    Add support for Solaris AMD65 a64_sol
**	12-May-2005 (hanje04)
**	    Remove bu4_us5 section. I'm not sure why it's there, it does
**	    nothing useful and it's causing problems for Mac OS X
**	14-Sep-2005 (hanje04)
**	    Remove local declarations on time() & ftime() and define
**	    ACftime to be time on Mac as ftime has been depriciated.
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
*/

#ifndef _ACCOMPAT_H
#define _ACCOMPAT_H

#ifdef NT_GENERIC
#include <windows.h>
#include <time.h>

#define CHILDKILLEDRC	16
#endif /* END #ifdef NT_GENERIC */

#ifdef VMS

#include <time.h>

#else


#include <sys/types.h>
#include <sys/timeb.h>
#ifdef sgi_us5
#include <sys/time.h>
#endif

#endif /* VMS */

#if defined(dg8_us5) && !defined(_BSD_SIGNAL_FLAVOR)
#define _BSD_SIGNAL_FLAVOR
#endif /* dg8_us5 && !_BSD_SIGNAL_FLAVOR */

#if defined(dgi_us5) && !defined(_BSD_SIGNAL_FLAVOR)
#define _BSD_SIGNAL_FLAVOR
#endif /* dgi_us5 && !_BSD_SIGNAL_FLAVOR */

#include <signal.h>

#ifdef VMS

#ifdef main
#undef main
#endif

#define	ACftime			ftime;
#define	ACtime			time;

#define ACblock(x)		sys$setast(0)
#define ACunblock()		sys$setast(1)

#define	ACinitRandom()		srand( (int) time( (long *) 0) )
#define ACrandom()		(i4) rand()

/*#define ACgetEvents()		sys$hiber() - before mailbox */

/** add mailbox read on startup for Vers 2 **/
#define ACgetEvents()          { readmbox2() ; sys$hiber(); }
#define UID			int
#define ACgetuid()		getuid()

#define LO_RES_TIME		int
#define HI_RES_TIME		struct timeb
#define TIME_STR_SIZE		20
#define ACloToHi(x, y)		(*y).time = (*x);(*y).millitm = 0

#define ACintEnabled(x)		(tests[(x)]->t_inttime.time != -1)
#define ACkillEnabled(x)	(tests[(x)]->t_killtime.time != -1)

#define ACintChild(x)		kill((x)->a_pid, SIGINT)
#define ACkillChild(x)		kill((x)->a_pid, SIGKILL)

#define ACdisableAlarm()	sys$cantim(1)

#define ACrightChar()		input_char == stat_char

/*#define AClogEntryFormat	"%s  %3d   %3d   %5d  %5x  %-12s" */
#define AClogTermFormat         "%s  %3d   %3d   %5d  %5d  %-12s  %3d  %s\n"

#define AClogEntryFormat        "%s  %3d   %3d   %5d  %5x  %-12s\n"

#define ACpidFormat(x)		(x) % 65536

#define NO_SUCH_USER    	OK + 1
#define NOT_ROOT        	OK + 2
#define NOT_IMPLEMENTED 	OK + 3

FUNC_EXTERN VOID   ACaddTime();
FUNC_EXTERN VOID   ACchown();
FUNC_EXTERN i4     ACcompareTime();
FUNC_EXTERN VOID   ACforkChild();
FUNC_EXTERN i4     ACgetopt();
FUNC_EXTERN char  *ACgetRuntime();
FUNC_EXTERN VOID   ACgetTime();
FUNC_EXTERN VOID   ACinitAlarm();
FUNC_EXTERN VOID   ACinitTimes();
FUNC_EXTERN VOID   ACioInit();
FUNC_EXTERN VOID   ACprintUser();
FUNC_EXTERN VOID   ACreleaseChildren();
FUNC_EXTERN VOID   ACresetAlarm();
FUNC_EXTERN VOID   ACresetPrint();
FUNC_EXTERN VOID   ACsetServer();
FUNC_EXTERN VOID   ACsetTerm();
FUNC_EXTERN VOID   ACsetupHandlers();
FUNC_EXTERN VOID   ACtimeStr();
/** add new mailbox function for Ver. 2 **/
FUNC_EXTERN i4    readmbox2();
GLOBALREF char input_char, stat_char;

#define MBOX_BUFSIZE	    256
/** END VMS Specific stuff **/
#else

#if defined(su4_us5) ||  defined(sqs_ptx) || defined(sui_us5) || \
    defined(sgi_us5) ||  defined(rmx_us5) || defined(rux_us5) || \
    defined(OSX)
#include <sys/time.h>
#define	ACftime(ptr)		time(ptr)
#else 
/** NT Specific **/
#ifndef NT_GENERIC
extern	int	ftime();
#endif /* NT_GENERIC */
/** END NT Specific **/
#define	ACftime(ptr)		ftime(ptr)
#endif /* su4_us5 || sqs_ptx || sui_us5 */
#ifndef NT_GENERIC
extern	time_t	time();
#endif /* NT_GENERIC */
#define	ACtime			time;
#ifdef NT_GENERIC
FUNC_EXTERN i4  ACgetopt();
#else
#define ACgetopt(x, y, z)	getopt ( (x), (y), (z) )
#endif /* NT_GENERIC */
#if defined(hp8_us5) || defined(hp3_us5) || defined(sqs_ptx) || \
    defined(su4_us5) || defined(NT_GENERIC) || defined(sui_us5) || \
    defined(rmx_us5) || defined(rux_us5)
#define	ACinitRandom()		srand( (unsigned) time( (long *) 0) )
#define ACrandom()		(i4) rand()
#else
#define	ACinitRandom()		srandom( (int) time( (long *) 0) )
#define ACrandom()		(i4) random()
#endif /* hp8_us5 ... */

# if defined(hp8_us5) || defined(hp3_us5)
#define ACgetEvents()		while (1) sigpause(0L)
# elif defined(su4_us5)
#define ACgetEvents()		while (1) sigpause(SIGCHLD)
# else
#ifdef NT_GENERIC
FUNC_EXTERN VOID ACgetEvents();
#else 
#define ACgetEvents()		while (1) sigpause(0)
#endif /* END #ifdef NT_GENERIC */
# endif /* hp8_us5 || hp3_us5 */

#define UID			int
#ifdef NT_GENERIC
FUNC_EXTERN readfile();
FUNC_EXTERN start_child();
#else
#define ACgetuid()		getuid()
#endif /* NT_GENERIC */
#define LO_RES_TIME		int
#define HI_RES_TIME		struct timeb
#define TIME_STR_SIZE		20
#define ACloToHi(x, y)		(*y).time = (*x);(*y).millitm = 0

#define ACintEnabled(x)		(tests[(x)]->t_inttime.time != -1)
#define ACkillEnabled(x)	(tests[(x)]->t_killtime.time != -1)

#define ACintChild(x)		(((x)->a_pid)?kill((x)->a_pid, SIGINT):0)
#ifdef NT_GENERIC
FUNC_EXTERN ACkillChild();
FUNC_EXTERN VOID ACreleaseChildren();
#else
#define ACkillChild(x)		(((x)->a_pid)?kill((x)->a_pid, SIGKILL):0)
#define ACreleaseChildren()	{}	
#endif /* END #ifdef NT_GENERIC */

#define ACresetAlarm()		{}
#ifdef NT_GENERIC
FUNC_EXTERN VOID   ACdisableAlarm();
#else
#define ACdisableAlarm()	signal(SIGALRM, SIG_IGN)
#endif /* END #ifdef NT_GENERIC */

#define ACrightChar()		1
#define ACresetPrint()		{}
#define AClogEntryFormat	"%s  %3d   %3d   %5d  %5d  %-12s\n"
#define AClogTermFormat	        "%s  %3d   %3d   %5d  %5d  %-12s  %3d  %s\n"
#define ACpidFormat(x)		(x)

#define ACioInit()		{}

#define NO_SUCH_USER		OK + 1
#define NOT_ROOT		OK + 2

FUNC_EXTERN VOID   ACaddTime();
FUNC_EXTERN VOID   ACblock();
FUNC_EXTERN VOID   ACchown();
FUNC_EXTERN i4     ACcompareTime();
FUNC_EXTERN STATUS ACfindExec();
FUNC_EXTERN VOID   ACforkChild();
FUNC_EXTERN char  *ACgetRuntime();
FUNC_EXTERN VOID   ACgetTime();
FUNC_EXTERN VOID   ACtimeStr();
FUNC_EXTERN VOID   ACinitAlarm();
FUNC_EXTERN VOID   ACinitTimes();
#ifndef NT_GENERIC
FUNC_EXTERN VOID   ACprintUser();
#endif /* END #ifndef NT_GENERIC */
FUNC_EXTERN VOID   ACsetServer();
FUNC_EXTERN VOID   ACsetTerm();
FUNC_EXTERN VOID   ACsetupHandlers();
FUNC_EXTERN VOID   ACioini();
#ifdef NT_GENERIC
FUNC_EXTERN 	   log_entry();
#endif /* END #ifdef NT_GENERIC */


GLOBALREF char   orig_quit;
#endif /* VMS */


#if defined(ris_us5) || defined(hp8_us5) || defined(su4_us5) || \
    defined(rs4_us5) || defined(NT_GENERIC) || defined(sui_us5) || \
    defined(ris_u64) || defined(a64_sol)
#define WAIT_STATUS     int
#else
#define WAIT_STATUS     union wait
#endif /* ris_us5 ... */
#undef line_1
#undef line_2

#endif /* _ACCOMPAT_H */
