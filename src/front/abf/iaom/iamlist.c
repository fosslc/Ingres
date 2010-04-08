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
** Name:	iamlist.c -	Application Component Lists Module.
**
** Description:
**	Contains routines used to add and remove application components and
**	forms references from the internal application list.  This list is
**	kept sorted in ascending order.  Defines:
**
**	IIAMinsComp()	insert componenent to application components list.
**	iiamRmComp()	remove component from application components list.
**	iiamLkComp()	look up componenent in application components list.
**	iiamMvComp()	move a component because it has a new object id
**
** History
**	Revision 6.2  89/02  wong
**	Initial revision.
**
**	7/19/90 (Mike S) Put components into hash table
*/

FUNC_EXTERN VOID IIAMactAddCompTable();
FUNC_EXTERN STATUS IIAMdctDelCompTable();

/*{
** Name:	IIAMinsComp() -	Insert Component into Application
**					Components List.
** Description:
**	Insert a component into the components list for an application.  The
**	components list is sorted in ascending order on the object ID, although
**	we try not to rely on that.
**
** Inputs:
**	app	{APPL *}  The application object.
**	comp	{APPL_COMP *}  The application component object.
**
** History:
**	02/89 (jhw)  Written.
*/
VOID
IIAMinsComp ( app, comp )
APPL			*app;
register APPL_COMP	*comp;
{
	register APPL_COMP	*newp;
	register APPL_COMP	**last;

	comp->appl = app;
	last = (APPL_COMP **)&app->comps;
	for ( newp = *last ; newp != NULL && newp->ooid <= comp->ooid ;
			newp = newp->_next )
	{
		last = &newp->_next;
	}
	comp->_next = newp;
	*last = comp;

	/* Add to hash table */
	IIAMactAddCompTable(comp);
}

/*{
** Name:	iiamRmComp() -	Remove a Component from the Application
**					Components List.
** Description:
**	Removes a component from the internal components list for an
**	application.
**
** Inputs:
**	app	{APPL *}  The application object.
**	comp	{APPL_COMP *}  The application component object.
**
** History:
**	02/89 (jhw)  Written.
*/
VOID
iiamRmComp ( app, comp )
APPL			*app;
register APPL_COMP	*comp;
{
	register APPL_COMP	*listp;
	register APPL_COMP	**last;
	STATUS status;

	if ( comp == NULL )
		return;

	last = (APPL_COMP **)&app->comps;
	for ( listp = *last ; listp != NULL && listp != comp ;
			listp = listp->_next )
	{
		last = &listp->_next;
	}
	if ( listp == comp )
	{
		*last = listp->_next;
		comp->appl = NULL;
		comp->_next = NULL;
	}

	status = IIAMdctDelCompTable(comp);
}

/*{
** Name:	iiamLkComp() -	Look up a Component in the Application
**					Components List.
** Description:
**	Finds a component by its ID in the internal components list for an
**	application.  
**
** Inputs:
**	app	{APPL *}  The application object.
**	id	{OOID}  The application component object ID.
**
** Returns:
**	{APPL_COMP *}  The application component object.
**
** History:
**	02/89 (jhw)  Written.
*/
APPL_COMP *
iiamLkComp ( app, id )
APPL	*app;
OOID	id;
{
	register APPL_COMP	*listp;

	if ( id <= 0 )
		return NULL;

	for ( listp = (APPL_COMP *)app->comps ;
			listp != NULL && listp->ooid != id ;
				listp = listp->_next )
		;

	return listp;
}


/*{
** Name:	iiamMvComp()	move a component because it has a new object id
**
** Description:
**	Form references are sometimes entered with temporary (negative)
**	object ID's, with their real object ID's entered later.  We move
**	the formref to its proper place in the chain, but don't change its
**	associated dependencies.	
**
** Inputs:
**	app	{APPL *}  The application object.
**	comp	{APPL_COMP *}  The application component object.
**	newid	{OOID}		The new object id
**
** History:
**	9/14 (Mike S) Initial version
*/
VOID
iiamMvComp(app, comp, newid)
APPL		*app;
APPL_COMP	*comp;
OOID		newid;
{
	register APPL_COMP	*listp;
	register APPL_COMP	**last;

	/* Find it in the list */
	last = (APPL_COMP **)&app->comps;
	for ( listp = *last ; listp != NULL && listp != comp ;
			listp = listp->_next )
	{
		last = &listp->_next;
	}
	if (listp == NULL)
		return;		/* We don't expect this */

	/* Remove it */
	*last = listp->_next;
	
	/* Re-insert it */
	comp->ooid = newid;
	last = (APPL_COMP **)&app->comps;
	for ( listp = *last ; listp != NULL && listp->ooid <= newid ;
			listp = listp->_next )
	{
		last = &listp->_next;
	}
	comp->_next = listp;
	*last = comp;
}
