/******************************************************************************
**
** Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

/*
** Name: ddginit.h - Data Dictionary Generation
**
** Description:
**  This file contains declarations of functions which use to initialize 
**	the DDG module
**
** History: 
**      02-mar-98 (marol01)
**          Created
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

#ifndef INIT_DDG_INCLUDED
#define INIT_DDG_INCLUDED

#include <dbaccess.h>
#include <ddfsem.h>

typedef u_i4 DDG_OBJECT_NUMBER;

typedef struct __DDG_OBJECT 
{
	char			*name;
	PDB_PROPERTY	properties;
	u_i4			prop_max;
	u_i4			key_max;
} DDG_OBJECT, *PDDG_OBJECT;

typedef struct __DDG_STATEMENT 
{
	char			*stmt;
	char*			params;
	u_i4			prop_max;
} DDG_STATEMENT, *PDDG_STATEMENT;

typedef struct __DDG_DESCR 
{
	u_i4			dbtype;
	char			*node;
	char			*dbname;
	char			*svrClass;
	PDDG_OBJECT		objects;
	u_i4			object_max;
	PDDG_STATEMENT	statements;
	u_i4			stmt_max;
} DDG_DESCR, *PDDG_DESCR;

GSTATUS 
DDGInitialize(
	u_i4 module, 
	char *name, 
	char *node, 
	char *dbname, 
	char *svrClass, 
	char *user_name,
	char *password,
	PDDG_DESCR descr);

GSTATUS 
DDGTerminate(
	PDDG_DESCR descr);

#endif
