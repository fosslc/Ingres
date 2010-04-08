/*
**	iitbsetio.c
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
# include	<er.h>
# include	<rtvars.h>

/**
** Name:	iitbsetio.c	-	Set up for EQUEL I/O stmt
**
** Description:
**	Validate that subsequent I/O commands can be executed, and
**	set up row numbers and global ptrs for i/o.
**
**	Public (extern) routines defined:
**		IItbsetio()
**	Private (static) routines defined:
**		row_range()
**		row_to_ds()
**		ds_to_row()
**		row_cursor()
**
** History:
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	09/16/87 (dkh) - Integrated table field change bit.
**	04/06/88 (dkh) - Fixed deleterow semantics so that it does
**			 not validate the row being deleted.
**	11/10/88 (dkh) - Fixed jup bug 3582.
**      19-jul-89 (bruceb)
**              Flag region in dataset is now an i4.  IITBicb/IITBscb
**              changed to handle this.
**	05/20/90 (dkh) - Turn off special fake row bits if a putform is done.
**	07/22/90 (dkh) - Added support for table field cell highlighting.
**	07/18/93 (dkh) - Added support for the purgetable statement.
**	04/25/96 (chech02)
**		Added function type i4  to IItbsetio() for windows 3.1 port.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

static i4	row_range();				/* row within range  */
static i4	row_to_ds();				/* current ds record */
static bool	ds_to_row();				/* ds record to row  */
static i4	row_cursor();				/* curr cursor row   */

static	i4	dstofrm_attr();
static	i4	frmtods_attr();

GLOBALREF	TBSTRUCT	*IIcurtbl;

FUNC_EXTERN	i4		IItvalval();
FUNC_EXTERN	COLDESC		*IIcdget();
FUNC_EXTERN	FLDCOL		*FDfndcol();
FUNC_EXTERN	i4		RTrcdecode();



/*{
** Name:	IItbsetio	-	Validate for i/o, set globals
**
** Description:
**	Validate general table and frame I/O (IItstart() ) and get
**	back a table structure with the given name.  Based on the 
**	"rownum" update the current row number.  If a data set is 
**	linked to the table point at the current row record.
**
**	If the table is an APPEND or UPDATE type table, then validation
**	checks are required when scrolling, deleting and getting. This
**	may be full table field validation -- FDckffld(), or single
**	row validation -- FDckrow().
**
**	This routine is part of TBACC's external interface.
**	
** Inputs:
**	cmnd		Code indicating type of i/o request
**	formname	Name of the form
**	tabname		Name of the tablefield
**	rownum		Row number
**
** Outputs:
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Example and Code Generation:
**	## getrow form1 tbl1 3 (buf=col1)
**
**	if (IItbsetio(3,"form1","tbl1",-2) != 0 )
**	{
**		IItcolret(1, 32, 0, buf, "col1");
**	}
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	04-mar-1983 	- Written (ncg)
**	30-apr-1983	- Modified to work independently for each type
**			  of calling command, and to add validation
**			  checks.  More code, but much clearer. (ncg)
**	12-jan-1984	- Commented out FDckrow(), done in FDgetcol().
**	30-apr-1984	- Improved interface to FD routines (ncg)
**	03-jul-1984	- Added putrow within unload context (ncg)
**	13-jun-1985	- Made some commands go through FDckffld
**			  for empty "fake-row" problem (ncg)
**
*/

