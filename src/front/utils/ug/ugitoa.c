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
** Name:    ugitoa.c -	Front-End Internal Int to ASCII Conversion Module.
**
** Description:
**	Routine to do conversions between i2 and i4 data types to
**	char strings.  At present this is a cover for a call to CV.
**	This could very well change to call the ADF coersion routines. 
**
** Defines:
**	IIUGitoa()
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
**	Convert integer to ascii.  Assumes that the string buffer passed in is
**	big enough to hold result.
**
** Inputs:
**	conv_int    int to be converted
**	int_str	    address of output string
**	length	    byte length of integer to be converted (e.g. 2 
**						    for i2, 4 for i4)
**
** Outputs:
**	int_str	    filled in
**
** History:
**	2-nov-1987 (danielt)
**	    	written
*/
VOID
IIUGitoa(conv_int, int_str, length)
i4	conv_int;
char	*int_str;
i4	length;
{
	CVla((i4) conv_int, int_str);
	return;
}
