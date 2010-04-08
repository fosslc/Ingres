/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"

/*
** movetab.c
**
** contains all routines which move the cursor around a table field.
**
** DEFINES
**	FTmovetab(tbl)
**		TBLFLD	*tbl;
**
**	FTrightcol(tbl)
**		TBLFLD  *tbl;
**
**	FTscrdown(tbl)
**	FTdownrow(tbl)
**	FTuprow(tbl)
**
** HISTORY:
**
** Additional code added to handle skipping display only columns.
** Only FTrightcol is modified because FTleftcol is no longer called
** by any one (dkh)
** Added optional Query mode traversal (ncg)
** Added Query Only column skipping (ncg)
**
**  7/17/85 - Updated to handle noecho fields. (dkh)
**  8/19/85 - Added support for sideway scrolling in a field. (dkh)
**	03/03/87 (dkh) - Added support for ADTs.
**	16-mar-87 (bruceb)
**		FTscrrng now places the cursor at the appropriate 'beginning'
**		of the window depending on whether the field is standard or
**		right-to-left.
**	05/02/87 (dkh) - Changed to call FD routines via passed pointers.
**	06/19/87 (dkh) - Code cleanup.
**	11/04/87 (dkh) - Changed screen recovery after error occurs.
**	11/28/87 (dkh) - Added screen recovery for range mode also.
**	12/23/87 (dkh) - Performance changes.
**	29-jan-88 (bruceb)
**		Minimally, temporary fix for 6.0 bug 1903.
**		Don't turn off fdI_CHG flag after testing for existence.
**	05/11/88 (dkh) - Added hooks for floating popups.
**	05/26/88 (dkh) - Fixed redisplay problem with doing uparrows
**			 in a table field.  This existed in 5.0 as well.
**	20-dec-88 (bruceb)
**		Handle readonly columns.
**	13-jul-89 (bruceb)
**		New parameter on FDvalidate calls; used to indicate need
**		for an FTbotrst even though the field itself was OK.
**	21-aug-89 (bruceb)
**		FTdownrow now refreshes the row as FTuprow does.
**	08-sep-89 (bruceb)
**		Handle invisible columns.
**	14-dec-89 (bruceb)
**		Handle invisible columns even better.  Change is that
**		FTwmvtab returns NULL if invisible column; needed because
**		fdI_CHG may be on even for invisible columns.
**	09-jan-90 (bruceb)
**		When table field scroll occurs, don't display derived
**		columns before the scroll--prevent visual flicker.
**	01/11/90 (dkh) - Changed FTrightcol() to move to first visible
**			 column instead of always column zero.
**	04/04/90 (dkh) - Integrated MACWS changes.
**	07/10/90 (dkh) - Integrated more MACWS changes.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**	04/11/91 (dkh) - Fixed bug 36415.
**	08/20/93 (dkh) - Fixing NT compile problems.  Changed _leave
**			 to lvcursor for the WINDOW structure.
**	28-Mar-2005 (lakvi01)
**	    Corrected function prototypes.
*/

# define	RESET	-1

bool	FTrngcval();
bool	FTrngrval();
i4	FTcoltrv();
VOID	IIFTrwRefreshWin();

GLOBALREF	i4	FTfldno;
GLOBALREF	FRAME	*FTfrm;
GLOBALREF	bool	FTqbf;

FUNC_EXTERN	WINDOW	*FTgetwin();
FUNC_EXTERN	VOID	FTbotsav();
FUNC_EXTERN	VOID	FTbotrst();
FUNC_EXTERN	VOID	IIFTsiiSetItemInfo();
FUNC_EXTERN	WINDOW	*TDsubwin();

static i4	FTscrup();
static i4	FTscrdown();
static bool	can_refresh = TRUE;
static bool	do_refresh = FALSE;


/*
**  Special routine to associate a window with a
**  specific row and column in a table field.
**  Returns NULL if there is no window to associate--
**  such as if the column is invisible.
*/


