/*
**	Copyright (c) 1987, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ui.h>
EXEC SQL INCLUDE	<ooclass.sh>;
#include	"iamint.h"

/**
** Name:	iamcdel.sc -	Application Catalog Object Delete Module.
**
** Description:
**	Delete catalog record for an ABF object.  Defines:
**
**	IIAMcdCatDel()	delete application catalog rows.
**
** History:
**	Revision 6.2  89/01  wong
**	Merged queries and added IL encodings delete with 'class' parameter.
**
**	Revision 6.0  87/05  bobm
**	Initial revision.
*/

STATUS abort_it();

/*{
** Name:	IIAMcdCatDel() -	Delete Application Catalog Rows.
**
** Description:
**	Delete catalog record for an ABF object, which can be the entire
**	application.  The catalog record consists of several rows from
**	several tables.
**
** Inputs:
**	appid	{OOID}  Application ID.
**	objid	{OOID}  Object ID; OC_UNDEFINED for entire application.
**	fid	{OOID}	Fid for objects which got 'em (OSL objects).
**	class	{OOID}  Object class.
**
** Returns:
**	{STATUS}  OK		success
**		  ILE_NOOBJ	doesn't exist
**		  ILE_FAIL	DB failure
**
** History:
**	5/87 (bobm)	written
**	18-aug-18 (kenl)
**		Changed QUEL to SQL
**	12-sep-88 (kenl)
**		Changed EXEC SQL COMMITs to appropriate call to IIUI...Xaction
**		routines.
**	09-nov-88 (marian)
**		Modified column names for extended catalogs to allow them
**		to work with gateways.
**	20-jan-1988 (jhw)  Added 'class' parameter and delete for IL encodings,
**		and merged queries.
**	(9/89) bobm	add fid argument.
*/

