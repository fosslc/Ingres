# include	<compat.h>
#include    <gl.h>
# include	<si.h>
# include	<ds.h>

GLOBALDEF readonly DSTEMPLATE	DSchar = {
	/* i4	ds_id	*/		DS_CHAR,
	/* i4	ds_size	*/		sizeof (char),
	/* i4	ds_cnt	*/		0,
	/* i4	dstemp_type */		DS_IN_CORE,
	/* DSPTRS *ds_ptrs */		0,
	/* char	*dstemp_file */		0,
	/* i4	(*dstemp_func)() */	0
};
