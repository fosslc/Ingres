/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
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
#define        OPS_SHUTDOWN      TRUE
#include    <tr.h>
#include    <opxlint.h>


/**
**
**  Name: OPSSHUT.C - shutdown a server
**
**  Description:
**      Routine to support server shutdown for OPF
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
**	06-aug-1997 (canor01)
**	    Remove semaphore before releasing memory.
**	09-Oct-1998 (jenjo02)
**	    Removed SCF semaphore functions, inlining the CS calls instead.
[@history_line@]...
**/

/*{
** Name: ops_shutdown	- shutdown the server for the optimizer
**
** Description:
**      This routine will shutdown the server for the optimizer
**
** Inputs:
**      opf_cb                          ptr to optimizer control block
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
**	29-jun-86 (seputis)
**          initial creation
**	16-jan-91 (seputis)
**	    support for OPF active flag
**	06-aug-1997 (canor01)
**	    Remove semaphore before releasing memory.
**	09-Oct-1998 (jenjo02)
**	    Removed SCF semaphore functions, inlining the CS calls instead.
[@history_line@]...
*/
DB_STATUS
ops_shutdown(
	OPF_CB             *opf_cb)
{
    DB_STATUS              status;      /* ulm return status */
    OPG_CB		   *servercb;	/* server control block */

    servercb = (OPG_CB*)opf_cb->opf_server;
    {	/* RDF shutdown procedure */
	RDF_CCB                rdfcb;	/* control block used to initialize
                                        ** RDF */
        rdfcb.rdf_fcb = servercb->opg_rdfhandle; /* save 
                                        ** handle to be used for calls to RDF
                                        */
	rdfcb.rdf_fac_id = DB_OPF_ID;   /* optimizer's id for SCF */
        status = rdf_call(RDF_TERMINATE, (PTR)&rdfcb);
# ifdef E_OP0094_RDF_SHUTDOWN
	if (DB_FAILURE_MACRO(status))
	{
	    opx_rverror( opf_cb, status, E_OP0094_RDF_SHUTDOWN,
			 rdfcb.rdf_error.err_code);
	    return (E_DB_FATAL);
	}
# endif
    }

    if (servercb->opg_mask & OPG_CONDITION)
    {
	STATUS	    cs_status;

	/* Print out OPF statistics */
	TRdisplay("\nOPF Statistics Report\n");
	TRdisplay("Number of times a session has to wait  = %8d\n", 
	    servercb->opg_swait);
	if (servercb->opg_slength > servercb->opg_mxsess)
	    servercb->opg_slength -= servercb->opg_mxsess; /* thread only had to wait if
					** length became longer than maximum sessions
					** able to execute in OPF */
	else
	    servercb->opg_slength = 0;
	TRdisplay("Maximum length of the wait queue       = %8d\n", 
	    servercb->opg_slength);
	TRdisplay("Number of retrys                       = %8d\n", 
	    servercb->opg_sretry);
	TRdisplay("Maximum wait time of a thread          = %8d msec\n", 
	    servercb->opg_mwaittime);
	TRdisplay("Average wait time of threads which wait= %8d msec\n", 
	    (i4)servercb->opg_avgwait);

	cs_status = CScnd_free(&servercb->opg_condition);
# ifdef E_OP0099_CONDITION_FREE
	if (DB_FAILURE_MACRO(cs_status))
	{
	    opx_rverror( opf_cb, cs_status, E_OP0099_CONDITION_FREE,
                         (OPX_FACILITY)cs_status);
	    return(E_DB_ERROR);
	}
# endif
    }

    {
	ULM_RCB                ulmrcb;	/* control block used to call ULM */

	ulmrcb.ulm_facility = DB_OPF_ID;	/* optimizer's id */
	ulmrcb.ulm_poolid = servercb->opg_memory; /* memory pool of 
					    ** optimizer */
	status = ulm_shutdown(&ulmrcb);	/* deallocate the ULM memory */
# ifdef E_OP0088_ULM_SHUTDOWN
	if (DB_FAILURE_MACRO(status))
	{
	    opx_rverror( opf_cb, status, E_OP0088_ULM_SHUTDOWN,
			 ulmrcb.ulm_error.err_code);
	    return (E_DB_ERROR);
	}
# endif
    }
    {
	SCF_CB              scf_cb;
	DB_STATUS           scfstatus;

	/* first remove the semaphore before freeing the memory */
	CSr_semaphore(&servercb->opg_semaphore);

	/* Prepare to release server control block to SCF. */

	scf_cb.scf_length = sizeof(SCF_CB);
	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_session = DB_NOSESSION;
	scf_cb.scf_facility = DB_OPF_ID;
	scf_cb.scf_scm.scm_in_pages = (sizeof(OPG_CB) - 1)/SCU_MPAGESIZE + 1;
	scf_cb.scf_scm.scm_addr = (char *) servercb;

	/* Now ask SCF to free the memory. */
	scfstatus = scf_call(SCU_MFREE, &scf_cb);
# ifdef E_OP008B_SCF_SERVER_MEM
	if (DB_FAILURE_MACRO(scfstatus))
	{
	    opx_rverror( opf_cb, scfstatus, E_OP008B_SCF_SERVER_MEM,
                         scf_cb.scf_error.err_code);
	    return(E_DB_ERROR);
	}
# endif
    }
    return (E_DB_OK);
}

