/*
**Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include	<sl.h>
#include	<iicommon.h>
#include	<dbdbms.h>
#include	<ddb.h>
#include	<ulf.h>
#include	<ulm.h>
#include	<cs.h>
#include	<scf.h>
#include	<ulh.h>
#include	<adf.h>
#include	<dmf.h>
#include	<dmtcb.h>
#include	<dmrcb.h>
#include	<qefmain.h>
#include        <qsf.h>
#include	<qefrcb.h>
#include	<qefqeu.h>
#include	<rdf.h>
#include	<rqf.h>
#include	<rdfddb.h>
#include	<ex.h>
#include	<rdfint.h>

/**
**
**  Name: RDFTERM.C - Terminate RDF for a facility.
**
**  Description:
**	This file contains the routine for terminate RDF for a facility. 
**
**	rdf_terminate - Terminate RDF for a facility.
**
**
**  History:
**      28-mar-86 (ac)    
**          written
**	12-jan-87 (puree)
**	    add a call to ULH to close the hash table for the calling
**	    facility.
**	09-may-89 (neil)
**	    Add stats for rule gathering.
**	17-Oct-89 (teg)
**	    change logic to get memleft from Server Control Block, since RDF
**	    now allocates its own memory.  This is part of the fix for bug 5446.
**	27-oct-89 (neil)
**	    Rules - lint field names.
**	    Alerters - Dump event diagnostics.
**	13-feb-90 (teg)
**	    remove call to ulh_close() -- this is now handled in rdf_terminate.
**	31-aug-90 (teresa)
**	    pick up 6.3 bugfix 4390.
**	20-dec-91 (teresa)
**	    added include of header files ddb.h, rqf.h and rdfddb.h for sybil
**	08-dec-92 (anitap)
**	    Added include of header file qsf.h for CREATE SCHEMA.
**	06-jul-93 (teresa)
**	    change include of dbms.h to include of gl.h, sl.h, iicommon.h &
**	    dbdbms.h.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    RDI_SVCB *svcb is now a GLOBALREF, added macros 
**	    GET_RDF_SESSCB macro to extract *RDF_SESSCB directly
**	    from SCF's SCB, added (ADF_CB*)rds_adf_cb,
**	    deprecating rdu_gcb(), rdu_adfcb(), rdu_gsvcb()
**	    functions.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value DB_ERROR structure.
**	    Pass pointer to facilities DB_ERROR instead of just its err_code
**	    in rdu_ferror().
**/

/*{
** Name: rdf_terminate - Terminate RDF for a facility.
**
**	External call format:	status = rdf_call(RDF_TERMINATE, &rdf_ccb);
**
** Description:
**	This function terminates RDF for a facility and release the memory
**	used for storing the facility control block.
**	
** Inputs:
**
**      rdf_ccb                          
**		.rdf_fcb		A pointer to the internal structure
**					which contains the poolid of the 
**					memory pool where all the table
**					information can be released.
**					
** Outputs:
**      rdf_ccb                          
**					
**		.rdf_error              Filled with error if encountered.
**
**			.err_code	One of the following error numbers.
**				E_RD0000_OK
**					Operation succeed.
**				E_RD0006_MEM_CORRUPT
**					Memory pool is corrupted.
**				E_RD0007_MEM_NOT_OWNED
**					Memory pool is not owned by the calling
**					facility.
**				E_RD0008_MEM_SEMWAIT
**					Error waiting for exclusive access of 
**					memory.
**				E_RD0009_MEM_SEMRELEASE
**					Error releasing exclusive access of 
**					memory.
**				E_RD000A_MEM_NOT_FREE
**					Error freeing memory.
**				E_RD000B_MEM_ERROR
**					Whatever memory error not mentioned 
**					above.
**				E_RD0003_BAD_PARAMETER
**					Bad input parameters.
**				E_RD0020_INTERNAL_ERR
**					RDF internal error.
**
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_ERROR			Function failed due to error by caller;
**					error status in rdf_ccb.rdf_error.
**	    E_DB_FATAL			Function failed due to some internal
**					problem; error status in
**					rdf_ccb.rdf_error.
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	28-mar-86 (ac)
**          written
**	02-jan-87 (puree)
**	    modified for cache version.
**	24-mar-87 (puree)
**	    delete FCB and info blocks on facility termination.
**	09-may-89 (neil)
**	    Add stats for rule gathering.
**	17-Oct-89 (teg)
**	    change logic to get memleft from Server Control Block, since RDF
**	    now allocates its own memory.  This is part of the fix for bug 5446.
**	27-oct-89 (neil)
**	    Rules - lint field names.
**	    Alerters - Dump event diagnostics.
**	13-feb-90 (teg)
**	    remove call to ulh_close() -- this is now handled in rdf_terminate.
**      18-apr-90 (teg)
**          rdu_ferror interface changed as part of fix for bug 4390.
**	31-jan-91 (teresa)
**	    remove statistics reporting.  They are now reported in 
**	    rdfshut.c at server shutdown. This is moved because separate
**	    PSF and RDF statistics are no longer kept.  Instead, joint
**	    statistics are kept in the SVCB.
**	23-jan-92 (teresa)
**	    SYBIL CHANGES:
**		rdv_c1_memleft->rdv_memleft; rdi_poolid->rdv_poolid
**	16-jul-92 (teresa)
**	    prototypes
**	30-jul-97 (nanpr01)
**	    Initialize the streamid to NULL after deallocation.
**	07-Aug-1997 (jenjo02)
**	    Initialize the streamid to NULL BEFORE deallocation.
**	    Also destroy FCB pointer in rdf_ccb.
*/
DB_STATUS
rdf_terminate(	RDF_GLOBAL	*global,
		RDF_CCB		*rdf_ccb)
{
    DB_STATUS		status;
    RDI_FCB		*rdi_fcb;
    ULM_RCB		ulm_rcb;
    DB_STATUS		temp_status;

    status = E_DB_OK;
    rdi_fcb = (RDI_FCB *)(rdf_ccb->rdf_fcb);
    global->rdfccb = rdf_ccb;
    CLRDBERR(&rdf_ccb->rdf_error);

    /* Check input parameter. */

    if (rdi_fcb == NULL)
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return(status);
    }

    /*
    ** Release FCB and user's info blocks.
    */
    ulm_rcb.ulm_poolid = Rdi_svcb->rdv_poolid;
    ulm_rcb.ulm_memleft = &Rdi_svcb->rdv_memleft;
    ulm_rcb.ulm_facility = DB_RDF_ID;
    ulm_rcb.ulm_streamid_p = &rdi_fcb->rdi_streamid;
    ulm_rcb.ulm_blocksize = 0;
    rdf_ccb->rdf_fcb = (PTR)NULL;

    temp_status = ulm_closestream(&ulm_rcb);
    if (DB_FAILURE_MACRO(temp_status))
    {
	status = temp_status;
	rdu_ferror(global, status, &ulm_rcb.ulm_error, 
	    E_RD012B_ULM_CLOSE, 0);
    }
    /* ULM has nullified rdi_streamid */
    return(status);	
}
