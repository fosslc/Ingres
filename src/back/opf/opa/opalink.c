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
#define             OPA_LINK            TRUE
#include    <bt.h>
#include    <me.h>
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 
/**
**
**  Name: OPALINK.C - routines which link function aggregates to query
**
**  Description:
**      This file contains routines which link inner function aggregates 
**      to the outer aggregate subquery (or main query).
**
**  History:    
**      19-apr-86 (seputis)    
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**	31-Aug-2006 (kschendel)
**	    Watch for HFAGG as well as RFAGG, even though they shouldn't
**	    appear yet this early in opa.
**/

/*{
** Name: opa_modqual	- link the inner to the outer aggregate
**
** Description:
**      This routine will link the inner to the outer aggregate, by adding
**      a qualification to the outer aggregate using the bylist of the 
**      inner aggregate.
**
** Inputs:
**      global                          ptr to global state variable
**      inner                           ptr to inner function aggregate subquery
**      outer                           ptr to outer aggregate subquery
**
** Outputs:
**      outer                           outer aggregate subquery will have
**					additional qualifications on the
**                                      right branch of the root
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	19-apr-86 (seputis)
**          initial creation
**	10-sep-89 (seputis)
**          do not include non-printing resdoms in link back operation
**      15-dec-91 (seputis)
**          fix b40402
**	09-jul-93 (andre)
**	    recast first arg to pst_resolve() to (PSF_SESSCB_PTR) to agree
**	    with the prototype declaration
[@history_line@]...
*/
static VOID
opa_modqual(
	OPS_STATE          *global,
	OPS_SUBQUERY       *inner,
	OPS_SUBQUERY       *outer)
{
    PST_QNODE           *bylist;    /* used to traverse the bylist of the
				    ** inner aggregate
				    */
    OPV_IGVARS		innervarno; /* variable number of temporary result
				    ** of inner aggregate
				    */

    outer->ops_vmflag = FALSE;      /* since links are added to this subquery
                                    ** the var bit map is now invalid */
    innervarno = (*inner->ops_agg.opa_graft)->pst_sym.pst_value.pst_s_var.
        pst_vno;		    /* get the variable number of the inner
				    ** aggregate */

    if (inner->ops_agg.opa_fbylist)
        bylist = inner->ops_agg.opa_fbylist;
                                    /* if the original subselect has a "group by" then
                                    ** only link back the corelated variables */
    else
        bylist = inner->ops_agg.opa_byhead->pst_left;   /* get first bylist
                                    ** element */
    for (;
        bylist && bylist->pst_sym.pst_type == PST_RESDOM; /* has the end of
                                    ** the bylist been reached? */
        bylist = bylist->pst_left)  /* get the next bylist element */
    {
	PST_QNODE              *and_node;   /* used to create a node which
				    ** will be added to the qualification
				    */
	if (!(bylist->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT))
	    continue;		    /* skip over the non-printing resdoms since
				    ** they only participate in the duplicate semantics
				    ** and should not participate in the by list link back */
	and_node = opv_opnode(global, PST_AND, (ADI_OP_ID) 0,
	    (OPL_IOUTER)inner->ops_oj.opl_aggid);  /* allocate memory for 
                                    ** "and" node */
        if (inner->ops_oj.opl_aggid != PST_NOJOIN)
	    BTset ((i4)inner->ops_oj.opl_aggid, (char *)&outer->ops_oj.
		opl_pinner->opl_parser[innervarno]);
	/* the PST_AND node does not have pst_dataval defined, and the
        ** components of PST_OP_NODE are also undefined 
	*/

	and_node->pst_right = outer->ops_root->pst_right; /* link into the 
				    ** qualification list of the outer subquery
				    */
	outer->ops_root->pst_right = and_node;

	{
	    /* create the "equals" node for the link */
	    PST_QNODE          *equals_node;
	    DB_ATT_ID          dmfattr;	    /* attribute number which will be
                                            ** associated with the temp
                                            ** relation */
	    /* there is a subtle optimization which depends on an order
	    ** dependency here, i.e. the implicit link back needs to be
	    ** defined before any uses of the aggregate result in order
	    ** to obtain full benefits of checking for degenerate outer joins 
	    ** by setting maps in opz_ctree correctly */
 	    and_node->pst_left = 
            equals_node = opv_opnode(global, PST_BOP, 
		global->ops_cb->ops_server->opg_eq,
		(OPL_IOUTER)inner->ops_oj.opl_aggid); /* get 
					** memory for "=" node */
	    equals_node->pst_sym.pst_value.pst_s_op.pst_flags |= PST_IMPLICIT;
					/* save indicator that this operator
					** was created due to an implicit
					** link back operation */
	    dmfattr.db_att_id =	bylist->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
	    equals_node->pst_left = opv_varnode (global, 
		&bylist->pst_sym.pst_dataval, innervarno, &dmfattr); /* create
				        ** a var node
					** with the var number of the inner
					** aggregate temporary relation, and
					** the attribute number of the
					** respective bylist domain
					*/

	    equals_node->pst_right = bylist->pst_right; /* use the same
					** expression as the bylist attribute
					*/
            opv_copytree(global, &equals_node->pst_right); /* copy tree since
                                        ** both inner aggregate and outer
                                        ** aggregate will modify this expression
                                        */
	    {
                /* need to resolve the types for the new BOP equals node */

		DB_STATUS	    res_status;	/* return status from resolution
                                        ** routine */
		DB_ERROR	    error; /* error code from resolution routine
                                        */
		res_status = pst_resolve(
		    (PSF_SESSCB_PTR) NULL,	 /* ptr to "parser session
						 ** control block, which is
						 ** needed for tracing
						 ** information */
		    global->ops_adfcb, 
		    equals_node, 
		    outer->ops_root->pst_sym.pst_value.pst_s_root.pst_qlang,
		    &error);
		if (DB_FAILURE_MACRO(res_status))
		    opx_verror( res_status, E_OP0685_RESOLVE, error.err_code);
	    }
	}
    }
}

