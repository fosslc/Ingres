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
#define        OPN_SM2      TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>

/**
**
**  Name: OPNSM2.C - set up bit maps required by opn_nodecost
**
**  Description:
**      routines to set up bit maps required by opn_nodecost
**
**  History:    
**      30-may-86 (seputis)    
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
**/

/*
**  Forward and/or External function references.
*/

static bool
opb_spattr_check(
	OPS_SUBQUERY	*subquery,
	OPV_VARS	*varp,
	OPE_IEQCLS	speqc
);

/*{
** Name: opb_vars_in_bm	- chk qtree vars in boolean factor bitmap
**
** Description:
**      This routine will determine whether the bmvars passed is consistent
**	with the parse tree fragment.
**
** Inputs:
**	node			ptr to parse tree fragment
**	bmvars			bitmap that 'should' contain all the vno
**				bits set
**
** Outputs:
**	Returns:
**	    bool		FALSE if any VAR node in tree not included in bm and
**				TRUE otherwise.
** History:
**      05-Jan-2010 (kiria01) SIR 121883
**          Created
*/
static bool
opb_vars_in_bm(
	PST_QNODE	*node,
	OPV_BMVARS	*bmvars)
{
    while (node)
    {
	if (node->pst_sym.pst_type == PST_VAR)
	    return BTtest(node->pst_sym.pst_value.pst_s_var.pst_vno, (char*)bmvars);
	else if (node->pst_sym.pst_type == PST_CONST)
	    break;
	if (node->pst_right)
	{
	    if (!node->pst_left)
		node = node->pst_right;
	    else
	    {
		if (!opb_vars_in_bm(node->pst_right, bmvars))
		    return FALSE;
		node = node->pst_left;
	    }
	}
	else
	    node = node->pst_left;
    }
    return TRUE;
}


