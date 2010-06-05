/*
** Copyright 2010 Ingres Corp
**
**
**	27-Apr-2010 (kschendel) SIR 123639
**	    Byte-ize the bitmap columns.
*/
/* Description of table iirule from database iidbdb
  EXEC SQL DECLARE iirule TABLE
	(rule_name	char(32) not null,
	 rule_owner	char(32) not null,
	 rule_type	smallint not null,
	 rule_flags	smallint not null,
	 rule_tabbase	integer not null,
	 rule_tabidx	integer not null,
	 rule_qryid1	integer not null,
	 rule_qryid2	integer not null,
	 rule_treeid1	integer not null,
	 rule_treeid2	integer not null,
	 rule_statement	integer not null,
	 rule_dbp_name	char(32) not null,
	 rule_dbp_owner	char(32) not null,
	 rule_time_date	date not null,
	 rule_time_int	date not null,
	 rule_col	byte(DB_COL_BYTES) not null,
	 rule_id1	integer not null,
	 rule_id2	integer not null,
	 rule_dbp_param	smallint not null,
	 rule_pad	smallint not null,
	 rule_free	char(8) not null);
*/

  struct rule_tbl_ {
	char	rule_name[33];
	char	rule_owner[33];
	i2	rule_type;
	i2	rule_flags;
	i4	rule_tabbase;
	i4	rule_tabidx;
	i4	rule_qryid1;
	i4	rule_qryid2;
	i4	rule_treeid1;
	i4	rule_treeid2;
	i4	rule_statement;
	char	rule_dbp_name[33];
	char	rule_dbp_owner[33];
	char	rule_time_date[26];
	char	rule_time_int[26];
	char	rule_col[DB_COL_BYTES];
	i4	rule_id1;
	i4	rule_id2;
	i2	rule_dbp_param;
	i2	rule_pad;
	char	rule_free[9];
  } rule_tbl;
