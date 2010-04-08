/*
** Copyright (c) 2001, 2007 Ingres Corporation
*/

/**CL_SPEC
** Name: CXPRIVATE.H - CX compatibility library definitions.
**
** Description:
**      This header contains the description of the types, constants and
**      function used privately by the CX facility.
**
** Specification:
**  
**  Description:
**      The CX ('C'luster e'X'tension) sub-facility provides an OS 
**	independent interface for services needed to support Ingres 
**	clusters.   
**
**	This includes functions	for support of a "Distributed lock manager" 
**	(DLM), plus 'C'luster 'M'emory 'O'bjects (CMOs), and low level
**	node management and cross cluster IPC support routines.
**	Cross cluster IPC inter process communication has two styles.
**	One allows passing of very short messages with the guarantee
**	that when you return from the send routine, all interested
**	parties will have seen and had a chance to act on your message,
**	and another which allows large messages to be left to be read
**	later by interested parties who redeem a "chit" sent to them
**	via another means (e.g. the short message facility).  This
**	alternate message facility is called the 'C'luster 'L'ong
**	'M'essage facility (CLM).
** 
**  Intended Uses:
**	The CX routines are mainly used by the LG & LK facilities
**	in direct support of Ingres clusters.  LK.H must be included
**	before this file.
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
**	08-jan-2004 (devjo01)
**	    Bulk of cxcl.h split off to here.  
**	    Please see cxcl.h for earlier history.
**	01-apr-2004 (devjo01)
**	    Add DLM_PURGE, cx_clm_type.  Remove Tru64 5.0a build stuff.
**	    Move CX_LKS_PER_CMO, CX_MSG_FRAGSIZE here from cxcl.h;
**	    CX_MAX_MSG_STOW  here from cx.h.  Add new fields in CX_PROC_CB
**	    and CX_NODE_CB for support of CX_CLM_FILE.
**	    Add cx_msg_clm_file_init() prototype.
**	28-apr-2004 (devjo01)
**	    Add CX_TRACE_MESSAGE, CX_REPORT_STATUS macros.
**	14-may-2004 (devjo01)
**	    Add CX_PF_QUIET and CX_NOISY_OUTPUT
**	14-Jul-2004 (hanje04)
**	    Make inclusion of dml.h dependent on conf_CLUSTER_BUILD as
**	    well as int_lnx.
**      10-Sep-2004 (devjo01)
**          Change assumed location of OpenDLM header files to be
**          in a subdirectory named opendlm off the default system
**          header location. (e.g. /usr/include/opendlm).
**      20-Sep-2004 (devjo01)
**	    Cumulative changes for Linux beta.
**          - Add DLM_SCNOP to function vectors defined for OpenDLM (LNX).
**	    - Add info to CX_NODE_CB for CX_LSN support.
**	    - Add cx_lkkey_fmt_func, cx_error_log_func.
**      02-nov-2004 (devjo01)
**          Support for OpenDLM recovery domains.
**	08-feb-2005 (devjo01)
**	    Add CX_VALID_FUNC_PTR in support of another paranoia check.
**	12-apr-2005 (devjo01)
**	    Rename CX_MSG_FUNC to CX_MSG_RCV_FUNC.
**	22-apr-2005 (devjo01)
**	    VMS porting submission.
**	09-may-2005 (devjo01)
**	    Distributed deadlock support (sir 114504/114136).
**	    - Add flag CX_PF_DODEADLOCK to CX_PROC_CB.cx_flags.
**	    - Add cx_ni, cxvms_bli_sem, cxvms_bli_buf, and cxvms_bli_size
**	      to CX_PROC_CB.
**	    - Add cxvms_csids to CX_NODE_CB.
**	21-jun-2005 (devjo01)
**	    Address GLC problems with shared cache and/or processes with
**          UIC's whose group does not match that of CSP.
**	    - Add macros to convert between offset & ptrs within CX shared mem.
**	    - Add cx_proxy_offset, cx_transactions_required, cx_locks_required,
**	      cx_resources_required and cxvms_uic_group to CX_PROC_CB.
**	    - Add cxvms_csp_glc, cxvms_glcs, cxvms_glm_free, and 
**	      cxvms_csp_uic_group to CX_NODE_CB.
**	    - Add CX_GLC_MSG, CX_GLC_CTX structs.
**	    - Add jpidef.h to VMS specific headers to include.
**	    - Remove unneeded Lock key buffers, as cx_key now holds the
**	      key rather than the address of a key.
**	    - Add prototype for cx_find_glc.
**      30-Jun-2005 (hweho01)
**          Make inclusion of header files dependent on conf_CLUSTER_BUILD 
**          for axp_osf platform.
**       6-Nov-2006 (hanal04) SIR 117044
**          Add int.rpl for Intel Rpath Linux build.
**	13-jul-2007 (joea)
**	    Use two buffers for DLM lock info on VMS.
*/



/*
**      OS specific headers needed for OS specific declarations.
*/
#if defined(axp_osf) && defined(conf_CLUSTER_BUILD)

# ifndef CLCONFIG_H_INCLUDED
# include "clconfig.h"
# endif
	/* Tru64 specific header files */
#ifndef __C_ASM_H
#include <c_asm.h>
#endif
#ifndef _SYS_TYPES_H_
#include <sys/types.h>
#endif
#ifndef _DLM_H
#include <sys/dlm.h>
#endif
#ifndef IMC_H_INCLUDED
#include <sys/imc.h>
#endif
#ifndef _SYS_CLU_H_
#include <sys/clu.h>
#endif
#ifndef _CLUSTER_DEFS_H_
#include <sys/cluster_defs.h>
#endif
#ifndef _SYS_SIGNAL_H_
#include <signal.h>
#endif

