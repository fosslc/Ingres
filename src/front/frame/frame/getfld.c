/*
**	getfld.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	getfld.c - Get value of a regular field.
**
** Description:
**	Get value of a regular field.  Routine defined here is:
**	- FDgetfld - Get value of a regular field.
**	- FDstorput - Needs to change for 6.0.
**
** History:
**	02/15/87 (dkh) - Added header.
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
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
# include	<frserrno.h>
# include	"fdfuncs.h"
# include	<si.h>
# include       <er.h>



FUNC_EXTERN	FIELD	*FDfndfld();
FUNC_EXTERN	i4	FDgtfldno();



/*{
** Name:	FDgetfld - Get value of a regular field.
**
** Description:
**	This routine gets the contents of a field, varifies it, converts it,
**	and passes it on to the upper level program calling this routine.
**	This is to provide a uniform interface for extracting data from
**	a frame.  The calling routine matches the calling routine of its
**	opposite, putfld().
**
**	To use this routine the user justs passes a pointer to the variable
**	he wishes to fill with this data, the data type of the variable, and
**	the length in bytes of the variable.  For example:
**
**		f4  number1;
**		getfield(frm1, "number1", &number1, DB_FLT_TYPE, 4);
**	or
**		char	name[20];
**		getfield(frm, "lastname", name, DB_CHR_TYPE, 20);
**
** Inputs:
**	frm		Form to extract data from.
**	name		Name of the field.
**
** Outputs:
**	pdbv	Pointer to a DB_DATA_VALUE;
**
**	Returns:
**		TRUE	Variable passed ok.
**		FALSE	Error in passing variable.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	JEN - 1 Nov 1981  (written)
**	5-jan-1987 (peter)	Change RTfnd* to FDfnd.
**	02/15/87 (dkh) - Added procedure header.
*/
i4
FDgetfld(frm, name, pdbv)	/* FDGETFLD: */
FRAME		*frm;		/* frame to extract data from */
char		*name;		/* name of field to be examined */
DB_DATA_VALUE    **pdbv;	/* union of legal C types */
{
	FIELD	*fd;
	REGFLD	*fld;		/* temporary field pointer */
	i4	mode = FT_UPDATE;
	i4	fldno;
	bool	fldnonsq;

	fd = FDfndfld(frm, name, &fldnonsq);
	if (fd == NULL)
	{
		IIFDerror(GFFLNF, 1, name);
		return (FALSE);
	}
	if (fldnonsq)
		mode = FT_DISPONLY;

	if (fd->fltag != FREGULAR)
    	{
    		IIFDerror(GFFLRG, 1, name);
		return (FALSE);
	}

	fldno = FDgtfldno(mode, frm, fd, &fld);

	if (FDgetdata(frm, fldno, mode, FT_REGFLD, (i4) 0, (i4) 0) == FALSE)
		return (FALSE);

	*pdbv = ((&(fld->flval))->fvdbv);
	return(TRUE);
}
