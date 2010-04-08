/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: icinit92.sql
**
** Description:
**      Create the ice repository objects an properties.
##
## Note:
##      Any changes made to the iceinit.sql file should be reflected here.
##
## History:
##      18-Apr-2001 (fanra01)
##          Bug 104506
##          Created for case differences in SQL 92.
##      10-Jul-2002 (fanra01)
##          Bug 108224
##          Add select permissions to everyone on the tables.
##
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

insert into RepDescObject values (0, 0, 'ICE_DBUSERS');
insert into RepDescObject values (0, 1, 'ICE_USERS');
insert into RepDescObject values (0, 2, 'ICE_ROLES');
insert into RepDescObject values (0, 3, 'ICE_USER_ROLE');
insert into RepDescObject values (0, 4, 'ICE_RES_URIGHTS');
insert into RepDescObject values (0, 5, 'ICE_RES_RRIGHTS');
insert into RepDescObject values (0, 6, 'ICE_DB');
insert into RepDescObject values (0, 7, 'ICE_USER_DB');
\p\g

insert into RepDescProp values (0, 0, 1, 'ID',          1, 'd',   4, 0, 0);
insert into RepDescProp values (0, 0, 2, 'ALIAS',       0, 's',  32, 0, 1);
insert into RepDescProp values (0, 0, 3, 'NAME',        0, 's',  32, 0, 0);
insert into RepDescProp values (0, 0, 4, 'PASSWORD',    0, 's',  32, 0, 0);
insert into RepDescProp values (0, 0, 5, 'COMMENT',     0, 's', 100, 1, 0);

insert into RepDescProp values (0, 1, 1, 'ID',          1, 'd',   4, 0, 0);
insert into RepDescProp values (0, 1, 2, 'NAME',        0, 's',  32, 0, 1);
insert into RepDescProp values (0, 1, 3, 'PASSWORD',    0, 's',  32, 0, 0);
insert into RepDescProp values (0, 1, 4, 'DBUSER',      0, 'd',   4, 0, 0);
insert into RepDescProp values (0, 1, 5, 'FLAG',        0, 'd',   4, 0, 0);
insert into RepDescProp values (0, 1, 6, 'TIMEOUT',     0, 'd',   4, 0, 0);
insert into RepDescProp values (0, 1, 7, 'COMMENT',     0, 's', 100, 1, 0);
insert into RepDescProp values (0, 1, 8, 'AUTHTYPE',    0, 'd',   4, 0, 0);

insert into RepDescProp values (0, 2, 1, 'ID',          1, 'd',   4, 0, 0);
insert into RepDescProp values (0, 2, 2, 'NAME',        0, 's',  32, 0, 1);
insert into RepDescProp values (0, 2, 3, 'COMMENT',     0, 's', 100, 1, 0);

insert into RepDescProp values (0, 3, 1, 'USER_ID',     1, 'd',   4, 0, 0);
insert into RepDescProp values (0, 3, 2, 'ROLE_ID',     1, 'd',   4, 0, 0);

insert into RepDescProp values (0, 4, 1, 'USER_ID',     1, 'd',   4, 0, 0);
insert into RepDescProp values (0, 4, 2, 'RESOURCE',    1, 's',  32, 0, 0);
insert into RepDescProp values (0, 4, 3, 'RIGHT',       0, 'd',   4, 0, 0);
insert into RepDescProp values (0, 4, 4, 'REFS',        0, 'd',   4, 0, 0);

insert into RepDescProp values (0, 5, 1, 'ROLE_ID',     1, 'd',   4, 0, 0);
insert into RepDescProp values (0, 5, 2, 'RESOURCE',    1, 's',  32, 0, 0);
insert into RepDescProp values (0, 5, 3, 'RIGHT',       0, 'd',   4, 0, 0);

insert into RepDescProp values (0, 6, 1, 'ID',          1, 'd',   4, 0, 0);
insert into RepDescProp values (0, 6, 2, 'NAME',        0, 's',  32, 0, 1);
insert into RepDescProp values (0, 6, 3, 'DBNAME',      0, 's',  32, 0, 0);
insert into RepDescProp values (0, 6, 4, 'DBUSER',      0, 'd',   4, 0, 0);
insert into RepDescProp values (0, 6, 5, 'FLAG',        0, 'd',   4, 0, 0);
insert into RepDescProp values (0, 6, 6, 'COMMENT',     0, 's', 100, 1, 0);

insert into RepDescProp values (0, 7, 1, 'USER_ID',     1, 'd',   4, 0, 0);
insert into RepDescProp values (0, 7, 2, 'DB_ID',       1, 'd',   4, 0, 0);

/* Example */
insert into RepDescExecute values (0, 0, 'delete from ice_res_urights where resource = %s');
insert into RepDescExecute values (0, 1, 'delete from ice_res_rrights where resource = %s');
insert into RepDescExecute values (0, 2, 'delete from ice_res_urights where user_id = %d');
insert into RepDescExecute values (0, 3, 'delete from ice_res_rrights where role_id = %d');
insert into RepDescExecute values (0, 4, 'delete from ice_user_role where user_id = %d');
insert into RepDescExecute values (0, 5, 'delete from ice_user_db where user_id = %d');

