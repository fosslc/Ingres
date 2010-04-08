/*
 * accompat.c
 *
 * History:
 *      05-July-91 (DonJ)
 *              Change quotes to brackets on include of achilles.h.
 *		Add quote to end of include in sqs_us5 section.
 *		change <time.h> to <sys/time.h>.
 *	31-Oct-91 (DonJ)
 *		Fix perror("Oops") call in pathlist search.
 *	17-dec-91 (francel/kirke)
 *		Add headers for hp8_us5, hp3_us5, ris_us5, dg8_us5,ds3_ulx.
 *      29-jan-1992 (francel)   
 *              Changed a.800.h to a800.h for proper filename coding standard.
 *	16-apr-92 (deastman)
 *		Added hdrs for pyr_u42.
 *      29-apr-92 (purusho)
 *              Added amd_us5 specific changes
 *	18-may-92 (donj)
 *		Fixed one of (purusho)'s changes. He left off a semi-colon.
**	18-Aug-92 (GordonW)
**		On SYStem V machimes we need to re-plant the signal handler.
**		It won;t hurt to do this for everyone. If we don't, the
**		first SIGALRM will kill the achilles driver.
**		On HP machines the parameter rto sigblock is a long not int.
**	10-Nov-92 (GordonW)
**		Achilles forks all the time, so try suing vfork to speed
**		up the processs creation phase.
**	11-Nov-92 (GordonW)
**		If we are defining ACfork we might as well use it.
**	23-Nov-92 (GordonW)
**		Replant signal handler sooner.
**		Remove an extra puts call.
**      30-Dec-92 (rkumar)
**              (sqs_ptx specific changes)
**              This machine uses the termio interface rather than the 
**              sgtty interface  (and not all the ioctl's are supported).
**              Add routines to translate and place the correct 
**              information in the termio structure and makes the
**              right calls.
**	08-feb-93 (pghosh)
**		Added ncr port specific changes for nc4_us5. For signal
**		handling & wait3(), we are using '/usr/ucbinclude' files
**		and a special MINGH file for ncr in achilles/achilles to 
**		include -lucb library while building achilles.
**	13-May-93 (GordonW)
**		Provide the ability to use II_ACH_TIMER to select internal
**		limit for itimers. BTW; resolution is 1 secs.
**	15-Jun-93 (Jeffr)
**		Changed stdout to write to logfile in ACforkChild - fixes
**		problem of writing to stderr (unbuffered I/O) 
**	17-Jul-93 (jeffr)
** 		Changed way HP does unblocking in (ACunblock) - 
**		must replant handler in SVR 3.X based HP-UX
**	19-Jul-93 (Jeffr)
** 		Changed HP-UX to termio paradigm (HP-UX is SVR3 not BSD)
**	20-Jul-93 (Jeffr)
**		Added report generation stuff in ACchildHandler
**	27-Jul-93 (jeffr)
**		Set default state of children to Block in ACforkchild - 1-11
**		are caught - the rest should probably be ignored 
**	5-Aug-93 (jeffr)
**		Added return status checking from Children for report generation
**              in ACchildHandler 
**
**	8-7-93 (Jeffr)
**              Fixed file close logic bug - compares to stdout now	
**	8-13-93 (jeffr)
**		relocated total count for children to count correctly
**	8-13-93 (jeffr)
**		compare child for error with 0377 now
**	11-nov-93 (dianeh)
**		Added machine/ to a.out.h, so it can be properly found.
**	11-nov-93 (dianeh)
**		Back out previous change -- it wasn't right, but as it turns
**		out, you don't even need a.out.h, so take it out altogether.
**	15-Nov-93 (GordoNW)
**		remove searching local directory. Just rely on PATH symbol.
**		Hp8 should use sys/file.h
**
**	10-Jan-94 (jeffr)
**              change divisor to float to prevent underflow in acreport module
**
**	05-Apr-94 (jeffr)
**		added HP8 to ACseterm - status char was not being init for this
**		box
**	20-Dec-94 (GordonW)
**		Adding Solaris.
**	13-Feb-1995 (canor01)
**		Adding rs4_us5 (AIX 4.1).
**	22-jun-95 (allmi01)
**		Added dgi_us5 support for DG-UX on Intel based platforms following dg8_us5.
**	15-jan-1996 (toumi01; from 1.1 axp_osf port)
**		Adding axp_osf.
**	27-feb-96 (hopgi01)
**		Added support for sui_us5 following su4_us5
**	03-jun-1996 (canor01)
**		The Solaris 2.5 compiler defines "unix" but not "UNIX". Also
**		expects signal handler to accept an integer argument.
**      09-apr-1997 (hweho01) 
**              Cross-integrated UnixWare 2.1 fix from OI 1.2/01: 
**              Added support for usl_us5. 
**	14-may-97 (stephenb)
**		ACblock() and ACunblock() have no effect on Solaris, seemingly
**		due to a slip in #ifdefs, fixed problem and used recomended
**		Solaris routines to perform signal masking and 
**		blocking/unblocking
**	02-jun-1997 (muhpa01)
**		Removed calloc declaration for HP-UX to quiet compile complaint
**	23-jun-1997 (walro03)
**		Added support for ICL DRS 6000 (dr6_us5)
**	29-jul-1997 (walro03)
**		Adding pym_us5 (pyramid) and rmx_us5 (siemens).
**	12-aug-1997 (fucch01)
**		Added support for Tandem Nonstop (ts2_us5).
**      29-aug-1997 (allmi01)
**              Added support for SCO OpenServer (sos_us5).
**      29-aug-97 (musro02)
**              For sqs_ptx, don't define sigmask
**	02-jul-1998 (kosma01 for fucch01)
**		Added support for Silicon Graphics (sgi_us5).
**	29-sep-1998 (matbe01)
**		Removed include for signal.h and added define for sigmask for
**		NCR (nc4_us5).
**	15-nov-1998 (popri01)
**		With Unixware 7, the BSD signal stack structure is used by
**		default and the (ucb) BSD hdr causes errors; so delete it.
**	03-may-1999 (toumi01)
**		Added support for Linux (lnx_us5).
**	10-may-1999 (walro03)
**		Remove obsolete version string amd_us5, pyr_u42, sqs_us5.
**	06-oct-1999 (toumi01)
**		Change Linux config string from lnx_us5 to int_lnx.
**	22-Nov-1999 (hweho01)
**		Added support for ris_u64 (AIX 64-bit platform).
**      18-Feb-2000 (linke01)
**              Removed include file signal.h and added other support for 
**              Unixware (usl_us5). 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	15-Jun-2000 (hanje04)
**		Added support for OS/390 Linux (ibm_lnx)
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-Sep-2000 (hanje04)
**		Added support for axp_lnx (Alpha Linux).
**	26-Mar-20001 (hanch04
**		Removed calloc declaration for all platforms.
**	15-May-2002 (hanje04)
**	    Added support for Itanium Linux (i64_lnx)
**      08-Oct-2002 (hanje04)
**          As part of AMD x86-64 Linux port, replace individual Linux
**          defines with generic LNX define where apropriate
**	16-Oct-2002 (inifa01)
**	    Added missing ")" to fix compile error.
**	15-Mar-2005 (bonro01)
**	    Add support for Solaris AMD64 a64_sol
**      18-Apr-2005 (hanje04)
**          Add support for Max OS X (mac_osx)).
**          Based on initial changes by utlia01 and monda07.
**	11-Apr-2007 (hanje04)
**	    SIR 117985
**	    Fix compile problems against GLIBC 2.5 for PowerPC Linux port.
**	30-Apr-2007 (hanje04)
**	    SIR 117985
**	    Use xCL_NEED_RPCSVC_REX_HDR to conditionally include rcpsrv/rex.h
**	    instead of GLIBC version as it's not consistent across Linuxes.
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
**	28-Jun-2009 (kschendel) SIR 122138
**	    Use sparc-sol, any-hpux, any-aix symbols where needed.
**/