#ifdef xCL_095_USE_LOCAL_NUMA_HEADERS
# error "No longer support building under Tru64 5.0a"
#else
#include <numa.h>
#endif

/* end axp_osf */

#elif (defined(int_lnx) || defined(int_rpl)) && defined(conf_CLUSTER_BUILD)
/*
** Linux/OpenDLM header files
*/
#include <opendlm/dlm.h>

#elif defined(VMS) /* _VMS_ */
/*
** OpenVMS header files needed by CX
*/
#include    <starlet.h>
#include    <ssdef.h>
#include    <lckdef.h>
#include    <lkidef.h>
#include    <iosbdef.h>
#include    <descrip.h>
#include    <psldef.h>
#include    <jpidef.h>
#include    <prvdef.h>
#include    <syidef.h>

#ifndef SS$_XVALNOTVALID	/* Building on 7.3, but need 8.2 error code. */
# define SS$_XVALNOTVALID	2984
#endif
#include    <vmstypes.h>

/* end _VMS_ */
#endif	/* OS specific header files */


/*
**  Symbolic constants
*/

/* Default signal used by underlying DLM */
#if defined(axp_osf)
#define CX_DLM_DFLT_SIGNAL        SIGIO
#elif defined(int_lnx) || defined(int_rpl)
#define CX_DLM_DFLT_SIGNAL        SIGUSR1
#else
#define CX_DLM_DFLT_SIGNAL	  0
#endif

#if defined(LNX)
#define CX_CMO_DEFAULT          CX_CMO_SCN
#else
#define CX_CMO_DEFAULT          CX_CMO_DLM
#endif

#if	defined(axp_osf)

	/* Tru64 specific symbolic constants */
#define CXAXP_IMC_DFLT_RAIL		 0
#define CXAXP_CMO_IMC_SPINCHK	    (1<<10)
#define CXAXP_CMO_IMC_SPINMAX	    (1<<20)
#define MB() asm("mb")

        /*axp_osf*/
#elif	defined(VMS) /* _VMS_ */

#else
	/* Other OS specific symbolic constants */
#endif	/*axp_osf*/

#define CX_LKS_PER_CMO		4	/* # DLM locks per CMO */

/* Constants for IMC CLM (Cluster Long Message) implementation. */
#define CX_MSG_FRAGSIZE		512	/* Allocation unit for long msgs.*/
#define CX_MAX_MSG_STOW  ((31*512)-sizeof(i4)) /* Max size of any "long" msg */
#define CX_CLM_SIZE_SCALE 65536 /* Granularity for CLM allocation (64K) */

/*
**	Macros
*/
#define CX_UNALIGNED_PTR(p)	((long)(p) & (CX_ALIGN_BOUNDRY-1))

/*
** This pair of macros is used to convert pointers to from & offset values
** in the CX portion of the LGK shared memory segment.  Since CX does
** not "know" about LK, the offsets are based on the beginning of its
** chunk of memory, rather than the base of the whole segment.  In
** addition, since the CX portion of the memory is relatively small,
** we explicitly use 32bit offsets to minimize memory requirements, and
** to allow the use of CScas4 when manipulating these values.
*/
#define CX_PTR_TO_OFFSET(p) \
    (((char*)(p)) - (char*)(CX_Proc_CB.cx_ncb))
#define CX_OFFSET_TO_PTR(o) \
    (((char*)(CX_Proc_CB.cx_ncb)) + (o))

#define CX_TRACE_MESSAGE(fmt,param) \
    TRdisplay( "%@ %s:%d " fmt "\n", __FILE__, __LINE__, param )

#define CX_REPORT_STATUS(s) \
CX_TRACE_MESSAGE( "Status=0x%x", (s) )

#define REPORT_BAD_STATUS(s) \
if (OK != (s)) { CX_REPORT_STATUS(s); }

#define BREAK_ON_BAD_STATUS(s) \
if (OK != (s)) { CX_REPORT_STATUS(s); break; }

#define CX_NOISY_OUTPUT \
if ( !(CX_Proc_CB.cx_flags & CX_PF_QUIET) ) TRdisplay

#if defined(int_lnx) || defined(int_rpl)
# define CX_VALID_FUNC_PTR(p) ((0x00FFFFFF & *(long*)p) == 0x00E58955)
#else
# define CX_VALID_FUNC_PTR(p) 1
#endif

/*
**      Typedefs
*/

typedef	struct	_CX_PROC_CB	CX_PROC_CB;
typedef	struct	_CX_NODE_CB	CX_NODE_CB;
typedef struct  _CX_CMO_DLM_CB	CX_CMO_DLM_CB;
typedef struct  _CX_MSG_CB	CX_MSG_CB;
typedef struct  _CX_MM_CB	CX_MM_CB;
typedef struct  _CX_MSG_HEAD	CX_MSG_HEAD;
typedef union   _CX_MSG_FRAG	CX_MSG_FRAG;
typedef struct  _CX_GLC_MSG	CX_GLC_MSG;
typedef struct  _CX_GLC_CTX	CX_GLC_CTX;

/*
**	Implementation specific typedefs
*/
#ifdef  axp_osf
	/* Tru64 specific typedefs */
