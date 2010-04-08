/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** History:
**      07-Sep-1998 (fanra01)
**          Correct incomplete last line.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
#ifndef WSS_PROFILE_INCLUDE
#define WSS_PROFILE_INCLUDE

#include <ddfcom.h>

typedef struct __WSS_ID_LIST 
{
	u_i4				 id;
	struct __WSS_ID_LIST *previous;
	struct __WSS_ID_LIST *next;
} WSS_ID_LIST;

typedef struct __WSS_PROFILE
{
	u_i4		id;
	char		*name;
	i4		flags;
	i4		timeout;
	u_i4		dbuser_id;
	WSS_ID_LIST	*dbs;
	WSS_ID_LIST	*roles;
	struct __WSS_PROFILE	*next;
} WSS_PROFILE, *WSS_PPROFILE;

GSTATUS 
WSSGetProfile(
	char* name, 
	u_i4* id);

GSTATUS 
WSSCreateProfile (
	char *name, 
	u_i4 dbuser_id, 
	i4 flags,
	i4 timeout,
	u_i4*	id);

GSTATUS 
WSSUpdateProfile (
	u_i4	id,
	char*	name, 
	u_i4	dbuser_id, 
	i4 flags,
	i4 timeout);

GSTATUS 
WSSSelectProfile (
	PTR		*cursor,
	u_i4	**id,
	char	**name, 
	u_i4	**dbuser_id,
	i4	**flags,
	i4	**timeout);

GSTATUS 
WSSRetrieveProfile (
	u_i4	id,
	char	**name, 
	u_i4	**dbuser_id,
	i4	**flags,
	i4	**timeout);

GSTATUS 
WSSDeleteProfile (
	u_i4 profile_id);

GSTATUS 
WSSCreateProfileRole (
	u_i4 profile_id, 
	u_i4 role_id);

GSTATUS 
WSSProfilesUseRole(
	u_i4	role_id);

GSTATUS 
WSSSelectProfileRole (
	PTR		*cursor,
	u_i4	profile_id, 
	u_i4	**role_id,
	char	**role_name);

GSTATUS 
WSSDeleteProfileRole (
	u_i4	profile_id, 
	u_i4	role_id);

GSTATUS 
WSSCreateProfileDB (
	u_i4	profile_id, 
	u_i4	db_id);

GSTATUS 
WSSProfilesUseDB(
	u_i4	db_id);

GSTATUS 
WSSSelectProfileDB (
	PTR		*cursor,
	u_i4	profile_id, 
	u_i4	**db_id,
	char	**db_name);

GSTATUS 
WSSDeleteProfileDB (
	u_i4	profile_id, 
	u_i4	db_id);

GSTATUS 
WSSProfileInitialize ();

GSTATUS 
WSSProfileTerminate ();

#endif