#if defined(UNIX) || defined(unix)
#include <clconfig.h>
#endif

#include <achilles.h>

#include <pwd.h>

#ifdef dr6_us5
#include <sys/ttold.h>
#include <sys/termio.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include "wait3.h"
#define sigmask(x)	(1 << ((x) - 1))
#endif /* dr6_us5 */

#include <sys/file.h>

#if defined(dg8_us5) || defined(dgi_us5)
#define _BSD_TTY_FLAVOR
#include <ioctl.h>
#endif

#ifdef any_hpux
#include <sgtty.h> 
#include  <termio.h>
#include "missing.h"
#include "a800.h"
static struct termio tbuf;
#endif

#ifdef sqs_ptx
#include <sys/wait.h>
#include <sys/ttold.h>
#include <sys/fcntl.h>
#include <sys/termio.h>
#include <sys/signal.h>
#include "missing.h"
#include "wait3.h"
#endif

#if defined(usl_us5)
#include "/usr/ucbinclude/sys/wait.h"
#include <sys/ttold.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <sys/termios.h>
#define sigmask(x)    (1 << ((x) - 1)) 
#endif

#ifdef hp3_us5
#include <sgtty.h> 
#include  <termio.h>
#include "missing.h"
static struct termio tbuf;
#endif

#if defined(pym_us5) || defined(rmx_us5) || defined(rux_us5)
#define	F_OK	0
#define	X_OK	1
#define	sigmask(x)	(1 << ((x) - 1))
#include "/usr/ucbinclude/sys/wait.h"
#include <sys/ttold.h>
#include <sys/fcntl.h>
#include <sys/termio.h>
#endif

