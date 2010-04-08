
# include	"telocal.h"

/*
**Copyright (c) 2004 Ingres Corporation
**
**	Name:
**		TEgtty()
**
**	Function:
**		Connected to a terminal?
**
**	Arguments:
**		bool	connected;
**		i4	fd;
**		char	*char_buf;
**
**	Result:
**		TRUE if backend is running standalone (connected to a terminal).
**		Is used on UNIX for debugging.  Always returns FALSE on VMS.
**
**
**	Side Effects:
**		None
**
**	History:
**	
**		Revision 30.2  86/06/30  15:49:51  boba
**		Use TERMIO instead of WECO.
**		
**		Revision 3.0  85/05/10  16:32:51  wong
**		Fixed to actually return a boolean value.
**		
**		10/83 - (mmm)
**			written
**
**		14-apr-1989 (fredv)
**			Replaced TERMIO by xCL_018_TERMIO_EXISTS.
**		06-May-2005 (hanje04)
**		    As part of Mac OSX port, add support for TERMIOS
**	14-Feb-2008 (hanje04)
**	    SIR S119978
**	    Update includes for renamed header
*/

bool
TEgtty(fd, char_buf)
i4	fd;
char	*char_buf;
{
# ifdef xCL_018_TERMIO_EXISTS
	return (ioctl(fd, TCGETA, char_buf) == 0);
# elif defined( xCL_TERMIOS_EXISTS )
	return( tcgetattr( fd, (struct termios *)char_buf ) == 0 );
# else
	return (gtty(fd, char_buf) == 0);
# endif
}
