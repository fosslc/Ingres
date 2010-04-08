/*
** Copyright (c) 2004 Ingres Corporation
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
# include	<multi.h>
# include	<ex.h>
# include	<me.h>
# include	<st.h>
# include	<er.h>
# include	<erfi.h>
# include	"erfd.h"
# include	<scroll.h>


/*
**	GETDATA.c  -  Get Data from a Field
**
**	Fdgetdata() gets the contents of the data window of a field,
**	interprets those contents, and places the value in the storage
**	area of the field according to its data type.
**
**	This routine is used for retrieving data and checking data in
**	the frame driver.
**
**	Data is currently checked according to datatype.
**
**	Argument: fld - field to get data from
**
**	History:  JEN - 1 Nov 1981  (written)
**		       18 Feb 1983 - numbers are default formatted to be
**				     left justified (nml)
**		       20 Oct 1983 - char fields now right justified if
**				     is specified as such (bug #1414).
**				   - Increased bufr size to handle text -
**				     use DB_MAXSTRING as define in ingconst.h
**				     (nml).
**	8-dec-86 (bruceb)	Fix for bugs 10917 and 10139.
**		When downloading data from right-justified fields
**		into permanent storage, ignore any leading blanks.
**	19-jun-87 (bruceb) Code cleanup.
**	06/19/87 (dkh) - Fixed mandatory and nullable field behavior.
**	13-jul-87 (bruceb) Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	10/12/87 (dkh) - Fixed jup bug 434.
**	10/14/87 (dkh) - Made error messages trappable.
**	10/29/87 (dkh) - Fixed so that clearing out a nullable field
**			 will result in a NULL value for the field.
**	09-nov-87 (bruceb)
**		For scrolling fields, create a temporary format
**		with the same data type as before, but with a single
**		row of width equivalent to the entire scrolling buffer.
**		This is used when transferring data between the
**		display and internal buffers to avoid truncation of
**		the data to the size of the field's visible window.
**	12/23/87 (dkh) - Performance changes.
**	09-feb-88 (bruceb)
**		fdSCRLFD now in fhd2flags; can't use most significant
**		bit of fhdflags.
**	07-apr-88 (bruceb)
**		Changed from using sizeof(DB_TEXT_STRING)-1 to using
**		DB_CNTSIZE.  Previous calculation is in error.
**	15-apr-1988 (danielt)
**		Added EXdelete() to match EXdeclare()
**	29-apr-88 (bruceb)
**		After a failed fmt_cvt(), check against ABS of the datatype.
**	16-may-88 (bruceb)
**		Only update the field flags (fdI_CHG and fdVALCHKED) if
**		FDdatafmt succeeds.
**	06/18/88 (dkh) - Integrated CMS changes.
**	10/13/88 (dkh) - Fixed jup bug 2712.
**	13-oct-88 (sylviap)
**		Added TITAN changes.  DB_MAXSTRING -> DB_GW4_MAXSTRING.
**	11/01/88 (dkh) - Performance changes.
**	12/01/88 (dkh) - Fixed cross field validation problem.
**	19-jun-89 (bruceb)
**		Abstracted contents of FDgetdata into IIFDvfValidateFld().
**		IIFDvfValidateFld is called with additional argument which
**		indicates whether or not to display the changes.  This param
**		is only useful for derived field processing, and is otherwise
**		TRUE.  Only call vl_evalfld() for non-derived fields.
**	06-mar-90 (bruceb)
**		Pass down frm->frtag to IIFDssf(), and no longer MEfree fmtptr.
**	18-apr-90 (bruceb)
**		Lint change; vl_evalfld now called with 1 arg, not 3.
**      04/25/90 (esd) - Add val and frm as 2nd and 3rd parms to vl_evalfld.
**	02-may-90 (bruceb)
**		An empty mandatory field with fdCLRMAND set will generate
**		message E_FI226E instead of E_FI21CA.
**      05/06/90 (esd) - If mandatory field omitted or bad data entered,
**                       give FT3270 an opportunity to position cursor
**                       and set frcurfld and tfcurrow/col.
**	03-jul-90 (bruceb)	Fix for 31018.
**		Field data error messages now give field title and name when
**		available (instead of 'current field').  IIFDfde() does most
**		of the work.
**	06/26/92 (dkh) - Added support for decimal datatype for 6.5.
**	12/12/92 (dkh) - Added support for edit masks.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**      24-sep-96 (hanch04)
**          Global data moved to framdata.c
**      06-Nov-98 (hanal04)
**              Ensure field validate uses the number separator
**              specified by II_DECIMAL. b92987, b93524.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
*/

