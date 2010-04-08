/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
**	INSERT.c  -  The INSERT Mode of the Frame Driver
**
**	This routine constitues the INSERT mode of the frame driver.
**	Those commands that are legal for the INSERT mode can be
**	processed here.	 The routine putfrm() call insert() to drive
**	the frame in this mode.	 Basically, this routine allows the
**	user to type over data in the fields unless the character
**	is a control character.	 If so, insert() checks to see if
**	it is a command and executes the command.  If it is not a
**	command and it is a control character, insert() will beep
**	an error.
**
**	Arguments:  frm	 - frame to drive
**		    mode - mode in which data is being accepted.
**
**	Returns:    0	    - if control-Z (end of frame) is depressed
**		    fdopESC - if the ESCAPE character is depressed
**				(call menu)
**		    default - if the user depresses an application
**				defined key that is not defined in
**				INSERT mode.
**
**	History:  JEN - 1 Nov 1981 (written)
**		  NCG - 15 Sep 1983 Added old code option.
**		  NCG - 16 May 1984 Added  query only fields.
**		  DKH - 3 May 1985 Added lower/upper case attribute.
**		  DKH - 7 May 1985 Added (NO)Auto-tab and NO-ECHO option.
**			Also cleaned things up.
**		  DKH - 19 Aug 1985 Added support for sideway scrolling in
**			a field.
**		  BAB - 15 July 1986 Fix for bug 9505.	Don't clear to bottom
**			of field when pressing RETURN if cursor in last
**			position of field, and auto tab is off.	 Also, when
**			autotab is off, don't beep until a character is
**			typed AFTER the field is already full.
**	09/19/86 (KY)  -- Changed CH.h to CM.h.
**	bruceb - 9/23/86 Fix for bug 10411.  Prevent no-echo fields
**			from auto-tabbing one char too soon.
**	10/02/86 (KY)  -- Added temporary window 'WINDOW *tempwin'
**			    for using to shift the window data right.
**	10/10/86 (KY)  -- Changed handling for Overstrike-datainput mode.
**			    Then Double Byte characters are able to
**			    treate as same as single byte characters.
**	DKH - 12/24/86 - Added support for new activations.
**	03/04/87 (dkh) - Added support for ADTs.
**	27-apr-87 (bruceb)
**		Added code for right-to-left fields for Hebrew project.
**		Involves placement of cursor at right side of field,
**		movement in an RL fashion, deletion of chars to the
**		right of the cursor, insertion to the right of the cursor
**		and checking current loc against proper end of the window.
**		FTfldattr accepts additional parameter that indicates
**		whether field is RL or not.
**		NOTE:	Flow of control for modification of field internal
**			and display buffers (particularly for RL fields)
**			is as follows:	(The display buffer contains the same
**			information as does the window when the form is on
**			the screen--hence contains a RL depiction of field's
**			contents.  The internal buffer contains a standard LR
**			version of the field.) Moving out of a field WITH
**			validation checking turned ON involves calling FTfld_str
**			to retrieve the window's contents, and FTvalidate.
**			FTvalidate calls fmt_cvt, which modifies the value in
**			the display buffer for storage in the internal buffer;
**			this includes reversing the order of the field's text.
**			FTvalidate then calls FDdatafmt which in turn calls
**			either fmt_format (then f_string) or fmt_multi
**			depending on whether the field is one row or more.
**			Those routines (f_string and fmt_multi) in turn take
**			the internal buffer (which is LR) and reverse it for
**			storage (and later display via FTstr_fld) in RL fashion.
**			If NO validation is occurring for this field, then no
**			modification of the buffers occurs.
**
**			(GETFORM's call fmt_cvt to modify the internal buffer,
**			reversing on need, prior to returning that buffer.
**			PUTFORM's call f_string or fmt_multi to modify the
**			display buffer, reversing on need, prior to displaying
**			that buffer.)
**	05/02/87 (dkh) - Changed to call FD routines via passed pointers.
**	05/02/87 (dkh) - Integrated support for change bits.
**	05/12/87 (dkh) - Fixed problem with tempwin not being updated
**			 when user moves to a new field with new sizes.
**	06/19/87 (dkh) - Code cleanup.
**	02-jul-87 (marian) - Fixed bug 12408.  Changed FTvalidate() so it
**			 now calls FTfldchng() for fdRNGMD.  THis takes
**			 care of dumping the frame for testing purposes.
**	08/18/87 (dkh/bruceb) - ER changes/code cleanup.
**	10-sep-87 (bruceb)
**		Changed from use of ERR to FAIL (for DG.)
**	24-sep-87 (bruceb)
**		Added call on FTshell for the shell function key.
**	11/04/87 (dkh) - Changed screen recovery after error occurs.
**	11/28/87 (dkh) - Added screen recovery for range mode also.
**	01/23/88 (dkh) - Fixed so that typing into field will clear fdVALCHKED.
**	09-feb-88 (bruceb)
**		Look for fdSCRLFD in fhd2flags rather than fhdflags;
**		can't use most significant bit of fhdflags.
**	02/27/88 (dkh) - Added support for nextitem command.
**	05/05/88 (dkh) - Fixed 5.0 bug 14834.
**	05/05/88 (dkh) - Fixed 5.0 bug 13077.
**	05/11/88 (dkh) - Added hooks for floating popups.
**	05/20/88 (dkh) - Added support for inquiring on current position.
**	10/23/88 (dkh) - Performance changes.
**	11-nov-88 (bruceb)
**		Added support for timeout; return fdopTMOUT.
**	11/30/88 (dkh) - Fixed venus bug 4073.
**	15-feb-89 (bruceb)
**		Propagated fix to 5.0 bug 16153.  On autotab'ng out of
**		of a field (or table field column) set evcb->event to
**		fdopRET rather than fdopNEXT.  Needed for an activate
**		with a resume next.
**	22-mar-89 (bruceb)
**		Prefer to return fdopNXITEM instead of fdopRET for the
**		previous bug fix.  (On autotab.)
**	04/07/89 (dkh) - Fixed 6.2 bug 4945.
**	20-apr-89 (bruceb)
**		Changes to support new clearrest semantics on mandatory
**		fields.
**	05/16/89 (dkh) - Fixed 6.2 bug 6030.
**	03-jul-89 (bruceb)
**		FTbotrst() no longer touches/refreshes FTwin.  Not needed
**		now that FTboxerr() restores other areas on the screen.
**	11-jul-89 (bruceb)
**		FDvalidate now takes extra
**		parameter that indicates if some error occurred on a field
**		other than that being validated--derived fields--so that
**		FTbotrst() will still be called.
**	07/12/89 (dkh) - Added support for emerald internal hooks.
**	02-aug-89 (bruceb)
**		Modified for aggregate derivations.  Involved changes to
**		FTfldattr(), which among other things now returns a FLDHDR
**		pointer.
**	09/02/89 (dkh) - Put in support for per display loop keystroke act.
**	09/14/89 (dkh) - Fixed bug 7607.
**	09/15/89 (dkh) - Fixed bug 7944.
**	09/28/89 (dkh) - Added on_menu parameter on call to intercept routine.
**	10/19/89 (dkh) - Changed code to eliminate duplicate FT files
**			 in GT directory.
**	11/16/89 (dkh) - Fixed problem with not updating internal buffers
**			 with screen buffers before exiting due to keystroke
**			 activation.
**	11/24/89 (dkh) - Added support for goto's from wview.
**	12/27/89 (dkh) - Added support for hot spot trim.
**	01-feb-90 (bruceb)
**		Moved TEflush's into FTbell.
**	14-mar-90 (bruceb)	Fix for bug 20581.
**		Validate current tblfld cell on fdopUP/DOWN before activation.
**	16-mar-90 (bruceb)
**		Added support for locator events.  FKmapkey() is now called
**		with a function pointer used to determine what affect a locator
**		event should have.
**	04/04/90 (dkh) - Integrated MACWS changes.
**	12-apr-90 (bruceb)	Fix for bug 21122.
**		Fixed problems with the noecho code.  However, THIS
**		CODE STILL HAS LOTS OF PROBLEMS WITH EDITING IN SUCH FIELDS
**		WITH KANJI CHARS!!!
**		Also, removed fdopSKPFLD code as it's basically obsolete.
**	16-apr-90 (bruceb)
**		Changes for different semantics of clearrest on mandatory
**		fields.  Clear the field, give error message and restore the
**		contents.
**	04/20/90 (dkh) - Allow user to move off fakerow at end of tf (us #196).
**	02-may-90 (bruceb)
**		Clearrest semantics shouldn't be 'special' for mandatory
**		fields when in query mode.
**	07/10/90 (dkh) - Integrated more MACWS changes.
**	08/15/90 (dkh) - Fixed bug 21670.
**	28-aug-90 (bruceb)	Fix for bug 32836.
**		Use of fdopADUP should set change bit.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**	10/14/90 (dkh) - Integrated round 4 of macws changes.
**	19-dec-91 (leighb) DeskTop Porting Change:
**		Changes to allow menu line to be at either top or bottom.
**	06/05/92 (dkh) - Added support for edit masks.
**	15-nov-93 (rudyw)
**		Moved a section of code in the page scrolling case section of
**		FTinsert ahead of the conditional in which it was placed. Code
**		placement now corresponds to fdopNEXT/fdopDOWN sections and
**		results in changed field values being retained on scroll.47663
**	18-mar-94 (rudyw)
**		Fix 55636. Up/Down arrow out of a simple scrollable field was
**		getting confused under some arrowing sequences after reentry
**		causing characters to be misplaced on the screen. Must reset
**		the scroll field when exiting with arrows in preparation for
**		reentering the field at the beginning char.
**	22-feb-95 (forky01)
**		Fix 66951. Last char ignored in numeric template field if
**		previous non-numeric template field left through autotab.
**		lastch not being cleared on new field entry.  Also, autotab
**		is not compatible with numeric template fields, so add extra
**		conditionals to check fmttype != F_NT for autotab code.
**	17-nov-95 (kch)
**		In the function FTinsert(), if an error occurs due to a null
**		value being entered into a mandatory field, we set
**		'field already validated' (fdVALCHKED) on in the change-bit
**		for the field (chgbit) after the field is refreshed with the
**		previous valid value. This prevents the reformatting of the
**		already formatted value. This change fixes bug 69967.
**      23-sep-96 (mcgem01)
**              Global data moved to ftdata.c
**	06-Aug-1997 (rodjo04) bug 82140.
**		When pressing return when on an input masked field, 
**		we should pass to the next field. The user should not 
**		have to 'tab' to pass out of input masked fields.
**		Fixed inputing and editing of numeric fields.
**	18-Aug-1997 (bobmart)
**		Amend above change.
**      13-Nov-97 (fanra01)
**              Reapplied program behaviour of no action when pressing
**              carriage return within an input masked field.
**	27-nov-1997 (kitch01)
**		Bug 87260. Amend FTinsert to allow upfront to scroll a tablefield in 
**		insert mode.
**	19-dec-97 (kitch01)
**		Bug 87110. If the form contains more than 1 tablefiled then a scroll
**		action from Upfront via MWS can scroll the wrong tablefield. This is 
**		because IIcurtbl has not been updated to reflect the tablefield with
**		focus before the call to IItscroll. I amended the code to go back to 
**		IItputfrm so that this action can be handled there.
**	23-dec-97 (kitch01)
**		Bugs 87709, 87808, 87812, 87841.
**		All related to scrolling tablfields in Upfront. I have redesigned the
**		code so that a refreshscrollbar message is sent to Upfront providing
**		this is the first time the form has been displayed. This ensures that
**		all tablefields have the scrollbar correctly positioned initially. Any
**		subsequent scroll of the tablefield will have the message sent by the 
**		scrolling code.
**  24-Dec-1997 (rodjo04)
**      Bug 87872 Fixed input masking for +'---zzz'. If 'n' is not used in
**      the input mask a user will not be able to enter a negative number.
**      Since the buffer bufr will not be padded with zeros (a digit),
**      bufr will be set to EOS destroying the negitive sign.
**      Now we check for '-'.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-dec-2001 (somsa01)
**	    Added LEVEL1_OPTIM to i64_win to prevent ISQL from SEGV'ing
**	    when a line of query test is entered followed by a carriage
**	    return.
**	20-Aug-2003 (bonro01)
**	    Added NO_OPTIM for i64_hpu to eliminate illegal instruction
**	    abend in the 3gl COBOL tests.
**	26-Aug-2009 (kschendel) b121804
**	    Include termdr.h for gcc 4.3.
*/
/*
NO_OPTIM=i64_hpu
*/


# include	<compat.h>
# include	<st.h>		 
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	"ftfuncs.h"
# include	"ftrange.h"
# include	<frscnsts.h>
# include	<frsctblk.h>
# include	<cm.h>
# include	<te.h>
# include	<er.h>
# include	<erft.h>
# include	<scroll.h>
# include	<afe.h>
# include	<termdr.h>

# if defined(i64_win)
# pragma optimize("s", on)
# endif

# define	XLG_STRING	4096
# define	TRUUEST		2	/* More true even then TRUE */

GLOBALREF	bool	ftvalnext;	/* validate when going to next field */
GLOBALREF	bool	ftvalprev;	/* validate when going to prev field */
GLOBALREF	bool	ftvalmenu;	/* validate if menu key pressed */
GLOBALREF	bool	ftvalfrskey;	/* validate when a frskey is pressed */
GLOBALREF	bool	ftactnext;	/* activate going to next field */
GLOBALREF	bool	ftactprev;	/* activate going to prev field */
GLOBALREF 	i4	IITDAllocSize;			 

FUNC_EXTERN	i4	TDrefresh();
FUNC_EXTERN	bool	FTgotmenu();
FUNC_EXTERN	bool	FTgotfrsk();
FUNC_EXTERN	RGRFLD	*FTgetrg();
FUNC_EXTERN	WINDOW	*FTgetwin();
FUNC_EXTERN	WINDOW	*TDtmpwin();
FUNC_EXTERN	bool	FTrngcval();
FUNC_EXTERN	VOID	FTrdelchar();
FUNC_EXTERN	VOID	FTrinschar();
FUNC_EXTERN	VOID	FTraddchar();
FUNC_EXTERN	VOID	FTprnscr();
FUNC_EXTERN	VOID	FTword();
FUNC_EXTERN	VOID	IIFTsfrScrollFldReset();
FUNC_EXTERN	VOID	IIFTsfeScrollFldEnd();
FUNC_EXTERN	VOID	IIFTcesClrEndScrollFld();
FUNC_EXTERN	VOID	IIFTcsfClrScrollFld();
FUNC_EXTERN	VOID	IIFTdcsDelCharScroll();
FUNC_EXTERN	VOID	IIFTmdsMvDelcharScroll();
FUNC_EXTERN	VOID	IIFTrbfRuboutScroll();
FUNC_EXTERN	VOID	IIFTmlsMvLeftScroll();
FUNC_EXTERN	VOID	IIFTmrsMvRightScroll();
FUNC_EXTERN	i4	IIFTsicScrollInsertChar();
FUNC_EXTERN	i4	IIFTsacScrollAddChar();
FUNC_EXTERN	VOID	IIFTsiiSetItemInfo();
FUNC_EXTERN	bool	IIFTsfsScrollFldStart();
FUNC_EXTERN	bool	IIFTtoTimeOut();
FUNC_EXTERN	VOID	IIFTfldFrmLocDecode();
FUNC_EXTERN	VOID	IIFTsmShiftMenu();
FUNC_EXTERN	VOID	IIFTefsEmptyFldStr();
FUNC_EXTERN	VOID	IIFTrfsRestoreFldStr();
FUNC_EXTERN	i4	fmt_ntposition();
FUNC_EXTERN	i4	fmt_stposition();
GLOBALREF	i4	(*IIFTldrLastDsRow)();
static		bool	FTfldstart();

GLOBALREF	i4	FTfldno;
GLOBALREF	RGRFLD	*rgfptr;

static	i4		FTimod = 0;	/* mode: insert or overstrike */
static	SCROLL_DATA	*scroll_fld = NULL;
static	char		FTbottom[MAX_TERM_SIZE + 1];
static	u_char		FTbotattr;							 
static	bool		inrngmd = FALSE;
static	i4		form_mode = 0;
static	char		dec_sym[3] = {0};

VOID
FTbotsav()
{
	FTbotattr = IITDwin_attr(stdmsg);					 
	TDwin_str(stdmsg, FTbottom, (bool) FALSE);
}

VOID
FTbotrst()
{
	TDclear(stdmsg);
	TDhilite(stdmsg, 0, IITDAllocSize, FTbotattr);				 
	TDaddstr(stdmsg, FTbottom);
	TDrefresh(stdmsg);
}


IIFTEmaskSetPos(afld, win, lfmt, fmttype, bufr, nbuf)
FIELD	*afld;
WINDOW	*win;
FMT	*lfmt;
i4	fmttype;
char	*bufr;
char	*nbuf;
{
	FLDVAL	*nval;
	i4	pos = -1;
	bool	isnullable;
	bool	dummy;

	if (fmttype == F_ST || fmttype == F_DT)
	{
		TDmove(win, 0, fmt_stposition(lfmt, 0, 1));
	}
	else
	{
		bool	isnullable;
		bool	dummy;
		i4	pos = -1;

		nval = (*FTgetval)(afld);
		isnullable = AFE_NULLABLE(nval->fvdbv->db_datatype);
		TDwin_str(win, bufr, (bool) FALSE);
		/*
		**  Entering the field, position cursor
		**  on the decimal point if there is one.
		**  fmt_ntmodify does all of the work.
		*/
		fmt_ntmodify(lfmt, isnullable, EOS, bufr, &pos,
			nbuf, &dummy);

		TDmove(win, 0, pos);
	}
}



VOID
FTtxtmv(bufr, val)
char	*bufr;
FLDVAL	*val;
{
	DB_TEXT_STRING	*text;
	i4		count = 0;
	u_char		*cp;

	text = (DB_TEXT_STRING *) val->fvdsdbv->db_data;
	for (cp = val->fvbufr; *bufr; )
	{
		*cp++ = *bufr++;
		count++;
	}
	text->db_t_count = count;
}


VOID
FTfldattr(frm, fldno, lcase, ucase, autotab, noecho, bufr, bufrend,
	chgbit, win, reverse, derivesrc, agginval, rethdr, rowinfo, newfmt,
	edit_mask, fmttype)
FRAME	*frm;
i4	fldno;
bool	*lcase;
bool	*ucase;
bool	*autotab;
bool	*noecho;
char	*bufr;
char	**bufrend;
i4	**chgbit;
WINDOW	*win;
bool	*reverse;
bool	*derivesrc;
bool	*agginval;
FLDHDR	**rethdr;
i4	**rowinfo;
FMT	**newfmt;
i4	*edit_mask;
i4	*fmttype;
{
	DB_TEXT_STRING	*text;
	FIELD		*fld;
	FLDHDR		*hdr;
	FLDVAL		*val;
	TBLFLD		*tf = NULL;
	FLDTYPE		*type;
	bool		nottfqrymode = TRUE;

	fld = frm->frfld[FTfldno];
	if (fld->fltag == FTABLE)
	{
		tf = fld->fld_var.fltblfld;
		if (tf->tfhdr.fhdflags & fdtfQUERY)
		{
			nottfqrymode = FALSE;
		}
	}

	/*
	**  Note that there is no need to check whether the field
	**  is a table field or not since FTgethdr does the right
	**  thing. (dkh)
	*/

	hdr = (*FTgethdr)(fld);
	*rethdr = hdr;

	type = (*FTgettype)(fld);

	/*
	**  Check for force lowercase attribute.
	*/

	if (hdr->fhdflags & fdLOWCASE)
	{
		*lcase = TRUE;
	}
	else
	{
		*lcase = FALSE;
	}

	/*
	**  Check for force uppercase attribute.
	*/

	if (hdr->fhdflags & fdUPCASE)
	{
		*ucase = TRUE;
	}
	else
	{
		*ucase = FALSE;
	}

	/*
	**  Check for NO-AUTOTAB attribute.
	*/

	if (hdr->fhdflags & fdNOAUTOTAB)
	{
		*autotab = FALSE;
	}
	else
	{
		*autotab = TRUE;
	}

	/*
	**  Check for NO-ECHO attribute.
	*/

	if (hdr->fhdflags & fdNOECHO)
	{
		*noecho = TRUE;
		val = (*FTgetval)(fld);
		text = (DB_TEXT_STRING *) val->fvdsdbv->db_data;
		STlcopy(val->fvbufr, bufr, text->db_t_count);
		*bufrend = bufr + STlength(bufr);
	}
	else
	{
		*noecho = FALSE;
	}

	if (frm->frmflags & fdRNGMD)
	{
		rgfptr = FTgetrg(frm, fldno);
	}
	else
	{
		rgfptr = NULL;
	}

	tempwin = TDtmpwin(tempwin, win->_maxy, win->_maxx);

	/*
	**  Set up pointer to flags space for field so
	**  we can update the change bits, etc.
	*/
	if (tf)
	{
		*chgbit = tf->tffflags + (tf->tfcurrow * tf->tfcols) +
			tf->tfcurcol;
		*rowinfo = tf->tffflags + (tf->tfcurrow * tf->tfcols);
	}
	else
	{
		*chgbit = &(hdr->fhdflags);
		*rowinfo = NULL;
	}

	/*
	**  Check for field being right-to-left
	*/

	if (hdr->fhdflags & fdREVDIR)
	{
		*reverse = TRUE;
	}
	else
	{
		*reverse = FALSE;
	}

	if (hdr->fhd2flags & fdSCRLFD)
	{
		scroll_fld = IIFTgssGetScrollStruct(fld);
	}
	else
	{
		scroll_fld = NULL;
	}

	/*
	** Only a derivesrc (for purposes of invalidating aggregate
	** information) if a table field which contains any aggregate
	** source columns. Further, form can't be in query mode.  No
	** guarantee even then that this is an aggregate source column
	** since this could be the parent of another column which is an
	** aggregate source column....  Not checking that this column is
	** even a source column since many actions require invalidating
	** all aggregates in the table field.
	*/
	if (tf && (tf->tfhdr.fhd2flags & fdAGGSRC)
		&& frm->frres2->formmode != fdrtQUERY)
	{
		*derivesrc = TRUE;
	}
	else
	{
		*derivesrc = FALSE;
	}
	*agginval = FALSE; /* Certainly haven't invalidated anything yet. */

	/*
	**  Find FMT structure and set it.
	*/
	*newfmt = type->ftfmt;


	/*
	**  Check to see if new field can have edit mask processing.
	*/
	if ((hdr->fhd2flags & fdUSEEMASK) && !inrngmd &&
		form_mode != fdmdQRY && nottfqrymode)
	{
		*edit_mask = TRUE;
		*fmttype = (*newfmt)->fmt_type;
	}
	else
	{
		*edit_mask = FALSE;
		*fmttype = -1;
	}

# ifdef DATAVIEW
	/*  We do not want any scroll or no-echo management */
	if (IIMWimIsMws())
	{
		scroll_fld = NULL;
		*noecho = FALSE;
	}
# endif	/* DATAVIEW */
}


bool
FTvalidate(frm, fldno, bufr, tbl, mode, win, fld_changed, chgbit)
FRAME	*frm;
i4	fldno;
char	*bufr;
TBLFLD	*tbl;
i4	mode;
WINDOW	*win;
bool	*fld_changed;
i4	*chgbit;
{
	i4	fldtype;
	i4	curcol;
	i4	currow;
	i4	retval = TRUE;
	FLDHDR	*hdr;
	i4	newx = 0;
	bool	reverse;
	bool	val_error;

	if (frm->frmflags & fdRNGMD)
	{
		/*
		**  Fix for BUG 12408. (marian)
		*/
		FTfldchng(frm, fld_changed);

		/*
		**  Fix for BUG 7561. (dkh)
		*/
		TDrefresh(win);

		FTbotsav();
		if (!FTrngcval(frm, fldno, tbl, win))
		{
			FTbotrst();
			retval = FALSE;
			hdr = (*FTgethdr)(frm->frfld[fldno]);
			if (hdr->fhdflags & fdREVDIR)
				newx = win->_maxx - 1;
			TDmove(win, (i4) 0, newx);
			TDrefresh(win);

# ifdef DATAVIEW
			_VOID_ IIMWrfRstFld(frm, fldno, tbl);
# endif	/* DATAVIEW */

		}
		return((bool)retval);
	}

	if ((tbl == NULL && (mode == fdmdADD || mode == fdmdUPD)) ||
		(tbl != NULL && !(tbl->tfhdr.fhdflags & fdtfQUERY)))
	{
		FTfldchng(frm, fld_changed);

		if (tbl == NULL)
		{
			fldtype = FT_REGFLD;
			curcol = 0;
			currow = 0;
		}
		else
		{
			fldtype = FT_TBLFLD;
			curcol = tbl->tfcurcol;
			currow = tbl->tfcurrow;
		}

		/*
		**  Fix for BUG 7561. (dkh)
		*/
		TDrefresh(win);

		hdr = (*FTgethdr)(frm->frfld[FTfldno]);
		FTbotsav();
		retval = (*FTvalfld)(frm, FTfldno, fldtype, curcol, currow,
			&val_error);

		/*
		** Derivation source field, so ensure derived values are
		** displayed.  Do this before moving the cursor, else cursor
		** can end up in the wrong field on the screen.
		*/
		if (hdr->fhdrv)
		{
			TDrefresh(FTwin);
		}

		if (retval == FALSE)
		{
			FTbotrst();
			reverse = (hdr->fhdflags & fdREVDIR) ?
				(bool)TRUE : (bool)FALSE;
			if (scroll_fld)
			{
				IIFTsfrScrollFldReset(win, scroll_fld, reverse);
			}
			else
			{
				if (reverse)
					newx = win->_maxx - 1;
				TDmove(win, (i4) 0, newx);
			}
			TDrefresh(win);

# ifdef DATAVIEW
			_VOID_ IIMWrfRstFld(frm, fldno, tbl);
# endif	/* DATAVIEW */

		}
		else
		{
			*chgbit |= fdVALCHKED;
		}
	}
	return((bool)retval);
}


/*{
** Name:	IIFTsciSetCursorInfo - Set Screen Cursor Information
**
** Description:
**	Set screen cursor position as well as current screen beginning
**	position in the passed in event control block.  This is needed
**	to support inquiring on cursor position and starting screen
**	position of the current field.
**
** Inputs:
**	evcb	Event control block.
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
**	05/20/88 (dkh) - Initial version.
*/
VOID
IIFTsciSetCursorInfo(evcb)
FRS_EVCB	*evcb;
{
	evcb->scrxoffset = curscr->_begx;
	evcb->scryoffset = curscr->_begy;
	evcb->xcursor = curscr->_curx;
	evcb->ycursor = curscr->_cury;
}


i4
FTinsert(frm, mode, frscb)		/* FTINSERT: */
FRAME	*frm;				/* frame to be displayed */
i4	mode;
FRS_CB	*frscb;
{
	WINDOW	*win;			/* pointer to data window */
	u_char	ch = 0; /* char read from terminal */
	KEYSTRUCT	*inp;		/* char read from terminal */
	i4	x,y;			/* hold position of field */
	i4	lastch;			/* pos of last char read in */
	i4	code = fdNOCODE;	/* code assoc. with char(s) */
	i4	flg;			/* flg assoc. with char(s) */
	i4	mvcode;
	i4	noechox;
	i4	noechoy;
	FIELD	*afld;
	FLDHDR	*hdr;
	TBLFLD	*tbl = NULL;
	FLDVAL	*nval;
	FLDTYPE	*ntype;
	char	*noechoptr;
	char	*bufrend;
	char	bufr[XLG_STRING + 1];
	char	nbuf[XLG_STRING + 1];
	bool	fld_changed;
	bool	lcase;
	bool	ucase;
	bool	auto_tab;
	bool	noecho = FALSE;
	bool	first_bell = TRUE;
	bool	validate = FALSE;
	i4	typed_last = FALSE;
	FRS_EVCB *evcb;
	FRS_AVCB *avcb;
	i4	*chgbit;
	i4	*rowinfo;
	bool	reverse;
	i4	newx;
	bool	nospace;
	bool	need_ref;
	bool	was_valid;
	bool	clr_mand;
	i4	(*kyint)();
	i4	keytype;
	i4	keyaction;
	bool	val_error;
	bool	derivesrc;
	bool	agginval;
	STATUS	retval;
	i4	retval2;
	i4	event;
	FMT	*lfmt;
	i4	have_edited_char;
	u_char	ebuf[10];
	i4	edit_mask;
	i4	fmttype;
	bool	on_newfld = FALSE;
	bool	isnullable;
	bool	dummy;
	i4	curs_pos;
# ifdef DATAVIEW
	bool			do_here = TRUE;
# endif	/* DATAVIEW */

	lastch = OK;

	/*
	**  Save the form mode for use by FTfldattr().
	*/
	form_mode = mode;

	if (dec_sym[0] == '\0')
	{
		dec_sym[0] = FEadfcb()->adf_decimal.db_decimal;
	}

	FTfldno = frm->frcurfld;
	if ((win = FTgetwin(frm, &FTfldno)) == NULL)
	{
		return (fdopMENU);
	}

	IIFTsiiSetItemInfo(win->_begx + FTwin->_startx + 1,
		win->_begy + FTwin->_starty + 1, win->_maxx, win->_maxy);

	if (frm->frmflags & fdRNGMD)
	{
		inrngmd = TRUE;
		FTrgtag(frm->frrngtag);
	}
	else
	{
		inrngmd = FALSE;
	}


	fld_changed = FALSE;

	FTfldattr(frm, FTfldno, &lcase, &ucase, &auto_tab,
		&noecho, bufr, &bufrend, &chgbit, win, &reverse,
		&derivesrc, &agginval, &hdr, &rowinfo, &lfmt, &edit_mask,
		&fmttype);

	if (reverse)
		newx = win->_maxx - 1;
	else
		newx = 0;
	TDmove(win, (i4)0, newx);

	afld = frm->frfld[FTfldno];

	/*
	**  If edit_mask processing is possible for field, then
	**  move to first non-constant character position in
	**  field.
	*/
	if (edit_mask)
	{
		IIFTEmaskSetPos(afld, win, lfmt, fmttype, bufr, nbuf);
# ifdef OLD
		if (fmttype == F_ST || fmttype == F_DT)
		{
			TDmove(win, 0, fmt_stposition(lfmt, 0, 1));
		}
		else
		{
			bool	isnullable;
			bool	dummy;
			i4	pos = -1;

			nval = (*FTgetval)(afld);
			isnullable = AFE_NULLABLE(nval->fvdbv->db_datatype);
			TDwin_str(win, bufr, (bool) FALSE);
			/*
			**  Entering the field, position cursor
			**  on the decimal point if there is one.
			**  fmt_ntmodify does all of the work.
			*/
			fmt_ntmodify(lfmt, isnullable, EOS, bufr, &pos,
				nbuf, &dummy);

			TDmove(win, 0, pos);
		}
# endif
	}

	TDrefresh(win);
	(*FTdmpmsg)(ERx("INSERT loop:\n"));
	(*FTdmpcur)();

	noechoptr = bufr;
	noechox = noechoy = 0;

	evcb = frscb->frs_event;
	avcb = frscb->frs_actval;
	if (frscb->frs_globs->intr_frskey0)
	{
		kyint = frscb->frs_globs->key_intcp;
	}
	else
	{
		kyint = NULL;
	}

	for(;;)				/* for ever */
	{

# ifdef DATAVIEW
		if (IIMWimIsMws() && do_here)
		{

			/* Bugs 87709, 87808, 87812, 87841. We now only send the
			** refreshscrollbar message if this is the first time the
			** form has been displayed.
			*/
			if (IIMWisrInitScrollbarReqd(frm))
			{
				IItscroll_init();
				IIMWsiScrollbarInitialized(frm);
			}

			if ((IIMWscfSetCurFld(frm, FTfldno) == FAIL) ||
				(IIMWscmSetCurMode(FTcurdisp(), fdcmINSRT,
					kyint != NULL, FALSE,
					FALSE) == FAIL))
			{
				return(fdNOCODE);
			}
		}
# endif	/* DATAVIEW */

		afld = frm->frfld[FTfldno];
		need_ref = FALSE;
		was_valid = FALSE;
		clr_mand = FALSE;

  		if (edit_mask && on_newfld)
		{
			IIFTEmaskSetPos(afld, win, lfmt, fmttype, bufr, nbuf);
			on_newfld = FALSE;
			lastch = OK;
			TDrefresh(win);
		}

		tbl = NULL;
		if (afld->fltag == FTABLE)
		{
			tbl = afld->fld_var.fltblfld;
		}

		IIFTsciSetCursorInfo(evcb);
		inp = FTTDgetch(win);

# ifdef DATAVIEW
		if (IIMWimIsMws())
		{
			/*
			**  If there is an error while getting the
			**  user response, ks would be returned as
			**  NULL.  If this happens, we just return
			**  to higher level, so that we don't end up
			**  referencing through a NULL ptr.
			*/
			if (inp == (KEYSTRUCT *) NULL)
				return(fdNOCODE);
			IIMWfcFldChanged(chgbit);
			if (*chgbit & fdI_CHG)
			{
				if (rowinfo != NULL)
					*rowinfo |= fdtfRWCHG;
				if (derivesrc && !agginval)
				{
					(*IIFTiaInvalidateAgg)(hdr, fdIASET);
					agginval = TRUE;
				}
			}
		}
# endif	/* DATAVIEW */

		if (IIFTtoTimeOut(evcb))
		{
			/* Save away contents of current field */
			frm->frcurfld = FTfldno;
			nval = (*FTgetval)(afld);

			if (noecho)
				FTtxtmv(bufr, nval);
			if (*chgbit & fdI_CHG)
				FTfld_str(hdr, nval, win);
			if (scroll_fld)
			{
				IIFTsfrScrollFldReset(win, scroll_fld, reverse);
				TDrefresh(win);
			}
			return(fdopTMOUT);
		}

		evcb->event = fdNOCODE;

		retval = FKmapkey(inp, IIFTfldFrmLocDecode, &code, &flg,
			&validate, &ch, frscb->frs_event);

		/*
		**  If keystroke intercept is active, return keystroke
		**  info.
		*/
		if (kyint)
		{
			event = evcb->event;
			if (inp->ks_fk != 0)
			{
				if (event == fdopHSPOT)
				{
					keytype = evcb->mf_num;
				}
				else
				{
					keytype = FUNCTION_KEY;
					if (code == fdopMENU)
					{
					    /*
					    ** Special casing for locators.
					    ** Not changing evcb->event, just
					    ** that value using by (*kyint)().
					    */
					    event = fdopMENU;
					}
				}
			}
			else if (CMcntrl(inp->ks_ch))
			{
				keytype = CONTROL_KEY;
			}
			else
			{
				keytype = NORM_KEY;
			}
			keyaction = (*kyint)(event, keytype, inp->ks_ch,
				(bool) FALSE);
			if (keyaction == FRS_IGNORE)
			{
# ifdef DATAVIEW
				if (IIMWdlkDoLastKey(TRUE, &do_here) == FAIL)
					return(fdNOCODE);
# endif	/* DATAVIEW */
				continue;
			}
			else if (keyaction == FRS_RETURN)
			{
				evcb->event = fdopFRSK;
				evcb->val_event = FRS_NO;
				evcb->act_event = FRS_NO;
				evcb->intr_val = frscb->frs_globs->intr_frskey0;

				/*
				**  Before returning, synchronize buffers.
				*/
				frm->frcurfld = FTfldno;

				nval = (*FTgetval)(frm->frfld[FTfldno]);

				if (noecho)
				{
					FTtxtmv(bufr, nval);
				}

				if (*chgbit & fdI_CHG)
				{
					FTfld_str(hdr, nval, win);
				}

				if (scroll_fld)
				{
					IIFTsfrScrollFldReset(win,
						scroll_fld, reverse);
					TDrefresh(win);
				}
				return(evcb->intr_val);
			}

			/*
			**  Drop through to process the character.
			*/
		}
		
		if (retval != OK)
		{
			FTbell();
			continue;
		}

# ifdef DATAVIEW
		if (IIMWdlkDoLastKey(FALSE, &do_here) == FAIL)
			return(fdNOCODE);
		if ( ! do_here)
			continue;
# endif	/* DATAVIEW */

		if(!(flg & fdcmINSRT))	 /* if it isn't accepted by INSERT */
		{
			code = fdopERR;	 /* then bad code, add as is */
			flg = fdcmNULL;
		}


		switch(code)
		{
		  case fdopNULL:
		  case fdopHSPOT:
			break;

		  case fdopRET:	 /* if carriage return, clear to end
				 ** of field, move to next (fall thru)
				 */

			/*
			**  For edit masks, we don't support clearing
			**  to the end of the field at present.  The
			**  concept is not really compatible with edit masks.
			**  We just beep for now.
			*/
			if (edit_mask)
			{
                            FTbell();
                            break;
			}

			frm->frchange = TRUE;
			*chgbit |= (fdI_CHG | fdX_CHG);
			*chgbit &= ~fdVALCHKED;
			if (rowinfo)
			{
				*rowinfo |= fdtfRWCHG;
			}
			fld_changed = TRUE;
			if (noecho)
			{
				*noechoptr = '\0';
			}
			else
			{
				y = win->_cury;
				x = win->_curx;
				/*
				** fix for bug 9505.  Don't TDclrbot
				** for field with autotab off, if
				** just typed char in last position.  (bruceb)
				*/
				if ( (auto_tab && fmttype != F_NT)
					 || !typed_last ) 
				{
				    if ((hdr->fhdflags & fdMAND)
					&& (mode != fdmdQRY)
					&& !inrngmd && avcb->val_next
					&& FTfldstart(win, scroll_fld, reverse))
				    {
					clr_mand = TRUE;
					nval = (*FTgetval)(afld);
					FTfld_str(hdr, nval, win);
				    }
				    TDclrbot(win, reverse);
				    TDmove(win, y, x);
				    if (scroll_fld)
				    {
					IIFTcesClrEndScrollFld(scroll_fld,
					    reverse);
				    }
				}
				if (inrngmd)
				{
					rgfptr->tcur = NULL;
				}
			}

		  case fdopGOTO:
			/*
			**  A goto command from wview.  Follow
			**  needs to be done (regardless of
			**  whether we are going to the same
			**  field; wview could of course, optimize
			**  this boundary condition if possible).
			**  1) Check for next field validation.
			**  2) if (1) OK, check for exit activation.
			**  3) if no exit validation, then call
			**     FTmove() to move to destination.
			**  4) Check for any entry activation.
			**  Special consideration must be given
			**  to going to a tf cell that is not
			**  accessible/active and RANGE mode for QBF.
			**  TF cell access must take care to
			**  only goto a row that is valid.
			**
			**  If going to same location, then no-op.
			*/
			if (code == fdopGOTO)
			{
				if (FTfldno == evcb->gotofld)
				{
					if (!tbl || (tbl->tfcurcol ==
						evcb->gotocol &&
						tbl->tfcurrow == evcb->gotorow))
					{
						break;
					}
				}
				if (!(*IIFTgtcGoToChk)(frm, evcb))
				{
					/*
					**  If destination is NO good, send
					**  out a beep and ignore command.
					*/
					FTbell();
					break;
				}
			}
DO_TAB:
		  case fdopNXITEM:
		  case fdopNROW:
		  case fdopNEXT:   /* move to next field, go to top of
				   ** the frame if at last field
				   ** or next row if table field.
				   */
		  {
			nval = (*FTgetval)(afld);

			if (derivesrc && !agginval)
			{
				/*
				** Invalidate on a TAB, etc., because this
				** can instantiate a non-nullable numeric
				** column.
				*/
				(*IIFTiaaInvalidAllAggs)(frm, tbl);
				agginval = TRUE;
			}

			typed_last = FALSE;

			if (noecho)
			{
				/*
				**  Do a copy so we can call
				**  FTfld_str to do some other
				**  things for us. (dkh)
				*/

				FTtxtmv(bufr, nval);
			}

			if (*chgbit & fdI_CHG)
			{
				if (clr_mand)
				{
				    IIFTefsEmptyFldStr(nval);
				    frm->frmflags |= fdCLRMAND;
				}
				else
				{
				    FTfld_str(hdr, nval, win);
				    need_ref = TRUE;
				}
			}
			if (*chgbit & fdVALCHKED)
			{
				was_valid = TRUE;
			}

			if (avcb->val_next)
			{
				if (!FTvalidate(frm, FTfldno, bufr, tbl,
					mode, win, &fld_changed, chgbit))
				{
					if (noecho)
					{
						noechoptr = bufr;
						noechox = noechoy = 0;
					}
					if (clr_mand)
					{
					    ntype = (*FTgettype)(afld);
					    IIFTrfsRestoreFldStr(nval);
					    if (tbl)
						FTtblhdr(&(tbl->tfhdr));
					    FTstr_fld(hdr, ntype, nval, frm);
					    if (tbl)
						FTtblhdr((FLDHDR *)NULL);
					    TDmove(win, y, x);
					    TDtouchwin(win);
					    TDrefresh(win);
					    frm->frmflags &= ~fdCLRMAND;
					    /* Bug 69967 */
					    *chgbit |= fdVALCHKED;
					}
					agginval = FALSE;
					break;
				}
				else if (clr_mand)
				{
				    /* Empty fld evidently OK. */
				    frm->frmflags &= ~fdCLRMAND;
				}
				/*
				**  If the field had not been validated
				**  and is now valid, then we need to
				**  update screen with formatted string
				**  by setting need_ref to TRUE.
				*/
				if (!was_valid && *chgbit & fdVALCHKED)
				{
					need_ref = TRUE;
				}
			}
			else
			{
				FTfldchng(frm, &fld_changed);
			}

			if (inrngmd)
			{
				FTrngfldres(rgfptr, win, reverse);
				need_ref = TRUE;
			}
			else if (scroll_fld)
			{
				IIFTsfrScrollFldReset(win, scroll_fld, reverse);
				need_ref = TRUE;
			}

			if (need_ref)
			{
				TDtouchwin(win);
				TDrefresh(win);
			}

			IIFTsciSetCursorInfo(evcb);

			if (avcb->act_next && hdr->fhintrp != 0)
			{
				frm->frcurfld = FTfldno;
				if (tbl != NULL)
				{
					tbl->tfdelete = code;
				}
				return(hdr->fhintrp);
			}
			if ((mvcode = FTmove(code, frm, &FTfldno,
				&win, (i4)1, evcb)) != fdNOCODE)
			{
				return(mvcode);
			}
			FTfldattr(frm, FTfldno, &lcase, &ucase,
				&auto_tab, &noecho, bufr, &bufrend,
				&chgbit, win, &reverse,
				&derivesrc, &agginval, &hdr, &rowinfo, &lfmt,
				&edit_mask, &fmttype);
			noechoptr = bufr;
			noechox = noechoy = 0;
			on_newfld = TRUE;
# ifdef OLD
			if (edit_mask)
			{
				IIFTEmaskSetPos(afld, win, lfmt, fmttype,
					bufr, nbuf);
				if (fmttype == F_ST || fmttype == F_DT)
				{
					TDmove(win, 0, fmt_stposition(lfmt,
						0, 1));
				}
				else
				{
					bool	isnullable;
					bool	dummy;
					i4	pos = -1;

					nval = (*FTgetval)(afld);
					isnullable = AFE_NULLABLE(nval->fvdbv->db_datatype);
					TDwin_str(win, bufr, (bool) FALSE);

					fmt_ntmodify(lfmt, isnullable, EOS,
						bufr, &pos, nbuf, &dummy);
					TDmove(win, 0, pos);
				}
				TDrefresh(win);
			}
# endif
			break;
		  }

		  case fdopDELF: /* Delete character at present position */
			if (!noecho)
			{
				if (derivesrc && !agginval)
				{
					(*IIFTiaInvalidateAgg)(hdr, fdIASET);
					agginval = TRUE;
				}

				typed_last = FALSE;

				frm->frchange = TRUE;
				*chgbit |= (fdI_CHG | fdX_CHG);
				*chgbit &= ~fdVALCHKED;
				if (rowinfo)
				{
					*rowinfo |= fdtfRWCHG;
				}
				if (inrngmd)
				{
					FTrdelchar(win, rgfptr, reverse);
				}
				else if (scroll_fld)
				{
					IIFTdcsDelCharScroll(win, scroll_fld,
						reverse);
				}
				else if (edit_mask)
				{
					if (fmttype == F_NT)
					{
						char	*cur_ch;

						nval = (*FTgetval)(afld);
						isnullable = AFE_NULLABLE(nval->fvdbv->db_datatype);
						TDwin_str(win, bufr,
							(bool) FALSE);

						curs_pos = win->_curx;
						cur_ch = &(bufr[curs_pos]);
						if (CMdigit(cur_ch))
						{
							char	*decpt;
							char	ch_input = '@';
							i4	xyz;

							decpt = STindex(bufr,
								dec_sym, 0);
							if (decpt == NULL)
							{
								decpt = bufr + win->_maxx;
							}
							for (xyz = curs_pos;
								xyz > 0; xyz--)
							{
								bufr[xyz] = bufr[xyz - 1];
							}
							bufr[0] = ' ';
							if (&bufr[curs_pos] < decpt && curs_pos + 1 <= win->_maxx)
							{
								curs_pos++;
							}
							nval = (*FTgetval)(afld);
							isnullable = AFE_NULLABLE(nval->fvdbv->db_datatype);
							if (fmt_ntmodify(lfmt, isnullable, ch_input, bufr, &curs_pos, nbuf, &dummy) == FALSE)
							{
								FTbell();
								break;
							}
							TDmove(win, 0, 0);
							if  (*nbuf == EOS)
							{
								TDerschars(win);
                                TDmove(win, 0, (win->_maxx - 1)); 
                            }
							else
							{
								TDaddstr(win, nbuf);
								curs_pos = fmt_ntposition(nbuf,
										 	win->_maxx, curs_pos, 1);
								TDmove(win, 0, curs_pos);
							}
					        TDtouchwin(win);
						}
						else
						{
							if (++curs_pos < win->_maxx)
							{
								curs_pos = fmt_ntposition(bufr, win->_maxx, curs_pos, 1);
								TDmove(win, 0, curs_pos);
								TDtouchwin(win);
								TDrefresh(win);
							}
							break;
						}
					}
					else
					{
						i4	curs_pos;
						bool	warn;

						curs_pos = win->_curx;
						STcopy(ERx(" "), bufr);
						fmt_stvalid(lfmt, bufr, 1,
							curs_pos, &warn);
						TDaddchar(win, bufr, reverse);
						TDmove(win, 0, curs_pos);
					}
				}
				else
				{
					if (reverse)
						TDrdelch(win);
					else
						TDdelch(win);
				}
				fld_changed = TRUE;
				TDrefresh(win);

				IIFTsciSetCursorInfo(evcb);
			}
			break;

		  case fdopCLRF: /* Clear the entire field */
			/*
			**  For edit masks, we don't support clearing
			**  the entire field at present.  The
			**  concept is not really compatible with edit masks.
			**  We just beep for now.
			*/
			if (edit_mask)
			{
				FTbell();
				break;
			}

			typed_last = FALSE;

			frm->frchange = TRUE;
			*chgbit |= (fdI_CHG | fdX_CHG);
			*chgbit &= ~fdVALCHKED;
			if (rowinfo)
			{
				*rowinfo |= fdtfRWCHG;
			}
			if (derivesrc && !agginval)
			{
				(*IIFTiaInvalidateAgg)(hdr, fdIASET);
				agginval = TRUE;
			}
			if (noecho)
			{
				noechoptr = bufr;
				*noechoptr = '\0';
				noechox = noechoy = 0;
			}
			else
			{
				TDerschars(win);
				if (reverse)
					win->_curx = win->_maxx - 1;
				win->_clear = TRUE;
				TDrefresh(win);
				if (inrngmd)
				{
					rgfptr->hcur = rgfptr->tcur = NULL;
				}
				else if (scroll_fld)
				{
					IIFTcsfClrScrollFld(scroll_fld,
						reverse);
				}
			}
			fld_changed = TRUE;
			break;

		  case fdopRUB:	 /* Delete the character to the left
				 ** of the cursor
				 */
			typed_last = FALSE;
			frm->frchange = TRUE;
			*chgbit |= (fdI_CHG | fdX_CHG);
			*chgbit &= ~fdVALCHKED;
			if (rowinfo)
			{
				*rowinfo |= fdtfRWCHG;
			}
			if (derivesrc && !agginval)
			{
				(*IIFTiaInvalidateAgg)(hdr, fdIASET);
				agginval = TRUE;
			}
			if (edit_mask)
			{
				i4	curs_pos;
				bool	warn;
				char	*decpt;
				i4	xyz;
				i4	ch_input = '@';
				i4	opos;

				if (fmttype == F_NT)
				{
					opos = curs_pos = win->_curx;
					TDwin_str(win, bufr, (bool) FALSE);
					if (!CMdigit(&bufr[curs_pos - 1]))
					{
						if (--curs_pos > 0)
						{
							curs_pos = fmt_ntposition(bufr, win->_maxx, curs_pos, -1);
							TDmove(win, 0,curs_pos);
							TDrefresh(win);
						}
						break;
					}
					decpt = STindex(bufr, dec_sym, 0);
					if (decpt == NULL)
					{
						decpt = bufr + win->_maxx;
					}
					if (&bufr[curs_pos] > decpt)
					{
						curs_pos--;
					}
					for (xyz = opos - 1; xyz > 0; xyz--)
					{
						bufr[xyz] = bufr[xyz - 1];
					}
					bufr[0] = ' ';
					nval = (*FTgetval)(afld);
					isnullable = AFE_NULLABLE(nval->fvdbv->db_datatype);
					if (fmt_ntmodify(lfmt, isnullable,
						ch_input, bufr, &curs_pos, nbuf,
						&dummy) == FALSE)
					{
						FTbell();
						break;
					}
					TDmove(win, 0, 0);
					if  (*nbuf == EOS)
					{
						TDerschars(win);
					}
					else
					{
						TDaddstr(win, nbuf);
						curs_pos = fmt_ntposition(nbuf,
										 win->_maxx, curs_pos, -1);
						TDmove(win, 0, curs_pos);
					}
					TDrefresh(win);
				}
				else
				{
					curs_pos = fmt_stposition(lfmt,
						win->_curx - 1, -1);
					if (curs_pos <= win->_curx - 1)
					{
						STcopy(ERx(" "), bufr);
						fmt_stvalid(lfmt, bufr, 1,
							curs_pos, &warn);
						TDmove(win, 0, curs_pos);
						TDaddchar(win, bufr, reverse);
						TDmove(win, 0, curs_pos);
						TDrefresh(win);
					}
					else
					{
						break;
					}
				}
			}
			else if (!noecho)
			{
				y = win->_cury;
				x = win->_curx;

				if ((reverse && TDrmvright(win, 1) != FAIL)
				    || (!reverse && TDmvleft(win, 1) != FAIL))
				{
					/*
					**  If not at logical start of window,
					**  then delete char.
					*/

					if (inrngmd)
					{
						FTrdelchar(win, rgfptr, reverse);
						if (reverse)
						{
							if (win->_cury == 0 && win->_curx == win->_maxx - 1)
							{
								if (FTrmvright(win, rgfptr, reverse) != FAIL)
								{
									FTrmvleft(win, rgfptr, reverse);
								}
							}
						}
						else
						{
							if (win->_cury == 0 && win->_curx == 0)
							{
								if(FTrmvleft(win, rgfptr, reverse) != FAIL)
								{
									FTrmvright(win, rgfptr, reverse);
								}
							}
						}
						TDtouchwin(win);
						TDrefresh(win);
						break;
					}
					else if (scroll_fld)
					{
						IIFTmdsMvDelcharScroll(win,
							scroll_fld, reverse);
					}
					else
					{
						if (reverse)
							TDrdelch(win);
						else
							TDdelch(win);
					}
				}
				else
				{
					/*
					**  We are at the logical start
					**  of the window.
					*/

					if (inrngmd)
					{
						if (rgfptr->hcur != NULL)
						{
							if (rgfptr->hcur == rgfptr->hbuf)
							{
								rgfptr->hcur = NULL;
							}
							else
							{
								(rgfptr->hcur)++;
								CMprev(rgfptr->hcur, rgfptr->hbuf);
								(rgfptr->hcur)--;
							}
							break;
						}
					}
					else if (scroll_fld)
					{
						IIFTrbsRuboutScroll(scroll_fld,
							reverse);
						break;
					}

					FTbell();
				}
				TDrefresh(win);
			}
			else
			{
				if (noechoptr > bufr)
				{
					char	*cp;

					noechoptr--;
					for (cp = noechoptr; *cp; cp++)
					{
						*cp = *(cp + 1);
					}
					if (noechox == 0)
					{
					    /* OK since (noechoptr > bufr). */
					    noechox = win->_maxx - 1;
					    noechoy--;
					}
					else
					{
					    noechox--;
					}
				}
			}
			fld_changed = TRUE;
			break;

		  case fdopREFR: /* Refresh the screen */
		  {
			/* if graphics on frame, erase graphics screen */
			if ((FTgtflg & FTGTACT) != 0)
				(*iigtclrfunc)();
			TDrefresh(curscr);
			/* if graphics on frame, refresh graphics */
			if ((FTgtflg & FTGTACT) != 0)
				(*iigtreffunc)();

# ifdef DATAVIEW
			_VOID_ IIMWrRefresh();
# endif	/* DATAVIEW */

			break;
		  }

		  case fdopPREV: /* move to prev field, wrap if last */
		  {
			typed_last = FALSE;

			nval = (*FTgetval)(frm->frfld[FTfldno]);
			if (noecho)
			{
				FTtxtmv(bufr, nval);
			}

			if (*chgbit & fdI_CHG)
			{
				FTfld_str(hdr, nval, win);
			}
			if (*chgbit & fdVALCHKED)
			{
				was_valid = TRUE;
			}
			FTfld_str(hdr, nval, win);
			if (derivesrc && !agginval)
			{
				/*
				** Invalidate on prev. because this
				** can instantiate a non-nullable numeric
				** column.  (And possibly the entire row.)
				*/
				(*IIFTiaaInvalidAllAggs)(frm, tbl);
				agginval = TRUE;
			}
			if (avcb->val_prev)
			{
				if (!FTvalidate(frm, FTfldno, bufr, tbl,
					mode, win, &fld_changed, chgbit))
				{
					if (noecho)
					{
						noechoptr = bufr;
						noechox = noechoy = 0;
					}
					agginval = FALSE;
					break;
				}
				if (!was_valid && *chgbit & fdVALCHKED)
				{
					need_ref = TRUE;
				}
			}
			else
			{
				FTfldchng( frm, &fld_changed );
			}
			/* 19-Mar-1987 fix bug */
			if (inrngmd)
			{
				FTrngfldres(rgfptr, win, reverse);
				TDrefresh(win);
			}
			if (scroll_fld)
			{
				IIFTsfrScrollFldReset(win, scroll_fld, reverse);
				TDrefresh(win);
			}

			IIFTsciSetCursorInfo(evcb);

			/*
			**  We now activate on going to
			**  previous fields if ftactprev is set.
			*/
			if (avcb->act_prev && hdr->fhintrp != 0)
			{
				frm->frcurfld = FTfldno;
				return(hdr->fhintrp);
			}
		  }

		  case fdopUP:	 /* move cursor up in the field */
		  case fdopDOWN: /* move cursor down in the field */
			typed_last = FALSE;

			nval = (*FTgetval)(frm->frfld[FTfldno]);

			if (derivesrc && !agginval)
			{
				/*
				** Invalidate on an arrow up/down because this
				** can instantiate the row.
				*/
				(*IIFTiaaInvalidAllAggs)(frm, tbl);
				agginval = TRUE;
			}

			if (noecho)
			{
				FTtxtmv(bufr, nval);
			}

			if (*chgbit & fdI_CHG)
			{
				FTfld_str(hdr, nval, win);
				need_ref = TRUE;
			}
			if (tbl)
			{
# ifdef DATAVIEW
				/*
				**  If MWS returned this code, the user must
				**  be at an extreme row.  Set the window
				**  parameters to reflect this so that
				**  appropriate things happen in FT.
				*/
				if (IIMWimIsMws())
				{
					if (code == fdopUP)
						win->_cury = 0;
					else if (code == fdopDOWN)
						win->_cury = win->_maxy - 1;
				}
# endif	/* DATAVIEW */

				/*
				**  BUG 1946 - Do not activate column
				**  when in middle of multiple
				**  line column, otherwise it screws
				**  up the control on returning
				**  (ncg)
				*/
				if ((code == fdopPREV) ||
					(code == fdopUP && win->_cury == 0) ||
					(code ==fdopDOWN && win->_cury == win->_maxy -1))
				{
					if (code == fdopUP && !inrngmd &&
						(*IIFTldrLastDsRow)(tbl) &&
						(*rowinfo & fdtfFKROW) &&
						!(*rowinfo & fdtfRWCHG))
					{
						return(fdopDELROW);
					}
					/*
					**  If user moves cursor up or down
					**  and validation on next field is on,
					**  validate the current tf cell
					**  before doing other things.
					**  This enhancement addresses
					**  problem reported as bug 20581.
					*/
					if (code != fdopPREV)
					{
					    if (avcb->val_next)
					    {
						if (*chgbit & fdVALCHKED)
						{
						    was_valid = TRUE;
						}
						if (!FTvalidate(frm, FTfldno,
						    bufr, tbl, mode, win,
						    &fld_changed, chgbit))
						{
						    if (noecho)
						    {
							noechoptr = bufr;
							noechox = noechoy = 0;
						    }
						    agginval = FALSE;
						    break;
						}
						if (!was_valid
						    && (*chgbit & fdVALCHKED))
						{
						    need_ref = TRUE;
						}
					    }
					    else
					    {
						FTfldchng(frm, &fld_changed);
					    }
					}

					if (inrngmd)
					{
						FTrngfldres(rgfptr,
								win, reverse);
						TDrefresh(win);
					}
					else if (scroll_fld)
					{
						IIFTsfrScrollFldReset(win,
							scroll_fld, reverse);
						TDrefresh(win);
					}

					IIFTsciSetCursorInfo(evcb);

					if ((code != fdopPREV || avcb->act_prev)
							&& hdr->fhintrp != 0)
					{
						frm->frcurfld = FTfldno;
						if (need_ref)
						{
							TDrefresh(win);
						}
						tbl->tfdelete = code;
						return(hdr->fhintrp);
					}
				}
			}
			else if ( scroll_fld )
			{
				/*
				** This section has been added to fix 55636.
				** Should only get here if simple scroll field.
				*/
				IIFTsfrScrollFldReset(win, scroll_fld, reverse);
				TDrefresh(win);
			}

			/* fall through if not interrupted */



		  case fdopLEFT:	/* move the cursor left IN the field */
		  case fdopRIGHT:	/* move the cursor right IN the fld */
			if (code == fdopLEFT || code == fdopRIGHT)
			{
			    if (noecho)
			    {
				/* Left and Right have no use with noecho. */
				break;
			    }
			    else
			    {
				typed_last = FALSE;

				if (inrngmd)
				{
				    if (code == fdopLEFT)
				    {
					FTrmvleft(win, rgfptr, reverse);
				    }
				    else
				    {
					FTrmvright(win, rgfptr, reverse);
				    }
				    TDtouchwin(win);
				    TDrefresh(win);
				    break;
				}
				if (scroll_fld)
				{
				    if (code == fdopLEFT)
				    {
					IIFTmlsMvLeftScroll(win, scroll_fld,
					    reverse);
				    }
				    else
				    {
					IIFTmrsMvRightScroll(win, scroll_fld,
					    reverse);
				    }
				    TDrefresh(win);
				    break;
				}
			    }

			    /*
			    **  If edit mask is active for field, then
			    **  move according to edit mask so we can
			    **  skip over constant characters.  This
			    **  doesn't handle double byte characters
			    **  at all.  We'll have to cross that bridge
			    **  when we get to it.
			    */
			    if (edit_mask)
			    {
				i4	dir;
				i4	target;
				i4	newpos;

				if (code == fdopLEFT)
				{
					dir = -1;
					if ((target = win->_curx - 1) < 0)
					{
						target = 0;
					}
				}
				else
				{
					dir = 1;
					if ((target = win->_curx + 1) >=
						win->_maxx)
					{
						target = win->_maxx - 1;
					}
				}
				if (fmttype == F_NT)
				{
					nval = (*FTgetval)(afld);
					TDwin_str(win, bufr, (bool) FALSE);
					newpos = win->_curx + dir;
					newpos = fmt_ntposition(bufr,
						win->_maxx, newpos, dir);
				}
				else
				{
					newpos = fmt_stposition(lfmt, target,
						dir);
				}
				TDmove(win, 0, newpos);
				TDrefresh(win);
				break;
			    }
			}

			IIFTsciSetCursorInfo(evcb);

			if (need_ref)
			{
				TDtouchwin(win);
				TDrefresh(win);
			}
			if ((mvcode = FTmove(code, frm, &FTfldno,
				&win, (i4)1, evcb)) != fdNOCODE)
			{
				return(mvcode);
			}
			FTfldattr(frm, FTfldno, &lcase, &ucase,
				&auto_tab, &noecho, bufr, &bufrend,
				&chgbit, win, &reverse,
				&derivesrc, &agginval, &hdr, &rowinfo, &lfmt,
				&edit_mask, &fmttype);
			noechoptr = bufr;
			noechox = noechoy = 0;
			on_newfld = TRUE;
			break;

		  case fdopWORD:
			/*
			**  Moving foward a word is not compatible with
			**  edit masks.  We just beep for now.
			*/
			if (edit_mask)
			{
				FTbell();
				break;
			}
			else if (!noecho)
			{
				typed_last = FALSE;

				FTword(frm, FTfldno, win, 1);
				TDrefresh(win);
			}
			break;

		  case fdopBKWORD:
			/*
			**  Moving back a word is not compatible with
			**  edit masks.  We just beep for now.
			*/
			if (edit_mask)
			{
				FTbell();
				break;
			}
			else if (!noecho)
			{
				typed_last = FALSE;

				FTword(frm, FTfldno, win, -1);
				TDrefresh(win);
			}
			break;

		  case fdopVERF:
			/*
			**  Editing field contents in an editor is
			**  not compatible with edit masks.  One
			**  will lose the benefits of having edit
			**  masks if we allowed this.
			**  We just beep for now.
			*/
			if (edit_mask)
			{
				FTbell();
				break;
			}
			else if (!noecho)
			{
				if (derivesrc && !agginval)
				{
					(*IIFTiaInvalidateAgg)(hdr, fdIASET);
					agginval = TRUE;
				}

				/* leaves cursor in first char. position */
				typed_last = FALSE;

				FTvi(frm, FTfldno, win);
				TDrefresh(win);
				frm->frchange = TRUE;
				*chgbit |= (fdI_CHG | fdX_CHG);
				*chgbit &= ~fdVALCHKED;
				if (rowinfo)
				{
					*rowinfo |= fdtfRWCHG;
				}
			}
			break;

		  case fdopADUP: /* auto duplication */
		  {
			i4	fldtype;
			i4	curcol;
			i4	currow;

			/*
			**  Do not auto-duplicate if sideway srolling is
			**  on or if on a table field or if form is
			**  in QUERY mode. (dkh)
			*/
			if (inrngmd || tbl != NULL || mode == fdmdQRY)
			{
				FTbell();
				continue;
			}
			*chgbit |= (fdI_CHG | fdX_CHG);
			*chgbit &= ~fdVALCHKED;
			if (!noecho)
			{
				typed_last = FALSE;

				nval = (*FTgetval)(afld);

				if (tbl != NULL)
				{
					fldtype = FT_TBLFLD;
					curcol = tbl->tfcurcol;
					currow = tbl->tfcurrow;
				}
				else
				{
					fldtype = FT_REGFLD;
					curcol = 0;
					currow = 0;
				}

				(*FTdmpmsg)(ERx("CHANGED FIELD %s (autodup)\n"),
						hdr->fhtitle );
				(*FTdmpcur)();

				if (!(*FTfieldval)(frm, FTfldno, FT_UPDATE,
						fldtype, curcol, currow))
				{
					if (scroll_fld)
					{
						IIFTsfrScrollFldReset(win,
							scroll_fld, reverse);
					}
					else
					{
						if (reverse)
							newx = win->_maxx - 1;
						else
							newx = 0;
						TDmove(win, (i4) 0, newx);
					}
					TDrefresh(win);
					break;
				}

				/*
				**  No need to check for lower/case
				**  attribute since everything should
				**  already have been converted by now.
				*/

				/*
				**  No need to save contents of bufr
				**  here since autodup is only valid
				**  for fields that echo.
				*/

				FTfld_str(hdr, nval, win);

				FTbotsav();
				if (avcb->val_next)
				{
				    retval2 = (*FTvalfld)(frm, FTfldno, fldtype,
					    curcol, currow, &val_error);

				    /*
				    ** Derivation source field, so ensure
				    ** derived values are displayed.
				    */
				    if (hdr->fhdrv)
				    {
					TDrefresh(FTwin);
				    }

				    if (retval2 == FALSE)
				    {
					FTbotrst();
					if (scroll_fld)
					{
						IIFTsfrScrollFldReset(win,
							scroll_fld, reverse);
					}
					else
					{
						if (reverse)
							newx = win->_maxx - 1;
						else
							newx = 0;
						TDmove(win, (i4) 0, newx);
					}
					TDrefresh(win);
					break;
				    }
				}

				TDtouchwin(win);
				/* if no auto-tab, move cursor to last pos. */
				if (!auto_tab)
				{
					if (scroll_fld)
					{
						/* Position cursor at fld end */
						IIFTsfeScrollFldEnd(win,
							scroll_fld, reverse);
					}
					else
					{
						if (reverse)
							newx = 0;
						else
							newx = win->_maxx - 1;
						TDmove(win, win->_maxy - 1,
							newx);
					}
					typed_last = TRUE;
				}
				TDrefresh(win);

				/*
				**  If no auto-tab set then just wait in field.
				*/
				if (!auto_tab)
				{
					break;
				}

				IIFTsciSetCursorInfo(evcb);

				if (avcb->act_next && hdr->fhintrp != 0)
				{
					frm->frcurfld = FTfldno;
					return(hdr->fhintrp);
				}

				/*
				**  Use fdopRET and not fdopNEXT so that
				**  table fields reaching the end of the
				**  window will not go to the next
				**  field, but will try to TDscroll
				**  down.  RET and NEXT are equivalent
				**  with normal fields.
				*/

				if ((mvcode = FTmove(fdopRET, frm, &FTfldno,
					&win, (i4)1, evcb)) != fdNOCODE)
				{
					return(mvcode);
				}
				FTfldattr(frm, FTfldno, &lcase, &ucase,
					&auto_tab, &noecho, bufr, &bufrend,
					&chgbit, win, &reverse,
					&derivesrc, &agginval, &hdr, &rowinfo,
					&lfmt, &edit_mask, &fmttype);
				noechoptr = bufr;
				noechox = noechoy = 0;
				on_newfld = TRUE;
				if (!edit_mask)
				{
					TDrefresh(win);
				}
			}
			break;
		  }

		  case fdopMODE:
			if (!noecho)
			{
				FTimod = 1 - FTimod;

			/*
				TDrefresh(win);
			*/
			}
			break;

		  case fdopERR:	 /* Character is unrecognized, add it
				 ** to the field as is.
				 */
		  {
			/*
			**  If the character is a control char,
			**  then ignore it.
			*/

			if (CMcntrl(inp->ks_ch))
			{
				FTbell();
				break;
			}

			have_edited_char = FALSE;

			/*
			**  If we have an edit mask and this field
			**  has enabled using the mask, then change
			**  for validity of input.
			*/
# ifdef OLD
			if (/* hdr->fhd2flags & fdUSEEMASK && */
				lfmt->fmt_emask != NULL && !noecho &&
				!inrngmd && win->_maxy == 1 &&
				scroll_fld == NULL)
# endif
			if (lcase)
			{
				CMtolower(inp->ks_ch, inp->ks_ch);
			}
			else if (ucase)
			{
				CMtoupper(inp->ks_ch, inp->ks_ch);
			}

			if (edit_mask)
			{
				i4	num_good;
				bool	warn;

				CMcpychar(inp->ks_ch, ebuf);
				ebuf[CMbytecnt(inp->ks_ch)] = EOS;

				/*
				**  Note that the warn parameter is not
				**  used in the character world since
				**  we only handle one character at a
				**  time.  Even if it is pasted from
				**  xterm.
				*/
				if (fmttype == F_ST || fmttype == F_DT)
				{
					num_good = fmt_stvalid(lfmt, ebuf,
						(i4) 1, (i4) win->_curx,
						&warn);

					if (num_good != 1)
					{
						/*
						**  Will need to check what
						**  option user has set for
						**  indicating an invalid
						**  character was entered.
						**  User may not want a bell.
						*/
						FTbell();
						break;
					}
				}
				else
				{
					bool	isnullable;
					bool	dummy;
					i4	pos;

					nval = (*FTgetval)(afld);
					isnullable = AFE_NULLABLE(nval->fvdbv->db_datatype);
					TDwin_str(win, bufr, (bool) FALSE);
					pos = win->_curx;
                    
                    /*
                    ** If this is a numeric field then check
                    ** to see if the buffer has numeric 
                    ** characters. If not then NULL it.
                    */

                    if (fmttype == F_NT)
                    {
                        reg u_char	*sptr;
                        ADF_CB *cb = FEadfcb();
                        char decimal_sym[1];
                        bool found = FALSE;
                        decimal_sym[0] = cb->adf_decimal.db_decimal;
                       
                        for (sptr = (u_char *) bufr; *sptr != EOS; CMnext(sptr))
                        {
                            if (CMdigit(sptr) || *sptr == decimal_sym[0] || *sptr == '-')
                            {
                                found = TRUE;
                                break;
                            }
                        }
                        if (!found)
                            *bufr = EOS;
                    }                     
					/*
					**  Entering the field, position cursor
					**  on the decimal point if there is
					**  one.  fmt_ntmodify does all of the
					**  work.
					*/
					fmt_ntmodify(lfmt, isnullable, *ebuf,
						bufr, &pos, nbuf, &dummy);

					TDmove(win, 0, 0);
					TDaddstr(win, nbuf);
					TDmove(win, 0, pos);
					TDtouchwin(win);
				}
				have_edited_char = TRUE;
			}

			frm->frchange = TRUE;
			*chgbit |= (fdI_CHG | fdX_CHG);
			*chgbit &= ~fdVALCHKED;
			if (rowinfo)
			{
				*rowinfo |= fdtfRWCHG;
			}
			if (derivesrc && !agginval)
			{
				(*IIFTiaInvalidateAgg)(hdr, fdIASET);
				agginval = TRUE;
			}

			fld_changed = TRUE;

			if (noecho)
			{
				bool	update = TRUE;

				/*
				** set lastch to trigger auto_tab at end
				** of field
				*/
				if (noechox == win->_maxx -
					CMbytecnt(inp->ks_ch) &&
					noechoy == win->_maxy - 1)
				{
					/*
					**  If we just fit, set lastch
					**  so we can autotab out.
					*/
					lastch = FAIL;
				}
				else
				{
					lastch = OK;
				}

				if (noechox + CMbytecnt(inp->ks_ch) >
					win->_maxx)
				{
					if (noechoy >= win->_maxy - 1)
					{
						/*
						**  If we can't fit, then
						**  just autotab out.
						**  This really is only a
						**  problem for double byte
						**  characters.
						*/
						update = FALSE;
						lastch = FAIL;
					}
					else
					{
						noechox = 0;
						noechoy++;
						lastch = OK;
					}
				}

				if (update)
				{
					CMbyteinc(noechox, inp->ks_ch);
					CMcpychar(inp->ks_ch, noechoptr);
					CMnext(noechoptr);
					if (noechoptr > bufrend)
					{
						*noechoptr = '\0';
					}
				}
				y = noechoy;
				if (lastch != FAIL)
				{
					x = noechox;
				}
				else
				{
					x = win->_maxx - CMbytecnt(inp->ks_ch);
				}
			}
			else
			{
				y = win->_cury;
				x = win->_curx;

				/*
				**  insert mode, but only if not doing
				**  edit masks.
				*/
				if (FTimod && !have_edited_char)
				{
					/* add the character */

					/*
					**  Check to see if we have
					**  to push anything off the
					**  end.
					*/

					if (inrngmd)
					{
						FTrinschar(win, rgfptr,
							inp->ks_ch, reverse);
						TDtouchwin(win);
						TDrefresh(win);
						break;
					}
					else if (scroll_fld)
					{
					    lastch = IIFTsicScrollInsertChar
						(win, scroll_fld, reverse,
						inp->ks_ch, &nospace);
					}
					else
					{
						if (reverse)
							lastch = TDrinsstr(win,
								inp->ks_ch);
						else
							lastch = TDinsstr(win,
								inp->ks_ch);
					}
				}
				else		/* overstrike mode */
				{
					if (inrngmd)
					{
						FTraddchar(win, rgfptr,
							inp->ks_ch, reverse);
						TDtouchwin(win);
						TDrefresh(win);
						break;
					}
					else if (scroll_fld)
					{
					    lastch = IIFTsacScrollAddChar
						(win, scroll_fld, reverse,
						inp->ks_ch, &nospace);
					}
					else if (fmttype != F_NT)
					{
						/*
						**  If not a numeric template,
						**  we should add the char.
						**  fmt_ntmodify() already
						**  updated the value so
						**  there is no need to
						**  do it here.
						**
						**  Note that it is not
						**  possible to do auto-tabbing
						**  for numeric templates
						**  since input starts on
						**  the right of the field.
						**  and thus we can't assume
						**  that data entered into
						**  the rightmost position
						**  should cause an auto-tab.
						*/
						u_char	*ecp;

						if (have_edited_char)
						{
							ecp = ebuf;
						}
						else
						{
							ecp = inp->ks_ch;
						}
						lastch = TDaddchar(win,
							ecp, reverse);
					}
				}
				/* display the change */

				if (!have_edited_char)
				{
					TDrefresh(win);
				}
			}

			if (!inrngmd &&
				(CMdbl1st(inp->ks_ch) && y == win->_maxy - 1
					&& x == win->_maxx - 1)
				|| (scroll_fld && nospace))
			{
				/*
				** Not in the range mode and if a Double Byte
				** character was added or inserted to the last
				** position of the window, beep and do nothing.
				*/
				FTbell();
				typed_last = FALSE;

				if (have_edited_char)
				{
					TDrefresh(win);
				}
				break;
			}

			/*
			**  If the character was added to the
			**  last position in the field, move
			**  to the next field if auto-tab is
			**  in effect.
			**
			**  Noecho is a separate check since no guarantee
			**  that field won't be 'reverse'.  Also, when lastch
			**  is FAIL and noecho, x will be "maxx - CMbytecnt",
			**  so no need to check.
			*/

			if (lastch == FAIL && y == win->_maxy - 1
			    && ((scroll_fld) || (noecho)
				|| (reverse && x == 0)
				|| (x == win->_maxx - CMbytecnt(inp->ks_ch)
					&& !reverse)))
			{
				if (have_edited_char)
				{
					TDrefresh(win);
				}

				if (!typed_last)
					typed_last = TRUE;
				else
					typed_last = TRUUEST;
				if (auto_tab && fmttype != F_NT)
				{
					typed_last = FALSE;

					nval = (*FTgetval)(afld);

					FTfldchng( frm, &fld_changed );

					if (noecho)
					{
						FTtxtmv(bufr, nval);
					}

					FTfld_str(hdr, nval, win);

					if (derivesrc)
					{
						/*
						** Don't check for agginval
						** since already set for
						** partial row-invalidation
						** at top of fdopERR code.
						** Invalidate on autotab out.
						** This can instantiate the row.
						*/
						(*IIFTiaaInvalidAllAggs)(frm,
						    tbl);
						agginval = TRUE;
					}

					if (avcb->val_next)
					{
						if (!FTvalidate(frm,
						    FTfldno, bufr, tbl, mode,
						    win, &fld_changed, chgbit))
						{
						    if (noecho)
						    {
							    noechoptr = bufr;
							    /*
							    **  Set to
							    **  beginning
							    **  of field.
							    */
							    noechoy = 0;
							    noechox = 0;
						    }
						    agginval = FALSE;
						    break;
						}
					}
					else
					{
						FTfldchng(frm, &fld_changed);
					}

					if (inrngmd)
					{
						FTrngfldres(rgfptr,
								win, reverse);
					}
					if (scroll_fld)
					{
						IIFTsfrScrollFldReset(win,
							scroll_fld, reverse);
					}

					TDtouchwin(win);
					TDrefresh(win);

					/*
					** if the field was signalled
					** to to interrupt on this
					** field, return control to
					** the upper level program
					** and return the value of
					** this field
					*/

					/*
					**  This is here so inquire on
					**  command will return "nextfield"
					**  value when we autotab out
					**  of a field.
					*/
					evcb->event = fdopNXITEM;

					IIFTsciSetCursorInfo(evcb);

					if (avcb->act_next && hdr->fhintrp != 0)
					{
						frm->frcurfld = FTfldno;
						return(hdr->fhintrp);
					}

					/*
					**  Use opRET and not opNEXT so
					**  that table fields reaching
					**  the end of the window won't
					**  go to the next field, but
					**  will try to TDscroll down.
					**  RET and NEXT are equivalent
					**  with normal fields.
					*/

					if ((mvcode = FTmove(fdopRET, frm,
						&FTfldno, &win,
						(i4)1, evcb)) != fdNOCODE)
					{
						return(mvcode);
					}
					FTfldattr(frm, FTfldno, &lcase, &ucase,
						&auto_tab, &noecho, bufr,
						&bufrend, &chgbit,
						win, &reverse, &derivesrc,
						&agginval, &hdr, &rowinfo,
						&lfmt, &edit_mask, &fmttype);
					noechoptr = bufr;
					noechox = noechoy = 0;
					on_newfld = TRUE;
				}
				else
				{
					/*
					**  No need to handle skipping of
					**  template constants here since
					**  this code is for reaching the
					**  last position in a field with
					**  auto tab turned off.  In this
					**  case, we follow the default
					**  (i.e., no edit mask) behavior
					**  and leave the cursor in the
					**  last position.
					*/

					if (first_bell
						&& (typed_last == TRUUEST))
					{
						first_bell = FALSE;
						FTbell();
					}
					continue;
				}
			}
			else
			{
				typed_last = FALSE; /* not the last char */

				/*
				**  Now do check to see if we have to skip
				**  over any template constant characters.
				**  fmt_stpoistion() will take care of
				**  the case where we tried to move on top
				**  of the last character in a field and
				**  that character is a template constant
				**  character.  In that case, fmt_stposition()
				**  should leave us where we are.
				*/
				if (have_edited_char)
				{
					if (fmttype != F_NT)
					{
						i4	newpos;

						newpos = fmt_stposition(lfmt,
							win->_curx, (i4) 1);

						TDmove(win, win->_cury, newpos);
					}
					TDrefresh(win);
				}
			}
			break;
		  }

		  case fdopSCRUP:
		  case fdopSCRDN:
			typed_last = FALSE;
			/*
			**  Scroll table field in cursor on
			**  a table field.
			*/
			if (tbl)
			{
				i4	repts;
				i4	tmvcode;
				FLDVAL	*val;

				val = (*FTgetval)(frm->frfld[FTfldno]);
				if (derivesrc && !agginval)
				{
				    /*
				    ** Invalidate on a scroll because this
				    ** can instantiate the row.
				    */
				    (*IIFTiaaInvalidAllAggs)(frm, tbl);
				    agginval = TRUE;
				}

				/*
				** This code moved out/ahead of avcb->val_next
				** section to resolve bug 47663.
				*/
				if (*chgbit & fdI_CHG)
				{
					FTfld_str(hdr, val, win);
					need_ref = TRUE;
				}

				/*
				**  Validate current row_column.
				*/
				if (avcb->val_next)
				{
					if (*chgbit & fdVALCHKED)
					{
						was_valid = TRUE;
					}
					if (!FTvalidate(frm, FTfldno, bufr,
						tbl, mode, win, &fld_changed,
						chgbit))
					{
						if (noecho)
						{
							noechoptr = bufr;
							noechox = noechoy = 0;
						}
						agginval = FALSE;
						break;
					}
					if (!was_valid && *chgbit & fdVALCHKED)
					{
						need_ref = TRUE;
					}
				}
				else
				{
					FTfldchng(frm, &fld_changed);
				}

				/*
				**  Figure out how many rows to scroll.
				*/
				repts = tbl->tfrows - 1;

				/*
				**  Fix up scroll counts, etc.
				*/
				if (code == fdopSCRUP)
				{
					tmvcode = fdopDOWN;
					if (tbl->tfcurrow != tbl->tfrows - 1)
					{
						repts += tbl->tfrows - 1
								- tbl->tfcurrow;
					}

					/*
					**  Fix for BUG 9320. (dkh)
					*/
					win->_cury = win->_maxy - 1;
				}
				else
				{
					tmvcode = fdopUP;
					if (tbl->tfcurrow != 0)
					{
						repts += tbl->tfcurrow;
					}

					/*
					**  Fix for BUG 9320. (dkh)
					*/
					win->_cury = 0;
				}

				if (inrngmd)
				{
					FTrngfldres(rgfptr, win, reverse);
					TDrefresh(win);
				}
				else if (scroll_fld)
				{
					IIFTsfrScrollFldReset(win, scroll_fld,
						reverse);
					TDrefresh(win);
				}

				IIFTsciSetCursorInfo(evcb);

				/*
				**  If activate, then do that first.
				*/
				/*
				**  Fix for BUG 9697. (dkh)
				*/
				if (hdr->fhintrp != 0)
				{
					frm->frcurfld = FTfldno;
					if (need_ref)
					{
						TDrefresh(win);
					}
					tbl->tfdelete = code;
					return(hdr->fhintrp);
				}

				/*
				**  If can scroll horizontally, then
				**  call special routines.
				*/
				if (inrngmd)
				{
					FTscrrng(frm, FTfldno, tmvcode,
						repts, tbl, &win);
				}
				else
				{
					/*
					**  Otherwise, do normal thing.
					*/
					if ((tmvcode = FTmove(tmvcode, frm,
						&FTfldno, &win,
						repts, evcb)) != fdNOCODE)
					{
						return(tmvcode);
					}
				}
				FTfldattr(frm, FTfldno, &lcase, &ucase,
					&auto_tab, &noecho, bufr,
					&bufrend, &chgbit, win, &reverse,
					&derivesrc, &agginval, &hdr, &rowinfo,
					&lfmt, &edit_mask, &fmttype);
				noechoptr = bufr;
				noechox = noechoy = 0;
				on_newfld = TRUE;
			}
			else
			{
				/*
				**  Just scroll form.
				*/
				if (code == fdopSCRUP)
				{
					TDscrlup(FTwin, win);
				}
				else
				{
					TDscrldn(FTwin, win);
				}
				/*
				**  No need to call TDrefresh on
				**  win since TDscrlup and TDscrldn
				**  will do that for us.
				*/
			}
			break;

		  case fdopSCRLT:
			typed_last = FALSE;
			TDscrllt(FTwin, win);
			break;

		  case fdopSCRRT:
			typed_last = FALSE;
			TDscrlrt(FTwin, win);
			break;

		  case fdopPRSCR:
		  {
			char	msgline[MAX_TERM_SIZE + 1];

			TDsaveLast(msgline);
			FTprnscr(NULL);
			TDwinmsg(msgline);
			TDrefresh(win);
			break;
		  }

		  case fdopSKPFLD:
# ifdef DATAVIEW
			if (IIMWimIsMws())
			{
			    i4		skipcnt;
			    i4		scode;
			    FIELD	*fld;
			    bool	doact = FALSE;

			    typed_last = FALSE;
			    if (IIMWgscGetSkipCnt(frm, FTfldno,
				&skipcnt) != OK)
			    {
				break;
			    }

			    if (skipcnt == 0)
			    {
				break;
			    }
			    if (skipcnt < 0)
			    {
				scode = fdopPREV;
				skipcnt = -skipcnt;
				/*
				**  Copied stuff from the skipcnt > 0
				**  case to make the two cases
				**  symmetrical.
				*/
				fld = frm->frfld[FTfldno];
				nval = (*FTgetval)(fld);
				hdr = (*FTgethdr)(fld);
				if (noecho)
				{
					FTtxtmv(bufr, nval);
				}
				FTfld_str(hdr, nval, win);
				if (avcb->val_prev)
				{
					if (!FTvalidate(frm, FTfldno, bufr,
						tbl, mode, win, &fld_changed,
						chgbit))
					{
						if (noecho)
						{
							noechoptr = bufr;
						}
						break;
					}
				}
				else
				{
					FTfldchng( frm, &fld_changed );
				}
			    }
			    else
			    {
				scode = fdopNEXT;
				fld = frm->frfld[FTfldno];
				nval = (*FTgetval)(fld);
				hdr = (*FTgethdr)(fld);
				if (noecho)
				{
					FTtxtmv(bufr, nval);
				}

				FTfld_str(hdr, nval, win);

				if (avcb->val_next)
				{
					if (!FTvalidate(frm, FTfldno, bufr,
						tbl, mode, win, &fld_changed,
						chgbit))
					{
						if (noecho)
						{
							noechoptr = bufr;
						}
						break;
					}
				}
				else
				{
					FTfldchng(frm, &fld_changed);
				}
			    }
			    if (inrngmd)
			    {
				FTrngfldres(rgfptr, win, reverse);
				TDrefresh(win);
			    }
			    if (scroll_fld)
			    {
				IIFTsfrScrollFldReset(win, scroll_fld, reverse);
				TDrefresh(win);
			    }

			    doact = FALSE;
			    if (scode == fdopNEXT)
			    {
				if (avcb->act_next)
				{
					doact = TRUE;
				}
			    }
			    else
			    {
				if (avcb->act_prev)
				{
					doact = TRUE;
				}
			    }

			    IIFTsciSetCursorInfo(evcb);

			    if (doact && hdr->fhintrp != 0)
			    {
				frm->frcurfld = FTfldno;
				if (tbl != NULL)
				{
					tbl->tfdelete = code;
				}
				return(hdr->fhintrp);
			    }
			    scode = fdopSKPFLD;
			    if ((mvcode = FTmove(scode, frm, &FTfldno,
				&win, skipcnt, evcb)) != fdNOCODE)
			    {
				return(mvcode);
			    }
			    FTfldattr(frm, FTfldno, &lcase, &ucase,
				&auto_tab, &noecho, bufr, &bufrend,
				&chgbit, win, &reverse, &lfmt, &edit_mask,
				&fmttype);
			    noechoptr = bufr;
			    noechox = noechoy = 0;
			    on_newfld = TRUE;
			}
# endif	/* DATAVIEW */
			break;

		  case fdopSHELL:
			FTshell();
			break;

# ifdef DATAVIEW
			/* Bug 87260 Upfront can send 'scroll to' messges to scroll the tablefield
			** Include the case fdopSCRTO to allow for this.
			*/
		  case fdopSCRTO:
			/* Scroll bar button was moved */
			/* Bug 87710. Return to caller to pass through its tablefield
			** operation code to ensure correct tablefield is scrolled
			*/
			
			return (code);
#endif

		  /*
		  **  User has specified an application
		  **  defined command.	Return control
		  **  to the upper level program and the
		  **  value of this command.
		  */

		  case fdopMENU:
		  default:
			event = evcb->event;
			if ( code != fdopMENU )
			{
				(*FTdmpmsg)(ERx("ACTIVATING ON CHARACTER %s\n"),
					CMunctrl(inp->ks_ch) );
			}
			else
			{
			    if ((event == fdopSCRLT) || (event == fdopSCRRT))
			    {
				evcb->event = fdopMENU;
			    }
			    else
			    {
				(*FTdmpmsg)(ERx("MENU KEY selected\n"));
			    }
			}
			frm->frcurfld = FTfldno;

			nval = (*FTgetval)(frm->frfld[FTfldno]);

			if (noecho)
			{
				FTtxtmv(bufr, nval);
			}

			if (*chgbit & fdI_CHG)
			{
				FTfld_str(hdr, nval, win);
			}

			if (scroll_fld)
			{
				IIFTsfrScrollFldReset(win, scroll_fld, reverse);
				TDrefresh(win);
				/* reposition cursor at bottom of screen */
				FTvisclumu();
			}

			/*
			**  Only validate if menu key was pressed
			**  and ftvalmenu is set.
			*/
			if (evcb->event == fdopMENU)
			{
				if (avcb->val_mu)
				{
					if (!FTvalidate(frm, FTfldno, bufr,
						tbl, mode, win, &fld_changed,
						chgbit))
					{
						if (noecho)
						{
						    noechoptr = bufr;
						    noechox = noechoy = 0;
						}
						agginval = FALSE;
						break;
					}
				}
				if (!inrngmd && avcb->act_mu)
				{
					if (hdr->fhintrp)
					{
						return(hdr->fhintrp);
					}
				}
			}

			/*
			**  FRS key validation/activation will happen
			**  later.
			*/
			if (evcb->event != fdopFRSK)
			{
				FTfldchng(frm, &fld_changed);
			}

			if ((event == fdopSCRLT) || (event == fdopSCRRT))
			{
			    /* User has clicked on menu's '<' or '>'. */
			    IIFTsmShiftMenu(event);
			}

			return(code);

		}
		first_bell = TRUE;
	}
}


FTfldchng( frm, fld_changed )
FRAME	*frm;
bool	*fld_changed;
{
	FIELD	*fld;
	FLDHDR	*rnhdr;

	fld = frm->frfld[FTfldno];

	if ( *fld_changed )
	{
		rnhdr = (*FTgethdr)( fld );
		(*FTdmpmsg)(ERx("CHANGED FIELD %s\n"), rnhdr->fhtitle );
		(*FTdmpcur)();
		*fld_changed = FALSE;
	}
}


/*{
** Name:	FTfldstart	- Determine if user at start of the field
**
** Description:
**	Determine if the user is at the start of the field.  This means
**	that the cursor is at the beginning of the window (end, if a reverse
**	field) and if a scrolling field, is at the appropriate end of
**	the scroll buffer.
**
** Inputs:
**	win		Window user is in.
**	scroll_fd	NULL, or pointer to scroll buffer for the field.
**	reverse		Is this a right-to-left or left-to-right field.
**
** Outputs:
**
**	Returns:
**		TRUE	User at start of the field.
**		FALSE	Not at start of the field.
**
**	Exceptions:
**		None
**
** Side Effects:
**
** History:
**	20-apr-89 (bruceb)	Written.
*/

static bool
FTfldstart(win, scroll_fd, reverse)
WINDOW		*win;
SCROLL_DATA	*scroll_fd;
bool		reverse;
{
    bool	retval = FALSE;

    if ((win->_cury == 0)
	&& ((!reverse && win->_curx == 0)
	    || (reverse && win->_curx == (win->_maxx - 1))))
    {
	if (!scroll_fd)
	    retval = TRUE;
	else
	    retval = IIFTsfsScrollFldStart(scroll_fd, reverse);
    }

    return(retval);
}
