/*
**Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include	<cv.h>

/**
** Name: CVAN.C - ascii character string to natual integer conversion.
**
** Description:
**      This file contains the following external routines:
**    
**      CVan()        - ascii character string to natual integer conversion.
**
** History:
 * Revision 1.1  90/03/09  09:14:22  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:39:37  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:45:25  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:05:10  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:11:20  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:28:44  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  87/11/17  16:54:22  mikem
**      changed to not use CH anymore
**      
**      Revision 1.2  87/11/10  12:37:08  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.3  87/07/27  11:23:20  mikem
**      Updated to conform to jupiter coding standards.
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # defines */
/* typedefs */
/* forward references */
/* externs */
/* statics */


/*{
** Name: CVan	 - ascii character string to natual integer conversion.
**
** Description:
**    `a' is a pointer to the character string, `i' is a
**    pointer to the natural integer which is to contain the result.
**
**    A valid string is of the form:
**    	<space>* [+-] <space>* <digit>* <space>*
**
** Inputs:
**	a				    ascii string.
**
** Output:
**	i				    return number.
**
**      Returns:
**	    OK:				    succesful conversion; 
**					    `i' contains the integer
**	    CV_OVERFLOW:		    numeric overflow; `i' is unchanged
**	    CV_UNDERFLOW:		    numeric underflow; 'i' is unchanged
**	    CV_SYNTAX:		    	    syntax error; `i' is unchanged
**
** History:
**	20-sep-1985 (joe)
**	    written
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.
*/

/*
** ON most machines call CVal
*/
STATUS
CVan(a, i)
register char	*a;
i4	*i;
{
	return CVal(a, i);
}
