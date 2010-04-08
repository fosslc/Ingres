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
#define             OPA_OPTIMIZE        TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 
/**
**
**  Name: OPAOPTIM.C - routines to optimize variables from the query
**
**  Description:
**      This file contains the routines which will optimize variables from
**      the query by replacing them with "by list attributes" of temporary
**      function aggregate relations. 
**
**
**  History:    
**      15-april-1986 (seputis)    
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
**/

/*{
** Name: opa_checkopt	- check for possibility of optimization
**
** Description:
**      This routine will create variable map of what the outer aggregate
**      would appear like if all possible substitutions of the inner aggregate
**      bylist attributes were made.  The map of the all the inner aggregate
**      bylist elements used in the substitution is also created.  Thus, if
**      no substitutions are made then this map would be empty.
**
** Inputs:
**      global                          global state variable
**      root                            root of query tree which will
**                                      be analyzed for possible substitutions
**                                      - this root is a subtree of the outer
**                                      aggregate.
**      bylist                          base of bylist which will be
**                                      used for substitutions
**
** Outputs:
**      usedmap                         ptr to map of all variables found
**                                      in the "hit list" i.e. map of variables
**                                      which will be replaced if the
**                                      substitution were actually made
**	newmap				ptr to varmap of root, filled in if 
**					the substitutions were actually made
**	Returns:
**	    varmap of root if the substitutions were actually made
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-apr-86 (seputis)
**          initial creation
**	4-dec-02 (inkdo01)
**	    Return variable changed to a call-by-ref parm to support
**	    range table expansion.
[@history_line@]...
*/
static VOID
opa_checkopt(
	OPS_STATE          *global,
	PST_QNODE          *bylist,
	PST_QNODE          *root,
	OPV_GBMVARS	   *usedmap,
	OPV_GBMVARS	   *newmap)
{
    PST_QNODE           *node;	    /* ptr to current by list element being
                                    ** substituted
                                    */

    MEfill(sizeof(*newmap), 0, (char *)newmap); /* init the bit map */

    if ( root )
    {
	for(node = bylist;	    /* get first element of by list */
	    node && node->pst_sym.pst_type != PST_TREE; /* at the end of 
                                    ** the bylist ? */
	    node = node->pst_left)  /* get next bydom attribute */
	{
	    if ( opv_ctrees( global, root, node->pst_right ) )
	    {
		opv_mapvar( root, usedmap); /* update map of global range 
                                    ** variables used in subtree */
		return;             /* substitution is made so no vars will
                                    ** be contributed to the map of the tree
                                    ** after substitution i.e. return 0
                                    */
	    }
	}
	/* try the subtrees if none of the bylist elements matched */
	{
	    OPV_GBMVARS	    tempmap; /* used to create bit map for var node*/

	    if (root->pst_sym.pst_type == PST_VAR)
	    {
		OPV_GRV	    *gvarp;	    /* global range var associated
					    ** with this node */
		BTset( (i4)root->pst_sym.pst_value.pst_s_var.pst_vno, 
		       (char *)newmap);
		gvarp = global->ops_rangetab.opv_base->
		    opv_grv[root->pst_sym.pst_value.pst_s_var.pst_vno];
		if (gvarp->opv_gsubselect
		    &&
		    gvarp->opv_gsubselect->opv_subquery)
		{
		    MEcopy((char *)&gvarp->opv_gsubselect->opv_subquery->
			ops_correlated, sizeof(tempmap), (char *)&tempmap);
		    BTor(OPV_MAXVAR, (char *)&gvarp->opv_gsubselect->opv_subquery->
			ops_fcorelated, (char *)&tempmap);
		    if (BTcount((char *)&tempmap, OPV_MAXVAR))
		    {   /* if a correlated subquery is referenced then the
			** correlated variables cannot be substituted 
                	** - FIXME need to find all correlated vars, and see if
                	** enough attributes exist to supply correlated values
                	** this can be done by running opa_subselect in OPAFINAL.C
                	** prior to this optimization
                	** ... also need to make sure the correlated subquery
                	** is not used in another context as determined by
                	** opa_compat
                	** - for now do not substitute correlated variables
                	*/
			BTor( (i4)BITS_IN(OPV_GBMVARS), (char *)
			    &gvarp->opv_gsubselect->opv_subquery->ops_correlated,
			    (char *)newmap);
			BTor( (i4)BITS_IN(OPV_GBMVARS), (char *)
			    &gvarp->opv_gsubselect->opv_subquery->ops_fcorelated,
			    (char *)newmap);
		    }
		}
		return;		    /* return map with bit set for this var
                                    ** since a substition will not be made, so
                                    ** the var will appear in the optimized tree
                                    ** if it is created */
	    }
	    else
	    {
		opa_checkopt( global, bylist, root->pst_left, usedmap, newmap);
		MEcopy((char *)newmap, sizeof(tempmap), (char *)&tempmap);
		BTand(OPV_MAXVAR, (char *)usedmap, (char *)&tempmap);
		if (BTcount((char *)&tempmap, OPV_MAXVAR))
		    /* if the maps have an intersection then abort the search
                    ** since there will not be a commit made
                    */
		    return;

		/* traverse the right side of the tree */
		opa_checkopt( global, bylist, root->pst_right,usedmap, &tempmap);
		BTor(OPV_MAXVAR, (char *)&tempmap, (char *)newmap);
		return;
		    
	    }
	}
    }
    MEfill(sizeof(*newmap), 0, (char *)newmap); /* return empty map */
    return;
}

