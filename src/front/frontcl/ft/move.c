/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	"ftfuncs.h"
# include	<frsctblk.h>
# include	<te.h>
# ifdef		SCROLLBARS
# include	<wn.h>
# endif

/**
** Name:	move.c  -  The MOVE Actions of the Frame Driver
**
** Description:
**	This file defines:
**	FTmove		- Used to move the cursor to a new location.
**
** History:
**	mm/dd/yy (RTI) -- created for 5.0.
**	10/13/86 (KY)  -- Changed CH.h to CM.h.
**	24-apr-87 (bruceb)
**		When appropriate, handle left/right movement
**		in reversed fields.
**	05/02/87 (dkh) - Changed to call FD routines via passed pointers.
**	05/02/87 (dkh) - Added support for tf cursor tracking.
**	25-jun-87 (bruceb)	Code cleanup.
**	02/27/88 (dkh) - Added support for nextitem command.
**	04/06/88 (dkh) - Refined support for nextitem command.
**	04/15/88 (dkh) - More tweeking for the nextitem command.
**	05/11/88 (dkh) - Added hooks for floating popups.
**	19-dec-88 (bruceb)
**		Handle movement given existence of readonly and invisible
**		fields.
**	28-feb-89 (bruceb)
**		Changed file and routine headers.
**		Added support for entry activation.  Sets the frm->frres2
**		row and column values if returning up with a scroll
**		command value.  This is because if the scroll fails, still
**		may have been moved by the FTmove code, so still want to
**		trigger an EA.  Note that tfcurrow value should never be
**		out of bounds BEFORE the scroll is attempted.
**	08-sep-89 (bruceb)
**		Handle invisible columns.
**	11/24/89 (dkh) - Added support for goto's from wview.
**	12/27/89 (dkh) - Moved IIFTgcGotoChk() to frame!fdmove.c.
**	01/04/90 (dkh) - Added checks to move to first visible column
**			 when entering a table field.
**	04/14/90 (dkh) - Changed "# if defined" to "# ifdef".
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**	26-may-92 (rogerf) Added scrollbar processing inside SCROLLBARS ifdef.
**	05/30/92 (dkh) - Added support for moving to fields above/below
**			 with up/down arrow keys.
**	21-mar-93 (fraser)
**		Incorporated changes relating to scrollbar handling, including
**		new parameter for IIFTdsrDSRecords.
**	04/23/94 (dkh) - Changed code to only allow field exit via up/down
**			 arrow keys if the user requests it.  This is for
**			 backwards compatibility so existing applications
**			 won't break.
**	23-may-96 (chech02)
**		Fixed compiler complaints for windows 3.1 port.
**  11/05/99 (kitch01) 
**     Bug 48278. If CTRL+N (fdopNROW), is requested search for the first
**     visible tablefield column to prevent trying to land on an invisible
**     column. Change to FTmove().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

FUNC_EXTERN	i4	TDrefresh();
FUNC_EXTERN	VOID	FTrcrtrk();
FUNC_EXTERN	VOID	FTscrtrk();
FUNC_EXTERN	WINDOW	*FTgetwin();
FUNC_EXTERN	VOID	IIFTsiiSetItemInfo();
FUNC_EXTERN	i4	FTrightcol();
FUNC_EXTERN	i4	FTuprow();
FUNC_EXTERN	i4	FTdownrow();

GLOBALREF	FRS_AVCB	*IIFTavcb;
GLOBALREF	FRS_EVCB	*IIFTevcb;
GLOBALREF	FRS_GLCB	*IIFTglcb;


#ifdef SCROLLBARS
GLOBALREF	VOID	(*IIFTdsrDSRecords)();
FUNC_EXTERN	VOID	FTWNRedraw();
#endif

# define	ABOVE	(i4) -1
# define	BELOW	(i4) 1


/*{
** Name:	IIFTgcGotoChk - Can we land on desired column in table field.
**
** Description:
**	This routine determines if we can land on the desired column
**	that is set in tbl->tfcurcol.
**
**	A table field column is landable if it is not readonly,
**	invisible or queryonly.  The latter is OK if the form is
**	in query mode.  In addition, if the column is visible and
**	is the FIRST (leftmost) column of a table field that has
**	NO landable columns, then we allow user to land on the
**	first column.
**
** Inputs:
**	fd	The table field in question.
**	tqry	Are we in query mode.
**
** Outputs:
**
**	Returns:
**		TRUE	If we can land on desired column.
**		FALSE	If we can't land on desired column.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	11/30/89 (dkh) - Initial version.
*/
i4
IIFTxgcGotoChk(frm, evcb)
FRAME		*frm;
FRS_EVCB	*evcb;
{
	FLDHDR	*hdr;
	TBLFLD	*tbl;
	i4	i;
	i4	firstcol = -1;
	i4	can_land = TRUE;
	i4	tqry;
	FIELD	*fd;
	i4	qrymd;
	i4	origcol;
	i4	origrow;

	/*
	**  FTinsert() and FTbrowse() have already checked to
	**  see if destination is different.  NO need to check again.
	*/
	fd = frm->frfld[evcb->gotofld];
	if (fd->fltag == FREGULAR)
	{
		hdr = (*FTgethdr)(fd);
		qrymd = (FTcurdisp() == fdmdQRY ||
			FTcurdisp() == fdmdADD);
		if ((hdr->fhdflags & fdINVISIBLE) ||
			(hdr->fhd2flags & fdREADONLY) ||
			(!qrymd && hdr->fhdflags & fdQUERYONLY))
		{
			/*
			**  Field is currently not reachable.
			*/
			return(FALSE);
		}
		return(TRUE);
	}
	else
	{
		/*
		**  Dealing with a table field as the destination.
		*/
		tbl = fd->fld_var.fltblfld;
		hdr = &(tbl->tfhdr);
		if (hdr->fhdflags & (fdINVISIBLE | fdTFINVIS))
		{
			return(FALSE);
		}

		/*
		**  First check for valid rows.  Note that in
		**  QBF range mode, one can land on any visible
		**  row.
		*/
		if ((frm->frmflags & fdRNGMD &&
			evcb->gotorow > tbl->tfrows) ||
			evcb->gotorow > tbl->tflastrow)
		{
			/*
			**  User selected invalid row.
			*/
			return(FALSE);
		}

		/*
		**  Now check for valid column.
		*/
		tqry = tbl->tfhdr.fhdflags & fdtfQUERY ||
			tbl->tfhdr.fhdflags & fdtfAPPEND;
		origcol = tbl->tfcurcol;
		origrow = tbl->tfcurrow;
		tbl->tfcurcol = evcb->gotocol;
		tbl->tfcurrow = evcb->gotorow;
		hdr = (*FTgethdr)(fd);
		if ((hdr->fhdflags & fdtfCOLREAD) ||
			(hdr->fhd2flags & fdREADONLY) ||
			(hdr->fhdflags & fdINVISIBLE) ||
			(!tqry && hdr->fhdflags & fdQUERYONLY))
		{
			can_land = FALSE;
		}
		if (can_land)
		{
			tbl->tfcurcol = origcol;
			tbl->tfcurrow = origrow;
			return(can_land);
		}

		for (i = 0; i < tbl->tfcols; i++)
		{
			tbl->tfcurcol = i;
			hdr = (*FTgethdr)(fd);
			if (!(hdr->fhdflags & fdINVISIBLE))
			{
				firstcol = i;
				break;
			}
		}
		if (firstcol == -1 || firstcol != evcb->gotocol)
		{
			/*
			**  The desired column is NOT even the first
			**  visible column (no need to look further)
			**  or we have the strange situation where all
			**  columns are invisible.
			**  We just return FALSE to avoid problems.
			*/
			tbl->tfcurcol = origcol;
			tbl->tfcurrow = origrow;
			return(FALSE);
		}

		/*
		**  Now check to see if rest of columns to right of
		**  firstcol are not landable as well.
		*/
		for (i = firstcol + 1; i < tbl->tfcols; i++)
		{
			tbl->tfcurcol = i;
			hdr = (*FTgethdr)(fd);
			if ((hdr->fhdflags & fdtfCOLREAD) ||
				(hdr->fhd2flags & fdREADONLY) ||
				(hdr->fhdflags & fdINVISIBLE) ||
				(!tqry && hdr->fhdflags & fdQUERYONLY))
			{
				continue;
			}

			/*
			**  A column is accessible, need to return FALSE.
			*/
			tbl->tfcurcol = origcol;
			tbl->tfcurrow = origrow;
			return(FALSE);
		}

		/*
		**  No columns to right are accessible, so we can land on this
		**  first visible column.
		*/
		tbl->tfcurcol = origcol;
		tbl->tfcurrow = origrow;
		return(TRUE);
	}
}



