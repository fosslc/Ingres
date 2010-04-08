/* Description of table iiextend from database iidbdb */
  EXEC SQL DECLARE iiextend TABLE
	(lname	varchar(32) not null,
	 dname	varchar(32) not null,
	 status	integer not null,
	 rawstart	integer not null,
	 rawblocks	integer not null);

  struct ext_tbl_ {
	char	lname[33];
	char	dname[33];
	i4	status;
	i4	rawstart;
	i4	rawblocks;
  } ext_tbl;