STATUS
IIAMcdCatDel ( appid, objid, fid, class )
EXEC SQL BEGIN DECLARE SECTION;
OOID	appid;
OOID	objid;
OOID	fid;
EXEC SQL END DECLARE SECTION;
OOID	class;
{
	if ( objid == appid )
		objid = OC_UNDEFINED;

	iiuicw1_CatWriteOn();

	IIUIbeginXaction();	/* begin transaction */

#ifdef FULLDELETE
	if ( objid == OC_UNDEFINED || class == OC_OSLFRAME ||
		  class == OC_MUFRAME || class == OC_APPFRAME ||
		  class == OC_UPDFRAME || class == OC_BRWFRAME ||
			class == OC_OSLPROC )
	{ /* Delete encoded IL objects */
		if (objid == OC_UNDEFINED)
		{
			EXEC SQL REPEATED DELETE FROM ii_encodings
				WHERE encode_object IN (
				SELECT ii_objects.object_id
				FROM ii_objects, ii_abfdependencies dep,
					ii_abfobjects appobj
				WHERE abfdef_name = ii_objects.object_name  
				  AND dep.object_id = appobj.object_id  
				  AND applid = :appid  
				  AND dep.object_class = :OC_ILCODE
				);

			if ( FEinqerr() != OK )
				return abort_it();

			EXEC SQL REPEATED DELETE FROM ii_objects
				WHERE object_id IN (
				SELECT dep.object_id
				FROM ii_abfdependencies dep,
					ii_abfobjects appobj
				WHERE abfdef_name = ii_objects.object_name  
				  AND dep.object_id = appobj.object_id  
				  AND applid = :appid  
				  AND dep.object_class = :OC_ILCODE
			);

			if ( FEinqerr() != OK )
				return abort_it();
		}
		else
		{
			EXEC SQL REPEATED DELETE FROM ii_encodings
				WHERE encode_object = :fid;

			if ( FEinqerr() != OK )
				return abort_it();

			EXEC SQL REPEATED DELETE FROM ii_objects
				WHERE object_id = :fid;

			if ( FEinqerr() != OK )
				return abort_it();
		}
	}
#endif

	if (objid != OC_UNDEFINED)
	{
#ifdef FULLDELETE
		EXEC SQL REPEATED
			DELETE FROM ii_longremarks WHERE object_id = :objid;

		if ( FEinqerr() != OK )
			return abort_it();

		EXEC SQL REPEATED
			DELETE FROM ii_abfdependencies WHERE object_id = :objid;

		if ( FEinqerr() != OK )
			return abort_it();

		EXEC SQL REPEATED
			DELETE FROM ii_vqtabcols WHERE object_id = :objid;

		if ( FEinqerr() != OK )
			return abort_it();

		EXEC SQL REPEATED
			DELETE FROM ii_menuargs WHERE object_id = :objid;

		if ( FEinqerr() != OK )
			return abort_it();

		EXEC SQL REPEATED
			DELETE FROM ii_vqtables WHERE object_id = :objid;

		if ( FEinqerr() != OK )
			return abort_it();

		EXEC SQL REPEATED
			DELETE FROM ii_framevars WHERE object_id = :objid;

		if ( FEinqerr() != OK )
			return abort_it();

		EXEC SQL REPEATED
			DELETE FROM ii_vqjoins WHERE object_id = :objid;

		if ( FEinqerr() != OK )
			return abort_it();
#endif

		/* Note:  ii_objects delete cannot be last! */

		EXEC SQL REPEATED
			DELETE FROM ii_objects WHERE object_id = :objid;

		if ( FEinqerr() != OK )
			return abort_it();

#ifdef FULLDELETE
		EXEC SQL REPEATED
			DELETE FROM ii_abfobjects WHERE object_id = :objid;

		if ( FEinqerr() != OK )
			return abort_it();
#endif
	}
	else
	{
#ifdef FULLDELETE
		EXEC SQL REPEATED
			DELETE FROM ii_longremarks WHERE object_id IN (
				SELECT object_id FROM ii_abfobjects
					WHERE applid = :appid ); 

		if ( FEinqerr() != OK )
			return abort_it();

		EXEC SQL REPEATED
			DELETE FROM ii_abfdependencies WHERE object_id IN (
				SELECT object_id FROM ii_abfobjects
					WHERE applid = :appid );

		if ( FEinqerr() != OK )
			return abort_it();

		EXEC SQL REPEATED
			DELETE FROM ii_vqtabcols WHERE object_id IN (
				SELECT object_id FROM ii_abfobjects
					WHERE applid = :appid );

		if ( FEinqerr() != OK )
			return abort_it();

		EXEC SQL REPEATED
			DELETE FROM ii_menuargs WHERE object_id IN (
				SELECT object_id FROM ii_abfobjects
					WHERE applid = :appid );

		if ( FEinqerr() != OK )
			return abort_it();

		EXEC SQL REPEATED
			DELETE FROM ii_vqtables WHERE object_id IN (
				SELECT object_id FROM ii_abfobjects
					WHERE applid = :appid );

		if ( FEinqerr() != OK )
			return abort_it();

		EXEC SQL REPEATED
			DELETE FROM ii_framevars WHERE object_id IN (
				SELECT object_id FROM ii_abfobjects
					WHERE applid = :appid );

		if ( FEinqerr() != OK )
			return abort_it();

		EXEC SQL REPEATED
			DELETE FROM ii_vqjoins WHERE object_id IN (
				SELECT object_id FROM ii_abfobjects
					WHERE applid = :appid );

		if ( FEinqerr() != OK )
			return abort_it();
#endif

		/* Note:  ii_objects delete cannot be last! */

		EXEC SQL REPEATED
			DELETE FROM ii_objects WHERE object_id IN (
				SELECT object_id FROM ii_abfobjects
					WHERE applid = :appid );

		if ( FEinqerr() != OK )
			return abort_it();

#ifdef FULLDELETE
		EXEC SQL REPEATED
			DELETE FROM ii_abfobjects WHERE applid = :appid;

		if ( FEinqerr() != OK )
			return abort_it();
#endif

	}

	IIUIendXaction();	/* end transaction */

	iiuicw0_CatWriteOff();

	return OK;
}