FUNC_EXTERN	FLDHDR	*FDgethdr();
FUNC_EXTERN	FLDVAL	*FDgetval();
FUNC_EXTERN	FLDTYPE *FDgettype();
FUNC_EXTERN	FIELD	*FDfldofmd();
FUNC_EXTERN	i4	FDdatafmt();
FUNC_EXTERN	STATUS	FDflterr();
FUNC_EXTERN	VOID	IIFDssfSetScrollFmt();
FUNC_EXTERN	i4	IIFDgdvGetDeriveVal();
FUNC_EXTERN	i4	(*IIseterr())();
FUNC_EXTERN	i4	IIFDdecDerErrCatcher();

GLOBALREF FRS_GLCB	*IIFDglcb;

i4	IIFDvfValidateFld();
VOID	IIFDfdeFldDataError();


GLOBALREF	PTR	IIFDcdCvtData ;
GLOBALREF	char	IIFDftFldTitle[fdTILEN + 1];
GLOBALREF	char	IIFDfinFldInternalName[FE_MAXNAME + 1];

EX
FDadfxhdlr(ex)
EX_ARGS *ex;
{
	if (ex->exarg_num == EXFLTOVF || ex->exarg_num == EXINTOVF ||
		ex->exarg_num == EXDECOVF || ex->exarg_num == EXDECDIV)
	{
		IIFDfdeFldDataError(E_FI21CE_8654, 0);
	}
	else if (ex->exarg_num == EXFLTUND)
	{
		IIFDfdeFldDataError(E_FI21CF_8655, 0);
	}
	else
	{
		return(EXRESIGNAL);
	}

	return(EXDECLARE);
}


i4
FDnodata(frm, fldno, disponly, fldtype, col, row)
FRAME	*frm;
i4	fldno;
i4	disponly;
i4	fldtype;
i4	col;
i4	row;
{
	return(FDgetdata(frm, fldno, disponly, fldtype, col, row));
}



i4
FDgetdata(frm, fldno, disponly, fldtype, col, row)
FRAME	*frm;
i4	fldno;
i4	disponly;
i4	fldtype;
i4	col;
i4	row;
{
    FIELD	*fld;
    TBLFLD	*tbl;
    FLDHDR	*hdr;
    i4		ocol;
    i4		orow;
    i4		retval;
    i4		(*oldproc)();

    fld = FDfldofmd(frm, fldno, disponly);

    if (fldtype == FT_TBLFLD)
    {
	tbl = fld->fld_var.fltblfld;
	ocol = tbl->tfcurcol;
	orow = tbl->tfcurrow;
	tbl->tfcurcol = col;
	tbl->tfcurrow = row;
	hdr = FDgethdr(fld);
	tbl->tfcurcol = ocol;
	tbl->tfcurrow = orow;
	frm->frres2->rownum = row;
    }
    else
    {
	hdr = FDgethdr(fld);
	col = BADCOLNO;
    }

    if (hdr->fhd2flags & fdDERIVED)
    {
	/*
	** If in middle of bulk validation, no need to validate derived
	** simple fields/columns directly; evaluate via the IIFDsdv() call
	** below.
	*/
	if (IIFDglcb->in_validation & VLD_BULK)
	{
	    retval = TRUE;
	}
	else
	{
	    /*
	    ** Don't supress error messages since specifically validating
	    ** this derived field.
	    */
	    retval = IIFDgdvGetDeriveVal(frm, fld, col);

	    if (retval == fdINVALID)
	    {
		retval = FALSE;
	    }
	    else
	    {
		retval = TRUE;
	    }

	    if (hdr->fhdrv->deplist)	/* Also a source field. */
	    {
		/*
		** Call IIFDsdv() so that dependents are as visible as
		** the sources.
		*/
		oldproc = IIseterr(IIFDdecDerErrCatcher);

		_VOID_ IIFDsdvSetDeriveVal(frm, fld, col);

		_VOID_ IIseterr(oldproc);
	    }
	}
    }
    else
    {
	retval = IIFDvfValidateFld(frm, fldno, disponly, fldtype,
	    col, row, (bool)TRUE);

	if (hdr->fhdrv)	/* Derivation source field. */
	{
	    /*
	    ** Call IIFDsdv() so that dependents are as visible as
	    ** the sources.
	    */
	    oldproc = IIseterr(IIFDdecDerErrCatcher);

	    _VOID_ IIFDsdvSetDeriveVal(frm, fld, col);

	    _VOID_ IIseterr(oldproc);
	}
    }

    return(retval);
}


