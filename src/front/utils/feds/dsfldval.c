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
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<fedsz.h>

/*
** DSfldval.h
**
**	Data structure template for FLDVAL structure
**
*/


/* Jam hints
**
LIBRARY = IMPFRAMELIBDATA
**
*/


/*
**	Take out VMS DS structures.  Written 06/85 by jrc.
**
** History:
**	29-nov-91 (seg)
**	    remove obsolote PCINGRES ifdef, change defs to Ingres standard
**	    CL_OFFSETOF macro.  Previous expression is not allowed in ANSI.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
**
** # ifdef VMS
**
** 
** # define	FVfvbufr	4
** 
** # else
*/

# ifndef	INCL_FEDSZ

# define	FVfvbufr	CL_OFFSETOF(struct fldval, fvbufr)

# endif	/* INCL_FEDSZ */

/* # endif */

static DSPTRS fldvalPtr[] =
{
	FVfvbufr, DSSTRP, 0, 0
};

GLOBALDEF DSTEMPLATE DSfldval =
{
	DS_FLDVAL,			/* ds_id */
	sizeof(struct fldval),		/* ds_size */
	0,				/* ds_cnt */
	DS_IN_CORE,			/* dstemp_type */
	NULL,				/* ds_ptrs */
	NULL,				/* dstemp_file */
	NULL				/* dstemp_func */
};
