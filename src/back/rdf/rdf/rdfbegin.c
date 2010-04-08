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
#include	<me.h>
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
**  Name: RDFBEGIN.C - Begin an RDF session
**
**  Description:
**      This file contains the rdf_bgn_session function which is called
**	by SCF to begin an RDF session.
**
**          rdf_bgn_session - begin an RDF session.
**
**
**  History:
**      02-mar-87 (puree)
**          initial version.
**      03-sep-88 (seputis)
**          resource sharing change
**	31-aug-90 (teresa)
**	    add logic to bypass checking iisynonym catalog.
**	20-dec-91 (teresa)
**	    added include of header files ddb.h, rqf.h and rdfddb.h for sybil
**	08-dec-92 (anitap)
**	    Included <qsf.h> for CREATE SCHEMA.
**	07-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Initialize rds_dbxlate with address of case translation mask.
**	    Initialize rds_cat_owner with address of catalog owner name.
**	06-jul-93 (teresa)
**	    change include of dbms.h to include of gl.h, sl.h, iicommon.h &
**	    dbdbms.h.  Also added include of me.h.
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
**/

/*{
** Name: rdf_bgn_session - begin an RDF session.
**
**	External call format:	status = rdf_call(RDF_BGN_SESSION, &rdf_ccb)
**
** Description:
**	This routine used to set the global variable Rdf_svcb to the RDF 
**	server control block pointer in rdf_ccb.rdf_server.  However, RDF
**	has been modified to stop using the global variable Rdf_svcb and get
**	the RDF Server Control Block from SCF when it needs it.  
**
**	The only thing that this routine currently does is check for info
**	passed in the RDF_CCB.rdf_flags_mask, and set flags in RDF_SVCB as
**	appropriate.
**	
** Inputs:
**      rdf_ccb
**	        .rdf_server		ptr to server control block.
**
** Outputs:
**      rdf_ccb                          
**					
**		.rdf_error              Filled with error if encountered.
**
**			.err_code	One of the following error numbers.
**				E_RD0000_OK
**					Operation succeed.
**				E_RD0003_BAD_PARAMETER
**					Bad pointer to RDF server block.
**
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_ERROR			Function failed due to error by caller;
**					error status in rdf_ccb.rdf_error.
**					    E_RD0279_NO_SESSION_CONTROL_BLK
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	02-mar-87 (puree)
**	    initial version.
**	16-jan-90 (teresa)
**	    move most of this to server startup because there is only 1 cache
**	    instead of one for each facility.
**	20-jan-91 (teresa)
**	    add logic to set flag to conditionally bypass checking iisynonym 
**	    catalog
**	10-feb-92 (teresa)
**	    drop in the following for SYBIL:
**	    05-apr-89 (mings)
**		added session control infomation to CCB.
**	29-jul-92 (teresa)
**	    initialize the RDF_SESS_CB.  Note that RDF_CCB.rdf_strace moved to
**	    RDF_SESS_CB.rds_strace and RDF_CCB.rdf_ddb_id moved to 
**	    RDF_SESS_CB.rds_ddb_id.  RDF_SESS_CB is obtained from a call to 
**	    rdu_gcb().
**	30-mar-93 (teresa)
**	    move svcb->rdv_special_condt to session control block as 
**	    rdf_sess_cb->rds_special_condt
**	07-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Initialize rds_dbxlate with address of case translation mask.
**	    Initialize rds_cat_owner with address of catalog owner name.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value RDF_CCB's rdf_error.
*/
DB_STATUS
rdf_bgn_session(    RDF_GLOBAL	*global,
		    RDF_CCB	*rdf_ccb)
{
	RDF_SESS_CB	*rdf_sess_cb;

	global->rdfccb = rdf_ccb;
	CLRDBERR(&rdf_ccb->rdf_error);

	/* rdf_call() has stuck *RDF_SESS_CB into global */
	rdf_sess_cb = global->rdf_sess_cb;

	/* Set pointer to session's ADF_CB in session cb */
	rdf_sess_cb->rds_adf_cb = rdf_ccb->rdf_adf_cb;

	/* initialize session specific tracepoints */
        ult_init_macro(&rdf_sess_cb->rds_strace, RDF_NB, RDF_NVP, RDF_NVAO);

	/* preserve the Distributed Database ID */
	rdf_sess_cb->rds_ddb_id = rdf_ccb->rdf_ddb_id;
    
	/* indicate if this session is distributed */
	rdf_sess_cb->rds_distrib = (rdf_ccb->rdf_distrib & DB_3_DDB_SESS);

	rdf_sess_cb->rds_special_condt = RDS_NO_SPECIAL_CONDIT;

	/* save case translation mask and catalog owner name */
	rdf_sess_cb->rds_dbxlate = rdf_ccb->rdf_dbxlate;
	rdf_sess_cb->rds_cat_owner = rdf_ccb->rdf_cat_owner;

	/* see if there is anything to do. */
	if (rdf_ccb->rdf_flags_mask)
	{
	    /* see if we need to suppress using iisynonyms catalog */
	    if (rdf_ccb->rdf_flags_mask & RDF_CCB_SKIP_SYNONYM_CHECK)
	    {
		rdf_sess_cb->rds_special_condt |= RDS_DONT_USE_IISYNONYM;
	    }
	}

	return (E_DB_OK);
}
