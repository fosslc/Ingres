COPYFORM:	6.4	1992_12_04 19:45:19 GMT  
FORM:	abfprocd	ABF Procedure Component Definition Form.	Defines the attributes for a GRAPH frame of an ABF application.
	80	21	0	0	15	0	0	9	0	0	0	0	0	0	0	15
FIELD:
	0	title	32	80	0	80	1	80	0	0	80	0	0		42	0	0	131072	512	0	0	ABF - Edit a 4GL Procedure Definition.	c80			0	0
	1	name	32	32	0	32	1	48	2	2	32	0	16	Procedure Name:	0	0	0	65536	512	0	0		c32			0	1
	2	language	32	8	0	8	1	25	6	1	8	0	17	Source Language:	0	0	0	65536	512	0	0	4GL/SQL	c8			0	2
	3	symbol	37	34	0	32	1	40	7	10	32	0	8	Symbol:	0	0	0	16847880	0	0	0		c32			0	3
	4	library	37	5	0	3	1	12	7	52	3	0	9	Library:	0	0	0	16846936	0	0	0	no	c3	Enter "yes" for a Library Procedure, else "no".	library in ["yes", "ye", "no", "y", "n"]	0	4
	5	sourcefile	37	50	0	48	1	61	8	5	48	0	13	Source File:	0	0	0	70680	0	0	0		c48			0	5
	6	return_type	37	34	0	32	1	45	9	5	32	0	13	Return Type:	0	0	0	70744	0	0	0	integer	c32			0	6
	7	nullable	37	5	0	3	1	13	10	8	3	0	10	Nullable:	0	0	0	69720	0	0	0	no	c3	Nullability must be "yes" or "no".	nullable in ["yes", "no", "n", "y", "ye"]	0	7
	8	pass_dec	-21	10	0	7	1	36	12	16	7	0	29	Decimal arguments passed as:	0	0	0	1024	0	0	0		c7			0	8
	9	short_remark	37	62	0	60	1	74	4	1	60	0	14	Short Remark:	0	0	0	70664	0	0	0		c60			0	9
	10	create_date	3	12	0	20	1	29	14	7	20	0	9	Created:	0	0	0	65536	512	0	0		c20			0	10
	11	owner	32	32	0	32	1	39	14	38	32	0	7	Owner:	0	0	0	65536	512	0	0		c32			0	11
	12	alter_date	3	12	0	20	1	35	16	1	20	0	15	Last Modified:	0	0	0	65536	512	0	0		c20			0	12
	13	last_altered_by	32	32	0	32	1	36	16	41	32	0	4	By:	0	0	0	65536	512	0	0		c32			0	13
	14	warning	32	50	0	50	3	62	18	9	50	1	11	Warning:	1	1	0	17039617	512	0	0	Procedure name conflicts with a system function.	c50			0	14
TRIM:
