COPYFORM:	6.4	1991_03_18 19:04:34 GMT  
FORM:	rffile	RBF Popup that promptd for filename.	
	43	10	0	0	5	0	1	9	0	0	0	0	0	193	0	5
FIELD:
	0	fname	-21	259	0	25	1	36	5	6	25	0	11	File name:	0	0	0	1024	64	0	0		C25	Please enter the name of the file to conatin the report.  This is a mandatory field.	fname != ""	0	0
	1	batch	-21	6	0	3	1	16	7	4	3	0	13	to complete:	0	0	0	1088	0	0	0		c3	Please enter either "yes" or "no" to specify if you want to wait while the report executes.	 batch in ["y","ye","yes","n","no"]	0	1
	2	log_name	-21	259	0	25	1	37	9	5	25	0	12	Report Log:	0	0	0	16778240	64	0	0		C25			0	2
	3	batch1	-21	4	0	1	1	17	6	0	1	0	16	Wait for report	0	0	0	0	512	0	0		c1			0	3
	4	instructions	-21	89	0	86	2	43	2	0	43	0	0		0	0	0	0	512	0	0		cf86.43			0	4
TRIM:
	0	0	RBF - Sending a Report to a File	0	0	0	0
