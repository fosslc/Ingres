/*
** Copyright (c) 2004 Ingres Corporation
** All rights reserved.
*/

/*
** Name:	fdutil.c - Utility routines used by frame and runtime/tbacc.
**
** Description:
**	This file contains utility routines that are used by the frame
**	directory as well as the runtime/tbacc directories.
**
** History:
**	xx/xx/83 (jen) - Initial version.
**	30-apr-1984	Improved interface to routines for performance (ncg)
**	02/04/85 (jen) - Changes for 3.0.
**	12/23/86 (dkh) - Added support for new activations.
**	02/08/87 (dkh) - Added support for ADTs.
**	05/02/87 (dkh) - Integrated support for external change bit.
**	05/06/87 (dkh) - Fixed VMS compile warnings.
**	19-jun-87 (bruceb)	Code cleanup.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added change bit for datasets.
**	09/16/87 (dkh) - Integrated table field change bit.
**	11/14/87 (dkh) - Corrected order of args to FDputoper.
**	12/11/87 (dkh) - Fixed to correctly set user change bit for field.
**	02/05/88 (dkh) - Fixed jup bug 1965.
**	02/27/88 (dkh) - Added support for nextitem command.
**	05/27/88 (dkh) - Added new routine IIFDsfsSetFieldScrolling().
**	08/12/88 (dkh) - Fixed DG reported bug 79.
**	11-nov-88 (bruceb)
**		Added CMD_TMOUT as valid 'last command' value.
**	17-feb-89 (bruceb)
**		Ignore readonly columns equally as displayonly columns.
**	14-mar-89 (bruceb)
**		Last command inquiry now uses evcb->lastcmd instead of event.
**	07-jul-89 (bruceb)
**		IIFDgccb and IIFDtscb changed to get/set fdI_CHG, fdX_CHG
**		and fdVALCHKED instead of just fdX_CHG.
**	12/27/89 (dkh) - Added support for GOTOs.
**	23-apr-90 (bruceb)	Fix for bug 21349.
**		FDavchk now returns 0 on readonly or invisible flds/cols.
**	07/24/90 (dkh) - Added support for table field cell highlighting.
**	09/12/90 (dkh) - Fixed bug with not correctly removing old dataset
**			 color value when a new one is set.
**	10/21/90 (dkh) - Fixed bug 33996.
**	12/12/92 (dkh) - Fixed trigraph warnings from acc.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	04/19/97 (cohmi01)
**	    Add FDrsbRefreshScrollButton() as higher level wrapper for
**	    new MWS functionality. (bug 81574)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	27-dec-2001 (somsa01)
**	    Added LEVEL1_OPTIM for i64_win to prevent iiinterp.exe from
**	    SEGV'ing.
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
# include	<frscnsts.h>
# include	<frsctblk.h>
# include	<frserrno.h>
# include       <er.h>
# include	"fdfuncs.h"

# if defined(i64_win)
# pragma optimize("s", on)
# endif


FUNC_EXTERN	FIELD	*FDfndfld();
FUNC_EXTERN	FLDCOL	*FDfndcol();
FUNC_EXTERN	i4	FDgtfldno();
FUNC_EXTERN	FRAME	*FDFTgetiofrm();
FUNC_EXTERN	FIELD	*FDfldofmd();

i4	IIFDscb();
i4	IIFDccb();

static	bool	intr_chg = FALSE;

/*{
** Name:	FDtblcount - Number of rows in a table field.
**
** Description:
**	Given a TBLFLD pointer, returns the number of physical
**	rows in the table field.  Is this routine still needed.
**
** Inputs:
**	tbl	Pointer to a TBLFLD strucutre.
**
** Outputs:
**	Returns:
**		rows	Number of rows in the table field.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	xx/xx/xx (xxx) - Initial version.
**	02/14/87 (dkh) - Added procedure header.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
*/
i4
FDtblcount(tbl)
TBLFLD	*tbl;
{
	return(tbl->tfrows);
}



/*{
** Name:	FDlastrow - Update last row number displayed.
**
** Description:
**	Given a pointer to a TBLFLD structure, update the current
**	row so that it is no bigger than "lastrow".  This ensures
**	that the current row is on a row that is valid.
**
** Inputs:
**	tbl	Pointer to a TBLFLD structure.
**
** Outputs:
**	Returns:
**		Always returns TRUE.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	xx/xx/xx (xxx) - Initial version.
**	02/14/87 (dkh) - Added procedure header.
*/
i4
FDlastrow(tbl)
TBLFLD	*tbl;
{
	if (tbl->tflastrow > 0)
	{
		tbl->tflastrow--;
		if (tbl->tfcurrow > tbl->tflastrow)
			tbl->tfcurrow = tbl->tflastrow;
	}
	return TRUE;
}

/*{
** Name:	FDrownum - Return the current row number.
**
** Description:
**	Given a pointer to a TBLFLD structure, return the current
**	row number for the table field.  Is this routine still used?
**
** Inputs:
**	tbl	Pointer to a TBLFLD structure.
**
** Outputs:
**
**	Returns:
**		currow	The current row number for the table field.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	xx/xx/xx (xxx) - Initial version.
**	02/14/87 (dkh) - Added procedure header.
*/
i4
FDrownum(tbl)
TBLFLD	*tbl;
{
	return(tbl->tfcurrow);
}


