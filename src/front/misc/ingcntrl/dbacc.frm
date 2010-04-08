COPYFORM:	6.4	1993_08_24 18:29:43 GMT  
FORM:	dbaccess	ACCESSDB - change database access	
	52	20	0	0	6	0	0	9	0	0	0	0	0	0	0	9
FIELD:
	0	dbname	32	32	0	32	1	42	2	7	32	0	10	Database:	0	0	0	65536	512	0	0		c32			0	0
	1	ownname	32	32	0	32	1	39	3	10	32	0	7	Owner:	0	0	0	65536	512	0	0		c32			0	1
	2	title	21	47	0	45	1	45	0	0	45	0	0		0	0	0	0	512	0	0		c45			0	2
	3	access_table	0	9	0	3	13	48	7	2	1	3	0		0	0	0	1057	0	0	0					1	3
	0	uname	32	32	0	32	1	32	0	1	32	0	1	User Name	1	1	0	65536	0	0	0		c32			2	4
	1	type	21	8	0	6	1	6	0	34	6	0	34	Type	34	1	0	16842752	0	0	0		c6			2	5
	2	access	21	5	0	3	1	6	0	41	3	0	41	Access	41	1	0	65600	0	0	0		c3			2	6
	4	defaultaccess	21	11	0	9	1	25	5	1	9	0	16	Default Access:	0	0	0	66560	0	0	0	public	c9	Default access must be "public" or "private"	defaultaccess in ["public", "private"]	0	7
	5	orig_def_access	21	11	0	9	1	9	5	28	9	0	0		0	0	0	16777216	512	0	0		c9			0	8
TRIM:
