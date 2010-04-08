/*
**  FTbuild
**
**  Copyright (c) 2004 Ingres Corporation
**
**  ftbuild.c
**
**  History
**
**  6/22/85 - Added support for titleless table fields, etc. (dkh)
**  7/18/85 - Added support for NO-ECHO field attribute. (dkh)
**  8/21/85 - Added support for invisible table fields. (dkh)
**  10/11/85 - Added FTsyncdisp as a new interface routine. (dkh)
**	03/04/87 (dkh) - Added support for ADT changes.
**	06/19/87 (dkh) - Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	09-nov-87 (bruceb)
**		Added code for scrolling fields; build such fields
**		by displaying the proper portion of the underlying data.
**	12/05/87 (dkh) - Put in support for tf no column separators.
**	09-feb-88 (bruceb)
**		Moved scrolling field flag to fhd2flags; can't use most
**		significant bit of fhdflags.
**	05/01/88 (dkh) - Updated for Venus.
**	09-may-88 (kenl/bruceb)
**              Set win after FTfullwin is reallocated.
**	05/28/88 (dkh) - Added support for drawing boxes.
**	05/28/88 (dkh) - Integrated Tom Bishop's Venus code changes.
**	05/31/88 (dkh) - Fixed problems due to integration of boxes.
**	06/01/88 (dkh) - Fixed bug with displaying data for the first time.
**	06/18/88 (dkh) - Added code to prevent box trim from showing
**			 up in fields that occupy more than one line.
**	06/22/88 (dkh) - Modified to not clear the border area of a
**			 field so boxes will join correctly.
**	07/12/88 (dkh) - Changed handling of table field row highlighting
**			 so the highlight does not jump around as much
**			 when returning from a nested display statement.
**	07/15/88 (dkh) - Fixed problem with regular trim displayed on
**			 top of a box.
**	08/20/88 (dkh) - Fixed venus bug 3119.
**	09/05/88 (dkh) - Fixed problem with FTfullwin passed in but
**			 is not big enough for current form being built.
**	10/13/88 (dkh) - Added entry point to update FTprevfrm.
**	10/27/88 (dkh) - Performance changes.
**	05-jan-89 (bruceb)
**		Additional parameter to FTbuild for use in FTregbld and
**		FTtblbld.  If original caller was Printform, than don't
**		print out the fvbufr contents.
**	05-jan-89 (bruceb)
**		Don't build invisible fields (not just table fields)
**		unless called from Printform.
**	10-jan-89 (bruceb)
**		If frame's frmflag set to fdREBUILD, set samefrm to FALSE
**		regardless of other logic.  Currently used to rebuild
**		form after changing a field's visibility.  Turn off
**		fdREBUILD after use.
**	01/19/89 (dkh) - Put in a change to make sure box/trim graphics
**			 are always set up correctly for a popup.
**	03/14/89 (brett) - Add windex command string interfaces.
**	04/07/89 (tom) - pass thru the attribute even for trim
**			 for dynamic frames.
**	05/19/89 (dkh) - Fixed bug 5398.
**	06/08/89 (brett) - Change simple field command string. Add simple
**			field title command string.
**	06/12/89 (brett) - Added the intrp parameter for field activation
**			information in IITDfield().
**	07/12/89 (dkh) - Integrated windex code changes from Brett into 6.2.
**	08-sep-89 (bruceb)
**		Handle invisible table field columns.
**	09/22/89 (dkh) - Porting changes integration.
**	09/23/89 (dkh) - More porting changes.
**	09/29/89 (dkh) - Made sure a full screen form is at least as big
**			 as the screen size.
**	12-dec-89 (bruceb)
**		Only modify win's _maxy, _maxx if not from printform.
**	12/27/89 (dkh) - Added support for hot spot trim.
**	01/30/90 (brett) - Added frame flags to IITDnwscrn() call.  This
**			   is needed for popup positioning.
**	03/04/90 (dkh) - Integrated above change (ingres61b3ug/2336).
**	19-mar-90 (bruceb)
**		Added locator support.  While building the form also build
**		object coordinate structures.  Hotspot trim takes precedence
**		over (is recognized before) fields.
**	23-apr-90 (bruceb)
**		Locator support; clicking anywhere within a table field cell,
**		rather than just in the data region, will move the cursor to
**		that cell.
**	07/24/90 (dkh) - A more complete fix for bug 30408.
**	07/26/90 (dkh) - Fixed bug 31985.
**	21-aug-90 (bruceb)	Fix for bug 32724.
**		Don't provide locator support when called fromPrintform.
**	07/26/90 (dkh) - Added support for table field cell highlighting.
**	08/25/90 (dkh) - Integrated bug fix for table field highlighting.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**	09/14/90 (dkh) - Fixed windowview hook so that the correct
**			 coordinates is sent for fixed position popups.
**	03/08/91 (dkh) - Fixed bug 36300.
**	26-may-92 (rogerf) DeskTop Porting Change:
**		Added scrollbar processing inside SCROLLBARS ifdef.
**	05-jun-92 (leighb) DeskTop Porting Change:
**		Changed COORD to IICOORD to avoid conflict
**		with a structure in wincon.h for Windows/NT
**	23-sep-96 (mcgem01)
**		Global data moved to ftdata.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Mar-2005 (lakvi01)
**	    Updated function prototypes.
**    25-Oct-2005 (hanje04)
**        Add prototype for FTregbld() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**    30-May-2006 (hweho01)
**        Changed the old style parameter list in FTregbld(),  
**        prevent compiler error on AIX platform.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	<frsctblk.h>
# include	<si.h>
# include	<st.h>
# include	<er.h>
# include	<erft.h>
# include	<scroll.h>
# ifdef		SCROLLBARS
# include	<wn.h>
# endif

FUNC_EXTERN	WINDOW	*TDsextend();
FUNC_EXTERN	i4	FTattrxlate();
FUNC_EXTERN	VOID	IITDhsHotspotString();
FUNC_EXTERN	VOID	IITDpciPrtCoordInfo();
FUNC_EXTERN	VOID	IITDptiPrtTraceInfo();
FUNC_EXTERN	VOID	IIFTdcaDecodeCellAttr();

VOID	IIFTacsAllocCoordStruct();
VOID	IIFTrfoRegFormObj();

static STATUS FTregbld(
			FRAME	*frm,
			REGFLD	*rfld,
			bool	first,
			WINDOW	*win,
			bool	fromPrintform);

static FRAME	*FTprevfrm = NULL;
static bool	found_box = FALSE;

GLOBALREF	bool	samefrm; 
GLOBALREF	WINDOW	*FTfullwin;
GLOBALREF	WINDOW	*IIFTdatwin;
GLOBALREF	bool	IITDccsCanChgSize;
GLOBALREF	i4	YN;
GLOBALREF	i4	YO;
GLOBALREF	bool	IITDlsLocSupport; /* Locator support requested? */
GLOBALREF	FRS_GLCB	*IIFTglcb;

