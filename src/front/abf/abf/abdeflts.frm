COPYFORM:	6.4	1994_05_03 05:37:01 GMT  
FORM:	abfdefaults	ABF Application Defaults Form.	ABF application defaults form contains fields for the source file directory, link options file, etc.
	73	21	0	0	10	0	1	9	0	0	0	0	0	129	0	10
FIELD:
	0	srcdir	32	180	0	180	4	60	4	7	60	1	0	Source-Code Directory:	0	0	0	66584	0	0	0		c180.60			0	0
	1	linkopt	32	48	0	48	1	71	9	1	48	0	23	Link-options filename:	0	0	0	66568	0	0	0		c48			0	1
	2	image	32	48	0	48	1	68	10	4	48	0	20	Default image name:	0	0	0	66568	0	0	0		c48			0	2
	3	startframe	32	32	0	32	1	53	12	5	32	0	21	Default start frame:	0	0	0	66632	0	0	0		c32	Only one of the start fields can have a value.	startframe = "" or (startframe = "*" and startproc = "")	0	3
	4	startproc	32	32	0	32	1	57	13	1	32	0	25	Default start procedure:	0	0	0	66632	0	0	0		c32	Only one of the start fields can have a value.	startproc = "" or (startproc = "*" and startframe = "")	0	4
	5	role	21	34	0	32	1	50	15	8	32	0	18	Application role:	0	0	0	1088	0	0	0		c32			0	5
	6	compform	21	5	0	3	1	31	17	16	3	0	28	 Always use compiled forms:	0	0	0	5208	0	0	0	yes	c3	Answer must be "yes" or "no"	compform in ["yes", "ye", "no", "y", "n"]	0	6
	7	newcompform	21	5	0	3	1	47	18	0	3	0	44	 By default, new frames use compiled forms:	0	0	0	16782416	0	0	0	yes	c3	Answer must be "yes" or "no".	newcompform in ["yes", "ye", "no", "y", "n"]	0	7
	8	title	32	40	0	40	1	40	0	0	40	0	0		40	0	0	131072	512	0	0	Application Defaults	c40			0	8
	9	menustyle	21	17	0	15	1	42	20	17	15	0	27	Style for new Menu frames:	0	0	0	5208	0	0	0	tablefield	c15	Answer must be "tablefield" or "single-line".	menustyle in ["tablefield", "single-line"]	0	9
TRIM:
	1	2	Enter values for the following defaults that you wish to use:	0	0	0	0
