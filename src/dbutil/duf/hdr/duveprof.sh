/* Description of table iiprofile from database iidbdb */
  EXEC SQL DECLARE iiprofile TABLE
	(name	char(32) not null,
	 status	integer not null,
	 default_group	char(32) not null,
	 reserve	char(8) not null,
	 default_priv	integer not null,
	 expire_date	date not null,
	 flags_mask	integer not null);

  struct uprof_tbl_ {
	char	name[33];
	i4	status;
	char	default_group[33];
	char	reserve[9];
	i4	default_priv;
	char	expire_date[26];
	i4	flags_mask;
  } uprof_tbl;
