COPYFORM:	6.4	1992_11_30 20:44:15 GMT  
FORM:	icinsacc	Accessdb's Installation Wide Access Privs screen.	
	52	17	0	0	1	0	1	9	0	0	0	0	0	193	0	4
FIELD:
	0	authid_access	0	10	0	3	14	48	2	2	1	3	0		1	1	0	33	0	0	0					1	0
	0	identifier	21	34	0	32	1	32	0	1	32	0	1	Authorization Id	1	1	0	65536	64	0	0		c32			2	1
	1	type	21	8	0	6	1	6	0	34	6	0	34	Type	34	1	0	65536	512	0	0		c6			2	2
	2	access	21	5	0	3	1	6	0	41	3	0	41	Access	41	1	0	65600	0	0	0		c3	'y' means GRANT ACCESS ON INSTALLATION; 'n' means GRANT NOACCESS ON INSTALLATION.	authid_access.access in ["y", "n"]	2	3
TRIM:
	0	0	Installation-Wide Access Grants	0	0	0	0
