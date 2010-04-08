/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include 	<compat.h>
# include	<iiufsys.h>

/*
**
** Name: iiufsqlM.c - mixed-case version of iiufsql.c
**
** Description:
**	This file includes iiufsql.c, renaming all of the global symbols
**	to their mixed case equivalents, in order to support the -mixedcase
**	option of the LPI FORTRAN compiler.
**
** History:
**	27-feb-90 (russ)
**		Created.
**	30-Jul-1992 (fredv)
**		Added two missing symbols for MIXEDCASE_FORTRAN, 
**		IIsqDt and IIsqDe.
**	17-dec-1992 (larrym)
**	    Added IIsqlcdInit and IIsqGdInit
**	06-feb-1996 (morayf)
**	    Added IILQcn, IIsqTP and IILQsi definitions.
**	    This, along with similar additions, allows shared library
**	    linking of libq to succeed !
**  22-Sep-2003 (fanra01)
**      Bug 110906
**      Disabled definition for windows platform as there are windows
**      specific calls.
*/
#ifndef NT_GENERIC

# ifdef MIXEDCASE_FORTRAN
# define	iisqco_	IIsqCo
# define	iisqdi_	IIsqDi
# define	iisqfl_	IIsqFl
# define	iisqin_	IIsqIn
# define	iisqlc_	IIsqlc
# define	iisqgd_	IIsqGd
# define	iisqst_	IIsqSt
# define	iisqpr_	IIsqPr
# define	iisqus_	IIsqUs
# define	iisqpe_	IIsqPe
# define	iisqei_	IIsqEI
# define	iisqex_	IIsqEx
# define	iicsda_	IIcsDa
# define	iisqmo_	IIsqMo
# define	iisqpa_	IIsqPa
# define        iisqtp_ IIsqTP
# define        iilqsi_ IILQsi
# define	iisqdt_ IIsqDt
# define	iisqde_ IIsqDe
# define        iilqcn_ IILQcn
# define        iisadr_ IIsadr

# include "iiufsql.c"

# else /* MIXEDCASE_FORTRAN */

static int iiufsqlM_dummy;

# endif /* MIXEDCASE_FORTRAN */

# endif /* NT_GENERIC */
