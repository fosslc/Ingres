/*
** DSf8.c
**
**	Data structure template for an F8 (float).
**
**	static char	Sccsid[] = "@(#)DSf8.c	30.1	1/24/85";
*/

# include	<compat.h>
#include    <gl.h>
# include	<si.h>
# include	<ds.h>

GLOBALDEF readonly DSTEMPLATE	DSf8 = {
	/* i4	ds_id	*/		DS_F8,
	/* i4	ds_size	*/		sizeof (f8),
	/* i4	ds_cnt	*/		0,
	/* i4	dstemp_type */		DS_IN_CORE,
	/* DSPTRS *ds_ptrs */		0,
	/* char	*dstemp_file */		0,
	/* i4 	(*dstemp_func)() */	0
};