i4
IItbsetio(i4 cmnd, const char *formname, const char *tabname, i4 rownum)
{
	register TBSTRUCT	*tb;
	i4			tmprow = rownum;
	i4			i;
	TBLFLD			*tfld;
	i4			*rowinfo;
	
	/* update current table, and I/O frame */
	if ( (tb = IItstart(tabname, formname) ) == NULL)
	{
			return (FALSE);
	}

	tb->tb_state = tbDISPLAY;

	switch (cmnd)
	{
	  case cmSCROLL:		/* ## SCROLL */

		if (tb->tb_display == 0)
		{
			return (FALSE);
		}
		/*
		** If scrolling data from APPEND or UPDATE type displays, then
		** make sure that data in the display is put into data windows.
		** This can be left out for non APPEND type tables, as the data
		** is always checked when put there.
		*/
		if (tb->tb_mode != fdtfREADONLY)
		{
			if (!FDckffld(IIfrmio->fdrunfrm, tb->tb_name))
			{
				IIsetferr((ER_MSGID) 1);
				return (FALSE);
			}
		}
		break;
	
	  case cmGETR:			/* ## GETROW */

		/* if fetching a row check that the table is not empty */
		if (tb->tb_display == 0)
		{
			/* cannot fetch row from empty table */
			IIFDerror(TBEMPTGR, 1, (char *) tb->tb_name);
			return (FALSE);
		}
		if (tmprow == rowCURR)    
		{
			/* set current row to cursor row */
			if ((tmprow = row_cursor(IIfrmio, tb)) < 0)
			{
				return (FALSE);
			}
		}
		if ( (tb->tb_rnum = row_range(tb->tb_name, (i4) 1, tb->tb_display, tmprow)) < 0 )
		{
			return (FALSE);
		}

		/*
		** If fetching data from an APPEND or UPDATE type display, 
		** then validate the row and make sure the data is put into
		** the data windows. COMMENTED OUT, done in FDgetcol().
		if (tb->tb_mode != fdtfREADONLY)
		{
			if (!FDckrow(tb->tb_fld, tb->tb_rnum -1))
			{
				IIsetferr((ER_MSGID) 1);
				return (FALSE);
			}
		}
		*/

		/*
		** Update data set record -- may be hidden column.
		*/
		row_to_ds(tb->tb_rnum, tb);
		break; 

	  case cmPUTR:			/* ## PUTROW */

		if (tmprow == rowCURR)    
		{
			/* 
			** Putrow on current within an unloadtable.  Use the
			** cuurent data set record, if its not on the delete 
			** list.
			*/
			if (IIunldstate() == tb)
			{
				/* Delete list record (should be FDerror) */
				if (IIrec_num() == rowNONE)
				{
					return (FALSE);
				}
				/* Set state for following setcols */
				tb->tb_state = tbUNLDPUT;
				if (!ds_to_row(tb))
					return (FALSE);

				if (tb->tb_rnum != rowNONE)
				{
					tfld = tb->tb_fld;
					rowinfo = tfld->tffflags +
					    ((tb->tb_rnum - 1) * tfld->tfcols);
					*rowinfo &= ~(fdtfFKROW | fdtfRWCHG);
				}
				break;
			}
			/* set current row to cursor row */
			if ((tmprow = row_cursor(IIfrmio, tb)) < 0)
			{
				return (FALSE);
			}
		}
		if (tb->dataset == NULL)
		{
			/* allow to put anywhere in table display */
			if ( (tb->tb_rnum = row_range(tb->tb_name, (i4) 1, tb->tb_numrows, tmprow)) < 0 )
			{
				return (FALSE);
			}
			tb->tb_display = max(tmprow, tb->tb_display);
		}
		/* data set linked, treat as regular row */
		else 
		{
			if ( (tb->tb_rnum = row_range(tb->tb_name, (i4) 1, tb->tb_display, tmprow)) < 0 )
			{
				return (FALSE);
			}
			/*
			** Update data set record -- may be hidden column.
			*/
			row_to_ds(tb->tb_rnum, tb);

			tfld = tb->tb_fld;
			rowinfo = tfld->tffflags +
				((tb->tb_rnum - 1) * tfld->tfcols);
			*rowinfo &= ~(fdtfFKROW | fdtfRWCHG);
		}
		break;

	  case cmDELETER:			/* ## DELETEROW */

		/* if deleting a row check that the table is not empty */
		if (tb->tb_display == 0)
		{
			/* cannot delete row from empty table */
			IIFDerror(TBEMPTDR, 1, (char *) tb->tb_name);
			return (FALSE);
		}
		if (tmprow == rowCURR)    
		{
			/* set current row to cursor row */
			if ((tmprow = row_cursor(IIfrmio, tb)) < 0)
			{
				return (FALSE);
			}
		}
		if ( (tb->tb_rnum = row_range(tb->tb_name, (i4) 1,
			tb->tb_display, tmprow)) < 0 )
		{
			return (FALSE);
		}
		/*
		** When deleting data from a table field, a scroll may be 
		** executed to compact the data.  If using an APPEND or UPDATE
		** type display, then all the rows need to be validated, 
		** causing the correct data to be put into the data windows.
		*/
		if (tb->tb_mode != fdtfREADONLY)
		{
			for (i = 0; i < tb->tb_display; i++)
			{
				/*
				**  Skip the row to be deleted.
				*/
				if (i == tmprow - 1)
				{
					continue;
				}
				if (!FDckrow(tb->tb_fld, i))
				{
					IIsetferr((ER_MSGID) 1);
					return(FALSE);
				}
			}
		}
		row_to_ds(tb->tb_rnum, tb);
		break; 

	  case cmINSERTR:			/* ## INSERTROW */

		if (tmprow == rowCURR)    
		{
			/* set current row to cursor row */
			if ((tmprow = row_cursor(IIfrmio, tb)) < 0)
			{
				return (FALSE);
			}
		}
		if (tb->dataset == NULL)
		{
			/*
			** Allow to insert after any row.  Row 0 means insert
			** before top row.
			*/
			if (row_range(tb->tb_name, (i4) 0, tb->tb_numrows, tmprow) < 0 )
			{
				return (FALSE);
			}
		}
		/* data set linked, treat as regular row in 0..display */
		else if (row_range(tb->tb_name, (i4) 0, tb->tb_display, tmprow) < 0 )
		{
			return (FALSE);
		}
		tb->tb_rnum  = tmprow;
		/*
		** When inserting data into a table field, a sub-scroll may be 
		** executed to expand the data, requiring validation of the 
		** current data.
		** BUG 2284 - Do not validate when no rows. (ncg)
		*/
		if (tb->tb_mode != fdtfREADONLY && tb->tb_display >= 1)
		{
			if (!FDckffld(IIfrmio->fdrunfrm, tb->tb_name))
			{
				IIsetferr((ER_MSGID) 1);
				return (FALSE);
			}
		}
		break;

	  case cmCLEARR:				/* ## CLEARROW */
	  case cmVALIDR:				/* ## VALIDROW */

		if (tb->tb_display == 0)
			return (FALSE);

		if (tmprow == rowCURR)    
		{
			/* set current row to cursor row */
			if ((tmprow = row_cursor(IIfrmio, tb)) < 0)
			{
				return (FALSE);
			}
		}
		/*
		** Allow clearing and validation of any row.  (No data transfer
		** involved.)
		*/
		if ( (tb->tb_rnum = row_range(tb->tb_name, (i4) 1, tb->tb_numrows, tmprow)) < 0 )
		{
			return (FALSE);
		}
		if (cmnd == cmVALIDR)
			IItvalval((i4) TRUE);
		break;

	  case cmPURGE:
		if (!IITBptPurgeTable(tb))
		{
			return(FALSE);
		}
		break;

	  default:
	  {
		return (FALSE);
	  }

	}
	IIcurtbl = tb;
	return (TRUE);
}