static bool	loc_support = FALSE;

STATUS
FTbuild(frm, first, usewin, fromPrintform)
FRAME	*frm;
bool	first;
WINDOW	*usewin;
bool	fromPrintform;
{
	i4	eysize = LINES;
	i4	exsize = COLS;
	i4	diff;
	i4	i;
	i4	j;
	TRIM	*trm;
	TRIM	**frtrim;
	FIELD	*fld;
	FIELD	**frfld;
	WINDOW	*win;
	i4	rebuild = FALSE;
	i4	scrsize;
	char	cmd_str[100];
	i4	len;

	if (frm == NULL)
	{
		FTmessage(ERget(E_FT0003_FTbuild__passed_NULL_), TRUE, FALSE);
		return(FAIL);
	}

	samefrm = FALSE;

	if (usewin == (WINDOW *) NULL)
	{
		win = FTfullwin;
	}
	else
	{
		win = usewin;
	}

	/*
	**  Check to see if we are building a form image
	**  for FTsyncdisp.
	*/
	if (FTwin == win)
	{
		if (FTprevfrm == frm)
		{
			samefrm = TRUE;
		}
		else
		{
			FTprevfrm = frm;
		}
	}

	/*
	**  If any fields have changed their visibility, entirely rebuild
	**  the form.
	*/
	if (frm->frmflags & fdREBUILD)
	{
		samefrm = FALSE;
		rebuild = TRUE;
		frm->frmflags &= ~fdREBUILD;
	}

#ifdef	SCROLLBARS
	if (samefrm == FALSE)
	        WNSbarDestroy();
#endif
	/*
	**  If form is a popup and this is NOT the first time,
	**  then no need to build the form again.  The form
	**  image should almost always be uptodate.  The only
	**  glitch is with invisible fields in popup forms.
	**  For such popups (when rebuild is TRUE) recreate the
	**  form anyway.
	*/
	if ((frm->frmflags & fdISPOPUP) && !first && !rebuild)
	{
		return(OK);
	}

	if (win == FTfullwin)
	{

		/*
		**  Note:  If this routine is called with the same frame
		**  at least twice in a row and the size of the frame
		**  changes we are in TROUBLE.	Code below has to
		**  changed.  (dkh)
		*/

		if (frm->frmaxy > FTwmaxy || frm->frmaxx > FTwmaxx)
		{
			if ((diff = frm->frmaxy - FTwmaxy) > LINES)
			{
				eysize = FTwmaxy + diff;
			}
			else
			{
				eysize = FTwmaxy + LINES;
			}
			if ((diff = frm->frmaxx - FTwmaxx) > COLS)
			{
				exsize = FTwmaxx + diff;
			}
			else
			{
				exsize = FTwmaxx + COLS;
			}
			FTwmaxy = eysize;
			FTwmaxx = exsize;
			TDdelwin(FTfullwin);
			TDdelwin(FTcurwin);
			TDdelwin(FTutilwin);
			TDdelwin(IIFTdatwin);
			if (TDonewin(&FTfullwin, &FTcurwin, &FTutilwin,
				FTwmaxy, FTwmaxx) != OK)
			{
				return(FAIL);
			}

			if ((IIFTdatwin = TDsubwin(FTfullwin, FTfullwin->_maxy,
				FTfullwin->_maxx, 0, 0, NULL)) == NULL)
			{
				return(FAIL);
			}

			/*
			**  Must have been passed in with usewin == FTfullwin.
			**  So need to adjust usewin to new FTfullwin.
			*/
			if (usewin != NULL)
			{
				usewin = FTfullwin;
			}
		}
	}

	if (usewin == (WINDOW *) NULL)
	{
		win = FTfullwin;
	}
	else
	{
		win = usewin;
	}

	/*
	**  Don't modify window size if from printform since window can't
	**  expand (special window for printform) and simply not needed.
	*/
	if (!fromPrintform)
	{
	    if (!(frm->frmflags & fdISPOPUP)
		&& (win->_maxy = frm->frmaxy) < LINES - 1)
	    {
		/*
		**  Maxy is lines - 1 to account for the menu line.
		*/
		win->_maxy = LINES - 1;
	    }
	    if (!(frm->frmflags & fdISPOPUP))
	    {
		scrsize = COLS;
		if (IITDccsCanChgSize)
		{
		    if (frm->frmflags & fdWSCRWID)
		    {
			scrsize = YO;
		    }
		    else if (frm->frmflags & fdNSCRWID)
		    {
			scrsize = YN;
		    }
		}
		if ((win->_maxx = frm->frmaxx) < scrsize)
		{
		    win->_maxx = scrsize;
		}
	    }
	}

# ifdef DATAVIEW
	if (!fromPrintform)
	{
		if (first)
		{
			/* Tell MWS that we are starting a new form */
			if (IIMWbfBldForm(frm) == FAIL)
				return(FAIL);
		}
		else if (rebuild)
		{
			if (IIMWshfShowHideFlds(frm) == FAIL)
				return(FAIL);
		}
	}
# endif /* DATAVIEW */

	if (!samefrm)
	{
		TDerase(win);
	}

	/* Locator support requested. */
	if (loc_support = IITDlsLocSupport)
	{
	    if (fromPrintform)
	    {
		/* Mouse support not required when printing the form. */
		loc_support = FALSE;
	    }
	    else
	    {
	        IIFTacsAllocCoordStruct(frm);
	    }
	}

	/*
	**  brett, New screen windex command string.
	**  Note: This is where the sequenced fields of the frame
	**  are put into windex through termdr.
	*/
	if (frm->frmflags & fdISPOPUP)
	{
        	IITDnwscrn(frm->frname, win->_startx, win->_starty, frm->frmaxx,
			frm->frmaxy, frm->frmflags, frm->frm2flags);
	}
	else
	{
        	IITDnwscrn(frm->frname, frm->frposx, frm->frposy, frm->frmaxx,
			frm->frmaxy, frm->frmflags, frm->frm2flags);
	}

	/*
	**  Note that we are cheating here and not putting up the
	**  hotspots again if we are dealing with the same form.
	**  This should be OK for emerald since it is always
	**  building new FRAME structs and samefrm will thus always
	**  be FALSE.
	*/
	if (!samefrm || frm->frmflags & fdTRHSPOT)
	{
		found_box = FALSE;

		j = frm->frtrimno;
		frtrim = frm->frtrim;

		/*
		**  First pass to pick up boxes.
		*/
		for (i = 0; i < j; i++)
		{
			trm = *frtrim++;

			/* If it's a piece of box trim */
			if (trm->trmflags & fdBOXFLD)
			{
				i4	cols;
				i4	rows;
				i4	attr;

				/*
				**  Get size of box encoded in text string.
				**  For now, we ignore it if we can't find
				**  two numbers.
				*/
				if (STscanf(trm->trmstr, ERx("%d:%d"), &rows,
					&cols) != 2)
				{
					continue;
				}

				/* if the box is not invisible */
			   	if (!(trm->trmflags & fdINVISIBLE))
				{
					/*
					**  Translate any display attributes
					**  to be handled by termdr.
					*/
					attr = FTattrxlate(trm->trmflags);

					found_box = TRUE;
					/*
					**  Display box.
					*/
					TDsbox(win, trm->trmy, rows, trm->trmx,
						cols, '|', '-', '+',
						attr, attr, FALSE);
				}
				if (trm->trmflags & fdTRHSPOT)
				{
					IITDhsHotspotString(i, cmd_str);
					IITDhotspot(trm->trmx, trm->trmy, cols,
						rows, cmd_str, 1);
					if (loc_support)
					{
					    IIFTrfoRegFormObj(frm, FHOTSPOT,
						trm->trmy, trm->trmx,
						rows, cols, i, 0, 0);
					}
				}
			}
		}

		frtrim = frm->frtrim;
		/*
		**  Second pass to pick up ordinary text trim.
		*/
		for (i = 0; i < j; i++)
		{
			trm = *frtrim++;
			if (! (trm->trmflags & fdBOXFLD))
			{
				len = STlength(trm->trmstr);
				if (TDsubwin(win, 1,
					len, trm->trmy, trm->trmx,
					FTutilwin) == NULL)
				{
					continue;
				}

				/*
				**  Call to TDerase on parent window
				**  above should make this unnecessary.
				**  except when box/trim is present.
				**  For this case we do a TDputattr() call.
				TDerase(FTutilwin);
				**  Otherwise, this is an ordinary
				**  piece of text trim.
				**
				**  (tom) In addition if the trim has attributes
				**  then we must output them in the window
				*/
				if (found_box || trm->trmflags)
				{
					TDputattr(FTutilwin, trm->trmflags);
				}
				_VOID_ TDmvstrwadd(win, trm->trmy, trm->trmx,
					trm->trmstr);
				if (trm->trmflags & fdTRHSPOT)
				{
					IITDhsHotspotString(i, cmd_str);
					IITDhotspot(trm->trmx, trm->trmy,
						len, 1, cmd_str, 1);
					if (loc_support)
					{
					    IIFTrfoRegFormObj(frm, FHOTSPOT,
						trm->trmy, trm->trmx,
						1, len, i, 0, 0);
					}
				}
			}
		}
	}

	j = frm->frfldno;
	frfld = frm->frfld;

        /*
         * brett, New screen windex command string.
         * Note: This is where the sequenced fields of the frame
	 *	are put into windex through termdr.
         */
	if (frm->frmflags & fdISPOPUP)
	{
        	IITDnwscrn(frm->frname, win->_startx, win->_starty, frm->frmaxx,
			frm->frmaxy, frm->frmflags, frm->frm2flags);
	}
	else
	{
        	IITDnwscrn(frm->frname, frm->frposx, frm->frposy, frm->frmaxx,
			frm->frmaxy, frm->frmflags, frm->frm2flags);
	}

	for (i = 0; i < j; i++)
	{
		fld = *frfld++;
		if (fld->fltag == FREGULAR)
		{
                        /* brett, New regular section windex command string */
                        IITDnwrsec(fld->fld_var.flregfld);

			_VOID_ FTregbld(frm, fld->fld_var.flregfld, first, win,
			    fromPrintform);

                        /* brett, End regular section windex command string */
                        IITDedrsec();
		}
		else
		{
                        /* brett, New table section windex command string */
                        IITDnwtbl(fld->fld_var.fltblfld->tfhdr.fhseq,
                                fld->fld_var.fltblfld);

			_VOID_ FTtblbld(frm, fld->fld_var.fltblfld, first, win,
			    fromPrintform);

                        /* brett, End table section windex command string */
                        IITDedtbl();
		}
	}
	/* brett, End screen windex command string */
        IITDedscrn();


	j = frm->frnsno;
	frfld = frm->frnsfld;

	for (i = 0; i < j; i++)
	{
		fld = *frfld++;
		if (fld->fltag == FREGULAR)
		{
			_VOID_ FTregbld(frm, fld->fld_var.flregfld, first, win,
			    fromPrintform);
		}
		else
		{
			_VOID_ FTtblbld(frm, fld->fld_var.fltblfld, first, win,
			    fromPrintform);
		}
	}

	/*
	**  Fix up overlapping of box graphics characters.
	*/
	TDboxfix(win);

	if (usewin == (WINDOW *) NULL)
	{
		FTsetiofrm(frm);
	}

	return(OK);
}

