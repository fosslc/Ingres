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
** Name:	iifrsvars.c
**
** Description:
**
**	Public (extern) routines defined:
**		IIfrsvars()
**	Private (static) routines defined:
**
** History:
**/

/*{
** Name:	IIfrsvars	-	Initialize runtime variables
**
** Description:
**	Initialize the forms Runtime system variables data structure
**	nad set variable pointers to NULL.
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
**	
*/

IIfrsvars(frs)
register FRSDATA	*frs;
{
	frs->fdrunfld = NULL;		/* current field name */
	frs->fdrunno = NULL;		/* current field num */
	frs->fdrunlen = NULL;		/* length of field */
	frs->fdruntype = NULL;		/* type of field */
	frs->fdrunchg = NULL;		/* change in frame */
	frs->fdrunfmt = NULL;		/* format string of field */
	frs->fdrunval = NULL;		/* validation str of field */
}
