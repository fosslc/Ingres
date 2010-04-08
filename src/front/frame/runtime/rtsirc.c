/*
**	rtsirc.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	rtsirc.c - Routines for set/inquire on row x column.
**
** Description:
**	This file contains routines to support the set/inquire_frs
**	on a row_column and row statements.  Also support for
**	causing the correct display attribute to appear for the
**	current form is located in this file.
**
** History:
**	Created - 08/30/85 (dkh)
**	05/02/87 (dkh) - Integrated change bit code.
**	19-jun-87 (bab)	Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Changed <eqforms.h> to "setinq.h"
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	09/01/87 (dkh) - Removed change bit support for row_column.
**	09/16/87 (dkh) - Integrated table field change bit.
**	07/24/90 (dkh) - Added support for table field cell highlighting.
**	19-feb-92 (leighb) DeskTop Porting Change:
**		adh_evcvtdb() has only 3 args, bogus 4th one deleted.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	25-apr-1996 (chech02)
**	    	added FAR to IIfrscb for windows 3.1 port.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

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
# include	"setinq.h"
# include	<rtvars.h>
# include       <er.h>

GLOBALREF	RUNFRM	*IIsirfrm;
GLOBALREF	FRAME	*IIsifrm;
GLOBALREF	i4	IIsirow;
GLOBALREF	TBSTRUCT	*IIsitbs;
GLOBALREF	TBLFLD		*IIsitbl;

#ifdef WIN16
GLOBALREF	FRS_CB	*FAR IIfrscb;
#else
GLOBALREF	FRS_CB	*IIfrscb;
#endif
GLOBALREF	FRAME	*IIsifrm;
GLOBALREF	FIELD	*IIsifld;


/*{
** Name:	RTrcdecode - Decode display attribute data utility routine.
**
** Description:
**	This is a utility routine used by RTsetrow() and RTsetrc() to
**	support setting/resetting display attributes for a ROW or
**	ROW_COLUMN.  These are RTI internal use only features.  Restrictions
**	of the current implementation are that the clearing out a display
**	attribute clears all display attributes and setting display attributes
**	is not additive.  That is, setting a display attribute for a row and
**	row_column will wipe any previous settings.  A final caution
**	is that these settings are very temporary (no place to store
**	the settings) and will disappear when switching between displays
**	of forms.  Display attributes that are recognized:
**	 - normal (clear out) display attribute.
**	 - reverse video.
**	 - blinking.
**	 - underlining.
**	 - display intensity change.
**
**
** Inputs:
**	frsflg	Display attribute operation to decode.
**	data	A 0/1 indicator for setting/resetting the display attribute.
**
** Outputs:
**	flag	Pointer to integer where decoded information is placed.
**
**	Returns:
**		TRUE	If decoding was successful.
**		FALSE	If an unknown operation or bad "data" was passed in.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08/30/85 (dkh) - Initial version.
**	02/13/87 (dkh) - Added procedure header.
*/
i4
RTrcdecode(i4 frsflg, i4 *flag, i4 data)
{
	i4	onoff;

	if (data == 0 || data == 1)
	{
		onoff = data;
	}
	else
	{
		IIFDerror(SIDANOFF, 0);
		return(FALSE);
	}
	switch(frsflg)
	{
		case frsNORMAL:
			if (onoff)
			{
				*flag = 0;
			}
			else
			{
				return(FALSE);
			}
			break;

		case frsRVVID:
			if (onoff)
			{
				*flag = fdRVVID;
			}
			break;
		
		case frsBLINK:
			if (onoff)
			{
				*flag = fdBLINK;
			}
			break;

		case frsUNLN:
			if (onoff)
			{
				*flag = fdUNLN;
			}
			break;

		case frsINTENS:
			if (onoff)
			{
				*flag = fdCHGINT;
			}
			break;

		default:
			return(FALSE);
	}
	return(TRUE);
}



