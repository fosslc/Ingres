/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** History:
**      11-Sep-1998 (fanra01)
**          Corrected incomplete last line.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
#ifndef WSS_APP_INCLUDE
#define WSS_APP_INCLUDE

#include <ddfcom.h>

GSTATUS 
WSSAppInitialize ();

GSTATUS 
WSSAppTerminate ();

GSTATUS 
WSSAppLoad (
	u_i4 id,
	char* name);

GSTATUS 
WSSAppCreate (
	u_i4 *id,
	char* name);

GSTATUS
WSSAppUpdate (
	u_i4	id,
	char*	name);

GSTATUS
WSSAppDelete (
	u_i4	id);

GSTATUS
WSSAppExist (
	char *name,
	bool *result);

GSTATUS 
WSSAppBrowse (
	PTR *cursor,
	u_i4	**id,
	char	**name);

GSTATUS 
WSSAppRetrieve (
	u_i4	id,
	char	**name);

#endif
