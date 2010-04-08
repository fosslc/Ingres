/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include 	<compat.h>
# include	<iiufsys.h>

/*
**
** Name: iiuflibqU.c - upper-case version of iiuflibq.c
**
** Description:
**	This file includes iiuflibq.c, renaming all of the global symbols
**	to their uppercase equivalents, in order to support the -uppercase
**	option of the LPI FORTRAN compiler.
**
** History:
**	27-feb-90 (russ)
**		Created.
**	12-sep-1990 (barbara)
**	    Added new calls for Alerters.
**	15-nov-1990 (barbara)
**	    Added iisete_ to interface (IIseterr) to fix bug #33181.
**      26-feb-1991 (kathryn) Added:
**              IILQSS - IILQssSetSqlio
**              IILQIS - IILQisInqSqlio
**		IILQSH - IILQshSetHandler
**	25-apr-1991 (teresal)
**	    Removed obsolete Event function IILQegEvGetio.
**      09-dec-1991 (rudyw)
**	    Add missing mapping of iiseterr_ to IISETERR so that all 
**	    routines found in iiuflibq.c will be mapped to uppercase.
**	14-oct-1992 (lan)
**		Added IILQled_LoEndData.
**      01-dec-1992 (kathryn)
**              Added  IILQldh_LoDataHandler, IILQlgd_LoGetData
**              and IILQlpd_LoPutData.
**	26-jul-1993 (lan)
**		Added IIG4... for EXEC 4GL.
**	18-aug-1993 (lan)
**		Removed IIG4... (moved them to iiufg4U.c).
*/

# ifdef MIXEDCASE_FORTRAN
# define	iibrea_		IIBREA
# define	iicscl_		IICSCL
# define	iicsde_		IICSDE
# define	iicser_		IICSER
# define	iicsge_		IICSGE
# define	iicsop_		IICSOP
# define	iicsqu_		IICSQU
# define	iicsrd_		IICSRD
# define	iicsre_		IICSRE
# define	iicsrp_		IICSRP
# define	iicsrt_		IICSRT
# define	iieqiq_		IIEQIQ
# define	iieqst_		IIEQST
# define	iierrt_		IIERRT
# define	iiexde_		IIEXDE
# define	iiexex_		IIEXEX
# define	iiexit_		IIEXIT
# define	iiflus_		IIFLUS
# define	iigetd_		IIGETD
# define	iiingo_		IIINGO
# define	iilprs_		IILPRS
# define	iilprv_		IILPRV
# define	iilqpr_		IILQPR
# define	iinexe_		IINEXE
# define	iinext_		IINEXT
# define	iinotr_		IINOTR
# define	iiparr_		IIPARR
# define	iipars_		IIPARS
# define	iiputc_		IIPUTC
# define	iiputd_		IIPUTD
# define	iireti_		IIRETI
# define	iisexe_		IISEXE
# define	iisync_		IISYNC
# define	iitupg_		IITUPG
# define	iiutsy_		IIUTSY
# define	iivars_		IIVARS
# define	iiwrit_		IIWRIT
# define	iixact_		IIXACT
# define	iiserr_		IISERR
# define	iisete_		IISETE
# define	iiseterr_	IISETERR
# define	iilqes_		IILQES
# define	iilqss_		IILQSS
# define	iilqis_		IILQIS
# define	iilqsh_		IILQSH
# define	iilqld_		IILQLD
# define	iilqle_		IILQLE
# define	iilqlg_		IILQLG
# define	iilqlp_		IILQLP

# include "iiuflibq.c"

# else /* MIXEDCASE_FORTRAN */

static int iiuflibqU_dummy;

# endif /* MIXEDCASE_FORTRAN */
