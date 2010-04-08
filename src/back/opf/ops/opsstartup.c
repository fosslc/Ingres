/*
**Copyright (c) 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmccb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
/* beginning of optimizer header files */
#include    <opglobal.h>
#include    <opdstrib.h>
#include    <opfcb.h>
#include    <opgcb.h>
#include    <opscb.h>
#include    <ophisto.h>
#include    <opboolfact.h>
#include    <oplouter.h>
#include    <opeqclass.h>
#include    <opcotree.h>
#include    <opvariable.h>
#include    <opattr.h>
#include    <openum.h>
#include    <opagg.h>
#include    <opmflat.h>
#include    <opcsubqry.h>
#include    <opsubquery.h>
#include    <opcstate.h>
#include    <opstate.h>
/* external routine declarations definitions */
#define        OPS_STARTUP      TRUE
#include    <me.h>
#include    <cv.h>
#include    <mh.h>
#include    <st.h>
#include    <opxlint.h>

/**
**
**  Name: OPSSTARTUP.C - start up a server
**
**  Description:
**      Routine to startup a server for the OPF
**
**  History:    
**      30-jun-86 (seputis)    
**          initial creation
**	26-jun-91 (seputis)
**	    enable parameters for OPF startup flags
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	01-sep-93 (jhahn)
**	    Added opg_psession_id for maintaining server
**	    lifetime unique id's.
**	06-mar-96 (nanpr01)
**	    Added initialization to fill in the max tuple size, max page size,
**	    page sizes available and tuple sizes available of the 
**	    installation. 
**	29-jul-97 (popri01)
**	    opg activeuser count was never initialized (note comment about 
**	    MZERO mask). Resulting garbage value caused a CScnd wait hang.
**	31-aug-98 (sarjo01)
**	    Set ops_sestime (joinop timeout) from configurable
**	    opf_joinop_timeout rather than constant OPG_TIMSESMAX
**	15-sep-98 (sarjo01)
**	    Set new param ops_timeoutabort from opf_cb->opf_joinop_timeoutabort
**	09-Oct-1998 (jenjo02)
**	    Removed SCF semaphore functions, inlining the CS calls instead.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE for memory pool > 2Gig.
**	11-Nov-2009 (kiria01) SIR 121883
**	    Pick up cardinality_check setting.
**	03-Dec-2009 (kiria01) b122952
**	    Add .opf_inlist_thresh for control of eq keys on large inlists.
[@history_line@]...
**/

/*{
** Name: opg_getfunc	- init function OP id
**
** Description:
**      In order to place as much intelligent into ADF as possible
**	OPF will use a "textual" interface to the OPids of ADF 
**
** Inputs:
**      opf_cb                          ptr to OPF control block
**      func_name                       ptr to name of function to lookup
**      language                        language of function
**
** Outputs:
**      op_ptr                          location to return op id
**
**	Returns:
**	    DB_STATUS - E_DB_OK if successful, E_DB_FATAL otherwise
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      18-nov-89 (seputis)
**          initial creation
[@history_template@]...
*/
static DB_STATUS
opg_getfunc(
	OPF_CB             *opf_cb,
	char               *func_name,
	DB_LANG            language,
	ADI_OP_ID	   *op_ptr)
{
    DB_STATUS	    adfstatus;	/* return status from ADF */
    ADI_OP_NAME	    adi_oname;  /* name of an operation */
    ADF_CB          adf_scb;    /* temp session control block for 
				    ** ADF errors */

    /* FIXME: Ed: Please check that the following adf_scb is initialized
    ** correctly, esp considering that I filled in the qlang to QUEL. We
    ** need to consider SQL here also. (eric 17 Mar 87)
    */
    /* Initialize 'adf_scb' */
    MEfill(sizeof (adf_scb), (u_char)0, (PTR)&adf_scb);
    adf_scb.adf_qlang = language;
    /* FIXME: Ed: Please check that the above adf_scb is initialized
    ** correctly. (eric 17 Mar 87)
    */

    STcopy(func_name, &adi_oname.adi_opname[0]); /* get operator */
    adfstatus = adi_opid( &adf_scb, &adi_oname, op_ptr);
# ifdef E_OP078D_ADI_OPID
    if (DB_FAILURE_MACRO(adfstatus))
    {
	opx_rverror( opf_cb, adfstatus, E_OP078D_ADI_OPID, 
	    adf_scb.adf_errcb.ad_errcode);
	return( E_DB_FATAL );
    }
# endif
    return(E_DB_OK);
}


