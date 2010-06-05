/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <bt.h>
#include    <er.h>
#include    <lo.h>
#include    <me.h>
#include    <pm.h>
#include    <qu.h>
#include    <st.h>
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

/**
**
**  Name: psyrules.c - Rule processor
**
**  Description:
**	This module contains the rule processor.  This processor modifies
**	the query to add rule firing statements.
**
**  Defines:
**	psy_rule 	- Apply rules to a statement.
**	psy_drule	- Define a rule.
**	psy_krule	- Destroy a rule.
**
**
**  History:
**	09-mar-89 (neil)
**	    Written for Terminator/rules.
**	18-apr-89 (jrb)
**	    Prec field cleanups and pst_node interface change for decimal
**	    project.
**	20-apr-89 (neil)
**	    Modified psy_rules for cursor support.
**	09-may-89 (neil)
**	    Added saving of rule name for tracing.
**	08-sep-89 (neil)
**	    PSY_CB field psy_rule is now  psy_tuple.psy_rule.
**	26-sep-89 (neil)
**	    Defined PSY_QUEL_RULES - allows rule application in QUEL.
**	05-dec-90 (ralph)
**	    copy rule owner name to pst_cprocstmt (b34531)
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	03-aug-92 (barbara)
**	    Initialize RDF_CB by calling pst_rdfcb_init before calling RDF.
**	30-jul-92 (rickh)
**	    FIPS CONSTRAINTS:  changed DBR_T_TABLE to DBR_T_ROW.
**	           also changed uses of dbr_column to dbr_columns; 
**                 it is now a bitmap instead of an i2.   (rblumer)
**	16-nov-92 (rblumer)
**	    Initialize upd_cols even for non-UPDATE statements, as now
**	    pst_ruledup always looks at its value when passed a non-zero filter.
**	08-dec-92 (anitap)
**	    Changed the order of #includes of header files for CREATE SCHEMA.
**	15-dec-92 (wolf) 
**	    ulf.h must be included before qsf.h
**	21-dec-92 (rblumer)
**	    In psy_drule, suppress text for system-generated rules
**	    (i.e., don't store text in iiqrytext catalog);
**	    in psy_rules, put statement-level rules on a separate rule list,
**	    and put system-generated rules at the end of each rule list.
**	17-mar-93 (rblumer)
**	    in psy_rules, attach the stmt-level rule list to the end of the
**	    row-level rule list, to simplify processing in OPC;
**	    added static function attach_rule_lists.
**	11-jun-93 (rblumer)
**	    in psy_drule, Store RDF_INVALIDATE return status in loc_status;
**	    otherwise we can overwrite the error status of an earlier call.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-aug-93 (andre)
**	    fixed causes of compiler warnings
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	23-May-1997 (shero03)
**	    Save the rdr_info_blk after UNFIX
**	12-dec-2000 (somsa01)
**	    Changed references from STcasecmp() to STbcompare(), as the
**	    ST changes have not been crossed into the ingres25 code line.
**      01-Dec-2000 (hanal04) Bug 100680 INGSRV 1123
**          Updating rules fired by updates can loop indeterminately
**          if the update causes the row to move position in the table.
**          Flag the use of the prefetch solution unless it has been
**          disabled by setting the config parameter rule_upd_prefetch
**          to OFF. This change reworks inkdo01's change 448530.
**	10-Jan-2001 (jenjo02)
**	    Supply session's SID to QSF in qsf_sid.
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp(),
**	    pst_treedup().
**	04-Oct-2002 (gygro01) b108628 / ingsrv 1872
**	    Statement level rule with qualification and wihtout dbp
**	    parameters defined won't apply the qualification due to
**	    implementation of stmt-level-rules in qef.
**	    Flagging such rules as row level rules in order to
**	    get the qualification applied correctly (psy_rule()).
**	    Anyway does not make much sense to use a stmt-level-rule,
**	    if no params needs to be passed to the procedure.
**      22-feb-2006 (stial01)
**          psy_rules() Set flag if system generated rule (b115765)
**      30-aug-06 (thaju02)
**          In psy_rules(), if delete query and PSS_RULE_DEL_PREFETCH, 
**          then set PST_DEL_PREFETCH bit in the pst_pmask. (B116355)
**      01-apr-2010 (stial01)
**          Changes for Long IDs
[@history_template@]...
**/

/* To turn QUEL rules off remove this closing comment 	     ---> */
#define	    PSY_QUEL_RULES	/* Rules in QUEL - can turn off ^ */

/*{
** Name: psy_drule	- Define a rule.
**
**  INTERNAL PSF call format: status = psy_drule(&psy_cb, &sess_cb);
**
**  EXTERNAL call format:     status = psy_call(PSY_DRULE, &psy_cb);
**
** Description:
**      This routine stores the definition of a table-type and time-type rule
**	in the system table iirule and iiqrytext.  If there's a tree
**	associated with the table-type rule (a parameter list and/or a
**	qualification) then the tree is stored in iitree.
**
**	The actual work of storing the rule is done by RDF and QEU.  Note
**	that the rule name is not yet validated within the user scope and
**	QEU may return a "duplicate rule" error.
**
** Inputs:
**      psy_cb
**	    .psy_tuple.psy_rule		Rule tuple to insert.  All
**					user-specified fields of this
**					parameter are filled in by the
**					grammar.  Internal fields (tree and
**					text ids) are filled in just before
**					storage in QEU when the ids are
**					constructed.  The owner of the rule
**					is filled in this routine from
**					pss_user before storing.
**          .psy_qrytext                Id of query text as stored in QSF.
**	    .psy_istree			TRUE if there is a tree.
**          .psy_intree                 Query tree representing parameter list
**					and/or qualification of rule if
**					psy_istree is set.
**					integrity if psy_istree is set.
**	sess_cb				Pointer to session control block.
**
** Outputs:
**      psy_cb
**	    .psy_error.err_code		Filled in if an error happens:
**		E_PS0D18_QSF_LOCK 	  Cannot lock text or tree.
**		E_PS0D19_QSF_INFO	  Cannot get information for text/tree.
**		E_PS0D1A_QSF_DESTROY	  Could not destroy text or tree.
**		E_PS0903_TAB_NOTFOUND	  Table not found.
**		Generally, the error code returned by RDF and QEU
**		qrymod processing routines are passed back to the caller.
**
**	Returns:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	Stores query tree in iitree, query text in iiqrytext, rule tuple in
**	iirule identifying the rule.  Does a DMF alter table to indicate
**	that the table has rules on it.
**
** History:
**	09-mar-89 (neil)
**	   Written for Terminator/rules.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	03-aug-92 (barbara)
**	    Invalidate tableinfo from RDF's cache.
**	30-jul-92 (rickh)
**	    FIPS CONSTRAINTS:  changed DBR_T_TABLE to DBR_T_ROW.
**	16-nov-92 (rblumer)
**	    Initialize upd_cols even for non-UPDATE statements, as now
**	    pst_ruledup always looks at its value when passed a non-zero filter.
**	21-dec-92 (rblumer)
**	    Suppress text for system-generated rules
**	    (i.e., don't store text in iiqrytext catalog).
**	11-jun-93 (rblumer)
**	    Store RDF_INVALIDATE return status in loc_status, not status,
**	    otherwise we can overwrite the error status of an earlier call.
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for qsf_owner which has changed type to PTR.
**	18-jul-96 (inkdo01)
**	    Removed code which suppressed query text of system-generated
**	    rules. This syntax has now been externalized and is ok to 
**	    store on the catalog.
*/

