/* Description of table iidatabase from database iidbdb */
  EXEC SQL DECLARE iidatabase TABLE
	(name	char(32) not null,
	 own	char(32) not null,
	 dbdev	char(32) not null,
	 ckpdev	char(32) not null,
	 jnldev	char(32) not null,
	 sortdev	char(32) not null,
	 access	integer not null,
	 dbservice	integer not null,
	 dbcmptlvl	integer not null,
	 dbcmptminor	integer not null,
	 db_id	integer not null,
	 dmpdev	char(32) not null,
	 dbfree	char(8) not null);

  struct db_tbl_ {
	char	name[33];
	char	own[33];
	char	dbdev[33];
	char	ckpdev[33];
	char	jnldev[33];
	char	sortdev[33];
	i4	access;
	i4	dbservice;
	u_i4	dbcmptlvl;
	i4	dbcmptminor;
	i4	db_id;
	char	dmpdev[33];
	char	dbfree[9];
  } db_tbl;