/*{
** Name:	IIFTfabFldAboveBelow - Find field above/below current field
**
** Description:
**	This routine looks for the closest field that is above/below the
**	field that is passed in.  Closest is based on the nearest
**	line and field boundary to the cursor position.
**
**	Calculations of cursor and field positions are all zero
**	based and relative to the form and not the terminal screen.
**
** Inputs:
**	frm	Pointer to current active form structure.
**	fld	Pointer to current field structure.
**	fldx	X coordinate of cursor within field.
**	fldy	Y coordinate of cursor within field.
**
** Outputs:
**
**	Returns:
**		<  0	If no field can be found below.
**		>= 0	Field number of field to go to.
**
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/30/92 (dkh) - Initial version.
*/
i4
IIFTfabFldAboveBelow(frm, curfld, fldx, fldy, direction)
FRAME	*frm;
FIELD	*curfld;
i4	fldx;
i4	fldy;
i4	direction;
{
	i4	frmx;
	i4	frmy;
	i4	newfldno = -1;
	i4	deltax = -1;
	i4	newy;
	i4	i;
	i4	curfldno;
	i4	end_of_form;
	i4	diff;
	i4	end_of_fld;
	i4	qrymd;
	FIELD	*nfld;
	FLDHDR	*hdr;
	FLDVAL	*val;
	FLDTYPE	*type;
	TBLFLD	*tbl;

	hdr = (*FTgethdr)(curfld);
	val = (*FTgetval)(curfld);
	type = (*FTgettype)(curfld);
	curfldno = hdr->fhseq;

	/*
	**  The following calculations are correct since
	**  we only get here if we start from a simple field.
	**  So there is no need to worry about table fields
	**  as a starting point.
	*/
	frmx = hdr->fhposx + type->ftdatax + fldx;
	frmy = hdr->fhposy + val->fvdatay + fldy;
	if (direction == BELOW)
	{
		end_of_form = frm->frmaxy;
	}
	else
	{
		end_of_form = 0;
	}

	for (newy = frmy + direction; newy != end_of_form; newy += direction)
	{
		for (i = 0; i < frm->frfldno; i++)
		{
			nfld = frm->frfld[i];
			if (nfld->fltag == FTABLE)
			{
				hdr = &(nfld->fld_var.fltblfld->tfhdr);
			}
			else
			{
				hdr = (*FTgethdr)(nfld);
			}

			/*
			**  We have found a field that is on the desired line.
			**  and it is ACCESSIBLE.
			*/
			if (hdr->fhposy == newy)
			{
				/*
				**  Skip invisible (simple and table) fields.
				*/
				if (hdr->fhdflags & (fdINVISIBLE | fdTFINVIS))
				{
					continue;
				}

				qrymd = (FTcurdisp() == fdmdQRY ||
					FTcurdisp() == fdmdADD);

				/*
				**  If this is a simple field and the field
				**  is readonly or field is query only but
				**  the form mode is not query, then skip
				**  this field.
				*/
				if (nfld->fltag == FREGULAR &&
				  (hdr->fhd2flags & fdREADONLY ||
				  (!qrymd && (hdr->fhdflags & fdQUERYONLY))))
				{
					continue;
				}

				/*
				**  Field starts on desired column position.
				**  This is the field to go to.
				*/
				if (hdr->fhposx == frmx)
				{
					newfldno = i;
					break;
				}

				/*
				**  Is candidate field starting position
				**  greater than cursor column position?
				**  If so, check to see if this is closer
				**  than other fields that we have seen.
				*/
				if (hdr->fhposx > frmx)
				{
					/*
					**  Check to see if this
					**  is a closer field.
					*/
					diff = hdr->fhposx - frmx;

					if (newfldno < 0 ||
						diff < deltax)
					{
						deltax = diff;
						newfldno = i;
					}
				}
				else
				{
					/*
					**  Field starts before cursor
					**  column position.  Need to
					**  see how far away end of field
					**  is.  If field covers the desired
					**  cursor column position, then this
					**  is the field we want to go to.
					*/

					end_of_fld = hdr->fhposx + hdr->fhmaxx;

					/*
					**  Field covers desire position.
					**  This is the field to go to.
					*/
					if (end_of_fld >= frmx)
					{
						newfldno = i;
						break;
					}
					else
					{
						/*
						**  Check to see if this
						**  is a closer field.
						*/
						diff = frmx - end_of_fld;
						if (newfldno < 0 ||
							diff < deltax)
						{
							deltax = diff;
							newfldno = i;
						}
					}
				}
			}
		}

		/*
		**  If we found a field, then the above loop will have
		**  set "newfldno" to be a valid field number (>= 0).
		**  In this case, we can break out, if not, continue
		**  looking until we reach the boundary of the form.
		*/
		if (newfldno >= 0)
		{
			/*
			**  Before breaking out, we need to determine
			**  if we are headed for a table field.  If so,
			**  must calculate which column in table field
			**  we can land on.
			*/
			nfld = frm->frfld[newfldno];
			tbl = nfld->fld_var.fltblfld;
			qrymd = tbl->tfhdr.fhdflags & fdtfQUERY
				|| tbl->tfhdr.fhdflags & fdtfAPPEND;
			tbl->tfcurcol = 0;
			while (tbl->tfcurcol < tbl->tfcols)
			{
				hdr = (*FTgethdr)(nfld);
				if ((hdr->fhdflags & fdtfCOLREAD)
				    || (hdr->fhd2flags & fdREADONLY)
				    || (hdr->fhdflags & fdINVISIBLE)
				    || (!qrymd && hdr->fhdflags & fdQUERYONLY))
				{
					tbl->tfcurcol++;
				}
				else
				{
					break;
				}
			}
			/*
			**  If none are normally landable, then
			**  pick first visible column.
			*/
			if (tbl->tfcurcol >= tbl->tfcols)
			{
				tbl->tfcurcol = 0;
				while (tbl->tfcurcol < tbl->tfcols)
				{
					hdr = (*FTgethdr)(nfld);
					if (hdr->fhdflags & fdINVISIBLE)
					{
						tbl->tfcurcol++;
				 	}
				 	else
				 	{
						break;
				 	}
				}
			}
			break;
		}
	}

	/*
	**  Now return the new field number, if any.  Since "newfldno"
	**  is initialized to -1, no additional checks are needed.
	*/
	return(newfldno);
}


