/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include 	<compat.h>
# include	<iiufsys.h>

/*
**
** Name: iiufsqlL.c - lower-case version of iiufsql.c
**
** Description:
**	This file includes iiufsql.c, renaming all of the global symbols
**	to their lowercase equivalents, in order to support the -lowercase
**	option of the LPI FORTRAN compiler.
**
** History:
**	27-feb-90 (russ)
**		Created.
**	14-sep-90 (barbara)
**		Added four missing definitions.
**	17-dec-1992 (larrym)
**	    Added IIsqlcdInit and IIsqGdInit
**	09-mar-1993 (larrym)
**	    added IILQcnConName (iilqcn)
*/

# ifdef MIXEDCASE_FORTRAN
# define	iisqco_	iisqco
# define	iisqdi_	iisqdi
# define	iisqfl_	iisqfl
# define	iisqin_	iisqin
# define	iisqlc_	iisqlc
# define	iisqgd_	iisqgd
# define	iisqst_	iisqst
# define	iisqpr_	iisqpr
# define	iisqus_	iisqus
# define	iisqpe_	iisqpe
# define	iisqei_	iisqei
# define	iisqex_	iisqex
# define	iicsda_	iicsda
# define	iisqmo_	iisqmo
# define	iisqpa_	iisqpa
# define        iisqtp_ iisqtp
# define        iilqsi_ iilqsi
# define        iisqde_ iisqde
# define        iisqdt_ iisqdt
# define        iilqcn_ iilqcn

# include "iiufsql.c"

# else /* MIXEDCASE_FORTRAN */

static int iiufsqlL_dummy;

# endif /* MIXEDCASE_FORTRAN */
