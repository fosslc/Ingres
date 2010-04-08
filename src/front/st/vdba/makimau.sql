/*
** Table registrations for the IMA Standard Schema - UNIX only
**
## History:
##
##	30-Apr-95 (johna)
##		Created as a simplified (and clearer) version of the 
##		schema from the earlier IMA-based IMP tools.
##	15-May-95 (johna)
##		Used 'set session authorization' to prevent the need 
##		for the -u flag. Also added ima_gca_connections.
##	9-Aug-96 (johna)
##		Adapted for the standard schema from earlier work. Once 
##		complete, this schema should be added to the createdb set
##		Added:
##			ima_gcn_info
##	4-Sep-96 (johna)
##		Reworked to provide a 'static' table of information 
##		(values fixed at installation start time) and 'dynamic' tables
##		of information which may change during the life of the
##		installation.
##	20-Sep-96 (johna)
##		Completed read-only instrumentation of iigcn
##	06-Nov-1997 (hanch04)
##	    Changed history comments
##      26-Nov-1997 (kinte01)
##          Changed grants from ingres to public. There may not necessarily be
##          an ingres user on OpenVMS. Implementing Johna's suggestion off making
##          the grants to public
##      04-Dec-1997 (kinte01)
##          Reversed previous change as this file is UNIX specific
##      14-Jan-98 (fanra01)
##          Extracted the buffer management stats from the
##          ima_dmf_cache_stats table and add them in their own table
##          ima_dmf_buffer_manager_stats.
##          Add ima_dmf_bm_configuration for config parameters.
##      18-Jun-98 (fanra01)
##          Changed session_id in ima_server_sessions to be scb_self to match
##          what is used in the locklist_session.
##          Changed session_id in ima_server_sessions_extra to be cs_self to
##          match ima_server_sessions.
##          Update the ima_remove_session procedure to get correct session
##          reference to close session.
##      19-Oct-98 (fanra01)
##          Add exp.scf.scd.start.name for the server flavour to
##          ima_dbms_server_parameters.
##      09-Nov-98 (fanra01)
##          Add icesvr ima objects.
##          Add procedures to open and close a server.
##      20-Jan-99 (wonst02)
##          Added cssampler tables.
##          Added event types to ima_cssampler_threads table.
##          Removed extraneous _ from max_/min_/norm_ thread__priority.
##      16-Feb-1999 (fanra01)
##          Removed fields from script ima_lgmo_lfb.  These fields were removed
##          as part of the largefile64 project and add new fields.
##	25-aug-1999 (somsa01)
##	    Added the registration of procedures ima_start_sampler and
##	    ima_stop_sampler, which will start/stop the Sampler thread
##	    for a particular server.
##      14-Dec-1999 (fanra01)
##          Sir 99785
##          Add registration for ima_server_sessions_desc table which contains
##          the scb_description field from SET SESSION WITH DESCRIPTION=.
##	14-feb-2001 (somsa01)
##	    Modified the grant option on ima_gcn_registrations,
##	    ima_set_vnode_domain, ima_dmf_cache_stats, ima_locks,
##	    ima_cssampler_threads, and ima_cssampler_stats to public
##	    read access. This is so that the Ingres performance monitor
##	    stuff can be run by anyone.
##      12-Feb-2003 (fanra01)
##          Bug 109284
##          Add procedure ima_drop_sessions that can terminate all 
##          sessions within the  local server.
##  30-Jun-2004 (noifr01)
##      Sir 111718
##      Added table registration for ima_config
##  20-Jul-2004 (noifr01)
##      Sir 111718
##      updated length of parameter name and length in ima_config,
##      for consistency with the API calls
##  16-Sep-2004 (somsa01)
##	Changed grants from ingres to public. The "ingres" user is going away.
##  28-Feb-2005 (fanra01)
##      Bug 113976
##      Removed mutex_wait_count and wb_flush attributes from the
##      ima_dmf_buffer_manager_stats registration.  These fields
##      are not applicable to this table through a change in 
##      write behind processing.
##  16-Mar-2005 (thaju02)
##	Removed write_behind_flush_count and mutex_waits attributes
##	from ima_dmf_bm_configuration registration. (B114101)
##  09-May-2005 (fanra01)
##      SIR 114610
##      Include session connected time and session CPU statistics
##  24-May-2005 (fanra01)
##      Bug 114612
##      Update ima_gcc_info to increase count field sizes to prevent
##      numeric truncation.
##  01-Jun-2005 (fanra01)
##      SIR 114614
##      Add ima_version and ima_instance_cnf table registrations.
##  30-Aug-2005 (fanra01)
##      Bug 115124
##      Remove ima_qef_info table registration as there are no defined
##      objects in qef.
##	7-march-2008 (dougi)
##	    Add rqp_text to ima_qsf_rqp for SIR 119414.
##	01-may-2008 (smeke01) b118780
##	    Corrected name of sc_avgrows to sc_totrows and IMA field to total_rows.
##	04-jun-2008 (joea)
##	    Correct datatype of lkd_status.
*/

set autocommit on;
\p\g
set session authorization '$ingres';
\p\g

/*
** IMA_MIB_OBJECTS - 'flat' table required for updates/procedure support
*/
drop table ima_mib_objects;
\p\g
register table ima_mib_objects (
	server varchar(64) not null not default is 
		'SERVER',
	classid varchar(64) not null not default is 
		'CLASSID',
	instance varchar(64) not null not default is 
		'INSTANCE',
	value varchar(64) not null not default is 
		'VALUE',
	perms integer2 not null not default is 
		'PERMISSIONS'
)
as import from 'objects'
with update,
dbms = IMA,
structure = unique sortkeyed,
key = (server, classid, instance);
\p\g

/*
** IMA_MO_META - 'metadata' cross tab
*/
drop ima_mo_meta;
\p\g
register table ima_mo_meta (	
	server varchar(64) not null not default is 
		'SERVER', 
	classid varchar(64) not null not default is 
		'exp.glf.mo.meta.classid', 
	class varchar(64) not null not default is 
		'exp.glf.mo.meta.class', 
	oid varchar(8) not null not default is 
		'exp.glf.mo.meta.oid', 
	perms integer2 not null not default is 
		'exp.glf.mo.meta.perms', 
	size integer2 not null not default is 
		'exp.glf.mo.meta.size', 
	xindex varchar(64) not null not default is 
		'exp.glf.mo.meta.index' 
) as import from 'tables' 
with dbms = IMA, 
structure = unique sortkeyed, 
key = (server, classid);
\p\g

/*
** Consider the Ingres installation as a whole, then look at it's 
** constituent parts in detail.
*/
drop ima_installation_info;
\p\g
register table ima_installation_info (
	vnode varchar(64) not null not default is 
		'VNODE',
	cpu_count integer4 not null not default is
		'exp.clf.unix.cs.num_processors'
) as import from 'tables'
with dbms = IMA,
structure = sortkeyed,
key = (vnode);
\p\g

/* 
** A minimal client installation will (normally) have a name server and a 
** net server, a minimal server installation will have a name server and a 
** set of 'DMF' server processes.
*/

/* 
** IMA_GCN_INFO - information about the name server
*/
drop ima_gcn_info;
\p\g
register table ima_gcn_info(
	name_server varchar(64) not null not default is
		'SERVER',
	default_class varchar(64) not null not default is
		'exp.gcf.gcn.default_class',
	local_vnode varchar(64) not null not default is
		'exp.gcf.gcn.local_vnode',
	remote_vnode varchar(64) not null not default is
		'exp.gcf.gcn.remote_vnode',
	time_now integer4 not null not default is
		'exp.gcf.gcn.now',
	time_now_str  varchar(30) not null not default is
		'exp.gcf.gcn.now_str',
	next_cleanup  integer4 not null not default is
		'exp.gcf.gcn.next_cleanup',
	next_cleanup_str  varchar(30) not null not default is
		'exp.gcf.gcn.next_cleanup_str',
	cache_modified  integer4 not null not default is
		'exp.gcf.gcn.cache_modified',
	cache_modified_str  varchar(30) not null not default is
		'exp.gcf.gcn.cache_modified_string',
	max_sessions integer4 not null not default is
		'exp.gcf.gcn.max_sessions',
	check_interval integer4 not null not default is
		'exp.gcf.gcn.check_interval',
	clustered integer4 not null not default is
		'exp.gcf.gcn.clustered',
	tracing integer4 not null not default is
		'exp.gcf.gcn.tracing',
	gcn_pid integer4 not null not default is
		'exp.gcf.gcn.gcn_pid'
)
as import from 'tables'
with dbms = IMA,
structure = sortkeyed,
key = (name_server);
\p\g


/* 
** IMA_GCN_REGISTRATIONS - all INGRES processes registered with the IIGCN
*/
drop ima_gcn_registrations;
\p\g
register table ima_gcn_registrations (
	name_server varchar(64) not null not default is
		'SERVER',
	listen_address varchar(64) not null not default is
		'exp.gcf.gcn.server.address',
  server_class varchar(32) not null not default is
		'exp.gcf.gcn.server.class',
	server_dblist varchar(32) not null not default is
		'exp.gcf.gcn.server.object'
)
as import from 'tables'
with dbms = IMA,
structure = sortkeyed,
key = (name_server);
\p\g