/*{
** Name:	row_range	-	Validate that row num is legal
**
** Description:
**	If the row number is within legal range, return it to the
**	caller, else return -1.  Display error if row number is illegal.
**
**	This routine is INTERNAL to runtime.
**
** Inputs:
**	tbname		Tablefield name
**	lower		Lower bound of row range
**	upper		Upper bound of row range
**	rownum		Row number to check
**
** Outputs:
**
** Returns:
**	The row number, or -1 if it was not legal.
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
row_range(tbname, lower, upper, rownum)
char		*tbname;			/* current table name */
i4		lower;				/* range limits */
i4		upper;
i4	 	rownum;				/* row number to check */
{
	i4	rnum;

	if ( (rownum < lower) || (rownum > upper) )
	{
		/* bad row number used on call to table I/O routine */
		rnum = rownum;
		IIFDerror(TBROWNUM, 2, (char *) &rnum, tbname);
		return (-1);
	}
	return (rownum);
}

/*{
** Name:	row_to_ds	-	Map displayed rowno to dataset
**
** Description:
**	Provided with the row number from the display, update the
**	dataset descriptor so that it's current row is the the
**	same one.
**
** Inputs:
**	row		Display row number
**	tb		Tablefield descriptor
** Outputs:
**
** Returns:
**
** Exceptions:
**	none
**
** Side Effects:
**	Updates dataset->crow.
**
** History:
**	
*/

static i4
row_to_ds(row, tb)
i4		row;
TBSTRUCT	*tb;
{
	register i4	i;
	register TBROW	*cr;

	if (tb->dataset == NULL)	
		return;

	if ( (row > tb->tb_display) || (row < 1) )
	{
		/*
		** If it gets here it's a bug, but it can be cleaned up so
		** do not print bug message.
		*/
		return;
	}
	/* loop through from top of display */
	for (i = 1, cr = tb->dataset->distop; i < row && cr != NULL; i++, cr = cr->nxtr)
		;
	tb->dataset->crow = cr;
	return;
}


/*{
** Name:	ds_to_row	-	Map dataset row number to display
**
** Description:
** 	When using a data set in unload mode, then update the current
**	row to coincide with the current record. This ensures that if
** 	the row is within the display range then it will get displayed.  
**	Used when a putrow is done with the current row within an unload 
**	block.
**
**	This routine is INTERNAL to tbacc.
**
** Inputs:
**	tb	Ptr to the tablefield descriptor
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
ds_to_row(tb)
TBSTRUCT	*tb;
{
	register i4		i;
	register TBROW		*cr;
	register DATSET		*ds;

	if ((ds = tb->dataset) == NULL)	
		return (FALSE);

	/* Loop through from top of display, updating row number */
	if (ds->distop == NULL)
		return (FALSE);

	for (i = 1, cr = ds->distop; cr != ds->crow && cr != ds->disbot;
	     i++, cr = cr->nxtr)
		;
	if (cr == ds->crow)
		tb->tb_rnum = i;
	else
		tb->tb_rnum = rowNONE;
	return (TRUE);
}

/*{
** Name:	row_cursor	-	Get row number of cursor location
**
** Description:
** 	When no row number is given return the row number that the 
**	cursor is on (if the table is currently active), or -1
**	if the the cursor is not on the tablefield..
**
** Inputs:
**	frm		Ptr to the current frame
**	tb		Ptr to the tablefield descriptor
**
** Outputs:
**
** Returns:
**	i4	Row number the cursor is on, or -1.
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
row_cursor(frm, tb)
RUNFRM		*frm;
TBSTRUCT	*tb;
{
	i4	num;		/* temp row number */	
	TBLFLD	*tfld;			/* frame's table field */

	/* is the table field the currently active one */
	if (!FDactive(frm->fdrunfrm, tb->tb_fld))
	{
		/* cursor is not positioned on table field */
		IIFDerror(TBNOTACT, 1, (char *) tb->tb_name);
		return (-1);
	}
	tfld = tb->tb_fld;
	if ((num = tfld->tfcurrow) < 0)
	{
		/*
		** During a scroll down, tfcurrow is decremented and later 
		** incremented on the return from the scroll. Thus tfcurrow
		** may be < 0, but in this case we can give the caller the 
		** topmost row. (see fixstate.c in frame)
		*/
		num = 0;
	}
	else if (num > tfld->tflastrow)
	{
		/* 
		** Same comment as above, but for scroll up tfcurrow is 
		** incremented and later decremented. Thus give the caller the
		** bottom row. (see fixstate.c in frame)
		*/
		num = tfld->tflastrow;
	}
	/* "+1" because frame driver starts at row 0, here we start at row 1 */
	return (num + 1); 
}


