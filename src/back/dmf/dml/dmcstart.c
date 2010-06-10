/*
** Copyright (c) 1992, 2005 Ingres Corporation
**
*/

/*
NO_OPTIM=dr6_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cm.h>
#include    <cv.h>
#include    <me.h>
#include    <pc.h>
#include    <er.h>
#include    <nm.h>
#include    <tm.h>
#include    <tr.h>
#include    <st.h>
#include    <lo.h>
#include    <sr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lk.h>
#include    <cx.h>
#include    <lg.h>
#include    <pm.h>
#include    <tmtz.h>
#include    <ulf.h>
#include    <scf.h>
#include    <adf.h>
#include    <add.h>
#include    <adp.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmxcb.h>
#include    <dmtcb.h>
#include    <dmucb.h>
#include    <dmmcb.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmm.h>
#include    <dmpp.h>
#include    <dmppn.h>
#include    <dm0m.h>
#include    <dm0p.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <sxf.h>
#include    <dma.h>
#include    <dm0llctx.h>
#include    <dm0s.h>
#include    <dm2t.h>
#include    <dml.h>
#include    <dmfgw.h>
#include    <dmve.h>
#include    <dmfrcp.h>
#include    <dmfinit.h>
#include    <dmftrace.h>
#include    <dmpecpn.h>
#include    <dmftm.h>
#include    <dm2rep.h>
#include    <dmse.h>
#include    <dmst.h>
#include    <dm0pbmcb.h>
#include    <dmfcrypt.h>

/**
** Name: DMCSTART.C - Functions used to start a server.
**
** Description:
**      This file contains the functions necessary to start a server.
**      This file defines:
**    
**      dmc_start_server() -  Routine to perform the start 
**                            server operation.
**      dmc_ui_handler()   -  Routine to handle user interrupts.
**
** History:
**      01-sep-1985 (jennifer)
**          Created for Jupiter.      
**	18-nov-1985 (derek)
**	    Completed code for dmc_start_server().
**	28-feb-1989 (rogerk)
**	    Added shared buffer manager support.  Accept DMC_C_SHARED_BUFMGR
**	    option, and added dmc_cache_name to the dmc_cb struct.
**	    Added options for database and table cache lock array sizes.
**	    Added new parameters to dm0p_allocate().
**	    Added LGalter call to register cache with logging system.
**	10-apr-1989 (rogerk)
**	    Check connection count to buffer manger at startup time.
**	20-apr-1989 (fred)
**	    Add support for obtaining the user defined ADT version identifiers
**	    so that the server can tell if it is legitimate to run with this
**	    recovery system.
**	15-may-1989 (rogerk)
**	    Return LOCK_QUOTA_EXCEEDED if LKcreate_list or LKrequest calls fail
**	    due to lock quotas.
**      14-jul-89 (jennifer)
**          Added code to initialize server to C2 secure.
**	02-feb-1990 (fred)
**	    Added support to initialize ADF's peripheral datatype handling
**	    extension.
**	08-aug-90 (ralph)
**	    Return dma_show_styate entry point in dmc_cb.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.
**	24-sep-1990 (rogerk)
**	    Moved bugfixes from 6.3 to RPLUS.
**	19-nov-1990 (rogerk)
**	    Added svcb_timezone field to hold the timezone in which the DBMS
**	    server operates.  Internal DMF functions will operate in this
**	    timezone while user operations operate in the session's timezone.
**	    This was done as part of the DBMS Timezone changes.
**	10-dec-1990 (rogerk)
**	    Added support for the DMC_C_SCANFACTOR startup parameter.
**      06-jan-92 (jnash)
**          If greater than DM_GFAULT_MAX_PAGES configured group buffer size,
**          issue error and disallow startup.
**	07-apr-1992 (jnash)
**	    Add support for locking the DMF shared cache via 
**	    DMC_C_LOCK_CACHE.
**	13-may-1992 (bryanp)
**	    Added support for DMC_C_INT_SORT_SIZE and svcb_int_sort_size.
**	7-jul-1992 (bryanp)
**	    Prototyping DMF.
**	8-jul-1992 (walt)
**	    Prototyping DMF.
**	22-jul-1992 (bryanp)
**	    Added support for DMC_RECOVERY startup flag.
**	    Zero out the svcb_trace vector when allocating the svcb.
**      25-sep-1992 (stevet)
**          Get timezone default by calling TMtz_init() instead of TMzone().
**      15-oct-1992 (jnash)
**          Reduced logging project.  Add support for II_DBMS_CHECKSUMS
**	    logical, used to enable/disable checksumming.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project:
**	    Moved call to dm0l_allocate to be done before allocating the
**	    Buffer Manager so that the buffer manager can now allocate
**	    a transaction context to use when forcing pages from the cache.
**	8-oct-1992 (bryanp)
**	    Store check_dead_timeout in svcb. Also store group commit parms.
**	    If this is the Recovery Server, call dmf_udt_lkinit() to set and
**	    hold the LK_CONTROL lock value.
**	05-nov-92 (rickh)
**	    Initialize catalog templates.
**	30-November-1992 (rmuth)
**	    File Extend: Add call to adg_fexi routine which defines, for ADF,
**	    the DMF routine to provide information for iitotal_allocated_pages
**	    and iitotal_overflow_pages.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	14-dec-1992 (jnash)
**	    Change default behavior to have page checksums enabled.
**	17-dec-1992 (rogerk)
**	    Added lookup of II_DMF_TRACE variable during server startup.
**	18-jan-1993 (bryanp)
**	    Allow svcb_cp_timer to be set through a characteristic value.
**	16-feb-1993 (rogerk)
**	    Added comments about use of dmm_init_catalog_templates call.
**	15-mar-1993 (rogerk)
**	    Excised some leftover C2 stuff that tracked the creation of
**	    shared memory segments for audit states.
**	26-apr-1993 (bryanp/keving)
**	    6.5 Cluster Support:
**		New arguments to dm0l_allocate for cluster recovery use.
**		Include dmfinit.h to pick up function prototypes.
**		Add have_log parameter to dm0p_allocate.
**	26-apr-1993 (jnash)
**	    Change II_DBMS_CHECKSUMS to be PM parameter 
**	    ii.*.dbms.page_checksums.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**      24-Jun-1993 (fred)
**          Added include of dmpecpn.h to pick up dmpe prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (mikem)
**	    Initialize MO interface to provide dbms performance stats through
**	    queries.
**	23-aug-1993 (bryanp)
**	    Add support for Star's desire to start a "minimal" DMF server. This
**		minimal DMF server supports only the DMT_SORT_COST operation,
**		which the star optimizer thinks it needs.
**	23-aug-1993 (andys)
**          When dmc_ui_handler is called because of a "remove session"
**	    in iimonitor, it is possible that the session control blocks 
**	    for the session to be removed have not yet been set up properly. 
**	    If this is the case then we simply return. [bug 45020]
**	    [was 02-sep-1992 (andys)]
**	31-jan-1994 (bryanp)
**	    B58369, B58370, B58371, B58372, B58373, B58374, B58375:
**	    Check return code from dmf_tmmo_attach_tmperf.
**	    Set dmc->error.err_code if dmm_init_catalog_templates fails.
**	    Log scf_error.err_code if scf_call fails; return proper retcode.
**	    Log LG/LK status if LG/LK call fails.
**	    Return error if request for LK_CONTROL lock fails.
**	    Handle dm0l_deallocate & dm0p_deallocate cleanup consistently.
**	    Ensure that err_code is set if LGalter call fails.
**	18-apr-1994 (jnash)
**	    Call dmf_setup_optim() to establish svcb_load_optim config
**	    param.
**	23-may-1994 (jnash)
** 	    If RCP configured to have fewer than 200 DMF cache pages,
** 	    round up to 200.  Fewer screws up log space reservation.
**	05-may-1995 (cohmi01)
**          Set BATCHMODE in svcb_status if running in 'batch mode', which
**          causes certain modifications to cache behaviour (eg groups).
**	    Added support for DMC_IOMASTER startup flag, for iomaster server.
**	02-jun-1995 (cohmi01)
**	    Add init for readahead control blocks, anchored in svcb.
**	06-mar-1996 (stial01 for bryanp)
**	    Initialize svcb_page_size in the DM_SVCB.
**      06-mar-1996 (stial01)
**          Variable Page Size project:
**          Process dmf buffer cache parameters for 4k,8k,16k,32k,64k pools
**          Process DMC_C_MAXTUPLEN parameter 
**          Added rcp_dmf_cache_size()
**	5-jun-96 (stephenb)
**	    Add code to check replicator transaction queue size and number of
**	    queue management threads (derived from PM in SCF). Code will 
**	    create/attach to segment if qsize <> 0.
**	22-jul-1996 (canor01)
**	    Name the svcb_mutex, svcb_mem_mutex, lctx_mutex and hcb_mutex.
**      01-aug-1996 (nanpr01 for ICL keving & phil.p)
**          Initialise LK callback data structures once we have initialised
**          locking memory.
**          Added support for a server to run the Distributed Multi-Cache  
**          Management (DMCM) protocol via DMC_C_DMCM.
**          DMCM lock "II_DMCM" initialised by RCP and checked by every    
**          server on start-up to ensure that either they all run the      
**          protocol or none of them do.
**	23-sep-1996 (canor01)
**	    Moved global data definitions to dmldata.c.
**	15-oct-96 (mcgem01)
**	    E_DM9056_CHECKSUM_DISABLE and E_DM9057_PM_CHKSM_INVALID
**	    now take SystemCfgPrefix as a parameter.
**	24-Oct-1996 (jenjo02)
**	    Replaced hcb_mutex with hash_mutex, one for each hash entry.
**	    Added hcb_tq_mutex to protect TCB free queue.
**	25-nov-1996 (nanpr01)
**	    Sir 79075 : allow setting the default page size through CBF.
**	17-Jan-1997 (jenjo02)
**	    Replaced svcb_mutex with svcb_dq_mutex (DCB queue) and
**	    svcb_sq_mutex (SCB queue).
**      27-feb-1997 (stial01)
**          IF SINGLE-SERVER mode, we need a separate lock list for SV_DATABASE
**          locks.
**	28-feb-1997 (cohmi01) 
**	    Init hcb_tcblimit based on tcb_hash size. For bug 80050, 80242.
**	09-apr-97 (cohmi01)
**	    Allow readlock, timeout & maxlocks system defaults to be 
**	    configured. #74045, #8616.
**	06-mar-1997 (toumi01)
**	    Move #defines to column 1, as required by many compilers.
**	06-aug-1997 (walro03)
**	    Moved #define statements to col. 1 to prevent compile errors.
**	02-sep-1997 (kinte01)
**	    Add ME_NOTPERM_MASK to MEget_pages call for setting Replicator
**	    shared memory. 
**      14-oct-1997 (stial01)
**          READLOCK is lock mode for Readonly cursors, NOT AN ISOLATION LEVEL.
**          Init c_lk_readlock = DMC_C_READ_SHARE, Added system isolation level.
**	29-aug-1997 (bobmart)
**	    Allow user to configure the types of lock escalations to be
**	    logged to the errlog.
**	07-May-1998 (jenjo02)
**	    Initialize hcb_cache_tcbcount array.
**	13-May-1998 (jenjo02)
**	    WriteBehind threads now allocated per-cache rather than per-server.
**	31-jul-1998 (kinte01)
**	    Add ME_NOTPERM_MASK to other MEget_pages call for setting 
**	    Replicator shared memory and also for attaching to already 
**	    existing shared memory pass value for "pages" instead of 0.
**	    This should not effect Unix as in most cases elsewhere in the
**	    code, we pass a value for "pages" even if we are just 
**	    attaching to already existing memory.
**	    Alpha VMS can sometimes return more pages than the number
**	    of pages allocated due to OS page alignment so before
**	    returning from MEget_pages, allocated_pages is set to pages
**	    if this situation has occurred. If 0 is passed as value for
**	    pages then we will fail the check of allocated_pages = pages
**	    and with shared cache ON any additional servers will fail to
**	    start (bug 92155). 
**      22-Feb-1999 (bonro01)
**          Replace references to svcb_log_id with
**	    dmf_svcb->svcb_lctx_ptr->lctx_lgid
**	02-Mar-1999 (jenjo02)
**	    Pass pointer to log_id instead of log_id in 1st PTR when
**	    establishing the force_abort handler to avoid 64-bit
**	    problems.
**      01-Apr-1999 (stial01)
**          For the recovery server, the maxreclen is not configurable
**	    Pass the address of svcb->svcb_lctx_ptr->lctx_lgid
**      21-apr-1999 (hanch04)
**          Replace STindex with STchr
**      31-aug-1999 (stial01)
**          Initialize svcb_etab_pgsize, svcb_etab_tmpsz in the DM_SVCB.
**      10-sep-1999 (stial01)
**          dmc_start_server, 0 is valid svcb_etab_pgsize
**          Fixed initialization of svcb_etab_tmpsz.
**      28-sep-1999 (stial01)
**          Temporarily disable blob etab page size parameters
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LKcreate_list() prototype.
**	15-Dec-1999 (jenjo02)
**	    Removed DB_DIS_TRAN_ID parm from LKcreate_list() prototype.
**      21-dec-1999 (stial01)
**          Enable VPS etab, make sure server is configured for svcb_etab_pgsize
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      07-sep-2000 (stial01)
**          Disable vps etab.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**      28-nov-2000 (stial01)
**          Enable VPS etab (2K,4K DM_PG_V1 otherwise DM_PG_V2), default 2K
**	08-Jan-2001 (jenjo02)
**	    Deleted DML_SCF. DML_SCB is now embedded in SCF's SCB in
**	    its place. Save offset from SCF's SCB to *DML_SCB in SVCB.
**      01-feb-2001 (stial01)
**          Verify DB* sizes match DMPP* sizes(Variable Page Type SIR 102955)
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**	05-Mar-2002 (jenjo02)
**	    Init svcb_seq for Sequence Generators.
**      16-sep-2003 (stial01)
**          Initialize svcb_etab_type at server startup (SIR 110923)
**      25-nov-2003 (stial01)
**          Added DM723, svcb_etab_logging to set etab logging on/off
**      02-jan-2004 (stial01)
**          Removed temporary trace points DM722, DM723
**	15-Jan-2004 (schka24)
**	    Revised how TCB hash works for partitioned tables project.
**	6-Feb-2004 (schka24)
**	    Init the new name-generator scheme.
**      18-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**          Updated consistency check for LG_MAX_RSZ
**	03-Apr-2004 (devjo01)
**	    Call lk_cback_init if clustered, even if DMCM not set.
**      14-apr-2004 (stial01)
**	    New config parameter max_btree_key, DMC_C_MAXBTREEKEY (SIR 108315)
**      11-may-1004 (stial01)
**          Removed unnecessary max_btree_key
**      06-oct-2004 (stial01)
**          Added DMC_C_PAGETYPE_V5
**      06-oct-2004 (stial01)
**          Added DMC_C_PAGETYPE_V5
**      08-nov-2004 (stial01)
**          Added DMC_C_DOP for DegreeOfParallelism
**	17-nov-2004 (devjo01)
**	    Support for server class node affinity.
**      16-dec-2004 (stial01)
**          Changes for config param system_lock_level
**	06-apr-2005 (devjo01)
**	    Prevent multiple instances of a node affinity class from
**	    starting if NAC uses a private cache, even if on the same node.
**	09-may-2005 (devjo01)
**	    Correct instance where returning with OK status was inadvertently
**	    left within a conditional compile block.
**      11-May-2005 (stial01)
**          Pass LG id_id for this logging process to dmfcsp_class_start
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**	01-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameter to LKrequest avoid random implicit
**	    lock conversions.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dmf_gw?, dm0p_? functions converted to DB_ERROR *
**      16-Jun-2009 (hanal04) Bug 122117
**          When setting SCB_FORCE_ABORT call LKalter() to mark the
**          associated llb with LLB_FORCEABORT. This is used to stop
**          the force aborted session from entering a lock wait which
**          could lead to a LOG/LOCK hang.
**	13-Apr-2010 (toumi01) 122403
**	    Dmc_crypt shared memory segment for encryption keywords.
[@history_template@]...
*/

/*
** globals
*/
GLOBALREF	DMC_REP		*Dmc_rep;
GLOBALREF	DMC_CRYPT	*Dmc_crypt;
# ifdef UNIX
GLOBALREF     i4      Ex_core_enabled;  
# endif

/*
**  Definition of static variables and forward static functions.
*/

/* Get our temporary name generator ID */
static DB_STATUS dmc_start_name_id(
	DM_SVCB *svcb);

    /* User Interrupt handler. */
static STATUS dmc_ui_handler(
    i4	event_id,
    DML_SCB	*scb);

static VOID rcp_dmf_cache_size(
    i4     buffers[],
    i4     cache_flags[]);

static STATUS dmcm_lkinit(
    DMC_CB	*dmc,
    i4     lock_list,
    i4          dmcm,
    i4          create);