/* 
** IMA_GCB_INFO - information about a bridge server
*/
drop ima_gcb_info;
\p\g
register table ima_gcb_info(
	bridge_server varchar(64) not null not default is
		'SERVER',
	inbound_current integer4 not null not default is
		'exp.gcf.gcb.ib_conn_count',
	outbound_current integer4 not null not default is
		'exp.gcf.gcb.ob_conn_count',
	gcb_pid integer4 not null not default is
		'exp.gcf.gcb.gcc_pid'
)
as import from 'tables'
with dbms = IMA,
structure = sortkeyed,
key = (bridge_server);
\p\g

/* 
** IMA_GCC_INFO - information about a net server
*/
drop table ima_gcc_info;
\p\g
register table ima_gcc_info (
	net_server		    varchar(64) not null not default 
    is 'SERVER',
  inbound_max	      varchar(20) not null not default 
    is 'exp.gcf.gcc.ib_max',
	inbound_current	  varchar(20) not null not default 
    is 'exp.gcf.gcc.ib_conn_count',
	outbound_max	    varchar(20) not null not default 
    is 'exp.gcf.gcc.ob_max',
	outbound_current	varchar(20) not null not default 
    is 'exp.gcf.gcc.ob_conn_count'
) as import from 'tables'
with dbms = IMA,
structure = sortkeyed,
key = (net_server);
\p\g

/*
** Information about servers:
*/
drop ima_dbms_server_parameters;
\p\g
register table ima_dbms_server_parameters (
	server varchar(64) not null not default is
		'SERVER',
	startup_name varchar(64) not null not default is
		'exp.scf.scd.start.name',
	server_name varchar(64) not null not default is
		'exp.scf.scd.server_name',
	name_server_registration integer4 not null not default is
		'exp.scf.scd.names_reg',
	capabilities varchar(64) not null not default is
		'exp.scf.scd.capabilities_str',
	capability_values integer4 not null not default is
		'exp.scf.scd.capabilities_num',
	cursors_per_session integer4 not null not default is
		'exp.scf.scd.acc',
	cursor_flags integer4 not null not default is
		'exp.scf.scd.csrflags',
	fast_commit integer4 not null not default is
		'exp.scf.scd.fastcommit',
	query_flattening integer4 not null not default is
		'exp.scf.scd.flatflags',
	expected_row_count integer4 not null not default is
		'exp.scf.scd.irowcount', /* default 20 */
	max_thread_priority integer4 not null not default is
		'exp.scf.scd.max_priority',
	min_thread_priority integer4 not null not default is
		'exp.scf.scd.min_priority',
	norm_thread_priority integer4 not null not default is
		'exp.scf.scd.norm_priority',
	avoid_cluster_optimizations integer4 not null not default is
		'exp.scf.scd.no_star_cluster',
	avoid_2pc_recovery integer4 not null not default is
		'exp.scf.scd.nostar_recovr',
	max_threads integer4 not null not default is
		'exp.scf.scd.nousers',
	psf_memory integer4 not null not default is
		'exp.scf.scd.psf_mem',
	risk_udt_inconsistency integer4 not null not default is
		'exp.scf.scd.risk_inconsistency',
	rule_depth integer4 not null not default is
		'exp.scf.scd.rule_depth',
	max_connections integer4 not null not default is
		'exp.scf.scd.server.max_connections',
	reserved_connections integer4 not null not default is
		'exp.scf.scd.server.reserved_connections',
	write_behind_threads integer4 not null not default is
		'exp.scf.scd.writebehind',
	sole_server integer4 not null not default is
		'exp.scf.scd.soleserver',
	startup_time varchar(64) not null not default is
		'exp.scf.scd.startup_time', 
	server_pid integer4 not null not default is
		'exp.scf.scd.server.pid'
)
as import from 'tables'
with dbms = IMA,
structure = sortkeyed,
key = (server);
\p\g


drop ima_dbms_server_status;
\p\g
register table ima_dbms_server_status (
	server varchar(64) not null not default is
		'SERVER',
	total_rows integer4 not null not default is
		'exp.scf.scd.totrows',
	listen_fails integer4 not null not default is
		'exp.scf.scd.gclisten_fail',
	selects_processed integer4 not null not default is
		'exp.scf.scd.selcnt',
	current_connections integer4 not null not default is
		'exp.scf.scd.server.current_connections',
	highwater_connections integer4 not null not default is
		'exp.scf.scd.server.highwater_connections',
	listen_mask integer4 not null not default is
		'exp.scf.scd.server.listen_mask',
	listen_state varchar(20) not null not default is
		'exp.scf.scd.server.listen_state',
	shutdown_state varchar(20) not null not default is
		'exp.scf.scd.server.shutdown_state',
	server_state varchar(20) not null not default is
		'exp.scf.scd.state_str',
	server_state_mask integer4 not null not default is
		'exp.scf.scd.state_num'
)
as import from 'tables'
with dbms = IMA,
structure = sortkeyed,
key = (server);
\p\g

/*
** Control objects 
|exp.scf.scd.server.control.close                                |
|exp.scf.scd.server.control.crash                                |
|exp.scf.scd.server.control.open                                 |
|exp.scf.scd.server.control.shut                                 |
|exp.scf.scd.server.control.stop                                 |
*/

/*
** IMA_SERVER_SESSIONS - information about connected and system sessions
*/
drop table ima_server_sessions;
\p\g
register table ima_server_sessions (
	server varchar(64) not null not default is 
		'SERVER',
	session_ptr varchar(32) not null not default is
		'exp.scf.scs.scb_index',
	session_id varchar(32) not null not default is
		'exp.scf.scs.scb_self',
	effective_user varchar(32) not null not default is
        	'exp.scf.scs.scb_euser',
	real_user varchar(32) not null not default is
        	'exp.scf.scs.scb_ruser',
	db_name varchar(32) not null not default is
        	'exp.scf.scs.scb_database',
	db_owner varchar(32) not null not default is 
        	'exp.scf.scs.scb_dbowner',
	db_lock varchar(9) not null not default is
        	'exp.scf.scs.scb_dblockmode',
	server_facility varchar(10) not null not default is
        	'exp.scf.scs.scb_facility_name',
	session_activity varchar(80) not null not default is
        	'exp.scf.scs.scb_activity',
	activity_detail varchar(80) not null not default is
        	'exp.scf.scs.scb_act_detail',
	session_query varchar(1000) not null not default is
        	'exp.scf.scs.scb_query',
	session_terminal varchar(12) not null not default is
        	'exp.scf.scs.scb_terminal',
	session_group varchar(8) not null not default is
        	'exp.scf.scs.scb_group',
	session_role varchar(8) not null not default is
        	'exp.scf.scs.scb_role',
	client_host varchar(20) not null not default 
		is 'exp.scf.scs.scb_client_host',
	client_pid varchar(20) not null not default
		is 'exp.scf.scs.scb_client_pid',
	client_terminal varchar(20) not null not default
		is 'exp.scf.scs.scb_client_tty',
	client_user varchar(32) not null not default
		is 'exp.scf.scs.scb_client_user',
	client_connect_string varchar(64) not null not default
		is 'exp.scf.scs.scb_client_connect',
	client_info varchar(64) not null not default
		is 'exp.scf.scs.scb_client_info',
	application_code integer4 not null not default
    is 'exp.scf.scs.scb_appl_code',
  session_name varchar(32) not null not default
    is 'exp.scf.scs.scb_s_name',
	server_pid integer4 not null not default is
        	'exp.scf.scs.scb_pid'
)
as import from 'tables'
with dbms = IMA, structure = unique sortkeyed,
key = (server, session_id)
\p\g

drop table ima_server_sessions_desc;
\p\g
register table ima_server_sessions_desc (
	server varchar(64) not null not default is 
		'SERVER',
	session_id varchar(32) not null not default is
		'exp.scf.scs.scb_index',
	session_desc varchar(256) not null not default is
		'exp.scf.scs.scb_description'
)
as import from 'tables'
with dbms = IMA, structure = unique sortkeyed,
key = (server, session_id)
\p\g

drop table ima_server_sessions_extra;
\p\g
register table ima_server_sessions_extra (
	server varchar(64) not null not default
    is 'SERVER',
	session_id varchar(32) not null not default
    is 'exp.clf.unix.cs.scb_self',
  session_time integer not null not default
    is  'exp.clf.unix.cs.scb_connect',
  session_cpu integer not null not default
    is  'exp.clf.unix.cs.scb_cputime',
  session_state varchar(32) not null not default
    is 'exp.clf.unix.cs.scb_state',
  session_state_num integer4 not null not default
    is 'exp.clf.unix.cs.scb_state_num',
  session_mask varchar(32) not null not default
    is 'exp.clf.unix.cs.scb_mask',
  session_wait_reason varchar(32) not null not default
    is 'exp.clf.unix.cs.scb_memory'
)
as import from 'tables'
with dbms = IMA, structure = unique sortkeyed,
key = (server, session_id)
\p\g

/*
** IMA_CSSAMPLER_STATS - Information about the CS Monitor Sampler
*/
drop ima_cssampler_stats;
\p\g
register table ima_cssampler_stats (
	server varchar(64) not null not default 
		is 'SERVER',
	samp_index varchar(32) not null not default 
		is 'exp.clf.nt.cs.samp_index',
	numsamples integer4 not null not default 
		is 'exp.clf.nt.cs.samp_numsamples',
	numtlssamples integer4 not null not default 
		is 'exp.clf.nt.cs.samp_numtlssamples',
	numtlsslots integer4 not null not default 
		is 'exp.clf.nt.cs.samp_numtlsslots',
	numtlsprobes integer4 not null not default 
		is 'exp.clf.nt.cs.samp_numtlsprobes',
	numtlsreads integer4 not null not default 
		is 'exp.clf.nt.cs.samp_numtlsreads',
	numtlsdirty integer4 not null not default 
		is 'exp.clf.nt.cs.samp_numtlsdirty',
	numtlswrites integer4 not null not default 
		is 'exp.clf.nt.cs.samp_numtlswrites'
)
as import from 'tables'
with dbms = IMA,
structure = sortkeyed,
key = (server)
\p\g

