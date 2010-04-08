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
#define             OPX_TMROUTINES      TRUE
#define		    OPS_DEALLOCATE      TRUE
#include    <me.h>
#include    <bt.h>
#include    <tm.h>
#include    <opxlint.h>


/**
**
**  Name: OPSDEALLOCATE.C - deallocate resources in OPF
**
**  Description:
**      Routine to deallocate resources inside OPF
**
**  History:    
**      30-jun-86 (seputis)    
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**      24-jan-95 (inkdo01)
**          add F023_NOCOMP check to F032_NOEX, to clean up QEF structs
**          (fixes 62908)
**	02-jun-1997 (shero03)
**	    Update the saved rdf_info_block after calling RDF.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
[@history_line@]...
**/

/*{
** Name: ops_qsfdestroy	- destroy a query plan
**
** Description:
**      Since the QP can be in several states, in order to destroy it
**	several error recovery paths may be needed. 
**
** Inputs:
**      global                          ptr to global state variable
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      13-apr-89 (seputis)
**          initial creation
[@history_template@]...
*/
static DB_STATUS
ops_qsfdestroy(
	OPS_STATE          *global)
{
    DB_STATUS	    qsfqpstatus;
    qsfqpstatus = qsf_call( QSO_DESTROY, &global->ops_qsfcb);
    if (DB_FAILURE_MACRO(qsfqpstatus))
    {
	/* attempt to get a lock on the object, since and error
	** may have occurred between the time the OPC released
	** the object and prior to returning to SCF */
	global->ops_qsfcb.qsf_lk_state = QSO_SHLOCK;
	qsfqpstatus = qsf_call( QSO_LOCK, &global->ops_qsfcb );
					/* ignore the error from
					** this routine since we
					** will attempt to delete the
					** object again anyway */

	qsfqpstatus = qsf_call( QSO_DESTROY, &global->ops_qsfcb);
    }
    return(qsfqpstatus);
}