/*{
** Name:	RTfldda - Update display with changed display attributes.
**
** Description:
**	This routine is called to update display information in the
**	FT layer when display attributes for fields in the CURRENTLY
**	displayed form have been changed.
**
** Inputs:
**	frm	Pointer to a FRAME structure.
**	fld	Pointer to a FIELD structure.
**	rfld	Pointer to a REGFLD structure.
**
** Outputs:
**	Returns:
**		Always returns TRUE.
**	Exceptions:
**		None.
**
** Side Effects:
**	Causes data structures in the FT layer to be changed and
**	the change will eventually be reflected in what the user
**	sees.
**
** History:
**	08/30/85 (dkh) - Initial version.
**	02/13/87 (dkh) - Added procedure header.
*/
i4
RTfldda(frm, fld, rfld)
FRAME	*frm;
FIELD	*fld;
REGFLD	*rfld;
{
	i4	mode;
	i4	fldno;
	FLDHDR	*hdr;
	REGFLD	*dummy;

	hdr = &(rfld->flhdr);
	if (hdr->fhseq >= 0)
	{
		mode = FT_UPDATE;
	}
	else
	{
		mode = FT_DISPONLY;
	}
	fldno = FDgtfldno(mode, frm, fld, &dummy);
	FTsetda(frm, fldno, mode, FT_REGFLD, 0, 0, hdr->fhdflags);
	return(TRUE);
}



/*{
** Name:	RTcolda - Update display with changed column attributes.
**
** Description:
**	This routine is the column version of RTfldda().  Display
**	information for a column in a table field is updated with
**	the changed display attribute.  A call to this routine is
**	only necessary if the table field is in the form that is
**	CURRENTLY displayed.
**
** Inputs:
**	frm	Pointer to a FRAME structure.
**	tbl	Pointer to a TBLFLD structure.
**	col	Pointer to a FLDCOL structure.
**
** Outputs:
**	Returns:
**		Always returns TRUE.
**	Exceptions:
**		None.
**
** Side Effects:
**	Causes data structures in the FT layer to be changed and
**	the change will eventually be reflected in what the user
**	sees.
**
** History:
**	08/30/85 (dkh) - Initial version.
**	02/13/87 (dkh) - Added procedure header.
*/
i4
RTcolda(frm, tbl, col)
FRAME	*frm;
TBLFLD	*tbl;
FLDCOL	*col;
{
	i4	fldno;
	i4	maxrow;
	i4	i;
	i4	colnum;
	i4	mode = FT_UPDATE;
	FLDHDR	*hdr;
	FLDHDR	*chdr;

	hdr = &(tbl->tfhdr);
	chdr = &(col->flhdr);
	fldno = hdr->fhseq;
	colnum = chdr->fhseq;
	maxrow = tbl->tfrows;

	for (i = 0; i < maxrow; i++)
	{
		FTsetda(frm, fldno, mode, FT_TBLFLD, colnum, i, chdr->fhdflags);
	}
	return(TRUE);
}


/*{
** Name:	RTinqrow - Inquiring on a row of a table field.
**
** Description:
**	This routine is the entry point for supporting the
**	inquire_frs on a row (of a table field) statement.
**	It is here only for completeness since any information
**	that can be inquired on (as a result of a set_frs on
**	a row statement) is not saved.  As a result, a value
**	of zero is always returned in case a user stumbles
**	across this undocumented statement.  Options that can
**	be inquired upon are:
**	 - Color for a row_column. (integer)
**	 - Is reverse video set. (integer)
**	 - Is blinking set. (integer)
**	 - Is underlining set. (integer)
**	 - Is a different display intensity set. (integer)
**
** Inputs:
**	tbl	Pointer to a TBLFLD structure.
**	col	Pointer to a FLDCOL structure.
**	frsflg	Operation to perform.
**
** Outputs:
**	data	Data area where result is to be placed.
**
**	Returns:
**		FALSE only if row is out of range
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08/30/85 (dkh) - Initial version.
**	02/13/87 (dkh) - Added procedure header.
*/
i4
RTinqrow(TBLFLD *tbl, FLDCOL *col, i4 frsflg, i4 *data)
{
	i4	srow;
	i4	scol;
	i4	junk;
	i4	flag;

	switch (frsflg)
	{
	case frsFRMCHG:
		/*
		**  Call tbacc routine to handle things.
		*/
		IITBicb(IIsirfrm, IIsitbs, col->flhdr.fhdname, IIsirow, data);
		break;
	case frsSROW:
		IIFRfsz(IIsifld,&srow,&scol,&junk,&junk);
		*data = srow + IIsifrm->frposy +
				(IIfrscb->frs_event)->scryoffset + 1;
		break;
	case frsSCOL:
		IIFRfsz(IIsifld,&srow,&scol,&junk,&junk);
		*data = scol + IIsifrm->frposx +
				(IIfrscb->frs_event)->scrxoffset + 1;
		break;

	case frsNORMAL:
	case frsRVVID:
	case frsBLINK:
	case frsUNLN:
	case frsINTENS:
	case frsCOLOR:
		if (IITBicaInqCellAttr(IIsirfrm, IIsitbs, col->flhdr.fhdname,
			IIsirow, &flag) != OK)
		{
			break;
		}
		switch(frsflg)
		{
			case frsRVVID:
				if (flag & fdRVVID)
				{
					*data = 1;
				}
				else
				{
					*data = 0;
				}
				break;
			case frsBLINK:
				if (flag & fdBLINK)
				{
					*data = 1;
				}
				else
				{
					*data = 0;
				}
				break;
			case frsUNLN:
				if (flag & fdUNLN)
				{
					*data = 1;
				}
				else
				{
					*data = 0;
				}
				break;
			case frsINTENS:
				if (flag & fdCHGINT)
				{
					*data = 1;
				}
				else
				{
					*data = 0;
				}
				break;
			case frsCOLOR:
				*data = RTfindcolor(flag);
				break;
		}
		break;

	default:
		if (IIsirow < 1 || IIsirow > IIsitbl->tfrows)
		{
			IIFDerror(SIROW, 0);
			return(FALSE);
		}
		*data = 0;
	}

	return(TRUE);
}



