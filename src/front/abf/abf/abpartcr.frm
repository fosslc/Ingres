COPYFORM:	6.4	1991_05_01 17:49:21 GMT  
FORM:	abfpartcreate	ABF Application Create Global Variable Form.	This frame allows a user to specify the name of the global before creating it.
	78	12	0	0	15	0	0	9	0	0	0	0	0	64	0	15
FIELD:
	0	title	32	50	0	50	1	50	0	0	50	0	0		40	0	0	131072	512	0	0	ABF - Edit a Global Variable	c50			0	0
	1	name	32	32	0	32	1	38	2	9	32	0	6	Name:	0	0	0	69720	0	0	0	unspecified	c32			0	1
	2	short_remark	37	62	0	60	1	74	4	1	60	0	14	Short Remark:	0	0	0	70656	0	0	0		c60			0	2
	3	data_type	32	32	0	20	1	26	6	9	20	0	6	Type:	0	0	0	70736	64	0	0		c20			0	3
	4	yn_field_title	32	8	0	8	1	9	6	35	8	0	1		0	0	0	131072	512	0	0		+c8			0	4
	5	nullable	21	5	0	3	1	5	6	44	3	0	2	:	0	0	0	70744	0	0	0	no	c3	Must be "Yes" or "No".	nullable in ["No", "Yes", "no", "yes","ye","y","n"]	0	5
	6	create_date	3	12	0	20	1	29	8	6	20	0	9	Created:	0	0	0	65536	512	0	0		c20			0	6
	7	owner	32	32	0	32	1	39	8	39	32	0	7	Owner:	0	0	0	65536	512	0	0		c32			0	7
	8	alter_date	3	12	0	20	1	35	10	0	20	0	15	Last Modified:	0	0	0	65536	512	0	0		c20			0	8
	9	long_remark	32	1	0	1	1	2	6	58	1	0	1		0	0	0	16842752	512	0	0		c1			0	9
	10	xfield1	32	1	0	1	1	2	6	62	1	0	1		0	0	0	16842752	512	0	0		c1			0	10
	11	xfield2	32	1	0	1	1	2	6	67	1	0	1		0	0	0	16842752	512	0	0		c1			0	11
	12	xfield3	32	1	0	1	1	2	6	70	1	0	1		0	0	0	16842752	512	0	0		c1			0	12
	13	xfield4	32	1	0	1	1	2	6	73	1	0	1		0	0	0	16842752	512	0	0		c1			0	13
	14	last_altered_by	32	32	0	32	1	36	10	42	32	0	4	By:	0	0	0	65536	512	0	0		c32			0	14
TRIM:
