/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <bt.h>
#include    <qu.h>
#include    <me.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>

/*
**
**  Name: PSYSYNONYM.C - Code related to creation/destruction of synonyms.
**
**  Description:
**	This module contains code used to create and delete synonyms.
**
**          psy_create_synonym - Insert a tuple into IISYNONYM.
**	    psy_drop_synonym   - Drop an IISYNONYM tuple
**
**  History:
**      19-apr-90 (andre)
**	    Created.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	08-dec-92 (anitap)
**	    Changed the order of #includes of header files for CREATE SCHEMA.
**	15-dec-92 (wolf) 
**	    ulf.h must be included before qsf.h
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	15-sep-93 (swm)
**	    Added cs.h include above other header files which need its
**	    definition of CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
*/

/* TABLE OF CONTENTS */
i4 psy_create_synonym(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb);
i4 psy_drop_synonym(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb);

/*{
** Name: psy_create_synonym - Insert a tuple into IISYNONYM.
**
** Description:
**	Call RDF_UPDATE to insert a tuple into IISYNONYM.
** Inputs:
**	psy_cb		PSY control block.
**	sess_cb		PSF session control block.
** Outputs:
**	Exceptions:
**	    none
** Returns:
**	E_DB_OK			synonym tuple has been inserted successfully;
**	error status from RDF	otherwise
**
** Side Effects:
**	    Modifies system catalogs.
**
** History:
**      19-apr-90 (andre)
**	    Created.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	03-aug-92 (barbara)
**	    Invalidate base object's infoblk from the RDF cache.
**	10-aug-93 (andre)
**	    fixed causes of compiler warnings
*/
DB_STATUS
psy_create_synonym(
	PSY_CB	   *psy_cb,
	PSS_SESBLK *sess_cb)
{
    RDF_CB		rdf_cb;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    DB_STATUS		status;
    i4		err_code;

    /* Initialize the RDF request block. */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    STRUCT_ASSIGN_MACRO(sess_cb->pss_user, rdf_rb->rdr_owner);
    rdf_rb->rdr_2types_mask = (RDF_TYPES) RDR2_SYNONYM;
    rdf_rb->rdr_update_op = RDR_APPEND;
    rdf_rb->rdr_qrytuple = psy_cb->psy_tupptr;
    rdf_rb->rdr_tabid.db_tab_base = DM_B_SYNONYM_TAB_ID;
    rdf_rb->rdr_tabid.db_tab_index = DM_I_SYNONYM_TAB_ID;

    /* Insert a tuple into IISYNONYM */
    status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);

    if (DB_FAILURE_MACRO(status))
    {
	if (rdf_cb.rdf_error.err_code == E_RD0143_CREATE_SYNONYM)
	{
	    DB_IISYNONYM	*syn = (DB_IISYNONYM *) psy_cb->psy_tupptr;
	    
	    (VOID) psf_error(E_PS0454_CREATE_SYN_ERROR, 0L, PSF_USERERR,
		&err_code, &psy_cb->psy_error, 1,
		psf_trmwhite(sizeof(DB_SYNNAME), (char *) &syn->db_synname),
		&syn->db_synname);
	}
	else
	{
	    (VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error,
		&psy_cb->psy_error);
	}

	return (status);
    }

    /* Invalidate the base object's info block from the RDF cache */
    {
	DB_IISYNONYM	*syn_tuple = (DB_IISYNONYM *) psy_cb->psy_tupptr;

	pst_rdfcb_init(&rdf_cb, sess_cb);
	STRUCT_ASSIGN_MACRO(syn_tuple->db_syntab_id, rdf_rb->rdr_tabid);

	status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_cb);

	if (DB_FAILURE_MACRO(status))
	{
	    (VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_cb.rdf_error,
				&psy_cb->psy_error);
	}
    }
    return (status);
}