/*{
** Name:	FDactive - Is table field the current field.
**
** Description:
**	Given a pointer to a form (FRAME structure) and a table
**	field (TBLFLD) structure, check if the table field is
**	the current (active) field in the form.  Return TRUE
**	if so, FALSE otherwise.
**
** Inputs:
**	frm	Pointer to a FRAME structure.
**	tbl	Pointer to a TBLFLD structure.
**
** Outputs:
**	Returns:
**		TRUE	If table field is the current field in the form.
**		FALSE	If the table field is NOT the current field.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	xx/xx/xx (xxx) - Initial version.
**	02/14/87 (dkh) - Added procedure header.
*/
i4
FDactive(frm, tbl)
FRAME	*frm;
TBLFLD	*tbl;
{
	return (tbl->tfhdr.fhseq == frm->frcurfld);
}


/*{
** Name:	FDlastcmd - Last command that caused an activation.
**
** Description:
**	Determine the last command issued by user that caused return
**	from FT.  The three cases are: 1) menu key, 2) frs key or 3)
**	leaving a field in some way, causing an activation.  The information
**	is stored in an event control block (by FT) and passed to this
**	routine for decoding.  If an unknown command was stored in the
**	control block, this routine will just return the undefined
**	command.  The possible commands that can be returned are:
**	 - CMD_UNDEF - An undefined (unknown) command was found.
**	 - CMD_MENU - The menu key was pressed.
**	 - CMD_FRSk - A frs key was selected.
**	 - CMD_MUITEM - A menu item was selected.
**	 - CMD_NEXT - The nextfield/auto-duplicate command was used.
**	 - CMD_PREV - The previousfield command was used.
**	 - CMD_DOWN - The downline command was used.
**	 - CMD_UP - the upline command was used.
**	 - CMD_NROW - the new/nextrow command was used.
**	 - CMD_RET - the clearrest command was used.
**	 - CMD_SCRUP - the scrollup command was used.
**	 - CMD_SCRDN - the scrolldown command was used.
**	 - CMD_NXITEM - the nextitem command was used.
**	 - CMD_TMOUT - the program timed out on input.
**
** Inputs:
**	evcb	A pointer to a EVCB (event control block).
**
** Outputs:
**	Returns:
**		cmd	Numeric encoding of the command that caused
**			activation.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	11/29/86 (dkh) - Initial version.
**	02/14/87 (dkh) - Added procedure header.
*/
i4
FDlastcmd(evcb)
FRS_EVCB	*evcb;
{
	i4	cmd = CMD_UNDEF;

	switch (evcb->lastcmd)
	{
		case fdopMENU:
			cmd = CMD_MENU;
			break;

		case fdopFRSK:
			cmd = CMD_FRSK;
			break;

		case fdopMUITEM:
			cmd = CMD_MUITEM;
			break;

		case fdopNEXT:
		case fdopADUP:
			cmd = CMD_NEXT;
			break;

		case fdopPREV:
			cmd = CMD_PREV;
			break;

		case fdopDOWN:
			cmd = CMD_DOWN;
			break;

		case fdopUP:
			cmd = CMD_UP;
			break;

		case fdopNROW:
			cmd = CMD_NROW;
			break;

		case fdopRET:
			cmd = CMD_RET;
			break;

		case fdopSCRUP:
			cmd = CMD_SCRUP;
			break;

		case fdopSCRDN:
			cmd = CMD_SCRDN;
			break;

		case fdopNXITEM:
			cmd = CMD_NXITEM;
			break;

		case fdopTMOUT:
			cmd = CMD_TMOUT;
			break;

		case fdopGOTO:
			cmd = CMD_GOTO;
			break;

		default:
			break;
	}
	return(cmd);
}


/*
**  Do activation and validation checks.
*/