DB_STATUS
psy_drule(
	PSY_CB	   *psy_cb,
	PSS_SESBLK *sess_cb)
{
    RDF_CB		rdf_cb;
    RDR_RB		*rdf_rb = &rdf_cb.rdf_rb;
    i4			textlen;		/* Length of query text */
    QSF_RCB		qsf_rb;
    DB_STATUS		status, loc_status;
    i4		err_code;

    /* Fill in QSF request to lock text and tree */
    qsf_rb.qsf_type	= QSFRB_CB;
    qsf_rb.qsf_ascii_id	= QSFRB_ASCII_ID;
    qsf_rb.qsf_length	= sizeof(qsf_rb);
    qsf_rb.qsf_owner	= (PTR)DB_PSF_ID;
    qsf_rb.qsf_sid	= sess_cb->pss_sessid;

    /* Initialize RDF_CB */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    rdf_rb->rdr_types_mask  = RDR_RULE;			/* Rule definition */
    rdf_rb->rdr_update_op   = RDR_APPEND;
    rdf_rb->rdr_qrytuple    = (PTR)&psy_cb->psy_tuple.psy_rule; /* Rule tuple */

    /* If there's a tree get the tree id from QSF */
    if (psy_cb->psy_istree)
    {
	STRUCT_ASSIGN_MACRO(psy_cb->psy_intree, qsf_rb.qsf_obj_id);
	status = qsf_call(QSO_INFO, &qsf_rb);
	if (status != E_DB_OK)
	{
	    psf_error(E_PS0D19_QSF_INFO, qsf_rb.qsf_error.err_code,
		      PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	    goto cleanup;
	}
	rdf_rb->rdr_qry_root_node = qsf_rb.qsf_root;	/* Tree root */
	STRUCT_ASSIGN_MACRO(psy_cb->psy_tuple.psy_rule.dbr_tabid,
			    rdf_rb->rdr_tabid);
    }
    else
    {
	/*
	** No tree root - maybe no parameters and qualification, or maybe
	** a time-type rule.
	*/
	rdf_rb->rdr_qry_root_node      = NULL;		/* No tree root */
	rdf_rb->rdr_tabid.db_tab_base  = 0;
	rdf_rb->rdr_tabid.db_tab_index = 0;
    } /* If there's a tree */

    /*
    ** Get the query text from QSF.  QSF has stored the text 
    ** as a {nat, string} pair - maybe not be aligned.
    */
    STRUCT_ASSIGN_MACRO(psy_cb->psy_qrytext, qsf_rb.qsf_obj_id);
    status = qsf_call(QSO_INFO, &qsf_rb);
    if (status != E_DB_OK)
    {
	psf_error(E_PS0D19_QSF_INFO, qsf_rb.qsf_error.err_code,
		  PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	goto cleanup;
    }
    MEcopy((PTR)qsf_rb.qsf_root, sizeof(i4), (PTR)&textlen);
    rdf_rb->rdr_l_querytext	= textlen;
    rdf_rb->rdr_querytext	= ((char *)qsf_rb.qsf_root) + sizeof(i4);

    /* There used to be code here which blanked out the text of system-
    ** generated rules (as in referential/check constraints). This was
    ** because they contained unapproved syntax. This syntax has now been 
    ** sanitized and approved, so the text suppression is no longer
    ** required. */

    /* Assign user */
    STRUCT_ASSIGN_MACRO(sess_cb->pss_user,
			psy_cb->psy_tuple.psy_rule.dbr_owner);

    /* Create a new rule in iirule */
    status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
    if (status != E_DB_OK)
    {
	if (   rdf_cb.rdf_error.err_code == E_RD0002_UNKNOWN_TBL
	    && psy_cb->psy_tuple.psy_rule.dbr_type == DBR_T_ROW)
	{
	    psf_error(E_PS0903_TAB_NOTFOUND, 0L, PSF_USERERR, &err_code,
		      &psy_cb->psy_error, 1, 
		      psf_trmwhite(sizeof(psy_cb->psy_tabname[0]),
				   (char *) &psy_cb->psy_tabname[0]),
		      &psy_cb->psy_tabname[0]);
	}
	else
	{
	    _VOID_ psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error,
				 &psy_cb->psy_error);
	}
	goto cleanup;
    } /* If RDF error */

cleanup:
    qsf_rb.qsf_lk_state	= QSO_EXLOCK;
    if (psy_cb->psy_istree)		/* If there's a tree - destroy it */
    {
	/* We must lock it to destroy it */
	STRUCT_ASSIGN_MACRO(psy_cb->psy_intree, qsf_rb.qsf_obj_id);
	loc_status = qsf_call(QSO_LOCK, &qsf_rb);
	if (loc_status != E_DB_OK)
	{
	    psf_error(E_PS0D18_QSF_LOCK, qsf_rb.qsf_error.err_code,
		      PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	    if (loc_status > status)
		status = loc_status;
	}
	loc_status = qsf_call(QSO_DESTROY, &qsf_rb);
	if (loc_status != E_DB_OK)
	{
	    psf_error(E_PS0D1A_QSF_DESTROY, qsf_rb.qsf_error.err_code,
		      PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	    if (loc_status > status)
		status = loc_status;
	}
    } /* If there was a tree */    

    /* Destroy query text - lock it first */
    STRUCT_ASSIGN_MACRO(psy_cb->psy_qrytext, qsf_rb.qsf_obj_id);
    loc_status = qsf_call(QSO_LOCK, &qsf_rb);
    if (loc_status != E_DB_OK)
    {
	psf_error(E_PS0D18_QSF_LOCK, qsf_rb.qsf_error.err_code,
		  PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	if (loc_status > status)
	    status = loc_status;
    }
    loc_status = qsf_call(QSO_DESTROY, &qsf_rb);
    if (loc_status != E_DB_OK)
    {
	psf_error(E_PS0D1A_QSF_DESTROY, qsf_rb.qsf_error.err_code,
		  PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	if (loc_status > status)
	    status = loc_status;
    }

    /* Invalidate base object's tableinfo from RDF cache */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    STRUCT_ASSIGN_MACRO(psy_cb->psy_tuple.psy_rule.dbr_tabid,
				rdf_rb->rdr_tabid);
    loc_status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_cb);
    if (DB_FAILURE_MACRO(loc_status))
    {
	(VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_cb.rdf_error,
				&psy_cb->psy_error);
	if (loc_status > status)
	    status = loc_status;
    }

    return (status);
} /* psy_drule */

/*{
** Name: psy_krule	- Destroy a rule.
**
**  INTERNAL PSF call format: status = psy_krule(&psy_cb, &sess_cb, &qeu_cb);
**
**  EXTERNAL call format:     status = psy_call(PSY_KRULE, &psy_cb, &sess_cb);
**
** Description:
**      This removes the definition of a rule from the system tables.
**
**	The actual work of deleting the rule is done by RDF and QEU.  Note
**	that the rule name is not yet validated within the user scope and
**	QEU may return an "unknown rule" error.
**
** Inputs:
**      psy_cb
**	    .psy_tuple.psy_rule		Rule tuple to delete.
**		.psy_name		Name of rule.  The owner of the
**					rule is filled in this routine
**					from pss_user before calling
**					RDR_UPDATE.
**					integrity if psy_istree is set.
**	sess_cb				Pointer to session control block.
**	qeu_cb				Initialized QEU_CB.
**
** Outputs:
**      psy_cb
**	    .psy_error.err_code		Filled in if an error happens:
**		    Generally, the error code returned by RDF and QEU qrymod
**		    processing routines are passed back to the caller.
**	Returns:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	Deletes query tree from iitree, query text from iiqrytext, rule tuple
**	from iirule.  May do a DMF alter table to indicate that the table
**	has no rules on it.
**
** History:
**	09-mar-89 (neil)
**	   Written for Terminator/rules.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	03-aug-92 (barbara)
**	    Invalidate infoblk from rdf cache.
**	07-aug-92 (teresa)
**	    Must make two calls to RDF_INVALID, one to invalidate the relation
**	    object the rule was defined on and the other invalidating the
**	    rule tree from the qtree cache.
*/

DB_STATUS
psy_krule(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    RDF_CB              rdf_cb;
    RDR_RB		*rdf_rb = &rdf_cb.rdf_rb;
    DB_STATUS		status;

    /* Assign user */
    STRUCT_ASSIGN_MACRO(sess_cb->pss_user,psy_cb->psy_tuple.psy_rule.dbr_owner);

    /* Fill in the RDF request block */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    rdf_rb->rdr_types_mask  = RDR_RULE;			/* Rule deletion */
    rdf_rb->rdr_update_op   = RDR_DELETE;
    rdf_rb->rdr_qrytuple    = (PTR)&psy_cb->psy_tuple.psy_rule;	/* Rule tuple */

    status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);	/* Destroy rule */
    if (status != E_DB_OK)
	_VOID_ psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, &psy_cb->psy_error);


    /*
    ** Invalidate base object's infoblk off rdf cache: supply RDF
    ** with rule name, owner, underlying table's id and set flag
    ** to RDR2_ALIAS to invalidate tree
    */

    pst_rdfcb_init(&rdf_cb, sess_cb);
    MEcopy((PTR)&psy_cb->psy_tuple.psy_rule.dbr_owner,
		    sizeof(DB_OWN_NAME), (PTR)&rdf_rb->rdr_owner);
    MEcopy((PTR)&psy_cb->psy_tuple.psy_rule.dbr_name,
		    sizeof(DB_NAME), (PTR)&rdf_rb->rdr_name.rdr_rlname);
    rdf_rb->rdr_types_mask = RDR_RULE;
    STRUCT_ASSIGN_MACRO(psy_cb->psy_tuple.psy_rule.dbr_tabid,
			rdf_rb->rdr_tabid);

    /* invalidate the infoblk of table rule applies to from RDF's relation cache
    */
    status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_cb);
    if (DB_FAILURE_MACRO(status))
	(VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_cb.rdf_error,
			&psy_cb->psy_error);

    /* invalidate the rule tree from RDF's qtree cache */
    rdf_rb->rdr_2types_mask |= RDR2_ALIAS;
    rdf_rb->rdr_types_mask = RDR_RULE;
    status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_cb);
    if (DB_FAILURE_MACRO(status))
	(VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_cb.rdf_error,
			&psy_cb->psy_error);

    return (status);
} /* psy_krule */


/*
** Name: psy_rl_coalesce - attach one rule list to end of another
**
** Description:	    
**          Finds end of first rule list, then attaches 2nd rule list.
**          Handles both CP node rules and IF->CP node rules.
**
** Inputs:
**	list1		first rule list
**	list2		list to attach to list1
**
** Outputs:
**	list1		now points to the combined list of rule statements
**	
** Returns:
**	none
**
** Side effects:
**	none
**
** History:
**
**	17-mar-93 (rblumer)
**	    written
**	08-apr-93 (andre)
**	    renamed Roger's attach_rule_lists to psy_rl_coalesce because it will
**	    have to be externaly visible to be used by pst_header() and
**	    pst_execute().  Because rules can now be defined on views,
**	    psy_rules() will no longer be in business of coalescing rule lists.
**	    This responsibility will go to pst_header and pst_execute()
*/
void
psy_rl_coalesce(
		  PST_STATEMENT **list1p,
		  PST_STATEMENT *list2)
{
    if (list2 == (PST_STATEMENT *) NULL)
	return;

    if (*list1p == (PST_STATEMENT *) NULL)
    {
	*list1p = list2;
    }
    else /* list1p non-null */
    {
	PST_STATEMENT	*last_rule;

	/* find last statement in the rule list
	** and attach the 2nd list of rules to it
	*/
	for (last_rule = *list1p; 
	     last_rule->pst_next != (PST_STATEMENT *) NULL;
	     last_rule = last_rule->pst_next)
	    ;

	/* if last statement is an CALL PROC (CP) statement,
	** just connect it up to the 2nd rule list;
	** else the statement is an IF statement and
	** we have to fix up conditionals and other links
	*/
	if (last_rule->pst_type == PST_CP_TYPE)
	{
	    last_rule->pst_next = list2;
	    last_rule->pst_link = list2;
	}
	else
	{
	    /* We have an IF node--
	    ** Have to make sure TRUE and FALSE branches point to the NEXT
	    ** statement (which is the 2nd rule list).
	    ** Since we know the TRUE branch has exactly 1 statement and the
	    ** FALSE branch has no statements, this is relatively easy to do.
	    */
	    last_rule->pst_specific.pst_if.pst_false          = list2;
	    last_rule->pst_specific.pst_if.pst_true->pst_next = list2;
	    last_rule->pst_next = list2;

	    /* since we know the link from the IF node is linked to the CP node
	    ** which is linked to NULL, we can link the 2nd rule list 
	    ** to the 1st list to make sure the pst_link list stays unbroken
	    */
	    last_rule->pst_link->pst_link = list2;
	}
    } /* end else list1p non-null */
    
    return;
}  /* end psy_rl_coalesce */

