/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include 	<compat.h>
# include	<iiufsys.h>

/*
**
** Name: iiuftbL.c - lower-case version of iiuftb.c
**
** Description:
**	This file includes iiuftb.c, renaming all of the global symbols
**	to their lowercase equivalents, in order to support the -lowercase
**	option of the LPI FORTRAN compiler.
**
** History:
**	27-feb-90 (russ)
**		Created.
**	12-sep-90 (barbara)
**		Added new call for 6.4
*/

# ifdef MIXEDCASE_FORTRAN
# define	iitbac_		iitbac
# define	iitbce_		iitbce
# define	iitbin_		iitbin
# define	iitbse_		iitbse
# define	iitbsm_		iitbsm
# define	iitclc_		iitclc
# define	iitclr_		iitclr
# define	iitcog_		iitcog
# define	iitcol_		iitcol
# define	iitcop_		iitcop
# define	iitdat_		iitdat
# define	iitdel_		iitdel
# define	iitdte_		iitdte
# define	iitfil_		iitfil
# define	iithid_		iithid
# define	iitins_		iitins
# define	iitrcp_		iitrcp
# define	iitscp_		iitscp
# define	iitscr_		iitscr
# define	iitune_		iitune
# define	iitunl_		iitunl
# define	iitval_		iitval
# define	iitvlr_		iirvlr

# include "iiuftb.c"

# else /* MIXEDCASE_FORTRAN */

static int iiuftbL_dummy;

# endif /* MIXEDCASE_FORTRAN */
