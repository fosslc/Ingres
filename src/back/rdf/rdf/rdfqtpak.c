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
#include        <cs.h>
#include	<scf.h>
#include	<ulm.h>
#include	<ulh.h>
#include	<adf.h>
#include	<dmf.h>
#include	<dmtcb.h>
#include	<dmrcb.h>
#include	<qsf.h>
#include	<qefrcb.h>
#include	<qefqeu.h>
#include	<rqf.h>
#include	<rdf.h>
#include	<rdfddb.h>
#include	<ex.h>
#include	<rdfint.h>

/**
**
**  Name: RDFQTPACK.C - handle external requests to pack query text/tree tuples
**
**  Description:
**	This file contains functions that handle external requests to pack query
**	text/query tree tuples.  Bulk of the work will continue to be performed
**	by rdu_treenode_to_tuple() and rdu_qrytext_to_tuple() - the functions in
**	this module will simply verify that the caller has passed reasonably
**	looking parameters and invoke appropriate function to do the actual
**	work.
**
**	rdf_qtext_to_tuple  - handle request to pack query text tuples
**	rdf_qtree_to_tuple  - handle request to pack query tree tuples
**
**  History:
**	07-jan-93 (andre)
**	    written
**	06-jul-93 (teresa)
**	    change include of dbms.h to include of gl.h, sl.h, iicommon.h &
**	    dbdbms.h.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) to that ULM can
**	    destroy the handle when the memory is freed.
**	    Removed check of caller-supplied poolid & streamid - 
**	    ULM now sanity checks the handles instead of SEGVing.
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
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

/*
** Name: rdf_qtext_to_tuple - handle a request to pack query text into IIQRYTEXT
**			      format
**
**
**	External call format:   status = rdf_call(RDF_PACK_QTEXT, &rdf_cb);
**	
** Description:
**	This function handles external request to pack query text into
**	IIQRYTEXT format.  It will verify input parameters and call
**	rdu_qrytext_to_tuple() to do the actual work.
**
** Input:
**
**	rdf_cb
**	    .rdf_rb
**		.rdr_db_id		    Database id.
**		.rdr_session_id		    Session id.
**		.rdr_l_querytext	    Length of query text to pack
**		.rdr_querytext		    Query text to pack
**		.rdr_status		    Status word for qrytext
**		    DBQ_WCHECK		    View created WITH CHECK OPTION
**		    DBQ_SQL		    SQL view
**		.rdr_instr		    special intructions for RDF
**		    RDF_USE_CALLER_ULM_RCB  use caller's ULM_RCB
**		.rdf_fac_ulmrcb		    caller's ULM_RCB
**		    .ulm_poolid		    pool id (must be non-NULL)
**		    .ulm_streamid	    stream id (must be non-NULL)
**		    .ulm_memleft	    memory left
**		    .ulm_facility	    owner of memory stream
**			DB_OPF_ID	    OPF
**			DB_QEF_ID	    QEF
**		.rdr_qt_pack_rcb	    Additional info for packing
**		    .rdf_qt_tuples	    List of IIQRYTEXT tuples will go
**					    here
**		    .rdf_qt_tuple_count	    Number of IIQRYTEXT tuples will go
**					    here
**		    .rdf_qt_mode	    Query mode
**			DB_VIEW		    Packing view text
**			DB_INTG		    Packing integrity text
**			DB_PROT		    Packing permit text
**
**  Outputs:
**	rdf_cb
**	    .rdf_rb
**		.rdr_qt_pack_rcb
**		    .rdf_qt_tuples          List of IIQRYTEXT tuples
**		    .rdf_qt_tuple_count     Number of IIQRYTEXT tuples
**	    .rdf_error			    Filled with error if encountered.
**
**  Returns:
**
**	E_DB_OK				    Function completed normally.
**	E_DB_ERROR			    Function failed due to error by
**					    caller; error status in
**					    rdf_cb.rdf_error.
**	E_DB_FATAL			    Function failed due to some internal
**					    problem; error status in
**					    rdf_cb.rdf_error.
**
**  Exceptions:
**	none
**
**  Side Effects:
**	Memory's allocated from the user's pool
**
**  History:
**	07-jan-93 (andre)
**	    written
*/
DB_STATUS
rdf_qtext_to_tuple( RDF_GLOBAL      *global,
		    RDF_CB          *rdfcb)
{
    DB_STATUS		status;
    RDR_RB		*rdf_rb = &rdfcb->rdf_rb;
    RDF_QT_PACK_RCB	*pack_rcb;
    ULM_RCB		*ulm_rcb;
    RDD_QDATA		qrydata;
    
    /*
    ** set up pointer to rdf_cb in global for subordinate
    ** routines.  The rdfcb points to the rdr_rb, which points to the rdr_fcb.
    */

    global->rdfcb = rdfcb;
    
    CLRDBERR(&rdfcb->rdf_error);

    if (   rdf_rb->rdr_db_id == NULL
	|| rdf_rb->rdr_session_id == (CS_SID) 0
	|| rdf_rb->rdr_l_querytext <= 0
	|| rdf_rb->rdr_querytext == NULL
	|| rdf_rb->rdr_status & ~(DBQ_WCHECK | DBQ_SQL)
	|| ~rdf_rb->rdr_instr & RDF_USE_CALLER_ULM_RCB
	|| (ulm_rcb = (ULM_RCB *) rdf_rb->rdf_fac_ulmrcb) == (ULM_RCB *) NULL
	|| (   ulm_rcb->ulm_facility != DB_OPF_ID
	    && ulm_rcb->ulm_facility != DB_QEF_ID
	   )
	|| (pack_rcb = rdf_rb->rdr_qt_pack_rcb) == (RDF_QT_PACK_RCB *) NULL
	|| pack_rcb->rdf_qt_tuples == (QEF_DATA **) NULL
	|| pack_rcb->rdf_qt_tuple_count == (i4 *) NULL
	|| (   pack_rcb->rdf_qt_mode != DB_VIEW
	    && pack_rcb->rdf_qt_mode != DB_INTG
	    && pack_rcb->rdf_qt_mode != DB_PROT
	   )
       )
    {
	rdu_ierror(global, E_DB_ERROR, E_RD0003_BAD_PARAMETER);
	return(E_DB_ERROR);
    }

    qrydata.qrym_data = (QEF_DATA *) NULL;
    qrydata.qrym_cnt  = 0;

    status = rdu_qrytext_to_tuple(global, pack_rcb->rdf_qt_mode, &qrydata);
    if (DB_FAILURE_MACRO(status))
	return(status);

    /* tell caller where to find tuples and how many there are */
    *pack_rcb->rdf_qt_tuples = qrydata.qrym_data;
    *pack_rcb->rdf_qt_tuple_count = qrydata.qrym_cnt;

    return(E_DB_OK);
}

