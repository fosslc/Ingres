/*
**Copyright (c) 2004 Ingres Corporation
 *
 *	Name:
 *		PC.c
 *
 *	Function:
 *		None
 *
 *	Arguments:
 *		None
 *
 *	Result:
 *		define and initialize PC module's global variables.
 *			PCstatus -- stores current STATUS of PC routines
 *
 *	Side Effects:
 *		None
 *
 *	History:
 *		03/83	--	(gb)
 *			written
 *
 *
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
 */

/* static char	*Sccsid = "@(#)PC.c	3.1  9/30/83"; */


# include	<compat.h>
# include	<gl.h>

STATUS		PCstatus = 0;
