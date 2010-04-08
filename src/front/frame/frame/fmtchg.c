/*
**	Copyright (c) 2004 Ingres Corporation
*/

/*
NO_OPTIM = dg8_us5
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<fmtchg.h>
# include	<adf.h>
# include	<frame.h>
# include	<afe.h>
# include	<frserrno.h>
# include	<er.h>
# include	<erfi.h>
# include	"erfd.h"
# include	<ug.h>
# include	<cm.h>
# include	<me.h>
# include	<st.h>


/**
** Name:	fmtchg.c -	Handle changing a field's display format.
**
** Description:
**	This file defines:
**
**	IIFDfssFmtStructSetup	Create struct preparatory to changing a
**				field's display format.
**	IIFDcffChgFldFmt	Swap the formats, set fdREBUILD, reformat
**				the field contents.
**	IIFDshsSetHdrSize	Recalculate field size based on change in
**				rows/columns needed for this format.
**	IIFDfctFldColTraverse	Call the passed-in routine on the field or on
**				the displayed rows of the column.
**
** History:
**	08-feb-89 (bruceb)	Initial implementation.
**	22-feb-89 (bruceb)
**		Restrict formats for scrolling fields--disallow justified
**		formats.
**	04/07/89 (dkh) - Added parameter to IIUGbmaBadMemoryAllocation().
**	06/07/89 (dkh) - Fixed bug 6397.
**	04/03/90 (dkh) - Fixed bug 20486.
**	03/24/91 (dkh) - Integrated changes from PC group.
**	01/29/92 (jillb-DGC) - Added NO_OPTIM for dg8_us5 - this is required
**		so that embedded fortran will work on this platform.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
**/

FUNC_EXTERN	i4	FDputdata();
FUNC_EXTERN	i4	FDgetdata();
FUNC_EXTERN	i4	FDcmptbl();
FUNC_EXTERN	bool	fmt_justify();
STATUS			IIFDcffChgFldFmt();
VOID			IIFDshsSetHdrSize();
i4			IIFDfctFldColTraverse();


