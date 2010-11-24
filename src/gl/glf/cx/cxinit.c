/*
** Copyright (c) 2001, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <er.h>
#include    <pc.h>
#include    <cs.h>
#include    <st.h>
#include    <tr.h>
#include    <cm.h>
#include    <cv.h>
#include    <pm.h>
#include    <lk.h>
#include    <cx.h>
#include    <cxprivate.h>
#ifdef VMS
#include <starlet.h>
#include <efndef.h>
#include <iledef.h>
#endif
#include "cxlocal.h"

/**
**
**  Name: CXINIT.C - Connect to (initialize) CX facility.
**
**  Description:
**      This module contains those routines which manage the initialization
**	and shutdown of the CX facility.  It is expected that internal
**	implementations will be highly OS specific.
**
**          CXinitialize()	- Connect to (initialize) CX.
**	    CXjoin()		- Complete enrollment of process into cluster.
**	    CXterminate()	- Release CX resources & revert CX to
**				  uninitialized state.
**	    CXalter()		- Modify CX facility parameters.
**	    CXstartrecovery()	- Set any necessary context for node
**				  failure recovery at CX level.
**	    CXfinishrecovery()	- Complete processing of recovery
**				  context started by previous call
**				  to 'CXstartrecovery'.
**
**	    		Private routines
**
**	    cx_debug()		- Conditionally compiled CX iimonitor
**				  extention.
**
**  History:
**      06-feb-2001 (devjo01)
**          Created.
**	06-jan-2004 (devjo01)
**	    Add Intel Linux support (s111426).
**      28-Jan-2004 (fanra01)
**	    Add ifdef around cx_indx_to_signum as constant initializers are
**	    not available on windows.
**	01-apr-2004 (devjo01)
**	    Add support for an alternate CLM implementation using files.
**	    Add DLM_PURGE to LNX OpenDLM api functions supported.
**	28-apr-2004 (devjo01)
**	    Add DLM version check for Linux.
**      04-May-2004 (hanje04)
**          Only pull in DLM specific code on Linux if we're building
**          CLUSTER_ENABLED. Regular Linux builds should not include
**          this code.
**	07-May-2004 (devjo01)
**	    hanje04 inadvertently used CLUSTER_ENABLED where
**	    conf_CLUSTER_BUILD was intended.  Corrected this, and
**	    reworked logic so this is used for all platforms.
**	14-may-2004 (devjo01)
**	    Add support for CX_A_QUIET.
**      20-sep-2004 (devjo01)
**	    Cumulative CX changes for Linux beta.
**          - Add dlm_scnop to OpenDLM API list.  
**	    - Add init for CX_CMO_SCN.
**          - Set init state to CX_ST_INIT before call to cx_msg_clm_file_init,
**            so CX_PARANOIA checks are not triggered by CXdlm calls.
**	    - Set default values for cx_lkkey_fmt_func, and cx_error_log_func.
**	    - Move common CX_NODE_CB initialization out of per-OS sections.
**	    - Add support to CXalter for CX_A_LK_FORMAT_FUNC and
**	      CX_A_ERR_MSG_FUNC.
**	02-nov-2004 (devjo01)
**	    - Support for OpenDLM recovery domains.
**	    - CXterminate to explicitly release all process owned locks.
**	14-feb-2005 (devjo01)
**	    Add support of dlmlibrary parameter to allow CX to resolve
**	    libdlm.so location in environments where LD_LIBRARY_PATH
**	    is not correctly set or cannot be inherited.
**	22-apr-2005 (devjo01)
**	    VMS port submission.
**	09-may-2005 (devjo01)
**	    Add flexibility to OpenDLM version checking.
**    11-may-2005 (stial01)
**        Changed sizeof LK_CSP lock from 8 to 12 (3 * sizeof(i4))
**	11-may-2005 (devjo01)
**	    Distributed deadlock support. Sir 114504.
**	21-jun-2005 (devjo01)
**	    Add support for CX_A_PROXY_OFFSET to CXalter.
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
**	28-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
**      03-nov-2010 (joea)
**          Include cxlocal.h for prototypes.
*/


/*
**	OS independent declarations
*/

GLOBALREF	CX_PROC_CB	CX_Proc_CB;

GLOBALREF	CX_CMO_DLM_CB	CX_cmo_data[CX_MAX_CMO][CX_LKS_PER_CMO];

typedef struct _CX_CSP_LKVAL
{
    u_i4	cx_csplv_nodenumber;
    u_i4	cx_csplv_state;
    u_i1        cx_csplv_spare[CX_DLM_VALUE_SIZE-2*sizeof(u_i4)];
}	CX_CSP_LKVAL;


typedef struct _CX_CFG_LKVAL
{
    u_i4	cx_cfglv_known_nodes;
    u_i1	cx_cfglv_cmotype;	/* All */
    u_i1	cx_cfglv_dlmsignal;	/* Tru64, Linux only */
    u_i1	cx_cfglv_imcrail;	/* Tru64: "rail" used by IMC */
    u_i1	cx_cfglv_clmtype;	/* ALL: Clust Long Msg impl. Type */
    u_i1	cx_cfglv_clmgran;	/* ALL: Chunk size as power of 2 */
    u_i1	cx_cfglv_clmsize;	/* ALL: Size in CX_CLM_SIZE_SCALE. */
    u_i1	cx_cfglv_spare[CX_DLM_VALUE_SIZE-(sizeof(u_i4)+6*sizeof(u_i1))];
}	CX_CFG_LKVAL;

static STATUS cx_get_static_config( CX_CFG_LKVAL * );

static void cx_default_error_log_func( i4, PTR, i4, ... );

# if defined(CX_ENABLE_DEBUG) || defined(CX_PARANOIA)
static STATUS cx_debug( i4 (*)(PTR,i4,char*), char *);
# endif

# if defined(LNX) && defined(conf_CLUSTER_BUILD)
/*
** Signal handler for OpenDLM asynchronous processing.
*/
void
cx_lnx_dlm_poller( int ignore )
{
    ASTPOLL( CX_Proc_CB.cx_pid, 0 );
}

/*
** Parse an OpenDLM version string.  
**
** Expected formats are <major>.<minor> and <major>.<minor>.<dont_care>,
** where <major> and <minor> are integers.
*/
static STATUS
cx_parse_opendlm_version( char *version, i4 *majorver, i4 *minorver )
{
    STATUS	status = FAIL;
    char	buf[32];
    char	*bp, *vp;
    *majorver = *minorver = 0;

    do /* Something to break out of */
    {
	/* Extract Major Version */
	vp = version;
	bp = buf;
	while ( CMdigit(vp) && ((vp - version) < sizeof(buf)) )
	    CMcpyinc(vp,bp);
	if ( vp == version || '.' != *vp )
	    break;
	CMcpychar("", bp);
	if ( OK != CVan(buf, majorver) )
	    break;

	/* Extract Minor Version */
	bp = buf;
	version = CMnext(vp);
	while ( CMdigit(vp) && ((vp - version) < sizeof(buf)) )
	    CMcpyinc(vp,bp);
	if ( vp == version || ('\0' != *vp && '.' != *vp ) )
	    break;
	CMcpychar("", bp);
	if ( OK != CVan(buf, minorver) )
	    break;

	status = OK;
    } while (0);

    return status;
}

# endif /* LNX */

# define USE_OPENDLM_RDOM 1


