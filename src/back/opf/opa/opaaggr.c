/*
**Copyright (c) 2004, 2010 Ingres Corporation
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
#define OPA_AGGREGATE TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 
/**
**
**  Name: OPAAGGR.C - aggregate processing phase of optimizer
**
**  Description:
**      Contains main entry point to the aggregate analysis phase of the 
**      optimizer
**
**  History:    
**      25-jun-86 (seputis)    
**          initial creation
**	16-aug-91 (seputis)
**	    add CL include files for PC group
**	6-apr-1992 (bryanp)
**	    Add support for PSQ_DGTT_AS_SELECT for temporary tables.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**      04-apr-98 (dacri01)  (inkdo01)
**          One more attempt to assure that the resulting where fragments
**          get normalized (avoiding OP0682's).
**      06-jun-99 (sarjo01) from 28-Nov-97 (horda03)
**          For STAR (Distributed) queries, do not attempt to PUSH
**          the main sq where clause into the aggregate sq. This
**          change fixes bug 87493.
**	13-jun-2001 (bespi01) bug#103424 problem ingsrv#1471 issue 10507939/1
**	    Fixed two unchecked memory accesses which caused certain
**	    create database procedure statements to fail
**	3-dec-02 (inkdo01)
**	    Range table expansion - OPV_GBMVARS is now char array.
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**	31-Aug-2006 (kschendel)
**	    Add rfagg-to-hashagg transformation.  Also, even though
**	    HFAGG should not show up until then, add HFAGG to places
**	    that test for RFAGG .. just in case.
**	18-Aug-2009 (drivi01)
**	    Cleanup warnings and precedence warnings in efforts
**	    to port to Visual Studio 2008.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
static void opa_mapvar(
	OPS_STATE *global,
	OPZ_BMATTS *joinmap,
	OPZ_BMATTS *covarmap,
	OPZ_BMATTS *lvarmap,
	OPV_IGVARS localvar,
	OPV_IGVARS covar,
	PST_QNODE *qnodep,
	bool and_node);
static void opa_rename(
	OPS_SUBQUERY *subquery,
	OPV_IGVARS localvar,
	OPV_IGVARS covar,
	PST_QNODE *qnodep,
	bool and_node);
static void opa_varsub(
	OPS_STATE *global);
static void opa_ptids(
	OPS_STATE *global);
static void opa_push(
	OPS_STATE *global);
static bool opa_pushandcheck(
	OPS_STATE *global,
	OPS_SUBQUERY *aggsq,
	PST_QNODE *groupp,
	PST_QNODE **nodep,
	OPV_IGVARS aggvar,
	OPV_GBMVARS *varmap);
static bool opa_pushcheck(
	OPS_STATE *global,
	PST_QNODE *groupp,
	PST_QNODE **nodep,
	OPV_IGVARS aggvar,
	OPV_GBMVARS *varmap,
	bool *gotvar);
static void opa_pushchange(
	OPS_STATE *global,
	PST_QNODE *groupp,
	PST_QNODE **nodep);
static void opa_noaggs(
	OPS_STATE *global);
static bool opa_suckdriver(
	OPS_STATE *global,
	OPS_SUBQUERY *subquery,
	OPS_SUBQUERY **mainsq,
	bool toplevel,
	bool *isunion);
static void opa_suck(
	OPS_STATE *global,
	OPS_SUBQUERY *subquery,
	OPS_SUBQUERY *msq);
static bool opa_suckandcheck(
	OPS_STATE *global,
	OPS_SUBQUERY *subquery,
	PST_QNODE **nodep,
	PST_QNODE **vararray,
	bool *tidyvars);
static bool opa_suckrestrict(
	OPS_STATE *global,
	OPS_SUBQUERY *subquery,
	PST_QNODE *nodep,
	PST_QNODE **vararray);
static void opa_suckeqc(
	OPS_STATE *global,
	OPV_IGVARS aggvar,
	PST_QNODE **vararray,
	PST_QNODE *resdomp,
	PST_QNODE **nodep,
	bool firstcall,
	OPS_SUBQUERY *subquery);
static bool opa_suckcheck(
	OPS_STATE *global,
	OPS_SUBQUERY *subquery,
	PST_QNODE **nodep,
	PST_QNODE **vararray,
	bool *gotvar);
static void opa_suckchange(
	OPS_STATE *global,
	OPS_SUBQUERY *subquery,
	PST_QNODE **nodep,
	PST_QNODE **vararray);
static void opa_sucktidyvars(
	PST_QNODE *nodep);
static void opa_predicate_check(
	PST_QNODE **andp,
	PST_QNODE **parent,
	bool looking_left);
static void opa_hashagg(
	OPS_STATE *global);
static bool opa_tproc_check(
	OPS_STATE *global,
	OPT_NAME *tp1,
	OPT_NAME *tp2);
static bool opa_tproc_parmanal(
	OPS_STATE *global,
	OPV_IGVARS *gvno,
	OPV_IGVARS *cycle_gvno);
static bool opa_tproc_nodeanal(
	OPS_STATE *global,
	OPV_IGVARS *cycle_gvno,
	OPV_IGVARS *other_gvno,
	PST_QNODE *nodep);
void opa_aggregate(
	OPS_STATE *global);

/*{
** Name: opa_mapvar	- map attributes used by self join variables
**
** Description:
**      In order to determine if the self-join and be eliminated, a
**	map of attributes used in the subquery for the correlated variable
**	must be a subset of the equi-join variables.  The equi-join can be
**	eliminated and correlated variable substituted if this is the case. 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      localvar                        variable which can be substituted
**      covar                           correlated variable
**      qnodep                          ptr to parse tree node being analyzed
**
** Outputs:
**      joinmap                         ptr to map of attributes involved
**					in join of localvar and covar
**	covarmap			map of all correlated attributes
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
**      21-feb-91 (seputis)
**          initial creation for b33278
[@history_template@]...
*/
static VOID
opa_mapvar(
	OPS_STATE	   *global,
	OPZ_BMATTS	   *joinmap,
	OPZ_BMATTS         *covarmap,
	OPZ_BMATTS         *lvarmap,
	OPV_IGVARS         localvar,
	OPV_IGVARS         covar,
	PST_QNODE          *qnodep,
	bool		   and_node)
{
    for (;qnodep; qnodep = qnodep->pst_left)
    {
	switch (qnodep->pst_sym.pst_type)
	{
	case PST_BOP:
	{
	    /* check for self-join */
	    if (and_node
		&&
		(   qnodep->pst_sym.pst_value.pst_s_op.pst_opno
		    == 
		    global->ops_cb->ops_server->opg_eq
		)
		&&
		(qnodep->pst_left->pst_sym.pst_type == PST_VAR)
		&&
		(qnodep->pst_right->pst_sym.pst_type == PST_VAR)
		&&
		((   (qnodep->pst_left->pst_sym.pst_value.pst_s_var.pst_vno == localvar)
		    &&
		    (qnodep->pst_right->pst_sym.pst_value.pst_s_var.pst_vno == covar)
		)
		||
		(   (qnodep->pst_right->pst_sym.pst_value.pst_s_var.pst_vno == localvar)
		    &&
		    (qnodep->pst_left->pst_sym.pst_value.pst_s_var.pst_vno == covar)
		))
		&&
		(   qnodep->pst_right->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id
		    ==
		    qnodep->pst_left->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id
		))
		BTset((i4)qnodep->pst_right->pst_sym.pst_value.
		    pst_s_var.pst_atno.db_att_id, (char *)joinmap);
	    break;
	}
	case PST_VAR:
	{
	    if (covar == qnodep->pst_sym.pst_value.pst_s_var.pst_vno)
		/* attribute from correlated relation has been found */
		BTset((i4)qnodep->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id,
		    (char *)covarmap);
	    if (localvar == qnodep->pst_sym.pst_value.pst_s_var.pst_vno)
		/* attribute from correlated relation has been found */
		BTset((i4)qnodep->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id,
		    (char *)lvarmap);
	    break;
	}
	case PST_AGHEAD:
	case PST_ROOT:
	case PST_AND:
	    break;
	default:
	    and_node = FALSE;
	    break;
	}
	opa_mapvar(global, joinmap, covarmap, lvarmap, localvar, covar, 
	    qnodep->pst_right, and_node);
    }
}

/*{
** Name: opa_rename	- rename the correlated variable in the query
**
** Description:
**      Some self-join flattened aggregate queries can be evaluated with 
**	referencing the parent subselect variable.  The correlated variable
**	can be eliminated from the aggregate, after substitution is made.
**
** Inputs:
**      localvar                        variable which will supply all correlated
**					values
**	covar				correlated variable to be eliminated
**
** Outputs:
**      qnodepp                         ptr to current parse tree node being
**					analyzed
[@PARAM_DESCR@]...
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
**      21-feb-91 (seputis)
**          initial creation for b33278
[@history_template@]...
*/
static VOID
opa_rename(
	OPS_SUBQUERY	   *subquery,
	OPV_IGVARS         localvar,
	OPV_IGVARS         covar,
	PST_QNODE          *qnodep,
	bool		   and_node)
{
    for (;qnodep; qnodep = qnodep->pst_left)
    {
	switch (qnodep->pst_sym.pst_type)
	{
	case PST_BOP:
	{
	    /* check for self-join */
	    OPZ_IATTS	atno;
	    if (and_node
		&&
		(   qnodep->pst_sym.pst_value.pst_s_op.pst_opno
		    == 
		    subquery->ops_global->ops_cb->ops_server->opg_eq
		)
		&&
		(qnodep->pst_left->pst_sym.pst_type == PST_VAR)
		&&
		(qnodep->pst_right->pst_sym.pst_type == PST_VAR)
		&&
		((  (qnodep->pst_left->pst_sym.pst_value.pst_s_var.pst_vno == localvar)
		    &&
		    (qnodep->pst_right->pst_sym.pst_value.pst_s_var.pst_vno == covar)
		)
		||
		(   (qnodep->pst_right->pst_sym.pst_value.pst_s_var.pst_vno == localvar)
		    &&
		    (qnodep->pst_left->pst_sym.pst_value.pst_s_var.pst_vno == covar)
		))
		&&
		(   (	atno = qnodep->pst_right->pst_sym.pst_value.
			    pst_s_var.pst_atno.db_att_id
		    )
		    ==
		    qnodep->pst_left->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id
		))
	    {
		/* in order to preserve proper NULL handling, any NULLABLE
		** attributes in the join map should have NULLs eliminated
		** so add a phrase "IS NOT NULL" for any nullable attribute */
		if (atno == DB_IMTID)
		    break;	    /* TIDs are not nullable */
		if (subquery->ops_global->ops_rangetab.opv_base->opv_grv[localvar]->
			opv_relation->rdr_attr[atno]->att_type < 0)
		{   /* NULLable attribute found so convert equi-join into
		    ** IS NOT NULL test */
		    qnodep->pst_sym.pst_type = PST_UOP;
		    qnodep->pst_sym.pst_value.pst_s_op.pst_opno
			= subquery->ops_global->ops_cb->ops_server->opg_isnotnull;
		    qnodep->pst_right = NULL;   /* FIXME - reuse this memory */
		    qnodep->pst_left->pst_sym.pst_value.pst_s_var.pst_vno = localvar;
		    opa_resolve(subquery, qnodep);
		    return;
		}
	    }
	    break;
	}
	case PST_VAR:
	{
	    if (covar == qnodep->pst_sym.pst_value.pst_s_var.pst_vno)
		qnodep->pst_sym.pst_value.pst_s_var.pst_vno = localvar;
	    break;
	}
	case PST_AGHEAD:
	case PST_ROOT:
	case PST_AND:
	    break;
	default:
	    and_node = FALSE;
	    break;
	}
	opa_rename(subquery, localvar, covar, qnodep->pst_right, and_node);
    }
}

