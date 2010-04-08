/*
** Copyright (c) 1985, 2008 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<cv.h>
# include	<cm.h>
# include	<st.h>
# include	<si.h>
# include	<me.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	<ctrl.h>
# include	<ft.h>
# include	<frsctblk.h>
# include	<mapping.h>
# include	"ftfuncs.h"
# include	<erft.h>
# include	<te.h>
# include	<uigdata.h>
# ifdef	DLGBOXPRMPT
# include	<wn.h>
# endif

/**
** Name:    ftboxerr.c -	FT Output Error Message in Pop-Up Box.
**
**  Description:
**	This will first put out a single line error message, which
**	the user can erase with the Return or frskey3 key.  If they
**	choose help (frskey1), they can get a small window at the
**	bottom of the screen which will display longer text of an
**	error message.
**
**	This routine rebuilds the windows needed for the errors
**	each time, as the size may change at each invocation.
**
**	If the shortstring is not specified or empty, control
**	goes directly to the long string.  If the longstring is not
**	specified, or empty, no help is available when the
**	short string is specified.
**
**  Inputs:
**	shortstring	- string to print for single line.
**			  NULL if no single line message.
**	longstring	- string to print for long message.
**			  NULL if no window message.
**	frscb		- Forms system control block.
**
**  Outputs:
**	OK		if succeeded
**	FAIL		if anything failed.
**
**  History:
**	02-nov-1985 - (dkh)
**		Added calls to FTdmpmsg to put more information into
**		screen dump files for testing purposes.
**	06-mar-1987 (dkh) - Added support for ADTs.
**	26-mar-1987 (peter)
**		Taken from fterror.c.  Add second parameter and
**		single line message.
**	18-apr-1987 (dkh) - Changed to use the KEYSTRUCT abstraction.
**	05/02/87 (dkh) - Changed to call FD routines via passed pointers.
**	06/19/87 (dkh) - Code cleanup.
**	13-jul-87 (bruceb) Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	10/14/87 (dkh) - Added flushing of input.
**	11/04/87 (dkh) - Changed screen recovery after error occurs.
**	02/02/88 (dkh) - Fixed jup bug 1913. (dkh)
**	02/24/88 (dkh) - Fixed jup bug 1963. (dkh)
**	02/25/88 (dkh) - Put in Pop-up messages and prompts support.
**	07-apr-88 (bruceb)
**		Changed from using sizeof(DB_TEXT_STRING)-1 to using
**		DB_CNTSIZE.  Previous calculation is in error.
**	05/27/88 (dkh) - Changed to put cursor at beginning of HIT RETURN
**			 message for popup prompts, messages and long error
**			 messages.  Also, reorganized logging of error
**			 messages into testing .scr files.
**	07/09/88 (dkh) - Added recognition of fdopREFR.
**	07/22/88 (dkh) - Implemented saveunder for popup prompts and messages.
**	09/05/88 (dkh) - Changed popup prompts to use saveunder properly and
**			 set popup prompt input window to be absolute.
**	21-nov-88 (bruceb)
**		Set timeout value if it has changed.  If timeout occurs then
**		return.  Event control block is updated to indicate timeout to
**		calling routines.
**	10-mar-89 (bruceb)
**		Save and restore event value in FTboxerr().  If timeout
**		should happen to occur, still want 'resume next' to work.
**		NOT changing 'last command' for an error message.
**	27-jun-89 (bruceb)	Fix for bug 6443.
**		Take _starty into consideration in IIFTsuSaveUnder().
**	07/11/89 (dkh) - Added changes for PV to dump screen for testing
**			 boxed messages and prompts, changed dumping of
**			 short message for FTboxerr() and fix the
**			 EOS problem when dumping a long message.
**	07/11/89 (brett)
**		Added hotspot command strings for windex.  These will
**		allow a user to activate either the error line or
**		extended error box with the mouse.
**	07/12/89 (dkh) - Integrated windex code changes from Brett into 6.2.
**	07/22/89 (dkh) - Fixed bug 6888.
**	07/27/89 (dkh) - Added special marker parameter in call to fmt_multi.
**	07/27/89 (dkh) - Moved call to FTdmpcur() to pick up message correctly.
**	08/25/89 (dkh) - Save menu line as well as part of support
**			 for derived fields.
**	09/23/89 (dkh) - Porting changes integration.
**	11-jan-90 (bruceb)	Fix for bug 9415.
**		Don't muck with maxy in IIFTpum() unless it's too big.
**		Use maxy instead of nlines in subwin calculations.
**	01-feb-90 (bruceb)
**		Moved TEflush's into FTbell.
**	02/22/90 (dkh) - Fixed bug 9749.
**	03/01/90 (dkh) - Changed to allocate the correct size for popup
**			 message and prompt windows for 80/132 support.
**	05-mar-90 (bruceb)
**		Use STlength rather than hardwired '12' for '[HIT RETURN]'.
**		Removed unused 'long_len' var from FTboxerr.  Added a
**		'return OK' to FTboxerr at end of routine.
**	12-mar-90 (bruceb)
**		Changed so that var used for hotspots is guaranteed to be set
**		when used.
**	13-mar-90 (bruceb)
**		Added locator support for the error line and popup messages.
**	30-apr-90 (bruceb)
**		Added locator support for prompts.
**	04/03/90 (dkh) - Integrated MACWS changes.
**	08-jun-90 (bruceb)
**		Locator support changed so that clicking anywhere on the
**		bottom line except on 'More' will generate an 'End'.
**	08/15/90 (dkh) - Fixed bug 21670.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**	08/30/90 (dkh) - Integrated porting change ingres6202p/130036.
**			 This adds support for HP terminals in HP mode.
**	10/09/90 (dkh) - Rearrange some code so that test output will
**			 capture the image of a popup message again.
**			 This gotten broken with integration of support
**			 for hp terminals.
**	10/26/90 (dkh) - Fixed bug 34162.
**      11/12/90 (stevet) - Fixed bug 34400.
**      3/21/91 (elein)         (b35574) Add TRUE parameter to call to
**                              fmt_multi. TRUE is used internally for boxed
**                              messages only.  They need to have control
**                              characters suppressed.
**	04/26/91 (dkh) - Changed to use MEreqmem for fmt workspace allocation
**			 so we can free it up when we are done displaying
**			 an error message.
**	08/19/91 (dkh) - Integrated changes from PC group.
**	20-feb-92 (mgw) Bug #42340
**		Initialize muline[] char array in FTboxerr() to prevent
**		garbage from displaying on the menu line after certain
**		popups.
**      21-apr-92 (kevinm/jillb/jroy--DGC)
**            Changed fmt_init to IIfmt_init since it is a fairly global
**            routine.  The old name, fmt_init, conflicts with a Green Hills
**            Fortran function name.
**	05-jun-92 (leighb) DeskTop Porting Change:
**		Changed COORD to IICOORD to avoid conflict
**		with a structure in wincon.h for Windows/NT
**	09/20/92 (dkh) - Fixed bug 43657.  Mapping control keys to frskeys
**			 will now work correctly for error message handling.
**	08/20/93 (dkh) - Fixing NT compile problems.  Changed _leave
**			 to lvcursor for the WINDOW structure.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**      14-mar-95 (peeje01: jmbp)
**              + rework of Double byte fix prior to integration. Fix comment
**                follows and is from: 11/20/93 (mikehan)
**		- Double-byte short error message is displayed incorrectly
**		  if it is longer than half the screen length. Normally, if a
**		  error message is too long to fit on the screen, it will
**		  be cut off word by word, and this method is based on the
**		  fact that enlish words are separated by spaces while, for
**		  example, japanese words are not. So, add some extra cases 
**		  to check for double-byte messages as well. 
**		- Scanning backwards does not help either because a byte can 
**		  only be guaranteed to be the first byte of a double-byte 
**		  character if we actually scan the string from the start.
**                  + therefore use CMprev() which is costly for DOUBLEBYTE
**                    but shouldn't have to be called all that often. (jmbp)
**		- The additional code is in FTboxerr();
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
*/