/*{
** Name: opa_rqual	- remove unnecessary clauses in the qualification
**
** Description:
**      This routine will traverse the qualification of the outer aggregate 
**      which has just been optimized and remove any unnecessary 
**      qualifications, which may occur due to the optimization
**
** Inputs:
**      subquery                        ptr to subquery containing qualification
**      previous			ptr to ptr to qualification list 
**                                      of outer aggregate
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
**	16-apr-86 (seputis)
**          initial creation
[@history_line@]...
*/
static VOID
opa_rqual(
	OPS_SUBQUERY	   *subquery,
	PST_QNODE          **previous)
{
    register PST_QNODE  *bop;   /* ptr to binary operator node associated
                                ** with the qualification
                                */
    register PST_QNODE	*qual;  /* ptr to qualification being analyzed */

    for (qual = *previous;	/* get initial qualification */
	qual && (qual->pst_sym.pst_type == PST_AND); /* check for end of list */
	qual = *previous)	/* get the next qualification 
				** in list */
    {
	/* if this is an EQ node then check for an unnecessary compare */
	bop = qual->pst_left;
	if (bop->pst_sym.pst_type == PST_BOP 
	    && 
	    bop->pst_sym.pst_value.pst_s_op.pst_opno 
	    == 
	    subquery->ops_global->ops_cb->ops_server->opg_eq
	    &&
	    opv_ctrees(subquery->ops_global, bop->pst_left, bop->pst_right)
	    )
	{
	    /* eliminate the equality clause if the left and right side
	    ** are equal
	    */
	    subquery->ops_vmflag = FALSE; /* map invalid since qual is removed*/
	    *previous = qual->pst_right; /* throw away link */
	    continue;   /* previous ptr is still valid so go to top of
			** loop without updating previous
			*/
	}
	previous = &(*previous)->pst_right;
    }
}

/*{
** Name: opa_commit	- commit the subsitutions found in opa_checkopt
**
** Description:
**      This routine will traverse the outer aggregate in the same way
**      as opa_checkopt but in this traversal, all the substitutions will
**      be made, since it has been determined that they were useful. 
**
** Inputs:
**      global                          global state variable
**      bylist                          bylist used for substitutions
**      varno				var number of inner aggregate
**
** Outputs:
**      root                            ptr to subtree which will have
**                                      all possible substitutions made
**	Returns:
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-apr-86 (seputis)
**          initial creation
**	5-dec-90 (seputis)
**	    fix b34217 - define eqc for update when agg substitution occurs.
**	6-may-91 (seputis)
**	    fix b37347 - do not substitute constant nodes, since simple
		aggregates have not been fully processed at this point
[@history_line@]...
*/
static VOID
opa_commit(
	OPS_STATE          *global,
	PST_QNODE          *bylist,
	PST_QNODE          **root,
	OPV_IGVARS          varno)
{
    PST_QNODE           *node;	    /* ptr to current by list element being
                                    ** substituted
                                    */

    if ( *root )
    {
	for(node = bylist;	    /* get first element of by list */
	    node && node->pst_sym.pst_type != PST_TREE; /* at the end of 
                                   ** the bylist ? */
	    node = node->pst_left) /* get next bydom attribute */
	{
	    if ( opv_ctrees( global, *root, node->pst_right ) )
	    {
		/* query tree matched so perform substitution */
		DB_ATT_ID    attribute;  /* attribute number of inner aggregate
					** bylist element
					*/
		attribute.db_att_id = node->pst_sym.pst_value.pst_s_rsdm.
		    pst_rsno; 
		/* FIXME - do we need to do a PNODERSLV here??? */
		if ((*root)->pst_sym.pst_type != PST_VAR)
		{
		    /* if node is not a var node then create a new var
                    ** node for the substitution
                    */
		    if ((*root)->pst_sym.pst_type != PST_CONST)
			(*root) = opv_varnode(global, &node->pst_sym.pst_dataval,
			    varno, &attribute); /* do not substitute constant
					** nodes since it is not required and
					** made cause problems if simple aggregates
					** are referenced - fix bug 37347 */
		}
		else
		{
		    if ((global->ops_gmask & OPS_TCHECK)
			&&
			((*root)->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id == DB_IMTID)
			&&
			(   (*root)->pst_sym.pst_value.pst_s_var.pst_vno 
			    ==
			    global->ops_qheader->pst_restab.pst_resvno
			))
		    {	/* if this is the TID eqc which will be used for the
			** update then keep track of it in
			** the global structure, since update by TID will need
			** to be via this temporary by list attribute */
			global->ops_tvar = varno;
			STRUCT_ASSIGN_MACRO( attribute,	global->ops_tattr);
		    }
		    /* if it is already a var then just change it */
		    (*root)->pst_sym.pst_value.pst_s_var.pst_vno = varno;
		    STRUCT_ASSIGN_MACRO( attribute,
			(*root)->pst_sym.pst_value.pst_s_var.pst_atno);
		}
		return;
	    }
	}
	/* try the subtrees if none of the bylist elements matched */
	{
	    if ((*root)->pst_sym.pst_type == PST_VAR)
	    {
		return;
	    }
	    else
	    {
		opa_commit(global, bylist, &((*root)->pst_left), varno);
		opa_commit(global, bylist, &((*root)->pst_right), varno);
	    }
	}
    }
    return;
}