void
IITBicb(RUNFRM *rfrm, TBSTRUCT *tbs, char *colname, i4 row, i4 *uservar)
{
	TBSTRUCT	*unldtbl;
	COLDESC		*cd;
	DATSET		*ds;
	PTR		datarec;
	i4		*dsflags;
	i4		chgbit;
	i4		trow;

	/*
	**  Check if in unload loop.  If so, check if table
	**  is same as one being unloaded.  In this case,
	**  don't allow a specific row number and get information
	**  from row just unloaded.   For all other cases, check
	**  to make sure row number is valid.  If not tied to
	**  a dataset, any row in the table field display is valid.
	**  For a dataset, must restrict to rows that have real data.
	*/
	if ((unldtbl = IIunldstate()) != NULL && unldtbl == tbs)
	{
		/*
		**  We are in any unload loop and referencing the
		**  table field being unloaded.  Make sure user
		**  has not specified an explicit row number.
		*/
		if (row != 0)
		{
			IIFDerror(RTCBNOROW, 0);
			return;
		}

		/*
		**  Access datarow for row just unloaded and calculate
		**  access into row for change bit of requested column.
		*/
		if ((cd = IIcdget(tbs, colname)) == NULL)
		{
			/* column name is not found in table */
			IIFDerror(TBBADCOL, 2, (char *) tbs->tb_name, colname);
			return;
		}
		/*
		**  Get dataset and find out where change bit is for
		**  the requested column.
		*/
		ds = tbs->dataset;
		datarec = ds->crow->rp_data;
		dsflags = (i4 *) ((char *) datarec + cd->c_chgoffset);

		/*
		**  Assign value to user variable.
		*/
		*uservar = ((*dsflags & fdX_CHG) ? 1 : 0);
	}
	else
	{
		/*
		**  Can't inquire on a table field that is empty.
		*/
		if (tbs->tb_display == 0)
		{
			IIFDerror(TBEMPTGR, 1, tbs->tb_name);
			return;
		}
		/*
		**  Check to see if user is referencing current row
		**  or an explicit number.  Either way, must check
		**  for a valid row.
		*/
		if (row == 0)
		{
			if ((row = row_cursor(rfrm, tbs)) < 0)
			{
				return;
			}
		}

		/*
		**  Make sure we have a valid row number.  This
		**  should take care of table fields with or without
		**  datasets.
		*/
		if ((trow = row_range(tbs->tb_name, 1,
			tbs->tb_display, row)) < 0)
		{
			return;
		}

		/*
		**  Set up for getting from table field display only.
		**  Don't bother with dataset, as that may not have
		**  the latest info.
		*/
		IIFDgccb(tbs->tb_fld, trow - 1, colname, &chgbit);
		*uservar = ((chgbit & fdX_CHG) ? 1 : 0);
	}
}


void
IITBscb(RUNFRM *rfrm, TBSTRUCT *tbs, char *colname, i4 row, i4 data)
{
	TBSTRUCT	*unldtbl;
	COLDESC		*cd;
	DATSET		*ds;
	TBROW		*cr;
	i4		i;
	PTR		datarec;
	i4		*dsflags;
	i4		trow;

	/*
	**  Check if in unload loop.  If so, check if table
	**  is same as one being unloaded.  In this case,
	**  don't allow a specific row number and get information
	**  from row just unloaded.   For all other cases, check
	**  to make sure row number is valid.  If not tied to
	**  a dataset, any row in the table field display is valid.
	**  For a dataset, must restrict to rows that have real data.
	*/
	if ((unldtbl = IIunldstate()) != NULL && unldtbl == tbs)
	{
		/*
		**  We are in any unload loop and referencing the
		**  table field being unloaded.  Make sure user
		**  has not specified an explicit row number.
		*/
		if (row != 0)
		{
			IIFDerror(RTCBNOROW, 0);
			return;
		}

		/*
		**  User data can only be zero or one.
		*/
		if (data != 0 && data != 1)
		{
			IIFDerror(RTCB0OR1, 0);
			return;
		}

		/*
		**  Ignore deleted rows.
		*/
		if (IIrec_num() == rowNONE)
		{
			return;
		}

		/*
		**  Access datarow for row just unloaded and calculate
		**  access into row for change bit of requested column.
		*/
		if ((cd = IIcdget(tbs, colname)) == NULL)
		{
			/* column name is not found in table */
			IIFDerror(TBBADCOL, 2, (char *) tbs->tb_name, colname);
			return;
		}

		/*
		**  Ignore hidden columns at this time.
		*/
		if (cd->c_hide)
		{
			return;
		}

		/*
		**  Check to see if dataset row is visible
		**  in table field display.
		*/
		if (((ds = tbs->dataset) == NULL) && (ds->distop == NULL))
		{
			return;
		}

		for (i = 1, cr = ds->distop; cr != ds->crow && cr != ds->disbot;
			i++, cr = cr->nxtr)
		{
			;
		}

		/*
		**  If we didn't found row, then must be visible.
		**  Var "i" constains visible row number using 1 indexing.
		*/
		if (cr == ds->crow)
		{
			/*
			**  Set value into display structure.
			*/
			IIFDtscb(tbs->tb_fld, i - 1, colname, fdX_CHG, data);
		}

		/*
		**  Get dataset and find out where change bit is for
		**  the requested column.
		*/
		datarec = ds->crow->rp_data;
		dsflags = (i4 *) ((char *) datarec + cd->c_chgoffset);

		/*
		**  Assign value from user variable.
		*/
		if (data)
		{
			*dsflags |= fdX_CHG;
		}
		else
		{
			*dsflags &= ~fdX_CHG;
		}
	}
	else
	{
		/*
		**  Can't set on a table field that is empty.
		*/
		if (tbs->tb_display == 0)
		{
			IIFDerror(TBEMPTGR, 1, tbs->tb_name);
			return;
		}

		/*
		**  Find column descriptor.
		*/
		if ((cd = IIcdget(tbs, colname)) == NULL)
		{
			/* column name is not found in table */
			IIFDerror(TBBADCOL, 2, (char *) tbs->tb_name, colname);
			return;
		}

		/*
		**  Ignore hidden columns at this time.
		*/
		if (cd->c_hide)
		{
			return;
		}

		/*
		**  Check to see if user is referencing current row
		**  or an explicit number.  Either way, must check
		**  for a valid row.
		*/
		if (row == 0)
		{
			if ((row = row_cursor(rfrm, tbs)) < 0)
			{
				return;
			}
		}

		/*
		**  Make sure we have a valid row number.  This
		**  should take care of table fields with or without
		**  datasets.
		*/
		if ((trow = row_range(tbs->tb_name, 1,
			tbs->tb_display, row)) < 0)
		{
			return;
		}

		/*
		**  Set up for setting to table field display only.
		**  Don't bother with dataset, as that will get
		**  synched up later on.  This is OK even if we
		**  do an inquire later on since we get from
		**  the display on an inquire also, if not in
		**  an unload loop.
		*/
		IIFDtscb(tbs->tb_fld, trow - 1, colname, fdX_CHG, data);
	}
}


