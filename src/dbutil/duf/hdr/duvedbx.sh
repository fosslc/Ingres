/*
**Copyright (c) 2004 Ingres Corporation
*/
/* Description of table iidbid_idx from database iidbdb */
  EXEC SQL DECLARE iidbid_idx TABLE
	(db_id	integer not null,
	 tidp	integer not null);

  struct dbx_tbl_ {
	i4	db_id;
	i4	tidp;
  } dbx_tbl;
