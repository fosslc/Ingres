/*
**Copyright (c) 2004 Ingres Corporation
*/
/** History:
**
**	15-mar-2002 (toumi01)
**	   add prodomset11-33 to support 1024 cols/row
**
*/
/* Description of table iiprotect from database iidbdb */
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
	 prodomset1	integer not null,
	 prodomset2	integer not null,
	 prodomset3	integer not null,
	 prodomset4	integer not null,
	 prodomset5	integer not null,
	 prodomset6	integer not null,
	 prodomset7	integer not null,
	 prodomset8	integer not null,
	 prodomset9	integer not null,
	 prodomseta	integer not null,
	 prodomset11	integer not null,
	 prodomset12	integer not null,
	 prodomset13	integer not null,
	 prodomset14	integer not null,
	 prodomset15	integer not null,
	 prodomset16	integer not null,
	 prodomset17	integer not null,
	 prodomset18	integer not null,
	 prodomset19	integer not null,
	 prodomset20	integer not null,
	 prodomset21	integer not null,
	 prodomset22	integer not null,
	 prodomset23	integer not null,
	 prodomset24	integer not null,
	 prodomset25	integer not null,
	 prodomset26	integer not null,
	 prodomset27	integer not null,
	 prodomset28	integer not null,
	 prodomset29	integer not null,
	 prodomset30	integer not null,
	 prodomset31	integer not null,
	 prodomset32	integer not null,
	 prodomset33	integer not null,
	 proreserve	char(32) not null);

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
	i4	prodomset1;
	i4	prodomset2;
	i4	prodomset3;
	i4	prodomset4;
	i4	prodomset5;
	i4	prodomset6;
	i4	prodomset7;
	i4	prodomset8;
	i4	prodomset9;
	i4	prodomseta;
	i4	prodomset11;
	i4	prodomset12;
	i4	prodomset13;
	i4	prodomset14;
	i4	prodomset15;
	i4	prodomset16;
	i4	prodomset17;
	i4	prodomset18;
	i4	prodomset19;
	i4	prodomset20;
	i4	prodomset21;
	i4	prodomset22;
	i4	prodomset23;
	i4	prodomset24;
	i4	prodomset25;
	i4	prodomset26;
	i4	prodomset27;
	i4	prodomset28;
	i4	prodomset29;
	i4	prodomset30;
	i4	prodomset31;
	i4	prodomset32;
	i4	prodomset33;
	char	proreserve[33];
  } pro_tbl;
