COPYFORM:	6.4	1992_12_08 20:00:05 GMT  
FORM:	extenddb	Accessdb's form for EXTENDDB	T
	80	23	0	0	3	0	2	9	0	0	0	0	0	64	0	9
FIELD:
	0	extended	0	5	0	3	9	75	3	2	1	3	0		0	0	0	1057	0	0	0					1	0
	0	dbname	32	32	0	28	1	28	0	1	28	0	1	       Database Name	1	1	0	65536	576	0	0		c28			2	1
	1	locname	32	32	0	28	1	28	0	30	28	0	30	   Current Location Name	30	1	0	65536	576	0	0		c28			2	2
	2	usage	-32	16	0	15	1	15	0	59	15	0	59	     Usage	59	1	0	0	512	0	0		c15			2	3
	1	newextensions	0	5	0	3	9	69	14	2	1	3	0		0	0	0	1073741857	0	0	0					1	4
	0	dbname	32	32	0	32	1	32	0	1	32	0	1	         Database Name	1	1	0	65536	0	0	0		c32			2	5
	1	locname	-32	33	0	32	1	32	0	34	32	0	34	       New Location Name	34	1	0	65536	0	0	0		c32	Location name does not exist.	newextensions.locname in iilocations.lname	2	6
	2	usage_flag	-30	5	0	1	1	1	0	67	1	0	67		67	1	0	16777216	0	0	0		-i1			2	7
	2	dbaname	32	32	0	32	1	74	2	2	32	0	42	Existing Databases and Locations for DBA:	0	0	0	65536	512	0	0		c32			0	8
TRIM:
	0	0	ACCESSDB - Extend Databases to Alternate Locations	0	0	0	0
	2	13	Enter New Locations for Databases:	0	0	0	0
