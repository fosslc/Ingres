/*
**  itin.c
**
**  Get an international character (8th bit turned on or a
**  special leading sequence.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History
**    Written - 6/22/85 (dkh)
**	01/28/87 (KY) -- Added CM.h and
**			 changed return value of ITnxtch() and ITin()
**			 to KEYSTRUCT structure.
**	19-jun-87 (bab) Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	09-nov-88 (bruceb)
**		In ITnxtch():  If TEget returns because of timeout (returned
**		character is TE_TIMEDOUT), then return a filled KEYSTRUCT (to
**		avoid upsetting calling routines) and set the IIFTtmout global
**		to TRUE.  TEget also called with the IIFTntsNumTmoutSecs global
**		as number of seconds until timeout occurs.
**	11/29/90 (dkh) - Moved check for EOF here since ITin() is the only
**			 that can tell the difference between EOF and a
**			 legal character that is 0xFF.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**      24-sep-96 (mcgem01)
**              Global data moved to termdrdata.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<si.h>
# include	<te.h>
# include	"it.h"
# include	<cm.h>
# include	<ex.h>
# include	<kst.h>
# include	<termdr.h>
# include	<er.h>
# include	"ertd.h"


GLOBALREF	i4	ITdebug;

GLOBALREF	bool	IIFTtmout;
GLOBALREF	i4	IIFTntsNumTmoutSecs;

static	u_char	ITinbuf[ITBUFSIZE] ZERO_FILL;
static	i4	ITbufptr = 0;


/*
**  This routine must be instrumented to handle the -I flag for testing
**  purposes.
*/

KEYSTRUCT   *
ITnxtch()
{
	KEYSTRUCT	*ks;
	i4		chr;

	IIFTtmout = FALSE;

	ks = TDgetKS();
	if (ITbufptr > 0)
	{
		TDsetKS(ks, (i4)ITinbuf[--ITbufptr], 1);
		if (CMdbl1st(ks->ks_ch))
		{
			TDsetKS(ks, (i4)ITinbuf[--ITbufptr], 2);
		}
	}
	else
	{
		chr = TEget(IIFTntsNumTmoutSecs);
		if (chr == EOF)
		{
			/*
			**  This should get us out of this routine
			**  so we won't have to worry about setting
			**  up a dummy return value.
			*/
			EXsignal(EXTEEOF, 0);
		}
		if (chr == TE_TIMEDOUT)
		{
			IIFTtmout = TRUE;
			chr = ' ';
		}

		TDsetKS(ks, chr, 1);
		/*
		** If timeout occurred above, than ks has been set to
		** a blank, and thus not a dbl1st.
		*/
		if (CMdbl1st(ks->ks_ch))
		{
			chr = TEget(IIFTntsNumTmoutSecs);
			if (chr == EOF)
			{
				/*
				**  This should get us out of this routine
				**  so we won't have to worry about setting
				**  up a dummy return value.
				*/
				EXsignal(EXTEEOF, 0);
			}
			if (chr == TE_TIMEDOUT)
			{
				IIFTtmout = TRUE;
				TDsetKS(ks, (i4)' ', 1);
			}
			else
			{
				TDsetKS(ks, chr, 2);
			}
		}
	}

	return(ks);
}


ITpushback(buf)
u_char	*buf;
{
	u_char	*cp;
	u_char	*ip;

	if ((ITbufptr + STlength((char *) buf)) > ITBUFSIZE)
	{
		SIprintf(ERget(E_TD0001_Buffer_overflow));
	}
	else
	{
		for (cp = buf; *cp; cp++)
		{
		}
		cp--;
		ip = &ITinbuf[ITbufptr];
		for (; cp >= buf; ITbufptr++)
		{
			*ip++ = *cp--;
		}
	}
}


KEYSTRUCT   *
ITin()
{
	u_char		*cp1;
	u_char		*cp2;
	i4		found;
	u_char		inbuf[40];
	KEYSTRUCT	*ks;

	if (ZZA)
	{
		ks = ITnxtch();
		CMcpychar(ks->ks_ch, inbuf);
		cp1 = inbuf;
		cp2 = (u_char *) ZZA;
		found = TRUE;
		if (!CMcmpcase(cp1, cp2))   /* (*cp1 == *cp2) */
		{
			CMnext(cp2);
			while(*cp2)
			{
				CMnext(cp1);
				CMcpychar((ITnxtch())->ks_ch, cp1);
				if (!CMcmpcase(cp1, cp2))   /* (*cp1 == *cp2) */
				{
					CMnext(cp2);
				}
				else
				{
					found = FALSE;
					break;
				}
			}
			if (!found)
			{
				CMnext(cp1);
				*cp1 = '\0';
				ITpushback(inbuf);
				ks = ITnxtch();
			}
			else
			{
				ks = ITnxtch();
				ks->ks_ch[0] = (u_char) (ks->ks_ch[0] + ZZC);
			}
		}
		}
	else
	{
		ks = ITnxtch();
	}
	if (ITdebug)
	{
		SIprintf(ERx("ITin returns %x\n"), (i4) ks->ks_ch[0]);
	}
	return(ks);
}
