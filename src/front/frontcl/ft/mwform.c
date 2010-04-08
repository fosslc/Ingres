/*
**	MWform.c
**	"@(#)mwform.c	1.42"
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include <compat.h>
# include <me.h>
# include <st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include <adf.h>
# include <fmt.h>
# include <ft.h>
# include <frame.h>
# include <fe.h>
# include "ftrange.h"
# include "mwproto.h"
# include "mwmws.h"
# include "mwhost.h"
# include "mwform.h"
# include "mwintrnl.h"

/**
** Name:	MWform.c - Host Frontend Routines to Support MacWorkStation.
**
** Usage:
**	INGRES FE system and MWS test host on Macintosh.
**
** Description:
**	Contains functions to display and manipulate forms.
**
**	Most of the time when a table in a form is built, all or most
**	of the cells do not have any text, and their flags are the
**	same.  In this case, message traffic to MWS is siginificantly
**	reduced if all these cells are combined into one message.
**	The static function put_tbl uses this approach.  Also, INGRES
**	often clears a table by clearing each cell.  Again, net
**	traffic is reduced by combining these into one message.
**	When IIMWufUpdFld is called for a table cell and there is
**	no text, no message is sent to MWS; instead the cell number
**	is remembered.  As long as there are more cells to be
**	cleared, and all the cells together can be defined by
**	specifying start_row, start_col & end_row, end_col, they
**	are saved on the host side.  If a cell that has text is to be
**	changed or if the cell does not fit into the area defined, all
**	the accumulated cells are sent to MWS.  The static functions
**	entry_fits, send_tbl_cells and start_recording facilitate
**	the implementation of this feature.
**
**	Routines defined here are:
**
**	High level forms display routines:
**	  - IIMWbfBldForm	Display a new form.
**	  - IIMWbqtBegQueryTbl	Start getting cell values of a query table.
**	  - IIMWcfCloseForms	Shut down; free memory.
**	  - IIMWdfDelForm	Remove a form from display.
**	  - IIMWdsmDoSkipMove	Implementation of fdopSKPFLD.
**	  - IIMWeqtEndQueryTbl	End getting cell values of a query table.
**	  - IIMWfcFldChanged	Find out whether field changed by user.
**	  - IIMWgrvGetRngVal	Get the range value of a table cell.
**	  - IIMWgscGetSkipCnt	Get the field user skipped to.
**	  - IIMWgvGetVal	Get the text of a reg. field or tbl cell.
**	  - IIMWfrvFreeRngVals	Discard the structures for ranges.
**	  - IIMWpvPutVal	Put the value of a fld in the frame data str.
**	  - IIMWrRefresh	Redisplay all objects within the given frame.
**	  - IIMWrdcRndDnCp	Scroll table down in range mode.
**	  - IIMWrfRstFld	Reset the field.
**	  - IIMWrrvRestoreRngVals Restore the text values of fields.
**	  - IIMWrucRngUpCp	Scroll table up in range mode.
**	  - IIMWsaSetAttr	Set display attributes of a field.
**	  - IIMWscfSetCurFld	Select a field.
**	  - IIMWshfShowHideFlds Tell MWS about visibility of fields in the frame.
**	  - IIMWsrvSaveRngVals	Save the text values of fields.
**	  - IIMWtvTblVis	Make a table field (in)visible.
**	  - IIMWufUpdFld	Change the text of a field.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
**	04/14/90 (dkh) - Eliminated use of "goto"'s where safe to do so.
**	05/22/90 (dkh) - Changed NULLPTR to NULL for VMS.
**	07/10/90 (dkh) - Integrated changes into r63 code line.
**	06/08/92 (fredb) - Enclosed file in 'ifdef DATAVIEW' to protect
**		ports that are not using MacWorkStation from extraneous
**		code and data symbols.  Had to include 'fe.h' to get
**		DATAVIEW (possibly) defined.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	11/04/97 (kitch01) - Bug 79872. entry_fits now takes total columns
**		as third parm. With this we can check to ensure that the
**		last cell held is the cell immeadiately prior to the cell
**		we are checking.
**      18-apr-97 (cohmi01)
**          Added IIMWrsbRefreshScrollButton for UPFRONT (bug 81574).
**	07/07/97 (kitch01)
**		Bug 81855 - Fix IIMWgvGetVal to prevent E_FI1FBF errors.
**	05-aug-97 (kitch01)
**		Bug 83587 - Fix IIMWsaSetAttr to send the DisplayOnly attribute.
**  23-dec-97 (kitch01)
**		Bugs 87709, 87808, 87812, 87841.
**		Added IIMWsiScrollbarInitialized and IIMWisrInitScrollbarReqd. Also
**		amended frm_aliases structure to contain new boolean x200_sent. 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# ifdef DATAVIEW

# define INCR_SZ	10
/* states of a frame */
# define FREE		0
# define DISPLAYED	1
# define SAVED		2


static	i4		 fld_changed = 0;
static  struct frm_rec {
	FRAME		*frm_ptr;
	i4		 state;
	bool	x200_sent;
	}		*frm_aliases = NULL;
static	u_i4		 num_frm_aliases = 0;
static	i4		 cur_fldno = -1;
static	FRAME		*cur_frm = NULL;

/* static vars for recording table cells */
static	i4		 rec_frm_alias = -1;
static	i4		 rec_item_alias = 0;
static	i4		 start_row = 0;
static	i4		 start_col = 0;
static	i4		 end_row = 0;
static	i4		 end_col = 0;
static	i4		 rec_flags = 0;

/* static vars to support skip field */
static	i4		 new_fldno = 0;
static	i4		 new_row = 0;
static	i4		 new_col = 0;

/* static vars to support getting table cells for query tables */
static	bool		 qry_tbl_save = FALSE;
static	i4		 qry_frm_alias = -1;
static	i4		 qry_item_alias = 0;
static	i4		 qry_beg_row = 0;
static	i4		 qry_beg_col = 0;
static	i4		 qry_end_row = 0;
static	i4		 qry_end_col = 0;
static	char		 qry_buf[4096] = {0};
				/* we assume buf is large enough */

static  i4		 get_alias();
static	i4	         new_alias();
static	STATUS		 put_tbl();

/* local functions for recording or sending table cells */
static	i4		 entry_fits();
static	STATUS		 send_tbl_cells();
static	VOID		 start_recording();

static	STATUS		 show_hide();

GLOBALREF       MWSinfo IIMWVersionInfo;