WINDOW *
FTwmvtab(tbl, curcol, val)
TBLFLD	*tbl;
i4	curcol;
FLDVAL	*val;
{
	FLDHDR	*hdr;
	FLDTYPE	*type;
	FLDHDR	*chdr;

	hdr = &(tbl->tfhdr);
	type = &(tbl->tfflds[curcol]->fltype);
	chdr = &(tbl->tfflds[curcol]->flhdr);
	if (chdr->fhdflags & fdINVISIBLE)
	{
		return(NULL);
	}
	else
	{
		return(TDsubwin(FTwin,
			(type->ftwidth + type->ftdataln - 1)/type->ftdataln,
			type->ftdataln, hdr->fhposy + val->fvdatay,
			hdr->fhposx + type->ftdatax, FTutilwin));
	}
}



/*
** FTRIGHTCOL
**
** Move the current column right.  Called when user hits RETURN or NROW.
*/

i4
FTrightcol(tbl, repts)
reg	TBLFLD	*tbl;
i4	repts;
{
    reg	i4	ocurcol;
    FLDCOL	*col;
    reg	FLDHDR	*hdr;
    reg	FLDVAL	*val;
    i4		code;
    i4		qry;
    bool	rngmd = FALSE;
    bool	val_error;
    i4		retval;
    i4		i;
    i4		original_row;
    i4		original_col;
    i4		new_row;

    /*
    ** Make sure column is valid, before continuing
    */

    if (FTfrm->frmflags & fdRNGMD)
    {
	rngmd = TRUE;
    }
    if (tbl->tfcurcol < 0 || tbl->tfcurcol >= tbl->tfcols)
    {
	return(fdNOCODE);
    }
    col = tbl->tfflds[tbl->tfcurcol];
    val = tbl->tfwins + (tbl->tfcurrow * tbl->tfcols) + col->flhdr.fhseq;

    original_row = tbl->tfcurrow;
    original_col = tbl->tfcurcol;

# ifdef DATAVIEW
    if (IIMWgvGetVal(FTfrm, tbl) != OK)
	return(fdNOCODE);
# endif	/* DATAVIEW */

    hdr = &col->flhdr;
    FTfld_str(hdr, val, FTwmvtab(tbl, tbl->tfcurcol, val));

    /*
    **  If we are getting data from a table field and the
    **  table field is in READ ONLY mode then don't
    **  bother to run user validation on the data.
    **  Bad data should only get in if its been loaded
    **  by the program.  Part of fix to BUG 5199. (dkh)
    */

    FTbotsav();
    if (rngmd)
    {
	if (!FTrngcval(FTfrm, FTfldno, tbl,
	    FTwmvtab(tbl, tbl->tfcurcol, val)))
	{
	    FTbotrst();
	    return(fdNOCODE);
	}
    }

    if (!rngmd && !FTqbf && !(tbl->tfhdr.fhdflags & fdtfREADONLY))
    {
	retval = (*FTvalfld)(FTfrm, FTfldno, FT_TBLFLD, tbl->tfcurcol,
	    tbl->tfcurrow, &val_error);

	/*
	** Derivation source field, so ensure derived values are displayed.
	*/
	if (hdr->fhdrv && hdr->fhdrv->deplist)
	{
	    do_refresh = TRUE;
	}

	if (retval == FALSE)
	{
	    IIFTrwRefreshWin(TRUE);
	    FTbotrst();
	    return(fdNOCODE);
	}
    }

    /*
    ** If there is only 1 column (readonly or not) then move right
    ** as normal.  Calculate the number of rows or columns to move
    ** to based on the array format of the table field.
    */

    if (tbl->tfcols == 1)
    {
	tbl->tfcurrow += repts;
	/* tfcurcol is unchanged at 0 */
    }
    else
    {
	/*
	** Skip display only columns.  Save the original column number in case
 	** there is only one or zero regular columns.  Search until returning
	** to the original column or finding another regular column.
	*/
	ocurcol = tbl->tfcurcol;
	tbl->tfcurcol++;

	/*
	**  Query Only fields may be reached in Query
	**  and Fill modes.
	*/

	qry = tbl->tfhdr.fhdflags & fdtfQUERY
	    || tbl->tfhdr.fhdflags & fdtfAPPEND;

	while (tbl->tfcurcol != ocurcol)
	{
		/*
		** Pass the last column, update to next row.
		*/
		if (tbl->tfcurcol >= tbl->tfcols)
		{
			tbl->tfcurcol = 0;
			tbl->tfcurrow++;
			continue;
		}
		col = tbl->tfflds[tbl->tfcurcol];
		hdr = &col->flhdr;

		/*
		** Current column is display/query only, skip it and continue.
		*/
		if ((hdr->fhd2flags & fdREADONLY)
		    || (hdr->fhdflags & fdINVISIBLE)
		    || (hdr->fhdflags & fdtfCOLREAD)
		    || (!qry && hdr->fhdflags & fdQUERYONLY))
		{
			tbl->tfcurcol++;
		}
		else
		{
			/*
			** Found a regular column, exit search.
			*/
			break;
		}
	}

	/*
	** If we are back at the original column, and the original column
	** is read/query only, then all columns are read/query only.  So return
	** to first column.
	*/
	if (tbl->tfcurcol == ocurcol)
	{
		col = tbl->tfflds[tbl->tfcurcol];
		hdr = &col->flhdr;
		if ((hdr->fhd2flags & fdREADONLY)
		    || (hdr->fhdflags & fdINVISIBLE)
		    || (hdr->fhdflags & fdtfCOLREAD)
		    || (!qry && hdr->fhdflags & fdQUERYONLY))
		{
			/*
			**  Can't assume column 0 is the right one since
			**  it may be invisible.
			*/
			for (i = 0; i < tbl->tfcols; i++)
			{
				col = tbl->tfflds[i];
				hdr = &col->flhdr;
				if (!(hdr->fhdflags & fdINVISIBLE))
				{
					tbl->tfcurcol = i;
					break;
				}
			}
		}
	}
    }
    /*
    **  If we have moved to a new row, then need to check entire row
    **  before moving to new row
    */
    if (original_row != tbl->tfcurrow && !rngmd && !FTqbf &&
	!(tbl->tfhdr.fhdflags & fdtfREADONLY))
    {
	new_row = tbl->tfcurrow;
	tbl->tfcurrow = original_row;
	if (!FTrowtrv(tbl, original_row, TRUE))
	{
		/*
		**  Note that column has been set by
		**  FTrowtrv() if it finds an error.
		*/
		tbl->tfcurrow = original_row;
		IIFTrwRefreshWin(TRUE);
		FTbotrst();
		return(fdNOCODE);
	}
	tbl->tfcurrow = new_row;
    }
    if (rngmd)
    {
	if (tbl->tfcurrow >= tbl->tfrows)
	{
		/*
		**  Copy up data for one row only.
		*/

		/*
		**  Call to FTwtrv has the effect of saving
		**  the window chars into fvbufr for us.
		*/

		if (FTwtrv(tbl, FDALLROW))
		{
			/*
			**  Special mode not available in BROWSE
			**  mode, so we can assume that we only
			**  increment by 1 row at most.
			*/
			tbl->tfcurrow = tbl->tfrows - 1;
			FTrgscrup(FTfrm, FTfldno, tbl, 1);
		}
	}
    }
    else
    {
	if (tbl->tfcurrow > tbl->tflastrow)
	{
	    /*
	    ** Before the TDscroll get all the data
	    */
	    IIFTrwRefreshWin(FALSE);
	    if (FTtbltrv(tbl, FDDISPROW))
	    {
	    	if ((code = FTscrup(tbl)) != fdNOCODE)
		{
		    /* TF scroll, so refresh will be handled elsewhere. */
		    IIFTrwRefreshWin(RESET);
		    return(code);
		}
	    	tbl->tfcurrow = tbl->tfrows - 1;
	    }
	}
	/* Refresh FTwin now if updated derivation source columns. */
	IIFTrwRefreshWin(TRUE);
    }
    return (fdNOCODE);
}

