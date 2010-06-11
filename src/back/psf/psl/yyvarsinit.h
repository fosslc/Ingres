/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: YYVARSINIT.H - create and initialize local variable for grammar.
**
** Description:
**  This file will be included in xxxparse to define and initialize local 
**  variable of type defined in YYVARS.H. This mechanism is provided so 
**  that variable, which transcends productions, can be defined.
**
**  To make YACC include this file, define YACC_VARS in the grammar.
**
**  Two variables must always be created if YACC_VARS has been specified,
**  yyvars and a pointer to it yyvarsp. All references in actions of the 
**  grammar to fields defined in the variable must be done in the following
**  way:
**		    $Yfield_name
**  
**
** History:
**  13-nov-87 (stec)
**      Created.
**  19-jan-88 (stec)
**	Added list_clause initialization.
**  27-oct-88 (stec)
**	Added initialization for updcollst.
**	(was added on 16-nov-1988)
**  09-mar-89 (neil)
**	Added initialization of in_rule.
**  03-aug-89 (andre)
**	set in_groupby_clause to 0.  Set var_names.numvars to -1 to indicate
**	that no names have been stored yet.
**  15-may-90 (andre)
**	initialize newly defined illegal_agg_relmask.
**  16-may-90 (andre)
**	initialize flists[0] since it is never be initialized in SELECT query,
**	but we may need to check it.
**  19-feb-91 (andre)
**	initialize nonupdt to FALSE (part of fix for bug #35872)
**  15-jun-92 (barbara)
**	Sybil merge.  Initialize distributed-only fields.
**  31-aug-92 (barbara)
**	Initialized pss_skip for Star.
**  26-sep-92 (andre)
**	var_names has been replaced with tbl_refs, so instead of initializing
**	var_names.numvars to -1 we will initilaize tbl_refs to NULL
**  25-oct-92 (rblumer)
**	Improvements for FIPS CONSTRAINTS.
**	Initialize cons_list pointer (which will hold constraint info).
**  12-oct-92 (ralph)
**	id_type is initialized to 0 in support of delimited identifiers.
**  23-nov-92 (ralph)
**      CREATE SCHEMA:
**	initialize yyvars.bypass_actions, .submode, .deplist, .stmtstart
**  20-nov-92 (rblumer)
**	Initialize default_tree to NULL.
**  18-dec-92 (andre)
**	init underlying_rel and nonupdt_reason
**  20-jan-93 (rblumer)
**	Initialize default_text to NULL, change default_tree to default_node.
**  10-mar-93 (barbara)
**	Moved ldb_flags field to PSS_SESBLK.
**  19-nov-97 (inkdo01)
**	Init inhaving for count(*)-in-where-clause detection.
**  27-nov-98 (inkdo01)
**	Init in_orderby for non-select list order by entries.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	3-mar-00 (inkdo01)
**	    init first_n (row count for "select first n ...")
**	11-mar-02 (inkdo01)
**	    Init seq_ops for sequence support.
**	31-Jan-2004 (schka24)
**	    Make sure we're doing regular keywords.
**	19-jan-06 (dougi)
**	    Add DERIVED_TABLE symbol to allow init of additional YYVARS'.
**	15-Mar-2006 (kschendel)
**	    Init the arg stack stuff.
**	9-may-2007 (dougi)
**	    Init loc_count for FREE LOCATOR.
**	19-june-2007 (dougi)
**	    Init in_update_set_clause.
**	18-july-2007 (dougi)
**	    Init offset_n.
**    25-Oct-2007 (smeke01) b117218
**        Init in_case_function.
**	19-Mar-2008 (kschendel) b122118
**	    Remove some ancient unused index-structure stuff.
**	19-march-2008 (dougi)
**	    Init named_parms.
**	6-june-2008 (dougi)
**	    Init done_identity and sequence for identity columns.
**	7-May-2009 (kibro01) b121738
**	    Add union_head setting so that ORDER BY can determine whether
**	    a column is on both sides of a UNION correctly.
**	16-Jun-2009 (kiria01) SIR 121883
**	    Scalar Sub-queries changes.
**	18-Mar-2010 (kiria01) b123438
**	    Scalar sub-query support changes. Renamed qual_type bit vector
**	    to more meaningful in_join_search and added similar bitvector
**	    for SINGLETON support.
**	23-Mar-2010 (kiria01) b123438
**	    Added first_n_ok_depth
**	02-Apr-2010 (kiria01) b123536
**	    Add comment relating to initialising of pointers:
**		NOTE: Pointers initialised in here to refer to the block
**		being initialised - &yyvars - will remain pointing at the
**		block if the block is on the stack such as is the case
**		with psl_push_yyvars(). In this case these pointers must
**		be initialised properly in any roueing that copies the block
**		such as psl_push_yyvars() - otherwise their will point into
**		stack that will get corrupted!
**      02-Jun-2010 (coomi01) b123821
**           Init save_seq_ops array.
**     
**/


YYVARS	yyvars, *yyvarsp = &yyvars, **yyvarspp = &yyvarsp;
i4	i;

#ifndef DERIVED_TABLE
/* Not a yyvars, but I'd rather put this here than byacc.par */
cb->pss_yacc->yy_partalt_kwds = FALSE;
#endif

yyvars.prev_yyvarsp = (PSS_YYVARS *) NULL;
yyvars.with_journaling 	= 0;
yyvars.with_location	= 0;
yyvars.with_dups	= PST_DNTCAREDUPS;
yyvars.is_heapsort	= 0;
yyvars.in_from_clause	= 0;
yyvars.in_where_clause	= 0;
yyvars.in_target_clause = 0;
yyvars.in_groupby_clause = FALSE;
yyvars.in_update_set_clause = FALSE;
yyvars.in_orderby       = FALSE;
yyvars.in_rule		= 0;
yyvars.sort_list	= 0;
yyvars.sort_by		= 0;
yyvars.agg_func		= 0;
yyvars.aggr_allowed	= (PSS_AGG_ALLOWED | PSS_STMT_AGG_ALLOWED);
yyvars.list_clause	= 0;
(VOID)MEfill(sizeof(yyvars.fe_cursor_id), (u_char)0, (PTR)&yyvars.fe_cursor_id);
yyvars.qp_shareable	= TRUE;
yyvars.exprlist         = (PSS_EXLIST *) NULL;
yyvars.isdbp		= FALSE;
yyvars.dbpinfo		= (PSS_DBPINFO *) NULL;
yyvars.updcollst	= (PST_QNODE *) NULL;
yyvars.check_for_vlups 	= FALSE;
yyvars.tbl_refs		= (PSS_TBL_REF *) NULL;
yyvars.pss_join_info.depth = -1;
yyvars.join_id          = PST_NOJOIN;
(VOID)MEfill(sizeof(PST_J_MASK), 0, (char *)&yyvars.join_tbls);
yyvars.bypass_actions	= FALSE;
yyvars.submode		= 0;
yyvars.stmtstart	= (char *) NULL;
yyvars.deplist		= (PST_OBJDEP *) NULL;
yyvars.underlying_rel   = (PSS_RNGTAB *) NULL;
yyvars.nonupdt_reason   = 0;
yyvars.first_n_ok_depth	= 1;
yyvars.first_n		= (PST_QNODE *) NULL;
yyvars.offset_n		= (PST_QNODE *) NULL;
yyvars.seq_ops		= FALSE;
(VOID)MEfill(sizeof(yyvars.save_seq_ops), 0, (char *)&yyvars.save_seq_ops);
yyvars.in_case_function = 0;
yyvars.dynqp_comp	= FALSE;
yyvars.repeat_dyn	= FALSE;
yyvars.named_parm	= FALSE;
yyvars.in_from_tproc	= FALSE;
yyvars.done_identity	= FALSE;
yyvars.seqp		= &yyvars.sequence; /* Needs deferred pointer init */
(VOID)MEfill(sizeof(DB_IISEQUENCE), 0, (char *)&yyvars.sequence);
QUinit((QUEUE *)&yyvars.dcol_list); /* Needs deferred pointer init */

/*
** when an operator is encountered while not processing a qualification, we will
** be checking the 0-th bit of in_join_search
*/
yyvars.qual_depth       = 0;
BTclear(0, (char *) yyvars.in_join_search);
yyvars.singleton_depth	= 0;
BTclear(0, (char *) yyvars.singleton);
yyvars.j_qual = &yyvars.first_qual; /* Needs deferred pointer init */
yyvars.j_qual->pss_next = (PSS_J_QUAL *) NULL;
yyvars.j_qual->pss_qual = (PST_QNODE *) NULL;

/* Initialize fields in dup_rb */
yyvars.dup_rb.pss_op_mask = 0;
yyvars.dup_rb.pss_num_joins = PST_NOJOIN;
yyvars.dup_rb.pss_tree_info = (i4 *) NULL;
yyvars.dup_rb.pss_mstream = &cb->pss_ostream;
yyvars.dup_rb.pss_err_blk = &psq_cb->psq_error;
for (i = 0; i < PST_NUMVARS; i++)
{
    *(yyvars.rng_vars + i) = (PSS_RNGTAB *) NULL;
}
(VOID)MEfill(sizeof(PST_J_MASK), 0, (char *)&yyvars.illegal_agg_relmask);
(VOID)MEfill(sizeof(PST_J_MASK), 0, (char *)&yyvars.flists[0]);
yyvars.qry_mask		    = (i4) 0;
yyvars.mult_corr_attrs.depth = -1;
yyvars.corr_aggr.depth	= -1;
yyvars.nonupdt		= FALSE;
yyvars.inhaving		= 0;
yyvars.id_type		= 0;

yyvars.cons_list        = (PSS_CONS *) NULL;
yyvars.default_node     = (PST_QNODE *) NULL;
yyvars.default_text     = (DB_TEXT_STRING *) NULL;
yyvars.max_args		= PSS_INITIAL_ARGS;
yyvars.func_args	= &yyvars.initial_args[0]; /* Needs deferred pointer init */
yyvars.arg_stack_ix	= 0;
yyvars.arg_base		= 0;
yyvars.arg_ix		= 0;
yyvars.loc_count	= 0;
MEfill(sizeof(yyvars.initial_args), 0, &yyvars.initial_args[0]);
yyvars.union_head	= (PST_QNODE *)NULL;

if (cb->pss_distrib & DB_3_DDB_SESS)
{
    /* Set up variables only used in distributed thread */
    yyvars.scanbuf_ptr	= (PTR) NULL;
    yyvars.shr_qryinfo	= (DB_SHR_RPTQRY_INFO *) NULL;
    {
	i4	tmp = (cb->pss_endbuf - cb->pss_qbuf) * PSS_BSIZE_MULT;

	yyvars.xlated_qry.pss_buf_size = (tmp > PSS_MAXBSIZE) ? PSS_MAXBSIZE
							      : tmp;
    }

    yyvars.xlated_qry.pss_q_list.pss_tail =
	yyvars.xlated_qry.pss_q_list.pss_head = (DD_PACKET *)NULL;

    yyvars.xlated_qry.pss_q_list.pss_skip = 0;

#ifndef DERIVED_TABLE
    /* BB_FIX_ME:Shouldn't the following three assignments take
    ** place in psq_call()
    */

    /* copy default LDB description so it may be modified */
    STRUCT_ASSIGN_MACRO(*psq_cb->psq_dfltldb, *psq_cb->psq_ldbdesc);

    /* this flag is set to FALSE for user queries */
    psq_cb->psq_ldbdesc->dd_l1_ingres_b = FALSE;

    /* we may not assume that the LDB id is the same as that of CDB */
    psq_cb->psq_ldbdesc->dd_l5_ldb_id = DD_0_IS_UNASSIGNED;
#endif

}
