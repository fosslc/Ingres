/*
**Copyright (c) 2004 Ingres Corporation
*/
/* Description of table iistar_cdbs from database iidbdb */
  EXEC SQL DECLARE iistar_cdbs TABLE
	(ddb_name	char(32) not null,
	 ddb_owner	char(32) not null,
	 cdb_name	char(32) not null,
	 cdb_owner	char(32) not null,
	 cdb_node	char(32) not null,
	 cdb_dbms	char(32) not null,
	 scheme_desc	char(32) not null,
	 create_date	char(25) not null,
	 original	char(8) not null,
	 cdb_id	integer not null,
	 cdb_capability	integer not null);

  struct cdbs_tbl_ {
	char	ddb_name[33];
	char	ddb_owner[33];
	char	cdb_name[33];
	char	cdb_owner[33];
	char	cdb_node[33];
	char	cdb_dbms[33];
	char	scheme_desc[33];
	char	create_date[26];
	char	original[9];
	i4	cdb_id;
	i4	cdb_capability;
  } cdbs_tbl;