FUNC_EXTERN	i4		TDrefresh();
FUNC_EXTERN	i4		TDclear();
FUNC_EXTERN	WINDOW		*TDsubwin();
FUNC_EXTERN	WINDOW		*TDnewwin();
FUNC_EXTERN	ADF_CB		*FEadfcb();
FUNC_EXTERN	KEYSTRUCT	*FTTDgetch();
FUNC_EXTERN	KEYSTRUCT	*FTgetc();
FUNC_EXTERN	u_char		*FTuserinp();
FUNC_EXTERN	STATUS		IIFTpumPopUpMessage();
FUNC_EXTERN	STATUS		IIFTpupPopUpPrompt();
FUNC_EXTERN	bool		IIFTtoTimeOut();
FUNC_EXTERN	VOID		IIFTstsSetTmoutSecs();
FUNC_EXTERN	VOID		IITDpciPrtCoordInfo();
FUNC_EXTERN	i4		IITDlioLocInObj();
FUNC_EXTERN	VOID		IITDposPrtObjSelected();
FUNC_EXTERN	VOID		IIFTsplSetPromptLoc();

GLOBALREF	i4	IITDAllocSize;

static	VOID		free_mem();
static	bool		FTisref();

static	WINDOW		*IIpopwin = NULL;
static	WINDOW		*IIpmsgwin = NULL;
static	WINDOW		*IIputilwin = NULL;
static	WINDOW		*IIsavewin = NULL;
static	i4		IIypwin = 0;
static	i4		IIxpwin = 0;


static bool
FTisref(ks)
KEYSTRUCT	*ks;
{
	i4	inx;
	u_char	inchr;

	if (ks->ks_fk == 0)
	{
	    inchr = ks->ks_ch[0];
	    if (inchr >= ctrlA && inchr <= ctrlZ)
	    {
		inx = inchr - ctrlA;
	    }
	    else if (inchr == ctrlESC)
	    {
		inx = CTRL_ESC;
	    }
	    else if (inchr == ctrlDEL)
	    {
		inx = CTRL_DEL;
	    }
	    else
	    {
		return(FALSE);
	    }
	}
	else
	{
	    inx = ks->ks_fk - KEYOFFSET + MAX_CTRL_KEYS;
	}
	if ((&keymap[inx])->cmdval == fdopREFR)
	{
	    return(TRUE);
	}
	return(FALSE);
}





/*{
** Name:	IIFTsuSaveUnder - Save display under popup
**
** Description:
**	This routine will save the curscr screen information that a
**	popup message or prompt will obscure.  Information will
**	later be restored by a call to TDoverwrite.
**
** Inputs:
**	popwin	Window that will obscure information on screen.
**	savewin	Window to save things in.
**
** Outputs:
**	None.
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	07/22/88 (dkh) - Initial version.
*/
VOID
IIFTsuSaveUnder(popwin, savewin)
WINDOW	*popwin;
WINDOW	*savewin;
{
	i4	y;
	i4	maxy;
	i4	maxx;
	i4	begy;
	i4	begx;

	/*
	**  It is assumed that savewin is big enough.
	*/

	maxx = popwin->_maxx;
	maxy = popwin->_maxy;
	begy = popwin->_begy + popwin->_starty;
	begx = popwin->_begx;

	for (y = 0; y < maxy; y++)
	{
		MEcopy(&(curscr->_y[begy + y][begx]), maxx, savewin->_y[y]);
		MEcopy(&(curscr->_da[begy + y][begx]), maxx, savewin->_da[y]);
		MEcopy(&(curscr->_dx[begy + y][begx]), maxx, savewin->_dx[y]);
	}
}




/*
** History:
**	28-aug-1990 (Joe)
**	    Changed IIUIgdata to a function.
**	30-aug-1990 (Joe)
**	    Changed the name of IIUIgdata to IIUIfedata.
**	10-dec-92 (leighb) DeskTop Porting Change:
**	    Call Windows MessageBox if DLGBOXPRMPT is defined.
*/
STATUS
FTboxerr (shortmsg, longmsg, frscb)
char	*shortmsg;
char	*longmsg;
FRS_CB	*frscb;
{
    i4		shrt_len;
    STATUS	stat;
    i4		event;
    i4		index;
    struct mapping *mptr;
    char	muline[MAX_TERM_SIZE + 1];
    IICOORD	done;		/* Location of the 'End' message. */
    IICOORD	more_loc;	/* Location of the 'More' message. */
    IICOORD	*more = &more_loc;

    muline[0] = EOS;	/* For bug 42340 */

# ifdef DATAVIEW
    if (IIMWimIsMws())
    {
	_VOID_ IIMWfiFlushInput();
	return(IIMWmwhMsgWithHelp(shortmsg, longmsg, frscb->frs_event));
    }
# endif	/* DATAVIEW */

    /*
    **  Flush any typeahead first, but only if we are not testing.
    */
    if (!IIUIfedata()->testing)
    {
	TEinflush();
    }

    if (shortmsg != NULL && (shrt_len = STlength(shortmsg)) < 1)
    {
	shortmsg = NULL;
    }
    if (longmsg != NULL && STlength(longmsg) < 1)
    {
	longmsg = NULL;
    }

# ifdef	DLGBOXPRMPT
    if (frscb->frs_event->timeout == 0)
    {
	FTbell();
	WNMsgBoxOK(longmsg ? longmsg : (shortmsg ? shortmsg : "Error"), NULL);
	return OK;
    }
# endif


    (*FTdmpmsg)(ERx("\nDISPLAYING ERROR MESSAGE VIA IIFDERROR.\n"));

    if (shortmsg != NULL)
    {
	char	*cp;
        i4      clen;
	i4	plen;
	i4	slen;
	char	bufr[MAX_TERM_SIZE + 1]; /* bufr to store formatted record */
	char		menu[MAX_TERM_SIZE + 1];
	i4		end_len;	/* brett, windex */

	static const char	_Ellipses[] = ERx("... ");
	static const char	_MenuItem[] = ERx(" %s(%s)");

	/*
	**  Save menu line for later restoration.
	*/
	TDwin_str(stdmsg, muline, (bool)TRUE);

	event = frscb->frs_event->event;

	IIFTstsSetTmoutSecs(frscb->frs_event);

	/*
	**	Put out the short message on the message line, and look for the
	**	Help key to be selected.  This fakes itself as a menu input to
	**	accept FRSkeys.	 If it is not, return.
	*/

	_VOID_ STprintf(menu, _MenuItem, ERget(FE_End), ERget(F_FT0001_Return));
	end_len = STlength(menu);

	if (longmsg != NULL)
	{
            /*
            **	Get the mapping for the Help key (FRSKEY1) so we can use it.
            */

	    reg struct mapping	*hkey;
	    reg i4		keyi;

	    hkey = NULL;
	    for (keyi = 0 ; keyi < (MAX_CTRL_KEYS + KEYTBLSZ + 4) ; ++keyi)
	    {
	        if ((&keymap[keyi])->cmdval == ftFRS1)
	        {
		    hkey = &keymap[keyi];
		    break;
	        }
	    }

	    /*
	    ** brett, windex, need this for hot spots later.
	    */
	    _VOID_ STprintf(menu + end_len, _MenuItem,
		    ERget(F_FT0002_More), (hkey != NULL && hkey->label != NULL)
			? hkey->label : ERget(F_FT0003_MoreKey));
	}

	/*
	**  If the menuitem stuff for the short message is longer
	**  than half the screen width, then arbitrarily cut it back
	**  to half the screen width.
	*/
	if ((plen = STlength(menu) + 5) > (COLS - 1)/2)
	{
		plen = (COLS - 1)/2;
	}

	clen = CMcopy(shortmsg, COLS - plen - (sizeof(_Ellipses) - 1), bufr);
	*(bufr+clen) = EOS;
	for (cp = bufr ; *cp != EOS ; CMnext(cp))
	{ /* Terminate short message at tab or newline */
	    if (*cp == '\n' || *cp == '\t')
	    {
		*cp = EOS;
		break;
	    }
	}
	slen = cp - bufr;
	if (shrt_len > slen)
	{ /* Total string was too large ... */
	    if (slen < clen)
	    { /* ... but terminated short message fits */
		*cp = ' ';
		if (*++cp != EOS)
		    *cp++ = ' ';
		*cp = EOS;
	    }
	    /* jmbp 14-mar-95:
	    ** step backwards through the string looking for the next place
	    ** to terminate it. Note use of CMprev() instead of cp--.
	    */
	    else for (cp = bufr + slen - 1 ; cp > bufr ; CMprev(cp, bufr))
	    { /* else don't end in the middle of a word */

		/* jmbp 14-mar-95:
		** Terminate on white space or at
		** the beginning of the next dbl byte char 
		*/
		if (CMwhite(cp) || CMdbl1st(cp))
		{
		    /* jmbp 14-mar-95: Put a space incase CMwhite succeeded */
		    *(cp) = ' ';
		    *(cp + 1) = EOS;
		    break;
		}
	    }
	    STcat(bufr, _Ellipses);
	    slen = (cp - bufr) + sizeof(_Ellipses) - 1;
	}

	(*FTdmpmsg)(ERx("\nSHORT ERROR MESSAGE IS:\n"));
	(*FTdmpmsg)(ERx("%s\n"), bufr);
	(*FTdmpmsg)(ERx("\nEND OF SHORT ERROR MESSAGE.\n"));

	MEfill(COLS - plen - slen, ' ', bufr + slen);
	STlcopy(menu, &bufr[COLS - plen], plen);

	/*	Add message to window buffer */

	/*
	** Tue Jul 11 22:10:07 PDT 1989
	** brett, windex command string for a hotspot.
	** Allow user to mouse activate the error menu line.
	*/
	IITDhotspot(COLS-plen+1, LINES-1, end_len-1, 1, "\r", 1);

	done.begy = done.endy = LINES - 1;
	done.begx = 0;
	done.endx = COLS - 1;
	IITDpciPrtCoordInfo(ERx("FTboxerr"), ERx("done"), &done);

	if (longmsg != NULL)
	{
	    IITDhotspot(COLS-plen+end_len+1, LINES-1, plen-end_len-6,
		1, "h", 1);

	    more->begy = more->endy = LINES - 1;
	    more->begx = COLS - plen + 1 + end_len;
	    more->endx = COLS - 6;
	    IITDpciPrtCoordInfo(ERx("FTboxerr"), ERx("more"), more);
	}
	else
	{
	    more = NULL;
	}

	FTbell();
	TDerase(stdmsg);		/* erase previous contents */
	TDaddstr(stdmsg, bufr);		/* add the prompt */
	TDsetattr(stdmsg, (i4) fdRVVID);/* reverse video */
	TDrefresh(stdmsg);		/* refresh the window */

	for (;;)
	{ /* Loop until FRSKEY3 or Return or FRSKEY1 */
	    reg KEYSTRUCT	*ks;		/* Input character */

	    ks = FTTDgetch(stdmsg);
	    if (ks->ks_ch[0] == '\r' || ks->ks_ch[0] == '\n'
#ifdef PMFE
		|| ((&keymap[ks->ks_fk - KEYOFFSET + MAX_CTRL_KEYS])->cmdval ==
			fdopMENU)
# endif
		|| IIFTtoTimeOut(frscb->frs_event))
	    {
		/*
		** Tue Jul 11 22:17:36 PDT 1989
		** brett, windex command string to remove hotspots.
		*/
		IITDkillspots();
		/*
		**  Clear bottom line so user will see visual
		**  feedback that we have accepted the return
		**  key that they typed.
		**
		**  Visually, timeout is identical here to hitting a CR.
		*/
		TDerase(stdmsg);
		TDaddstr(stdmsg, muline);
		TDrefresh(stdmsg);
		TDclrattr();
		/* Only IIFTtoTimeOut() may modify event and only if timeout. */
		frscb->frs_event->event = event;
		return OK;
	    }

	    CVlower(ks->ks_ch);
	    if (ks->ks_ch[0] == 'h')
	    {
		break;
	    }

	    if (ks->ks_fk != 0)
	    {
		reg i4	keyi;

		if (ks->ks_fk == KS_LOCATOR)
		{
		    if (more && IITDlioLocInObj(more, ks->ks_p1, ks->ks_p2))
		    {
			/* Long message requested. */
			IITDposPrtObjSelected(ERx("more"));
			break;
		    }
		    else if (IITDlioLocInObj(&done, ks->ks_p1, ks->ks_p2))
		    {
			IITDposPrtObjSelected(ERx("done"));
			/* No need to clear hotspots; not running windex. */
			TDerase(stdmsg);
			TDaddstr(stdmsg, muline);
			TDrefresh(stdmsg);
			TDclrattr();
			return OK;
		    }
		    else
		    {
			FTbell();
			continue;
		    }
		}

		keyi = ks->ks_fk - KEYOFFSET + MAX_CTRL_KEYS;
		if ((&keymap[keyi])->cmdval == ftFRS1)
		{
		    break;
		}
		if ((&keymap[keyi])->cmdval == ftFRS3)
		{
		    /*
		    ** Tue Jul 11 22:17:36 PDT 1989
		    ** brett, windex command string to remove hotspots.
		    */
		    IITDkillspots();
		    TDerase(stdmsg);
		    TDaddstr(stdmsg, muline);
		    TDrefresh(stdmsg);
		    TDclrattr();
		    return OK;
		}
	    }

	    /*
	    **  Now check to see if we have control key that is mapped
	    **  to either frskey1 or frskey3.
	    **
	    **  Note that we don't worry about mappings for EBCDIC
	    **  since FEs no longer run in that environment.
	    */
	    index = -1;
	    if (ks->ks_ch[0] == ctrlDEL)
	    {
		index = CTRL_DEL;
	    }
	    else if (ks->ks_ch[0] == ctrlESC)
	    {
		index = CTRL_ESC;
	    }
	    else if (ks->ks_ch[0] <= ctrlZ &&
# ifdef PMFE
		ks->ks_ch[0] != ctrlU &&
# endif
		ks->ks_ch[0] >= ctrlA)
	    {
		index = ks->ks_ch[0] - ctrlA;
	    }

	    if (index >= 0)
	    {
		mptr = &keymap[index];
		if (mptr->cmdval == ftFRS1)
		{
		    break;
		}
		else if (mptr->cmdval == ftFRS3)
		{
		    IITDkillspots();
		    TDerase(stdmsg);
		    TDaddstr(stdmsg, muline);
		    TDrefresh(stdmsg);
		    TDclrattr();
		    return(OK);
		}
	    }

	    if (FTisref(ks))
	    {
		TDrefresh(curscr);
	    }
	    else
	    {
	    	FTbell();
	    }
	} /* end input for */
    } /* end short message display */

    /*
    ** Tue Jul 11 22:17:36 PDT 1989
    ** brett, windex command string to remove hotspots.
    */
    IITDkillspots();
    /*
    **  Clear out terminal display attributes as a nicety to
    **  users in case they want to do their own write to
    **  the screen.
    */
    TDclrattr();

    /*
    **  Call IIFTpumPopUpMessage() to display long message.
    */
    if (longmsg != NULL)
    {
	/*
	**  Remove short message before displaying long one.
	*/
	TDwinmsg(ERx(" "));

	event = frscb->frs_event->event;
	stat = IIFTpumPopUpMessage(longmsg, FALSE, POP_DEFAULT, POP_DEFAULT,
		POP_DEFAULT, POP_DEFAULT, frscb, TRUE);
	frscb->frs_event->event = event;
	/*
	**  Restore menu line befor exiting.
	*/
	TDerase(stdmsg);
	TDaddstr(stdmsg, muline);
	TDrefresh(stdmsg);
	if (!XS)        /* HP terminals are better off without this */
	{
		TDclrattr();
	}
	return(stat);
    }

    return OK;
}



/*{
** Name:	IIFTpumPopUpMessage - Display a popup message
**
** Description:
**	Displays a message in a window on the terminal.  If
**	this is part of a popup prompt then don't display
**	the "HIT RETURN" message.
**
** Inputs:
**	msg		Message to display
**	isprompt	Is this for a popup prompt.
**	begy		Beginning row position of popup.
**	begx		Beginning column position of popup.
**	maxy		Number of rows popup occupies.
**	maxx		Number of columns popup occupies.
**
** Outputs:
**	None.
**
**	Returns:
**		OK	If popup is possible
**		FAIL	If popup is not possible
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/25/88 (dkh) - Initial version.
*/
STATUS
IIFTpumPopUpMessage(msg, isprompt, begy, begx, maxy, maxx, frscb, iserror)
char	*msg;
i4	isprompt;
i4	begy;
i4	begx;
i4	maxy;
i4	maxx;
FRS_CB	*frscb;
i4	iserror;
{
	ADF_CB		*cb;
	DB_DATA_VALUE	ldbv;
	PTR		wksp;
	DB_TEXT_STRING	*text;
	bool		more;
	reg i4		nlines;
	i4		len;
	i4		msglen;
	char		msgline[MAX_TERM_SIZE + 1];
	char		fmtstr[60];
	i4		fmtsize;
	i4		retstart;
	i4		retlen;
	PTR		blk;
	KEYSTRUCT	*ks;		/* Input character */
	i4		keyi;
	FMT		*IIerrpfmt;
	DB_DATA_VALUE	IIerrdbv;
	bool            def_maxy;
	IICOORD		msgbox;		/* Location of the popup message. */
	i4		zx;		/* Work variables for "stretching */
	i4		zy;		/* the popup window */
	i4		zz;


# ifdef DATAVIEW
	if (IIMWimIsMws())
		return(IIMWdmDisplayMsg(msg, FALSE, frscb->frs_event));
# endif	/* DATAVIEW */

	cb = FEadfcb();

	/*
	**  Calculate the placement and size of window to display
	**  message in.
	*/
	maxx = (maxx == POP_DEFAULT) ? COLS - 2 : ((maxx > COLS) ? COLS : maxx);

	def_maxy = (maxy == POP_DEFAULT);
	maxy = def_maxy ? LINES - 1 : ((maxy > LINES - 1) ? LINES - 1 :maxy);

	/*
	**  Set up the FMT workspace and structure.  Since the
	**  size of the window can change on each call, we need
	**  to recalculate each time.
	*/
	_VOID_ STprintf(fmtstr, ERx("cf0.%d"), maxx - 4);
	if (fmt_areasize(cb, fmtstr, &fmtsize) != OK ||
		 ((blk = MEreqmem(0, fmtsize, TRUE, (STATUS *)NULL)) == NULL) ||
		fmt_setfmt(cb, fmtstr, blk, &IIerrpfmt, &len) != OK )
	{
		return(FAIL);
	}
	IIerrdbv.db_datatype = DB_LTXT_TYPE;
	IIerrdbv.db_prec = 0;
	IIerrdbv.db_length = maxx - 4 + DB_CNTSIZE;

	if ((IIerrdbv.db_data = MEreqmem(0, IIerrdbv.db_length, TRUE,
		(STATUS *)NULL)) == NULL)
	{
		MEfree(blk);
		return(FAIL);
	}

	/*
	**  Now set up the DB_DATA_VALUE for the message string
	**  to use in the formatting routines.
	*/

	msglen = STlength(msg);

	ldbv.db_datatype = DB_CHR_TYPE;
	ldbv.db_length = msglen;
	ldbv.db_prec = 0;
	ldbv.db_data = (PTR) msg;

	fmt_workspace(cb, IIerrpfmt, &ldbv, &len);
	if ((wksp = MEreqmem(0, len, TRUE, (STATUS *)NULL)) == NULL)
	{
	    MEfree(blk);
	    MEfree(IIerrdbv.db_data);
	    return(FAIL);
	}

	/*
	**  First see how many lines are in the message in
	**  order to allocate the right size window.
	*/

	IIfmt_init(cb, IIerrpfmt, &ldbv, wksp);
	for (nlines = 0 ;; ++nlines)
	{
	    if (fmt_multi(cb, IIerrpfmt, &ldbv, wksp, &IIerrdbv, &more,
		FALSE, TRUE) != OK)
	    {
		free_mem(blk, IIerrdbv.db_data, wksp);
		return(FAIL);
	    }
	    else if (!more)
	    {
		break;
	    }
	}

	/* Don't allow the window to get larger than the screen. */

	if (nlines > maxy - 3)
	{
	    nlines = maxy - 3;
	}
	/*
	** Set maxy to a true size if user requested the default.  If user
	** requested a specific size, however, leave that size alone.
	*/
	if (def_maxy)
	{
	    maxy = nlines + 3;
	}

	/*
	**  Now set up the windows to fit the size of the message.
	**  First, check to see if the windows have been allocated
	**  yet.
	*/
	if (IIpopwin == NULL)
	{
		if (TDonewin(&IIpopwin, &IIpmsgwin, &IIputilwin,
			LINES - 1, IITDAllocSize) != OK)
		{
			free_mem(blk, IIerrdbv.db_data, wksp);
			return(FAIL);
		}
		IIpopwin->lvcursor = FALSE;
		IIypwin = LINES - 1;
		IIxpwin = COLS;

		if ((IIsavewin = TDnewwin(LINES, IITDAllocSize, 0, 0)) == NULL)
		{
			free_mem(blk, IIerrdbv.db_data, wksp);
			return(FAIL);
		}

	}
	else
	{
		TDerase(IIpopwin);
	}

	IIpopwin->_maxy = maxy;
	IIpopwin->_maxx = maxx;

	begy = (begy == POP_DEFAULT) ? LINES - maxy - 1 : begy - 1;
	begx = (begx == POP_DEFAULT) ? 1 : begx - 1;

	if (maxy + begy > LINES - 1)
	{
		begy = LINES - 1 - maxy;
	}
	if (maxx + begx > COLS)
	{
		begx = COLS - maxx;
	}

	IIpopwin->_begy = begy;
	IIpopwin->_begx = begx;

	IIpopwin->_relative = FALSE;	/* window is absolute */
	IIputilwin->_relative = FALSE;	/* input window must also be absolute */

	retlen = STlength(ERget(FE_HitEnter));
	if ((retstart = begx + maxx - (retlen + 7)) < begx)
	{
		retstart = begx;
	}

	if ((IIpmsgwin = TDsubwin(IIpopwin, nlines, maxx - 4,
		IIpopwin->_begy + 1, IIpopwin->_begx + 2, IIpmsgwin)) == NULL)
	{
	    free_mem(blk, IIerrdbv.db_data, wksp);
	    return(FAIL);
	}

	if (iserror)
	{
	    (*FTdmpmsg)(ERx("\nLONG ERROR MESSAGE IS:\n"));
	}

	TDbox(IIpopwin, '|', '-', '+');

	if (!isprompt)
	{
	    if ((IIputilwin = TDsubwin(IIpopwin, 1, retlen,
		IIpopwin->_begy + nlines + 1, retstart, IIputilwin)) == NULL)
	    {
		free_mem(blk, IIerrdbv.db_data, wksp);
		return(FAIL);
	    }
	    TDsetattr(IIputilwin, (i4) fdRVVID);
	    IIputilwin->_cury = IIputilwin->_curx = 0;
	    TDaddstr(IIputilwin, ERget(FE_HitEnter));
	}

	/*
	**  Put cursor at beginning of HIT RETURN message.
	*/
	IIpopwin->_cury = maxy - 2;
	IIpopwin->_curx = retstart - IIpopwin->_begx;

	IIpmsgwin->_cury = IIpmsgwin->_curx = 0;

	/*
	**	Put the text into the window
	*/

	text = (DB_TEXT_STRING *)IIerrdbv.db_data;
	IIfmt_init(cb, IIerrpfmt, &ldbv, wksp);
	while (nlines-- > 0)
	{
	    if (fmt_multi(cb, IIerrpfmt, &ldbv, wksp, &IIerrdbv, &more,
		FALSE) != OK)
	    {
		free_mem(blk, IIerrdbv.db_data, wksp);
		return(FAIL);
	    }
	    else if (!more)
	    {
		break;
	    }
	    TDxaddstr(IIpmsgwin, text->db_t_count, text->db_t_text);
	    if (iserror)
	    {
	    	MEcopy((PTR) text->db_t_text, (u_i2) text->db_t_count,
			(PTR) msgline);
		msgline[text->db_t_count] = EOS;
		(*FTdmpmsg)(ERx("%s\n"), msgline);
	    }
	}

	/*
	** [HP terminals only ... ]
	** 'Stretch' the popup window by copying characters and
	** attributes from the curscr WINDOW into the corresponding
	** locations in the IIpopwin WINDOW. Then we change maxx for
	** the popup window.
	*/
	if (XS)         /* HP terminals only */
	{
		for (zy = 0; zy < maxy; zy++)
		{
			for (zx = begx+maxx, zz = maxx; zx < curscr->_maxx;
				zx++, zz++)
			{
				IIpopwin->_y[zy][zz]=curscr->_y[begy+zy][zx];
				IIpopwin->_da[zy][zz]=curscr->_da[begy+zy][zx];
				IIpopwin->_dx[zy][zz]=curscr->_dx[begy+zy][zx];
				IIpopwin->_lastch[zy]=curscr->_maxx-begx-1;
			}
		}
		IIpopwin->_maxx = curscr->_maxx-begx;   /*right edge of screen*/
	}
	/*
	** Save the portion of the screen image that will be
	** 'under' the popup window. This must be done after
	** the HP terminal specific code above.
	*/
	IIFTsuSaveUnder(IIpopwin, IIsavewin);

	TDrefresh(IIpopwin);			/* Print error message */

	/*
	**  Don't move this if/else statement before the above TDrefresh
	**  call.  It is here so that we can capture the image of
	**  of the popup message in the test output.
	*/
	if (iserror)
	{
	    (*FTdmpmsg)(ERx("\nEND OF LONG ERROR MESSAGE.\n"));
	}
	else
	{
		(*FTdmpcur)();
	}

	if (isprompt)
	{
    		free_mem(blk, IIerrdbv.db_data, wksp);
		return(OK);
	}
	else
	{
		/*
		** Tue Jul 11 22:20:33 PDT 1989
		** brett, windex command string for a hotspot.
		** Allow user to mouse activate the popup window.
		*/
		IITDhotspot(IIpopwin->_begx, IIpopwin->_begy, IIpopwin->_maxx,
			IIpopwin->_maxy, "\r", 1);

		msgbox.begy = IIpopwin->_begy;
		msgbox.begx = IIpopwin->_begx;
		msgbox.endy = IIpopwin->_begy + IIpopwin->_maxy - 1;
		msgbox.endx = IIpopwin->_begx + IIpopwin->_maxx - 1;
		IITDpciPrtCoordInfo(ERx("IIFTpum"), ERx("msgbox"), &msgbox);

		IIFTstsSetTmoutSecs(frscb->frs_event);

		/*
		**  Wait for a carriage return or new line character.
		*/
		for(;;)
		{
	    		ks = FTgetc(stdin);
	    		if ((ks->ks_ch[0] == '\r') || (ks->ks_ch[0] == '\n')
#ifdef PMFE
			    || ((&keymap[ks->ks_fk - KEYOFFSET +
				MAX_CTRL_KEYS])->cmdval == fdopMENU)
#endif
			    || (IIFTtoTimeOut(frscb->frs_event)))
	    		{
				/*
				** Visually, timeout is identical here to
				** hitting a CR.
				*/
				break;
	    		}
	    		if (ks->ks_fk != 0)
	    		{
				if (ks->ks_fk == KS_LOCATOR)
				{
				    if (IITDlioLocInObj(&msgbox,
					ks->ks_p1, ks->ks_p2))
				    {
					IITDposPrtObjSelected(ERx("msgbox"));
					break;
				    }
				    else
				    {
					FTbell();
					continue;
				    }
				}

				keyi = ks->ks_fk - KEYOFFSET + MAX_CTRL_KEYS;
				if ((&keymap[keyi])->cmdval == ftFRS3)
				{
		    			break;
				}
	    		}
			if (FTisref(ks))
			{
				TDrefresh(curscr);
			}
			else
			{
	    			FTbell();
			}
		}
	}

	TDoverwrite(IIsavewin, IIpopwin);
	TDtouchwin(IIpopwin);
	TDrefresh(IIpopwin);		/* Clear bottom of the screen */
	TDerase(IIpopwin);		/* Clear window */

	free_mem(blk, IIerrdbv.db_data, wksp);
	/*
	** Tue Jul 11 22:20:33 PDT 1989
	** brett, windex command string to remove hotspots.
	*/
	IITDkillspots();
	return(OK);
}


/*{
** Name:	free_mem - Free up some memory
**
** Description:
**	Utility routine to free up some memory.  Just calls MEfree()
**	for each of the three passed in arguments.  It is assumed
**	to always works.  This routine is only used in this file.
**
** Inputs:
*	ptr1	Pointer to memory to free.
*	ptr2	Pointer to memory to free.
*	ptr3	Pointer to memory to free.
**
** Outputs:
**	None.
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/25/88 (dkh) - Initial version.
*/
static VOID
free_mem(ptr1, ptr2, ptr3)
PTR	ptr1;
PTR	ptr2;
PTR	ptr3;
{
	_VOID_ MEfree(ptr1);
	_VOID_ MEfree(ptr2);
	_VOID_ MEfree(ptr3);
}


/*{
** Name:	IIFTpupPopUpPrompt - Display a popup prompt
**
** Description:
**	This routine displays a popup prompt.  It calls FTuserinp()
**	to handle prompt input.  IIFTpumPopUpMessage() is called
**	to set up the display.
**
** Inputs:
**	prompt	Prompt string to display.
**	begy	Beginning row position.
**	begx	Beginning column position.
**	maxy	Number of rows popup occupies.
**	maxx	Number of columns popup occupies.
**	echo	Should input be echoed.
**
** Outputs:
**	result	- Result buffer for saving user input.
**
**	Returns:
**		OK	If popup ran successfully.
**		FAIL	If popup could not run.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/25/88 (dkh) - Initial version.
*/
STATUS
IIFTpupPopUpPrompt(prompt, begy, begx, maxy, maxx, echo, frscb, result)
char	*prompt;
i4	begy;
i4	begx;
i4	maxy;
i4	maxx;
i4	echo;
FRS_CB	*frscb;
char	*result;
{
	STATUS		stat = OK;
	char		*tp;

# ifdef DATAVIEW
	if (IIMWimIsMws())
		return(IIMWguiGetUsrInp(prompt, echo,
			frscb->frs_event, result));
# endif	/* DATAVIEW */
	if ((stat =IIFTpumPopUpMessage(prompt, TRUE, begy, begx,
		maxy, maxx, frscb, FALSE)) == OK)
	{
		if ((IIputilwin = TDsubwin(IIpopwin, 1, IIpopwin->_maxx - 3,
			IIpopwin->_begy + IIpopwin->_maxy - 2,
			IIpopwin->_begx + 2, IIputilwin)) == NULL)
		{
			stat = FAIL;
		}
		else
		{
			/*
			**  Set input area to reverse video.
			*/
			TDclear(IIputilwin);
			TDtouchwin(IIputilwin);
			TDrefresh(IIputilwin);
			IIputilwin->_flags |= _BOXFIX;

			IIFTsplSetPromptLoc(IIpopwin);
			tp = (char *)FTuserinp(IIputilwin, ERx(""), FT_PROMPTIN,
				echo, frscb->frs_event);
			STcopy(tp, result);

			TDerase(IIputilwin);
			IIputilwin->_flags &= ~_BOXFIX;
		}
	}


	/*
	**  Put back information that was saved in call to IIFTpumPopUpMessage.
	*/
	TDoverwrite(IIsavewin, IIpopwin);
	TDtouchwin(IIpopwin);
	TDrefresh(IIpopwin);		/* Clear bottom of the screen */
	TDerase(IIpopwin);		/* Clear window */

	return(stat);
}
