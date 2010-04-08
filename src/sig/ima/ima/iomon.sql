/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** table and procedure registrations for the IMA based I/O monitor
**
** History:
**
** 31-May-95 (johna)
**	written
*/

set session authorization '$ingres';
\p\g

/* Flat table - required for updates */

drop table ima_mib_objects;
\p\g
register table ima_mib_objects (
	server varchar(64) not null not default is 'SERVER',
	classid varchar(64) not null not default is 'CLASSID',
	instance varchar(64) not null not default is 'INSTANCE',
	value varchar(64) not null not default is 'VALUE',
	perms i2 not null not default is 'PERMISSIONS'
)
as import from 'objects'
with update,
dbms = IMA,
structure = unique sortkeyed,
key = (server, classid, instance);
\p\g
commit;
\p\g

drop table ima_server_sessions;
\p\g
register table ima_server_sessions (
	server varchar(64) not null not default is 
		'SERVER',
	session_id varchar(32) not null not default is
		'exp.scf.scs.scb_index',
	effective_user varchar(20) not null not default is
        	'exp.scf.scs.scb_euser',
	real_user varchar(20) not null not default is
        	'exp.scf.scs.scb_ruser',
	db_name varchar(12) not null not default is
        	'exp.scf.scs.scb_database',
	db_owner varchar(8) not null not default is 
        	'exp.scf.scs.scb_dbowner',
	db_lock varchar(9) not null not default is
        	'exp.scf.scs.scb_dblockmode',
	server_facility varchar(10) not null not default is
        	'exp.scf.scs.scb_facility_name',
	session_activity varchar(50) not null not default is
        	'exp.scf.scs.scb_activity',
	activity_detail varchar(50) not null not default is
        	'exp.scf.scs.scb_act_detail',
	session_query varchar(1000) not null not default is
        	'exp.scf.scs.scb_query',
	session_terminal varchar(12) not null not default is
        	'exp.scf.scs.scb_terminal',
	session_group varchar(8) not null not default is
        	'exp.scf.scs.scb_group',
	session_role varchar(8) not null not default is
        	'exp.scf.scs.scb_role',
	server_pid integer4 not null not default is
        	'exp.scf.scs.scb_pid'
)
as import from 'tables'
with dbms = IMA, 
structure = unique sortkeyed,
key = (server, session_id)
\p\g
grant select on ima_server_sessions to public;
\p\g

drop table ima_unix_sessions;
\p\g
register table ima_unix_sessions (

    server varchar(64) not null not default
        is 'SERVER',
    session_id varchar(32) not null not default
        is 'exp.clf.unix.cs.scb_index',
    state varchar(32) not null not default
        is 'exp.clf.unix.cs.scb_state',
    wait_reason varchar(32) not null not default
        is 'exp.clf.unix.cs.scb_memory',
    mask varchar(32) not null not default
        is 'exp.clf.unix.cs.scb_mask',
    priority i4 not null not default
        is 'exp.clf.unix.cs.scb_priority',
    thread_type varchar(8) not null not default
        is 'exp.clf.unix.cs.scb_thread_type',
    timeout i4 not null not default
        is 'exp.clf.unix.cs.scb_timeout',
    mode i4 not null not default
        is 'exp.clf.unix.cs.scb_mode',
    uic i4 not null not default
        is 'exp.clf.unix.cs.scb_uic',
    cputime i4 not null not default
        is 'exp.clf.unix.cs.scb_cputime',
    dio i4 not null not default
        is 'exp.clf.unix.cs.scb_dio',
    bio i4 not null not default
        is 'exp.clf.unix.cs.scb_bio',
    locks i4 not null not default
        is 'exp.clf.unix.cs.scb_locks',
    username varchar(32) not null not default
        is 'exp.clf.unix.cs.scb_username'
)
as import from 'tables'
with dbms = IMA, 
structure = unique sortkeyed,
key = (server, session_id);
\p\g
grant select on ima_unix_sessions to public;
\p\g

drop procedure ima_set_vnode_domain;
\p\g
create procedure ima_set_vnode_domain as
begin
	update ima_mib_objects set value =dbmsinfo('ima_vnode')
	where classid = 'exp.gwf.gwm.session.control.add_vnode'
	and instance = '0'
	and server = dbmsinfo('ima_server');
end;
\p\g
grant execute on procedure ima_set_vnode_domain to public;
\p\g
