/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<adf.h>

/**
** Name:	affnamck.c -	Front-End ADF Function Name Check Routine.
**
** Description:
**	Contains a routine that checks whether a name is an ADF function.
**	Defines:
**
**	IIAFckFuncName()	check for ADF function operator.
**
** History:
**	Revision 6.2  89/02  wong
**	Initial revision from 'oschkfunc()' and 'absnamchk()' in
**	"abf!osl!oscktype.c" and "abf!abf!abret.c".
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-Aug-2009 (kschendel) 121804
**	    Update the function declarations to fix gcc 4.3 problems.
**/

/*{
** Name:	IIAFckFuncName() -	Check For ADF Function Operator.
**
** Description:
**	Determines whether the input function name is an ADF function operator.
**
** Input:
**	fct_name	{char *}  Function name.
**
** Returns:
**	{bool}  TRUE if ADF function operator.
**
** History:
**	02/87 (jhw) -- Written.
*/

bool
IIAFckFuncName ( char *fct_name )
{
	i4	type;

	ADF_CB	*FEadfcb();

	return (bool) (afe_fitype(FEadfcb(), fct_name, &type) == OK &&
			type == ADI_NORM_FUNC) ;
}
