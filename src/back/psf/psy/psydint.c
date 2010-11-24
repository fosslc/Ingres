/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <bt.h>
#include    <me.h>
#include    <st.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <adf.h>
#include    <adfops.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <ulm.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <opfcb.h>
#include    <qefmain.h>
#include    <qefqeu.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>
#include    <psydint.h>
#include    <cui.h>

/**
**
**  Name: PSYDINT.C - Functions used to define an integrity.
**
**  Description:
**      This file contains the functions necessary to define an integrity.
**
**          psy_dinteg - Define an integrity.
**
**
**  History:
**      02-oct-85 (jeff)    
**          written
**	24-mar-88 (stec)
**	    Implement RETRY.
**	25-mar-88 (stec)
**	    Delete integrity tuple on error; ABORT will not do it
**	    because of an internal savepoint established by the internal
**	    query run to verify that the integrity holds initially.
**	13-oct-88 (stec)
**	    Handle following QEF return codes:
**		- E_QE0024_TRANSACTION_ABORTED
**		- E_QE002A_DEADLOCK
**		- E_QE0034_LOCK_QUOTA_EXCEEDED
**		- E_QE0099_DB_INCONSISTENT
**	16-dec-88 (stec)
**	    Correct octal constant problem.
**	06-feb-89 (ralph)
**	    Modified to use DB_COL_WORDS when resetting dbi_domset bits
**	06-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    initialized new db_integrity field, dbi_seq.
**	17-apr-89 (jrb)
**	    Interface change for pst_node and prec cleanup for decimal project.
**	08-may-89 (ralph)
**	    Initialize reserved field to blanks (was \0).
**	06-jun-89 (ralph)
**	    Fix unix portability problems
**	14-sep-90 (teresa)
**	    psq_force changed from boolean to bitflag
**	09-nov-90 (stec)
**	    Changed psy_dinteg() to initialize opf_thandle in OPF_CB.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	15-oct-92 (rblumer)
**	    Initalize new fields in DB_INTEGRITY tuple by setting 
**          entire tuple to zeros.  Change dbi_domset to dbi_columns.
**	06-nov-1992 (rog)
**	    <ulf.h> must be included before <qsf.h>.
**	08-apr-93 (ralph)
**	    DELIM_IDENT:  Use appropriate case for "tid"
**      23-mar-1993 (smc)
**          Cast parameters to match prototypes. Fixed up makeidset() to
**          use u_i4 to match the type it is passed.
**	11-jun-93 (rblumer)
**	    fix spurious RDF_INVALIDATE error in psy_dinteg by checking correct
**	    variable for failure (i.e. stat instead of status).
**	    Other minor changes to remove compiler warnings.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	7-jul-93 (rickh)
**	    Prototyped qef_call invocations.
**	10-aug-93 (andre)
**	    fixed causes of compiler warnings
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	30-sep-93 (stephenb)
**	    pass tablename to RDF in psy_dinteg().
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for qsf_owner which has changed type to PTR.
**	10-Jan-2001 (jenjo02)
**	    Supply session's SID to QSF in qsf_sid.
**      01-Nov-2002 (stial01)
**          Don't validate integrities from upgradedb (SIR 106745)
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**	21-Oct-2010 (kiria01) b124629
**	    Use the macro symbol with ult_check_macro instead of literal.
[@history_template@]...
**/

/*
**  Definition of static variables and forward static functions.
*/
static VOID
makeidset(
	register i4        varno,
	register PST_QNODE *tree,
	u_i4		   dset[]);