/*{
** Name:	FDavchk - Check if validation/activation is necessary.
**
** Description:
**	This is the central routine that determines whether any
**	validation/activation should be done.  If the validation
**	is enabled and fails, then no activation is done.  The
**	frs control block (FRS_CB) that is passed in contains
**	all the pertinent information to do the check.  The
**	activation/validation control block (FRS_AVCB) contains
**	the global settings on the various activation/validation
**	options.  User action (command selected, etc.) is contained
**	in the event control block (FRS_EVCB).  If user action was
**	a menu item or frs key selection, then the event
**	control block also contains the activation and validate
**	options for the selected menu item or frs key.  The interrupt
**	value for the selected menu item or frs key is also stored
**	in the event control block.  The activate and validate
**	options for the selected menu item or frs key has precedence
**	over the global settings.  The global settings are consulted
**	only if no local settings were specified for the selected
**	menu item or frs key.
**
**	This is a NO-OP if the current field is a table field and
**	1) it is currently in READ mode or 2) if the current column is
**	READONLY or 3) if the table field is not in QUERY or APPEND
**	mode and the column is QUERYONLY.  In this case return zero (0).
**
**	Don't do any validation now if the table field is in
**	QUERY mode.
**
**	If validation needs to be done, only run data type checking
**	if the form is in query mode (argument "isqry").  Otherwise
**	run, the full validation check.  Return -1 if validation
**	fails.
**
**	Return the interrupt value if activation needs to be done.
**	Otherwise, return 0.
**
** Inputs:
**	frm	Pointer to a FRAME structure (a form).
**	frscb	Pointer to a FRS_CB structure (a frs control block).
**	isqry	TRUE if forms is in query mode, FALSE otherwise.
**
** Outputs:
**	Returns:
**		<0	If validation failed.
**		0	If validation passed or there was no need
**			to do it and activation turned off.
**		>0	Same as case 0 except that activation is turned
**			on and we need to return the interrupt value which
**			is always greater than 0.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	12/23/86 (dkh) - Initial version.
**	02/14/87 (dkh) - Added procedure header.
*/
i4
FDavchk(frm, frscb, isqry)
FRAME	*frm;
FRS_CB	*frscb;
i4	isqry;
{
	FIELD	*fld;
	TBLFLD	*tf;
	FLDCOL	*col;
	FLDHDR	*hdr;
	FRS_EVCB *evcb;
	FRS_AVCB *avcb;
	i4	intrval;
	i4	needval = TRUE;
	i4	doval = FALSE;
	i4	doact = FALSE;
	i4	glob_val;
	i4	glob_act;
	i4	fldno;
	i4	row = 0;
	i4	colnum = 0;
	i4	fldtype;

	/*
	**  Find current field.  If a table field and it is
	**  in read mode or if column is read only, then don't
	**  activate/validate.
	*/
	fldno = frm->frcurfld;
	fld = frm->frfld[fldno];
	if (fld->fltag == FTABLE)
	{
		tf = fld->fld_var.fltblfld;
		fldtype = FT_TBLFLD;
		row = tf->tfcurrow;
		colnum = tf->tfcurcol;
		col = tf->tfflds[colnum];
		if ((tf->tfhdr.fhdflags & fdtfREADONLY) ||
			(tf->tfhdr.fhdflags & fdTFINVIS) ||
			(col->flhdr.fhdflags & fdtfCOLREAD) ||
			(col->flhdr.fhd2flags & fdREADONLY) ||
			(col->flhdr.fhdflags & fdINVISIBLE) ||
			(!(tf->tfhdr.fhdflags & fdtfQUERY) &&
			!(tf->tfhdr.fhdflags & fdtfAPPEND) &&
			(col->flhdr.fhdflags & fdQUERYONLY)))
		{
			return(0);
		}
		if (tf->tfhdr.fhdflags & fdtfQUERY)
		{
			needval = FALSE;
		}
		intrval = col->flhdr.fhintrp;
	}
	else
	{
		fldtype = FT_REGFLD;
		hdr = FDgethdr(fld);
		if ((hdr->fhd2flags & fdREADONLY) ||
			(hdr->fhdflags & fdINVISIBLE))
		{
			return(0);
		}
		intrval = hdr->fhintrp;
	}


	/*
	**  Resolve local and global settings for
	**  activate/validate individually.
	**  If validation fails, then return -1.
	**  If validation passed and no interrupt
	**  then return 0.  If validation passed
	**  and interrupt, then return interrupt value.
	*/

	avcb = frscb->frs_actval;
	evcb = frscb->frs_event;
	if (evcb->event == fdopMUITEM)
	{
		glob_val = (i4) avcb->val_mi;
		glob_act = (i4) avcb->act_mi;
	}
	else if (evcb->event == fdopFRSK)
	{
		glob_val = (i4) avcb->val_frsk;
		glob_act = (i4) avcb->act_frsk;
	}
	else
	{
		glob_val = (i4) avcb->val_mu;
		glob_act = (i4) avcb->act_mu;
	}

	if (evcb->event != fdopMENU)
	{
		if ((evcb->val_event == FRS_YES) ||
			(evcb->val_event == FRS_UF && glob_val == TRUE))
		{
			doval = TRUE;
		}
		if ((evcb->act_event == FRS_YES) ||
			(evcb->act_event == FRS_UF && glob_act == TRUE))
		{
			doact = TRUE;
		}
	}
	else
	{
		doval = glob_val;
		doact = glob_act;
	}
	if (needval && doval)
	{
		if (isqry)
		{
			if (FDqrydata(frm, fldno, FT_UPDATE, fldtype, colnum,
				row) == FALSE)
			{
				return(-1);
			}
		}
		else
		{
			if (FDgetdata(frm, fldno, FT_UPDATE, fldtype, colnum,
				row) == FALSE)
			{
				return(-1);
			}
		}
	}
	if (doact)
	{
		return(intrval);
	}
	return(0);
}


