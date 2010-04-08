# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	"monitor.h"
# include	"ermo.h"

#ifdef SEP

# include <tc.h>

GLOBALREF   TCFILE *IITMincomm;
GLOBALREF   TCFILE *IITMoutcomm;

#endif /* SEP */

/*
** Copyright (c) 2004 Ingres Corporation
**
**  PRINT QUERY BUFFER
**
**	The logical query buffer is printed on the terminal regardless
**	of the "Nodayfile" mode.  Autoclear is reset, so the query
**	may be rerun.
**
**	Uses trace flag 6
**
**  HISTORY:
**	8/17/85 (peb)	Updated for multinational character support.
**      10-may-89 (Eduardo)
**              Added interface to SEP tool
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
*/

print()
{
	register struct qbuf	*iop;
	register i4		c;

	FUNC_EXTERN	 i4	q_getc();

	Autoclear = 0;

	clrline(1);

	if ((iop = q_ropen(Qryiop, 'r')) == NULL)
		/* print: q_ropen 1 */
		ipanic(E_MO0068_1501400);

	/* list file on standard output */

	Notnull = 0;

	while ((c = q_getc(iop)) > 0)
	{
		char	realc = c;
		char	x[3];

		/* Any Kanji (double char) impact here ??? */
		ITxout(&realc, x, 1);
#ifdef SEP
		if (IITMoutcomm != NULL)
		    TCputc(*x,IITMoutcomm);
		else
#endif /* SEP */
		    SIputc(*x, stdout);

		Notnull++;
	}

	if (q_ropen(iop, 'a') == NULL)
		/* print: q_ropen 2 */
		ipanic(E_MO0069_1501401);

#ifdef SEP
	if (IITMoutcomm != NULL)
	    TCflush(IITMoutcomm);
	else
#endif /* SEP */
	    SIflush(stdout);

	cgprompt();
}
