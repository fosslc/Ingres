
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"


/**
** Name:	ftattr.c - attribute translation function 
**
** Description:
**	Translate from frs attribute top screen attribute
**
** This file defines:
**
**	FTattrxlate	- translate frs attributte to screen attribute
**
** History:
**	06/15/87 (tom) - taken from vtbox.c
**	05/10/89 (tom) - fix bug 5199 "when color of a box is set to 
**			anything but 0 the attributes are'nt displayed 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
/* GLOBALDEF's */
/* extern's */
/* static's */

/*{
** Name:	FTattrxlate - translate atttribute into screen def.
**
** Description:
**	Translate the given attribute from form system values into
**	terminal driver (virtual screen) values .. fact is that 
**	the  form system should probably just use the virtual screen
**	values so that this translation is not necessary.
**
**
** Inputs:
**	i4 attr;	- form system type attribute
**
** Outputs:
**	Returns:
**		i4	- term driver type attribute
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	05/19/88  (tom) written
**	05/10/89 (tom) - fix bug 5199 "when color of a box is set to 
**			anything but 0 the attributes are'nt displayed 
*/
i4
FTattrxlate(flags)
i4 flags;
{
	i4 attr = 0;

	if (flags & fdBLINK)
		attr |= _BLINK;
	if (flags & fdRVVID)
		attr |= _RVVID;
	if (flags & fdUNLN)
		attr |= _UNLN;
	if (flags & fdCHGINT)
		attr |= _CHGINT;

	if (flags & fdCOLOR)	/* gross check first.. any colors? */
	{
		if (flags & fd1COLOR)
			attr |= _COLOR1;
		else if (flags & fd2COLOR)
			attr |= _COLOR2;
		else if (flags & fd3COLOR)
			attr |= _COLOR3;
		else if (flags & fd4COLOR)
			attr |= _COLOR4;
		else if (flags & fd5COLOR)
			attr |= _COLOR5;
		else if (flags & fd6COLOR)
			attr |= _COLOR6;
		else if (flags & fd7COLOR)
			attr |= _COLOR7;
	}
	return (attr);
}
