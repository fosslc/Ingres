/*
**	clrflds.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	clrflds.c - Routines to clear out fields.
**
** Description:
**	Various routines to clear out fields are defined here.  They are:
**	- FDclrflds - Clear display for all fields in a form.
**	- FDclr - Clear the display buffer.
**	- FDdat - Clear the data for a field.
**
** History:
**	JEN  - 1 Nov 1981 (written)
**	02/08/87 (dkh) - Added support for ADTs.
**	25-jun-87 (bruceb)	Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	09/16/87 (dkh) - Integrated table field change bit.
**	02/05/88 (dkh) - Fixed jup bug 1965.
**	12/05/88 (dkh) - Performance changes.
**	16-jun-89 (bruceb)
**		Modified FDclrflds() for derived fields.  Removed the
**		(unused) FDclrdat() routine.
**	16-nov-1999 (kitch01)
**		Bug 99039. clear field on a field with input masking on, will
**		remove the input mask. This result in ABF rejecting the value
**		entered as it is expecting to read the input mask characters 
**		which are no longer present in the field. I used the code from
**		putfrm.c which creates the input masks during form display.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	19-may-2001 (somsa01)
**	    Due to previous cross-integration, replaced nat and longnat with i4.
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
# include	"fdfuncs.h"
# include	<me.h>
# include	<st.h>

i4	FDclr();
i4	FDdat();

FUNC_EXTERN	FRAME	*FDFTgetiofrm();
FUNC_EXTERN	FIELD	*FDfldofmd();
FUNC_EXTERN	FLDVAL	*FDgetval();
FUNC_EXTERN	i4	FDfldtrv();
FUNC_EXTERN	STATUS	IIFDsdvSetDeriveVal();


static	ADF_CB	*loc_adfcb = NULL;



/*{
** Name:	FDclrflds - Clear the display buffers in a form.
**
** Description:
**	This routine clears the display buffers for each field
**	in the passed in form.  Including fields that are display
**	only.  The data portion of the field is not touched.
**
**	Derived fields may not be cleared by the application.
**	If a simple field is a derivation source field, call IIFDsdv()
**	to invalidate all its descendants.
**
** Inputs:
**	frm	Pointer to form that contains fields to clear.
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
**	02/15/87 (dkh) - Added procedure header.
*/
i4
FDclrflds(frm)
FRAME	*frm;
{
	reg	i4	fldno;
	reg	FRAME	*ofrm;
	FIELD		*fld;

	ofrm = FDFTgetiofrm();
	FDFTsetiofrm(frm);

	FDFTsmode(FT_UPDATE);

	for(fldno = 0; fldno < frm->frfldno; fldno++)
	{
	    fld = frm->frfld[fldno];
	    if (fld->fltag == FREGULAR)
	    {
		if (fld->fld_var.flregfld->flhdr.fhd2flags & fdDERIVED)
		{
		    /* Skip clearing this field. */
		    continue;
		}
	    }

	    FDfldtrv(fldno, FDclr, FDALLROW);

	    if (fld->fltag == FREGULAR)
	    {
		/* Table field derivations are dealt with in IIclrflds() */
		_VOID_ IIFDsdvSetDeriveVal(frm, fld, BADCOLNO);
	    }
	}

	FDFTsmode(FT_DISPONLY);

	for (fldno = 0; fldno < frm->frnsno; fldno++)
	{
	    FDfldtrv(fldno, FDclr, FDALLROW);
	}

	FDFTsetiofrm(ofrm);
	frm->frcurfld = 0;
	return (TRUE);
}


i4
FDclr(frm, fldno, disponly, fldtype, col, row)
FRAME	*frm;
i4	fldno;
i4	disponly;
i4	fldtype;
i4	col;
i4	row;
{
	DB_DATA_VALUE	*ddbv;
	DB_TEXT_STRING	*text;
	FIELD		*fld;
	TBLFLD		*tbl;
	FLDVAL		*val;
	FLDHDR		*hdr;
	i4		*pflags;

	fld = FDfldofmd(frm, fldno, disponly);
	hdr = FDgethdr(fld);

	if (fldtype == FT_TBLFLD)
	{
		tbl = fld->fld_var.fltblfld;
		pflags = tbl->tffflags + (row * tbl->tfcols) + col;
    	val = tbl->tfwins+(row * tbl->tfcols) + col;
	}
	else
	{
		pflags = &(hdr->fhdflags);
		val = FDgetval(fld);
	}

	ddbv = val->fvdsdbv;
	text = (DB_TEXT_STRING *) ddbv->db_data;
	/* Bug 99039. If input masking is turned on make sure we redisplay
	** the input mask.
	*/
	if (hdr->fhd2flags & fdUSEEMASK)
	{
		bool	dummy;
		u_char	buf[DB_GW4_MAXSTRING + 1];
		u_char	buf2[DB_GW4_MAXSTRING + 1];
		u_char	*from;
		u_char	*to;
		i4 	fmttype;
		i4  i=0;
		FLDTYPE *type = FDgettype(fld);
		i4  dslen;

		/* Empty out the field of the old value, if any*/
		adc_getempty(FEadfcb(), val->fvdbv);
		if (fmt_format(FEadfcb(), type->ftfmt,
			val->fvdbv, ddbv,
			(bool) TRUE) == OK)
		{
			/* Field is now all spaces */
			fmttype = type->ftfmt->fmt_type;
			MEcopy((PTR) text->db_t_text,
				text->db_t_count,
				(PTR) buf);
			buf[text->db_t_count] = EOS;
			if (fmttype == F_ST ||
				fmttype == F_DT)
			{
			/* put the format template into buf */
			 (void) fmt_stvalid(type->ftfmt,
				buf,
				type->ftfmt->fmt_width,
				0, &dummy);
			}

			/* setup to copy format template to field */
			from = buf;
			to = text->db_t_text;
			dslen = STlength((char *) buf);
			if (dslen > (ddbv->db_length - DB_CNTSIZE))
			{
				dslen = ddbv->db_length - DB_CNTSIZE;
			}
			/* copy template into field */
			for (i = 0; i < dslen; i++)
			{
				/*
				**  No need to worry
				**  about double byte
				**  here since we are
				**  just moving bytes
				**  around and not
				**  interepeting them.
				*/
				*to++ = *from++;
			}
			text->db_t_count = dslen;
		}
		else
		{
			/* Can't use format so treat as empty field */
			text->db_t_count = 0;
		}
	}
	else
	{
		/* No input masking so set field count to zero
		** This causes an empty field to be displayed in FTfldupd
		*/
		text->db_t_count = 0;
	}

	/*
	**  Clear out external change bit, set internal
	**  change bit and clear validated bit.
	*/
	*pflags &= ~(fdX_CHG | fdVALCHKED);
	*pflags |= fdI_CHG;

	FTfldupd(frm, fldno, disponly, fldtype, col, row);

	/*
	**  It is assumed that FTclrfld will also take care of
	**  setting the count for the display buffer to zero.
	**  FTclrfld(frm, fldno, disponly, fldtype, col, row);
	*/

	return (TRUE);	/* Return value is used as is needed in FDfldtrv() */
}

i4
FDdat(hdr, type, val)
FLDHDR	*hdr;
FLDTYPE	*type;
FLDVAL	*val;
{
	if (!loc_adfcb)
	{
		loc_adfcb = FEadfcb();
	}

	/*
	**  Should eventually check for bad return values.
	*/
	_VOID_ adc_getempty(loc_adfcb, val->fvdbv);

	return (TRUE);
}
