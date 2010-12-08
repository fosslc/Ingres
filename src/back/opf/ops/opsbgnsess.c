/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <ex.h>
#include    <sl.h>
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
#define        OPS_BGN_SESSION      TRUE
#define	       OPX_EXROUTINES       TRUE
#include    <opxlint.h>


/**
**
**  Name: OPSBGNSESSION.C - begin a new session
**
**  Description:
**      Routine which will begin a new OPF session
**
**  History:
**      30-jun-86 (seputis)    
**          initial creation
**	15-dec-91 (seputis)
**	    make linting fix, used for parameter checking
**      8-sep-92 (ed)
**          remove star ifdef code
**	07-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Copy case translation flags into session cb (ops_dbxlate).
**	    Copy catalog owner name into session db (ops_cat_owner).
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	01-sep-93 (jhahn)
**	    Added ops_psession_id and ops_session_ctr for maintaining server
**	    lifetime unique id's.
**	24-jan-1994 (gmanning)
**	    Added #include of me.h in order to compile on NT.
**	11-oct-93 (johnst)
**	    Bug #56447
**	    Cast ops_cb->ops_sid = (CS_SID)opf_cb->opf_sid on assignment to
**	    remove compiler/lint warning message. CS_SID is guaranteed to be
**	    sizeof(PTR).
**      19-dec-94 (sarjo01) from 15-sep-94 (johna)
**          initialize dmt_mustlock (added by markm while fixing bug (58681)
**      06-mar-96 (nanpr01)
**          initialize the scan block with default since distributed does 
**	    not have DMF. Also initialize the scan block for different
**	    buffer pools.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 ops_bgn_session(
	OPF_CB *opf_cb);

/*{
** Name: ops_bgn_session	- initialize session control block
**
** Description:
**      This routine will initialize the session control block.  The memory 
**      for the session control block has been allocated for the optimizer 
**      by the SCF.
**
** Inputs:
**      opf_cb                          - ptr to caller's control block
**
** Outputs:
**      opf_cb                          - ptr to caller's control block
**          .ops_scb                    - pointer to session control block
**                                      has been initialized
**	Returns:
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	27-mar-86 (seputis)
**          initial creation
**	23-oct-88 (seputis)
**          save pointer to coordinator descriptor
**	16-jul-90 (seputis)
**	    added fix for b30809
**      08-jan-91 (jkb)
**          Add EXmath to make sure all Floating point exceptions are
**          returned before the dmfcall (this forces the 387 co-processor
**          on Sequent to report any unreported FPE's)
**	07-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Copy case translation flags into session cb (ops_dbxlate).
**	    Copy catalog owner name into session db (ops_cat_owner).
**      20-jul-93 (ed)
**          changed name ops_lock for solaris, due to OS conflict
**	5-aug-93 (ed)
**	    fixed casting error
**	01-sep-93 (jhahn)
**	    Added ops_psession_id and ops_session_ctr for maintaining server
**	    lifetime unique id's.
**	25-oct-93 (vijay)
**	    cast opf_sid to CS_SID.
**	8-may-94 (ed)
**	    - b59537 - OP130 non-critical consistency checks skipped
**      19-dec-94 (sarjo01) from 15-sep-94 (johna)
**          initialize dmt_mustlock (added by markm while fixing bug (58681)
**      06-mar-96 (nanpr01)
**          initialize the scan block with default since distributed does 
**	    not have DMF. Also initialize the scan block for different
**	    buffer pools.
**	18-june-99 (inkdo01)
**	    Added ops_default_user for temp table model histogram feature.
**	26-Feb-2001 (jenjo02)
**	    Provide session id to RDF.
**       6-Oct-2008 (hanal04) Bug 120798
**          Explicitly cast assignment of dmt_s_estimated_records and
**          dmt_s_width to ensure the full width of the field is set.
**      25-Nov-2008 (hanal04) Bug 121248
**          Initialise rdf_info_blk and caller_ref in rdfcb.
**	4-Dec-2008 (kschendel) b122118
**	    opx_verror is not callable here, crashes the server.  Use
**	    opx_rverror to report any DMF call error.
**	09-Nov-2010 (wanfr01) SIR 124714
** 	    adjust ops_scanblocks based on ops_holdfactor to allow holdfactor
**	    tuning (to allow disk i/o estimate tuning)
[@history_line@]...
*/
DB_STATUS
ops_bgn_session(
	OPF_CB             *opf_cb)
{
    OPS_CB          *ops_cb;
    DB_STATUS       status;
    OPG_CB	    *opg_cbp;
    i4		    i;
    
    ops_cb = (OPS_CB *)(opf_cb->opf_scb); /* get ptr to session control block 
                                     */
    ops_cb->ops_state = NULL;        /* set the state pointer to NULL to
                                     ** indicate that an optimization is not
                                     ** occuring
                                     */
    /* ops_retstatus - was initialized prior to enabling the exception handler*/
    ops_cb->ops_udbid = opf_cb->opf_udbid; /* save unique database id */
    ops_cb->ops_dbid = opf_cb->opf_dbid; /* save database id for this session */
    ops_cb->ops_sid = (CS_SID) opf_cb->opf_sid; /* save session id */
    ops_cb->ops_adfcb = opf_cb->opf_adfcb; /* save ptr to ADF control block */
    ops_cb->ops_interrupt = FALSE;   /* reset the interrupt flag which 
                                     ** is TRUE iff an interrupt occurs
                                     */
    ops_cb->ops_check = FALSE;       /* set the trace flag indicator which is
                                     ** TRUE iff at least one trace flag is set
                                     */
    ops_cb->ops_skipcheck = FALSE;   /* TRUE if non-critical consistency checks
				     ** should be skipped */
    ops_cb->ops_server = 
    opg_cbp = (OPG_CB*)opf_cb->opf_server; /* save server control block ptr */
    ops_cb->ops_coordinator = (DD_1LDB_INFO *)opf_cb->opf_coordinator; 
				    /* save ptr to coordinator
				    ** descriptor for this session */

    /* Copy case translation flags and catalog owner name into session cb */
    ops_cb->ops_dbxlate = opf_cb->opf_dbxlate;
    ops_cb->ops_cat_owner = opf_cb->opf_cat_owner;
    ops_cb->ops_session_ctr = 0;
    STRUCT_ASSIGN_MACRO(opf_cb->opf_default_user, ops_cb->ops_default_user);
    
    status = ops_exlock(opf_cb, &opg_cbp->opg_semaphore);
    if (DB_FAILURE_MACRO(status))
	return(status);		    /* lock server control block */

    STRUCT_ASSIGN_MACRO(opg_cbp->opg_alter, ops_cb->ops_alter); /*
                                     ** copy initial
                                     ** characteristics for a new session */
    if (opf_cb->opf_smask & OPF_SDISTRIBUTED)
    {
	ops_cb->ops_smask = OPS_MDISTRIBUTED; /* true for distributed thread */
	if (!opg_cbp->opg_first)
	{
	    /* get the distributed cost info, note that conceptually this
	    ** call should be made at server startup, but RDF needs a thread
	    ** in order to send queries to the iidbdb, and this is not available at
	    ** server startup, so the first thread to be started will 
	    ** initialize this */
	    RDF_CB		rdfcb;		/* RDF info control block */
	    DB_STATUS	rdf1status;

            rdfcb.rdf_info_blk = NULL;
            rdfcb.caller_ref = (RDR_INFO **)NULL;

	    rdfcb.rdf_rb.rdr_fcb = opg_cbp->opg_rdfhandle; /* only the memory manager
					    ** handle is needed for this server
					    ** level call */
	    rdfcb.rdf_rb.rdr_r1_distrib = DB_3_DDB_SESS; /*
					    ** request cluster information request*/
	    rdfcb.rdf_rb.rdr_r2_ddb_req.rdr_d1_ddb_masks = RDR_DD9_CLUSTERINFO; /*
					    ** request cluster information request*/
	    rdfcb.rdf_rb.rdr_session_id = ops_cb->ops_sid;


	    rdf1status = rdf_call(RDF_DDB, (PTR) &rdfcb); /* get the DDB
					    ** info for the server */
# ifdef E_OP0087_RDF_INITIALIZE
	    if (DB_FAILURE_MACRO(rdf1status))
	    {
		opx_rverror( opf_cb, rdf1status, E_OP0087_RDF_INITIALIZE, 
			     rdfcb.rdf_error.err_code);
		return(E_DB_ERROR);
	    }
# endif
	    opg_cbp->opg_cluster = rdfcb.rdf_rb.rdr_r2_ddb_req.rdr_d9_cluster_p; /*
					    ** nodes in an ingres cluster */
	    opg_cbp->opg_nodelist = rdfcb.rdf_rb.rdr_r2_ddb_req.rdr_d10_node_p; /*
					    ** relative node speeds/info */
	    opg_cbp->opg_netlist = rdfcb.rdf_rb.rdr_r2_ddb_req.rdr_d11_net_p; /*
					    ** relative network speeds */
	}
    }
    else
	ops_cb->ops_smask = 0;
    opg_cbp->opg_first = TRUE;		    /* initialize the control block
					    ** when first thread is ready  */
    ops_cb->ops_psession_id = opg_cbp->opg_psession_id++; /* get session
							  ** number
							  */
    status = ops_unlock(opf_cb, &opg_cbp->opg_semaphore);
    if (DB_FAILURE_MACRO(status))
	return(status);			    /* unlock server control block */

    if (opf_cb->opf_smask & OPF_RESOURCE)
	ops_cb->ops_alter.ops_amask |= OPS_ARESOURCE; /* this will force enumeration
					    ** to be used for single variable queries
					    ** so that accurate estimates can be found */
    if (opf_cb->opf_smask & OPF_SDISTRIBUTED)
    {
	for (i = 0; i < DB_NO_OF_POOL; i++)
	  ops_cb->ops_alter.ops_scanblocks[i] = DB_HFBLOCKSIZE * 100 / (ops_cb->ops_alter.ops_holdfactor * DB_MINPAGESIZE);
					    /* since
					    ** distributed does not have a DMF
					    ** use a default scan factor */
    }
    else
    {	/* fetch current blocking factor from DMF */
	DMT_CB	dmt_cb;		/* DMF control block used to get cost*/
	DB_STATUS   dmf_status;     /* DMF return status */

	dmt_cb.type = DMT_TABLE_CB;
	dmt_cb.length = sizeof(DMT_CB);
	dmt_cb.dmt_flags_mask = 0;
	dmt_cb.dmt_s_estimated_records = (f4)1; 
	dmt_cb.dmt_s_width = (i4)1; 
        dmt_cb.dmt_mustlock = FALSE;
	EXmath(EX_OFF);
	dmf_status = dmf_call( DMT_SORT_COST, &dmt_cb);
	if (dmf_status != E_DB_OK)
	{
	    opx_rverror( opf_cb, dmf_status, E_OP048C_SORTCOST, 
		dmt_cb.error.err_code);
	    return (dmf_status);
	}
	for (i = 0; i < DB_NO_OF_POOL; i++)
	{
	  ops_cb->ops_alter.ops_scanblocks[i] = dmt_cb.dmt_io_blocking[i] * 100 / ops_cb->ops_alter.ops_holdfactor;
	  if (ops_cb->ops_alter.ops_scanblocks[i] < 1)
	      ops_cb->ops_alter.ops_scanblocks[i] = 1;
	}
				    /* fetch
				    ** current blocking factor from DMF */
    }

    ult_init_macro( &ops_cb->ops_tt, OPT_BMSESSION, OPT_SWVALUES, 
	OPT_SVALUES );		     /* init trace timing vector for session */

    status = opx_init(opf_cb);	    /* initialize the AIC and PAINE handler */
    if (DB_FAILURE_MACRO(status))
	return(status);
    return(E_DB_OK);
}