static STATUS
FTregbld(
FRAME	*frm, 
REGFLD	*rfld,
bool	first,
WINDOW	*win,
bool	fromPrintform )
{
	DB_TEXT_STRING	*text;
	FLDHDR		*hdr;
	FLDVAL		*val;
	FLDTYPE		*type;
	u_char		*str;
	SCROLL_DATA	*scroll;
	WINDOW		*twin;

	hdr = &(rfld->flhdr);

	/*
	**  If field is invisible, don't display it.
	**  If, however, from Printform, than display it anyway.
	*/

	if ((hdr->fhdflags & fdINVISIBLE) && !fromPrintform)
	{
		return(TRUE);
	}

	val = &(rfld->flval);
	type = &(rfld->fltype);

	if (TDsubwin(win, hdr->fhmaxy, hdr->fhmaxx, hdr->fhposy, hdr->fhposx,
		FTutilwin) == NULL)
	{
		SIfprintf(stderr, ERget(E_FT0004__rFIELD_INIT___s__s__),
			frm->frname, hdr->fhdname);
		SIflush(stderr);
		return(FALSE);
	}

	/*
	** Register this field.  Coordinates include data and title section,
	** and box if a boxed field.
	*/
	if (loc_support)
	{
	    IIFTrfoRegFormObj(frm, FREGULAR, hdr->fhposy, hdr->fhposx,
		hdr->fhmaxy, hdr->fhmaxx, hdr->fhseq, 0, 0);
	}

	if (!samefrm)
	{
		/*
		**  Clear window for field so no box trim will show thru.
		*/
		if (found_box)
		{
			if (hdr->fhdflags & fdBOXFLD)
			{
				_VOID_ TDsubwin(FTutilwin, FTutilwin->_maxy - 2,
					FTutilwin->_maxx - 2,
					FTutilwin->_begy + 1,
					FTutilwin->_begx + 1, FTcurwin);
				twin = FTcurwin;
			}
			else
			{
				twin = FTutilwin;
			}
			TDerase(twin);
		}

		/*
		**  if boxing flag is set
		*/

		if (hdr->fhdflags & fdBOXFLD)
		{
			TDbox(FTutilwin, '|', '-', '+');
			/* since we are writing box characters we must post in
			   the parent window structure the need to fixup
			   the box characters before refresh */
			win->_flags |= _BOXFIX;
		}

		/*
		**  add title of field to window
		*/

		if (hdr->fhtitle != NULL)
		{
			TDmvstrwadd(FTutilwin, hdr->fhtity, hdr->fhtitx,
				hdr->fhtitle);
		}
	}

	/*
	**  simulate allocation of data window (TDsubwindow of field window)
	*/
        /* brett, Simple field windex command string */
        IITDfield((hdr->fhposx + type->ftdatax),
		(hdr->fhposy + val->fvdatay), type->ftdataln,
		(i4)(type->ftwidth+type->ftdataln-1)/type->ftdataln,
		hdr->fhseq, hdr->fhdflags, hdr->fhd2flags, hdr->fhintrp);
	/* brett, Simple field title windex command string */
	if (hdr->fhtitle != NULL)
		IITDfldtitle((hdr->fhposx + hdr->fhtitx),
			(hdr->fhposy + hdr->fhtity), hdr->fhtitle);

	if (TDsubwin(FTutilwin,
		(type->ftwidth+type->ftdataln-1)/type->ftdataln,
		type->ftdataln, hdr->fhposy+val->fvdatay,
		hdr->fhposx+type->ftdatax, FTcurwin) == NULL)
	{
		SIfprintf(stderr, ERget(E_FT0005__rFIELD_INIT___s__s__),
			frm->frname, hdr->fhdname);
		SIflush(stderr);
		return(FALSE);
	}

	/*
	**  if field in standout
	*/

	if (!samefrm && (hdr->fhdflags & fdDISPATTR))
	{
		TDsetattr(FTcurwin, hdr->fhdflags);
	}

	if (!(hdr->fhdflags & fdNOECHO) && !fromPrintform)
	{
		if ((hdr->fhdflags & fdREVDIR) && (hdr->fhd2flags & fdSCRLFD))
		{
			scroll = (SCROLL_DATA *)val->fvdatawin;
			/*
			** Want to add string from proper point in the
			** display buffer.  For an RL field, this is that
			** point in the buffer such that an LR addition
			** starting there will cover the rest (i.e. the
			** beginning) of the buffer.
			**  for example:
			**  +----------------------------------------+
			**  |            display buffer              |
			**  +----------------------------------------+
			**                        ^
			**
			**                        +------------------+
			**                        |  visible window  |
			**                        +------------------+
			** This is equivalent to the width of the
			** visible window (maxy * maxx) before the end
			** of fvbufr (fvbufr plus total width of scroll
			** buffer).
			**
			** Needed just for reversed scrolling fields
			** because LR fields start at fvbufr's start,
			** and non-scrolling RL fields have an fvbufr
			** width equivalent to the width of the visible
			** window, so the calculation would result in
			** simply pointing to the start of fvbufr.
			*/
			str = val->fvbufr + (i4)(scroll->right - scroll->left)
				- (FTcurwin->_maxy * FTcurwin->_maxx) + 1;
		}
		else
		{
			str = val->fvbufr;
		}
		text = (DB_TEXT_STRING *) val->fvdsdbv->db_data;
		TDxaddstr(FTcurwin, text->db_t_count, str);
	}

	return(TRUE);
}

