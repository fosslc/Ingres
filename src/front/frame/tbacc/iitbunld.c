/*
**	iitbunld.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
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
# include	<er.h>
# include	"ertb.h"

/**
** Name:	iitbunld.c	-	unloadtable support routines
**
** Description:
**	Support routines for the EQUEL '## UNLOADTABLE' statement.
**
**	Typical call looks like:
**
**		while (IItunload() != 0)		## UNLOADTABLE
**	or:
**		IItunend()				## ENDLOOP
**
**
**	Public (extern) routines defined:
**		IItunload()
**		IItunend()
**		IIunldstate()
**		IIrec_num()
**	Private (static) routines defined:
**
** History:
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	08/01/89 (dkh) - Added support for aggregate processing.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**      10/26/99 (hweho01)
**          Added *IItfind() function prototype. Without the 
**          explicit declaration, the default int return value for 
**          a function will truncate a 64-bit address on ris_u64 
**          platform. 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

GLOBALREF	TBSTRUCT	*IIcurtbl;

FUNC_EXTERN	COLDESC	*IIcdget();
FUNC_EXTERN	VOID	IIFDdsgDoSpecialGDV();


static	i2		onDellist = 0;		/* for unloading delete list */
static	TBSTRUCT	*lasttb = NULL;		/* current table on entry into
						   unload loop */
static	TBROW		*nxtrow = NULL;		/* next row to unload */
static	i4		recnum = 0;		/* record number in data set */

static	bool		unldnesterr = FALSE;	/* User tried to nest unloads */

static	TBROW		*nxaggrow = NULL;	/* next agg row to unload */
static	i4		aggoffset = 0;		/* datarow offset for agg */
static	DB_DATA_VALUE	aggdbv = {0};		/* dbv for agg */
static	i4		aggchgoffset = 0;	/* flag offset for agg */
static	i4		aggvisrow = 0;		/* visible row # for agg */
static	TBSTRUCT	*aggtb = NULL;		/* tbstruct for agg */
static	i4		aggcolnum = 0;		/* col num for agg */

/*{
** Name:	IItunload	-	Control UNLOADTABLE while loop
**
** Description:
**	Called repeatedly out of the WHILE look of an UNLOADTABLE
**	statement, this routine will return false when the entire
**	table has been unloaded, or when an error occurs, terminating
**	the loop.
**
** Need to keep track of the current table on entry into the while loop.  A
** typical application may be  ## unloadtable form1 tab1 (...)
**			       ## {
**			       ##	loadtable form2 tab2 (...)
**			       ## }
** which may well change the value of the global IIcurtbl and possibly the
** table state too. Consequently lasttb was implemented to save this state.
**
**	This routine is part of TBACC's external interface.
**
** Inputs:
**
** Outputs:
**
** Returns:
**	bool	TRUE if more to unload
**		FALSE if error, or all data unloaded
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**		04-mar-1983	- written (ncg)
**		6/16/86 (bab)	- add IIsetferr(0) to fix bug 9491.
**
*/

bool
IItunload(void)
{
	register	DATSET	*ds;

	IIsetferr((ER_MSGID) 0); /* reset error num. off for each unload loop */
	if (lasttb == NULL)			/* first time round the loop */
	{
		/* validate the current table status */
		/*
		** Check to see what the state is - if a user did a 'goto'
		** out of (or into) the loop, the state will be corrupted.
		*/
		if (IIcurtbl == NULL)
		{
			IItbug(ERget(S_TB0005_IItunload_IIcurtbl));
			return (FALSE);
		}
		else if (IIcurtbl->tb_state != tbUNLOAD)
		{
			IItbug(ERget(S_TB0006_IItunload_table_not));
			return (FALSE);
		}
		/* save state and loop table */
		lasttb = IIcurtbl;
		/* set up first row */
		nxtrow = IIcurtbl->dataset->top;
		recnum = 0;
	}
	ds = lasttb->dataset;
	/*
	** BUG 5842 - Comment out this optimization.  The manual does not
	** mention this skipping, and it can cause problems when asking for
	** "datarows" and then UNLOADTABLE - things will be out of sync. (ncg)
	**
	** skip over all types of empty rows (a nice for loop)
	for (rp = nxtrow; (rp != NULL) &&
	    (rp->rp_state == stNEWEMPT || rp->rp_state == stUNDEF);
	     rp = rp->nxtr, recnum++)
		;
	ds->crow = rp;
	*/
	ds->crow = nxtrow;

	/* are we at the end of some list */
	if (ds->crow == NULL)
	{
		if (onDellist)			/* deleted list is unloaded */
		{
			IItunend();	/* end while loop */
			return (FALSE);
		}

		/* start unloading delete list if any (onDlist = False) */
		if ((ds->crow = ds->dellist) == NULL)
		{
			IItunend();	/* end while loop */
			return (FALSE);
		}
		onDellist = 1;			/* starting delete list */
		recnum = rowNONE;		/* flag non-existant	*/
	}
	nxtrow = ds->crow->nxtr;		/* keep track of next row */
	if (!onDellist)
		recnum++;			/* and current rec number */
	lasttb->tb_state = tbUNLOAD;
	IIcurtbl = lasttb;			/* set for pending IItret() */
	return (TRUE);				/* continue unloading loop */
}

