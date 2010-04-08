COPYFORM:	6.4	1998_01_23 20:48:47 GMT  
FORM:	logpassedit	Login/Password edit form	The old logpass form is modified to provide the user capability            to change the type of login.
	49	14	0	0	8	0	1	9	0	0	0	0	0	193	0	8
FIELD:
	0	infoline	-21	27	0	24	1	24	1	12	24	0	0		0	0	0	0	512	0	0		c24			0	0
	1	f_node	-21	20	0	17	1	36	3	2	17	0	19	Virtual Node Name:	0	0	0	0	512	0	0		c17			0	1
	2	f_login	21	66	0	17	1	24	7	14	17	0	7	Login:	0	0	0	1024	64	0	0		c17			0	2
	3	f_password	21	19	0	17	1	27	8	11	17	0	10	Password:	0	0	0	9216	0	0	0		c17			0	3
	4	f_password_conf	21	19	0	17	1	36	9	2	17	0	19	Re-enter Password:	0	0	0	9216	0	0	0		c17			0	4
	5	f_login_type	-21	10	0	7	1	23	5	5	7	0	16	Private/Global:	0	0	0	1024	512	0	0		c7			0	5
	6	prompt_line	-21	51	0	48	1	48	11	1	48	0	0		0	0	0	2048	512	0	0		c48			0	6
	7	noecho_reminder	-21	51	0	48	1	48	12	1	48	0	0		0	0	0	0	512	0	0		c48			0	7
TRIM:
	2	1	NETUTIL -	0	0	0	0
