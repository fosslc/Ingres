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
#include	<ooclass.h>
#include	<oodefine.h>
#include	<oosymbol.h>
#include	<oocat.h>
# include	<uigdata.h>

/**
** Name:	catrec.c -	Catalog Record Methods.
**
** Description:
**	Contains utilities used to access the rows in an object catalog
**	table field of a form.	This file defines:
**
**	OOcatObject()	initialize object from row information.
**	OOcatRecord()	initialize row record from object
**
** History:
**	Revision 6.2  89/01  wong
**	Moved from "catutil.qsc".
**	89/08/01  danielt  Concurrency changes.
**
**	Revision 4.0  86/01  wong/peterk
**	Initial revision.
*/

/*{
** Name:	OOcatObject() -	Initialize (or Create) Object from Record.
**
** Description:
**	Argument 'row' must point to an allocated OO_CATREC structure
**	previously filled in with information from a catalog tableField
**	row (e.g. from a call to OOcatCheck.)
**
**	If id in row argument is not a valid object id then
**	allocate a new object structure initialized from row structure
**	members; else re-initialize existing object from row structure
**	members.
**
**	The object is NOT written to the database;
**
**	On return with STATUS of OK, row->id contains a valid id
**	for the object.
**
**	Initialization is performed by invoking the _init
**	method of the object's class, and passing the members of the
**	OO_CATREC structure as the initializing attributes.
**
** Input params:
**	OOID	class;		// object class id
**	OO_CATREC	*row;	// ptr to filled in row structure.
**
** Output params:
**	OO_CATREC	*row;	// ptr to row structure.
**			    .id // will contain valid object id
**				   if function returns OK.
** Returns:
**	STATUS
**	    OK		// operation performed successfully
**	    FAIL	// operation failed.
**
** Side Effects:
**	new OO_OBJECT structure may be allocated.
***
** History
**	23-may-1988 (danielt)
**	fixed jupbug # ?? (if the object is in memory, don't create
**		a new one.)
**	08-jan-1989 (danielt)
**		Set object state not in DB when instantiated through "_new0"
**		since 'OOp()' will fetch it if not in memory.
**	28-aug-1990 (Joe)
**	    Changed references to IIUIgdata to the UIDBDATA structure
**	    returned from IIUIdbdata().
**      18-oct-1993 (kchin)
**          	Since the return type of OOsnd() has been changed to PTR,
**          	when being called in OOcatObject(), its return
**          	type needs to be cast to the proper datatype.
**      06-dec-1993 (smc)
**	    Bug #58882
**          Commented lint pointer truncation warning.
*/
STATUS
OOcatObject(class, row)
OOID	class;
OO_CATREC	*row;	/* ptr to filled in row structure. */
{
	OO_OBJECT	*obj;
	char		*long_rem; 
	OOID		objid = row->id;
	
	long_rem = ( *row->long_remark == EOS )
			? NULL : STalloc(row->long_remark);
	/*
	** instantiate the object (create a new level 0 object)
	** if it isn't already in memory or in the database.
	*/
	if ( row->id <= UNKNOWN || (obj = OOp(row->id)) == NULL )
	{
		if ( row->id <= UNKNOWN )
			row->id = IIOOnewId();
    		/* lint truncation warning if size of ptr > OOID, 
		   but code valid */
		objid  = (OOID)OOsnd(class, _new0, row->id, STalloc(row->name),
				row->env, IIUIdbdata()->user, row->is_current,
				STalloc(row->short_remark),
	    			STalloc(row->create_date),
				STalloc(row->alter_date), long_rem
		);
		if ( objid == nil )
			return FAIL;
		obj = OOp(objid);
		obj->data.inDatabase = FALSE;
	}
	else
	{
		obj->name = STalloc(row->name);
		obj->env = row->env;
	    	obj->owner = IIUIdbdata()->user;
		obj->is_current = row->is_current; 
		obj->short_remark = STalloc(row->short_remark);
	    	obj->create_date = STalloc(row->create_date); 
		obj->alter_date = STalloc(row->alter_date); 
		obj->long_remark = long_rem;
	}
	obj->data.dbObject = TRUE;
	row->id = obj->ooid;
	return OK;
}

/*{
** Name:	OOcatRecord() -	initialize OO_CATREC row record from object
**
** Description:
**	Assigns values to 'rec' OO_CATREC structure from attributes of
**	object with OOID 'id'.
**
** Inputs:
**	OOID		id;	// object id
**	OO_CATREC	*rec;	// ptr to catalog tableField row record.
**
** Outputs:
**	OO_CATREC	*rec;	// row record structure is filled in.
**
** Returns:
**	void
**
** History:
**	6/13/87 (peterk) - created.
*/

#define ifn(x)	(((x) != NULL) ? (x) : _)

STATUS
OOcatRecord(id, rec)
OOID		id;
OO_CATREC	*rec;
{
	OO_OBJECT	*obj = OOp(id);

	if ( obj == NULL )
		return FAIL;

	rec->id = obj->ooid;
	rec->class = obj->class;
	STcopy(ifn(obj->name), rec->name);
	rec->env = obj->env;
	STcopy(ifn(obj->owner), rec->owner);
	rec->is_current = obj->is_current;
	STcopy(ifn(obj->short_remark), rec->short_remark);
	STcopy(ifn(obj->alter_date), rec->alter_date);
	STcopy(ifn(obj->create_date), rec->create_date);
	STcopy(ifn(obj->long_remark), rec->long_remark);

	return OK;
}
