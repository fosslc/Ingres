/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include 	<compat.h>
# include	<iiufsys.h>

/*
**
** Name: iiufutilU.c - upper-case version of iiufutil.c
**
** Description:
**	This file includes iiufutil.c, renaming all of the global symbols
**	to their uppercase equivalents, in order to support the -uppercase
**	option of the LPI FORTRAN compiler.
**
** History:
**	27-feb-90 (russ)
**		Created.
**      02-aug-1995 (morayf)
**              Added iips_ define for donc change of 17-dec-1993.
*/

# ifdef MIXEDCASE_FORTRAN
# define	IIxintrans	IIXINTRANS
# define	IIxouttrans	IIXOUTTRANS
# define	II_sdesc	II_SDESC
# define	iinum_		IINUM
# define	iips_		IIPS
# define	iistr_		IISTR
# define	iisd_		IISD
# define	iisdno_		IISDNO
# define	iislen_		IISLEN
# define	iisadr_		IISADR

# include "iiufutil.c"

# else /* MIXEDCASE_FORTRAN */

static int iiufutilL_dummy;

# endif /* MIXEDCASE_FORTRAN */
