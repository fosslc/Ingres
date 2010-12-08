/*
**Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<te.h>
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
** History:
**      21-apr-1999 (hanch04)
**        Replace STrindex with STrchr
**      18-Jan-1999 (fanra01)
**          Modified to call iiCLttyname.
**	14-Feb-2008 (hanje04)
**	    SIR S119978
**	    Update includes for renamed header
**	04-Nov-2010 (miket) SIR 124685
**	    Prototype cleanup.
*/

STATUS
TEname(tty)
char	*tty;
{
	extern	char	*ttyname();
	char	buf[50];
	char	*cp;
	STATUS status = FAIL;

	if ((status = iiCLttyname (STDERRFD, buf, sizeof(buf)-1)) == OK)
	{
	    cp = STrchr(buf, '/');
	    if (cp != NULL)
	    {
		STcopy(cp + 1, tty);
	    }
	    else
	    {
		status = FAIL;
	    }
	}
	return(status);
}
