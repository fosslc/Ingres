COPYFORM:	6.0	1989_06_10 23:15:00 GMT  
QBFNAME:	iialarm	QBFname for joindef 'iialarm' and form 'iialarm'	This is specifies that joindef 'iialarm' and form 'iialarm' will be used together.  This QBFname is used for data entry and modification for the SQL*SCHEDULER.
	iialarm	iialarm	1
JOINDEF:	iialarm		
	0	y			
	1	i_alarm_settings	0	ias	1
	2	ias	alarm_user	alarm_user	011
	3	ias	alarm_user	iaqt	alarm_user
	257	i_alarm_query_text	1	iaqt	1
	258	ias	alarm_id	alarm_id	011
	259	ias	alarm_id	iaqt	alarm_id
	514	ias	alarm_desc	alarm_desc	101
	770	ias	alarm_priority	alarm_priority	101
	1026	ias	last_time_run	last_time_run	101
	1282	ias	start_time	start_time	101
	1538	ias	interval_num	interval_num	101
	1794	ias	interval_units	interval_units	101
	2050	ias	next_time	next_time	101
	2306	ias	retry_enabled	retry_enabled	101
	2562	ias	retry_number	retry_number	101
	2818	ias	retry_interval_num	retry_interval_num	101
	3074	ias	retry_interval_units	retry_interval_units	101
	3330	ias	save_start_time	save_start_time	101
	3586	ias	save_interval_num	save_interval_num	101
	3842	ias	save_interval_units	save_interval_units	101
	4098	ias	alarm_enabled	alarm_enabled	101
	4354	iaqt	alarm_user	alarm_user	010
	4610	iaqt	alarm_id	alarm_id	010
	4866	iaqt	query_seq	query_seq	101
	5122	iaqt	failure_action	failure_action	101
	5378	iaqt	text_dml	text_dml	101
	5634	iaqt	text_sequence	text_sequence	101
	5890	iaqt	text_segment	text_segment	101
	6146	iaqt	sqlca_sqlcode	sqlca_sqlcode	101
	6402	iaqt	iialarmchk_error	iialarmchk_error	101
FORM:	iialarm	Help user's set alarms	
	1994	34	0	0	18	0	3	7	0	0	0	0	0	0	0	25
FIELD:
	0	alarm_id	21	34	0	32	1	42	2	4	32	0	10	Alarm Id:	0	0	0	276	0	0	0		c32			0	0
	1	alarm_enabled	-21	4	0	1	1	16	2	53	1	0	15	Alarm Enabled:	0	0	0	384	0	0	0	Y	c1	"Y" - enabled, "N" - disabled	alarm_enabled in ["Y","N"]	0	1
	2	alarm_desc	21	130	0	128	2	76	3	2	64	0	12	Alarm Desc:	0	0	0	256	0	0	0		c128.64			0	2
	3	alarm_user	21	34	0	32	1	38	5	12	32	0	6	User:	0	0	0	256	0	0	0		c32	This field must be set to your username	alarm_user in iidbconstants.user_name	0	3
	4	start_time	20	25	0	25	1	37	6	6	25	0	12	Start Time:	0	0	0	272	0	0	0	1989_01_01 00:00:00 GMT	c25			0	4
	5	interval_num	30	4	0	6	1	16	7	8	6	0	10	Interval:	0	0	0	272	0	0	0	1	-i6			0	5
	6	interval_units	20	8	0	8	1	9	7	24	8	0	1	-	0	0	0	272	0	0	0	day	c8			0	6
	7	retry_number	30	4	0	4	1	16	9	6	4	0	12	on Failure:	0	0	0	256	0	0	0	0	-i4			0	7
	8	retry_interval_num	30	4	0	6	1	22	10	2	6	0	16	Retry Interval:	0	0	0	256	0	0	0	10	-i6			0	8
	9	retry_interval_units	-21	11	0	8	1	9	10	24	8	0	1	-	0	0	0	256	0	0	0	minutes	c8	Must be valid 'date' interval specifier	retry_interval_units in ["second","seconds","minute","minutes","hour","hours","day","days","week","weeks","month","months","quarter","quarters","year","years"]	0	9
	10	alarm_priority	30	4	0	4	1	20	11	2	4	0	16	Alarm Priority:	0	0	0	256	0	0	0	0	-i4			0	10
	11	detailtbl	0	5	0	7	9	1985	14	0	1	3	0		0	0	0	33	0	0	0					1	11
	0	query_seq	30	4	0	4	1	6	0	1	4	0	1	QrySeq	1	1	0	272	0	0	0		-i4	Query Seq must start at 0, do not skip numbers	detailtbl.query_seq >= 0	2	12
	1	failure_action	20	1	0	1	1	8	0	8	1	0	8	StpOnErr	8	1	0	400	0	0	0		c1	'C' to continue with rest of alarm, 'S' to stop.	detailtbl.failure_action in ["C", "S"]	2	13
	2	text_dml	20	1	0	1	1	3	0	17	1	0	17	DML	17	1	0	400	0	0	0		c1	DML must be 'S' (SQL) or 'B' (Builtin)	detailtbl.text_dml in ["S", "B"]	2	14
	3	text_sequence	30	4	0	4	1	6	0	21	4	0	21	TxtSeq	21	1	0	272	0	0	0		-i4	0 to 5.  Use 1 to 5 if query text > 1970 chars	detailtbl.text_sequence >= 0 and detailtbl.text_sequence <= 5	2	15
	4	sqlca_sqlcode	30	4	0	7	1	7	0	28	7	0	28	Sqlcode	28	1	0	4	0	0	0		-i7			2	16
	5	iialarmchk_error	30	4	0	7	1	7	0	36	7	0	36	MiscErr	36	1	0	4	0	0	0		-i7			2	17
	6	text_segment	21	1942	0	1940	1	1940	0	44	1940	0	44	Query Text (only one query per line)	44	1	0	256	0	0	0		c1940			2	18
	12	retry_enabled	30	4	0	4	1	19	7	40	4	0	15	Retry Enabled:	0	0	0	4	0	0	0	0	-i4			0	19
	13	save_start_time	20	25	0	25	1	42	8	38	25	0	17	Orig Start Time:	0	0	0	4	0	0	0		c25			0	20
	14	save_interval_num	30	4	0	6	1	21	9	40	6	0	15	Orig Interval:	0	0	0	4	0	0	0	0	-i6			0	21
	15	save_interval_units	20	8	0	8	1	9	9	61	8	0	1	-	0	0	0	4	0	0	0		c8			0	22
	16	last_time_run	20	25	0	25	1	40	10	40	25	0	15	Last Time Run:	0	0	0	4	0	0	0		c25			0	23
	17	next_time	-20	251	0	250	1	261	11	44	250	0	11	Next Time:	0	0	0	4	0	0	0		c250			0	24
TRIM:
	0	8	Number of Retries	0	0	0	0
	0	13	QUERY TEXT (queries executed in Query Seq order - start at 0, do not skip #'s)	0	0	0	0
	1	0	SQL*SCHEDULER - Alarm Settings	0	0	0	0
