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
** Name:	iifindfrm.c
**
** Description:
**	Find a frame in the active list
**
**	Public (extern) routines defined:
**		IIfindfrm()
**
**	Private (static) routines defined:
**
** History:
**		16-feb-1983  -  Extracted from original runtime.c (jen)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Aug-2009 (kschendel) 121804
**	    Update the function declarations to fix gcc 4.3 problems.
**/


/*{
** Name:	IIfindfrm	-	Find a frame in the active list
**
** Description:
**	Look for the named frame in the active list, and return a 
**	pointer to it if found.  Basically a cover routine for a call 
**	to RTfindfrm.
**
** Inputs:
**	nm		Name of the frame to find
**
** Outputs:
**
** Returns:
**	Pointer to the frame if found, or NULL if not found.
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	
*/

FRAME *
IIfindfrm(char *nm)
{
	RUNFRM	*runf;
	reg	char	*name;
	char		fbuf[MAXFRSNAME+1];

	/*
	**	Check all variables make sure they are valid.
	**	Also check to see that the forms system has been
	**	initialized.
	*/
	if (nm == NULL)
		return (FALSE);
	name = IIstrconv(II_CONV, nm, fbuf, (i4)MAXFRSNAME);

	/*
	**	Look through the linked list of frames until
	**	the frame name is found.
	*/
	runf = RTfindfrm(name);
	if (runf == NULL)
		return (NULL);
	else
		return (runf->fdrunfrm);
}