/*{
** Name: CXinitialize()	- Establish connection to (initialize) CX sub-facility.
**
** Description:
**      This routine is called after shared memory for the local node
**	has been mapped (LGK_initialize), but before any CX DLM activity
**	or CMO manipulation.   Before LK (Ingres level) lock activity can
**	be performed, additional CX calls must be made to join the calling
**	process to an Ingres Cluster.   See design doc for Sir 103715
**	for details.
**
** Inputs:
**      installation	- Two char identiity string  for this installation.
**	psharedmem	- Ptr. to shared memory segment for this node.
**	flags		- OR'ed special processing flags. Valid bits are:
**				CX_F_IS_CSP - Set if caller is CSP process.
**
** Outputs:
**	none
**
**	Returns:
**
**	    E_CL2C10_CX_E_NOSUPPORT	- No OS cluster support.
**	    E_CL2C11_CX_E_BADPARAM	- Called with bad parameter.
**	    E_CL2C13_CX_E_NOTCONFIG	- Cluster support not configured
**	    E_CL2C14_CX_E_NOCSP		- Caller not a CSP & no CSP up.
**	    E_CL2C15_CX_E_SECONDCSP	- Caller is a CSP & another CSP is up.
**	    E_CL2C16_CX_E_BADCONFIG	- config'ed node# out of range,
**					  or was added after cluster
**					  startup.
**	    E_CL2C1A_CX_E_CORRUPT	- Inexplicable contol info mismatch.
**	    E_CL2C1B_CX_E_BADCONTEXT	- Already connected.
**	    E_CL2C20_CX_E_NODLM		- Vendor DLM unavailable.
**	    E_CL2C28_CX_E_DLM_BADVERSION - Incorrect DLM version.
**
**	    Also depending on OS implementation, various
**	    E_CL2C1x_CX_E_BADCONFIG	- OS specific config problem.
**	    E_CL2C3x_CX_E_OSFAIL	- OS level failure.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    CX control blocks for process filled out.
**	    CX DLM calls enabled.
**	    CX CMO calls enabled.
**	    If caller is a CSP control block for node inititalized.
**	    If caller is CSP, "CSP lock" acquired.
**
** History:
**	06-feb-2001 (devjo01)
**	    Created.
**	27-mar-2004 (devjo01)
**	    Remove redundant E_CL2C33 set.
**	28-apr-2004 (devjo01)
**	    Add DLM version check for Linux.
**	14-may-2004 (devjo01)
**	    Only connect to CLM if a CSP or a regular server (NEED_CSP).
**	06-may-2004 (fanch01)
**	    Change for CMR recovery.
**	    Remove CMR initialization.  CX should ideally not be aware of the
**	    CMR and moving it to dmfcsp is required for CMR recovery.
**	11-may-2005 (devjo01)
**	    - Init new vms specific cx_proc_cb members used to support
**            CXdlm_get_blki (cxvms_bli_sem, cxvms_bli_buf, cxvms_bli_size).
**	    - Get CX_CONFIGURATION in generic code and anchor to cx_ni.
**	    - Fill in cxvms_csids (node# -> VMS csid lookup array).
**	13-jul-2007 (joea)
**	    Use two buffers for DLM info.
**	04-Oct-2007 (jonj)
**	    New CXalter function CX_A_NEED_CSID to discover VMS
**	    CSID.
**	04-Apr-2008 (jonj)
**	    Replace cx_glc_ready with cx_glc_triggered.
*/
STATUS
CXinitialize(char *installation, VOID *psharedmem, u_i4 flags)
{
    CX_NODE_CB		*sms;
    STATUS		 status;
    i4			 osstat = 0;
    i4			 cmotype;
    i4			 clmtype;
    i4			 clmgran;
    i4			 clmsize;
    LK_LOCK_KEY		 lockkey;
    i4			 nodenumber;
    CX_CSP_LKVAL	*csplv;
    CX_CFG_LKVAL	*cfglv;
    CX_CMO_CMR		 cmrcmo;

    sms = (CX_NODE_CB*)psharedmem;

    if ( !CX_Proc_CB.cx_lkkey_fmt_func )
	CX_Proc_CB.cx_lkkey_fmt_func = CXdlm_lock_key_to_string;

    if ( !CX_Proc_CB.cx_error_log_func )
	CX_Proc_CB.cx_error_log_func = cx_default_error_log_func;

    for ( ; /* something to break out of */ ; )
    {
#	if defined(CX_ENABLE_DEBUG) || defined(CX_PARANOIA)
	IICSmon_register( "cx", cx_debug );
#	endif

#       ifdef CX_PARANOIA
	/* Can't be called twice. */
	if ( CX_ST_UNINIT != CX_Proc_CB.cx_state )
	{
	    status = E_CL2C1B_CX_E_BADCONTEXT;
	    return status; /* quick exit */
	}

	/*
	** Check parameters.
	*/
	if ( (flags & (~CX_F_IS_CSP)) ||
	     (NULL == installation) ||
	     (!installation[0]) ||
	     (!installation[1]) ||
	     (NULL == psharedmem) )
	{
	    status = E_CL2C11_CX_E_BADPARAM;
	    return status; /* quick exit */
	}
#       endif /*CX_PARANOIA*/

	/* Check that this box supports clusters */
	if ( !CXcluster_support() )
	{
	     status = E_CL2C10_CX_E_NOSUPPORT;
	     break;
	}

	/* Start filling in PCB */
	CX_Proc_CB.cx_ncb = sms;
	PCpid(&CX_Proc_CB.cx_pid);

	/* Get node number */
	nodenumber = CXnode_number( NULL );
	if ( 0 == nodenumber )
	{
	    status = E_CL2C13_CX_E_NOTCONFIG;
	    break;
	}
	if ( nodenumber < 0 )
	{
	    status = E_CL2C16_CX_E_BADCONFIG;
	    break;
	}
			    
	CX_Proc_CB.cx_node_num = nodenumber;

	/* Fetch info about nodes configured. */
	status = CXcluster_nodes( NULL, &CX_Proc_CB.cx_ni );
	if ( OK != status )
	{
	    status = E_CL2C16_CX_E_BADCONFIG;
	    break;
	}

	/*
	** Preserve certain flags ( CX_PF_ENABLE_FLAG,
	** CX_PF_ENABLE_FLAG_VALID, CX_PF_NODENUM_VALID,
	** CX_PF_NEED_CSP, and CX_PF_IS_CSP ), then mask
	** in passed flags.
	*/
	CX_Proc_CB.cx_flags &= 
	 CX_PF_ENABLE_FLAG|CX_PF_ENABLE_FLAG_VALID|CX_PF_IS_CSP|
	 CX_PF_NEED_CSP|CX_PF_NODENUM_VALID|CX_PF_QUIET;
	CX_Proc_CB.cx_flags |= flags;

	/* 
	** If a regular lock client, make sure SMS valid, and 
	** CSP is alive, and joined to cluster.  Most psuedo
	** servers do not require the CSP to be on-line,
	** and restrict usage of the CX facility to process
	** level locks.
	*/
	if ( CX_Proc_CB.cx_flags & CX_PF_IS_CSP )
	{
	    if ( CX_ST_UNINIT != sms->cx_node_state &&
		 0 != sms->cx_csp_pid &&
		 PCis_alive( sms->cx_csp_pid ) )
	    {
		/* CSP already installed */
		status = E_CL2C15_CX_E_SECONDCSP;
		break;
	    }
	    /* Perform common "static" CX_NODE_CB initialization */
	    sms->cx_mem_mark = CX_NCB_TAG;
	    sms->cx_ncb_version = CX_NCB_VER;
	    sms->cx_node_num = nodenumber;
	    CS_ACLR( &sms->cx_uniqprot );
	    CS_ACLR( &sms->cx_ivb_seen );
	    sms->cx_lsn.cx_lsn_low = sms->cx_lsn.cx_lsn_high = 0;
	}
	else if ( CX_Proc_CB.cx_flags & CX_PF_NEED_CSP )
	{
	    if ( sms->cx_node_state != CX_ST_JOINED ||
		 0 == sms->cx_csp_pid || 
		!PCis_alive( sms->cx_csp_pid ) )
	    {
		status = E_CL2C14_CX_E_NOCSP;
		break;
	    }

	    if ( sms->cx_node_num != nodenumber ||
		 sms->cx_mem_mark != CX_NCB_TAG ||
		 sms->cx_ncb_version != CX_NCB_VER )
	    {
		status = E_CL2C1A_CX_E_CORRUPT;
		break;
	    }
	}

	csplv = (CX_CSP_LKVAL *)CX_Proc_CB.cx_csp_req.cx_value;
	cfglv = (CX_CFG_LKVAL *)CX_Proc_CB.cx_cfg_req.cx_value;

	/*
	**	Start OS specific stuff
	*/

#if !defined(conf_CLUSTER_BUILD)

    status = E_CL2C10_CX_E_NOSUPPORT;

#elif defined(axp_osf)
    /*
    *****************************************************************
    **	Tru64 implementation
    *****************************************************************
    **
    **	Tru64 provides both a vendor DLM, plus an API to a
    **	gigabit intra-cluster memory channel (IMC), which
    **	allows very fast messaging between cluster nodes.
    **
    **	We will take advantage of both of these.  IMC will
    **	be used to support CMO, while the OS DLM will form
    **	the guts of the CX DLM routines.   For a fuller
    **	discussion of this implementation, see the design
    **	doc for SIR 103715.
    **
    **	Tru64 DLM provides name spaces at the process, group 
    **	and system levels.  Since all Ingres installations use 
    **	'ingres' as the	owning user, and since only one name 
    **	space can exist per user/group/system in the Tru64 
    **	implementation, Resource names need to be qualified with 
    **	the Installation code in order to allow multiple installations 
    **	per cluster.  Installation code is converted into a
    **	16-bit quantity which is masked into the high 16 bits
    **	of the resource name when constructing the internal
    **	lock key.
    **
    **	Tru64 allows locks on any one node to be taken in the
    **	context of a "group lock container" (GLC).  We will
    **	take advantage of this in order to simplify lock client
    **	failure processing.
    **
    **	Tru64 also supports recovery domains in which if a
    **	resource is left in an unknown osstate by a process
    **	holding an write (SIX,X) lock dying, the resource
    **	osstate is maintained even if all locks on this
    **	resource were lost.   We use this, along with
    **	"dead man locks" to detect lock client failure in
    **	an Ingres cluster.
    **
    **	Tru64 allows locks to be associated with transactions
    **	so we can use the OS deadlock detection in leiu of
    **	Ingres'es if desired.
    **
    **	Tru64 IMC API, allows a memory region to be mapped by
    **	processes running on different physical machines, but
    **	with the odd twist, that individual mappings can be
    **	either write only, or read only, and that data written
    **	to the write-only address, will not be seen at the
    **	read-only address for a few nano-secs.   We use this
    **	shared memory support for CMO.
    **
    */
    {	/* AXP block */
	static char *cxaxp_dlm_fnames[] = {
	 "dlm_nsjoin",	 "dlm_nsleave",	    "dlm_set_signal",
	 "dlm_rd_attach","dlm_rd_detach",   "dlm_rd_collect",
	 "dlm_rd_validate","dlm_glc_create","dlm_glc_destroy",
	 "dlm_glc_attach","dlm_glc_detach", "dlm_locktp",
	 "dlm_quelocktp","dlm_cvt",         "dlm_quecvt",
	 "dlm_unlock",   "dlm_cancel",      "dlm_get_lkinfo",
	 "dlm_get_rsbinfo" };
	static char *cxaxp_imc_fnames[] = { 
	 "imc_api_init", "imc_rderrcnt_mr", "imc_ckerrcnt_mr",
	 "imc_asalloc",  "imc_asattach",    "imc_lkalloc",
	 "imc_lkacquire","imc_lkrelease" };

	i4		 dlmsignal;
	i4		 imcrail;
	char		 rdname[4];
	i4		 i;

	/*
	** Connect to DLM
	*/
	status = cx_dl_load_lib( ERx("libdlm.so"),
	 &CX_Proc_CB.cxaxp_dlm_lib_hndl,
	 CX_Proc_CB.cxaxp_dlm_funcs, cxaxp_dlm_fnames,
	 sizeof(cxaxp_dlm_fnames)/sizeof(cxaxp_dlm_fnames[0]));
	if ( status )
	    break;

	/* Join a namespace private to ingres */
	osstat = DLM_NSJOIN( geteuid(), &CX_Proc_CB.cxaxp_nsp, DLM_USER );
	if ( DLM_SUCCESS != osstat )
	{
	    break;
	}

	/* Get process lock recovery domain */
	rdname[0] = installation[0];
	rdname[1] = installation[1];
	rdname[2] = 'P';
	rdname[3] = '\0';
	osstat = DLM_RD_ATTACH( rdname, 
	   &CX_Proc_CB.cxaxp_prc_rd, DLM_RD_PROCESS );
	if ( DLM_SUCCESS != osstat )
	{
	    break;
	}

	/* Calc. mask to use when gening internal lock keys (resource names) */
	CX_Proc_CB.cxaxp_lk_instkey = ((1<<24)*(u_i4)installation[0]) +
	 (1<<16)*(u_i4)installation[1];
	CX_Proc_CB.cx_lk_nodekey = ((1<<8)*nodenumber);

	/* Process CSP lock */
	if ( CX_Proc_CB.cx_flags & CX_PF_IS_CSP )
	{
	    /*
	    ** Caller is a CSP.
	    */
	    dlm_flags_t	lkflgs = DLM_VALB | DLM_PERSIST | DLM_NOQUEUE;
	    i4		countdown = 0;

	    lockkey.lk_type = LK_CSP | CX_Proc_CB.cxaxp_lk_instkey;
	    lockkey.lk_key1 = nodenumber;
	    lockkey.lk_key2 = 0;

	    /* Loop here, trying to get "CSP lock" */
	    while ( 1 )
	    {
		/* Try for CSP lock (cannot use CXdlm_request yet) */
		osstat = DLM_LOCKTP( CX_Proc_CB.cxaxp_nsp, 
			    (uchar_t*)&lockkey, 
			    3 * sizeof(i4), NULL, 
			    (dlm_lkid_t *)&CX_Proc_CB.cx_csp_req.cx_lock_id, 
			    DLM_EXMODE, (dlm_valb_t*)csplv, lkflgs,
			    (callback_arg_t)0, (callback_arg_t)0, 
			    (blk_callback_t)0, (uint_t)(10 * 100),
			    (dlm_trans_id_t)0, CX_Proc_CB.cxaxp_prc_rd );
		if ( DLM_SUCCESS == osstat || DLM_SUCCVALNOTVALID == osstat )
		{
		    /* All is well, procede. */
		    break;
		}

		if ( DLM_NOTQUEUED == osstat )
		{
		    /* Turn off NOQUEUE, so next lock request will wait. */
		    lkflgs = DLM_VALB | DLM_PERSIST;
		}
		else if ( DLM_TIMEOUT == osstat )
		{
		    if ( 0 == countdown-- )
		    {
			/* A 2nd or subsequent request timed out. */
			(void)TRdisplay(
		  ERx("%@ CSP on node %s waiting for recovery to complete.\n"), 
			  CXnode_name(NULL) );
			/* Only one message per minute. */
			countdown = 5;
		    }
		}
		else
		{
		    /* Unexpected error */
		    status = cx_translate_status( osstat );
		    break;
		}

		/*
		** Another CSP must already be up, or recovery is
		** being performed for this node.  Check SMS to 
		** see if allocating CSP still lives.
		*/
		if ( 0 != sms->cx_csp_pid && PCis_alive( sms->cx_csp_pid ) )
		{
		    status = E_CL2C15_CX_E_SECONDCSP;
		    break;
		}

		/* 
		** Allocating CSP is not alive, so we can assume reason
		** lock cannot be grabbed is because another CSP is holding
		** a DMN lock on the node, while it performs recovery on
		** behalf of a previous CSP instance for this node.
		** Code will continue to loop, performing this check every
		** ten seconds, until lock is granted, or process is
		** aborted by user.
		*/
	    } /* end while (1) */

	    if ( OK != status )
	    {
		/* Could not get CSP lock.  Redundant CSP, or gross failure. */
		break;
	    }

	    /* (re)Initialize lock value block */
	    (void)MEfill( sizeof(csplv),'\0',(PTR)csplv );
	    csplv->cx_csplv_nodenumber = nodenumber;
	    csplv->cx_csplv_state = CX_ST_INIT;

	    /* Force out new CSP lock value */
	    osstat =
	      DLM_CVT( (dlm_lkid_t *)&CX_Proc_CB.cx_csp_req.cx_lock_id, 
			DLM_EXMODE, (dlm_valb_t*)csplv, DLM_VALB, 
			(callback_arg_t)0, (callback_arg_t)0,
			(blk_callback_t)0, (uint_t)0 );
	    if ( DLM_SUCCESS != osstat )
	    {
		/* Unexpected failure */
		break;
	    }
	    CX_Proc_CB.cx_csp_req.cx_old_mode =
	     CX_Proc_CB.cx_csp_req.cx_new_mode = LK_X;
	    CX_Proc_CB.cx_csp_req.cx_lki_flags = CX_LKI_F_PCONTEXT;

	    /*
	    ** Check for "first CSP" status, and get config info
	    ** that must be consistent across the entire cluster.
	    */
	    /* Loop here, trying to get "Config lock" */
	    while (1)
	    {
		lockkey.lk_key1 = 0;
		lockkey.lk_key2 = 0;
		osstat = DLM_LOCKTP( CX_Proc_CB.cxaxp_nsp, 
				(uchar_t*)&lockkey, 
				3 * sizeof(i4), NULL, 
				(dlm_lkid_t *)&CX_Proc_CB.cx_cfg_req.cx_lock_id,
				DLM_EXMODE, (dlm_valb_t*)cfglv, 
				DLM_VALB | DLM_PERSIST | DLM_NOQUEUE,
				(callback_arg_t)0, (callback_arg_t)0, 
				(blk_callback_t)0, (uint_t)0,
				(dlm_trans_id_t)0, CX_Proc_CB.cxaxp_prc_rd );
		if ( DLM_SUCCESS == osstat || DLM_SUCCVALNOTVALID == osstat )
		{
		    /*
		    ** This CSP is now the "first CSP".  Get critical cluster
		    ** scope configuration info from configuration file.
		    */
		    CX_Proc_CB.cx_cfg_req.cx_old_mode =
		      CX_Proc_CB.cx_cfg_req.cx_new_mode = LK_X;
		    CX_Proc_CB.cx_cfg_req.cx_lki_flags = CX_LKI_F_PCONTEXT;

		    status = cx_get_static_config( cfglv );
		    if (status != OK)
		    {
			break;
		    }

		    cmotype = (i4)(cfglv->cx_cfglv_cmotype);
		    dlmsignal = (i4)(cfglv->cx_cfglv_dlmsignal);
		    imcrail = (i4)(cfglv->cx_cfglv_imcrail);
		    clmtype = (i4)(cfglv->cx_cfglv_clmtype);
		    clmgran = (i4)(cfglv->cx_cfglv_clmgran);
		    clmsize = (i4)(cfglv->cx_cfglv_clmsize);

		    {
			u_i4	i = CX_Proc_CB.cx_ni->cx_node_cnt;

			while ( i != 0 )
			{
			    cfglv->cx_cfglv_known_nodes |= 1 <<
			     (CX_Proc_CB.cx_ni->cx_nodes 
			       [CX_Proc_CB.cx_ni->cx_xref[i]].cx_node_number - 1);
			    i--;
			}
		    }

		    /* Flush out config value */
		    osstat = DLM_CVT( 
		      (dlm_lkid_t *)&CX_Proc_CB.cx_cfg_req.cx_lock_id, 
		      DLM_EXMODE, (dlm_valb_t*)cfglv, DLM_VALB, 
		      (callback_arg_t)0, (callback_arg_t)0,
		      (blk_callback_t)0, (uint_t)0 );
		    if ( DLM_SUCCESS != osstat )
		    {
			/* Unexpected failure */
			break;
		    }

		    /* CSP is "First CSP", and initial Master CSP. */
		    CX_Proc_CB.cx_flags |= CX_PF_FIRSTCSP | CX_PF_MASTERCSP;
		    break;
		}
		else if ( DLM_NOTQUEUED == osstat )
		{
		    /*
		    ** CSP is NOT the "first CSP", get configuration
		    ** from lock value block.
		    **
		    ** CSP may block here for a while, while "First CSP"
		    ** running on another node puts everything in order.
		    */
		    osstat = DLM_LOCKTP( CX_Proc_CB.cxaxp_nsp, 
				(uchar_t*)&lockkey, 
				3 * sizeof(i4), NULL, 
				(dlm_lkid_t *)&CX_Proc_CB.cx_cfg_req.cx_lock_id,
				DLM_PRMODE, (dlm_valb_t*)cfglv, 
				DLM_VALB | DLM_PERSIST,
				(callback_arg_t)0, (callback_arg_t)0, 
				(blk_callback_t)0, (uint_t)0,
				(dlm_trans_id_t)0, 
				CX_Proc_CB.cxaxp_prc_rd );
		    if ( DLM_SUCCESS == osstat )
		    {
			CX_Proc_CB.cx_cfg_req.cx_old_mode =
			 CX_Proc_CB.cx_cfg_req.cx_new_mode = LK_S;

			/* Got cfg lock in share. Read values, and move on */
			cmotype = (i4)cfglv->cx_cfglv_cmotype;
			clmtype = (i4)cfglv->cx_cfglv_clmtype;
			dlmsignal = (i4)cfglv->cx_cfglv_dlmsignal;
			imcrail = (i4)cfglv->cx_cfglv_imcrail;
			clmgran = (i4)(cfglv->cx_cfglv_clmgran);
			clmsize = (i4)(cfglv->cx_cfglv_clmsize);

			/*
			** Check to see if node was added to configuration 
			** after cluster was initially started.  This is
			** currently illegal.
			*/
			if ( !( ( 1 << (nodenumber - 1) ) &
			     cfglv->cx_cfglv_known_nodes ) )
			{
			    status = E_CL2C16_CX_E_BADCONFIG;
			}
			break;
		    }
		    else if ( DLM_SUCCVALNOTVALID == osstat )
		    {
			/*
			** Lock value was invalidated! Previous "First CSP"
			** candidate must have died.  Try again to be 
			** "First CSP".
			*/
			(void)DLM_UNLOCK(
			   (dlm_lkid_t *)&CX_Proc_CB.cx_cfg_req.cx_lock_id,
			   (dlm_valb_t *)NULL, 0 );
			CX_ZERO_OUT_ID(&CX_Proc_CB.cx_cfg_req.cx_lock_id);
			continue;
		    }
		    else
		    {
			/* Unexpected error. */
			break;
		    }
		}
		else
		{
		    /* Unexpected error. */
		    break;
		}
	    } /* end-while */

	    if ( DLM_SUCCESS != osstat || OK != status )
	    {
		/* Failure in get configuration step. */
                break;
            }

	    /* 
	    ** Check to see if Shared Mem. Seg. was previously used.
	    ** If so, release any resources held from before.
	    */
	    if ( sms->cxaxp_glc != (dlm_glc_id_t)0 )
	    {
		/*
		** Old GLC may still exist, kill it, and all processes
		** attached to it.   Local lock clients should have already
		** died when the CSP which allocated the SMS died, so 
		** there should be no lock clients at this point.
		*/
		(void)DLM_GLC_DESTROY( sms->cxaxp_glc, DLM_GLC_FORCE );
	    }

	    /* Create new GLC. */
	    osstat = DLM_GLC_CREATE( &CX_Proc_CB.cxaxp_glc_id, DLM_GLC_OWNED, 
				     &CX_Proc_CB.cxaxp_nsp, 1 );
	    if ( DLM_SUCCESS != osstat )
	    {
		/* Unexpected failure */
		break;
	    }

	    /* Fill in Rest of NCB (SMS) */
	    sms->cx_node_state = CX_ST_INIT;
	    sms->cx_cmo_type = cmotype;
	    sms->cx_clm_type = clmtype;
	    sms->cx_clm_gran = clmgran;
	    sms->cx_clm_size = clmsize;
	    sms->cx_csp_pid = CX_Proc_CB.cx_pid;
	    sms->cxaxp_glc = CX_Proc_CB.cxaxp_glc_id;
	    sms->cx_imc_rail = imcrail;
	    sms->cx_dlm_signal = dlmsignal;
	    (void)CSw_semaphore( &sms->cx_lsn_sem, CS_SEM_MULTI,
		ERx("cx_lsn_sem") );
	}
	else if ( CX_Proc_CB.cx_flags & CX_PF_NEED_CSP )
	{
	    /*
	    ** Caller is a regular cluster member.
	    */

	    /* Get config parameters from NCB */
	    cmotype = sms->cx_cmo_type;
	    clmtype = sms->cx_clm_type;
	    clmgran = sms->cx_clm_gran;
	    clmsize = sms->cx_clm_size;
	    dlmsignal = sms->cx_dlm_signal;
	    imcrail = sms->cx_imc_rail;

	    /* Attach to GLC established by CSP */
	    if ( sms->cxaxp_glc == (dlm_glc_id_t)0 )
	    {
		status = E_CL2C14_CX_E_NOCSP;
		break;
	    }
	    osstat = DLM_GLC_ATTACH( sms->cxaxp_glc, 0 );
	    if ( DLM_SUCCESS != osstat )
	    {
		/* Unexpected failure */
                break;
            }
	    /* Remember connection */
	    CX_Proc_CB.cxaxp_glc_id = sms->cxaxp_glc;
	}
	else
	{
	    /*
	    ** Caller needs to use limited CX facilities, but
	    ** need not be (and cannot be) a full member of
	    ** the cluster.   Certain DMF utilities (RCPSTAT,
	    ** LOGSTAT, LOCKSTAT, et.al.) fall into this class.
	    ** This permits caller to operate "off-line", that
	    ** is before the CSP process has started.
	    **
	    ** Only locks taken at process level may be acquired.
	    ** (See LKRQST.C for code making this decision.)
	    */
	    if ( sms->cx_node_num == nodenumber &&
		 sms->cx_mem_mark == CX_NCB_TAG &&
		 sms->cx_ncb_version == CX_NCB_VER )
	    {
		/* If memory segment initialized, get config from it. */
		dlmsignal = sms->cx_dlm_signal;
		/* cmotype = sms->cx_cmo_type; Don't care about this */
		/* imcrail = sms->cx_imc_rail; Don't care about this */
	    }
	    else
	    {
		/*
		** Read configuration from config.dat, tolerating the
		** minute risk that config has changed while other
		** cluster members are on-line.
		*/
		status = cx_get_static_config( cfglv );
		if ( status )
		    break;
		cmotype = (i4)(cfglv->cx_cfglv_cmotype);
		dlmsignal = (i4)(cfglv->cx_cfglv_dlmsignal);
		imcrail = (i4)(cfglv->cx_cfglv_imcrail);
		clmtype = (i4)(cfglv->cx_cfglv_clmtype);
		clmgran = (i4)(cfglv->cx_cfglv_clmgran);
		clmsize = (i4)(cfglv->cx_cfglv_clmsize);
	    }
# if 0
	    /* 
	    ** Processes not fully connected to cluster do not use
	    ** CMO facilities.   
	    */
	    cmotype = CX_CMO_NONE;
# endif
	}

	/* Squirrel away some more config info in PCB. */
	CX_Proc_CB.cx_cmo_type = cmotype;
	CX_Proc_CB.cx_clm_type = clmtype;
	CX_Proc_CB.cx_clm_blksize = (1 << clmgran);
	CX_Proc_CB.cx_clm_blocks =
	 (CX_CLM_SIZE_SCALE * clmsize) / CX_Proc_CB.cx_clm_blksize;
	CX_Proc_CB.cx_imc_rail = imcrail;

	/* Let DLM know what signal to use for blocking routines */
	(void)DLM_SET_SIGNAL( dlmsignal, NULL );

	if ( CX_Proc_CB.cx_flags & (CX_PF_IS_CSP|CX_PF_NEED_CSP) )
	{
	    /* Attach to Group Recovery Domain */
	    rdname[2] = 'G';
	    osstat = DLM_RD_ATTACH( rdname, &CX_Proc_CB.cxaxp_grp_rd,
				    DLM_RD_GROUP );
	    if ( DLM_SUCCESS != osstat )
	    {
		/* Unexpected failure */
		break;
	    }
	}

	if ( CX_CMO_IMC == cmotype )
	{
	    /* 
	    ** Dynamically link to IMC library, and fill in function
	    ** pointers.  IMC_<func> macros will dereference the
	    ** filled in function pointer array, and cast pointer
	    ** to proper type.
	    */
	    status = cx_dl_load_lib( ERx("libimc.so"),
	     &CX_Proc_CB.cxaxp_imc_lib_hndl,
	     CX_Proc_CB.cxaxp_imc_funcs, cxaxp_imc_fnames,
	     sizeof(cxaxp_imc_fnames)/sizeof(cxaxp_imc_fnames[0]));
	    if ( OK != status )
	    {
		break;
	    }

	    /* Start initializing Tru64 IMC */
	    osstat = IMC_API_INIT(NULL);
	    if ( IMC_SUCCESS != osstat )
	    {
		/* IMC Unconfigured, or unexpected error */
		TRdisplay("IMC not initialized!\n");
		break;
	    }

	    /* 
	    ** Get memory channel error count at program start.  
	    ** Note negative # is returned in the extremely unlikely case 
	    ** of a simultaneous update. 
	    */
	    while ( ( CX_Proc_CB.cxaxp_imc_errs = IMC_RDERRCNT_MR(0) ) < 0 )
		    /* Boy are we "lucky".  Try again */
		    ;
	    CX_Proc_CB.cxaxp_imc_errb = CX_Proc_CB.cxaxp_imc_errs;

	    /* Get chunk of global shared mem keyed by installation code */
	    osstat  = IMC_ASALLOC( CX_Proc_CB.cxaxp_lk_instkey, 
			CX_CMO_IMC_SEGSIZE + CX_MSG_IMC_SEGSIZE,
			IMC_URW|IMC_GRW|IMC_ORW, 
			IMC_COHERENT, &CX_Proc_CB.cxaxp_imc_gsm_id, imcrail );
	    if ( IMC_SUCCESS != osstat )
	    {
		/* Unexpected failure */
		break;
	    }

	    /* Get address used to write to GSM */
	    osstat  = IMC_ASATTACH( CX_Proc_CB.cxaxp_imc_gsm_id, IMC_TRANSMIT,
			IMC_SHARED, IMC_LOOPBACK, &CX_Proc_CB.cxaxp_imc_txa );
	    if ( IMC_SUCCESS != osstat )
	    {
		/* Unexpected failure */
		break;
	    }
	    CX_Proc_CB.cxaxp_cmo_txa = (CXAXP_CMO_IMC_CB *)
	     ( (PTR)CX_Proc_CB.cxaxp_imc_txa + CX_MSG_IMC_SEGSIZE );

	    /* Get address used to read from GSM */
	    osstat  = IMC_ASATTACH( CX_Proc_CB.cxaxp_imc_gsm_id, IMC_RECEIVE, 
			IMC_SHARED, 0, &CX_Proc_CB.cxaxp_imc_rxa );
	    if ( IMC_SUCCESS != osstat )
	    {
		/* Unexpected failure */
		break;
	    }
	    CX_Proc_CB.cxaxp_cmo_rxa = (CXAXP_CMO_IMC_CB *)
	     ( (PTR)CX_Proc_CB.cxaxp_imc_rxa + CX_MSG_IMC_SEGSIZE );

	    /*
	    ** <FIX-ME> permissions are set to user group & other,
	    ** because of a bug? in imc_lkalloc, in which real user
	    ** ID is used instead of the effective user ID, leading
	    ** to IMC_PERMIT errors when a user other than ingres
	    ** runs dmfjsp (and others), even though iimerge has the
	    ** sticky bit on.  Set permissions to IMC_LKU only, once
	    ** Compaq fixes this.
	    */
	    CX_Proc_CB.cxaxp_imc_lk_cnt = CXAXP_IMC_LK_CNT;
	    osstat = IMC_LKALLOC(CX_Proc_CB.cxaxp_lk_instkey + 1, 
			&CX_Proc_CB.cxaxp_imc_lk_cnt,
			IMC_LKU|IMC_LKG|IMC_LKO, IMC_CREATOR,
			&CX_Proc_CB.cxaxp_imc_lk_id);
	    if ( IMC_SUCCESS == osstat )
	    {
		/* This is the first process. Initialize the global region  */
		bzero( CX_Proc_CB.cxaxp_cmo_txa, CX_CMO_IMC_SEGSIZE );

		/* release the lock */
		osstat  = IMC_LKRELEASE(CX_Proc_CB.cxaxp_imc_lk_id, 0);
	    }
	    else if ( IMC_EXISTS == osstat )
	    {
		/* This is a secondary process */
		osstat = IMC_LKALLOC( CX_Proc_CB.cxaxp_lk_instkey + 1,
			   &CX_Proc_CB.cxaxp_imc_lk_cnt, 
			   IMC_LKU|IMC_LKG|IMC_LKO, 0, 
		 	   &CX_Proc_CB.cxaxp_imc_lk_id);          
	    }
	    if ( IMC_SUCCESS != osstat )
	    {
		/* Unexpected failure */
		break;
	    }
	}
    }	/* end axp block */

#elif defined(LNX)

    /*
    *****************************************************************
    **	Intel Linux implementation
    *****************************************************************
    **
    **  Linux implementation rests on the Open source
    **  OpenDLM project.
    **
    **	OpenDLM provides name spaces at the process, group 
    **	and system levels.  Since all Ingres installations use 
    **	'ingres' as the	owning user, and since only one name 
    **	space can exist per user/group/system in the Tru64 
    **	implementation, Resource names need to be qualified with 
    **	the Installation code in order to allow multiple installations 
    **	per cluster.  Installation code is converted into a
    **	16-bit quantity which is masked into the high 16 bits
    **	of the resource name when constructing the internal
    **	lock key.
    **
    **	OpenDLM allows locks on any one node to be taken in the
    **	context of a "group".  We will take advantage of this in
    **  order to simplify lock client failure processing.
    **
    **	OpenDLM allows locks to be associated with transactions
    **	so we can use the OS deadlock detection in leiu of
    **	Ingres's if desired.
    **
    **	Tru64 IMC API, allows a memory region to be mapped by
    **	processes running on different physical machines, but
    **	with the odd twist, that individual mappings can be
    **	either write only, or read only, and that data written
    **	to the write-only address, will not be seen at the
    **	read-only address for a few nano-secs.   We use this
    **	shared memory support for CMO.
    **
    */
    {	/* Intel Linux block */
	static char *cxlnx_dlm_fnames[] = {
	 "dlm_setnotify",  "dlm_grp_create", "dlm_grp_attach",
	 "dlm_grp_detach", "dlmlock_sync",   "dlmlock",
	 "dlmlockx_sync",  "dlmlockx",       "dlmunlock_sync",
	 "ASTpoll", "dlm_purge", "dlm_scnop",
	 /* New Recovery domain functions */
	 "dlm_rd_attach", "dlm_rd_detach",
	 "dlm_rd_collect", "dlm_rd_validate"
	};

	i4		 dlmsignal;
	i4		 imcrail;
	i4		 i;
	char		*param;

	/*
	** Attempt to get DLM version from proc system.
	*/
	{
	    FILE        *fp;
	    char	 version[16];
	    i4		 emajor, eminor, vmajor, vminor;

	    status = PMget( ERx("ii.$.config.dlmverfile"), &param );
	    if ( OK != status || NULL == param || '\0' == *param )
		param = ERx("/proc/haDLM/version");

	    if ( 0 != STcmp( param, ERx("none") ) )
	    {
		CX_NOISY_OUTPUT( "%@ DLM version file = %s\n", param );

		fp = fopen( param, "r" );
		if ( NULL == fp )
		{
		    /* OpenDLM is not installed/active */
		    status = E_CL2C20_CX_E_NODLM;
		    break;
		}

		i = fscanf( fp, "%15s", version );
		fclose( fp );

		if ( 1 != i )
		{
		    status = E_CL2C29_CX_E_DLM_BADVERSION;
		    CX_REPORT_STATUS(status);
		    break;
		}

		CX_NOISY_OUTPUT( "%@ DLM version = %s\n", version );

		status = PMget( ERx("ii.$.config.dlmversion"), &param );
		if ( OK != status || NULL == param || '\0' == *param )
		    param = CX_DLM_VERSION;

		/*
		** DLM major versions must match.  Minor version
		** may be checked for specific capabilities.
		** Major version "0" gets special treatment, in
		** that versions prior to 0.4 are universally
		** depreciated.
		*/
		if ( cx_parse_opendlm_version(param, &emajor, &eminor ) ||
		     cx_parse_opendlm_version(version, &vmajor, &vminor ) ||
		     emajor != vmajor ||
		     ((emajor == 0) && (eminor < 4)) )
		{
		    TRdisplay( "%@ Expected DLM version = %s\n", param );
		    status = E_CL2C29_CX_E_DLM_BADVERSION;
		    break;
		}

	    }
	}
	
	/*
	** Connect to DLM
	*/
	status = PMget( ERx("ii.$.config.dlmlibrary"), &param );
	if ( OK != status || NULL == param || '\0' == *param )
	{
	    param = ERx("/usr/local/lib/libdlm.so");
	}

	status = cx_dl_load_lib( param, &CX_Proc_CB.cxlnx_dlm_lib_hndl,
	 CX_Proc_CB.cxlnx_dlm_funcs, cxlnx_dlm_fnames,
	 sizeof(cxlnx_dlm_fnames)/sizeof(cxlnx_dlm_fnames[0]));

	BREAK_ON_BAD_STATUS( status );

	/* Calc. mask to use when gening internal lock keys (resource names) */
	CX_Proc_CB.cxlnx_lk_instkey = ((1<<24)*(u_i4)installation[0]) +
	 (1<<16)*(u_i4)installation[1];

	/* Process CSP lock */
	if ( CX_Proc_CB.cx_flags & CX_PF_IS_CSP )
	{
	    /*
	    ** Caller is a CSP.
	    */
	    int		lkflgs = LKM_VALBLK | LKM_NOQUEUE |
	                         LKM_NO_DOMAIN | LKM_PROC_OWNED;
	    i4		countdown = 0;

	    lockkey.lk_type = LK_CSP | CX_Proc_CB.cxlnx_lk_instkey;
	    lockkey.lk_key1 = nodenumber;
	    lockkey.lk_key2 = 0;

	    /* Loop here, trying to get "CSP lock" */
	    while ( 1 )
	    {
		/* Try for CSP lock (cannot use CXdlm_request yet) */
		osstat = DLMLOCK_SYNC(LKM_EXMODE,
		    (struct lockstatus *)&CX_Proc_CB.cx_csp_req.cx_dlm_ws,
		    lkflgs, &lockkey, 3 * sizeof(i4), NULL, NULL);
# if 0
		if ( DLM_NORMAL != osstat )
		{
		    /* Unexpected error */
		    status = cx_translate_status( osstat );
		    CX_REPORT_STATUS(status);
		    break;
		}

		osstat = CX_Proc_CB.cx_csp_req.cx_dlm_ws.cx_dlm_status;
# endif

		if ( DLM_NOTQUEUED == osstat )
		{
		    /* Turn off NOQUEUE, so next lock request will wait. */
		    lkflgs = LKM_VALBLK | LKM_TIMEOUT |
		             LKM_NO_DOMAIN | LKM_PROC_OWNED;
		    CX_Proc_CB.cx_csp_req.cx_dlm_ws.cx_dlm_timeout =
		     10 * 100; /* 10 seconds in 1/100ths seconds */
		}
		else if ( DLM_TIMEOUT == osstat )
		{
		    if ( 0 == countdown-- )
		    {
			/* A 2nd or subsequent request timed out. */
			(void)TRdisplay(
		  ERx("%@ CSP on node %s waiting for recovery to complete.\n"), 
			  CXnode_name(NULL) );
			/* Only one message per minute. */
			countdown = 5;
		    }
		}
		else if ( DLM_VALNOTVALID == osstat )
		{
		    /* Invalid lock value block is OK in this context. */
		    status = OK;
		}
		else
		{
		    /* Probably DLM_NORMAL, but check */
		    status = cx_translate_status( osstat );
		    break;
		}

		/*
		** Another CSP must already be up, or recovery is
		** being performed for this node.  Check SMS to 
		** see if allocating CSP still lives.
		*/
		if ( 0 != sms->cx_csp_pid && PCis_alive( sms->cx_csp_pid ) )
		{
		    status = E_CL2C15_CX_E_SECONDCSP;
		    break;
		}

		/* 
		** Allocating CSP is not alive, so we can assume reason
		** lock cannot be grabbed is because another CSP is holding
		** a DMN lock on the node, while it performs recovery on
		** behalf of a previous CSP instance for this node.
		** Code will continue to loop, performing this check every
		** ten seconds, until lock is granted, or process is
		** aborted by user.
		*/
	    } /* end while (1) */

	    if ( OK != status )
	    {
		/* Could not get CSP lock.  Redundant CSP, or gross failure. */
		CX_REPORT_STATUS(status);
		break;
	    }

	    /* (re)Initialize lock value block */
	    (void)MEfill( sizeof(csplv),'\0',(PTR)csplv );
	    csplv->cx_csplv_nodenumber = nodenumber;
	    csplv->cx_csplv_state = CX_ST_INIT;

	    /* Force out new CSP lock value */
	    osstat = DLMLOCK_SYNC( LKM_EXMODE,
	      (struct lockstatus *)&CX_Proc_CB.cx_csp_req.cx_dlm_ws, 
	      LKM_VALBLK | LKM_CONVERT | LKM_PROC_OWNED | LKM_NO_DOMAIN,
	      NULL, 0, NULL, NULL );
	    if ( DLM_NORMAL != osstat )
	    {
		/* Unexpected failure */
		break;
	    }
	    CX_Proc_CB.cx_csp_req.cx_old_mode =
	     CX_Proc_CB.cx_csp_req.cx_new_mode = LK_X;
	    CX_Proc_CB.cx_csp_req.cx_lki_flags = CX_LKI_F_PCONTEXT;

	    /*
	    ** Check for "first CSP" status, and get config info
	    ** that must be consistent across the entire cluster.
	    */
	    /* Loop here, trying to get "Config lock" */
	    while (1)
	    {
		lockkey.lk_key1 = 0;
		lockkey.lk_key2 = 0;
		osstat = DLMLOCK_SYNC( LKM_EXMODE,
		    (struct lockstatus *)&CX_Proc_CB.cx_cfg_req.cx_dlm_ws, 
		    LKM_VALBLK | LKM_PROC_OWNED | LKM_NOQUEUE | LKM_NO_DOMAIN, 
		    &lockkey, 3 * sizeof(i4), NULL, NULL );

		if ( DLM_NORMAL == osstat || DLM_VALNOTVALID == osstat )
		{
		    /*
		    ** This CSP is now the "first CSP".  Get critical cluster
		    ** scope configuration info from configuration file.
		    */
		    CX_Proc_CB.cx_cfg_req.cx_old_mode =
		      CX_Proc_CB.cx_cfg_req.cx_new_mode = LK_X;
		    CX_Proc_CB.cx_cfg_req.cx_lki_flags = CX_LKI_F_PCONTEXT;

		    status = cx_get_static_config( cfglv );
		    if (status != OK)
		    {
			break;
		    }
		    cmotype = (i4)(cfglv->cx_cfglv_cmotype);
		    dlmsignal = (i4)(cfglv->cx_cfglv_dlmsignal);
		    imcrail = (i4)(cfglv->cx_cfglv_imcrail);
		    clmtype = (i4)(cfglv->cx_cfglv_clmtype);
		    clmgran = (i4)(cfglv->cx_cfglv_clmgran);
		    clmsize = (i4)(cfglv->cx_cfglv_clmsize);

		    {
			u_i4	i = CX_Proc_CB.cx_ni->cx_node_cnt;

			while ( i != 0 )
			{
			    cfglv->cx_cfglv_known_nodes |= 1 <<
			     (CX_Proc_CB.cx_ni->cx_nodes 
			       [CX_Proc_CB.cx_ni->cx_xref[i]].
			       cx_node_number - 1);
			    i--;
			}
		    }

		    /* Flush out config value */
		    osstat = DLMLOCK_SYNC( LKM_EXMODE,
		      (struct lockstatus *)&CX_Proc_CB.cx_cfg_req.cx_dlm_ws,
		      LKM_VALBLK | LKM_CONVERT | LKM_PROC_OWNED | LKM_NO_DOMAIN,
		      NULL, 0, NULL, NULL );
		    if ( DLM_NORMAL != osstat )
		    {
			/* Unexpected failure */
			break;
		    }

		    /* CSP is "First CSP", and initial Master CSP. */
		    CX_Proc_CB.cx_flags |= CX_PF_FIRSTCSP | CX_PF_MASTERCSP;
		    break;
		}
		else if ( DLM_NOTQUEUED == osstat )
		{
		    /*
		    ** CSP is NOT the "first CSP", get configuration
		    ** from lock value block.
		    **
		    ** CSP may block here for a while, while "First CSP"
		    ** running on another node puts everything in order.
		    */
		    osstat = DLMLOCK_SYNC( LKM_PRMODE,
			(struct lockstatus *)&CX_Proc_CB.cx_cfg_req.cx_dlm_ws,
			LKM_VALBLK | LKM_PROC_OWNED | LKM_NO_DOMAIN,
			&lockkey, 3 * sizeof(i4), NULL, NULL );

		    if ( DLM_NORMAL != osstat )
		    {
			/* Unexpected error. */
			break;
		    }

		    osstat = CX_Proc_CB.cx_cfg_req.cx_dlm_ws.cx_dlm_status;

		    if ( DLM_NORMAL == osstat )
		    {
			CX_Proc_CB.cx_cfg_req.cx_old_mode =
			 CX_Proc_CB.cx_cfg_req.cx_new_mode = LK_S;

			/* Got cfg lock in share. Read values, and move on */
			cmotype = (i4)cfglv->cx_cfglv_cmotype;
			clmtype = (i4)cfglv->cx_cfglv_clmtype;
			clmgran = (i4)(cfglv->cx_cfglv_clmgran);
			clmsize = (i4)(cfglv->cx_cfglv_clmsize);
			dlmsignal = (i4)cfglv->cx_cfglv_dlmsignal;
			imcrail = (i4)cfglv->cx_cfglv_imcrail;

			/*
			** Check to see if node was added to configuration 
			** after cluster was initially started.  This is
			** currently illegal.
			*/
			if ( !( ( 1 << (nodenumber - 1) ) &
			     cfglv->cx_cfglv_known_nodes ) )
			{
			    status = E_CL2C16_CX_E_BADCONFIG;
			}
			break;
		    }
		    else if ( DLM_VALNOTVALID == osstat )
		    {
			/*
			** Lock value was invalidated! Previous "First CSP"
			** candidate must have died.  Try again to be 
			** "First CSP".
			*/
			(void)DLMUNLOCK_SYNC( CX_Proc_CB.cx_cfg_req.cx_lock_id,
			                      NULL, 0 );
			CX_ZERO_OUT_ID(&CX_Proc_CB.cx_cfg_req.cx_lock_id);
			continue;
		    }
		    else
		    {
			/* Unexpected error. */
			break;
		    }
		}
		else
		{
		    /* Unexpected error. */
		    break;
		}
	    } /* end-while */

	    if ( DLM_NORMAL != osstat || OK != status )
	    {
		/* Failure in get configuration step. */
                break;
            }

	    /* 
	    ** Check to see if Shared Mem. Seg. was previously used.
	    ** If so, release any resources held from before.
	    */
# if USE_OPENDLM_RDOM
	    if ( sms->cxlnx_dlm_rdom_id )
	         (void)DLM_RD_DETACH( sms->cxlnx_dlm_rdom_id, 0 );
# endif

	    /* Create new GLC. */
	    osstat = DLM_GRP_CREATE( &CX_Proc_CB.cxlnx_dlm_grp_id, 0 );
	    if ( DLM_NORMAL != osstat )
	    {
		/* Unexpected failure */
		break;
	    }

# if USE_OPENDLM_RDOM
	    /* Create recovery domain */
	    osstat = DLM_RD_ATTACH( installation,
	                            &CX_Proc_CB.cxlnx_dlm_rdom_id, 0 );
	    if ( DLM_NORMAL != osstat )
	    {
		/* Unexpected failure */
		break;
	    }
# endif

	    /* Fill in NCB (SMS) */
	    sms->cx_node_state = CX_ST_INIT;
	    sms->cx_cmo_type = cmotype;
	    sms->cx_clm_type = clmtype;
	    sms->cx_clm_gran = clmgran;
	    sms->cx_clm_size = clmsize;
	    sms->cx_csp_pid = CX_Proc_CB.cx_pid;
	    sms->cx_imc_rail = imcrail;
	    sms->cx_dlm_signal = dlmsignal;
	    sms->cxlnx_dlm_grp_id = CX_Proc_CB.cxlnx_dlm_grp_id;
	    sms->cxlnx_dlm_rdom_id = CX_Proc_CB.cxlnx_dlm_rdom_id;
	    (void)CSw_semaphore( &sms->cx_lsn_sem, CS_SEM_MULTI,
		ERx("cx_lsn_sem") );
	}
	else if ( CX_Proc_CB.cx_flags & CX_PF_NEED_CSP )
	{
	    /*
	    ** Caller is a regular cluster member.
	    */

	    /* Get config parameters from NCB */
	    cmotype = sms->cx_cmo_type;
	    clmtype = sms->cx_clm_type;
	    clmgran = sms->cx_clm_gran;
	    clmsize = sms->cx_clm_size;
	    dlmsignal = sms->cx_dlm_signal;
	    imcrail = sms->cx_imc_rail;

	    /* Attach to GLC established by CSP */
# if USE_OPENDLM_RDOM
	    if ( sms->cxlnx_dlm_grp_id == 0 ||
	         sms->cxlnx_dlm_rdom_id == 0 )
# else
	    if ( sms->cxlnx_dlm_grp_id == 0 )
# endif
	    {
		status = E_CL2C14_CX_E_NOCSP;
		break;
	    }
	    osstat = DLM_GRP_ATTACH( sms->cxlnx_dlm_grp_id, 0 );
	    if ( DLM_NORMAL != osstat )
	    {
		/* Unexpected failure */
                break;
            }
	    /* Remember connection */
	    CX_Proc_CB.cxlnx_dlm_grp_id = sms->cxlnx_dlm_grp_id;

# if USE_OPENDLM_RDOM
	    osstat = DLM_RD_ATTACH( installation, &sms->cxlnx_dlm_rdom_id,
	                            0 );
	    if ( DLM_NORMAL != osstat )
	    {
		/* Unexpected failure */
                break;
	    }
	    /* Remember connection */
	    CX_Proc_CB.cxlnx_dlm_rdom_id = sms->cxlnx_dlm_rdom_id;
# endif
	}
	else
	{
	    /*
	    ** Caller needs to use limited CX facilities, but
	    ** need not be (and cannot be) a full member of
	    ** the cluster.   Certain DMF utilities (RCPSTAT,
	    ** LOGSTAT, LOCKSTAT, et.al.) fall into this class.
	    ** This permits caller to operate "off-line", that
	    ** is before the CSP process has started.
	    **
	    ** Only locks taken at process level may be acquired.
	    ** (See LKRQST.C for code making this decision.)
	    */
	    if ( sms->cx_node_num == nodenumber &&
		 sms->cx_mem_mark == CX_NCB_TAG &&
		 sms->cx_ncb_version == CX_NCB_VER )
	    {
		/* If memory segment initialized, get config from it. */
		cmotype = sms->cx_cmo_type;
		dlmsignal = sms->cx_dlm_signal;
		imcrail = sms->cx_imc_rail;
		clmtype = CX_CLM_NONE;
		clmgran = sms->cx_clm_gran;
		clmsize = sms->cx_clm_size;
	    }
	    else
	    {
		/*
		** Read configuration from config.dat, tolerating the
		** minute risk that config has changed while other
		** cluster members are on-line.
		*/
		status = cx_get_static_config( cfglv );
		if ( status )
		    break;
		cmotype = (i4)(cfglv->cx_cfglv_cmotype);
		dlmsignal = (i4)(cfglv->cx_cfglv_dlmsignal);
		imcrail = (i4)(cfglv->cx_cfglv_imcrail);
		clmtype = CX_CLM_NONE;
		clmgran = (i4)(cfglv->cx_cfglv_clmgran);
		clmsize = (i4)(cfglv->cx_cfglv_clmsize);
	    }

# if USE_OPENDLM_RDOM
	    osstat = DLM_RD_ATTACH( installation, &sms->cxlnx_dlm_rdom_id,
	                            0 );
	    if ( DLM_NORMAL != osstat )
	    {
		/* Unexpected failure */
                break;
	    }
	    /* Remember connection */
	    CX_Proc_CB.cxlnx_dlm_rdom_id = sms->cxlnx_dlm_rdom_id;
# endif
	}

	/* Squirrel away some more config info in PCB. */
	CX_Proc_CB.cx_cmo_type = cmotype;
	CX_Proc_CB.cx_clm_type = clmtype;
	CX_Proc_CB.cx_clm_blksize = (1 << clmgran);
	CX_Proc_CB.cx_clm_blocks =
	 (CX_CLM_SIZE_SCALE * clmsize) / CX_Proc_CB.cx_clm_blksize;
	CX_Proc_CB.cx_imc_rail = imcrail;

	/* Hook in handler which will call ASTpoll */
	signal(dlmsignal, cx_lnx_dlm_poller );

	/* Let DLM know what signal to use for blocking routines */
	(void)DLM_SETNOTIFY( dlmsignal, NULL );

    }	/* end LNX block */
#elif defined(VMS) /* _VMS_ */

    /*
    *****************************************************************
    **	OpenVMS implementation
    *****************************************************************
    */
    {
	i4		 i, lkflgs;
	char		*param;

	status = OK;

	/* Calc. mask to use when gening internal lock keys (resource names) */
	CX_Proc_CB.cxvms_lk_instkey = ((1<<24)*(u_i4)installation[0]) +
	 (1<<16)*(u_i4)installation[1];
	CX_Proc_CB.cxvms_dlm_rsdm_id = 0;
	CX_Proc_CB.cx_flags |= CX_PF_DO_DEADLOCK_CHK;

	/*
	** Save group portion of UIC as the DLM namespace ident, and
	** make sure program is authorized to access SYSTEM locks.
	*/
	{
	    IOSB	iosb;
	    unsigned	aprivs[4];
	    unsigned	cprivs[4];
	    unsigned	iprivs[4];
	    ILE3 itmlst[5] = {
	      { sizeof(CX_Proc_CB.cxvms_uic_group), JPI$_GRP,
	        &CX_Proc_CB.cxvms_uic_group, 0 },
	      { sizeof(cprivs), JPI$_CURPRIV, cprivs, 0 },
	      { sizeof(aprivs), JPI$_AUTHPRIV, aprivs, 0 },
	      { sizeof(iprivs), JPI$_IMAGPRIV, iprivs, 0 },
	      { 0, }
	    };

	    osstat = sys$getjpiw( EFN$C_ENF, &CX_Proc_CB.cx_pid, (void *)0,
				  itmlst, &iosb, NULL, 0 );
	    if (osstat & 1)
		osstat = iosb.iosb$w_status;
	    if ( SS$_NORMAL != osstat )
	    {
		/* Unexpected failure */
		break;
	    }

	    if ( !(PRV$M_SYSLCK & cprivs[0]) )
	    {
		if ( !(PRV$M_SYSLCK & (aprivs[0] | iprivs[0])) )
		{
		    /* SYSLCK is not authorized. */
		    TRdisplay( "%@ %s:%d - No SYSLCK Priv. "
		               "AUTHPRIV=0x%X IMAGPRIV=0x%X\n",
			       __FILE__, __LINE__,
			       aprivs[0], iprivs[0] );
		    status = E_CL2C3C_CX_E_OS_CONFIG_ERR;
		    break;
		}
		/* Enable privilege */
		aprivs[0] |= PRV$M_SYSLCK;
		osstat = sys$setprv( '\001', aprivs, '\0', NULL );
		if ( SS$_NORMAL != osstat )
		{
		    /* Unexpected failure */
		    break;
		}
	    }
	}

	/*
	** Because VMS has the DLM built in, we can use CXdlm earlier
	** than for other platforms.
	*/
	CX_Proc_CB.cx_state = CX_ST_INIT;

	/* Process CSP lock */
	if ( CX_Proc_CB.cx_flags & CX_PF_IS_CSP )
	{
	    /*
	    ** Caller is a CSP.
	    */
	    i4		countdown = 0;

	    /*
	    ** Make sure SMS is not owned by someone else.
	    */
	    if ( 0 != sms->cx_csp_pid && PCis_alive( sms->cx_csp_pid ) )
	    {
		status = E_CL2C15_CX_E_SECONDCSP;
		break;
	    }

	    /* Stuff my group UIC in SMS for CXdlm_request's benefit */
	    sms->cx_csp_pid = CX_Proc_CB.cx_pid;
	    sms->cxvms_csp_uic_group = CX_Proc_CB.cxvms_uic_group;

	    lockkey.lk_type = LK_CSP;
	    lockkey.lk_key1 = nodenumber;
	    lockkey.lk_key2 = 0;

	    CX_INIT_REQ_CB(&CX_Proc_CB.cx_csp_req, LK_X, &lockkey);

	    /* Loop here, trying to get "CSP lock" */
	    lkflgs = CX_F_NOWAIT | CX_F_PCONTEXT |
	             CX_F_NO_DOMAIN | CX_F_NODEADLOCK;
	    while ( 1 )
	    {
		/* Try for CSP lock */
		status = CXdlm_request( lkflgs, &CX_Proc_CB.cx_csp_req, NULL );

		if ( E_CL2C27_CX_E_DLM_NOGRANT == status )
		{
		    /* Turn off NOQUEUE, so next lock request will wait. */
		    lkflgs = CX_F_STATUS | CX_F_PCONTEXT |
		             CX_F_NO_DOMAIN | CX_F_NODEADLOCK;
		}
		else if ( OK == status )
		{
		    if ( lkflgs & CX_F_NOWAIT )
		    {
			/* Lock was granted. */
			break;
		    }

		    /* Request was enqueued */
		    status = CXdlm_wait( 0, &CX_Proc_CB.cx_csp_req, 10 );
		    if ( OK == status )
		    {
			/* Lock granted asynchronously. */
			break;
		    }

		    if ( E_CL2C08_CX_W_SUCC_IVB == status )
		    {
			/* Invalid LVB is OK in this context. */
			status = OK;
			break;
		    }

		    if ( E_CL2C0A_CX_W_TIMEOUT == status )
		    {
			if ( 0 == countdown-- )
			{
			    /* A 2nd or subsequent request timed out. */
			    (void)TRdisplay(
		  ERx("%@ CSP on node %s waiting for recovery to complete.\n"), 
				  CXnode_name(NULL) );
			    /* Only one message per minute. */
			    countdown = 5;
			}
		    }
		}
		else if ( E_CL2C01_CX_I_OKSYNC == status )
		{
		    /* Granted synchronously, no need to wait. */
		    status = OK;
		    break;
		}
		else if ( E_CL2C08_CX_W_SUCC_IVB == status )
		{
		    /* Invalid lock value block is OK in this context. */
		    status = OK;
		    break;
		}

		/* 
		** CSP which allocate SMS is not alive, so we can assume reason
		** lock cannot be grabbed is because another CSP is holding
		** a DMN lock on the node, while it performs recovery on
		** behalf of a previous CSP instance for this node.
		** Code will continue to loop, performing this check every
		** ten seconds, until lock is granted, or process is
		** aborted by user.
		*/
	    } /* end while (1) */

	    if ( OK != status )
	    {
		/* Could not get CSP lock.  Redundant CSP, or gross failure. */
		CX_REPORT_STATUS(status);
		break;
	    }

	    /* (re)Initialize lock value block */
	    (void)MEfill( sizeof(csplv),'\0',(PTR)csplv );
	    csplv->cx_csplv_nodenumber = nodenumber;
	    csplv->cx_csplv_state = CX_ST_INIT;

	    /* Force out new CSP lock value */
	    CX_Proc_CB.cx_csp_req.cx_new_mode = LK_X;
	    status = CXdlm_convert( CX_F_WAIT, &CX_Proc_CB.cx_csp_req );
	    if ( OK != status )
	    {
		/* Unexpected failure */
		break;
	    }

	    /*
	    ** Check for "first CSP" status, and get config info
	    ** that must be consistent across the entire cluster.
	    */

	    lockkey.lk_key1 = 0;
	    CX_INIT_REQ_CB(&CX_Proc_CB.cx_cfg_req, LK_X, &lockkey);

	    /* Loop here, trying to get "Config lock" */
	    while (1)
	    {
		status = CXdlm_request( CX_F_NOWAIT |  
		                        CX_F_PCONTEXT | CX_F_NO_DOMAIN,
					&CX_Proc_CB.cx_cfg_req, NULL );
		if ( OK == status || E_CL2C08_CX_W_SUCC_IVB == status )
		{
		    /*
		    ** This CSP is now the "first CSP".  Get critical cluster
		    ** scope configuration info from configuration file.
		    */
		    status = cx_get_static_config( cfglv );
		    if (status != OK)
		    {
			break;
		    }
		    cmotype = (i4)(cfglv->cx_cfglv_cmotype);
		    clmtype = (i4)(cfglv->cx_cfglv_clmtype);
		    clmgran = (i4)(cfglv->cx_cfglv_clmgran);
		    clmsize = (i4)(cfglv->cx_cfglv_clmsize);

		    {
			u_i4	i = CX_Proc_CB.cx_ni->cx_node_cnt;

			while ( i != 0 )
			{
			    cfglv->cx_cfglv_known_nodes |= 1 <<
			     (CX_Proc_CB.cx_ni->cx_nodes 
			       [CX_Proc_CB.cx_ni->cx_xref[i]].
			       cx_node_number - 1);
			    i--;
			}
		    }

		    /* Flush out config value */
		    CX_Proc_CB.cx_cfg_req.cx_new_mode = LK_X;
		    status = CXdlm_convert( CX_F_WAIT, &CX_Proc_CB.cx_cfg_req );
		    if ( OK != status )
		    {
			/* Unexpected failure */
			break;
		    }

		    /* CSP is "First CSP", and initial Master CSP. */
		    CX_Proc_CB.cx_flags |= CX_PF_FIRSTCSP | CX_PF_MASTERCSP;
		    break;
		}
		else if ( E_CL2C27_CX_E_DLM_NOGRANT == status )
		{
		    /*
		    ** CSP is NOT the "first CSP", get configuration
		    ** from lock value block.
		    **
		    ** CSP may block here for a while, while "First CSP"
		    ** running on another node puts everything in order.
		    */
		    CX_Proc_CB.cx_cfg_req.cx_new_mode = LK_S;
		    status = CXdlm_request( CX_F_WAIT |  
					    CX_F_PCONTEXT | CX_F_NO_DOMAIN,
					    &CX_Proc_CB.cx_cfg_req, NULL );
		    if ( E_CL2C08_CX_W_SUCC_IVB == status )
		    {
			/*
			** 1st CSP died a'borning,
			** try again for 1st CSP role.
			*/
			(void)CXdlm_release( 0, &CX_Proc_CB.cx_cfg_req );
			CX_Proc_CB.cx_cfg_req.cx_new_mode = LK_X;
			continue;
		    }

		    if ( OK != status )
		    {
			/* Unexpected error. */
			break;
		    }

		    /* Got cfg lock in share. Read values, and move on */
		    cmotype = (i4)cfglv->cx_cfglv_cmotype;
		    clmtype = (i4)cfglv->cx_cfglv_clmtype;
		    clmgran = (i4)(cfglv->cx_cfglv_clmgran);
		    clmsize = (i4)(cfglv->cx_cfglv_clmsize);

		    /*
		    ** Check to see if node was added to configuration 
		    ** after cluster was initially started.  This is
		    ** currently illegal.
		    */
		    if ( !( ( 1 << (nodenumber - 1) ) &
			 cfglv->cx_cfglv_known_nodes ) )
		    {
			status = E_CL2C16_CX_E_BADCONFIG;
		    }
		    break;
		}
		else
		{
		    /* Unexpected error. */
		    break;
		}
	    } /* end-while */

	    if ( OK != status )
	    {
		/* Failure in get configuration step. */
                break;
            }

	    /* 
	    ** Check to see if Shared Mem. Seg. was previously used.
	    ** If so, release any resources held from before.
	    */
# if USE_VMS_RDOM
	    if ( sms->cxvms_dlm_rdom_id )
	         (void)DLM_RD_DETACH( sms->cxvms_dlm_rdom_id, 0 );

	    /* Create recovery domain */
	    osstat = DLM_RD_ATTACH( installation,
	                            &CX_Proc_CB.cxvms_dlm_rdom_id, 0 );
	    if ( SS$_NORMAL != osstat )
	    {
		/* Unexpected failure */
		break;
	    }
# endif

	    /* Fill in rest of NCB (SMS) */
	    sms->cx_node_state = CX_ST_INIT;
	    sms->cx_cmo_type = cmotype;
	    sms->cx_clm_type = clmtype;
	    sms->cx_clm_gran = clmgran;
	    sms->cx_clm_size = clmsize;
	    sms->cxvms_dlm_rsdm_id = CX_Proc_CB.cxvms_dlm_rsdm_id;
	    (void)CSw_semaphore( &sms->cx_lsn_sem, CS_SEM_MULTI,
		ERx("cx_lsn_sem") );

	    /* Fill in VMS csid <-> node number array. */
	    {
		CX_NODE_INFO	*pnodeinfo;

		for ( i = 1, pnodeinfo = CX_Proc_CB.cx_ni->cx_nodes;
		      i <= CX_MAX_NODES; pnodeinfo++, i++ )
		{

		    if ( pnodeinfo->cx_node_number )
		    {
			if ( status = CXalter( CX_A_NEED_CSID, (PTR)i ) )
			    break;
		    }
		    else
			/* Not configured */
			sms->cxvms_csids[i] = 0;
		}
		/* Also save our own OS CSID for easy reference */
		sms->cxvms_csids[0] = sms->cxvms_csids[nodenumber];
	    }

	    if ( status )
		break;

	    /* Initialize GLC structures. */
	    {
		CX_GLC_MSG *glm;

		sms->cxvms_csp_glc.cx_glc_glms = 0;
		sms->cxvms_csp_glc.cx_glc_free = 0;
		sms->cxvms_csp_glc.cx_glc_fcnt = 0;
		sms->cxvms_glc_hwm = 0;
		CS_TAS(&sms->cxvms_csp_glc.cx_glc_valid);
		sms->cxvms_csp_glc.cx_glc_triggered = 0;
		/* Messages "live" just after CX_NODE_CB */
		glm = (CX_GLC_MSG *)( sms + 1 );
		sms->cxvms_glm_free = CX_PTR_TO_OFFSET( glm );
		for ( i = 0;
		      i < CX_Proc_CB.cx_transactions_required;
		      i++, glm++ )
		{
		    glm->cx_glm_next = CX_PTR_TO_OFFSET( glm + 1 );
		}
		glm->cx_glm_next = 0;
	    }
	}
	else if ( CX_Proc_CB.cx_flags & CX_PF_NEED_CSP )
	{
	    /*
	    ** Caller is a regular cluster member.
	    */

	    /* Get config parameters from NCB */
	    cmotype = sms->cx_cmo_type;
	    clmtype = sms->cx_clm_type;
	    clmgran = sms->cx_clm_gran;
	    clmsize = sms->cx_clm_size;

	    /* Attach to GLC established by CSP */
# if USE_VMS_RDOM
	    if ( sms->cxlnx_dlm_rdom_id == 0 )
	    {
		status = E_CL2C14_CX_E_NOCSP;
		break;
	    }

	    osstat = DLM_RD_ATTACH( installation, &sms->cxlnx_dlm_rdom_id,
	                            0 );
	    if ( SS$_NORMAL != osstat )
	    {
		/* Unexpected failure */
                break;
	    }
	    /* Remember connection */
	    CX_Proc_CB.cxvms_dlm_rsdm_id = sms->cxvms_dlm_rsdm_id;
# endif
	    /* Add GLC context */
	    if ( !cx_find_glc( CX_Proc_CB.cx_pid, 1 ) )
	    {
		status = E_CL2C12_CX_E_INSMEM;
		break;
	    }
	}
	else
	{
	    /*
	    ** Caller needs to use limited CX facilities, but
	    ** need not be (and cannot be) a full member of
	    ** the cluster.   Certain DMF utilities (RCPSTAT,
	    ** LOGSTAT, LOCKSTAT, et.al.) fall into this class.
	    ** This permits caller to operate "off-line", that
	    ** is before the CSP process has started.
	    **
	    ** Only locks taken at process level may be acquired.
	    ** (See LKRQST.C for code making this decision.)
	    */
	    if ( sms->cx_node_num == nodenumber &&
		 sms->cx_mem_mark == CX_NCB_TAG &&
		 sms->cx_ncb_version == CX_NCB_VER )
	    {
		/* If memory segment initialized, get config from it. */
		cmotype = sms->cx_cmo_type;
		clmtype = CX_CLM_NONE;
		clmgran = sms->cx_clm_gran;
		clmsize = sms->cx_clm_size;
	    }
	    else
	    {
		/*
		** Read configuration from config.dat, tolerating the
		** minute risk that config has changed while other
		** cluster members are on-line.
		*/
		status = cx_get_static_config( cfglv );
		if ( status )
		    break;
		cmotype = (i4)(cfglv->cx_cfglv_cmotype);
		clmtype = CX_CLM_NONE;
		clmgran = (i4)(cfglv->cx_cfglv_clmgran);
		clmsize = (i4)(cfglv->cx_cfglv_clmsize);
	    }

# if USE_VMS_RDOM
	    osstat = DLM_RD_ATTACH( installation, &sms->cxlnx_dlm_rdom_id,
	                            0 );
	    if ( SS$_NORMAL != osstat )
	    {
		/* Unexpected failure */
                break;
	    }
	    /* Remember connection */
	    CX_Proc_CB.cxlnx_dlm_rdom_id = sms->cxlnx_dlm_rdom_id;
# endif
	}

	/* Squirrel away some more config info in PCB. */
	CX_Proc_CB.cx_cmo_type = cmotype;
	CX_Proc_CB.cx_clm_type = clmtype;
	CX_Proc_CB.cx_clm_blksize = (1 << clmgran);
	CX_Proc_CB.cx_clm_blocks =
	 (CX_CLM_SIZE_SCALE * clmsize) / CX_Proc_CB.cx_clm_blksize;

	/* Init data structures supporting CXdlm_get_bli */
	CX_Proc_CB.cxvms_bli_buf[0] = CX_Proc_CB.cxvms_bli_buf[1] = NULL;
	CX_Proc_CB.cxvms_bli_size[0] = CX_Proc_CB.cxvms_bli_size[1] = 0;

    }	/* end _VMS_ block */
#else /* All others */
/*
**	All other platforms
*/
    	status = E_CL2C10_CX_E_NOSUPPORT;
        break;

#endif /*OS SPECIFIC*/

	/*
	** Resume OS independent code.
	*/
        /* Can now use CXdlm for process level locks */
	CX_Proc_CB.cx_state = CX_ST_INIT;

	if ( ( CX_CMO_DLM == CX_Proc_CB.cx_cmo_type ) ||
             ( CX_CMO_SCN == CX_Proc_CB.cx_cmo_type ) )
	{
	    /* Initialize CMO DLM control block */
	    i4			 cmoi, lki;
	    CX_CMO_DLM_CB	*pcmocb;

	    for ( cmoi = 0; cmoi < CX_MAX_CMO; cmoi++ )
	    {
		for ( lki = 0; lki < CX_LKS_PER_CMO; lki++ )
		{
		    pcmocb = &CX_cmo_data[cmoi][lki];
		    CS_ACLR( &pcmocb->cx_cmo_busy );
		    pcmocb->cx_cmo_lkreq.cx_key.lk_type = LK_CMO;
		    pcmocb->cx_cmo_lkreq.cx_key.lk_key1 = cmoi;
		}
	    }
	}

	if ( ( CX_CLM_FILE == CX_Proc_CB.cx_clm_type ) &&
	     ( CX_Proc_CB.cx_flags & (CX_PF_IS_CSP|CX_PF_NEED_CSP) ) )
	{
	    status = cx_msg_clm_file_init();
	    if ( OK != status )
	    {
		TRdisplay( "%@ %s:%d status = 0x%x\n", __FILE__, __LINE__,
			    status );
		/*
		** Use of E_CL2C19_CX_E_BADCONFIG here should not conflict
		** with IMC usage, since CX_CLM_FILE will generally not be
		** used if IMC is available.
		*/
		status = E_CL2C19_CX_E_BADCONFIG;
		break;
	    }
	}

	break;
    } /* endfor */

    /* confirm success */
    if ( OK == status )
	status = cx_translate_status( osstat );

    if ( OK != status )
    {
	/* Not successful, undo any work done locally */
	(void)CXterminate( flags );
    }
    return status;
} /* CXinitialize */


