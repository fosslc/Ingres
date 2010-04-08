/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include 	<compat.h>
# include	<iiufsys.h>

/*
**
** Name: iiuflibqL.c - lower-case version of iiuflibq.c
**
** Description:
**	This file includes iiuflibq.c, renaming all of the global symbols
**	to their lowercase equivalents, in order to support the -lowercase
**	option of the LPI FORTRAN compiler.
**
** History:
**	27-feb-90 (russ)
**		Created.
**	14-sep-1990 (barbara)
**		Added new calls for 6.4, etc.
**	15-nov-1990 (barbara)
**		Added iisete_ to the interface (IIseterr) to fix bug #33181.
**      26-feb-1991 (kathryn) Added:
**		iilqss - IILQssSetSqlio
**		iilqis - IILQisInqSqlio
**		iilqsh - IILQshSetHandler
**	25-apr-1991 (teresal)
**		Removed IILQegEvGetio
**      09-dec-1991 (rudyw)
**		Add missing mapping of iiseterr_ to iiseterr so that all 
**		routines found in iiuflibq.c will be mapped to lowercase.
**	14-oct-1992 (lan)
**		Added IILQled_LoEndData.
**	01-dec-1992 (kathryn)
**	  	Added  IILQldh_LoDataHandler, IILQlgd_LoGetData
**		and IILQlpd_LoPutData.
**	26-jul-1993 (lan)
**		Added IIG4... for EXEC 4GL.
**	18-aug-1993 (lan)
**		Removed iig4... (moved them to iiufg4L.c).
*/

# ifdef MIXEDCASE_FORTRAN
# define	iibrea_		iibrea
# define	iicscl_		iicscl
# define	iicsde_		iicsde
# define	iicser_		iicser
# define	iicsge_		iicsge
# define	iicsop_		iicsop
# define	iicsqu_		iicsqu
# define	iicsrd_		iicsrd
# define	iicsre_		iicsre
# define	iicsrp_		iicsrp
# define	iicsrt_		iicsrt
# define	iieqiq_		iieqiq
# define	iieqst_		iieqst
# define	iierrt_		iierrt
# define	iiexde_		iiexde
# define	iiexex_		iiexex
# define	iiexit_		iiexit
# define	iiflus_		iiflus
# define	iigetd_		iigetd
# define	iiingo_		iiingo
# define	iilprs_		iilprs
# define	iilprv_		iilprv
# define	iilqpr_		iilqpr
# define	iinexe_		iinexe
# define	iinext_		iinext
# define	iinotr_		iinotr
# define	iiparr_		iiparr
# define	iipars_		iipars
# define	iiputc_		iiputc
# define	iiputd_		iiputd
# define	iireti_		iireti
# define	iisexe_		iisexe
# define	iisync_		iisync
# define	iitupg_		iitupg
# define	iiutsy_		iiutsy
# define	iivars_		iivars
# define	iiwrit_		iiwrit
# define	iixact_		iixact
# define	iiserr_		iiserr
# define	iisete_		iisete
# define        iiseterr_       iiseterr
# define        iilqes_         iilqes
# define	iilqss_		iilqss
# define	iilqis_		iilqis
# define	iilqsh_		iilqsh
# define	iilqld_		iilqld
# define	iilqle_		iilqle
# define	iilqlg_		iilqlg
# define	iilqlp_		iilqlp

# include "iiuflibq.c"

# else /* MIXEDCASE_FORTRAN */

static int iiuflibqL_dummy;

# endif /* MIXEDCASE_FORTRAN */
