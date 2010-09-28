/*
** Copyright (c) 2001, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <pc.h>
#include    <cs.h>
#include    <er.h>
#include    <lk.h>
#include    <tr.h>
#include    <cx.h>
#include    <st.h>
#include    <cxprivate.h>
#include    <me.h>
#ifdef VMS
#include <starlet.h>
#include <efndef.h>
#include <iledef.h>
#include <iosbdef.h>
#include <vmstypes.h>
#include <astjacket.h>
#endif

/*
**
**  Name: CXDLM.C - CX 'D'istributed 'L'ock 'M'anager functions.
**
**  Description:
**      This module contains the CX routines which provide the
**	generic interface to either a vendor supplied DLM, or
**	the Ingres DLM implementation.
**
**	Please see design doc for SIR 103715 for overview.
**
**	Routines here can be only be called if CXconnect was successful.
**
**	Public routines
**
**          CXdlm_request 	- Request a new DLM lock.
**          CXdlm_convert 	- Convert a DLM lock to a (possibly) new mode.
**          CXdlm_release	- Release a DLM lock.
**          CXdlm_cancel 	- Cancel a DLM lock request or conversion.
**          CXdlm_wait	 	- Wait for completion of asynchronous request.
**	    CXdlm_get_blki	- Obtain info about holders of a resource.
**	    CXdlm_mark_as_deadlocked - Mark request as deadlock victim.
**	    CXdlm_glc_support	- Body of GLC thread.
**
**	Debug routines
**
**	    CXdlm_dump_lk	- Dump lock info
**	    CXdlm_dump_both	- Dump lock & associated resource
**
**	Internal functions
**
**	    OS specific completion routines, & blocking routine wrappers.
**
**	Conditional Compile defines
**
**	    CX_DLM_ENABLE_TRACE - if defined code to trace CX DLM
**				  activity using TRdisplay will be
**				  included.  This has significant
**				  overhead.
**
**	    CX_PARANOIA -	  Perform parameter and consistency
**				  checks that should not be needed
**				  in a mature tested product.  That
**				  is to say that CX_PARANOIA should
**				  be asserted during development, but
**				  undefined for the final build passed
**				  to release QA.
**
**  History:
**      15-feb-2001 (devjo01)
**          Created.
**	24-oct-2003 (kinte01)
**	    Add missing me.h header file
**	08-jan-2004 (devjo01)
**	    Linux support.
**	04-feb-2004 (devjo01)
**	    Tweak trace macros, and add an additional CX_PARANOIA
**	    check within CXdlm_wait.
**	04-May-2004 (hanje04)
**	    Only pull in DLM specific code on Linux if we're building
**	    conf_CLUSTER_BUILD. Regular Linux builds should not include
**	    this code.
**	07-May-2004 (devjo01)
**	    hanje04 inadvertently used CLUSTER_ENABLED where
**	    conf_CLUSTER_BUILD was intended.  Corrected this, and
**	    reworked logic so this is used for all platforms.
**      07-Sep-2004 (mutma03)
**          Called CSsuspend_for_AST instead of CSsuspend to avoid the
**          the race condition caused by CScond_signal when called
**        from AST. CSsuspend_for_AST uses sem_wait.
**      20-sep-2004 (devjo01)
**	    Cumulative changes for Linux Beta.
**          - Support for CX_F_NAB. (Notify & Block flag)
**          - Support for CX_F_LSN_IN_LVB.  (Pass internal CX_LSN
**	    around in high portion of LVB.
**	    - Add CXdlm_lock_key_to_string.
**	14-dec-2004 (devjo01)
**	    CX_F_NO_DOMAIN support.
**      14-Mar-2005 (mutma03/devjo01)
**          Fix for sem count issue with CXsuspend_for_AST/CXdlm_cancel.
**	18-mar-2005 (devjo01)
**	    - Start adding VMS support.  Search for _VMS_ to locate sections.
**	    - Correct logic lapse in CXdlm_wait, where if a lock time-out
**	      expires, but the lock is granted before we can cancel it,
**	      we fail to "consume" AST notification.   This "missed"
**	      AST resume will then cause the next suspend for AST to
**	      immediately return.
**    11-may-2005 (stial01)
**        Changed sizeof LK_CSP lock from 8 to 12 (3 * sizeof(i4))
**	10-may-2005 (devjo01)
**	    - Add CXdlm_get_bli as part of VMS distributed deadlock support.
**	    - Add cx_ingres_lk_compat for use in detecting cvt-cvt deadlock.
**	    - Add CXdlm_mark_as_deadlocked for VMS distributed deadlock.
**	    - Use LCK$M_EXPEDITE for VMS NULL locks.
**	31-may-2005 (devjo01)
**	    Quiet compiler warnings by making implementation of 
**	    CXdlm_get_bli consonant with declaration in cx.h.
**	21-jun-2005 (devjo01)
**	    Redirect lock requests under VMS if (1) lock is a group lock,
**	    but process operating on this is not the original allocator 
**	    (VMS only has process owned locks, and the group lock container
**	    is being simulated); or (2) Process is in a different UIC group.
**	    (Different groups implicitly use different namespaces under VMS,
**	    so those utilities which take locks dirrectly, but are not
**	    necessarilly run as Ingres (I.e. JSP stuff) must  have the CSP
**	    take locks on their behalf.
**	23-Sep-2005 (hanje04)
**	    Move prototype for cx_dlm_sync_lsn_from_dlm() outside 
**	    conf_CLUSTER_BUILD as function its self is.
**      30-Jun-2005 (hweho01)
**          Make inclusion of CXdlm_is_blocking() function dependent on
**          conf_CLUSTER_BUILD define string  for axp_osf platform.
**	10-oct-2005 (devjo01)
**	    In CXdlm_convert, add CX_F_OWNSET logic to VMS implementation,
**	    even if PID cannot change, session to be resumed may.
**	16-nov-2005 (devjo01)
**	    - Rename CXdlm_mark_as_deadlocked to CXdlm_mark_as_posted to
**	      reflect more generic usage.
**	    - Correct flaw in CXdlm_get_blki where "queue" was not being
**	      set correctly.
**	28-nov-2005 (devjo01) b115583
**	    Add use of CX_F_IS_CSP flag within CXdlm_request to specify
**	    that this request must be reflected to the CSP.
**	27-oct-2006 (abbjo03)
**	    Add missing VMS headers.  Use IOSB in sys$getlki call.  Add static
**	    prototype of cx_vms_pass_dlm_req.  In cx_vms_free_glm, declare
**	    cachehead as volatile.
**	02-jan-2007 (abbjo03)
**	    Add cx_ingres_lk_convert array and use it in CXdlm_get_blki to
**	    parallel the LK mode conversions.
**	18-jan-2007 (jenjo02/abbjo03)
**	    In CXdlm_get_blki, adjust the requested mode if the lock has been
**	    granted in a conversion.  Include granted locks in max request mode
**	    calculation as they are contributing to the block.
**	23-Feb-2007 (jonj)
**	    In cx_find_glc, check that searched-for PID is still alive,
**	    cleanup its GLC if not, return NULL.
**	23-Jul-2008 (jonj)
**	    Add VERIFY_LOCK macro and underlying static function to
**	    aid debugging of VMS proxy locks.
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
**      03-sep-2010 (joea)
**          In CXdlm_get_blki, if a lock has been granted, update rqmode.
*/


/*
**	OS independent declarations
*/

#ifdef CX_DLM_ENABLE_TRACE

STATUS CXdlm_dump_both( i4 (*)(PTR, i4, char *), CX_REQ_CB * );

static char ingres_lk_modes[] = "NL,IS,IX,S,SIX,U,X\0";

static u_i4 cx_dlm_trace_seqn = 0; /* Don't protect update of this */

#define CX_DLM_TRACE(where, preq) \
    if ((CX_Proc_CB.cx_flags|flags) & CX_F_DLM_TRACE) { \
	(void)TRdisplay(ERx(">CX [%d] %s:%d --- %d:%x ---\n"), \
	 ++cx_dlm_trace_seqn, where, __LINE__, (preq)->cx_owner.pid,
	 (preq)->cx_owner.sid ); \
	CXdlm_dump_both( NULL, preq ); \
	(void)TRdisplay( \
	 ERx("< status = %x, modes = %w -> %w, lki_flags = %x\n"), \
	 (preq)->cx_status, ingres_lk_modes, (preq)->cx_old_mode, \
	 ingres_lk_modes, (preq)->cx_new_mode, (preq)->cx_lki_flags ); \
    }

#define CX_DLM_TRACE_TERSE(where, preq) \
    if (CX_Proc_CB.cx_flags & CX_F_DLM_TRACE) { \
	(void)TRdisplay( \
	 ERx("-CX [%d] %s:%d --- %d:%x --- LkID = %x:%x status = %x\n"), \
	 ++cx_dlm_trace_seqn, where, __LINE__, \
	 (preq)->cx_owner.pid, (preq)->cx_owner.sid, \
	 (preq)->cx_lock_id.lk_uhigh, (preq)->cx_lock_id.lk_ulow, \
	 (preq)->cx_status ); \
    }

#define CX_DLM_TRACE_STATUS(where, preq, statvar) \
    if (CX_Proc_CB.cx_flags & CX_F_DLM_TRACE) { \
	(void)TRdisplay( \
	 ERx("-CX [%d] %s:%d --- %d:%x --- LkID = %x:%x status = %x\n"), \
	 ++cx_dlm_trace_seqn, where, __LINE__, \
	 (preq)->cx_owner.pid, (preq)->cx_owner.sid, \
	 (preq)->cx_lock_id.lk_uhigh, (preq)->cx_lock_id.lk_ulow, \
	 statvar ); \
    }

#else
#define CX_DLM_TRACE(where,preq)
#define CX_DLM_TRACE_TERSE(where, preq)
#define CX_DLM_TRACE_STATUS(where, preq, statvar)
#endif

#undef	VERIFY_VMS_LOCKS	/* define this here to verify VMS locks */

#if defined(VMS) && defined(conf_CLUSTER_BUILD) && defined(VERIFY_VMS_LOCKS)
#define	VERIFY_LOCK(preq,cstatus,flags) \
	CXdlm_verify_lock(preq,cstatus,flags,__FILE__,__LINE__)
#else
#define VERIFY_LOCK(preq,cstatus,flags)
#endif

#define CX_LSN_IN_LVB_PTR(r) \
    ((CX_LSN *)(((PTR)(&(r)->cx_value))+sizeof(CX_VALUE)-sizeof(CX_LSN)))

GLOBALREF	CX_PROC_CB	CX_Proc_CB;
GLOBALREF	CX_STATS	CX_Stats;

/*
** Ingres lock mode conversions.  This must match data in LK_convert_mode.
**
** convert_mode =  cx_ingres_lk_convert[grant_mode] [request_mode];
*/
static  i4      cx_ingres_lk_convert [8][8] =

{
/*                N       IS      IX      S       SIX       U      X    */
/* N   */    { LK_N,   LK_IS,  LK_IX,  LK_S,   LK_SIX,    LK_U,  LK_X },
/* IS  */    { LK_IS,  LK_IS,  LK_IX,  LK_S,   LK_SIX,    LK_U,  LK_X },
/* IX  */    { LK_IX,  LK_IX,  LK_IX,  LK_SIX, LK_SIX,    LK_X,  LK_X },
/* S   */    { LK_S,   LK_S,   LK_SIX, LK_S,   LK_SIX,    LK_U,  LK_X },
/* SIX */    { LK_SIX, LK_SIX, LK_SIX, LK_SIX, LK_SIX,    LK_SIX,LK_X },
/* U   */    { LK_U,   LK_U,   LK_X,   LK_S,   LK_SIX,    LK_U,  LK_X },
/* X   */    { LK_X,   LK_X,   LK_X,   LK_X,   LK_X,      LK_X,  LK_X }
};                                  /* Table [grant mode][request mode]
                                    ** to new mode */

#if defined(conf_CLUSTER_BUILD)

/* Lookup # significant bytes in LK_LOCK_KEY indexed by lk_type - 1 */
/* <INC> put correct maximum sizes for non-CX Ingres types */
static	i4	cx_reslen_tab[] =
{
	/* LK_DATABASE          1	*/	sizeof(LK_LOCK_KEY),
	/* LK_TABLE             2	*/	sizeof(LK_LOCK_KEY),
	/* LK_PAGE              3	*/	sizeof(LK_LOCK_KEY),
	/* LK_EXTEND_FILE       4	*/	sizeof(LK_LOCK_KEY),
	/* LK_BM_PAGE           5	*/	sizeof(LK_LOCK_KEY),
	/* LK_CREATE_TABLE      6	*/	sizeof(LK_LOCK_KEY),
	/* LK_OWNER_ID          7	*/	sizeof(LK_LOCK_KEY),
	/* LK_CONFIG            8	*/	sizeof(LK_LOCK_KEY),
	/* LK_DB_TEMP_ID        9	*/	sizeof(LK_LOCK_KEY),
	/* LK_SV_DATABASE      10	*/	sizeof(LK_LOCK_KEY),
	/* LK_SV_TABLE         11	*/	sizeof(LK_LOCK_KEY),
	/* LK_SS_EVENT         12	*/	sizeof(LK_LOCK_KEY),
	/* LK_TBL_CONTROL      13	*/	sizeof(LK_LOCK_KEY),
	/* LK_JOURNAL          14	*/	sizeof(LK_LOCK_KEY),
	/* LK_OPEN_DB          15	*/	sizeof(LK_LOCK_KEY),
	/* LK_CKP_DB           16	*/	sizeof(LK_LOCK_KEY),
	/* LK_CKP_CLUSTER      17	*/	sizeof(LK_LOCK_KEY),
	/* LK_BM_LOCK          18	*/	sizeof(LK_LOCK_KEY),
	/* LK_BM_DATABASE      19	*/	sizeof(LK_LOCK_KEY),
	/* LK_BM_TABLE         20	*/	sizeof(LK_LOCK_KEY),
	/* LK_CONTROL          21	*/	sizeof(LK_LOCK_KEY),
	/* LK_EVCONNECT        22	*/	sizeof(LK_LOCK_KEY),
	/* LK_AUDIT            23	*/	sizeof(LK_LOCK_KEY),
	/* LK_ROW              24	*/	sizeof(LK_LOCK_KEY),
	/* LK_CKP_TXN          25	*/	sizeof(LK_LOCK_KEY),
	/* LK_PH_PAGE          26	*/	sizeof(LK_LOCK_KEY),
	/* LK_VAL_LOCK         27	*/	sizeof(LK_LOCK_KEY),
	/* LK_SEQUENCE         28	*/	sizeof(LK_LOCK_KEY),
	/* LK_XA_CONTROL       29       */      sizeof(LK_LOCK_KEY),
	/* LK_CSP              30	*/	12,
	/* LK_CMO              31	*/	8,
	/* LK_MSG              32	*/	8,
	/* LK_ONLN_MDFY	       33	*/	sizeof(LK_LOCK_KEY),
};

/*
** Ingres lock compatibilities.  This must match data in LK_compatible.
** 
** Test using (cx_ingres_lk_compat[heldmode] | (1 << reqmode))
*/
static	i4	cx_ingres_lk_compat[] = {
	/* 0:LK_N   */ (1<<LK_N)|(1<<LK_IS)|(1<<LK_IX)|(1<<LK_S)|
	               (1<<LK_SIX)|(1<<LK_U)|(1<<LK_X),
	/* 1:LK_IS  */ (1<<LK_N)|(1<<LK_IS)|(1<<LK_IX)|(1<<LK_S)|(1<<LK_SIX),
	/* 2:LK_IX  */ (1<<LK_N)|(1<<LK_IS)|(1<<LK_IX),
	/* 3:LK_S   */ (1<<LK_N)|(1<<LK_IS)|(1<<LK_S)|(1<<LK_U),
	/* 4:LK_SIX */ (1<<LK_N)|(1<<LK_IS),
	/* 5:LK_U   */ (1<<LK_N),
	/* 6:LK_X   */ (1<<LK_N)
	};

#ifdef	axp_osf

/*
**	Tru64 specific declarations
*/

/* Lookup equivalent Tru64 lock mode, indexed by Ingres lock mode. */
static  i4      cxaxp_lk_to_dlm_mode[] =
{
	/* LK_N            	0	*/	DLM_NLMODE,
	/* LK_IS           	1	*/	DLM_CRMODE,
	/* LK_IX           	2	*/	DLM_CWMODE,
	/* LK_S            	3	*/	DLM_PRMODE,
	/* LK_SIX          	4	*/	DLM_PWMODE,
	/* LK_U            	5	*/	DLM_EXMODE,
	/* LK_X            	6	*/	DLM_EXMODE,
};

/* Lookup equivalent Ingres lock mode, indexed by DLM lock mode. */
static	i4	cxaxp_dlm_to_lk_mode[] =
{
	/* DLM_NOMODE		0	*/	0,	/* unused */
	/* DLM_NLMODE		1	*/	LK_N,
	/* DLM_CRMODE		2	*/	LK_IS,
	/* DLM_CWMODE		3	*/	LK_IX,
	/* DLM_PRMODE		4	*/	LK_S,
	/* DLM_PWMODE		5	*/	LK_SIX,
	/* DLM_EXMODE		6	*/	LK_X,
};

/* 
** Lookup table for legal Tru64 convert wait combinations.
** Index = old mode.  Bit positions = shifted new mode.  
** If bit set DLM_QUECVT s/b specified for dlm_cvt, dlm_quecvt.
*/
static  i4	cxaxp_waitcvts[] =
{
	/* LK_N	  0 */	(1<<LK_IS)|(1<<LK_IX)|(1<<LK_S)|
			(1<<LK_SIX)|(1<<LK_U)|(1<<LK_X),
	/* LK_IS  1 */	(1<<LK_IX)|(1<<LK_S)|
			(1<<LK_SIX)|(1<<LK_U)|(1<<LK_X),
	/* LK_IX  2 */	(1<<LK_SIX)|(1<<LK_X),
	/* LK_S   3 */	(1<<LK_SIX)|(1<<LK_X),
	/* LK_SIX 4 */	0x0,
	/* LK_U   5 */	0x0,
	/* LK_X   6 */	0x0,
};

/*
** Lookup table for incompatible Tru64 lock modes.
*/
static	i4	cxaxp_incompatable_modes[] =
{
	/* DLM_NOMODE */ 0,
	/* DLM_NLMODE */ 0,
	/* DLM_CRMODE */ (1<<DLM_EXMODE),
	/* DLM_CWMODE */ (1<<DLM_PRMODE)|(1<<DLM_PWMODE)|(1<<DLM_EXMODE),
	/* DLM_PRMODE */ (1<<DLM_CWMODE)|(1<<DLM_PWMODE)|(1<<DLM_EXMODE),
	/* DLM_PWMODE */ (1<<DLM_CWMODE)|(1<<DLM_PRMODE)|
			 (1<<DLM_PWMODE)|(1<<DLM_EXMODE),
	/* DLM_EXMODE */ (1<<DLM_CRMODE)|(1<<DLM_CWMODE)|(1<<DLM_PRMODE)|
			 (1<<DLM_PWMODE)|(1<<DLM_EXMODE)
};


static	char	cxaxp_lock_mode_text[] = "NO,NL,CR,CW,PR,PW,EX\0";

void cxaxp_cmpl_rtn( callback_arg_t pdata, dlm_lkid_t *plkid, 
		dlm_status_t dlmstat );
void cxaxp_blk_rtn( callback_arg_t pdata, callback_arg_t hint, 
		dlm_lkid_t *plkid, dlm_lkmode_t dlmmode );

/* end axp_osf section */

#elif defined(LNX)

/*
**	Linux (OpenDLM) declarations
*/

/* Lookup equivalent OpenDLM lock mode, indexed by Ingres lock mode. */
static  i4      cxlnx_lk_to_dlm_mode[] =
{
	/* LK_N            	0	*/	LKM_NLMODE,
	/* LK_IS           	1	*/	LKM_CRMODE,
	/* LK_IX           	2	*/	LKM_CWMODE,
	/* LK_S            	3	*/	LKM_PRMODE,
	/* LK_SIX          	4	*/	LKM_PWMODE,
	/* LK_U            	5	*/	LKM_EXMODE,
	/* LK_X            	6	*/	LKM_EXMODE,
};

/* Lookup equivalent Ingres lock mode, indexed by DLM lock mode. */
static	i4	cxlnx_dlm_to_lk_mode[] =
{
	/* LKM_NOMODE		0	*/	0,	/* unused */
	/* LKM_NLMODE		1	*/	LK_N,
	/* LKM_CRMODE		2	*/	LK_IS,
	/* LKM_CWMODE		3	*/	LK_IX,
	/* LKM_PRMODE		4	*/	LK_S,
	/* LKM_PWMODE		5	*/	LK_SIX,
	/* LKM_EXMODE		6	*/	LK_X,
};

/*
** Lookup table for incompatible OpenDLM lock modes.
*/
static	i4	cxlnx_incompatable_modes[] =
{
	/* LKM_NOMODE */ 0,
	/* LKM_NLMODE */ 0,
	/* LKM_CRMODE */ (1<<LKM_EXMODE),
	/* LKM_CWMODE */ (1<<LKM_PRMODE)|(1<<LKM_PWMODE)|(1<<LKM_EXMODE),
	/* LKM_PRMODE */ (1<<LKM_CWMODE)|(1<<LKM_PWMODE)|(1<<LKM_EXMODE),
	/* LKM_PWMODE */ (1<<LKM_CWMODE)|(1<<LKM_PRMODE)|
			 (1<<LKM_PWMODE)|(1<<LKM_EXMODE),
	/* LKM_EXMODE */ (1<<LKM_CRMODE)|(1<<LKM_CWMODE)|(1<<LKM_PRMODE)|
			 (1<<LKM_PWMODE)|(1<<LKM_EXMODE)
};