drop ima_cssampler_threads;
\p\g
register table ima_cssampler_threads (
	server varchar(64) not null not default 
		is 'SERVER',
	numthreadsamples integer4 not null not default 
		is 'exp.clf.nt.cs.thread_numsamples',
	stateFREE integer4 not null not default 
		is 'exp.clf.nt.cs.thread_stateFREE',
	stateCOMPUTABLE integer4 not null not default 
		is 'exp.clf.nt.cs.thread_stateCOMPUTABLE',
	stateEVENT_WAIT integer4 not null not default 
		is 'exp.clf.nt.cs.thread_stateEVENT_WAIT',
	stateMUTEX integer4 not null not default 
		is 'exp.clf.nt.cs.thread_stateMUTEX',
	facilityCLF integer4 not null not default 
		is 'exp.clf.nt.cs.thread_facilityCLF',
	facilityADF integer4 not null not default 
		is 'exp.clf.nt.cs.thread_facilityADF',
	facilityDMF integer4 not null not default 
		is 'exp.clf.nt.cs.thread_facilityDMF',
	facilityOPF integer4 not null not default 
		is 'exp.clf.nt.cs.thread_facilityOPF',
	facilityPSF integer4 not null not default 
		is 'exp.clf.nt.cs.thread_facilityPSF',
	facilityQEF integer4 not null not default 
		is 'exp.clf.nt.cs.thread_facilityQEF',
	facilityQSF integer4 not null not default 
		is 'exp.clf.nt.cs.thread_facilityQSF',
	facilityRDF integer4 not null not default 
		is 'exp.clf.nt.cs.thread_facilityRDF',
	facilitySCF integer4 not null not default 
		is 'exp.clf.nt.cs.thread_facilitySCF',
	facilityULF integer4 not null not default 
		is 'exp.clf.nt.cs.thread_facilityULF',
	facilityDUF integer4 not null not default 
		is 'exp.clf.nt.cs.thread_facilityDUF',
	facilityGCF integer4 not null not default 
		is 'exp.clf.nt.cs.thread_facilityGCF',
	facilityRQF integer4 not null not default 
		is 'exp.clf.nt.cs.thread_facilityRQF',
	facilityTPF integer4 not null not default 
		is 'exp.clf.nt.cs.thread_facilityTPF',
	facilityGWF integer4 not null not default 
		is 'exp.clf.nt.cs.thread_facilityGWF',
	facilitySXF integer4 not null not default 
		is 'exp.clf.nt.cs.thread_facilitySXF',
	eventDISKIO integer4 not null not default 
		is 'exp.clf.nt.cs.thread_eventDISKIO',
	eventMESSAGEIO integer4 not null not default 
		is 'exp.clf.nt.cs.thread_eventMESSAGEIO',
	eventLOGIO integer4 not null not default 
		is 'exp.clf.nt.cs.thread_eventLOGIO',
	eventLOG integer4 not null not default 
		is 'exp.clf.nt.cs.thread_eventLOG',
	eventLOCK integer4 not null not default 
		is 'exp.clf.nt.cs.thread_eventLOCK',
	eventLOGEVENT integer4 not null not default 
		is 'exp.clf.nt.cs.thread_eventLOGEVENT',
	eventLOCKEVENT integer4 not null not default 
		is 'exp.clf.nt.cs.thread_eventLOCKEVENT'
)
as import from 'tables'
with dbms = IMA,
structure = sortkeyed,
key = (server)
\p\g

/*
** IMA_locklists - each session maintains a locklist of active locks
*/
drop ima_locklists;
\p\g
register table ima_locklists (
	vnode varchar(64) not null not default is 
		'VNODE',
	locklist_id integer4 not null not default is 
		'exp.dmf.lk.llb_id.id_id',
	locklist_logical_count integer4 not null not default is 
		'exp.dmf.lk.llb_llkb_count',
	locklist_status varchar(50) not null not default is 
		'exp.dmf.lk.llb_status',
	locklist_lock_count integer4 not null not default is 
		'exp.dmf.lk.llb_lkb_count',
	locklist_max_locks integer4 not null not default is 
		'exp.dmf.lk.llb_max_lkb',
	locklist_name0 integer4 not null not default is 
		'exp.dmf.lk.llb_name0',
	locklist_name1 integer4 not null not default is 
		'exp.dmf.lk.llb_name1',
	locklist_server_pid integer4 not null not default is 
		'exp.dmf.lk.llb_pid',
	locklist_session_id varchar(32) not null not default is 
		'exp.dmf.lk.llb_sid',
	locklist_wait_id integer4 not null not default is 
		'exp.dmf.lk.llb_wait_id_id',
  locklist_related_llb	integer4 not null not default is
    'exp.dmf.lk.llb_related_llb',
  locklist_related_llb_id_id integer4 not null not default is
    'exp.dmf.lk.llb_related_llb_id_id',
  locklist_related_count integer4 not null not default is
    'exp.dmf.lk.llb_related_count'
)
as import from 'tables'
with dbms = IMA,
structure = unique sortkeyed,
key = (vnode, locklist_id);
\p\g

/*
** IMA_LOCKS - individual locks on a locklist
*/
drop ima_locks;
\p\g
register table ima_locks (
	vnode varchar(64) not null not default is 
		'VNODE',
	lock_id integer4 not null not default is 
		'exp.dmf.lk.lkb_id.id_id',
	lock_request_mode varchar(3) not null not default is 
		'exp.dmf.lk.lkb_request_mode',
	lock_grant_mode varchar(3) not null not default is 
		'exp.dmf.lk.lkb_grant_mode',
	lock_state varchar(20) not null not default is 
		'exp.dmf.lk.lkb_state',
	lock_attributes varchar(30) not null not default is 
		'exp.dmf.lk.lkb_attribute',
	resource_id integer4 not null not default is 
		'exp.dmf.lk.lkb_rsb_id_id',
	locklist_id integer4 not null not default is 
		'exp.dmf.lk.lkb_llb_id_id'
)
as import from 'tables'
with dbms = IMA,
structure = unique sortkeyed,
key = (vnode, lock_id);
\p\g

/*
** IMA_RESOURCES - things which may be locked (db, table, page etc)
*/
drop ima_resources;
\p\g
register table ima_resources (
	vnode varchar(64) not null not default is 
		'VNODE',
	resource_id integer4 not null not default is 
		'exp.dmf.lk.rsb_id.id_id',
	resource_grant_mode varchar(3) not null not default is 
		'exp.dmf.lk.rsb_grant_mode',
  resource_convert_mode varchar(3) not null not default is
    'exp.dmf.lk.rsb_convert_mode',
	resource_key varchar(60) not null not default is 
		'exp.dmf.lk.rsb_name',
	resource_type integer4 not null not default is 
		'exp.dmf.lk.rsb_name0',
	resource_database_id integer4 not null not default is
		'exp.dmf.lk.rsb_name1',
	resource_table_id integer4 not null not default is
		'exp.dmf.lk.rsb_name2',
	resource_index_id integer4 not null not default is 
		'exp.dmf.lk.rsb_name3',
	resource_page_number integer4 not null not default is 
		'exp.dmf.lk.rsb_name4',
	resource_row_id integer4 not null not default is 
		'exp.dmf.lk.rsb_name5',
	resource_key6 integer4 not null not default is 
		'exp.dmf.lk.rsb_name6',
	resource_lock_id0 integer4 not null not default is 
		'exp.dmf.lk.rsb_value0',
	resource_lock_id1 integer4 not null not default is 
		'exp.dmf.lk.rsb_value1',
  resource_invalid  integer4 not null not default is
    'exp.dmf.lk.rsb_invalid'
)
as import from 'tables'
with dbms = IMA,
structure = unique sortkeyed,
key = (vnode, resource_id);
\p\g

/*
** IMA_LOG_PROCESSES - the INGRES processes active in the logging system
*/
drop ima_log_processes;
\p\g
register table ima_log_processes (
	vnode varchar(64) not null not default is 
		'VNODE',
  log_id_instance integer4 not null not default is
    'exp.dmf.lg.lpb_id.id_instance',
	log_process_id integer4 not null not default is 
		'exp.dmf.lg.lpb_id.id_id',
	buffer_manager_id varchar(20) not null not default is
		'exp.dmf.lg.lpb_bufmgr_id',
	condition varchar(20) not null not default is
		'exp.dmf.lg.lpb_cond',
	force_abort_session varchar(30) not null not default is
		'exp.dmf.lg.lpb_force_abort_sid',
	group_commit_asleep varchar(20) not null not default is
		'exp.dmf.lg.lpb_gcmt_asleep',
	group_commit_session varchar(30) not null not default is
		'exp.dmf.lg.lpb_gcmt_sid',
	process_count integer4 not null not default is
		'exp.dmf.lg.lpb_lpd_count',
	process_status varchar(200) not null not default is 
		'exp.dmf.lg.lpb_status',
	process_status_num integer4 not null not default is 
		'exp.dmf.lg.lpb_status_num',
	process_tx_begins integer4 not null not default is
		'exp.dmf.lg.lpb_stat.begin',
	process_tx_ends integer4 not null not default is
		'exp.dmf.lg.lpb_stat.end',
	process_log_forces integer4 not null not default is
		'exp.dmf.lg.lpb_stat.force',
	process_reads integer4 not null not default is
		'exp.dmf.lg.lpb_stat.readio',
	process_waits integer4 not null not default is
		'exp.dmf.lg.lpb_stat.wait',
	process_writes integer4 not null not default is
		'exp.dmf.lg.lpb_stat.write',
	process_pid integer4 not null not default is 
		'exp.dmf.lg.lpb_pid'
)
as import from 'tables'
with dbms = IMA,
structure = unique sortkeyed,
key = (vnode, log_process_id);
\p\g

