/*
**	qryop.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	qryop.c	- Get query operator for a field.
**
** Description:
**	This file contains a routine to get the query operator
**	for a field.  The only routine in this file is FDqryop().
**
** History:
**	??/??/?? (???) - Initial version.
**	02/14/87 (dkh) - Added support for ADTs.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	"fdfuncs.h" 


FUNC_EXTERN	REGFLD	*FDqryfdop();


/*{
** Name:	FDqryop - Get the query operator for a field.
**
** Description:
**	This routine gets the query operator for a field given
**	a pointer to the form and the name of the field in the
**	form.
**
** Inputs:
**	frm	Pointer to a FRAME structure, where field resides.
**	name	Name of the field.
**
** Outputs:
**	oper	Pointer to a i4  where operator is to be stored.
**
**	Returns:
**		TRUE	If field exist.
**		FALSE	If the field does not exist or a bad DB_DATA_VALUE
**			is passed in.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	09/14/82 (???) - Initial version.
**	02/11/87 (dkh) - Added support for ADTs.
*/
bool
FDqryop(FRAME *frm, char *name, i4 *oper)
{
    	register REGFLD	*fld;

	if ( oper == NULL )
		return FALSE;

	*oper = ( (fld = FDqryfdop(frm, name)) == NULL )
			? fdNOP : fld->fltype.ftoper;
	return TRUE;
}
