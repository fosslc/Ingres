
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
** Name:	gnfind.c - find name space information
**			GN internal use only
**	
** Defines
**		find_space()
*/

extern SPACE *List;
extern i4  Hlen;
extern bool Igncase;

/*{
** Name:   find_space - find name space.
**
** Description:
**
**	find a numbered name space.  This also sets Hlen for use in the
**	hash function.  We want to be absolutely certain that Hlen tracks
**	with the hash table we're using, and this routine is always called
**	to get the SPACE information.  For this reason we also set Igncase
**	here, also.
**
** Inputs:
**
**     gcode - generation class code, ie. numeric identifier for table.
**
** Outputs:
**	pred - predecessor to space, if found (NULL) if head of list.
**
**     return:
**	entry on space list, NULL for failure.
*/

SPACE *
find_space(gcode,pred)
register i4  gcode;
register SPACE **pred;
{
	register SPACE *sp;

	*pred = NULL;
	for (sp = List; sp != NULL; sp = sp->next)
	{
		if (sp->gcode == gcode)
		{
			Hlen = sp->ulen;
			Igncase = sp->crule != GN_SIGNIFICANT;
			break;
		}
		*pred = sp;
	}

	return (sp);
}
