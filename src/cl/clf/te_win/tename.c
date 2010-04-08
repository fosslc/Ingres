# include	<compat.h>
# include	"telocal.h"
/*
** TEname
**	Find the name of the terminal you are using.
**
** Argument
**	tty - routine copies the terminal name into this
**	buffer.  it is assumed that the buffer is big
**	enough for whatever terminal name you expect.
**
** Returns OK if everything works, FAIL if you are
** not hooked up to a terminal (e.g. running in background)
** or whatever can go wrong.
**
*/

STATUS
TEname(
char	*tty)
{
	return(FAIL);
}
