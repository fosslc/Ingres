COPYFORM:	6.4	1991_05_01 17:49:07 GMT  
FORM:	abfqbfd	ABF Query-By-Forms Frame Definition Form.	Defines the attributes for a QBF frame of an ABF application.
	77	18	0	0	11	0	1	9	0	0	0	0	0	0	0	11
FIELD:
	0	qtype	32	7	0	7	1	26	6	3	7	0	19	Query Object Type:	0	0	0	70744	0	0	0	Table	c7	Must be "Table" or "JoinDef".	qtype in ["Table", "table", "JoinDef", "joindef"]	0	0
	1	joindef	37	34	0	32	1	51	9	3	32	0	19	Query Object Name:	0	0	0	70680	0	0	0	unspecified	c32			0	1
	2	form	37	34	0	32	1	43	10	11	32	0	11	Form Name:	0	0	0	70728	0	0	0	unspecified	c32			0	2
	3	comline	32	48	0	48	1	68	11	2	48	0	20	Command Line Flags:	0	0	0	70664	0	0	0		c48			0	3
	4	short_remark	37	62	0	60	1	74	4	1	60	0	14	Short Remark:	0	0	0	66568	0	0	0		c60			0	4
	5	name	32	32	0	32	1	44	2	3	32	0	12	Frame Name:	0	0	0	65536	512	0	0		c32			0	5
	6	title	32	40	0	40	1	41	0	0	40	0	0		41	0	0	131072	512	0	0	ABF - Edit a QBF Frame Definition	c40			0	6
	7	alter_date	3	12	0	20	1	35	16	0	20	0	15	Last Modified:	0	0	0	65536	512	0	0		c20			0	7
	8	create_date	3	12	0	20	1	29	14	6	20	0	9	Created:	0	0	0	65536	512	0	0		c20			0	8
	9	owner	32	32	0	32	1	39	14	37	32	0	7	Owner:	0	0	0	65536	512	0	0		c32			0	9
	10	last_altered_by	32	32	0	32	1	36	16	40	32	0	4	By:	0	0	0	65536	512	0	0		c32			0	10
TRIM:
	2	7	(Table or JoinDef)	0	0	0	0
