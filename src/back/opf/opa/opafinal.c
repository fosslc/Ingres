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
#define             OPA_FINAL           TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 
/**
**
**  Name: OPAFINAL.C - final pass for aggregate processing
**
**  Description:
**      Contains the final pass of the aggregate processor
**
**  History:    
**      28-jul-86 (seputis)
**          initial creation
**	12-jan-91 (seputis)
**	    - changes for b35243
**      6-apr-1992 (bryanp)
**          Add support for PSQ_DGTT_AS_SELECT for temporary tables.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	06-08-93 (shailaja)
**	    changed funcion opa_corelated from static bool to void.
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	06-mar-96 (nanpr01)
**	    Remove the dependencies on DB_MAXTUP for increased tuple size
**	    project. Also use the datatype of width consistently.
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**	31-Aug-2006 (kschendel)
**	    Watch for HFAGG as well as RFAGG, even though they shouldn't
**	    appear yet this early in opa.
**/

/*{
** Name: opa_corelated	- process corelated variable
**
** Description:
**      This routine is called if the variable is corelated and needs to
**      be substituted by a "repeat query constant".  A list of corelated
**      variables and constant pairs is created at the outermost subselect
**      i.e. the subselect at which the substitution will occur.  This
**      means that children of this subselect will use the same repeat
**      query constant number.
**      FIXME- need to check for scope before opa_optimize because
**      corelated variables may not be substituted
**
** Inputs:
**      subquery                        ptr to subquery descriptor which
**                                      defines the scope to be searched
**      qnodepp                         ptr to var node to be substituted
**                                      if it is corelated
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      18-feb-87 (seputis)
**          initial creation
**      18-mar-91 (seputis)
**          process all subselects which have the same substructure
**	    needed for common hold file optimization
**      13-may-93 (ed)
**          - add null byte if outer joins exist to any correlated constant
**	 3-jun-04 (hayke02)
**	    Modify the above change so that we only add the null byte for
**	    those vars that are the inner in an outer join. This change fixes
**	    problem INGSRV 2674, bug 111629.
**	20-apr-05 (inkdo01)
**	    Change previous fix to handle 128-variable range tables (must
**	    use BTcount on pst_j_mask's).
[@history_template@]...
*/
 
VOID
opa_corelated(
	OPS_SUBQUERY       *subquery,
	PST_QNODE          **qnodepp)
{   /* check if this var has already been seen in the corelated list */
    OPV_IGVARS	    gvar;	    /* global range variable number */
    PST_QNODE	    *qnode;	    /* ptr to var node which may be corelated */
    OPS_STATE	    *global;
    OPS_SUBQUERY    *together;	    /* process all subselects which
				    ** have common structures */
    PST_QNODE	    *constantp;

    global = subquery->ops_global;
    qnode = *qnodepp;
    gvar = qnode->pst_sym.pst_value.pst_s_var.pst_vno;
    constantp = NULL;
    for (together = subquery; together;
	together = together->ops_agg.opa_concurrent)
    {
 /* find the FROM list of the outer select, or outer aggregate 
	** in which this var is defined
	** - check if the repeat query parameter is defined
	** at the outer subselect that can supply the value */
	OPS_SUBQUERY    *outer;         /* used to find correct scope to 
				    ** substitute the variable */
	for (outer = together; 
	    (outer->ops_sqtype != OPS_MAIN)
	    &&
	    (!BTtest((i4)gvar, (char *)&outer->ops_agg.opa_father->
		    ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm));
	    outer = outer->ops_agg.opa_father)
	    ;
#ifdef    E_OP0280_SCOPE
	    if (outer->ops_sqtype == OPS_MAIN)
		opx_error( E_OP0280_SCOPE); /* corelated var not found */
#endif
	{
	    OPV_GSUBSELECT  *gsubselect;    /* ptr to subselect descriptor - used to
					** attach corelated variable descriptors */
	    gsubselect = global->ops_rangetab.opv_base->
		opv_grv[outer->ops_gentry]->opv_gsubselect;
#ifdef    E_OP0280_SCOPE
	    if (!gsubselect)
		opx_error( E_OP0280_SCOPE); /* subselect struct expected */
#endif
					/* FIXME - this check should not be
					** needed once corelated aggregates
					** are working */
	    if (gsubselect->opv_parm)
	    {   /* there are already some corelated parameters defined so check if
		** the current var node is represented in this list */
		OPV_SEPARM  *parm;	    /* correlated var descriptor */
		bool	    found;

		found = FALSE;
		for (parm = gsubselect->opv_parm; parm; 
		    parm = parm->opv_senext)
		{
		    if (gvar == parm->opv_sevar->pst_sym.pst_value.pst_s_var.pst_vno)
		    {
			if (qnode->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id
			    ==
			    parm->opv_sevar->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id
			    )
			{
			    *qnodepp = parm->opv_seconst; /* variable is already in
					    ** the corelation list so substitute 
					    ** the constant */
			    found=TRUE;
			    break;
			}
		    }
		}
		if (found)
		    continue;		    /* check next subquery */
	    }
	    {   /* add variable to corelation list and substitute constant node */
		OPV_SEPARM	*newparm;
		newparm = (OPV_SEPARM *) opu_memory( global, (i4)sizeof(OPV_SEPARM));
		newparm->opv_sevar = qnode;
		if (!constantp)
		    constantp = opv_constnode(global, 
		    &qnode->pst_sym.pst_dataval,
		    ++global->ops_parmtotal); /* allocate repeat query
						** parameter for same type
						** as var node */
		newparm->opv_seconst = constantp;
		newparm->opv_senext = gsubselect->opv_parm; /* place in
						** linked list of parms */
                if ((global->ops_qheader->pst_numjoins > 0)
		    &&
		    outer->ops_agg.opa_father->ops_oj.opl_pinner
		    &&
		    BTcount((char *)&outer->ops_agg.opa_father->ops_oj.
			opl_pinner->opl_parser[gvar].pst_j_mask, OPV_MAXVAR))
                {       /* convert constant to a nullable parameter, since it may
                    ** be involved in an outer join */
                    if (newparm->opv_seconst->pst_sym.pst_dataval.db_datatype > 0)
		    {
                        newparm->opv_seconst->pst_sym.pst_dataval.db_datatype =
                         -newparm->opv_seconst->pst_sym.pst_dataval.db_datatype;
                        newparm->opv_seconst->pst_sym.pst_dataval.db_length++; /*
						** add null byte */
		    }
                }
		gsubselect->opv_parm = newparm;
		*qnodepp = newparm->opv_seconst;/* variable is added to
						** the corelation list so substitute 
						** the constant */
		if (global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
		{	/* FIXME - assumption is that highest repeat query numbers
		    ** are used for subselect correlated variables,... validate
		    ** this assumption */
		    if (global->ops_gdist.opd_repeat
			||
			(global->ops_parmtotal % OPZ_MAXATT))
		    {   /* if initial pass or more memory needed then allocate new
			** buffer */
			OPZ_BMATTS	    *temp;
			i4		    newsize;
			temp = global->ops_gdist.opd_repeat;
			newsize = ((global->ops_parmtotal/OPZ_MAXATT) + 1)
			    *sizeof(*global->ops_gdist.opd_repeat);
			global->ops_gdist.opd_repeat = (OPZ_BMATTS *)opu_memory(global,
			    (i4)newsize);	    /* FIXME - old memory is thrown away
						** -try to reuse this */
			global->ops_gdist.opd_repeat = (OPZ_BMATTS *)opu_memory(global,
			    (i4)sizeof(*global->ops_gdist.opd_repeat));
			MEfill(newsize, (u_char)0, 
			    (PTR)global->ops_gdist.opd_repeat);
			if (temp)	/* if second time then copy variable */
			    MEcopy((PTR)temp, 
				newsize-sizeof(*global->ops_gdist.opd_repeat),
				(PTR)global->ops_gdist.opd_repeat);
		    }
		    BTset((i4)global->ops_parmtotal, 
			(char *)global->ops_gdist.opd_repeat);
		}
	    }
	}
    }
}

/*{
** Name: opa_subselect	- assign repeat query parameters to subselect node
**
** Description:
**      Recursive descent of the query tree associated with the subselect 
**      and replace all corelated var nodes with repeat query constant 
**      nodes
**
** Inputs:
**      subquery                        subquery representing subselect
**      qnodepp                         parse tree node being analyzed
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    replaces parse tree corelated var nodes with constant nodes
**
** History:
**      8-feb-87 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opa_subselect(
	OPS_SUBQUERY       *subquery,
	PST_QNODE          **qnodepp)
{
    PST_QNODE           *qnode;		/* dereference ptr to ptr */

    qnode = *qnodepp;
    if (qnode->pst_sym.pst_type == PST_VAR)
    {   
        /* check if this var exists in the corelation map */
	if (BTtest((i4)qnode->pst_sym.pst_value.pst_s_var.pst_vno,
	    (char *)&subquery->ops_correlated))
	{
	    opa_corelated(subquery, qnodepp); /* find or create constant node
                                        ** for substitution */
	    subquery->ops_vmflag = FALSE; /* varmap can be made invalid by this 
				    ** the removal of the corelated variable */
	    return;
	}
    }
    if (qnode->pst_left)
	opa_subselect(subquery, &qnode->pst_left);
    if (qnode->pst_right)
	opa_subselect(subquery, &qnode->pst_right);
}