STATUS
FTtbltrv(tbl, top)
TBLFLD	*tbl;
i4	top;
{
	return(FTtrv(tbl, top, TRUE));
}

STATUS
FTwtrv(tbl, top)
TBLFLD	*tbl;
i4	top;
{
	return(FTtrv(tbl, top, FALSE));
}

STATUS
FTtrv(tbl, top, validate)
TBLFLD	*tbl;
i4	top;
bool	validate;
{
	i4	i;
	i4	oldrow;
	i4	lastrow = 1;

	oldrow = tbl->tfcurrow;
	switch(top)
	{
		case FDALLROW:
			lastrow = tbl->tfrows;
			break;

		case FDDISPROW:
			lastrow = tbl->tflastrow + 1;
			break;

		case FDTOPROW:
			lastrow = 1;
			break;
	}

# ifdef DATAVIEW
	_VOID_ IIMWbqtBegQueryTbl();
# endif /* DATAVIEW */

	for (i = 0; i < lastrow; i++)
	{
		tbl->tfcurrow = i;
		if (!FTrowtrv(tbl, i, validate))
		{

# ifdef DATAVIEW
			_VOID_ IIMWeqtEndQueryTbl();
# endif /* DATAVIEW */

			return(FALSE);
		}
	}
	tbl->tfcurrow = oldrow;

# ifdef DATAVIEW
	_VOID_ IIMWeqtEndQueryTbl();
# endif /* DATAVIEW */

	return(TRUE);
}

