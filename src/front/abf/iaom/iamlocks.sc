/*
**	Copyright (c) 1989, 2004 Ingres Corporation
*/

# include <compat.h>
# include <id.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include <fe.h>
# include <er.h>
# include <cm.h>
# include <st.h>
# include <ug.h>
exec sql include <ooclass.sh>;
# include <oocat.h>
# include <uigdata.h>
# include "eram.h"

/**
** Name:	iamlocks.c -	locking utilities
**
** Description:
**
**	Utilities called from abf to handle locking
**
**	IIAMelExistingLocks - find existing locks
**	IIAMalApplLocks - find existing locks in an application
**	IIAMslSessLocks - find existing locks for a session.
**	IIAMdlDeleteLocks - delete a lock or several locks
**	IIAMlcuLockCleanUp - clean up stale locks
**	IIAMclCreateLock - create a new lock
**	IIAMfsiFindSessionId - return a new session id to use with locks
**	IIAMcolChangeOtherLocks - change lock type for other sessions
**	IIAMcslChangeSessLock - change lock type for this session
**/

char *UGcat_now();

/*{
** Name - IIAMelExistingLocks
**
** Description:
**	Find existing locks for a given object.
**
** Inputs:
**	objid - object id.
**	hook - action routine to pass lock information to:
**
**		(*hook)(who,date,how,sessid)
**		char *who;
**		char *date;
**		char *how;
**		i4 sessid;
**
**	This routine should return TRUE to continue retrieving locks,
**	FALSE to end the retrieve.  Since it is being called inside a
**	select, it may not do any DB access.  The character buffers
**	are overwritten between calls, so copies must be made if the
**	strings are to be retained.
**
** Outputs:
**
** Returns:
**	STATUS
**
** History:
**	12/89 (bobm)	written
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

STATUS
IIAMelExistingLocks(objid,hook)
exec sql begin declare section;
OOID objid;
exec sql end declare section;
bool (*hook)();
{
	exec sql begin declare section;
	i4 sessid;
	char who[FE_MAXNAME+1];
	char dstamp[26];
	char why[17];
	exec sql end declare section;

	exec sql repeated select session_id, locked_by, lock_date, lock_type
		into :sessid, :who, :dstamp, :why
		from ii_locks
		where entity_id = :objid;
	exec sql begin;
		if (! (*hook)(who,dstamp,why,sessid))
			exec sql endselect;
	exec sql end;

	return FEinqerr();
}


/*{
** Name - IIAMslSessionLocks
**
** Description:
**	Find existing locks for a given session.
**
** Inputs:
**	sessid - session id.
**	hook - action routine to pass lock information to:
**
**		(*hook)(who,date,how,objid)
**		char *who;
**		char *date;
**		char *how;
**		OOID objid;
**
**	This routine should return TRUE to continue retrieving locks,
**	FALSE to end the retrieve.  Since it is being called inside a
**	select, it may not do any DB access.  The character buffers
**	are overwritten between calls, so copies must be made if the
**	strings are to be retained.
**
** Outputs:
**
** Returns:
**	STATUS
**
** History:
**	12/89 (bobm)	written
*/

STATUS
IIAMslSessionLocks(sessid,hook)
exec sql begin declare section;
OOID sessid;
exec sql end declare section;
bool (*hook)();
{
	exec sql begin declare section;
	OOID objid;
	char who[FE_MAXNAME+1];
	char dstamp[26];
	char why[17];
	exec sql end declare section;

	exec sql repeated select entity_id, locked_by, lock_date, lock_type
		into :objid, :who, :dstamp, :why
		from ii_locks
		where session_id = :sessid;
	exec sql begin;
		if (! (*hook)(who,dstamp,why,objid))
			exec sql endselect;
	exec sql end;

	return FEinqerr();
}

/*{
** Name - IIAMalApplLocks
**
** Description:
**	Find locks for a given application.
**
** Inputs:
**	applid - application id, OC_UNDEFINED for all applications.
**	hook - action routine to pass lock information to:
**
**		(*hook)(who,date,how,sessid,oname,class,objid)
**		char *who;
**		char *date;
**		char *how;
**		i4 sessid;
**		char *oname;
**		OOID class;
**		OOID objid;
**
**	This routine should return TRUE to continue retrieving locks,
**	FALSE to end the retrieve.  Since it is being called inside a
**	select, it may not do any DB access.  The character buffers
**	are overwritten between calls, so copies must be made if the
**	strings are to be retained.
**
** Outputs:
**
** Returns:
**	STATUS
**
** History:
**	12/89 (bobm)	written
*/

STATUS
IIAMalApplLocks(applid,hook)
exec sql begin declare section;
OOID applid;
exec sql end declare section;
bool (*hook)();
{
	exec sql begin declare section;
	i4 sessid;
	char who[FE_MAXNAME+1];
	char dstamp[26];
	char why[17];
	char oname[FE_MAXNAME+1];
	OOID oclass;
	OOID oid;
	exec sql end declare section;

	exec sql repeated select
		ii_objects.object_id, ii_objects.object_class,
			ii_objects.object_name,
		ii_locks.session_id, ii_locks.locked_by,
			ii_locks.lock_date, ii_locks.lock_type
		into :oid, :oclass, :oname, :sessid, :who, :dstamp, :why
		from ii_objects, ii_locks, ii_abfobjects
		where ( :applid = -1 or ii_abfobjects.applid = :applid ) and
			ii_objects.object_id = ii_abfobjects.object_id and
			ii_locks.entity_id = ii_objects.object_id
		order by ii_locks.lock_date;
	exec sql begin;
	{
		if (! (*hook)(who,dstamp,why,sessid,oname,oclass,oid))
			exec sql endselect;
	}
	exec sql end;

	return FEinqerr();
}

