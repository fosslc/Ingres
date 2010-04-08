COPYFORM:	6.4	1992_11_11 00:30:26 GMT  
FORM:	rfiidetail	Report Detail Form.	Detail form for reports, identical to ii_detail except for the addition of some fields specific to reports (such as whether the report is edittable by RBF, etc.)
	86	22	0	0	11	0	1	9	0	0	0	0	0	64	0	11
FIELD:
	0	short_remark	32	60	0	60	1	74	6	1	60	0	14	Short Remark:	0	0	0	66560	0	0	0		c60			0	0
	1	long_remark	37	602	0	600	10	77	9	1	75	1	1		1	1	0	65537	0	0	0		c600.75			0	1
	2	title	32	79	0	79	1	80	0	0	79	0	0		80	0	0	131072	512	0	0		c79			0	2
	3	name	32	32	0	32	1	39	2	1	32	0	7	Name:	0	0	0	65536	512	0	0		c32			0	3
	4	create_date	3	12	0	25	1	35	2	42	25	0	10	Created:	0	0	0	65536	512	0	0		c25			0	4
	5	owner	32	32	0	32	1	39	4	1	32	0	7	Owner:	0	0	0	65536	512	0	0		c32			0	5
	6	alter_date	3	12	0	25	1	35	4	42	25	0	10	Modified:	0	0	0	65536	512	0	0		c25			0	6
	7	xfield1	32	3	0	3	1	17	20	1	3	0	14	RBF Editable?	0	0	0	65536	512	0	0		c3			0	7
	8	xfield2	32	133	0	24	1	36	20	33	24	0	12	Data Table:	0	0	0	65536	576	0	0		c24			0	8
	9	xfield3	32	15	0	15	1	15	21	33	15	0	0		0	0	0	0	512	0	0		c15			0	9
	10	xfield4	32	4	0	4	1	6	21	50	4	0	0		6	0	0	65536	512	0	0		c4			0	10
TRIM:
	1	8	Long Remark:	0	0	0	0