/*{
** Name:	FTmove	- Used to move the cursor to a new location
**
** Description:
**	Used to move the cursor to a new location.  Given the
**	operation, move the cursor within the field or to a new field.
**
** Inputs:
**	op	operation being done
**	frm	current frame
**	fldno	pointer to the number of the current field
**	win	pointer to a pointer to the current data window
**	repts	how many moves to do
**	evcb	event control block for state information
**
** Outputs:
**	fldno	point to the number of the current field after movement
**	win	point to a pointer to the data window after movement
**
**	Returns:
**		fdNOCODE	Default return--"nothing more to do."
**		code		Interrupt value, or specification of special
**				operation.  (i.e. force-to-browse or scroll-up.)
**
**	Exceptions:
**		None
**
** Side Effects:
**	Can change the current field.
**
** History:
**			Skip display only columns. (dkh)
**	22-mar-1984 -	Skip query only fields/columns. (ncg)
**	19-aug-1985 -	Added support for invisible table fields. (dkh)
**	05/02/87 (dkh) - Changed to call FD routines via passed pointers.
**	04/04/90 (dkh) - Integrated MACWS changes.
**  11/05/99 (kitch01) 
**     Bug 48278. If CTRL+N (fdopNROW), is requested search for the first
**     visible tablefield column to prevent trying to land on an invisible
**     column.
*/