/*{
** Name: dmc_start_server - Starts a server.
**
**  INTERNAL DMF call format:    status = dmc_start_server(&dmc_cb);
**
**  EXTERNAL call format:        status = dmf_call(DMC_START_SERVER,&dmc_cb);
**
** Description:
**    The dmc_start_server function handles the initialization of a server.
**    A server is a collect of routines which provides concurrent access to
**    multiple databases by multiple users.  This command causes DMF to 
**    establish a working environment for the server and initialize all
**    server configuration values with their defaults or the value specified
**    in this call.  The configuration values established at this time exist 
**    until altered or until the server is closed.
**
**    If the DMC_C_SHARED_BUFMGR option is specified, then the buffer manager
**    is allocated in shared memory (or connected to if one has already been
**    allocated by another server).  In this case, other servers will be
**    able to share the same buffer cache as this server.  The shared memory
**    cache to connect to is given by the parameter dmc_cache_name.
**
** Inputs:
**      dmc_cb 
**          .type                           Must be set to DMC_CONTROL_CB.
**          .length                         Must be at least sizeof(DMC_CB).
**          .dmc_op_type                    Must be DMC_SERVER_OP.
**	    .dmc_id			    Unique server identifer.
**          .dmc_name                       Must be zero or a unique
**                                          server name.
**	    .dmc_cache_name		    String describing shared cache
**					    name - used only if option
**					    DMC_C_SHARED_BUFMGR is specified.
**          .dmc_flag_mask                  DMC_DEBUG	    - 
**                                          DMC_NODEBUG -
**					    DMC_FSTCOMMIT - use fast commit.
**                                          DMC_C2_SECURE to inidicate if
**                                          running C2 server.
**					    DMC_RECOVERY - this is the recovery
**						server, not a normal server.
**					    DMC_STAR_SVR - this is a star server
**						which needs DMF only in order
**						to call DMT_SORT_COST.
**					    DMC_IOMASTER - this is a server
**						dedicated to I/O functions
**          .dmc_char_array                 Pointer to an area used to input
**                                          an array of characteristics 
**                                          entries of type DMC_CHAR_ENTRY.
**                                          See below for description of
**                                          <dmc_char_array> entries.
**
**          <dmc_char_array> entries are of type DMC_CHAR_ENTRY and
**          must have the following values:
**          char_id                         Must be one of following:
**                                          DMC_C_DATABASE_MAX
**                                          DMC_C_LSESSION
**                                          DMC_C_MEMORY_MAX
**					    DMC_C_TRANSATION_MAX
**					    DMC_C_BUFFERS
**					    DMC_C_GBUFFERS
**					    DMC_C_GCOUNT
**					    DMC_C_TCB_HASH
**					    DMC_C_MLIMIT
**					    DMC_C_FLIMIT
**					    DMC_C_WSTART
**					    DMC_C_WEND
**					    DMC_C_SHARED_BUFMGR
**					    DMC_C_DBCACHE_SIZE
**					    DMC_C_TBLCACHE_SIZE
**					    DMC_C_MAJOR_UDADT
**					    DMC_C_MINOR_UDADT
**					    DMC_C_SCANFACTOR
**					    DMC_C_INT_SORT_SIZE
**					    DMC_C_CHECK_DEAD_INTERVAL
**					    DMC_C_CP_TIMER
**					    DMC_C_DEF_PAGE_SIZE
**					    DMC_C_LOG_ESC_LPR_SC
**					    DMC_C_LOG_ESC_LPR_UT
**					    DMC_C_LOG_ESC_LPT_SC
**					    DMC_C_LOG_ESC_LPT_UT
**					    DMC_C_NSTHREADS
**					    DMC_C_PSORT_ROWS
**					    DMC_C_PSORT_NTHREADS
**					    DMC_C_DOP
**          char_value                      Must be an integer value or
**                                          one of the following:
**                                          DMC_C_ON
**                                          DMC_C_OFF
**                                          DMC_C_READ_ONLY
**					    DMC_C_UNKNOWN
**
** Output:
**      dmc_cb 
**          .dmc_server                     The address of the DMF global area.
**					    Must be pass to a DMC_BEGIN_SESSION
**					    operation.
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM000D_BAD_CHAR_ID
**                                          E_DM000E_BAD_CHAR_VALUE
**                                          E_DM001A_BAD_FLAG
**					    E_DM002A_BAD_PARAMETER
**                                          E_DM002E_BAD_SERVER_NAME
**                                          E_DM002F_BAD_SERVER_ID
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**                                          E_DM0065_USER_INTR
**                                          E_DM0064_USER_ABORT
**					    E_DM007F_ERROR_STARTING_SERVER
**					    E_DM0081_NO_LOGGING_SYSTEM
**                                          E_DM0112_RESOURCE_QUOTA_EXCEEDED
**
**          .error.err_data                 Set to characteristic in error 
**                                          by returning index into 
**                                          characteristic array.
**	    .dmc_char_array[?].char_value   Set to characteristic value if the
**					    value requested is an output value.
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_WARN                       Function completed normally with 
**                                          a termination status which is in 
**                                          dmc_cb.error.err_code.
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dmc_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmc_cb.error.err_code.
** History:
**      01-sep-1985 (jennifer)
**          Created for Jupiter.
**	18-nov-1985 (derek)
**	    Wrote the code.
**	30-sep-1988 (rogerk)
**	    Added startup options for modify limit, free limit, write behind
**	    start and write behind end.
**	 6-feb-1989 (rogerk)
**	    Added shared buffer manager support.  Accept DMC_C_SHARED_BUFMGR
**	    option, and added dmc_cache_name to the dmc_cb struct.
**	    Added options for database and table cache lock array sizes.
**	    Added new parameters to dm0p_allocate().  Call LGalter to register
**	    buffer manager id.
**	10-apr-1989 (rogerk)
**	    Check connection count to buffer manger at startup time.
**	20-apr-1989 (fred)
**	    Added cooperative support for user defined Abstract Datatypes
**	    (UDADT's).  This support consists of filling the appropriate
**	    characteristic values for the the constants DMC_C_MAJOR_UDADT and
**	    DMC_C_MINOR_UDADT.  If these values are requested, then this routine
**	    obtains the values from the recovery system (currently via a lock
**	    value), placing them into the characteristics array with there
**	    requesting characteristics (i.e. the char_value associated with the
**	    char_id DMC_C_*_UDADT is set to the appropriate value).
**	15-may-1989 (rogerk)
**	    Return LOCK_QUOTA_EXCEEDED if LKcreate_list call fails due to
**	    lock quotas.
**      
**      14-jul-89 (jennifer)
**          Added code to initialize serve to C2 secure.
**	07-aug-1989 (fred)
**	    Completed initialization of dmf_svcb.  Previously, the system relied
**	    on the fact that new memory is (usually!) zero.  This was not always
**	    the case.  This code should (now) match that in dmfinit.c.
**	16-jan-1990 (sandyh)
**	    Fixed bug 9595 - WBSTART calculation shoulld be MLIMIT - 15% cache
**	02-feb-1990 (fred)
**	    Added code to inform ADF that DMF is handling peripheral datatype
**	    management.  This happens via ADF's adg_fexi() (function externally
**	    implemented) routine, which is called with the address of the
**	    dmpe_call() routine, which conforms to ADF's peripheral datatype
**	    service routine protocol.
**	08-aug-90 (ralph)
**	    Return dma_show_styate entry point in dmc_cb
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.  Call gateway initialize routine
**	    and set svcb_gateway to indicate the existence of gateway.
**	19-nov-1990 (rogerk)
**	    Added svcb_timezone field to hold the timezone in which the DBMS
**	    server operates.  Internal DMF functions will operate in this
**	    timezone while user operations operate in the session's timezone.
**	    This was done as part of the DBMS Timezone changes.
**	10-dec-1990 (rogerk)
**	    Added support for the DMC_C_SCANFACTOR startup parameter.
**	    Initialize svcb_scanfactor value.  Added support for float-valued
**	    startup parameters.
**	06-jan-92 (jnash)
**          If greater than DM_GFAULT_MAX_PAGES configured group buffer size,
**          issue error and disallow startup.
**	07-apr-1992 (jnash)
**	    Add support for locking the DMF shared cache via a DM0P_LOCK_CACHE
**	    flag to dm0p_allocate and corresponding DMC_C_LOCK_CACHE case.
**	13-may-1992 (bryanp)
**	    Added support for DMC_C_INT_SORT_SIZE and svcb_int_sort_size.
**      21-aug-1992 (fred)
**          Added support for large objects.  Added call to adg_fexi() routine
**	    which defines, for ADF, the DMF routine to provide for storage of
**	    peripheral objects.
**	22-sep-1992 (bryanp)
**	    Added support for DMC_RECOVERY startup flag.
**	    Zero out the svcb_trace vector when allocating the svcb.
**      25-sep-1992 (stevet)
**          Get timezone default by calling TMtz_init() instead of TMzone().
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project:
**	    Moved call to dm0l_allocate to be done before allocating the
**	    Buffer Manager so that the buffer manager can now allocate
**	    a transaction context to use when forcing pages from the cache.
**	14-oct-92 (robf,pholman)
**	    Removed check for Security Audit Proceess which is now
**          obsolete, with its functionality being replaced by SXF.
**	8-oct-1992 (bryanp)
**	    Store check_dead_timeout in svcb. Also store group commit parms.
**	    If this is the Recovery Server, call dmf_udt_lkinit() to set and
**	    hold the LK_CONTROL lock value.
**	05-nov-92 (rickh)
**	    Initialize catalog templates.
**	30-November-1992 (rmuth)
**	    File Extend: Add call to adg_fexi routine which defines, for ADF,
**	    the DMF routine to provide information for iitotal_allocated_pages
**	    and iitotal_overflow_pages.
**	17-dec-1992 (rogerk)
**	    Added lookup of II_DMF_TRACE variable during server startup.
**	    This defines a default set of trace points to set.  Copied it
**	    here from dmf_init so it could be used by the new RCP, but it
**	    will now also read by DBMS servers when they start up.
**	19-nov-1992 (robf)
**	    Replace DMF auditing by SXF auditing. Remove references to
**	    svcb_audit_state, this is now done by SXF.
**	18-jan-1993 (bryanp)
**	    Allow svcb_cp_timer to be set through a characteristic value.
**	15-mar-1993 (rogerk)
**	    Excised some leftover C2 stuff that tracked the creation of
**	    shared memory segments for audit states.
**	26-jul-1993 (mikem)
**	    Initialize MO interface to provide dbms performance stats through
**	    queries.
**	23-aug-1993 (bryanp)
**	    Add support for Star's desire to start a "minimal" DMF server. This
**		minimal DMF server supports only the DMT_SORT_COST operation,
**		which the star optimizer thinks it needs. Star indicates this
**		operation through the DMC_STAR_SVR flag.
**	25-oct-93 (vijay)
**	 	Correct type cast.
**	10-oct-93 (swm)
**	    Bug #56440.
**	    Initialise svcb->svcb->tcb_id to 1. This value is incremented
**	    and used each time a new DMP_TCB is allocated, to provide a
**	    per DMP_TCB unique lock event value.
**	    Bug #56441.
**	    Changed type of abort_handler to PTR and extract its value from
**	    the new char_pvalue rather than char_value of the characteristic
**	    array element.
**	    Pass both svcb->svcb_log_id and abort_handler to LGalter() via
**	    a buffer (a two element PTR array).
**	31-jan-1994 (bryanp)
**	    B58369, B58370, B58371, B58372, B58373, B58374, B58375:
**	    Check return code from dmf_tmmo_attach_tmperf.
**	    Set dmc->error.err_code if dmm_init_catalog_templates fails.
**	    Log scf_error.err_code if scf_call fails; return proper retcode.
**	    Log LG/LK status if LG/LK call fails.
**	    Return error if request for LK_CONTROL lock fails.
**	    Handle dm0l_deallocate & dm0p_deallocate cleanup consistently.
**	    Ensure that err_code is set if LGalter call fails.
**	18-apr-1994 (jnash)
**	    Call dmf_setup_optim() to establish svcb_load_optim config
**	    param.
**	23-may-1994 (jnash)
** 	    If RCP configured to have fewer than 200 DMF cache pages,
** 	    round up to 200.  Fewer screws up log space reservation.
**	05-may-1995 (cohmi01)
**          Set BATCHMODE in svcb_status if running in 'batch mode', which
**          causes certain modifications to cache behaviour (eg groups).
**	    Added support for DMC_IOMASTER startup flag, for iomaster server.
**	02-jun-1995 (cohmi01)
**	    Add init for readahead control blocks, anchored in svcb.
**	06-mar-1996 (stial01 for bryanp)
**	    Initialize svcb_page_size in the DM_SVCB.
**      06-mar-1996 (stial01)
**          Variable Page Size project:
**          Process dmf buffer cache parameters for 4k,8k,16k,32k,64k pools
**          Process DMC_C_MAXTUPLEN parameter
**	06-may-1996 (nanpr01)
**	    Added sanity check to make sure duplicate defines are changed
**	    correctly.
**	5-jun-96 (stephenb)
**	    Add code to check replicator transaction queue size and number of
**	    queue management threads (derived from PM in SCF). Code will 
**	    create/attach to segment if qsize <> 0.
**	09-jul-1996 (thaju02)
**	    Ensure that maxtuplen can not exceed DM_TUPLEN_MAX.
**	    added this constant due to iirelation.relwid datatype restriction
** 	    of signed i2.  
**	22-jul-1996 (canor01)
**	    Name the svcb_mutex, svcb_mem_mutex, lctx_mutex and hcb_mutex.
**      01-aug-1996 (nanpr01 for ICL keving and phil.p)
**          Initialise LK callback data structures once we have initialised
**          locking memory.
**          Introduced concept of DMCM lock, key "II_DMCM", initialised
**          by RCP and checked by each DBMS, to check for consistent
**          usage of the Distrbuted Multi-Cache Management (DMCM) protocol 
**          by servers not sharing a common cache.
**	24-Oct-1996 (jenjo02)
**	    Replaced hcb_mutex with hash_mutex, one for each hash entry.
**	    Added hcb_tq_mutex to protect TCB free queue.
**	25-nov-1996 (nanpr01)
**	    Sir 79075 : allow setting the default page size through CBF.
**	09-dec-1996 (nanpr01)
**	    Bug 79703 : fix max_tuple_length check. If zero, set it based on
**	    page size or otherwise.
**      27-feb-1997 (stial01)
**          IF SINGLE-SERVER mode, we need a separate lock list for SV_DATABASE
**          locks.
**	28-feb-1997 (cohmi01) 
**	    Init hcb_tcblimit based on tcb_hash size. For bug 80050, 80242.
**	09-apr-97 (cohmi01)
**	    Allow readlock, timeout & maxlocks system defaults to be 
**	    configured. #74045, #8616.
**	14-apr-1997 (nanpr01)
**	    We currently donot allow to disable group buffers. But all of
**	    us in development feels that why not ? So let us make zero
**	    a valid value.
**	29-aug-1997 (bobmart)
**	    Allow user to configure the types of lock escalations to be
**	    logged to the errlog.
**	25-nov-97 (stephenb)
**	    add new replicator locking parameters to svcb at startup.
**	18-feb-98 (inkdo01)
**	    Add support for parallel sort threads project - alloc/init
**	    sort threads pool header.
**	26-mar-98 (nanpr01)
**	    Add support for defining  the default etab structure.
**	13-apr-98 (inkdo01)
**	    Switched parallel sort to use factotum threads, rather than 
**	    thread pool. Dropped DM_STHCB structure, but added svcb_stbsize.
**	13-May-1998 (jenjo02)
**	    WriteBehind threads now allocated per-cache rather than per-server.
**	26-may-1998 (nanpr01)
**	    Added the config parameters for configuring the exchange
**	    buffer for parallel index building.
**	28-jul-1998 (rigka01)
**	    Cross integrate fix for bug 90691 from 1.2 to 2.0 code line
**	    In highly concurrent environment, temporary file names may not
**	    be unique so improve algorithm for determining names
**	    Handle svcb_DM2F_TempTblCnt_mutex and svcb_DM2F_TempTblCnt
**	02-Mar-1999 (jenjo02)
**	    Pass pointer to log_id instead of log_id in 1st PTR when
**	    establishing the force_abort handler to avoid 64-bit
**	    problems.
**      12-oct-2000 (stial01)
**          Added consistency check for DB_PAGE_OVERHEAD_MACRO 
**	28-Feb-2002 (jenjo02)
**	    Made LK_MULTITHREAD an attribute of svcb_lock_list,
**	    svcb_ss_lock_list instead of an abused LKrequest/release
**	    flag.
**      11-jul-2002 (stial01)
**          Added consistency check for LG_MAX_RSZ
**	15-Nov-2002 (jenjo02)
**	    If too many group buffers (E_DM9428), fail startup
**	    by placing the "break" properly.
**	16-May-2003 (jenjo02)
**	    Don't call lk_cback_init() unless running DMCM.
**      20-sept-2004 (huazh01)
**          Pull out the fix for b90691 as it has been 
**          rewritten. 
**          INGSRV2643, b111502. 
**	15-Jan-2004 (schka24)
**	    Redo tcb hash scheme for partitioned tables.
**	5-Mar-2004 (schka24)
**	    Tcb free list is now a real queue, fix here.
**	    Add dmf_tcb_limit, deprecate dmf_hash_size.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**	03-Nov-2004 (jenjo02)
**	    Set SVCB_IS_MT if OS-threaded.
**	24-Feb-2005 (thaju02)
**	    c_pind_bsize to be at least 100 rows. (B113970)
**	    Also set min for c_stbsize.
**	22-Apr-2005 (fanch01)
**	    Add dmfcsp_class_start call for cluster dbms class failover.
**	06-May-2005 (fanch01)
**	    Wrap dmfcsp_class_start call and related code in
**	    conf_CLUSTER_BUILD.  Breaks non-cluster builds because it pulls
**	    in symbols from dmfcsp which aren't available.
**      30-mar-2005 (stial01)
**          Init svcb_pad_bytes (dbms config param dmf_pad_bytes)
**	04-May-2005 (wanfr01)
**	    Bug 114452, Problem INGREP175
**	    Named Dmc rep Mutex.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Include SCB ShortTerm memory size in SCF allocation
**	    hint.
**	9-Nov-2005 (schka24)
**	    Set svcb status flag if II_DBMS_REP is set, meaning that we are
**	    to never replicate.
**	6-Jan-2006 (kschendel)
**	    Update call to dm0p_allocate.
**	13-Jun-2006 (jenjo02)
**	    Figure largest possible Btree Leaf entry size and
**	    TRdisplay it along with DM1B_KEYLENGTH and LG_MAX_RSZ,
**	    just useful information.
**	27-july-06 (dougi)
**	    Add initialization of svcb_sortcpu_factor & svcb_sortio_factor.
**      30-Aug-2006 (kschendel)
**          Expose mwrite_pages as a config parameter (dmf_build_pages).
**	6-oct-2006 (dougi)
**	    Turn sortcpu/sortio into multiplicative factors in which 
**	    user value is applied to default value & doesn't simply replace it.
**	15-May-2007 (toumi01)
**	    For supportability add process info to shared memory.
**	5-Nov-2007 (wanfr01)
**	    SIR 119419
**	    Add support for core_enabled
**	04-Apr-2008 (jonj)
**	    Set SVCB_ULOCKS in svcb_status if Update-mode locking is
**	    supported.
**	17-Apr-2008 (kibro01) b120276
**	    Initialise ADF_CB structure
**	12-Nov-2008 (jonj)
**	    SIR 120874: dmf_udt_lkinit() now takes DB_ERROR*
**	9-sep-2009 (stephenb)
**	    Add dmf_last_id FEXI
**	5-Nov-2009 (kschendel) SIR 122757
**	    Rename setup-optim to setup-directio.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: LGshow(LG_S_XID_ARRAY) to extract info about
**	    LG xid array into svcb_xid_array_ptr, _size, _lg_id_max.
**	    Add page types V6, V7
**	08-Mar-2010 (thaju02)
**	    Removed max_tuple_length.
**	16-Mar-2010 (troal01)
**	    Added dmf_get_srs to ADF FEXI.
*/

DB_STATUS
dmc_start_server(
DMC_CB    *dmc_cb)
{
    DMC_CB		*dmc = dmc_cb;
    DM_SVCB		*svcb;
    DMP_HCB		*hcb;
    DM_DATA		*name = 0;
    DMC_CHAR_ENTRY	*chr = 0;
    i4		i;
    i4		chr_count;
    i4		c_rep_qman = 0;
    i4		c_rep_qsize = 0;
    i4		c_rep_salock = 0;
    i4		c_rep_iqlock = 0;
    i4		c_rep_dqlock = 0;
    i4		c_rep_dtmaxlock = 0;
    i4		c_database_max = 1;
    i4		c_session_max = 1;
    i4		c_memory_max = 512;
    i4		c_transaction_max = 2;
    i4		c_tcb_hash = 0;
    i4		c_tcb_limit = 0;
    i4		c_dbcache_size = DM_DBCACHE_SIZE;
    i4		c_tblcache_size = DM_TBLCACHE_SIZE;
    i4		c_buffers[DM_MAX_CACHE];
    i4		c_gbuffers[DM_MAX_CACHE];
    i4		c_gcount[DM_MAX_CACHE];
    i4		c_wbstart[DM_MAX_CACHE];
    i4		c_wbend[DM_MAX_CACHE];
    i4		c_writebehind[DM_MAX_CACHE];
    i4		c_flimit[DM_MAX_CACHE];
    i4		c_mlimit[DM_MAX_CACHE];
    i4          c_cache_flags[DM_MAX_CACHE];
    i4                  cache_ix;
    i4		c_shared_cache = FALSE;
    i4		c_cache_lock = FALSE;
    i4		c_check_dead_interval = 5;
    i4		c_cp_timer = 0;
    i4		c_gc_interval = 20;
    i4		c_gc_numticks = 5;
    i4		c_gc_threshold = 1;
    i4		c_lk_readlock = DMC_C_READ_SHARE;
    i4		c_lk_maxlocks = DM_DEFAULT_MAXLOCKS;
    i4		c_lk_waitlock = DM_DEFAULT_WAITLOCK;
    i4             c_isolation = DMC_C_SERIALIZABLE;
    i4		c_lk_level = DMC_C_SYSTEM;
    i4             c_logreadnolock = FALSE;
    i4		c_def_page_size = DM_PG_SIZE;
    i4          c_etab_page_size = DM_PG_SIZE;
    i4		c_blob_etab_struct = DB_HASH_STORE;
    i4		c_stbsize = DMST_BSIZE;
    i4		c_strows = DMST_STROWS;
    i4		c_stnthreads = DMST_STNTHREADS;
    f4		c_stcpufact = 1.0 / (f4)DMSE_SORT_CPUFACTOR;
    f4		c_stiofact = 1.0 / (f4)DMSE_SORT_IOFACTOR;
    i4		c_pind_bsize =  DM_DEFAULT_EXCH_BSIZE;
    i4		c_pind_nbuffers = DM_DEFAULT_EXCH_NBUFF;
    i4		c_dop = SCB_DOP_DEFAULT;
    i4		c_pad_bytes = 0;
    i4		c_crypt_maxkeys = 0;
    bool		gc_numticks_changed = FALSE;
    bool		gc_threshold_changed = FALSE;
    bool		log_allocated = FALSE;
    bool		bufmgr_allocated = FALSE;
    i4			star_server;
    i4			major_id_index = -1;
    i4			minor_id_index = -1;
    char		*c_cache_name = DM_DEFAULT_CACHE_NAME;
    i4		c_mwrite_blocks = DM_MWRITE_PAGES;
    double		c_scanfactor[DM_MAX_CACHE];
    i4		c_int_sort_size = 150000;
    DB_STATUS		status = E_DB_ERROR;
    i4		err_code;
    STATUS		stat;
    DB_STATUS		local_err_code;
    CL_ERR_DESC		sys_err;
    SCF_CB		scf_cb;
    PTR		abort_handler = (PTR)0;
    i4		flags;
    i4		gateway_present = 0;
    i4		gw_xacts = 0;
    LG_LGID	recovery_log_id = 0;
    i4		length;	
    i4		lgd_status;
    i4		c_numrahead;
    i4		segment_ix;
    i4		c_dmcm = FALSE;
    char	sem_name[CS_SEM_NAME_LEN];
    u_i2	c_lk_log_esc_lpr_sc = 0;
    u_i2	c_lk_log_esc_lpr_ut = 0;
    u_i2	c_lk_log_esc_lpt_sc = 0;
    u_i2	c_lk_log_esc_lpt_ut = 0;
    i4		lg_max1;
    i4		lg_max2;
    i4		clustered;
    i4		pgsize,pgtype;
    i4		local_node_number;
    char	*svr_class_name;
    char	*rep_var = NULL;
    i4		MaxLeafLength, LeafLength;
    DB_ERROR	local_dberr;
    i4		config_pgtypes;

    CLRDBERR(&dmc->error);
    config_pgtypes = (SVCB_CONFIG_V1 | SVCB_CONFIG_V2 | SVCB_CONFIG_V3 |
	SVCB_CONFIG_V4 | SVCB_CONFIG_V5 | SVCB_CONFIG_V6 | SVCB_CONFIG_V7);

#ifdef DEBUG
    TRdisplay("V1 over %d %d, line %d %d, hdr %d %d, \n",
DB_VPT_PAGE_OVERHEAD_MACRO(TCB_PG_V1), DMPPN_VPT_OVERHEAD_MACRO(TCB_PG_V1),
DB_VPT_SIZEOF_LINEID(TCB_PG_V1), DM_VPT_SIZEOF_LINEID_MACRO(TCB_PG_V1),
DB_VPT_SIZEOF_TUPLE_HDR(TCB_PG_V1), DMPP_VPT_SIZEOF_TUPLE_HDR_MACRO(TCB_PG_V1));

    TRdisplay("V2 over %d %d, line %d %d, hdr %d %d, \n",
DB_VPT_PAGE_OVERHEAD_MACRO(TCB_PG_V2), DMPPN_VPT_OVERHEAD_MACRO(TCB_PG_V2),
DB_VPT_SIZEOF_LINEID(TCB_PG_V2), DM_VPT_SIZEOF_LINEID_MACRO(TCB_PG_V2),
DB_VPT_SIZEOF_TUPLE_HDR(TCB_PG_V2), DMPP_VPT_SIZEOF_TUPLE_HDR_MACRO(TCB_PG_V2));

    TRdisplay("V3 over %d %d, line %d %d, hdr %d %d, \n",
DB_VPT_PAGE_OVERHEAD_MACRO(TCB_PG_V3), DMPPN_VPT_OVERHEAD_MACRO(TCB_PG_V3),
DB_VPT_SIZEOF_LINEID(TCB_PG_V3), DM_VPT_SIZEOF_LINEID_MACRO(TCB_PG_V3),
DB_VPT_SIZEOF_TUPLE_HDR(TCB_PG_V3), DMPP_VPT_SIZEOF_TUPLE_HDR_MACRO(TCB_PG_V3));

    TRdisplay("V4 over %d %d, line %d %d, hdr %d %d, \n",
DB_VPT_PAGE_OVERHEAD_MACRO(TCB_PG_V4), DMPPN_VPT_OVERHEAD_MACRO(TCB_PG_V4),
DB_VPT_SIZEOF_LINEID(TCB_PG_V4), DM_VPT_SIZEOF_LINEID_MACRO(TCB_PG_V4),
DB_VPT_SIZEOF_TUPLE_HDR(TCB_PG_V4), DMPP_VPT_SIZEOF_TUPLE_HDR_MACRO(TCB_PG_V4));
#endif

    if (DB_VPT_SIZEOF_LINEID(TCB_PG_V1) != DM_VPT_SIZEOF_LINEID_MACRO(TCB_PG_V1)
	|| 
	DB_VPT_SIZEOF_TUPLE_HDR(TCB_PG_V1) != DMPP_VPT_SIZEOF_TUPLE_HDR_MACRO(TCB_PG_V1) ||
	DB_VPT_PAGE_OVERHEAD_MACRO(TCB_PG_V1) != DMPPN_VPT_OVERHEAD_MACRO(TCB_PG_V1) ||
	DB_VPT_SIZEOF_LINEID(TCB_PG_V2) != DM_VPT_SIZEOF_LINEID_MACRO(TCB_PG_V2) || 
	DB_VPT_SIZEOF_TUPLE_HDR(TCB_PG_V2) != DMPP_VPT_SIZEOF_TUPLE_HDR_MACRO(TCB_PG_V2) ||
	DB_VPT_PAGE_OVERHEAD_MACRO(TCB_PG_V2) != DMPPN_VPT_OVERHEAD_MACRO(TCB_PG_V2) ||
    	DB_VPT_SIZEOF_LINEID(TCB_PG_V3) != DM_VPT_SIZEOF_LINEID_MACRO(TCB_PG_V3)
	|| 
	DB_VPT_SIZEOF_TUPLE_HDR(TCB_PG_V3) != DMPP_VPT_SIZEOF_TUPLE_HDR_MACRO(TCB_PG_V3) ||
	DB_VPT_PAGE_OVERHEAD_MACRO(TCB_PG_V3) != DMPPN_VPT_OVERHEAD_MACRO(TCB_PG_V3) ||
	DB_VPT_SIZEOF_LINEID(TCB_PG_V4) != DM_VPT_SIZEOF_LINEID_MACRO(TCB_PG_V4) || 
	DB_VPT_SIZEOF_TUPLE_HDR(TCB_PG_V4) != DMPP_VPT_SIZEOF_TUPLE_HDR_MACRO(TCB_PG_V4) ||
	DB_VPT_PAGE_OVERHEAD_MACRO(TCB_PG_V4) != DMPPN_VPT_OVERHEAD_MACRO(TCB_PG_V4))
    {
	SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
	return(E_DB_ERROR);
    }

    if (!CS_is_mt()) c_stnthreads = 0;	/* Ingres threads default for
					** parallel sort processing */

    clustered = CXcluster_enabled();
    if ( clustered )
        local_node_number = CXnode_number(NULL);

    svr_class_name = PMgetDefault(3); /* Ideally class name would have
                                         been passed down from SCF. */

    /* Init dmf parameters maintained for each page size */
    for (cache_ix = 0; cache_ix < DM_MAX_CACHE; cache_ix++)
    {
	c_buffers[cache_ix] = 0;
	c_gbuffers[cache_ix] = 0;
	c_gcount[cache_ix] = 0;
	c_wbstart[cache_ix] = 0;
	c_wbend[cache_ix] = 0;
	c_flimit[cache_ix] = 0;
	c_mlimit[cache_ix] = 0;
	c_scanfactor[cache_ix] = 0;
	c_cache_flags[cache_ix] = 0;
	/* WriteBehind defaults to ON */
	c_writebehind[cache_ix] = 1;
    }

    if ( !(dmc->dmc_flags_mask & (DMC_RECOVERY|DMC_STAR_SVR)) )
	c_crypt_maxkeys = 1; /* by default allocate 1 page of encryption keys */

    for (;;)
    {
	/*	Verify control block parameters. */

	if (dmf_svcb)
	{
	    SETDBERR(&dmc->error, 0, E_DM006F_SERVER_ACTIVE);
	    break;
	}
	if (dmc->dmc_op_type != DMC_SERVER_OP)
	{
	    SETDBERR(&dmc->error, 0, E_DM000C_BAD_CB_TYPE);
	    break;
	}
	if ( dmc->dmc_flags_mask &
		~(DMC_C2_SECURE | DMC_DEBUG | DMC_NODEBUG | DMC_FSTCOMMIT |
		  DMC_RECOVERY  | DMC_STAR_SVR | DMC_IOMASTER |
		  DMC_BATCHMODE | DMC_DMCM | DMC_CLASS_NODE_AFFINITY) )
	{
	    SETDBERR(&dmc->error, 0, E_DM001A_BAD_FLAG);
	    break;
	}
	star_server = (dmc->dmc_flags_mask & DMC_STAR_SVR) != 0;
	if (dmc->dmc_name.data_address)
	{
	    name = &dmc->dmc_name;
	    if (dmc->dmc_name.data_in_size == 0)
	    {
		SETDBERR(&dmc->error, 0, E_DM002E_BAD_SERVER_NAME);
		break;
	    }
	}
	if (dmc->dmc_char_array.data_address)
	{
	    chr = (DMC_CHAR_ENTRY *)dmc->dmc_char_array.data_address;
	    chr_count = dmc->dmc_char_array.data_in_size / 
                            sizeof (DMC_CHAR_ENTRY);
	    for (i = 0; i < chr_count; i++)
	    {
		switch (chr[i].char_id)
		{
		case DMC_C_DATABASE_MAX:
		    c_database_max = chr[i].char_value;
		    continue;

		case DMC_C_LSESSION:
		    c_session_max = chr[i].char_value;
		    continue;

		case DMC_C_MEMORY_MAX:
		    c_memory_max = chr[i].char_value;
		    continue;

		case DMC_C_TRAN_MAX:
		    c_transaction_max = chr[i].char_value;
		    continue;

		case DMC_C_BUFFERS:
		    c_buffers[0] = chr[i].char_value;
		    continue;

		case DMC_C_GBUFFERS:
		    c_gbuffers[0] = chr[i].char_value;
		    continue;

		case DMC_C_GCOUNT:
		    c_gcount[0] = chr[i].char_value;
		    continue;

		case DMC_C_TCB_HASH:
		    c_tcb_hash = chr[i].char_value;
		    continue;

		case DMC_C_TCB_LIMIT:
		    c_tcb_limit = chr[i].char_value;
		    continue;

		case DMC_C_ABORT:
		    abort_handler = chr[i].char_pvalue;
		    continue;

		case DMC_C_WSTART:
		    c_wbstart[0] = chr[i].char_value;
		    continue;

		case DMC_C_WEND:
		    c_wbend[0] = chr[i].char_value;
		    continue;

		case DMC_C_MLIMIT:
		    c_mlimit[0] = chr[i].char_value;
		    continue;

		case DMC_C_FLIMIT:
		    c_flimit[0] = chr[i].char_value;
		    continue;

		case DMC_C_SHARED_BUFMGR:
		    c_shared_cache = TRUE;
		    if (dmc->dmc_cache_name)
			c_cache_name = dmc->dmc_cache_name;
		    continue;

		case DMC_C_DBCACHE_SIZE:
		    c_dbcache_size = chr[i].char_value;
		    continue;

		case DMC_C_TBLCACHE_SIZE:
		    c_tblcache_size = chr[i].char_value;
		    continue;

		case DMC_C_MAJOR_UDADT:
		    major_id_index = i;
		    continue;

		case DMC_C_MINOR_UDADT:
		    minor_id_index = i;
		    continue;
		    
		case DMC_C_SCANFACTOR:
		    c_scanfactor[0] = chr[i].char_fvalue;
		    continue;

		case DMC_C_INT_SORT_SIZE:
		    c_int_sort_size = chr[i].char_value;
		    continue;

		case DMC_C_LOCK_CACHE:
		    c_cache_lock = TRUE;
		    continue;

		case DMC_C_CHECK_DEAD_INTERVAL:
		    c_check_dead_interval = chr[i].char_value;
		    continue;

# ifdef UNIX
		case DMC_C_CORE_ENABLED:
		    Ex_core_enabled = TRUE;
		    continue;
# endif


		case DMC_C_CP_TIMER:
		    c_cp_timer = chr[i].char_value;
		    continue;

		case DMC_C_GC_INTERVAL:
		    c_gc_interval = chr[i].char_value;
		    continue;

		case DMC_C_GC_NUMTICKS:
		    c_gc_numticks = chr[i].char_value;
		    gc_numticks_changed = TRUE;
		    continue;

		case DMC_C_GC_THRESHOLD:
		    c_gc_threshold = chr[i].char_value;
		    gc_threshold_changed = TRUE;
		    continue;

		case DMC_C_NUMRAHEAD:
		    c_numrahead = chr[i].char_value;
		    continue;

		case DMC_C_LREADLOCK:
		    c_lk_readlock = chr[i].char_value;
		    continue;

		case DMC_C_LMAXLOCKS:
		    c_lk_maxlocks = chr[i].char_value;
		    continue;

		case DMC_C_LTIMEOUT:
		    c_lk_waitlock = chr[i].char_value;
		    continue;

		case DMC_C_ISOLATION_LEVEL:
		    c_isolation = chr[i].char_value;
		    continue;

		case DMC_C_4K_BUFFERS:
		    c_buffers[1] = chr[i].char_value;
		    continue;

                case DMC_C_DMCM:
                    c_dmcm = TRUE;
                    continue;

		case DMC_C_LOG_READNOLOCK:
		    c_logreadnolock = TRUE;
		    continue;

		case DMC_C_LOG_ESC_LPR_SC:
		    c_lk_log_esc_lpr_sc = TRUE;
		    continue;

		case DMC_C_LOG_ESC_LPR_UT:
		    c_lk_log_esc_lpr_ut = TRUE;
		    continue;

		case DMC_C_LOG_ESC_LPT_SC:
		    c_lk_log_esc_lpt_sc = TRUE;
		    continue;

		case DMC_C_LOG_ESC_LPT_UT:
		    c_lk_log_esc_lpt_ut = TRUE;
		    continue;

		case DMC_C_2K_WRITEBEHIND:
		    /* 2k WriteBehind OFF */
		    c_writebehind[0] = 0;
		    continue;
		    
		case DMC_C_4K_GBUFFERS:
		    c_gbuffers[1] = chr[i].char_value;
		    continue;

		case DMC_C_4K_GCOUNT:
		    c_gcount[1] = chr[i].char_value;
		    continue;

		case DMC_C_4K_WSTART:
		    c_wbstart[1] = chr[i].char_value;
		    continue;

		case DMC_C_4K_WEND:
		    c_wbend[1] = chr[i].char_value;
		    continue;

		case DMC_C_4K_MLIMIT:
		    c_mlimit[1] = chr[i].char_value;
		    continue;

		case DMC_C_4K_FLIMIT:
		    c_flimit[1] = chr[i].char_value;
		    continue;

		case DMC_C_4K_NEWALLOC:
		    c_cache_flags[1] |= DM0P_NEWSEG;
		    continue;

		case DMC_C_4K_SCANFACTOR:
		    c_scanfactor[1] = chr[i].char_fvalue;
		    continue;

		case DMC_C_4K_WRITEBEHIND:
		    /* 4k WriteBehind OFF */
		    c_writebehind[1] = 0;
		    continue;

		case DMC_C_8K_BUFFERS:
		    c_buffers[2] = chr[i].char_value;
		    continue;

		case DMC_C_8K_GBUFFERS:
		    c_gbuffers[2] = chr[i].char_value;
		    continue;

		case DMC_C_8K_GCOUNT:
		    c_gcount[2] = chr[i].char_value;
		    continue;

		case DMC_C_8K_WSTART:
		    c_wbstart[2] = chr[i].char_value;
		    continue;

		case DMC_C_8K_WEND:
		    c_wbend[2] = chr[i].char_value;
		    continue;

		case DMC_C_8K_MLIMIT:
		    c_mlimit[2] = chr[i].char_value;
		    continue;

		case DMC_C_8K_FLIMIT:
		    c_flimit[2] = chr[i].char_value;
		    continue;

		case DMC_C_8K_NEWALLOC:
		    c_cache_flags[2] |= DM0P_NEWSEG;
		    continue;

		case DMC_C_8K_SCANFACTOR:
		    c_scanfactor[2] = chr[i].char_fvalue;
		    continue;

		case DMC_C_8K_WRITEBEHIND:
		    /* 8k WriteBehind OFF */
		    c_writebehind[2] = 0;
		    continue;

		case DMC_C_16K_BUFFERS:
		    c_buffers[3] = chr[i].char_value;
		    continue;

		case DMC_C_16K_GBUFFERS:
		    c_gbuffers[3] = chr[i].char_value;
		    continue;

		case DMC_C_16K_GCOUNT:
		    c_gcount[3] = chr[i].char_value;
		    continue;

		case DMC_C_16K_WSTART:
		    c_wbstart[3] = chr[i].char_value;
		    continue;

		case DMC_C_16K_WEND:
		    c_wbend[3] = chr[i].char_value;
		    continue;

		case DMC_C_16K_MLIMIT:
		    c_mlimit[3] = chr[i].char_value;
		    continue;

		case DMC_C_16K_FLIMIT:
		    c_flimit[3] = chr[i].char_value;
		    continue;

		case DMC_C_16K_NEWALLOC:
		    c_cache_flags[3] |= DM0P_NEWSEG;
		    continue;

		case DMC_C_16K_SCANFACTOR:
		    c_scanfactor[3] = chr[i].char_fvalue;
		    continue;

		case DMC_C_16K_WRITEBEHIND:
		    /* 16k WriteBehind OFF */
		    c_writebehind[3] = 0;
		    continue;

		case DMC_C_32K_BUFFERS:
		    c_buffers[4] = chr[i].char_value;
		    continue;

		case DMC_C_32K_GBUFFERS:
		    c_gbuffers[4] = chr[i].char_value;
		    continue;

		case DMC_C_32K_GCOUNT:
		    c_gcount[4] = chr[i].char_value;
		    continue;

		case DMC_C_32K_WSTART:
		    c_wbstart[4] = chr[i].char_value;
		    continue;

		case DMC_C_32K_WEND:
		    c_wbend[4] = chr[i].char_value;
		    continue;

		case DMC_C_32K_MLIMIT:
		    c_mlimit[4] = chr[i].char_value;
		    continue;

		case DMC_C_32K_FLIMIT:
		    c_flimit[4] = chr[i].char_value;
		    continue;

		case DMC_C_32K_NEWALLOC:
		    c_cache_flags[4] |= DM0P_NEWSEG;
		    continue;

		case DMC_C_32K_SCANFACTOR:
		    c_scanfactor[4] = chr[i].char_fvalue;
		    continue;

		case DMC_C_32K_WRITEBEHIND:
		    /* 32k WriteBehind OFF */
		    c_writebehind[4] = 0;
		    continue;

		case DMC_C_64K_BUFFERS:
		    c_buffers[5] = chr[i].char_value;
		    continue;

		case DMC_C_64K_GBUFFERS:
		    c_gbuffers[5] = chr[i].char_value;
		    continue;

		case DMC_C_64K_GCOUNT:
		    c_gcount[5] = chr[i].char_value;
		    continue;

		case DMC_C_64K_WSTART:
		    c_wbstart[5] = chr[i].char_value;
		    continue;

		case DMC_C_64K_WEND:
		    c_wbend[5] = chr[i].char_value;
		    continue;

		case DMC_C_64K_MLIMIT:
		    c_mlimit[5] = chr[i].char_value;
		    continue;

		case DMC_C_64K_FLIMIT:
		    c_flimit[5] = chr[i].char_value;
		    continue;

		case DMC_C_64K_NEWALLOC:
		    c_cache_flags[5] |= DM0P_NEWSEG;
		    continue;

		case DMC_C_64K_SCANFACTOR:
		    c_scanfactor[5] = chr[i].char_fvalue;
		    continue;

		case DMC_C_64K_WRITEBEHIND:
		    /* 64k WriteBehind OFF */
		    c_writebehind[5] = 0;
		    continue;

		case DMC_C_REP_QSIZE:
		    c_rep_qsize = chr[i].char_value;
		    continue;

		case DMC_C_REP_SALOCK:
		    c_rep_salock = chr[i].char_value;
		    continue;

		case DMC_C_REP_DQLOCK:
		    c_rep_dqlock = chr[i].char_value;
		    continue;

		case DMC_C_REP_DTMAXLOCK:
		    c_rep_dtmaxlock = chr[i].char_value;
		    continue;

		case DMC_C_REP_IQLOCK:
		    c_rep_iqlock = chr[i].char_value;
		    continue;

		case DMC_C_BLOB_ETAB_STRUCT:
		    c_blob_etab_struct = chr[i].char_value;
		    continue;

		case DMC_C_DEF_PAGE_SIZE:
		    c_def_page_size = chr[i].char_value;
		    if (c_def_page_size == 2048 || c_def_page_size == 4096 ||
			c_def_page_size == 8192 || c_def_page_size == 16384 ||
			c_def_page_size == 32768 || c_def_page_size == 65536)
                        continue;
		    else
		    {
                	uleFormat(NULL, E_DM0184_PAGE_SIZE_NOT_CONF, 
				(CL_ERR_DESC *)NULL, ULE_LOG , NULL, (char * )NULL, 
				0L, (i4 *)NULL, &err_code, 0);
			c_def_page_size = DM_PG_SIZE;
                        continue;
		    }

		case DMC_C_ETAB_PAGE_SIZE:
		    c_etab_page_size = chr[i].char_value;
		    if (c_etab_page_size == 0 ||
			c_etab_page_size == 2048 || c_etab_page_size == 4096 ||
			c_etab_page_size == 8192 || c_etab_page_size == 16384 ||
			c_etab_page_size == 32768 || c_etab_page_size == 65536)
                        continue;
		    else
		    {
                	uleFormat(NULL, E_DM0184_PAGE_SIZE_NOT_CONF, 
				(CL_ERR_DESC *)NULL, ULE_LOG , NULL, (char * )NULL, 
				0L, (i4 *)NULL, &err_code, 0);
			c_etab_page_size = DM_PG_SIZE;  /* Go back to default */
			continue;
		    }

		case DMC_C_PSORT_BSIZE:
		    c_stbsize = chr[i].char_value;
		    if (c_stbsize < 100)
			c_stbsize = 100;
		    continue;

		case DMC_C_PSORT_ROWS:
		    c_strows = chr[i].char_value;
		    continue;

		case DMC_C_PSORT_NTHREADS:
		    c_stnthreads = chr[i].char_value;
		    continue;

		case DMC_C_SORT_CPUFACTOR:
		    c_stcpufact *= chr[i].char_fvalue;
		    continue;

		case DMC_C_SORT_IOFACTOR:
		    c_stiofact *= chr[i].char_fvalue;
		    continue;

		case DMC_C_PIND_BSIZE:
		    c_pind_bsize = chr[i].char_value;
		    if (c_pind_bsize < 100)
			c_pind_bsize = 100;
		    continue;

		case DMC_C_PIND_NBUFFERS:
		    c_pind_nbuffers = chr[i].char_value;
		    /* Number of buffers should be atleast 2 */
		    if (c_pind_nbuffers <= 1)
			c_pind_nbuffers = 2;
		    continue;

		case DMC_C_PAGETYPE_V1:
		    config_pgtypes &= ~(SVCB_CONFIG_V1);
		    continue;

		case DMC_C_PAGETYPE_V2:
		    config_pgtypes &= ~(SVCB_CONFIG_V2);
		    continue;

		case DMC_C_PAGETYPE_V3:
		    config_pgtypes &= ~(SVCB_CONFIG_V3);
		    continue;

		case DMC_C_PAGETYPE_V4:
		    config_pgtypes &= ~(SVCB_CONFIG_V4);
		    continue;

		case DMC_C_PAGETYPE_V5:
		    config_pgtypes &= ~(SVCB_CONFIG_V5);
		    continue;

		case DMC_C_PAGETYPE_V6:
		    config_pgtypes &= ~(SVCB_CONFIG_V6);
		    continue;

		case DMC_C_PAGETYPE_V7:
		    config_pgtypes &= ~(SVCB_CONFIG_V7);
		    continue;

		case DMC_C_DOP:
		    if (chr[i].char_value >= 0
				&& chr[i].char_value <= SCB_DOP_DEFAULT)
			c_dop = chr[i].char_value;
		    else
			TRdisplay("Invalid value for 'degreeofparallelism' %d\n",
				chr[i].char_value);
		    continue;

		case DMC_C_LLEVEL:
		    c_lk_level = chr[i].char_value;
		    continue;

		case DMC_C_PAD_BYTES:
		    c_pad_bytes = chr[i].char_value;
		    continue;

		case DMC_C_CRYPT_MAXKEYS:
		    c_crypt_maxkeys = chr[i].char_value;
		    continue;

                case DMC_C_BUILD_PAGES:
                    c_mwrite_blocks = chr[i].char_value;
                    continue;

		default:
		    break;
		}
		break;
	    }
	    if (i == 0 || i != chr_count)
	    {
		SETDBERR(&dmc->error, i, E_DM000D_BAD_CHAR_ID);
		break;
	    }
	}

	if (star_server == 0)
	{
	    /* Initialize "tmmo" interface to allow gathering of dbms server
	    ** performance statistics through MO.
	    */
	    status = dmf_tmmo_attach_tmperf();
	    if ( status != E_DB_OK )
	    {
		SETDBERR(&dmc->error, 0, E_DM004A_INTERNAL_ERROR);
		break;
	    }
	}

	if (star_server == 0)
	{
	    /*
	    ** Initialize the DMF Core Catalog Templates.
	    ** These describe the layout of the catalogs iirelation, iiattribute,
	    ** iiindex, and iirel_idx.  They are used by DMF to build the core
	    ** catalog TCB's at opendb time and by createdb to populate new
	    ** databases.
	    */

	    status = dmm_init_catalog_templates( );
	    if ( status != E_DB_OK )
	    {
		uleFormat(NULL, E_DM91A7_BAD_CATALOG_TEMPLATES, (CL_ERR_DESC *)NULL, 
		    ULE_LOG , NULL, (char * )NULL, 0L, (i4 *)NULL, &err_code, 0);
		SETDBERR(&dmc->error, 0, E_DM004A_INTERNAL_ERROR);
		break;
	    }
	}

	/*  Allocate the server control block. */

	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_length = sizeof(SCF_CB);
	scf_cb.scf_session = DB_NOSESSION;
	scf_cb.scf_facility = DB_DMF_ID;
	scf_cb.scf_scm.scm_functions = SCU_MZERO_MASK;
	scf_cb.scf_scm.scm_in_pages = ((sizeof(DM_SVCB) + SCU_MPAGESIZE - 1)
	    & ~(SCU_MPAGESIZE - 1)) / SCU_MPAGESIZE;
	status = scf_call(SCU_MALLOC, &scf_cb);
	if (status != OK)
	{
	    uleFormat(&scf_cb.scf_error, 0, (CL_ERR_DESC *)NULL, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
		&err_code, 0);
	    SETDBERR(&dmc->error, 0, E_DM923D_DM0M_NOMORE);
	    break;
	}
	svcb = dmf_svcb = (DM_SVCB *)scf_cb.scf_scm.scm_addr;
	
	/*	Now initialize the Server Control Block. */

	/* Set offset from CS_SCB to *DML_SCB */
	svcb->svcb_dmscb_offset = dmc->dmc_dmscb_offset;

	svcb->svcb_q_next = NULL;
	svcb->svcb_q_prev = NULL;
	svcb->svcb_length = sizeof(*svcb);
	svcb->svcb_type = SVCB_CB;
	svcb->svcb_s_reserved = 0;
	svcb->svcb_l_reserved = NULL;
	svcb->svcb_owner = (PTR)svcb;
	svcb->svcb_ascii_id = SVCB_ASCII_ID;
	svcb->svcb_tcb_id = 1;
	svcb->svcb_id = dmc->dmc_id;
	svcb->svcb_d_next = (DMP_DCB*) &svcb->svcb_d_next;
	svcb->svcb_d_prev = (DMP_DCB*) &svcb->svcb_d_next;
	svcb->svcb_s_next = (DML_SCB*) &svcb->svcb_s_next;
	svcb->svcb_s_prev = (DML_SCB*) &svcb->svcb_s_next;
	svcb->svcb_hcb_ptr = NULL;

	svcb->svcb_lctx_ptr = NULL;
	svcb->svcb_lock_list = 0;
	svcb->svcb_ss_lock_list = 0;
	svcb->svcb_stbsize = c_stbsize;
	svcb->svcb_strows = c_strows;
	svcb->svcb_stnthreads = c_stnthreads;
	svcb->svcb_sort_cpufactor = c_stcpufact;
	svcb->svcb_sort_iofactor = c_stiofact;
	svcb->svcb_pind_bsize = c_pind_bsize;
	svcb->svcb_pind_nbuffers = c_pind_nbuffers;

	svcb->svcb_s_max_session = c_session_max;
	svcb->svcb_d_max_database = c_database_max;
	svcb->svcb_m_max_memory = c_memory_max * 1024;
	svcb->svcb_x_max_transactions = c_transaction_max;
	svcb->svcb_mwrite_blocks = c_mwrite_blocks;
	svcb->svcb_int_sort_size = c_int_sort_size;
	for (segment_ix = 0; segment_ix < DM_MAX_BUF_SEGMENT; segment_ix++)
	    svcb->svcb_bmseg[segment_ix] = NULL;
	svcb->svcb_lbmcb_ptr = NULL;
	svcb->svcb_status = 0;
	svcb->svcb_check_dead_interval = c_check_dead_interval;
	svcb->svcb_gc_interval = c_gc_interval;
	svcb->svcb_gc_numticks = c_gc_numticks;
	svcb->svcb_gc_threshold = c_gc_threshold;
	svcb->svcb_lk_readlock = c_lk_readlock;
	svcb->svcb_lk_maxlocks = c_lk_maxlocks;
	svcb->svcb_lk_waitlock = c_lk_waitlock;
	svcb->svcb_rep_salock = c_rep_salock;
	svcb->svcb_rep_iqlock = c_rep_iqlock;
	svcb->svcb_rep_dqlock = c_rep_dqlock;
	svcb->svcb_rep_dtmaxlock = c_rep_dtmaxlock;
	svcb->svcb_isolation = c_isolation;
	svcb->svcb_lk_level = c_lk_level;
	svcb->svcb_log_err = 0;
	svcb->svcb_seq = (DML_SEQ*)NULL;
	if (c_logreadnolock)
	    svcb->svcb_log_err |= SVCB_LOG_READNOLOCK;
	if (c_lk_log_esc_lpr_sc)
	    svcb->svcb_log_err |= SVCB_LOG_LPR_SC;
	if (c_lk_log_esc_lpr_ut)
	    svcb->svcb_log_err |= SVCB_LOG_LPR_UT;
	if (c_lk_log_esc_lpt_sc)
	    svcb->svcb_log_err |= SVCB_LOG_LPT_SC;
	if (c_lk_log_esc_lpt_ut)
	    svcb->svcb_log_err |= SVCB_LOG_LPT_UT;
	svcb->svcb_cp_timer = c_cp_timer;
	svcb->svcb_dop = c_dop;
	svcb->svcb_page_size = c_def_page_size;
	svcb->svcb_page_type = TCB_PG_INVALID;

	svcb->svcb_config_pgtypes = 0;
	svcb->svcb_config_pgtypes = config_pgtypes;
	if (!svcb->svcb_config_pgtypes)
	{
	    /* if no page types set, default to v1 and v2 */
	    uleFormat(NULL, E_DM0184_PAGE_SIZE_NOT_CONF,
		    (CL_ERR_DESC *)NULL, ULE_LOG , NULL, (char * )NULL,
		    0L, (i4 *)NULL, &err_code, 0);
	    svcb->svcb_config_pgtypes = SVCB_CONFIG_V1 | SVCB_CONFIG_V2;
	}
	
	svcb->svcb_blob_etab_struct = c_blob_etab_struct;
	stat = TMtz_init(&svcb->svcb_tzcb);
	if (stat != E_DB_OK)
	{
	    uleFormat(NULL, stat, 0, ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	    SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
	    break;
	}

#ifdef xDEBUG
	if (clustered)
	    TRdisplay("DMF: This is a cluster installation.\n");
#endif
	
	/* TRdisplay maximum btree leaf entry length */

	/* Assume the largest leaf will store no atts (Clustered), and have a 4-byte key */
	MaxLeafLength = 0;
	for (pgsize = 2048; pgsize <= 65536; pgsize *=2)
	{
	    for (pgtype = 1; pgtype <= DB_MAX_PGTYPE; pgtype++)
	    {
		LeafLength = DM1B_VPT_MAXLEAFLEN_MACRO(pgtype, pgsize, 0, 4);
		if ( LeafLength > MaxLeafLength )
		    MaxLeafLength = LeafLength;
#ifdef xDEBUG
		TRdisplay("Max Btree leaf entry for (%2dK V%d) = %5d\n",
			pgsize/1024, pgtype, LeafLength);
#endif
	    }
	}
	TRdisplay("Max Btree leaf entry length = %d, dm1b_keylength = %d, lg_max_rsz = %d\n",
		    MaxLeafLength, DM1B_KEYLENGTH, LG_MAX_RSZ);

	if (dmc->dmc_flags_mask & DMC_C2_SECURE)
	    svcb->svcb_status |= SVCB_C2SECURE;

	/* Let all know if we're OS-threaded */
	if ( CS_is_mt() )
	    svcb->svcb_status |= SVCB_IS_MT;

        if ( clustered &&
             (DMC_CLASS_NODE_AFFINITY == (dmc->dmc_flags_mask &
              (DMC_CLASS_NODE_AFFINITY|DMC_RECOVERY|DMC_STAR_SVR|DMC_IOMASTER))) &&
             NULL != svr_class_name && '\0' != *svr_class_name &&
             '*' != *svr_class_name )
        {
            /* Only clustered database servers in a named class can set this.*/
            svcb->svcb_status |= SVCB_CLASS_NODE_AFFINITY;
        }

	/* If II_DBMS_REP env var is set, never replicate.  This was
	** designed for situations where the replication environment is
	** messed up somehow and can't be trusted.
	** We don't check the value, just whether the variable exists
	** and has some value.
	*/
	NMgtAt("II_DBMS_REP", &rep_var);
	if (rep_var != NULL && *rep_var != EOS)
	{
	    svcb->svcb_status |= SVCB_NO_REP;
	    TRdisplay("DMF_INIT: Replication disabled via II_DBMS_REP\n");
	}

	MEfill(sizeof(svcb->svcb_cnt), 0, (PTR)&svcb->svcb_cnt);
	MEfill(sizeof(svcb->svcb_stat), 0, (PTR)&svcb->svcb_stat);
	MEfill(sizeof(svcb->svcb_trace), '\0', (PTR)&svcb->svcb_trace[0]);

	/*  Initialize the memory manager. */

	/* For now, use max LT pools, maybe configurable? */
	if ( svcb->svcb_status & SVCB_IS_MT )
	    svcb->svcb_pools = SVCB_MAXPOOLS;
	else
	    /* If Ingres threads, just use one pool */
	    svcb->svcb_pools = 1;
	/* Initial OS-threaded ShortTerm pool size, perhaps configurable? */
	if ( svcb->svcb_status & SVCB_IS_MT )
	    svcb->svcb_st_ialloc = DM0M_PageSize;
	/*
	** FIXME: for now, leave it zero until we can better
	** determine which types of DMF sessions might benefit
	** rather than wastefully allocating it to all sessions,
	** including those which will never do dm0m_allocate()'s.
	**
	** If there's a value here, it's picked up by SCF and
	** added to the size of DML_SCB when initiating a
	** DMF-needing session.
	*/
	svcb->svcb_st_ialloc = 0;

	svcb->svcb_pad_bytes = c_pad_bytes;
	dm0m_init();


	if (name)
	    MEmove(name->data_in_size, (char *)name->data_address, ' ',
		sizeof(svcb->svcb_name), (char *)&svcb->svcb_name);
	dm0s_minit(&svcb->svcb_dq_mutex, "SVCB DCB mutex");
	dm0s_minit(&svcb->svcb_sq_mutex, "SVCB SCB mutex");

	if (star_server)
	{
	    /*
	    ** The star server currently needs DMF only in order to perform
	    ** the DMT_SORT_COST operation. That operation requires only that
	    ** the svcb_scanfactor and svcb_bmcb_ptr->bm_sbufcnt fields be
	    ** filled in. So, now that we have a mostly-initialized dm_svcb,
	    ** we simply fabricate a bm_sbufcnt field and return.
	    **
	    ** Note that by fabricate we truly mean fabricate. None of the
	    ** information we return really means anything, but the optimizer
	    ** wants it anyway, and we aim to please.
	    **
	    ** (The reason for wanting to start only a truly minimal star server
	    ** is that starting up LG and DI in a Unix system implies a lot
	    ** of overhead, because we will fork slaves and connect to the
	    ** logging system and access shared memory and do lots of unhappy
	    ** glorp. By starting a truly minimal star server we avoid all that)
	    */
	    for (cache_ix = 0; cache_ix < DM_MAX_CACHE; cache_ix++)
	    {
		svcb->svcb_scanfactor[cache_ix] = (c_scanfactor[cache_ix] ? 
			     c_scanfactor[cache_ix] : c_gcount[cache_ix]);
		svcb->svcb_c_buffers[cache_ix]  = c_buffers[cache_ix];
	    }

	    return (E_DB_OK);
	}

	/*
	** Set startup DMF trace flags.
	*/
	{
	    char	*trace = NULL;
	    char	*comma;
	    i4	trace_point;

	    NMgtAt("II_DMF_TRACE", &trace);
	    if ((trace != NULL) && (*trace != EOS))
	    {
		do
		{
		    if (comma = STchr(trace, ','))
			*comma = EOS;
		    if (CVal(trace, &trace_point))
			break;
		    trace = &comma[1];
		    if (trace_point < 0 ||
			trace_point >= DMZ_CLASSES * DMZ_POINTS)
		    {
			continue;
		    }

		    TRdisplay("%@ DMF_INIT: Set trace flag %d\n", trace_point);
		    DMZ_SET_MACRO(svcb->svcb_trace, trace_point);
		} while (comma);
	    }
	}

	/*
	** If this is the recovery server, perform the initial recovery server
	** setup processing, which includes the LK and LG initialization for
	** the installation. If this is NOT the recovery server, make sure that
	** the recovery server is up before going any further.
	*/
	if ((dmc->dmc_flags_mask & DMC_RECOVERY) != 0)
	{
	    status = dmr_rcp_init(&recovery_log_id);
	    if (status)
	    {
		SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		break;
	    }
	}

	status = OK;

	/*  Initialize the the locking system. */

	if ((dmc->dmc_flags_mask & DMC_RECOVERY) == 0)
	{
	    status = LKinitialize(&sys_err, svr_class_name);
	    if (status != OK)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			(char * )NULL, 0L, (i4 *)NULL, &err_code, 0);
		uleFormat(NULL, E_DM901E_BAD_LOCK_INIT, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL,
			(char * )NULL, 0L, (i4 *)NULL, &err_code, 0);
		SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		break;
	    }

	    /*
	    ** Initialize the logging system
	    ** Also consistency check for LG_MAX_RSZ which should be
	    ** big enough for largest log record (when logging 64k tables)
	    */
	    status = LGinitialize(&sys_err, svr_class_name);
	    lg_max1 = (sizeof(DM0L_BTSPLIT) - sizeof(DMPP_PAGE) + 65536 +
		    DM1B_KEYLENGTH + sizeof(LG_RECORD));
	    lg_max2 = (sizeof(DM0L_REP) + (2 * dm2u_maxreclen(TCB_PG_V2, 65536)) +
		    sizeof(LG_RECORD));
	    if (status != OK || LG_MAX_RSZ < lg_max1 || LG_MAX_RSZ < lg_max2)
	    {
		if (status == OK)
		    TRdisplay("Consistency check for maximum log record size failed\n");

		TRdisplay("Maximum log record size must be max(%d, %d) but it is %d\n",
			lg_max1, lg_max2, LG_MAX_RSZ);

		uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			(char * )NULL, 0L, (i4 *)NULL, &err_code, 0);
		uleFormat(NULL, E_DM9011_BAD_LOG_INIT, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL,
			(char * )NULL, 0L, (i4 *)NULL, &err_code, 0);
		SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		break;
	    }
	    status = LGshow(LG_S_LGSTS, (PTR)&lgd_status, sizeof(lgd_status), 
			    &length, &sys_err);
	    if (status)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			(char * )NULL, 0L, (i4 *)NULL, &err_code, 0);
		uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL,
			(char * )NULL, 0L, (i4 *)NULL, &err_code, 1,
			0, (i4)LG_S_LGSTS);
		SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		break;
	    }
	    if ((lgd_status & 1) == 0)
	    {
		uleFormat(NULL, E_DM9C0A_RSRV_NOT_ONLINE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL,
			(char * )NULL, 0L, (i4 *)NULL, &err_code, 0);
		status = E_DB_ERROR;
		SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		break;
	    }
	}


	/* Both the recovery and dbms servers need the LG xid_array info */
	if ( status == OK )
	{
	    /* Get pointer to LG active transaction array */
	    status = LGshow(LG_S_XID_ARRAY_PTR, (PTR)&svcb->svcb_xid_array_ptr, 
	    			sizeof(svcb->svcb_xid_array_ptr), 
			    	&length, &sys_err);
	    if (status)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			(char * )NULL, 0L, (i4 *)NULL, &err_code, 0);
		uleFormat(NULL, E_DM9011_BAD_LOG_INIT, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL,
			(char * )NULL, 0L, (i4 *)NULL, &err_code, 0);
		SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		break;
	    }
	    /* Get size of LG active transaction array */
	    status = LGshow(LG_S_XID_ARRAY_SIZE, (PTR)&svcb->svcb_xid_array_size, 
	    			sizeof(svcb->svcb_xid_array_size), 
			    	&length, &sys_err);
	    if (status)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			(char * )NULL, 0L, (i4 *)NULL, &err_code, 0);
		uleFormat(NULL, E_DM9011_BAD_LOG_INIT, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL,
			(char * )NULL, 0L, (i4 *)NULL, &err_code, 0);
		SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		break;
	    }
	    /* Compute the highest lg_id bound of the array */
	    svcb->svcb_xid_lg_id_max = svcb->svcb_xid_array_size / sizeof(u_i4 *);

	    TRdisplay("%@ dmc_start_server:%d svcb xid_array %p, size %d, lg_id_max %x\n",
	    		__LINE__,
			svcb->svcb_xid_array_ptr,
			svcb->svcb_xid_array_size,
			svcb->svcb_xid_lg_id_max);
	}

	if (status == OK)
	    status = LKcreate_list(LK_ASSIGN | LK_NONPROTECT | 
				   LK_NOINTERRUPT | LK_MULTITHREAD,
		(i4)0, (LK_UNIQUE *)0, (LK_LLID *)&svcb->svcb_lock_list, 0,
	        &sys_err);
	if (status != OK)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
	            (char * )NULL, 0L, (i4 *)NULL, &err_code, 0);
	    uleFormat(NULL, E_DM901A_BAD_LOCK_CREATE, &sys_err, ULE_LOG , NULL, 
	            (char * )NULL, 0L, (i4 *)NULL, &err_code, 0);
	    if (status == LK_NOLOCKS)
		SETDBERR(&dmc->error, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
	    else
		SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
	    break;
	}

	/*
	** Query LK to see if Update locks are supported.
	**
	** If they are not, LK silently changes LK_U requests
	** to LK_X.
	*/
	if ( (LKshow(LK_S_ULOCKS, 
			0, (LK_LKID*)NULL, (LK_LOCK_KEY*)NULL,
			0, (PTR)NULL, (u_i4*)NULL, (u_i4*)NULL,
			&sys_err)) == OK )
	{
	    svcb->svcb_status |= SVCB_ULOCKS;
	}
	    

        if ( svcb->svcb_status & SVCB_CLASS_NODE_AFFINITY )
        {
            /*
            ** If Server class is restricted to a single node, then obtain
            ** a lock on the server class.  We're overloading the LK_CONTROL
            ** locktype for this, since its purpose and key format are
            ** close to what we want.  Note however this means that server
            ** class names of II_DMCM, II_DATATYPE_LEVEL, and II_TFNAME[0-9]*,
            ** could collide with other LK_CONTROL locks.  However, as there
            ** is a general warning that users should treat identifiers
            ** starting with II as potentially reserved, I assert this is
            ** acceptable, and we don't need to define a separate LK_SVRCLASS
            ** locktype.
            **
            ** We 1st try no-wait to get lock in Exclusive mode.  If successful
            ** we write out the node number server is running on by converting
            ** down to share mode.
            **
	    ** If we couldn't get it, and the class is defined to use a
	    ** shared cache, we try again this time for share mode with
	    ** a timeout.  If lock value block (LVB) has a different
	    ** node number then we must exit.   If LVB is invalid,
	    ** release lock and try again for exclusive.  If we timeout
	    */
#           define DAWDLE_TIME        5 /* Seconds to wait for lock */

            LK_LOCK_KEY         lock_key;
            LK_VALUE            lock_value;
            LK_LKID             lockid;

            lock_key.lk_type = LK_CONTROL;
            MEmove(STlength(svr_class_name), svr_class_name, ' ',
                    6 * sizeof(lock_key.lk_key1),
                    (PTR) &lock_key.lk_key1);
	    MEfill(sizeof(LK_LKID), 0, &lockid);
            for ( ; /* Something to break out of */ ; )
            {
                status = LKrequest((LK_PHYSICAL | LK_NODEADLOCK | LK_NOWAIT),
                 (LK_LLID) svcb->svcb_lock_list, &lock_key, LK_X, &lock_value,
                 &lockid, (i4)0, &sys_err);
                if ( OK == status || LK_VAL_NOTVALID == status )
                {
                    /* Got the lock, write out node number */
                    lock_value.lk_high = 0;
                    lock_value.lk_low = local_node_number;
                    lock_value.lk_mode =
		     (NULL == dmc->dmc_cache_name) ? LK_X : LK_S;

		    /*
		    ** Convert downward to LK_S if using a shared cache
		    ** buffer, otherwise retain lock in exclusive mode.
		    */
                    status = LKrequest(
                     (LK_PHYSICAL | LK_NODEADLOCK | LK_CONVERT),
                     (LK_LLID) svcb->svcb_lock_list, &lock_key, 
		     lock_value.lk_mode,
                     &lock_value, &lockid, (i4)0, &sys_err);
		    if ( OK == status )
			break;
                }
                else if ( LK_BUSY == status )
                {
		    /*
		    ** Blocked.  If a NAC server class is using a private
		    ** cache, control lock will be held exclusively for the
		    ** life of the server, and a conflict here indicates
		    ** we should not start another instance.  Otherwise,
		    ** this is a possible race condition with another
		    ** instance of the server class starting.  If the
		    ** server starting here uses shared cache, try again
		    ** but with LK_S, and only for a short time, since lock
		    ** should only be held exclusively against us for a
		    ** significant time if the lock is owned by a server
		    ** class instance on another node defined to use a
		    ** private cache.  If it is, we don't want to hang
		    ** here until that other instance shuts down.
		    */
		    if ( NULL == dmc->dmc_cache_name )
		    {
			/*
			** Using private cache, and since there can only
			** be one instance of this server class using
			** a private cache anywhere in the cluster, and
			** at least one instance has started or is
			** starting elsewhere in the cluster, scram!
			*/
			uleFormat(NULL, E_DM006B_SERVER_CLASS_ONCE, &sys_err,
			    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			    &err_code, 1, 0, svr_class_name);
			break;
		    }

		    /*
		    ** Class is defined to use shared cache on this node.
		    ** We either have a startup race condition, or an
		    ** instance of this server class is started on another
		    ** node and is configured to use a private cache.
		    */
                    status = LKrequest(
                     (LK_PHYSICAL | LK_NODEADLOCK),
                     (LK_LLID) svcb->svcb_lock_list, &lock_key, LK_S,
                     &lock_value, &lockid, (i4)DAWDLE_TIME, &sys_err);
                    if ( OK == status )
                    {
                        /* Got node id */
			if ( lock_value.lk_low != local_node_number )
                        {
			    /* Server class is open on another node */
			    uleFormat(NULL, E_DM006C_SERVER_CLASS_ELSEWHERE,
				&sys_err, ULE_LOG, NULL, (char *)NULL,
				(i4)0, (i4 *)NULL, &err_code, 1,
				0, svr_class_name);

                            /* Drop the lock */
                            status = LKrelease(0,
				(LK_LLID)svcb->svcb_lock_list,
				&lockid, (LK_LOCK_KEY *)NULL,
				&lock_value, &sys_err);
                        }
                        if ( OK == status )
			    break;
                    }
		    else if ( LK_TIMEOUT == status )
		    {
			/* Assume server class is open on another node */
			uleFormat(NULL, E_DM006C_SERVER_CLASS_ELSEWHERE,
			    &sys_err, ULE_LOG, NULL, (char *)NULL,
			    (i4)0, (i4 *)NULL, &err_code, 1,
			    0, svr_class_name);
			break;
		    }
                    else if ( LK_VAL_NOTVALID == status )
                    {
                        /* Drop this lock & try again */
                        status = LKrelease(0, (LK_LLID) svcb->svcb_lock_list,
                         &lockid, (LK_LOCK_KEY *)NULL, &lock_value, &sys_err);
                        if ( OK == status ) 
                            continue;
                    }
                }
                
		/* Unanticipated lock return value. Report lock error */
                uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
                        (char * )NULL, 0L, (i4 *)NULL, &err_code, 0);
                uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err,
                    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                    &err_code, 2, 0, LK_X, 0, svcb->svcb_lock_list);
		break;
            }

            if ( OK != status )
            {
		SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
                break;
            }
        }

	/*
	** IF SINGLE-SERVER mode, we need a separate lock list for 
	** SV_DATABASE locks
	*/
	if (dmc->dmc_s_type & DMC_S_SINGLE)
	{
	    status = LKcreate_list(LK_ASSIGN | LK_NONPROTECT | 
				   LK_NOINTERRUPT | LK_MULTITHREAD,
		(i4)0, (LK_UNIQUE *)0, (LK_LLID *)&svcb->svcb_ss_lock_list,
		0, &sys_err);
	    if (status != OK)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			(char * )NULL, 0L, (i4 *)NULL, &err_code, 0);
		uleFormat(NULL, E_DM901A_BAD_LOCK_CREATE, &sys_err, ULE_LOG , NULL, 
			(char * )NULL, 0L, (i4 *)NULL, &err_code, 0);
		if (status == LK_NOLOCKS)
		    SETDBERR(&dmc->error, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
		else
		    SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		break;
	    }
	}

        /*
        ** (ICL keving)
        ** Initialise Callback data structures
	** if running DMCM or install is clustered.
        */
        if ( (clustered || c_dmcm) && (status = lk_cback_init()) )
	{
	    SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
            break;
	}

	/* Now that we have a locking system attachment, get a temporary
	** filename generator ID
	*/
	status = dmc_start_name_id(svcb);
	if (DB_FAILURE_MACRO(status))
	{
	    SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
	    break;
	}

	if ((dmc->dmc_flags_mask & DMC_RECOVERY) != 0)
	{
	    status = dmf_udt_lkinit(svcb->svcb_lock_list, (i4)TRUE, &dmc->error);
	    if (status)
		break;
	}

	/* If UDADT major or minor (or both) id's requested, find them	    */

	if ((major_id_index != -1) || (minor_id_index != -1))
	{
	    LK_LOCK_KEY		lock_key;
	    LK_VALUE		lock_value;
	    LK_LKID		lockid;
	    
	    lock_key.lk_type = LK_CONTROL;
	    MEmove(STlength(ADD_LOCK_KEY),
		    ADD_LOCK_KEY,
		    ' ',
		    6 * sizeof(lock_key.lk_key1),
		    (PTR) &lock_key.lk_key1);
	    MEfill(sizeof(LK_LKID), 0, &lockid);

	    status = LKrequest((LK_PHYSICAL | LK_NODEADLOCK),
		(LK_LLID) svcb->svcb_lock_list, &lock_key, LK_N, &lock_value,
		&lockid, (i4)0, &sys_err);
	    if ((status != OK) && (status != LK_VAL_NOTVALID))
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			(char * )NULL, 0L, (i4 *)NULL, &err_code, 0);
		uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 2,
		    0, LK_X, 0, svcb->svcb_lock_list);
		SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		break;
    	    }
	    else
	    {
		if (major_id_index != -1)
		{
		    chr[major_id_index].char_value =
			(status != LK_VAL_NOTVALID)
			    ?	(i4) lock_value.lk_high
			    :	DMC_C_UNKNOWN;
		}
		if (minor_id_index != -1)
		{
		    chr[minor_id_index].char_value =
			(status != LK_VAL_NOTVALID)
			    ?	(i4) lock_value.lk_low
			    :	DMC_C_UNKNOWN;
		}
		status = LKrelease(0, (LK_LLID) svcb->svcb_lock_list,
		    &lockid, (LK_LOCK_KEY *)NULL, &lock_value, &sys_err);

		if (status != OK)
		{
		    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			    (char * )NULL, 0L, (i4 *)NULL, &err_code, 0);
		    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, 
			NULL, (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 1,
			0, svcb->svcb_lock_list);
		    SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		    break;
		}
	    }
	}

        /*                                                              
        ** (ICL phil.p)
        ** Initialise/check DMCM lock.
        ** To enforce consistent use of protocols, initialise/check
        ** the DMCM lock value to determine whether or not the server is
        ** running DMCM.
        */
        status = dmcm_lkinit(dmc, svcb->svcb_lock_list, c_dmcm,
                             ((dmc->dmc_flags_mask & DMC_RECOVERY)!=0));
        if (status != E_DB_OK)                                          
            break; 

	/*
	** Read direct I/O config hints.
	*/
	status = dmf_setup_directio();
	if (status != E_DB_OK)
	{
	    SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
	    break;
	}

	/*
	** Allocate and initialize the Logging Context Block.
	** This is needed previous to the Buffer Manager creation as LG
	** calls will be made by it.
	*/
	flags = 0;

        /*
        ** (ICL phil.p)
        ** Mark as FastCommit if DMCM
        */
        if ((dmc->dmc_flags_mask & DMC_FSTCOMMIT) ||
            (dmc->dmc_flags_mask & DMC_DMCM))
            flags |= DM0L_FASTCOMMIT;
	if (dmc->dmc_flags_mask & DMC_RECOVERY)
	    flags |= (DM0L_RECOVER | DM0L_NOLOG);
	/* recovery_log_id will be non-zero in recovery server only */
	status = dm0l_allocate(flags, 0, recovery_log_id, 0, 0,
	    (DMP_LCTX **)0, &dmc->error);
	if (status != E_DB_OK)
	    break;
	log_allocated = TRUE;

	/*
	** Allocate and initialize the Buffer Manager Control Block.
	*/
	{
	    i4 pgsize;

	    flags = 0;
	    if (c_cache_lock)
		flags |= DM0P_LOCK_CACHE;
	    if (c_shared_cache)
		flags |= DM0P_SHARED_BUFMGR;
	    if ((dmc->dmc_flags_mask & DMC_RECOVERY) != 0)
		flags |= DM0P_MASTER;

            if ((dmc->dmc_flags_mask & DMC_BATCHMODE) != 0)
                dmf_svcb->svcb_status |= SVCB_BATCHMODE;                       

	    if ((dmc->dmc_flags_mask & DMC_IOMASTER) != 0)
	    {
		flags |= DM0P_IOMASTER;		/* indicate IO serv is up */
		dmf_svcb->svcb_status |= SVCB_IOMASTER; /* this is IO server */
	    }

	    /* If recovery server, see if rcp cache sizes have 
	    ** been specified.
	    */

            /*
            ** (ICL phil.p)
            ** set DMCM to pass into dm0p_allocate.
            */
            if (c_dmcm)
                flags |= DM0P_DMCM;

	    /*
	    ** Don't allow RCP cache to be too small, it really screws
	    ** up log space reservation.
	    */
	    if (dmc->dmc_flags_mask & DMC_RECOVERY)
		rcp_dmf_cache_size(c_buffers, c_cache_flags);

	    for (cache_ix = 0, pgsize = DM_PG_SIZE; cache_ix < DM_MAX_CACHE; 
		 cache_ix++, pgsize *= 2)
	    {
		/* If this is not the 2k cache, don't process the
		** parameters unless cache was specified
		*/
		if (c_buffers[cache_ix] < 0)
		    c_buffers[cache_ix] = 0;

		if (cache_ix > 0  && !(c_buffers[cache_ix]))
		{
		    if (pgsize == c_def_page_size)
		    {
                        uleFormat(NULL, E_DM0184_PAGE_SIZE_NOT_CONF,
                                (CL_ERR_DESC *)NULL, ULE_LOG , NULL, (char * )NULL,
                                0L, (i4 *)NULL, &err_code, 0);
			svcb->svcb_page_size = DM_PG_SIZE;
		    }
		    continue;
		}

		/*
		** Default cache size is four times the num of sessions.
		*/
		if (c_buffers[cache_ix] <= 0)
		{
		    c_buffers[cache_ix] = 128 + 4 * svcb->svcb_s_max_session;
		}
		/* if set to less than 0, then only calculate the default */
		if (c_gbuffers[cache_ix] < 0)
		    c_gbuffers[cache_ix] = 4 + svcb->svcb_s_max_session / 4;
		if (c_gcount[cache_ix] <= 0)
		    c_gcount[cache_ix] = 8;

		/*
		** Default for modify limit is 3/4 of buffer manager.
		** If write behind start limit has been specified, then
		** default modify lmit is halfway between the write 
		** behind start and the size of the cache.
		*/
		if ((c_mlimit[cache_ix] <= 0) || 
				(c_mlimit[cache_ix] > c_buffers[cache_ix]))
		{
		    c_mlimit[cache_ix] = c_buffers[cache_ix] * 3 / 4;
		    if ((c_wbstart[cache_ix] > 0) 
				&& (c_wbstart[cache_ix] <= c_buffers[cache_ix]))
			c_mlimit[cache_ix] = c_wbstart[cache_ix] +  
			     ((c_buffers[cache_ix] - c_wbstart[cache_ix]) / 2);
		}

		/*
		** Default for free limit is 1/32 of buffer manager.
		*/
		if ((c_flimit[cache_ix] <= 0) 
				|| (c_flimit[cache_ix] > c_buffers[cache_ix]))
		    c_flimit[cache_ix] = 2 + c_buffers[cache_ix] / 32;

		/*
		** Default for write behind start is 15% less than 
		** the modify limit.
		** bug 9595 - WBSTART was MLIMIT + 15% of cache (sandyh)
		*/
		if ((c_wbstart[cache_ix] <= 0) 
				|| (c_wbstart[cache_ix] > c_buffers[cache_ix]))
		    c_wbstart[cache_ix] = c_mlimit[cache_ix] - 
					(c_buffers[cache_ix] * 15 / 100);

		/*
		** Default for write behind end is 1/2 of buffer manager
		** If that is greater than the write behind start then 
		** use 50% of write behind start.
		*/
		if ((c_wbend[cache_ix] <= 0)  
				|| (c_wbend[cache_ix] > c_wbstart[cache_ix]))
		    c_wbend[cache_ix] = c_buffers[cache_ix] / 2;

		if (c_wbend[cache_ix] > c_wbstart[cache_ix])
		    c_wbend[cache_ix] = c_wbstart[cache_ix] / 2;
	    }

	    status = dm0p_allocate(flags,
		FALSE /* not connect-only */,
		c_buffers, c_flimit, c_mlimit,
		c_gbuffers, c_gcount, c_writebehind, c_wbstart, c_wbend, c_cache_flags,
                c_dbcache_size, c_tblcache_size, c_cache_name, &dmc->error); 
	    if (status != E_DB_OK)
		break;
	    bufmgr_allocated = TRUE;
	}

	/*
	** Initialize the cost factor for multi-block IO's.  Use the
	** group buffer size unless overridden by the scan factor parameter.
	** Note that we had to defer this until after the dm0p_allocate call
	** since the code above may have set c_gcount to a default in
	** preparation for the dm0p_allocate call.
	*/
	for (cache_ix = 0; cache_ix < DM_MAX_CACHE; cache_ix++)
	{
	    svcb->svcb_scanfactor[cache_ix] = 
			(c_scanfactor[cache_ix] ? c_scanfactor[cache_ix] : c_gcount[cache_ix]);
	}

	/*
	** If c_etab_page_size is zero, etabs will be created with
	** base table page size
	** Otherwise etabs will be created with page size c_etab_page_size
	*/
	if (dm0p_has_buffers(c_etab_page_size))
	    svcb->svcb_etab_pgsize = c_etab_page_size;
	else
	{
	    TRdisplay("Server not configured for etab page size %d using %d\n",
		c_etab_page_size, svcb->svcb_page_size);
	    svcb->svcb_etab_pgsize = svcb->svcb_page_size;
	}

	/* 
	** Since svcb_page_size can be changed with a tracepoint...
	** we need to save temp etab page size at server startup
	*/
	if (svcb->svcb_etab_pgsize)
	    svcb->svcb_etab_tmpsz = svcb->svcb_etab_pgsize;
	else
	    svcb->svcb_etab_tmpsz = svcb->svcb_page_size;

	TRdisplay("BLOB etab page_size = %d TMP etab page_size = %d\n",
	    svcb->svcb_etab_pgsize, svcb->svcb_etab_tmpsz);

	/*
	** Define Force Abort exception handler in LG.
	** This is the routine that the Logging System will call when
	** it wants to signal a force abort to our server.
	*/
	if (abort_handler)  /* Check here for standalone test to work. */
	{    
	    /* Tell the logging system the server force abort handler. */
	    PTR abort_buf[2];

	    /* 
	    ** Pass 2 pointers, the first to the log_id,
	    ** the second to the abort_handler.
	    */
	    abort_buf[0] = (PTR)&svcb->svcb_lctx_ptr->lctx_lgid;
	    abort_buf[1] = abort_handler;

	    status = LGalter(LG_A_FORCE_ABORT, (PTR)abort_buf,
				sizeof(abort_buf), &sys_err);
	    if (status)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			(char * )NULL, 0L, (i4 *)NULL, &err_code, 0);
		uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 1, 0,
		    LG_A_FORCE_ABORT);
		SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		break;
	    }
	}

	/*
	** If using a shared buffer manager, register it with the
	** logging system so recovery can be done if one of the servers
	** connected to it fails.
	*/
	if (c_shared_cache)
	{
	    i4	alter_buf[2];

	    alter_buf[0] = svcb->svcb_lctx_ptr->lctx_lgid;
	    alter_buf[1] = dm0p_bmid();

	    stat = LGalter(LG_A_BUFMGR_ID, (PTR)alter_buf,
		sizeof(alter_buf), &sys_err);
	    if (stat != OK)
	    {
		uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL,
			(char * )NULL, 0L, (i4 *)NULL, &err_code, 0);
		uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 1, 0, 
		    LG_A_BUFMGR_ID);
		SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		status = E_DB_ERROR;
		break;
	    }

	    /*
	    ** Check the number of connections to the buffer manager.
	    ** If there has been a fatal error in the buffer manger by another
	    ** server, then we cannot start up with this buffer manager.
	    */
	    dm0p_count_connections();
	}

	/* 
	** Determine if page checksums are desired, setup svcb_checksum.
	*/
	dmc_setup_checksum();

	/*  Allocate Hash Control Block. */

	status = dm0m_allocate(sizeof(DMP_HCB) + 
	    HCB_HASH_ARRAY_SIZE * sizeof(DMP_HASH_ENTRY) +
	    HCB_MUTEX_ARRAY_SIZE * sizeof(DM_MUTEX), 
	    DM0M_ZERO, (i4)HCB_CB,
	    (i4)HCB_ASCII_ID, (char *)svcb, 
	    (DM_OBJECT **)&svcb->svcb_hcb_ptr, &dmc->error);
	if (status != E_DB_OK)
	    break;

	/* Initialize the lctx mutex. */

	dm0s_minit(&svcb->svcb_lctx_ptr->lctx_mutex, "LCTX mutex");

	/*  Initialize the HCB. */

	hcb = svcb->svcb_hcb_ptr;
	dm0s_minit(&hcb->hcb_tq_mutex, "HCB tq");
	hcb->hcb_ftcb_list.q_next = &hcb->hcb_ftcb_list;
	hcb->hcb_ftcb_list.q_prev = &hcb->hcb_ftcb_list;
	hcb->hcb_tcbcount = hcb->hcb_tcbreclaim = 0;
	/* First mutex is after last hash entry */
	hcb->hcb_mutex_array = (DM_MUTEX *) &hcb->hcb_hash_array[HCB_HASH_ARRAY_SIZE];
	for (i = 0; i < DM_MAX_CACHE; i++)
	    hcb->hcb_cache_tcbcount[i] = 0;
	/* dmf_hash_size is the old deprecated parameter, use it to set
	** dmf_tcb_limit default if the latter wasn't given.
	*/
	if (c_tcb_limit == 0)
	    c_tcb_limit = c_tcb_hash * 8;
	if (c_tcb_limit < 500 /* FIXME */ )
	    c_tcb_limit = 500;
	hcb->hcb_tcblimit = c_tcb_limit;
	for (i = 0; i < HCB_HASH_ARRAY_SIZE; i++)
	{
	    hcb->hcb_hash_array[i].hash_q_next = (DMP_TCB*)
		&hcb->hcb_hash_array[i].hash_q_next;
	    hcb->hcb_hash_array[i].hash_q_prev = (DMP_TCB*)
		&hcb->hcb_hash_array[i].hash_q_next;
	}
	for (i = 0; i < HCB_MUTEX_ARRAY_SIZE; i++)
	{
	    dm0s_minit(&hcb->hcb_mutex_array[i],
		STprintf(sem_name, "HCB hash %d", i));
	}

	/*  Notify SCF of Control C/User Interrupt handler. */

	scf_cb.scf_facility = DB_DMF_ID;
	scf_cb.scf_nbr_union.scf_amask = SCS_AIC_MASK | SCS_ALWAYS_MASK;
	scf_cb.scf_ptr_union.scf_afcn = dmc_ui_handler;
	status = scf_call(SCS_DECLARE, &scf_cb);
	if (status != OK)
	{
	    uleFormat(&scf_cb.scf_error, 0,
		NULL, ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	    SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
	    break;
	}
	/*
	** Setup the FEXI functions
	*/
	{
	    ADF_CB		    adf_scb;
	    FUNC_EXTERN DB_STATUS   dmf_tbl_info();
	    FUNC_EXTERN DB_STATUS   dmf_last_id();
	    FUNC_EXTERN DB_STATUS   dmf_get_srs();

	    MEfill(sizeof(ADF_CB),0,(PTR)&adf_scb);

	    status = adg_add_fexi(&adf_scb, ADI_01PERIPH_FEXI, dmpe_call);
	    if (status != E_DB_OK)
	    {
		uleFormat(&adf_scb.adf_errcb.ad_dberror, 0,
		    0, ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		break;
	    }

	    status = adg_add_fexi(&adf_scb, ADI_03ALLOCATED_FEXI, 
				  dmf_tbl_info );
	    if (status != E_DB_OK)
	    {
		uleFormat(&adf_scb.adf_errcb.ad_dberror, 0,
		    0, ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		break;
	    }

	    status = adg_add_fexi(&adf_scb, ADI_04OVERFLOW_FEXI, 
				  dmf_tbl_info );
	    if (status != E_DB_OK)
	    {
		uleFormat(&adf_scb.adf_errcb.ad_dberror, 0,
		    0, ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		break;
	    }
	    
	    status = adg_add_fexi(&adf_scb, ADI_07LASTID_FEXI, 
				  dmf_last_id );
	    if (status != E_DB_OK)
	    {
		uleFormat(&adf_scb.adf_errcb.ad_dberror, 0,
		    0, ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		break;
	    }
	    status = adg_add_fexi(&adf_scb, ADI_08GETSRS_FEXI,
				  dmf_get_srs );
	    if (status != E_DB_OK)
	    {
		uleFormat(&adf_scb.adf_errcb.ad_dberror, 0,
		    0, ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		break;
	    }
	}

	/* initialize readahead control structures */
	if (c_numrahead)
	{
	    i4		numreqs;  
	    DMP_PFH	*dm_prefet;
	    DMP_PFREQ	*pfr, *last_pfr;

	    /* alloc 2 request blocks per RA thread */
	    numreqs = 2 * c_numrahead;
	    status = dm0m_allocate(sizeof(DMP_PFH) + 
				    numreqs * sizeof(DMP_PFREQ), 
				    DM0M_ZERO, (i4)PFH_CB,
				    (i4)PFH_ASCII_ID, (char *)svcb, 
				    (DM_OBJECT **)&svcb->svcb_prefetch_hdr, &dmc->error);
	    if (status != E_DB_OK)
	        break;

            /* initialize the prefetch queue header. This is the anchor for  */
            /* a queue of readahead events, and contains information that is */
            /* global to the entire read-ahead mechanism. (eg mutex, stats)  */
	    dm_prefet = svcb->svcb_prefetch_hdr;

            /* init the readahead queue mutex  */
            if ((svcb->svcb_status & SVCB_SINGLEUSER) == 0)
            {
            	CSi_semaphore(&dm_prefet->pfh_mutex, CS_SEM_SINGLE);
            	CSn_semaphore(&dm_prefet->pfh_mutex, "Readahead mutex" );
            }
	    dm_prefet->pfh_numreqs = numreqs;
	
	    /* create a doubly linked circular list of prefetch request  */
	    /* blocks with both the 'next' and 'free' ptrs pointing to   */
	    /* the 'head' of the list.					 */
	    pfr = (DMP_PFREQ *) ((char *)dm_prefet + sizeof(DMP_PFH));
	    dm_prefet->pfh_next = dm_prefet->pfh_freehdr = pfr;
	    last_pfr = pfr + (numreqs - 1);
	    while (numreqs--)
	    {
	  	pfr->pfr_status |= PFR_FREE;
		pfr->pfr_next = numreqs ? pfr + 1 : dm_prefet->pfh_freehdr;
		pfr->pfr_prev = last_pfr;

		last_pfr = pfr;
		pfr = pfr->pfr_next;
	    }
	}
	/*
	** init replicator control block
	*/
	Dmc_rep = NULL;
	if (c_rep_qsize)
	{
	    SIZE_TYPE	   pages = ((sizeof(DMC_REP) + c_rep_qsize * 
				sizeof(DMC_REPQ) - 1) / ME_MPAGESIZE) + 1;
	    SIZE_TYPE	   pages_got;
	    CL_ERR_DESC	   sys_err;
	    /*
	    ** round up to make full use of memory
	    */
	    c_rep_qsize = ((pages * ME_MPAGESIZE) - sizeof(DMC_REP)) /
		sizeof(DMC_REPQ);
	    /*
	    ** get shared memory
	    */
	    stat = MEget_pages(ME_SSHARED_MASK | ME_MZERO_MASK | ME_CREATE_MASK | ME_NOTPERM_MASK,
		pages, DMC_REP_MEM, (PTR *)&Dmc_rep, &pages_got, &sys_err);
	    if (stat == OK && pages_got == pages)
	    {
		/*
		** init semaphore
		*/
	        stat = CSi_semaphore(&Dmc_rep->rep_sem, CS_SEM_MULTI);
	        if (stat != OK)
	        {
		    SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		    break;
	        }
                CSn_semaphore(&Dmc_rep->rep_sem, "Dmc rep Mutex");
		/*
		** then grab it and init header
		*/
		stat = CSp_semaphore(TRUE, &Dmc_rep->rep_sem);
		if (stat != OK)
	        {
		    SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		    break;
	        }
		Dmc_rep->seg_size = c_rep_qsize;
		Dmc_rep->queue_start = Dmc_rep->queue_end = 0;
		stat = CSv_semaphore(&Dmc_rep->rep_sem);
		if (stat != OK)
	        {
		    SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		    break;
	        }
	    }
	    else if (stat == ME_ALREADY_EXISTS) /* just attach to it */
	    {
		stat = MEget_pages(ME_SSHARED_MASK | ME_NOTPERM_MASK, pages, 
                    DMC_REP_MEM, (PTR *)&Dmc_rep, &pages_got, &sys_err);
		if (stat != OK || pages_got != pages)
	        {
		    SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		    break;
	        }
	    }
	    else
	    {
		SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		Dmc_rep = NULL;
		break;
	    }
	}

	/*
	** init encryption key block
	*/

	Dmc_crypt = NULL;
	if (c_crypt_maxkeys)
	{
	    SIZE_TYPE	   pages = ( (sizeof(DMC_CRYPT) +
				c_crypt_maxkeys * sizeof(DMC_CRYPT_KEY) )
				/ ME_MPAGESIZE) + 1;
	    SIZE_TYPE	   pages_got;
	    CL_ERR_DESC	   sys_err;
	    /*
	    ** round up to make full use of memory
	    */
	    c_crypt_maxkeys = ((pages * ME_MPAGESIZE) - sizeof(DMC_CRYPT)) /
		sizeof(DMC_CRYPT_KEY);
	    TRdisplay("Encryption maximum active keys = %d\n", c_crypt_maxkeys);
	    /*
	    ** get shared memory
	    */
	    stat = MEget_pages(
		ME_SSHARED_MASK|ME_MZERO_MASK|ME_CREATE_MASK|ME_NOTPERM_MASK,
		pages, DMC_CRYPT_MEM, (PTR *)&Dmc_crypt, &pages_got, &sys_err);
	    if (stat == OK && pages_got == pages)
	    {
		/*
		** init semaphore
		*/
	        stat = CSi_semaphore(&Dmc_crypt->crypt_sem, CS_SEM_MULTI);
	        if (stat != OK)
	        {
		    SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		    break;
	        }
                CSn_semaphore(&Dmc_crypt->crypt_sem, "Dmc crypt Mutex");
		/*
		** then grab it and init header
		*/
		stat = CSp_semaphore(TRUE, &Dmc_crypt->crypt_sem);
		if (stat != OK)
	        {
		    SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		    break;
	        }
		Dmc_crypt->seg_size = c_crypt_maxkeys;
		Dmc_crypt->seg_active = 0;
		stat = CSv_semaphore(&Dmc_crypt->crypt_sem);
		if (stat != OK)
	        {
		    SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		    break;
	        }
	    }
	    else if (stat == ME_ALREADY_EXISTS) /* just attach to it */
	    {
		stat = MEget_pages(ME_SSHARED_MASK|ME_NOTPERM_MASK, pages, 
                    DMC_CRYPT_MEM, (PTR *)&Dmc_crypt, &pages_got, &sys_err);
		if (stat != OK || pages_got != pages)
	        {
		    SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		    break;
	        }
	    }
	    else
	    {
		SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		Dmc_crypt = NULL;
		break;
	    }
	}

	/*  Mark server as ACTIVE. */

	svcb->svcb_status |= SVCB_ACTIVE;

	/*
	** Check for existence of DMF gateway.
	** Call Gateway initialization routine - this will return whether or
	** not there is a gateway linked into the server.
	**
	** If a gateway is present, then gateway tables may be opened by this
	** server.  Also check whether gateway transactions are required.
	**
	** NOTE that if there is a gateway which requires transactions in order
	** to process updates, then the server must call into the gateway on
	** each BEGIN and END transaction call, regardless of whether a gateway
	** table is actually accessed during the transaction.
	*/
	status = dmf_gwf_init(&gateway_present, &gw_xacts, &dmc->error);
	if (status != E_DB_OK)
	    break;
	svcb->svcb_gateway = gateway_present;
	svcb->svcb_gw_xacts = gw_xacts;

	if (dmc->dmc_flags_mask & DMC_RECOVERY)
	{
	    status = dmr_log_init();
	    if (status)
	    {
		SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		break;
	    }
	}
	
	/* Include shortterm SCB mem pool when allocating DML_SCB */
	dmc->dmc_scfcb_size = sizeof(DML_SCB) + svcb->svcb_st_ialloc;
	dmc->dmc_server = (PTR)svcb;

	if (gc_numticks_changed)
	{
	    stat = LGalter(LG_A_GCMT_NUMTICKS, (PTR)&svcb->svcb_gc_numticks,
			    sizeof(svcb->svcb_gc_numticks), &sys_err);
	    if (stat != OK)
	    {
		uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL, (char *)NULL,
			    (i4)0, (i4 *)NULL, &err_code, 0);
		uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err,
			    ULE_LOG, NULL, (char *)NULL,
			    (i4)0, (i4 *)NULL, &err_code,
			    1, 0, LG_A_GCMT_NUMTICKS);
		SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		status = E_DB_ERROR;
		break;
	    }
	}
	if (gc_threshold_changed)
	{
	    stat = LGalter(LG_A_GCMT_THRESHOLD, (PTR)&svcb->svcb_gc_threshold,
			    sizeof(svcb->svcb_gc_threshold), &sys_err);
	    if (stat != OK)
	    {
		uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL, (char *)NULL,
			    (i4)0, (i4 *)NULL, &err_code, 0);
		uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err,
			    ULE_LOG, NULL, (char *)NULL,
			    (i4)0, (i4 *)NULL, &err_code,
			    1, 0, LG_A_GCMT_THRESHOLD);
		SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		status = E_DB_ERROR;
		break;
	    }
	}

#if defined(conf_CLUSTER_BUILD)
	if (clustered && !CXconfig_settings(CX_HAS_CSP_ROLE))
	{
	    LG_PROCESS		lpb;
	    struct _TMP_LG_ID
	    {
		u_i2	id_instance;	/* The instance of this identifier. */
		u_i2	id_id;		/* The identifier. */
	    } *tmp_lg_id;

	    /* Get LG unique id for this process */
	    lpb.pr_id = svcb->svcb_lctx_ptr->lctx_lgid;
	    status = LGshow(LG_S_APROCESS, (PTR)&lpb, sizeof(lpb), 
				    &length, &sys_err);

	    if (status != OK)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 0);
		uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, 0, ULE_LOG, NULL, 
			    NULL, 0, NULL, &err_code, 0);
		SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		break;
	    }

	    /*
	    ** Notify the CSP of the class start.
	    */
	    tmp_lg_id = (struct _TMP_LG_ID *)&lpb.pr_id;
	    svcb->svcb_csp_dbms_id = tmp_lg_id->id_id;
	    status = dmfcsp_class_start(svr_class_name, local_node_number,
					dmc->dmc_id, svcb->svcb_csp_dbms_id);
	    if (status != OK)
	    {
		SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
		break;
	    }
	}