/*{
** Name: opb_gbfbm	- get boolean factor bitmaps for this node
**
** Description:
**      This routine will determine the boolean factors and the 
**      bitmaps to be evaluated at this node
**
** Inputs:
**      subquery                        ptr to subquery state variable
**      lojevalmap                      map of outer joins IDs which have
**					been entirely evaluated within
**					the left subtree 
**      rojevalmap                      map of outer joins IDs which have
**					been entirely evaluated within
**					the right subtree 
**	ojinnermap			ptr to a map of outer joins IDs which
**					have at least one occurrence of the
**					outer join in the subtree, (not 
**					including the current node)
**	cop				CO node if passed to be augmented
**					with any cardinality info from the
**					join.
**					In addition:
**					If not NULL the boolean factors which
**					need to be reevaluated should be
**					placed in the outer jxbfm map
**					- in either case the equivalence
**					classes for booleans factors which
**					fall into this category will always
**					be placed in leqr, reqr
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**
** Side Effects:
**
** History:
**      26-oct-88 (seputis)
**          initial creation
**	15-feb-94 (ed)
**	    - bug 59598 - correct mechanism in which boolean factors are
**	    evaluated at several join nodes
**	28-apr-94 (ed)
**	    - bug 59928, 59760 - missing eqc problem in OPC, need to make sure
**	    full join eqcls needed for ON boolean factors are available
**	11-jun-96 (inkdo01)
**	    Changes to force spatial join functions into upper TID join, even
**	    though they look to have been done at lower key join.
**	4-nov-97 (inkdo01)
**	    Change to request enough eqcs that TID join can execute the 
**	    spatial predicate.
**	12-jun-98 (inkdo01)
**	    Fix for bug which causes OP0482, apparently in folded outer
**	    joins.
**	28-aug-98 (inkdo01)
**	    Fixes to handle spatial join bug in which non-key spatial predicate
**	    is not applied.
**	 7-jul-99 (hayke02)
**	    Modify the fix for bug 87175 so that we now do not skip the
**	    boolean factor bitmap assignments for folded outer joins which
**	    have been marked as requiring a cartesian product join lower in
**	    the query tree. This change fixes bug 96265.
**	20-jun-00 (hayke02)
**	    Back out the fix for bug 87175 (and 96265) - this bug has been
**	    fixed by the fix for SIR 94906, bug 98117. This change fixes
**	    bug 101847.
**	23-aug-01 (hayke02)
**	    Re-introduce the fix for bug 87175 now that the change for SIR
**	    94906 has been backed out.
**	22-may-02 (hayke02)
**	    We now use maxoj (subquery->ops_oj.opl_lv) rather than maxbf for size
**	    in calls to BTsubset(). This prevents incorrect results when maxbf
**	    > 32 (OPL_MAXOUTER). This change fixes bug 107829.
**	22-may-02 (hayke02)
**	    When setting outer_join the test for bp->opb_ojid has been modified
**	    from != OPL_NOOUTER (-1) to >= 0. This is because the fix for bug
**	    102582 has added OPL_NOINIT (-2) as a valid opb_ojid value.
**	    This change fixes bug 107551.
**	10-jan-03 (hayke02)
**	    Re-introduce the fix for bug 96265 now that the change for SIR
**	    94906 has been backed out. This also fixes bug 108601.
**      24-jan-2005 (huazh01 for hayke02)
**          do not send up the subselect boolean factor if we have an or'ed
**          subselect. This is because opc_serip() will 'rip' the subselect
**          pst_node apart and we end up with an incorrectly compiled query. 
**          This fixes b111102, INGSRV2552. 
**	27-jan-05 (hayke02)
**	    Modify the fix for 96265 so that we skip the bitmap assignments for
**	    boolean factors that have been used in selectivity calculations
**	    (opb_mask & OPB_BFUSED). This change fixes bug 113764, problem
**	    INGSRV 3122.
**	08-jul-05 (inkdo01)
**	    Add group by vars to histogram EQC map.
**	30-Aug-2005 (schka24)
**	    Extract by-list var map to separate routine.
**	9-feb-2007 (dougi)
**	    Be more careful about what EQCs to add to leqr/reqr - fixes 
**	    bug 117655.
**	30-jul-08 (hayke02)
**	    Replace test for opb_ojid == OPL_NOOUTER (-1) with < 0, as we can
**	    also have OPL_NOINIT (-2). This change fixes bugs 114117 and
**	    120708.
**	13-Nov-2009 (kiria01) SIR 121883
**	    Distinguish between LHS and RHS card checks for 01.
**	    We have a problem. Although the parse tree has been annotated
**	    with the cardinality requirements for the left and right hand
**	    sides, this gets invalidated, probably by cost optimisation so that
**	    the order of CO inner and outer nodes may be reversed in sense
**	    from what the parse tree holds. How we can determine true left from
**	    from right is the issue. The only check we can make at present is
**	    to note when an outer join is in consistant with the parse tree.
**	    If this is the case we swap the inner-outer sence to follow.
**	    This will of course not work for cardinality checks on inners!
**	14-Jan-2010 (kiria01) SIR 121883
**	    Refine above change to inspect the consistancy of var maps
**	    with trees to determine orientation.
*/
VOID
opb_gbfbm(
	OPS_SUBQUERY       *subquery,
	OPE_BMEQCLS        *avail,
	OPE_BMEQCLS        *lavail,
	OPE_BMEQCLS        *ravail,
	OPL_BMOJ	   *lojevalmap,
	OPL_BMOJ	   *rojevalmap,
	OPL_BMOJ	   *ojevalmap,
	OPE_IEQCLS         maxeqcls,
	OPV_VARS           *lvarp,
	OPV_VARS           *rvarp,
	OPB_BMBF           *jxbfm,
	OPE_BMEQCLS        *jeqh,
	OPE_BMEQCLS        *jxeqh,
	OPE_BMEQCLS        *leqr,
	OPE_BMEQCLS        *reqr,
	OPV_BMVARS	   *lvmap,
	OPV_BMVARS         *rvmap,
	OPV_BMVARS	   *bvmap,
	OPL_IOUTER	   ojid,
	OPL_BMOJ	   *ojinnermap,
	OPO_CO		   *cop) /* Only present re-evaluating */
{
    /* 
    ** for each boolean factor -
    ** if the set of equivalence classes used in the factor is contained in 
    **     "lp->opn_eqm union rp->opn_eqm"    
    ** but not in
    **     "lp->opn_eqm or rp->opn_eqm" 
    ** then set bit in jxbfm.
    */
    OPB_IBF                bf;		/* current boolean factor being
					** analyzed */
    OPB_BFT                *bfbase;     /* ptr to base of array of ptrs
					** to boolean factor elements */
    OPL_OJT		   *lbase;	/* ptr to base of outer join descriptor
					** table */
    OPV_RT		   *vbase;	/* ptr to base of local range table */
    OPB_IBF		   maxbf;
    OPL_IOUTER		   maxoj;
    OPV_IVARS		   maxvar;
    OPV_IVARS		   rtvarno;
    OPV_VARS		   *rtvarp = NULL;
    OPE_IEQCLS		   rtreeeqc;
    bool		   rtree;	/* are we doing rtree processing? */
    bool		   opjcall = (cop != NULL);


    lbase = subquery->ops_oj.opl_base;
    maxeqcls = subquery->ops_eclass.ope_ev;
    MEfill(sizeof(*jxbfm), (u_char)0, (PTR)jxbfm ); /* init 0 - map of
					** boolean factors to be applied 
                                        ** at output of this join node */
    bfbase = subquery->ops_bfs.opb_base; /* get ptr to base of array of
					** ptrs to boolean factors */
    maxbf = subquery->ops_bfs.opb_bv;
    maxoj = subquery->ops_oj.opl_lv;
    vbase = subquery->ops_vars.opv_base;
    maxvar = subquery->ops_vars.opv_rv;
    rtree = FALSE;

    /* Two types of Rtree processing are done in this function. If the call
    ** is from opj_maordering, the boolean factor bitmap is being built and
    ** we need to determine predicates which must be applied at the Rtree
    ** TID join (the first place the Rtree indexed column is available), 
    ** rather than at the spatial join (where only the Hilbert numbers are 
    ** available). The Rtree logic for opj calls is designed to attach the
    ** factors to the TID join node.
    **
    ** If the call is from opn_sm2 (from opn_nodecost), the "required eqclass"
    ** bit maps are being built. If this is a TID join using TIDs from a
    ** spatial join, the eqclasses from Boolean factors which reference the
    ** Rtree indexed column must be added to the required list (in anticipation
    ** that the factors will be applied here).
    **
    ** The duality of logic for the opj_maordering/opn_sm2 calls is required
    ** because the former processes the plan nodes from leaf node to root
    ** and is suitable for identifying the factors to be applied at the spatial
    ** TID join, whereas the latter processes the nodes from the plan root
    ** down to the leaf nodes and is suitable for identifying the eqclasses
    ** required for the spatial TID joins. OPF complexity and Rtree 
    ** kludginess require it to work this way.
    */
    if (opjcall && rvarp && rvarp->opv_mask & OPV_SPATF) rtvarp = rvarp;

    if (!opjcall && subquery->ops_mask & OPS_SPATF)
    {
	/* Check outer tables (lvmap) for rtree secondary on current rvarp.
	** This means we're completing TID join from Rtree. */
	for (rtvarno = -1; (rtvarno = BTnext((i4)rtvarno, (char *)lvmap,
		maxvar)) != -1; )
	 if ((rtvarp = vbase->opv_rt[rtvarno])->opv_mask & OPV_SPATF &&
		vbase->opv_rt[rtvarp->opv_index.opv_poffset] == rvarp) break;
					/* if var is rtree index to rvarp */
	if (rtvarno == -1) rtvarp = NULL;
					/* reset rtvarp if none found */
    }
	
    if (rtvarp != NULL)
    {
	OPZ_IATTS	attno;
	OPZ_ATTS	*attp;

	/* Loop over joinop attr array, looking for Rtree key column. */
	for (attno = 0; attno < subquery->ops_attrs.opz_av; attno++)
	{
	    attp = subquery->ops_attrs.opz_base->opz_attnums[attno];
	    if (attp->opz_attnm.db_att_id == 1 &&
		rtvarp == vbase->opv_rt[attp->opz_varnm])
	     break;		/* found the Rtree target column */
	}
	if (attno < subquery->ops_attrs.opz_av)
	{
	    rtree = TRUE;
	    rtreeeqc = attp->opz_equcls;
					/* save key column's eqclass */
	}
    }

    for (bf = maxbf; bf--;)
    {
	bool               allintree;   /* TRUE if boolean factor
					** can be evaluated at this
					** node */
	bool               notinsubtree; /* TRUE if boolean factor
					** cannot be evaluated in either
					** subtree */
	OPB_BOOLFACT       *bp;         /* ptr to current boolean factor
					** element being analyzed */
	OPV_BMVARS	    tempmap;	/* at least one variable from the inner
					** must exist in the subtree for the
					** outer join qualification to be executed
					** at this point */
	bool		    left_bool;
	bool		    right_bool;
	bool		    outer_join;	/* TRUE if this boolean factor was 
					** introduced as part of a outer join
					** qualification */
	bool		    ojboolcheck; /* TRUE if special OJ placement
					** of boolean factors is required */
	bool		    sendojeqcup; /* TRUE if the boolean factor can be
					** evaluated at the current level but
					** needs to also be evaluated once again
					** at higher levels, therefore eqcls
					** must be made available for this 
					** purpose */
	bool		    spbfact;	/* TRUE if curr fact is spatial join 
					** and this is TID join node */

	bp = bfbase->opb_boolfact[bf];  /* get ptr to boolean factor element
					*/
	if (rtree && opjcall && !(bp->opb_mask & OPB_SPATJ) &&
		BTtest((i4)rtreeeqc, (char *)&bp->opb_eqcmap))
	 bp->opb_mask |= OPB_SPATP;	/* if this is spatial join node and
					** non-key spatial predicate, flag the
					** predicate as "pending" execution
					** at the corresponding TID join node.
					** When the corr. TID join is encountered,
					** the pending preds are set in its
					** Boolean factor bit map */
	outer_join = lbase && (bp->opb_ojid >= 0);
	ojboolcheck = ((bp->opb_mask & OPB_OJMAPACTIVE) != 0);
	allintree = BTsubset ((char *)&bp->opb_eqcmap, (char *)avail, 
	    (i4) maxeqcls)             /* TRUE if equivalence classes
					** used in boolean factor is a
					** subset of the equivalence classes
					** available at this node */
	    &&
	    (	!ojboolcheck
		||
	        /* this boolean factor cannot be evaluated until a certain
		** set of outer joins have been evaluated, this situation
		** occurs in cases of nested outer joins in which
		** an attribute can be set to NULL at more than one
		** outer join node */
		BTsubset((char *)&bp->opb_ojmap, (char *)ojevalmap,
		    (i4)maxoj))
	    &&
	    (
		!outer_join
		||
		(bp->opb_ojid == ojid)
		||
		BTtest((i4)bp->opb_ojid, (char *)ojinnermap)
		||
		!bvmap 
		||
		BTsubset((char *)bvmap, (char *)
		    lbase->opl_ojt[bp->opb_ojid]->opl_maxojmap, (i4)maxvar)
					/* check if subtree tries to evaluated
					** outer join qualification outside
					** scope of outer join e.g. select
					** * from r left join s on r.a>5
					** should not evaluate on "r.a" in
					** "r" subtree */
					
	    );
	left_bool = right_bool = TRUE;
	if  (	(bp->opb_virtual && lvarp) /* a
					** subselect cannot be evaluated
					** at a leaf so place any boolean
					** factor which contains a subselect
					** at the appropriate interior node
					*/
		||
		(   left_bool = !BTsubset((char *)&bp->opb_eqcmap, (char *)lavail, 
			(i4)maxeqcls)	/* check if boolean factor can be
					** evaluated in the left tree */
		)
		||
		(   ojboolcheck
		    && 
		    !BTsubset((char *)&bp->opb_ojmap, (char *)lojevalmap,
			(i4)maxoj)	/* do not evaluate lower if attributes
					** change to NULL */
		)
	    )
	    notinsubtree = TRUE;
	else if ( !left_bool && outer_join )
	{
	    /* even though the boolean factor can be placed lower in the
	    ** tree, the semantics for outer joins may require it to
	    ** to be evaluated at this point since we cannot eliminate tuples
	    ** until an inner is involved in the subtree */
	    notinsubtree =
		(   !BTtest((i4)bp->opb_ojid, (char *)ojinnermap)
		    /* ojinnermap is a map of outer join ids which have at least one
		    ** minoj variable in this subtree, so there will be at least one
		    ** more outer join evaluated,... since the placement strategy
		    ** requires that all boolean factors for the outer join at this
		    ** point cannot be evaluated lower, or if it can then it must
		    ** must be evaluated within inner relations only, or at another
		    ** outer join with the same ID as this one,
		    ** - if the outer join is evaluated at this node and not lower
		    ** it should not be in ojinnermap
		    ** - if the ON clause boolean factor references only outer
		    ** relation attributes then this test will cause those
		    ** attributes to be made available at the first outer
		    ** join node for that on clause e.g. select * from 
		    ** r left join s on r.a > 5;  so that r.a is required to
                    ** be evaluated at the join node
		    */
		    &&
		    lvmap /* if this subtree contains only inner relations
		    ** then it is possible to evaluate the boolean factor
		    ** as low as possible in the tree, it may be necessary
		    ** to reevaluate it higher up, but that can be taken
		    ** care of by opl_ojverify below */
		    &&
		    !BTsubset((char *)lvmap, (char *)
			lbase->opl_ojt[bp->opb_ojid]->opl_maxojmap, (i4)maxvar)
		    /* in the case of select r left join s on r.a=s.a and
                    ** s.a > 5, it would be reasonable to evaluate the boolean
                    ** factor in the "s" subtree but not the "r" subtree,
		    ** FIXME - there may be an unnecessary evaluation of the
		    ** boolean factor at the join node in the case of
		    ** select r left join s on r.a=s.a and r.a > 5, but
		    ** it would be desirable to evaluate the boolean factor in the
		    ** "s" subtree due to the transitive property */
		);			/* since
					** all relations in this map
					** are inner to the outer join
					** the boolean factor should
					** be able to be evaluated lower
					** in the tree */
	}
	else
	    notinsubtree = FALSE;

	if (notinsubtree)
	{
	    if(	(bp->opb_virtual && rvarp)
		||
		(   right_bool = !BTsubset((char *)&bp->opb_eqcmap, (char *)ravail,
			(i4)maxeqcls)
					/* check if boolean factor can be
					** evaluated in the right tree */
		)
		||
		(   ojboolcheck
		    && 
		    !BTsubset((char *)&bp->opb_ojmap, (char *)rojevalmap,
			(i4)maxoj)	/* do not evaluate lower if attributes
					** change change to NULL */
		)
	      )				/* TRUE - if neither subtree contain
					** sufficient equivalence classes
					** to evaluate boolean factor */
		;
	    else if (!right_bool && outer_join)
	    {
		notinsubtree =
		    (   !BTtest((i4)bp->opb_ojid, (char *)ojinnermap)
			&&
			rvmap 
			&&
			!BTsubset((char *)rvmap, (char *)lbase->opl_ojt
			    [bp->opb_ojid]->opl_maxojmap, (i4)maxvar)
		    );
	    }
	    else
		notinsubtree = FALSE;
	}

	    /* for the case in which a where clause qualification can be
	    ** evaluated within the scope of an outer join, then attributes
	    ** may need to be sent up, in order to reevaluate the 
	    ** qualification e.g. select * from r left join s on r.a=s.a
	    ** where r.a > 5 or s.b is NULL ... the where clause can be
	    ** evaluated in the scope of the outer join but also must be
	    ** evaluated again at the join node in order to eliminate 
	    ** tuples which may have been preserved by the outer join */
	sendojeqcup = 
	    (!left_bool || !right_bool)
	    &&
	    (!notinsubtree || allintree)
	    &&
	    (bp->opb_mask & OPB_OJATTREXISTS)
	    /* this check is only needed if at least one attribute
	    ** of an inner relation in an outer join is referenced
	    ** in the qualification */
	    &&	    
	    /* determine if the boolean factor will need
            ** to be reevaluated further up the tree until a point is reached
            ** when variables equal to or greater than in oj scope exist is 
            ** reached in each equivalence class 
            ** select r.a from (r left join s on s.a=r.a) where r.a>5 or s.b =6
            ** the where clause can be evaluated within the project rest node
            ** for s, but needs to be reevaluated again at the output node
            ** of the join */
	    (ojid != bp->opb_ojid)
	    &&
	    (	(bp->opb_ojid < 0)	/* where clause boolean factor needs
					** to be evaluated at all outer joins
					** since null semantics can change */
		||
		(   (ojid >= 0)
		    /* since this is a nested outer join situation, the
		    ** value of some attributes can be changed to NULL, need
		    ** to determine if this ojid is lower in scope than the
		    ** boolean factor and re-evaluate it if the boolean factor
		    ** is about to evaluate lower */
		    && 
		    !BTtest((i4)bp->opb_ojid, (char *)lojevalmap)
		    &&
		    !BTtest((i4)bp->opb_ojid, (char *)rojevalmap)
		    /* FIXME - it would not be necessary to reevaluate the
		    ** boolean factor if the attribute can come from the outer
		    ** tree of a oj child e.g. select r.c from (r left join
		    ** (s left join t on s.b=t.b) on r.a=s.a and s.c=5)
		    ** would current reevaluate s.c=5 when this was not
		    ** necessary since it could be safely evaluated at the
		    ** orig node for s */
		)
	    );
	if (outer_join
	    &&
	    (lbase->opl_ojt[bp->opb_ojid]->opl_type == OPL_FULLJOIN)
	    &&
	    !BTtest((i4)ojid, (char *)ojinnermap) /* if outerjoin is
					    ** evaluated in the subtree then
					    ** this is a TID join on top of
					    ** a full join since full joins
					    ** cannot be split in the current
					    ** model except for TID joins */
	    &&
	    BTsubset((char *)bvmap, (char *)lbase->opl_ojt[bp->opb_ojid]->
		opl_ojtotal, (i4)maxvar) /* check if relations are in
					    ** the scope of this FULL join */
	    )
	{
	    /* for full join, all boolean factors need to be evaluated at
	    ** join node since tuples cannot be eliminated lower in the tree */
	    if (ojid == bp->opb_ojid)
	    {
		notinsubtree = TRUE;    /* FULL JOIN boolean factors must
					** be evaluated at the FULL join node
					** and cannot be evaluated lower since
					** it would remove tuples needed for
					** the outer join node */
		allintree = TRUE;
	    }
	    else
	    {   /* cause equivalence classes to be retrieved to evaluate
		** outer join for parent */
		notinsubtree = TRUE;
		allintree = FALSE;
	    }
	}
	if ((notinsubtree || sendojeqcup) /* if boolfact is not
					** completely contained in
					** either left or right subtree
					** then a histogram will be
					** needed for some of its
					** eqclasses */
	    &&
	    leqr 			/* and if leqr is requested, i.e. */
	    &&				/* it is not NULL */
	    (bp->opb_ojid < 0 ||
	    (lbase->opl_ojt[bp->opb_ojid]->opl_type != OPL_FOLDEDJOIN) ||
	    ((lbase->opl_ojt[bp->opb_ojid]->opl_mask & (OPL_OJCARTPROD | OPL_BOJCARTPROD)) && !(bp->opb_mask & OPB_BFUSED))))
	{
	    OPE_IEQCLS            bfeqcls; /* current equivalence class of
					** boolean factor being analyzed */
	    OPZ_IATTS		  attno;
	    OPE_BMEQCLS		  ojeqcs;
	    OPE_IEQCLS		  attreqc;

	    /* If factor has OPB_OJATTREXISTS set, place eqcs of attrs in 
	    ** opb_ojattr into ojeqcs, so we only add eqcs that we REALLY 
	    ** need. Fixes bug 117655. */
	    MEfill(sizeof(ojeqcs), (u_char)0, &ojeqcs);
	    if (bp->opb_mask & OPB_OJATTREXISTS)
	    for (attno = -1; (attno = BTnext((i4)attno, (PTR)bp->opb_ojattr,
			subquery->ops_attrs.opz_av)) >= 0; )
	     if ((attreqc = subquery->ops_attrs.opz_base->opz_attnums[attno]->
			opz_equcls) >= 0)
		BTset((i4)attreqc, (PTR)&ojeqcs);

	    for (bfeqcls = -1; (bfeqcls = BTnext((i4)bfeqcls, 
						 (char *)&bp->opb_eqcmap, 
						 (i4)maxeqcls)
			       ) >= 0 ; ) /* for each eqclass in this
					** boolfact which is in this 
					** subtree */
	    {
		if (BTtest((i4)bfeqcls, (char *)avail))
		{   /* this equivalence class is in the subtree */
		    if (notinsubtree	/* for outer joins just ask
					** for the equivalence class
					** but not the histogram since
					** histogramming is not done for
					** boolean factors evaluated
					** multiple times */
			&&
			opb_sfind (bp, bfeqcls)) /* if the eqclass
					** exists in a sargable clause */
		    {
			BTset((i4)bfeqcls, (char *)jeqh); /* we need a histo
					** for it from the join */
			if (!allintree || sendojeqcup) /* if boolfact has eqclasses
					** outside this subtree then
					** the histogram for this eqclass
					** needs to be sent back up */
			    BTset((i4)bfeqcls, (char *)jxeqh);
		    }
		    else if (!notinsubtree && sendojeqcup &&
				!BTtest((i4)bfeqcls, (PTR)&ojeqcs))
			continue;	/* Skip the flag set when we're only
					** here because of OPB_OJATTREXISTS
					** and this eqc isn't amongst the
					** opb_ojattr */
		    if (BTtest((i4)bfeqcls, (char *)lavail)) /* if in 
					** left subtree set map, else set 
					** right subtree map (if in both 
					** it was handled previously when
					** the joining eqclass was chosen)
					*/
			BTset((i4)bfeqcls, (char *)leqr);
		    else if (BTtest((i4)bfeqcls, (char *)ravail)) /* if in 
					** the right subtree then set the map
					*/
			BTset((i4)bfeqcls, (char *)reqr);
		    else
		    {	/* this is probably a function attribute which is used
			** to evaluate this boolean factor; most often it is
			** the outer join special equivalence class; previously
			** function attribute would only be used in joins, but
			** now they are used in other ways */
			
		    }
		}
	    }
	}
	/* If this factor contains a reference to the key column of an Rtree
	** index lower in the join tree, ALL its eqclasses must be added to
	** the required list. */
	if (rtree && !opjcall && BTtest((i4)rtreeeqc, (char *)&bp->opb_eqcmap))
	 BTor((i4)maxeqcls, (char *)&bp->opb_eqcmap, (char *)leqr);

	/* If this is a spatial join boolean factor and it has already been 
	** covered in the left subtree, check to see if rvarp is the 2nd
	** base table of the join. If so, the spatial join factor must be 
	** re-evaluated here. */
	if (allintree && !notinsubtree && bp->opb_mask & OPB_SPATJ && rvarp)
	{
	    OPE_IEQCLS	speqc;
	    OPB_IBF	bf1;

	    speqc = BTnext((i4)-1, (char *)&bp->opb_eqcmap,
		(i4)maxeqcls);		/* get 1st eqclass ref'ed by bf */
	    spbfact = opb_spattr_check(subquery, rvarp, speqc);
	    if (!spbfact && (speqc = BTnext((i4)speqc, (char *)&bp->opb_eqcmap,
		(i4)maxeqcls)) >= 0)
		spbfact = opb_spattr_check(subquery, rvarp, speqc);
					/* if 1st eqclass didn't work, 
					** try 2nd eqclass, too */
	    if (spbfact && leqr) BTor((i4)maxeqcls, (char *)&bp->opb_eqcmap,
		(char *)leqr);		/* request all bp eqs from below */

	    /* Next, see if any pending (non_key) spatial predicates can be
	    ** resolved at this node. They are simply the OPB_SPATP predicates
	    ** whose opb_eqcmap contain speqc. */
	    if (opjcall)
	     for (bf1 = maxbf; bf1--;)
	    {
		OPB_BOOLFACT	*bp1 = bfbase->opb_boolfact[bf1];

		if (bp1 == bp) continue;  /* skip current factor */
		if (bp1->opb_mask & OPB_SPATP && 
			BTtest((i4)speqc, (char *)&bp1->opb_eqcmap))
		{
		    bp1->opb_mask &= ~OPB_SPATP;
					/* turn off flag */
		    BTset((i4)bf1, (char *)jxbfm);
					/* tag to this node */
		}
	    }
	}
	else spbfact = FALSE;		/* NOT a spatial boolfact */
	if (allintree)
	{
	    if (notinsubtree || spbfact)  /* if boolfact contained within
					** this subtree but not
					** completely in either left
					** subtree or right subtree
					** OR it's a qualifying spatial
					** join boolean factor,
					** then this boolean factor
					** will be evaluated at this node */
	    {
		BTset((i4)bf, (char *)jxbfm);
	    }
	    else if (opjcall /* Second pass - re-eval bf */)
	    {	/* for those boolean factors which can be evaluated lower in
		** the tree, but need to be reevaluated at this point to eliminate
		** outer tuples e.g. select * from r left join s on r.a=s.a 
		** where r.a >1 OR s.b < 1 would normally be evaluated only
		** in the "s" subtree, but needs to be reevaluated at the output of
		** the join node */

		if (sendojeqcup
		    &&
		    (ojid >= 0)
		    &&
		    (	(bp->opb_ojid < 0)
			||
			BTsubset((char *)lbase->opl_ojt[ojid]->opl_maxojmap,
			    (char *)lbase->opl_ojt[bp->opb_ojid]->opl_maxojmap,
			    (i4)maxvar) /* one outer join is being forced into
					** the scope of an inner outer join so
					** the boolean factor needs to be 
					** reevaluated */
		    )
                    &&
                    /* INGSRV2552, b111102 */
                    !(bp->opb_virtual
                    &&
                    bp->opb_virtual->opb_subselect
                    &&
                    bp->opb_orcount))
		{
                    BTset((i4)bf, (char *)jxbfm);
		}
	    }
	}

	if (opjcall)
	{
	    i4 opmeta;

	    /* We may have cardinality restrictions on this node
	    ** that need placing in the cop node. */
	    if (bp->opb_qnode->pst_sym.pst_type == PST_BOP &&
		(opmeta = bp->opb_qnode->pst_sym.pst_value.pst_s_op.pst_opmeta) &&
		!(bp->opb_qnode->pst_sym.pst_value.pst_s_op.pst_flags & PST_OPMETA_DONE))
	    {
		OPL_OJTYPE jtype = cop->opo_variant.opo_local->opo_type;
		if (opb_vars_in_bm(bp->opb_qnode->pst_left,
			    cop->opo_outer->opo_variant.opo_local->opo_bmvars) &&
		    opb_vars_in_bm(bp->opb_qnode->pst_right,
			    cop->opo_inner->opo_variant.opo_local->opo_bmvars))
		{
		    /* If this is an outer join we can make a sanity check. */
		    if (jtype == OPL_RIGHTJOIN && opmeta == PST_CARD_01R ||
			jtype == OPL_LEFTJOIN && opmeta == PST_CARD_01L)
			opx_1perror(E_OP009D_MISC_INTERNAL_ERROR,
				    "opmeta conflict jt");

		    switch (opmeta)
		    {
		    case PST_CARD_01R:
			if (cop->opo_card_inner != opmeta)
			{
			    if (cop->opo_card_inner)
				opx_1perror(E_OP009D_MISC_INTERNAL_ERROR,
					    "opmeta conflict");
			    cop->opo_card_inner = opmeta;
			}
			break;
		    case PST_CARD_01L:
			if (cop->opo_card_outer != OPO_CARD_01)
			{
			    if (cop->opo_card_outer)
				opx_1perror(E_OP009D_MISC_INTERNAL_ERROR,
					    "opmeta conflict");
			    cop->opo_card_outer = OPO_CARD_01;
			}
			break;
		    }
		    /* We do not want to apply this again further up the cop tree so mark
		    ** the node that opmeta is now in cop node. (Cannot clear opmeta as
		    ** sejoin processing will still need it) */
		    bp->opb_qnode->pst_sym.pst_value.pst_s_op.pst_flags |= PST_OPMETA_DONE;
		}
		else if (opb_vars_in_bm(bp->opb_qnode->pst_right,
			    cop->opo_outer->opo_variant.opo_local->opo_bmvars) &&
		    opb_vars_in_bm(bp->opb_qnode->pst_left,
			    cop->opo_inner->opo_variant.opo_local->opo_bmvars))
		{
		    cop->opo_is_swapped = TRUE;
		    /* If this is an outer join we can make a sanity check. */
		    if (jtype == OPL_RIGHTJOIN && opmeta == PST_CARD_01L ||
			jtype == OPL_LEFTJOIN && opmeta == PST_CARD_01R)
			opx_1perror(E_OP009D_MISC_INTERNAL_ERROR,
				    "opmeta conflict jt");
		    switch (opmeta)
		    {
		    case PST_CARD_01R:
			if (cop->opo_card_outer != opmeta)
			{
			    if (cop->opo_card_outer)
				opx_1perror(E_OP009D_MISC_INTERNAL_ERROR,
					    "opmeta conflict");
			    cop->opo_card_outer = opmeta;
			}
			break;
		    case PST_CARD_01L:
			if (cop->opo_card_inner != OPO_CARD_01)
			{
			    if (cop->opo_card_inner)
				opx_1perror(E_OP009D_MISC_INTERNAL_ERROR,
					    "opmeta conflict");
			    cop->opo_card_inner = OPO_CARD_01;
			}
			break;
		    }
		    /* We do not want to apply this again further up the cop tree so mark
		    ** the node that opmeta is now in cop node. (Cannot clear opmeta as
		    ** sejoin processing will still need it) */
		    bp->opb_qnode->pst_sym.pst_value.pst_s_op.pst_flags |= PST_OPMETA_DONE;
		}
	    }
	}
    }
}


