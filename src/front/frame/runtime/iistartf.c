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
** Name:	iistartf.c
**
** Description:
**
**	Public (extern) routines defined:
**		IIstartfrm()
**	Private (static) routines defined:
**
** History:
**/

/*{
** Name:	IIstartfrm	-	Start the current frame
**
** Description:
**	This routine sets the appropriate modes for the frame driver.
**	It probably isn't really necessary and its function transfered
**	to IIrunfrm().  It is here for historical reasons and it is
**	referenced in the preprocessor.
**
** Inputs:
**
** Outputs:
**
** Returns:
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	22-feb-1983  -  Extracted from original runtime.c (jen)
**	11-jun-1984  -  Mode this a NOOP as is unused. (ncg)
**	
*/

IIstartfrm()
{
	return (TRUE);
}
