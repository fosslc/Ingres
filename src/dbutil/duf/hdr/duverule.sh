/* Description of table iirule from database iidbdb */
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
	 rule_col1	integer not null,
	 rule_col2	integer not null,
	 rule_col3	integer not null,
	 rule_col4	integer not null,
	 rule_col5	integer not null,
	 rule_col6	integer not null,
	 rule_col7	integer not null,
	 rule_col8	integer not null,
	 rule_col9	integer not null,
	 rule_cola	integer not null,
	 rule_col11	integer not null,
	 rule_col12	integer not null,
	 rule_col13	integer not null,
	 rule_col14	integer not null,
	 rule_col15	integer not null,
	 rule_col16	integer not null,
	 rule_col17	integer not null,
	 rule_col18	integer not null,
	 rule_col19	integer not null,
	 rule_col20	integer not null,
	 rule_col21	integer not null,
	 rule_col22	integer not null,
	 rule_col23	integer not null,
	 rule_col24	integer not null,
	 rule_col25	integer not null,
	 rule_col26	integer not null,
	 rule_col27	integer not null,
	 rule_col28	integer not null,
	 rule_col29	integer not null,
	 rule_col30	integer not null,
	 rule_col31	integer not null,
	 rule_col32	integer not null,
	 rule_col33	integer not null,
	 rule_id1	integer not null,
	 rule_id2	integer not null,
	 rule_dbp_param	smallint not null,
	 rule_pad	smallint not null,
	 rule_free	char(8) not null);

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
	i4	rule_col1;
	i4	rule_col2;
	i4	rule_col3;
	i4	rule_col4;
	i4	rule_col5;
	i4	rule_col6;
	i4	rule_col7;
	i4	rule_col8;
	i4	rule_col9;
	i4	rule_cola;
	i4	rule_col11;
	i4	rule_col12;
	i4	rule_col13;
	i4	rule_col14;
	i4	rule_col15;
	i4	rule_col16;
	i4	rule_col17;
	i4	rule_col18;
	i4	rule_col19;
	i4	rule_col20;
	i4	rule_col21;
	i4	rule_col22;
	i4	rule_col23;
	i4	rule_col24;
	i4	rule_col25;
	i4	rule_col26;
	i4	rule_col27;
	i4	rule_col28;
	i4	rule_col29;
	i4	rule_col30;
	i4	rule_col31;
	i4	rule_col32;
	i4	rule_col33;
	i4	rule_id1;
	i4	rule_id2;
	i2	rule_dbp_param;
	i2	rule_pad;
	char	rule_free[9];
  } rule_tbl;
