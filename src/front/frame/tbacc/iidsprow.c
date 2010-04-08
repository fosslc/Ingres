/*
**	iidisprow.c
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
# include	<frserrno.h>
# include       <er.h>

/**
** Name:	iidisprow.c
**
** Description:
**	Display a tablefield dataset row in the tablefield
**
**	Public (extern) routines defined:
**		IIdisprow()
**	Private (static) routines defined:
**
** History:
**	18-feb-1987	Made changes for ADT support (drh)
**	08/14/87 (dkh) - ER changes.
**	09/16/87 (dkh) - Integrated table field change bit.
**	11/19/87 (dkh) - Fixed change bit updates for row fdtfTOP and fdtfBOT.
**	11/20/87 (dkh) - The change of 11/19/87 also fixes jup bug 1337.
**	08/12/88 (dkh) - Fixed DG reported bug 79.
**	02/19/89 (dkh) - Put in change to make sure rows scrolled
**			 into view will be validated.
**	19-jul-89 (bruceb)
**		Flag region in dataset is now an i4.  Change bit code now
**		changed to deal with fdI_CHG, fdX_CHG, fdVALCHKED.
**	04/20/90 (dkh) - Fixed us #196 so users can get off newly
**			 opened rows without being trapped by validations.
**	07/27/90 (dkh) - Added support for table field cell highlighting.
**	02/19/91 (dkh) - Added support for 4gl table field qualification.
**	07/15/92 (dkh) - Added support for edit masks.
**	02/05/92 (dkh) - Changed interface to IIdisprow() so that we can
**			 get row state information as well.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Update prototypes for gcc 4.3.
**/

FUNC_EXTERN	FLDCOL	*FDfndcol();
FUNC_EXTERN	VOID	FDftbl();

/*{
** Name:	IIdisprow	-	Display tf dataset row
**
** Description:
**	Provided with the tablefield row number to update, loops
**	through all columns calling FDputcol (and FDinsqcol, if
**	query mode) to put the data from the dataset record onto
**	the frame.
**
** Inputs:
**	tb		Ptr to the tablefield to update
**	row		Row number in the tablefield to update
**	rp		Ptr to a TBROW structure describing the row to display
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
**		04-mar-1983 	- written (ncg)
**		12-jan-1984	- added query mode (ncg)
**		30-apr-1984	- Improved interface to FD routines (ncg)
**		19-feb-1987	- Changed dataset row ptr parameter to PTR,
**				  change FDputcol, and FDinsqcol calls to
**				  pass DB_DATA_VALUES (drh)
**	06/11/87 (dkh) - Changed to deal with query ops properly.
*/

i4
IIdisprow(TBSTRUCT *tb, i4 row, TBROW *rp)
{
	register ROWDESC	*rd;		/* row descriptor */
	register COLDESC	*cd;		/* column descriptor */
	DB_DATA_VALUE		opdbv;		/* dbv for the query op */
	register i4		i;
	i4			*opptr;
	i4			*dspflags;
	i4			*rowinfo;
	FLDCOL			*col;
	i4			*pflags;
	i4			dsflags;
	i4			oflags;
	FRAME			*frm;
	i4			fldno;
	PTR			rowdata;

	FDftbl(tb->tb_fld, &frm, &fldno);
	if (frm == NULL)
	{
		IIFDerror(TBLNOFRM, 0, (char *) NULL);
		return (FAIL);
	}

	rowdata = rp->rp_data;

	/*
	**  Initialize the DB_DATA_VALUE for the query operator 
	*/

	opdbv.db_datatype = DB_INT_TYPE;
	opdbv.db_length = sizeof(i4);
	opdbv.db_prec = 0;

	/* for each column in the row display the info */

	for (i = 0, rd = tb->dataset->ds_rowdesc, cd = rd->r_coldesc; 
	     i < rd->r_ncols; i++, cd = cd->c_nxt)
	{
		cd->c_dbv.db_data = (PTR) (((char *) rowdata)+ cd->c_offset );
		FDputcol(tb->tb_fld, row, cd->c_name, &cd->c_dbv );

		if (tb->tb_mode == fdtfQUERY)
		{
			opptr = (i4 *) (((char *) rowdata) +
						cd->c_qryoffset );
			FDinsqcol(tb->tb_fld, row, cd->c_name, *opptr,
				(FLDVAL *)NULL, (FLDCOL *)NULL);
		}
		if (row == fdtfTOP)
		{
			row = 0;
		}
		else if (row == fdtfBOT)
		{
			row = tb->tb_fld->tfrows - 1;
		}
		else if (row == fdtfCUR)
		{
			row = tb->tb_fld->tfcurrow;
		}

		/*
		**  Set validated and change bits in the table field
		**  row from the values in the dataset row.  This will
		**  ensure that unvalidated values get validated, but
		**  that no unnecessary validation will occur.
		*/
		dspflags = (i4 *) (((char *) rowdata) + cd->c_chgoffset);
		IIFDtccb(tb->tb_fld, row, cd->c_name);
		IIFDtscb(tb->tb_fld, row, cd->c_name,
			(*dspflags & (fdI_CHG | fdX_CHG | fdVALCHKED)), TRUE);

		/*
		**  Now pushd any display attributes from the dataset
		**  into the table field.
		**
		**  No need to check return value of FDfndcol since
		**  everything has been checked before we get here.
		*/
		col = FDfndcol(tb->tb_fld, cd->c_name);
		pflags = tb->tb_fld->tffflags + (row * tb->tb_fld->tfcols) +
			col->flhdr.fhseq;
		/*
		**  Clear away any old dataset attributes
		*/
		oflags = *pflags & dsALLATTR;
		*pflags &= ~dsALLATTR;

		/*
		**  Now set new attributes.
		*/
		dsflags = *dspflags & dsALLATTR;
		*pflags |= dsflags;

		/*
		**  Clear away old tfDISP_EMPTY attribute.
		*/
		*pflags &= ~tfDISP_EMPTY;

		/*
		**  Now set tfDISP_EMPTY if dsDISP_EMPTY is set.
		*/
		if (*dspflags & dsDISP_EMPTY)
		{
			*pflags |= tfDISP_EMPTY;
		}

		/*
		**  If the attributes have changed for the
		**  window, then call FTsetda() to make
		**  the change visible.
		*/
		if (dsflags || oflags)
		{
			/*
			**  Note that it doesn't matter what we pass as the last
			**  arg to FTsetda() since it will check the attributes
			**  set for the column and in tffflags to determine what
			**  to set.
			*/
			FTsetda(frm, tb->tb_fld->tfhdr.fhseq, FT_UPDATE,
				FT_TBLFLD, col->flhdr.fhseq, row, 0);
		}
		rowinfo = tb->tb_fld->tffflags + (row * tb->tb_fld->tfcols);
		*rowinfo &= ~(fdtfFKROW | fdtfRWCHG);
		if (rp != NULL && rp->rp_state == stNEWEMPT)
		{
			*rowinfo |= fdtfFKROW;
		}

	}
	return (TRUE);
}