/*{
** Name:  IIMWbfBldForm -- Display a new form.
**
** Description:
**	Put a new form on the display.
**
** Inputs:
**	frm	Pointer to the new form.
**
** Outputs:
** 	Returns:
**		OK/FAIL.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	20-sep-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWbfBldForm(frm)
FRAME	*frm;
{
	STATUS	retval;
	i4	frm_alias;
	i4 i;
	TRIM	*theTrim;

	if ( ! IIMWmws)
		return(OK);

	frm_alias = get_alias(frm, SAVED);
	if (frm_alias == -1)
	{
		frm_alias = new_alias(frm);
		if (frm_alias == -1)
			return(FAIL);
	}

	if (IIMWafAddForm(frm_alias,frm) == FAIL)
	{
		return(FAIL);
	}

	retval = OK;
	for (i = 0; i < frm->frtrimno; i++)
	{
		theTrim = frm->frtrim[i];
		if (theTrim->trmflags & fdBOXFLD)
			if (IIMWatrAddTRim(frm_alias,
				i + TRIMINX, theTrim) == FAIL)
		{
			retval = FAIL;
			break;
		}
	}

	if (retval == OK)
	{
	    for (i = 0; i < frm->frtrimno; i++)
	    {
		theTrim = frm->frtrim[i];
		if (!(theTrim->trmflags & fdBOXFLD))
			if (IIMWatrAddTRim(frm_alias,
				i + TRIMINX, theTrim) == FAIL)
		{
			retval = FAIL;
			break;
		}
	    }
	}

	if (retval == OK)
	{
	    for (i = 0; i < frm->frfldno; i++)
	    {
		if ((frm->frfld[i])->fltag == FTABLE)
		{
			if (put_tbl(frm_alias, i + FLDINX,
				(frm->frfld[i])->fld_var.fltblfld) == FAIL)
			{
				retval = FAIL;
				break;
			}
		}
		else
		{
			if (IIMWafiAddFieldItem(frm_alias, i + FLDINX,
				(frm->frfld[i])->fld_var.flregfld) == FAIL)
			{
				retval = FAIL;
				break;
			}
		}
	    }
	}

	if (retval == OK)
	{
	    for (i = 0; i < frm->frnsno; i++)
	    {
		if ((frm->frnsfld[i])->fltag == FTABLE)
		{
			if (put_tbl(frm_alias, i + NSNOINX,
				(frm->frnsfld[i])->fld_var.fltblfld) == FAIL)
			{
				retval = FAIL;
				break;
			}
		}
		else
		{
			if (IIMWafiAddFieldItem(frm_alias, i + NSNOINX,
				(frm->frnsfld[i])->fld_var.flregfld) == FAIL)
			{
				retval = FAIL;
				break;
			}
		}
	    }
	}

	if (retval == OK && IIMWsfShowForm(frm_alias) == OK)
	{
		frm_aliases[frm_alias].state |= DISPLAYED;
		/* Bugs 87709, 87808, 87812, 87841.*/
		frm_aliases[frm_alias].x200_sent = FALSE;
		return(OK);
	}

	_VOID_ IIMWtfTossForm(frm_alias);
	return(FAIL);
}

/*{
** Name:  IIMWbqtBegQueryTbl -- Start getting cell values of a query table.
**
** Description:
**	Enable the mode whereby when querying for the value of a
**	cell in a query table, we find out that the vaule is null,
**	MWS has returned the value of the next non-null cell.
**	Record this value, and use it to return values of cells
**	in subsequent calls.
**	This call is complemented by IIMWeqtEndQueryTbl. Calls to
**	these functions can be nested.
**
** Inputs:
**	None.
**
** Outputs:
** 	Returns:
**		STATUS
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	07-jul-90 (nasser)
**		Initial definition.
*/
STATUS
IIMWbqtBegQueryTbl()
{
	if (! IIMWmws)
		return(OK);

	qry_tbl_save++;

	return(OK);
}

/*{
** Name:  IIMWcfCloseForms -- End the forms module
**
** Description:
**	Free all memory used by this module.
**
** Inputs:
**	None.
**
** Outputs:
** 	Returns:
**		None.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	06-oct-89 (nasser)
**		Initial definition.
*/
VOID
IIMWcfCloseForms()
{
	_VOID_ MEfree((PTR) frm_aliases);
	frm_aliases = NULL;
	num_frm_aliases = 0;
	rec_frm_alias = -1;
}

/*{
** Name:  IIMWdfDelForm -- Delete a form.
**
** Description:
**	Send command to toss form.
**
** Inputs:
**	frm	Pointer to form to be deleted.
**
** Outputs:
** 	Returns:
**		STATUS.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	20-sep-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWdfDelForm(frm)
FRAME	*frm;
{
	i4	alias;

	if ( ! IIMWmws)
		return(OK);

	alias = get_alias(frm, DISPLAYED);
	if (alias == -1)
	{
		IIMWpelPrintErrLog(mwERR_ALIAS, "DelForm");	
		return(FAIL);
	}

	if (rec_frm_alias == alias)
		rec_frm_alias = -1;

	frm_aliases[alias].state &= ~DISPLAYED;
	return(IIMWtfTossForm(alias));
}

/*{
** Name:  IIMWdsmDoSkipMove -- Implementation of fdopSKPFLD.
**
** Description:
**	IIMWdsmDoSkipMove implements the functionality needed for
**	the fdopSKPFLD operation.  On the Mac, the user can click
**	on any field to indicate that he wants to go there.  The
**	location of the new field is gotten by calling
**	IIMWgscGetSkipCnt() which must have been invoked before
**	calling IIMWdsmDoSkipMove.
**	IIMWdsmDoSkipMove first determines if the user is allowed
**	to go to the new field.  If so, it does validation check
**	if required, and then sets the values for the new field.
**	I obtained the logic for the checking from the FTmove(),
**	FTuprow() and FTdownrow() functions.
**
** Inputs:
**	frame		Ptr to info about the frame
**	fldno		Ptr to the frame nbr
**	dispmode	Current mode FT is in
**	rowtrv		Function to call for row validation
**	valfld		Function ptr to pass to rowtrv
**
** Outputs:
**	fldno	The new field nbr.  May be unchanged
** 	Returns:
**		FAIL	If cannot move to new field or validation fails
**		OK	If move succeeded
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	16-dec-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWdsmDoSkipMove(frm, fldno, dispmode, rowtrv, valfld)
FRAME	*frm;
i4	*fldno;
i4	 dispmode;
i4	(*rowtrv)();
i4	(*valfld)();
{
	i4	 old_fldno;
	FIELD	*new_fld;
	FIELD	*old_fld;
	TBLFLD	*old_tbl;
	TBLFLD	*new_tbl;
	FLDHDR	*hdr;
	bool	 qrymd;
	i4	 tqry;

	if ( ! IIMWmws)
		return(OK);

	old_fldno = *fldno;
	new_fld = frm->frfld[new_fldno];
	old_fld = frm->frfld[old_fldno];
	if (old_fld->fltag == FTABLE)
		old_tbl = old_fld->fld_var.fltblfld;

	if (new_fld->fltag == FREGULAR)
	{
		hdr = &(new_fld->fld_var.flregfld->flhdr);
		/*
		**  Check if user can go to this field.
		**  This is the same check as performed by FTmove()
		**  for next field or previous field operation.
		*/
		qrymd = ((dispmode == fdmdQRY) || (dispmode == fdmdADD));
		if ((hdr->fhd2flags & fdREADONLY) ||
			( ! qrymd && (hdr->fhdflags & fdQUERYONLY)))
		{
			return(FAIL);
		}
		if (old_fld->fltag == FTABLE)
		{
			/*
			**  If leaving a table field, do row validation
			**  This check is performed by FTuprow() and
			**  FTdownrow().
			*/
			if ( ! (*rowtrv)(old_tbl, old_tbl->tfcurrow,
				valfld, TRUE))
			{
				return(FAIL);
			}
			old_tbl->tfcurrow = 0;
			old_tbl->tfcurcol = 0;
		}
	}
	else
	{
		new_tbl = new_fld->fld_var.fltblfld;
		/*
		**  Check if user can go to this field.
		**  This is the same check as performed by FTmove()
		**  for next field or previous field operation.
		*/
		tqry = (new_tbl->tfhdr.fhdflags & fdtfQUERY) ||
			(new_tbl->tfhdr.fhdflags &fdtfAPPEND);
		/* get col_hdr */
		hdr = &(new_tbl->tfflds[new_tbl->tfcurcol]->flhdr);
		if ((hdr->fhdflags & fdtfCOLREAD) ||
			(hdr->fhd2flags & fdREADONLY) ||
			( ! tqry && hdr->fhdflags & fdQUERYONLY))
		{
			return(FAIL);
		}
		/*
		**  If leaving a table field or row, do row validation
		**  This check is performed by FTuprow() and FTdownrow().
		*/
		if ((old_fld->fltag == FTABLE) &&
			((old_fldno != new_fldno) ||
				(old_tbl->tfcurrow != new_row)))
		{
			if ( ! (*rowtrv)(old_tbl, old_tbl->tfcurrow,
				valfld, TRUE))
			{
				return(FAIL);
			}
			if (old_fldno != new_fldno)
			{
				old_tbl->tfcurrow = 0;
				old_tbl->tfcurcol = 0;
			}
		}
		new_tbl->tfcurrow = new_row;
		new_tbl->tfcurcol = new_col;
	}
	*fldno = new_fldno;
	return(OK);
}

