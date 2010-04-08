/*
**Copyright (c) 2004 Ingres Corporation
*/
/* description of table iigw06_relation */

EXEC SQL DECLARE iigw06_relation TABLE
	(reltid		integer not null,
	 reltidx	integer not null,
	 audit_log	char(256) not null,
	 reg_date	integer not null,
	 flags		integer not null);

struct gwrel_tbl_ {
	i4 	reltid;
	i4 	reltidx;
	char 	audit_log[256];
	i4	reg_date;
	i4	flags;
} gwrel_tbl;
