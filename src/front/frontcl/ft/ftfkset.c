/*
**  FTfkset
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	04/03/90 (dkh) - Integrated MACWS changes.
**	05/22/90 (dkh) - Enabled MACWS changes.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
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
# include	"ftframe.h"


FTfkset(mode)
i4	mode;
{
	i4	fkmode;

# ifdef DATAVIEW
	_VOID_ IIMWfkmFKMode(mode);
# endif	/* DATAVIEW */

	if (mode == FT_FUNCKEY)
	{
		fkmode = TD_FK_SET;
	}
	else
	{
		fkmode = TD_FK_UNSET;
	}
	TDfkmode(fkmode);
}
