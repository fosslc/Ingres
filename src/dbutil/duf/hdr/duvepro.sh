/*
**Copyright (c) 2004 Ingres Corporation
*/
/** History:
**
**	15-mar-2002 (toumi01)
**	   add prodomset11-33 to support 1024 cols/row
**	27-Apr-2010 (kschendel) SIR 123639
**	    Byte-ize the bitmap columns.
**
*/
/* Description of table iiprotect from database iidbdb
  EXEC SQL DECLARE iiprotect TABLE
	(protabbase	integer not null,
	 protabidx	integer not null,
	 propermid	smallint not null,
	 proflags	smallint not null,
	 protodbgn	smallint not null,
	 protodend	smallint not null,
	 prodowbgn	smallint not null,
	 prodowend	smallint not null,
	 proqryid1	integer not null,
	 proqryid2	integer not null,
	 protreeid1	integer not null,
	 protreeid2	integer not null,
	 procrtime1	integer not null,
	 procrtime2	integer not null,
	 proopctl	integer not null,
	 proopset	integer not null,
	 provalue	integer not null,
	 prodepth	smallint not null,
	 probjtype	char(1) not null,
	 probjstat	char(1) not null,
	 probjname	char(32) not null,
	 probjowner	char(32) not null,
	 prograntor	char(32) not null,
	 prouser	char(32) not null,
	 proterm	char(16) not null,
	 profill3	smallint not null,
	 progtype	smallint not null,
	 proseq	integer not null,
	 prodomset	byte(DB_COL_BYTES) not null,
	 proreserve	char(32) not null);
*/

  struct pro_tbl_ {
	i4	protabbase;
	i4	protabidx;
	short	propermid;
	short	proflags;
	short	protodbgn;
	short	protodend;
	short	prodowbgn;
	short	prodowend;
	i4	proqryid1;
	i4	proqryid2;
	i4	protreeid1;
	i4	protreeid2;
	i4	procrtime1;
	i4	procrtime2;
	i4	proopctl;
	i4	proopset;
	i4	provalue;
	short	prodepth;
	char	probjtype[2];
	char	probjstat[2];
	char	probjname[33];
	char	probjowner[33];
	char	prograntor[33];
	char	prouser[33];
	char	proterm[17];
	short	profill3;
	short	progtype;
	i4	proseq;
	char	prodomset[DB_COL_BYTES];
	char	proreserve[33];
  } pro_tbl;
