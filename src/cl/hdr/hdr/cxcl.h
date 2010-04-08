/*
** Copyright (c) 2001, 2006 Ingres Corporation
**
*/

/**CL_SPEC
** Name: CXCL.H - CX compatibility library definitions.
**
** Description:
**      This header contains those declarations which may vary
**	from platform to platform, but which need to be referenced
**	by cx.h which defines the public interface to CX.
**
** Specification:
**  
**  Description:
**      The CX ('C'luster e'X'tension) sub-facility provides an OS 
**	independent interface for services needed to support Ingres 
**	clusters.   
**
**  Intended Uses:
**	The CX routines are mainly used by the LG & LK facilities
**	in direct support of Ingres clusters.
**      
**  Assumptions:
**	Caller should not assume that this facility will be fully functional 
**	on all platforms.  In those environments in which clustering is not 
**	supported or configured, many CX routines return a status of
**	E_CL2C10_CX_E_NOSUPPORT, and calling programs must allow for this.
**
**  Definitions:
**
**  Concepts:
**	
**	Please see design Spec for SIR 103715.
**
** History:
**      06-feb-2001 (devjo01)
**          Created.
**	29-may-2002 (devjo01)
**	    Add "extended" (long) message functions.
**	12-sep-2002 (devjo01)
**	    Add CX_MSG_CHIT_MASK.
**	23-oct-2003 (kinte01)
**	    Move include of clconfig.h to axp_osf specific area as it is 
**	    only required for tru64
**	08-jan-2004 (devjo01)
**	    Most of the contents of this file split off into new header
**	    !cl!clf!hdr cxprivate.h, which now holds all declarations private
**	    to the CX facility.  Residual contents of this header are
**	    limited to those declarations which are platform specific,
**	    and need to be referenced by the public interface to CX (cx.h).
**	01-Apr-2004 (devjo01)
**	    Move CX_MAX_MSG_STOW, CX_MSG_FRAGSIZE to cxprivate.h
**	28-Apr-2004 (devjo01)
**	    Add CX_DLM_VERSION for Linux, increase LVB size to 32 for
**	    Linux to match new OpenDLM size.
**	30-Apr-2004 (devjo01)
**	    Add cx_dlm_orphan_site, cx_dlm_orphan_pid to CX_DLM_CB to
**	    match changes introduced to OpenDLM struct lockstatus by
**	    to open source community.  CX_DLM_CB must match struct
**	    lockstatus.  This change and the previous allow us to
**	    use version 0.2 of OpenDLM.
**      14-Sep-2004 (devjo01)
**          Add CX_CMO_SCN.  Bump version to 0.3.
**	02-nov-2004 (devjo01)
**	    Bump version to 0.4 in support for OpenDLM recovery domains.
**	22-apr-2005 (devjo01)
**	    VMS port submission.
**	13-may-2005 (devjo01)
**	    Correct errors in CX_ID_EQUAL macros.
**	    Add cx_dlm_posted to VMS version of CX_DLM_WS.
**	    Revert VMS LVB size to 16.
**      24-jul-2006 (abbjo03)
**          Changed CX_LOCK_ID to II_VMS_LOCK_ID on VMS.
**	17-May-2007 (jonj)
**	    Add CXDLM_IS_POSTED macros.
**/


#ifndef	CXCL_H_INCLUDED
#define CXCL_H_INCLUDED
#ifdef VMS
#include <vmstypes.h>
#endif

/*
**  Symbolic constants
*/

/* CMO implementation "styles" */
#define	CX_CMO_NONE   		0	/* None or uninitialized.   */
#define	CX_CMO_DLM		1	/* Use DLM value blks (all) */
#define	CX_CMO_IMC		2	/* Use memory channel (axp) */
#define	CX_CMO_SCN		3	/* Use sys commit# (opendlm)*/

#define CX_MSG_MAX_CHIT	      256	/* High range, and # of, valid chits */
#define CX_MSG_CHIT_MASK      0x1FF	/* Used to mask out base chit id */

#define CX_ALIGN_BOUNDRY	sizeof(PTR) /* Alignment requirements */

/*
** Implementation dependent declares for CX <-> DLM
*/