#endif
	return (E_DB_OK);
    }

    /*	Handle error reporting and recovery. */

    if (dmc->error.err_code > E_DM_INTERNAL)
    {
	uleFormat(&dmc->error, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	SETDBERR(&dmc->error, 0, E_DM007F_ERROR_STARTING_SERVER);
    }
    if (bufmgr_allocated && dm0p_deallocate(&local_dberr))
	uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
		    &local_err_code, 0);
    if (log_allocated && dm0l_deallocate(&svcb->svcb_lctx_ptr, &local_dberr))
	uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
		    &local_err_code, 0);

    return (E_DB_ERROR);
}

/*{
** Name: dmc_ui_handler - User Interrupt handler.
**
**  INTERNAL DMF call format:    (STATUS) dmc_ui_handler(event_id, &scf_cb);
**
**
** Description:
**      This is the user interrupt handler routine required by SCF to
**      deliver the aic when a user hits control c.
**
** Inputs:
**      event_id                            Type of event causing interrupt.
**      scb                                 Pointer to DMF's session control
**                                          block.
**
** Output:
**      none
** History:
**      06-mar-87 (jennifer)
**          Created.
**      08-Dec-87 (ac)
**          Added force abort support.
**	23-aug-1993 (andys)
**          When dmc_ui_handler is called because of a "remove session"
**	    in iimonitor, it is possible that the session control blocks 
**	    for the session to be removed have not yet been set up properly. 
**	    If this is the case then we simply return. [bug 45020]
**	    [was 02-sep-1992 (andys)]
**	25-jun-98 (stephenb)
**	    Add support for CS_CTHREAD_EVENT
**	08-Jan-2001 (jenjo02)
**	    This function now called with *DML_SCB, not *DML_SCF.
**	20-Apr-2004 (jenjo02)
**	    If force abort and session wants to continue
**	    (logfull commit), return FAIL, let only DMF 
**	    be aware of and deal with force-abort.
**	3-May-2004 (schka24)
**	    Rename CUT thread event, OR it instead of setting it.
**	10-May-2004 (jenjo02)
**	    Use CS_KILL_EVENT signal to kill a query rather than
**	    CS_RCVR_EVENT, which aborted the entire transaction
**	    rather than just the query and was confusing and
**	    conflicted with ON_LOGFULL continue options.
**	11-Sep-2006 (jonj)
**	    Discard check of scb_ui_state & SCB_FORCE_ABORT,
**	    recursive DMF calls (see blobs) may ignore the
**	    force-abort until the transaction is at an
**	    atomic point in execution.
**	26-Feb-2007 (kschendel) b122040
**	    Set SCB flag, not XCB flag, for query kill.  The XCB flag
**	    shows that we've noticed the interrupt and have started
**	    aborting, which isn't the goal.  Need to set the SCB user
**	    interrupt flag for query kill to take effect.
*/
static STATUS
dmc_ui_handler(
i4   event_id,
DML_SCB   *scb)
{
    DB_STATUS status;
    CL_SYS_ERR sys_err;

    if ( scb )
    {
	if (event_id == CS_ATTN_EVENT)
	    scb->scb_ui_state = SCB_USER_INTR;
	else if (event_id == CS_CTHREAD_EVENT)
	    /* Include CUT attn with other states */
	    /* *Note* As of May '04, nobody pays attention to this state,
	    ** and it's not cleared by anything once set.  If/when someone
	    ** wants to notice CUT attn, some cleanup will be needed.
	    */
    	    scb->scb_ui_state |= SCB_CTHREAD_INTR;
	else if ( event_id == CS_RCVR_EVENT )
	{
	    /* 
	    ** Force abort event. Mark the session force abort only 
	    ** if there is a transaction in the session.
	    */
	    if ( scb->scb_x_ref_count )
	    {
		/* "on logfull commit"? */
		if ( scb->scb_sess_mask & SCB_LOGFULL_COMMIT )
		{
		    scb->scb_ui_state = SCB_FORCE_ABORT;

                    status = LKalter(LK_A_FORCEABORT, scb->scb_lock_list, 
                                                      &sys_err);
		    /*
		    ** DMF will handle force abort,
		    ** no other facilities will see
		    ** the interrupt.
		    */
		    return(FAIL);
		}
		/* Regular force abort - everyone participates */
		scb->scb_ui_state = (SCB_FORCE_ABORT | SCB_RCB_FABORT);
                status = LKalter(LK_A_FORCEABORT, scb->scb_lock_list, 
                                                  &sys_err);
	    }
	}
	else if ( event_id == CS_KILL_EVENT )
	{
	    /*
	    ** Kill the query if the session is not otherwise being  
	    ** interrupted and if there's a transaction in progress
	    ** that is not otherwise being aborted.
	    */
	    if ( scb->scb_ui_state == 0 )
	    {
		DML_XCB	*xcb = (DML_XCB*)scb->scb_x_next;

		if ( xcb != (DML_XCB*)&scb->scb_x_next &&
		     (xcb->xcb_state & XCB_ABORT) == 0 )
		{
		    scb->scb_ui_state = SCB_USER_INTR;
		}
	    }
	}
    }

    return(OK);
}

