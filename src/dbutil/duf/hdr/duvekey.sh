/*
**Copyright (c) 2004 Ingres Corporation
*/
/* Description of table iikey from database iidbdb */
  EXEC SQL DECLARE iikey TABLE
	(key_consid1	integer not null,
	 key_consid2	integer not null,
	 key_attid	smallint not null,
	 key_position	smallint not null);

  struct key_tbl_ {
	i4	key_consid1;
	i4	key_consid2;
	short	key_attid;
	short   key_position;
  } key_tbl; 	
