COPYFORM:	6.0	1989_07_27 22:10:17 GMT  
FORM:	abfobjects	ABF Application Component Objects Catalog Form.	Contains a table field used to list the component objects that define an application.  Also displayed are the name of the application, the DML query language for the application, and any default start frame or procedure.
	80	23	0	0	6	0	1	7	0	0	0	0	0	64	0	9
FIELD:
	0	frames@procs	0	12	0	3	16	80	4	0	1	3	0		0	0	0	1073741857	0	0	0					1	0
	0	name	32	32	0	24	1	24	0	1	24	0	1	Frame/Procedure Name	1	1	0	65536	64	0	0		c24			2	1
	1	type	32	24	0	13	1	13	0	26	13	0	26	Type	26	1	0	69632	64	0	0		c13			2	2
	2	short_remark	32	60	0	39	1	39	0	40	39	0	40	Short Remark	40	1	0	65536	64	0	0		c39			2	3
	1	mode	32	11	0	11	1	17	3	2	11	0	6	Mode:	0	0	0	16842752	512	0	0	Interactive	c11			0	4
	2	dml	32	4	0	4	1	20	3	40	4	0	16	Query Language:	0	0	0	16842752	512	0	0	SQL	c4			0	5
	3	title	32	40	0	40	1	79	0	0	40	0	0		79	0	0	131072	512	0	0		c40			0	6
	4	name	32	32	0	32	1	38	2	2	32	0	6	Name:	0	0	0	65536	512	0	0		c32			0	7
	5	defaultstart	32	24	0	24	1	39	2	41	24	0	15	Default Start:	0	0	0	65536	512	0	0		c24			0	8
TRIM:
	11	21	Place cursor on row and select desired operation from menu.	0	0	0	0
