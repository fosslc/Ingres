COPYFORM:	6.4	2008_03_06 21:01:39 GMT  
FORM:	vifattr	VIFRED Field/Column Attributes Form.	
	80	22	0	0	14	0	1	9	0	0	0	0	0	64	0	16
FIELD:
	0	vifattr	0	16	0	2	20	26	2	0	1	3	0		0	0	0	1057	0	0	0					1	0
	0	attr	32	20	0	20	1	20	0	1	20	3	1	Attribute	1	1	0	65536	512	0	0		c20			2	1
	1	sett	32	1	0	1	1	3	0	22	1	3	22	Set	22	1	0	65600	0	0	0		c1	"Set" only takes 'y' or 'n'.	vifattr.sett in ["y", "n"]	2	2
	1	name	32	32	0	32	1	47	3	29	32	0	15	Internal Name:	0	0	0	65536	0	0	0		c32			0	3
	2	datatype	32	45	0	25	1	36	5	29	25	0	11	Data Type:	0	0	0	65600	64	0	0		c25			0	4
	3	nullable	32	1	0	1	1	11	5	66	1	0	10	Nullable:	0	0	0	65600	0	0	0		c1	Datatype Nullability may only be 'y' or 'n'.	nullable in ["y", "n"]	0	5
	4	derived	32	1	0	1	1	10	7	29	1	0	9	Derived:	0	0	0	65600	0	0	0		c1	Field Derivability may only be 'y' or 'n'.	derived in ["y", "n"]	0	6
	5	expl_text	32	25	0	25	1	26	7	41	25	0	1		0	0	0	65536	512	0	0		c25			0	7
	6	valstrtitle	32	28	0	28	1	29	9	28	28	0	1		0	0	0	65536	512	0	0		c28			0	8
	7	valstr	32	240	0	50	1	51	10	28	50	0	1		0	0	0	69632	64	0	0		c50			0	9
	8	valmsg	32	100	0	50	2	50	12	29	50	1	0	Validation Error Message:	0	0	0	69632	64	0	0		c50			0	10
	9	defval	32	50	0	50	2	40	15	29	25	0	15	Default Value:	0	0	0	65536	0	0	0		c50.25			0	11
	10	colortitle	32	6	0	6	1	7	18	28	6	0	1		0	0	0	0	512	0	0	Color	c6			0	12
	11	color	30	4	0	1	1	2	18	35	1	0	1		0	0	0	65536	0	0	0		-i1	Color value must be in range 0 thru 7.	color >= 0 and color <= 7	0	13
	12	scrollable	32	1	0	1	1	13	20	29	1	0	12	Scrollable:	0	0	0	16842816	0	0	0		c1			0	14
	13	scrollsz	30	4	0	4	1	17	20	57	4	0	13	Scroll Size:	0	0	0	16842752	0	0	0		-i4			0	15
TRIM:
	0	0	VIFRED - Attributes for Field	0	0	0	0
