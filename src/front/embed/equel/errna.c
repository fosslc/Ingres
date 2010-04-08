/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include 	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */

/*
+* Filename:	errna.c
** Purpose:	Error argument conversion routine.
**
** Defines:	er_na		- Convert i4  args to char.
-*
** History:
**		20-may-1987	- Written.
**				  (bjb)
**		23-dec-1987	- Seperate file for OSL. (jhw)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*{
+*  Name: er_na - Convert integer to character string for callers of er_write()
**
**  Description:
**	This routine is called by callers of er_write when a parameter to
**	an error message is an integer (i4).  The IIUG routines actually
**	permit parameters of all types; however, the preprocessor convention
**	is to always send character string arguments.
**	The routine returns a pointer to a static character string 
**	representing the incoming integer number.  For example, er_write is 
**	called as follows:
**
**	    er_write( E_EQ0001_EqERRCONST, 3, "oh my", er_na(1), er_na(4) );
**
**  Inputs:
**	er_nat		Nat to be converted
**
**  Outputs:
**	Returns:
**	    Pointer to converted string.
**	Errors:
**	    None.
-*
**  Side Effects:
**	
**  History:
**	20-may-1987 - written (bjb)
*/

static char	er_buf[3][12] ZERO_FILL;
static i4	er_index = 0;

char	*
er_na( er_nat )
i4	er_nat;
{
    char	*er_ptr;

    er_ptr = er_buf[er_index];
    CVna( er_nat, er_ptr );
    er_index = (er_index +1) % 3;

    return er_ptr;
}
