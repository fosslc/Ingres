/*
** Copyright (c) 1998, 2008 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <sl.h>
#include    <cm.h>
#include    <cs.h>
#include    <cv.h>
#include    <tm.h>
#include    <ci.h>
#include    <er.h>
#include    <ex.h>
#include    <lo.h>
#include    <nm.h>
#include    <pc.h>
#include    <pm.h>
#include    <tm.h>
#include    <tmtz.h>
#include    <st.h>
#include    <lk.h>
#include    <cx.h>
#include    <tr.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmtcb.h>
#include    <qsf.h>
#include    <ddb.h>
#include    <opfcb.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <scf.h>
#include    <duf.h>
#include    <dudbms.h>
#include    <gca.h>
#include    <iiapi.h>
#include    <ccm.h>
/* added for scs.h prototypes, ugh! */
#include <dmrcb.h>
#include <copy.h>
#include <qefqeu.h>
#include <qefcopy.h>

#include    <sc.h>
#include    <sca.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <sc0e.h>
#include    <sc0m.h>
#include    <sce.h>
#include    <scfcontrol.h>
#include    <scserver.h>
#include    <scdopt.h>

#include    <urs.h>
#include    <wtsinit.h>
#include    <ascf.h>
#include    <ascs.h>
#include    <ascd.h>
#include    <asct.h>

/**
**
**  Name: SCDINIT.C - Initiation and Termination rtns for the server
**
**  Description:
**      This file contains the code which control the initiation and termination
**	of the Server.
**
**          scd_initiate - start the server and all its associated facilities
**          ascd_terminate - stop all associated facilities and the server
**
**
**  History:
**	12-mar-1998 (wonst02)
**	    Simple rename to URSB.
**	04-May-1998 (shero03)
**	    Remove agwf.  
**      28-May-98 (fanra01)
**          Integrate ICE changes.  Initialise the DDF for ice.
** 	11-Jun-1998 (wonst02)
** 	    Continue even if the URSM returns a warning that no app was started.
** 	23-Jun-1998 (wonst02)
** 	    Removed ref. to DB_GWF_ID, some unneeded variables, & some includes.
**	22-Jul-1998 (fanra01)
**	    Removed iicmapi.h. Not used.
**      09-Nov-98 (fanra01)
**          Add ule_initiate for icesvr.
**      15-Jan-199 (fanra01)
**          Add ccm, urs, ascf and ascd includes.
**          Renamed following functions to make ascd specific
**          scd_initiate
**          scd_terminate
**          Renamed following functions to remove duplication.  Perhaps
**          delete later
**          scd_dbinfo_fcn
**          scd_adf_printf
**          scd_init_sngluser
**      12-Feb-99 (fanra01)
**          Include headers for ascs.  Renamed scs_ctlc to ascs_ctlc.
**      15-Feb-1999 (fanra01)
**          Add license thread and cleanup threads.
**          Also change order of initialision to reduce period between
**          registering with the name server and listening on the port.
**      04-Mar-1999 (fanra01)
**          Add tracing thread.  The thread wakes up periodically and flushes
**          memory buffers to disk.
**      08-Mar-99 (fanra01)
**          Ensure that the GCA_INITIATE is performed before starting ice
**          server initialisation.  This ensures that first gca cb instance
**          defined contains the server class.  Ice server initialisation
**          invokes multiple GCA_INITIATEs through the api and no server class
**          information is saved.
**      12-Mar-1999 (fanra01)
**          Add Trace thread initialisation.
**	22-nov-1999 (somsa01)
**	    In ascd_initiate(), moved ule_initiate() to proper place after
**	    retrieving the ICESVR id. Also, after a successful startup,
**	    also write E_SC051D_LOAD_CONFIG to the errlog.
**	04-Oct-2000 (jenjo02)
**	    Changed
**		gca_cb.gca_p_semaphore = CSp_semaphore_function;
**		gca_cb.gca_v_semaphore = CSv_semaphore_function;
**	    back to 
**		gca_cb.gca_p_semaphore = CSp_semaphore;
**		gca_cb.gca_v_semaphore = CSv_semaphore;
**	30-may-2001 (devjo01)
**	    Replace LGcluster() with CXcluster_enabled. s103715.
**	11-Jun-2004 (hanch04)
**	    Removed reference to CI for the open source release.
**	15-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	18-Oct-2004 (shaha03)
**	    SIR #112918, support for configurable default cursor mode added
**  11-Feb-2005 (fanra01)
**      Sir 112482
**      Exclude initialization of urs facility.  This facility is not required
**      by the icesvr.
**  11-Feb-2005 (fanra01)
**      Sir 112482
**      Exclude termination from urs facility missed in previous change.
**  16-nov-2008 (stial01)
**      Redefined name constants without trailing blanks.
**      16-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	30-May-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN void scd_mo_init(void);

static DB_STATUS start_lglk_special_threads(
				    i4  recovery_server,
				    i4  cp_timeout,
				    i4  num_logwriters,
				    i4  start_gc_thread,
				    i4  start_lkcback_thread);


/*
** Definition of all global variables owned by this file.
*/

#define	NEEDED_SIZE (((sizeof(SC_MAIN_CB) + sizeof(SCV_LOC) + (SCU_MPAGESIZE - 1)) & ~(SCU_MPAGESIZE - 1)) / SCU_MPAGESIZE)

/* number of extras sessions to have CS support so we can have
   sessions available for special users even when regular users
   have chewed up all the -connected_session connections */

# define SCD_RESERVED_CONNS	(2)

GLOBALREF SC_MAIN_CB     *Sc_main_cb;    /* Core structure for SCF */
GLOBALREF i4     	 Sc_server_type; /* for handing to scd_initiate */
GLOBALREF SCS_IOMASTER   IOmaster;   	 /* Anchor for list of databases */

GLOBALREF ASCTFL asctfile;
GLOBALREF ASCTFR asctentry;
GLOBALREF i4     asctlogmsgsize;

					 /* that IOMASTER server handles */
/*}
** Name:	DBMS_INFO_DESC
**
** Description:
**	Describes a DBMS_INFO request we're registering.
**	We have a table of these.  Ordering of entries is unimportant.
**
**	To add a DBMS_INFO constant, add a define to scf.h, an entry
**	in this table below, and dispatching in scuisvc.c.
**
** History:
*/

typedef struct
{
    char	*info_name;
    i4 		info_reqcode;
    i4 		info_inputs;
    i4 		info_dt1;
    i4 		info_fixedsize;
    i4 		info_type;
} DBMS_INFO_DESC;

static DBMS_INFO_DESC dbms_info_items[] =
{
    { "TRANSACTION_STATE",	SC_XACT_INFO,	0,	0, /* 0 */
	  			4,	DB_INT_TYPE },

    { "AUTOCOMMIT_STATE",	SC_AUTO_INFO,	0,	0, /* 1 */
				4,	DB_INT_TYPE },

    { "_BINTIM",	SCI_NOW,	0,	0, /* 2 */
			4,	DB_INT_TYPE },

    { "_CPU_MS",	SCI_CPUTIME,	0,	0, /* 3 */
			sizeof(i4     ),	DB_INT_TYPE },

    { "_ET_SEC",	SC_ET_SECONDS,	0,	0, /* 4 */
			4,	DB_INT_TYPE },

    { "DBA",	SCI_DBA,	0,	0, /* 8 */
				sizeof(DB_OWN_NAME),	DB_CHR_TYPE },

    { "USERNAME",	SCI_USERNAME,	0,	0, /* 9 */
				sizeof(DB_OWN_NAME),	DB_CHR_TYPE },

    { "_VERSION",	SCI_IVERSION,	0,	0, /* 10 */
			DB_TYPE_MAXLEN,	DB_CHR_TYPE },

    { "DATABASE",	SCI_DBNAME,	0,	0, /* 11 */
			sizeof(DB_DB_NAME),	DB_CHR_TYPE },

    { "TERMINAL",	SCI_UTTY,	0,	0, /* 12 */
			sizeof(DB_TERM_NAME),	DB_CHR_TYPE },

    { "QUERY_LANGUAGE",		SC_QLANG,	0,	0, /* 13 */
				sizeof(DB_TERM_NAME),	DB_CHR_TYPE },

    { "DB_COUNT",	SC_DB_COUNT,	0,	0, /* 14 */
			sizeof(i4     ),	DB_INT_TYPE },

    { "SERVER_CLASS",	SC_SERVER_CLASS,	0,	0, /* 15 */
			sizeof(DB_DB_NAME),	DB_CHR_TYPE },

    { "OPEN_COUNT",	SC_OPENDB_COUNT,	0,	DB_CHR_TYPE, /* 15 */
			sizeof(i4     ),	DB_INT_TYPE },

    { "DBMS_CPU",	SCI_DBCPUTIME,	0,	0, /* 16 */
			sizeof(i4     ),	DB_INT_TYPE },

    { "DBMS_DIO",	SCI_DBDIOCNT,	0,	0, /* 17 */
			sizeof(i4     ),	DB_INT_TYPE },

    { "DBMS_BIO",	SCI_DBBIOCNT,	0,	0, /* 18 */
			sizeof(i4     ),	DB_INT_TYPE },

    { "GROUP",	SCI_GROUP,	0,	0, /* 19 */
				sizeof(DB_OWN_NAME),	DB_CHR_TYPE },

    { "ROLE",	SCI_APLID,	0,	0, /* 20 */
				sizeof(DB_OWN_NAME),	DB_CHR_TYPE },

    { "QUERY_IO_LIMIT",		SCI_QDIO_LIMIT,	0,	0, /* 21 */
				sizeof(i4     ),	DB_INT_TYPE },

    { "QUERY_ROW_LIMIT",	SCI_QROW_LIMIT,	0,	0, /* 22 */
				sizeof(i4     ),	DB_INT_TYPE },

    { "QUERY_CPU_LIMIT",	SCI_QCPU_LIMIT,	0,	0, /* 23 */
				sizeof(i4     ),	DB_INT_TYPE },

    { "QUERY_PAGE_LIMIT",	SCI_QPAGE_LIMIT,	0,	0,/* 24 */
				sizeof(i4     ),	DB_INT_TYPE },

    { "QUERY_COST_LIMIT",	SCI_QCOST_LIMIT,	0,	0, /* 25 */
				sizeof(i4     ),	DB_INT_TYPE },

    { "DB_PRIVILEGES",		SCI_DBPRIV,	0,	0, /* 26 */
				sizeof(i4     ),	DB_INT_TYPE },

    { "DATATYPE_MAJOR_LEVEL",	SC_MAJOR_LEVEL,	0,	0, /* 27 */
				sizeof(Sc_main_cb->sc_major_adf),
				DB_INT_TYPE },

    { "DATATYPE_MINOR_LEVEL",	SC_MINOR_LEVEL,	0,	0, /* 28 */
				sizeof(Sc_main_cb->sc_minor_adf),
				DB_INT_TYPE },

    { "COLLATION",	SC_COLLATION,	0,	0,
	sizeof(Sc_main_cb->sc_proxy_scb->scb_sscb.sscb_ics.ics_collation),
			DB_CHR_TYPE },

    { "LANGUAGE",	SC_LANGUAGE,	0,	0,
	  		ER_MAX_LANGSTR,	DB_CHR_TYPE },

    /* "MAXQUERY" is an alias for "MAXIO" */

    { "MAXQUERY",	SCI_MXIO,	0,	0,
			sizeof(i4     ),	DB_INT_TYPE },

    { "MAXIO",		SCI_MXIO,	0,	0,
			sizeof(i4     ),	DB_INT_TYPE },

    { "MAXROW",		SCI_MXROW,	0,	0,
			sizeof(i4     ),	DB_INT_TYPE },

    { "MAXCPU",		SCI_MXCPU,	0,	0,
			sizeof(i4     ),	DB_INT_TYPE },

    { "MAXPAGE",	SCI_MXPAGE,	0,	0,
			sizeof(i4     ),	DB_INT_TYPE },

    { "MAXCONNECT",	SCI_MXCONNECT,	0,	0,
			sizeof(i4     ),	DB_INT_TYPE },

    { "MAXIDLE",	SCI_MXIDLE,	0,	0,
			sizeof(i4     ),	DB_INT_TYPE },

    { "DB_ADMIN",	SCI_DB_ADMIN,	0,	0,	1,	DB_CHR_TYPE },

    { "SESSION_ID",	SCI_SESSID,	0,	0,	8,	DB_CHR_TYPE },

    { "INITIAL_USER",	SCI_INITIAL_USER,	0,	0,
			sizeof(DB_OWN_NAME),	DB_CHR_TYPE },

    { "SESSION_USER",	SCI_SESSION_USER,	0,	0,
			sizeof(DB_OWN_NAME),	DB_CHR_TYPE },

    { "SECURITY_PRIV",	SCI_SECURITY,	0,	0,	1,	DB_CHR_TYPE },
 
    { "SYSTEM_USER",	SCI_RUSERNAME,	0,	0,
			sizeof(DB_OWN_NAME),	DB_CHR_TYPE },

    { "ON_ERROR_STATE",	SC_ONERROR_STATE,	0,	0,
			32,			DB_CHR_TYPE },

    { "ON_ERROR_SAVEPOINT",	SC_SVPT_ONERROR,	0,	0,
			32,			DB_CHR_TYPE },

    { "SECURITY_AUDIT_LOG",	SCI_AUDIT_LOG,	0,	0,
			32,			DB_CHR_TYPE },

    { "IMA_VNODE",	SCI_IMA_VNODE,	0,	0,	
			64,			DB_CHR_TYPE },

    { "IMA_SERVER",	SCI_IMA_SERVER,	0,	0,	
			64,			DB_CHR_TYPE },

    { "IMA_SESSION",	SCI_IMA_SESSION,	0,	0,	
			32,			DB_CHR_TYPE },

    { "SECURITY_PRIV",	SCI_SECURITY,	0,	0,
    			1,		DB_CHR_TYPE },

    { "DB_NAME_CASE",		SCI_NAME_CASE,	0,	0,
    			5,		DB_CHR_TYPE },

    { "DB_DELIMITED_CASE",	SCI_DELIM_CASE,	0,	0,
    			5,		DB_CHR_TYPE },
    
    { "CONNECT_TIME_LIMIT",	SCI_CONNECT_LIMIT,	0,	0,	
			sizeof(i4     ),	DB_INT_TYPE },

    { "IDLE_TIME_LIMIT",	SCI_IDLE_LIMIT,	0,	0,	
			sizeof(i4     ),	DB_INT_TYPE },

    { "SESSION_PRIORITY_LIMIT",	SCI_PRIORITY_LIMIT,	0,	0,	
			sizeof(i4     ),	DB_INT_TYPE },

    { "SESSION_PRIORITY",	SCI_CUR_PRIORITY,	0,	0,	
			sizeof(i4     ),	DB_INT_TYPE },

    { "SECURITY_AUDIT_STATE",	SCI_AUDIT_STATE,	0,	0,
			32,			DB_CHR_TYPE },

    { "DB_REAL_USER_CASE",	SCI_REAL_USER_CASE,	0,	0,
    			5,	DB_CHR_TYPE },

};