static	char	cxlnx_lock_mode_text[] = "NO,NL,CR,CW,PR,PW,EX\0";

void cxlnx_cmpl_rtn( void *pdata );
void cxlnx_blk_rtn( void *pdata,  int mode );

/* End Linux (OpenDLM) section */

#elif defined (VMS) /* _VMS_ */

/*
** If we ever want to or need to use 64 byte LVBs, then remove
** the redefinition of LCK$M_XVALBLK to zero.
**
** We've had bad results with this in a mixed cluster, and do
** not at present use this feature, but have kept the flag in
** the source below to make it easier to turn it back on.
*/
# undef LCK$M_XVALBLK
# define LCK$M_XVALBLK	 0 /* #  define LCK$M_XVALBLK	 0x10000 */ 

/* Lookup equivalent VMS lock mode, indexed by Ingres lock mode. */
static  i4      cxvms_lk_to_dlm_mode[] =
{
	/* LK_N            	0	*/	LCK$K_NLMODE,
	/* LK_IS           	1	*/	LCK$K_CRMODE,
	/* LK_IX           	2	*/	LCK$K_CWMODE,
	/* LK_S            	3	*/	LCK$K_PRMODE,
	/* LK_SIX          	4	*/	LCK$K_PWMODE,
	/* LK_U            	5	*/	LCK$K_EXMODE,
	/* LK_X            	6	*/	LCK$K_EXMODE,
};

/* Lookup equivalent Ingres lock mode, indexed by DLM lock mode. */
static	i4	cxvms_dlm_to_lk_mode[] =
{
	/* DLM_NLMODE		0	*/	LK_N,
	/* DLM_CRMODE		1	*/	LK_IS,
	/* DLM_CWMODE		2	*/	LK_IX,
	/* DLM_PRMODE		3	*/	LK_S,
	/* DLM_PWMODE		4	*/	LK_SIX,
	/* DLM_EXMODE		5	*/	LK_X,
};

/* 
** Lookup table for legal VMS convert wait combinations.
** Index = old mode.  Bit positions = shifted new mode.  
** If bit set LCK$M_QUECVT s/b specified in DLM flags.
*/
static  i4	cxvms_waitcvts[] =
{
	/* LK_N	  0 */	(1<<LK_IS)|(1<<LK_IX)|(1<<LK_S)|
			(1<<LK_SIX)|(1<<LK_U)|(1<<LK_X),
	/* LK_IS  1 */	(1<<LK_IX)|(1<<LK_S)|
			(1<<LK_SIX)|(1<<LK_U)|(1<<LK_X),
	/* LK_IX  2 */	(1<<LK_S)|(1<<LK_SIX)|(1<<LK_X),
	/* LK_S   3 */	(1<<LK_SIX)|(1<<LK_X),
	/* LK_SIX 4 */	(1<<LK_X),
	/* LK_U   5 */	0x0,
	/* LK_X   6 */	0x0,
};

static void cxvms_blk_rtn( void * );
 
static void cxvms_cmpl_rtn( void *p );

static STATUS cx_vms_pass_dlm_req(i4 flags, CX_REQ_CB *preq, i4 opcode);

static void cx_vms_free_glm( CX_GLC_CTX *glc, CX_GLC_MSG **glmp );

#if defined(VERIFY_VMS_LOCKS) && defined(conf_CLUSTER_BUILD)
static STATUS CXdlm_verify_lock(
		CX_REQ_CB *preq,
		STATUS 	cstatus,
		i4	flags,
		char 	*file,
		i4	line);
#endif /* VERIFY_VMS_LOCKS */

/* End _VMS_ section */

#endif /* OS specific declarations */

#endif /* defined(conf_CLUSTER_BUILD) */

static STATUS cx_dlm_sync_lsn_from_dlm( CX_REQ_CB * );


/*{
** Name: CXdlm_request	- Request a new distributed lock.
**
** Description:
**
**	Perform either a synchronous or asynchronous DLM lock request.
**	
**
** Inputs:
**
**	flags 		Bit mask of one or more of the following:
**
**	    - CX_F_PCONTEXT	Set if lock is to be taken in process context.
**				For Tru64, this means lock is not kept in the
**				default GLC, for other implementations this 
**				flag may be irrelevant.
**
**	    - CX_F_NOWAIT	Equal to LK_NOWAIT.   Request fails 
**				if it cannot be granted synchronously.
**
**	    - CX_F_NODEADLOCK	Set if lock should ignored when checking for 
**				deadlock cycles.   This may be the case, if 
**				calling thread is not blocked on lock, or if 
**				lock will be released when a blocking
**				notification routine associated with this 
**				lock is triggered.
**
**	    - CX_F_WAIT		Set if synchronous lock request is desired.   
**				Calling thread will block until lock is 
**				granted, or request deadlocks, or otherwise
**				fails.
**
**	    - CX_F_PRIORITY	Set if we want this lock granted before other
**				grantable locks requested without this flag.
**				Not supported (or needed) on Tru64, but 
**				provided for those platforms which lack 
**				equivalents to persistent resources or 
**				recovery domains, or otherwise need to 
**				assure that DMN locks are granted before 
**				regular lock requests.
**
**	    - CX_F_OWNSET	Set if cx_session & cx_pid already filled
**				out. A minor optimization, but getting SID
**				on an OS threaded system may not be
**				negligibly cheap. 
**
**	    - CX_F_IGNOREVALUE  Set by lock caller, if lock value is
**				not to be read.  This is NEVER used
**				by LK, since IVB detection depends on
**				requesting a value.  Only current use,
**				is when requesting a dead-man notification
**				lock.
**
**	    - CX_F_NOTIFY	Set if instead of a blocking notification,
**				a completion notification is required.
**				Calling thread will NOT be resumed if
**				this is specified.
**
**          - CX_F_NAB          Set if both completion & blocking notification
**                              is required.  Address of blocking routine
**                              to be passed in cx_user_extra.
**
**	    - CX_F_LSN_IN_LVB	Set if CX needs to refresh local internal
**				LSN from the LVB.  Will only do this if
**				LVB valid, and new mode is LK_SIX, or LK_X.
**
**	    - CX_F_NO_DOMAIN	Set if lock is not to be part of any
**				DLM recovery domain CX may have joined 
**				as part of its initialization.
**				
**	    - CX_F_DLM_TRACE	Set if debug tracing required for this
**				call.
**		
**	    - CX_F_PROXY_OK	Sanity check flags set if the lock request
**			        can be processed by another process.  This
**				is only used under VMS where we may have
**				a process acquiring locks that is not in
**			        the same UIC group as the "Ingres" instance
**				owner.  Only set this flag if request block
**				resides in the LGK shared memory segment.
**			
**	    - CX_F_USEPROXY	If set, completion routine will extract
**			        the process/session to resume indirectly from 
**				cx_user_extra.
**
**	    - CX_F_IS_CSP	Lock request should be redirected to the
**				CSP.
**
**	preq		Pointer to CX_REQ_CB.   The memory used for
**	   		this block must stay allocatted so long as
**			there is any chance that a completion routine
**			will be triggered (request is not immediately
**			granted or rejected), or a blocking notification
**			routine was specified.  Field usage as follows.
**		
**	    - cx_lock_id	Will receive a unique identifier for this lock.
**
**	    - cx_old_mode	Ignored.
**
**	    - cx_new_mode	Must be set to requested Ingres lock mode.
**
**	    - cx_status		Will be set to zero until request completes.
**
**	    - cx_owner		Will have ownership filled in, if asynch.
**				completion possible and CX_F_OWNSET not set.
**
**	    - cx_key		Fill it with resource key.
Ingres lock
**				key structure.
**
**	    - cx_user_func 	If non-zero, must be the address of a callback
**				routine to be triggered if after lock is 
**				granted one or more incompatable requests
**				are made for the locked resource.  However if
**				CX_F_NOTIFY set, then instead of a blocking
**				notification routine, this holds the address
**				of a completion routine.  Callback routines
**				must only be called from the process which
**				sets this address.
**
**	    - cx_user_extra	Pointer to optional user defined data
**				to be used by blocking routines, or if
**                              CX_F_NAB set blocking notification function
**                              address.  If request block is in shared
**				memory, and may be addressed by multiple
**				processes, it is suggested but not enforced
**				that the user data be stored in the same
**			        shared memory segment as the request, and this
**				field filled with the offset of the data
**				relative to 'preq'.
**
**	    - cx_value		Buffer to hold lock value when request
**				is granted.
**    	
**	ptranid		Pointer to a unique identifier derived from the
**			Ingres lock list ID.  This must be unique cluster
**			wide, as it may be used by the DLM for deadlock 
**			detection on those platforms that use OS
**			deadlock detection.
**
** Outputs:
**	preq		Following fields updated or conditionally updated
**			as described above.  cx_lock_id, cx_status, 
**			cx_owner, cx_value.
**
**	Returns:
**		
**		OK			- Normal successful completion.
**		  If request was queued, this is only an intermediate
**		  return code, and CXdlm_wait must be called, or cx_status
**		  in request block polled, to determine final status.
**		E_CL2C01_CX_I_OKSYNC	- Distinct success code returned
**		  if request synchronously granted, and CX_F_STATUS set.
**
**		E_CL2C09_CX_W_SYNC_IVB  - Request synchronously granted,
**					  but value block was invalid.
**
**		E_CL2C10_CX_E_NOSUPPORT - CX DLM not supported this OS.
**		E_CL2C11_CX_E_BADPARAM	- Invalid argument passed.
**		E_CL2C1B_CX_E_BADCONTEXT - CX state not JOINED,
**		  or CX_F_PCONTEXT not set & CX state not INIT or JOINED.
**		E_CL2C1C_CX_E_LSN_LVB_FAILURE - Lock Granted, but
**		  there was a failure in LSN propagation.  Caller
**		  MUST abort his transaction.
**		E_CL2C21_CX_E_DLM_DEADLOCK - synchronous lock request
**		  chosen as deadlock victim.
**		E_CL2C22_CX_E_DLM_CANCEL - synchronous lock request
**                canceled.
**		E_CL2C27_CX_E_DLM_NOGRANT - CX_F_NOWAIT was set, and
**		  lock could not be synchronously granted.
**		E_CL2C3x_CX_E_OS_xxxxxx	- Misc implementation dependent
**		  failures at the OS DLM level.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	- Lock request may be left pending if for asynchronous completion
**	  if neither CX_F_WAIT or CX_F_NOWAIT was specified.
**	- Thread may block (wait for synchronous completion of lock
**	  request.) if CX_F_WAIT was set. 
**	- Local CX LSN may be synchronized with global CX LSN if
**	  CX_F_LSN_IN_LVB and conditions met.
**
** History:
**	15-feb-2001 (devjo01)
**	    Created.
**	29-nov-2005 (devjo01)
**	    Overload CX_F_IS_CSP as a valid flag to this routine, which
**	    for VMS only will redirect request to CSP.  This is one of
**	    the ways we're compensating for the lack of a group lock
**	    container on this platform.
**	18-May-2007 (jonj)
**	    Clear cx_dlm_posted before doing anything else.
**	14-Dec-2007 (jonj)
**	    Add LK_CKP_CLUSTER to list of those locks to be
**	    taken in LCK$M_SYSTEM context. 
**	04-Apr-2008 (jonj)
**	    Get event flag using lib$get_ef() rather than defaulting
**	    to zero.
**	09-May-2008 (jonj)
**	    If from GLC and request has already been posted
**	    (like by deadlock), just return OK.
**	11-Jul-2008 (jonj)
**	    Don't extract cx_dlm_status prematurely when doing
**	    async enq's, that's the AST's task.
**	23-July-2008 (jonj)
**	    Relocate cx_lki_flags CX_LKI_F_USEPROXY to inter-process-safe
**	    cx_inproxy variable.
**	    Stow resultant DLM flags in cx_dlmflags as debugging aid.
*/
STATUS
CXdlm_request( u_i4 flags, CX_REQ_CB *preq, LK_UNIQUE *ptranid )
{
    LK_LOCK_KEY	*plockkey;
    STATUS	 status;

    CX_Stats.cx_dlm_requests++;

#ifdef	CX_PARANOIA
    if ( (NULL == preq) || CX_UNALIGNED_PTR(preq) )
	return E_CL2C11_CX_E_BADPARAM;

    for ( ; ; )
    {
	if ( (flags & ~(CX_F_NOWAIT|CX_F_WAIT|CX_F_STATUS|CX_F_OWNSET|
			CX_F_PCONTEXT|CX_F_NODEADLOCK|CX_F_PRIORITY|
			CX_F_IGNOREVALUE|CX_F_NOTIFY|CX_F_DLM_TRACE|
			CX_F_LOCALLOCK|CX_F_NAB|CX_F_LSN_IN_LVB|
			CX_F_NO_DOMAIN|CX_F_USEPROXY|CX_F_PROXY_OK
			CX_F_IS_CSP)) ||
	     ((CX_F_IGNOREVALUE|CX_F_LSN_IN_LVB) ==
	      ((CX_F_IGNOREVALUE|CX_F_LSN_IN_LVB) & flags)) ||
	     (preq->cx_new_mode > LK_X) || 
	     (preq->cx_key.lk_type <= 0) ||
	     (preq->cx_key.lk_type >
	       sizeof(cx_reslen_tab)/sizeof(cx_reslen_tab[0])) ||
	     CX_UNALIGNED_PTR(ptranid) ||
	     ( (CX_F_NOTIFY & flags) && !preq->cx_user_func )
	   )
	    status = E_CL2C11_CX_E_BADPARAM;
	else if ( CX_Proc_CB.cx_state != CX_ST_JOINED &&
		  !( (CX_Proc_CB.cx_state == CX_ST_INIT) &&
		     ((flags & CX_F_PCONTEXT) ||
		      (CX_Proc_CB.cx_flags & CX_PF_FIRSTCSP))))
	    /*
	    ** May only ask for locks if joined, OR if the lock has
	    ** process context or caller is the first CSP, and call
	    ** is made during the period between CXinitialize &&
	    ** CXjoin.
	    */
	    status = E_CL2C1B_CX_E_BADCONTEXT;
	else
	    break;
	preq->cx_status = status;
	return status;
    }
#endif /*CX_PARANOIA*/

    plockkey = &preq->cx_key;
    preq->cx_lki_flags = flags &
	(CX_LKI_F_PCONTEXT|CX_LKI_F_NOTIFY|CX_LKI_F_NAB|
	 CX_LKI_F_NO_DOMAIN|CX_LKI_F_LSN_IN_LVB);

    /* Don't clear status again if proxy call from GLC */
    if ( !preq->cx_inproxy )
	preq->cx_status = 0;

    /*
    **	Start OS dependent declarations.
    */
#if !defined(conf_CLUSTER_BUILD)
    status = E_CL2C10_CX_E_NOSUPPORT;
#elif defined(axp_osf)
    /*
    **	Tru64 implementation
    **
    **  FIXME: NAB & LSN_IN_LVB support not coded yet for this platform.
    */
    {
    dlm_status_t dlmstat;	/* Tru64 dlm status code */
    dlm_flags_t dlmflags;	/* Tru64 dlm flag bits */
    dlm_lkmode_t dlmmode;	/* Tru64 lock modes */
    dlm_rd_id_t	resdom;		/* Tru64 recovery domain */
    dlm_trans_id_t tranid;	/* Tru64 transcation ID */
    blk_callback_t blkrtn;	/* Tru64 blocking callback */
    i4		reslen;		/* Length of internal lock key */
    i4		resname[DLM_RESNAMELEN/sizeof(i4)]; /* Massaged key. */

    /* Generate internal lock key */
    reslen = cx_reslen_tab[plockkey->lk_type - 1];
    (void)MEcopy( plockkey, reslen, resname );
    resname[0] |= CX_Proc_CB.cxaxp_lk_instkey;
    if ( CX_F_LOCALLOCK & flags )
	resname[0] |= CX_Proc_CB.cx_lk_nodekey;

    dlmflags = DLM_SYNCSTS | DLM_PERSIST;

    if ( CX_F_NOWAIT & flags )
	dlmflags |= DLM_NOQUEUE;

    if ( CX_F_PCONTEXT & flags )
    {
	resdom = CX_Proc_CB.cxaxp_prc_rd;
    }
    else
    {
	resdom = CX_Proc_CB.cxaxp_grp_rd;
	dlmflags |= DLM_GROUP_LOCK;
    }

    if ( CX_F_NO_DOMAIN & flags )
    {
	resdom = (dlm_rd_id_t)0;
	dlmflags &= ~DLM_PERSIST;
    }

    if ( CX_F_NODEADLOCK & flags )
	dlmflags |= DLM_NODLCKBLK | DLM_NODLCKWT;

    if ( ptranid )
    {
	dlmflags |= DLM_TXID_LOCAL;
	tranid = *(dlm_trans_id_t *)ptranid;
    }
    else
    {
	tranid = (dlm_trans_id_t)0;
    }

    if ( !(CX_F_IGNOREVALUE & flags ) )
	dlmflags |= DLM_VALB;

    dlmmode = cxaxp_lk_to_dlm_mode[preq->cx_new_mode];

    blkrtn = NULL;
    if ( preq->cx_user_func && !( CX_F_NOTIFY & flags ) )
	blkrtn = cxaxp_blk_rtn;

    if ( !( (CX_F_WAIT|CX_F_NOWAIT|CX_F_OWNSET) & flags ) )
    {
	/* Fill in ownership info */
	CSget_cpid( &preq->cx_owner );
    }

    /* Stow dlmflags in preq for debugging */
    preq->cx_dlmflags = (u_i4)dlmflags;

    if ( (CX_F_WAIT|CX_F_NOWAIT) & flags )
	dlmstat = DLM_LOCKTP( CX_Proc_CB.cxaxp_nsp, (uchar_t *)resname,
	    (ushort_t)reslen, (dlm_lkid_t *)NULL, 
	    (dlm_lkid_t *)&preq->cx_lock_id,
	    dlmmode, (dlm_valb_t *)preq->cx_value, dlmflags, 
	    (callback_arg_t)preq, (callback_arg_t)NULL,
	    blkrtn, (uint_t)0, tranid, resdom ); 
    else
	dlmstat = DLM_QUELOCKTP( CX_Proc_CB.cxaxp_nsp, (uchar_t *)resname,
	    (ushort_t)reslen, (dlm_lkid_t *)NULL,
	    (dlm_lkid_t *)&preq->cx_lock_id,
	    dlmmode, (dlm_valb_t *)preq->cx_value, dlmflags, 
	    (callback_arg_t)preq, (callback_arg_t)NULL,
	    blkrtn, (uint_t)0, cxaxp_cmpl_rtn,
	    tranid, resdom ); 

    status = cx_translate_status( (i4)dlmstat );
    }
#elif defined(LNX) 
    /*
    **	Linux implementation
    */
    {
    dlm_stats_t	 dlmstat;	/* OpenDLM status code */
    int		 dlmflags;	/* OpenDLM flag bits */
    int		 dlmmode;	/* OpenDLM lock modes */
    dlm_bastlockfunc_t *blkrtn;	/* OpenDLM blocking callback */
    i4		reslen;		/* Length of internal lock key */
    i4		resname[7]; 	/* Massaged key. */

    /* Generate internal lock key */
    reslen = cx_reslen_tab[plockkey->lk_type - 1];
    (void)MEcopy( plockkey, reslen, resname );
    resname[0] |= CX_Proc_CB.cxlnx_lk_instkey;
    if ( CX_F_LOCALLOCK & flags )
	resname[0] |= CX_Proc_CB.cx_lk_nodekey;

    dlmflags = LKM_SYNCSTS;		/* Always return sync status */

    if ( CX_F_NOWAIT & flags )
	dlmflags |= LKM_NOQUEUE;

    if ( CX_F_PCONTEXT & flags )
    {
	dlmflags |= LKM_PROC_OWNED;
    }

    if ( CX_F_NO_DOMAIN & flags )
    {
	dlmflags |= LKM_NO_DOMAIN;
    }

    if ( CX_F_NODEADLOCK & flags )
	dlmflags |= LKM_NODLCKWT;

    if ( ptranid )
    {
	dlmflags |= LKM_XID_CONFLICT;
    }

    if ( !(CX_F_IGNOREVALUE & flags ) )
    {
	dlmflags |= LKM_VALBLK;
    }

    dlmmode = cxlnx_lk_to_dlm_mode[preq->cx_new_mode];

    blkrtn = NULL;
    if ( ( preq->cx_user_func && !( CX_F_NOTIFY & flags ) ) ||
         ( preq->cx_user_extra && ( CX_F_NAB & flags ) ) )
	blkrtn = cxlnx_blk_rtn;

    if ( !( (CX_F_WAIT|CX_F_NOWAIT|CX_F_OWNSET) & flags ) )
    {
	/* Fill in ownership info */
	CSget_cpid( &preq->cx_owner );
    }

    /* Stow dlmflags in preq for debugging */
    preq->cx_dlmflags = (u_i4)dlmflags;

    if ( (CX_F_WAIT|CX_F_NOWAIT) & flags )
    {
	dlmstat = DLMLOCKX_SYNC( dlmmode, (struct lockstatus *)&preq->cx_dlm_ws,
	    dlmflags, resname, reslen, preq, blkrtn, (dlm_xid_t *)ptranid );
	if ( DLM_NORMAL == dlmstat ) dlmstat = preq->cx_dlm_ws.cx_dlm_status;
    }
    else
    {
	dlmstat = DLMLOCKX( dlmmode, (struct lockstatus *)&preq->cx_dlm_ws,
	    dlmflags, resname, reslen, cxlnx_cmpl_rtn, preq, blkrtn,
	    (dlm_xid_t *)ptranid );
	if ( DLM_SYNC == dlmstat &&
	     DLM_NORMAL != preq->cx_dlm_ws.cx_dlm_status )
	     dlmstat = preq->cx_dlm_ws.cx_dlm_status;
    }

    status = cx_translate_status( (i4)dlmstat );
    }
#elif defined(VMS) /* _VMS_ */

    /*
    **	VMS implementation
    */
    {
    unsigned int dlmstat;	/* OS status code */
    unsigned int dlmflags;	/* VMS DLM flag bits */
    unsigned int dlmmode;	/* VMS DLM lock modes */
    struct dsc$descriptor_s resname; /* Resource descriptor */
    void	 (*blkrtn)(void *);/* VMS DLM blocking callback */
    i4		reslen;		/* Length of internal lock key */
    i4		resbuffer[7]; 	/* Massaged key. */
    II_VMS_EF_NUMBER	efn;

#ifdef VERIFY_VMS_LOCKS
    if ( preq->cx_lki_flags & CX_LKI_F_QUEUED )
    {
	char	keybuffer[256];
	CS_CPID *rqst = (CS_CPID*)((PTR)(CX_RELOFF2PTR(preq, preq->cx_user_extra))
				+ CX_Proc_CB.cx_proxy_offset);
	TRdisplay("%@ CXdlm_request %d QUEUED flags %x lki_flags %x mode %d,%d\n"
		"\towner %x.%x rqst %x.%x instance %d\n"
		"\tkey %s\n",
		__LINE__,
		flags, preq->cx_lki_flags,
		preq->cx_old_mode, preq->cx_new_mode,
		preq->cx_owner.pid, preq->cx_owner.sid,
		rqst->pid, rqst->sid,
		preq->cx_instance,
		(*CX_Proc_CB.cx_lkkey_fmt_func)(&preq->cx_key, keybuffer));
    }
#endif /* VERIFY_VMS_LOCKS */


    /* Don't clear posted flag if proxy call from GLC */
    if ( preq->cx_inproxy )
    {
	/* If lagging GLC, just return */
	if ( CXDLM_IS_POSTED(preq) )
	    return(OK);
    }
    else
    {
	/* Clear posted flag */
	CS_ACLR(&preq->cx_dlm_ws.cx_dlm_posted);
	preq->cx_inproxy = FALSE;
    }

    /*
    ** Set flags to always return sync status, and to suppress deadlock
    ** processing by OS.  sys$enq does not support a transaction, so
    ** we would get false deadlock indications otherwise.
    */
    dlmflags = LCK$M_SYNCSTS | LCK$M_NODLCKWT |
               LCK$M_NODLCKBLK | LCK$M_PROTECT;

    if ( LK_CSP == plockkey->lk_type ||
         LK_CMO == plockkey->lk_type ||
	 LK_MSG == plockkey->lk_type ||
         LK_CKP_CLUSTER == plockkey->lk_type )
    {
	/* These locks are taken in system context */
	dlmflags |= LCK$M_SYSTEM;
    }
    else if ( ( CX_Proc_CB.cxvms_uic_group !=
                CX_Proc_CB.cx_ncb->cxvms_csp_uic_group ) ||
	      ( (flags & CX_F_IS_CSP) &&
		(CX_Proc_CB.cx_pid != CX_Proc_CB.cx_ncb->cx_csp_pid) ) )
    {
	/*
	** We need to pass off operation on this lock to the CSP,
	** since the requestor is in a separate UIC group than the
	** installation, which would implicitly put any locks
	** directly requested by this process in a separate namespace.
	*/
	status = cx_vms_pass_dlm_req( flags, preq, CX_DLM_OP_RQST );
	return status;
    }

    /* Generate internal lock key */
    reslen = cx_reslen_tab[plockkey->lk_type - 1];
    resname.dsc$w_length = (unsigned short)reslen;
    resname.dsc$b_dtype = DSC$K_DTYPE_V;
    resname.dsc$b_class = DSC$K_CLASS_S;
    resname.dsc$a_pointer = (void*)resbuffer;

    (void)MEcopy( plockkey, reslen, resbuffer );
    resbuffer[0] |= CX_Proc_CB.cxvms_lk_instkey;
    if ( CX_F_LOCALLOCK & flags )
	resbuffer[0] |= CX_Proc_CB.cx_lk_nodekey;

    if ( CX_F_NOWAIT & flags )
	dlmflags |= LCK$M_NOQUEUE;

    if ( !(CX_F_IGNOREVALUE & flags ) )
    {
	dlmflags |= LCK$M_VALBLK | LCK$M_XVALBLK;
    }

    dlmmode = cxvms_lk_to_dlm_mode[preq->cx_new_mode];

    if ( LCK$K_NLMODE == dlmmode )
    {
	dlmflags |= LCK$M_EXPEDITE;
    }

    blkrtn = NULL;
    if ( ( preq->cx_user_func && !( CX_F_NOTIFY & flags ) ) ||
         ( preq->cx_user_extra && ( CX_F_NAB & flags ) ) )
	blkrtn = cxvms_blk_rtn;

    if ( !( CX_F_OWNSET & flags ) )
    {
	/* Fill in ownership info. Do this even if WAIT/NOWAIT set. */
	CSget_cpid( &preq->cx_owner );
    }

    /* Stow dlmflags in preq for debugging */
    preq->cx_dlmflags = (u_i4)dlmflags;

    if ( CX_F_WAIT & flags )
    {
	dlmstat = sys$enqw( EFN$C_ENF, dlmmode,
			    (struct _lksb *)&preq->cx_dlm_ws, dlmflags,
			    &resname, 0, NULL, preq, blkrtn,
			    PSL$C_USER, CX_Proc_CB.cxvms_dlm_rsdm_id, NULL );
	if ( SS$_NORMAL == dlmstat ) 
	    dlmstat = preq->cx_dlm_ws.cx_dlm_status;
    }
    else
    {
	dlmstat = sys$enq( EFN$C_ENF, dlmmode,
			   (struct _lksb *)&preq->cx_dlm_ws, dlmflags,
			   &resname, 0, cxvms_cmpl_rtn, preq, blkrtn,
			   PSL$C_USER, CX_Proc_CB.cxvms_dlm_rsdm_id, NULL );
    }

#ifdef VERIFY_VMS_LOCKS
    if ( dlmstat == SS$_BADPARAM )
    {
	char	keybuffer[256];
	CS_CPID *rqst = (CS_CPID*)((PTR)(CX_RELOFF2PTR(preq, preq->cx_user_extra))
				+ CX_Proc_CB.cx_proxy_offset);
	TRdisplay("%@ CXdlm_request %d BADPARAM flags %x dlmflags %x lki_flags %x mode %d,%d\n"
		"\towner %x.%x rqst %x.%x instance %d\n"
		"\tkey %s\n",
		__LINE__,
		flags, dlmflags, preq->cx_lki_flags,
		preq->cx_old_mode, preq->cx_new_mode,
		preq->cx_owner.pid, preq->cx_owner.sid,
		rqst->pid, rqst->sid,
		preq->cx_instance,
		(*CX_Proc_CB.cx_lkkey_fmt_func)(&preq->cx_key, keybuffer));

	TRdisplay("%@ SLEEPING - take a lockstat\n");
	CSsuspend(CS_INTERRUPT_MASK, 0, 0);
    }
#endif /* VERIFY_VMS_LOCKS */

    status = cx_translate_status( (i4)dlmstat );

    }
    /* end _VMS_ */
#else /* others */
    /*
    **	All other platforms
    */
    status = E_CL2C10_CX_E_NOSUPPORT;
#endif /* OS specific bits */
    /*
    **	End OS dependent declarations.
    */

    /* 
    ** Save intermediate status into request block.  OK here indicates
    ** that request was enqueued, unless CX_F_WAIT was set.
    */

    /* Was request queued? */
    if ( OK == status && !(flags & (CX_F_WAIT|CX_F_NOWAIT)) )
    {
	CX_Stats.cx_dlm_quedreqs++;
	preq->cx_lki_flags |= CX_LKI_F_QUEUED;
    }
    else
    {
	/* Request not queued, set final code. */
	preq->cx_status = status;

	/* Post request as complete for LK */
	CXdlm_mark_as_posted(preq);

	/* If called from LK, verify state of VMS lock */
	if ( flags & CX_F_PROXY_OK )
	    VERIFY_LOCK(preq, status, flags);

	/* Check for synchronous grant. */
	if ( E_CL2C01_CX_I_OKSYNC == status ||
	     E_CL2C08_CX_W_SUCC_IVB == status ||
	     OK == status )
	{
	    CX_Stats.cx_dlm_sync_grants++;
	    if ( ( CX_F_LSN_IN_LVB & flags ) && 
	         ( preq->cx_new_mode >= LK_SIX ) )
	    {
		if ( OK != cx_dlm_sync_lsn_from_dlm( preq ) )
		    status = E_CL2C1C_CX_E_LSN_LVB_FAILURE;
	    }
	    preq->cx_old_mode = preq->cx_new_mode;
	}
    }

    /* Convert informational statuses to OK if CX_F_STATUS not set. */
    if ( !( CX_F_STATUS & flags ) && CX_OK_STATUS(status) )
	status = OK;

    CX_DLM_TRACE("REQ",preq);

    /* Return ( possibly intermediate ) status code. */
    return status;
} /* CXdlm_request */



