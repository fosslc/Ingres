COPYFORM:	6.4	2001_05_15 17:59:27 GMT  
FORM:	nlocation	ACCESSDB's location viewing/changing form	T
	66	20	0	0	12	0	1	9	0	0	0	0	0	129	0	13
FIELD:
	0	title	21	62	0	60	1	60	0	0	60	0	0		0	0	0	0	512	0	0		c60			0	0
	1	locname	32	32	0	32	1	47	2	1	32	0	15	Location Name:	0	0	0	65536	512	0	0		c32			0	1
	2	dbases	0	8	0	1	12	34	4	30	1	3	0		0	0	0	1073742881	0	0	0					1	2
	0	dbname	32	32	0	32	1	32	0	1	32	3	1	Databases Using Location	1	1	0	65536	0	0	0		c32			2	3
	3	dbs	32	3	0	3	1	14	6	3	3	0	11	Databases:	0	0	0	65600	0	0	0		c3	'yes' or 'no' only.	dbs in ["y","yes", "ye", "n", "no"]	0	4
	4	jnls	32	3	0	3	1	13	7	4	3	0	10	Journals:	0	0	0	65600	0	0	0		c3	'yes' or 'no' only.	jnls in ["y","yes", "ye", "n", "no"]	0	5
	5	ckps	32	3	0	3	1	16	8	1	3	0	13	Checkpoints:	0	0	0	65600	0	0	0		c3	'yes' or 'no' only.	ckps in ["y","yes", "ye", "n", "no"]	0	6
	6	work	32	3	0	3	1	9	9	8	3	0	6	Work:	0	0	0	65600	0	0	0		c3	'yes' or 'no' only.	work in ["y","yes", "ye", "n", "no"]	0	7
	7	dmps	32	3	0	3	1	10	10	7	3	0	7	Dumps:	0	0	0	65600	0	0	0		c3	'yes' or 'no' only.	dmps in ["y","yes", "ye", "n", "no"]	0	8
	8	rawpct	-30	5	0	3	1	11	12	6	3	0	8	Rawpct:	0	0	0	0	0	0	0	0	-i3	Rawpct must be between 0 and 100 percent	rawpct >= 0 AND rawpct <= 100	0	9
	9	rawstart	-30	5	0	8	1	18	13	4	8	0	10	Rawstart:	0	0	0	0	512	0	0		-i8			0	10
	10	rawblocks	-30	5	0	8	1	19	14	3	8	0	11	Rawblocks:	0	0	0	0	512	0	0		-i8			0	11
	11	area	32	128	0	150	3	56	16	2	50	0	6	Area:	0	0	0	65552	0	0	0		c128.50			0	12
TRIM:
	1	4	Location Can Be Used For:	0	0	0	0
