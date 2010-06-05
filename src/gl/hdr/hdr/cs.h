/*
** Copyright (c) 1993, 2008 Ingres Corporation
*/
# ifndef CS_HDR_INCLUDED
# define CS_HDR_INCLUDED

#include    <cscl.h>
#include    <tm.h>	    /* needed for CSstatistics */
#if defined(axp_osf)
# include <c_asm.h>
#endif   /* defined(axp_osf) */

/**CL_SPEC
** Name:	CS.h	- Define CS function externs
**
** Specification:
**
** Description:
**	Contains CS function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**     31-aug-1993 (smc)
**          Prototypes of functions using sids altered to make the sid
**          a portable CS_SID type.
**	18-oct-1993 (kwatts)
**	    Added prototypes for non-II versions of CSresume, CSget_sid, and
**	    CSsuspend since these are referenced by the ICL Smart Disk module.
**	10-Apr-1995 (jenjo02)
**	    Added CSp_semaphore, CSv_semaphore macros.
**	    Added prototype for CSw_semaphore().
**	25-May-1995 (jenjo02)
**	    Moved CSp|v_semaphore macros to csnormal.h to avoid
**	    porting problems introduced by defining them here.
**	21-July-1995 (hanch04)
**	    Back out shero03 change for NT.  This breaks UNIX.
**	24-aug-95 (wonst02)
**	    Add CSfind_sid() to return a SID given an SCB pointer.
**	07-Dec-1995 (jenjo02)
**	    Added CSs_semaphore() function to return selected statistics
**	    for a given semaphore.
**	 7-jan-1997 (kch)
**	    Added CS_HDR_INCLUDED to prevent multiple inclusion of cs.h.
**	30-sep-1996 (canor01)
**	    Prevent multiple inclusion of cs.h.
**	18-feb-1997 (hanch04)
**	    Add prototype for CS_is_mt().
**	    Add prototype for CSMT versions of functions
**	31-mar-1997 (canor01)
**	    Add prototypes for CScp_resume(), gen_Psem() and gen_Vsem().
**	10-apr-1997 (canor01)
**	    Correct prototype for gen_useCSsems().
**	08-jul-1997 (canor01)
**	    Add CSremove_all_sems().
**	22-jul-1997 (canor01)
**	    Add CSkill();
**	02-feb-1998 (canor01)
**	    Restore CSkill, which was lost in cross-integration.
**	31-Mar-1998 (jenjo02)
**	    Added thread_id output parm to CSadd_thread(),
**	    CSMTadd_thread() prototypes into which created
**	    thread's SID is returned.
**	16-Nov-1998 (jenjo02)
**	    Added new function CSget_cpid() to return the cross-process
**	    identity (CS_CPID) of a session.
**	    CScp_resume() now takes CS_CPID* as argument instead of
**	    pid, sid.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      24-Sep-1999 (hanch04)
**        Added CSget_svrid
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Oct-2000 (jenjo02)
**	    Added CS_FUNCTIONS structure, function vector
**	    pointer array for CS functions which have both
**	    a Ingres and OS-threaded versions. Prototyped
**	    both CS and CSMT functions which were missing.
**	16-oct-2000 (somsa01)
**	    The last change is only needed for UNIX platforms.
**	24-jan-2002 (devjo01)
**	    Added IIresume_from_AST to CS_FUNCTIONS.
**      29-aug-2002 (devjo01)
**          Added CSadjust_counter.
**      04-sep-2002 (devjo01)
**          Added IICSmon_register, IICSmon_deregister.
**	09-Jan-2003 (rigka01) bug 105321
**	    NT portion of fix to bug 105321 requires IICSnoresnow definition
**	25-Apr-2003 (jenjo02) SIR 111028
**	    Added external function CSdump_stack(), used by,
**	    at least, dmd_check().
**	17-Dec-2003 (jenjo02)
**	    Added CS_SID to CScnd_signal() function prototype.
**	2-Apr-2004 (mutma03)
**	    Added IIpoll_for_AST for threads which missed AST
**	    due to suspend/resume race condition
**	2-sep-2004 (mutma03)
**	    Removed IIpoll_for_AST and added IIsuspend_for_AST.
**	13-Feb-2007 (kschendel)
**	    Added CScancelCheck.
**	30-Aug-2007 (bonro01)
**	    Moved CS_MEMBAR_MACRO() to cs.h from csnormal.h because
**	    Windows does not include csnormal.h
**	5-Sep-2007 (kibro01)
**	    Corrected typo in MEMBAR macro name.
**	2-Jan-2008 (kibro01) b118591
**	    Add MEMBAR for Linux
**	22-Jan-2008 (kibro01) b118591
**	    Only add MEMBAR for versions of Linux with it in asm/system.h
**	23-Jan-2008 (kibro01) b118591
**	    Remove MEMBAR for Linux until one is found which is generic
**      16-Oct-2008 (hweho01)
**          Add CS_MEMBAR_MACRO for Solaris/SPARC platform.
**	21-nov-2008 (joea)
**	    Extend 4-Oct-2000 redefinition of CS_find_scb to VMS.    
**	24-nov-2008 (joea)
**	    Expand use of CS_FUNCTIONS to VMS.
**      20-Jan-2009 (hweho01)
**          Add CS_MEMBAR_MACRO for Tru64 platform (axp_osf).
**      18-mar-2009 (stegr01)
**          define CS_MEMBAR_MACRO for IA64 and ALpha VMS
**          define CS_LOCK_LONG and CS_UNLOCK_LONG for IA64 and Alpha VMS
**      20-apr-2010 (stephenb)
**          define CSadjust_i8counter. CSadjust_counter is defined twice
**          remove one of them.
**	17-may-2010 (stephenb)
**	    Fix typo in above change.
**/

/* Adjust counter */
#ifndef CSadjust_counter
/* not already defined in csnormal, we need a real function */
#define CSadjust_counter IICSadjust_counter
#define NEED_CSADJUST_COUNTER_FUNCTION
FUNC_EXTERN i4 CSadjust_counter(
            i4          *pcounter,
            i4           adjustment
);
#endif

#ifndef CSadjust_i8counter
/* not already defined in csnormal, we need a real function */
#define CSadjust_i8counter IICSadjust_i8counter
#define NEED_CSADJUST_I8COUNTER_FUNCTION
FUNC_EXTERN i8 CSadjust_i8counter( 
	    i8 *pcounter, 
	    i8 adjustment 
);
#endif

/* lock sem */
#define CSp_semaphore IICSp_semaphore
FUNC_EXTERN STATUS CSp_semaphore(
	    i4		exclusive,
	    CS_SEMAPHORE    *sem
);

/* unlock sem */
#define CSv_semaphore IICSv_semaphore
FUNC_EXTERN STATUS CSv_semaphore(
	    CS_SEMAPHORE *sem
);

/* init sem */
#define CSi_semaphore IICSi_semaphore
FUNC_EXTERN STATUS CSi_semaphore(
	    CS_SEMAPHORE *sem,
	    i4		type
);

/* remove sem */
#define CSr_semaphore IICSr_semaphore
FUNC_EXTERN STATUS CSr_semaphore(
	    CS_SEMAPHORE *sem
);

/* return semaphore statistics */
#define CSs_semaphore IICSs_semaphore
FUNC_EXTERN STATUS CSs_semaphore(
	    i4		 mode,
#define CS_INIT_SEM_STATS  0x00
#define CS_ROLL_SEM_STATS  0x01
#define CS_CLEAR_SEM_STATS 0x02
	    CS_SEMAPHORE *sem,
	    CS_SEM_STATS *stats,
	    i4		 length
);

#define CSa_semaphore IICSa_semaphore
FUNC_EXTERN STATUS CSa_semaphore( 
	    CS_SEMAPHORE *sem 
);

#define CSd_semaphore IICSd_semaphore
FUNC_EXTERN STATUS CSd_semaphore( 
	    CS_SEMAPHORE *sem 
);

#define CSn_semaphore IICSn_semaphore
FUNC_EXTERN VOID CSn_semaphore( 
	    CS_SEMAPHORE    *sem,
	    char	    *string
);

#define CSw_semaphore IICSw_semaphore
FUNC_EXTERN STATUS CSw_semaphore( 
	    CS_SEMAPHORE    *sem,
	    i4		    type,
	    char	    *string
);

/* fetch the session id */
#define CSget_sid IICSget_sid
FUNC_EXTERN VOID     CSget_sid(
	    CS_SID	*sidptr
);

#define CSfind_sid IICSfind_sid
FUNC_EXTERN CS_SID   CSfind_sid(
	    CS_SCB 	*scb
);

#define CSset_sid IICSset_sid
FUNC_EXTERN VOID CSset_sid(
	    CS_SCB	*scb
);

#define CSadd_thread IICSadd_thread
FUNC_EXTERN STATUS CSadd_thread(
	    i4		priority,
	    PTR		crb,
	    i4		thread_type,
	    CS_SID	*thread_id,
	    CL_ERR_DESC	*error
);

/* session interrupt ast */
#define CSattn IICSattn
FUNC_EXTERN VOID CSattn(
	    i4		eid,
	    CS_SID	sid
);

#define CScancelled IICScancelled
FUNC_EXTERN VOID CScancelled(
	    PTR		ecb
);

#define CScnd_broadcast IICScnd_broadcast
FUNC_EXTERN STATUS CScnd_broadcast(
	    CS_CONDITION *cnd 
);

#define CScnd_free IICScnd_free
FUNC_EXTERN STATUS CScnd_free(
	     CS_CONDITION *cnd 
);

#define CScnd_init IICScnd_init
FUNC_EXTERN STATUS CScnd_init(
	CS_CONDITION    *cnd
);

#define CScnd_signal IICScnd_signal
FUNC_EXTERN STATUS CScnd_signal(
	    CS_CONDITION *cnd,
	    CS_SID	 sid
);

#define CScnd_wait IICScnd_wait
FUNC_EXTERN STATUS CScnd_wait(
	    CS_CONDITION *cnd,
	    CS_SEMAPHORE *sem
);

/* fetch the scb */
#define CSget_scb IICSget_scb
FUNC_EXTERN VOID     CSget_scb(
	    CS_SCB	**scbptr
);	

#define CSfind_scb IICSfind_scb
FUNC_EXTERN CS_SCB  * CSfind_scb(
	    CS_SID	sid
);

#if defined(UNIX) || defined(VMS)
#define CS_find_scb IICS_find_scb
#endif /* UNIX || VMS */
FUNC_EXTERN CS_SCB  * CS_find_scb(
	    CS_SID	sid
);

/* fetch the cross-process session id */
#define CSget_cpid  IICSget_cpid
FUNC_EXTERN VOID     CSget_cpid(
	    CS_CPID 	*cpid
);

/* resume session */
#define CSresume IICSresume
FUNC_EXTERN VOID CSresume(
	    CS_SID	sid
);	

/* resume session from an AST/signal handler */
#define CSresume_from_AST IICSresume_from_AST
FUNC_EXTERN VOID CSresume_from_AST(
	    CS_SID	sid
);	

/* poll and signal any thread which missed as AST */
#define CSsuspend_for_AST  IICSsuspend_for_AST
FUNC_EXTERN STATUS CSsuspend_for_AST(
	    i4		mask,
	    i4		to_cnt,
	    PTR		ecb
);	

#define CSstatistics IICSstatistics
FUNC_EXTERN STATUS CSstatistics(
	    TIMERSTAT	*timer_block,
	    i4		server_stats
);

/* suspend current session */
#define CSsuspend IICSsuspend
FUNC_EXTERN STATUS     CSsuspend(
	    i4		mask,
	    i4		to_cnt,
	    PTR		ecb
);

#ifndef	CSswitch
#define CSswitch IICSswitch
FUNC_EXTERN	VOID CSswitch(
	    void
);
#endif

#define CSdispatch IICSdispatch
FUNC_EXTERN STATUS CSdispatch(
	    void
);

#define CSterminate IICSterminate
FUNC_EXTERN STATUS CSterminate(
	    i4		mode, 
	    i4		*active_count
);

#define CSintr_ack IICSintr_ack
FUNC_EXTERN VOID    CSintr_ack(
	    void
);

#define CSremove IICSremove
FUNC_EXTERN STATUS CSremove(
	    CS_SID	sid
);

#define CSaltr_session IICSaltr_session
FUNC_EXTERN STATUS CSaltr_session(
	    CS_SID	session_id,
	    i4		option, 
	    PTR		item
);

#define CSdump_statistics IICSdump_statistics
FUNC_EXTERN VOID CSdump_statistics(
	    i4	(*print_fcn)(char *, ...)
);

#define CSmonitor IICSmonitor
FUNC_EXTERN STATUS CSmonitor(
	    i4		mode,
	    CS_SCB	*scb,
	    i4		*next_mode,
	    char	*command,
	    i4		powerful,
	    i4		(*print_fcn)()
);

#define CS_is_mt IICS_is_mt
FUNC_EXTERN bool CS_is_mt( void );

#define CSalter IICSalter
FUNC_EXTERN STATUS CSalter(
	    CS_CB	*ccb
);

#define CScnd_get_name IICScnd_get_name
FUNC_EXTERN char * CScnd_get_name(
	    CS_CONDITION *cond
);

#define CScnd_name IICScnd_name
FUNC_EXTERN STATUS CScnd_name(
	    CS_CONDITION *cond,
	    char	*name
);

/* Cross-process resume */
#define CScp_resume IICScp_resume
FUNC_EXTERN VOID CScp_resume(
	    CS_CPID 	*cpid
);


/* fetch the server id */
#define CSget_svrid IICSget_svrid
FUNC_EXTERN VOID     CSget_svrid(
	    char	*cs_name
);

#define CSinitiate IICSinitiate
FUNC_EXTERN STATUS CSinitiate(
	    i4		*argc, 
	    char	***argv, 
	    CS_CB	*ccb 
);

#define CSremove_all_sems IICSremove_all_sems
FUNC_EXTERN STATUS CSremove_all_sems(
	    i4		    type
);

#define gen_Psem iigen_Psem
FUNC_EXTERN STATUS gen_Psem(
	    CS_SEMAPHORE    *sem
);
#define gen_Vsem iigen_Vsem
FUNC_EXTERN STATUS gen_Vsem(
	    CS_SEMAPHORE    *sem
);
#define gen_Isem iigen_Isem
FUNC_EXTERN STATUS gen_Isem(
	    CS_SEMAPHORE    *sem
);
#define gen_Nsem iigen_Nsem
FUNC_EXTERN STATUS gen_Nsem(
	    CS_SEMAPHORE    *sem,
	    char	    *name
);
#define gen_useCSsems iigen_useCSsems
FUNC_EXTERN VOID gen_useCSsems(
	    STATUS (*psem)(i4 excl, CS_SEMAPHORE *sem),
	    STATUS (*vsem)(CS_SEMAPHORE *sem),
	    STATUS (*isem)(CS_SEMAPHORE *sem, i4 type),
	    VOID   (*nsem)(CS_SEMAPHORE *sem, char *string)
);

#define CSkill IICSkill
FUNC_EXTERN STATUS CSkill(
	    CS_SID	    sid,
	    bool	    force,
	    u_i4	    waittime
);

#ifndef	CSnoresnow
#define CSnoresnow IICSnoresnow
FUNC_EXTERN     VOID CSnoresnow(
            char  *descrip,
            int   pos
);
#endif /* CSnoresnow */

#define CSdump_stack IICSdump_stack
FUNC_EXTERN VOID CSdump_stack( void );



#define CScancelCheck IICScancelCheck
FUNC_EXTERN void CScancelCheck(CS_SID sid);


# if defined(OS_THREADS_USED) && (defined(UNIX) || defined(VMS))
/*
** Define pointer-vector array for CS functions which
** have Ingres-threaded and OS-threaded components.
**
** Ingres-threaded functions are prefixed with "IICS"
** OS-threaded functions are prefixed with "IICSMT"
**
** Note that the Ingres-threaded functions themselves
** must be named starting with "IICS" as the define
** for CS to IICS will be dropped.
**
** If configured for Ingres threads, these will be filled
** with pointers to the IICS functions; if
** configured for OS threads, they will contain pointers
** to the corresponding IICSMT functions.
**
** The pointer array is defined as a static in 
** csinterface.c, is initialized one way or the 
** other by CS(MT)initiate, and is addressed 
** through the GLOBALREF pointer "Cs_fvp", also
** defined in csinterface.c.
*/
typedef struct	_CS_FUNCTIONS	CS_FUNCTIONS;
GLOBALREF	CS_FUNCTIONS	*Cs_fvp;

