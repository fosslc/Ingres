/*
** Copyright (c) 1985, 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <me.h>
#include    <bt.h>
#include    <st.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmacb.h>
#include    <dmucb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <sxf.h>
#include    <qefmain.h>
#include    <qeuqcb.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>
#include    <psyaudit.h>
#include    <cui.h>
/**
**
**  Name: PSLCVIEW.C - Functions used by both SQL and QUEL grammars in parsing
**		       of CREATE/DEFINE VIEW statements.
**
** Function Name Format:  (this applies to externally visible functions only)
** 
**	PSL_CV<number>{[S|Q]}_<production name>
**	
**	where <number> is a unique (within this file) number between 1 and 99
**	
**	if the function will be called only by the SQL grammar, <number> must be
**	followed by 'S';
**	
**	if the function will be called only by the QUEL grammar, <number> must
**	be followed by 'Q';
**
**  Description:
**      This files contains functions used by both SQL and QUEL grammars in
**	parsing of CREATE/DEFINE VIEW statements.
**
**          psl_cv1_create_view	- semantic action for create_view: (defview: in
**				  QUEL) production; verify that a view can be
**				  created + build lists of objects/privileges on
**				  which the view depends.
**	    psl_cv2_viewstmnt	- semantic action for viewstmnt: production
**
**  History:    $Log-for RCS$
**      02-oct-85 (jeff)    
**          written
**      03-sep-86 (seputis)
**          fixed bugs (lint errors)
**      24-jul-89 (jennifer)
**          added code to audit failure to define a view.
**	23-nov-92 (andre)
**	    renamed PSYDVIEW.C to PSLCVIEW.C; combined semantic action for
**	    create_view/defview productions with psy_dview() to form
**	    psl_cv1_create_view().
**	08-apr-93 (ralph)
**	    DELIM_IDENT:  Use appropriate case for "tid"
**	29-jun-93 (andre)
**	    #included ST.H
**	10-aug-93 (andre)
**	    removed causes of compiler warnings
**	15-sep-93 (swm)
**	    Added cs.h include above other header files which need its
**	    definition of CS_SID.
**	18-nov-93 (stephenb)
**	    Include psyaudit.h for prototyping.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp(),
**	    psl_rptqry_tblids(), psl_cons_text(), psl_gen_alter_text(),
**	    psl_tmulti_out(), psq_1rptqry_out(), psq_tout(), pst_treedup().
**	17-Jan-2001 (jenjo02)
**	    Short-circuit calls to psy_secaudit() if not C2SECURE.
**      17-nov-2003 (xu$we01)
**          The view will not be created if user specify the WITH CHECK
**          OPTION clause on:
**          1) A view involving UNION in the outermost subselect.
**          2) A view involving DISTINCT in the outermost subselect.
**          3) A view involving multi-variable in the outermost subselect.
**          4) A view involving GROUP BY in the outermost subselect.
**          5) A view involving HAVING in the outermost subselect.
**          6) A view involving set function (count, avg, sum, etc.)
**             in the outermost subselect.
**	09-feb-2004 (soma01)
**	    Backed out original change for bug 110708 so that it can be
**	    properly fixed later.
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/*
**  Definition of static variables and forward static functions.
*/


/*{
** Name: psl_cv1_create_view - wrap up CREATE/DEFINE VIEW processing; this
**			       includes checking for various syntax errors and
**			       verifying that a view can be created
**
** Description:
**	This function will receive the information collected in the course of
**	parsing a CREATE/DEFINE VIEW statement and will be responsible for
**	checking for various syntax errors (e.g. duplicate column name, WCO on a
**	non-updatable SQL view, etc.) + verifying that the view is not abandoned
**	(i.e. verifying that the owner of the view possesses privileges required
**	to create this view).
**
**	This function will generate a PROCEDURE TREE consisting of a
**	PST_CREATE_VIEW node.
**
**	This function is being created from the code extracted from SQL
**	create_view: and QUEL defview: productions and (what used to be)
**	psy_dview().
**
** Inputs:
**	sess_cb				PSF session CB
**	    pss_object			address of PST_STATEMENT node containing
**					partially initialzied PST_CREATE_VIEW
**					structure
**					
**	psq_cb				PSF query CB
**	
**	new_col_list			list of PST_RESDOMs with names of
**					columns as specified by the user (may be
**					NULL if column names were not specified;
**					will be NULL when called for a QUEL
**					view)
**					
**	query_expr			tree representing the <query expresion>
**
**	check_option			TRUE if WITH CHECK OPTION was specified;
**					FALSE otherwise (always TRUE for QUEL
**					views)
**					
**	yyvarsp				ptr to the variables shared by all
**					grammar productions
**
** Outputs:
**      psq_cb
**	    .psq_error			Filled in if error happens
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates mamory
**
** History:
**	02-oct-85 (jeff)
**          written
**	07-jul-86 (jeff)
**	    actually implemented
**	10-may-88 (stec)
**	    Accept new query tree (db procs).
**	03-sep-88 (andre)
**	    Modify call to pst_rgent to pass 0 as a query mode since it is
**	    clearly not PSQ_DESTROY.
**      24-jul-89 (jennifer)
**          added code to audit failure to define a view.
**	01-mar-90 (andre)
**	    Let RDF know if the view is being created by the DBA.  This is
**	    needed so that RDF would "know" to call QEF to create a special
**	    permit if the user is the dba.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**	21-may-90 (teg)
**	    set DBA_VIEW and VIEW_GRANT_OK in rdr_instr instead of rdr_status.
**	    also init rdr_instr to 0.
**	19-sep-90 (andre)
**	    psy_protect() expects a new parameter - an array of view descriptors
**	    (this way it does not have to call psy_sqlview() to determine if a
**	    given view is QUEL or SQL.  The change in psy_protect() was made to
**	    eliminate repeated calls to RDF in both psy_view() and
**	    psy_protect(), so psy_dview() will have to comply.
**	11-mar-91 (andre)
**	    We will make sure that the view is useable, i.e. run QRYMOD
**	    algorithm over its tree.  Until now we used to simply run
**	    psy_protect() (only) over SQL view trees, and this failed to detect
**	    problems if a view used in this view definition was a QUEL view OR
**	    was owned by the current user.  It is important that this gets done
**	    AFTER we are done looking at the PSF range table as it is likely to
**	    be substantially modified by the qrymod algorithm.
**	    This change was made to fix bug 33038.
**	07-aug-91 (andre)
**	    psy_dview() will determine if the view tree built by the caller is
**	    "updateable", i.e. one relation in the outermost subselect, no
**	    aggregates, UNIONs or DISTINCT in the outermost subselect.  After
**	    that, psy_qrymod() will be called to ascertain that the view is not
**	    abandoned.  In the process of unraveling the view tree, psy_view()
**	    will be responsible for determining whether the view is, indeed,
**	    updateable and/or grantable.  This information will be returned from
**	    psy_view() via psy_qrymod().  Indicator of view's grantability will
**	    be stored in the IIRELATION.relstat.  Indicator of updateability
**	    still needs to find a home.
**	11-sep-91 (andre)
**	    list of indepndent object ids for a given view may be a subset of
**	    ids of objects used in the view definition and psy_view() will be
**	    responsible for generating this list.  For example if V was created
**	    by
**		create view andre.V as select * from andre.T, eric.QUELVIEW,
**		    greg.V
**	    list of indepndent object ids for andre.V will consist of andre.T
**	    and eric.QUELVIEW.  andre.V will also depend on GRANT-compatible
**	    SELECT perivilege on greg.V, but this information will be produced
**	    by dopro() (and I haven't started making the changes yet)
**	12-sep-91 (andre)
**	    delay closing the memory stream used in psy_qrymod() until the
**	    cleanup time since elements of sess_cb->pss_indep_objs would be
**	    allocated from this stream.
**
**	    (RE: 07-aug-91 (andre)) we will not try to determine if the newly
**	    created view is "updateable", to a large degree because the criteria
**	    for deciding if the view is updateable is likely to change
**	    (multi-variable views may be treated as updateable) and storing an
**	    indicator of updateability in some catalog will result in a large
**	    conversion effort later.
**	11-nov-91 (rblumer)
**	  merged from 6.4:  26-feb-91 (andre)
**	    PST_QTREE was changed to store the range table as an array of
**	    pointers to PST_RNGENTRY structure.
**	17-jan-92 (andre)
**	    when recreating a range table before calling psy_qrymod() we can
**	    take advantage of two facts:
**		- tables in the header range table are stored in the order of
**		  their range number
**		- since psy_view() has not been invoked on the view's tree,
**		  there will be no empty slots in the header range table
**	    As a result, we do not need to scan the view tree looking for
**	    relations used in it, nor do we need to remap them once tree copy
**	    has been made.
**
**	    Previously I changed code to pass
**	    sess_cb->pss_indep_objs.psq_objs->psq_objid to QEF in place of a
**	    list of ids of objects which appeared in the view's definition.
**	    This was incorrect - instead, we will build a list of ids
**	    of objects used in the view definition and pass it AND a pointer to
**	    sess_cb->pss_indep_objs to QEF.
**	19-may-92 (andre)
**	    once ps_ostream has been successfully opened, make
**	    pss_dependencies_stream point at it.  After closing pss_ostream, set
**	    pss_dependencies_stream to NULL to avoid referencing a closed stream
**	29-may-92 (andre)
**	    before calling rdf_update(), set sess_cb->psq_objs.psq_grantee to
**	    &sess_cb->pss_user
**	15-june-92 (barbara)
**	    Sybil merge.  For distributed thread, omit check that view is
**	    usable.  Star cannot detect when a local action renders a
**	    distributed view unusable, so don't check initially.  Save array
**	    of distributed object types and calculate row width for RDF.
**	    Also, change interface to pst_rgent and set rdr_r1_distrib.
**	03-aug-92 (barbara)
**	    For the objects on which the view is based, tell RDF to
**	    invalidate their cached descriptions.  Initalize RDF_CB by
**	    call to pst_rdfcb_init before calling rdf_call.
**	28-aug-92 (andre)
**	    when we create a local view which depends on some objects and/or
**	    privileges, this information is filled in in the course of running
**	    psy_qrymod().  However, in STAR we do not invoke psy_qrymod() (since
**	    one cannot grant privileges in STAR.  Therefore if we are creatring
**	    a STAR view, we will populate the independent object list with ids
**	    of tables/views which were used in the view's definition
**	20-nov-92 (andre)
**	    WITH CHECK OPTION specified for local (non-STAR) SQL views ONLY will
**	    be enforced using statement-level rules and set-input procedures
**	    (courtesy of the constraints project.) 
**	    Before creating the rules and the dbproc to enforce CHECK OPTION, we
**	    will verify that CHECK OPTION may be specified for the view being
**	    created + that it could be violated by UPDATE/INSERT
**	04-dec-1992 (pholman)
**	    C2: replace obsolete qeu_audit with psy_secaudit.
**	29-dec-92 (andre)
**	    if a user specified WITH CHECK OPTION, and we determine that
**	    CHECK OPTION should not be specified for this view because it is not
**	    updatable or its underlying base table is also involved in the
**	    subquery inside its qualification, we will issue a warning message
**	    and proceed to create a view WITHOUT CHECK OPTION
**	30-dec-92 (andre)
**	    if during parsing of a view definition it was determined that the
**	    view involves an aggregate and/or GROUP BY outside of a view's
**	    qualification, set PST_AGG_IN_OUTERMOST_SUBSEL and/or
**	    PST_GROUP_BY_IN_OUTERMOST_SUBSEL in PST_QTREE.pst_mask1.  RDF will
**	    store them in the catalog which will save us the expense of
**	    rediscovering this info every time we come across a given view
**	31-dec-92 (andre)
**	    if during parsing of a view definition we encountered singleton
**	    subselect(s), set PST_SINGLETON_SUBSEL in PST_QTREE.pst_mask1
**	04-feb-93 (andre)
**	    do not check stat unless rdf_call() returned bad status.
**	10-feb-93 (andre)
**	    for STAR views, pst_wchk was being used before it was set; move code
**	    initializing pst_wchk before the code where it gets used
**	20-feb-93 (rblumer)
**	    remove the distinction between QUEL and SQL views when determining
**	    defaultability; set calculated columns to have the UNKNOWN default.
**	08-apr-93 (ralph)
**	    DELIM_IDENT:  Use appropriate case for "tid"
**	17-jun-93 (andre)
**	    changed interface of psy_secaudit() to accept PSS_SESBLK
**	 5-jul-93 (robf)
**	    Pass security label to psy_secaudit
**	07-jul-93 (shailaja)
**	    Fixed prototype incompatibility warnings.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	09-sep-93 (teresa)
**	    Fix Star bug 54383 (decimal data types)
**	15-nov-93 (andre)
**	    PSS_SINGLETON_SUBSELECT got moved from pss_stmt_flags to
**	    pss_flattening_flags
**	17-nov-93 (andre)
**	    if PSS_SUBSEL_IN_OR_TREE/PSS_ALL_IN_TREE/PSS_MULT_CORR_ATTRS/
**	    PSS_CORR_AGGR bits are set in sess_cb->pss_flattening_flags,
**	    set PST_SUBSEL_IN_OR_TREE/PST_ALL_IN_TREE/PST_MULT_CORR_ATTRS/
**	    PST_CORR_AGGR bits in qtree->pst_mask1
**	18-nov-93 (andre)
**	    moved code mentioned in my comment from 11-17 to pst_header()
**	15-dec-04 (inkdo01)
**	    Add support of attcollid column.
**      13-Jun-08 (coomi01) Bug 110708
**          On detecting a sql_check_option error, record the condition
**          in the pst_flags
**      14-Jul-08 (coomi01) Bug 110708
**          On detecting a sql_check_option error, record the condition
**          in the pst_flags, other cases discovered!
**      26-Oct-2009 (coomi01) b122714
**          Use PST_TEXT_STORAGE to allow for long (>64k) view text.
**      16-Feb-2010 (hanal04) Bug 123292
**          Initialise attr_seqTuple to avoid spurious values being
**          picked up.
**	22-Mar-2010 (toumi01) SIR 122403
**	    Add encflags and encwid for encryption.
**	25-Mar-2010 (kiria01) b123535
**	    Added call to psl_ss_flatten() to apply the flattening of
**	    subselects in views too.
**	11-Jun-2010 (kiria01) b123908
**	    Initialise pointers after psf_mopen would have invalidated any
**	    prior content.
*/
DB_STATUS
psl_cv1_create_view(
	PSS_SESBLK	    *sess_cb,
	PSQ_CB		    *psq_cb,
	PST_QNODE	    *new_col_list,
	PST_QNODE	    *query_expr,
	i4		    check_option,
	PSS_YYVARS	    *yyvarsp)
{
    PSS_USRRANGE	*rngtab = &sess_cb->pss_auxrng;
    DB_STATUS		status, stat;
    register i4	i;
    bool		grantable;
    DB_TAB_ID           *tabids = (DB_TAB_ID *) NULL;
    i4			*tabtypes = (i4 *) NULL;
    DD_OBJ_TYPE         *obj_types = (DD_OBJ_TYPE *) NULL;
    bool		wco_insert_rule = FALSE;
    bool		wco_update_rule = FALSE;
    PST_STATEMENT	*snode = (PST_STATEMENT *) sess_cb->pss_object;
    PST_CREATE_VIEW	*crt_view = &snode->pst_specific.pst_create_view;
    PST_QTREE		*qtree;
    QEUQ_CB		*qeuq_cb = (QEUQ_CB *) crt_view->pst_qeuqcb;
    DMU_CB		*dmu_cb = (DMU_CB *) qeuq_cb->qeuq_dmf_cb;
    DMF_ATTR_ENTRY	**attrs;
    DMF_ATTR_ENTRY	*attr, **attr_pp;
    PST_QNODE		*resdom;
    PSF_MSTREAM		tempstream;
    i4			colno;
    i4			ntype, sstype;
    PST_QNODE		*node, *ssnode;
    DMT_ATT_ENTRY       *attribute;
    i4             err_code;
    PSS_RNGTAB		*rngtabp;
    char		tid[DB_ATT_MAXNAME];
    char		*name;
    PST_PROCEDURE	*pnode;
    bool		sqlview = sess_cb->pss_lang == DB_SQL;
    bool		sql_check_option = (sqlview && check_option);
    bool		close_stream = FALSE;

    /*
    ** if the view has been determined to be non-updatable and WITH CHECK
    ** OPTION was specified, flag it as an error
    */
    if (sql_check_option && yyvarsp->nonupdt)
    {
	i4	err_no;
	
	/*
	** during parsing of CREATE VIEW statement, we have determined that
	** the view will not be updateable (yyvarsp->nonupdt_reason describes
	** reason(s)) - consequently, WITH CHECK OPTION may not be specified
	** for this view
	*/

	if (yyvarsp->nonupdt_reason & PSS_UNION_IN_OUTERMOST_SUBSEL)
	    err_no = W_PS0580_WCO_AND_UNION;
	else if (yyvarsp->nonupdt_reason & PSS_DISTINCT_IN_OUTERMOST_SUBSEL)
	    err_no = W_PS0581_WCO_AND_DISTINCT;
	else if (yyvarsp->nonupdt_reason & PSS_MULT_VAR_IN_OUTERMOST_SUBSEL)
	    err_no = W_PS0582_WCO_AND_MULT_VARS;
	else if (yyvarsp->nonupdt_reason & PSS_GROUP_BY_IN_OUTERMOST_SUBSEL)
	    err_no = W_PS0583_WCO_AND_GROUP_BY;
	else if (yyvarsp->nonupdt_reason & PSS_HAVING_IN_OUTERMOST_SUBSEL)
	    err_no = W_PS0584_WCO_AND_HAVING;
	else if (yyvarsp->nonupdt_reason & PSS_SET_FUNC_IN_OUTERMOST_SUBSEL)
	    err_no = W_PS0585_WCO_AND_SET_FUNCS;
	else
	    err_no = W_PS058F_WCO_AND_UNSPEC_REASON;

	/* 
	** Take note of this reaction
	*/
	crt_view->pst_flags |= PST_SUPPRESS_CHECK_OPT;

	(VOID) psf_error(err_no, 0L, PSF_USERERR, &err_code,
	    &psq_cb->psq_error, 1,
	    psf_trmwhite(sizeof(crt_view->pst_view_name),
		(char *) &crt_view->pst_view_name),
	    (PTR) &crt_view->pst_view_name);

	/* view will be created WITHOUT CHECK OPTION */
	check_option = sql_check_option = FALSE;
    }

    /*
    ** if creating an SQL view and WITH CHECK OPTION was specified and the view
    ** involved a qualification, verify that at least one of the view's
    ** attributes appears to be updatable and that the view's simply underlying
    ** table does not appear in any of the FROM-lists in the view's
    ** qualification
    */
    if (sql_check_option)
    {
	bool	    updatable_attr;
    
	/*
	** Make sure that at least one attribute of the new view appears
	** updatable (some of the attributes that look updatable now may turn
	** out to be non-updatable after we pump the view's tree through
	** qrymod, but if none of the attributes appear updatable now, the view
	** is most certainly non-updatable)
	*/
	for (resdom = query_expr->pst_left, updatable_attr = FALSE;
	     (   resdom != (PST_QNODE *) NULL
	      && resdom->pst_sym.pst_type == PST_RESDOM);
	     resdom = resdom->pst_left)
	{
	    if (resdom->pst_right->pst_sym.pst_type == PST_VAR)
	    {	/* not an expression */
		updatable_attr = TRUE;
		break;
	    }
	}

	if (!updatable_attr)
	{
	    /* 
	    ** Take note of this reaction
	    */
	    crt_view->pst_flags |= PST_SUPPRESS_CHECK_OPT;

	    /*
	    ** none of the attributes of the view appear updatable - view cannot
	    ** possibly be updatable
	    */
	    (VOID) psf_error(W_PS058D_WCO_AND_NO_UPDATABLE_COLS, 0L,
		PSF_USERERR, &err_code, &psq_cb->psq_error, 1,
		psf_trmwhite(sizeof(crt_view->pst_view_name),
		    (char *) &crt_view->pst_view_name),
		(PTR) &crt_view->pst_view_name);

	    /* view will be created WITHOUT CHECK OPTION */
	    check_option = sql_check_option = FALSE;
	}

	/*
	** if query expression involved a qualification and that qualification
	** involved subqueries, verify that none of them contained a FROM-list
	** containing the table on which the view is being defined
	*/
	else if (   query_expr->pst_right
		 && query_expr->pst_right->pst_sym.pst_type != PST_QLEND
		 && sess_cb->pss_auxrng.pss_maxrng > 0
		)
	{
	    PSS_RNGTAB		*r;

	    for (i = 0, r = rngtab->pss_rngtab; i < PST_NUMVARS; i++, r++)
	    {
		if (   !r->pss_used
		    || r->pss_rgno < 0
		    || r == yyvarsp->underlying_rel)
		{
		    continue;
		}

		if (   yyvarsp->underlying_rel->pss_tabid.db_tab_base ==
			   r->pss_tabid.db_tab_base
		    && yyvarsp->underlying_rel->pss_tabid.db_tab_index ==
			   r->pss_tabid.db_tab_index
		   )
		{
		    /*
		    ** view's underlying table also appears in a FROM-list in
		    ** the qualification of the view - WITH CHECK OPTION may not
		    ** be specified for such view
		    */

	            /* 
	            ** Take note of this reaction
	            */
	            crt_view->pst_flags |= PST_SUPPRESS_CHECK_OPT;

		    (VOID) psf_error(W_PS058C_WCO_AND_LUT_IN_WHERE_CLAUSE, 0L,
			PSF_USERERR, &err_code, &psq_cb->psq_error, 1,
			psf_trmwhite(sizeof(crt_view->pst_view_name),
			    (char *) &crt_view->pst_view_name),
			(PTR) &crt_view->pst_view_name);

		    /* view will be created WITHOUT CHECK OPTION */
		    check_option = sql_check_option = FALSE;
		    break;
		}
	    }
	}
    }

    if (sqlview &&
	    (status = psl_ss_flatten(&query_expr, yyvarsp,
					sess_cb, psq_cb)))
	return(status);
   /*
    ** Update RESDOMs in subselect with info from 'newcolspec'.
    **
    ** Note: for QUEL views, new_col_list will always be NULL
    */
    if (new_col_list != (PST_QNODE *) NULL)
    {
	for (node = new_col_list, ssnode = query_expr->pst_left;
	     ;
	     node = node->pst_left, ssnode = ssnode->pst_left
	    )
	{
	    ntype  = node->pst_sym.pst_type;
	    sstype = ssnode->pst_sym.pst_type;

	    if ((ntype == PST_TREE) && (sstype == PST_TREE))
	    {
		/* equal # of nodes */
		break;
	    }
	    else if ((ntype == PST_RESDOM) && (sstype == PST_RESDOM))
	    {
		/* good pair of nodes; process them */
	    }
	    else
	    {	/* not equal # of nodes */
		(VOID) psf_error(2051L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 0);
		
		status = E_DB_ERROR;
		goto cleanup;
	    }

	    /* copy domain name */
	    MEcopy((char *) node->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
		sizeof(DB_ATT_NAME),
		(char *) ssnode->pst_sym.pst_value.pst_s_rsdm.pst_rsname);
	}
    }

    /*
    ** If creating an SQL view, we have to count resdoms again because
    ** sess_cb->pss_rsdmno might have been altered by any subselect in the
    ** qualification + convert the root node of the first subselect into the
    ** root node for the query.
    */
    if (sqlview)
    {
	for (ssnode = query_expr->pst_left, sess_cb->pss_rsdmno = 0;
	     ssnode->pst_sym.pst_type != PST_TREE;
	     ssnode = ssnode->pst_left, sess_cb->pss_rsdmno++
	    )
	;

	query_expr->pst_sym.pst_value.pst_s_root.pst_rtuser = TRUE;
    }

    /*
    ** Make the query tree header - do NOT generate
    ** PST_PROCEDURE/PST_STATEMENT nodes for the tree that will be
    ** stored in IITREE
    */
    status = pst_header(sess_cb, psq_cb, PST_UNSPECIFIED,
	    (PST_QNODE *) NULL, query_expr, &crt_view->pst_view_tree,
	    &pnode, 0, (sqlview) ? &yyvarsp->xlated_qry : (PSS_Q_XLATE *) NULL);
    if (status != E_DB_OK)
	goto cleanup;

    qtree = crt_view->pst_view_tree;
    
    /*
    ** for SQL view set pst_numjoins - yyvarsp->join_id contains the highest
    ** join id; for QUEL view do nothing since pst_numjoins was set to
    ** PST_NOJOIN in pst_header()
    */
    if (sqlview)
    {
	qtree->pst_numjoins = yyvarsp->join_id;
    }

    /*
    ** if a view was found to be non-updatable, we need to remember whether it
    ** was caused by presence of an AGGREGATE or GROUP BY outside of view's
    ** qualification
    */
    if (yyvarsp->nonupdt)
    {
	if (yyvarsp->nonupdt_reason & PSS_SET_FUNC_IN_OUTERMOST_SUBSEL)
	    qtree->pst_mask1 |= PST_AGG_IN_OUTERMOST_SUBSEL;

	if (yyvarsp->nonupdt_reason & PSS_GROUP_BY_IN_OUTERMOST_SUBSEL)
	    qtree->pst_mask1 |= PST_GROUP_BY_IN_OUTERMOST_SUBSEL;
    }

    if (!yyvarsp->isdbp)	/* will be FALSE if/when we add support for
				** CREATE VIEW inside a dbproc */
    {
	/*
	** allocate procedure node for CREATE VIEW statement
	*/

	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PST_PROCEDURE),
	    (PTR *) &pnode, &psq_cb->psq_error);

	if (DB_FAILURE_MACRO(status))
	    goto cleanup;

	MEfill(sizeof(PST_PROCEDURE), (u_char) 0, (PTR) pnode);
	
	pnode->pst_mode = (i2) 0;
	pnode->pst_vsn = (i2) PST_CUR_VSN;
	pnode->pst_isdbp = FALSE;
	pnode->pst_stmts = snode;
	pnode->pst_parms = (PST_DECVAR *) NULL;

	status = psf_mroot(sess_cb, &sess_cb->pss_ostream, (PTR) pnode,
	    &psq_cb->psq_error);
	if (status != E_DB_OK)
	    goto cleanup;
    }

    /*
    ** Allocate the space for the DMU_CB column descriptions.
    */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
	sess_cb->pss_rsdmno * sizeof(DMF_ATTR_ENTRY *),
	(PTR *) &dmu_cb->dmu_attr_array.ptr_address, &psq_cb->psq_error);
    if (status != E_DB_OK)
	goto cleanup;

    attrs = (DMF_ATTR_ENTRY **) dmu_cb->dmu_attr_array.ptr_address;

    dmu_cb->dmu_attr_array.ptr_in_count = sess_cb->pss_rsdmno;
    dmu_cb->dmu_attr_array.ptr_size = sizeof(DMF_ATTR_ENTRY);

    /*
    ** Run down the RESDOMS, and put the column names and formats into
    ** the attribute list for the DMU_CB.
    **
    ** While at it, make sure that none of view's attributes is named "tid" and
    ** that the same name has not been assigned to to any two attributes
    */

    /* Normalize the attribute name */
    STmove(((*sess_cb->pss_dbxlate & CUI_ID_REG_U) ? "TID" : "tid"),
	   ' ', DB_ATT_MAXNAME, tid);

    for (resdom = query_expr->pst_left;
	 (   resdom != (PST_QNODE *) NULL
	  && resdom->pst_sym.pst_type == PST_RESDOM);
	 resdom = resdom->pst_left)
    {
	/*
	** resdom numbers start counting from 1, entries in dmu_attr_array -
	** from 0
	*/
	colno = resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno - 1;

	name = resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsname;

	/* Is the name equal to "tid" */
	if (!MEcmp(name, tid, DB_ATT_MAXNAME))
	{
	    (VOID) psf_error(2052L, 0L, PSF_USERERR, &err_code,
		&psq_cb->psq_error, 1,
		psf_trmwhite(DB_ATT_MAXNAME, name), name);
	    
	    status = E_DB_ERROR;
	    goto cleanup;
	}

	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DMF_ATTR_ENTRY),
	    (PTR *) &attrs[colno], &psq_cb->psq_error);
	if (status != E_DB_OK)
	    goto cleanup;

	attr = attrs[colno];
	/* Put in attribute name */
	MEcopy((PTR) name, DB_ATT_MAXNAME, (PTR) attr->attr_name.db_att_name);

	/* check for duplicate attribute names */
	for (i = colno + 1, attr_pp = attrs + i;
	     i < sess_cb->pss_rsdmno;
	     i++, attr_pp++)
	{
	    if (!MEcmp((PTR) name, (PTR) &(*attr_pp)->attr_name, DB_ATT_MAXNAME))
	    {
		if (sqlview)
		{
		    (VOID) psf_error(2013L, 0L, PSF_USERERR, &err_code,
			&psq_cb->psq_error, 1,
			psf_trmwhite(DB_ATT_MAXNAME, name), name);
		}
		else
		{
		    (VOID) psf_error(5105L, 0L, PSF_USERERR, &err_code,
			&psq_cb->psq_error, 2,
			sizeof(dmu_cb->dmu_table_name),
			(char *) &dmu_cb->dmu_table_name,
			psf_trmwhite(DB_ATT_MAXNAME, name), name);
		}

		status = E_DB_ERROR;
		goto cleanup;
	    }
	}
		
	/* Put in attribute type (takes care of nullability) */
	attr->attr_type = resdom->pst_sym.pst_dataval.db_datatype;
	/* Put in attribute length */
	attr->attr_size = resdom->pst_sym.pst_dataval.db_length;
	/* Put in attribute precision */
	attr->attr_precision = resdom->pst_sym.pst_dataval.db_prec;
	attr->attr_collID = resdom->pst_sym.pst_dataval.db_collID;
        attr->attr_seqTuple = (DB_IISEQUENCE *) NULL;

	/*
	 * Put in geospatial attributes
	 */
	attr->attr_geomtype = -1;
	attr->attr_srid = -1;

	attr->attr_encflags = 0;
	attr->attr_encwid = 0;

	/*
	** Set up defaultibility.
	**
	** If the view column is based directly on a column (i.e. is a PST_VAR),
	** we copy that column's default value;
	** otherwise the view column must be based on a calculated column or
	** expression, so set its default value to UNKNOWN. 
	*/
	if (resdom->pst_right->pst_sym.pst_type != PST_VAR)
	{
	    /* view column based on expression - consider non-updatable */
	    attr->attr_flags_mask = 0;
	    SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_UNKNOWN);
	    attr->attr_defaultTuple  = (DB_IIDEFAULT *) NULL;
	}
	else	/* column of view is based on a column of underlying table */
	{
	    /* find range table entry */
	    status = pst_rgfind(
			sqlview ? &sess_cb->pss_auxrng : &sess_cb->pss_usrrange,
			&rngtabp,
			resdom->pst_right->pst_sym.pst_value.pst_s_var.pst_vno,
			&psq_cb->psq_error);
	    if (status != E_DB_OK)
	    {
		(VOID) psf_error(E_PS0909_NO_RANGE_ENTRY, 0L, PSF_INTERR,
		    &err_code, &psq_cb->psq_error, 1,
		    psf_trmwhite(DB_ATT_MAXNAME, name), name);
		goto cleanup;
	    }

	    /* Look up the attribute */
	    attribute = pst_coldesc(rngtabp,
		&resdom->pst_right->pst_sym.pst_value.pst_s_var.pst_atname);

	    /* Check for attribute not found */
	    if (attribute == (DMT_ATT_ENTRY *) NULL)
	    {
		(VOID) psf_error(E_PS090A_NO_ATTR_ENTRY, 0L, PSF_INTERR,
		    &err_code, &psq_cb->psq_error, 1,
		    psf_trmwhite(DB_ATT_MAXNAME, name), name);
		goto cleanup;
	    }

	    attr->attr_flags_mask = attribute->att_flags;
	    attr->attr_defaultID  = attribute->att_defaultID;
	    attr->attr_defaultTuple  = (DB_IIDEFAULT *) NULL;
	}
    }

    /*
    ** allocate an array of DB_TAB_IDs for ids of tables used in the view
    ** definition;
    ** NOTE: we may not allocate it on stack because the statement could be
    ** being prepared.
    */

    if (qtree->pst_rngvar_count > 0)
    {
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
	    qtree->pst_rngvar_count * (sizeof(DB_TAB_ID)+sizeof(i4)),
	    (PTR *) &tabids, &psq_cb->psq_error);
	if (status != E_DB_OK)
	    goto cleanup;
	tabtypes = (i4 *)&tabids[qtree->pst_rngvar_count];
    }
    
    /* 
    ** We need to check whether the view is usable (i.e. whether the user would
    ** be able to select data from the view).  This is accomplished by checking
    ** if the user is allowed to execute the SELECT ... part of the view
    ** defintion.  Note that we will not perform a full QRYMOD algorithm in that
    ** once it is known that we are allowed to retrieve data from a given
    ** view, we will not try expand it EXCEPT for the case when we need to
    ** verify that WITH CHECK OPTION may be specified for a given SQL view.
    ** In that case we will continue unravelling the view trees as we would for
    ** a regular SELECT.  This is required so that we can ascertain that a view
    ** is updatable + that its leaf underlying base table is not involved in any
    ** of the subqueries in the view's qualification.
    ** 
    ** We need to run QRYMOD algorithm over the view tree (actually, we will run
    ** it over the copy of the tree since we do not want to modify the tree
    ** built in QUEL or SQL grammar.)  
    **
    ** Don't bother checking when creating a distributed view since in STAR
    ** PUBLIC has ALL by default and no permits can be granted anyway
    */

    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
	if (qtree->pst_rngvar_count > 0)
	{
	    PSQ_OBJ	    *objlist;
	    
	    /*
	    ** in local case, the list of objects on which a view depends is not
	    ** necessarily the same as the list of objects used in its
	    ** definition; for STAR they will be the same.  Here we will
	    ** allocate a PSQ_OBJ structure which will contain ids of all
	    ** tables/views used in the view's definition
	    */
	    
	    /*
	    ** allocate a PSQ_OBJ structure big enough to hold ids of
	    ** tables/views used in the new view's definition and make
	    ** sess_cb->pss_indep_objs.psq_objs point at it
	    */
	    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
		(i4) (sizeof(PSQ_OBJ) +
		       sizeof(DB_TAB_ID) * qtree->pst_rngvar_count -
		       sizeof(DB_TAB_ID) - sizeof(DB_DBP_NAME)),
		(PTR *) &sess_cb->pss_indep_objs.psq_objs, &psq_cb->psq_error);

	    if (DB_FAILURE_MACRO(status))
	    {
		goto cleanup;
	    }

	    objlist = sess_cb->pss_indep_objs.psq_objs;

	    /* this will be the only element in the list */
	    objlist->psq_next = (PSQ_OBJ *) NULL;

	    /* populate the new entry */
	    objlist->psq_objtype = PSQ_OBJTYPE_IS_TABLE;
	    objlist->psq_num_ids = qtree->pst_rngvar_count;

	    /*
	    ** allocate array of object types
	    */
	    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
		qtree->pst_rngvar_count * sizeof(DD_OBJ_TYPE),
		(PTR *) &obj_types, &psq_cb->psq_error);
	    if (status != E_DB_OK)
		goto cleanup;

	    /*
	    ** for STAR view, we simply enter information about tables/views
	    ** which were used in the view's definition into an array of object
	    ** ids (whose relstat will be changed to indicate that there are
	    ** view(s) defined on top of them and object types.  In addition we
	    ** will construct a list of view's independent objects which will be
	    ** the same as the list of ob objects used in the view's definition
	    */
	    for (i = 0; i < qtree->pst_rngvar_count; i++)
	    {
		objlist->psq_objid[i].db_tab_base = tabids[i].db_tab_base =
		    qtree->pst_rangetab[i]->pst_rngvar.db_tab_base;
		objlist->psq_objid[i].db_tab_index = tabids[i].db_tab_index =
		    qtree->pst_rangetab[i]->pst_rngvar.db_tab_index;

		obj_types[i] = qtree->pst_rangetab[i]->pst_dd_obj_type;
	    }
	}
    }
    else
    {
	if (qtree->pst_rngvar_count == 0)
	{
	    /*
	    ** view definition involves no relation, no reason to invoke qrymod
	    */
	    grantable = TRUE;
	}
	else
	{
	    PST_QNODE	    *tree_copy;
	    PSS_DUPRB	    dup_rb;
	    PST_J_ID	    dummy;
	    PST_QNODE	    *tlist_start = (PST_QNODE *) NULL;
	    i4	    qrymod_resp_mask;
	    bool	    leave_loop;

	    /*
	    ** we are going to copy a view tree and pump it through qrymod - but
	    ** once qrymod is done, we no longer need the tree.  We will
	    ** temporarily save pss_ostream in pss_cbstream and reopen
	    ** pss_ostream for use by qrymod processing (this is made possible
	    ** by the fact that only pss_ostream is being used to allocate
	    ** memory for CREATE VIEW processing)
	    ** 
	    ** NOTE: sess_cb->pss_dependencies_stream MUST point at the stream
	    **	     that will be handed back to SCF, so we will set it to
	    **	     address of pss_cbstream once we copy the pss_ostream into
	    **	     it. 
	    ** ANOTHER NOTE: the only reason we are copying pss_stream into
	    **		     pss_cbstream instead of a temporary var allocated
	    **		     on stack is that if an error (or an exception)
	    **		     occurs, streams described by headers in sess_cb
	    **		     will be automatically closed
	    */
	    STRUCT_ASSIGN_MACRO(sess_cb->pss_ostream, sess_cb->pss_cbstream);
	    sess_cb->pss_dependencies_stream = &sess_cb->pss_cbstream;

	    sess_cb->pss_ostream.psf_mstream.qso_handle = (PTR) NULL;
	    
	    status = psf_mopen(sess_cb, QSO_QTEXT_OBJ, &sess_cb->pss_ostream,
		&psq_cb->psq_error);
	    if (DB_FAILURE_MACRO(status))
		goto cleanup;
	    sess_cb->pss_stk_freelist = NULL;

	    for (i = 0; i < qtree->pst_rngvar_count; i++)
	    {
		/*
		** insert object id into a list of tables and views used in the
		** definition of the new view to be passed to QEF
		*/
		tabids[i].db_tab_base =
		    qtree->pst_rangetab[i]->pst_rngvar.db_tab_base;
		tabids[i].db_tab_index =
		    qtree->pst_rangetab[i]->pst_rngvar.db_tab_index;
		tabtypes[i] = qtree->pst_rangetab[i]->pst_rgtype;
	    }

	    /* initialize fields in dup_rb */
	    dup_rb.pss_op_mask = 0;
	    dup_rb.pss_num_joins = PST_NOJOIN;
	    dup_rb.pss_tree_info = (i4 *) NULL;
	    dup_rb.pss_mstream = &sess_cb->pss_ostream;
	    dup_rb.pss_err_blk = &psq_cb->psq_error;
	    dup_rb.pss_tree = qtree->pst_qtree;
	    dup_rb.pss_dup = &tree_copy;
	    
	    status = pst_treedup(sess_cb, &dup_rb);

	    if (DB_FAILURE_MACRO(status))
	    {
		goto cleanup;
	    }

	    /*
	    ** if WITH CHECK OPTION was specified for an SQL view, we want to
	    ** tell psy_view() that we are verifying that WCO may be specified
	    ** so that if a view appears nonupdatable, it would notify us of the
	    ** reason and return error
	    */
	    if (sql_check_option)
	    {
		/*
		** remember that rule(s) may be defined on a view defined on top
		** of this object
		*/
		yyvarsp->underlying_rel->pss_var_mask |= PSS_RULE_RNG_VAR;

		/*
		** tell psy_qrymod() that we will be validating WITH CHECK
		** OPTION specified in the view definition in the course of
		** verifying that a new view is not abandoned
		*/
		sess_cb->pss_stmt_flags |= PSS_VALIDATING_CHECK_OPTION;
	    }

	    /*
	    ** we are not using the updated join number provided by psy_qrymod()
	    ** (at least not yet), but it doesn't hurt us to pass the real value
	    ** down
	    */
	    dummy = qtree->pst_numjoins;

	    /* Apply QRYMOD processing */
	    status = psy_qrymod(tree_copy, sess_cb, psq_cb, &dummy,
		&qrymod_resp_mask);

	    if (DB_FAILURE_MACRO(status))
	    {
		/* Must audit failure to create view. */
		if ( Psf_srvblk->psf_capabilities & PSF_C_C2SECURE )
		{
		    DB_ERROR    e_error;
		    DB_STATUS   local_status;

		    local_status = psy_secaudit(FALSE, sess_cb,
			    (char *) &crt_view->pst_view_name,
			    &sess_cb->pss_user,
			    sizeof(DB_TAB_NAME), SXF_E_VIEW,
			    I_SX2014_VIEW_CREATE, SXF_A_FAIL | SXF_A_CREATE,
			    &e_error);

		    if (DB_FAILURE_MACRO(local_status) && local_status > status)
		    {
			status = local_status;     /* remember the highest status */
		    }
		}
		
		goto cleanup;
	    }


	    /*
	    ** psy_qrymod() may have detected that a view which we are
	    ** trying to create WITH CHECK OPTION will not be updatable or
	    ** its leaf underlying base table will be also involved in the
	    ** view's qualification; since psy_qrymod() does not "know" the
	    ** name of the view being created, it communicates this info to
	    ** us rather than reporting an error
	    */
	    if (sql_check_option && qrymod_resp_mask)
	    {
		i4         err_no = 0L;

		if (qrymod_resp_mask & PSY_UNION_IN_UNDERLYING_VIEW)
		    err_no = W_PS0586_WCO_AND_UNION_IN_UV;
		else if (qrymod_resp_mask & PSY_DISTINCT_IN_UNDERLYING_VIEW)
		    err_no = W_PS0587_WCO_AND_DISTINCT_IN_UV;
		else if (qrymod_resp_mask & PSY_MULT_VAR_IN_UNDERLYING_VIEW)
		    err_no = W_PS0588_WCO_AND_MULT_VARS_IN_UV;
		else if (qrymod_resp_mask & PSY_GROUP_BY_IN_UNDERLYING_VIEW)
		    err_no = W_PS0589_WCO_AND_GROUP_BY_IN_UV;
		else if (qrymod_resp_mask & PSY_SET_FUNC_IN_UNDERLYING_VIEW)
		    err_no = W_PS058B_WCO_AND_SET_FUNCS_IN_UV;
		else if (qrymod_resp_mask &PSY_NO_UPDT_COLS_IN_UNDRLNG_VIEW)
		    err_no = W_PS058D_WCO_AND_NO_UPDATABLE_COLS;

		if (err_no)
		{
		    /* 
		    ** Take note of this reaction
		    */
		    crt_view->pst_flags |= PST_SUPPRESS_CHECK_OPT;

		    (VOID) psf_error(err_no, 0L, PSF_USERERR, &err_code,
			&psq_cb->psq_error, 1, 
			psf_trmwhite(sizeof(crt_view->pst_view_name),
			    (char *) &crt_view->pst_view_name),
			(PTR) &crt_view->pst_view_name);

		    /* view will be created WITHOUT CHECK OPTION */
		    check_option = sql_check_option = FALSE;
		}
	    }
	    /*
	    ** swap pss_ostream and pss_cbstream and remember to close the
	    ** memory stream that was opened for qrymod processing
	    */
	    STRUCT_ASSIGN_MACRO(sess_cb->pss_ostream, tempstream);
	    STRUCT_ASSIGN_MACRO(sess_cb->pss_cbstream,
				sess_cb->pss_ostream);
	    STRUCT_ASSIGN_MACRO(tempstream, sess_cb->pss_cbstream);
	    close_stream = TRUE;

	    /* remember if the view is grantable */
	    grantable = ((qrymod_resp_mask & PSY_VIEW_IS_GRANTABLE) != 0);
	    
	    /*
	    ** If WITH CHECK OPTION was specified, verify that it was legal;
	    ** if WCO was specified for a local SQL view, determine whether it
	    ** will need to be enforced and, if so, create PST_CREATE_RULEs and
	    ** PST_CREATE_PROCEDURE
	    */

	    /*
	    ** if creating an SQL view and WITH CHECK OPTION was specified and
	    ** the view involved a qualification, apply one more test - verify
	    ** that the view's simply underlying table does not appear in any of
	    ** the FROM-lists in the qualification
	    */

	    /* 
	    ** leave_loop is used to avoid compiler warnings - instead of
	    ** having break as the last statement in the loop, we reset 
	    ** leave_loop to TRUE which causes us to exit the loop
	    */
	    leave_loop = FALSE;

	    while (sql_check_option && !leave_loop) 
	    {
		bool		updatable_attr, wco_insrt_attrs, wco_updt_attrs;
		bool		wco_u_table_wide;
		PSS_RNGTAB	*rule_rng_var = (PSS_RNGTAB *) NULL, *r;
		PSY_ATTMAP	insrt_map;
		i4		i;
	    
		/*
		** make sure that the underlying table of the new view does NOT
		** also appear in a subquery inside the qualification;
		** first we scan the range table looking for an entry with
		** PSS_RULE_RNG_VAR in pss_var_mask; then we scan it again
		** looking for the entry for the same table WITHOUT
		** PSS_RULE_RNG_VAR in pss_var_mask
		*/

		for (i = 0, r = sess_cb->pss_auxrng.pss_rngtab;
		     i < PST_NUMVARS;
		     i++, r++)
		{
		    if (!r->pss_used || r->pss_rgno < 0)
			continue;

		    if (r->pss_var_mask & PSS_RULE_RNG_VAR)
		    {
			rule_rng_var = r;
			break;
		    }
		}

		for (i = 0, r = sess_cb->pss_auxrng.pss_rngtab;
		     i < PST_NUMVARS;
		     i++, r++)
		{
		    if (   !r->pss_used
			|| r->pss_rgno < 0
			|| r->pss_var_mask & PSS_RULE_RNG_VAR)
		    {
			continue;
		    }

		    if (   rule_rng_var->pss_tabid.db_tab_base ==
			       r->pss_tabid.db_tab_base
			&& rule_rng_var->pss_tabid.db_tab_index ==
			       r->pss_tabid.db_tab_index
		       )
		    {
			(VOID) psf_error(W_PS058C_WCO_AND_LUT_IN_WHERE_CLAUSE,
			    0L, PSF_USERERR, &err_code, &psq_cb->psq_error, 1,
			    psf_trmwhite(sizeof(crt_view->pst_view_name),
				(char *) &crt_view->pst_view_name),
			    (PTR) &crt_view->pst_view_name);

			/* view will be created WITHOUT CHECK OPTION */
			check_option = sql_check_option = FALSE;
			break;
		    }
		}

		if (!sql_check_option)
		{
		    /*
		    ** sql_check_option got reset because the view may not be
		    ** created WITH CHECK OPTION - skip the rest of this code
		    ** block
		    */
		    break;
		}

		/*
		** compute a map of attributes of the view's underlying base
		** table that are used in the view's qualification;
		** 
		** if any of the attributes of the underlying base table appear
		** in the view's qualification, CHECK OPTION will have to be
		** enforced dynamically after INSERT into the view
		*/
		psy_fill_attmap(insrt_map.map, (i4) 0);
		psy_att_map(tree_copy->pst_right, rule_rng_var->pss_rgno,
		    (char *) insrt_map.map);

		/*
		** remember whether any of the underlying base table's
		** attributes are involved in the qualification of the view or
		** of any of its underlying views
		*/
		if (BTnext(0, (char *) insrt_map.map, DB_MAX_COLS + 1) > -1)
		{
		    /*
		    ** unless none of the view's attributes are updatable (which
		    ** will be treated as an error), CHECK OPTION will have to
		    ** be enforced dynamically after an INSERT into the view
		    */
		    wco_insrt_attrs = TRUE;
		}
		else
		{
		    wco_insrt_attrs = FALSE;
		}

		/*
		** Make sure that at least one attribute of the new view is
		** updatable; otherwise treat it as an error.
		** 
		** while at it, if the view's qualification involved any of the
		** attributes of the underlying base table, store a map of
		** view's updatable attributes which are based on the attributes
		** of the underlying table that are also used in the
		** qualification - whenever one or more of these attributes is
		** updated, CHECK OPTION will have to be enforced dynamically
		*/

		DB_COLUMN_BITMAP_INIT(crt_view->pst_updt_cols.db_domset, i, 0);

		wco_updt_attrs = FALSE;

		/*
		** assume that underlying column of every updatable column of
		** the view is involved in the view's qualification (this will
		** mean that the rule firing an enforcement dbproc after UPDATE
		** will not need to specify individual columns);
		** otherwise, if UPDATE of only some of view's updatable columns
		** has a potential to violate CHECK OPTION, rule will be crested
		** to fire enforcer dbproc after UPDATE of all such columns
		*/
		wco_u_table_wide = TRUE;
		
		for (resdom = tree_copy->pst_left, updatable_attr = FALSE;
		     (   resdom != (PST_QNODE *) NULL
		      && resdom->pst_sym.pst_type == PST_RESDOM);
		     resdom = resdom->pst_left)
		{
		    /* not an expression */
		    if (resdom->pst_right->pst_sym.pst_type == PST_VAR)
		    {
			PST_VAR_NODE	    *var;

			var = &resdom->pst_right->pst_sym.pst_value.pst_s_var;
			
			updatable_attr = TRUE;

			if (   wco_insrt_attrs
			    && BTtest((i4) var->pst_atno.db_att_id,
				      (char *) insrt_map.map)
			   )
			{
			    /*
			    ** remember that the map of attributes s.t. UPDATE
			    ** of any one of them will require that CHECK OPTION
			    ** to be enforced dynamically is non-empty
			    */
			    wco_updt_attrs = TRUE;

			    /*
			    ** add the attribute of the view to the map of
			    ** attributes UPDATE of which would require that
			    ** CHECK OPTION be enforced dynamically
			    */
			    BTset(resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno,
				(char *) crt_view->pst_updt_cols.db_domset);
			}
			else
			{
			    /*
			    ** if we have to create a rule responsible for
			    ** enforcement of CHECK OPTION after UPDATE of the
			    ** view, this rule will fire after UPDATE of any of
			    ** the columns represented by
			    ** crt_view->pst_updt_cols
			    */
			    wco_u_table_wide = FALSE;
			}
		    }
		}

		if (!updatable_attr)
		{
		    /*
		    ** none of the attributes of the view appear updatable -
		    ** view cannot possibly be updatable
		    */
		    (VOID) psf_error(W_PS058D_WCO_AND_NO_UPDATABLE_COLS, 0L,
			PSF_USERERR, &err_code, &psq_cb->psq_error, 1,
			psf_trmwhite(sizeof(crt_view->pst_view_name),
			    (char *) &crt_view->pst_view_name),
			(PTR) &crt_view->pst_view_name);

		    /*
		    ** view will be created WITHOUT CHECK OPTION - skip the
		    ** rest of the code block
		    */
		    check_option = sql_check_option = FALSE;
		    break;
		}

		/*
		** if CHECK OPTION after INSERT into the view will need to be
		** enforced dynamically, set PST_CHECK_OPT_DYNAMIC_INSRT in
		** crt_view->pst_flags
		*/
		if (wco_insrt_attrs)
		    crt_view->pst_flags |= PST_CHECK_OPT_DYNAMIC_INSRT;

		/*
		** if CHECK OPTION after UPDATE of the view's attributes
		** described by crt_view->pst_updt_cols will need to be
		** enforced dynamically, set PST_CHECK_OPT_DYNAMIC_UPDT in
		** crt_view->pst_flags
		*/
		if (wco_updt_attrs)
		{
		    crt_view->pst_flags |= PST_CHECK_OPT_DYNAMIC_UPDT;

		    /*
		    ** remember whether UPDATE of every one of the updatable
		    ** columns of the view has a potential to violate CHECK
		    ** OPTION;
		    ** otherwise those of the view's columns UPDATE of which
		    ** could result in violation of CHECK OPTION are represented
		    ** by crt_view->pst_updt_cols
		    */
		    if (wco_u_table_wide)
			crt_view->pst_flags |= PST_VERIFY_UPDATE_OF_ALL_COLS;
		}

		/* 
		** it's best if this statement is the last statement in the 
		** loop; it will cause the the while() loop to be exited
		*/
		leave_loop = TRUE;
	    }
	}
    }

    /* 
    ** Use psq_store_text, it is able to store more than 64k of text 
    */
    status = psq_store_text(sess_cb,
	(sess_cb->pss_lang == DB_SQL) ? (PSS_USRRANGE *) NULL
			              : &sess_cb->pss_usrrange,
			    sess_cb->pss_tchain, &sess_cb->pss_ostream,
			    (PTR *)&crt_view->pst_view_text_storage, (bool) FALSE,&psq_cb->psq_error);

    if (DB_FAILURE_MACRO(status))
	goto cleanup;

    /*
    ** Populate QEUQ_CB
    */

    /*
    ** list of objects used in the view's definition + count + (for STAR only),
    ** list of types of objects used in the view's definition
    */
    qeuq_cb->qeuq_tsz	    = qtree->pst_rngvar_count;
    qeuq_cb->qeuq_tbl_id    = tabids;
    qeuq_cb->qeuq_tbl_type  = tabtypes;

    /*
    ** if creating a local view or a constant STAR view, obj_types will be NULL
    */
    qeuq_cb->qeuq_obj_typ_p = obj_types;
    
    /*
    ** pass information about objects/privileges on which the new view depends
    ** to QEF
    */
    {
	PSQ_INDEP_OBJECTS	*indep_objs;
	
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
	    sizeof(sess_cb->pss_indep_objs), (PTR *) &indep_objs,
	    &psq_cb->psq_error);
	if (status != E_DB_OK)
	    goto cleanup;

	STRUCT_ASSIGN_MACRO(sess_cb->pss_indep_objs, (*indep_objs));

	status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
	    sizeof(*indep_objs->psq_grantee), (PTR *) &indep_objs->psq_grantee,
	    &psq_cb->psq_error);
	if (status != E_DB_OK)
	    goto cleanup;

	STRUCT_ASSIGN_MACRO(sess_cb->pss_user,  (*indep_objs->psq_grantee));
	
	qeuq_cb->qeuq_indep = (PTR) indep_objs;
    }

    /*
    ** populate the view status word
    */

    /*
    ** we had to wait until we were done verifyin g that a view may be
    ** specified WITH CHECK OPTION before setting pst_wchk in the query tree
    ** header
    */
    if (qtree->pst_wchk = check_option)
	crt_view->pst_status = DBQ_WCHECK;

    if (sess_cb->pss_lang == DB_SQL)
	crt_view->pst_status |= DBQ_SQL;

    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
	ADF_CB          *adf_scb = (ADF_CB *) sess_cb->pss_adfcb;
	DB_DATA_VALUE   dv;
	DB_DATA_VALUE   col_width;
	i4		row_width = 0;
	DB_DT_ID	datatype;	

	/*
	** in distributed mode, setup language type, with check option, and row
	** width
	*/
	qeuq_cb->qeuq_ddb_cb.qeu_1_lang = sess_cb->pss_lang;
	qeuq_cb->qeuq_ddb_cb.qeu_2_view_chk_b = (qtree->pst_wchk != 0);
	qeuq_cb->qeuq_ano = dmu_cb->dmu_attr_array.ptr_in_count;

	crt_view->pst_flags |= PST_STAR_VIEW;

	for (i = 0; i < qeuq_cb->qeuq_ano; i++)
	{
	    attr = attrs[i];

	    datatype = (attr->attr_type < 0) ?
			    (DB_DT_ID) -attr->attr_type:
			    (DB_DT_ID) attr->attr_type;
	    if (datatype == DB_DEC_TYPE)
	    {
		/* this is a decimal data type, so use a macro to get
		** length of data portion.  If this is nullable, then
		** increase length by 1 to hold null value.  Assure that
		** adjusted size and precision are put back into attr.
		*/
		col_width.db_length = attr->attr_size;
		col_width.db_prec = attr->attr_precision;

		dv.db_length= DB_PREC_TO_LEN_MACRO((i4)attr->attr_size);
		if (attr->attr_type < 0) 
		{
		    col_width.db_length++;
		    dv.db_length++;
		}
		dv.db_prec = DB_PS_ENCODE_MACRO(attr->attr_size,
					       attr->attr_precision);
		/* scaled length and precision */
		attr->attr_precision = dv.db_prec;	
		attr->attr_size = dv.db_length;
	    }
	    else
	    {
		/* Setup db data value & call ADF for column width conversion.
		** Assure that adjusted size is put back into attr 
		*/
		dv.db_datatype = attr->attr_type;
		dv.db_length   = attr->attr_size;
		dv.db_prec	   = 0;
		status = adc_lenchk(adf_scb, FALSE, &dv, &col_width);
		if (DB_FAILURE_MACRO(status))
		    goto cleanup;
		attr->attr_size = col_width.db_length;
	    }	    
	    row_width += col_width.db_length;
	}

	qeuq_cb->qeuq_ddb_cb.qeu_3_row_width = row_width;
    }
    else
    {
	/* Let QEF know if the view is owned by the DBA */
	if (!MEcmp(sess_cb->pss_dba.db_tab_own.db_own_name, 
		   sess_cb->pss_user.db_own_name,
		   sizeof(DB_OWN_NAME)))
	{
	    qeuq_cb->qeuq_flag_mask |= QEU_DBA_VIEW;
	}

	/* tell QEF whether this is a QUEL view */
	if (sess_cb->pss_lang == DB_QUEL)
	{
	    qeuq_cb->qeuq_flag_mask |= QEU_QUEL_VIEW;
	}
    }

    /*
    ** let QEF know whether DMT_VGRANT_OK bit needs to be turned on in relstat
    */
    if (grantable)
    {
	qeuq_cb->qeuq_flag_mask |= QEU_VGRANT_OK;
    }

