/*
**Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include	<cv.h>
#include	<cm.h>
#include	<st.h>

/**
** Name: CVRXL.C - Functions used to convert radix-64 to long
**
** Description:
**      This file contains the following external routines:
**    
**      CVrxl()             -  convert radix-64 to long.
**
** History:
**	14-feb-00 (loera01) bug 100276
**	    Created.
**      27-jun-00 (loera01)
**	    Added missing include for st.h.
**/

# define RADIX_64_STRLEN 6


/*{
** Name:	CVal - CHARACTER STRING TO 32-BIT INTEGER
**		       CONVERSION
**
** Description:
**    BASE 64 (RADIX) ASCII CHARACTER STRING TO 32-BIT INTEGER CONVERSION
**
**    Convert a base-64 character string pointed to by `a' to an
**    i4 pointed to by `i'. 
**
**    Base-64 ASCII is a notation by which long integers can be
**    represented by up to six characters; each character represents a
**    "digit" in a radix-64 notation.
**
**    The characters used to represent "digits" are . for 0, / for 1, 0
**    through 9 for 2-11, A through Z for 12-37, and a through z for 38-63.
**
**    The leftmost character is the least significant digit.  For example,
**                   a0 = (38 x 64^0) + (2 x 64^1) = 166
** Inputs:
**	a	char pointer, contains string to be converted.
**
** Outputs:
**	i	i4 pointer, will contain the converted long value
**
**   Returns:
**	OK:		succesful conversion; `i' contains the
**			integer
**	CV_OVERFLOW:	numeric overflow; `i' is unchanged
**	CV_UNDERFLOW:	numeric underflow; 'i' is unchanged
**	CV_SYNTAX:	syntax error; `i' is unchanged
**	
** History:
**	18-Jan-00 loera01
**	    Created.
*/
STATUS
CVrxl(register char *a, i4 *i)
{
    STATUS ret = OK;

    if (!CMprint(a))
    {
	return (CV_SYNTAX);
    }
    if (STlength(a) > RADIX_64_STRLEN)
    {
	ret = CV_OVERFLOW;
    }
    *i = a64l(a);	
    return (ret);
}