/*{
** Name:	FDdbvget - Get a DB_DATA_VALUE pointer for a field.
**
** Description:
**	This routine is called by the RUNTIME/TBACC layers of the
**	forms system to obtain the DB_DATA_VALUE pointer for the field.
**	The RUNTIME/TBACC can either use the value contained in
**	the DB_DATA_VALUE (performing a getform) or place a new value
**	directly into it (performing a putform).  The latter case is
**	allowed since type compatibility and other safety checks are
**	performed before a conversion and update of the DB_DATA_VALUE
**	takes place.
**
** Inputs:
**	frm	Pointer to a FRAME strucutre.
**	fname	Name of field that we want to obtain a DB_DATA_VALUE for.
**	cname	Name of a column if field is a table field.
**	rownum	A row number (zero based) if field is a table field.
**
** Outputs:
**	ptr	Pointer to DB_DATA_VALUE for field is assigned to what
**		this param points to.
**	Returns:
**		OK	If field and it's DB_DATA_VALUE exist.
**		FAIL	If field does not exist or there is no DB_DATA_VALUE.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/08/87 (dkh) - Initial version.
*/
STATUS
FDdbvget(frm, fname, cname, rownum, ptr)
FRAME		*frm;
char		*fname;
char		*cname;
i4		rownum;
DB_DATA_VALUE	**ptr;
{
	FIELD	*fld;
	TBLFLD	*tf;
	FLDVAL	*val;
	FLDCOL	*col;
	bool	dsonly;

	if ((fld = FDfndfld(frm, fname, &dsonly)) == NULL)
	{
		IIFDerror(PFFLNF, 1, fname);
		return(FAIL);
	}

	if (fld->fltag == FTABLE)
	{
		tf = fld->fld_var.fltblfld;
		if (rownum < 0 || rownum >= tf->tfrows)
		{
			return(FAIL);
		}
		if ((col = FDfndcol(tf, cname)) == NULL)
		{
			IIFDerror(TBBADCOL, 2, tf->tfhdr.fhdname, cname);
			return(FAIL);
		}
		val = tf->tfwins + (rownum * tf->tfcols) + col->flhdr.fhseq;
	}
	else
	{
		val = FDgetval(fld);
	}

	*ptr = val->fvdbv;
	return(OK);
}



/*{
** Name:	FDfmtget - Get the the format (FMT) pointer for a field.
**
** Description:
**	This routine is called by the range query code of the
**	forms system to obtain the FMT pointer for the specific field.
**	The returned FMT pointer will be used for formatting purposes
**	by the caller and is READ ONLY to the caller.
**
** Inputs:
**	frm	Pointer to a FRAME strucutre.
**	fname	Name of field that we want to obtain a FMT pointer for.
**	cname	Name of a column if field is a table field.
**	rownum	A row number (zero based) if field is a table field.
**
** Outputs:
**	ptr	The FMT pointer for the field is assigned to what
**		this param points to.
**	Returns:
**		OK	If field and it's FMT structure exist.
**		FAIL	If field does not exist or there is no FMT structure.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	03/24/87 (dkh) -Initial version written.
*/
STATUS
FDfmtget(frm, fname, cname, rownum, ptr)
FRAME	*frm;
char	*fname;
char	*cname;
i4	rownum;
FMT	**ptr;
{
	FIELD	*fld;
	TBLFLD	*tf;
	FLDTYPE	*type;
	FLDCOL	*col;
	bool	dsonly;

	if ((fld = FDfndfld(frm, fname, &dsonly)) == NULL)
	{
		IIFDerror(PFFLNF, 1, fname);
		return(FAIL);
	}

	if (fld->fltag == FTABLE)
	{
		tf = fld->fld_var.fltblfld;
		if (rownum < 0 || rownum >= tf->tfrows)
		{
			return(FAIL);
		}
		if ((col = FDfndcol(tf, cname)) == NULL)
		{
			IIFDerror(TBBADCOL, 2, tf->tfhdr.fhdname, cname);
			return(FAIL);
		}
		type = &(col->fltype);
	}
	else
	{
		type = FDgettype(fld);
	}

	*ptr = type->ftfmt;
	return(OK);
}




/*{
** Name:	FDsetoper - Set query operator for a field.
**
** Description:
**	Routine to set the query operator for a field.  This
**	replaces the trick of using a different data type to
**	represent a query operator being placed into a field.
**
** Inputs:
**	frm	Pointer to a FRAME structure.
**	fname	Name of field that we want to set query operator on.
**	cname	Name of column for setting query operator on.
**	rownum	Row number (zero based) if field is a table field.
**	oper	The query operator.
**
** Outputs:
**	None.
**	Returns:
**		OK	If the operator was set.
**		FAIL	If operator could not be set (e.g., no such field).
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/08/87 (dkh) - Initial version.
*/
STATUS
FDsetoper(frm, fname, cname, rownum, oper)
FRAME	*frm;
char	*fname;
char	*cname;
i4	rownum;
i4	oper;
{
	FIELD	*fld;
	TBLFLD	*tf;
	REGFLD	*fd;
	FLDCOL	*col;
	i4	fldchain = FT_UPDATE;
	i4	fldtype = FT_REGFLD;
	i4	row = 0;
	i4	numcol = 0;
	i4	fldno = 0;
	bool	dsonly;

	if ((fld = FDfndfld(frm, fname, &dsonly)) == NULL)
	{
		IIFDerror(PFFLNF, 1, fname);
		return(FAIL);
	}

	if (dsonly)
	{
		fldchain = FT_DISPONLY;
	}

	if (fld->fltag == FTABLE)
	{
		fldtype = FT_TBLFLD;
		tf = fld->fld_var.fltblfld;
		if (rownum < 0 || rownum >= tf->tfrows)
		{
			return(FAIL);
		}
		row = rownum;
		if ((col = FDfndcol(tf, cname)) == NULL)
		{
			IIFDerror(TBBADCOL, 2, tf->tfhdr.fhdname, cname);
			return(FAIL);
		}
		numcol = col->flhdr.fhseq;
		fldno = tf->tfhdr.fhseq;
	}
	else
	{
		fldno = FDgtfldno(fldchain, frm, fld, &fd);
	}

	FDputoper(oper, frm, fldno, fldchain, fldtype, numcol, row);

	return(OK);
}