/*
** IMA_LOG_DATABASES - databases active in the logging system
*/
drop ima_log_databases;
\p\g
register table ima_log_databases (
	vnode varchar(64) is 
		'VNODE',
	db_id integer4 not null not default is
		'exp.dmf.lg.ldb_id.id_id',
	db_status varchar(100) not null not default is 
		'exp.dmf.lg.ldb_status',
	db_database_id integer4 not null not default is 
		'exp.dmf.lg.ldb_database_id',
	db_name varchar(32) not null not default is 
		'exp.dmf.lg.ldb_db_name',
	db_owner varchar(32) not null not default is
		'exp.dmf.lg.ldb_db_owner',
	db_buffer varchar(32) not null not default is
		'exp.dmf.lg.ldb_buffer',
	db_first_la varchar(32) not null not default is
		'exp.dmf.lg.ldb_d_first_la',
	db_first_la_offset integer4 not null not default is
		'exp.dmf.lg.ldb_d_first_la.la_offset',
	db_first_la_sequence integer4 not null not default is
		'exp.dmf.lg.ldb_d_first_la.la_sequence',
	db_last_la varchar(32) not null not default is
		'exp.dmf.lg.ldb_d_last_la',
	db_last_la_offset integer4 not null not default is
		'exp.dmf.lg.ldb_d_last_la.la_offset',
	db_last_la_sequence integer4 not null not default is
		'exp.dmf.lg.ldb_d_last_la.la_sequence',
	db_first_jla varchar(32) not null not default is
		'exp.dmf.lg.ldb_j_first_la',
	db_first_jla_offset integer4 not null not default is
		'exp.dmf.lg.ldb_j_first_la.la_offset',
	db_first_jla_sequence integer4 not null not default is
		'exp.dmf.lg.ldb_j_first_la.la_sequence',
	db_last_jla varchar(32) not null not default is
		'exp.dmf.lg.ldb_j_last_la',
	db_last_jla_offset integer4 not null not default is
		'exp.dmf.lg.ldb_j_last_la.la_offset',
	db_last_jla_sequence integer4 not null not default is
		'exp.dmf.lg.ldb_j_last_la.la_sequence',
	db_l_buffer varchar(32) not null not default is
		'exp.dmf.lg.ldb_l_buffer',
	db_process_count integer4 not null not default is
		'exp.dmf.lg.ldb_lpd_count',
	db_tx_count integer4 not null not default is
		'exp.dmf.lg.ldb_lxb_count',
	db_online_ckp_tx_count integer4 not null not default is
		'exp.dmf.lg.ldb_lxbo_count',
	db_sback_lsn varchar(32) not null not default is
		'exp.dmf.lg.ldb_sback_lsn',
	db_sback_lsn_high varchar(32) not null not default is
		'exp.dmf.lg.ldb_sback_lsn_high',
	db_sback_lsn_low varchar(32) not null not default is
		'exp.dmf.lg.ldb_sback_lsn_low',
	db_ldb_sbackup varchar(32) not null not default is
		'exp.dmf.lg.ldb_sbackup',
	db_ldb_sbackup_offset integer4 not null not default is
		'exp.dmf.lg.ldb_sbackup.la_offset',
	db_ldb_sbackup_sequence integer4 not null not default is
		'exp.dmf.lg.ldb_sbackup.la_sequence',
	db_tx_begins integer4 not null not default is
		'exp.dmf.lg.ldb_stat.begin',
	db_tx_ends integer4 not null not default is
		'exp.dmf.lg.ldb_stat.end',
	db_forces integer4 not null not default is
		'exp.dmf.lg.ldb_stat.force',
	db_reads integer4 not null not default is
		'exp.dmf.lg.ldb_stat.read',
	db_waits integer4 not null not default is
		'exp.dmf.lg.ldb_stat.wait',
	db_writes integer4 not null not default is
		'exp.dmf.lg.ldb_stat.write',
	db_status_num integer4 not null not default is
		'exp.dmf.lg.ldb_status_num'
)
as import from 'tables'
with dbms = IMA,
structure = unique sortkeyed,
key = (vnode, db_id);
\p\g

/* 
** IMA_LOG_TRANSACTIONS - transactions in the logging system
*/
drop ima_log_transactions;
\p\g
register table ima_log_transactions (
	vnode varchar(64) not null not default is 
		'VNODE',
	tx_id_id integer4 not null not default is 
		'exp.dmf.lg.lxb_id.id_id',
	tx_id_instance integer4 not null not default is 
		'exp.dmf.lg.lxb_id.id_instance',
	tx_status varchar(200) not null not default is 
		'exp.dmf.lg.lxb_status',
	tx_db_id_id integer4 not null not default is 
		'exp.dmf.lg.lxb_db_id_id',
	tx_db_name varchar(32) not null not default is 
		'exp.dmf.lg.lxb_db_name',
	tx_db_owner varchar(32) not null not default is 
		'exp.dmf.lg.lxb_db_owner',
	tx_pr_id_id integer4 not null not default is 
		'exp.dmf.lg.lxb_pr_id_id',
	tx_wait_reason varchar(16) not null not default is 
		'exp.dmf.lg.lxb_wait_reason',
	tx_first_log_address varchar (32) not null not default is 
		'exp.dmf.lg.lxb_first_lga',
	tx_last_log_address varchar (32) not null not default is 
		'exp.dmf.lg.lxb_last_lga',
	tx_cp_log_address varchar (32) not null not default is 
		'exp.dmf.lg.lxb_cp_lga',
	tx_transaction_id varchar(32) not null not default is 
		'exp.dmf.lg.lxb_tran_id',
	tx_transaction_high integer4 not null not default is 
		'exp.dmf.lg.lxb_tran_id.db_high_tran',
	tx_transaction_low integer4 not null not default is 
		'exp.dmf.lg.lxb_tran_id.db_low_tran',
	tx_server_pid integer4 not null not default is 
		'exp.dmf.lg.lxb_pid',
	tx_session_id varchar(32) not null not default is 
		'exp.dmf.lg.lxb_sid',
	tx_user_name varchar(32) not null not default is 
		'exp.dmf.lg.lxb_user_name',
	tx_state_split integer4 not null not default is 
		'exp.dmf.lg.lxb_stat.split',
	tx_state_write integer4 not null not default is 
		'exp.dmf.lg.lxb_stat.write',
	tx_state_force integer4 not null not default is 
		'exp.dmf.lg.lxb_stat.force',
	tx_state_wait integer4 not null not default is 
		'exp.dmf.lg.lxb_stat.wait'
)
as import from 'tables'
with dbms = IMA,
structure = unique sortkeyed,
key = (vnode, tx_id_id);
\p\g

/*
** IMA_GCA_CONNECTIONS - information about active GCA connections
*/
drop ima_gca_connections;
\p\g
register table ima_gca_connections (
	server varchar(64) not null not default
		is 'SERVER',
	assoc_id integer4 not null not default 
		is 'exp.gcf.gca.assoc',
	assoc_flags varchar(30) not null not default 
		is 'exp.gcf.gca.assoc.flags',
	partner_protocol integer4 not null not default 
		is 'exp.gcf.gca.assoc.partner_protocol',
	session_protocol integer4 not null not default
		is 'exp.gcf.gca.assoc.session_protocol',
	assoc_userid varchar(32) not null not default
		is 'exp.gcf.gca.assoc.userid'
)
as import from 'tables'
with dbms = IMA,
structure = unique sortkeyed,
key = (server, assoc_id)
\p\g

