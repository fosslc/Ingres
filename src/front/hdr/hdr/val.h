/*
**	Copyright (c) 2004 Ingres Corporation
*/
typedef struct val
{
	i2	val_type;	/* type of value */

	union
	{
		char		*v_str;		/* string */
		NUMBER		v_num;		/* number */
		DATENTRNL	*v_date;	/* date */
	} val_val;
}   VAL;