/*{
** Name:	RTsetrow - Set display attributes for a row.
**
** Description:
**	This is the entry point for supporting the set_frs
**	on a row statement.  Only display attributes may be
**	set currently, but other options may be added in the
**	future.  Several caveats are associated with this
**	feature.  One, the table field must be in the CURRENTLY
**	displayed form.  Two, the displayed attribute that is
**	set is not saved (there is no space in the data structures).
**	Three, setting a display attribute wipes out any
**	previously set attribute from the view point of what
**	the user sees on the screen.  Thus, setting a display
**	attribute is not additive, as is the case for fields
**	and columns.  Finally, resetting (clearing) a display
**	attribute really clears out all display attribute
**	for the row.  Supported display attributes are:
**	 - Color. (integer)
**	 - Reverse video. (integer)
**	 - Blinking. (integer)
**	 - Underlining. (integer)
**	 - A different display intensity. (integer)
**
** Inputs:
**	tbl	Pointer to a TBLFLD structure.
**	row	Row number to use in the set command.
**	frsflg	Display attribute to set.
**	data	Indicator for setting/resetting the display attribute.
**
** Outputs:
**	Returns:
**		TRUE	If the command was performed successfully.
**		FALSE	If an invalid command or bad indicator value
**			was passed in.
**	Exceptions:
**		None.
**
** Side Effects:
**	Display information in the FT layer is also updated for the
**	affected row.
**
** History:
**	08/30/85 (dkh) - Initial version.
**	02/13/87 (dkh) - Added procedure header.
**	04/15/97 (cohmi01)
** 	    Remove unused variables of types that dont belong in this subsys.
*/
i4
RTsetrow(TBLFLD *tbl, FLDCOL *col, i4 frsflg, i4 *data)
{
	FLDHDR	*hdr;
	i4	flags = 0;
	i4	maxcol;
	i4	fldno;
	i4	i;
	i4	row;
	i4	value;
	i4	colour;
	i4	current = FALSE;

	value = *data;
	hdr = &(tbl->tfhdr);
	maxcol = tbl->tfcols;
	fldno = hdr->fhseq;
	row = IIsirow;

	switch (frsflg)
	{
		case frsFRMCHG:
			/*
			**  Check to only allow a value of 0 or 1.
			*/
			if (value != 0 && value != 1)
			{
				return(FALSE);
			}

			/*
			**  Call tbacc routine to do the dirty work.
			*/
			IITBscb(IIsirfrm, IIsitbs, col->flhdr.fhdname,
				IIsirow, value);
			return(TRUE);
			break;

		case frsCOLOR:
			if (value < fdMINCOLOR || value > fdMAXCOLOR)
			{
				colour = value;
				IIFDerror(SIBCOLOR, 1, &colour);
				return(FALSE);
			}
			if ((flags = RTselcolor(value)) == 0)
			{
				flags = fdCOLOR;
				/* value is already zero */
			}
			break;

		case frsNORMAL:
		case frsRVVID:
		case frsUNLN:
		case frsBLINK:
		case frsINTENS:
			switch (frsflg)
			{
				case frsNORMAL:
					if (value != 1)
					{
						IIFDerror(SIDANOFF, 0);
						return(FALSE);
					}
					value = 0;
					flags = fdATTR;
					break;

				case frsRVVID:
					flags = fdRVVID;
					break;

				case frsUNLN:
					flags = fdUNLN;
					break;

				case frsBLINK:
					flags = fdBLINK;
					break;

				case frsINTENS:
					flags = fdCHGINT;
					break;
			}
			break;

		default:
			return(FALSE);
			break;
	}

	/*
	**  Row checking will be done by IITBscaSetCellAttr() below.
	if (row < 1 || row > IIsitbl->tfrows)
	{
		IIFDerror(SIROW, 0);
		return(FALSE);
	}
	*/

	if (IIsirfrm == IIstkfrm)
	{
		current = TRUE;
	}

	/*
	**  Call IITBscaSetCellAttr() to do unload loop checking, etc.
	*/
	IITBscaSetCellAttr(IIsirfrm, IIsitbs, col->flhdr.fhdname, IIsirow,
		flags, value, current);

# ifdef OLD
	/*
	**  Do zero based indexing.
	*/

	row--;

	for (i = 0; i < maxcol; i++)
	{
		FTsetda(IIsifrm, fldno, FT_UPDATE, FT_TBLFLD, i, row, flags);
	}
# endif
	return(TRUE);
}


