/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h> 
# include	<runtime.h> 
# include	"fdfuncs.h"


/**
** Name:	fdmove.c  -  The MOVE Actions of the Frame Driver
**
** Description:
**	This file defines:
**	FDmove		- Used to move the cursor to a new location.
**
** History:
**			Skip display only columns. (dkh)
**	22-mar-1984 -	Skip query only fields/columns. (ncg)
**	03/05/87 (dkh) - Added support for ADTs.
**	11/11/87 (dkh) - Code cleanup.
**	04/15/88 (dkh) - Added support for nextitem command.
**	22-dec-88 (bruceb)
**		Handle movement given existence of readonly and invisible
**		fields.
**	01-mar-89 (bruceb)
**		Changed file and routine headers.
**	08-sep-89 (bruceb)
**		Handle invisible columns.
**	12/27/89 (dkh) - Moved IIFTgcGotoChk() to this file from FT. 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

FUNC_EXTERN	FLDHDR	*FDgethdr();
FUNC_EXTERN	i4	FDrightcol();
FUNC_EXTERN	i4	FDuprow();
FUNC_EXTERN	i4	FDdownrow();
FUNC_EXTERN	i4	IIFDgcGotoChk();

GLOBALREF	FRS_EVCB	*IIFDevcb;

/*{
** Name:	IIFDgcGotoChk - Can we land on desired column in table field.
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
IIFDgcGotoChk(frm, evcb)
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
	**  Clients (like FTinsert() and FTbrowse() and FDmove()) have
	**  already checked to
	**  see if destination is different.  NO need to check again.
	*/
	fd = frm->frfld[evcb->gotofld];
	if (fd->fltag == FREGULAR)
	{
		hdr = FDgethdr(fd);
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
		hdr = FDgethdr(fd);
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
			hdr = FDgethdr(fd);
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
			hdr = FDgethdr(fd);
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
** Name:	FDmove	- Used to move the cursor to a new location
**
** Description:
**	Used to move the cursor to a new location.  Given the
**	operation, move the cursor within a table field or to a new field.
**
** Inputs:
**	op	operation being done
**	frm	current frame
**	fldno	pointer to the number of the current field
**	repts	how many moves to do
**
** Outputs:
**	fldno	point to the number of the current field after movement
**
**	Returns:
**		fdNOCODE	Default return--"nothing more to do."
**		code		Specification of special operation.
**				(i.e. scroll-up.)
**
**	Exceptions:
**		None
**
** Side Effects:
**	Can change the current field.
**
** History:
**	01-mar-89 (bruceb)
**		Added this header.
*/

i4
FDmove(op, frm, fldno, repts)
i4	op;
FRAME	*frm;
i4	*fldno;
i4	repts;
{
	reg	FIELD	*fd;
	reg	TBLFLD	*tbl = NULL;
	i4		code;
	i4		tqry;		/* table field in query mode */
	i4		ofldno;
	reg	FLDHDR	*hdr;
	i4		islast;
	i4		ocol;
	bool		qrymd;

    	fd = frm->frfld[*fldno];
    	if (fd->fltag == FTABLE)
    	{
		tbl = fd->fld_var.fltblfld;
		tqry = tbl->tfhdr.fhdflags & fdtfQUERY
		    || tbl->tfhdr.fhdflags & fdtfAPPEND;
    	}

	if (op == fdopNXITEM)
	{
		/*
		**  If we are in the last accessible column of a
		**  table field, then
		**  NEXTITEM command emulates a NEWROW command.
		**  Otherwise, NEXITEM becomes a NEXTFIELD command.
		*/
		op = fdopNEXT;
		if (tbl)
		{
			islast = TRUE;
			ocol = tbl->tfcurcol;
			tbl->tfcurcol++;
			while(tbl->tfcurcol < tbl->tfcols)
			{
				hdr = FDgethdr(fd);
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
		**  Check the desired location since it may no longer
		**  be good due to code that was executed in activate
		**  block.
		*/
		/* Temp hack for now */
		if (!IIFDgcGotoChk(frm, IIFDevcb))
		{
			FTbell();
			break;
		}
		*fldno = IIFDevcb->gotofld;
		fd = frm->frfld[*fldno];
		if (fd->fltag == FTABLE)
		{
			tbl = fd->fld_var.fltblfld;
			tbl->tfcurcol = IIFDevcb->gotocol;
			tbl->tfcurrow = IIFDevcb->gotorow;
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
				hdr = FDgethdr(fd);
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

		qrymd = (FDcurdisp() == fdmdQRY || FDcurdisp() == fdmdADD);

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
			    break;
			}
		    }
		    else
		    {
			hdr = FDgethdr(fd);

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
				if ((code = FDdownrow(tbl, repts)) != fdNOCODE)
				{
					return(code);
				}
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
    			if ((code = FDrightcol(tbl, repts)) != fdNOCODE)
    				return(code);
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
			while(repts > 0 && tbl->tfcurcol < tbl->tfcols)
			{
				hdr = FDgethdr(fd);
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
				*/
				tbl->tfcurcol = 0;
				tbl->tfcurrow = 0;
			}
	        }

		/*
		** Either exited the table field or was in regular field.
		** Skip QueryOnly fields in non-Query form.  Stop at a table
		** field, or all the way round the form.
		*/
	        *fldno = (*fldno + repts) % frm->frfldno;

		qrymd = (FDcurdisp() == fdmdQRY || FDcurdisp() == fdmdADD);

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
			    break;
			}
		    }
		    else /* regular field */
		    {
			hdr = FDgethdr(fd);

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

	/*
	**  Fix for BUG 8889. (dkh)
	*/
	case fdopSCRDN:
	case fdopUP:		/* Move cursor up in field */
    		if (tbl)
    		{
    			if ((code = FDuprow(tbl, repts)) != fdNOCODE)
    				return code;
    		}
	        break;

	/*
	**  Fix for BUG 8889. (dkh)
	*/
	case fdopSCRUP:
	case fdopDOWN:		/* Move cursor down in field */
    		if (tbl)
    		{
    			if ((code = FDdownrow(tbl, repts)) != fdNOCODE)
    				return code;
    		}
	        break;
        }          

    	/* At this point, the fldno may have been changed.  */
	frm->frcurfld = *fldno;

    	return fdNOCODE;
}