#define NUM_DBMSINFO_REQUESTS \
	( sizeof(dbms_info_items) / sizeof(dbms_info_items[0]) )





/*{
** Name: ascd_initiate	- start up the INGRES server
**
** Description:
**      This routine initiates the DBMS Server.  The main control structure 
**      of SCF is initialized.  A message is logged indicating and attempt 
**      to start the server.  All internal SCF state is initialized. 
**      The client facilities are initialized.  Finally, a message is logged 
**      to indicate successful startup.  If any errors are encountered whilest 
**      attempting to start the server, the errors are logged, and an unsuccessful 
**      startup message is logged.
**
** Inputs:
**      csib                            CS_INFO_CB -- information about
**					user requested server configurations
**
** Outputs:
**      None
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      05-Dec-97 (sheor03)
**          Initialize URS.
**	04-Mar-1998 (shero03)
**	    Initialize API.
**	08-May-1998 (shero03)
**	    Initialize the API / Connection Mgr
**      28-May-98 (fanra01)
**          Add ICE initialization.
** 	11-Jun-1998 (wonst02)
** 	    Continue even if the URSM returns a warning that no app was started.
**      09-Nov-98 (fanra01)
**          Add ule_initiate for icesvr.
**      15-Feb-1999 (fanra01)
**          Add ICE license checking.
**          Rearrange initialisation of ICE to reduce the period between
**          registering with the name server and actually listening.
**      04-Mar-1999 (fanra01)
**          Add tracing thread.  The thread wakes up periodically and flushes
**          memory buffers to disk.
**      08-Mar-99 (fanra01)
**          Ensure that the GCA_INITIATE is performed before starting ice
**          server initialisation.  This ensures that first gca cb instance
**          defined contains the server class.  Ice server initialisation
**          invokes multiple GCA_INITIATEs through the api and no server class
**          information is saved.
**      12-Mar-1999 (fanra01)
**          Add Trace thread initialisation.
**	22-nov-1999 (somsa01)
**	    Moved ule_initiate() to proper place after retrieving the ICESVR
**	    id. Also, after a successful startup, write E_SC051D_LOAD_CONFIG
**	    to the errlog.
**      7-oct-2004 (thaju02)
**          Change size to SIZE_TYPE.
**      11-Feb-2005 (fanra01)
**          Removed urs initialization code.
**      27-Mar-2009 (hanal04) Bug 121857
**          Wrong number of parameters supplied to adg_startup(). Found
**          with xDEBUG defined in adf.h
*/
DB_STATUS
ascd_initiate( CS_INFO_CB  *csib )
{
    DB_STATUS           status;
    DB_STATUS		error;
    CL_SYS_ERR	        sys_err;
    STATUS		stat;
    SIZE_TYPE 		size;
    i4 			i;
    i4 			num_databases = 0;
    i4 			csib_idx_databases = -1;
    SCS_MCB		*mcb_head;
    SCV_LOC		*dbdb_loc;
    ADF_TAB_DBMSINFO	*dbinfo_block;
    SCF_CB		scf_cb;
    GCA_IN_PARMS	gca_cb;
    GCA_RG_PARMS	gcr_cb;
    SCD_CB		scd_cb;
    LOCATION		dbloc;
    char		loc_buf[MAX_LOC];
    char		*dbplace;
    GLOBALREF const
			char	Version[];
    GCA_LS_PARMS	local_crb;
    CS_CB               cscb;
    i4                  user_threads;
    i4                  dbms_tasks = 0;
    i4 			priority;
    i4 			sharedcache = FALSE;
    char		*cache_name = NULL;
    PTR                 tz_cb;  /* pointer to default TZ structure */
    i4 			sxf_memory = 0;
    PTR			block;
    bool		term_thread=FALSE;
    bool		secaudit_writer=FALSE;
    bool                cleanup_thread=FALSE;
    bool                logtrc_thread=FALSE;

    if (Sc_main_cb)
	return(E_SC0100_MULTIPLE_MEM_INIT);

    status = sc0m_get_pages(SC0M_NOSMPH_MASK,
			NEEDED_SIZE,
			&size,
			&block);
    Sc_main_cb = (SC_MAIN_CB *)block;
    if ((status) || (size != NEEDED_SIZE))
	return(E_SC0204_MEMORY_ALLOC);

    scd_mo_init();

    csib->cs_svcb = (PTR) Sc_main_cb;

	/*
	** Basic info about dynamic blocks transferred to the server control block
	*/
	csib->sc_main_sz = sizeof(SC_MAIN_CB);
	csib->scd_scb_sz = sizeof(SCD_SCB);
	csib->scb_typ_off = CL_OFFSETOF(SCD_SCB, scb_sscb) +
			            CL_OFFSETOF(SCS_SSCB, sscb_stype);
	csib->scb_fac_off = CL_OFFSETOF(SCD_SCB, scb_sscb) +
			            CL_OFFSETOF(SCS_SSCB, sscb_cfac);
	csib->scb_usr_off = CL_OFFSETOF(SCD_SCB, scb_sscb) +
			            CL_OFFSETOF(SCS_SSCB, sscb_ics) +
			            CL_OFFSETOF(SCS_ICS, ics_gw_parms);
	csib->scb_qry_off = CL_OFFSETOF(SCD_SCB, scb_sscb) +
			            CL_OFFSETOF(SCS_SSCB, sscb_ics) +
			            CL_OFFSETOF(SCS_ICS, ics_l_qbuf);
	csib->scb_act_off = CL_OFFSETOF(SCD_SCB, scb_sscb) +
			            CL_OFFSETOF(SCS_SSCB, sscb_ics) +
			            CL_OFFSETOF(SCS_ICS, ics_l_act1);
	csib->scb_ast_off = CL_OFFSETOF(SCD_SCB, scb_sscb) +
			            CL_OFFSETOF(SCS_SSCB, sscb_astate);

    MEfill(NEEDED_SIZE * SCU_MPAGESIZE, 0, (char *)Sc_main_cb);

    csib->cs_mem_sem = (CS_SEMAPHORE *) &Sc_main_cb->sc_sc0m.sc0m_semaphore;

    PCpid(&Sc_main_cb->sc_pid);
    MEmove((u_i2)STlength(Version),
		(PTR) Version,
		' ',
		sizeof(Sc_main_cb->sc_iversion),
		Sc_main_cb->sc_iversion);
    Sc_main_cb->sc_scf_version = SC_SCF_VERSION;	
    Sc_main_cb->sc_nousers = csib->cs_scnt;
    Sc_main_cb->sc_max_conns = csib->cs_scnt;
    Sc_main_cb->sc_rsrvd_conns = SCD_RESERVED_CONNS;
    Sc_main_cb->sc_listen_mask = (SC_LSN_REGULAR_OK | SC_LSN_SPECIAL_OK);
    Sc_main_cb->sc_irowcount = SC_ROWCOUNT_INITIAL;
    Sc_main_cb->sc_selcnt = 0;
    Sc_main_cb->sc_session_chk_interval=SC_DEF_SESS_CHK_INTERVAL;
    Sc_main_cb->sc_capabilities = Sc_server_type; /* glob from scdmain.c */
    Sc_main_cb->sc_names_reg = TRUE;
    if (csib->cs_ascnt == 0)
    {
	csib->cs_ascnt = csib->cs_scnt;
    }
    Sc_main_cb->sc_soleserver = DMC_S_MULTIPLE;
    Sc_main_cb->sc_solecache = DMC_S_MULTIPLE;
    Sc_main_cb->sc_acc = 16;

    /* Initialize it with DB_MAXTUP */ 
    Sc_main_cb->sc_maxtup = DB_MAXTUP;

    Sc_main_cb->sc_min_priority = csib->cs_min_priority;
    Sc_main_cb->sc_max_priority = csib->cs_max_priority;
    Sc_main_cb->sc_min_usr_priority = csib->cs_min_priority;
    Sc_main_cb->sc_norm_priority = csib->cs_norm_priority;
    /*
    ** Allow 4 for system threads to run at various higher priorities
    ** if possible.
    */
    Sc_main_cb->sc_max_usr_priority = Sc_main_cb->sc_max_priority-4;
    /*
    ** Check if this CS doesn't allow us enough headroom between norm and max,
    */
    if (Sc_main_cb->sc_max_usr_priority < Sc_main_cb->sc_norm_priority+1)
    {
        Sc_main_cb->sc_max_usr_priority = Sc_main_cb->sc_norm_priority+1;
        if (Sc_main_cb->sc_max_usr_priority > Sc_main_cb->sc_max_priority)
    	    Sc_main_cb->sc_max_usr_priority = Sc_main_cb->sc_max_priority;

    }
    Sc_main_cb->sc_rule_depth = QEF_STACK_MAX;		/* Default init value */
    Sc_main_cb->sc_events = SCE_NUM_EVENTS;		/* Default init value */
    Sc_main_cb->sc_startup_time = TMsecs();     /* remember when we started */

    ult_init_macro(&Sc_main_cb->sc_trace_vector, SCF_NBITS, SCF_VPAIRS, SCF_VONCE);

    /*
    ** Initialize the scd_cb, the collection of oddball parameters that
    ** have yet to be moved into the proper facilities' control blocks.
    ** Also, initialize those the pieces of qsf_rcb and sxf_rcb that
    ** scd_options() can touch.
    */

    scd_cb.max_dbcnt = 0;
    scd_cb.qef_data_size = 2560;	/* QEF Default */
    scd_cb.num_logwriters = 1;
    scd_cb.start_gc_thread = 1;
    scd_cb.cp_timeout = 600;
    scd_cb.sharedcache = FALSE;
    scd_cb.cache_name = NULL;
    scd_cb.dblist = NULL;
    scd_cb.c2_mode = 0;
    scd_cb.local_vnode = PMgetDefault( 1 );	/* cheesy! */
    scd_cb.secure_level=NULL;	/* No secure level initially */
    scd_cb.num_writealong = scd_cb.num_readahead = 0;
    scd_cb.num_ioslaves = 0;

    switch( Sc_server_type )
    {
    case SC_INGRES_SERVER:	scd_cb.server_class = GCA_INGRES_CLASS; break;
    case SC_ICE_SERVER:	scd_cb.server_class = GCA_ICE_CLASS; break;
    case SC_RMS_SERVER:		scd_cb.server_class = GCA_RMS_CLASS; break;
    case SC_STAR_SERVER:	scd_cb.server_class = GCA_STAR_CLASS; break;
    case SC_RECOVERY_SERVER:	scd_cb.server_class = "RECOVERY"; break;
    case SC_IOMASTER_SERVER:  	scd_cb.server_class = "IOMASTER"; break;
    default:			scd_cb.server_class = GCA_INGRES_CLASS; break;
    }

#ifdef FOR_EXAMPLE_ONLY

    qsf_rcb.qsf_pool_sz = 0;
    sxf_rcb.sxf_pool_sz = 0;

    /*
     * Fetch options from PM and distribute their values into the
     * various facilities' control blocks.
     */

    status = ascd_options( 
		dca + dca_index,
		&dca_index,
		&opf_cb,
		&rdf_ccb,
		&dmc,
		&psq_cb,
		&qsf_rcb,
		&sxf_rcb,
		&scd_cb );

    if (status)
	return status;


    /*
    ** Check for startup parameter conflicts
    */

    /*@FIX_ME@*/

    /*
    ** Set default CURSOR_MODE flags for dbmsinfo()
    */
    if (Sc_main_cb->sc_csrflags == 0x00L)
	/* Default is CURSOR_UPDATE_MODE=DEFERRED */
	Sc_main_cb->sc_csrflags |= SC_CSRDEFER;

    if (Sc_main_cb->sc_defcsrflag == 0x00L)
	/* Default cursor open mode CURSOR_DEFAULT_MODE=UPDATE */
	Sc_main_cb->sc_defcsrflag |= SC_CSRUPD;

    if (Sc_main_cb->sc_capabilities & SC_RECOVERY_SERVER)
    {
	/* don't turn off name server registration here, or IMA won't work */
        /*
        ** NOTE - the recovery server should not run with B1 or C2 security. 
        */
	scd_cb.c2_mode = FALSE;
	scd_cb.secure_level = NULL;
    }

    if (Sc_main_cb->sc_capabilities & SC_STAR_SERVER)
    {
        Sc_main_cb->sc_events = 0;	/* No Events for Star */
	Sc_main_cb->sc_writebehind = 0;	/* No Writebehinds for Star */
	Sc_main_cb->sc_fastcommit = 0;  /* No FastCommit for Star */
        Sc_main_cb->sc_dmcm = 0;        /* No DMCM for Star */
	scd_cb.sharedcache = 0;		/* No SharedCache for Star */
	scd_cb.c2_mode = FALSE;		/* No C2 for Star, but allow B1 */
	dmc.dmc_flags_mask |= DMC_STAR_SVR; /* tell DMF this is star */
    }
    else
    {
	Sc_main_cb->sc_nostar_recovr = TRUE;  /* Star only, turn it off */
	Sc_main_cb->sc_no_star_cluster = FALSE; /* Star only, turn it off */
    }

    if (Sc_main_cb->sc_capabilities & SC_IOMASTER_SERVER)
    {
	Sc_main_cb->sc_writealong  = max(scd_cb.num_writealong, 10);
	Sc_main_cb->sc_readahead   = max(scd_cb.num_readahead, 1); 	
	IOmaster.num_ioslaves      = max(scd_cb.num_ioslaves,  
	    (Sc_main_cb->sc_writealong + Sc_main_cb->sc_readahead));
    }
    else
    if (Sc_server_type == SC_INGRES_SERVER)
	Sc_main_cb->sc_readahead   = scd_cb.num_readahead;
	
    /*
    ** Disallow C2 security without the capability.
    */
    if( scd_cb.c2_mode && CIcapability(CI_RESRC_CNTRL) != OK )
    {
	sc0e_0_put(E_SC032E_C2_NEEDS_KME, 0);
	scd_cb.c2_mode = FALSE;
	/* Continue here, just warning */
    }
    if(scd_cb.c2_mode)
    {
	status=scd_sec_init_config(FALSE, scd_cb.c2_mode);
	if(status!=E_DB_OK)
		return (E_SC0124_SERVER_INITIATE);
	Sc_main_cb->sc_capabilities |= SC_C_C2SECURE;
    }

    /*
    ** Terminator thread needed if KME and not disabled.
    ** Recovery/IOmaster servers do not have a terminator thread since they
    ** doesn't have regular users currently
    */
    if ( Sc_main_cb->sc_session_chk_interval>0 &&
       !(Sc_main_cb->sc_capabilities & SC_RECOVERY_SERVER) &&
       !(Sc_main_cb->sc_capabilities & SC_IOMASTER_SERVER))
	    term_thread=TRUE;

    /*
    ** Start security audit writer if C2 server and not 
    ** recovery server.
    */
    if (scd_cb.c2_mode && 
	!(Sc_main_cb->sc_capabilities & SC_RECOVERY_SERVER))
	    secaudit_writer=TRUE;
    
    /*
    ** If this server will have dedicated server tasks, then alter
    ** the dispatcher to allow more threads so that the server tasks
    ** will not eat into the max_sessions parameters.
    */
    user_threads = Sc_main_cb->sc_nousers;

    /* reserve sessions for special users on servers with sessions */
    if ( !(Sc_main_cb->sc_capabilities & SC_RECOVERY_SERVER) )
	user_threads += Sc_main_cb->sc_rsrvd_conns;

    /* # of write behind threads */
    dbms_tasks = Sc_main_cb->sc_writebehind;

    /*
    ** Add a special thread for Consistency Point handling.
    ** There is now a Consistency Point Thread in all servers.
    */
    dbms_tasks++;

    if (Sc_main_cb->sc_capabilities & SC_RECOVERY_SERVER)
    {
	dbms_tasks += 2;    /* recovery thread + deadlock detect thread */
	if (scd_cb.cp_timeout)
	    dbms_tasks++;   /* add cp timer thread */
    }

    if (Sc_main_cb->sc_capabilities & SC_IOMASTER_SERVER)
    {
    	dbms_tasks += Sc_main_cb->sc_writealong;
    }
    dbms_tasks += Sc_main_cb->sc_readahead;

    /*
    ** All servers can also have the logging & locking system special
    ** threads: check-dead, group-commit, force-abort, and logwriters.
    */
    dbms_tasks += scd_cb.num_logwriters;
    dbms_tasks += scd_cb.start_gc_thread;
    dbms_tasks += 3;

    /* Add a special thread for EVENT handling */
    if (Sc_main_cb->sc_events != 0)
	dbms_tasks++;
    
    /* Add a special thread for security auditing */
    if (scd_cb.c2_mode)
	dbms_tasks++;

    /* Add a special thread for session termination */
    if (term_thread)
	dbms_tasks++;

    /* Add a special thread for security audit writer */
    if (secaudit_writer)
	dbms_tasks++;

#endif /* FOR_EXAMPLE_ONLY */
    /*
    ** If this is an ice server start a cleanup thread
    */
    if (Sc_main_cb->sc_capabilities & SC_ICE_SERVER)
    {
        cleanup_thread = TRUE;
        dbms_tasks+=1;
        user_threads = Sc_main_cb->sc_nousers;
    }
    if (dbms_tasks || (user_threads != cscb.cs_scnt) )
    {
	cscb.cs_scnt = user_threads + dbms_tasks;
	cscb.cs_ascnt = csib->cs_scnt;
	cscb.cs_stksize = CS_NOCHANGE;
	stat = CSalter(&cscb);
	if (stat != OK)
	{
	    sc0e_0_put(stat, 0);
	    sc0e_put(E_SC0242_ALTER_MAX_SESSIONS, 0, 2,
		     sizeof(csib->cs_scnt), (PTR)&csib->cs_scnt,
		     sizeof(cscb.cs_scnt), (PTR)&cscb.cs_scnt,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0 );
	    return(E_SC0124_SERVER_INITIATE);
	}
    }

    /*
    ** Determine the number of threads for each facility startup parameter.
    ** Some special threads do not require all facilities.
    */
    Sc_main_cb->sc_nousers += dbms_tasks;
#ifdef FOR_EXAMPLE_ONLY
    num_psf_sessions = user_threads;
    num_qef_sessions = user_threads;
    num_qsf_sessions = user_threads;
    num_opf_sessions = user_threads;
    num_dmf_sessions = user_threads + dbms_tasks;
#endif /* FOR_EXAMPLE_ONLY */ 

    /* First, initialize SCF */

    for (;;)
    {
	scf_cb.scf_length = sizeof(SCF_CB);
	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_facility = DB_SCF_ID;
	scf_cb.scf_session = DB_NOSESSION;
	Sc_main_cb->sc_state = SC_INIT;
	
	status = sc0m_initialize();	    /* initialize the memory allocation */
	if (status != E_SC_OK)
	    break;

	TMend(&Sc_main_cb->sc_timer);

	/* now allocate the queues and que headers so that work can proceed normally */

	status = sc0m_allocate(SC0M_NOSMPH_MASK,
				(i4     )sizeof(SCS_MCB),
				MCB_ID,
				(PTR) SCD_MEM,
				MCB_ASCII_ID,
				(PTR *) &mcb_head);
	if (status != E_SC_OK)
	    break;
	mcb_head->mcb_next = mcb_head->mcb_prev = mcb_head;

	mcb_head->mcb_facility = DB_SCF_ID;
	Sc_main_cb->sc_mcb_list = mcb_head;
	CSw_semaphore( &Sc_main_cb->sc_mcb_semaphore, CS_SEM_SINGLE, "SC MCB" );
	CSw_semaphore( &Sc_main_cb->sc_misc_semaphore, CS_SEM_SINGLE, "SC MISC" );
	
	Sc_main_cb->sc_kdbl.kdb_next = (SCV_DBCB *) &Sc_main_cb->sc_kdbl;
	Sc_main_cb->sc_kdbl.kdb_prev = (SCV_DBCB *) &Sc_main_cb->sc_kdbl;

	 
	CSw_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore, CS_SEM_SINGLE,
			"SCS kdb_semaphore");

	CSw_semaphore(&Sc_main_cb->sc_gcsem, CS_SEM_SINGLE,
			"SCS gcsem");

	break;
    }

    if (status)
    {
	if (status & E_SC_MASK)
	{
	    sc0e_0_put(E_SC0204_MEMORY_ALLOC, 0);
	    sc0e_0_put(status, 0);
	}
	else
	{
	    sc0e_0_put(E_SC0200_SEM_INIT, 0);
	    sc0e_0_put(scf_cb.scf_error.err_code, 0);
	}
	return(E_SC0124_SERVER_INITIATE);
    }

    Sc_main_cb->sc_dbdb_loc = (char *) Sc_main_cb + sizeof(SC_MAIN_CB);
    dbdb_loc = (SCV_LOC *) Sc_main_cb->sc_dbdb_loc;
    MEmove(8, "$default", ' ',
	    sizeof(dbdb_loc->loc_entry.loc_name),
	    (char *)&dbdb_loc->loc_entry.loc_name);
    LOingpath("II_DATABASE",
	"iidbdb",
	LO_DB,
	&dbloc);
    LOcopy(&dbloc, loc_buf, &dbloc);
    LOtos(&dbloc, &dbplace);
    dbdb_loc->loc_entry.loc_l_extent = STlength(dbplace);
    MEmove( (u_i2)dbdb_loc->loc_entry.loc_l_extent,
	    dbplace,
	    ' ',
	    sizeof(dbdb_loc->loc_entry.loc_extent),
	    dbdb_loc->loc_entry.loc_extent);

    dbdb_loc->loc_entry.loc_flags = LOC_DEFAULT | LOC_DATA;
    dbdb_loc->loc_l_loc_entry = 1 * sizeof(DMC_LOC_ENTRY);
    

    for (;;)
    {
	gca_cb.gca_expedited_completion_exit = ascs_ctlc;
	gca_cb.gca_normal_completion_exit = scc_gcomplete;
	gca_cb.gca_alloc_mgr = sc0m_gcalloc;
	gca_cb.gca_dealloc_mgr = sc0m_gcdealloc;
	gca_cb.gca_p_semaphore = CSp_semaphore;
	gca_cb.gca_v_semaphore = CSv_semaphore;
	gca_cb.gca_modifiers = GCA_ASY_SVC|GCA_API_VERSION_SPECD;
	gca_cb.gca_api_version = GCA_API_LEVEL_5;
        gca_cb.gca_local_protocol = GCA_PROTOCOL_LEVEL_62;
	gca_cb.gca_decompose = NULL;	/* EJLFIX: Fill in when Adf is ready */
	gca_cb.gca_cb_decompose = NULL;	/* EJLFIX: Fill in when Adf is ready */
	gca_cb.gca_lsncnfrm = CSaccept_connect;

#ifdef GCA_BATCHMODE
        if ((Sc_main_cb->sc_capabilities & SC_BATCH_SERVER) &&
            (Sc_main_cb->sc_capabilities & SC_GCA_SINGLE_MODE))
	   gca_cb.gca_modifiers |= GCA_BATCHMODE;
#endif


	status = IIGCa_call(GCA_INITIATE,
		    (GCA_PARMLIST *)&gca_cb,
		    GCA_SYNC_FLAG,
		    (PTR)0,
		    -1,
		    &error);
	if (DB_FAILURE_MACRO(status))
	{
	    sc0e_0_put(error, 0);
	    break;
	}
	else if (gca_cb.gca_status != E_GC0000_OK)
	{
	    sc0e_0_put(status = gca_cb.gca_status, &gca_cb.gca_os_status);
	    break;
	}
	/* Once the switch to GCA calls with CB is made,
	** sc_gca_cb will be used. For now mark it as non-zero
	** to distinguish it from standalone mode. */
	Sc_main_cb->sc_gca_cb = (PTR)1;

	gcr_cb.gca_srvr_class = scd_cb.server_class;

        /*
        ** Initialization ICE
        */
        if (Sc_main_cb->sc_capabilities & SC_ICE_SERVER)
        {
            DB_ERROR        err;

            if ((status = asct_initialize ()) == E_DB_OK)
            {
                logtrc_thread = TRUE;
                dbms_tasks+=1;
            }

            status = WTSInitialize(&err);
            if (status == E_DB_WARN)
                status = E_DB_OK;                /* Continue without ICE */
            else if (status == E_DB_ERROR)
            {
                sc0e_0_put(err.err_code, 0);
                break;
            }
            else
                Sc_main_cb->sc_facilities |= 1 << DB_ICE_ID;
        }

	/*
	** Formulate arguments to GCA_REGISTER in support of dblist.
	*/

	if( !scd_cb.dblist || !*scd_cb.dblist )
	{
	    gcr_cb.gca_l_so_vector = 0;
	    gcr_cb.gca_served_object_vector = NULL;
	    if (Sc_server_type == SC_IOMASTER_SERVER)
	    {
	    	IOmaster.dbcount  = 0;    
	    	IOmaster.dblist   = NULL;
	    }
	}
	else
	{
	    char    *dblist;
	    i4	    dblist_cnt = 0;
	    i4 	    waswhite = 1;
	    char    *p;

	    /* Count the whitespace or comma separated words in dblist 
	    ** - STcountwords()? 
	    ** Transform COMMAs into SPACEs so that STgetwords() still works.
	    */

	    for(  p = scd_cb.dblist; *p; CMnext( p ) )
	    {
		i4  iswhite;
		if( !CMcmpcase( p, "," ) )
		    *p = ' ';		/* Transform COMMA into SPACE */
		iswhite = CMwhite( p );

		if( !iswhite && waswhite )
		    dblist_cnt++;
		
		waswhite = iswhite;
	    }

	    /* Allocate blocks for a copy of the string and for the word list */

	    status = sc0m_allocate(
			SC0M_NOSMPH_MASK,
			(i4     )(STlength( scd_cb.dblist ) + 1),
			MCB_ID,
			(PTR) SCD_MEM,
			MCB_ASCII_ID,
			&block);

	    if (DB_FAILURE_MACRO(status))
	    {
		sc0e_0_put(E_SC0204_MEMORY_ALLOC, 0);
		sc0e_0_put(status, 0);
		break;
	    }

	    dblist = (char *)block;

	    status = sc0m_allocate(
			SC0M_NOSMPH_MASK,
			(i4     )(dblist_cnt *
				sizeof( gcr_cb.gca_served_object_vector)),
			MCB_ID,
			(PTR) SCD_MEM,
			MCB_ASCII_ID,
			&block);

	    if (DB_FAILURE_MACRO(status))
	    {
		sc0e_0_put(E_SC0204_MEMORY_ALLOC, 0);
		sc0e_0_put(status, 0);
		break;
	    }

	    gcr_cb.gca_l_so_vector = dblist_cnt;
	    gcr_cb.gca_served_object_vector = (char **)block;

	    STcopy( scd_cb.dblist, dblist );
	    STgetwords( dblist, &dblist_cnt, gcr_cb.gca_served_object_vector );

	    if (Sc_server_type == SC_IOMASTER_SERVER)
	    {
	    	IOmaster.dbcount = dblist_cnt;
	    	IOmaster.dblist   = dblist;  /* list of null termed strings */ 
	    }
	}

	gcr_cb.gca_installn_id = 0;
	gcr_cb.gca_process_id = 0;
	gcr_cb.gca_no_connected = Sc_main_cb->sc_nousers;
	gcr_cb.gca_no_active = Sc_main_cb->sc_nousers;
	gcr_cb.gca_modifiers = 0;

	if (Sc_main_cb->sc_names_reg != TRUE)
	    gcr_cb.gca_modifiers = GCA_RG_NO_NS;

	if (Sc_main_cb->sc_soleserver == DMC_S_SINGLE)
	    gcr_cb.gca_modifiers |= GCA_RG_SOLE_SVR;

	/*
	** Set GCM MIB support flags
	*/
	if( !(Sc_main_cb->sc_capabilities & SC_STAR_SERVER) )
	    gcr_cb.gca_modifiers |= GCA_RG_INSTALL;

	if (Sc_main_cb->sc_capabilities & SC_RECOVERY_SERVER)
	{
	    gcr_cb.gca_modifiers |= GCA_RG_IIRCP;
	    gcr_cb.gca_srvr_class = GCA_IUSVR_CLASS;
	}
	else
	{
	    gcr_cb.gca_modifiers |= GCA_RG_INGRES;
	}

	gcr_cb.gca_listen_id = 0;

	status = IIGCa_call(GCA_REGISTER,
			    (GCA_PARMLIST *)&gcr_cb,	/* Parameter list */
			    GCA_SYNC_FLAG,	/* Synchronous execution */
			    (PTR)0,		/* ID for asynch rtn. if any */
			    (i4     ) -1,	/* No timeout */
			    &error);		/* Error type if E_DB_ERROR
						   was returned */

	if (DB_FAILURE_MACRO(status))
	{
	    sc0e_0_put(error, 0);
	    break;
	}
	else if (gcr_cb.gca_status != E_GC0000_OK)
	{
	    sc0e_0_put(status = gcr_cb.gca_status, &gcr_cb.gca_os_status);
	    break;
	}

	MEmove(STlength(gcr_cb.gca_listen_id),
		gcr_cb.gca_listen_id,
		' ',
		sizeof(Sc_main_cb->sc_sname),
		Sc_main_cb->sc_sname);
	Sc_main_cb->sc_facilities |= 1 << DB_GCF_ID;

	/* Pass down listen address as cs_name */

	STlcopy( gcr_cb.gca_listen_id, csib->cs_name, sizeof(csib->cs_name)-1 );

	MEmove( 
	    STlength( scd_cb.local_vnode ),
	    scd_cb.local_vnode,
	    ' ',
	    sizeof( Sc_main_cb->sc_ii_vnode ),
	    Sc_main_cb->sc_ii_vnode );

	/*
	** Get the node for STAR
	*/

	if (Sc_main_cb->sc_capabilities & SC_STAR_SERVER)
	{

	    ule_initiate(Sc_main_cb->sc_ii_vnode,
			 sizeof( Sc_main_cb->sc_ii_vnode ),
			 Sc_main_cb->sc_sname,
			 sizeof(Sc_main_cb->sc_sname));
	}

	/* 
	** Initialize semaphore and load the default TZ structure.
	*/
	CSw_semaphore((CS_SEMAPHORE *) &Sc_main_cb->sc_tz_semaphore, 
					CS_SEM_SINGLE,
					"SCF TZ sem" );
	stat = TMtz_init(&tz_cb);
	if(stat != OK)
	{
	    PTR  tz_name;

	    sc0e_0_put(stat, 0);
	    if (stat != TM_NO_TZNAME)
	    {
		/* 
		** Display timezone name as well if problem is not because
		** of II_TIMEZONE_NAME not defined
		*/
		NMgtAt(ERx("II_TIMEZONE_NAME"), &tz_name);		
		sc0e_put(E_SC033D_TZNAME_INVALID, 0, 3,
		      STlength((char *)tz_name),
		      tz_name,
		      STlength(SystemVarPrefix), SystemVarPrefix,
		      STlength(SystemVarPrefix), SystemVarPrefix,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0);		
	    }
	    status = E_DB_FATAL;
	    break;
	}

	/*
	** Initialize the server's II_DATE_CENTURY_BOUNDARY
	*/
	{
	    i4  year_cutoff;

	    stat = TMtz_year_cutoff(&year_cutoff);

	    if (stat != OK )
	    {
	    	sc0e_0_put(stat, 0);
		sc0e_put(E_SC037B_BAD_DATE_CUTOFF, 0, 2,
				  STlength(SystemVarPrefix), SystemVarPrefix,
				  0, 
				  (PTR)(SCALARP)year_cutoff);
	    	status = E_DB_FATAL;
		break;
	    }
	}

	/*
	** Now move on and initiate the other facilities in the server.
	** The order of invocation depends upon who calls who to do
        ** work.  At the moment it is ADF, ULF, GWF, DMF, then random.
	*/

	size = adg_srv_size();
	scf_cb.scf_scm.scm_functions = 0;
	scf_cb.scf_scm.scm_in_pages =
	    ((size + SCU_MPAGESIZE - 1) & ~(SCU_MPAGESIZE - 1)) / SCU_MPAGESIZE;
	status = scf_call(SCU_MALLOC, &scf_cb);
	if (status)
	{
	    sc0e_0_put(scf_cb.scf_error.err_code, 0);
	    break;
	}
	Sc_main_cb->sc_advcb = (PTR) scf_cb.scf_scm.scm_addr;
	if (status)
	    break;

	status = sc0m_allocate(SC0M_NOSMPH_MASK,
				(i4     )(sizeof(SC0M_OBJECT) +
				sizeof(ADF_TAB_DBMSINFO) -
					sizeof(ADF_DBMSINFO) +
					(NUM_DBMSINFO_REQUESTS * sizeof(ADF_DBMSINFO))),
				DB_ADF_ID,
				(PTR) SCD_MEM,
				CV_C_CONST_MACRO('a','d','v','b'),
				(PTR *) &dbinfo_block);
	if (status)
	    break;
	    
	dbinfo_block = (ADF_TAB_DBMSINFO *)
			    ((char *) dbinfo_block + sizeof(SC0M_OBJECT));
	dbinfo_block->tdbi_type = ADTDBI_TYPE;
	dbinfo_block->tdbi_ascii_id = ADTDBI_ASCII_ID;
	dbinfo_block->tdbi_numreqs = NUM_DBMSINFO_REQUESTS;
	for (i = 0; i < NUM_DBMSINFO_REQUESTS; i++)
	{
	    dbinfo_block->tdbi_reqs[i].dbi_func = ascd_dbinfo_fcn;
	    dbinfo_block->tdbi_reqs[i].dbi_num_inputs = 0;
	    dbinfo_block->tdbi_reqs[i].dbi_lenspec.adi_lncompute = ADI_FIXED;

	    STcopy( dbms_info_items[i].info_name,
		   dbinfo_block->tdbi_reqs[i].dbi_reqname );
	    dbinfo_block->tdbi_reqs[i].dbi_reqcode =
		dbms_info_items[i].info_reqcode;
	    dbinfo_block->tdbi_reqs[i].dbi_num_inputs =
		dbms_info_items[i].info_inputs;
	    dbinfo_block->tdbi_reqs[i].dbi_dt1 =
		dbms_info_items[i].info_dt1;
	    dbinfo_block->tdbi_reqs[i].dbi_lenspec.adi_fixedsize =
		dbms_info_items[i].info_fixedsize;
	    dbinfo_block->tdbi_reqs[i].dbi_dtr = 
		dbms_info_items[i].info_type;
	}

	status = adg_startup(Sc_main_cb->sc_advcb, size, dbinfo_block, (i4)scd_cb.c2_mode);
	if (status)
	{
	    sc0e_0_put(E_SC012C_ADF_ERROR, 0);
	    break;
	}
	Sc_main_cb->sc_facilities |= 1 << DB_ADF_ID;

	/*
	** Start OME/UDTS unless disabled
	*/
	if (!(Sc_main_cb->sc_secflags & SC_SEC_OME_DISABLED))
	{
	    DB_ERROR		    error;
	    PTR			    new_block;

	    status = sca_add_datatypes((SCD_SCB *) 0,
					Sc_main_cb->sc_advcb,
					scf_cb.scf_scm.scm_in_pages,
					DB_SCF_ID,  /* Deallocate old */
					&error,
					(PTR *) &new_block,
					0);
	    if (status)
		break;
	    Sc_main_cb->sc_advcb = new_block;
	}

	/* Add scd_adf_printf() to FEXI table */
        {
	    ADF_CB                 adf_scb;
	    
	    status = adg_add_fexi( &adf_scb, ADI_02ADF_PRINTF_FEXI, 
				   &ascd_adf_printf);
	    if (status)
	    {
		sc0e_0_put(E_SC012C_ADF_ERROR, 0);
		break;
	    }
	}

	/* ULF has no initiation */

	/* Initialize the API 	*/
	{
	  IIAPI_INITPARM  initParm;
	  II_PTR		  lerr;
	  initParm.in_timeout = -1;
	  initParm.in_version = IIAPI_VERSION_1;
	  IIapi_initialize( &initParm );
	  if (initParm.in_status != IIAPI_ST_SUCCESS)
	  {
	      sc0e_0_put(E_SC0124_SERVER_INITIATE, 0);  /* FIXME */
	      break;
	  }
	  else
    	/* Initialize the Connection Manager */
    	IIcm_Initialize((PTR)&lerr);
	  	
	}  /* Initialize the API */

	ule_initiate(Sc_main_cb->sc_ii_vnode,
		STlength(Sc_main_cb->sc_ii_vnode),
		Sc_main_cb->sc_sname,
		sizeof(Sc_main_cb->sc_sname));

#ifdef FOR_EXAMPLE_ONLY
    /* Now do GWF -- before DMF because DMF will ask GWF for info. */

	/*
	** Tell GWF facility the location of dmf_call().
	*/
	gw_rcb.gwr_type     = GWR_CB_TYPE;
	gw_rcb.gwr_length   = sizeof(GW_RCB);
	/* gw_rcb.gwr_dmf_cptr = (DB_STATUS (*)())dmf_call; */
	gw_rcb.gwr_dmf_cptr = NULL;
	gw_rcb.gwr_gca_cb = Sc_main_cb->sc_gca_cb;

	status = gwf_call( GWF_INIT, &gw_rcb );
	if (status)
	{
	    sc0e_0_put(gw_rcb.gwr_error.err_code, 0);
	    break;
	}
	Sc_main_cb->sc_gwbytes = gw_rcb.gwr_scfcb_size;
	Sc_main_cb->sc_gwvcb = gw_rcb.gwr_server;

	/* schang 04/21/92 GW merge
	** if the gateway returned its release identifier, stuff this
	** into the server control block to be coughed up when the user
	** wonders "select dbmsinfo( '_version' )"
	*/

	if ( gw_rcb.gwr_out_vdata1.data_address != 0 )
	{
	    MEmove( gw_rcb.gwr_out_vdata1.data_out_size,
		gw_rcb.gwr_out_vdata1.data_address,
		' ',
		sizeof( Sc_main_cb->sc_iversion ),
		Sc_main_cb->sc_iversion );
	}

	if (Sc_main_cb->sc_capabilities & SC_STAR_SERVER)
	{
	    /* For STAR start a minimal DMF */
	    dca[dca_index].char_id = DMC_C_SCANFACTOR;
	    dca[dca_index++].char_fvalue = 8.0;
	    dca[dca_index].char_id = DMC_C_BUFFERS;
	    dca[dca_index++].char_value  = 30;
	    scd_cb.cp_timeout = 0;
	}
    
	/* Now do DMF */

	dmc.dmc_char_array.data_address =  (char *) dca;
	dca[dca_index].char_id = DMC_C_ABORT;
	dca[dca_index++].char_pvalue = (PTR) scs_log_event;
	dca[dca_index].char_id = DMC_C_LSESSION;
	dca[dca_index++].char_value = num_dmf_sessions;
	dca[dca_index].char_id = DMC_C_TRAN_MAX;
	dca[dca_index++].char_value = csib->cs_ascnt + dbms_tasks;
	if (scd_cb.cp_timeout)
	{
	    dca[dca_index].char_id = DMC_C_CP_TIMER;
	    dca[dca_index++].char_value = scd_cb.cp_timeout;
	}

	if (scd_cb.c2_mode)
		dmc.dmc_flags_mask |= DMC_C2_SECURE;

	/*
	** If shared cache has been specified then pass the cache
	** name to DMF.
	*/
	dmc.dmc_cache_name = NULL;
	if (scd_cb.sharedcache)
	    dmc.dmc_cache_name = scd_cb.cache_name;

	/* if parms indicated batch mode, let dmf know. */
	if (Sc_main_cb->sc_capabilities & SC_BATCH_SERVER)
	    dmc.dmc_flags_mask |= DMC_BATCHMODE;

	/* if parms indicated SINGLE SERVER mode, let dmf know */
	dmc.dmc_s_type = Sc_main_cb->sc_soleserver;

	/*
	** If max database count was set in options, then use that for
	** database_max, otherwise use num_users + 1 (extra for iidbdb).
	*/
	dca[dca_index].char_id = DMC_C_DATABASE_MAX;
	if (scd_cb.max_dbcnt)
	    dca[dca_index++].char_value = scd_cb.max_dbcnt;
	else
	    dca[dca_index++].char_value = num_dmf_sessions + 1;

        /* pass # of readahead threads, for storage alloc */
        dca[dca_index].char_id = DMC_C_NUMRAHEAD;
        dca[dca_index++].char_value = Sc_main_cb->sc_readahead;

	/* Make sure we didn't exceed the size of dmc */
	if (dca_index > (sizeof (dca) / sizeof(DMC_CHAR_ENTRY)))
	{
	    sc0e_0_put(E_SC0024_INTERNAL_ERROR, 0);
	    return(E_SC0124_SERVER_INITIATE);
	}

	dmc.dmc_char_array.data_in_size = 
	    dca_index * sizeof(DMC_CHAR_ENTRY);

	status = dmf_call(DMC_START_SERVER, &dmc);
	if (status)
	{
	    sc0e_0_put(dmc.error.err_code, 0);
	    break;
	}
	Sc_main_cb->sc_dmbytes = dmc.dmc_scfcb_size;
	Sc_main_cb->sc_dmvcb = dmc.dmc_server;


	if (!(Sc_main_cb->sc_capabilities & SC_STAR_SERVER))
	{
	  /* Initialize all tuple size */
          dmc.dmc_op_type = DMC_BMPOOL_OP;
          dmc.dmc_flags_mask = DMC_TUPSIZE;
          dmc.dmc_char_array.data_address = (PTR) dmc_char;
          dmc.dmc_char_array.data_out_size = sizeof(DMC_CHAR_ENTRY) *
						DB_MAX_CACHE;
          status = dmf_call(DMC_SHOW, (PTR) &dmc);
          if (status)
          {
	    sc0e_0_put(dmc.error.err_code, 0);
	    break;
          }
	  for (i = 0; i < DB_MAX_CACHE; i++)
	  {
	    if (dmc_char[i].char_value == DMC_C_OFF)
              Sc_main_cb->sc_tupsize[i] = 0;
	    else
	    {
              Sc_main_cb->sc_tupsize[i] = dmc_char[i].char_value;
              Sc_main_cb->sc_maxtup = dmc_char[i].char_value;
	    }
	  }

	  /* Initialize all page size */
          dmc.dmc_op_type = DMC_BMPOOL_OP;
          dmc.dmc_flags_mask = 0;
          dmc.dmc_char_array.data_address = (PTR) dmc_char;
          dmc.dmc_char_array.data_out_size = sizeof(DMC_CHAR_ENTRY) *
					     DB_MAX_CACHE;
          status = dmf_call(DMC_SHOW, (PTR) &dmc);
          if (status)
          {
	    sc0e_0_put(dmc.error.err_code, 0);
	    break;
          }
	  for (i = 0, pagesize = DB_MIN_PAGESIZE; i < DB_MAX_CACHE; 
		i++, pagesize *= 2)
	  {
	    if (dmc_char[i].char_value == DMC_C_ON)
	    {
              Sc_main_cb->sc_pagesize[i] = pagesize;
              Sc_main_cb->sc_maxpage = pagesize;
	    }
	    else
              Sc_main_cb->sc_pagesize[i] = 0;
	  }
	}
	else {
	  /*
	  ** For Star we have to have maxtup size etc. Otherwise, scd_main
	  ** will allocate smaller space for tuple area. Though at this
	  ** time, we do not know about the local buffer manager of the 
	  ** coordinator. So fake it with the max size, for now and may be
	  ** we will readjust it at the begin session time.
	  */
	  for (i = 0, pagesize = DB_MIN_PAGESIZE; i < DB_MAX_CACHE; 
		i++, pagesize *= 2)
	  {
              Sc_main_cb->sc_pagesize[i] = pagesize;
              Sc_main_cb->sc_maxpage = pagesize;

	      /* Initialize all tuple size */
              dmc.dmc_op_type = DMC_BMPOOL_OP;
              dmc.dmc_flags_mask = DMC_TUPSIZE;
	      dmc_char[0].char_value = pagesize;
              dmc.dmc_char_array.data_address = (PTR) &dmc_char[0];
              dmc.dmc_char_array.data_out_size = sizeof(DMC_CHAR_ENTRY);
              status = dmf_call(DMC_SHOW, (PTR) &dmc);
              if (status)
              {
	        sc0e_0_put(dmc.error.err_code, 0);
	        break;
	      }
              Sc_main_cb->sc_tupsize[i] = dmc_char[0].char_value;
              Sc_main_cb->sc_maxtup = dmc_char[0].char_value;
	  }
	}
	/*
	** STAR only needs DMF for sort cost routines.
	** Turn off it's interrupt handlers.
	*/
	if (Sc_main_cb->sc_capabilities & SC_STAR_SERVER)	
	{
	    SCF_CB   tmp_scb;
	    tmp_scb.scf_type     = SCF_CB_TYPE;
	    tmp_scb.scf_length   = sizeof(tmp_scb);
	    tmp_scb.scf_facility = DB_DMF_ID;
	    tmp_scb.scf_session  = DB_NOSESSION;
	    tmp_scb.scf_nbr_union.scf_amask = SCS_AIC_MASK | SCS_PAINE_MASK;
	    status = scf_call(SCS_CLEAR, &tmp_scb);
	    if (status)
	    {
		sc0e_0_put(tmp_scb.scf_error.err_code, 0);
		break;
	    }
	}	

	Sc_main_cb->sc_facilities |= 1 << DB_DMF_ID;

        MEmove(
            sizeof( Sc_main_cb->sc_ii_vnode ),
            Sc_main_cb->sc_ii_vnode,
            ' ',
	    sizeof(node),
	    node);
	if (cluster = CXcluster_enabled())
	{
	    CXnode_name(node);
	}

	MEmove(sizeof(node), node, ' ',
	       sizeof(Sc_main_cb->sc_cname), Sc_main_cb->sc_cname);
					/* Store Cluster node name */
	ule_initiate(node,
			 STlength(node),
			 Sc_main_cb->sc_sname,
			 sizeof(Sc_main_cb->sc_sname));

	/*
	**	    Why don't we call sca_check in a Star server?
	**
	** sca_check verifies that the ADF User Defined Datatypes with which
	** we are linked match the data types which the installation was
	** started with (i.e., the version with which the RCP was linked).
	**
	** The version with which the RCP was linked was returned by DMF as
	** a side-effect of calling dmc_start_server. However, in a Star
	** server, the full DMF startup is not performed (only the bare minimum
	** startup which supports dmt_sort_cost calls was performed). Therefore,
	** dmc_start_server did *not* return the RCP UDT version numbers, and
	** if we call sca_check here we will get a message from sca_check to the
	** error log indicating that the wrong UDT libraries may be in use.
	**
	** Therefore, in a star server we bypass this sca_check call. This is
	** unfortunate, because it removes a bit of verification that might
	** catch an accidental mismatch of UDT libraries. But the alternative
	** is to perform a full DMF server startup, which is expensive. And, we
	** assert the window of vulnerability is in fact small, since the star
	** server is actually the same executable program as is the local server
	** and the RCP, so it is unlikely that the star server would ever be
	** running with different UDT libraries than the local server (the
	** only example we could envision was where the user had linked new
	** UDT libraries while the installation was running, and then after
	** linking the new libraries the user restarted the star server but left
	** the RCP and all local servers up). Thus we judged the skipping of
	** this test to be an acceptable risk in a star server.
	**
	** Bryan Pendleton, Sep, 1993.
	*/

	if ((Sc_main_cb->sc_capabilities & SC_STAR_SERVER) == 0)
	{
	    if (!sca_check(dca[0].char_value, dca[1].char_value))
	    {
		sc0e_put(E_SC0269_SCA_INCOMPATIBLE, 0, 2,
			 sizeof(dca[0].char_value),  (PTR)&dca[0].char_value,
			 sizeof(dca[1].char_value),  (PTR)&dca[1].char_value,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0 );
		status = E_DB_FATAL;
		break;
	    }
	}
#endif /* FOR_EXAMPLE_ONLY */ 

#ifdef FOR_EXAMPLE_ONLY
	Sc_main_cb->sc_capabilities |= SC_RESRC_CNTRL;

	/*
	** Don't startup SXF in STAR servers currently
	*/
	if (!(Sc_main_cb->sc_capabilities & SC_STAR_SERVER))
	{
	    /*
	    ** Now initialize SXF. 
	    **
	    ** NOTE: SXF must be initialized after DMF because it needs 
	    ** to make calls to DMF during its initialization phase.
	    */
	    sxf_rcb.sxf_length = sizeof (sxf_rcb);
	    sxf_rcb.sxf_cb_type = SXFRCB_CB;
	    sxf_rcb.sxf_status = 0L;
	    if (Sc_main_cb->sc_capabilities & SC_STAR_SERVER)
		sxf_rcb.sxf_status |= SXF_STAR_SERVER;
	    if (scd_cb.c2_mode)
		sxf_rcb.sxf_status |= SXF_C2_SERVER;
	    sxf_rcb.sxf_mode = SXF_MULTI_USER;
	    sxf_rcb.sxf_max_active_ses = user_threads + 1;
	    sxf_rcb.sxf_dmf_cptr = dmf_call;

	    status = sxf_call(SXC_STARTUP, &sxf_rcb);
	    if (status != E_DB_OK)
	    {
		sc0e_put(sxf_rcb.sxf_error.err_code, 0, 0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0 );
		break;
	    }
	    Sc_main_cb->sc_sxvcb = sxf_rcb.sxf_server;
	    Sc_main_cb->sc_sxbytes = sxf_rcb.sxf_scb_sz;
	    Sc_main_cb->sc_facilities |= 1 << DB_SXF_ID;
	    Sc_main_cb->sc_sxf_cptr = sxf_call;
	}

	if (Sc_main_cb->sc_capabilities & SC_STAR_SERVER)
	{
	    rdf_ccb.rdf_distrib = DB_2_DISTRIB_SVR;
	    if (rdf_ccb.rdf_maxddb == 0)
	    	rdf_ccb.rdf_maxddb = user_threads;
	}
	else
	    rdf_ccb.rdf_distrib = DB_1_LOCAL_SVR;

	status = rdf_call(RDF_STARTUP, (PTR) &rdf_ccb);
	if (status)
	{
	    sc0e_0_put(rdf_ccb.rdf_error.err_code, 0);
	    break;
	}
	Sc_main_cb->sc_rdvcb = rdf_ccb.rdf_server;
	Sc_main_cb->sc_rdbytes = rdf_ccb.rdf_sesize;
	Sc_main_cb->sc_facilities |= 1 << DB_RDF_ID;

	qef_cb.qef_qpmax = num_qef_sessions * Sc_main_cb->sc_acc;
	qef_cb.qef_dsh_maxmem = 2048 + (qef_cb.qef_qpmax * scd_cb.qef_data_size);
        qef_cb.qef_sort_maxmem = num_qef_sessions * opf_cb.opf_qefsort;
	qef_cb.qef_sess_max = num_qef_sessions;
	qef_cb.qef_maxtup = Sc_main_cb->sc_maxtup;
	if (Sc_main_cb->sc_capabilities & SC_STAR_SERVER)
	    qef_cb.qef_s1_distrib = DB_2_DISTRIB_SVR;
	else
	    qef_cb.qef_s1_distrib = DB_1_LOCAL_SVR;
	if (scd_cb.c2_mode)
	    qef_cb.qef_flag_mask |= QEF_F_C2SECURE;
	qef_cb.qef_qescb_offset = CL_OFFSETOF(SCD_SCB, scb_sscb) +
		CL_OFFSETOF(SCS_SSCB, sscb_qescb);
	status = qef_call(QEC_INITIALIZE, (PTR) &qef_cb);
	if (status)
	{
	    sc0e_0_put(qef_cb.error.err_code, 0);
	    break;
	}
	Sc_main_cb->sc_qebytes = qef_cb.qef_ssize;
	Sc_main_cb->sc_qevcb = qef_cb.qef_server;
	Sc_main_cb->sc_facilities |= 1 << DB_QEF_ID;

	psq_cb.psq_mxsess = num_psf_sessions;
        if (opf_cb.opf_mxsess == 0)
            opf_cb.opf_mxsess = (num_opf_sessions + 1)/2; /* this
                                        ** value is necessary in
                                        ** order to determine value
                                        ** for PSQ_NOTIMEOUTERROR */
        if (opf_cb.opf_mxsess < num_opf_sessions)
            psq_cb.psq_flag |= PSQ_NOTIMEOUTERROR; /* PSF should
                                        ** not allow timeout=0 on
                                        ** certain tables */

	if (scd_cb.c2_mode)
	    psq_cb.psq_version |= PSQ_C_C2SECURE;

        /* If psf.memory specified, pass it to parser startup */
        if (Sc_main_cb->sc_psf_mem)
            psq_cb.psq_mserver = Sc_main_cb->sc_psf_mem;

	status = psq_call(PSQ_STARTUP, &psq_cb, (PTR) 0);
	if (status)
	{
	    sc0e_0_put(psq_cb.psq_error.err_code, 0);
	    break;
	}
	Sc_main_cb->sc_psvcb = psq_cb.psq_server;
	Sc_main_cb->sc_psbytes = psq_cb.psq_sesize;
	Sc_main_cb->sc_facilities |= 1 << DB_PSF_ID;

	opf_cb.opf_actsess = num_opf_sessions;
	opf_cb.opf_maxpage = Sc_main_cb->sc_maxpage;
	opf_cb.opf_maxtup = Sc_main_cb->sc_maxtup;
	for (i = 0; i < DB_MAX_CACHE; i++)
	{
	  opf_cb.opf_pagesize[i] = Sc_main_cb->sc_pagesize[i];
	  opf_cb.opf_tupsize[i] = Sc_main_cb->sc_tupsize[i];
	}
	if (Sc_main_cb->sc_capabilities & SC_STAR_SERVER)
	{
	    opf_cb.opf_smask = 0;
	    opf_cb.opf_ddbsite = (PTR)&Sc_main_cb->sc_ii_vnode[0];
	}

	status = opf_call(OPF_STARTUP, &opf_cb);
	if (status)
	{
	    sc0e_0_put(opf_cb.opf_errorblock.err_code, 0);
	    break;
	}
	Sc_main_cb->sc_opvcb = opf_cb.opf_server;
	Sc_main_cb->sc_opbytes = opf_cb.opf_sesize;
	Sc_main_cb->sc_facilities |= 1 << DB_OPF_ID;

	/* QSF */
	qsf_rcb.qsf_type = QSFRB_CB;
	qsf_rcb.qsf_ascii_id = QSFRB_ASCII_ID;
	qsf_rcb.qsf_length = sizeof(qsf_rcb);
	qsf_rcb.qsf_owner = (PTR) DB_SCF_ID;
	qsf_rcb.qsf_server = 0;
	qsf_rcb.qsf_max_active_ses = num_qsf_sessions;
	qsf_rcb.qsf_qsscb_offset = CL_OFFSETOF(SCD_SCB, scb_sscb) +
		CL_OFFSETOF(SCS_SSCB, sscb_qsscb);
	status = qsf_call(QSR_STARTUP, &qsf_rcb);
	if (status)
	{
	    sc0e_0_put(qsf_rcb.qsf_error.err_code, 0);
	    break;
	}
	Sc_main_cb->sc_qsvcb = qsf_rcb.qsf_server;
	Sc_main_cb->sc_qsbytes = qsf_rcb.qsf_scb_sz;
	Sc_main_cb->sc_facilities |= 1 << DB_QSF_ID;


	/*
	** User-added sessions will stall in scs_initiate() as long as 
	** sc_state == SC_INIT, so there's no need for sc_urdy_semaphore.
	*/

# if 0
	/* This has never been used (daveb) */
	
	/* Now set up to start the system initialization thread.	    */

	local_crb.gca_status = 0;
	local_crb.gca_assoc_id = 0;
	local_crb.gca_size_advise = 0;
	local_crb.gca_user_name = DB_SYSINIT_THREAD;
	local_crb.gca_account_name = 0;
	local_crb.gca_access_point_identifier = "NONE";
	local_crb.gca_application_id = 0;

	/*
	** Start init commit thread at higher priority than user sessions.
	*/
	priority = Sc_main_cb->sc_max_usr_priority + 3;
	if (priority > Sc_main_cb->sc_max_priority)
	    priority = Sc_main_cb->sc_max_priority;

	stat = CSadd_thread(priority, (PTR) &local_crb, 
          			    SCS_SERVER_INIT, (CS_SID*)NULL, &sys_err);
	if (stat != OK)
	{
	    sc0e_0_put(stat, 0);
	    sc0e_0_put(E_SC0245_SERVER_INIT_ADD, 0);
	    status = E_DB_ERROR;
	    break;
	}
# endif

	if (!(Sc_main_cb->sc_capabilities & SC_STAR_SERVER))
	{
	    status = start_lglk_special_threads(
		    ( Sc_main_cb->sc_capabilities & SC_RECOVERY_SERVER ) != 0,
		    scd_cb.cp_timeout,
		    scd_cb.num_logwriters,
		    scd_cb.start_gc_thread,
		    Sc_main_cb->sc_dmcm);
	    if (status)
		break;
	}

	/*
	** Set up the connect request block (crb) to add the DMF Consistency
	** Point Thread.  This thread acts as a write behind process which
	** flushes dirty pages out of the buffer cache.  This is a dummy
	** crb since there is really no GCF connection to this thread.
	**
	** All servers must have a Consistency Point Thread.
	** (Note: Previous to 6.4 this was called the Fast Commit thread
	** and was started only in servers which used Fast Commit protocols)
	*/
	if (!(Sc_main_cb->sc_capabilities & SC_STAR_SERVER))
	{
	    local_crb.gca_status = 0;
	    local_crb.gca_assoc_id = 0;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = DB_BCP_THREAD;
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;

	    /*
	    ** Start thread at higher priority than user sessions.
	    */
	    priority = Sc_main_cb->sc_max_usr_priority + 3;
	    if (priority > Sc_main_cb->sc_max_priority)
		priority = Sc_main_cb->sc_max_priority;

	    stat = CSadd_thread(priority, (PTR)&local_crb,
				SCS_SFAST_COMMIT, (CS_SID*)NULL, &sys_err);
	    if (stat != OK)
	    {
		sc0e_0_put(stat, 0);
		sc0e_0_put(E_SC0237_FAST_COMMIT_ADD, 0);
		status = E_DB_ERROR;
		break;
	    }
	}

	/* FIXME:  ICE should have event notification */
	/*  However, that uses LKevent and that needs LGK which is too much */

	/* Create a thread to handle event notifications */
	if (Sc_main_cb->sc_events > 0)
	{
	    /*
	    ** Initialize event memory in-line.  Currently errors & warnings
	    ** are ignored and set to OK, because the event thread is not
	    ** a vital task.  The error has already been logged but it would
	    ** be better to indicate the server started up with warnings.
	    */
	    status = sce_initialize();
	    if (status != E_DB_OK)
	    {
		if (status == E_DB_FATAL)
		    break;
		status = E_DB_OK;	/* Somehow set E_DB_WARNING ?? */
	    }
	    else
	    {

		local_crb.gca_status = 0;
		local_crb.gca_assoc_id = 0;
		local_crb.gca_size_advise = 0;
		local_crb.gca_user_name = DB_EVENT_THREAD;
		local_crb.gca_account_name = 0;
		local_crb.gca_access_point_identifier = "NONE";
		local_crb.gca_application_id = 0;
		/*
		** start at same priority as normal threads ( we used
		** to start higher but this caused problems )
		*/
		priority = Sc_main_cb->sc_norm_priority;
		if (priority > Sc_main_cb->sc_max_priority)
		    priority = Sc_main_cb->sc_max_priority;
		stat = CSadd_thread(priority, (PTR)&local_crb,
				    SCS_SEVENT, (CS_SID*)NULL, &sys_err);
		/* Check for error adding thread */
		if (stat != OK)
		{
		    sc0e_0_put(stat, 0);
		    sc0e_0_put(E_SC0272_EVENT_THREAD_ADD, &sys_err);
		    status = E_DB_OK;	/* Somehow set E_DB_WARNING ?? */
		}
	    }
	}

	/*
	** If server is configured to run write behind threads, then
	** add those threads.  Set up a dummy connect request block
	** to call CSadd_thread.
	*/
	if (Sc_main_cb->sc_writebehind)
	{
	    local_crb.gca_status = 0;
	    local_crb.gca_assoc_id = 0;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = DB_BWRITBEHIND_THREAD;
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;

	    /*
	    ** Start write behind threads at higher priority than user sessions.
	    */
	    priority = Sc_main_cb->sc_max_usr_priority + 2;
	    if (priority > Sc_main_cb->sc_max_priority)
		priority = Sc_main_cb->sc_max_priority;

	    stat = OK;
	    for (i = 0; i < Sc_main_cb->sc_writebehind; i++)
	    {
		stat = CSadd_thread(priority, (PTR)&local_crb,
				    SCS_SWRITE_BEHIND, (CS_SID*)NULL, &sys_err);
		if (stat != OK)
		    break;
	    }

	    /* Check for error adding thread */
	    if (stat != OK)
	    {
		sc0e_0_put(stat, 0);
		sc0e_0_put(E_SC0243_WRITE_BEHIND_ADD, 0);
		status = E_DB_ERROR;
		break;
	    }
	}
	/*
	** If server is configured to run replicator queue management threads, 
	** then add those threads.  Set up a dummy connect request block
	** to call CSadd_thread.
	*/
	if (Sc_main_cb->sc_rep_qman)
	{
	    local_crb.gca_status = 0;
	    local_crb.gca_assoc_id = 0;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = DB_REP_QMAN_THREAD;
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;

	    /*
	    ** Start write behind threads at same priority as user sessions.
	    */
	    priority = Sc_main_cb->sc_max_usr_priority;

	    stat = OK;
	    for (i = 0; i < Sc_main_cb->sc_rep_qman; i++)
	    {
		stat = CSadd_thread(priority, (PTR)&local_crb,
				    SCS_REP_QMAN, (CS_SID*)NULL, &sys_err);
		if (stat != OK)
		    break;
	    }

	    /* Check for error adding thread */
	    if (stat != OK)
	    {
		sc0e_0_put(stat, 0);
		sc0e_0_put(E_SC037D_REP_QMAN_ADD, 0);
		status = E_DB_ERROR;
		break;
	    }
	}
	/*
	**  STAR needs to start a thread to clean-up all orphan DX's,
	**  if the server stared with the option /nostarrecovery do not
	**  start the thread.
	*/
	if ((Sc_main_cb->sc_capabilities & SC_STAR_SERVER) &&
	    (Sc_main_cb->sc_nostar_recovr == FALSE))
	{
	    local_crb.gca_status = 0;
	    local_crb.gca_aux_data = (PTR)NULL;
	    local_crb.gca_assoc_id = -1;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = DB_DXRECOVERY_THREAD;
	    local_crb.gca_password  = (char *)NULL;
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;
	    
	    /* Start recovery thread at higher priority than user sessions. */
	    priority = Sc_main_cb->sc_max_usr_priority + 2;
	    if (priority > Sc_main_cb->sc_max_priority)
		priority = Sc_main_cb->sc_max_priority;

	    stat = OK;
	    stat = CSadd_thread(priority, (PTR) &local_crb, 
				SCS_S2PC, (CS_SID*)NULL, &sys_err);
	    if (stat != OK)
	    {
		sc0e_0_put(stat, 0);
		sc0e_0_put(E_SC0314_RECOVER_THREAD_ADD, 0);
		break;
	    }
	}

	/*
	** If this server is running with security auditing enabled
	** we need to start up the auditing thread now.
	*/
	if (scd_cb.c2_mode)
	{
	    local_crb.gca_status = 0;
	    local_crb.gca_assoc_id = 0;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = DB_AUDIT_THREAD;
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;

	    /*
	    ** Start audit thread at higher priority than user sessions.
	    */
	    priority = Sc_main_cb->sc_max_usr_priority + 1;
	    if (priority > Sc_main_cb->sc_max_priority)
		priority = Sc_main_cb->sc_max_priority;

	    stat = CSadd_thread(priority, (PTR) &local_crb, 
				SCS_SAUDIT, (CS_SID*)NULL, &sys_err);
	    if (stat != OK)
	    {
		sc0e_0_put(stat, 0);
		sc0e_0_put(E_SC0329_AUDIT_THREAD_ADD, 0);
		status = E_DB_ERROR;
		break;
	    }
	}
#endif /* FOR_EXAMPLE_ONLY */ 

	if(term_thread)
	{
	    local_crb.gca_status = 0;
	    local_crb.gca_assoc_id = 0;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = DB_TERM_THREAD;
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;

	    /*
	    ** Start terminator thread at higher priority than user sessions.
	    */
	    priority = Sc_main_cb->sc_max_usr_priority + 1;
	    if (priority > Sc_main_cb->sc_max_priority)
		priority = Sc_main_cb->sc_max_priority;

	    stat = CSadd_thread(priority, (PTR) &local_crb, 
				SCS_SCHECK_TERM, (CS_SID*)NULL, &sys_err);
	    if (stat != OK)
	    {
		sc0e_0_put(stat, 0);
		sc0e_0_put(E_SC0340_TERM_THREAD_ADD, 0);
		status = E_DB_ERROR;
		break;
	    }
	}

        if (cleanup_thread)
        {
	    local_crb.gca_status = 0;
	    local_crb.gca_assoc_id = 0;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = ICE_CLEANUPTHREAD_NAME;
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;

	    /*
	    ** Start licensing thread at lower priority than user sessions.
	    */
	    priority = Sc_main_cb->sc_max_usr_priority - 2;
	    if (priority < Sc_main_cb->sc_min_priority)
		priority = Sc_main_cb->sc_min_priority;

	    stat = CSadd_thread(priority, (PTR) &local_crb, 
				SCS_PERIODIC_CLEAN, (CS_SID*)NULL, &sys_err);
	    if (stat != OK)
	    {
		sc0e_0_put(stat, 0);
		sc0e_0_put(E_SC0384_CLEANUP_THREAD_ADD, 0);
		status = E_DB_ERROR;
		break;
	    }
        }
        if (logtrc_thread)
        {
	    local_crb.gca_status = 0;
	    local_crb.gca_assoc_id = 0;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = ICE_LOGTRCTHREAD_NAME;
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;

	    /*
	    ** Start licensing thread at lower priority than user sessions.
	    */
	    priority = Sc_main_cb->sc_max_usr_priority - 2;
	    if (priority < Sc_main_cb->sc_min_priority)
	    	priority = Sc_main_cb->sc_min_priority;

            stat = CSadd_thread( priority, (PTR) &local_crb,
                SCS_TRACE_LOG, (CS_SID*)NULL, &sys_err );

	    if (stat != OK)
	    {
		sc0e_0_put(stat, 0);
		sc0e_0_put(E_SC0386_LOGTRC_THREAD_ADD, 0);
		status = E_DB_ERROR;
		break;
	    }
        }
#ifdef FOR_EXAMPLE_ONLY
	if(secaudit_writer)
	{
	    local_crb.gca_status = 0;
	    local_crb.gca_assoc_id = 0;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = DB_SECAUD_WRITER_THREAD;
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;

	    /*
	    ** Start audit writer thread at user priority.
	    */
	    priority = Sc_main_cb->sc_max_usr_priority;

	    stat = CSadd_thread(priority, (PTR) &local_crb, 
				SCS_SSECAUD_WRITER, (CS_SID*)NULL, &sys_err);
	    if (stat != OK)
	    {
		sc0e_0_put(stat, 0);
		sc0e_0_put(E_SC0360_SEC_WRITER_THREAD_ADD, 0);
		status = E_DB_ERROR;
		break;
	    }
	}

	/*
	** Add write-along  threads. Set up a dummy connect request block
	** to call CSadd_thread.
	*/
	if (Sc_main_cb->sc_writealong) 
	{
	    local_crb.gca_status = 0;
	    local_crb.gca_assoc_id = 0;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = DB_BWRITALONG_THREAD;   
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;

	    /*
	    ** Start write along threads at higher priority than user sessions.
	    */
	    priority = Sc_main_cb->sc_max_usr_priority + 2;
	    if (priority > Sc_main_cb->sc_max_priority)
		priority = Sc_main_cb->sc_max_priority;

	    stat = OK;
	    for (i = 0; i < Sc_main_cb->sc_writealong; i++)
	    {
		stat = CSadd_thread(priority, (PTR)&local_crb,
				    SCS_SWRITE_ALONG, (CS_SID*)NULL, &sys_err);
		if (stat != OK)
		    break;
	    }

	    /* Check for error adding thread */
	    if (stat != OK)
	    {
		sc0e_0_put(stat, 0);
		sc0e_0_put(E_SC0367_WRITE_ALONG_ADD, 0);
		status = E_DB_ERROR;
		break;
	    }
	}

	/*
	** Add read-ahead   threads. Set up a dummy connect request block
	** to call CSadd_thread.
	*/
	if (Sc_main_cb->sc_readahead)  
	{
	    local_crb.gca_status = 0;
	    local_crb.gca_assoc_id = 0;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = DB_BREADAHEAD_THREAD;   
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;

	    /* If IOMASTER use hi pri for readahead, or it may be too,late */
	    /* For regular server, less than user pri, since its better to */
	    /* do what user needs, before doing what we think user needs   */
	    if (Sc_server_type == SC_IOMASTER_SERVER)
	    {
		priority = Sc_main_cb->sc_max_usr_priority + 4;
		if (priority > Sc_main_cb->sc_max_priority)
		    priority = Sc_main_cb->sc_max_priority;
	    }
	    else
		priority = Sc_main_cb->sc_min_usr_priority;

	    stat = OK;
	    for (i = 0; i < Sc_main_cb->sc_readahead; i++)
	    {
		stat = CSadd_thread(priority, (PTR)&local_crb,
				    SCS_SREAD_AHEAD, (CS_SID*)NULL, &sys_err);
		if (stat != OK)
		    break;
	    }

	    /* Check for error adding thread */
	    if (stat != OK)
	    {
		sc0e_0_put(stat, 0);
		sc0e_0_put(E_SC0368_READ_AHEAD_ADD, 0);
		status = E_DB_ERROR;
		break;
	    }
	}

#endif /* FOR_EXAMPLE_ONLY */ 

	break;
    }