STATUS
FTrowtrv(tbl, rownum, validate)
TBLFLD	*tbl;
i4	rownum;
bool	validate;
{
	i4	j;
	FLDHDR	*hdr;
	FLDVAL	*val;
	FLDCOL	**col;
	i4	oldcol;
	bool	rngmd = FALSE;
	i4	*bflag;
	i4	*flag;
	bool	val_error;
	i4	retval;

	if (FTfrm->frmflags & fdRNGMD)
	{
		rngmd = TRUE;
	}
	FTbotsav();
	oldcol = tbl->tfcurcol;
	val = tbl->tfwins + (tbl->tfcurrow * tbl->tfcols);
	col = tbl->tfflds;
	bflag = tbl->tffflags + (tbl->tfcurrow * tbl->tfcols);
	for (j = 0; j < tbl->tfcols; j++, val++, col++)
	{
		tbl->tfcurcol = j;
		flag = bflag + j;
		hdr = &(*col)->flhdr;
		if (*flag & fdI_CHG)
		{

# ifdef DATAVIEW
			if (IIMWgvGetVal(FTfrm, tbl) == FAIL)
				return(FALSE);
# endif	/* DATAVIEW */

			FTfld_str(hdr, val, FTwmvtab(tbl, j, val));
			/* *flag &= ~fdI_CHG;  LESS OPTIMIZATION FOR NOW */
		}

		/*
		**  If we are getting data from a table field and the
		**  table field is in READ ONLY mode then don't
		**  bother to run user validation on the data.
		**  Bad data should only get in if its been loaded
		**  by the program.  Part of fix to BUG 5199. (dkh)
		*/

		if (rngmd && validate)
		{
			if (!FTrngrval(FTfrm, FTfldno, tbl, tbl->tfcurrow))
			{
				FTbotrst();
				return(FALSE);
			}
		}
		if (!rngmd && !FTqbf && !(tbl->tfhdr.fhdflags & fdtfREADONLY)
			&& !(hdr->fhd2flags & fdDERIVED))
		{
		    retval = (*FTvalfld)(FTfrm, FTfldno, FT_TBLFLD,
			tbl->tfcurcol, tbl->tfcurrow, &val_error);

		    /*
		    ** Derivation source field, so ensure derived values are
		    ** displayed.
		    */
		    if (hdr->fhdrv)
		    {
			if (can_refresh)
			    TDrefresh(FTwin);
			else
			    do_refresh = TRUE;
		    }

		    if (retval == FALSE)
		    {
			FTbotrst();
			return(FALSE);
		    }
		}
	}
	tbl->tfcurcol = oldcol;
	return(TRUE);
}

/*
** FTSCRUP
**
** TDscroll the table field up
*/
static i4
FTscrup(tbl)
reg TBLFLD	*tbl;
{
    if (tbl->tfscrup != fdNOCODE)
    {
	tbl->tfstate = tfSCRUP;
    	return tbl->tfscrup;
    }
    return fdNOCODE;
}