/*{
** Name:	IIFDfssFmtStructSetup	- Create struct preparatory to
**					  changing a field's display format.
**
** Description:
**	Given a format string and the field to be changed, determine
**	if the string is a good format, if the format is compatible with
**	the field's datatype, if neither the field nor the format are
**	'reverse' and if the new display size is acceptable.
**	If all such tests pass, than add a new FMT_INFO node to the
**	field's list (containing the new format's information).
**
** Inputs:
**	frm		Frame being modified.
**	tbl		Table field containing column being modified--if
**			fld is a column.
**	fmtstr		New format for the field.
**	fld		Field being modified.
**
** Outputs:
**	fld		Modified by addition of a new FMT_INFO node to
**			FLDTYPE struct if all OK.
**
**	Returns:
**		VOID
**
**	Exceptions:
**		None
**
** Side Effects:
**	None
**
** History:
**	08-feb-89 (bruceb)	Initial implementation.
*/
VOID
IIFDfssFmtStructSetup(frm, tbl, fmtstr, fld)
FRAME	*frm;
TBLFLD	*tbl;
char	*fmtstr;
FIELD	*fld;
{
    bool	valid;
    bool	reverse;
    bool	error = FALSE;
    i4		orows = 0;
    i4		ocolumns = 0;
    i4		rows = 0;
    i4		columns = 0;
    i4		fmtsize;
    i4		dataclass;
    char	*datatypenm;
    ADF_CB	*ladfcb;
    FMT_MAX	*maxstruct;
    FMT_INFO	*infoptr;
    FMT_INFO	*nextfmtptr = NULL;
    PTR		blk = NULL;
    FMT		*fmtptr;
    FLDHDR	*hdr;
    FLDTYPE	*type;
    DB_DATA_VALUE	dbv;

    /*
    ** Note that fld->fld_var.flregfld may really be pointing at
    ** a FLDCOL if this was a set_frs column statement.
    */
    hdr = &(fld->fld_var.flregfld->flhdr);
    type = &(fld->fld_var.flregfld->fltype);

    STtrmwhite(fmtstr);
    if (*fmtstr == EOS)
    {
	/* Error specifying format string */
	IIFDerror(CFFLFMT, 2, frm->frname, hdr->fhdname);
    }
    else if (hdr->fhdflags & fdREVDIR)
    {
	/* Can't change format on reverse fields */
	IIFDerror(SIFDRVFMT, 1, hdr->fhdname);
    }
    else
    {
	ladfcb = FEadfcb();

	if (fmt_size(ladfcb, type->ftfmt, (DB_DATA_VALUE *)NULL, &orows,
	    &ocolumns) != OK)
	{
	    /* Internal error--existing fmt should work properly */
	    IIUGerr(E_FD0028_internal_fmt_error, UG_ERR_ERROR, 2,
		type->ftfmtstr, hdr->fhdname);
	    error = TRUE;
	}
	else if (type->ftfu == NULL)
	{
	    /*
	    ** If type->ftfu == NULL then no prior attempt has been made to
	    ** change this field's display format.  Create the FMT_MAX struct
	    ** and hang it off of ftfu.
	    */
	    if ((type->ftfu = MEreqmem((u_i4)0, (u_i4)sizeof(FMT_MAX),
		FALSE, (STATUS *)NULL)) == NULL)
	    {
		IIUGbmaBadMemoryAllocation(ERx("IIFDfssFmtStructSetup"));
	    }
	    else
	    {
		maxstruct = (FMT_MAX *)(type->ftfu);
		maxstruct->maxrows = orows;
		maxstruct->maxcols = ocolumns;
		maxstruct->firstfmt = NULL;
	    }
	}
	else
	{
	    /*
	    ** Search the existing list to determine if the new format
	    ** matches a previous format.  If a match is found, point
	    ** nextfmtptr at the matching format's struct.
	    */
	    maxstruct = (FMT_MAX *)(type->ftfu);
	    infoptr = maxstruct->firstfmt;
	    while (infoptr != NULL)
	    {
		if (STcompare(fmtstr, infoptr->ftfmtstr) == 0)
		{
		    if (fmt_size(ladfcb, infoptr->ftfmt, (DB_DATA_VALUE *)NULL,
			&rows, &columns) != OK)
		    {
			/* Internal error--unable to get size of new format */
			IIUGerr(E_FD0028_internal_fmt_error, UG_ERR_ERROR, 2,
			    fmtstr, hdr->fhdname);
			error = TRUE;
		    }
		    else
		    {
			nextfmtptr = infoptr;
		    }
		    break;
		}
		infoptr = infoptr->next;
	    }
	}
	infoptr = NULL;	/* Help free memory later on. */

	if (!error && nextfmtptr == NULL)
	{
	    if (fmt_areasize(ladfcb, fmtstr, &fmtsize) != OK)
	    {
		error = TRUE;
	    }
	    else if ((blk = MEreqmem((u_i4)0, (u_i4)fmtsize, TRUE,
		(STATUS *)NULL)) == NULL)
	    {
		IIUGbmaBadMemoryAllocation(ERx("IIFDfssFmtStructSetup"));
	    }
	    else if (fmt_setfmt(ladfcb, fmtstr, blk, &fmtptr,
		(i4 *)NULL) != OK)
	    {
		error = TRUE;
	    }

	    if (error == TRUE)
	    {
		/* Error specifying format string */
		IIFDerror(CFFLFMT, 2, frm->frname, hdr->fhdname);
	    }
	    else
	    {
		/* Set only db_datatype; that's all that fmt_isvalid() uses. */
		dbv.db_datatype = type->ftdatatype;
		if ((fmt_isvalid(ladfcb, fmtptr, &dbv, &valid) != OK)
		    || (valid == FALSE))
		{
		    /* Display format not suitable for this field's datatype */
		    IIAFddcDetermineDatatypeClass(type->ftdatatype, &dataclass);
		    if (dataclass == NUM_DTYPE)
		    {
			datatypenm = ERget(S_FD0029_numeric);
		    }
		    else if (dataclass == CHAR_DTYPE)
		    {
			datatypenm = ERget(S_FD002A_character);
		    }
		    else if (dataclass == DATE_DTYPE)
		    {
			datatypenm = ERget(S_FD002B_date);
		    }
		    else
		    {
			datatypenm = ERx("");
		    }
		    IIFDerror(SIBADFMT, 3, fmtstr, hdr->fhdname, datatypenm);
		    error = TRUE;
		}
		else if ((fmt_isreversed(ladfcb, fmtptr, &reverse) != OK)
		    || (reverse == TRUE))
		{
		    /* Can't change to a reverse display format */
		    IIFDerror(SIFMTREV, 1, hdr->fhdname);
		    error = TRUE;
		}
		else if ((hdr->fhd2flags & fdSCRLFD)
		    && (fmt_justify(ladfcb, fmtptr) == TRUE))
		{
		    /* Can't give scrolling field a justified display format */
		    IIFDerror(SISCRLJST, 1, hdr->fhdname);
		    error = TRUE;
		}
		else if (fmt_size(ladfcb, fmtptr, (DB_DATA_VALUE *)NULL,
		    &rows, &columns) != OK)
		{
		    /* Internal error--unable to determine size of new format */
		    IIUGerr(E_FD0028_internal_fmt_error, UG_ERR_ERROR, 2,
			fmtstr, hdr->fhdname);
		    error = TRUE;
		}
		else if (rows > maxstruct->maxrows
		    || columns > maxstruct->maxcols)
		{
		    /* Size of the new display format out of bounds */
		    IIFDerror(SIFMTSZ, 4, fmtstr, hdr->fhdname,
			(PTR)(&maxstruct->maxrows), (PTR)(&maxstruct->maxcols));
		    error = TRUE;
		}
		else if (((infoptr = (FMT_INFO *)MEreqmem((u_i4)0,
		    (u_i4)sizeof(FMT_INFO), FALSE, (STATUS *)NULL)) == NULL)
		    || ((infoptr->ftfmtstr = STalloc(fmtstr)) == NULL))
		{
		    IIUGbmaBadMemoryAllocation(ERx("IIFDfssFmtStructSetup"));
		}
		else
		{
		    infoptr->ftfmt = fmtptr;
		    infoptr->ftwidth = rows * columns;
		    infoptr->ftdataln = columns;
		    infoptr->next = maxstruct->firstfmt;
		    maxstruct->firstfmt = infoptr;
		    nextfmtptr = infoptr;
		}
	    }
	}
	if (nextfmtptr != NULL)
	{
	    if (IIFDcffChgFldFmt(nextfmtptr, frm, tbl, fld) == OK)
	    {
		IIFDshsSetHdrSize(orows, ocolumns, rows, columns, fld);
	    }
	    else
	    {
		error = TRUE;
	    }
	}
	/*
	** If error occurred, free up fmt struct, fmt_info struct and
	** format string.
	*/
	if (error)
	{
	    if (blk != NULL)
	    {
		MEfree(blk);
	    }
	    if (infoptr != NULL)
	    {
		/*
		**  We need to check if maxstruct->firstfmt
		**  has been updated to point to infoptr.
		**  If so, we need to set maxstruct->firstfmt
		**  back to original pointer so we
		**  don't point to deallocated memory.
		*/
# ifdef PMFE
		if (type->ftfu && (maxstruct == type->ftfu)
			&& (maxstruct->firstfmt == infoptr))
		{
			maxstruct->firstfmt = infoptr->next;
			MEfree(infoptr->ftfmtstr);
			MEfree((PTR) infoptr);
		}
# else
		if (infoptr == maxstruct->firstfmt)
		{
			maxstruct->firstfmt = infoptr->next;
		}
		MEfree(infoptr->ftfmtstr);
		MEfree((PTR)infoptr);
# endif /* PMFE */
	    }
	}
    }
}