/*{
** Name:	RTinqrc - Inquiring on a ROW_COLUMN object.
**
** Description:
**	This is the entry point for supporting the inquire_frs
**	on a ROW_COLUMN (i.e., a cell) of a table field.
**	It exists merely for completeness.  The result is
**	always zero for now since the type of information that
**	can be inquired upon is not saved (due to no space).
**	Options that can be inquired upon are:
**	 - Color of a row_column. (integer)
**	 - Is reverse video set. (integer)
**	 - Is blinking set. (integer)
**	 - Is underlining set. (integer)
**	 - Is a different display intensity set. (integer)
**
** Inputs:
**	tbl	Pointer to a TBLFLD structure.
**	col	Pointer to a FLDCOL structure.
**	frsflg	Display attribute option to inquire on.
**
** Outputs:
**	data	Data space where result is to be placed.  Always
**		set result to zero currently.
**
**	Returns:
**		Always returns TRUE.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08/30/85 (dkh) - Initial version.
**	02/14/87 (dkh) - Added procedure header.
*/
i4
RTinqrc(TBLFLD *tbl, FLDCOL *col, i4 frsflg, i4 *data)
{
	*data = 0;

	return(TRUE);
}


/*{
** Name:	RTsetrc - Set display attributes for a ROW_COLUMN.
**
** Description:
**	RTsetrc() is the entry point for supporting setting display
**	attributes on a ROW_COLUMN (i.e., cell) of a table field.
**	Only display attributes are supported for now though new
**	options may be added in the future.  Several caveats are
**	associated with this internal to RTI feature.  One, the
**	table field must be in the CURRENTLY displayed form.  Two,
**	the displayed attribute that is set is not saved (there is
**	no space in the data structures).  Three, setting a display
**	attributes wipes out any previously set attribute from the
**	viewpoint of what the user sees on the screen.  Thus, setting
**	a display attribute is not additive, as is the case for
**	fields and columns.  Finally, resetting (clearing) a display
**	attribute really clears out all display attributes for the
**	ROW_COLUMN.  Supported displayed attributes are:
**	 - Color. (integer)
**	 - Reverse Video. (integer)
**	 - Blinking. (integer)
**	 - Underlining. (integer)
**	 - A different display intensity. (integer)
**
** Inputs:
**	tbl	Pointer to a TBLFLD structure.
**	col	Pointer to a FLDCOL structure.
**	frsflg	Display attribute option to set.
**	data	Indicator for setting/resetting the display attribute.
**
** Outputs:
**	Returns:
**		TRUE	If display attribute set successfully.
**		FALSE	If a bad display attribute option or
**			indicator value was passed in.
**	Exceptions:
**		None.
**
** Side Effects:
**	Display information in the FT layer is also updated for the
**	affected row_column.
**
** History:
**	08/30/85 (dkh) - Initial version.
**	02/14/87 (dkh) - Added procedure header.
*/
i4
RTsetrc(TBLFLD *tbl, FLDCOL *col, i4 frsflg, i4 *data)
{
	i4	row = IIsirow;
	i4	colnum;
	i4	flags = 0;
	FLDHDR	*hdr;
	FLDHDR	*chdr;
	i4	value;
	i4	colour;

	value = *data;
	hdr = &(tbl->tfhdr);
	chdr = &(col->flhdr);
	colnum = chdr->fhseq;

	if (frsflg == frsCOLOR)
	{
		if (value < fdMINCOLOR || value > fdMAXCOLOR)
		{
			colour = value;
			IIFDerror(SIBCOLOR, 1, &colour);
			return(FALSE);
		}
		flags = RTselcolor(value);
	}
	else if (!RTrcdecode(frsflg, &flags, value))
	{
		return(FALSE);
	}

	/*
	**  Do zero based indexing.
	*/

	row--;

	FTsetda(IIsifrm, hdr->fhseq, FT_UPDATE, FT_TBLFLD, colnum, row, flags);

	return(TRUE);
}