/*
** FTSCRDOWN
**
** TDscroll the table field down
*/
static i4
FTscrdown(tbl)
reg TBLFLD	*tbl;
{
    if (tbl->tfscrdown != fdNOCODE)
    {
	tbl->tfstate = tfSCRDOWN;
   	return tbl->tfscrdown;
    }
    return fdNOCODE;
}

/*
** FTDOWNROW
**
** move down a row in the table field
*/

i4
FTdownrow(tbl, repts)
reg TBLFLD	*tbl;
i4		repts;
{
	i4	code;
	bool	rngmd;
	FLDHDR	*hdr;
	WINDOW	*win;

    if (FTfrm->frmflags & fdRNGMD)
    {
	rngmd = TRUE;
    }
    else
    {
	rngmd = FALSE;
	IIFTrwRefreshWin(FALSE);
    }

    /*
    ** Make sure row is valid, before continuing
    */
    if (!FTrowtrv(tbl, tbl->tfcurrow, TRUE))
    {
	if (!rngmd)
	    IIFTrwRefreshWin(TRUE);
	return (fdNOCODE);
    }
    tbl->tfcurrow += repts;

    /*
    **  Redisplay the row if we are not in range mode and
    **  we are still visible.  This is because no one
    **  else will force a redisplay.  We do the redisplay
    **  here so that the whole row appears at once instead
    **  of each column by itself.
    */
    if (!rngmd && tbl->tfcurrow <= tbl->tflastrow)
    {
	/*
	** If do_refresh, then IIFTrwRefreshWin(TRUE) at bottom of this
	** routine will handle the requisite redisplay.
	*/
	if (!do_refresh)
	{
	    hdr = &(tbl->tfhdr);
	    win = TDsubwin(FTwin, hdr->fhmaxy, hdr->fhmaxx, hdr->fhposy,
		hdr->fhposx, FTutilwin);
	    TDtouchwin(win);
	    win->lvcursor = TRUE;
	    TDrefresh(win);
	    win->lvcursor = FALSE;
	}
    }

    if (rngmd)
    {
	/*
	**  Copy up data if necessary.
	*/
	if (tbl->tfcurrow >= tbl->tfrows)
	{
		/*
		**  Calling FTwtrv has the desirable effect of
		**  saving window chars into fvbufr for us.
		*/

		if (FTwtrv(tbl, FDALLROW))
		{
			tbl->tfcurrow -= repts;
			FTrgscrup(FTfrm, FTfldno, tbl, repts);
		}

		/*
		**  Call FTtbltrv again to make sure we save the
		**  new scrolled row in fvbufr.
		*/
		FTwtrv(tbl, FDALLROW);
	}
    }
    else
    {
	if (tbl->tfcurrow > tbl->tflastrow)
	{
	    /*
	    ** Before the TDscroll get all the data
	    */
	    IIFTrwRefreshWin(FALSE);
	    if (FTtbltrv(tbl, FDDISPROW))
	    {
	    	if ((code = FTscrup(tbl)) != fdNOCODE)
		{
		    /*
		    ** Refresh FTwin at start of FTbrowse/FTinsert if TF has
		    ** derivation source columns.
		    */
		    IIFTrwRefreshWin(RESET);
		    return(code);
		}
	    	tbl->tfcurrow = tbl->tfrows - 1;
	    }
	}
	/* Refresh FTwin now if updated derivation source columns. */
	IIFTrwRefreshWin(TRUE);
    }
    return fdNOCODE;
}

/*
** FTUPROW
**
** move up a row in the table field
*/