/*{
** Name: psy_rules	- Apply rules modifications to statement tree.
**
** Description:
**      This function applies rules processing to the current statement.
**	The statement is checked for valid rule-applicable statements and
**	rules that apply to the current statement are attached to one of the
**	lists maintained in sess_cb.  Rule processing is done as a part of
**	qrymod since rules can now be defined on views.
**
**	If rules can apply and the current table has the rules bit set
**	(DMT_RULE), rule tuples satisfying this statement are extracted from
**	RDF. "Satisfying" means the rule refers to the current table, the
**	rule statement is the same as the current statement, and, if 
**	specific column(s) were listed in the firing condition, then pne or more
**	of them are referenced in the target list of this statement.
**
**	Each rule tuple extracted may also have a condition and/or parameter
**	list associated with it.  These subordinate values are also extracted
**	from RDF via the tree pointed at within the rule tuple.
**
**	If the rule has conditions associated with it this routine does
**	not try to check the condition, as syntactic comparisons alone would
**	not be enough.  For example, fire rule R when C = 10; this routine
**	does not check if the expression C = 9+1 is or is not 10.
**
**	Even though procedure names (or aliases) are read out of the rule
**	tuple, this routine does not verify that the procedure does exist
**	or are loaded.  This would have too high an overhead for rules that
**	are not actually fired.  Mid-execution procedure recreation is
**	handled elsewhere.
**
**	After reading the rule tuple, a statement tree is built which may
**	check a run-time condition, and then call a procedure.  If more than
**	one rule can apply, another rule statement construct is added to
**	the appropriate list (see note below).  This routine "pushes" them
**	in order that they were read, thus they will probably be executed
**	in reverse order that they were returned from RDF.
**
**	Rule lists built by (possibly multiple) invocations of psy_rules() will
**	be coalesced (system-generated row-level rules will be appended to
**	user-defined ones and system-generated statement-level rules will be
**	appended to user-defined ones) in pst_header() or psl_init_dbp_stmt().
**	For prepared statements, row- and statement-level rule lists will be
**	coalesced in pst_execute(); otherwise they will be coalesced in
**	pst_header() or psl_init_dbp_stmt().  Before handing over a query tree
**	to OPF, pst_after_stmt will point at the list of row-level rules with
**	statement-level rules attached at the end, and pst_statementEndrules
**	will point at the beginning of the list of statement-level rules.
**
**	The trees extracted from RDF may reference OLD and/or NEW column
**	values (PST_BEFORE/PST_AFTER).  Even though OLD cannot apply
**	to an INSERT statement, and NEW cannot apply to a DELETE
**	statement, psy_rules will not traverse the tree to fix up the
**	node types, as OPC already handles these cases.
**	
**   Note:  Multiple rule lists
**
**      As mentioned above, we will distinguish four types of rules:
**	user-defined row-level, system-generated row-level, user-defined
**	statement-level and system-generated statement-level.  Separate lists
**	will be maintained in sess_cb until we are done with qrymod and are
**	building the statement node because rules now can be defined on views as
**	well as base tables.
**	before we return.
**
**   Cursor Note:
**
**	When processing cursor statements on the initial DELETE or UPDATE
**	statement we retrieve all rules and construct ALL the DELETE and
**	UPDATE statement trees (as described above).  Since they are all
**	built we tag each rule statement with the type (and optional column)
**	of the original rule.  This tag (allocated out of the cursor memory
**	stream) will allow us to later filter out unused rules, without having
**	to reaccess RDF.  All rules read during the initial invocation are
**	attached to the cursor CB (and the "rules checked" indicator is set).
**	After initial invocation all other cursor statements that access rules
**	will just duplicate the applicable rule statements by filtering out
**	the unused rules (via pst_ruledup).
**
** Restrictions:
**	1. Only SQL - this can be relaxed by not validating the language.
**	   This restriction is only applied if PSY_QUEL_RULES is undefined.
**	2. Only DELETE [CURSOR], INSERT and UPDATE [CURSOR] statements
**	   provide rules.
**
** Inputs:
**	sess_cb				Session control block.  Various fields
**					are used to fill in the RDF message
**					requests and for validating current
**					information.
**	    .pss_crsr			If a cursor then must be set.
**	qmode				Query mode of current query.  Current
**					checks are for:
**					    PSQ_APPEND, PSQ_DELETE, PSQ_UPDATE,
**					    PSQ_REPCURS, PSQ_DELCURS
**	root				Root of current query tree.  This
**					may be traversed.
** Outputs:
**	sess_cb				Session control block:
**	    .pss_row_lvl_usr_rules	user-defined row-level after rules 
**					will be	attached to this list
**	    .pss_row_lvl_sys_rules	system-generated row-level after rules 
**					will be	attached to this list
**	    .pss_stmt_lvl_usr_rules	user-defined statement-level after 
**					rules will be attached to this list
**	    .pss_stmt_lvl_sys_rules	system-generated statement-level after
**					rules will be attached to this list
**	    .pss_row_lvl_usr_before_rules user-defined row-level before rules 
**					will be	attached to this list
**	    .pss_row_lvl_sys_before_rules system-generated row-level before
**					rules will be attached to this list
**	    .pss_stmt_lvl_usr_before_rules user-defined statement-level before
**					rules will be attached to this list
**	    .pss_stmt_lvl_sys_before_rules system-generated statement-level 
**					before rules will be attached to this 
**					list
**	err_blk.err_code		Filled in if an error happened:
**		E_RD0002_UNKNOWN_TBL	  RDF error for table not found.
**		E_PS0903_TAB_NOTFOUND	  Table's gone (concurrent activity)
**	        E_PS0D33_RULE_PARAM	  Param count/list mismatch
**		Errors from RDF & other PSF utilities
**
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	Allocate rule trees and attach to cursor CB.
**
** History:
**	08-mar-89 (neil)
**	    Written for Terminator/rules.
**	18-apr-89 (neil)
**	    Modified to support rules applied to cursors.
**	09-may-89 (neil)
**	    Added saving of rule name for tracing.
**	26-sep-89 (neil)
**	    Allow conditional rule application in QUEL.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	12-jun-90 (andre)
**	    if dbproc is owned by the creator of the rule, we will allow
**	    referring to the dbproc using private alias (i.e. regardless of
**	    whether the dbproc is "grantable").  If, on the other hand, the
**	    dbproc is not owned by the creator of the rule, we will inssit that
**	    it must be grantable by specifying public alias.
**	05-dec-90 (ralph)
**	    copy rule owner name to pst_cprocstmt (b34531)
**	29-sep-92 (andre)
**	    RDF may choose to allocate a new info block and return its address
**	    in rdf_info_blk - we need to copy it over to pss_rdrinfo to avoid
**	    memory leak and other assorted unpleasantries
**	30-jul-92 (rblumer)
**	    changed uses of dbr_column to dbr_columns; 
**          it is now a bitmap instead of an i2.
**	07-nov-92 (andre)
**	    we will no longer be creating private aliases for dbproc QPs
**	    FIX_ME: when recreating a dbproc fired by a rule, we need to verify
**	    that the creator of the rule may grant EXECUTE on the dbproc; this
**	    requires that PSF be told who owns the rule, and then a few assorted
**	    changes in PSF to take care of privilege checking (quite similar to
**	    what happens now if a user is trying to grant EXECUTE on a dbproc)
**	09-nov-92 (rblumer)
**	    Initialize new PST_STATEMENT nodes to all zeros (thereby setting
**	    new pst_statementEndRules field (and other fields) to NULL)
**	21-dec-92 (rblumer)
**	    put statement-level rules on a separate rule list,
**	    and put system-generated rules at the end of each rule list.
**	04-jan-93 (andre)
**	    If processing DELETE/UPDATE WHERE CURRENT OF, sess_cb->pss_rules
**	    will be set to point to the list of applicable rules;
**	    As a result of adding support for rules on views, for
**	    DELETE/INSERT/UPDATE on a view, it may already be pointing at
**	    rules which were defined on that view or on some of its underlying
**	    views;
**	    notice that pss_rules gets reset to NULL before processing any
**	    statement that may take us here (i.e. to psy_rules())
**	10-Feb-93 (Teresa)
**	    Changed RDF_GETINFO to RDF_READTUPLES for new RDF I/F
**	03-mar-93 (andre)
**	    For statement-level rules, we need to obtain a copy of an 
**	    IIPROCEDURE tuple describing the dbproc and set 
**	    pst_callproc->pst_tuple to point at it
**	17-mar-93 (rblumer)
**	    attach the stmt-level rule list to the end of the row-level
**	    rule list, to simplify processing in OPC.
**	07-apr-93 (andre)
**	    Rules for cursor UPDATE/DELETE will no longer be attached to the
**	    cursor CB proper.  Instead, they will be attached to elements of a
**	    list of PSC_TBL_DESCRs based on the table/view on which they are
**	    defined
**
**	    For cursor UPDATE/REPLACE, we will search for PSC_TBL_DESCR with id
**	    matching that in sess_cb->pss_resrng; for cursor DELETE, we will
**	    simply point at the first entry in the list (which must correspond
**	    to the table/view on which the permit was defined and whose
**	    description was read in by psq_dlcsrrow()).  For cursor DELETE, we
**	    will loop through all entries in the list looking for applicable
**	    rules.  For cursor UPDATE/REPLACE, we will search only rules
**	    hanging off the current PSC_TBL_DESCR entry.  For all other queries,
**	    we will ask RDF for rules defined on the table/view whose
**	    description is pointed at by pss_resrng.
**
**	    Another minor change is that during the first pass through
**	    psy_rules() for an updatable cursor, we will allocate statement
**	    nodes using cursor's memory stream which should save us the expense
**	    of calling pst_ruledup() to copy them from ostream into cursor's
**	    stream and then recursively calling psy_rules() to copy them from
**	    cursor stream back into ostream.
**	08-apr-93 (andre)
**	    psy_rules() will no longer be in business of coalescing rule lists.
**	    This is done for two reasons:
**		- rules can now be defined on views, so coaleascing rule lists
**		  before we have attached all of them will result in rule lists
**		  not being built correctly
**		- when processing PREPARE, we must avoid coalescing row- and
**		  statement-level rules because in pst_execute() we will call
**		  pst_ruledup() to duplicate rule lists and coalescing the
**		  original lists will result in a row-level rule list containing
**		  both row-level and statement-level rules which will probably
**		  confuse the hell out of OPF
**	26-jun-93 (andre)
**	    if in the process of scanning rules that apply to a view we come
**	    across one that was created to enforce CASCADED CHECK OPTION, we
**	    will attach it to the query tree ONLY IF another rule enforcing
**	    CASCADED CHECK OPTION on a view for which the current view is a
**	    generally underlying view has not been attached to the query tree.
**	    Once a rule enforcing CASCADED CHECK OPTION has been attached to the
**	    tree, we will set PSS_ATTACHED_CASC_CHECK_OPT_RULE in pss_var_mask
**	    of the range variable representing the view to ensure that rules
**	    enforcing CASCADED CHECK OPTION on this view's underlying views do
**	    not get attached to the query tree
**	23-May-1997 (shero03)
**	    Save the rdf_info_blk after UNFIX
**	02-Jun-1997 (shero03)
**	    Update the pss_rdrinfo after calling rdf
**	17-nov-00 (inkdo01)
**	    Changed pst_ptype to pst_pmask and check for rule update prefetch
**	    CBF indicator for this rule (bug 100680)
**      01-Dec-2000 (hanal04) Bug 100680 INGSRV 1123
**          Changed pst_ptype to pst_pmask and set rule update prefetch
**          for rules fired by updates if PSS_RULE_UPD_PREFETCH is set.
**          This change reworks inkdo01's change 448530.
**	04-Oct-2002 (gygro01) b108628 / ingsrv 1872
**	    Flagging stmt-level-rules with qualification and w/o dbp_param
**	    as row level rules in order to get the qualification applied
**	    correctly.
**	15-june-06 (dougi)
**	    Add support for "before" triggers.
**	30-aug-06 (thaju02)
**	    If delete query and PSS_RULE_DEL_PREFETCH, then set
**	    PST_DEL_PREFETCH bit in the pst_pmask. (B116355)
*/
DB_STATUS
psy_rules(
	PSS_SESBLK      *sess_cb,
	i4		qmode,
	PST_QNODE	*root,
	DB_ERROR        *err_blk)
{
    i4		stmt_type;		/* Mask of current statement  */
    PST_STATEMENT	*stmt_if;		/* IF statement head */
    PST_IFSTMT		*ifs;
    PST_QNODE		*if_qual;
    PST_STATEMENT	*stmt_cp;		/* CALLPROC statement head */
    PST_CPROCSTMT	*cps;
    PST_QNODE		*cp_args;
    PST_QNODE		*pst_tree_node;
    PSS_RNGTAB		*restab;		/* Currently updated table */
    RDF_CB		rdf_rule;		/* RDF for rule */
#define	 RUL_RDF_TUPS	20			/* # of rule tuples in one go */
    RDF_CB		rdf_tree;		/* RDF for tree */
    QEF_DATA		*rdat;			/* Qrymod data from RDF */
    DB_IIRULE		*rtup;			/* Rule tuple returned */
    i4			rtupnum;		/* Number of rule tuples */
    PST_QNODE		*pnode;
    PST_QNODE		*rtree;			/* Saved tree */
    DB_COLUMN_BITMAP	upd_cols;		/* Mask for UPDATE columns */
    DB_COLUMN_BITMAP	tst_cols;		/* Test Mask for UPD columns */
    bool		upd_set = FALSE;	/* Is "upd_cols" mask set? */
    PSC_CURBLK		*csr = NULL;		/* Set if cursor stmt */
    PSC_RULE_TYPE	*csr_rtype;		/* Dynamically alloc'd array */
    i4			csr_rleft = 0;		/* # left of above rule types */
#define	RUL_PSC_TYPES	(256/sizeof(csr_rtype))	/* # of rule types in one go */
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		loc_status;
    i4		err_code;
    PSS_DUPRB		dup_rb;
    PST_STATEMENT	**cur_rule_listp;	/* rule list on which to put
						** the current rule
						*/
    PSC_TBL_DESCR	*tbl_descr = (PSC_TBL_DESCR *) NULL,
			*last_tbl_descr;
    bool		get_next_descr = FALSE;


#ifndef PSY_QUEL_RULES
    if (sess_cb->pss_lang != DB_SQL)		/* Validate only SQL */
	return (E_DB_OK);
#endif
    switch (qmode)				/* Validate query mode */
    {
	case PSQ_APPEND:
	{
	    stmt_type = DBR_S_INSERT;
	    break;
	}
	case PSQ_DELCURS:
	{
	    csr = sess_cb->pss_crsr;

	    /* we will start with the very first descriptor */
	    tbl_descr = (PSC_TBL_DESCR *) csr->psc_tbl_descr_queue.q_next;
	    last_tbl_descr = (PSC_TBL_DESCR *) csr->psc_tbl_descr_queue.q_prev;

	    /* fall through */
	}
	case PSQ_DELETE:
	{
	    stmt_type = DBR_S_DELETE;
	    break;
	}
	case PSQ_REPCURS:
	{
	    DB_TAB_ID	*res_tabid = &sess_cb->pss_resrng->pss_tabid;
	    
	    csr = sess_cb->pss_crsr;

	    /*
	    ** find the table descriptor corresponding to the table/view whose
	    ** description is pointed to be sess_cb->pss_resrng
	    */
	    for (tbl_descr = (PSC_TBL_DESCR *) csr->psc_tbl_descr_queue.q_next;
		 (   tbl_descr->psc_tabid.db_tab_base  !=
			res_tabid->db_tab_base
		  || tbl_descr->psc_tabid.db_tab_index !=
			res_tabid->db_tab_index);
		 tbl_descr = (PSC_TBL_DESCR *) tbl_descr->psc_queue.q_next)
	    ;

	    /* we are only interested in this descriptor */
	    last_tbl_descr = tbl_descr;

	    /* fall through */
	}
	case PSQ_REPLACE:
	{
	    stmt_type = DBR_S_UPDATE;
	    break;
	}
	default:
	{
	    return (E_DB_OK);
	}
    } /* switch on qmode */

    /* initialize RDF_CB which will be used to retrieve rule tuples */
    pst_rdfcb_init(&rdf_rule, sess_cb);
    rdf_rule.rdf_rb.rdr_types_mask = RDR_RULE;
    rdf_rule.rdf_rb.rdr_qtuple_count = RUL_RDF_TUPS;

    /* initialize RDF_CB which will be used to fetch rule trees */
    pst_rdfcb_init(&rdf_tree, sess_cb);
    rdf_tree.rdf_rb.rdr_types_mask = RDR_RULE | RDR_QTREE;

    /* Initialize fields in dup_rb */
    dup_rb.pss_op_mask = 0;
    dup_rb.pss_num_joins = PST_NOJOIN;
    dup_rb.pss_tree_info = (i4 *) NULL;
    dup_rb.pss_mstream = &sess_cb->pss_ostream;
    dup_rb.pss_err_blk = err_blk;

    /*
    ** allocate PST_TREE node that may be used to represent empty parameter list
    ** we are doing it here because rule trees for cursor-related statements
    ** need to be stored in the cursor memory and pst_node() may not be able to
    ** deal with it - on the other hand, having alocated it here, we can always
    ** call pst_trdup() to copy the node into cursor memory
    */
    status = pst_node(sess_cb, &sess_cb->pss_ostream, (PST_QNODE *) NULL,
	(PST_QNODE *) NULL, PST_TREE, (PTR) NULL, 0, DB_NODT, (i2) 0, (i4) 0,
	(DB_ANYTYPE *)NULL, &pst_tree_node, err_blk, (i4) 0);
    if (DB_FAILURE_MACRO(status))
	return(status);

    do
    {
	/*
	** If this is a cursor statement then check if there are any rules.
	** If we've already been through this (earlier cursor DELETE or UPDATE)
	** just copy/filter the rule list.  Otherwise collect ALL rules and
	** attach to cursor.
	*/
	if (csr != NULL)
	{
	    if (get_next_descr)
	    {
		tbl_descr = (PSC_TBL_DESCR *) tbl_descr->psc_queue.q_next;

		status = pst_rgrent(sess_cb, sess_cb->pss_resrng,
		    &tbl_descr->psc_tabid, qmode, err_blk);
		if (DB_FAILURE_MACRO(status))
		{
		    if (err_blk->err_code == E_PS0903_TAB_NOTFOUND)
		    {
			(VOID) psf_error(2120L, 0L, PSF_USERERR,
			    &err_code, err_blk, 0);
		    }

		    return(status);
		}

		get_next_descr = FALSE;
	    }
	    
	    if (~tbl_descr->psc_tbl_mask & DMT_RULE)	/* Rules for table? */
	    {
		/*
		** if there are more descriptors to consider, skip to the
		** next one; otherwise we are done
		*/
		if (tbl_descr == last_tbl_descr)
		{
		    break;
		}
		else
		{
		    get_next_descr = TRUE;
		    continue;
		}
	    }

	    if (tbl_descr->psc_flags & PSC_RULES_CHECKED) /* 2nd time through */
	    {
		if (   tbl_descr->psc_row_lvl_usr_rules
		    || tbl_descr->psc_row_lvl_sys_rules
		    || tbl_descr->psc_stmt_lvl_usr_rules
		    || tbl_descr->psc_stmt_lvl_sys_rules
		    || tbl_descr->psc_row_lvl_usr_before_rules
		    || tbl_descr->psc_row_lvl_sys_before_rules
		    || tbl_descr->psc_stmt_lvl_usr_before_rules
		    || tbl_descr->psc_stmt_lvl_sys_before_rules)
		{
		    i4	    i;
		    
		    /*
		    ** If cursor UPDATE then prepare column bit mask for
		    ** filtering 
		    */
		    if (stmt_type == DBR_S_UPDATE)
		    {
			/*
			** build a map of attributes being updated; first we
			** zero out the map, then run down the RESDOM list
			** setting bits corresponding to resdom numbers
			*/
			DB_COLUMN_BITMAP_INIT(upd_cols.db_domset, i, 0);

			for (pnode = root->pst_left;
			     pnode && pnode->pst_sym.pst_type == PST_RESDOM;
			     pnode = pnode->pst_left)
			{
			    BTset((i4)pnode->pst_sym.pst_value.pst_s_rsdm.
								    pst_rsno,
				  (char *) upd_cols.db_domset);
			}
		    } /* If UPDATE */
		    else 
		    {
			/* set all the bits in the column bitmap for DELETE
			 */
			DB_COLUMN_BITMAP_INIT(upd_cols.db_domset, i,
			    ~((u_i4) 0));
		    }

		    /*
		    ** if any user-defined after row-level rules were defined on
		    ** this table, attach those of them that apply to this query
		    ** to the query tree
		    */
		    if (tbl_descr->psc_row_lvl_usr_rules)
		    {
			status = pst_ruledup(sess_cb, &sess_cb->pss_ostream,
			    stmt_type, (char *) upd_cols.db_domset, 
			    tbl_descr->psc_row_lvl_usr_rules,
			    &sess_cb->pss_row_lvl_usr_rules, err_blk);
			if (status != E_DB_OK)
			    return(status);
		    }

		    /*
		    ** if user-defined before row-level rules were defined on
		    ** this table, attach those of them that apply to this query
		    ** to the query tree
		    */
		    if (tbl_descr->psc_row_lvl_usr_before_rules)
		    {
			status = pst_ruledup(sess_cb, &sess_cb->pss_ostream,
			    stmt_type, (char *) upd_cols.db_domset, 
			    tbl_descr->psc_row_lvl_usr_before_rules,
			    &sess_cb->pss_row_lvl_usr_before_rules, err_blk);
			if (status != E_DB_OK)
			    return(status);
		    }

		    /*
		    ** if system-generated after row-level rules were defined on
		    ** this table, attach those of them that apply to this query
		    ** to the query tree
		    */
		    if (tbl_descr->psc_row_lvl_sys_rules)
		    {
			status = pst_ruledup(sess_cb, &sess_cb->pss_ostream,
			    stmt_type, (char *) upd_cols.db_domset, 
			    tbl_descr->psc_row_lvl_sys_rules,
			    &sess_cb->pss_row_lvl_sys_rules, err_blk);
			if (status != E_DB_OK)
			    return(status);
		    }

		    /*
		    ** if sys-generated before row-level rules were defined on
		    ** this table, attach those of them that apply to this query
		    ** to the query tree
		    */
		    if (tbl_descr->psc_row_lvl_sys_before_rules)
		    {
			status = pst_ruledup(sess_cb, &sess_cb->pss_ostream,
			    stmt_type, (char *) upd_cols.db_domset, 
			    tbl_descr->psc_row_lvl_sys_before_rules,
			    &sess_cb->pss_row_lvl_sys_before_rules, err_blk);
			if (status != E_DB_OK)
			    return(status);
		    }

		    /*
		    ** if user-defined after statement-level rules were defined
		    ** on this table, attach those of them that apply to this
		    ** query to the query tree
		    */
		    if (tbl_descr->psc_stmt_lvl_usr_rules)
		    {
			status = pst_ruledup(sess_cb, &sess_cb->pss_ostream,
			    stmt_type, (char *) upd_cols.db_domset, 
			    tbl_descr->psc_stmt_lvl_usr_rules,
			    &sess_cb->pss_stmt_lvl_usr_rules, err_blk);
			if (status != E_DB_OK)
			    return(status);
		    }

		    /*
		    ** if user-defined before statement-level rules were defined
		    ** on this table, attach those of them that apply to this
		    ** query to the query tree
		    */
		    if (tbl_descr->psc_stmt_lvl_usr_before_rules)
		    {
			status = pst_ruledup(sess_cb, &sess_cb->pss_ostream,
			    stmt_type, (char *) upd_cols.db_domset, 
			    tbl_descr->psc_stmt_lvl_usr_before_rules,
			    &sess_cb->pss_stmt_lvl_usr_before_rules, err_blk);
			if (status != E_DB_OK)
			    return(status);
		    }

		    /*
		    ** if system-generated after statement-level rules were
		    ** defined on this table, attach those of them that apply to
		    ** this query to the query tree
		    */
		    if (tbl_descr->psc_stmt_lvl_sys_rules)
		    {
			status = pst_ruledup(sess_cb, &sess_cb->pss_ostream,
			    stmt_type, (char *) upd_cols.db_domset, 
			    tbl_descr->psc_stmt_lvl_sys_rules,
			    &sess_cb->pss_stmt_lvl_sys_rules, err_blk);
			if (status != E_DB_OK)
			    return(status);
		    }

		    /*
		    ** if system-generated before statement-level rules were
		    ** defined on this table, attach those of them that apply to
		    ** this query to the query tree
		    */
		    if (tbl_descr->psc_stmt_lvl_sys_before_rules)
		    {
			status = pst_ruledup(sess_cb, &sess_cb->pss_ostream,
			    stmt_type, (char *) upd_cols.db_domset, 
			    tbl_descr->psc_stmt_lvl_sys_before_rules,
			    &sess_cb->pss_stmt_lvl_sys_before_rules, err_blk);
			if (status != E_DB_OK)
			    return(status);
		    }
		}
		
		/*
		** applicable rules (if any) have been attched to the rule lists
		** maintained in sess_cb - we're done with this descriptor;
		** if there are more descriptors to consider, skip to the
		** next one; otherwise we are done
		*/
		if (tbl_descr == last_tbl_descr)
		{
		    break;
		}
		else
		{
		    get_next_descr = TRUE;
		    continue;
		}
	    }

	    /*
	    ** We're in a cursor statement but this is the first time through.
	    ** Next we will collect all rules that may apply to the cursor
	    */
	}

	restab = sess_cb->pss_resrng;		/* Does table have rules */
	if (~restab->pss_tabdesc->tbl_status_mask & DMT_RULE)
	{
	    /*
	    ** if processing cursor-related statement, update psc_tbl_mask
	    ** inside the table descriptor (it could mean that between the time
	    ** the cursor was declared and now all rules were dropped OR that
	    ** the table description used during DECLARE CURSOR processing was
	    ** stale)
	    */
	    if (csr)
	    {
		tbl_descr->psc_tbl_mask = restab->pss_tabdesc->tbl_status_mask;
		continue;
	    }
	    else
	    {
		break;
	    }
	}

	/*
	** Get all rules that can be applied to this table from RDF.
	** First set up constant parts of RDF requests.
	*/

	/* RDF rule request */
	STRUCT_ASSIGN_MACRO(restab->pss_tabid, rdf_rule.rdf_rb.rdr_tabid);
	rdf_rule.rdf_rb.rdr_update_op = RDR_OPEN;
	rdf_rule.rdf_info_blk = restab->pss_rdrinfo;

	/* RDF rule tree request */
	STRUCT_ASSIGN_MACRO(restab->pss_tabid, rdf_tree.rdf_rb.rdr_tabid);
	rdf_tree.rdf_rb.rdr_qtuple_count = 1;  	/* Get 1 rule tree at a time */
	rdf_tree.rdf_info_blk = restab->pss_rdrinfo;

	/* 
	** For each group of RUL_RDF_TUPS rule tuples, get the rule tree (if
	** there is one) and construct the rule to attach.
	*/
	while (rdf_rule.rdf_error.err_code == 0)
	{
	    status = rdf_call(RDF_READTUPLES, (PTR) &rdf_rule);
	    
	    /*
	    ** RDF may choose to allocate a new info block and return its
	    ** address in rdf_info_blk - we need to copy it over to pss_rdrinfo
	    ** to avoid memory leak and other assorted unpleasantries
	    */
	    if (rdf_rule.rdf_info_blk != restab->pss_rdrinfo)
	    {
		restab->pss_rdrinfo = rdf_tree.rdf_info_blk =
		    rdf_rule.rdf_info_blk;
	    }

	    rdf_rule.rdf_rb.rdr_update_op = RDR_GETNEXT;    /* For next rule */
	    if (status != E_DB_OK)
	    {
		switch (rdf_rule.rdf_error.err_code)
		{
		  case E_RD0002_UNKNOWN_TBL:
		    psf_error(2117L, 0L, PSF_USERERR,
			      &err_code, err_blk, 1,
			      psf_trmwhite(sizeof(DB_TAB_NAME),
					   (char *) &restab->pss_tabname),
			      &restab->pss_tabname);
		    status = E_DB_ERROR;
		    goto cleanup;
		  case E_RD0011_NO_MORE_ROWS:
		    status = E_DB_OK;	/* But complete the tree part */
		    break;
		  case E_RD0013_NO_TUPLE_FOUND:
		    status = E_DB_OK;	/* None here - will break out of loop */
		    continue;
		  default:
		    _VOID_ psf_rdf_error(RDF_READTUPLES, &rdf_rule.rdf_error,
			err_blk);
		    status = E_DB_ERROR;
		    goto cleanup;
		} /* switch on RDF errors */
	    } /* If error getting rule */

	    /*
	    ** We now have one or more rule tuples.  For each rule get the
	    ** tree (if any) and put them together.
	    */
	    for 
	    (
		rdat = rdf_rule.rdf_info_blk->rdr_rtuples->qrym_data,
		    rtupnum = 0;
		rtupnum < rdf_rule.rdf_info_blk->rdr_rtuples->qrym_cnt;
		rdat = rdat->dt_next, rtupnum++
	    )
	    {
		rtup = (DB_IIRULE *)rdat->dt_data;

		/*
		** Can we ignore this rule?  Either a cursor and doesn't apply
		** to UPDATE/DELETE or not a cursor and not for this statement
		** type.
		** 
		** If the rule was created to enforce CASCADED CHECK OPTION and
		** another rule enforcing CASCADED CHECK OPTION on a view for
		** which the view represented by *sess_cb->pss_resrng is a
		** generally underlying view has already been attached to the
		** query tree, we will not attach this rule either.
		*/
		if (   (   (csr != NULL)
			&& !(rtup->dbr_statement & (DBR_S_UPDATE|DBR_S_DELETE))
		       )
		    || (csr == NULL && (rtup->dbr_statement & stmt_type) == 0)
		    || (rtup->dbr_flags & DBR_F_CASCADED_CHECK_OPTION_RULE &&
			restab->pss_var_mask & PSS_ATTACHED_CASC_CHECK_OPT_RULE
		       )
		   )
		{
		    continue;
		}

		/*
		** If this isn't a cursor (where we collect all potential rules)
		** and if the rule action specified 'UPDATE(columnlist)',
		** then check if any of those columns are referenced in this
		** UPDATE
		** (NOTE: Bit 0 of the bitmap is only set if no column list
		**        was specified when the rule was created)
		*/
		if (   csr == NULL
		    && stmt_type == DBR_S_UPDATE
		    && (BTtest(0, (char *)rtup->dbr_columns.db_domset) == 0))
		{
		    /* 
		    ** If UPDATE bit mask is not yet initialized then clear it
		    ** and fix it.  This allows a single traversal of the target
		    ** list for all the rules.
		    */
		    if (!upd_set)
		    {
			MEfill(sizeof(upd_cols),		/* Clear it */
			       0, (PTR) &upd_cols);

			for (pnode = root->pst_left;		/* Fill it */
			     pnode != NULL &&
				pnode->pst_sym.pst_type == PST_RESDOM;
			     pnode = pnode->pst_left)
			{
			    BTset((i4)
				  pnode->pst_sym.pst_value.pst_s_rsdm.pst_rsno,
				  (char *)upd_cols.db_domset);
			}
			upd_set = TRUE;
		    } /* If UPDATE bit mask not yet set */

		    /* copy the update bit mask to an overwriteable test mask,
		    ** and test that against the column bitmap of the rule
		    */
		    STRUCT_ASSIGN_MACRO(upd_cols, tst_cols);
		    BTand((i4) DB_COL_BITS, 
			  (char *) rtup->dbr_columns.db_domset, 
			  (char *) tst_cols.db_domset);

		    /* If none of the rule's columns are in current target list,
		    ** skip this rule
		    */
		    if (BTcount((char *)tst_cols.db_domset, DB_COL_BITS) == 0)
			continue;

		} /* If column-check */

		/*
		** If there's no tree then confirm that there are no parameters
		** and build a dummy parameter list (PST_TREE).  Otherwise
		** extract the tree from RDF and point at the parameter list
		** and/or the IF condition.
		*/
		if (   rtup->dbr_treeid.db_tre_high_time == 0
		    && rtup->dbr_treeid.db_tre_low_time == 0)
		{
		    if_qual = NULL;

		    /*
		    ** cp_args must be set to point at a PST_TREE node; if
		    ** procesing cursor-related statement, that node must be
		    ** allocated using cursor memory; otherwise it will be
		    ** allocated using pss_ostream
		    */
		    if (csr)
		    {
			status = pst_trdup(csr->psc_stream, pst_tree_node,
			    &cp_args, &sess_cb->pss_memleft, err_blk);
		    }
		    else
		    {
			dup_rb.pss_tree = pst_tree_node;
			dup_rb.pss_dup = &cp_args;
			status = pst_treedup(sess_cb, &dup_rb);
		    }
		    if (status != E_DB_OK)
			goto cleanup;
		}
		else
		{
		    /* Extract the tree from RDF */
		    STRUCT_ASSIGN_MACRO(rtup->dbr_treeid,
					rdf_tree.rdf_rb.rdr_tree_id);
		    rdf_tree.rdf_rb.rdr_rec_access_id = NULL;
		    MEcopy((PTR)&rtup->dbr_name, sizeof(DB_NAME),
			    (PTR)&rdf_tree.rdf_rb.rdr_name.rdr_rlname);
		    MEcopy((PTR)&rtup->dbr_owner, sizeof(DB_OWN_NAME),
			    (PTR)&rdf_tree.rdf_rb.rdr_owner);
		    status = rdf_call(RDF_GETINFO, (PTR) &rdf_tree);

		    /*
		    ** RDF may choose to allocate a new info block and return
		    ** its address in rdf_info_blk - we need to copy it over to
		    ** pss_rdrinfo to avoid memory leak and other assorted
		    ** unpleasantries
		    */
		    if (rdf_tree.rdf_info_blk != restab->pss_rdrinfo)
		    {
			restab->pss_rdrinfo =
			    rdf_rule.rdf_info_blk = rdf_tree.rdf_info_blk;
		    }

		    if (status != E_DB_OK)
		    {
			if (rdf_tree.rdf_error.err_code == E_RD0002_UNKNOWN_TBL)
			{
			    psf_error(E_PS0903_TAB_NOTFOUND, 0L, PSF_USERERR,
				      &err_code, err_blk, 1,
				      psf_trmwhite(sizeof(DB_TAB_NAME),
					(char *) &restab->pss_tabname),
				      &restab->pss_tabname);
			}
			else
			{
			    _VOID_ psf_rdf_error(RDF_GETINFO,
				&rdf_tree.rdf_error, err_blk);
			}
			goto cleanup;
		    }

		    rtree =
		       ((PST_PROCEDURE *)
			 rdf_tree.rdf_rb.rdr_rule->qry_root_node)->
			    pst_stmts->pst_specific.pst_tree->pst_qtree;

		    /*
		    ** Copy tree (from rtree to rtree) as it's in RDF memory
		    ** if processing a cursor-related statement, copy into
		    ** cursor's memory; otherwise copy into pss_ostream
		    */
		    if (csr)
		    {
			status = pst_trdup(csr->psc_stream, rtree, &rtree,
			    &sess_cb->pss_memleft, err_blk);
		    }
		    else
		    {
			dup_rb.pss_tree = rtree;
			dup_rb.pss_dup  = &rtree;
			status = pst_treedup(sess_cb, &dup_rb);
		    }

		    /* Unfix the tree no matter what the above status is */
		    loc_status = rdf_call(RDF_UNFIX, (PTR) &rdf_tree);
		    restab->pss_rdrinfo = 
			rdf_rule.rdf_info_blk = rdf_tree.rdf_info_blk;
		    if (loc_status != E_DB_OK)
		    {
			_VOID_ psf_rdf_error(RDF_UNFIX, &rdf_tree.rdf_error,
					     err_blk);
			status = loc_status;
		    }
		    if (status != E_DB_OK)
			goto cleanup;		/* Close rule file */
		    
		    /* Verify that we have a good parameter list */
		    if (   (   (rtup->dbr_dbpparam != 0)
			    && (   (rtree->pst_left == NULL)
				|| (rtree->pst_left->pst_sym.pst_type ==
				        PST_TREE)
			       )
			   )
			||
			   (   (rtup->dbr_dbpparam == 0)
			    && (rtree->pst_left != NULL)
			    && (rtree->pst_left->pst_sym.pst_type != PST_TREE)
			   )
		       )
		    {
			psf_error(E_PS0D33_RULE_PARAM, 0L, PSF_INTERR,
				  &err_code, err_blk, 0);
			status = E_DB_ERROR;
			goto cleanup;
		    }
		    

		    /* Get parameter list (if there's one) */
		    if ((cp_args = rtree->pst_left) == NULL)
		    {
			/*
			** cp_args must be set to point at a PST_TREE node; if
			** procesing cursor-related statement, that node must be
			** allocated using cursor memory; otherwise it will be
			** allocated using pss_ostream
			*/
			if (csr)
			{
			    status = pst_trdup(csr->psc_stream, pst_tree_node,
				&cp_args, &sess_cb->pss_memleft, err_blk);
			}
			else
			{
			    dup_rb.pss_tree = pst_tree_node;
			    dup_rb.pss_dup = &cp_args;
			    status = pst_treedup(sess_cb, &dup_rb);
			}
			if (status != E_DB_OK)
			    goto cleanup;
		    }

		    /* Get IF condition */
		    if (   rtree->pst_right == NULL
			|| rtree->pst_right->pst_sym.pst_type == PST_QLEND)
			if_qual = NULL;
		    else
			if_qual = rtree->pst_right;
		} /* If there is a tree */

		/*
		** At this point cp_args is pointing at parameter list (or
		** PST_TREE) and if_qual is either NULL or a condition tree.
		*/

		/*
		** Allocate CALLPROC statement node and initialize fields
		** if processing cursor-related statement, use cursor memory;
		** otherwise use pss_ostream
		*/
		if (csr)
		{
		    status = psf_umalloc(sess_cb, csr->psc_stream,
			sizeof(PST_STATEMENT), (PTR *) &stmt_cp, err_blk);
		}
		else
		{
		    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
			sizeof(PST_STATEMENT), (PTR *) &stmt_cp, err_blk);
		}
		if (status != E_DB_OK)
		    goto cleanup;
		MEfill(sizeof(PST_STATEMENT), (u_char) 0,
		    (PTR) stmt_cp);

		stmt_cp->pst_mode = qmode;
		stmt_cp->pst_type = PST_CP_TYPE;
		cps = &stmt_cp->pst_specific.pst_callproc;
		cps->pst_pmask = PST_CPRULE;
		/* Fill in rule name for tracing */
		STRUCT_ASSIGN_MACRO(rtup->dbr_name, cps->pst_rulename);
		STRUCT_ASSIGN_MACRO(rtup->dbr_owner, cps->pst_ruleowner);
		cps->pst_procname.qso_n_id.db_cursor_id[0] =
		    cps->pst_procname.qso_n_id.db_cursor_id[1] = 0;
		
		MEcopy((PTR)&rtup->dbr_dbpname,
		    sizeof(rtup->dbr_dbpname),
		    (PTR) cps->pst_procname.qso_n_id.db_cur_name);
		STRUCT_ASSIGN_MACRO(rtup->dbr_dbpowner,
				    cps->pst_procname.qso_n_own);
		cps->pst_procname.qso_n_dbid = sess_cb->pss_udbid;
		cps->pst_argcnt = rtup->dbr_dbpparam;
		cps->pst_arglist = cp_args;
		if (rtup->dbr_flags & DBR_F_SYSTEM_GENERATED)
		    cps->pst_pmask |= PST_CPSYS;

		/* Pick correct list to put this rule on.
		** 
		** if processing a cursor-related statement, attach it to the
		** list asociated with the current table descriptor - otherwise
		** attach it to the list mainatined in the session CB
		** 
		** The two types of rule lists are Row-level and
		** Statement-level;
		** in addition, we want system-generated rules to fire AFTER
		** user rules, so we put them on separate lists; pst_header()
		** will assemble these lists in a manner expected by OPF
		*/

		/* b108628 - For Statement level rules with qualification
		** and no dbp parameters, we flag them to execute as row
		** level rules otherwise the qualification won't be applied.
		*/

		if (rtup->dbr_type == DBR_T_STMT &&
	 	    rtup->dbr_dbpparam == 0 &&
		    if_qual)
		    rtup->dbr_type = DBR_T_ROW;

		if (rtup->dbr_type == DBR_T_ROW)
		{
		    /*
		    ** attach to row-level rule list;
		    */
		    if (rtup->dbr_flags & DBR_F_SYSTEM_GENERATED)
		    {
			/* attach to system-generated rule list
			** (currently these are rules used for CHECK
			** constraints)
			*/
			if (rtup->dbr_flags & DBR_F_BEFORE)
			    cur_rule_listp =
				(csr) ? &tbl_descr->psc_row_lvl_sys_before_rules
				  : &sess_cb->pss_row_lvl_sys_before_rules;
							/* before trigger */
			    else cur_rule_listp =
				(csr) ? &tbl_descr->psc_row_lvl_sys_rules
				  : &sess_cb->pss_row_lvl_sys_rules;
							/* after trigger */
		    }
		    else
		    {
			/* attach to user rule list
			 */
			if (rtup->dbr_flags & DBR_F_BEFORE)
			    cur_rule_listp =
				(csr) ? &tbl_descr->psc_row_lvl_usr_before_rules
				  : &sess_cb->pss_row_lvl_usr_before_rules;
							/* before trigger */
			    else cur_rule_listp =
				(csr) ? &tbl_descr->psc_row_lvl_usr_rules
				  : &sess_cb->pss_row_lvl_usr_rules;
							/* after trigger */
		    }

                    /* Also, if this is an update request. If so, must
                    ** set flag to indicate need to compile prefetch
                    ** strategy for update (to prevent bug 100680).
                    */
                    if ((sess_cb->pss_ses_flag & PSS_RULE_UPD_PREFETCH) &&
                        (qmode == PSQ_REPLACE))
                    {
                        cps->pst_pmask |= PST_UPD_PREFETCH;
                    }

		    if ((qmode == PSQ_DELETE) &&
			(sess_cb->pss_ses_flag & PSS_RULE_DEL_PREFETCH))
			cps->pst_pmask |= PST_DEL_PREFETCH;

		}
		else /* statement-level rule */
		{
		    PSS_DBPINFO		*dbpinfo_p;
		    PSQ_CB		psqcb;
		    i4			ret_flags;

		    /* attach to statement-level rule lists
		     */
		    if (rtup->dbr_flags & DBR_F_SYSTEM_GENERATED)
		    {
			/* attach to system-generated rule list
			** (these are rules used for REFERENTIAL constraints)
			*/
			if (rtup->dbr_flags & DBR_F_BEFORE)
			    cur_rule_listp =
				(csr) ? &tbl_descr->psc_stmt_lvl_sys_before_rules
				  : &sess_cb->pss_stmt_lvl_sys_before_rules;
			    else cur_rule_listp =
				(csr) ? &tbl_descr->psc_stmt_lvl_sys_rules
				  : &sess_cb->pss_stmt_lvl_sys_rules;
		    }
		    else
		    {
			/* attach to user rule list
			 */
			if (rtup->dbr_flags & DBR_F_BEFORE)
			    cur_rule_listp =
				(csr) ? &tbl_descr->psc_stmt_lvl_usr_before_rules
				  : &sess_cb->pss_stmt_lvl_usr_before_rules;
			    else cur_rule_listp =
				(csr) ? &tbl_descr->psc_stmt_lvl_usr_rules
				  : &sess_cb->pss_stmt_lvl_usr_rules;
		    }

		    /* 
		    ** for statement-level rules, we need to obtain an
		    ** IIPROCEDURE tuple for this dbproc and have
		    ** cps->pst_ptuple point at it
		    */
		    MEfill(sizeof(psqcb), (u_char) 0, (PTR) &psqcb);
		    psqcb.psq_mode = qmode;
		    psqcb.psq_error.err_code = E_PS0000_OK;

		    status = pst_dbpshow(sess_cb, &rtup->dbr_dbpname,
			&dbpinfo_p, &rtup->dbr_dbpowner, (DB_TAB_ID *) NULL,
			PSS_DBP_BY_OWNER, &psqcb, &ret_flags);
		    if (   DB_FAILURE_MACRO(status)
			|| ret_flags == PSS_MISSING_DBPROC)
		    {
			STRUCT_ASSIGN_MACRO(psqcb.psq_error, (*err_blk));
			goto cleanup;
		    }

		    /* 
		    ** OPF counts on pst_ptuple pointing at an IIPROCEDURE tuple 
		    ** describing the dbproc fired by this rule - let's not 
		    ** disapoint it; if processing a cursor-related statement,
		    ** we need to copy IIPROCEDURE tuple into cursor memory,
		    ** otherwise, we can use dbpinfo_p->pss_ptuple (*dbpinfo_p
		    ** was allocated using pss_ostream)
		    */
		    if (csr)
		    {
			status = psf_umalloc(sess_cb, csr->psc_stream,
			    sizeof(DB_PROCEDURE), (PTR *) &cps->pst_ptuple,
			    err_blk);
			if (DB_FAILURE_MACRO(status))
			    goto cleanup;

			STRUCT_ASSIGN_MACRO(dbpinfo_p->pss_ptuple,
					    (*cps->pst_ptuple));
		    }
		    else
		    {
			cps->pst_ptuple = &dbpinfo_p->pss_ptuple;
		    }
		}

		/*
		** Attach to appropriate list as next statement - reverse order.  
		** IF statement may be attached above this next.
		*/
		stmt_cp->pst_next = *cur_rule_listp;
		stmt_cp->pst_link = *cur_rule_listp;

		/* If there was an IF statement then construct that too */
		if (if_qual)
		{
		    /*
		    ** Allocate IF statement node and initialize fields
		    ** If processing a cursor-related statement, PST_STATEMENT
		    ** will be allocated using cursor memory; otherwise
		    ** pss_ostream will be used
		    */
		    if (csr)
		    {
			status = psf_umalloc(sess_cb, csr->psc_stream,
			    sizeof(PST_STATEMENT), (PTR *) &stmt_if, err_blk);
		    }
		    else
		    {
			status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
			    sizeof(PST_STATEMENT), (PTR *) &stmt_if, err_blk);
		    }
		    if (status != E_DB_OK)
			goto cleanup;
			
		    MEfill(sizeof(PST_STATEMENT),
			(u_char) 0, (PTR) stmt_if);

		    stmt_if->pst_mode = qmode;
		    stmt_if->pst_type = PST_IF_TYPE;
		    ifs = &stmt_if->pst_specific.pst_if;
		    ifs->pst_condition = if_qual;

		    /* Attach above the CALLPROC statement */
		    stmt_if->pst_link = stmt_cp;
		    stmt_if->pst_next = *cur_rule_listp;
		    ifs->pst_true = stmt_cp;
		    ifs->pst_false = *cur_rule_listp;
		    *cur_rule_listp = stmt_if;
		    
		}  /* end if (if_qual) */
		else
		{
		    *cur_rule_listp = stmt_cp;
		} /* If there's an IF statement */

		/*
		** If processing a cursor statement (this must be first time)
		** allocate a rule/cursor type descriptor and assign it to
		** pst_opf. If none left, refresh the list from the cursor
		** memory stream.
		*/
		if (csr != NULL)
		{
		    if (csr_rleft <= 0)			/* Refresh */
		    {
			csr_rleft = RUL_PSC_TYPES;
			status = psf_umalloc(sess_cb, csr->psc_stream,
					     sizeof(*csr_rtype) * csr_rleft, 
					     (PTR *)&csr_rtype, err_blk);
			if (status != E_DB_OK)
			    goto cleanup;
		    }
		    csr_rtype->psr_statement = rtup->dbr_statement;
		    
		    /* copy column bitmap 
		    ** (for DELETE and INSERT, will be all 1's;
		    **  for UPDATEs, can either be all 1's 
		    **  or have from 1 to DB_MAX_COLS bits set)
		    */
		    MEcopy((PTR)rtup->dbr_columns.db_domset, 
			   DB_COL_WORDS*sizeof(i4),
			   (PTR) csr_rtype->psr_columns.db_domset);

		    (*cur_rule_listp)->pst_opf = (PTR)csr_rtype;

		    csr_rtype++;
		    csr_rleft--;
		}
		else
		{
		    (*cur_rule_listp)->pst_opf = NULL;
		}/* If we needed a cursor/rule type */

		/*
		** if this rule was created to enforce CASCADED CHECK OPTION,
		** remember that we attached it to the query tree (this will
		** tell us to avoid attaching rules enforcing CASCADED CHECK
		** OPTION on the view's underlying views)
		*/
		if (rtup->dbr_flags & DBR_F_CASCADED_CHECK_OPTION_RULE)
		{
		    restab->pss_var_mask |= PSS_ATTACHED_CASC_CHECK_OPT_RULE;
		}
		
	    } /* For each rule found in RDF set */

	} /* While getting rule tuple from RDF */

cleanup:
	if (rdf_rule.rdf_rb.rdr_rec_access_id != NULL)
	{
	    /* Unfix rule tuples */
	    rdf_rule.rdf_rb.rdr_update_op = RDR_CLOSE;
	    loc_status = rdf_call(RDF_READTUPLES, (PTR) &rdf_rule);
	    restab->pss_rdrinfo = rdf_tree.rdf_info_blk =
	        rdf_rule.rdf_info_blk;
	    if (loc_status != E_DB_OK)
	    {
		_VOID_ psf_rdf_error(RDF_READTUPLES, &rdf_rule.rdf_error,
		    err_blk);
		if (loc_status > status)
		    status = loc_status;
	    }
	}

	if (DB_FAILURE_MACRO(status))
	    break;

	/*
	** if the statement being processed is cursor-related, remember that we
	** have checked for rules on the current table or view; otherwise, we
	** are done
	*/
	if (csr)
	    tbl_descr->psc_flags |= PSC_RULES_CHECKED;
	else
	    break;
	
    } while (1);

    return (status);
} /* psy_rules */