/*{
** Name: opa_resdom	- process aggregate resdom list
**
** Description:
**      Create resdom list for aggregates
**
** Inputs:
**      subquery                        subquery representing aggregate
**      simple                          TRUE if a simple aggregate
**
** Outputs:
**      subquery                        ptr to subquery state var
**      root                            root node containing resdom list to process
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      23-mar-87 (seputis)
**          initial creation
**      26-aug-88 (seputis)
**          fixed casting problem
**      26-sep-88 (seputis)
**          skip over non-printing resdoms for duplicate semantics
**      6-nov-88 (seputis)
**          new variables may be added to the target list so
**	    ops_vmflag is reset
**      10-nov-88 (seputis)
**          added some resdom relabelling for OPC
**      10-sep-89 (seputis)
**          make sure non-printing resdoms are in monotically decreasing 
**	    order, since OPC assumes this, otherwise nested aggregates may
**          fail
**      23-aug-90 (seputis)
**          - fix b32738, opa_graft was changed by opa_optimize
**      12-aug-91 (seputis)
**          - fix bug 38691 - print error if aggregate temp too large
**	15-dec-91 (seputis)
**	    fix bug 40402 - corelated group by aggregate gives wrong answer
**	26-dec-91 (seputis)
**	    fix bug 41526 - if expressions exist in multi-variable aggregate
**	    queries then an incorrect answer may be given due to incorrect
**	    top sort node elimination
**	06-mar-96 (nanpr01)
**	    Remove the dependencies on DB_MAXTUP for increased tuple size
**	    project. Also use the datatype of width consistently.
**	22-june-01 (inkdo01)
**	    Changes for SAGG's to preserve 2nd operand ptr of OLAP binary aggs.
**	6-apr-06 (dougi)
**	    Permit PST_GSET/GBCR/GCL in BY list (for GROUP BY enhancements).
[@history_template@]...
*/
static bool
opa_resdom(
	OPS_SUBQUERY       *subquery,
	PST_QNODE	   *root)
{   /* place all the resdom nodes associated with the aggregate 
    ** operators into the resdom list on the left side of the root
    ** - the resdom nodes have already been initialized */
    OPS_SUBQUERY       *concurrent; /* subqueries executed concurrently
				*/
    bool		non_printing; /* TRUE if at least one non-printing
				** resdom has been found */
    bool		relabel_resdom; /* TRUE if at least one non-printing
				** resdom has been found, and this is
				** an aggregate */
    OPS_WIDTH		width;	/* width of target list, using the aggregate 
				** expressions rather than aggregate results
				** - only necessary for function aggregates */
    bool		expr_found; /* TRUE - if at least one expression
				** was found, so top sort node removal cannot
				** be made */

    width = 0;
    relabel_resdom =
    non_printing = FALSE;
    expr_found = FALSE;

    {   /* skip over any non-printing resdoms used for duplicate
	** semantics, any non printing resdoms were added
	** immediately to the left side of the query tree 
	** also skip over any byhead nodes or aop nodes */
	PST_QNODE	**resdompp;
	PST_QNODE	*treenodep;
	PST_QNODE	*nodep;
	bool		byexpr_found;
	bool		tree_found;
	bool		gsets_found;

	byexpr_found = FALSE;
	tree_found = FALSE;
	gsets_found = FALSE;
	treenodep = NULL;
	for (resdompp = &subquery->ops_root->pst_left; *resdompp;)
	{
	    switch ((*resdompp)->pst_sym.pst_type)
	    {
	    case PST_TREE:
		if (tree_found)
		    opx_error(E_OP0681_UNKNOWN_NODE); /* only one TREE
				    ** node should be found */
		tree_found = TRUE;
		treenodep = (*resdompp);
		resdompp = &(*resdompp)->pst_left;
		break;
	    case PST_RESDOM:
		if (tree_found)
		    opx_error(E_OP0681_UNKNOWN_NODE); /* only one TREE
				    ** node should be found */
		if (!((*resdompp)->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT))
		    non_printing = TRUE; /* detect if a non-printing resdom
				    ** is in the list */
		if ((*resdompp)->pst_sym.pst_dataval.db_length > 0)
		    width += (*resdompp)->pst_sym.pst_dataval.db_length;
		if (((*resdompp)->pst_right->pst_sym.pst_type != PST_VAR)
		    &&
		    ((*resdompp)->pst_right->pst_sym.pst_type != PST_AOP)
		    )
		    expr_found = TRUE; /* look for group by expressions */
		resdompp = &(*resdompp)->pst_left;
		break;
	    case PST_BYHEAD: /* BYHEAD for function agg */
		subquery->ops_byexpr = (*resdompp)->pst_left; /* resdom list
				    ** for by-values should be intact here */
		if (byexpr_found
		    ||
		    (	subquery->ops_sqtype != OPS_FAGG
			&&
			subquery->ops_sqtype != OPS_HFAGG
			&&
			subquery->ops_sqtype != OPS_RFAGG
		    )
		    )
		    opx_error(E_OP0681_UNKNOWN_NODE); /* only one BYHEAD
				    ** or one AOP should be found */

		if (subquery->ops_byexpr->pst_sym.pst_type == PST_GSET)
		{
		    /* BY list contains grouping sets. Make a copy of it
		    ** before messing around with the RESDOMs. */
		    opv_copytree(subquery->ops_global, &subquery->ops_byexpr);
		}
		*resdompp = (*resdompp)->pst_left;
		byexpr_found = TRUE;
		break;
	    case PST_GSET:
		if( subquery->ops_sqtype != OPS_FAGG
		    &&
		    subquery->ops_sqtype != OPS_HFAGG
		    &&
		    subquery->ops_sqtype != OPS_RFAGG
		    )
		    opx_error(E_OP0681_UNKNOWN_NODE); /* GSETs can only
					** appear in function aggs */

		/* Merge GSETs inner list with outer RESDOM list. */
		for (nodep = (*resdompp)->pst_right;
			nodep && nodep->pst_left->pst_sym.pst_type != PST_TREE;
			nodep = nodep->pst_left);
		nodep->pst_left = (*resdompp)->pst_left;
		*resdompp = (*resdompp)->pst_right;
		break;
	    case PST_GBCR:
		if( subquery->ops_sqtype != OPS_FAGG
		    &&
		    subquery->ops_sqtype != OPS_HFAGG
		    &&
		    subquery->ops_sqtype != OPS_RFAGG
		    )
		    opx_error(E_OP0681_UNKNOWN_NODE); /* GBCRs can only
					** appear in function aggs */
		resdompp = &(*resdompp)->pst_left;
		break;
	    case PST_GCL:
		/* PST_GCL should be empty and should be in func agg. */
		if( subquery->ops_sqtype != OPS_FAGG
		    &&
		    subquery->ops_sqtype != OPS_HFAGG
		    &&
		    subquery->ops_sqtype != OPS_RFAGG
		    ||
		    (*resdompp)->pst_right != (PST_QNODE *) NULL
		    )
		    opx_error(E_OP0681_UNKNOWN_NODE); /* GBCRs can only
					** appear in function aggs */
		*resdompp = (*resdompp)->pst_right;
		break;
	    case PST_AOP:   /* AOP could be found for simple agg */
		if( subquery->ops_sqtype != OPS_SAGG
		    &&
		    subquery->ops_sqtype != OPS_RSAGG
		    )
		    opx_error(E_OP0681_UNKNOWN_NODE); /* only one BYHEAD
				    ** or one AOP should be found */
		*resdompp = NULL;   /* skip the remainder of the simple
				    ** aggregate expression */
		break;
	    default:
		opx_error(E_OP0681_UNKNOWN_NODE);
	    }
	}
	if (treenodep)
	{
	    if (&treenodep->pst_left != resdompp)
		opx_error(E_OP0681_UNKNOWN_NODE); /* the last node found
				    ** should be the tree node */
	}
	else
	{
	    PST_QNODE	*treenode;  /* simple aggregate - needs a 
				    ** terminator for the resdom list 
				    ** - the terminator for the main query can be used */
	    for (treenode = subquery->ops_global->ops_qheader->pst_qtree; 
		treenode && treenode->pst_sym.pst_type != PST_TREE;
		treenode = treenode->pst_left)
		; /* search for end of list */
	    *resdompp= treenode;	/* initialize simple
					** aggregate resdom list */
	}
    }

    if (subquery->ops_agg.opa_concurrent)
	subquery->ops_vmflag = FALSE;	/* some new variables may
				    ** be added to the target list
				    ** from the aggregate expression */
    for ( concurrent = subquery; 
	concurrent; 
	concurrent = concurrent->ops_agg.opa_concurrent)
    {
	PST_QNODE	*resdom;    /* resdom node associated with the
				    ** aggregate expression */
	PST_QNODE       *aop;	    /* ptr to respective PST_AOP node*/

	if (concurrent->ops_agg.opa_redundant)
	    continue;		    /* flag was set to indicate that
				    ** this subquery was identical to
				    ** another subquery - so the agg
				    ** expression does not need to be
				    ** recomputed */
	aop = concurrent->ops_agg.opa_aop; /* place AOP node into
				    ** query tree for query compilation
				    ** - joinop will ignore this 
				    ** - aop node will still have the
				    ** aggregate expression on the
				    ** right hand side */
	resdom = opv_resdom(subquery->ops_global, aop,
	    &aop->pst_sym.pst_dataval); /* assign
				    ** resdom node which will be used 
				    ** in the resdom list for the 
				    ** aggregate expression */

	resdom->pst_left = root->pst_left; /* link into list */
	root->pst_left = resdom;
	resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsflags |= PST_RS_PRINT; /* make
				    ** all resdoms printing for query
				    ** compilation */
	if (aop->pst_left
	    &&
	    (	(aop->pst_left->pst_sym.pst_type != PST_VAR)
		&&
		(aop->pst_left->pst_sym.pst_type != PST_CONST)
	    ))
	    expr_found = TRUE;	    /* aggregate expression found */
	switch (subquery->ops_sqtype)
	{

	case OPS_SAGG:
	{
				    /* removed code to save grafted constant 
				    ** in AOP's pst_right. This clobbered 2nd
				    ** operand ptr of OLAP binary aggs and we
				    ** now store simulated RQ parm no in
				    ** pst_opparm instead */
	    resdom->pst_sym.pst_value.pst_s_rsdm.pst_ntargno = 
		aop->pst_sym.pst_value.pst_s_op.pst_opparm;
				    /* save the repeat query parameter number
                                    ** for simple aggregates */
	    resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno = 
		++subquery->ops_agg.opa_attlast; /* assign unique resdom numbers for
				    ** simple aggregate expressions
				    ** - the values do not matter since
				    ** parameter constants are used for
				    ** substituting the values
				    */
	    relabel_resdom = non_printing; /* if there is at least one non-printing
				    ** resdom then relabel them to be higher
				    ** and any printing resdom for aggregates */
	    break;
	}
	case OPS_FAGG:
	case OPS_HFAGG:
	case OPS_RFAGG:
	case OPS_RSAGG:
	{
	    if (!(aop->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fiflags &
			ADI_F16_OLAPAGG))
		aop->pst_right = NULL;  /* nothing needed for function
				    ** aggregates (if not OLAP binaries) */
	    resdom->pst_sym.pst_value.pst_s_rsdm.pst_ntargno = /* this
				    ** needs to always match the pst_rsno
				    ** field  for function aggregates */
	    resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno = 
		concurrent->ops_agg.opa_atno.db_att_id;
/* this may be changed by opa_link as in b32738 
		(*concurrent->ops_agg.opa_graft)->
		pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
*/
			    /* assign the result domain number for the 
			    ** aggregate expression which is the same
			    ** as the attribute number in the 
			    ** substituted var node */
	    relabel_resdom = non_printing; /* if there is at least one non-printing
				    ** resdom then relabel them to be higher
				    ** and any printing resdom for aggregates */

	    if (aop->pst_left 
		&&
		(aop->pst_left->pst_sym.pst_dataval.db_length > 0)
		)
		width += aop->pst_left->pst_sym.pst_dataval.db_length;
	    break;
	}
	default:
	    opx_error(E_OP0281_SUBQUERY);
	}
    }
    if ((width > subquery->ops_global->ops_cb->ops_server->opg_maxtup)
	&&
	(   (subquery->ops_sqtype == OPS_FAGG)
	    ||
	    (subquery->ops_sqtype == OPS_HFAGG)
	    ||
	    (subquery->ops_sqtype == OPS_RFAGG)
	))
	subquery->ops_mask |= OPS_CAGGTUP;  /* make sure top sort node is
			    ** not used for this plan, or report error */

    if (expr_found)
	subquery->ops_mask |= OPS_AGEXPR;   /* mark that an expression
				** has been found so that top sort node 
				** removal cannot be done */
    return(relabel_resdom);
}

