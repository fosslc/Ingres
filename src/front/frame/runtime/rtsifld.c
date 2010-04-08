/*
**	rtsifld.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	rtsifld.c - Support routines for set/inquire on fields.
**
** Description:
**	This file contains routines that support the set/inquire_frs
**	on field features.
**
** History:
**	xx/xx/xx (jen) - Initial version.
**	09/20/85 (dkh) - Added 4.0 features.
**	02/13/87 (dkh) - Added headers.
**	05/02/87 (dkh) - Integrated change bit code.
**	19-jun-87 (bruceb)	Code cleanup.
**	08/01/87 (dkh) - Integrated frsDATYPE changes from personal path
**			 with those done by jhw.
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Changed <eqforms.h> to "setinq.h"
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	20-dec-88 (bruceb)
**		Added support for frsRDONLY and frsINVIS.  Displayonly supported
**		for simple fields and table field columns; invisible supported
**		for fields, not columns.  When turning on readonly/queryonly,
**		turn off the other attribute.
**	06-feb-89 (bruceb)
**		Added support for frsFLDFMT (the setting thereof).
**	29-jun-89 (bruceb)
**		Handle inquiries/setting of displayonly on derived fields.
**	08/03/89 (dkh) - Updated with interface change to adc_lenchk().
**	08/29/89 (dkh) - Fixed bug 7320.
**	08-sep-89 (bruceb)
**		Handle setting of column (in)visibility.
**	09/21/89 (dkh) - Porting changes integration.
**	09/22/89 (dkh) - More porting changes.
**	10/06/89 (dkh) - Added support for inquire on derivation string
**			 and whether field/column is derived or not.
**	11-oct-89 (bruceb)
**		Added support for set/inquire of table field mode.
**	08/16/91 (dkh) - Passed FRAME pointer to IITBdsmDynSetMode() so
**			 so that clearing a table field will clear out
**			 dataset attributes from the table field cells
**			 holding area as well.
**	07/20/92 (dkh) - Added support for inquiring on existence of
**			 a field.
**	09/20/92 (dkh) - Fixed bug 42994.  When setting the mode of a
**			 table field, we need to make sure that IIfrmio and
**			 FDFTiofrm are set up correctly since i/o will
**			 be occuring.
**	01/20/93 (dkh) - Added support for inputmasking toggle.
**	02/11/93 (dkh) - Fixed code to handle nullable decimal datatypes.
**	07/11/93 (dkh) - Fixed bug 34335 again.  Added code to handle
**			 constant length datatypes that are nullable.
**	07/19/93 (dkh) - Changed call to adc_lenchk() to match
**			 interface change.  The second parameter is
**			 now a i4  instead of a bool.
**      20-nov-1994 (andyw)
**          Added NO_OPTIM due to Solaris 2.4/Compiler 3.00 errors
**      1-june-1995 (hanch04)
**          Removed NO_OPTIM su4_us5, installed sun patch
**      03-Feb-96 (fanra01)
**          Extracted data for DLLs on Windows NT.
**	25-apr-1996 (chech02)
**	    added FAR to IIfrscb for windows 3.1 port.
**      24-sep-96 (hanch04)
**          Global data moved to data.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<afe.h>
# include	<frame.h>
# include	<menu.h>
# include	<runtime.h> 
# include	<frserrno.h>
# include	"setinq.h"
# include	<er.h>
# include	<me.h>
# include	<rtvars.h>

/*
** Base version of a form; the frsRDONLY and frsINVIS operations won't work
** on forms of an earlier version.
*/
# define	BASEVERSION	7

GLOBALREF	RUNFRM	*IIsirfrm;
#ifdef WIN16
GLOBALREF	FRS_CB	*FAR IIfrscb;
#else
GLOBALREF	FRS_CB	*IIfrscb;
#endif
GLOBALREF	FRAME	*IIsifrm;
GLOBALREF	TBLFLD	*IIsitbl;


FUNC_EXTERN	TBSTRUCT	*RTgettbl();
FUNC_EXTERN	DB_STATUS	adh_dbtoev();
FUNC_EXTERN	i4		IIFDgcb();
FUNC_EXTERN	VOID		IIFDtvcTblVisibilityChg();
FUNC_EXTERN	VOID		IITBdsmDynSetMode();

static	i4	RTinternal = 0;

