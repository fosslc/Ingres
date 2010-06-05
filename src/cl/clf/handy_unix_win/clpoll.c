/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
# ifdef OS_THREADS_USED
# ifdef FD_SETSIZE
# undef FD_SETSIZE
# endif /* FD_SETSIZE */
#if defined(any_aix)
# define    FD_SETSIZE 32767
# else
# define    FD_SETSIZE 8192
# endif /* aix */
# endif /* OS_THREADS_USED */
#include    <gl.h>

# ifdef sos_us5
# undef FD_SETSIZE
# define    FD_SETSIZE 8192
# endif

#include    <er.h>
#include    <lo.h>
#include    <nm.h>
#include    <pm.h>
#include    <tr.h>
#include    <clconfig.h>
#include    <clpoll.h>
#include    <systypes.h>
#include    <clsigs.h>
#include    <fdset.h>
#include    <errno.h>
#include    <me.h>
#include    <meprivate.h>
#include    <pc.h>

#include    "bsshmio.h"   
PTR shm_addr();

#include    <sys/stat.h>

/*
**  NO_OPTIM = dg8_us5 || i64_aix
*/

#ifndef GCF63
#include    <cs.h>
#endif 

# if defined(dr6_us5) || defined(usl_us5) || defined(sparc_sol) \
  || defined(sui_us5) || defined(nc4_us5) || defined(sqs_ptx)
/*
** setjump/longjmp is used to fix to bug #44975
*/
# include <setjmp.h>
# endif /* dr6_us5, usl_us5, su4_us5, sui_us5, nc4_us5, sqs_ptx */

/* use select() unless only poll() is available */

# ifdef xCL_043_SYSV_POLL_EXISTS
# define USE_POLL
# else
# ifdef xCL_020_SELECT_EXISTS
# define USE_SELECT
# endif
# endif

# ifdef USE_POLL
# include <poll.h>
# ifdef xCL_STROPTS_H_EXISTS
# include <stropts.h>
# endif /* xCL_STROPTS_H_EXISTS */
# endif

# ifdef  xCL_021_SELSTRUCT
# include    <sys/select.h>
# endif

# ifdef xCL_015_SYS_TIME_H_EXISTS
# ifndef xCL_021_SELSTRUCT	/* AIX gets timeval from sys/select.h */
# include    <sys/time.h>
# endif
# endif

# ifdef xCL_006_FCNTL_H_EXISTS
# include   <fcntl.h>
# endif

# ifdef xCL_007_FILE_H_EXISTS
# include   <sys/file.h>
# endif

