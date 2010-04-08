/*
** Copyright (c) 2001, 2008 Ingres Corporation
**
*/

/**CL_SPEC
** Name: CX.H - CX compatibility library definitions.
**
** Description:
**      This header contains the description of the types, constants and
**      function used in the CX ('C'luster e'X'tension) public interface.
**
** Specification:
**  
**  Description:
**      The CX routines provide an OS independent interface for services
**	needed to support Ingres clusters. 
**
**	This includes functions	for support of a "Distributed lock manager" 
**	(DLM), plus 'C'luster 'M'emory 'O'bjects (CMOs), and low level
**	node management and cross cluster IPC support routines.
** 
**  Intended Uses:
**	The CX routines are mainly used by the LG & LK facilities
**	in direct support of Ingres clusters.  LK.H must be included
**	before this file.
**      
**	There also exist a number of routines for getting and setting
**	NUMA context which may be used in a non-clustered NUMA 
**	environment.
**
**  Assumptions:
**	Caller should not assume that this facility will be fully functional 
**	on all platforms.  In those environments in which clustering is not 
**	supported or configured, many CX routines return a status of
**	E_CL2C10_CX_E_NOSUPPORT, and calling programs must allow for this.
**
**  Definitions:
**
**	See DDS.
**
**  Concepts:
**	
**	Please see DDS for Linux cluster support, and going backwards in
**	time, the design Spec for SIR 103715, and the various design
**	docs for VAX clustering.  Please keep in mind the older documents
**	reflect the design at the time they were written, and the source is
**	the true mirror of reality.
**
** History:
**      06-feb-2001 (devjo01)
**          Created.
**	10-jan-2002 (devjo01)
**	    Added CX_F_LOCALLOCK
**	29-may-2002 (devjo01)
**	    Added extended message functions.
**	12-sep-2002 (devjo01)
**	    Added NUMA related functions.
**	01-apr-2004 (devjo01)
**	    Added CX_HAS_FAST_CLM.  Move CX_MAX_MSG_STOW to cxprivate.h
**	    One more parameter to CXmsg_append.
**	14-may-2004 (devjo01)
**	    Added CXalter flag CX_A_QUIET.
**	    Added prototype for CXmsg_clmclose.
**	20-sep-2004 (devjo01)
**	    Cumulative CX changes for cluster beta.
**	    - Add flag CX_F_NAB which allows user to specify both an async
**	    notification and a blocking notification function to a lock
**	    request or conversion (currently not exploited).
**	    - Add prototypes for CXcmo_sequence, CXcmo_synch.  These
**	    functions are versions of the standard CMO functions optimized
**	    to exploit the most efficient available mechanism for
**	    incrementing a cluster wide counter (CXcmo_sequence), or
**	    setting a local and cluster wide counter to whichever holds
**	    the highest value.
**	    - Add CX_F_LSNINLVB, CXcmo_lsn_gsynch, CXcmo_lsn_lsynch, and
**	    CXcmo_lsn_next, to support efficient cluster Log Sequence Number
**	    (LSN) coordination by piggybacking LSN info into the unused
**	    portions of the Lock Value Block (LVB).
**	    - Add CX_A_LK_FORMAT_FUNC and CX_A_ERR_MSG_FUNC to CXalter.
**	14-dec-2004 (devjo01)
**	    Add CX_F_NO_DOMAIN, E_CL2C2A_CX_E_DLM_RD_DIRTY as part
**	    of recovery domain support.
**	12-apr-2005 (devjo01)
**	    - Split a separate type for passing CX messages from CX_CMO.
**	    CX_MSG will allow 32 byte short messages.  This implies that
**	    on all platforms we will have at least a 32 byte LVB or use
**	    a separate mechanism for passing the short synchronized messages.
**	    - Add CX_HAS_DLM_GLC as a valid parameter for CXconfig_settings.
**	22-apr-2005 (devjo01)
**	    VMS porting changes.  Add CX_A_ONE_NODE_OK.
**	06-may-2005 (fanch01)
**	    Change for CMR recovery.
**	    Add cx_cmr_populated to the CMR to indicate whether the CMR
**	    has been populated by another node.  An empty (all zero) LVB was
**	    legal so we need a flag to indicate it has been properly populated.
**	05-may-2005 (devjo01)
**	    Folded in distributed deadlock support.  Needed on platforms
**	    like VMS where native DLM is unaware of "transactions".
**	    - New functions CXdlm_get_bli, CXdlm_mark_as_deadlocked.
**	    - New flag (CX_F_CVT_CVT) as argument to CXdlm_get_bli where
**	      cvt-cvt deadlock detection is desired.
**	    - New type CX_BLKI (Blocking locks info);
**	    - PPP error codes.
**	    - Make CX_MSG_VALUE_SIZE dependent on CX_DLM_VALUE_SIZE.
**	21-jun-2005 (devjo01)
**	    - Add E_CL2C2B_CX_E_DLM_GLC_FAIL, CX_LKI_F_USEPROXY,
**            CX_A_PROXY_OFFSET, CX_MAX_SERVERS.
**	    - Change cx_key from pointer to embedded struct.
**	    - Add E_CL2C3C_CX_E_OS_CONFIG_ERR to be returned if SYSLCK
**	      privilege is not authorized.
**	16-nov-2005 (devjo01)
**	    - Rename CXdlm_mark_as_deadlocked to CXdlm_mark_as_posted.
**	02-jan-2007 (abbjo03)
**	    Add trace parameter to CXppp_alloc.
**	20-Mar-2007 (jonj)
**	    Add CX_F_NOQUECVT for CXdlm_convert call from lk_handle_ivb
**	    to prevent reordering of established lock requests.
**	13-jul-2007 (joea)
**	    Add which_buf parameter to CXdlm_get_blki and correct name.
**	04-Oct-2007 (jonj)
**	    Add CS_A_NEED_CSID for CXalter.
**	07-jan-2008 (joea)
**	    Discontinue use of cluster nicknames.
**	21-feb-2008 (joea)
**	    Add prototypes for CXdlm_lock_info and CXnode_name_from_id.
**	    Add CX_LOCK_INFO and CX_LOCK_LIST.
**	23-Aug-2009 (kschendel) 121804
**	    Need pc.h too.
**/