#ifdef xDEBUG
    sc0m_check(SC0M_NOSMPH_MASK | SC0M_READONLY_MASK | SC0M_WRITEABLE_MASK, "");
#endif
    if (status)
    {
	/* More detail message logged above */
	return(E_SC0124_SERVER_INITIATE);
    }
    else
    {
	/*
	** Obtain and fix up the configuration name, then log it.
	*/
	char	server_flavor[256];
	char	*tmpptr, *tmpbuf = PMgetDefault(3);
	char	server_type[32];

	tmpptr = (char *)&server_flavor;
	while (*tmpbuf != '\0')
	{
	    if (*tmpbuf != '\\')
		*(tmpptr++) = *tmpbuf;

	    tmpbuf++;
	}
	*tmpptr = '\0';

	STcopy(PMgetDefault(2), server_type);
	CVupper(server_type);

	Sc_main_cb->sc_state = SC_OPERATIONAL;
	sc0e_put(E_SC051D_LOAD_CONFIG, 0, 2,
		 STlength(server_flavor), server_flavor,
		 STlength(server_type), server_type,
		 0, (PTR)0, 
		 0, (PTR)0, 
		 0, (PTR)0, 
		 0, (PTR)0 );

	sc0e_put(E_SC0129_SERVER_UP, 0, 2,
		 STlength(Version), (PTR)Version,
		 STlength(SystemProductName), SystemProductName, 
		 0, (PTR)0, 
		 0, (PTR)0, 
		 0, (PTR)0, 
		 0, (PTR)0 );
	return(E_DB_OK);
    }
}

