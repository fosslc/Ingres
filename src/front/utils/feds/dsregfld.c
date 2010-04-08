/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
** DSregfld.c
**
**	Data structure template for REGFLD structure.
**	The REGFLD structure consists of a FLDHDR structure, a FLDTYPE,
**	and a FLDVAL.
**
**	static char	Sccsid[] = "@(#)DSregfld.c	30.2	1/21/85";
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
** # define	RFhtitle	24
** # define	RFtdefault	60
** # define	RFtvalstr	88
** # define	RFtvalmsg	92
** # define	RFtvalchk	96
** # define	RFtfmtstr	100
** # define	RFtfmt	104
** # define	RFvbufr	112
** 
** # else
*/

# ifndef	INCL_FEDSZ

# define	RFhtitle	CL_OFFSETOF(struct regfldstr, flhdr.fhtitle)
# define	RFtdefault	CL_OFFSETOF(struct regfldstr, fltype.ftdefault)
# define	RFtvalstr	CL_OFFSETOF(struct regfldstr, fltype.ftvalstr)
# define	RFtvalmsg	CL_OFFSETOF(struct regfldstr, fltype.ftvalmsg)
# define	RFtvalchk	CL_OFFSETOF(struct regfldstr, fltype.ftvalchk)
# define	RFtfmtstr	CL_OFFSETOF(struct regfldstr, fltype.ftfmtstr)

# endif	/* INCL_FEDSZ */

/* # endif */

static DSPTRS regfldPtr[] =
{
	RFhtitle, DSSTRP, 0, 0,
	RFtdefault, DSSTRP, 0, 0,
	RFtvalstr, DSSTRP, 0, 0,
	RFtvalmsg, DSSTRP, 0, 0,
	RFtfmtstr, DSSTRP, 0, 0
};

GLOBALDEF DSTEMPLATE DSregfld =
{
	DS_REGFLD,			/* ds_id */
	sizeof(struct regfldstr),	/* ds_size */
	5,				/* ds_cnt */
	DS_IN_CORE,			/* dstemp_type */
	regfldPtr,			/* ds_ptrs */
	NULL,				/* dstemp_file */
	NULL				/* dstemp_func */
};

/*
** Template for a pointer to a REGFLD structure.  Such a pointer
** is a member of the union in a FIELD structure.
*/
static DSPTRS pregfldPtr[] =
{
	0, DSNORMAL, 0, DS_REGFLD
};

GLOBALDEF DSTEMPLATE DSpregfld = {
	DS_PREGFLD,				/* ds_id */
	sizeof (struct regfldstr *),		/* ds_size */
	1,					/* ds_cnt */
	DS_IN_CORE,				/* dstemp_type */
	pregfldPtr,				/* ds_ptrs */
	NULL,					/* dstemp_file */
	NULL					/* dstemp_func */
};
