/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
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
#define             OPA_BYPROJECT       TRUE
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 

/**
**
**  Name: OPABYPRJ.C - determine if any aggregates need bydoms projected
**
**  Description:
**      Routines to determine if any bydoms need projection
**
**  History:    
**      28-jun-86 (seputis)    
**          initial creation from bydom_project in aggregate.c
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
**/

/*{
** Name: opa_bydom_project	- project the "by domains" if necessary
**
** Description:
**	set the opa_projection flag if we need to project the by domain values
** 
**	We don't need to project the by_doms if the aggregate qualification 
**      is a subset of the qualification in the parent query 
**	In the future, we can implement the code that would turn off projections
**	if the parent restriction was at least as restrictive as the BOOL_EXPR.
**	This would cover the following types of examples:
**		ret (x=sum(r.a by r.b where r.c < C)) where r.c < C - 1.
**			or
**		ret (x=sum(r.a by r.b where r.c = A or r.c = B)) where r.c = A
**
**	The idea is that if the BOOL_EXPR created by-vals with no agg result 
**	values then the more restrictive parent qual will throw at 
**	least those rows out (and possibly more). 
**	However, determining restrictiveness levels is more complicated and I am
**	not sure that it will be needed or will benefit that many people. The 
**	Intellegent user can always add a qualification to the outer 
**	that mirrors the inner to remove duplicates. Unfortunately this 
**	requires knowledge of an implementation.
**	
** Inputs:
**      subquery                        ptr to subquery being analzyed
**
** Outputs:
**	Returns:
**	    TRUE - if the "by domain values" require projection
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	13-apr-86 (seputis)
**          initial creation from bydom_project
[@history_line@]...
*/
static bool
opa_bydomproject(
    OPS_SUBQUERY       *subquery)
{
    PST_QNODE           *inner;	    /* ptr to qualification list for inner
                                    ** aggregate */
    PST_QNODE           *outer;	    /* ptr to qualification list for outer
				    ** aggregate */

    if (subquery->ops_root->pst_right 
	&&
	(subquery->ops_root->pst_right->pst_sym.pst_type == PST_QLEND)
	)
	return(FALSE);		    /* there is no qualification so do not
                                    ** project */
	
    outer = subquery->ops_agg.opa_father->ops_root->pst_right;/* this
				    ** node is the right branch of the parent
				    ** subquery root node, which should be the
				    ** qualification in both the aghead case
				    ** and root node case
				    */

    /* Since there is a list of subqueries, the multi_parent failure can
    ** be eliminated if code was written to test each of the subqueries
    ** which will be executed together (FIXME)
    */
    if ((outer && (outer->pst_sym.pst_type == PST_QLEND)) /* if no 
				    ** parent qualification */
	||
	subquery->ops_agg.opa_multi_parent
       )			    /* OR more than one parent for this 
				    ** aggregate */
	return (TRUE);		    /* project since aggregate qualification
				    ** is not a subset of the parent's
				    */

    opj_normalize(subquery, &subquery->ops_root->pst_right);
				    /* normalize inner subquery if necessary*/
    opj_normalize(subquery->ops_agg.opa_father, 
	    &subquery->ops_agg.opa_father->ops_root->pst_right);    /* normalize 
				    ** outer subquery if necessary*/

    outer = subquery->ops_agg.opa_father->ops_root->pst_right;/* this
				    ** node is the right branch of the parent
				    ** subquery root node, which should be the
				    ** qualification in both the aghead case
				    ** and root node case
				    */
    /*find a match for each qualification in the aggregate qualification list */
    for (inner = subquery->ops_root->pst_right; /* qualification is on the 
				    ** right branch of the root node */
	inner->pst_sym.pst_type != PST_QLEND;
	inner = inner->pst_right)   /*get next inner conjunct of qualification*/
	{
	    bool	    status; /* TRUE if projection is required */
	    PST_QNODE	    *tmp;   /* used to traverse parent qualification
				    ** list
				    */
	    status = TRUE;	    /* assume projection is required until
				    ** a match is found in the parent's list
				    */
	    for (tmp = outer; 
		 tmp->pst_sym.pst_type != PST_QLEND; 
		 tmp = tmp->pst_right)
	    {
		if (opv_ctrees(subquery->ops_global, inner->pst_left, 
			       tmp->pst_left)
                   )
		{
		    /* compare the inner aggregate conjunct tree with the
                    ** each conjunct of the outer aggregate qualification, 
		    ** - if a match is found then try the next inner conjunct
		    ** - if a match is not found then the qualification 
                    ** may not be a subset so a projection is required
		    */
		    status = FALSE; /* do not project */
		    break;
		}
	    }
		
	    if (status)
		return(TRUE);   /* quit if no matching qual in outer qual 
				** - project is required since qualification
				** is not a subset
				*/


    }
    return (FALSE); /* do not project since the aggregate qualification
		    ** is a subset of the parent's qualification
		    */
}