/*{
** Name: ascd_terminate	- Shut down the Ingres DBMS Server
**
** Description:
**      This routine shuts down the dbms server.  It assumes that all
**      sessions have been closed, and will fail if any client facilities 
**      fail to shut down. 
** 
**      The facilities are shut down in the reverse order of startup,
**	except for GCF, which is shut down last.
**
** Inputs:
**      None
**
** Outputs:
**      None
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-May-1998 (shero03)
**	    Terminate the API / Connection Mgr
**  11-Feb-2005 (fanra01)
**      Removed reference to urs facility.
**  01-May-2008 (smeke01) b118780
**      Corrected name of sc_avgrows to sc_totrows.
[@history_template@]...
*/
DB_STATUS
ascd_terminate(void)
{
    DB_STATUS           status;
    i4 			error = 0;
    i4 			gca_error;
    i4 			i;
    GCA_TE_PARMS	gca_cb;

    if (Sc_main_cb == 0 || Sc_main_cb->sc_state == SC_UNINIT)
	return(OK);

    /* Mark shutdown flag */
    Sc_main_cb->sc_state = SC_SHUTDOWN;

    if (Sc_main_cb->sc_facilities & (1 << DB_ICE_ID))
		{
			DB_ERROR		err;
			status = WTSTerminate(&err);
			if (status)
			{
					sc0e_0_put(err.err_code, 0);
					error++;
			}
			Sc_main_cb->sc_facilities &= ~(1 << DB_ICE_ID);
		}

    if (Sc_main_cb->sc_facilities & (1 << DB_ADF_ID))
    {
	status = adg_shutdown();
	if (status)
	{
	    sc0e_0_put(E_SC012C_ADF_ERROR, 0);
	    error++;
	}
	Sc_main_cb->sc_facilities &= ~(1 << DB_ADF_ID);
    }

    status = sce_shutdown();
    if (status)		/* Error already logged */
	error++;

	/* Terminate the Connection Manager */
	{
	  II_PTR		lerr;
	  IIAPI_TERMPARM termParm;

      IIcm_Terminate((PTR)&lerr);

	  termParm.tm_status = IIAPI_ST_WARNING;  /* set up for loop */
	  /*
	  ** This loop is necessary because API keeps a 
	  ** use count for api_initialize calls and
	  ** terminate must be called for each one
	  ** Note that the return code is IIAPI_ST_NOT_INITIALIZED
	  ** if we go too far.
	  */
	  while (termParm.tm_status == IIAPI_ST_WARNING)
	    IIapi_terminate(&termParm); 
	}

    if (Sc_main_cb->sc_facilities & (1 << DB_GCF_ID))
    {
        status = IIGCa_call(GCA_TERMINATE,
                            (GCA_PARMLIST *)&gca_cb,
                            GCA_SYNC_FLAG,
                            (PTR)0,
                            -1,
                            &gca_error);
        if (status)
        {
            sc0e_0_put(gca_error, 0);
            error++;
        }
        else if (gca_cb.gca_status != E_GC0000_OK)
        {
            sc0e_0_put(gca_cb.gca_status, &gca_cb.gca_os_status);
            error++;
        }
        Sc_main_cb->sc_facilities &= ~(1 << DB_GCF_ID);
    }

    /* Dump session statistics to the log */
    TRdisplay("\n%29*- SCF Session Statistics %28*-\n\n");
    TRdisplay("    %32s  Current  Created      Hwm\n", "Session Type");
    for (i = 0; i <= SCS_MAXSESSTYPE; i++)
    {
	if (Sc_main_cb->sc_session_count[i].created)
	{
	    TRdisplay("    %32w %8d %8d %8d\n",
"User,Monitor,Fast Commit,Write Behind,Server Init,Event,\
Two Phase Commit,Recovery,Log Writer,Check Dead,Group Commit,\
Force Abort,Audit,CP Timer,Check Term,Security Audit Writer,\
Write Along,Read Ahead,Replicator Queue Manager,Lock Callback,\
Deadlock Detector,Sampler,Sort,Factotum", i,
		Sc_main_cb->sc_session_count[i].current,
		Sc_main_cb->sc_session_count[i].created,
		Sc_main_cb->sc_session_count[i].hwm);
	}
    }
    TRdisplay("\n%80*-\n");

    Sc_main_cb->sc_state = SC_UNINIT;
    {
	i4     		avg;

	avg = (Sc_main_cb->sc_selcnt
		    ? Sc_main_cb->sc_totrows / Sc_main_cb->sc_selcnt
		    : 0);
	sc0e_put(E_SC0235_AVGROWS, 0, 2,
		 sizeof(Sc_main_cb->sc_selcnt), (PTR) &Sc_main_cb->sc_selcnt,
		 sizeof(avg), (PTR) &avg,
		 0, (PTR)0, 
		 0, (PTR)0, 
		 0, (PTR)0, 
		 0, (PTR)0 );
    }
    if (error)
    {
	return(E_SC0127_SERVER_TERMINATE);
    }
    else
    {
        sc0e_0_put(E_SC0128_SERVER_DOWN, 0);
	return(E_DB_OK);
    }
}