/*{
** Name: ops_deallocate	- deallocate resources for an optimization
**
** Description:
**      This routine will deallocate the resources used for an optimization.
**      Resources include any memory requested from the optimizer memory pool
**      and any RDF cache objects which were locked in the global range
**      table
**
** Inputs:
**      global                          ptr to global state variable
**      report                          TRUE if errors should be reported
**                                      via the user's control block.
**      partial_dbp                     partial deallocation required for
**                                      statement within a procedure
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    memory resources released, RDF unfixed, 
**          QSF query tree memory released
**
** History:
**	29-jun-86 (seputis)
**          initial creation
**	8-nov-88 (seputis)
**          if no query run trace point is set then destroy the QP since
**	    SCF assumes optimizer cleans up after error
**	8-nov-88 (seputis)
**          turn off CPU accounting if was off originally
**	28-jan-91 (seputis)
**	    added support for OPF ACTIVE flag
**      20-jul-93 (ed)
**	    changed name ops_lock for solaris, due to OS conflict
**	29-jul-93 (andre)
**	    rdr_types_mask must be initialized (to RDR_RELATION) before
**	    calling RDF_UNFIX.  Otherwise RDF may end up complaining because we
**	    ask it to destroy a relation cache entry while RDR_PROCEDURE bit is
**	    set.
**	12-aug-93 (swm)
**	    Cast first parameter of CSaltr_session() to CS_SID to match
**	    revised CL interface specification.
**	02-Jun-1997 (shero03)
**	    Update the saved rdf_info_block after calling RDF.
**      02-Aug-2001 (hanal04) Bug 105360 INGSRV 1505
**          Plug the RDF memory leak introduced by inkdo01's new
**          function oph_temphist().
**	17-Dec-2003 (jenjo02)
**	    Added (CS_SID)NULL to CScnd_signal prototype.
**	6-Feb-2006 (kschendel)
**	    Fix some squirrely looking code that purported to avoid dangling
**	    references, but didn't really.  (No symptoms known.)
**	14-nov-2007 (dougi)
**	    Add support for cached dynamic query plans.
**	20-may-2008 (dougi)
**	    Add support for table procedures.
**	29-may-2009 (wanfr01)    Bug 122125
**	    Need to add dbid to cache_dynamic queries for db uniqueness
*/
VOID
ops_deallocate(
	OPS_STATE          *global,
	bool               report,
	bool               partial_dbp)
{
    DB_STATUS           finalstatus;	/* this status is returned to the user
                                        ** - it will contain the first error
                                        ** during resource deallocation
                                        */
    DB_ERROR            error;          /* error code from offending facility
                                        */

    finalstatus = E_DB_OK;
    error.err_code = 0;

    {    /* close any fixed RDF objects - deallocate prior to closing the
         ** global memory stream 
         */

	OPV_IGVARS             gvar;	    /* index into global range variable
                                            ** table */
	OPV_GRT                *gbase;	    /* ptr to base of array of ptrs
                                            ** to global range table elements
                                            */
	OPV_IGVARS	       maxgvar;	    /* number of global range table
                                            ** elements allocated
                                            */
	RDF_CB                 *rdfcb;	    /* ptr to rdf control block used
                                            ** unfix the relation info */
	OPV_GBMVARS            *rdfmap;     /* ptr to map of global range
                                            ** variables which have RDF info
                                            ** fixed */

	gbase = global->ops_rangetab.opv_base;
        maxgvar = global->ops_rangetab.opv_gv;
	rdfcb = &global->ops_rangetab.opv_rdfcb;
        rdfmap = &global->ops_rangetab.opv_mrdf;

	/*
	** rdr_types_mask needs to be initialized - since we will be unfixing
	** relation entries, RDR_RELATION seems like a good choice, although 0
	** would suffice as well
	*/
	rdfcb->rdf_rb.rdr_types_mask = RDR_RELATION;
	
	if (global->ops_cstate.opc_relation)
	{   /* OPC allocates a RDF descriptor for cursors so deallocate
            ** if this is the case */
	    DB_STATUS	   opcrdfstatus; /* RDF return status */

	    rdfcb->rdf_info_blk = global->ops_cstate.opc_relation;
	    opcrdfstatus = rdf_call( RDF_UNFIX, (PTR)rdfcb );
	    if ( (DB_FAILURE_MACRO(opcrdfstatus)) && (DB_SUCCESS_MACRO(finalstatus)) )
	    {
		finalstatus = opcrdfstatus;
		error.err_code = rdfcb->rdf_error.err_code;
	    }
	    global->ops_cstate.opc_relation = NULL;
	}
	if (maxgvar)
	{
	    for ( gvar = -1; 
		 (gvar = BTnext((i4)gvar, (char *)rdfmap, (i4)maxgvar)) >=0;)
	    {
		OPV_GRV             *gvarp;	    /* ptr to global range variable to
						** be deallocated */

		if	((gvarp = gbase->opv_grv[gvar]) /* NULL if not allocated */
		     &&
		     (gvarp->opv_relation)	    /* not NULL if RDF has been
						** called for this range variable */
		     &&
		     !(gvarp->opv_gmask & OPV_TPROC)	/* not table procedure */
		    )
		{
		    /* if this element has been allocated and if it has an RDF
		    ** cache element associated with it */
		    DB_STATUS	   rdfstatus; /* RDF return status */

		    gbase->opv_grv[gvar] = NULL; /* so we do not try to deallocate
						** twice in case of an error */
		    rdfcb->rdf_info_blk = gvarp->opv_relation;
		    rdfstatus = rdf_call( RDF_UNFIX, (PTR)rdfcb );
		    if ( (DB_FAILURE_MACRO(rdfstatus)) && (DB_SUCCESS_MACRO(finalstatus)) )
		    {
			finalstatus = rdfstatus;
			error.err_code = rdfcb->rdf_error.err_code;
		    }
		    gvarp->opv_relation = NULL;
		}

		if ((gvarp) && (gvarp->opv_ttmodel))
		{
		    /* if this element has been allocated and if it has an RDF
		    ** cache element associated with a persistent table
                    ** which provides histogram models.
		    */
		    DB_STATUS      rdfstatus; /* RDF return status */

		    rdfcb->rdf_info_blk = gvarp->opv_ttmodel;
		    gvarp->opv_ttmodel = NULL;
		    rdfstatus = rdf_call( RDF_UNFIX, (PTR)rdfcb );
		    if ( (DB_FAILURE_MACRO(rdfstatus)) && (DB_SUCCESS_MACRO(finalstatus)) )
		    {
		        finalstatus = rdfstatus;
	                error.err_code = rdfcb->rdf_error.err_code;
		    }
		    
		}

	    }
	    global->ops_rangetab.opv_gv = 0;
	}
    }
    if (partial_dbp)
	return;					/* only deallocate the global range table
						** for DBP, and keep the memory streams
						** until the end */
    if (global->ops_estate.opn_statistics
	&&
	global->ops_estate.opn_reset_statistics)
    {	/* statistics CPU accounting was turned on, and needs to be reset */
	STATUS		cs_status;
	i4		turn_off;

	turn_off = FALSE;			/* turn off accounting */
	global->ops_estate.opn_statistics = FALSE;
	cs_status = CSaltr_session((CS_SID)0, CS_AS_CPUSTATS, (PTR)&turn_off);
	if (cs_status != OK)
	{
	    finalstatus = E_DB_ERROR;
	    error.err_code = cs_status;
	}
    }

    /* deallocate ULM memory stream */
    if (global->ops_mstate.ops_streamid == NULL)	/* non-zero if allocated */
    {
	/* check if ULM stream does not exist then this deallocation has
        ** already occurred so just return */
        return;
    }
    else
    {
	DB_STATUS            ulm1status;/* ULM return status */
	global->ops_mstate.ops_ulmrcb.ulm_streamid_p = 
	    &global->ops_mstate.ops_streamid; /* ulm will NULL ops_streamid */
	ulm1status = ulm_closestream( &global->ops_mstate.ops_ulmrcb );
	if ( (DB_FAILURE_MACRO(ulm1status)) && (DB_SUCCESS_MACRO(finalstatus)) )
	{
	    finalstatus = ulm1status;
	    error.err_code = global->ops_mstate.ops_ulmrcb.ulm_error.err_code;
	}
    }

    /* deallocate ULM temp buffer memory stream */
    if ( global->ops_mstate.ops_tstreamid )	/* non-zero if allocated */
    {
	DB_STATUS            ulm2status; /* ULM return status */
	global->ops_mstate.ops_ulmrcb.ulm_streamid_p
	    = &global->ops_mstate.ops_tstreamid; /* ulm will NULL ops_tstreamid */
	ulm2status = ulm_closestream( &global->ops_mstate.ops_ulmrcb );
	if ( (DB_FAILURE_MACRO(ulm2status)) && (DB_SUCCESS_MACRO(finalstatus)) )
	{
	    finalstatus = ulm2status;
	    error.err_code = global->ops_mstate.ops_ulmrcb.ulm_error.err_code;
	}
    }

    /* deallocate OPC ULM buffer memory stream */
    if ( global->ops_mstate.ops_sstreamid )	/* non-zero if allocated */
    {
	DB_STATUS            ulm3status; /* ULM return status */
	global->ops_mstate.ops_ulmrcb.ulm_streamid_p = &global->ops_mstate.ops_sstreamid;
	/* ulm will NULL ops_sstreamid */
	ulm3status = ulm_closestream( &global->ops_mstate.ops_ulmrcb );
	if ( (DB_FAILURE_MACRO(ulm3status)) && (DB_SUCCESS_MACRO(finalstatus)) )
	{
	    finalstatus = ulm3status;
	    error.err_code = global->ops_mstate.ops_ulmrcb.ulm_error.err_code;
	}
    }

    if (!report
#ifdef    OPT_F032_NOEXECUTE
	||
	/* if trace flag is set then cleanup QSF memory since optimizer will
	** generate an error to SCF and SCF assumes optimizer will cleanup
        */
	    (global->ops_cb->ops_check 
	    && 
	    (opt_strace( global->ops_cb, OPT_F032_NOEXECUTE)
	    || 
	    opt_strace( global->ops_cb, OPT_F023_NOCOMP)  )
	    )
#endif
       )
    {	/* an error or an asychronous abort has occurred so destroy the plan
        ** or shared plan , FIXME destroy the shared plan in the earlier
        ** exception handler */
	DB_STATUS	       qsfqpstatus;   /* QSF return status */
	if(global->ops_qpinit)
	{   /* deallocate QSF object for query plan if another error has occurred 
            ** - in this case OPC has already created a new QP handle and has
            ** gotten a lock on it */

	    STRUCT_ASSIGN_MACRO(global->ops_caller_cb->opf_qep, global->ops_qsfcb.qsf_obj_id); /* get
						** query plan id */
	    global->ops_qsfcb.qsf_lk_id = global->ops_qplk_id; /* get lock id for 
						** QSF */
	    qsfqpstatus = ops_qsfdestroy(global); /* destroy the query plan */
	    if ( (DB_FAILURE_MACRO(qsfqpstatus)) && (DB_SUCCESS_MACRO(finalstatus)) )
	    {
		finalstatus = qsfqpstatus;
		error.err_code = global->ops_qsfcb.qsf_error.err_code;
	    }
	}
	else 
	{   /* OPC has not been reached so need to check for shared query plan
            */
	    if (!global->ops_procedure)
	    {	/* get query tree if it has not already been retrieved */
		qsfqpstatus = ops_gqtree(global);
		if ( (DB_FAILURE_MACRO(qsfqpstatus)) && (DB_SUCCESS_MACRO(finalstatus)) )
		{
		    finalstatus = qsfqpstatus;
		    error.err_code = global->ops_qsfcb.qsf_error.err_code;
		}
	    }
	    if (global->ops_qheader && 
			(global->ops_qheader->pst_mask1 & PST_RPTQRY))
	    {	/* shared query plan possible */
		if (global->ops_procedure->pst_flags & PST_REPEAT_DYNAMIC)
		{
		    char	*p;

		    global->ops_qsfcb.qsf_obj_id.qso_lname = 
						sizeof(DB_CURSOR_ID) + sizeof(i4);
		    MEfill(sizeof(global->ops_qsfcb.qsf_obj_id.qso_name), 0,
			global->ops_qsfcb.qsf_obj_id.qso_name);
		    MEcopy((PTR)&global->ops_procedure->
						pst_dbpid.db_cursor_id[0],
			sizeof (global->ops_procedure->
						pst_dbpid.db_cursor_id[0]),
			(PTR)global->ops_qsfcb.qsf_obj_id.qso_name);
		    p = (char *) global->ops_qsfcb.qsf_obj_id.qso_name + 
								2*sizeof(i4);
		    MEcopy((PTR)"qp", sizeof("qp"), p);
		    p = (char *) global->ops_qsfcb.qsf_obj_id.qso_name + 
						sizeof(DB_CURSOR_ID);
		    I4ASSIGN_MACRO(global->ops_caller_cb->opf_udbid, *(i4 *) p);
		    
		}
		else	/* must be proc or regular repeat query */
		{
		    global->ops_qsfcb.qsf_obj_id.qso_lname = 
				sizeof (global->ops_procedure->pst_dbpid);
		    MEcopy((PTR)&global->ops_procedure->pst_dbpid, 
			    sizeof (global->ops_procedure->pst_dbpid),
			    (PTR)&global->ops_qsfcb.qsf_obj_id.qso_name[0]);
		}
		global->ops_qsfcb.qsf_obj_id.qso_type = QSO_QP_OBJ;
		global->ops_qsfcb.qsf_lk_state = QSO_SHLOCK;
		qsfqpstatus = qsf_call(QSO_GETHANDLE, &global->ops_qsfcb);
		if (DB_SUCCESS_MACRO(qsfqpstatus))
		{
		    qsfqpstatus = ops_qsfdestroy( global );
		    if ( (DB_FAILURE_MACRO(qsfqpstatus)) && (DB_SUCCESS_MACRO(finalstatus)) )
		    {
			finalstatus = qsfqpstatus;
			error.err_code = global->ops_qsfcb.qsf_error.err_code;
		    }
		}
		else if (global->ops_qsfcb.qsf_error.err_code != E_QS0019_UNKNOWN_OBJ)
		{   /* if object is not found then this is not a shared query */
		    finalstatus = qsfqpstatus;
		    error.err_code = global->ops_qsfcb.qsf_error.err_code;
		}
	    }
	}	
    }

    /* release QSF memory allocated to query tree, make sure that this
    ** is done after the QP has been processed since pst_rptqry is still
    ** needed for above block */
    {
	DB_STATUS	       qsfstatus;   /* QSF return status */

	STRUCT_ASSIGN_MACRO(global->ops_caller_cb->opf_query_tree, global->ops_qsfcb.qsf_obj_id); /* get
					    ** query tree id */
        global->ops_qsfcb.qsf_lk_id = global->ops_lk_id; /* get lock id for 
                                            ** QSF */
	qsfstatus = ops_qsfdestroy( global );
	if ( (DB_FAILURE_MACRO(qsfstatus)) && (DB_SUCCESS_MACRO(finalstatus)) )
	{
	    finalstatus = qsfstatus;
	    error.err_code = global->ops_qsfcb.qsf_error.err_code;
	}
    }

    /* signal that the session is exiting OPF and that another thread may enter */
    if (global->ops_cb->ops_smask & OPS_MCONDITION)
    {
	DB_STATUS   lockstatus;
	OPG_CB	    *servercb;

	servercb = global->ops_cb->ops_server;
	global->ops_cb->ops_smask &= (~OPS_MCONDITION);
	lockstatus = ops_exlock(global->ops_caller_cb, &servercb->opg_semaphore); 
					    /* check if server
					    ** thread is available, obtain
					    ** semaphore lock on critical variable */
	servercb->opg_activeuser--;	    /* since exit is about to occur, and memory
					    ** has already been deallocated, allow another
					    ** user to enter OPF */
	servercb->opg_waitinguser--;	    /* since exit is about to occur, and memory
					    ** has already been deallocated, allow another
					    ** user to enter OPF */
	if (DB_FAILURE_MACRO(lockstatus) && (DB_SUCCESS_MACRO(finalstatus)))
	{
	    finalstatus = lockstatus;
	    error.err_code = global->ops_caller_cb->opf_errorblock.err_data;
	}
	else
	{
	    if (servercb->opg_waitinguser > servercb->opg_activeuser)
	    {
		STATUS	    csstatus;
		csstatus = CScnd_signal(&servercb->opg_condition, (CS_SID)NULL);
						/* signal only if some users are waiting */
		if ((csstatus != OK) && (DB_SUCCESS_MACRO(finalstatus)))
		{
		    finalstatus = E_DB_ERROR;
		    error.err_code = csstatus;
		}
	    }
	    lockstatus = ops_unlock(global->ops_caller_cb, 
		&servercb->opg_semaphore);	/* check if server
						** thread is available */
	    if (DB_FAILURE_MACRO(lockstatus) && (DB_SUCCESS_MACRO(finalstatus)))
	    {
		finalstatus = lockstatus;
		error.err_code = global->ops_caller_cb->opf_errorblock.err_data;
	    }
	}
    }

    if (DB_FAILURE_MACRO(finalstatus))
    {
	if (report)
	    opx_verror( finalstatus, E_OP0084_DEALLOCATION, error.err_code); /* report
					    ** error and generate an exception */
	else
	    opx_rverror(global->ops_cb->ops_callercb, finalstatus,
		E_OP0084_DEALLOCATION, error.err_code);
					    /* report error only but do not generate an
					    ** exception */
    }
}
