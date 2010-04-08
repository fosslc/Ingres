/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
/* beginning of optimizer header files */
#include    <opglobal.h>
#include    <opdstrib.h>
#include    <opfcb.h>
#include    <opgcb.h>
#include    <opscb.h>
#include    <ophisto.h>
#include    <opboolfact.h>
#include    <oplouter.h>
#include    <opeqclass.h>
#include    <opcotree.h>
#include    <opvariable.h>
#include    <opattr.h>
#include    <openum.h>
#include    <opagg.h>
#include    <opmflat.h>
#include    <opcsubqry.h>
#include    <opsubquery.h>
#include    <opcstate.h>
#include    <opstate.h>

/* external routine declarations definitions */
#include    <ex.h>
#define             OPX_EXROUTINES      TRUE
#define		    OPS_SEQUENCER	TRUE
#undef  COUNTS
#include    <tm.h>
#include    <bt.h>
#include    <me.h>
#include    <tr.h>
#include    <cv.h>
#include    <opxlint.h>

#ifdef COUNTS
FUNC_EXTERN void ade_barf_counts();
#endif

static VOID 
ops_statement(
	OPS_STATE	    *global,
	PST_STATEMENT	    *sp);

/**
**
**  Name: OPSSEQ.C - Sequencer for optimizer
**
**  Description:
**	Routines used for query optimization
**
**  History:
**      20-feb-86 (seputis)    
**          initial creation
**	29-jan-89 (paul)
**	    Changed main sequencing loop by adding ops_statement(). Modified
**	    to support compilation of rule action lists.
**	30-mar-89 (paul)
**	    Not enogh global state reset for each statement compiled.
**	    Corrected this.
**	4-apr-89 (paul)
**	    Integrate Ed Seputis's change to normalize IF conditional expressions.
**	20-apr-89 (paul)
**	    Correct uninitialized variable in the 4-apr IF change.
**	22-may-89 (neil)
**	    Put PSQ_DELCURS through same fast path as PSQ_REPCURS.
**      10-dec-90 (neil)
**          Alerters: Add recognition of new EVENT actions to ops_statement.
**      16-aug-91 (seputis)
**          add CL include files for PC group
**      6-apr-1992 (bryanp)
**          Add support for PSQ_DGTT_AS_SELECT for temporary tables.
**      8-sep-92 (ed)
**          remove OPF_DIST ifdefs
**      28-oct-92 (jhahn)
**          Added support for statement level rules.
**	30-oct-92 (barbara)
**	    Don't use pst_qtext to set 'single_site' boolean.
**	    CREATE AS SELECT puts WITH clause text in pst_qtext
**	    but is not necessarily single_site.
**      02-nov-92 (anitap)
**          Added support for PST_CREATE_SCHEMA in ops_statement(). Should
**          not be optimized.
**	07-dec-92 (teresa)
**	    added PST_REGPROC_TYPE to list of statements that are not 
**	    optimized, only compiled.
**	12-nov-92 (rickh)
**	    Pass DDL statements onto OPC for compilation.
**	17-jan-93 (anitap)
**	    Added support for PST_EXEC_IMM_TYPE in ops_statement(). Should
**	    not be optimized.
**	24-mar-93 (jhahn)
**	    Added fix for support of statement level rules. (FIPS)
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**      4-may-94 (ed)
**          b62908 - rules broke trace point op151 which turns
**          query compilation
**      21-mar-95 (inkdo01)
**          Added support for op170 (parse tree dump).
**      18-Jan-1999 (hanal04) Bug 92148 INGSRV494
**          Enhanced OPC to support compilation of the non-cnf case
**          in opc_crupcurs().
[@history_line@]...
**/

/*{
** Name: ops_seqhandler	- exception handler for sequencer
**
** Description:
**      This is exception handler was declared for the optimizer so that
**      the memory allocated for this optimization can be deallocated.
**      This will be done at the point at which the exception occured.
**
**      If the optimizer signalled the exception via EXOPF then:
**      It will return to the point at which the exception handler was 
**      established and return to the user with the appropriate return status
**      which was placed in the caller's control block by the routine which
**      signalled the exception.
**        
**      If there was another reason for the exception then just return 
**      until some way is given to get the session control block. 
**
** Inputs:
**      none
**
** Outputs:
**      none
**	Returns:
**	    EXDECLARE
**	    EXRESIGNAL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	27-mar-86 (seputis)
**          initial creation
**	7-nov-88 (seputis)
**          add EX_UNWIND
**	13-mar-92 (rog)
**          Remove the EX_DECLARE case and change the EX_UNWIND case to return
**	    EXRESIGNAL.
[@history_line@]...
*/
static STATUS
ops_seqhandler(
	EX_ARGS		*args)
{
    switch (EXmatch(args->exarg_num, (i4)2, EX_JNP_LJMP, EX_UNWIND))
    {
    case 1:
	return (EXDECLARE);	    /* return to invoking exception point
                                    ** - this is a non-enumeration exception
                                    ** being reported by the optimizer e.g.OPC
                                    ** - error status should already have
                                    ** been set in the caller's control block
                                    */
    case 2:
	return (EXRESIGNAL);
    default:
        return (opx_exception(args)); /* handle unexpected exception */
    }
}

