/*
**  restore.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	1/30/86 (bab) -- added new routine TDrestore() to handle
**		lower level operations than should go in FTrestore().
**	08/21/87 (dkh) - Generalized TDrestore().
**	09/04/87 (dkh) - Added new argument to TDrestore().
**	08/28/90 (dkh) - Integrated porting change ingres6202p/131215.
**	12/27/90 (dkh) - Fixed bug 35055.
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
# include	<te.h>
# include	<termdr.h>

GLOBALREF	i4	IITDcur_size;
GLOBALREF	bool	IITDccsCanChgSize;
GLOBALREF	i4	YO;
GLOBALREF	i4	YN;
GLOBALREF	char	*YR;
GLOBALREF	char	*YS;

VOID
TDfkmode(mode)
i4	mode;
{
	char	outbuf[TDBUFSIZE];

	TDobptr = outbuf;
	TDoutbuf = outbuf;
	TDoutcount = TDBUFSIZE;
	if (KY)
	{
		if (mode == TD_FK_SET)
		{
			_puts(KS);
		}
		else
		{
			_puts(KE);
		}
	}
	TEwrite(TDoutbuf, (i4)(TDBUFSIZE - TDoutcount));
	TEflush();
}

VOID
TDrestore(mode, init)
i4	mode;
i4	init;
{
	char	outbuf[TDBUFSIZE];

	TDclrattr();

	TDobptr = outbuf;
	TDoutbuf = outbuf;
	TDoutcount = TDBUFSIZE;

	if (mode == TE_FORMS)
	{
		TErestore(TE_FORMS);

		if (init)
		{
			_puts(IS);
		}
		else
		{
			/*
			**  If not initializing, must be from
			**  FTrestore.  So we assume worst case
			**  that the screen size has changed
			**  and we must set it back correctly.
			*/
			if (IITDccsCanChgSize)
			{
				if (IITDcur_size == YO)
				{
					_puts(YS);
				}
				else if (IITDcur_size == YN)
				{
					_puts(YR);
				}
			}
			else
			{
				/*
				**  If we don't have the size change
				**  info, try using the IS string
				**  to set the terminal to proper size.
				*/
				_puts(IS);
			}
		}

		_puts(VS);

		if (KY && KS != NULL)
		{
			_puts(KS);
		}
	}
	else
	{
		TErestore(TE_NORMAL);

		_puts(VE);

		if (KY && KE != NULL)
		{
			_puts(KE);
		}
	}

	TEwrite(TDoutbuf, (i4) TDBUFSIZE - TDoutcount);
	TEflush();
}
