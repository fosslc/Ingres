/*
**	iitbinsrt.c
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
** Name:	iitbinsrt.c
**
** Description:
**
**	Public (extern) routines defined:
**		IItinsert()
**	Private (static) routines defined:
**		ins_tb()
**		ins_display()
**		ins_ds()
**
** History:
**	08/14/87 (dkh) - ER changes.
**	17-apr-89 (bruceb)
**		Initialize IITBccvChgdColVals to FALSE.
**		Checked in IITBceColEnd().
**	08/17/93 (dkh) - Fixed bug 53310.  A newly inserted row will
**			 no longer pick up display attributes from
**			 the row that it replaced.
**	04/15/97 (cohmi01)
**	    Keep track of ordinal position in dataset. (b81574)
**	31/12/97 (kitch01)
**		Bug 87940. After succesful insertion of a row call
**		FDrsbRefreshScrollButton so that MWS client can keep track
**		of the changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

static bool	ins_tb();		/* bare table updates */
static bool	ins_ds();		/* data set insertions */
static bool	ins_display();		/* raw display insertions */

GLOBALREF	TBSTRUCT	*IIcurtbl;

GLOBALREF	bool	IITBccvChgdColVals;

/*{
** Name:	IItinsert	-	Insert row into tablefield
**
** Description:
**	Insert a row after the 'current' row in a tablefield
**
**	If no data set is involved then a simple insertion is executed.
**	If a data set is being used then a scroll may be required if
**	the newly inserted row is put between two valid rows.  For
**	both types of tables, first the row/record management is done,
**	and at the end the actual execution of the sub-scroll using
**	FDcopytab() is done.
** 
**	Both the current row, the current column and the lastrow of
**	the frame's table field are updated to allow access to the 
**	newly inserted row. 
**	tf->tfcurrow = new row num,
**	tf->tfcurcol = first column,
**	tf->tflastrow is incremented if neccessary.
**
**	If a data set is linked to the current table and the mode is APPEND
**	then check to see if the last displayed row is the fake append type
**	row (that the runtime user has no knowledge of being on!)  If so
**	update the record pointers so that the insertion takes its place,
**	and does not follow it.
**
** Inputs:
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
**	26-apr-1983 	- Written (ncg)
**	30-apr-1984	- Improved interface to FD routines (ncg)
**	22-aug-1985	- Initial UPDATE mode is READ mode. (ncg)
**	15-apr-1997 (cohmi01)
**	    Keep track of ordinal position in dataset. (b81574)
**	
*/

IItinsert()
{

	register TBSTRUCT	*tb;	/* current table field */
	register DATSET		*ds;	/* current data set */
	i4 retval;

	IITBccvChgdColVals = FALSE;
	tb = IIcurtbl;

	if ((ds = tb->dataset) == NULL)
	{
		/* bare table insertion */
		return ((i4)ins_tb(tb));
	}
	/* 
	** Data set linked.
	** First check special APPEND type cases. User's are not aware of
	** the fake row that the cursor may be on, so if an insertion is
	** done before or after an empty fake row, then delete it and start
	** from the previous row. 
	*/
	if (tb->tb_mode == fdtfAPPEND)
	{
		/* 
		** If the current row number to insert after is 0 or 1
		** and the only row is the empty fake row then delete it
		** and reset the data set to an empty one.
		*/
		if ( (tb->tb_rnum == 0 || tb->tb_rnum == 1) &&
		      tb->tb_display == 1)
		{
			/* retrieve the top row and check if still empty */
			IIretrow(tb, (i4) fdtfTOP, ds->distop);
			/* is it still empty */
			if (ds->distop->rp_state == stNEWEMPT)
			{
				/* reset values so we load into first row */
				tb->tb_display--;
				ds->ds_records--;
				ds->distopix = 0;
				tb->tb_rnum = 0;
				IIt_rfree(ds, ds->distop);
				ds->top = NULL;
			}
		}
		/* 
		** Is the user inserting after the last row which may be the 
		** fake append type row ? 
		*/
		else if (tb->tb_rnum == tb->tb_display &&
			 tb->tb_display <= tb->tb_numrows &&
			 ds->disbot == ds->bot)
		{
			/* retrieve the bottom row and check if still empty */
			IIretrow(tb, tb->tb_display -1, ds->disbot);
			/* is it still empty */
			if (ds->disbot->rp_state == stNEWEMPT)
			{
				/* reset values so we load into previous row */
				tb->tb_display--;
				ds->ds_records--;
				tb->tb_rnum--;
				ds->disbot = ds->disbot->prevr;
				ds->bot = ds->bot->prevr;
				IIt_rfree(ds, ds->disbot->nxtr);
				ds->disbot->nxtr = NULL;
			}
		}
	}		
	/* execute data set insertion */
	retval = (i4)ins_ds(tb, ds);
	if (retval)
		FDrsbRefreshScrollButton(tb->tb_fld, ds->distopix, ds->ds_records);

	return (retval);
}