/*{
** Name:  IIMWeqtEndQueryTbl -- End getting cell values of a query table.
**
** Description:
**	Disable the mode whereby when querying for the value of a
**	cell in a query table, we find out that the vaule is null,
**	MWS has returned the value of the next non-null cell.
**	This call is complemented by IIMWbqtBegQueryTbl. Calls to
**	these functions can be nested.
**
** Inputs:
**	None.
**
** Outputs:
** 	Returns:
**		STATUS
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	07-jul-90 (nasser)
**		Initial definition.
*/
STATUS
IIMWeqtEndQueryTbl()
{
	if (! IIMWmws)
		return(OK);
	
	if (qry_tbl_save <= 0)
		return(FAIL);

	qry_tbl_save--;
	if (qry_tbl_save == 0)
		qry_frm_alias = -1;
	
	return(OK);
}

/*{
** Name:  IIMWfcFldChanged -- Return info whether the field changed.
**
** Description:
**	Return change-bit for the last field user manipulated.
**
** Inputs:
**	chgbit	change-bit for the field.	
**
** Outputs:
**	chgbit	change-bit for the field reflecting user-manipulation.
** 	Returns:
**		None.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	20-sep-89 (nasser)
**		Initial definition.
*/
VOID
IIMWfcFldChanged(chgbit)
i4	*chgbit;
{
	if ( ! IIMWmws)
		return;

	if (fld_changed)
	{
		*chgbit |= (fdI_CHG | fdX_CHG);
		*chgbit &= ~fdVALCHKED;
		fld_changed = FALSE;
	}
}

/*{
** Name:  IIMWgrvGetRngVal -- Get the value of a range field.
**
** Description:
**	Get the value of a range field.
**
** Inputs:
**	frm	Pointer to frame
**	fldno	Field number
**	tbl	Pointer to table field if it is a table
**
** Outputs:
**	buf	Buffer the value is returned in
** 	Returns:
**		STATUS
** 	Exceptions:
**		None
**
** Side Effects:
**
** History:
**	20-sep-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWgrvGetRngVal(frm, fldno, tbl, buf)
FRAME	*frm;
i4	 fldno;
TBLFLD	*tbl;
u_char	*buf;
{
	i4	 frm_alias;
	i4	 item_alias;
	RGFLD	*range;
	RGFLD	*rgptr;
	RGTFLD	*trgptr;
	i4	 col;
	i4	 row;
	i4	 col_returned;
	i4	 row_returned;
	STATUS	 stat;

	if ( ! IIMWmws)
		return(OK);

	frm_alias = get_alias(frm, DISPLAYED);
	if (frm_alias == -1)
	{
		IIMWpelPrintErrLog(mwERR_ALIAS, "GetRngVal");
		return(FAIL);
	}
	item_alias = fldno + FLDINX;

	if (frm->frfld[fldno]->fltag == FREGULAR)
	{
		stat = IIMWgitGetItemText(frm_alias, item_alias, buf);
	}
	else
	{
		range = (RGFLD *) frm->frrngptr;
		rgptr = &(range[fldno]);
		trgptr = rgptr->rg_var.rgtblfld;
		col = tbl->tfcurcol;
		row = tbl->tfcurrow + trgptr->rgtoprow;

		if (qry_tbl_save && (frm_alias == qry_frm_alias) &&
		    (item_alias == qry_item_alias))
		{
			if ((col == qry_end_col) && (row == qry_end_row))
			{
				STcopy(qry_buf, (char *) buf);
				return(OK);
			}
			else if ((row == qry_end_row) && (row == qry_beg_row))
			{
				if ((col >= qry_beg_col) && (col < qry_end_col))
				{
					buf[0] = NULLCHAR;
					return(OK);
				}
			}
			else if ((row == qry_end_row) &&
				 (col >= 0) && (col < qry_end_col))
			{
				buf[0] = NULLCHAR;
				return(OK);
			}
			else if ((row == qry_beg_row) &&
				 (col >= qry_beg_col) && (col <= tbl->tfcols))
			{
				buf[0] = NULLCHAR;
				return(OK);
			}
			else if ((row > qry_beg_row) && (row < qry_end_row))
			{
				buf[0] = NULLCHAR;
				return(OK);
			}
		}

		col_returned = col;
		row_returned = row;
		stat = IIMWgttGetTbcellText(frm_alias, item_alias,
					    &col_returned, &row_returned,
					    TRUE, buf);
		if ((stat == OK) &&
		    ((col_returned != col) || (row_returned != row)))
		{
			if (qry_tbl_save)
			{
				STtrmwhite((char *) buf);
				STcopy((char *) buf, qry_buf);
				qry_frm_alias = frm_alias;
				qry_item_alias = item_alias;
				qry_beg_col = col;
				qry_beg_row = row;
				qry_end_col = col_returned;
				qry_end_row = row_returned;
			}
			buf[0] = '\0';
		}
	}

	if (stat == OK)
		STtrmwhite((char *) buf);
	return(stat);
}

/*{
** Name:  IIMWgscGetSkipCnt -- Get the field user skipped to.
**
** Description:
**	Find out which field the user wanted to go to.  Set static
**	variables to remember, and return a rough skip count to
**	the caller.  The skip count can be used by the caller
**	to figure out whether to skip forward or backward, and
**	should be used for no other purpose.
**
** Inputs:
**	frm		Ptr to info about the frame
**	old_fldno	field nbr. user was in
**
** Outputs:
**	cnt		rough count of field to skip
** 	Returns:
**		STATUS
** 	Exceptions:
**		None.
**
** Side Effects:
**	Set static variables for the new field number and
**	new row and column numbers.
**
** History:
**	16-dec-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWgscGetSkipCnt(frm, old_fldno, cnt)
FRAME	*frm;
i4	 old_fldno;
i4	*cnt;
{
	i4	 frm_alias;
	i4	 item_alias;
	FIELD	*fld;
	TBLFLD	*tbl;

	if ( ! IIMWmws)
		return(OK);

	frm_alias = get_alias(frm, DISPLAYED);
	if (frm_alias == -1)
	{
		IIMWpelPrintErrLog(mwERR_ALIAS, "GetSkipCnt");
		return(FAIL);
	}

	if (IIMWgciGetCurItem(&frm_alias, &item_alias,
		&new_col, &new_row) != OK)
	{
		return(FAIL);
	}
	new_fldno = item_alias - FLDINX;
	fld = frm->frfld[old_fldno];
	if (new_fldno != old_fldno)
		*cnt = new_fldno - old_fldno;
	else if (fld->fltag != FTABLE)
	{
		*cnt = 0;
	}
	else
	{
		tbl = fld->fld_var.fltblfld;
		if (new_row != tbl->tfcurrow)
			*cnt = new_row - tbl->tfcurrow;
		else if (new_col != tbl->tfcurcol)
			*cnt = new_col - tbl->tfcurcol;
		else
			*cnt = 0;
	}

	return(OK);
}

/*{
** Name:  IIMWgvGetVal -- Get the value of a field.
**
** Description:
**	Get the value of a cell in a table; always cur_fldno.
**
** Inputs:
**	frm	Pointer to the frame
**	tbl	Pointer to table field if it is a table
**
** Outputs:
** 	Returns:
**		STATUS
** 	Exceptions:
**		None.
**
** Side Effects:
**	The text value is updated in the tbl data structure.
**
** History:
**	20-sep-89 (nasser)
**		Initial definition.
**	07-jul-97 (kitch01)
**		Bug 81855 - Add a buffer area to receive the value from
**		the tablefield cell. This is required because when the 
**		value is received it is null terminated, but val->fvbufr
**		does not hold null terminated values. I also amended the
**		the stat check as the previous version would never work.
*/
STATUS
IIMWgvGetVal(frm, tbl)
FRAME	*frm;
TBLFLD	*tbl;
{
	i4	 frm_alias;
	i4	 item_alias;
	i4	 col;
	i4	 row;
	i4	 col_returned;
	i4	 row_returned;
	FLDVAL	*val;
	STATUS	 stat;
	char	buf[4096];
	int		buf_len;

	if ( ! IIMWmws)
		return(OK);

	frm_alias = get_alias(frm, DISPLAYED);
	if (frm_alias == -1)
	{
		IIMWpelPrintErrLog(mwERR_ALIAS, "GetVal");
		return(FAIL);
	}

	item_alias = cur_fldno + FLDINX;
	col = tbl->tfcurcol;
	row = tbl->tfcurrow;
	col_returned = col;
	row_returned = row;
	val = tbl->tfwins + (row * tbl->tfcols) + col;
	stat = IIMWgttGetTbcellText(frm_alias, item_alias,
				    &col_returned, &row_returned,
					FALSE, buf);

	if (stat == OK)
	{
		buf_len = STlength(buf);
		MEcopy(buf, buf_len, (char *)val->fvbufr);
		((DB_TEXT_STRING *) val->fvdsdbv->db_data)->db_t_count = buf_len;
	}
	else
	{
		*(val->fvbufr) = NULLCHAR;
		((DB_TEXT_STRING *) val->fvdsdbv->db_data)->db_t_count = 0;
	}

	return(stat);
}