#ifndef CX_H_INCLUDED
#define CX_H_INCLUDED

/*
**  Mandatory header files.
*/

#include <pc.h>

#ifndef ME_HDR_INCLUDED
#include <me.h>
#endif

#ifndef LK_H_INCLUDED
#include <lk.h>
#endif

#ifndef CXCL_H_INCLUDED
#include <cxcl.h>
#endif


/*
**  Defined Constants.
*/
#define	CX_MAX_NODES 	16	/* Maximum # supported nodes per cluster */
#define CX_MAX_SERVERS  32      /* Maximum # supported servers per node */
#define CX_MAX_CMO	 8	/* Maximum # supported CMOs */
#define CX_MAX_CHANNEL	 2	/* Maximum # supported channels. */

#define CX_MAX_HOST_NAME_LEN	64	/* Must be == GCC_L_NODE */
#define CX_MAX_NODE_NAME_LEN	72	/* Maximum length of a node name
					   Must be >= GCC_L_NODE + 8 */
#define CX_MSG_VALUE_SIZE	CX_DLM_VALUE_SIZE /* # of bytes in a MSG */
#define CX_CMO_VALUE_SIZE	16	/* Number of bytes in a CMO */
#define CX_CMO_RESERVED		 3	/* # CMO's reserved for internal use */
#define CX_LSN_CMO		 2	/* CMO index for CX internal LSN */

/* CX facility states */
#define	CX_ST_UNINIT		0	/* CX facility is uninitialized */
#define	CX_ST_INIT		1	/* CX facility is initialized */
#define	CX_ST_JOINED		2	/* CX facility is joined to cluster */
#define	CX_ST_DYING 		3	/* CX facility has commited to exit */

/*
**  CX Function flag values.
**
**  Note: not all functions take all flags.  Where possible,
**  flags which have the same semantic meaning in LK are given
**  the same values here, so as to reduce translation overhead
*/
#define	CX_F_NOWAIT	LK_NOWAIT	/* (1L<<0) Don't queue request */
#define	CX_F_WAIT	(1L<<1)		/* Process request synchronously */
#define CX_F_USEPROXY   (1L<<2)		/* Use LK not CX owner to resume. */
#define	CX_F_STATUS	LK_STATUS	/* (1L<<3) Enable alt. succ codes */
#define	CX_F_PCONTEXT	(1L<<4)		/* Get lock in process context */
#define CX_F_NAB        (1L<<5)         /* Set ansyn & block notify funcs */
#define	CX_F_NODEADLOCK	LK_NODEADLOCK	/* (1L<<6) Don't check for deadlock */
#define CX_F_LSN_IN_LVB	(1L<<7)		/* Propagate an LSN in LVB */
#define CX_F_OWNSET	(1L<<8)		/* cx_owner has already been set */
#define	CX_F_NO_DOMAIN  (1L<<9)		/* Lock not in any recovery domain */
#define CX_F_INTERRUPTOK LK_INTERRUPTABLE /* (1L<<10) Lock wait interrupt OK */
#define CX_F_IGNOREVALUE (1L<<11)	/* Do NOT set lock value. */
#define CX_F_NOTIFY	(1L<<12)	/* Call 'cx_user_func' on completion. */
#define CX_F_DLM_TRACE	(1L<<13)	/* Trace DLM. (if tracing compiled) */
#define CX_F_CMO_TRACE	(1L<<14)	/* Trace CMO. (if tracing compiled) */
#define CX_F_MSG_TRACE	(1L<<15)	/* Trace MSG. (if tracing compiled) */
#define CX_F_LOCALLOCK	(1L<<16)	/* Lock scope restricted to one node */
#define CX_F_CLEAR_ONLY (1L<<17)	/* Only clear request structure. */
#define	CX_F_PRIORITY	(1L<<18)	/* Grant ahead of normal grants*/
#define CX_F_INVALIDATE	(1L<<19)	/* Mark value block invalid */
#define CX_F_CVT_CVT    (1L<<20)	/* Check for cvt-cvt deadlock */
#define	CX_F_IS_CSP	(1L<<21)	/* Caller is CSP */
#define	CX_F_IS_SERVER	(1L<<22)	/* Caller is a server (multi-threads) */
#define	CX_F_PROXY_OK 	(1L<<23)	/* DLM req can be redirected at need */
#define	CX_F_NOQUECVT 	(1L<<24)	/* Do not opt for QUECVT in CXdlm_convert */

#define	CX_LK_SHARED_REQ_FLAGS	(LK_NOWAIT|LK_STATUS|LK_NODEADLOCK)
#define	CX_LK_SHARED_WAIT_FLAGS	(LK_INTERRUPTABLE)

/*
**  CX configuration settings.
**
**  These symbols are passed to CXconfig_settings to query certain boolean
**  behavioral settings for the CX facility.
*/
#define CX_HAS_OS_CLUSTER_SUPPORT	0
#define CX_HAS_OS_CLUSTER_ENABLED	1
#define CX_HAS_CX_CLUSTER_ENABLED	2
#define CX_HAS_PCONTEXT_LOCKS_ONLY	3
#define CX_HAS_DEADLOCK_CHECK		4
#define CX_HAS_CSP_ROLE			5
#define CX_HAS_1ST_CSP_ROLE		6
#define CX_HAS_MASTER_CSP_ROLE		7
#define CX_HAS_NUMA_SUPPORT		8
#define CX_HAS_FAST_CLM			9
#define CX_HAS_DLM_GLC			10

/*
**  CX attributes for CXalter
*/
#define CX_A_NEED_CSP			0
#define CX_A_DLM_TRACE			1
#define CX_A_CMO_TRACE			2
#define CX_A_MSG_TRACE			3
#define CX_A_ALL_TRACE			4
#define CX_A_CSP_ROLE			5
#define CX_A_IVB_SEEN			6
#define CX_A_MASTER_CSP_ROLE		7
#define CX_A_QUIET			8
#define CX_A_LK_FORMAT_FUNC		9
#define CX_A_ERR_MSG_FUNC		10
#define CX_A_ONE_NODE_OK		11
#define CX_A_PROXY_OFFSET		12
#define CX_A_NEED_CSID			13

/*
** CXget_context flags
*/
#define CX_NSC_REPORT_ERRS	(1<<0)	/* If set, report error to stderr */
#define CX_NSC_IGNORE_NODE	(1<<1)	/* If set ignore "-node" args */
#define CX_NSC_CONSUME		(1<<2)  /* Eat params after successful scan */
#define CX_NSC_RAD_OPTIONAL	(1<<4)  /* Specific RAD value is optional */
#define CX_NSC_NODE_GLOBAL 	(1<<5)  /* Non-local nodes OK. */
#define CX_NSC_SET_CONTEXT 	(1<<6)  /* Set CX context if OK. */

/* Typical flags for a utility that can operate on remote nodes (eg rcpstat) */
#define CX_NSC_STD_CLUSTER (CX_NSC_REPORT_ERRS|CX_NSC_NODE_GLOBAL| \
 CX_NSC_CONSUME|CX_NSC_SET_CONTEXT)

/* Typical flags for a utility which can only effect local host (csinstall)*/
#define CX_NSC_STD_NUMA (CX_NSC_REPORT_ERRS|CX_NSC_CONSUME|CX_NSC_SET_CONTEXT)


/*
**  CX return status codes.  Note: OK (zero) is the normal success code
*/

/*  Alternate success codes */
/* Lock was granted or converted synchronously */
#define	E_CL2C01_CX_I_OKSYNC	(E_CL_MASK + E_CX_MASK + 0x01)
/* Status code set in cx_status on asynchronous grant */
#define	E_CL2C02_CX_I_OKASYNC	(E_CL_MASK + E_CX_MASK + 0x02)
/* First lock granted on resource */
/* #define	E_CL2C03_CX_I_NEWLOCK	(E_CL_MASK + E_CX_MASK + 0x03) */
/* Lock or conversion granted, but user interrupt received. */
#define E_CL2C04_CX_I_OKINTR	(E_CL_MASK + E_CX_MASK + 0x04)
/* Iterative process has completed. */
#define E_CL2C05_CX_I_DONE	(E_CL_MASK + E_CX_MASK + 0x05)

/*  Qualified success codes (warnings) */
#define	CX_WARN_THRESHHOLD 	(E_CL_MASK + E_CX_MASK + 0x08)
/* Lock converted or granted, but Value block read is marked invalid */
#define	E_CL2C08_CX_W_SUCC_IVB	(E_CL_MASK + E_CX_MASK + 0x08)
/* Lock cancel unsuccessful, target request was granted */
#define	E_CL2C09_CX_W_CANT_CAN	(E_CL_MASK + E_CX_MASK + 0x09)
/* Lock wait timed out. */
#define	E_CL2C0A_CX_W_TIMEOUT	(E_CL_MASK + E_CX_MASK + 0x0A)
/* Lock wait interrupted. Request cancelled. */
#define	E_CL2C0B_CX_W_INTR	(E_CL_MASK + E_CX_MASK + 0x0B)
/* Lock wait interrupted, but request was granted, but value found invalid. */
#define	E_CL2C0C_CX_W_INTR_IVB	(E_CL_MASK + E_CX_MASK + 0x0C)

/*  Failure codes */
#define	CX_ERR_THRESHHOLD  	(E_CL_MASK + E_CX_MASK + 0x10)
/* Clustering is not supported or is not configured at the OS level */
#define	E_CL2C10_CX_E_NOSUPPORT	(E_CL_MASK + E_CX_MASK + 0x10)
/* Invalid parameter passed to routine. */
#define	E_CL2C11_CX_E_BADPARAM	(E_CL_MASK + E_CX_MASK + 0x11)
/*  */
#define E_CL2C12_CX_E_INSMEM	(E_CL_MASK + E_CX_MASK + 0x12)
/* Ingres is not configured for cluster support. */
#define E_CL2C13_CX_E_NOTCONFIG	(E_CL_MASK + E_CX_MASK + 0x13)
/* A CSP must be active on node before a non-CSP CX user can start. */
#define E_CL2C14_CX_E_NOCSP	(E_CL_MASK + E_CX_MASK + 0x14)
/* Only one CSP per node. */
#define E_CL2C15_CX_E_SECONDCSP	(E_CL_MASK + E_CX_MASK + 0x15)
/* Configured node number is out of range. */
#define E_CL2C16_CX_E_BADCONFIG	(E_CL_MASK + E_CX_MASK + 0x16)
/* Invalid value for ii.$.config.cmotype. */
#define E_CL2C17_CX_E_BADCONFIG	(E_CL_MASK + E_CX_MASK + 0x17)
/* OS specific configuration error. */
#define E_CL2C18_CX_E_BADCONFIG	(E_CL_MASK + E_CX_MASK + 0x18)
/* OS specific configuration error. */
#define E_CL2C19_CX_E_BADCONFIG	(E_CL_MASK + E_CX_MASK + 0x19)
/* Inexplicable inconsistency in internal CX structures */
#define E_CL2C1A_CX_E_CORRUPT	(E_CL_MASK + E_CX_MASK + 0x1A)
/* CX call made in bad context. Two connects, DLM request < connect, etc.*/
#define E_CL2C1B_CX_E_BADCONTEXT (E_CL_MASK + E_CX_MASK + 0x1B)
/* Failure in CX LSN propagation system. */
#define E_CL2C1C_CX_E_LSN_LVB_FAILURE	(E_CL_MASK + E_CX_MASK + 0x1C)

/* Vendor provided DLM is unavailable */
#define E_CL2C20_CX_E_NODLM	(E_CL_MASK + E_CX_MASK + 0x20)
/* Lock request/conversion failed due to deadlock */
#define E_CL2C21_CX_E_DLM_DEADLOCK (E_CL_MASK + E_CX_MASK + 0x21)
/* Lock request/conversion canceled */
#define E_CL2C22_CX_E_DLM_CANCEL (E_CL_MASK + E_CX_MASK + 0x22)
/* Problems connecting to, or disconnecting from DLM. */
#define E_CL2C23_CX_E_DLM_CONNECTION (E_CL_MASK + E_CX_MASK + 0x23)
/*  */
#define E_CL2C24_CX_E_DLM_SPINOUT (E_CL_MASK + E_CX_MASK + 0x24) 
/* Converting a lock not granted */
#define E_CL2C25_CX_E_DLM_NOTHELD (E_CL_MASK + E_CX_MASK + 0x25)
/* Lock administration action aborted/interrupted. */
#define E_CL2C26_CX_E_DLM_ABORTED (E_CL_MASK + E_CX_MASK + 0x26)
/* Request or convert could not be synchronously granted & NOWAIT set. */
#define E_CL2C27_CX_E_DLM_NOGRANT (E_CL_MASK + E_CX_MASK + 0x27)
/* Failure with asynchronous completion */
#define E_CL2C28_CX_E_DLM_CMPLERR (E_CL_MASK + E_CX_MASK + 0x28)
/* Open DLM version is incompatible */
#define E_CL2C29_CX_E_DLM_BADVERSION (E_CL_MASK + E_CX_MASK + 0x29)
/* Recovery Domain has invalidated locks. */
#define E_CL2C2A_CX_E_DLM_RD_DIRTY (E_CL_MASK + E_CX_MASK + 0x2A)
/* Miscellaneous group lock failure (VMS). */
#define E_CL2C2B_CX_E_DLM_GLC_FAIL (E_CL_MASK + E_CX_MASK + 0x2B)

/* Concurrent send to same channel by same process */
/* define E_CL2C2C_CX_E_MSG_BUSY	(E_CL_MASK + E_CX_MASK + 0x2C) */
#define E_CL2C2C_CX_E_MSG_NOSUCH	(E_CL_MASK + E_CX_MASK + 0x2C)

#define E_CL2C2D_CX_E_NOBIND		(E_CL_MASK + E_CX_MASK + 0x2D)
#define E_CL2C2E_CX_E_NONUMA		(E_CL_MASK + E_CX_MASK + 0x2E)
#define E_CL2C2F_CX_E_BADRAD		(E_CL_MASK + E_CX_MASK + 0x2F)

/* Start miscellaneous OS error equivalents */
#define E_CL2C30_CX_E_OS_UNEX_FAIL (E_CL_MASK + E_CX_MASK + 0x30)
/* Memory fault detected within OS call. */
#define E_CL2C31_CX_E_OS_EFAULT	(E_CL_MASK + E_CX_MASK + 0x31)
/* Bad parameter report from OS routine. */
#define E_CL2C32_CX_E_OS_BADPARAM  (E_CL_MASK + E_CX_MASK + 0x32)
/* Non-DLM OS subsystem not initialized. */
#define E_CL2C33_CX_E_OS_NOINIT	(E_CL_MASK + E_CX_MASK + 0x33)
/* Unexpected failure in non-DLM OS subsystem. (E.g. Tru64 IMC) */
#define E_CL2C34_CX_E_OS_SUBSYSERR (E_CL_MASK + E_CX_MASK + 0x34)
/* Implementation dependent OS errors.  Meaning depends on platform */
#define E_CL2C35_CX_E_OS_MISC_ERR (E_CL_MASK + E_CX_MASK + 0x35)
#define E_CL2C36_CX_E_OS_MISC_ERR (E_CL_MASK + E_CX_MASK + 0x36)
#define E_CL2C37_CX_E_OS_MISC_ERR (E_CL_MASK + E_CX_MASK + 0x37)
#define E_CL2C38_CX_E_OS_MISC_ERR (E_CL_MASK + E_CX_MASK + 0x38)
/* Implementation dependent OS errors due to resource depletion */
#define E_CL2C39_CX_E_OS_CONFIG_ERR	(E_CL_MASK + E_CX_MASK + 0x39)
#define E_CL2C3A_CX_E_OS_CONFIG_ERR	(E_CL_MASK + E_CX_MASK + 0x3A)
#define E_CL2C3B_CX_E_OS_CONFIG_ERR	(E_CL_MASK + E_CX_MASK + 0x3B)
#define E_CL2C3C_CX_E_OS_CONFIG_ERR	(E_CL_MASK + E_CX_MASK + 0x3C)

/* Message codes for CXget_context() */
#define E_CL2C40_CX_E_BAD_PARAMETER	(E_CL_MASK + E_CX_MASK + 0x40)
#define E_CL2C41_CX_E_MUST_SET_RAD	(E_CL_MASK + E_CX_MASK + 0x41)
#define E_CL2C42_CX_E_NOT_CONFIGURED	(E_CL_MASK + E_CX_MASK + 0x42)
#define E_CL2C43_CX_E_REMOTE_NODE	(E_CL_MASK + E_CX_MASK + 0x43)

/* Non-interactive server didn't get NUMA context. */
#define E_CL2C44_CX_E_NO_NUMA_CONTEXT	(E_CL_MASK + E_CX_MASK + 0x44)

/* PPP error values */
#define E_CL2C48_CX_E_PPP_OPEN		(E_CL_MASK + E_CX_MASK + 0x48)
#define E_CL2C49_CX_E_PPP_LISTEN	(E_CL_MASK + E_CX_MASK + 0x49)
#define E_CL2C4A_CX_E_PPP_RECEIVE	(E_CL_MASK + E_CX_MASK + 0x4A)
#define E_CL2C4B_CX_E_PPP_SEND		(E_CL_MASK + E_CX_MASK + 0x4B)
#define E_CL2C4C_CX_E_PPP_CLOSE		(E_CL_MASK + E_CX_MASK + 0x4C)
#define E_CL2C4D_CX_E_PPP_CONNECT	(E_CL_MASK + E_CX_MASK + 0x4D)

#define	CX_OK_STATUS(x)	\
	((u_i4)(x) < CX_WARN_THRESHHOLD)
#define	CX_WARN_STATUS(x) \
	(((u_i4)(x) >= CX_WARN_THRESHHOLD) && ((u_i4)(x) < CX_ERR_THRESHHOLD))
#define	CX_ERR_STATUS(x) \
	((u_i4)(x) >= CX_ERR_THRESHHOLD)

/*
**  Type declarations
*/

typedef struct _CX_RES_CB	CX_RES_CB;

typedef struct _CX_REQ_CB	CX_REQ_CB;

typedef struct _CX_BLKI		CX_BLKI;

typedef struct _CX_STATS	CX_STATS;

typedef struct _CX_CMO_CMR	CX_CMO_CMR;

typedef struct _CX_NODE_INFO	CX_NODE_INFO;

typedef struct _CX_CONFIGURATION CX_CONFIGURATION;

typedef struct _CX_RAD_INFO	CX_RAD_INFO;

typedef struct _CX_RAD_DEV_INFO	CX_RAD_DEV_INFO;

/*
** Types and macros for manipulating sets of node numbers 
** ( 1 .. CX_MAX_NODES ) as a bit array.
*/
typedef u_i4			CX_NODE_BITS;

# define CX_INIT_NODE_BITS(a) \
	((a)=0)

# define CX_SET_NODE_BIT(a,n)	\
	((a) |= (1l << (((u_i4)(n))-1)))

# define CX_CLR_NODE_BIT(a,n)   \
	((a) &= ~(1l << (((u_i4)(n))-1)))

# define CX_GET_NODE_BIT(a,n)   \
	((a) & (1l << (((u_i4)(n))-1)))


/*
**  Generic short synchronized message block
*/
typedef char	CX_MSG[CX_MSG_VALUE_SIZE];

/*
**  Generic CMO value.
*/
typedef char    CX_CMO[CX_CMO_VALUE_SIZE];

/*
**  CMO sequence value
**
**  [0] = least significant unsigned long value.
**  [1] = next most significant unsigned long values.
**  [2] [3] always zero for now.
*/
typedef u_i4	CX_CMO_SEQ[CX_CMO_VALUE_SIZE/sizeof(u_i4)];

/*
**  CMO Log Sequence Number (LSN)
**
**  Internal LSN used by CX.  It has the same form as a CX_CMO_SEQ,
**  but [2]&[3] are missing and implicitly zero.   Note this is
**  the reverse convention from an Ingres LSN.  Also note the CX
**  LSN is always a raw sequence number, while the Ingres LSN may
**  be further "decorated" to encode the node number.
*/
typedef struct _cx_lsn
{
    u_i4	cx_lsn_low;
    u_i4	cx_lsn_high;
}	CX_LSN;

/*
**  Reserved CMO for tracking 'C'luster 'M'embership 'R'oster.
*/
#define	CX_CMO_CMR_IDX		0	/* Index of CMO for tracking CMR. */

struct _CX_CMO_CMR
{
    CX_NODE_BITS cx_cmr_members;	/* Bit array of cluster members. */
    u_i4	 cx_cmr_recovery; /* Recovery status. (0=not in recovery)*/
    u_i4	 cx_cmr_populated;	/* Indicates if CMO is populated */
    u_i1	 cx_cmr_extra[CX_CMO_VALUE_SIZE-
				 (2*sizeof(u_i4)+sizeof(CX_NODE_BITS))];
};

/*
**  Reserved CMO for CXmsg facility usage.
*/
#define CX_CMO_CXMSG_IDX	1	/* Index of CMO for CX msg use */

/*
**  User callback function format.
**
**  By default this is a blocking notification function,
**  and the 2nd parameter is the incompatible lock mode
**  that triggered the notification.
**
**  If CX_F_NOTIFY was passed as a flag, then 'cx_user_func'
**  contains a notification function that is called when
**  an asynchronous lock request achieves final resolution.
**  In this case the 2nd parameter is the status code
**  for the request result.  Normally this will be the
**  same as the value in 'cx_status', except if 'cx_status'
**  is E_CL2C02_CX_I_OKASYNC, when OK is passed.
**
**  Notification functions are not triggered for requests
**  processed synchronously.
*/
typedef VOID	(CX_USER_FUNC(CX_REQ_CB *, i4));

/*
**  Standard message processing function declarations.
*/
typedef bool    (CX_MSG_RCV_FUNC( CX_MSG *, PTR, bool ));
typedef STATUS	(CX_MSG_SEND_FUNC( CX_MSG *, CX_MSG *, PTR, bool ));

/*
**  Standard CMO update function declaration.
*/
typedef STATUS	(CX_CMO_FUNC( CX_CMO *, CX_CMO *, PTR, bool ));

/*
**  CX_REQ_CB
**
**  Generic DLM request block.   Memory used for this block must
**  remain allocated until CXdlm_request or CXdlm_convert action
**  is completed or canceled, or if blocking notification is enabled,
**  memory must be retained until lock is released.   Many instances
**  of this structure will be allocated, so care should be taken to
**  keep it as small as possible.  Also care should be taken to 
**  keep elements "nicely" aligned for access.  And finally, cx_dlm_ws
**  needs to match the control block passed to the native underlying
**  DLM, and as a platform varying structure is defined in cxcl.h.
**
**	09-May-2008 (jonj)
**	    Add cx_instance, modify CX_INIT_REQ_CB macro
**	    to preserve and increment it.
**	23-Jul-2008 (jonj)
**	    Expand cx_lk_flags from u_i2 to u_i4 to be
**	    VMS compiler friendly.
**	    Extract CX_LKI_F_USEPROXY flag to its own
**	    inter-process-safe variable "cx_inproxy",
**	    apparent source of assumption in CXdlm_wait()
**	    that a request had been synchronously granted
**	    when in reality the request was still pending
**	    in the proxy server and causing late CSresumes.
**	    Add cx_spare, cx_dlmflags (as debugging aid);
**  
*/
# define cx_lock_id	cx_dlm_ws.cx_dlm_lock_id
# define cx_value	cx_dlm_ws.cx_dlm_valblk

struct _CX_REQ_CB
{
    u_i4	cx_lki_flags;	/* Internal status flags, do not modify */
#define CX_LKI_F_QUEUED         (1<<0)  /* Request queued for async grant */
#define CX_LKI_F_ISCONVERT      (1<<1)  /* Request is a convert */
#define CX_LKI_F_PCONTEXT       CX_F_PCONTEXT  /* (1<<4) lock held by process*/
#define CX_LKI_F_NAB		CX_F_NAB /* (1<<5) use user cmpl AND blk rtn */
#define CX_LKI_F_LSN_IN_LVB	CX_F_LSN_IN_LVB /* (1<<7) LSN may be in LVB */
#define CX_LKI_F_NO_DOMAIN	CX_F_NO_DOMAIN /* (1<<9) Lock not in rdom */
#define CX_LKI_F_NOTIFY		CX_F_NOTIFY /* (1<<12) use user cmpl rtn */
    u_i1	cx_inproxy;	/* Request has been redirected to proxy server */
    u_i1	cx_old_mode;	/* Current mode held by lock, or 0. */
    u_i1	cx_new_mode;	/* New mode specified for this request. */
    u_i1	cx_spare;	/* Unused */
    u_i4	cx_instance;	/* Instantiation of this CX_REQ_CB */
    u_i4	cx_dlmflags;	/* The flags passed to the underlying DLM */
    i4		cx_status;	/* Returned Async status value */
    LK_LOCK_KEY cx_key;		/* Resource key. */
    CS_CPID	cx_owner;	/* Unique ID for session making request. */
    CX_USER_FUNC *cx_user_func;	/* Rtn called when granted lock blocks others */
    VOID       *cx_user_extra;	/* Ptr to optional data for cmpl & blk rtn's.
                                   or if CX_F_NAB set address of blk rtn.
				   Caution: If these fields are referenced
				   by another process (only possible if this
				   struct resides in shared memory), these
				   addresses may not be valid, since on
				   some platforms (i.e. VMS) the shared
				   memory resides at a different address,
				   and pointers to local storage are obviously
				   useless.  If you require additional user
				   data to be passed for a request in 
				   shared memory, it is best to pass an 
				   self relative offset and reconstruct the
				   pointer to the user data (S/B in the same
				   shared memory segment) as needed.
				   MACROS CX_PTR2RELOFF and CX_RELOFF2PTR
				   are provided for this purpose.
				   On no platform is a function address
				   guaranteed to be the same, particularly
				   if different images are run.  So take
				   care to only call function pointers
				   in the setting processes context. */
    CX_DLM_CB   cx_dlm_ws;	/* Workspace for underlying DLM */
};

/* Initialize minimum fields for a valid request */
#define CX_INIT_REQ_CB(preq,mode,key) \
 { \
     u_i4	cx_instance = ((CX_REQ_CB*)(preq))->cx_instance; \
     MEfill(sizeof(CX_REQ_CB),'\0',(PTR)(preq)); \
     ((CX_REQ_CB*)(preq))->cx_key=*(LK_LOCK_KEY*)(key); \
     ((CX_REQ_CB*)(preq))->cx_new_mode=(u_i1)mode; \
     ((CX_REQ_CB*)(preq))->cx_instance=++cx_instance; \
 }

#define CX_PTR2RELOFF(b,p)	(((char *)(p)) - ((char *)(b)))
#define CX_RELOFF2PTR(b,ro)	(((char *)(b)) + ((SIZE_TYPE)(ro)))

/*
** CX_BLKI
**
** Blocking lock info structure used by CXdlm_get_bli.
**
** This is analogous to the LKIDEF structure used by VMS to report
** information about distributed locks by sys$getlki, but the
** values contained have all been converted to generic representations.
*/
struct _CX_BLKI
{
    i4		cx_blki_csid;	/* Which node hosts lock owner */
    PID		cx_blki_pid;	/* Process ID of lock owner */
    LK_UNIQUE	cx_blki_trans;  /* for VMS (0, PID) */
    LK_UNIQUE	cx_blki_lkid;	/* Lock id (in csid context) */
    i1		cx_blki_queue;  /* Queue lock is on. */
#   define	CX_BLKI_GRANTQ	0
#   define	CX_BLKI_CONVQ	1
#   define	CX_BLKI_WAITQ	2
    i1		cx_blki_rqmode; /* Requested mode this lock. */
    i1		cx_blki_gtmode; /* Granted mode this lock. */
    i1		cx_blki_spare1b;/* unused */
};

/*
** CX_LOCK_INFO - Generic lock information
*/
typedef struct
{
    u_i4	cxlk_csid;         /* Which node hosts lock owner */
    PID		cxlk_pid;          /* Process ID of lock owner */
    u_i4	cxlk_lkid;         /* Lock id (in csid context) */
    const char	*cxlk_queue;       /* Queue lock is on. */
    const char	*cxlk_req_mode;    /* Requested mode this lock. */
    const char	*cxlk_grant_mode;  /* Granted mode this lock. */
    u_i4	cxlk_mast_csid;    /* Which node hosts lock owner */
    u_i4	cxlk_mast_lkid;    /* Lock id (in csid context) */
} CX_LOCK_INFO;

