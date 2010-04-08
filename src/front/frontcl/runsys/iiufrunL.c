/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include 	<compat.h>
# include	<iiufsys.h>

/*
**
** Name: iiufruntL.c - lower-case version of iiufrunt.c
**
** Description:
**	This file includes iiufrunt.c, renaming all of the global symbols
**	to their lowercase equivalents, in order to support the -lowercase
**	option of the LPI FORTRAN compiler.
**
** History:
**	27-feb-90 (russ)
**		Created.
**	21-apr-91 (teresal)
**		Add new call for activate event.
**      06-feb-1996 (morayf)
**		Added macros to make consistent with U and M files.
*/

# ifdef MIXEDCASE_FORTRAN
# define        iiaccl_ iiaccl
# define        iiactc_ iiactc
# define        iiactf_ iiactf
# define        iiacts_ iiacts
# define        iiaddf_ iiaddf
# define        iichkf_ iichkf
# define        iiclrf_ iiclrf
# define        iiclrs_ iiclrs
# define        iidisp_ iidisp
# define        iiendf_ iiendf
# define        iiendm_ iiendm
# define        iiendn_ iiendn
# define        iienfo_ iienfo
# define        iifgps_ iifgps
# define        iifldc_ iifldc
# define        iifnam_ iifnam
# define        iifomi_ iifomi
# define        iiform_ iiform
# define        iifrae_ iifrae
# define        iifraf_ iifraf
# define        iifrgp_ iifrgp
# define        iifrit_ iifrit
# define        iifrre_ iifrre
# define        iifrsa_ iifrsa
# define        iifrsh_ iifrsh
# define        iifrto_ iifrto
# define        iifset_ iifset
# define        iifsin_ iifsin
# define        iifsqd_ iifsqd
# define        iifsqe_ iifsqe
# define        iifsse_ iifsse
# define        iigetf_ iigetf
# define        iigeto_ iigeto
# define        iigfld_ iigfld
# define        iigtqr_ iigtqr
# define        iihelp_ iihelp
# define        iiinit_ iiinit
# define        iiinqs_ iiinqs
# define        iiiqfs_ iiiqfs
# define        iiiqse_ iiiqse
# define        iimess_ iimess
# define        iimuon_ iimuon
# define        iinest_ iinest
# define        iinfrs_ iinfrs
# define        iinmua_ iinmua
# define        iiprmp_ iiprmp
# define        iiprns_ iiprns
# define        iiputf_ iiputf
# define        iiputo_ iiputo
# define        iiredi_ iiredi
# define        iiresc_ iiresc
# define        iiresf_ iiresf
# define        iiresm_ iiresm
# define        iiresn_ iiresn
# define        iiretv_ iiretv
# define        iirfpa_ iirfpa
# define        iirunf_ iirunf
# define        iirunm_ iirunm
# define        iirunn_ iirunn
# define        iisfpa_ iisfpa
# define        iislee_ iislee
# define        iistfs_ iistfs
# define        iitbca_ iitbca
# define        iivalf_ iivalf

# include "iiufrunt.c"

# else /* MIXEDCASE_FORTRAN */

static int iiufruntL_dummy;

# endif /* MIXEDCASE_FORTRAN */
