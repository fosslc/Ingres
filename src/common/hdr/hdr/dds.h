/******************************************************************************
**
** Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

/*
** Name: dds.h - Data Dictionary Security
**
** Description:
**  This file contains declaration of functions available API for DDS.
**	This list of function corresponding to the DDS API.
**
** History: 
**      02-mar-98 (marol01)
**          Created
**      4-mar-98 (marol01)
**          deleted a PDDG_DESCR parameter of the DDSInitialize function
**      28-May-98 (fanra01)
**          Add timeout parameter to prototype.
**      14-Dec-1998 (fanra01)
**          Add DDSGetDBUserInfo function prototype.
**      07-Jan-1999 (fanra01)
**          Change prototypes to include authtype.
**      05-May-2000 (fanra01)
**          Bug 101346
**          Add prototype for DDSMemCheckRightRef.
**          Modified prototypes for DDSAssignUserRight and DDSDeleteUserRight.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      07-Nov-2000 (wansh01)
**          Changed prototype DDSMemCheckRightRef to DDSCheckRightRef. 
**/

#ifndef DDS_INCLUDED
#define DDS_INCLUDED

#define CHECK_RIGHT(X, Y) ((X & Y) ? TRUE : FALSE)

GSTATUS 
DDSInitialize (
	char*	dllname,
	char*	node,
	char*	dictionary_name,
	char*	svrClass,
	char*	user_name,
	char* password,
	u_i4	right_hash, 
		/* defines the hash table size for concurent rights */
	u_i4	user_hash);
		/* defines the hash table size for users */

GSTATUS 
DDSTerminate ();

GSTATUS 
DDSCheckUser (
	char *name, 
	char *password, 
	u_i4 *user_id);

GSTATUS 
DDSCheckUserFlag (
	u_i4 user_id,
	i4 flag,
	bool *result);

GSTATUS 
DDSRequestUser (
	u_i4 user_id);

GSTATUS 
DDSReleaseUser (
	u_i4 user_id);

GSTATUS 
DDSCheckRight (
	u_i4	user, 
	char*	resource, 
	u_i4	right,
	bool	*result);

GSTATUS
DDSCheckRightRef(
    u_i4    user,
    char*   resource,
    i4      right,
    i4*     refs,
    bool    *result );

GSTATUS 
DDSCreateDBUser (
	char *alias,
	char *name, 
	char *password, 
	char *comment, 
	u_i4 *id);

GSTATUS 
DDSUpdateDBUser (
	u_i4 id, 
	char *alias,
	char *name, 
	char *password, 
	char *comment);

GSTATUS 
DDSDeleteDBUser (
	u_i4 id);

GSTATUS 
DDSOpenDBUser (PTR		*cursor);

GSTATUS 
DDSRetrieveDBUser (
	u_i4	id,
	char	**alias,
	char	**name, 
	char	**password, 
	char	**comment);

GSTATUS 
DDSGetDBUser (
	PTR		*cursor,
	u_i4	**id, 
	char	**alias,
	char	**name, 
	char	**password, 
	char	**comment);

GSTATUS 
DDSCloseDBUser (PTR		*cursor);

GSTATUS 
DDSCreateRole (
	char *name, 
	char *comment, 
	u_i4 *id);

GSTATUS 
DDSUpdateRole (
	u_i4 id, 
	char *name, 
	char *comment);

GSTATUS 
DDSDeleteRole (
	u_i4 id);

GSTATUS 
DDSOpenRole (PTR		*cursor);

GSTATUS 
DDSRetrieveRole (
	u_i4	id,
	char	**name, 
	char	**comment);

GSTATUS 
DDSGetRole (
	PTR		*cursor,
	u_i4	**id, 
	char	**name, 
	char	**comment);

GSTATUS 
DDSCloseRole (PTR		*cursor);

GSTATUS 
DDSCreateAssUserRole (
	u_i4 user_id, 
	u_i4 role_id);

GSTATUS 
DDSOpenUserRole (PTR *cursor);

GSTATUS 
DDSGetUserRole (
	PTR		*cursor,
	u_i4	user_id, 
	u_i4	**role_id,
	char	**role_name);

GSTATUS 
DDSCloseUserRole (PTR *cursor);

GSTATUS 
DDSDeleteAssUserRole (
	u_i4 user_id, 
	u_i4 role_id);

GSTATUS 
DDSAssignRoleRight (
	u_i4 role, 
	char* resource, 
	u_i4 right);

GSTATUS 
DDSDeleteRoleRight (
	u_i4 role_id, 
	char* resource, 
	u_i4 right);