/*
**
**  Name: CLPOLL.C - File descriptor polling support
**
**  Description:
**	This file implements a file descriptor polling system which can be
**	used in either multi-thread or single-thread programs.  Its
**	original use is for multi-channel communication support for
**	the GC module.
**
**	Basically, CLpoll provides an abstraction of the BSD UNIX select
**	and (not yet) the ATT UNIX poll system calls.  This abstraction 
**	lends well to separation of modules which need to have one common 
**	focal point which may wait for an operation on a file descriptor.  
**	CLpoll is this focal point.
**
**  Usage:
**	A user of this system will register a UNIX file descriptor with
**	the CLpoll facility via the 'iiCLfdreg' call.  The user provides
**	the file descriptor, an operation, a function to call when the
**	operation is polled to be possible, and an argument to that
**	function.  This final argument is known as a closure because it
**	must allow access to all the information that may be needed to
**	close the gap between the registration of the file descriptor and
**	the function called later by the polling routine.  Once a file
**	descriptor is registered, 'iiCLpoll' should be called on a regular
**	basis to effect the polling and actual I/O operations.  In
**	a single-thread program, this poll will typically occur shortly
**	after the registration.  A multi-threaded program will typically
**	poll between threads by the dispatcher and as the heart of the
**	idle task.
**
**		iiCLfdreg() - The file descriptor registration routine
**		iiCLpoll()  - The polling routine
**		iiCLintrp() - signal handler to interrupt iiCLpoll() 
**
**  History:
**	17-jan-89 (seiwald)
**	    Added iiCLintrp() to interrupt iiCLpoll() reliably
**	9-jan-89 (seiwald)
**	    Version with static tables (instead of dynamically alloc'ed 
**	    ones).  iiCLfdreg() now registers for only one operation, not 
**	    an operation set.  iiCLfdreg() with a null function pointer
**	    clears the registration.  iiCLpoll() calls callbacks with the
**	    closure parameter only, not the file descriptor and operation
**	    mask.  iiCLpoll() takes a timeout of milliseconds.
**      22-dec-88 (anton)
**          Created and Specified.
**	    First functional version.
**	10-Nov-89 (seiwald)
**	    Timeouts are now provided on iiCLfdreg(), and completion
**	    routines are driven with a status indicating whether the
**	    operation was successful or timed out.  
**	    Timeout parameters are now i4.
**	    iiCLpoll now is VOID, since the caller of iiCLpoll should not 
**	    need to be aware of timeouts.  Errors in select() other than
**	    E_INTERRUPTED cause an abort().
**	31-Jan-90 (seiwald)
**	    Reverse direction of bit scanning on each call to 
**	    ii_CL_poll_call.  Scanning in one direction only favors
**	    low numbered file descriptors.
**	06-Mar-90 (seiwald)
**	    Fixed bit twiddling in ii_CL_poll_call by declaring variables
**	    as unsigned.
**	20-Mar-90 (seiwald)
**	    Use semaphores in 6.4 to modify global data.
**	02-Apr-90 (anton)
**	    Many more semaphore calls and MEreqmem should zero memory
**	25-May-90 (seiwald)
**	    Support for poll(2).
**	26-May-90 (seiwald)
**	    Use the #defines from clconfig.h for configuration.
**	5-Jul-90 (seiwald)
**	    New CL_poll_fd for clpoll.h.
**	15-Aug-90 (jkb)
**	    set fds[].events to 0 in iiCLintrp so poll will always exit
**	    when interrupted by a signal.
**	16-Aug-90 (jkb)
**	    use a variable rather than hardcoding SIGUSR2 in iiCLintrp.
**	22-Aug-90 (anton)
**	    MCT support - semaphores and signal drived iiCLpoll wakeup
**	    resulting from a iiCLfdreg.
**	23-Aug-90 (jkb)
**	    Add ifndef for xCL_068_USE_SIGACTION in iiCLintrp
**	05-Sep-90 (anton)
**	    Added a V of CL_misc_sem that was missing
**	    in recovery from out of memory condition
**	    Use symbol MT_CS to identify MCT specific code
**	    Handle EBADF error from select.  (MCT only situation)
**      04-oct-90 (jkb)
**          block SIGUSR2 signals when not in poll to protect other
**          system calls from interrupts.  Remove change to set
**          fds[].events to 0 in iiCLintrp this can cause events to be lost
**      14-dec-90 (mikem)
**	    bug #32958
**	    The changes to cshl.c to bump the CS_ADD_QUANTUM count depend on
**	    iiCLpoll() returning E_TIMEOUT when it times out (as documented).
**	    This return was eliminated at some point, and with this change
**	    it has been reinstated.
**	30-Jan-91 (anton)
**	    For MCT - add calls to lock out iiCLpoll when needed
**	26-Feb-91 (seiwald)
**	    When computing the new timer in ii_CL_timeout(), use the global
**	    CL_poll.timer.expire rather than a stack variable, so that 
**	    callback calls to iiCLfdreg() can properly manipulate the timer.
**	04 mar-91 (jkb) 
**	    Define MCT_polllock, set to true in CSdispatch (MCT only)
**	12-Aug-91 (seiwald)
**	    Reimplemented CLPOLL_xxxxFD() macros as CLPOLL_FD().
**      17-Sep-91 (hhsiung)
**          For Bull DPX/2 :
**          Timein value still bump to some craze value in iiCLpoll,
**          just simply check for extraordinary value and make it -1
**          ( for no time-out ).   Otherwise, select(2) will send out
**          E_AGAIN and iiCLpoll will call abort() to destroy Ingres
**          processes. Also, check -1 instead of FOREVER for Bull DPX/2
**          for the timeout value send to select.
**	30-Oct-91 (Sanjive/johnst)
**	    Integrate ingresug fix to add error handling in ii_CL_poll_poll() 
**	    for POLLERR,POLLHUP and POLLNVAL conditions. Adapt ingresug code
**	    to match inverted FD_SET/CLR logic for this stuff versus alogorithm
**	    used in ingresug code.
**	12-Nov-91 (seiwald)
**	    Reworked #include iffdef's, and keyed use of tims structure
**	    to (new) define USE_SELECT.
**	7-Nov-91 (seiwald)
**	    Made CL_poll_fd not attempt to dup -1.
**	13-Dec-91 (seiwald)
**	    Add handling of poll() error indications to ii_CL_poll_poll().
**	10-mar-92 (swm)
**	    Work-around for ICL platforms; the introduction of CL_poll_fd()
**	    caused socket connect() to hang with no occurence of an event on
**	    the listen side. Dup-ing a socket (via fcntl here) gave rise to
**	    this behaviour. So just return the original fd.
**	20-Mar-92 (gautam)
**	    1. Add FD_EXCEPTION support for #ifdef xCL_088_FD_EXCEPT_LIST 
**	    2. Changed include to <sys/file.h> to compile
**	    3. Reset interrupted to FALSE when interrupted, so a iiCLpoll() call
**	       is not wasted the next time through.
**	16-apr-92 (seiwald) bug #40490, #42884, #43509
**	    Cancelling an registered operation now guarantees that the
**	    callback will not get driven.  Previously, only the 'selected'
**	    bits determined what callbacks to drive; now the 'active' bits
**	    are examined as well.
**      24-jul-92 (mwc/swm)
**	    Bug #44975. On ICL SVR4 platforms, introduced conditional use
**	    of setjmp/longjmp around poll() to avoid the race condition
**	    where SIGUSR2 is caught after testing the CLpoll.interrupt flag
**	    but before the system call. The fix avoids the symptom of a
**	    90 second sleep in poll() caused by the race condition and no
**	    file descriptor activity.
**	    Correct 04-oct-90 change to re-block SIGUSR2 in the sigaction()
**          call with SIG_BLOCK - the current code specifies SIG_SETMASK
**          which would zap the kernel signal mask for any other blocked
**          signals (if there were any).
**	    The 10-mar-92 (swm) change should have been applied to
**	    clf/hdr/clpoll.h, so remove the change from here and add
**	    appropriate change to clpoll.h.
**    21-aug-92 (puru)
**	    Do not abort() if select() fails with EAGAIN error code.
**	    Previously, the server dies on Amdahl when multiple sessions
**	    try to connect to server simultaneously because select() fails
**	    due to lack of system resources with EAGAIN error code and
**	    iiCLpoll will abort() silently. Also just check for timein value
**	    and make it -1 if it is abnormally high ( for no time-out ).
**	7-sep-92 (gautam)
**	    CL_poll_fd rewritten. On analysis MAX_STDIO_FD on all 
**	    bad stdio systems was found to be 255. 
**	    Do not realloacate for every fd < MAX_STDIO_FD. 
**	    Instead reallocate only in the (MAX_STDIO_FD-5) - (MAX_STDIO_FD) 
**	    range
**	16-Sep-92 (eileenm)
**	    Some systems returns invalid fd (POLLVNAL) as well as 
**	    POLLHUP and POLLERR on poll read operations. Not recognizing
**	    the error code causes servers to loop when a front end 
**	    dies.
**	22-oct-92 (gautam)
**	    Support for II_CL_TRACE
**	30-dec-92 (mikem)
**	    su4_us5 port.  Changed cast of select in iiCLpoll(). Warning was 
**	    generated by su4_us5 compiler.  Also saved the errno associated
**	    with a failed select, before it could be possibly lost by system
**	    calls resulting from II_CLTRACE() calls.
**	4-jan-93 (peterk)
**	    in iiCLpoll() save errno after select() so if tracing is
**	    enabled (which has side effect of clearing errno), the error
**	    testing logic won't break.
**	11-Jan-93 (edg)
**	    Use gcu_get_tracesym() to get trace level.
**	18-Jan-93 (edg)
**	    Replace gcu_get_tracesym() with inline code.
**	21-jun-93 (shelby)
**	    Since the TI1500 doesn't have SIGACTION functions, ifdef
**	    around those calls.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	23-aug-93 (mikem)
**	    Remove debugging code from iiCLpoll() left over from previous 6.4 
**	    to 6.5 buf fix integration.  Calls to fstat from this code were 
**	    showing up relatively high in cpu profiles of the code.
**	24-Aug-93 (seiwald)
**	    Include sys/file.h, not file.h.  The check to set 
**	    xCL_007_FILE_H_EXISTS looks for sys/file.h.
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in iiCLfdreg().
**	08-nov-93 (johnst)
**	    Bug #58644
**	    Corrected unconditional use of unsigned long's to use the
**	    appropriate definition for fd_mask when manipulating file descriptor
**	    bit masks. This fixes problems on platforms, eg DEC alpha, where
**	    long's are 64-bit and not the usual 32-bit quantities.
**	    Added usl_us5 to platforms needing bug fix #44975 (see above).
**	03-mar-94 (swm)
**	    Bug #59869
**	    The fix to bug #58644 introduced bug #59869 on platforms where
**	    the file descriptor mask, fd_mask, is a signed type (eg. solaris);
**	    if fd_mask is signed right-shift operations on the sign bit
**	    introduce mask corruption.
**	    Changed type of xr and xw back to unsigned long and cast the
**	    relevant assignment expressions to avoid this problem. This
**	    change assumes that sizeof(unsigned long) >= sizeof(fd_mask).
**	    Added comment indicating the size assumption.
**	03-apr-95 (canor01)
**          Made ii_CLrealloc set only the added memory to zero
**          after the old block is copied in.
**	12-apr-95 (canor01)
**	    Added su4_us5 to platforms needing bug fix #44975 (see above).
**	05-may-95 (smiba01)
**	    Added support for dr6_ues based on dr6_us5
**	05-jun-95 (canor01)
**	    For shared memory batchmode connections, emulate the poll
**	    and select calls.  Do not block on the actual poll and select.
**	18-jul-95 (canor01)
**	    Clean up defines for MCT semaphores
**	03-Aug-95 (walro03/mosjo01)
**	    Added NO_OPTIM dg8_us5 to Ming hints.
**      10-nov-1995 (murf)
**              Added sui_us5 to all areas specifically defined with su4_us5.
**              Reason, to port Solaris for Intel.
**		The changes have been added to all sections od this file.
**	07-jun-1996 (bobmart)
**	    Added nc4_us5 and sqs_ptx for bug 44975
**	03-jun-1996 (canor01)
**	    Remove extraneous MCT code. With operating-system threads, iiCLpoll
**	    will be called only by single-threaded processes.  Multi-threaded
**	    servers use blocking IO.
**	25-oct-96 (nick)
**	    Don't INFTIM the timeout value when it exceeds 10^4 seconds and
**	    using poll(). #76458
**	11-nov-1996 (canor01)
**	    Store a private copy of the global CL_poll structure in local
**	    storage for each thread so the multi-threaded servers can use
**	    iiCLpoll.  Do not use longjmp() in multi-threaded server.
**      22-nov-1996 (canor01)
**          Changed names of CL-specific thread-local storage functions
**          from "MEtls..." to "ME_tls..." to avoid conflict with new
**          functions in GL.  Included <meprivate.h> for function prototypes.
**	 4-feb-97 (kch)
**	    On Unixware (usl_us5) the 10^11 millisecond constant used for
**	    the timeout comparison in ii_CL_poll_poll() (see previous change)
**	    is too large. This has been decreased to 2x10^9 on this platform
**	    only.
**	 7-feb-97 (kch)
**	    On Unixware (usl_us5), errno is not set to EINVAL if timeout is
**	    greater than 2x10^9 milliseconds.
**      07-mar-1997 (canor01)
**          Allow functions to be called from programs not linked with
**          threading libraries.
**	25-mar-1997 (canor01)
**	    When OS_THREADS_USED is defined, but executing without OS
**	    threads, use the global CL_poll structure for interrupts.
**      10-apr-97 (hweho01/popri01)
**          Cross-integrated UnixWare (usl_us5) fix (change #428664) from
**	    1.2/01: Now set timeout to -1 and don't return -1.
**	23-apr-1997 (canor01)
**	    Increase limit on file descriptors that can be polled by
**	    changing FD_SETSIZE from 1024 to 8192.
**	 7-feb-97 (kch)
**	    On Unixware (usl_us5), we now set timeout to -1 and don't return -1.
**	25-jun-97 (ocoro02)
**	    Added sos_us5 to defines for the above Unixware problem.
**	01-jul-1997 (bobmart)
**	    Added nc4_us5 to defines for the above Unixware problem.
**	16-jul-1997 (ocoro02)
**	    Added dr6_us5 to defines for the above Unixware problem.
**	12-sep-1997 (ocoro02)
**	    Added rmx_us5 to defines for the above Unixware problem.
**	17-sep-1997 (canor01)
**	    Change MEreqmem() to MEreqlng() to guarantee allocation of
**	    memory that may be larger than 64K.
**	19-sep-1997 (canor01)
**	    Allocate the callvec structures in thread-local storage
**	    so they will be freed at session end.
**      23-sep-1997 (allmi01)
**          Changed max value check for timeout on SCO Openserver to be the
**          max value that a i4 can hold (2^31).
**	19-feb-1999 (matbe01)
**	    Moved the #define for FD_SETSIZE after clsigs.h and fdset.h.
**	    Both invoke sys/time.h which in turn defines FD_SETSIZE. Therefore
**	    FD_SETSIZE must be undefined before being defined again. This
**	    resolves compile errors on AIX (rs4_us5).
**	    Added NO_OPTIM for AIX (rs4_us5) to prevent csphil from failing.
**	19-apr-1999 (hanch04)
**	    Use longs for bits shifting
**	10-may-1999 (walro03)
**	    Remove obsolete version string amd_us5, bu2_us5, bu3_us5, dr6_ues,
**	    dr6_uv1, dra_us5, sqs_us5.
**      03-jul-99 (podni01)
**          Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	23-Mar-2000 (jenjo02)
**	  o Backed out 19-Feb-1999 (matbe01) change and relocated
**	    define of FD_SETSIZE back to where it was. It -must- appear
**	    before the include of fdset.h because fdset.h generates
**	    the size of the fd_set structure based on the current
**	    value of FD_SETSIZE.
**	  o When calling static functions, pass pointer to known 
**	    CLpoll structure to avoid repeating ME_tls_get calls.
**	  o When scanning the sel_bits backward, ensure we start with the
**	    last fd_mask in the set, not the last plus one; if 
**	    "count" grows to become FD_SETSIZE, we'd be positioned on
**	    the word -following- the last entry in fd_set and would test 
**	    the bits of "count", get a hit, and wildly call a totally bogus
**	    (null) callback function. 
**	  o Init CL_poll_ptr->timer.expire = FOREVER when allocating
**	    thread local CLpoll structure.
**	  o When (re)allocating the callback structures, don't allow
**	    allocations larger than FD_SETSIZE; code assumes that
**	    fd_set is at least as large as sel->count!
**	  o Added hints to bound the bits of fd_set that need to be 
**	    looked at, shortening the time to find what we need.
**	  o Pass the number of set bits to ii_CL_poll_call(); when that
**	    many callbacks have been made, our work is done.
**	28-Mar-2000 (jenjo02)
**	    Refined fd hint usage to start with exact bit instead of
**	    hint fd's fd_mask. In ii_CL_poll_poll(), recompute first_fd,
**	    last_fd to cull out those which may have been deregistered.
**	17-May-2000 (jenjo02)
**	    Whoops, IIGCN uses FD 0, but "first" and "last" fd boundary
**	    hints interpreted FD 0 as "no FD", causing the FD registration
**	    and poll to be lost. Instead, use -1 to mean "no FD".
**      26-jun-2000 (loera01) Bug 101800
**          Cross-integrated from change 276901 in ingres63p:
**          Fix select() trace display, so that it returns a true dump of the
**          bit masks in select(), and only displays when an error is found.
**          Allow poll tracing when II_CLPOLL_TRACE is set, instead of
**          hard-coding a cltrace value.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**      09-Oct-2001 (hanal04) Bug 101849 INGSRV 1200 - BACKED OUT (hanje04)
**          Adjust change for bug 105265 to iiCLpoll() as the source of the
**          -ve timein values has been isolated;
**               timein = CL_poll_ptr->timer.expire - CL_poll_ptr->timer.now
**          Mutex protected references to the CLpoll structure's
**          now and expire fields to ensure a consistent update / reference.
**          Trap the case where timeout parameter is -ve.
**	06-jun-2002 (devjo01)
**	    Corrected problems in which because of sign extension,
**	    some of the algorithms used to manipulate the fd bit
**	    sets would operate on bits past the end of the set,
**	    causing catastophic SEGV's on Tru64 when an attempt
**	    was made to jump through a function pointer in memory
**	    that was actually past the end of an array.
**	06-sep-2002 (somsa01)
**	    Corrected the way we reference fds_bits, since "long" is of
**	    different sizes between 32-bit and 64-bit. This fixes a 64-bit
**	    server from hanging in CLpoll() on HP during startup.
**      09-Oct-2001 (hanal04) Bug 101849 INGSRV 1200
**          Adjust change for bug 105265 to iiCLpoll() as the source of the
**          -ve timein values has been isolated;
**               timein = CL_poll_ptr->timer.expire - CL_poll_ptr->timer.now
**          Mutex protected references to the CLpoll structure's
**          now and expire fields to ensure a consistent update / reference.
**          Trap the case where timeout parameter is -ve.
**      11-Jun-2002 (hanal04) Bug 107953 INGREP 115, Bug 107942 INGCBT 422
**          Remove the above change for bug 101849 and revert to correcting
**          The negative timein values only if we use select().
**	19-mar-2003 (somsa01)
**	    In iiCLfdreg() and iiCLpoll(), return FALSE when MEreqmem() of
**	    CL_poll_ptr fails.
**      22-Apr-2003 (huazh01)
**          For rs4_us5, increase the number of file descriptors that 
**          can be polled by changing FD_SETSIZE from 8192 to 32677. 
**          This prevents select() from returning EBADF. 
**          Bug 110117, INGSRV 2228.
**      23-Jul-2003 (wanfr01)
**          Bug 109763, INGSRV 2117.
**          For sos_us5, set FD_SETSIZE to 8192 (versus the default of 
**	    256).  This prevents a segv when a server is started with 
**	    a high connect limit on SCO.
**	23-Jun-2004 (rajus01) Bug # 112549, INGNET 144.
**	    Set the default FD_SETSIZE to 4096 for SCO. Since OS_THREADS_USED
**	    is not applicable to SCO, the patch 8517 Ingres build had used 
**	    value of 150 for FD_SETSIZE which caused crashing of IIGCC when
**	    number of open concurrent sessions exceeded 75.
**	10-Sep-2004 (sheco02)
**	    Fix the x-integration mistake for change 471716. Just back it out.
**	22-Nov-2004 (hweho01)
**	    Remove rs4_us5 from NO_OPTIM list. 
**	23-Sep-2005 (hanje04)
**	    Re-order setting of FD_SETSIZE to quiet compiler warnings.
**	20-Oct-2005 (hanje04)
**	    Prototype ii_CL_poll_poll to stop compiler errors with GCC 4.0.
**	11-Jun-2007 (kschendel, modified 21-Dec-2007 by toumi01)
**	    Temporary change to relieve the 1024 fd limit on Linux.
**	01-apr-2008 (toumi01)
**	    Activate 8192 fd limit for Linux change.
**	4-Dec-2008 (kschendel) b122041
**	    Declare errno correctly (i.e. via errno.h).
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/


#if defined(LNX)
/*
** Temporary change to relieve the 1024 fd limit on Linux.
**
** - this change also requires a modification to the #if logic at the top
**   of clfd.c to assign a value of 8192 to CL_FD_SETSIZE for LNX.
**
** - to run a server with 8192 FDs on Linux be sure to set the file limit
**   before starting Ingres (for example "ulimit -n 8192" as root and then
**   su to the ingres user id).
**
** Redefine fd_set to be a CL_FD_SETSIZE bit-set; it's wired as a 1024
** bit-set in Linux's sys/types.h. FIXME: the "right" way to do this is
** to use the epoll() API for fd polling and eliminate the Ingres use of
** fd_set for management of polling callbacks.
*/
#undef FD_SETSIZE
#define FD_SETSIZE 8192

typedef struct {
    fd_mask fds_bits[FD_SETSIZE / (8*sizeof(fd_mask))];
} my_fd_set;
/* This define will override the fd_set typedef in sys/select.h */
#define fd_set my_fd_set
#endif /* LNX fd_set override */

static i4  cltrace = 0;
static i4  callvec_incr = 8;

# if defined(dr6_us5) || defined(usl_us5) || defined(sparc_sol) \
  || defined(sui_us5) || defined(nc4_us5) || defined(sqs_ptx)
/*
** setjump/longjmp environment for fix to bug #44975
*/
static jmp_buf jmp_env;

/*
** latch to control when we need to longjmp() from SIGUSR2 handler
*/
static i4     use_longjmp = 0;


# endif /* dr6_us5, usl_us5, su4_us5, sui_us5, nc4_us5, sqs_ptx */

# define CLTRACE(n) if( cltrace >= n )(void)TRdisplay

# define	FOREVER		MAXI4
# define	NEVER		-1


/*
** Name: struct CL_poll - private data for iiCLpoll()
**
** Description:
**	CL_poll contains the table of callbacks and bit list of registered
**	file descriptors for read and write calls.  This structure's size
**	is determined at compile time, but could be allocated in a future
**	version.
**
** History:
**	9-Jan-89 (seiwald)
**	    Created.
*/

static struct _CLpoll
{
	/*
	** interrupted - iiCLintrp() was called to interrupt CLpoll()
	*/

	bool init;		/* timeout set */
	bool interrupted;
	i4  read_write; 
	i4  bat_fd;         /* batch mode fd              */
	PTR  bat_shmseg;     /* batch mode shared mem ptr  */

	/*
	** read, write - structure for read and write components of poll
	*/

#ifdef xCL_088_FD_EXCEPT_LIST
#define    NUM_SEL      3
#else
#define    NUM_SEL      2
#endif
	struct selector 
	{
		/*
		** act_bits - bit mask of active descriptors
		** sel_bits - copy of bits to hand to select
		** *callvec - callback and closure for each fd
		** count - last of callvec
		** first_fd - lowest registered fd
		** last_fd -  highest registered fd
		*/

		fd_set		act_bits;
		fd_set		sel_bits;
		int		count;
		int		first_fd; /* boundary hints */
		int		last_fd;  /* for bit scans  */

		struct calls {
			VOID		(*func)();
			PTR		closure;
			i4		timeout;	/* -1 = forever */
		} *callvec;

	} op[ NUM_SEL ];