/*
** function used to obtain configuration options from config.dat.
*/
static STATUS
cx_get_static_config( CX_CFG_LKVAL *cfg )
{
    /* 
    ** Moved declaration on cx_indx_to_signum into conditional compile
    ** section as constant initializers not available on Windows.
    */
# if defined (LNX) || defined (axp_osf)
    static u_i1	 cx_indx_to_signum[] = 
     { CX_DLM_DFLT_SIGNAL, SIGUSR1, SIGIO, SIGUSR2 };
# endif     /* LNX || axp_osf */
    STATUS	 status;
    i4		 dlmsignal;
    i4		 cmotype;
    i4		 clmtype;
    i4		 imcrail;

    /* (re)Initialize lock value block */
    (void)MEfill( sizeof(CX_CFG_LKVAL),'\0',(PTR)cfg );

    for ( ; ; )
    {
# if defined (LNX) || defined (axp_osf)
	/* Signal to use for blocking notification */
	status = cx_config_opt( ERx("ii.$.config.dlmsignal"),
				ERx("SIGUSR1 SIGIO SIGUSR2"),
				&dlmsignal );
	if (status != OK)
	{
	    status = E_CL2C18_CX_E_BADCONFIG;
	    break;
	}
	cfg->cx_cfglv_dlmsignal = cx_indx_to_signum[dlmsignal];
# else
	cfg->cx_cfglv_dlmsignal = CX_DLM_DFLT_SIGNAL;
# endif

	/* Specific CMO implementation set? */
	status = cx_config_opt( ERx("ii.$.config.cmotype"), 
				ERx("DLM IMC SCN"),
				&cmotype );
	if (status != OK)
	{
	    break;
	}
	if ( !cmotype )
	{
	    cmotype = CX_CMO_DEFAULT;
	}

	cfg->cx_cfglv_cmotype = (u_i1)cmotype;

	/* Specific CMl implementation set? */
	status = cx_config_opt( ERx("ii.$.config.clmtype"), 
				ERx("FILE IMC"),
				&clmtype );
	if (status != OK)
	{
	    break;
	}
	if ( !clmtype )
	{
	    clmtype = CX_CMO_IMC == cmotype ? CX_CLM_IMC : CX_CLM_FILE;
	}

	cfg->cx_cfglv_clmtype = (u_i1)clmtype;

	switch( clmtype )
	{
	case CX_CLM_FILE:
	    cfg->cx_cfglv_clmgran = (u_i1)12;   /* FIXME s/b DI_RAWIO_ALIGN */
	    cfg->cx_cfglv_clmsize =
	     (u_i1)( 1048576 / CX_CLM_SIZE_SCALE );/* FIXME s/b configurable */
	    break;
	case CX_CLM_IMC:
	    cfg->cx_cfglv_clmgran = (u_i1)9;    /* 2**9 == CX_MSG_FRAGSIZE */
	    cfg->cx_cfglv_clmsize =
	     (u_i1)( CX_MSG_IMC_SEGSIZE / CX_CLM_SIZE_SCALE ); 
	default:
	    break;
	}

	imcrail = 0;
# if defined (axp_osf)
	if ( CX_CMO_IMC == cmotype )
	{
	    /* Specific CMO implementation set? */
	    status = cx_config_opt( ERx("ii.$.config.imcrail"), 
				    ERx("0 1 2 3"),
				&imcrail );
	    if (status != OK)
	    {
		status = E_CL2C19_CX_E_BADCONFIG;
		break;
	    }
	    imcrail = ( imcrail == 0 ) ?
	     CXAXP_IMC_DFLT_RAIL : imcrail - 1; 
	}
# endif
	cfg->cx_cfglv_imcrail = (u_i1)imcrail;
	break;
    }
    return status;
} /* cx_get_static_config */



