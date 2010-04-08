COPYFORM:	6.0	1989_09_08 00:57:57 GMT  
FORM:	vftrmatr	VIFRED Trim attributes form	
	80	10	0	0	3	0	2	8	0	0	0	0	0	0	0	5
FIELD:
	0	vftrmatr	0	4	0	2	8	26	2	0	1	3	0		0	0	0	1057	0	0	0					1	0
	0	attr	32	20	0	20	1	20	0	1	20	3	1	Attribute	1	1	0	65536	512	0	0		c20			2	1
	1	sett	32	1	0	1	1	3	0	22	1	3	22	Set	22	1	0	65600	0	0	0		c1	"Set" only takes 'y' or 'n'.	vftrmatr.sett in ["y", "n"]	2	2
	1	color	30	4	0	1	1	2	6	44	1	0	1		0	0	0	65536	0	0	0		-i1	Color value must be in range 0 thru 7.	color >= 0 and color <= 7	0	3
	2	colortitle	32	6	0	6	1	7	6	36	6	0	1		0	0	0	0	512	0	0		c6			0	4
TRIM:
	0	0	VIFRED - Attributes for Trim	0	0	0	0
	37	8	(Enter a color value from 0 to 7)	0	0	0	0
