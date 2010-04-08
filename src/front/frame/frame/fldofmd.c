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
**	FLDOFMD.c  -  Get fld pointer from a frame, field num, and mode.
**	
**	History:  bruceb - 20 Feb 1985 (swiped from a dozen different routines)
**	03/05/87 (dkh) - Added support for ADTs.
**	12/05/88 (dkh) - Performance changes.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


FIELD *
FDfldofmd(frm, fldno, disponly)
FRAME	*frm;
i4	fldno;
i4	disponly;
{
	FIELD	*fld;

	if (disponly == FT_UPDATE)
	{
		fld = frm->frfld[fldno];
	}
	else
	{
		fld = frm->frnsfld[fldno];
	}

	return (fld);
}