/*{
** Name: CXjoin()	- Complete process of joining process to cluster.
**
** Description:
**      This routine is called after CXinitialize, and all higher level
**	processing required to join a cluster, including if this is the
**	first CSP, all cold start recovery activity.   It will release/
**	downgrade any locks serializing cluster startup, and transition
**	process from INIT state to JOIN state.
**
** Inputs:
**	flags		- OR'ed special processing flags. Valid bits are:
**				None - this param for future use.
**
** Outputs:
**	none
**
**	Returns:
**
**	    E_CL2C1B_CX_E_BADCONTEXT	- CXinitialize not called.
**	    E_CL2C11_CX_E_BADPARAM	- Called with bad parameter.
**
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	14-may-2001 (devjo01)
**	    Created.
**	10-Jun-2005 (fanch01)
**	    Move shared memory join state change before LVB set to avoid
**	    possible race condition.
*/
STATUS
CXjoin( u_i4 flags )
{
    STATUS	status = OK;

#   ifdef CX_PARANOIA
    /* Called in bad context? */
    if ( CX_ST_INIT != CX_Proc_CB.cx_state )
    {
	status = E_CL2C1B_CX_E_BADCONTEXT;
	return status; /* quick exit */
    }
    if ( flags & ~(CX_F_IS_SERVER) )
    {
	status = E_CL2C11_CX_E_BADPARAM;
	return status; /* quick exit */
    }
#       endif /*CX_PARANOIA*/

    if ( OK == status )
    {
	if ( CX_Proc_CB.cx_flags & CX_PF_IS_CSP )
	{
	    /*
	    ** Set state to JOINED for entire node.
	    */
	    ((CX_CSP_LKVAL *)CX_Proc_CB.cx_csp_req.cx_value)->cx_csplv_state =
	      CX_ST_JOINED;
	    status = CXdlm_convert( CX_F_WAIT, &CX_Proc_CB.cx_csp_req );
	    if ( OK == status )
		CX_Proc_CB.cx_ncb->cx_node_state = CX_ST_JOINED;
	}
    }

    if ( OK == status )
    {
	CX_Proc_CB.cx_state = CX_ST_JOINED;

	/* Downgrade "Config" lock if process thinks it is the "First CSP" */
	if ( CX_Proc_CB.cx_flags & CX_PF_FIRSTCSP )
	{
	    CX_Proc_CB.cx_cfg_req.cx_new_mode = LK_S;
	    status = CXdlm_convert( CX_F_WAIT, &CX_Proc_CB.cx_cfg_req ); 
	}
    }

    return status;
} /* CXjoin */



