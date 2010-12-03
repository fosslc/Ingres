/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cv.h>

/**
** Name: CVOL.C - convert octal string to long.
**
** Description:
**      This file contains the following external routines:
**      This file contains the following internal routines:
**    
**      CVol()             -  Convert octal string to long.
**
** History:
 * Revision 1.1  90/03/09  09:14:25  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:40:24  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:45:55  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:05:40  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:11:23  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:28:52  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  87/11/10  17:09:56  mikem
**      fix template comments
**      
**      Revision 1.2  87/11/10  12:37:56  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.3  87/07/27  11:23:56  mikem
**      Updated to conform to jupiter coding standards.
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	29-Nov-2010 (frima01) SIR 124685
**	    Added include of cv.h with CVol prototype.
**/

/* # defines */
/* typedefs */
/* forward references */
/* externs */
/* statics */


/*{
** Name: CVol - convert octal to i4
**
** Description:
**    converts 'octal' number to an integer which is returned in 'result'
**
** Inputs:
**	octal				    string representation to convert.
**
** Output:
**	result				    result of conversion.
**
**      Returns:
**          OK                         	    Function completed normally. 
**          non-OK status                   Function completed abnormally.
**
** History:
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.
*/
STATUS
CVol(octal, result)
char	octal[];
i4	*result;
{
	register	i4	i;
	register	i4	n;


	n = 0;

	for (i = 0; octal[i] >= '0' && octal[i] <= '7'; ++i)
		n = 8 * n + octal[i] - '0';

	*result = n;

	return(OK);
}
