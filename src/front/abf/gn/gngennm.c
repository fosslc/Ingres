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
** Name:	gngennm - generate unique name
**	
** Defines
**		IIGNgnGenName ()
*/

extern i4  Clen;

/*{
** Name:   IIGNgnGenName - Generate new, unique name.
**
** Description:
**
**     Generate a new, unique name within this numbered name space.
**     Name to be based on user name.  The user name may be any string
**     which contains alphanumeric characters.  It may be longer than
**     the truncation length for the name space.  It is not intended
**     to contain the prefix string, although doing so will only
**     result in the generated name attempting to reflect a prefixed
**     prefix.
**
** Inputs:
**
**     gcode - generation class code, ie. numeric identifier for table.
**     uname - user name generated name construction is to be based on.
**
** Outputs:
**
**     name - generated unique name.
**
**     return:
**	 OK          success
**	 GNE_NAME    bad name
**	 GNE_FULL    name space full
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
IIGNgnGenName(gcode,uname,name)
i4  gcode;
char *uname;
char **name;
{
	SPACE *sp,*pred;
	char newname [NAMEBUFFER];
	char *ukey;
	STATUS rc;

	if ((sp = find_space(gcode,&pred)) == NULL)
		return (GNE_CODE);

	/*
	** get a new name
	*/
	if ((rc = make_name(sp,uname,newname)) != OK)
		return (rc);

	/*
	** allocate a copy
	*/
	if ((*name = FEtsalloc(sp->tag,newname)) == NULL)
	{
		(*(sp->afail))();
		return (GNE_MEM);
	}

	/*
	** when we enter a name, we enter the name as the data, also.
	** set Clen to NAMEBUFFER, although comparisons shouldn't get
	** done to enter a new hash table item.
	*/
	Clen = NAMEBUFFER;
	if (IIUGheHtabEnter(sp->htab,*name,*name) != OK)
		return (GNE_MEM);

	return (OK);
}