#ifdef sun_u42      /* plus sun3 */
#include <sys/ioctl.h>
#include <sys/unistd.h>
#include <sys/errno.h>
#endif

#ifdef su4_u42      /* plus sun4 */
#include <sys/ioctl.h>
#include <sys/unistd.h>
#include <sys/errno.h>
#endif

#if defined(sparc_sol)	/* plus solaris */
#include <fcntl.h>
#include <sgtty.h>
#include <termios.h>
#include <eti.h>
#include <unistd.h>
/**
#include <sys/errno.h>
#include <sys/ttold.h>
**/
#define sigmask(x)      (1 << ((x) - 1))
#endif

#ifdef a64_sol
#include <fcntl.h>
#include <sgtty.h>
#include <sys/ttold.h>
#endif

#if defined(sui_us5)	/* plus solaris for intel */
#include <ttold.h>
#include <fcntl.h>
#include <sgtty.h>
#include <eti.h>
#include <unistd.h>
#define sigmask(x)      (1 << ((x) - 1))
#endif

#ifdef ds3_ulx      /* plus ultrix  */
#include <sys/ioctl.h>
#endif

#if defined(any_aix)
#include <xcoff.h>
#include <sys/ioctl.h>
#endif

# ifdef nc4_us5 
#include "/usr/ucbinclude/sys/wait.h"
#include <sys/ttold.h>
#include <sys/fcntl.h>
#include <sys/termio.h>
#include <unistd.h>
#define sigmask(x)      (1 << ((x) - 1))
#endif

#include <sys/wait.h>
#if defined(any_aix)
#include <sys/m_wait.h>
#endif

#include <sys/stat.h>

#include <sys/time.h>

#ifdef ts2_us5
#include <sgtty.h>
#include <sys/ioctl.h>
#include <sys/ioccom.h>
#include <unistd.h>
#include <fcntl.h>
#include "missing.h"
#define sigmask(x)	(1 << ((x) - 1))
#endif

#ifdef axp_osf
#include <sys/ioctl.h>
#endif

#ifdef sos_us5
#include <sys/wait.h>
#include <sys/ttold.h>
#include <sys/termio.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include "missing.h"
#include "accompat.h"
#define sigmask(x)      (1 << ((x) - 1))
#endif

#ifdef sgi_us5
#include <sys/ttold.h>
#include <sgtty.h>
#include <wait.h>
#include "accompat.h"
#endif

#ifdef LNX
#include <termio.h>
#  ifdef xCL_NEED_RPCSVC_REX_HDR
#    include <rpcsvc/rex.h>
#  endif
static struct termio tbuf;
#endif

#ifdef OSX      /* Mac OS X */
#include <sys/types.h>
#include <termios.h>
#include <sys/tty.h>
#include <sgtty.h> 
static struct termios tbuf;
#endif

#include <signal.h>

#ifdef xCL_023_VFORK_EXISTS
#define	ACfork	vfork
#else
#define	ACfork	fork
#endif

/*
** History:
**	26-Sept-89 (GordonW)
**		use a portable MEreqmem.
**	28-Sept-89 (GordonW)
**		use a stat call instead of a MAGIC macro call to test
**		for a shell script.
**	31-Jun-90  (curleym)
**		Signal and include for child handler.
**	23-Jul-90  (curleym)
**		Add the necessary includes and ifdef's for AT&T universe
**		compilation and loading on Sequent.
**	31-Jul-90  (curleym)
**		Modified #includes for HP-UX.
**	13-Feb-90  (dufour)
**	        Removed "R_OK" validation from access.  Executables do
**		not require read permission.  When executables created
**		via ming, read permission bit is not normally set.
**	29-apr-92  ( purusho )
**		Moved the #define ACunblock from hdr/accompat.h to accompat.c
**	 	because Amdahl doesnot support sigsetmask etc and used the
**		following work around om amd_us5
**	10-Nov-92 (GordonW)
**		Make it compile on HP8.
**      14-May-1999 (allmi01)
**              Changed compile behavior to get rmx_us5 to not use BSD ucb
**              functions.
**      03-jul-99 (podni01)
**              Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**      27-Jun-2000 (linke01)
**              modify change#446085 with adding "\" on ifdef statement. 
**	16-Oct-2002 (inifa01)
**		Added missing ")" to fix compile error.
*/