/*{
** Name: opa_varsub	- substitute correlated variables if possible
**
** Description:
**      Some self-join flattened aggregate queries can be evaluated with 
**	referencing the parent subselect variable.  The correlated variable
**	can be eliminated from the aggregate, after substitution is made.
**	This happens most often in a query which is flatten such as
**
**	 select * from emp e1 where salary= 
**	    (select count(e2.salary) from emp e2 where e2.dno=e1.dno)
**
**	Normally, a join is used in the aggregate calculation, but the
**	duplicates from e1 are eliminated, so the self join to this
**	variable are not useful.
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
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
**      21-feb-91 (seputis)
**          initial creation for b33278
**	24-jun-91 (seputis)
**	    fix problem with star regression tests, b33278
**      15-dec-91 (seputis)
**          fix variable elimination optimization problem in which
**          some cases of optimization where missed due to incorrect
**          from list maps being assumed, the correct answer was
**          still returned but not as fast as possible
**      27-apr-93 (ed)
**          - b43974 - avoid variable elimination if flattening is turned
**          off
**	3-dec-02 (inkdo01)
**	    Changes for range table expansion.
**      14-aug-2006 (huazh01)
**          avoid variable substitution if subquery's having clause
**          contain two aggregate.
**          This fixes 116202.
**	24-jan-07 (hayke02)
**	    Avoid variable substitution if covar or localvar are the outer var
**	    in an outer join. This change fixes bug 117304.
**      25-mar-2010 (huazh01)
**          don't do variable substitution if subquery contains
**          corelated variable aggregage OPS_COAGG. It can be optimized
**          better in opa_push()/opa_suck() if opa_varsub() does not
**          modify its query tree. (b119735)
[@history_template@]...
*/
static VOID
opa_varsub(
	OPS_STATE	   *global)
{
    OPS_SUBQUERY    *subquery;

    if (global->ops_gmask & OPS_QUERYFLATTEN)
        return;                     /* avoid variable elimination if query flattening
                                    ** is turned off */
    for (subquery = global->ops_subquery; subquery; subquery = subquery->ops_next)
    {
	OPV_GBMVARS	    *localmap;	/* variables which are local to this query
				    */
	OPV_IGVARS	    covar;	/* current correlated variable being analyzed
				    */
	OPV_GRT		    *gbase;

	OPV_GBMVARS         globalmap;
	gbase = subquery->ops_global->ops_rangetab.opv_base;
	localmap = (OPV_GBMVARS *)&subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm;
	if (subquery->ops_duplicates == OPS_DKEEP)
	    continue;

	/* b116202 */
        if (subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_mask1 & PST_6HAVING)
            continue; 

        /* b119735 */
        if (subquery->ops_sqtype == OPS_FAGG &&
            subquery->ops_mask & OPS_COAGG)
            continue; 

	/* ensure the no more than 1 correlated variables exist in the by list, otherwise
	** a cartesean product is expected for the result of the aggregate */
        opv_submap(subquery, &globalmap);
        localmap = &globalmap;
        gbase = subquery->ops_global->ops_rangetab.opv_base;
	if (subquery->ops_agg.opa_projection)
	{
	    PST_QNODE	    *qnodep;
	    OPV_IGVARS	    varno;

	    varno = OPV_NOGVAR;
	    for (qnodep = subquery->ops_root->pst_left; qnodep; qnodep = qnodep->pst_left)
	    {
		if ((qnodep->pst_sym.pst_type == PST_RESDOM)
		    &&
		    (qnodep->pst_right->pst_sym.pst_type == PST_VAR)
		    &&
		    qnodep->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT)
		{
		    if (varno == OPV_NOGVAR)
			varno = (OPV_IGVARS)qnodep->pst_right->pst_sym.pst_value.pst_s_var.pst_vno;
		    else if (varno != qnodep->pst_right->pst_sym.pst_value.pst_s_var.pst_vno)
			break;
		}
	    }
	    if (qnodep)
		continue;	    /* if a premature break occurred then this subquery
				    ** cannot have variable substitution since there
				    ** are at least 2 correlated variables, note that
				    ** there is an assumption a correlated variable in
				    ** the by list is always a var node, but this should
				    ** be true due to the flattening algorithm */
	}

	for (covar = -1; (covar = (OPV_IGVARS)BTnext((i4)covar, (char *)localmap, 
	    (i4)BITS_IN(*localmap)))>=0;)
	{
	    OPV_IGVARS	localvar; /* possible candidate to substitute
				    ** covar */
	    RDR_INFO	*co_rdfp; /* ptr to rdf descriptor for correlated
				    ** variable */
	    co_rdfp = gbase->opv_grv[covar]->opv_relation;
	    if (!co_rdfp)
		continue;	    /* ignore non-user specified tables, and
				    ** correlated variables which have tids kept */
	    for (localvar= covar; (localvar = (OPV_IGVARS)BTnext((i4)localvar, (char *)localmap, 
		(i4)BITS_IN(*localmap)))>=0;)
	    {
		RDR_INFO	    *local_rdfp;

		local_rdfp = gbase->opv_grv[localvar]->opv_relation;
		if (!local_rdfp
		    ||	/* ignore non-user specified tables */
		    (localvar == covar)
		    )
		    continue;	/* variable cannot map to itself */
		if (    (co_rdfp->rdr_rel->tbl_id.db_tab_base
			==
			local_rdfp->rdr_rel->tbl_id.db_tab_base)
		    &&
			(co_rdfp->rdr_rel->tbl_id.db_tab_index
			==
			local_rdfp->rdr_rel->tbl_id.db_tab_index)
		    &&
		    (	!subquery->ops_agg.opa_tidactive
			||
			!BTtest((i4)covar, (char *)&subquery->ops_agg.opa_keeptids)
			||
			!BTtest((i4)localvar, (char *)&subquery->ops_agg.opa_keeptids)
		    )
		    &&
		    (!subquery->ops_oj.opl_pouter
		    ||
		    ((BTcount((char *)&subquery->ops_oj.opl_pouter->
		    opl_parser[covar], OPL_MAXOUTER) == 0)
		    &&
		    (BTcount((char *)&subquery->ops_oj.opl_pouter->
		    opl_parser[localvar], OPL_MAXOUTER) == 0)))
		    )
		{   /* found local variable with same ID as the
		    ** correlated variable so a potential for variable
		    ** substitution exists */
		    OPZ_BMATTS	    joinmap;	/* map of attributes
						    ** for which joins exist */
		    OPZ_BMATTS	    covarmap;	/* map of all attributes
						    ** of the correlated variable
						    ** used in the query */
		    OPZ_BMATTS	    lvarmap;	/* map of attributes used in
						** the local var */
		    OPV_IGVARS	    var1;
		    OPV_IGVARS	    var2;
		    bool	    subvar;	/* TRUE if variable can be
						** substituted */
		    MEfill(sizeof(joinmap), (u_char)0, (PTR)&joinmap);
		    MEfill(sizeof(covarmap), (u_char)0, (PTR)&covarmap);
		    MEfill(sizeof(lvarmap), (u_char)0, (PTR)&lvarmap);
		    opa_mapvar(global, &joinmap, &covarmap, &lvarmap, localvar, covar,
			subquery->ops_root, TRUE);
		    if (BTsubset((char *)&covarmap, (char *)&joinmap, 
			    (i4)BITS_IN(joinmap))
			&&
			(   !subquery->ops_agg.opa_tidactive
			    ||
			    !BTtest((i4)covar, (char *)&subquery->ops_agg.opa_keeptids)
			)
			)
		    {
			var1 = covar;
			var2 = localvar;
			subvar = TRUE;
		    }
		    else if (BTsubset((char *)&lvarmap, (char *)&joinmap, 
				(i4)BITS_IN(joinmap))
			&&
			(   !subquery->ops_agg.opa_tidactive
			    ||
			    !BTtest((i4)localvar, (char *)&subquery->ops_agg.opa_keeptids)
			)
			)
		    {
			var1 = localvar;
			var2 = covar;
			subvar = TRUE;
		    }
		    else
			subvar = FALSE;

		    if (subvar)
		    {
			opa_rename(subquery, var2, var1, subquery->ops_root, TRUE);
						    /* if the correlated variables
						    ** used can be supplied by all
						    ** the self equi-join variables
						    ** then subsitution can occur */
			BTclear((i4)var1, (char *)&subquery->ops_root->pst_sym.
			    pst_value.pst_s_root.pst_tvrm);
			subquery->ops_vmflag = FALSE; /* variable maps are now invalid
						    ** for this query */
		    }
		}
	    }
	}
    }
}

/*{
** Name: opa_ptids	- process tids
**
** Description:
**      This routine will traverse the subquery list and determine the TID
**	handling requirements of the various queries 
**
** Inputs:
**      global                          ptr to state variable
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
**      7-may-90 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opa_ptids(
	OPS_STATE          *global)
{
    OPS_SUBQUERY	*subquery;
    for (subquery = global->ops_subquery; subquery; subquery = subquery->ops_next)
    {
	if (subquery->ops_agg.opa_tidactive || global->ops_union)
	    opa_stid(subquery);	    /* process TIDs for subquery */
    }
}

/*{
** Name: OPA_PUSH	- analyze subquery structure in search of "having" 
**	clause which can be pushed into where clause. 
**
** Description:
**	This function coordinates a minor heuristic which attempts to push 
**	having clauses as close to base table retrieval as possible. For
**	example, a having clause with no aggregate functions (only grouping
**	column restrictions) is semantically equivalent to a where clause. 
**	More useful (since explicit having clauses with no aggregates are
**	poorly coded in the first place) is the application of this heuristic 
**	to retrievals from aggregate views. Where clauses on such selects 
**	are treated by Ingres like having clauses. Pushing the having into the 
**	where clause can increase optimization potential.
**
**	NOTE: if the implementation of this heuristic looks clumsy and a bit
**	haphazard, it is because this whole phase of processing is very
**	difficult to penetrate! The heuristic currently only handles "simple"
**	cases - unions disable it, nested subselects probably disable it, etc.
**	Once the code is better understood (that is, if it is ever better 
**	understood), a more general attempt to push HAVING predicates through
**	the aggregate functions may be attempted.
**
** Inputs: global	- pointer to OPF global state variable
**
** Outputs:
**	Modified subquery structure, with main sq having anded to aggregate
**	sq where.
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      19-feb-97 (inkdo01)
**          written
**	9-may-97 (inkdo01)
**	    Minor fix to tidy modified qualification parse tree.
**	19-may-97 (inkdo01)
**	    One more fix to disable transform on OPS_PROJECTION, since it 
**	    undermines projection semantics.
**	27-jun-97 (inkdo01)
**	    Reenable OPS_PROJECTION. Moved predicates must also be applied to
**	    AGGF/RAGGF between PROJ and MAIN.
**	29-aug-97 (inkdo01)
**	    This time, left copied predicates in MAIN, too, for the PROJECTION
**	    heuristic (only drop it for the HAVING push heuristic).
**	19-sep-97 (inkdo01)
**	    Fixed bug 84129 - looks for func aggs followed by simple agg, then
**	    fiddles duplicates option.
**	23-sep-97 (inkdo01)
**	    A further refinement of the OPS_PROJECTION heuristic, to allow 
**	    promotion of restriction predicates when an OPS_SAGG subquery
**	    follows the RFAGG/FAGG. This happens with simple aggs with
**	    "not exists" queries.
**	31-mar-98 (inkdo01)
**	    One more attempt to assure that the resulting where fragments
**	    get normalized (avoiding OP0682's).
**	3-dec-02 (inkdo01)
**	    Changes for range table expansion.
**      14-Apr-08 (macde01)
**	    Cleaning up Klocwork bugs, initializing pointers to NULL.
**	 4-jun-08 (hayke02)
**	    Return if we have a concurrent subquery (non-NULL opa_concurrent),
**	    as we do in opa_suckdriver(). This change fixes bug 120357.
**      19-mar-2009 (huazh01)
**          Modify the fix to bug 120357. We now allow concurrent subqry
**          to call opa_pushandcheck(). However, on opa_pushandcheck(), we
**          will only modify the query tree if opa_pushandcheck() return
**          true for both children of a AND node. (bug 121792)
**      29-jul-2009 (huazh01)
**          remove pst_qnode in the form of 'x = x' from the qualification. 
**          (b122361)
**      30-jul-2010 (huazh01)
**          Re-introduce opa_push() part of the fix to b120357, but restrict
**          it so we apply the fix only if the concurrent query's result
**          relation ops_gentry is the same as the current one. if they are
**          not the same, propagating restriction will cause wrong result.
**          This fixes b123330.
[@history_template@]...
*/
static VOID 
opa_push(
	OPS_STATE	*global)

{
    OPS_SUBQUERY	*usq = NULL, 
                        *psq = NULL, 
                        *fsq = NULL, 
                        *msq = NULL, 
                        *sq1 = NULL;
    PST_QNODE		*groupp;	/* ptr to start of group col RESDOMs */
    PST_QNODE		*andp, *n1;
    OPV_GBMVARS		varmap;


    /* The heuristic looks for two cases - a OPS_PROJECTION subquery followed
    ** by a OPS_FAGG/RFAGG, then the MAIN subquery; or a OPS_FAGG/RFAGG, then
    ** the MAIN subquery. In the case of the former, a bit map is made 
    ** from range variables referenced in the PROJECTION subquery, and any
    ** boolean factor on the right side of the MAIN subquery which can be
    ** applied by that set of variables is moved to the right side of the 
    ** PROJECTION and (R)FAGG subquery parse trees. This specifically helps 
    ** optimization of correlated NOT EXISTS subselects in which there are
    ** other outer select restriction predicates. In the case of the latter, 
    ** Boolean factors in the MAIN subquery involving atomic columns (NOT 
    ** aggregate functions) projected by the aggregate function (FAGG/RFAGG) 
    ** are moved to the FAGG/RFAGG parse tree. 
  
    ** The code loops over the union subquery structure, as well. However, 
    ** union'ed selects seem to be related in a more subtle way, so this
    ** code only works in the NO union case. This is something else which
    ** could be done to improve this heuristic in the future. */

    MEfill(sizeof(varmap), 0, (char *)&varmap);

    for (usq = global->ops_subquery; usq; usq = usq->ops_union)
    {				/* loop over all the unions */
	/* First, apply a very small fix which looks for simple aggs of
	** func aggs, which should only happen on aggs of agg views. Not-
	** quite-right code in opagen forces duplicates removal throughout
	** such queries, possibly resulting in incorrect simple aggs, when 
	** the presence of the func agg guarantees the dups removal anyway.
	** Resetting dups option here fixes bug 84129. */
	if ((usq->ops_sqtype == OPS_RFAGG || usq->ops_sqtype == OPS_FAGG
	    || usq->ops_sqtype == OPS_HFAGG) &&
	    usq->ops_duplicates == OPS_DKEEP &&
  	    usq->ops_next && usq->ops_next->ops_sqtype == OPS_SAGG)
 	 usq->ops_next->ops_duplicates = OPS_DKEEP;

	psq = fsq = sq1 = usq;
	if (psq->ops_sqtype != OPS_PROJECTION) psq = NULL;
	 else fsq = psq->ops_next;
	if (fsq->ops_sqtype == OPS_FAGG || fsq->ops_sqtype == OPS_RFAGG
		|| fsq->ops_sqtype == OPS_HFAGG)
	    msq = fsq->ops_next;
	else return;

	if (msq == NULL || msq->ops_sqtype != OPS_MAIN && 
	    msq->ops_sqtype != OPS_SAGG || 
	    msq->ops_root->pst_right == NULL || msq->ops_root->pst_right->
	    pst_sym.pst_type == PST_QLEND || (psq &&
	    (msq->ops_root->pst_sym.pst_type != PST_ROOT &&
	    msq->ops_sqtype == OPS_MAIN ||
	    msq->ops_root->pst_sym.pst_type != PST_AGHEAD &&
	    msq->ops_sqtype == OPS_SAGG ||
	    msq->ops_root->pst_sym.pst_value.pst_s_root.pst_qlang != DB_SQL))) 
	 return;		/* must be a OPS_MAIN/SAGG with having clause 
				** and in the PROJECTION case, it must be an
				** SQL query. Quel semantics differ here. */
  
	/* Look for start of group cols in (R)FAGG. It's the first RESDOM
	** which directly owns a PST_VAR. */

	for (groupp = fsq->ops_root->pst_left; groupp; 
		groupp = groupp->pst_left)
	 if (groupp->pst_right && 
		groupp->pst_right->pst_sym.pst_type == PST_VAR)
	 break;			/* got it */
	if (groupp == NULL) return;

        if (usq->ops_agg.opa_concurrent &&
            usq->ops_agg.opa_concurrent->ops_gentry !=
            usq->ops_gentry)
           return;

	/* If this is a PROJECTION, build a bit map of ref'ed vars. */
	if (psq) for (n1 = psq->ops_root->pst_left; n1 && n1->pst_sym.pst_type == 
			PST_RESDOM; n1 = n1->pst_left)
	 if (n1->pst_right->pst_sym.pst_type == PST_VAR)
		BTset((i4)n1->pst_right->pst_sym.pst_value.pst_s_var.pst_vno,
			(char *)&varmap);
	 else return;		/* if not PST_VAR, we're lost */
	else sq1 = fsq;		/* no PROJECTION */
 
	/* Call routine which determines eligibility of HAVING clause */
	if (!opa_pushandcheck(global, sq1, groupp, &msq->ops_root->pst_right, 
	    fsq->ops_gentry, &varmap)) return;

	/* Got a candidate structure. Call routine to change the PST_VAR nodes,
	** then splice having clause into (R)FAGG where clause. */
	if (!psq) opa_pushchange(global, groupp, &msq->ops_root->pst_right);

	andp = opv_opnode(global, PST_AND, (ADI_OP_ID)0, PST_NOJOIN);
	andp->pst_left = msq->ops_root->pst_right;
	andp->pst_right = sq1->ops_root->pst_right;
	sq1->ops_root->pst_right = andp;
	if (psq)
	{
	    /* If it's a PROJECTION, must also apply predicate to 
	    ** corresponding FAGG/RFAGG, else AGGF/PROJ semantics get
	    ** screwed up */
	    opv_copytree(global, &sq1->ops_root->pst_right->pst_left);
						/* first, copy the PROJ part */
	    opv_copytree(global, &andp);	/* then, copy to FAGG/RFAGG */
	    andp->pst_right = fsq->ops_root->pst_right;
						/* splice the predicate */
	    fsq->ops_root->pst_right = andp;
	    opj_adjust(fsq, &fsq->ops_root->pst_right);
	}
	 else msq->ops_root->pst_right = opv_qlend(global);
				/* detach whole qual from main */
	opj_adjust(sq1, &sq1->ops_root->pst_right);
				/* make qual look right for rest of OPF */

        /* b122361: remove any 'x = x' nodes from the qual.
        */
        opa_predicate_check(&(sq1->ops_root->pst_right),
                        &(sq1->ops_root), FALSE);

    }		/* end of union loop */
    /* Just to make sure we're clean, reset ops_normal to force
    ** re-normalization of where clause fragments. */
    if (sq1) sq1->ops_normal = FALSE;
    if (fsq) fsq->ops_normal = FALSE;
    if (msq) msq->ops_normal = FALSE;
    if (psq) psq->ops_normal = FALSE;

}	/* end of opa_push */

