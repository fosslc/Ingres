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
** fldname.c
**
** DEFINES
**	FDfldname()	returns the name of the current field.
**
** Sccsid[] = "%W%	%G%";
**
** Copyright (c) 2004 Ingres Corporation
**
** History:
**	03/05/87 (dkh) - Added support for ADTs.
*/

FUNC_EXTERN	char	*FDGFName();


char *
FDfldname(frm)
FRAME	*frm;
{
	FIELD	*fld;

	fld = frm->frfld[frm->frcurfld];
	return (FDGFName(fld));
}