/*{
** Name:  IIMWfrvFreeRngVals -- Discard the structures for ranges.
**
** Description:
**	Send command to discard the structures used for saving
**	range values for a range-mode form.
**
** Inputs:
**	frm	Pointer to form.
**
** Outputs:
** 	Returns:
**		STATUS.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	05-dec-89 (nasser)
**		Initial definition.
**	13-jul-90 (nasser)
**		Added check to make sure there is at least field
**		in the form.
**	13-jul-90 (nasser)
**		Get the alias for the frame from the saved frames
**		rather than from the general list of frames.
*/
STATUS
IIMWfrvFreeRngVals(frm)
FRAME	*frm;
{
	i4	alias;

	if ( ! IIMWmws)
		return(OK);
	if (frm->frfldno < 1)
		return(OK);

	alias = get_alias(frm, SAVED);
	if (alias == -1)
	{
		IIMWpelPrintErrLog(mwERR_ALIAS, "FreeRngVals");	
		return(FAIL);
	}
	frm_aliases[alias].state &= ~SAVED;

	return(IIMWfftFreeFldsText(alias));
}

/*{
** Name:  IIMWpvPutVal -- Update the value of fld. in the frame data str.
**
** Description:
**	After the user has manipulated a field of a form, he may have
**	changed the value.  If he does change the value, this function
**	is called to update the frame data structure.
**	The frame and field are given by the values of cur_frm and
**	cur_fldno.
**
** Inputs:
**	changed		TRUE if field was changed by user.
**	buf		The changed text of the field.
**
** Outputs:
** 	Returns:
**		None.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	17-oct-89 (nasser)
**		Initial definition.
**    24-oct-1996 (angusm)
**            The means for copying the data from buf is wrong:
**            overshoots by one byte, corrupting data pointer
**            for subsequent column (bug 77265).
*/
VOID
IIMWpvPutVal(changed, buf)
i4	 changed;
char	*buf;
{
	FIELD	*fld;
	TBLFLD	*tbl;
	FLDVAL	*val;
	u_i2	len1;

	if ( ! changed)
		return;

	fld_changed = changed;

	cur_frm->frchange = TRUE;

	/* don't update value in frame data if range mode */
	if (cur_frm->frmflags & fdRNGMD)
		return;

	fld = cur_frm->frfld[cur_fldno];
	if (fld->fltag != FTABLE)
	{
		val = &(fld->fld_var.flregfld->flval);
	}
	else
	{
		tbl = fld->fld_var.fltblfld;
		val = tbl->tfwins + (tbl->tfcurrow * tbl->tfcols) + 
			tbl->tfcurcol;
	}
	len1 = STlength(buf);
	MEcopy((PTR)buf, len1, (PTR) val->fvbufr);
	((DB_TEXT_STRING *) val->fvdsdbv->db_data)->db_t_count = len1;
}

/*{
** Name:	IIMWrRefresh - Redisplay all objects within the given frame.
**
** Description:
**	Redisplay all objects within the given frame. 
**	
** Inputs:
**	None.
**
** Outputs:
**	Returns:
**		STATUS.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
*/
STATUS
IIMWrRefresh()
{
	i4	frm_alias;

	if ( ! IIMWmws)
		return(OK);

	frm_alias = get_alias(cur_frm, DISPLAYED);
	if (frm_alias == -1)
	{
		IIMWpelPrintErrLog(mwERR_ALIAS, "Refresh");
		return(FAIL);
	}

	return(IIMWsfShowForm(frm_alias));
}

/*{
** Name:  IIMWrdcRngDnCp -- Copy info down in a table field.
**
** Description:
**	Copy information down in a table field.  Aka scrolling
**	down in a table field.
**
** Inputs:
**	frm	Pointer to the frame
**	fldno	Field number
**	count	Count of rows to copy
**
** Outputs:
** 	Returns:
**		OK/FAIL.
** 	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	20-sep-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWrdcRngDnCp(frm, fldno, count)
FRAME	*frm;
i4	 fldno;
i4	 count;
{
	i4	frm_alias;
	i4	item_alias;

	if ( ! IIMWmws)
		return(OK);

	frm_alias = get_alias(frm, DISPLAYED);
	if (frm_alias == -1)
	{
		IIMWpelPrintErrLog(mwERR_ALIAS, "RngDnCp");
		return(FAIL);
	}
	item_alias = fldno + FLDINX;
	return(IIMWstfScrollTblFld(frm_alias, item_alias, -count));
}

/*{
** Name:  IIMWrfRstFld -- Send command to reset the field.
**
** Description:
**	Reset the field so that the initial part of the field is
**	displayed and the cursor is at the first position within
**	the field.
**
** Inputs:
**	frm	Pointer to frame
**	fldno	Field number
**	tbl	Pointer to table field if it is one
**
** Outputs:
** 	Returns:
**		STATUS.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	20-sep-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWrfRstFld(frm, fldno, tbl)
FRAME	*frm;
i4	 fldno;
TBLFLD	*tbl;
{
	i4	frm_alias;
	i4	item_alias;
	i4	col;
	i4	row;

	if ( ! IIMWmws)
		return(OK);

	frm_alias = get_alias(frm, DISPLAYED);
	if (frm_alias == -1)
	{
		IIMWpelPrintErrLog(mwERR_ALIAS, "RstFld");
		return(FAIL);
	}
	item_alias = fldno + FLDINX;
	if (tbl == NULL)
	{
		col = -1;
		row = -1;
	}
	else
	{
		col = tbl->tfcurcol;
		row = tbl->tfcurrow;
	}
	return(IIMWprfPRstFld(frm_alias, item_alias, col, row));
}

/*{
** Name:  IIMWrrvRestoreRngVals -- Restore the text values of fields.
**
** Description:
**	Send command to restore the text values of all fields
**	in the range-mode form.
**
** Inputs:
**	frm	Pointer to form.
**
** Outputs:
** 	Returns:
**		STATUS.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	05-dec-89 (nasser)
**		Initial definition.
**	13-jul-90 (nasser)
**		Added check to make sure there is at least field
**		in the form.
*/
STATUS
IIMWrrvRestoreRngVals(frm)
FRAME	*frm;
{
	i4	alias;

	if ( ! IIMWmws)
		return(OK);
	if (frm->frfldno < 1)
		return(OK);

	alias = get_alias(frm, DISPLAYED|SAVED);
	if (alias == -1)
	{
		IIMWpelPrintErrLog(mwERR_ALIAS, "RestoreRngVals");	
		return(FAIL);
	}

	/* discard any table cell update commands that are pending */
	rec_frm_alias = -1;

	return(IIMWrftRestoreFldsText(alias));
}

/*{
** Name:  IIMWrucRngUpCp -- Copy info up in a table field.
**
** Description:
**	Copy information up in a table field.  Aka scrolling info up
**	in a table field.
**
** Inputs:
**	frm	Pointer to the frame
**	fldno	Field number
**	count	Count of rows to copy
**
** Outputs:
** 	Returns:
**		OK/FAIL.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	20-sep-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWrucRngUpCp(frm, fldno, count)
FRAME	*frm;
i4	 fldno;
i4	 count;
{
	i4	frm_alias;
	i4	item_alias;

	if ( ! IIMWmws)
		return(OK);

	frm_alias = get_alias(frm, DISPLAYED);
	if (frm_alias == -1)
	{
		IIMWpelPrintErrLog(mwERR_ALIAS, "RngUpCp");
		return(FAIL);
	}
	item_alias = fldno + FLDINX;
	return(IIMWstfScrollTblFld(frm_alias, item_alias, count));
}

/*{
** Name:  IIMWsaSetAttr -- Set the attributes of a field.
**
** Description:
**	Set the display attributes of a field.
**
** Inputs:
**	frm		Pointer to frame
**	fldno		Field number
**	disponly	Boolean value indicating whether sequential field
**	col		Column number if it is a table field
**	row		Row number if it is a table field
**	attr		Value of new attributes
**
** Outputs:
** 	Returns:
**		STATUS
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	20-sep-89 (nasser)
**		Initial definition.
**	05-aug-97 (kitch01)
**		Ensure that we send the DisplayOnly (readonly) attribute to Upfront
**		This has meaning for a Windows app. 
*/
STATUS
IIMWsaSetAttr(frm, fldno, disponly, col, row, attr)
FRAME	*frm;
i4	 fldno;
i4	 disponly;
i4	 col;
i4	 row;
i4	 attr;
{
	i4	 frm_alias;
	i4	 item_alias;
	FIELD	*fld;
	TBLFLD	*tbl;
	FLDCOL	*column;

	if ( ! IIMWmws)
		return(OK);

	frm_alias = get_alias(frm, DISPLAYED);
	if (frm_alias == -1)
	{
		IIMWpelPrintErrLog(mwERR_ALIAS, "SetAttr");
		return(FAIL);
	}

	if (disponly == FT_UPDATE)
	{
		fld = frm->frfld[fldno];
		item_alias = fldno + FLDINX;
	}
	else
	{
		fld = frm->frnsfld[fldno];
		item_alias = fldno + NSNOINX;
	}

	if (fld->fltag != FTABLE)
	{
		col = -1;
		row = -1;
	/* Move flags from fhd2flags */
		if (fld->fld_var.flregfld->flhdr.fhd2flags & fdSCRLFD)
			attr |= mwSCRLFD;
		if (fld->fld_var.flregfld->flhdr.fhd2flags & fdREADONLY)
			attr |= mwREADONLY;
	}
	else
	{
		tbl = fld->fld_var.fltblfld;
		column = tbl->tfflds[col];
			/* Move flags from fhd2flags */
		if (column->flhdr.fhd2flags & fdSCRLFD)
			attr |= mwSCRLFD;
		if (column->flhdr.fhd2flags & fdREADONLY)
			attr |= mwREADONLY;
	}

	return(IIMWsiaSetItemAttr(frm_alias, item_alias, col, row, attr));
}

/*{
** Name:  IIMWscfSetCurFld -- Set current field.
**
** Description:
**	Set the current frame and field.  This is the field which
**	the user will be able to manipulate when DoUsrInput is
**	called.
**	If the current field is a table field, the current column
**	and row numbers are contained in the frame data structure.
**
** Inputs:
**	frm	Pointer to frame
**	fldno	Field number
**
** Outputs:
** 	Returns:
**		STATUS.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	20-sep-89 (nasser)
**		Initial definition.
**	16-oct-89 (nasser)
**		Filled in the meat.
*/
STATUS
IIMWscfSetCurFld(frm, fldno)
FRAME	*frm;
i4	 fldno;
{
	i4	 frm_alias;
	i4	 item_alias;
	FIELD	*fld;
	STATUS	 stat;

	if ( ! IIMWmws)
		return(OK);

	frm_alias = get_alias(frm, DISPLAYED);
	if (frm_alias == -1)
	{
		IIMWpelPrintErrLog(mwERR_ALIAS, "SetCurFld");
		return(FAIL);
	}

	if (send_tbl_cells(FALSE) == FAIL)
		return(FAIL);

	cur_frm = frm;
	cur_fldno = fldno;

	fld = frm->frfld[fldno];
	item_alias = fldno + FLDINX;

	if (fld->fltag != FTABLE)
	{
		stat = IIMWsciSetCurItem(frm_alias, item_alias,
			frm, fld->fld_var.flregfld);
	}
	else
	{
		stat = IIMWsctSetCurTbcell(frm_alias, item_alias,
			frm, fld->fld_var.fltblfld);
	}
	return(stat);
}