/*{
** Name: psy_dinteg	- Define an integrity.
**
**  INTERNAL PSF call format: status = psy_dinteg(&psy_cb, &sess_cb, &qeu_cb);
**
**  EXTERNAL call format:     status = psy_call(PSY_DINTEG, &psy_cb, &sess_cb);
**
** Description:
**      The psy_dinteg function stores the definition of an integrity on a 
**      table.  It stores the text of the defining query in the iiqrytext
**      system relation, and the query tree representing the integrity in the 
**      tree system relation.  It inverts the query tree (forms the logical 
**      NOT of the tree), forms an any aggregate tree with this qualification,
**	and optimizes and runs the aggregate.  If the answer is not zero, the
**	integrity doesn't hold initially, and modifications must be rolled
**	back (and the user must receive an error). This is actually done by
**	calling RDF to delete integrity with the integrity seq. no. returned
**	by RDF from the APPEND call. Good status is returned (and a message)
**	because the tx is to be committed. This strange error recovery technique
**	is used because execution of internal query (to verify if integrity
**	holds) created a savepoint, and any abort that follows rolls back to
**	the savepoint rather than that state when begin tx was issued, as one
**	might expect. This would result in the work done by RDF committed
**	and not cleaned up.
** 
**      It is assumed that the input query tree is a qualification on a single 
**      table, and that the query text really is a "define integrity" command
**	(or its equivalent in SQL).
**
** Inputs:
**      psy_cb
**          .psy_qrytext                Id of query text as stored in QSM
**          .psy_tables[0]              Id of table to receive integrity
**          .psy_intree                 Query tree representing predicate of
**					integrity.
**	    .psy_tabname[0]		Name of table receiving integrity
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**	qeu_cb				Initialized QEU_CB
**
** Outputs:
**      psy_cb
**	    .psy_error			Filled in if an error happens
**		.err_code		    What the error was
**		    E_PS0000_OK			Success
**		    E_PS0001_USER_ERROR		User made a mistake
**		    E_PS0002_INTERNAL_ERROR	Internal inconsistency in PSF
**		    E_PS0B04_MUST_REPARSE	Must reparse the query
**		    E_PS0008_RETRY		Let SCF know that query should
**						reparsed and reexecuted.
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s);
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Stores query tree in tree relation, query text in iiqrytext
**	    iiqrytext relation, row in integrities relation identifying
**	    the integrity.  Does a DMF alter table to indicate that the
**	    table has integrities on it.
**
** History:
**	02-oct-85 (jeff)
**          written
**      03-sep-86 (seputis)
**          assigned storage for integrity tuple
**          changed some psy_cb. to psy_cb->
**          caste parameters to pst_node so lint would not complain
**	22-apr-87 (stec)
**	    Removed call to adi_opid(), use ADI_ANY_OP to init opid
**	    instead, this is necessary because SQL doesn't have ANY defined.
**	24-apr-87 (stec)
**	    Init agghead correctly for SQL.
**	14-oct-87 (stec)
**	    Moved TX handling to psy_call.
**	27-oct-87 (stec)
**	    Call qef_call twice to close the table.
**	21-jan-88 (stec)
**	    Added proper initialization of PST_AOP node.
**	24-mar-88 (stec)
**	    Implement RETRY.
**	25-mar-88 (stec)
**	    Delete integrity tuple on error; ABORT will not do it
**	    because of an internal savepoint established by the internal
**	    query run to verify that the integrity holds initially.
**	10-may-88 (stec)
**	    Modified for db procs.
**	03-oct-88 (andre)
**	    modified call to pst_rgent() to pass 0 as a query mode since it is
**	    clearly not PSQ_DESTROY
**	13-oct-88 (stec)
**	    Handle following QEF return codes:
**		- E_QE0024_TRANSACTION_ABORTED
**		- E_QE002A_DEADLOCK
**		- E_QE0034_LOCK_QUOTA_EXCEEDED
**		- E_QE0099_DB_INCONSISTENT
**	16-dec-88 (stec)
**	    Correct octal constant problem (0038 -> 38).
**	06-feb-89 (ralph)
**	    Modified to use DB_COL_WORDS when resetting dbi_domset bits
**	06-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    initialized new db_integrity field.
**	08-may-89 (ralph)
**	    Initialize reserved field to blanks (was \0).
**	19-may-89 (andre)
**	    Change query sent to determine if there are any rows in the relation
**	    that violate the integrity qualification from
**	    (1) retrieve (a = R.tid where !qual) TO
**	    (2) retrieve (a = count(R.tid) - count(R.tid where qual)).  If the
**	    result is non-zero, integrity doesn't hold.  This change was made to
**	    fix bug a problewm that results from the fact that if the column on
**	    which integrity is defined has NULL(s) in it and the integrity
**	    doesn't explicitly exclude NULLs, (1) would not detect it, while the
**	    subsequent attempts to INSERT NULLs into this column would fail.
**	06-jun-89 (ralph)
**	    Fix unix portability problems
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**	17-apr-90 (andre)
**	    If qef_call() returns with QE0022 (query aborted) we should not try
**	    to delete the newly created integrity from the catalog, as the
**	    transaction has been aborted + use 4708L insead of E_PS0003 since
**	    we can not differentiate between run-time error and user interrupt.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	14-sep-90 (teresa)
**	    psq_force changed from boolean to bitflag
**	09-nov-90 (stec)
**	    Initialize opf_thandle in OPF_CB.
**	11-nov-91 (rblumer)
**	  merged from 6.4:  26-feb-91 (andre)
**	    PST_QTREE was changed to store the range table as an array of
**	    pointers to PST_RNGENTRY structure.
**	15-jun-92 (barbara)
**	    Changed interface to pst_rgent().
**	03-aug-92 (barbara)
**	    Initalize RDF_CB by calling pst_rdfcb_init.  After defining the
**	    integrity, RDF must refresh its cached infoblk.  Previously the
**	    code flushed the cache and then requested the tableinfo again;
**	    now we omit the flush and request the tableinfo with the "refresh"
**	    bit on.  If definition phase fails, we invalidate the infoblk.
**	15-oct-92 (rblumer)
**	    Initalize new fields in DB_INTEGRITY tuple by setting 
**          entire tuple to zeros.  Change dbi_domset to dbi_columns.
**	08-apr-93 (ralph)
**	    DELIM_IDENT:  Use appropriate case for "tid"
**	11-jun-93 (rblumer)
**	    fix spurious RDF_INVALIDATE error by checking correct variable for
**	    failure (i.e. stat instead of status).
**	30-sep-93 (stephenb)
**	    pass tablename from psy_cb to rdf_rb before calling rdf_update() so
**	    that we can use the information to audit in QEF.
**	10-sep-96 (nanpr01)
**	    bug 85107 : user error in create integrity cause the session
**	    to terminate. Actually, qeq_query aborts any error correctly
**	    consequently, we donot need to call rdf_update with delete flag.
**	27-nov-02 (inkdo01)
**	    Range table expansion.
**	25-Jun-2008 (kiria01) SIR120473
**	    Move pst_isescape into new u_i2 pst_pat_flags.
*/
DB_STATUS
psy_dinteg(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb,
	QEU_CB		   *qeu_cb)
{
    DB_STATUS           status;
    DB_STATUS		stat;
    DB_INTEGRITY	inttuple;
    register DB_INTEGRITY *inttup;
    RDF_CB		rdf_cb;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    QSF_RCB		qsf_rb;
    PST_PROCEDURE	*pnode;
    PST_QTREE		*qtree;
    i4		err_code;
    PSF_MSTREAM		mstream;
    PST_QNODE		*qualnode, *tidnode, *aopnode, *bopnode,
			*agheadnode1, *agheadnode2, *resdomnode, *qlendnode;
    PST_OP_NODE		operator;
    PST_VAR_NODE	var;
    PST_RSDM_NODE	resdom;
    PST_RT_NODE	        aghead;
    QEF_DATA		qresult;
    PST_RT_NODE		*root;
    i4			qdata = (i4) 0;
    OPF_CB		opf_cb;
    i4			textlen, tree_lock = 0,
    			text_lock = 0, tree_exists = 1,
    			delinteg = 0;
#ifdef    xDEBUG
    i4		val1;
    i4		val2;
#endif

    /* Fill in the RDF request block */

    rdf_rb->rdr_r1_distrib = sess_cb->pss_distrib;

    pst_rdfcb_init(&rdf_cb, sess_cb);

    /* The table which is receiving the integrity */
    STRUCT_ASSIGN_MACRO(psy_cb->psy_tables[0], rdf_rb->rdr_tabid);

    /* Tell RDF we're doing an integrity definition */
    rdf_rb->rdr_types_mask = RDR_INTEGRITIES;
    rdf_rb->rdr_update_op = RDR_APPEND;

    /* Get the tree from QSF */
    qsf_rb.qsf_type	= QSFRB_CB;
    qsf_rb.qsf_ascii_id	= QSFRB_ASCII_ID;
    qsf_rb.qsf_length	= sizeof(qsf_rb);
    qsf_rb.qsf_owner	= (PTR)DB_PSF_ID;
    qsf_rb.qsf_sid	= sess_cb->pss_sessid;
    STRUCT_ASSIGN_MACRO(psy_cb->psy_intree, qsf_rb.qsf_obj_id);
    qsf_rb.qsf_lk_state	= QSO_EXLOCK;
    status = qsf_call(QSO_LOCK, &qsf_rb);
    if (DB_FAILURE_MACRO(status))
    {
	(VOID) psf_error(E_PS0D19_QSF_INFO, qsf_rb.qsf_error.err_code,
	    PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	goto exit;
    }
    pnode = (PST_PROCEDURE *) qsf_rb.qsf_root;
    qtree = pnode->pst_stmts->pst_specific.pst_tree;
    root = &qtree->pst_qtree->pst_sym.pst_value.pst_s_root;
    tree_lock = qsf_rb.qsf_lk_id;

    /* Get the query text */
    STRUCT_ASSIGN_MACRO(psy_cb->psy_qrytext, qsf_rb.qsf_obj_id);
    qsf_rb.qsf_lk_state = QSO_EXLOCK;
    status = qsf_call(QSO_LOCK, &qsf_rb);
    if (DB_FAILURE_MACRO(status))
    {
	(VOID) psf_error(E_PS0D19_QSF_INFO, qsf_rb.qsf_error.err_code,
	    PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	goto exit;
    }
    MEcopy((PTR) qsf_rb.qsf_root, sizeof(i4), (PTR) &textlen);
    rdf_rb->rdr_qry_root_node	= (PTR) pnode;
    rdf_rb->rdr_l_querytext	= textlen;
    rdf_rb->rdr_querytext	= ((char *) qsf_rb.qsf_root) + sizeof(i4);
    text_lock			= qsf_rb.qsf_lk_id;

    /* NULL out the DMU control block pointer, just in case */
    rdf_rb->rdr_dmu_cb = (PTR) NULL;

    /* Give RDF pointer to integrity tuple */
    inttup = &inttuple;
    rdf_rb->rdr_qrytuple = (PTR) inttup;

    /* Init entire tuple to nulls */
    (VOID) MEfill(sizeof(inttuple), NULLCHAR, (PTR) inttup);

    /* Fill in the integrity tuple from the psy_cb */

    inttup->dbi_seq = 0;		    /* Init sequence number */

    /* The table which is receiving the integrity */
    STRUCT_ASSIGN_MACRO(psy_cb->psy_tables[0], inttup->dbi_tabid);

    makeidset(qtree->pst_restab.pst_resvno, qtree->pst_qtree,
	      inttup->dbi_columns.db_domset);

    /* Number of result range variable */
    inttup->dbi_resvar = qtree->pst_restab.pst_resvno;

    if (sess_cb->pss_lang == DB_SQL)
	rdf_rb->rdr_status |= DBQ_SQL;

    /* Pass tablename to RDF */
    rdf_rb->rdr_name.rdr_tabname = psy_cb->psy_tabname[0];

    /* Let RDF put integrity definition in catalogs */
    status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
    if (DB_FAILURE_MACRO(status))
    {
	if (rdf_cb.rdf_error.err_code == E_RD0002_UNKNOWN_TBL)
	{
	    (VOID) psf_error(E_PS0903_TAB_NOTFOUND, 0L, PSF_USERERR, &err_code,
		&psy_cb->psy_error, 1, 
		psf_trmwhite(sizeof(psy_cb->psy_tabname[0]),
		    (char *) &psy_cb->psy_tabname[0]),
		&psy_cb->psy_tabname[0]);
	}
	else
	{
	    (VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error,
		&psy_cb->psy_error);
	}
	goto exit;
    }

    /* refresh the range table since timestamps have changed as a result
    ** of adding the integrity.
    */
    {
	bool		    in_retry = (sess_cb->pss_retry & PSS_REFRESH_CACHE);
	DB_TAB_TIMESTAMP    *timestamp;
	PSS_RNGTAB	    *rngvar;

	sess_cb->pss_retry |= PSS_REFRESH_CACHE;
	status = pst_rgent(sess_cb, &sess_cb->pss_auxrng, -1, "", PST_SHWID,
	    (DB_TAB_NAME *) NULL, (DB_TAB_OWN *) NULL, &psy_cb->psy_tables[0],
	    FALSE, &rngvar, (i4) 0, &psy_cb->psy_error);
	if (DB_FAILURE_MACRO(status))
	{
	    delinteg++;
	    goto exit;
	}
	timestamp = &rngvar->pss_tabdesc->tbl_date_modified;
	STRUCT_ASSIGN_MACRO(*timestamp, qtree->pst_rangetab[0]->pst_timestamp);
	root->pst_tvrc = 1;
	MEfill(sizeof(PST_J_MASK), 0, (char *)&root->pst_tvrm);
	BTset(rngvar->pss_rgno, (char *) &root->pst_tvrm);
	if (!in_retry)
	    sess_cb->pss_retry &= ~PSS_REFRESH_CACHE;
    }

    /*
    ** The integrity is represented by a qualification, which is given in
    ** qtree.
    ** Build a tree for retrieve(a=count(R.tid) - count(R.tid where qual))
    ** Optimize and run this query.  If the result is not zero, there exists at
    ** least one row that doesn't match the original qualification.
    ** In this case, give the user an error.
    */

    /* Don't validate integrity if this is upgradedb */
    if (sess_cb->pss_ses_flag & PSS_RUNNING_UPGRADEDB)
	goto exit;

    /* Copy the object id and lock id to a memory stream */
    STRUCT_ASSIGN_MACRO(psy_cb->psy_intree, mstream.psf_mstream);
    mstream.psf_mlock = tree_lock;

    /*
    ** Now turn the query tree from an integrity into
    ** "retrieve (count(R.tid) - count(R.tid where qual))"
    */
    
    qtree->pst_mode = PSQ_RETRIEVE;
    qtree->pst_agintree = TRUE;

    /* First build "count(R.tid where qual)" */
    
    qualnode = psy_trmqlnd(qtree->pst_qtree->pst_right);

    /* Create a TID node */
    var.pst_vno = qtree->pst_restab.pst_resvno;
    var.pst_atno.db_att_id = 0;	    /* Tid is attribute zero */
    STmove(((*sess_cb->pss_dbxlate & CUI_ID_REG_U) ? "TID" : "tid"),
	   ' ', sizeof(var.pst_atname), (char *) &var.pst_atname);
    status = pst_node(sess_cb, &mstream, (PST_QNODE *) NULL, (PST_QNODE *) NULL,
	PST_VAR, (PTR) &var, sizeof(var), DB_INT_TYPE, (i2) 0, (i4) 4,
	(DB_ANYTYPE *) NULL, &tidnode, &psy_cb->psy_error, (i4) 0);
    if (DB_FAILURE_MACRO(status))
    {
	delinteg++;
	goto exit;
    }

    /* Create an AOP node for "count" */
    operator.pst_opno = ADI_CNT_OP;
    operator.pst_retnull = FALSE;
    operator.pst_distinct = PST_DONTCARE;
    operator.pst_opmeta = PST_NOMETA;

    status = pst_node(sess_cb, &mstream, tidnode, (PST_QNODE *) NULL, PST_AOP,
	(PTR) &operator, sizeof(operator), DB_NODT, (i2) 0, (i4) 0,
	(DB_ANYTYPE *) NULL, &aopnode, &psy_cb->psy_error, (i4) 0);
    if (DB_FAILURE_MACRO(status))
    {
	delinteg++;
	goto exit;
    }

    aghead.pst_project	= FALSE;		    /* for both SQL, quel */
    aghead.pst_union.pst_next = 0;
    aghead.pst_qlang	= sess_cb->pss_lang;
    aghead.pst_dups	= PST_ALLDUPS;
    aghead.pst_tvrm	= root->pst_tvrm;   /* copy from_list */
    aghead.pst_tvrc	= 1;		    /* there should only be one var */

    /* Allocate the AGHEAD node */
    status = pst_node(sess_cb, &mstream, aopnode, qualnode, PST_AGHEAD, 
	(PTR) &aghead,
	sizeof(PST_RT_NODE), aopnode->pst_sym.pst_dataval.db_datatype,
	aopnode->pst_sym.pst_dataval.db_prec,
	aopnode->pst_sym.pst_dataval.db_length, (DB_ANYTYPE *) NULL,
	&agheadnode1, &psy_cb->psy_error, (i4) 0);
    if (DB_FAILURE_MACRO(status))
    {
	delinteg++;
	goto exit;
    }

    /* build count(R.tid) */

    /*
    ** Create a TID node; note that it's been build once before, so we do not
    ** need to initialize
    */

    status = pst_node(sess_cb, &mstream, (PST_QNODE *) NULL, (PST_QNODE *) NULL,
	PST_VAR, (PTR) &var, sizeof(var), DB_INT_TYPE, (i2) 0, (i4) 4,
	(DB_ANYTYPE *) NULL, &tidnode, &psy_cb->psy_error, (i4) 0);
    if (DB_FAILURE_MACRO(status))
    {
	delinteg++;
	goto exit;
    }

    /*
    ** Create an AOP node for "count"; note that it's been build once
    **  before, so we do not need to initialize
    */

    status = pst_node(sess_cb, &mstream, tidnode, (PST_QNODE *) NULL, PST_AOP,
	(PTR) &operator, sizeof(operator), DB_NODT, (i2) 0, (i4) 0,
	(DB_ANYTYPE *) NULL, &aopnode, &psy_cb->psy_error, (i4) 0);
    if (DB_FAILURE_MACRO(status))
    {
	delinteg++;
	goto exit;
    }

    /*
    ** Create a QLEND node since we are building count(R.tid), so there is
    ** no qualification involved
    */
    status = pst_node(sess_cb, &mstream, (PST_QNODE *) NULL, (PST_QNODE *) NULL,
	PST_QLEND, (PTR) NULL, 0, DB_NODT, (i2) 0, (i4) 0, (DB_ANYTYPE *) NULL,
	&qlendnode, &psy_cb->psy_error, (i4) 0);
    if (DB_FAILURE_MACRO(status))
    {
	delinteg++;
	goto exit;
    }

    /*
    ** Build AGHEAD node; note that it's been build once before, so we do not
    ** need to initialize
    */

    status = pst_node(sess_cb, &mstream, aopnode, qlendnode, PST_AGHEAD, 
	(PTR) &aghead,
	sizeof(PST_RT_NODE), aopnode->pst_sym.pst_dataval.db_datatype,
	aopnode->pst_sym.pst_dataval.db_prec,
	aopnode->pst_sym.pst_dataval.db_length, (DB_ANYTYPE *) NULL,
	&agheadnode2, &psy_cb->psy_error, (i4) 0);
    if (DB_FAILURE_MACRO(status))
    {
	delinteg++;
	goto exit;
    }

    /* Build a BOP node for "-" */

    operator.pst_opno = ADI_SUB_OP;
    operator.pst_opmeta = PST_NOMETA;
    operator.pst_pat_flags = AD_PAT_DOESNT_APPLY;
    
    status = pst_node(sess_cb, &mstream, agheadnode2, agheadnode1, PST_BOP,
	(PTR) &operator, sizeof(operator), DB_NODT, (i2) 0, (i4) 0,
	(DB_ANYTYPE *) NULL, &bopnode, &psy_cb->psy_error, (i4) 0);
    if (DB_FAILURE_MACRO(status))
    {
	delinteg++;
	goto exit;
    }

    /* Set up a result domain */
    resdom.pst_rsno	= 1;
    resdom.pst_ntargno	= resdom.pst_rsno;
    resdom.pst_ttargtype= PST_USER;
    resdom.pst_rsupdt	= FALSE;	/* Not an updateable cursor */
    resdom.pst_rsflags  = PST_RS_PRINT;
    status = pst_node(sess_cb, &mstream, qtree->pst_qtree->pst_left, bopnode,
	PST_RESDOM, (PTR) &resdom, sizeof(resdom), DB_INT_TYPE, (i2) 0, (i4) 4,
	(DB_ANYTYPE *) NULL, &resdomnode, &psy_cb->psy_error, (i4) 0);
    if (DB_FAILURE_MACRO(status))
    {
	delinteg++;
	goto exit;
    }

    /* Create a QLEND node to replace the qualification in the main query */
    status = pst_node(sess_cb, &mstream, (PST_QNODE *) NULL, (PST_QNODE *) NULL,
	PST_QLEND, (PTR) NULL, 0, DB_NODT, (i2) 0, (i4) 0, (DB_ANYTYPE *) NULL,
	&qlendnode, &psy_cb->psy_error, (i4) 0);
    if (DB_FAILURE_MACRO(status))
    {
	delinteg++;
	goto exit;
    }

    /* Hook up the query and QLEND to the root node */
    qtree->pst_qtree->pst_left = resdomnode;
    qtree->pst_qtree->pst_right = qlendnode;

#ifdef	xDEBUG
    /*
    ** Print the final query tree.
    */
    if (ult_check_macro(&sess_cb->pss_trace,
				PSS_LONG_QRY_TREE_TRACE, &val1, &val2))
    {   
	TRdisplay("Final query tree:\n\n\n");
	status = pst_prmdump(qtree->pst_qtree, qtree,
	    &psy_cb->psy_error, DB_PSF_ID);
    }
    if (ult_check_macro(&sess_cb->pss_trace,
				PSS_SHORT_QRY_TREE_TRACE, &val1, &val2))
    {   
	TRdisplay("Final query tree:\n\n\n");
	status = pst_1prmdump(qtree->pst_qtree, qtree, &psy_cb->psy_error);
    }
#endif

    /* Unlock the query tree from QSF to pass to OPF */
    STRUCT_ASSIGN_MACRO(psy_cb->psy_intree, qsf_rb.qsf_obj_id);
    qsf_rb.qsf_lk_id = tree_lock;

    status = qsf_call(QSO_UNLOCK, &qsf_rb);
    if (DB_FAILURE_MACRO(status))
    {
	(VOID) psf_error(E_PS0D1C_QSF_UNLOCK, qsf_rb.qsf_error.err_code,
	    PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	delinteg++;
	goto exit;
    }
    tree_lock = 0;

    /* Now optimize the query */
    opf_cb.opf_length	= sizeof(opf_cb);
    opf_cb.opf_type	= OPFCB_CB;
    opf_cb.opf_owner	= (PTR)DB_PSF_ID;
    opf_cb.opf_ascii_id = OPFCB_ASCII_ID;
    STRUCT_ASSIGN_MACRO(psy_cb->psy_intree, opf_cb.opf_query_tree);
    opf_cb.opf_scb = NULL;	/* OPF must get its own session cb */
    opf_cb.opf_thandle = NULL;
    /* this call will destroy query tree */
    tree_exists = 0;
#ifdef xDEBUG
    if (ult_check_macro(&sess_cb->pss_trace,
				PSS_NO_INTEG_VALIDATION_TRACE, &val1, &val2) == 0)
    {
#endif
	status = opf_call(OPF_CREATE_QEP, &opf_cb);
	if (DB_FAILURE_MACRO(status))
	{
	    switch (opf_cb.opf_errorblock.err_code)
	    {
		case E_OP008F_RDF_MISMATCH:
		    psy_cb->psy_error.err_code = E_PS0008_RETRY;
		    break;
		default:
		    psy_cb->psy_error.err_code = E_PS0D1F_OPF_ERROR;
		    break;
	    }
	    delinteg++;
	    goto exit;
	}
#ifdef xDEBUG
    }
#endif
    /* Now run the query */
    qeu_cb->qeu_qso = opf_cb.opf_qep.qso_handle;
    qeu_cb->qeu_input = NULL;
    qeu_cb->qeu_pcount = 0;
    qeu_cb->qeu_count = 1;
    qeu_cb->qeu_output = &qresult;
    qresult.dt_next = (QEF_DATA *) NULL;    /* Only one row comes back */
    qresult.dt_size = sizeof(qdata);	    /* It contains an i4 */
    qresult.dt_data = (PTR) &qdata;
#ifdef xDEBUG
    if (ult_check_macro(&sess_cb->pss_trace,
				PSS_NO_INTEG_VALIDATION_TRACE, &val1, &val2) == 0)
    {
#endif
	/*
	** The qef_call should be called twice in order for the table to be
	** closed, which is required for qef_call(ETRAN,..) to work properly.
	** 1 row of data is expected back (ANY aggregate).
	*/
	for (;qeu_cb->error.err_code != E_QE0015_NO_MORE_ROWS;)
	{
	    status = qef_call(QEU_QUERY, ( PTR ) qeu_cb);

	    if (DB_FAILURE_MACRO(status))
	    {
		switch (qeu_cb->error.err_code)
		{
		    /* object unknown */
		    case E_QE0031_NONEXISTENT_TABLE:		
			    {
				psy_cb->psy_error.err_code = E_PS0008_RETRY;
				break;
			    }
		    /* interrupt */
		    case E_QE0022_QUERY_ABORTED:
			    {
				(VOID) psf_error(4708L, 0L, PSF_USERERR,
				    &err_code, &psy_cb->psy_error, 0);
				/*
				** note that we do not increment delinteg since
				** there is no integrity to delete from the
				** catalog as the transaction has been aborted.
				*/
				goto exit;
			    }
		    /* resource related */
		    case E_QE000D_NO_MEMORY_LEFT:
		    case E_QE000E_ACTIVE_COUNT_EXCEEDED:
		    case E_QE000F_OUT_OF_OTHER_RESOURCES:
		    case E_QE001E_NO_MEM:
			    {
				(VOID) psf_error(E_PS0D23_QEF_ERROR,
				    qeu_cb->error.err_code, PSF_USERERR,
				    &err_code, &psy_cb->psy_error, 0);
				delinteg++;
				goto exit;
				break;
			    }
		    /* lock timer */
		    case E_QE0035_LOCK_RESOURCE_BUSY:
		    case E_QE0036_LOCK_TIMER_EXPIRED:
			    {
				(VOID) psf_error(4702L,
				    qeu_cb->error.err_code, PSF_USERERR,
				    &err_code, &psy_cb->psy_error, 0);
				delinteg++;
				goto exit;
				break;
			    }
		    /* resource quota */
		    case E_QE0052_RESOURCE_QUOTA_EXCEED:
		    case E_QE0067_DB_OPEN_QUOTA_EXCEEDED:
		    case E_QE0068_DB_QUOTA_EXCEEDED:
		    case E_QE006B_SESSION_QUOTA_EXCEEDED:
			    {
				(VOID) psf_error(4707L,
				    qeu_cb->error.err_code, PSF_USERERR,
				    &err_code, &psy_cb->psy_error, 0);
				delinteg++;
				goto exit;
				break;
			    }
		    /* log full */
		    case E_QE0024_TRANSACTION_ABORTED:
			    {
				(VOID) psf_error(4706L,
				    qeu_cb->error.err_code, PSF_USERERR,
				    &err_code, &psy_cb->psy_error, 0);
				break;
			    }
		    /* deadlock */
		    case E_QE002A_DEADLOCK:
			    {
				(VOID) psf_error(4700L,
				    qeu_cb->error.err_code, PSF_USERERR,
				    &err_code, &psy_cb->psy_error, 0);
				break;
			    }
		    /* lock quota */
		    case E_QE0034_LOCK_QUOTA_EXCEEDED:
			    {
				(VOID) psf_error(4705L,
				    qeu_cb->error.err_code, PSF_USERERR,
				    &err_code, &psy_cb->psy_error, 0);
				break;
			    }
		    /* inconsistent database */
		    case E_QE0099_DB_INCONSISTENT:
			    {
				(VOID) psf_error(38L,
				    qeu_cb->error.err_code, PSF_USERERR,
				    &err_code, &psy_cb->psy_error, 0);
				break;
			    }
		    case E_QE0025_USER_ERROR:
			    {
			      psy_cb->psy_error.err_code = E_PS0001_USER_ERROR;
			      goto exit;
			      break;
			    }
		    default:
			    {
				(VOID) psf_error(E_PS0D1E_QEF_ERROR,
				    qeu_cb->error.err_code,
				    PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
				delinteg++;
				goto exit;
				break;
			    }
		}

		delinteg++;
		goto exit;
	    }
	}

#ifdef xDEBUG
    }
#endif
    status = E_DB_OK;
    /*
    ** If the aggregate doesn't return zero, it means the integrity doesn't
    ** hold initially, so we should complain to the user and back out.
    */
    if (qdata != (i4) 0)
    {
	(VOID) psf_error(3492L, 0L, PSF_USERERR, &err_code, 
	    &psy_cb->psy_error, 1,
	    psf_trmwhite(sizeof(psy_cb->psy_tabname[0]), 
		(char *) &psy_cb->psy_tabname[0]),
	    &psy_cb->psy_tabname[0]);

	delinteg++;
	/* Commented out, because we want to return good status to commit rather
	** than abort in this case. Abort is done `manually' by calling RDF to
	** remove integrity at the end of this proc.
	/*	status = E_DB_ERROR;	*/
	goto exit;
    }
exit:
    if (tree_exists)
    {
	/* Destroy query tree */
	STRUCT_ASSIGN_MACRO(psy_cb->psy_intree, qsf_rb.qsf_obj_id);
	qsf_rb.qsf_lk_id	= tree_lock;
	if (tree_lock == 0)
	{
	    stat = qsf_call(QSO_LOCK, &qsf_rb);
	    if (DB_FAILURE_MACRO(stat))
	    {
		(VOID) psf_error(E_PS0D18_QSF_LOCK, qsf_rb.qsf_error.err_code,
		    PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
		if (stat > status)
		    status = stat;
	    }
	    tree_lock	= qsf_rb.qsf_lk_id;
	}
	stat = qsf_call(QSO_DESTROY, &qsf_rb);
	if (DB_FAILURE_MACRO(stat))
	{
	    (VOID) psf_error(E_PS0D1A_QSF_DESTROY, qsf_rb.qsf_error.err_code,
		PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	    if (stat > status)
		status = stat;
	}
	tree_lock = 0;
    }    
    /* Destroy query text */
    STRUCT_ASSIGN_MACRO(psy_cb->psy_qrytext, qsf_rb.qsf_obj_id);
    qsf_rb.qsf_lk_id	= text_lock;
    if (text_lock == 0)
    {
	stat = qsf_call(QSO_LOCK, &qsf_rb);
	if (DB_FAILURE_MACRO(stat))
	{
	    (VOID) psf_error(E_PS0D18_QSF_LOCK, qsf_rb.qsf_error.err_code,
		PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	    if (stat > status)
		status = stat;
	}
	text_lock	= qsf_rb.qsf_lk_id;
    }

    stat = qsf_call(QSO_DESTROY, &qsf_rb);
    if (DB_FAILURE_MACRO(stat))
    {
	(VOID) psf_error(E_PS0D1A_QSF_DESTROY, qsf_rb.qsf_error.err_code,
	    PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	if (stat > status)
	    status = stat;
    }
    text_lock = 0;

    /* Now destroy integrity definition if necessary */
    if (delinteg)
    {
	rdf_rb->rdr_update_op = RDR_DELETE;
	rdf_rb->rdr_qrymod_id = inttup->dbi_number;
	stat = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
	if (DB_FAILURE_MACRO(stat))
	{
	    stat = E_DB_SEVERE;
	    (VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error,
		&psy_cb->psy_error);
	}

	if (stat > status)
	    status = stat;
    }

    pst_rdfcb_init(&rdf_cb, sess_cb);
    STRUCT_ASSIGN_MACRO(psy_cb->psy_tables[0], rdf_rb->rdr_tabid);
    stat = rdf_call(RDF_INVALIDATE, (PTR) &rdf_cb);
    if (DB_FAILURE_MACRO(stat))
    {
	(VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_cb.rdf_error,
			&psy_cb->psy_error);
    }
    if (stat > status)
	status = stat;

    return    (status);
}

/*{
** Name: makeidset	- Make domain set for integrity
**
** Description:
**	This function makes a bit map of the columns checked by an integrity.
**	It assumes the bit map is initially empty.
**
** Inputs:
**      varno                           The range variable number to check
**	tree				Pointer to query tree to check
**	dset				Pointer to bit map
**
** Outputs:
**      dset                            Filled with map of columns checked
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-jul-86 (jeff)
**          Adapted from makeidset() in 4.0
*/
static VOID
makeidset(
	register i4        varno,
	register PST_QNODE *tree,
	u_i4		   dset[])
{
    while (tree != (PST_QNODE *) NULL)
    {
	if (tree->pst_sym.pst_type == PST_VAR &&
	    tree->pst_sym.pst_value.pst_s_var.pst_vno == varno)
	{
	    BTset((i4) tree->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id, 
		(char *) dset);
	}

	/* Handle left subtree recursively */
	makeidset(varno, tree->pst_left, dset);

	/* Handle right tree iteratively */
	tree = tree->pst_right;
    }
}
