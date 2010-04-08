COPYFORM:	6.0	1989_02_10 23:29:12 GMT
FORM:	abfconflict	ABF System Function Names Conflicts Display Form.	Used to display application procedure component names that conflict with system function names.
	80	22	0	0	1	0	4	7	0	0	0	0	0	0	0	2
FIELD:
	0	names	0	13	0	1	17	34	5	23	1	3	0		0	0	0	33	0	0	0					1	0
	0	procname	32	32	0	32	1	32	0	1	32	0	1	Conflicting Procedure Names	1	1	0	65536	0	0	0		c32			2	1
TRIM:
	0	0	WARNING:  This application contains procedures whose names conflict with system	0	0	0	0
	0	1	function names.  The application will still run normally.  But because of the	0	0	0	0
	0	2	conflicts, the corresponding system functions will be overridden and may not	0	0	0	0
	0	3	be called from 4GL in this application.  Refer to documentation for details.	0	0	0	0