typedef	struct _CXAXP_CMO_IMC_CB CXAXP_CMO_IMC_CB;
#endif

/*}
** Name: CX_OFFSET - Offset relative to CX_NODE_CB
**
** Description:
**	This is used to declare structure elements and variables
**	which hold a memory offset relative to the start of the
**	CX shared memory area (Which starts with the CX_NODE_CB).
**
**	This is declared as an i4 rather than a SIZE_TYPE or
**	ptrdiff_t, since we never anticipat going anywhere near
**	requiring gigabytes of CX memory, so a four byte offset
**	is more than sufficient, and conserves space.
**
** History:
**      21-jun-2005 (devjo01)
**	    Created.
*/
typedef i4	CX_OFFSET;

/*}
** Name: CX_GLC_MSG - Used to pass a message re a group lock in VMS.
**
** Description:
**      This structure is used to pass a message for those lock
**	operations which a done in a simulated group lock container.
**	Since VMS has no GLC, if a process needs to operate on
**	a lock owned by another member of the GLC, he has to pass
**	the lock request off to the actual owner.
**
** History:
**      21-jun-2005 (devjo01)
**	    Created.
**	09-May-2008 (jonj)
**	    Add cx_glm_instance.
*/
struct _CX_GLC_MSG
{
    volatile CX_OFFSET	cx_glm_next;	/* Offset next message or 0 */
    i4		cx_glm_opcode;		/* operation to perform. */
# define CX_DLM_OP_RQST		0
# define CX_DLM_OP_CONV		1
# define CX_DLM_OP_RLSE		2
# define CX_DLM_OP_CANC		3
    u_i4	cx_glm_instance;	/* Instantiation of CX_REQ_CB */
    i4		cx_glm_flags;		/* Flags passed to orig req. */
    SIZE_TYPE	cx_glm_req_off;		/* Offset to CX_REQ_CB */
    CS_CPID	cx_glm_caller;		/* Process/session to resume */
}; /*CX_GLC_MSG*/

/*}
** Name: CX_GLC_CTX - Used to anchor GLC information for a process.
**
** Description:
**      This structure holds the connection context for one partner
**	sharing the GLC.
**
** History:
**      21-jun-2005 (devjo01)
**	    Created.
**	04-Apr-2008 (jon)
**	    Replaced cx_glc_ready with cx_glc_triggered.
*/
struct _CX_GLC_CTX
{
    CS_CPID		cx_glc_threadid; /* Id of owning session */
    CS_ASET		cx_glc_valid;	 /* Asserted if CTX in use. */
    i4			cx_glc_triggered; /* Zero if CTX needs resume */
    volatile i4		cx_glc_fcnt;	 /* Loose count of free local GLMs */
# define CX_MAX_GLC_LOCAL_CACHE	10
    volatile CX_OFFSET	cx_glc_free;	 /* Start of local free GLM list */
    volatile CX_OFFSET	cx_glc_glms;	 /* Start of pending GLM list. */
}; /*CX_GLC_CTX*/


