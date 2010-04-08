/*
**Copyright (c) 2004 Ingres Corporation
*/
/* Description of table iihistogram from database iidbdb */
  EXEC SQL DECLARE iihistogram TABLE
	(htabbase	integer not null,
	 htabindex	integer not null,
	 hatno	smallint not null,
	 hsequence	smallint not null,
	 histogram	char(228) not null);

  struct hist_tbl_ {
	i4	htabbase;
	i4	htabindex;
	short	hatno;
	short	hsequence;
	char	histogram[229];
  } hist_tbl;
