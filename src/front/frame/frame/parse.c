# include	<compat.h>
# include	<er.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<frserrno.h>
# include	<ex.h>
# include	"fdfuncs.h"
# include	<ug.h>


/*
NO_OPTIM = i64_hpu
*/
/*
**	parse.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	parse.c - Parse validation strings for fields.
**
** Description:
**	Parse the validation strings of a field return TRUE on
**	a fatal error.
**	Routines defined:
**	- FDparse - Parse validation strings in a form.
**	- FDprstbl - Parse a table field.
**	- FDprsreg - Parse a regular field.
**	- FDprsfld - Do the actual parsing.
**
** History:
**	17 Oct 1983 - extracted from frcreate.qc
**	(dkh)	  13 Sep 1985 - Fixed to clear ftvalchk if ftvalstr is blank.
**	02/17/87 (dkh) - Added header.
**	25-jun-87 (bruceb)	Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	03/16/88 (dkh) - Added EXdelete call to conform to coding spec.
**	06/18/88 (dkh) - Integrated CMS changes.
**	10/21/88 (dkh) - Added check to "commit" if not in a transaction.
**	12/01/88 (dkh) - Fixed cross field validation problem.
**	06/22/89 (dkh) - Put in support for derived fields.
**	09/23/89 (dkh) - Porting changes integration.
**	12/23/89 (dkh) - VMS shared lib changes.
**	02/02/90 (dkh) - Put in special hooks to allow Vifred to edit a
**			 form even though it may have validation syntax
**			 errors.
**	10-may-90 (bruceb)
**		If derivation parsing fails, and IIFDeoErrsOK, still don't
**		perform circularity checking; structures used by that
**		function may not have been created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	14-May-2003 (bonro01)
**	    NO_OPTIM required for i64_hpu to fix abend in frs27.sep
**	    Add prototype to prevent SEGV
**	24-Aug-2009 (kschendel) 121804
**	    Need ug.h to satisfy gcc 4.3.
**/


FUNC_EXTERN	VOID	IIFVdsmDrvSetMsg();
FUNC_EXTERN	VOID	IIFVcpConstProp();
FUNC_EXTERN	STATUS	vl_except();
GLOBALREF	i4	vl_help;
GLOBALREF	bool	vl_syntax;
GLOBALREF	i4	FDparscol;
GLOBALREF	bool	IIFVxref;

FUNC_EXTERN	char	*FDgetname(FRAME *frm, i4 fldno);

/*
**  Boolean indicating if a table field lookup
**  validation was parse.
*/
static	bool	prsgetlist = FALSE;

static	i4	IIFDeoErrsOK = FALSE;

/*
**  Boolean indicating if derivation formulas were found.
*/
static	bool	drv_found = FALSE;	

/*
**  Boolean indicating if we have just parsed a derivation
**  formula.
*/
static	bool	doing_drv = FALSE;

/*
**  Boolean indicating that an error occurred when parsing
**  one of the derivation formulas.
*/
static	bool	drv_error = FALSE;