/* WSF module number 1 */
insert into RepDescObject values (1, 0, 'ICE_LOCATIONS');
insert into RepDescObject values (1, 1, 'ICE_UNITS');
insert into RepDescObject values (1, 2, 'ICE_DOCUMENTS');
insert into RepDescObject values (1, 3, 'ICE_UNITS_LOCS');
insert into RepDescObject values (1, 4, 'ICE_PROFILES');
insert into RepDescObject values (1, 5, 'ICE_PROFILES_ROLES');
insert into RepDescObject values (1, 6, 'ICE_PROFILES_DBS');
insert into RepDescObject values (1, 7, 'ICE_SERVER_VARS');
insert into RepDescObject values (1, 8, 'ICE_FILES');
insert into RepDescObject values (1, 9, 'ICE_APPLICATIONS');

insert into RepDescProp values (1, 0, 1, 'ID',          1, 'd',   4, 0, 0);
insert into RepDescProp values (1, 0, 2, 'NAME',        0, 's', 128, 0, 1);
insert into RepDescProp values (1, 0, 3, 'TYPE',        0, 'd',   4, 0, 0);
insert into RepDescProp values (1, 0, 4, 'PATH',        0, 's', 256, 0, 0);
insert into RepDescProp values (1, 0, 5, 'EXTENSIONS',  0, 's', 256, 0, 0);

insert into RepDescProp values (1, 1, 1, 'ID',          1, 'd',   4, 0, 0);
insert into RepDescProp values (1, 1, 2, 'NAME',        0, 's',  32, 0, 1);
insert into RepDescProp values (1, 1, 3, 'OWNER',       0, 'd',   4, 0, 0);

insert into RepDescProp values (1, 2, 1, 'ID',          1, 'd',   4, 0, 0);
insert into RepDescProp values (1, 2, 2, 'UNIT',        0, 'd',   4, 0, 0);
insert into RepDescProp values (1, 2, 3, 'NAME',        0, 's', 128, 0, 0);
insert into RepDescProp values (1, 2, 4, 'SUFFIX',      0, 's',  32, 0, 0);
insert into RepDescProp values (1, 2, 5, 'FLAG',        0, 'd',   4, 0, 0);
insert into RepDescProp values (1, 2, 6, 'OWNER',       0, 'd',   4, 0, 0);
insert into RepDescProp values (1, 2, 7, 'EXT_LOC',     0, 'd',   4, 1, 0);
insert into RepDescProp values (1, 2, 8, 'EXT_FILE',    0, 's', 128, 1, 0);
insert into RepDescProp values (1, 2, 9, 'EXT_SUFFIX',  0, 's',  32, 1, 0);

insert into RepDescProp values (1, 3, 1, 'UNIT',        1, 'd',   4, 0, 0);
insert into RepDescProp values (1, 3, 2, 'LOC',         1, 'd',   4, 0, 0);

insert into RepDescProp values (1, 4, 1, 'ID',          1, 'd',   4, 0, 0);
insert into RepDescProp values (1, 4, 2, 'NAME',        0, 's',  32, 0, 1);
insert into RepDescProp values (1, 4, 3, 'DBUSER',      0, 'd',   4, 0, 0);
insert into RepDescProp values (1, 4, 4, 'FLAGS',       0, 'd',   4, 0, 0);
insert into RepDescProp values (1, 4, 5, 'TIMEOUT',     0, 'd',   4, 0, 0);

insert into RepDescProp values (1, 5, 1, 'PROFILE',     1, 'd',   4, 0, 0);
insert into RepDescProp values (1, 5, 2, 'ROLE',        1, 'd',   4, 0, 0);

insert into RepDescProp values (1, 6, 1, 'PROFILE',     1, 'd',   4, 0, 0);
insert into RepDescProp values (1, 6, 2, 'DB',          1, 'd',   4, 0, 0);

insert into RepDescProp values (1, 7, 1, 'NAME',        1, 's', 128, 0, 0);
insert into RepDescProp values (1, 7, 2, 'VALUE',       0, 's', 256, 0, 0);

insert into RepDescProp values (1, 8, 1, 'ID',          1, 'd',   4, 0, 0);
insert into RepDescProp values (1, 8, 2, 'DOC',         0, 'l',   0, 1, 0);

insert into RepDescProp values (1, 9, 1, 'ID',          1, 'd',   4, 0, 0);
insert into RepDescProp values (1, 9, 2, 'NAME',        0, 's',  32, 0, 1);

insert into RepDescExecute values (1, 0, 'delete from ice_profiles_roles where profile = %d');
insert into RepDescExecute values (1, 1, 'delete from ice_profiles_dbs where profile = %d');
insert into RepDescExecute values (1, 2, 'delete from ice_units_locs where unit = %d');

/* create a specific descriptor for quick access with multiple connections */
insert into RepDescObject values (2, 0, 'ICE_FILES');
insert into RepDescProp values (2, 0, 1, 'ID',          1, 'd',   4, 0, 0);
insert into RepDescProp values (2, 0, 2, 'DOC',         0, 'l',   0, 1, 0);
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
	right integer not null
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
alter table ice_locations add  UNIQUE("NAME")\p\g
alter table ice_units add  UNIQUE("NAME")\p\g
alter table ice_profiles add  UNIQUE("NAME")\p\g
alter table ice_applications add  UNIQUE("NAME")\p\g
alter table ice_dbusers add  UNIQUE("ALIAS")\p\g
alter table ice_users add  UNIQUE("NAME")\p\g
alter table ice_roles add  UNIQUE("NAME")\p\g
alter table ice_db add  UNIQUE("NAME")\p\g

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
