COPYFORM:	6.4	1991_05_01 18:14:19 GMT  
FORM:	vqlocalcreate	Emerald Application Create Local Variable Form.	This frame allows a user to specify the name of the local variable before creating it.
	49	11	0	0	6	0	0	9	0	0	0	0	0	193	0	6
FIELD:
	0	title	32	40	0	40	1	40	0	0	40	0	0		40	0	0	131072	512	0	0		c40			0	0
	1	name	32	65	0	32	1	38	2	8	32	0	6	Name:	0	0	0	70744	64	0	0		c32			0	1
	2	data_type	32	103	0	105	3	46	4	3	35	0	11	Data Type:	0	0	0	70672	0	0	0		c103.35			0	2
	3	isarray	21	5	0	3	1	10	7	7	3	0	7	Array:	0	0	0	70744	0	0	0	no	c3	Array must be "Yes" or "No".	isarray in ["y","n","yes","no","ye"]	0	3
	4	nullable	21	5	0	3	1	13	8	4	3	0	10	Nullable:	0	0	0	70744	0	0	0	no	c3	Nullable must be "Yes" or "No".	nullable in ["yes","ye","y","no","n"]	0	4
	5	short_remark	21	62	0	60	2	44	9	0	30	0	14	Short Remark:	0	0	0	66560	0	0	0		c60.30			0	5
TRIM:
