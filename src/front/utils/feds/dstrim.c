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
** DStrim.h
**
**	Data structure template for TRIM structure
**
**	static char	Sccsid[] = "@(#)DStrim.c	30.3	1/24/85";
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


/*
**	Take out VMS DS structures.  Written 06/85 by jrc.
**
** # ifdef VMS
**
** 
** # define	TStrmstr	8
** 
** # else
*/

# ifndef	INCL_FEDSZ

# define	TStrmstr 	CL_OFFSETOF(struct trmstr, trmstr)

# endif	/* INCL_FEDSZ */

/* # endif */

static DSPTRS trimPtr[] =
{
	TStrmstr, DSSTRP, 0, 0
};

GLOBALDEF DSTEMPLATE DStrim =
{
	DS_TRIM,			/* ds_id */
	sizeof(TRIM),			/* ds_size */
	1,				/* ds_cnt */
	DS_IN_CORE,			/* dstemp_type */
	trimPtr,			/* ds_ptrs */
	NULL,				/* dstemp_file */
	NULL				/* dstemp_func */
};

static DSPTRS Pptr[] =
{
	0, DSNORMAL, 0, DS_TRIM
};

GLOBALDEF DSTEMPLATE DSptrim =
{
	DS_PTRIM,			/* ds_id */
	sizeof (TRIM *),		/* ds_size */
	1,				/* ds_cnt */
	DS_IN_CORE,			/* dstemp_type */
	Pptr,				/* ds_ptrs */
	NULL,				/* dstemp_file */
	NULL				/* dstemp_func */
};

GLOBALDEF DSTEMPLATE DSspecial =
{
	DS_SPECIAL,			/* ds_id */
	sizeof (TRIM *),		/* ds_size */
	1,				/* ds_cnt */
	DS_IN_CORE,			/* dstemp_type */
	Pptr,				/* ds_ptrs */
	NULL,				/* dstemp_file */
	NULL				/* dstemp_func */
};
