/*
**	fdcvto60.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	fdcvto60.c - Convert "OLD" compiled form to 6.0 format.
**
** Description:
**	This file contains routines to convert OLD (5.0 and earlier)
**	compiled forms format to 6.0 format.
**
** History:
**	02/05/87 (dkh) - Initial version.
**	04/09/87 (dkh) - Added forward declarations of routine return values.
**	19-jun-87 (bruceb) Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	07-apr-88 (bruceb)
**		Changed from using sizeof(DB_TEXT_STRING)-1 to using
**		DB_CNTSIZE.  Previous calculation is in error.
**	17-apr-90 (bruceb)
**		Lint changes; removed args from calls on FDflalloc,
**		FDtblalloc and FDcolalloc.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
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
# include	<frame.h>
# include	<oframe.h>
# include	<er.h>


FUNC_EXTERN	FLDCOL	*FDnewcol();
FUNC_EXTERN	FIELD	*FDnewfld();
FUNC_EXTERN	i4	FDflalloc();
FUNC_EXTERN	i4	FDtblalloc();
FUNC_EXTERN	i4	FDcolalloc();
FUNC_EXTERN	char	*FEsalloc();



FIELD	*FD60fldbld();
TRIM	*FD60trmbld();
i4	FD60tycp();
VOID	FD60hdrcp();
VOID	FD60valcp();

/*{
** Name:	FDcvto60 - Convert a compiled form to 6.0 format.
**
** Description:
**	This routine is the entry point for converting an older
**	(5.0 and earlier) compiled form format into the new
**	(6.0) format.
**
** Inputs:
**	ofrm	Pointer to an OLD frame (OFRAME) structure.
**
** Outputs:
**	None.
**
**	Returns:
**		TRUE	If conversion was successful.
**		FALSE	If conversion failed.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/05/87 (dkh) - Initial version.
*/
FRAME *
FDcvto60(ofrm)
OFRAME	*ofrm;
{
	FRAME	*nfrm = NULL;
	i4	maxcnt;
	i4	i;

	/*
	**  Allocate new frame structure.  Tag
	**  region is assumed to be around.
	*/
	if ((nfrm = FDnewfrm(ofrm->frname)) == NULL)
	{
		return(nfrm);
	}

	/*
	**  Copy other information over to new structure.
	*/
	nfrm->frfldno = ofrm->frfldno;
	nfrm->frnsno = ofrm->frnsno;
	nfrm->frtrimno = ofrm->frtrimno;
	nfrm->frmaxx = ofrm->frmaxx;
	nfrm->frmaxy = ofrm->frmaxy;
	nfrm->frposx = ofrm->frposx;
	nfrm->frposy = ofrm->frposy;
	nfrm->frmflags = ofrm->frmflags;

	/*
	**  Allocate field and trim arrays.
	*/
	if (!FDfralloc(nfrm))
	{
		return(NULL);
	}

	/*
	**  Allocate individual fields in update chain.
	*/
	maxcnt = nfrm->frfldno;
	for (i = 0; i < maxcnt; i++)
	{
		if ((nfrm->frfld[i] = FD60fldbld(nfrm, ofrm->frfld[i])) == NULL)
		{
			return(NULL);
		}
	}

	/*
	**  Allocate individual fields in display only chain.
	*/
	maxcnt = nfrm->frnsno;
	for (i = 0; i < maxcnt; i++)
	{
		if ((nfrm->frnsfld[i] = FD60fldbld(nfrm,
			ofrm->frnsfld[i])) == NULL)
		{
			return(NULL);
		}
	}

	/*
	**  Allocate trims.
	*/
	maxcnt = nfrm->frtrimno;
	for (i = 0; i < maxcnt; i++)
	{
		if ((nfrm->frtrim[i] = FD60trmbld(ofrm->frtrim[i])) == NULL)
		{
			return(NULL);
		}
	}

	return(nfrm);
}


