/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include 	<compat.h>
# include	<iiufsys.h>

/*
**
** Name: iiufutilL.c - lower-case version of iiufutil.c
**
** Description:
**	This file includes iiufutil.c, renaming all of the global symbols
**	to their lowercase equivalents, in order to support the -lowercase
**	option of the LPI FORTRAN compiler.
**
** History:
**	27-feb-90 (russ)
**		Created.
**      02-aug-1995 (morayf)
**              Added iips_ define for donc change of 17-dec-1993.
*/

# ifdef MIXEDCASE_FORTRAN
# define	IIxintrans	iixintrans
# define	IIxouttrans	iixouttrans
# define	II_sdesc	ii_sdesc
# define	iinum_		iinum
# define	iips_		iips
# define	iistr_		iistr
# define	iisd_		iisd
# define	iisdno_		iisdno
# define	iislen_		iislen
# define	iisadr_		iisadr

# include "iiufutil.c"

# else /* MIXEDCASE_FORTRAN */

static int iiufutilL_dummy;

# endif /* MIXEDCASE_FORTRAN */
