COPYFORM:	6.4	1990_09_19 00:36:59 GMT  
FORM:	abfreportd	ABF Report Writer Frame Definition Form.	Defines the attributes for a Report frame of an ABF application.
	80	20	0	0	14	0	0	9	0	0	0	0	0	0	0	14
FIELD:
	0	library	32	1	0	1	1	20	6	7	1	0	19	RBF Report? (y/n):	0	0	0	66624	0	0	0	y	c1	Answer must be 'y' or 'n'.	library in ["y","n"]	0	0
	1	report	37	34	0	32	1	45	8	13	32	0	13	Report Name:	0	0	0	70744	0	0	0	unspecified	c32	Must be a valid INGRES name.	report = "*"	0	1
	2	sourcefile	32	48	0	48	1	68	9	6	48	0	20	Report Source File:	0	0	0	70680	0	0	0		c48			0	2
	3	form	37	34	0	32	1	56	10	2	32	0	24	Report Parameters Form:	0	0	0	70728	0	0	0		c32	Must be a valid INGRES name.	form = "" or form = "*"	0	3
	4	compform	21	5	0	3	1	22	11	7	3	0	19	Use Compiled Form:	0	0	0	16782424	0	0	0	yes	c3	Answer must be "yes" or "no".	compform in ["yes", "ye", "no", "y", "n"]	0	4
	5	output	32	48	0	48	1	61	12	13	48	0	13	Output File:	0	0	0	70664	0	0	0		c48	Must be "printer", "terminal", or a file name.	output = "" or output in ["printer", "terminal"] or output = "*"	0	5
	6	comline	32	48	0	48	1	68	13	6	48	0	20	Command Line Flags:	0	0	0	70664	0	0	0		c48			0	6
	7	short_remark	37	62	0	60	1	74	4	2	60	0	14	Short Remark:	0	0	0	66568	0	0	0		c60			0	7
	8	title	32	80	0	80	1	80	0	0	80	0	0		40	0	0	131072	512	0	0	ABF - Edit a REPORT Frame Definition	c80			0	8
	9	create_date	3	12	0	20	1	29	16	7	20	0	9	Created:	0	0	0	65536	512	0	0		c20			0	9
	10	alter_date	3	12	0	20	1	35	18	1	20	0	15	Last Modified:	0	0	0	65536	512	0	0		c20			0	10
	11	owner	32	32	0	32	1	39	16	38	32	0	7	Owner:	0	0	0	65536	512	0	0		c32			0	11
	12	last_altered_by	32	32	0	32	1	36	18	41	32	0	4	By:	0	0	0	65536	512	0	0		c32			0	12
	13	name	32	32	0	32	1	44	2	4	32	0	12	Frame Name:	0	0	0	65536	512	0	0		c32			0	13
TRIM:
