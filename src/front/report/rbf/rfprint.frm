COPYFORM:	6.4	1991_03_18 19:04:41 GMT  
FORM:	rfprint	RBF Popup that prompts for printer information	 
	44	11	0	0	6	0	1	9	0	0	0	0	0	193	0	6
FIELD:
	0	print_name	-21	35	0	25	1	34	5	8	25	0	9	Printer:	0	0	0	1024	64	0	0		c25	Please enter a valid printer name.  This field is mandatory.	print_name != ""	0	0
	1	copies	-30	5	0	3	1	11	6	9	3	0	8	Copies:	0	0	0	1024	0	0	0		-i3	Number of copies must be 1 or more.	copies > 0	0	1
	2	batch	-21	6	0	3	1	16	8	4	3	0	13	to complete:	0	0	0	1088	0	0	0		c3	Please enter either "yes" or "no" to specify if you want to wait while the report executes.	batch in ["y","ye","yes","n","no"]	0	2
	3	log_name	-21	259	0	25	1	37	10	5	25	0	12	Report Log:	0	0	0	16778240	64	0	0		c25			0	3
	4	batch1	-21	4	0	1	1	17	7	0	1	0	16	Wait for report	0	0	0	0	512	0	0		c1			0	4
	5	instructions	-21	91	0	88	2	44	2	0	44	0	0		0	0	0	0	512	0	0		cf88.44			0	5
TRIM:
	0	0	RBF - Sending a Report to a Printer	0	0	0	0