struct	_CS_FUNCTIONS {
    STATUS	(*IIp_semaphore)(
		    i4		 exclusive,
		    CS_SEMAPHORE *sem);
    STATUS	(*IIv_semaphore)(
		    CS_SEMAPHORE *sem);
    STATUS	(*IIi_semaphore)(
		    CS_SEMAPHORE *sem, i4 type);
    STATUS	(*IIr_semaphore)(
		    CS_SEMAPHORE *sem);
    STATUS	(*IIs_semaphore)(
		    i4		 mode,
		    CS_SEMAPHORE *sem,
		    CS_SEM_STATS *stats,
		    i4		 length);
    STATUS	(*IIa_semaphore)(
		    CS_SEMAPHORE *sem);
    STATUS	(*IId_semaphore)(
		    CS_SEMAPHORE *sem);
    VOID	(*IIn_semaphore)(
		    CS_SEMAPHORE *sem,
		    char	 *string);
    STATUS	(*IIw_semaphore)(
		    CS_SEMAPHORE *sem,
		    i4		 type,
		    char	 *string);
    
    VOID        (*IIget_sid)(
		    CS_SID	*sidptr);
    CS_SID   	(*IIfind_sid)(
		    CS_SCB 	*scb);
    VOID 	(*IIset_sid)(
		    CS_SCB	*scb);
    VOID     	(*IIget_scb)(
		    CS_SCB	**scbptr);	
    CS_SCB     *(*IIfind_scb)(
		    CS_SID	sid);
    CS_SCB     *(*II_find_scb)(
		    CS_SID	sid);
    VOID     	(*IIget_cpid)(
		    CS_CPID 	*cpid);
    VOID 	(*IIswitch)(
		    void);
    STATUS      (*IIsuspend)(
		    i4		mask,
		    i4		to_cnt,
		    PTR		ecb);
    VOID 	(*IIresume)(
		    CS_SID	sid);	
    STATUS 	(*IIstatistics)(
		    TIMERSTAT	*timer_block,
		    i4		server_stats);
    STATUS 	(*IIadd_thread)(
		    i4		priority,
		    PTR		crb,
		    i4		thread_type,
		    CS_SID	*thread_id,
		    CL_ERR_DESC	*error);
    VOID 	(*IIcancelled)(
		    PTR		ecb);

    STATUS 	(*IIcnd_init)(
		    CS_CONDITION    *cnd);
    STATUS 	(*IIcnd_free)(
		     CS_CONDITION *cnd);
    STATUS 	(*IIcnd_wait)(
		    CS_CONDITION *cnd,
		    CS_SEMAPHORE *sem);
    STATUS 	(*IIcnd_signal)(
		    CS_CONDITION *cnd,
		    CS_SID	 sid);
    STATUS 	(*IIcnd_broadcast)(
		    CS_CONDITION *cnd);

    VOID 	(*IIattn)(
		    i4		eid,
		    CS_SID	sid);
    VOID    	(*IIintr_ack)(
		    void);
    STATUS 	(*IIremove)(
		    CS_SID	sid);
    STATUS 	(*IIaltr_session)(
		    CS_SID	session_id,
		    i4		option, 
		    PTR		item);
    STATUS 	(*IIdispatch)(
		    void);
    STATUS 	(*IIterminate)(
		    i4		mode, 
		    i4		*active_count);
    VOID 	(*IIdump_statistics)(
		    i4	(*print_fcn)(char *, ...));
    STATUS 	(*IImonitor)(
		    i4		mode,
		    CS_SCB	*scb,
		    i4		*next_mode,
		    char	*command,
		    i4		powerful,
		    i4		(*print_fcn)());
    VOID 	(*IIresume_from_AST)(
		    CS_SID	sid);	
    STATUS 	(*IIsuspend_for_AST)(
		    i4		mask,
		    i4		to_cnt,
		    PTR		ecb);
    void	(*IIcancelCheck)(
		    CS_SID	sid);
};

#undef  CSp_semaphore
#define CSp_semaphore   	Cs_fvp->IIp_semaphore
#define CSMTp_semaphore		IICSMTp_semaphore
FUNC_EXTERN STATUS CSMTp_semaphore(
	    i4		exclusive,
	    CS_SEMAPHORE    *sem
);

#undef  CSv_semaphore
#define CSv_semaphore   	Cs_fvp->IIv_semaphore
#define CSMTv_semaphore		IICSMTv_semaphore
FUNC_EXTERN STATUS CSMTv_semaphore(
	    CS_SEMAPHORE    *sem
);

#undef  CSi_semaphore
#define CSi_semaphore   	Cs_fvp->IIi_semaphore
#define CSMTi_semaphore		IICSMTi_semaphore
FUNC_EXTERN STATUS CSMTi_semaphore(
	    CS_SEMAPHORE *sem,
	    i4		type
);

#undef  CSr_semaphore
#define CSr_semaphore   	Cs_fvp->IIr_semaphore
#define CSMTr_semaphore		IICSMTr_semaphore
FUNC_EXTERN STATUS CSMTr_semaphore(
	    CS_SEMAPHORE    *sem
);

#undef  CSs_semaphore
#define CSs_semaphore   	Cs_fvp->IIs_semaphore
#define CSMTs_semaphore		IICSMTs_semaphore
FUNC_EXTERN STATUS CSMTs_semaphore(
	    i4		 mode,
	    CS_SEMAPHORE *sem,
	    CS_SEM_STATS *stats,
	    i4		 length
);

#undef  CSa_semaphore
#define CSa_semaphore   	Cs_fvp->IIa_semaphore
#define CSMTa_semaphore		IICSMTa_semaphore
FUNC_EXTERN STATUS CSMTa_semaphore(
	    CS_SEMAPHORE    *sem
);

#undef  CSd_semaphore
#define CSd_semaphore   	Cs_fvp->IId_semaphore
#define CSMTd_semaphore		IICSMTd_semaphore
FUNC_EXTERN STATUS CSMTd_semaphore(
	    CS_SEMAPHORE    *sem
);

#undef  CSn_semaphore
#define CSn_semaphore   	Cs_fvp->IIn_semaphore
#define CSMTn_semaphore		IICSMTn_semaphore
FUNC_EXTERN VOID CSMTn_semaphore( 
	    CS_SEMAPHORE    *sem,
	    char	    *string
);

#undef  CSw_semaphore
#define CSw_semaphore   	Cs_fvp->IIw_semaphore
#define CSMTw_semaphore		IICSMTw_semaphore
FUNC_EXTERN STATUS CSMTw_semaphore( 
	    CS_SEMAPHORE    *sem,
	    i4		    type,
	    char	    *string
);

#undef  CSget_sid
#define CSget_sid 		Cs_fvp->IIget_sid
#define CSMTget_sid		IICSMTget_sid
FUNC_EXTERN VOID     CSMTget_sid(
	    CS_SID	*sidptr
);
#undef  CSfind_sid
#define CSfind_sid 		Cs_fvp->IIfind_sid
#define CSMTfind_sid		IICSMTfind_sid
FUNC_EXTERN CS_SID   CSMTfind_sid(
	    CS_SCB 	*scb
);
#undef  CSset_sid
#define CSset_sid 		Cs_fvp->IIset_sid
#define CSMTset_sid		IICSMTset_sid
FUNC_EXTERN VOID CSMTset_sid(
	    CS_SCB	*scb
);

#undef  CSget_scb
#define CSget_scb 		Cs_fvp->IIget_scb
#define CSMTget_scb		IICSMTget_scb
FUNC_EXTERN VOID     CSMTget_scb(
	    CS_SCB	**scbptr
);	
#undef  CSfind_scb
#define CSfind_scb 		Cs_fvp->IIfind_scb
#define CSMTfind_scb		IICSMTfind_scb
FUNC_EXTERN CS_SCB  * CSMTfind_scb(
	    CS_SID	sid
);
#undef  CS_find_scb
#define CS_find_scb 		Cs_fvp->II_find_scb
#define CSMT_find_scb		IICSMT_find_scb
FUNC_EXTERN CS_SCB  * CSMT_find_scb(
	    CS_SID	sid
);

#undef  CSget_cpid
#define CSget_cpid 		Cs_fvp->IIget_cpid
#define CSMTget_cpid		IICSMTget_cpid
FUNC_EXTERN VOID     CSMTget_cpid(
	    CS_CPID 	*cpid
);

