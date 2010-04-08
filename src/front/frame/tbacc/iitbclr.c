/*
**	iitbclr.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
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
# include	<menu.h>
# include	<runtime.h>
# include	<rtvars.h>

/**
** Name:	iitbclr.c
**
** Description:
**
**	Public (extern) routines defined:
**		IITBtclr()
**	Private (static) routines defined:
**
** History:
**	08/14/87 (dkh) - ER changes.
**	09/16/87 (dkh) - Integrated table field change bit.
**	23-mar-89 (bruceb)
**		If table field being cleared out is the field user was
**		last on (the current field as of return to user code from
**		the FRS), and field has a dataset (don't do anything for bare
**		table fields) reset origfld to bad value to force an entry
**		activation.
**	06-oct-89 (bruceb)
**		Invalidate aggregate derivations based off this table when
**		clearing.  Explicitly done for update and read mode.
**		Implicitly done for fill mode (under the IIscr_fake).
**	08/11/91 (dkh) - Added fix to clear out dataset attributes from
**			 table field celss when a table field is cleared.
**	03/01/92 (dkh) - Renamed IItclr() to IITBtclr() to avoid name
**			 space conflicts with eqf generated symbols.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

FUNC_EXTERN	VOID	IIFDiaaInvalidAllAggs();


/*{
** Name:	IITBtclr	-	Clear a tablefield
**
** Description:
**	Clear the display of a tablefield, and free it's data set.
**
**	The tablefield's current row, last row, and current column
**	(tf->currow, tf->lastrow, tf->curcol) are all reset.
**
** Inputs:
**	tb		Ptr to the tablefield structure to clear
**
** Outputs:
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	04-mar-1983 	- written (ncg)
**	14-jun-1985	- Initial UPDATE mode is READ mode. (ncg)
**
*/

IITBtclr(tb, frm)
register	TBSTRUCT	*tb;
FRAME	*frm;
{
	DATSET			*ds;		/* data set (if linked) */
	FUNC_EXTERN	i4	FDclr();	/* clear display area */
	FUNC_EXTERN	i4	FDdat();	/* clear data area */
	FUNC_EXTERN	i4	IIFDccb();
	i4			*origfld;
	TBLFLD			*tf;
	i4			i;
	i4			j;
	i4			*flags;
	i4			*pflag;
	i4			cols;
	i4			rows;

	/* reset current values */
	tb->tb_fld->tflastrow = 0;
	tb->tb_fld->tfcurrow = 0;
	tb->tb_fld->tfcurcol = 0;

	/*
	**  Clear away the dataset attribute flags from the cells
	**  in the table field.
	*/
	tf = tb->tb_fld;
	flags = tf->tffflags;
	rows = tf->tfrows;
	cols = tf->tfcols;
	for (i = 0; i < rows; i++)
	{
		for (j = 0; j < cols; j++)
		{
			pflag = flags + (i * cols) + j;

			/*
			**  Only do real work if attributes for
			**  the cell are actually set.
			*/
			if (*pflag & dsALLATTR)
			{
				*pflag &= ~dsALLATTR;
				FTsetda(frm, tf->tfhdr.fhseq, FT_UPDATE,
					FT_TBLFLD, j, i, 0);
			}
		}
	}

	/* clear display and data areas */
	FDtbltrv(tb->tb_fld, FDclr, FDALLROW);
	FDstbltrv(tb->tb_fld, FDdat, FDALLROW);
	FDtbltrv(tb->tb_fld, IIFDccb, FDALLROW);

	tb->tb_rnum = 0;
	tb->tb_display = 0;

	if ( (ds = tb->dataset) == NULL)
		return (TRUE);

	origfld = &(IIstkfrm->fdrunfrm->frres2->origfld);
	if (tb->tb_fld->tfhdr.fhseq == *origfld)
	{
	    /* Illegal sequence number for the 'current' field--force an EA. */
	    *origfld = BADFLDNO;
	}

	IIfree_ds(ds);
	/* if using an append type table leave a fake row around for user */
	if (tb->tb_mode == fdtfAPPEND || tb->tb_mode == fdtfQUERY)
	{
	    IIscr_fake(tb, ds);
	}
	else if (tb->tb_mode == fdtfUPDATE) /* Turn on READ mode */
	{
	    tb->tb_fld->tfhdr.fhdflags &= ~fdtfUPDATE;
	    tb->tb_fld->tfhdr.fhdflags |= fdtfREADONLY;
	    IIFDiaaInvalidAllAggs(IIstkfrm->fdrunfrm, tb->tb_fld);
	}
	else
	{
	    IIFDiaaInvalidAllAggs(IIstkfrm->fdrunfrm, tb->tb_fld);
	}
	return (TRUE);				
}
