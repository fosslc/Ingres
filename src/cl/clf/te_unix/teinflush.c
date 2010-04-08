/*
**	teinflush.c - Flush input.
**
**Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	teinflush.c - Flush input.
**
** Description:
**	Flush any type ahead input from a terminal.
**
** History:
**	06/11/87 (dkh) - Initial version.
**	10/17/87 (dkh) - Changed to work properly on Pyramid.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	14-Feb-2008 (hanje04)
**	    SIR S119978
**	    Update includes for renamed header
**/

# include	<compat.h>
# include	<gl.h>
# include	<te.h>
# include	"telocal.h"

/*{
** Name:	TEinflush - Flush input.
**
** Description:
**	Flush any type ahead input from the terminal.  The
**	assumption here is that this routine will NOT be
**	called if a forms based front end is in test mode
**	(i.e., input coming from a file).  This will
**	eliminate the possibility of doing a flush on
**	input that comes from a file.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	06/11/87 (dkh) - Initial version.
**	10/17/87 (dkh) - Changed to work properly on Pyramid.
**
**	14-apr-1989 (fredv)
**		Replaced TERMIO by xCL_018_TERMIO_EXISTS.
**	06-May-2005 (hanje04)
**	    As part of Mac OSX port, add support for TERMIOS
*/
VOID
TEinflush()
{

# ifdef xCL_018_TERMIO_EXISTS

	/*
	**  The last argument (0) actually tells SYSTEM V
	**  to flush the input queue.
	*/
	ioctl(STDINFD, TCFLSH, 0);

# elif defined( xCL_TERMIOS_EXISTS )
	tcflush( STDINFD, TCIFLUSH );
# else

	struct	sgttyb	_tty;

	ioctl(STDINFD, TIOCGETP, &_tty);
	/*
	**  The last argument is not used according
	**  to the BSD documentation.  But Pyramid
	**  needs it to work correctly.
	*/
	ioctl(STDINFD, TIOCFLUSH, &_tty);

# endif /* xCL_018_TERMIO_EXISTS */
}