#undef  CSswitch
#define CSswitch 		Cs_fvp->IIswitch
#define CSMTswitch		IICSMTswitch
FUNC_EXTERN	VOID CSMTswitch(
	    void
);

#undef  CSsuspend
#define CSsuspend 		Cs_fvp->IIsuspend
#define CSMTsuspend		IICSMTsuspend
FUNC_EXTERN STATUS     CSMTsuspend(
	    i4		mask,
	    i4		to_cnt,
	    PTR		ecb
);

#undef  CSresume
#define CSresume 		Cs_fvp->IIresume
#define CSMTresume		IICSMTresume
FUNC_EXTERN VOID CSMTresume(
	    CS_SID	sid
);	

#undef  CSresume_from_AST
#define CSresume_from_AST 		Cs_fvp->IIresume_from_AST
#define CSMTresume_from_AST		IICSMTresume_from_AST
FUNC_EXTERN VOID CSMTresume_from_AST(
	    CS_SID	sid
);	

#undef  CSsuspend_for_AST
#define CSsuspend_for_AST 			Cs_fvp->IIsuspend_for_AST
#define CSMTsuspend_for_AST		IICSMTsuspend_for_AST
FUNC_EXTERN STATUS CSMTsuspend_for_AST(
	    i4		mask,
	    i4		to_cnt,
	    PTR		ecb
); 

#undef  CSstatistics
#define CSstatistics 		Cs_fvp->IIstatistics
#define CSMTstatistics		IICSMTstatistics
FUNC_EXTERN STATUS CSMTstatistics(
	    TIMERSTAT	*timer_block,
	    i4		server_stats
);

#undef  CSadd_thread
#define CSadd_thread 		Cs_fvp->IIadd_thread
#define CSMTadd_thread		IICSMTadd_thread
FUNC_EXTERN STATUS CSMTadd_thread(
	    i4		priority,
	    PTR		crb,
	    i4		thread_type,
	    CS_SID	*thread_id,
	    CL_ERR_DESC	*error
);

#undef  CScancelled
#define CScancelled 		Cs_fvp->IIcancelled
#define CSMTcancelled		IICSMTcancelled
FUNC_EXTERN VOID CSMTcancelled(
	    PTR		ecb
);

#undef  CScnd_init
#define CScnd_init 		Cs_fvp->IIcnd_init
#define CSMTcnd_init		IICSMTcnd_init
FUNC_EXTERN STATUS CSMTcnd_init(
	CS_CONDITION    *cnd
);

#undef  CScnd_free
#define CScnd_free 		Cs_fvp->IIcnd_free
#define CSMTcnd_free		IICSMTcnd_free
FUNC_EXTERN STATUS CSMTcnd_free(
	     CS_CONDITION *cnd 
);

#undef  CScnd_wait
#define CScnd_wait 		Cs_fvp->IIcnd_wait
#define CSMTcnd_wait		IICSMTcnd_wait
FUNC_EXTERN STATUS CSMTcnd_wait(
	    CS_CONDITION *cnd,
	    CS_SEMAPHORE *sem
);

#undef  CScnd_signal
#define CScnd_signal 		Cs_fvp->IIcnd_signal
#define CSMTcnd_signal		IICSMTcnd_signal
FUNC_EXTERN STATUS CSMTcnd_signal(
	    CS_CONDITION *cnd,
	    CS_SID	 sid
);

#undef  CScnd_broadcast
#define CScnd_broadcast 	Cs_fvp->IIcnd_broadcast
#define CSMTcnd_broadcast	IICSMTcnd_broadcast
FUNC_EXTERN STATUS CSMTcnd_broadcast(
	    CS_CONDITION *cnd 
);

#undef  CSattn
#define CSattn 			Cs_fvp->IIattn
#define CSMTattn		IICSMTattn
FUNC_EXTERN VOID CSMTattn(
	    i4		eid,
	    CS_SID	sid
);

#undef  CSintr_ack
#define CSintr_ack 		Cs_fvp->IIintr_ack
#define CSMTintr_ack		IICSMTintr_ack
FUNC_EXTERN VOID    CSMTintr_ack(
	    void
);

#undef  CSremove
#define CSremove 		Cs_fvp->IIremove
#define CSMTremove		IICSMTremove
FUNC_EXTERN STATUS CSMTremove(
	    CS_SID	sid
);

