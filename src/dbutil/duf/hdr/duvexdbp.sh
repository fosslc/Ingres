/*
**Copyright (c) 2004 Ingres Corporation
*/
/* Description of table iixprocedure from database iidbdb */
  EXEC SQL DECLARE iixprocedure TABLE
	(dbp_id	integer not null,
	 dbp_idx	integer not null,
	 tidp	integer not null);

  struct xdbp_tbl_ {
	i4	dbp_id;
	i4	dbp_idx;
	i4	tidp;
  } xdbp_tbl;
