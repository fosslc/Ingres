/* NOTE: module_number = "_UR_CLASS" 233 * 0x10000 = 15269888 
 		plus the unique Frontier server ID */

delete from RepDescObject where module_id = 15269889;
delete from RepDescProp where module_id = 15269889;
delete from RepDescExecute where module_id = 15269889;
\p\g

/* Frontier Application Server Objects */
insert into RepDescObject values (15269889, 1, 'fas_srvr');
insert into RepDescObject values (15269889, 2, 'fas_appl');
insert into RepDescObject values (15269889, 3, 'fas_intfc');
insert into RepDescObject values (15269889, 4, 'fas_method');
insert into RepDescObject values (15269889, 5, 'fas_arg');
\p\g
	/* Application Server: table fas_srvr */
insert into RepDescProp values (15269889, 1, 1, 'fas_srvr_id',	 1, 'd', 4,  0);
insert into RepDescProp values (15269889, 1, 2, 'fas_srvr_name', 0, 's', 32, 0);
\p\g

	/* Application: table fas_appl */
insert into RepDescProp values (15269889, 2, 1, 'fas_srvr_id',	1, 'd', 4,   0);
insert into RepDescProp values (15269889, 2, 2, 'fas_appl_id',	1, 'd', 4,   0);
insert into RepDescProp values (15269889, 2, 3, 'fas_appl_name',0, 's', 32,  0);
insert into RepDescProp values (15269889, 2, 4, 'fas_appl_loc',	0, 's', 260, 0);
insert into RepDescProp values (15269889, 2, 5, 'fas_appl_type',0, 'd', 4,   0);
insert into RepDescProp values (15269889, 2, 6, 'fas_appl_parm',0, 's', 1024, 0);
\p\g

	/* Interface: table fas_intfc */
insert into RepDescProp values (15269889, 3, 1, 'fas_srvr_id',	1, 'd', 4,   0);
insert into RepDescProp values (15269889, 3, 2, 'fas_appl_id',	1, 'd', 4,   0);
insert into RepDescProp values (15269889, 3, 3, 'fas_intfc_id',	1, 'd', 4,   0);
insert into RepDescProp values (15269889, 3, 4, 'fas_intfc_name',0,'s', 32,  0);
insert into RepDescProp values (15269889, 3, 5, 'fas_intfc_loc', 0,'s', 260, 0);
insert into RepDescProp values (15269889, 3, 6, 'fas_intfc_parm',0,'s', 1024, 0);
\p\g

	/* Method: table fas_method */
insert into RepDescProp values (15269889, 4, 1, 'fas_srvr_id',	1, 'd', 4,  0);
insert into RepDescProp values (15269889, 4, 2, 'fas_appl_id',	1, 'd', 4,  0);
insert into RepDescProp values (15269889, 4, 3, 'fas_intfc_id',	1, 'd', 4,  0);
insert into RepDescProp values (15269889, 4, 4, 'fas_method_id',1, 'd', 4,  0);
insert into RepDescProp values (15269889, 4, 5, 'fas_method_name',0,'s',32, 0);
\p\g

	/* Argument: table fas_arg */
insert into RepDescProp values (15269889, 5, 1, 'fas_srvr_id',	1, 'd', 4,  0);
insert into RepDescProp values (15269889, 5, 2, 'fas_appl_id',	1, 'd', 4,  0);
insert into RepDescProp values (15269889, 5, 3, 'fas_intfc_id',	1, 'd', 4,  0);
insert into RepDescProp values (15269889, 5, 4, 'fas_method_id',1, 'd', 4,  0);
insert into RepDescProp values (15269889, 5, 5, 'fas_arg_id',	1, 'd', 4,  0);
insert into RepDescProp values (15269889, 5, 6, 'fas_arg_name',	0, 's', 32, 0);
insert into RepDescProp values (15269889, 5, 7, 'fas_arg_type',	0, 'd', 4,  0);
insert into RepDescProp values (15269889, 5, 8, 'fas_arg_datatype',0,'d',2, 0);
insert into RepDescProp values (15269889, 5, 9, 'fas_arg_length',0,'d', 4,  0);
insert into RepDescProp values (15269889, 5,10,	'fas_arg_precision',0,'d',2,0);
\p\g

drop fas_srvr;
drop fas_appl;
drop fas_intfc;
drop fas_method;
drop fas_arg;
\p\g

create table fas_srvr(
	fas_srvr_id integer not null,
	fas_srvr_name varchar(32) not null);
	\p\g
