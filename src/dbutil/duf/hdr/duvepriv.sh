/*
**Copyright (c) 2004 Ingres Corporation
*/
/* Description of table iidbpriv from database iidbdb */
  EXEC SQL DECLARE iidbpriv TABLE
	(dbname	char(32) not null,
	 grantee	char(32) not null,
	 gtype	smallint not null,
	 dbflags	smallint not null,
	 control	integer not null,
	 flags	integer not null,
	 qrow_limit	integer not null,
	 qdio_limit	integer not null,
	 qcpu_limit	integer not null,
	 qpage_limit	integer not null,
	 qcost_limit	integer not null,
	 idle_time_limit	integer not null,
	 connect_time_limit	integer not null,
	 priority_limit	integer not null,
	 reserve	char(32) not null);

  struct priv_tbl_ {
	char	dbname[33];
	char	grantee[33];
	short	gtype;
	short	dbflags;
	i4	control;
	i4	flags;
	i4	qrow_limit;
	i4	qdio_limit;
	i4	qcpu_limit;
	i4	qpage_limit;
	i4	qcost_limit;
	i4	idle_time_limit;
	i4	connect_time_limit;
	i4	priority_limit;
	char	reserve[33];
  } priv_tbl;