/*{
** Name - IIAMdlDeleteLocks
**
** Description:
**	Delete locks for a specific object, or for a session.
**
** Inputs:
**	objid - object id - OC_UNDEFINED to delete all session locks.
**	sessid - session id - OC_UNDEFINED to delete all locks on object.
**
** Returns:
**	STATUS
**
** History:
**	12/89 (bobm)	written
*/

STATUS
IIAMdlDeleteLocks(objid,sessid)
exec sql begin declare section;
OOID objid;
i4 sessid;
exec sql end declare section;
{
	STATUS rval;

	iiuicw1_CatWriteOn();

	if (objid == OC_UNDEFINED)
	{
		exec sql repeated delete
			from ii_locks
			where session_id = :sessid;
	}
	else
	{
		if (sessid == OC_UNDEFINED)
		{
			exec sql repeated delete
				from ii_locks
				where entity_id = :objid;
		}
		else
		{
			exec sql repeated delete
				from ii_locks
				where entity_id = :objid and
					session_id = :sessid;
		}
	}

	rval = FEinqerr();

	iiuicw0_CatWriteOff();

	return rval;
}

/*{
** Name - IIAMlcuLockCleanUp
**
** Description:
**	Delete stale locks.  This only deletes locks with non-zero
**	session ids's.
**
** Inputs:
**	dstr - date string.  Locks older than this will be removed.
**
** Returns:
**	STATUS
**
** History:
**	2/90 (bobm)	written
*/

STATUS
IIAMlcuLockCleanUp(dstr)
exec sql begin declare section;
char *dstr;
exec sql end declare section;
{
	STATUS rval;

	iiuicw1_CatWriteOn();

	exec sql repeated delete from ii_locks
		where session_id <> 0 and lock_date < :dstr;

	rval = FEinqerr();

	iiuicw0_CatWriteOff();

	return rval;
}

/*{
** Name - IIAMclCreateLock
**
** Description:
**	Create a lock
**
** Inputs:
**	objid - object id.
**	sessid - sessid.
**	name - name of locker.
**	how - how locked (type of lock).
**
** Returns:
**	STATUS
**
** History:
**	12/89 (bobm)	written
*/

STATUS
IIAMclCreateLock(objid,sessid,name,how)
exec sql begin declare section;
OOID objid;
i4 sessid;
char *name;
char *how;
exec sql end declare section;
{
	exec sql begin declare section;
	char *dtstr;
	exec sql end declare section;
	STATUS	rval;

	dtstr = UGcat_now();

	iiuicw1_CatWriteOn();

	exec sql repeated insert into ii_locks
	(
		entity_id,
		session_id,
		locked_by,
		lock_date,
		lock_type
	)
	values
	(
		:objid,
		:sessid,
		:name,
		:dtstr,
		:how
	);

	rval = FEinqerr();

	iiuicw0_CatWriteOff();

	return rval;
}

/*{
** Name - IIAMfsiFetchSessionId
**
** Description:
**	Get a unique session id and appropriate name for locking operations.
**
** Inputs:
**	name - buffer for name.
**
** Outputs:
**	sessid - returned session id.
**	name - name returned.
**
** Returns:
**	STATUS
**
** History:
**	12/89 (bobm)	written
**	6/90 (Mike S)	Strip trailing whitespace from name
*/

STATUS
IIAMfsiFetchSessionId(sessid,name)
i4 *sessid;
char *name;
{
	i4 odelta;

	odelta = IIOOsetidDelta(1);

	iiuicw1_CatWriteOn();

	*sessid = IIOOnewId();

	iiuicw0_CatWriteOff();

	_VOID_ IIOOsetidDelta(odelta);


	if (*sessid <= OC_UNDEFINED )
                return FAIL;

	IDname(&name);	/* yes, address of.  yes, it's silly */
	_VOID_ STtrmwhite(name); /* Strip trailing whitespace (for VMS) */

	return IIAMclCreateLock(0,*sessid,name,ERx("abf"));
}

/*{
** Name - IIAMcolChangeOtherLocks - change lock type for other sessions
**
** Description:
**	Changes the type of lock on other sessions.
**
** Inputs:
**	objid - object to change lock on.
**	sessid - session id to leave alone.
**	how - new lock type.
**
** Returns:
**	STATUS
**
** History:
**	12/89 (bobm)	written
*/

STATUS
IIAMcolChangeOtherLocks(objid,sessid,how)
exec sql begin declare section;
OOID objid;
i4 sessid;
char *how;
exec sql end declare section;
{
	STATUS rval;

	iiuicw1_CatWriteOn();

	exec sql repeated update ii_locks
		set lock_type = :how
		where entity_id = :objid and session_id <> :sessid;

	rval = FEinqerr();

	iiuicw0_CatWriteOff();

	return rval;
}

/*{
** Name - IIAMcslChangeSessLock - change lock type for other sessions
**
** Description:
**	Changes the type of lock on a given session.
**
** Inputs:
**	objid - object to change lock on.
**	sessid - session id to change lock on.
**	how - new lock type.
**
** Returns:
**	STATUS
**
** History:
**	12/89 (bobm)	written
*/

STATUS
IIAMcslChangeSessLock(objid,sessid,how)
exec sql begin declare section;
OOID objid;
i4 sessid;
char *how;
exec sql end declare section;
{
	STATUS rval;

	iiuicw1_CatWriteOn();

	exec sql repeated update ii_locks
		set lock_type = :how
		where entity_id = :objid and session_id = :sessid;

	rval = FEinqerr();

	iiuicw0_CatWriteOff();

	return rval;
}