STATUS 
FTtblbld(frm, tbl, first, win, fromPrintform)
FRAME	*frm;
TBLFLD	*tbl;
bool	first;
WINDOW	*win;
bool	fromPrintform;
{
	DB_TEXT_STRING	*text;
	i4		colcnt;
	i4		tfcols;
	i4		rowcnt;
	i4		tfrows;
	FLDHDR		*hdr;
	FLDTYPE		*type;
	FLDVAL		*val;
	FLDHDR		*chdr;
	FLDCOL		**cols;
	FLDCOL		*col;
	i4		*pflag;
	u_char		*str;
	SCROLL_DATA	*scroll;
	i4		iscurfld = FALSE;
	i4		height;
	i4		cattr;

	hdr = &(tbl->tfhdr);
	if (frm->frcurfld == hdr->fhseq)
	{
		iscurfld = TRUE;
	}

	/*
	**  If table field is invisible, don't display it.
	**  However, if called from Printform, display it anyway.
	*/

	if ((hdr->fhdflags & (fdINVISIBLE | fdTFINVIS)) && !fromPrintform)
	{
		return(TRUE);
	}

	if (TDsubwin(win, hdr->fhmaxy, hdr->fhmaxx, hdr->fhposy, hdr->fhposx,
		FTutilwin) == NULL)
	{
		SIfprintf(stderr, ERget(E_FT0006_TBL_INIT__can_t_alloc),
			hdr->fhdname);
		SIflush(stderr);
		return(FALSE);
	}

	if (!samefrm)
	{
		/*
		**  Clear window for field so no box trim will
		**  show thru.
		*/
		if (found_box)
		{
			_VOID_ TDsubwin(FTutilwin, FTutilwin->_maxy - 2,
				FTutilwin->_maxx - 2, FTutilwin->_begy + 1,
				FTutilwin->_begx + 1, FTcurwin);
			TDerase(FTcurwin);
		}

		/* add title to field */

		FTdisptf(FTutilwin, tbl, hdr);
		/* since we are writing box characters we must post in
		   the parent window structure the need to fixup
		   the box characters before refresh */
		win->_flags |= _BOXFIX;
	}

	if (loc_support)
	{
	    /* Add scrolling hotspots to the table field */
	    /* -- to the entire border trim above the first row. */
	    IIFTrfoRegFormObj(frm, FSCRDN,
		hdr->fhposy + tbl->tfstart - 1, hdr->fhposx,
		1, hdr->fhmaxx, hdr->fhseq, 0, 0);
	    /* -- to the entire border trim below the last row. */
	    IIFTrfoRegFormObj(frm, FSCRUP,
		hdr->fhposy + hdr->fhmaxy - 1, hdr->fhposx,
		1, hdr->fhmaxx, hdr->fhseq, 0, 0);
	}

	/*
	**  simulate allocation of data window (TDsubwindow of field window)
	**  for each column, etc.
	*/

	tfcols = tbl->tfcols;
	tfrows = tbl->tfrows;
	cols = tbl->tfflds;
	for (colcnt = 0; colcnt < tfcols; colcnt++)
	{
		col = *cols++;
		chdr = &(col->flhdr);

		/* Don't instantiate invisible columns. */
		if (chdr->fhdflags & fdINVISIBLE)
		{
			continue;
		}

		type = &(col->fltype);
		height = (type->ftwidth + type->ftdataln - 1)/type->ftdataln;

		val = tbl->tfwins + chdr->fhseq;

		for (rowcnt = 0; rowcnt < tfrows; val += tfcols, rowcnt++)
		{
                        /*
                        ** brett, Simple field windex command string.
                        ** Note: Pass all columns through and let
			** windex worry about modes.
                        */
                        IITDfield((hdr->fhposx + type->ftdatax),
				(hdr->fhposy + val->fvdatay), type->ftdataln,
				height,
				chdr->fhseq, chdr->fhdflags, chdr->fhd2flags,
				chdr->fhintrp);

			/*
			** Register this table field cell.
			*/
			if (loc_support)
			{
			    IIFTrfoRegFormObj(frm, FTABLE,
				hdr->fhposy + val->fvdatay,
				hdr->fhposx + type->ftdatax,
				tbl->tfwidth, chdr->fhmaxx,
				hdr->fhseq, rowcnt, chdr->fhseq);
			}

			if (TDsubwin(FTutilwin,
				height,
				type->ftdataln, hdr->fhposy + val->fvdatay,
				hdr->fhposx + type->ftdatax, FTcurwin) == NULL)
			{
				SIfprintf(stderr,
					ERget(E_FT0007__rFIELD_INIT___s__s__),
					frm->frname, hdr->fhdname);
				SIflush(stderr);
				return(FALSE);
			}
			if (!samefrm && (chdr->fhdflags & fdDISPATTR))
			{
				TDsetattr(FTcurwin, chdr->fhdflags);
			}

			/*
			**  Check for row highlighting and set
			**  to reverse video if on.
			*/
			pflag = tbl->tffflags + (rowcnt * tbl->tfcols) + colcnt;
			if (!first)
			{
				if (*pflag & fdROWHLIT)
				{
				    if (iscurfld && rowcnt == tbl->tfcurrow)
				    {
					TDsetattr(FTcurwin, (i4) fdRVVID);
				    }
				    else
				    {
					*pflag &= ~fdROWHLIT;

					/*
					**  Clear attributes out of
					**  window since TDsetattr
					**  ORs in new attributes
					**  and a previous build could
					**  have set the highlighting.
					**  Call to TDerase on parent
					**  window in FTbuild should
					**  make this unnecessary.  In
					**  the case of being called from
					**  FTtfvis().  There shoud be
					**  nothing there first place.
					*/
					/*
					**  Put back normal column
					**  display attributes if
					**  unhilighting a row.
					*/
					TDputattr(FTcurwin, chdr->fhdflags);
				    }
				}
				else
				{
				    /*
				    **  If hightlighting enabled and current
				    **  row is not hightlighted yet, set it.
				    */
				    if ((tbl->tfhdr.fhdflags & fdROWHLIT) &&
					iscurfld && rowcnt == tbl->tfcurrow)
				    {
					*pflag |= fdROWHLIT;
					TDsetattr(FTcurwin, fdRVVID);
				    }
				}
			}

			/*
			**  Now put in dataset and cell attributes
			**  which are OR'ed into the normal attributes.
			*/
			if (*pflag & (dsALLATTR | tfALLATTR))
			{
				cattr = 0;
				IIFTdcaDecodeCellAttr(*pflag, &cattr);
				if (cattr)
				{
					TDsetattr(FTcurwin, cattr);
				}
			}

			if (!(chdr->fhdflags & fdNOECHO) && !fromPrintform)
			{
				if ((chdr->fhdflags & fdREVDIR)
					&& (chdr->fhd2flags & fdSCRLFD))
				{
					scroll = (SCROLL_DATA *)val->fvdatawin;
					/*
					** Want to add string from proper point
					** in the display buffer.  For an RL
					** field, this is that point in the
					** buffer such that an LR addition
					** starting there will cover the rest
					** (i.e. the beginning) of the buffer.
					**  for example:
					**  +--------------------------------+
					**  |    display buffer              |
					**  +--------------------------------+
					**                ^
					**
					**                +------------------+
					**                |  visible window  |
					**                +------------------+
					** This is equivalent to the width of
					** the visible window (maxy * maxx)
					** before the end of fvbufr (fvbufr plus
					** total width of scroll buffer).
					**
					** Needed just for reversed scrolling
					** fields because LR fields start at
					** fvbufr's start, and non-scrolling RL
					** fields have an fvbufr width
					** equivalent to the width of the
					** visible window, so the calculation
					** would result in simply pointing to
					** the start of fvbufr.
					*/
					str = val->fvbufr +
						(i4)(scroll->right
							- scroll->left)
						- (FTcurwin->_maxy
							* FTcurwin->_maxx) + 1;
				}
				else
				{
					str = val->fvbufr;
				}
				text = (DB_TEXT_STRING *) val->fvdsdbv->db_data;
				TDxaddstr(FTcurwin, text->db_t_count, str);
			}
		}

		if (!samefrm)
		{
			FTdispcol(FTutilwin, tbl, hdr, chdr);
		}
	}
	return(TRUE);
}

VOID
FTdisptf(win, tbl, hdr)
WINDOW		*win;
TBLFLD		*tbl;
reg FLDHDR	*hdr;
{
	reg i4	maxx;
	reg i4	i;

	maxx = hdr->fhmaxx - 1;
	if (hdr->fhdflags & fdNOTFTOP)
	{ /* Draw top-less box */
		TDboxtopless(win, '|', '-', '+');
	}
	else
	{ /* Draw box */
		TDbox(win, '|', '-', '+');
	}

	if ( hdr->fhtitle != NULL && *hdr->fhtitle != EOS )
	{ /* Draw table field title and seperator end intersections */
		TDmvstrwadd(win, hdr->fhtity, hdr->fhtitx, hdr->fhtitle);
		TDleftt(win, 2, 0);
		TDrightt(win, 2, maxx);
	}

	i = tbl->tfstart - 1;
	if ( !(hdr->fhdflags & fdNOTFTITLE) )
	{ /* Draw column title seperator end intersections */
		TDleftt(win, i, 0);
		TDrightt(win, i, maxx);
	}
	if (!(hdr->fhdflags & fdNOTFLINES))
	{ /* Draw horizontal seperators end intersections */
		reg i4	maxy;
		reg i4	width;

		i += (width = tbl->tfwidth + 1);
		maxy = hdr->fhmaxy - 1;
		for (; i < maxy; i += width)
		{
			TDleftt(win, i, 0);
			TDrightt(win, i, maxx);
		}
	}
	if (hdr->fhdflags & fdTFXBOT)
	{ /* Special bottom */
		TDleftt(win, hdr->fhmaxy - 1, 0);
		TDrightt(win, hdr->fhmaxy - 1, hdr->fhmaxx - 1);
	}
}

VOID
FTdispcol(win, tbl, hdr, chdr)
WINDOW		*win;
TBLFLD		*tbl;
reg FLDHDR	*hdr;
reg FLDHDR	*chdr;
{
	i4	maxx;
	i4	maxy;
	i4	width;
	reg i4	i;

	/*
	**  add title to column
	*/

	if ( !(hdr->fhdflags & fdNOTFTITLE) &&
			chdr->fhtitle != NULL && *chdr->fhtitle != EOS )
	{ /* display column title */
		TDmvstrwadd(win, chdr->fhtity, chdr->fhtitx, chdr->fhtitle);
	}

	maxx = chdr->fhposx + chdr->fhmaxx;
	maxy = hdr->fhmaxy - 1;
	width = tbl->tfwidth + 1;
	/* Draw horizontal seperation line */
	for (i = chdr->fhposx; i < maxx; i++)
	{
		reg i4	j;

		if ( hdr->fhtitle != NULL && *hdr->fhtitle != EOS )
		{ /* Table field title seperator */
			TDtfdraw(win, chdr->fhposy, i, '-');
		}
		if ( !(hdr->fhdflags & fdNOTFTITLE) )
		{ /* Draw column title seperator */
			TDtfdraw(win, tbl->tfstart - 1, i, '=');
		}
		if (hdr->fhdflags & fdNOTFLINES)
		{
			continue;
		}
		/* Data seperators */
		for (j = tbl->tfstart + tbl->tfwidth; j < maxy; j += width)
		{
			TDtfdraw(win, j, i, '-');
		}
	}
	if ( !(hdr->fhdflags & fdNOCOLSEP) &&
			(chdr->fhposx+chdr->fhmaxx) != (hdr->fhmaxx - 1) )
	{ /* Draw vertical column lines and intersections */
		maxx = chdr->fhposx + chdr->fhmaxx;
		maxy = hdr->fhmaxy - 1;
		width = tbl->tfwidth + 1;
		if (hdr->fhdflags & fdNOTFTOP)
		{ /* no top */
			TDtfdraw(win, chdr->fhposy, maxx, '|');
		}
		else
		{
			TDtopt(win, chdr->fhposy, maxx);
		}
		for (i = chdr->fhposy + 1; i < maxy; i++)
		{
			TDtfdraw(win, i, maxx, '|');
		}
		if ( !(hdr->fhdflags & fdNOTFLINES) )
		{ /* Draw horizontal seperators intersection */
			if (hdr->fhdflags & fdNOTFTOP)
			{
				i = width - 1;
			}
			else if (hdr->fhdflags & fdNOTFTITLE)
			{
				i = width;
			}
			else
			{
				i = chdr->fhposy + 2;
			}
			for (; i < maxy; i += width)
			{
				TDxlines(win, i, maxx);
			}
		}
		else
		{ /* No data seperator lines */
			if ( !(hdr->fhdflags & fdNOTFTITLE) )
			{ /* Draw title seperator intersection */
				TDxlines(win, chdr->fhposy +
					 ((hdr->fhdflags & fdNOTFTOP) ? 1 : 2),
					maxx
				);
			}
		}
		if (hdr->fhdflags & fdTFXBOT)
		{ /* Special bottom */
			TDxlines(win, maxy, maxx);
		}
		else
		{ /* Normal bottom */
			TDbottomt(win, maxy, maxx);
		}
	}
}




