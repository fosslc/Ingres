/*
**	iiscrscn.c
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
** Name:	iiscrscan.c
**
** Description:
**	Support for a 'SCROLL TO' statement
**
**	Public (extern) routines defined:
**		IIscr_scan()
**	Private (static) routines defined:
**		fix_recnum()
**
** History:
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	07/03/88 (dkh) - Fixed problem with scroll commands executed within
**			 an activate scroll block.
**	19-dec-89 (bruceb)
**		Fixed algorithm so no scroll occurs if requested row is
**		already visible.  Also, if requested row is above the top
**		of the visible display, when scrolled into view the row
**		will be displayed at top of TF.  The latter change is in
**		keeping with the result of scrollup/down frs keys.
**
**	06-may-92 (leighb) DeskTop Porting Change:
**		Added IIFDdsrDSRecords routine for use with ScrollBars.
**	01/27/92 (dkh) - Changed the way IIFDdsrDSRecords is set up so that
**			 it is not a link problem for various forms utilities.
**	02/05/92 (dkh) - Updated to handle interface change to IIdisprow().
**	19-mar-93 (fraser)
**		Restored IIFDdsrDSRecords stuff to its original form and
**		ifdef-ed it (on SCROLLBARS) to avoid linking problems.
**		Incorporated some changes in IIFDdsrDSRecords.
**	05/03/93 (dkh) - Changed the name back to IITBdsrDSRecords and
**			 removed the ifdef since this will make the
**			 changes generic regardless of which platform
**			 the code is compiled on.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	15-apr-1997 (cohmi01)
**	    Keep track of ordinal position in dataset. (b81574)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Update prototypes for gcc 4.3.
**/

static i4	fix_recnum();	/* is recnum in valid range */

/*{
** Name:	IIscr_scan	-	Scroll tblfld to target row
**
** Description:
**	Scroll a tablefield dataset to show the specified row.
**
**	An EQUEL '## SCROLL TO' statement will generate a call to this
**	routine.
**
**	If no data set is being used then just set the cursor to the
**	specified row number.
**	If a data set is linked then scan through the data set with
**	record pointer to the top of the final displayed record. Once
**	reaching the specified record number, update another record
**	pointer to the bottom of the display.  Display all the data
**	between these two pointers, and update the current cursor-row
**	to the corresponding row number.
**
**	Update the frame's table field's current row (tf->tfcurrow)
**	to the row number where the record scrolled to is displayed.
**	Update the frame's table field's current column (tf->tfcurcol)
**	to the first column.  This is to make a SCROLL TO similar to
**	a regular SCROLL UP or DOWN, that is triggerred by a cursor
**	movement.
**
** Inputs:
**	tb		Tablefield to scroll
**	recnum		Dataset record number to scroll to
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
**	18-feb-1987	- Changed rp_rec reference to new rp_data (drh)
**	15-apr-1997 (cohmi01)
**	    Keep track of ordinal position in dataset. (b81574)
**
*/

