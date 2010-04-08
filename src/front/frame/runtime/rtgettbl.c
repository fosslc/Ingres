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
** Name:	rtgettbl.c
**
** Description:
**
**	Public (extern) routines defined:
**		RTgettbl()
**	Private (static) routines defined:
**
** History:
**/

FUNC_EXTERN	char	*FDfldnm();

/*{
** Name:	RTgettbl	-	Get table field descriptor
**
** Description:
**	Provided with a frame pointer, and the name of the table
**	field to find, calls IItfind and returns a pointer to the 
**	tablefield descriptor.
**
** Inputs:
**	stkf		Ptr to the frame
**	nm		Name of the table field to find
**
** Outputs:
**
** Returns:
**	Ptr to the TBSTRUCT descriptor for the tablefield
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**		21-jul-1983  -  written (jen)
**	18-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	24-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**	
*/

TBSTRUCT *
RTgettbl(stkf, nm)
reg	RUNFRM	*stkf;
reg	char	*nm;
{
	char		*FDfldnm();

	if (nm == NULL || *nm == EOS)
	{
		nm = FDfldnm(stkf->fdrunfrm);
	}

	return (IItfind(stkf, nm));
}