	struct
	{
		i4		now;		/* current time - reset often */
		i4		expire;		/* now...FOREVER */
# ifdef USE_SELECT
		struct timeval 	tims;		/* select() call time */
# endif
	} timer ;

# ifdef	USE_POLL
	struct pollfd	fds[ FD_SETSIZE ];
# endif

} CL_poll ZERO_FILL;

static bool CL_poll_init = FALSE;

# ifdef OS_THREADS_USED
static ME_TLS_KEY CLpoll_key = 0;
static ME_TLS_KEY CLcallvec_key[ NUM_SEL ] ZERO_FILL;
# endif /* OS_THREADS_USED */

static i4
ii_CL_poll_poll( struct _CLpoll  *CL_poll_ptr,
		int	minfd,
		int	maxfd,
		int 	timeout,
		int	*poll_errno
		);


/*{
**
*/

static PTR
ii_CLrealloc( ptr, size, old, new, status )
PTR	ptr;
i4	size;
i4	old, new;
STATUS	*status;
{
	PTR	newptr;
	i4     oldsize, newsize;

	oldsize = old * size;
	newsize = new * size;
	
        newptr = MEreqmem( 0, newsize, FALSE, status );

	/* Basically realloc the array. */

	if ( newptr )
		MEfill( (newsize - oldsize), 0, newptr + oldsize );

	if( newptr && old )
	{
		MEcopy( ptr, oldsize, newptr );
		MEfree( ptr );
	}

	CLTRACE(1)( "ii_CLrealloc ptr=%p size=%d old=%d new=%d oldsize=%d newsize=%d newptr=%p\n",
		ptr, size, old, new, oldsize, newsize, newptr );

	return newptr;
}


/*{
** Name: iiCLfdreg - register a file descriptor for polling within the CL
**
** Description:
**	Register a file descriptor to be polled for possible I/O
**	A callback routine will be run on behalf of the user when possible I/O
**	is detected by the iiCLpoll function.
**
**	The callback routine should not block in order to maintain proper
**	asyncronous behavior.  Nor should the routine rely on being run in
**	any thread as no thread may be running when a poll occurs.  It may
**	not make session orented calls such as CSget_sid.
**
**	Use of the stack in the callback function is also limited even
**	more than the standard thread stack limit as the callback function's
**	stack may be any stack of any thread.
**
**	CS_find_events or iiCLpoll may not be called from the the callback
**	routine or its decendants.
**
**	If a file descriptor function is reregistered the previous 
**	registration is superceeded.
**
**	If a file descriptor function is reregistered with null function
**	pointer, the registration is cancelled.
**
**
** Inputs:
**	fd				file descriptor to poll
**
**	op				operation flag
**				FD_READ		(poll for reading)
**				FD_WRITE	(poll for writing)
**
**	func				Callback function
**				Callback is called with: (*func)(closure)
**
**	closure				closure pointer
**				This is used to allow whatever data structures
**				the callback routine needs to be passed
**				along.  This pointer is not dereferenced by
**				the file descriptor registration system
**				and may be NULL.
**
** Outputs:
**	None.
**
**	Returns:
**	    E_DB_OK
**	    FAIL		Needed memory would could not get from ME
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    previous regitrations may be superceeded.
**
** History:
**	22-dec-88 (anton)
**	    Working Spec and first working version.
**      12-dec-88 (anton)
**          First Spec.
**	02-Apr-90 (anton)
**	    Add CL_misc_sem calls
**	05-Sep-90 (anton)
**	    Added a V of CL_misc_sem that was missing
**	    in recovery from out of memory condition
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	23-Mar-2000 (jenjo02)
**	    "sel->count" cannot be extended beyond the number of
**	    bits in fd_set (FD_SETSIZE)!
*/

# define ROUNDUP( x, n ) ( ( (x) | ( (n) - 1 ) ) + 1 )

STATUS
iiCLfdreg( fd, op, func, closure, timeout )
register int	fd;
i4		op;
VOID		(*func)();
PTR		closure;
i4		timeout;
{
        i4      batch_mode_ind = 0;
	register struct selector *sel;
	struct _CLpoll *CL_poll_ptr;
	i4	num_sel;
	STATUS	status;

	/* Initialization */

# ifdef OS_THREADS_USED
	if ( CLpoll_key == 0 )
	{
	    ME_tls_createkey( &CLpoll_key, &status );
	    ME_tls_set( CLpoll_key, NULL, &status );
	}
	if ( CLpoll_key == 0 )
	{
	    /* not linked with threading library */
	    CLpoll_key = -1;
        }
	if ( CLpoll_key == -1 )
	{
	    CL_poll_ptr = &CL_poll;
	}
	else
	{
	    ME_tls_get( CLpoll_key, (PTR *)&CL_poll_ptr, &status );
	    if ( CL_poll_ptr == NULL )
	    {
    	        CL_poll_ptr = (struct _CLpoll *) MEreqmem( 0, 
						    sizeof(struct _CLpoll),
						    TRUE, NULL );
		if (!CL_poll_ptr)
		{
		    TRdisplay("iiCLfdreg FATAL: Memory allocation failed for CL_poll_ptr.\n");
		    return(FAIL);
		}

    	        ME_tls_set( CLpoll_key, (PTR)CL_poll_ptr, &status );
		/* Initialize thread's timer expiration */
		CL_poll_ptr->timer.expire = FOREVER;

		/* Init lower fd boundary hint */
		num_sel = NUM_SEL;
		while ( num_sel-- )
		{
		    CL_poll_ptr->op[num_sel].first_fd = -1;
		    CL_poll_ptr->op[num_sel].last_fd = -1;
		}
	    }
	}
# else /* OS_THREADS_USED */
	CL_poll_ptr = &CL_poll;
# endif /* OS_THREADS_USED */
	if( !CL_poll_init )
	{
		char	*trace;

		CL_poll_init = TRUE;
		CL_poll_ptr->timer.expire = FOREVER;

		/* Init lower fd boundary hint */
		num_sel = NUM_SEL;
		while ( num_sel-- )
		{
		    CL_poll_ptr->op[num_sel].first_fd = -1;
		    CL_poll_ptr->op[num_sel].last_fd = -1;
		}

		NMgtAt( "II_CLPOLL_TRACE", &trace );
		if ( !(trace && *trace) 
		     && PMget("!.clpoll_trace_level", &trace) != OK )
		{
		    cltrace = 0;
		}
		else
		{
		    cltrace = atoi(trace);
		}
                if ( PMget( "!.connect_limit", &trace ) != OK )
                    callvec_incr = 8;
                else
                    callvec_incr = 8 + atoi( trace );
	}

	CLTRACE(1)( "iiCLfdreg fd=%d op=%d func=%p closure=%p timeout=%d\n",
		fd, op, func, closure, timeout );

        if (fd < 0)
	{
	  batch_mode_ind     = fd;
	  fd                *= -1;
	  CL_poll_ptr->bat_fd     = fd;
	  CL_poll_ptr->read_write = (op == FD_READ) ? 1 : 2; 
	  if (CL_poll_ptr->bat_shmseg == NULL)
	    CL_poll_ptr->bat_shmseg = shm_addr(NULL, 2); /* attach shared memory */
        }
        
	/* Point sel at the appropriate structure.  */

	switch( op )
	{
	case FD_READ:	num_sel = 0; break;
	case FD_WRITE:	num_sel = 1; break;

#ifdef xCL_088_FD_EXCEPT_LIST
	case FD_EXCEPT:	num_sel = 2; break;
#endif  /* xCL_088_FD_EXCEPT_LIST */

	default:	return FAIL;
	}
        sel = &CL_poll_ptr->op[num_sel];

# ifdef OS_THREADS_USED
	/*
	** this memory is always allocated, so no need for 
	** a non-threaded default.
	*/
        if ( CLcallvec_key[num_sel] == 0 )
        {
            ME_tls_createkey( &CLcallvec_key[num_sel], &status );
            ME_tls_set( CLcallvec_key[num_sel], NULL, &status );
        }
        if ( CLcallvec_key[num_sel] == 0 )
        {
            /* not linked with threading library */
            CLcallvec_key[num_sel] = -1;
        }
	/*
	** Don't need to ME_tls_get the callvec; it's zero (not yet
	** allocated), or pointed to by sel->callvec.
	*/
# endif /* OS_THREADS_USED */

	/*
	** Extend call list if fd is bigger than previous ones,
	** but only if we're registering an fd. If deregistering,
	** we won't reference the call list.
	*/
	if( fd >= sel->count && func )
	{
		STATUS status;
		i4 newfd = ROUNDUP( fd, callvec_incr );

		/* Cannot allow call list larger than max fd's */
		if ( newfd > FD_SETSIZE )
		    newfd = FD_SETSIZE;

		if ( fd >= newfd )
		{
		    /* This should not be possible, but... */
		    TRdisplay("iiCLfdreg FATAL: fd %d surpasses max fd %d\n",
			fd, newfd);
		    return(FAIL);
		}

		sel->callvec = (struct calls *)ii_CLrealloc( 
			(PTR)sel->callvec, sizeof( struct calls ),
			sel->count, newfd, &status );

		if( status != OK )
		    return status;

# ifdef OS_THREADS_USED
		ME_tls_set( CLcallvec_key[num_sel], (PTR)sel->callvec, &status );
# endif /* OS_THREADS_USED */

		sel->count = newfd;
	}

	/* Register or clear registration, as indicated by non-null func. */

	if( func )
	{
		if( timeout != -1 )
		{
			timeout += CL_poll_ptr->timer.now;

			if( timeout < CL_poll_ptr->timer.expire )
				CL_poll_ptr->timer.expire = timeout;
		}

		sel->callvec[ fd ].func = func;
		sel->callvec[ fd ].closure = closure;
		sel->callvec[ fd ].timeout = timeout;

		FD_SET( fd, &sel->act_bits );

		/* Set lowest numbered registered fd "hint" */
		if ( sel->first_fd < 0 || fd < sel->first_fd )
		    sel->first_fd = fd;
		if ( fd > sel->last_fd )
		    sel->last_fd = fd;

	} else {
		CL_poll_ptr->read_write = 0;
		FD_CLR( fd, &sel->act_bits );
		FD_CLR( fd, &sel->sel_bits );
	}

	return OK;
}


