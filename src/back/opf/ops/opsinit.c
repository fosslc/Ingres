/*
**Copyright (c) 2004 Ingres Corporation
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
#define        OPS_INIT      TRUE
#include    <me.h>
#include    <opxlint.h>


/**
**
**  Name: OPSINIT.C - initialize structures for query optimization
**
**  Description:
**      Routine to initialize the structures required for query optimization
**
**  History:
**      30-jun-86 (seputis)    
**          initial creation
**	29-jan-89 (paul)
**	    Enhanced to initialize state information for rule processing.
**	22-may-89 (neil)
**	    Put PSQ_DELCURS through same fast path as PSQ_REPCURS.
**	26-nov-90 (stec)
**	    Added initialization of ops_prbuf in ops_init().
**	28-dec-90 (stec)
**	    Changed initialization of print buffer in ops_init().
**      16-aug-91 (seputis)
**          add CL include files for PC group
**      04-sep-92 (fpang)
**          In ops_init(), fixed initialization of rdr_r1_distrib.
**	28-oct-92 (jhahn)
**	    Added initialization of ops_inStatementEndRules.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	15-Aug-1997 (jenjo02)
** 	    New field, ulm_flags, added to ULM_RCB in which to specify the
**	    type of stream to be opened, shared or private.
**	30-jan-2003 (somsa01)
**	    In ops_qinit(), moved the initialization of ops_gmask before
**	    checking for !statementp. This fixes E_OP0791_ADE_INSTRGEN when
**	    creating a procedure after the fixes for bug 109194.
**/

/*{
** Name: ops_gqtree	- get query tree from QSF
**
** Description:
**      This procedure will get a query tree from QSF
**      and initialize the global QSF control block
**
** Inputs:
**      global                          ptr to global state variable
**
** Outputs:
**      global->ops_qsfcb               initialized
**	Returns:
**	    QSF status
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-jan-88 (seputis)
**          initial creation
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for qsf_owner which has changed type to PTR.
**	10-Jan-2001 (jenjo02)
**	    Initialize qsf_sid with session's SID.
[@history_template@]...
*/
DB_STATUS
ops_gqtree(
	OPS_STATE          *global)
{
    /* get the query tree from QSF */
    DB_STATUS	       qsfstatus;  /* QSF return status */

    global->ops_qsfcb.qsf_length = sizeof(QSF_RCB); /* initialize header
				    ** for control block */
    global->ops_qsfcb.qsf_type = QSFRB_CB;
    global->ops_qsfcb.qsf_owner = (PTR)DB_OPF_ID;
    global->ops_qsfcb.qsf_ascii_id = QSFRB_ASCII_ID;
    global->ops_qsfcb.qsf_sid = global->ops_cb->ops_sid;

    STRUCT_ASSIGN_MACRO(global->ops_caller_cb->opf_query_tree, global->ops_qsfcb.qsf_obj_id);
				    /* move the name to the QSF control 
				    ** block */
    global->ops_qsfcb.qsf_lk_state = QSO_EXLOCK; /* lock exclusively so 
				    ** that this object can be destroyed 
				    ** later */

    qsfstatus = qsf_call( QSO_LOCK, &global->ops_qsfcb); /* get exclusive 
				    ** access to object */
    if (DB_SUCCESS_MACRO(qsfstatus))
    {
        global->ops_lk_id = global->ops_qsfcb.qsf_lk_id; /* save the lock id 
					** so that the object can be destroyed 
                                        ** later */
	global->ops_procedure = (PST_PROCEDURE *)global->ops_qsfcb.qsf_root; /* save 
                                        ** the ptr to procedure header which
                                        ** should be the root */
    }
    return(qsfstatus);
}

/*{
** Name: ops_qinit - initialize structures needed for optimization
**
** Description:
**	This routine will initialize the "global state" variable for one query
**      within a procedure.  It must be called prior to the optimization of
**      every query in the procedure
**
** Inputs:
**      global				ptr to global state variable
**          .ops_cb                     ptr to session control block
**          .ops_caller_cb              ptr to same object as opf_cb
**      statementp			ptr to statement containing query
**                                      to be optimized
**
** Outputs:
**      global                          all components initialized
**	Returns:
**	    E_DB_OK, E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-feb-86 (seputis)
**          initial creation
**	8-sep-86 (seputis)
**          DBP init range table only if statementp == NULL
**      6-nov-88 (seputis)
**          initial opn_statistics
**	28-jan-89 (paul)
**	    Initialize rules processing flag to FALSE. Indicates we are
**	    not currently compiling a rule action.
**	22-may-89 (neil)
**	    Put PSQ_DELCURS through same fast path as PSQ_REPCURS.
**      22-sep-89 (seputis)
**          added ops_gmask for boolean expansion
**      14-nov-89 (seputis)
**          fixed 8629, to not use repeat parameters in procedures
**	6-jun-90 (seputis)
**	    fix b30463 - moved some large memory structure handling to
**	    the procedure level rather than query level
**      28-aug-90 (seputis)
**          - fix b32757 - init opn_statistics in all cases
**	26-nov-90 (stec)
**	    Added initialization of ops_prbuf in OPS_STATE struct.
**	23-jan-91 (seputis)
**	    moved RDF_CB initialization here so that OPC can use
**	    this initialization also.
**	28-oct-92 (jhahn)
**	    Added initialization of ops_inStatementEndRules.
**      01-Jan-00 (sarjo01)
**          Bug 99120: Added opn_initcollate() call
**	5-dec-02 (inkdo01)
**	    Changes for range table expansion.
**	30-jan-2003 (somsa01)
**	    Moved the initialization of ops_gmask before checking
**	    for !statementp. This fixes E_OP0791_ADE_INSTRGEN when
**	    creating a procedure after the fixes for bug 109194.
**	4-nov-04 (inkdo01)
**	    Init opn_fragcolist for greedy enumeration.
**	15-june-06 (dougi)
**	    Add support for "before" triggers.
**	21-Oct-2010 (kiria01) b123345
**	    Do not assume PST_DV_TYPE is only present when the
**	    context is a DBP. It may also be present from temporaries
**	    created by the parser.
[@history_template@]...
*/
VOID
ops_qinit(
	OPS_STATE          *global,
	PST_STATEMENT      *statementp)
{
    MEfill(sizeof(OPV_GBMVARS), 0, (char *)&global->ops_rangetab.opv_mrdf);  
					/* used for deallocation */
    global->ops_rangetab.opv_gv = 0;    /* no global range variables defined */
    global->ops_rangetab.opv_base = NULL; /* - NULL indicates that the
                                        ** global range table not been allocated
                                        ** - used by error handling to
                                        ** deallocate ULM and RDF resources */

    /* Initially, we are not processing rules at the beginning of an	    */
    /* optimization or the beginning of a PST_QT_TYPE */
    global->ops_inAfterRules = global->ops_inBeforeRules = FALSE;
    global->ops_inAfterStatementEndRules = FALSE;
    global->ops_inBeforeStatementEndRules = FALSE;
    global->ops_gmask = 0;		/* init boolean mask */

    if (!statementp)
	return;				/* only initialize if statement is provided
					** otherwise, only init the global range
					** table for resource deallocation */
    global->ops_statement = statementp;
    global->ops_qheader = statementp->pst_specific.pst_tree; /*
				    ** get the query tree header root from
				    ** the statement */


    global->ops_copiedco = NULL;        /* no CO nodes have been allocated yet */
    global->ops_subquery = NULL;        /* initialize the subquery list */

    /* global->ops_astate initialized by aggregate processing phase */
    /* global->ops_estate initialized by joinop processing phase */
    {
	global->ops_mstate.ops_usemain = FALSE; /* disable redirection of
					** memory allocation */
	global->ops_terror = FALSE;	/* TRUE if tuple too wide for 
					** intermediate relation */
    }

    /* the number of parameters may increase in a query if any simple 
    ** aggregates are found, thus a separate count of query parameters 
    ** is kept in the state variable.
    */
    if (global->ops_procedure->pst_isdbp)
    {	/* procedures do not have repeat query parameters, so use
	** the local variable declaration to find the largest parameter
	** number, there is an assumption that there is only one
	** PST_DECVAR statement and this can be used to determine
	** the largest allocated constant number */
	PST_STATEMENT	    *decvarp;
	decvarp = global->ops_procedure->pst_stmts;
	if (decvarp->pst_type != PST_DV_TYPE)
	    opx_error(E_OP0B81_VARIABLE_DEC); /* verify statement type, an
					** assumption is that a declaration
					** is always the first statement of
					** a procedure */
	global->ops_parmtotal = decvarp->pst_specific.pst_dbpvar->pst_nvars
	    + decvarp->pst_specific.pst_dbpvar->pst_first_varno - 1;
    }
    else
    {
	global->ops_parmtotal = global->ops_qheader->pst_numparm;
	if (global->ops_procedure->pst_stmts->pst_type == PST_DV_TYPE)
	{
	    /* Support local variables that may have been introduced
	    ** as temporaries */
	    PST_DECVAR *decvarp = global->ops_procedure->
				pst_stmts->pst_specific.pst_dbpvar;
	    global->ops_parmtotal += decvarp->pst_nvars
				+ decvarp->pst_first_varno - 1;
	}
    }
   {
        /* initialize outer join descriptors */
        global->ops_goj.opl_gbase = (OPL_GOJT *) NULL;
        global->ops_goj.opl_view = (OPL_GOJT *) NULL;
        global->ops_goj.opl_fjview = (OPL_GOJT *) NULL;
        global->ops_goj.opl_glv = global->ops_qheader->pst_numjoins;
        if (global->ops_qheader->pst_numjoins > 0)
            global->ops_goj.opl_mask = OPL_OJFOUND;
            else
        global->ops_goj.opl_mask = 0;
    }
    global->ops_rqlist = NULL;	/* list of repeat query descriptors */

    if ((    (global->ops_qheader->pst_mode != PSQ_REPCURS)
	    && 
	    (global->ops_qheader->pst_mode != PSQ_DELCURS)
	)
	||
	(global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
	)

    {
	opv_grtinit( global, TRUE);	/* initialize the global range table 
                                        ** ops_rangetab structure, ... replace
                                        ** & delete cursor do not need this */

    }
    else
    {	/* special case short cut "hack" for replace/delete cursor stmt which
	** should not need to go through enumeration,... it should go
        ** directly to query compilation to compile the target list
        ** expressions */
   	/* for distributed the parse tree is traversed to handle the
   	** ~V case for multi-site so this section is not needed */
#if 0
/* looks like this was replaced by opv_grtinit call, with the extra
** boolean parameter */
        MEfill(sizeof(OPV_GLOBAL_RANGE), (u_char)0,
            (PTR)&global->ops_rangetab);    /* init global range table for
                                            ** query compilation */
#endif
	opv_grtinit( global, FALSE);
	global->ops_subquery = (OPS_SUBQUERY *)opu_memory(global, 
	    (i4) sizeof(OPS_SUBQUERY));
        MEfill(sizeof(OPS_SUBQUERY), (u_char)0, 
	    (PTR)global->ops_subquery);	    /* init subquery structure for
					    ** query compilation */
	global->ops_subquery->ops_sqtype = OPS_MAIN;
	global->ops_subquery->ops_root = global->ops_qheader->pst_qtree;
    }
    /* these fields should be initialized once before the enumeration phase
    */
    global->ops_estate.opn_cocount = 0; /* number of CO nodes available */
    global->ops_estate.opn_statistics = FALSE; /* statistics has not been
				    ** turned on yet */
    global->ops_estate.opn_colist = NULL; /* list of avail perm  CO nodes */
    global->ops_estate.opn_fragcolist = NULL; 
    global->ops_union = FALSE;	/* TRUE if union view needs to be
				    ** processed */
    global->ops_tvar = OPV_NOGVAR;  /* init var for special case update
				    ** by TID */
    {	/* distributed initialization */
	global->ops_gdist.opd_tcost = NULL;
	global->ops_gdist.opd_dv = 0;
	global->ops_gdist.opd_base = NULL;
	global->ops_gdist.opd_tbestco = NULL;
	global->ops_gdist.opd_copied = NULL;
	global->ops_gdist.opd_scopied = FALSE;
	global->ops_gdist.opd_repeat = NULL;
	global->ops_gdist.opd_gmask = 0;
	global->ops_gdist.opd_user_parameters = 0;
    }
    opn_initcollate(global);

}

/*{
** Name: ops_init - initialize structures needed for optimization
**
** Description:
**	This routine will initialize the "global state" variable which contains
**      all information for the optimization of this query for this session.
**
** Inputs:
**      global				ptr to global state variable
**          .ops_cb                     ptr to session control block
**          .ops_caller_cb              ptr to same object as opf_cb
**      opf_cb                          caller's control block
**
** Outputs:
**      global                          all components initialized
**	Returns:
**	    E_DB_OK, E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-feb-86 (seputis)
**          initial creation
**	26-nov-90 (stec)
**	    Added initialization of ops_prbuf in OPS_STATE struct.
**	28-dec-90 (stec)
**	    Changed initialization of print buffer; ops_prbuf has been
**	    removed, opc_prbuf in OPC_STATE struct will be initialized
**	    instead.
**      28-jan-91 (seputis)
**          added support for OPF ACTIVE FLAG
**	11-jun-91 (seputis)
**	    ifdef distributed code until header files merge
**      04-sep-92 (fpang)
**          Fixed initialization of rdr_r1_distrib.
**      27-apr-95 (inkdo01)
**          Init ops_osubquery to NULL 
**	26-nov-96 (inkdo01)
**	    Allow users OPF memory equal to 1.5 config.dat definition. That way
**	    the boundary users don't dictate the size of the mem pool (it being
**	    highly unlikely that all users will want big memory simultaneously).
**	12-dec-96 (inkdo01)
**	    Shamed into doing the above change more rigourously. Now a new 
**	    config parm (opf_maxmemf) defines the proportion of the OPF pool
**	    available to any session. This gets computed in opsstartup, so the
**	    code added for the previous change is now removed.
**	20-jun-1997 (nanpr01)
**	    Initialize the rdf_info_blk to NULL. This is specially required
**	    for  query without a base table.
**	23-oct-98 (inkdo01)
**	    Init opc_retrowno, opc_retrowoff for row producing procs.
**	27-oct-98 (inkdo01)
**	    Quicky afterthought to init opc_retrow_rsd.
**	11-oct-2006 (hayke02)
**	    Send E_OP0002_NOMEMORY to errlog.log. This change fixes bug 116309.
**      25-Nov-2008 (hanal04) Bug 121248
**          Initialise new caller_ref field in the opv_rdfcb to avoid SEGVs
**          later on.
**	25-feb-10 (smeke01) b123333
**	    As the NULL-ing of ops_trace.opt_conode has been remmoved from 
**	    opt_printCostTree() we need to make sure it is initialised here  
**	    prior to any call of opt_cotree() by trace point op145 (set qep).
[@history_line@]...
[@history_template@]...
*/
VOID
ops_init(
	OPF_CB             *opf_cb,
	OPS_STATE          *global)
{
    /* initialize some variables so that error recovery can determine which
    ** resources to free (i.e. which resources have been successfully
    ** allocated)
    ** - this must be done prior to the allocation of a memory stream since
    ** the streamid is used to indicate whether any resources at all have
    ** been allocated
    */

    global->ops_cb = (OPS_CB *)opf_cb->opf_scb; /* save session control block */
    /* global->ops_caller_cb initialized before exception handler established */
    global->ops_adfcb = global->ops_cb->ops_adfcb; /* get current ADF control
                                        ** block for session */
    if (global->ops_adfcb)
    {	/* init adf_constants since this may uninitialized after the
	** previous query executed by QEF, and may cause ADF to write
	** to deallocated memory */
	global->ops_adfcb->adf_constants = (ADK_CONST_BLK *)NULL;
    }
    global->ops_qheader = NULL;		
    global->ops_statement = NULL;		
    global->ops_procedure = NULL;	/* - NULL indicates that QSF query
                                        ** tree has not been fixed 
                                        ** - used by error handling to determine
                                        ** if this resource needs to be 
                                        ** deallocated
                                        */
    /* global->ops_lk_id initialized below */
    /* global->ops_parmtotal initialized below */

    global->ops_mstate.ops_streamid = NULL;/* init so deallocate routines do not
                                        ** access unless it is necessary */
    global->ops_mstate.ops_sstreamid = NULL; /* init so deallocate routines do
                                        ** not access unless it is necessary */
    global->ops_mstate.ops_tstreamid = NULL; /* the temporary buffer stream is
                                        ** allocated only when needed */
    global->ops_subquery = NULL;        /* initialize the subquery list */
    global->ops_osubquery = NULL;

    /* initialise pointer used in opt_printCostTree() for tracing CO node */
    global->ops_trace.opt_conode = NULL;  

    /* global->ops_astate initialized by aggregate processing phase */
    /* global->ops_estate initialized by joinop processing phase */
    global->ops_qpinit = FALSE;		/* query plan object not allocated yet*/
    global->ops_cstate.opc_prbuf = NULL;/* trace print buffer ptr. */
    global->ops_cstate.opc_relation = NULL;  /* init relation descriptor
                                            ** so deallocation routine will
                                            ** only be done if OPC allocates
                                            ** an RDF descriptor */
    global->ops_cstate.opc_retrowno = -1;
    global->ops_cstate.opc_retrowoff = 0;  /* result row buffer init */
    global->ops_cstate.opc_retrow_rsd = (PST_QNODE *) NULL;
    ops_qinit(global, (PST_STATEMENT *)NULL); /* init range table only for resource
					** deallocation */
    {
	/* allocate a memory stream to be used by the optimizer
        ** - the streamid PTR was initialized to NULL earlier - prior to the
        ** establishment of the exception handler so that it can be used
        ** to indicate to the cleanup routines that no resources have been
        ** allocated
        */
        DB_STATUS       ulmstatus;

        global->ops_mstate.ops_ulmrcb.ulm_facility = DB_OPF_ID; /* identifies optimizer
					** so that ULM can make SCF calls on
                                        ** behave of the optimizer */
        global->ops_mstate.ops_ulmrcb.ulm_poolid = global->ops_cb->ops_server->opg_memory;
                                        /* poolid of OPF
                                        ** obtained at server startup time */
        global->ops_mstate.ops_ulmrcb.ulm_blocksize = 0; /* use default for 
					** now */
        global->ops_mstate.ops_memleft =global->ops_cb->ops_alter.ops_maxmemory;
                                        /* save amount of memory which can be 
                                        ** used by this session */
        global->ops_mstate.ops_mlimit = global->ops_mstate.ops_memleft / 10;
					/* if 10% of memory is left trigger 
                                        ** garbage collection routines */
        global->ops_mstate.ops_ulmrcb.ulm_memleft = &global->ops_mstate.ops_memleft; /* 
					** and point to it for ULM */
	global->ops_mstate.ops_ulmrcb.ulm_streamid_p = &global->ops_mstate.ops_streamid; /*
					** Where ULM will return streamid */
	global->ops_mstate.ops_ulmrcb.ulm_flags = ULM_PRIVATE_STREAM;
					/* Allocate private, thread-safe streams */
        ulmstatus = ulm_openstream(&global->ops_mstate.ops_ulmrcb); /* get memory for the
                                        ** optimizer */
        if (DB_FAILURE_MACRO(ulmstatus))
	{
	    opx_lerror(E_OP0002_NOMEMORY, 0);
	    opx_verror( ulmstatus, E_OP0002_NOMEMORY, 
		global->ops_mstate.ops_ulmrcb.ulm_error.err_code);
	}
        global->ops_mstate.ops_tptr = NULL; /* init temp buffer ptr from 
                                        ** ops_tstreamid*/
        global->ops_mstate.ops_tsize = 0;          /* init temp buffer size */
					/* ULM has set ops_streamid to the
                                        ** streamid for "global optimizer"
                                        ** memory, note the enumeration will
                                        ** create other streamids for "local"
                                        ** memory but still use the 
                                        ** same control ops_ulmrcb 
                                        ** in order to decrement and
                                        ** increment the same "memleft" counter
                                        */
	/* initialize ptrs to full size array of ptrs, allocate once for DB procedure
	** or 4K will be wasted for each assignment statement and query */
        global->ops_mstate.ops_trt = NULL;
        global->ops_mstate.ops_tft = NULL;
        global->ops_mstate.ops_tat = NULL;
        global->ops_mstate.ops_tet = NULL;
        global->ops_mstate.ops_tbft = NULL;
	global->ops_mstate.ops_usemain = FALSE; /* disable redirection of
					** memory allocation */
	global->ops_mstate.ops_totalloc = 0;	/* init stats fields */
	global->ops_mstate.ops_countalloc = 0;
	global->ops_mstate.ops_count2kalloc = 0;
	global->ops_mstate.ops_recover = 0;
    }

    
    {
	/* get the procedure from QSF */
        DB_STATUS	       qsfstatus;  /* QSF return status */

	qsfstatus = ops_gqtree(global); /* get procedure from QSF */
        if (DB_FAILURE_MACRO(qsfstatus))
	    opx_verror( qsfstatus, E_OP0085_QSO_LOCK, 
		global->ops_qsfcb.qsf_error.err_code); /* report error */

    }

    {
	/* initialize the RDF control block used to fetch information for
        ** the global range table */
	RDR_RB                 *rdfrb;      /* ptr to RDF request block */

	rdfrb = &global->ops_rangetab.opv_rdfcb.rdf_rb;
	global->ops_rangetab.opv_rdfcb.rdf_info_blk = NULL;
        global->ops_rangetab.opv_rdfcb.caller_ref = (RDR_INFO **)NULL;
	rdfrb->rdr_db_id = global->ops_cb->ops_dbid; /* save the data base id
                                            ** for this session only */
        rdfrb->rdr_unique_dbid = global->ops_cb->ops_udbid; /* save unique
                                            ** dbid for all sessions */
        rdfrb->rdr_session_id = global->ops_cb->ops_sid; /* save the session id
                                            ** for this session */
        rdfrb->rdr_fcb = global->ops_cb->ops_server->opg_rdfhandle; /* save the
					    ** poolid for the RDF info */
        if (global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
            rdfrb->rdr_r1_distrib = DB_3_DDB_SESS;
	else
            rdfrb->rdr_r1_distrib = 0;
	if (global->ops_cb->ops_smask & OPS_MCONDITION)
	    rdfrb->rdr_2types_mask = RDR2_TIMEOUT; /* indicate that timeout should
					    ** occur on all RDF accesses */
	else
	    rdfrb->rdr_2types_mask = 0;
	rdfrb->rdr_instr = RDF_NO_INSTR;
    }
}
