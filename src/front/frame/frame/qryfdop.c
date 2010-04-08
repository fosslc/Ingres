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
# include       <er.h>
# include	"fdfuncs.h"

/*
**	qryfdop.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	qryfdop.c - Check field for query value.
**
** Description:
**	Contains Utility routines to check field for a query value.
**	Routines defined:
**	- FDqryfdop - Get regular field pointer for querying.
**
** History:
**	26 feb 1985 - extracted from qryop.c
**	5-jan-1987 (peter)	Changed RTfnd* to FDfnd.
**	02/17/87 (dkh) - Added header.
**	25-jun-87 (bab)	Code cleanup.
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

/*
**	QRYFDOP.c  -  Get field pointer.
**		called by FDqryfld() and FDqryop().
**	
**	Arguments:  frm		- frame to extract data from
**		    name	- name of the field
**	
**	Returns:  NULL -  error occurred
**		  pointer to field.
**	
**	History:  26 feb 1985 - extracted from qryop.c
**		5-jan-1987 (peter)	Changed RTfnd* to FDfnd.
*/

FUNC_EXTERN	FIELD	*FDfndfld();
FUNC_EXTERN	i4	FDgtfldno();



/*{
** Name:	FDqryfdop - Get regular field pointer for querying.
**
** Description:
**	Given a form and the name of a regular field, find the
**	field in the form and check it for valid query data.
**	Return the rEGFLD pointer for the field if valid data
**	was found.  Calls FDqrydata() to do the checking.
**
** Inputs:
**	frm	Form containing field.
**	name	Name of regular field to look for.
**
** Outputs:
**	Returns:
**		ptr	REGFLD pointer for the field.  NULL if field
**			does not exist or bad data found.
**	Exceptions:
**		None.
**
** Side Effects:
**	Query operator for field also gets set.
**
** History:
**	02/17/87 (dkh) - Added procedure header.
*/
REGFLD	*
FDqryfdop(frm, name)
FRAME	*frm;			/* frame to work with */
char	*name;			/* name of field to be examined */
{
	reg FIELD	*fd;		/* temporary field pointer */
    	REGFLD		*fld;
	reg i4		mode = FT_UPDATE;
	reg i4		fldno;
	bool		fldnonsq;

	fd = FDfndfld(frm, name, &fldnonsq);
	if (fd == NULL)
	{
		IIFDerror(GFFLNF, 1, name);
		return ((REGFLD *) NULL);
	}
	if (fldnonsq)
		mode = FT_DISPONLY;

	if (fd->fltag != FREGULAR)
	{
		IIFDerror(GFFLRG, 1, name);
		return ((REGFLD *) NULL);
	}

	fldno = FDgtfldno(mode, frm, fd, &fld);

	if (FDqrydata(frm, fldno, mode, FT_REGFLD, (i4) 0, (i4) 0) == FALSE)
		return ((REGFLD *) NULL);

	return (fld);
}