/*{
** Name: CXterminate()	- Terminate connection to CX sub-facility.
**
** Description:
**      This routine is called to release any CX resources held 
**	by current process.  Must be called after LG & LK termination,
**	and after process has left the Ingres cluster.   If
**	routine returns an error status, there is not much that
**	can be done.   Routine will already have made the final
**	and best attempts to release resources, so an error status
**	(except for bad param) is almost moot.
**
** Inputs:
**	flags		- OR'ed special processing flags. Valid bits are:
**				None - this param for future use.
**
** Outputs:
**	none
**
**	Returns:
**
**	    E_CL2C11_CX_E_BADPARAM	- Called with bad parameter.
**					  No disconnect on this error!
**	    E_CL2C20_CX_E_NODLM		- Vendor DLM became unavailable.
**
**	    Also depending on OS implementation, various
**	    E_CL2C3x_CX_E_OSFAIL	- OS level failure.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    CX DLM calls disabled.
**	    CX CMO calls disabled.
**	    All residual DLM locks released.
**	    All OS resources obtained by CX released.
**	    CX control block for process zeroed out.
**
** History:
**	12-feb-2001 (devjo01)
**	    Created.
**	02-nov-2004 (devjo01)
**	    - Support for OpenDLM recovery domains.
**	11-may-2005 (devjo01)
**	    Release memory grabbed by CXdlm_get_bli.
**	13-jul-2007 (joea)
**	    Use two buffers for DLM info.
*/
STATUS
CXterminate( u_i4 flags )
{
    STATUS	 status, tstatus;
    i4		 osstat;
    i4		 step;

#   ifdef CX_PARANOIA
    /*
    ** Check parameters.
    */
    if ( flags )
    {
	status = E_CL2C11_CX_E_BADPARAM;
	return status; /* quick exit */
    }
#   endif /*CX_PARANOIA*/

    status = OK;

    /*
    **	Start OS specific stuff
    */

#if !defined(conf_CLUSTER_BUILD)

    status = E_CL2C10_CX_E_NOSUPPORT;

#elif	defined(axp_osf)
    /*
    **	Tru64 implementation
    */
    {	/* AXP block */

	/*
	** Figure out what must be undone
	*/
	step = 0;
	for ( ; /* Something to break out of */ ; )
	{
	    if ( (dlm_nsp_t)0 == CX_Proc_CB.cxaxp_nsp )
		break;
	    step = 1;

	    if ( (dlm_rd_id_t)0 == CX_Proc_CB.cxaxp_prc_rd )
		break;
	    step = 2;

	    if ( (dlm_glc_id_t)0 == CX_Proc_CB.cxaxp_glc_id )
		break;
	    step = 4;

	    if ( (dlm_rd_id_t)0 == CX_Proc_CB.cxaxp_grp_rd )
		break;
	    step = 5;

	    /*
	    ** IMC CMO resources are all implicitly released on
	    ** process exit.  DLM CMO resources are all included
	    ** in global release of OS DLM locks for this process.
	    ** Either way, no explicit cleanup is required.
	    */
	    break;
	} /*forever*/

	if ( step >= 2 )
	{
# if 0
	    <<< FIX ME, explicitly release process locks >>

	    /* Toss any remaining DLM locks for this process */
	    osstat = DLM_UNLOCK( (dlm_lkid_t *)0, (dlm_valb_t *)0,
				 DLM_DEQALL );
	    status = cx_translate_status( osstat );
# endif
	}

	/* Now undo everything else in reverse allocation order */
	switch ( step )
	{
	case 5:
	    /* Detach from group recovery domain */
	    osstat = DLM_RD_DETACH( CX_Proc_CB.cxaxp_grp_rd, 0 );
	    tstatus = cx_translate_status( osstat );
	    if ( OK == status ) status = tstatus;

	case 4:
	    /* Detach from group lock container */
	    osstat = DLM_GLC_DETACH();
	    tstatus = cx_translate_status( osstat );
	    if ( OK == status ) status = tstatus;

	case 3:
	case 2:
	    /* Detach from process recovery domain */
	    osstat = DLM_RD_DETACH( CX_Proc_CB.cxaxp_prc_rd, 0 );
	    tstatus = cx_translate_status( osstat );
	    if ( OK == status ) status = tstatus;

	case 1:
	    /* Detach from name space */
	    osstat = DLM_NSLEAVE( &CX_Proc_CB.cxaxp_nsp, 0 );
	    tstatus = cx_translate_status( osstat );
	    if ( OK == status ) status = tstatus;
		
	default:
	    /* Nothing more to do */
	    break;
	} /* switch */

    }	/* end axp block */

#elif defined(LNX)

    /*
    **	Linux OpenDLM implementation
    */
    if ( CX_ST_UNINIT != CX_Proc_CB.cx_state )
    {
	dlm_stats_t	dlmstat;

	/* Explicitly release all locks owned by this process */
	dlmstat = DLM_PURGE( 0, CX_Proc_CB.cx_pid, 0 );
	status = cx_translate_status( dlmstat );

	if ( CX_Proc_CB.cxlnx_dlm_grp_id != 0 )
	{
	    dlmstat = DLM_GRP_DETACH(0);

	    tstatus = cx_translate_status( dlmstat );
	    if ( OK == status ) status = tstatus;
	}

# if USE_OPENDLM_RDOM
	/* Detach from recovery domain */
	if ( CX_Proc_CB.cxlnx_dlm_rdom_id != 0 )
	{
	    dlmstat = DLM_RD_DETACH(CX_Proc_CB.cxlnx_dlm_rdom_id, 0);

	    tstatus = cx_translate_status( dlmstat );
	    if ( E_CL2C2A_CX_E_DLM_RD_DIRTY == tstatus &&
	         !(CX_Proc_CB.cx_flags & CX_PF_IS_CSP) )
		tstatus = OK; /* Only report RD_DIRTY if CSP */
	    if ( OK == status ) status = tstatus;
	}
# endif

    }   /* end Linux block */

#elif defined(VMS) /* _VMS_ */
    /*
    **	VMS implementation
    */
    if ( CX_ST_UNINIT != CX_Proc_CB.cx_state )
    {
	CX_GLC_CTX	*glc;
	CX_GLC_MSG	*glm;
	CX_REQ_CB	*preq;
	i4		nextoff, glmlist; 
	unsigned int	vmsstat;

# if 0  /* FIX-ME This was hanging 4-25-2005 */
	/* Explicitly release all locks owned by this process */
	vmsstat = sys$deq( 0, NULL, 0, LCK$M_DEQALL );
	status = cx_translate_status( vmsstat );
# endif

# if USE_VMS_RDOM
	/* Detach from recovery domain */
	if ( CX_Proc_CB.cxvms_dlm_rsdm_id != 0 )
	{
	    dlmstat = DLM_RD_DETACH(CX_Proc_CB.cxvms_dlm_rsdm_id, 0);

	    tstatus = cx_translate_status( vmsstat );
	    if ( E_CL2C2A_CX_E_DLM_RD_DIRTY == tstatus &&
	         !(CX_Proc_CB.cx_flags & CX_PF_IS_CSP) )
		tstatus = OK; /* Only report RD_DIRTY if CSP */
	    if ( OK == status ) status = tstatus;
	}
# endif
	/* Free  LKIDEF buffers */
	if (CX_Proc_CB.cxvms_bli_buf[0])
	    MEfree(CX_Proc_CB.cxvms_bli_buf[0]);
	if (CX_Proc_CB.cxvms_bli_buf[1])
	    MEfree(CX_Proc_CB.cxvms_bli_buf[1]);

	if ( !(CX_Proc_CB.cx_flags & CX_PF_IS_CSP) )
	{
	    glc = cx_find_glc( CX_Proc_CB.cx_pid, 0 );
	}
	else
	{
	    glc = &CX_Proc_CB.cx_ncb->cxvms_csp_glc;
	}

	if ( glc )
	{
	    /* Trash PID, so this GLC can no longer be found. */
	    glc->cx_glc_threadid.pid = 0;


	    /* Unhook any pending requests */
	    do
	    {
		glmlist = glc->cx_glc_glms;
	    } while ( CScas4( &glc->cx_glc_glms, glmlist, 0 ) );

	    /* Resume any unfortunates with queued requests */
	    if ( 0 != glmlist )
	    {
		/* Walk list */
		nextoff = glmlist;
		while ( nextoff )
		{
		    glm = (CX_GLC_MSG *)CX_OFFSET_TO_PTR( nextoff );
		    preq = (CX_REQ_CB *)CX_OFFSET_TO_PTR( glm->cx_glm_req_off );
		    preq->cx_status = E_CL2C2B_CX_E_DLM_GLC_FAIL;
		    CScp_resume( &glm->cx_glm_caller );
		    nextoff = glm->cx_glm_next;
		}

		/* Return GLMs to shared pool */
		do
		{
		    glm->cx_glm_next = CX_Proc_CB.cx_ncb->cxvms_glm_free;
		} while ( CScas4( &CX_Proc_CB.cx_ncb->cxvms_glm_free,
		                  glm->cx_glm_next, glmlist ) );
		glc->cx_glc_glms = 0;
	    }

	    /* Return local cache of free GLMs to shared pool. */
	    if ( 0 != ( nextoff = glc->cx_glc_free ) )
	    {
		/* Find end of list */
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
	}
    }   /* end VMS block */
#endif /*OS Specific*/

    /*
    ** Resume OS independent code.
    */

    /* Zap CX process status block */
    (void)MEfill( sizeof(CX_PROC_CB), '\0', (PTR)&CX_Proc_CB );
    return status;
} /* CXterminate */




/*{
** Name: CXalter()	- Modify CX facility parameters.
**
** Description:
**      This routine is used to modify various CX facility parameters,
**
** Inputs:
**	attribute	- Attribute to be changed.
**
**			Attributes taking a 0 (off), 1 (on) pvalue.
**		CX_A_NEED_CSP	- Inform CX if process requires a running CSP
**		CX_A_DLM_TRACE  - Enable/Disable DLM tracing.
**		CX_A_CMO_TRACE  - Enable/Disable CMO tracing.
**		CX_A_MSG_TRACE  - Enable/Disable MSG tracing.
**		CX_A_ALL_TRACE  - Enable/Disable all CX traces.
**		CX_A_CSP_ROLE	- Inform CX that process is a CSP.
**		CX_A_IVB_SEEN	- Inform CX that invalid DLM VB seen.
**		CX_A_MASTER_CSP_ROLE - Inform CX that CSP is MASTER CSP.
**		CX_A_QUIET	- Suppress certain TRdisplay calls in CX.
**		CX_A_ONE_NODE_OK - On VMS allow a non-cluster machine
**				  to pretend" its a one node cluster
**				  for testing, otherwise a No-Op.
**
**			Attributes with a function pointer pvalue.
**		CX_A_LK_FORMAT_FUNC - Set address of function used to
**			format Lock keys for trace messages.  Prototype =
**			char *(*pvalue)( LK_LOCK_KEY *, char * );
**		CX_A_ERR_MSG_FUNC - Set address of function used to
**		        display CX error message text. Prototype =
**			void (*pvalue)( i4, PTR, i4, ... ); 
**
**	pvalue		- pointer to new attribute value.  Ptr
**			  value may be just an integer value if
**			  a simple integer is all that is needed.
**
** Outputs:
**	none
**
**	Returns:
**	    OK				- All went well
**	    E_CL2C05_CX_I_DONE		- Operation was redundant.
**	    E_CL2C11_CX_E_BADPARAM	- Called with bad parameter.
**
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	22-jun-2001 (devjo01)
**	    Created.
**	14-may-2004 (devjo01)
**	    Add support for CX_A_QUIET.
**	20-sep-2004 (devjo01)
**	    Add support for CX_A_LK_FORMAT_FUNC and CX_A_ERR_MSG_FUNC.
**	22-apr-2005 (devjo01)
**	    Add CX_A_ONE_NODE_OK to allow cxtest & cxmsgtest to work
**	    on a non-clustered VMS node.
**	04-Oct-2007 (jonj)
**	    New CXalter function CX_A_NEED_CSID to discover VMS
**	    CSID during CX initialization and later, when a node
**	    joins the cluster.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/
STATUS
CXalter( u_i4 attribute, PTR pvalue )
{
# define ALTER_FLAG(flag) \
	if (pvalue) \
	    CX_Proc_CB.cx_flags |= (flag); \
	else \
	    CX_Proc_CB.cx_flags &= ~(flag)

    STATUS	status = OK;

    switch ( attribute )
    {
    case CX_A_NEED_CSP:
	ALTER_FLAG( CX_PF_NEED_CSP );
	break;

    case CX_A_CSP_ROLE:
	ALTER_FLAG( CX_PF_IS_CSP );
	break;

    case CX_A_DLM_TRACE:
    case CX_A_CMO_TRACE:
    case CX_A_MSG_TRACE:
	ALTER_FLAG( 1l << (attribute+12) );
	break;

    case CX_A_ALL_TRACE:
	ALTER_FLAG( CX_PF_DLM_TRACE|CX_PF_CMO_TRACE|CX_PF_MSG_TRACE );
	break;

    case CX_A_IVB_SEEN:
	if ( pvalue )
	{
	    if ( CS_ISSET(&CX_Proc_CB.cx_ncb->cx_ivb_seen) ||
		 !CS_TAS(&CX_Proc_CB.cx_ncb->cx_ivb_seen) )
		status = E_CL2C05_CX_I_DONE;
	}
	else
	{
	    CS_ACLR(&CX_Proc_CB.cx_ncb->cx_ivb_seen);
	}
	break;

    case CX_A_MASTER_CSP_ROLE:
	ALTER_FLAG( CX_PF_MASTERCSP );
	break;

    case CX_A_QUIET:
	ALTER_FLAG( CX_PF_QUIET );
	break;

    case CX_A_ONE_NODE_OK:
	ALTER_FLAG( CX_PF_ONE_NODE_OK );
	break;

    case CX_A_LK_FORMAT_FUNC:
	CX_Proc_CB.cx_lkkey_fmt_func = 
	 (char * (*)( LK_LOCK_KEY *, char * ))pvalue;
	break;

    case CX_A_ERR_MSG_FUNC:
	CX_Proc_CB.cx_error_log_func =
	 (void (*)( i4, PTR, i4, ... ))pvalue;
	break;

    case CX_A_PROXY_OFFSET:
	CX_Proc_CB.cx_proxy_offset = (i4)(SCALARP)pvalue;
	break;

    case CX_A_NEED_CSID:

#if defined(VMS)
	/* Fill in VMS csid <-> node number */

	/* pvalue is the node number of interest */
	{
	    CX_NODE_CB		*sms = CX_Proc_CB.cx_ncb;
	    CX_NODE_INFO	*pnodeinfo;
	    i4			node;
	    i4			osstat;
	    $DESCRIPTOR(nodename,"");
	    ILE3  syi_item_list [] = {
		{ sizeof(i4), SYI$_NODE_CSID, 0, 0 },
		{ 0, 0, }
	    };

	    if ( (node = (i4)pvalue) > 0 && node <= CX_MAX_NODES )
	    {
		/* NB: cx_nodes is 1-based, cxvms_csids is 0-based */
		pnodeinfo = &CX_Proc_CB.cx_ni->cx_nodes[node-1];

		/* Node configured? */
		if ( pnodeinfo->cx_node_number )
		{
		    IOSB iosb;
		    /*
		    ** FIX-ME Following few lines are a kludge to force
		    ** the VMS host name to upper case.
		    */
		    char ucasebuf[CX_MAX_HOST_NAME_LEN+1];
		    STlcopy(pnodeinfo->cx_host_name, ucasebuf,
			    pnodeinfo->cx_host_name_l);
		    ucasebuf[pnodeinfo->cx_host_name_l] = '\0';
		    CVupper(ucasebuf);
		    nodename.dsc$a_pointer = ucasebuf;

		    /* nodename.dsc$a_pointer = pnodeinfo->cx_host_name; */
		    nodename.dsc$w_length =
		     (unsigned short)(pnodeinfo->cx_host_name_l);
		    syi_item_list[0].ile3$ps_bufaddr = &sms->cxvms_csids[node];
		    osstat = sys$getsyiw(EFN$C_ENF, NULL, &nodename,
					 &syi_item_list, &iosb, 0, 0);
		    if (osstat & 1)
			osstat = iosb.iosb$w_status;
		    /*
		    ** The node may be offline during CSP initialization;
		    ** if so, just ignore the error.
		    **
		    ** When the node comes online later, we'll call
		    ** back here to once again try to get the CSID.
		    */
		    if ( osstat == SS$_NOSUCHNODE )
		    {
			sms->cxvms_csids[node] = 0;
			osstat = SS$_NORMAL;
		    }
		}
		else
		    status = E_CL2C19_CX_E_BADCONFIG;
	    }
	    else
		status = E_CL2C11_CX_E_BADPARAM;

	    if ( status == OK )
		status = cx_translate_status( osstat );
	}
#endif /* if defined(VMS) */
	break;

    default:
	status = E_CL2C11_CX_E_BADPARAM;
    }

    return status;
} /* CXalter */



