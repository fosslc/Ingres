COPYFORM:	6.4	1993_09_29 22:31:00 GMT  
FORM:	mqbftbls	QBF Join Definition Tables Form.	Form used to specify the name of the join definition, whether to use table field format, and the tables for a join definition along with the role each plays in the join (MASTER or detail) and the correlation name for each.
	80	22	0	0	3	0	5	9	0	0	0	0	0	64	0	7
FIELD:
	0	name	32	32	0	32	1	46	2	9	32	0	14	JoinDef Name:	0	0	0	65792	0	0	0		c32			0	0
	1	tables	0	6	0	4	10	64	8	8	1	3	0		0	0	0	1057	0	0	0					1	1
	0	role	32	7	0	7	1	7	0	1	7	0	1	Role	1	1	0	65536	0	0	0		c7	Must specify as "MASTER" (or blank) or as "detail".	tables.role = "" or tables.role ="[Mm]*" or tables.role = "[Dd]*"	2	2
	1	table	32	66	0	32	1	32	0	9	32	0	9	Table Name	9	1	0	65536	64	0	0		c32			2	3
	2	owner	32	66	0	8	1	8	0	42	8	0	42	Owner	42	1	0	65536	64	0	0		c8			2	4
	3	abbr	32	66	0	12	1	12	0	51	12	0	51	Abbreviation	51	1	0	65536	64	0	0		c12			2	5
	2	tablefield	32	3	0	3	1	30	19	25	3	0	27	Table Field Format? (y/n):	0	0	0	65664	0	0	0	YES	c3	Must answer yes, no or leave blank.	tablefield in ["YES","YE","Y","NO","N",""]	0	6
TRIM:
	0	0	QBF - JoinDef Definition Form	0	0	0	0
	9	4	For each table in the JoinDef, enter table name (with optional	0	0	0	0
	9	5	abbreviation for table name) below.  For Master/Detail JoinDefs	0	0	0	0
	9	6	enter Master or Detail under Role.  (Default is Master if blank.)	0	0	0	0
	14	21	Select the "Go" menu item to run the Join Definition.	0	0	0	0