/*
** CX_LOCK_LIST - Generic lock list by resource
*/
typedef struct cx_lock_list
{
    struct cx_lock_list *cxll_next;
    LK_LOCK_KEY   cxll_resource;
    u_i4	  cxll_lock_count;
    CX_LOCK_INFO  cxll_locks[1];   /* base of dynamically allocated array */
} CX_LOCK_LIST;

/*
** CX_PPP_CTX
**
** Opaque pointer used to reference a point-point-pipe context.
*/
typedef	void *CX_PPP_CTX;

/*
**  Miscellaneous CX statistical information for one process.
**
**  Updates to these values are not protected, so absolute
**  accuracy is not guaranteed.
*/
struct _CX_STATS
{
    u_i4	cx_severe_errors;	/* # of severe errors recorded. */
    u_i4	cx_msg_mon_poll_interval;/* Msg monitor poll interval (ms) */
    u_i4	cx_msg_missed_blocks;	/* # Missed blocking notifications */
    u_i4	cx_msg_notifies;	/* # Message notifications rcvd */
    u_i4	cx_dlm_requests;	/* # CXdlm_request calls. */
    u_i4	cx_dlm_quedreqs;	/* # Requests enqueued. */
    u_i4	cx_dlm_sync_grants;	/* # Synchronous grants. */
    u_i4	cx_dlm_converts;	/* # CXdlm_convert calls. */
    u_i4	cx_dlm_quedcvts;	/* # Converts enqueued. */
    u_i4	cx_dlm_sync_cvts;	/* # Synchronous conversion grants. */
    u_i4	cx_dlm_cancels; 	/* # CXdlm_cancel calls. */
    u_i4	cx_dlm_releases;	/* # CXdlm_release calls. */
    u_i4	cx_dlm_async_waits;	/* # Async lock waits. */
    u_i4	cx_dlm_async_grants;	/* # Async lock grants. */
    u_i4	cx_dlm_completions;	/* # lock completions. */
    u_i4	cx_dlm_blk_notifies;	/* # Blocking notifications rcvd. */
    u_i4	cx_dlm_deadlocks;	/* # Deadlock victims. */
    u_i4	cx_cmo_reads;		/* # CMO reads. */
    u_i4	cx_cmo_writes;		/* # CMO writes. */
    u_i4	cx_cmo_updates;		/* # CMO updates. */
    u_i4	cx_cmo_seqreqs;		/* # CMO sequence number requests. */
    u_i4	cx_cmo_synchreqs;	/* # CMO sequence # synchronizations */
    u_i4	cx_cmo_lsn_gsynchreqs;	/* # Global LSN sequence # synchs */
    u_i4	cx_cmo_lsn_lsynchreqs;	/* # Local LSN sequence # synchs */
    u_i4	cx_cmo_lsn_nextreqs;	/* # LSN sequence #'s generated */
    u_i4	cx_cmo_lsn_lvblsyncs;	/* # LVB LSN local synchs from LVB */
    u_i4	cx_cmo_lsn_lvbrsyncs;	/* # LVB LSN local synchs to LVB */
    u_i4	cx_cmo_lsn_lvbgsyncs;	/* # LVB LSN global synchs */
};


/*
**  Node Configuration information.
*/
struct _CX_NODE_INFO
{
    u_i4	cx_node_number;		/* Node number assigned this node */
    i4		cx_rad_id;		/* RAD ident, if NUMA cluster or 0.*/
    u_i4	cx_node_name_l;		/* Length of node name */
    char	cx_node_name[CX_MAX_NODE_NAME_LEN];	/* Node name */
    u_i4	cx_host_name_l;		/* Length of node host name */
    char        cx_host_name[CX_MAX_HOST_NAME_LEN]; /* Node host if virtual
					   node in a NUMA cluster */
};


/*
**  Cluster Configuration information.
*/
struct _CX_CONFIGURATION
{
    u_i4	cx_xref[CX_MAX_NODES+1];  /* Lookup cx_nodes entry used indexed
					     by node number, entry zero holds
					     total number of configured nodes.*/
# define cx_node_cnt    cx_xref[0]
    CX_NODE_INFO cx_nodes[CX_MAX_NODES];/* 1st 'cx_node_cnt' entries hold
					    public per node config info. */
};

/*
**  RAD configuration information.
*/

struct _CX_RAD_DEV_INFO
{
    i4		 cx_rdi_type;		/* Device type.  Only devices of
					   interest in configuring Ingres
					   should be returned. */
# define	 CX_RDITYPE_UNKNOWN	0
# define	 CX_RDITYPE_DISK	1
    i4		 cx_rdi_size;		/* If disk, size in Meg. */
    char	*cx_rdi_name;		/* OS device name as ASCIIZ string. */
};

struct _CX_RAD_INFO
{
    i4		cx_radi_rad_id;		/* Ingres RAD ID */
    i4		cx_radi_cpus;		/* # of CPU's this RAD */
    i4		cx_radi_physmem;	/* Amount phys memory in K */
    i4		cx_radi_numdevs;	/* # of DEV_INFO structs. */
    PTR		cx_radi_os_id;		/* OS ID of rad. */
    char *    (*cx_radi_os_id_decoder)(PTR,char *, i4); /* Address of func
					to decode OS rad ID into a buffer */
    CX_RAD_DEV_INFO *cx_radi_devs;	/* Ptr to dev info or NULL */
    char 	cx_radi_scratch[24];	/* internal usage.  Do not modify */
};

/*
**  Public function declarations
*/

/*  API functions. (available in libingres) */
bool     CXcluster_configured(void);

bool     CXnuma_configured(void);

bool     CXnuma_cluster_configured(void);

char	*CXhost_name(void);

char	*CXnode_name( char *nodename );

void CXnode_name_from_id(u_i4 id, char *node_name);

i4	 CXnode_number( char *nodename );