/*{
** Name:	FD60bldfld - Build a 6.0 field.
**
** Description:
**	Build a 6.0 field from a 5.0 (or earlier) field.
**
** Inputs:
**	nfrm	A 6.0 form.
**	ofld	A 5.0 field.
**
** Outputs:
**	Returns:
**		fld	A 6.0 field.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/19/87 (dkh) - Initial version.
*/
FIELD *
FD60fldbld(nfrm, ofld)
OFRAME	*nfrm;
OFIELD	*ofld;
{
	FIELD	*fld = NULL;
	TBLFLD	*tbl = NULL;
	OTBLFLD *otbl = NULL;
	REGFLD	*rfld = NULL;
	OREGFLD *orfld = NULL;
	FLDCOL	**col = NULL;
	OFLDCOL **ocol = NULL;
	i4	i = 0;
	i4	maxcnt = 0;

	orfld = ofld->fld_var.flregfld;
	if ((fld = FDnewfld(orfld->flhdr.fhdname, orfld->flhdr.fhseq,
		ofld->fltag)) == NULL)
	{
		return(fld);
	}

	/*
	**  Should only be regular and table fields.
	*/
	if (fld->fltag == FREGULAR)
	{
		rfld = fld->fld_var.flregfld;
		orfld = ofld->fld_var.flregfld;
		/*
		**  Copy components of a FLDHDR, FLDTYPE, FLDVAL.
		*/
		FD60hdrcp(&(orfld->flhdr), &(rfld->flhdr));
		if (!FD60tycp(&(orfld->fltype), &(rfld->fltype)))
		{
			return(NULL);
		}
		FD60valcp(&(orfld->flval), &(rfld->flval));

		if (!FDflalloc(fld))
		{
			return(NULL);
		}

	}
	else
	{
		/*
		**  Build a table field.
		*/
		tbl = fld->fld_var.fltblfld;
		otbl = ofld->fld_var.fltblfld;
		FD60hdrcp(&(otbl->tfhdr), &(tbl->tfhdr));
		tbl->tfrows = otbl->tfrows;
		tbl->tfcols = otbl->tfcols;
		tbl->tfstart = otbl->tfstart;
		tbl->tfwidth = otbl->tfwidth;
		if (!FDtblalloc(tbl))
		{
			return(NULL);
		}
		maxcnt = tbl->tfcols;
		ocol = otbl->tfflds;
		col = tbl->tfflds;
		for (i = 0; i < maxcnt; i++, ocol++, col++)
		{
			if ((*col = FDnewcol((*ocol)->flhdr.fhdname,
				(*ocol)->flhdr.fhseq)) == NULL)
			{
				return(NULL);
			}
		}
		ocol = otbl->tfflds;
		col = tbl->tfflds;
		for (i = 0; i < maxcnt; i++, ocol++, col++)
		{
			FD60hdrcp(&((*ocol)->flhdr), &((*col)->flhdr));
			if (!FD60tycp(&((*ocol)->fltype), &((*col)->fltype)))
			{
				return(NULL);
			}
		}
		for (i = 0, col = tbl->tfflds; i < maxcnt; i++, col++)
		{
			if (!FDcolalloc(tbl, *col))
			{
				return(NULL);
			}
		}
	}
	return(fld);
}



/*{
** Name:	FD60trmbld - Build a 6.0 trim.
**
** Description:
**	Build a 6.0 trim from a 5.0 trim.
**
** Inputs:
**	otrim	A 5.0 trim to build
**
** Outputs:
**	Returns:
**		trim	Pointer to a 6.0 trim.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/19/87 (dkh) - Initial version.
*/
TRIM *
FD60trmbld(otrim)
OTRIM	*otrim;
{
	return(FDnewtrim(otrim->trmstr, otrim->trmy, otrim->trmx));
}



