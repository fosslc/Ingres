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
** DSfield.h
**
**	Data structure template for FIELD structure
**
**	static char	Sccsid[] = "@(#)DSfield.c	30.3	1/24/85";
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
** # define	FLDTAG	0
** # define	FLDVAR	4
** 
** # else
*/

# ifndef	INCL_FEDSZ
# define	FLDTAG	CL_OFFSETOF(struct fldstr, fltag)
# define	FLDVAR 	CL_OFFSETOF(struct fldstr, fld_var)

# endif	/* INCL_FEDSZ */

/* # endif */

static DSPTRS fieldPtr[] =
{
	FLDVAR, DSUNION, FLDTAG, 0
};

GLOBALDEF DSTEMPLATE DSfield =
{
	DS_FIELD,			/* ds_id */
	sizeof(FIELD),			/* ds_size */
	1,				/* ds_cnt */
	DS_IN_CORE,			/* dstemp_type */
	fieldPtr,			/* ds_ptrs */
	NULL,				/* dstemp_file */
	NULL				/* dstemp_func */
};

/*	DS template for (FIELD *) structures */
static DSPTRS Pptr[] =
{
	0,	DSNORMAL, 0,	DS_FIELD
};

GLOBALDEF DSTEMPLATE DSpfield =
{
	DS_PFIELD,			/* ds_id */
	sizeof(FIELD *),		/* ds_size */
	1,				/* ds_cnt */
	DS_IN_CORE,			/* dstemp_type */
	Pptr,				/* ds_ptrs */
	NULL,				/* dstemp_file */
	NULL				/* dstemp_func */
};

GLOBALDEF DSTEMPLATE DStable =
{
	DS_TABLE,			/* ds_id */
	sizeof(FIELD *),		/* ds_size */
	1,				/* ds_cnt */
	DS_IN_CORE,			/* dstemp_type */
	Pptr,				/* ds_ptrs */
	NULL,				/* dstemp_file */
	NULL				/* dstemp_func */
};
