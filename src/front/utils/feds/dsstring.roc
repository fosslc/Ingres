/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include	<ds.h>

/*
** dsstring.c -- Data Template Structure Definition for a null-terminated
**			character string.
**
**  History:
**	12/10/89 (dkh) - Put in transfer vector for outside access to DSstring.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
*/


/* Jam hints
**
LIBRARY = IMPFRAMELIBDATA
**
*/


static	DSPTRS dsptrs = {
	/* i4	ds_off; */	(i4) 0,
	/* i4	ds_kind; */	DSSTRP,
	/* i4	ds_sizeoff; */	0,
	/* i4	ds_temp; */	0
};

GLOBALDEF DSTEMPLATE	DSstring = {
	/* i4	ds_id; */		DS_STRING,
	/* i4	ds_size; */		sizeof (char *),
	/* i4	ds_cnt; */		(i4) 1,
	/* i4	dstemp_type; */		(i4) DS_IN_CORE,
	/* DSPTRS	*ds_ptrs; */	&dsptrs,
	/* char	*dstemp_file; */	NULL,
	/* i4	(*dstemp_func)(); */	NULL,
};

GLOBALDEF DSTEMPLATE	*IIDSstring = &DSstring;
