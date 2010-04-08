/*
**Copyright (c) 2004 Ingres Corporation
*/
# include	<ds.h>
/*
**
LIBRARY = IMPCOMPATLIBDATA
**
**      03-Jan-96 (fanra01)
**          Added GLOBALDEF to definition.  For DLL import decoration.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPCOMPATLIBDATA
**	    in the Jamfile.

*/
GLOBALDEF DSTEMPLATE	DSnat = {
	/* i4	ds_id	*/		DS_NAT,
	/* i4	ds_size	*/		sizeof (i4),
	/* i4	ds_cnt	*/		0,
	/* i4	dstemp_type */		DS_IN_CORE,
	/* DSPTRS *ds_ptrs */		0,
	/* char	*dstemp_file */		0,
	/* i4  	(*dstemp_func)() */	0
};