GSTATUS 
DDSCreateUser (
	char *name, 
	char *password, 
	u_i4 dbuser_id, 
	char *comment, 
	i4 flag,
	i4 timeout,
        i4 authtype,
	u_i4 *id);

GSTATUS 
DDSUpdateUser (
	u_i4 id, 
	char *name, 
	char *password, 
	u_i4 dbuser_id, 
	char *comment,
	i4 flag,
	i4 timeout,
        i4 authtype);

GSTATUS 
DDSDeleteUser (
	u_i4 id);

GSTATUS 
DDSOpenUser (PTR		*cursor);

GSTATUS 
DDSRetrieveUser (
	u_i4	id, 
	char	**name, 
	char	**password, 
	u_i4	**dbuser_id, 
	char	**comment,
	i4	**flag,
	i4	**timeout,
        i4 **authtype);

GSTATUS 
DDSGetUser (
	PTR		*cursor,
	u_i4	**id, 
	char	**name, 
	char	**password, 
	u_i4	**dbuser_id, 
	char	**comment,
	i4 **flag,
	i4 **timeout,
        i4 **authtype);

GSTATUS 
DDSCloseUser (PTR		*cursor);

GSTATUS 
DDSAssignUserRight(
    u_i4 user,
    char* resource,
    u_i4 right,
    i4 refs,
    bool* created );

GSTATUS 
DDSDeleteUserRight(
    u_i4 user,
    char* resource,
    u_i4 right,
    i4 refs,
    bool* deleted );

GSTATUS 
DDSCreateDB (
	char *name, 
	char *dbname, 
	u_i4 dbuser_id, 
	i4 flag,
	char *comment,
	u_i4 *id);

GSTATUS 
DDSUpdateDB (
	u_i4 id, 
	char *name, 
	char *dbname, 
	u_i4 dbuser_id,
	i4 flag,
	char *comment);

GSTATUS 
DDSDeleteDB (
	u_i4 id);

GSTATUS 
DDSOpenDB (PTR		*cursor);

GSTATUS 
DDSRetrieveDB (
	u_i4	id, 
	char	**name, 
	char	**dbname, 
	u_i4	**dbuser_id,
	i4 **flag,
	char	**comment);

GSTATUS 
DDSGetDB (
	PTR		*cursor,
	u_i4	**id, 
	char	**name, 
	char	**dbname, 
	u_i4	**dbuser_id,
	i4 **flag,
	char	**comment);

GSTATUS 
DDSCloseDB (PTR		*cursor);

GSTATUS 
DDSCreateAssUserDB (
	u_i4 user_id, 
	u_i4 db_id);

GSTATUS 
DDSOpenUserDB (PTR *cursor);

GSTATUS 
DDSGetUserDB (
	PTR		*cursor,
	u_i4	user_id, 
	u_i4	**db_id,
	char	**db_name);

GSTATUS 
DDSCloseUserDB (PTR *cursor);

GSTATUS 
DDSDeleteAssUserDB (
	u_i4 user_id, 
	u_i4 db_id);

GSTATUS 
DDSGetDBForUser (
	u_i4	user_id, 
	char	*name, 
	char	**dbname,
	char	**user, 
	char	**password,
	i4 *flag);

GSTATUS 
DDSOpenRoleResource (PTR *cursor, char* resource);

GSTATUS 
DDSGetRoleResource (
	PTR		*cursor,
	u_i4	*id,
	i4	*right);

GSTATUS 
DDSCloseRoleResource (PTR *cursor);

GSTATUS 
DDSOpenUserResource (PTR *cursor, 	char*	resource);

GSTATUS 
DDSGetUserResource (
	PTR		*cursor,
	u_i4	*id,
	i4	*right);

GSTATUS 
DDSCloseUserResource (PTR *cursor);

GSTATUS 
DDSGetDBUserName(u_i4 id, char** name);

GSTATUS 
DDSGetDBName(u_i4 id, char** name);

GSTATUS 
DDSGetUserName(u_i4 id, char** name);

GSTATUS 
DDSGetUserTimeout(u_i4  id, i4* timeout);

GSTATUS 
DDSGetRoleName(u_i4 id, char** name);

GSTATUS
DDSDeleteResource(char* name);

GSTATUS
DDSGetDBUserPassword(
	char*	alias,
	char*	*dbuser,
	char*	*password);

GSTATUS
DDSGetDBUserInfo(
	char*	*alias,
	char*	*dbuser,
	char*	*password);

#endif
