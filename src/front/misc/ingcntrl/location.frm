COPYFORM:	6.4	1993_08_24 23:22:45 GMT  
FORM:	nlocation	ACCESSDB's location viewing/changing form	T
	66	20	0	0	9	0	1	9	0	0	0	0	0	129	0	10
FIELD:
	0	title	21	62	0	60	1	60	0	0	60	0	0		0	0	0	0	512	0	0		c60			0	0
	1	locname	32	32	0	32	1	47	2	1	32	0	15	Location Name:	0	0	0	65536	512	0	0		c32			0	1
	2	dbases	0	8	0	1	12	34	4	30	1	3	0		0	0	0	1073742881	0	0	0					1	2
	0	dbname	32	32	0	32	1	32	0	1	32	3	1	Databases Using Location	1	1	0	65536	0	0	0		c32			2	3
	3	dbs	32	1	0	1	1	12	6	3	1	0	11	Databases:	0	0	0	65600	0	0	0	y	c1	Enter 'y' if Databases allowed, 'n' if not allowed	dbs in ["y","n"]	0	4
	4	jnls	32	1	0	1	1	11	7	4	1	0	10	Journals:	0	0	0	65600	0	0	0	n	  c1	Enter 'y' if Journals allowed, 'n' if not allowed.	jnls in ["y","n"]	0	5
	5	ckps	32	1	0	1	1	14	8	1	1	0	13	Checkpoints:	0	0	0	65600	0	0	0	n	c1	Enter 'y' if Checkpoints allowed, 'n' otherwise.	ckps in ["y","n"]	0	6
	6	work	32	1	0	1	1	7	9	8	1	0	6	Work:	0	0	0	65600	0	0	0	n	c1	Enter 'y' if work location, 'n' otherwise.	work in ["y","n"]	0	7
	7	dmps	32	1	0	1	1	8	10	7	1	0	7	Dumps:	0	0	0	65600	0	0	0	n	c1	Enter 'y' if Dumps allowed, 'n' otherwise.	dmps in ["n","y"]	0	8
	8	area	32	128	0	150	3	56	16	2	50	0	6	Area:	0	0	0	65552	0	0	0		c128.50			0	9
TRIM:
	1	4	Location Can Be Used For:	0	0	0	0
