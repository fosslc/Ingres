# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"

/*
**	ftcrtrk.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	ftcrtrk.c - Cursor tracking routines.
**
** Description:
**	Routines to handle cursor tracking for a table field.
**
** History:
**	05/02/87 (dkh) - Initial version.
**	02/02/88 (dkh) - Fixed jup bug 1915.
**	07-jul-88 (bruceb)	Fix for bug 2718
**		In FTscrtrk(), when calling FTsetda() with fdRVVID for
**		hilighting, be sure to 'or' in the current attribute
**		flags to retain those attributes for the row/col cell.
**	10/30/88 (dkh) - Performance changes.
**	05/19/89 (dkh) - Fixed bug 5398.
**	11-sep-89 (bruceb)
**		Handle invisible columns.
**	15-dec-89 (bruceb)	Fix for bug 8807.
**		This fix is already part of 6.4 code.
**	01/04/90 (dkh) - Changed FTrcrtrk() to key off of the last
**			 column that was cleaned up when refreshing
**			 the table field window.  This is necessary
**			 in case we are cleaning up due to an exit
**			 from a table field (which sets tfcurcol to 0)
**			 and the first column is invisible.  This is
**			 here since we can't change exit behavior in FTmove().
**	04/03/90 (dkh) - Integrated MACWS changes.
**	04/14/90 (dkh) - Changed "# if defined" to "# ifdef".
**	07/20/90 (dkh) - Change the update behavior for FTrcrtrk so that
**			 it will not cause unnecessary scrolling when
**			 the cursor moves off of a row highlighted tf.
**	08/12/90 (dkh) - Added support for table field cell highlighting.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/130036.
**			 This supports HP terminals in HP mode.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


FUNC_EXTERN	WINDOW	*FTwmvtab();



/*{
** Name:	FTrcrtrk - Reset cursor tracking.
**
** Description:
**	Routine to reset cursor tracking for a field.
**	No-op if field is not a table field or no
**	cursor tracking for table field is enabled.
**
** Inputs:
**	frm	Pointer to a form.
**	fldno	Field number of field to reset.
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
**	05/02/87 (dkh) - Initial version.
*/
VOID
FTrcrtrk(frm, fldno)
FRAME	*frm;
i4	fldno;
{
	FIELD	*fld;
	TBLFLD	*tbl;
	FLDCOL	*col;
	FLDHDR	*hdr;
	FLDHDR	*thdr;
	i4	i;
	i4	j;
	i4	*pflag;
	WINDOW	*win;
	FLDVAL	*val;
	i4	found = FALSE;
	i4	lastcol;

	fld = frm->frfld[fldno];

	if (fld->fltag != FTABLE)
	{
		return;
	}

	tbl = fld->fld_var.fltblfld;

	thdr = &(tbl->tfhdr);
	if (!(thdr->fhdflags & fdROWHLIT))
	{
		return;
	}
	/* No need to turn off the row highlight on an invisible field. */
	if ((thdr->fhdflags & (fdINVISIBLE | fdTFINVIS)))
	{
		return;
	}

	val = tbl->tfwins + (tbl->tfcurrow * tbl->tfcols);

	for (i = 0; i < tbl->tfrows; i++)
	{
		pflag = tbl->tffflags + (i * tbl->tfcols) + 0;
		if (*pflag & fdROWHLIT)
		{
			found = TRUE;
			for (j = 0; j < tbl->tfcols; j++)
			{
				pflag = tbl->tffflags + (i * tbl->tfcols) + j;
				*pflag &= ~fdROWHLIT;
				col = tbl->tfflds[j];
				hdr = &(col->flhdr);

# ifdef	DATAVIEW
				IIMWemEnableMws(FALSE);
# endif	/* DATAVIEW */

				if (!(hdr->fhdflags & fdINVISIBLE))
				{
				    FTsetda(frm, fldno, FT_UPDATE, FT_TBLFLD,
					    j, i, hdr->fhdflags);
				    lastcol = j;
				}

# ifdef	DATAVIEW
				IIMWemEnableMws(TRUE);
# endif	/* DATAVIEW */
			}
		}
	}

	if (!found)
	{
		return;
	}

	/*
	**  Force entire form image to terminal.  This is a little
	**  expensive but only way to get desired behavior.
	*/
	win = (FTwin->_parent != NULL) ? FTwin->_parent : FTwin;

	TDtouchwin(win);
	TDrefresh(win);
}