/*{
** Name:	FD60hdrcp - Copy a FLDHDR structure.
**
** Description:
**	Copy information from a 5.0 FLDHDR structure into a 6.0 one.
**
** Inputs:
**	ohdr	5.0 version of FLDHDR strucutre.
**	hdr	6.0 version of FLDHDR structure.
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
**	02/19/87 (dkh) - Initial version.
*/
VOID
FD60hdrcp(ohdr, hdr)
OFLDHDR *ohdr;
FLDHDR	*hdr;
{
	hdr->fhtitle = ohdr->fhtitle;
	hdr->fhtitx = ohdr->fhtitx;
	hdr->fhtity = ohdr->fhtity;
	hdr->fhposx = ohdr->fhposx;
	hdr->fhposy = ohdr->fhposy;
	hdr->fhmaxx = ohdr->fhmaxx;
	hdr->fhmaxy = ohdr->fhmaxy;
	hdr->fhdflags = ohdr->fhdflags;
}



/*{
** Name:	FD60tycp - Copy a FLDTYPE structure.
**
** Description:
**	Copy information from a 5.0 FLDTYPE structure into a 6.0 one.
**
** Inputs:
**	otype	5.0 version of FLDTYPE structure.
**	type	6.0 version of FLDTYPE structure.
**
** Outputs:
**	Returns:
**		TRUE	If conversion worked.
**		FALSE	If conversion failed.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/19/87 (dkh) - Initial version.
*/
i4
FD60tycp(otype, type)
OFLDTYPE	*otype;
FLDTYPE		*type;
{
	i4		dtype;
	char		*fmtstr;
	char		fmtbuf[200];

	type->ftdefault = otype->ftdefault;

	/*
	**  No need to convert length since only
	**  'c' data type was available before 6.0.
	*/
	type->ftlength = otype->ftlength;

	type->ftwidth = otype->ftwidth;
	type->ftdatax = otype->ftdatax;
	type->ftdataln = otype->ftdataln;

	/*
	**  Convert old datatype to new datatype encoding.
	*/
	dtype = otype->ftdatatype;
	fmtstr = otype->ftfmtstr;
	switch(dtype)
	{
		case fdINT:
			dtype = DB_INT_TYPE;
		case fdFLOAT:
			if (dtype == fdFLOAT)
			{
				dtype = DB_FLT_TYPE;
			}

			/*
			**  Put on a leading - sign if needed.
			*/
			if (*fmtstr != '-' && *fmtstr != '+')
			{
				STprintf(fmtbuf, ERx("-%s"), fmtstr);
				if ((fmtstr = FEsalloc(fmtbuf)) == NULL)
				{
					IIUGbmaBadMemoryAllocation(ERx("FD60tycp"));
					return(FALSE);
				}
			}
			break;
		case fdSTRING:
			/*
			**  Convert to "text" data type
			**  if data length is longer than
			**  the maximum for a 'c' data type.
			*/
			if (type->ftlength > 255)
			{
				dtype = DB_TXT_TYPE;
				type->ftlength += DB_CNTSIZE;
			}
			else
			{
				/*
				**  FDstoralloc() will add extra
				**  byte to hold EOS.
				*/
				dtype = DB_CHR_TYPE;
			}

			/*
			**  If format has a leading - sign on it,
			**  take it away so things work correctly
			**  with 6.0 format routines.
			*/
			if (*fmtstr == '-')
			{
				fmtstr++;
			}
			break;
	}
	type->ftdatatype = dtype;

	type->ftvalstr = otype->ftvalstr;
	type->ftvalmsg = otype->ftvalmsg;

	type->ftfmtstr = fmtstr;

	return(TRUE);
}



/*{
** Name:	FD60valcp - Copya a FLDVAL structure.
**
** Description:
**	Copy information from a 5.0 FLDVAL structure into a 6.0 one.
**
** Inputs:
**	oval	5.0 version of FLDVAL structure.
**	val	6.0 version of FLDVAL structure.
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
**	02/19/87 (dkh) - Initial version.
*/
VOID
FD60valcp(oval, val)
OFLDVAL *oval;
FLDVAL	*val;
{
	val->fvdatay = oval->fvdatay;
}
