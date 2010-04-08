/*
**Copyright (c) 2004 Ingres Corporation
*/
/* description of table iigw06_attribute */

EXEC SQL DECLARE iigw06_attribute TABLE
	(reltid		integer not null,
	 reltidx	integer not null,
	 attid		smallint not null,
	 attname	char(32) not null,
	 auditid	smallint not null,
	 auditname	char(32) not null);

struct gwatt_tbl_ {
	i4 	reltid;
	i4 	reltidx;
	short	attid;
	char	attname[32];
	short	auditid;
	char	auditname[32];
} gwatt_tbl;
