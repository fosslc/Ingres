COPYFORM:	6.4	1994_05_03 05:41:38 GMT  
FORM:	abfuserd	ABF User Frame Definition Form.	Defines the attributes for a User frame of an ABF application.
	80	17	0	0	14	0	0	9	0	0	0	0	0	0	0	14
FIELD:
	0	form	37	34	0	32	1	43	6	5	32	0	11	Form Name:	0	0	0	70744	0	0	0		c32	Must be a valid INGRES name.	form = "*"	0	0
	1	sourcefile	37	50	0	48	1	61	7	3	48	0	13	Source File:	0	0	0	70680	0	0	0		c48			0	1
	2	return_type	37	26	0	24	1	37	8	3	24	0	13	Return Type:	0	0	0	70680	0	0	0	none	c24			0	2
	3	nullable	32	3	0	3	1	13	9	6	3	0	10	Nullable:	0	0	0	70744	0	0	0	No	c3	Nullability must be "Yes" or "No".	nullable in ["No", "Yes", "no", "yes", "n", "y", "ye"]	0	3
	4	fstatic	37	5	0	3	1	11	10	8	3	0	8	Static:	0	0	0	70744	0	0	0	No	c3	Static must be either "Yes" or "No".	fstatic in ["No", "Yes", "no", "yes", "n", "y", "ye"]	0	4
	5	compform	21	5	0	3	1	22	10	45	3	0	19	Use compiled form:	0	0	0	16782424	0	0	0	yes	c3	Answer must be "yes" or "no".	compform in ["yes", "ye", "no", "y", "n"]	0	5
	6	name	32	32	0	32	1	44	2	4	32	0	12	Frame Name:	0	0	0	65536	512	0	0		c32			0	6
	7	title	32	80	0	80	1	80	0	0	80	0	0		40	0	0	131072	512	0	0	ABF - Edit a USER Frame Definition.	c80			0	7
	8	create_date	3	12	0	20	1	29	13	7	20	0	9	Created:	0	0	0	65536	512	0	0		c20			0	8
	9	owner	32	32	0	32	1	39	13	38	32	0	7	Owner:	0	0	0	65536	512	0	0		c32			0	9
	10	alter_date	3	12	0	20	1	35	15	1	20	0	15	Last Modified:	0	0	0	65536	512	0	0		c20			0	10
	11	last_altered_by	32	32	0	32	1	36	15	41	32	0	4	By:	0	0	0	65536	512	0	0		c32			0	11
	12	menustyle	21	17	0	15	1	27	11	52	15	0	12	Menu style:	0	0	0	16782424	0	0	0	tablefield	c15	Answer must be "tablefield" or "single-line".	menustyle in ["tablefield", "single-line"]	0	12
	13	short_remark	37	62	0	60	1	74	4	2	60	0	14	Short Remark:	0	0	0	70664	0	0	0		c60			0	13
TRIM:
