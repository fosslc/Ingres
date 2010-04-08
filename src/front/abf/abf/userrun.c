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
#include	<abclass.h>
#include	<fdesc.h>
#include	<abfrts.h>
#include	"abfgolnk.h"
#include	<abfglobs.h>
#include	"erab.h"

/**
** Name:	userrun.c -	ABF Run Named Application Module.
**
** Description:
**	Contains the routine used to run an application by name either directly
**	on entry into ABF (flagged on the command-line) or from the ABF catalogs
**	frame.  Defines:
**
**	IIABrun()	run named application.
**
** History:
**	Revision 6.2  89/02  wong
**	Rewrote to call 'IIAMappFetch()'.
**	89/03  wong  Check for default start.  JupBug #5001 (and #5056.)
**
**	Revision 2.0  82/04  joe
**	Initial revision.
**	23-Aug-2009 (kschendel) 121804
**	    Need abfglobs.h to satisfy gcc 4.3.
**/

/*{
** Name:	IIABrun() -		Run Named Application.
**
** Description:
**	Run an application given the name, possibly the ID, and possibly a frame
**	name by fetching it from the DBMS and then running it.
**
** Input:
**	appname	{char *}  The name of the application to run.
**	id	{OOID}  The object ID of the application (optional.)
**	fname	{char *}  The name of the frame to run (optional.)
**
** Called by:
**	'IIABabf()', 'IIABcatalogs()'.
**
** History:
**	Written 4/15/82 (jrc)
**	28-oct-88 (mgw) Bug #2874
**		Don't call IIABsdirSet() or IIABcdirCreate() here. They'll
**		get called by IIABgcatGet() if the application is cataloged.
**	02/89 (jhw)  Modified to call new 'IIABappFetch()'.
**	03/89 (jhw)  Check for default start.  JupBug #5001 (and #5056.)
*/

VOID
IIABrun ( appname, id, fname )
char	*appname;
OOID	id;
char	*fname;
{
	APPL		app;

	app.ooid = id;
	app.data.inDatabase = FALSE;

	if ( IIABappFetch(&app, appname, OC_UNDEFINED, TRUE) == OK  &&
			IIABchkSrcDir(app.source, FALSE) )
	{
		OOID		start_class = OC_UNDEFINED;
		ABLNKTST	test_image;

		if ( fname == NULL || *fname == EOS )
		{ /* check for default start */
			fname = app.start_name;
			start_class = ( app.start_proc || app.batch )
					? OC_UNPROC : OC_APPLFRAME;
		}

		test_image.user_link = FALSE;
		test_image.one_frame = NULL;
		test_image.plus_tree = FALSE;

		if ( IIABarunApp(&app, &test_image, fname, start_class) != OK )
		{
			IIUGerr(E_AB0127_NoRun, UG_ERR_ERROR, 0);
		}
		iiabRmTest(&test_image);
		IIAMappFree(&app);
	}
}