/*{
** Name:	FTscrtrk - Set cursor tracking.
**
** Description:
**	Routine to set cursor tracking for a field.
**	No-op if field is not a table field or no
**	cursor tracking for table field is enabled.
**
** Inputs:
**	frm	Pointer to a form.
**	fldno	Field number of field to set.
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
**	05/02/87 (dkh) - Initial version.
*/
VOID
FTscrtrk(frm, fldno)
FRAME	*frm;
i4	fldno;
{
	FIELD	*fld;
	TBLFLD	*tbl;
	FLDCOL	*col;
	FLDHDR	*hdr;
	FLDHDR	*thdr;
	i4	i;
	i4	j;
	i4	*pflag;
	WINDOW	*win;
	WINDOW	*tfwin = NULL;
	i4	currow;
	FLDVAL	*val;
	i4	tfx;
	i4	tfy;
	i4	lines;
	i4	orow = -1;
	i4	nrow;
	i4     zx;             /* work variable for HP terminal stuff */

	fld = frm->frfld[fldno];

	if (fld->fltag != FTABLE)
	{
		return;
	}

	tbl = fld->fld_var.fltblfld;

	thdr = &(tbl->tfhdr);
	if (!(thdr->fhdflags & fdROWHLIT))
	{
		return;
	}

	currow = tbl->tfcurrow;

	for (i = 0; i < tbl->tfrows; i++)
	{
		pflag = tbl->tffflags + (i * tbl->tfcols) + 0;
		if (*pflag & fdROWHLIT)
		{
			/*
			**  If there are attributes set, we need
			**  to be less optimal and force attributes
			**  out again in case the currently highlighted
			**  row has new attributes.
			*/
			if (i != currow || (*pflag & (dsALLATTR | tfALLATTR)))
			{
				for (j = 0; j < tbl->tfcols; j++)
				{
					pflag = tbl->tffflags +
						(i * tbl->tfcols) + j;
					*pflag &= ~fdROWHLIT;
					col = tbl->tfflds[j];
					hdr = &(col->flhdr);

# ifdef	DATAVIEW
					IIMWemEnableMws(FALSE);
# endif	/* DATAVIEW */

					if (!(hdr->fhdflags & fdINVISIBLE))
					{
					    FTsetda(frm, fldno, FT_UPDATE,
						FT_TBLFLD, j, i, hdr->fhdflags);
					}

# ifdef	DATAVIEW
					IIMWemEnableMws(TRUE);
# endif	/* DATAVIEW */
				}
				
				/*
				**  Only touch the row that is being unset.
				*/
				val = tbl->tfwins + (i * tbl->tfcols);
				orow = val->fvdatay;
			}
			else
			{
				/*
				**  Current row already highlighted.
				**  Don't need to do anything.
				*/
				return;
			}
		}
	}

	for (j = 0; j < tbl->tfcols; j++)
	{
		pflag = tbl->tffflags + (currow * tbl->tfcols) + j;
		col = tbl->tfflds[j];
		hdr = &(col->flhdr);

# ifdef	DATAVIEW
		IIMWemEnableMws(FALSE);
# endif	/* DATAVIEW */

		if (!(hdr->fhdflags & fdINVISIBLE))
		{
		    FTsetda(frm, fldno, FT_UPDATE, FT_TBLFLD, j, currow,
			    (i4) (fdRVVID | hdr->fhdflags));
		}

# ifdef	DATAVIEW
		IIMWemEnableMws(TRUE);
# endif	/* DATAVIEW */

		*pflag |= fdROWHLIT;
	}

	/*
	**  Only touch the row that is being set.
	*/
	val = tbl->tfwins + (currow * tbl->tfcols);
	win = FTwmvtab(tbl, tbl->tfcurcol, val);
	tfx = win->_begx;
	tfy = win->_begy;
	nrow = val->fvdatay;

	if (tfwin == NULL)
	{
		/*
		** zx becomes the maxx value for the sub window used
		** for cursor tracking. IF we are running on an HP
		** terminal in HP mode (XS), we use the right edge
		** of the screen for this value; Otherwise,
		** we use the maxx value of the table field.
		*/
		if (XS)
		{
			zx = FTwin->_maxx - thdr->fhposx;
		}
		else
		{
			zx = thdr->fhmaxx;
		}
		tfwin = TDsubwin(FTwin, thdr->fhmaxy, zx,
			thdr->fhposy, thdr->fhposx, FTutilwin);
	}

	if ((lines = tbl->tfwidth) == 1)
	{
		/*
		**  Need to do this test since we may not have any
		**  highlighting set we this routine is called.
		*/
		if (orow >= 0)
		{
			tfwin->_firstch[orow] = 0;
			tfwin->_lastch[orow] = tfwin->_maxx - 1;
		}
		tfwin->_firstch[nrow] = 0;
		tfwin->_lastch[nrow] = tfwin->_maxx - 1;
	}
	else
	{
		for (i = 0; i < lines; i++)
		{
			/*
			**  Need to do this test since we may not have any
			**  highlighting set we this routine is called.
			*/
			if (orow >= 0)
			{
				tfwin->_firstch[orow] = 0;
				tfwin->_lastch[orow++] = tfwin->_maxx - 1;
			}
			tfwin->_firstch[nrow] = 0;
			tfwin->_lastch[nrow++] = tfwin->_maxx - 1;
		}
	}
	TDmove(tfwin, tfy - tfwin->_begy, tfx - tfwin->_begx);

	TDrefresh(tfwin);
}