/*{
** Name: dmc_setup_checksum - Check if checksums desired
**
**
** Description:
**      This function sets up svcb_checksum according to the setting
**	of the ii.*.dbms.page_checksums PM variable.  The default is
**	to enable checksums.
**
**	The routine is called both by the regular and recovery 
**	servers, and by the CSP.  
**
** Inputs:
**      none
**
** Output:
**      none
**
** History:
**      15-oct-92 (jnash)
**          Created as part of the reduced logging project.
**	14-dec-1992 (jnash)
**	    Change default behavior to enable page checksums.
**	26-apr-1993 (jnash)
**	    Change II_DBMS_CHECKSUMS to be PM parameter:
**	    ii.*.dbms.page_checksums.
*/
VOID
dmc_setup_checksum(VOID)
{
    char	*pm_str;
    char 	*c1;
    char	*c2;
    char	*c3;
    STATUS	status;
    i4	err_code;

    /*
    ** Default is to enable checksums.
    */
    dmf_svcb->svcb_checksum = TRUE;

    status = PMget("ii.$.dbms.page_checksums", &pm_str);
    if (status == OK)
    {
	c1 = pm_str++;
	c2 = pm_str++;
	c3 = pm_str;

        if ((CMcmpnocase(c1, "O") == 0) &&
	    (CMcmpnocase(c2, "F") == 0) &&
	    (CMcmpnocase(c3, "F") == 0))
	{
	    /*
	    ** Note that the user has disabled checksums.  This could 
	    ** potentially be a bad thing if done inappropriately.
	    */
	    dmf_svcb->svcb_checksum = FALSE;
	    uleFormat(NULL, E_DM9056_CHECKSUM_DISABLE, (CL_ERR_DESC *)NULL, 
		ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 
		1, STlength(SystemCfgPrefix), SystemCfgPrefix);
	}
        else if ((CMcmpnocase(c1, "O") != 0) || 
		 (CMcmpnocase(c2, "N") != 0))
	{
	    /*
	    ** ON and OFF are the only valid arguments.
	    */
	    uleFormat(NULL, E_DM9057_PM_CHKSM_INVALID, (CL_ERR_DESC *)NULL, 
		ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 
		1, STlength(SystemCfgPrefix), SystemCfgPrefix);
	}
    }
    else if (status != PM_NOT_FOUND)
    {
	/*
	** Issue error but otherwise ignore it.  
	*/
        uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
        return;
    }
}


/*{
** dmcm_lkinit - Initialises/Verifies the DMCM protocol lock.
**
** Description:
**   If one DBMS server in an installation is running the Distributed
**   Multi-Cache Management (DMCM) procotol, then all DBMS servers in the
**   installation must be. Otherwise the differing cache locking strategies
**   are likely to cause severe - unpredicatable - problems.
**
**   This routine, when called by the RCP, initialises the DMCM lock, key
**   "II_DMCM", which will be used to verify that all DBMS servers use the
**   same protocol. If the installation is configured for DMCM, then the
**   value DMCM_LOCK_VALUE is written into the lock_value. If the
**   installation is not configured for DMCM, then the value 
**   NON_DMCM_LOCK_VALUE is written into the lock value.
**
**   When called by DBMS servers, the DMCM lock is taken. If we are
**   running DMCM, the lock_value is compared against DMCM_LOCK_VALUE.
**   If we are not running DMCM, the lock_value is compared against
**   NON_DMCM_LOCK_VALUE. 
**
**   A failure of these comparisons causes STATUS to be set to E_DB_ERROR 
**   and the err_code E_DM0171_BAD_CACHE_PROTOCOL to be returned.
**
** Inputs:
**   lock_list - A lock list on which the DMCM lock will be created.
**   dmcm      - Protocol, are we running DMCM?
**   create    - Mode, create lock if we're the RCP.
** 
** Outputs:
**   NONE
**
** Returns:
**   E_DB_OK        Function completed successfully.
**   E_DB_ERROR     Function completed abnormally with a termination 
**                  status which is in err_code.
**
** History:
**	01-aug-1996 (nanpr01 for ICL phil.p)
**          Created.
**	07-feb-2005 (devjo01)
**	    Change stategy for obtaining DMCM control lock.  If 'create' 
**	    set, 1st try LK_X NOWAIT, if gotten then we set value and
**	    convert down to LK_S.  Otherwise value is just read from
**	    a share lock.  If not an RCP lock is released.
**	    This is done to prevent the LVB for this lock from becoming
**	    invalidated in a partial cluster failure as would be the case 
**	    in the previous algorithm where we converted down tp LK_N
**	    after reading the lock.  This would cause a schedule deadlock
**	    for a starting CSP if the lock was invalidated causing a stall.
*/
static STATUS
dmcm_lkinit(
    DMC_CB	*dmc,
    i4     lock_list,
    i4          dmcm,
    i4          create)
{
    LK_LOCK_KEY    lock_key;
    LK_VALUE       lock_value;
    LK_LKID        lock_id;
    STATUS         status, lstatus;
    i4		   mode;
    i4		   error;
    CL_ERR_DESC    sys_err;

    CLRDBERR(&dmc->error);

#define DMCM_LOCK_KEY  "II_DMCM"
#define DMCM_LOCK_VALUE  2
#define NON_DMCM_LOCK_VALUE  1

    lock_key.lk_type = LK_CONTROL;
    MEmove(STlength(DMCM_LOCK_KEY),
           DMCM_LOCK_KEY,
           ' ',
           6 * sizeof(lock_key.lk_key1),
           (PTR) &lock_key.lk_key1);
    MEfill(sizeof(LK_LKID), 0, &lock_id);
	
    if ( create )
    {
	mode = LK_X;
	status = LKrequest((LK_PHYSICAL | LK_NODEADLOCK | LK_NOWAIT),
	    (LK_LLID) lock_list, &lock_key, LK_X, &lock_value,
	    &lock_id, (i4)0, &sys_err);
    }

    if ( !create || (LK_BUSY == status) || (LK_UBUSY == status) )
    {
	mode = LK_S;
	status = LKrequest((LK_PHYSICAL | LK_NODEADLOCK),
	    (LK_LLID) lock_list, &lock_key, LK_S, &lock_value,
	    &lock_id, (i4)0, &sys_err);
    }

    if ((status != OK) && (status != LK_VAL_NOTVALID))
    {
        uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
            (char * )NULL, (i4)0, (i4 *)NULL, &error, 0);
        uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG, NULL,
            (char *)NULL, (i4)0, (i4 *)NULL, &error, 2,
	    0, mode, 0, lock_list);
	SETDBERR(&dmc->error, 0, E_DM0172_DMCM_LOCK_FAIL);
        return(E_DB_ERROR);
    }
    else
    {
        if (create && mode == LK_X)
        {
            /* 
            ** Initialise lock 
            */
            if (dmcm)
            {
                lock_value.lk_high = DMCM_LOCK_VALUE;
                lock_value.lk_low = DMCM_LOCK_VALUE;
            }
            else
            {
                lock_value.lk_high = NON_DMCM_LOCK_VALUE;
                lock_value.lk_low = NON_DMCM_LOCK_VALUE;
            } 
     
            /*
            ** Convert down to Share
            */
            status = LKrequest(LK_PHYSICAL | LK_CONVERT | LK_NODEADLOCK,
                (LK_LLID) lock_list, (LK_LOCK_KEY *) NULL,
                LK_S, &lock_value, &lock_id, (i4)0, &sys_err);
    
            if (status != OK)
            {
		uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		    (char * )NULL, (i4)0, (i4 *)NULL, &error, 0);
                uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG, NULL,
                    (char *)NULL, (i4)0, (i4 *)NULL, &error, 2,
	            0, LK_N, 0, lock_list);
		SETDBERR(&dmc->error, 0, E_DM0172_DMCM_LOCK_FAIL);
                status = E_DB_ERROR;
            }
        }
        else
        {
           /*
           ** Consistent protocol usage ?
           ** Check value block.
           */
           if (dmcm)
           {
               if ((lock_value.lk_high != DMCM_LOCK_VALUE) ||
                   (lock_value.lk_low != DMCM_LOCK_VALUE))
               {
		   SETDBERR(&dmc->error, 0, E_DM0171_BAD_CACHE_PROTOCOL);
		   status = E_DB_ERROR;
               }
           }
           else
           {
               if ((lock_value.lk_high != NON_DMCM_LOCK_VALUE) ||
                   (lock_value.lk_low != NON_DMCM_LOCK_VALUE))
               {
		   SETDBERR(&dmc->error, 0, E_DM0171_BAD_CACHE_PROTOCOL);
		   status = E_DB_ERROR;
               }
           }
	}

	if ( !create || status == E_DB_ERROR )
	{
           lstatus = LKrelease(0, (LK_LLID) lock_list, &lock_id,
                     (LK_LOCK_KEY *)NULL, &lock_value, &sys_err);

           if (lstatus != OK)
           {
	       uleFormat(NULL, lstatus, &sys_err, ULE_LOG, NULL,
	           (char * )NULL, (i4)0, (i4 *)NULL, &error, 0);
               uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
                   (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
	           0, lock_list);
	       if ( E_DB_OK == status )
		    SETDBERR(&dmc->error, 0, E_DM0172_DMCM_LOCK_FAIL);
               status = E_DB_ERROR;
           }
       }
    } 
    return(status);
}

