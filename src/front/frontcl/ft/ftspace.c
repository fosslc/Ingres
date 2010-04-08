/*
**  FTspace
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	*Sccsid = "%W%	%G%";
**
**  Description:
**	FTspace returns the number of bytes (0 or 1) to be reserved
**	between certain pairs of adjoining elements of a new form.  
**	The relevant sorts of pairs of adjoining elements are:
**	any 2 adjoining non-box elements, except for 2 adjoining
**	"boxed" fields.  (A "boxed" field is a table field or
**	a boxed regular field).
**
**  History:
**	09/11/90 (esd) - Derive value from logical symbol II_ATTRBYTES_DEFAULT
**			 The non-FT3270 version will default to 0 if this
**			 logical symbol is not a valid 'yes' or 'no' response.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h> 
# include	<er.h> 
# include	<nm.h> 

static spacesize = -1;

i4
FTspace()
{
	char	*cp;
	STATUS	status;

	if (spacesize < 0)
	{
		NMgtAt(ERx("II_ATTRBYTES_DEFAULT"), &cp);
		(VOID) IIUGyn(cp, &status);
		if (status == E_UG0004_Yes_Response)
		{
			spacesize = 1;
		}
		else
		{
			spacesize = 0;
		}
	}
	return (spacesize);
}