i4
IIscr_scan(TBSTRUCT *tb, i4 recnum)
{
	DATSET		*ds;
	register TBROW	*rptop;			/* top of final display */
	register TBROW	*rpbot;			/* bottom of final display */
	register i4	i;			/* row counter */
	TBROW		*current;
	TBROW		*distop;
	i4		current_recnum;

	/*
	**  Set tf->tfstate to tfNORMAL.
	**  This is necessary in case we want to do a "scroll to"
	**  in an activate scrollup|scrolldown block.  In such a
	**  situation, we don't want tfSTATE to be still set to
	**  tfSCRUP or tfSCRDOWN, which will cause the current
	**  row to be something other than the "scrolled to" row.
	**  Since one must do an explicit scroll up|down command
	**  in an activate scroll block, this change should not
	**  break anything.
	*/
	tb->tb_fld->tfstate = tfNORMAL;

	/* if no data set just set current cursor-row and cursor-column */
	if ((ds = tb->dataset) == NULL)
	{
		if (!fix_recnum(tb->tb_display, tb, &recnum))
			return (FALSE);

		/*
		** Set current row to caller's row number.
		** "-1" as frame uses C arithmatic.
		*/
		tb->tb_fld->tfcurrow = recnum -1;
		tb->tb_fld->tfcurcol = 0;
		return (TRUE);
	}

	/* data set is linked scan for caller's record number */
	if (!fix_recnum(ds->ds_records, tb, &recnum))
		return (FALSE);

	/* check if scanning is required -- more than screenfull of records */
	if (ds->ds_records > tb->tb_numrows)
	{
		distop = ds->distop;
		/*
		** Advance to top of TF display or recnum record, whichever
		** comes first.
		*/
		for (current_recnum = 1, current = ds->top;
		    current_recnum < recnum && current != distop;
		    current_recnum++, current = current->nxtr)
		{
		    ;
		}

		if ((current == distop)
		    && (current_recnum + tb->tb_numrows > recnum))
		{
		    /*
		    ** Recnum record is already visible in the TF,
		    ** so don't modify the display, just the current row.
		    */
		    recnum -= (current_recnum - 1);
		    ds->distopix  = current_recnum;
		}
		else
		{
		    /*
		    ** The requested record isn't visible, so need to modify
		    ** the display.  If recnum is before the currently
		    ** displayed records in the dataset, place it at the top
		    ** of the visible section, else if after the displayed
		    ** records, place at the bottom of the visible section.
		    */

		    rptop = current;

		    /* First, retrieve all displayed rows before losing them. */
		    for (i = 0, current = distop; i < tb->tb_display;
			 i++, current = current->nxtr)
		    {
			IIretrow(tb, i, current);
		    }

		    if (current_recnum != recnum)
		    {
			/*
			** Recnum is below the visible TF display, so need
			** to advance through the dataset.
			*/
			for (i = current_recnum + tb->tb_numrows - 1;
			    i < recnum; i++, rptop = rptop->nxtr)
			{
			    ;
			}
			/* keep track of distop's changed ordinal position */
			ds->distopix = recnum - (tb->tb_numrows - 1);
		    }
		    else
		    {
			/* Recnum is above the visible display */
		    	ds->distopix = recnum;
		    }

		    /* Display the new rows and update rpbot. */
		    for (i = 0, rpbot = rptop; i < tb->tb_numrows;
			i++, rpbot = rpbot->nxtr)
		    {
			IIdisprow(tb, i, rpbot);
		    }
		    /*
		    ** Update bottom of new display, since was bypassed by
		    ** one record.
		    */
		    if (rpbot == NULL)
		    {
			/* Must have reached last record. */
			rpbot = ds->bot;
		    }
		    else
		    {
			/* Go back one from the end of the loop. */
			rpbot = rpbot->prevr;
		    }
		    ds->distop = rptop;
		    ds->disbot = rpbot;

		    /* Reset recnum to point at top or bottom of TF display. */
		    if (current_recnum == recnum)
		    {
			/* Display record at top of the TF. */
			recnum = 1;
		    }
		    else
		    {
			/* Display record at bottom of the TF. */
			recnum = tb->tb_numrows;
		    }
		}
	}
	/*
	** Set current row to caller's row number.
	** "-1" as frame uses C arithmatic.
	*/
	tb->tb_fld->tfcurrow = recnum -1;
	tb->tb_fld->tfcurcol = 0;
	return (TRUE);
}

/*{
** Name:	fix_recnum
**
** Description:
**	Check that record number is in range, make record number absolute
**	if necessary.
**
** Inputs:
**	max_recnum
**	tb
**	recnum
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

static	i4
fix_recnum(max_recnum, tb, recnum)
i4		max_recnum;
TBSTRUCT	*tb;
i4		*recnum;
{
	register i4	record	=	*recnum;
	i4		recval;

	if (record == rowEND)
	{
		*recnum = max_recnum;	/* change recnum from a relative to
					 * an absolute value.
					 */
	}
	else if (record < 1 || record > max_recnum)
	{
		/* record number out of range */
		recval = record;
		IIFDerror(TBRECNUM, 2, &recval, tb->tb_name);
		return (FALSE);
	}
	return (TRUE);
}

/*{
** Name:        IITBdsrDSRecords
**
** Description:
**      Return nbr records in current dataset and number records preceeding
**      current row.
**
** Inputs:
**	tblfld		Ptr to TBLFLD (to identify the Tablefieid)
**	ds_records	Ptr to i4  to load with nbr of records,
**	ds_currow	Ptr to i4  to load with 0-relative current position
**
** Outputs:
**      Nbr of records in the dataset,
**      Nbr of records preceeding current row.
**
** Returns:
**      none
**
** Exceptions:
**      If any of the 3 ptrs needed is NULL, 0 is returned for both values.
**
** Side Effects:
**
** History:
**      
**      6-may-92 (leighb) Written.
**      26-jan-93 (leighb)
**              Added SCROLLBARS ifdef around IIFDdsrDSRecords;
**              removed old version of IIFDdsrDSRecords.
**      23-feb-93 (leighb)
**              Added TBLFLD argument.
*/

VOID
IITBdsrDSRecords(tblfld, ds_records, ds_currow)
TBLFLD *tblfld;
i4  *   ds_records;
i4  *   ds_currow;
{
	i4     i;
	TBROW * r;
	TBROW * distop;
	TBSTRUCT * tbstruct;

	*ds_records = *ds_currow = 0;
	if ((IIfrmio == NULL) || (IIfrmio->fdruntb == NULL))
	 	return;
	for (tbstruct = IIfrmio->fdruntb;
		tbstruct->tb_fld != tblfld;
		   tbstruct = tbstruct->tb_nxttb)
	{
		if (tbstruct->tb_nxttb == NULL)
			return;
	}
	if (tbstruct->dataset == NULL)
		return;
	if ((*ds_records = tbstruct->dataset->ds_records) > 1)
	{
		distop = tbstruct->dataset->distop;
		for (i=0, r = tbstruct->dataset->top; r != distop; ++i)
			r = r->nxtr;
		*ds_currow = i + tbstruct->tb_fld->tfcurrow;
	}
	return;
}
