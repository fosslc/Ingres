/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <bt.h>
#include    <qu.h>
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
**  Name: PSYCOMMENT.C - Code related to creation of column/table comments.
**
**  Description:
**	This module contains code used to store (and, because of definition of
**	COMMENT command, delete) table/column comments.
**
**          psy_comment - Insert long and/or short remark for a table or a
**	    column.
**
**  History:
**      21-feb-90 (andre)
**	    Created.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	30-nov-92 (anitap)
**	    Moved around the order of include of header files for CREATE 
**	    SCHEMA.	
**	15-dec-92 (wolf) 
**	    ulf.h must be included before qsf.h
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-aug-93 (andre)
**	    fixed causes of compiler warnings
**	15-sep-93 (swm)
**	    Added cs.h include above other header files which need its
**	    definition of CS_SID.
[@history_template@]...
*/

/*{
** Name: psy_comment - Insert long and/or short remark for a table or a column.
**
** Description:
**	Call RDF_UPDATE to insert or change a comment on a table or a column.
** Inputs:
**	psy_cb		PSY control block.
**	sess_cb		PSF session control block.
** Outputs:
**	Exceptions:
**	    none
** Returns:
**	E_DB_OK			comment has been inserted successfully;
**	error status from RDF	otherwise
**
** Side Effects:
**	    Modifies system catalogs.
**
** History:
**      21-feb-90 (andre)
**	    Created.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	03-aug-92 (barbara)
**	    Call pst_rdfcb_init to initialize RDF_CB before call to rdf_call.
**	    Invalidate base object's infoblk from RDF cache after update.
**	2-sep-93 (stephenb)
**	    Initialize dbc_tabname in rdr_qrytuple, we need this info to 
**	    audit comment on table action in QEF.
*/

DB_STATUS
psy_comment(
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
    rdf_rb->rdr_types_mask = RDR_COMMENT;
    rdf_rb->rdr_update_op = RDR_APPEND;
    rdf_rb->rdr_qrytuple = psy_cb->psy_tupptr;
    ((DB_1_IICOMMENT *)rdf_rb->rdr_qrytuple)->dbc_tabname =
	psy_cb->psy_tabname[0];
    rdf_rb->rdr_tabid.db_tab_base = DM_B_COMMENT_TAB_ID;
    rdf_rb->rdr_tabid.db_tab_index = DM_I_COMMENT_TAB_ID;

    /* Insert/change long and/or short remark. */
    status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);

    if (DB_FAILURE_MACRO(status))
    {
	if (rdf_cb.rdf_error.err_code == E_RD013D_CREATE_COMMENT)
	{
	    if (psy_cb->psy_colq.q_next != &psy_cb->psy_colq)
	    {
		PSY_COL		*err_col = (PSY_COL *) psy_cb->psy_colq.q_next;

		/* error creating comment on a column */
		(VOID) psf_error(E_PS0BA5_COLUMN_COMMENT_ERROR, 0L, PSF_USERERR,
		    &err_code, &psy_cb->psy_error, 2,
		    psf_trmwhite(sizeof(DB_ATT_NAME),
			(char *) &err_col->psy_colnm),
		    &err_col->psy_colnm,
		    psf_trmwhite(sizeof(DB_TAB_NAME), 
			(char *) psy_cb->psy_tabname),
		    psy_cb->psy_tabname);
	    }
	    else
	    {
		/* error creating comment on a table */
		(VOID) psf_error(E_PS0BA4_TABLE_COMMENT_ERROR, 0L, PSF_USERERR,
		    &err_code, &psy_cb->psy_error, 1,
		    psf_trmwhite(sizeof(DB_TAB_NAME), 
			(char *) psy_cb->psy_tabname),
		    psy_cb->psy_tabname);
	    }
	}
	else
	{
	    (VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error,
		&psy_cb->psy_error);
	}
    }

    return (status);
}