modify fas_srvr to btree unique on fas_srvr_id;
\p\g

create table fas_appl (
	fas_srvr_id Integer not null,
	fas_appl_id Integer not null,
	fas_appl_name varchar(32) not null,
	fas_appl_loc varchar(260) not null,
	fas_appl_type Integer not null,
	fas_appl_parm varchar(1024) not null);
	\p\g
modify fas_appl to btree unique on fas_srvr_id,fas_appl_id;
\p\g

create table fas_intfc (
	fas_srvr_id Integer not null,
	fas_appl_id Integer not null,
	fas_intfc_id Integer not null,
	fas_intfc_name varchar(32) not null,
	fas_intfc_loc varchar(260) not null,
	fas_intfc_parm varchar(1024) not null);
	\p\g
modify fas_intfc to btree unique on fas_srvr_id,fas_appl_id,fas_intfc_id;
\p\g

create table fas_method (
	fas_srvr_id Integer not null,
	fas_appl_id Integer not null,
	fas_intfc_id Integer not null,
	fas_method_id Integer not null,
	fas_method_name varchar(32) not null);
	\p\g
modify fas_method to btree unique on fas_srvr_id,fas_appl_id,fas_intfc_id,
	fas_method_id;
\p\g

create table fas_arg (
	fas_srvr_id Integer not null,
	fas_appl_id Integer not null,
	fas_intfc_id Integer not null,
	fas_method_id Integer not null,
	fas_arg_id Integer not null,
	fas_arg_name varchar(32) not null,
	fas_arg_type Integer not null,
	fas_arg_datatype integer not null,
	fas_arg_length integer not null,
	fas_arg_precision integer not null);
	\p\g
modify fas_arg to btree unique on fas_srvr_id,fas_appl_id,fas_intfc_id,
	fas_method_id,fas_arg_id;
\p\g

grant all on fas_srvr to public;
grant all on fas_appl to public;
grant all on fas_intfc to public;
grant all on fas_method to public;
grant all on fas_arg to public;
\p\g

insert into fas_srvr values(1, 'frontier');
\p\g

/* 
**	The internal Frontier application and interface 
**
** NOTE: module_number = "_UR_CLASS" 233 * 0x10000 = 15269888 
*/
insert into fas_appl values(1, 15269888, 'oiursnt', 'e:\frontier\ingres\bin', 2, '');
\p\g

/*
**	The Frontier interface and internal methods
*/
insert into fas_intfc	values(1, 15269888, 1, 'Frontier', '', '');
\p\g
insert into fas_method	values(1, 15269888, 1, 1, 'GetInterfaces');
insert into fas_arg		values(1, 15269888, 1, 1, 1, 'InterfaceName',	2, 21, 32,   0);
insert into fas_arg		values(1, 15269888, 1, 1, 2, 'InterfacePath',	2, 21, 260,		0);
insert into fas_arg		values(1, 15269888, 1, 1, 3, 'InterfaceParm',	2, 21, 1024, 0);
\p\g
insert into fas_method	values(1, 15269888, 1, 2, 'GetMethods');
insert into fas_arg		values(1, 15269888, 1, 2, 1, 'InterfaceName',	1, 21, 32,   0);
insert into fas_arg		values(1, 15269888, 1, 2, 2, 'MethodName',		2, 21, 32,   0);
insert into fas_arg		values(1, 15269888, 1, 2, 3, 'MethodArgCount',	2, 30, 4,    0);
\p\g
insert into fas_method	values(1, 15269888, 1, 3, 'GetArgs');
insert into fas_arg		values(1, 15269888, 1, 3, 1, 'InterfaceName',	1, 21, 32,   0);
insert into fas_arg		values(1, 15269888, 1, 3, 2, 'MethodName',		1, 21, 32,   0);
insert into fas_arg		values(1, 15269888, 1, 3, 3, 'ArgName',			2, 21, 32,   0);
insert into fas_arg		values(1, 15269888, 1, 3, 4, 'ds_dataType',		2, 30, 4,    0);
insert into fas_arg		values(1, 15269888, 1, 3, 5, 'ds_nullable',		2, 30, 4,    0);
insert into fas_arg		values(1, 15269888, 1, 3, 6, 'ds_length',		2, 30, 4,    0);
insert into fas_arg		values(1, 15269888, 1, 3, 7, 'ds_precision',	2, 30, 2,    0);
insert into fas_arg		values(1, 15269888, 1, 3, 8, 'ds_scale',		2, 30, 2,    0);
insert into fas_arg		values(1, 15269888, 1, 3, 9, 'ds_columnType',	2, 30, 2,    0);
\p\g
insert into fas_method	values(1, 15269888, 1, 4, 'RefreshInterface');
insert into fas_arg		values(1, 15269888, 1, 4, 1, 'InterfaceName',	1, 21, 32,   0);
\p\g

