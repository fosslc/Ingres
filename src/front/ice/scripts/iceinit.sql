/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: iceinit.sql
**
** Description:
**      Create the ice repository objects an properties.
##
## History:
##      05-Nov-98 (fanra01)
##          Add history.
##      19-Nov-1998 (fanra01)
##          Add the creation of the ice repository tables.
##      23-Nov-1998 (fanra01)
##          Add unique constraint to repository properties.
##      07-Jan-1999 (fanra01)
##          Add authtype to ice_users.  This field indicates what method to
##          use for password authentication. 0 means repository password.
##          1 means Ingres GCusrpwd authentication.
##      05-May-2000 (fanra01)
##          Bug 101346
##          Add reference count for user rights objects as unit access is
##          granted whilst individual files are readable.
##      16-Mar-2001 (fanra01)
##          Bug 104245
##          Increase field length for name values of locations in ice_locations
##          and, filenames and extensions in ice_documents.
##      10-Jul-2002 (fanra01)
##          Bug 108224
##          Add select permissions to everyone on the tables.
*/
drop table RepDescObject;\p\g
create table RepDescObject (
    module_id Integer not null,
    id Integer not null,
    name varchar(32) not null
);\p\g

drop table RepDescProp;\p\g
create table RepDescProp (
    module_id Integer not null,
    object Integer not null,
    id Integer not null,
    name varchar(32) not null,
    isKey Integer not null,     /* 0 = NO, 1 = YES */
    type char not null,         /* d = Integer, s = char, l = long binary */
    length Integer not null,
    isnull Integer not null,
    isunique Integer not null
);\p\g

drop table RepDescExecute;\p\g
create table RepDescExecute (
    module_id Integer not null,
    id Integer not null,
    statement varchar(1000) not null
);\p\g

insert into RepDescObject values (0, 0, 'ice_dbusers');
insert into RepDescObject values (0, 1, 'ice_users');
insert into RepDescObject values (0, 2, 'ice_roles');
insert into RepDescObject values (0, 3, 'ice_user_role');
insert into RepDescObject values (0, 4, 'ice_res_urights');
insert into RepDescObject values (0, 5, 'ice_res_rrights');
insert into RepDescObject values (0, 6, 'ice_db');
insert into RepDescObject values (0, 7, 'ice_user_db');
\p\g

insert into RepDescProp values (0, 0, 1, 'id',          1, 'd',   4, 0, 0);
insert into RepDescProp values (0, 0, 2, 'alias',       0, 's',  32, 0, 1);
insert into RepDescProp values (0, 0, 3, 'name',        0, 's',  32, 0, 0);
insert into RepDescProp values (0, 0, 4, 'password',    0, 's',  32, 0, 0);
insert into RepDescProp values (0, 0, 5, 'comment',     0, 's', 100, 1, 0);

insert into RepDescProp values (0, 1, 1, 'id',          1, 'd',   4, 0, 0);
insert into RepDescProp values (0, 1, 2, 'name',        0, 's',  32, 0, 1);
insert into RepDescProp values (0, 1, 3, 'password',    0, 's',  32, 0, 0);
insert into RepDescProp values (0, 1, 4, 'dbuser',      0, 'd',   4, 0, 0);
insert into RepDescProp values (0, 1, 5, 'flag',        0, 'd',   4, 0, 0);
insert into RepDescProp values (0, 1, 6, 'timeout',     0, 'd',   4, 0, 0);
insert into RepDescProp values (0, 1, 7, 'comment',     0, 's', 100, 1, 0);
insert into RepDescProp values (0, 1, 8, 'authtype',    0, 'd',   4, 0, 0);

insert into RepDescProp values (0, 2, 1, 'id',          1, 'd',   4, 0, 0);
insert into RepDescProp values (0, 2, 2, 'name',        0, 's',  32, 0, 1);
insert into RepDescProp values (0, 2, 3, 'comment',     0, 's', 100, 1, 0);

insert into RepDescProp values (0, 3, 1, 'user_id',     1, 'd',   4, 0, 0);
insert into RepDescProp values (0, 3, 2, 'role_id',     1, 'd',   4, 0, 0);

insert into RepDescProp values (0, 4, 1, 'user_id',     1, 'd',   4, 0, 0);
insert into RepDescProp values (0, 4, 2, 'resource',    1, 's',  32, 0, 0);
insert into RepDescProp values (0, 4, 3, 'right',       0, 'd',   4, 0, 0);
insert into RepDescProp values (0, 4, 4, 'refs',        0, 'd',   4, 0, 0);

insert into RepDescProp values (0, 5, 1, 'role_id',     1, 'd',   4, 0, 0);
insert into RepDescProp values (0, 5, 2, 'resource',    1, 's',  32, 0, 0);
insert into RepDescProp values (0, 5, 3, 'right',       0, 'd',   4, 0, 0);

insert into RepDescProp values (0, 6, 1, 'id',          1, 'd',   4, 0, 0);
insert into RepDescProp values (0, 6, 2, 'name',        0, 's',  32, 0, 1);
insert into RepDescProp values (0, 6, 3, 'dbname',      0, 's',  32, 0, 0);
insert into RepDescProp values (0, 6, 4, 'dbuser',      0, 'd',   4, 0, 0);
insert into RepDescProp values (0, 6, 5, 'flag',        0, 'd',   4, 0, 0);
insert into RepDescProp values (0, 6, 6, 'comment',     0, 's', 100, 1, 0);

insert into RepDescProp values (0, 7, 1, 'user_id',     1, 'd',   4, 0, 0);
insert into RepDescProp values (0, 7, 2, 'db_id',       1, 'd',   4, 0, 0);

/* Example */
insert into RepDescExecute values (0, 0, 'delete from ice_res_urights where resource = %s');
insert into RepDescExecute values (0, 1, 'delete from ice_res_rrights where resource = %s');
insert into RepDescExecute values (0, 2, 'delete from ice_res_urights where user_id = %d');
insert into RepDescExecute values (0, 3, 'delete from ice_res_rrights where role_id = %d');
insert into RepDescExecute values (0, 4, 'delete from ice_user_role where user_id = %d');
insert into RepDescExecute values (0, 5, 'delete from ice_user_db where user_id = %d');

/* WSF module number 1 */
insert into RepDescObject values (1, 0, 'ice_locations');
insert into RepDescObject values (1, 1, 'ice_units');
insert into RepDescObject values (1, 2, 'ice_documents');
insert into RepDescObject values (1, 3, 'ice_units_locs');
insert into RepDescObject values (1, 4, 'ice_profiles');
insert into RepDescObject values (1, 5, 'ice_profiles_roles');
insert into RepDescObject values (1, 6, 'ice_profiles_dbs');
insert into RepDescObject values (1, 7, 'ice_server_vars');
insert into RepDescObject values (1, 8, 'ice_files');
insert into RepDescObject values (1, 9, 'ice_applications');

insert into RepDescProp values (1, 0, 1, 'id',          1, 'd',   4, 0, 0);
insert into RepDescProp values (1, 0, 2, 'name',        0, 's', 128, 0, 1);
insert into RepDescProp values (1, 0, 3, 'type',        0, 'd',   4, 0, 0);
insert into RepDescProp values (1, 0, 4, 'path',        0, 's', 256, 0, 0);
insert into RepDescProp values (1, 0, 5, 'extensions',  0, 's', 256, 0, 0);

insert into RepDescProp values (1, 1, 1, 'id',          1, 'd',   4, 0, 0);
insert into RepDescProp values (1, 1, 2, 'name',        0, 's',  32, 0, 1);
insert into RepDescProp values (1, 1, 3, 'owner',       0, 'd',   4, 0, 0);

insert into RepDescProp values (1, 2, 1, 'id',          1, 'd',   4, 0, 0);
insert into RepDescProp values (1, 2, 2, 'unit',        0, 'd',   4, 0, 0);
insert into RepDescProp values (1, 2, 3, 'name',        0, 's', 128, 0, 0);
insert into RepDescProp values (1, 2, 4, 'suffix',      0, 's',  32, 0, 0);
insert into RepDescProp values (1, 2, 5, 'flag',        0, 'd',   4, 0, 0);
insert into RepDescProp values (1, 2, 6, 'owner',       0, 'd',   4, 0, 0);
insert into RepDescProp values (1, 2, 7, 'ext_loc',     0, 'd',   4, 1, 0);
insert into RepDescProp values (1, 2, 8, 'ext_file',    0, 's', 128, 1, 0);
insert into RepDescProp values (1, 2, 9, 'ext_suffix',  0, 's',  32, 1, 0);

insert into RepDescProp values (1, 3, 1, 'unit',        1, 'd',   4, 0, 0);
insert into RepDescProp values (1, 3, 2, 'loc',         1, 'd',   4, 0, 0);

insert into RepDescProp values (1, 4, 1, 'id',          1, 'd',   4, 0, 0);
insert into RepDescProp values (1, 4, 2, 'name',        0, 's',  32, 0, 1);
insert into RepDescProp values (1, 4, 3, 'dbuser',      0, 'd',   4, 0, 0);
insert into RepDescProp values (1, 4, 4, 'flags',       0, 'd',   4, 0, 0);
insert into RepDescProp values (1, 4, 5, 'timeout',     0, 'd',   4, 0, 0);

insert into RepDescProp values (1, 5, 1, 'profile',     1, 'd',   4, 0, 0);
insert into RepDescProp values (1, 5, 2, 'role',        1, 'd',   4, 0, 0);

insert into RepDescProp values (1, 6, 1, 'profile',     1, 'd',   4, 0, 0);
insert into RepDescProp values (1, 6, 2, 'db',          1, 'd',   4, 0, 0);

insert into RepDescProp values (1, 7, 1, 'name',        1, 's', 128, 0, 0);
insert into RepDescProp values (1, 7, 2, 'value',       0, 's', 256, 0, 0);

insert into RepDescProp values (1, 8, 1, 'id',          1, 'd',   4, 0, 0);
insert into RepDescProp values (1, 8, 2, 'doc',         0, 'l',   0, 1, 0);

insert into RepDescProp values (1, 9, 1, 'id',          1, 'd',   4, 0, 0);
insert into RepDescProp values (1, 9, 2, 'name',        0, 's',  32, 0, 1);

insert into RepDescExecute values (1, 0, 'delete from ice_profiles_roles where profile = %d');
insert into RepDescExecute values (1, 1, 'delete from ice_profiles_dbs where profile = %d');
insert into RepDescExecute values (1, 2, 'delete from ice_units_locs where unit = %d');

/* create a specific descriptor for quick access with multiple connections */
insert into RepDescObject values (2, 0, 'ice_files');
insert into RepDescProp values (2, 0, 1, 'id',          1, 'd',   4, 0, 0);
insert into RepDescProp values (2, 0, 2, 'doc',         0, 'l',   0, 1, 0);
\p\g

create table ice_profiles_dbs(
	profile integer not null,
	db integer not null
)
with duplicates,
location = (ii_database),
security_audit=(table,norow)
;
modify ice_profiles_dbs to btree unique on
	profile,
	db
with nonleaffill = 80,
	leaffill = 70,
	fillfactor = 80,
	extend = 16,
	allocation = 4
\p\g

create table ice_profiles(
	id integer not null,
	name varchar(32) not null,
	dbuser integer not null,
	flags integer not null,
	timeout integer not null
)
with duplicates,
location = (ii_database),
security_audit=(table,norow)
;
modify ice_profiles to btree unique on
	id