/*{
** Name: opa_link	- links function aggregates to outer subqueries
**
** Description:
**	Add the proper qualification to the query tree so that
**	the results from the aggregate result relation are
**	retrieved (joined) with the other variables correctly.
**
**	This link insures that the variables used in the by_list
**	are visible to the next outer query. The link is only made
**	if the by_list variables are then used outside the aggregate.
**	opa_optimize might decide to discard the link if it proves to be
**	unneeded.
**
**	The qualification is always of the type:
**
**		T.by_1 = by_value_1 expression
**		T.by_2 = by_value_2 expression	
**
**	Where T is the aggregate result relation, by_1 and by_2 are
**	the computed by_value expressions, and by_value_1 and
**	by_value_2 are the expressions as typed by the user.
**
**	The link is made between in the qualification of the aggregate
**	expressions' parent. If the current aggregate is in the by_list
**	of another aggregate, then the link is also moved to the next outer
**	parent's qualification.
**
**      The procedure will traverse the subqueries and generate a bylist
**      var map for each of the function aggregate subqueries.
**      At this point in aggregate processing each function aggregate can be
**      represented by the var number of the temporary aggregate result
**      relation.  A pass is made using the bylist map of the aggregates
**      to generate a map of inner aggregate variables visible at each outer
**      aggregate node.
**      A final pass is made in which the varmaps will be used to determine
**      whether it is necessary to link a function aggregate to the outer
**      subquery.
** Inputs:
**      global                          global state variable ptr
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
**	20-apr-86 (seputis)
**          initial creation
**	16-oct-89 (seputis)
**          add test for aggregrate linkback of views (b8160)
**	1-apr-93 (ed)
**	    fix b43974, b45822, b50747, use proper maps when linking
**	    inner to outer relations.
**	17-may-93 (ed)
**	    - fix b51879 - concurrent queries do not necessarily have
**	    same global var number since self join optimizations are
**	    now being done.
**	3-dec-02 (inkdo01)
**	    Changes for range table expansion.
**	24-aug-05 (inkdo01)
**	    Prevent link of non-correlated, single valued SQs (SAGGs)
**	    (but this is a good candidate for a fix to back out if subselects
**	    start going wrong).
**      04-aug-2010 (huazh01)
**          Modify the above fix. do not skip SAGG if a PST_VAR in child agg's
**          'bylist' has been added into main subquery as a non-printing resdom.
**          (b124187)
[@history_line@]...
*/
VOID
opa_link(
	OPS_STATE          *global)
{
    OPS_SUBQUERY           *subquery;   /* used to traverse subquery list */
    OPS_SUBQUERY           *mainSqry; 
    PST_QNODE              *p1, *p2, *bylist; 
    bool                   found = FALSE; 
    
    {
        mainSqry = global->ops_subquery;

        while (mainSqry->ops_sqtype != OPS_MAIN)
              mainSqry = mainSqry->ops_next;

	/* traverse the list of subqueries and update the visibility maps
        ** - a visibility map for an aggregate is a bitmap of the temporary
        ** range variables which are visible in the bylist of the aggregrate
        ** - this loop starts at a child and propagates the visibility
        ** to the parents */

	for ( subquery = global->ops_subquery;
	    subquery->ops_sqtype != OPS_MAIN; /* exit if the main query */
	    subquery = subquery->ops_next)
	{
	    if ((subquery->ops_sqtype == OPS_FAGG)
		||
		(subquery->ops_sqtype == OPS_HFAGG)
		||
		(subquery->ops_sqtype == OPS_RFAGG)
	       )
	    {
		/* if this is a function aggregate subquery then traverse
                ** the list of aggregate subqueries which will be executed
                ** concurrently and mark the aggregate visible to the
                ** outer aggregates
		*/
		OPS_SUBQUERY          *concurrent;  /* used to traverse 
					    ** concurrent subqueries
					    */

		for ( concurrent = subquery;
		    concurrent;
		    concurrent = concurrent->ops_agg.opa_concurrent)
		{
		    /* traverse the list of fathers if the inner function
                    ** aggregate is visible and update the visibility map
                    */
		    OPS_SUBQUERY      *father;	/* traverse outer aggregates */
		    OPV_IGVARS	      innervarno; /* variable number of most
                                                ** recent inner aggregate with
                                                ** a bylist
                                                */
		    OPV_IGVARS	      ivarno;	/* var no of temporary inner
						** aggregate result relation
						*/
		    ivarno = (*concurrent->ops_agg.opa_graft)->pst_sym.pst_value.
			    pst_s_var.pst_vno;
		    if (!(concurrent->ops_mask & OPS_AGGNEWVAR)
			&&
			(ivarno != (*subquery->ops_agg.opa_graft)->pst_sym.pst_value.
                            pst_s_var.pst_vno))
			opx_error(E_OP028C_SELFJOIN);	
			/* get variable number of temporary aggregate result relation 
			** - this should be the same for all subqueries to be executed
			** concurrently
			*/

                    innervarno = ivarno;	/* initialize variable number
                                                ** visible to that of inner
                                                ** aggregate
                                                */
		    for ( father = concurrent->ops_agg.opa_father;
			father;
			father = father->ops_agg.opa_father)
		    {	/* look at all parents and update visibility maps */
			/* Skip SQL SAGGs with no predicates. They must be 
			** non-correlated single value SQs and we don't want
			** to cause them to link back. 
                        **
                        ** b124187: Child agg must be visible (linked to) SAGG if child's
                        ** bylist appears in the main subquery as non-printing resdoms.
                        ** Otherwise, the non-printing resdom in the main query will appear
                        ** to be in an eqcls by itself, causing a cart-prod and duplicated 
                        ** rows. 
                        */
			if (father->ops_sqtype == OPS_SAGG &&
			    father->ops_root && 
			    father->ops_root->pst_sym.pst_value.pst_s_root.
				pst_qlang == DB_SQL &&
			    (father->ops_root->pst_right == NULL ||
			    father->ops_root->pst_right->pst_sym.pst_type == PST_QLEND))
                        {
                            p1 = mainSqry->ops_root;
                            found = FALSE;

                            while (p1)
                            {
                                 if (p1->pst_sym.pst_type == PST_RESDOM &&
                                     p1->pst_right &&
                                     p1->pst_right->pst_sym.pst_type == PST_VAR &&
                                     !(p1->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & 
                                         PST_RS_PRINT))
                                 {
                                    if (concurrent->ops_agg.opa_fbylist)
                                       p2 = concurrent->ops_agg.opa_fbylist;
                                    else
                                       p2 = concurrent->ops_agg.opa_byhead->pst_left;

                                    while (p2)
                                    {
                                       if (p2->pst_sym.pst_type == PST_RESDOM  &&
                                           p2->pst_right &&
                                           p2->pst_right->pst_sym.pst_type == PST_VAR &&
                                           p2->pst_right->pst_sym.pst_value.pst_s_var.pst_vno
                                           ==
                                           p1->pst_right->pst_sym.pst_value.pst_s_var.pst_vno
                                           &&
                                           p2->pst_right->pst_sym.pst_value.pst_s_var.\
		   				pst_atno.db_att_id ==
                                           p1->pst_right->pst_sym.pst_value.pst_s_var.\
						pst_atno.db_att_id)
                                       {
                                           found = TRUE;
                                           break;
                                       }
                                       p2 = p2->pst_left;
                                    }
                                 }

                                 if (found)
                                    break;

                                 p1 = p1->pst_left;
                            }

                            if (!found)
                               continue;
                        }
			BTset((i4)ivarno, (char *)&father->ops_agg.opa_visible);
                                                /* function aggregate is visible
                                                ** its immediate parent always
                                                */
			opv_smap( father );	/* update the var maps of the
                                                ** outer aggregate subquery
						*/
			if (!BTtest((i4)innervarno, 
			            (char *)&father->ops_agg.opa_blmap)
			    ||
			    (father->ops_root->pst_sym.pst_value.pst_s_root.
				pst_qlang == DB_SQL) /* SQL aggregates are only
						** visible in immediate parent*/
                           )
			    break;		/* exit loop if the aggregate
                                                ** is not visible in the
                                                ** father's bylist
                                                */

			innervarno = (*father->ops_agg.opa_graft)->pst_sym.
			    pst_value.pst_s_var.pst_vno; /* update the varno
						** to see if it is visible
						** in the next parent's bylist
						*/

		    }
		}
	    }
	}
    }

    /* Each aggregate has a map of all inner aggregates which are visible.
    ** This map is used to generate a map of all variables which are
    ** visible at the outer aggregate.
    */
    {
	/* traverse the list of subqueries looking for aggregate functions */

	for ( subquery = global->ops_subquery;
	    subquery->ops_sqtype != OPS_MAIN;
	    subquery = subquery->ops_next)
	{
	    if ((subquery->ops_sqtype == OPS_FAGG)
		||
		(subquery->ops_sqtype == OPS_HFAGG)
		||
		(subquery->ops_sqtype == OPS_RFAGG)
	       )
	    {
		/* if this is a function aggregate subquery then traverse
                ** the list of aggregate subqueries which will be executed
                ** concurrently and determine if it is necessary to link
                ** the aggregate to the outer aggregates
		*/
		OPS_SUBQUERY          *inner;  /* used to traverse 
					    ** concurrent subqueries
					    */

		for ( inner = subquery;
		    inner;
		    inner = inner->ops_agg.opa_concurrent)
                /* traverse the list of concurrent subqueries and link if
                ** necessary */
		{
		    OPS_SUBQUERY      *outer;	/* traverse outer aggregates */
		    OPV_GBMVARS	      mapbylist;/* varmap of bylist for
						** inner aggregate
						*/
		    OPV_GBMVARS	      tempmap;
		    OPV_IGVARS	      varno;	/* var no of temporary inner
						** aggregate result relation
						*/

		    /* get variable number of temporary aggregate result relation */
		    varno = (*inner->ops_agg.opa_graft)->pst_sym.pst_value.
			pst_s_var.pst_vno;
		    {
			/* move this calculation inside inner loop to fix bugs
			** 45822, 50747, since variables may have been mapped by
			** opacompat.c */
			/* update the bylist map of the inner function aggregate */
			opv_smap( inner );
			if (inner->ops_root->pst_sym.pst_value.pst_s_root.
			    pst_qlang == DB_SQL)
			    MEcopy((char *)&inner->ops_root->pst_sym.pst_value.
				pst_s_root.pst_tvrm, sizeof(mapbylist),
				(char *)&mapbylist); /* in SQL all variables in the 
						    ** function aggregate are visible 
						    ** to the immediate parent */
			else
			    MEcopy((char *)&inner->ops_agg.opa_blmap,
				sizeof(mapbylist), (char *)&mapbylist); /* in QUEL
						    ** only bylist variables are
						    ** visible */
						    /* all the subqueries
						    ** will have the same bylist map
						    ** so save this value
						    */
		    }
		    for ( outer = inner->ops_agg.opa_father;
			outer;
			outer = outer->ops_agg.opa_father)
		    /* traverse the list of fathers check if the inner function
                    ** aggregate is visible and link if necessary
                    */
		    {
			OPV_GBMVARS	     map;   /* map of aggregate
                                                    ** function variables
                                                    ** visible in father
                                                    ** subquery, not including
                                                    ** the current subquery
                                                    */
			OPV_GBMVARS	     srcmap;/* map of variables visible
                                                    ** in outer aggregate
                                                    */

                        if (BTtest((i4)varno, (char *)&outer->ops_agg.opa_linked))
			    break;                  /* break if this inner
                                                    ** has already been linked*/
			MEcopy((char *)&outer->ops_agg.opa_visible, 
				sizeof(map), (char *)&map);
						    /* map of aggregates
						    ** which are visible at
                                                    ** this parent
                                                    */
			if (!BTtest((i4)varno, (char *)&map))
			    break;		    /* do not link if the
                                                    ** inner aggregate is not
                                                    ** visible at this parent
                                                    */
                        BTclear((i4)varno, (char *)&map);/* do not look at 
                                                    ** yourself when computing
						    ** the bylist maps
                                                    */
			MEfill(sizeof(srcmap), 0, (char *)&srcmap);
						    /* initialize bylist map*/

                        if (BTcount((char *)&map, OPV_MAXVAR) != 0)
			{   /* if the map indicates other aggregates are visible
                            ** at this node then generate a map of the bylists
                            ** - this will be used to link "brothers" to one
                            ** another
                            */
			    OPS_SUBQUERY     *bylistpass;

			    for( bylistpass = global->ops_subquery;
				bylistpass->ops_sqtype != OPS_MAIN;
				bylistpass = bylistpass->ops_next)
			    {
				if ((bylistpass->ops_sqtype != OPS_FAGG)
				    &&
				    (bylistpass->ops_sqtype != OPS_HFAGG)
				    &&
				    (bylistpass->ops_sqtype != OPS_RFAGG)
				   )
				    continue;	/* look for function 
						** aggregates only
						*/
				/* include the bylist map if the aggregrate
                                ** is visible to the father aggregate
                                */
				if (BTtest((i4)(*bylistpass->ops_agg.opa_graft)
					->pst_sym.pst_value.pst_s_var.pst_vno, 
					(char *)&map)
				   )
				{
				    if (bylistpass->ops_root->pst_sym.pst_value.
					pst_s_root.pst_qlang == DB_SQL)
					BTor(OPV_MAXVAR, (char *)&bylistpass->
					    ops_root->pst_sym.pst_value.
					    pst_s_root.pst_tvrm,
					    (char *)&srcmap);
				    else
					BTor(OPV_MAXVAR, (char *)&bylistpass->
					    ops_agg.opa_blmap, 
					    (char *)&srcmap);
				}
			    }
			}
			if ((outer->ops_root->pst_sym.pst_value.pst_s_root
				.pst_qlang == DB_QUEL)
			    &&
			    (inner->ops_root->pst_sym.pst_value.pst_s_root
				.pst_qlang == DB_QUEL)
			    &&
			    (outer->ops_root->pst_sym.pst_value.pst_s_root
                                .pst_dups == PST_ALLDUPS
			    )
			   )
			{   /* set keeptid map to implement the rule for
                            ** linking quel aggregates i.e. link the
                            ** function aggregate to the duplicate free
                            ** projection of the variables referenced in
                            ** the bylist, only set flag if it duplicates
                            ** are to be kept in the parent, otherwise it
                            ** is not correct to keep tids */
			    OPV_GBMVARS		notidmap;
			    MEcopy((char *)&inner->ops_agg.opa_blmap,
				sizeof(notidmap), (char *)&notidmap);
			    BTor(OPV_MAXVAR, (char *)&srcmap, (char *)&notidmap);
						/* current bitmap of variables
						** visible in the by list which
						** should have duplicates removed */
			    if (global->ops_astate.opa_vid != OPA_NOVID)
			    {	/* at least one view was querymoded, so that
				** duplicates must be removed for all tables
				** with the same view ID, so that the view
				** behaves like a base table */
				OPV_GBMVARS	*fromp; /* ptr to from list
						** of the query */
				OPV_GBMVARS	viewmap;    /* map of view IDs 
						** which are found in the by list */
				OPV_IGVARS	gvar; /* current global range table
						** index being analyzed */
				OPV_GRT		*gbase;
				bool		view_found;
				MEfill(sizeof(viewmap), 0, (char *)&viewmap);
				view_found = FALSE;
				gbase = global->ops_rangetab.opv_base;
				for (gvar = -1;
				  (gvar = BTnext((i4)gvar, (char *)&notidmap, (i4)sizeof(notidmap)))>=0;
				  )
				{
				    OPA_VID	    viewid1;
				    viewid1 = gbase->opv_grv[gvar]->opv_gvid;
				    if (viewid1 != OPA_NOVID)
				    {
					BTset((i4)viewid1, (char *)&viewmap);
					view_found = TRUE;
				    }
				}
				if (view_found)
				{
				    fromp = (OPV_GBMVARS *)&outer->ops_root->pst_sym.
					pst_value.pst_s_root.pst_tvrm;
				    for (gvar = -1;
				      (gvar = BTnext((i4)gvar, (char *)fromp, (i4)sizeof(*fromp)))>=0;
				      )
				    {
					OPA_VID	    viewid2;
					viewid2 = gbase->opv_grv[gvar]->opv_gvid;
					if ((viewid2 != OPA_NOVID)
					    &&
					    (BTtest((i4)viewid2, (char *)&viewmap))
					    )
					{
					    /* found a table with the same viewid so
					    ** duplicates need to be removed */
					    BTset((i4)gvar, (char *)&notidmap);					
					}
				    }
				}
			    }
			    BTnot(OPV_MAXVAR, (char *)&notidmap);
			    BTand(OPV_MAXVAR, (char *)&notidmap, 
				(char *)&outer->ops_agg.opa_keeptids);
						/* reset all variables
						** reference in the bylist of
                                                ** a visible inner */
			    outer->ops_agg.opa_tidactive = TRUE;
			}
			BTor(OPV_MAXVAR, (char *)&outer->ops_root->pst_sym.
			    pst_value.pst_s_root.pst_tvrm, (char *)&srcmap);
			BTor(OPV_MAXVAR, (char *)&outer->ops_root->pst_sym.
			    pst_value.pst_s_root.pst_lvrm, (char *)&srcmap);
			BTor(OPV_MAXVAR, (char *)&outer->ops_root->pst_sym.
			    pst_value.pst_s_root.pst_rvrm, (char *)&srcmap);
			/* srcmap now contains a map of all the visible bylists
                        ** as well as variables in the outer aggregate itself
                        */

			MEcopy((char *)&mapbylist, sizeof(tempmap), 
						(char *)&tempmap);
			BTand(OPV_MAXVAR, (char *)&srcmap, (char *)&tempmap);
			if (BTcount((char *)&tempmap, OPV_MAXVAR) != 0)
			{   /* if some variables are referenced both in the 
                            ** outer aggregate and the inner aggregate, then
                            ** the inner aggregate needs to be linked
			    */
			    opa_modqual( global, inner, outer);
                            BTset((i4)varno, (char *)&outer->ops_agg.opa_linked);
							/* indicate that inner
                                                        ** agg was linked to
                                                        ** the outer agg */
			}
		    }
		}
	    }
	}
    }
}