cleanup:

    /*
    ** reset pss_dependencies_stream to NULL to avoid reuse of invalid
    ** stream descriptor
    */
    sess_cb->pss_dependencies_stream = (PSF_MSTREAM *) NULL;

    if (DB_SUCCESS_MACRO(status) && close_stream)
    {
	/*
	** now we need to close the memory stream which was used during
	** the qrymod processing - don't bother closing it if we are going to
	** report an error since code in psq_parseqry() will take care of it
	*/
	status = psf_mclose(sess_cb, &sess_cb->pss_cbstream, &psq_cb->psq_error);
	sess_cb->pss_cbstream.psf_mstream.qso_handle = (PTR) NULL;
    }

    if (DB_SUCCESS_MACRO(status))
    {
	/* 
	** Invalidate base items from RDF cache.  Note that even though
	** assignments into the tabids array were made on the basis of
	** bits in a bit vector, we know that all the "on" bits started at
	** bit 0 and were consecutive (no gaps).
	*/

	RDF_CB	    rdf_cb;
	RDR_RB	    *rdf_rb = &rdf_cb.rdf_rb;

	pst_rdfcb_init(&rdf_cb, sess_cb);

	rdf_rb->rdr_types_mask = (RDF_TYPES) 0;
	rdf_rb->rdr_update_op = 0;

	for (i = 0; i < qtree->pst_rngvar_count; i++)
	{
	    STRUCT_ASSIGN_MACRO(tabids[i], rdf_rb->rdr_tabid);
	    if (stat = rdf_call(RDF_INVALIDATE, (PTR) &rdf_cb))
	    {
		(VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_cb.rdf_error, 
			    &psq_cb->psq_error);

		if (stat > status)
		{
		    status = stat;
		}
		
		break;
	    }
	}
    }
        
    return (status);
}

