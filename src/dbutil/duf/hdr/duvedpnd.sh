/*
**Copyright (c) 2004 Ingres Corporation
*/
/* Description of table iidbdepends from database iidbdb */
  EXEC SQL DECLARE iidbdepends TABLE
	(inid1	integer not null,
	 inid2	integer not null,
	 itype	integer not null,
	 i_qid	integer not null,
	 qid	integer not null,
	 deid1	integer not null,
	 deid2	integer not null,
	 dtype	integer not null);

  struct dpnd_tbl_ {
	i4	inid1;
	i4	inid2;
	i4	itype;
	i4	i_qid;
	i4	qid;
	i4	deid1;
	i4	deid2;
	i4	dtype;
  } dpnd_tbl;
