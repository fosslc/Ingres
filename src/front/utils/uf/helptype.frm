COPYFORM:	6.0	1989_03_03 07:31:22 GMT
FORM:	help_type	Generic Field Selection Help For Types.	Displays the allowed types and allows the user to enter a length and select the type.
	54	16	0	0	2	0	4	7	0	0	0	0	0	129	0	4
FIELD:
	0	title	32	40	0	40	1	40	0	0	40	0	0		40	0	0	131072	512	0	0	Help -- Field Selection for Type.	c40			0	0
	1	types	0	10	0	2	14	16	2	1	1	3	0		1	1	0	1073741857	0	0	0					1	1
	0	type	32	7	0	7	1	7	0	1	7	0	1	Type	1	1	0	65536	512	0	0		c7			2	2
	1	length	-30	3	0	4	1	6	0	9	4	0	9	Length	9	1	0	4096	0	0	0		-i4		types.length is null or(types.type in["c","text","*char"]and types.length>0 and types.length<=4000)or(not types.type in["none","string","date","money"]and types.length>0 and types.length<=8)	2	3
TRIM:
	18	3	Position the cursor on the desired	0	0	0	0
	18	4	row and then if applicable enter	0	0	0	0
	18	5	the desired length to select the	0	0	0	0
	18	6	type.	0	0	0	0
