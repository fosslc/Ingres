COPYFORM:	6.0	1990_01_04 01:54:40 GMT  
FORM:	rbfabf	Special "create" form for when rbf called from abf	Form seen when rbf is called from abf and the report named does not exist.
	80	23	0	0	2	0	8	8	0	0	0	0	0	0	0	4
FIELD:
	0	rep_name	-21	35	0	32	1	32	3	19	32	0	0		0	0	0	0	512	0	0		c32			0	0
	1	rep_src	0	3	0	2	5	60	8	0	1	1	0		1	1	0	1073758241	0	0	0					1	1
	0	source	-21	13	0	10	1	10	0	1	10	0	1	source	1	-1	0	0	0	0	0		c10			2	2
	1	expl	-21	50	0	47	1	47	0	12	47	0	12	expl	12	-1	0	0	0	0	0		c47			2	3
TRIM:
	0	0	9:1:1	1	0	0	0
	0	0	1:60:0	1	0	0	0
	1	1	RBF - Creating a Report	0	0	0	0
	1	3	Requested Report:	0	0	0	0
	1	5	The report does not exist.  You can create it by choosing	0	0	0	0
	1	6	one of the selections below.  If you do not want to create	0	0	0	0
	1	7	a new report of this name then "CANCEL" this operation.	0	0	0	0
	59	0	9:1:2	1	0	0	0