/*{
** Name:	IIFTspfSetPrevFrm - Set static variable FTprevfrm.
**
** Description:
**	All this routine does is to set static variable FTprevfrm
**	to passed in value.  The only value that is passed is
**	NULL at the moement.
**
** Inputs:
**	frm	Frame pointer to set FTprevfrm to.
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
**	10/13/88 (dkh) - Initial version.
*/
VOID
IIFTspfSetPrevFrm(frm)
FRAME	*frm;
{
	FTprevfrm = frm;
}


/*{
** Name:	IIFTacsAllocCoordStruct - Allocate/clear frm->frres2->coords.
**
** Description:
**	When locator support is requested, allocate the requisite structures.
**	If the structures already exist, then clear out the 'coords' array,
**	and place all LOC and IICOORD structs on 'free lists' pointed to by
**	the topmost entries in the coords array.
**
**	Structure is an array of LOC pointers, with entries pointing to the
**	first and last LOCs on each line of the form.
**
** Inputs:
**	frm	Frame for which to set up the structures.
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
**	19-mar-90 (bruceb)	Written.
*/
VOID
IIFTacsAllocCoordStruct(frm)
FRAME	*frm;
{
    LOC		*lastloc;
    IICOORD	*lastcoord;
    LOC		**coords;
    LOC		**freelocs;
    IICOORD	**freecoords;
    i4		i, j;
    LOC		dloc;
    IICOORD	dcoord;
    char	frname[36 + 4 + 1];	/* 'Frm frame_name' */
    i4		maxy;

    maxy = frm->frres2->frmaxy;

    if (frm->frres2->coords)
    {
	/*
	** Structures already allocated.  Clear them out for re-use.
	** Involves placing all LOC structures on one free list, all
	** IICOORD structures on another free list.
	*/
	coords = (LOC **)(frm->frres2->coords);
	freelocs = &(coords[maxy]);
	freecoords = (IICOORD **)&(coords[2 * maxy + 1]);

	/* Point 'last' pointers to end of their free lists. */
	lastloc = *freelocs;
	if (lastloc == NULL)
	{
	    *freelocs = lastloc = &dloc;
	    dloc.next = NULL;
	}
	else
	{
	    while (lastloc->next != NULL)
	    {
		lastloc = lastloc->next;
	    }
	}

	lastcoord = *freecoords;
	if (lastcoord == NULL)
	{
	    *freecoords = lastcoord = &dcoord;
	    dcoord.next = NULL;
	}
	else
	{
	    while (lastcoord->next != NULL)
	    {
		lastcoord = lastcoord->next;
	    }
	}

	for (i = 0, j = maxy + 1; i < maxy; i++, j++)
	{
	    if (coords[i] != NULL)
	    {
		lastloc->next = coords[i];
		while (lastloc->next != NULL)
		{
		    lastloc = lastloc->next;
		    if (lastloc->obj->begy == i)
		    {
			/* Place IICOORDs on free list. */
			lastcoord->next = lastloc->obj;
			lastcoord = lastcoord->next;
		    }
		    lastloc->obj = NULL;
		}
		coords[i] = coords[j] = NULL;
	    }
	}

	if (*freelocs == &dloc)
	{
	    *freelocs = dloc.next;
	}
	lastcoord->next = NULL;
	if (*freecoords == &dcoord)
	{
	    *freecoords = dcoord.next;
	}
    }
    else
    {
	/* Allocate array with extra pointers for use with free lists. */
	if ((frm->frres2->coords = FEreqmem((u_i4)frm->frtag,
	    (u_i4)((2 * (maxy + 1)) * sizeof(LOC *)), TRUE,
	    (STATUS *)NULL)) == NULL)
	{
	    IIUGbmaBadMemoryAllocation(ERx("IIFTacs"));
	}
    }

    STprintf(frname, ERx("\nFrm %s:\n----------\n"), frm->frname);
    IITDptiPrtTraceInfo(frname);
}


