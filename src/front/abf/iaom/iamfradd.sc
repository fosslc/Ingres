/*
**	Copyright (c) 1987, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<er.h>
#include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ui.h>
#include	<ooclass.h>
#include	<oodep.h>
#include	<oocat.h>
#include	"iamstd.h"
#include	"eram.h"

/**
** Name:	iamfradd.c -	Add a FID to the Catalogs Module.
**
** History:
**	Revision 6.0  87/08/17  joe
**	Initial revision; added for CopyApp.
**
**	Revision 6.1  88/06  wong
**	Modified to use SQL.
**
**	Revision 6.2  89/01  wong
**	Removed 'owner' parameter and changed to call 'iiamAddDep()'.
**
**	Revision 6.5
**	18-may-93 (davel)
**		In IIAMafrAddFidRecord(), add parameter to iiamAddDep() call.
*/

/*{
** Name:	IIAMafrAddFidRecord() -	Add Interpreted Frame Object Dependency
**						to ABF Catalogs.
**
** Description:
**	Add a fid record to ii_objects and ii_abfdependencies.
**
**	Used by copyapp.  Not usable by ABF, since it doesn't pass enough
**	infomration to iiamAddDep toupdate the DM graph.
**
** Inputs:
**	name	name of frame
**	id	id of frame to add fid for.
**	applid	application id
**
** Returns:
**	{STATUS}  OK		success
**		  ILE_FRDWR	ingres failure
**
** Side Effects:
**	Adds a tuple to the 'ii_abfdependencies' table.  (Indirectly,
**	'fid_name()' will update 'ii_id' and add to 'ii_objects'.)
**
** History:
**	17-aug-1987 (Joe)
**		Initial Version.
**	12-sep-88 (kenl)
**		Changed EXEC SQL COMMIT to call to routine IIUIendXatcion().
**	09-nov-88 (marian)
**		Modified column names for extended catalogs to allow them
**		to work with gateways.
**	01/89 (jhw) -- Removed 'owner' parameter and changed to call new
**		'iiamAddDep()'.
**	18-may-93 (davel)
**		Add parameter to iiamAddDep() call.
**	13-May-2005 (kodse01)
**	    replace %ld with %d for old nat and long nat variables.
*/

STATUS
IIAMafrAddFidRecord ( name, id, applid )
char	*name;
OOID	id;
OOID	applid;
{
	OOID	fid;
	STATUS	rval;
	char	buf[16];
	OO_OBJECT dummy;

	if ( (rval = IIAMfaFidAdd(name, _, _, &fid)) != OK )
		return rval;

	iiuicw1_CatWriteOn();
	dummy.ooid = id;
	dummy.class = OC_UNDEFINED;
	rval = iiamAddDep( &dummy, applid, STprintf(buf, ERx("%d"), fid),
				OC_ILCODE, OC_DTDBREF, (char *)NULL, _, 0
	);
	IIUIendXaction();
	iiuicw0_CatWriteOff();

	return rval;
}