STATUS	 CXcluster_nodes( u_i4 *pnodecnt, CX_CONFIGURATION **hconfig );

char    *CXdecorate_file_name( char *filename, char *node );

STATUS   CXget_context( i4 *pargc, char *argv[], i4 flags,
			char *outbuf, i4 outlen );

STATUS	 CXset_context( char *node, i4 rad );

CX_NODE_INFO *CXnode_info( char *host, i4 rad );


/*  Informational functions */

bool	 CXcluster_support(void);

bool	 CXcluster_enabled(void);

bool	 CXconfig_settings( u_i4 setting );

u_i4	 CXshm_required( u_i4 flags, u_i4 nodes, u_i4 transactions,
			 u_i4 locks, u_i4 resources );

STATUS	 CXunique_id( LK_UNIQUE *punique );

/*  Administrative functions */

STATUS	 CXinitialize(char *installation, VOID *psharedmem, u_i4 flags);

STATUS	 CXjoin( u_i4 flags );

STATUS	 CXterminate( u_i4 flags );

STATUS	 CXstartrecovery( u_i4 flags );

STATUS	 CXfinishrecovery( u_i4 flags );

STATUS	 CXalter( u_i4 attribute, PTR pvalue );

/*  DLM functions */

STATUS	 CXdlm_request( u_i4 flags, CX_REQ_CB *preq, LK_UNIQUE *ptranid );

STATUS	 CXdlm_convert( u_i4 flags, CX_REQ_CB *preq );

STATUS	 CXdlm_release( u_i4 flags, CX_REQ_CB *preq );

STATUS	 CXdlm_cancel( u_i4 flags, CX_REQ_CB *preq );

STATUS	 CXdlm_wait( u_i4 flags, CX_REQ_CB *preq, i4 timeout );

bool	 CXdlm_is_blocking( CX_REQ_CB *preq );

bool	 CXdlm_mark_as_posted( CX_REQ_CB *preq );

char	*CXdlm_lock_key_to_string( LK_LOCK_KEY *lkkey, char *buf );

STATUS   CXdlm_get_blki( u_i4 flags, CX_REQ_CB *preq, i4 which_buf,
		        CX_BLKI *blkibuf, u_i4 *pblkimax, i4 *plkmode,
		        CX_BLKI **pplkibuf, u_i4 *pblkicnt );

STATUS   CXdlm_glc_support( void );

STATUS CXdlm_lock_info(char *inst_id, bool waiters_only,
                       CX_LOCK_LIST **lock_list);

/*  CMO functions */

STATUS   CXcmo_read( u_i4 cmo_index, CX_CMO *pcmo );

STATUS   CXcmo_write( u_i4 cmo_index, CX_CMO *pcmo );

STATUS   CXcmo_update( u_i4 cmo_index, CX_CMO *pcmo, 
                       CX_CMO_FUNC *upd_func, PTR upd_data );

STATUS   CXcmo_sequence( u_i4 cmo_index, CX_CMO_SEQ *pseq );

STATUS   CXcmo_synch( u_i4 cmo_index, CX_CMO_SEQ *pseq );

STATUS   CXcmo_lsn_gsynch( CX_LSN *lsnbuf );

STATUS   CXcmo_lsn_lsynch( CX_LSN *lsnbuf );

STATUS   CXcmo_lsn_next( CX_LSN *lsnbuf );

/*  MSG functions */

STATUS	 CXmsg_monitor(void);

STATUS	 CXmsg_shutdown(void);

STATUS   CXmsg_connect( u_i4 channel, CX_MSG_RCV_FUNC *pfunc, PTR pdata );

STATUS   CXmsg_disconnect( u_i4 channel );

STATUS   CXmsg_send( u_i4 channel, CX_MSG *pmsg,
		     CX_MSG_SEND_FUNC *pfunc, PTR pdata );

bool  	 CXmsg_channel_ready( u_i4 channel );

/* "Extended" message functions */

STATUS   CXmsg_stow( i4 *pchit, PTR msgtext, i4 msglength );

STATUS   CXmsg_append( i4 chit, PTR msgtext, i4 offset,
                       i4 length, i4 msglength );

STATUS	 CXmsg_redeem( i4 chit, PTR msgbuf, i4 msgbufsize, i4 offset,
		       i4 *retreived );

STATUS	 CXmsg_release( i4 *pchit );

STATUS	 CXmsg_clmclose ( i4 node );

/* Point - Point - Pipe routines */
STATUS	 CXppp_alloc( CX_PPP_CTX *hctx, char *name,
                      i4 readconnections, i4 bufsize, bool trace );

STATUS	 CXppp_free( CX_PPP_CTX *hctx );

STATUS	 CXppp_listen( CX_PPP_CTX ctx );

STATUS	 CXppp_read( CX_PPP_CTX ctx, i4 recordsize, i4 recordmax, i4 timeout,
            char **nextrecord, i4 *recordsread );

STATUS	 CXppp_connect( CX_PPP_CTX ctx, char *host, char *service ); 

STATUS	 CXppp_write( CX_PPP_CTX ctx, char *buffer,
                      i4  recordsize, i4 recordcount );

STATUS	 CXppp_close( CX_PPP_CTX ctx );

STATUS	 CXppp_disconnect( CX_PPP_CTX ctx );


/* NUMA functions (available in libingres) */

bool	 CXnuma_support(void);

i4	 CXnuma_rad_count(void);

i4	 CXnuma_cluster_rad(void);

i4	 CXnuma_user_rad(void);

i4	 CXnuma_shmget( i4 key, i4 size, i4 flags, i4 *perrno );

bool	 CXnuma_rad_configured( char *host, i4 rad );

STATUS   CXnuma_bind_to_rad( i4 rad );	 

STATUS	 CXnuma_get_info( i4 rad, CX_RAD_INFO *radinfobuf );

STATUS	 CXnuma_free_info( CX_RAD_INFO *radinfobuf );

# endif /* CX_H_INCLUDED */
