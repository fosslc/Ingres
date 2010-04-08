/*
**	Copyright (c) 1989, 2004 Ingres Corporation
*/

#include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
exec sql include	<ooclass.sh>;
#include	<oocat.h>

/**
** Name:	iamildat.sc -	Return Date for Interpreted Frame Object
**					from Catalogs.
** Description:
**	Contains a routine that returns the catalog date for an interpreted
**	frame object.  Defines:
**
**	IIAMfdFidDate()	get catalog date for IL object.
**
** History:
**	Revision 6.2  89/08  wong
**	Initial revision.	(For JupBug #7226.)
**/

/*{
** Name:	IIAMfdFidDate() - Get Catalog Date for Interpreted Frame Object.
**
** Description:
**	Given an ID for an IL object, return the last altered date for
**	the object from the system catalogs.
**
** Input:
**	objid	{OOID}  The IL object ID.
**	date	{char []}  The reference to the date to be returned.
**
** Output:
**	date	{char []}  The catalog last altered date.
**
** Returns:
**	{STATUS}  OK when a date is read.
**		  FAIL if the object does not exist.
** History:
**	08/89 (jhw) -- Written for JupBug #7227.
*/
STATUS
IIAMfdFidDate ( objid, date )
EXEC SQL BEGIN DECLARE SECTION;
OOID	objid;
char	date[OODATESIZE+1];
EXEC SQL END DECLARE SECTION;
{
	STATUS	rval;

	EXEC SQL REPEATED
		SELECT alter_date INTO :date
				FROM ii_objects, ii_encodings
		WHERE ii_objects.object_id = :objid
			and ii_encodings.encode_object = ii_objects.object_id
			and encode_sequence = 0;

	if ( (rval = FEinqerr()) != OK )
		return rval;
	else if ( FEinqrows() != 1 )
		return FAIL;
	else
		return OK;
}
