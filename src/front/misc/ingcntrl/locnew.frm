COPYFORM:	6.4	2001_07_11 17:48:05 GMT  
FORM:	locnew	ACCESSDB - create a new location	
	59	17	0	0	8	0	2	9	0	0	0	0	0	129	0	8
FIELD:
	0	locname	32	32	0	32	1	47	2	1	32	0	15	Location Name:	0	0	0	66576	0	0	0		c32			0	0
	1	area	32	128	0	150	3	56	4	1	50	0	6	Area:	0	0	0	66576	0	0	0		c128.50			0	1
	2	rawpct	30	4	0	3	1	11	8	1	3	0	8	Rawpct:	0	0	0	1024	0	0	0	0	-i3	Rawpct must be between 0 and 100 percent	rawpct >= 0 AND rawpct <= 100	0	2
	3	dbs	32	1	0	1	1	12	12	10	1	0	11	Databases:	0	0	0	66624	0	0	0		c1	'yes' or 'no' only.	dbs in ["y","yes", "ye", "n", "no"]	0	3
	4	jnls	32	1	0	1	1	11	13	11	1	0	10	Journals:	0	0	0	66624	0	0	0		c1	'yes' or 'no' only.	jnls in ["y","yes", "ye", "n", "no"]	0	4
	5	ckps	32	1	0	1	1	14	14	8	1	0	13	Checkpoints:	0	0	0	66624	0	0	0		c1	'yes' or 'no' only.	ckps in ["y","yes", "ye", "n", "no"]	0	5
	6	work	32	1	0	1	1	7	15	15	1	0	6	Work:	0	0	0	1088	0	0	0		c1	'yes' or 'no' only.	work in ["y","yes", "ye", "n", "no"]	0	6
	7	dmps	32	1	0	1	1	8	16	14	1	0	7	Dumps:	0	0	0	66624	0	0	0		c1	'yes' or 'no' only.	dmps in ["y","yes", "ye", "n", "no"]	0	7
TRIM:
	0	0	ACCESSDB - Create a Location	0	0	0	0
	1	10	Location Can Be Used For:	0	0	0	0
