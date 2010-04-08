/*
**  FTmessage
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	05/02/87 (dkh) - Changed to call FD routines via passed pointers.
**	10/17/87 (dkh) - Added call to TEinflush.
**	22-jan-88 (bruceb)
**		Allow completion by a newline as well as a return.
**	09-nov-88 (bruceb)
**		Set number of seconds until timeout.
**		If timeout then occurs, set IIfrscb's event to fdopTMOUT and
**		return.
**	10-mar-89 (bruceb)
**		Don't wish to alter the 'last command' contents for
**		FTmessage.  Use dummy evcb for testing if timeout has
**		occurred thus leaving IIFTevcb->event alone.
**	09/23/89 (dkh) - Porting changes integration.
**	22-mar-90 (bruceb)
**		Added locator support.  Clicking anywhere on the message line
**		terminates the waiting.
**	04/03/90 (dkh) - Integrated MACWS changes.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**	28-aug-1990 (Joe)
**	    Changed IIUIgdata to a function.
**	30-aug-1990 (Joe)
**	    Changed the name of IIUIgdata to IIUIfedata.
**	05-jun-92 (leighb) DeskTop Porting Change:
**		Changed COORD to IICOORD to avoid conflict
**		with a structure in wincon.h for Windows/NT
**	10-dec-92 (leighb) DeskTop Porting Change:
**		Use Windows MessageBox for 'wait' msgs if DLGBOXPRMPT defined.
**	 5-feb-93 (rudyw) Fix Bug 49346
**		Calling routine IImessage had it's buffer lengths increased
**		and a corresponding increase must be done here to 'lbuf' to
**		avoid the possibility of writing past the end of buffer.
**      17-aug-95 (tutto01)
**              Added Windows NT to ifdef's to add message box support.
**	24-sep-95 (tutto01)
**		Removed obsolete defines for Windows NT.
**	11-jan-1996 (toumi01; from 1.1 axp_osf port)
**		Added 10-sep-93 (kchin)
**		Changed type of arguments: a1-a10 in FTmessage()
**		from type i4  to PTR.  As these arguments will be used to
**		hold pointers.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	<frsctblk.h>
# include	<te.h>
# include	<kst.h>
# include	<uigdata.h>
# include	<er.h>
# if defined (DLGBOXPRMPT)
# include	<wn.h>
# endif


GLOBALREF	FRS_EVCB	*IIFTevcb;

FUNC_EXTERN	KEYSTRUCT	*FTgetch();
FUNC_EXTERN	VOID   		TEinflush();
FUNC_EXTERN	bool   		IIFTtoTimeOut();
FUNC_EXTERN	VOID   		IIFTstsSetTmoutSecs();
FUNC_EXTERN	VOID   		IITDpciPrtCoordInfo();
FUNC_EXTERN	VOID   		IITDposPrtObjSelected();

VOID
FTmessage(s, bell, wait, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
char	*s;
bool	bell;
bool	wait;
PTR	a1, a2, a3, a4, a5, a6, a7, a8, a9, a10;
{
	KEYSTRUCT	*key;
	char		lbuf[DB_MAXSTRING + 2];
	char		buf[FE_PROMPTSIZE + 1];
	IICOORD		done;
	FRS_EVCB	dummy;

	if (bell)
	{
		FTbell();
	}

	(*FTdmpmsg)(s, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);

	/*
	**  Only internal routines call this routine with parameters
	**  and these are guaranteed to fit in the buffer space.
	*/
	STlcopy(s, buf, FE_PROMPTSIZE);
	STprintf(lbuf, buf, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);

# ifdef DATAVIEW
	if (IIMWimIsMws())
	{
		if (wait)
		{
			_VOID_ IIMWfiFlushInput();
			_VOID_ IIMWdmDisplayMsg(lbuf, FALSE,
				(FRS_EVCB *) NULL);
		}
		else
		{
			_VOID_ IIMWfmFlashMsg(lbuf, FALSE);
		}
		return;
	}
# endif	/* DATAVIEW */

# if defined (DLGBOXPRMPT)
	if (wait && (IIFTevcb->timeout == 0))
	{
		WNMsgBoxOK(lbuf, "");
		return;
	}
# endif
	TDwinmsg(lbuf);

	if (wait)
	{
		IIFTstsSetTmoutSecs(IIFTevcb);

		/*
		**  Flush input so user type ahead does not
		**  clear error message.
		*/
		if (!IIUIfedata()->testing)
		{
			TEinflush();
		}

		done.begy = done.endy = stdmsg->_begy;
		done.begx = stdmsg->_begx;
		done.begx = stdmsg->_begx + stdmsg->_maxx - 1;
		IITDpciPrtCoordInfo(ERx("FTmessage"), ERx("done"), &done);

		for (;;)
		{
		    key = FTgetch();
		    if (IIFTtoTimeOut(&dummy)
			|| key->ks_ch[0] == '\r' || key->ks_ch[0] == '\n')
		    {
			break;
		    }
		    if (key->ks_fk == KS_LOCATOR)
		    {
			if (key->ks_p1 == done.begy)
			{
			    IITDposPrtObjSelected(ERx("done"));
			    break;
			}
			else
			{
			    FTbell();
			}
		    }
		}
	}
}
