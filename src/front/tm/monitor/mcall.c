# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"monitor.h"

/*
** Copyright (c) 2004 Ingres Corporation
**
**  MCALL -- call a macro
**
**	This takes care of springing a macro and processing it for
**	any side effects.  Replacement text is saved away in a static
**	buffer and returned.
**
**	Parameters:
**		mac -- the macro to spring.
**
**	Returns:
**		replacement text.
**
**	Side Effects:
**		Any side effects of the macro.
**
**	Called By:
**		go.c
**		proc_error
**
**	Compilation Flags:
**		none
**
**	Trace Flags:
**		18
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		none
**
**	History:
**		8/10/78 (eric) -- broken off from go().
**	26-Aug-2009 (kschendel) 121804
**	    Need monitor.h to satisfy gcc 4.3.
*/

char *
mcall(char *mac)
{
	register char	c;
	register char	*p;
	static char	buf[100];
	FUNC_EXTERN i4	macsget();

	/* set up a new environment to process macro */
	macnewev(macsget, &mac);

	/* process it -- save result */

	for (p = buf; (c = macgetch()) > 0; )
	{
		if (p < &buf[sizeof buf])
			*p++ = c;
	}

	*p = 0;

	/* restore the old environment */
	macpopev();

	return (buf);
}
