COPYFORM:	6.0	1989_12_28 20:48:39 GMT  
FORM:	abloktr	ABF Application Flow Lock Form.	
	64	17	0	0	2	0	5	8	0	0	0	0	0	64	0	5
FIELD:
	0	lockers	0	4	0	3	8	64	9	0	1	3	0		1	1	0	1073741857	0	0	0					1	0
	0	user	32	32	0	12	1	12	0	1	12	0	1	Active user	1	1	0	65536	64	0	0		c12			2	1
	1	frame	32	32	0	24	1	24	0	14	24	0	14	Frame or procedure	14	1	0	65536	64	0	0		c24			2	2
	2	since	3	12	0	24	1	24	0	39	24	0	39	Editing since	39	1	0	65536	0	0	0		d"Feb 03, 1901 04:05:06 PM"			2	3
	1	desc	32	48	0	48	1	48	0	0	48	0	0		0	0	0	65536	512	0	0		c48			0	4
TRIM:
	0	2	This component, or ones contained in the call tree beneath it,	0	0	0	0
	0	3	are being edited by other users.  You may continue with your	0	0	0	0
	0	4	operation, but you should be aware that you may disconnect	0	0	0	0
	0	5	these components from the rest of the application.	0	0	0	0
	0	7	Choose OK to continue operation, or Cancel.	0	0	0	0