extern i4  achilles_start;
i4  active_children = 0;

void ACunblock()
{
#if defined (sqs_ptx)
	int zero = 0; 
	sigprocmask(SIG_UNBLOCK, &zero, (int *)0); 
#else
#if defined(nc4_us5) || defined(hp3_us5) || defined(any_hpux)
	/* We want to setup the proper Handlers just before we unblock. */
	extern VOID alarm_handler();
	ACsetupHandlers();
	signal(SIGALRM, alarm_handler);
#endif /* nc4_us5 ... */
#if defined(sparc_sol) || defined(sui_us5) || defined(sos_us5) || \
    defined(rmx_us5) || defined(rux_us5) || defined(usl_us5) || \
    defined(a64_sol)
	sigset_t	mask;
	sigfillset(&mask);
	sigprocmask(SIG_UNBLOCK, &mask, (sigset_t *)0);
#else
        sigsetmask(0);
#endif
#endif /* sqs_ptx */
}


VOID ACblock ( flags )
i4  flags;
{
#if defined(hp3_us5) || defined(any_hpux)
	long newmask = 0;
#else
#if defined(sparc_sol) || defined(sui_us5) || defined(sos_us5) || \
    defined(a64_sol)
	sigset_t	newmask;
	(VOID)sigemptyset(&newmask);
#else
	int newmask = 0;
#endif
#endif

	if (flags & CHILD)
#if defined(sparc_sol) || defined(sui_us5) || defined(a64_sol)
		(VOID)sigaddset(&newmask, SIGCHLD);
#else
		newmask |= sigmask(SIGCHLD);
#endif
	if (flags & STATRPT)
#if defined(sparc_sol) || defined(sui_us5) || defined(a64_sol)
		(VOID)sigaddset(&newmask, SIGQUIT);
#else
		newmask |= sigmask(SIGQUIT);
#endif
	if (flags & ABORT)
#if defined(sparc_sol) || defined(sui_us5) || defined(a64_sol)
		(VOID)sigaddset(&newmask, SIGINT | SIGTERM);
#else
		newmask |= sigmask(SIGINT) | sigmask(SIGTERM);
#endif
	if (flags & ALARM)
#if defined(sparc_sol) || defined(sui_us5) || defined(a64_sol)
		(VOID)sigaddset(&newmask, SIGALRM);
#else
		newmask |= sigmask(SIGALRM);
#endif
#if defined(sqs_ptx) || defined(sparc_sol) || defined(sui_us5) \
 || defined(sos_us5) || defined(rmx_us5) || defined(rux_us5) \
 || defined(usl_us5) || defined(a64_sol)
	sigprocmask(SIG_BLOCK, &newmask, 0);
#else
	sigblock(newmask);
#endif
} /* ACblock */

