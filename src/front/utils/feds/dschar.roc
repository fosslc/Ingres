
# include <compat.h>
# include	<ds.h>

/*
** Copyright (c) 2004 Ingres Corporation
**
** dschar.c -- Data Template Structure Definition for a char.
**
**  History:
**	12/10/89 (dkh) - Put in transfer vector for outside access to DSchar.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
*/

/* Jam hints
**
LIBRARY = IMPFRAMELIBDATA
**
*/

GLOBALDEF DSTEMPLATE	DSchar = {
	/* i4	ds_id	*/		DS_CHAR,
	/* i4	ds_size	*/		sizeof (char),
	/* i4	ds_cnt	*/		0,
	/* i4	dstemp_type */		DS_IN_CORE,
	/* DSPTRS *ds_ptrs */		0,
	/* char	*dstemp_file */		0,
	/* i4	(*dstemp_func)() */	0
};


GLOBALDEF DSTEMPLATE	*IIDSchar = &DSchar;