/*{
** Name:	IITBscaSetCellAttr - Setting cell attributes
**
** Description:
**	This routine handles setting cell attributes for a table field.
**	If we are in an unload loop, then target is for the row just
**	unloaded.  If not in unload loop, then targe is for physical
**	row in table field display.
**
** Inputs:
**	rfrm		RUNFRM struct of parent form.
**	tbs		TBSTRUCT struct of target table field.
**	colname		Name of targe column.
**	row		Target row number.
**	data		Attribute to set.
**	current		Flag indicating if we are dealing with current form.
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
**	07/22/90 (dkh) - Initial version.
*/
VOID
IITBscaSetCellAttr(RUNFRM *rfrm, TBSTRUCT *tbs, char *colname,
	i4 row, i4 data, i4 onoff, i4 current)
{
	TBSTRUCT	*unldtbl;
	COLDESC		*cd;
	DATSET		*ds;
	TBROW		*cr;
	i4		i;
	PTR		datarec;
	i4		trow;
	i4		*attrptr;
	i4		dsattrib;
	i4		isvisible = FALSE;

	/*
	**  First, check to see if we are in an unload loop.  If
	**  we are, then setting is relative to the row just
	**  unloaded.  Otherwise, we need to go to the physical
	**  row in the table field display.
	*/
	if ((unldtbl = IIunldstate()) != NULL && unldtbl == tbs)
	{
		/*
		**  Did user specified a row # in an unload loop?
		*/
		if (row != 0)
		{
			/*
			**  Yup, user specify an explicit row number.
			**  Tell user this is a no-no and return.
			*/
			IIFDerror(E_FI2023_8227, 0);
			return;
		}

		/*
		**  Ignore deleted rows.
		*/
		if (IIrec_num() == rowNONE)
		{
			return;
		}

		/*
		**  Access datarow for row just unloaded and calculate
		**  access to area for holding attribute bits.
		*/
		if ((cd = IIcdget(tbs, colname)) == NULL)
		{
			/*
			**  Bad column name passed by user.
			*/
			IIFDerror(TBBADCOL, 2, (char *)tbs->tb_name, colname);
			return;
		}

		/*
		**  Just ignore hidden columns at this time.
		*/
		if (cd->c_hide)
		{
			return;
		}

		/*
		**  If no dataset row, just return.
		*/
		if (((ds = tbs->dataset) == NULL) && (ds->distop == NULL))
		{
			return;
		}

		/*
		**  Now check to see if row is visible or not.
		*/
		for (i = 1, cr = ds->distop; cr != ds->crow && cr != ds->disbot;
			i++, cr = cr->nxtr)
		{
			;
		}

		if (cr == ds->crow)
		{
			/*
			**  We have a row that is visible, set
			**  local indicator for later use.
			*/
			isvisible = TRUE;
		}

		/*
		**  Now update info into the dataset itself.
		*/
		datarec = ds->crow->rp_data;
		attrptr = (i4 *) ((char *) datarec + cd->c_chgoffset);
		dsattrib = frmtods_attr(data);
		if (onoff == 0)
		{
			/*
			**  We don't have to worry about the case where
			**  user is setting normal to zero since that
			**  should have been caught by RTsetrow() and
			**  execution will never get here.
			**
			**  Also, setting color to zero is also taken
			**  care with the following code.
			*/
			*attrptr &= ~dsattrib;
		}
		else
		{
			/*
			**  Clear out old color setting if
			**  we are setting a new non-zero one.
			*/
			if (data & fdCOLOR)
			{
				*attrptr &= ~dsCOLOR;
			}
			*attrptr |= dsattrib;
		}
		/*
		**  If visible, then update to tffflags and
		**  throw call FTsetda().
		*/
		if (isvisible)
		{
			/*
			**  Go to zero indexing for table field access.
			*/
			i--;
			IIFDscaSetCellAttr(rfrm->fdrunfrm, tbs->tb_fld,
				colname, i, data, onoff, TRUE, current);
		}
	}
	else
	{
		/*
		**  Not in an unload loop, so we have to deal with the
		**  physical table field.
		**
		**  Note that we can set attributes on any cell
		**  with no problems.
		*/
		if (tbs->dataset != NULL)
		{
			/*
			**  Check to see if user
			**  passed a hidden column name.
			*/
			if ((cd = IIcdget(tbs, colname)) == NULL)
			{
				/*
				**  User passed a bad column name.
				*/
				IIFDerror(TBBADCOL, 2, (char *) tbs->tb_name,
					colname);
				return;
			}

			/*
			**  We just ignore hidden columns for now.
			*/
			if (cd->c_hide)
			{
				return;
			}
		}

		/*
		**  Now check for a valid row number.  Zero
		**  means to user current row.
		*/
		if (row == 0)
		{
			/*
			**  Now figure out what is the current row.
			**  Note that we CAN'T call row_cursor since
			**  we are allowing any cell to have attributes
			**  set on it.  Not just ones that have data.
			*/
			if (!FDactive(rfrm->fdrunfrm, tbs->tb_fld))
			{
				/*
				**  Cursor is not positioned on
				**  table field
				*/
				IIFDerror(TBNOTACT, 1, (char *) tbs->tb_name);
				return;
			}
			if ((row = tbs->tb_fld->tfcurrow) < 0)
			{
				row = 0;
			}
			else if (row > tbs->tb_fld->tfrows - 1)
			{
				row = tbs->tb_fld->tfrows - 1;
			}
			row++;
		}

		/*
		**  If we have any invalid row number, just return.
		*/
		if (row < 1 || row > tbs->tb_fld->tfrows)
		{
			return;
		}

		/*
		**  Now set attribute for physical table field cell.
		**  First, go to 0 indexing.
		*/
		row--;
		IIFDscaSetCellAttr(rfrm->fdrunfrm, tbs->tb_fld, colname,
			row, data, onoff, FALSE, current);
	}
}




