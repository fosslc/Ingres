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
** Name:	gnoldnm - add old name to space
**	
** Defines
**		IIGNonOldName
*/

extern i4  Clen;

/*{
** Name:   IIGNonOldName - add old name to the space.
**
** Description:
**
**	Add old, previously generated name to a numbered name space.
**	This is used after IIGNssStartSpace to fill in all the existing
**	names prior to generating new ones.  Will generate a copy - the
**	input name need not be permanent storage.
**
**	Because of backwards compatibility issues, this routine is a little
**	tricky - the PREFIX will not be enforced!  GNE_EXIST indicates
**	collision with the EXACT NAME, including those beyond the uniqueness
**	length.  Names longer than truncation length are allowed also.
**
** Inputs:
**
**	gcode - generation class code, ie. numeric identifier for table.
**	name - old name
**
** Outputs:
**
**     return:
**	 OK          success
**	 GNE_EXIST   name collides
**	 GNE_NAME    bad name
**	 GNE_CODE    numbered name space not defined
**	 GNE_MEM     memory allocation failure
**
** History:
**
**	04-mar-92 (leighb) DeskTop Porting Change:
**		Moved function declaration outside of calling function.
**		This is so function prototypes will work.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
*/

STATUS
IIGNonOldName(gcode,name)
i4  gcode;
char *name;
{
	SPACE *sp,*pred;
	PTR *dat;
	char ncopy[NAMEBUFFER];

	if ((sp = find_space(gcode,&pred)) == NULL)
		return (GNE_CODE);

	if (legalize(sp,name) != OK)
		return (GNE_NAME);

	/*
	** compare whole string in searching for entry
	*/
	Clen = NAMEBUFFER;
	if (IIUGhfHtabFind(sp->htab,name,&dat) == OK)
		return (GNE_EXIST);

	if ((name = FEtsalloc(sp->tag,name)) == NULL)
	{
		(*(sp->afail))();
		return (GNE_MEM);
	}

	if (IIUGheHtabEnter(sp->htab,name,name) != OK)
		return (GNE_MEM);

	return (OK);
}
