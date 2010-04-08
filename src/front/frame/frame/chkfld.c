/*
**	chkfld.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	chkfld.c - Check a field.
**
** Description:
**	Contains routines to validate a field.  Routines defined are:
**	- FDckffld - Do full field check.
**	- FDckqfld - Do query mode checking on field.
**
** History:
**	JEN - 19 Jan 1983  (written)
**	NCG -  3 Mar 1983  (Table field implementation added)
**	NCG - 12 Jan 1984  (Added Query mode for Table fields)
**	5-jan-1987 (peter)	Changed RTfnd* to FDfnd.
**	02/15/87 (dkh) - Added header.
**	19-jun-87 (bruceb)	Code cleanup.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	25-jul-89 (bruceb)
**		Set 'in_validation' while validation occurs.
**	02/23/91 (dkh) - Fixed bug 36017.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
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
# include	<frsctblk.h> 
# include	<frserrno.h>
# include       <er.h>
# include	"fdfuncs.h"


FUNC_EXTERN i4		FDgetdata();
FUNC_EXTERN i4		FDqrydata();
FUNC_EXTERN i4		FDnodata();
FUNC_EXTERN FRAME	*FDFTgetiofrm();
FUNC_EXTERN FIELD	*FDfndfld();
FUNC_EXTERN i4		FDgtfldno();
FUNC_EXTERN VOID	IIFDiaaInvalidAllAggs();

GLOBALREF	FRS_GLCB	*IIFDglcb;




/*{
** Name:	FDckffld - Check a field.
**
** Description:
**	This routine gets the contents of a field, and verifies it.
**	This is to provide a uniform interface for extracting data from
**	a frame.
**	Called when a " ## validate field " is issued and the form IS NOT in
**	Query mode.  If the field is a table field which is in Query
**	mode, then use FDqrydata() for that field. (ncg)
**
** Inputs:
**	frm	Form to find field in.
**	name	Name of field to check.
**
** Outputs:
**	Returns:
**		TRUE	If field passed.
**		FALSE	If field failed.
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
FDckffld(frm, name)		/* FDCKFFLD: */
FRAME	*frm;				/* frame to extract data from */
char	*name;				/* name of field to be examined */
{
	FIELD		*fd;
	REGFLD		*fld;		/* temporary field pointer */
	i4		fldno;		/* current field pointer */
	reg TBLFLD	*tfld;		/* table field to be checked */
	i4		(*routine)();
	i4		mode = FT_UPDATE;
	FRAME		*ofrm;
	i4		retval;
	bool		fldnonsq;

	fd = FDfndfld(frm, name, &fldnonsq);
	if (fd == NULL)
	{
		IIFDerror(GFFLNF, 1, name);
		return (FALSE);
	}

	if (fldnonsq)
		mode = FT_DISPONLY;
		
	if (fd->fltag == FREGULAR)
    	{
		IIFDglcb->in_validation |= VLD_FLD;

		fldno = FDgtfldno(mode, frm, fd, &fld);

		/* get the data from frame and verify */
		if (!FDgetdata(frm, fldno, mode, FT_REGFLD, (i4) 0, (i4) 0))
		{
			if (fldno >= 0)
				frm->frcurfld = fldno;
			retval = FALSE;
		}
		else
			retval = TRUE;

		IIFDglcb->in_validation &= ~VLD_FLD;
	}
	else		/* table field */
	{
		tfld = fd->fld_var.fltblfld;

		/*
		**  If table field is in read only mode then
		**  behave as if its OK.  Part of fix for
		**  BUGS 5199 & 4531. (dkh)
		*/
		if (tfld->tfhdr.fhdflags & fdtfREADONLY)
		{
			return(TRUE);
		}

		ofrm = FDFTgetiofrm();
		FDFTsetiofrm(frm);

		IIFDglcb->in_validation |= VLD_TFLD;

		/* validate table field until tfld->tflastrow */
		if (tfld->tfhdr.fhdflags & fdtfQUERY)
		{
			routine = FDqrydata;
		}
		else
		{
			IIFDiaaInvalidAllAggs(frm, tfld);
			if (tfld->tfhdr.fhdflags & fdtfAPPEND)
			{
				/*
				**  If table field is in append mode
				**  check if it is empty.  It is
				**  OK if it is empty.  Fix for BUG
				**  5199. (dkh)
				*/
				routine = FDnodata;
			}
			else
			{
				routine = FDgetdata;
			}
		}
		if (FDtbltrv(tfld, routine, FDDISPROW))
			retval = TRUE;
		else
			retval = FALSE;

		FDFTsetiofrm(ofrm);

		IIFDglcb->in_validation &= ~VLD_TFLD;
	}

	return(retval);
}


/*{
** Name:	FDckqfld - Check a field in query mode.
**
** Description:
**	Called when a " ## validate field " is issued and the form IS in
**	Query mode.  In all cases use FDqrydata(). (ncg)
**
** Inputs:
**	frm	Form to find field in.
**	name	Name of field to check.
**
** Outputs:
**	Returns:
**		TRUE	If field passed.
**		FALSE	If field failed.
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
FDckqfld(frm, name)		/* FDCKQFLD: */
FRAME	*frm;				/* frame to extract data from */
char	*name;				/* name of field to be examined */
{
	FIELD	*fd;
	REGFLD	*fld;		/* temporary field pointer */
	reg i4	fldno;		/* current field pointer */
	TBLFLD	*tfld;		/* table field to be checked */
	i4	mode = FT_UPDATE;
	FRAME	*ofrm;
	i4	retval;
	bool	fldnonsq;

	fd = FDfndfld(frm, name, &fldnonsq);
	if (fd == NULL)
	{
		IIFDerror(GFFLNF, 1, name);
		return (FALSE);
	}
	if (fldnonsq)
		mode = FT_DISPONLY;

	if (fd->fltag != FREGULAR)	/* table field */
    	{
		tfld = fd->fld_var.fltblfld;

		/*
		**  If table field not in query mode, then
		**  let FDckffld() do the dirty work becasuse
		**  the form is in query mode.
		**
		**  Note that we only get here because user has
		**  done some sort of validate field statement.
		**  The cover routine in runtime calls either
		**  FDckffld() or FDckqfld() depending on the
		**  mode of the form.  Since a table field can
		**  have its own mode, we need to do the following
		**  check to make sure that the table field is
		**  validated correctly.
		*/
		if (!(tfld->tfhdr.fhdflags & fdtfQUERY))
		{
			return(FDckffld(frm, name));
		}

		ofrm = FDFTgetiofrm();
		FDFTsetiofrm(frm);

		/* validate table field till tfld->tflastrow */
		if (FDtbltrv(tfld, FDqrydata, FDDISPROW))
		{
			retval = TRUE;
		}
		else
		{
			retval = FALSE;
		}

		FDFTsetiofrm(ofrm);
		return(retval);

	}
	fldno = FDgtfldno(mode, frm, fd, &fld);

	/* get the data from frame and verify */
	if (!FDqrydata(frm, fldno, mode, FT_REGFLD, (i4) 0, (i4) 0))
	{
		if (fldno >= 0)
			frm->frcurfld = fldno;
		return (FALSE);
	}
	else
		return (TRUE);
}