/*}
** Name: CX_PROC_CB - CX Process Control Block
**
** Description:
**      This structure contains all the information used on a per process
**	basis by CX facility clients.   It also contains redundant copies
**	of some information common with CX_NODE_CB, both for purposes of
**	determining how far initialization has progressed, and to avoid
**	shared memory access where practical.   Structure contains 
**	generic information which is used on every platform, as well 
**	as OS specific fields.   OS specific fields will be named 
**	in the format cx<ostag>_<fld_suffix>, while generic fields
**	will have the format cx_<fld_suffix>. 
**
**	Macros for dynamically loaded functions will have the same name
**	as the underlying function, except all in uppercase, except if
**	there is a conflict with a vendor provided symbol, in which case
**	"_f" is appended to the standard name.
**
** History:
**      06-Feb-2001 (devjo01)
**	    Created from specification for SIR 103715.
**	09-may-2005 (devjo01)
**	    Add cx_ni, cxvms_bli_sem, cxvms_bli_buf, and cxvms_bli_size
**	13-jul-2007 (joea)
**	    Use two DLM buffers on VMS.
*/
struct _CX_PROC_CB 
{
    CX_NODE_CB		*cx_ncb;	/* Pointer to NCB in shared memory */
    i4			 cx_state;	/* Current state of CX this process */
    i4			 cx_flags;	/* Process staus flags */
#   define CX_PF_ENABLE_FLAG		(1<<0) /* Set if cluster enabled. */
#   define CX_PF_ENABLE_FLAG_VALID	(1<<1) /* Set if cluster flag valid */
#   define CX_PF_IS_CSP  CX_F_IS_CSP	       /* (1<<2) */
#   define CX_PF_NODENUM_VALID		(1<<3) /* Set if node # valid. */
#   define CX_PF_FIRSTCSP		(1<<4) /* Set if "First CSP" */
#   define CX_PF_MASTERCSP		(1<<5) /* Set if Master CSP */
#   define CX_PF_NEED_CSP		(1<<6) /* Set if can't run w/o CSP */
#   define CX_PF_HAVENUMA_FLAG		(1<<7) /* Set if NUMA supported */
#   define CX_PF_HAVENUMA_FLAG_VALID	(1<<8) /* Set if NUMA flag valid. */
#   define CX_PF_USENUMA_FLAG		(1<<9) /* Set if NUMA in use */
#   define CX_PF_USENUMA_FLAG_VALID	(1<<10)/* " " NUMA in use flag OK. */
#   define CX_PF_USENUMACLUST_FLAG	(1<<11)/* " " NUMA clustering */
#   define CX_PF_USENUMACLUST_FLAG_VALID (1<<12)/*" " NUMA clust. flag OK. */
#   define CX_PF_DLM_TRACE CX_F_DLM_TRACE      /* (1<<13) DLM trace on/off */
#   define CX_PF_CMO_TRACE CX_F_CMO_TRACE      /* (1<<14) CMO trace on/off */
#   define CX_PF_MSG_TRACE CX_F_MSG_TRACE      /* (1<<15) MSG trace on/off */
#   define CX_PF_QUIET			(1<<16)/* No CX_NOISY_OUTPUT */
#   define CX_PF_ONE_NODE_OK		(1<<17)/* On VMS 1-node "cluster" OK */
#   define CX_PF_DO_DEADLOCK_CHK	(1<<18)/* Do own deadlock checks */
    i4			 cx_cmo_type;	/* Style of CMO implentation */
    i4			 cx_clm_type;	/* Style of CLM implentation */
#   define CX_CLM_NONE			0 /* No cluster long message support */
#   define CX_CLM_FILE			1 /* Transfer through shared files */
#   define CX_CLM_IMC			2 /* Use IMC (Tru64 only). */
    i4			 cx_clm_blocks;	/* Number of CLM alloc units */
    i4			 cx_clm_blksize;/* Granularity of CLM alloc units */
    i4			 cx_node_num;	/* Node number for this node */
    i4			 cx_numa_user_rad; /* Affinity RAD ID */
    i4			 cx_numa_cluster_rad;/* NUMA cluster RAD ID */
    i4			 cx_imc_rail;	/* MC "rail" to use. */
    i4			 cx_lk_nodekey; /* Bits added to lock key to
				keep nodes as separate scopes. */
    PID			 cx_pid;	/* Process ID this process. */
    CX_REQ_CB		 cx_csp_req;	/* DLM CB for CSP lock (CSP only) */
    CX_REQ_CB		 cx_cfg_req;	/* DLM CB for config lock (CSP only) */
    char *(*cx_lkkey_fmt_func)( LK_LOCK_KEY *, char * );
    void (*cx_error_log_func)( i4, PTR, i4, ... );
    CX_CONFIGURATION    *cx_ni;         /* Info about the Ingres Node cfg. */
    i4			 cx_proxy_offset;/* Offset to proxy CS_CPID */
    i4			 cx_transactions_required;
    i4			 cx_locks_required;
    i4			 cx_resources_required;
#if defined(axp_osf) &&  defined(conf_CLUSTER_BUILD)
/*
**      Tru64 implementation
*/
    /* DLM related elements */
    i4			 cxaxp_lk_instkey; /* Bits to add to lock key to
				force installations to have separate scope */
    dlm_nsp_t		 cxaxp_nsp;	/* Name space handle */
    dlm_glc_id_t	 cxaxp_glc_id;	/* Set to SMS GLC if attached, else 0 */
    dlm_rd_id_t		 cxaxp_prc_rd;	/* Process recovery domain */
    dlm_rd_collection_hndl_t cxaxp_prc_rdh; /* Handle to process recovery set*/
    dlm_rd_id_t		 cxaxp_grp_rd;	/* Group recovery domain */
    dlm_rd_collection_hndl_t cxaxp_grp_rdh; /* Handle to group recovery set. */
    void		*cxaxp_dlm_lib_hndl; /* Handle to DLM library */
    void		*cxaxp_dlm_funcs[19]; /* Dynamically loaded DLM funcs.*/
#   define	DLM_NSJOIN \
	((dlm_status_t (*)(dlm_nsid_t, dlm_nsp_t *, dlm_flags_t)) \
	CX_Proc_CB.cxaxp_dlm_funcs[0])
#   define	DLM_NSLEAVE \
	((dlm_status_t (*)(dlm_nsp_t *, dlm_flags_t)) \
	CX_Proc_CB.cxaxp_dlm_funcs[1])
#   define	DLM_SET_SIGNAL \
	((dlm_status_t (*)(int, int)) \
	CX_Proc_CB.cxaxp_dlm_funcs[2])
#   define	DLM_RD_ATTACH \
	((dlm_status_t (*)(char *, dlm_rd_id_t *, dlm_rd_flags_t)) \
	CX_Proc_CB.cxaxp_dlm_funcs[3])
#   define	DLM_RD_DETACH \
	((dlm_status_t (*)(dlm_rd_id_t,dlm_rd_flags_t)) \
	CX_Proc_CB.cxaxp_dlm_funcs[4])
#   define	DLM_RD_COLLECT \
	((dlm_status_t (*)(dlm_rd_id_t,dlm_rd_collection_hndl_t *, \
	dlm_rd_flags_t)) CX_Proc_CB.cxaxp_dlm_funcs[5])
#   define	DLM_RD_VALIDATE \
	((dlm_status_t (*)(dlm_rd_id_t,dlm_rd_collection_hndl_t *, \
        dlm_rd_flags_t)) CX_Proc_CB.cxaxp_dlm_funcs[6])
#   define	DLM_GLC_CREATE \
	((dlm_status_t (*)(dlm_glc_id_t *, dlm_glc_flags_t, dlm_nsp_t *, \
	int)) CX_Proc_CB.cxaxp_dlm_funcs[7])
#   define	DLM_GLC_DESTROY \
	((dlm_status_t (*)(dlm_glc_id_t, dlm_glc_flags_t)) \
	CX_Proc_CB.cxaxp_dlm_funcs[8])
#   define	DLM_GLC_ATTACH \
	((dlm_status_t (*)(dlm_glc_id_t, dlm_glc_flags_t)) \
	CX_Proc_CB.cxaxp_dlm_funcs[9])
#   define	DLM_GLC_DETACH \
	((dlm_status_t (*)(void)) \
	CX_Proc_CB.cxaxp_dlm_funcs[10])
#   define	DLM_LOCKTP \
	((dlm_status_t (*)(dlm_nsp_t, uchar_t *, ushort_t, dlm_lkid_t *, \
	dlm_lkid_t *, dlm_lkmode_t, dlm_valb_t *, dlm_flags_t, \
	callback_arg_t, callback_arg_t, blk_callback_t, uint_t, \
	dlm_trans_id_t, dlm_rd_id_t)) CX_Proc_CB.cxaxp_dlm_funcs[11])
#   define	DLM_QUELOCKTP \
	((dlm_status_t (*)(dlm_nsp_t, uchar_t *, ushort_t, dlm_lkid_t *, \
	dlm_lkid_t *, dlm_lkmode_t, dlm_valb_t *, dlm_flags_t, \
	callback_arg_t, callback_arg_t, blk_callback_t, uint_t, \
	cpl_callback_t, dlm_trans_id_t, dlm_rd_id_t)) \
	CX_Proc_CB.cxaxp_dlm_funcs[12])
#   define	DLM_CVT \
	((dlm_status_t (*)(dlm_lkid_t *, dlm_lkmode_t, dlm_valb_t *, \
        dlm_flags_t, callback_arg_t, callback_arg_t, blk_callback_t, \
        uint_t)) CX_Proc_CB.cxaxp_dlm_funcs[13])
#   define	DLM_QUECVT_f \
	((dlm_status_t (*)(dlm_lkid_t *, dlm_lkmode_t, dlm_valb_t *, \
        dlm_flags_t, callback_arg_t, callback_arg_t, blk_callback_t, \
        uint_t,cpl_callback_t)) CX_Proc_CB.cxaxp_dlm_funcs[14])
#   define	DLM_UNLOCK \
	((dlm_status_t (*)(dlm_lkid_t *, dlm_valb_t *, dlm_flags_t)) \
	CX_Proc_CB.cxaxp_dlm_funcs[15])
#   define	DLM_CANCEL_f \
	((dlm_status_t (*)(dlm_lkid_t *, dlm_flags_t)) \
	CX_Proc_CB.cxaxp_dlm_funcs[16])
#   define	DLM_GET_LKINFO \
	((dlm_status_t (*)(dlm_lkid_t *, dlm_lkinfo_t *)) \
	CX_Proc_CB.cxaxp_dlm_funcs[17])
#   define	DLM_GET_RSBINFO \
	((dlm_status_t (*)(dlm_nsp_t, uchar_t *, ushort_t, \
	dlm_rsbinfo_t *, dlm_lkid_t *)) CX_Proc_CB.cxaxp_dlm_funcs[18])
    /* IMC related elements */
    i4			 cxaxp_imc_errs;/* Current cnt of IMC MC errors */
    i4			 cxaxp_imc_errb;/* Base cnt of IMC MC errors */
#   define	CXAXP_IMC_LK_CNT	1
    i4			 cxaxp_imc_lk_cnt;/* # of "locks" in lock set. */
    i4			 cxaxp_imc_maxspin; /* Largest spin value seen. */
    i4			 cxaxp_imc_retries; /* Number retries so far. */
    i4			 cxaxp_imc_spinfails; /* failures due to spins. */
    i4			 cxaxp_imc_isofails; /* fails due to isolation */
    imc_asid_t		 cxaxp_imc_gsm_id;/* ID for global shared mem region */
    imc_lkid_t		 cxaxp_imc_lk_id;/* ID for IMC "lock set" */
    caddr_t		 cxaxp_imc_txa;	/* Transmit address to GSM */
    caddr_t		 cxaxp_imc_rxa;	/* Receive address from GSM */
    CXAXP_CMO_IMC_CB	*cxaxp_cmo_txa; /* Transmit addr to CMO part of GSM */
    CXAXP_CMO_IMC_CB	*cxaxp_cmo_rxa; /* Receive addr to CMO part of GSM */
    void		*cxaxp_imc_lib_hndl; /* Handle to libimc.so */
    void		*cxaxp_imc_funcs[8]; /* Dynamically loaded fncs */
#   define	IMC_API_INIT \
	((int (*)(unsigned long *))CX_Proc_CB.cxaxp_imc_funcs[0])
#   define	IMC_RDERRCNT_MR \
	((int (*)(int))CX_Proc_CB.cxaxp_imc_funcs[1])
#   define	IMC_CKERRCNT_MR \
	((int (*)(int *,int))CX_Proc_CB.cxaxp_imc_funcs[2])
#   define	IMC_ASALLOC \
	((int (*)(imc_key_t, imc_size_t, imc_perm_t, int, imc_asid_t *, \
	 int))CX_Proc_CB.cxaxp_imc_funcs[3])
#   define	IMC_ASATTACH \
	((int (*)(imc_asid_t, imc_dir_t, int, int, caddr_t *)) \
	 CX_Proc_CB.cxaxp_imc_funcs[4])
#   define	IMC_LKALLOC \
	((int (*)(imc_key_t, int *, imc_perm_t, int, imc_lkid_t *)) \
	 CX_Proc_CB.cxaxp_imc_funcs[5])
#   define	IMC_LKACQUIRE \
	((int (*)(imc_lkid_t, int, int, int))CX_Proc_CB.cxaxp_imc_funcs[6])
#   define	IMC_LKRELEASE \
	((int (*)(imc_lkid_t, int))CX_Proc_CB.cxaxp_imc_funcs[7])
    