/*{
** Name:	FDparse - Parse validation string for entire form.
**
** Description:
**	Parse the validation string for each field in a form.  If
**	"vl_syntax" is FALSE, the also build the validation tree for
**	each field.  For table fields, parse each column instead.
**	Only parse updateable fields.
**
**	Updated to call the derivation parser if the field is
**	a derived field.
**
** Inputs:
**	frm	Form to parse.
**
** Outputs:
**	Returns:
**		TRUE	If bad validation syntax found.
**		FALSE	If all validation strings are correct.
**	Exceptions:
**		None.
**
** Side Effects:
**	A validation tree (used at runtime execution) is also built for
**	each field if "vl_syntax" is FALSE.
**
** History:
**	02/17/87 (dkh) - Added procedure header.
**	06/22/89 (dkh) - Added support for parsing derived fields.
*/
i4
FDparse(frm)
FRAME	*frm;
{
	i4		i;
	i4		j;
	FIELD		**fd;
	FIELD		*afld;
	FLDCOL		**col;
	FLDCOL		*acol;
	TBLFLD		*tbl;
	STATUS		exstat;
	EX_CONTEXT	context;
	char		errbuf[ER_MAX_LEN + 1];
	i4		errnum;

	/*
	**  Temporary fix to clear out fhdrv pointers before parsing.
	**  This is necessary since the old pointer values are
	**  still around when the form was saved into the DB by Vifred.
	*/
	for (i = 0, fd = frm->frfld; i < frm->frfldno; i++, fd++)
	{
		if ((*fd)->fltag == FREGULAR)
		{
			(*fd)->fld_var.flregfld->flhdr.fhdrv = NULL;
		}
		else
		{
			tbl = (*fd)->fld_var.fltblfld;
			for (j = 0, col = tbl->tfflds; j < tbl->tfcols;
				j++, col++)
			{
				(*col)->flhdr.fhdrv = NULL;
			}
		}
	}

	prsgetlist = FALSE;
	drv_found = FALSE;
	drv_error = FALSE;

	vl_help = TRUE;
	/* for each field, parse the validation checks */
	for(i = 0, fd = frm->frfld; i < frm->frfldno; i++, fd++)
	{
		exstat = EXdeclare(vl_except, &context);
		if (exstat != OK)
		{
			/* This code does not follow the CL spec */
			if (exstat != EXDECLARE)
			{
				EXdelete();
				IIFDerror(XCPTNOSET, 0, (char *) NULL);
				PCexit(-1);
			}
		}
		if (vl_help == TRUE)		/* set escape jump */
		{
			if ((*fd)->fltag == FTABLE)
				FDprstbl(frm, *fd, (*fd)->fld_var.fltblfld);
			else
				FDprsreg(frm, *fd, (*fd)->fld_var.flregfld);
			EXdelete();
		}
		else
		{
			EXdelete();
			/*
			**  We are here because the validation string
			**  didn't parse correctly.  But we will go
			**  on if we are just checking syntax and
			**  that we've been told to allow errors (for
			**  now, its just Vifred).
			*/
			if (vl_syntax && IIFDeoErrsOK)
			{
				vl_help = TRUE;
				if (doing_drv)
				{
					drv_error = TRUE;
				}
			}
			else
			{
				if (doing_drv)
				{
					errnum = E_FI1F4A_8010;
				}
				else
				{
					errnum = CFFLVAL;
				}
				IIFDerror(errnum, 2, frm->frname,
					FDgetname(frm, i));
				frm->frcurfld = i;
				return(TRUE);
			}
		}
	}

	/*
	**  Do special constant propgataion checking and
	**  circularity checking if any derivation formulas
	**  have been parsed--but not if any errors occurred.
	*/
	if (drv_found && !drv_error)
	{
		/*
		**  Do constant propagation checking.
		*/
		if (!vl_syntax)
		{
			IIFVcpConstProp(frm);
		}
		/*
		**  Need to do circularity checking.
		*/
		if (IIFVdccDrvCircChk(frm) != OK)
		{
			/*
			**  Circular reference in derivation
			**  formula found.  Put out error.
			*/
			afld = frm->frfld[frm->frcurfld];
			if (afld->fltag == FREGULAR)
			{
				IIUGfmt(errbuf, ER_MAX_LEN,
					ERget(E_FI20D0_8400), 2, frm->frname,
					afld->fld_var.flregfld->flhdr.fhdname);
				IIFVdsmDrvSetMsg(errbuf);
				IIFDerror(E_FI20D0_8400, 2, frm->frname,
					afld->fld_var.flregfld->flhdr.fhdname);
			}
			else
			{
				tbl = afld->fld_var.fltblfld;
				acol = tbl->tfflds[tbl->tfcurcol];
				IIUGfmt(errbuf, ER_MAX_LEN,
					ERget(E_FI20D1_8401), 3, frm->frname,
					acol->flhdr.fhdname,
					tbl->tfhdr.fhdname);
				IIFVdsmDrvSetMsg(errbuf);
				IIFDerror(E_FI20D1_8401, 3, frm->frname,
					acol->flhdr.fhdname,
					tbl->tfhdr.fhdname);
			}
			return(TRUE);
		}
	}
	return(FALSE);
}

