/*
**Copyright (c) 1987, 2004 Ingres Corporation
** All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<gn.h>
# include	"gnspace.h"
# include	"gnint.h"

/**
** Name:	gninspc - see if name already in space
**	
** Defines
**		IIGNisInSpace
*/

extern i4  Clen;

/*{
** Name:   IIGNisInSpace - see if name is already in space
**
** Description:
**
**	Check to see if a name is already in a numbered name space.
**	Could actually be done by calling IIGNonOldName, checking for
**	GNE_EXIST, and removing the name again if it returned OK, but
**	that seems a trifle silly.  Also, this won't require any allocation.
**
** Inputs:
**
**	gcode - generation class code, ie. numeric identifier for table.
**	name - old name
**
** Outputs:
**
**     return:
**		FALSE, if name or space doesn't exist.
**		TRUE, otherwise
*/

bool
IIGNisInSpace(gcode,name)
i4  gcode;
char *name;
{
	SPACE *sp,*pred;
	PTR *dat;

	if ((sp = find_space(gcode,&pred)) == NULL)
		return (FALSE);

	/*
	** compare whole string in searching for entry
	*/
	Clen = NAMEBUFFER;
	if (IIUGhfHtabFind(sp->htab,name,&dat) == OK)
		return (TRUE);

	return (FALSE);
}
