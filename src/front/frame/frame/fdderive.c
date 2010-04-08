/*
**	Copyright (c) 2004 Ingres Corporation
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
# include	<frsctblk.h>
# include	<erfi.h>


/**
** Name:	fdderive.c -	Routines to handle derivation evaluation
**
** Description:
**	The routines in this file handle derived field evaluation.  In order
**	for the routines to function properly, when operating on a table
**	field, the row being evaluated should be pointed at by
**	frm->frres2->rownum (if the row is visible) and by frm->frres2->dsrow
**	(if pointing at the dataset.)
**
**	Routines defined are:
**		IIFDgdvGetDeriveVal	Get value of a derived field
**		IIFDsdvSetDeriveVal	Set values of flds derived from this fld
**		IIFDidfInvalidDeriveFld	Invalidate a derived field
**		IIFDudfUpdateDeriveFld	Update a valid derived field
**		IIFDvsfValidateSrcFld	Validate a derivation source field
**		IIFDdecDerErrCatcher	Don't print derivation error messages
**		IIFDiaaInvalidAllAggs	Invalidate all agg src cols in a tbl
**		IIFDpadProcessAggDeps	Process aggregate dependents
**		IIFDdsgDoSpecialGDV	Special handling in IIFDgdv
**
** History:
**	16-jun-89 (bruceb)	Written
**	26-jun-90 (bruceb)
**		Turn off error messages when processing aggregate dependents.
**	08/23/90 (dkh) - Added changes to support table field cell highlighting.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


VOID	IIFDidfInvalidDeriveFld();
VOID	IIFDudfUpdateDeriveFld();
i4	IIFDvsfValidateSrcFld();
i4	IIFDvfValidateFld();

FUNC_EXTERN	STATUS	IIFVedEvalDerive();
FUNC_EXTERN	STATUS	IIFVeaEvalAgg();
FUNC_EXTERN	VOID	IIFViaInvalidateAgg();
FUNC_EXTERN	i4	FDdatafmt();
FUNC_EXTERN	FLDHDR	*FDgethdr();
FUNC_EXTERN	FLDTYPE	*FDgettype();
FUNC_EXTERN	FLDVAL	*FDgetval();
FUNC_EXTERN	i4	(*IIseterr())();
FUNC_EXTERN	i4	IIFDdecDerErrCatcher();
    
GLOBALREF FRS_RFCB	*IIFDrfcb;

static	bool	ignore_visited = FALSE;


/*{
** Name:	IIFDgdvGetDeriveVal	- Get value of a derived field
**
** Description:
**	Determine the value of the specified field by recursively determining
**	the value of source fields.
**		If an ancestor field is found to be invalid than derived fields
**		in the path between that invalid field and the field being
**		ultimately 'retrieved' are blanked.  If the original invalid
**		field was itself a derived field, it also is blanked.
**
**		Once an ancestor path is determined invalid, no subsequent
**		paths are examined.
**
**		If a field is marked as 'visited' then return.  The visited
**		marking indicates that this IIFDgdv call is part of the
**		process of setting a field (IIFDsdv).  No need to re-evaluate
**		this ancestor path of the field.  The exception to this
**		statement occurs when ignore_visited is TRUE.  This is set
**		by the aggregate-setup routine, aggregates requiring special
**		handling.
**
**	Actual evaluation of a value once all sources are determined to be
**	valid is handed off to derivation evaluation routines in the 'valid'
**	directory.
**
** Inputs:
**	frm		Frame containing the fields in question.
**	fld		Field whose value is to be determined.
**	colno		Sequence number of the column if fld is a table field,
**			else BADCOLNO if a simple field.
**
** Outputs:
**
**	Returns:
**		fdINVALID	If the value for this field is invalid.
**		fdNOCHG		If the value for this field is unchanged.
**		fdCHNG		If the value for this field has been modified.
**
**	Exceptions:
**		None
**
** Side Effects:
**
** History:
**	16-jun-89 (bruceb)	Written.
*/

i4
IIFDgdvGetDeriveVal(frm, fld, colno)
FRAME	*frm;
FIELD	*fld;
i4	colno;
{
    bool		modified = FALSE;
    i4			retval;
    FLDHDR		*hdr;
    FLDTYPE		*type;
    FLDVAL		*val;
    DRVREL		*sptr;		/* Source field pointer. */
    TBLFLD		*tbl;
    i4			valid;
    i4			*tfflags;
    DB_DATA_VALUE	*idbv;

    if (colno == BADCOLNO)
    {
	hdr = FDgethdr(fld);
	type = FDgettype(fld);
	val = FDgetval(fld);
	idbv = val->fvdbv;
	valid = (hdr->fhdflags & fdVALCHKED);
    }
    else
    {
	tbl = fld->fld_var.fltblfld;
	hdr = &(tbl->tfflds[colno]->flhdr);
	type = &(tbl->tfflds[colno]->fltype);
	idbv = (*IIFDrfcb->getTFdbv)(frm, fld, colno);
	tfflags = (*IIFDrfcb->getTFflags)(frm, fld, colno);
	valid = (*tfflags & fdVALCHKED);
    }

    /* 'Get' is nested inside a 'set' derivation. */
    if ((hdr->fhd2flags & fdVISITED) && !ignore_visited)
    {
	return(fdCHNG);
    }

    if (hdr->fhdrv != NULL)
    {
	for (sptr = hdr->fhdrv->srclist; sptr != NULL; sptr = sptr->next)
	{
	    /*
	    ** If currently on a simple field, but this source is a table
	    ** field column, than call aggregate derivation routines below
	    ** (via IIFVedEvalDerive) rather than IIFDgdv here.
	    */
	    if ((colno == BADCOLNO) && (sptr->colno != BADCOLNO))
	    {
		modified = TRUE;
	    }
	    else
	    {
		retval = IIFDgdvGetDeriveVal(frm, sptr->fld, sptr->colno);
		if (retval == fdINVALID)
		{
		    IIFDidfInvalidDeriveFld(frm, fld, colno);
		    return(fdINVALID);
		}
		else if (retval == fdCHNG)
		{
		    modified = TRUE;
		}
	    }
	}
    }

    /*
    ** If this is an already validated field, and no sources have been
    ** modified, simply return.
    **
    ** If this is a non-constant value derived field, re-evaluate.  If
    ** IIFVedEvalDerive() succeeded, than display dbv is already updated, so
    ** update the form if simple field or visible table field row.  If it
    ** failed however, than call IIFDidf() to blank the field.
    ** (Note:  Calling IIFVed() even if not modified because of need to
    ** handle case where math exception occurs, field is blanked, but since
    ** source field isn't subsequently modified, error messages wouldn't
    ** otherwise be again displayed on future probes.)
    **
    ** Finally, attempt to validate the (source) field.
    */
    if (valid && !modified)
    {
	return(fdNOCHG);
    }
    else if ((hdr->fhd2flags & fdDERIVED)
	&& !(hdr->fhdrv->drvflags & fhDRVCNST))
    {
	if (IIFVedEvalDerive(frm, hdr, type, idbv) == FAIL)
	{
	    IIFDidfInvalidDeriveFld(frm, fld, colno);
	    return(fdINVALID);
	}
	else
	{
	    IIFDudfUpdateDeriveFld(frm, fld, colno);
	    return(fdCHNG);
	}
    }
    else if (IIFDvsfValidateSrcFld(frm, fld, colno))
    {
	return(fdCHNG);		/* Field is valid. */
    }
    else
    {
	return(fdINVALID);
    }
}


/*{
** Name:	IIFDsdvSetDeriveVal	- Set values of fields derived from
**					  this field
**
** Description:
**	Modify the values of the derivation dependents of the specified
**	field.  If this field is invalid, than mark all dependents invalid.
**	For each field examined, call IIFDgdv() to obtain the values of it
**	and its ancestors.  Mark each field 'set' with a 'visited' flag so
**	that IIFDgdv() will not duplicate work.
**
** Inputs:
**	frm		Frame containing the fields in question.
**	fld		Field whose dependents are to be derived.
**	colno		Sequence number of the column if fld is a table field,
**			else BADCOLNO (-1) if a simple field.
**
** Outputs:
**
**	Returns:
**		OK	If not fdINVALID occurrence
**		FAIL	Otherwise
**
**	Exceptions:
**		None
**
** Side Effects:
**
** History:
**	16-jun-89 (bruceb)	Written.
*/