# if defined(sparc_sol) || defined(a64_sol)
VOID ACchildHandler (int sig)
# else
VOID ACchildHandler ()
# endif
{
	register i4    i,
		       j,
		       pid,
		       found_it;
	char	      *fname;
	char	       timebuf [80];
	extern PID     achpid;
	HI_RES_TIME    now;
#if defined(sparc_sol) || defined(a64_sol) || defined(LNX)
/* and probably others... */
	int		status;
# define STANDARD_WAIT
#else
	WAIT_STATUS     status;
#endif
#if !defined(sparc_sol) && !defined(sui_us5)
	struct tchars  tc;
#endif
	struct sgttyb  sg; 
	ACTIVETEST    *curtest;
	struct stat    st;
	int		termsig =0;
	int		retcode=0;


		ACblock(ALARM | STATRPT | ABORT);
#if defined(STANDARD_WAIT)
	while ( (pid=(int)waitpid(0, &status, (WNOHANG|WUNTRACED))) > 0 )
#else
	while ( (pid=(int)wait3(&status, (WNOHANG|WUNTRACED), NULL)) > 0 )
#endif
	{
		found_it = 0;
		/* Find which child process terminated. */
		for (i = 0 ; i < numtests ; i++)
		{
			for (j = 0 ; j < tests[i]->t_nkids ; j++)
				if (tests[i]->t_children[j].a_pid == pid)
				{
					found_it++;
					break;
				}
			if (found_it)
				break;
		}

		signal ( SIGCHLD,ACchildHandler );

		ACgetTime(&now, (LO_RES_TIME *) NULL);

		/* There's no reason why we should ever get info on a process
		   that's not stored in the data structures, but in case...
		*/
		if (i == numtests)
		{
			log_entry(&now, -1, -1, -1, pid, C_NOT_FOUND, 0, 0);
			ACunblock();
			return;
		}
		curtest = &(tests[i]->t_children[j]);
		/** keep running total of child times for each instance **/
	  tests[i]->t_ctime+= ACcompareTime(&now,&curtest->a_start)/3600.0;
		/*SIfprintf(logfile,"tests[%d]->t_ctime %f\n",i,tests[i]->t_ctime,'.'); */ 

#if defined(STANDARD_WAIT)
		if (WIFEXITED(status))
		    retcode = WEXITSTATUS(status);
		if (WIFSIGNALED(status))
		    termsig = WTERMSIG(status);
#else
		termsig = status.w_termsig;
		retcode = status.w_retcode;
#endif
		
		/* Log the dead process. */
		/** verify the state first to see if it didnt die normally **/
		 if (termsig != SIGKILL && 
                           (retcode & 0377 != 0 )) tests[i]->t_cbad++;

		if (termsig)
			log_entry(&now, i, j, curtest->a_iters, pid, C_TERM,
			  termsig, &curtest->a_start);
		else
			log_entry(&now, i, j, curtest->a_iters, pid, C_EXIT,
			  retcode, &curtest->a_start);
		curtest->a_iters++;
		/* If more iteration if this instance should be run, start a
		   new one.
		*/
		if ( (curtest->a_iters <= tests[i]->t_niters)
		  || (tests[i]->t_niters <= 0) ) 
                 {     
		    active_children--;
		    start_child(i, j, curtest->a_iters);
		/** add to total children running - vers 2 - jeffr **/
        	    tests[i]->t_ctotal++;
		 }
		else
		{
			/* If we don't need to start any more tests, clean up
			   the output file: If no output file was specified in
			   the config file, and the "temporary" output file
			   remained empty (ie no unexpected error messages),
			   remove it. If it shouldn't be removed, make it owned
			   by the user who's running this program. (It's owned
			   by root for now.)
			*/
			LOtos(&curtest->a_outfloc, &fname);
			if ( (!*tests[i]->t_outfile)
			  && (!stat(fname, &st) )
			  && (st.st_size == 0) )
				unlink(fname);
			else
				ACchown(&curtest->a_outfloc, uid);
			/* Record the end time of the last process in a_start,
			   for use by print_status.
			*/
			curtest->a_start = now.time;
			active_children--;
		}
	/** add to total children run for this test - includes all tagged children **/
	}
	/* If there are no more outstanding child processes, we're done */
	if (pid < 0)
	{

	    /* time and date stamp the conclusion of the achilles test run
	       11/16/89, bls
	    */

	    ACgetTime (&now, (LO_RES_TIME *) NULL);
	    ACtimeStr (&now, timebuf);
	    SIfprintf (logfile, "%-20.20s  %-15.15s  %5d  %-12s\n", 
	               timebuf, "ACHILLES END", achpid, "END");
	    SIflush(logfile);
/** generate report **/
	    ACreport();

		if (logfile != stdout) fclose(logfile);
#if defined(hp3_us5) || defined(any_hpux) || defined(LNX)
		if (ioctl(fileno(stdout), TCGETA, &tbuf) != -1)
                {
                        tbuf.c_cc[1] = orig_quit; 
                        ioctl(fileno(stdout), TCSETA, &tbuf);
                }
	
		if (ioctl(fileno(stdout), TCGETA, &tbuf) != -1)
                {
                        tbuf.c_lflag |= ECHO;
                        ioctl(fileno(stdout), TCSETAF, &tbuf);
                }
#else 
#if defined(sqs_ptx) 
		if (_get_term(fileno(stdout), &tc) != -1)
		{
			tc.t_quitc = orig_quit;
			_set_term(fileno(stdout), &tc, 0);
		}
		termop(fileno(stdout), _ECHO | _DRAIN);
#else 
#if !defined(sparc_sol) && !defined(sui_us5)
/*		fprintf(stderr,"doing sun term reset\n"); */
		if (ioctl(fileno(stdout), TIOCGETC, &tc) != -1)
		{
			tc.t_quitc = orig_quit;
			ioctl(fileno(stdout), TIOCSETC, &tc);
		}

		if (ioctl(fileno(stdout), TIOCGETP, &sg) != -1)
		{
			sg.sg_flags |= ECHO;
			ioctl(fileno(stdout), TIOCSETN, &sg);
		}
#endif
#endif /* sqs_ptx */
#endif
		PCexit(OK);
		signal ( SIGCHLD,ACchildHandler );
		/* Reinitialize handler */
	}

	else
	{
	    ACgetTime (&now, (LO_RES_TIME *) NULL);
	    ACtimeStr (&now, timebuf);
	    SIfprintf (logfile, "%-20.20s  %-15.15s  %5d  %-12s\n", 
	               timebuf, "ACHILLES WAIT", achpid, "SUSPENDED");
	    SIflush(logfile);
	}

	signal ( SIGCHLD,ACchildHandler );
	ACunblock();
	return;
} /* ACchildHandler */