/*{
** Name: ascd_dbinfo_fcn	- Function to provide adf dbms_info things
**
** Description:
**      This routine provides dbms information as a response to  
**      a dbms_info() call on the part of a user (via ADF). 
**      It behaves as documented in the ADF specification.
**
** Inputs:
**      dbi                             Ptr to ADF_DBMSINFO block describing
**                                      data desired.
**      dvi                             Input data value (null in all cases)
**      dvr                             Data value for result.
**      error                           Ptr to DB_ERROR struct.
**
** Outputs:
**      *dvr                            Filled in with answer
**	Returns:
**	    STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-Oct-1997 (shero03)
**          Created from scdinit.
[@history_template@]...
*/
DB_STATUS
ascd_dbinfo_fcn(ADF_DBMSINFO *dbi,
	       DB_DATA_VALUE *dvi,
	       DB_DATA_VALUE *dvr,
	       DB_ERROR *error )
{
    DB_STATUS           status;
    SCD_SCB		*scb;
    union
    {
	SCF_CB              scf_cb;
	QEF_RCB		    qef_cb;
    }	cb;
    SCF_CB	*scf_cb;
    SCF_SCI	scf_sci;
    char	erlangname[ER_MAX_LANGSTR];

    status = E_DB_OK;
    error->err_code = E_DB_OK;

    CSget_scb((CS_SCB **)&scb);
    switch (dbi->dbi_reqcode)
    {

	case SC_ET_SECONDS:
	    *((i4 *) dvr->db_data) = TMsecs() - scb->cs_scb.cs_connect;
	    status = 0;
	    error->err_code = 0;
	    break;

	case SC_LANGUAGE:
	    status = ERlangstr(scb->scb_sscb.sscb_ics.ics_language,
		erlangname);
	    if (status == OK)
	    {
		MEmove(STnlength(ER_MAX_LANGSTR, erlangname), erlangname,
		    ' ', dvr->db_length, dvr->db_data);
	    }
	    else
	    {
		MEmove(9, "<unknown>", ' ', dvr->db_length, dvr->db_data);
	    }
	    break;

	case SC_COLLATION:
	    MEmove(STnlength(sizeof scb->scb_sscb.sscb_ics.ics_collation,
		    scb->scb_sscb.sscb_ics.ics_collation),
		scb->scb_sscb.sscb_ics.ics_collation, ' ',
		dvr->db_length, dvr->db_data);
	    status = 0;
	    error->err_code = 0;
	    break;

	case SC_QLANG:
	    if (scb->scb_sscb.sscb_ics.ics_qlang == DB_SQL)
	    {
		MEmove(3, "sql", ' ', dvr->db_length, dvr->db_data);
	    }
	    else if (scb->scb_sscb.sscb_ics.ics_qlang == DB_QUEL)
	    {
		MEmove(4, "quel", ' ', dvr->db_length, dvr->db_data);
	    }
	    else if (scb->scb_sscb.sscb_ics.ics_qlang == DB_NDMF)
	    {
		MEmove(4, "ndmf", ' ', dvr->db_length, dvr->db_data);
	    }
	    else
	    {
		MEmove(9, "<unknown>", ' ', dvr->db_length, dvr->db_data);
	    }
	    break;

	case SC_SERVER_CLASS:
            if (Sc_main_cb->sc_capabilities & SC_RMS_SERVER)
                MEmove(18, "RMS Gateway Server", ' ', dvr->db_length,
                    dvr->db_data);
            else if (Sc_main_cb->sc_capabilities & SC_STAR_SERVER)
                MEmove(18, "Ingres Star Server", ' ', dvr->db_length,
                    dvr->db_data);
            else if (Sc_main_cb->sc_capabilities & SC_IOMASTER_SERVER)
                MEmove(18, "I/O Master Server ", ' ', dvr->db_length,
                    dvr->db_data);
            else
                MEmove(18, "Ingres DBMS Server", ' ', dvr->db_length,
                    dvr->db_data);
	    break;


	case SCI_IVERSION:
	    MEmove(	sizeof( Sc_main_cb->sc_iversion ),
			Sc_main_cb->sc_iversion,
			' ',
			dvr->db_length,
			dvr->db_data);
	    break;

	case SC_MAJOR_LEVEL:
	    MEcopy((PTR) &Sc_main_cb->sc_major_adf,
			    (u_i2) dvr->db_length, dvr->db_data);
	    break;

	case SC_MINOR_LEVEL:
	    MEcopy((PTR) &Sc_main_cb->sc_minor_adf,
			    (u_i2) dvr->db_length, dvr->db_data);
	    break;

	case SC_CLUSTER_NODE:
	    MEmove(sizeof(Sc_main_cb->sc_cname),
			Sc_main_cb->sc_cname,
			' ',
			dvr->db_length,
			dvr->db_data);
	    break;
	    
	default:
	    scf_cb = &cb.scf_cb;
	    scf_cb->scf_length = sizeof(SCF_CB);
	    scf_cb->scf_type = SCF_CB_TYPE;
	    scf_cb->scf_owner = (PTR) DB_SCF_ID;
	    scf_cb->scf_ascii_id = SCF_ASCII_ID;
	    scf_cb->scf_facility = DB_SCF_ID;
	    scf_cb->scf_session = scb->cs_scb.cs_self;
	    scf_cb->scf_len_union.scf_ilength = 1;
	    scf_cb->scf_ptr_union.scf_sci = (SCI_LIST *) &scf_sci;
	    scf_sci.sci_length = dvr->db_length;
	    scf_sci.sci_code = dbi->dbi_reqcode;
	    scf_sci.sci_aresult = dvr->db_data;
	    scf_sci.sci_rlength = &dvr->db_length;
	    status = scf_call(SCU_INFORMATION, scf_cb);
	    STRUCT_ASSIGN_MACRO(scf_cb->scf_error, *error);
	    break;
    }

    return(status);
}

