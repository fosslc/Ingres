/******************************************************************************
**
** Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

/*
** Name: dbmethod.h - File header for driver methods
**
** Description:
**  This file contains every structures and defien available to write a driver
**  This file contains every function declaration that must appear in a driver
**
** History: 
**      02-mar-98 (marol01)
**          Created
**      20-Nov-1998 (fanra01)
**          Add isunique flag for unique fields.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      16-Mar-2001 (fanra01)
**          Bug 104298
**          Add maxrowset and maxretsize parameters to the prepare prototype
**          and define default values.
**/

#ifndef DBMETHOD_INCLUDED
#define DBMETHOD_INCLUDED

#include <ddfcom.h>
#include <erddf.h>

#define DEFAULT_ROW_SET         1
#define DEFAULT_SET_SIZE        65536
#define DEFAULT_MIN_ROW_SET     256

typedef struct __DB_PROPERTY 
{
	char					*name;
	bool					key;
	char					type;
	u_i4					length;
	bool					isnull;
	bool					isunique;
	struct __DB_PROPERTY	*next;
} DB_PROPERTY, *PDB_PROPERTY;

typedef struct DB_QUERY 
{
	PTR		SpecificInfo;
} DB_QUERY, *PDB_QUERY;

typedef struct DB_SESSION 
{
	PTR		SpecificInfo;
} DB_SESSION, *PDB_SESSION;

typedef PTR			DB_VALUE;
typedef DB_VALUE	*PDB_VALUE;

typedef u_i4		PROP_NUMBER;

/******** type *************/
#define DDF_DB_INT	 ((char)'d')
#define DDF_DB_FLOAT ((char)'f')
#define DDF_DB_TEXT	 ((char)'s')
#define DDF_DB_DATE	 ((char)'t')
#define DDF_DB_BLOB	 ((char)'l')
#define DDF_DB_BYTE	 ((char)'b')

GSTATUS
DBDriverName (
	char*			*name);

GSTATUS
PDBConnect (
	PDB_SESSION		session, 
	char			*node, 
	char			*dbname, 
	char			*svr_class, 
	char			*user, 
	char			*password,
	i4	timeout);

GSTATUS 
PDBCommit (
	PDB_SESSION		session);

GSTATUS 
PDBRollback(
	PDB_SESSION		session);

GSTATUS 
PDBDisconnect(
	PDB_SESSION		session);

GSTATUS 
PDBExecute(
	PDB_SESSION		session, 
	PDB_QUERY		query);

GSTATUS 
PDBPrepare(
    PDB_QUERY   query,
    char        *statement,
    i4          maxRowset,
    i4          maxSetsize );

GSTATUS 
PDBGetStatement(
	PDB_QUERY		query, 
	char*			*statement);

GSTATUS 
PDBParams(
	PDB_QUERY		query, 
	char			*paramdef, 
	...);

GSTATUS 
PDBParamsFromTab(
	PDB_QUERY		query, 
	PDB_VALUE		values);

GSTATUS 
PDBNumberOfProperties(
	PDB_QUERY		query, 
	PROP_NUMBER		*properties);

GSTATUS 
PDBName(
	PDB_QUERY		query, 
	PROP_NUMBER		property, 
	char			**data);

GSTATUS 
PDBType(
	PDB_QUERY		query, 
	PROP_NUMBER		property, 
	char			*type);

GSTATUS 
PDBText(
	PDB_QUERY		query, 
	PROP_NUMBER		property, 
	char			**data);

GSTATUS 
PDBInteger(
	PDB_QUERY		query, 
	PROP_NUMBER		property, 
	i4				**data);

GSTATUS 
PDBFloat(
	PDB_QUERY		query, 
	PROP_NUMBER		property, 
	float			**data);

GSTATUS 
PDBDate(
	PDB_QUERY		query, 
	PROP_NUMBER		property, 
	char			**data);

GSTATUS 
PDBBlob(
	PDB_QUERY		query, 
	PROP_NUMBER		property, 
	LOCATION		*loc);

GSTATUS 
PDBNext(
	PDB_QUERY		query, 
	bool			*exist);

GSTATUS 
PDBClose(
	PDB_QUERY		query,
	u_i4 *count);

GSTATUS 
PDBRelease(
	PDB_QUERY		query);

GSTATUS 
PDBRepInsert(
	char			*name, 
	PDB_PROPERTY	properties, 
	PDB_QUERY		query);

GSTATUS 
PDBRepUpdate (
	char			*name, 
	PDB_PROPERTY	properties, 
	PDB_QUERY		query);

GSTATUS 
PDBRepDelete(
	char			*name, 
	PDB_PROPERTY	properties, 
	PDB_QUERY		query);

GSTATUS 
PDBRepSelect(
	char			*name, 
	PDB_PROPERTY	properties, 
	PDB_QUERY		query);

GSTATUS 
PDBRepKeySelect(
	char			*name, 
	PDB_PROPERTY	properties, 
	PDB_QUERY		query);

GSTATUS 
PDBRepObjects(
	PDB_SESSION		session, 
	u_i4			module, 
	PDB_QUERY		query);

GSTATUS 
PDBRepProperties	(
	PDB_SESSION		session, 
	u_i4			module, 
	PDB_QUERY		query);

GSTATUS 
PDBRepStatements	(
	PDB_SESSION		session, 
	u_i4			module, 
	PDB_QUERY		query);

GSTATUS 
PDBGenerate(
	PDB_SESSION		session, 
	char			*name, 
	PDB_PROPERTY	properties);

#endif
