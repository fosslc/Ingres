/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<cm.h> 

/**
** Name:    fechkname.c -	Front-End Check Object Name Module.
**
** Description:
**	Contains the routine used to verify object names as legal.  Defines:
**
**	FEchkname()	check object name.
**
** History:
**	Revision 60.0  87/11/03  wong
**	Modified to use CM module and check for name length.
**
**	Revision 50.0  86/02/06  peter
**	Add first letter alpha restrict.
**
**	Revision 40.0  85/11/26  peter
**	Removed restriction on relation names for underscores ('_'.)
**
**	Revision 30.0  84/02/01  nadene
**	Initial revision.
*/

/*{
** Name:    FEchkname() -	Check Object Name.
**
** Description:
**	This routine verifies that the input object name is legal.  It does so
**	by using the CM module to check that the first character is a legal
**	start character and that subsequent characters are also legal characters
**	for an INGRES object name.  Finally, it checks that the length of the
**	name is also legal.
**
** Input:
**	name	{char *}  The object name to be checked.
**
** Returns:
**     {STATUS}	OK - legal INGRES name.
**		FAIL - illegal character in name.
**
** History:
**	2/1/84 (nml)  Written.
**	11/26/85 (peter)  Take out restriction on table names.  They can
**			now contain underscores.
**	2/6/86 (prs)  Add first letter alpha restrict.
**	3/87 (jhw)  Modified to use CM module and check for name length.
**	9-sep-1987 (peter)
**		Change name length check to use FE_MAXNAME.
*/

STATUS
FEchkname (name)
char	*name;
{
    register char    *cp = name;

    if (CMnmstart(cp))
    {
	do {
	    CMnext(cp);
	} while (*cp != EOS && CMnmchar(cp));
    }

    return (*cp == EOS && cp - name <= FE_MAXNAME) ? OK : FAIL;
}