/*{
** Name: ops_pushnots	- remove nots from the expressions
**
** Description:
**      Special routine to remove nots from the expressions
**      for normalization to work.
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      9-feb-88 (seputis)
**          initial creation
**	22-mar-91 (seputis)
**	    - bug 42900, add case for PST_UOP so that complement
**	    routine can operate on IS NULL
**	21-jun-93 (andre)
**	    we were not adequately handling cases where the left child of
**	    PST_NOT was another PST_NOT.  Because we were resetting *qnodepp to
**	    q->pst_left BEFORE calling ops_pushnots() on q's left subtree, if q
**	    was of type PST_NOT and so was q->pst_left, we would reset
**	    q->pst_left to q->pst_left->pst_left, but this would not get
**	    propagated to *qnodepp which would continue pointing at q->pst_left
**
**	    This problem would manifest itself whenever a tree contained a chain
**	    of 2 or more PST_NOTs. To correct it, I moved code resetting
**	    *qnodepp if q was of type PST_NOT to AFTER the call to
**	    ops_pushnots() which ensures that we skip past ALL PST_NOTs in a
**	    chain
**	    
[@history_line@]...
[@history_template@]...
*/
static VOID
ops_pushnots(
	OPS_SUBQUERY       *subquery,
	PST_QNODE          **qnodepp,
	bool		    demorgan)
{
    PST_QNODE	*q;

    q = *qnodepp;
    while (q)
    {
	switch (q->pst_sym.pst_type)
	{
	case PST_NOT:
	{   /* apply DeMorgan's laws to remove PST_NOT operator */
	    demorgan = (demorgan == FALSE);
	    break;
	}
	case PST_UOP:
	case PST_BOP:
	{   /* check for subselect and remove it if only simple
	    ** aggregates are computed, this should work for simple
	    ** aggregates and corelated simple aggregates 
	    ** FIXME - OPF will not */
	    if (demorgan)
		opa_not(subquery->ops_global, q);
	    return;
	}
	case PST_OR:
	{
	    if (demorgan)
		/* NOT (A OR B) ==> ((NOT A) AND (NOT B)) */
		q->pst_sym.pst_type = PST_AND;
	    break;
	}
	case PST_AND:
	{
	    if (demorgan)
		/* NOT (A AND B) ==> ((NOT A) OR (NOT B)) */
		q->pst_sym.pst_type = PST_OR;
	    break;
	}
	default:
	    return;
	}
	if (q->pst_left)
	    ops_pushnots(subquery, &q->pst_left, demorgan); /* recurse
					** down left side */
	/*
	** if we have just invoked ops_pushnots() on left child of PST_NOT,
	** reset *qnodepp to q->pst_left to skip past that PST_NOT and, if its
	** left descendant(s) were PST_NOTs with no interfering nodes of
	** different type, skip past them as well
	*/
	if (q->pst_sym.pst_type == PST_NOT)
	    *qnodepp = q->pst_left;

	qnodepp = &q->pst_right;	/* iterate down right side */
	q = *qnodepp;
    }
}

/*{
** Name: ops_normalize	- normalize constant expression
**
** Description:
**      Normalize the constant expression for replace cursor 
**      for OPC and query compilation. Set up as a separate
**      procedure so that the local OPS_SUBQUERY struct
**      does not occupy stack space.
**
** Inputs:
**      global                          ptr to global state 
**                                      variable
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**	4-apr-89 (paul)
**	    Integrated Ed Seputis's change to allow 'or' clauses in
**	    IF conditional expressions.
**	20-apr-89 (paul)
**	    4-apr change did not initialize the field ops_normal in the
**	    temporary subquery structure used by opj_normalize. Needless
**	    to say, this determined whether opj_normalize actually
**	    normalized the expression.
**      18-Jan-1999 (hanal04) Bug 92148 INGSRV494
**          Added new parameter to allow the caller to specify whether
**          we call opj_normalize().
[@history_template@]...
*/
static VOID
ops_normalize(
	OPS_STATE	    *global,
	PST_QNODE	    **qnodepp,
	bool                cnf)
{
    /* need to normalize the qualification for OPC since PSF
    ** does not normalize now */
    OPS_SUBQUERY	subquery;
    subquery.ops_global = global;
    subquery.ops_normal = FALSE;
    subquery.ops_root = NULL;
    
    if (*qnodepp)
	ops_pushnots(&subquery, qnodepp, FALSE);
				    /* make one pass to push nots
				    ** down the tree since this is
				    ** usually done within
				    ** opa_generate */
    if(cnf == TRUE)
        opj_normalize(&subquery, qnodepp);
}

