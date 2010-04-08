/*
** Copyright (c) 2004 Ingres Corporation
*/
#include <compat.h>
#include <cm.h>
#include <rpu.h>

/*
** Name:	validnm.c - is it a valid Ingres name
**
** Description:
**	Defines
**		RPvalid_name - is it a valid Ingres name?
**
** History:
**	16-dec-96 (joea)
**		Created based on validnm.c in replicator library.
**/

/*{
** Name:	RPvalid_name - is it a valid Ingres name?
**
** Description:
**	Verifies that the name argument is a valid Ingres name, that is
**	starts with an alphabetic character and contains no embedded
**	blanks or other illegal characters.
**
** Inputs:
**	name	- character array
**
** Returns:
**	TRUE	- name is valid
**	FALSE	- illegal name error
**/
bool
RPvalid_name(
char		*name)
{
	if (!CMnmstart(name))
		return (FALSE);

	while (CMnext(name) != EOS)
	{
		if (*name == EOS)
			return (TRUE);
		if (!CMnmchar(name))
			return (FALSE);
	}
}

/*{
** Name:	RPvalid_vnode - is it a valid Ingres vnode name?
**
** Description:
**	Verifies that the name argument is a valid Ingres vnode name.
**      This is less restrictive than RPvalid_name above, for instance a 
**      hyphen is not allowed in an ingres name.
**
** Inputs:
**	name	- character array
**
** Returns:
**	TRUE	- name is valid
**	FALSE	- illegal name error
** History:
**      20-Nov-2009 (coomi01) b122933
**          Created.
**/
bool
RPvalid_vnode(
char		*name)
{
    char *startPt = name;

    do
    {
	/*
	** Allowed to embed a single (but not double)
	** colon in middle (but not end) of name 
	*/
	if ( *name == ':' )
	{
	    CMnext(name);
	    if ( *name == ':' )
	    {
		return (FALSE);
	    }
	    if ( *name == EOS )
	    {
		return (FALSE);
	    }
	}

	if (*name == EOS)
	{
	    /* Null names not allowed */
	    return (name != startPt);
	}

	/* No commas, slashes, white spaces or non printables  */
	if ( (*name == ',') || *name == '/' || CMwhite(name) || !CMprint(name) )
	    return (FALSE);
    }
    while (CMnext(name) != EOS);

    return (TRUE);
}
