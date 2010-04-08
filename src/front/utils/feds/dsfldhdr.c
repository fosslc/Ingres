/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** DSfldhdr.c
**
**	Data structure template for FLDHDR structure
**
**	static char	Sccsid[] = "@(#)DSfldhdr.c	30.1	1/16/85";
**
** History:
**	29-nov-91 (seg)
**	    remove obsolote PCINGRES ifdef, change defs to Ingres standard
**	    CL_OFFSETOF macro.  Previous expression is not allowed in ANSI.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
*/


/* Jam hints
**
LIBRARY = IMPFRAMELIBDATA
**
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
**	Take out VMS DS structures.  Written 06/85 by jrc.
**
** # ifdef VMS
**
**
** # define	FHSfhtitle	24
** 	
** # else
*/

# ifndef	INCL_FEDSZ

# define	FHSfhtitle	CL_OFFSETOF(struct fldhdrstr, fhtitle)

# endif	/* INCL_FEDSZ */

/* # endif */

static DSPTRS fldhdrPtr[] =
{
	FHSfhtitle, DSSTRP, 0, 0
};

GLOBALDEF DSTEMPLATE DSfldhdr =
{
	DS_FLDHDR,			/* ds_id */
	sizeof(struct fldhdrstr),	/* ds_size */
	1,				/* ds_cnt */
	DS_IN_CORE,			/* dstemp_type */
	fldhdrPtr,			/* ds_ptrs */
	NULL,				/* dstemp_file */
	NULL				/* dstemp_func */
};