/*{
** Name:	IIFDcffChgFldFmt	- Swap the formats, set fdREBUILD,
**					  reformat the contents.
**
** Description:
**	Swap the new for the old format (save old fmt information in
**	the struct currently holding the new fmt information) and
**	set the fdREBUILD flag so that the next build of the form
**	will recalculate the field.  Also reformat the field contents,
**	and recalculate the field size (hdr->maxx,maxy) based on change
**	in number of rows, columns of new/old format.
**
** Inputs:
**	fmtptr	FMT_INFO pointer for the new format.
**	frm	Frame on which the changed field resides.
**	tbl	Table field containing column being modified--if a column.
**	fld	Field being modified.
**
** Outputs:
**	fld	Modified by addition of a new FMT_INFO node to
**		FLDTYPE struct if all OK.
**
**	Returns:
**		OK	If nothing fails;
**		FAIL	Otherwise.
**
**	Exceptions:
**		None
**
** Side Effects:
**	Will result in the form image being rebuilt at some later time.
**
** History:
**	08-feb-89 (bruceb)	Initial implementation.
*/
STATUS
IIFDcffChgFldFmt(fmtptr, frm, tbl, fld)
FMT_INFO	*fmtptr;
FRAME		*frm;
TBLFLD		*tbl;
FIELD		*fld;
{
    i4		fldno;
    i4		colno;
    i4		mode;
    i4		twidth;
    i4		tdataln;
    char	*tfmtstr;
    FMT		*tfmt;
    FLDHDR	*hdr;
    FLDTYPE	*type;
    REGFLD	*junk;
    STATUS	retval = OK;

    hdr = &(fld->fld_var.flregfld->flhdr);
    type = &(fld->fld_var.flregfld->fltype);

    /* Update the display dbv's so that next FTbuild will show proper data. */
    mode = FT_UPDATE;
    if (fld->fltag == FREGULAR)
    {
	tbl = NULL;	/* clear this out; contains garbage since not a col. */
	if ((fldno = hdr->fhseq) < 0)
	{
	    mode = FT_DISPONLY;
	    fldno = FDgtfldno(mode, frm, fld, &junk);
	}
    }
    else
    {
	/* The field's a column.  fld->fltag == FCOLUMN. */
	fldno = FDcmptbl(frm, tbl); /* no failure expected--survived ii40stiq */
	colno = hdr->fhseq;
    }

    /*
    ** Validate data to be reformatted.  If this fails then do no more.
    ** Else, reformat the data.
    **
    ** DO I WANT A LOCAL ERROR MESSAGE????
    */
    if (IIFDfctFldColTraverse(frm, tbl, fldno, mode, FDgetdata, colno))
    {
	/* The next FTbuild() will handle display attributes and box trim. */
	frm->frmflags |= fdREBUILD;

	/* Swap the format information. */
	twidth = type->ftwidth;
	tdataln = type->ftdataln;
	tfmtstr = type->ftfmtstr;
	tfmt = type->ftfmt;

	type->ftwidth = fmtptr->ftwidth;
	type->ftdataln = fmtptr->ftdataln;
	type->ftfmtstr = fmtptr->ftfmtstr;
	type->ftfmt = fmtptr->ftfmt;

	fmtptr->ftwidth = twidth;
	fmtptr->ftdataln = tdataln;
	fmtptr->ftfmtstr = tfmtstr;
	fmtptr->ftfmt = tfmt;

	_VOID_ IIFDfctFldColTraverse(frm, tbl, fldno, mode, FDputdata, colno);
    }
    else
    {
	IIFDerror(SIINVALID, 1, hdr->fhdname);
	retval = FAIL;
    }

    return(retval);
}


