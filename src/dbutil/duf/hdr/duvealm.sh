/*
**Copyright (c) 2004 Ingres Corporation
*/
/* Description of table iisecalarm from database iidbdb */
  EXEC SQL DECLARE iisecalarm TABLE
	(alarm_name	char(32) not null,
	 alarm_num	integer not null,
	 obj_id1	integer not null,
	 obj_id2	integer not null,
	 obj_name	char(32) not null,
	 obj_type	char(1) not null,
	 subj_type	integer1 not null,
	 subj_name	char(32) not null,
	 alarm_flags	smallint not null,
	 alarm_txtid1	integer not null,
	 alarm_txtid2	integer not null,
	 alarm_opctl	integer not null,
	 alarm_opset	integer not null,
	 event_id1	integer not null,
	 event_id2	integer not null,
	 event_text	char(256) not null,
	 alarm_id1	integer not null,
	 alarm_id2	integer not null,
	 alarm_reserve	char(32) not null);

/* long changed to i4 since long is 8 bytes on axp_osf */
  struct alarm_tbl_ {
	char	alarm_name[33];
	i4	alarm_num;
	i4	obj_id1;
	i4	obj_id2;
	char	obj_name[33];
	char	obj_type[2];
	short	subj_type;
	char	subj_name[33];
	short	alarm_flags;
	i4	alarm_txtid1;
	i4	alarm_txtid2;
	i4	alarm_opctl;
	i4	alarm_opset;
	i4	event_id1;
	i4	event_id2;
	char	event_text[257];
	i4	alarm_id1;
	i4	alarm_id2;
	char	alarm_reserve[33];
  } alarm_tbl;
