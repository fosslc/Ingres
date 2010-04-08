/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<frserrno.h>
# include       <er.h>
# include	"fdfuncs.h"

/*
**	static	char	Sccsid[] = "%W% %G%";
*/

/*
** copyup.c
**
** contains
**	FDcopyup(tbl, from, to)
**
** History:
**	30-apr-1984	Improved interface to routines for performance (ncg)
**	02/08/87 (dkh) - Added support for ADTs.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added change bit for datasets.
**	09/16/87 (dkh) - Integrated table field change bit.
**	12/23/87 (dkh) - Performance changes.
**	02/01/88 (dkh) - Fixed jup bug 1905.
**	07/26/90 (dkh) - Added support for table field cell highlighting.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**      02-apr-97 (cohmi01)
**	    Add FDmoverows() to accomodate block mode scrolling for bug 73587.
**	    Add 'skip' parm to FDdocopy() to allow for shift of >1 row.
**	05-may-97 (cohmi01)
**	    To improve single line scroll response with UPFRONT, try
**	    to send X131 msg if available. (73587).
**	01-aug-97 (kitch01)
**		Bug 83988. The above change did not update the displayed rows array.
**		This could cause "phantom" updates as this array is now out of step
**		with what is actually displayed. Moved the sending of X131 to FDdocopy.
**	09-dec-97 (kitch01)
**		Bug 87666. Re-instate the sending of X131 to FDmoverows. FDmoverows is called
**		only when we are scrolling the DS. FDdocopy can be called when inserting/deleting
**		rows and under certain circumstances would send X131. This caused bug 87666. Amended
**		the interface to FDdocopy to prevent old scrolling method if X131 was sent.
**	
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
*/


/*
** copy the data in the table field tbl,
** from is the start and to is the ending row.
** if from > to then copy in reverse direction.
*/
FUNC_EXTERN	i4	FDersfld();
FUNC_EXTERN	i4	IIFDccb();
static		VOID	FDdocopy();

FDcopyup(tbl, from, to)
TBLFLD	*tbl;
i4	from,
	to;
{
	if (from > to)
	{
		FDdocopy(tbl, from, to, (i4) -1, 0, FAIL);
		return;
	}

	FDdocopy(tbl, from, to, (i4) 1, 0, FAIL);
}

FDcopydown(tbl, from, to)
TBLFLD	*tbl;
i4	from,
	to;
{
	FDdocopy(tbl, from, to, (i4) -1, 0, FAIL);
}

/*{
** Name:        FDmoverows - Move a block of rows up/down n rows.
**
** Description:
** 	Similar to FDcopydown/FDcopyup except that the move may
**	be to a target position more than one row away from the
**	origin. Uses new 'skip' parm added to FDdocopy.
**
** Inputs:
**
**	tbl		Tablefield info
**	from		Start of block of rows (0 based)
**	to		One more than 0 based last row number of block.
**	skip		# of rows to augment the usual 1 row shift done
**			by FDdocopy().
** Outputs:
**      Returns:
**              None.
**
**
** History:
**	02-apr-97 (cohmi01)
**	    Created.
**	01-aug-97 (kitch01)
**		Bug 83988. Placing X131 message (for Upfront) here causes "phantom"
**		updates of the database (should the app allow updates). This is because
**		the displayed rows array is not updated here.
**  11-nov-1997 (rodjo04)
**      In FDdocopy(), before issueing a X131, we should check to see if we
**      need to scroll. A check for (skip == 0) is no longer enough.
**	08-dec-1997 (kitch01)
**		Re-instated the call for sending X131 to upfront.
*/

FDmoverows(tbl, from, to, skip)
TBLFLD	*tbl;
i4	from,
	to,
	skip;
{
  	i4 direction;
	i4	fldno;
	FRAME	*frm;
	i4 X131_failed = FAIL;

	if (from > to)
	{
	    direction = -1;
	    skip = -skip;
	}
	else
	    direction = 1;
	/* 
	** If skip is zero we are shifting many rows by one position,
	** a case that may be optimised for MWS with new X131 cmd.
	** Also don't issue a X131 if we don't need to scroll. 
	*/
	
	if ((skip == 0) && IIMWimIsMws() && (from != to))
	{
		FDftbl(tbl, &frm, &fldno);
		if (frm == NULL)
		{
			IIFDerror(TBLNOFRM, 0, (char *) NULL);
			PCexit(-1);
		}
	    /*
	    ** Try to issue X131, if fails, it's not available in this
	    ** release of MWS, so do it the slower way, else all done.
	    */
	    X131_failed = IIMWtstfTryScrollTblFld(frm, fldno, direction);
	}


	FDdocopy(tbl, from, to, direction, skip, X131_failed);
}