VOID ACchown (loc, uid)
LOCATION *loc;
UID uid;
{
	char *fname;

	LOtos(loc, &fname);
	chown(fname, uid, -1);
} /* ACchown */



extern char *testpath;
extern int errno;

STATUS ACfindExec (tempdata, gooddata, data_end, argv, cur_argv)
char  *tempdata,
     **gooddata,
     **data_end,
      *argv[];
i4    *cur_argv;
{
	char	      exname[128];
	i4	      i;
	static char **pathlist = NULL;

	/* If user specified a location, look there first. */
	errno = 0;
	if (testpath)
	{
		STprintf(exname, "%s/%s", testpath, tempdata);
		if(!access(exname, F_OK))
		{
			if (!access(exname, X_OK) )
				*gooddata = exname;
			else
				perror("access");
		}
	}

	/* Next, search the directories in the PATH environment variable. */
	if (!*gooddata)
	{
		/* If the PATH variable hasn't already been translated into an
		   argv-like array of strings, do the conversion now.
		*/
		if (!pathlist)
		{
			if(ACgetPathList(&pathlist) < 0)
				return(FAIL);
		}
		for (i =0 ; pathlist[i] ; i++)
		{
			STprintf(exname, "%s/%s", pathlist[i], tempdata);
			errno = 0;
			if(!access(exname, F_OK))
			{
				if (!access(exname, X_OK) )
				{
					*gooddata = exname;
					errno = 0;
					break;
				}
				else
					perror("access");
			}
		}
	}

	/* Finally, look in the current directory. If we find it here, we don't
	   have to prepend a directory name.
	*/
#ifdef USE_LOCAL
	if ( !*gooddata)
	{
		STcopy(tempdata, exname);
		if(!access(exname, F_OK))
		{
			if(!access(exname, X_OK))
				*gooddata = tempdata;
			else
				perror("access");
		}
	}
#endif

	if(!*gooddata)
		fprintf(stderr, "Can't find %s\n", tempdata);

	return(*gooddata ? OK : !OK);
} /* ACfindExec */

VOID ACforkChild (testnum, curtest, iternum)
i4	    testnum;
ACTIVETEST *curtest;
i4  iternum;
{
	char *fname;
        extern      FILE *filename;        /* logfile name               */
        extern      int  silentflg;        /* discard front-end stdout ? */
                                           /* 10-27-89, bls              */
	char buf[BUFSIZ];


	if ( (curtest->a_pid = ACfork() ) == 0)
	{
		int fd;
		int f_mode;
		/* Since the parent process has special signal handling in
		   place, we need to restore the child to something more
		   normal. We also need to detach the child process from the
		   parent's controlling terminal, since otherwise INT and QUIT
		   signals meant for the parent will get sent to the child as
		   well.
		*/
		ACblock(ALARM | STATRPT | ABORT);

		if ( (fd = open("/dev/tty", O_RDONLY) ) >= 0)
		{
#ifndef sqs_ptx
			ioctl(fd, TIOCNOTTY, 0);
			close(fd);
#else
/** this isnt preventing signals from killing children (i.e. SIGQUIT)
      - therfore they are blocked above - (jeffr)
**/
			setsid();   /* backgroud it */
			close(fd);
#endif 
		}
		else perror("open");
   
		if (*tests[testnum]->t_infile)
		{
			if ((fd = open(tests[testnum]->t_infile, O_RDONLY)) < 0)
			{
				perror(tests[testnum]->t_infile);
				_exit(254);
			}
			if (fd != 0)
			{
				dup2(fd, 0);
				close(fd);
			}
		}

                                     /* if "silent mode" has been specified */
                                     /* throw away front-end stdout, and    */
                                     /* re-direct stderr to achilles logfile*/
                                     /* 10-27-89, bls                       */
		if (silentflg)
                    fname = "/dev/null";
		else
		{
		    LOtos(&curtest->a_outfloc, &fname);
		}

		f_mode = (O_WRONLY | O_CREAT | O_TRUNC);

		if ((fd = open(fname, f_mode, 0666)) < 0)
                {
		    perror(fname);
		    fd = fileno(stdout);
                }

		if (fd != 1) {
			/*dup2(fd, 1); */
			dup2(fileno (logfile), 1); 
			/** stdout should point to logfile now -  
			child now writes to where stdout is pointing to,
			before it wrote to where stderr was pointing to
			- jeffr **/
		}
		if (fd != 2)
		{
			dup2(fileno (logfile), 2);
			/*** dup2(fd, 2); ***/

			if (fd != 1)
				close(fd);
		}
		

		if (setuid(tests[testnum]->t_user) < 0)
			perror("Can't change user ID of child process");
		execv(curtest->a_argv[0], curtest->a_argv);
		perror("execv");
		_exit(254);

/** add one to global active children - jeffr **/
	
/**	fprintf(stderr,"active children = %d\n",active_children); **/
	}
	else
		if (curtest->a_pid < 0) 
		{
			curtest->a_pid = 0;
			perror("fork");
		}
	active_children++;
	/*fprintf(stderr,"active children = %d\n",active_children);  */
} /* ACforkChild */

