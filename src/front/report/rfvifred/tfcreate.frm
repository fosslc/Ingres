COPYFORM:	6.0	1989_04_08 02:01:23 GMT
FORM:	tfcreate	VIFRED Table Field Edit Form.	Displays table field display and column information for editing.
	80	22	0	0	10	0	1	7	0	0	0	0	0	64	0	13
FIELD:
	0	name	32	32	0	24	1	45	2	0	24	0	21	Name of Table Field:	0	0	0	69640	64	0	0		c24			0	0
	1	rows	30	4	0	3	1	32	2	46	3	0	29	Number of Rows to Display:	0	0	0	65536	0	0	0	4	-i3	Number of rows can only be 1 to 99	rows > 0 and rows < 100	0	1
	2	lines	32	1	0	1	1	22	3	0	1	0	21	Display Lines?(y/n):	0	0	0	65600	0	0	0	y	c1	Value for Display Lines can only be 'y' or 'n'.	lines in ["y", "n"]	0	2
	3	trackcursor	32	1	0	1	1	30	3	46	1	0	29	Highlight Current Row?(y/n):	0	0	0	65600	0	0	0	n	c1	Value for Row Highlighting can only be 'y' or 'n'.	trackcursor in ["y", "n"]	0	3
	4	coltitle	32	1	0	1	1	30	4	0	1	0	29	Display Column Titles?(y/n):	0	0	0	65600	0	0	0	y	c1	Value for Column Titles can only be 'y' or 'n'.	coltitle in ["y", "n"]	0	4
	5	invisfld	32	1	0	1	1	24	4	46	1	0	23	Invisible Field?(y/n):	0	0	0	65600	0	0	0	n	c1	Value for Invisible Field can only be 'y' or 'n'.	invisfld in ["y", "n"]	0	5
	6	tfcreate	0	12	0	3	16	78	6	0	1	3	0		0	0	0	1073742881	0	0	0					1	6
	0	title	32	50	0	30	1	30	0	1	30	3	1	Title of a Column	1	1	0	69632	64	0	0		c30			2	7
	1	internname	32	32	0	24	1	24	0	32	24	3	32	Column Internal Name	32	1	0	69632	64	0	0		c24			2	8
	2	format	32	60	0	20	1	20	0	57	20	3	57	Display Format	57	1	0	69632	64	0	0		c20			2	9
	7	colsep	32	1	0	1	1	33	1	46	1	0	32	Display Column Separator?(y/n):	0	0	0	16842816	0	0	0	y	c1	Value for Column Separator can only be 'y' or 'n'.	colsep in ["y", "n"]	0	10
	8	dispbotx	32	1	0	1	1	31	5	0	1	0	30	Display special bottom?(y/n):	0	0	0	16842816	0	0	0	n	c1	Value for Display Bottom can only be 'y' or 'n'.	dispbotx in ["y", "n"]	0	11
	9	disptop	32	1	0	1	1	20	5	46	1	0	19	Display Top?(y/n):	0	0	0	16842816	0	0	0	y	c1	Value for Display Top can only be 'y' or 'n'.	disptop in ["y", "n"]	0	12
TRIM:
	0	0	VIFRED - Table Field	0	0	0	0