STATUS
IIFDsdvSetDeriveVal(frm, fld, colno)
FRAME	*frm;
FIELD	*fld;
i4	colno;
{
    i4		invalid;
    i4		ncolno;
    FIELD	*nfld;
    FLDHDR	*hdr;
    DRVREL	*dptr;			/* Dependent field pointer. */
    i4		*tfflags;
    STATUS	retval = OK;
    i4		gdv_return = fdNOCHG;

    if (colno == BADCOLNO)
    {
	hdr = FDgethdr(fld);
	invalid = !(hdr->fhdflags & fdVALCHKED);
    }
    else
    {
	hdr = &(fld->fld_var.fltblfld->tfflds[colno]->flhdr);
	tfflags = (*IIFDrfcb->getTFflags)(frm, fld, colno);
	invalid = !(*tfflags & fdVALCHKED);
    }

    hdr->fhd2flags |= fdVISITED;

    if (hdr->fhdrv != NULL)
    {
	for (dptr = hdr->fhdrv->deplist; dptr != NULL; dptr = dptr->next)
	{
	    nfld = dptr->fld;
	    ncolno = dptr->colno;
	    /*
	    ** If invalid, and NOT evaluating the dependent of an AGGSRC,
	    ** clear out the dependent field.  Otherwise, continue to
	    ** evaluate.
	    */
	    if (!invalid || ((colno != BADCOLNO) && (ncolno == BADCOLNO)))
	    {
		gdv_return = IIFDgdvGetDeriveVal(frm, nfld, ncolno);
	    }
	    else
	    {
		/* Clear out the dependent field. */
		IIFDidfInvalidDeriveFld(frm, nfld, ncolno);
	    }

	    if ((IIFDsdvSetDeriveVal(frm, nfld, ncolno) != OK)
		|| (gdv_return == fdINVALID))
	    {
		retval = FAIL;
	    }
	}
    }

    hdr->fhd2flags &= ~fdVISITED;
    return(retval);
}


/*{
** Name:	IIFDidfInvalidDeriveFld	- Invalidate a derived field
**
** Description:
**	Called to invalidate a derived field.  This involves turning off
**	the fdVALCHKED flag and blanking the field.
**
** Inputs:
**	frm		Frame containing the field in question.
**	fld		Field to be invalidated.
**	colno		If a table field, the column sequence number,
**			else BADCOLNO.
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**
** History:
**	19-jun-89 (bruceb)	Written.
*/

VOID
IIFDidfInvalidDeriveFld(frm, fld, colno)
FRAME	*frm;
FIELD	*fld;
i4	colno;
{
    FLDHDR	*hdr;
    i4		fldtype;
    i4		row;
    i4		*tfflags;
    i4		xflag;

    if (colno == BADCOLNO)
    {
	hdr = FDgethdr(fld);
	tfflags = &(hdr->fhdflags);
	fldtype = FT_REGFLD;
	row = 0;
    }
    else
    {
	hdr = &(fld->fld_var.fltblfld->tfhdr);
	fldtype = FT_TBLFLD;
	row = (*IIFDrfcb->getTFrow)(frm);
	tfflags = (*IIFDrfcb->getTFflags)(frm, fld, colno);
    }

    if (row == NOTVISIBLE)
    {
	*tfflags &= ~fdVALCHKED;
    }
    else
    {
	xflag = (*tfflags & fdX_CHG);
	FDclr(frm, hdr->fhseq, FT_UPDATE, fldtype, colno, row);
	*tfflags |= xflag;	/* Restore fdX_CHG -- turned off by FDclr(). */
    }
}


/*{
** Name:	IIFDudfUpdateDeriveFld	- Update a valid derived field
**
** Description:
**	Update a valid derived field.  That involves updating the field's
**	fdVALCHKED flag and calling FDdatafmt() on the field.
**
** Inputs:
**	frm		Frame containing the field in question.
**	fld		Field to be updated.
**	colno		If a table field, the column sequence number,
**			else BADCOLNO.
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**
** History:
**	20-jun-89 (bruceb)	Written.
*/

VOID
IIFDudfUpdateDeriveFld(frm, fld, colno)
FRAME	*frm;
FIELD	*fld;
i4	colno;
{
    FLDHDR	*hdr;
    FLDTYPE	*type;
    FLDVAL	*val;
    TBLFLD	*tbl;
    i4		fldtype;
    i4		row;
    i4		fldno;
    i4		*tfflags;

    if (colno == BADCOLNO)
    {
	hdr = FDgethdr(fld);
	type = FDgettype(fld);
	val = FDgetval(fld);
	hdr->fhdflags |= fdVALCHKED;
	fldtype = FT_REGFLD;
	row = 0;
	fldno = hdr->fhseq;
    }
    else
    {
	tfflags = (*IIFDrfcb->getTFflags)(frm, fld, colno);
	*tfflags |= fdVALCHKED;
	row = (*IIFDrfcb->getTFrow)(frm);
	if (row != NOTVISIBLE)
	{
	    tbl = fld->fld_var.fltblfld;
	    hdr = &(tbl->tfflds[colno]->flhdr);
	    type = &(tbl->tfflds[colno]->fltype);
	    val = tbl->tfwins + (row * tbl->tfcols) + colno;
	    fldtype = FT_TBLFLD;
	    fldno = tbl->tfhdr.fhseq;
	}
    }

    if (row != NOTVISIBLE)
    {
	(VOID) FDdatafmt(frm, fldno, FT_UPDATE, fldtype, colno, row,
	    hdr, type, val);
    }
}


/*{
** Name:	IIFDvsfValidateSrcFld	- Validate a derivation source field
**
** Description:
**	Validate a derivation source field.  Primarily involves setting
**	up arguments for call to normal validation routines.
**
**	Non-standard action occurs when the value to be validated is in a
**	non-visible section of a table field dataset.  If such occurs,
**	then exchange idbv and flags with row 0, validate there
**	without changing the display, and re-exchange afterwords.
**
** Inputs:
**	frm		Frame containing the field in question.
**	fld		Field to be validated.
**	colno		If a table field, the column sequence number,
**			else BADCOLNO.
**
** Outputs:
**
**	Returns:
**		Value of IIFDvfValidateFld().  (TRUE or FALSE.)
**
**	Exceptions:
**		None
**
** Side Effects:
**
** History:
**	20-jun-89 (bruceb)	Written.
*/

i4
IIFDvsfValidateSrcFld(frm, fld, colno)
FRAME	*frm;
FIELD	*fld;
i4	colno;
{
    FLDHDR		*hdr;
    TBLFLD		*tbl;
    i4			retval;
    i4			fldtype;
    i4			row;
    bool		visible_update = TRUE;
    i4			saveflags;
    DB_DATA_VALUE	*savedbv;
    i4			*flgs;

    if (colno == BADCOLNO)
    {
	fldtype = FT_REGFLD;
	hdr = FDgethdr(fld);
	row = 0;
    }
    else
    {
	fldtype = FT_TBLFLD;
	hdr = &(fld->fld_var.fltblfld->tfhdr);
	row = (*IIFDrfcb->getTFrow)(frm);
	if (row == NOTVISIBLE)
	{
	    visible_update = FALSE;
	    tbl = fld->fld_var.fltblfld;

	    /* Save flags and idbv of cell (0, colno). */
	    saveflags = (*(tbl->tffflags + colno) &
		(fdI_CHG | fdX_CHG | fdVALCHKED));
	    savedbv = (tbl->tfwins + colno)->fvdbv;

	    /* Set flags and idbv of cell (0, colno) from dataset. */
	    flgs = (*IIFDrfcb->getTFflags)(frm, fld, colno);
	    *(tbl->tffflags + colno) &= ~(fdI_CHG | fdX_CHG | fdVALCHKED);
	    *(tbl->tffflags + colno) |=
		(*flgs & (fdI_CHG | fdX_CHG | fdVALCHKED));
	    (tbl->tfwins + colno)->fvdbv
		= (*IIFDrfcb->getTFdbv)(frm, fld, colno);

	    row = 0;
	}
    }

    retval = IIFDvfValidateFld(frm, hdr->fhseq, FT_UPDATE, fldtype,
	colno, row, visible_update);
    
    if (!visible_update)
    {
	/* Save the flag values to the dataset. */
	*flgs &= ~(fdI_CHG | fdX_CHG | fdVALCHKED);
	*flgs |= (*(tbl->tffflags + colno) & (fdI_CHG | fdX_CHG | fdVALCHKED));

	/* Restore contents of cell (0, colno). */
	*(tbl->tffflags + colno) &= ~(fdI_CHG | fdX_CHG | fdVALCHKED);
	*(tbl->tffflags + colno) |= saveflags;
	(tbl->tfwins + colno)->fvdbv = savedbv;
    }

    return(retval);
}


