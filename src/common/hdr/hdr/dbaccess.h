/******************************************************************************
**
** Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

/**
** Name: dbaccess.h - Database Driver Connection
**
** Description:
**  This file contains components of the DDC needed to user a generic driver
**
** History: 
**      02-mar-98 (marol01)
**          Created
**	29-Jul-1998 (fanra01)
**	    Changed case of include filename.
**	    Removed forward reference typedef and updated DB_CACHED_SESSION
**	    structure.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      16-Mar-2001 (fanra01)
**          Bug 104298
**          Add rowset and setsize to PDBPrepare function declaration.
**      04-Apr-2001 (fanra01)
**          Bug 104422
**          Add dbuser and connflags fields for further differentiation of
**          dbms sessions.
**/

#ifndef DBACCESS_INCLUDED
#define DBACCESS_INCLUDED

#include <dbmethod.h>
#include <ddfhash.h>
#include <ddfsem.h>

/*
** Name: DB_CACHED_SESSION
**
** Description:
**      Structure holds database connection information about available
**      database connections to allow connection reuse.
**
** Fields:
**
**      driver          Driver library index
**      set             Pointer to the cached session set
**      used            In-use flag
**      session         Database session information
**      timeout         Timeout value
**      timeout_end     Time of next timeout
**      dbuser          Pointer to named owner of session
**      connflags       Properties of connection. Reserved for future use
**      previous        Previous entry in list of sessions about to timeout
**      next            Next entry in the list of session about to timeout
**      previous_in_set Previous session entry for this database
**      next_in_set     Next session entry for this database
**
** History:
**      04-Apr-2001 (fanra01)
**          Add dbuser and connflags fields.
*/
typedef struct __DB_CACHED_SESSION 
{
    u_i4                            driver;
    struct __DB_CACHED_SESSION_SET* set;
    bool                            used;
    DB_SESSION                      session;
    i4                              timeout;
    i4                              timeout_end;
    char*                           dbuser;         /* session connected as */
    u_i4                            connflags;      /* connection properties*/

    struct __DB_CACHED_SESSION*     previous;
    struct __DB_CACHED_SESSION*     next;

    struct __DB_CACHED_SESSION*     previous_in_set;
    struct __DB_CACHED_SESSION*     next_in_set;
} DB_CACHED_SESSION, *PDB_CACHED_SESSION;

typedef struct __DB_CACHED_SESSION_SET 
{
    char    *name;
    PDB_CACHED_SESSION sessions;
} DB_CACHED_SESSION_SET, *PDB_CACHED_SESSION_SET;

typedef struct __DB_ACCESS_DEF 
{
	char				*name;
	PTR					handle;
	DDFHASHTABLE	cached_sessions;

	GSTATUS ( *PDBDriverName )		(char* *name);

	GSTATUS ( *PDBConnect )				(	PDB_SESSION session, 
																	char *node, 
																	char *dbname, 
																	char *svrClass, 
																  char *user, 
																	char *password,
																	i4	timeout);

	GSTATUS ( *PDBCommit )				(	PDB_SESSION session);

	GSTATUS ( *PDBRollback )			(	PDB_SESSION session);

	GSTATUS ( *PDBDisconnect )		(	PDB_SESSION session);

	GSTATUS ( *PDBExecute )				(	PDB_SESSION session, 
																	PDB_QUERY qry);

	GSTATUS ( *PDBPrepare )( PDB_QUERY qry,
	    char *statement, i4 rowset, i4 setsize );

	GSTATUS ( *PDBGetStatement	)	( PDB_QUERY qry, 
																	char* *statement);

	GSTATUS ( *PDBParams )				( PDB_QUERY qry, 
																	char *paramdef, 
																	...);

	GSTATUS ( *PDBParamsFromTab )	(	PDB_QUERY qry, 
																	PDB_VALUE values);

	GSTATUS ( *PDBNumberOfProperties )	
																( PDB_QUERY qry, 
																	PROP_NUMBER* properties);

	GSTATUS ( *PDBName )					(	PDB_QUERY qry, 
																	PROP_NUMBER property, 
																	char **data);

	GSTATUS ( *PDBType )					(	PDB_QUERY qry, 
																	PROP_NUMBER property, 
																	char *type);

	GSTATUS ( *PDBText )					(	PDB_QUERY qry, 
																	PROP_NUMBER property, 
																	char **data);

	GSTATUS ( *PDBInteger )				(	PDB_QUERY qry, 
																	PROP_NUMBER property, 
																	i4 **data);

	GSTATUS ( *PDBFloat )					(	PDB_QUERY qry, 
																	PROP_NUMBER property, 
																	float **data);

	GSTATUS ( *PDBDate )					(	PDB_QUERY qry, 
																	PROP_NUMBER property, 
																	char **data);

	GSTATUS ( *PDBBlob )					(	PDB_QUERY qry, 
																	PROP_NUMBER property, 
																	LOCATION *loc);

	GSTATUS ( *PDBNext )					(	PDB_QUERY qry, 
																	bool *exist);

	GSTATUS ( *PDBClose )					(	PDB_QUERY qry, u_i4 *count);

	GSTATUS ( *PDBRelease )				(	PDB_QUERY qry);

	GSTATUS ( *PDBRepInsert )			(	char *name, 
																	PDB_PROPERTY properties, 
																	PDB_QUERY qry);

	GSTATUS ( *PDBRepUpdate )			(	char *name, 
																	PDB_PROPERTY properties, 
																	PDB_QUERY qry);

	GSTATUS ( *PDBRepDelete )			(	char *name, 
																	PDB_PROPERTY properties, 
																	PDB_QUERY qry);

	GSTATUS ( *PDBRepSelect )			(	char *name, 
																	PDB_PROPERTY properties, 
																	PDB_QUERY qry);

	GSTATUS ( *PDBRepKeySelect)		(	char *name, 
																	PDB_PROPERTY properties, 
																	PDB_QUERY qry);

	GSTATUS ( *PDBRepObjects)			(	PDB_SESSION session, 
																	u_i4 module, 
																	PDB_QUERY qry);

	GSTATUS ( *PDBRepProperties)	(	PDB_SESSION session, 
																	u_i4 module, 
																	PDB_QUERY qry);

	GSTATUS ( *PDBRepStatements)	(	PDB_SESSION session, 
																	u_i4 module, 
																	PDB_QUERY qry);

	GSTATUS ( *PDBGenerate)				(	PDB_SESSION session, 
																	char *name, 
																	PDB_PROPERTY properties);

} DB_ACCESS_DEF, *PDB_ACCESS_DEF;