#if defined(conf_CLUSTER_BUILD)
/*
**	Completion, and blocking routine functions.
**
**	Completion routine always does some simple
**	sanity checks, then fills in cx_status,
**	and resumes process.
**
**	Blocking routine is simply a wrapper for
**	user specified function in cx_user_func.
**
**	Routines must conform with the vendor DLM's parameter 
**	expectations for completion & blocking routines.   
*/

#ifdef	axp_osf
/*
**	Tru64 implementation
*/
void
cxaxp_cmpl_rtn( callback_arg_t pdata, dlm_lkid_t *plkid, 
		dlm_status_t dlmstat )
{
    STATUS	 status;
    CX_REQ_CB	*preq = (CX_REQ_CB *)pdata;

    CX_Stats.cx_dlm_completions++;

#ifdef	CX_PARANOIA
    if ( plkid->seqn != preq->cx_lock_id.lk_uhigh ||
	 plkid->hndl != preq->cx_lock_id.lk_ulow )
	status = E_CL2C28_CX_E_DLM_CMPLERR;
    else
#endif /*CX_PARANOIA*/
    status = cx_translate_status( (i4)dlmstat );
    
    if ( OK == ( preq->cx_status = status ) )
	preq->cx_status = E_CL2C02_CX_I_OKASYNC;

    if ( ( OK == status ) ||
	 ( E_CL2C08_CX_W_SUCC_IVB == status ) )
    {
	CX_Stats.cx_dlm_async_grants++;
	preq->cx_old_mode = preq->cx_new_mode;
    }

    CX_DLM_TRACE_TERSE("Cmpl",preq);

    if ( preq->cx_lki_flags & CX_LKI_F_NOTIFY )
	(*preq->cx_user_func)( preq, status );
    else
	CSresume( preq->cx_owner.sid );
    return;
} /* cxaxp_cmpl_rtn */


void
cxaxp_blk_rtn( callback_arg_t pdata, callback_arg_t hint, dlm_lkid_t *plkid, 
		dlm_lkmode_t dlmmode )
{
    CX_REQ_CB	*preq = (CX_REQ_CB *)pdata;

    CX_Stats.cx_dlm_blk_notifies++;

#ifdef	CX_PARANOIA
    if ( plkid->seqn != preq->cx_lock_id.lk_uhigh ||
	 plkid->hndl != preq->cx_lock_id.lk_ulow || 
	 NULL == preq->cx_user_func )
	preq->cx_status = E_CL2C28_CX_E_DLM_CMPLERR;
#endif /*CX_PARANOIA*/
    
    CX_DLM_TRACE_TERSE("Blk",preq);

    (*preq->cx_user_func)( preq, cxaxp_dlm_to_lk_mode[dlmmode] );
    return;
} /* cxaxp_blk_rtn */

#endif /*axp_osf (Tru64)*/


#if defined(LNX)
/*
** Linux OpenDLM implementation.
*/

void
cxlnx_cmpl_rtn( void *pdata )
{
    STATUS	 status;
    CX_REQ_CB	*preq = (CX_REQ_CB *)pdata;

    CX_Stats.cx_dlm_completions++;

    status = cx_translate_status( (i4)preq->cx_dlm_ws.cx_dlm_status );
    
    if ( OK == ( preq->cx_status = status ) )
	preq->cx_status = E_CL2C02_CX_I_OKASYNC;

    if ( ( OK == status ) ||
	 ( E_CL2C08_CX_W_SUCC_IVB == status ) )
    {
	CX_Stats.cx_dlm_async_grants++;
	preq->cx_old_mode = preq->cx_new_mode;
    }

    CX_DLM_TRACE_TERSE("Cmpl",preq);

    if ( preq->cx_lki_flags & (CX_LKI_F_NOTIFY|CX_LKI_F_NAB) )
	(*preq->cx_user_func)( preq, status );
    else
	CSresume( preq->cx_owner.sid );
    return;
}  /* cxlnx_cmpl_rtn */

void
cxlnx_blk_rtn( void *pdata, int dlmmode )
{
    CX_REQ_CB	*preq = (CX_REQ_CB *)pdata;

    CX_Stats.cx_dlm_blk_notifies++;

    CX_DLM_TRACE_TERSE("Blk",preq);

    if ( dlmmode < 0 || dlmmode > LKM_EXMODE )
    {
	CS_breakpoint();
	dlmmode = 0;
    }

    if ( preq->cx_lki_flags & CX_LKI_F_NAB )
    {
	if ( CX_VALID_FUNC_PTR(preq->cx_user_extra) )
	    (*(CX_USER_FUNC *)preq->cx_user_extra)
	     ( preq, cxlnx_dlm_to_lk_mode[dlmmode] );
	else
	    CS_breakpoint();
    }
    else
    {
	if ( CX_VALID_FUNC_PTR(preq->cx_user_func) )
	    (*preq->cx_user_func)( preq, cxlnx_dlm_to_lk_mode[dlmmode] );
	else
	    CS_breakpoint();
    }
    return;
} /* cxlnx_blk_rtn */

#endif /* Linux */

#if defined(VMS)  /* start _VMS_ */

/*
** VMS DLM implementation.
*/

/*
** Note: VMS implementation relies on Ingres to perform the
** distributed deadlock checking.  Routine CXdlm_mark_as_posted
** will be called from the deadlock detection threads, and will
** attempt to atomically set cx_dlm_posted.  If it succeeds, it
** will post a resume, otherwise it will not.  If the AST is
** kicked off before CXdlm_mark_as_posted is called, it
** sets cx_dlm_posted itself, since the deadlock threads are
** running in another process, and could otherwise possibly
** post a resume after this lock request had exited CXdlm_wait.
**
** History:
**	20-nov-2006 (jenjo02/abbjo03)
**	    Call CXdlm_mark_as_posted instead of setting cx_dlm_posted
**	    inline.
**	04-Apr-2008 (jonj)
**	    Avoid double resumption of thread. The lock request may
**	    have been picked for deadlock and CSresume-d prior to 
**	    this completion AST. If the request has already been
**	    posted, don't resume, and avoid spurious CSnoresnow()
**	23-July-2008 (jonj)
**	    Relocate cx_lki_flags CX_LKI_F_USEPROXY to inter-process-safe
**	    cx_inproxy variable.
**	    notifications.
*/
static void
cxvms_cmpl_rtn( void *pdata )
{
    STATUS	 status;
    CX_REQ_CB	*preq = (CX_REQ_CB *)pdata;
    bool	WePosted;

    CX_Stats.cx_dlm_completions++;

    /* Note if we posted, or already posted */
    if ( WePosted = CXdlm_mark_as_posted(preq) )
	status = cx_translate_status( (i4)preq->cx_dlm_ws.cx_dlm_status );
    else
	status = preq->cx_status;

    if ( OK == ( preq->cx_status = status ) )
	preq->cx_status = E_CL2C02_CX_I_OKASYNC;

    if ( ( OK == status ) ||
	 ( E_CL2C08_CX_W_SUCC_IVB == status ) )
    {
	CX_Stats.cx_dlm_async_grants++;
	preq->cx_old_mode = preq->cx_new_mode;
    }

    CX_DLM_TRACE_TERSE("Cmpl",preq);

    if ( preq->cx_inproxy )
    {
	/*
	** Resume session in this or other process, 
	** it will call user funcs if needed.
	**
	** NB: for the timid, this resolves to the LKB's lkb_lkreq.cpid,
	**     the CPID of the lock requestor.
	*/
	if ( WePosted )
	{
	    CScp_resume(
	     (CS_CPID *)((PTR)(CX_RELOFF2PTR(preq, preq->cx_user_extra)) + 
	     CX_Proc_CB.cx_proxy_offset) );
	}
    }
    else if ( preq->cx_lki_flags & (CX_LKI_F_NOTIFY|CX_LKI_F_NAB) )
    {
	(*preq->cx_user_func)( preq, status );
    }
    else 
    {
	if ( WePosted )
	    CSresume( preq->cx_owner.sid );
    }
    return;
} /* cxvms_cmpl_rtn */

static void
cxvms_blk_rtn( void *pdata )
{
    /*
    ** VMS DLM does not pass mode of blocked request.  While we could
    ** query the OS to see whose blocked, it was decided to simply
    ** pass the most restrictive lock mode.  It is believed that in
    ** actual Ingres usage this will effectively give the same behavior
    ** anyway.
    */
    int		dlmmode = LCK$K_EXMODE;

    CX_REQ_CB	*preq = (CX_REQ_CB *)pdata;

    CX_Stats.cx_dlm_blk_notifies++;

    CX_DLM_TRACE_TERSE("Blk",preq);

    if ( preq->cx_lki_flags & CX_LKI_F_NAB )
    {
	if ( CX_VALID_FUNC_PTR(preq->cx_user_extra) )
	    (*(CX_USER_FUNC *)preq->cx_user_extra)
	     ( preq, cxvms_dlm_to_lk_mode[dlmmode] );
	else
	    CS_breakpoint();
    }
    else
    {
	if ( CX_VALID_FUNC_PTR(preq->cx_user_func) )
	    (*preq->cx_user_func)( preq, cxvms_dlm_to_lk_mode[dlmmode] );
	else
	    CS_breakpoint();
    }
    return;
} /* cxvms_blk_rtn */

/*
** Utility routine to encapsulate freeing a GLM.
*/
static void
cx_vms_free_glm( CX_GLC_CTX *glc, CX_GLC_MSG **glmp )
{
    CX_GLC_MSG	*glm;
    volatile i4	       *cachehead;
    i4		oldhead, newhead;

    glm = *glmp;
    *glmp = (CX_GLC_MSG*)NULL;

    newhead = CX_PTR_TO_OFFSET(glm);
    do
    {
	/* Add message back to a free list */
	if ( glc->cx_glc_fcnt >= CX_MAX_GLC_LOCAL_CACHE )
	{
	    /* Add to shared free cache */
	    cachehead = &CX_Proc_CB.cx_ncb->cxvms_glm_free;
	}
	else
	{
	    /* Add to local free cache */
	    cachehead = &glc->cx_glc_free;
	}

	oldhead = *cachehead;
	glm->cx_glm_next = oldhead;
    } while ( CScas4( cachehead, oldhead, newhead ) );

    if ( cachehead == &glc->cx_glc_free )
    {
	glc->cx_glc_fcnt++; /* unprotected update */
    }
} /* cx_vms_free_glm */

