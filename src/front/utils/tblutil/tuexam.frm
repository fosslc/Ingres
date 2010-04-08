COPYFORM:	6.4	1992_08_21 19:55:27 GMT  
FORM:	tuexaminefr	Tables Utility Examine Table Form.	This form is used by the table utility when users wish to examine a table.  It displays the table name, owner, number of rows, storage structure, etc., in fields, and information pertaining to its columns in a table field.
	80	22	0	0	13	0	0	9	0	0	0	0	0	64	0	18
FIELD:
	0	attributes	0	9	0	5	13	70	9	5	1	3	0		0	0	0	1073742881	0	0	0					1	0
	0	name	32	32	0	32	1	32	0	1	32	0	1	Column Name	1	1	0	65536	0	0	0		c32			2	1
	1	datafmt	32	33	0	14	1	14	0	34	14	0	34	Data Type	34	1	0	65536	64	0	0		c14			2	2
	2	key	32	4	0	4	1	5	0	49	4	0	49	Key #	49	1	0	65536	0	0	0		c4			2	3
	3	nullable	32	4	0	4	1	5	0	55	4	0	55	Nulls	55	1	0	65536	0	0	0		c4			2	4
	4	defable	32	5	0	5	1	8	0	61	5	0	61	Defaults	61	1	0	65536	0	0	0		c5			2	5
	1	utitle	-21	43	0	40	1	40	0	0	40	0	0		40	0	0	131072	512	0	0		c40			0	6
	2	rows	32	12	0	12	1	18	7	6	12	0	6	Rows:	0	0	0	65536	512	0	0		c12			0	7
	3	journaled	32	18	0	18	1	30	7	50	18	0	12	Journaling:	0	0	0	65536	512	0	0		c18			0	8
	4	name	32	32	0	32	1	53	2	18	32	0	21	Information on Table	0	0	0	67584	512	0	0		c32			0	9
	5	owner	32	32	0	32	1	39	4	5	32	0	7	Owner:	0	0	0	65536	512	0	0		c32			0	10
	6	rtype	32	18	0	18	1	30	4	50	18	0	12	Table Type:	0	0	0	65536	512	0	0		c18			0	11
	7	width	32	12	0	12	1	23	5	1	12	0	11	Row Width:	0	0	0	65536	512	0	0		c12			0	12
	8	number	32	12	0	12	1	21	6	3	12	0	9	Columns:	0	0	0	65536	512	0	0		c12			0	13
	9	stostru_bastab	32	18	0	18	1	18	5	43	18	0	0		0	0	0	65536	512	0	0		+c18			0	14
	10	storage	32	32	0	18	1	18	5	62	18	0	0		0	0	0	65536	576	0	0		c18			0	15
	11	ovpag_basown	32	20	0	20	1	20	6	41	20	0	0		0	0	0	65536	512	0	0		+c20			0	16
	12	pages	32	32	0	18	1	18	6	62	18	0	0		0	0	0	65536	576	0	0		c18			0	17
TRIM:
