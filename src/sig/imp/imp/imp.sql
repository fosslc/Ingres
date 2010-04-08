/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** table and view registrations for the IMA based IPM lookalike tool
**
## History:
##
## 26-Jan-94	(johna)
##	Started.
## 01-Feb-94	(johna)
##	Added procedure for remove session.
## 02-Feb-94	(johna)
##	Added server parameter table
## 03-Feb-94	(johna)
##	Added procedures for server_close,server_open,server_shut,server_stop
## 10-mar-94	(johna)_
##	Changed imp_gcn_view to dummy up the active session information instead
##	of selecting from the imp_srv_block. Also changed the imp_srv_block to
##	use SCF objects instead of CL ones.
## 30-Nov-94 	(johna)
##	Added client_info objects
## 07-Apr-95    (trevor)
##      Added imp_lk_type to supply mapping for lock types based on
##      'decode_rsb0'
## 10-Apr-02    (mdf)
##      imp_lkmo_llb was not showing any rows.
##      Changed imp_lkmo_llb so that it shows some lock info.
##
##      imp_lkmo_rsb was not showing any rows.
##      Changed imp_lkmo_rsb so that it shows some lock info.
##
##      imp_lkmo_lkb was not showing any rows.
##      Changed imp_lkmo_lkb so that it shows some lock info.
##
## 12-Apr-02    (mdf)
##      some columns removed from imp_lgmo_lfb so that logging works in 2.5
##
## 16-Apr-02    (mdf)
##      Added a new table imp_gcc_info with wider columns than the ima
##      equivalent
##
##      Removed some columns from imp_lkmo_llb so that locking works in 2.5
## 02-Jul-2002 (fanra01)
##      Sir 108159
##      Added to distribution in the sig directory.
## 12-Nov-2002 (fanra01)
##      Bug 109104
##      Added imp_nt_sessions table and included it into the imp_cs_sessions 
##      view.  Constants were used in the view where columns do not exist in 
##      the base table.
##      Copied imp_srv_block table to imp_srv_block_unix, imp_srv_block_vms
##      and imp_srv_block_nt.  Changed imp_srv_block from a table to a view
##      of the base tables.
## 04-Sep-03    (mdf) 
## mdf040903
##    1 Undone the change made to the where clause in imp_gcn_view 
##      (2.6 now seems to be happy without the 091002 change!)
##    2  Added Comm Server connection info into imp_gcn_view
##    3 Added the lastquery session info into a new table imp_lastquery
##      Note that this info is only effective from 2.6 onwards, which is
##      why it has gone into a separate table.
##    4.Added Windows sessions table imp_win_sessions and included it in 
##      the view imp_cs_sessions
##    5.Widened the imp_gcc_globals columns to show all data
##    6.Added extra dmf wait info into imp_dmf_cache_stats
##     06-Apr-04    (srisu02) 
##              Sir 108159
##              Integrating changes made by flomi02  
## 20-Sep-2005 (fanra01)
##      Replaced imp_srv_block view definition.
*/
set session authorization '$ingres';
\p\g

/* Flat table - required for updates */

drop table imp_mib_objects;
\p\g
register table imp_mib_objects (
	server varchar( 64 ) not null not default is 'SERVER',
	classid varchar( 64 )  not null not default is 'CLASSID',
	instance varchar( 64 ) not null not default is 'INSTANCE',value
	varchar( 64 ) not null not default is 'VALUE',
	perms i2 not null not default is 'PERMISSIONS'
	)
	as import from 'objects'
with update,
dbms = IMA,
structure = unique sortkeyed,
key = ( server, classid, instance );
\p\g
commit work;
\p\g

/* 
##mdf040903 
*/
drop table imp_lastquery;
register table imp_lastquery
      ( server varchar(64) not null not default
          is 'SERVER',
        session_id varchar(32) not null not default
          is 'exp.scf.scs.scb_self',
        session_lastquery varchar(1000) not null not default
        is 'exp.scf.scs.scb_lastquery')
      as import from 'tables'
      with dbms = IMA,
      structure = unique sortkeyed,
      key = (server, session_id)
\p\g
 
commit work;
\p\g
/* 
##mdf040903 
*/

drop table imp_gcc_globals;

register table imp_gcc_globals (
	server		varchar(64) 	not null not default is 'SERVER',
	conn_count	varchar(11) not null not default is 
		'exp.gcf.gcc.conn_count',
	ib_conn_count	varchar(11) not null not default is 
		'exp.gcf.gcc.ib_conn_count',
	ib_max	varchar(10) not null not default is 
		'exp.gcf.gcc.ib_max',
	ob_conn_count	varchar(11) not null not default is 
		'exp.gcf.gcc.ob_conn_count',
	ob_max	varchar(11) not null not default is 
		'exp.gcf.gcc.ob_max'

) as import from 'tables'
with dbms = IMA,
structure = sortkeyed,
key = (server);
\p\g


commit work;
\p\g
drop imp_gcn_entry;
\p\g

register table imp_gcn_entry (

    gcn		varchar( 64 )	not null not default is
				'SERVER',
    entry	i4		not null not default is
				'exp.gcf.gcn.server.entry',
    address	varchar( 64 )	not null not default is
				'exp.gcf.gcn.server.address',
    class	varchar( 32 )	not null not default is
				'exp.gcf.gcn.server.class',
    flags	i4		not null not default is
				'exp.gcf.gcn.server.flags',
    object	varchar( 32 )	not null not default is
				'exp.gcf.gcn.server.object'
    )
    as import from 'tables'
    with dbms = IMA,
    structure = unique sortkeyed,
    key = ( gcn, entry )

\p\g
drop table imp_srv_block_unix
\p\g

register table imp_srv_block_unix (

	server varchar( 64 ) not null not default is
		'SERVER',
	listen_addr varchar( 64) not null not default is
		'exp.scf.scd.server_name',
		/* 'exp.gcf.gca.listen_address', */
	max_connections integer4 not null not default is 
		'exp.scf.scd.server.max_connections',
	num_connections integer4 not null not default is 
		'exp.scf.scd.server.current_connections',
	server_pid integer4 not null not default is 
		'exp.clf.unix.cs.srv_block.pid'
    )
    as import from 'tables'
    with dbms = IMA,
    structure = sortkeyed,
    key = ( server  )

\p\g

drop table imp_srv_block_nt
\p\g

register table imp_srv_block_nt (

	server varchar( 64 ) not null not default is
		'SERVER',
	listen_addr varchar( 64) not null not default is
		'exp.scf.scd.server_name',
		/* 'exp.gcf.gca.listen_address', */
	max_connections integer4 not null not default is 
		'exp.scf.scd.server.max_connections',
	num_connections integer4 not null not default is 
		'exp.scf.scd.server.current_connections',
	server_pid integer4 not null not default is 
		'exp.clf.nt.cs.srv_block.pid'
    )
    as import from 'tables'
    with dbms = IMA,
    structure = sortkeyed,
    key = ( server  )

\p\g

drop table imp_srv_block_vms
\p\g

register table imp_srv_block_vms (

	server varchar( 64 ) not null not default is
		'SERVER',
	listen_addr varchar( 64) not null not default is
		'exp.scf.scd.server_name',
		/* 'exp.gcf.gca.listen_address', */
	max_connections integer4 not null not default is 
		'exp.scf.scd.server.max_connections',
	num_connections integer4 not null not default is 
		'exp.scf.scd.server.current_connections',
	server_pid integer4 not null not default is 
		'exp.clf.vms.cs.srv_block.pid'
    )
    as import from 'tables'
    with dbms = IMA,
    structure = sortkeyed,
    key = ( server  )

\p\g

commit work;
\p\g
drop view imp_srv_block
\p\g

create view imp_srv_block as
select
    server,
    listen_addr,
    max_connections,
    num_connections,
    server_pid
from
    imp_srv_block_unix
union
select
    server,
    listen_addr,
    max_connections,
    num_connections,
    server_pid
from
    imp_srv_block_nt
union
select
    server,
    listen_addr,
    max_connections,
    num_connections,
    server_pid
from
    imp_srv_block_vms
    
\p\g

commit work;
\p\g
drop view imp_gcn_view 
\p\g

create view imp_gcn_view as
select 
	gcn,
	address,
	class,
	max_connections as max_sessions,
	max_active = 0,
	num_connections as num_sessions,
	num_active = 0,
	object,
	server_pid 
from
	imp_gcn_entry,
	imp_srv_block
where
	class not in ('IINMSV','COMSVR') 
and	address = listen_addr   /* 
##mdf091002
*/
union
select 
	gcn,
	address,
	class,
	max_sessions = 0,
	max_active = 0,
	num_sessions = 0,
	num_active = 0,
	object,
	server_pid = 0
from
	imp_gcn_entry
where class ='IINMSVR'                                                 
                                                                         
