/*
**Copyright (c) 2004 Ingres Corporation
*/
/* Description of table iiindex from database iidbdb */
  EXEC SQL DECLARE iiindex TABLE
	(baseid	integer not null,
	 indexid	integer not null,
	 sequence	smallint not null,
	 idom1	smallint not null,
	 idom2	smallint not null,
	 idom3	smallint not null,
	 idom4	smallint not null,
	 idom5	smallint not null,
	 idom6	smallint not null,
	 idom7	smallint not null,
	 idom8	smallint not null,
	 idom9	smallint not null,
	 idom10	smallint not null,
	 idom11	smallint not null,
	 idom12	smallint not null,
	 idom13	smallint not null,
	 idom14	smallint not null,
	 idom15	smallint not null,
	 idom16	smallint not null,
	 idom17	smallint not null,
	 idom18	smallint not null,
	 idom19	smallint not null,
	 idom20	smallint not null,
	 idom21	smallint not null,
	 idom22	smallint not null,
	 idom23	smallint not null,
	 idom24	smallint not null,
	 idom25	smallint not null,
	 idom26	smallint not null,
	 idom27	smallint not null,
	 idom28	smallint not null,
	 idom29	smallint not null,
	 idom30	smallint not null,
	 idom31	smallint not null,
	 idom32	smallint not null);

  struct index_tbl_ {
	i4	baseid;
	i4	indexid;
	short	sequence;
	short	idom1;
	short	idom2;
	short	idom3;
	short	idom4;
	short	idom5;
	short	idom6;
	short	idom7;
	short	idom8;
	short	idom9;
	short	idom10;
	short	idom11;
	short	idom12;
	short	idom13;
	short	idom14;
	short	idom15;
	short	idom16;
	short	idom17;
	short	idom18;
	short	idom19;
	short	idom20;
	short	idom21;
	short	idom22;
	short	idom23;
	short	idom24;
	short	idom25;
	short	idom26;
	short	idom27;
	short	idom28;
	short	idom29;
	short	idom30;
	short	idom31;
	short	idom32;
  } index_tbl;