/*
** Lock was allocated either as part of a GLC, or from a process in a
** different UIC group from the Ingres instance owner.  Because VMS at
** present does not support group lock containers, and the namespace
** for the lock resources is implicitly determined by the group the process
** is in, there are cases where we need to pass off the lock operation.
**
**	04-Apr-2008 (jonj)
**	    Changed wakeup/sleep mechanism to use
**	    CSadjust_counter(cx_glc_triggered) instead
**	    of buggy CS_TAS(cx_glc_ready) which sometimes
**	    resulted in eternal sleep and/or double
**	    resumes of GLC thread.
**	10-Jun-2008 (jonj)
**	    Repair misplaced "*" in cx_lkkey_fmt_func callback.
**	23-July-2008 (jonj)
**	    Relocate cx_lki_flags CX_LKI_F_USEPROXY to inter-process-safe
**	    cx_inproxy variable.
*/
static STATUS
cx_vms_pass_dlm_req( i4 flags, CX_REQ_CB *preq, i4 opcode )
{
    STATUS	status = OK;
    CX_GLC_CTX *glc;
    CX_GLC_MSG *glcmsg;
    CX_OFFSET	glmoff, oldval;
    char	keybuffer[132];

    do
    {
	/*
	** Sanity check.  If caller did not set this flag, then this
	** request should not be redirected.  This would be the case
	** where the CX_REQ_CB did not reside in shared memory.
	** Rather than potentially SEGV later, we report this here
	** so we do less damage and we've a better guarantee of identifying
	** the offending call.
	*/
	if ( !(flags & CX_F_PROXY_OK) )
	{
	    TRdisplay( "%@ %s:%d Attempt to pass a DLM request without "
	               "CX_F_PROXY_OK set.\nOp=%d, key=%s\n",
		       __FILE__,__LINE__, opcode, 
		       (*CX_Proc_CB.cx_lkkey_fmt_func)( &preq->cx_key,
		         keybuffer ) );
	    preq->cx_status = status = E_CL2C2B_CX_E_DLM_GLC_FAIL;
	    break;
	}

	switch ( opcode )
	{
	case CX_DLM_OP_RQST:
	    preq->cx_owner = CX_Proc_CB.cx_ncb->cxvms_csp_glc.cx_glc_threadid;
	    flags |= CX_F_OWNSET;

	    /* drop into next cases. */

	case CX_DLM_OP_CONV:
	    preq->cx_inproxy = TRUE;
	    flags |= CX_F_STATUS;

	    /* drop into next cases. */

	case CX_DLM_OP_RLSE:
	case CX_DLM_OP_CANC:
	    flags |= CX_F_USEPROXY;
	    if ( preq->cx_owner.pid == CX_Proc_CB.cx_ncb->cx_csp_pid )
	    {
		/* CSP will perform lock operation */
		glc = &CX_Proc_CB.cx_ncb->cxvms_csp_glc;
	    }
	    else if ( NULL == ( glc = cx_find_glc( preq->cx_owner.pid, 0 ) ) )
	    {
		/*
		** If GLC does not exist owning process is gone &
		** lock invalid as it has been implicitly freed.
		*/
		preq->cx_status = status = E_CL2C25_CX_E_DLM_NOTHELD;

		TRdisplay( "%@ %s:%d NOTHELD - GLC not found for op %w\n"
	               "\tcx_owner: %x,%x key: %s\n",
		       __FILE__,__LINE__, 
		       "RQST,CONV,RLSE,CANC", opcode, 
		       preq->cx_owner.pid,
		       preq->cx_owner.sid,
		       (*CX_Proc_CB.cx_lkkey_fmt_func)( &preq->cx_key,
		         keybuffer ) );
	    }
	    break; 

	default: /* Bad argument */
	    status = E_CL2C11_CX_E_BADPARAM;
	    break;
	}

	if ( OK != status )
	{
	    break;
	}

	do
	{
	    /* Grab a glcmsg off of the private queue. */
	    do
	    {
		if ( 0 != (oldval = glc->cx_glc_free ) )
		{
		    glcmsg = (CX_GLC_MSG *)CX_OFFSET_TO_PTR(oldval);
		}
		else
		{
		    glcmsg = NULL;
		    break;
		}
	    } while ( CScas4( &glc->cx_glc_free, oldval, 
	              glcmsg->cx_glm_next ) );

	    if ( NULL == glcmsg )
	    {
		/* Grab a glcmsg off of the shared queue. */
		do
		{
		    if ( 0 != (oldval = CX_Proc_CB.cx_ncb->cxvms_glm_free) )
		    {
			glcmsg = (CX_GLC_MSG *)CX_OFFSET_TO_PTR(oldval);
		    }
		    else
		    {
			glcmsg = NULL;
			if ( 0 == glc->cx_glc_free )
			{
			    /*
			    ** No free blocks either on private or shared 
			    **
			    ** This is unexpected as CX facility allocates
			    ** enough blocks to cover the configured xacts
			    ** for this node.  Since each session should
			    ** need at most one GLM, we should never
			    ** run out.  However, it would be nicer
			    ** not to simply bomb here, but instead wait
			    ** for one to be freed.
			    */
			    TRdisplay( "%@ %s:%d No CX_GLC_MSG blocks\n",
			               __FILE__, __LINE__ );
			    status = E_CL2C12_CX_E_INSMEM;
			    return status;
			}
			break;
		    }
		} while ( CScas4( &CX_Proc_CB.cx_ncb->cxvms_glm_free, oldval, 
			  glcmsg->cx_glm_next ) );
	    }
	    else
	    {
		glc->cx_glc_fcnt--; /* Unprotected update */
	    }
	} while ( NULL == glcmsg );

	/* Package message. All GLC req CB's must be in the same SMS */
	glcmsg->cx_glm_flags = flags;
	glcmsg->cx_glm_opcode = opcode;
	glcmsg->cx_glm_req_off = CX_PTR_TO_OFFSET(preq);
	CSget_cpid( &glcmsg->cx_glm_caller );
	glcmsg->cx_glm_instance = preq->cx_instance;
	
	/* Add message to queue for target process. */
	glmoff = oldval;
	do
	{
	    if ( !CS_ISSET(&glc->cx_glc_valid) )
	    {
		/* Our target has died/gone away */
		TRdisplay( "%@ %s:%d No CX_GLC_MSG blocks\n",
			   __FILE__, __LINE__ );

		/* Give GLM to shared free list */
		do
		{
		    oldval = CX_Proc_CB.cx_ncb->cxvms_glm_free;
		    glcmsg->cx_glm_next = oldval;
		} while ( CScas4( &CX_Proc_CB.cx_ncb->cxvms_glm_free, oldval, 
			  glcmsg->cx_glm_next ) );

		/* Report the bad news */
		preq->cx_status = status = E_CL2C25_CX_E_DLM_NOTHELD;
		return status;
	    }
	    oldval = glc->cx_glc_glms;
	    glcmsg->cx_glm_next = oldval;
	} while ( CScas4( &glc->cx_glc_glms, oldval, glmoff ) );

	/* If GLC thread is sleeping, wake it */
	if ( (CSadjust_counter( &glc->cx_glc_triggered, 1 )) == 1 )
	    CScp_resume( &glc->cx_glc_threadid );

	if ( (opcode == CX_DLM_OP_RLSE) ||
	     (flags & (CX_F_WAIT|CX_F_NOWAIT)) ||
	     (opcode == CX_DLM_OP_CANC) )
	{
	    /*
	    ** Synchronous call: suspend until resumed by lock
	    ** processing thread in target. then call user registered
	    ** functions as needed.
	    */
	    status = CSsuspend( CS_LOCK_MASK, 0, (PTR)&preq->cx_key );
	    if ( OK == status )
	    {
		/* Retrieve final status */
		status = preq->cx_status;

		switch (opcode)
		{
		case CX_DLM_OP_RQST:
		case CX_DLM_OP_CONV:
		    if ( preq->cx_lki_flags & (CX_LKI_F_NOTIFY|CX_LKI_F_NAB) )
		    {
			(*preq->cx_user_func)( preq, preq->cx_status );
		    }
		    break;
		case CX_DLM_OP_RLSE:
		    break;
		case CX_DLM_OP_CANC:
		    status = glcmsg->cx_glm_opcode;
		    cx_vms_free_glm( glc, &glcmsg );
		    break;
		}
	    }

	    /* Verify grant/convert state of VMS lock */
	    if ( opcode == CX_DLM_OP_RQST || opcode == CX_DLM_OP_CONV )
		VERIFY_LOCK(preq, status, flags);

	    /* No double suspend */
	    preq->cx_inproxy = FALSE;
	    preq->cx_lki_flags &= ~CX_LKI_F_QUEUED;
	}
	else
	{
	    /*
	    ** Asynchronous call: CXdlm_wait will block on this request
	    ** until resumed by the proxy process.
	    */
	    status = OK;
	}
    } while (0);
    return status;
} /* cx_vms_pass_dlm_req */

#endif /* end _VMS_ */

#endif /* CLUSTER_BUILD */


/*{
** Name: CXdlm_convert	- Convert an existing lock
**
** Description:
**
**	Perform either a synchronous or asynchronous DLM lock
**	conversion request to the same or a different lock mode.
**
** Inputs:
**
**	flags 		Bit mask of one or more of the following:
**
**	    - CX_F_NOWAIT	Equal to LK_NOWAIT.   Request fails 
**				if it cannot be granted synchronously.
**
**	    - CX_F_NODEADLOCK	Set if lock should ignored when checking for 
**				deadlock cycles.   This may be the case, if 
**				calling thread is not blocked on lock, or if 
**				lock will be released when a blocking
**				notification routine associated with this 
**				lock is triggered.
**
**	    - CX_F_WAIT		Set if synchronous conversion is desired.   
**				Calling thread will block until lock is 
**				granted in new mode, or request deadlocks, 
**				or otherwise fails.
**
**	    - CX_F_OWNSET	Set if cx_session & cx_pid already filled
**				out. A minor optimization, but getting SID
**				on an OS threaded system may not be
**				negligibly cheap. 
**
**	    - CX_F_IGNOREVALUE  Set by lock caller, if lock value is
**				not to be updated.  This is only used
**				by LK in the case where a non-CSP process
**				has converted a lock, and found an IVB,
**				and needs to back out the convert.
**		
**	    - CX_F_NOTIFY	Set if instead of a blocking notification,
**				a completion notification is required.
**				Calling thread will NOT be resumed if
**				this is specified.
**
**          - CX_F_NAB          Set if both completion & blocking notification
**                              is required.  Address of blocking routine
**                              to be passed in cx_user_extra.
**
**	    - CX_F_LSN_IN_LVB	Set if CX needs to refresh local internal
**				LSN from the LVB.  Will only do this if
**				LVB valid, and new mode is LK_SIX, or LK_X.
**
**	    - CX_F_PROXY_OK	Sanity check flag set if the lock request
**			        can be processed by another process.  This
**				is only used under VMS where we have no true
**				group lock container, and locks must be handled
**			        by the allocating process.  Only set this
**			        flag if request block resides in the LGK
**				shared memory segment.
**
**	    - CX_F_USEPROXY	If set, completion routine will extract
**			        the process/session to resume indirectly from 
**				cx_user_extra.
**
**	    - CX_F_NOQUECVT	If set, do not queue conversion requests
**			        behind existing convert requests. Used solely 
**				by lk_handle_ivb.
**
**	preq		Pointer to CX_REQ_CB.   The memory used for
**	   		this block must stay allocatted so long as
**			there is any chance that a completion routine
**			will be triggered (request is not immediately
**			granted or rejected), or until lock is released,
**			if a blocking notification routine was specified.
**		
**			Field usage as follows:
**		
**	    - cx_lock_id	Must be set to unique identifier for 
**				a granted lock owned by calling session.
**
**	    - cx_old_mode	Should be set to current mode held, or 0.
**
**	    - cx_new_mode	Must be set to new requested Ingres lock mode.
**
**	    - cx_status		Will be set to zero until request completes.
**
**	    - cx_owner		Will have ownership filled in, if asynch.
**				completion possible and CX_F_OWNSET not set.
**
**	    - cx_key		Ignored.
**
**	    - cx_user_func 	If non-zero, must be the address of a callback
**				routine to be triggered if one or more 
**				incompatable requests are made for the 
**				resource associated with this lock instance.
**				If CX_F_NOTIFY set, then instead of a blocking
**				notification routine, this holds the address
**				of a user completion routine to be called in
**				leiu of resuming the caller if asynchronous
**				completion is required.
**
**	    - cx_user_extra	Pointer to optional user defined data
**				to be used by blocking routines, or if
**                              CX_F_NAB set blocking notification function
**                              address.
**
**	    - cx_value		Buffer to hold lock value when request
**				is granted.
**
**	    			cx_value will be updated with current value 
**				of resource when conversion is granted if lock
**				held in a non-exclusive mode, and new mode is
**				equal to or greater than old mode.  Value
**				will be written to DLM if current mode held
**				is SIX or X.   
**    	
** Outputs:
**	preq		Following fields updated or conditionally updated
**			as described above.  cx_status, 
**			cx_owner, cx_value.
**
**	Returns:
**		
**		OK			- Normal successful completion.
**		  If request was queued, this is only an intermediate
**		  return code, and CXdlm_wait must be called, or cx_status
**		  in request block polled, to determine final status.
**		E_CL2C01_CX_I_OKSYNC	- Distinct success code returned
**		  if request synchronously granted, and CX_F_STATUS set.
**
**		E_CL2C09_CX_W_SYNC_IVB  - Request synchronously granted,
**					  but value block was invalid.
**
**		E_CL2C10_CX_E_NOSUPPORT - CX DLM not supported this OS.
**		E_CL2C11_CX_E_BADPARAM	- Invalid argument passed.
**		E_CL2C1B_CX_E_BADCONTEXT - CX state not JOINED,
**		  or CX_F_PCONTEXT not set & CX state not INIT or JOINED.
**		E_CL2C21_CX_E_DLM_DEADLOCK - synchronous lock request
**		  chosen as deadlock victim.
**		E_CL2C22_CX_E_DLM_CANCEL - synchronous lock request
**                canceled.
**		E_CL2C1C_CX_E_LSN_LVB_FAILURE - Lock Granted, but
**		  there was a failure in LSN propagation.  Caller
**		  MUST abort his transaction.
**		E_CL2C27_CX_E_DLM_NOGRANT - CX_F_NOWAIT was set, and
**		  conversion could not be synchronously granted.
**		E_CL2C3x_CX_E_OS_xxxxxx	- Misc implementation dependent
**		  failures at the OS DLM level.
**
** Outputs:
**	none.
**
**	Returns:
**		OK	- Normal successful completion.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-feb-2001 (devjo01)
**	    Created.
**	10-oct-2005 (devjo01)
**	    Add CX_F_OWNSET logic to VMS implementation,
**	    even if PID cannot change, session to be resumed may.
**	30-Mar-2007 (jonj)
**	    Add CX_F_NOQUECVT to prevent reordering of extant
**	    lock requests.
**	18-May-2007 (jonj)
**	    Clear cx_dlm_posted before doing anything else.
**	04-Apr-2008 (jonj)
**	    Get event flag using lib$get_ef() rather than defaulting
**	    to zero.
**	09-May-2008 (jonj)
**	    If from GLC and request has already been posted
**	    (like by deadlock), just return OK.
**	11-Jul-2008 (jonj)
**	    Don't extract cx_dlm_status prematurely when doing
**	    async enq's, that's the AST's task.
**	23-July-2008 (jonj)
**	    Relocate cx_lki_flags CX_LKI_F_USEPROXY to inter-process-safe
**	    cx_inproxy variable.
**	    Stow resultant DLM flags in cx_dlmflags as debugging aid.
**	    Increment cx_instance on each reuse of preq.
*/
STATUS
CXdlm_convert( u_i4 flags, CX_REQ_CB *preq )
{
    STATUS	 status;

    CX_Stats.cx_dlm_converts++;

#ifdef	CX_PARANOIA
    if ( (NULL == preq) || CX_UNALIGNED_PTR(preq) )
	return E_CL2C11_CX_E_BADPARAM;

    for ( ; ; )
    {
	if ( (flags & ~(CX_F_NOWAIT|CX_F_WAIT|CX_F_STATUS|CX_F_OWNSET|
	     CX_F_NODEADLOCK|CX_F_IGNOREVALUE|CX_F_NOTIFY|CX_F_DLM_TRACE|
	     CX_F_LOCALLOCK|CX_F_NAB|CX_F_LSN_IN_LVB|CX_F_USEPROXY|
	     CX_F_PROXY_OK|CX_F_NOQUECVT)) ||
	     ((CX_F_IGNOREVALUE|CX_F_LSN_IN_LVB) ==
	      ((CX_F_IGNOREVALUE|CX_F_LSN_IN_LVB) & flags)) ||
	     (preq->cx_new_mode > LK_X) ||
	     ( (CX_F_NOTIFY & flags) && !preq->cx_user_func ) )
	    status = E_CL2C11_CX_E_BADPARAM;
	else if ( CX_Proc_CB.cx_state != CX_ST_JOINED &&
		  !( (CX_Proc_CB.cx_state == CX_ST_INIT) &&
		     ((preq->cx_lki_flags & CX_LKI_F_PCONTEXT) ||
		      (CX_Proc_CB.cx_flags & CX_PF_FIRSTCSP))))
	    status = E_CL2C1B_CX_E_BADCONTEXT;
	else
	    break;
	preq->cx_status = status;
	return status;
    }
#endif /*CX_PARANOIA*/

    preq->cx_lki_flags &= CX_LKI_F_PCONTEXT | CX_LKI_F_NO_DOMAIN;
    preq->cx_lki_flags |= CX_LKI_F_ISCONVERT |
     ( (CX_F_NOTIFY|CX_F_NAB|CX_F_LSN_IN_LVB|CX_F_USEPROXY) & flags );

    /* Don't clear status again if proxy call from GLC */
    if ( !preq->cx_inproxy )
    {
	preq->cx_status = 0;
	preq->cx_instance++;
    }

    if ( ( CX_F_LSN_IN_LVB & flags ) &&
	 ( preq->cx_new_mode < LK_SIX ) &&
	 ( preq->cx_old_mode >= LK_SIX ) )
    {
	CX_LSN		*plsn = CX_LSN_IN_LVB_PTR(preq);

	/*
	** Pop current local LSN into LVB.
	**
	** Failure here is not fatal (or likely), as the only penalty
	** is that any potential lock acquirer will see an empty LSN
	** and will have to globally sync.
	*/
	plsn->cx_lsn_low = plsn->cx_lsn_high = 0;
	(void)CXcmo_lsn_lsynch( plsn );
	CX_Stats.cx_cmo_lsn_lvbrsyncs++;
    }

    /*
    **	Start OS dependent declarations.
    */

#if !defined(conf_CLUSTER_BUILD)
    status = E_CL2C10_CX_E_NOSUPPORT;
#elif defined(axp_osf)
    /*
    **	Tru64 implementation
    */
    {
    dlm_status_t dlmstat;	/* Tru64 dlm status code */
    dlm_flags_t dlmflags;	/* Tru64 dlm flag bits */
    dlm_lkmode_t dlmmode;	/* Tru64 lock modes */
    blk_callback_t blkrtn;	/* Tru64 blocking callback */

    dlmflags = DLM_SYNCSTS;
    if ( CX_F_NODEADLOCK & flags )
	dlmflags |= DLM_NODLCKBLK | DLM_NODLCKWT;
    if ( CX_F_NOWAIT & flags )
	dlmflags |= DLM_NOQUEUE;
    else if ( !(flags & CX_F_NOQUECVT) &&
              (1 << preq->cx_new_mode) & cxaxp_waitcvts[ preq->cx_old_mode ] )
	dlmflags |= DLM_QUECVT;
    if ( !(CX_F_IGNOREVALUE & flags) )
	dlmflags |= DLM_VALB;

    dlmmode = cxaxp_lk_to_dlm_mode[preq->cx_new_mode];

    blkrtn = NULL;
    if ( ( preq->cx_user_func && !( CX_F_NOTIFY & flags ) ) ||
         ( preq->cx_user_extra && ( CX_F_NAB & flags ) ) )
	blkrtn = cxlnx_blk_rtn;


    if ( !( (CX_F_WAIT|CX_F_NOWAIT|CX_F_OWNSET) & flags ) )
    {
	/* Fill in ownership info */
	CSget_cpid( &preq->cx_owner );
    }

    /* Stow dlmflags in preq for debugging */
    preq->cx_dlmflags = (u_i4)dlmflags;

    if ( (CX_F_WAIT|CX_F_NOWAIT) & flags )
	dlmstat = DLM_CVT(  (dlm_lkid_t *)&preq->cx_lock_id,
	    dlmmode, (dlm_valb_t *)preq->cx_value, dlmflags, 
	    (callback_arg_t)preq, (callback_arg_t)NULL,
	    blkrtn, (uint_t)0 ); 
    else
	dlmstat = DLM_QUECVT_f( (dlm_lkid_t *)&preq->cx_lock_id,
	    dlmmode, (dlm_valb_t *)preq->cx_value, dlmflags, 
	    (callback_arg_t)preq, (callback_arg_t)NULL,
	    blkrtn, (uint_t)0, cxaxp_cmpl_rtn );

    status = cx_translate_status( (i4)dlmstat );
    }
#elif defined(LNX)
    /*
    **	Linux implementation
    */
    {
    dlm_stats_t	 dlmstat;	/* OpenDLM status code */
    int		 dlmflags;	/* OpenDLM flag bits */
    int		 dlmmode;	/* OpenDLM lock modes */
    dlm_bastlockfunc_t *blkrtn;	/* OpenDLM blocking callback */

    dlmflags = LKM_SYNCSTS | LKM_CONVERT; 

    if ( CX_F_NOWAIT & flags )
	dlmflags |= LKM_NOQUEUE;

    if ( CX_F_NODEADLOCK & flags )
	dlmflags |= LKM_NODLCKWT;

    if ( !(CX_F_IGNOREVALUE & flags ) )
    {
	dlmflags |= LKM_VALBLK;
    }

    if ( preq->cx_lki_flags & CX_LKI_F_NO_DOMAIN )
    {
	dlmflags |= LKM_NO_DOMAIN;
    }

    dlmmode = cxlnx_lk_to_dlm_mode[preq->cx_new_mode];

    blkrtn = NULL;
    if ( preq->cx_user_func && !( CX_F_NOTIFY & flags ) )
	blkrtn = cxlnx_blk_rtn;

    if ( !( (CX_F_WAIT|CX_F_NOWAIT|CX_F_OWNSET) & flags ) )
    {
	/* Fill in ownership info */
	CSget_cpid( &preq->cx_owner );
    }

    /* Stow dlmflags in preq for debugging */
    preq->cx_dlmflags = (u_i4)dlmflags;

    if ( (CX_F_WAIT|CX_F_NOWAIT) & flags )
    {
	dlmstat = DLMLOCK_SYNC( dlmmode, (struct lockstatus *)&preq->cx_dlm_ws,
	    dlmflags, NULL, 0, preq, blkrtn );
	if ( DLM_NORMAL == dlmstat ) dlmstat = preq->cx_dlm_ws.cx_dlm_status;
    }
    else
    {
	dlmstat = DLMLOCK( dlmmode, (struct lockstatus *)&preq->cx_dlm_ws,
	    dlmflags, NULL, 0, cxlnx_cmpl_rtn, preq, blkrtn );
	if ( DLM_SYNC == dlmstat &&
	     DLM_NORMAL != preq->cx_dlm_ws.cx_dlm_status )
	     dlmstat = preq->cx_dlm_ws.cx_dlm_status;
    }

    status = cx_translate_status( (i4)dlmstat );
    }

#elif defined(VMS) /* Start _VMS_ */
    /*
    **	OpenVMS Implementation
    */
    {
    unsigned	 osstat;	/* VMS OS status code */
    int		 dlmflags;	/* VMS DLM flag bits */
    int		 dlmmode;	/* VMS DLM lock modes */
    void	 (*blkrtn)(void *);/* VMS DLM blocking callback */
    II_VMS_EF_NUMBER	efn;

#ifdef VERIFY_VMS_LOCKS
    if ( preq->cx_lki_flags & CX_LKI_F_QUEUED )
    {
	char	keybuffer[256];
	CS_CPID *rqst = (CS_CPID*)((PTR)(CX_RELOFF2PTR(preq, preq->cx_user_extra))
				+ CX_Proc_CB.cx_proxy_offset);
	TRdisplay("%@ CXdlm_convert %d QUEUED flags %x lki_flags %x mode %d,%d\n"
		"\towner %x.%x rqst %x.%x instance %d\n"
		"\tdlm %x key %s\n",
		__LINE__,
		flags, preq->cx_lki_flags,
		preq->cx_old_mode, preq->cx_new_mode,
		preq->cx_owner.pid, preq->cx_owner.sid,
		rqst->pid, rqst->sid,
		preq->cx_instance,
		preq->cx_lock_id,
		(*CX_Proc_CB.cx_lkkey_fmt_func)(&preq->cx_key, keybuffer));
    }
#endif /* VERIFY_VMS_LOCKS */

    /* Don't clear posted flag if proxy call from GLC */
    if ( preq->cx_inproxy )
    {
	/* If lagging GLC, just return */
	if ( CXDLM_IS_POSTED(preq) )
	    return(OK);
    }
    else
    {
	/* Clear posted flag */
	CS_ACLR(&preq->cx_dlm_ws.cx_dlm_posted);
    }

    dlmflags = LCK$M_SYNCSTS | LCK$M_CONVERT |
      LCK$M_NODLCKWT | LCK$M_NODLCKBLK | LCK$M_PROTECT;

    if ( CX_Proc_CB.cx_pid != preq->cx_owner.pid )
    {
	/*
	** We need to pass off operation on this lock to the actual
	** process which allocated it.  This is because VMS at
	** present does not support group lock containers.
	*/
	status = cx_vms_pass_dlm_req( flags, preq, CX_DLM_OP_CONV );
	return status;
    }

    if ( CX_F_NOWAIT & flags )
	dlmflags |= LCK$M_NOQUEUE;
    else if ( !(flags & CX_F_NOQUECVT) &&
              (1 << preq->cx_new_mode) & cxvms_waitcvts[ preq->cx_old_mode ] )
	dlmflags |= LCK$M_QUECVT;

    if ( !(CX_F_IGNOREVALUE & flags ) )
    {
	dlmflags |= LCK$M_VALBLK | LCK$M_XVALBLK;
    }

    dlmmode = cxvms_lk_to_dlm_mode[preq->cx_new_mode];

    /*
    ** Per the VMS documentation, "this flag is valid only for
    ** new lock requests", which this is not.
    ** I don't think it does any harm to set it, but it's
    ** misleading to the code readers at best.
    **
    ** if ( LCK$K_NLMODE == dlmmode )
    ** {
    **     dlmflags |= LCK$M_EXPEDITE;
    ** }
    */

    if ( !( (CX_F_WAIT|CX_F_NOWAIT|CX_F_OWNSET) & flags ) )
    {
	/* Fill in ownership info */
	CSget_cpid( &preq->cx_owner );
    }

    blkrtn = NULL;
    if ( preq->cx_user_func && !( CX_F_NOTIFY & flags ) )
	blkrtn = cxvms_blk_rtn;

    /* Stow dlmflags in preq for debugging */
    preq->cx_dlmflags = (u_i4)dlmflags;

    if ( CX_F_WAIT & flags )
    {
	osstat = sys$enqw( EFN$C_ENF, dlmmode,
			   (struct _lksb *)&preq->cx_dlm_ws, dlmflags,
	                   NULL, 0, NULL, preq, blkrtn,
			   PSL$C_USER, CX_Proc_CB.cxvms_dlm_rsdm_id, NULL );
	if ( SS$_NORMAL == osstat ) 
	    osstat = preq->cx_dlm_ws.cx_dlm_status;
    }
    else
    {
	osstat = sys$enq( EFN$C_ENF, dlmmode,
			  (struct _lksb *)&preq->cx_dlm_ws, dlmflags,
	                  NULL, 0, cxvms_cmpl_rtn, preq, blkrtn,
			  PSL$C_USER, CX_Proc_CB.cxvms_dlm_rsdm_id, NULL );
    }

#ifdef VERIFY_VMS_LOCKS
    if ( osstat == SS$_BADPARAM )
    {
	char	keybuffer[256];
	CS_CPID *rqst = (CS_CPID*)((PTR)(CX_RELOFF2PTR(preq, preq->cx_user_extra))
				+ CX_Proc_CB.cx_proxy_offset);
	TRdisplay("%@ CXdlm_convert %d BADPARAM flags %x dlmflags %x lki_flags %x mode %d,%d\n"
		"\towner %x.%x rqst %x.%x instance %d\n"
		"\tdlm %x key %s\n",
		__LINE__,
		flags, dlmflags, preq->cx_lki_flags,
		preq->cx_old_mode, preq->cx_new_mode,
		preq->cx_owner.pid, preq->cx_owner.sid,
		rqst->pid, rqst->sid,
		preq->cx_instance,
		preq->cx_lock_id,
		(*CX_Proc_CB.cx_lkkey_fmt_func)(&preq->cx_key, keybuffer));

	TRdisplay("%@ SLEEPING - take a lockstat\n");
	CSsuspend(CS_INTERRUPT_MASK, 0, 0);
    }
#endif /* VERIFY_VMS_LOCKS */

    status = cx_translate_status( (i4)osstat );

    } /* end _VMS_ */
#else 
    /*
    **	All other platforms
    */
    status = E_CL2C10_CX_E_NOSUPPORT;
#endif /*OS Specific*/
    /*
    **	End OS dependent declarations.
    */

    /* Was request queued? */
    if ( OK == status && !(flags & (CX_F_WAIT|CX_F_NOWAIT)) )
    {
	CX_Stats.cx_dlm_quedcvts++;
	preq->cx_lki_flags |= CX_LKI_F_QUEUED;
    }
    else
    {
	/* Request not queued, set final code. */
	preq->cx_status = status;

	/* Post request as complete for LK */
	CXdlm_mark_as_posted(preq);

	/* If called from LK, verify lock conversion */
	if ( flags & CX_F_PROXY_OK )
	    VERIFY_LOCK(preq, status, flags);

	/* Update old mode, if synchronously granted */
	if ( ( E_CL2C01_CX_I_OKSYNC == status ) ||
	     ( OK == status ) ||
	     ( E_CL2C08_CX_W_SUCC_IVB == status ) )
	{
	    CX_Stats.cx_dlm_sync_cvts++;

	    if ( ( CX_F_LSN_IN_LVB & flags ) && 
	         ( preq->cx_new_mode >= LK_SIX ) &&
		 ( preq->cx_old_mode < LK_SIX ) )
	    {
		/*
		** If converting to a write mode (but not
		** from a write mode to a write mode), and passing
		** LSN info in the LVB, synchronize the Local CX LSN
		*/ 
		if ( OK != cx_dlm_sync_lsn_from_dlm( preq ) )
		    status = E_CL2C1C_CX_E_LSN_LVB_FAILURE;
	    }
	    preq->cx_old_mode = preq->cx_new_mode;
	}
    }

    CX_DLM_TRACE("CONV", preq);

    /* Convert informational statuses to OK if CX_F_STATUS not set. */
    if ( !( CX_F_STATUS & flags ) && CX_OK_STATUS(status) )
	status = OK;

    /* Return ( possibly intermediate ) status code. */
    return status;
} /* CXdlm_convert */