/*{
** Name:	IIFDvfValidateFld	- Validate the contents of a field
**
** Description:
**	Routine called by FDgetdata and FDvalidate to do the main work of
**	validating a field.
**
** Inputs:
**	frm
**	fldno
**	disponly	FT_UPDATE or FT_DISPONLY.
**	fldtype		FT_TBLFLD or FT_REGFLD.
**	col		Column number if field is a table field.
**	row		Row number if field is a table field.
**	dispchg		TRUE if display is to be updated.
**
** Outputs:
**
**	Returns:
**		TRUE if all went well, else FALSE.
**
**	Exceptions:
**		None
**
** Side Effects:
**
** History:
**	19-jun-89 (bruceb)
**		Code yanked from FDgetdata/FDvalidate.
**      04-Sep-98 (hanal04)
**              Ensure field validate uses the number separator
**              specified by II_DECIMAL. Repeated resolution of
**              II_DECIMAL is avoided with the use of a static
**              variable. b92987, b93524.
**              Cross integration includes carsu07's change for
**              case where II_DECIMAL is not set.
*/
i4
IIFDvfValidateFld(frm, fldno, disponly, fldtype, col, row, dispchg)
FRAME	*frm;
i4	fldno;
i4	disponly;
i4	fldtype;
i4	col;
i4	row;
bool	dispchg;
{
    DB_DATA_VALUE	*dbv;
    DB_DATA_VALUE	*ddbv;
    DB_TEXT_STRING	*text;
    FIELD		*fld;
    TBLFLD		*tbl;
    FLDHDR		*hdr;
    FLDTYPE		*type;
    FLDVAL		*val;
    i4			ocol;
    i4			orow = -1;
    i4			*pflag;
    i4			flag;
    DB_DATA_VALUE	mdbv;
    ADF_CB		*ladfcb;
    i4			isnull;
    ER_MSGID		error_id;
    EX_CONTEXT		context;
    FMT			*fmtptr;
    bool		scrolling = FALSE;
    i4			retval = TRUE;
    i4			fmtflags = PM_NO_CHECK;
    i4			sqlpat = PM_NOT_FOUND;
    /* b92987 */
    char                *cp;
    static char         ii_decimal = NULLCHAR;


    fld = FDfldofmd(frm, fldno, disponly);
    if (fldtype == FT_TBLFLD)
    {
	tbl = fld->fld_var.fltblfld;
	ocol = tbl->tfcurcol;
	orow = tbl->tfcurrow;
	tbl->tfcurcol = col;
	tbl->tfcurrow = row;
    }

    hdr = FDgethdr(fld);
    type = FDgettype(fld);
    val = FDgetval(fld);

    if (fldtype == FT_TBLFLD)
    {
	pflag = tbl->tffflags + (row * tbl->tfcols) + col;
	tbl->tfcurcol = ocol;
	tbl->tfcurrow = orow;
	flag = *pflag;
    }
    else
    {
	pflag = &(hdr->fhdflags);
	flag = *pflag;
    }

    ddbv = val->fvdsdbv;
    dbv = val->fvdbv;
    text = (DB_TEXT_STRING *) ddbv->db_data;

    if (hdr->fhtitle)
    {
	STcopy(hdr->fhtitle, IIFDftFldTitle);
    }
    else
    {
	IIFDftFldTitle[0] = EOS;
    }
    STcopy(hdr->fhdname, IIFDfinFldInternalName);
    if ((flag & fdI_CHG))
    {
	if (EXdeclare(FDadfxhdlr, &context) != OK)
	{
	    EXdelete();
	    return(FALSE);
	}

	/*
	**  Do check for mandatory field.
	**  First, have to convert value to see
	**  if a NULL value exists since NULL values
	**  may be displayed as the empty string.
	**  If value is not NULL, then do mandatory
	**  check by checking length of display buffer.
	**  Must convert into another DBV to avoid
	**  stepping on real field value.
	*/
	MEcopy((PTR) dbv, (u_i2) sizeof(DB_DATA_VALUE), (PTR) &mdbv);
	if (IIFDcdCvtData == NULL)
	{
	    if ((IIFDcdCvtData = MEreqmem((u_i4)0,
		(u_i4)DB_GW4_MAXSTRING + DB_CNTSIZE,
		TRUE, (STATUS *)NULL)) == NULL)
	    {
		IIUGbmaBadMemoryAllocation(ERx("IIFDvfValidateFld"));
		EXdelete();
		return(FALSE);
	    }
	}

	mdbv.db_data = IIFDcdCvtData;

	/*
	**  Data is currently checked only for its data type
	**  and for the validation cluase.  Eventually it will
	**  check against the edit checks specified according
	**  to a format.
	**  Do conversion from long text to internal value.
	*/

	if (hdr->fhd2flags & fdSCRLFD)
	    scrolling = TRUE;

	if (scrolling)
	{
	    /* create a fake, full scrolling-buf-width fmt struct */
	    IIFDssfSetScrollFmt(type->ftfmtstr, frm->frtag,
		(SCROLL_DATA *)val->fvdatawin, &fmtptr);
	}
	else
	{
	    fmtptr = type->ftfmt;
	}

	if (hdr->fhd2flags & fdUSEEMASK)
	{
		fmtflags |= INP_MASK_ACTIVE;
	}

	ladfcb = FEadfcb();

        /* b92987 - Set local adf cb with II_DECIMAL */
        if (ii_decimal == NULLCHAR)
        {
           /* Set up value of II_DECIMAL */
           NMgtAt(ERx("II_DECIMAL"), &cp);
           cp = STalloc(cp);
           if ( cp != NULL && *cp != EOS )
           {
              if (*(cp+1) != EOS || (*cp != '.' && *cp != ','))
              {
                 ii_decimal = ladfcb->adf_decimal.db_decimal;
              }
              else
              {
                 ii_decimal = cp[0];
              }
           }
           else
                ii_decimal = ladfcb->adf_decimal.db_decimal;
           MEfree(cp);
        }
        ladfcb->adf_decimal.db_decimal = ii_decimal;

	if (text->db_t_count == 0)
	{
	    _VOID_ adc_getempty(ladfcb, &mdbv);
	}
	else if (fcvt_sql(ladfcb, fmtptr, ddbv, &mdbv, (bool) FALSE, 0,
		fmtflags, &sqlpat) != OK)
	{
	    /*
	    **  Try to give error messages similar to 5.0.
	    */
	    if (abs(type->ftdatatype) == DB_INT_TYPE)
	    {
		error_id = E_FI21CB_8651;
	    }
	    else if (abs(type->ftdatatype) == DB_FLT_TYPE)
	    {
		error_id = E_FI21CC_8652;
	    }
	    else
	    {
		error_id = E_FI21CD_8653;
	    }
# ifdef FT3270
	    IIFTfbfFlagBadField(frm, val);
# endif
	    IIFDfdeFldDataError(error_id, 0);
	    EXdelete();
	    return(FALSE);
	}

	EXdelete();

	/*
	**  Since the DBVs are the same, just do a MEcopy.
	*/
	MEcopy(mdbv.db_data, (u_i2) mdbv.db_length, dbv->db_data);
    }

    if (!(flag & fdVALCHKED) || hdr->fhdflags & fdXREFVAL)
    {

	/*
	** No point checking for mandatory, or evaluating the validation
	** strings of derived fields--which have no such attributes.
	*/
	if (!(hdr->fhd2flags & fdDERIVED))
	{
	    if (adc_isnull(ladfcb, dbv, &isnull) != OK)
	    {
		return(FALSE);
	    }

	    if (hdr->fhdflags & fdMAND && (isnull || text->db_t_count == 0))
	    {
# ifdef FT3270
		IIFTfbfFlagBadField(frm, val);
# endif
		if (frm->frmflags & fdCLRMAND)
		{
		    IIFDfdeFldDataError(E_FI226E_8814_ClrMandFld, 0);
		}
		else
		{
		    IIFDfdeFldDataError(E_FI21CA_8650, 0);
		}
		return(FALSE);
	    }

	    /*
	    ** vl_evalfld() expects to be validating the 'current' row,
	    ** so oblige it.  Needed since aggregate processing can
	    ** validate any row.  (Wasn't needed for other types of
	    ** bulk validation since the traversal routines themselves
	    ** set and restored current column/row information.)
	    */
	    if (orow != -1)
		tbl->tfcurrow = row;

	    retval = vl_evalfld(type, val, frm);

	    if (orow != -1)
		tbl->tfcurrow = orow;

	    if (retval == FALSE)
		return (FALSE);
	}

	if (dispchg)	/* If display is to be changed. */
	{
	    /* Display is to be changed. */
	    retval = FDdatafmt(frm, fldno, disponly, fldtype, col, row, hdr,
		type, val);
	}
    }

    /* Now, if all went well, update the field flags */
    if (retval == TRUE)
    {
	*pflag &= ~fdI_CHG;
	*pflag |= fdVALCHKED;
    }

    return(retval);
}


