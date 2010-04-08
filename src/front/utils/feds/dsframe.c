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
** DSframe.h
**
**	Data structure template for FRAME structure
**
**	static char	Sccsid[] = "@(#)DSframe.c	30.2	1/10/85";
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
** # define	FSfrfld		16
** # define	FSfrfldno	20
** # define	FSfrnsfld	24
** # define	FSfrnsno	28
** # define	FSfrtrim	32
** # define	FSfrtrmno	36
** # define	FSfrcurnm	104
*/

/* # else */

# ifndef	INCL_FEDSZ

# define	FSfrfld		CL_OFFSETOF(struct frmstr, frfld)
# define	FSfrnsfld	CL_OFFSETOF(struct frmstr, frnsfld)
# define	FSfrtrim	CL_OFFSETOF(struct frmstr, frtrim)
# define	FSfrcurnm	CL_OFFSETOF(struct frmstr, frcurnm)
# define	FSfrfldno	CL_OFFSETOF(struct frmstr, frfldno)
# define	FSfrnsno	CL_OFFSETOF(struct frmstr, frnsno)
# define	FSfrtrmno	CL_OFFSETOF(struct frmstr, frtrimno)

# endif	/* INCL_FEDSZ */

/* # endif */

static DSPTRS framePtr[] =
{
	FSfrfld, DSARRAY, FSfrfldno, -DS_FIELD,
	FSfrnsfld, DSARRAY, FSfrnsno, -DS_FIELD,
	FSfrtrim, DSARRAY, FSfrtrmno, -DS_TRIM,
	FSfrcurnm, DSSTRP, 0, 0
};

GLOBALDEF DSTEMPLATE DSframe =
{
	DS_FRAME,			/* ds_id */
	sizeof(struct frmstr),		/* ds_size */
	4,				/* ds_cnt */
	DS_IN_CORE,			/* dstemp_type */
	framePtr,			/* ds_ptrs */
	NULL,				/* dstemp_file */
	NULL				/* dstemp_func */
};