/*{
** Name: CXstartrecovery	- Initiate failover recovery for CX.
**
** Description:
**      This routine is called by the Master CSP to gather the
**	cluster state at the start of recovery. 
**
** Inputs:
**	flags	- currently unused, always pass in zero.
**
** Outputs:
**	none
**
**	Returns:
**
**	    E_CL2C11_CX_E_BADPARAM	- Called with bad parameter.
**
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	22-jun-2001 (devjo01)
**	    Created.
*/
STATUS
CXstartrecovery( u_i4 flags )
{
    STATUS	status;

#   ifdef CX_PARANOIA
    /*
    ** Check parameters.
    */
    if ( flags )
    {
	status = E_CL2C11_CX_E_BADPARAM;
	return status; /* quick exit */
    }
#   endif /*CX_PARANOIA*/

    /*
    ** Clear IVB flag, BEFORE collecting invalidated locks.
    ** This way we can be sure this flag is not left unset,
    ** if a new failure event happens after we create our
    ** collection, but before we clear flag.  We should
    ** not worry about this flag being set redundantly for
    ** the same failure event, after it has been cleared
    ** here, since normal (LK) lock clients will be stalled.
    **
    ** In the worst case, if this flag does get set again,
    ** an extra collect/validate cycle will occur.
    */
    status = CXalter( CX_A_IVB_SEEN, (PTR)0 );

    /*
    **	Start OS specific stuff
    */

# if 0 /* Must completely disable this code, since if we
       ** collect and validate all the failed locks,
       ** without re-establishing the proper lock values,
       ** then the LK layer will not properly detect the
       ** need to report an invalid lock block, and the DMF
       ** layer will not detect the need to refresh the
       ** lock block.    
       ** 
       ** In order to turn this back on, we would need a
       ** mechanism to walk the list of invalidated locks,
       ** and either refresh them with the correct value,
       ** or at least record the invalidated status at
       ** the LK layer (on each surviving node!).
       **
       ** The downside of not doing this at all is that
       ** after a true failure, Ingres will go through a
       ** period where invalid lock values are reported,
       ** and need to be treated as a potential indication
       ** of a new cluster failure.
       */
    {	/* OS specific code */
#if defined(axp_osf)
    /*
    **	Tru64 implementation
    **
    **  Main job for this routine under Tru64, is to gather
    **  the set of DLM resources whose values have been marked
    **  invalid because they were held in an update mode by
    **  a failed node (for a group lock), or process (process
    **	locks only).
    */
    dlm_status_t dlmstat;

    for ( ; /* something to break out of */ ; )
    {
/* # if 0 /* Disable until Tru64 DLM EALREADY bug fixed */
	    dlmstat = DLM_RD_COLLECT( CX_Proc_CB.cxaxp_prc_rd, 
	      &CX_Proc_CB.cxaxp_prc_rdh, (dlm_rd_flags_t)0 );
	    if ( DLM_SUCCESS != dlmstat )
		break;
/* # endif */

	dlmstat = DLM_RD_COLLECT( CX_Proc_CB.cxaxp_grp_rd, 
	  &CX_Proc_CB.cxaxp_grp_rdh, (dlm_rd_flags_t)0 );
	break;
    }

    status = cx_translate_status( dlmstat );
#elif defined(LNX)
    /*
    ** OpenDLM implementation
    */
    dlm_stats_t dlmstat;

    dlmstat = DLM_RD_COLLECT( CX_Proc_CB.cxlnx_dlm_rdom_id, 0 );
    status = cx_translate_status( dlmstat );
#else
    status = OK;
#endif /* axp_osf */
    }
# endif /* Disable all */

    return status;
} /* CXstartrecovery */