/*{
** Name:	dstofrm_attr - Translate attributes from dataset to frame.
**
** Description:
**	Translate the dataset attribute encodings into frame attribute
**	encodings.  This is used to inquiring on the attributes.
**
** Inputs:
**	dsattr			Encodings of dataset attributes.
**
** Outputs:
**
**	Returns:
**		frmattr		Frame encodings of attributes.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	07/25/90 (dkh) - Initial version.
*/
static i4
dstofrm_attr(dsattr)
i4	dsattr;
{
	i4	frmattr = 0;

	if (dsattr & dsRVVID)
	{
		frmattr |= fdRVVID;
	}
	if (dsattr & dsBLINK)
	{
		frmattr |= fdBLINK;
	}
	if (dsattr & dsUNLN)
	{
		frmattr |= fdUNLN;
	}
	if (dsattr & dsCHGINT)
	{
		frmattr |= fdCHGINT;
	}
	if (dsattr & ds1COLOR)
	{
		frmattr |= fd1COLOR;
	}
	if (dsattr & ds2COLOR)
	{
		frmattr |= fd2COLOR;
	}
	if (dsattr & ds3COLOR)
	{
		frmattr |= fd3COLOR;
	}
	if (dsattr & ds4COLOR)
	{
		frmattr |= fd4COLOR;
	}
	if (dsattr & ds5COLOR)
	{
		frmattr |= fd5COLOR;
	}
	if (dsattr & ds6COLOR)
	{
		frmattr |= fd6COLOR;
	}
	if (dsattr & ds7COLOR)
	{
		frmattr |= fd7COLOR;
	}

	return(frmattr);
}



/*{
** Name:	tftofrm_attr - Translate attributes from table field to frame.
**
** Description:
**	Translate the table field attribute encodings into frame attribute
**	encodings.  This is used to inquiring on the attributes.
**
** Inputs:
**	tfattr			Encodings of table field attributes.
**
** Outputs:
**
**	Returns:
**		frmattr		Frame encodings of attributes.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	07/25/90 (dkh) - Initial version.
*/
static i4
tftofrm_attr(tfattr)
i4	tfattr;
{
	i4	frmattr = 0;

	if (tfattr & tfRVVID)
	{
		frmattr |= fdRVVID;
	}
	if (tfattr & tfBLINK)
	{
		frmattr |= fdBLINK;
	}
	if (tfattr & tfUNLN)
	{
		frmattr |= fdUNLN;
	}
	if (tfattr & tfCHGINT)
	{
		frmattr |= fdCHGINT;
	}
	if (tfattr & tf1COLOR)
	{
		frmattr |= fd1COLOR;
	}
	if (tfattr & tf2COLOR)
	{
		frmattr |= fd2COLOR;
	}
	if (tfattr & tf3COLOR)
	{
		frmattr |= fd3COLOR;
	}
	if (tfattr & tf4COLOR)
	{
		frmattr |= fd4COLOR;
	}
	if (tfattr & tf5COLOR)
	{
		frmattr |= fd5COLOR;
	}
	if (tfattr & tf6COLOR)
	{
		frmattr |= fd6COLOR;
	}
	if (tfattr & tf7COLOR)
	{
		frmattr |= fd7COLOR;
	}

	return(frmattr);
}


