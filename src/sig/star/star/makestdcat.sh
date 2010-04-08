:
#
# Copyright (c) 2004 Ingres Corporation
#
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#
sql $1/star +U -u'$ingres' << END_INPUT
/*
iiregistered_objects

COL NAME	    DATA TYPE	DESCRIPTION
-----------------   ---------	----------------------------------------------
ddb_object_name	    char(32)	name of star registered object (the registered
				object will be a table, view or index)
ddb_object_owner    char(32)	owner of star registered object
register_date	    char(25)	date object was registered
ldb_database	    char(32)	name of LDB database that registered object
				resides in
ldb_node	    char(32)	node that ldb_database resides on
ldb_dbmstype	    char(32)	type of database ldb_database is ('INGRES',
				'RMS', 'DB2','RDB'... etc).  This is the same 
				types used by iinamu.
ldb_object_name	    char(32)	name the LDB uses for the registered object
ldb_object_owner    char(32)	owner of the registered object in the LDB
ldb_object_type	    char(8)	Type of local object:  "T" for table, "V"
				for view, "I" for index.
*/
create view iiregistered_objects (
	ddb_object_name,
	ddb_object_owner,
	register_date,
	ldb_database,
	ldb_node,
	ldb_dbmstype,
	ldb_object_name,
	ldb_object_owner,
	ldb_object_type)
as select  
	object_name,
	object_owner,
	create_date,
	ldb_database,
	ldb_node,
	ldb_dbms,
	table_name,
	table_owner,
	local_type
from iiddb_objects, iiddb_ldbids, iiddb_tableinfo where
	iiddb_objects.object_base=iiddb_tableinfo.object_base and
	iiddb_objects.object_index=iiddb_tableinfo.object_index and
	iiddb_tableinfo.ldb_id=iiddb_ldbids.ldb_id and
	(system_object='N' or system_object=' ') and
	object_type = 'L'
\p\g
\q
END_INPUT