union                                                                    
/* 
##mdf040903 
*/
select                                                                   
        gcn,                                                             
        address,                                                         
        class,                                                           
        max_sessions = 0,                                                
        max_active = 0,                                                  
        num_sessions = ifnull(int4(conn_count),0),
        num_active = 0,                                                  
        object,                                                          
        server_pid = 0                                                   
from imp_gcn_entry gcn left join imp_gcc_globals gcc                     
on trim(dbmsinfo('ima_vnode') + '::/@' + varchar(address)) =  gcc.server 
where gcn.class = 'COMSVR'                                               
/* 
##mdf040903 
*/

\p\g

commit work;
\p\g
drop procedure imp_complete_vnode
\p\g

create procedure imp_complete_vnode as
begin

	update imp_mib_objects set value =dbmsinfo('ima_vnode')
	where classid = 'exp.gwf.gwm.session.control.add_vnode'
	and instance = '0'
	and server = dbmsinfo('ima_server');
end;
\p\g

commit work;
\p\g
drop procedure imp_reset_domain
\p\g

create procedure imp_reset_domain as
begin

	update imp_mib_objects set value = ' '
	where classid = 'exp.gwf.gwm.session.control.reset_domain'
	and instance = '0'
	and server = dbmsinfo('ima_server');
end;
\p\g

commit work;
\p\g
drop procedure imp_set_domain
\p\g

create procedure imp_set_domain(entry varchar(30) not null) as
begin

	update imp_mib_objects set value = trim(:entry)
	where classid = 'exp.gwf.gwm.session.control.add_vnode'
	and instance = '0'
	and server = dbmsinfo('ima_server');
end;
\p\g

commit work;
\p\g
drop procedure imp_del_domain
\p\g

create procedure imp_del_domain(entry varchar(30) not null) as
begin

	update imp_mib_objects set value = :entry
	where classid = 'exp.gwf.gwm.session.control.del_vnode'
	and instance = '0'
	and server = dbmsinfo('ima_server');
end;
\p\g

/* 
##mdf040903
*/
grant select on imp_lastquery to public
\p\g

grant select on imp_gcn_entry to public;
\p\g
grant select on imp_srv_block to public;
\p\g
grant select on imp_gcn_view to public;
\p\g
grant execute on procedure imp_complete_vnode to public;
\p\g
grant execute on procedure imp_set_domain to public;
grant execute on procedure imp_del_domain to public;
\p\g
grant execute on procedure imp_reset_domain to public;
\p\g

/*
** information about the sessions
*/

commit work;
\p\g
drop table imp_scs_sessions
\p\g

register table imp_scs_sessions (

    server varchar(64) not null not default
        is 'SERVER',

    session_id varchar(32) not null not default
        is 'exp.scf.scs.scb_index',

    assoc_id integer not null not default
        is 'exp.scf.scs.scb_gca_assoc_id',

    euser varchar(20) not null not default
        is 'exp.scf.scs.scb_euser',

    ruser varchar(20) not null not default
        is 'exp.scf.scs.scb_ruser',

    database varchar(12) not null not default
        is 'exp.scf.scs.scb_database',

    dblock varchar(9) not null not default
        is 'exp.scf.scs.scb_dblockmode',

    facidx i1 not null not default
        is 'exp.scf.scs.scb_facility_index',

    facility varchar(3) not null not default
        is 'exp.scf.scs.scb_facility_name',

    activity varchar(40) not null not default
        is 'exp.scf.scs.scb_activity',

    act_detail varchar(50) not null not default
        is 'exp.scf.scs.scb_act_detail',

    query varchar(1000) not null not default
        is 'exp.scf.scs.scb_query',

    dbowner varchar(8) not null not default
        is 'exp.scf.scs.scb_dbowner',

    terminal varchar(12) not null not default
        is 'exp.scf.scs.scb_terminal',

    s_name varchar(20) not null not default
        is 'exp.scf.scs.scb_s_name',

    groupid varchar(8) not null not default
        is 'exp.scf.scs.scb_group',

    role varchar(8) not null not default
        is 'exp.scf.scs.scb_role',

    appcode i1 not null not default
        is 'exp.scf.scs.scb_appl_code',

    scb_pid i4 not null not default	/* needed to join with locks */
        is 'exp.scf.scs.scb_pid'

    )
    as import from 'tables'
    with dbms = IMA, structure = unique sortkeyed,
    key = ( server, session_id )

\p\g

/*
** A thread table - Sessions in the server are maped to threads.
*/

commit work;
\p\g
drop table imp_vms_sessions
\p\g

register table imp_vms_sessions (

    server varchar(64) not null not default
        is 'SERVER',

    session_id varchar(32) not null not default
        is 'exp.clf.vms.cs.scb_index',

    state varchar(32) not null not default
        is 'exp.clf.vms.cs.scb_state',

    wait_reason varchar(32) not null not default
        is 'exp.clf.vms.cs.scb_memory',

    mask varchar(32) not null not default
        is 'exp.clf.vms.cs.scb_mask',

    priority i4 not null not default
        is 'exp.clf.vms.cs.scb_priority',

    thread_type varchar(8) not null not default
        is 'exp.clf.vms.cs.scb_thread_type',

    timeout i4 not null not default
        is 'exp.clf.vms.cs.scb_timeout',

    mode i4 not null not default
        is 'exp.clf.vms.cs.scb_mode',

    session_time i4 not null not default         /*
##mdf040903
*/
        is 'exp.clf.vms.cs.scb_connect',

    uic i4 not null not default
        is 'exp.clf.vms.cs.scb_uic',

    cputime i4 not null not default
        is 'exp.clf.vms.cs.scb_cputime',

    dio i4 not null not default
        is 'exp.clf.vms.cs.scb_dio',

    bio i4 not null not default
        is 'exp.clf.vms.cs.scb_bio',

    locks i4 not null not default
        is 'exp.clf.vms.cs.scb_locks',

    username varchar(32) not null not default
        is 'exp.clf.vms.cs.scb_username'

    )
    as import from 'tables'
    with dbms = IMA, structure = unique sortkeyed,
    key = ( server, session_id );

\p\g

commit work;
\p\g
/* 
##mdf040903. 
Include Windows sessions table */
drop table imp_win_sessions
;

register table imp_win_sessions (
server varchar(64) not null not default is 'SERVER',
server_pid integer4 not null not default is 'exp.scf.scs.scb_pid',
session_id varchar(32) not null not default is 'exp.scf.scs.scb_index',
effective_user varchar(32) not null not default is 'exp.scf.scs.scb_euser',
real_user varchar(32) not null not default is 'exp.scf.scs.scb_ruser',
db_name varchar(32) not null not default is 'exp.scf.scs.scb_database',
db_owner varchar(32) not null not default is 'exp.scf.scs.scb_dbowner',
server_facility varchar(10) not null not default is 'exp.scf.scs.scb_facility_name',
session_query varchar(500) not null not default is 'exp.scf.scs.scb_query',
client_user varchar(32) not null not default is 'exp.scf.scs.scb_client_user',
cputime integer4 not null not default is 'exp.clf.nt.cs.scb_cputime',
dio integer4 not null not default is 'exp.clf.nt.cs.scb_dio',
bio integer4 not null not default is 'exp.clf.nt.cs.scb_bio',
lio integer4 not null not default is 'exp.clf.nt.cs.scb_lio'
,state varchar(32) not null not default is 'exp.clf.nt.cs.scb_state'
, wait_reason varchar(32) not null not default is 'exp.clf.nt.cs.scb_memory'
, mask varchar(32) not null not default is 'exp.clf.nt.cs.scb_mask'
--, priority i4 not null not default is 'exp.clf.nt.cs.scb_priority'
, thread_type varchar(8) not null not default is 'exp.clf.nt.cs.scb_thread_type'
--, timeout i4 not null not default  is 'exp.clf.nt.cs.scb_timeout'
, mode i4 not null not default      is 'exp.clf.nt.cs.scb_mode'
, session_time i4 not null not default         /*
##mdf040903
*/
        is 'exp.clf.nt.cs.scb_connect'

--, uic i4 not null not default       is 'exp.clf.nt.cs.scb_uic'
, locks i4 not null not default     is 'exp.clf.nt.cs.scb_locks'    
, username varchar(32) not null not default    is 'exp.clf.nt.cs.scb_username'      

)
as import from 'tables'
with dbms = IMA, structure = unique sortkeyed,
key = (server, session_id);
\p\g


commit work;
\p\g
drop table imp_unix_sessions
\p\g