/*{
** Name:	FDdispupd - Update the display for a field.
**
** Description:
**	This routine will update the display buffer for a field from
**	the field's actual value.  The value for a field may have
**	been chosen by RUNTIME/TBACC.  RUNTIME/TBACC MUST call
**	this routine to synchronize the field's value and display
**	if it changes the value for a field.
**
** Inputs:
**	frm	A pointer to a FRAME structure
**	fname	Name of field to update.
**	cname	Name of column if field is a table field.
**	rownum	Row number if field is a table field.
**
** Outputs:
**	None.
**	Returns:
**		OK	If display update was successful.
**		FAIL	If update was not possible (e.g., no such field).
**	Exceptions:
**		None.
**
** Side Effects:
**	The display buffer for the specified field will be changed
**	along with updates sent to the FT level for appropriate
**	screen changes, etc.
**
** History:
**	02/08/87 (dkh) - Initial version.
*/
i4
FDdispupd(frm, fname, cname, rownum)
FRAME	*frm;
char	*fname;
char	*cname;
i4	rownum;
{
	FIELD	*fld;
	REGFLD	*rfld;
	TBLFLD	*tf;
	FLDVAL	*val;
	FLDCOL	*col;
	FLDHDR	*hdr;
	FLDTYPE	*type;
	REGFLD	*dummy;
	i4	colnum;
	i4	fldtype;
	i4	disponly;
	i4	fldno;
	bool	dsonly;

	if ((fld = FDfndfld(frm, fname, &dsonly)) == NULL)
	{
		IIFDerror(PFFLNF, 1, fname);
		return(FALSE);
	}

	if (dsonly)
	{
		disponly = FT_DISPONLY;
	}
	else
	{
		disponly = FT_UPDATE;
	}

	fldno = FDgtfldno(disponly, frm, fld, &dummy);

	if (fld->fltag == FTABLE)
	{
		tf = fld->fld_var.fltblfld;
		if (rownum < 0 || rownum >= tf->tfrows)
		{
			return(FALSE);
		}
		if ((col = FDfndcol(tf, cname)) == NULL)
		{
			IIFDerror(TBBADCOL, 2, tf->tfhdr.fhdname, cname);
			return(FALSE);
		}
		if (rownum > tf->tflastrow)
		{
			tf->tflastrow = rownum;
		}
		hdr = &(col->flhdr);
		colnum = hdr->fhseq;
		type = &(col->fltype);
		val = tf->tfwins + (rownum * tf->tfcols) + colnum;
		fldtype = FT_TBLFLD;
	}
	else
	{
		rfld = fld->fld_var.flregfld;
		hdr = &(rfld->flhdr);
		type = &(rfld->fltype);
		val = &(rfld->flval);
		fldtype = FT_REGFLD;
	}

	/*
	**  Format field value into the display buffer.
	**  FDdatafmt() will also do update to FT.
	*/
	return(FDdatafmt(frm, fldno, disponly, fldtype, colnum, rownum, hdr,
		type, val));
}



/*{
** Name:	FDschgbit - Set per field change bit.
**
** Description:
**	This routine is called by RUNTIME to set the per field
**	change bit for each field (even readonly ones) in the form.
**	Can't call normal traverse routines since we need to pass
**	the field pointers along due to the way the cell flag
**	space is stored differently than for regular fields.
**
** Inputs:
**	frm	Pointer to form.
**	set	Set or reset change bits.
**		0 - to reset.
**		1 - to set.
**	intr	Set the internal or external change bit.
**		TRUE - to (re)set the internal change bit.
**		FALSE - to (re)set the external change bit.
**
** Outputs:
**	None.
**	Returns:
**		None
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/02/87 (dkh) - Initial version.
*/
VOID
FDschgbit(frm, set, intr)
FRAME	*frm;
i4	set;
i4	intr;
{
	i4	i;
	FRAME	*ofrm;
	i4	(*routine)();

	if (set)
	{
		routine = IIFDscb;
	}
	else
	{
		routine = IIFDccb;
	}

	ofrm = FDFTgetiofrm();
	FDFTsetiofrm(frm);
	intr_chg = intr;

	FDFTsmode(FT_UPDATE);
	for (i = 0; i < frm->frfldno; i++)
	{
		FDfldtrv(i, routine, FDALLROW);
	}

	FDFTsmode(FT_DISPONLY);
	for (i = 0; i < frm->frnsno; i++)
	{
		FDfldtrv(i, routine, FDALLROW);
	}

	FDFTsetiofrm(ofrm);
}