/*{
** Name:	IIFTrfoRegFormObj - Register an object for locator support.
**
** Description:
**	When locator support is requested, place the specified object onto
**	the form's locator support structures.  In order to work on old
**	forms with non-sequence fields, this routine will ignore any objects
**	for which the sequence value is < 0.
**
**	An object is added to the end of the list for the rows that object
**	covers.  Since FTbuild calls this routine for hotspot trim before
**	processing fields, hotspot trim will be discovered before (take
**	precedence over) overlapping fields.
**
** Inputs:
**	frm		Frame for which to set up the structures.
**	type		FREGULAR, FTABLE or FHOTSPOT.
**	begy		Coordinates for top, left corner of the object.
**	begx
**	leny		Distance to bottom, right corner of the object.
**	lenx
**	sequence	Field or trim sequence number.
**	tf_row		Table field row and column.
**	tf_col
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
**	19-mar-90 (bruceb)	Written.
*/
VOID
IIFTrfoRegFormObj(frm, type, begy, begx, leny, lenx, sequence, tf_row, tf_col)
FRAME	*frm;
i4	type;
i4	begy;
i4	begx;
i4	leny;
i4	lenx;
i4	sequence;
i4	tf_row;
i4	tf_col;
{
    i4		i, j;
    i4		endy;
    IICOORD	*obj;
    LOC		*loc;
    LOC		*tmp;
    LOC		**list;
    LOC		**freelocs;
    IICOORD	**freecoords;
    char	buf[200];
    i4		maxy;

    if (sequence < 0)
    {
	/*
	** This is a non-sequence (old-style displayonly) field.
	** Such fields can never be reached, so ignore.
	*/
	return;
    }

    maxy = frm->frres2->frmaxy;

    list = (LOC **)(frm->frres2->coords);
    freelocs = &(list[maxy]);
    freecoords = (IICOORD **)&(list[2 * maxy + 1]);
    if (*freecoords != NULL)
    {
	obj = *freecoords;
	*freecoords = obj->next;
    }
    else
    {
	if ((obj = (IICOORD *)FEreqmem((u_i4)frm->frtag,
	    (u_i4)sizeof(IICOORD), FALSE, (STATUS *)NULL)) == NULL)
	{
	    IIUGbmaBadMemoryAllocation(ERx("IIFTrfo"));
	}
    }
    obj->begy = begy;
    obj->begx = begx;
    obj->endy = endy = begy + leny - 1;
    obj->endx = begx + lenx - 1;


    for (i = begy, j = maxy + begy + 1; i <= endy; i++, j++)
    {
	if (*freelocs != NULL)
	{
	    loc = *freelocs;
	    *freelocs = loc->next;
	}
	else
	{
	    if ((loc = (LOC *)FEreqmem((u_i4)frm->frtag,
		(u_i4)sizeof(LOC), FALSE, (STATUS *)NULL)) == NULL)
	    {
		IIUGbmaBadMemoryAllocation(ERx("IIFTrfo"));
	    }
	}
	loc->obj = obj;
	loc->objtype = type;
	loc->sequence = sequence;
	loc->row_number = tf_row;
	loc->col_number = tf_col;
	loc->next = NULL;
	tmp = list[j];
	list[j] = loc;	/* Place obj at end of list. */
	if (tmp != NULL)
	    tmp->next = loc;
	else
	    list[i] = loc;	/* Place also as first on the list. */
    }

    if (type == FREGULAR)
    {
	STprintf(buf, ERx("Fld #%d (%s)"), sequence,
	    frm->frfld[sequence]->fld_var.flregfld->flhdr.fhdname);
    }
    else if (type == FTABLE)
    {
	STprintf(buf, ERx("TFld #%d (%s), row/col (%d/%d)"),
	    sequence, frm->frfld[sequence]->fld_var.fltblfld->tfhdr.fhdname,
	    tf_row, tf_col);
    }
    else if (type == FSCRDN)
    {
	STprintf(buf, ERx("TFld #%d scroll down"), sequence);
    }
    else if (type == FSCRUP)
    {
	STprintf(buf, ERx("TFld #%d scroll up"), sequence);
    }
    else
    {
	STprintf(buf, ERx("Trim #%d"), sequence);
    }

    IITDpciPrtCoordInfo(NULL, buf, obj);
}
