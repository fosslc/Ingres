/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<me.h>		 
#include	<lo.h>
#include	<st.h>		 
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<abclass.h>
#include	<ilerror.h>
#include	<abrterr.h>
#include	"erab.h"

/**
** Name:	abappget.c -	ABF Application Fetch Routine.
**
** Description:
**	Contains routines that read an application definition from the ABF
**	system catalogs or gets the ID for a named application.  Defines:
**
**	IIABappFetch()	get application definition from ABF system catalogs.
**	iiabGtAppID()	get object ID for named application.
**
** History:
**	Revision 6.4  23-mar-1992 (mgw)
**	Delete reference to obsolete routine abappset()
**
**	Revision 6.3  90/03  wong
**	Abstracted 'iiabGtAppID()' from 'IIABappFetch()' for bug #20229.
**
**	Revision 6.2  89/02  wong
**	Rewrote from 'IIABgcatGet()'.
**
**	Revision 6.0  87/06  wong
**	Moved from "abfcat.c".
**	Modified to use IIAM module to get application.	 (bobm)
**/

GLOBALREF LOCATION	IIabWrkDir;
GLOBALREF char		IIabNamWrk[];		 

/*{
** Name:	IIABappFetch() -	Get Application from ABF Catalogs.
**
** Input:
**	app	{char *}  The application object.
**			.ooid			{OOID}  The application ID,
**							which may be undefined.
**			.data.inDatabase	{bool}  Whether application
**							is in the DB.
**	appname	{char *}  The name of the application to fetch.
**	id	{OOID}  The ID of the object to be retrieved.  If undefined
**				retrieve all application components.
**	iirestrict {bool}	Whether to restrict fetch call dependencies
**
** Returns:
**	{STATUS}  OK, or return status from 'IIAMoiObjId()', 'IIAMapFetch()',
**			or library allocation.
**
** Called by:
**	'IIABeditApp()', 'abfimage()', 'IIABrun()', 'IIABilnkImage()'.
**
** History:
**	apr-1982  Written (jrc)
**	02-jun-86 (bobm)  6.0 catalog / name generation changes.
**	06/87 (jhw) -- Moved call of 'abexcapp()' to 'abdefapp()'.
**	02/89 (jhw)  Rewrote from 'IIABgcatGet()'.
**	07/90 (jhw)  Pass object-code directory location to work directory
**			routines and moved directory error reporting here along
**			with call to 'IIABlnxExtFile()'.
**	15-nov-91 (leighb) DeskTop Porting Change: Interpreted Images
**		Save 'app->name' in globlal string 'IIabNamWrk[]' because
**		the LOCATION 'IIabWrkDir' is global and should not have its
**		name in a automatic variable.
**	11-jan-2001 (bolke01) 
**		Required change to avoid conflict with reserved word restrict.
**		Identified after latest compiler OSFCMPLRS440 installed on axp.osf
**		which includes support for this bases on the C9X review draft .
*/
STATUS
IIABappFetch ( app, appname, id, iirestrict )
register APPL 	*app;
char		*appname;
OOID		id;
bool		iirestrict;
{
	STATUS	stat;

	if ( app->ooid == OC_UNDEFINED || !app->data.inDatabase )
	{
		OOID	appid = app->ooid;

		MEfill(sizeof(*app), '\0', (PTR)app);

		app->ooid = appid;
		app->class = OC_APPL;
		app->data.tag = FEgettag();
		app->comps = NULL;

	}

	if ( app->ooid == OC_UNDEFINED &&
			(stat = iiabGtAppID( appname, &app->ooid )) != OK )
	{ /* not found */
		return stat;
	}

	app->data.dbObject = TRUE;

	if ( (stat = IIAMapFetch(app, id, iirestrict)) != OK )
	{
		if ( appname != NULL && *appname != EOS )
			IIUGerr(NOSUCHAPP, UG_ERR_ERROR, 1, appname);
		else
		{ /* may have been concurrently deleted */
			IIUGerr(E_AB0047_CatGet, UG_ERR_ERROR, 1, app->name);
		}
		return ILE_NOOBJ;
	}

	STcopy(app->name, IIabNamWrk);					 
	if ( (stat = IIABsdirSet(IIabNamWrk, &IIabWrkDir)) != OK )	 
	{
		char	*cp;

		LOtos(&IIabWrkDir, &cp);
		if ( stat != FAIL )
		{ /* cannot write to directory . . . */
			IIUGerr(ABOBJWRT, UG_ERR_ERROR, 1, cp);
			return stat;
		}

		/*  directory does not exist . . . create it. */
		IIUGerr(NOOBJDIR, UG_ERR_ERROR, 2, cp, app->name);

		if ( IIABcdirCreate(app->name, &IIabWrkDir) != OK )
			return FAIL;
	}

	IIABlnxExtFile(&IIabWrkDir);

	return OK;
}

/*{
** Name:	iiabGtAppID() -	Get Object ID for Named Application.
**
** Input:
**	appname	{char *}  The name of the application.
**
** Output:
**	id	{OOID *}  The object ID of the application.
**
** Returns:
**	{STATUS}  OK, if the application exists,
**		  FAIL, if it does not exist,
**		  or return status from 'IIAMoiObjId()'.
**
** Called by:
**	'IIABappFetch()', 'IIabf()'.
**
** History:
**	03/90 (jhw)  Abstracted from 'IIABappFetch()'.
*/
STATUS
iiabGtAppID ( appname, id )
char 	*appname;
OOID	*id;
{
	STATUS	stat;
	OOID	class;

	if ( (stat = IIAMoiObjId( appname, OC_APPL, 0, id, &class )) != OK )
	{ /* error cases */
		if ( stat == ILE_FAIL )
		{
			/*
			** If query failed, BAIL OUT!  We don't really know if
			** there's an application or not.  We don't want the
			** user creating a new application with the same name.
			*/
			IIUGerr( E_AB0047_CatGet, UG_ERR_FATAL, 1, appname );
			/*NOT REACHED*/
		}
	  	else if ( stat == ILE_NOMEM )
		{
			IIUGerr(E_AB000F_Out_of_Memory, UG_ERR_ERROR, 0);
		}
		else if ( stat == ILE_NOOBJ )
		{
			IIUGerr(NOSUCHAPP, UG_ERR_ERROR, 1, appname);
			stat = FAIL;
		}
	}
	return stat;
}
