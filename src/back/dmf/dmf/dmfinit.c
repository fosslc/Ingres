/*
** Copyright (c) 1986, 2004 Ingres Corporation
**
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <di.h>
#include    <er.h>
#include    <pc.h>
#include    <ex.h>
#include    <tr.h>
#include    <cv.h>
#include    <nm.h>
#include    <tmtz.h>
#include    <ci.h>
#include    <me.h>
#include    <st.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lk.h>
#include    <cx.h>
#include    <lo.h>
#include    <pm.h>
#include    <lg.h>
#include    <ulf.h>
#include    <adf.h>
#include    <add.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dm0m.h>
#include    <sxf.h>
#include    <dmxe.h>
#include    <dm2t.h>
#include    <dm0p.h>
#include    <dm0c.h>
#include    <dm1b.h>
#include    <dmftrace.h>
#include    <dmfjsp.h>
#include    <dmfinit.h>
#ifdef VMS
#include    <ssdef.h>
#include    <starlet.h>
#include    <astjacket.h>
#endif

/**
** Name: DMFINIT.C - Establish a working environment for the recovery process, 
**		     archiver, and recoverdb to use DMF, ADF & SXF.
**
** Description:
**
**	Function prototypes defined in DM.H.
**
**      This file contains the function that initializes the DMF server 
**	control block. 
**
**      This file defines:
**    
**      dmf_init() -  Routine to initialize the DMF server control block.
**	dmf_udt_lkinit() - Routine to request UDT Control Lock.
**
**  History:
**	22-Oct-1986 (ac)
**	    Created for Jupiter.
**	6-Sept-1988 (Edhsu)
**	    Process max_db passed in from dmfrcp
**	30-sep-1988 (rogerk)
**	    Added new arguments to dm0p_allocate.
**	28-feb-1989 (rogerk)
**	    Added new arguments to dm0p_allocate for shared buffer manager
**	    (flags, dbcache_size, tblcache_size, cache_name).
**	20-mar-1989 (rogerk)
**	    Don't allocate database or table cache when initializing through
**	    dmf_init().
**	20-apr-1989 (fred)
**	    Added code to support udadt's.  If this application is the RCP or
**	    CSP, and the lock for UDADT level does not exist, then create it,
**	    holding it in NULL mode.  Everyone else, hold the lock in NULL mode,
**	    so that it will survive system crashes.
**      05-sep-89 (jennifer)
**          Added code to initialize the svcb for security enhancements.
**	06-Oct-1989 (ac)
**	    Added DM0P_MASTER for creating the buffer manager lock list
**	    as a master lock list.
**	14-jun-1991 (Derek)
**	    Added logical name II_DMF_TRACE that can be used to set DMF trace
**	    flags in the DMF utilities.
**	25-jun-1991 (bonobo)
**	    Fixed the "unacceptable operand of &" su4 compiler problem.
**	25-sep-1991 (mikem) integrated following change: 19-nov-1990 (rogerk)
**	    Added svcb_timezone field to hold the timezone in which the DBMS
**	    server operates.  Internal DMF functions operate in this timezone.
**	    This was done as part of the DBMS Timezone changes.
**	15-oct-1991 (mikem)
**	    Include "st.h" to get correct definition of ST routines which 
**	    eliminates unix compiler warnings ("illegal combination of pointer 
**	    and integer, op =") about use of STindex().
**	13-may-1992 (bryanp)
**	    Initialize svcb_int_sort_size to 0 for non-server processes.
**	8-july-1992 (ananth)
**	    Prototyping DMF.
**      24-sep-1992 (stevet)
**          Replace TMzone with call to TMtz_init to load the default 
**          timezone information file.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**      15-oct-1992 (jnash)
**          Reduced logging project.  Add support for II_DBMS_CHECKSUMS
**	    logical, used to enable/disable checksumming.
**	02-nov-1992 (robf)
**	    For C2 installations initialize SXF in case auditing is
**	    needed.
**	    Remove old B1 shared memory initialization.
**	12-oct-1992 (bryanp)
**	    Break UDT lock code out into separate subroutine so that Recovery
**	    Server code can call it.
**	    Initialize new svcb fields.
**      11-nov-92 (jnash)
**          Fix problem with linking CSP by removing dmf_write_msg() 
**	    calls, substituting ule_format.  Additional error reporting
**	    may be desirable.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	18-nov-92 (robf)
**	    Removed initialization of audit server control block, this
**	    is now handled by SXF.
**	18-jan-1993 (bryanp)
**	    Initialize svcb_cp_timer to 0.
**	16-feb-1993 (rogerk)
**	    Added call to dmm_init_catalog_templates needed now in all
**	    dmf processes that open databases.
**	10-feb-1993 (jnash)
**	    Add header files required to resolve dmc_setup_checksum()
**	    compiler warning. 
**	22-feb-1993 (markg)
**	    Put the call to check for user specified audit configuration data
**	    into an xDEBUG ifdef. Also removed references to the redundant
**	    variable "init_state" in dmf_init().
**	30-feb-1993 (rmuth)
**	    Include di.h for dm0c.h.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Project:
**		Include dmfinit.h for function prototypes.
**		When the CSP calls dmf_init, the local logging system is not yet
**		    ready to accept LGadd/LGbegin calls. So we can't call
**		    dm0p_allocate until we're ready to make those calls. Added
**		    allocate_buffer_manager argument to dmf_init to make this
**		    more clear.
**		Added have_log parameter to dm0p_allocate.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Add csp_lock_list parameter to dmf_init to allow dmfcsp to create
**		an LK_MASTER_CSP lock list.
**	30-sep-93 (stephenb)
**	    Set PM defaults for later use in JSP and CSP.
**	19-oct-93 (stephenb)
**	    Change C2 PM parameters in PMget() to be compatible with CBF.
**	16-dec-93 (tyler)
**	    Replaced GChostname() with call to PMhost().
**	31-jan-1994 (jnash)
**	    CSinitiate() now returns errors (called by scf_call(SCU_MALL0C)),
**	    so dump them.
**	18-apr-1994 (jnash)
**	    fsync project: Add dmf_setup_optim() to get config.dat 
**	    load optimization config param ("ii.$.rcp.load_optim").  
**	    Call routine in dmf_init().
**	25-apr-1994 (bryanp) integrated 27-sep-1993 (rachael)
**         Bug 53516 - the RCP,JSP(auditdb, ckpdb, rollforwarddb, etc) and CSP
**	   (in VAXclusters) must know about user defined datatypes (UDADTs), as
**         well as the DBMS server. To facilitate this, the INGRES system
**	   maintains a lock whose value block contains the major/minor pair
**	   associated with the versions of the datatypes in use. When any
**         component (DBMS, RCP, etc.) of the system starts, it checks that
**         the version of datatypes it has is compatible with the remainder
**         of the operational system. In the event of a mismatch, the
**         default action is to refuse to start.
**
**         This check is made in dmf!dmfinit.c ( the routine that
**         establishes a working environment for the RCP, CSP and the JSP
**         utilities). An LKrequest for the UDADT lock is made, in
**         exclusive mode and passing in an address for the lock_value. If
**         the LKrequest returns either OK or LK_VAL_NOTVALID, processing
**         continued. (The piccolo submission for bug 47950 contains a
**         complete description of these return values.  One of the fixes
**         bug 47950 made was to return LK_VAL_NOTVALID in all appropriate
**         situations.)
**
**         If the application making the LKrequest is the RCP or CSP, it is
**         OK not to find a value for this lock, the assumption being that
**         we are the first process to come up in this installation and we
**         will initialize the lock with the UDADT major/minor values
**         obtained from SCF. The lock is then converted down to a null
**         lock, passing in those values, thereby causing the lock value
**         block to be updated.
**
**         If the application is not the RCP or CSP, a check is made for
**         LK_VAL_NOTVALID.  If the status is not LK_VAL_NOTVALID,  the
**         value contained in the lock's value block is used to verify that
**         the current UDADT code level is compatible with the operational
**         INGRES installation. 
**	
**	   If the status is LK_VAL_NOTVALID, the value is assumed to be
**         untrustworthy; the UDADT level can't be determined, and the
**         application will fail to start.
**	   
**         However, the VMS Distributed Lock Manager invalidates locks more
**         often than one may expect.  Since in this particular case, we
**         can determine whether the value is valid, even though
**         LK_VAL_NOTVALID is returned, it seems reasonable to do so and
**         continue executing if the value is determined to be valid.  When
**         the lock is released, it will be made valid by the locking
**         system.
**
**         This problem did not occur prior to fixing bug 47950 which
**         resolved the problem of the Ingres lock manager not properly
**         returning LK_VAL_NOTVALID in all situations; it was returning a
**         normal lock completion instead.  Therefore, the JSP utilities
**         never received the LK_VAL_NOTVALID return and this problem did
**         not occur.
**	06-mar-1996 (stial01 for bryanp)
**	    Initialize svcb_page_size in the DM_SVCB.
**      06-mar-1996 (stial01)
**          Variable page size project
**          dmf_init() Added buffers parameter.
**          Changed dm0p_allocate call in support of multiple buffer caches.
**	23-sep-1996 (canor01)
**	    Moved global data definitions to dmfdata.c.
**	12-mar-1997 (cohmi01)
**	    Init hcb_tcblimit and related fields added for bug 80050.
**	24-Oct-1996 (jenjo02)
**	    Replaced hcb_mutex with hash_mutex, one for each hash entry.
**	    Added hcb_tq_mutex to protect TCB free queue.
**	17-Jan-1997 (jenjo02)
**	    Replaced svcb_mutex with svcb_dq_mutex (DCB queue) and
**	    svcb_sq_mutex (SCB queue).
**	15-aug-97 (stephenb)
**	    add DML_SCB parameter to dm2t_open() call.
**	07-May-1998 (jenjo02)
**	    Initialize hcb_cache_tcbcount array, set cache writebehind
**	    threads off.
**      27-jul-1998 (rigka01)
**          Cross integrate fix for bug 90691 from 1.2 to 2.0 code line
**          In highly concurrent environment, temporary file names may not
**          be unique so improve algorithm for determining names
**          Initialize svcb_DM2F_TempTblCnt_mutex
**	31-Aug-1998 (hanch04)
**	    Set sxf_pool_sz to SXF_BYTES_PER_SINGLE_SERVER for single server,
**	    such as dmfjsp.
**	22-Feb-1999 (bonro01)
**	    Remove references to svcb_log_id.
**      21-apr-1999 (hanch04)
**	    Replace STindex with STchr
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LKcreate_list() prototype.
**	15-Dec-1999 (jenjo02)
**	    Removed DB_DIS_TRAN_ID parm from LKcreate_list() prototype.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-jun-2001 (abbjo03)
**	    If we cannot map shared memory, change err_code to E_DM00AB instead
**	    of E_DM007F.
**	28-jun-2001 (devjo01)
**	    Add CX calls for UNIX cluster support. s103715.
**	    Correct some ule_format params to resolve compiler warnings.
**	05-Mar-2002 (jenjo02)
**	    Init svcb_seq for Sequence Generators.
**	09-sep-2003 (abbjo03)
**	    Add missing parameters in call to adg_startup().
**      20-sep-2004 (huazh01)
**         Rewrite the fix for b90691 and changed prototype 
**         for dmxe_commit().
**         INGSRV2643, b111502.
**	15-Jan-2004 (schka24)
**	    Change around hcb mutex array.
**	6-Feb-2004 (schka24)
**	    Set up name-generator environment.
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**      11-Jan-2005 (stial01)
**          Init svcb_lk_level
**      30-mar-2005 (stial01)
**          Init svcb_pad_bytes
**	05-Aug-2005 (hanje04)
**	    Back out change 478041 as it should never have been X-integrated
**	    into main in the first place.
**	3-Jan-2006 (kschendel)
**	    Mods to allow a caller to ask to attach to an existing
**	    shared cache.  (For fastload and Datallegro's fastdump.)
**      16-nov-2007 (shust01)
**          Casted parameters passed to sca_add_datatypes(). Was causing segv
**          due to zero being passed as an address was not being passed as 8 
**          bytes on 64 bit platform.  b119492.
**      21-Nov-2006 (kschendel)
**          We call SCA directly, which is illegal;  until fixed,
**          the call has to use the proper parameter types.
**	12-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_? functions converted to DB_ERROR *
**	21-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**      22-dec-2008 (stegr01)
**          Itanium VMS port.
[@history_template@]...
**/


/*
** Globals
*/
GLOBALREF DMP_DCB *dmf_jsp_dcb;	/* Current DCB  */
/*
**  Definition of static variables and forward static functions.
*/

static  PTR         dmf_adf_svcb = 0;	/* Adf's server control block */
static  PTR	    dmf_sxf_svcb = 0;	/* Sxf's server control block */ 
static  DB_STATUS   dmf_sxf_djc();	/* Dmf/Sxf mapping function */
static  DML_XCB	    dmf_jsp_xcb; 
static  DML_SCB	    dmf_jsp_scb;	
static    STATUS  ex_handler(
	EX_ARGS	*ex_args);	    /*	DMF catch-all exception handler. */


/*{
** Name: dmf_init - Initialize the DMF server control block.
**
**
** Description:
**    This command causes a DMF working environment for the recovery processs,
**    archiver and recoverdb to be established.
**
**	The dm0l_flag field holds logging system initialization flags;
**	these flags are passed through to dm0l_allocate as is. 
**
** Inputs:
**	dm0l_flag			Special initialize options.
**	connect_only			TRUE to connect to an existing
**					shared cache (but not create one).
**	cache_name			Name of shared cache if connecting.
**      buffers                         How many buffers for each page size.
**					Only used when NOT connect-only.
**	csp_lock_list			lock list creation flags (for CSP).
**	size				Log block size for calculating the
**					size of LCTX.
**	lg_id				The logging id to represent the
**					current process.
**	maxdb				max db specified in the config file
**
** Output:
**	err_code			    reason for error status.
**
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_ERROR                      Function completed abnormally.
**
** History:
**	Oct-22-1986 (ac)
**	    Created for Jupiter.
**	30-sep-1988 (rogerk)
**	    Added new arguments to dm0p_allocate.
**	28-feb-1989 (rogerk)
**	    Added new arguments to dm0p_allocate for shared buffer manager
**	    (flags, dbcache_size, tblcache_size, cache_name).
**	20-mar-1989 (rogerk)
**	    Don't allocate database or table cache when initializing through
**	    dmf_init().
**	20-apr-1989 (fred)
**	    Added code to support udadt's.  If this application is the RCP or
**	    CSP, and the lock for UDADT level does not exist, then create it,
**	    holding it in NULL mode.  Everyone else, hold the lock in NULL mode,
**	    so that it will survive system crashes.
**      05-sep-89 (jennifer)
**          Added code to initialize the svcb for security enhancements.
**	25-sep-1991 (mikem) integrated following change: 19-nov-1990 (rogerk)
**	    Added svcb_timezone field to hold the timezone in which the DBMS
**	    server operates.  Internal DMF functions operate in this timezone.
**	    This was done as part of the DBMS Timezone changes.
**	13-may-1992 (bryanp)
**	    Initialize svcb_int_sort_size to 0 for non-server processes.
**      24-sep-1992 (stevet)
**          Replace TMzone with call to TMtz_init to load the default 
**          timezone information file.
**      15-oct-1992 (jnash)
**          Reduced logging project.  Add support for II_DBMS_CHECKSUMS
**	    logical, used to enable/disable checksumming.
**	02-nov-1992 (robf)
**	    Remove old B1 shared memory initialization.
**	    For C2 initialize SXF facility in case auditing is needed.
**	20-oct-1992 (bryanp)
**	    Initialize new svcb fields.
**	17-dec-1992 (robf)
**	    Update initialization of C2 based on KM and PM values.
**	    Don't initialize SXF for the CSP.
**	10-dec-1992 (pholman)
**	    Initialize PM, and perform C2 checks via PM instead of CI
**	18-jan-1993 (bryanp)
**	    Initialize svcb_cp_timer to 0.
**	16-feb-1993 (rogerk)
**	    Added call to dmm_init_catalog_templates needed now in all
**	    dmf processes that open databases.
**	22-feb-1993 (markg)
**	    Put the call to check for user specified audit configuration data
**	    into an xDEBUG ifdef. Also removed references to the redundant
**	    variable "init_state".
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Project:
**		When the CSP calls dmf_init, the local logging system is not yet
**		    ready to accept LGadd/LGbegin calls. So we can't call
**		    dm0p_allocate until we're ready to make those calls. Added
**		    allocate_buffer_manager argument to dmf_init to make this
**		    more clear.
**		Added create_udt_lock argument to be more explicit about when
**		    dmf_init is expected to create a UDT lock value. Peeking
**		    into the dm0l_flag values was gross.
**	28-jun-1993 (robf)
**	    Secure 2.0:
**	    - Removed old ORANGE code, replaced by generic.
**	    - Initialize security state on startup.
**	26-jul-1993 (bryanp)
**	    Add csp_lock_list parameter to dmf_init to allow dmfcsp to create
**		an LK_MASTER_CSP lock list.
**	30-sep-93 (robf)
**          Set PM defaults, preserve C2/B1 mode in dmf_svcb
**	31-jan-1994 (jnash)
**	    CSinitiate() now returns errors (called by scf_call(SCU_MALL0C)),
**	    so dump them.
**	18-apr-1994 (jnash)
**	    fsync project: Call dmf_setup_optim() to setup load optim
**	    svcb field.
**	06-mar-1996 (stial01 for bryanp)
**	    Initialize svcb_page_size in the DM_SVCB.
**      06-mar-1996 (stial01)
**          Variable page size project:
**          Added buffers parameter.
**          Changed dm0p_allocate call in support of multiple buffer caches.
**	24-Oct-1996 (jenjo02)
**	    Replaced hcb_mutex with hash_mutex, one for each hash entry.
**	    Added hcb_tq_mutex to protect TCB free queue.
**	12-mar-1997 (cohmi01)
**	    Init hcb_tcblimit and related fields added for bug 80050.
**	8-apr-1997 (nanpr01)
**	    Use the defines rather than the hardcoded constants.
**      27-jul-1998 (rigka01)
**          Cross integrate fix for bug 90691 from 1.2 to 2.0 code line
**          In highly concurrent environment, temporary file names may not
**          be unique so improve algorithm for determining names
**          Initialize svcb_DM2F_TempTblCnt_mutex
**      31-aug-1999 (stial01)
**          Initialize svcb_etab_pgsize, svcb_etab_tmpsz in the DM_SVCB.
**	08-jun-2001 (abbjo03)
**	    If we cannot map shared memory, change err_code to E_DM00AB instead
**	    of E_DM007F.
**      20-sept-2004 (huazh01)
**          Pull out the fix for b90691 as it has been rewritten.
**          INGSRV2643, b111502. 
**	28-jun-2001 (devjo01)
**	    Add CX calls for UNIX cluster support. s103715.
**	5-Mar-2004 (schka24)
**	    TCB free list is now a real queue, fix here.
**	30-apr-2004 (thaju02)
**	    Initialize svcb_maxbtreekey.
**      11-may-2004 (stial01)
**          Removed unnecessary svcb_maxbtreekey
**	07-feb-2005 (devjo01)
**	    Change stategy for obtaining UDT control lock in dmf_udt_lkinit.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Call dm0m_init() to set up for SINGLEUSER memory use.
**	6-Jan-2006 (kschendel)
**	    Allow caller to specify connect-only to shared cache.
**	15-May-2007 (toumi01)
**	    For supportability add process info to shared memory.
*/
DB_STATUS
dmf_init(
i4		dm0l_flag,
bool		connect_only,
char		*cache_name,
i4         buffers[],
i4		csp_lock_list,
i4		size,
i4		lg_id,
i4		maxdb,
DB_ERROR	*dberr,
char		*lgk_info)
{
    DM_SVCB		*svcb;
    DMP_HCB		*hcb;
    i4		i;
    i4		c_database_max = maxdb;
    i4		c_session_max = 1;
    i4		c_memory_max = 128;
    DB_STATUS		status = E_DB_ERROR;
    CL_ERR_DESC		sys_err;
    i4			err;
    static DM_SVCB	local_svcb ZERO_FILL;
    i4		adf_size;
    SCF_CB		scf_cb;
    bool		c2_secure_server = FALSE;
    char                *nmname = NULL;
    STATUS              stat;
    SXF_RCB   	        sxf_rcb;
    char		*sec_mode;
    i4                  segment_ix;
    i4                  cache_ix;
    i4             flimit[DM_MAX_CACHE];
    i4             mlimit[DM_MAX_CACHE];
    i4             group[DM_MAX_CACHE];
    i4             gpages[DM_MAX_CACHE];
    i4             writebehind[DM_MAX_CACHE];
    i4             wbstart[DM_MAX_CACHE];
    i4             wbend[DM_MAX_CACHE];
    i4             cache_flags[DM_MAX_CACHE];
    char		sem_name[CS_SEM_NAME_LEN];
    i4			clustered;
    DB_ERROR		local_dberr;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** If in an Ingres cluster, notify CX that we are not
    ** the CSP, but require the CSP to be active.
    */
    if (clustered = CXcluster_enabled())
    {
	(VOID)CXalter( CX_A_NEED_CSP, (PTR)1 );
    }
    
    /*
    ** Set PM defaults
    */
    PMsetDefault( 0, ERx( "ii" ) );
    PMsetDefault( 1, PMhost() );

    /*
    **	If not the CSP, then check if we are C2-Secure
    **	This depends on:
    **  1) C2 MODE being set on ON.
    */
    {
	char 	*pmvalue;
	char 	*pmfile;
	LOCATION pmloc;
# ifdef xDEBUG
	/*
	**	Check if an override PM file is used
	**	(this is consistent with the usage within SXF)
	*/
	NMgtAt("II_SXF_PMFILE",&pmfile);
	if (pmfile && *pmfile)
	{
		LOfroms(PATH & FILENAME, pmfile, &pmloc);
		if(PMload(&pmloc,NULL)!=OK)
		{
			/*
			**	Warning then continue
			*/
			TRdisplay("II_SXF_PMFILE '%s' incurred error\n",
				pmfile);
		}
	}
# endif

	stat = PMget(SX_C2_MODE,&pmvalue);
	if (stat==OK)
	{
		if (!STcasecmp(pmvalue, "on" )) 
			c2_secure_server = TRUE;
	}
    }

    for (;;)
    {
	/*	Now initialize the Server Control Block. */

	svcb = &local_svcb;
	dmf_svcb = &local_svcb;
	svcb->svcb_q_next = NULL;
	svcb->svcb_q_prev = NULL;
	svcb->svcb_length = sizeof(*svcb);
	svcb->svcb_type = SVCB_CB;
	svcb->svcb_s_reserved = 0;
	svcb->svcb_l_reserved = NULL;
	svcb->svcb_owner = (PTR)svcb;
	svcb->svcb_ascii_id = SVCB_ASCII_ID;

	svcb->svcb_id = 0;
	svcb->svcb_d_next = (DMP_DCB*) &svcb->svcb_d_next;
	svcb->svcb_d_prev = (DMP_DCB*) &svcb->svcb_d_next;
	svcb->svcb_s_next = (DML_SCB*) &svcb->svcb_s_next;
	svcb->svcb_s_prev = (DML_SCB*) &svcb->svcb_s_next;
	svcb->svcb_hcb_ptr = NULL;
	svcb->svcb_lctx_ptr = NULL;
	svcb->svcb_lock_list = 0;
	/* Set SINGLEUSER now to defeat all those dm0s_m? semaphores */
	svcb->svcb_status = SVCB_SINGLEUSER;
	if (c2_secure_server)
	    svcb->svcb_status |= SVCB_C2SECURE;
	svcb->svcb_mwrite_blocks = DM_MWRITE_PAGES;
	svcb->svcb_s_max_session = c_session_max;
	svcb->svcb_d_max_database = c_database_max;
	svcb->svcb_m_max_memory = c_memory_max * 1024;
	svcb->svcb_x_max_transactions = 1;
	for (segment_ix = 0; segment_ix < DM_MAX_BUF_SEGMENT; segment_ix++)
	    svcb->svcb_bmseg[segment_ix] = 0;
	svcb->svcb_cnt.cur_session = 0;
	svcb->svcb_cnt.peak_session = 0;
	svcb->svcb_cnt.cur_database = 0;
	svcb->svcb_cnt.peak_database = 0;
	svcb->svcb_int_sort_size = 0;	/* perform sorts externally */

	/*  Initialize the memory manager. */
	dm0m_init();

	svcb->svcb_check_dead_interval = 5;
	svcb->svcb_gc_interval = 20;
	svcb->svcb_gc_numticks = 5;
	svcb->svcb_gc_threshold = 1;
	svcb->svcb_cp_timer = 0;
	svcb->svcb_page_size = DM_PG_SIZE;
	svcb->svcb_etab_tmpsz = DM_PG_SIZE;
	svcb->svcb_etab_pgsize = 0;  /* If not set, use base table page size */
	svcb->svcb_tfname_id = -1;	/* Non-server DMF */
	svcb->svcb_tfname_gen = 0;
	svcb->svcb_seq = (DML_SEQ*)NULL;
	svcb->svcb_lk_level = DMC_C_SYSTEM;
	/*
	**  Set DMF trace flags for this utility.
	*/
	{
	    char		*trace = EOS;

	    NMgtAt("II_DMF_TRACE", &trace);
	    if ((trace != NULL) && (*trace != EOS))
	    {
		char    *comma;
		i4 trace_point;
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
	** Read the direct-io hints in the configuration..
	*/
	status = dmf_setup_directio();
	if (status != E_DB_OK)
	{
	    SETDBERR(dberr, 0, E_DM9411_DMF_INIT);
	    return (status);
	}

	/*
	** Initialize the DMF Core Catalog Templates.
	** These describe the layout of the catalogs iirelation, iiattribute,
	** iiindex, and iirel_idx.  They are used by DMF to build the core
	** catalog TCB's at opendb time and by createdb to populate new
	** databases.
	*/
	status = dmm_init_catalog_templates();
	if (status != E_DB_OK)
	{
	    uleFormat(NULL, E_DM91A7_BAD_CATALOG_TEMPLATES, (CL_ERR_DESC *)NULL, 
		ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM9411_DMF_INIT);
	    return (status);
	}

	/*
	** Transaction state initially inactive 
	*/
	dmf_jsp_xcb.xcb_state=XCB_FREE;
	dmf_jsp_xcb.xcb_scb_ptr = &dmf_jsp_scb;
	dmf_jsp_xcb.xcb_x_type=0;

        /* 
        ** Setup checksum status - this determines whether or not the buffer
	** manager will check and write checksums to all database pages.
        */
	dmc_setup_checksum();

	stat = TMtz_init(&svcb->svcb_tzcb);
	if( stat != OK)
	{
	    SETDBERR(dberr, 0, stat);
	    break;
	}

	/* Initialize ADF. */

	adf_size = adg_srv_size();

	/*  Allocate the ADF server control block. */

	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_length = sizeof(SCF_CB);
	scf_cb.scf_session = DB_NOSESSION;
	scf_cb.scf_facility = DB_DMF_ID;
	scf_cb.scf_scm.scm_functions = 0;
	scf_cb.scf_scm.scm_in_pages = ((adf_size + SCU_MPAGESIZE - 1)
	    & ~(SCU_MPAGESIZE - 1)) / SCU_MPAGESIZE;

	status = scf_call(SCU_MALLOC, &scf_cb);

	if (status != OK)
	{
	    *err_code = status;
	    break;
	}

	dmf_adf_svcb = (PTR) scf_cb.scf_scm.scm_addr;

	status = adg_startup(dmf_adf_svcb, adf_size, 0,
	    (i4)c2_secure_server);
	if (status)
	{
	    SETDBERR(dberr, 0, E_DMF012_ADT_FAIL);
	    break;
	}	
	{
	    PTR			new_area;

            /* ****** FIXME this is illegal!  Need an scf_call! */
            status = sca_add_datatypes(NULL,
					dmf_adf_svcb,
					scf_cb.scf_scm.scm_in_pages,
					(i4)DB_DMF_ID,
					    /* Deallocate old on completion */
					dberr,
					&new_area,
                                        NULL   /* Don't care how big */);
	    if (status)
		break;

	    dmf_adf_svcb = new_area;
	}
	
	dm0s_minit(&svcb->svcb_dq_mutex, "SVCB DCB mutex");
	dm0s_minit(&svcb->svcb_sq_mutex, "SVCB SCB mutex");

	/*  Create a lock list for server objects. */

	status = LKinitialize(&sys_err, lgk_info);
	if (status != OK)
	{
	    (VOID) uleFormat(NULL, E_DM901E_BAD_LOCK_INIT, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM9411_DMF_INIT);
	    return(E_DB_ERROR);
	}
	status = LKcreate_list(
	    csp_lock_list |
		(LK_ASSIGN | LK_NONPROTECT | LK_MASTER | 
		 LK_NOINTERRUPT | LK_MULTITHREAD),
            (i4)0, (LK_UNIQUE *)0, (LK_LLID *)&svcb->svcb_lock_list, 0,
	    &sys_err);
	if (status != OK)
	{
	    (VOID) uleFormat(NULL, E_DM901A_BAD_LOCK_CREATE, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);

	    uleFormat(NULL, E_DM00D0_LOCK_MANAGER_ERROR, &sys_err, ULE_LOG, NULL, 
		(char * )0, 0L, (i4 *)NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM9411_DMF_INIT);
	    return(E_DB_ERROR);
	}

	/* A JSP program should never be the first DMF up, don't ask
	** to create the UDT lock.
	*/
	status = dmf_udt_lkinit(svcb->svcb_lock_list,
				FALSE,
				 dberr);
	if (status != OK)
	    break;

	/*  Allocate the LCTX and a buffer manager. */

	status = dm0l_allocate(dm0l_flag, size, lg_id, 0, 0, (DMP_LCTX **)0,
				dberr);
	if (status != OK)
	    break;

	for (cache_ix = 0; cache_ix < DM_MAX_CACHE; cache_ix++)
	{
	    group[cache_ix] = 8;
	    gpages[cache_ix] = 8;
	    cache_flags[cache_ix] = 0;
	    writebehind[cache_ix] = 0;	/* No WriteBehind threads */
	    if (! connect_only && buffers[cache_ix] == 0)
	    {
		/*
		** Allocate default number of buffers etc.
		** If this is the 2k pool, always allocate a page array too.
		** If this is not the 2k pool, defer the allocation of 
		** the page array.
		*/
		buffers[cache_ix] = 256;
		flimit[cache_ix] = 3;
		mlimit[cache_ix] = 200;
		wbstart[cache_ix] = 200;
		wbend[cache_ix] = 200;
		if (cache_ix != 0)
		    cache_flags[cache_ix] = DM0P_DEFER_PGALLOC;
	    }
	    else
	    {

		flimit[cache_ix] = 3 + buffers[cache_ix] / 32;
		mlimit[cache_ix] = buffers[cache_ix] * 3 /4;
		wbstart[cache_ix] = mlimit[cache_ix] - 
					(buffers[cache_ix] * 15 /100);
		wbend[cache_ix] = buffers[cache_ix] /2;
		if (wbend[cache_ix] > wbstart[cache_ix])
		    wbend[cache_ix] = wbstart[cache_ix] / 2;

	    }
	}

	/* Only set master if doing our own private cache */
	status = dm0p_allocate(connect_only ? DM0P_SHARED_BUFMGR : DM0P_MASTER,
	    connect_only,
	    buffers, flimit, mlimit, group, gpages,
	    writebehind, wbstart, wbend, cache_flags,
	    (i4)0, (i4)0, cache_name, dberr);

	if (status != E_DB_OK)
	{
	    /* Let caller deal with msg if bad cache name given */
	    if (dberr->err_code == E_DM101A_JSP_CANT_CONNECT_BM)
		return (E_DB_ERROR);
	    break;
	}

	/*  Allocate Hash Control Block. */

	status = dm0m_allocate(sizeof(DMP_HCB) + 
	    HCB_HASH_ARRAY_SIZE * sizeof(DMP_HASH_ENTRY) +
	    HCB_MUTEX_ARRAY_SIZE * sizeof(DM_MUTEX),
            DM0M_ZERO, (i4)HCB_CB,
	    (i4)HCB_ASCII_ID, (char *)svcb, 
            (DM_OBJECT **)&svcb->svcb_hcb_ptr, dberr);
	if (status != E_DB_OK)
	{
	    (VOID)dm0p_deallocate(&local_dberr);
	    break;
	}

	/*  Initialize the HCB. */

	hcb = svcb->svcb_hcb_ptr;
	dm0s_minit(&hcb->hcb_tq_mutex, "HCB tq");
	hcb->hcb_ftcb_list.q_next = &hcb->hcb_ftcb_list;
	hcb->hcb_ftcb_list.q_prev = &hcb->hcb_ftcb_list;
	hcb->hcb_tcbcount = hcb->hcb_tcbreclaim = 0;
	for (i = 0; i < DM_MAX_CACHE; i++)
	    hcb->hcb_cache_tcbcount[i] = 0;
	/* This limit is rather arbitrary... */
	hcb->hcb_tcblimit = HCB_HASH_ARRAY_SIZE; 
	for (i = 0; i < HCB_HASH_ARRAY_SIZE; i++)
	{
	    hcb->hcb_hash_array[i].hash_q_next = (DMP_TCB*)
		&hcb->hcb_hash_array[i].hash_q_next;
	    hcb->hcb_hash_array[i].hash_q_prev = (DMP_TCB*)
		&hcb->hcb_hash_array[i].hash_q_next;
	}
	hcb->hcb_mutex_array = (DM_MUTEX *) &hcb->hcb_hash_array[HCB_HASH_ARRAY_SIZE];
	for (i = 0; i < HCB_MUTEX_ARRAY_SIZE; i++)
	{
	    dm0s_minit(&hcb->hcb_mutex_array[i],
		STprintf(sem_name, "HCB hash %d", i));
	}
	svcb->svcb_status |= SVCB_ACTIVE;

	/*
	**	Need to initialize SXF in case any user auditing occurs
	**      on a C2 secure server. Note this must be after
	**	DMF/ADF initialization. The CSP does NOT have SXF.
	*/

	sxf_rcb.sxf_ascii_id=SXFRCB_ASCII_ID;
	sxf_rcb.sxf_length = sizeof (sxf_rcb);
	sxf_rcb.sxf_cb_type = SXFRCB_CB;
	sxf_rcb.sxf_status = 0;

	if ( c2_secure_server==TRUE )
	    sxf_rcb.sxf_status|=SXF_C2_SERVER;

	sxf_rcb.sxf_mode = SXF_SINGLE_USER;
	sxf_rcb.sxf_pool_sz = SXF_BYTES_PER_SINGLE_SERVER;
	sxf_rcb.sxf_max_active_ses = 1;
	sxf_rcb.sxf_dmf_cptr = dmf_sxf_djc;

	status = sxf_call(SXC_STARTUP, &sxf_rcb);
	if (status != E_DB_OK)
	{
	    uleFormat(&sxf_rcb.sxf_error, 0, NULL, ULE_LOG, NULL, 
	    		NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM1030_JSP_SXF_INIT_ERR);
	    break;
	}
	dmf_sxf_svcb = sxf_rcb.sxf_server;
	status = CXjoin(0);
	if (status != E_DB_OK)
	{
	    uleFormat(NULL, status, NULL, ULE_LOG, NULL,
	    		NULL, 0, NULL, err_code, 0);
	    break;
	}
	return (E_DB_OK);
    }
    uleFormat(dberr, 0, NULL, ULE_LOG, NULL,
		NULL, 0, NULL, err_code, 0);
#ifndef NT_GENERIC
    if (status == E_CS0037_MAP_SSEG_FAIL) 
	SETDBERR(dberr, 0, E_DM00AB_CANT_MAP_SHARED_MEM);
    else
#endif
	SETDBERR(dberr, 0, E_DM007F_ERROR_STARTING_SERVER);

    return(E_DB_ERROR);
}

/*{
** Name: dmf_getadf	- Return Adf Server control block
**
** Description:
**      This routine returns the address of the ADF server control block for use
**	by dmfatp().  This routine needs the server control block to be able to
**	initialize a bona fide adf session.  Previously, this was done by
**	redoing the adg_startup() in dmfatp(), but this seemed a rather crude
**	way to manage things.  Furthermore, with udadt's, this action voided all
**	the user defined adt's. 
**
** Inputs:
**      None
**
** Outputs:
**      None.
**
**	Returns:
**	    PTR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-apr-89 (fred)
**          Created.
*/
PTR
dmf_getadf(VOID)
{
    return(dmf_adf_svcb);
}

/*{
** Name: dmf_sxf_bgn_session - Starts a DMF SXF session.
**
** Description:
**	This starts a single-user SXF session, so any audited events
**	may be written.
**
** Inputs:
**      User name, real user name, database name, user status
**
** Outputs:
**      Status
**
** Side Effects:
**	    Starts new SXF session.
**
** History:
**      02-nov-92 (robf)
**          Created.
**      11-nov-92 (jnash)
**          Fix problem with linking CSP by removing dmf_write_msg() 
**	    calls, substituting ule_format.  Additional error reporting
**	    may be desirable.
*/
DB_STATUS
dmf_sxf_bgn_session(
	DB_OWN_NAME *ruser,
	DB_OWN_NAME *user,
	char *dbname,
	i4 ustat)
{
	SXF_RCB 	sxf_rcb;
	DB_STATUS 	status = E_DB_OK;
	DB_DB_NAME 	dbdbname;
	STATUS		s;
	CL_ERR_DESC  	sys_err;
	i4 	err_code;
	/*
	**	If SXF hasn't been initialized, then no-op
	*/
	if (dmf_sxf_svcb==NULL)
		return E_DB_OK;
	/*
	**	Initialize a single-user SXF session
	*/
	sxf_rcb.sxf_ascii_id=SXFRCB_ASCII_ID;
	sxf_rcb.sxf_length = sizeof (sxf_rcb);
	sxf_rcb.sxf_cb_type = SXFRCB_CB;
	sxf_rcb.sxf_user=user;
	sxf_rcb.sxf_ruser=ruser;
	MEcopy(dbname,
		sizeof(dbdbname.db_db_name), 
		dbdbname.db_db_name);
	sxf_rcb.sxf_db_name= &dbdbname;
	sxf_rcb.sxf_ustat = ustat;
	sxf_rcb.sxf_server= dmf_sxf_svcb;

	status = sxf_call(SXC_BGN_SESSION, &sxf_rcb);

	if (status != E_DB_OK)
	{
	    (VOID) uleFormat(&sxf_rcb.sxf_error, 0, NULL, ULE_LOG, 
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    (VOID) uleFormat(NULL, E_DM1031_JSP_SXF_SES_INIT, NULL, ULE_LOG, 
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	}
	return status;
}

/*{
** Name: dmf_sxf_end_session - Ends a DMF SXF session.
**
** Description:
**	This ends a single-user SXF session, including flushing 
**      any records first.
**
** Inputs:
**      none
**
** Outputs:
**      Status
**
** Side Effects:
**	    Terminates current SXF session.
**
** History:
**      02-nov-92 (robf)
**          Created.
**      11-nov-92 (jnash)
**          Fix problem with linking CSP by removing dmf_write_msg() 
**	    calls, substituting ule_format.  Additional error reporting
**	    may be desirable.
*/
DB_STATUS
dmf_sxf_end_session()
{
	SXF_RCB sxf_rcb;
	DB_STATUS status = E_DB_OK;
	i4 err_code;

	/*
	**	If SXF hasn't been initialized, then no-op
	*/
	if (dmf_sxf_svcb==NULL)
		return E_DB_OK;
	/*
	**	Terminate a single-user SXF session
	*/
	sxf_rcb.sxf_ascii_id=SXFRCB_ASCII_ID;
	sxf_rcb.sxf_length = sizeof (sxf_rcb);
	sxf_rcb.sxf_cb_type = SXFRCB_CB;
	sxf_rcb.sxf_server= dmf_sxf_svcb;
	status = sxf_call(SXR_FLUSH, &sxf_rcb);
	if (status != E_DB_OK)
	{
	    uleFormat(&sxf_rcb.sxf_error, 0, NULL, ULE_LOG, NULL, 
	    		NULL, 0, NULL, &err_code, 0);
	    (VOID) uleFormat(NULL, E_DM1033_JSP_SXF_SES_FLUSH, NULL, ULE_LOG, 
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    /* Continue on to terminate the session */
	}

	status = sxf_call(SXC_END_SESSION, &sxf_rcb);
	if (status != E_DB_OK)
	{
	    uleFormat(&sxf_rcb.sxf_error, 0, NULL, ULE_LOG, NULL,
	    		NULL, 0, NULL, &err_code, 0);
	    (VOID) uleFormat(NULL, E_DM1032_JSP_SXF_SES_TERM, NULL, ULE_LOG, 
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	}
	return status;
}

/*{
** Name: dmf_sxf_terminate - Closes down a single-user SXF facility.
**
** Description:
**	This shuts down a single-user version of the SXF facility.
**      any records first.
**
** Inputs:
**      none
**
** Outputs:
**      Status, error code.
**
** Side Effects:
**	    Terminates current SXF session.
**
** History:
**      02-nov-92 (robf)
**          Created.
*/
DB_STATUS
dmf_sxf_terminate(DB_ERROR *dberr)
{
	SXF_RCB sxf_rcb;
	i4	local_err;
	DB_STATUS status = E_DB_OK;

	CLRDBERR(dberr);

	/*
	**	If SXF hasn't been initialized, then no-op
	*/
	if (dmf_sxf_svcb==NULL)
		return E_DB_OK;
	/*
	**	Terminate the SXF facility
	*/
	sxf_rcb.sxf_ascii_id=SXFRCB_ASCII_ID;
	sxf_rcb.sxf_length = sizeof (sxf_rcb);
	sxf_rcb.sxf_cb_type = SXFRCB_CB;
	sxf_rcb.sxf_server= dmf_sxf_svcb;

	status = sxf_call(SXC_SHUTDOWN, &sxf_rcb);

	if (status != E_DB_OK)
	{
	    uleFormat(&sxf_rcb.sxf_error, 0, NULL, ULE_LOG, NULL,
	    		NULL, 0, NULL, &local_err, 0);

	    SETDBERR(dberr, 0, E_DM0083_ERROR_STOPPING_SERVER);
	}
	dmf_sxf_svcb=NULL;
	return status;
}

/*
** Name: dmf_udt_lkinit() - Routine to request UDT Control Lock.
**
** Description:
**	If this application is the RCP or CSP, and the lock for UDADT level
**	does not exist, then create it, holding it in NULL mode.  Everyone
**	else, request the lock to check the datatype ID level, but then the
**	lock can be released.
**
** Inputs:
**	lock_list		- lock_list to use.
**	create_flag		- if non zero, lock should be created.
**
** Outputs:
**	err_code		- Detailed error message if error occurs.
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	12-oct-1992 (bryanp)
**	    Created.
**	25-apr-1994 (bryanp) integrated 27-sep-1993 (rachael)
**	    Bug 53516. The lock for UDADT level has been marked invalid by
**	    the VMS Distributed Lock Manager. (See the submission for bug 47950
**	    for a description of this problem.)  While the CSP and RCP do
**	    not check for lk_val_notvalid, the dmfjsp utilities do. However,
**	    the appropriate value can be obtained from SCF and compared to
**	    the value in the lock value block; thus allowing the utilities to
**	    continue executing if the value has been marked invalid improperly.
**	28-Feb-2002 (jenjo02)
**	    Removed LK_MULTITHREAD from LKrequest/release flags. 
**	    MULTITHREAD is now an attribute of the lock list.
**	07-feb-2005 (devjo01)
**	    Change stategy for obtaining lock.  If 'create_flag' set,
**	    1st try LK_X NOWAIT, if gotten then we set value then
**	    convert down to LK_S.  Otherwise value is just read from
**	    a share lock.  As before if not an RCP lock is released.
**	    This is done to prevent the LVB for this lock from becoming
**	    invalidated as would be the case in the previous algorithm
**	    where we converted down tp LK_N after reading the lock.  This
**	    would cause a schedule deadlock for a starting CSP if the 
**	    lock was invalidated causing a stall.
**	01-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameter to LKrequest avoid random implicit
**	    lock conversions.
*/
STATUS
dmf_udt_lkinit(
    i4	lock_list,
    i4		create_flag,
    DB_ERROR	*dberr)
{
    LK_LOCK_KEY		lock_key;
    LK_VALUE		lock_value;
    LK_LKID		lockid;
    i4		major_id;
    i4		minor_id;
    i4		mode;
    STATUS		status, lstatus;
    CL_ERR_DESC		sys_err;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);
    
    lock_key.lk_type = LK_CONTROL;
    MEmove(STlength(ADD_LOCK_KEY),
	    ADD_LOCK_KEY,
	    ' ',
	    6 * sizeof(lock_key.lk_key1),
	    (PTR) &lock_key.lk_key1);
    MEfill(sizeof(LK_LKID), 0, &lockid);

    if ( create_flag )
    {
	mode = LK_X;
	status = LKrequest((LK_PHYSICAL | LK_NODEADLOCK | LK_NOWAIT),
	    (LK_LLID) lock_list, &lock_key, LK_X, &lock_value,
	    &lockid, (i4)0, &sys_err);
    }

    if (!create_flag || (status == LK_BUSY) || (status == LK_UBUSY))
    {
	/* Try in share mode. */
	mode = LK_S;
	status = LKrequest((LK_PHYSICAL | LK_NODEADLOCK),
	    (LK_LLID) lock_list, &lock_key, LK_S, &lock_value,
	    &lockid, (i4)0, &sys_err);
    }

    if ((status != OK) && (status != LK_VAL_NOTVALID))
    {
	uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 2,
	    0, mode, 0, lock_list);
	SETDBERR(dberr, 0, E_DMF012_ADT_FAIL);
	return (E_DB_ERROR);
    }
    else
    {
	sca_obtain_id(&major_id, &minor_id);
	
	if (create_flag && LK_X == mode)
	{
	    /* If we are the RCP or CSP, then it is OK not to find  */
	    /* a value for this lock.  We will initialize it.	    */

	    if (lock_value.lk_high == 0)
	    {
		lock_value.lk_high = major_id;
		lock_value.lk_low = minor_id;
	    }

	    status = LKrequest(LK_PHYSICAL | LK_CONVERT |
				    LK_NODEADLOCK,
			(LK_LLID) lock_list,
			(LK_LOCK_KEY *)NULL, LK_S,
			&lock_value, &lockid, (i4)0, &sys_err);

	    if (status != OK)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 2,
		    0, LK_S, 0, lock_list);
		SETDBERR(dberr, 0, E_DMF012_ADT_FAIL);
		status = E_DB_ERROR;
	    }
	}
	else
	{
	    status = E_DB_OK;

	    major_id = (i4) lock_value.lk_high;
	    minor_id = (i4) lock_value.lk_low;

	    if (!sca_check(major_id, minor_id))
	    {
		SETDBERR(dberr, 0, E_DMF012_ADT_FAIL);
		status = E_DB_ERROR;
	    }
	}
	
	if (E_DB_OK != status || !create_flag)
	{
	    lstatus = LKrelease(0, (LK_LLID) lock_list,
		     &lockid, (LK_LOCK_KEY *)NULL, &lock_value, &sys_err);

	    if (lstatus != OK)
	    {
		uleFormat(NULL, lstatus, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
		    0, lock_list);
		if ( E_DB_OK == status )
		    SETDBERR(dberr, 0, E_DM007F_ERROR_STARTING_SERVER);
		status = E_DB_ERROR;
	    }
	}
    }
    return (status);
}

/*
** Name: dmf_setup_directio() - Setup direct I/O hints.
**
** Description:
**	This routine loads direct I/O hint parameters:
**	    ii.$.config.direct_io
**	    ii.$.config.direct_io_load
**	both having ON/OFF values.  There's a direct_io_log config option
**	as well, but LGK will read that one.  (LG is standoff-ish and
**	doesn't use the DMF svcb.)
**
**	These are "config" settings, not dbms or recovery settings, because
**	direct I/O needs to apply to ALL dmf's, even the archiver,
**	fastload, etc.  If some dmf's use direct I/O and some don't,
**	it can cause the OS extra work, and can expose a linux kernel
**	bug in a couple versions.  (2.6.19 and .20, I believe.)
**
**	We'll also inquire about the OS-required direct I/O alignment, and
**	store that value in case anything is using direct I/O.
**
** Inputs:
**
** Outputs:
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	18-apr-1994 (jnash)
**	    Created for fsync project.
**	5-Nov-2009 (kschendel) SIR 122757
**	    Rename, read direct-io (tables) and direct-io-load params.
*/
STATUS
dmf_setup_directio(void)
{
    char		*pm_str;
    STATUS		status;
    i4		err_code;

    /* Defaults for direct I/O are OFF */
    dmf_svcb->svcb_directio_tables = FALSE;
    dmf_svcb->svcb_directio_load = FALSE;

    status = PMget("ii.$.config.direct_io", &pm_str);
    if (status == E_DB_OK)
    {
	if (STcasecmp(pm_str, "ON" ) == 0)
	    dmf_svcb->svcb_directio_tables = TRUE;
    }
    else if (status != PM_NOT_FOUND)
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	return(E_DB_ERROR);
    }

    status = PMget("ii.$.config.direct_io_load", &pm_str);
    if (status == E_DB_OK)
    {
	if (STcasecmp(pm_str, "ON" ) == 0)
	    dmf_svcb->svcb_directio_load = TRUE;
    }
    else if (status != PM_NOT_FOUND)
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	return(E_DB_ERROR);
    }
    /* Get alignment if either direct I/O hint is set. */
    dmf_svcb->svcb_directio_align = 0;
    if (dmf_svcb->svcb_directio_tables || dmf_svcb->svcb_directio_load)
	dmf_svcb->svcb_directio_align = DIget_direct_align();

    return(E_DB_OK);
}

/*
** Name: dmf_sxf_djc - Sxf/Dmf mapping function
**
** Description:
**	This function is a replacement for dmf_call() for use by
**	SXF when called in the JSP context. It accepts a limited set
**	of dmf_call-type requests and maps them to the lower level
**	context understood by the JSP. 
**
**	The reason for this function is that SXF depends on the dmf_call
**	interface to make DMF requests. This happens not to be true in the 
**	JSP, which operates at a lower level. Typical problems include no 
**	Session context so DMF code AVs in a bunch of places when called 
**	via dmf_call, which assumes this context is available, and different
**	handling of XCBs and DCBs.
**	
**	Note: Long term this should be readdressed so a more consistent
**	interface is used within JSP
**
** Inputs/Outputs:
**	dmf_call() requests:
**	DMT_OPEN, DMT_CLOSE
**	DMR_GET, DMR_POSITION
**	DMX_BEGIN, DMX_ABORT, DMX_COMMIT
**	DMC_SHOW
**
** History:
**	1-oct-93 (robf)
**	   Created
**	1-mar-94 (robf)
**         Add error trace message.
*/

static DB_STATUS
dmf_sxf_djc (operation, control_block)
DM_OPERATION        operation;
PTR		    control_block;
{
    DMC_CB	    *cb = (DMC_CB *)control_block;
    DB_STATUS	    status=E_DB_OK;
    EX_CONTEXT	    context;
    DMP_DCB	    *dcb;
    DMC_CB	    *dmc;
    DMT_CB	    *dmt;
    DMP_RCB	    *rcb;
    DB_TAB_TIMESTAMP ctime;
    DB_TAB_TIMESTAMP    stamp;
    i4	     access;
    static DB_OWN_NAME      username;
    static bool		got_username=FALSE;
    i4		    local_error;
    i4		    *err_code = &cb->error.err_code;

    CLRDBERR(&cb->error);

    if(got_username==FALSE)
    {
	char	    *cp = (char *)&username;

	IDname(&cp);
	MEmove(STlength(cp), cp, ' ', sizeof(username), 
			username.db_own_name);
	got_username=TRUE;
    }
    /*
    ** Use the current dcb context
    */
    dcb=dmf_jsp_dcb;
    /*
    ** Process options
    */
    if (EXdeclare(ex_handler, &context) == OK &&
	(dmf_svcb == 0 || (dmf_svcb->svcb_status & SVCB_CHECK) == 0))
    {
	switch (operation)
	{
	/* RECORD operations */
	case DMR_GET:
	    status = dmr_get((DMR_CB *)cb);
	    break;


	case DMR_POSITION:
	    status = dmr_position((DMR_CB *)cb);
	    break;

	/* TABLE operations. */

	case DMT_CLOSE:
	    dmt=(DMT_CB*)cb;
	    rcb=(DMP_RCB*)dmt->dmt_record_access_id;
	    status=dm2t_close(rcb, 0, &cb->error);
	    break;

	case DMT_OPEN:
	    dmt=(DMT_CB*)cb;
 	    switch (dmt->dmt_access_mode)
	    {
            case    DMT_A_READ:
            case    DMT_A_RONLY:
                access= DM2T_A_READ;
                break;
            case    DMT_A_WRITE:
                access= DM2T_A_WRITE;
                break;
            case    DMT_A_OPEN_NOACCESS:
                access= DM2T_A_OPEN_NOACCESS;
                break;
            default:
		SETDBERR(&cb->error, 0, E_DM002A_BAD_PARAMETER);
		status=E_DB_ERROR;
	    }
	    if(status!=E_DB_OK)
		break;

	    status = dm2t_open( dcb, 
			&dmt->dmt_id,
			DM2T_IS,
			dmt->dmt_update_mode,
			access, 
			0,	/* timeout */
			20,     /* locks */
			0,      /* spid */
			0,
			dmf_svcb->svcb_lock_list, 
			0, 0, 0,
			&dmf_jsp_xcb.xcb_tran_id,
			&stamp,
			&rcb,
			(DML_SCB *)0, 
			&cb->error);

	    if(status==E_DB_OK)
	    {
		/*
		** Minimum support required for SXF 
		*/
		rcb->rcb_internal_req=1;
		dmt->dmt_record_access_id = (char*)rcb;
		rcb->rcb_xcb_ptr = &dmf_jsp_xcb;
	    }
	    break;

	/*	TRANSACTION operations. */

	case DMX_BEGIN:
	    status = dmf_jsp_begin_xact(DMXE_READ, 0, dcb, dcb->dcb_log_id,
			&username, dmf_svcb->svcb_lock_list,
			&dmf_jsp_xcb.xcb_log_id,
			&dmf_jsp_xcb.xcb_tran_id,
			&dmf_jsp_xcb.xcb_lk_id,
			&cb->error);
	    break;

	case DMX_COMMIT:
	    status = dmf_jsp_commit_xact (&dmf_jsp_xcb.xcb_tran_id, 
				dmf_jsp_xcb.xcb_log_id, 
				dmf_jsp_xcb.xcb_lk_id,
				DMXE_ROTRAN, 
				&ctime,
				&cb->error);
	    break;

	case DMX_ABORT:
	    status = dmf_jsp_abort_xact((DML_ODCB*)0, dcb, 
				&dmf_jsp_xcb.xcb_tran_id,
				dmf_jsp_xcb.xcb_log_id, 
				dmf_jsp_xcb.xcb_lk_id, DMXE_ROTRAN,
				(i4)0,
				(LG_LSN*)0, 
				&cb->error);
	    break;

	/* CONTROL operations */
	case DMC_SHOW:
	    /*
	    ** Support for tran id only
	    */
	    dmc=(DMC_CB*)cb;
	    if(dmc->dmc_op_type==DMC_SESSION_OP &&
	       (dmc->dmc_flags_mask==DMC_XACT_ID))
	    {
                if(dmf_jsp_xcb.xcb_x_type!=0)
			dmc->dmc_xact_id=(PTR)&dmf_jsp_xcb;
		else
			dmc->dmc_xact_id=(PTR)0;
		status=E_DB_OK;
	    }
	    else
	    {
		 SETDBERR(&cb->error, 0, E_DM002A_BAD_PARAMETER);
		 status = E_DB_ERROR;
	    }
	    break;

	default:
	    SETDBERR(&cb->error, 0, E_DM002A_BAD_PARAMETER);
	    status = E_DB_ERROR;
	    break;
	}
	EXdelete();
	if(status!=E_DB_OK)
	{
		uleFormat(NULL, E_DM1055_JSP_SXF_DMF_ERR, NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 
		    2, 
		    sizeof(operation), &operation,
		    sizeof(cb->error.err_code), &(cb->error.err_code));
	}

        return (status);
    }

    /*
    **	If exception handler declares or DMF is inconsistent already
    **	just return a severe error.
    */

    EXdelete();
    SETDBERR(&cb->error, 0, E_DM004A_INTERNAL_ERROR);
    return (E_DB_FATAL);
}