/*{
** Name: CXdlm_release	- Release a previously granted DLM lock
**
** Description:
**
**	Release a single DLM lock.
**
** Inputs:
**
**	flags 		Bit mask of zero or more of the following:
**
**	    - CX_F_IGNOREVALUE  Set by lock caller, if lock value is
**				not to be updated.  This is only used
**				by LK in the case where a non-CSP process
**				was granted a new lock, and found an IVB,
**				and needs to release the premature grant.
**
**	    - CX_F_INVALIDATE	If lock was held in an exclusive mode
**				(SIX, or X), lock value block is
**				marked invalid as the lock is released.
**				For test purposes only.  Do NOT use with
**				regular LK operations.
**
**	    - CX_F_CLEAR_ONLY	Release is called in a context where
**				we known underlying DLM lock has been
**				released, or cx_lock_id is otherwise
**				invalid.  If set request structure is
**				reset as if lock was released, but
**				no actual DLM operation is performed.
**		
**	    - CX_F_PROXY_OK	Sanity check flags set if the lock release
**			        can be processed by another process.  This
**				is only used under VMS where we have no true
**				group lock container, and locks must be handled
**			        by the allocating process.  Only set this
**			        flag if request block resides in the LGK
**				shared memory segment.
**
**	preq		Pointer to CX_REQ_CB.
**
**	    - cx_lock_id	Must be set to unique identifier for 
**				a granted lock, cleared on completion.
**
**	    - cx_old_mode	Ignored, cleared on completion.
**
**	    - cx_new_mode	Ignored, cleared on completion.
**
**	    - cx_status		Ignored.
**
**	    - cx_owner		Ignored.
**
**	    - cx_key		Ignored.
**
**	    - cx_user_func 	Ignored.
**
**	    - cx_user_extra	Ignored.
**    	
**	    - cx_value		Buffer to hold lock value when request
**				is granted.  Value in cx_value will be 
**				written to DLM if lock held is SIX or X
**				mode and CX_F_INVALIDATE is not set.
**
** Outputs:
**	none.
**
**	Returns:
**		OK			- Normal successful completion.
**		E_CL2C10_CX_E_NOSUPPORT - CX DLM not supported this OS.
**		E_CL2C11_CX_E_BADPARAM	- Invalid argument passed.
**		E_CL2C25_CX_E_DLM_NOTHELD - Lock not held.
**		E_CL2C3x_CX_E_OS_xxxxxx	- Misc implementation dependent
**		  failures at the OS DLM level.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    Waiting incompatible lock requests may be granted.
**
** History:
**	13-feb-2001 (devjo01)
**	    Created.
**	23-Jul-2008 (jonj)
**	    Increment cx_instance on each reuse of preq.
*/
STATUS
CXdlm_release( u_i4 flags, CX_REQ_CB *preq )
{
    STATUS	 status;

    CX_Stats.cx_dlm_releases++;

#ifdef	CX_PARANOIA
    if ( (NULL == preq) || CX_UNALIGNED_PTR(preq) )
	return E_CL2C11_CX_E_BADPARAM;

    if ( flags & ~(CX_F_INVALIDATE|CX_F_IGNOREVALUE|
		   CX_F_CLEAR_ONLY|CX_F_DLM_TRACE|CX_F_PROXY_OK) )
    {
	preq->cx_status = E_CL2C11_CX_E_BADPARAM;
	return E_CL2C11_CX_E_BADPARAM;
    }

#endif /*CX_PARANOIA*/

    CX_DLM_TRACE("REL", preq);

    /*
    **	Start OS dependent declarations.
    */

    preq->cx_instance++;

    if ( CX_F_CLEAR_ONLY & flags )
    {
	status = OK;
    }
    else
    {
#if !defined(conf_CLUSTER_BUILD)
    status = E_CL2C10_CX_E_NOSUPPORT;
#else

    if ( ( CX_LKI_F_LSN_IN_LVB & preq->cx_lki_flags ) &&
         ( preq->cx_old_mode >= LK_SIX ) )
    {
	CX_LSN		*plsn = CX_LSN_IN_LVB_PTR(preq);

	/*
	** Pop current local LSN into LVB if using LVB to
	** help synchronize the CX LSN, and converting
	** downward from a write mode.
	**
	** Failure here is not fatal (or likely), as the only penalty
	** is that any potential lock acquirer will see an empty LSN
	** and will be have to globally sync.
	*/
	plsn->cx_lsn_low = plsn->cx_lsn_high = 0;
	(void)CXcmo_lsn_lsynch( plsn );
	CX_Stats.cx_cmo_lsn_lvbrsyncs++;
    }
    
#if defined(axp_osf)
    /*
    **	Tru64 implementation
    */
    {
    dlm_status_t dlmstat;	/* Tru64 dlm status code */
    dlm_flags_t dlmflags;	/* Tru64 dlm flag bits */

    if ( CX_F_INVALIDATE & flags )
	dlmflags = DLM_INVVALBLK;
    else if ( !(CX_F_IGNOREVALUE & flags) )
	dlmflags = DLM_VALB;
    else
	dlmflags = 0;

    /* Stow dlmflags in preq for debugging */
    preq->cx_dlmflags = (u_i4)dlmflags;

    dlmstat = DLM_UNLOCK( (dlm_lkid_t *)&preq->cx_lock_id,
		    (dlm_valb_t *)preq->cx_value, dlmflags );

    status = cx_translate_status( (i4)dlmstat );
    }
#elif defined(LNX)
    /*
    **	Linux OpenDLM implementation
    */
    {
    dlm_stats_t	 dlmstat;	/* OpenDLM dlm status code */
    int		 dlmflags;	/* OpenDLM dlm flag bits */

    if ( CX_F_INVALIDATE & flags )
	dlmflags = LKM_INVVALBLK;
    else if ( !(CX_F_IGNOREVALUE & flags) )
	dlmflags = LKM_VALBLK;
    else
	dlmflags = 0;

    /* Stow dlmflags in preq for debugging */
    preq->cx_dlmflags = (u_i4)dlmflags;

    dlmstat = DLMUNLOCK_SYNC( preq->cx_lock_id, (char *)preq->cx_value,
			      dlmflags );

    status = cx_translate_status( (i4)dlmstat );
    }
#elif defined(VMS) /* Start _VMS_ */
    /*
    **	OpenVMS implementation
    */
    {
    int		 dlmstat;	/* OpenDLM dlm status code */
    int		 dlmflags;	/* OpenDLM dlm flag bits */
    void	*valp;

    if ( CX_Proc_CB.cx_pid != preq->cx_owner.pid )
    {
	/*
	** We need to pass off operation on this lock to the actual
	** process which allocated it.  This is because VMS at
	** present does not support group lock containers.
	*/
	status = cx_vms_pass_dlm_req( flags, preq, CX_DLM_OP_RLSE );
	return status;
    }

    if ( CX_F_INVALIDATE & flags )
	dlmflags = LCK$M_INVVALBLK;
    else
	dlmflags = 0;

    valp = (void *)&preq->cx_value;
    if ( CX_F_IGNOREVALUE & flags )
    {
	valp = NULL;
    }

    /* Stow dlmflags in preq for debugging */
    preq->cx_dlmflags = (u_i4)dlmflags;

    dlmstat = sys$deq( preq->cx_lock_id, valp, PSL$C_USER, dlmflags );

    status = cx_translate_status( (i4)dlmstat );

    }
    /* end _VMS_ */
#else
    /*
    **	All other platforms
    */
    status = E_CL2C10_CX_E_NOSUPPORT;
#endif /*OS SPECIFIC*/
#endif /* Cluster support */
    }
    /*
    **	End OS dependent declarations.
    */
    if ( OK == (preq->cx_status = status) )
    {
	preq->cx_lki_flags = 0;
	CX_ZERO_OUT_ID(&preq->cx_lock_id);
	preq->cx_old_mode = preq->cx_new_mode = LK_N;
    }
    return status;
} /* CXdlm_release */




/*{
** Name: CXdlm_cancel	- Cancel a previously queued lock request.
**
** Description:
**
**	Cancel a previously queued lock or conversion request.
**
** Inputs:
**
**	flags
**
**	    - CX_F_PROXY_OK	Sanity check flags set if the lock request
**			        can be processed by another process.  This
**				is only used under VMS where we have no true
**				group lock container, and locks must be handled
**			        by the allocating process.  Only set this
**			        flag if request block resides in the LGK
**				shared memory segment.
**
**	preq		Pointer to CX_REQ_CB.   
**		
**	    - cx_lock_id	Must be set to unique identifier for 
**				a granted lock, cleared on completion.
**
**	    - cx_old_mode	Ignored, cleared on completion.
**
**	    - cx_new_mode	Ignored, cleared on completion.
**
**	    - cx_status		Ignored, set on completion.
**
**	    - cx_owner		Ignored.
**
**	    - cx_key		Ignored.
**
**	    - cx_user_func 	Ignored.
**
**	    - cx_user_extra	Ignored.
**    	
**	    - cx_value		Ignored.
**
** Outputs:
**	none.
**
**	Returns:
**		OK			- Normal successful completion.
**		E_CL2C09_CX_W_CANT_CAN	- Request granted before canceled.
**		E_CL2C10_CX_E_NOSUPPORT - CX DLM not supported this OS.
**		E_CL2C11_CX_E_BADPARAM	- Invalid argument passed.
**		E_CL2C25_CX_E_DLM_NOTHELD - Invalid lock ID.
**		E_CL2C3x_CX_E_OS_xxxxxx	- Misc implementation dependent
**		  failures at the OS DLM level.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    Other lock requests may become grantable when target request
**	    is canceled.   Completion routine for enqueued request will
**	    be triggered with E_CL2C22_CX_E_DLM_CANCEL status.
**
** History:
**	16-feb-2001 (devjo01)
**	    Created.
**	08-jan-2004 (devjo01)
**	    Changed 2nd parameter to CX_REQ_CB *, to allow cx_lock_id
**	    type to vary by underlying DLM implementation.
**	23-Jul-2008 (jonj)
**	    Increment cx_instance on each reuse of preq.
*/
STATUS
CXdlm_cancel( u_i4 flags, CX_REQ_CB *preq )
{
    STATUS	 status;

    CX_Stats.cx_dlm_cancels++;

#ifdef	CX_PARANOIA
    if ( flags & ~(CX_F_DLM_TRACE|CX_F_PROXY_OK) )
	return E_CL2C11_CX_E_BADPARAM;
#endif /*CX_PARANOIA*/

    /*
    **	Start OS dependent declarations.
    */

    preq->cx_instance++;

#if !defined(conf_CLUSTER_BUILD)
    status = E_CL2C10_CX_E_NOSUPPORT;
#elif defined(axp_osf)
    /*
    **	Tru64 implementation
    */
    {
    dlm_status_t dlmstat;	/* Tru64 dlm status code */

    dlmstat = DLM_CANCEL_f( (dlm_lkid_t *)&preq->cx_lock_id, DLM_CANCELWAITOK );

    status = cx_translate_status( (i4)dlmstat );
    }
#elif defined(LNX)
    /*
    **	OpenDLM implementation
    */
    {
    dlm_stats_t dlmstat;		/* OpenDLM status code */

    dlmstat = DLMUNLOCK_SYNC( preq->cx_lock_id, NULL, LKM_CANCEL );

    status = cx_translate_status( (i4)dlmstat );
    } /* end LNX */
#elif defined(VMS) /* Start _VMS_ */
    /*
    **	OpenVMS implementation
    */
    {
    int		dlmstat;

    if ( CX_Proc_CB.cx_pid != preq->cx_owner.pid )
    {
	/*
	** We need to pass off operation on this lock to the actual
	** process which allocated it.  This is because VMS at
	** present does not support group lock containers.
	*/
	status = cx_vms_pass_dlm_req( flags, preq, CX_DLM_OP_CANC );
	return status;
    }

    dlmstat = sys$deq( preq->cx_lock_id, NULL, PSL$C_USER, LCK$M_CANCEL );

    status = cx_translate_status( (i4)dlmstat );

    } /* end _VMS_ */
#else
    /*
    **	All other platforms
    */
    status = E_CL2C10_CX_E_NOSUPPORT;
#endif /*OS SPECIFIC*/
    /*
    **	End OS dependent declarations.
    */
    CX_DLM_TRACE_STATUS("CAN", preq, status);

    return status;
} /* CXdlm_cancel */