/*{
** Name:	RTstspec - Set special RTI internal flag.
**
** Description:
**	This routine sets/resets a static flag (RTinternal) that enables
**	special features for RTI internal use.  The only
**	feature in this category is setting the mode of a
**	table field dynamically.  HOWEVER, this features is
**	currently incomplete and guaranteed not to work.
**
** Inputs:
**	val	Value to set special flag to.
**
** Outputs:
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	09/20/85 (dkh) - Initial version.
**	02/13/87 (dkh) Added procedure header.
*/
void
RTstspec(val)
i4	val;
{
	RTinternal = val;
}


/*{
** Name:	RTselcolor - Select form color encoding from a color number.
**
** Description:
**	This routine maps a color number (1 to 7) issued via a set_frs
**	statement on the color of a field to the form system's encoding
**	of the color number.  A value outside the range of 1 to 7
**	causes a value of zero to be returned.  This is a utility routine
**	called by RTsetfld().
**
** Inputs:
**	color	The color number (in the range of 0 to 7).
**
** Outputs:
**	Returns:
**		val	The form system's encoding of the color number
**			passed in.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	09/20/85 (dkh) - Initial version.
**	02/13/87 (dkh) - Added procedure header.
*/
i4
RTselcolor(i4 color)
{
	i4	val = 0;

	switch(color)
	{
		case 1:
			val = fd1COLOR;
			break;

		case 2:
			val = fd2COLOR;
			break;

		case 3:
			val = fd3COLOR;
			break;

		case 4:
			val = fd4COLOR;
			break;

		case 5:
			val = fd5COLOR;
			break;

		case 6:
			val = fd6COLOR;
			break;

		case 7:
			val = fd7COLOR;
			break;

		default:
			break;
	}

	return(val);
}


/*{
** Name:	RTfindcolor - Decode field color value for external use.
**
** Description:
**	This routine decodes a form system's color encoding into
**	a color number (1 to 7) that is returned to user programs.
**	Routine RTinqfld() will call this routine in support of
**	the inquire_frs on field color statement.  A value of zero
**	is returned if no color value match is found.
**
** Inputs:
**	flags	The attribute bits for a field (including color).
**
** Outputs:
**	Returns:
**		retval	A color value in the range of 0 to 7.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	09/20/85 (dkh) - Initial version.
**	02/13/87 (dkh) - Added procedure header.
*/
i4
RTfindcolor(i4 flags)
{
	i4	color = 0;
	i4	retval = 0;

	if (color = (flags & fdCOLOR))
	{
		if (color == fd1COLOR)
		{
			retval = 1;
		}
		else if (color == fd2COLOR)
		{
			retval = 2;
		}
		else if (color == fd3COLOR)
		{
			retval = 3;
		}
		else if (color == fd4COLOR)
		{
			retval = 4;
		}
		else if (color == fd5COLOR)
		{
			retval = 5;
		}
		else if (color == fd6COLOR)
		{
			retval = 6;
		}
		else if (color == fd7COLOR)
		{
			retval = 7;
		}
	}

	return(retval);
}


/*{
** Name:	RTinqfld - Entry point for handling inquiring on fields.
**
** Description:
**	This routine is the entry point for supporting inquiring on fields
**	and columns.  Supported features include:
**	 - Name of field/column. (string)
**	 - Sequence number of field/column. (integer)
**	 - Data length of field/column. (integer)
**	 - Data type of field/column. (integer)
**	 - Data format of field/column. (string)
**	 - Validation string of field/column. (string)
**	 - Whether field is a table field. (integer)
**	 - Mode of field/column. (string)
**	 - Whether no (normal) display attributes set for
**	   field/column. (integer)
**	 - Whether reverse video is set for field/column. (integer)
**	 - Whether blinking is set for field/column. (integer)
**	 - Whether underlining is set for field/column. (integer)
**	 - Whether display intensity is set for field/column. (integer)
**	 - What color (if any) is set for field/column. (integer)
**	 - Whether readonly is set for field/column. (integer)
**	 - Whether invisible is set for field. (integer)
**
**	Inquiry for display attributes or readonly of a table field is an error.
**	A mode of "TABLE" is returned if inquiring on the mode
**	of a field that is a table field.
**
** Inputs:
**	fld	Pointer to a FIELD structure.
**	regfld	Pointer to a REGFLD structure.
**	frsflg	Numeric encoding of what user is inquiring for.
**
** Outputs:
**	data	Data area where result of inquiry is to be placed.
**
**	Returns:
**		TRUE	If inquiry was successfully completed.
**		FALSE	If an unknown inquiry or inquiry on an
**			inappropriate object was requested.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	xx/xx/xx (jen) - Initial version.
**	09/20/85 (dkh) - Added 4.0 features.
**	02/13/87 (dkh) - Added procedure header.
**	08/01/87 (jhw) - Added support for "frsDATYPE".
**	08/01/87 (dkh) - Integrated frsDATYPE changes from personal path
**			 with those done by jhw.
*/
i4
RTinqfld(FIELD *fld, REGFLD *regfld, i4 frsflg, i4 *data)
{
	FLDHDR			*hdr;
	FLDTYPE			*type;
	i4			tag;
	i4			usrlen;
	DB_DATA_VALUE		dbv;
	DB_DATA_VALUE		sdbv;
	ADF_CB			*ladfcb;
	DB_EMBEDDED_DATA	edv;
	i4			srow;
	i4			scol;
	i4			nrow;
	i4			ncol;
	TBSTRUCT		*tb;

	if (frsflg != frsEXISTS)
	{
		ladfcb = FEadfcb();
		hdr = &regfld->flhdr;
		type = &regfld->fltype;
		tag = fld->fltag;
	}

	switch (frsflg)
	{
	  case frsFLDNM:
		STcopy(hdr->fhdname, (char *) data);
		break;
			
	  case frsFLDNO:
		*data = ((hdr->fhseq < 0) ? -1 : (hdr->fhseq + 1));
		break;

	  case frsFLDLEN:
		if (tag != FTABLE)
		{
			/*
			**  Need to use local dbv since this can
			**  be called by RTsetcol() and no FLDVAL
			**  structure is set in this case.
			*/
			dbv.db_datatype = type->ftdatatype;
			dbv.db_length = type->ftlength;
			dbv.db_prec = type->ftprec;
			if (adc_lenchk(ladfcb, FALSE, &dbv,
				&sdbv) != OK)
			{
				break;
			}
			usrlen = sdbv.db_length;
			/*
			**  If userlen is zero, must be fixed
			**  length datatype, so return the
			**  fixed length in type->ftlength.
			*/
			if (usrlen)
			{
				*data = usrlen;
			}
			else
			{
				*data = type->ftlength;

				/*
				**  If datatype is nullable,
				**  then subtract one since
				**  we are supposed to hand
				**  back the user length.
				*/
				if (AFE_NULLABLE(type->ftdatatype))
				{
					*data -= 1;
				}
			}
		}
		else
		{
			*data = 0;
		}
		break;

	  case frsFLDTYPE:
		if (tag != FTABLE)
		{
			dbv.db_datatype = type->ftdatatype;
			if (abs(dbv.db_datatype) == DB_DEC_TYPE)
			{
				dbv.db_datatype = DB_FLT_TYPE;
			}
			dbv.db_length = type->ftlength;
			dbv.db_prec = type->ftprec;
			if (adh_dbtoev(ladfcb, &dbv, &edv) != E_DB_OK)
			{
				/*
				**  Will need to give an error message
				**  here if we can't figure out the
				**  old datatype.  For now, just return
				**  a zero to indicate an error.
				*/
				*data = 0;
				break;
			}
			switch (edv.ed_type)
			{
				case DB_INT_TYPE:
					*data = 1;
					break;
				case DB_FLT_TYPE:
					*data = 2;
					break;
				case DB_CHR_TYPE:
					*data = 3;
					break;
			}
		}
		else
		{
			*data = 0;
		}
		break;

	  case frsDATYPE:
		/*
		**  Always return zero if the field
		**  is a table field.  Can't use DB_NODT since
		**  we can't assume it will always be zero.
		**  For a simple field just return the ADF datatype
		**  encoding.  This means that NULLable datatypes
		**  will be returned as negative numbers.
		*/
		*data = (tag != FTABLE) ? type->ftdatatype : 0;
		break;

	  case frsFRMCHG:
		if (tag == FREGULAR)
		{
			*data = IIFDgcb(fld);
		}
		else
		{
			*data = 0;
		}
		break;

	  case frsFLDFMT:
 		if (tag != FTABLE)
 		{
 			if (type->ftfmtstr != NULL)
 				STcopy(type->ftfmtstr, (char *) data);
 			else
				*((char *) data) = EOS;
 		}
		break;

	  case frsFLDVAL:
 		if (tag != FTABLE)
 		{
 			if (!(hdr->fhd2flags & fdDERIVED) &&
				type->ftvalstr != NULL)
 				STcopy(type->ftvalstr, (char *) data);
 			else
				*((char *) data) = EOS;
 		}
		break;

	  case frsFLDTBL:
		if (tag == FTABLE)
			*data = 1;
		else
			*data = 0;
		break;

	  case frsFLDMODE:
		if (tag == FTABLE)	/* Should request on object TABLE */
		{
			hdr = &(fld->fld_var.fltblfld->tfhdr);
			tb = RTgettbl(IIsirfrm, hdr->fhdname);
			switch (tb->tb_mode)
			{
			  case fdtfREADONLY:
				STcopy(ERx("READ"), (char *) data);
				break;

			  case fdtfUPDATE:
				STcopy(ERx("UPDATE"), (char *) data);
				break;

			  case fdtfAPPEND:
				STcopy(ERx("FILL"), (char *) data);
				break;

			  case fdtfQUERY:
				STcopy(ERx("QUERY"), (char *) data);
				break;
			}
		}
		else if (hdr->fhdflags & fdQUERYONLY)
		{
			STcopy(ERx("QUERY"), (char *) data);
		}
		else if ((hdr->fhd2flags & fdREADONLY)
			|| ((tag == FCOLUMN) && (hdr->fhdflags & fdtfCOLREAD))
			|| ((tag == FREGULAR) && (hdr->fhseq < 0)))
		{
			STcopy(ERx("READ"), (char *) data);
		}
		else
		{
			STcopy(ERx("FILL"), (char *) data);
		}
		break;

	  case frsNORMAL:
		if (tag == FTABLE)
		{
			IIFDerror(SIFLDDA, 0);
			break;
		}
		if (!(hdr->fhdflags & (fdRVVID | fdBLINK | fdUNLN | fdCHGINT)))
		{
			*data = 1;
		}
		else
		{
			*data = 0;
		}
		break;

	  case frsRVVID:
		if (tag == FTABLE)
		{
			IIFDerror(SIFLDDA, 0);
			break;
		}
		if (hdr->fhdflags & fdRVVID)
		{
			*data = 1;
		}
		else
		{
			*data = 0;
		}
		break;

	  case frsBLINK:
		if (tag == FTABLE)
		{
			IIFDerror(SIFLDDA, 0);
			break;
		}
		if (hdr->fhdflags & fdBLINK)
		{
			*data = 1;
		}
		else
		{
			*data = 0;
		}
		break;

	  case frsUNLN:
		if (tag == FTABLE)
		{
			IIFDerror(SIFLDDA, 0);
			break;
		}
		if (hdr->fhdflags & fdUNLN)
		{
			*data = 1;
		}
		else
		{
			*data = 0;
		}
		break;

	  case frsINTENS:
		if (tag == FTABLE)
		{
			IIFDerror(SIFLDDA, 0);
			break;
		}
		if (hdr->fhdflags & fdCHGINT)
		{
			*data = 1;
		}
		else
		{
			*data = 0;
		}
		break;

	  case frsCOLOR:
		if (tag == FTABLE)
		{
			IIFDerror(SIFLDDA, 0);
			break;
		}
		*data = RTfindcolor(hdr->fhdflags);
		break;

	  case frsSROW:
		IIFRfsz(fld,&srow,&scol,&nrow,&ncol);
		*data = srow + IIsifrm->frposy +
				(IIfrscb->frs_event)->scryoffset + 1;
		break;

	  case frsSCOL:
		IIFRfsz(fld,&srow,&scol,&nrow,&ncol);
		*data = scol + IIsifrm->frposx +
				(IIfrscb->frs_event)->scrxoffset + 1;
		break;

	  case frsHEIGHT:
		IIFRfsz(fld,&srow,&scol,&nrow,&ncol);
		*data = nrow;
		break;

	  case frsWIDTH:
		IIFRfsz(fld,&srow,&scol,&nrow,&ncol);
		*data = ncol;
		break;

	  case frsRDONLY:
		if (IIsifrm->frversion < BASEVERSION)
		{
			IIFDerror(SIOLDFRM, 1, ERx("displayonly"));
			break;
		}
		if (tag == FTABLE)
		{
			IIFDerror(SITBLRO, 0);
			break;
		}
		if ((hdr->fhd2flags & fdREADONLY)
			&& !(hdr->fhd2flags & fdDERIVED))
		{
			*data = 1;
		}
		else
		{
			*data = 0;
		}
		break;

	  case frsINVIS:
		if (IIsifrm->frversion < BASEVERSION)
		{
			IIFDerror(SIOLDFRM, 1, ERx("invisible"));
			break;
		}
		if (hdr->fhdflags & fdINVISIBLE)
		{
			*data = 1;
		}
		else
		{
			*data = 0;
		}
		break;

	  case frsDRVED:
		if (hdr->fhd2flags & fdDERIVED)
		{
			*data = 1;
		}
		else
		{
			*data = 0;
		}
		break;

	  case frsDRVSTR:
		*((char *) data) = EOS;
		if ((hdr->fhd2flags & fdDERIVED) && type->ftvalstr != NULL)
		{
			STcopy(type->ftvalstr, (char *) data);
		}
		break;

	  case frsDPREC:
		if (tag == FTABLE || abs(type->ftdatatype) != DB_DEC_TYPE)
		{
			*data = -1;
		}
		else
		{
			*data = type->ftprec/256;
		}
		break;

	  case frsDSCL:
		if (tag == FTABLE || abs(type->ftdatatype) != DB_DEC_TYPE)
		{
			*data = -1;
		}
		else
		{
			*data = type->ftprec%256;
		}
		break;

	  case frsDSZE:
		if (tag == FTABLE || abs(type->ftdatatype) != DB_DEC_TYPE)
		{
			*data = -1;
		}
		else
		{
			*data = type->ftprec;
		}
		break;

	  case frsEXISTS:
		if (fld != NULL)
		{
			*data = 1;
		}
		else
		{
			*data = 0;
		}
		break;

	  case frsEMASK:
		if (hdr->fhd2flags & fdEMASK)
		{
			*data = 1;
		}
		else
		{
			*data = 0;
		}
		break;

	  default:
		return (FALSE);			/* preprocessor bug */
	}
	return (TRUE);
}


