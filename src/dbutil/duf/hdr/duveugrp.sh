/*
**Copyright (c) 2004 Ingres Corporation
*/
/* Description of table iiusergroup from database iidbdb */
  EXEC SQL DECLARE iiusergroup TABLE
	(groupid	char(32) not null,
	 groupmem	char(32) not null,
	 reserve	char(32) not null);

  struct ugrp_tbl_ {
	char	groupid[33];
	char	groupmem[33];
	char	reserve[33];
  } ugrp_tbl;