/*{
** Name:	ins_tb		-	Insert row into 'bare' tblfld
**
** Description:
**	Insert a row into a tablefield that does not have a linked
**	dataset.  This routine is INTERNAL to tbacc.
**
** Inputs:
**	tb		Ptr to tablefield structure
**
** Outputs:
**
** Returns:
**	bool	TRUE
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

static bool 
ins_tb(tb)
register TBSTRUCT	*tb;
{
	/* flag to update frame's table field's tflastrow */
	i2		uplastrow = 0;

	if (tb->tb_rnum <= tb->tb_display)
	{
		/* may need to update display - bottom of table */
		if (tb->tb_display < tb->tb_numrows)
		{
			/*
			** If display becomes 1 then the frame driver will 
			** leave tf->tflastrow as 0, so do not increment.
			*/
			if (++tb->tb_display > 1)
				uplastrow++;
		}
	}
	/* may need to update display because of new row */
	else if (tb->tb_display < tb->tb_numrows)
	{
		tb->tb_display = min(tb->tb_numrows, tb->tb_rnum + 1);
		/*
		** tfld->tflastrow may need to be incremented by more than just
		** 1, but INSERTROW's following IItsetcol() which calls 
		** FDputcol() will give the final update to tfld->tflastrow. 
		** This confusion only exists with "bare" table fields, where 
		** rows can be inserted anywhere in the display. 
		** If display becomes 1 then the frame driver will leave 
		** tf->tflastrow as 0, so do not increment.
		*/
		if (tb->tb_display > 1)
			uplastrow++;
	}
	return (ins_display(tb, uplastrow, (TBROW *) NULL));
}


