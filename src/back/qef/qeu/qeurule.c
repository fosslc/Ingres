/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <er.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <tm.h>
#include    <bt.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <usererror.h>
#include    <cs.h>
#include    <lk.h>
#include    <scf.h>
#include    <ulb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <qsf.h>
#include    <dmf.h>
#include    <dmacb.h>
#include    <dmucb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefscb.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>

#include    <ade.h>
#include    <dudbms.h>
#include    <dmmcb.h>
#include    <ex.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <sxf.h>
#include    <qefprotos.h>
#include    <rdf.h>


/**
**  Name: qeurule.c - Provide qrymod rule support for RDF and PSF.
**
**  Description:
**      The routines defined in this file provide the qrymod semantics for
**	RDF (relation description facility) rulkes processing.	Qrymod
**	provides query modification to query trees in the form of added
**	protections, integrities, rules and views. The routines in this file
**	create and destroy rule objects.  See qeuqmod.c for other qrymod
**	objects.
**
**  Defines:
**      qeu_crule		- Create a rule
**      qeu_drule		- Destroy a rule
**
**  Locals:
**
**  History:
** 	21-mar-89 (neil)
**	   Written for Terminator/rules.
**	31-may-89 (neil)
**	   Trim blanks off of object names on errors.
**	09-feb-90 (neil)
**	   Updating auditing error recovery.
**	11-jun-90 (nancy)
**	    Fixed bug 30529 in qeu_drule().  See comments below.
**	02-feb-91 (davebf)
**	    Added init of qeu_cb.qeu_mask
**	30-jul-92 (rickh)
**	    FIPS CONSTRAINTS:  changed DBR_T_TABLE to DBR_T_ROW.
**	08-dec-1992 (pholman)
**	    C2: Replace obsolete qeu_audit with qeu_secaudit
**	16-mar-93 (rickh)
**	    At rule creation time, allocate a rule id.
**	04-apr-93 (rblumer)
**	    FIPS CONSTRAINTS:  don't allow users to drop NOT_DROPPABLE rules.
**	    included er.h and added externs for dmf_call & qef_error.
**	24-jun-93 (robf)
**	    Add MAC access on rule prior to dropping it.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	07-jul-93 (anitap)
**	    Changed arguments for qea_reserveID() in qeu_crule().
**      19-aug-93 (anitap)
**          Initialized qef_cb->qef_dbid in qeu_crule().
**	20-aug-93 (stephenb)
**	    Changed calls to qeu_secaudit() to audit messages 
**	    I_SX2037_RULE_CREATE and I_SX2038_RULE_DROP on rule creation and 
**	    destruction instead of the generic I_SX202F_RULE_ACCESS.
**      14-sep-93 (smc)
**          Fixed up casts of qeuq_d_id now session ids are CS_SID.
**	17-mar-1999 (somsa01)
**	    Added include of pc.h and lk.h to resolve references to locking
**	    ids.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to qeu_secaudit(), qeu_secaudit_detail()
**	    if not C2SECURE.
**	    Deleted redeclaration of dmf_call().
**      18-Apr-2001 (hweho01)
**          Removed the prototype declaration of qef_error(), since it
**          is defined in qefprotos.h header file.
**	28-aug-2002 (gygro01) ingsrv1859
**	    b108474 (b108635) Added additional check in qeu_drule()
**	    for drop column cascade processing in order to have system
**	    generated rules with referential constraints dependencies
**	    dropped, if the dependent referential constraint has been
**	    dropped previously.
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**	13-Feb-2006 (kschendel)
**	    Fix a couple annoying int == NULL warnings.
**	12-Apr-2008 (kschendel)
**	    DMF qualifier not used any more (since secondary index was
**	    added) -- delete the routine.
**	4-Jun-2009 (kschendel) b122118
**	    Make sure dmt-cb doesn't have junk in it.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/

/*{
** Name: qeu_crule 	- Store rule information for one rule.
**
** External QEF call:   status = qef_call(QEU_CRULE, &qeuq_cb);
**
** Description:
**	Add the rule tuple, the rule query text, and, if there's a tree,
**	the rule tree to the appropriate tables (iirule, iiqrytext and iitree),
**	and updated the table "relstat" field.
**
**	The rule tuple was filled from the CREATE RULE statement, except
**	that since this is the first access to iirule, the uniqueness
**	of the rule name (within the current user scope) is validated here.
**	First, all rules that have the same name and owner are fetched.
**	If there are any then the rule is a duplicate and an error is
**	returned.  If this is unique then the rule is entered into iirule.
**	Note that for fast rule access during query processing rules are
**	qualified and keyed off of the table ids they reference.  In this
**	case the rule is qualified by the name and owner.
**
**	An internal qrytext id (timestamp) is created and all the qrytext
**	tuples associated with the rule are entered into iiqrytext.  An
**	internal tree id (timestamp) is created and all the tree tuples
**	associated with the rule are entered into iiqrytext. Note that
**	based on size of text and/or tree there may be more than one
**	tuple of each for the single rule.  There can be zero tree tuples
**	if the rule had no associated tree.
**
**	After successfully entering rule information the "relstat" field of
**	the table is updated to reflect there are rules (DMT_RULE).
**
** Inputs:
**      qef_cb                  QEF session control block
**      qeuq_cb
**	    .qeuq_eflag	        Designate error handling for user errors.
**		QEF_INTERNAL	Return error code.
**		QEF_EXTERNAL	Send message to user.
**	    .qeuq_ct		Number of tree tuples (may be 0).
**	    .qeuq_tre_tup	Tree tuple data.
**	    .qeuq_cq		Number of query text tuples.
**	    .qeuq_qry_tup	Query text tuples.
**	    .qeuq_cr		Number of rule tuples.  Must be 1.
**	    .qeuq_rul_tup	Rule tuple.
**	    .qeuq_db_id		Database id.
**	    .qeuq_d_id		DMF session id.
**	
** Outputs:
**      qeuq_cb
**	    .error.err_code	E_QE0002_INTERNAL_ERROR
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0022_ABORTED
**				E_US189C_6300_RULE_EXISTS
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** History:
**	07-mar-89 (neil)
**	    Written for Terminator/rules.
**	31-may-89 (neil)
**	   Trim blanks off of object names on errors.
**	16-mar-93 (rickh)
**	    At rule creation time, allocate a rule id.
**	07-jul-93 (anitap)
**	    Added new arguments to qea_reserveID().
**	19-aug-93 (anitap)
**	    Initialized qef_cb->qef_dbid.
**	07-sep-93 (andre)
**	    if creating a rule on a view, alter timestamp of its underlying base
**	    table - this will ensure that any QPs dependent on that view will 
**	    get invalidated and, where appropriate, the newly created rule will 
**	    get attached to query trees
**	27-oct-93 (andre)
**	    As a part of fix for bug 51852 (which was reported in 6.4 but can
**	    and will manifest itself in 6.5 as soon as someone places some 
**	    stress on the DBMS), we want to use rule id (really guaranteed to
**	    be unique) instead of timestamps (allegedly unique, but in fact may
**	    be non-unique if several objects get created in rapid succession on
**	    a reasonably fast box) to identify IIQRYTEXT and IITREE tuples
**          associated with a given rule.  This id (pumped through randomizing 
**	    function) will be used to key into IIQRYTEXT and will be stored 
**	    instead of the timestamp in IITREE 
**	04-nov-93 (andre)
**	    interface of qeu_force_qp_invalidation() has changed to enable 
**	    caller to request and receive a notification of whether QPs 
**	    affected by the change have, indeed, been invalidated.  When 
**	    creating a rule, we are not really interested in this information 
**	    (since we can't do very much if QPs could not be invalidated + I do
**	    not expect this situation to arise too often since rules cannot be 
**	    created on non-updatable views)
**	24-nov-93 (andre)
**	    qeu_force_qp_invalidation() will report its own errors; this 
**	    requires that we supply it with address of DB_ERROR instead of 
**	    space to store error number
**	27-may-1999 (nanpr01)
**	    Created a secondary index on owner name and rule name to do
**	    the existence check without scanning the entire table.
*/
DB_STATUS
qeu_crule(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb)
{
    DB_IIQRYTEXT	*qtuple;	/* Rule text */
    DB_IITREE	    	*ttuple;	/* Rule tree (may be null) */
    DB_IIRULE    	*rtuple;	/* New rule tuple */
    DB_IIRULE    	rtuple_temp;	/* Tuple to check for uniqueness */
    DB_IIRULEIDX1	xrultuple1;
    DMR_ATTR_ENTRY	xrul1key_array[2];
    DMR_ATTR_ENTRY	*xrul1key_ptr_array[2];
    DB_STATUS	    	status, local_status;
    i4	    	error;
    bool	    	transtarted = FALSE;	    
    bool	    	tbl_opened = FALSE;
    bool		err_already_reported = FALSE;
    i4		    	i;		/* Querytext and tree tuple counter */
    QEF_DATA	    	*next;		/*	     and data pointer       */
    QEU_CB	    	tranqeu;
    DB_TAB_ID		randomized_id;
    QEU_CB	    	qeu;
    QEF_DATA	    	qef_data;
    DMT_CHAR_ENTRY  	char_spec;	/* DMT_ALTER specification */
    DMT_CB	    	dmt_cb;

    for (;;)	/* Dummy for loop for error breaks */
    {
	/* Validate CB and parameters */
	if (qeuq_cb->qeuq_type != QEUQCB_CB ||
            qeuq_cb->qeuq_length != sizeof(QEUQ_CB))	
	{
	    status = E_DB_ERROR;
	    error = E_QE0017_BAD_CB;
	    break;
	}
	if (   (qeuq_cb->qeuq_cq == 0 || qeuq_cb->qeuq_qry_tup == NULL)
            || (qeuq_cb->qeuq_cr != 1 || qeuq_cb->qeuq_rul_tup == NULL)
            || (qeuq_cb->qeuq_db_id == NULL)
            || (qeuq_cb->qeuq_d_id == 0))
	{
	    status = E_DB_ERROR;
	    error = E_QE0018_BAD_PARAM_IN_CB;
	    break;
	}
		
	/* 
	** Check to see if transaction is in progress, if so set a local
	** transaction, otherwise we'll use the user's transaction.
	*/
	if (qef_cb->qef_stat == QEF_NOTRAN)
	{
	    tranqeu.qeu_type = QEUCB_CB;
	    tranqeu.qeu_length = sizeof(QEUCB_CB);
	    tranqeu.qeu_db_id = qeuq_cb->qeuq_db_id;
	    tranqeu.qeu_d_id = qeuq_cb->qeuq_d_id;
	    tranqeu.qeu_flag = 0;
	    tranqeu.qeu_mask = 0;
	    status = qeu_btran(qef_cb, &tranqeu);
	    if (status != E_DB_OK)
	    {	
		error = tranqeu.error.err_code;
		break;	
	    }	    
	    transtarted = TRUE;
	}
	/* Escalate the transaction to MST */
	if (qef_cb->qef_auto == QEF_OFF)
	    qef_cb->qef_stat = QEF_MSTRAN;

	rtuple = (DB_IIRULE *)qeuq_cb->qeuq_rul_tup->dt_data;

	/* 
        ** Alter the table relstat to flag rules exist. This validates that
        ** the table exists and ensures that we get an exclusive lock on it.
        */
	char_spec.char_id = DMT_C_RULE;
	char_spec.char_value = DMT_C_ON;
	MEfill(sizeof(DMT_CB), 0, (PTR) &dmt_cb);
	dmt_cb.dmt_db_id = qeuq_cb->qeuq_db_id;
	dmt_cb.dmt_char_array.data_in_size = sizeof(DMT_CHAR_ENTRY);
	dmt_cb.dmt_char_array.data_address = (PTR)&char_spec;
	dmt_cb.length = sizeof(DMT_CB);
	dmt_cb.type = DMT_TABLE_CB;
	dmt_cb.dmt_id.db_tab_base  = rtuple->dbr_tabid.db_tab_base;
	dmt_cb.dmt_id.db_tab_index = rtuple->dbr_tabid.db_tab_index;
	dmt_cb.dmt_tran_id = qef_cb->qef_dmt_id;
	status = dmf_call(DMT_ALTER, &dmt_cb);
	if (status != E_DB_OK)
	{
	    error = dmt_cb.error.err_code;
	    break;
	}
    
	/* Validate that the rule name/owner is unique */
	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_RULEIDX1_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_RULEIDX1_TAB_ID;
        qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        qeu.qeu_lk_mode = DMT_IS;
        qeu.qeu_flag = DMT_U_DIRECT;
        qeu.qeu_access_mode = DMT_A_READ;
	qeu.qeu_mask = 0;
	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = TRUE;

	/* Retrieve the same named rule - if not there then ok */
	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DB_IIRULEIDX1);
	qeu.qeu_output = &qef_data;
	qef_data.dt_next = NULL;
        qef_data.dt_size = sizeof(DB_IIRULEIDX1);
        qef_data.dt_data = (PTR) &xrultuple1;

	qeu.qeu_key = xrul1key_ptr_array;
	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_klen = 2;

	xrul1key_ptr_array[0] = &xrul1key_array[0];
	xrul1key_ptr_array[0]->attr_number = DM_1_RULEIDX1_KEY;
	xrul1key_ptr_array[0]->attr_operator = DMR_OP_EQ;
	xrul1key_ptr_array[0]->attr_value = (char *)&rtuple->dbr_owner;
	xrul1key_ptr_array[1] = &xrul1key_array[1];
	xrul1key_ptr_array[1]->attr_number = DM_2_RULEIDX1_KEY;
	xrul1key_ptr_array[1]->attr_operator = DMR_OP_EQ;
	xrul1key_ptr_array[1]->attr_value = (char *)&rtuple->dbr_name;
	qeu.qeu_qual = NULL;
	qeu.qeu_qarg = NULL;

	status = qeu_get(qef_cb, &qeu);
	if (status == E_DB_OK)		/* Found the same rule! */
	{
	    (VOID)qef_error(E_US189C_6300_RULE_EXISTS, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 1,
			    qec_trimwhite(sizeof(rtuple->dbr_name),
					  (char *)&rtuple->dbr_name),
			    (PTR)&rtuple->dbr_name);
	    error = E_QE0025_USER_ERROR;
	    break;
	}
	if (qeu.error.err_code != E_QE0015_NO_MORE_ROWS)
	{
	    /* Some other error */
	    error = qeu.error.err_code;
	    break;
	}

	status = qeu_close(qef_cb, &qeu);    
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = FALSE;

	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_RULE_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_RULE_TAB_ID;
        qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = DMT_U_DIRECT;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask = 0;
	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = TRUE;

	/* The rule is unique - append it */
	status = E_DB_OK;

	/* 
	** allocate a rule id and use it to compute a randomized id which will
	** be used to join this IIRULE tuple to IIQRYTEXT/IITREE tuples 
	** associated with it
	*/

	{
	    QEF_RCB	qef_rcb;
	    qef_rcb.qef_cb = qef_cb;
	    qef_rcb.qef_db_id = qeuq_cb->qeuq_db_id;
	    qef_cb->qef_dbid = qeuq_cb->qeuq_db_id;

	    status = qea_reserveID(&rtuple->dbr_ruleID, qef_cb, &qef_rcb.error);

	    if (status != E_DB_OK)
	    {   error = qef_rcb.error.err_code;  break; }

	    randomized_id.db_tab_base = 
		ulb_rev_bits(rtuple->dbr_ruleID.db_tab_base);
	    randomized_id.db_tab_index = 
		ulb_rev_bits(rtuple->dbr_ruleID.db_tab_index);
	}

	/* set query text id to the "randomized" rule id */
	rtuple->dbr_txtid.db_qry_high_time = randomized_id.db_tab_base;
	rtuple->dbr_txtid.db_qry_low_time  = randomized_id.db_tab_index;

	/* 
	** if there are IITREE tuples associated with this rule, set tree id to
	** the "randomized" rule id 
	*/
	if (qeuq_cb->qeuq_ct > 0)
	{
	    rtuple->dbr_treeid.db_tre_high_time = randomized_id.db_tab_base;
	    rtuple->dbr_treeid.db_tre_low_time  = randomized_id.db_tab_index;
	}

	/* Insert single rule tuple */
	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DB_IIRULE);
	qeu.qeu_input = qeuq_cb->qeuq_rul_tup;
	status = qeu_append(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	status = qeu_close(qef_cb, &qeu);    
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = FALSE;

	/* Update all query text tuples with query id - validate list */
	next = qeuq_cb->qeuq_qry_tup;
	for (i = 0; i < qeuq_cb->qeuq_cq; i++)
	{
	    qtuple = (DB_IIQRYTEXT *)next->dt_data;
	    next = next->dt_next;
	    qtuple->dbq_txtid.db_qry_high_time = randomized_id.db_tab_base;
	    qtuple->dbq_txtid.db_qry_low_time  = randomized_id.db_tab_index;
	    if (i < (qeuq_cb->qeuq_cq -1) && next == NULL)
	    {
		error = E_QE0018_BAD_PARAM_IN_CB;
		status = E_DB_ERROR;
		break;
	    }
	} /* for all query text tuples */
	if (status != E_DB_OK)
	    break;

	/* Insert all query text tuples */
	qeu.qeu_tab_id.db_tab_base = DM_B_QRYTEXT_TAB_ID;
	qeu.qeu_tab_id.db_tab_index = 0L;
	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = TRUE;
	qeu.qeu_count = qeuq_cb->qeuq_cq;
        qeu.qeu_tup_length = sizeof(DB_IIQRYTEXT);
	qeu.qeu_input = qeuq_cb->qeuq_qry_tup;
	status = qeu_append(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	status = qeu_close(qef_cb, &qeu);    
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = FALSE;
	
	/* Do the same for the tree tuples (if any) */
	if (qeuq_cb->qeuq_ct > 0)
	{
	    /* Update all tree tuples with tree id - validate list */
	    next = qeuq_cb->qeuq_tre_tup;
	    for (i = 0; i < qeuq_cb->qeuq_ct; i++)
	    {
		ttuple = (DB_IITREE *)next->dt_data;
		next = next->dt_next;
		ttuple->dbt_trid.db_tre_high_time = randomized_id.db_tab_base;
		ttuple->dbt_trid.db_tre_low_time  = randomized_id.db_tab_index;
		if (i < (qeuq_cb->qeuq_ct - 1) && next == NULL)
		{
		    error = E_QE0018_BAD_PARAM_IN_CB;
		    status = E_DB_ERROR;
		    break;
		}
	    } /* for all tree tuples */
	    if (status != E_DB_OK)    		
		break;

	    /* Insert all tree tuples */
	    qeu.qeu_tab_id.db_tab_base = DM_B_TREE_TAB_ID;
	    qeu.qeu_tab_id.db_tab_index = DM_I_TREE_TAB_ID;
	    status = qeu_open(qef_cb, &qeu);
	    if (status != E_DB_OK)
	    {
		error = qeu.error.err_code;
		break;
	    }
	    tbl_opened = TRUE;
	    qeu.qeu_count = qeuq_cb->qeuq_ct;
	    qeu.qeu_tup_length = sizeof(DB_IITREE);
	    qeu.qeu_input = qeuq_cb->qeuq_tre_tup;
	    status = qeu_append(qef_cb, &qeu);
	    if (status != E_DB_OK)
	    {
		error = qeu.error.err_code;
		break;
	    }
	    status = qeu_close(qef_cb, &qeu);    
	    if (status != E_DB_OK)
	    {
		error = qeu.error.err_code;
		break;
	    }
	    tbl_opened = FALSE;
	} /* If there were any tree tuples */
	
	/* 
	** determine whether the rule is being created on a view - if so, we 
	** need to alter timestamp of the view's underlying base table to force
	** invalidation of QPs that depend on this view
	*/
        status = qeu_force_qp_invalidation(qef_cb, &rtuple->dbr_tabid, 
	    (bool) TRUE, (bool *) NULL, &qeuq_cb->error);
	if (DB_FAILURE_MACRO(status))
	{
	    err_already_reported = TRUE;
	    break;
	}

	if (transtarted)		   /* If we started a transaction */
	{
	    status = qeu_etran(qef_cb, &tranqeu);
	    if (status != E_DB_OK)
	    {
		error = tranqeu.error.err_code;
		break;
	    }
	}

	/* Security audit rule create */

	if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	{
	    DB_ERROR e_error;

	    status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
	    		(char *)&rtuple->dbr_name,
			&rtuple->dbr_owner,
	    		sizeof(rtuple->dbr_name), SXF_E_RULE,
	      		I_SX2037_RULE_CREATE, SXF_A_SUCCESS | SXF_A_CREATE,
	      		&e_error);
	    error = e_error.err_code;

	    if (status != E_DB_OK)
		break;
	}

	qeuq_cb->error.err_code = E_QE0000_OK;
	return (E_DB_OK);
    } /* End dummy for */

    if (!err_already_reported)
    {
        /* call qef_error to handle error messages */
        (VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error, 0);
    }
    
    /* Close off all the tables. */
    local_status = E_DB_OK;
    if (tbl_opened)		    /* If system table opened, close it */
    {
	local_status = qeu_close(qef_cb, &qeu);
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(qeu.error.err_code, 0L, local_status, &error, 
		    	     &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }

    if (transtarted)		   /* If we started a transaction */
    {
        local_status = qeu_atran(qef_cb, &tranqeu);
        if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(tranqeu.error.err_code, 0L, local_status, 
			     &error, &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }
    return (status);
} /* qeu_crule */

/*{
** Name: qeu_drule 	- Drop single or all rule definition.
**
** External QEF call:   status = qef_call(QEU_DRULE, &qeuq_cb);
**
** Description:
**	Drop the rule tuple, the rule text, and, if there's a rule tree,
**	the rule from tree from the appropriate tables (iirule, iiqrytext
**	and iitree), and update the table "relstat" field. A rule can
**	be dropped either by name (when the DROP RULE statement is
**	issued) or by table id (when its associated table is dropped).
**
**	When originating from the DROP RULE statement the single rule tuple
**	is deleted based on its name and owner - both provided in the
**	qeuq_rul_tup parameter.  In this case the single rule is fetched
**	(to collect the text, tree and table id) and deleted.  If it wasn't
**	found then an error is returned.  If the rule was found, and it's a
**	table-type rule (DBR_T_RULE) the table id is used later to find at
**	least one rule still remaining for the current table - if there are
**	none left then the "relstat" field rule bit is turned off (DMT_RULE).
**	The secondary index on iirule is used for dropping a specific rule.
**
**	When originating from a DROP TABLE statement (this is similar to
**	the DROP INTEGRITY ON table ALL), all tuples that satisfy the
**	table-id are fetched (to collect their tree and text ids) and dropped.
**	If none are found no error is returned.  The "relstat" field is left
**	alone as the table will be dropped anyway.
**
**	For each rule that is dropped the tree (if any) and text tuples are
**	also dropped based on the ids collected from the fetched rule.
**
**	The way this routine works is the following:
**
**	    set up QEU CB's for iirule, iiqrytext and iitree and open tables;
**	    if (DROP RULE) then
**		set up iirule "qualified" name and owner;
**	    else
**		set up iirule "keyed" on DROP TABLE table id;
**	    endif;
**	    for (all rule tuples found) loop
**		get rule tuple;
**		if (not found) then
**		    if (DROP RULE) then
**			error - rule does not exist;
**			break;
**		    else
**			clear error;
**			break;
**		    endif;
**		endif;
**		if (rule has a tree id) then
**		    set up iitree keys based on rule tree id;
**		    for (all tree tuples found) loop
**			get tree tuple;
**			delete tree tuple;
**		    endloop;
**		endif;
**		set up iiqrytext keys based on rule text id;
**		for (all text tuples found) loop
**		    get text tuple;
**		    delete text tuple;
**		endloop;
**		delete current rule tuple;
**		if (DROP RULE) then
**		    endloop;
**		endif;
**	    endloop;
**	    if (DROP RULE) then
**		set up iirule keys based on table id in current tuple;
**		reposition and get rule tuple;
**		if (not found) then
**		    call DMT_ALTER to turn off rules;
**		endif;
**	    endif;
**	    close tables;
**
** Inputs:
**      qef_cb                  QEF session control block
**      qeuq_cb
**	    .qeuq_eflag	        Designate error handling for user errors.
**		QEF_INTERNAL	Return error code.
**		QEF_EXTERNAL	Send message to user.
**	    .qeuq_rtbl		Table id of rule - if DROP TABLE.  If DROP
**				RULE then this is ignored.
**	    .qeuq_cr		Must be 1 if DROP RULE, 0 if DROP TABLE.
**	    . qeuq_flag_mask	misc. bits of information
**		QEU_FORCE_QP_INVALIDATION
**				force invalidation of QPs that may involve
**				the rule being dropped; this bit may be set 
**				ONLY WHEN requesting destruction of a specific 
**				rule
**	    .qeuq_rul_tup	Rule tuple to be deleted - if DROP RULE, 
**				otherwise this is ignored:
**		.dbr_name	Rule name.
**		.dbr_owner	Rule owner.
**	    .qeuq_db_id		Database id.
**	    .qeuq_d_id		DMF session id.
**	from_drop_rule		TRUE if called as a part of DROP RULE
**				processing; FALSE otherwise
**	
** Outputs:
**      qeuq_cb
**	    .error.err_code	E_QE0002_INTERNAL_ERROR
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0022_ABORTED
**				E_US189D_6301_RULE_ABSENT
**				E_US18A8_6312_RULE_DRPTAB
**	    .qeuq_rul_tupe
**		.dbr_tabid	Will contain table id of dropped rule.
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** History:
**	07-mar-89 (neil)
**	    Written for Terminator/rules.
**	31-may-89 (neil)
**	   Trim blanks off of object names on errors.
**	11-jun-90 (nancy)
**	    Bug 30529 where rule will continue to fire after its dropped.
**	    Timestamp needs to be changed on a "drop rule" regardless of
**	    whether this is last rule or not.
**	03-aug-1992 (barbara)
**	    Copy table id and rule owner from rule tuple into caller's rule
**	    tuple structure.  This will assist in clearing table information off
**	    the RDF cache.
**	30-jul-92 (rickh)
**	    FIPS CONSTRAINTS:  changed DBR_T_TABLE to DBR_T_ROW.
**	04-apr-93 (rblumer)
**	    FIPS CONSTRAINTS:  don't allow users to drop NOT_DROPPABLE rules.
**	14-apr-93 (andre)
**	    Added from_drop_rule to the interface
**	    
**	    When we get called to drop a specific rule as a part of DROP RULE
**	    processing, we will prevent user from dropping system-generated
**	    rules; otherwise (we were called as a part of destroying a
**	    constraint which this rule used to enforce) we will merrily proceed
**	    to destroy it
**	24-jun-93 (robf)
**	    Add MAC access check on rule prior to dropping it.
**	6-jul-93 (robf)
**	    Pass security label to qeu_secaudit()
**	07-sep-93 (andre)
**	    if dropping a specific rule on a view, and the caller told us to 
**	    force invalidation of QPs which may involve firing this rule, alter
**	    timestamp of some base table over which the view was defined - this
** 	    will ensure that any QPs dependent on that view will get invalidated
**	04-nov-93 (andre)
**	    interface of qeu_force_qp_invalidation() has changed to enable 
**	    caller to request and receive a notification of whether QPs 
**	    affected by the change have, indeed, been invalidated.  When 
**	    dropping a rule, we are not really interested in this information 
**	    (since we can't do very much if QPs could not be invalidated + I do
**	    not expect this situation to arise too often since rules cannot be 
**	    created on non-updatable views)
**	24-nov-93 (andre)
**	    qeu_force_qp_invalidation() will report its own errors; this 
**	    requires that we supply it with address of DB_ERROR instead of 
**	    space to store error number
**	23-jul-96 (ramra01)
**	    Alter Table add/drop column implementation with rules on
**	    restrict and cascade option.
**	02-jan-1997 (nanpr01)
**	    Map the error message correctly to E_QE016B_COLUMN_HAS_OBJECTS.
**	29-jan-1997 (shero03 & nanpr01)
**	    B77960: call qeu_v_col_drop to patch the iitree data when dropping
**	    a column from a table with a rule.
**	04-mar-1997 (nanpr01)
**	    For referential constraint, the table may be different from
**	    what is being altered because of the dependency.
**	26-mar-1997 (nanpr01)
**	    More segv on referential integrity col drop.
**	06-may-1997 (nanpr01)
**	    b81811 : Referential integrity not enforce after drop column.
**	    This is due to dropping of few lines in a previous integration.
**	27-may-1999 (nanpr01)
**	    Created a secondary index on owner name and rule name to do
**	    the existence check without scanning the entire table.
**	16-jun-1999 (nanpr01)
**	    alt29.sep loops because on specific rule drop we have to
**	    break out instead of continue.
**	28-aug-2002 (gygro01)
**	    b108474 (b108635) - During drop columns cascade processing 
**	    for system generated rules dependent on referential
**	    constraints column bitmap checking is not sufficient to
**	    decide, if the rule needs to be dropped or not.
**	    This change will only drop rules, if the dependent
**	    referential constraint has been dropped previously.
*/
DB_STATUS
qeu_drule(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
bool		from_drop_rule)
{
    GLOBALREF QEF_S_CB  *Qef_s_cb;
    QEU_CB	    	tranqeu;	/* For transaction request */
    bool	    	transtarted = FALSE;	    
    QEU_CB	    	rqeu;		/* For iirule table */
    QEF_DATA	    	rqef_data;
    DB_IIRULE	    	*rtuple_name;	/* Input tuple with name and owner */
    DB_IIRULE	    	rtuple;		/* Tuple currently being deleted */
    bool	    	rule_opened = FALSE;
    bool		err_already_reported = FALSE;
    DMR_ATTR_ENTRY  	rkey_array[2];
    DMR_ATTR_ENTRY  	*rkey_ptr_array[2];
    QEU_CB	    	qqeu;		/* For iiqrytext table */
    DB_IIQRYTEXT	qtuple;
    QEF_DATA	    	qqef_data;
    bool	    	qtext_opened = FALSE;
    DMR_ATTR_ENTRY  	qkey_array[2];
    DMR_ATTR_ENTRY  	*qkey_ptr_array[2];
    QEU_CB	    	tqeu;		/* For iitree table */
    DB_IITREE		ttuple;
    QEF_DATA	    	tqef_data;
    bool	    	tree_opened = FALSE;
    DMR_ATTR_ENTRY  	tkey_array[2];
    DMR_ATTR_ENTRY  	*tkey_ptr_array[2];
    DMT_CHAR_ENTRY  	char_spec;	/* DMT_ALTER specification */
    DMT_CB	    	dmt_cb;
    bool		drop_specific_rule;	/* TRUE if dropping a specific
						** rule */
    QEU_CB	    	xrul1qeu;		/* For xrulidx1 table */
    DB_IIRULEIDX1	xrul1tuple;
    QEF_DATA	    	xrul1qef_data;
    DMR_ATTR_ENTRY	xrul1key_array[2];
    DMR_ATTR_ENTRY	*xrul1key_ptr_array[2];
    bool	    	xrul1_opened = FALSE;

    DB_TAB_ID		rtab;
    DB_TREE_ID		last_tree_id = { 0, 0};
    DB_STATUS	    	status, local_status;
    i4	    	error;
    RDF_CB		rdfcb, *rdf_inv_cb = &rdfcb;
    RDR_RB		*rdf_inv_rb = &rdf_inv_cb->rdf_rb;

    for (;;)	/* Dummy for loop for error breaks */
    {
	/* Validate CB and parameters */
	if (qeuq_cb->qeuq_type != QEUQCB_CB ||
            qeuq_cb->qeuq_length != sizeof(QEUQ_CB))	
	{
	    status = E_DB_ERROR;
	    error = E_QE0017_BAD_CB;
	    break;
	}
	if (   (qeuq_cb->qeuq_rtbl == NULL && qeuq_cb->qeuq_cr == 0)
            || (qeuq_cb->qeuq_db_id == NULL)
            || (qeuq_cb->qeuq_d_id == 0)
			/* 
			** QEU_FORCE_QP_INVALIDATION may only be set when 
			** requesting destruction of a specific rule
			*/
	    || (   qeuq_cb->qeuq_flag_mask & QEU_FORCE_QP_INVALIDATION
		&& qeuq_cb->qeuq_cr == 0)
	   )
	{
	    status = E_DB_ERROR;
	    error = E_QE0018_BAD_PARAM_IN_CB;
	    break;
	}

	drop_specific_rule = (qeuq_cb->qeuq_cr == 1) ? TRUE : FALSE;

	/* 
	** Check to see if transaction is in progress, if so set a local
	** transaction, otherwise we'll use the user's transaction.
	*/
	if (qef_cb->qef_stat == QEF_NOTRAN)
	{
	    tranqeu.qeu_type = QEUCB_CB;
	    tranqeu.qeu_length = sizeof(QEUCB_CB);
	    tranqeu.qeu_db_id = qeuq_cb->qeuq_db_id;
	    tranqeu.qeu_d_id = qeuq_cb->qeuq_d_id;
	    tranqeu.qeu_flag = 0;
	    tranqeu.qeu_mask = 0;
	    status = qeu_btran(qef_cb, &tranqeu);
	    if (status != E_DB_OK)
	    {	
		error = tranqeu.error.err_code;
		break;	
	    }	    
	    transtarted = TRUE;
	}
	/* Escalate the transaction to MST */
	if (qef_cb->qef_auto == QEF_OFF)
	    qef_cb->qef_stat = QEF_MSTRAN;

	/* Open iirule, iiqrytext and iitree tables */
	rqeu.qeu_type = QEUCB_CB;
        rqeu.qeu_length = sizeof(QEUCB_CB);
        rqeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        rqeu.qeu_lk_mode = DMT_IX;
        rqeu.qeu_flag = DMT_U_DIRECT;
        rqeu.qeu_access_mode = DMT_A_WRITE;
	rqeu.qeu_mask = 0;
	STRUCT_ASSIGN_MACRO(rqeu, qqeu);	/* Quick initialization */
	STRUCT_ASSIGN_MACRO(rqeu, tqeu);

        rqeu.qeu_tab_id.db_tab_base = DM_B_RULE_TAB_ID;    /* Open iirule */
        rqeu.qeu_tab_id.db_tab_index = DM_I_RULE_TAB_ID; 	
	status = qeu_open(qef_cb, &rqeu);
	if (status != E_DB_OK)
	{
	    error = rqeu.error.err_code;
	    break;
	}
	rule_opened = TRUE;

        qqeu.qeu_tab_id.db_tab_base = DM_B_QRYTEXT_TAB_ID; /* Open iiqrytext */
        qqeu.qeu_tab_id.db_tab_index = DM_I_QRYTEXT_TAB_ID; 	
	status = qeu_open(qef_cb, &qqeu);
	if (status != E_DB_OK)
	{
	    error = qqeu.error.err_code;
	    break;
	}
	qtext_opened = TRUE;

        tqeu.qeu_tab_id.db_tab_base = DM_B_TREE_TAB_ID;    /* Open iitree */
        tqeu.qeu_tab_id.db_tab_index = DM_I_TREE_TAB_ID; 	
	status = qeu_open(qef_cb, &tqeu);
	if (status != E_DB_OK)
	{
	    error = tqeu.error.err_code;
	    break;
	}
	tree_opened = TRUE;

	rqeu.qeu_count = 1;	   	    /* Initialize rule-specific qeu */
        rqeu.qeu_tup_length = sizeof(DB_IIRULE);
	rqeu.qeu_output = &rqef_data;
	rqef_data.dt_next = NULL;
        rqef_data.dt_size = sizeof(DB_IIRULE);
        rqef_data.dt_data = (PTR)&rtuple;

	qqeu.qeu_count = 1;	   	    /* Initialize qtext-specific qeu */
        qqeu.qeu_tup_length = sizeof(DB_IIQRYTEXT);
	qqeu.qeu_output = &qqef_data;
	qqef_data.dt_next = NULL;
        qqef_data.dt_size = sizeof(DB_IIQRYTEXT);
        qqef_data.dt_data = (PTR)&qtuple;

	tqeu.qeu_count = 1;	   	    /* Initialize tree-specific qeu */
        tqeu.qeu_tup_length = sizeof(DB_IITREE);
	tqeu.qeu_output = &tqef_data;
	tqef_data.dt_next = NULL;
        tqef_data.dt_size = sizeof(DB_IITREE);
        tqef_data.dt_data = (PTR)&ttuple;

	/*
	** If DROP RULE then position iirule table based on rule name and owner
	** and drop the specific rule (if we can't find it issue an error).
	** Positioning is done via secondary index.  If DROP TABLE then fetch
	** and drop ALL rules applied to the table.
	*/
	rqeu.qeu_getnext = QEU_REPO;
	if (drop_specific_rule)		/* dropping a specific rule */
	{
	    /* open IIRULEIDX */
	    STRUCT_ASSIGN_MACRO(rqeu, xrul1qeu);
	    xrul1qeu.qeu_tab_id.db_tab_base  = DM_B_RULEIDX1_TAB_ID;
	    xrul1qeu.qeu_tab_id.db_tab_index = DM_I_RULEIDX1_TAB_ID;

	    status = qeu_open(qef_cb, &xrul1qeu);	/* open iiruleidx1 */
	    if (status != E_DB_OK)
	    {
		error = xrul1qeu.error.err_code;
		break;
	    }
	    xrul1_opened = TRUE;

	    /* Position based on secondary indices */
	    xrul1qeu.qeu_count = 1;
	    xrul1qeu.qeu_tup_length = sizeof(DB_IIRULEIDX1);
	    xrul1qeu.qeu_output = &xrul1qef_data;
	    xrul1qef_data.dt_next = 0;
	    xrul1qef_data.dt_size = sizeof(DB_IIRULEIDX1);
	    xrul1qef_data.dt_data = (PTR) &xrul1tuple;

	    rtuple_name   = (DB_IIRULE *)qeuq_cb->qeuq_rul_tup->dt_data;
	    xrul1qeu.qeu_klen = 2;
	    xrul1qeu.qeu_key = xrul1key_ptr_array;
	    xrul1key_ptr_array[0] = &xrul1key_array[0];
	    xrul1key_ptr_array[1] = &xrul1key_array[1];
	    xrul1key_ptr_array[0]->attr_number = DM_1_RULEIDX1_KEY;
	    xrul1key_ptr_array[0]->attr_operator = DMR_OP_EQ;
	    xrul1key_ptr_array[0]->attr_value = (PTR)&rtuple_name->dbr_owner;
	    xrul1key_ptr_array[1]->attr_number = DM_2_RULEIDX1_KEY;
	    xrul1key_ptr_array[1]->attr_operator = DMR_OP_EQ;
	    xrul1key_ptr_array[1]->attr_value = (PTR)&rtuple_name->dbr_name;
	    xrul1qeu.qeu_qual = NULL;
	    xrul1qeu.qeu_qarg = NULL;

	    status = qeu_get(qef_cb, &xrul1qeu);
	    if (DB_FAILURE_MACRO(status))
	    {
		if (xrul1qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		{
		    if (from_drop_rule)
		    {
			(VOID)qef_error(E_US189D_6301_RULE_ABSENT, 0L,
				    E_DB_ERROR, &error, &qeuq_cb->error, 1,
				    qec_trimwhite(sizeof(rtuple_name->dbr_name),
						(char *)&rtuple_name->dbr_name),
				    (PTR)&rtuple_name->dbr_name);
			error = E_QE0025_USER_ERROR;
		    }
		    else	/* No rules for table */
		    {
			error = E_QE0000_OK;
			status = E_DB_OK;
		    }
		}
		else
		    error = xrul1qeu.error.err_code;
		break;
	    }
	    
	    status = qeu_close(qef_cb, &xrul1qeu);    
	    xrul1_opened = FALSE;

	    /* Get rule that matches rule name and owner (qualified) */
	    rqeu.qeu_flag = QEU_BY_TID;
	    rqeu.qeu_getnext = QEU_NOREPO;
	    rqeu.qeu_klen = 0;				
	    rqeu.qeu_key = (DMR_ATTR_ENTRY **) NULL;
	    rqeu.qeu_tid = xrul1tuple.dbri1_tidp;
	    rqeu.qeu_qual = NULL;
	    rqeu.qeu_qarg = NULL;
	}
	else				/* DROP TABLE */
	{
	    /* Get rule that applies to table id (keyed) */
	    rqeu.qeu_klen = 2;
	    rqeu.qeu_key = rkey_ptr_array;
	    rkey_ptr_array[0] = &rkey_array[0];
	    rkey_ptr_array[1] = &rkey_array[1];
	    rkey_ptr_array[0]->attr_number = DM_1_RULE_KEY;
	    rkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	    rkey_ptr_array[0]->attr_value =
				    (char *)&qeuq_cb->qeuq_rtbl->db_tab_base;
	    rkey_ptr_array[1]->attr_number = DM_2_RULE_KEY;
	    rkey_ptr_array[1]->attr_operator = DMR_OP_EQ;
	    rkey_ptr_array[1]->attr_value =
				    (char *)&qeuq_cb->qeuq_rtbl->db_tab_index;
	    rqeu.qeu_qual = NULL;
	    rqeu.qeu_qarg = NULL;
	}
	for (;;)		/* For all rule tuples found */
	{
	    status = qeu_get(qef_cb, &rqeu);
	    if (status != E_DB_OK)
	    {
		if (rqeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		{
		    if (from_drop_rule)
		    {
			(VOID)qef_error(E_US189D_6301_RULE_ABSENT, 0L,
				    E_DB_ERROR, &error, &qeuq_cb->error, 1,
				    qec_trimwhite(sizeof(rtuple_name->dbr_name),
						(char *)&rtuple_name->dbr_name),
				    (PTR)&rtuple_name->dbr_name);
			error = E_QE0025_USER_ERROR;
		    }
		    else	/* No rules for table */
		    {
			error = E_QE0000_OK;
			status = E_DB_OK;
		    }
		}
		else	/* Other error */
		{
		    error = rqeu.error.err_code;
		}
		break;
	    } /* If no rules found */
	    rqeu.qeu_getnext = QEU_NOREPO;
            rqeu.qeu_klen = 0;
	    
	    if (qeuq_cb->qeuq_flag_mask & QEU_DROP_COLUMN)
	    {
	        ULM_RCB   ulm;
		DB_TREE_ID tree_id;
		DB_ERROR  err_blk;
		bool	  cascade;
		bool      drop_depobj = FALSE;

	        if (( (BTtest (qeuq_cb->qeuq_ano,
			(char *) rtuple.dbr_columns.db_domset)) &&
		    (BTcount((char *) rtuple.dbr_columns.db_domset,
			DB_COL_BITS) != DB_COL_BITS) ) &&
	            ( !(qeuq_cb->qeuq_flag_mask & QEU_DROP_CASCADE) ))
	        {
		  error = E_QE016B_COLUMN_HAS_OBJECTS;
		  status = E_DB_ERROR;
		  break;
	        }

		  /* b108474 (b108635)
		  ** If dependent referential constraint has been
		  ** dropped previously, have this rule dropped to
		  ** otherwise leave it as it is.
		  */
		  if (qeuq_cb->qeuq_flag_mask & QEU_DEP_CONS_DROPPED)
		  {
			drop_depobj=TRUE;
		  }
		  else
		  {
			/* check if there is a dependent object ? */
			if (( (BTtest (qeuq_cb->qeuq_ano,
			    (char *) rtuple.dbr_columns.db_domset)) &&
			    (BTcount((char *) rtuple.dbr_columns.db_domset,
			     DB_COL_BITS) != DB_COL_BITS) )) 
			   drop_depobj = TRUE;	
		  }

		STRUCT_ASSIGN_MACRO(rtuple.dbr_treeid, tree_id);
		if (tree_id.db_tre_high_time != 0 || 
		    tree_id.db_tre_low_time != 0)
		{
		    if ((tree_id.db_tre_high_time == 
					last_tree_id.db_tre_high_time) &&
		        (tree_id.db_tre_low_time == 
					last_tree_id.db_tre_low_time))
		    {
			if (drop_specific_rule)	 /* dropping a specific rule */
			{
			    error = E_QE0000_OK;
			    status = E_DB_OK;
			    break;
			}
			else
		            continue;
		    }
		    else
		        STRUCT_ASSIGN_MACRO(tree_id, last_tree_id);
			
		    /*
		    **  Create a stream for iitree tuples
		    **  Call qeu_v_col_drop to fix up iitree entries for 
		    **  this rule
		    */
		
		    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm);
		    ulm.ulm_streamid = (PTR)NULL;
		    if ((status = qec_mopen(&ulm)) != E_DB_OK)
		    {
	                error = E_QE001E_NO_MEM;
		        status = E_DB_ERROR;
		        break;
		    }

		    cascade = (qeuq_cb->qeuq_flag_mask & QEU_DROP_CASCADE) ? 
				TRUE:FALSE;
		    if ((status = qeu_v_col_drop(&ulm, qef_cb,
				&tqeu, (DB_TAB_ID *) NULL, &rtuple.dbr_tabid,
				&tree_id, (i2) 0,
				&rtuple.dbr_name, &rtuple.dbr_owner,
				DB_RULE, qeuq_cb->qeuq_ano,
				cascade,   
				&err_blk)) != E_DB_OK)
		    {
		        ulm_closestream(&ulm);
		        error = err_blk.err_code;
		        status = E_DB_ERROR;
		        break;
		    }
		
		    ulm_closestream(&ulm);
		}
		    
		/* 
		** We have to delete the rows now  so we cannot 
		** just continue 
		*/
		if (!drop_depobj)
		    continue;
	    }

	    /* don't allow users to drop system-generated rules
	     */
	    if ((from_drop_rule) && (rtuple.dbr_flags & DBR_F_NOT_DROPPABLE))
	    {
		(VOID)qef_error(E_US18AB_6315_NOT_DROPPABLE, 0L, E_DB_ERROR, 
				&error, &qeuq_cb->error, 2,
				sizeof(ERx("DROP RULE"))-1, ERx("DROP RULE"),
				qec_trimwhite(sizeof(rtuple_name->dbr_name),
					      rtuple_name->dbr_name.db_name),
				rtuple_name->dbr_name.db_name);
		error = E_QE0025_USER_ERROR;
		break;
	    }

	    /*
	    ** If processing DROP RULE, copy table id and rule name into
	    ** caller's rule tuple area.  This will allow caller to invalidate
	    ** table info on RDF cache.
	    */
	    if (from_drop_rule)
	    {
		STRUCT_ASSIGN_MACRO(rtuple.dbr_tabid, rtuple_name->dbr_tabid);
		MEcopy((PTR)&rtuple.dbr_owner, sizeof(DB_OWN_NAME),
			(PTR)&rtuple_name->dbr_owner);
	    }
            /*
	    ** If the rule tuple has a tree then position iitree and delete
	    ** that tuple.  If there's a tree then this must be a table-type
	    ** rule, and then the tree is positioned based on table id so we
	    ** must get them all and just delete the one we are looking for.
	    */
	    if (   rtuple.dbr_treeid.db_tre_high_time != 0
		|| rtuple.dbr_treeid.db_tre_low_time != 0)
	    {
		tkey_ptr_array[0] = &tkey_array[0];
		tkey_ptr_array[1] = &tkey_array[1];
		tqeu.qeu_getnext = QEU_REPO;
		tqeu.qeu_klen = 2;       
		tqeu.qeu_key = tkey_ptr_array;
		tkey_ptr_array[0]->attr_number = DM_1_TREE_KEY;
		tkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
		tkey_ptr_array[0]->attr_value =
				(char *)&rtuple.dbr_tabid.db_tab_base;
		tkey_ptr_array[1]->attr_number = DM_2_TREE_KEY;
		tkey_ptr_array[1]->attr_operator = DMR_OP_EQ;
		tkey_ptr_array[1]->attr_value =
				(char *)&rtuple.dbr_tabid.db_tab_index;
		tqeu.qeu_qual = NULL;
		tqeu.qeu_qarg = NULL;

		/* 
		** just reset the output which might have been reset 
		** by drop col routine
		*/
		tqeu.qeu_output = &tqef_data;
		tqeu.qeu_count = 1;        /* Initialize tree-specific qeu */

		for (;;)	/* For all tree tuples */
		{
		    status = qeu_get(qef_cb, &tqeu);
		    if (status != E_DB_OK)
		    {
			error = tqeu.error.err_code;
			break;
		    }
		    tqeu.qeu_getnext = QEU_NOREPO;
		    tqeu.qeu_klen = 0;
		    /* Make sure it's ours */
		    if (   (rtuple.dbr_treeid.db_tre_high_time != 
			    ttuple.dbt_trid.db_tre_high_time)
		        || (rtuple.dbr_treeid.db_tre_low_time !=
			    ttuple.dbt_trid.db_tre_low_time)
		       )
			continue;
		    status = qeu_delete(qef_cb, &tqeu);
		    if (status != E_DB_OK)
		    {
			error = tqeu.error.err_code;
			break;
		    }
		} /* For all tree tuples */
		if (error != E_QE0015_NO_MORE_ROWS)
		    break;
	    } /* If there was a tree */
	    status = E_DB_OK;

            /* Postion iiqrytxt table for query id associated with this rule */
	    qkey_ptr_array[0] = &qkey_array[0];
	    qkey_ptr_array[1] = &qkey_array[1];
	    qqeu.qeu_getnext = QEU_REPO;
	    qqeu.qeu_klen = 2;       
	    qqeu.qeu_key = qkey_ptr_array;
	    qkey_ptr_array[0]->attr_number = DM_1_QRYTEXT_KEY;
	    qkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	    qkey_ptr_array[0]->attr_value =
				(char *)&rtuple.dbr_txtid.db_qry_high_time;
	    qkey_ptr_array[1]->attr_number = DM_2_QRYTEXT_KEY;
	    qkey_ptr_array[1]->attr_operator = DMR_OP_EQ;
	    qkey_ptr_array[1]->attr_value =
				(char *)&rtuple.dbr_txtid.db_qry_low_time;
	    qqeu.qeu_qual = NULL;
	    qqeu.qeu_qarg = NULL;
	    for (;;)		/* For all query text tuples */
	    {
		status = qeu_get(qef_cb, &qqeu);
		if (status != E_DB_OK)
		{
		    error = qqeu.error.err_code;
		    break;
		}
		qqeu.qeu_klen = 0;
		qqeu.qeu_getnext = QEU_NOREPO;
		status = qeu_delete(qef_cb, &qqeu);
		if (status != E_DB_OK)
		{
		    error = qqeu.error.err_code;
		    break;
		}
	    } /* For all query text tuples */
	    if (error != E_QE0015_NO_MORE_ROWS)
		break;

	    /* Now delete the current rule tuple */
	    status = qeu_delete(qef_cb, &rqeu);
	    if (status != E_DB_OK)
	    {
		error = rqeu.error.err_code;
		break;
	    }
		
	    /* Security audit rule drop. */

	    if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	    {
	        DB_ERROR e_error;

	        status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
	    		(char *)&rtuple.dbr_name,
			&rtuple.dbr_owner,
	    		sizeof(rtuple.dbr_name), SXF_E_RULE,
	      		I_SX2038_RULE_DROP, SXF_A_DROP | SXF_A_SUCCESS,
	      		&e_error);

	        error = e_error.err_code;

		if (status != E_DB_OK)
		    break;		
	    }

	    /*
	    ** If  doing a specific DROP RULE we're done with this loop.
	    ** Otherwise continue with next rule applied to table.
	    */
	    if (drop_specific_rule)
		break;
	} /* For all rules found */

	if (status != E_DB_OK)
	    break;

	/* 
	** If we're deleting a single rule tuple, fetch all tuples left that
	** correspond to the same table id.  If none are left then turn off
	** the rule relstat bit.  Don't bother if dropping the table, as the
	** iirelation tuple gets deleted anyway.
	*/
	if ( (drop_specific_rule) ||
	     (qeuq_cb->qeuq_flag_mask & QEU_DROP_COLUMN) )
	{
	    /* Assign to temp so as not to write over input keys */
	    STRUCT_ASSIGN_MACRO(rtuple.dbr_tabid, rtab);

	    /* Find at least one rule tuple that has the same table id */
	    rqeu.qeu_getnext = QEU_REPO;
	    rqeu.qeu_klen = 2;
	    rqeu.qeu_key = rkey_ptr_array;
	    rkey_ptr_array[0] = &rkey_array[0];
	    rkey_ptr_array[1] = &rkey_array[1];
	    rkey_ptr_array[0]->attr_number = DM_1_RULE_KEY ;
	    rkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	    rkey_ptr_array[0]->attr_value = (char *)&rtab.db_tab_base;
	    rkey_ptr_array[1]->attr_number = DM_2_RULE_KEY ;
	    rkey_ptr_array[1]->attr_operator = DMR_OP_EQ;
	    rkey_ptr_array[1]->attr_value = (char *)&rtab.db_tab_index;
	    rqeu.qeu_qual = NULL;
	    rqeu.qeu_qarg = NULL;
	    status = qeu_get(qef_cb, &rqeu);
	    if ((  status != E_DB_OK
		&& rqeu.error.err_code == E_QE0015_NO_MORE_ROWS) ||
		(status == E_DB_OK))
	    {
		char_spec.char_id = DMT_C_RULE;
		if (status == E_DB_OK)
		{
		    char_spec.char_value = DMT_C_ON;
		}
		else
		{
		    /* No rules left for this table */
		    char_spec.char_value = DMT_C_OFF;
		}

		MEfill(sizeof(DMT_CB), 0, (PTR) &dmt_cb);
		dmt_cb.dmt_db_id = qeuq_cb->qeuq_db_id;
		dmt_cb.dmt_char_array.data_in_size = sizeof(DMT_CHAR_ENTRY);
		dmt_cb.dmt_char_array.data_address = (PTR)&char_spec;
		dmt_cb.length = sizeof(DMT_CB);
		dmt_cb.type = DMT_TABLE_CB;
		dmt_cb.dmt_id.db_tab_base  = rtab.db_tab_base;
		dmt_cb.dmt_id.db_tab_index = rtab.db_tab_index;
		dmt_cb.dmt_tran_id = qef_cb->qef_dmt_id;
		status = dmf_call(DMT_ALTER, &dmt_cb);
		if (status != E_DB_OK)
		{
		    error = dmt_cb.error.err_code;
		    if (error == E_DM0054_NONEXISTENT_TABLE)
		    {
			/* How was table removed? */
			(VOID)qef_error(E_US18A8_6312_RULE_DRPTAB, 0L,
				    E_DB_ERROR, &error, &qeuq_cb->error, 1,
				    qec_trimwhite(sizeof(rtuple_name->dbr_name),
						(char *)&rtuple_name->dbr_name),
				    (PTR)&rtuple_name->dbr_name);
			error = E_QE0025_USER_ERROR;
			status = E_DB_OK;	/* Still remove the rule */
		    }
		    break;
		}
	    } /* If found no more rules applied to table */

	    /* 
	    ** if caller stipulated that QPs which may involve this rule need
	    ** to be invalidated, determine whether the rule was defined on a 
	    ** view and if so, alter timestamp of some base table on which
	    ** the view depends
	    */
	    if (qeuq_cb->qeuq_flag_mask & QEU_FORCE_QP_INVALIDATION)
	    {
                status = qeu_force_qp_invalidation(qef_cb, &rtab, (bool) TRUE, 
		    (bool *) NULL, &qeuq_cb->error);
	        if (DB_FAILURE_MACRO(status))
		{
		    err_already_reported = TRUE;
	            break;
		}
	    }

	} /* if dropping a specific rule */
	break;
    } /* End dummy for loop */

    /* Handle any error messages */
    if (status != E_DB_OK  && !err_already_reported)
	(VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error, 0);
    
    /* Close off all the tables and transaction */
    if (rule_opened)
    {
	local_status = qeu_close(qef_cb, &rqeu);    
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(rqeu.error.err_code, 0L, local_status, 
			     &error, &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }
    if (qtext_opened)
    {
	local_status = qeu_close(qef_cb, &qqeu);    
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(qqeu.error.err_code, 0L, local_status, 
			     &error, &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }
    if (tree_opened)
    {
	local_status = qeu_close(qef_cb, &tqeu);    
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(tqeu.error.err_code, 0L, local_status, 
			     &error, &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }
    if (xrul1_opened)
    {
	local_status = qeu_close(qef_cb, &xrul1qeu);    
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(xrul1qeu.error.err_code, 0L, local_status, 
			     &error, &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }
    if (transtarted)
    {
	if (status == E_DB_OK)
	    local_status = qeu_etran(qef_cb, &tranqeu);
	else
	    local_status = qeu_atran(qef_cb, &tranqeu);
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(tranqeu.error.err_code, 0L, local_status, 
			     &error, &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }
    return (status);
} /* qeu_drule */
