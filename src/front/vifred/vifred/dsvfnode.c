/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ds.h>
# include	<feds.h>
# include	<fmt.h>
# include	<list.h>
# include	"pos.h"
# include	"node.h"
# include	<fedsz.h>

/*
** History:
**	29-oct-93 (swm)
**		Added history section, as it was missing.
**		Dummied out truncating i4  casts for offset definitions
**		when linting to avoid lint truncation warnings which can
**		safely be ignored.
*/

# ifndef	INCL_FEDSZ
# if defined(NODSPTRS) || defined(LINT)

# define	VNnd_pos	4
# define	VNnd_adj	8

# else

# define	VNnd_pos		(i4) &(((VFNODE *)0)->nd_pos)
# define	VNnd_adj		(i4) &(((VFNODE *)0)->nd_adj)

# endif 	/* NODSPTRS */
# endif	/* INCL_FEDSZ */

/* Template of VFNODE data structure */

static DSPTRS	node_dsptr[] = {
	/* ds_off		       ds_kind   ds_sizeoff ds_temp */
	VNnd_pos, DSNORMAL,         0, DS_POS,
	VNnd_adj, DSNORMAL,         0, DS_LIST
};

GLOBALDEF DSTEMPLATE DSvfnode = {
	/* ds_id */		DS_VFNODE,
	/* ds_size */		sizeof(VFNODE),
	/* ds_cnt */		2,
	/* dstemp_type */	DS_IN_CORE,
	/* ds_ptrs */		node_dsptr,
	/* dstemp_file */	NULL,
	/* dstemp_func */	NULL	
};

/* Template of (VFNODE *) data structure */

static DSPTRS	pptrs[] = {
	/* ds_off		ds_kind   ds_sizeoff ds_temp */
	0,			DSNORMAL,         0, DS_VFNODE,
};

GLOBALDEF DSTEMPLATE DSpvfnode = {
	/* ds_id */		DS_PVFNODE,
	/* ds_size */		sizeof(VFNODE *),
	/* ds_cnt */		1,
	/* dstemp_type */	DS_IN_CORE,
	/* ds_ptrs */		pptrs,
	/* dstemp_file */	NULL,
	/* dstemp_func */	NULL	
};