/*{
** Name: ascd_adf_printf	- Function to print trace message for ADF.
**
** Description:
**      This routine takes an input string and print it using SCC_TRACE. 
**      
** Inputs:
**      cbuf                            NULL terminated input string to
**                                      print.
** Outputs:
**      none
**
**	Returns:
**	    STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
*/

DB_STATUS
ascd_adf_printf( char *cbuf )
{
    SCF_CB     scf_cb;
    DB_STATUS  scf_status;

    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_ADF_ID;
    scf_cb.scf_nbr_union.scf_local_error = 0;
    scf_cb.scf_len_union.scf_blength = STnlength(STlength(cbuf), cbuf);
    scf_cb.scf_ptr_union.scf_buffer = cbuf;
    scf_cb.scf_session = DB_NOSESSION;
    scf_status = scf_call(SCC_TRACE, &scf_cb);

    if (scf_status != E_DB_OK)
    {
	TRdisplay("SCF error displaying an ADF message to user\n");
	TRdisplay("The ADF message is :%s",cbuf);
    }
    return( scf_status);
}

/*
** Name: ascd_init_sngluser - Init a single user server for SCF
**
** Description:
**      The rouitne initializes SCF for a single user server. Currently
**      all single user servers are run from DMF. With the addition of
**      the fastload function to the JSP, and the subseqnet support for
**      long datatypes, which require couponification, calls are made
**      to SCF from a single user server (for information gathering
**      rather than thread management). This routine initializes the SCF
**      control blocks that are required for these calls.
**
** Inputs:
**      scf_cb - SCF call control block
**
** Outputs:
**      None
**
** Returns:
**      status
**
** History:
**      30-jan-97 (stephenb)
**          Initial creation.
*/
DB_STATUS
ascd_init_sngluser(
        SCF_CB          *scf_cb)
{
    SCD_SCB     *scb;

    /* get SCB */
    CSget_scb((CS_SCB **)&scb);

    /* set DML_SCB */
    scb->scb_sscb.sscb_dmscb = scf_cb->scf_ptr_union.scf_dml_scb;

    return(E_DB_OK);
}
