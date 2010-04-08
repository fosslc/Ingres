/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<cv.h>
#include	<ex.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<adf.h>
#include	<afe.h>

/**
** Name:    ugitoa.c -	Front-End Internal ASCII to Int Conversion Module.
**
** Description:
**	Routine to do conversions between char strings and i2 and i4 data types.
**	At present this is a cover for a call to CV.
**	This could very well change to call the ADF coersion routines. 
**
** Defines:
**	IIUGatoi()
**
** History:
**  2-oct-1987 (danielt)
**	written 
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*{
** Name:    IIUGitoa() -
**
** Description:
**	Convert ascii to integer.  
**
** Inputs:
**	int_str	    string to be converted
**	int_ptr	    address of result integer.
**	length	    byte length of result integer 
**
** Outputs:
**	int_ptr	    points at result integer.
**
** History:
**	2-nov-1987 (danielt)
**	    	written
*/
VOID
IIUGatoi(int_str, int_ptr, length)
char	*int_str;
i4	*int_ptr;
i4	length;
{
	i4	tmp_int;

	CVal(int_str, int_ptr);
	return;
}
