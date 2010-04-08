COPYFORM:	6.4	1994_07_21 09:03:42 GMT  
FORM:	usrfrm	Accessdb's NEW form for updating User info	T
	80	23	0	0	17	0	1	9	0	0	0	0	0	0	0	20
FIELD:
	0	title	21	47	0	45	1	45	0	0	45	0	0		0	0	0	0	512	0	0		c45			0	0
	1	name	32	32	0	32	1	43	1	13	32	0	11	User Name:	0	0	0	65536	512	0	0		c32			0	1
	2	profile_name	32	32	0	32	1	50	2	6	32	0	18	Profile for User:	0	0	0	65536	0	0	0		c32			0	2
	3	default_group	21	34	0	32	1	47	3	9	32	0	15	Default Group:	0	0	0	65536	0	0	0		c32			0	3
	4	expire_date	3	12	0	32	1	45	4	11	32	0	13	Expire Date:	0	0	0	65536	0	0	0		c32	Expire date must be later than the current time	expire_date>="now" or expire_date=""	0	4
	5	createdb	32	1	0	1	1	18	6	18	1	0	17	Create Database:	0	0	0	65600	0	0	0	y	c1	'y' allows database creation, 'n' disallows it, 'r' allows on request.	createdb in ["y", "n", "r" ]	0	5
	6	operator	21	3	0	1	1	11	6	51	1	0	10	Operator:	0	0	0	65600	0	0	0	n	c1	'y' = Operator privileges; 'n' = no Operator privileges, 'r' = Operator privilege on request.	operator in ["n","y","r"]	0	6
	7	security	21	3	0	1	1	25	7	11	1	0	24	Security Administrator:	0	0	0	65600	0	0	0	n	c1	'y' makes user a Security Administrator; 'n' makes them a non-Security Administrator	security in ["y", "n", "r"]	0	7
	8	trace	21	3	0	1	1	18	7	44	1	0	17	Set Trace Flags:	0	0	0	65600	0	0	0	n	c1	'y' allows Set Trace permission, 'n' disallows it.	trace in ["y", "n", "r"]	0	8
	9	system_admin	21	3	0	1	1	23	8	13	1	0	22	  Maintain Locations:	0	0	0	65600	0	0	0	n	c1	'y' permits a user to maintain locations; 'n' prevents it.	system_admin in ["y", "n", "r"]	0	9
	10	maintain_users	21	3	0	1	1	17	8	45	1	0	16	Maintain Users:	0	0	0	65600	0	0	0	n	c1	'y' permits a user to maintain users; 'n' prevents it.	maintain_users in ["y", "n", "r"]	0	10
	11	maintain_audit	21	3	0	1	1	26	9	10	1	0	25	Maintain Security Audit:	0	0	0	65600	0	0	0	n	c1	'y' permits a user to maintain audit, 'n' prevents it.	maintain_audit in ["y", "n", "r"]	0	11
	12	auditor	21	3	0	1	1	10	9	52	1	0	9	Auditor:	0	0	0	65600	0	0	0	n	c1	'y' permits a user to maintain audit, 'n' prevents it.	maintain_audit in ["y", "n", "r"]	0	12
	13	audit_query_text	21	3	0	1	1	19	10	17	1	0	18	Audit Query Text:	0	0	0	65600	0	0	0	n	c1	'y' specifies that the user has query auditing. 'n' prevents.	audit_query_text in ["y", "n"]	0	13
	14	audit_all	21	3	0	1	1	21	10	41	1	0	20	Audit All Activity:	0	0	0	65600	0	0	0	n	c1	'y' turns on auditing of all events for this user; 'n' turns it off.	audit_all in ["y", "n"]	0	14
	15	owns	0	7	0	1	11	34	11	2	1	3	0		0	0	0	1073742881	0	0	0					1	15
	0	dbname	32	32	0	32	1	32	0	1	32	3	1	Databases Owned	1	1	0	65536	512	0	0		c32			2	16
	16	access	0	7	0	2	11	41	11	37	1	3	0		0	0	0	1073742881	0	0	0					1	17
	0	dbname	32	32	0	32	1	32	0	1	32	3	1	Authorized Databases	1	1	0	65536	0	0	0		c32			2	18
	1	access	21	5	0	3	1	6	0	34	3	3	34	Access	34	1	0	16842816	0	0	0		c3			2	19
TRIM:
	0	6	Permissions:	0	0	0	0
