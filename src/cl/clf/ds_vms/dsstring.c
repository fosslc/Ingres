# include	<compat.h>
# include	<gl.h>
# include	<si.h>
# include	<ds.h>

/*
**	DSstring.h
**
**	Data structure template for null-terminated character strings
** History:
**
**		Created
**	26-oct-2001 (kinte01)
**		clean up compiler warnings	
*/

static 	DSPTRS dsptrs = {
	/* i4	ds_off; */	(i4) 0,
	/* i4	ds_kind; */	DSSTRP,
	/* i4	ds_sizeoff; */	0,
	/* i4	ds_temp; */	0
};

GLOBALDEF readonly DSTEMPLATE	DSstring = {
	/* i4	ds_id; */		DS_STRING,
	/* i4	ds_size; */		sizeof (char *),
	/* i4	ds_cnt; */		(i4) 1,
	/* i4	dstemp_type; */		(i4) DS_IN_CORE,
	/* DSPTRS	*ds_ptrs; */	&dsptrs,
	/* char	*dstemp_file; */	NULL,
	/* i4	(*dstemp_func)(); */	NULL,
};