/*
** Name: dmf_jsp_abort/begin_xact
**
** Description:
**	These are wrappers for the dmxe_abort/commit/begin functions
**	accessed within JSP. They maintain additional context so
**	that dmf_sxf_djc() can operate appropriately
**
** History
**	1-oct-93 (robf)
**	   Created
*/
DB_STATUS 
dmf_jsp_abort_xact(
    DML_ODCB		*odcb,
    DMP_DCB		*dcb,
    DB_TRAN_ID		*tran_id,
    i4		log_id,
    i4		lock_id,
    i4		flag,
    i4		sp_id,
    LG_LSN		*savepoint_lsn,
    DB_ERROR		*dberr)
{
    DB_STATUS status;

    CLRDBERR(dberr);

    /* Call DMXE to perform abort */
    status=dmxe_abort(odcb,
		dcb,
		tran_id,
		log_id,
		lock_id,
		flag,
		sp_id,
		savepoint_lsn,
		dberr);

    /* Update local transaction info */
    dmf_jsp_xcb.xcb_state=XCB_FREE;
    dmf_jsp_xcb.xcb_x_type= 0;

    return status;
}
DB_STATUS 
dmf_jsp_begin_xact(
    i4		mode,
    i4		flags,
    DMP_DCB		*dcb,
    i4		dcb_id,
    DB_OWN_NAME		*user_name,
    i4		related_lock,
    i4		*log_id,
    DB_TRAN_ID		*tran_id,
    i4		*lock_id,
    DB_ERROR		*dberr)
{
	DB_STATUS status;

	CLRDBERR(dberr);

	status=dmxe_begin(
		mode,
		flags,
		dcb,
		dcb_id,
		user_name,
		related_lock,
		log_id,
		tran_id,
		lock_id,
		(DB_DIS_TRAN_ID *)NULL,
		dberr);

	if(status==E_DB_OK)
	{
		/* Update transaction info */
		dmf_jsp_xcb.xcb_tran_id= *tran_id;
		dmf_jsp_xcb.xcb_log_id= *log_id;
		dmf_jsp_xcb.xcb_lk_id = *lock_id;
		dmf_jsp_xcb.xcb_x_type=XCB_RONLY;
	}
	return status;
}

