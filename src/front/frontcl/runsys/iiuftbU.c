/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include 	<compat.h>
# include	<iiufsys.h>

/*
**
** Name: iiuftbU.c - upper-case version of iiuftb.c
**
** Description:
**	This file includes iiuftb.c, renaming all of the global symbols
**	to their uppercase equivalents, in order to support the -uppercase
**	option of the LPI FORTRAN compiler.
**
** History:
**	27-feb-90 (russ)
**		Created.
**	12-sep-90 (barbara)
**		Added new call for 6.4
*/

# ifdef MIXEDCASE_FORTRAN
# define	iitbac_		IITBAC
# define	iitbce_		IITBCE
# define	iitbin_		IITBIN
# define	iitbse_		IITBSE
# define	iitbsm_		IITBSM
# define	iitclc_		IITCLC
# define	iitclr_		IITCLR
# define	iitcog_		IITCOG
# define	iitcol_		IITCOL
# define	iitcop_		IITCOP
# define	iitdat_		IITDAT
# define	iitdel_		IITDEL
# define	iitdte_		IITDTE
# define	iitfil_		IITFIL
# define	iithid_		IITHID
# define	iitins_		IITINS
# define	iitrcp_		IITRCP
# define	iitscp_		IITSCP
# define	iitscr_		IITSCR
# define	iitune_		IITUNE
# define	iitunl_		IITUNL
# define	iitval_		IITVAL
# define	iitvlr_		IIRVLR

# include "iiuftb.c"

# else /* MIXEDCASE_FORTRAN */

static int iiuftbL_dummy;

# endif /* MIXEDCASE_FORTRAN */
