/*
**Copyright (c) 2004 Ingres Corporation
*/
/* Description of table iiqrytext from database iidbdb */
  EXEC SQL DECLARE iiqrytext TABLE
	(txtid1	integer not null,
	 txtid2	integer not null,
	 mode	integer not null,
	 seq	integer not null,
	 status	integer not null,
	 txt	varchar(240) not null);

  struct qry_tbl_ {
	i4	txtid1;
	i4	txtid2;
	i4	mode;
	i4	seq;
	i4	status;
	char	txt[241];
  } qry_tbl;