/*
** IMA_DI_SLAVE_INFO - information about active slave processes
*/
drop ima_di_slave_info;
\p\g
register table ima_di_slave_info (
	server varchar(64) not null not default
		is 'SERVER',
	di_slaveno integer4 not null not default
		is 'exp.clf.di.di_slaveno',
	di_dio integer4 not null not default
		is 'exp.clf.di.dimo_dio',
	cpu_tm_msecs integer4 not null not default 
		is 'exp.clf.di.dimo_cpu.tm_msecs',
	cpu_tm_secs integer4 not null not default
		is 'exp.clf.di.dimo_cpu.tm_secs',
	di_idrss integer4 not null not default
		is 'exp.clf.di.dimo_idrss',
	di_maxrss integer4 not null not default
		is 'exp.clf.di.dimo_maxrss',
	di_majflt integer4 not null not default
		is 'exp.clf.di.dimo_majflt',
	di_minflt integer4 not null not default
		is 'exp.clf.di.dimo_minflt',
	di_msgrcv integer4 not null not default
		is 'exp.clf.di.dimo_msgrcv',
	di_msgsnd integer4 not null not default
		is 'exp.clf.di.dimo_msgsnd',
	di_msgtotal integer4 not null not default
		is 'exp.clf.di.dimo_msgtotal',
	di_nivcsw integer4 not null not default
		is 'exp.clf.di.dimo_nivcsw',
	di_nsignals integer4 not null not default
		is 'exp.clf.di.dimo_nsignals',
	di_nvcsw integer4 not null not default
		is 'exp.clf.di.dimo_nvcsw',
	di_nswap integer4 not null not default
		is 'exp.clf.di.dimo_nswap',
	di_reads integer4 not null not default 
		is 'exp.clf.di.dimo_reads',
	di_writes integer4 not null not default
		is 'exp.clf.di.dimo_writes',
	di_stime_tm_msecs integer4 not null not default
		is 'exp.clf.di.dimo_stime.tm_msecs',
	di_stime_tm_secs integer4 not null not default
		is 'exp.clf.di.dimo_cpu.tm_secs',
	di_utime_tm_msecs integer4 not null not default
		is 'exp.clf.di.dimo_utime.tm_msecs',
	di_utime_tm_secs integer4 not null not default
		is 'exp.clf.di.dimo_utime.tm_secs'
)
as import from 'tables'
with dbms = IMA,
structure = unique sortkeyed,
key = (server, di_slaveno)
\p\g

/*
** IMA_DMF_BUFFER_MANAGER_STATS - information about DMF buffer manager
*/
drop ima_dmf_buffer_manager_stats;
\p\g

register table ima_dmf_buffer_manager_stats (
    server varchar(64) not null not default
        is 'SERVER',
    cache_wait_count integer4 not null not default
        is 'exp.dmf.dm0p.lbmc_bmcwait',
    fc_flush_count integer4 not null not default
        is 'exp.dmf.dm0p.lbmc_fcflush',
    cache_lock_reclaims integer4 not null not default
        is 'exp.dmf.dm0p.lbmc_lockreclaim'
)
as import from 'tables'
with dbms = IMA,
structure = sortkeyed,
key = (server)
\p\g

/*
** IMA_DMF_BM_CONFIGURATION - information about buffer manager configuration
*/
drop ima_dmf_bm_configuration;
\p\g

register table ima_dmf_bm_configuration (
    server varchar(64) not null not default
        is 'SERVER',
    db_cache_entry_count integer4 not null not default
        is 'exp.dmf.dm0p.bmc_dbcsize',
    table_cache_entry_count integer4 not null not default
        is 'exp.dmf.dm0p.bmc_tblcsize',
    buffer_manager_srv_count integer4 not null not default
        is 'exp.dmf.dm0p.bmc_srv_count',
    current_cp_id integer4 not null not default
        is 'exp.dmf.dm0p.bmc_cpcount',
    current_cp_buffer_id integer4 not null not default
        is 'exp.dmf.dm0p.bmc_cpindex',
    fc_done_cp_count integer4 not null not default
        is 'exp.dmf.dm0p.bmc_cpcheck',
    cache_lockreclaim integer4 not null not default
        is 'exp.dmf.dm0p.bmc_lockreclaim',
    fast_commit_flush_count integer4 not null not default
        is 'exp.dmf.dm0p.bmc_fcflush',
    db_tbl_cache_wait_count integer4 not null not default
        is 'exp.dmf.dm0p.bmc_bmcwait',
    buffer_manager_status integer4 not null not default
        is 'exp.dmf.dm0p.bmc_status'
)
as import from 'tables'
with dbms = IMA,
structure = sortkeyed,
key = (server)
\p\g

/*
** IMA_DMF_CACHE_STATS - information about the DMF cache
*/
drop ima_dmf_cache_stats;
\p\g
register table ima_dmf_cache_stats (
	server varchar(64) not null not default 
		is 'SERVER',
	page_size integer4 not null not default
	    is 'exp.dmf.dm0p.bm_pgsize',
	/* cumulative stats */
	force_count integer4 not null not default 
		is 'exp.dmf.dm0p.bm_force',
	io_wait_count integer4 not null not default 
		is 'exp.dmf.dm0p.bm_iowait',

	group_buffer_read_count integer4 not null not default 
		is 'exp.dmf.dm0p.bm_greads',
	group_buffer_write_count integer4 not null not default 
		is 'exp.dmf.dm0p.bm_gwrites',

	fix_count integer4 not null not default 
		is 'exp.dmf.dm0p.bm_fix',
	unfix_count integer4 not null not default 
		is 'exp.dmf.dm0p.bm_unfix',
	read_count integer4 not null not default 
		is 'exp.dmf.dm0p.bm_reads',
	write_count integer4 not null not default 
		is 'exp.dmf.dm0p.bm_writes',
	hit_count integer4 not null not default 
		is 'exp.dmf.dm0p.bm_hit',
	dirty_unfix_count integer4 not null not default 
		is 'exp.dmf.dm0p.bm_dirty',

	pages_still_valid integer4 not null not default 
		is 'exp.dmf.dm0p.bm_check',
	pages_invalid integer4 not null not default 
		is 'exp.dmf.dm0p.bm_refresh',

	/* various cache info */
	buffer_count integer4 not null not default
		is 'exp.dmf.dm0p.bm_bufcnt',
	page_buffer_count integer4 not null not default 
		is 'exp.dmf.dm0p.bm_sbufcnt',
	flimit integer4 not null not default 
		is 'exp.dmf.dm0p.bm_flimit',
	mlimit integer4 not null not default 
		is 'exp.dmf.dm0p.bm_mlimit',
	wbstart integer4 not null not default 
		is 'exp.dmf.dm0p.bm_wbstart',
	wbend integer4 not null not default 
		is 'exp.dmf.dm0p.bm_wbend',
	hash_bucket_count integer4 not null not default 
		is 'exp.dmf.dm0p.bm_hshcnt',

	/* group buffer info */

	group_buffer_count integer4 not null not default 
		is 'exp.dmf.dm0p.bm_gcnt',
	group_buffer_size integer4 not null not default 
		is 'exp.dmf.dm0p.bm_gpages',

	/* current stats */
	cache_status integer4 not null not default 
		is 'exp.dmf.dm0p.bm_status',
		/*
		** BM_GROUP            0x0001
		** BM_FWAIT            0x0002
		** BM_WBWAIT           0x0004
		** BM_FCFLUSH          0x0008
		** BM_SHARED_BUFMGR    0x0010
		** BM_PASS_ABORT       0x0020
		** BM_PREPASS_ABORT    0x0040
		*/
	free_buffer_count integer4 not null not default 
		is 'exp.dmf.dm0p.bm_fcount',
	free_buffer_waiters integer4 not null not default 
		is 'exp.dmf.dm0p.bm_fwait',
	fixed_buffer_count integer4 not null not default 
		is 'exp.dmf.dm0p.bm_lcount',
	modified_buffer_count integer4 not null not default 
		is 'exp.dmf.dm0p.bm_mcount',

	free_group_buffer_count integer4 not null not default 
		is 'exp.dmf.dm0p.bm_gfcount',
	fixed_group_buffer_count integer4 not null not default 
		is 'exp.dmf.dm0p.bm_glcount',
	modified_group_buffer_count integer4 not null not default 
		is 'exp.dmf.dm0p.bm_gmcount'
)
as import from 'tables'
with dbms = IMA,
structure = sortkeyed,
key = (server)
\p\g

/*
** IMA_QSF_CACHE_STATS - information about the QSF cache
*/
drop ima_qsf_cache_stats;
\p\g
register table ima_qsf_cache_stats (
        server varchar(64) not null not default
                is 'SERVER',
        qsf_bkts_used integer4 not null not default
                is 'exp.qsf.qsr.qsr_bkts_used',
        qsf_bmaxobjs integer4 not null not default
                is 'exp.qsf.qsr.qsr_bmaxobjs',
        qsf_decay_factor integer4       not null not default
                is 'exp.qsf.qsr.qsr_decay_factor',
        qsf_memleft integer4 not null not default
                is 'exp.qsf.qsr.qsr_memleft',
        qsf_memtot integer4 not null not default
                is 'exp.qsf.qsr.qsr_memtot',
        qsf_mx_index integer4 not null not default
                is 'exp.qsf.qsr.qsr_mx_index',
        qsf_mx_named integer4 not null not default
                is 'exp.qsf.qsr.qsr_mx_named',
        qsf_mx_rsize integer4 not null not default
                is 'exp.qsf.qsr.qsr_mx_rsize',
        qsf_mx_size integer4 not null not default
                is 'exp.qsf.qsr.qsr_mx_size',
        qsf_mx_unnamed integer4 not null not default
                is 'exp.qsf.qsr.qsr_mx_unnamed',
        qsf_mxbkts_used integer4 not null not default
                is 'exp.qsf.qsr.qsr_mxbkts_used',
        qsf_mxobjs integer4 not null not default
                is 'exp.qsf.qsr.qsr_mxobjs',
        qsf_mxsess integer4 not null not default
                is 'exp.qsf.qsr.qsr_mxsess',
        qsf_named_requests integer4 not null not default
                is 'exp.qsf.qsr.qsr_named_requests',
        qsf_nbuckets integer4 not null not default
                is 'exp.qsf.qsr.qsr_nbuckets',
        qsf_no_destroyed integer4 not null not default
                is 'exp.qsf.qsr.qsr_no_destroyed',
        qsf_no_index integer4 not null not default
                is 'exp.qsf.qsr.qsr_no_index',
        qsf_no_named integer4 not null not default
                is 'exp.qsf.qsr.qsr_no_named',
        qsf_no_unnamed integer4 not null not default
                is 'exp.qsf.qsr.qsr_no_unnamed',
        qsf_nobjs integer4 not null not default
                is 'exp.qsf.qsr.qsr_nobjs',
        qsf_nsess integer4 not null not default
                is 'exp.qsf.qsr.qsr_nsess'
)
as import from 'tables'
with dbms = IMA,
structure = sortkeyed,
key = (server);
\p\g

