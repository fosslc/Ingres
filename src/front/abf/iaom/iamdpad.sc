/*
**	Copyright (c) 1987, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<oodep.h>
EXEC SQL INCLUDE <ooclass.sh>;
EXEC SQL INCLUDE  <uigdata.sh>;
#include	<acatrec.h>
#include	"iamint.h"

/**
** Name:	iamdpad.sc -	ABF Dependency Catalog Module.
**
** Description:
**	Contains routines to add and delete records from the ABF dependency
**	catalog.  This module is internal to IAOM.  Defines:
**
**	IIAMadlAddDepList()	insert dependency list into catalog.
**	iiamAddDep()		insert dependency for component into catalog.
**	IIAMdpDeleteDeps()	delete dependencies from catalog.
**	iiamRrdReadRecordDeps() read record rependencies for application.
**
** History:
**
**	Revision 6.0  87/10  bobm
**	Initial revision (extracted from "iamcadd.qc.")
**
**	Revision 6.1  88/08/18  kenl
**	Changed QUEL to SQL.
**
**	Revision 6.2  89/01  wong
**	Added 'IIAMdpDeleteDeps()' and abstracted 'iiamAddDep()'.
**
**	7/90 (Mike S)
**	Redid for dependency manager.  Application ID now goes into
**	ii_abfdependencies, and we don't need dummy dependencies.
**
**	Revision 6.5
**	18-may-93 (davel)
**		Added new argument to iiamAddDep() for depended-upon 
**		object owner. Also modify iiamRrdReadRecordDeps() to read
**		the object owner into the ACATDEP structs.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

char *fid_name();

GLOBALREF char *Catowner;

/*{
** Name:	IIAMadlAddDepList() -	Add Dependency Records.
**
** Description:
**	add dependency records to database for a known object.
**	Note that this routine is not usable by ABF, since it can't
**	update the DM graph.  It's only provided for OSL, copyapp, and
**	abfto60.
**
**	Caller handles any transaction management.
**
** Inputs:
**	dep	dependency list.
**	applid	application id
**	oid	id of object depending on dep.
**
** Returns:
**	{STATUS}	OK		success
**			FAIL		database write failure
**
** History:
**	10/87 (bobm)	extracted from iamcadd.qc
**	18-aug-88 (kenl)
**		Changed QUEL to SQL.
**	09-nov-88 (marian)
**		Modified column names for extended catalogs to allow them
**		to work with gateways.
**	01/89 (jhw) -- Abstracted out 'iiamAddDep()'.
**	7/90 (Mike S) -- changed name from IIAMdpAddDeps
**	28-aug-1990 (Joe)
**	    Changed references to IIUIgdata to the UIDBDATA structure
**	    returned from IIUIdbdata().
*/

STATUS
IIAMadlAddDepList ( dep, applid, oid )
register ACATDEP	*dep;
OOID			applid;
OOID			oid;
{

	OO_OBJECT odummy;

	/*
	** We no longer need dummy dependencies, since we've eliminated the
	** 3-way join.
	*/
	for ( ; dep != NULL; dep = dep->next )
	{
		STATUS	rval;

		odummy.ooid = oid;
		odummy.class = OC_UNDEFINED;

		if (dep->class == OC_ILCODE)
			dep->name = fid_name(dep->id);
		if ( (rval =
			iiamAddDep(&odummy, applid, dep->name, dep->class, 
				dep->deptype, dep->owner, dep->deplink, 
				dep->deporder))
				!= OK )
			return rval;
	}

	return OK;
}

/*{
** Name:	iiamAddDep() -	Insert Dependency for Component into Catalog.
**
** Description:
**	Insert a dependency for an ABF component object.  The user is the
**	current one.
**	
**	If the object_class of "comp" is OC_UNDEFINED, don't put the
**	dependency into the graph.
**
** Inputs:
**	comp		{OO_OBJECT *} ABF application component object
**	applid		{OOID}  The ABF application object id.
**	dep_name	{char *}  The name of the depended on object.
**	dep_class	{OOID}  The class of the depended on object.
**	dep_type	{OOID}  The type of dependency.
**	dep_owner	{char *}  The owner of the depended on object.
**	dep_link	{char *}  The dependency link name (for a menu type).
**	dep_order	{nat } The dependency order (for a menu type).
**
** Returns:
**	{STATUS}  The DBMS query status for INSERT INTO.
**
** History:
**	01/89 (jhw)  Abstracted from 'IIAMdepApp()'.
**	28-aug-1990 (Joe)
**	    Changed references to IIUIgdata to the UIDBDATA structure
**	    returned from IIUIdbdata().
**	18-may-93 (davel)
**		Added owner argument for depended-upon object owner.
**		This is usually passed in as NULL 
**		but can also be the explicit owner of the
**		object.  Currently the only case of this is for the "type of
**		table x.y" construct, where the dependency owner will be
**		the table owner ("x" in the above example).  For all other
**		cases when NULL is passed in, default to the current user.
*/


STATUS
iiamAddDep ( comp, applid, dep_name, dep_class, dep_type, dep_owner, 
	     dep_link, dep_order )
EXEC SQL BEGIN DECLARE SECTION;
OO_OBJECT *comp;
OOID	applid;
char	*dep_name;
OOID	dep_class;
OOID	dep_type;
char	*dep_owner;
char	*dep_link;
i4	dep_order;
EXEC SQL END DECLARE SECTION;
{

	if (dep_owner == (char *)NULL)
	{
		dep_owner = (dep_type == OC_DTTABLE_TYPE) 
			    ? ERx("") : IIUIdbdata()->user;
	}
	EXEC SQL REPEATED INSERT INTO ii_abfdependencies
		( object_id, abfdef_applid, abfdef_name, abfdef_owner, 
			object_class, abfdef_deptype,
			abf_linkname, abf_deporder )
		VALUES ( :comp->ooid, :applid, :dep_name, :dep_owner,
				:dep_class, :dep_type,
				:dep_link, :dep_order
			);

	/* Add to graph, as well */
	if (comp->class != OC_UNDEFINED)
		iiamDMaAddDep(comp, dep_name, dep_class, dep_type, 
			      dep_link, dep_order);

	return FEinqerr();
}

/*{
** Name:	IIAMdpDeleteDeps() -	Delete Dependencies from Catalog.
**
** Description:
**	Delete the dependencies for an ABF component object.  Either all
**	dependencies will be removed, or just a certain type of dependencies
**	as specified by the input parameter.
**
** Inputs:
**	comp		{OO_OBJECT *}  The ABF application component object.
**	depty1,2,3	{OOID}  The dependency type to be deleted,
**				all if depty1 = OC_UNDEFINED.
**
** Returns:
**	{STATUS}  The DBMS query status for DELETE FROM.
**
** History:
**	01/89 (jhw)  Written.
*/

STATUS
IIAMdpDeleteDeps ( comp, depty1, depty2, depty3 )
EXEC SQL BEGIN DECLARE SECTION;
OO_OBJECT *comp;
OOID	depty1;
OOID	depty2;
OOID	depty3;
EXEC SQL END DECLARE SECTION;
{
	EXEC SQL REPEATED
		DELETE FROM ii_abfdependencies dep
			WHERE dep.object_id = :comp->ooid  AND
				( :depty1 = -1 /* :OC_UNDEFINED XXX */ OR
					dep.abfdef_deptype = :depty1 OR
					dep.abfdef_deptype = :depty2 OR
					dep.abfdef_deptype = :depty3 );

	/* Delete them from dependency graph */
	iiamDMdDelDep(comp, depty1, depty2, depty3);

	return FEinqerr();

}

/*{
** Name:	iiamRrdReadRecordDeps() -	Read Record Dependencies.
**
** Description:
**	Read the record dependencies for an ABF application.  
**
** Inputs:
**
** Returns:
**	
**
** History:
**	11/89 (billc)  Written.
**	18-may-93 (davel)
**		Select the depended-upon object owner also, and store in
**		the ACATDEP struct..
*/

ACATDEP *
iiamRrdReadRecordDeps ( applid, tag, stat )
EXEC SQL BEGIN DECLARE SECTION;
OOID	applid;
EXEC SQL END DECLARE SECTION;
TAGID	tag;
STATUS	*stat;
{
EXEC SQL BEGIN DECLARE SECTION;
	char 	dname[FE_MAXNAME + 1];
	char 	oname[FE_MAXNAME + 1];
	OOID	class;
	OOID	deptype;
	i4	deporder;
EXEC SQL END DECLARE SECTION;
	ACATDEP	*ret = NULL;

	EXEC SQL REPEATED
		SELECT DISTINCT d.abfdef_name, d.abfdef_owner, d.object_class,
			d.abfdef_deptype, d.abf_deporder
		INTO :dname, :oname, :class, :deptype, :deporder
		FROM ii_abfdependencies d
		WHERE 	d.object_class = :OC_RECORD
			AND d.abfdef_applid = :applid;  
	    	EXEC SQL BEGIN;
	    	{
			ACATDEP	*tmp;

			tmp = (ACATDEP*)FEreqmem(tag, sizeof(ACATDEP), 
						FALSE, stat
					);
			if (tmp == NULL)
				EXEC SQL ENDSELECT;

			tmp->name = FEtsalloc(tag, dname);
			tmp->owner = FEtsalloc(tag, oname);
			tmp->class = class;
			tmp->deptype = deptype;
			tmp->deporder = deporder;

			tmp->next = ret;
			ret = tmp;
	    	}
	    	EXEC SQL END;

	if (stat != NULL)
		*stat = FEinqerr();
	return ret;
}

