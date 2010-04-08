COPYFORM:	6.4	1991_09_03 23:27:13 GMT  
FORM:	dut_f1_intro	Introductory form for STARVIEW, DDB Admin Tool.	This form assist an INGRES/STAR user to select a distributed database
	80	23	0	0	4	0	3	9	0	0	0	0	0	0	0	6
FIELD:
	0	dut_f1_1ddbname	-21	35	0	32	1	54	2	2	32	0	22	Distributed Database:	0	0	0	320	0	0	0		c32			0	0
	1	dut_f1_2nodename	-21	35	0	32	1	43	4	2	32	0	11	Node Name:	0	0	0	320	0	0	0		c32			0	1
	2	dut_f1_4ddblist	0	8	0	2	12	67	10	2	1	3	0		0	0	0	1073741857	0	0	0					1	2
	0	dut_t1_1ddbname	-21	35	0	32	1	32	0	1	32	3	1	Distributed Database Name	1	1	0	64	512	0	0		c32			2	3
	1	dut_t1_2ddbowner	-21	35	0	32	1	32	0	34	32	3	34	Owner Name	34	1	0	64	512	0	0		c32			2	4
	3	dut_f1_3nodename	-21	35	0	32	1	62	9	2	32	0	30	Distributed databases on node	0	0	0	1088	512	0	0		c32			0	5
TRIM:
	0	0	StarView - Distributed Database Administration Tool	0	0	0	0
	4	6	"DDBHelp" will list all distributed databases on node specified above,	0	0	0	0
	13	7	default is current node.	0	0	0	0
