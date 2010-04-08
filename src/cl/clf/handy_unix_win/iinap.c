/*
**  Copyright (c) 1989, 2002 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <clconfig.h>
# include <systypes.h>
# include <fdset.h>
# include <errno.h>
# ifndef NT_GENERIC
# include <sys/param.h>
# endif  /* NT_GENERIC */
# include <clpoll.h>
# include <iinap.h>

# ifdef	xCL_021_SELSTRUCT
# define GOT_TIMEVAL
# include    <sys/select.h>
# endif	/* SELSTRUCT */

# ifdef xCL_043_SYSV_POLL_EXISTS
# ifdef xCL_STROPTS_H_EXISTS
# include <stropts.h>
# endif /* xCL_STROPTS_H_EXISTS */
# include <poll.h>
# endif 	/*xCL_043_SYSV_POLL_EXISTS*/

# include <rusage.h>

# ifdef NT_GENERIC
GLOBALREF bool	WSAStartup_called;
# endif  /* NT_GENERIC */


/*
** Name: II_nap() - sub-second sleep
**
** Description:
**	II_nap() sleeps the specified number of microseconds.  It
**	returns 0 on normal termination and -1 if the function fails
**	to sleep for the specified time.
**
**	Implementation of II_nap() may not be able to provide micro-second
**	resolution.  Implementations using the poll() system call will 
**	round micro-second timeouts to the next highest milli-second increment.
**
**	Most OS implementations of select() will also not provide 
**	micro-second resolution.  For instance on sun4c machines a 
**	1 micro second timeout always results in a minimum of a 10 millisecond
**	sleep.
**      
** Inputs:
**      usecs -	Number of microseconds to sleep.
**
** Outputs:
**      none
**
**	Returns:
**	    -1 on error, 0 otherwise
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-jul-89 (russ)
**          Function initially created.
**	27-jul-89 (daveb)
**	    Include fdset.h, fix inclusion of time.h problem that also
**	    caused cOmpilation failure on sequent.
**	22-Aug-1989 (fredv)
**	    In AIX, timeval structure is defined in sys/select.h and sys/time.h.
**		To avoid the redeclaration error from the AIX compiler, added
**		ifdef GOT_TIMEVAL to indicate if we need to include sys/time.h.
**		Rearranged the headers so that rusage.h can see GOT_TIMEVAL
**		if it is defined, and hence, do not include sys/time.h.
**	23-Aug-89 (GordonW)
**	    needed include clpoll.h for sqs_us5/sqs_u42
**	18-Sep-1989 (fredv)
**		Forgot to move rusage.h after select.h.
**	23-nov-89 (blaise)
**		Added code to allow use of poll when it exists and when
**        	select doesn't. Preference is given to select when both
**		exist.
**	22-jan-90 (mikem)
**	    Added the file in from NSE line of code into development's
**	    "termcl" (6.3 line of code).  The RCP now always uses II_nap()
**	    to timout.
**	06-apr-92 (mikem)
**	    SIR #43445
**	    Changed the poll() implementation to round the number of usecs to
**	    the next highest millisecond increment for all positive timeout
**	    values.  This fixes the problem where very small timeout (ie. 
**	    1 or 10 usecs) were changed into non-blocking polls (0 ms. timeouts)
**	    rather than short sleeps.  Callers no longer need to know the 
**	    minimum resolution of II_nap().  Calls from 
**	    II_yield_cpu_time_slice() will usually pass very short timeouts, 
**	    but do not want them rounded to 0.
**	28-apr-92 (johnst)
**	    Fixed typo in previous change within xCL_043_SYSV_POLL_EXISTS ifdef,
**	    renaming "usec" reference to "usecs".
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	20-apr-94 (mikem)
**	    proto-ized and exported definitions through iinap.h.
**      03-jul-99 (podni01)
**          Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-jul-2002 (somsa01)
**	    Enabled for Windows.
**	26-jul-2002 (somsa01)
**	    For Windows, we may need to run WSAStartup(). Also, added
**	    Sleep(0) so that we give back some CPU to the OS.
**	4-Dec-2008 (kschendel) b122041
**	    Declare errno correctly (i.e. via errno.h).
*/