/*{
** Name: CXdlm_wait	- Wait for an enqueued request to complete.
**
** Description:
**
**	Wait for a previously enqueued request or conversion to
**	complete.  This is complicated in that we may lose patience
**	(time out), or be interrupted (if CX_F_INTERRUPTOK set),
**	before completion routine is fired.   If this happens,
**	the pending request is cancelled, but there is a potential
**	race condition between the completion routine firing,
**	and the cancellation request.   However even if the
**	completion routine wins, and the request is granted, we must
**	still inform caller if an interrupt was received.
**
**	A single session should not have more than one DLM lock request
**	pending, since we cannot match up a resume with a completed
**	request.
**
**	CXdlm_wait MUST be called after any request that could possibly 
**	queue.
**
** Inputs:
**
**	flags 		Bit mask of zero or more of the following:
**
**	    - CX_F_INTERRUPTOK	Set if enqueued lock or conversion
**				request can be aborted due to time-out 
**				or user interrupt.
**		
**	preq		Pointer to CX_REQ_CB.   This should be the
**			same CX_REQ_CB as used for the orginal request,
**			or an identical copy.
**
**			Do not modify any of the request contents between
**			queued call and corrisponding wait.
**		
** Outputs:
**
**	preq->cx_status	- will hold final detailed request status.
**	preq->cx_old_mode - will hold current granted mode.
**
**	Returns:
**		OK			- Normal successful completion.
**					  Request or conversion granted.
**		E_CL2C04_CX_I_OKINTR	- Granted, but interrupt received.
**		E_CL2C08_CX_W_SUCC_IVB	- Granted, but value block invalid.
**		E_CL2C0A_CX_W_TIMEOUT   - Timed out w/o grant.
**		E_CL2C0B_CX_W_INTR	- User interrupt canceled grant.
**		E_CL2C0C_CX_W_INTR_IVB	- Granted, and interrupt received,
**					  AND value block invalid.
**		E_CL2C10_CX_E_NOSUPPORT - CX DLM not supported this OS.
**		E_CL2C11_CX_E_BADPARAM	- Invalid argument passed.
**		E_CL2C1A_CX_E_CORRUPT	- Thread resumed w/o cx_status set.
**		E_CL2C1C_CX_E_LSN_LVB_FAILURE - Lock Granted, but
**		  there was a failure in LSN propagation.  Caller
**		  MUST abort his transaction.
**		E_CL2C21_CX_E_DLM_DEADLOCK - Request was deadlock victim.
**		E_CL2C22_CX_E_DLM_CANCEL - Request canceled.
**		E_CL2C25_CX_E_DLM_NOTHELD - Lock not held.
**		E_CL2C3x_CX_E_OS_xxxxxx	- Misc implementation dependent
**		  failures at the OS DLM level.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    Session may be suspended.
**
** History:
**	16-feb-2001 (devjo01)
**	    Created.
**      14-mar-2005 (mutma03/devjo01)
**	  Resolved the issue with timedwait causing unadjusted sem count
**	  in cases where CSsuspend_for_AST with timedwait returns E_TIMEDOUT
**	  and the subsequent CXdlm_cancel fails due to the lock was granted
**	  and sem count is incremented. In this situation called 
**	  CSsuspend_for_AST to adjust the sem count.
**	21-nov-2006 (abbjo03)
**	    Fix some of the tests of the completion status.
**	05-may-2008 (joea)
**	    Eliminate CSsuspend after CXdlm_cancel since the sys$deq it calls
**	    is synchronous.
**	23-July-2008 (jonj)
**	    Relocate cx_lki_flags CX_LKI_F_USEPROXY to inter-process-safe
**	    cx_inproxy variable.
**      23-dec-2008 (stegr01)
**          Itanium VMS port
**/
STATUS
CXdlm_wait( u_i4 flags, CX_REQ_CB *preq, i4 timeout )
{
    STATUS	 status, suspstatus, cmplstatus;
    i4		 csflags, cancelflags;

#ifdef	CX_PARANOIA
    if ( (NULL == preq) || CX_UNALIGNED_PTR(preq) )
	return E_CL2C11_CX_E_BADPARAM;
    if ( flags & ~(CX_F_INTERRUPTOK|CX_F_DLM_TRACE) )
    {
	preq->cx_status = E_CL2C11_CX_E_BADPARAM;
	return E_CL2C11_CX_E_BADPARAM;
    }
#endif /*CX_PARANOIA*/

    if ( preq->cx_lki_flags & CX_LKI_F_QUEUED || preq->cx_inproxy )
    {
	/* 
	** Suspend session until woken by completion routine, 
	** (Resume may already be pending), or session is
	** interrupted, or lock request is canceled.
        */
	CX_DLM_TRACE("WAIT-",preq);

	CX_Stats.cx_dlm_async_waits++;

	if ( preq->cx_inproxy )
	{
	    cancelflags = CX_F_PROXY_OK;
	}
	else
	{
	    cancelflags = 0;
	}

	csflags = CS_LOCK_MASK;
	if (flags & CX_F_INTERRUPTOK)
	{
	    csflags |= CS_INTERRUPT_MASK;
	}
	if (timeout)
	{
	    csflags |= CS_TIMEOUT_MASK;
	}

	suspstatus = CSsuspend( csflags, timeout, (PTR)&preq->cx_key );
	if ( suspstatus || 
	     ( ( CX_Proc_CB.cx_flags & CX_PF_DO_DEADLOCK_CHK ) &&
	       ( E_CL2C21_CX_E_DLM_DEADLOCK == preq->cx_status ) ) )
	{ 
	    /*
	    ** Timed out or interrupted or deadlocked (if performing 
	    ** own deadlock checking).  Cancel queued request.
	    */
	    if ( suspstatus &&
	         ( CX_Proc_CB.cx_flags & CX_PF_DO_DEADLOCK_CHK ) )
	    {
		/*
		** Atomically set posted indicator to avoid potential
		** race condition where deadlock processing thread,
		** posts resume after we've woken up due to an interrupt,
		** or time out.
		*/
		if ( !CXdlm_mark_as_posted( preq ) )
		{
		    /*
		    ** Hit race condition with the deadlock thread. 
		    ** No need to do anything here since call to
		    ** CScancelled will eat the extra resume.
		    */
		    ;
		}
	    }
	    status = CXdlm_cancel( cancelflags, preq );
	    if ( OK == status )
	    {
		if ( E_CS0009_TIMEOUT == suspstatus )
		{
		    cmplstatus = E_CL2C0A_CX_W_TIMEOUT;
		}
		else if ( E_CS0008_INTERRUPTED == suspstatus )
		{
		    cmplstatus = E_CL2C0B_CX_W_INTR;
		}
		else if ( E_CL2C22_CX_E_DLM_CANCEL == preq->cx_status )
		{
		    cmplstatus = E_CL2C21_CX_E_DLM_DEADLOCK;
		}
		else
		{
		    cmplstatus = preq->cx_status;
		}
	    }
	    else if ( E_CL2C09_CX_W_CANT_CAN == status )
	    {
		preq->cx_old_mode = preq->cx_new_mode;
		cmplstatus = preq->cx_status;
                if ( 0 == cmplstatus )
                {
                    /*
                    ** AST informing us that lock has been granted has
                    ** not yet arrived.  Suspend once more.
                    */
                    status = CSsuspend( CS_LOCK_MASK, 0l, (PTR)&preq->cx_key );
		    cmplstatus = preq->cx_status;
                }

		if (suspstatus == E_CS0008_INTERRUPTED)
		{
		    /* Interrupt was received as lock/conv granted. */
		    if ( E_CL2C08_CX_W_SUCC_IVB == cmplstatus )
		    {
			/* Interrupted as lock with invalid value granted */
			cmplstatus = E_CL2C0C_CX_W_INTR_IVB;
		    }
		    else
		    {
			/* Received interrupt as lock was granted. */
			cmplstatus = E_CL2C04_CX_I_OKINTR;
		    }
		}
		else if (cmplstatus == E_CL2C21_CX_E_DLM_DEADLOCK)
		    cmplstatus = OK;
	    }
	    else if ( status == E_CL2C25_CX_E_DLM_NOTHELD
		      && preq->cx_inproxy )
	    {
		/*
		** Ignore "no such lock" errors - if the request has
		** yet to be enqueued by the GLC, the DLM lockid
		** is probably nonexistent.
		*/
		cmplstatus = OK;
	    }
	    else
	    {
		/* Report gross failure in cancel */
		cmplstatus = status;
	    }
	    CScancelled( (PTR)0 );
	}
	else
	{
	    /* Normal resume from completion routine. */
	    cmplstatus = preq->cx_status;

	    /* Make sure we really have the lock */
	    VERIFY_LOCK(preq, cmplstatus, flags);
	}

#ifdef CX_PARANOIA
	if ( 0 == cmplstatus )
	{
	    /* Thread resumed, but asynchronous completion not called!? */
	    CX_DLM_TRACE_TERSE("WAIT error!",preq);
	    cmplstatus = E_CL2C1A_CX_E_CORRUPT;
	}
#endif /* CX_PARANOIA */

	if ( preq->cx_inproxy )
	{
	    if ( preq->cx_lki_flags & (CX_LKI_F_NOTIFY|CX_LKI_F_NAB) )
	    {
		(*preq->cx_user_func)( preq, preq->cx_status );
	    }
	}

	preq->cx_lki_flags &= ~CX_LKI_F_QUEUED;
	preq->cx_inproxy = FALSE;
	CX_DLM_TRACE("WAIT+",preq);
    }
    else
    {
	/* Request was processed synchronously, no resume is expected */
	cmplstatus = preq->cx_status;

	/* Make sure we really have the lock */
	VERIFY_LOCK(preq, cmplstatus, flags);
    }

    if ( (preq->cx_lki_flags & CX_LKI_F_LSN_IN_LVB) &&
	  preq->cx_new_mode >= LK_SIX )
    {
	if ( OK != cx_dlm_sync_lsn_from_dlm( preq ) ) 
	    status = E_CL2C1C_CX_E_LSN_LVB_FAILURE;
    }

    if ( CX_OK_STATUS( cmplstatus ) && cmplstatus != E_CL2C04_CX_I_OKINTR )
    {
	status = OK;
    }
    else
    {
	status = cmplstatus;
    }
    return status;
} /* CXdlm_wait */

static void
cxdlm_getblki_ast(CS_SID *sid)
{
    CSresume(*sid);
}


/*{
** Name: CXdlm_get_blki	- Get info about locks on a resource.
**
** Description:
**
**	The primary use of this routine is in distributed deadlock
**	detection.   Underlaying DLM is queried for information
**	about the locks on a resource, and depending on the flags,
**	information for all locks is returned (CX_F_NODEADLOCK set),
**	information about all blocking locks on other nodes (return
**	value equals OK, and CX_F_NODEADLOCK clear), or a truncated
**	subset of the blocking locks on other nodes (CX_F_CVT_CVT set
**	and a convert-convert deadlock was detected giving return
**	value E_CL2C21_CX_E_DLM_DEADLOCK).  In this last case last
**	entry in the partial array of blocking locks returned will
**	be the lock causing the cvt-cvt deadlock.
**
**	Routine will attempt to allocate a larger buffer if buffer
**	passed in is inadequate, or NULL.  It is the callers
**	responsibility to free the larger buffer returned.
**
** Inputs:
**
**	flags 		CX_F_CVT_CVT - check for cvt-cvt deadlock.
**			CX_F_NODEADLOCK - Return all info, don't
**			 perform a deadlock check or filter returned
**			 info.   These flags are mutually exclusive.
**		
**	preq		Pointer to CX_REQ_CB.   
**		
**	    - cx_lock_id	Used to determine which resource to 
**				query (at least for VMS).
**
**	    - cx_old_mode	Used in cvt-cvt check.
**
**	    - cx_new_mode	Used in cvt-cvt check.
**
**	    - cx_status		Status in block is left unchanged.
**
**	    - cx_owner		Ignored.
**
**	    - cx_key		May be used for resource look up
**				in future implementations.
**
**	    - cx_user_func 	Ignored.
**
**	    - cx_user_extra	Ignored.
**    	
**	    - cx_value		Ignored.
**
**	which_buf	Which of two buffers to use.
**
**	blkibuf		Pointer to passed CX_BLKI buffer, or NULL.
**
**	pblkimax	Pointer to i4 holding max number of CX_BLKI
**			entries that can fit in blkibuf.
**
**	*plkmode	Max convert mode thus far of preq.
**
**	pplkibuf	Address of pointer to update with buffer used
**			to satisfy request.  May differ from blkibuf
**			if more than blkimax records needed to be
**			returned.
**
**	pblkicnt	Address of i4 updated with actual number
**			of records returned.
**
** Outputs:
**
**	*plkmode	Updated with max convert mode of all cluster 
**			locks ahead of and including preq.
**
**	*pblkimax	updated with number of records *pplkibuf can hold.
**
**	*pplkibuf	updated with address of buffer used. buffer will
**			hold *pnblkicnt CX_BLKI entries.
**
**	*pblkicnt	updated with actual number of records returned.
**
**	Returns:
**		OK			- Normal successful completion.
**		E_CL2C10_CX_E_NOSUPPORT - CX DLM not supported this OS.
**		E_CL2C11_CX_E_BADPARAM	- Invalid argument passed.
**		E_CL2C12_CX_E_INSMEM	- Could not allocate a large enough
**					  output buffer.
**		E_CL2C21_CX_E_DLM_DEADLOCK - cvt-cvt deadlock detected.
**					  Filling buffer is short-circuited
**					  as a deadlock is already found.
**		E_CL2C25_CX_E_DLM_NOTHELD - Invalid lock ID, or other
**			                  failure in retrieving info.
**		E_CL2C3x_CX_E_OS_xxxxxx	- Misc implementation dependent
**		  failures at the OS DLM level.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    Memory may be allocated.
**
** History:
**	11-may-2005 (devjo01)
**	    Created loosely based on do_getlki from dmfcsp.c.
**	12-Feb-2007 (jonj)
**	    VMS occasionaly does not set requested mode to granted mode
**	    for granted locks.
**	30-May-2007 (jonj)
**	    Do -not- seed *plkmode with request mode of caller's lock,
**	    rather build on what's passed.
**	13-jul-2007 (joea)
**	    Use async call to get lock info and separate buffers.
**	06-Sep-2007 (jonj)
**	    Do -not- seed *plkmode with request mode of caller's lock,
**	    rather build on what's passed.
**	11-Jan-2008 (jonj)
**	    If caller's lock-of-request is GRanted, extract all and
**	    only granted locks. This keeps interesting grants -behind-
**	    the lock-of-request in play.
*/
STATUS
CXdlm_get_blki( u_i4 flags, CX_REQ_CB *preq, i4 which_buf,
                CX_BLKI *blkibuf, u_i4 *pblkimax, i4 *plkmode,
	        CX_BLKI **ppblkibuf, u_i4 *pblkicnt )
{
    STATUS	 status = OK;
    i4		 outcount;
    CX_BLKI	*pcxblki;

#ifdef	CX_PARANOIA
    if ( ( flags != 0 && flags != CX_F_NODEADLOCK && flags != CX_F_CVT_CVT ) ||
         (NULL == pblkimax ) || ( NULL == pplkibuf ) || ( NULL == pnblkicnt ) )
	return E_CL2C11_CX_E_BADPARAM;
#endif /*CX_PARANOIA*/

    if ( !blkibuf ) *pblkimax = 0;

    *ppblkibuf = pcxblki = blkibuf;
    outcount = 0;

    /*
    **	Start OS dependent declarations.
    */

#if !defined(conf_CLUSTER_BUILD)
    status = E_CL2C10_CX_E_NOSUPPORT;
#elif defined(axp_osf)
    /*
    **	Tru64 implementation
    */
    status = E_CL2C10_CX_E_NOSUPPORT;
#elif defined(LNX)
    /*
    **	OpenDLM implementation
    */
    status = E_CL2C10_CX_E_NOSUPPORT;
#elif defined(VMS) /* Start _VMS_ */
    /*
    **	OpenVMS implementation
    */
    {
#   define	LKBI_ALLOC_INC	64
    int		vmsstat;
    struct {
	u_i4        length:16;
	u_i4        width:15;
	u_i4        overflow:1;
    } local_siz;
    i4  cvt, grant, wait, count, totallki, idx, node;
    i1	grmode, rqmode, queue;
    ILE3  lki_item_list [] =
    {
        { sizeof cvt, LKI$_CVTCOUNT, &cvt, NULL },
        { sizeof grant, LKI$_GRANTCOUNT, &grant, NULL },
        { sizeof wait, LKI$_WAITCOUNT, &wait, NULL },
        { 0, LKI$_LOCKS, NULL, (unsigned short *)&local_siz },
        { 0, 0 }
    };
    LKIDEF	*plki;
    bool	OnlyGrants;

    /*
    ** Note, we don't use the LKI$_BLOCKING code, since Ingres
    ** passes LCK$M_NODLCKBLK when asking the system for a
    ** lock, which explicitly excludes our locks as blockers.
    ** Locks are returned in order of granted (LIFO), converting
    ** (FIFO), and wait (FIFO).  (Small test program used to
    ** confirm this.
    */
    totallki = LKBI_ALLOC_INC; /* trick to force initial allocation */

    while (1) /* something to break out of */
    {
	IOSB iosb;
	CS_SID my_sid;

	if (totallki * sizeof(LKIDEF) > CX_Proc_CB.cxvms_bli_size[which_buf])
	{
	    /* Need to expand/allocate lki buffer */
	    if (CX_Proc_CB.cxvms_bli_buf[which_buf])
		/* Free old buffer */
		MEfree(CX_Proc_CB.cxvms_bli_buf[which_buf]);

	    /* Allocate expanded/initial buffer */
	    CX_Proc_CB.cxvms_bli_buf[which_buf] = MEreqmem(0, 
		    totallki * sizeof(LKIDEF), 0, NULL);
	    if (!CX_Proc_CB.cxvms_bli_buf[which_buf])
	    {
		status = E_CL2C12_CX_E_INSMEM;
		break;
	    }
	    CX_Proc_CB.cxvms_bli_size[which_buf] = totallki * sizeof(LKIDEF);
	}

	/* Get info from VMS DLM */
	lki_item_list[3].ile3$w_length = CX_Proc_CB.cxvms_bli_size[which_buf];
	lki_item_list[3].ile3$ps_bufaddr = CX_Proc_CB.cxvms_bli_buf[which_buf];

	CSget_sid(&my_sid);
	vmsstat = sys$getlki(0, &preq->cx_lock_id, lki_item_list, &iosb,
			     cxdlm_getblki_ast, &my_sid, 0);
	if (vmsstat != SS$_NORMAL)
	    break;

        CSsuspend(0, 0, 0);
	vmsstat = iosb.iosb$w_status;
	if (vmsstat != SS$_NORMAL)
	    break;

	totallki = grant + cvt + wait;

	/* Check for overflow */
	if (local_siz.overflow)
	    /* Recalculate requirements, & reallocate buffer */
	    continue;

	if (local_siz.width != sizeof(LKIDEF))
	{
	    /* VERY unexpected */
	    status = E_CL2C38_CX_E_OS_MISC_ERR;
	    break;
	}

	/*
	** Recalc totallki, JIC.  It might be paranoia, but
	** I'm not 100% sure the cvt, grant, and wait counts
	** are atomically set along with local_siz.
	*/
	totallki = local_siz.length / sizeof(LKIDEF);

	/*
	** Calculate max convert mode to use for determining who is
	** blocking us. This may be greater than the mode
	** we've requested because grants, converts and/or waiters
	** ahead of us may be asking for a higher mode, and
	** lock being tested cannot be granted until all those
	** ahead of it are granted.
	**
	** Compute the "max request mode" of all locks ahead of this lock,
	** using grant mode for grants, request mode for converts/waiters.
	*/

	/* Presume caller has seeded plkmode */

	OnlyGrants = FALSE;

	for (count = 0, plki = (LKIDEF *)CX_Proc_CB.cxvms_bli_buf[which_buf];
	     count < totallki; plki++, count++)
	{
	    /*
	    ** Terminate the loop if this is the lock we started out
	    ** with and it's not on the grant queue; if this blki
	    ** call was made on a granted lock, extract all granted
	    ** locks, even those after the lock-of-request.
	    */
	    if ( plki->lki$l_lkid == preq->cx_lock_id )
	    {
		if ( plki->lki$b_queue == LKI$C_GRANTED &&
		     !(flags & CX_F_NODEADLOCK) )
		{
		    OnlyGrants = TRUE;
		}
		else
		{
		    /*
		    ** This is the lock we started out with; we can end the
		    ** search right here: we only care about locks queued ahead
		    ** of us and ourselves, not behind us.
		    */
		    break;
		}
	    }

	    /* Include grants, converters and waiters */

	    /* First, convert from DLM to LK mode */
	    if ( plki->lki$b_queue == LKI$C_GRANTED )
		rqmode = cxvms_dlm_to_lk_mode[plki->lki$b_grmode];
	    else if ( !OnlyGrants )
		rqmode = cxvms_dlm_to_lk_mode[plki->lki$b_rqmode];
	    else
	    {
		/* ...then we've finished with the grant queue */
		break;
	    }
		
	    /* Then from request mode to convert mode */
	    rqmode = cx_ingres_lk_convert[*plkmode][rqmode];

	    /* Stow away max convert mode */
	    if ( rqmode > *plkmode )
		*plkmode = rqmode;
	}

	if ( flags & CX_F_NODEADLOCK )
	    /* Return all if not doing deadlock processing. */
	    count = totallki;

	for (plki = (LKIDEF *)CX_Proc_CB.cxvms_bli_buf[which_buf];
	     count > 0; plki++, count--)
	{
	    grmode = cxvms_dlm_to_lk_mode[plki->lki$b_grmode];
	    rqmode = cxvms_dlm_to_lk_mode[plki->lki$b_rqmode];
	    switch ( plki->lki$b_queue )
	    {
	    case LKI$C_GRANTED:
		queue = CX_BLKI_GRANTQ;
                rqmode = grmode;
		break;
	    case LKI$C_CONVERT:
		queue = CX_BLKI_CONVQ;
		break;
	    default:
		queue = CX_BLKI_WAITQ;
	    }
	    if (!(flags & CX_F_NODEADLOCK))
	    {
		/* Filter out those LKI records that are not blocking us */

		/*
		** LKI returns records for grant, convert, and
		** wait in order.  Once we reach a waiting LKI
		** there are no more blocking LKIs of interest,
		** since unless there is a converter or a granted
		** holder of the contested resource on a node,
		** there is no need to visit that node.  Expressed
		** another way, although waiters ahead of us
		** have significance in calculating the effective
		** lock mode for blocking calculations, a waiter
		** by definition has no hold on the resource and
		** by itself is not blocking the current request.
		*/
		    
		/*
		** Correction: We need to return waiters to caller,
		** Since lock that originated deadlock search may
		** be the only lock touching this resource on it's
		** node and if we filter waiters out, we may not
		** complete the deadlock cycle.
		*/

		/* Exclude those already checked locally */
		if ( plki->lki$l_csid == CX_Proc_CB.cx_ncb->cxvms_csids[0] )
		{
		    /* Blockers on our own node are checked in LK */
		    continue;
		}

		/*  
		** If this lock's grant/request mode is incompatible with the
		** max request mode of the waiters, then this lock request is
		** blocking the resource and may be causing deadlock.
		**
		** Note that we have to check the request mode, not grant mode,
		** of remote converters ahead of this lock. While the grant mode
		** may be compatible, the request mode may not, and when the
		** convert is granted to the new mode, it'll still be blocking
		** this lock and may be part of a deadlock cycle.
		*/

		/*
		** Check for Cvt - Cvt deadlock with locks on
		** other nodes.
		**
		** Done here to short circuit preparation
		** of blocking locks list.
		*/
		if ( queue == CX_BLKI_CONVQ && flags & CX_F_CVT_CVT &&
		     !(cx_ingres_lk_compat[preq->cx_old_mode] & (1 << rqmode)) )
		{
		    /*
 		    ** Remote lock will never get granted because of
 		    ** the lock strength held by the current lock,
 		    ** and current lock cannot be granted because
 		    ** of the lock strength held by this foreign
 		    ** blocker.  This is a convert-convert deadlock,
 		    ** and the current lock loses.
		    */
		    status = E_CL2C21_CX_E_DLM_DEADLOCK;

		    /* Fall through to add to output buffer, then break... */
		}
		else
		{
		    /* Exclude grants with compatible grant mode */
		    if ( queue == CX_BLKI_GRANTQ &&
			     (cx_ingres_lk_compat[grmode] & (1 << *plkmode)))
		    {
			/* Compatible granted lock */
			continue;
		    }
		    /* Exclude converts with compatible request mode */
		    else if ( queue == CX_BLKI_CONVQ &&
			     (cx_ingres_lk_compat[rqmode] & (1 << *plkmode)))
		    {
			/* Compatible converting lock */
			continue;
		    }
		    /* Note all waiters are included, compatible or not... */
		}
	    }

	    /* Add to output buffer */
	    if (outcount >= *pblkimax)
	    {
		/*
		** Expand buffer by greater of remaining totallki or
		** LKBI_ALLOC_INC
		*/
		*pblkimax += totallki > LKBI_ALLOC_INC ? totallki
						       : LKBI_ALLOC_INC;
		pcxblki = MEreqmem(0, sizeof(CX_BLKI) * *pblkimax, 0, NULL);
		if (pcxblki)
		{
		    if (outcount > 0)
		    {
			MEcopy(blkibuf, outcount * sizeof(CX_BLKI), pcxblki);
			/*
			** Caller needs to decide if he will free
			** new or old buffer.  We can't do it here
			** as original buffer may be a declared array.
			*/
		    }
		    *ppblkibuf = pcxblki;
		    pcxblki = pcxblki + outcount;
		}
		else
		{
		    /* Cripes! couldn't expand buffer. */
		    *pblkimax = outcount;
		    status = E_CL2C38_CX_E_OS_MISC_ERR;
		    break;
		}
	    }

	    idx = CX_Proc_CB.cx_ni->cx_node_cnt;
	    while (idx > 0)
	    {
		node = CX_Proc_CB.cx_ni->cx_xref[idx] + 1;
		if (plki->lki$l_csid == CX_Proc_CB.cx_ncb->cxvms_csids[node])
		{ 
		    pcxblki->cx_blki_csid = node;
		    break;
		}
		--idx;
	    }
	    pcxblki->cx_blki_pid = plki->lki$l_pid;
	    pcxblki->cx_blki_trans.lk_uhigh = 0;
	    pcxblki->cx_blki_trans.lk_ulow = plki->lki$l_pid;
	    CX_EXTRACT_LOCK_ID( &pcxblki->cx_blki_lkid, &plki->lki$l_lkid );
	    pcxblki->cx_blki_rqmode = rqmode;
	    pcxblki->cx_blki_gtmode = grmode;
	    pcxblki->cx_blki_queue = queue;
	    pcxblki->cx_blki_spare1b = 0;
	    outcount++;

	    /* if remote cvt-cvt deadlock found, make this the last entry */
	    if ( flags & CX_F_CVT_CVT && status == E_CL2C21_CX_E_DLM_DEADLOCK )
		break;
	    
	    pcxblki++;
	} /* endfor */

	break; /* Exit while */
    } /* endwhile */

    if ( OK == status )
    {
	status = cx_translate_status( (i4)vmsstat );
    }

    } /* end _VMS_ */
#else
    /*
    **	All other platforms
    */
    status = E_CL2C10_CX_E_NOSUPPORT;
#endif /*OS SPECIFIC*/
    /*
    **	End OS dependent declarations.
    */

    *pblkicnt = outcount;

    CX_DLM_TRACE_STATUS("BLI", preq, status);

    return status;
} /* CXdlm_get_blki */