i4
FTuprow(tbl, repts)
reg TBLFLD	*tbl;
i4		repts;
{
	i4	code;
	bool	rngmd = FALSE;
	FLDHDR	*hdr;
	WINDOW	*win;

    /*
    ** Make sure row is valid, before continuing
    */
    if (!FTrowtrv(tbl, tbl->tfcurrow, TRUE))
    {
	return (fdNOCODE);
    }
    tbl->tfcurrow -= repts;

    if (FTfrm->frmflags & fdRNGMD)
    {
	rngmd = TRUE;
    }

    /*
    **  Redisplay the row if we are not in range mode and
    **  we are still visible.  This is because no one
    **  else will force a redisplay.  We do the redisplay
    **  here so that the whole row appears at once instead
    **  of each column by itself.
    */
    if (!rngmd && tbl->tfcurrow >= 0)
    {
	hdr = &(tbl->tfhdr);
	win = TDsubwin(FTwin, hdr->fhmaxy, hdr->fhmaxx, hdr->fhposy,
	    hdr->fhposx, FTutilwin);
	TDtouchwin(win);
	win->lvcursor = TRUE;
	TDrefresh(win);
	win->lvcursor = FALSE;
    }

    if (tbl->tfcurrow < 0)
    {
	if (rngmd)
	{
		/*
		**  Copy down data.
		*/

		/*
		**  Calling FTtbltrv has the desirable effect of saving
		**  window chars into fvbufr for us.
		*/
		if (FTwtrv(tbl, FDALLROW))
		{
			FTrngdcp(FTfrm, FTfldno, tbl, repts);
			tbl->tfcurrow = 0;
		}

		/*
		**  Call FTtbltrv again to make sure we save the
		**  new scrolled row in fvbufr.
		*/
		FTwtrv(tbl, FDALLROW);
	}
	else
	{
	    /*
	    ** Before the TDscroll get all the data
	    */
	    if (FTtbltrv(tbl, FDDISPROW))
	    {
	    	if ((code = FTscrdown(tbl)) != fdNOCODE)
    			return(code);
	    	tbl->tfcurrow = 0;
	    }
	}
    }
    return fdNOCODE;
}


/*
** scroll a table field in range mode.
*/
VOID
FTscrrng(frm, fldno, code, repts, tbl, win)
FRAME	*frm;
i4	fldno;
i4	code;
i4	repts;				/* number of rows to scroll */
TBLFLD	*tbl;
WINDOW	**win;
{
	i4	lfldno;
	FLDHDR	*hdr;

	if (code == fdopDOWN)
	{
		FTrgscrup(frm, fldno, tbl, repts);
	}
	else if (code == fdopUP)
	{
		tbl->tfcurrow -= repts;
		FTrngdcp(frm, fldno, tbl, 0 - tbl->tfcurrow);
		tbl->tfcurrow = 0;
	}
	FTwtrv(tbl, FDALLROW);	/* validates all rows */
	lfldno = fldno;
        *win = FTgetwin(frm, &lfldno);

	IIFTsiiSetItemInfo((*win)->_begx + FTwin->_startx + 1,
	    (*win)->_begy + FTwin->_starty + 1, (*win)->_maxx, (*win)->_maxy);

	hdr = (*FTgethdr)(frm->frfld[lfldno]);
	if (hdr->fhdflags & fdREVDIR)
	        TDmove(*win, (i4)0, (*win)->_maxx - 1);
	else
        	TDmove(*win, (i4)0, (i4)0);
        TDrefresh(*win);
}


/*{
** Name:	IIFTrwRefreshWin - Refresh FTwin now, or request for later.
**
** Description:
**	Either request that TDrefresh of FTwin occur later (should it in fact
**	be required), indicate that refreshes are again acceptable, or
**	actually perform the refresh if required.  This is used to prevent
**	derived values from being displayed in a table field just before the
**	table field is scrolled, thus preventing visual flicker.
**
** Inputs:
**	now	Indicate nature of the request.
**		FALSE	Don't call TDrefresh.
**		TRUE	Call TDrefresh if do_refresh has been set.
**		RESET	Re-enable refreshes, but don't refresh now.
**
** Outputs:
**	None.
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	01/09/90 (bruceb) - Initial version.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
VOID
IIFTrwRefreshWin(now)
i4	now;
{
    if (now == FALSE)
    {
	can_refresh = FALSE;
    }
    else	/* now == TRUE or now == RESET. */
    {
	if ((now == TRUE) && do_refresh)
	{
	    TDrefresh(FTwin);
	}
	can_refresh = TRUE;
	do_refresh = FALSE;
    }
}
