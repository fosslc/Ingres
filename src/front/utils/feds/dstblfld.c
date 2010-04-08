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
** DStblfld.h
**
**	Data structure template for TBLFLD structure.
**	The TBLFLD structure contains a FLDHDR structure within it.
**
** History:
**	29-nov-91 (seg)
**	    remove obsolote PCINGRES ifdef, change defs to Ingres standard
**	    CL_OFFSETOF macro.  Previous expression is not allowed in ANSI.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
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
** # ifdef VMS
**
** 
** # define	TFSfhtitle	24
** # define	TFStfcols	92
** # define	TFStfflds	112
** # define	TFStfwins	116
** # define	TFStftoprow	120
** # define	TFStfbotrow	124
** 
** # else
*/

# ifndef	INCL_FEDSZ

# define	TFSfhtitle 	CL_OFFSETOF(struct tflfldstr, tfhdr.fhtitle)
# define	TFStfflds	CL_OFFSETOF(struct tflfldstr, tfflds)
# define	TFStfcols	CL_OFFSETOF(struct tflfldstr, tfcols)
# define	TFStfwins	CL_OFFSETOF(struct tflfldstr, tfwins)

# endif	/* INCL_FEDSZ */

/* # endif */

static DSPTRS tblfldPtr[] =
{
		TFSfhtitle, DSSTRP, 0, 0,
		TFStfflds, DSARRAY, 	TFStfcols, -DS_FLDCOL,
		TFStfwins, DSNORMAL, 0, DS_FLDVAL,
};

GLOBALDEF DSTEMPLATE DStblfld =
{
	DS_TBLFLD,			/* ds_id */
	sizeof(struct tflfldstr),	/* ds_size */
	3,				/* ds_cnt */
	DS_IN_CORE,			/* dstemp_type */
	tblfldPtr,			/* ds_ptrs */
	NULL,				/* dstemp_file */
	NULL				/* dstemp_func */
};

/*
** Template for a pointer to a TBLFLD structure.  Such a pointer is
** a member of the union in a FIELD structure.
*/
static DSPTRS ptblfldPtr[] =
{
	0, DSNORMAL, 0, DS_TBLFLD
};

GLOBALDEF DSTEMPLATE DSptblfld = {
	DS_PTBLFLD,				/* ds_id */
	sizeof (struct tflfldstr *),		/* ds_size */
	1,					/* ds_cnt */
	DS_IN_CORE,				/* dstemp_type */
	ptblfldPtr,				/* ds_ptrs */
	NULL,					/* dstemp_file */
	NULL					/* dstemp_func */
};