/*{
** Name:	frmtods_attr - Translate frm attribute encodings to dataset.
**
** Description:
**	Translate the frame attribute encodings into the set used for
**	storing in the dataset.
**
** Inputs:
**	frmattr		Frame attribute encodings.
**
** Outputs:
**
**	Returns:
**		dsattr	Dataset attribute encodings.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	07/24/90 (dkh) - Initial version.
*/
static i4
frmtods_attr(frmattr)
i4	frmattr;
{
	i4	dsattr = 0;

	if (frmattr & fdRVVID)
	{
		dsattr |= dsRVVID;
	}
	if (frmattr & fdBLINK)
	{
		dsattr |= dsBLINK;
	}
	if (frmattr & fdUNLN)
	{
		dsattr |= dsUNLN;
	}
	if (frmattr & fdCHGINT)
	{
		dsattr |= dsCHGINT;
	}
	if (frmattr & fd1COLOR)
	{
		dsattr |= ds1COLOR;
	}
	if (frmattr & fd2COLOR)
	{
		dsattr |= ds2COLOR;
	}
	if (frmattr & fd3COLOR)
	{
		dsattr |= ds3COLOR;
	}
	if (frmattr & fd4COLOR)
	{
		dsattr |= ds4COLOR;
	}
	if (frmattr & fd5COLOR)
	{
		dsattr |= ds5COLOR;
	}
	if (frmattr & fd6COLOR)
	{
		dsattr |= ds6COLOR;
	}
	if (frmattr & fd7COLOR)
	{
		dsattr |= ds7COLOR;
	}

	return(dsattr);
}




/*{
** Name:	IITBicaInqCellAttr - Inquiring on cell attributes.
**
** Description:
**	This routine supports inquiring on cell attributes for a table field.
**	If we are in an unload loop, then target is for the row just
**	unloaded.  If not in unload loop, then targe is for physical
**	row in table field display.
**
**	We just return the appropriate set of attributes values to the
**	caller and let the caller figure out what to return to the user.
**
** Inputs:
**	rfrm		RUNFRM struct of parent form.
**	tbs		TBSTRUCT struct of target table field.
**	colname		Name of targe column.
**	row		Target row number.
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
**	07/22/90 (dkh) - Initial version.
*/
STATUS
IITBicaInqCellAttr(RUNFRM *rfrm, TBSTRUCT *tbs, char *colname, i4 row, i4 *flagval)
{
	i4		*pflag;
	TBSTRUCT	*unldtbl;
	TBLFLD		*tbl;
	FLDCOL		*col;
	COLDESC		*cd;
	DATSET		*ds;
	TBROW		*cr;
	i4		i;
	PTR		datarec;
	i4		trow;
	i4		*attrptr;

	/*
	**  First, check to see if we are in an unload loop.  If
	**  we are, then inquiring is relative to the row just
	**  unloaded.  Otherwise, we need to go to the physical
	**  row in the table field display.
	*/
	if ((unldtbl = IIunldstate()) != NULL && unldtbl == tbs)
	{
		/*
		**  Did user specified a row # in an unload loop?
		*/
		if (row != 0)
		{
			/*
			**  User did specify a row number in an
			**  unload loop.  Give user bad news and return.
			*/
			IIFDerror(E_FI2023_8227, 0);
			return(FAIL);
		}

		/*
		**  Ignore deleted rows.
		*/
		if (IIrec_num() == rowNONE)
		{
			return(FAIL);
		}

		/*
		**  Access datarow for row just unloaded and calculate
		**  access to area for holding attribute bits.
		*/
		if ((cd = IIcdget(tbs, colname)) == NULL)
		{
			/*
			**  Bad column name passed by user.
			*/
			IIFDerror(TBBADCOL, 2, (char *)tbs->tb_name, colname);
			return(FAIL);
		}

		/*
		**  Just ignore hidden columns at this time.
		*/
		if (cd->c_hide)
		{
			return(FAIL);
		}

		/*
		**  If no dataset row, just return.
		*/
		if (((ds = tbs->dataset) == NULL) && (ds->distop == NULL))
		{
			return(FAIL);
		}

# ifdef TEST
		/*
		**  Now check to see if row is visible or not.
		*/
		for (i = 1, cr = ds->distop; cr != ds->crow && cr != ds->disbot;
			i++, cr = cr->nxtr)
		{
			;
		}

		if (cr == ds->crow)
		{
			/*
			**  We have a row that is visible, need to call
			**  set this info in the tffflags part of the
			**  TBLFLD struct and call FTsetda().
			*/
		}
# endif

		/*
		**  Now assemble the attributes to hand back
		**  to caller.
		*/
		datarec = ds->crow->rp_data;
		attrptr = (i4 *) ((char *) datarec + cd->c_chgoffset);
		*flagval = dstofrm_attr(*attrptr);
		return(OK);
	}
	else
	{
		/*
		**  Not in an unload loop, so we have to deal with the
		**  physical table field.
		**
		**  Note that we can set attributes on any cell
		**  with no problems.
		*/
		if (tbs->dataset != NULL)
		{
			/*
			**  Check to see if user
			**  passed a hidden column name.
			*/
			if ((cd = IIcdget(tbs, colname)) == NULL)
			{
				/*
				**  User passed a bad column name.
				*/
				IIFDerror(TBBADCOL, 2, (char *) tbs->tb_name,
					colname);
				return(FAIL);
			}

			/*
			**  We just ignore hidden columns for now.
			*/
			if (cd->c_hide)
			{
				return(FAIL);
			}
		}

		/*
		**  Now check for a valid row number.  Zero
		**  means to user current row.
		*/
		if (row == 0)
		{
			/*
			**  Now figure out what is the current row.
			**  Note that we CAN'T call row_cursor since
			**  we are allowing any cell to have attributes
			**  set on it.  Not just ones that have data.
			*/
			if (!FDactive(rfrm->fdrunfrm, tbs->tb_fld))
			{
				/*
				**  Cursor is not positioned on
				**  table field
				*/
				IIFDerror(TBNOTACT, 1, (char *) tbs->tb_name);
				return(FAIL);
			}
			if ((row = tbs->tb_fld->tfcurrow) < 0)
			{
				row = 0;
			}
			else if (row > tbs->tb_fld->tfrows - 1)
			{
				row = tbs->tb_fld->tfrows - 1;
			}
			row++;

		}

		/*
		**  If we have any invalid row number, just return.
		*/
		if (row < 1 || row > tbs->tb_fld->tfrows)
		{
			return(FAIL);
		}

		/*
		**  Now set attribute for physical table field cell.
		**  First, go to 0 indexing.
		*/
		row--;
		tbl = tbs->tb_fld;
		if ((col = FDfndcol(tbl, colname)) == NULL)
		{
			/*
			**  Bad column name passed.
			*/
			IIFDerror(TBBADCOL, 2, (char *) tbs->tb_name, colname);
			return(FAIL);
		}
		pflag = tbl->tffflags + (row * tbl->tfcols) + col->flhdr.fhseq;
		*flagval = tftofrm_attr(*pflag);
		return(OK);
	}
}




