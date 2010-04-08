/******************************************************************************
**
** Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

/*
** Name: ddgexec.h - Data Dictionary Generation
**
** Description:
**  This file contains declarations of functions which use to execute every 
**	DDG order
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

#ifndef EXECDDG_INCLUDED
#define EXECDDG_INCLUDED

#include <ddfsem.h>
#include <ddginit.h>

typedef struct __DDG_SESSION 
{
	u_i2				memory_tag;
	PDDG_DESCR	descr;
	SEMAPHORE		sem;
	PDB_CACHED_SESSION	cached_session;
	PDB_QUERY		dbinsert;
	PDB_QUERY		dbupdate;
	PDB_QUERY		dbdelete;
	PDB_QUERY		dbselect;
	PDB_QUERY		dbkeyselect;
	PDB_QUERY		dbexecute;

	PDB_QUERY		current;
	u_i4				numberOfProp;
} DDG_SESSION, *PDDG_SESSION;

GSTATUS 
DDGConnect(
	PDDG_DESCR descr,
	char*	user_name,
	char*	password,
	PDDG_SESSION session);

GSTATUS 
DDGCommit(
	PDDG_SESSION session);

GSTATUS 
DDGRollback(
	PDDG_SESSION session);

GSTATUS 
DDGDisconnect(
	PDDG_SESSION session);

GSTATUS 
DDGInsert(
	PDDG_SESSION session, 
	DDG_OBJECT_NUMBER number, 
	char *paramDef, 
	...); 

GSTATUS 
DDGUpdate(
	PDDG_SESSION session, 
	DDG_OBJECT_NUMBER number, 
	char *paramDef, 
	...);

GSTATUS 
DDGDelete(
	PDDG_SESSION session, 
	DDG_OBJECT_NUMBER number, 
	char *paramDef, 
	...);

GSTATUS 
DDGSelectAll(
	PDDG_SESSION session, 
	DDG_OBJECT_NUMBER number);

GSTATUS 
DDGKeySelect(
	PDDG_SESSION session, 
	DDG_OBJECT_NUMBER number, 
	char *paramDef, 
	...);

GSTATUS 
DDGExecute(
	PDDG_SESSION session, 
	DDG_OBJECT_NUMBER number, 
	char *paramDef, 
	...);

GSTATUS 
DDGProperties(
	PDDG_SESSION session, 
	char *paramDef, 
	...);

GSTATUS 
DDGNext(
	PDDG_SESSION session, 
	bool *existing);

GSTATUS 
DDGClose(
	PDDG_SESSION session);

#endif
