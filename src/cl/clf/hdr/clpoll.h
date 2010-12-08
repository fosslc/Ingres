/*
** Copyright (c) 2004 Ingres Corporation
*/

#ifndef CLPOLL_H_INCLUDED
#define CLPOLL_H_INCLUDED

/**
** Name: CLPOLL.H - Definitions for file descritor poll facility
**
** Description:
**	Defines of constants for the CLpoll facility interface.
**
** History:
**      22-dec-88 (anton)
**          Created.
**	06-Feb-89 (anton)
**	    Added E_HANDY_MASK
**	21-Mar-89 (GordonW)
**	    Added E_FAILED
**	4-Apr-89 (markd)
**	    Added Sequent select redfinition
**	5-Jul-90 (seiwald)
**	    Reordered history.
**	    Removed cruft.
**	    Added macros for the sysv fds problem.
**	12-feb-91 (mikem)
**	    Enabled the FD reservation scheme, previously only implemented 
**	    for SYSV, also when xCL_083_USE_SUN_DBE_INCREASED_FDS is defined.
**	12-Aug-91 (seiwald)
**	    Reimplemented CLPOLL_xxxxFD() macros as CLPOLL_FD().
**	23-Oct-91 (seiwald)
**	    Removed bogus () from null implementation of CLPOLL_FD().
**	31-Jan-92 (gautam)
**	    Added define for FD_EXCEPT for exception fd polling support
**	7-Sep-92 (gautam)
**	    CLPOLL_FD is now compiled around xCL_091_BAD_STDIO_FD define.
**	09-mar-1998 (canor01)
**	    Define CLPOLL_FD for xCL_RESERVE_STREAM_FDS platforms.
**	10-may-1999 (walro03)
**	    Remove obsolete version string sqs_us5.
**	16-Nov-2010 (kschendel) SIR 124685
**	    Prototype / include fixes.
**	29-Nov-2010 (frima01) SIR 124685
**	    Added iiCLfdreg and CL_poll_fd prototypes.
**	1-Dec-2010 (kschendel) SIR 124685
**	    Fine tune iiCLfdreg prototype.
**/

/*
**	interface defined constants
*/

# define	FD_READ		0x01
# define	FD_WRITE	0x02
# define	FD_EXCEPT	0x03

# define	E_INTERRUPTED	E_CL_MASK + E_HANDY_MASK + 0x01
# define	E_TIMEOUT	E_CL_MASK + E_HANDY_MASK + 0x02
# define	E_FAILED	E_CL_MASK + E_HANDY_MASK + 0x03

/*
** Name: CLPOLL_FD() - macros for renumbering fds for use with CLpoll.
**
** Description:
**	On some SYSV machines, stdio can only use the first NFILE (20)
**	descriptors, even though more are available.  Some INGRES
**	programs open many connections and quickly use up the first
**	NFILE descriptors, leaving stdio and library calls built upon
**	stdio (such as getpwent()) inoperable.
**
**	When SUN's DBE product is installed and code enabled by 
**	xCL_083_USE_SUN_DBE_INCREASED_FDS is running then it is possible
**	to have up to 1024 file descriptors available.  Unfortunately, due
**	to the "char" declaration of the fd in the file pointer, only the
**	first 256 may be used by stdio calls.  The same solution as was
**	implemented for the above "SYSV" problem has been chosed for this
**	"DBE" problem.
**	
**	Since our stdio operations are invariable open-read/write-close,
**	we'll never need more than one fd reserved.
**
**	To ensure one low numbered fd is available we follow all calls
**	which allocate a fd with a call to the CLPOLL_FD() macro.  In 
**	truth, we only need follow calls which get fds for communications, 
**	since communication code is the sole consumer of the higher 
**	numbered fds.
**
**	A sample use is:
**		fd = socket(...);
**		CLPOLL_FD( fd );
**
**	This macro dups the fd to one above CLPOLL_RESVFD if the fd is
**	below CLPOLL_RESVFD, closing the old fd and setting fd to the
**	new one.
**
**	CLPOLL_RESVFD should be left as low as possible so that the 
**	dup'ing and closing code can be shortcut often, but high enough
**	that normal (non communication code) use can't consume all
**	the fds below it.
**
**	This approach seems easier than bracketing all stdio calls,
**	since they may be used implicitly by code we don't control.
**
** History:
**	5-Jul-90 (seiwald)
**	    Written.
**	12-feb-91 (mikem)
**	    Enabled the FD reservation scheme, previously only implemented 
**	    for SYSV, also when xCL_083_USE_SUN_DBE_INCREASED_FDS is defined.
**	12-Aug-91 (seiwald)
**	    Reimplemented CLPOLL_xxxxFD() macros as a single CLPOLL_FD()
**	    macro.  The old way only reserved one file descriptor, and that
**	    wasn't always enough on sqs_us5.  The new way guarantees that 
**	    comm code will be the last to use the fds below CLPOLL_RESVFD.
**	24-jul-92 (swm)
**	    relocation of 10-mar-92 (swm) change from clf/handy/clpoll.c
**	    on advice from seiwald:
**	    Work-around for ICL platforms; the introduction of CL_poll_fd()
**	    caused socket connect() to hang with no occurence of an event on
**	    the listen side. Dup-ing a socket (via fcntl) gave rise to
**	    this behaviour. So do not use the mechanism.
**	7-Sep-92 (gautam)
**	    CLPOLL_FD is now compiled around xCL_091_BAD_STDIO_FD define.
**	    Defined MAX_STDIO_FD for those systems with the 
**	    xCL_091_BAD_STDIO_FD problem - this is defaulted to 255. 
**	11-jun-1998 (walro03)
**	    Compile errors in clpoll.c because ifdef in clpoll.c does not
**	    match the one here.
**	    
*/


# if defined(xCL_091_BAD_STDIO_FD) || defined (xCL_RESERVE_STREAM_FDS)

# define  MAX_STDIO_FD	255
# define CLPOLL_RESVFD  20

# define	CLPOLL_FD( fd ) ( fd = CL_poll_fd( fd ) )

# else 

# define	CLPOLL_FD( fd )

# endif /* xCL_091_BAD_STDIO_FD || xCL_RESERVE_STREAM_FDS */

/* Externs */

FUNC_EXTERN void iiCLintrp(i4);
FUNC_EXTERN STATUS iiCLpoll(i4 *);
FUNC_EXTERN i4 CL_poll_fd(i4 fd);
FUNC_EXTERN STATUS iiCLfdreg(
        register int fd,
        i4 op,
        void (*func)(void *, i4),
        void *closure,
        i4 timeout);

#endif /* CLPOLL_H_INCLUDED */