    /* NUMA related fields */
    void		*cxaxp_numa_lib_hndl; /* Handle to libnuma.so */
    void		*cxaxp_numa_funcs[14]; /* Dynamically loaded fncs */
#   define	RAD_GET_NUM \
	((int (*)(void))CX_Proc_CB.cxaxp_numa_funcs[0])
#   define	NSHMGET \
	((int (*)(key_t, size_t, int, memalloc_attr_t *)) \
	CX_Proc_CB.cxaxp_numa_funcs[1])
#   define	MEMALLOC_ATTR \
	((int (*)(vm_offset_t, memalloc_attr_t *)) \
	CX_Proc_CB.cxaxp_numa_funcs[2])
#   define	RADSETCREATE \
	((int (*)(radset_t *))CX_Proc_CB.cxaxp_numa_funcs[3])
#   define      RADSETDESTROY \
	((int (*)(radset_t *))CX_Proc_CB.cxaxp_numa_funcs[4])
#   define	RADADDSET \
	((int (*)(radset_t, radid_t))CX_Proc_CB.cxaxp_numa_funcs[5]) 
#   define	RADFILLSET \
	((int (*)(radset_t))CX_Proc_CB.cxaxp_numa_funcs[6]) 
#   define      RAD_FOREACH \
	((int (*)(radset_t, unsigned int, radset_cursor_t *)) \
	CX_Proc_CB.cxaxp_numa_funcs[7])
#   define      RADISEMPTYSET \
	((int (*)(radset_t)) CX_Proc_CB.cxaxp_numa_funcs[8])
#   define	RAD_ATTACH_PID \
	((int (*)(pid_t, radset_t, ulong_t)) \
	CX_Proc_CB.cxaxp_numa_funcs[9])
#   define	RAD_GET_INFO \
	((int (*)(radid_t, rad_info_t *))CX_Proc_CB.cxaxp_numa_funcs[10])
#   define	CPUSETCREATE \
	((int (*)(cpuset_t *))CX_Proc_CB.cxaxp_numa_funcs[11])
#   define      CPUSETDESTROY \
	((int (*)(cpuset_t *))CX_Proc_CB.cxaxp_numa_funcs[12])
#   define      CPUCOUNTSET \
	((int (*)(cpuset_t))CX_Proc_CB.cxaxp_numa_funcs[13])
	
#elif defined(LNX)
/*
**      Linux implementation
*/
    /* DLM related elements */
    i4			 cxlnx_lk_instkey; /* Bits to add to lock key to
				force installations to have separate scope */
    int			 cxlnx_dlm_grp_id; /* Set to GRP if attached, else 0 */
    int			 cxlnx_dlm_rdom_id;/* Recovery domain ID */
    void		*cxlnx_dlm_lib_hndl; /* Handle to DLM library */
    void		*cxlnx_dlm_funcs[16]; /* Dynamically loaded DLM funcs.*/
#   define	DLM_SETNOTIFY \
	((dlm_stats_t (*)(int, int *))CX_Proc_CB.cxlnx_dlm_funcs[0])
#   define	DLM_GRP_CREATE \
	((dlm_stats_t (*)(int *, int))CX_Proc_CB.cxlnx_dlm_funcs[1])
#   define	DLM_GRP_ATTACH \
	((dlm_stats_t (*)(int, int))CX_Proc_CB.cxlnx_dlm_funcs[2])
#   define	DLM_GRP_DETACH \
	((dlm_stats_t (*)(int))CX_Proc_CB.cxlnx_dlm_funcs[3])
#   define	DLMLOCK_SYNC \
	((dlm_stats_t (*)(int, struct lockstatus *, int, void *, \
             unsigned int, void *, dlm_bastlockfunc_t *)) \
	CX_Proc_CB.cxlnx_dlm_funcs[4])
#   define	DLMLOCK \
	((dlm_stats_t (*)(int, struct lockstatus *, int, void *, \
        unsigned int, dlm_astlockfunc_t *, void *, dlm_bastlockfunc_t *)) \
	CX_Proc_CB.cxlnx_dlm_funcs[5])
#   define	DLMLOCKX_SYNC \
	((dlm_stats_t (*)(int, struct lockstatus *, int, void *, \
	unsigned int, void *, dlm_bastlockfunc_t *, dlm_xid_t *)) \
	CX_Proc_CB.cxlnx_dlm_funcs[6])
#   define	DLMLOCKX \
	((dlm_stats_t (*)(int, struct lockstatus *, int, void *, \
	unsigned int, dlm_astlockfunc_t *, void *, dlm_bastlockfunc_t *, \
	dlm_xid_t *)) CX_Proc_CB.cxlnx_dlm_funcs[7])
#   define	DLMUNLOCK_SYNC \
	((dlm_stats_t (*)(int, char *, int)) \
	CX_Proc_CB.cxlnx_dlm_funcs[8])
#   define	ASTPOLL \
	((dlm_stats_t (*)(int, int)) \
	CX_Proc_CB.cxlnx_dlm_funcs[9])
#   define	DLM_PURGE \
	((dlm_stats_t (*)(int, int, int)) \
	CX_Proc_CB.cxlnx_dlm_funcs[10])
#   define	DLM_SCNOP \
	((dlm_stats_t (*)(int, scn_op_t, short, char *, char *)) \
	CX_Proc_CB.cxlnx_dlm_funcs[11])
#   define	DLM_RD_ATTACH \
	((dlm_stats_t (*)(char *, int *, int)) \
	CX_Proc_CB.cxlnx_dlm_funcs[12])
#   define	DLM_RD_DETACH \
	((dlm_stats_t (*)(int, int)) \
	CX_Proc_CB.cxlnx_dlm_funcs[13])
#   define	DLM_RD_COLLECT \
	((dlm_stats_t (*)(int, int)) \
	CX_Proc_CB.cxlnx_dlm_funcs[14])
#   define	DLM_RD_VALIDATE \
	((dlm_stats_t (*)(int, int)) \
	CX_Proc_CB.cxlnx_dlm_funcs[15])
#elif defined(VMS) /* _VMS_ */
    i4			 cxvms_lk_instkey; /* Bits to add to lock key to
				force installations to have separate scope */
    int			 cxvms_dlm_rsdm_id; /* Set to GRP if attached, else 0 */
    PTR			 cxvms_bli_buf[2]; /* Pointer to LKIDEF buffers */
    SIZE_TYPE		 cxvms_bli_size[2]; /* Size of buffers in bytes */
    i4			 cxvms_uic_group; /* Group portion of UIC */

#endif	/*OS SPECIFIC SECTION*/
}; /*CX_PROC_CB*/