/*{
** Name: opb_spattr_check	- see if current eqclass matches TID 
**	join on top of spatial key join.
**
** Description:
**      This routine applies a couple of tests designed to determine if 
**	the input eqclass (extracted from the caller's boolean factor
**	structure) covers the secondary Rtree index-to-base table mapping 
**	required for a TID join on top of a spatial key join.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      speqc				EQclass being analyzed
**	varp				ptr to range table variable on
**					right side of node being analyzed
** Outputs:
**	none
**
** Returns:
**	flag indicating whether eqclass passed tests
** Exceptions:
**	none
**
** Side Effects:
**	none
**
** History:
**	11-jun-96 (inkdo01)
**          initial creation
[@history_line@]...
*/
static bool
opb_spattr_check(OPS_SUBQUERY	*subquery,
		OPV_VARS	*varp,
		OPE_IEQCLS	speqc)

{
    OPZ_ATTS	*attrp;
    OPZ_IATTS	att;
    OPV_IVARS	varno;
    bool	test1 = FALSE, test2 = FALSE;

    /* A qualifying eqclass will have at least 2 constituent attributes -
    ** one is the secondary Rtree index column and the other is the 
    ** corresponding base table indexed column. This loop processes each
    ** attribute, applying each of the qualifying tests. */

    for (varno = 0; varno < subquery->ops_vars.opv_prv; varno++)
     if (varp == subquery->ops_vars.opv_base->opv_rt[varno]) break;
    if (varno == subquery->ops_vars.opv_prv) return(FALSE);
				/* first find inner varno - FALSE if
				** not a base table */

    for (att = -1; (att = BTnext((i4)att, (char *)&subquery->ops_eclass.
	ope_base->ope_eqclist[speqc]->ope_attrmap, (i4)subquery->ops_attrs.
	opz_av)) >= 0;)
				/* loop over attrs in eqclass */
    {
	attrp = subquery->ops_attrs.opz_base->opz_attnums[att];
	if (!test1 && attrp->opz_varnm == varno)
	 test1 = TRUE;		/* rvarp is covered by spatial func */
	else if (!test2 && attrp->opz_varnm >= subquery->ops_vars.opv_prv &&
	    subquery->ops_vars.opv_base->opv_rt[attrp->opz_varnm]->
	    opv_index.opv_poffset == varno)
	 test2 = TRUE;		/* Rtree col is covered by eqclass */
	if (test1 && test2) return(TRUE);
				/* quit now, if both are TRUE */
    }

    return(FALSE); 		/* must've flunked to get here */
}	/* end of opb_spattr_check */