/*{
** Name:	IIFDfdeFldDataError	- Fix fld data error msgs for IIFDerror
**
** Description:
**	Process field data error messages before calling IIFDerror.  Depending
**	on the contents of IIFDftFldTitle and IIFDfinFldInternalName, pass on
**	one of three possible args to IIFDerror.  Either 'current field',
**	'field (title: X, internal name: iX)', or 'field (internal name: iX)'.
**
** Inputs:
**	error_id	The error number.
**	parcount	Count of the number of parameters to display (0 or 1).
**	a1		A possible argument.
**
** Outputs:
**
**	Returns:
**		None
**
**	Exceptions:
**		None
**
** Side Effects:
**
** History:
**	03-jul-90 (bruceb)	Written.
*/
VOID
IIFDfdeFldDataError(error_id, parcount, a1)
ER_MSGID	error_id;
i4		parcount;
PTR		a1;
{
    char	*fld;
	/* Enough space for 'field (title: X, internal name: iX)' */
    char	buf[150];

    if (IIFDftFldTitle[0] != EOS)
    {
	fld = STprintf(buf, ERget(F_FD0003_title_int_name), IIFDftFldTitle,
	    IIFDfinFldInternalName);
    }
    else if (IIFDfinFldInternalName[0] != EOS)
    {
	fld = STprintf(buf, ERget(F_FD0004_int_name), IIFDfinFldInternalName);
    }
    else
    {
	fld = ERget(F_FD0002_current_field);
    }
    IIFDerror(error_id, (i4)(parcount + 1), (PTR)fld, a1);
}
