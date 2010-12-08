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
#define        OPN_JMAPS      TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>


/**
**
**  Name: OPNJMAPS.C - set opn_eqm, opn_rlasg and opn_rlmap
**
**  Description:
**      initialize various maps in the operator tree
**
**  History:    
**      11-jun-86 (seputis)    
**          initial creation from setjmaps.c
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
bool opn_jmaps(
	OPS_SUBQUERY *subquery,
	OPN_JTREE *nodep,
	OPL_BMOJ *ojmap);

/*{
** Name: opn_jmaps	- set various join operator tree maps
**
** Description:
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      nodep                           ptr to current operator node which
**                                      will have maps initialized
**	ojmap				NULL if no outer joins exist
**					- map of outer joins which are
**					evaluated in parent nodes.
**
** Outputs:
**      nodep->opn_eqm                  set of equivalence class available
**                                      from subtree
**      nodep->opn_rlmap                bitmap of relations in the subtree
**      nodep->opn_rlasg                order in which the relations in
**                                      opn_rlmap are assigned to the leaves
**	Returns:
**	    TRUE if there is a valid placement of subselect nodes with
**          all required equivalence classes available for execution of the
**          boolean factor used for the subselect
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	11-jun-86 (seputis)
**          initial creation from setjmaps
**      29-mar-90 (seputis)
**          fix byte alignment problems
**	2-apr-91 (seputis)
**	    only partially copied relation assignment causing run_all diffs
**	    fix for b35461, this would cause the OPF cache of query plans
**	    to not be effectively used, and could cause performance problems
**	    on non-VMS systems.  This fix should be used whenever a poor
**	    query plan bug is reported which cannot be reproduced on VMS.
**      15-feb-94 (ed)
**          - bug 59598 - correct mechanism in which boolean factors are
**          evaluated at several join nodes
**      11-apr-94 (ed)
**          - bug 59937 - E_OP0489 consistency check due to inner join ojid
**          not being visible in boolean factor placement maps
**	31-jul-97 (inkdo01)
**	    Fix to force SVAR func atts ref'ed in ON clauses to materialize
**	    just before needed (to incorporate proper null semantics).
**	 9-jul-99 (hayke02 for inkdo01)
**	    Set opn_eqm bits in lower level nodes (e.g. leaf nodes) when
**	    func attrs are found in higher level OJ nodes. This change
**	    fixes bug 92749.
**	20-Mar-02 (wanfr01)
**	    Bug 106678, INGSRV 1633
**	    Confirm function attribute as an ojid before associating it
**	    with an outer join node.
**	16-sep-03 (hayke02)
**	    Check for OPN_OJINNERIDX in nodep->opn_jmask to indicate that
**	    placement of a coverqual inner index outer join needs to be
**	    checked.
**	28-apr-04 (hayke02)
**	    Modify previous change so that we return FALSE and reject the QEP
**	    if the index outer join node (node->LEFT) has a non-zero opn_nchild
**	    for node->LEFT->RIGHT. This will allow only QEPs where the OJ
**	    inner is the index only and not the index joined to another
**	    relation. This change fixes problem INGSRV 2808, bug 112211.
**      05-Oct-2004 (huazh01)
**          Remove the above fix. The fix for 111627 handles b106678
**          as well. This fixes b113160, INGSRV2984. 
**	14-mar-05 (hayke02)
**	    Modify the fix for problem INGSRV 2808, bug 112211 so that we now
**	    check for more than 1 var in the opl_ivmap. This now allows the
**	    correct rejection of plans that have had their opn_child's set up
**	    with left joins 'reversed' into right joins. This change fixes
**	    problems INGSRV 3049 and 3094, bugs 113457 and 113990.
**	23-sep-05 (hayke02)
**	    Check for a WHERE clause (OPL_NOOOUTER opz_ojid) OJSVAR func att,
**	    and make sure that all OJs that this func att is inner to are
**	    executed before the join involving this func att. This change fixes
**	    bug 114912.
**	30-jan-07 (hayke02)
**	    Disable the fix for bug 114912 for cart prod (OPL_BOJCARTPROD) OJs.
**	    This change fixes bug 117513.
**	27-Oct-2009 (kiria01) SIR 121883
**	    Scalar sub-selects - protect subp->opv_eqcrequired from bad de-ref.
*/
bool
opn_jmaps(
	OPS_SUBQUERY       *subquery,
	OPN_JTREE          *nodep,
	OPL_BMOJ	   *ojmap)
{
	OPL_OUTER	    *outerp;

    if (nodep->opn_nleaves == 1)
    {	/* leaf node */
	OPV_IVARS              varno;		/* joinop range variable number
                                                ** of leaf */

	varno = nodep->opn_prb[0];		/* by definition of leaf - only
                                                ** one variable in partition */
	nodep->opn_rlasg[0] = varno;            /* trivial ordering for leaf */
	MEfill( sizeof(nodep->opn_rlmap), (u_char)0, (PTR)&nodep->opn_rlmap );
	if (subquery->ops_oj.opl_lv > 0)
	{
	    MEfill( sizeof(nodep->opn_ojinnermap),
		(u_char)0, (PTR)&nodep->opn_ojinnermap);
	    MEfill( sizeof(nodep->opn_ojevalmap),
		(u_char)0, (PTR)&nodep->opn_ojevalmap);
	    opl_sjij(subquery, varno, &nodep->opn_ojinnermap, ojmap);
	}
	BTset((i4)varno, (char *)&nodep->opn_rlmap); /* only one bit set for leaf*/
	MEcopy ((PTR)&subquery->ops_vars.opv_base->opv_rt[varno]->opv_maps.opo_eqcmap,
	    sizeof(nodep->opn_eqm), (PTR)&nodep->opn_eqm ); /* copy map of
                                                ** equivalence classes 
						** associated with varno of this
                                                ** leaf */

	return(TRUE);
    }
    else
    {	/* non-leaf node */
	OPN_JTREE              *leftchildp;	/* ptr to left child node */
	OPN_JTREE              *rightchildp;	/* ptr to right child node */

	leftchildp = nodep->opn_child[OPN_LEFT];
	rightchildp = nodep->opn_child[OPN_RIGHT];
	if (ojmap)
	{
	    MEfill(sizeof(nodep->opn_ojevalmap), (u_char)0,
		(PTR)&nodep->opn_ojevalmap);
	    if( (nodep->opn_ojid >= 0)
		&&
		!BTtest((i4)nodep->opn_ojid, (char *)ojmap))
	    {   /* setup the outer join map which contains all outer joins
		** which are completely evaluated within this subtree */
		BTset((i4)nodep->opn_ojid, (char *)&nodep->opn_ojevalmap);
		BTset((i4)nodep->opn_ojid, (char *)ojmap);
	    }
	}
	if (!opn_jmaps (subquery, leftchildp, ojmap))	/* get info on left child */
	    return(FALSE);
	if (!opn_jmaps (subquery, rightchildp, ojmap))	/* get info on right child */
	    return(FALSE);

	MEcopy ((PTR)&leftchildp->opn_rlmap,
		sizeof(nodep->opn_rlmap),
		(PTR)&nodep->opn_rlmap );	/* get var bitmap from left
                                                ** child */
	BTor(	(i4)BITS_IN(nodep->opn_rlmap),
		(char *)&rightchildp->opn_rlmap,
		(char *)&nodep->opn_rlmap );	/* "OR" rightchildp->opn_rlmap
						** into nodep->opn_rlmap */
	if (ojmap)
	{   /* setup ojinnermap which contains all outer joins which are
	    ** partially or totally evaluated within this subtree, but 
	    ** not at this node unless it is in the subtree */
	    MEcopy((PTR)&leftchildp->opn_ojinnermap,
		sizeof(leftchildp->opn_ojinnermap),
		(PTR)&nodep->opn_ojinnermap);
	    BTor((i4)BITS_IN(rightchildp->opn_ojinnermap),
                (char *)&rightchildp->opn_ojinnermap,
                (char *)&nodep->opn_ojinnermap);
	    if (leftchildp->opn_ojid >= 0)
		BTset((i4)leftchildp->opn_ojid, (char *)&nodep->opn_ojinnermap);
	    if (rightchildp->opn_ojid >= 0)
		BTset((i4)rightchildp->opn_ojid, (char *)&nodep->opn_ojinnermap);
	    if (subquery->ops_mask & OPS_IJCHECK)
		opl_ijcheck(subquery, &leftchildp->opn_ojinnermap,
		    &rightchildp->opn_ojinnermap, &nodep->opn_ojinnermap,
		    &leftchildp->opn_ojevalmap, &rightchildp->opn_ojevalmap,
		    &nodep->opn_rlmap);
	    /* map of outerjoins which are entirely evaluated within
	    ** this subtree */
	    BTor((i4)BITS_IN(leftchildp->opn_ojevalmap),
                (char *)&leftchildp->opn_ojevalmap,
                (char *)&nodep->opn_ojevalmap);
	    BTor((i4)BITS_IN(rightchildp->opn_ojevalmap),
                (char *)&rightchildp->opn_ojevalmap,
                (char *)&nodep->opn_ojevalmap);
	    if ((nodep->opn_jmask & OPN_OJINNERIDX) &&
		((nodep->opn_ojid != nodep->opn_child[OPN_LEFT]->opn_ojid) ||
		((nodep->opn_ojid == nodep->opn_child[OPN_LEFT]->opn_ojid) &&
		(nodep->opn_ojid >= 0) &&
		(BTcount((char *)subquery->ops_oj.opl_base->opl_ojt
		[nodep->opn_ojid]->opl_ivmap, subquery->ops_vars.opv_rv) > 1))))
		return(FALSE);
	}
	MEcopy ((PTR)leftchildp->opn_rlasg,
		leftchildp->opn_nleaves * sizeof(nodep->opn_rlasg[0]),
		(PTR)nodep->opn_rlasg);		/* get relations from left
                                                ** child */
	MEcopy ((PTR)rightchildp->opn_rlasg,
		rightchildp->opn_nleaves * sizeof(nodep->opn_rlasg[0]),
		(PTR)(nodep->opn_rlasg + leftchildp->opn_nleaves)); /* get
                                                ** relations from right child
                                                ** and place them beside the
                                                ** ones from the left child */

	MEcopy ((PTR)&leftchildp->opn_eqm,
		sizeof(nodep->opn_eqm),
		(PTR)&nodep->opn_eqm );		/* copy equivalence class map
                                                ** from left child to this node 
                                                */
	BTor(	(i4)BITS_IN(nodep->opn_eqm),
		(char *)&rightchildp->opn_eqm,
		(char *)&nodep->opn_eqm );	/* "OR" right child equivalence
                                                ** map into this to produce 
                                                ** map for this node */
	if (subquery->ops_joinop.opj_virtual)
	{   /* there is a subselect in this query so check for availability
            ** of equivalence classes used for evaluation of boolean factors
	    ** containing the subselect
            */
	    OPV_SUBSELECT	*subp;		/* ptr to subselect descriptor
                                                ** for range variable */
	    if  (   (rightchildp->opn_nleaves == 1)
		    &&
		    (
			subp = subquery->ops_vars.opv_base->
			    opv_rt[rightchildp->opn_prb[0]]->opv_subselect
		    )
		)
	    {	/* right child is a subselect so test for correct leaf
                ** placement */
		if (!subp->opv_eqcrequired ||
			!BTsubset(	(char *)subp->opv_eqcrequired,
				(char *)&nodep->opn_eqm,
				(i4)BITS_IN(OPE_BMEQCLS)
			    )
		    )
		    return (FALSE);		/* cannot evaluate all boolean
                                                ** factors with SEJOIN nodes
                                                ** in this configuration */
		else
		{   /* check if all corelated variables are available in the
		    ** outer - FIXME create another pointer field which
                    ** contains this information so this loop is avoided */
		    for (;subp; subp = subp->opv_nsubselect)
		    {
			if (!BTsubset(	(char *)&subp->opv_eqcmp,
					(char *)&leftchildp->opn_eqm,
					(i4)BITS_IN(OPE_BMEQCLS)
				    )
			    )
			    return (FALSE);	/* not all correlated
                                                ** equivalence classes
						** are available from the outer
						** so return */
		    }
		}
	    }
	    if  (   (leftchildp->opn_nleaves == 1)
		    &&
		    (subp = subquery->ops_vars.opv_base->
			opv_rt[leftchildp->opn_prb[0]]->opv_subselect)
		)
	    {
		if (!subp->opv_eqcrequired ||
			!BTsubset(	(char *)subp->opv_eqcrequired,
				(char *)&nodep->opn_eqm,
				(i4)BITS_IN(OPE_BMEQCLS)
			    )
		    )
		    return (FALSE);		/* cannot evaluate all boolean
                                                ** factors with SEJOIN nodes
                                                ** in this configuration */
		else
		{   /* check if all corelated variables are available in the
		    ** outer - FIXME create another pointer field which
                    ** contains this information so this loop is avoided */
		    for (;subp; subp = subp->opv_nsubselect)
		    {
			if (!BTsubset(	(char *)&subp->opv_eqcmp,
					(char *)&rightchildp->opn_eqm,
					(i4)BITS_IN(OPE_BMEQCLS)
				    )
			    )
			    return (FALSE);	/* not all correlated
                                                ** equivalence classes
						** are available from the outer
						** so return */
		    }
		}
	    }
	}
	if ((subquery->ops_oj.opl_lv > 0)
	    &&
	    (nodep->opn_ojid != OPL_NOOUTER))
	{
	    OPV_IVARS	    maxvar;

	    /* this is an outer join function attribute which should
	    ** only appear at the point that the outer join is
	    ** actually performed */
	    maxvar = subquery->ops_vars.opv_rv;
	    outerp = subquery->ops_oj.opl_base->opl_ojt[nodep->opn_ojid];
	    if ((outerp->opl_type == OPL_LEFTJOIN)
		||
		(outerp->opl_type == OPL_FULLJOIN))
	    {
		OPV_BMVARS	tempvmap;
		OPV_IVARS	innervar;

		MEcopy((PTR)outerp->opl_maxojmap, sizeof(tempvmap),
		    (PTR)&tempvmap);
		BTand((i4)BITS_IN(tempvmap), (char *)&nodep->opn_rlmap,
		    (char *)&tempvmap);
		for (innervar = -1; (innervar = BTnext((i4)innervar,
		    (char *)&tempvmap, (i4)maxvar))>=0;)
		{
		    OPE_IEQCLS	    ojeqcls;
		    ojeqcls = subquery->ops_vars.opv_base->opv_rt
			[innervar]->opv_ojeqc;
		    if (ojeqcls != OPE_NOEQCLS)
			BTset((i4)ojeqcls, (char *)&nodep->opn_eqm);
		}
	    }
	}
	{   /* determine multi-variable functions that can first be calculated
            ** at this node 
	    */
	    OPZ_IFATTS         fattr;		/* current function attribute
                                                ** being analyzed */
	    OPZ_IFATTS	       maxfattr;        /* maximum number of function
                                                ** attributes defined */
	    OPZ_FT             *fbase;          /* ptr to base of array of ptrs
                                                ** to function attribute
                                                ** elements */
	    OPZ_AT	       *abase;          /* ptr to base of array of ptrs
                                                ** to joinop attribute elements
                                                */
	    maxfattr = subquery->ops_funcs.opz_fv; /* number of function
                                                ** attributes defined */
	    fbase = subquery->ops_funcs.opz_fbase; /* ptr to base of array
                                                ** of function attributes */
	    abase = subquery->ops_attrs.opz_base; /* ptr to base of array
						** of ptrs to joinop attribute
                                                ** elements */
	    for (fattr = 0; fattr < maxfattr ; fattr++)
	    {
		OPZ_FATTS             *fattrp;	/* ptr to current function
                                                ** attribute being analyzed */
		OPE_IEQCLS            eqcls;    /* equivalence class of the
                                                ** multi-variable function
                                                ** attribute */

		fattrp = fbase->opz_fatts[fattr];
		eqcls = abase->opz_attnums[fattrp->opz_attno]->opz_equcls; /*
						** equivalence class associated
                                                ** with the multi-variable
                                                ** function attribute */
		/* check for WHERE clause (OPL_NOOUTER opz_ojid) OJSVAR func att
		** and then make sure that all OJs that this func att is inner
		** to are executed before the join involving this func att
		*/
		if (fattrp->opz_type == OPZ_SVAR
		    &&
		    (fattrp->opz_mask & OPZ_OJSVAR)
		    &&
		    nodep->opn_ojid != OPL_NOOUTER
		    &&
		    fattrp->opz_ojid == OPL_NOOUTER
		    &&
		    (fattrp->opz_ijmap
		    &&
		    BTtest((i4)nodep->opn_ojid, (char *)fattrp->opz_ijmap)
		    &&
		    !(outerp->opl_mask & OPL_BOJCARTPROD)))
		{
		    OPZ_IATTS		attno;
		    bool		allinrlmap = TRUE;
		    OPE_EQCLIST		*eqclsp;

		    eqclsp = subquery->ops_eclass.ope_base->ope_eqclist[eqcls];
		    for (attno = -1;  (attno = BTnext((i4)attno,
			(PTR)&eqclsp->ope_attrmap,
			(i4)subquery->ops_attrs.opz_av)) != -1; )
		    {
			if (!BTtest((i4)abase->opz_attnums[attno]->opz_varnm,
						    (char *)&nodep->opn_rlmap))
			{
			    allinrlmap = FALSE;
			    break;
			}
		    }
		    if (allinrlmap)
			return(FALSE);
		}
		if ((fattrp->opz_type != OPZ_MVAR)
		    && !(fattrp->opz_type == OPZ_SVAR &&
			 fattrp->opz_mask & OPZ_OJSVAR &&
			 fattrp->opz_ojid == nodep->opn_ojid)
		    ||
		    (fattrp->opz_mask & OPZ_OJFA))
		    continue;			/* only multi-variable 
						** functions are assigned here 
						** (and SVARs in ON clauses
						** of OJs eval'd at this node)
						** since others where assigned
                                                ** earlier, ... also outer join
						** special eqc were assigned 
						** prior to this loop */
		if (fattrp->opz_type == OPZ_SVAR)
		{
		    /* Must be ON clause ref'ed SVAR. Set bit in whichever
		    ** child node covers the SVAR eqcmap. */
		    if (BTsubset((char *)&fattrp->opz_eqcm,
			(char *)&nodep->opn_child[1]->opn_eqm,
			(i4)BITS_IN(nodep->opn_eqm)))
		     BTset((i4)eqcls, (char *)&nodep->opn_child[1]->opn_eqm);
		    else if (BTsubset((char *)&fattrp->opz_eqcm,
			(char *)&nodep->opn_child[0]->opn_eqm,
			(i4)BITS_IN(nodep->opn_eqm)))
		     BTset((i4)eqcls, (char *)&nodep->opn_child[0]->opn_eqm);
						/* set fattr eqcls bit in 
						** proper child opn_eqm */
		    continue;
		}
		if (BTtest( (i4)eqcls, (char *)&nodep->opn_eqm))
			continue;		/* if function attribute has
                                                ** been added then
                                                ** continue */
		if (BTsubset((char *)&fattrp->opz_eqcm,
			     (char *)&nodep->opn_eqm,
			     (i4)BITS_IN(nodep->opn_eqm))
		    )
		    BTset((i4)eqcls, (char *)&nodep->opn_eqm); /* set bit if all 
                                                ** the required equivalence 
                                                ** classes are available 
                                                ** for the function attribute
                                                */
	    }
	}
    }
    return(TRUE);
}