/*
**  Lock value block size used by underlying DLM
**
**  This must be equal to or greater than underlying DLM value block size.
**  Ingres itself will only use 8 bytes for data, plus 4 for keeping the
**  valid/invalid status from Ingres'es perspective.  This is less
**  than the minimun value block size for supported DLM's (16).
*/
#if defined(axp_osf) || defined(LNX)
#	define CX_DLM_VALUE_SIZE       32
#else
#	define CX_DLM_VALUE_SIZE       16
#endif

typedef u_i4    CX_VALUE[CX_DLM_VALUE_SIZE/sizeof(u_i4)];

/*
**  "Private" OS specific implementation portion of CX_REQ_CB.
*/
typedef struct _CX_DLM_CB CX_DLM_CB;

#if defined(VMS)	/* All flavors of VMS - _VMS_ */

typedef II_VMS_LOCK_ID CX_LOCK_ID;

#define CX_ID_EQUAL(a,b) \
	( (*(i4 *)(a)) == (*(i4 *)(b)) )
#define CX_NONZERO_ID(p) \
	( 0 != *(i4 *)(p) )
#define CX_ZERO_OUT_ID(p) \
	( *(i4 *)(p) = 0 )
#define CX_EXTRACT_LOCK_ID(d,s) \
	( ((LK_UNIQUE*)(d))->lk_uhigh = 0, \
          ((LK_UNIQUE*)(d))->lk_ulow = *(i4 *)(s) )

#if defined(conf_CLUSTER_BUILD)
#define CXDLM_IS_POSTED(preq) \
	CS_ISSET(&(preq)->cx_dlm_ws.cx_dlm_posted)
#else /* Not cluster build */
#define CXDLM_IS_POSTED(preq) FALSE
#endif
	

/* Layout of following must match LKSB passed to SYS$ENQ */
struct _CX_DLM_CB
{
	u_i2		cx_dlm_status;
	u_i2		cx_dlm_reserved;
	CX_LOCK_ID	cx_dlm_lock_id;
	CX_VALUE	cx_dlm_valblk;
	CS_ASET		cx_dlm_posted;
};

/* end - _VMS_ */

#elif defined(LNX)	/* All flavors of Linux */

typedef int	 CX_LOCK_ID;

#define CX_ID_EQUAL(a,b) \
	( (*(int *)(a)) == (*(int *)(b)) )
#define CX_NONZERO_ID(p) \
	( 0 != *(int *)(p) )
#define CX_ZERO_OUT_ID(p) \
	( *(int *)(p) = 0 )
#define CX_EXTRACT_LOCK_ID(d,s) \
	( ((LK_UNIQUE*)(d))->lk_uhigh = 0, \
          ((LK_UNIQUE*)(d))->lk_ulow = *(int *)(s) )

#define CXDLM_IS_POSTED(preq) FALSE

/* Layout of following must match LKSB passed to OpenDLM */
struct _CX_DLM_CB
{
	int		cx_dlm_status;
	CX_LOCK_ID	cx_dlm_lock_id;
	CX_VALUE	cx_dlm_valblk;
	int		cx_dlm_orphan_site;
	int		cx_dlm_orphan_pid;
	unsigned int    cx_dlm_timeout;
};

#define CX_DLM_VERSION	"0.4"

#else	/* Generic implementation */

typedef LK_UNIQUE CX_LOCK_ID;

#define CX_ID_EQUAL(a,b) \
	( (((LK_UNIQUE*)(a))->lk_ulow == ((LK_UNIQUE*)(b))->lk_ulow) && \
	(((LK_UNIQUE*)(a))->lk_uhigh == ((LK_UNIQUE*)(b))->lk_uhigh) )
#define CX_NONZERO_ID(p) \
	( ((LK_UNIQUE*)(p))->lk_uhigh || ((LK_UNIQUE*)(p))->lk_ulow )
#define CX_ZERO_OUT_ID(p) \
	( ((LK_UNIQUE*)(p))->lk_uhigh = ((LK_UNIQUE*)(p))->lk_ulow = 0 )
#define CX_EXTRACT_LOCK_ID(d,s) \
	( *(LK_UNIQUE*)(d) = *(LK_UNIQUE*)(s) )

#define CXDLM_IS_POSTED(preq) FALSE

struct _CX_DLM_CB
{
	CX_LOCK_ID	cx_dlm_lock_id;
	CX_VALUE	cx_dlm_valblk;
};

#endif

#endif /* CXCL_H_INCLUDED */
