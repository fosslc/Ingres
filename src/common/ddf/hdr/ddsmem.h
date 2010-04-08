/******************************************************************************
**
** Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

/*
** Name: ddsmem.h - Data Dictionary Security
**
** Description:
**  This file contains declaration of functions will allow to manage \
**	DDS information in memory
**
** History: 
**      02-mar-98 (marol01)
**          Created
**	13-may-1998 (canor01)
**	    Add DDS_PASSWORD_LENGTH.
**      28-May-98 (fanra01)
**          Add timeout parameter to prototype.
**	29-Jul-1998 (fanra01)
**	    Add DDSMemGetUserTimeout prototype.
**      14-Dec-1998 (fanra01)
**          Add prototype for DDSMemGetDBUserInfo.
**      07-Jan-1999 (fanra01)
**          Prototype changes and additions for authtype.
**      06-aug-1999
**          Replace nat and longnat with i4.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	27-Jul-2004 (hanje04)
**	    Add DDS_USER_LENGTH.
**	    Prototype DDSencode
**/

#ifndef DDS__INCLUDED
#define DDS__INCLUDED

#include <ddfcom.h>

typedef struct __DDS_SCANNER
{
	char*	name;
	PTR		current;
} DDS_SCANNER;

#define RIGHT_TYPE_USER	1
#define RIGHT_TYPE_ROLE	2

#define DDS_DEFAULT_DBUSER				10
#define DDS_DEFAULT_USER				100
#define DDS_DEFAULT_ROLE				10
#define DDS_DEFAULT_DB					10

#define DDS_OBJ_DBUSER					0
#define DDS_OBJ_USER					1
#define DDS_OBJ_ROLE					2
#define DDS_OBJ_ASS_USER_ROLE			3
#define DDS_OBJ_USER_RIGHT				4
#define DDS_OBJ_ROLE_RIGHT				5
#define DDS_OBJ_DB						6
#define DDS_OBJ_ASS_USER_DB				7

#define DDS_DEL_RES_USER				0
#define DDS_DEL_RES_ROLE				1
#define DDS_DEL_USER_RIGHT				2
#define DDS_DEL_ROLE_RIGHT				3
#define DDS_DEL_USER_ROLE				4
#define DDS_DEL_USER_DB					5

#define DDS_PASSWORD_LENGTH				64 /* from gcpasswd.h */
#define DDS_USER_LENGTH					32 /* from gcaint.h */

GSTATUS
DDSMemCheckUser (
	char *name, 
	char *password, 
	u_i4 *user_id);

GSTATUS 
DDSMemVerifyUserPass (
	u_i4 user_id,
	char *password,
	bool *result);

GSTATUS 
DDSMemVerifyDBUserPass (
	u_i4 user_id,
	char *password,
	bool *result);

GSTATUS
DDSMemRequestUser (
	u_i4 user_id);

GSTATUS 
DDSMemCheckUserFlag (
	u_i4 user_id,
	i4 flag,
	bool *result);

GSTATUS 
DDSMemCheckRight(
	u_i4	user_id, 
	char*	resource, 
	i4 right,
	bool	*result);

GSTATUS 
DDSMemCheckRightRef(
    u_i4    user,
    char*   resource,
    i4      right,
    i4*     refs,
    bool    *result );

GSTATUS
DDSMemReleaseUser (
	u_i4 user_id);

GSTATUS 
DDSPerformDBUserId (
	u_i4 *id);

GSTATUS 
DDSMemCreateDBUser (
	u_i4 id, 
	char *alias,
	char *name, 
	char *password);

GSTATUS 
DDSMemUpdateDBUser (
	u_i4 id, 
	char *alias,
	char *name, 
	char *password);

GSTATUS 
DDSMemDeleteDBUser (
	u_i4 id);

GSTATUS 
DDSMemGetDBForUser(
	u_i4	user_id,
	char	*name,
	char	**dbname, 
	char	**user, 
	char	**password, 
	i4      *flags);

GSTATUS 
DDSPerformUserId (
	u_i4 *id);

GSTATUS 
DDSMemCreateUser (
	u_i4 id, 
	char *name, 
	char *password, 
	i4      flags,
	i4     	tineout,
	u_i4 dbuser_id,
        i4      authtype);

GSTATUS 
DDSMemUpdateUser (
	u_i4 id, 
	char *name, 
	char *password, 
	i4 flags,
	i4 timeout,
	u_i4 dbuser_id,
        i4 authtype);