/*{
** Name:	ins_display	-	Insert row into tblfld display
**
** Description:
**	Insert into the frame representation of the tablefield.  This
**	routine is INTERNAL to tbacc.
**
**	If any scrolls need be executed then  FDcopytab will do them.
**	If no scroll is required then FDcopytab()
**	should return immediately, without doing any data transfer.  If any
**	rows were scrolled then the "start" row is cleared in FDcopytab().
**
**	Updates tf->tfcurrow to newly inserted row,
**	and tf->tfcurcol to the first column.
**	May also update tf->tflastrow to current tb->tb_display.
**
**	If routine is passed a non-NULL TBROW pointer(rp) , then 
**	check to see if that row has any display attributes set.
**	If so, need to unset it so that the newly inserted row
**	does not inherit the attributes when displayed.
**
** Inputs:
**	tb		Ptr to the tablefield descriptor
**	uplast		Flag - update tf->tflastrow.
**	rp		Pointer to dataset data of row that was
**			displaced by an inserted row.
**
** Outputs:
**
** Returns:
**	bool	TRUE
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

static bool 
ins_display(tb, uplast, rp)
register TBSTRUCT	*tb;	
i2			uplast;			/* update tf->tflastrow */
TBROW			*rp;
{
	TBLFLD		*tfld;			/* frame's table field */
	i4		*pflags;
	i4		dsflags;
	i4		*dspflags;
	i4		i;
	PTR		rowdata;
	TBLFLD		*tbl;
	COLDESC		*cd;
	i4		screen_row;

	if (tb->tb_rnum == tb->tb_numrows)
	{
		/* insert at bottom of table, and scroll UPWARDS */
		FDcopytab(tb->tb_fld, (i4) fdtfTOP, (i4) fdtfBOT);
		FDclrrow(tb->tb_fld, (i4) fdtfBOT);
		screen_row = tb->tb_fld->tfrows - 1;
	}
	else
	{
		/* upadate for new insertion */
		tb->tb_rnum++;	
		/*
		** Scroll DOWNWARDS from current row to bottom of display to
		** allow insertion at current row.
		*/
		FDcopytab(tb->tb_fld, (i4) (tb->tb_display -1), (i4) (tb->tb_rnum -1));
		FDclrrow(tb->tb_fld, tb->tb_rnum -1);
		screen_row = tb->tb_rnum - 1;
	}

	/*
	**  Check to see if we need to turn off any visible display
	**  attributes from dataset row that was displayed in the
	**  table field row that new row will be occupying.
	*/
	if (rp != NULL)
	{
		tbl = tb->tb_fld;
		rowdata = rp->rp_data;
		cd = tb->dataset->ds_rowdesc->r_coldesc;
		for (i = 0; i < tbl->tfcols; i++, cd = cd->c_nxt)
		{
			dspflags = (i4 *)(((char *) rowdata + cd->c_chgoffset));

			/*
			**  If column in row (that used to occupy the
			**  display area of inserted row) had any
			**  attributes set, then unset the attributes.
			*/
			if (*dspflags & dsALLATTR)
			{
				pflags = tbl->tffflags +
					(screen_row * tbl->tfcols) + i;
				*pflags &= ~dsALLATTR;
				FTsetda(IIfrmio->fdrunfrm, tbl->tfhdr.fhseq,
					FT_UPDATE, FT_TBLFLD, i, screen_row, 0);
			}
		}
	}

	/*
	** Get real frame table field, and update table field values.
	** '-1' for frame arithmatic.
	*/
	tfld = tb->tb_fld;
	tfld->tfcurrow = tb->tb_rnum -1;
	tfld->tfcurcol = 0;
	if (uplast)
	{
		/* tflastrow uses C arithmatic, thus the "-1" */
		if (tfld->tflastrow < tfld->tfrows -1)
		{
			/*
			** The reason we assign it tb_display -1 and not just
			** use tflastrow++ is because of the special append
			** case in the main routine.  When we found a last row
			** that was the append type row, we decremented
			** tb_display, but not tflastrow -- thus we may have
			** been in an inconsistent state.  This restores the
			** frame's and local value of tf->lastrow/tb_display.
			*/
			tfld->tflastrow = tb->tb_display -1;
		}
	}
	return (TRUE);
}