/*{
** Name: rcp_dmf_cache_size - Determine cache size for RCP server
**
**
** Description:
**      The default cache size for the recovery server is 200 pages
**      for each valid page size. This is necessary because
**      the recovery server needs to be ready to handle any situation.
**      However, since this may be wasteful, we will allow this
**      to be configured. 
**
**      It is true that if a particular cache was not allocated
**      by the recovery server, it will be allocated when needed.
**      However, it would be fatal if the recovery server had 
**      insufficient memory to allocate the cache.
** 
**      Therefore, a cache should be allocated with zero pages only
**      if the recovery server would NEVER need to read/write
**      tables that size.
**
** Inputs:
**      none
**
** Output:
**      none
**
** History:
**      06-mar-96 (stial01)
**          Created for variable page size project.
**	30-jul-2001 (stephenb)
**	    Fixed #endif to quiet compiler
*/
static VOID
rcp_dmf_cache_size(
i4 buffers[],
i4 cache_flags[])
{
    char	*pm_str;
    STATUS	status;
    i4	err_code;
    i4     cache_size;
    i4     pagesize;
    i4          cache_ix;
    char        *cacheparm;

#ifdef debug_xxx
    status = PMget("ii.$.rcp.dmf_cache_size_debug", &pm_str);
    if (status == OK)
    {
	for (cache_ix = 0; cache_ix < DM_MAX_CACHE; status++)
	{
	    cache_ix = 0;
	}
    }
#endif /* debug_xxx */

    /*
    ** Default is 200 buffers for each page size.
    */
    for (cache_ix = 0, pagesize = 2048; cache_ix < DM_MAX_CACHE;
				cache_ix++, pagesize *= 2)
    {
	switch (pagesize)
	{
	    case 2048:  cacheparm = "ii.$.rcp.dmf_cache_size"; 
			break;
	    case 4096:  cacheparm = "ii.$.rcp.dmf_cache_size4k";
			break;
	    case 8192:  cacheparm = "ii.$.rcp.dmf_cache_size8k";
			break;
	    case 16384: cacheparm = "ii.$.rcp.dmf_cache_size16k";
			break;
	    case 32768: cacheparm = "ii.$.rcp.dmf_cache_size32k";
			break;
	    case 65536: cacheparm = "ii.$.rcp.dmf_cache_size64k";
			break;
	}

	/*
	** The default is to allocate a RCP cache with 200 pages.
	** Don't allow RCP cache to be too small, it really
	** screws up log space reservation.
	*/
	buffers[cache_ix] = 200;

	status = PMget(cacheparm, &pm_str);
	if (status == OK)
	{
	    status = CVal(pm_str, &cache_size);
	    if (status == OK)
	    {
		if (cache_size > 0)
		    buffers[cache_ix] = cache_size;
		else
		    cache_flags[cache_ix] |= DM0P_DEFER_PGALLOC;
	    }
	    else
	    {
		/* FIX ME Issue error, but otherwise ignore it */
	    }
	}
    }
}

