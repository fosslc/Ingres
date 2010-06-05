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
**	27-Apr-2010 (kschendel) SIR 123639
**	    Byte-ize the bitmap columns.
*/
/* Description of table iiintegrity from database iidbdb */

/** History:
 ** 
 **	27-jul-2001 ( huazh01 )
 **	   add consdelrule, consupdrule, change the size of 
 **	   intreserve to reflect the change of 
 ** 	   table iiintegrity from 2.0 to 2.5
 **         

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
	 intdomset	byte(DB_COL_BYTES) not null,
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
 */

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
	char	intdomset[DB_COL_BYTES];
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