with nonleaffill = 80,
	leaffill = 70,
	fillfactor = 80,
	extend = 16,
	allocation = 4
\p\g

create table ice_users(
	id integer not null,
	name varchar(32) not null,
	password varchar(32) not null,
	dbuser integer not null,
	flag integer not null,
	timeout integer not null,
	comment varchar(100),
        authtype integer not null
)
with duplicates,
location = (ii_database),
security_audit=(table,norow)
;
modify ice_users to btree unique on
	id
with nonleaffill = 80,
	leaffill = 70,
	fillfactor = 80,
	extend = 16,
	allocation = 4
\p\g

create table ice_locations(
	id integer not null,
	name varchar(128) not null,
	type integer not null,
	path varchar(256) not null,
	extensions varchar(256) not null
)
with duplicates,
location = (ii_database),
security_audit=(table,norow)
;
modify ice_locations to btree unique on
	id
with nonleaffill = 80,
	leaffill = 70,
	fillfactor = 80,
	extend = 16,
	allocation = 4
\p\g

create table ice_server_vars(
	name varchar(128) not null,
	value varchar(256) not null
)
with duplicates,
location = (ii_database),
security_audit=(table,norow)
;
modify ice_server_vars to btree unique on
	name
with nonleaffill = 80,
	leaffill = 70,
	fillfactor = 80,
	extend = 16,
	allocation = 4
\p\g

create table ice_units(
	id integer not null,
	name varchar(32) not null,
	owner integer not null
)
with duplicates,
location = (ii_database),
security_audit=(table,norow)
;
modify ice_units to btree unique on
	id
with nonleaffill = 80,
	leaffill = 70,
	fillfactor = 80,
	extend = 16,
	allocation = 4
\p\g

create table ice_user_db(
	user_id integer not null,
	db_id integer not null
)
with duplicates,
location = (ii_database),
security_audit=(table,norow)
;
modify ice_user_db to btree unique on
	user_id,
	db_id
with nonleaffill = 80,
	leaffill = 70,
	fillfactor = 80,
	extend = 16,
	allocation = 4
\p\g

create table ice_units_locs(
	unit integer not null,
	loc integer not null
)
with duplicates,
location = (ii_database),
security_audit=(table,norow)
;
modify ice_units_locs to btree unique on
	unit,
	loc
with nonleaffill = 80,
	leaffill = 70,
	fillfactor = 80,
	extend = 16,
	allocation = 4
\p\g

create table ice_user_role(
	user_id integer not null,
	role_id integer not null
)
with duplicates,
location = (ii_database),
security_audit=(table,norow)
;
modify ice_user_role to btree unique on
	user_id,
	role_id
with nonleaffill = 80,
	leaffill = 70,
	fillfactor = 80,
	extend = 16,
	allocation = 4
\p\g

create table ice_profiles_roles(
	profile integer not null,
	role integer not null
)
with duplicates,
location = (ii_database),
security_audit=(table,norow)
;
modify ice_profiles_roles to btree unique on
	profile,
	role
with nonleaffill = 80,
	leaffill = 70,
	fillfactor = 80,
	extend = 16,
	allocation = 4
\p\g

create table ice_res_rrights(
	role_id integer not null,
	resource varchar(32) not null,
	"right" integer not null
)
with duplicates,
location = (ii_database),
security_audit=(table,norow)
;
modify ice_res_rrights to btree unique on
	role_id,
	resource
with nonleaffill = 80,
	leaffill = 70,
	fillfactor = 80,
	extend = 16,
	allocation = 4
\p\g

create table ice_applications(
	id integer not null,
	name varchar(32) not null
)
with duplicates,
location = (ii_database),
security_audit=(table,norow)
;
modify ice_applications to btree unique on
	id
with nonleaffill = 80,
	leaffill = 70,
	fillfactor = 80,
	extend = 16,
	allocation = 4
\p\g

