/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>

/**
** Name:	iigetoper.c
**
** Description:
**	Defines the function calls of getoper and putoper
**
**	Public (extern) routines defined:
**		IIgetoper()
**		IIsetoper()
**	Private (static) routines defined:
**
** History:
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

/*{
** Name:	IIgetoper() -	Return the getoper flag
**
** Description:
**	This routine is used to manipulate a static flag that
**	indicates whether or not the current 'get' requests the
**	'oper' for a field.
**
**	When one of the EQUEL 'get' statements recognizes the getoper
**	function, it will generate a call to this routine with set=1
**	to indicate to the subsequent routine that returns a value that
**	it should return the oper for the field or column, not its
**	value.  That routine will also call this routine with set=0,
**	inquiring about the state of the flag, and resetting it to 0.
**
**	This routine is part of RUNTIME's external interface.
**
** Inputs:
**	set	{nat}	Flag - 1 to set flag that next 'get' is a getoper
**			       0 to test if current 'get' is getoper, and
**				 reset local flag.
**
** Returns:
**	{nat}	0 if this is a 'set' request, or in an inquiry if the
**		  current 'get' is not a 'getoper' request.
**		1 if this is an inquiry, and this is a getoper.
**
** Example and Code Generation:
**	## getform f1 (i4var = getoper(f2))
**
**	if (IIfsetio("f1") != 0) {
**	    IIgetoper(TRUE);
**	    IIretfield(ISREF, DB_INT_TYPE, 4, &i4var, "f2");
**	}
**
** Side Effects:
**	Saves the state of the 'putop_flag' as input by 'set'.
**
** History:
*/

static bool	getop_flag = FALSE;

i4
IIgetoper( i4 set )
{
	bool	  old;

	if ( set )
	{
		getop_flag = TRUE;
		return (i4)FALSE;
	}
	old = getop_flag;
	getop_flag = FALSE;
	return (i4)old;
}

/*{
** Name:	IIputoper() -	function putoper call
**
** Description:
**	Operates the same way that IIgetoper does for the putoper function,
**	using a different static flag to track putoper state.
**
**	This routine is part of RUNTIME's external interface.
**
** Inputs:
**	set	{nat}	Flag - 1 to set flag that next 'put' is a putoper
**			       0 to test if current 'put' is putoper, and
**				 reset local flag.
**
** Returns:
**	{nat}	0 if this is a 'set' request, or in an inquiry if the
**		  current 'put' is not a 'putoper' request.
**		1 if this is an inquiry, and this is a putoper.
**
** Example and Code Generation:
**	## putform f1 (putoper(f2) = i4var)
**
**	if (IIfsetio("f1") != 0) {
**	    IIputoper(TRUE);
**	    IIsetfield("f2", ISVAL, DB_INT_TYPE, 4, i4var);
**	}
**
**
** Side Effects:
**	Saves the the state of the 'putop_flag' as input by 'set'.
**
** History:
*/

static bool	putop_flag = FALSE;

i4
IIputoper( i4 set )
{
	bool	  old;

	if ( set )
	{
		putop_flag = TRUE;
		return (i4)FALSE;
	}
	old = putop_flag;
	putop_flag = FALSE;
	return (i4)old;
}