/*{
** Name:  IIMWshfShowHideFlds -- Tell MWS about the visibility
**				 of fields in a frame.
**
** Description:
**	The visibility of the fields in a frame may have changed.
**	Tell MWS to show/hide each field as indicated by the
**	flags.
**
** Inputs:
**	frm	Pointer to the form.
**
** Outputs:
** 	Returns:
**		OK/FAIL.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	14-nov-89 (nasser) -- Initial definition.
**	13-sep-90 (nasser) -- Divided stuff into show_hide().
*/
STATUS
IIMWshfShowHideFlds(frm)
FRAME	*frm;
{
	i4	 frm_alias;
	i4	 i;

	if ( ! IIMWmws)
		return(OK);

	frm_alias = get_alias(frm, DISPLAYED);
	if (frm_alias == -1)
		return(FAIL);

	for (i = 0; i < frm->frfldno; i++)
	{
		if (show_hide(frm_alias, i + FLDINX, frm->frfld[i]) != OK)
			return(FAIL);
	}
	for (i = 0; i < frm->frnsno; i++)
	{
		if (show_hide(frm_alias, i + NSNOINX, frm->frnsfld[i]) != OK)
			return(FAIL);
	}

	return(OK);
}

/*{
** Name:  IIMWsrvSaveRngVals -- Save the text values of fields.
**
** Description:
**	Send command to save the text values of all fields
**	in the range-mode form.
**
** Inputs:
**	frm	Pointer to form.
**
** Outputs:
** 	Returns:
**		STATUS.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	05-dec-89 (nasser)
**		Initial definition.
**	13-jul-90 (nasser)
**		Added check to make sure there is at least field
**		in the form.
**	13-jul-90 (nasser)
**		Save the alias number for the frame.
*/
STATUS
IIMWsrvSaveRngVals(frm)
FRAME	*frm;
{
	i4	alias;

	if ( ! IIMWmws)
		return(OK);
	if (frm->frfldno < 1)
		return(OK);

	alias = get_alias(frm, DISPLAYED);
	if (alias == -1)
	{
		IIMWpelPrintErrLog(mwERR_ALIAS, "SaveRngVals");	
		return(FAIL);
	}

	frm_aliases[alias].state |= SAVED;

	/* send any table cell update commands that are pending */
	if (send_tbl_cells(FALSE) == FAIL)
		return(FAIL);

	return(IIMWsftSaveFldsText(alias));
}

/*{
** Name:  IIMWtvTblVis -- Make a table field visible or invisible.
**
** Description:
**	Make table field identified by "fldno" in frame "frm"
**	visible or invisible.
**
** Inputs:
**	frm		Pointer to frame
**	fldno		Field number
**	disponly	Value indicating whether field can be updated
**	display		Boolean whether to make field visible/invisible
**
** Outputs:
** 	Returns:
**		STATUS
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	20-sep-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWtvTblVis(frm, fldno, disponly, display)
FRAME	*frm;
i4	 fldno;
i4	 disponly;
i4	 display;
{
	i4	frm_alias;
	i4	item_alias;
	STATUS	stat;

	if ( ! IIMWmws)
		return(OK);

	frm_alias = get_alias(frm, DISPLAYED);
	if (frm_alias == -1)
	{
		IIMWpelPrintErrLog(mwERR_ALIAS, "TblVis");
		return(FAIL);
	}
	if (disponly == FT_UPDATE)
		item_alias = fldno + FLDINX;
	else
		item_alias = fldno + NSNOINX;
	if (display)
		stat = IIMWsiShowItem(frm_alias, item_alias, -1);
	else
		stat = IIMWhiHideItem(frm_alias, item_alias, -1);

	return(stat);
}

/*{
** Name:  IIMWufUpdFld -- Update the value of a field.
**
** Description:
**	Take whatever is in the dispaly buffer for the specified
**	filed and send the updated information to the MacWorkStation.
**
** Inputs:
**	frm		Pointer to form containing field for update
**	fldno		Field number of field for update
**	disponly	Indicates whether field is updatable
**	col		Column number if field is a table field
**	row		Row number if field is a table field
**
** Outputs:
** 	Returns:
**		STATUS
** 	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	20-sep-89 (nasser)
**		Initial definition.
*/
STATUS
IIMWufUpdFld(frm, fldno, disponly, col, row)
FRAME	*frm;
i4	 fldno;
i4	 disponly;
i4	 col;
i4	 row;
{
	i4		 frm_alias;
	i4		 item_alias;
	FIELD		*fld;
	TBLFLD		*tbl;
	FLDVAL		*val;
	DB_TEXT_STRING	*text;
	STATUS		 stat;

	if ( ! IIMWmws)
		return(OK);

	frm_alias = get_alias(frm, DISPLAYED);
	if (frm_alias == -1)
	{
		IIMWpelPrintErrLog(mwERR_ALIAS, "UpdFld");
		return(FAIL);
	}

	if (disponly == FT_UPDATE)
	{
		fld = frm->frfld[fldno];
		item_alias = fldno + FLDINX;
	}
	else
	{
		fld = frm->frnsfld[fldno];
		item_alias = fldno + NSNOINX;
	}
	if (fld->fltag != FTABLE)
	{
		val = &(fld->fld_var.flregfld->flval);
		text = (DB_TEXT_STRING *) val->fvdsdbv->db_data;
		stat = IIMWsitSetItemText(frm_alias, item_alias,
			text->db_t_count, val->fvbufr);
	}
	else if ((row != -1) && (col != -1))
	{
		tbl = fld->fld_var.fltblfld;
		val = tbl->tfwins + (row * tbl->tfcols) + col;
		text = (DB_TEXT_STRING *) val->fvdsdbv->db_data;
		if (text->db_t_count != 0)
		{
			if (send_tbl_cells(FALSE) != OK)
				return(FAIL);
			stat = IIMWsttSetTbcellText(frm_alias, item_alias,
				col, row, text->db_t_count, val->fvbufr);
		}
		else if (rec_frm_alias == -1)
		{
			start_recording(frm_alias, item_alias, col, row);
		}
		else if ((frm_alias != rec_frm_alias) ||
			 (item_alias != rec_item_alias) ||
			 ( ! entry_fits(col, row, tbl->tfcols)))
		{
			if (send_tbl_cells(FALSE) != OK)
				return(FAIL);
			start_recording(frm_alias, item_alias, col, row);
		}
	}

	return(stat);
}

/*{
** Name:  get_alias -- get the alias of a frame.
**
** Description:
**	Find out the alias of a frame given by the frame pointer 'frm.'
**	Note that the aliases used are 1-based.
**
** Inputs:
**	frm	Pointer to frame
**
** Outputs:
** 	Returns:
**		Alias for the frame
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	20-sep-89 (nasser)
**		Initial definition.
*/
static i4
get_alias(frm, state)
FRAME	*frm;
i4	 state;
{
	i4	alias = -1;
	i4	i;

	/*
	**  To implement 1-based aliases, we'll leave the 0-entry
	**  in the array unused.
	*/
	for (i = 1; i < num_frm_aliases; i++)
		if (frm_aliases[i].frm_ptr == frm)
		{
			if ((frm_aliases[i].state & state) == state)
				alias = i;
			break;
		}
	return(alias);
}

