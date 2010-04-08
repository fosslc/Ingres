/*
**Copyright (c) 2004 Ingres Corporation
*/
/* Description of table iistatistics from database iidbdb */
  EXEC SQL DECLARE iistatistics TABLE
	(stabbase	integer not null,
	 stabindex	integer not null,
	 snunique	float4 not null,
	 sreptfact	float4 not null,
	 snull	float4 not null,
	 satno	smallint not null,
	 snumcells	smallint not null,
	 sdomain	smallint not null,
	 scomplete	integer1 not null,
	 sunique	integer1 not null,
	 sdate	date not null,
	 sversion	char(8) not null,
	 shistlength	smallint not null);

  struct stat_tbl_ {
	i4	stabbase;
	i4	stabindex;
	float	snunique;
	float	sreptfact;
	float	snull;
	short	satno;
	short	snumcells;
	short	sdomain;
	short	scomplete;
	short	sunique;
	char	sdate[26];
	char	sversion[9];
	short	shistlength;
  } stat_tbl;
