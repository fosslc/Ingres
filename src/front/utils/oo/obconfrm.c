/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<ooclass.h>
#include	<oosymbol.h>
#include	<oodefine.h>
#include	<ui.h>
#include	<uf.h>
#include	"eroo.h"

/**
** Name:	obconfrm.c -	Object Module Confirm Name Module.
**
** Description:
**	Contains the routine used to confirm a name in the database.  Defines:
**
**	iiobConfirmName()	check name with respect to name space.
**
** History:
**	03-may-93 - Added include for uf.h to define IIUFver() as a boolean
**		    function. (shelby)
**      18-oct-1993 (kchin)
**          	Since the return type of OOsnd() has been changed to PTR,
**          	when being called in iiobConfirmName(), its return
**          	type needs to be cast to the proper datatype.
**
**	Revision 6.4  89/07  wong
**	Renamed from 'iiooConfirmSave()' and moved here as an interactive
**	confirm name method.
**
**	Revision 6.2  89/02  wong
**	Modified to use '_authorized' method to check for overwrite ability.
**
**	Revision 6.0  87/04  peterk
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

char	*iiooStrAlloc();

/*{
** Name:	iiobConfirmName() -	Check Name with Respect to Name Space.
**
** Description:
**	Check the database for any name conflict with any accessible object of
**	the same type before saving it.  If any such object exists the user is
**	informed and is asked for confirmation before any save that would
**	overwrite or make inaccessible any such object.  If the saved object
**	will be a new one based on an edit of an existing one, the new object
**	is instantiated here, with the new ID being returned to the caller.
**
**	If we're checking for a rename, we're checking any special requirements
**	on the name.  There aren't any at this generic level, so we'll
**	just return OC_UNDEFINED, which is success, since it isn't nil.
**
**	Notes:  Assumes that the name is a legal INGRES or object name.
**
** Input params:
**	OBJECT	*obj;		// object being created or saved.
**	char	*savename;	// name under which to save object.
**	i4	rename;		// Are we checking validity for a rename
**
** Returns:
**	{OOID}	ID of object to be saved
**		nil if error.
**
** Side Effects:
**	Existing object of same name may be retrieved from database;
**
** History:
**	4/87 (peterk) - created.
**	11/25/87 (peterk) - changed to mark short_ and long_remark fields
**		changed whenever an object is being overwritten to make sure
**		that blank remarks fields are picked up and overwrite existing
**		remarks.
**	02/89 (jhw)  Modified to use '_authorized' method to check for
**			overwrite ability.
**	07/89 (jhw)  Modified to be method to confirm name.  (Legality check
**		is now separate _checkName method.)
**	10/90 (Mike S) Add argument, so we know if it's a Rename check.
**      06-dec-93 (smc)
**		Bug #58882
**		Commented lint pointer truncation warning.
*/
OOID
iiobConfirmName ( obj, savename, rename )
register OO_OBJECT	*obj;
char			*savename;
i4			rename;
{
	if (rename)
		return OC_UNDEFINED;	/* No special check to make */

	if ( !STequal(obj->name, savename) || !obj->data.inDatabase )
	{ /* new name or new object */
	    OOID	id2 = OC_UNDEFINED;
	    OOID	class = obj->class;

    	    /* lint truncation warning if size of ptr > OOID, but code valid */
	    if ( (id2 = (OOID)OOsnd(class, _withName, savename, (char*)NULL)) != nil )
	    { /* already exists in DB ... */
		i4		argcnt;
		ER_MSGID	promptid;
		OO_OBJECT	*obj2;
		bool		newobj = FALSE;

		obj2 = OOp(id2);
		if ( newobj = !(bool)OOsndSelf(obj2, _authorized) )
		{ /* ... cannot change it (likely owned by the DBA) */
			IIUGerr( S_OO002C_OwnedByDBA, UG_ERR_ERROR,
					2, OOpclass(class)->name, savename
			);
			argcnt = 1;
			promptid = S_OO002D_SaveAsNewObject_Promp;
			obj->data.inDatabase = FALSE;
		}
		else
		{ /* ... so you can overwrite it */
			IIUGerr( S_OO002E_ExistsInDatabase, UG_ERR_ERROR,
					2, OOpclass(class)->name, savename
			);
			argcnt = 0;
			promptid = S_OO002F_OverwriteIt_Prompt;
		}
		if ( !IIUFver(ERget(promptid), argcnt, (PTR) savename) )
			return nil;

		if ( !newobj )
		{ /* destroy for overwrite ... */
			OOsndSelf(obj2, _destroy);
		}
	    }
	    if ( obj->data.inDatabase )
	    { /* requires new object in DB ... */
		/*
		** A new name here means to save as a different object.
		** If ID already represents an existing object (i.e., is
		** a permanent ID,) then we have to create a different
		** installed object structure other than that currently
		** pointed to by 'obj'.
		*/
    		/* lint truncation warning if size of ptr > OOID, 
		   but code valid */
		id2 = (OOID)OOsnd(obj->ooid, _copy);
		obj = OOp(id2);
		obj->create_date = NULL;
	    }
	    obj->name = iiooStrAlloc(obj, savename);
	}
	return obj->ooid;
}