/*}
** Name: CX_NODE_CB - CX Node Control Block
**
** Description:
**      This structure contains all the information used on a per node
**	basis by CX facility clients.   It normally is initialized by
**	the CSP process using shared memory allocatted out of the LGK
**	shared memory segment, but other arrangements may be made for
**	testing purposes.
**
**	Structure contains generic information which is used on every 
**	platform, as well as OS specific fields.   Naming conventions
**	are the same as for CX_PROC_CB.
**
** History:
**      06-Feb-2001 (devjo01)
**	    Created from specification for SIR 103715.
**	20-Sep-2004 (devjo01)
**	    Cumulative changes for Linux beta.  Add cx_lsn, and
**	    cx_lsn_sem to support CX LSN generation.
**	09-may-2005 (devjo01)
**	    Add cxvms_csids.
*/
struct _CX_NODE_CB 
{
    i4			 cx_mem_mark;	/* 'C''X''N''C' */
#   define	CX_NCB_TAG	CV_C_CONST_MACRO('C','X','N','C')
    i4			 cx_ncb_version;/* Version of block x 100 */
#   define	CX_NCB_VER	102
    i4			 cx_node_state;	/* Status for this NCB */
    i4			 cx_node_num;	/* Node number for this node */
    CS_ASET		 cx_uniqprot;	/* Protect cx_unique */
    LK_UNIQUE		 cx_unique;	/* Unique ID sequence. */
    i4			 cx_cmo_type;	/* CMO implementation style */
    i4			 cx_clm_type;	/* CLM implementation style */
    i4			 cx_clm_gran;	/* CLM chunk size as pwr of two */
    i4			 cx_clm_size;	/* CLM resource pool in 64K units */
    PID			 cx_csp_pid;	/* Process ID of CSP this node. */
    CS_ASET		 cx_ivb_seen;	/* Invalid lock value block seen. */
    i4			 cx_assigned_rad; /* RAD ID if NUMA or 0 */
    i4			 cx_dlm_signal; /* Sig. for DLM callbacks (Tru64,Lnx) */
    i4			 cx_imc_rail;	/* MC "rail" to use. (Tru64 only) */
    i4			 cx_clm_chitseq;/* Next chit seq # for CLM file impl. */
    CS_SEMAPHORE	 cx_clm_sem;	/* Sem for CLM chit gen serialization */
    CX_LSN		 cx_lsn;	/* Internal "local" LSN this node. */
    CS_SEMAPHORE	 cx_lsn_sem;	/* Sem for LSN gen serialization */
#if defined(axp_osf) &&  defined(conf_CLUSTER_BUILD)
/*
**      Tru64 implementation
*/
    dlm_glc_id_t	 cxaxp_glc;	/* Group lock container ID this node */
#elif defined(LNX)
/*
**      Linux/OpenDLM implementation
*/
    int			 cxlnx_dlm_grp_id; /* Lock group ID for this node */
    int			 cxlnx_dlm_rdom_id;/* Recovery domain ID */
#elif defined(VMS) /* _VMS_ */
/*
**      OpenVMS implementation
*/
    int                  cxvms_dlm_rsdm_id;/* Resource domain ID */
    i4			 cxvms_csids[CX_MAX_NODES+1]; /* VMS cluster IDs,
                            indexed by Ingres cluster ID.  Element 0 also
			    holds VMS cluster ID for this node. */
    CX_GLC_CTX		 cxvms_csp_glc;
    CX_GLC_CTX		 cxvms_glcs[CX_MAX_SERVERS];
    CX_OFFSET		 cxvms_glm_free; /* Head of CX_GLC_MSG list */
    i4			 cxvms_csp_uic_group; /* Group portion of UIC for CSP */
    i4			 cxvms_glc_hwm;	/* High water mark for cxvms_glcs */
#endif	/*OS SPECIFIC SECTION*/
}; /*CX_NODE_CB*/