i4
II_nap(
i4 usecs)	/* micro seconds */
{
# ifdef xCL_020_SELECT_EXISTS
# ifdef xCL_021_SELSTRUCT
	SELLIST(FD_SETSIZE/NFDBITS, 1) xstruct1,xstruct2,xstruct3;
# endif
	struct timeval timo;
	i4 rc;

	timo.tv_sec  = usecs / MICRO_PER_SEC;
	timo.tv_usec = usecs % MICRO_PER_SEC;

# ifdef xCL_021_SELSTRUCT
	xstruct1.msgids[0] = 0xffffffff;
	FD_ZERO(&xstruct1);
	xstruct2.msgids[0] = 0xffffffff;
	FD_ZERO(&xstruct2);
	xstruct3.msgids[0] = 0xffffffff;
	FD_ZERO(&xstruct3);
	rc = select(0,&xstruct1,&xstruct2,&xstruct3,&timo);
# else
# if defined(rmx_us5) || defined(rux_us5)
        /*  Select calls poll on pym_us5.  If timo.tv_usec < 1000 microsec,
            poll gets a 0 timeout.  We want to force it the sleep at
            least briefly.
        */
        if (!timo.tv_sec && timo.tv_usec < 1000) timo.tv_usec = 1000;
# endif /* rmx_us5 */
# ifdef NT_GENERIC
	if (!WSAStartup_called)
	{
	    WORD	version = MAKEWORD(1,1);
	    WSADATA	startup_data;

	    WSAStartup(version, &startup_data);
	    WSAStartup_called = TRUE;
	}
# endif /* NT_GENERIC */
	rc = select(0, (fd_set *)NULL, (fd_set *)NULL, (fd_set *)NULL, &timo);
# endif

	if (rc == -1)
	{
# ifdef NT_GENERIC
	    /* Give some CPU back to the OS before returning. */
	    Sleep(0);
# endif /* NT_GENERIC */
	    return(-1);
	}

	return(0);

# else  /* xCL_020_SELECT_EXISTS */
# ifdef xCL_043_SYSV_POLL_EXISTS     
	i4 rc;                 
	struct pollfd   pollarg[1];
	long msecs;

	/* round microsecond input to the next highest millisecond boundary */
	if (usecs >= 0)
	    msecs = (usecs + MICRO_PER_MILLI - 1) / MICRO_PER_MILLI;
	else
	    msecs = 0;

	rc = poll(pollarg, 0, msecs);

	if (rc == -1)
	    return(-1);

	return(0);
# else
error: must implement II_nap
# endif	/* xCL_020_SYSV_POLL_EXISTS */
# endif /* xCL_020_SELECT_EXISTS */
}

/*{
** Name: II_yield_cpu_time_slice()	- yield timeslice to another process.
**
** Description:
**	This routine is used to try to force a context switch.  Currently
**	the most portable implementation is to call II_nap() with a very
**	short timeout (as provided by the "timeout" parameter).  
**	Alternate implementations depend specific OS features. If a future
**	OS provides a method for descheduling the current process and 
**	immediately putting it at the end of the run queue then that 
**	implementation should be added to this routine.  
**
**	This routine also gets the pid of the process which should be 
**	scheduled.  Currently this information is unused.  If an OS provided
**	a way for the server to schedule another process, then this information
**	could be used to schedule the process which is causing the block
**	in the first place.
**
**	Note that the resolution of II_nap() is machine dependent, so the input
**	timeout parameter can only be used as a hint to how long to wait.  
**	Depending upon the system calls available II_nap() may wait a minimum 
**	of 1 microsecond or 1 millisecond. 
**
** Inputs:
**	pid				A hint at the pid of process to yield 
**					time slice to.  There is no guarantee
**					that by the time this routine is called
**					that the process still exists.
**					0 - means pid unknown.
**
**      timeout				Suggested amount of time to pause if
**					the method of yield is to put process to
**					sleep.
** Outputs:
**	none.
**
**	Returns:
**	    VOID
**
** History:
**      06-apr-92 (mikem)
**	    SIR #43445
**          Created (substituted real routine for macro previously used in 
**	    CSp_semaphore()).
**	20-apr-94 (mikem)
**	    proto-ized and exported definitions through iinap.h.
*/
/* ARGSUSED */
VOID
II_yield_cpu_time_slice(
i4		pid,
i4		timeout)
{
    II_nap(timeout);
}