/*
** Name: iiCLintrp() - interrupt CLpoll
**
** Description:
**	Interrupts CLpoll().  For a signal handler to interrupt
**	CLpoll() reliably, it must call iiCLintrp() to cover these
**	three cases:
**
**	o	Before CLpoll calls select() - iiCLintrp() sets 
**		CL_poll.interrupted, which CLpoll() checks before 
**		calling select().
**
**	o	Before CLpoll calls select() but after it checks 
**		CL_poll.interrupted - iiCLintrp() zeros the CL_poll.tims 
**		so that select() will not block.
**
**	o	While CLpoll is in the select() - the delivery of the 
**		signal will interrupt select().  iiCLintrp() is a 
**		no-op then.
**
** Called by:
**	from above, presumably on receipt of a signal.
*/

void
iiCLintrp(signum)
i4	signum;
{

#if !defined xCL_011_USE_SIGVEC && !defined xCL_068_USE_SIGACTION
        /*
        ** re-establish signal handler
        */
        (void)signal(signum, iiCLintrp);
#endif

# ifdef USE_SELECT
	CL_poll.timer.tims.tv_sec = 0;
	CL_poll.timer.tims.tv_usec = 0;
# endif /* USE_SELECT */

	CL_poll.interrupted = TRUE;
	
# if defined(dr6_us5) || defined(usl_us5) || defined(sparc_sol) || \
     defined(sui_us5) || defined(nc4_us5) || defined(sqs_ptx)
	/*
	** if latch is set, longjmp to avoid race condition with poll(),
	** bug #44975
	*/
	if (use_longjmp)
	{
	    longjmp(jmp_env,0);
	}

# endif /* dr6_us5, usl_us5, su4_us5, sui_us5, nc4_us5, sqs_ptx */
}


/*{
** Name: iiCLpoll - poll registered file descriptors
**
** Description:
**	check registered file descriptors for possible I/O
**	and do callback functions as needed.
**	See iiCLfdreg for details.
**
** Inputs:
**	timeout			timeout for poll
**					-1 : block until operation
**					0  : no delay poll
**					>0 : delay in milliseconds
**
** Outputs:
**	timeout			timeout may be reduced by time spent
**				waiting for an operation. (not implemented)
**
**	Returns:
**	    OK
**	    E_TIMEOUT		poll timer expired
**	    E_INTERRUPTED	poll was interrupted by an EXception.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Callback function(s) run. (See iiCLfdreg for details)
**
** History:
**      12-dec-88 (anton)
**          First Spec.
**	22-dec-88 (anton)
**	    First working version & detailed spec.
**	9-jan-89 (seiwald)
**	    Revamped.
**      14-dec-90 (mikem)
**	    bug #32958
**	    The changes to cshl.c to bump the CS_ADD_QUANTUM count depend on
**	    iiCLpoll() returning E_TIMEOUT when it times out (as documented).
**	    This return was eliminated at some point, and with this change
**	    it has been reinstated.
**	30-dec-92 (mikem)
**	    su4_us5 port.  Changed cast of select 4th argument to match system
**	    prototype (fd_set * rather than int *).  Warning was generated by
**	    su4_us5 compiler.  Also saved the errno associated
**	    with a failed select, before it could be possibly lost by system
**	    calss resulting from II_CLTRACE() calls.
**	14-apr-93 (vijay)
**	    some generic code was inside the else part of ifdef
**	    CL_FD_EXCEPT_LIST. Move it out.
**	23-aug-93 (mikem)
**	    Remove debugging code left over from previous 6.4 to 6.5 buf fix
**	    integration.  Calls to fstat from this code were showing up 
**	    relatively high in cpu profiles of the code.
**	25-oct-96 (nick)
**	    Tidy handling of poll()/select() failure.
**      07-Sep-2001 (hanal04) Bug 105265 INGSRV 1496
**          Log a TRdisplay before calling abort().
**          Trap and correct negative timein values which would cause
**          select() to return EINVAL. Limit associated DBMS log entries
**          in order to prevent disk fill.
**      11-Jun-2002 (hanal04) Bug 107953 INGREP 115, Bug 107942 INGCBT 422
**          Remove the above change for bug 101849 and revert to correcting
**          the negative timein values only if we use select().
**          Note that the negative value comes from:
**          timein = CL_poll_ptr->timer.expire - CL_poll_ptr->timer.now;
**	05-Mar-2008 (thaju02)
**	    Long-standing session may hang in poll(). Negative timein
**	    calculated, overriding zero timeout. (B120065)
*/

