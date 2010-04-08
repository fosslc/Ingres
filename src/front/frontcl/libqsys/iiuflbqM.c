/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include 	<compat.h>
# include	<iiufsys.h>

/*
**
** Name: iiuflibqM.c - mixed-case version of iiuflibq.c
**
** Description:
**	This file includes iiuflibq.c, renaming all of the global symbols
**	to their mixed case equivalents, in order to support the -mixedcase
**	option of the LPI FORTRAN compiler.
**
** History:
**	27-feb-90 (russ)
**		Created.
**	12-sep-1990 (barbara)
**	    Added new calls for alertters.
**	15-nov-1990 (barbara)
**	    Added iisete_ to the interface (IIseterr) to fix bug #33181.
**      26-feb-1991 (kathryn) Added:
**              IILQss - IILQssSetSqlio
**              IILQis - IILQisInqSqlio
**		IILQsh - IILQshSetHandler
**	26-apr-1991 (teresal)
**	    Removed obsolete Event function IILQegEvGetio.	
**      09-dec-1991 (rudyw)
**	    Add missing mapping of iiseterr_ to IIseterr so that all 
**	    routines found in iiuflibq.c will be mapped to mixedcase.
**      22-jul-1992 (rudyw)
**          Define EXCLUDE_IISETERR in this mixedcase file so that iiseterr_
**          need not be remapped or its code pulled into this file. Any
**          application using IIseterr will load the real routine directly.
**	14-oct-1992 (lan)
**		Added IILQled_LoEndData.
**      01-dec-1992 (kathryn)
**              Added  IILQldh_LoDataHandler, IILQlgd_LoGetData
**              and IILQlpd_LoPutData.
**	26-jul-1993 (lan)
**		Added IIG4... for EXEC 4GL.
**	18-aug-1993 (lan)
**		Removed IIG4... (moved them to iiufg4M.c).
*/

# ifdef MIXEDCASE_FORTRAN
# define	iibrea_		IIbrea
# define	iicscl_		IIcsCl
# define	iicsde_		IIcsDe
# define	iicser_		IIcsER
# define	iicsge_		IIcsGe
# define	iicsop_		IIcsOp
# define	iicsqu_		IIcsQu
# define	iicsrd_		IIcsRd
# define	iicsre_		IIcsRe
# define	iicsrp_		IIcsRp
# define	iicsrt_		IIcsRt
# define	iieqiq_		IIeqiq
# define	iieqst_		IIeqst
# define	iierrt_		IIerrt
# define	iiexde_		IIexDe
# define	iiexex_		IIexEx
# define	iiexit_		IIExit
# define	iiflus_		IIflus
# define	iigetd_		IIgetd
# define	iiingo_		IIingo
# define	iilprs_		IILprs
# define	iilprv_		IILprv
# define	iilqpr_		IILQpr
# define	iinexe_		IInexe
# define	iinext_		IInext
# define	iinotr_		IInotr
# define	iiparr_		IIparr
# define	iipars_		IIpars
# define	iiputc_		IIputc
# define	iiputd_		IIputd
# define	iireti_		IIreti
# define	iisexe_		IIsexe
# define	iisync_		IIsync
# define	iitupg_		IItupg
# define	iiutsy_		IIutsy
# define	iivars_		IIvars
# define	iiwrit_		IIwrit
# define	iixact_		IIXact
# define	iiserr_		IIserr
# define	iisete_		IIsete
# define	iilqes_		IILQes
# define	iilqss_		IILQss
# define	iilqis_		IILQis
# define	iilqsh_		IILQsh
# define	iilqld_		IILQld
# define	iilqle_		IILQle
# define	iilqlg_		IILQlg
# define	iilqlp_		IILQlp

# define        EXCLUDE_IISETERR

# include "iiuflibq.c"

# else /* MIXEDCASE_FORTRAN */

static int iiuflibqM_dummy;

# endif /* MIXEDCASE_FORTRAN */
