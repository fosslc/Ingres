/*
**Copyright (c) 2004 Ingres Corporation
*/
/* Description of table iidbms_comment from database iidbdb */
  EXEC SQL DECLARE iidbms_comment TABLE
	(comtabbase	integer not null,
	 comtabidx	integer not null,
	 comattid	smallint not null,
	 comtype	smallint not null,
	 short_remark	char(60) not null,
	 text_sequence	smallint not null,
	 long_remark	varchar(1600) not null);

  struct cmnt_tbl_ {
	i4	comtabbase;
	i4	comtabidx;
	short	comattid;
	short	comtype;
	char	short_remark[61];
	short	text_sequence;
	char	long_remark[1601];
  } cmnt_tbl;
