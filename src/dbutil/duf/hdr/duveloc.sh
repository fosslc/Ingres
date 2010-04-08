/* Description of table iilocations from database iidbdb */
  EXEC SQL DECLARE iilocations TABLE
	(status	integer not null,
	 lname	varchar(32) not null,
	 area	varchar(128) not null,
	 free	char(8) not null,
	 rawpct	integer not null);

  struct loc_tbl_ {
	i4	status;
	char	lname[33];
	char	area[129];
	char	free[9];
	i4	rawpct;
  } loc_tbl;