/* Name: opn_bylist_map - set up bit map for BY-list eqc's
**
** Description:
**	If the subquery contains a BY-list, set up a map of the equivalence
**	classes for the BY-attributes (if they are simple variables;  we
**	won't try to deal with expressions).  Only BY-vars which are
**	available in either the left or right subtrees are considered.
**
**	The map will be used to request the histograms for those BY-vars,
**	so that the histograms can be carried up the tree and eventually
**	used to produce an estimate of the number of groups emerging
**	from the aggregate.
**
** Inputs:
**	subquery		The subquery being analyzed.
**	byeqcmap		(an output) Bit-map of BY-list eqc's
**	lavail			Map of eqc's available from the left subtree
**	ravail			Map of eqc's available from the right subtree
**
** Outputs:
**	byeqcmap		Filled in with BY-list eqc's, if any
**	No return value
**
** History:
**	1-Sep-2005 (schka24)
**	    Extract from gbfbm, fix typo (left->right) that prevented
**	    BY-list from being traversed properly.  Return BY-list map
**	    separately from jeqh so that outer join stuff doesn't puke
**	    on the extra bits.  (It would think that the extra bits
**	    represent non-joining ON-clause predicates and they aren't.)
*/

static void
opn_bylist_map(OPS_SUBQUERY *subquery, OPE_BMEQCLS *byeqcmap,
	OPE_BMEQCLS *lavail, OPE_BMEQCLS *ravail)
{
    OPZ_AT		*abase = subquery->ops_attrs.opz_base;
    OPE_IEQCLS		byeqc;		/* Equiv class for BY-list attr */
    PST_QNODE		*byresp, *byvalp;

    MEfill(sizeof(OPE_BMEQCLS), (u_char) 0, (PTR)byeqcmap);
					/* init map of EQCs in the BY list */

    /* Run through the BY-list (if any) until we hit a non-resdom,
    ** consider any list items that are simple VAR's.
    */
    for (byresp = subquery->ops_byexpr;
	 byresp != NULL && byresp->pst_sym.pst_type == PST_RESDOM; 
	 byresp = byresp->pst_left)
    {
	byvalp = byresp->pst_right;
	if (byvalp->pst_sym.pst_type == PST_VAR)
	{
	    byeqc = abase->opz_attnums[
		byvalp->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id]->opz_equcls;
	    if (BTtest(byeqc, (char *)lavail) || BTtest(byeqc, (char *)ravail))
		BTset(byeqc, (char *) byeqcmap);
	}
    }

} /* opn_bylist_map */

