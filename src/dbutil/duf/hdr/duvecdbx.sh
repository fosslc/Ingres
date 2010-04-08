/*
**Copyright (c) 2004 Ingres Corporation
*/
/* Description of table iicdbid_idx from database iidbdb */
  EXEC SQL DECLARE iicdbid_idx TABLE
	(cdb_id	integer not null,
	 tidp	integer not null);

  struct cdb_indx_ {
	i4	cdb_id;
	i4	tidp;
  } cdb_indx;
