/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** REGISTER TABLE statements which register the current SXF MO data structures
** for retrieval through the IMA MO gateway
**
** History:
**	9-feb-1994 (robf)
**	   Created
*/
remove sxfmo;

register table sxfmo (
vnode varchar(16) not null not default
	is 'VNODE',
server varchar(16) not null not default 
	is 'SERVER',
audit_audit_wakeup i4 not null not default
	is 'exp.sxf.audit.audit_wakeup',
audit_close_count i4 not null not default
	is 'exp.sxf.audit.close_count',
audit_create_count i4 not null not default
	is 'exp.sxf.audit.create_count',
audit_extend_count i4 not null not default
	is 'exp.sxf.audit.extend_count',
audit_flush_count i4 not null not default
	is 'exp.sxf.audit.flush_count',
audit_flush_empty i4 not null not default
	is 'exp.sxf.audit.flush_empty',
audit_flush_predone i4 not null not default
	is 'exp.sxf.audit.flush_predone',
audit_logswitch_count i4 not null not default
	is 'exp.sxf.audit.logswitch_count',
audit_open_read i4 not null not default
	is 'exp.sxf.audit.open_read',
audit_open_write i4 not null not default
	is 'exp.sxf.audit.open_write',
audit_read_buffered i4 not null not default
	is 'exp.sxf.audit.read_buffered',
audit_read_direct i4 not null not default
	is 'exp.sxf.audit.read_direct',
audit_write_buffered i4 not null not default
	is 'exp.sxf.audit.write_buffered',
audit_write_direct i4 not null not default
	is 'exp.sxf.audit.write_direct',
audit_write_full i4 not null not default
	is 'exp.sxf.audit.write_full',
db_build i4 not null not default
	is 'exp.sxf.db_build',
db_purge i4 not null not default
	is 'exp.sxf.db_purge',
db_purge_all i4 not null not default
	is 'exp.sxf.db_purge_all'
)
as import from 'tables'
with dbms=ima,
structure= sortkeyed,
key=(vnode);