/*{
** Name: opa_obylist	- replace variables in outer by inner
**
** Description:
**      This procedure will attempt to replace the variables in the outer 
**      aggregate by using bylist attributes of the inner aggregate. 
**
** Inputs:
**      global                          global state variable
**      inner                           inner function aggregate subquery 
**					whose bylist will be used to attempt
**                                      to replace the outer aggregate variables
**      outer                           outer aggregate subquery whose variables
**                                      will possibly be replaced by the inner
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
**	15-apr-86 (seputis)
**          initial creation
**	16-may-96 (inkdo01)
**	    Change 409554 has been backed out to fix bug 74793. It claimed to
**	    eliminate obsolete code (because of change 409457), but the code
**	    appears to have still been necessary for queries involving outer
**	    joins of aggregate views.
**	3-dec-02 (inkdo01)
**	    Changes for range table expansion.
**	23-nov-05 (inkdo01)
**	    Fix a bug in one of the more complex expressions that derived from 
**	    the range table expansion.
[@history_line@]...
*/
static VOID
opa_obylist(
	OPS_STATE          *global,
	OPS_SUBQUERY       *inner,
	OPS_SUBQUERY       *outer)
{
    OPV_GBMVARS         outermap;   /* var map of outer aggregate */

    opv_smap(outer);		    /* get variable map of outer aggregate */
    MEcopy((char *)&outer->ops_root->pst_sym.pst_value.pst_s_root.pst_lvrm,
	sizeof(outermap), (char *)&outermap);
    BTor(OPV_MAXVAR, (char *)&outer->ops_root->pst_sym.pst_value.pst_s_root.pst_rvrm,
	(char *)&outermap);
    opv_smap(inner);                /* get variable map of inner aggregate */

    BTand(OPV_MAXVAR, (char *)&inner->ops_agg.opa_blmap, (char *)&outermap);
    if (BTcount((char *)&outermap, OPV_MAXVAR) == 0)
	/* if the outer aggregate and the inner aggregate do not have any
	** variables in common then there can be no replacement so return.
	*/
	return;

    BTor(OPV_MAXVAR, (char *)&inner->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm,
	(char *)&outer->ops_aggmap);
    BTor(OPV_MAXVAR, (char *)&inner->ops_root->pst_sym.pst_value.pst_s_root.pst_lvrm,
	(char *)&outer->ops_aggmap);
    BTor(OPV_MAXVAR, (char *)&inner->ops_root->pst_sym.pst_value.pst_s_root.pst_rvrm,
	(char *)&outer->ops_aggmap);
				    /* this set of variables could be substituted
                                    ** in the outer, so they are not to be
                                    ** assumed to be in the from list for
                                    ** a cartesean product as in the query
                                    ** "select r.a from r,s" */
    {
	OPV_GBMVARS	       usedmap;	/* var map of variables which were
					** replaced by substituting the
					** bylist attributes of the inner
					*/
	OPV_GBMVARS            newmap;	/* var map of the outer aggregate
                                        ** after the inner aggregate bylist
                                        ** used to substitute expressions
                                        ** in the outer
                                        */
	OPV_GBMVARS		tempmap;
	MEfill(sizeof(usedmap), 0, (char *)&usedmap);
					/* initialize var map */
	opa_checkopt(global, inner->ops_agg.opa_byhead->pst_left, outer->ops_root,
	    &usedmap, &newmap); 	/* this routine will return information
                                        ** on what the query tree would
                                        ** look like if the inner aggregate
                                        ** was substituted (without actually
                                        ** doing the substitution)
                                        */
	MEcopy((char *)&newmap, sizeof(newmap), (char *)&tempmap);
	BTand(OPV_MAXVAR, (char *)&usedmap, (char *)&tempmap);
	/* This replaces the old (32 bit varmap) test of "usedmap &&
	** !(newmap & usedmap)". */
	if (BTcount((char *)&usedmap, OPV_MAXVAR) != 0 &&
					/* non-zero implies some optimizations
                                        ** were found */
	    BTcount((char *)&tempmap, OPV_MAXVAR) == 0)
					/* the substitution would eliminate
                                        ** those variables entirely */
	{
	    /* COMMIT THE CHANGES
	    ** the usedmap is non-zero so some variable subtitutions were
	    ** found.  Moreover, the substitutions would entirely eliminate
            ** the variables since newmap is non-zero
            **
	    ** First making a copy of the bylist for the outer aggregate 
	    ** if it exists (and if it is not the main query).  This
            ** is done to avoid the problem of optimizing away the links
            ** made by the outer aggregate.  For example, in the query
            ** RETRIEVE SUPPLIERS WHO SUPPLY ALL PARTS 
            **   ret( s.sname, s.s) where any(p.p by s.s where any(sp.tid
            **	    by p.p,s.s where p.p=sp.p and s.s=sp.s)=0)=0
            ** In this query the outer aggregate references only attributes
            ** in the BY list of the inner aggregate, and won't have the
            ** aggregate result linked to the main query ... if we did
            ** not make a copy of the bylist ... remember that the outer
            ** aggregate was linked to the main query by using the by list
            ** subtrees directly!
            ** FIXME - OPA_LINK will copy the bylists anyways so this copy
            ** is not needed
            */
	    OPV_IGVARS            innervarno; /* var number of inner
                                            ** aggregate which will be
                                            ** referenced for substitution */

	    if (outer->ops_sqtype == OPS_MAIN) 
		global->ops_gmask |= OPS_TCHECK;
	    else if (outer->ops_agg.opa_byhead)   /* outer aggregate has a by list */
	    {
		PST_QNODE             *bylist; /* used to traverse the bylist */

		/* traverse the bylist and copy the subtrees */
		for ( bylist = outer->ops_agg.opa_byhead->pst_left;
		    bylist && bylist->pst_sym.pst_type != PST_TREE;
		    bylist = bylist->pst_left)

		    opv_copytree( global, &bylist->pst_right );
	    }

	    /* Traverse the tree and actually perform the substitutions instead
            ** of only checking for them
            */
	    innervarno = (*inner->ops_agg.opa_graft)->pst_sym.pst_value.
		    pst_s_var.pst_vno;
	    if (outer->ops_global->ops_qheader->pst_numjoins > 0)
	    {	/* make sure that all the outer joins semantics are
		** the same for all the relations referenced, or else
		** semantics are lost, i.e. cannot substitute if variables
		** have different maps */
		PST_J_MASK	pinner;
		PST_J_MASK	pouter;
		bool		first_time;
		OPV_IGVARS	gvar;
		PST_J_MASK	*ijmaskp;
		PST_J_MASK	*ojmaskp;
		OPL_PARSER	*pinnerp;
		OPL_PARSER	*pouterp;

		first_time = TRUE;
		pinnerp = outer->ops_oj.opl_pinner;
		pouterp = outer->ops_oj.opl_pouter;
		for (gvar = -1; (gvar = BTnext((i4)gvar, (char *)&usedmap, 
			(i4)BITS_IN(usedmap)))>=0;)
		{
		    if (first_time)
		    {
			MEcopy((PTR)&pinnerp->opl_parser[gvar],
			    sizeof(pinner), (PTR)&pinner);
			MEcopy((PTR)&pouterp->opl_parser[gvar],
			    sizeof(pouter), (PTR)&pouter);
		    }
		    else
		    {
			if (MEcmp((PTR)&pinnerp->opl_parser[gvar],
			    (PTR)&pinner, sizeof(pinner))
			    ||
			    MEcmp((PTR)&pouterp->opl_parser[gvar],
			    (PTR)&pouter, sizeof(pouter))
			    )
			    return;	    /* outer joins semantics of
					    ** variables to be substituted are
					    ** different, FIXME, try to substitute
					    ** one variable instead of 2 */
		    }
		}
		/* copy the outer join semantics to the substituted variable */
		ijmaskp = &pinnerp->opl_parser[innervarno]; 
		ojmaskp = &pouterp->opl_parser[innervarno];
		if ((BTnext((i4)-1, (char *)ijmaskp, (i4)BITS_IN(*ijmaskp)) >= 0)
		    ||
		    (BTnext((i4)-1, (char *)ojmaskp, (i4)BITS_IN(*ojmaskp)) >= 0)
		    )
		    opx_error(E_OP0288_OJAGG);	/* should not already have an
					    ** outer join defined on this aggregate
					    ** in this query */
		MEcopy((PTR)&pinner, sizeof(*ijmaskp), (PTR)ijmaskp);
		MEcopy((PTR)&pouter, sizeof(*ojmaskp), (PTR)ojmaskp);
	    }
	    outer->ops_vmflag = FALSE;  /* bitmaps need to be updated if a
                                        ** substitution on the outer is made */
	    opa_commit(global, inner->ops_agg.opa_byhead->pst_left, 
		&outer->ops_root, innervarno); /* this routine will traverse 
                                        ** the tree in the same way as 
                                        ** opa_checkopt except that 
                                        ** substitutions will actually be made
                                        */
	    global->ops_gmask &= (~OPS_TCHECK);
	}
    }
}