/*{
** Name:	IItsetattr	- Set attribute for a value being loaded.
**
** Description:
**	This routine supports setting an attribute for a dataset value
**	at loadtable or insertrow time.  Two things to note is that if
**	user is setting color to zero, then the attributes passed
**	contains all 7 colors but onoff is passed as zero.  Similarly,
**	if the normal attribute is being set to one, then all attributes
**	are passed in flags and onoff is passed as zero.
**
**	If there is no databaset present, then this routine generates
**	an error due to fact that the application is doing an insertrow
**	into a bare table field.
**
** Inputs:
**	colname		Name of column to set attribute on.
**	flag		Bit map containing attribute to set.
**	onoff		Are we setting or clearing an attribute.
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
**	If dataset value is visible, then force the attributes to the
**	screen as well.
**
** History:
**	07/25/90 (dkh) - Initial version.
*/
VOID
IItsetattr(char *colname, i4 flag, i4 onoff)
{
	i4		tfattr;
	DATSET		*ds;
	COLDESC		*cd;
	TBSTRUCT	*tb;
	PTR		datarec;
	i4		*ptr;
	i4		*pflag;
	FLDCOL		*col;
	TBLFLD		*tbl;

	tb = IIcurtbl;

	if ((ds = tb->dataset) == NULL)
	{
		/*
		**  No dataset attached, we must be doing an
		**  insertrow on a bare table field.  We don't
		**  support setting attributes in this case.
		**  Put out error and return.
		*/
		IIFDerror(E_FI2024_8228, 0);
		return;
	}

	/*
	**  Set up routines for insertrow (IItbsetio and IItinsert)
	**  and loadtable (IItbact) have already set "ds->crow" to
	**  be the dataset row that data is to be placed in.
	*/
	datarec = ds->crow->rp_data;

	/*
	**  Now need to grab chgoffset (which does column
	**  name checking).
	*/
	if ((cd = IIcdget(tb, colname)) == NULL)
	{
		/*
		**  User passed a bad column name
		*/
		IIFDerror(TBBADCOL, 2, (char *) tb->tb_name, colname);
		return;
	}

	ptr = (i4 *) ((char *) datarec + cd->c_chgoffset);

	tfattr = frmtods_attr(flag);

	if (onoff == 0)
	{
		*ptr &= ~tfattr;
	}
	else
	{
		/*
		**  If setting a new non-zero color, clear
		**  away any old color settings.
		*/
		if (flag & fdCOLOR)
		{
			*ptr &= ~dsCOLOR;
		}
		*ptr |= tfattr;
	}

	/*
	**  If doing a loadtable and we are pass the bottom of the
	**  display, no need to do anything since row is not visible.
	*/
	if ((tb->tb_state == tbLOAD) && (ds->crow != ds->disbot))
	{
		return;
	}

	tbl = tb->tb_fld;

	/*
	**  Find the FLDCOL structure for the column, no need to
	**  check return value for correctness since we wouldn't
	**  get this far if we have a bad column name.
	*/
	col = FDfndcol(tbl, colname);

	pflag = tbl->tffflags + ((tb->tb_rnum - 1) * tbl->tfcols) +
		col->flhdr.fhseq;

	*pflag &= ~dsALLATTR;
	*pflag |= (*ptr & dsALLATTR);

	/*
	**  Note that it doesn't matter what we pass as the last
	**  arg to FTsetda() since it will check the attributes
	**  set for the column and in tffflags to determine what
	**  to set.
	*/
	FTsetda(IIfrmio->fdrunfrm, tbl->tfhdr.fhseq, FT_UPDATE, FT_TBLFLD,
		col->flhdr.fhseq, tb->tb_rnum - 1, 0);
}
