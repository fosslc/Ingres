/* Description of table iievent from database iidbdb */
  EXEC SQL DECLARE iievent TABLE
	(event_name	char(32) not null,
	 event_owner	char(32) not null,
	 event_create	date not null,
	 event_type	integer not null,
	 event_idbase	integer not null,
	 event_idx	integer not null,
	 event_qryid1	integer not null,
	 event_qryid2	integer not null,
	 event_free	char(8) not null);

  struct evnt_tbl_ {
	char	event_name[33];
	char	event_owner[33];
	char	event_create[26];
	i4	event_type;
	i4	event_idbase;
	i4	event_idx;
	i4	event_qryid1;
	i4	event_qryid2;
	char	event_free[9];
  } evnt_tbl;