/*
** IMA_QSF_DBP - information about database procedures in QSF
*/
drop ima_qsf_dbp;
\p\g
register table ima_qsf_dbp (
	server varchar(64) not null not default 
		is 'SERVER',
	dbp_index integer4 not null not default 
		is 'exp.qsf.qso.dbp.index',
	dbp_name varchar(60) not null not default 
		is 'exp.qsf.qso.dbp.name',
	dbp_owner varchar(24) not null not default 
		is 'exp.qsf.qso.dbp.owner',
	dbp_size integer4 not null not default 
		is 'exp.qsf.qso.dbp.size',
	dbp_dbid integer4 not null not default 
		is 'exp.qsf.qso.dbp.udbid',
	dbp_usage_count integer4 not null not default 
		is 'exp.qsf.qso.dbp.usage'
) as import from 'tables'
with dbms = ima,
structure = unique sortkeyed,
key = (server, dbp_index);
\p\g


/*
** IMA_QSF_RQP - information about repeat queries in QSF
*/
drop ima_qsf_rqp;
\p\g
register table ima_qsf_rqp (
	server varchar(64) not null not default 
		is 'SERVER',
	rqp_index integer4 not null not default 
		is 'exp.qsf.qso.rqp.index',
	rqp_name varchar(60) not null not default 
		is 'exp.qsf.qso.rqp.name',
	rqp_text varchar(1000) not null not default
		is 'exp.qsf.qso.rqp.text',
	rqp_size integer4 not null not default 
		is 'exp.qsf.qso.rqp.size',
	rqp_dbid integer4 not null not default 
		is 'exp.qsf.qso.rqp.udbid',
	rqp_usage_count integer4 not null not default 
		is 'exp.qsf.qso.rqp.usage'
) as import from 'tables'
with dbms = ima,
structure = unique sortkeyed,
key = (server, rqp_index);
\p\g

/*
** IMA_MEMORY_INFO - information about memory allocation in the server
*/
drop ima_memory_info;
\p\g
register table ima_memory_info (
	server varchar(64) not null not default 
		is 'SERVER',
	bytes_used integer4 not null not default 
		is 'exp.clf.unix.me.num.bytes_used',
	free_pages integer4 not null not default 
		is 'exp.clf.unix.me.num.free_pages',
	get_pages integer4 not null not default 
		is 'exp.clf.unix.me.num.get_pages',
	pages_used integer4 not null not default 
		is 'exp.clf.unix.me.num.pages_used'
) as import from 'tables'
with dbms = ima,
structure = sortkeyed,
key = (server);
\p\g

/*
** IMA_RDF_CACHE_INFO - information about the state of the RDF cache
*/
drop ima_rdf_cache_info;
\p\g
register table ima_rdf_cache_info (
	server varchar(64) not null not default
		is 'SERVER',
	state_num integer4 not null not default 
		is 'exp.rdf.rdi.state_num',
	state_string varchar(60) not null not default 
		is 'exp.rdf.rdi.state_str',
	cache_size integer4 not null not default 
		is 'exp.rdf.rdi.cache.size',
	max_tables integer4 not null not default
		is 'exp.rdf.rdi.cache.max_tables',
	max_qtrees integer4 not null not default
		is 'exp.rdf.rdi.cache.max_qtree_objs',
	max_ldb_descs integer4 not null not default
		is 'exp.rdf.rdi.cache.max_ldb_descs',
	max_defaults integer4 not null not default
		is 'exp.rdf.rdi.cache.max_defaults'
) as import from 'tables'
with dbms = ima,
structure = sortkeyed,
key = (server);
\p\g

drop view ima_lk_summary_view ;
\p\g

drop table ima_lkmo_lkd;
\p\g