/*{
** Name:  new_alias -- Assign alias to a frame.
**
** Description:
**	Assign a new alias to a frame.  Aliases are index into
**	the aliases array, and we find a new alias by searching
**	through the array for an unused entry.  If no such
**	entry is found, we enlarge the array.
**	Note that the aliases are 1-based, so the 0-th entry
**	in the aliases array is left unused.
**
** Inputs:
**	frm	Pointer to frame
**
** Outputs:
** 	Returns:
**		Alias for the frame
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	20-sep-89 (nasser)
**		Initial definition.
*/
static i4
new_alias(frm)
FRAME	*frm;
{
	i4	  	 alias = -1;
	i4		 i;
	struct frm_rec	*tmp_aliases;

	for (i = 1; i < num_frm_aliases; i++)
		if (frm_aliases[i].state == FREE)
		{
			alias = i;
			break;
		}
	if (alias == -1)
	{
		tmp_aliases = (struct frm_rec *) MEreqmem(0,
			(num_frm_aliases+INCR_SZ) * sizeof(struct frm_rec),
			TRUE, (STATUS *) NULL);
		MEcopy((PTR) frm_aliases,
			(u_i2) num_frm_aliases * sizeof(struct frm_rec),
			(PTR) tmp_aliases);
		_VOID_ MEfree((PTR) frm_aliases);
		frm_aliases = tmp_aliases;
		if (num_frm_aliases == 0)
			alias = 1;
		else
			alias = num_frm_aliases;
		num_frm_aliases += INCR_SZ;
	}
	frm_aliases[alias].frm_ptr = frm;
	return(alias);
}

/*{
** Name:  entry_fits -- Find out if the current cell is the next entry.
**
** Description:
**	Find out if the cell defined by the parameters row and col is
**	the next entry in the table.  This is used to determine if
**	this cell can be grouped with other cells from the table in
**	sending messages to MWS.
**	In deciding if the the given cell is the next entry, this
**	function assumes that each row is completed before moving
**	on to the next row.
**
** Inputs:
**	col	Column number
**	row	row number
**
** Outputs:
** 	Returns:
**		TRUE/FALSE
** 	Exceptions:
**		None.
**
** Side Effects:
**	The values that keep track of the cells so far
**	recorded are updated.
**
** History:
**	08-nov-89 (nasser)
**		Initial definition.
**	11-Apr-97 (kitch01)
**		Bug 79872. Now accept total columns as parm. We check
**		to ensure the incoming cell does actually follow the
**		preceeding cell reading left-right top-bottom. EG. for
**		a 2 column tf, cell 0,1 MUST follow cell 1,0.
*/
static i4
entry_fits(col, row, tot_cols)
i4	col;
i4	row;
i4	tot_cols;
{
	i4	fits = FALSE;

	tot_cols--;

	if (row == end_row)
	{
		if (col == end_col + 1)
		{
			end_col++;
			fits = TRUE;
		}
		else if (row == start_row)
		{
			if ((col >= start_col) && (col <= end_col))
			{
				fits = TRUE;
			}
		}
		else
		{
			if (col <= end_col)
			{
				fits = TRUE;
			}
		}
	}
	/* The end_col stored must also be the last col in the TF for
	** this cell to logically follow it in sequence.
	*/
	else if ((row == end_row + 1) && (col == 0) && (end_col == tot_cols))
	{
		end_row++;
		end_col = 0;
		fits = TRUE;
	}

	return(fits);
}

/*{
** Name:  put_tbl -- Create the specified table item.
**
** Description:
**	Tell MWS about a new table item.  This involves creating
**	the table, all its columns and all the table cells.
**
** Inputs:
**	formAlias	alias of window containing field
**	itemAlias	alias of field to be displayed
**	theTable 	INGRES table data structure
**
** Outputs:
** 	Returns:
**		OK/FAIL.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	25-sep-89 (bspence)
**		Initial definition.
*/
static STATUS
put_tbl(formAlias, itemAlias, theTable)
i4	 formAlias;
i4	 itemAlias;
TBLFLD	*theTable;
{
	i4		 row;
	i4		 col;
	i4		 tflags;
	FLDVAL		*val;
	DB_TEXT_STRING	*text;
	i4		 tmp;

	/* Send any table cells that are queued up */
	if (send_tbl_cells(FALSE) != OK)
		return(FAIL);

	if (IIMWatiAddTableItem(formAlias, itemAlias, theTable) == FAIL)
		return(FAIL);
	for (col = 0; col < theTable->tfcols; col++)
	{
		if (IIMWaciAddColumnItem(formAlias, itemAlias,
			col, theTable->tfflds[col]) == FAIL)
		{
			return(FAIL);
		}
	}
	for (row = 0; row < theTable->tfrows; row++)
		for (col = 0; col < theTable->tfcols; col++)
		{
			val = theTable->tfwins + (row*theTable->tfcols) + col;
			text = (DB_TEXT_STRING *) val->fvdsdbv->db_data;
			tflags = theTable->tffflags[(row * theTable->tfcols) +
				col];
			tmp = tflags;
			IIFTdcaDecodeCellAttr(tmp, &tflags);
			if (text->db_t_count != 0)
			{
				/*
				**  First send all queued up cells,
				**  then send this cell.
				*/
				if (send_tbl_cells(TRUE) != OK)
					return(FAIL);
				if (IIMWatcAddTableCell(formAlias, itemAlias,
					col, row, tflags, val) == FAIL)
				{
					return(FAIL);
				}
			}
			else if (rec_frm_alias == -1)
			{
				start_recording(formAlias, itemAlias, col, row);
				rec_flags = tflags;
			}
			else if (tflags != rec_flags)
			{
				if (send_tbl_cells(TRUE) != OK)
					return(FAIL);
				start_recording(formAlias, itemAlias, col,row);
				rec_flags = tflags;
			}
			else
			{
				/*
				**  We don't need to call entry_fits since
				**  we know that this IS next entry, but
				**  entry_fits will set variables for us
				**  so we call it anyway.
				*/
				_VOID_ entry_fits(col, row, theTable->tfcols);
			}
		}

	if (send_tbl_cells(TRUE) != OK)
		return(FAIL);

	return(OK);
}

/*{
** Name:  send_tbl_cells -- Send the group of table cells.
**
** Description:
**	Send the group of table cells accumulated so far.
**
** Inputs:
**	inc_flags	TRUE if flags to be included
**
** Outputs:
** 	Returns:
**		STATUS
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	08-nov-89 (nasser)
**		Initial definition.
*/
static STATUS
send_tbl_cells(inc_flags)
i4	inc_flags;
{
	STATUS	stat;

	if (rec_frm_alias == -1)
		return(OK);

	if (inc_flags)
		stat = IIMWatgAddTblGroup(rec_frm_alias, rec_item_alias,
			start_col, start_row, 
			end_col, end_row, rec_flags);
	else
		stat = IIMWctgClearTblGroup(rec_frm_alias, rec_item_alias,
			start_col, start_row, end_col, end_row);
	if (stat == FAIL)
		return(FAIL);

	rec_frm_alias = -1;
	return(OK);
}