/*
** Name: rdf_qtree_to_tuple - handle a request to pack query tree into IITREE
**			      format
**
**
**	External call format:   status = rdf_call(RDF_PACK_QTREE, &rdf_cb);
**	
** Description:
**	This function handles external request to pack query tree into
**	IITREE format.  It will verify input parameters and call
**	rdu_treenode_to_tuple() to do the actual work.
**
** Input:
**
**	rdf_cb
**	    .rdf_rb
**		.rdr_db_id		    Database id.
**		.rdr_session_id		    Session id.
**		.rdr_qry_root_node	    root of the query tree to pack
**		.rdr_instr		    special intructions for RDF
**		    RDF_USE_CALLER_ULM_RCB  use caller's ULM_RCB
**		.rdf_fac_ulmrcb		    caller's ULM_RCB
**		    .ulm_poolid		    pool id (must be non-NULL)
**		    .ulm_streamid	    stream id (must be non-NULL)
**		    .ulm_memleft	    memory left
**		    .ulm_facility	    owner of memory stream
**			DB_OPF_ID	    OPF
**			DB_QEF_ID	    QEF
**		.rdr_qt_pack_rcb	    Additional info for packing
**		    .rdf_qt_tuples	    List of IITREE tuples will go
**					    here
**		    .rdf_qt_tuple_count	    Number of IITREE tuples will go
**					    here
**		    .rdf_qt_mode	    Query mode
**			DB_VIEW		    Packing view tree 
**			DB_INTG		    Packing integrity tree 
**			DB_RULE		    Packing rule tree
**
**  Outputs:
**	rdf_cb
**	    .rdf_rb
**		.rdr_qt_pack_rcb
**		    .rdf_qt_tuples          List of IITREE tuples
**		    .rdf_qt_tuple_count     Number of IITREE tuples
**	    .rdf_error			    Filled with error if encountered.
**
**  Returns:
**
**	E_DB_OK				    Function completed normally.
**	E_DB_ERROR			    Function failed due to error by
**					    caller; error status in
**					    rdf_cb.rdf_error.
**	E_DB_FATAL			    Function failed due to some internal
**					    problem; error status in
**					    rdf_cb.rdf_error.
**
**  Exceptions:
**	none
**
**  Side Effects:
**	Memory's allocated from the user's pool
**
**  History:
**	07-jan-93 (andre)
**	    written
**	26-feb-1997 (nanpr01)
**	    Alter Table drop column project.
*/
DB_STATUS
rdf_qtree_to_tuple( RDF_GLOBAL      *global,
		    RDF_CB          *rdfcb)
{
    DB_STATUS		status;
    RDR_RB		*rdf_rb = &rdfcb->rdf_rb;
    RDF_QT_PACK_RCB	*pack_rcb;
    ULM_RCB		*ulm_rcb;
    RDD_QDATA		qrydata;
    
    /*
    ** set up pointer to rdf_cb in global for subordinate
    ** routines.  The rdfcb points to the rdr_rb, which points to the rdr_fcb.
    */

    global->rdfcb = rdfcb;
    
    CLRDBERR(&rdfcb->rdf_error);

    if (   rdf_rb->rdr_db_id == NULL
	|| rdf_rb->rdr_session_id == (CS_SID) 0
	|| rdf_rb->rdr_qry_root_node == NULL
	|| ~rdf_rb->rdr_instr & RDF_USE_CALLER_ULM_RCB
	|| (ulm_rcb = (ULM_RCB *) rdf_rb->rdf_fac_ulmrcb) == (ULM_RCB *) NULL
	|| (   ulm_rcb->ulm_facility != DB_OPF_ID
	    && ulm_rcb->ulm_facility != DB_QEF_ID
	   )
	|| (pack_rcb = rdf_rb->rdr_qt_pack_rcb) == (RDF_QT_PACK_RCB *) NULL
	|| pack_rcb->rdf_qt_tuples == (QEF_DATA **) NULL
	|| pack_rcb->rdf_qt_tuple_count == (i4 *) NULL
	|| (   pack_rcb->rdf_qt_mode != DB_VIEW
	    && pack_rcb->rdf_qt_mode != DB_INTG
	    && pack_rcb->rdf_qt_mode != DB_RULE
	   )
       )
    {
	rdu_ierror(global, E_DB_ERROR, E_RD0003_BAD_PARAMETER);
	return(E_DB_ERROR);
    }

    qrydata.qrym_data = (QEF_DATA *) NULL;
    qrydata.qrym_cnt  = 0;

    status = rdu_treenode_to_tuple(global, rdf_rb->rdr_qry_root_node,
	&rdf_rb->rdr_tabid, pack_rcb->rdf_qt_mode, &qrydata);
    if (DB_FAILURE_MACRO(status))
	return(status);

    /* tell caller where to find tuples and how many there are */
    *pack_rcb->rdf_qt_tuples = qrydata.qrym_data;
    *pack_rcb->rdf_qt_tuple_count = qrydata.qrym_cnt;

    return(E_DB_OK);
}