/*{
** Name:	IIsetattrio - Set attributes at insertrow/loadtable time.
**
** Description:
**	External interface call to support setting an attribute
**	at insertrow/loadtable time.  Does some consistency check
**	before calling IItsetattr() to put the attribute into the
**	dataset (and the display windows, if necessary).
**
** Inputs:
**	fsitype		Attribute to set in set/inquire encoding.
**	colname		Name of column to set attribute on.
**	nullind		Null indicator variable, always NULL here.
**	isvar		Is user using a variable (TRUE) or constant (FALSE).
**	datatype	Datatype of value for setting the attribute.
**	datalen		Data lenght of value for setting the attribute.
**	value		The value for setting the attribute.
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
**	07/24/90 (dkh) - Initial version.
*/
VOID
IIFRsaSetAttrio(i4 fsitype, char *cname, i2 *nullind,
	i4 isvar, i4 datatype, i4 datalen, PTR value)
{
	DB_EMBEDDED_DATA	edv;
	DB_DATA_VALUE		dbv;
	i4			natval;
	i4			colour;
	i4			attrflag = 0;
	i4			attrtype;
	char			*colname;
	char			buf[MAXFRSNAME + 1];

	edv.ed_type = datatype;
	edv.ed_length = datalen;
	edv.ed_data = isvar ? value : (PTR) &value;
	edv.ed_null = nullind;

	dbv.db_datatype = DB_INT_TYPE;
	dbv.db_length = sizeof(i4);
	dbv.db_data = (PTR) &natval;

	if (adh_evcvtdb(FEadfcb(), &edv, &dbv) != OK)	 
	{
		/*
		**  Data conversion error.
		*/
		IIFDerror(RTSIERR, 0);
		return;
	}

	colname = IIstrconv(II_CONV, cname, buf, MAXFRSNAME);

	attrtype = IIgetrtcode(fsitype);

	if (attrtype == frsCOLOR)
	{
		if (natval < fdMINCOLOR || natval > fdMAXCOLOR)
		{
			colour = natval;
			IIFDerror(SIBCOLOR, 1, &colour);
			return;
		}
		if ((attrflag = RTselcolor(natval)) == 0)
		{
			attrflag = fdCOLOR;
			/* natval is already zero */
		}
	}
	else
	{
		/*
		**  Exit if user didn't specify zero or one for
		**  the non-color display attributes.
		*/
		if (natval != 0 && natval != 1)
		{
			IIFDerror(SIDANOFF, 0);
			return;
		}
		if (attrtype == frsNORMAL)
		{
			/*
			**  Return if user specified normal = 0.
			**  It doesn't make sense it this case.
			*/
			if (natval == 0)
			{
				return;
			}
			attrflag = fdATTR;
			natval = 0;
		}
		else if (attrtype == frsRVVID)
		{
			attrflag = fdRVVID;
		}
		else if (attrtype == frsBLINK)
		{
			attrflag = fdBLINK;
		}
		else if (attrtype == frsUNLN)
		{
			attrflag = fdUNLN;
		}
		else if (attrtype == frsINTENS)
		{
			attrflag = fdCHGINT;
		}
		else
		{
			return;
		}
	}

	/*
	**  If the row is visible, IItsetattr() will flush the
	**  attributes to the screen for us.
	*/

	IItsetattr(colname, attrflag, natval);
}