/*{
** Name: psy_drop_synonym - Drop an IISYNONYM tuple.
**
** Description:
**	Call RDF_UPDATE to delete a tuple from IISYNONYM.
** Inputs:
**	psy_cb		PSY control block.
**	sess_cb		PSF session control block.
** Outputs:
**	Exceptions:
**	    none
** Returns:
**	E_DB_OK			synonym tuple has been deleted successfully;
**	error status from RDF	otherwise
**
** Side Effects:
**	    Modifies system catalogs.
**
** History:
**      19-apr-90 (andre)
**	    Created.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	03-aug-92 (barbara)
**	    Invalidate base table infoblk from RDF's cache.
**	10-aug-93 (andre)
**	    fixed causes of compiler warnings
**	13-sep-93 (andre)
**	    QEF will assume responsibility for altering timestamps of tables
**	    (or underlying tables of views) synonym(s) on which have been 
**	    dropped.  We will supply QEF with the id of the object on which 
**	    the synonym was defined
**	22-oct-93 (andre)
**	    In the unlikely event that RDF's cache entry for the synonym named 
**	    in the DROP SYNONYM statement is stale and the synonym no longer 
**	    exists, RDF will return E_RD014A_NONEXISTENT_SYNONYM which we will 
**	    translate into 2753L
*/
DB_STATUS
psy_drop_synonym(
	PSY_CB	   *psy_cb,
	PSS_SESBLK *sess_cb)
{
    RDF_CB		rdf_cb;
    RDF_CB		rdf_inv_cb;
    DB_IISYNONYM	syn_tuple;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    DB_STATUS		status;
    register i4	syn_count;
    i4		err_code;


    /* Initialize the RDF request block. */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    pst_rdfcb_init(&rdf_inv_cb, sess_cb);
    STRUCT_ASSIGN_MACRO(sess_cb->pss_user, rdf_rb->rdr_owner);
    rdf_rb->rdr_2types_mask = (RDF_TYPES) RDR2_SYNONYM;
    rdf_rb->rdr_update_op = RDR_DELETE;
    rdf_rb->rdr_qrytuple = (PTR) &syn_tuple;
    rdf_rb->rdr_tabid.db_tab_base = DM_B_SYNONYM_TAB_ID;
    rdf_rb->rdr_tabid.db_tab_index = DM_I_SYNONYM_TAB_ID;

    MEmove(sizeof(DB_OWN_NAME), (char *)&sess_cb->pss_user, ' ',
           sizeof(DB_SYNOWN), (char *)&syn_tuple.db_synowner);

    for (syn_count = 0; syn_count < psy_cb->psy_numtabs; syn_count++)
    {
	/* store synonym name in the tuple */
	MEmove(sizeof(DB_TAB_NAME), (char *)(psy_cb->psy_tabname + syn_count),
	       ' ', sizeof(DB_SYNNAME), (char *)&syn_tuple.db_synname);
	
	syn_tuple.db_syntab_id.db_tab_base = 
	    psy_cb->psy_tables[syn_count].db_tab_base;
	
	syn_tuple.db_syntab_id.db_tab_index = 
	    psy_cb->psy_tables[syn_count].db_tab_index;

	/* Drop a tuple from IISYNONYM */
	status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);

	if (DB_FAILURE_MACRO(status))
	{
	    if (rdf_cb.rdf_error.err_code == E_RD0144_DROP_SYNONYM)
	    {
		(VOID) psf_error(E_PS0456_DROP_SYN_ERROR, 0L, PSF_USERERR,
		    &err_code, &psy_cb->psy_error, 1,
		    psf_trmwhite(sizeof(DB_SYNNAME), 
			(char *) &syn_tuple.db_synname),
		    &syn_tuple.db_synname);
	    }
	    else if (rdf_cb.rdf_error.err_code == E_RD014A_NONEXISTENT_SYNONYM)
	    {
		/* 
		** looks like RDF cache entry was stale - tell user that 
		** synonym did not exist and proceed on to the next entry
		*/
		status = E_DB_OK;

		(VOID) psf_error(2753L, 0L, PSF_USERERR, &err_code,
		    &psy_cb->psy_error, 2,
		    sizeof("DROP SYNONYM") - 1, "DROP SYNONYM",
		    psf_trmwhite(sizeof(DB_SYNNAME), 
			(char *) &syn_tuple.db_synname),
		    &syn_tuple.db_synname);
		
		continue;
	    }
	    else
	    {
		(VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error,
		    &psy_cb->psy_error);
	    }
	    break;
	}


	STRUCT_ASSIGN_MACRO(psy_cb->psy_tables[syn_count],
			    rdf_inv_cb.rdf_rb.rdr_tabid);
				
	status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_inv_cb);

	if (DB_FAILURE_MACRO(status))
	{
	    (VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_inv_cb.rdf_error,
				&psy_cb->psy_error);
	    break;
	}
    }

    return (status);
}