/*{
** Name:	IIFDdecDerErrCatcher	- Don't print derivation error messages
**
** Description:
**	Don't print any trappable error messages caused by derivation
**	processing.  Error messages generated due to extra/new validations
**	are suppressed.
**
** Inputs:
**	errnum		Error number to check, and possibly to suppress.
**
** Outputs:
**
**	Returns:
**		0	error is to be suppressed
**
**	Exceptions:
**		None
**
** Side Effects:
**
** History:
**	31-jul-89 (bruceb)	Written.
*/

i4
IIFDdecDerErrCatcher(errnum)
i4	*errnum;
{
    return(0);
}


/*{
** Name:	IIFDiaaInvalidAllAggs	- Invalidate all agg src cols in a tbl
**
** Description:
**	Invalidate any aggregate source columns in the specified table field.
**
** Inputs:
**	frm	Frame on which invalidation occurs.
**	tbl	Table field in which to invalidate agg src cols.
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**
** History:
**	02-aug-89 (bruceb)	Written.
*/
VOID
IIFDiaaInvalidAllAggs(frm, tbl)
FRAME	*frm;
TBLFLD	*tbl;
{
    i4		i;
    FLDCOL	**col;
    FLDHDR	*hdr;

    /*
    ** Don't need to invalidate anything if there are no aggregate
    ** source columns in this table.
    */
    if (tbl->tfhdr.fhd2flags & fdAGGSRC)
    {
	frm->frmflags |= fdINVALAGG;
	col = tbl->tfflds;
	for (i = 0; i < tbl->tfcols; i++, col++)
	{
	    hdr = &((*col)->flhdr);
	    if (hdr->fhd2flags & fdAGGSRC)
	    {
		IIFViaInvalidateAgg(hdr, fdIANODE);
	    }
	}
    }
}


/*{
** Name:	IIFDpadProcessAggDeps	- Process aggregate dependents
**
** Description:
**	Any aggregate dependent fields on the form will be evaluated.
**
** Inputs:
**	frm	Form to be scanned for aggregate dependent fields.
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**
** History:
**	04-aug-89 (bruceb)	Written.
*/
VOID
IIFDpadProcessAggDeps(frm)
FRAME	*frm;
{
    i4		i;
    FIELD	**fld;
    FLDHDR	*hdr;
    i4		(*oldproc)();

    if (frm->frres2->formmode != fdrtQUERY)
    {
	if (frm->frmflags & fdINVALAGG)
	{
	    oldproc = IIseterr(IIFDdecDerErrCatcher);
	    fld = frm->frfld;
	    for (i = 0; i < frm->frfldno; i++, fld++)
	    {
		if ((*fld)->fltag == FREGULAR)
		{
		    hdr = FDgethdr(*fld);
		    if ((hdr->fhd2flags & fdDERIVED)
			&& (hdr->fhdrv->drvflags & fhDRVAGGDEP))
		    {
			/*
			** Obtain the value for the aggregate dependent field.
			** Then, set the value of any fields for which this
			** field is a source.
			*/
			_VOID_ IIFDgdvGetDeriveVal(frm, *fld, BADCOLNO);
			_VOID_ IIFDsdvSetDeriveVal(frm, *fld, BADCOLNO);
		    }
		}
	    }
	    frm->frmflags &= ~fdINVALAGG;
	    _VOID_ IIseterr(oldproc);
	}
    }
}


/*{
** Name:	IIFDdsgDoSpecialGDV	- Special handling in IIFDgdv
**
** Description:
**	Specify special processing in IIFDgdvGetDeriveVal.  Used by
**	aggregate setup routine to request derivation processing with no
**	regard to any notion of the table field column having already
**	been visited.
**
** Inputs:
**	value	TRUE/FALSE -- Is special handling requested?
**
** Outputs:
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**
** History:
**	25-aug-89 (bruceb)	Written.
*/
VOID
IIFDdsgDoSpecialGDV(value)
bool	value;
{
    ignore_visited = value;
}