/*
** Name: dmf_jsp_commit_xact
**
** Description:
**      These are wrappers for the dmxe_abort/commit/begin functions
**      accessed within JSP. They maintain additional context so
**      that dmf_sxf_djc() can operate appropriately
**
** History
**      1-oct-93 (robf)
**         Created
**      20-sep-2004 (huazh01)
**         changed prototype for dmxe_commit(). 
**         INGSRV2643, b111502.
**	28-Jul-2005 (schka24)
**	    Back out x-integration for bug 111502, doesn't
**	    apply to r3.
*/
DB_STATUS 
dmf_jsp_commit_xact(
    DB_TRAN_ID		*tran_id,
    i4		log_id,
    i4		lock_id,
    i4		flag,
    DB_TAB_TIMESTAMP    *ctime,
    DB_ERROR		*dberr)
{
	DB_STATUS status;

	CLRDBERR(dberr);

	status=dmxe_commit (
		tran_id,
		log_id,
		lock_id,
		flag,
		ctime,
		dberr);

    	/* Update local transaction info */
    	dmf_jsp_xcb.xcb_state=XCB_FREE;
	dmf_jsp_xcb.xcb_x_type=0;
	return status;
}

/*{
** Name: ex_handler	- DMF exception handler.
**
** Description:
**      This routine will catch all DMF_SXF exceptions.  Any exception caught
**	by DMF is considered SEVERE.  DMf is marked as inconsistent and
**	all future callers will be return an error of E_DM004A_INTERNAL_ERROR.
**
** Inputs:
**      ex_args                         Exception arguments.
**
** Outputs:
**	Returns:
**	    EXDECLARE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      1-oct-93 (robf)
**          Added, cloned from dmf_call handler
*/
static STATUS
ex_handler(
EX_ARGS		    *ex_args)
{
    i4	    err_code;
    i4	    status;

#ifdef VMS
    status = sys$setast(0);
#endif
    
    if (ex_args->exarg_num == EX_DMF_FATAL)
    {
	err_code = E_DM904A_FATAL_EXCEPTION;
    }
    else
    {
	err_code = E_DM9049_UNKNOWN_EXCEPTION;
    }

    /*
    ** Report the exception.
    */
    ulx_exception( ex_args, DB_DMF_ID, err_code, FALSE );

    dmf_svcb->svcb_status |= SVCB_CHECK;
#ifdef VMS
    if (status == SS$_WASSET) 
        sys$setast(1);
#endif
    return (EXDECLARE);
}