create table ice_db(
	id integer not null,
	name varchar(32) not null,
	dbname varchar(32) not null,
	dbuser integer not null,
	flag integer not null,
	comment varchar(100)
)
with duplicates,
location = (ii_database),
security_audit=(table,norow)
;
modify ice_db to btree unique on
	id
with nonleaffill = 80,
	leaffill = 70,
	fillfactor = 80,
	extend = 16,
	allocation = 4
\p\g

create table ice_files(
	id integer not null,
	doc long byte
)
with duplicates,
location = (ii_database),
security_audit=(table,norow)
;
modify ice_files to btree unique on
	id
with nonleaffill = 80,
	leaffill = 70,
	fillfactor = 80,
	extend = 16,
	allocation = 4
\p\g

create table ice_res_urights(
	user_id integer not null,
	resource varchar(32) not null,
	right integer not null,
        refs integer not null
)
with duplicates,
location = (ii_database),
security_audit=(table,norow)
;
modify ice_res_urights to btree unique on
	user_id,
	resource
with nonleaffill = 80,
	leaffill = 70,
	fillfactor = 80,
	extend = 16,
	allocation = 4
\p\g

create table ice_roles(
	id integer not null,
	name varchar(32) not null,
	comment varchar(100)
)
with duplicates,
location = (ii_database),
security_audit=(table,norow)
;
modify ice_roles to btree unique on
	id
with nonleaffill = 80,
	leaffill = 70,
	fillfactor = 80,
	extend = 16,
	allocation = 4
\p\g

create table ice_documents(
	id integer not null,
	unit integer not null,
	name varchar(128) not null,
	suffix varchar(32) not null,
	flag integer not null,
	owner integer not null,
	ext_loc integer,
	ext_file varchar(128),
	ext_suffix varchar(32)
)
with duplicates,
location = (ii_database),
security_audit=(table,norow)
;
modify ice_documents to btree unique on
	id
with nonleaffill = 80,
	leaffill = 70,
	fillfactor = 80,
	extend = 16,
	allocation = 4
\p\g

create table ice_dbusers(
	id integer not null,
	alias varchar(32) not null,
	name varchar(32) not null,
	password varchar(32) not null,
	comment varchar(100)
)
with duplicates,
location = (ii_database),
security_audit=(table,norow)
;
modify ice_dbusers to btree unique on
	id
with nonleaffill = 80,
	leaffill = 70,
	fillfactor = 80,
	extend = 16,
	allocation = 4
\p\g
commit;\p\g

/* CONSTRAINTS */
set session authorization ingres\p\g
alter table ice_locations add  UNIQUE("name")\p\g
alter table ice_units add  UNIQUE("name")\p\g
alter table ice_profiles add  UNIQUE("name")\p\g
alter table ice_applications add  UNIQUE("name")\p\g
alter table ice_dbusers add  UNIQUE("alias")\p\g
alter table ice_users add  UNIQUE("name")\p\g
alter table ice_roles add  UNIQUE("name")\p\g
alter table ice_db add  UNIQUE("name")\p\g

grant select on  table ice_profiles_dbs to public;\p\g
grant select on  table ice_profiles to public;\p\g
grant select on  table ice_users to public;\p\g
grant select on  table ice_locations to public;\p\g
grant select on  table ice_server_vars to public;\p\g
grant select on  table ice_units to public;\p\g
grant select on  table ice_user_db to public;\p\g
grant select on  table ice_units_locs to public;\p\g
grant select on  table ice_user_role to public;\p\g
grant select on  table ice_profiles_roles to public;\p\g
grant select on  table ice_res_rrights to public;\p\g
grant select on  table ice_applications to public;\p\g
grant select on  table ice_db to public;\p\g
grant select on  table ice_files to public;\p\g
grant select on  table ice_res_urights to public;\p\g
grant select on  table ice_roles to public;\p\g
grant select on  table ice_documents to public;\p\g
grant select on  table ice_dbusers to public;\p\g