/*{
** Name:	IIFDshsSetHdrSize	- Recalculate field size based on
**					  change in rows/columns needed for
**					  this format.
**
** Description:
**	Given the sizes (rows/columns) of the old and new display formats,
**	determine the new size of the field (both title and data space.)
**	Simply take a delta of old and new sizes and apply that to
**	FLDHDR's fhmaxx and fhmaxy.
**
**	Does nothing for table field columns since changes to a table field's
**	size is undesirable.  (At least, for now.)
**
** Inputs:
**	orows		Number of lines in the old format.
**	ocolumns	Number of columns in the old format.
**	rows		Number of lines in the new format.
**	columns		Number of columns in the new format.
**	fld		Field being modified.
**
** Outputs:
**	fld	Contains new fhmaxx, fhmaxy members of fld's FLDHDR.
**
**	Returns:
**		None
**
**	Exceptions:
**		None
**
** Side Effects:
**	None
**
** History:
**	08-feb-89 (bruceb)	Initial implementation.
*/
VOID
IIFDshsSetHdrSize(orows, ocolumns, rows, columns, fld)
i4	orows;
i4	ocolumns;
i4	rows;
i4	columns;
FIELD	*fld;
{
    FLDHDR	*hdr;
    i4		deltax;
    i4		deltay;

    if (fld->fltag == FREGULAR)
    {
	hdr = &(fld->fld_var.flregfld->flhdr);

	deltay = orows - rows;
	deltax = ocolumns - columns;

	hdr->fhmaxy -= deltay;
	hdr->fhmaxx -= deltax;
    }
}

/*{
** Name:	IIFDfctFldColTraverse	- Call the passed-in routine on the
**					  field or the displayed rows of the
**					  column.
**
** Description:
**	Call the routine passed in on the specified field, or if the field
**	is a table field (evidenced by a non-NULL tbl parameter) than on
**	all displayed rows of the specified column.
**
** Inputs:
**	frm		Frame on which the field being traversed resides.
**	tbl		If a table field, than the field containing the
**			column being traversed.
**	fldno		Sequence number of the field being traversed.
**	mode		Mode of the field being traversed.
**	routine		Routine to call on the specified field/column.
**	col		Sequence number of the column being traversed.
**
** Outputs:
**
**	Returns:
**		TRUE/FALSE depending on the return value of the called routine.
**
**	Exceptions:
**		None
**
** Side Effects:
**	None
**
** History:
**	08-feb-89 (bruceb)	Initial implementation.
*/
i4
IIFDfctFldColTraverse(frm, tbl, fldno, mode, routine, col)
FRAME	*frm;
TBLFLD	*tbl;
i4	fldno;
i4	mode;
i4	(*routine)();
i4	col;
{
    i4		row;
    i4		oldrow;
    i4		oldcol;
    i4		lastrow;

    if (tbl)
    {
	oldrow = tbl->tfcurrow;
	oldcol = tbl->tfcurcol;
	lastrow = tbl->tflastrow + 1;
	for (row = 0; row < lastrow; row++)
	{
	    if (!(*routine)(frm, fldno, mode, FT_TBLFLD, col, row))
	    {
		return(FALSE);
	    }
	}
	tbl->tfcurrow = oldrow;
	tbl->tfcurcol = oldcol;
	return(TRUE);
    }
    else
    {
	return((*routine)(frm, fldno, mode, FT_REGFLD, (i4) 0, (i4) 0));
    }
}
