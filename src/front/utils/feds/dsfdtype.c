/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
** DSfldtype.c
**
**	Data structure template for FLDTYPE structure
**
**	static char	Sccsid[] = "@(#)DSfldtype.c	30.1	1/16/85";
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
** # define	FTdefault	0
** # define	FTvalstr	28
** # define	FTvalmsg	32
** # define	FTvalchk	36
** # define	FTfmtstr	40
** # define	FTfmt	44
** 
** # else
*/

# define 	FTdefault 	CL_OFFSETOF(struct fldtypinfo, ftdefault)
# define 	FTvalstr 	CL_OFFSETOF(struct fldtypinfo, ftvalstr)
# define 	FTvalmsg 	CL_OFFSETOF(struct fldtypinfo, ftvalmsg)
# define 	FTvalchk 	CL_OFFSETOF(struct fldtypinfo, ftvalchk)
# define 	FTfmtstr 	CL_OFFSETOF(struct fldtypinfo, ftfmtstr)

static DSPTRS fldtypePtr[] =
{
	FTdefault, DSSTRP, 0, 0,
	FTvalstr, DSSTRP, 0, 0,
	FTvalmsg, DSSTRP, 0, 0,
	FTfmtstr, DSSTRP, 0, 0,
};

GLOBALDEF DSTEMPLATE DSfldtype =
{
	DS_FLDTYPE,			/* ds_id */
	sizeof(struct fldtypinfo),	/* ds_size */
	4,				/* ds_cnt */
	DS_IN_CORE,			/* dstemp_type */
	fldtypePtr,			/* ds_ptrs */
	NULL,				/* dstemp_file */
	NULL				/* dstemp_func */
};