/*}
** Name: CX_CMO_DLM_CB - Control information for one CMO lock.
**
** Description:
**      This structure contains information maintained for one lock for the
**	implementation of CMO support using the 'D'istributed 'L'ock
**	'M'anager.  A small array of these are kept for each CMO, so that
**	multiple threads can manipulate CMO DML locks without having
**	to allocate & release a new lock for each instance
**
** History:
**      26-Feb-2001 (devjo01)
**	    Created from specification for SIR 103715.
*/
struct _CX_CMO_DLM_CB
{
    CS_ASET             cx_cmo_busy;
    CX_REQ_CB		cx_cmo_lkreq;
}; /*CX_CMO_DLM_CB*/


/*}
** Name: CX_MSG_CB - Control information for one message channel.
**
** Description:
**      This structure contains information maintained for one 
**	message channel.
**
** History:
**      01-Mar-2001 (devjo01)
**	    Created from specification for SIR 103715.
*/
struct _CX_MSG_CB
{
    LK_UNIQUE		 cx_msg_transid;
    CX_REQ_CB		 cx_msg_hello_lkreq;
    CX_REQ_CB		 cx_msg_goodbye_lkreq;
    CX_MSG_RCV_FUNC	*cx_msg_rcvfunc;
    PTR			 cx_msg_rcvauxdata;
    i4			 cx_msg_pending;
    i4			 cx_msg_ready;
}; /*CX_MSG_CB*/