/* 
**	The FasC test application and interface 
**
** NOTE: module_number = "_UR_CLASS" 233 * 0x10000 + 1 = 15269889
*/
insert into fas_appl values(1, 15269889, 'oiurtnt', 'e:\frontier\ingres\bin', 2, '');
\p\g

/*
**	The FasC test interface and internal methods
*/
insert into fas_intfc	values(1, 15269889, 1, 'FasC', '', '');
\p\g
insert into fas_method	values(1, 15269889, 1, 1, 'GetInterfaces');
insert into fas_arg		values(1, 15269889, 1, 1, 1, 'InterfaceName',	2, 21, 32,   0);
insert into fas_arg		values(1, 15269889, 1, 1, 2, 'InterfacePath',	2, 21, 260,		0);
insert into fas_arg		values(1, 15269889, 1, 1, 3, 'InterfaceParm',	2, 21, 1024, 0);
\p\g
insert into fas_method	values(1, 15269889, 1, 2, 'GetMethods');
insert into fas_arg		values(1, 15269889, 1, 2, 1, 'InterfaceName',	1, 21, 32,   0);
insert into fas_arg		values(1, 15269889, 1, 2, 2, 'MethodName',		2, 21, 32,   0);
insert into fas_arg		values(1, 15269889, 1, 2, 3, 'MethodArgCount',	2, 30, 4,    0);
\p\g
insert into fas_method	values(1, 15269889, 1, 3, 'GetArgs');
insert into fas_arg		values(1, 15269889, 1, 3, 1, 'InterfaceName',	1, 21, 32,   0);
insert into fas_arg		values(1, 15269889, 1, 3, 2, 'MethodName',		1, 21, 32,   0);
insert into fas_arg		values(1, 15269889, 1, 3, 3, 'ArgName',			2, 21, 32,   0);
insert into fas_arg		values(1, 15269889, 1, 3, 4, 'ds_dataType',		2, 30, 4,    0);
insert into fas_arg		values(1, 15269889, 1, 3, 5, 'ds_nullable',		2, 30, 4,    0);
insert into fas_arg		values(1, 15269889, 1, 3, 6, 'ds_length',		2, 30, 4,    0);
insert into fas_arg		values(1, 15269889, 1, 3, 7, 'ds_precision',	2, 30, 2,    0);
insert into fas_arg		values(1, 15269889, 1, 3, 8, 'ds_scale',		2, 30, 2,    0);
insert into fas_arg		values(1, 15269889, 1, 3, 9, 'ds_columnType',	2, 30, 2,    0);
\p\g
insert into fas_method	values(1, 15269889, 1, 4, 'RefreshInterface');
insert into fas_arg		values(1, 15269889, 1, 4, 1, 'InterfaceName',	1, 21, 32,   0);
\p\g
insert into fas_method	values(1, 15269889,	1, 5, 'GetTime');
insert into fas_arg		values(1, 15269889,	1, 5, 1, 'Time',			2, 21, 512,		0);
\p\g
insert into fas_method	values(1, 15269889, 1, 6, 'Sum');
insert into fas_arg		values(1, 15269889, 1, 6, 1, 'i',				1, 30,	4,				0);
insert into fas_arg		values(1, 15269889, 1, 6, 2, 'j',				1, 30,	4,				0);
insert into fas_arg		values(1, 15269889, 1, 6, 3, 'result',			2, 30,	4,				0);
\p\g

/* 
** OpenRoad application 
*/
insert into fas_appl values(1, 1, 'orntier', 'e:\frontier\ingres\bin', 1, '');
\p\g

/* 
** Sample "query" interface and "runquery" method 
*/
insert into fas_intfc	values(1, 1, 1, 'query.img', '', '-Tno');
insert into fas_method	values(1, 1, 1, 1, 'runquery');
insert into fas_arg		values(1, 1, 1, 1, 1, 'DBName',	1, 32, 32,   0);
insert into fas_arg		values(1, 1, 1, 1, 2, 'Input',	1, 21, 2000, 0);
insert into fas_arg		values(1, 1, 1, 1, 3, 'Output',	2, 21, 2000, 0);
insert into fas_arg		values(1, 1, 1, 1, 4, 'Status',	2, 30, 4,    0);
\p\g