/*{
** Name:	RTsetfld - Entry point for set_frs on a field/column.
**
** Description:
**	This routine is the entry point for supporting the set_frs
**	on a field statement.  Supported operations are:
**	 - Set/reset the mode for a field/column. (string)
**	 - Set/reset the color for a field/column. (integer)
**	 - Clear out display attributes (excluding color) for
**	   a field/column. (integer)
**	 - Set/reset the reverse video attribute for a field/column. (integer)
**	 - Set/reset the blink attribute for a field/column. (integer)
**	 - Set/reset the underline attribute for a field/column. (integer)
**	 - Set/reset the display intensity attribute for a field/column. (int)
**	 - Set/reset the readonly attribute for a field/column. (integer)
**	 - Set/reset the invisible attribute for a field. (integer)
**	 - Set/reset the data format of field/column. (string)
**
**	It is an error to reset the NORMAL (i.e., no) display attribute
**	for a field/column.  Other than for the mode and color operations,
**	the data to use in the setting must be zero or one.
**	The support for setting the mode of a table field dynamically
**	is incomplete and thus is guaranteed to not work.
**
** Inputs:
**	fld	Pointer to a FIELD structure.
**	regfld	Pointer to a REGFLD structure.
**	frsflg	Operation to perform.
**	data	Data to use in carrying out the operation.
**
** Outputs:
**	Returns:
**		TRUE	If requested operation was performed.
**		FALSE	If unknown operation or invalid object request
**			was received.
**	Exceptions:
**		None.
**
** Side Effects:
**	Various data structures may be updated along with actual
**	display changes.
**
** History:
**	xx/xx/xx (jen) - Initial version.
**	xx/xx/xx (ncg) - Added query only and read only options.
**	09/27/85 (dkh) - Added 4.0 features.
**	02/13/87 (dkh) - Added procedure header.
**	27-may-87 (mgw) Bug # 12539
**		Clear the color part of the hdr->fhdflags before putting
**		a new color there with an |= to prevent color changes from
**		reverting back to default color.
*/
i4
RTsetfld(FIELD *fld, REGFLD *regfld, i4 frsflg, i4 *data)
{
	FLDHDR		*hdr;
	FLDTYPE		*type;
	i4		tag;
	i4		onoff;
	TBSTRUCT	*tb;
	TBLFLD		*tbl = NULL;
	char		*chptr;
	i4		mode = 0;

	hdr = &regfld->flhdr;
	type = &regfld->fltype;
# ifdef BYTE_ALIGN
	MECOPY_CONST_MACRO((PTR)&fld->fltag, sizeof(tag), (PTR)&tag);
	MECOPY_CONST_MACRO((PTR)data, sizeof(*data), (PTR)&onoff);
# else
	tag = fld->fltag;

	onoff = *data;
# endif /* BYTE_ALIGN */

	if (frsflg != frsFLDMODE && frsflg != frsCOLOR && frsflg != frsFLDFMT)
	{
		if (onoff != 0 && onoff != 1)
		{
			return(FALSE);
		}
	}

	switch(frsflg)
	{
		case frsNORMAL:
		case frsRVVID:
		case frsBLINK:
		case frsUNLN:
		case frsINTENS:
		case frsCOLOR:
			if (tag == FTABLE)
			{
				IIFDerror(SIFLDDA, 0);
				return(FALSE);
			}
			break;
		case frsRDONLY:
			if (tag == FTABLE)
			{
				IIFDerror(SITBLRO, 0);
				return(FALSE);
			}
			break;
		case frsFLDFMT:
			if (tag == FTABLE)
			{
				IIFDerror(SITBLFMT, 0);
				return(FALSE);
			}
			break;
		default:
			break;

	}
	switch (frsflg)
	{
	  case frsFLDMODE:
	    {
	    	char ch = *(char *)data;

		if (tag == FREGULAR)	/* Should not be done on table */ 
		{ 
		    if ( ch == 'q' )
		    {
			hdr->fhdflags |= fdQUERYONLY;
			hdr->fhd2flags &= ~fdREADONLY;
		    }
		    else if ( ch == 'f' )
		    {
			hdr->fhdflags &= ~fdQUERYONLY;
		    }
		}
		else if (tag == FTABLE)
		{
		    hdr = &(fld->fld_var.fltblfld->tfhdr);
		    tb = RTgettbl(IIsirfrm, hdr->fhdname);
		    if (!tb->dataset)
		    {
			/* Can't set mode on bare table fields. */
			IIFDerror(E_FI226B_8811_ModeBareTF, 1, hdr->fhdname);
		    }
		    else
		    {
			if (ch == 'q')
			    mode = fdtfQUERY;
			else if (ch == 'r')
			    mode = fdtfREADONLY;
			else if (ch == 'f')
			    mode = fdtfAPPEND;
			else if (ch == 'u')
			    mode = fdtfUPDATE;

			if (!mode)
			{
			    /* Bad mode specified.  Error msg but do nothing. */
			    IIFDerror(E_FI226C_8812_BadTFMode,
				2, hdr->fhdname, (PTR)data);
			}
			else
			{
			    /*
			    **  Since we will be doing table field I/O, we
			    **  need to set things up to use the correct
			    **  frame.
			    */
			    IIfsetio(IIsirfrm->fdfrmnm);

			    IITBdsmDynSetMode(tb, mode, IIsifrm);
			}
		    }
		}
		else if (tag == FCOLUMN)
		{ 
		    if ( ch == 'q' )
		    {
			hdr->fhdflags &= ~fdtfCOLREAD;	/* Turn off disponly */
			hdr->fhd2flags &= ~fdREADONLY;	/* Turn off disponly */
			hdr->fhdflags |= fdQUERYONLY;	/* Turn on query */
		    }
		    else if ( ch == 'r' || ch == 'd' )
		    {
			hdr->fhdflags &= ~fdQUERYONLY;	/* Turn off query */
			hdr->fhd2flags |= fdREADONLY;	/* Turn on disponly */
		    }
		    else if ( ch == 'f' )
		    {
			hdr->fhdflags &= ~fdQUERYONLY;	/* Turn off query */
		    }
		}
	    }
	    break;

	  case frsNORMAL:
		if (onoff)
		{
			hdr->fhdflags &= ~(fdRVVID | fdBLINK | fdUNLN | fdCHGINT);
		}
		else
		{
			IIFDerror(SIDANOFF, 0);
			return(FALSE);
		}
		IIsifrm->frmflags |= fdREDRAW;
		break;

	  case frsRVVID:
		if (onoff)
		{
			hdr->fhdflags |= fdRVVID;
		}
		else
		{
			hdr->fhdflags &= ~fdRVVID;
		}
		IIsifrm->frmflags |= fdREDRAW;
		break;

	  case frsBLINK:
		if (onoff)
		{
			hdr->fhdflags |= fdBLINK;
		}
		else
		{
			hdr->fhdflags &= ~fdBLINK;
		}
		IIsifrm->frmflags |= fdREDRAW;
		break;

	  case frsUNLN:
		if (onoff)
		{
			hdr->fhdflags |= fdUNLN;
		}
		else
		{
			hdr->fhdflags &= ~fdUNLN;
		}
		IIsifrm->frmflags |= fdREDRAW;
		break;

	  case frsINTENS:
		if (onoff)
		{
			hdr->fhdflags |= fdCHGINT;
		}
		else
		{
			hdr->fhdflags &= ~fdCHGINT;
		}
		IIsifrm->frmflags |= fdREDRAW;
		break;

	  case frsFRMCHG:
		if (tag == FREGULAR)
		{
			if (*data == 0 || *data == 1)
			{
				IIFDfscb(fld, *data);
			}
		}
		break;

	  case frsRDONLY:
		if (IIsifrm->frversion < BASEVERSION)
		{
			IIFDerror(SIOLDFRM, 1, ERx("displayonly"));
			break;
		}
		if (hdr->fhd2flags & fdDERIVED)
		{
			IIFDerror(E_FI2268_8808_DispDerived, 1, hdr->fhdname);
			break;
		}
		if (onoff)
		{
			hdr->fhdflags &= ~fdQUERYONLY;	/* Turn off query */
			hdr->fhd2flags |= fdREADONLY;	/* Turn on disponly */
		}
		else
		{
			hdr->fhd2flags &= ~fdREADONLY;
		}
		break;

	  case frsINVIS:
		if (IIsifrm->frversion < BASEVERSION)
		{
			IIFDerror(SIOLDFRM, 1, ERx("invisible"));
			break;
		}

		if (onoff)
		{
			hdr->fhdflags |= fdINVISIBLE;
		}
		else
		{
			hdr->fhdflags &= ~fdINVISIBLE;
		}

		IIsifrm->frmflags |= fdREBUILD;
		if (tag == FTABLE)
		{
		    tbl = fld->fld_var.fltblfld;
		}
		else if (tag == FCOLUMN)
		{
		    tbl = IIsitbl;
		}

		if (tbl)
		{
		    IIFDtvcTblVisibilityChg(tbl);
		    /*
		    ** If table field is invisible, make sure that current
		    ** column is set to 0.
		    */
		    if (tbl->tfhdr.fhdflags & (fdINVISIBLE | fdTFINVIS))
		    {
			tbl->tfcurcol = 0;
		    }
		}

		break;

	  case frsCOLOR:
	  {
		i4	color = *data;

		if (color < fdMINCOLOR || color > fdMAXCOLOR)
		{
			i4	colour;

			colour = color;
			IIFDerror(SIBCOLOR, 1, &colour);
			return(FALSE);
		}
		hdr->fhdflags &= ~fdCOLOR;	/* Fix for bug 12539 */
		hdr->fhdflags |= RTselcolor(color);
		IIsifrm->frmflags |= fdREDRAW;
		break;
	  }

	  case frsFLDFMT:
		chptr = (char *)data;
		/*
		** IIsitbl will contain garbage unless the 'fld' is
		** really pointing towards a FLDCOL struct.
		** IIFDfss() needs to realize this.
		*/
		IIFDfssFmtStructSetup(IIsifrm, IIsitbl, chptr, fld);
		break;

	  case frsEMASK:
		if (onoff)
		{
			i4	rows;

			if (hdr->fhd2flags & fdEMASK)
			{
				break;
			}

			hdr->fhd2flags |= fdEMASK;
			rows = (type->ftwidth + type->ftdataln - 1)/type->ftdataln;
			if (!(hdr->fhd2flags & fdSCRLFD) &&
				!(hdr->fhdflags & (fdNOECHO | fdREVDIR)) &&
				(type->ftfmt->fmt_emask != NULL ||
				type->ftfmt->fmt_type == F_NT) && rows == 1)
			{
				hdr->fhd2flags |= fdUSEEMASK;
			}
		}
		else
		{
			hdr->fhd2flags &= ~(fdUSEEMASK | fdEMASK);
		}
		break;

	  default:
		return (FALSE);			/* preprocessor bug */
	}
	return (TRUE);
}

GLOBALREF RTATTLIST     IIattfld[];