/*
** Name: ddsmem.h
**
** Description:
**      Prototypes for the memory access functions.
**
** History:
**      05-May-2000 (fanra01)
**          Add history.
**          Bug 101346
**          Update prototypes for DDSMemAssignUserRight and
**          DDSMemDeleteUserRight.
*/
GSTATUS 
DDSMemDeleteUser (
	u_i4 id);

GSTATUS 
DDSMemAssignUserRight (
    u_i4 user,
    char* resource,
    i4 *right,
    i4 *refs,
    bool *created);

GSTATUS 
DDSMemDeleteUserRight (
    u_i4 user,
    char* resource,
    i4 *right,
    i4 *refs,
    bool *deleted);

GSTATUS 
DDSPerformRoleId (
	u_i4 *id);

GSTATUS 
DDSMemCreateRole (
	u_i4 id, 
	char *role_name);

GSTATUS 
DDSMemUpdateRole (
	u_i4 id, 
	char *role_name);

GSTATUS 
DDSMemDeleteRole (
	u_i4 id);

GSTATUS 
DDSMemAssignRoleRight (
	u_i4 role, 
	char* resource, 
	i4      *right,
	bool	*created);

GSTATUS 
DDSMemDeleteRoleRight (
	u_i4 user_id, 
	char* resource, 
	i4      *right,
	bool	*deleted);

GSTATUS 
DDSMemCreateAssUserRole (
	u_i4 user_id, 
	u_i4 role_id);

GSTATUS 
DDSMemCheckAssUserRole (
	u_i4 user_id, 
	u_i4 role_id);

GSTATUS 
DDSMemSelectAssUserRole (
	PTR		*cursor,
	u_i4	user_id, 
	u_i4	**role_id,
	char	**role_name);

GSTATUS 
DDSMemDeleteAssUserRole (
	u_i4 user_id, 
	u_i4 role_id);

GSTATUS 
DDSPerformDBId (
	u_i4 *id);

GSTATUS 
DDSMemCreateDB (
	u_i4 id, 
	char *name, 
	char *dbname, 
	u_i4 dbuser_id,
	i4      flags);

GSTATUS 
DDSMemUpdateDB (
	u_i4 id, 
	char *name, 
	char *dbname, 
	u_i4 dbuser_id,
	i4      flags);

GSTATUS 
DDSMemDeleteDB (
	u_i4 id);

GSTATUS 
DDSMemCheckAssUserDB (
	u_i4 user_id, 
	u_i4 db_id);

GSTATUS 
DDSMemCreateAssUserDB (
	u_i4 user_id, 
	u_i4 db_id);

GSTATUS 
DDSMemSelectAssUserDB (
	PTR		*cursor,
	u_i4	user_id, 
	u_i4	**db_id,
	char	**db_name);

GSTATUS 
DDSMemDeleteAssUserDB (
	u_i4 user_id, 
	u_i4 db_id);

GSTATUS 
DDSMemInitialize (
	u_i4	concurent_rights, /* defines the hash table size for concurent rights */
	u_i4	max_user);		  /* defines the hash table size for users */

GSTATUS 
DDSMemTerminate ();

GSTATUS 
DDSMemSelectResource (
	PTR		*cursor,
	u_i4	type,
	char*	resource,
	u_i4	*id,
	i4     	*flag);

GSTATUS
DDSMemGetDBUserName(
	u_i4	id,
	char**	name);

GSTATUS
DDSMemGetDBName(
	u_i4	id,
	char**	name);

GSTATUS
DDSMemGetRoleName(
	u_i4	id,
	char**	name);

GSTATUS
DDSMemGetUserName(
	u_i4	id,
	char**	name);

GSTATUS
DDSMemGetUserTimeout(
        u_i4 id,
        i4     * timeout);

GSTATUS 
DDSMemDeleteResource(
	char*	resource);

GSTATUS 
DDSMemGetUserTimeout(
	u_i4 id, 
	i4     * timeout);

GSTATUS 
DDSMemGetDBUserPassword(
	char*	alias, 
	char*	*dbuser,
	char* *password);

GSTATUS
DDSMemGetDBUserInfo(
    char*   dbuser,
    char*   *alias,
    char*   *password);

GSTATUS
DDSMemGetAuthType(u_i4 id, i4* authtype);

STATUS
DDSencode(
    char    *key,
    char    *str,
    char    *buff);
#endif