/*{
** Name: ops_decvar	- add a local variable declaration 
**
** Description:
**      OPC cannot handle repeat query parameters in procedures 
**      so that a local variable declaration needs to be used instead
**	for "simple aggregates" and "correlated subselects" which
**	previously only use repeat query parameters 
**
** Inputs:
**      global                          ptr to global state variable
**
** Outputs:
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      9-nov-89 (seputis)
**          initial creation to fix b8629
**      16-oct-92 (ed)
**          - fix bug 40835 - wrong number of duplicates when union views and
**          subselects are used
[@history_template@]...
*/
VOID
ops_decvar(
	OPS_STATE          *global,
	PST_QNODE	    *constp)
{
    OPS_RQUERY	*rqdescp;	/* used to traverse the list
				    ** of repeat query descriptors*/
    i4		node_parmno;	/* parameter number of the
				    ** node being analyzed */
    PST_STATEMENT *statementp;
    PST_DECVAR	*decvarp;
    i4		first_varno;
    i4		const_count;
    
    statementp = global->ops_procedure->pst_stmts;
    if (statementp->pst_type == PST_DV_TYPE)
	first_varno = statementp->pst_specific.pst_dbpvar->pst_nvars
		+ statementp->pst_specific.pst_dbpvar->pst_first_varno;
    else
	first_varno = 1;
    if (constp)
    {
	const_count = 1;	/* create variable declaration for
				** this constant node */
    }
    else
    {
	const_count = global->ops_parmtotal - first_varno + 1;
	if (const_count <= 0)
	    return;			/* no variables are needed */
    }
    decvarp = (PST_DECVAR *)opu_memory(global, (i4)sizeof(*decvarp));
    decvarp->pst_nvars = const_count;
    decvarp->pst_first_varno = first_varno;
    decvarp->pst_vardef = (DB_DATA_VALUE *)opu_memory(global,
	(i4)(const_count * sizeof(DB_DATA_VALUE)));
    decvarp->pst_varname = (DB_PARM_NAME *)opu_memory(global,
	(i4)(const_count * sizeof(DB_PARM_NAME)));
    MEfill(const_count * sizeof(DB_PARM_NAME),
	(u_char)' ', (PTR)decvarp->pst_varname);
    decvarp->pst_parmmode = (i4 *) NULL;

    if (constp)
    {
	MEcopy((PTR)&constp->pst_sym.pst_dataval,
	    sizeof(DB_DATA_VALUE),
	    (PTR)&decvarp->pst_vardef[0]);
	decvarp->pst_varname[0].db_parm_name[0]='z';
	CVna((i4)constp->pst_sym.pst_value.pst_s_cnst.pst_parm_no, 
	    &decvarp->pst_varname[0].db_parm_name[1]);
    }
    else
    {
	node_parmno = global->ops_parmtotal + 1;
	for (rqdescp = global->ops_rqlist; 
	    rqdescp && (node_parmno > first_varno); 
	    rqdescp = rqdescp->ops_rqnext)
	{	/* all OPF generated constants are contiguous so check this */
	    node_parmno--;
	    if (node_parmno != rqdescp->ops_rqnode->pst_sym.pst_value.
		    pst_s_cnst.pst_parm_no)
		opx_error(E_OP0B82_MISSING_PARAMETER); /* make sure the parameters
					** are in order */
	    MEcopy((PTR)&rqdescp->ops_rqnode->pst_sym.pst_dataval,
		sizeof(DB_DATA_VALUE),
		(PTR)&decvarp->pst_vardef[node_parmno-first_varno]);
	    decvarp->pst_varname[node_parmno-first_varno].db_parm_name[0]='z';
	    CVna((i4)node_parmno, &decvarp->pst_varname[node_parmno-first_varno].db_parm_name[1]);
	}
    }

    statementp = (PST_STATEMENT *)opu_memory(global, (i4)sizeof(*statementp));
    MEfill(sizeof(*statementp), (u_char)0, (PTR)statementp); /* fill with
				    ** zero so that new fields like pst_audit
				    ** will have some reasonable value */
    statementp->pst_mode = PSQ_VAR;
    statementp->pst_next = global->ops_procedure->pst_stmts;
    statementp->pst_after_stmt = (PST_STATEMENT *)NULL;
    statementp->pst_before_stmt = (PST_STATEMENT *)NULL;
    statementp->pst_opf = (PTR) NULL;
    statementp->pst_type = PST_DV_TYPE;
    statementp->pst_specific.pst_dbpvar = decvarp;
    statementp->pst_lineno = 0;
    statementp->pst_link = global->ops_procedure->pst_stmts;
    global->ops_procedure->pst_stmts = statementp; /* insert statement
				    ** at beginning of list */
    
    {	
	OPS_SUBQUERY	*save_subquery;
	PST_STATEMENT	*save_statement;

	save_subquery = global->ops_subquery;
	save_statement = global->ops_statement;
	ops_statement(global, statementp);
	/* restore subquery state */
	global->ops_subquery = save_subquery;
	global->ops_statement = save_statement;
    }
}

