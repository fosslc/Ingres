/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include 	<compat.h>
# include	<iiufsys.h>

/*
**
** Name: iiuftbM.c - mixed-case version of iiuftb.c
**
** Description:
**	This file includes iiuftb.c, renaming all of the global symbols
**	to their mixed case equivalents, in order to support the -mixedcase
**	option of the LPI FORTRAN compiler.
**
** History:
**	27-feb-90 (russ)
**		Created.
**	12-sep-90 (barbara)
**		Added new 6.4 call.
*/

# ifdef MIXEDCASE_FORTRAN
# define	iitbac_		IItbac
# define	iitbce_		IITBce
# define	iitbin_		IItbin
# define	iitbse_		IItbse
# define	iitbsm_		IItbsm
# define	iitclc_		IItclc
# define	iitclr_		IItclr
# define	iitcog_		IItcog
# define	iitcol_		IItcol
# define	iitcop_		IItcop
# define	iitdat_		IItdat
# define	iitdel_		IItdel
# define	iitdte_		IItdte
# define	iitfil_		IItfil
# define	iithid_		IIthid
# define	iitins_		IItins
# define	iitrcp_		IItrcp
# define	iitscp_		IItscp
# define	iitscr_		IItscr
# define	iitune_		IItune
# define	iitunl_		IItunl
# define	iitval_		IItval
# define	iitvlr_		IIrvlr

# include "iiuftb.c"

# else /* MIXEDCASE_FORTRAN */

static int iiuftbL_dummy;

# endif /* MIXEDCASE_FORTRAN */