/*{
** Name:	IIFDmiMarkInvalid - Mark a field invalid.
**
** Description:
**	This routine simply marks the passed field/cell invalid by clearing out
**	bit fdVALCHKED.
**
** Inputs:
**	frm		Pointer to form containing field.
**	fldno		Field number of field.
**	disponly	Is field a displayo only or updateable field.
**	fldtype		Is field a simple or table field.
**	col		Column number of cell if a table field.
**	row		Row number of cell if a table field.
**
** Outputs:
**
**	Returns:
**		TRUE	Always returns TRUE.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	10/21/90 (dkh) - Initial version.
*/
i4
IIFDmiMarkInvalid(frm, fldno, disponly, fldtype, col, row)
FRAME	*frm;
i4	fldno;
i4	disponly;
i4	fldtype;
i4	col;
i4	row;
{
	i4	*pflags;
	FIELD	*fld;
	TBLFLD	*tbl;
	FLDHDR	*hdr;
	i4	bitval;

	fld = FDfldofmd(frm, fldno, disponly);

	if (fldtype == FT_TBLFLD)
	{
		tbl = fld->fld_var.fltblfld;
		pflags = tbl->tffflags + (row * tbl->tfcols) + col;
	}
	else
	{
		hdr = FDgethdr(fld);
		pflags = &(hdr->fhdflags);
	}

	*pflags &= ~fdVALCHKED;

	return(TRUE);
}



/*{
** Name:	IIFDcvbClearValidBit - Clear out the valid bit for a field.
**
** Description:
**	This routine simply calls IIFDmiMarkInvalid() to clear out the
**	valid bit (fdVALCHKED) for each field in the form.
**
** Inputs:
**	frm		Pointer to form to clear out valid bit.
**
** Outputs:
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
**	10/21/90 (dkh) - Initial version.
*/
VOID
IIFDcvbClearValidBit(frm)
FRAME	*frm;
{
	i4	i;
	FRAME	*ofrm;

	ofrm = FDFTgetiofrm();
	FDFTsetiofrm(frm);

	FDFTsmode(FT_UPDATE);
	for (i = 0; i < frm->frfldno; i++)
	{
		FDfldtrv(i, IIFDmiMarkInvalid, FDALLROW);
	}

	FDFTsmode(FT_DISPONLY);
	for (i = 0; i < frm->frnsno; i++)
	{
		FDfldtrv(i, IIFDmiMarkInvalid, FDALLROW);
	}

	FDFTsetiofrm(ofrm);
}


/*{
** Name:	IIFDgccb - Get change bit for a column.
**
** Description:
**	Sets the change bit value for a particular column in a row.
**	A value of 1 (one) is returned if change bit set.  Zero (0)
**	if change bit is NOT set.
**
** Inputs:
**	tbl	Pointer to a table field structure.
**	row	Row number.
**	colname	Column name.
**
** Outputs:
**	val	Values of change bits are placed here.
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	09/01/87 (dkh) - Initial version.
*/
VOID
IIFDgccb(tbl, row, colname, val)
TBLFLD	*tbl;
i4	row;
char	*colname;
i4	*val;
{
	FLDCOL	*col;
	i4	*pflag;
	i4	rnum;

	switch(row)
	{
	    case fdtfTOP:
		rnum = 0;
		break;

	    case fdtfBOT:
		rnum = tbl->tfrows - 1;
		break;

	    case fdtfCUR:
		rnum = tbl->tfcurrow;
		break;

	    default:
		rnum = row;
		break;
	}
	if ((col = FDfndcol(tbl, colname)) == NULL)
	{
	    *val = 0;
	}
	else
	{
	    pflag = tbl->tffflags + (rnum * tbl->tfcols) + col->flhdr.fhseq;
	    *val = *pflag & (fdI_CHG | fdX_CHG | fdVALCHKED);
	}
}


i4
IIFDgcb(fld)
FIELD	*fld;
{
	FLDHDR	*hdr;

	hdr = FDgethdr(fld);
	if (hdr->fhdflags & fdX_CHG)
	{
		return(1);
	}
	return(0);
}



IIFDtccb(tbfld, row, colname)
TBLFLD	*tbfld;
i4	row;
char	*colname;
{
	i4	*pflags;
	FLDCOL	*col;

	if ((col = FDfndcol(tbfld, colname)) == NULL)
	{
		return;
	}
	pflags = tbfld->tffflags + (row * tbfld->tfcols) + col->flhdr.fhseq;
	*pflags &= ~(fdX_CHG | fdVALCHKED | fdI_CHG);
}


IIFDtscb(tbfld, row, colname, val, on)
TBLFLD	*tbfld;
i4	row;
char	*colname;
i4	val;
i4	on;
{
	i4	*pflags;
	FLDCOL	*col;

	if ((col = FDfndcol(tbfld, colname)) == NULL)
	{
		return;
	}
	pflags = tbfld->tffflags + (row * tbfld->tfcols) + col->flhdr.fhseq;

	if (on)
	{
		*pflags |= val;
	}
	else
	{
		*pflags &= ~(val);
	}
}



VOID
IIFDfccb(frm, fldnm)
FRAME	*frm;
char	*fldnm;
{
	FIELD	*fld;
	FLDHDR	*hdr;
	bool	dummy;

	if ((fld = FDfndfld(frm, fldnm, &dummy)) == NULL)
	{
		return;
	}
	hdr = FDgethdr(fld);
	hdr->fhdflags &= ~(fdX_CHG | fdVALCHKED | fdI_CHG);
}


VOID
IIFDfscb(fld, val)
FIELD	*fld;
i4	val;
{
	FLDHDR	*hdr;

	hdr = FDgethdr(fld);
	if (val == 0)
	{
		hdr->fhdflags &= ~fdX_CHG;
	}
	else
	{
		hdr->fhdflags |= fdX_CHG;
	}
}