/*{
** Name:	IItunend	-	exit unloadtable while loop
**
** Description:
**	Reset state variable as part of exit of unloadtable loop
**
**	This routine is part of TBACC's external interface.
**
** Inputs:
**
** Outputs:
**
** Returns:
**
** Exceptions:
**	none
**
** Side Effects:
**	Resets all local static variables to initial state.
**
** History:
**
*/

i4
IItunend(void)
{
	if (unldnesterr)
	{
		unldnesterr = FALSE;
	}
	else
	{
		onDellist = 0;		/* reset for next time */
		lasttb = NULL;		/* reset table state */
		nxtrow = NULL;		/* next row to unload */
		recnum = 0;
	}
	return (TRUE);
}


/*{
** Name:	IInesterr - Set flag indicating user tried to nest unloads.
**
** Description:
**	Sets static variable "unldnesterr" to TRUE to indicate that
**	user just tried to nest unloadtable loops.  This is used
**	by routine IItunend() above to not clear out critical information
**	for the unloadtable that is active.
**
** Inputs:
**	None.
**
** Outputs:
**	Non.e
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	07/07/87 (dkh) - Initial version.
*/
VOID
IInesterr()
{
	unldnesterr = TRUE;
}

/*{
** Name:	IIunldstate	-	Get current tblfld being unloaded
**
** Description:
**	Returns to the caller the current value of 'lasttb', the
**	tablefield that is currently being unloaded in an UNLOADTABLE
**	loop.
**
** Inputs:
**
** Outputs:
**
** Returns:
**	Ptr to the TBSTRUCT for the table currently being unloaded.
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**
*/

TBSTRUCT *
IIunldstate(void)
{
	return (lasttb);
}

/*{
** Name:	IIrec_num	-	ds rec num currently being unloaded
**
** Description:
**	Returns to the caller the dataset record number currently
**	being unloaded in an unloadtable loop.
**
** Inputs:
**
** Outputs:
**
** Returns:
**	i4	Dataset record number currently being unloaded
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**
*/

i4
IIrec_num(void)
{
	return (recnum);
}




/*{
** Name:	IITBasAggSetup - Set up for table field aggregate scanning.
**
** Description:
**	This routine sets up various static variables to support
**	scanning a table field dataset for derivation aggregate
**	processing.  Values are obtained from the dataset unless
**	the dataset row is currently visible.  In this case the
**	value is obtained from the table field display instead.
**	We are assuming that only one aggregate is active at
**	any one time.
**
**	This routine is normally called by the derivation processing
**	routines via a call back function pointer (struct member
**	aggsetup) in the FRS_RFCB control block pointed to by
**	variable IIFRrfcb.
**
**	If the table field is in query mode, then simply return FAIL.
**
** Inputs:
**	frm		Pointer to form containing table field.
**	fld		Pointer to table field for doing setup on.
**	colno		Column number (zero indexed) indicating
**			column to set up for  aggregate run.
**
** Outputs:
**	None.
**
**	Returns:
**		OK	If set up completed correctly.
**		FAIL	If table field was in query mode.
**	Exceptions:
**		None.
**
** Side Effects:
**	Various static variables are set up for dataset scanning
**	as well as causing cross column derivation to be run and
**	table field displayed values to be synchronized with the
**	visual display.
**
** History:
**	08/07/89 (dkh) - Initial version.
*/
STATUS
IITBasAggSetup(FRAME *frm, FIELD *fld, i4 colno)
{
	DATSET		*ds;
	RUNFRM		*runf;
	TBSTRUCT	*tb;
	char		*colname;
	FLDCOL		*col;
	TBLFLD		*tfld;
	COLDESC		*aggcol;
	i4		lastrow;
	i4		i;
	i4		rownum_orig;

	/*
	**  Find the dataset
	*/

	runf = RTfindfrm(frm->frname);
	tfld  = fld->fld_var.fltblfld;
	aggtb = tb = (TBSTRUCT *) IItfind(runf, tfld->tfhdr.fhdname);

	/*
	**  If table field in query mode, return FAIL.
	if (tb->tb_mode == fdtfQUERY)
	{
		return(FAIL);
	}
	*/

	aggvisrow = -1;
	if ((ds = tb->dataset) == NULL || tb->tb_display == 0 ||
		tb->tb_mode == fdtfQUERY)
	{
		/*
		**  If bare table field, just return nothing.
		*/
		nxaggrow = NULL;
		aggoffset = 0;
		aggchgoffset = 0;
	}
	else
	{
		col = tfld->tfflds[colno];
		colname = col->flhdr.fhdname;
		aggcolnum = col->flhdr.fhseq;
		aggcol = IIcdget(tb, colname);
		MEcopy((PTR) &(aggcol->c_dbv), (u_i2) sizeof(DB_DATA_VALUE),
			(PTR) &aggdbv);
		aggoffset = aggcol->c_offset;
		aggchgoffset = aggcol->c_chgoffset;
		nxaggrow = ds->top;

		/*
		**  Finally, sync up table field values with screen
		**  representation and do any necessary cross-column
		**  derivations by calling IIFDgcvGetDeriveVal.
		*/
		rownum_orig = frm->frres2->rownum;
		IIFDdsgDoSpecialGDV((bool)TRUE);
		for (i = 0, lastrow = tfld->tflastrow + 1; i < lastrow; i++)
		{
			frm->frres2->rownum = i;
			(VOID) IIFDgdvGetDeriveVal(frm, fld, colno);
		}
		IIFDdsgDoSpecialGDV((bool)FALSE);
		frm->frres2->rownum = rownum_orig;
	}
	return(OK);
}




