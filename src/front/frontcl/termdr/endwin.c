/*
**  Clean things up before exiting.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	10/22/88 (dkh) - Performance changes.
**	08/04/89 (dkh) - Added support for 80/132 runtime switching.
**	08/28/90 (dkh) - Integrated porting change ingres6202p/131215.
**	10-oct-92 (leighb) DeskTop Porting Change:
**		Don't write '\r' '\n' to stdout in Tools for Windows.
**      14-Jun-95 (fanra01)
**              Added NT_GENERIC to PMFEWIN3 sections.
**	01-oct-96 (mcgem01)
**		extern changed to GLOBALREF for global data project.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Mar-2205 (lakvi01)
**	    Corrected function prototypes.
*/

/*
**	static	char	*Sccsid = "%W%	%G%";
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<te.h>
# include	<termdr.h>
# include	"it.h"

GLOBALREF	bool	TDisopen;

VOID
TDendwin()
{
	char	outbuf[TDBUFSIZE];

	if (!TDisopen)
	{
		return;
	}
#if !defined(PMFEWIN3) && !defined(NT_GENERIC)
	TEput('\r');
	TEput('\n');
#endif
	TDobptr = outbuf;
	TDoutbuf = outbuf;
	TDoutcount = TDBUFSIZE;
	_puts(VE);
	_puts(TE);
	_echoit = TRUE;
	_rawmode = FALSE;
	_pfast = FALSE;
	_puts(EA);		/* clean display attributes up */
	if (LD)
	{
		_puts(LE);
	}
	if (KY)
	{
		_puts(KE);		/* get out of keypad mode */
	}

	/*
	**  Terminal exit string to allow users to
	**  reset terminal to whatever state they want.
	*/
	_puts(ES);

	IITDrstsize();

	TEwrite(TDoutbuf, (i4)(TDBUFSIZE - TDoutcount));
	TEflush();
	TEclose();
	TDisopen = FALSE;
	if (curscr)
	{

		/* code for STANDOUT taken out since
		** STANDOUT should never be set now,
		** I hope.  (dkh)
		*/

		_endwin = TRUE;
	}
}

VOID
TDflush(c)
char	c;
{
	i4	i;
	char	buf[3072 + 1];
	char	*cp;

	*TDobptr++ = c;
	if (ZZB)
	{
		i = ITxout(TDoutbuf, buf, (i4) TDBUFSIZE);
		cp = buf;
	}
	else
	{
		i = TDBUFSIZE;
		cp = TDoutbuf;
	}
	TEwrite(cp, i);
	TDobptr = TDoutbuf;
	TDoutcount = TDBUFSIZE;
}

VOID
TDrstcur(by, bx, ey, ex)
i4	by;
i4	bx;
i4	ey;
i4	ex;
{
	i4	i;
	char	buf[TDBUFSIZE];
	char	xbuf[3072 + 1];

	TDoutcount = TDBUFSIZE;
	TDoutbuf = buf;
	TDobptr = buf;
	TDsmvcur(by, bx, ey, ex);
	i = ITxout(TDoutbuf, xbuf, (i4) TDBUFSIZE - TDoutcount);
	TEwrite(xbuf, i);
	TEflush();
}
