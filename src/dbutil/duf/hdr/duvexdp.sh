/*
**Copyright (c) 2004 Ingres Corporation
*/
/* Description of table iixdbdepends from database iidbdb */
  EXEC SQL DECLARE iixdbdepends TABLE
	(deid1	integer not null,
	 deid2	integer not null,
	 dtype	integer not null,
	 qid	integer not null,
	 tidp	integer not null);

  struct xdpnd_tbl_ {
	i4	deid1;
	i4	deid2;
	i4	dtype;
	i4	qid;
	i4	tidp;
  } xdpnd_tbl;