/*{
** Name: CXdlm_mark_as_posted	- Atomically mark CX_REQ_CB as posted.
**
** Description:
**
**	The sole use of this routine is in distributed deadlock
**	processing.  If we're not using the DLM to handle deadlock,
**	there exists a race condition between the firing of the
**	completion handler, the resume needed if Ingres selects
**	this request as the deadlock victim, and session resumption
**	due to time-out or interrupt.
**
**	An atomic operation is performed to mark the request as being
**	posted.  If this fails the completion routine has already fired,
**	and the deadlock detector must not resume the lock owner.
**
** Inputs:
**
**	preq		Pointer to CX_REQ_CB.   
**		
**	    - cx_status		Ignored.
**
**	    - cx_owner		Ignored.
**
**	    - cx_key		Ignored.
**
**	    - cx_user_func 	Ignored.
**
**	    - cx_user_extra	Ignored.
**    	
**	    - cx_dlm_ws.cx_dlm_posted	Set.
**
**	    - cx_value		Ignored.
**
** Outputs:
**	    none
**		
**	Returns:
**		TRUE	- cx_dlm_posted set by this caller.
**		FALSE	- Not VMS, or CX_REQ_CB already posted.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    None.
**
** History:
**	20-may-2005 (devjo01)
**	    Created.
**	04-Apr-2008 (jonj)
**	    CS_ISSET is not atomic, just do the CS_TAS.
*/
bool
CXdlm_mark_as_posted( CX_REQ_CB *preq )
{
# if defined(VMS) /* start _VMS_ */
    return( CS_TAS( &preq->cx_dlm_ws.cx_dlm_posted ) );
# endif /* end _VMS_ */
    return(FALSE);
} /* CXdlm_mark_as_posted */


/*{
** Name: CXdlm_glc_support	- Body of GLC support thread.
**
** Description:
**
**	This routine forms the body of the group lock container
**	support thread.  It is only needed for those implementations
**	whose DLM's do not support native GLCs.
**
** Inputs:
**
**	none
**
** Outputs:
**	    none
**		
**	Returns:
**		OK	- Normal shutdown completion.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    Routine will not return until server shutdown.
**
** History:
**	21-jun-2005 (devjo01)
**	    Created.
**	04-Apr-2008 (jonj)
**	    Changed wakeup/sleep mechanism to use
**	    CSadjust_counter(cx_glc_triggered) instead
**	    of buggy CS_TAS(cx_glc_ready) which sometimes
**	    resulted in eternal sleep and/or double
**	    resumes of GLC thread.
**	    Extract what's needed from msg for DLM call,
**	    release msg back to free list before doing
**	    DLM.
**	09-May-2008 (jonj)
**	    Check for stale requests and ignore them. The
**	    GLC thread may lag what's going on elsewhere
**	    in the node.
*/
STATUS
CXdlm_glc_support( void )
{
    STATUS	status = OK;

# if defined(VMS) /* start _VMS_ */
    STATUS	lstatus;
    CX_GLC_CTX *glc;
    CX_GLC_MSG *in_msg;
    CX_REQ_CB  *preq;
    CX_OFFSET  	local_list, prevoff, nextoff;
    u_i4	flags;
    CS_CPID	LKcpid;
    i4		posts;

    glc = cx_find_glc( CX_Proc_CB.cx_pid, 0 );
    if ( NULL == glc )
    {
	/* Called before CXinitialize called? */
	status = E_CL2C1B_CX_E_BADCONTEXT;
    }
    else
    {
	CSget_cpid( &glc->cx_glc_threadid );
	glc->cx_glc_triggered = 1;

	TRdisplay("%@ Starting GLC thread %x,%x glc %p\n",
			glc->cx_glc_threadid.pid,
			glc->cx_glc_threadid.sid,
			glc);

	/* Main loop whirls here */
	while ( status == OK )
	{
	    /* Watch for new msgs while we process this batch */
	    posts = glc->cx_glc_triggered;

	    /* Unclip queue of pending messages, if any */
	    do 
	    {
		local_list = glc->cx_glc_glms;
	    } while ( CScas4( &glc->cx_glc_glms, local_list, 0 ) );

	    if ( !local_list )
	    {
		/* If no new posts, go to sleep */
		if ( (CSadjust_counter( &glc->cx_glc_triggered, -posts )) == 0 )
		    status = CSsuspend( CS_INTERRUPT_MASK, 0, 0 );

		continue;
	    }

	    /*
	    ** Reverse list. We can now operate on this list without
	    ** protection since it is unhooked from the input queue.
	    */
	    prevoff = 0;
	    for( ;; )
	    {
		in_msg = (CX_GLC_MSG*)CX_OFFSET_TO_PTR(local_list);
		nextoff = in_msg->cx_glm_next;
		in_msg->cx_glm_next = prevoff;
		if ( nextoff == 0 )
		    break;
		prevoff = local_list;
		local_list = nextoff;
	    }
	    nextoff = local_list;

	    while ( nextoff )
	    {
		in_msg = (CX_GLC_MSG *)CX_OFFSET_TO_PTR( nextoff );
		nextoff = in_msg->cx_glm_next;

		preq = (CX_REQ_CB *)CX_OFFSET_TO_PTR( in_msg->cx_glm_req_off );
		if ( preq->cx_owner.pid != CX_Proc_CB.cx_pid )
		{
		    /* Wrong/corrupt struct? */
		    TRdisplay( "%@ %s:%d Bad preq, pid = %x\n", __FILE__,
		     __LINE__, preq->cx_owner.pid );
		    cx_vms_free_glm( glc, &in_msg );
		}
		else if ( preq->cx_instance != in_msg->cx_glm_instance )
		{
		    /*
		    ** If the CX_REQ_CB has been reinstantiated since this
		    ** CX_GLC_MSG was formed, then ignore it. LK may have
		    ** deadlocked, interrupted, or timed out the request
		    ** by the time this GLC thread ever sees it.
		    */
		    TRdisplay( "%@ GLC %d %x,%x stale request cx_instance %d glm_instance %d\n",
				 __LINE__, 
				 preq->cx_owner.pid,
				 preq->cx_owner.sid,
				 preq->cx_instance,
				 in_msg->cx_glm_instance);
		    cx_vms_free_glm( glc, &in_msg );
		}
		else
		{
		    switch ( in_msg->cx_glm_opcode )
		    {
		    case CX_DLM_OP_RQST:
			/* Suck out what we need from msg, then discard it */
			flags = in_msg->cx_glm_flags;
			LKcpid = in_msg->cx_glm_caller;
			cx_vms_free_glm( glc, &in_msg );

			lstatus = CXdlm_request( flags & ~CX_F_WAIT, preq, NULL );

			if ( OK != lstatus )
			{
			    /* Make sure we have the lock */
			    VERIFY_LOCK(preq, lstatus, flags);

			    /* Synchronous completion */
			    CScp_resume( &LKcpid );
			}
			break;

		    case CX_DLM_OP_CONV:
			/* Suck out what we need from msg, then discard it */
			flags = in_msg->cx_glm_flags;
			LKcpid = in_msg->cx_glm_caller;
			cx_vms_free_glm( glc, &in_msg );

			lstatus = CXdlm_convert( flags & ~CX_F_WAIT, preq );
			
			if ( OK != lstatus )
			{
			    /* Make sure we have the lock */
			    VERIFY_LOCK(preq, lstatus, flags);

			    /* Synchronous completion */
			    CScp_resume( &LKcpid );
			}
			break;

		    case CX_DLM_OP_RLSE:
			/* Suck out what we need from msg, then discard it */
			flags = in_msg->cx_glm_flags;
			LKcpid = in_msg->cx_glm_caller;
			cx_vms_free_glm( glc, &in_msg );

			lstatus = CXdlm_release( flags, preq );
			CScp_resume( &LKcpid );
			break;

		    case CX_DLM_OP_CANC:
			flags = in_msg->cx_glm_flags;
			lstatus = CXdlm_cancel( flags, preq );

			/*
			** Kludge.  Send back lstatus in glm_opcode.
			** we can't use cx_status as async completion
			** routine uses this to set its status.  We
			** could add a field to GLM, but it is only
			** needed in this case.  It is the responsibility
			** of the caller to free the GLM.
			*/
			in_msg->cx_glm_opcode = lstatus;
			CScp_resume( &in_msg->cx_glm_caller );
			break;

		    default:
			/*
			** Wrong/corrupt struct?  We abandon this suspect
			** GLM, as we're not sure what we can trust.
			** Should not hit this code in any case.
			*/
			TRdisplay( "%@ %s:%d Bad opcode = %d from "
			           "session 0x%x:%x\n", __FILE__,
				 __LINE__, in_msg->cx_glm_opcode,
				 in_msg->cx_glm_caller.pid,
				 in_msg->cx_glm_caller.sid );
			cx_vms_free_glm( glc, &in_msg );
		    }

# if defined(CX_DLM_ENABLE_TRACE)
		    TRdisplay( "%@ GLC OP=%d %d:%d:%d:%d:%d, status=%x\n", 
		     in_msg->cx_glm_opcode, preq->cx_key.lk_type,
		     preq->cx_key.lk_key1, preq->cx_key.lk_key2,
		     preq->cx_key.lk_key3, preq->cx_key.lk_key4,
		     lstatus );
# endif
		}
	    } /* while nextoff */
	} /* for(ever) */

	TRdisplay("%@ Terminating GLC thread %x,%x glc %p\n",
			glc->cx_glc_threadid.pid,
			glc->cx_glc_threadid.sid,
			glc);
    }
# else
    status = CSsuspend( CS_INTERRUPT_MASK, 0, 0 );
# endif /* end _VMS_ */

    if ( status == E_CS0008_INTERRUPTED )
	status = OK;
    return status;
} /* CXdlm_glc_support */



/*{
** Name: cx_find_glc	- find or allocate GLC anchor.
**
** Description:
**
**	This routine searches the CX_GLC_CTX array for a matching PID,
**	or if inserting a matching PID or an open space.
**
**	If inserting GLC is formatted.
**
** Inputs:
**
**	pid		- PID to to search for / allocate for.
**
**	insert		- If non-zero call is for insert.
**
** Outputs:
**	    none
**		
**	Returns:
**		ptr to GLC	- Normal shutdown completion.
**		NULL		- Can't find or no space.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    None.
**
** History:
**	21-jul-2005 (devjo01)
**	    Created.
**	23-Feb-2007 (jonj)
**	    Check that searched-for PID is still alive,
**	    cleanup its GLC if not, return NULL.
*/
CX_GLC_CTX *
cx_find_glc( PID pid, i4 insert )
{
# if defined(VMS) /* start _VMS_ */
    PID		spid;
    i4		i;
    CX_OFFSET	nextoff;
    CX_GLC_CTX *glc;
    CX_GLC_MSG *glm;


    if ( pid == CX_Proc_CB.cx_ncb->cx_csp_pid )
    {
	return &CX_Proc_CB.cx_ncb->cxvms_csp_glc;
    }

    spid = insert ? 0 : pid;
    do
    {
	for ( i = 0, glc = CX_Proc_CB.cx_ncb->cxvms_glcs;
	      i < CX_Proc_CB.cx_ncb->cxvms_glc_hwm;
	      i++, glc++ )
	{
	    if ( pid == glc->cx_glc_threadid.pid ||
	         spid == glc->cx_glc_threadid.pid )
	    {
		/*
		** If PID is dead, cleanup its GLC for future reuse,
		** return NULL.
		**
		** This happens if a server is killed without going
		** through its rundown, and we may be called here to release
		** locks via the dead process detector rundown and the
		** lock's GLC process is that of the dead server.
		** If we don't notice that the GLC's process is dead,
		** we'll return and suffer a CSsuspend to wait forever on the
		** GLC Support Thread (cx_glc_threadid.pid) in the 
		** now-dead server. Instead, we cleanup the GLC and
		** return NULL, posting a NOTHELD notification
		** to the lock requestor.
		*/
		if ( pid && !PCis_alive(pid) )
		{
		    /* Note that we noticed */
		    TRdisplay("%@ cx_find_glc: GLC owner pid %x is dead\n", pid);

		    /* Trash PID, so this GLC can no longer be found. */
		    glc->cx_glc_threadid.pid = 0;

		    /* Return GLMs, if any, to shared pool */
		    if ( nextoff = glc->cx_glc_glms )
		    {
			/* Find end of GLM list */
			while ( nextoff )
			{
			    glm = (CX_GLC_MSG *)CX_OFFSET_TO_PTR( nextoff );
			    nextoff = glm->cx_glm_next;
			}

			/* Return GLMs to shared pool */
			do
			{
			    glm->cx_glm_next = CX_Proc_CB.cx_ncb->cxvms_glm_free;
			} while ( CScas4( &CX_Proc_CB.cx_ncb->cxvms_glm_free,
				      glm->cx_glm_next, glc->cx_glc_glms ) );
			glc->cx_glc_glms = 0;
		    }

		    /* Return local cache of free GLMs to shared pool. */
		    if ( nextoff = glc->cx_glc_free )
		    {
			/* Find end of free GLM list */
			while ( nextoff )
			{
			    glm = (CX_GLC_MSG *)CX_OFFSET_TO_PTR( nextoff );
			    nextoff = glm->cx_glm_next;
			}

			/* Hook in entire list */
			do
			{
			    glm->cx_glm_next = CX_Proc_CB.cx_ncb->cxvms_glm_free;
			} while ( CScas4( &CX_Proc_CB.cx_ncb->cxvms_glm_free,
					  glm->cx_glm_next, glc->cx_glc_free ) );
			glc->cx_glc_free = 0;
			glc->cx_glc_fcnt = 0;
		    }

		    /* Mark GLC as free */
		    CS_ACLR( &glc->cx_glc_valid );
		    glc->cx_glc_triggered = 0;

		    /* Return NULL glc */
		    return NULL;
		}
		else if ( insert )
		{
		    if ( !CS_TAS( &glc->cx_glc_valid ) &&
		         glc->cx_glc_threadid.pid != pid )
		    {
			/*
			** Grabbed by other in alloc race condition,
			** and not an instance of us encountering
			** a PID laid down by another image running
			** in this terminal session. (On VMS, all
			** images except those explicitly detached
			** are run in the process context of the
			** controlling terminal or batch queue).
			** The glc is released in CXterminate, but
			** JIC process is stopped w/o rundown.
			*/
			continue;
		    }
		    /* So resume is not sent. */
		    glc->cx_glc_triggered = 1;

		    if ( glc->cx_glc_threadid.pid != pid )
		    {
			/* Clean init/re-init of GLC */
			glc->cx_glc_fcnt = 0;
			glc->cx_glc_free = 0;
			glc->cx_glc_glms = 0;
			glc->cx_glc_threadid.pid
			 = pid; /* Must init this last. */
		    }
		    else if ( glc->cx_glc_glms )
		    {
			/* Recycling GLC left in unclean state. */
			/* Find end of GLM list. */
			nextoff = glc->cx_glc_glms;
			while ( nextoff )
			{
			    glm = (CX_GLC_MSG *)CX_OFFSET_TO_PTR( nextoff );
			    nextoff = glm->cx_glm_next;
			}

			/*
			** Return GLMs to shared pool, and leave local
			** free GLMs and fcnt alone.
			*/
			do
			{
			    glm->cx_glm_next =
                              CX_Proc_CB.cx_ncb->cxvms_glm_free;
			} while ( CScas4( &CX_Proc_CB.cx_ncb->cxvms_glm_free,
                                  glm->cx_glm_next, glc->cx_glc_glms ) );
			glc->cx_glc_glms = 0;
		    }
		}
		else if ( !CS_ISSET( &glc->cx_glc_valid ) )
		{
		    continue;
		}
		return glc;
	    }
	}

	if ( insert )
	{
	    /* No space in array, extend array by one & try again. */
	    do
	    {
		if ( (i = CX_Proc_CB.cx_ncb->cxvms_glc_hwm) >= CX_MAX_SERVERS )
		{
		    /* No room at all, give up! */
		    insert = 0;
		    break;
		}
	    } while ( CScas4( &CX_Proc_CB.cx_ncb->cxvms_glc_hwm, i, i+1 ) );
	}
    } while ( insert );
# endif /* end _VMS_ */
    return NULL;
} /* cx_find_glc */


