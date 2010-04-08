/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include 	<compat.h>
# include	<iiufsys.h>

/*
**
** Name: iiufsqlU.c - upper-case version of iiufsql.c
**
** Description:
**	This file includes iiufsql.c, renaming all of the global symbols
**	to their uppercase equivalents, in order to support the -uppercase
**	option of the LPI FORTRAN compiler.
**
** History:
**	27-feb-90 (russ)
**		Created.
**	12-sep-90 (barbara)
**	    Added four missing defintions.
**	17-dec-1992 (larrym)
**	    Added IIsqlcdInit and IIsqGdInit
**      09-mar-1993 (larrym)
**          added IILQcnConName (iilqcn)
*/

# ifdef MIXEDCASE_FORTRAN
# define	iisqco_	IISQCO
# define	iisqdi_	IISQDI
# define	iisqfl_	IISQFL
# define	iisqin_	IISQIN
# define	iisqlc_	IISQLC
# define	iisqgd_	IISQGD
# define	iisqst_	IISQST
# define	iisqpr_	IISQPR
# define	iisqus_	IISQUS
# define	iisqpe_	IISQPE
# define	iisqei_	IISQEI
# define	iisqex_	IISQEX
# define	iicsda_	IICSDA
# define	iisqmo_	IISQMO
# define	iisqpa_	IISQPA
# define        iisqtp_ IISQTP
# define        iilqsi_ IILQSI
# define        iisqde_ IISQDE
# define        iisqdt_ IISQDT
# define        iilqcn_ IILQCN

# include "iiufsql.c"

# else /* MIXEDCASE_FORTRAN */

static int iiufsqlU_dummy;

# endif /* MIXEDCASE_FORTRAN */
