# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	"ermo.h"
# include	"monitor.h"

/*
** Copyright (c) 2004 Ingres Corporation
**
**  Clear query buffer
**	Flag f is set if called explicitly (with \q) and is
**		not set if called automatically.
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
*/

QBclear(f)
i4	f;
{
	extern char	*beep;

	Autoclear = 0;

	if (q_ropen(Qryiop, 'w') == (struct qbuf *)NULL)
		/* clear: q_ropen */
		ipanic(E_MO0043_1500300);

	if ((Nodayfile >= 0) && f)
		putprintf(ERget(F_MO000A_beep_go), beep);

	if (f)
		clrline(0);

	Notnull = 0;
}