register table imp_unix_sessions (

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

    session_time i4 not null not default         /*
##mdf040903
*/
        is 'exp.clf.unix.cs.scb_connect',

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
    with dbms = IMA, structure = unique sortkeyed,
    key = ( server, session_id );

\p\g

create view imp_cs_sessions as
	select 	server,
		session_id,
		state,
		wait_reason,
		mask,
		priority,
		thread_type,
		timeout,
		mode,
		session_time,
		uic,
		cputime,
		dio,
		bio,
		locks,
		username
	from imp_unix_sessions
union
	select server,
		session_id,
		state,
		wait_reason,
		mask,
		priority,
		thread_type,
		timeout,
		mode,
		session_time,
		uic,
		cputime,
		dio,
		bio,
		locks,
		username
	from imp_vms_sessions
/* 
##mdf040903 
 */
union
	select server,
		session_id,
		state,
		wait_reason,
		mask,
		0,      /* priority not found on NT */
		thread_type,
		0,      /* timeout not found on NT */
		mode,
		session_time,
		0,      /* uic not found on NT */
		cputime,
		dio,
		bio,
		locks,
		username
	from imp_win_sessions
        /* 
##mdf040903 
*/
\p\g

commit work;
\p\g
drop view imp_session_view\p\g

create view imp_session_view as select 
	c.server, 
	c.session_id, 
	c.state, 
	c.wait_reason, 
	c.mask, 
	c.priority,
	c.thread_type, 
	c.timeout, 
	c.mode as smode, 
	c.uic, 
	c.cputime,
	c.dio,
	c.bio,
	c.locks,
	c.username,
	s.assoc_id,
	s.euser,
	s.ruser,
	s.database,
	s.dblock,
	s.facidx,
	s.facility,
	s.activity,
	s.act_detail,
	s.query,
	s.dbowner,
	s.terminal,
	s.s_name,
	s.groupid,
	s.role,
	s.appcode,
	s.scb_pid
from
	imp_cs_sessions c,
	imp_scs_sessions s
where 
c.server = s.server and
c.session_id = s.session_id

\p\g

/* 
##mdf160402 
*/
drop table imp_gcc_info
\p\g
/* Replaced table with extra column info (below) 
##mdf040903 
register table imp_gcc_info ( net_server varchar(64) not null not
 default is 'SERVER', inbound_max varchar(11) not null not default is
 'exp.gcf.gcc.ib_max', inbound_current varchar(11) not null not
 default is 'exp.gcf.gcc.ib_conn_count', outbound_max varchar(11) not
 null not default is 'exp.gcf.gcc.ob_max', outbound_current
 varchar(11) not null not default is 'exp.gcf.gcc.ob_conn_count' ) as
 import from 'tables' with dbms = IMA, structure = sortkeyed, key =
 (net_server)
\p\g
*/

register table imp_gcc_info
( net_server varchar(64) not null not       default is 'SERVER'
, inbound_max varchar(11) not null not default is   	'exp.gcf.gcc.ib_max'
, inbound_current varchar(11) not null not  
       default is 	'exp.gcf.gcc.ib_conn_count'
, outbound_max varchar(11) not null not default is 	'exp.gcf.gcc.ob_max'
, outbound_current  varchar(11) not null not 
       default is 'exp.gcf.gcc.ob_conn_count' 

--, a varchar(12) not null not default is 'exp.gcf.gcc.conn'
--, b  varchar(12) not null not default is 'exp.gcf.gcc.conn.flags'
--, c  varchar(12) not null not default is 'exp.gcf.gcc.conn.pl_protocol'
, conn_count  varchar(12) not null not default is 'exp.gcf.gcc.conn_count'
, data_in  varchar(12) not null not default is 'exp.gcf.gcc.data_in'
, data_out  varchar(12) not null not default is 'exp.gcf.gcc.data_out'
, ib_encrypt_mech  varchar(12) not null not default 
    is 'exp.gcf.gcc.ib_encrypt_mech'
, ib_encrypt_mode  varchar(12) not null not default 
    is 'exp.gcf.gcc.ib_encrypt_mode'
, msgs_in  varchar(12) not null not default is 'exp.gcf.gcc.msgs_in'
, msgs_out  varchar(12) not null not default is 'exp.gcf.gcc.msgs_out'
, ob_encrypt_mech  varchar(12) not null not default 
    is 'exp.gcf.gcc.ob_encrypt_mech'
, ob_encrypt_mode  varchar(12) not null not default 
    is 'exp.gcf.gcc.ob_encrypt_mode'
, pl_protocol  varchar(12) not null not default is 'exp.gcf.gcc.pl_protocol'
--, p  varchar(12) not null not default is 'exp.gcf.gcc.protocol'
--, q  varchar(12) not null not default is 'exp.gcf.gcc.protocol.port'
, registry_mode  varchar(12) not null not default is 'exp.gcf.gcc.registry_mode'
, trace_level  varchar(12) not null not default is 'exp.gcf.gcc.trace_level'
) as   import from 'tables' with dbms = IMA, structure = sortkeyed, key =    
 (net_server)      
; commit;
\p\g

	/* PERMISSIONS */
grant select on imp_unix_sessions to public;
grant select on imp_vms_sessions to public;
grant select on imp_win_sessions to public;
grant select on imp_cs_sessions to public;
grant select on imp_scs_sessions to public;
grant select on imp_session_view to public;
grant select on imp_gcc_info to public;

\p\g

commit work;
\p\g
drop  procedure imp_remove_session
\p\g
create procedure imp_remove_session(	session_id varchar(32) not null,
					server_id varchar(32) not null) as
begin

	update imp_mib_objects set value = :session_id
	where classid = 'exp.scf.scs.scb_remove_session'
	and instance = :session_id
	and server = :server_id;

	if iirowcount = 0 then
		message 'No such session - it must have exited'
	else
		message 'Session removed'
	endif;
	return 0;
end;
\p\g

grant execute on procedure imp_remove_session to public;
\p\g

commit work;
\p\g
drop table imp_server_parms;\p\g

register table imp_server_parms (

server		varchar(64) not null not default 
		is 'SERVER',

cursors		integer4 not null not default
		is 'exp.scf.scd.acc',

capabilities	varchar(45) not null not default
		is 'exp.scf.scd.capabilities_str',
	
cursor_flags	integer4 not null not default
		is 'exp.scf.scd.csrflags',

fastcommit	integer4 not null not default 
		is 'exp.scf.scd.fastcommit',

/*
fips		integer4 not null not default
		is 'exp.scf.scd.fips',
		*/

flatten		integer4 not null not default
		is 'exp.scf.scd.flatflags',

listen_fails	integer4 not null not default
		is 'exp.scf.scd.gclisten_fail',

name_service	integer4 not null not default
		is 'exp.scf.scd.names_reg',

no_star_cluster	integer4 not null not default 
		is 'exp.scf.scd.no_star_cluster',

/*
noflatten	integer4 not null not default
		is 'exp.scf.scd.noflatten',
	*/

nostar_recov	integer4 not null not default
		is 'exp.scf.scd.nostar_recovr',

nousers		integer4 not null not default
		is 'exp.scf.scd.nousers',

incons_risk	integer4 not null not default
		is 'exp.scf.scd.risk_inconsistency',

rule_depth	integer4 not null not default
		is 'exp.scf.scd.rule_depth',

connects	integer4 not null not default
		is 'exp.scf.scd.server.current_connections',

listen_state	varchar(6) not null not default
		is 'exp.scf.scd.server.listen_state',

max_connects	integer4 not null not default
		is 'exp.scf.scd.server.max_connections',

res_connects	integer4 not null not default
		is 'exp.scf.scd.server.reserved_connections',

shut_state	varchar(7) not null not default
		is 'exp.scf.scd.server.shutdown_state',

server_name	varchar(16) not null not default
		is 'exp.scf.scd.server_name',

soleserver	integer4 not null not default
		is 'exp.scf.scd.soleserver',

svr_state	varchar(11) not null not default
		is 'exp.scf.scd.state_str',

wbehind		integer4 not null not default
		is 'exp.scf.scd.writebehind'
	
)
as import from 'tables'
with dbms = IMA,
     structure = sortkeyed,
     key = (server);
\p\g

grant select on imp_server_parms to public;

\p\g
commit work;
\p\g
drop  procedure imp_close_server
\p\g
create procedure imp_close_server(server_id varchar(32) not null) as
begin

	update imp_mib_objects set value = :server_id
	where classid = 'exp.scf.scd.server.control.close'
	and instance = 0
	and server = :server_id;

	if iirowcount = 0 then
		message 'No such server - it must have exited'
	endif;
	commit;
	return 0;
end;
\p\g
grant execute on procedure imp_close_server to public;
\p\g
commit work;
\p\g
drop  procedure imp_open_server
\p\g
create procedure imp_open_server(server_id varchar(32) not null) as
begin

	update imp_mib_objects set value = :server_id
	where classid = 'exp.scf.scd.server.control.open'
	and instance = 0
	and server = :server_id;

	if iirowcount = 0 then
		message 'No such server - it must have exited'
	endif;
	commit;
	return 0;
end;
\p\g
grant execute on procedure imp_open_server to public;
\p\g
commit work;
\p\g
drop  procedure imp_stop_server
\p\g
create procedure imp_stop_server(server_id varchar(32) not null) as
begin

	update imp_mib_objects set value = :server_id
	where classid = 'exp.scf.scd.server.control.stop'
	and instance = 0
	and server = :server_id;

	if iirowcount = 0 then
		message 'No such server - it must have exited'
	endif;
	commit;
	return 0;
end;
\p\g
grant execute on procedure imp_stop_server to public;
\p\g
commit work;
\p\g
drop  procedure imp_shut_server
\p\g
create procedure imp_shut_server(server_id varchar(32) not null) as
begin

	update imp_mib_objects set value = :server_id
	where classid = 'exp.scf.scd.server.control.shut'
	and instance = 0
	and server = :server_id;

	if iirowcount = 0 then
		message 'No such server - it must have exited'
	endif;
	commit;
	return 0;
end;
\p\g
grant execute on procedure imp_shut_server to public;
\p\g

commit work;
\p\g
drop table imp_mo_meta
\p\g
register table imp_mo_meta (

    server varchar(64) not null not default
        is 'SERVER',

    classid varchar( 64 ) not null not default
        is 'exp.glf.mo.meta.classid',

    class varchar( 64 ) not null not default
        is 'exp.glf.mo.meta.class',

    oid varchar( 8 ) not null not default
        is 'exp.glf.mo.meta.oid',

    perms i2 not null not default
        is 'exp.glf.mo.meta.perms',

    size i2 not null not default
        is 'exp.glf.mo.meta.size',

    xindex varchar( 32 ) not null not default
        is 'exp.glf.mo.meta.index'
    )
    as import from 'tables'
    with dbms = IMA, structure = unique sortkeyed,
    key = ( server, classid );
\p\g
commit work;
\p\g

/*
** Domain table
** - show the somain of all servers and vnodes visible to the session
*/

commit work;
\p\g
drop table imp_domain
\p\g

register table imp_domain (

    domplace varchar( 64 ) not null not default is
	'exp.gwf.gwm.session.dom.index'
    )
    as import from 'tables'
    with dbms = IMA,
    structure = unique sortkeyed,
    key = ( domplace );


commit
\p\g

grant select on imp_domain to public
\p\g
grant select on imp_mo_meta to public
\p\g

/*
** Locking System Information - cribbed from BryanP.
*/

commit work;
\p\g
drop table imp_lkmo_lkd;\p\g
drop table imp_lkmo_lkd_stat;\p\g
drop table imp_lkmo_llb;\p\g
drop table imp_lkmo_rsb;\p\g
drop table imp_lkmo_lkb;\p\g

register table imp_lkmo_lkd (

    vnode	    varchar(64) not null not default is 'VNODE',

    lkd_status			    i4 not null not default
			    is 'exp.dmf.lk.lkd_status',
    lkd_csp_id			    i4 not null not default
			    is 'exp.dmf.lk.lkd_csp_id',
    lkd_rsh_size		    i4 not null not default
			    is 'exp.dmf.lk.lkd_rsh_size',
    lkd_lkh_size		    i4 not null not default
			    is 'exp.dmf.lk.lkd_lkh_size',
    lkd_max_lkb			    i4 not null not default
			    is 'exp.dmf.lk.lkd_max_lkb',
    lkd_llb_inuse		    i4 not null not default
			    is 'exp.dmf.lk.lkd_llb_inuse',
    lkd_rsb_inuse		    i4 not null not default
			    is 'exp.dmf.lk.lkd_rsb_inuse',
    lkd_lkb_inuse		    i4 not null not default
			    is 'exp.dmf.lk.lkd_lkb_inuse',
    lkd_lbk_count		    i4 not null not default
			    is 'exp.dmf.lk.lkd_lbk_count',
    lkd_lbk_size		    i4 not null not default
			    is 'exp.dmf.lk.lkd_lbk_size',
    lkd_sbk_count		    i4 not null not default
			    is 'exp.dmf.lk.lkd_sbk_count',
    lkd_sbk_size		    i4 not null not default
			    is 'exp.dmf.lk.lkd_sbk_size',
    lkd_node_fail		    i4 not null not default
			    is 'exp.dmf.lk.lkd_node_fail',
    lkd_csid			    i4 not null not default
			    is 'exp.dmf.lk.lkd_csid'
)
as import from 'tables'
with dbms = IMA,
     structure = sortkeyed,
     key = (vnode);
\p\g

register table imp_lkmo_lkd_stat (

    vnode	    varchar(64) not null not default is 'VNODE',

/*
    lkd_stat_next_id		    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.next_id',
*/
    lkd_stat_create_list	    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.create_list',
    lkd_stat_request_new	    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.request_new',
    lkd_stat_request_cvt	    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.request_convert',
    lkd_stat_convert		    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.convert',
    lkd_stat_release		    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.release',
    lkd_stat_rlse_partial	    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.release_partial',
    lkd_stat_release_all	    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.release_all',
    lkd_stat_wait		    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.wait',
    lkd_stat_convert_wait	    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.convert_wait',
    lkd_stat_cvt_search		    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.convert_search',
    lkd_stat_cvt_deadlock	    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.convert_deadlock',
    lkd_stat_dlock_srch		    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.deadlock_search',
    lkd_stat_deadlock		    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.deadlock',
    lkd_stat_cancel		    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.cancel',
    lkd_stat_enq		    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.enq',
    lkd_stat_deq		    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.deq',
    lkd_stat_gdlck_search	    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.gdlck_search',
    lkd_stat_gdeadlock		    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.gdeadlock',
    lkd_stat_gdlck_grant	    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.gdlck_grant',
    lkd_stat_tot_gdlck_srch	    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.totl_gdlck_search',
    lkd_stat_gdlck_call		    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.gdlck_call',
    /*
    lkd_stat_gdlck_send		    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.gdlck_send',
    */
    lkd_stat_cnt_gdlck_call	    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.cnt_gdlck_call',
    lkd_stat_cnt_gdlck_sent	    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.cnt_gdlck_sent',
/*
    lkd_stat_uns_gdlck_srch	    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.unsent_gdlck_search',
*/
    lkd_stat_sent_all		    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.sent_all'

)
as import from 'tables'
with dbms = IMA,
     structure = sortkeyed,
     key = (vnode);
\p\g

register table imp_lkmo_llb (

    vnode	    varchar(64) not null not default is 'VNODE'
,    llb_id_id		    i4 not null not default
			    is 'exp.dmf.lk.llb_id.id_id' 
/* 
##mdf100402
, llb_id_instance	    i4 not null not default
			    is 'exp.dmf.lk.llb_id.id_instance' 
*/
,    llb_lkb_count	    i4 not null not default
			    is 'exp.dmf.lk.llb_lkb_count' 
,    llb_status		    char(50) not null not default
			    is 'exp.dmf.lk.llb_status' 
,    llb_status_num	    i4 not null not default
			    is 'exp.dmf.lk.llb_status_num' 
,    llb_llkb_count	    i4 not null not default
			    is 'exp.dmf.lk.llb_llkb_count' 
,    llb_max_lkb		    i4 not null not default
			    is 'exp.dmf.lk.llb_max_lkb' 
,    llb_related_llb	    i4 not null not default
			    is 'exp.dmf.lk.llb_related_llb' 
,    llb_related_llb_id_id	i4 not null not default
			    is 'exp.dmf.lk.llb_related_llb_id_id' 
,    llb_related_count	    i4 not null not default
			    is 'exp.dmf.lk.llb_related_count' 
,    llb_wait_id_id		i4 not null not default
			    is 'exp.dmf.lk.llb_wait_id_id' 
,    llb_name0		    i4 not null not default
			    is 'exp.dmf.lk.llb_name0' 
,    llb_name1		    i4 not null not default
			    is 'exp.dmf.lk.llb_name1' 
,    llb_search_count	    i4 not null not default
			    is 'exp.dmf.lk.llb_search_count' 
,    llb_pid		    i4 not null not default
			    is 'exp.dmf.lk.llb_pid' 
,    llb_sid		    varchar(32) not null not default
			    is 'exp.dmf.lk.llb_sid' 
,    llb_connect_count	    i4 not null not default
			    is 'exp.dmf.lk.llb_connect_count' 
/* 
##mdf160402
,    llb_event_type	    i4 not null not default
			    is 'exp.dmf.lk.llb_event_type' 
,    llb_event_value	    i4 not null not default
			    is 'exp.dmf.lk.llb_event_value' 
*/
,    llb_evflags		    i4 not null not default
			    is 'exp.dmf.lk.llb_evflags' 
/* 
##mdf100402
,    llb_timeout		    i4 not null not default
			    is 'exp.dmf.lk.llb_timeout' 
*/
,    llb_stamp		    i4 not null not default
			    is 'exp.dmf.lk.llb_stamp' 
,    llb_tick		    i4 not null not default
			    is 'exp.dmf.lk.llb_tick'

)
as import from 'tables'
with dbms = IMA,
     structure = unique sortkeyed,
     key = (vnode, llb_id_id);
\p\g

register table imp_lkmo_lkb (

    vnode	    varchar(64) not null not default is 'VNODE',

/*
    lkb_id_instance	    i4 not null not default
			    is 'exp.dmf.lk.lkb_id.id_instance',
*/
    lkb_id_id		    i4 not null not default
			    is 'exp.dmf.lk.lkb_id.id_id',
    lkb_count		    i4 not null not default
			    is 'exp.dmf.lk.lkb_count',
    lkb_grant_mode		char(10) not null not default
			    is 'exp.dmf.lk.lkb_grant_mode',
    lkb_grant_mode_num		i4 not null not default
			    is 'exp.dmf.lk.lkb_grant_mode_num',
    lkb_request_mode		char(10) not null not default
			    is 'exp.dmf.lk.lkb_request_mode',
    lkb_request_mode_num	i4 not null not default
			    is 'exp.dmf.lk.lkb_request_mode_num',
    lkb_state			char(100) not null not default
			    is 'exp.dmf.lk.lkb_state',
    lkb_state_num		i4 not null not default
			    is 'exp.dmf.lk.lkb_state_num',
    lkb_attribute		char(100) not null not default
			    is 'exp.dmf.lk.lkb_attribute',
    lkb_attribute_num		i4 not null not default
			    is 'exp.dmf.lk.lkb_attribute_num',
    lkb_rsb_id_id	    i4 not null not default
			    is 'exp.dmf.lk.lkb_rsb_id_id',
    lkb_llb_id_id	    i4 not null not default
			    is 'exp.dmf.lk.lkb_llb_id_id'
)
as import from 'tables'
with dbms = IMA,
     structure = unique sortkeyed,
     key = (vnode, lkb_id_id);
\p\g


register table imp_lkmo_rsb (

    vnode	    varchar(64) not null not default is 'VNODE',

/* rsb_id_instance	    i4 not null not default
			    is 'exp.dmf.lk.rsb_id.id_instance', 
*/
    rsb_id_id		    i4 not null not default
			    is 'exp.dmf.lk.rsb_id.id_id',
    rsb_grant_mode	    char(4) not null not default
			    is 'exp.dmf.lk.rsb_grant_mode',
    rsb_convert_mode	    char(4) not null not default
			    is 'exp.dmf.lk.rsb_convert_mode',
    rsb_name 		    varchar(50) not null not default
			    is 'exp.dmf.lk.rsb_name',
    rsb_name0		    i4 not null not default
			    is 'exp.dmf.lk.rsb_name0',
    rsb_name1		    i4 not null not default
			    is 'exp.dmf.lk.rsb_name1',
    rsb_name2		    i4 not null not default
			    is 'exp.dmf.lk.rsb_name2',
    rsb_name3		    i4 not null not default
			    is 'exp.dmf.lk.rsb_name3',
    rsb_name4		    i4 not null not default
			    is 'exp.dmf.lk.rsb_name4',
    rsb_name5		    i4 not null not default
			    is 'exp.dmf.lk.rsb_name5',
    rsb_name6		    i4 not null not default
			    is 'exp.dmf.lk.rsb_name6',
    rsb_value0		    i4 not null not default
			    is 'exp.dmf.lk.rsb_value0',
    rsb_value1		    i4 not null not default
			    is 'exp.dmf.lk.rsb_value1',
    rsb_invalid		    i4 not null not default
			    is 'exp.dmf.lk.rsb_invalid'

)
as import from 'tables'
with dbms = IMA,
     structure = unique sortkeyed,
     key = (vnode, rsb_id_id);
\p\g

grant select on  imp_lkmo_lkd to public;\p\g
grant select on  imp_lkmo_lkd_stat to public;\p\g
grant select on  imp_lkmo_llb to public;\p\g
grant select on  imp_lkmo_rsb to public;\p\g
grant select on  imp_lkmo_lkb to public;\p\g

commit work;
\p\g
drop view imp_lk_summary_view ;
create view imp_lk_summary_view as select
	lkd.vnode,
	lkd_rsh_size,		/* size of resource has table */
	lkd_lkh_size,		/* size of lock hash table */

	lkd_max_lkb,		/* locks per transaction */

	lkd_stat_create_list, 	/* lock lists created */
	lkd_stat_release_all,	/* lock lists released */

	lkd_llb_inuse,		/* lock lists in use */
	((lkd_lbk_size + 1) - lkd_llb_inuse) as lock_lists_remaining,
				/* lock lists remaining */
	(lkd_lbk_size + 1) as lock_lists_available,	
				/* total lock lists available */
	lkd_lkb_inuse,		/* locks in use */
	((lkd_sbk_size + 1) - lkd_lkb_inuse) as locks_remaining,
				/* locks remaining */
	(lkd_sbk_size + 1) as locks_available,
				/* total locks available */

	lkd_rsb_inuse,		/* Resource Blocks in use */

	lkd_stat_request_new,	/* locks requested */
	lkd_stat_request_cvt,	/* locks re-requested */
	lkd_stat_convert,	/* locks converted */
	lkd_stat_release,	/* locks released */
	lkd_stat_cancel,	/* locks cancelled */
	lkd_stat_rlse_partial,	/* locks escalated */

	lkd_stat_dlock_srch,	/* Deadlock Search */
	lkd_stat_cvt_deadlock,	/* Convert Deadlock */
	lkd_stat_deadlock,	/* deadlock */

	lkd_stat_convert_wait,	/* convert wait */
	lkd_stat_wait		/* lock wait */


from 	imp_lkmo_lkd lkd,
	imp_lkmo_lkd_stat stat
where
	lkd.vnode = stat.vnode;

grant select on imp_lk_summary_view to public;

\p\g

/*
** REGISTER TABLE statements which register the current LG MO data structures
** for retrieval through the IMA MO Gateway.
**
## History:
##	6-oct-1992 (bryanp)
##	    Created for the new logging system.
##	10-November-1992 (rmuth)
##	    Remove lgd_n_servers
##	13-nov-1992 (bryanp)
##	    Add new formatted log address fields.
##	14-Dec-1992 (daveb)
##	    Revamped for 12/14 register syntax.
##	18-jan-1993 (bryanp)
##	    Add more MO objects for XA_RECOVER support.
##	26-apr-1993 (bryanp)
##	    More work following 6.5 cluster support.
##	20-sep-1993 (mikem)
##	    Updated register statement to reflect new lfb_archive_window_*
##	    fields.
##	31-jan-1994 (mikem)
##	    bug #57045
##	    Remove registering for the following fields: lgd_stall_limit, 
##	    lgd_dmu_cnt, lxb_dmu_cnt.  These fields were removed from the code
##	    in the 18-oct-93 integration.  Also fixed up some other fields
##	    that had become out of date with lgmo.c (because of naming changes).
##	24-feb-1994 (johna)
##	    changed vnode to 64 characters
*/

remove imp_lgmo_lgd;\p\g
remove imp_lgmo_lfb;\p\g
remove imp_lgmo_lxb;\p\g
remove imp_lgmo_lpb;\p\g
remove imp_lgmo_ldb;\p\g

register table imp_lgmo_lgd (

    vnode	varchar(64)  not null not default is 'VNODE',

    lgd_status	    char(100) not null not default is 'exp.dmf.lg.lgd_status',
    lgd_status_num  i4 not null not default is 'exp.dmf.lg.lgd_status_num',
    lgd_lpb_inuse   i4 not null not default is 'exp.dmf.lg.lgd_lpb_inuse',
    lgd_lxb_inuse   i4 not null not default is 'exp.dmf.lg.lgd_lxb_inuse',
    lgd_ldb_inuse   i4 not null not default is 'exp.dmf.lg.lgd_ldb_inuse',
    lgd_lpd_inuse   i4 not null not default is 'exp.dmf.lg.lgd_lpd_inuse',
    lgd_protect_count	i4 not null not default
					is 'exp.dmf.lg.lgd_protect_count',
    lgd_n_logwriters	i4 not null not default
					is 'exp.dmf.lg.lgd_n_logwriters',
    lgd_no_bcp	    i4 not null not default is 'exp.dmf.lg.lgd_no_bcp',
/*
    lgd_sbk_count   i4 not null not default is 'exp.dmf.lg.lgd_sbk_count',
    lgd_sbk_size    i4 not null not default is 'exp.dmf.lg.lgd_sbk_size',
    lgd_lbk_count   i4 not null not default is 'exp.dmf.lg.lgd_lbk_count',
    lgd_lbk_size    i4 not null not default is 'exp.dmf.lg.lgd_lbk_size',

*/
    lgd_cnodeid		    i4 not null not default
				is 'exp.dmf.lg.lgd_cnodeid',
    lgd_csp_pid		    i4 not null not default
				is 'exp.dmf.lg.lgd_csp_pid',
    lgd_stat_add	    i4 not null not default
				is 'exp.dmf.lg.lgd_stat.add',
    lgd_stat_remove	    i4 not null not default
				is 'exp.dmf.lg.lgd_stat.remove',
    lgd_stat_begin	    i4 not null not default
				is 'exp.dmf.lg.lgd_stat.begin',
    lgd_stat_end	    i4 not null not default
				is 'exp.dmf.lg.lgd_stat.end',
    lgd_stat_write	    i4 not null not default
				is 'exp.dmf.lg.lgd_stat.write',
    lgd_stat_split	    i4 not null not default
				is 'exp.dmf.lg.lgd_stat.split',
    lgd_stat_force	    i4 not null not default
				is 'exp.dmf.lg.lgd_stat.force',
    lgd_stat_readio	    i4 not null not default
				is 'exp.dmf.lg.lgd_stat.readio',
    lgd_stat_writeio	    i4 not null not default
				is 'exp.dmf.lg.lgd_stat.writeio',
    lgd_stat_wait	    i4 not null not default
				is 'exp.dmf.lg.lgd_stat.wait',
    lgd_stat_group_force    i4 not null not default
				is 'exp.dmf.lg.lgd_stat.group_force',
    lgd_stat_group_count    i4 not null not default
				is 'exp.dmf.lg.lgd_stat.group_count',
    lgd_stat_inconsist_db   i4 not null not default
				is 'exp.dmf.lg.lgd_stat.inconsist_db',
    lgd_stat_pgyback_check  i4 not null not default
				is 'exp.dmf.lg.lgd_stat.pgyback_check',
    lgd_stat_pgyback_write  i4 not null not default
				is 'exp.dmf.lg.lgd_stat.pgyback_write',
    lgd_stat_kbytes	    i4 not null not default
				is 'exp.dmf.lg.lgd_stat.kbytes',
    lgd_stat_free_wait	    i4 not null not default
				is 'exp.dmf.lg.lgd_stat.free_wait',
    lgd_stat_stall_wait	    i4 not null not default
				is 'exp.dmf.lg.lgd_stat.stall_wait',
    lgd_stat_bcp_stall_wait i4 not null not default
				is 'exp.dmf.lg.lgd_stat.bcp_stall_wait',
    lgd_stat_log_readio	    i4 not null not default
				is 'exp.dmf.lg.lgd_stat.log_readio',
    lgd_stat_dual_readio    i4 not null not default
				is 'exp.dmf.lg.lgd_stat.dual_readio',
    lgd_stat_log_writeio    i4 not null not default
				is 'exp.dmf.lg.lgd_stat.log_writeio',
    lgd_stat_dual_writeio   i4 not null not default
				is 'exp.dmf.lg.lgd_stat.dual_writeio',

    lgd_cpstall		    i4 not null not default
				is 'exp.dmf.lg.lgd_cpstall',
    lgd_check_stall	    i4 not null not default
				is 'exp.dmf.lg.lgd_check_stall',
    lgd_gcmt_threshold	    i4 not null not default
				is 'exp.dmf.lg.lgd_gcmt_threshold',
    lgd_gcmt_numticks	    i4 not null not default
				is 'exp.dmf.lg.lgd_gcmt_numticks'

)
as import from 'tables'
with dbms = IMA,
     structure = sortkeyed,
     key = (vnode);
\p\g
    
register table imp_lgmo_lfb (

    vnode	varchar(64)  not null not default is 'VNODE',
    lfb_status		    char(100) not null not default
				is 'exp.dmf.lg.lfb_status',
    lfb_status_num	    i4 not null not default
				is 'exp.dmf.lg.lfb_status_num',
    lfb_hdr_lgh_version	    i4 not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_version',
    lfb_hdr_lgh_checksum    i4 not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_checksum',
    lfb_hdr_lgh_size	    i4 not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_size',
    lfb_hdr_lgh_count	    i4 not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_count',
    lfb_hdr_lgh_status	    char(100) not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_status',
    lfb_hdr_lgh_status_num  i4 not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_status_num',
    lfb_hdr_lgh_l_logfull   i4 not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_l_logfull',
    lfb_hdr_lgh_l_abort	    i4 not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_l_abort',
    lfb_hdr_lgh_l_cp	    i4 not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_l_cp',
    lfb_hdr_lgh_cpcnt	    i4 not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_cpcnt',
    lfb_hdr_lgh_tran_id	    char (20) not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_tran_id',
    lfb_hdr_lgh_tran_high   i4 not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_tran_high',
    lfb_hdr_lgh_tran_low    i4 not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_tran_low',
    lfb_hdr_lgh_begin	    char (30) not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_begin',
    lfb_hdr_lgh_begin_seq   i4 not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_begin_seq',
    lfb_hdr_lgh_begin_off   i4 not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_begin_off',
    lfb_hdr_lgh_end	    char (30) not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_end',
    lfb_hdr_lgh_end_seq	    i4 not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_end_seq',
    lfb_hdr_lgh_end_off	    i4 not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_end_off',
    lfb_hdr_lgh_cp	    char (30) not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_cp',
    lfb_hdr_lgh_cp_seq	    i4 not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_cp_seq',
    lfb_hdr_lgh_cp_off	    i4 not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_cp_off',
    lfb_hdr_lgh_active_logs i4 not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_active_logs',
    lfb_forced_lga	    char(30) not null not default
				is 'exp.dmf.lg.lfb_forced_lga',
    lfb_forced_high	    i4 not null not default
				is 'exp.dmf.lg.lfb_forced_lga.la_sequence',
    lfb_forced_low	    i4 not null not default
				is 'exp.dmf.lg.lfb_forced_lga.la_offset',
    lfb_forced_lsn	    char (20) not null not default
				is 'exp.dmf.lg.lfb_forced_lsn',
    lfb_forced_lsn_high	    i4 not null not default
				is 'exp.dmf.lg.lfb_forced_lsn_high',
    lfb_forced_lsn_low	    i4 not null not default
				is 'exp.dmf.lg.lfb_forced_lsn_low',
/*
    lfb_last_lsn	    char (20) not null not default
				is 'exp.dmf.lg.lfb_last_lsn',
    lfb_last_lsn_high	    i4 not null not default
				is 'exp.dmf.lg.lfb_last_lsn_high',
    lfb_last_lsn_low	    i4 not null not default
				is 'exp.dmf.lg.lfb_last_lsn_low',
*/
    lfb_archive_start_lga   char(30) not null not default
				is 'exp.dmf.lg.lfb_archive_start',
    lfb_archive_start_high  i4 not null not default
				is 'exp.dmf.lg.lfb_archive_start.la_sequence',
    lfb_archive_start_low   i4 not null not default
				is 'exp.dmf.lg.lfb_archive_start.la_offset',
    lfb_archive_end_lga	    char(30) not null not default
				is 'exp.dmf.lg.lfb_archive_end',
    lfb_archive_end_high    i4 not null not default
				is 'exp.dmf.lg.lfb_archive_end.la_sequence',
    lfb_archive_end_low	i4 not null not default
				is 'exp.dmf.lg.lfb_archive_end.la_offset',
    lfb_archive_prevcp_lga  char(30) not null not default
				is 'exp.dmf.lg.lfb_archive_prevcp',
    lfb_archive_prevcp_high i4 not null not default
				is 'exp.dmf.lg.lfb_archive_prevcp.la_sequence',
    lfb_archive_prevcp_low  i4 not null not default
				is 'exp.dmf.lg.lfb_archive_prevcp.la_offset',
    lfb_active_log	    char (40) not null not default
				is 'exp.dmf.lg.lfb_active_log',
    lfb_active_log_num	    i4 not null not default
				is 'exp.dmf.lg.lfb_active_log_num',
    lfb_buf_cnt		    i4 not null not default
				is 'exp.dmf.lg.lfb_buf_cnt',
    lfb_reserved_space	    i4 not null not default
				is 'exp.dmf.lg.lfb_reserved_space',
    lfb_channel_blk	    i4 not null not default
				is 'exp.dmf.lg.lfb_channel_blk',
    lfb_dual_channel_blk    i4 not null not default
				is 'exp.dmf.lg.lfb_dual_channel_blk',
    lfb_stat_split	    i4 not null not default
				is 'exp.dmf.lg.lfb_stat_split',
    lfb_stat_write	    i4 not null not default
				is 'exp.dmf.lg.lfb_stat_write',
    lfb_stat_force	    i4 not null not default
				is 'exp.dmf.lg.lfb_stat_force',
    lfb_stat_wait	    i4 not null not default
				is 'exp.dmf.lg.lfb_stat_wait',
    lfb_stat_end	    i4 not null not default
				is 'exp.dmf.lg.lfb_stat_end',
    lfb_stat_writeio	    i4 not null not default
				is 'exp.dmf.lg.lfb_stat_writeio',
    lfb_stat_kbytes	    i4 not null not default
				is 'exp.dmf.lg.lfb_stat_kbytes',
    lfb_stat_log_readio	    i4 not null not default
				is 'exp.dmf.lg.lfb_stat_log_readio',
    lfb_stat_dual_readio    i4 not null not default
				is 'exp.dmf.lg.lfb_stat_dual_readio',
    lfb_stat_log_writeio    i4 not null not default
				is 'exp.dmf.lg.lfb_stat_log_writeio',
    lfb_stat_dual_writeio   i4 not null not default
				is 'exp.dmf.lg.lfb_stat_dual_writeio'

)
as import from 'tables'
with dbms = IMA,
     structure = sortkeyed,
     key = (vnode);
\p\g
    
register table imp_lgmo_lpb (

    vnode	varchar(64)  not null not default is 'VNODE',

    lpb_id_instance	    i4 not null not default
				is 'exp.dmf.lg.lpb_id.id_instance',
    lpb_id_id		    i4 not null not default
				is 'exp.dmf.lg.lpb_id.id_id',
    lpb_status		    char(100) not null not default
				is 'exp.dmf.lg.lpb_status',
    lpb_status_num	    i4 not null not default
				is 'exp.dmf.lg.lpb_status_num',
    lpb_lpd_count	    i4 not null not default
				is 'exp.dmf.lg.lpb_lpd_count',
    lpb_pid		    i4 not null not default
				is 'exp.dmf.lg.lpb_pid',
    lpb_cond		    i4 not null not default
				is 'exp.dmf.lg.lpb_cond',
    lpb_bufmgr_id	    i4 not null not default
				is 'exp.dmf.lg.lpb_bufmgr_id',
    lpb_force_abort_sid	    varchar(32) not null not default
				is 'exp.dmf.lg.lpb_force_abort_sid',
    lpb_gcmt_sid	    varchar(32) not null not default
				is 'exp.dmf.lg.lpb_gcmt_sid',
    lpb_gcmt_asleep	    i4 not null not default
				is 'exp.dmf.lg.lpb_gcmt_asleep',
    lpb_stat_readio	    i4 not null not default
				is 'exp.dmf.lg.lpb_stat.readio',
    lpb_stat_write	    i4 not null not default
				is 'exp.dmf.lg.lpb_stat.write',
    lpb_stat_force	    i4 not null not default
				is 'exp.dmf.lg.lpb_stat.force',
    lpb_stat_wait	    i4 not null not default
				is 'exp.dmf.lg.lpb_stat.wait',
    lpb_stat_begin	    i4 not null not default
				is 'exp.dmf.lg.lpb_stat.begin',
    lpb_stat_end	    i4 not null not default
				is 'exp.dmf.lg.lpb_stat.end'

)
as import from 'tables'
with dbms = IMA,
     structure = unique sortkeyed,
     key = (vnode, lpb_id_id);
\p\g

register table imp_lgmo_ldb (

    vnode	varchar(64)  not null not default is 'VNODE',

    ldb_id_instance	    i4 not null not default
				is 'exp.dmf.lg.ldb_id.id_instance',
    ldb_id_id		    i4 not null not default
				is 'exp.dmf.lg.ldb_id.id_id',
    ldb_status		    char(100) not null not default
				is 'exp.dmf.lg.ldb_status',
    ldb_status_num	    i4 not null not default
				is 'exp.dmf.lg.ldb_status_num',
    ldb_lpd_count	    i4 not null not default
				is 'exp.dmf.lg.ldb_lpd_count',
    ldb_lxb_count	    i4 not null not default
				is 'exp.dmf.lg.ldb_lxb_count',
    ldb_lxbo_count	    i4 not null not default
				is 'exp.dmf.lg.ldb_lxbo_count',
    ldb_database_id	    i4 not null not default
				is 'exp.dmf.lg.ldb_database_id',
    ldb_l_buffer	    i4 not null not default
				is 'exp.dmf.lg.ldb_l_buffer',
    ldb_buffer		    char(200) not null not default
				is 'exp.dmf.lg.ldb_buffer',
    ldb_db_name		    char (32) not null not default
				is 'exp.dmf.lg.ldb_db_name',
    ldb_db_owner	    char (32) not null not default
				is 'exp.dmf.lg.ldb_db_owner',
    ldb_j_first_la	    char (30) not null not default
				is 'exp.dmf.lg.ldb_j_first_la',
    ldb_j_first_high	    i4 not null not default
				is 'exp.dmf.lg.ldb_j_first_la.la_sequence',
    ldb_j_first_low	    i4 not null not default
				is 'exp.dmf.lg.ldb_j_first_la.la_offset',
    ldb_j_last_la	    char (30) not null not default
				is 'exp.dmf.lg.ldb_j_last_la',
    ldb_j_last_la_high	    i4 not null not default
				is 'exp.dmf.lg.ldb_j_last_la.la_sequence',
    ldb_j_last_la_low	    i4 not null not default
				is 'exp.dmf.lg.ldb_j_last_la.la_offset',
    ldb_d_first_la	    char (30) not null not default
				is 'exp.dmf.lg.ldb_d_first_la',
    ldb_d_first_high	    i4 not null not default
				is 'exp.dmf.lg.ldb_d_first_la.la_sequence',
    ldb_d_first_low	    i4 not null not default
				is 'exp.dmf.lg.ldb_d_first_la.la_offset',
    ldb_d_last_la	    char (30) not null not default
				is 'exp.dmf.lg.ldb_d_last_la',
    ldb_d_last_high	    i4 not null not default
				is 'exp.dmf.lg.ldb_d_last_la.la_sequence',
    ldb_d_last_low	    i4 not null not default
				is 'exp.dmf.lg.ldb_d_last_la.la_offset',
    ldb_sbackup_lga	    char (30) not null not default
				is 'exp.dmf.lg.ldb_sbackup',
    ldb_sbackup_high	    i4 not null not default
				is 'exp.dmf.lg.ldb_sbackup.la_sequence',
    ldb_sbackup_low	    i4 not null not default
				is 'exp.dmf.lg.ldb_sbackup.la_offset',
    ldb_sback_lsn	    char (20) not null not default
				is 'exp.dmf.lg.ldb_sback_lsn',
    ldb_sback_lsn_high	    i4 not null not default
				is 'exp.dmf.lg.ldb_sback_lsn_high',
    ldb_sback_lsn_low	    i4 not null not default
				is 'exp.dmf.lg.ldb_sback_lsn_low',
    ldb_stat_read	    i4 not null not default
				is 'exp.dmf.lg.ldb_stat.read',
    ldb_stat_write	    i4 not null not default
				is 'exp.dmf.lg.ldb_stat.write',
    ldb_stat_begin	    i4 not null not default
				is 'exp.dmf.lg.ldb_stat.begin',
    ldb_stat_end	    i4 not null not default
				is 'exp.dmf.lg.ldb_stat.end',
    ldb_stat_force	    i4 not null not default
				is 'exp.dmf.lg.ldb_stat.force',
    ldb_stat_wait	    i4 not null not default
				is 'exp.dmf.lg.ldb_stat.wait'

)
as import from 'tables'
with dbms = IMA,
     structure = unique sortkeyed,
     key = (vnode, ldb_id_id);
\p\g

register table imp_lgmo_lxb (

    vnode	varchar(64)  not null not default is 'VNODE',

    lxb_id_instance	    i4 not null not default
				is 'exp.dmf.lg.lxb_id.id_instance',
    lxb_id_id		    i4 not null not default
				is 'exp.dmf.lg.lxb_id.id_id',
    lxb_status		    char(100) not null not default
				is 'exp.dmf.lg.lxb_status',
    lxb_status_num	    i4 not null not default
				is 'exp.dmf.lg.lxb_status_num',
    lxb_db_name		    char (32) not null not default
				is 'exp.dmf.lg.lxb_db_name',
    lxb_db_owner	    char (32) not null not default
				is 'exp.dmf.lg.lxb_db_owner',
    lxb_db_id_id	    i4 not null not default
				is 'exp.dmf.lg.lxb_db_id_id',
    lxb_pr_id_id	    i4 not null not default
				is 'exp.dmf.lg.lxb_pr_id_id',
    lxb_wait_reason	    char(20) not null not default
				is 'exp.dmf.lg.lxb_wait_reason',
    lxb_wait_reason_num	    i4 not null not default
				is 'exp.dmf.lg.lxb_wait_reason_num',
    lxb_sequence	    i4 not null not default
				is 'exp.dmf.lg.lxb_sequence',
    lxb_first_lga	    char (30) not null not default
				is 'exp.dmf.lg.lxb_first_lga',
    lxb_first_high	    i4 not null not default
				is 'exp.dmf.lg.lxb_first_lga.la_sequence',
    lxb_first_low	    i4 not null not default
				is 'exp.dmf.lg.lxb_first_lga.la_offset',
    lxb_last_lga	    char (30) not null not default
				is 'exp.dmf.lg.lxb_last_lga',
    lxb_last_high	    i4 not null not default
				is 'exp.dmf.lg.lxb_last_lga.la_sequence',
    lxb_last_low	    i4 not null not default
				is 'exp.dmf.lg.lxb_last_lga.la_offset',
    lxb_last_lsn	    char (30) not null not default
				is 'exp.dmf.lg.lxb_last_lsn',
    lxb_last_lsn_high	    i4 not null not default
				is 'exp.dmf.lg.lxb_last_lsn_high',
    lxb_last_lsn_low	    i4 not null not default
				is 'exp.dmf.lg.lxb_last_lsn_low',
    lxb_cp_lga		    char (30) not null not default
				is 'exp.dmf.lg.lxb_cp_lga',
    lxb_cp_high		    i4 not null not default
				is 'exp.dmf.lg.lxb_cp_lga.la_sequence',
    lxb_cp_low		    i4 not null not default
				is 'exp.dmf.lg.lxb_cp_lga.la_offset',
    lxb_tran_id		    char(32) not null not default
				is 'exp.dmf.lg.lxb_tran_id',
    lxb_tran_high	    i4 not null not default
				is 'exp.dmf.lg.lxb_tran_id.db_high_tran',
    lxb_tran_low	    i4 not null not default
				is 'exp.dmf.lg.lxb_tran_id.db_low_tran',
    lxb_dis_tran_id_hexdump char(350) not null not default
				is 'exp.dmf.lg.lxb_dis_tran_id_hexdump',
    lxb_pid		    i4 not null not default
				is 'exp.dmf.lg.lxb_pid',
    lxb_sid		    varchar(32) not null not default
				is 'exp.dmf.lg.lxb_sid',
    lxb_reserved_space	    i4 not null not default
				is 'exp.dmf.lg.lxb_reserved_space',
    lxb_user_name	    char(100) not null not default
				is 'exp.dmf.lg.lxb_user_name',
    lxb_is_prepared	    char (1) not null not default
				is 'exp.dmf.lg.lxb_is_prepared',
    lxb_is_xa_dis_tran_id   char (1) not null not default
				is 'exp.dmf.lg.lxb_is_xa_dis_tran_id',
    lxb_stat_split	    i4 not null not default
				is 'exp.dmf.lg.lxb_stat.split',
    lxb_stat_write	    i4 not null not default
				is 'exp.dmf.lg.lxb_stat.write',
    lxb_stat_force	    i4 not null not default
				is 'exp.dmf.lg.lxb_stat.force',
    lxb_stat_wait	    i4 not null not default
				is 'exp.dmf.lg.lxb_stat.wait'

)
as import from 'tables'
with dbms = IMA,
     structure = unique sortkeyed,
     key = (vnode, lxb_id_id);
\p\g

commit work;
\p\g
drop view imp_lgmo_xa_dis_tran_ids;
\p\g
create view lgmo_xa_dis_tran_ids (xa_dis_tran_id, xa_database_name) as
	select lxb_dis_tran_id_hexdump, lxb_db_name
	    from imp_lgmo_lxb
	    where lxb_is_prepared = 'Y' and lxb_is_xa_dis_tran_id = 'Y';
\p\g
grant select on imp_lgmo_lgd to public;\p\g
grant select on imp_lgmo_lfb to public;\p\g
grant select on imp_lgmo_lxb to public;\p\g
grant select on imp_lgmo_lpb to public;\p\g
grant select on imp_lgmo_ldb to public;\p\g

/*
** Client info
*/
register table imp_client_info (

	server varchar(64) not null not default
		is 'SERVER',
	session_id varchar(32) not null not default
		is 'exp.scf.scs.scb_index',
	client_host varchar(20) not null not default 
		is 'exp.scf.scs.scb_client_host',
	client_pid varchar(20) not null not default
		is 'exp.scf.scs.scb_client_pid',
	client_tty varchar(20) not null not default
		is 'exp.scf.scs.scb_client_tty',
	client_user varchar(20) not null not default
		is 'exp.scf.scs.scb_client_user',
	client_info varchar(64) not null not default
		is 'exp.scf.scs.scb_client_info',
	client_connect varchar(64) not null not default
		is 'exp.scf.scs.scb_client_connect'
) 
as import from 'tables'
with dbms = IMA,
structure = unique sortkeyed,
key = ( server,session_id  )

\p\g

grant select on imp_client_info to public\g



create table imp_lk_type(
	lk_int smallint not null,
	lk_name varchar(32) not null
)
with duplicates,
location = (ii_database),
security_audit=(table,norow)
;
modify imp_lk_type to heap
with extend = 16,
	allocation = 4
\p\g
copy imp_lk_type(
	lk_int= c0tab,
	lk_name= varchar(0)nl,
	nl= d1)
from 'imp_lk_t._in'
with row_estimate = 0
\p\g
grant select on imp_lk_type to public\p\g




	/* IMADB REGISTRATIONS */
commit work;
\p\g
  drop table imp_dmf_cache_stats 
\p\g
  register table imp_dmf_cache_stats ( 
server varchar(64) not null not default is 'SERVER',
       page_size integer4 not null not default     
           is 'exp.dmf.dm0p.bm_pgsize',            
  force_count integer4 not null not default is 'exp.dmf.dm0p.bm_force',
 io_wait_count integer4 not null not default is 'exp.dmf.dm0p.bm_iowait',
 group_buffer_read_count integer4 not null not default is 'exp.dmf.dm0p.bm_greads',
 group_buffer_write_count integer4 not null not default is 'exp.dmf.dm0p.bm_gwrites',
 fix_count integer4 not null not default is 'exp.dmf.dm0p.bm_fix',
 unfix_count integer4 not null not default is 'exp.dmf.dm0p.bm_unfix',
 read_count integer4 not null not default is 'exp.dmf.dm0p.bm_reads',
 write_count integer4 not null not default is 'exp.dmf.dm0p.bm_writes',
 hit_count integer4 not null not default is 'exp.dmf.dm0p.bm_hit',
 dirty_unfix_count integer4 not null not default is 'exp.dmf.dm0p.bm_dirty',
 pages_still_valid integer4 not null not default is 'exp.dmf.dm0p.bm_check',
 pages_invalid integer4 not null not default is 'exp.dmf.dm0p.bm_refresh',
  buffer_count integer4 not null not default is 'exp.dmf.dm0p.bm_bufcnt',
 page_buffer_count integer4 not null not default is 'exp.dmf.dm0p.bm_sbufcnt',
 flimit integer4 not null not default is 'exp.dmf.dm0p.bm_flimit',
 mlimit integer4 not null not default is 'exp.dmf.dm0p.bm_mlimit',
 wbstart integer4 not null not default is 'exp.dmf.dm0p.bm_wbstart',
 wbend integer4 not null not default is 'exp.dmf.dm0p.bm_wbend',
 hash_bucket_count integer4 not null not default is 'exp.dmf.dm0p.bm_hshcnt',
  group_buffer_count integer4 not null not default is 'exp.dmf.dm0p.bm_gcnt',
 group_buffer_size integer4 not null not default is 'exp.dmf.dm0p.bm_gpages',
 cache_status integer4 not null not default is 'exp.dmf.dm0p.bm_status',
  free_buffer_count integer4 not null not default is 'exp.dmf.dm0p.bm_fcount',
 free_buffer_waiters integer4 not null not default is 'exp.dmf.dm0p.bm_fwait',
 fixed_buffer_count integer4 not null not default is 'exp.dmf.dm0p.bm_lcount',
 modified_buffer_count integer4 not null not default is 'exp.dmf.dm0p.bm_mcount',
 free_group_buffer_count integer4 not null not default is 'exp.dmf.dm0p.bm_gfcount',
 fixed_group_buffer_count integer4 not null not default is 'exp.dmf.dm0p.bm_glcount',
 modified_group_buffer_count integer4 not null not default is 'exp.dmf.dm0p.bm_gmcount' 
/* 
##mdf040903 
Added extra Wait info */
,fc_wait integer4 not null not default is 'exp.dmf.dm0p.bm_fcwait'
, bm_gwait integer4 not null not default is 'exp.dmf.dm0p.bm_gwait'
, bm_mwait integer4 not null not default is 'exp.dmf.dm0p.bm_mwait'
--, bmc_wait integer4 not null not default is 'exp.dmf.dm0p.bmc_bmcwait'
) as import from 'tables' with dbms = IMA,

 structure = sortkeyed,
 key = (server)
\p\g

	/* PERMISSIONS */
grant select on "$ingres".imp_dmf_cache_stats to public 
\p\g                                                     