/*
** Name: dmc_start_name_id - Set up server name generator ID
**
** Description:
**	Regular server-ish DMF's generate temporary names using a
**	fairly simple sequential number.  In order to make sure that
**	multiple servers don't crash into one another, the low order
**	few bits (9 bits) of the sequential number are a server
**	ID, which is held in the SVCB svcb_tfname_id field.  This
**	routine initializes that ID to 1 through 511.
**
**	So that ID choosing works in pretty much any context, we'll
**	do it by attempting to take a nowait control lock on the key
**	II_TFNAME_IDnnn where nnn is the ID number we're trying
**	for.  As soon as we get a lock, that's our ID.  In most
**	installations there will only be a very few ID's handed out.
**	This scheme works even if servers are stopped and restarted
**	many many times, as long as the total number of servers is
**	never more than 511.
**
** Inputs:
**	svcb		The server control block
**
** Outputs:
**	Returns OK or E_DB_ERROR
**	The svcb->svcb_tfname_id is set to a number 1 through 511.
**
** History:
**	6-Feb-2004 (schka24)
**	    Written for new naming scheme that gets rid of the DMU
**	    statement count limit.  The  limit was a problem for
**	    partitioned tables.
**	4-Jun-2005 (schka24)
**	    Make sure filename generators are inited.
*/

static DB_STATUS
dmc_start_name_id(DM_SVCB *svcb)
{
    char id_key[20];			/* II_TFNAME_IDnnn */
    CL_ERR_DESC sys_err;
    LK_LOCK_KEY lock_key;
    LK_VALUE lock_value;
    LK_LKID lock_id;
    STATUS st;

    svcb->svcb_tfname_gen = 0;
    svcb->svcb_hfname_gen = 0;
    for (svcb->svcb_tfname_id = 1;
	 svcb->svcb_tfname_id <= 511;
	 ++ svcb->svcb_tfname_id)
    {
	STprintf(id_key, "II_TFNAME_ID%d", svcb->svcb_tfname_id);
	lock_key.lk_type = LK_CONTROL;
	MEmove(STlength(id_key),
		id_key,
		' ',
		6 * sizeof(lock_key.lk_key1),
		(PTR) &lock_key.lk_key1);
	MEfill(sizeof(LK_LKID), 0, &lock_id);
 
	st = LKrequest((LK_PHYSICAL | LK_NOWAIT | LK_NODEADLOCK),
		svcb->svcb_lock_list, &lock_key, LK_X, &lock_value,
		&lock_id, (i4)0, &sys_err);

	if (st == OK)
	{
	    TRdisplay("%@ DMF server ID %d chose name-generator ID: %d\n",
		svcb->svcb_id, svcb->svcb_tfname_id);
	    return (E_DB_OK);
	}

    }

    /* FIXME an errlog message might be good here */
    TRdisplay("%@ All attempts to get a temporary name ID failed.\n");
    return (E_DB_ERROR);
}
