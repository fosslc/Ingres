/*
**  TPUTS.c - Generate output string for terminal control
**  Put the character string cp out, with padding.
**  The number of affected lines is affcnt, and the routine
**  used to output one character is outc.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	*Sccsid = "%W%	%G%";
**
**  History:
**	8/21/85 daveb - Fixed incorrect dereferencing of outc.
**	10/20/86 (KY)  -- Changed CH.h to CM.h.
**	08/14/87 (dkh) - ER changes.
**	11/01/88 (dkh) - Performance changes.
**	12/27/89 (dkh) - Put in change to not split escape sequences across
**			 system I/O calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Mar-2005 (lakvi01)
**	    Corrected function prototypes.
*/


# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<termdr.h>
# include	<cm.h>
# include	<te.h>
# include	"it.h"


GLOBALREF	i4	ospeed;
GLOBALREF	i4	plodcnt;
GLOBALREF	bool	plodflg;

FUNC_EXTERN	i4	_TDputchar();
FUNC_EXTERN	i4	TDplodput();

VOID
TDtputs(acp, affcnt, outc)
char	*acp;
i4	affcnt;
i4	(*outc)();
{
	reg	char	*cp = acp;
	reg	i4	i = 0;
	reg	i4	mspc10;
	reg	i4	xcnt;
		char	xbuf[3072 + 1];

	if (cp == 0)
		return;

	/*
	**  Convert the number representing the delay.
	*/
	if (!(outc == TDplodput && plodflg))
	{
	    if (CMdigit(cp))
	    {
		do
		    i = i * 10 + *cp++ - '0';
		while (CMdigit(cp));
	    }
	    if (!PF)
	    {
		i *= 10;
	    }
	    if (*cp == '.')
	    {
		cp++;
		if (CMdigit(cp))
		    i += *cp - '0';
		/*
		**  Only one digit to the right of the decimal point.
		*/
		while (CMdigit(cp))
		    cp++;
	    }
	    /*
	    ** If the delay is followed by a `*', then
	    ** multiply by the affected lines count.
	    */
	    if (*cp == '*')
	    {
		cp++, i *= affcnt;
	    }
	    if (TDoutcount < 100)
	    {
		if (ZZB)
		{
		    *TDobptr = '\0';
		    xcnt = ITxout(TDoutbuf, xbuf, TDBUFSIZE - TDoutcount);
		    TEwrite(xbuf, xcnt);
		}
		else
		{
		    TEwrite(TDoutbuf, TDBUFSIZE - TDoutcount);
		}
		TEflush();
		TDobptr = TDoutbuf;
		TDoutcount = TDBUFSIZE;
	    }
	}


	/*
	**  The guts of the string.
	*/

	if (outc == _TDputchar)
	{
		while(*cp)
			TDput(*cp++);
	}
	else if (outc == TDplodput)
	{
		if (plodflg)
		{
			while (*cp++)
			{
				plodcnt--;
			}
			i = 0;
		}
		else
		{
			while (*cp)
			{
				TDput(*cp++);
			}
		}
	}
	else
	{
		while (*cp)
			(*outc)(*cp++);
	}

	/*
	**  If no delay needed, or output speed is
	**  not comprehensible, then don't try to delay.
	*/

	if (i == 0)
		return;

	/*
	**  Round up by a half a character frame,
	**  and then do the delay.
	**  Too bad there are no user program accessible programmed delays.
	**  Transmitting pad characters slows many
	**  terminals down and also loads the system.
	*/


/*
** The code is OK since we are setting ospeed with a call to
** TEspeed which should take care of the 19.2k baud rate
** problems (BUGS 507, 573, 832)
*/

	if (!PF)
	{
		mspc10 = ospeed;
		i += mspc10 / 2;
		i /= mspc10;
	}
	if (outc == _TDputchar)
	{
		for (; i > 0; i--)
			TDput(PC);
	}
	else
	{
		for (; i > 0; i--)
			(*outc)(PC);
	}
}
