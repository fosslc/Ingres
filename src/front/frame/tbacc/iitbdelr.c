/*
**	iitbdelr.c
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
# include	<rtvars.h>
# include       <er.h>

/**
** Name:	iitbdelr.c	-	Delete tblfld row
**
** Description:
**
**	Public (extern) routines defined:
**		IItdelrow()
**	Private (static) routines defined:
**		tb_delrow()
**		ds_delrow()
**
** History:
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	12/23/87 (dkh) - Performance changes.
**	04/06/88 (dkh) - Changed the deleterow statement semantics
**			 so that it does not perform a IIretrow()
**			 call before deleting the dataset row.
**	23-mar-89 (bruceb)
**		If table field containing the row being deleted is the
**		field user was last on (the current field as of return to
**		user code from the FRS), and dataset row is the current
**		row (don't do anything for bare table fields) reset origfld
**		to bad value to force an entry
**	03-aug-89 (bruceb)
**		Invalidate any derivation aggregate columns.
**		Call IIFDndcNoDeriveClr() before FDclrrow() when a dataset
**		exists--wish FDclrrow to just wipe everything (including
**		constant derivations.)
**	02/05/92 (dkh) - Updated to handle interface change to IIdisprow().
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	15-apr-1997 (cohmi01)
**	    Keep track of ordinal position in dataset. (b81574)
**	31-dec-1997 (kitch01)
**		Bug 87939. After a succesfull deletion of a row from a dataset
**		call FDrsbRefreshScrollButton so that MWS client can keep track
**		of the changes.
**	15-jan-1998 (kitch01)
**		Bug 88208. When trying to bring in new record from above after
**		deleting the fake row decrease the oridinal position. ds_delrow().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

GLOBALREF	TBSTRUCT	*IIcurtbl;

FUNC_EXTERN	VOID	IIFDiaaInvalidAllAggs();
FUNC_EXTERN	VOID	IIFDndcNoDeriveClr();

static	i4	tb_delrow();
static  i4	ds_delrow();

/*{
** Name:	IItdelrow	-	Delete row from tablefield
**
** Description:
**	Delete a row from a tablefield.  A flag specifies whether
**	there is a pending EQUEL 'IN' list waiting.  The IN list 
**	is not allowed if the tablefield is linked to a dataset.
**
**	This routine is part of TBACC's external interface.
**	
** Inputs:
**	tofill		Flag: is EQUEL IN list waiting
**
** Outputs:
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Example and Code Generation:
**	## deleterow form1 tbl1 1
**
**	if (IItbsetio(4,"form1","tbl1",1) != 0 )
**	{
**		if ( IItdelrow(0) != 0 )
**		{
**		}
**	}
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	04-mar-1983 	- written (ncg)
**	30-apr-1984	- Improved interface to FD routines (ncg)
**	14-jun-1985	- Initial UPDATE mode is READ mode. (ncg)
**	
*/

i4
IItdelrow(i4 tofill)
{
	i4	retval;

	if (IIcurtbl->dataset == NULL)
	{
		/* delete row of regular table */
		return (tb_delrow(IIcurtbl, tofill));
	}

	/* current table is linked to data set */
	if (tofill)
	{
		/* cannot have IN list */
		IIFDerror(TBDRINLIST, 1, (char *) IIcurtbl->tb_name);
		return (FALSE);
	}
	retval = ds_delrow(IIcurtbl);
	if (retval)
		FDrsbRefreshScrollButton(IIcurtbl->tb_fld, IIcurtbl->dataset->distopix, IIcurtbl->dataset->ds_records);
	IIFDiaaInvalidAllAggs(IIfrmio->fdrunfrm, IIcurtbl->tb_fld);

	return (retval);
}	


/*{
** Name:	tb_delrow	-	Delete row in tf w/o dataset
**
** Description:
**	Deletes a row from a tablefield that is not linked to a dataset.
**	This routine is INTERNAL to tbacc.
**
**	Using FDcopytab() it makes sure that the displayed row is 
**	overwritten.  If no filler is waiting it decrements the number
**	of displayed rows, otherwise the row to be filled is the last 
**	row.
** 
**	Updates tf->tflastrow in frame's table field.
**
** Inputs:
**	tb		Ptr to tablefield to delete from
**	tofill		EQUEL 'IN' list
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
**	
*/

static i4
tb_delrow(tb, tofill)
register TBSTRUCT	*tb;
i4			tofill;
{
	if (tb->tb_display == 0)
	{
		return (FALSE);
	}
	/* copy table up from bottom to current row , and blank out lastrow */
	FDcopytab(tb->tb_fld, (i4) (tb->tb_rnum -1),
		(i4) (tb->tb_display - 1));
	FDclrrow(tb->tb_fld, tb->tb_display - 1);
	if (tofill)
	{
		tb->tb_rnum = tb->tb_display;		/* row to be filled */
	}
	else
	{
		/* update last row */
		FDlastrow(tb->tb_fld);
		tb->tb_display--;
	}
	return (TRUE);
}