i4
IIFDccb(frm, fldno, disponly, fldtype, col, row)
FRAME	*frm;
i4	fldno;
i4	disponly;
i4	fldtype;
i4	col;
i4	row;
{
	i4	*pflags;
	FIELD	*fld;
	TBLFLD	*tbl;
	FLDHDR	*hdr;
	i4	bitval;

	if (intr_chg)
	{
		bitval = fdI_CHG;
	}
	else
	{
		bitval = fdX_CHG;
	}

	fld = FDfldofmd(frm, fldno, disponly);

	if (fldtype == FT_TBLFLD)
	{
		tbl = fld->fld_var.fltblfld;
		pflags = tbl->tffflags + (row * tbl->tfcols) + col;
	}
	else
	{
		hdr = FDgethdr(fld);
		pflags = &(hdr->fhdflags);
	}

	*pflags &= ~bitval;

	return (TRUE);
}


i4
IIFDscb(frm, fldno, disponly, fldtype, col, row)
FRAME	*frm;
i4	fldno;
i4	disponly;
i4	fldtype;
i4	col;
i4	row;
{
	i4	*pflags;
	FIELD	*fld;
	TBLFLD	*tbl;
	FLDHDR	*hdr;
	i4	bitval;

	if (intr_chg)
	{
		bitval = fdI_CHG;
	}
	else
	{
		bitval = fdX_CHG;
	}

	fld = FDfldofmd(frm, fldno, disponly);

	if (fldtype == FT_TBLFLD)
	{
		tbl = fld->fld_var.fltblfld;
		pflags = tbl->tffflags + (row * tbl->tfcols) + col;
	}
	else
	{
		hdr = FDgethdr(fld);
		pflags = &(hdr->fhdflags);
	}

	*pflags |= bitval;

	if (intr_chg)
	{
		/*
		**  Mark all fields as unvalidated since setting the internal
		**  flags is only called at the start of a display loop
		**  or changing the mode of a form.  We play it safe for now.
		*/
		*pflags &= ~fdVALCHKED;
	}

	return(TRUE);
}



/*{
** Name:	IIFDsfsSetFieldScrolling - (Un)set field scrolling bits.
**
** Description:
**	This routine will traverse the fields in the passed in form and
**	set/unset scrolling bits so that qbf range query will work.
**	The scrolling information is actually stored in a different
**	bit when unset and restored from the other bit when being set.
**
** Inputs:
**	frm	Form to set/unset scrolling bits.
**	set	Sets scrolling bit if 1, unset if 0.
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
**	05/27/88 (dkh) - Initial version.
*/
VOID
IIFDsfsSetFieldScrolling(frm, set)
FRAME	*frm;
i4	set;
{
	i4	fldno;
	i4	colno;
	FIELD	**fld;
	TBLFLD	*tfld;
	FLDCOL	**cols;
	FLDHDR	*hdr;

	/*
	**  Return if there is nothing to do.
	*/
	if (set)
	{
	    if (!(frm->frmflags & fd2SCRFLD))
	    {
		return;
	    }
	    frm->frmflags |= fdSCRLFD;
	    frm->frmflags &= ~fd2SCRFLD;
	}
	else
	{
	    if (!(frm->frmflags & fdSCRLFD))
	    {
		return;
	    }
	    frm->frmflags |= fd2SCRFLD;
	    frm->frmflags &= ~fdSCRLFD;
	}

	for (fldno = 0, fld = frm->frfld; fldno < frm->frfldno; fldno++, fld++)
	{
	    if ((*fld)->fltag == FTABLE)
	    {
		tfld = (*fld)->fld_var.fltblfld;
		cols = tfld->tfflds;
		for (colno = 0; colno < tfld->tfcols; colno++, cols++)
		{
		    hdr = &((*cols)->flhdr);
		    if ((hdr->fhdflags & fdtfCOLREAD)
			|| (hdr->fhd2flags & fdREADONLY))
		    {
			continue;
		    }
		    if (set)
		    {
			if (hdr->fhd2flags & fd2SCRFLD)
			{
			    hdr->fhd2flags |= fdSCRLFD;
			    hdr->fhd2flags &= ~fd2SCRFLD;
			}
		    }
		    else
		    {
			if (hdr->fhd2flags & fdSCRLFD)
			{
			    hdr->fhd2flags |= fd2SCRFLD;
			    hdr->fhd2flags &= ~fdSCRLFD;
			}
		    }
		}
	    }
	    else
	    {
		hdr = &((*fld)->fld_var.flregfld->flhdr);
		if (set)
		{
		    if (hdr->fhd2flags & fd2SCRFLD)
		    {
			hdr->fhd2flags |= fdSCRLFD;
			hdr->fhd2flags &= ~fd2SCRFLD;
		    }
		}
		else
		{
		    if (hdr->fhd2flags & fdSCRLFD)
		    {
			hdr->fhd2flags |= fd2SCRFLD;
			hdr->fhd2flags &= ~fdSCRLFD;
		    }
		}
	    }
	}
}


