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
# include	<list.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	"pos.h"
# include	"node.h"
# include	"writefrm.h"
# include	<fedsz.h>

/*
** DSwritefrm.c
**
**	Data structure template for WRITEFRM structure
**		(line table + frame)
**
**  History:
**	static char	Sccsid[] = "@(#)DSwritefrm.c	30.1	1/21/85";
**	03/23/87 (dkh) -Added support for ADTs.
**	29-oct-93 (swm)
**		Dummied out truncating i4 casts for offset definitions
**		when linting to avoid lint truncation warnings which can
**		safely be ignored.
**      17-Oct-2002 (bonro01)
**              Modified macros that were causing compile errors on the
**              64-bit build of SGI.  Extra Cast specifier was needed in order
**              to convert a 64-bit address to a 32-bit offset.
*/

# ifndef	INCL_FEDSZ
# if defined(NODSPTRS) || defined(LINT)

# define	WFlinesize	0
# define	WFline		4
# define	WFframe		8

# else

# define	WFlinesize		(i4)(long) &(((WRITEFRM *)0)->linesize)
# define	WFline		(i4)(long) &(((WRITEFRM *)0)->line)
# define	WFframe		(i4)(long) &(((WRITEFRM *)0)->frame)

# endif 	/* NODSPTRS */
# endif	/* INCL_FEDSZ */

static DSPTRS Ptrs[] =
{
	WFline, DSARRAY, WFlinesize, -DS_VFNODE,
	WFframe, DSNORMAL, 0, DS_FRAME,
};

GLOBALDEF DSTEMPLATE DSwframe =
{
	DS_WRITEFRM,			/* ds_id */
	(i4) sizeof(WRITEFRM),		/* ds_size */
	(i4) 2,				/* ds_cnt */
	(i4) DS_IN_CORE,		/* dstemp_type */
	Ptrs,				/* ds_ptrs */
	NULL,				/* dstemp_file */
	NULL				/* dstemp_func */
};