/*{
** Name: opa_optimize	- remove variables from query if possible
**
** Description:
**      This routine will traverse the list of subqueries.  Whenever 
**      an aggregate subquery is found, the outer aggregate subqueries of the
**      inner are analyzed to determine whether a variable of the outer 
**      aggregate can be removed by using some combination of bylist attributes 
**      of the inner function aggregate subquery.
**
**      In 4.0 there was semantics to avoid optimization if the outer function
**      aggregate was not prime.  This was done to preserve duplicates
**      since the inner aggregate may remove the duplicates.  The semantics
**      in 4.0 have changed (from 3.0) so that unless explicit TIDs are
**      referenced, the outer aggregate will use the duplicate free projection 
**      of the inner temporary relation.  Thus, this check was not necessary
**      even in 4.0.  This comment was placed here since optimizations
**      will be done in this case (where it was explicitly avoided in 4.0)
**      and hopefully no side effects will occur.
**             e.g. ret(x = sum(r.a by r.b where sum( r.c by r.a, r.b) >= 0 ))
**      The story of duplicates continues, in V5.0/4 in which duplicates
**      were kept if the outer aggregrate i.e. like 3.0.  However, this
**      will require optimizations to be avoided here as in 3.0.  FIXME, the
**      3.0 duplicate semantics were not implemented in 6.0 yet.  The rational
**      behind this is that it is much more obvious to the user that the outer
**      sum aggregate preserves duplicates, rather than an obscure link-back
**      rule which removes duplictes.  However, one side effect may be to
**      affect the duplicate handling of SQL, which should not be changed.
**      See the description below, i.e. for SQL do NOT avoid the optimization
**      for aggregates, but do avoid it for QUEL.
**
**      - bug fix, the following script would produce duplicates for the final
**        result, where there should be none, due to the link back of the
**        aggregate temporaries, this problem can be avoided by causing
**        complete substitution of the SQL aggregate temporaries which can be
**        guaranteed due to the nature of the SQL GROUP BY i.e. only variables
**        in the group by can be visible in the target list of the view
**	drop tmp;\p\g
**	drop v1;\p\g
**	create table tmp(k1 integer2, k2 integer2, k3 integer2, a1 integer2);\p\g
**	insert into tmp(k1,k2,k3,a1) values(1,1,1,15);\p\g
**	insert into tmp(k1,k2,k3,a1) values(1,1,2,16);\p\g
**	insert into tmp(k1,k2,k3,a1) values(1,2,1,17);\p\g
**	insert into tmp(k1,k2,k3,a1) values(1,2,2,16);\p\g
**	insert into tmp(k1,k2,k3,a1) values(2,1,1,16);\p\g
**	insert into tmp(k1,k2,k3,a1) values(2,1,2,16);\p\g
**	insert into tmp(k1,k2,k3,a1) values(2,2,1,16);\p\g
**	insert into tmp(k1,k2,k3,a1) values(2,2,2,16);\p\g
**	create view v1 as select k1,k2,agg1=sum(a1) from tmp group by k1,k2\p\g
**	select k1, agg2=sum(agg1) from v1 group by k1;\p\g
**
**    - as the note above mentioned, if we go back to the 3.0 semantics
**	optimization will need to be avoided for the case of a duplicate
**      retaining aggregate (sum,count,avg) containing another type of aggregate
**      with a by list.  This is because the optimization will remove
**      duplicates.  Another case to consider is that the qrymod flattening
**      of views have different handling of duplicates on the link back than
**      the normal explicit link back.  For example in the query above, the
**      link back of the view is a duplicate free projection, this will need
**      to be a special case when 3.0 semantics are reimplemented.
**
** Inputs:
**      global                          global state variable
**          .ops_subquery               beginning of subquery list to be
**                                      analyzed
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-apr-86 (seputis)
**          initial creation
**	31-Aug-2006 (kschendel)
**	    Watch for HFAGG as well as RFAGG, even though they shouldn't
**	    appear yet this early in opa.
*/
VOID
opa_optimize(
	OPS_STATE          *global)
{
    OPS_SUBQUERY        *subquery;  /*ptr to current subquery being analyzed */

    for( subquery = global->ops_subquery;
	subquery;		    /* look at main query to remove unnecessary
                                    ** qualifications */
	subquery = subquery->ops_next)	
    {
	/* check if this is a function aggregate i.e. does it have a by list
        ** which can be used to replace outer aggregate variables?
        */
	if ((subquery->ops_sqtype == OPS_FAGG)
	    ||
	    (subquery->ops_sqtype == OPS_HFAGG)
	    ||
	    (subquery->ops_sqtype == OPS_RFAGG)
	   )
	{
	    OPS_SUBQUERY       *concurrent; /* used to traverse all function
					** aggregate subqueries
					** which will be executed
					** concurrently
					*/
	    for( concurrent = subquery;	/* first in list of subqueries to be
					** executed concurrently. 
					*/
		concurrent;             /* is this the last concurrent
                                        ** subquery in the list? 
					*/
		concurrent = concurrent->ops_agg.opa_concurrent) 
	    {
		OPS_SUBQUERY	    *father; /* used to look optimize outer
					** aggregates with this particular
					** subquery
					*/
		for ( father = concurrent->ops_agg.opa_father; /* get first
					** ancestor for optimization
                                        ** - try to optimize
                                        ** any visible grandfather etc.
					*/
		      father;
		      father = father->ops_agg.opa_father)
		{
		    if ((father->ops_duplicates != OPS_DKEEP)
			||
			(!(subquery->ops_mask & OPS_COAGG))
			)
			opa_obylist(global, concurrent, father); /* substitute
					** only if duplicates do not matter, which
					** is the case for most aggregate linkbacks
					** except for some SQL flattened ones */
		    if ((father->ops_sqtype != OPS_FAGG)
			&&
			(father->ops_sqtype != OPS_HFAGG)
			&&
			(father->ops_sqtype != OPS_RFAGG)
			)
			break;		/* break if the current father is not
					** a function aggregate, thus the
					** variable is not visible past this point
                                        */
		}
	    }
	}
	opa_rqual( subquery, &subquery->ops_root->pst_right ); /* remove 
                                        ** unnecessary qualifications */
    }
}
