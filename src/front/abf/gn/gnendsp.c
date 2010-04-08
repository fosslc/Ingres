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
** Name:	gnendsp.c - end name space
**	
** Defines
**		IIGNesEndSpace()
*/

extern SPACE *List;

/*{
** Name:   IIGNesEndSpace - end numbered name space.
**
** Description:
**
**     Remove a numbered name space from existence.  No more reference
**     may be made to this gcode until a new IIGNssStartSpace call
**     restarting it.
**
** Inputs:
**
**     gcode - generation class code, ie. numeric identifier for table.
**
** Outputs:
**
**     return:
**	 OK		success
**	 GNE_CODE	name space does not exist
*/

STATUS
IIGNesEndSpace(gcode)
i4  gcode;
{
	SPACE *pred, *sp;

	if ((sp = find_space(gcode,&pred)) == NULL)
		return (GNE_CODE);

	/* unlink from list of spaces */
	if (pred == NULL)
		List = sp->next;
	else
		pred->next = sp->next;

	/* release associated hash table, free memory */
	IIUGhrHtabRelease(sp->htab);
	FEfree(sp->tag);

	return (OK);
}