STATUS
iiCLpoll( timeout )
i4	*timeout;
{
	int		i;
	int		minfd, maxfd;
	int		timein = *timeout;
	struct timeval	*timo, batch_timeo;
	STATUS		ret_val = OK;
	int		select_errno = 0;
	struct _CLpoll  *CL_poll_ptr;
	STATUS		status;
        static i4	time_resets = 0;
	struct selector *s;

# ifdef OS_THREADS_USED
	if ( CLpoll_key == 0 )
	{
	    ME_tls_createkey( &CLpoll_key, &status );
	    ME_tls_set( CLpoll_key, NULL, &status );
	}
        if ( CLpoll_key == 0 )
        {
            /* not linked with threading library */
            CLpoll_key = -1;
        }
        if ( CLpoll_key == -1 )
        {
            CL_poll_ptr = &CL_poll;
        }
        else
        {
	    ME_tls_get( CLpoll_key, (PTR *)&CL_poll_ptr, &status );
	    if ( CL_poll_ptr == NULL )
	    {
    	        CL_poll_ptr = (struct _CLpoll *) MEreqmem( 0, 
						    sizeof(struct _CLpoll),
						    TRUE, NULL );
		if (!CL_poll_ptr)
		{
		    TRdisplay("iiCLpoll FATAL: Memory allocation failed for CL_poll_ptr.\n");
		    return(FAIL);
		}

    	        ME_tls_set( CLpoll_key, (PTR)CL_poll_ptr, &status );
		/* Initialize thread's timer expiration */
		CL_poll_ptr->timer.expire = FOREVER;

		/* Init lower fd boundary hint */
		i = NUM_SEL;
		while ( i-- )
		{
		    CL_poll_ptr->op[i].first_fd = -1;
		    CL_poll_ptr->op[i].last_fd = -1;
		}
	    }
	}
# else /* OS_THREADS_USED */
        CL_poll_ptr = &CL_poll;
# endif /* OS_THREADS_USED */

	/*
	** Copy the range of active fd bits to the selection bits
	** and determine minfd and maxfd values for all selectors.
	*/
	minfd = -1;
	maxfd = -1;

	for( i = NUM_SEL; i--; )
	{
	    struct selector *s = &CL_poll_ptr->op[ i ];
		
	    if ( s->first_fd >= 0 )
	    {
		fd_mask *sel = &((fd_mask *)&s->sel_bits.fds_bits)
				[ s->first_fd / NFDBITS ];
		fd_mask *act = &((fd_mask *)&s->act_bits.fds_bits)
				[ s->first_fd / NFDBITS ];
		i4	n    = (s->last_fd  / NFDBITS) -
			       (s->first_fd / NFDBITS);

		for ( ; n-- >= 0 ; *sel++ = *act++ );
		
		if ( minfd < 0 || s->first_fd < minfd )
		    minfd = s->first_fd;
		if ( s->last_fd  > maxfd )
		    maxfd = s->last_fd;
	    }
	}

	/* construct timeout structure for select */

	if( timein == -1 || CL_poll_ptr->timer.expire - CL_poll_ptr->timer.now < timein )
        {
# ifdef USE_POLL
	    if ((timein >= 0) && ((CL_poll_ptr->timer.expire - 
				CL_poll_ptr->timer.now) < 0))
		TRdisplay("iiCLpoll: timeout=%d override calcd timein=%d \
  expire=%d   now=%d\n",
			timein,
			(CL_poll_ptr->timer.expire - CL_poll_ptr->timer.now),
			CL_poll_ptr->timer.expire,
			CL_poll_ptr->timer.now);
	    else
# endif  /* USE_POLL */
	    timein = CL_poll_ptr->timer.expire - CL_poll_ptr->timer.now;
            
        }

	CLTRACE(2)( "iiCLpoll: now %d timer %d, minfd %d maxfd %d\n", 
		CL_poll_ptr->timer.now, timein, minfd, maxfd );

	/* do the select call */

# ifdef USE_POLL

	i = ii_CL_poll_poll( CL_poll_ptr, minfd, maxfd, timein,
				&select_errno );

        if ( CL_poll.interrupted )
        {
	    CL_poll.interrupted = FALSE;
	    if ( i < 0 )
	    {
		ret_val = E_INTERRUPTED;
		goto done;
	    }
        }


# endif /* USE_POLL */

# ifdef USE_SELECT

	switch( timein )
	{
	case FOREVER:
	    timo = NULL;
	    break;
	case 0:
	    CL_poll_ptr->timer.tims.tv_sec = 0;
	    CL_poll_ptr->timer.tims.tv_usec = 0;
	    timo = &CL_poll_ptr->timer.tims;
	    break;
	default:
            if(timein < 0 )
            {
		/* Don't allow a -ve value to be passed to select () */
                timein = 200;
            }
	    CL_poll_ptr->timer.tims.tv_sec = timein / 1000;
	    CL_poll_ptr->timer.tims.tv_usec = ( timein % 1000 ) * 1000;
	    timo = &CL_poll_ptr->timer.tims;
	    break;
	}

	/* check if interrupted before we set up tims */

	if( CL_poll.interrupted )
	{
	    CL_poll.interrupted = FALSE;
	    ret_val =  E_INTERRUPTED;
	    goto done;
	}

        if ( CL_poll_ptr->bat_shmseg != NULL) 
        {
          MEfill(sizeof(struct timeval), 0, &batch_timeo);
          timo= &batch_timeo;
        }

#ifdef xCL_088_FD_EXCEPT_LIST
	i = select( maxfd+1,
		&CL_poll_ptr->op[ 0 ].sel_bits, 
		&CL_poll_ptr->op[ 1 ].sel_bits, 
		&CL_poll_ptr->op[ 2 ].sel_bits, 
		timo );
#else

	i = select( maxfd+1,
		&CL_poll_ptr->op[ 0 ].sel_bits, 
		&CL_poll_ptr->op[ 1 ].sel_bits, 
		(fd_set *)NULL, timo );

#endif  /* #ifdef xCL_088_FD_EXCEPT_LIST */

	select_errno = errno;

	CLTRACE(1)( "select returns %d; errno = %d\n", i, select_errno );

        if ((select_errno != OK) && (cltrace >= 4 ))
        {
            i2 j=0,k,m,n;

            for (n = 0; n < 2; n++)
            {
                for (k = 0; k < 2; k++)
                {
                    TRdisplay ("%s %s\n",k ? "Write" : "Read", n ?
                        "act_bits:" : "sel_bits:");
                    for( m = 0; m < (FD_SETSIZE / NFDBITS); m++)
                    {
                        TRdisplay ("%x ", n ?
                            &(CL_poll.op[k].act_bits.fds_bits)[m] :
                            &(CL_poll.op[k].sel_bits.fds_bits)[m]);
                        j++;
                        if (!(j % 8))
                            TRdisplay("\n");
                    }
                    TRdisplay("\n");
                }
            }
        }


# endif /* USE_SELECT */

        if ( CL_poll_ptr->bat_shmseg != NULL) 
	{
	  GC_BATCH_SHM  *shmptr= (GC_BATCH_SHM *)CL_poll_ptr->bat_shmseg;
	  GC_BATCH_SHM  *oshmptr= shmptr;
	  register  struct selector *s;
	  register  fd_mask *sel;
	  i4        idx;

	  idx = shm_getcliservidx();
	  if ( idx )
	      shmptr = (GC_BATCH_SHM *)((char*)oshmptr+(oshmptr->bat_pages));
	  else
	      oshmptr = (GC_BATCH_SHM *)((char*)shmptr+(shmptr->bat_pages));
	  if (CL_poll_ptr->read_write && 
	      PCis_alive(oshmptr->process_id) == FALSE)
	      shmptr->bat_signal = 1;

	  if (shmptr->bat_signal > 0 &&
	      CL_poll_ptr->read_write == 1)                 /* read  mode */
	  {
	    s = &CL_poll_ptr->op[0];
	    FD_SET(CL_poll_ptr->bat_fd, &s->sel_bits);
	    i++;
          }
          else
	  if (shmptr->bat_signal > 0 &&
	      CL_poll_ptr->read_write == 2)                /* write mode */
	  {
	    s = &CL_poll_ptr->op[1];
	    FD_SET(CL_poll_ptr->bat_fd, &s->sel_bits);
	    i++;
          }
	  else
	  if (shmptr->bat_signal <= 0)
	  {
	      /*II_nap(1);*/
	  }
        } /* if ( CL_poll_ptr->bat_shmseg != NULL) */

	CLTRACE(1)( "-------- %d fds\n", i );

	/*
	** Check status of select.  If select timed out (i==0), we bump
	** "now" by the timeout interval to reflect the current time.
	** If select succeeded, we bump "now" by the lapsed time: since
	** a real timer would be too expensive, we estimate that to be
	** 10ms per fd.  Finally, if select failed for a reason other than
	** a signal (EINTR), we have programming error, and we raise a
	** signal with abort().
	*/

	if ( i == 0 )
	{
	    CL_poll_ptr->timer.now += timein;
	    ret_val = E_TIMEOUT;
	}
	else if ( i > 0 )
	{
	    CL_poll_ptr->timer.now += i * 10;
	}
	else if (select_errno == EINTR)
	{
	    CL_poll.interrupted = FALSE;
	}
        else if ( (select_errno != EINTR) && (select_errno != EAGAIN) )
	{
            CLTRACE(2) ("iiCLpoll: select() failed errno %d\n", select_errno);
    	    TRdisplay("iiCLpoll: select() failed errno %d\n", select_errno);
	    abort();
	}

	/*
	** Drive completion routines.  Call ii_CL_poll_call to drive
	** the successful selects, and ii_CL_timeout to drive file
	** i/o's that timed out.
	*/
	if ( i > 0 )
	    ii_CL_poll_call( CL_poll_ptr, i );

	if ( CL_poll_ptr->timer.now >= CL_poll_ptr->timer.expire )
	    ii_CL_timeout( CL_poll_ptr );

done:

	return(ret_val);
}


/*
** Name: ii_CL_poll_call() - call user callback functions
**
** Description:
**	Scans the bit list select(2) returns for file descriptors ready 
**	for I/O and calls the callback routines.  This routine is called
**	once for the read list and once for the write list.
**
**	23-Mar-2000 (jenjo02)
**	  o Pass number of set bits to ii_CL_poll_call(); when that
**	    many callbacks have been fired, our work is done and
**	    we don't need to keep scanning the remainder of fd_set.
**	  o Use fd hints to start fd_set scan instead of starting
**	    at zero or the end.
**	  o When scanning the sel_bits backward, ensure we start with the
**	    last fd_mask in the set, not the last plus one; if 
**	    "count" grows to FD_SETSIZE, we'd be positioned on
**	    the word -following- the last entry in fd_set and would test 
**	    the bits of "count", get a hit, and take a wild branch to
**	    a totally bogus (null) callback function.
**	28-Mar-2000 (jenjo02)
**	    Refine hints, start with exact first_fd, last_fd bit.
**	06-jun-2002 (devjo01)
**	    Prevent 'k' from exceeding bit array bounds.
*/

# define STEPS 8
# define MASK ( (long)( 1L << STEPS ) - 1L )