/*{
** Name: opa_relabel	- relabel non-printing resdoms
**
** Description:
**      Non-printing resdoms must be relabelled to be higher than all 
**      printing resdoms, since OPC assumptions are based on this 
**
** Inputs:
**      subquery                        subquery being processed
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
**      21-oct-91 (seputis)
**          initial creation as part of b40402
[@history_template@]...
*/
static VOID
opa_relabel(
	OPS_SUBQUERY       *subquery)
{	/* OPC requires all non-printing resdoms which are part of aggregates
    ** or retrieve into's to be higher in number than printing resdoms
    ** for retrieve into's this is done by default due to PSF assignment
    ** of numbers, but for aggregate temporaries this needs to be
    ** explicitly done, resdoms are added due to view semantics,
    ** tid duplicate handling semantics etc. */
    PST_QNODE		**np_resdompp;
    bool			first;

    first = TRUE;
    for (np_resdompp = &subquery->ops_root->pst_left; 
	*np_resdompp && ((*np_resdompp)->pst_sym.pst_type == PST_RESDOM); )
    {
	if (!((*np_resdompp)->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT))
	{	/* non printing resdom was found so label this and place
	    ** into the list */
	    PST_QNODE	*np_resp;
	    if (first)
		opx_error(E_OP0681_UNKNOWN_NODE); /* not expecting a non-printing
				    ** resdom to occur at this point
				    ** in the list */
	    np_resp = *np_resdompp;
	    if (subquery->ops_byexpr == np_resp)
		subquery->ops_byexpr = np_resp->pst_left;	/* since this
				    ** non-printing resdom is being relabelled
				    ** the by expression list needs to be moved
				    */
	    np_resp->pst_sym.pst_value.pst_s_rsdm.pst_ntargno = /* this
				    ** needs to always match the pst_rsno
				    ** field  for non-printing */
	    np_resp->pst_sym.pst_value.pst_s_rsdm.pst_rsno = 
		++subquery->ops_agg.opa_attlast; /* assign unique 
				    ** resdom numbers for
				    ** non-printing resdoms which are
				    ** greater than all printing resdoms
				    */
	    *np_resdompp = np_resp->pst_left;   /* remove from list
				    ** and insert at top because all
				    ** resdom numbers must be decreasing */
	    np_resp->pst_left = subquery->ops_root->pst_left;
	    subquery->ops_root->pst_left = np_resp;
	}
	else 
	    np_resdompp = &(*np_resdompp)->pst_left; /* skip over all
				    ** printing resdoms since they should be
				    ** lower in the list */
	first = FALSE;
    }
}


/*{
** Name: opa_tnode	- add TID node to target list
**
** Description:
**      This routine will create a TID var node, and RESDOM node 
**      and add it to the target list 
**
** Inputs:
**      global                          ptr to global state variable
**      rsno                            resdom number to use for PST_RESDOM
**      varno                           variable number to use for PST_VAR
**      root                            root of tree to add the resdom node
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      31-mar-89 (seputis)
**          initial creation
**	12-feb-04 (inkdo01)
**	    Change to TID8 for partitioned tables.
**	 6-mar-08 (hayke02)
**	    Init db_collID to -1. This change fixes bug 120084.
**	18-Mar-2010 (kiria01) b123438
**	    Initialise .db_data otherwise can break op170 output.
[@history_template@]...
*/
VOID
opa_tnode(
	OPS_STATE          *global,
	OPZ_IATTS          rsno,
	OPV_IGVARS	   varno,
	PST_QNODE          *root)
{
    PST_QNODE		*varp;		/* var node used to represent
					** the TID attribute */
    PST_QNODE		*resdomp;	/* resdom node used to represent
					** the TID attribute */
    DB_DATA_VALUE	datatype;	/* data type of TID */
    DB_ATT_ID		dmfattr;        /* ingres attribute number of TID */
    char		*tidname;

    dmfattr.db_att_id = DB_IMTID;	/* reserved ingres attribute number */
    datatype.db_data = NULL;		/* No data */
    datatype.db_datatype = DB_INT_TYPE;  /* data type of TID */
    datatype.db_prec = 0;		 /* no precision */
    datatype.db_length = DB_TID8_LENGTH; /* length of TID datatype */
    datatype.db_collID = -1;		 /* no collation */
    varp = opv_varnode(global, &datatype, varno, &dmfattr);
    resdomp = opv_resdom(global, varp, &datatype);
    resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags &= ~PST_RS_PRINT; /* make TID
				** resdom non printing */
    tidname = &resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsname[0];
    MEfill(sizeof(resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsname),
	' ', (PTR)tidname);	
    tidname[0] = 't';
    tidname[1] = 'i';
    tidname[2] = 'd';		/* assign TID name for dump routines */
    resdomp->pst_sym.pst_value.pst_s_rsdm.pst_ntargno =  /* this
				** needs to always match the pst_rsno
				** field  for function aggregates */
    resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsno =  rsno; /* assign
				** a unique resdom number to the TID
				** element */
    resdomp->pst_left = root->pst_left;
    root->pst_left = resdomp;   /* attach non printing resdom
				** to the target list */
}

