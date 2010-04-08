/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include 	<compat.h>
# include	<iiufsys.h>

/*
**
** Name: iiufrunM.c - mixed-case version of iiufrunt.c
**
** Description:
**	This file includes iiufrunt.c, renaming all of the global symbols
**	to their mixed case equivalents, in order to support the -mixedcase
**	option of the LPI FORTRAN compiler.
**
** History:
**	27-feb-90 (russ)
**		Created.
**	12-sep-90 (barbara)
**	    Added missing external symbols for 6.3 and 6.4.
**	21-apr-1991 (teresal)
**	    Add activate event call IIFRaeAlerterEvent.
**      06-feb-1996 (morayf)
**              Added macros to make consistent with U and L files.
*/

# ifdef MIXEDCASE_FORTRAN
# define	iiaccl_	IIaccl
# define	iiactc_	IIactc
# define	iiactf_	IIactf
# define	iiacts_	IIacts
# define	iiaddf_	IIaddf
# define	iichkf_	IIchkf
# define	iiclrf_	IIclrf
# define	iiclrs_	IIclrs
# define	iidisp_	IIdisp
# define	iiendf_	IIendf
# define	iiendm_	IIendm
# define	iiendn_	IIendn
# define	iienfo_	IIenfo
# define	iifgps_	IIFgps
# define	iifldc_	IIfldc
# define	iifnam_	IIfnam
# define	iifomi_	IIfomi
# define	iiform_	IIform
# define	iifrae_ IIFRae
# define	iifraf_ IIFRaf
# define	iifrgp_	IIFRgp
# define	iifrit_ IIFRit
# define	iifrre_ IIFRre
# define	iifrsa_	IIFRsa
# define	iifrsh_	IIfrsh
# define	iifrto_ IIFRto
# define	iifset_	IIfset
# define	iifsin_	IIfsin
# define	iifsqd_ IIFsqD
# define	iifsqe_	IIFsqE
# define	iifsse_	IIfsse
# define	iigetf_	IIgetf
# define	iigeto_	IIgeto
# define	iigfld_ IIgfld
# define	iigtqr_	IIgtqr
# define	iihelp_	IIhelp
# define	iiinit_	IIinit
# define	iiinqs_	IIinqs
# define	iiiqfs_	IIiqfs
# define	iiiqse_	IIiqse
# define	iimess_	IImess
# define	iimuon_	IImuon
# define	iinest_	IInest
# define	iinfrs_	IInfrs
# define	iinmua_	IInmua
# define	iiprmp_	IIprmp
# define	iiprns_	IIprns
# define	iiputf_	IIputf
# define	iiputo_	IIputo
# define	iiredi_	IIredi
# define	iiresc_	IIresc
# define	iiresf_	IIresf
# define	iiresm_	IIresm
# define	iiresn_	IIresn
# define	iiretv_	IIretv
# define	iirfpa_	IIrfpa
# define	iirunf_	IIrunf
# define	iirunm_	IIrunm
# define	iirunn_	IIrunn
# define	iisfpa_	IIsfpa
# define	iislee_	IIslee
# define	iistfs_	IIstfs
# define	iitbca_ IITBca
# define	iivalf_	IIvalf

# include "iiufrunt.c"

# else /* MIXEDCASE_FORTRAN */

static int iiufruntL_dummy;

# endif /* MIXEDCASE_FORTRAN */
