/*
 *	Copyright (c) 1983 Ingres Corporation
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
 */

# include	  <compat.h>  
#include    <gl.h>

globaldef STATUS		PCstatus;

globaldef i4			PCeventflg = 50;	/* unlikely to-be-used event flag for PCspawn/PCwait */