ii_CL_poll_call( 
struct _CLpoll *CL_poll_ptr,
int	bits_set)
{
    int o;
    static int back = 0;
    GC_BATCH_SHM  *shmptr;
    GC_BATCH_SHM  *oshmptr;
    i4     idx;
    int	   local_back;
    STATUS status;

    shmptr = (GC_BATCH_SHM *)CL_poll_ptr->bat_shmseg;
    oshmptr = (GC_BATCH_SHM *)CL_poll_ptr->bat_shmseg;

    if (shmptr != NULL)
    {
        idx = shm_getcliservidx();
	if ( idx )
	    shmptr = (GC_BATCH_SHM *)((char*)oshmptr+(oshmptr->bat_pages));
	else
	    oshmptr = (GC_BATCH_SHM *)((char*)shmptr+(shmptr->bat_pages));
    }

    local_back = back;
    for( o = NUM_SEL; o--; )
    {
	register struct selector *s;
	register fd_mask *sel;
	register fd_mask *act;
	register int j;
	register int k;
	register int h;
	int i, l;
	
	s = &CL_poll_ptr->op[ o ];

	if ( s->first_fd < 0 )
	    continue;

	if ( !local_back )
	{
	    /* Start with the lowest registered fd */
	    for ( i  = s->first_fd & ~(NFDBITS-1), j = s->first_fd % NFDBITS; 
		  i <= s->last_fd; 
		  i += NFDBITS, j = 0 )
	    {
		act = &((fd_mask *)&s->act_bits.fds_bits)[ i / NFDBITS ];
		sel = &((fd_mask *)&s->sel_bits.fds_bits)[ i / NFDBITS ];

		if( !*sel )
		    continue;

		/* Skip remainder of *sel if it becomes empty */
		for( j; *sel && j < NFDBITS; j += STEPS )
		    if( *sel & ( MASK << j ) )
		    {
			l = j + STEPS;
			if (l > NFDBITS)
			    l = NFDBITS;
			for( k = j; *sel && k < l; k++ )
			    if( *sel & ( 1L << k ) )
		{
		    struct calls *call = &s->callvec[ i + k ];
		    *act &= ~( 1L << k );
		    *sel &= ~( 1L << k );
		    CLTRACE(1)( "ii_CL_poll_call0 fd=%d op=%d call=%p func=%p closure=%p \n",
			i + k, o, 
			call,
			(call) ? call->func : 0,
			(call) ? call->closure : 0 );
          
		   if (shmptr != NULL && (i+k) == CL_poll_ptr->bat_fd)
		   {
	            if (shmptr->bat_signal <= 0)
			continue;
	            else
			shmptr->bat_signal = 0;
                   }

		    (*call->func)( call->closure, OK );

		    /* If all set bits handled, quit now */
		    if ( --bits_set == 0 )
		    {
			back = !back;
			return;
		    }
		}
		    }
	    }
	}
	else
	{
	    /* Start with highest registered fd */
	    for ( i = s->last_fd & ~(NFDBITS-1), j = s->last_fd % NFDBITS, h = 0;
		  i >= 0; 
		  i -= NFDBITS, j = NFDBITS - STEPS, h = STEPS - 1 )
	    {
		act = &((fd_mask *)&s->act_bits.fds_bits)[ i / NFDBITS ];
		sel = &((fd_mask *)&s->sel_bits.fds_bits)[ i / NFDBITS ];

		if( !*sel )
		    continue;

		/* Skip remainder of *sel if it becomes empty */
		for( j; *sel && j >= 0; j -= STEPS, h = STEPS - 1 )
		    if( *sel & ( MASK << j ) )
			for( k = j + h; *sel && k >= j; k-- )
			    if( *sel & ( 1L << k ) )
		{
		    struct calls *call = &s->callvec[ i + k ];
		    *act &= ~( 1L << k );
		    *sel &= ~( 1L << k );
		    CLTRACE(1)( "ii_CL_poll_call1 fd=%d op=%d call=%p func=%p closure=%x \n",
			i + k, o, 
			call,
			(call) ? call->func : 0,
			(call) ? call->closure : 0 );

                   if (shmptr != NULL && (i+k) == CL_poll_ptr->bat_fd)
		   {
	            if (shmptr->bat_signal <= 0)
			continue;
	            else
			shmptr->bat_signal = 0;
                    }

		    (*call->func)( call->closure, OK );

		    /* If all set bits handled, quit now */
		    if ( --bits_set == 0 )
		    {
			back = !back;
			return;
		    }
		}
	    }
	}
    }

    back = !back;
}

/*
** 	Processing after a timeout.  
**
**	Current state:
**		timer.now - time after select call
**		timer.expire - when first timer goes off
**		op[0].call[].timout - 
**
**	Perform this:
**	-	Drive expired events.
**	- 	Reset unexpired timeouts with repsect to timer.now.
** 	-	Compute next timer.
**
**	23-Mar-2000 (jenjo02)
**	    Use first_fd, last_fd hints to bound scan.
**	28-Mar-2000 (jenjo02)
**	    Refine hints, start with exact first_fd bit.
**	06-jun-2002 (devjo01)
**	    Prevent 'k' from exceeding bit array bounds.
*/

ii_CL_timeout( struct _CLpoll *CL_poll_ptr )
{
    i4  o;
    i4 now;
    STATUS status;

    now = CL_poll_ptr->timer.now;

    CL_poll_ptr->timer.now = 0;
    CL_poll_ptr->timer.expire = FOREVER;

    for( o = 2; o--; )
    {
	register i4  i;
	register struct selector *s;
	register fd_mask *act;
	register int j;
	register int k;
	int	     l;

	s = &CL_poll_ptr->op[ o ];

	if ( s->first_fd < 0 )
	    continue;

	/* Limit scan to boundary hints */
	for ( i  = s->first_fd & ~(NFDBITS-1), j = s->first_fd % NFDBITS; 
	      i <= s->last_fd; 
	      i += NFDBITS, j = 0 )
	{

	    act = &((fd_mask *)&s->act_bits.fds_bits)[ i / NFDBITS ];

	    if( !*act )
		continue;

	    for( j; *act && j < NFDBITS; j += STEPS )
		if( *act & ( MASK << j ) )
		{
		    l = j + STEPS;
		    if (l > NFDBITS)
			l = NFDBITS;
		    for( k = j; *act && k < l; k++ )
			if( *act & ( 1L << k ) )
	    {
		struct calls *call = &s->callvec[ i + k ];

		if( call->timeout == -1 )
			continue;

		call->timeout -= now;

		if( call->timeout <= 0 ) 
		{
			CLTRACE(1)( "ii_CL_timeout %d\n", i + k );

			*act &= ~( 1L << k );
			(*call->func)( call->closure, E_TIMEOUT );
		}
		else if( call->timeout < CL_poll_ptr->timer.expire )
		{
			CL_poll_ptr->timer.expire = call->timeout;
		}
	    }
		}

	}
    }


    CLTRACE(2)( "ii_CL_timeout new %d\n", CL_poll_ptr->timer.expire );
}

# ifdef USE_POLL

/*
** Name: ii_CL_poll_poll - emulate select() with poll()
**
** History:
**	13-Dec-91 (seiwald)
**	    If POLLERR or POLLHUP occur, pretend registered operation is
**	    ready, so that the actual I/O call can pick up the error 
**	    indication.
**	24-oct-96 (nick)
**	    Change arbitrary limit on poll() timeout to match that implemented
**	    on select() calls i.e. 10^8 seconds.
**	 4-feb-97 (kch)
**	    On Unixware (usl_us5) the 10^11 millisecond constant used for the
**	    timeout comparison (see previous change) is too large. This has
**	    been decreased to 2x10^9 on this platform only.
**	 7-feb-97 (kch)
**	    On Unixware (usl_us5), errno is not set to EINVAL if timeout is
**	    greater than 2x10^9 milliseconds.
**	 7-feb-97 (kch)
**	    On Unixware (usl_us5), we now set timeout to -1 and don't return -1.
**	25-jun-97 (ocoro02)
**	    Added sos_us5 to defines for above Unixware problem.
**	01-jul-1997 (bobmart)
**	    Added nc4_us5 to defines for the above Unixware problem.
**	16-jul-1997 (ocoro02)
**	    Added dr6_us5 to defines for the above Unixware problem.
**	12-sep-1997 (ocoro02)
**	    Added rmx_us5 to defines for the above Unixware problem.
**      10-apr-97 (hweho01/popri01)
**          Cross-integrated UnixWare(usl_us5) fix (change #428664) from 1.2/01:
**          Now set timeout to -1 and don't return -1.
**      13-Jun-1997 (allmi01)
**          Changed max value check for timeout on SCO Openserver to be the
**          max value that a i4 can hold (2^31).
**      03-Dec-1998 (musro02)
**          Added sqs_ptx to above timeout change for SCO
**	23-Mar-2000 (jenjo02)
**	    Pass "minfd", "maxfd" boundary markers of readfds and writefds
**	    so we won't waste time looking for bits that can't be set.
**	28-Mar-2000 (jenjo02)
**	    Recalculate fd hint boundaries to eliminate those which may
**	    have been deregistered.
**	22-May-2000 (jenjo02)
**	    There may be no registered FDs to poll (minfd=-1),
**	    so check for the possibility.
**	13-Jul-2006 (kschendel)
**	    timeout is declared as int.  I don't know what the above guys
**	    were smoking with their 10^11's.
*/