register table ima_lkmo_lkd (

    vnode	    varchar(64) not null not default is 'VNODE',

    lkd_status			    char(100) not null not default
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


drop table ima_lkmo_lkd_stat;
\p\g

register table ima_lkmo_lkd_stat (

    vnode	    varchar(64) not null not default is 'VNODE',
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
    lkd_stat_cnt_gdlck_call	    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.cnt_gdlck_call',
    lkd_stat_cnt_gdlck_sent	    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.cnt_gdlck_sent',
    lkd_stat_sent_all		    i4 not null not default
			    is 'exp.dmf.lk.lkd_stat.sent_all'
)
as import from 'tables'
with dbms = IMA,
     structure = sortkeyed,
     key = (vnode);
\p\g



create view ima_lk_summary_view as select
	lkd.vnode,
	lkd_rsh_size,		      /* size of resource has table */
	lkd_lkh_size,		      /* size of lock hash table */

	lkd_max_lkb,		      /* locks per transaction */

	lkd_stat_create_list, /* lock lists created */
	lkd_stat_release_all,	/* lock lists released */

	lkd_llb_inuse,	    	/* lock lists in use */
	((lkd_lbk_size + 1) - lkd_llb_inuse) as lock_lists_remaining, /* lock lists remaining */
	(lkd_lbk_size + 1) as lock_lists_available,               		/* total lock lists available */
	lkd_lkb_inuse,		    /* locks in use */
	((lkd_sbk_size + 1) - lkd_lkb_inuse) as locks_remaining,    	/* locks remaining */
	(lkd_sbk_size + 1) as locks_available,                				/* total locks available */

	lkd_rsb_inuse,		    /* Resource Blocks in use */

	lkd_stat_request_new,	/* locks requested */
	lkd_stat_request_cvt,	/* locks re-requested */
	lkd_stat_convert,	    /* locks converted */
	lkd_stat_release,	    /* locks released  */
	lkd_stat_cancel,	    /* locks cancelled */
	lkd_stat_rlse_partial,/* locks escalated */

	lkd_stat_dlock_srch,	/* Deadlock Search */
	lkd_stat_cvt_deadlock,/* Convert Deadlock */
	lkd_stat_deadlock,	  /* deadlock */

	lkd_stat_convert_wait,/* convert wait */
	lkd_stat_wait		      /* lock wait */


from 	ima_lkmo_lkd lkd, ima_lkmo_lkd_stat stat
where
	lkd.vnode = stat.vnode;
\p\g


drop table ima_lgmo_lgd;
\p\g
register table ima_lgmo_lgd (

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

    /* classid not available in Ingres 2.0 
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

drop table ima_lgmo_lfb;
\p\g
register table ima_lgmo_lfb (

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
    lfb_hdr_lgh_begin_blk   i4 not null not default
                                is 'exp.dmf.lg.lfb_hdr_lgh_begin_blk',
    lfb_hdr_lgh_begin_off   i4 not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_begin_off',
    lfb_hdr_lgh_end	    char (30) not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_end',
    lfb_hdr_lgh_end_seq	    i4 not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_end_seq',
    lfb_hdr_lgh_end_blk	    i4 not null not default
                                is 'exp.dmf.lg.lfb_hdr_lgh_end_blk',
    lfb_hdr_lgh_end_off	    i4 not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_end_off',
    lfb_hdr_lgh_cp	    char (30) not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_cp',
    lfb_hdr_lgh_cp_seq	    i4 not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_cp_seq',
    lfb_hdr_lgh_cp_blk      i4 not null not default
                                is 'exp.dmf.lg.lfb_hdr_lgh_cp_blk',
    lfb_hdr_lgh_cp_off	    i4 not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_cp_off',
    lfb_hdr_lgh_last_lsn    char(30) not null not default
                                is 'exp.dmf.lg.lfb_hdr_lgh_last_lsn',
    lfb_hdr_lgh_last_lsn_lsn_high i4 not null not default
                                is 'exp.dmf.lg.lfb_hdr_lgh_last_lsn_lsn_high',
    lfb_hdr_lgh_last_lsn_lsn_low i4 not null not default
                                is 'exp.dmf.lg.lfb_hdr_lgh_last_lsn_lsn_low',
    lfb_hdr_lgh_active_logs i4 not null not default
				is 'exp.dmf.lg.lfb_hdr_lgh_active_logs',
    lfb_hdr_lgh_percentage_logfull i4 not null not default
                                is 'exp.dmf.lg.lfb_hdr_lgh_percentage_logfull',
    lfb_forced_lga	    char(30) not null not default
				is 'exp.dmf.lg.lfb_forced_lga',
    lfb_forced_high	    i4 not null not default
				is 'exp.dmf.lg.lfb_forced_lga.la_sequence',
    lfb_forced_block        i4 not null not default
                                is 'exp.dmf.lg.lfb_forced_lga.la_block',
    lfb_forced_low	    i4 not null not default
				is 'exp.dmf.lg.lfb_forced_lga.la_offset',
    lfb_forced_lsn	    char (20) not null not default
				is 'exp.dmf.lg.lfb_forced_lsn',
    lfb_forced_lsn_high	    i4 not null not default
				is 'exp.dmf.lg.lfb_forced_lsn_high',
    lfb_forced_lsn_low	    i4 not null not default
				is 'exp.dmf.lg.lfb_forced_lsn_low',
    lfb_archive_start_lga   char(30) not null not default
				is 'exp.dmf.lg.lfb_archive_start',
    lfb_archive_start_high  i4 not null not default
				is 'exp.dmf.lg.lfb_archive_start.la_sequence',
    lfb_archive_start_block i4 not null not default
				is 'exp.dmf.lg.lfb_archive_start.la_block',
    lfb_archive_start_low   i4 not null not default
				is 'exp.dmf.lg.lfb_archive_start.la_offset',
    lfb_archive_end_lga	    char(30) not null not default
				is 'exp.dmf.lg.lfb_archive_end',
    lfb_archive_end_high    i4 not null not default
				is 'exp.dmf.lg.lfb_archive_end.la_sequence',
    lfb_archive_end_block   i4 not null not default
				is 'exp.dmf.lg.lfb_archive_end.la_block',
    lfb_archive_end_low	i4 not null not default
				is 'exp.dmf.lg.lfb_archive_end.la_offset',
    lfb_archive_prevcp_lga  char(30) not null not default
				is 'exp.dmf.lg.lfb_archive_prevcp',
    lfb_archive_prevcp_high i4 not null not default
				is 'exp.dmf.lg.lfb_archive_prevcp.la_sequence',
    lfb_archive_prevcp_block i4 not null not default
				is 'exp.dmf.lg.lfb_archive_prevcp.la_block',
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

/*
** IMA_BLOCK_INFO 
*/
drop table ima_block_info;
\p\g

register table ima_block_info (
	server varchar(64) not null not default is
		'SERVER',
  blk_num_sessions integer4 not null not default is
    'exp.clf.unix.cs.srv_block.num_sessions',
  blk_max_sessions integer4 not null not default is
    'exp.clf.unix.cs.srv_block.max_sessions',
  blk_num_active integer4 not null not default is
    'exp.clf.unix.cs.srv_block.num_active',
  blk_max_active integer4 not null not default is
    'exp.clf.unix.cs.srv_block.max_active'
)
as import from 'tables'
with dbms = IMA, structure = sortkeyed,
key = ( server )
\p\g
	
/*
** IMA instance information.
*/
drop ima_version;
\p\g
register table ima_version(
	name_server varchar(64) not null not default is
		'SERVER',
    version varchar(30) not null not default is
        'exp.clf.gv.version',
    platform varchar(30) not null not default is
        'exp.clf.gv.env',
    majorvers integer not null not default is
        'exp.clf.gv.majorvers',
    minorvers integer not null not default is
        'exp.clf.gv.minorvers',
    genlevel integer not null not default is
        'exp.clf.gv.genlevel',
    bldlevel integer not null not default is
        'exp.clf.gv.bldlevel',
    bytetype integer not null not default is
        'exp.clf.gv.bytetype',
    hardware integer not null not default is
        'exp.clf.gv.hw',
    os integer not null not default is
        'exp.clf.gv.os',
    patchlvl integer not null not default is
        'exp.clf.gv.patchlvl',
    tcp_port varchar(10) not null not default is
        'exp.clf.gv.tcpport',
    instance varchar(10) not null not default is
        'exp.clf.gv.instance',
    sql92conf varchar(10) not null not default is
        'exp.clf.gv.sql92_conf',
    language varchar(20) not null not default is
        'exp.clf.gv.language',
    charset varchar(20) not null not default is
        'exp.clf.gv.charset',
    system varchar(255) not null not default is
        'exp.clf.gv.system'
)
as import from 'tables'
with dbms = IMA,
structure = sortkeyed,
key = (name_server);
\p\g
grant all on ima_version to public with grant option;
\p\g

drop ima_instance_cnf;
\p\g
register table ima_instance_cnf(
	name_server varchar(64) not null not default is
		'SERVER',
    cnf varchar(20) not null not default is
        'exp.clf.gv.cnf_index',
    name varchar(60) not null not default is
        'exp.clf.gv.cnf_name',
    value varchar(60) not null not default is
        'exp.clf.gv.cnf_value'

)
as import from 'tables'
with dbms = IMA,
structure = sortkeyed,
key = (name_server);
\p\g
grant all on ima_instance_cnf to public with grant option;
\p\g


drop ima_config;
\p\g
register table ima_config(
	name_server varchar(64) not null not default is
		'SERVER',
    cnf varchar(20) not null not default is    
        'exp.clf.gv.cnfdat_index',
    name varchar(127) not null not default is    
        'exp.clf.gv.cnfdat_name',
    value varchar(255) not null not default is   
        'exp.clf.gv.cnfdat_value'

)
as import from 'tables'
with dbms = IMA,
structure = sortkeyed,
key = (name_server);
\p\g
grant all on ima_config to public with grant option;
\p\g

/*
** DATABASE PROCEDURES:
**
** ima_set_vnode_domain() - set the IMA domain to the current vnode
** ima_set_server_domain() - reset the IMA domain to the current server only
** ima_expand_domain() - expand the IMA domain to include the argument
** ima_contract_domain() - contract the IMA domain to exclude the argument
**
** ima_collect_slave_stats() - collect slave performance statistics
*/

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

drop procedure ima_set_server_domain;
\p\g
create procedure ima_set_server_domain as
begin
	update ima_mib_objects set value = ' '
	where classid = 'exp.gwf.gwm.session.control.reset_domain'
	and instance = '0'
	and server = dbmsinfo('ima_server');
end;
\p\g

/* Soft shutdown this procedure wait until the current sessions end, */
/* before the servers stops */
drop  procedure ima_shut_server
\p\g
create procedure ima_shut_server(server_id varchar(32) not null) as
begin

	update ima_mib_objects set value = :server_id
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

/* Turn off listen, disble new user connections */
drop  procedure ima_close_server
\p\g
create procedure ima_close_server(server_id varchar(32) not null) as
begin

	update ima_mib_objects set value = '0'
	where classid = 'exp.scf.scd.server.control.close'
	and instance = '0'
	and server = :server_id;

	if iirowcount = 0 then
		message 'No such server - it must have exited'
	endif;
	commit;
	return 0;
end;
\p\g

/* Turn on listen, re-enble new user connections */
drop  procedure ima_open_server
\p\g
create procedure ima_open_server(server_id varchar(32) not null) as
begin

	update ima_mib_objects set value = :server_id
	where classid = 'exp.scf.scd.server.control.open'
	and instance = '0'
	and server = :server_id;

	if iirowcount = 0 then
		message 'No such server - it must have exited'
	endif;
	commit;
	return 0;
end;
\p\g

/* Remove session from the server */
drop  procedure ima_remove_session
\p\g
create procedure ima_remove_session(session_id varchar(32) not null,
                                    server_id varchar(32) not null) as
declare
session_ref varchar(32) not null;
begin

        select session_ptr into :session_ref from ima_server_sessions
        where session_id = :session_id;
        update ima_mib_objects set value = :session_ref
        where classid = 'exp.scf.scs.scb_remove_session'
            and instance = :session_ref
            and server = :server_id;

        if iirowcount = 0 then
                message 'No such session - it must have exited'
        endif;
        return 0;
end;
\p\g

drop  procedure ima_drop_sessions
\p\g
commit;
\p\g
create procedure ima_drop_sessions as
declare
    instance varchar(32) not null not default;
    rowcount integer not null not default;
begin

    rowcount = 0;
    /*
    ** Retrieve all the active sessions from all servers and
    ** only remove sessions associated with databases
    ** Exclude connections to imadb.  If our session gets killed
    ** the query is aborted and all other sessions remain
    */
    for select session_id into :instance from ima_server_sessions
        where session_name not like ' <%' and server=dbmsinfo('ima_server') and 
            session_id!=dbmsinfo('ima_session') do
        update ima_mib_objects set value=:instance
            where classid='exp.scf.scs.scb_remove_session' and
            server=dbmsinfo('ima_server') and
            instance = :instance;
            select iirowcount + :rowcount into :rowcount;
    endfor;
    
    /*
    ** Now end this imadb session
    */
    for select session_id into :instance from ima_server_sessions
        where server=dbmsinfo('ima_server') and session_id = dbmsinfo('ima_session') do 
        update ima_mib_objects set value=:instance
            where classid='exp.scf.scs.scb_remove_session' and
            server=dbmsinfo('ima_server') and
            instance = :instance;
            select iirowcount + :rowcount into :rowcount;
    endfor;

	if :rowcount = 0 then
		message 'No available sessions'
	endif;
	commit;
	return 0;
end;
\p\g

drop  procedure ima_drop_all_sessions
\p\g
commit;
\p\g
create procedure ima_drop_all_sessions as
declare
    instance varchar(32) not null not default;
    servid   varchar(64) not null not default;
    rowcount integer not null not default;
begin
    /*
    ** Extend the domain to all servers in the installation
    */
	update ima_mib_objects set value =dbmsinfo('ima_vnode')
	where classid = 'exp.gwf.gwm.session.control.add_vnode'
	and instance = '0'
	and server = dbmsinfo('ima_server');

    rowcount = 0;
    /*
    ** Retrieve all the active sessions from all servers and
    ** only remove sessions associated with databases
    ** Exclude connections to imadb.  If our session gets killed
    ** the query is aborted and all other sessions remain
    */
    for select server, session_id into :servid, :instance from ima_server_sessions
        where session_name not like ' <%' and session_id != dbmsinfo('ima_session') do
        update ima_mib_objects set value=:instance
            where classid='exp.scf.scs.scb_remove_session' and
            server = :servid and
            instance = :instance;
            select iirowcount + :rowcount into :rowcount;
    endfor;
    
    /*
    ** Now end all imadb sessions
    */
    for select session_id into :instance from ima_server_sessions
        where session_name not like ' <%' and server=dbmsinfo('ima_server') do 
        update ima_mib_objects set value=:instance
            where classid='exp.scf.scs.scb_remove_session' and
            server=dbmsinfo('ima_server') and
            instance = :instance;
            select iirowcount + :rowcount into :rowcount;
    endfor;

	if :rowcount = 0 then
		message 'No available sessions'
	endif;
	commit;
	return 0;
end;
\p\g

/* Turn on Sampler thread for server */
drop  procedure ima_start_sampler
\p\g
create procedure ima_start_sampler(server_id varchar(32) not null) as
begin

	update ima_mib_objects set value = '0'
	where classid = 'exp.scf.scd.server.control.start_sampler'
	and instance = '0'
	and server = :server_id;

	commit;
	return 0;
end;
\p\g

/* Turn off Sampler thread for server */
drop  procedure ima_stop_sampler
\p\g
create procedure ima_stop_sampler(server_id varchar(32) not null) as
begin

	update ima_mib_objects set value = '0'
	where classid = 'exp.scf.scd.server.control.stop_sampler'
	and instance = '0'
	and server = :server_id;

	commit;
	return 0;
end;
\p\g

grant select,update on ima_mib_objects to public with grant option;
\p\g
grant select on ima_mo_meta to public with grant option;
\p\g
grant select on ima_installation_info to public with grant option;
\p\g
grant select on ima_gcn_info to public with grant option;
\p\g
grant select on ima_gcn_registrations to public with grant option;
\p\g
grant select on ima_gcc_info to public with grant option;
\p\g
grant select on ima_gcb_info to public with grant option;
\p\g
grant select on ima_dbms_server_parameters to public with grant option;
\p\g
grant select on ima_dbms_server_status to public with grant option;
\p\g
grant select on ima_server_sessions to public with grant option;
\p\g
grant select on ima_server_sessions_desc to public with grant option;
\p\g
grant select on ima_server_sessions_extra to public with grant option;
\p\g
grant select on ima_locklists to public with grant option;
\p\g
grant select on ima_locks to public with grant option;
\p\g
grant select on ima_resources to public with grant option;
\p\g
grant select on ima_log_processes to public with grant option;
\p\g
grant select on ima_log_databases to public with grant option;
\p\g
grant select on ima_log_transactions to public with grant option;
\p\g
grant select on ima_gca_connections to public with grant option;
\p\g
grant select on ima_di_slave_info to public with grant option;
\p\g
grant select on ima_dmf_buffer_manager_stats to public with grant option;
\p\g
grant select on ima_dmf_bm_configuration to public with grant option;
\p\g
grant select on ima_dmf_cache_stats to public with grant option;
\p\g
grant select on ima_qsf_cache_stats to public with grant option;
\p\g
grant select on ima_qsf_dbp to public with grant option;
\p\g
grant select on ima_qsf_rqp to public with grant option;
\p\g
grant select on ima_memory_info to public with grant option;
\p\g
grant select on ima_rdf_cache_info to public with grant option;
\p\g
grant select on ima_lk_summary_view to public with grant option;
\p\g
grant select on  ima_lkmo_lkd to public with grant option;
\p\g
grant select on  ima_lkmo_lkd_stat to public with grant option;
\p\g
grant select on ima_lgmo_lgd to public with grant option;
\p\g
grant select on ima_lgmo_lfb to public with grant option;
\p\g
grant select on ima_cssampler_stats to public with grant option;
\p\g
grant select on ima_cssampler_threads to public with grant option;
\p\g
grant select on ima_block_info to public with grant option;
\p\g
grant select on ima_gcc_info to public with grant option;
\p\g
grant execute on procedure ima_set_vnode_domain to public with grant option;
\p\g
grant execute on procedure ima_set_server_domain to public with grant option;
\p\g
grant execute on procedure ima_shut_server to public with grant option;
\p\g
grant execute on procedure ima_open_server to public with grant option;
\p\g
grant execute on procedure ima_close_server to public with grant option;
\p\g
grant execute on procedure ima_remove_session to public with grant option;
\p\g
grant execute on procedure ima_drop_sessions to public with grant option;
\p\g
grant execute on procedure ima_drop_all_sessions to public with grant option;
\p\g
grant execute on procedure ima_start_sampler to public with grant option;
\p\g
grant execute on procedure ima_stop_sampler to public with grant option;
\p\g

/*
** IMA objects for icesvr
*/
drop table ice_locations;\p\g
register table ice_locations (
    server varchar(64) not null not default is
        'SERVER',
    name varchar(32) not null not default is
        'exp.wsf.wcs.location.index',
    id varchar(32) not null not default is
        'exp.wsf.wcs.location.id',
    path varchar(32) not null not default is
        'exp.wsf.wcs.location.path',
    type varchar(32) not null not default is
        'exp.wsf.wcs.location.type',
    extension varchar(32) not null not default is
        'exp.wsf.wcs.location.extension'
)
as import from 'tables'
with dbms = IMA,
structure = sortkeyed,
key = (server);
\p\g
grant select on ice_locations to public with grant option;\p\g

drop table ice_cache;\p\g
register table ice_cache (
    server varchar(64) not null not default is
        'SERVER',
    cache_index varchar(32) not null not default is
        'exp.wsf.wps.cache.index',
    name varchar(32) not null not default is
        'exp.wsf.wps.cache.name',
    loc_name varchar(32) not null not default is
        'exp.wsf.wps.cache.loc_name',
    status varchar(32) not null not default is
        'exp.wsf.wps.cache.status',
    counter integer not null not default is
        'exp.wsf.wps.cache.file_counter',
    exist integer not null not default is
        'exp.wsf.wps.cache.exist',
    owner varchar(32) not null not default is
        'exp.wsf.wps.cache.owner',
    timeout integer not null not default is
        'exp.wsf.wps.cache.timeout',
    in_use integer not null not default is
        'exp.wsf.wps.cache.in_use',
    req_count integer not null not default is
        'exp.wsf.wps.cache.req_count'
)
as import from 'tables'
with dbms = IMA,
structure = sortkeyed,
key = (server);
\p\g
grant select on ice_cache to public with grant option;\p\g

drop table ice_active_users;\p\g
register table ice_active_users (
    server varchar(64) not null not default is
        'SERVER',
    name varchar(32) not null not default is
        'exp.wsf.act.index',
    host varchar(32) not null not default is
        'exp.wsf.act.host',
    query varchar(64) not null not default is
        'exp.wsf.act.query',
    err_count integer not null not default is
        'exp.wsf.act.err_count'
)
as import from 'tables'
with dbms = IMA,
structure = sortkeyed,
key = (server);
\p\g
grant select on ice_active_users to public with grant option;\p\g

drop table ice_users;\p\g
register table ice_users (
    server varchar(64) not null not default is
        'SERVER',
    name   varchar(32) not null not default is
        'exp.wsf.usr.index',
    ice_user   varchar(32) not null not default is
        'exp.wsf.usr.ice_user',
    req_count integer not null not default is
        'exp.wsf.usr.requested_count',
    timeout integer not null not default is
        'exp.wsf.usr.timeout'
)
as import from 'tables'
with dbms = IMA,
structure = sortkeyed,
key = (server);
\p\g
grant select on ice_users to public with grant option;\p\g

drop table ice_user_transactions;\p\g
register table ice_user_transactions (
    server varchar(64) not null not default is
        'SERVER',
    trans_key varchar(32) not null not default is
        'exp.wsf.usr.trans.index',
    name varchar(32) not null not default is
        'exp.wsf.usr.trans.name',
    trn_session varchar(32) not null not default is
        'exp.wsf.usr.trans.connect',
    owner varchar(32) not null not default is
        'exp.wsf.usr.trans.owner'
)
as import from 'tables'
with dbms = IMA,
structure = sortkeyed,
key = (server);
\p\g
grant select on ice_user_transactions to public with grant option;\p\g

drop table ice_user_cursors;\p\g
register table ice_user_cursors (
    server varchar(64) not null not default is
        'SERVER',
    curs_key varchar(32) not null not default is
        'exp.wsf.usr.cursor.index',
    name varchar(32) not null not default is
        'exp.wsf.usr.cursor.name',
    query varchar(64) not null not default is
        'exp.wsf.usr.cursor.query',
    owner varchar(32) not null not default is
        'exp.wsf.usr.cursor.owner'
)
as import from 'tables'
with dbms = IMA,
structure = sortkeyed,
key = (server);
\p\g
grant select on ice_user_cursors to public with grant option;\p\g

drop table ice_db_connections;\p\g
register table ice_db_connections (
    server varchar(64) not null not default is
        'SERVER',
    db_key varchar(32) not null not default is
        'exp.ddf.ddc.db_access_index',
    driver integer not null not default is
        'exp.ddf.ddc.db_driver',
    name varchar(64) not null not default is
        'exp.ddf.ddc.db_name',
    use_count integer not null not default is
        'exp.ddf.ddc.db_used',
    timeout integer not null not default is
        'exp.ddf.ddc.timtout'
)
as import from 'tables'
with dbms = IMA,
structure = sortkeyed,
key = (server);
\p\g
grant select on ice_db_connections to public with grant option;\p\g