FDprstbl(frm, fld, tbl)
FRAME	*frm;
FIELD	*fld;
TBLFLD	*tbl;
{
	reg FLDCOL	**col;
	i4		j;

	for (j = 0, col = tbl->tfflds; j < tbl->tfcols; j++, col++)
	{
		FDparscol = tbl->tfcurcol = j;
		FDprsfld(frm, fld, &((*col)->flhdr), &((*col)->fltype));
	}
	tbl->tfcurcol = 0;
}

FDprsreg(frm, ffld, fld)
FRAME	*frm;
FIELD	*ffld;
REGFLD	*fld;
{
	FDprsfld(frm, ffld, &fld->flhdr, &fld->fltype);
}

FDprsfld(frm, fld, hdr, type)
FRAME	*frm;
FIELD	*fld;
FLDHDR	*hdr;
FLDTYPE *type;
{
	doing_drv = FALSE;
	if (type->ftvalstr != NULL && STcompare(type->ftvalstr, ERx("")) != 0)
	{
		if (hdr->fhd2flags & fdDERIVED)
		{
			drv_found = TRUE;
			doing_drv = TRUE;
			/*
			**  Initialize for parsing
			**  a derivation string.
			*/
			IIFVdinit(frm, fld, hdr, type);

			/*
			**  Parse the derivation string.
			*/
			IIFVdpParse();
		}
		else
		{
			/*
			**  Initialize for parsing
			**  a validation string.
			*/
			IIFVvinit(frm, hdr, type);

			/*
			**  Parse the validation string.
			*/
			IIFVvpParse();

			if (!vl_syntax && IIFVxref)
			{
				hdr->fhdflags |= fdXREFVAL;
			}
		}
	}
	else
	{
		/*
		**  Clear out the validation tree in case a field
		**  previously had a validation string but the user removed it.
		**  Don't need to it in parse2.c since that should be only
		**  for runtime resolution.
		*/
		type->ftvalchk = NULL;
		hdr->fhdflags &= ~fdXREFVAL;
	}
}


/*{
** Name:	IIFDsgvSetGetlistVar - Set var "prsgetlist" to passed in value.
**
** Description:
**	All this routine does is to set the static variable "prsgetlist"
**	to the passed in value.  This routine is currently ONLY called
**	by routine vl_getlist() in the valid directory.  Variable
**	"prsgetlist" stores the fact that a table lookup validation
**	has been parsed.
**
** Inputs:
**	val	Value to set "prsgetlist" to.
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
**	10/22/88 (dkh) - Initial version.
*/
VOID
IIFDsgvSetGetlistVar(val)
bool	val;
{
	prsgetlist = val;
}




/*{
** Name:	IIFDggvGetGetlistVar - Get the value of variable "prsgetlist".
**
** Description:
**	This routine simply returns the value contained in variable
**	"prsgetlist" to the calling routine.  Currently known
**	clinets are FDcrfrm() and IIFDfrcrfle().
**
** Inputs:
**	None.
**
** Outputs:
**	val	Value contained in prsgetlist.
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
**	10/22/88 (dkh) - Initial version.
*/
bool
IIFDggvGetGetlistVar()
{
	return(prsgetlist);
}



/*{
** Name:	IIFDieIgnoreErrs - Routine to set IIFDeoErrsOK.
**
** Description:
**	This is just a cover routine to set the variable IIFDieIgnoreErrs.
**	The variable will control whether datatype mismatch in validation
**	strings is fatal or not.  But this is has affect only if vl_syntax
**	is TRUE.  This should allow folks to edit forms with syntax
**	errors.  But they still can't save them.
**
** Inputs:
**	val	Value to set IIFDeoErrsOK to.
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
**	02/02/90 (dkh) - Initial version.
*/
VOID
IIFDieIgnoreErrs(val)
i4	val;
{
	IIFDeoErrsOK = val;
}
