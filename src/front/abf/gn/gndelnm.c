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
** Name:	gndelnm.c - delete name from space
**	
** Defines
**		IIGNdnDelName()
*/

extern i4  Clen;

/*{
** Name:   IIGNdnDeleteName - Delete Name from space.
**
** Description:
**
**     Delete existing name from numbered name space.
**
** Inputs:
**
**     gcode - generation class code, ie. numeric identifier for table.
**     name - name.
**
** Outputs:
**
**     return:
**	OK		success
**	GNE_EXIST	name does not exist.
**	GNE_CODE	bad gcode
*/

STATUS
IIGNdnDeleteName(gcode,name)
i4  gcode;
char *name;
{
	SPACE *sp, *pred;
	char uname[NAMEBUFFER];

	if ((sp = find_space(gcode,&pred)) == NULL)
		return (GNE_CODE);

	/* compare ENTIRE name for removal */
	Clen = NAMEBUFFER;

	if (IIUGhdHtabDel(sp->htab,name) != OK)
		return (GNE_EXIST);

	return (OK);
}