static	VOID
FDdocopy(tbl, from, to, direction, skip, X131_failed)
TBLFLD	*tbl;
i4	from;
i4	to;
i4	direction;
i4	skip;
i4  X131_failed;
{
	FLDVAL	*torow,
		*fromrow;
	FLDVAL	*tocol,
		*fromcol;
	i4	i,
		j;
	i4	qry;
	i4	fldno;
	FRAME	*frm;
	i4	*toflags;
	i4	*fromflags;
	i4	hilight;
	i4	toattr;
	i4	fromattr;
	i4	dsattr;
	i4	rownum;

	FDftbl(tbl, &frm, &fldno);
	if (frm == NULL)
	{
		IIFDerror(TBLNOFRM, 0, (char *) NULL);
		PCexit(-1);
	}

	/* reset data testing flag for FDqrydata() */
	if ((qry = tbl->tfhdr.fhdflags & fdtfQUERY))
		FDqrytest((i4)FALSE);
	torow = tbl->tfwins + ((from - skip) * tbl->tfcols);
	fromrow = torow + ((direction + skip) * tbl->tfcols);
	for (i = 0; i < tbl->tfcols; i++)
	{
		tocol = torow;
		fromcol = fromrow;

		toflags = tbl->tffflags + ((from - skip) * tbl->tfcols) + i;
		fromflags = toflags + ((direction + skip) * tbl->tfcols);

		for (j = from ; ; j += direction)
		{
			if (direction == 1)
			{
				if (j >= to)
				{
					break;
				}
			}
			else
			{
				if (j <= to)
				{
					break;
				}
			}
			rownum = j - skip;

			/*
			**  Do not copy the row highlighting bit.
			**  If needed, it will be renabled later
			**  on when we enter FTrun.
			**
			**  Don't copy the attributes for the cell
			**  either.  But do copy the dataset attributes.
			*/

			hilight = ((*toflags & fdROWHLIT) ? TRUE : FALSE);
			if ((*toflags & dsALLATTR) != (*fromflags & dsALLATTR))
			{
				dsattr = TRUE;
			}
			else
			{
				dsattr = FALSE;
			}
			toattr = *toflags & tfALLATTR;
			*toflags = (*fromflags & ~(fdROWHLIT | tfALLATTR));
			if (hilight)
			{
				*toflags |= fdROWHLIT;
			}
			if (toattr)
			{
				*toflags |= toattr;
			}

			/*
			**  Need to call FTsetda if we have cells
			**  that changed their attributes.
			*/
			if (dsattr)
			{
				FTsetda(frm, tbl->tfhdr.fhseq, FT_UPDATE,
					FT_TBLFLD, i, rownum, 0);
			}

			/*
			**  Since the to and from datatypes are EXACTLY
			**  the same, we just do an MEcopy of the data
			**  area for speed.
			*/
			MEcopy(fromcol->fvdbv->db_data,
				(u_i2) fromcol->fvdbv->db_length,
				tocol->fvdbv->db_data);

			/*
			**  Copy the display buffers as well, since the
			**  data should have been formatted when the
			**  it was validated.  Also, since the display
			**  buffers are copied, the query operators
			**  should be in place already.
			*/
			MEcopy(fromcol->fvdsdbv->db_data,
				(u_i2) fromcol->fvdsdbv->db_length,
				tocol->fvdsdbv->db_data);

			/*
			** If the issueing of X131 for Upfront failed, or this is not
			** using Upfront, then send each cell to the form.
			*/
			if ( X131_failed )
				FTfldupd(frm, fldno, FT_UPDATE, FT_TBLFLD, i, rownum);

			tocol += (direction * tbl->tfcols);
			fromcol += (direction * tbl->tfcols);

			toflags += (direction * tbl->tfcols); 
			fromflags += (direction * tbl->tfcols);
		}
		torow++;
		fromrow++;
	}

	/* set data testing flag for FDqrydata() */
	if (qry)
		FDqrytest((i4)TRUE);
}