#undef  CSaltr_session
#define CSaltr_session 		Cs_fvp->IIaltr_session
#define CSMTaltr_session	IICSMTaltr_session
FUNC_EXTERN STATUS CSMTaltr_session(
	    CS_SID	session_id,
	    i4		option, 
	    PTR		item
);

#undef  CSdispatch
#define CSdispatch 		Cs_fvp->IIdispatch
#define CSMTdispatch		IICSMTdispatch
FUNC_EXTERN STATUS CSMTdispatch(
	    void
);

#undef  CSterminate
#define CSterminate 		Cs_fvp->IIterminate
#define CSMTterminate		IICSMTterminate
FUNC_EXTERN STATUS CSMTterminate(
	    i4		mode, 
	    i4		*active_count
);

#undef  CSdump_statistics
#define CSdump_statistics 	Cs_fvp->IIdump_statistics
#define CSMTdump_statistics	IICSMTdump_statistics
FUNC_EXTERN VOID CSMTdump_statistics(
	    i4	(*print_fcn)(char *, ...)
);

#undef  CSmonitor
#define CSmonitor 		Cs_fvp->IImonitor
#define CSMTmonitor		IICSMTmonitor
FUNC_EXTERN STATUS CSMTmonitor(
	    i4		mode,
	    CS_SCB	*scb,
	    i4		*next_mode,
	    char	*command,
	    i4		powerful,
	    i4		(*print_fcn)()
);

#define CSMTinitiate IICSMTinitiate
FUNC_EXTERN STATUS CSMTinitiate(
	    i4		*argc, 
	    char	***argv, 
	    CS_CB	*ccb 
);

#undef CScancelCheck
#define CScancelCheck Cs_fvp->IIcancelCheck
#define CSMTcancelCheck IICSMTcancelCheck
FUNC_EXTERN void IICSMTcancelCheck(CS_SID sid);

# endif /* OS_THREADS_USED && (UNIX || VMS) */

FUNC_EXTERN STATUS IICSmon_register( char *prefix,
                              i4 (*entry_fcn)(i4 (*)(PTR,i4,char*), char *) );

FUNC_EXTERN STATUS IICSmon_deregister( char *prefix );

/* Oct/16/2008 
** NOTE: On Solaris/SPARC, the CS_MEMBAR_MACRO can be implemented by     
** using the C function calls (such as membar_enter()...) if Ingres is  
** built on OS release 10 or above. The current Ingres release (9.2) is 
** built on Solaris 8, the macro is made by using assembly instruction  
** membar which is only available in SPARC architecture version 9.  
** (or in v8plus which is 32-bit but doesn't run on sun4m.)
** The instruction sets in the current build environment are used as :      
** SPARC V8+ is used for 32-bit build by compiler default;  
** (so that su4_us5 is only runnable on UltraSPARC 32 bit; the old
** sun4c / sun4m architectures are no longer supported.)
** SPARC V9 is used for 64-bit build by -xarch specification.
*/ 
# if defined(any_aix)
# define CS_MEMBAR_MACRO() __sync()
# elif defined(i64_hpu)
# define CS_MEMBAR_MACRO() _Asm_mf(_UP_MEM_FENCE | _DOWN_MEM_FENCE)
# elif defined(sparc_sol) && defined(BUILD_ARCH64)
# define CS_MEMBAR_MACRO() \
         __asm("membar #LoadLoad | #StoreLoad | #LoadStore | #StoreStore")
# elif defined(a64_lnx)
# define CS_MEMBAR_MACRO()    asm volatile("mfence":::"memory")
# elif defined(axp_osf)
# define CS_MEMBAR_MACRO()	asm ("mb")
# elif (defined(i64_vms) || defined(axm_vms))
# define CS_MEMBAR_MACRO() __MB() 
# else
# define CS_MEMBAR_MACRO()
# endif

# if defined(i64_vms)
# define CS_LOCK_LONG(p)  __LOCK_LONG(p)
# define CS_UNLOCK_LONG(p) __UNLOCK_LONG(p)
# elif defined(axm_vms)
# define CS_LOCK_LONG(p) *p = 1; __MB() 
# define CS_UNLOCK_LONG(p) __MB(); *p = 0 
# endif
 

# endif /* CS_HDR_INCLUDED */