/*{
** Name: ops_statement -- build a action list for a statement.
**
** Description:
**	This routine optimizes a single statement and builds the
**	corresponding action list to be attached to the current QP.
**	If this is a single statement or the first statement of a
**	db procedure the QP will be initialized. If this is a single
**	statement or the final statement of a db procedure the QP will
**	be completed.
**
**	This routine will also process any rule list associated with the
**	current statement. The rule list is treated as another list of
**	statements to compile. The compile of each rule list statement
**	is implemented by calling this routine recursively.  
**
** Inputs:
**      global          ptr to global state variable
**
**	sp		PST_STATEMENT to optimize and compile.
**
** Outputs:
**
****	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	29-jan-89 (paul)
**	    Added to support rules processing. Most of the logic is
**	    copied directly from ops_dbprocedure(). The processing
**	    of the rule list is new at this time.
**	30-mar-89 (paul)
**	    Not enough state information reset when compiling each
**	    statement. Corrected.
**	04-apr-89 (paul)
**	    Integrated Ed Seputis's change to allow 'or' clauses in
**	    IF conditional expressions.
**	22-may-89 (neil)
**	    Put PSQ_DELCURS through same fast path as PSQ_REPCURS.
**	10-dec-90 (neil)
**	    Alerters: Add recognition of new EVENT actions.
**	12-jan-91 (seputis)
**	    integrated some star changes 
**      4-jun-91 (seputis)
**          fix bug 37557 - print no query plan for single site query
**      6-apr-1992 (bryanp)
**          Add support for PSQ_DGTT_AS_SELECT for temporary tables.
**      28-oct-92 (jhahn)
**          Added support for statement level rules.
**	30-oct-92 (barbara)
**	    Don't use pst_qtext to set 'single_site' boolean.
**	    CREATE AS SELECT puts WITH clause text in pst_qtext
**	    but is not necessarily single_site.
**      12-nov-92 (anitap)
**          Added support for PST_CREATE_SCHEMA.
**	12-nov-92 (rickh)
**	    Pass DDL statements onto OPC for compilation.
**	07-dec-92 (teresa)
**	    added PST_REGPROC_TYPE to list of statements that are not 
**	    optimized, only compiled.
**	10-dec-92 (barbara)
**	    Fixed bug in testing for single site.
**	18-jan-93 (anitap)
**	    Added support for PST_EXEC_IMM_TYPE.
**	24-mar-93 (jhahn)
**	    Added fix for support of statement level rules. (FIPS)
**	15-apr-93 (ed)
**	    retrofit fix for b40591
**	20-may-94 (davebf)
**	    removed vestigial test for 6.4 style singlesite
**      21-mar-95 (inkdo01)
**          Added support for op170 (parse tree dump).
**      18-Jan-1999 (hanal04) Bug 92148 INGSRV494
**          Make non-cnf the default case for DBP IF statement's but
**          allow OLDCNF to be used via trace points or config settings.
**	20-Aug-2004 (schka24)
**	    Fix backwards choice of bool variable name that cost me
**	    way too much time trying to figure out how anything works.
**	3-feb-06 (dougi)
**	    Load ops_override from pst_hintmask for query hints.
**	15-june-06 (dougi)
**	    Add support for "before" triggers.
**	23-jan-2007 (dougi)
**	    Don't call opv_parser() unless the table is a PST_TABLE.
**	25-jan-2007 (dougi)
**	    ... or a PST_SETINPUT.
**	22-mar-10 (smeke01) b123457
**	    Moved trace point op146 call to opu_qtprint from beginning of
**	    opa_aggregate to ops_statement, just before the call to
**	    opa_aggregate. This is logically the same, but makes the display
**	    fit better with trace point op170. Amended condition so that it
**	    is triggered for trace point op146 or op214. Also added a trace
**	    point op214 call to opu_qtprint after opa_aggregate.
*/
static VOID
ops_statement(
	OPS_STATE	    *global,
	PST_STATEMENT	    *sp)
{
    PST_STATEMENT   *sp_rule;	    /* Statement pointer for rule list */
    
				    /*
				    ** force all DDL statements to go directly
				    ** to OPC
				    */
    i4		    statementType = (PST_IS_A_DDL_STATEMENT(sp->pst_type))
					    ? PST_FIRST_DDL_STATEMENT_TYPE
					    : sp->pst_type;
    bool	cnf = FALSE;
    
    /* Perform the optimization and compilation procedures relevant for	    */
    /* the type of statement given. */

    switch ( statementType )
    {
	case PST_IF_TYPE:
	    /* Normalize the expression before compiling. OPC can only	    */
	    /* handle expressions in conjunctive normal form. */
            /* b92148 - Default is now non-cnf but we must flag this as it  */
            /* is not the default in OPC. We still call ops_normalize()     */
            /* with the new cnf parameter set accordingly.                  */
            if((global->ops_cb->ops_server->opg_smask & OPF_OLDCNF) ||
               (opt_strace(global->ops_cb, OPT_F054_OLDCNF))) 
            {
                /* Force to use CNF by trace point of server parameter      */
                cnf = TRUE;
            }
            else
            {
                sp->pst_stmt_mask |= PST_NONCNF;
            }
	    ops_normalize(global, &sp->pst_specific.pst_if.pst_condition, cnf);

	    /* Fall through to continue processing */
	    	    
	case PST_FOR_TYPE:
	case PST_RETROW_TYPE:
	case PST_DV_TYPE:
	case PST_MSG_TYPE:
	case PST_RTN_TYPE:
	case PST_CMT_TYPE:
	case PST_RBK_TYPE:
	case PST_RSP_TYPE:
	case PST_CP_TYPE:
	case PST_EMSG_TYPE:
	case PST_IP_TYPE:
   	case PST_REGPROC_TYPE:
        case PST_EVREG_TYPE:
	case PST_EVDEREG_TYPE:
	case PST_EVRAISE_TYPE:
	case PST_CREATE_SCHEMA_TYPE:
	case PST_EXEC_IMM_TYPE:

	/* PST_FIRST_DDL_STATEMENT_TYPE covers CREATE TABLE/VIEW/SCHEMA, etc. */
	case PST_FIRST_DDL_STATEMENT_TYPE:
	    /* There is no optimization required for statements that do	    */
	    /* not involve queries. Such statements need only be compiled */
	    global->ops_subquery = NULL;    /* Indicate that there is no    */
					    /* query associated with this   */
					    /* statement. Required by OPC.  */
	    global->ops_cstate.opc_subqry = NULL;
	    
	    global->ops_statement = sp;	    /* statement to compile . */

#ifdef    OPT_F023_NOCOMP
	    /* if trace flag is set then query compilation phase will be skipped */
	    if ( !global->ops_cb->ops_check || 
		 !opt_strace( global->ops_cb, OPT_F023_NOCOMP))
#endif

		opc_querycomp(global);      /* compile subqueries/dbproc statements into QEP */
	    break;

	case PST_QT_TYPE:
	{
	    /* FIXME - need timer start and timer end here */
	    /* Optimize and compile this query statement */
	    /* Initialize the state information for compiling a query	    */
	    PST_QTREE	    *qheader;
	    bool	    not_rep_del; /* TRUE if not a replace or delete cursor */
	    bool	    single_site;

	    ops_qinit(global, sp);	/* initialize global structure */
	    qheader = global->ops_qheader;
	    single_site = (bool)FALSE;	/* kludge: singlesite code obsolete */
	    global->ops_cb->ops_override = qheader->pst_hintmask;

#ifdef    OPT_F041_MULTI_SITE
	    /* this will force single site queries to be multi-site for testing */
	    if ( single_site
		&&
		global->ops_cb->ops_check
		&&
		opt_strace( global->ops_cb, OPT_F041_MULTI_SITE))
	    {
		single_site = FALSE;
		qheader->pst_distr->pst_stype = PST_NSITE;
		qheader->pst_distr->pst_qtext = NULL;
	    }
#endif
	    if ((qheader->pst_mode == PSQ_RETRIEVE)
		||
		(   (qheader->pst_mode == PSQ_DEFCURS) /* check
				    ** for read-only cursor */
		    &&
		    (qheader->pst_updtmode == PST_READONLY)
		    &&
		    !qheader->pst_delete 
		))			/* check if there is possibility of update */
		global->ops_gdist.opd_gmask |= OPD_NOFIXEDSITE;
            else if ((  (qheader->pst_mode == PSQ_REPLACE)
                        &&
                        (BTcount((char *)&global->ops_qheader->pst_qtree->pst_sym.
                            pst_value.pst_s_root.pst_tvrm, (i4)BITS_IN(global->ops_qheader
                            ->pst_qtree->pst_sym.pst_value.pst_s_root.pst_tvrm)) <= 1)
                    )
                    ||
                    (qheader->pst_mode == PSQ_DEFCURS)
                    ||
                    (qheader->pst_mode == PSQ_DELETE)
                    )
                    /* before modifying the query tree check if the FROM list
                    ** extension to the update statement is used in distributed
                    ** and turn off the OPD_EXISTS flag if so, since the user
                    ** has specified non-common SQL and there are problems if
                    ** the common SQL techniques are applied , bug 35119 */
                global->ops_gdist.opd_gmask |= OPD_EXISTS; /* this
                                        ** query has constraints for syntax
                                        ** generation for SQL in that correlated
                                        ** references are needed, RETINTO has
                                        ** full select capability */
#if 0
/* this causes bug 40591 */
            else if (qheader->pst_mode != PSQ_RETINTO &&
                     qheader->pst_mode != PSQ_DGTT_AS_SELECT)
		global->ops_gdist.opd_gmask |= OPD_EXISTS; /* this
					** query has constraints for syntax
					** generation for SQL in that correlated
					** references are needed, RETINTO has
					** full select capability */
#endif
		

	    /* need to assign target relation variable for single site
	    ** cursor so the table name can be given for the delete cursor
            ** GCA message in distributed OPC */

	    not_rep_del = (qheader->pst_mode != PSQ_REPCURS)
		&& 
		(qheader->pst_mode != PSQ_DELCURS);
	    if (qheader->pst_restab.pst_resvno >= 0)
	    {	/* the parser may set the result variable when it does not
		** mean it, in particular a cursor which is updateable may
		** become non-updateable after further processing */
		if (global->ops_gdist.opd_gmask & OPD_NOFIXEDSITE ||
		    (qheader->pst_rangetab[qheader->pst_restab.pst_resvno]->
			pst_rgtype != PST_TABLE &&
		    qheader->pst_rangetab[qheader->pst_restab.pst_resvno]->
			pst_rgtype != PST_SETINPUT))
		    qheader->pst_restab.pst_resvno = OPV_NOVAR;
		else if (not_rep_del
		    ||
		    (global->ops_cb->ops_smask & OPS_MDISTRIBUTED))
		    (VOID) opv_parser (global, (OPV_IGVARS)qheader->pst_restab.pst_resvno, 
			OPS_MAIN, TRUE, TRUE, TRUE);	/* assign a
							** global range variable to
							** the result relation */
	    }
	    if (single_site)
	    {
		global->ops_parmtotal = 0; /* for single site queries the parameters
					** should be obtained by SCF from QEF, this
					** field will indicate this to OPC */
		qheader->pst_distr->pst_ldb_desc->dd_l1_ingres_b = opd_bflag(global); /* calculate
					** correct value for ingres_b flag, assume that
					** PSF has made a copy of the LDB descriptor
					** so that it can be overwritten */
		if (global->ops_cb->ops_alter.ops_qep
		  ||
		    (global->ops_cb->ops_check
		    &&
		    opt_strace( global->ops_cb, OPT_FCOTREE)
		    )
		)
		{
		    TRformat(opt_scc, (i4 *)NULL,
			(char *)&global->ops_trace.opt_trformat[0],
			(i4)sizeof(global->ops_trace.opt_trformat),
			 "No query plan for single site query\n");
		}
	    }
	    else
	    {
		/* A replace/delete cursor stmt does not need to be optimized, only
		** the expressions need to be compiled. All other query statements
		** must be optimized first */
		if (not_rep_del)
		{
                    PST_QTREE *qheader;  /* ptr to qheader tree header */  

# ifdef OPT_F001_DUMP_QTREE
		    /* dump query tree if trace flag selected */
		    if (global->ops_cb->ops_check &&
			( opt_strace(global->ops_cb, OPT_F001_DUMP_QTREE) ||
			    opt_strace(global->ops_cb, OPT_F086_DUMP_QTREE2)))
		    {	/* op214 or op146 - dump parse tree in op146 style */
			qheader = global->ops_qheader;
			opu_qtprint(global, qheader->pst_qtree, "before opa" );
		    }
# endif
                    if (global->ops_cb->ops_check &&
                           opt_strace(global->ops_cb, OPT_F042_PTREEDUMP))    
                    {         /* op170 - dump parse tree */
                        qheader = global->ops_qheader;
	                opt_pt_entry(global, qheader, qheader->pst_qtree, 
                            "before opa");
                    }
		    opa_aggregate(global);  /* break tree into subqueries */


# ifdef OPT_F086_DUMP_QTREE2
		    if (global->ops_cb->ops_check &&
			opt_strace(global->ops_cb, OPT_F086_DUMP_QTREE2))
		    {	/* op214 - dump parse tree in op146 style */
			qheader = global->ops_qheader;
			opu_qtprint(global, qheader->pst_qtree, "after opa");
		    }
# endif
                    if (global->ops_cb->ops_check &&
                           opt_strace(global->ops_cb, OPT_F042_PTREEDUMP))    
                    {         /* op170 - dump parse tree */
                        qheader = global->ops_qheader;
	                opt_pt_entry(global, qheader, qheader->pst_qtree, 
                            "after opa");
                    }
		    if (global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
			opd_idistributed(global); /* initialize the site movement
					** cost arrays needed by this 
					** distributed thread */
		    opj_joinop(global);	/* choose plans for subqueries */
		}
		else 
		{
		    /* replace or delete cursor statement, only
		    ** the expressions need to be compiled
		    */
		    if (global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
			opa_aggregate(global);  /* distributed needs the ~V parameters
					** in the target list expressions */
		    ops_normalize(global, &global->ops_qheader->pst_qtree->pst_right, TRUE);  
					/* normalize the constant expressions
					** for OPC */
		}
	    }

#ifdef    OPT_F023_NOCOMP
	    /* if trace flag is set then query compilation phase will be skipped */
	    if ( !global->ops_cb->ops_check || 
		 !opt_strace( global->ops_cb, OPT_F023_NOCOMP))
#endif
	    {
		if (global->ops_procedure->pst_isdbp
		    &&
		    global->ops_rqlist)
		    ops_decvar(global, (PST_QNODE*)NULL); /* convert the "repeat query 
					** list" to a local
					** variable list for the procedure */
		opc_querycomp(global);	/* compile subqueries/dbproc statements into QEP */
	    }

	    /* Now check for a rule statement list. IF one is found compile */
	    /* the rule list before returning to the optimization of the    */
	    /* outer statements.					    */
	    /* Also, note that we do not call ops_deallocate for the	    */
	    /* triggering query until after the rule list is compiled. This */
	    /* is necessary to maintain the state information from the	    */
	    /* triggering query needed for rule compilation.		    */
	    /* It is also important to note that the state of the	    */
	    /* compilation is maintained in global structures. This	    */
	    /* restricts the types of statements allowed in the rule list   */
	    /* to non-query statements. Significant changes to the OPF data */
	    /* structures will be required to allow queries in a rule list. */
	    if (global->ops_inAfterRules)
	    {
		/* OPC has already set the state information we need to	    */
		/* successfully compile the rule list. At this point only   */
		/* rule list statements may be compiled until the end of    */
		/* the rule list is reached. */
		/*							    */
		/* Also note that pst_link is the chain of rule list	    */
		/* statements that we compile. The last rule list entry	    */
		/* must have a NULL pst_link field. */
		for (sp_rule = sp->pst_after_stmt; sp_rule;
		     sp_rule = sp_rule->pst_link)
		{
		    if (sp_rule == sp->pst_statementEndRules)
			global->ops_inAfterStatementEndRules = TRUE;
		    ops_statement(global, sp_rule);
		}
		global->ops_inAfterStatementEndRules = FALSE;

		/* Make one last call to complete the compilation of the    */
		/* rule action list. This call also unsets the global	    */
		/* compilation flag that indicates we are processing a rule */
		/* list. */
		global->ops_statement = (PST_STATEMENT *)NULL;
#ifdef    OPT_F023_NOCOMP
		if
		    (
			/* if trace flag is set then query compilation phase will be skipped */
			!global->ops_cb->ops_check 
			|| 
			!opt_strace( global->ops_cb, OPT_F023_NOCOMP)
		    )
#endif
		    opc_querycomp(global);

		/* Completed rule processing, return to user query	    */
		/* compilation */
		global->ops_inAfterRules = FALSE;
		if (sp->pst_before_stmt)
		    global->ops_inBeforeRules = TRUE;
	    }

	    /* Now check for before rules and perform the same processing. */
	    if (global->ops_inBeforeRules)
	    {
		/* The after list (if any) has been run. Now go through
		** the before list and do the same stuff. */
		for (sp_rule = sp->pst_before_stmt; sp_rule;
		     sp_rule = sp_rule->pst_link)
		{
		    if (sp_rule == sp->pst_statementEndRules)
			global->ops_inAfterStatementEndRules = TRUE;
		    ops_statement(global, sp_rule);
		}
		global->ops_inBeforeStatementEndRules = FALSE;

		/* Make one last call to complete the compilation of the    */
		/* before rule action list. This call also unsets the global*/
		/* compilation flag that indicates we are processing a rule */
		/* list. */
		global->ops_statement = (PST_STATEMENT *)NULL;
#ifdef    OPT_F023_NOCOMP
		if
		    (
			/* if trace flag is set then query compilation phase will be skipped */
			!global->ops_cb->ops_check 
			|| 
			!opt_strace( global->ops_cb, OPT_F023_NOCOMP)
		    )
#endif
		    opc_querycomp(global);

		/* Completed rule processing, return to user query	    */
		/* compilation */
		global->ops_inBeforeRules = FALSE;
	    }
	    ops_deallocate(global, TRUE, TRUE); /* deallocate the RDF descriptors for
					** this query */
	    break;
	}
	default:
	    opx_error(E_OP0B80_UNKNOWN_STATEMENT);
    }
}

/*{
** Name: ops_dbprocedure	- process a procedure statement
**
** Description:
**      This routine will recursively optimize all the statements in a 
**      procedure.
**
** Inputs:
**      global                          ptr to global state variable
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-may-88 (seputis)
**          initial creation
**      5-june-88 (eric)
**          added line to set the ops_statement field.
**	29-jan-89 (paul)
**	    Split the function of this routine with ops_statement to support
**	    the compilation of rule statement lists.
[@history_template@]...
*/
static VOID
ops_dbprocedure(
	OPS_STATE	    *global,
	PST_STATEMENT	    *statementp)
{
    for(;statementp; statementp = statementp->pst_link)
    {	/* traverse the statement block until the end is reached */
	ops_statement(global, statementp);
    }
}

/*{
** Name: ops_sequencer	- Control phases of optimization execution
**
** Description:
**      This routine controls the flow of the query optimization through
**      the major phases of the OPF facility i.e. 
**          1) Initialization phase
**                - Establish exception handler
**                - retrieve query tree from QSM
**                - initialize query header block
**          2) Aggregate separation into subqueries
**                - create linked list of subqueries in the order which
**                  they need to be evaluated
**          3) Join optimization phase
**                - for each subquery...
**                  a) transform the subquery tree into a form which is more
**                    easily manipulated by the optimization phase (equivalent
**                    to old "joinop" phase)
**                  b)produce the respective "CO" tree which is the 
**                    best for each subquery (equivalent to old "enum" phase)
**          4) Query compilation into the QEP
**                - create a QEP which will evaluate the subqueries in the
**                  proper order
**          5) Disposition of query
**                - release internal storage used to optimize query
**                - unlock QSM storage used to pass query tree
**                - return to caller (send QEP back to SCF sequencer)
**
** Inputs:
**      opf_cb                          pointer to caller's control block
**
** Outputs:
**      opf_cb                          pointer to caller's control block
**          .opf_qep                    QSF qep id created
**	Returns:
**	    E_DB_OK, E_DB_ERROR, E_DB_FATAL
**	Exceptions:
**	    If any error occurs then an exception will be signaled which
**          will clear the call stack.  The return status is placed in
**          session control block and returned to the user at the point
**          at which the exception handler was declared.
**
** Side Effects:
**	    none
**
** History:
**	20-feb-85 (seputis)
**          initial creation
**	20-dec-90 (seputis)
**	    integrate some star changes
**	4-feb-91 (seputis)
**	    add support for the OPF active flag
**      20-jul-93 (ed)
**          changed name ops_lock for solaris, due to OS conflict
**	4-may-94 (ed)
**	    b62908 - rules broke trace point op151 which turns off
**	    query compilation
**	12-Oct-2007 (jgramling - DATAllegro) b119340
**	    In ops_sequencer, mutex OPF OPS sem lock left behind after
**	    opf active_limit waiting user cancelled or aborted.
**	1-feb-2008 (dougi)
**	    Add tp op144 to write OPF session stats to the DBMS log.
**	4-june-2008 (dougi)
**	    Temporarily inhibit execution of plans with table procedures.
*/
DB_STATUS
ops_sequencer(
	OPF_CB             *opf_cb)
{
    OPS_STATE      global;	    /* this is the session level state
				    ** control block - it is equivalent to the
                                    ** old Jn, Jnn, and Jne structures
				    */
    EX_CONTEXT     excontext;	    /* context block for exception handler*/
    DB_STATUS	   status;          /* user return status */
    i4		   cpums;

    /* need to initialize stuff for the exception handler */
    global.ops_mstate.ops_streamid = NULL;     /* memory manager stream is not allocated
                                    ** yet - NULL indicates this (in case there
                                    ** is an early exception so we do not try
                                    ** to deallocate this resource)
                                    */
    ((OPS_CB *)opf_cb->opf_scb)->ops_state = (struct _OPS_STATE *) &global; /* save ptr to global state
                                    ** variable in session control block
                                    ** so that exception handler can find it
                                    */
    global.ops_caller_cb = opf_cb;  /* - save the caller's control block
                                    ** - the exception handler can now find
                                    ** all control blocks given only the
                                    ** session control block as a handle (the
                                    ** session control block is the only one
                                    ** which can be requested from the SCF
                                    ** within an exception handler)
                                    */
    if ( EXdeclare(ops_seqhandler, &excontext) != OK)
    {
	/* - this exception handler was established to return optimizer memory
	** to the memory manager
	** - whenever any error occurs during optimization this exception
	** handler will be invoked and this exit point will be used i.e. if
	** the end of ANY internal OPF procedure is reached 
	** then no errors will have occurred... thus no error status is returned
	** from any procedure internal to this one
	*/
	(VOID)EXdelete();
	if ( EXdeclare(ops_seqhandler, &excontext) != OK)
	{
	    (VOID)EXdelete();
	    /* a very fatal exception occurred so do not continue but
            ** instead report the exception within the exception to the
            ** next exception handler */
	    opx_error (E_OP0083_UNEXPECTED_EXEX); /* report a new error to the
					    ** next exception handler
					    ** - an exception occurred within an
					    ** cleanup routine for exception 
					    ** handler */
	}
	status = opx_status( opf_cb,(OPS_CB *)opf_cb->opf_scb ); /* get user 
                                            ** return status since an exception 
                                            ** was signalled
                                            */
	ops_deallocate( &global, FALSE, FALSE);    /* deallocate resources for this
                                            ** optimization (memory resources
                                            ** RDF, and QSF resources)
                                            ** - FALSE indicates not to
                                            ** report any deallocation
                                            ** errors which may occur, since
                                            ** there is already an error to
                                            ** be reported
                                            */
    }
    else
    {
        /* FIXME - need timer start and timer end here */
	OPG_CB	    *servercb;
	servercb = ((OPS_CB *)opf_cb->opf_scb)->ops_server;
	if (servercb->opg_mask & OPG_CONDITION)
	{
	    status = ops_exlock(opf_cb, &servercb->opg_semaphore); /* check if server
						** thread is available, obtain
						** semaphore lock on critical variable */
	    if (DB_FAILURE_MACRO(status))
		return(status);
	    servercb->opg_waitinguser++;	/* increment number of waiting users
						** once */
	    if (servercb->opg_slength < servercb->opg_waitinguser)
		servercb->opg_slength = servercb->opg_waitinguser; /* keep
						** track of maximum length of the
						** queue */
	    if (servercb->opg_activeuser >= servercb->opg_mxsess)
	    {
		i4		waittime;
		servercb->opg_swait++;		/* this thread has to wait
						*/
		waittime = TMcpu();
		do 
		{
		    STATUS	    csstatus;
		    csstatus = CScnd_wait(&servercb->opg_condition, &servercb->opg_semaphore);
						    /* wait until signalled by exiting
						    ** OPF thread */
		    if (csstatus != OK)
		    {
			servercb->opg_waitinguser--;	/* decrement number of waiting
						** users since an error exit will be
						** taken */
			/* Remove this check ... always do ops_unlock or get
			** stuck when a waiting user session is aborted or
			** cancelled.
			** if (csstatus != E_CS000F_REQUEST_ABORTED)
			*/
			    /* usually on a server shutdown, cntrl C, this error can occur */
			status = ops_unlock(opf_cb, &servercb->opg_semaphore); /* check if server
						    ** thread is available */
			opx_rverror( opf_cb, E_DB_ERROR, E_OP0097_CONDITION, 
			    (OPX_FACILITY)csstatus);
			return(E_DB_ERROR);
		    }
		} 
		while (servercb->opg_activeuser >= servercb->opg_mxsess);
		waittime = TMcpu() - waittime;
		if (servercb->opg_mwaittime < waittime)
		    servercb->opg_mwaittime = waittime;	/* save maximum wait time */
		servercb->opg_avgwait += ((f8)waittime - servercb->opg_avgwait)
					/ (f8)servercb->opg_waitinguser; /* calculate
						** running average of wait time for
						** threads which wait */
	    }

	    servercb->opg_activeuser++;		/* increment active server count */
	    status = ops_unlock(opf_cb, &servercb->opg_semaphore); /* check if server
						** thread is available */
	    if (DB_FAILURE_MACRO(status))
	    {
		servercb->opg_activeuser--;	/* decrement in case of error */
		servercb->opg_waitinguser--;		/* decrement in case of error */
		return(status);
	    }
	    ((OPS_CB *)opf_cb->opf_scb)->ops_smask |= OPS_MCONDITION; /* mark this
						** session as having to signal
						** the condition upon exit */
	}
	
	ops_init(opf_cb, &global);          /* initialize global structure */

	/* Prepare to compute session CPU time. */
	{
	    STATUS	csstat;
	    TIMERSTAT	timer_block;

	    csstat = CSstatistics(&timer_block, (i4)0);
	    if (csstat == E_DB_OK)
		cpums = timer_block.stat_cpu;
	}

	ops_dbprocedure(&global, global.ops_procedure->pst_stmts);

	/* Check for display of adfi counts (inkdo01 kludge) */
#ifdef COUNTS
	if (global.ops_cb->ops_check 
		&& 
		opt_strace( global.ops_cb, OPT_F076_BARF)) ade_barf_counts();
#endif

	/* For rule processing, modified the system to require that all */
	/* queries call back to opc_querycomp a final time to complete  */
	/* the QP and set the QSO object root. Previously this was	    */
	/* done only for db procedures. */
	global.ops_statement = NULL;    /* this defines the final pass to
					** OPC */
#ifdef    OPT_F023_NOCOMP
	if
	    (
		/* if trace flag is set then query compilation phase will be skipped */
		!global.ops_cb->ops_check 
		|| 
		!opt_strace( global.ops_cb, OPT_F023_NOCOMP)
	    )
#endif
	    opc_querycomp(&global);	    /* compile subqueries into QEP */

	/* Compute session CPU time. */
	{
	    STATUS	csstat;
	    TIMERSTAT	timer_block;

	    csstat = CSstatistics(&timer_block, (i4)0);
	    if (csstat == E_DB_OK)
		cpums = timer_block.stat_cpu - cpums;
	}
#ifdef    OPT_F016_OPFSTATS
	if
	    (
		/* Write session stats to log if tp is set. */
		global.ops_cb->ops_check 
		&& 
		opt_strace( global.ops_cb, OPT_F016_OPFSTATS )
	    )
#endif
	{
	    /* Display OPF stats. */
	    i4	memory, mcount, m2kcount, recover;

	    memory = global.ops_mstate.ops_totalloc;
	    mcount = global.ops_mstate.ops_countalloc;
	    m2kcount = global.ops_mstate.ops_count2kalloc;
	    recover = global.ops_mstate.ops_recover;
	    TRdisplay("OPF stats: CPU:%d, memory:%d, mcount:%d, m2kcount:%d, recover:%d\n\n",
		cpums, memory, mcount, m2kcount, recover);
	}

	ops_deallocate( &global, TRUE, FALSE);     /* deallocate resources for this
                                            ** optimization (memory resources
                                            ** RDF, and QSF resources)
                                            ** - TRUE indicates to signal any
                                            ** errors which occur
                                            */
	status = global.ops_cb->ops_retstatus; /* if we got this far then
                                            ** everything went OK */
#ifdef    OPT_F032_NOEXECUTE
	/* if trace flag is set then execution will not occur */
        if (global.ops_cb->ops_check 
	    && 
	    (	    opt_strace( global.ops_cb, OPT_F032_NOEXECUTE)
		    ||
		    opt_strace( global.ops_cb, OPT_F023_NOCOMP)
	    )
	    &&
	    DB_SUCCESS_MACRO(status)
           )
	{   /* report a user error indicating trace flag set so query cannot
            ** be processed */
	    opx_verror(E_DB_ERROR, E_OP0008_NOEXECUTE, (OPX_FACILITY)0);
	}
#endif

    }

    (VOID)EXdelete();                             /* delete exception handler */
    return(status);			    
}