/*{
** Name: opa_byproject	- project bydoms if necessary
**
** Description:
**      Traverse the subqueries and determine if the by domains need to be 
**      projected for any function aggregates.
**
** Inputs:
**      global                          ptr to global state variable
**          .ops_subquery               beginning of a list of subqueries
**                                      to be analyzed
**
** Outputs:
**      global.ops_subquery...ops_next.ops_agg.opa_projection - this flag is
**                                      set TRUE for each subquery which will
**                                      require by domains to be projected.
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	28-jun-86 (seputis)
**          initial creation
**      25-aug-89 (seputis)
**          fix OPF access violation when parse tree gets normalized
**	31-Aug-2006 (kschendel)
**	    Watch for HFAGG as well as RFAGG, even though they shouldn't
**	    appear yet this early in opa.
*/
VOID
opa_byproject(
    OPS_STATE          *global)
{
    OPS_SUBQUERY        *subquery;	    /* used to traverse the list of
                                            ** subqueries */

    if (global->ops_cb->ops_alter.ops_noproject)
	return;				    /* no projections requested */

    for (subquery = global->ops_subquery;
	subquery->ops_sqtype != OPS_MAIN;   
        subquery = subquery->ops_next)
    {
#if 0
	if (subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_project
	    &&
	    (	subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_qlang
		== 
		DB_SQL
	    )
	    )
	{   /* this should only occur if the flattening code is enabled
	    ** so double check this */
	    if (subquery->ops_agg.opa_mask & OPA_APROJECT)
	    {
		if (!(global->ops_cb->ops_smask & OPS_MDISTRIBUTED)) /* distributed
						** should be able to deal with
						** SQL */
		    subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_qlang 
			= DB_QUEL;		/* FIXME - for now make this quel
						** since server does not support
						** projection of SQL aggregate,
						** in particular the empty set
						** returns a different value */
	    }
	    else
		opx_error(E_OP0283_PROJECT);	/* this is unexpected since
						** SQL never requires a projection */
	}
#endif

	if ((	(subquery->ops_sqtype == OPS_FAGG)
		||
		(subquery->ops_sqtype == OPS_HFAGG)
		||
		(subquery->ops_sqtype == OPS_RFAGG)
	    )
	    &&
	    (	(subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_qlang 
		== DB_QUEL)
		||
		(subquery->ops_agg.opa_mask & OPA_APROJECT)
	    )
	    &&
	    subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_project
	    )
	{
	    /* only valid for function aggregates */
	    if (subquery->ops_agg.opa_projection = opa_bydomproject(subquery))
	    {
		subquery->ops_sqtype = OPS_FAGG;    /* if projection is required
						** then a temporary is needed */
                subquery->ops_result = subquery->ops_gentry; /* assign a result
                                                ** relation */
                global->ops_rangetab.opv_base->opv_grv[subquery->ops_result]->
		    opv_gsubselect = NULL;      /* remove subselect descriptor*/
                global->ops_rangetab.opv_base->opv_grv[subquery->ops_result]->
		    opv_created = OPS_FAGG;     /* assign new creation type to
                                                ** global range variable */
	    }
	}
    }
}