/*{
** Name: ops_startup	- the routine initializes the OPF server control block
**
** Description:
**      This routine is called once by the SCF to initialize the OPF server
**      control block.
**
** Inputs:
**      opf_cb                          ptr to control block used to call us
**
** Outputs:
**      opf_cb->opf_server		server control block
**          .opg_memory                 initialize scratch memory pool from ULM
**          .opg_rdfmemory              initialize scratch memory pool for RDF
**          .opg_sesmemory              set default session memory limit
**          .opg_sestime                set default session time limit
**          .opg_check                  reset trace flag check
**          .opg_tt                     set all global trace flags to zero
**	Returns:
**	    E_DB_OK, E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	27-mar-86 (seputis)
**          initial creation
**	20-oct-88 (seputis)
**          add distribute site info for server site
**	1-may-89 (seputis)
**          add support for count(*) operator
**	29-sep-89 (seputis)
**	    add support for server flattening flag
**	16-may-90 (sandyh)
**	    added opf memory check to make sure passed in value
**	    is at least enough to run in minumum state.
**	10-aug-90 (seputis)
**	    fixed uninitialized variable from 16-may-90 fix
**	20-dec-90 (seputis)
**	    add server startup parameters
**	4-jan-90 (seputis)
**	    removed memory allocation for RDF
**	16-jan-91 (seputis)
**	    add support for OPF ACTIVE flag
**	12-aug-91 (seputis)
**	    - add support for the LIKE operator, used to test
**	    in ALLMATCH case in opbcreate for CAFS
**      23-sep-91 (seputis)
**          - precalculate constant used to determine underflow
**      18-may-92 (seputis)
**          - bug 44107 - do not eliminate null join if ifnull operator
**          is used
**      8-sep-92 (ed)
**          remove OPF_DIST
**      16-oct-92 (ed)
**          - fix bug 40835 - wrong number of duplicates when union views and
**          subselects are used
**	01-sep-93 (jhahn)
**	    Added opg_psession_id for maintaining server
**	    lifetime unique id's.
**	06-mar-96 (nanpr01)
**	    Added initialization to fill in the max tuple size, max page size,
**	    page sizes available and tuple sizes available of the 
**	    installation. 
**	12-dec-96 (inkdo01)
**	    Added support for config.dat parm (opf_maxmemf) which defines
**	    proportion of OPF memory pool available to any single session. 
**	    This replaces the more rigid approach of dividing pool exactly
**	    according to max sessions.
**      14-mar-97 (inkdo01)
**          Added ops_spatkey to assign default selectivity to spatial preds.
**	17-n0v-97 (inkdo01)
**	    Added ops_cnffact to give server control over CNF transform 
**	    complexity determination.
**	09-Oct-1998 (jenjo02)
**	    Removed SCF semaphore functions, inlining the CS calls instead.
**	31-aug-00 (inkdo01)
**	    Add ops_rangep_max to permit deprecation of OPC_RMAX constant.
**	3-nov-00 (inkdo01)
**	    Add ops_cstimeout to make OPN_CSTIMEOUT variable.
**	10-Jan-2001 (jenjo02)
**	    Initialize GLOBALDEF OPG_CB* Opg_cb pointer,
**	    save offset to OPS_CB* in opg_opscb_offset, both
**	    used by GET_OPS_CB macro.
**	22-jul-02 (hayke02)
**	    Added ops_stats_nostats_max to allow scaling of the
**	    opf_stats_nostats_max value. This change fixes bug 108327.
**	06-Feb-2003 (jenjo02)
**	    BUG 109586: Explicitly init opg_waitinguser, get init'd
**	    memory from SCF for added insurance.
**	10-june-03 (inkdo01)
**	    Add support for "set joinop [no]newenum" and "set [no]hash".
**	19-july-03 (inkdo01)
**	    Fixed default "new enum" flag setting.
**	12-nov-03 (inkdo01)
**	    Add support for ops_pq_dop/threshold for parallel query processing.
**	4-mar-04 (inkdo01)
**	    Add support for "set [no]parallel [n]".
**	19-jan-05 (inkdo01)
**	    Add ops_pq_partthreads for partitioning/parallel query support.
**	23-Nov-2005 (kschendel)
**	    Result structure is now passed in from config options.
**	13-may-2007 (dougi)
**	    Add ops_autostruct flag.
**	30-oct-08 (hayke02)
**	    Add ops_greedy_factor to adjust the ops_cost/jcost comparison in
**	    opn_corput() for greedy enum only. This change fixes bug 121159.
**      11-jan-2010 (hanal04) bug 120482
**          Set ops_parallel based on opf_pq_dop which now defaults to 0 (OFF).
*/
DB_STATUS
ops_startup(
	OPF_CB           *opf_cb)
{
    ULM_RCB                ulmrcb;	/* control block used to call ULM */
    OPG_CB                 *opg_cbp;    /* ptr to server control block */
    SIZE_TYPE		   opf_memory;  /* amount of memory allocated for
                                        ** OPF for all sessions */
    i4		   i;           /* index for looping diff cache */
    {	/* allocate memory for OPF server control block */
	STATUS		sem_status;
	DB_STATUS	scfstatus;
	SCF_CB          scf_cb;
	SIZE_TYPE       requested;	/* number of blocks requested from
                                        ** SCF for server control block */

	scf_cb.scf_length = sizeof(SCF_CB);
	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_facility = DB_OPF_ID;
	scf_cb.scf_session = DB_NOSESSION;
	scf_cb.scf_scm.scm_functions = SCU_MZERO_MASK;

	/*
	** SCF requires # of SCU_MPAGESIZE byte pages; so round up.
	*/
	requested = (sizeof(OPG_CB) - 1)/SCU_MPAGESIZE + 1;
	scf_cb.scf_scm.scm_in_pages = requested;

	/* Now ask SCF for the memory. */
	scfstatus = scf_call(SCU_MALLOC, &scf_cb);
# ifdef E_OP008A_SCF_SERVER_MEM
	if ((DB_FAILURE_MACRO(scfstatus))
	    ||
	    (requested < scf_cb.scf_scm.scm_out_pages)
	    )
	{
	    opx_rverror( opf_cb, scfstatus, E_OP008A_SCF_SERVER_MEM,
                         scf_cb.scf_error.err_code);
	    return(E_DB_ERROR);
	}
# endif
	opf_cb->opf_server = (PTR) scf_cb.scf_scm.scm_addr; 
	opg_cbp = (OPG_CB *) opf_cb->opf_server;		/* get ptr 
					** to server control block */
	/* Init the globaldef */
	Opg_cb = opg_cbp;
	/* Save offset to OPS_CB* */
	opg_cbp->opg_opscb_offset = opf_cb->opf_opscb_offset;

	/* Initialize semaphore, to be used for setting up a session control
        ** block with the initial state variables, i.e. the global var
        ** which define the initial resources that a session can use, are 
        ** protected from changes by this semaphore */
	sem_status = 
	    CSw_semaphore(&opg_cbp->opg_semaphore, CS_SEM_SINGLE, "OPF OPS sem");
# ifdef E_OP008C_SEMAPHORE
	if (sem_status != OK)
	{
	    opx_rverror( opf_cb, E_DB_ERROR, E_OP008C_SEMAPHORE,
                         sem_status);
	    return(E_DB_ERROR);
	}
# endif
    }

    /* call the ADF to get operator id's needed for optimization */
    if (opg_getfunc(opf_cb, "=", DB_QUEL, &opg_cbp->opg_eq) != E_DB_OK)
        return(E_DB_FATAL);
    if (opg_getfunc(opf_cb, "<=", DB_QUEL, &opg_cbp->opg_le) != E_DB_OK)
        return(E_DB_FATAL);
    if (opg_getfunc(opf_cb, ">=", DB_QUEL, &opg_cbp->opg_ge) != E_DB_OK)
        return(E_DB_FATAL);
    if (opg_getfunc(opf_cb, "<", DB_QUEL, &opg_cbp->opg_lt) != E_DB_OK)
        return(E_DB_FATAL);
    if (opg_getfunc(opf_cb, ">", DB_QUEL, &opg_cbp->opg_gt) != E_DB_OK)
        return(E_DB_FATAL);
    if (opg_getfunc(opf_cb, "!=", DB_QUEL, &opg_cbp->opg_ne) != E_DB_OK)
        return(E_DB_FATAL);
    if (opg_getfunc(opf_cb, "any", DB_QUEL, &opg_cbp->opg_any) != E_DB_OK)
        return(E_DB_FATAL);
    if (opg_getfunc(opf_cb, "min", DB_QUEL, &opg_cbp->opg_min) != E_DB_OK)
        return(E_DB_FATAL);
    if (opg_getfunc(opf_cb, "max", DB_QUEL, &opg_cbp->opg_max) != E_DB_OK)
        return(E_DB_FATAL);
    if (opg_getfunc(opf_cb, "sum", DB_SQL, &opg_cbp->opg_sum) != E_DB_OK)
        return(E_DB_FATAL);
    if (opg_getfunc(opf_cb, "avg", DB_SQL, &opg_cbp->opg_avg) != E_DB_OK)
        return(E_DB_FATAL);
    if (opg_getfunc(opf_cb, "not like", DB_SQL, &opg_cbp->opg_notlike) != E_DB_OK)
        return(E_DB_FATAL);
    if (opg_getfunc(opf_cb, "like", DB_SQL, &opg_cbp->opg_like) != E_DB_OK)
        return(E_DB_FATAL);
    if (opg_getfunc(opf_cb, "is not null", DB_SQL, &opg_cbp->opg_isnotnull) != E_DB_OK)
        return(E_DB_FATAL);
    if (opg_getfunc(opf_cb, "is null", DB_SQL, &opg_cbp->opg_isnull) != E_DB_OK)
        return(E_DB_FATAL);
    if (opg_getfunc(opf_cb, "exists", DB_SQL, &opg_cbp->opg_exists) != E_DB_OK)
        return(E_DB_FATAL);
    if (opg_getfunc(opf_cb, "not exists", DB_SQL, &opg_cbp->opg_nexists) != E_DB_OK)
        return(E_DB_FATAL);
    if (opg_getfunc(opf_cb, "count", DB_SQL, &opg_cbp->opg_count) != E_DB_OK)
        return(E_DB_FATAL);
    if (opg_getfunc(opf_cb, "count(*)", DB_SQL, &opg_cbp->opg_scount) != E_DB_OK)
        return(E_DB_FATAL);
    if (opg_getfunc(opf_cb, "ii_iftrue", DB_SQL, &opg_cbp->opg_iftrue) != E_DB_OK)
        return(E_DB_FATAL);
    if (opg_getfunc(opf_cb, "ifnull", DB_SQL, &opg_cbp->opg_ifnull) != E_DB_OK)
        return(E_DB_FATAL);
    if (opg_getfunc(opf_cb, "ii_row_count", DB_SQL, &opg_cbp->opg_iirow_count) != E_DB_OK)
        return(E_DB_FATAL);
         
    {
	/* allocate a memory pool for the optimizer */
	DB_STATUS		ulmstatus; /* return status from ULM */

	if (!opf_cb->opf_mserver)
	    opf_memory = opf_cb->opf_mxsess * OPG_MEMSESMAX;
					/* 
					** amount of memory to be allocated by 
					** optimizer, allocate max for each
                                        ** session */
	else
	{
	    SIZE_TYPE	opf_memmin;	/* minimum amount of memory allocated
					** for OPF for all sessions */
	    opf_memmin = opf_cb->opf_mxsess * 20000;
	    if (opf_cb->opf_mserver < opf_memmin)
	    {
		i4		   ops_error;

		ule_format(E_OP0096_INSF_OPFMEM, (CL_SYS_ERR *)NULL, ULE_LOG,
		    NULL, (char *)0, (i4)0, (i4 *)0, &ops_error, 0);
		return(E_DB_FATAL);
	    }
	    else
		opf_memory = opf_cb->opf_mserver;
	}
	ulmrcb.ulm_facility = DB_OPF_ID;/* optimizer's id */
        ulmrcb.ulm_sizepool = opf_memory;
	ulmrcb.ulm_blocksize = OPS_BSIZE;  /* block size for optimizer */
	ulmstatus = ulm_startup(&ulmrcb);  /* call the ulm */
# ifdef E_OP0086_ULM_STARTUP
        if (DB_FAILURE_MACRO(ulmstatus))
	{
	    opx_rverror( opf_cb, ulmstatus, E_OP0086_ULM_STARTUP, 
                         ulmrcb.ulm_error.err_code);
	    return(E_DB_ERROR);
	}
# endif
	opg_cbp->opg_memory = ulmrcb.ulm_poolid;
    }
    {
	/* initialize the RDF */

	RDF_CCB                rdfccb;	/* control block used to initialize
                                        ** RDF */
	DB_STATUS              rdfstatus;  /* return status from RDF */
	rdfccb.rdf_fac_id = DB_OPF_ID;   /* optimizer's id for SCF */
        rdfccb.rdf_max_tbl = OPG_MTRDF;  /* maximum number of tables allowed for
                                        ** RDF cache */
#if 0
	rdfccb.rdf_poolid = 
	opg_cbp->opg_rdf_memory = opg_cbp->opg_memory; /* use the same memory pool 
					** as the optimizer */
	rdfccb.rdf_memleft = opf_memory/2;/* amount of memory that rdf can use
                                        */
#else
	rdfccb.rdf_poolid = 
	opg_cbp->opg_rdf_memory = (PTR)NULL; /* use the RDF memory */
	rdfccb.rdf_memleft = 0;		/* amount of memory that rdf can use
					** is zero since it now allocates its'
					** own memory
                                        */
#endif
	rdfccb.rdf_distrib = DB_1_LOCAL_SVR | DB_2_DISTRIB_SVR; /* supports
					** both local and distributed */
        rdfstatus = rdf_call(RDF_INITIALIZE, (PTR)&rdfccb);
# ifdef E_OP0087_RDF_INITIALIZE
	if (DB_FAILURE_MACRO(rdfstatus))
	{
	    (VOID) ulm_shutdown( &ulmrcb ); /* return memory to ULM but ignore
					    ** error code */
	    opx_rverror( opf_cb, rdfstatus, E_OP0087_RDF_INITIALIZE, 
                         rdfccb.rdf_error.err_code);
	    return(E_DB_FATAL);
	}
# endif
        opg_cbp->opg_rdfhandle = rdfccb.rdf_fcb; /* save handle to be used for
                                        ** all future calls to RDF
                                        */
    }
    opf_cb->opf_version = OPF_VERSION;  /* return OPF version number */
    opf_cb->opf_sesize = sizeof( OPS_CB ); /* return size of session control
					** block to SCF */
    opg_cbp->opg_alter.ops_sestime =
		opf_cb->opf_joinop_timeout;

    opg_cbp->opg_alter.ops_timeoutabort =
		opf_cb->opf_joinop_timeoutabort;

    opg_cbp->opg_alter.ops_cnffact = opf_cb->opf_cnffact;  
    opg_cbp->opg_alter.ops_rangep_max = opf_cb->opf_rangep_max;
    opg_cbp->opg_alter.ops_pq_dop = opf_cb->opf_pq_dop;
    opg_cbp->opg_alter.ops_parallel = (opf_cb->opf_pq_dop > 1) ? TRUE : FALSE;
    opg_cbp->opg_alter.ops_pq_threshold = opf_cb->opf_pq_threshold;
    opg_cbp->opg_alter.ops_pq_partthreads = opf_cb->opf_pq_partthreads;
					/* just copy 'em over */
    if (opf_cb->opf_cstimeout <= 0) 
	opg_cbp->opg_alter.ops_cstimeout = OPN_CSTIMEOUT;
    else
	opg_cbp->opg_alter.ops_cstimeout = opf_cb->opf_cstimeout;
    if (opf_cb->opf_cpufactor > 0.0)
	opg_cbp->opg_alter.ops_cpufactor = opf_cb->opf_cpufactor; /* relationship between 
                                        ** cost of i/o and cost of cpu */
    else
	opg_cbp->opg_alter.ops_cpufactor = OPG_CPUFACTOR; /* relationship between 
                                        ** cost of i/o and cost of cpu */
    if (opf_cb->opf_timeout > 0.0)
	opg_cbp->opg_alter.ops_tout = opf_cb->opf_timeout; /* used to 
					** convert the cost units into
					** millisec's */
    else
	opg_cbp->opg_alter.ops_tout = OPS_MILADJUST; /* use default */
    if (opf_cb->opf_rfactor > 0.0)
	opg_cbp->opg_alter.ops_repeat= opf_cb->opf_rfactor; /* used to affect 
					** the timeout for repeat query evaluation */
    else
	opg_cbp->opg_alter.ops_repeat = 10.0; /* relationship between 
                                        ** cost of i/o and cost of cpu */
    if (opf_cb->opf_sortmax >= 0.0)
	opg_cbp->opg_alter.ops_sortmax = opf_cb->opf_sortmax; /* used to affect 
					** the timeout for sortmax query evaluation */
    else
	opg_cbp->opg_alter.ops_sortmax = -1.0; /* optimizer will avoid plans in which the
					** estimated size of a sort would involve
					** data which is larger than this number, unless
					** the semantics require that a sort is done, if
					** zero then no limit will be used */
    opg_cbp->opg_alter.ops_qsmemsize = opf_cb->opf_qefsort; /* used to affect 
					** the timeout for sortmax query evaluation */
    if ((opf_cb->opf_exact > 0.0) && (opf_cb->opf_exact < 1.0))
	opg_cbp->opg_alter.ops_exact = opf_cb->opf_exact;
					/* EXACTKEY startup flag, value between 0.0 and 1.0
					** which describes the selectivity of exact
					** matches, 0.0 means use the default (of 0.01)*/
    else
	opg_cbp->opg_alter.ops_exact = OPS_EXSELECT;
    if ((opf_cb->opf_range > 0.0) && (opf_cb->opf_range < 1.0))
	opg_cbp->opg_alter.ops_range = opf_cb->opf_range;
					/* RANGEKEY startup flag, value between 0.0 and 1.0
					** which describes the selectivity of inexact
					** matches 0.0 means use the default (of 0.10)*/
    else
	opg_cbp->opg_alter.ops_range = OPS_INEXSELECT;
    if ((opf_cb->opf_nonkey > 0.0) && (opf_cb->opf_nonkey < 1.0))
	opg_cbp->opg_alter.ops_nonkey = opf_cb->opf_nonkey;
					/* NONKEY startup flag, value between 0.0 and 1.0
					** which describes the selectivity of non-keying
					** qualifications, 0.0 means use the default
					** (of 0.50) */
    else
	opg_cbp->opg_alter.ops_nonkey = OPN_DUNKNOWN;

    if ((opf_cb->opf_spatkey > 0.0) && (opf_cb->opf_spatkey < 1.0))
	opg_cbp->opg_alter.ops_spatkey = opf_cb->opf_spatkey;
					/* SPATKEY startup flag, value between 0.0 and 1.0
					** which describes the selectivity of spatial
					** qualifications, 0.0 means use the default
					** (of 0.50) */
    else
	opg_cbp->opg_alter.ops_spatkey = OPN_DUNKNOWN;

    opg_cbp->opg_alter.ops_timeout = TRUE; /* TRUE - optimizer should timeout
                                        ** if estimate cost of plan equals
				        ** current computation time
                                        */
    opg_cbp->opg_alter.ops_noproject = OPG_NOPROJECT; /* default is to project
                                        ** aggregates */
    /* The result_structure comes from the config now */
    opg_cbp->opg_alter.ops_storage = opf_cb->opf_value;
    opg_cbp->opg_alter.ops_compressed = opf_cb->opf_compressed;
    opg_cbp->opg_alter.ops_autostruct = opf_cb->opf_autostruct;
    if (1.0 / (float)opf_cb->opf_mxsess < opf_cb->opf_maxmemf)
	opg_cbp->opg_alter.ops_maxmemory = opf_memory * opf_cb->opf_maxmemf;
     else opg_cbp->opg_alter.ops_maxmemory = opf_memory / opf_cb->opf_mxsess;
					/* set default maximum 
					** amount of memory per session */
    opg_cbp->opg_alter.ops_qep = FALSE;   /* default is not to print QEP's */
    if (opf_cb->opf_smask & OPF_NEW_ENUM) /* default is taken from CBF parm */
	opg_cbp->opg_alter.ops_newenum = TRUE;
    else 
	opg_cbp->opg_alter.ops_newenum = FALSE;
    if (opf_cb->opf_smask & OPF_HASH_JOIN) /* default taken from CBF parm */
	opg_cbp->opg_alter.ops_hash = TRUE;
    else 
	opg_cbp->opg_alter.ops_hash = FALSE;
    if (opf_cb->opf_smask & OPF_GSUBOPTIMIZE)
	opg_cbp->opg_alter.ops_amask = OPS_ANOFLATTEN; /* this flag will avoid
					** flattening for all sessions */
    else
	opg_cbp->opg_alter.ops_amask = 0; /* flattening is enabled */
    if (opf_cb->opf_smask & OPF_COMPLETE)
	opg_cbp->opg_alter.ops_amask |= OPS_ACOMPLETE; /* this flag will turn
					** on completeness as a default */
    if (opf_cb->opf_smask & OPF_NOAGGFLAT)
	opg_cbp->opg_alter.ops_amask |= OPS_ANOAGGFLAT; /* this flag suppress
					** flattening of queries which contain
					** correlated aggregates */
    if ((opf_cb->opf_stats_nostats_factor > 0.0) &&
	(opf_cb->opf_stats_nostats_factor <= 1.0))
	opg_cbp->opg_alter.ops_stats_nostats_factor =
				opf_cb->opf_stats_nostats_factor;
    else
	opg_cbp->opg_alter.ops_stats_nostats_factor = 1.0;
    if (opf_cb->opf_smask & OPF_DISABLECARDCHK)
	opg_cbp->opg_alter.ops_nocardchk = TRUE;

    opg_cbp->opg_alter.ops_greedy_factor = opf_cb->opf_greedy_factor;
    opg_cbp->opg_check = FALSE;           /* TRUE if any of the global trace
                                        ** or timing flags are set
                                        */
    opg_cbp->opg_alter.ops_inlist_thresh = opf_cb->opf_inlist_thresh;
    ult_init_macro( &opg_cbp->opg_tt, OPT_BMGLOBAL, OPT_GWVALUES, OPT_GVALUES );
    opg_cbp->opg_first = FALSE;		/* first session has not been initialized */
    if (opf_cb->opf_ddbsite)
    {	/* create fake LDB descriptor for target site */
	char	    *target_name;
	DD_LDB_DESC *ldbdescp;

	ldbdescp = &opg_cbp->opg_ddbsite;
	MEfill ((i4)sizeof(*ldbdescp), (u_char)' ', (PTR)ldbdescp);
	MEcopy ((PTR)opf_cb->opf_ddbsite, sizeof(ldbdescp->dd_l2_node_name),
	    (PTR)&ldbdescp->dd_l2_node_name[0]); /* initialize node name for the
					** site of the distributed server */
	ldbdescp->dd_l1_ingres_b = FALSE;
	target_name = "not applicable";
	MEmove(STlength(target_name), (PTR)target_name,
	    ' ', sizeof(ldbdescp->dd_l3_ldb_name),
	    (PTR)&ldbdescp->dd_l3_ldb_name[0]);
	target_name = "INGRES";		/* for some reason the convention is
					** to use capitals only for the LDB
					** type */
	MEmove(STlength(target_name), (PTR)target_name,
	    ' ', sizeof(ldbdescp->dd_l4_dbms_name),
	    (PTR)&ldbdescp->dd_l4_dbms_name[0]);
	ldbdescp->dd_l5_ldb_id = DD_0_IS_UNASSIGNED;
    }
    /* Initialize the parameters from opf_cb to opg_cb */
    opg_cbp->opg_maxtup = opf_cb->opf_maxtup; 
    opg_cbp->opg_maxpage = opf_cb->opf_maxpage; 
    for (i = 0; i < DB_NO_OF_POOL; i++)
    {
	opg_cbp->opg_pagesize[i] = opf_cb->opf_pagesize[i];
	opg_cbp->opg_tupsize[i] = opf_cb->opf_tupsize[i];
    }
    opg_cbp->opg_actsess = opf_cb->opf_actsess; /* save maximum number of sessions
					** for cache calculation purposes */
    opg_cbp->opg_mxsess = opf_cb->opf_mxsess; /* save maximum number of sessions
					** which can execute concurrently in OPF */
    opg_cbp->opg_smask = opf_cb->opf_smask; /* save the constraints on access path
					** usage from SCF */
    opg_cbp->opg_activeuser = 0;	/* init active user count */
    opg_cbp->opg_waitinguser = 0;	/* init waiting user count */
    opg_cbp->opg_swait = 0;		/* init opf queue wait count */
    opg_cbp->opg_slength = 0;		/* init maximum opf queue length */
    opg_cbp->opg_sretry = 0;		/* init opf retry count */
    opg_cbp->opg_avgwait = 0.0;		/* init average wait time */
    opg_cbp->opg_mwaittime = 0;		/* init maximum wait time */
    opg_cbp->opg_psession_id = 0;	/* init session counter */
    if (opg_cbp->opg_mxsess < opg_cbp->opg_actsess)
    {	/* need to create a condition, since some OPF users can be blocked */
	STATUS	    cond_status;
	cond_status = CScnd_init(&opg_cbp->opg_condition);
	if (cond_status != OK)
	{
	    (VOID) ulm_shutdown( &ulmrcb ); /* return memory to ULM but ignore
					    ** error code */
	    opx_rverror( opf_cb, E_DB_FATAL, E_OP0098_CONDITION_INIT, 
                         (OPX_FACILITY)cond_status);
	    return(E_DB_FATAL);
	}
	opg_cbp->opg_mask |= OPG_CONDITION; /* enable the condition code */
    }
    else
	opg_cbp->opg_mask = 0;
    opg_cbp->opg_lnexp = MHln(FMIN);    /* precalculate constant used to
                                        ** avoid underflow */
    return(E_DB_OK);
}