/*{
** Name: opn_sm2	- set up bit maps required by opn_nodecost
**
** Description:
**      This routine will set up the bit maps of the required equivalence
**      classes, and the boolean factors which can be evaluated at this node.
**      The jxeqh and jeqh maps are not entirely complete after this routine;
**      later after iomake (interesting orderings) are calculated, these bits
**      will also be included in the map.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      np                              ptr to current node of join tree
**                                      being analyzed (non-leaf node)
**      eqr                             eqclasses required above
**
** Outputs:
**      jxeqh                           histograms required for boolean 
**                                      factors to be applied at nodes above 
**                                      this one (output) 
**      jxbfm		                boolean factors to be applied at the 
**                                      output of this join node (output)
**      jeqh                            histograms required for boolean 
**                                      factors to be applied at this node 
**                                      and nodes above this one (output)
**	byeqcmap			Eqclasses needed for aggregate BY-list
**					vars so that we can get histograms
**      leqr                            eqclasses required from the left 
**                                      subtree (output)
**      reqr                            eqclasses required from the right 
**                                      subtree (output)
**      jxcnt 		                used by oph_relselect to consider the 
**                                      selectivity of joining predicates
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	30-may-86 (seputis)
**          initial creation
**	26-oct-88 (seputis)
**          add corelated parameter
**	24-jul-89 (seputis)
**          increment jxcnt for function attributes which must be evaluated
**          at this node, b6966
**      29-mar-90 (seputis)
**          fix byte alignment problems
**      28-jan-92 (seputis)
**          top sort node removal enhancement, request equivalence classes
**          of secondary index in case they can be used for top sort
**          node removal, FIXME - introduce more checks for cases in
**          which the extra cost of requesting equivalence classes will
**          not be useful.
**      1-apr-92 (seputis)
**          b40687 - part of tid join estimate fix.
**      5-jan-93 (ed)
**          b45695 - evaluate multi-variable function attribute and
**          request the other half of the equi-join if necessary, i.e.
**          if the equi-join needs to be evaluated as a boolean factor
**	15-apr-93 (ed)
**	    - support right tid FSM join, by requesting implicit tid
**	    at this point so it can be sorted in this special case
**	30-nov-93 (ed)
**	    - correct outer join problem in which special eqcls were
**	    not requested at the correct node,... this case is new
**	    since previously a function attribute was never needed
**	    to evaluate another function attribute at the same node.
**	31-jul-97 (inkdo01)
**	    Added code to set SVAR func attrs into EQC maps at the right
**	    point in the join tree, when func att is ref'ed in ON clause.
**	    This ensures prpoer null semantics.
**	10-feb-04 (inkdo01)
**	    (Possibly temporarily) disable KJOINs on partitioned tables.
**	16-Jul-2004 (schka24)
**	    Turn inner-partitioned kjoins back on, QEF can do them now.
**	9-aug-05 (inkdo01)
**	    Improve predicate placement in the face of OJs.
**	24-aug-05 (inkdo01)
**	    Drop preceding change - problem was solved differently.
**	1-Sep-2005 (schka24)
**	    Return BY-list eqc map independently of jeqh.
[@history_line@]...
*/
VOID
opn_sm2(
	OPS_SUBQUERY       *subquery,
	OPN_JTREE          *np,
	OPE_BMEQCLS        *jxeqh,
	OPB_BMBF           *jxbfm,
	OPE_BMEQCLS        *jeqh,
	OPE_BMEQCLS        *byeqcmap,
	OPE_BMEQCLS        *leqr,
	OPE_BMEQCLS        *reqr,
	OPE_BMEQCLS        *eqr,
	OPN_JEQCOUNT       *jxcnt,
	OPO_ISORT          *jordering,
	OPO_ISORT          *jxordering,
	OPO_ISORT          *lkeyjoin,
	OPO_ISORT          *rkeyjoin)
{
    OPE_IEQCLS          maxeqcls;           /* maximum number of equivalence
                                            ** class defined */
    OPV_VARS		*lvarp;             /* ptr to left variable descriptor
					    ** if it is a leaf */
    OPV_VARS            *rvarp;             /* ptr to right variable descriptor
                                            ** if it is a leaf */
    OPN_JTREE		*lp;		    /* ptr to left child subtree of
					    ** this node */
    OPN_JTREE		*rp;		    /* ptr to right child subtree of
					    ** this node */
    OPV_IVARS            maxvar;	    /* number of range variables in
					    ** subquery */
    OPE_BMEQCLS         faeqcmap;           /* map of function attributes
                                            ** used only for consistency
                                            ** check */

    maxvar = subquery->ops_vars.opv_rv;	    /* number of joinop range
                                            ** variables defined */
    lp = np->opn_child[OPN_LEFT];   /* get left child of this subtree */
    rp = np->opn_child[OPN_RIGHT];	/* get right child of this subtree*/
    maxeqcls = subquery->ops_eclass.ope_ev; /* get "highest" numbered of 
                                            ** equivalence classes */
    /* initialize some bit maps */
    MEfill(sizeof(*jxeqh), (u_char)0, (PTR)jxeqh ); /* init 0 - map
                                            ** of histograms 
					    ** required for boolean factors
                                            ** above this node */
    MEfill(sizeof(*jeqh), (u_char)0, (PTR)jeqh ); /* init 0 - same 
                                            ** as jxeqh but includes histograms
                                            ** for this node also */
    *jxordering = OPE_NOEQCLS;
    if (lp->opn_nleaves == 1)		    /* TRUE if left subtree is a leaf */
    {
	lvarp =subquery->ops_vars.opv_base->opv_rt[lp->opn_pra[0]]; /* get the
					    ** descriptor for this leaf node */
    }
    else
	lvarp = NULL;
    if (rp->opn_nleaves == 1)		    /* TRUE if right subtree is a leaf*/
    {
	rvarp =subquery->ops_vars.opv_base->opv_rt[rp->opn_pra[0]]; /* get the
					    ** descriptor for this leaf node */
    }
    else
	rvarp = NULL;

    {
	OPE_IEQCLS          tideqc;	    /* joining tid equivalence class
                                            ** if it exists */
	bool		    left_imtid;	    /* TRUE if left subtree is a leaf
					    ** which has the implicit TID
					    ** involved in a TID join 
                                            ** - defined only if tideqc is
                                            ** found */
	bool		    right_imtid;    /* TRUE if left subtree is a leaf
					    ** which has the implicit TID
					    ** involved in a TID join 
                                            ** - defined only if tideqc is
                                            ** found */
	OPE_IEQCLS          eqcls;	    /* joining equivalence class being
                                            ** analyzed */
	OPE_IEQCLS          joining_eqcls;  /* last joining equivalence class
					    ** which was analyzed */

	joining_eqcls = OPE_NOEQCLS;
	MEcopy ((PTR)&lp->opn_eqm, sizeof(OPE_BMEQCLS), (PTR)leqr);
	BTand((i4)BITS_IN(OPE_BMEQCLS), 
	    (char *)&rp->opn_eqm, 
	    (char *)leqr);		    /* all joining equivalence classes
                                            ** are now in joineqcs */
	tideqc = OPE_NOEQCLS;               /* no tid equivalence class yet */
        right_imtid = FALSE;
	left_imtid = FALSE;
	*jxcnt = OPN_NJEQCLS;		    /* - init to no joining equivalence
                                            ** equivalence classes found yet
                                            ** - the first one is the joining
					    ** eqclass, the rest are only
                                            ** joining predicate */
    
	for (eqcls = -1; 
	    (eqcls = BTnext((i4)eqcls, (char *)leqr, (i4)maxeqcls))
		>= 0;)			    /* for each possible joining
					    ** eqclass (it exists in both
					    ** subtrees) */
	{
	    OPE_EQCLIST		*eqclsp;    /* ptr to joining equivalence class
					    */
	    (*jxcnt)++;			    /* 0 implies one joining equivalence
					    ** class has been found... and >0
					    ** implies more than one */
	    eqclsp = subquery->ops_eclass.ope_base->ope_eqclist[eqcls]; /* get
					    ** ptr to joining equivalence class
					    */
	    joining_eqcls = eqcls;
	    if (eqclsp->ope_eqctype == OPE_TID) /* is this a TID equivalence 
					    ** class */
	    {
		OPL_OJTYPE	ojjointype; /* used to help support a right
					    ** tid FSM join, i.e. need to request
					    ** implicit tid in this special case
					    ** for the sort node */

		if (subquery->ops_oj.opl_base
		    &&
		    (np->opn_ojid >= 0))
		    ojjointype = opl_ojtype(subquery, np->opn_ojid,
			&lp->opn_rlmap, &rp->opn_rlmap);
		else
		    ojjointype = OPL_INNERJOIN;
		if (
		     lvarp		    /* is this a leaf node */
		     &&
		     (	(lvarp->opv_index.opv_poffset == lp->opn_pra[0])
			||
			(lvarp->opv_index.opv_poffset < 0) /* check if this
                                            ** is a primary relation */
		     )
		     &&
		     (lvarp->opv_primary.opv_tid == eqcls) /* is this the
					    ** tid equivalence class for the
					    ** primary relation */
		    )			    /* TRUE if implicit TID joined on
					    ** left tree */
		{
		    right_imtid = FALSE;
		    left_imtid = TRUE;	    /* left side has the implicit TID */
		}
		else if (
		     rvarp		    /* is this a leaf node */
		     &&
		     (	(rvarp->opv_index.opv_poffset == rp->opn_pra[0])
			||
			(rvarp->opv_index.opv_poffset < 0) /* check if this
                                            ** is a primary relation */
		     )
		     &&
		     (rvarp->opv_primary.opv_tid == eqcls) /* is this the
					    ** tid equivalence class for the
					    ** primary relation */
		    )			    /* TRUE if implicit TID joined on
					    ** left tree */
		{
		    right_imtid = TRUE;	    /* right side contains the implicit
                                            ** TID to be joined */
		    left_imtid = FALSE;
		}
		else
		{   /* implicit TID not found */
		    right_imtid = FALSE;
		    left_imtid = FALSE;
		}


		if ((right_imtid || left_imtid)
		    &&
		    (tideqc == OPE_NOEQCLS)	    /* only look at first implicit TID
						** join, user may want to join a
						** relation to itself, on the
						** implicit for some reason?? */
		   )
		{   /* if this joining equivalence class is an implicit TID
		    ** then save it, and then ignore other joining equivalence
		    ** classes since they will be equal anyway */

		    tideqc = eqcls;		    /* tideqc will be set in leqr
						** below after unnecessary
						** equivalence classes are 
						** removed */
		    if ((   (ojjointype != OPL_RIGHTJOIN)
			    ||
			    left_imtid
			)
			&&
			(   (ojjointype != OPL_LEFTJOIN)
			    ||
			    right_imtid
			))
			BTclear((i4)tideqc, (char *)leqr); /* do not ask for
						** implicit TID, except in special
						** right tid FSM join case need for
						** check option, otherwise no query
						** plan found error will be 
						** reported */
		}
		else
		    joining_eqcls = eqcls;	    /* save the joining equivalence
						** class in case there only is
						** one */
	    }
	}

    
	if (tideqc >= 0)
	{				    /* if explicit-implicit TID
					    ** then we do not want to ask
					    ** for associated eqclasses
					    ** which we know must be equal
                                            ** anyhow
                                            ** - ask for equivalence classes
                                            ** only if the ordering provided
                                            ** by the index, or potential sort
                                            ** node is possibly useful */
	    OPV_BMVARS           (*rmp);    /* ptr to bit map of relations
                                            ** one of which contains the
                                            ** implicit TID in the joining 
                                            ** equivalence class */
            OPV_IVARS            var;       /* joinop range variable number
                                            ** being analyzed */
            OPV_RT               *vbase;    /* ptr to base of array of ptrs
                                            ** to joinop range variables */
	    OPE_BMEQCLS		 (*useless); /* PTR to a bit map of eqcs
                                            ** which are useless for joining
                                            ** since they belong to indexes 
                                            ** on the base relation of this
                                            ** TID join */
            OPV_IVARS            keyed_var; /* variable number of base relation */
            useless = NULL;
	    if (left_imtid)		    /* point to map with the explicit
                                            ** TID */
	    {
		rmp = &rp->opn_rlmap;	    /* right subtree contains explicit
                                            ** TID */
		*lkeyjoin = tideqc;
		*rkeyjoin = OPE_NOEQCLS;
                keyed_var = lp->opn_pra[0];
	    }
	    else
	    {
		rmp = &lp->opn_rlmap;	    /* left subtree contains explicit
                                            ** TID */
		*lkeyjoin = OPE_NOEQCLS;
		*rkeyjoin = tideqc;
                keyed_var = rp->opn_pra[0];
	    }
            vbase = subquery->ops_vars.opv_base; /* base of array of ptrs to
                                            ** joinop range variables */
	    for (var = -1; 
		 (var = BTnext((i4)var, (char *)rmp, (i4)maxvar))>= 0;)
	    {
                OPV_VARS         *varp;	    /* ptr to range variable being
                                            ** analyzed */
		varp = vbase->opv_rt[var];
		/* if "var" is an index and it indexes the join eqclass, 
                ** then do not request the indexed attribute cause the TIDs 
                ** being equal will imply that the associated (indexed) 
                ** attributes are also equal 
                */
                if ( (keyed_var == varp->opv_index.opv_poffset)
                     &&
                     (vbase->opv_rt[keyed_var]->
                            opv_primary.opv_tid == tideqc))
		    /* note that there may be more than one index that 
		    ** this will hold true ... and there may be overlap
                    ** between multi-attribute indexes so test and reset
                    ** each bit, except for the first index found */
		{
                    /* Changed from 4.0 - this loop was incomplete for 
                    ** multi-attribute
                    ** keys since there could be useless joins between the
                    ** second, third etc. indexed attribute */
		    if (useless)
		    {	/* first index has already been defined so OR in
			** eqcs of next index */
			BTor((i4)maxeqcls, 
			    (char *)&varp->opv_maps.opo_eqcmap, 
			    (char *)useless);
		    }
		    else
		    {	/* this is the first index so init the bitmap */
			useless = reqr;
			MEcopy( (PTR)&varp->opv_maps.opo_eqcmap, 
			    sizeof(OPE_BMEQCLS),
			    (PTR)useless);  /* copy the equivalence classes
					    ** from the first index */
		    }
		}
	    }
	    if (useless)
	    {	/* at least one index was found, so reset all the joining
		** equivalence classes */
		BTnot( (i4)maxeqcls, (char *)useless); /* select the equivalence
                                            ** classes which still provide some
					    ** restriction */
/* Do not reset equivalence classes since they may provide orderings useful
** for joins or top sort node removal.  oph_jselect should not include
** equivalence classes in the join node as part of the restrictivity.  However,
** there is some added costs in requiring the redundant equivalence classes be
** requested in sort node costs, and temp file creation.  It would be desirable
** to restrict the attributes being brought up if possible... one solution would
** be to make try to enumerate the possibilities of bringing up or not bringing
** up all sets of equivalence classes, but this would require too much memory
** ... another solution would be to have 2 sets of required equivalence classes
** one would be a minimum set and the other would be a maximum set, and any
** naturally occuring ordering would use the maximum set and minimum set, but
** this would require that OPN_EQS lists be processed differently in opn_calcost
** which would require a lot of work... another solution would be to assume
** worse case and bring up all attributes, and when a new best plan is chosen
** make a traversal and remove unnecessary attributes, and recalculate sort node
** costs, 2 best costs would be needed, one would be used for the case of all
** attributes being brought up, and the other would be used for the pruned query
** plan.  the higher value is used for eliminating search space, but the lower
** cost is used for considering a new best plan.
**
** the following piece of code previously had useless and eqr interchanged
*/
                BTand( (i4)maxeqcls, (char *)leqr, (char *)useless); /* remove
                                            ** equivalence classes which do
                                            ** not provide restriction */
#if 0
                *jxcnt = BTcount((char *)useless, (i4)maxeqcls); /* number of
                                            ** equivalence classes which still
                                            ** restrict the relation */
#endif
                *jxcnt = OPN_NJEQCLS;       /* - init to no joining equivalence
                                            ** equivalence classes found yet
                                            ** - the first one is the joining
                                            ** eqclass, the rest are only
                                            ** joining predicate */
                if (BTnext((i4)-1, (char *)useless, (i4)maxeqcls) >=0)
                {   /* at least one extra eqcls exists for this tid join
                    ** so create jxordering */
                    BTset((i4)tideqc, (char *)useless);
                    *jxordering = opo_fordering(subquery, useless);
                }
	    }
	}
	else
	{   /* determine the keyed join orderings */
	    if (lvarp)			    /* TRUE if left subtree is a leaf */
		*lkeyjoin = opo_kordering(subquery, lvarp, leqr); /* NOTE the
					    ** "leqr" contains the equivalence
					    ** classes in common from the left
					    ** and right subtrees, reqr is NOT
					    ** initialized
					    ** - find the keying possibilities
                                            ** given the common attributes */
	    else
		*lkeyjoin = OPE_NOEQCLS;
	    if (rvarp)			    /* TRUE if right subtree is a leaf*/
		*rkeyjoin = opo_kordering(subquery, rvarp, leqr); /*
					    ** find the keying possibilities
                                            ** given the common attributes */
	    else
		*rkeyjoin = OPE_NOEQCLS;
	}
    
	MEcopy((PTR)leqr, sizeof(*reqr), (PTR)reqr ); /* request same
                                            ** equivalence class from right
                                            ** child subtree */
    
	/* select the ordering of the sort nodes which could be useful for 
	** evaluating this join */

	if (lvarp 
	    && 
	    lvarp->opv_subselect 
	    && 
	    lvarp->opv_subselect->opv_order >=0
	    )
	{   /* the ordering of equivalence classes has been chosen for this
	    ** virtual node, by earlier analysis of the correlated variables
            ** and keys for the SEJOIN */
	    *jordering = lvarp->opv_subselect->opv_order;
	}
	else
	if (rvarp 
	    && 
	    rvarp->opv_subselect 
	    && 
	    rvarp->opv_subselect->opv_order >=0
	    )
	{   /* the ordering of equivalence classes has been chosen for this
	    ** virtual node, by earlier analysis of the correlated variables
            ** and keys for the SEJOIN */
	    *jordering = rvarp->opv_subselect->opv_order;
	}
	else if (left_imtid || right_imtid)
        {   /* if this is a TID join then ordering
            ** on any other equivalence classes is not useful, so simply return
	    ** the TID equivalence class, as the required ordering */
	    *jordering = tideqc;	    /* explicit/implicit TID join
					    ** has priority */
	    if (left_imtid)
		BTset((i4)tideqc, (char *)reqr); /* request explicit TID if
					    ** right side is implicit TID */
	    if (right_imtid)
		BTset((i4)tideqc, (char *)leqr); /* request explicit TID if
					    ** left side is implicit TID */
        }
	else
	{
	    if ((*jxcnt) >0)
	    {	/* if there is more than one possible joining equivalence
		** classes then request a multi-attribute sort be performed
		** for any sort node which is a child of this node 
                ** FIXME - for subselects the key-expr should be sorted
                ** also since this can affect reevaluation of the subselect
                ** as well as correlated values, OR just sort the eqcs in the
                ** key expression, AFTER the correlated values, since a hold
                ** file is used for the results per set of correlated values */
		*jordering = opo_fordering(subquery, leqr); /* find or create
					    ** a multi-attribute ordering
					    ** associated with this set of
					    ** joining attributes */
	    }
	    else
		*jordering = joining_eqcls;  /* only one joining equivalence
					    ** class so order on this */
	}
    }
    if (subquery->ops_oj.opl_base && (np->opn_ojid != OPL_NOOUTER))
    {	/* mark outer join equivalence classes in the set required
	** for this outer join, these special equivalence classes
	** are used for selecting the "NULL" attribute and for
	** boolean factor positioning */
	OPE_BMEQCLS	temp;
	OPL_OUTER	*outerp;
	outerp = subquery->ops_oj.opl_base->opl_ojt[np->opn_ojid];
	MEcopy((PTR)outerp->opl_innereqc, sizeof(temp), (PTR)&temp);
	BTand((i4)maxeqcls, (char *)&lp->opn_eqm, (char *)&temp);
	BTor((i4)maxeqcls, (char *)&temp, (char *)leqr);
	MEcopy((PTR)outerp->opl_innereqc, sizeof(temp), (PTR)&temp);
	BTand((i4)maxeqcls, (char *)&rp->opn_eqm, (char *)&temp);
	BTor((i4)maxeqcls, (char *)&temp, (char *)reqr);
    }

    opb_gbfbm(subquery, &np->opn_eqm, &lp->opn_eqm, &rp->opn_eqm, 
	&lp->opn_ojevalmap, &rp->opn_ojevalmap, &np->opn_ojevalmap, maxeqcls, 
	lvarp, rvarp, jxbfm, jeqh, jxeqh, leqr, reqr, &lp->opn_rlmap, &rp->opn_rlmap,
	&np->opn_rlmap, np->opn_ojid, &np->opn_ojinnermap, NULL);
					    /* get the boolean factor bitmap
					    ** to be evaluated at this node */

    /* Get BY-list eqc's available from left or right, if any */
    opn_bylist_map(subquery, byeqcmap, &lp->opn_eqm, &rp->opn_eqm);

    MEfill(sizeof(faeqcmap), (u_char)0, (PTR)&faeqcmap);
    if (subquery->ops_funcs.opz_fmask & OPZ_MAFOUND)
    {	/* Could be a multi-variable function attribute */
	OPZ_IFATTS            fattr; /* function attribute currently
				    ** being analyzed */
	OPZ_IFATTS            maxfattr; /* maximum function attribute
				    ** defined */
	OPZ_AT                *abase; /* ptr to base of an array
				    ** of ptrs to attributes */
	OPZ_FT                *fbase; /* ptr to base of an array
				    ** of ptrs to function attributes*/

	maxfattr = subquery->ops_funcs.opz_fv; /* maximum number of
				    ** function attributes defined */
	fbase = subquery->ops_funcs.opz_fbase; /* ptr to base of array
				    ** of ptrs to function attribute
				    ** elements */
	abase = subquery->ops_attrs.opz_base; /* ptr to base of array
				    ** of ptrs to joinop attributes
				    */
	/*
	** If this equiv class is a function and the function
	** is not in the left or right side (as shown above)
	** and the function can be computed here, get all the
	** attributes in the function
	*/
	for (fattr = 0; fattr < maxfattr; fattr++)
	{
	    OPZ_FATTS		*fattrp; /* ptr to current function
				    ** attribute being analyzed */
	    OPE_IEQCLS          feqc; /* equivalence class containing
				    ** the function attribute */

	    fattrp = fbase->opz_fatts[fattr]; /* get ptr to current
				    ** function attr being analyzed*/
	    if (!(fattrp->opz_type == OPZ_MVAR || 
		fattrp->opz_type == OPZ_SVAR && 
		fattrp->opz_mask & OPZ_OJSVAR))
		continue;	    /* only multi-var function 
				    ** attributes will be calculated
				    ** at an interior node, or single-vars
				    ** ref'ing inner table of oj, so only
				    ** consider these */
	    feqc = abase->opz_attnums[fattrp->opz_attno]->opz_equcls;/*
				    ** get equivalence class containing
				    ** function attribute */
	    if (fattrp->opz_mask & OPZ_OJFA)
	    {	/* special eqcls was set in opn_jmaps */

		if (BTtest((i4)feqc, (char *)eqr)
		    &&
		    !BTtest((i4)feqc, (char *)&lp->opn_eqm)
		    &&
		    !BTtest((i4)feqc, (char *)&rp->opn_eqm))
		{
		    if (!BTtest((i4)feqc, (char *)&np->opn_eqm))
			opx_error(E_OP0287_OJVARIABLE);
		    BTset((i4)feqc, (char *)&faeqcmap); /* special eqcls
				    ** evaluated at this node */
		}
		continue;
	    }
            if (BTsubset ((char *)&fattrp->opz_eqcm, (char *)&np->opn_eqm,
                    (i4) maxeqcls))
            {   /* function attribute can be computed in this subtree */
		bool	    left_sub;
		bool	    right_sub;

                left_sub = BTsubset ((char *)&fattrp->opz_eqcm,
                              (char *)&lp->opn_eqm,
                              (i4) maxeqcls);
                right_sub = BTsubset ((char *)&fattrp->opz_eqcm,
                              (char *)&rp->opn_eqm,
                              (i4) maxeqcls);
                if ((!right_sub || rp->opn_nleaves == 1) &&
                        (!left_sub || lp->opn_nleaves == 1) ||
			fattrp->opz_type == OPZ_SVAR)
		{   /* found a function attribute in this equivalence
		    ** class and the function has all the required 
		    ** equivalence classes at this node, moreover, the
		    ** function attribute cannot be computed any lower
		    ** in the tree
		    ** - get the parts of the function from either the
		    ** left or right subtree 
		    */
		    OPE_BMEQCLS          (*feqcm); /* ptr to equivalence
				    ** class map required for function
				    ** evaluation */
		    OPE_BMEQCLS         required; /* temp bitmap used
				    ** to determine which attributes to
				    ** bring up for calculation of the
				    ** multi-var function attribute */
                    bool        found;
                    found = FALSE;
                    if (BTtest((i4)feqc, (char *)&lp->opn_eqm))
                    {
                        BTset((i4)feqc, (char *)leqr); /* ask for this equivalence class
                                                    ** if it is available in the left
                                                    ** subtree */
                        found = TRUE;
                    }
                    if (BTtest((i4)feqc, (char *)&rp->opn_eqm))
                    {
                        BTset((i4)feqc, (char *)reqr); /* ask for this equivalence class
                                                    ** if it is available in the right
                                                    ** subtree */
                        found = TRUE;
                    }
                    if (found)      /* and the equivalence class was found
				    ** in the current node, this would occur
				    ** in a query like b6966,
				    ** b1.n = b2.n + b3.n which is a
				    ** cart prod */
			(*jxcnt)++; /* count this join,
				    ** it will not occur due to poor
				    ** placement of function attributes
				    ** used to calculate it */
		    feqcm = &fattrp->opz_eqcm; /* save ptr to function
				    ** attribute map */
		    MEcopy( (PTR)feqcm, sizeof(OPE_BMEQCLS), (PTR)&required);
		    BTand( (i4)maxeqcls,
			(char *)&lp->opn_eqm,
			(char *)&required); /* map of equivalence
				    ** equivalence classes required from
				    ** left side */
		    BTor( (i4)maxeqcls,
			(char *)&required,
			(char *)leqr); /* ask for this equivalence
				    ** classes from left side */
		    BTnot( (i4)maxeqcls,
			(char *)&required); /* get mask of remaining
				    ** equivalence classes required from
				    ** right side for this function attr
				    */
		    BTand( (i4)maxeqcls,
			(char *)feqcm,
			(char *)&required);
		    BTor( (i4)maxeqcls,
			(char *)&required,
			(char *)reqr); /* ask for equivalence
				    ** classes from right side */
		    if (!BTsubset((char *)&required, (char *)&rp->opn_eqm, 
			(i4)maxeqcls))
		    {	/* possibly some special equivalence classes
			** which are created at this join node, and are
			** used by this function attribute */
			if (subquery->ops_oj.opl_base)
			{
			    OPE_IEQCLS	    sp_eqcls;	/* special eqcls being
				    ** analyzed */
			    BTor((i4)maxeqcls, (char *)&rp->opn_eqm,
				(char *)&required); 
			    BTxor((i4)maxeqcls, (char *)&rp->opn_eqm,
				(char *)&required); /* remove all eqcls which are
				    ** available at this node */
			    for (sp_eqcls = -1; (sp_eqcls = BTnext((i4)sp_eqcls, 
				(char *)&required, (i4)maxeqcls))>=0;)
			    {
				if (BTtest((i4)sp_eqcls, (char *)&np->opn_eqm))
				    BTclear((i4)sp_eqcls, (char *)reqr); /* eqc
					** is created at this node and is not
					** available at the children */
				else
				    opx_error(E_OP04B6_SPECIALOUTERJOIN); 
					/* unavailable eqc used */
			    }
			}
			else
			    opx_error(E_OP04B6_SPECIALOUTERJOIN);	
					/* unavailable eqc used */
		    }
                    BTset((i4)feqc, (char *)&faeqcmap);
		}
	    }
	}
    }
    {
        /* Get the equiv classes that are not used here but
        ** are just passed up to a later node.
        ** Above, we handled possible joins for this node
        ** and boolfacts for this and later nodes.
        ** However, we still need those equiv classes used
        ** for later joins.
        */
        OPE_IEQCLS      reqcls;             /* equivalence class required by
                                            ** above node */
        for (reqcls = -1; (reqcls = BTnext((i4)reqcls,
                                           (char *)eqr,
                                           (i4)maxeqcls)
                         ) >= 0;)
        {                                   /* for each eqclass required
                                            ** above, pass this requirement
                                            ** to the appropriate subtree */
            if (BTtest((i4)reqcls, (char *)&lp->opn_eqm))
            {
                BTset((i4)reqcls, (char *)leqr); /* ask for this equivalence class
                                            ** if it is available in the left
                                            ** subtree */
            }
            else if (BTtest((i4)reqcls, (char *)&rp->opn_eqm))
            {
                BTset(reqcls, (char *)reqr); /* ask for this equivalence class
                                            ** if it is available in the right
                                            ** subtree */
            }
            else if (!BTtest((i4)reqcls, (char *)&faeqcmap))
                opx_error(E_OP0487_NOEQCLS); /* this should have been created as
                                            ** a multi-variable function attribute
                                            ** earlier */
        }
    }
}
