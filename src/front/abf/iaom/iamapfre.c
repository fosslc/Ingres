/*
**	Copyright (c) 1989, 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<abclass.h>

/**
** Name:	iamapfre.c -	Application Object Free Routine.
**
** Description:
**	Contains the routine used to free an application object in memory
**	including its components.  Defines:
**
**	IIAMappFree()	free application object.
**
** History:
**	Revision 6.2  89/02  wong
**	Initial revision.
**/

/*{
** Name:	IIAMappFree() -	Free Application Object.
**
** Description:
**	Free in memory application object including its components.  This
**	removes the application object and its component objects from the
**	OO object cache, and then frees any memory for the objects.
**
** Inputs:
**	app	{APPL *}  Application object structure.
**
** History:
**	02/89 (jhw)  Written.
*/

VOID
IIAMappFree ( app )
APPL	*app;
{
	register APPL_COMP	*comp;

	for ( comp = (APPL_COMP *)app->comps ; comp != NULL ;
			comp = comp->_next )
		IIOOforget((OO_OBJECT *)comp);

	app->ooid = OC_UNDEFINED;
	app->data.inDatabase = FALSE;
	app->comps = NULL;

	IIUGtagFree(app->data.tag);
}
