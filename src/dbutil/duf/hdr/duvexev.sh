/*
**Copyright (c) 2004 Ingres Corporation
*/
/* Description of table iixevent from database iidbdb */
  EXEC SQL DECLARE iixevent TABLE
	(event_idbase	integer not null,
	 event_idx	integer not null,
	 tidp	integer not null);

  struct xev_tbl_ {
	i4	event_idbase;
	i4	event_idx;
	i4	tidp;
  } xev_tbl;