/*{
** Name: OPA_PUSHANDCHECK - analyze main sq where clause (i.e., having clause)
**	for transformation eligibility.
**
** Description:
**	This function works in conjunction with opa_pushcheck to descend the
**	having clause expression tree. It is interested in AND's at the top
**	of the parse tree, as they represent subportions of the having which
**	can be moved to the aggregate sq, if they qualify. Everything else 
**	is sent on to opa_pushcheck for analysis (including ANDs nested in
**	ORs). If one branch of a top AND qualifies, but not both, the 
**	qualifying branch is transformed here and moved from the having 
**	clause to the aggregate sq.
**
** Inputs: global	- pointer to OPF global state variable
**	aggsq	- pointer to aggregate sq structure
**	groupp	- start of RESDOM list which defines grouping columns
**	nodep	- pointer to pointer to root of expression being analyzed
**	aggvar	- var no of aggregate output "tuple"
**	varmap	- ptr to map of range vars in BY list (of PROJECTION)
**
** Outputs:
**	None
**	Returns:
**	    TRUE if the having clause is eligible for transformation,
**	    else FALSE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      19-feb-97 (inkdo01)
**          written
**	9-may-97 (inkdo01)
**	    Minor fix to tidy modified qualification parse tree.
**	27-jun-97 (inkdo01)
**	    Fix to re-enable "not exists" heuristic. Place moved predicates
**	    into FAGG/RFAGG subquery, as well as PROJ.
**	25-jul-97 (inkdo01)
**	    Fix the fix. Above change sometimes requires re-normalizing the
**	    resulting where clause.
**	29-aug-97 (inkdo01)
**	    This time, left copied predicates in MAIN, too, for the PROJECTION
**	    heuristic (only drop it for the HAVING push heuristic).
**	7-nov-97 (inkdo01)
**	    Add gotvar to pushcheck calls to assure we only transform when
**	    an applicable predicate is found.
**	3-dec-02 (inkdo01)
**	    Changes for range table expansion.
**	8-may-03 (inkdo01)
**	    Add code to drop ON clause predicates to fix bug 110206.
**      19-mar-2009 (huazh01)
**          for concurrent subquery (non-NULL opa_concurrent), do not
**          modify the query tree if only one of the children of a PST_AND
**          node qualifies. (bug 121792)
**      30-jul-2010 (huazh01)
**          Re-introduce opa_push() part of the fix to b120357, but restrict
**          it so we apply the fix only if the concurrent query's result
**          relation ops_gentry is the same as the current one. if they are
**          not the same, propagating restriction will cause wrong result.
**          Also back out the fix to b121792 because the new fix to b120357
**          covers b121792 as well. (b123330)
[@history_template@]...
*/
static bool 
opa_pushandcheck(
	OPS_STATE	*global,
	OPS_SUBQUERY	*aggsq,
	PST_QNODE	*groupp,
	PST_QNODE	**nodep,
	OPV_IGVARS	aggvar,
	OPV_GBMVARS	*varmap)

{
    bool	success, leftres, rightres, gotvar;
    PST_QNODE	*n1, *n2, *andp;


    /* The code simply recursively descends the having expression tree.
    ** It's primarily interested in ANDs, since ANDs in this function 
    ** anchor conjunctive factors which can be detached and ANDed to the
    ** aggregate sq without altering the semantics of the query. All other
    ** nodes are sent on to opa_pushcheck for assessment. If one but not both
    ** children of an AND qualify, the successful child is transformed and
    ** moved to the aggregate sq right here. If the whole having clause 
    ** qualifies, opa_push performs the final transform. */

    switch ((*nodep)->pst_sym.pst_type) {
			/* BIG switch */
     case PST_AND:
	leftres = ((*nodep)->pst_left && opa_pushandcheck(global, aggsq, 
		groupp, &(*nodep)->pst_left, aggvar,varmap));
	rightres = ((*nodep)->pst_right && opa_pushandcheck(global, aggsq, 
		groupp, &(*nodep)->pst_right, aggvar,varmap));
	if (leftres && rightres) return(TRUE);
	if (leftres)
	{
	    n1 = (*nodep)->pst_left;
	    n2 = (*nodep)->pst_right;
	}
	else if (rightres)
	{
	    n1 = (*nodep)->pst_right;
	    n2 = (*nodep)->pst_left;
	}
	else return(FALSE);

        if (n1->pst_sym.pst_type == PST_QLEND)
           return(FALSE);

	/* Copy (for not in/exists heuristic) or move (for having 
	** heuristic) where factor (in n1) from main subquery parse tree
	** to FAGG/RFAGG and PROJ subquery parse trees. */
	if (aggsq->ops_sqtype != OPS_PROJECTION)
	{
	   opa_pushchange(global, groupp, &n1);
	   (*nodep) = n2;
	   opj_adjust(aggsq, nodep);
	}
	andp = opv_opnode(global, PST_AND, (ADI_OP_ID)0, PST_NOJOIN);
	andp->pst_left = n1;
	andp->pst_right = aggsq->ops_root->pst_right;
	aggsq->ops_root->pst_right = andp;
	aggsq->ops_normal = FALSE;		/* re-norm FAGG where */
	if (aggsq->ops_sqtype == OPS_PROJECTION)
	{
	    /* If it's a PROJECTION, must also apply predicate to 
	    ** corresponding FAGG/RFAGG, else AGGF/PROJ semantics get
	    ** screwed up. Also, copy the original chunk from main -
	    ** differs from "having" heuristic which moves it. */
	    opv_copytree(global, &aggsq->ops_root->pst_right->pst_left);
	    opv_copytree(global, &andp);	/* make a copy */
	    andp->pst_right = aggsq->ops_next->ops_root->pst_right;
						/* splice the predicate */
	    aggsq->ops_next->ops_root->pst_right = andp;
	    aggsq->ops_next->ops_normal = FALSE; 
						/* make it re-norm the where */
	}
	return(FALSE);

    case PST_BOP:
    case PST_UOP:
	if ((*nodep)->pst_sym.pst_value.pst_s_op.pst_joinid > PST_NOJOIN)
	    return(FALSE);

    default:	/* all the rest just descend the tree */
	success = TRUE;
	gotvar = FALSE;
	if ((*nodep)->pst_left) success = opa_pushcheck(global, groupp, 
		&(*nodep)->pst_left, aggvar, varmap, &gotvar);
	if (success && (*nodep)->pst_right) success = opa_pushcheck(global,
		groupp, &(*nodep)->pst_right, aggvar, varmap, &gotvar);
	return(success);
    }		/* end of BIG switch */

}	/* end of opa_pushandcheck */

/*{
** Name: OPA_PUSHCHECK	- analyze main sq where clause (i.e., having clause)
**	for transformation eligibility.
**
** Description:
**	This function recursively descends the having clause parse tree,
**	checking to see if the having clause can be transformed and merged
**	with the aggregate sq where clause. The rules are that there can be
**	no aggregate functions in the having, and all column references 
**	must be to grouping columns (this actually must be true, if there are 
**	no aggregates). 
**
** Inputs: global	- pointer to OPF global state variable
**	groupp	- start of RESDOM list which defines grouping columns
**	nodep	- pointer to pointer to root of expression being analyzed
**	aggvar	- var no of aggregate output "tuple"
**	varmap	- bit map of referenced vars (for PROJECTION)
**	gotvar	- bool ptr to indicate if a qualifying VAR was found
**		  in this expression (so we don't copy constant exprs)
**
** Outputs:
**	None
**	Returns:
**	    TRUE if the having clause is eligible for transformation,
**	    else FALSE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      19-feb-97 (inkdo01)
**          written
**	7-nov-97 (inkdo01)
**	    Add gotvar to pushcheck calls to assure we only transform when
**	    an applicable predicate is found.
**      20-may-98 (hayke02)
**          Set gotvar to TRUE when a simple VAR has been found (not just
**          when a matching VAR has been found). This change fixes bug 90889.
**	3-dec-02 (inkdo01)
**	    Changes for range table expansion.
[@history_template@]...
*/
static bool 
opa_pushcheck(
	OPS_STATE	*global,
	PST_QNODE	*groupp,
	PST_QNODE	**nodep,
	OPV_IGVARS	aggvar,
	OPV_GBMVARS	*varmap,
	bool		*gotvar)