/*{
** Name: cx_dlm_sync_lsn_from_dlm - Encapsulate update LSN from LVB logic.
**
** Description:
**
**	If specified by the user, the DLM has the capability of helping
**	maintain an internal LSN in cooperation with CMO.  This routine
**	holds the logic used.  
**
**	The basic idea, is if the lock represents a potentially covering
**	lock for an update operation, the user can specify CX_F_LSN_IN_LVB
**	on the lock request, and CX will undertake to keep the Local LSN
**	in force at the time the lock was last released, or converted
**	downward in the unused portion of the LVB.  When lock is then 
**	taken in an update mode by another transaction, a cheap check can be 
**	made if the copy of the CX LSN local to the node is lagging.  If
**	so, it is bumped forward.  If the LVB is empty, or invalid, we 
**	pessimistically resync the local CX LSN to the global CX LSN.
**	
**	A failure here is potentially severe.
**
** Inputs:
**
**	preq		Pointer to CX_REQ_CB for lock to be checked.
**		
** Outputs:
**
**	none
**
**	Returns:
**		E_CL2C1C_CX_E_LSN_LVB_FAILURE - Lock Granted, but
**		  there was a failure in LSN propagation.  Caller
**		  MUST abort his transaction.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    Local CX LSN may be adjusted.
**
** History:
**	20-sep-2004 (devjo01)
**	    Created.
*/
static STATUS
cx_dlm_sync_lsn_from_dlm( CX_REQ_CB *preq )
{
    STATUS		 status;
    CX_LSN		*plsn = CX_LSN_IN_LVB_PTR(preq);
    char		 buf[128];

    if ( E_CL2C08_CX_W_SUCC_IVB == preq->cx_status ||
	 (plsn->cx_lsn_low == 0 && plsn->cx_lsn_high == 0) )
    {
	status = CXcmo_lsn_gsynch( plsn );
	CX_Stats.cx_cmo_lsn_lvbgsyncs++;
    }
    else
    {
	status = CXcmo_lsn_lsynch( plsn );
	CX_Stats.cx_cmo_lsn_lvblsyncs++;
    }
    if ( OK != status )
    {
	(*CX_Proc_CB.cx_lkkey_fmt_func)( &preq->cx_key, buf );
	(*CX_Proc_CB.cx_error_log_func)( status,
	    NULL, 4,
	    0, preq->cx_status,
	    0, preq->cx_old_mode,
	    0, preq->cx_new_mode,
	    0, buf );
	status = E_CL2C1C_CX_E_LSN_LVB_FAILURE;
    }
    return status;
} /* cx_dlm_sync_lsn_from_dlm */


/*{
** Name: CXdlm_is_blocking	- Check if lock is blocking other requests.
**
** Description:
**
**	This allows us to poll for blocking locks.   Currently ( and
**	fortunately ), this functionality is not really required, since
**	blocking notification appears reliable.  
**
** Inputs:
**
**	preq		Pointer to CX_REQ_CB for lock to be checked.
**		
** Outputs:
**
**	none
**
**	Returns:
**		TRUE		- lock is held in a mode incompatible
**				  with at least one request on the wait
**				  or convert queue for this resource.
**		FALSE		- mode held by this lock is compatible
**				  with any and all waiting requests.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	17-mar-2001 (devjo01)
**	    Created.
*/
bool
CXdlm_is_blocking( CX_REQ_CB *preq )
{
    bool		 isblocking = FALSE;

#ifdef	CX_NATIVE_GENERIC
    /*
    **	Native generic implementation
    */
#elif defined(axp_osf) && defined(conf_CLUSTER_BUILD)
    /*
    **	Tru64 implementation
    */
    {
    dlm_status_t dlmstat;	/* Tru64 dlm status code */
    dlm_lkinfo_t lkinfo;
    dlm_rsbinfo_t resinfo;

    dlmstat = DLM_GET_LKINFO( (dlm_lkid_t *)&preq->cx_lock_id, &lkinfo );
    
    if ( DLM_SUCCESS == dlmstat && GRANTED == lkinfo.lkstate )
    {
	dlmstat = DLM_GET_RSBINFO( CX_Proc_CB.cxaxp_nsp,
				   (uchar_t *)lkinfo.resnam,
				   lkinfo.resnlen, &resinfo, 
				   (dlm_lkid_t *)0 );
	if ( DLM_SUCCESS == dlmstat &&
	     ( ((1 << resinfo.cvtqmode) | (1 << resinfo.wtqmode)) &
	        cxaxp_incompatable_modes[lkinfo.grmode] ) )
	{
	    isblocking = TRUE;
	}
    }
    } /* end axp_osf data scope */
#else /*axp_osf (Tru64)*/
    /*
    **	All other platforms
    */
    isblocking = FALSE;
#endif /*CX_NATIVE_GENERIC*/
    /*
    **	End OS dependent declarations.
    */

    return isblocking;
} /* CXdlm_is_blocking */


/*{
** Name: CXdlm_lock_key_to_string - Default key formatter for trace/err msgs.
**
** Description:
**
**	Formats key value into buffer provided.
**
** Inputs:
**
**	plkkey	- Pointer to lock key to format.
**	pbuf	- Pointer to buffer to receive text.  Must be at
**		  least 128 bytes long.
**		
** Outputs:
**
**	none
**
**	Returns:
**		pbuf	- to allow routine to be used directly in printf, etc.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-Sep-2004 (devjo01)
**	    Initial version.
*/
char *
CXdlm_lock_key_to_string( LK_LOCK_KEY *plkkey, char *pbuf )
{
    STprintf( pbuf, "%d:%x.%x.%x.%x.%x.%x", plkkey->lk_type,
     plkkey->lk_key1, plkkey->lk_key2, plkkey->lk_key3, plkkey->lk_key4,
     plkkey->lk_key5, plkkey->lk_key6 );
    return pbuf;
} /* CXdlm_lock_key_to_string */


/*{
** Name: Miscellaneous debug routines.
**
**	    CXdlm_dump_lk	- Dump lock info
**	    CXdlm_dump_both	- Dump lock & associated resource
**
** Description:
**
**	These routines dump the lock description as known to the
**	underlying DLM to the trace log.
**
**	There are currently no direct calls to these routines in Ingres,
**	but they may be called by debuggers which support calling of
**	user routines, or included by enabling the CX_DLM_TRACE macros.
**
** Inputs:
**
**	See individual routines.
**
** Outputs:
**	none.
**
**	Returns:
**		OK			- Normal successful completion.
**		Possibly OS specific failure codes.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	09-mar-2001 (devjo01)
**	    Created.
*/

/*
**	Dump lock info by lock id
*/
STATUS
CXdlm_dump_lk( i4 (*output_fcn)(PTR, i4, char *), LK_UNIQUE *plkid )
{
    STATUS		 status;
    char		 buf[128];

    if ( NULL == output_fcn )
	output_fcn = TRwrite;

#if !defined(conf_CLUSTER_BUILD)
    status = E_CL2C10_CX_E_NOSUPPORT;
#elif defined(axp_osf)
    /*
    **	Tru64 implementation
    */
    {
    dlm_status_t dlmstat;	/* Tru64 dlm status code */
    dlm_lkinfo_t lkinfo;

    dlmstat = DLM_GET_LKINFO( (dlm_lkid_t *)plkid, &lkinfo );
    
    status = cx_translate_status( (i4)dlmstat );
    if ( OK == status )
    {
	(void)TRformat( output_fcn, (PTR)1, buf, sizeof(buf) - 1,
		   ERx("{ lkinfo for %08x.%08x"),
		   plkid->lk_uhigh, plkid->lk_ulow );
	(void)TRformat( output_fcn, (PTR)1, buf, sizeof(buf) - 1,
		   ERx("  resnam =%#.4{ %x%}"), lkinfo.resnlen / 4,
		   lkinfo.resnam );
	(void)TRformat( output_fcn, (PTR)1, buf, sizeof(buf) - 1,
		   ERx("  rqmode = DLM_%wMODE, valb @ %p"), 
		   cxaxp_lock_mode_text, lkinfo.rqmode, lkinfo.valb );
	(void)TRformat( output_fcn, (PTR)1, buf, sizeof(buf) - 1,
		   ERx("  grmode = DLM_%wMODE, lkstate = %w"),
		   cxaxp_lock_mode_text, lkinfo.grmode,
		   ERx("INTRANSIT,GRANTED,CONVERTING,WAITING"), 
		   lkinfo.lkstate );
	(void)TRformat( output_fcn, (PTR)1, buf, sizeof(buf) - 1,
		   ERx("  flags  = %v"),
	   ERx("b0,b1,b2,NOQUEUE,SYNCSTS,NODLCKWT,NODLCKBLK,EXPEDITE,")
	   ERx("QUECVT,DEQALL,INVVALBLK,b11,VALB,PERSIST,")
	   ERx("GROUP_LOCK,TXID_LOCAL,TXID_GLOBAL"), lkinfo.flags );
	(void)TRformat( output_fcn, (PTR)1, buf, sizeof(buf) - 1,
		   ERx("  PID    = %d, timeout = %d, sig = %d"),
		   lkinfo.lk_pid, lkinfo.timeout, lkinfo.sig );
	(void)TRformat( output_fcn, (PTR)1, buf, sizeof(buf) - 1,
		   ERx("  notprm @ %p, hint @ %p"),
		   lkinfo.notprm, lkinfo.hint );
	(void)TRformat( output_fcn, (PTR)1, buf, sizeof(buf) - 1,
		   ERx("  blkrtn @ %p, cplrtn @ %p"),
		   lkinfo.blkrtn, lkinfo.cplrtn );
	(void)TRformat( output_fcn, (PTR)1, buf, sizeof(buf) - 1, ERx("}") ); 
    }
    } /* end axp_osf data scope */
#else /*axp_osf (Tru64)*/
    /*
    **	All other platforms
    */
    status = E_CL2C10_CX_E_NOSUPPORT;
#endif /*CX_NATIVE_GENERIC*/
    /*
    **	End OS dependent declarations.
    */

    return status;
} /* CXdlm_dump_lk */


/*
**	Dump lock and associated resource info by lock id
*/
STATUS
CXdlm_dump_both( i4 (*output_fcn)(PTR, i4, char *), CX_REQ_CB *preq )
{
    STATUS		 status;
    char		 buf[128];

    if ( NULL == output_fcn )
	output_fcn = TRwrite;

#if !defined(conf_CLUSTER_BUILD)
    status = E_CL2C10_CX_E_NOSUPPORT;
#elif defined(axp_osf)
    /*
    **	Tru64 implementation
    */
    {
    dlm_status_t dlmstat;	/* Tru64 dlm status code */
    dlm_lkinfo_t lkinfo;
    dlm_rsbinfo_t resinfo;

    (void)TRformat( output_fcn, (PTR)1, buf, sizeof(buf) - 1,
	       ERx("{ lk & res info for %08x.%08x"),
	       preq->cx_lock_id.lk_uhigh, preq->cx_lock_id.lk_ulow );
    (void)TRformat( output_fcn, (PTR)1, buf, sizeof(buf) - 1,
	       ERx("  preq   @ %p, status = %x, iflags = %x"),
	       preq, preq->cx_status,  preq->cx_lki_flags );
    if ( CX_NONZERO_ID(&preq->cx_lock_id) )
    {
	dlmstat = DLM_GET_LKINFO( (dlm_lkid_t *)&preq->cx_lock_id, &lkinfo );
	
	status = cx_translate_status( (i4)dlmstat );
	if ( OK == status )
	{
	    (void)TRformat( output_fcn, (PTR)1, buf, sizeof(buf) - 1,
		       ERx("  resnam =%#.4{ %x%}"), lkinfo.resnlen / 4,
		       lkinfo.resnam );
	    (void)TRformat( output_fcn, (PTR)1, buf, sizeof(buf) - 1,
		       ERx("  rqmode = DLM_%wMODE, valb @ %p"), 
		       cxaxp_lock_mode_text, lkinfo.rqmode, lkinfo.valb );
	    (void)TRformat( output_fcn, (PTR)1, buf, sizeof(buf) - 1,
		       ERx("  grmode = DLM_%wMODE, lkstate = %w"),
		       cxaxp_lock_mode_text, lkinfo.grmode,
		       ERx("INTRANSIT,GRANTED,CONVERTING,WAITING"), 
		       lkinfo.lkstate );
	    (void)TRformat( output_fcn, (PTR)1, buf, sizeof(buf) - 1,
		       ERx("  flags  = %v"),
	       ERx("b0,b1,b2,NOQUEUE,SYNCSTS,NODLCKWT,NODLCKBLK,EXPEDITE,")
	       ERx("QUECVT,DEQALL,INVVALBLK,b11,VALB,PERSIST,")
	       ERx("GROUP_LOCK,TXID_LOCAL,TXID_GLOBAL"), lkinfo.flags );
	    (void)TRformat( output_fcn, (PTR)1, buf, sizeof(buf) - 1,
		       ERx("  PID    = %d, timeout = %d, sig = %d"),
		       lkinfo.lk_pid, lkinfo.timeout, lkinfo.sig );
	    (void)TRformat( output_fcn, (PTR)1, buf, sizeof(buf) - 1,
		       ERx("  notprm @ %p, hint @ %p"),
		       lkinfo.notprm, lkinfo.hint );
	    (void)TRformat( output_fcn, (PTR)1, buf, sizeof(buf) - 1,
		       ERx("  blkrtn @ %p, cplrtn @ %p"),
		       lkinfo.blkrtn, lkinfo.cplrtn );
	    dlmstat = DLM_GET_RSBINFO( CX_Proc_CB.cxaxp_nsp,
				       (uchar_t *)lkinfo.resnam,
				       lkinfo.resnlen, &resinfo, 
				       (dlm_lkid_t *)0 );
	    status = cx_translate_status( (i4)dlmstat );
	    if ( OK == status )
	    {
		(void)TRformat( output_fcn, (PTR)1, buf, sizeof(buf) - 1,
		    ERx("- vbstatus = %d, vbseqnum = %d, subrescnt = %d"),
		    resinfo.vbstatus, resinfo.vbseqnum, resinfo.subrescnt );
		(void)TRformat( output_fcn, (PTR)1, buf, sizeof(buf) - 1,
		    ERx("  lckcnt   = %d, blkastcnt= %d, ggcnt     = %d"),
		    resinfo.lckcnt, resinfo.blkastcnt, resinfo.ggcnt );
		(void)TRformat( output_fcn, (PTR)1, buf, sizeof(buf) - 1,
		    ERx("  ggmode = %w, cvtqmode = %w, wtqmode = %w"),
		    cxaxp_lock_mode_text, resinfo.ggmode,
		    cxaxp_lock_mode_text, resinfo.cvtqmode,
		    cxaxp_lock_mode_text, resinfo.wtqmode );
		(void)TRformat( output_fcn, (PTR)1, buf, sizeof(buf) - 1,
		    ERx("  valb =%4.4{ %x%}"), &resinfo.valb );
	    }
	    else
		(void)TRformat( output_fcn, (PTR)1, buf, sizeof(buf) - 1,
		    ERx("  !! Err fetching resinfo (status = %x)"),
		    status );
	}
	(void)TRformat( output_fcn, (PTR)1, buf, sizeof(buf) - 1, ERx("}") ); 
    }
    } /* end axp_osf data scope */
#else /*axp_osf (Tru64)*/
    /*
    **	All other platforms
    */
    status = E_CL2C10_CX_E_NOSUPPORT;
#endif /*CX_NATIVE_GENERIC*/
    /*
    **	End OS dependent declarations.
    */

    return status;
} /* CXdlm_dump_both */

#if defined(VMS) && defined(conf_CLUSTER_BUILD) && defined(VERIFY_VMS_LOCKS)

#define VMS_LOCK_MODE   "N,IS,IX,S,SIX,X"

/*
** Debugging aid for VMS wayward proxy locks.
**
** Called when we belive an lock has been granted/converted
** to verify with the DLM that it has; if not, report the error
** to the log, return FAIL.
**
** Useful for finding problems with Client->Proxy server
** and resultant CSnoresnow problems.
*/
static STATUS
CXdlm_verify_lock(
    CX_REQ_CB *preq,
    STATUS cstatus,
    i4	flags,
    char *file,
    i4	line)
{
    LKIDEF	lkidef[32], *plki;
    struct {
	u_i4        length:16;
	u_i4        width:15;
	u_i4        overflow:1;
    } local_siz;
    ILE3 itmlist1[] =
        {
	    { 0, LKI$_LOCKS, (char*)NULL, (unsigned short *)&local_siz },
	    { 0, 0 }
        };

    i4		i, totallki;
    i4 		status;
    bool	found = FALSE;
    char	keybuffer[256];
    i4		mode = cxvms_lk_to_dlm_mode[preq->cx_new_mode];
    i4		oldmode = cxvms_lk_to_dlm_mode[preq->cx_old_mode];
    i4		newmode = cxvms_lk_to_dlm_mode[preq->cx_new_mode];
    IOSB	iosb;
    CS_CPID	my_cpid;

    CSget_cpid(&my_cpid);

    itmlist1[0].bufadr = (char*)&lkidef;
    itmlist1[0].buflen = 32 * sizeof(LKIDEF);

    if ( !preq->cx_lock_id )
    {
	if ( preq->cx_status == E_CL2C27_CX_E_DLM_NOGRANT )
	    return(OK);
	    
	TRdisplay("%@ %s:%d\n"
		  "\tno DLM lock id, preq (%w,%w) mode %w\n"
		  "\tcstatus %x preq status %x flags %x lki_flags %x dlmflags %x posted %d\n"
		  "\towner %x.%x rqst %x.%x proxy %d\n"
		  "\t%s\n",
		      file, line,
		      VMS_LOCK_MODE, oldmode,
		      VMS_LOCK_MODE, newmode,
		      VMS_LOCK_MODE, mode,
		      cstatus, preq->cx_status, flags, preq->cx_lki_flags, preq->cx_dlmflags,
		      preq->cx_dlm_ws.cx_dlm_posted,
		      preq->cx_owner.pid, preq->cx_owner.sid,
		      my_cpid.pid, my_cpid.sid,
		      preq->cx_inproxy,
		      (CX_Proc_CB.cx_lkkey_fmt_func)
			? (*CX_Proc_CB.cx_lkkey_fmt_func)(&preq->cx_key, keybuffer)
			: "no fun");
	return(FAIL);
    }

    if ( (status = sys$getlki(0, &preq->cx_lock_id, itmlist1, &iosb, 
		cxdlm_getblki_ast, &my_cpid.sid, 0)) == SS$_NORMAL )
    {
	CSsuspend(0,0,0);

	if ( iosb.iosb$w_status == SS$_NORMAL )
	{
	    if ( local_siz.overflow )
	    {
		TRdisplay("%@ %s:%d DLM overflow, increase LKIDEF array\n",
			    "\t%s\n",
			    file, line,
			    (CX_Proc_CB.cx_lkkey_fmt_func)
				? (*CX_Proc_CB.cx_lkkey_fmt_func)(&preq->cx_key, keybuffer)
				: "no fun");
		return(FAIL);
	    }
		
	    totallki = local_siz.length / sizeof(LKIDEF);

	    for ( i = 0; i < totallki; i++ )
	    {
		plki = &lkidef[i];

		if ( plki->lki$l_lkid == preq->cx_lock_id )
		{
		    found = TRUE;
		    if ( plki->lki$b_queue != LKI$C_GRANTED ||
			 plki->lki$b_grmode != mode ||
			 !preq->cx_dlm_ws.cx_dlm_posted )
		    {
			TRdisplay("%@ %s:%d\n"
				  "\tDLM %x Q=%d(%s) GR %w RQ %w preq (%w,%w) expected %w\n"
				  "\tcstatus %x preq status %x flags %x lki_flags %x dlmflags %x posted %d\n"
				  "\topid %x owner %x.%x rqst %x.%x proxy %d i %d of %d\n"
				  "\t%s\n",
			    file, line,
			    preq->cx_lock_id,
			    plki->lki$b_queue,
			    (plki->lki$b_queue == LKI$C_GRANTED) ? "GR" :
			    (plki->lki$b_queue == LKI$C_CONVERT) ? "CV" :
			    (plki->lki$b_queue == LKI$C_WAITING) ? "WT" : "??",
			    VMS_LOCK_MODE, plki->lki$b_grmode,
			    VMS_LOCK_MODE, plki->lki$b_rqmode,
			    VMS_LOCK_MODE, oldmode,
			    VMS_LOCK_MODE, newmode,
			    VMS_LOCK_MODE, mode,
			    cstatus, preq->cx_status, flags, preq->cx_lki_flags, preq->cx_dlmflags,
			    preq->cx_dlm_ws.cx_dlm_posted,
			    plki->lki$l_pid,
			    preq->cx_owner.pid, preq->cx_owner.sid,
			    my_cpid.pid, my_cpid.sid,
			    preq->cx_inproxy,
			    i, totallki,
			    (CX_Proc_CB.cx_lkkey_fmt_func)
				? (*CX_Proc_CB.cx_lkkey_fmt_func)(&preq->cx_key, keybuffer)
				: "no fun");
			return(FAIL);
		    }
		    return(OK);
		}
	    }
	    if ( !found )
	    {
		TRdisplay("%@ %s:%d DLM %x not found in %d locks\n"
			  "\t%s\n",
			    file, line,
			    preq->cx_lock_id,
			    totallki,
			    (CX_Proc_CB.cx_lkkey_fmt_func)
				? (*CX_Proc_CB.cx_lkkey_fmt_func)(&preq->cx_key, keybuffer)
				: "no fun");
		return(FAIL);
	    }
	    return(OK);
	}
	else
	{
	    status = iosb.iosb$w_status;
	    TRdisplay("%@ %s:%d getlkiw returned iosb %x\n",
			    file, line,
			    status);
	}
    }
    else
    {
	TRdisplay("%@ %s:%d getlkiw returned %x\n",
			    file, line,
			    status);
    }
    return(status);

}

#endif /* end _VMS_ */


/* EOF: CXDLM.C */