/*{
** Name:  start_recording -- Start recording info about a group of tbl cells.
**
** Description:
**	Set static variables to record info about a group of table cells.
**
** Inputs:
**	frm_alias	Alias of frame
**	item_alias	Alias of item
**	col		Column number
**	row		Row number
**
** Outputs:
** 	Returns:
**		None.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	08-nov-89 (nasser)
**		Initial definition.
*/
static VOID
start_recording(frm_alias, item_alias, col, row)
i4	frm_alias;
i4	item_alias;
i4	col;
i4	row;
{
	rec_frm_alias = frm_alias;
	rec_item_alias = item_alias;
	start_row = end_row = row;
	start_col = end_col = col;
}

/*{
** Name:  show_hide -- Tell MWS about the visibility of a field.
**
** Description:
**	The visibility of the fields in a frame may have changed.
**	Tell MWS to show/hide each field as indicated by the
**	flags.
**
** Inputs:
**	frm_alias	Alias of the form.
**	item_alias	Alias of the field.
**	fld		Info about the field.
**
** Outputs:
** 	Returns:
**		OK/FAIL.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	13-sep-90 (nasser) -- Extracted from IIMWshf().
**	16-sep-90 (nasser) -- Added code to send column visibility info.
*/
static STATUS
show_hide(frm_alias, item_alias, fld)
i4	 frm_alias;
i4	 item_alias;
FIELD	*fld;
{
    TBLFLD	*tbl;
    FLDHDR	*hdr;
    i4		 invis_mask = fdINVISIBLE;
    i4		 colno;

    if (fld->fltag == FTABLE)
    {
	tbl = fld->fld_var.fltblfld;
	hdr = &tbl->tfhdr;
	invis_mask |= fdTFINVIS;
    }
    else
    {
	tbl = NULL;
	hdr = &fld->fld_var.flregfld->flhdr;
    }
    if (hdr->fhdflags & invis_mask)
    {
	if (IIMWhiHideItem(frm_alias, item_alias, -1) != OK)
	    return(FAIL);
    }
    else
    {
	if (IIMWsiShowItem(frm_alias, item_alias, -1) != OK)
	    return(FAIL);
	if (tbl != NULL)
	{
	    for (colno = 0; colno < tbl->tfcols; colno++)
	    {
		if (tbl->tfflds[colno]->flhdr.fhdflags & fdINVISIBLE)
		{
		    if (IIMWhiHideItem(frm_alias, item_alias, colno) != OK)
		        return(FAIL);
		}
		else
		{
		    if (IIMWsiShowItem(frm_alias, item_alias, colno) != OK)
		        return(FAIL);
		}
		    
	    }
	}
    }

    return(OK);
}

/*{
** Name:  IIMWrsbRefreshScrollButton - Refresh Scroll Button position  
**
** Description:
**	Sends information necessary to refresh the postion of a scroll
**	bar button subsequent to a scrolling operation.
**	This is supported by MWS version 3.2 and above.
**
** Inputs:
**	frm	Pointer to form containing table field of interest.
**	fldno	number of the table field in form.
**	cur     Ordinal 1-based index of current row at top of display.
**	tot     Total number of rows in dataset.
**
** Outputs:
** 	Returns:
**	STATUS
**
** Side Effects:
**	Messages to front end windowing environment.
**
** History:
**      18-apr-97 (cohmi01)
**          Added for UPFRONT (bug 81574).
*/
STATUS
IIMWrsbRefreshScrollButton(
FRAME	*frm,
i4	 fldno,
i4	cur,
i4	tot)
{
	i4	alias, item_alias;

	/* Do not send new X200 msg if MWS client is back level */
	if ( (! IIMWmws) || (IIMWVersionInfo.version < MWSVER_32))
		return(OK);

	alias = get_alias(frm, DISPLAYED);
	if (alias == -1)
	{
		IIMWpelPrintErrLog(mwERR_ALIAS, "RefreshScrollB");	
		return(FAIL);
	}
	item_alias = fldno + FLDINX;


	return(IIMWssbSendScrollButton(alias, item_alias, cur, tot));
}


/*{
** Name:  IIMWtstfTryScrollTblFld - Try Scroll Table Field via MWS X131 
**
** Description:
**	If the MWS client is at level 3.2 or above, send a X131 msg    
**	to scroll existing screen data up or down, else give back    
**	error so caller may scroll via FDcopy.
**
** Inputs:
**	frm	Pointer to form containing table field of interest.
**	fldno	number of the table field in form.
**	nrows   number of rows to scroll. Positive=up, Neg=down
**
** Outputs:
** 	Returns:
**	STATUS
**
** Side Effects:
**	Messages to front end windowing environment.
**
** History:
**      05-may-97 (cohmi01)
**          Added for UPFRONT scrolling performance (bug 73587).
*/
STATUS
IIMWtstfTryScrollTblFld(
FRAME	*frm,
i4	 fldno,
i4	nrows)
{
	i4	alias, item_alias;

	/* Do not send new X200 msg if MWS client is back level */
	if ( (! IIMWmws) || (IIMWVersionInfo.version < MWSVER_32))
		return(FAIL);  /* caller must do operation the old way */

	alias = get_alias(frm, DISPLAYED);
	if (alias == -1)
	{
		IIMWpelPrintErrLog(mwERR_ALIAS, "ScrollTblFld");	
		return(FAIL);
	}
	item_alias = fldno + FLDINX;


	return(IIMWstfScrollTblFld(alias, item_alias, nrows));
}

/*{
** Name:  IIMWisrInitScrollbarReqd - Initialise Scrollbar Required  
**
** Description:
**	Determines whether the form requires initial X200 scrollbar 
**	messages to be sent. Ensures that all tablefields displayed
**	will have correctly positioned scrollbar buttons when first
**	displayed.
**
** Inputs:
**	frm	Pointer to form containing table field(s) of interest.
**
** Outputs:
** 	Returns:
**	TRUE - Initial X200 need to be sent
**	FALSE - Initial X200 has already been sent for this form
**
** Side Effects:
**	None
**
** History:
**      22-dec-97 (kitch01)
**          Added for UPFRONT (bugs 87709, 87808, 87812, 87841).
*/
i4
IIMWisrInitScrollbarReqd(frm)
FRAME	*frm;
{
	i4 alias;

	/* if MWS client is back level we can't send X200 anyway */
	if ( (! IIMWmws) || (IIMWVersionInfo.version < MWSVER_32))
		return(FALSE);

	alias = get_alias(frm, DISPLAYED);
	if (alias == -1)
	{
		IIMWpelPrintErrLog(mwERR_ALIAS, "InitScrollB");	
		return(FALSE);
	}

	if (frm_aliases[alias].x200_sent)
		return (FALSE);
	else
		return (TRUE);

}

/*{
** Name:  IIMWsiScrollbarInitialized - Scrollbar Initialized  
**
** Description:
**	The scrollbar has been initialized. All required X200 messages
**	have been sent.
**
** Inputs:
**	frm	Pointer to form containing table field(s) of interest.
**
** Outputs:
** 	Returns:
**	STATUS
**
** Side Effects:
**	None
**
** History:
**      22-dec-97 (kitch01)
**          Added for UPFRONT (bugs 87709, 87808, 87812, 87841).
*/
STATUS
IIMWsiScrollbarInitialized(frm)
FRAME	*frm;
{
	i4 alias;

	alias = get_alias(frm, DISPLAYED);
	if (alias == -1)
	{
		IIMWpelPrintErrLog(mwERR_ALIAS, "InitScrollB");	
		return(FAIL);
	}

	frm_aliases[alias].x200_sent = TRUE;
	return (OK);
	
}

# endif /* DATAVIEW */
