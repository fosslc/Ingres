/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
**  Name: duveint.sh	- iiintegrity description.
**
**  Description:
**	This file contains the description of table iiintegrity from
**	database iidbdb
**
**  History:
**	06-nov-2000 (somsa01)
**	    Added consdelrule and consupdrule.
**	15-mar-2002 (toumi01)
**	   add intdomset11-33 to support 1024 cols/row
*/
/* Description of table iiintegrity from database iidbdb */

/** History:
 ** 
 **	27-jul-2001 ( huazh01 )
 **	   add consdelrule, consupdrule, change the size of 
 **	   intreserve to reflect the change of 
 ** 	   table iiintegrity from 2.0 to 2.5
 **         
 */

  EXEC SQL DECLARE iiintegrity TABLE
	(inttabbase	integer not null,
	 inttabidx	integer not null,
	 intqryid1	integer not null,
	 intqryid2	integer not null,
	 inttreeid1	integer not null,
	 inttreeid2	integer not null,
	 intresvar	smallint not null,
	 intnumber	smallint not null,
	 intseq	integer not null,
	 intdomset1	integer not null,
	 intdomset2	integer not null,
	 intdomset3	integer not null,
	 intdomset4	integer not null,
	 intdomset5	integer not null,
	 intdomset6	integer not null,
	 intdomset7	integer not null,
	 intdomset8	integer not null,
	 intdomset9	integer not null,
	 intdomseta	integer not null,
	 intdomset11	integer not null,
	 intdomset12	integer not null,
	 intdomset13	integer not null,
	 intdomset14	integer not null,
	 intdomset15	integer not null,
	 intdomset16	integer not null,
	 intdomset17	integer not null,
	 intdomset18	integer not null,
	 intdomset19	integer not null,
	 intdomset20	integer not null,
	 intdomset21	integer not null,
	 intdomset22	integer not null,
	 intdomset23	integer not null,
	 intdomset24	integer not null,
	 intdomset25	integer not null,
	 intdomset26	integer not null,
	 intdomset27	integer not null,
	 intdomset28	integer not null,
	 intdomset29	integer not null,
	 intdomset30	integer not null,
	 intdomset31	integer not null,
	 intdomset32	integer not null,
	 intdomset33	integer not null,
	 consname	char(32) not null,
	 consid1	integer not null,
	 consid2	integer not null,
	 consschema_id1	integer not null,
	 consschema_id2	integer not null,
	 consflags	integer not null,
	 cons_create_date	integer not null,
	 consdelrule	smallint not null,
	 consupdrule	smallint not null,
	 intreserve	char(32) not null);

  struct int_tbl_ {
	i4	inttabbase;
	i4	inttabidx;
	i4	intqryid1;
	i4	intqryid2;
	i4	inttreeid1;
	i4	inttreeid2;
	short	intresvar;
	short	intnumber;
	i4	intseq;
	i4	intdomset1;
	i4	intdomset2;
	i4	intdomset3;
	i4	intdomset4;
	i4	intdomset5;
	i4	intdomset6;
	i4	intdomset7;
	i4	intdomset8;
	i4	intdomset9;
	i4	intdomseta;
	i4	intdomset11;
	i4	intdomset12;
	i4	intdomset13;
	i4	intdomset14;
	i4	intdomset15;
	i4	intdomset16;
	i4	intdomset17;
	i4	intdomset18;
	i4	intdomset19;
	i4	intdomset20;
	i4	intdomset21;
	i4	intdomset22;
	i4	intdomset23;
	i4	intdomset24;
	i4	intdomset25;
	i4	intdomset26;
	i4	intdomset27;
	i4	intdomset28;
	i4	intdomset29;
	i4	intdomset30;
	i4	intdomset31;
	i4	intdomset32;
	i4	intdomset33;
	char	consname[33];
	i4	consid1;
	i4	consid2;
	i4	consschema_id1;
	i4	consschema_id2;
	i4	consflags;
	i4	cons_create_date;
	short	consdelrule;
	short	consupdrule;
	char	intreserve[33];
  } int_tbl;