static i4
ii_CL_poll_poll( CL_poll_ptr, minfd, maxfd, timeout, poll_errno )
struct _CLpoll  *CL_poll_ptr;
int		minfd;
int		maxfd;
int 		timeout;
int		*poll_errno;
{
    register i4 i, h;
    struct pollfd  *fd;
    register struct selector *sr = &CL_poll_ptr->op[ 0 ];
    register struct selector *sw = &CL_poll_ptr->op[ 1 ];
    STATUS 	status;
    register fd_set	*readfds  = &sr->sel_bits;
    register fd_set	*writefds = &sw->sel_bits;
    i4		sr_first_fd = -1, sr_last_fd = -1;
    i4		sw_first_fd = -1, sw_last_fd = -1;

# ifdef xCL_068_USE_SIGACTION
    sigset_t mask;
# endif

    fd = CL_poll_ptr->fds;

    /* 
    ** select() will return EINVAL is the timeout exceeds 10^8 seconds
    **
    ** We used to change the timeout to INFTIM here for poll if we passed
    ** anything greater than 10^4 seconds ; this is too short ( only 2.5 hrs
    ** or so ) and so we now emulate select() behaviour and error if we
    ** exceed the limit ( which is 10^11 milliseconds ).  Note that currently
    ** this results in abort() which is probably OK as an FE shouldn't be
    ** specifying such a large timeout and if the server is, then we ought
    ** to know about it.
    */

# if defined(usl_us5) || defined(nc4_us5) \
  || defined(dr6_us5) || defined(rmx_us5) || defined(rux_us5)
    if (timeout > 2000000000)
	timeout = -1;
    /* Note: if we get into any platforms that use an LPI64 model,
    ** we'll need the above-mentioned 10^11 check for them.
    ** Otherwise, ints are going to be no larger than i4's, which
    ** max out at 2x10^9, and there is no percentage in even testing.
    ** (timeout is in milliseconds.)
    */
# endif /* usl_us5 */

    if ( CL_poll_ptr->bat_shmseg != NULL) 
	  timeout = 0;

    /* Start at the first-used fd hint, if any */
    if ( minfd >= 0 )
    {
	for ( i =  minfd / NFDBITS, h = minfd % NFDBITS; 
	      i <= maxfd / NFDBITS; 
	      i++, h = 0 )
	{
	    register int j;
	    register unsigned long xr;
	    register unsigned long xw;

	    /*
	    ** these (unsigned long) casts assume that
	    ** sizeof(unsigned long) >= sizeof(fd_mask)
	    */
#ifdef hp2_us5
	    xr = &((unsigned long)readfds->fds_bits)[ i ];
	    xw = &((unsigned long)writefds->fds_bits)[ i ];
#else
	    xr = (unsigned long)readfds->fds_bits[ i ];
	    xw = (unsigned long)writefds->fds_bits[ i ];
#endif  /* hp2_us5 */

	    j = (i * NFDBITS) + h;

	    for( xr >>= h, xw >>= h; 
		 xr || xw; 
		 xr >>= 1, xw >>= 1, j++ )
	    {
		i4 ev = 0;

		if( xr & 1 )
		{
		    ev |= POLLIN;
		    xr &= ~1;
		    if ( sr_first_fd < 0 )
			sr_first_fd = j;
		    sr_last_fd = j;
		}
		if( xw & 1 )
		{
		    ev |= POLLOUT;
		    xw &= ~1;
		    if ( sw_first_fd < 0 )
			sw_first_fd = j;
		    sw_last_fd = j;
		}

		if( ev )
		{
		    fd->fd = j;
		    fd->events = ev;
		    fd++;
		}
	    }
	}
    }

    /* Update hints with what we found */
    sr->first_fd = sr_first_fd;
    sr->last_fd  = sr_last_fd;
    sw->first_fd = sw_first_fd;
    sw->last_fd  = sw_last_fd;


/* block SIGUSR2 signals when server is not in poll to protect system calls
from being interrupted */

# if defined(dr6_us5) || defined(usl_us5) || defined(sparc_sol) \
  || defined(sui_us5) || defined(nc4_us5) || defined(sqs_ptx)
/*
** bug #44975. disable longjmp() calls until after setjmp() has been called
*/
    use_longjmp = 0;
# endif /* dr6_us5, usl_us5, su4_us5, sui_us5, nc4_us5, sqs_ptx */

# ifdef xCL_068_USE_SIGACTION
    if ( CL_poll_ptr == &CL_poll )
    {
        sigemptyset(&mask);
        sigaddset(&mask,SIGUSR2);
        sigprocmask(SIG_UNBLOCK, &mask, (sigset_t *)0);
    }
# endif


# if defined(dr6_us5) || defined(usl_us5) || defined(sparc_sol) \
  || defined(sui_us5) || defined(nc4_us5) || defined(sqs_ptx)
    /*
    ** bug 44975, use of setjmp/longjmp to avoid race condition with poll(),
    ** see history comments above
    */

    /*
    ** check whether SIGUSR2 has already been caught, we may not need
    ** to setjmp()
    */

    if ( CL_poll_ptr == &CL_poll )
    {
        if ( CL_poll.interrupted )
        {
	    /*
	    ** zero timeout and drop through to poll() call
	    */
	    timeout = 0;
        }
        else
        {
	    if (setjmp(jmp_env))
	    {
	        /*
	        ** resuming from longjmp(), zero timeout and drop
	        ** through to poll() call
	        */
	        timeout = 0;
	    }
	    else
	    {
	        /*
	        ** set latch to indicate that we should longjmp() from
	        ** signal handler on receipt of SIGUSR2
	        */
	        use_longjmp = 1;
		
	        /*
	        ** re-check whether SIGUSR2 has been caught, if so we can
	        ** zero the timeout now and unset the longjmp() latch and
	        ** drop through to poll() call
	        */
	        if( CL_poll.interrupted )
	        {
		    timeout = 0;
		    use_longjmp = 0;
	        }
	    }
        }
    }

#else

    if ( CL_poll.interrupted )
	timeout = 0;

# endif /* dr6_us5, usl_us5, su4_us5, sui_us5, nc4_us5, sqs_ptx */

    CLTRACE(2)( "ii_CL_poll_poll: read (%d,%d) write (%d,%d)\n", 
		sr->first_fd, sr->last_fd,
		sw->first_fd, sw->last_fd);

    i = poll( CL_poll_ptr->fds, (fd_mask)(fd - CL_poll_ptr->fds), timeout );

    *poll_errno = errno;

# if defined(dr6_us5) || defined(usl_us5) || defined(sparc_sol) \
  || defined(sui_us5) || defined(nc4_us5) || defined(sqs_ptx)
    /*
    ** disable longjmp from SIGUSR2 handler (bug 44975, see history
    ** comments above)
    */
    use_longjmp = 0;
# endif /* dr6_us5, usl_us5, su4_us5, sui_us5, nc4_us5, sqs_ptx */

# ifdef xCL_068_USE_SIGACTION
    if ( CL_poll_ptr == &CL_poll )
    {
        sigprocmask(SIG_BLOCK, &mask, (sigset_t *)0);
    }
# endif
    if( CL_poll.interrupted && i <= 0)
    {
	*poll_errno = EINTR;
        return (-1);
    }

    while( fd-- > CL_poll_ptr->fds )
    {
	i4 ev = fd->events &~ fd->revents;

	CLTRACE(2)( "fd %d flags %x %x %x\n", 
		fd->fd, fd->events, fd->revents, ev );

	/* On errors, pretend registered operation is ready. */
#ifdef POLLNVAL
	if( fd->revents & ( POLLHUP | POLLERR | POLLNVAL ) )
#else
	if( fd->revents & ( POLLHUP | POLLERR ) )
#endif
	    continue;

	if( ev & POLLIN )
	    FD_CLR( fd->fd, readfds );
	if( ev & POLLOUT )
	    FD_CLR( fd->fd, writefds );
    }

    return (i);
}

# endif /* USE_POLL */

# if defined(xCL_091_BAD_STDIO_FD) || defined (xCL_RESERVE_STREAM_FDS)

/*
** Name: CL_poll_fd() - reallocate a file descriptor higher
**
** Description:
**	Reallocates a file descriptor to one above the CLPOLL_RESVFD
**	threshhold on systems where low-numbered file descriptors must
**	be reserved for stdio.
**
**	Should only be accessed through the CLPOLL_FD() macro (c.f.).
**
** History:
**	7-Aug-91 (seiwald)
**	    Written.
**	7-Nov-91 (seiwald)
**	    Don't try to dup -1 - it just scrambles errno.
**	10-mar-92 (swm)
**	    Work-around for ICL platforms; the introduction of CL_poll_fd()
**	    caused socket connect() to hang with no occurence of an event on
**	    the listen side. Dup-ing a socket (via fcntl here) gave rise to
**	    this behaviour. So just return the original fd.
**      24-jul-92 (swm)
**	    The 10-mar-92 (swm) change should have been applied to
**	    clf/hdr/clpoll.h, so remove the change from here and add
**	    appropriate change to clpoll.h.
**      07-sep-92 (gautam)
**	    CL_poll_fd rewritten. On analysis MAX_STDIO_FD on all 
**	    bad stdio systems was found to be 255. 
**	    Do not realloacate for every fd < MAX_STDIO_FD. 
**	    Instead reallocate only in the (MAX_STDIO_FD-5) - (MAX_STDIO_FD) 
**	    range
**	21-jul-1997 (canor01)
**	    If we are trying to preserve all fds below 255, force the
**	    reallocation on all low descriptors.
*/

i4
CL_poll_fd( fd )
i4	fd;
{
	i4	nfd;

# ifdef xCL_RESERVE_STREAM_FDS
        if ( fd > MAX_STDIO_FD )
        {
              return fd ;
        }

	nfd = fcntl( fd, F_DUPFD, MAX_STDIO_FD );

# else /* xCL_RESERVE_STREAM_FDS */
        if (fd < (MAX_STDIO_FD - 10) || fd > MAX_STDIO_FD)
        {
              /* the 10 is arbitrary */
              return fd ;
        }

	nfd = fcntl( fd, F_DUPFD, CLPOLL_RESVFD );

# endif /* xCL_RESERVE_STREAM_FDS */


	if( nfd < 0 )
	{
	    /* No high ones left. */
	    return fd;
	}
	else 
	{
	    /* Close old & return new. */
	    close( fd );
	    return nfd;
	} 
}
# endif /* xCL_091_BAD_STDIO_FD || xCL_RESERVE_STREAM_FDS */
