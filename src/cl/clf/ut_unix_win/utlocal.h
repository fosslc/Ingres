/*	SCCS ID = %W%  %G%	*/

/*
**Copyright (c) 2004 Ingres Corporation
**
**	Name:
**		UT.h -- UT declarations.
**
**	Function:
**		None
**
**	Arguments:
**		None
**
**	Result:
**		declares the UT module global UTstatus.
**		declares the UT module global UT_CO_DEBUG.
**		defines the debugging macro REPORT_ERR which calls UTerror
**		to print the error message associated with an error status
**		returned by a UT routine.
**
**	Side Effects:
**		None
**
**	History:
**		03/83 -- (gb)
**			written
**
**		12/84 (jhw) -- Added declaration of `UT_CO_DEBUG'.
*/


# include	<compat.h>	/* compatability library types */


extern		STATUS			UTstatus;


# define	REPORT_ERR(routine)	if ((UTstatus = routine) != OK) UTerror(UTstatus)

# ifdef UT_DEBUG
extern		bool			UT_CO_DEBUG;
# endif
