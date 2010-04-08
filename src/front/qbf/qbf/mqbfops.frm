COPYFORM:	6.4	1993_10_07 06:15:45 GMT  
FORM:	mqbfops	JoinDef Update & Delete Rules form	
	80	23	0	0	4	0	5	9	0	0	0	0	0	64	0	10
FIELD:
	0	mqdops	0	4	0	4	8	59	15	11	1	3	0		0	0	0	1057	0	0	0					1	0
	0	role	32	7	0	7	1	7	0	1	7	0	1	Role	1	1	0	65536	512	0	0		c7			2	1
	1	table	32	133	0	32	1	32	0	9	32	0	9	Table Name (or Abbreviation)	9	1	0	65536	576	0	0		c32			2	2
	2	owner	-21	69	0	8	1	8	0	42	8	0	42	Owner	42	1	0	0	576	0	0		c8			2	3
	3	del	32	4	0	4	1	7	0	51	4	0	51	Delete?	51	1	0	65600	0	0	0		c4	Delete semantics must be "yes" or "no".	mqdops.del in ["yes", "ye","y","no","n"]	2	4
	1	mquops	0	4	0	2	8	54	4	13	1	3	0		0	0	0	1057	0	0	0					1	5
	0	col	32	133	0	44	1	44	0	1	44	0	1	Column	1	1	0	65536	576	0	0		c44			2	6
	1	upd	32	4	0	4	1	7	0	46	4	0	46	Update?	46	1	0	65536	0	0	0		c4	Update semantics must be "yes" or "no".	mquops.upd in ["yes", "ye","y", "no","n"]	2	7
	2	uinfo	32	19	0	19	1	21	2	4	19	0	2		0	0	0	1024	512	0	0	Update Information:	c19			0	8
	3	dinfo	32	19	0	19	1	21	13	4	19	0	2		0	0	0	1024	512	0	0	Delete Information:	c19			0	9
TRIM:
	0	0	QBF - JoinDef Update & Delete Rules	0	0	0	0
	27	2	To enable modification of join fields in	0	0	0	0
	27	3	UPDATE mode, enter "Yes" under Update? column.	0	0	0	0
	27	13	To disable deletion of rows in a table during	0	0	0	0
	27	14	UPDATE mode, enter "No" under Delete? column.	0	0	0	0