/*{
** Name:	IITBanvAggNxVal - Return next valid value in dataset.
**
** Description:
**	This routine returns a pointer to a DBV containing a valid
**	value found in the dataset.  A NULL pointer is returned
**	where the dataset is exhausted.  Values are normally taken
**	from the dataset except where the dataset is visible
**	displayed in the table field.  In this case, the values
**	from the table field displayed are returned.  Note that
**	only values that are valid will be returned.
**
**	This routine assumes that a call to IITBasAggSetup() has
**	already been done to set up any cross column derivation values
**	and to synchronize the display with the internal values.
**
**	This routine is normally called by the derivation processing
**	routines via a call back function pointer (struct member
**	aggnxval) in the FRS_RFCB control block pointed to by
**	variable IIFRrfcb.

** Inputs:
**	dbvptr		Pointer to a DBV pointer.
**
** Outputs:
**
**	Returns:
**		dbvptr	NULL if there are no more valid values.
**			Set to point to a DBV containing a valid value.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08/07/89 (dkh) - Initial version.
*/
VOID
IITBanvAggNxVal(dbvptr)
DB_DATA_VALUE	**dbvptr;
{
	PTR	datarec;
	i4	flags;
	i4	good_value;
	TBLFLD	*tfld;
	FLDVAL	*val;
	i4	found_bottom = FALSE;

	/*
	**  Figure out the dbv and point to next row.
	*/
	for ( ; ; )
	{
		/*
		**  If no more data rows, return NULL.
		*/
		if (nxaggrow == NULL)
		{
			*dbvptr = NULL;
			break;
		}

		/*
		**  If we are to process the displayed portion of the
		**  dataset, set index (and flag) to indicate to
		**  get values from the table field itself.
		*/
		if (nxaggrow == aggtb->dataset->distop)
		{
			aggvisrow = 0;
		}

		/*
		**  Set flag if we are processing the displayed
		**  row.
		*/
		if (nxaggrow == aggtb->dataset->disbot)
		{
			found_bottom = TRUE;
		}

		/*
		**  We are to get data from the table field instead
		**  of the dataset.
		*/
		if (aggvisrow >= 0)
		{
			/*
			**  If on visible row, get data
			**  from table field.
			*/
			good_value = FALSE;
			tfld = aggtb->tb_fld;
			val = tfld->tfwins + (aggvisrow * tfld->tfcols) +
				aggcolnum;
			flags = *(tfld->tffflags + (aggvisrow * tfld->tfcols) +
				aggcolnum);

			if (flags & fdVALCHKED)
			{
				*dbvptr = val->fvdbv;
				good_value = TRUE;
			}

			/*
			**  If we just processed the last visibly
			**  displayed row, then reset back to dataset.
			*/
			if (found_bottom)
			{
				aggvisrow = -1;
			}
			else
			{
				/*
				**  Otherwise, increment to next row.
				*/
				aggvisrow++;
			}

			/*
			**  Increment dataset pointer as well to
			**  keep things in sync.
			*/
			nxaggrow = nxaggrow->nxtr;
			if (good_value)
			{
				break;
			}
		}
		else
		{
			/*
			**  Get data from dataset.
			*/
			datarec = nxaggrow->rp_data;

			/*
			**  Figure out if data is valid.
			*/
			flags = *(i4 *)((char *)datarec + aggchgoffset);

			/*
			**  If it is, then return the value.
			*/
			if (flags & fdVALCHKED)
			{
				aggdbv.db_data = (PTR) ((char *)datarec +
					aggoffset);
				*dbvptr = &aggdbv;
				nxaggrow = nxaggrow->nxtr;
				break;
			}
			/*
			**  Otherwise, look at next row.
			*/
			nxaggrow = nxaggrow->nxtr;
		}
	}
}