/*{
** Name: opa_stid	- add tids to resdom list
**
** Description:
**      This routine will add TIDs to the resdom list so that the top sort 
**      node will eliminate duplicates properly,  note that the top sort 
**      node will only sort resdoms expressions and NOT the seqm
**      equivalence class map
**
** Inputs:
**      subquery                        ptr to subquery to be analyzed
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      29-jun-87 (seputis)
**          initial creation
**      16-dec-88 (seputis)
**          check for case in which duplicates do not matter
**      7-jun-89 (seputis)
**          fix union view problem
**	16-may-90 (seputis)
**	    fix aggregate handling problem such as in
**	    select * from sp_v where qty*2 = 
**	        (select sum(qty) from spj_v group by sno having sno = 4);
**	    in which the function aggregate where clause is flattened into
**	    the parent and is visible in the parent lvrm map, which would cause
**	    TIDs to be kept for the aggregate variable.
**	    - also make sure that duplicate handling is correct as in
**	    drop duplicate
**	    \p\g
**	    create table duplicate (a1 i1)
**	    \p\g
**	    insert into duplicate (a1) values (0)
**	    \p\g
**	    insert into duplicate (a1) values (0)
**	    \p\g
**	    select * from duplicate where a1 <= (select avg(reltups) from iirelation where a1 <= reltups)
**	    \p\g
**	    should return 2 tuples 
**      26-feb-91 (seputis)
**          - b35862 changed to use new range table format
**	18-mar-91 (seputis)
**	    - update tid map instead of boolean for union view duplicate
**	    handling
**      18-sep-92 (ed)
**          b44397 - correct incorrect duplicate handling bug when using
**          union views
**      16-oct-92 (ed)
**          - fix bug 40835 - wrong number of duplicates when union views and
**          subselects are used
**      8-nov-92 (ed)
**	    add union/union all fips support
**          remove #if since star defines are now visible
**	4-dec-02 (inkdo01)
**	    Changes for range table expansion.
**	16-may-05 (inkdo01)
**	    Don't set OPS_DREMOVE for SQL queries with single var keeptid.
**	30-april-2008 (dougi) Bug 120270
**	    Apply view restriction to derived tables and with clauses, too.
**	2-Jun-2009 (kschendel) b122118
**	    Use clearmask instead of copy/not/and.
*/
VOID
opa_stid(
	OPS_SUBQUERY       *subquery)
{
    /* create a resdom list for the main query which reads the tuples
    ** from the union */
    OPZ_BMATTS		attrmap;	/* bit map of available resdom numbers
                                        ** to be assigned to TID elements */
    OPZ_IATTS		nextattr;       /* next available resdom number which
                                        ** can be used for a TID resdom */
    OPV_IGVARS		gvar;		/* global range var which requires TIDs
                                        ** to be kept */
    OPS_STATE		*global;	/* global state variable */
    i4			varcount;	/* count of var refs in sq tree */
    
    global = subquery->ops_global;
    if (global->ops_union && (subquery->ops_duplicates == OPS_DKEEP))
    {	/* A union view was reference somewhere in the query, so check for
        ** existence in this subquery, and determine if duplicates need
        ** to be removed from this subquery since it contains a union
	** view, bring TIDs up for all
        ** - this only needs to be considered if duplicates are being kept
        ** in the query which uses the union view, otherwise duplicates
        ** do not matter, or are not kept anyways
	** - FIXME there is a bug here if UNION ALL views and UNION views
        ** are mixed, in that TIDs will be asked from UNION ALL
        ** views,  this situation needs to be detected and the
        ** tables which need duplicates removed need to be separated
        ** into a separate query, and then linked back to the 
        ** main query which does not have duplicates removed, i.e.
        ** a remove duplicates view needs to be generated
	*/
	OPV_IGVARS	uvar;
	PST_QNODE	*unionp;

	for (uvar = -1;
	     (uvar = BTnext((i4)uvar, 
			    (char *)&subquery->ops_root->pst_sym.
				pst_value.pst_s_root.pst_tvrm,
			    (i4)BITS_IN(OPV_GBMVARS)
			   )
	     )
	     >= 0;
	    )
	{
	    if ((   uvar 
		    < 
		    global->ops_qheader->pst_rngvar_count
		)
		&&
		(   global->ops_qheader->pst_rangetab[uvar]->pst_rgtype
		    ==
		    PST_RTREE ||
		    global->ops_qheader->pst_rangetab[uvar]->pst_rgtype
		    ==
		    PST_DRTREE ||
		    global->ops_qheader->pst_rangetab[uvar]->pst_rgtype
		    ==
		    PST_WETREE
		)
		)
	    {   /* view has been found so check for type of union
		** view , FIXME currently all nodes are UNION or
                ** UNION ALL */
		unionp = global->ops_qheader->pst_rangetab[uvar]->pst_rgroot;
		if (unionp->pst_sym.pst_value.pst_s_root.pst_union.
		    pst_next
		    &&
		    (unionp->pst_sym.pst_value.pst_s_root.pst_union.
		    pst_dups == PST_NODUPS)
		    )
		{
		    OPV_GRV	*grvp;
		    grvp = global->ops_rangetab.opv_base->opv_grv[uvar];
		    if (!grvp
			||
			!grvp->opv_gquery
			||
			grvp->opv_gquery->ops_root != unionp)
			opx_error(E_OP03A4_SUBQUERY); /* lookup 
					** subquery associated with
					** parse tree node and valid it */
		    if (grvp->opv_gquery->ops_mask & OPS_SUBVIEW)
			continue;	/* duplicates will have been removed
					** in this case */
		    if (!subquery->ops_agg.opa_tidactive)
		    {
			MEcopy((PTR)&subquery->ops_root->pst_sym.pst_value.
			    pst_s_root.pst_tvrm, 
			    sizeof(subquery->ops_agg.opa_keeptids),
			    (PTR)&subquery->ops_agg.opa_keeptids);
			subquery->ops_agg.opa_tidactive = TRUE;
		    }
		    subquery->ops_duplicates = OPS_DREMOVE; /* remove 
					** duplicates if TIDs need to
					** be brought up */
		    BTclear( (i4)uvar,
			(char *)&subquery->ops_agg.opa_keeptids);
		}
	    }
	}
    }
    if (!subquery->ops_agg.opa_tidactive)
    {
	/* in the case of a union ALL, and perhaps other cases, the
	** non printing resdoms can be removed here, but watch out for
	** the case of TIDs added for update purposes, this should make
	** the target list shorter */
	return;
    }
    if (subquery->ops_duplicates == OPS_DUNDEFINED)
    {	/* this is a case of duplicates not making a difference such as
	** min, max, any */
	subquery->ops_agg.opa_tidactive = FALSE;
	MEfill(sizeof(OPV_GBMVARS), 0, (char *)&subquery->ops_agg.opa_keeptids);
	return;
    }
    {
	OPV_GBMVARS	    globalmap;	/* get valid from list minus
					** aggregate variables */
	opv_submap( subquery, &globalmap);
	varcount = BTcount((char *)&globalmap, OPV_MAXVAR);
					/* save for later keeptids test */
/*
	globalmap &= (~(subquery->ops_aggmap & subquery->ops_agg.opa_flatten)); 
	cannot use this because the following query would return duplicates
	where it should not
create table min_spj (sno text(2), pno text(2), jno text(2), qty integer)\p\g

insert into min_spj values('s1', 'p1', 'j1', 200);
insert into min_spj values('s1', 'p1', 'j1', 200);
insert into min_spj values('s2', 'p3', 'j4', 500);
insert into min_spj values('s5', 'p4', 'j4', 800);
\p\g

set qep\p\g

select min_spj.jno from min_spj where min_spj.pno = 'p1'
group by min_spj.jno
having avg (min_spj.qty) >=
(select min(min_spj.qty)
from min_spj where min_spj.jno = 'j1')
\p\g

*/
	BTclearmask(OPV_MAXVAR, (char *)&subquery->ops_aggmap, (char *)&globalmap);
					/* remove aggregate
					** variable references; this is 
					** given in opa_aggmap which also includes
					** correlated variables, which is why
					** opa_flatten is used to remove these */
	BTand(OPV_MAXVAR, (char *)&globalmap, 
				(char *)&subquery->ops_agg.opa_keeptids);
	/* check if dups to be removed are all SQL aggregates, in which case
	** since the group by has already removed the duplicates, there is
	** not a need for a sort */
	BTxor(OPV_MAXVAR, (char *)&subquery->ops_agg.opa_keeptids,
			(char *)&globalmap); /* these are the variables
					** which need duplicates removed */
	for (gvar = -1;
	 (gvar = BTnext((i4)gvar, 
			(char *)&globalmap, 
			(i4)BITS_IN(OPV_GBMVARS)
		       )
	 )
	 >= 0;
	)
	{
	    OPV_GRV		*gvar1p;

	    gvar1p = global->ops_rangetab.opv_base->opv_grv[gvar];
	    if (gvar1p
		&&
		(   (gvar1p->opv_created == OPS_FAGG)
		    ||
		    (gvar1p->opv_created == OPS_HFAGG)
		    ||
		    (gvar1p->opv_created == OPS_RFAGG)
		)
		&&
		gvar1p->opv_gquery
		&&
		(gvar1p->opv_gquery->ops_root->pst_sym.
		    pst_value.pst_s_root.pst_qlang == DB_SQL)
		&&
		(gvar1p->opv_gquery->ops_mask & OPS_COAGG)    /* at least one attribute must be
				    ** correlated so the link-back removes duplicates */
		&&
		!gvar1p->opv_gquery->ops_agg.opa_fbylist /* if original aggregate had
				    ** a group by then duplicates need to be
				    ** removed */
		)
	    {
		continue;	    /* continue while aggregates are found
				    ** since aggregate temps have duplicates
				    ** removed on the link-back */
	    }
	    break;
	}
	if (gvar < 0)
	{   /* TIDs do not need to be kept since all duplicates
	    ** are removed by the underlying aggregates */
	    subquery->ops_agg.opa_tidactive = FALSE;
	    MEfill(sizeof(OPV_GBMVARS), 0, (char *)&subquery->ops_agg.opa_keeptids);
	    return;
	}
    }

    /* Minor heuristic - if SQL, if there's only one entry in opa_keeptids
    ** and if there is only one var referenced in sq's parse tree fragment,
    ** there's no need for DREMOVE. It prevents use of more efficient hash
    ** aggregation, so we'll get rid of it here. */
    if (BTcount((char *)&subquery->ops_agg.opa_keeptids, OPV_MAXVAR) <= 1
	&& varcount <= 1
	&& subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_qlang == DB_SQL)
    {
	subquery->ops_agg.opa_tidactive = FALSE;
	MEfill(sizeof(OPV_GBMVARS), 0, (char *)&subquery->ops_agg.opa_keeptids);
	return;
    }

    if (opa_rsmap( global, subquery->ops_root, &attrmap)) /* attrmap has 
                                        ** available resdom numbers */
	subquery->ops_dist.opd_smask |= OPD_DUPS;   /* indicate to OPC that this
				    ** subquery has non-printing resdoms which need
				    ** to be used for duplicate handling semantics */
    nextattr = 0;

    for (gvar = -1;
	 (gvar = BTnext((i4)gvar, 
			(char *)&subquery->ops_agg.opa_keeptids, 
			(i4)BITS_IN(OPV_GBMVARS)
		       )
	 )
	 >= 0;
	)
    {	/* FIXME - check if tid is not already in target list */
        OPV_GRV		*gvarp;

	gvarp = global->ops_rangetab.opv_base->opv_grv[gvar];
	if (	!(global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
		&&
		(gvarp->opv_qrt != OPV_NOGVAR)
		&&
		(global->ops_qheader->pst_rangetab[gvarp->opv_qrt]->pst_rgtype
		==
		PST_DRTREE || 
		global->ops_qheader->pst_rangetab[gvarp->opv_qrt]->pst_rgtype
		==
		PST_WETREE || 
		global->ops_qheader->pst_rangetab[gvarp->opv_qrt]->pst_rgtype
		==
		PST_RTREE)	    /* cannot ask for TIDs from range table
                                    ** entries which have query trees 
                                    ** e.g. views
				    ** - however psuedo-tid using new ADF
				    ** increment function can be used for
				    ** duplicate behaviour */
	    )
	{
	    subquery->ops_mask |= OPS_TIDVIEW;	/* mark subquery has having
				    ** a TID attribute in the target list
				    ** which is on a view,... indicating
				    ** a special ADF function needs to be
				    ** called to initialize this attribute */
	}
	else if ((gvarp->opv_gsubselect) /* cannot ask for a TID from a virtual
                                    ** table */
	    ||
   	    (
   		(global->ops_cb->ops_server->opg_qeftype == OPF_GQEF)
   		&&
   		gvarp->opv_relation
   		&&
   		(gvarp->opv_relation->rdr_rel->tbl_status_mask & DMT_GATEWAY) 
				    /* these are gateway tables which 
   				    ** require duplicates
   				    ** to be kept but do not have TIDs */
   	    )
	    ||
	    (	/* check if links to views are being used */
		(global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
		&&
   		gvarp->opv_relation
   		&&
		(   (gvarp->opv_relation->rdr_obj_desc->dd_o6_objtype == DD_1OBJ_LINK) 
		    ||
		    (gvarp->opv_relation->rdr_obj_desc->dd_o6_objtype == DD_2OBJ_TABLE) 
				    /* this table possibily does have TIDs
				    ** if the LDB is a view */
		)
		&&
		(gvarp->opv_relation->rdr_obj_desc->dd_o9_tab_info.dd_t3_tab_type == DD_3OBJ_VIEW) 
	    )
	   )
	{
	    /* this relation does not have TIDs so place it in the
            ** bitmap of relations with no TIDs */
	    if (!subquery->ops_keep_dups)
	    {
		subquery->ops_keep_dups = (OPV_BMVARS *)opu_memory(global,
		    (i4)sizeof(*subquery->ops_keep_dups));
		MEfill(sizeof(*subquery->ops_keep_dups), (u_char)0,
		    (PTR)subquery->ops_keep_dups);
	    }
	    BTset((i4)gvar, (char *) subquery->ops_keep_dups);
	    continue;
	}
	BTset((i4)gvar, (char *)&subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_lvrm);
				    /* update the left var map to
				    ** include the vars which are referenced
                                    ** in the TID list */
	nextattr = BTnext(  (i4)nextattr, (char *)&attrmap, 
			    (i4)BITS_IN(attrmap) ); /* check if
				    ** too many attributes are being
				    ** defined */
	if (nextattr <=0 )
	    opx_error(E_OP0201_ATTROVERFLOW); /* too many attributes
				    ** are being defined */
	opa_tnode(global, nextattr, gvar, subquery->ops_root);
	subquery->ops_dist.opd_smask |= OPD_DUPS;   /* indicate to OPC that this
				    ** subquery has non-printing resdoms which need
				    ** to be used for duplicate handling semantics */
    }
    if (subquery->ops_keep_dups)
        subquery->ops_dist.opd_smask &= (~OPD_DUPS); /* if special view handling is
                                    ** needed so that a sort cannot be performed
                                    ** at the top node then turn off flag to avoid
                                    ** removing duplicates */
    else
        subquery->ops_duplicates = OPS_DREMOVE; /* remove duplicates if TIDs need to
				    ** be brought up */
}
#if 0

/*{
** Name: opa_duplicate	- determine duplicate handling for aggregate
**
** Description:
**      This routine will determine the duplicate handling for the aggregate
**      evaluation.  The list of subqueries to be evaluated together is
**      traversed and the duplicate semantics of each is analyzed.  If
**      at least one element in the list is either OPS_DKEEP or OPS_DREMOVE
**      then the search is ended and the respective duplicate handling
**      is selected.  If all the elements in the list are OPS_DUNDEFINED then
**      this mode is selected, since then "opo_existence" or the existence_only
**      check can be made, but a sort node does not need to be placed on top
**      of the query plan to keep the duplicate semantics.
**
** Inputs:
**      subquery                        aggregate to be calculated
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
[@history_template@]...
*/
static OPS_DUPLICATES
opa_duplicate(
	OPS_SUBQUERY       *subquery)
{
    for( ; subquery ; subquery = subquery->ops_agg.opa_concurrent)
    {
        switch( subquery->ops_agg.opa_aop->pst_sym.pst_value.pst_s_op.
	    pst_distinct)
	{   /* note that exit can be made on PST_DISTINCT and PST_NDISTINCT
	    ** since the aggregates would not be compatible if both existed
            ** in this list */
	case PST_DISTINCT:
	    return(OPS_DREMOVE);
	case PST_DONTCARE:
	    break;
	case PST_NDISTINCT:
	    return(OPS_DKEEP);
	default:
	    opx_error(E_OP0095_PARSE_TREE);
	}
    }
    return(OPS_DUNDEFINED);
}
#endif

/*{
** Name: opa_icount	- insert count aggregate test
**
** Description:
**      The routine will place a test for count(*)!=0 in the
**	query tree at the given node 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      qualpp                          insertion point of test
**      countp                          ptr to count(*) information
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
**      22-oct-91 (seputis)
**          initial creation for bug 40402
**      18-sep-92 (ed)
**          change parameter to opa_resolve from to subquery
[@history_template@]...
*/
static VOID
opa_icount(
	OPS_SUBQUERY       *subquery,
	PST_QNODE          **qualpp,
	PST_QNODE          *countp)
{
    PST_QNODE	    *andp;
    PST_QNODE	    *equalp;
    OPS_STATE	    *global;
    DB_ATT_ID	    atno;
    global = subquery->ops_global;
    andp = opv_opnode(global, PST_AND, (ADI_OP_ID)0, (OPL_IOUTER)PST_NOJOIN);
    andp->pst_left = *qualpp;
    *qualpp = andp;
    andp->pst_right =
    equalp = opv_opnode(global, PST_BOP,
	global->ops_cb->ops_server->opg_ne, (OPL_IOUTER)PST_NOJOIN);
    atno.db_att_id = countp->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
    equalp->pst_right = opv_varnode( global, &countp->pst_sym.pst_dataval, 
	    (OPV_IGVARS)subquery->ops_gentry, &atno);
    equalp->pst_left = opv_intconst(global, 0);
    opa_resolve(subquery, equalp);
}

/*{
** Name: opa_forqual	- function aggregrate insertion into qual
**
** Description:
**      Insert the count(*)!=0 function aggregate at points in the
**	query which are below OR's.  Function aggregates should not
**	return tuples when no tuples exist in the partition, but
**	the current transformations which tries to preserve equi-joins
**	uses a projection action so that no tuples are lost in the
**	link back, until the aggregate qualification is evaluated.
**	This results in extra tuples in the temporary relation which
**	should be eliminated after the link-back occurs.  This is
**	done by inserting a ANDed "count(*)!=0" test to any test
**	which references an aggregate result below an OR.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      qual                            ptr to qual being modified
**      countp                          ptr to count aggregate resdom
**
** Outputs:
**
**	Returns:
**	    TRUE - if the qual reserves an attribute of the aggregate.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      22-oct-91 (seputis)
**          initial creation as part of bug 40402 fix
**	15-may-92 (seputis)
**	    - fix bug 44267, used structure instead of pointer for countp
[@history_template@]...
*/
static bool
opa_forqual(
	OPS_SUBQUERY       *subquery,
	PST_QNODE          **qualpp,
	PST_QNODE          *countp)
{
    for(;(*qualpp); qualpp = &(*qualpp)->pst_right)
    {
	if (((*qualpp)->pst_sym.pst_type == PST_OR)
	    ||
	    ((*qualpp)->pst_sym.pst_type == PST_AND))
	{   /* if either branch of the qualification references an
	    ** attribute of the temporary, then the count(*)!=0 test
	    ** must be used */
	    if (opa_forqual(subquery, &(*qualpp)->pst_left, countp))
		opa_icount(subquery, &(*qualpp)->pst_left, countp);
	    if (opa_forqual(subquery, &(*qualpp)->pst_right, countp))
		opa_icount(subquery, &(*qualpp)->pst_right, countp);
	    return(FALSE);
	}
	if (((*qualpp)->pst_sym.pst_type == PST_VAR)
	    &&
	    (	(*qualpp)->pst_sym.pst_value.pst_s_var.pst_vno
		==
		subquery->ops_gentry	    /* check if var node is
					    ** associated with temporary */
	    ))
	    return(TRUE);   /* check if the aggregate result is being used
			    ** in the parent query */
	    
	if (opa_forqual(subquery, &(*qualpp)->pst_left, countp))
	    return(TRUE);
    }
    return(FALSE);
}

/*{
** Name: opa_fqual	- function aggregrate insertion into qual
**
** Description:
**      Insert the count(*)!=0 function aggregate at points in the
**	query which are below OR's.  Function aggregates should not
**	return tuples when no tuples exist in the partition, but
**	the current transformations which tries to preserve equi-joins
**	uses a projection action so that no tuples are lost in the
**	link back, until the aggregate qualification is evaluated.
**	This results in extra tuples in the temporary relation which
**	should be eliminated after the link-back occurs.  This is
**	done by inserting a ANDed "count(*)!=0" test to any test
**	which references an aggregate result below an OR.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      qual                            ptr to qual being modified
**      countp                          ptr to count aggregate resdom
**
** Outputs:
**
**	Returns:
**	    TRUE - if the qual reserves an attribute of the aggregate.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      22-oct-91 (seputis)
**          initial creation as part of bug 40402 fix
**	15-may-92 (seputis)
**	    - fix bug 44267, used structure instead of pointer for countp
[@history_template@]...
*/
static VOID
opa_fqual(
	OPS_SUBQUERY       *subquery,
	PST_QNODE          **qualpp,
	PST_QNODE          *countp)
{
    for(;(*qualpp); qualpp = &(*qualpp)->pst_right)
    {
	if ((*qualpp)->pst_sym.pst_type == PST_OR)
	{
	    (VOID)opa_forqual(subquery, qualpp, countp);
	    return;
	}
	opa_fqual(subquery, &(*qualpp)->pst_left, countp);
    }
}

/*{
** Name: opa_recorelated	- remove corelated equi-joins
**
** Description:
**      The routine will remove any corelated equi-join and calculates 
**      the variable map which remains after the corelated variables
**	have been removed. 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**
** Outputs:
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    remove variables from subquery from which values can be obtained
**	    from non-corelated variables
**
** History:
**      20-apr-92 (seputis)
**          initial creation for b44012 - variable elimination optimization
**	18-sep-92 (ed)
**	    bug 44801 - corrected node traversal algorithm which skipped certain
**	    orientations of the query tree, resulting in incorrect decisions to
**	    eliminate variables.
**      19-nov-92 (pearl)
**          Bug 47541 - when substituting a variable for an equi-join, make sure
**          to copy over the DB_DATA_VALUE; the type etc. may not be the same.
**      5-jan-92 (ed)
**          bug 47912 - check for mismatched datatypes and mark subquery to
**          indicate that var nodes need to be converted on the next pass of
**          the parent where clause
**	26-oct-93 (ed)
**	    - bug 54619, check for transitive joins which should not be
**	    substituted.
**	17-may-05 (inkdo01)
**	    Set back to OPS_DKEEP if equijoin removed and leaves SQL
**	    single variable parse tree.
[@history_template@]...
*/
static VOID
opa_recorelated(
	OPS_SUBQUERY       *subquery)
{
    PST_QNODE           *qnode;		/* dereference ptr to ptr */
    PST_QNODE		**qnodepp;	/* used to traverse query tree and
					** modify when necessary */
    OPV_GBMVARS		qualmap;	/* map of variables, not including the
					** the conjuncts in the equijoinp list */
    PST_QNODE		*equijoinp;	/* list of corelated equijoin conjuncts */
    OPV_IGVARS		maxvar;		/* number of valid bits in global variable
					** map */

    maxvar = subquery->ops_global->ops_rangetab.opv_gv;
    qnodepp = &subquery->ops_root->pst_right;
    equijoinp = NULL;
    MEfill(sizeof(qualmap), 0, (PTR)&qualmap);
    qnode = *qnodepp;
    if (qnode->pst_sym.pst_type == PST_BOP)
    {	/* create PST_AND node if single qualification */
	qnode = opv_opnode(subquery->ops_global, PST_AND, (ADI_OP_ID)0, (OPL_IOUTER)PST_NOJOIN);
	qnode->pst_left = *qnodepp;
	qnode->pst_right = opv_qlend(subquery->ops_global);
	*qnodepp = qnode;
    }
    while ((*qnodepp)
	    &&
	    ((*qnodepp)->pst_sym.pst_type != PST_QLEND)
	)
    {	/* look at all conjuncts in the qualification */
	while(((*qnodepp)->pst_sym.pst_type == PST_AND)
	    &&
	    (*qnodepp)->pst_left
	    &&
	    ((*qnodepp)->pst_left->pst_sym.pst_type == PST_AND)
	    )
	{   /* move all PST_AND nodes to right side of tree */
	    qnode = (*qnodepp)->pst_right;
	    (*qnodepp)->pst_right = (*qnodepp)->pst_left;
	    (*qnodepp)->pst_left = (*qnodepp)->pst_right->pst_right;
	    (*qnodepp)->pst_right->pst_right = qnode;
	    if (!((*qnodepp)->pst_left))
		*qnodepp = (*qnodepp)->pst_right; /* eliminate empty 
					    ** qualification */
	}
	qnode = *qnodepp;
	if (qnode
	    &&
	    (qnode->pst_sym.pst_type != PST_QLEND)
	    &&
	    (qnode->pst_sym.pst_type != PST_AND)
	    )
	{   /* create PST_AND node, if last node in the chain so that
	    ** all qualifications are scanned */
	    qnode = opv_opnode(subquery->ops_global, PST_AND, (ADI_OP_ID)0,  (OPL_IOUTER)PST_NOJOIN);
	    qnode->pst_left = *qnodepp;
	    qnode->pst_right = opv_qlend(subquery->ops_global);
	    *qnodepp = qnode;
	}
	if ( qnode->pst_sym.pst_type == PST_AND)
	{
	    qnode = (*qnodepp)->pst_left;
	    if ((qnode->pst_sym.pst_type == PST_BOP)
		&&
		(qnode->pst_sym.pst_value.pst_s_op.pst_opno 
		==
		subquery->ops_global->ops_cb->ops_server->opg_eq)
		)
	    {   /* possible equi-join */
		if ((qnode->pst_left->pst_sym.pst_type == PST_VAR)
		    &&
		    (qnode->pst_right->pst_sym.pst_type == PST_VAR)
		    )
		{	/* join between 2 variables */
		    OPV_IGVARS	leftvar;
		    OPV_IGVARS	rightvar;
		    bool	found;
		    found = FALSE;
		    leftvar = qnode->pst_left->pst_sym.pst_value.pst_s_var.pst_vno;
		    rightvar = qnode->pst_right->pst_sym.pst_value.pst_s_var.pst_vno;
		    if (BTtest((i4)leftvar, (char *)&subquery->ops_fcorelated))
		    {
			if (!BTtest((i4)rightvar, (char *)&subquery->ops_fcorelated))
			    found = TRUE;
		    }
		    else if (BTtest((i4)rightvar, (char *)&subquery->ops_fcorelated))
		    {
			if (!BTtest((i4)leftvar, (char *)&subquery->ops_fcorelated))
			{
			    PST_QNODE	*qnodep1;   /* have corelated variable on
						** right side of expression */
			    qnodep1 = qnode->pst_left;
			    qnode->pst_left = qnode->pst_right;
			    qnode->pst_right = qnodep1;
			    found = TRUE;
			}
		    }
		    if (found)
		    {   /* remove corelated equi-join from the qualification list */
                        DB_DATA_VALUE   *leftp;
                        DB_DATA_VALUE   *rightp;
			OPV_IVARS	var3;	/* var number of correlated attribute */
			OPZ_IATTS	attr3;	/* attribute number of correlated attr */
			PST_QNODE	*qnodep3; /* current equi-node being analyzed */

			/* make sure correlated attribute is not already in list
			** to avoid eliminating an implicit join 
			** i.e. r.a = s.a and r.a=s.b, then if r.a is
			** corelated it should not be eliminated bug 54619 */
			var3 = qnode->pst_left->pst_sym.pst_value.pst_s_var.pst_vno;
			attr3 = qnode->pst_left->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
			for (qnodep3 = equijoinp; qnodep3; qnodep3 = qnodep3->pst_right)
			{
			    if ((   qnodep3->pst_left->pst_left->pst_sym.pst_value.pst_s_var.
					pst_vno 
				    == 
				    var3
				)
				&&
				(   qnodep3->pst_left->pst_left->pst_sym.pst_value.pst_s_var.
					pst_atno.db_att_id
				    == 
				    attr3
				))
				break;
			}
			if (!qnodep3)
			{
			    leftp = &qnode->pst_left->pst_sym.pst_dataval;
			    rightp = &qnode->pst_right->pst_sym.pst_dataval;
			    if ((rightp->db_datatype != leftp->db_datatype)
				||
				(rightp->db_length != leftp->db_length)
				/*  || (rightp->db_prec != leftp->db_prec) */
				)
				subquery->ops_mask |= OPS_VOPT; /* this causes a
							** traversal of usages of the
							** attribute in the parents in
							** order to make the datatypes
							** compatible with the resdom
							** change made below */
			    qnode = *qnodepp;
			    *qnodepp = (*qnodepp)->pst_right;
			    qnode->pst_right = equijoinp;
			    equijoinp = qnode;	    /* place into equijoin list */
			    continue;
			}
		    }
		}
	    }
	}
	opv_mapvar((*qnodepp)->pst_left, &qualmap);
	qnodepp = &(*qnodepp)->pst_right;
    }
    for (qnode = subquery->ops_root->pst_left;
	qnode && (qnode->pst_sym.pst_type == PST_RESDOM);
	qnode = qnode->pst_left)
    {	/* there may also be corelated attributes in the aggregate expression
	** so include all aggregate expressions in the map */
	if (qnode->pst_right->pst_sym.pst_type != PST_VAR)
	    opv_mapvar(qnode->pst_right, &qualmap);
    }
    BTnot((i4)maxvar, (char *)&qualmap);
    BTand((i4)maxvar, (char *)&subquery->ops_fcorelated, (char *)&qualmap);
    for (qnodepp = &equijoinp; *qnodepp; )
    {
	if (BTtest((i4)(*qnodepp)->pst_left->pst_left->pst_sym.pst_value.pst_s_var.pst_vno,
	    (char *)&qualmap))
	    qnodepp = &(*qnodepp)->pst_right;	/* variable to be removed, so keep
					    ** equi-join around for renaming resdoms */
	else
	{   /* replace equi-join into list since the variable is not being removed */
	    qnode = (*qnodepp)->pst_right;
	    (*qnodepp)->pst_right = subquery->ops_root->pst_right;
	    subquery->ops_root->pst_right = *qnodepp;
	    *qnodepp = qnode;
	}
    }
    if (!equijoinp)
	return;				    /* return if no variables to substitute */
    for (qnode = subquery->ops_root->pst_left;
	qnode && (qnode->pst_sym.pst_type == PST_RESDOM);
	qnode = qnode->pst_left)
    {
	OPV_IGVARS	vno2;		    /* variable being substituted */

	vno2 = qnode->pst_right->pst_sym.pst_value.pst_s_var.pst_vno;
	if ((qnode->pst_right->pst_sym.pst_type == PST_VAR)
	    &&
	    BTtest((i4)vno2, (char *)&qualmap)
	    )
	{   /* found var to substitute so search for non-corelated match */
	    PST_QNODE	    *qnodep2;
	    OPZ_IATTS	    attr;
	    attr = qnode->pst_right->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
	    for (qnodep2 = equijoinp; qnodep2; qnodep2 = qnodep2->pst_right)
	    {
		if ((vno2 == qnodep2->pst_left->pst_left->pst_sym.pst_value.pst_s_var.pst_vno)
		    &&
		    (attr == qnodep2->pst_left->pst_left->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
		    )
		{   /* found equi-join, so substitute variable */
		    qnode->pst_right->pst_sym.pst_value.pst_s_var.pst_vno =
			qnodep2->pst_left->pst_right->pst_sym.pst_value.pst_s_var.pst_vno;
		    qnode->pst_right->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id =
			qnodep2->pst_left->pst_right->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
                    /* bug 47541 */
                    STRUCT_ASSIGN_MACRO (
                        qnodep2->pst_left->pst_right->pst_sym.pst_dataval,
                        qnode->pst_right->pst_sym.pst_dataval);
                    STRUCT_ASSIGN_MACRO (
                        qnodep2->pst_left->pst_right->pst_sym.pst_dataval,
                        qnode->pst_sym.pst_dataval);    /* assign datatype to resdom node,
                                                        ** to avoid overflows, NULL incompatibility
                                                        ** note that if this actually causes
                                                        ** a change, OPS_VOPT will be set, which
                                                        ** causes usages of this attribute
                                                        ** to be updated also */
		    break;
		}
	    }
	    if (!qnodep2)
		opx_error(E_OP028A_MISSING_CORELATION);	/* expecting to find a match */
	}
    }
    BTnot((i4)maxvar, (char *)&qualmap);
    BTand((i4)maxvar, (char *)&qualmap, (char *)&subquery->ops_fcorelated);
    BTand((i4)maxvar, (char *)&qualmap, (char *)&subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm);
    subquery->ops_vmflag = FALSE;
    opv_smap(subquery);					/* remap variables since variable subsitution has
							** occurred */
    {
	OPV_GBMVARS	    globalmap;	/* get valid from list minus
					** aggregate variables */
	opv_submap( subquery, &globalmap);
	if (subquery->ops_duplicates == OPS_DREMOVE &&
	    subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_qlang == DB_SQL
	    && BTcount((char *)&globalmap, OPV_MAXVAR) <= 1)
	{
	    subquery->ops_duplicates = OPS_DKEEP;
	}
	BTnot((i4)maxvar, (char *)&qualmap);
	BTand((i4)maxvar, (char *)&qualmap, (char *)&globalmap);
	if (BTnext((i4)-1, (char *)&globalmap, (i4)maxvar)>=0)
	    opx_error(E_OP028B_VAR_ELIMINATION);	/* variable should have been eliminated */
    }
}

/*{
** Name: opa_final	- final pass of the aggregate processor
**
** Description:
**      This pass of the aggregate processor will convert the queries into
**	a form understood by the joinop phase.  This includes
**          1) initialize target list for aggregate subqueries
**          2) creating a subquery to project the by-domains if necessary
**
** Inputs:
**      global                          ptr to global state variable
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      28-jul-86 (seputis)
**          initial creation
**      27-sep-88 (seputis)
**          do not ignore non-printing resdoms in simple aggregates
**	17-apr-89 (jrb)
**	    zeroed out precision field of db_data_value for db_int_type.
**      16-mar-89 (seputis)
**          save the projection subquery ptr in the global range var
**      28-may-90 (seputis)
**          fix duplicate handling problem
**      23-aug-90 (seputis)
**          - fix b32738, opa_graft was changed by opa_optimize
**      12-mar-91 (seputis)
**          - any correlated subquery should go thru enumeration
**	    since DIO/CPU estimates should be correctly weighted for
**	    each tuple substitution
**      6-apr-1992 (bryanp)
**          Add support for PSQ_DGTT_AS_SELECT for temporary tables.
**      18-sep-92 (ed)
**          - b44850 - pass new parameter to opv_agrv
**      5-jul-95 (inkdo01)
**          add logic to assure BY-list cols precede all others in 
**          RESDOM list for BY projection
**	8-feb-99 (wanfr01)
** 	    Fix for bug 83929:   
**          Modified fix for bug 64915 so only printed RESDOMs are shifted.
**	10-dec-00 (inkdo01)
**	    Added code to detect OPS_FAGG's which can be handled as RFAGG's
**	    (thus removing need for temp table)
**	16-jan-06 (hayke02)
**	    Correct code which adds the count(*) to the group by list for
**	    projections. This change fixes bug 113941.
**	4-dec-02 (inkdo01)
**	    Changes for range table expansion.
**	18-Mar-2007 (kschendel) b122118
**	    The FAGG replacement was testing for subquery ops-gentry > 0,
**	    which I think was a typo.  Make it >= 0, so that we don't
**	    use OPS_FAGG unnecessarily (shows up in an aggregate select
**	    from a simple view of a base table).
**	6-feb-2008 (dougi)
**	    Augment Karl's HAGGF changes with fix to convert another FAGG
**	    to RFAGG.
*/
VOID
opa_final(
	OPS_STATE          *global)
{
    OPS_SUBQUERY        **previous;

for (previous = &global->ops_subquery; 
     (*previous)->ops_sqtype != OPS_MAIN;   /* exit if the main query is found*/
     previous = &((*previous)->ops_next))
    {
	OPS_SUBQUERY        *subquery;
	PST_QNODE	    *root;	    /* ptr to root node of subquery to
                                            ** be optimized */
	
	subquery = *previous;		    /* ptr to current subquery being
					    ** analyzed */
        root = subquery->ops_root;          /* get ptr to root of query tree */
	/* this is not the main query, since it is not the last 
	** one on the list */

        if (subquery->ops_global->ops_qheader->pst_subintree
	    &&
	    (global->ops_gmask & OPS_QUERYFLATTEN))
	    /* corelated variables only occur with new scopes introduced
            ** by subselects */
	    opa_subselect(subquery, &subquery->ops_root); /*
					    ** replace corelated PST_VAR
                                            ** nodes with repeat query
                                            ** parameters */
	if (BTcount((char *)&subquery->ops_correlated, OPV_MAXVAR))
	    subquery->ops_enum = TRUE;	    /* any correlated subselect
					    ** may be executed several times
					    ** and DIO/CPU cost estimates
					    ** are important to indicate the
					    ** cost of each rescan */
	switch (subquery->ops_sqtype)
	{

	case OPS_SELECT:
	case OPS_UNION:
	case OPS_VIEW:
	{
	    subquery->ops_mode = PSQ_RETRIEVE;
	    break;			    /* go to next subquery
                                            ** since remainder of
                                            ** code in this loop
                                            ** deals with aggregates only
                                            */
	}
	case OPS_RSAGG:
	case OPS_SAGG:
	{

	    /* execute following only for aggregates */
/* 	    subquery->ops_duplicates = opa_duplicate(subquery); */
            subquery->ops_mode = PSQ_RETRIEVE;  /* simple aggregates do not
                                                ** create a temporary relation
                                                ** so use retrieve mode */
	    if (opa_resdom(subquery, root))	/* process resdom list */
		opa_relabel(subquery);		/* relabel non-printing resdoms */
	    break;
	}
	case OPS_HFAGG:
	case OPS_RFAGG:
	case OPS_FAGG:
	{
	    bool    relabel;
	    /* function aggregate subquery */
	    /* execute following only for aggregates */
/* 	    subquery->ops_duplicates = opa_duplicate(subquery);*/

#if 0
 this may have been changed by opa_link, and should already be
 set; check b32738 
            subquery->ops_gentry = (*subquery->ops_agg.opa_graft)->
		pst_sym.pst_value.pst_s_var.pst_vno; /* get result relation
                                            ** global var number */
#endif
	    relabel = opa_resdom(subquery, root);   /* process resdom list */
	    if (subquery == global->ops_subquery && subquery->ops_next && 
		(subquery->ops_next->ops_sqtype == OPS_MAIN ||
		 subquery->ops_next->ops_sqtype == OPS_SAGG &&
		 subquery->ops_next->ops_next &&
		 subquery->ops_next->ops_next->ops_sqtype == OPS_MAIN) &&
		subquery->ops_gentry >= 0 && !subquery->ops_agg.opa_projection)
	    {
		OPV_GRV	*grvp;

		grvp = global->ops_rangetab.opv_base->opv_grv[subquery->ops_gentry];
		grvp->opv_created = OPS_RFAGG;

		opa_gsub(subquery);
		subquery->ops_sqtype = OPS_RFAGG;
	    }
	    if (BTcount((char *)&subquery->ops_correlated, OPV_MAXVAR)
		||
		(subquery->ops_sqtype == OPS_HFAGG)
		||
		(subquery->ops_sqtype == OPS_RFAGG))
                subquery->ops_mode = PSQ_RETRIEVE;  /* function aggregates
					    ** which are corelated are
					    ** computed as needed and no
					    ** temporary is used */
	    else
	    {
		subquery->ops_result = subquery->ops_gentry;
                subquery->ops_mode = PSQ_RETINTO;  /* if function aggregate
					    ** is not corelated then a temporary
                                            ** is produced */
	    }
	    if (subquery->ops_agg.opa_fbylist
		&&
		subquery->ops_agg.opa_projection)
	    {
		/* since a projection action is required, but corelated
		** group by aggregates eliminate partitions which do not
		** have any tuples included, a count(*) aggregate needs to
		** be added to the group by list, so that it can be used
		** in the qualification to eliminate tuples in which the
		** count(*)==0, part of 40402 fix  */
		ADI_OP_ID	scount;	    /* ADF operator for count(*) */
		PST_QNODE	**resdompp;    /* used to traverse target list */
		PST_QNODE	*countp;    /* count (*) aggregate resdom ptr
					    ** to be used for != 0 test */
		OPS_SUBQUERY	*fsubquery; /* ptr to subquery being modified
					    ** with count(*)!=0 test */
		countp = NULL;
		scount = global->ops_cb->ops_server->opg_scount;
		for (resdompp = &root->pst_left; 
		    *resdompp && ((*resdompp)->pst_sym.pst_type == PST_RESDOM);
		    resdompp = &(*resdompp)->pst_left)
		{
		    if ((*resdompp)->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT)
		    {
			if (((*resdompp)->pst_right->pst_sym.pst_type == PST_AOP)
			    &&
			    ((*resdompp)->pst_right->pst_sym.pst_value.
				pst_s_op.pst_opno == scount)
			    )
			{
			    countp = *resdompp;	/* found a count(*) aggregate
					    ** so one does not need to be created */
			    break;
			}
		    }
		}
		if (!countp)
		{   /* count(*) aggregate not found so create a new aggregate
		    ** node at the insertion point */
		    PST_QNODE		*aop;		/* var node used to represent
							** the TID attribute */
		    DB_ANYTYPE		*junk;		/* used to obtain size of i4 */

		    aop = opv_opnode(global, PST_AOP, scount, (OPL_IOUTER)PST_NOJOIN);
		    aop->pst_sym.pst_dataval.db_datatype = DB_INT_TYPE;  /* data type of 
						** count(*) */
		    aop->pst_sym.pst_dataval.db_length = sizeof(junk->db_i4type); 
						/* length of i4 datatype */
		    opa_resolve(subquery, aop);
		    countp = opv_resdom(global, aop, &aop->pst_sym.pst_dataval);
		    countp->pst_sym.pst_value.pst_s_rsdm.pst_ntargno =  /* this
						** needs to always match the pst_rsno
						** field  for function aggregates */
		    countp->pst_sym.pst_value.pst_s_rsdm.pst_rsno = 
			++subquery->ops_agg.opa_attlast; /* assign
						** a unique resdom number to the count(*)
						** element */
		    countp->pst_left = root->pst_left; /* insert into target
						** list */
		    root->pst_left = countp;
		}
		for (fsubquery = subquery; fsubquery; 
		    fsubquery=fsubquery->ops_agg.opa_concurrent)
		{   /* traverse the parent subqueries and modify any qualification
		    ** which references an attribute of this temporary table below
		    ** an OR qualification */
		    OPS_SUBQUERY    *f1subquery;
		    for (f1subquery = subquery; 
			f1subquery->ops_agg.opa_father != fsubquery->ops_agg.opa_father;
			f1subquery = f1subquery->ops_agg.opa_concurrent)
			;		    /* modify the parent subquery only
					    ** once, if several aggregates point
					    ** to the same parent */
		    if (fsubquery->ops_agg.opa_father
			&&
			(f1subquery == fsubquery))
			opa_fqual(subquery, &fsubquery->ops_agg.opa_father->
			    ops_root->pst_right, countp);
		    fsubquery->ops_agg.opa_father->ops_normal = FALSE; /* query
					    ** tree was modified so it is not
					    ** normalized anymore */
		}
	    }
	    if (relabel)
		opa_relabel(subquery);		/* relabel non-printing resdoms */
            subquery->ops_enum = TRUE;      /* need estimates of intermediate
                                            ** results; histograms, and tuple
                                            ** count */
            if (subquery->ops_agg.opa_projection
                &&
                !(subquery->ops_mask & OPS_FLSQL)) /* do not create projection
                                            ** if aggregate flattening
                                            ** is enabled */
	    {	/* create a query tree that will get the projection of the
		** bylist values - add sort list for evaluation FIXME */
		PST_QNODE          *qroot;  /* root of query tree representing
					    ** the projected by domains */
		PST_QNODE          *qual;   /* save right part of query tree */
		OPS_SUBQUERY       *project;/* subquery created to implement
					    ** the projection of the aggregate
					    ** by list */
		PST_QNODE	    *target; /* target list may contain non-
					    ** printing resdoms */

		qroot = root;		    /* get root of aggregate query tree
					    */
		target = qroot->pst_left;   /* save target list */
#if 0
		if (subquery->ops_agg.opa_fbylist)
		{
		    /* remove variables for which projection tuples
		    ** should not be added */
		    qroot->pst_left = subquery->ops_agg.opa_fbylist; 
		    /* since a projection action is required, but corelated
		    ** group by aggregates eliminate partitions which do not
		    ** have any tuples included, a count(*) aggregate needs to
		    ** be added to the group by list */
		}
		else
#endif
		    qroot->pst_left = subquery->ops_byexpr; /* insert by-list only */
		qual = qroot->pst_right;    /* save right part of tree */
		qroot->pst_right = NULL;    /* project bydoms without
					    ** qualification */
		opv_copytree(global, &qroot);/* copy the query tree containing
					    ** the bylist only */
		root->pst_left = target;    /* restore target list */
		root->pst_right = qual;	    /* restore the 
					    ** qualification*/
		project = opa_initsq(global, &qroot, (OPS_SUBQUERY *)NULL); /*
					    ** create a new subquery */
                project->ops_sqtype = OPS_PROJECTION;
		project->ops_duplicates = OPS_DREMOVE; /* remove duplicates for
					    ** a projection */
		MEfill(sizeof(PST_J_MASK), 0, (char *)&project->ops_root->
			pst_sym.pst_value.pst_s_root.pst_tvrm);
					    /* delete the from list from the
                                            ** aggregate only variables visible
                                            ** in the by list should be used for
                                            ** the projection */
		project->ops_root->pst_right = opv_qlend(global);
                
		project->ops_width = opv_tuplesize(global, root); /* 
                                            ** result width of bylist only */
		project->ops_next = subquery;
		*previous = project;        /* link into subquery list */
                previous = &project->ops_next; /* look at current subquery */

		subquery->ops_projection = 
		project->ops_gentry =
		project->ops_result = opv_agrv(global, 
		    (DB_TAB_NAME *)NULL,    /* NULL means create a temp table*/
		    (DB_OWN_NAME *)NULL, 
		    (DB_TAB_ID *)NULL,
		    OPS_PROJECTION,         /* projection global range var*/
		    TRUE,		    /* TRUE - means abort if an error */
					    /* create global range variable for
					    ** temporary aggregate projection
					    ** result */
		    (OPV_GBMVARS *)NULL,
		    OPV_NOGVAR);
		global->ops_rangetab.opv_base->opv_grv[project->ops_result]
		    ->opv_gquery = project; /* save the subquery pointer
					    ** in the projection global range
					    ** variable */
		if (subquery->ops_sunion.opu_mask & OPU_REFUNION)  /* remember that a union
                                    ** view was found in this subquery */
		    project->ops_sunion.opu_mask |= OPU_REFUNION;
	    }
	    else if ((subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_qlang == DB_SQL)
#ifdef OPM_FLATAGGON
		    &&
		    (	!global->ops_cb->ops_check 
			|| 
			!opt_strace( global->ops_cb, OPT_F068_OLDFLATTEN)
		    )
#endif
		    )
	    {	/* eliminate corelated variables which are only involved in equi-joins
		** to non-corelated variables, unless the tuplecount of the corelated variable
		** are relatively small to the non-correlated variable */
		/* projection actions require that the corelated variable remain since
		** empty partitions of the correlated variable are significant */
		opa_recorelated(subquery);
	    }
	    break;
	}
	default:;
	}
	if ((subquery->ops_agg.opa_tidactive || global->ops_union)
	    &&
	    !global->ops_astate.opa_funcagg) /* if function aggregates then
					    ** TIDs have already be processed */
	    opa_stid(subquery);		    /* place TID resdoms in target list
                                            ** so that sorter can eliminate
                                            ** duplicates properly */
    }

    {
	PST_QTREE	    *qheader;		/* ptr to PSF query tree header
						*/
	OPS_SUBQUERY        *main;		/* ptr to main query */

	qheader = global->ops_qheader;
	main = *previous;
	main->ops_mode = qheader->pst_mode;	/* main query so get proper
						** query mode */
	if ((main->ops_agg.opa_tidactive || global->ops_union)
	    &&
	    !global->ops_astate.opa_funcagg) /* if function aggregates then
					    ** TIDs have already be processed */
	    opa_stid(main);			/* place TID resdoms in target list
						** so that sorter can eliminate
						** duplicates properly */
        if (qheader->pst_restab.pst_resvno >= 0)
	    main->ops_gentry =
            main->ops_result = qheader->pst_restab.pst_resvno; /* use parser 
						** result variable
                                                ** number if it is available */
        else if (qheader->pst_mode == PSQ_RETINTO ||
                qheader->pst_mode == PSQ_DGTT_AS_SELECT)
	    main->ops_gentry =
	    main->ops_result = opv_agrv(global, 
		(DB_TAB_NAME *)NULL, (DB_OWN_NAME *)NULL, 
		(DB_TAB_ID *)NULL, OPS_MAIN, TRUE,
		(OPV_GBMVARS *)NULL, OPV_NOGVAR); 
						/* assign a global range 
                                                ** variable for the
						** result relation so that query
						** compilation can use this */
    }
}
