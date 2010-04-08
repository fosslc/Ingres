/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** DSfldcol.c
**
**	Data structure template for FLDCOL structure.
**	The FLDCOL structure consists of a FLDHDR structure and a FLDTYPE.
**
**	static char	Sccsid[] = "@(#)DSfldcol.c	30.1	1/16/85";
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
** # define 	CFHTITLE	24
** # define	CFTDEFAULT	60
** # define	CFTVALSTR	88
** # define	CFTVALMSG	92
** # define	CFTVALCHK	96
** # define	CFTFMTSTR	100
** # define	CFTFMT	104
** 
** # else
*/
# ifndef	INCL_FEDSZ

# define 	CFHTITLE	CL_OFFSETOF(struct tblcolstr, flhdr.fhtitle)
# define	CFTDEFAULT	CL_OFFSETOF(struct tblcolstr, fltype.ftdefault)
# define	CFTVALSTR 	CL_OFFSETOF(struct tblcolstr, fltype.ftvalstr)
# define	CFTVALMSG	CL_OFFSETOF(struct tblcolstr, fltype.ftvalmsg)
# define	CFTVALCHK	CL_OFFSETOF(struct tblcolstr, fltype.ftvalchk)
# define	CFTFMTSTR	CL_OFFSETOF(struct tblcolstr, fltype.ftfmtstr)

# endif	/* INCL_FEDSZ */

/* # endif */

static DSPTRS fldcolPtr[] =
{
	CFHTITLE, DSSTRP, 0, 0,
	CFTDEFAULT, DSSTRP, 0, 0,
	CFTVALSTR, DSSTRP, 0, 0,
	CFTVALMSG, DSSTRP, 0, 0,
	CFTFMTSTR, DSSTRP, 0, 0,
};

GLOBALDEF DSTEMPLATE DSfldcol =
{
	DS_FLDCOL,			/* ds_id */
	sizeof(struct tblcolstr),	/* ds_size */
	5,				/* ds_cnt */
	DS_IN_CORE,			/* dstemp_type */
	fldcolPtr,			/* ds_ptrs */
	NULL,				/* dstemp_file */
	NULL				/* dstemp_func */
};