/*{
** Name:	IIFDscaSetCellAttr - Set attributes in table field cell.
**
** Description:
**	This routines sets the attributes for a specific cell in a
**	table field.  Values are stored in tffflags.  Once set, FTsetda()
**	is called to update screen structures.
**
** Inputs:
**	frm		Parent form for table field.
**	tbl		Parent table field for cell.
**	colname		Name of column cell is in.
**	row		Row number of cell.
**	flag		Attribute info to set.
**	current		Flag indicating if we are dealing with the current form.
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
IIFDscaSetCellAttr(frm, tbl, colname, row, flag, onoff, for_ds, current)
FRAME	*frm;
TBLFLD	*tbl;
char	*colname;
i4	row;
i4	flag;
i4	onoff;
i4	for_ds;
i4	current;
{
	i4	*pflags;
	FLDCOL	*col;
	i4	tfattr = 0;


	if ((col = FDfndcol(tbl, colname)) == NULL)
	{
		IIFDerror(TBBADCOL, 2, tbl->tfhdr.fhdname, colname);
		return;
	}

	pflags = tbl->tffflags + (row * tbl->tfcols) + col->flhdr.fhseq;

	if (for_ds)
	{
		if (flag & fdRVVID)
		{
			tfattr |= dsRVVID;
		}
		if (flag & fdUNLN)
		{
			tfattr |= dsUNLN;
		}
		if (flag & fdBLINK)
		{
			tfattr |= dsBLINK;
		}
		if (flag & fdCHGINT)
		{
			tfattr |= dsCHGINT;
		}
		if (flag & fd1COLOR)
		{
			tfattr |= ds1COLOR;
		}
		if (flag & fd2COLOR)
		{
			tfattr |= ds2COLOR;
		}
		if (flag & fd3COLOR)
		{
			tfattr |= ds3COLOR;
		}
		if (flag & fd4COLOR)
		{
			tfattr |= ds4COLOR;
		}
		if (flag & fd5COLOR)
		{
			tfattr |= ds5COLOR;
		}
		if (flag & fd6COLOR)
		{
			tfattr |= ds6COLOR;
		}
		if (flag & fd7COLOR)
		{
			tfattr |= ds7COLOR;
		}
	}
	else
	{
		if (flag & fdRVVID)
		{
			tfattr |= tfRVVID;
		}
		if (flag & fdUNLN)
		{
			tfattr |= tfUNLN;
		}
		if (flag & fdBLINK)
		{
			tfattr |= tfBLINK;
		}
		if (flag & fdCHGINT)
		{
			tfattr |= tfCHGINT;
		}
		if (flag & fd1COLOR)
		{
			tfattr |= tf1COLOR;
		}
		if (flag & fd2COLOR)
		{
			tfattr |= tf2COLOR;
		}
		if (flag & fd3COLOR)
		{
			tfattr |= tf3COLOR;
		}
		if (flag & fd4COLOR)
		{
			tfattr |= tf4COLOR;
		}
		if (flag & fd5COLOR)
		{
			tfattr |= tf5COLOR;
		}
		if (flag & fd6COLOR)
		{
			tfattr |= tf6COLOR;
		}
		if (flag & fd7COLOR)
		{
			tfattr |= tf7COLOR;
		}
	}

	if (onoff == 0)
	{

		*pflags &= ~tfattr;
	}
	else
	{
		/*
		**  Clear out old color value if we are setting
		**  a new color value.
		*/
		if (flag & fdCOLOR)
		{
			if (for_ds)
			{
				*pflags &= ~dsCOLOR;
			}
			else
			{
				*pflags &= ~tfCOLOR;
			}
		}

		*pflags |= tfattr;
	}

	/*
	**  Now call FTsetda() to set up screen info if we just updated
	**  the current form (the one being displayed).  Note that it
	**  doesn't matter what value we passed as the last argument since
	**  FTsetda() will look at the values set in tffflags to decide
	**  what to display.
	*/
	if (current)
	{
		FTsetda(frm, tbl->tfhdr.fhseq, FT_UPDATE, FT_TBLFLD,
			col->flhdr.fhseq, row, 0 /*flag*/);
	}

	frm->frmflags |= fdREDRAW;
}

/*{
** Name: FDrsbRefreshScrollButton - Refresh Scroll Button position   
**
** Description:
**	Sends information necessary to refresh the postion of a scroll 
**	bar button subsequent to a scrolling operation.
**	This is needed by the MWS environment.
**
** Inputs:
**	tf	ptr to tablefield structure.
**	cur	Ordinal 1-based index of current row at top of display.
**	tot	Total number of rows in dataset.
**
** Outputs:
**	Messages to front end windowing environment.
**
**	Returns:
**		
**	VOID
**
** History:
**	18-apr-97 (cohmi01)   
**	    Added for Refresh of Scroll Button position, currently used 
**	    only by MWS. (bug 81574)
*/

STATUS
FDrsbRefreshScrollButton(
TBLFLD	*tf,
i4	cur,
i4	tot)
{
    FRAME   *frm = NULL;
    i4  fld;

    /* Check if MWS protocol */
    if (IIMWimIsMws())
    {
	FDftbl(tf, &frm, &fld);
	if (frm == NULL)
	{
	    IIFDerror(TBLNOFRM, 0, (char *) NULL);
	    return;	/* not critical */
	}

	return (IIMWrsbRefreshScrollButton(frm, fld, cur, tot));
    }
}
