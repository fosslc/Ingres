/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** Header file for Terminal I/O Compatibility library for
** 4.2 Berkeley UNIX.
**
** History:
** 	Created 2-15-83 (dkh)
**	14-Apr-1989 (fredv)
**		Replaced TERMIO by xCL_018_TERMIO_EXISTS from <clconfig.h>,
**		also include <clconfig.h>.
**	29-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Add include <systeypes.h>;
**      6-Aug-1990 (jkb)
**              Add ifdef sqs_us5.  It is necessary to use UCB ioctl
**              semantics to get job control and be able to use function
**              keys.  The SYSV termio available is only a partial
**              emulation of standard SYSV termio which doesn't have a
**              mode equivalent to "CBREAK".
**	05-Dec-1997 (allmi01)
**		Added specieal # defines and # undefs for sgi_us5 to
**		get winsize struct definitions from sys/termios.h.
**	23-Mar-1998 (kosma01)
**		Remove sgi_us5 specific #defines and #undefs, for sigcontext
**		struct definitions. Other changes to the compiler command
**		made them obsolete.
**	10-may-1999 (walro03)
**		Remove obsolete version string sqs_us5.
**	06-May-2005 (hanje04)
**		As part of Mac OSX port add stuff for xCL_TERMIOS_EXISTS.
**	  	Much of the TERMO stuff has now been depricated.
**	12-Feb-2008 (hanje04)
**		SIR S119978
**		Renamed from TE.h to telocal.h
**	04-Nov-2010 (miket) SIR 124685
**	    Prototype cleanup.
**

*/

# include	<clconfig.h>
# include	<compat.h>		/* include compatiblity header file */
# include	<systypes.h>
# include	<st.h>


# ifdef	xCL_018_TERMIO_EXISTS
# include	<termio.h>		/* include header to handle tty's */
# elif defined(xCL_TERMIOS_EXISTS)
# include 	<termios.h>
# include 	<sys/ioctl.h>
# else
# include	<sgtty.h>
# endif

# ifdef dgc_us5
# include	<osgtty.h>
# endif

# include	<si.h>			/* to get stdio.h stuff */

# define	STDINFD		0	/* file descriptor for stdin on UNIX */
# define	STDOUTFD	1	/* file descriptor for stdout on UNIX*/
# define	STDERRFD	2	/* file descriptor for stderr on UNIX */

/*
** Terminal characteristics you can set
*/

# define	TECRMOD		1

/*
** The following are defines for the delays that a terminal 
** uses when running at a specific baud rate.
*/

# define	DE_ILLEGAL	-1
# define	DE_B50		2000
# define	DE_B75		1333
# define	DE_B110		909
# define	DE_B134		743
# define	DE_B150		666
# define	DE_B200		500
# define	DE_B300		333
# define	DE_B600		166
# define	DE_B1200	83
# define	DE_B1800	55
# define	DE_B2000	50
# define	DE_B2400	41
# define	DE_B3600	20
# define	DE_B4800	10
# define	DE_B9600	5
# define	DE_B19200	2

FUNC_EXTERN bool	TEisa(FILE *stream);