typedef u_i4 DB_OBJECT_NUMBER;

PDB_ACCESS_DEF DDFGetDriver(u_i4 number);

GSTATUS 
DBCachedConnect	(
	u_i4				driver,
	char*				node,
	char*				dbname,
	char*				svrClass,
	char*				flag,
	char*				user,
	char*				password,
	i4			timeout,
	PDB_CACHED_SESSION	*session);

GSTATUS
DBCachedDisconnect (
	PDB_CACHED_SESSION  *session);

GSTATUS
DBRemove(
	PDB_CACHED_SESSION cache_session,
	bool			   checkAdd);

GSTATUS
DBCleanTimeout();

GSTATUS
DBBrowse(
	PTR		*cursor,
	PDB_CACHED_SESSION *conn);

#define DBDriverName(X)						DDFGetDriver(X)->PDBDriverName
#define DBConnect(X)							DDFGetDriver(X)->PDBConnect
#define DBDisconnect(X)						DDFGetDriver(X)->PDBDisconnect
#define DBPrepare(X)							DDFGetDriver(X)->PDBPrepare
#define DBGetStatement(X)					DDFGetDriver(X)->PDBGetStatement
#define DBParams(X)								DDFGetDriver(X)->PDBParams
#define DBParamsFromTab(X)				DDFGetDriver(X)->PDBParamsFromTab
#define DBExecute(X)							DDFGetDriver(X)->PDBExecute
#define DBNumberOfProperties(X)		DDFGetDriver(X)->PDBNumberOfProperties
#define DBName(X)									DDFGetDriver(X)->PDBName
#define DBType(X)									DDFGetDriver(X)->PDBType
#define DBText(X)									DDFGetDriver(X)->PDBText
#define DBInteger(X)							DDFGetDriver(X)->PDBInteger
#define DBFloat(X)								DDFGetDriver(X)->PDBFloat
#define DBDate(X)									DDFGetDriver(X)->PDBDate
#define DBNext(X)									DDFGetDriver(X)->PDBNext
#define DBBlob(X)									DDFGetDriver(X)->PDBBlob
#define DBClose(X)								DDFGetDriver(X)->PDBClose
#define DBRelease(X)							DDFGetDriver(X)->PDBRelease
#define DBCommit(X)								DDFGetDriver(X)->PDBCommit
#define DBRollback(X)							DDFGetDriver(X)->PDBRollback

#define DBRepInsert(X)						DDFGetDriver(X)->PDBRepInsert	
#define DBRepUpdate(X)						DDFGetDriver(X)->PDBRepUpdate
#define DBRepDelete(X)						DDFGetDriver(X)->PDBRepDelete
#define DBRepSelect(X)						DDFGetDriver(X)->PDBRepSelect
#define DBRepKeySelect(X)					DDFGetDriver(X)->PDBRepKeySelect
#define DBRepObjects(X)						DDFGetDriver(X)->PDBRepObjects
#define DBRepProperties(X)				DDFGetDriver(X)->PDBRepProperties
#define DBRepStatements(X)				DDFGetDriver(X)->PDBRepStatements
#define DBGenerate(X)							DDFGetDriver(X)->PDBGenerate

GSTATUS
DDFInitialize();

GSTATUS
DDFTerminate();

GSTATUS 
DriverLoad(
	char *name, 
	u_i4 *number);

#endif
