/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include 	<compat.h>
# include	<iiufsys.h>

/*
**
** Name: iiufruntU.c - upper-case version of iiufrunt.c
**
** Description:
**	This file includes iiufrunt.c, renaming all of the global symbols
**	to their uppercase equivalents, in order to support the -uppercase
**	option of the LPI FORTRAN compiler.
**
** History:
**	27-feb-90 (russ)
**		Created.
**	21-apr-91 (teresal)
**		Add new call for activate event.
**      06-feb-1996 (morayf)
**		Added macros to make consistent with L and M files.
*/

# ifdef MIXEDCASE_FORTRAN
# define	iiaccl_	IIACCL
# define	iiactc_	IIACTC
# define	iiactf_	IIACTF
# define	iiacts_	IIACTS
# define	iiaddf_	IIADDF
# define	iichkf_	IICHKF
# define	iiclrf_	IICLRF
# define	iiclrs_	IICLRS
# define	iidisp_	IIDISP
# define	iiendf_	IIENDF
# define	iiendm_	IIENDM
# define	iiendn_	IIENDN
# define	iienfo_	IIENFO
# define	iifgps_ IIFGPS
# define	iifldc_	IIFLDC
# define	iifnam_	IIFNAM
# define	iifomi_	IIFOMI
# define	iiform_	IIFORM
# define        iifrae_ IIFRAE
# define        iifraf_ IIFRAF
# define        iifrgp_ IIFRGP
# define        iifrit_ IIFRIT
# define        iifrre_ IIFRRE
# define        iifrsa_ IIFRSA
# define	iifrsh_	IIFRSH
# define        iifrto_ IIFRTO
# define	iifset_	IIFSET
# define	iifsin_	IIFSIN
# define        iifsqd_ IIFSQD
# define        iifsqe_ IIFSQE
# define	iifsse_	IIFSSE
# define	iigetf_	IIGETF
# define	iigeto_	IIGETO
# define        iigfld_ IIGFLD
# define	iigtqr_	IIGTQR
# define	iihelp_	IIHELP
# define	iiinit_	IIINIT
# define	iiinqs_	IIINQS
# define	iiiqfs_	IIIQFS
# define	iiiqse_	IIIQSE
# define	iimess_	IIMESS
# define	iimuon_	IIMUON
# define	iinest_	IINEST
# define	iinfrs_	IINFRS
# define	iinmua_	IINMUA
# define	iiprmp_	IIPRMP
# define	iiprns_	IIPRNS
# define	iiputf_	IIPUTF
# define	iiputo_	IIPUTO
# define	iiredi_	IIREDI
# define	iiresc_	IIRESC
# define	iiresf_	IIRESF
# define	iiresm_	IIRESM
# define	iiresn_	IIRESN
# define	iiretv_	IIRETV
# define	iirfpa_	IIRFPA
# define	iirunf_	IIRUNF
# define	iirunm_	IIRUNM
# define	iirunn_	IIRUNN
# define	iisfpa_	IISFPA
# define	iislee_	IISLEE
# define	iistfs_	IISTFS
# define        iitbca_ IITBCA
# define	iivalf_	IIVALF

# include "iiufrunt.c"

# else /* MIXEDCASE_FORTRAN */

static int iiufruntL_dummy;

# endif /* MIXEDCASE_FORTRAN */
