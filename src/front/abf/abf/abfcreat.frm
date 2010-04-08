COPYFORM:	6.4	1991_05_01 17:48:51 GMT  
FORM:	abfcreate	ABF Application Create Form.	Display application detail information when it is being created.  Fields that may be set include the name, application source directory, and the application (DML) language.
	80	23	0	0	14	0	1	9	0	0	0	0	0	0	0	14
FIELD:
	0	xfield1	-21	4	0	1	1	2	5	1	1	0	1		0	0	0	0	512	0	0		c1			0	0
	1	xfield2	-21	4	0	1	1	2	5	4	1	0	1		0	0	0	0	512	0	0		c1			0	1
	2	xfield3	-21	4	0	1	1	2	5	8	1	0	1		0	0	0	0	512	0	0		c1			0	2
	3	xfield4	-21	4	0	1	1	2	5	12	1	0	1		0	0	0	0	512	0	0		c1			0	3
	4	title	32	79	0	79	1	79	0	0	79	0	0		31	0	0	131072	512	0	0		c79			0	4
	5	create_date	3	12	0	20	1	29	2	48	20	0	9	Created:	0	0	0	65536	512	0	0	"now"	c20			0	5
	6	alter_date	3	12	0	20	1	30	4	47	20	0	10	Modified:	0	0	0	65536	512	0	0		c20			0	6
	7	owner	37	34	0	32	1	39	4	1	32	0	7	Owner:	0	0	0	65600	512	0	0		c32			0	7
	8	name	37	34	0	32	1	38	2	2	32	0	6	Name:	0	0	0	66640	0	0	0	unspecified	c32			0	8
	9	short_remark	37	62	0	60	1	74	6	1	60	0	14	Short Remark:	0	0	0	66560	0	0	0		c60			0	9
	10	dml	37	6	0	4	1	14	7	5	4	0	10	Language:	0	0	0	65672	0	0	0	SQL	c4	Unknown data manipulation language.	dml in ["QUEL", "SQL"]	0	10
	11	menustyle	21	17	0	15	1	42	7	30	15	0	27	Style for new Menu Frames:	0	0	0	1088	0	0	0	tablefield	c15	Answer must be "tablefield" or "single-line".	menustyle in ["tablefield", "single-line"]	0	11
	12	srcdir	37	182	0	180	3	78	9	1	60	0	18	Source Directory:	0	0	0	66568	0	0	0		c180.60			0	12
	13	long_remark	37	602	0	600	10	77	13	1	75	1	1		1	1	0	65537	0	0	0		c600.75			0	13
TRIM:
	2	12	Long Remark:	0	0	0	0
