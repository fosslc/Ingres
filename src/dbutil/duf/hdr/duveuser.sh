/* Description of table iiuser from database iidbdb */
  EXEC SQL DECLARE iiuser TABLE
	(name	char(32) not null,
	 gid	smallint not null,
	 mid	smallint not null,
	 status	integer not null,
	 default_group	char(32) not null,
	 password	char(24) not null,
	 reserve	char(8) not null,
	 profile_name	char(32) not null,
	 default_priv	integer not null,
	 expire_date	date not null,
	 flags_mask	integer not null,
	 user_priv	integer not null);

  struct user_tbl_ {
	char	name[33];
	i2	gid;
	i2	mid;
	i4	status;
	char	default_group[33];
	char	password[25];
	char	reserve[9];
	char	profile_name[33];
	i4	default_priv;
	char	expire_date[26];
	i4	flags_mask;
	i4	user_priv;
  } user_tbl;
