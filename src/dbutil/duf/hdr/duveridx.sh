/*
**Copyright (c) 2004 Ingres Corporation
*/
/* Description of table iirel_idx from database iidbdb */
  EXEC SQL DECLARE iirel_idx TABLE
	(relid	char(32) not null,
	 relowner	char(32) not null,
	 tidp	integer not null);

  struct ridx_tbl_ {
	char	relid[33];
	char	relowner[33];
	i4	tidp;
  } ridx_tbl;