/*{
** Name:	ins_ds		-	Insert dataset ptrs for row insertion
**
** Description:
**	Update data set record pointers to allow for insertion.
**
** Inputs:
**	tb		Ptr to tablefield descriptor
**	ds		Ptr to dataset descriptor for the tablefield
**
** Outputs:
**
** Returns:
**	bool	TRUE
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

static bool 
ins_ds(tb, ds)
register TBSTRUCT	*tb;			/* current table field */
register DATSET		*ds;			/* current data set */
{
	register TBROW	*rp;			/* row pointer for scanning */
	TBROW		*old_row = NULL;
	i4		i;
	/* flag to update frame's table field's tflastrow */
	i2		uplastrow = 0;

	/* request to insert at the top of display */
	if (tb->tb_rnum == 0)
	{
		if (ds->top == NULL)		/* list is empty */
		{
			/* start off list and update top and bot as same */
			IIdsinsert(tb, ds, (TBROW *) NULL, (TBROW *) NULL,
			    stUNCHANGED);
			ds->distop = ds->disbot = ds->top;
			ds->distopix = 1;
			/* Turn off READ mode */
			if (tb->tb_mode == fdtfUPDATE)
			{
			    tb->tb_fld->tfhdr.fhdflags &= ~fdtfREADONLY;
			    tb->tb_fld->tfhdr.fhdflags |= fdtfUPDATE;
			}
		}
		else
		{
			/*
			** Insert between top and previous record. If the top
			** one was the last one then ds->top will be updated
			** in IIdsinsert() and ds->distop will always have
			** a record behind it.
			*/
			IIdsinsert(tb, ds, ds->distop->prevr, ds->distop,
			    stUNCHANGED);

			/*
			**  Save handle to row that is being bumped
			**  by inserted row.
			*/
			old_row = ds->distop;

			ds->distop = ds->distop->prevr;
			ds->distopix--;    /* keep track of ordinal position */
		}
		/* can new row can be displayed without scrolling OUT data */
		if (tb->tb_display < tb->tb_numrows)
		{
			/*
			** If display becomes 1 then the frame driver will 
			** leave tf->tflastrow as 0, so do not increment.
			*/
			if (++tb->tb_display > 1)
				uplastrow++;
		}
		else
		{
			/* retrieve bottom row before scrolling out */
			IIretrow(tb, (i4) fdtfBOT, ds->disbot);
			ds->disbot = ds->disbot->prevr;
		}
		/* record for following IItsetcol() */
		ds->crow = ds->distop;			
	}
	else if (tb->tb_rnum == tb->tb_display)
	{
		/* 
		** Insert betwen bottom and next row.  Same comment applies 
		** for ds->bot as dis above for ds->top.
		*/
		IIdsinsert(tb, ds, ds->disbot, ds->disbot->nxtr, stUNCHANGED);

		/*
		**  Save handle to row that is being bumped by inserted row.
		*/
		old_row = ds->disbot;

		ds->disbot = ds->disbot->nxtr;
		if (tb->tb_display < tb->tb_numrows)
		{
			/*
			** If display becomes 1 then the frame driver will 
			** leave tf->tflastrow as 0, so do not increment.
			*/
			if (++tb->tb_display > 1)
				uplastrow++;
		}
		else
		{
			/* retrieve top row before it is scrolled out */
			IIretrow(tb, (i4) fdtfTOP, ds->distop);
			ds->distop = ds->distop->nxtr;
			ds->distopix++;    /* keep track of ordinal position */
		}
		/* set for later IItsetcol() */
		ds->crow = ds->disbot;
	}
	else
	{
		/* scan till reach current row */
		for (i = 1, rp = ds->distop; i < tb->tb_rnum && rp != NULL;
		     i++, rp = rp->nxtr)
			;

		/*
		**  Save off pointer to row that is being bumped by
		**  new row being inserted.
		*/
		old_row = rp->nxtr;

		/* insert between current row and next row */
		IIdsinsert(tb, ds, rp, rp->nxtr, stUNCHANGED);
		if (tb->tb_display < tb->tb_numrows)
		{
			/*
			** If display becomes 1 then the frame driver will 
			** leave tf->tflastrow as 0, so do not increment.
			*/
			if (++tb->tb_display > 1)
				uplastrow++;
		}
		else
		{
			/* retrieve bottom row before it is scrolled out */
			IIretrow(tb, (i4) fdtfBOT, ds->disbot);
			ds->disbot = ds->disbot->prevr;
		}
		/* set for later IItsetcol() */
		ds->crow = rp->nxtr;
	}

	ds->ds_records++;			/* new record in list */
	tb->tb_state = tbINSERT;		/* flag for IItsetcol() */
	return (ins_display(tb, uplastrow, old_row));
}
