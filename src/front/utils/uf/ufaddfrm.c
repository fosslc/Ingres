/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<lo.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<uf.h>
#include	"eruf.h"

/**
** Name:	ufaddfrm.c -	Front-End Utility Add Forms Module.
**
** Description:
**	Contains the routine used to add forms to the FRS.  Defines:
**
**	IIUFaddForms()	add forms.
**
** History:
**	Revision 6.3  89/10  wong
**	Initial revision.
**/

/*{
** Name:	IIUFaddForms() -	Add Forms.
**
** Description:
**	Adds the forms in the input list to the FRS from the form index file.
**
** Input:
**	forms	{char *[]}  The list of forms, NULL terminated.
**
** Returns:
**	{STATUS}  OK if no errors, FAIL otherwise.
**
** History:
**	10/89  (jhw)  Written.
*/

STATUS
IIUFaddForms ( forms )
register char **forms;
{
	register LOCATION	*loc;

	if ( (loc = IIUFlcfLocateForm()) == NULL )
		return FAIL;

	while ( *forms != NULL )
	{
		if ( IIUFgtfGetForm(loc, *forms) != OK )
		{
			IIUGerr(E_UF0051_LoadForm, UG_ERR_ERROR, 1,(PTR)*forms);
			return FAIL;
		}
		++forms;
	}
	return OK;
}