/*{
** Name:	ds_delrow	-	Delete row in tf with dataset
**
** Description:
**	Delete a row in a tablefield linked to a dataset.  This routine
**	is INTERNAL to tbacc.
**
**	Retrieve the row back into the current row of the data set,
**	so that the most recent information is stored (for later
**	unloading). Based on the position of the displayed row, and
**	the number of records not currently diplayed a scroll may be
**	required.
**
**	Once the row is deleted, update all links. This is a little
**	confusing, but the only way to find all the special cases is by
**	going through by hand and enumerating the cases.
** 
**	Updates tf->tflastrow in frame's table field.
**
** Inputs:
**	tb		Ptr to tablefield to delete row from
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
**	18-feb-1987	Changed rp_rec reference to rp_data (drh)
**	15-apr-1997 (cohmi01)
**	    Keep track of ordinal position in dataset. (b81574)
**	15-jan-1998 (kitch01)
**		Bug 88208. When trying to bring in new record from above after
**		deleting the fake row decrease the oridinal position.
**	
*/

static i4
ds_delrow(tb)
register TBSTRUCT	*tb;
{
	register TBROW	*cr;				/* current row */
	register DATSET	*ds;				/* current data set */
	FRRES2		*eastruct;

	ds = tb->dataset;

	/* 
	** This can happen if one tries to DELETEROW a deleted row in an
	** UNLOADTABLE loop. But even then it should not get here!
	*/
	if (ds->crow->rp_state == stDELETE)
		return (FALSE);


	/*
	**  Removed call to IIretrow(tb, tb->tb_rnum - 1, ds->crow);
	**  so a table field row can always be deleted.  Even if
	**  invalid data has been entered.
	*/

	/* can bring in next record from below */
	if (ds->disbot != ds->bot)
	{
		/* delete the displayed rep of the row--subscroll */
		FDcopytab(tb->tb_fld, (i4) (tb->tb_rnum - 1), (i4) fdtfBOT);
		/* display new display bottom of data set */
		ds->disbot = ds->disbot->nxtr;
		IIdisprow(tb, (i4) fdtfBOT, ds->disbot);
	}
	/* try to bring in record from above */
	else if (ds->distop != ds->top)
	{
		/* delete the displayed rep of the row--subscroll */
		FDcopytab(tb->tb_fld, (i4) (tb->tb_rnum - 1), (i4) fdtfTOP);
		/* display new display top of data set */
		ds->distop = ds->distop->prevr;
		/* Bug 88208 */
		ds->distopix--;	   /* keep track of ordinal position */
		IIdisprow(tb, (i4) fdtfTOP, ds->distop);
	}
	/* nothing to replace with, update last row number */
	else 	
	{
		FDcopytab(tb->tb_fld, (i4) (tb->tb_rnum - 1),
		    (i4) (tb->tb_display -1));
		IIFDndcNoDeriveClr();
		FDclrrow(tb->tb_fld, tb->tb_display - 1);
		FDlastrow(tb->tb_fld);
		tb->tb_display--;
	}
	/* get just deleted current record */
	cr = ds->crow;

	/* bypass current row if in middle of data set */
	if (cr->nxtr != NULL)
		cr->nxtr->prevr = cr->prevr;	
	if (cr->prevr != NULL)
		cr->prevr->nxtr = cr->nxtr;

	/*
	 * Current row is pointing at display top -- can only happen if could
	 * either bring in from below, or nothing to bring in. This was added
	 * to avoid losing the distop to the delete list, as did happen.
	 */
	if (cr == ds->distop)
	{
		ds->distop = cr->nxtr;
	}
	/* current row is top of data set */
	if (cr == ds->top)
	{
		ds->top = cr->nxtr;
	}
	/* current row is last in data set */
	if (cr == ds->bot) 
	{
		ds->bot = cr->prevr;
		ds->disbot = cr->prevr;
	}
	IIt_rdel(ds, ds->crow);		/* put row on deleted list */
	ds->ds_records--;

	/* Set up for an entry activation. */
	eastruct = IIfrmio->fdrunfrm->frres2;
	if ((tb->tb_fld->tfhdr.fhseq == eastruct->origfld)
	    && (cr == (TBROW *)eastruct->origdsrow))
	{
	    eastruct->origfld = BADFLDNO;
	}

	/*
	 * if using an append type table, and no more rows left then need
	 * to display fake row.
	 * if update mode and no rows left then use read mode.
	 */
	if (ds->top == NULL)
	{
		if (tb->tb_mode == fdtfAPPEND || tb->tb_mode == fdtfQUERY)
		{
		    IIscr_fake(tb, ds);
		}
		else if (tb->tb_mode == fdtfUPDATE)
		{
		    tb->tb_fld->tfhdr.fhdflags &= ~fdtfUPDATE;
		    tb->tb_fld->tfhdr.fhdflags |= fdtfREADONLY;
		}
	}
	return (TRUE);
}