extern char *getenv();

ACgetPathList (plist)
char ***plist;
{
	register char *p;
	register int i;
	char *tmppath, *path;
	int word_cnt = 0, len;
	STATUS	status;

	if ( (tmppath = getenv("PATH") ) == NULL)
	{
		*plist = (char **) MEreqmem(0, sizeof(char *), TRUE, &status);
	  	if( status != OK )
		{
			SIprintf("Can't MEreqmem *plist\n");
			return(-1);
		}
	}
	else
	{
		path = MEreqmem(0, STlength(tmppath)+1, TRUE, &status);
		if( status != OK )
		{
			SIprintf("Can't MEreqmem tmppath\n");
			return(-1);
		}
		STcopy(tmppath, path);
		for (p = path ; *p ; p++)
			if (*p == ':')
				word_cnt++;
		*plist = (char **) MEreqmem(0, (word_cnt+2)*sizeof(char *),
		  			TRUE, &status);
		if(status != OK )
		{
			SIprintf("Can't MEreqmem word_cnt\n");
			return(-1);
		}
		(*plist)[0] = path;
		i = 1;
		for (p = path ; *p ; p++)
			if (*p == ':')
			{
				*p = '\0';
				(*plist)[i++] = p+1;
			}
		(*plist)[i] = NULL;
	}
	return(0);
} /* ACgetPathList */



STATUS ACgetUser (tempdata, user)
char *tempdata;
UID  *user;
{
	struct passwd *pw;

	if (!STcompare(tempdata, "<USER>") )
		*user = uid;
	else
		/* Make sure a real username was specified, and get the
		   corresponding user ID. Also, disallow running tests as root
		   unless root is running this program. (Security hole
		   otherwise - we don't like those just on principle.)
		*/
		if ( (pw = getpwnam(tempdata) ) == NULL)
			return(NO_SUCH_USER);
		else
			if ( (pw->pw_uid == 0) && (uid != 0) )
				return(NOT_ROOT);
			else
				*user = pw->pw_uid;
	return(OK);
} /* ACgetUser */



extern void alarm_handler();

VOID ACinitAlarm (waittime)
i4  waittime;
{
	struct itimerval it;
	char	*p;
	int	waitinternal = 1;

	signal(SIGALRM, alarm_handler);
	if( waittime >= 0 )
	{
		if(p=getenv("II_ACH_TIMER"))
			waitinternal = atoi(p);

		it.it_interval.tv_sec = waitinternal;
		it.it_interval.tv_usec = 0;
		it.it_value.tv_sec = waittime;
		it.it_value.tv_usec = 0;
		setitimer(ITIMER_REAL, &it, (struct itimerval *) 0);
	}
} /* ACinitAlarm */

VOID ACioini()
{
}


VOID ACprintUser (user)
UID user;
{
	struct passwd *pw;

	pw = getpwuid(user);
	SIprintf("   Effective user = '%s'\n", pw->pw_name);
} /* ACprintUser */

char orig_quit;

