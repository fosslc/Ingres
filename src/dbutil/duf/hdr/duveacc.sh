/*
**Copyright (c) 2004 Ingres Corporation
*/
/* Table iidbaccess is obsoleted for 6.5.  This is the 6.4 structure. */

  EXEC SQL DECLARE iidbaccess TABLE
	(usrname	char(24) not null,
	 dbname	char(24) not null,
	 xtra	char(24) not null);

  struct acc_tbl_ {
	char	usrname[25];
	char	dbname[25];
	char	xtra[25];
  } acc_tbl;
