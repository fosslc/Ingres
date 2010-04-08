/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cv.h>

/**
** Name: CVNA.C - convert i4  to ascii.
**
** Description:
**      This file contains the following external routines:
**    
**      CVna()             -  i4  to ascii
**
** History:
 * Revision 1.1  90/03/09  09:14:24  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:40:16  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:45:45  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:05:30  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:11:22  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:28:49  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.2  87/11/10  12:37:40  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.3  87/07/27  11:23:47  mikem
**      Updated to conform to jupiter coding standards.
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      23-may-97 (mcgem01)
**          Clean up compiler warnings.
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
** Name: CVna - Conver i4  to ascii.
**
** Description:
**	Converts the natural integer num to ascii and stores the
**	result in string, which is assumed to be large enough.
**
** Inputs:
**	i				    number to convert
**
** Output:
**	a				    buffer to put output in.
**
**      Returns:
**	    none. (VOID)
**
** History:
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.
*/
VOID
CVna(i, a)
register i4	i;
register char	*a;
{
	CVla(i, a);
}