i4
FTmove(op, frm, fldno, win, repts, evcb)
i4		op;
FRAME		*frm;
i4		*fldno;
WINDOW		**win;
i4		repts;
FRS_EVCB	*evcb;
{
	reg	FIELD	*fd;
	reg	TBLFLD	*tbl = NULL;
	TBLFLD		*ntbl;
	i4		code = fdNOCODE;
	i4		tqry;		/* table field in query mode */
	i4		ofldno;
	reg	FLDHDR	*hdr;
	reg	FLDHDR	*thdr;
	i4		origfldno;
	i4		islast;
	i4		ocol;
	bool		qrymd;
	i4		origrowno;	/* used for entry activation. */
	i4		origcolno;	/* used for entry activation. */
	i4		nbrrecds, currecd;
	i4		x_adj, y_adj;
	i4		newfldno;

	origfldno = *fldno;

    	fd = frm->frfld[*fldno];
    	if (fd->fltag == FTABLE)
    	{
		tbl = fd->fld_var.fltblfld;
		tqry = tbl->tfhdr.fhdflags & fdtfQUERY
				|| tbl->tfhdr.fhdflags & fdtfAPPEND;
		origcolno = tbl->tfcurcol;
		origrowno = tbl->tfcurrow;
    	}

	if (op == fdopNXITEM)
	{
		/*
		**  If we are in the last accessible column of a
		**  table field, then
		**  NEXTITEM command emulates a NEWRROW command.
		**  Otherwise, NEXTITEM becomes a NEXTFIELD command.
		*/
		op = fdopNEXT;
		if (tbl)
		{
			islast = TRUE;
			ocol = tbl->tfcurcol;
			tbl->tfcurcol++;
			while(tbl->tfcurcol < tbl->tfcols)
			{
				hdr = (*FTgethdr)(fd);
				if ((hdr->fhdflags & fdtfCOLREAD)
				    || (hdr->fhd2flags & fdREADONLY)
				    || (hdr->fhdflags & fdINVISIBLE)
				    || (!tqry && hdr->fhdflags & fdQUERYONLY))
				{
					tbl->tfcurcol++;
				}
				else
				{
					islast = FALSE;
					break;
				}
			}
			if (islast)
			{
				op = fdopRET;
			}
			tbl->tfcurcol = ocol;
		}
	}

        switch (op)
        {
	  case fdopGOTO:
		/*
		**  FTinsert() and FTbrowse() have already check
		**  that the destination is OK and that we are
		**  going to a NEW location.  Just set new
		**  destination and break out.
		*/
		if (tbl && *fldno != evcb->gotofld)
		{
			tbl->tfcurrow = 0;
			tbl->tfcurcol = 0;
		}

		*fldno = evcb->gotofld;
		fd = frm->frfld[*fldno];
		if (fd->fltag == FTABLE)
		{
			ntbl = fd->fld_var.fltblfld;
			ntbl->tfcurrow = evcb->gotorow;
			ntbl->tfcurcol = evcb->gotocol;
		}
		break;

	  case fdopPREV:		/* Go to previous field	 */
		/*
		** Special case if we are currently inside a table field.
		*/
                if (tbl)
	        {
			/*
			** Back out of column and skip read/query only columns.
			** Back out for as many repetitions as specified, or
			** until moving out of the table field.
			*/
			tbl->tfcurcol--;
			while (repts > 0 && tbl->tfcurcol >= 0)
			{
				hdr = (*FTgethdr)(fd);
				if ((hdr->fhdflags & fdtfCOLREAD)
				    || (hdr->fhd2flags & fdREADONLY)
				    || (hdr->fhdflags & fdINVISIBLE)
				    || (!tqry && hdr->fhdflags & fdQUERYONLY))
				{
					tbl->tfcurcol--;
				}
				else
				{
					repts--;
				}
			}

			/*
			** If we are still within the table field, then
			** the window has been found in the table field.
			*/
			if (tbl->tfcurcol >= 0)
			{
				break;
			}
			else
			{
				/*
				** Reset table field info before moving out.
				*/
				tbl->tfcurcol = 0;
				tbl->tfcurrow = 0;
			}
	        }
		/*
		** Either backed out of table field or was in regular field.
		** Skip QueryOnly fields in non-Query form.  Stop at a table
		** field, or all the way round the form.
		*/
		if ((*fldno -= (repts % frm->frfldno)) < 0)
		    *fldno += frm->frfldno;

		qrymd = (FTcurdisp() == fdmdQRY || FTcurdisp() == fdmdADD);

		ofldno = *fldno;
		do
		{
		    fd = frm->frfld[*fldno];
		    if (fd->fltag == FTABLE)
		    {
			/* If invisible, skip the field */
			if (!(fd->fld_var.fltblfld->tfhdr.fhdflags
			    & (fdINVISIBLE | fdTFINVIS)))
			{
			    /*
			    **  Before breaking out, we need to determine
			    **  which column is accessible.  Can't assume
			    **  that first one is landable since it may be
			    **  invisible.
			    */
			    tbl = fd->fld_var.fltblfld;
			    tqry = tbl->tfhdr.fhdflags & fdtfQUERY
				|| tbl->tfhdr.fhdflags & fdtfAPPEND;
			    tbl->tfcurcol = 0;
			    while (tbl->tfcurcol < tbl->tfcols)
			    {
				thdr = (*FTgethdr)(fd);
				if ((thdr->fhdflags & fdtfCOLREAD)
				    || (thdr->fhd2flags & fdREADONLY)
				    || (thdr->fhdflags & fdINVISIBLE)
				    || (!tqry && thdr->fhdflags & fdQUERYONLY))
				{
					tbl->tfcurcol++;
				}
				else
				{
					break;
				}
			    }
			    /*
			    **  If none are normally landable, then
			    **  pick first visible column.
			    */
			    if (tbl->tfcurcol >= tbl->tfcols)
			    {
				tbl->tfcurcol = 0;
				while (tbl->tfcurcol < tbl->tfcols)
				{
				    thdr = (*FTgethdr)(fd);
				    if (thdr->fhdflags & fdINVISIBLE)
				    {
					tbl->tfcurcol++;
				    }
				    else
				    {
					break;
				    }
				}
			    }
			    break;
			}
		    }
		    else /* regular field */
		    {
			hdr = (*FTgethdr)(fd);

			if (!(hdr->fhdflags & fdINVISIBLE)
			    && !(hdr->fhd2flags & fdREADONLY)
			    && (qrymd || !(hdr->fhdflags & fdQUERYONLY)))
			{
			    break;
			}
		    }

		    if (--(*fldno) < 0)
		    {
			*fldno = frm->frfldno - 1;
		    }
		} while (*fldno != ofldno);	/* Scanned all fields */
	        break;

	  case fdopNROW:
		if (tbl)
		{
			/*
			**  Special work if we are in READ ONLY table
			**  field and user has requested one or multiple
			**  control-Ns.
			*/

			if (tbl->tfhdr.fhdflags & fdtfREADONLY)
			{
				tbl->tfcurcol = 0;
				/* Bug 48278. Find the first visible column in the tablefield
				** prevents SEGV when col 0 is invisible
				*/
				while (tbl->tfcurcol < tbl->tfcols)
				{
				    thdr = (*FTgethdr)(fd);
				    if (thdr->fhdflags & fdINVISIBLE)
				    {
					tbl->tfcurcol++;
				    }
				    else
				    {
					break;
				    }
				}
				code = FTdownrow(tbl, repts);
				break;
			}

			/*
			** To go to start of next row, go last column and act
			** like a carriage return, by a fall through.
			*/
			tbl->tfcurcol = tbl->tfcols - 1;
		}		/* FALL THROUGH */
    	  case fdopRET:
    		if (tbl)
    		{
    			code = FTrightcol(tbl, repts);
    			break;
    		}		/* FALL THROUGH */
	  case fdopNEXT:		/* Go to next field field */
		/*
		** Special situation if we are in a table field.
		*/
        	if (tbl)
	        {
			/*
			** Step out of column and skip read/query only columns.
			** Go forward for as many repetitions as specified, or
			** until exiting the table field.
			*/
			tbl->tfcurcol++;
			while (repts > 0 && tbl->tfcurcol < tbl->tfcols)
			{
				hdr = (*FTgethdr)(fd);
				if ((hdr->fhdflags & fdtfCOLREAD)
				    || (hdr->fhd2flags & fdREADONLY)
				    || (hdr->fhdflags & fdINVISIBLE)
				    || (!tqry && hdr->fhdflags & fdQUERYONLY))
				{
					tbl->tfcurcol++;
				}
				else
				{
					repts--;
				}
			}

			/*
			** If we are still within the table field, then
			** the window has been found in the table field.
			*/
			if (tbl->tfcurcol < tbl->tfcols)
			{
				break;
			}
			else
			{
				/*
				** Reset table field info before moving out.
				** Note that we blindly assume the first
				** column is OK.  Even if this is not correct
				** we rely on cursor tracking and other
				** parts of the movement code to compensate
				** for this.  We need to leave this code
				** alone since we want to land on the first
				** landable column when we enter this table
				** field again.  Any changes here will only
				** insure that we are in a landable column
				** as we leave the table field and this is
				** not necessarily the first landable column
				** when we enter the table field later on.
				*/
				tbl->tfcurcol = 0;
				tbl->tfcurrow = 0;
#ifdef	SCROLLBARS
		                WNSbarDestroy();
#endif
			}
	        }

		/*
		** Either exited the table field or was in regular field.
		** Skip QueryOnly fields in non-Query form.  Stop at a table
		** field, or all the way round the form.
		*/
	        *fldno = (*fldno + repts) % frm->frfldno;

		qrymd = (FTcurdisp() == fdmdQRY || FTcurdisp() == fdmdADD);

		ofldno = *fldno;
		do
		{
		    fd = frm->frfld[*fldno];
		    if (fd->fltag == FTABLE)
		    {
			/* If invisible, skip the field */
			if (!(fd->fld_var.fltblfld->tfhdr.fhdflags
			    & (fdINVISIBLE | fdTFINVIS)))
			{
			    /*
			    **  Before breaking out, we need to determine
			    **  which column is accessible.  Can't assume
			    **  that first one is landable since it may be
			    **  invisible.
			    */
			    tbl = fd->fld_var.fltblfld;
			    tqry = tbl->tfhdr.fhdflags & fdtfQUERY
				|| tbl->tfhdr.fhdflags & fdtfAPPEND;
			    tbl->tfcurcol = 0;
			    while (tbl->tfcurcol < tbl->tfcols)
			    {
				thdr = (*FTgethdr)(fd);
				if ((thdr->fhdflags & fdtfCOLREAD)
				    || (thdr->fhd2flags & fdREADONLY)
				    || (thdr->fhdflags & fdINVISIBLE)
				    || (!tqry && thdr->fhdflags & fdQUERYONLY))
				{
					tbl->tfcurcol++;
				}
				else
				{
					break;
				}
			    }
			    /*
			    **  If none are normally landable, then
			    **  pick first visible column.
			    */
			    if (tbl->tfcurcol >= tbl->tfcols)
			    {
				tbl->tfcurcol = 0;
				while (tbl->tfcurcol < tbl->tfcols)
				{
				    thdr = (*FTgethdr)(fd);
				    if (thdr->fhdflags & fdINVISIBLE)
				    {
					tbl->tfcurcol++;
				    }
				    else
				    {
					break;
				    }
				}
			    }
			    break;
			}
		    }
		    else /* regular field */
		    {
			hdr = (*FTgethdr)(fd);

			if (!(hdr->fhdflags & fdINVISIBLE)
			    && !(hdr->fhd2flags & fdREADONLY)
			    && (qrymd || !(hdr->fhdflags & fdQUERYONLY)))
			{
			    break;
			}
		    }

		    if (++(*fldno) >= frm->frfldno)
		    {
			*fldno = 0;
		    }
		} while (*fldno != ofldno);	/* Scanned all fields */
	        break;

	  case fdopLEFT:		/* Move cursor right in field */
		hdr = (*FTgethdr)(fd);
		if (hdr->fhdflags & fdREVDIR)
			TDrmvleft(*win, repts);
		else
			TDmvleft(*win, repts);
	        TDrefresh(*win);
	        return fdNOCODE;

	  case fdopRIGHT:		/* Move cursor right in field */
		hdr = (*FTgethdr)(fd);
		if (hdr->fhdflags & fdREVDIR)
			TDrmvright(*win, repts);
		else
			TDmvright(*win, repts);
	        TDrefresh(*win);
	        return fdNOCODE;

	  case fdopUP:		/* Move cursor up in field */
    		if (tbl && (*win)->_cury == 0)
    		{
    			code = FTuprow(tbl, repts);
    			break;
    		}
		/*
		**  If we are currently in a simple field and the cursor
		**  is on the last line of the data entry area, then
		**  move to available field below current field.
		**
		**  The return value of IIFTfbFldBelow() is the field
		**  number of the field to go to.  If it is less than
		**  zero, then there is no available field and we just
		**  exit with fdNOCODE at this point.  If there is a
		**  field to go to, we break out of the loop so we can
		**  do processing for a change in field focus.
		*/
		if (!tbl && (*win)->_cury == 0 &&
			(IIFTglcb->enabled & UP_DOWN_EXIT))
		{
			if ((newfldno = IIFTfabFldAboveBelow(frm, fd,
				(*win)->_curx, (*win)->_cury, ABOVE)) >= 0)
			{
				*fldno = newfldno;
				break;
			}
			else
			{
				return(fdNOCODE);
			}
		}
		TDmvup(*win, repts);
	        TDrefresh(*win);
	        return fdNOCODE;

	  case fdopDOWN:		/* Move cursor down in field */
    		if (tbl && (*win)->_cury == (*win)->_maxy - 1)
    		{
    			code = FTdownrow(tbl, repts);
    			break;
    		}
		/*
		**  If we are currently in a simple field and the cursor
		**  is on the last line of the data entry area, then
		**  move to available field below current field.
		**
		**  The return value of IIFTfbFldBelow() is the field
		**  number of the field to go to.  If it is less than
		**  zero, then there is no available field and we just
		**  exit with fdNOCODE at this point.  If there is a
		**  field to go to, we break out of the loop so we can
		**  do processing for a change in field focus.
		*/
		if (!tbl && (*win)->_cury == (*win)->_maxy - 1 &&
			(IIFTglcb->enabled & UP_DOWN_EXIT))
		{
			if ((newfldno = IIFTfabFldAboveBelow(frm, fd,
				(*win)->_curx, (*win)->_cury, BELOW)) >= 0)
			{
				*fldno = newfldno;
				break;
			}
			else
			{
				return(fdNOCODE);
			}
		}
		TDmvdown(*win, repts);
	        TDrefresh(*win);
	        return fdNOCODE;

# ifdef	DATAVIEW
	case fdopSKPFLD:        /* Skip field for mouse movement */
		{
			/* Should get here only when MWS active */
			FUNC_EXTERN          FTrowtrv();

			_VOID_ IIMWdsmDoSkipMove(frm, fldno,
				FTcurdisp(), FTrowtrv, FTvalfld);
			break;
		}
# endif	/* DATAVIEW */

        }

	/*
	** If code != fdNOCODE, means that one of the movements above
	** generated a scroll, setting code to the scroll[up|down] value.
	*/
	if (code != fdNOCODE)
	{
		/*
		** See if location on form has changed since if so I'll
		** want to generate an EA after the scroll is handled.
		** Guaranteed that I'll be in a table field when 'code'
		** is changed.
		*/
		if ((tbl->tfcurcol != origcolno)
		    || (tbl->tfcurrow != origrowno))
		{
		    frm->frres2->savetblpos = TRUE;
		    frm->frres2->origrow = origrowno;
		    frm->frres2->origcol = origcolno;

	        }
#ifdef SCROLLBARS
        FTWNRedraw(frm, *fldno);
#endif
        return(code);
	}

    	/*
    	** Only gets here because of a break above.
	** At this point, the fldno may have been changed.
	** If we moved into a table field which is display
	** only we have to force a return to browse mode.
    	** Some operations do a return instead.
	** Pass address of field num as FTgetwin may change it.
    	*/
#ifdef SCROLLBARS
    FTWNRedraw(frm, *fldno);
#endif

	/*
	**  Take care of clearing cursor tracking on old field
	**  and setting it for new field.
	*/
	if (origfldno != *fldno)
	{
		FTrcrtrk(frm, origfldno);
#ifdef	SCROLLBARS
	        WNSbarDestroy();
#endif
	}
	FTscrtrk(frm, *fldno);

#ifdef	SCROLLBARS				 
	fd = frm->frfld[*fldno];
	tbl = fd->fld_var.fltblfld;
	thdr = &(tbl->tfhdr);
	(*IIFTdsrDSRecords)(tbl, &nbrrecds, &currecd);
	x_adj = frm->frposx ? frm->frposx - 1 : 0;
	y_adj = frm->frposy ? frm->frposy - 1 : 0;
    if (frm->frmflags & fdISPOPUP)
        ++y_adj;      /* column header */
    if (tbl->tfrows > 0 && nbrrecds > tbl->tfrows)
	{
	        WNSbarCreate(thdr->fhposx + x_adj,  /* x coordinate */
        	             thdr->fhposy + y_adj,  /* y coordinate */
	                     thdr->fhmaxx,          /* width  */
        	             thdr->fhmaxy,          /* height */
                	     currecd,		    /* current tbl fld row  */
	                     tbl->tfrows,           /* visible tbl fld rows */
        	             nbrrecds);		    /* dataset record count */
	}
#endif
        if ((*win = FTgetwin(frm, fldno)) == NULL)
		return (fdopMENU);

	IIFTsiiSetItemInfo((*win)->_begx + FTwin->_startx + 1,
	    (*win)->_begy + FTwin->_starty + 1, (*win)->_maxx, (*win)->_maxy);

	hdr = (*FTgethdr)(frm->frfld[*fldno]);
	if (hdr->fhdflags & fdREVDIR)
		TDmove(*win, (i4)0, (*win)->_maxx - 1);
	else
		TDmove(*win, (i4)0, (i4)0);
        TDrefresh(*win);
	frm->frcurfld = *fldno;

	/*
	** If entry activation is enabled, and the 'new' current
	** field has entry activation specified, and this really
	** is a new field (as opposed to movement within a field),
	** then set flag in event control block to indicate that user
	** is in an entry activation (so that resume code above won't
	** improperly re-trigger an EA), and return the activation
	** value.  Note that determination that this really is a new
	** field depends on the 'return(fdNOCODE)' code above--will
	** never get here on a local movement.  Of course, possible
	** to get here for original location if all other locations
	** are unreachable....
	*/
	if (IIFTavcb->act_entry && (hdr->fhenint != 0))
	{
	    if ((origfldno != *fldno)
		|| ((tbl != NULL)
		    && ((origrowno != tbl->tfcurrow)
			|| (origcolno != tbl->tfcurcol))))
	    {
		IIFTevcb->entry_act = TRUE;
		return(hdr->fhenint);
	    }
	}

	if (FTforcebrowse(frm, *fldno))
	{
		return fdopBRWSGO;
	}
    	return fdNOCODE;
}