/*{
** Name: CXfinishrecovery	- Complete failover recovery for CX.
**
** Description:
**      This routine is called by the Master CSP to finish up whatever
**	work the CX needs to do to finish recovery from a node failure. 
**
** Inputs:
**	flags	- currently unused, always pass in zero.
**
** Outputs:
**	none
**
**	Returns:
**
**	    E_CL2C11_CX_E_BADPARAM	- Called with bad parameter.
**
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	22-jun-2001 (devjo01)
**	    Created.
*/
STATUS
CXfinishrecovery( u_i4 flags )
{
    STATUS	status;

#   ifdef CX_PARANOIA
    /*
    ** Check parameters.
    */
    if ( flags )
    {
	status = E_CL2C11_CX_E_BADPARAM;
	return status; /* quick exit */
    }
#   endif /*CX_PARANOIA*/

    /*
    **	Start OS specific stuff
    */

    {	/* OS specific code */
# if 0 /* Disable ALL. (see notes in CXstartrecovery) */
#if defined(axp_osf)
    /*
    **	Tru64 implementation
    */
    dlm_status_t dlmstat;

    for ( ; /* something to break out of */ ; )
    {
/* # if 0 * Until Tru64 DLM EALREADY bug fixed */
	dlmstat = DLM_RD_VALIDATE( CX_Proc_CB.cxaxp_prc_rd, 
	  &CX_Proc_CB.cxaxp_prc_rdh, (dlm_rd_flags_t)0 );
	if ( DLM_SUCCESS != dlmstat )
	    break;
/* # endif */

	dlmstat = DLM_RD_VALIDATE( CX_Proc_CB.cxaxp_grp_rd, 
	  &CX_Proc_CB.cxaxp_grp_rdh, (dlm_rd_flags_t)0 );
	if ( DLM_MORE_TO_RECOVER == dlmstat )
	    dlmstat = DLM_SUCCESS;

/* # if 1 * Enable until Tru64 DLM EALREADY bug fixed */
	if ( DLM_SUCCESS != dlmstat )
	    break;

	/* FIXME
	**
	** Because Tru64 DLM is complaining about two
	** consecutive calls to dlm_rd_collect, even
	** if they are for separate recovery domains,
	** as a temp KLUDGE, we are doing a late collection
	** of the process locks.   This is wrong, since
	** we may include in this set, locks for nodes
	** which failed after the recovery set was determined.
	*/
	dlmstat = DLM_RD_COLLECT( CX_Proc_CB.cxaxp_prc_rd, 
	  &CX_Proc_CB.cxaxp_prc_rdh, (dlm_rd_flags_t)0 );
	if ( DLM_SUCCESS != dlmstat )
	    break;
	dlmstat = DLM_RD_VALIDATE( CX_Proc_CB.cxaxp_prc_rd, 
	  &CX_Proc_CB.cxaxp_prc_rdh, (dlm_rd_flags_t)0 );
	if ( DLM_MORE_TO_RECOVER == dlmstat )
	    dlmstat = DLM_SUCCESS;
	if ( DLM_SUCCESS != dlmstat )
	    break;
/* # endif * if 1 */
	break;
    }
    status = cx_translate_status( dlmstat );
#elif defined(LNX)
    /*
    ** OpenDLM implementation
    */
    dlm_stats_t dlmstat;

    dlmstat = DLM_RD_VALIDATE( CX_Proc_CB.cxlnx_dlm_rdom_id, 0 );
    status = cx_translate_status( dlmstat );
# else
    status = OK;
# endif /* axp_osf */
# else
    status = OK;
# endif /* Disable all */
    }	/* OS specific code */

    return status;
} /* CXfinishrecovery */

