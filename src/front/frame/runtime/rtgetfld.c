/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
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
# include	<menu.h>
# include	<runtime.h> 

/**
** Name:	rtgetfld.c
**
** Description:
**
**	Public (extern) routines defined:
**		RTgetfld()
**	Private (static) routines defined:
**
** History:
**/

FUNC_EXTERN	char	*FDfldnm();

/*{
** Name:	RTgetfld	-	Get a field descriptor
**
** Description:
**	Given a frame and a field name, find the field descriptor
**
** Inputs:
**	stkf		Ptr to the stack frame
**	nm		Name of the field to find
**
** Outputs:
**
** Returns:
**	Ptr to the field
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	21-jul-1983  -  written (jen)
**	05-jan-1987 (peter)	Changed RTfndfld to FDfndfld.
**	18-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	
*/

FIELD	*
RTgetfld(stkf, nm)
reg	RUNFRM	*stkf;
reg	char	*nm;
{
	reg	FRAME	*frm;
	FIELD		*FDfndfld();
	bool		junk;

	frm = stkf->fdrunfrm;
	if (nm == NULL || *nm == EOS)
	{
		nm = FDfldnm(frm);
	}

	return (FDfndfld(frm, nm, &junk));
}