/** default status char **/
char stat_char = '';
VOID ACsetTerm ()
{
#if !defined(sparc_sol) && !defined(sui_us5)
	struct tchars tc;
#endif
	struct sgttyb sg; 

	/* Change the terminal's QUIT character to something that is a little
	   more intuitive for getting a status report. (which ^\ isn't.)
	   Also, turn off keyboard echo - looks silly.
	*/
#if defined(sqs_ptx)
	if (_get_term(fileno(stdout), &tc) != -1)
	{
		orig_quit = tc.t_quitc;
		tc.t_quitc = stat_char;
		_set_term(fileno(stdout), &tc, 0);
        }
	termop(fileno(stdout), _NOECHO | _DRAIN);
#else
#if defined(hp3_us5) || defined(any_hpux) || defined(LNX)
/** HP-UX is SYS 5 not BSD **/

	if (ioctl(fileno(stdout), TCGETA, &tbuf) != -1)
	{
	        orig_quit = tbuf.c_cc[1];
	        tbuf.c_cc[1] = stat_char;	
		ioctl(fileno(stdout), TCSETA, &tbuf);
	}
	if (ioctl(fileno(stdout), TCGETA, &tbuf) != -1)
	{
		tbuf.c_lflag &= ~ECHO;
		ioctl(fileno(stdout), TCSETAF, &tbuf);
	}
#else
#if !defined(sparc_sol) && !defined(sui_us5)
/** BSD diehards go here **/

        if (ioctl(fileno(stdout), TIOCGETC, &tc) != -1)
        {
                orig_quit = tc.t_quitc;
                tc.t_quitc = stat_char;
                ioctl(fileno(stdout), TIOCSETC, &tc);
        }
        if (ioctl(fileno(stdout), TIOCGETP, &sg) != -1)
        {
                sg.sg_flags &= ~ECHO;
                ioctl(fileno(stdout), TIOCSETN, &sg);
        }
#endif
#endif
#endif
} /* ACsetTerm */

extern VOID ACchildHandler(), print_status(), abort_handler();

VOID ACsetupHandlers ()
{
	signal(SIGCHLD, ACchildHandler);
	signal(SIGQUIT, print_status);
	signal(SIGINT, abort_handler);
	signal(SIGTERM, abort_handler);
} /* ACsetupHandlers */

#if defined(sqs_ptx)
/*
 * This machine uses the termio interface rather than the sgtty interface
 * (and not all the ioctl's are supported). The next two routines translate
 * place the correct information in the termio structure and makes the 
 * right calls.
 */

int
_get_term(fd, tc)
int fd;
struct tchars *tc;
{
	struct termios tp;

	if (ioctl(fd, TCGETP, &tp) < 0)
		return -1;
	tc->t_intrc   = tp.c_cc[VINTR];
	tc->t_quitc   = tp.c_cc[VQUIT];
	tc->t_startc  = tp.c_cc[VSTART];
	tc->t_stopc   = tp.c_cc[VSTOP];
	tc->t_eofc    = tp.c_cc[VEOF];
	tc->t_brkc    = tp.c_cc[VEOL];

	return 0;
}

int _set_term(fd, tc, drain)
int fd;
struct tchars *tc;
char drain;
{
	struct termios tp;
	char op;

	if (ioctl(fd, TCGETP, &tp) < 0)
		return -1;
	tp.c_cc[VINTR] = tc->t_intrc;
	tp.c_cc[VQUIT] = tc->t_quitc;
	tp.c_cc[VSTART] = tc->t_startc;
	tp.c_cc[VSTOP] = tc->t_stopc;
	tp.c_cc[VEOF]  = tc->t_eofc;
	tp.c_cc[VEOL]  = tc->t_brkc;

	if (drain)
		op =  TCSETPW;
	if (ioctl(fd, op, &tp) < 0)
		return -1;
	return 0;
}

/* 
 * instead of using the sgtty structures, use the termio structures
 *	to do whatever needs to be done. For now, that's just turninof
 *	on and off echo (Let kernel translate, that's what is there for).
 */

termop(fd, op)
int fd;
int op;
{
	struct termios tp;

	if (ioctl(fd, TCGETP, &tp) < 0)
		return;
	switch (op & ~_DRAIN) { /* for now only one */
		case _ECHO:
			tp.c_lflag |= ECHO;
			break;
		case _NOECHO:
			tp.c_lflag &= ~ECHO;
		default:
			break;
	}

	if (op & _DRAIN) 
		ioctl(fd, TCSETPW, &tp);
	else
		ioctl(fd, TCSETP, &tp);
}
#endif

