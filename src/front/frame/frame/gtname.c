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

/*
**	GETNAME.c  -  Return the name of the specified field.
**
**	History:  05 Dec 1984 - extracted from parse.c
**	03/05/87 (dkh) - Added support for ADTs.
**
**	static	char	Sccsid[] = "%W%	%G%";
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-May-2003 (bonro01)
**	    Add prototype to prevent SEGV on HP Itanium (i64_hpu)
*/

FUNC_EXTERN	char	*FDGFName(FIELD *fd);


char *
FDgetname(frm, fldno)
FRAME	*frm;
i4	fldno;
{
	FIELD	*fld;

	fld = frm->frfld[fldno];
	return (FDGFName(fld));
}
