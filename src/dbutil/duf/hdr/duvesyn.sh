/*
**Copyright (c) 2004 Ingres Corporation
*/
/* Description of table iisynonym from database iidbdb */
  EXEC SQL DECLARE iisynonym TABLE
	(synonym_name	char(32) not null,
	 synonym_owner	char(32) not null,
	 syntabbase	integer not null,
	 syntabidx	integer not null,
	 synid	integer not null,
	 synidx	integer not null);

  struct syn_tbl_ {
	char	synonym_name[33];
	char	synonym_owner[33];
	i4	syntabbase;
	i4	syntabidx;
	i4	synid;
	i4	synidx;
  } syn_tbl;