/*
** Name: psl_cv2_viewstmnt  - semantic action for SQL and QUEL viewstmnt:
**			      production
**
** Description:
**	This function implements semantic action for SQL and QUEL viewstmnt:
**	production
**
** Input:
**	sess_cb		    PSF sesion CB
**	psq_cb		    PSF request CB
**	yyvarsp		    address of a structure containing internal variables
**			    known to the grammar
**			
** Output:
**	sess_cb
**	    pss_object	    will point at the statement node allocated for
**			    CREATE/DEFINE VIEW statement
**	    pss_tchain	    text chain opened for putting together text of the
**			    view definition
**	    pss_ostream	    memory stream opened for building query tree for the
**			    view as well as for storing query text of the view
**			    definition once we are ready to put it into one
**			    contiguous memory block
**	psq_cb
**	    psq_mode	    set to PSQ_VIEW
**	    psq_error	    filled in if an error occurs
**
** Side effects:
**	memory stream is opened and memory is allocated
**
** Returns:
**	E_DB_{OK,ERROR}
**
** History:
**	21-dec-92 (andre)
**	    extracted previously modified semantic action of viewstmnt:
**	    production from pslsgram.yi to form this function
*/
DB_STATUS
psl_cv2_viewstmnt(
		PSS_SESBLK	*sess_cb,
		PSQ_CB		*psq_cb,
		PSS_YYVARS	*yyvarsp)
{
    DB_STATUS			status;
    PTR				piece;
    PST_STATEMENT		*snode;
    PST_CREATE_VIEW	    	*crt_view;
    QEUQ_CB			*qeuq_cb;
    DMU_CB			*dmu_cb;

    psq_cb->psq_mode = PSQ_VIEW;

    /* Open the memory stream for allocating the query tree */
    if (yyvarsp->isdbp == FALSE)
    {
	status = psf_mopen(sess_cb, QSO_QTREE_OBJ, &sess_cb->pss_ostream,
	    &psq_cb->psq_error);
	if (status != E_DB_OK)
	    return (status);
	sess_cb->pss_stk_freelist = NULL;
    }

    /*
    ** CREATE/DEFINE VIEW statement requires query text to be stored in
    ** the iiqrytext relation.  Open a text chain and put the initial
    ** words "create view "/"define view " into it.  The text chain will be
    ** coalesced into a contiguous block later.
    */
    sess_cb->pss_stmt_flags |= PSS_TXTEMIT;	    /* Indicate to emit text */
    status = psq_topen(&sess_cb->pss_tchain, &sess_cb->pss_memleft,
	&psq_cb->psq_error);
    if (status != E_DB_OK)
	return (status);
	
    if (sess_cb->pss_lang == DB_SQL)
    {
	status = psq_tadd(sess_cb->pss_tchain, (u_char *) "create view ",
	    sizeof("create view ") - 1, &piece, &psq_cb->psq_error);
    }
    else
    {
	status = psq_tadd(sess_cb->pss_tchain, (u_char *) "define view ",
	    sizeof("define view ") - 1, &piece, &psq_cb->psq_error);
    }
    
    if (status != E_DB_OK)
	return (status);

    /* Allocate PST_STATEMENT for CREATE/DEFINE VIEW statement */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PST_STATEMENT),
	(PTR *) &snode, &psq_cb->psq_error);
    if (DB_FAILURE_MACRO(status))
	return (status);

    MEfill(sizeof(PST_STATEMENT), (u_char) 0, (PTR) snode);

    snode->pst_mode = PSQ_VIEW;
    snode->pst_type = PST_CREATE_VIEW_TYPE;
    
    sess_cb->pss_object = (PTR) snode;
    crt_view = &snode->pst_specific.pst_create_view;

    /* allocate a QEUQ_CB */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(QEUQ_CB),
	(PTR *) &crt_view->pst_qeuqcb, &psq_cb->psq_error);
    if (DB_FAILURE_MACRO(status))
	return (status);

    MEfill(sizeof(QEUQ_CB), (u_char) 0, (PTR) crt_view->pst_qeuqcb);
	
    qeuq_cb = (QEUQ_CB *) crt_view->pst_qeuqcb;

    /* fill in some basic fields in the new QEUQ_CB */
    qeuq_cb->qeuq_eflag	    = QEF_EXTERNAL;
    qeuq_cb->qeuq_type	    = QEUQCB_CB;
    qeuq_cb->qeuq_length    = sizeof(QEUQ_CB);
    qeuq_cb->qeuq_d_id	    = sess_cb->pss_sessid;
    qeuq_cb->qeuq_db_id	    = sess_cb->pss_dbid;

    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DMU_CB),
	(PTR *) &qeuq_cb->qeuq_dmf_cb, &psq_cb->psq_error);
    if (DB_FAILURE_MACRO(status))
	return (status);

    dmu_cb = (DMU_CB *) qeuq_cb->qeuq_dmf_cb;
    
    MEfill(sizeof(DMU_CB), (u_char) 0, (PTR) dmu_cb);

    /* Fill in the control block header */
    dmu_cb->type = DMU_UTILITY_CB;
    dmu_cb->length = sizeof(DMU_CB);
    dmu_cb->dmu_flags_mask = 0;
    dmu_cb->dmu_db_id = (char*) sess_cb->pss_dbid;
    STRUCT_ASSIGN_MACRO(sess_cb->pss_user, dmu_cb->dmu_owner);
    dmu_cb->dmu_gw_id = DMGW_NONE;

    /*
    ** Give it the default location, even though views
    ** don't have locations.
    */

    /* Allocate 1 location entry. */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DB_LOC_NAME),
	(PTR *) &dmu_cb->dmu_location.data_address, &psq_cb->psq_error);
    if (status != E_DB_OK)
	return (status);

    STmove("$default", ' ', DB_LOC_MAXNAME,
	(char *) dmu_cb->dmu_location.data_address);
    dmu_cb->dmu_location.data_in_size = sizeof(DB_LOC_NAME);

    dmu_cb->dmu_key_array.ptr_address   = 0;
    dmu_cb->dmu_key_array.ptr_in_count  = 0;
    dmu_cb->dmu_key_array.ptr_size	= 0;

    /* Allocate and fill in description entries */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DMU_CHAR_ENTRY),
	(PTR *) &dmu_cb->dmu_char_array.data_address, &psq_cb->psq_error);
    if (DB_FAILURE_MACRO(status))
	return(status);

    ((DMU_CHAR_ENTRY *)dmu_cb->dmu_char_array.data_address)->char_id 
						    = DMU_VIEW_CREATE;
    ((DMU_CHAR_ENTRY *) dmu_cb->dmu_char_array.data_address)->char_value 
						    = DMU_C_ON;
    dmu_cb->dmu_char_array.data_in_size = sizeof(DMU_CHAR_ENTRY);

    /*
    ** When processing CREATE VIEW, we want to replace all references to
    ** synonyms in the view definition with the actual tables + all table names
    ** in the from list(s) must be prefixed with their owner name.
    */
    if (sess_cb->pss_lang == DB_SQL)
    {
	yyvarsp->qry_mask |= (PSS_REPL_SYN_WITH_ACTUAL | PSS_PREFIX_WITH_OWNER);
    }

    /*
    ** group and role permits will be disregarded when parsing definitions
    ** of views, rules, and database procedures
    */
    sess_cb->pss_stmt_flags |= PSS_DISREGARD_GROUP_ROLE_PERMS;

    return(E_DB_OK);
}