/*{
** Name: cx_default_error_log_func
**
** Description:
**      Default error logging function.  For convenience of Ingres,
**	parameters match that of sc0e_put.
**
**	A "real" routine of this type would lookup a format
**	string using the error code, and interpret the
**	variable portion appropriately.  This default just
**	does a TRdisplay.
**
** Inputs:
**	errorcode  - error code to report.
**	errextra   - Keep this NULL for now.
**	numparams  - Number of parameter pairs to follow.
**	    Each parameter pair is an integer size, followed
**	    by an value, which must be cast as a (PTR), but
**	    may optionally be interpreted as a straight int
**	    value.
**
** Outputs:
**	none
**
**	Returns:
**
**	    none
**
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	20-Sep-2004 (devjo01)
**	    Created.
*/
static void cx_default_error_log_func( i4 errorcode,
                                       PTR errextra, i4 numparams, ... )
{
    TRdisplay( ERx("%@ CX received error code %x.\n"), errorcode );
}

# if defined(CX_ENABLE_DEBUG) || defined(CX_PARANOIA)
/*{
** Name: cx_debug	- CX debug extentions for IIMONITOR.
**
** Description:
**      This routine is registered with the iimonitor code, to support
**	interactive debugging of the CX module.  This code is conditionally
**	compiled, and should not be included in code provided to clients,
**	except at need.
**
**	Currently supported command stings are:
**
**	- "cx show lock <n>.<m>", where "<m>.<n>" is the LK_UNIQUE value
**	  for the DLM lock id as displayed in LOCKSTAT.
**
** Inputs:
**	output_fcn - Function to be used for display.  Must meet the
**		     requirements for and output function to be passed
**		     to TRformat.
**
**	command    - "compressed" command string. (all spaces removed)
**
** Outputs:
**	none
**
**	Returns:
**
**	    OK		- if syntax valid
**	    FAIL	- if bad command passed.
**
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	05-sep-2002 (devjo01)
**	    Created.
**	09-jan-2004 (devjo01)
**	    Allow "short" lock ids.
*/
static STATUS
cx_debug( i4 (*output_fcn)(PTR, i4, char *), char *command )
{
    char	*cp = command + 2;
    char	*pp;
    LK_UNIQUE	 lockid;
    char	 buf[128];

    if ( 0 == MEcmp(cp,"show",4) )
    {
	cp += 4;
	if ( 0 == MEcmp(cp,"lock",4) )
	{
	    cp += 4;
	    do
	    {
		pp = IISTrindex(cp, ".", 0);
		if ( NULL == pp )
		{
		    lockid.lk_uhigh = 0;
		    if ( OK != CVuahxl(cp, &lockid.lk_ulow) )
			break;

		}
		else
		{
		    *pp++ = '\0';
		    if ( '\0' == *pp )
			break;
		    if ( OK != CVuahxl(cp, &lockid.lk_uhigh) )
			break;
		    if ( OK != CVuahxl(pp, &lockid.lk_ulow) )
			break;
		}
		if ( OK == CXdlm_dump_lk( output_fcn, &lockid ) )
		    return OK;
	    } while (0);

	    TRformat( output_fcn, (PTR)1, buf, sizeof(buf) - 1,
		      "Bad lock ID (%s)", cp );
	    return OK;
	}
    }
    return FAIL;
}
# endif
/* EOF: CXINIT.C */