/*}
** Name: CX_MM_CB - Control information for message monitor.
**
** Description:
**      This structure contains information maintained for the
**	message monitor session.
**
** History:
**      01-Mar-2001 (devjo01)
**	    Created from specification for SIR 103715.
*/
struct _CX_MM_CB
{
    i4			 cx_mm_online;	/* non-zero if MM on-line */
    CS_SID		 cx_mm_sid;	/* SID on message monitor session */
}; /*CX_MM_CB*/


/*}
** Name: CX_MSG_HEAD - Structure of head of a "long" message.
**
** Description:
**      This struct represents the 1st eight bytes of a "long"
**	message. 
**
** Note: Tru64 IMC implementation requires that this be a multiple
**	of eight bytes in size.
**
** History:
**      13-sep-2002 (devjo01)
**	    Created to allow embeding of chit id in message
**	    as a cross check of message validity.
*/
struct _CX_MSG_HEAD
{
    i4			 cx_lmh_length;	/* Length of message after header. */
    i4			 cx_lmh_chit;	/* "Chit" used to redeem message. */
}; /*CX_MSG_HEAD*/

/*}
** Name: CX_MSG_FRAG - One chunk of a "long" message.
**
** Description:
**      This union represents a single message fragment, and is
**	defined, such that compilers will assume the most
**	restrictve alignment.
**
** History:
**      02-jun-2002 (devjo01)
**	    Created for long message usage.
*/
union _CX_MSG_FRAG
{
    struct {
	i4	length;
	i4	chit;
	char	body[1];
    }   msg;
    ALIGN_RESTRICT frag[CX_MSG_FRAGSIZE/sizeof(ALIGN_RESTRICT)];
}; /*CX_MSG_FRAG*/

#define CX_MSG_IMC_SEGSIZE	( CX_MSG_MAX_CHIT * CX_MSG_FRAGSIZE )

/*
**	Definition of implementation specific typedefs
*/
#ifdef  axp_osf
/*}
** Name: CX_CMO_IMC_CB - Control information for one IMC CMO region.
**
** Description:
**      This structure contains information maintained for one CMO for the
**	implementation of CMO using Tru64's 'I'nternal 'M'emory 'C'hannel.
**	Seq # & PID must be kept withing the same eight byte region to
**	preclude the possibility of one being updated without the other.
**
** History:
**      26-Feb-2001 (devjo01)
**	    Created from specification for SIR 103715.
*/
struct _CXAXP_CMO_IMC_CB
{
    u_i4		cx_cmo_seq;	/* Update sequence number */
    u_i4		cx_cmo_pid;	/* PID of last updater */
    u_i4		cx_cmo_errcnt;	/* Error cnt at write time */
    u_i4		cx_cmo_spare;	/* for alignment */
    CX_CMO	  	cx_cmo_value;	/* Value for this CMO */ 
};

#define	CX_CMO_IMC_SEGSIZE \
	( CX_MAX_CMO * sizeof(CXAXP_CMO_IMC_CB) )

#else	/*axp_osf*/
	/* Other OS specific typedefs */
#endif	/*axp_osf*/

/*
**  Function declarations for routines private to CX.
*/

STATUS	cx_translate_status( i4 osstatus );
STATUS  cx_config_opt( char *cfgrsc, char *choicelist, i4 *selection );
STATUS  cx_msg_clm_file_init( void );
CX_GLC_CTX *cx_find_glc( PID, i4 );

/*
**  Function declarations for debug routines.
*/
STATUS	CXdlm_dump_lk( i4 (*output_fcn)(PTR, i4, char *),
		       LK_UNIQUE *plkid );
STATUS	CXdlm_dump_both( i4 (*output_fcn)(PTR, i4, char *),
			 CX_REQ_CB *preq );

#ifdef axp_osf
STATUS	cx_axp_dl_load_lib( char *libname, void **plibhandle,
		   void *func_array[], char *name_array[], int numentries );
#endif
