/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>

/**
** Name:    ugfrs.c	- Setup routines for use in utilities
**
** Description:
**	This file defines:
**
**	iiugfrs_setting()
**
**	Globals:
**
**	IIugmfa_msg_function
**	IIugefa_err_function
**
** History:
**	5-apr-1987 (peter)	Taken from the FEforms.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
** Name:    IIugmfa_msg_function -	FE message routine pointer
**
** Description:
**	This points to the address of the routine used in printing
**	out front end messages when the forms system is active.
**	If it is NULL, output goes to standard output.
**	This is used in FEmsg and FEcopyright.
*/

GLOBALREF i4		(*IIugmfa_msg_function)();

/*
** Name:    IIugefa_err_function -	FE error msg routine pointer
**
** Description:
**	This points to the address of the routine used in printing
**	out front end error messages when the forms system is active.
**	If it is NULL, output goes to standard output.
**	This is used by iiugerr.
*/

GLOBALREF DB_STATUS	(*IIugefa_err_function)();

/*{
** Name:    iiugfrs_setting() -	Setup routines for use in utilities
**
** Description:
**	This routine sets up pointers to routine names to perform
**	utility functions.  The need for this is caused by the 
**	fact that the forms system is large, and some of the 
**	frontends (such as the report writer) do not need to use
**	it.  However, it is useful to have a single set of utilies
**	routines that can be used in all frontends.  In order to do
**	this, the iiugfrs_setting routine is called whenever a frontend
**	changes into or out of forms mode, telling whether going
**	in or out of forms mode.  This then sets pointers to
**	routines in the forms system to do the utility functions.
**	When the utility functions are called, they call the
**	forms system function if the address is set, else do a
**	non-forms mode of the function.
**
**	The primary utilities using this function (as of this
**	writing) are FEmsg, FEcopyright, and iiugerr (the error
**	routine).
**
** Inputs:
**	msg_function	Address of the routine to call by FEmsg
**			and FEcopyright.
**	err_function	Address of the routine to call by iiugerr
**
** Side Effects:
**	Sets global pointers IIugmfa_msg_function and IIugefa_err_function.
**
** History:
**	5-apr-1987 (peter)	Written.
*/

VOID 
iiugfrs_setting(msg_function, err_function)
i4	(*msg_function)();
i4	(*err_function)();
{
    IIugmfa_msg_function = msg_function;
    IIugefa_err_function = err_function;
}
