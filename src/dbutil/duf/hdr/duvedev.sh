/*
**Copyright (c) 2004 Ingres Corporation
*/
/* Description of table iidevices from database iidbdb */
  EXEC SQL DECLARE iidevices TABLE
	(devrelid	integer not null,
	 devrelidx	integer not null,
	 devrelocid	integer not null,
	 devloc	char(32) not null);

  struct dev_tbl_ {
	i4	devrelid;
	i4	devrelidx;
	i4	devrelocid;
	char	devloc[33];
  } dev_tbl;