{
    bool	success = TRUE;
    PST_QNODE	*n1;


    /* The code simply recursively descends the having expression tree,
    ** examining the nodes as it goes. Everything returns TRUE except 
    ** PST_AOPs (designating aggregate functions) and PST_VARs which don't
    ** match a grouping column from the aggregate subquery (though I'm not
    ** sure how that could happen). */

    switch ((*nodep)->pst_sym.pst_type) {
			/* BIG switch */
     case PST_AOP:
	return(FALSE);

     case PST_VAR:
	/* Validate the VAR - the varno must match the aggregate subquery 
	** result var and the atno must be in the grouping list, OR it
	** must directly reference a column in an original range table
	** entry (not the output of an aggregation). */
	if (BTcount((char *)varmap, OPV_MAXVAR) != 0)		
				/* PROJECTION - must be simple VAR */
	 if ((*nodep)->pst_sym.pst_value.pst_s_var.pst_vno <= 
		global->ops_qheader->pst_rngvar_count &&
		BTtest((i4)(*nodep)->pst_sym.pst_value.pst_s_var.pst_vno,
		(char *)varmap))
         {
            *gotvar = TRUE;
            return(TRUE);
         }
	 else return(FALSE);	/* PROJECTION, but no good */

	if ((*nodep)->pst_sym.pst_value.pst_s_var.pst_vno != aggvar)
	    return(FALSE);

	for (n1 = groupp; n1 && n1->pst_sym.pst_type == PST_RESDOM;
		n1 = n1->pst_left)	/* loop over group cols */
	 if (n1->pst_right->pst_sym.pst_type == PST_VAR &&
		n1->pst_sym.pst_value.pst_s_rsdm.pst_rsno ==
		(*nodep)->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
	{
	    *gotvar = TRUE;
	    return(TRUE);	/* found a match */
	}
	return(FALSE);		/* didn't found a match */

    default:	/* all the rest just descend the tree */
	if ((*nodep)->pst_left) success = opa_pushcheck(global, groupp, 
		&(*nodep)->pst_left, aggvar, varmap, gotvar);
	if (success && (*nodep)->pst_right) success = opa_pushcheck(global,
		groupp, &(*nodep)->pst_right, aggvar, varmap, gotvar);
	return(success && (*gotvar));
    }		/* end of BIG switch */

}	/* end of opa_pushcheck */

/*{
** Name: OPA_PUSHCHANGE	- locate PST_VAR nodes and replace them with 
**	corresponding PST_VAR node from aggregate subquery.
**
** Description:
**	This function recursively descends the having clause parse tree,
**	looking for PST_VAR nodes. Each PST_VAR is modified to reference 
**	the corresponding grouping column in the aggregate subquery. 
**
** Inputs: global	- pointer to OPF global state variable
**	groupp	- start of RESDOM list which defines grouping columns
**	nodep	- pointer to pointer to root of expression being analyzed
**
** Outputs:
**	Modified having clause
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      19-feb-97 (inkdo01)
**          written
[@history_template@]...
*/
static VOID 
opa_pushchange(
	OPS_STATE	*global,
	PST_QNODE	*groupp,
	PST_QNODE	**nodep)

{
    PST_QNODE	*n1;


    /* Recursively descend the having expression tree in search of 
    ** PST_VARs. Then search the grouping column list for a resdom with
    ** matching output column number. The contents of the corresponding 
    ** PST_VAR (from the group list) replace those of the having clause
    ** PST_VAR. This only needs to be done for PST_VARs representing 
    ** cols projected from the aggregate computation. Refs to original
    ** range table entries don't get into this function. */

    switch ((*nodep)->pst_sym.pst_type) {

     case PST_VAR:
	for (n1 = groupp; n1; n1 = n1->pst_left)
	 if (n1->pst_sym.pst_value.pst_s_rsdm.pst_rsno ==
		(*nodep)->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
	{
	    STRUCT_ASSIGN_MACRO(n1->pst_right->pst_sym.pst_value.pst_s_var,
		(*nodep)->pst_sym.pst_value.pst_s_var);
				/* copy PST_VAR contents */
	    break;
	}
	break;

     default:	/* everything else just recurses */
	if ((*nodep)->pst_left) opa_pushchange(global, groupp, 
					&(*nodep)->pst_left);
	else break;
        if ((*nodep)->pst_right) opa_pushchange(global, groupp, 
					&(*nodep)->pst_right);
    }		/* end of switch */

}	/* end of opa_pushchange */

/*{
** Name: opa_noaggs	- init subquery list given that no aggregates exist
**
** Description:
**      This routine will initialize the global range table and subquery list 
**      given that no aggregates exist
**
** Inputs:
**      global                          ptr to global state variable
**
** Outputs:
**      global->ops_range_tab           initialized with elements corresponding
**                                      to the parser range table
**      global->ops_subquery            one element placed in subquery list
**                                      corresponding to main query
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	1-jul-86 (seputis)
**          initial creation
**	12-jan-91 (seputis)
**	    -fix b35243
**	6-apr-1992 (bryanp)
**	    Add support for PSQ_DGTT_AS_SELECT for temporary tables.
**      18-sep-92 (seputis)
**          - fix 44850 - added parameter to opv_agrv as part of fix
[@history_line@]...
*/
static VOID
opa_noaggs(
	OPS_STATE          *global)
{
    /* if no aggregates exist then simply create a subquery list of one
    ** element.  Get memory for first link in subquery list and place root
    ** in this node.  
    */
    OPS_SUBQUERY		*subquery;  /* ptr to main query state variable
					*/
    PST_QTREE			*qheader;   /* ptr to PSF query tree header */

    qheader = global->ops_qheader;
    subquery = global->ops_subquery;
    subquery->ops_width 
	= opv_tuplesize(global, qheader->pst_qtree); /* result
					    ** width of resdom list only */
    subquery->ops_mode = qheader->pst_mode; /* main query so get proper query 
					    ** mode */
    if (qheader->pst_restab.pst_resvno >= 0)
	subquery->ops_gentry =
	subquery->ops_result = (OPV_IGVARS)qheader->pst_restab.pst_resvno; /* use parser 
                                            ** result variable
					    ** number if it is available */
    else if ((qheader->pst_mode == PSQ_RETINTO)
	    ||
	    (qheader->pst_mode == PSQ_DGTT_AS_SELECT))
    {
	subquery->ops_gentry =
	subquery->ops_result = opv_agrv(global, (DB_TAB_NAME *)NULL, 
	    (DB_OWN_NAME *)NULL, (DB_TAB_ID *)NULL, OPS_MAIN, TRUE, 
	    (OPV_GBMVARS *)NULL, OPV_NOGVAR); /* assign
                                            ** a global range variable for the
                                            ** result relation so that query
                                            ** compilation can use this */
    }
    if (subquery->ops_agg.opa_tidactive)
	opa_stid(subquery);		    /* if tids need to be kept then
					    ** create resdom nodes where
					    ** necessary */
    subquery->ops_dist.opd_dv_cnt = global->ops_parmtotal; /* init the
					    ** parameter count for the
					    ** single query */
}

/*{
** Name: OPA_SUCKDRIVER 	- analyzes subquery structure, looking 
**	for where clause fragments to promote from later subqueries to 
**	earlier ones.
**
** Description:
**	The "ops_next" chain through a set of subqueries more or less 
**	defines the sequence in which the corresponding plan fragments
**	are executed (sets of union'ed selects also have their subqueries
**	linked within the overall subquery sequnce). 
**
**	This series of routines (opa_suck....) implements a heuristic 
**	which attempts to promote applicable where clause fragments from
**	later subqueries to earlier ones. This is particularly beneficial
**	to "aggregate" subqueries when later where or having clause
**	fragments can be used to restrict "group by" columns during the
**	aggregate computation. Such restrictions can massively improve
**	performance (potentially changing full table scans into index
**	probes to retrieve only those rows restricted by the where/having 
**	clause).
**
**	"opa_suckdriver" recursively descends the chain of subqueries,
**	locating the last in sequence (the OPS_MAIN subquery). It then 
**	unwinds the recursions, calling opa_suck to locate where clause 
**	fragments which can be promoted from the later subquery to the
**	preceding subquery on the ops_next chain. This can be done for 
**	any conjunctive factor whose PST_VAR references appear in the
**	RESDOM list of the earlier subquery.
**
** Inputs: global	- pointer to OPF global state variable
**
** Outputs:
**	Modified subquery structure.
**
** Returns: FALSE, if nothing can be done and we just wish to
**		unwind the recursion.
**
** Side effects: none
**
** History:
**	8-jun-98 (inkdo01)
**	    Written.
**	26-apr-00 (inkdo01)
**	    Add code to move foldable factors over subqueries which
**	    may not reference them.
**	20-feb-03 (inkdo01 for hayke02)
**	    Return FALSE if we find that a concurrent subquery is present -
**	    this will prevent restrictions from one union partition being
**	    copied to another union partition. This change fixes bug 109182.
**	18-mar-03 (inkdo01)
**	    Fix to prevent recursing on each sq of a union set. Only recurse 
**	    once for whole set (UNIONs plus one VIEW).
**	14-july-06 (dougi)
**	    Slight change to 20-feb, above, to exclude concurrent case only
**	    when union is also present. I'm not sure why the concurrent
**	    problem can exist since there are no references to opa_concurrent
**	    in code that executes following this function.
**      14-Apr-08 (macde01)
**	    Klocwork bugs - Verify pointers are not null before dereference: 
**	    usq and subquery->ops_next.
**	 6-jun-08 (hayke02)
**	    Exclude concurrent case when subquery->ops_next and
**	    opa_concurrent->ops_next are both OPS_MAIN. This change fixes bug
**	    120357.
*/

static bool
opa_suckdriver(OPS_STATE 	*global,
	OPS_SUBQUERY	*subquery,
	OPS_SUBQUERY	**mainsq,
	bool		toplevel,
	bool		*isunion)

{
    OPS_SUBQUERY	*usq, *nsq;
    PST_QNODE		*root;
    bool		nextmain = FALSE;

    /* Check for unexpected subquery structures, then recurse until
    ** we get to MAIN subquery. */

    if (subquery == NULL || subquery->ops_sqtype != OPS_MAIN &&
	subquery->ops_sqtype != OPS_UNION && 
	subquery->ops_sqtype != OPS_VIEW &&
	subquery->ops_sqtype != OPS_SAGG &&
	subquery->ops_sqtype != OPS_FAGG &&
	subquery->ops_sqtype != OPS_RFAGG &&
	subquery->ops_sqtype != OPS_HFAGG &&
	subquery->ops_sqtype != OPS_PROJECTION) return(FALSE);
				/* gotta be one of the above */

    if (toplevel && subquery->ops_sqtype == OPS_MAIN) return(FALSE);
				/* trivial query */

    if (subquery->ops_sqtype == OPS_UNION)
    {
	for (usq = subquery->ops_next; 
	     usq && usq->ops_sqtype == OPS_UNION; usq = usq->ops_next)
		;               /* loop over bunch of UNION's */

	*isunion = TRUE;	/* tell folks it's a union query */
	if (usq == NULL || usq->ops_sqtype != OPS_VIEW) 
	    return(FALSE);	/* last must be VIEW (as far as I know */
	subquery = usq;		/* reset subquery to end of union for 
				** recursive call */
    }

    if (subquery->ops_next)
    {
        if (subquery->ops_next->ops_sqtype == OPS_MAIN) 
 	    *mainsq = subquery->ops_next; /* save for extra suck call */
        else                            /* != OPS_MAIN */
            if (!opa_suckdriver(global, subquery->ops_next, mainsq, 
                                FALSE, isunion)) 
	        return(FALSE);		/* from here's where we recurse down */	

    }
    if (subquery->ops_next
	&&
	subquery->ops_next->ops_sqtype == OPS_MAIN
	&&
	subquery->ops_agg.opa_concurrent
	&&
	subquery->ops_agg.opa_concurrent->ops_next
	&&
	subquery->ops_agg.opa_concurrent->ops_next->ops_sqtype == OPS_MAIN)
	nextmain = TRUE;

    if (subquery->ops_next == NULL || 
	(subquery->ops_agg.opa_concurrent && (*isunion || nextmain)) ||
	subquery->ops_next->ops_sqtype == OPS_MAIN &&
	((root = subquery->ops_next->ops_root)->pst_sym.pst_type != PST_ROOT ||
	 root->pst_sym.pst_value.pst_s_root.pst_qlang != DB_SQL))
	return(FALSE);		/* one more consistency check */

    /* Once we're here, we have 2 adjacent subqueries. Call opa_suck
    ** to determine if something can be moved from the next subquery's
    ** where clause, into the current subquery. 
    **
    ** If we're in a union, we loop over all subqueries in current
    ** union set.
    */

    for (nsq = subquery->ops_next, usq = subquery; usq;
		usq = usq->ops_union)
    {
	/* Simply call opa_suck with current subquery - should provide 
	** enough information to allow the transform. NOTE: only the first
	** subquery in a chain of unions addresses the proper "next", 
	** unless the subquery has a "father". */

	if (usq->ops_agg.opa_father) nsq = usq->ops_agg.opa_father;
	opa_suck(global, usq, nsq);
    }

    /* Now repeat the loop, but using the "main" subquery as the 
    ** restriction source. This helps with joins of union views. */

    if ((*mainsq) != subquery->ops_next)
     for (usq = subquery; usq; usq = usq->ops_union)
    {
	if ((*mainsq) != usq->ops_agg.opa_father)	/* don't redo suck */
	    opa_suck(global, usq, *mainsq);
    }

    return(TRUE); 		/* and if we get this far, we're golden! */
}

/*{
** Name: OPA_SUCK 	- oversees "suck" process on one subquery level
**
** Description:
**	Next subquery (or subquery set, if it's a union) is compared
**	to current subquery, to determine if where clause fragments (or
**	the whole where clause) can be promoted to the current subquery.
**
** Inputs: global	- pointer to OPF global state variable
**	subquery	- pointer to current subquery being analyzed
**	msq		- pointer to "next" subquery (whose where bits
**			may be promoted)
**
** Outputs:
**	(Possibly) modified subquery structure.
**
** Returns: none
**
** Side effects: none
**
** History:
**	8-jun-98 (inkdo01)
**	    Written.
**	18-mar-03 (inkdo01)
**	    Add variable & function call to clean up VAR nodes.
**	10-july-06 (dougi)
**	    Changes to perform EQC closure and promotion of additional
**	    restrictions.
**	1-april-2008 (dougi) BUG 120175
**	    Make use of restriction movearound more restrictive.
**	25-jun-2008 (macem01) Bug 120554
**	    Added to the fix of bug 120175 with Doug's help because that fix
**	    caused certain queries to return E_OP0082.
**      22-aug-2008 (huazh01)
**          inspect subquery's query tree and remove any 'x = x' nodes.
**          Bug 120709. 
**	20-Jan-2010 (smeke01) b122967 b122556
**	    Prevent opa_suckrestrict() from being called on aggregate 
**	    subqueries. 
**      15-mar-2010 (huazh01)
**          Back up the pst_tvrm map and restore it if opa_suckchange()
**          produces a useless 'x = x' node. (b123425)
*/

static VOID
opa_suck( OPS_STATE	*global,
	OPS_SUBQUERY	*subquery,
	OPS_SUBQUERY	*msq)

{
    PST_QNODE	*andp, *n1, *n2;
    PST_QNODE	*vararray[32];
    i4		i, j, k = 0;
    bool	tidyvars, gotvars, match;
    PST_J_MASK  old_tvrm; 


    /* Load source subquery address (it might be a list of union's) and
    ** initiate the descent of its left subtree, looking for where 
    ** fragments which can be copied to the target subquery. 
    */

    for ( ; msq && msq->ops_sqtype == OPS_UNION; msq = msq->ops_next);
				/* if a union list, get to its end and
				** work backwards */

    for ( ; msq ; msq = msq->ops_union)
    {
	if (msq->ops_root == NULL || msq->ops_root->pst_right == NULL ||
	    msq->ops_root->pst_right->pst_sym.pst_type == PST_QLEND ||
	    msq->ops_root->pst_sym.pst_type != PST_ROOT &&
	    msq->ops_root->pst_sym.pst_type != PST_AGHEAD ||
	    subquery->ops_root->pst_sym.pst_type != PST_ROOT &&
	    subquery->ops_root->pst_sym.pst_type != PST_AGHEAD)
	 continue;		/* must satisfy all that to be useful */

	/* Prepare vararray for equivalence class machinations. */
	for (i = 0; i < 32; i++) vararray[i] = (PST_QNODE *) NULL;
	opa_suckeqc(global, subquery->ops_gentry, vararray, 
		subquery->ops_root->pst_left, &msq->ops_root->pst_right, TRUE,
		subquery);

	gotvars = FALSE;
	for (i = 0; i < 32; i++)
	 if (vararray[i])
	 {
	    /* 2nd pass to pick up closures (a = b && b = c). */
	    opa_suckeqc(global, subquery->ops_gentry, vararray, 
		subquery->ops_root->pst_left, &msq->ops_root->pst_right, FALSE,
		subquery);
	    gotvars = TRUE;
	 }

	tidyvars = FALSE;	/* init flag */
	if (opa_suckandcheck(global, subquery, &msq->ops_root->pst_right,
						vararray, &tidyvars))
	{
	    /* copy whole where clause from msq to current subquery */
	    andp = opv_opnode(global, PST_AND, (ADI_OP_ID)0, PST_NOJOIN);
	    andp->pst_left = msq->ops_root->pst_right;
	    andp->pst_right = subquery->ops_root->pst_right;
	    opv_copytree(global, &andp->pst_left);
				/* make copy of where fragment */

            /* b123425: back up pst_tvrm and restore it if we end up getting a 
            ** 'x = x' node.
            */
            MEcopy((PTR)&subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm, 
                   sizeof(PST_J_MASK), 
                   (PTR)&old_tvrm); 

	    opa_suckchange(global, subquery, &andp->pst_left, vararray);

	    /* First check if we copied "x = x". It's too silly to keep. */
	    if (andp->pst_left->pst_sym.pst_type == PST_BOP &&
		andp->pst_left->pst_sym.pst_value.pst_s_op.pst_opno ==
		    global->ops_cb->ops_server->opg_eq &&
		(n1 = andp->pst_left->pst_left)->pst_sym.pst_type == PST_VAR &&
		(n2 = andp->pst_left->pst_right)->pst_sym.pst_type == PST_VAR &&
		n1->pst_sym.pst_value.pst_s_var.pst_vno ==
		    n2->pst_sym.pst_value.pst_s_var.pst_vno &&
		n1->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id ==
		    n2->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
            {
                MEcopy((PTR)&old_tvrm,
                  sizeof(PST_J_MASK),
                  (PTR)&subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm);

                return;		/* all for naught */
            }
	    
	    subquery->ops_root->pst_right = andp;
				/* splice to current sq where clause */
	    opj_adjust(subquery, &subquery->ops_root->pst_right);
	}

	/* Must assure that msq doesn't have tables not in subquery - 
	** otherwise opa_suckrestrict() heuristic may not work. */

	/* Loop over "main" tables, then sq tables. If there is more than 
	** 1 main table not in the sq, it may not work. This could be 
	** enhanced by adding all the other main tables and joins, but
	** I expect the law of diminishing returns enters the picture
	** at some point. */
	for (i = -1; (i = BTnext(i, (char *)&msq->ops_root->pst_sym.
		pst_value.pst_s_root.pst_tvrm, (i4)(BITS_IN(PST_J_MASK)))) >= 0
			&& i < global->ops_qheader->pst_rngvar_count; )
	{
	    PST_RNGENTRY	*mrng = global->ops_qheader->pst_rangetab[i];

	    if (mrng->pst_rgtype != PST_TABLE)
		continue;		/* we only want actual tables */

	    for (j = -1, match = FALSE; (j = BTnext(j, (char *)&subquery->
		ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm,
		(i4)(BITS_IN(PST_J_MASK)))) >= 0 && !match &&
			j < global->ops_qheader->pst_rngvar_count; )
	    {
		PST_RNGENTRY	*srng = global->ops_qheader->pst_rangetab[j];

		if (srng->pst_rgtype != PST_TABLE)
		    continue;

		if (mrng->pst_rngvar.db_tab_base == 
				srng->pst_rngvar.db_tab_base &&
		    mrng->pst_rngvar.db_tab_index ==
				srng->pst_rngvar.db_tab_index)
		    match = TRUE;
	    }

	    if (!match)
		k++;
	}

	/* If there's at least one vararray[] entry, check for 
	** additional restreictions to promote. */
	if (k <= 1 && gotvars && subquery->ops_sqtype != OPS_PROJECTION &&
		subquery->ops_sqtype != OPS_UNION &&
		subquery->ops_sqtype != OPS_VIEW &&
		subquery->ops_sqtype != OPS_RFAGG &&
		subquery->ops_sqtype != OPS_RSAGG &&
        (!global->ops_cb->ops_check ||
            !opt_strace(global->ops_cb, OPT_F034_NEWFEATS)))
	    tidyvars |= opa_suckrestrict(global, subquery, 
				msq->ops_root->pst_right, vararray);

	/* Tidy up equivalence array */
	for (i = 0; i < 32; i++)
	 while (vararray[i])
	{
	    n1 = vararray[i];
	    vararray[i] = n1->pst_left;
	    n1->pst_left = (PST_QNODE *)NULL;
	}

	if (tidyvars)
	{
	    opa_sucktidyvars(subquery->ops_root->pst_right);
	    opa_sucktidyvars(msq->ops_root->pst_right);
				/* NULL all where clause VAR node 
				** pst_left's and pst_right's */
	}

	subquery->ops_normal = FALSE;
				/* just to make sure */
    }	/* end of union loop - and regular processing */

    /* check if the query tree contains any 'x = x' predicates.
    ** remove them if found. (b120709)
    */
    opa_predicate_check(&(subquery->ops_root->pst_right->pst_left),
                        &(subquery->ops_root->pst_right), TRUE);
    opa_predicate_check(&(subquery->ops_root->pst_right->pst_right),
                        &(subquery->ops_root->pst_right), FALSE);

    return;
}


/*{
** Name: OPA_SUCKANDCHECK	- recurse on where clause of "next"
**	subquery, coordinating where movement analysis.
**
** Description:
**	Recursively descends where clause. Focus of this routine is AND
**	fragments, since they represent the conjunctive factors of the 
**	where clause, and can be freely promoted to the current subquery.
**	If only one conjunctive factor under an AND is deemed to be
**	promoteable, it is copied here. If the whole where clause is
**	promoteable, it is copied by opa_suck.
**
**	A factor of the next subquery is promoteable if it contains 
**	a PST_VAR node that maps onto the result list (resdoms) of the 
**	current subquery.
**
** Inputs: global	- pointer to OPF global state variable
**	subquery	- pointer to current subquery structure
**	nodep		- pointer to node pointer of current where fragment
**	vararray	- array of PST_VAR ptrs representing eqclasses
**	tidyvars	- pointer to flag set when at least one 
**			promoteable factor is found and VARs must therefore
**			be tidied
**
** Outputs:
**	Possibly modified subquery structure.
**
** Returns: TRUE - if entire where fragment anchored by *nodep is
**	copyable, else FALSE.
**
** Side effects: none
**
** History:
**	8-jun-98 (inkdo01)
**	    Written.
**      15-mar-2010 (huazh01)
**          Back up the pst_tvrm map and restore it if opa_suckchange()
**          produces a useless 'x = x' node. (b123425)
*/

static bool
opa_suckandcheck(OPS_STATE 	*global, 
	OPS_SUBQUERY	*subquery, 
	PST_QNODE	**nodep,
	PST_QNODE	**vararray,
	bool		*tidyvars)

{
    bool	success, leftres, rightres, gotvar;
    PST_QNODE	*n1, *n2, *andp;
    PST_J_MASK  old_tvrm; 


    /* This code recursively descends the where clause of the "next"
    ** subquery. ANDs are analyzed more rigourously, since they anchor 
    ** conjunctive factors which can be promoted individually. Otherwise,
    ** the whole where expression must be promotable. 
    */

    switch ((*nodep)->pst_sym.pst_type) {	/* node type switch */

     case PST_AND:
	leftres = ((*nodep)->pst_left && opa_suckandcheck(global, subquery,
					&(*nodep)->pst_left, vararray, tidyvars));
	rightres = ((*nodep)->pst_right && opa_suckandcheck(global, subquery,
					&(*nodep)->pst_right,vararray, tidyvars));
	if (leftres && rightres) return(TRUE);
	if (leftres)
	{
	    n1 = (*nodep)->pst_left;
	    n2 = (*nodep)->pst_right;
	}
	else if (rightres)
	{
	    n1 = (*nodep)->pst_right;
	    n2 = (*nodep)->pst_left;
	}
	else return(FALSE);

	if (n1->pst_sym.pst_type == PST_QLEND) return(FALSE);

	/* Got promotable conjunctive factor (not two), so copy it
	** to current subquery where clause. */
	*tidyvars = TRUE;			/* need to tidy up later */
	andp = opv_opnode(global, PST_AND, (ADI_OP_ID)0, PST_NOJOIN);
	andp->pst_left = n1;
	opv_copytree(global, &andp->pst_left);	/* copy the sucker */

        /* b123425: back up pst_tvrm and restore it if we end up getting a
        ** 'x = x' node.
        */
        MEcopy((PTR)&subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm,
               sizeof(PST_J_MASK),
               (PTR)&old_tvrm);

	opa_suckchange(global, subquery, &andp->pst_left, vararray);

	/* First check if we copied "x = x". It's too silly to keep. */
	if (andp->pst_left->pst_sym.pst_type == PST_BOP &&
	    andp->pst_left->pst_sym.pst_value.pst_s_op.pst_opno ==
		global->ops_cb->ops_server->opg_eq &&
	    (n1 = andp->pst_left->pst_left)->pst_sym.pst_type == PST_VAR &&
	    (n2 = andp->pst_left->pst_right)->pst_sym.pst_type == PST_VAR &&
	    n1->pst_sym.pst_value.pst_s_var.pst_vno ==
		n2->pst_sym.pst_value.pst_s_var.pst_vno &&
	    n1->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id ==
		n2->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
        {
            MEcopy((PTR)&old_tvrm,
               sizeof(PST_J_MASK),
               (PTR)&subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm);

            return(FALSE);         /* all for naught */
        }
	    
	andp->pst_right = subquery->ops_root->pst_right;
	subquery->ops_root->pst_right = andp;
				/* finish up the splicing */
	subquery->ops_normal = FALSE;	/* force re-normalization */
	return(FALSE);

     case PST_UOP:
     case PST_BOP:
	if ((*nodep)->pst_sym.pst_value.pst_s_op.pst_joinid > 0)
		return(FALSE);		/* leave oj's alone */


     default:	/* all the rest just descend the parse tree */
	success = TRUE;
	gotvar = FALSE;
	if ((*nodep)->pst_left) success = opa_suckcheck(global, 
		subquery, &(*nodep)->pst_left, vararray, &gotvar);
	if (success && (*nodep)->pst_right) success = opa_suckcheck(global,
		subquery, &(*nodep)->pst_right, vararray, &gotvar);
	if (success && gotvar)
	    *tidyvars = TRUE;
	return(success && gotvar);
    }	/* end of switch */

}	/* end of opa_suckandcheck */


/*{
** Name: OPA_SUCKRESTRICT	- looks for other restrictions in WHERE 
**	fragments (other than those promoted by opa_suckand) to promote.
**
** Description:
**	Looks for restrictions comparing PST_VAR to PST_CONST where PST_VAR
**	is NOT in vararray, but some other column from the same table is.
**	The restriction is promoted to the current subquery, along with 
**	all equijoin predicates between other columns of the same table
**	and PST_VARs in the RESDOM list of the current subquery (i.e., 
**	PST_VARs in the vararray).
**
** Inputs: global	- pointer to OPF global state variable
**	subquery	- pointer to current subquery structure
**	nodep 		- ptr to where clause fragment under 
**			analysis
**	vararray	- array of PST_VAR ptrs representing eqclasses
**
** Outputs:
**	none
**
** Returns: TRUE, if at least one predicate was copied (and tidyvars 
**		must then be called)
**
** Side effects: none
**
** History:
**	7-july-06 (dougi)
**	    Written.
**	25-Oct-2007 (kibro01) b119333
**	    Ensure that "x=x" kind of equivalence doesn't get copied.
**	7-nov-2007 (dougi)
**	    Verify that we only add real tables to sq, not other (possibly
**	    independent) sq's.
**	 2-jun-08 (hayke02)
**	    Test for PST_7HAVING ('having [not] <agg>' query), and return if
**	    set. This change fixes bug 120553.
*/

static bool
opa_suckrestrict(OPS_STATE 	*global,
	OPS_SUBQUERY	*subquery,
	PST_QNODE	*nodep,
	PST_QNODE	**vararray)

{
    PST_QNODE	*rsdmp, *n1, *n2, *nx, *andp, *bop;
    OPV_IVARS	vno;
    OPZ_IATTS	atno;
    DB_STATUS	status;
    DB_ERROR	error;
    i4		i, j;
    bool	leftres, rightres, noeqc, copysome;


    /* Recursively descend WHERE clause, looking closely at BOPs. */
    switch (nodep->pst_sym.pst_type) {

      case PST_AND:
	/* Recurse on left and right. */
	leftres = opa_suckrestrict(global, subquery, nodep->pst_left, 
						vararray);
	rightres = opa_suckrestrict(global, subquery, nodep->pst_right,
						vararray);
	return(leftres | rightres);

      case PST_BOP:
	if (nodep->pst_sym.pst_value.pst_s_op.pst_joinid > 0)
	    return(FALSE);		/* dodge outer join issues */

	/* Check for "column op constant" or vice versa. */
	n1 = nodep->pst_left;
	n2 = nodep->pst_right;
	if (n1 == (PST_QNODE *) NULL || n2 == (PST_QNODE *) NULL)
	    return(FALSE);
	else if (n1->pst_sym.pst_type != PST_VAR ||
			n2->pst_sym.pst_type != PST_CONST)
	{
	    nx = n1; n1 = n2; n2 = nx;	/* swap 'em */
	    if (n1->pst_sym.pst_type != PST_VAR ||
			n2->pst_sym.pst_type != PST_CONST)
		return(FALSE);		/* test again for VAR op CONST */
	}

	/* n1 is PST_VAR, n2 is PST_CONST - now check the remaining
	** conditions: the n1 VAR isn't already in vararray (else suckchange
	** would have already copied this restriction) but some other VAR
	** from the same table is. */
	if ((vno = (OPV_IGVARS)n1->pst_sym.pst_value.pst_s_var.pst_vno) == 
						subquery->ops_gentry)
	    return(FALSE);		/* no VARs that map to sq's rsdms */
	atno = n1->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
	noeqc = TRUE;
	copysome = FALSE;

	for (i = 0; i < 32; i++)
	 for (nx = vararray[i]; nx; nx = nx->pst_left)
	  if (nx->pst_sym.pst_value.pst_s_var.pst_vno == vno)
	  {
	    if (nx->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id == atno)
		return(FALSE);		/* restriction already copied */
	    noeqc = FALSE;		/* at least 1 eqc col from same tab */

	    if (nx->pst_right != (PST_QNODE *) -1)
		copysome = TRUE;	/* at least one eqc predicate to do */
	  }

	if (noeqc)
	    return(FALSE);		/* no matching equijoin - return */

	/* don't do it if predicate involves a "generated"
	** table (not a base table/view). */
	if (global->ops_rangetab.opv_base->opv_grv[vno]->opv_created != 0)
	    return(FALSE);

	/* don't do it if we have 'HAVING [NOT] <AGG>' query */
	if (subquery->ops_root->pst_sym.pst_type == PST_AGHEAD
	    &&
	    subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_mask1 & PST_7HAVING)
		    return(FALSE);

	/* If we get here, restriction needs to be copied. */
	andp = opv_opnode(global, PST_AND, (ADI_OP_ID) 0, PST_NOJOIN);
	andp->pst_left = nodep;
	opv_copytree(global, &andp->pst_left);
					/* replicate restriction */
	andp->pst_right = subquery->ops_root->pst_right;
					/* splice into sq's WHERE clause */
	subquery->ops_root->pst_right = andp;

	if (!copysome)
	    return(TRUE);		/* no eqc's to do */

	/* Now loop over vararray again to fabricate equij's between other
	** columns from same table as restriction operand and current sq. */
	BTset(vno, (char *)&subquery->ops_root->pst_sym.pst_value.pst_s_root.
						pst_tvrm);

	for (i = 0; i < 32; i++)
	 for (nx = vararray[i]; nx; nx = nx->pst_left)
	  if (nx->pst_sym.pst_value.pst_s_var.pst_vno == vno)
	   if (nx->pst_right == (PST_QNODE *) -1)
	    break;			/* this guy's already been done */
	   else
	   {
	    /* Got one. Get the corresponding entry in current sq RESDOM
	    ** list and make a "x = y" from them. */
	    for (rsdmp = subquery->ops_root->pst_left, j = 0;
		rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
		rsdmp = rsdmp->pst_left, j++)
	     if (i == j)
	     {
		bop = opv_opnode(global, PST_BOP, (ADI_OP_ID) 0, PST_NOJOIN);
		bop->pst_left = rsdmp->pst_right;
		opv_copytree(global, &bop->pst_left);
		bop->pst_right = nx;
		opv_copytree(global, &bop->pst_right);

		/* Do a quick sanity check to avoid x=x (kibro01) b119333 */
		if (bop->pst_left->pst_sym.pst_type == PST_VAR &&
		    bop->pst_right->pst_sym.pst_type == PST_VAR &&
		    bop->pst_left->pst_sym.pst_value.pst_s_var.pst_vno ==
			bop->pst_right->pst_sym.pst_value.pst_s_var.pst_vno &&
		    bop->pst_left->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id ==
			bop->pst_right->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
		    continue;
		    
		andp = opv_opnode(global, PST_AND, (ADI_OP_ID)0, PST_NOJOIN);
		andp->pst_left = bop;
		andp->pst_right = subquery->ops_root->pst_right;
		subquery->ops_root->pst_right = andp;
		bop->pst_left->pst_left = (PST_QNODE *) NULL;
		nx->pst_right = (PST_QNODE *) -1;

		/* Turn bop into "=". */
		bop->pst_sym.pst_value.pst_s_op.pst_opno = 
			global->ops_cb->ops_server->opg_eq;
		status = pst_resolve((PSF_SESSCB_PTR) NULL,
			global->ops_adfcb, bop,
			nodep->pst_sym.pst_value.pst_s_root.pst_qlang, 
			&error);
		if (status != E_DB_OK)
		    opx_verror(status, E_OP0685_RESOLVE, error.err_data);

		break;
	     }
	   }

	return(TRUE);			/* finally, ... */

      default:
	/* Everything else (including ORs)returns straightaway. */
	return(FALSE);
    }

}


/*{
** Name: OPA_SUCKEQC  	- looks for PST_BOPs representing conjunctive
**	equijoin factors which might enhance the propagation of restriction
**	predicates.
**
** Description:
**	Builds array of sets of equivalent PST_VAR nodes in order to 
**	expand potential to promote restriction predicates from next
**	subquery to current subquery. For each conjunctive equijoin predicate
**	between 2 PST_VAR's, one of which is based on the result of the 
**	current subquery, an entry is made in the array for the other VAR
**	node (showing its equivalence with the result VAR).
**
** Inputs: global	- pointer to OPF global state variable
**	aggvar 		- range table variable number corresponding to result
**			of current subquery
**	vararray	- array of PST_QNODE ptrs to sets of equivalent
**			PST_VAR nodes
**	resdomp		- address of current subquery RESDOM list
**	nodep 		- ptr to pointer to where clause fragment under 
**			analysis
**
** Outputs:
**	none
**
** Returns: none
**
** Side effects: none
**
** History:
**	10-jun-98 (inkdo01)
**	    Written.
**	27-apr-00 (inkdo01)
**	    Add logic to avoid promoting where restrictions on inner
**	    tables of outer joins.
**	6-july-06 (dougi)
**	    Add EQC's when "a = b and b = c".
**      16-aug-07 (huazh01)
**          add all qualified PST_VAR node found during the second pass
**          of the call into vararray[].  
**          This fixes b118964.
**	3-Sep-2009 (kibro01) b122556
**	    Don't pull a new table into a correlated aggregate subquery
**	    since it must be evaluated separately and you can end up 
**	    pulling the entire upper query in and evaluating everything
**	    multiple times.
**	20-Jan-2010 (smeke01) b122967 b122556
**	    Back out first attempt at a ffb 122556, as this was 
**	    preventing valid predicates from being pulled in. Also correct
**	    an old coding error in the var1/var2 for-loops for the sake 
**	    of readability.
*/

static VOID
opa_suckeqc(OPS_STATE 	*global,
	OPV_IGVARS	aggvar,
	PST_QNODE	**vararray,
	PST_QNODE	*resdomp,
	PST_QNODE	**nodep,
	bool		firstcall,
	OPS_SUBQUERY	*subquery)

{
    PST_QNODE	*n1, *n2, *var1, *var2;
    i4		i;


    /* Recurse down through AND's looking for BOPs which represent
    ** VAR = VAR. Anything else just returns. The VAR = VAR BOPs 
    ** are checked to see if either VAR is in resdomp list of
    ** outer subquery. If so, the OTHER VAR is added to its 
    ** VAR array as an equivalent node. */

    switch ((*nodep)->pst_sym.pst_type) {	/* switch on node type */

     case PST_AND:
	if ((*nodep)->pst_left) 
	    opa_suckeqc(global, aggvar, vararray, 
				resdomp, &(*nodep)->pst_left, firstcall, subquery);
				/* recurse on left */
	if ((*nodep)->pst_right) 
	    opa_suckeqc(global, aggvar, vararray, 
				resdomp, &(*nodep)->pst_right, firstcall, subquery);
				/* and recurse on right */
	break;

    case PST_BOP:
	if (!((*nodep)->pst_sym.pst_value.pst_s_op.pst_opno ==
		global->ops_cb->ops_server->opg_eq &&
	    (*nodep)->pst_sym.pst_value.pst_s_op.pst_joinid <= 0 &&
	    (var1 = (*nodep)->pst_left)->pst_sym.pst_type == PST_VAR &&
	    (var2 = (*nodep)->pst_right)->pst_sym.pst_type == PST_VAR)) 
	 break;			/* not a VAR = VAR */

	if (var1->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id ==
	    var2->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id &&
	    var1->pst_sym.pst_value.pst_s_var.pst_vno ==
	    var2->pst_sym.pst_value.pst_s_var.pst_vno) break;
				/* silly case of "x = x" */

	/* Now see if var1, then var2, can be found amongst the 
	** resdoms of the containing subquery. If so, add the OTHER
	** var to the array anchored by pst_rsno of resdom. */

	if (firstcall && var1->pst_sym.pst_value.pst_s_var.pst_vno == aggvar)
	 for (n1 = resdomp, i = 0; n1 && i < 32 && n1->pst_sym.pst_type == PST_RESDOM; 
			n1 = n1->pst_left, i++)
	  if (n1->pst_sym.pst_value.pst_s_rsdm.pst_rsno == 
		var1->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
	   if (n1->pst_right->pst_sym.pst_type == PST_VAR)
	{	/* got our condition - splice VAR into array */
	    n2 = vararray[i];
	    vararray[i] = var2;
	    var2->pst_left = n2;
	    return;	/* only one VAR can be attached */
	}
	   else break;
	/* end of var1 tests */

	if (firstcall && var2->pst_sym.pst_value.pst_s_var.pst_vno == aggvar)
	 for (n1 = resdomp, i = 0; n1 && i<32 && n1->pst_sym.pst_type == PST_RESDOM; 
			n1 = n1->pst_left, i++)
	  if (n1->pst_sym.pst_value.pst_s_rsdm.pst_rsno == 
		var2->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
	   if (n1->pst_right->pst_sym.pst_type == PST_VAR)
	{	/* got our condition - splice VAR into array */
	    n2 = vararray[i];
	    vararray[i] = var1;
	    var1->pst_left = n2;
	    return;	/* only one VAR can be attached */
	}
	   else break;
	/* end of var2 tests */

	/* If first pass to identify matches with RESDOMs of current
	** subquery is done, do it again to find matches with matches
	** (a.k.a. closure). For now, this only does one further pass
	** through the WHERE clause, though conceivably sequences of
	** equijoin predicates could require additional passes until
	** no more entries are added (just need to pass back a flag
	** to do that). */
	if (!firstcall)
	 for (i = 0; i < 32; i++)
	 {
	    for (n1 = vararray[i]; n1; n1 = n1->pst_left)
	     if (n1->pst_sym.pst_value.pst_s_var.pst_vno == 
		    var1->pst_sym.pst_value.pst_s_var.pst_vno &&
		n1->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id ==
		    var1->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
	     {
		for (n2 = vararray[i]; n2; n2 = n2->pst_left)
		 if (var2 == n2) 
		    break;

		/* If var2 is not already in list, insert it. */
		if (n2 == (PST_QNODE *) NULL)
		{
		    n2 = vararray[i];
		    vararray[i] = var2;
		    var2->pst_left = n2;
		    break;
		}
	     }
	     else if (n1->pst_sym.pst_value.pst_s_var.pst_vno == 
		    var2->pst_sym.pst_value.pst_s_var.pst_vno &&
		n1->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id ==
		    var2->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
	     {
		for (n2 = vararray[i]; n2; n2 = n2->pst_left)
		 if (var1 == n2) 
		    break;

		/* If var1 is not already in list, insert it. */
		if (n2 == (PST_QNODE *) NULL)
		{
		    n2 = vararray[i];
		    vararray[i] = var1;
		    var1->pst_left = n2;
		    break;
		}
	     }
	 }
	/* end of !firstcall tests */

	break;

     default:	/* all others are uninteresting */
	break;
    }

    return;

}	/* end of opa_suckeqc */

/*{
** Name: OPA_SUCKCHECK	- looks for PST_VARs in where fragments to 
**	compare with current subquery RESDOM list.
**
** Description:
**	If **nodep addresses PST_VAR (in where fragment), checks if VAR
**	maps onto current subquery output list (varno matches subquery's
**	ops_gentry, atno is amongst subquery's RESDOM list). Otherwise,
**	recurses.
**
** Inputs: global	- pointer to OPF global state variable
**	subquery	- pointer to current subquery structure
**	nodep 		- ptr to pointer to where clause fragment under 
**			analysis
**	vararray	- array of PST_VAR ptrs representing eqclasses
**	gotvar 		- flag indicating whether where fragment has 
**			at least one PST_VAR (i.e. isn't a constant)
**
** Outputs:
**	none
**
** Returns: TRUE, if fragment can be copied to current subquery
**
** Side effects: none
**
** History:
**	8-jun-98 (inkdo01)
**	    Written.
**	27-apr-00 (inkdo01)
**	    Add logic to avoid promoting where restrictions on inner
**	    tables of outer joins.
**	5-july-01 (inkdo01)
**	    Fix bug sucking factors referencing aggregate computed in 
**	    subquery doing the sucking.
**	12-jun-01 (hayke02 for inkdo01)
**	    The test and transformation of OPS_PROJECTION subqueries
**	    has been temporarily disabled in order to avoid restrictions
**	    from other parts of the query being applied to these subqueries
**	    and causing queries to return incorrect results. This change
**	    fixes bug 104818.
**	30-oct-01 (hayke02)
**	    When we find a matching PST_VAR we now check that the equivalent
**	    resdoms in other union view subqueries are also based on simple
**	    PST_VAR nodes. If this is not the case we reject this PST_VAR.
**	    This change fixes bug 105329.
**      13-dec-02 (hayke02)
**          Extend fix for 105167/105080 so that we don't copy back binary
**          operations (PST_BOP's). This change fixes bug 109305.
**	20-Feb-2009 (kibro01) b121615
**	    An inner join produced by the syntax "A join B on A.a=B.a" appears
**	    in the pst_inner_rel list, but both sides are registered as inner.
**	    To avoid eliminating propagatable restrictions in such circumstances
**	    confirm when preventing outer join propagation that this actually
**	    the inner side of an outer join by going through rngvars to check.
**      23-Jul-2009 (huazh01)
**          Modify the fix to b121615 by checking if pst_rangetab[rngvars] is
**          NULL before we do the BTtest(). (b122339)
*/

static bool
opa_suckcheck(OPS_STATE 	*global,
	OPS_SUBQUERY	*subquery,
	PST_QNODE	**nodep,
	PST_QNODE	**vararray,
	bool		*gotvar)

{
    PST_QNODE	*resdomp, *n1, *n2;
    OPV_IGVARS	aggvar;
    bool	success = TRUE;
    i4		i;
    bool	ok_to_push;
    char	*inner_rel;


    /* Recursively descend where fragment, avoiding outer join
    ** expressions and looking for VAR nodes who have a match in the
    ** next outer subquery's RESDOM list. */

    resdomp = subquery->ops_root->pst_left;
    aggvar = subquery->ops_gentry;

    switch ((*nodep)->pst_sym.pst_type) {	/* switch on node type */

     case PST_AOP:	/* these aren't useful */
	return(FALSE);

     case PST_VAR:
	/* See if this VAR is in resdom list of outer subquery. If so,
	** it's eligible for copying. */

	if ((*nodep)->pst_sym.pst_value.pst_s_var.pst_vno == aggvar)
	{
	    ok_to_push = TRUE;

	    if (global->ops_qheader->pst_rngvar_count > aggvar)
	    {
	        if (ok_to_push && !global->ops_qheader->pst_rangetab[aggvar])
		    ok_to_push = FALSE;

	        inner_rel = (char *)&global->ops_qheader->pst_rangetab[aggvar]
					->pst_inner_rel;

	        if (ok_to_push &&
		    BTcount(inner_rel, (i4)BITS_IN(PST_J_MASK)) != 0)
	        {
		/* We know we've got an inner join, but until we check we don't
		** know for certain if the other side is an outer join or also
		** inner (using the A join B on A.a=B.a syntax gives an 
		** inner_rel value, but with inners on both sides).
		** (kibro01) b121615
		*/
		    i4 our_inners;
		    i4 rngvars;
		
		    for (our_inners = BTnext((i4)-1, inner_rel,
			    (i4)BITS_IN(PST_J_MASK));
		         our_inners >= 0 && ok_to_push;
		         our_inners = BTnext(our_inners, inner_rel,
			    (i4)BITS_IN(PST_J_MASK)))
		    {
			for (rngvars = 0;
			     rngvars<global->ops_qheader->pst_rngvar_count;
			     rngvars++)
			{
			    if (global->ops_qheader->pst_rangetab[rngvars] &&
                                BTtest(rngvars,(char*)&global->ops_qheader->
					pst_rangetab[rngvars]->pst_outer_rel))
			    {
				ok_to_push = FALSE;
			    }
			}
		    }
	        }
	    }

	    if (!ok_to_push)
            {
		return(FALSE);		/* don't promote restrs on inners */
            }
	    else
            {
		for (n1 = resdomp; n1 && n1->pst_sym.pst_type == PST_RESDOM;
		     n1 = n1->pst_left)	/* loop over RESDOMs */
	        {
	            if (n1->pst_right->pst_sym.pst_type == PST_VAR &&
		        n1->pst_sym.pst_value.pst_s_rsdm.pst_rsno ==
		        (*nodep)->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id
		       )
	            {	/* got a match! */
	    /* Bug 105329 - check union view for resdoms not based on a var */
	    /* [inkdo01 19/3/3] - this code might be better placed after 
	    ** the vararray search logic to assure its execution regardless
	    ** of how the VAR node is found. */
			OPS_SUBQUERY	    *vsq = (OPS_SUBQUERY *)NULL;
			OPS_SUBQUERY	    *usq;
			bool		    found_view = FALSE;

			if (subquery->ops_sqtype == OPS_VIEW &&
				 subquery->ops_union)
			    vsq = subquery;
			else if (subquery->ops_sqtype == OPS_UNION)
			{
			    for (vsq = global->ops_subquery; vsq;
				 vsq = vsq->ops_next)
			    {
				if (vsq->ops_sqtype == OPS_VIEW &&
					vsq->ops_union)
				{
				    for (usq = vsq->ops_union;usq;
					 usq = usq->ops_union)
				    {
					if (usq == subquery)
					{
					    found_view = TRUE;
					    break;
					}
				    }
				    if (found_view)
					break;
				}
			    }
			}

			if (vsq)
			{
			    for (usq = vsq; usq; usq = usq->ops_union)
			    {
				if (usq == subquery)
				    continue;

				for (n2 = usq->ops_root->pst_left;
				    n2 && n2->pst_sym.pst_type == PST_RESDOM;
				    n2 = n2->pst_left)
				{
				    if ((n2->pst_sym.pst_value.pst_s_rsdm.pst_rsno == 
					(*nodep)->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
					&&
					n2->pst_right->pst_sym.pst_type != PST_VAR)
					return(FALSE);
				}
			    }
			}
			*gotvar = TRUE;
			return(TRUE);
		    }
		    else if (((n1->pst_right->pst_sym.pst_type == PST_AOP) ||
			    (n1->pst_right->pst_sym.pst_type == PST_BOP)) &&
			n1->pst_sym.pst_value.pst_s_rsdm.pst_rsno ==
			(*nodep)->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id
			    )
		    {	/* Can't copy back an agg or binary op  - it isn't
			** computed until subquery is complete. */
			return(FALSE);
		    }
		}
	    }
	}

	/* No match directly, now search for a match on an equivalent
	** VAR node in vararray. This succeeds if the VAR node is the
	** same as some other VAR node which was found in an equijoin
	** predicate with a VAR node which maps onto this subquery's
	** RESDOM list. */
	for (i = 0, n1 = vararray[0]; i < 32; i++, n1 = vararray[i])
	 while (n1) {
	    if (n1->pst_sym.pst_value.pst_s_var.pst_vno ==
		(*nodep)->pst_sym.pst_value.pst_s_var.pst_vno &&
		n1->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id ==
		(*nodep)->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
	    {	/* got a match! */
		*gotvar = TRUE;
		return(TRUE);
	    }
	    n1 = n1->pst_left;
	}

	/* Final attempt is for OPS_PROJECTION subqueries only. For 
	** reasons unknown, they can have RESDOMs owning PST_VARs which
	** exactly match a PST_VAR from the "next" subquery's where
	** clause. If this is the case, the where fragment can be copied
	** with no change. */
	if (FALSE && (subquery->ops_sqtype == OPS_PROJECTION))
	 for (n1 = resdomp; n1 && n1->pst_sym.pst_type == PST_RESDOM;
		n1 = n1->pst_left)	/* loop over RESDOMs */
	  if ((n2 = n1->pst_right)->pst_sym.pst_type == PST_VAR &&
		n2->pst_sym.pst_value.pst_s_var.pst_vno ==
		(*nodep)->pst_sym.pst_value.pst_s_var.pst_vno &&
		n2->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id ==
		(*nodep)->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
	 {	/* got a match! */
	    /* One last check - PROJECTION transforms can only be performed
	    ** if the corresponding aggregate computation doesn't involve
	    ** count(*). Count semantics may conflict with the resulting
	    ** PROJECTION/[R]FAGG. */
	    for (n1 = subquery->ops_next->ops_root->pst_left; 
		n1 && n1->pst_sym.pst_type == PST_RESDOM; n1 = n1->pst_left)
	     if (n1->pst_right && 
		n1->pst_right->pst_sym.pst_type == PST_AOP &&
		(n1->pst_right->pst_sym.pst_value.pst_s_op.pst_opno == 
			global->ops_cb->ops_server->opg_scount ||
		n1->pst_right->pst_sym.pst_value.pst_s_op.pst_opno == 
			global->ops_cb->ops_server->opg_count))
	     return(FALSE);
	    *gotvar = TRUE;
	    return(TRUE);
	 }

	return(FALSE);		/* no match */

     default:	/* all the rest just descend the tree */
	if ((*nodep)->pst_left) success = opa_suckcheck(global, subquery,
		&(*nodep)->pst_left, vararray, gotvar);
	if (success && (*nodep)->pst_right) success = opa_suckcheck(global,
		subquery, &(*nodep)->pst_right, vararray, gotvar);
	return(success && *gotvar);
    }	/* end of type switch */

}	/* end of opa_suckcheck */

/*{
** Name: OPA_SUCKCHANGE 	- edits a where clause fragment copied
**	to current subquery.
**
** Description:
**	Finds PST_VAR nodes in copied where clause fragment, then locates
**	corresponding RESDOM in current subquery and replaces PST_VAR
**	by RESDOM's VAR contents. This effectively resolves the PST_VAR
**	node to the context of the current subquery.
**
** Inputs: global	- pointer to OPF global state variable
**	subquery	- pointer to current subquery
**	nodep		- ptr to pointer to where fragment (now copied
**			to current subquery where clause) to resolve
**	vararray	- array of PST_VAR ptrs representing eqclasses
**
** Outputs:
**	Modified PST_VAR nodes.
**
** Returns: none
**
** Side effects: none
**
** History:
**	8-jun-98 (inkdo01)
**	    Written.
**	25-jul-02 (inkdo01)
**	    Fix to bug when restriction applied to column in union view that 
**	    is nullable in one select but not another (bug 108387).
**	21-feb-03 (inkdo01)
**	    Tidy up pst_left's of copied PST_VAR's.
**	18-mar-03 (inkdo01)
**	    Re-instate 2nd portion of VAR search logic (fix bug in SQL test).
*/

static VOID
opa_suckchange(OPS_STATE 	*global,
	OPS_SUBQUERY	*subquery,
	PST_QNODE	**nodep,
	PST_QNODE	**vararray)

{
    PST_QNODE	*n1, *n2;
    i4		i;


    /* Recursively descend the where clause fragment, looking for 
    ** PST_VAR node whose varno matches the ops_gentry of the containing
    ** subquery. The VAR node must then be mapped using the corresponding
    ** RESDOM node in the subquery's resdom list. If the varno does NOT
    ** match ops_gentry, the VAR node must be here by virtue of its
    ** equivalence to one which does. The vararray is used to locate the
    ** VAR, and the mapping through resdom is then done.
    */

    switch ((*nodep)->pst_sym.pst_type) {	/* node type */

     case PST_VAR:
	if ((*nodep)->pst_sym.pst_value.pst_s_var.pst_vno == 
						subquery->ops_gentry)
	 for (n1 = subquery->ops_root->pst_left; 
		n1 && n1->pst_sym.pst_type == PST_RESDOM; n1 = n1->pst_left)
	  if (n1->pst_sym.pst_value.pst_s_rsdm.pst_rsno ==
		(*nodep)->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id &&
		n1->pst_right->pst_sym.pst_type == PST_VAR)
	    {
		STRUCT_ASSIGN_MACRO(n1->pst_right->pst_sym.pst_value.pst_s_var,
			(*nodep)->pst_sym.pst_value.pst_s_var);
				/* copy PST_VAR contents */
		STRUCT_ASSIGN_MACRO(n1->pst_right->pst_sym.pst_dataval,
			(*nodep)->pst_sym.pst_dataval);
				/* & its data type info */
		BTset((*nodep)->pst_sym.pst_value.pst_s_var.pst_vno,
		    (char *)&subquery->ops_root->pst_sym.pst_value.
		    pst_s_root.pst_tvrm);
				/* set var in table mask, to assure 
				** it's copied into sq range table */
		return;
	    }

	/* If its pst_vno isn't current sq's ops_gentry, search the EQC
	** array for a PST_VAR in a "=" predicate with a pst_vno that
	** matches the current sq's ops_gentry. */

	if ((*nodep)->pst_sym.pst_value.pst_s_var.pst_vno !=
						subquery->ops_gentry)
	 for (i = 0, n1 = subquery->ops_root->pst_left, n2 = vararray[0]; 
		n1 && i < 32; i++, n1 = n1->pst_left, n2 = vararray[i])
	  while(n2) {
	    if (n2->pst_sym.pst_value.pst_s_var.pst_vno ==
		(*nodep)->pst_sym.pst_value.pst_s_var.pst_vno &&
		n2->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id ==
		(*nodep)->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
	    {
		STRUCT_ASSIGN_MACRO(n1->pst_right->pst_sym.pst_value.pst_s_var,
			(*nodep)->pst_sym.pst_value.pst_s_var);
				/* copy PST_VAR contents */
		STRUCT_ASSIGN_MACRO(n1->pst_right->pst_sym.pst_dataval,
			(*nodep)->pst_sym.pst_dataval);
				/* & its data type info */
		BTset((*nodep)->pst_sym.pst_value.pst_s_var.pst_vno,
		    (char *)&subquery->ops_root->pst_sym.pst_value.
		    pst_s_root.pst_tvrm);
				/* set var in table mask, to assure 
				** it's copied into sq range table */
		return;
	    }
	    n2 = n2->pst_left;
	}
	break;

     default:	/* everything else just recurses */
	if ((*nodep)->pst_left) opa_suckchange(global, subquery, 
					&(*nodep)->pst_left, vararray);
	if ((*nodep)->pst_right) opa_suckchange(global, subquery,
					&(*nodep)->pst_right, vararray);
	break;
    }	/* end of switch */

}	/* end of opa_suckchange */

/*{
** Name: OPA_SUCKTIDYVARS	- clean up where clause VAR nodes
**
** Description:
**	Recursively locates VAR nodes in modified where clause and
**	makes pst_left NULL (might have been non-null because it was
**	copied and participated in EQC array). This is done after all
**	adjustments to the where clause to prevent removal of entries
**	still participating in the EQC array.
**
** Inputs: nodep 	- pointer to where fragment currently being fixed
**
** Outputs:
**	Modified PST_VAR nodes.
**
** Returns: none
**
** Side effects: none
**
** History:
**	18-mar-03 (inkdo01)
**	    Written.
*/

static VOID
opa_sucktidyvars(PST_QNODE	*nodep)
{

    /* Just a switch that recurses on everything recurseable and cleans
    ** up the PST_VAR nodes. */
    if (nodep)
     switch (nodep->pst_sym.pst_type) {

      case PST_VAR:
	/* The only ones we care about. */
	nodep->pst_left = nodep->pst_right = (PST_QNODE *) NULL;
	return;

      case PST_TREE:
      case PST_QLEND:
	return;

      default:
	if (nodep->pst_left)
	    opa_sucktidyvars(nodep->pst_left);

	if (nodep->pst_right)
	    opa_sucktidyvars(nodep->pst_right);

	return;
    }

}

/*{
** Name: OPA_PREDICATE_CHECK  - check if query tree contains 'x = x' node.
**
** Description:
**      opa_suck() could produce a query tree containing 'x = x' predicates. 
**      This routine is to walk through a query tree recursively and check
**      whether it contains a 'x = x' predicate. If found one, modify the 
**      tree by removing it.
**
** Inputs: andp        - ptr to pointer of a fragment of the tree.
**         parent      - ptr to pointer of the parent node of 'andp'.
**
** Outputs:
**      Modified PST_QNODE trees.
**
** Returns: none
**
** Side effects: none
**
** History:
**      21-Aug-2008 (huazh01)
**          Written for bug 120709.
**	12-Jan-2009 (kibro01) b121493
**	    Explicitly specify whether we are checking the left or right.
**	    If we find "x=x" on the left, we need to go right, but if we find
**	    it on the right, the parent node should move to the left.
**	    Just setting *parent = *parent->pst_right may lose the 
**	    wrong half of the subtree.
*/
static VOID
opa_predicate_check(
        PST_QNODE       **andp,
        PST_QNODE       **parent,
	bool		looking_left)
{
    PST_QNODE *n1, *n2;

    if ((*andp) == (PST_QNODE *)NULL ||
        (*andp)->pst_left == (PST_QNODE *)NULL ||
        (*andp)->pst_right == (PST_QNODE *)NULL)
        return;

    n1 = (*andp)->pst_left;
    n2 = (*andp)->pst_right;

    /* if we found a 'x = x' node, remove it. Afer that,  
    ** recursively walks through the left and right
    ** side of the tree. 
    */ 
    if((*andp)->pst_sym.pst_type != PST_UOP  &&
        n1 != (PST_QNODE *)NULL              &&
        n2 != (PST_QNODE *)NULL              &&
        n1->pst_sym.pst_type == PST_VAR      &&
        n2->pst_sym.pst_type == PST_VAR      &&
        n1->pst_sym.pst_value.pst_s_var.pst_vno ==
            n2->pst_sym.pst_value.pst_s_var.pst_vno         &&
        n1->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id ==
            n2->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id
      )
    {
        *parent = looking_left ? (*parent)->pst_right : (*parent)->pst_left;
        opa_predicate_check(&(*parent)->pst_left, parent, TRUE);
        opa_predicate_check(&(*parent)->pst_right, parent, FALSE);
        return;
    }

    /* otherwise, check the left side of the tree */
    opa_predicate_check(&(*andp)->pst_left, andp, TRUE);

    n1 = (*andp)->pst_left; 
    n2 = (*andp)->pst_right; 

    /* query tree could be modified after the above 
    ** opa_predicate_check() returned. check if a 'x = x' node 
    ** is hanging right below the current 'andp'. if yes, remove 
    ** it and and recursively walks through the left and right side
    ** of the tree. 
    */
    if((*andp)->pst_sym.pst_type != PST_UOP  && 
        n1 != (PST_QNODE *)NULL              &&
        n2 != (PST_QNODE *)NULL              &&
        n1->pst_sym.pst_type == PST_VAR      &&
        n2->pst_sym.pst_type == PST_VAR      &&
        n1->pst_sym.pst_value.pst_s_var.pst_vno ==
            n2->pst_sym.pst_value.pst_s_var.pst_vno         &&
        n1->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id ==
            n2->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id
      )
    {
        *parent = looking_left ? (*parent)->pst_right : (*parent)->pst_left;
        opa_predicate_check(&(*parent)->pst_left, parent, TRUE);
        opa_predicate_check(&(*parent)->pst_right, parent, FALSE);
        return;
    }

    /* check the right side of the tree */
    opa_predicate_check(&(*andp)->pst_right, andp, FALSE);
    return;
}


/*
** Name: opa_hashagg - Convert sorted aggregations to hash aggregations
**
** Description:
**     At the very end of opa processing, aggregate subqueries are
**     labelled as one of OPS_SAGG or OPS_RSAGG for simple (scalar)
**     aggregates;  or OPS_FAGG or OPS_RFAGG for function aggregates.
**     This routine attempts to relabel OPS_RFAGG subqueries into
**     OPS_HFAGG subqueries, so that hash aggregation is used instead
**     of sorted aggregation.
**
**     It's advantageous to do this transform early, since with a
**     hash-agg on top, enumeration won't trip itself up attempting
**     to produce a sorted ordering to match a sorted-agg.  This
**     in turn makes it easier to transform merge joins into hash
**     joins after enumeration is complete.  After all join
**     transformation, if there happens to be an appropriate
**     sort-ordering, opj might change the HFAGG back into an RFAGG.
**
**     Hash aggregation can be used if:
**     - the use-hash-join setting is ON (the default);
**     - the BY-columns do not include one of the data types
**       that is not supported by hash aggregation (*);
**     - No duplicate removal (agg distinct) in this agg
**     - No duplicate removal in the next (outer) subquery, OR
**       that next outer one is SQL (this avoids some Quel oddities,
**       says Doug).
**     - the total key length is not larger than DB_MAXSTRING
**       (we could work around this by hashing in pieces, but really!)
**
**     (*) Hash aggregation, and hash join, do not currently use
**     the DMF-style hashprep preprocessing that ensures that two
**     values that are "equal" have identical bit patterns.
**     The manual setup used by OPC does not support all data
**     types, and any that are not supported have to use sorted
**     aggregation.
**
** Inputs:
**     global                  Optimization state control block
**
** Outputs:
**     none (subquery types possibly changed).
**
** History:
**     31-Aug-2006 (kschendel)
**         Written to enable more adventurous hash-join transforms.
*/

static void
opa_hashagg(OPS_STATE *global)
{
    bool ok;
    DB_DT_ID vartype;          /* abs of BY-expr var datatype */
    i4 hashlen;                        /* Length of hashagg BY-columns */
    OPS_SUBQUERY *nextsq;      /* The next (outer) subquery */
    OPS_SUBQUERY *sq;          /* The current subquery */
    PST_QNODE *byexpr;         /* BY-expression resdom node */
    PST_QNODE *varp;           /* BY-expression var node */

    if (! global->ops_cb->ops_alter.ops_hash)
	return;                         /* Skip all this if SET NOHASH */

    for (sq = global->ops_subquery; sq != NULL; sq = sq->ops_next)
    {
	if (sq->ops_sqtype == OPS_MAIN)
	    break;                      /* MAIN is at the end */
	if (sq->ops_sqtype != OPS_RFAGG)
	    continue;                   /* Only care about RFAGG's */
	if (sq->ops_duplicates == OPS_DREMOVE)
	    continue;                   /* Can't do agg distinct with hash */
	nextsq = sq->ops_next;
	if (nextsq != NULL && nextsq->ops_duplicates == OPS_DREMOVE
	    && nextsq->ops_root->pst_sym.pst_value.pst_s_root.pst_qlang == DB_QUEL)
	    continue;                   /* Avoid QUEL traps */

	/* We have a candidate, examine it for suitable data types */

	hashlen = 0;
	ok = TRUE;
	for (byexpr = sq->ops_byexpr;
	    byexpr != NULL && byexpr->pst_sym.pst_type == PST_RESDOM;
	    byexpr = byexpr->pst_left)
	{

	    varp = byexpr->pst_right;
	    if (varp == NULL)
	    {
		ok = FALSE;             /* Screwy BY-list, skip this one */
		break;
	    }
	    /* C type is excluded because multiple blanks are not
	    ** significant and it's too hard to prepare.
	    ** Unicode would have to be normalized and I don't think
	    ** opc can do that yet.
	    ** Alternate collations can map multiple characters to
	    ** compare equal, so character types have to go through
	    ** the actual comparison when alternate collation is
	    ** present.  (Perhaps an information-loss collation could
	    ** be flagged as such.)
	    */
	    vartype = abs(varp->pst_sym.pst_dataval.db_datatype);
	    if (vartype >= DB_DT_ILLEGAL
		|| vartype == DB_CHR_TYPE
		|| vartype == DB_NCHR_TYPE
		|| vartype == DB_NVCHR_TYPE
		|| (global->ops_adfcb->adf_collation != NULL
		    && (vartype == DB_CHA_TYPE || vartype == DB_VCH_TYPE
			|| vartype == DB_TXT_TYPE)))
	    {
		ok = FALSE;             /* Bad data type, skip it */
		break;
	    }

	    /* Ensure that hash key doesn't get too big */
	    hashlen += varp->pst_sym.pst_dataval.db_length;
	    if (hashlen > DB_MAXSTRING)
	    {
		ok = FALSE;
		break;
	    }
	}

	if (ok)
	{
	    /* Looks like this RFAGG can become an HFAGG */
	    sq->ops_sqtype = OPS_HFAGG;
	    global->ops_rangetab.opv_base
		->opv_grv[sq->ops_gentry]->opv_created = OPS_HFAGG;
	}
    }

} /* opa_hashagg */


/*
** Name: opa_tproc_check - look for parameter dependency cycles amongst
**	table procedures.
**
** Description:
**	Looks for table procedure range table entries and recursively
**	analyzes their parm lists, looking for dependency cycle
**	(e.g. proc A parmlist references proc B result column, proc B
**	parmlist references proc C result column, proc C parmlist
**	references proc A result column).
**
** Inputs:
**	global                  Ptr to global state control block
**	tproc1			Ptr to name of 1st bad proc (returned).
**	tproc2			Ptr to name of 2nd bad proc (returned).
**
** Outputs:
**	none (subquery types possibly changed).
**
** History:
**	18-april-2008 (dougi)
**	    Written for table procedure support.
**	20-apr-2010 (dougi) BUG 123604
**	    Fix dumb coding error.
*/

static bool
opa_tproc_check(
	OPS_STATE *global,
	OPT_NAME *tp1,
	OPT_NAME *tp2)
{
    OPV_IGVARS	gvno, cycle_gvno;
    i4		i, j;
    bool	oksofar = TRUE;


    /* First, loop over global range vars to see if there is more than
    ** one table proc. If not, the job is pretty easy. While we're there,
    ** set bits in global->ops_rangetab.opv_msubselect. */

    for (i = j = 0; i < global->ops_rangetab.opv_gv; i++)
     if (global->ops_rangetab.opv_base->opv_grv[i] &&
	global->ops_rangetab.opv_base->opv_grv[i]->opv_gmask & OPV_TPROC)
	j++;

    if (j <= 1)
	return(TRUE);			/* not much to that */

    /* Loop again - this time calling opa_tproc_parmanal() to perform 
    ** the cycle check. */
    for (i = 0; i < global->ops_rangetab.opv_gv && oksofar; i++)
     if (global->ops_rangetab.opv_base->opv_grv[i] &&
	global->ops_rangetab.opv_base->opv_grv[i]->opv_gmask & OPV_TPROC)
     {
	gvno = cycle_gvno = (OPV_IGVARS)i;
	oksofar = opa_tproc_parmanal(global, &gvno, &cycle_gvno);
     }

    if (!oksofar)
    {
	/* Found a cycle - load procedure names and stick 0x0 at end. */
	MEcopy(&global->ops_rangetab.opv_base->opv_grv[gvno]->
		opv_relation->rdr_rel->tbl_name, sizeof(DB_TAB_NAME), tp1);
	MEcopy(&global->ops_rangetab.opv_base->opv_grv[cycle_gvno]->
		opv_relation->rdr_rel->tbl_name, sizeof(DB_TAB_NAME), tp2);

	for (i = sizeof(DB_TAB_NAME); i > 1 && tp1->buf[i-1] == ' '; i--);
	tp1->buf[i] = 0x0;
	for (i = sizeof(DB_TAB_NAME); i > 1 && tp2->buf[i-1] == ' '; i--);
	tp2->buf[i] = 0x0;
    }

    return(oksofar);	/* all done */

}

/*
** Name: opa_tproc_parmanal - analyze the parmlist of a tproc
**
** Description:
**	Loops over parms of a table procedure range variable, passing
**	the parameter value expression to opa_tproc_nodeanal() to look
**	for parm cycles.
**
** Inputs:
**	global		Ptr to global state control block
**	gvno		global variable no for table proc whose parm
**			list is being checked
**	cycle_gvno	global var no that will complete cycle if 
**			found in gvno's parmlist
**
** Outputs:
**	none (subquery types possibly changed).
**
** History:
**	18-april-2008 (dougi)
**	    Written for table procedure support.
*/

static bool
opa_tproc_parmanal(
	OPS_STATE *global,
	OPV_IGVARS *gvno,
	OPV_IGVARS *cycle_gvno)

{
    OPV_GRV	*grvp = global->ops_rangetab.opv_base->opv_grv[*gvno];
    PST_QNODE	*resdomp;
    OPV_IGVARS	other_gvno = -1;

    /* Just loop over RESDOM list of parameter list. */
    /* for (resdomp = grvp->opv_gsubselect->opv_subquery->ops_root; */
    for (resdomp = global->ops_qheader->pst_rangetab[grvp->opv_qrt]->pst_rgroot;
		resdomp && resdomp->pst_sym.pst_type == PST_RESDOM;
		resdomp = resdomp->pst_left)
     if (!opa_tproc_nodeanal(global, cycle_gvno, &other_gvno, resdomp->pst_right))
     {
	if (other_gvno >= 0)
	{
	    *gvno = *cycle_gvno;
	    *cycle_gvno = other_gvno;
	    return(FALSE);
	}
     }

    /* If we finish the loop, we're gloden. */
    return(TRUE);

}
	

/*
** Name: opa_tproc_nodeanal - analyze a parameter value expression from a
**	table procedure
**
** Description:
**	Recurses/iterates over the parse tree fragment containing a 
**	parameter value specification looking for VAR nodes referencing
**	other table procedure result sets. If one is found that matches
**	"cycle_gvno", we have a dependency cycle.
**
** Inputs:
**	global                  Ptr to global state control block
**
** Outputs:
**	none (subquery types possibly changed).
**
** History:
**	18-april-2008 (dougi)
**	    Written for table procedure support.
**	25-Jun-2010 (kschendel) b123775
**	    Cycle checkers can alter the passed-in var no's, don't pass
**	    VAR node directly (paranoia), also it's an i4 not an OPV_IGVARS
**	    which is an i2, matters on some platforms.
*/

static bool
opa_tproc_nodeanal(
	OPS_STATE *global,
	OPV_IGVARS	*cycle_gvno,
	OPV_IGVARS	*other_gvno,
	PST_QNODE	*nodep)

{

    /* Switch on node type - scrutinize PST_VARs, recurse on the rest
    ** or return(TRUE). */
    switch (nodep->pst_sym.pst_type) {
      case PST_VAR:
	/* For PST_VARs, if it is a tproc result column and matches the
	** current cycle varno, it is a cycle. Otherwise, recurse on
	** tproc referenced in the varno. */
	if (global->ops_rangetab.opv_base->opv_grv[nodep->
		pst_sym.pst_value.pst_s_var.pst_vno]->opv_gmask & OPV_TPROC)
	{
	    OPV_IGVARS vno;

	    vno = nodep->pst_sym.pst_value.pst_s_var.pst_vno;
	    if (vno == *cycle_gvno
	      || ! opa_tproc_parmanal(global, &vno, cycle_gvno) )
	    {
		*other_gvno = *cycle_gvno;
		*cycle_gvno = vno;
		return(FALSE);
	    }
	}

	return(TRUE);			/* PST_VAR passed the tests */

      default:
	/* For the rest, recurse on left and/or right and/or return TRUE. */
	if (nodep->pst_left && !opa_tproc_nodeanal(global, cycle_gvno, 
						other_gvno, nodep->pst_left))
	    return(FALSE);
	if (nodep->pst_right)
	    return(opa_tproc_nodeanal(global, cycle_gvno, other_gvno,
						nodep->pst_right));
	else return(TRUE);

    }


}


/*{
** Name: opa_aggregate	- main entry point to aggregate processing phase
** 
** Description:
**      This routine will divide the query into a sequence of aggregate free
**      subqueries.  The sequence will be such that the a subquery will
**      occur in the list if all the simple aggregate result values or function 
**      aggregate temporary relations have been defined by the definition
**      of earlier an aggregate free sub-query.
**
**      The aggregate processing is divided into a number of phases.  Each
**      phase performs some transformation on the list of subqueries.  The
**      alternative would be to perform all transformations on one pass
**      of the subquery list.  However, this alternative would result in
**      difficult to understand interaction between the numerous 
**      transformations  (e.g. when analyzing a particular subquery some 
**      transformations operate on "outer" subqueries and some operate on 
**      "inner" subqueries, thus what is interaction of transformations
**      "outer" subqueries which will eventually become "inner" subqueries, 
**      on one pass?  This manifested itself with "special" varmap routines
**      which needed to know about "aghead" nodes).  The only overhead of the 
**      current approach is that an extra "for" loop is introduced for each 
**      pass of the subquery.  This loop is insignificant overhead relative 
**      to the processing done within the subquery.  In general the number 
**      of subqueries is small.
**      
**      Aggregate attempts to optimize aggregate processing
**      wherever possible. It replaces aggregates with their
**      values, and links aggregate functions which have
**      identical by-lists together.
**
**      Note that for the sake of this code, A "prime"
**      aggregate is one in which duplicates are removed.
**      These are COUNTU, SUMU, and AVGU.
**
**      Aggregate first forms a list of all aggregates in
**      the order they should be processed.  If there are no aggregates
**      then the subquery list will consist of one element i.e. the
**      entire query tree.
**
**      For each aggregate, it looks at all other aggregates
**      to see if there are two simple aggregates
**      or if there is another aggregate function with the
**      same by-list.
**
**      An attempt is made to run
**      as many aggregates as possible at once. This can be
**      done only if two or more aggregates have the same
**      qualifications and in the case of aggregate functions,
**      they must have identical by-lists.
**      Even then, certain combinations
**      of aggregates cannot occur together. The list is
**      itemized later in the code.
**
**      Note, that in 4.0 the aggregate function of the optimizer actually 
**      computed the value of the aggregate (by calling BYEVAL or AGEVAL)
**      This is not done anymore, instead only a list of aggregate free
**      subqueries is produced.
**
** BYDOM_RESTRICT -- produce a qualification on the by_list
**      (not implemented yet)
**
**	Any qualification should be applied as early as possible.
**	In aggregate processing it makes no sense to produce result rows
**	that will later be discarded. If a qualification in the parent
**	applies to the variables in the by_list, then apply the qualification
**	to the by_list while the aggregate is being processed.
**	In order to minimize complexity, we will only apply those qualifications 
**	that soley restrict the by_value variables. This means that the variables
**	in the qualification must be a subset of the variables in the bydoms
**	In addition, the attributes must be a subset of the bydom attributes.
**	The intent here is to minimize the size of the projection, not to eliminate
**	it. To restrict the size means that the qualification must refer to the
**	varnodes upon which the projection is based.
**      
**      FIXME - remember that aggregate temporaries are sorted, which may
**      save a sort node if they are linked back to the main query, really
**      DMF should support DB_SORT_STORE.
** Inputs:
**      global                    optimizer internal state control block
**
** Outputs:
**      global                    optimizer internal state control block
**          .ops_subquery               a list of aggregate free sub-queries
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	21-feb-86 (seputis)
**          initial creation
**	24-oct-88 (seputis)
**          defensive programming if pst_resvno is postive for non-updatable cursor
**	16-oct-89 (seputis)
**	    init opa_vid 
**	19-dec-89 (seputis)
**	    fix uninitialized variable problem, (for Sequent)
**	30-mar-90 (seputis)
**          remove top sort nodes from quel replaces, as in index60 tests
**	15-nov-90 (seputis)
**	    add union view optimization
**	6-apr-1992 (bryanp)
**	    Add support for PSQ_DGTT_AS_SELECT for temporary tables.
**	19-feb-97 (inkdo01)
**	    Added one more heuristic at end of everything to look for "having"s
**	    with no agg funcs which can be pushed into aggregate sq's where.
**	11-jun-98 (inkdo01)
**	    Added enhanced version of above heuristic which now also deals
**	    with unions, nested views and other complications. 
**	1-Sep-2006 (kschendel)
**	    Add pass to convert sorted function aggs to hash.
**	22-mar-10 (smeke01) b123457
**	    Moved trace point op146 call to opu_qtprint from beginning of
**	    opa_aggregate to ops_statement, just before the call to
**	    opa_aggregate. This is logically the same, but makes the display
**	    fit better with trace point op170. Added trace point op214 call
**	    to opu_qtprint.
**	14-Oct-2010 (kschendel) SIR 124544
**	    Remove bizarre code that looked at session default result_structure
**	    instead of actual result table structure!  Fortunately the test
**	    was also looking for dups set to "don't care", which the parser
**	    never allows to happen any more.
*/
VOID
opa_aggregate(
	OPS_STATE         *global)
{
    PST_QTREE	    *qheader;			/* ptr to qheader tree header */

    qheader = global->ops_qheader;

    global->ops_astate.opa_viewid = OPV_NOGVAR; /* init view ID */
    MEfill(sizeof(OPV_GBMVARS), 0, (char *)&global->ops_astate.opa_allviews);
						/* bitmap of all views
						** referenced in the query */
#if 0
    global->ops_astate.ops_uview = (OPS_SUBQUERY *)NULL;
#endif
    global->ops_astate.opa_vid = OPA_NOVID;	/* init view ID counter for
						** views to be substituted
						** by OPF */
    if ((qheader->pst_updtmode == PST_UNSPECIFIED)
	&&
        (   (qheader->pst_mode == PSQ_REPLACE)
	    ||
	    (qheader->pst_mode == PSQ_DELETE)
	))
	/* default for a set statement should be DEFERRED */
	qheader->pst_updtmode = PST_DEFER;

    global->ops_astate.opa_funcagg = FALSE;     /* this variable will be set
                                                ** TRUE if there is at least
                                                ** one function aggregate in
                                                ** the query */
    global->ops_astate.opa_subcount = 0;	/* number of subquery structures
						** generated, used for ID */
    /* PHASE 1 - create a list of subqueries
    ** This pass over the query tree will not make any modifications to
    ** the query tree itself, instead it will
    ** generate list of all aggregates with inner aggregates preceeding outer
    ** aggregates and place beginning of list in global->ops_subquery.
    ** This phase will determine if there is at least one function aggregate 
    ** in the query.
    ** Also, any range variables referenced in the subquery will have the
    ** corresponding element in the global range table initialized.  This will
    ** all temporary tables to be allocated in the global range table without
    ** conflicts.
    */ 
    opa_1pass(global);
    global->ops_gdist.opd_user_parameters = global->ops_parmtotal; /* save the number
				** of user parameters which were found in the query
				** these include all parameters except those which
				** represent simple aggregates */
    if (  !qheader->pst_agintree 
	    && 
	  !qheader->pst_subintree
	    &&
	  !global->ops_union
	    &&
	  !(global->ops_gmask & OPS_TPROCS)
	)
    {
        opa_noaggs( global );                   /* initialize subquery list
                                                ** given that no aggregates
                                                ** exists
                                                */
        return;
    };

    /* PHASE 2 - combine compatible aggregates and substitute aghead nodes
    **
    ** This pass over the subquery list will perform substitutions so that
    ** any aggregate subquery will be free of aghead nodes except for
    ** the root of the respective subquery.  Note that the aggregate is
    ** not evaluated but only analyzed and the substitution take the form
    ** of creating and replaceing the PST_AGHEAD query tree node of an
    ** inner aggregate with a "result node" of the appropriate type.
    ** This pass will determine whether two aggregates can be computed 
    ** concurrently, and make the appropriate query tree modification.
    */
    opa_compat(global);		/* combine compatible aggregates and substitute
				** result nodes for any inner aggregates
                                */

    if (global->ops_astate.opa_funcagg)
    {
	/* if there is at least one function aggregate in the query
	** then execute phases 3,4,5,6
	*/

	/* PHASE 3 - link the result relation to the main query.
	** This passes over the subquery list and add the proper 
	** qualifications to the outer aggregate, so that the 
	** results are joined with the other variables correctly.
	*/
	opa_link(global);

	/* PHASE 4 - determine if we need to project the bydoms
	*/
	opa_byproject(global);
    
    
	/* PHASE 5 - determine restrictions of bydoms for performance gains
	** This is not implemented currently. However, we can
	** apply restrictions from the parent qual to the bydoms
	** if the variables and attributes of a parent predicate 
	** are a subset of the bydom's vars and atts.
	*/
    
	opa_ptids(global);	/* process TIDs prior optimizing since
				** variable subsitution does not
				** take into account the TID requirements
				** of the parent query 
				*/
	/* PHASE 6
	** This pass over the subquery list will perform some aggregate
	** optimizations by using the "by list attributes" of any 
	** inner function aggregate to replace variables in the 
	** outer aggregates under certain conditions.  This pass 
	** must be last so that a replaced variable does not 
	** reappear due to a later query modification.
	*/
	opa_optimize(global);

	opa_varsub(global);	/* check for variable substitution which
				** can occur in an sql aggregate */
    }
        /* PHASE 7
        ** - do things done in byeval and ageval
        ** - This pass over the subquery list will create bydom nodes 
        ** for aggregate results.  It will also create new subqueries which
        ** may be needed to produce the projection of the by list, for
        ** function aggregate semantics.
        ** The ops_duplicates is set appropriately
        ** for each subquery.  (probably some of the code in the joinop
        ** phase dealing with TIDs and creation of top sortnodes should be
        ** be placed here also).
        */
    opa_final(global);

    /* PHASE 8 (?)
    ** Take a quick look at the sq structure to determine if main sq 
    ** where (i.e. having clause) can be pushed into the aggregate sq
    ** to be executed as part of its where clause. This requires NO agg
    ** funcs in having, just grouping col refs. But these do show up 
    ** in queries on aggregate views. Pushing having into where allows 
    ** greater optimization later on. opa_suckdriver implements a more 
    ** general form of the same heuristic. It attempts to also handle
    ** unions, views of unions and nested aggregate views - all cases 
    ** which were ignored by the original opa_push heuristic.
    */

    if (!(global->ops_cb->ops_smask & OPS_MDISTRIBUTED))
    {
	OPS_SUBQUERY	*mainsq = NULL;
	bool		isunion = FALSE;


# ifdef OPT_F086_DUMP_QTREE2
	if (global->ops_cb->ops_check &&
	    opt_strace(global->ops_cb, OPT_F086_DUMP_QTREE2))
	{	/* op214 - dump parse tree in op146 style */
	    qheader = global->ops_qheader;
	    opu_qtprint(global, qheader->pst_qtree, "before push/suck");
	}
# endif
	if (global->ops_cb->ops_check &&
	    opt_strace(global->ops_cb, OPT_F042_PTREEDUMP))    
	{         /* op170 - dump parse tree */
	    qheader = global->ops_qheader;
	    opt_pt_entry(global, qheader, qheader->pst_qtree, 
		"before push/suck");
	}
        opa_push(global);
 	opa_suckdriver(global, global->ops_subquery, &mainsq, TRUE, &isunion);
    }


    /* Phase 9 (!)
    ** Attempt to convert sorted function aggregates (OPS_RFAGG) into
    ** hashed function aggregates (OPS_HFAGG).  Hash aggregation is
    ** more desirable, and does not impose any sortedness expectations
    ** on the subquery.  After enumeration, if the subquery result
    ** happens to be sorted anyway, we'll change it back into
    ** a sorted function agg.
    */
    opa_hashagg(global);

    /* Phase 10 (!!)
    ** If there are table procedures in this query, check to assure that
    ** there are no parameter dependency cycles (proc A parmlist references
    ** proc B result column, proc B parmlist references proc C result column,
    ** proc C parmlist references proc A result column).
    */
    if (global->ops_gmask & OPS_TPROCS)
    {
	OPT_NAME	proc1, proc2; 

	/* Check for parameter cycles amongst table procedures. */
	if (!opa_tproc_check(global, &proc1, &proc2))
	    opx_2perror(E_OP028D_PARMCYCLE, (PTR)&proc1, (PTR)&proc2);
    }

}
