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
#define        OPN_NODECOST      TRUE
#include    <bt.h>
#include    <me.h>
#include    <ex.h>
#include    <opxlint.h>


/**
**
**  Name: OPNNCOST.C - routines to evaluate the minimum cost of a node
**
**  Description:
**	Contains the main entry point for the cost evaluation of a tree.
**
**  History:    
**      21-may-86 (seputis)    
**          initial creation from nodecost.c
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      11-aug-93 (ed)
**	    added missing includes
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
static i4 opn_tprocord_validate(
	OPS_SUBQUERY *subquery,
	OPN_SUBTREE *osbtp,
	OPN_SUBTREE *isbtp,
	OPN_LEAVES oleaves,
	OPN_LEAVES ileaves);
bool opn_nodecost(
	OPS_SUBQUERY *subquery,
	OPN_JTREE *np,
	OPE_BMEQCLS *eqr,
	OPN_SUBTREE **subtp,
	OPN_EQS **eqclp,
	OPN_RLS **rlmp,
	OPN_STATUS *sigstat);

/*{
** Name: opn_tprocord_validate - validate join tree order with respect to
**	table procedures
**
** Description:
**      This function looks for table procedures in the join tree and 
**	verifies that the join order suitably recognizes their parameter
**	dependencies.
**
** Inputs:
**      subquery			ptr to subquery state variable
**      osbtp				ptr to outer subtree descriptor
**	isbtp				ptr to inner subtree descriptor
**	oleaves				count of leaf entries in outer subtree
**	ileaves				count of leaf entries in inner subtree
**
** Outputs:
**	None
**	Returns:
**	    integer mask containing flags indicating that one of the following 
**	    conditions exist:
**		- join tree violates no parameter dependencies
**		- inner/outer call to opn_calcost() will violate parameter
**		dependencies (and won't be allowed)
**		- outer/inner call to opn_calcost() will violate parameter
**		dependencies (and won't be allowed)
**		- join tree violates parameter dependencies, regardless of 
**		inner/outer join sequence. Join tree will be rejected.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	9-may-2008 (dougi)
**	    Written to support table procedures.
*/
#define	BADOITREE	0x1
#define	BADIOTREE	0x2
#define	BADTREE		0x4

static i4
opn_tprocord_validate(
	OPS_SUBQUERY	*subquery,
	OPN_SUBTREE	*osbtp,
	OPN_SUBTREE	*isbtp,
	OPN_LEAVES	oleaves,
	OPN_LEAVES	ileaves)

{
    OPV_IVARS	maxvars = subquery->ops_vars.opv_rv;
    OPV_VARS	*varp;
    OPN_STLEAVES leaves;
    OPN_LEAVES	nleaves = oleaves + ileaves;
    i4		i, j, k, retval;

    retval = 0;

    /* Build leaves[] as concatenation of outer and inner leaves. */
    for (i = 0; i < oleaves; i++)
	leaves[i] = osbtp->opn_relasgn[i];
    for (j = 0; i < nleaves; i++, j++)
	leaves[i] = isbtp->opn_relasgn[j];

    /* Now loop over entire leaf node assignment looking for table procs. */
    for (i = 0; i < nleaves; i++)
    {
	varp = subquery->ops_vars.opv_base->opv_rt[leaves[i]];
	if (!(varp->opv_mask & OPV_LTPROC))
	    continue;
	if (varp->opv_parmmap == (OPV_BMVARS *) NULL ||
	    BTcount((char *)varp->opv_parmmap, maxvars) == 0)
	    continue;

	/* Have a table procedure with parameter dependencies. Loop back
	** over leaf assignments to see where they are, relative to proc. */
	for (j = -1; (j = BTnext(j, (char *)varp->opv_parmmap, maxvars)) >= 0;)
	{
	    /* Have a dependency - look for it amongst leaf nodes. */
	    for (k = 0; k < nleaves; k++)
		if (leaves[k] == j)
		    break;			/* found our match */

	    /* Found our match (if not, something is really wrong - or
	    ** we may be in greedy enumeration which may pose a problem).
	    ** Set return code depending on where we are relative to the
	    ** table procedure. */
	    if (k < i && ((k < oleaves && i < oleaves) || 
					(k >= oleaves && i >= oleaves)))
		retval |= BADTREE;
	    else if (i < oleaves && k >= oleaves)
		retval |= BADOITREE;
	    else if (k < oleaves && i >= oleaves)
		retval |= BADIOTREE;

	}
    }

    return(retval);
}

/*{
** Name: opn_nodecost	- evaluate cost of subtree node
**
** Description:
**      This is a recursive routine which will be used to evaluate the cost
**      of a subtree node
**
**	possible paths through nodecost:
**
**	node not the root
**	    A - node is a leaf
**		not a one-variable query
**			set eqh and bfm
**			a) create new rls stuct "rlp" with new hists as
**			   specified by eqh and bfm (rlp is the project-
**			   restrict of leaf->trl.
**			   set rlp->rlmap.
**			   set rlp->iomap, interesting orderings on which
**			      to do reformats.
**			b) create new eqs struct "ecp".
**			   set ecp->opn_eqcmp to required eqclasses.
**			   set relwid and relatts.
**			c) create new subtrees struct "sbp".
**			   set sbp->relasgn and sbp->structure.
**			d) create a co struct "cp".
**			   link to sbp.
**			   copy leaf's co struct into cp.
**			e) add all possible pr and reformats to sbp's
**			   co list.
**			f) save rlp, ecp and sbp in Jn.Savearr.
**
**		single variable query and this node is an index
**			same as not single variable query above.
**
**		single variable query and this node is a primary
**			same as not global->ops_estate.opn_.Singlevar but do not do step
**			(e) since a pr on the relation by itself
**			would be best.
**
**
**	    B - not a leaf node
**		a) set eqh, bfm and:
**		   jeqh - hists required out of the join.
**		   jeqc - joining eq class.
**		   leqr - required eqclasses from left subtree.
**		   reqr - required eqclasses from right subtree.
**		b) do nodecost on left and right subtrees setting:
**		   lsubtp, leqclp, lrlmp,
**		   rsubtp, reqclp, rrlmp.
**		c) create jrlmp (rls struct) for join of left and 
**		   right subtrees. 
**		   jrlmp will have the hists specified in jeqh.
**		d) create "rlp" (rls struct) which is the project-
**		   restrict of jrlmp and delete jrlmp.
**		   rlp will have hists specified in eqh and use the
**		   boolean factors in bfm.
**		   set rlp->relmap.
**		   set rlp->iomap, interesting orderings on which to
**		   do reformats.
**		e) create "ecp" (eqs struct).
**		   set ecp->opn_eqcmp, ecp->erelats and ecp->erelwid.
**		f) create "sbp" (subtrees struct)
**		   set sbp->relasgn.
**		   set sbp->structure.
**		g) add to sbp all co's possible from left and right
**		   co's (subject to several heuristics, e.g., if the
**		   cost of new co is greater than best solution so
**		   far then do not add to sbp).
**		h) add to sbp reformats from the co's already there
**		   from previous step. reformat to all interesting
**		   orderings.
**		i) save rlp, ecp, sbp in Jn.Savearr.
**
**
**	this node is at the root of tree
**	    this node is a leaf
**		(must be a single variable query)
**		set bfm map.
**		set "rlp" to point to leaf->trl.
**		create "ecp" (eqs struct).
**		   set ecp->opn_eqcmp, ecp->erelwid.
**		create "sbp" (subtrees struct).
**		create "cp" (co struct) and link to sbp.
**		   copy leaf's co struct into cp.
**		do a project-restrict and if best solution so far,
**		   save. if not then delete cp.
**		delete sbp and ecp.
**
**	    this node is not a leaf
**		a) as in (B).
**		b) as in (B).
**		c) as in (B).
**		d) "rlp" points to jrlmp. hists are those of jrlmp.
**		e) as in (B).
**		f) create "sbp".
**		g) do all joins possible and keep best.
**		h) delete rlp, ecp and sbp and rlp histos.
**
** Inputs:
**      global                          ptr to global state variable
**      np                              ptr to join subtree to evaluate i.e.
**                                      this is a structurally unique tree with
**                                      relations attached to the leaves
**      eqr                             required equivalence classes to be
**                                      produced by this node
**
** Outputs:
**      subtp                           list of cost ordering CO subtrees 
**                                      associated with this "np" i.e. all
**                                      CO subtrees have the same structure
**                                      and relations attached to leaves
**      eqclp                           OPN_EQS equivalence class node
**                                      associated with "np" i.e. head of list
**                                      which contains the subtp element above
**                                      and produces the "eqr" set of 
**                                      equivalence classes
**      rlmp                            OPN_RLS node which contains "eqclp" in
**                                      the opn_eqp list; i.e. head of list
**                                      which contains all OPN_EQS nodes which 
**                                      have exactly the relations assigned to
**                                      leaves as in "np"
**      cjeqc                           joining equivalence class is passed
**                                      back to the parent since it could
**                                      use the sorted order for "free"
**                                      on this equivalence class
**	sigstat				signal status. If an early exit was
**					requested in opn_calcost, this is set 
**					to OPN_SIGEXIT, otherwise OPN_SIGOK.
**	Returns:
**	    TRUE - if there are no usable cost orderings for this tree, could
**		happen if minimum cost tree is better than all subtrees
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	21-may-86 (seputis)
**          initial creation from nodecost
**	19-sep-88 (seputis)
**          fix double application of selectivity in opn_prleaf
**	26-oct-88 (seputis)
**          add corelated parameter to opn_sm1 and opn_sm2
**	24-jul-89 (seputis)
**          change OPN_1JEQCLS to OPN_NJEQCLS so the selectivity
**	    is not affected for leave nodes, b6966
**	14-oct-89 (seputis)
**	    - fixed access violation for complex queries
**      26-mar-90 (seputis)
**          - fixed b20632
**      29-mar-90 (seputis)
**          fix byte alignment problems
**      1-apr-92 (seputis)
**          - bug 42847 - access violation when complex query is optimized
**          and internal memory management rountines are invoked.
**	15-feb-93 (ed)
**	    - fix outer join placement problem
**	11-feb-94 (ed)
**	    - remove unneeded parameters from opl_histogram
**      15-feb-94 (ed)
**          bug 49814 - E_OP0397 consistency check
**	09-jan-1996 (toumi01; from 1.1 axp_osf port)
**	    Cast (long) 3rd... EXsignal args to avoid truncation.
**      06-mar-96 (nanpr01)
**          Increased tuple size project. Since now result tuple can be
**	    more than previous 2008(DB_MAXTUP) limit, do not check for this
**	   condition instead check against installtion maxtup size.
**	06-aug-98 (inkdo01)
**	    Added code to detect spatial joins and assure they're done by
**	    key join.
**	3-sep-98 (inkdo01)
**	    A little more code to prevent FSM joins for spatial joins.
**	19-nov-99 (inkdo01)
**	    Changes to support removal of EXsignal from early exit processing.
**	 5-mar-01 (inkdo01 & hayke02)
**	    Changes to support new RLS/EQS/SUBTREE structures. This change
**	    fixes bug 103858.
**	10-sep-01 (inkdo01)
**	    Added trace point op167 to disable key join selection.
**	18-oct-02 (inkdo01)
**	    Changes for new enumeration (only calls calcost with l/r sources,
**	    not r/l as well).
**	1-Sep-2005 (schka24)
**	    Keep BY-list eqcmap separate from jeqh so that we don't confuse
**	    anyone with the extra bits.  (The BY-list bits aren't necessarily
**	    indicative of non-joining predicates.)  Gather histograms for
**	    BY-list columns separately.
**	4-feb-06 (dougi)
**	    Add support for KEWYJ/NOKEYJ hints.
**	11-Mar-2008 (kschendel) b122118
**	    Kill OJ filtermap stuff, not used.
**	26-feb-10 (smeke01) b123349
**	    Function opn_prleaf() now returns OPN_STATUS to make trace point
**	    op255 work with set joinop notimeout.
[@history_line@]...
*/
bool
opn_nodecost(
	OPS_SUBQUERY 	   *subquery,
	OPN_JTREE          *np,
	OPE_BMEQCLS        *eqr,
	OPN_SUBTREE        **subtp,
	OPN_EQS            **eqclp,
	OPN_RLS            **rlmp,
	OPN_STATUS	   *sigstat)
{
    OPS_STATE           *global;        /* ptr to global state variable */
    OPN_SRESULT         rv;		/* the result of the search for
                                        ** previous work which has been
                                        ** completed
                                        */
    OPE_BMEQCLS         eqh;		/* bit map of equivalence classes
                                        ** which require histograms to be
                                        ** passed up the tree
                                        */
    OPB_BMBF            bfm;            /* bit map of boolean factors which
                                        ** can be applied at this node
                                        */
    OPN_RLS             *rlp;           /* equal to *rlmp */
    OPN_EQS             *ecp;           /* equal to *eqclp */
    OPN_SUBTREE         *sbp;           /* equal to *subtp */
    OPO_BLOCKS		blocks;		/* number of blocks in relation created
					** by this subtree 
					*/
    OPO_TUPLES		tups;		/* number of tuples in the relation
                                        ** created by this subtree
                                        */
    bool		icheck;		/* TRUE - if "saved" structures were
					** not found because this is the
					** first time this set of relations
					** containing this set of indexes
					** was evaluated */
    OPN_PERCENT         sel;		/* selectivity after applying boolean
					** factors to leaf */
    bool		selapplied;	/* TRUE is sel has been applied to
					** the tuple count in opn_reltups */

    global = subquery->ops_global;      /* get global state variable */
    global->ops_estate.opn_rootflg = np == global->ops_estate.opn_sroot; /*TRUE
                                        ** if routine is at top of join tree
                                        */
    *sigstat = opx_cinter( subquery );	/* this routine may not return if
                                        ** an interrupt has been detected
                                        */
    if (*sigstat == OPN_SIGEXIT) 
	return(FALSE);
    rv = opn_srchst (subquery, np->opn_nleaves, eqr, &np->opn_rlmap,
	np->opn_rlasg, np->opn_sbstruct, subtp, eqclp, rlmp, 
	&icheck);
					/* if this subtree has already been 
                                        ** evaluated return the subtp, eqclp, 
					** and rlmp pointers 
					*/
    if (rv == OPN_STEQRLS)
    {
	if (global->ops_estate.opn_rootflg)
	    EXsignal( (EX)EX_JNP_LJMP, (i4)1, (long)E_OP0400_MEMORY);
					/* partial fix for b30785
					** since index substitution was not allowed
					** if a bad guess was made initially
					** - this should rarely happen, easier to
					** just try again since opn_savest may break
					** trying to delete this
                                        */
	/* if standard enumeration decides that a subtree has no useful
	** cost orderings, that's the end of it.  However new enumeration
	** may not save a cost ordering because of a best-cost condition
	** on some plan subset that no longer applies at the current phase
	** of enumeration.  So for new enum and no cost ordering, try
	** recomputing the CO possibilities.
	*/
	if (!(subquery->ops_mask & OPS_LAENUM) || 
			(*subtp)->opn_coforw != (OPO_CO *)(*subtp))
	    return (FALSE);		/* all work was previously done so
                                        ** return with (subtp,eqclp,rlmp) info
                                        ** to the caller */
	else rv = OPN_EQRLS;		/* with new enumeration, recompute CO */
    }
    if (rv == OPN_DST)
	return (TRUE);                  /* indicates that the subtree is too
                                        ** expensive to be considered */
    if (rv == OPN_NONE)
	*rlmp = (OPN_RLS *) NULL;
    if (np->opn_nleaves == 1)		/* a leaf node */
    {	/* a leaf node was found */
	OPV_VARS               *varp;	/* ptr to range variable assigned to 
                                        ** this leaf
                                        */
	OPV_IVARS              varno;   /* range variable number assigned to 
                                        ** this leaf
                                        */
	bool		       keyed_only; /* TRUE - if this leaf will only be
                                        ** keyed and thus will not require
                                        ** project restricts or reformats */

	varno =  np->opn_prb[0];
	varp = subquery->ops_vars.opv_base->opv_rt[ varno ]; /* only one 
                                        ** variable in partition */
	if (rv <= OPN_NONE)		/* this set of relations not tried yet*/
	{
	    opn_sm1 (subquery, &np->opn_eqm, &eqh, &bfm, &np->opn_rlmap); 
					/* set eqh and bfm maps */
	    varp->opv_trl->opn_delflag = FALSE;	/* do not allow any deletion
					** from memory management routines until
					** parent CO node is finished */
	    if ((varp->opv_mask & OPV_CBF)
		&&
		(!varp->opv_bfmap))
	    {
		/* create map to determine selectivity of joining secondary indexes */
		varp->opv_bfmap = (OPB_BMBF *)opu_memory(global, (i4)sizeof(*varp->opv_bfmap));
		MEcopy((PTR)&bfm, sizeof(*varp->opv_bfmap), (PTR)varp->opv_bfmap);
	    }
	    
	    oph_relselect (subquery, rlmp, varp->opv_trl, &bfm, &eqh, 
		&selapplied, &sel, OPE_NOEQCLS, np->opn_nleaves, 
		OPN_NJEQCLS, varno);
	}
	else
	{
	    selapplied = TRUE;
	    sel = OPN_0PC;		/* only has an effect in prrleaf */
	}

	opn_ncommon (subquery, np, rlmp, eqclp, subtp, eqr, rv, 
	    np->opn_nleaves, &blocks, selapplied, sel, &tups);
					/* common routine 
				    	** for processing leaf and non-leaf 
				    	** node */
	rlp = *rlmp;
	ecp = *eqclp;
	sbp = *subtp;
	if (ecp->opn_relwid > subquery->ops_global->ops_cb->
	    ops_server->opg_maxtup)
	    opx_error(E_OP0009_TUPLE_SIZE);/* report an error if the required
                                        ** attributes from the base
                                        ** relation cannot be fetched into
                                        ** a temporary, FIXME - QEF/DMF should
                                        ** eliminate this restriction 
					*/
	keyed_only = 
	    (	!global->ops_estate.opn_rootflg 
		&&
		(global->ops_estate.opn_singlevar)
		&&
		(varno < subquery->ops_vars.opv_prv)
					/* we know that this rel is a primary
					** which will be accessed via 
					** TID's from an index. */
	    )
	    ||
	    (	(subquery->ops_joinop.opj_virtual)
		&&
		/* if this is a subselect leaf then do not add any 
		** project/restrict or reformat nodes since they are not 
		** applicable */
		varp->opv_subselect
		&&
		(   (varp->opv_grv->opv_created == OPS_SELECT)
#if 0
		    removed to only allow subselects, not aggregates to be
		    required to be a "keyed" join
		    ||
		    varp->opv_grv->opv_gsubselect->opv_subquery->ops_correlated
					/* don't allow project restrict nodes
					** for OPS_SELECT, or correlated
					** virtual tables,... views can be
					** restricted however */
#endif
		)
	   );
	if (sbp->opn_coback == (OPO_CO *)sbp)
	    *sigstat = opn_prleaf (subquery, sbp, ecp, rlp, blocks, selapplied, sel, varp, keyed_only);
					/* create new ORIG CO node on first 
					** pass through and a project/restrict
					** node if necessary, with any natural
					** ordering provide by BTREE storage 
					** structure */
    }
    else
    {	/* non-leaf node is being processed here */
	OPE_BMEQCLS	byeqcmap;	/* eqclasses which belong to the BY-
					** list (if any) and want to carry
					** forward histograms from this node
					** on up
					*/
	OPE_BMEQCLS	jeqh;		/* equivalence classes which require
                                        ** histograms for this node and above
                                        */
	OPN_JEQCOUNT	jxcnt;		/* number of joining equivalence
                                        ** classes from the subtree below
                                        */
	OPE_BMEQCLS	leqr;		/* equivalence classes required from
                                        ** the left subtree
                                        */
	OPE_BMEQCLS	reqr;		/* equivalence classes required from
                                        ** the right subtree
                                        */
	OPN_SUBTREE	*lsubtp;	/* cost orderings associated with
                                        ** the left subtree
                                        */
	OPN_SUBTREE	*rsubtp;	/* cost orderings associated with
                                        ** the right subtree
                                        */
        OPN_EQS		*leqclp;	/* OPN_EQS node containing lsubtp */
        OPN_EQS		*reqclp;	/* OPN_EQS node containing rsubtp */
        OPN_RLS		*lrlmp;		/* OPN_RLS node containing leqclp */
        OPN_RLS		*rrlmp;		/* OPN_RLS node containing reqclp */
        OPO_ISORT	jordering;	/* ordering required by this node
                                        ** from its' children, either a
                                        ** TID join or a set of joining
                                        ** equivalence classes */
        OPO_ISORT	jxordering;	/* ordering is only set if jordering is
                                        ** a tid join; this ordering represents
                                        ** useful join attributes other than
                                        ** those implied by the tid join */
        OPO_ISORT	lkeyjoin;	/* ordering for key join if left
                                        ** subtree is a ORIG node */
        OPO_ISORT	rkeyjoin;	/* ordering for key join if left
                                        ** subtree is a ORIG node */
	i4		tproc_flag = 0;	/* table procedure join dependencies */
	bool		lspat = FALSE;	/* flags to indicate left/right */
	bool		rspat = FALSE;	/* spatial joins */
	bool		lkjonly = FALSE, rkjonly = FALSE, kjhint = FALSE;
					/* kjoin hint indicators */

	if ((rv <= OPN_RLSONLY)		/* if the OPN_EQS structure was found
                                        ** then the check must have passed
                                        */
	    &&
	    (ope_twidth (subquery, eqr) > subquery->ops_global->ops_cb->
					     ops_server->opg_maxtup)
	   )
	{   /* check to see if the result of this intermediate relation
            ** is too large to place into a tuple, in which case this
            ** plan cannot be used */
	    subquery->ops_global->ops_terror = TRUE; /* at least one
				    ** query plan was skipped because
				    ** the tuple was too wide */
	    return(TRUE);	    /* tuple is too wide so ignore
				    ** this part of the search space */
	}
	opn_sm2 (subquery, np, &eqh, &bfm, &jeqh, &byeqcmap,
		 &leqr, &reqr, eqr, &jxcnt, &jordering, &jxordering,
		 &lkeyjoin, &rkeyjoin); /* split up eqr 
					** into leqr and reqr, 
                                        ** also add to leqr and reqr any 
                                        ** classes required for joins and 
                                        ** boolfacts */
	if ((global->ops_cb->ops_override & PST_HINT_NOKEYJ) ||
	    (subquery->ops_mask2 & OPS_HINTPASS &&
	    opn_nonkj_hint(subquery, np)) ||
	    (global->ops_cb->ops_check
		&&
	    opt_strace( global->ops_cb, OPT_F039_NO_KEYJOINS)
		&&
	    !(global->ops_cb->ops_override & PST_HINT_KEYJ)))
	{
	    /* Trace point says don't pick key joins. */
	    lkeyjoin = OPE_NOEQCLS;
	    rkeyjoin = OPE_NOEQCLS;
	}

	if (opn_nodecost (subquery, np->opn_child[OPN_LEFT], &leqr, &lsubtp, &leqclp, &lrlmp,
		sigstat)) 
	    return (TRUE);		/* no cost orderings less than min cost
                                        ** so delete the subtrees */
	if (*sigstat == OPN_SIGEXIT) 
	    return(FALSE);

	if (lsubtp->opn_coforw == (OPO_CO *) lsubtp)
	{
            /* FIXME - check if all CO trees are deleted to reclaim freespace
            ** that this check does not avoid including recalculation of
            ** possibly useful CO trees (since deleting CO trees does NOT
            ** include deleting the lsubtp associated with it)
            */
	    lrlmp->opn_delflag = TRUE;	/* this RLS structure may be deleted
                                        ** since no cost orderings were
                                        ** returned in one subtree */
	    return (TRUE);
	}
	if (opn_nodecost (subquery, np->opn_child[OPN_RIGHT], &reqr, &rsubtp, &reqclp, &rrlmp,
		sigstat)) 
	    return (TRUE);
	if (*sigstat == OPN_SIGEXIT) 
	    return(FALSE);

	if (rsubtp->opn_coforw == (OPO_CO *) rsubtp)
	{
	    rrlmp->opn_delflag = TRUE;
	    return (TRUE);		/* one of the subtrees is empty */
	}

	/* If subquery has TPROCs, check on parameter dependencies. */
	if (subquery->ops_mask2 & OPS_TPROC)
	    tproc_flag = opn_tprocord_validate(subquery, lsubtp, rsubtp,
		np->opn_child[0]->opn_nleaves, np->opn_child[1]->opn_nleaves);
	if (tproc_flag & BADTREE)
	    return(TRUE);		/* altogether bad ordering */
 
	global->ops_estate.opn_rootflg = np == global->ops_estate.opn_sroot;/*
					** reset because it was modified by
					** previous nodecost calls to children
					*/
	if (rv <= OPN_EQRLS)
	{   /* this relation has not been created yet */
	    OPN_RLS           *jrlmp;   /* histograms for the temporary
                                        ** relation after the join
                                        */
	    jrlmp = (OPN_RLS *) NULL;
	    if (!global->ops_estate.opn_rootflg)
	    {
		OPE_BMEQCLS         iom;/* interesting ordering map - map of
					** equivalence classes which should
					** have histograms created since
					** they may be useful
					*/
		opn_iomake (subquery, &np->opn_rlmap, &np->opn_eqm, &iom);
		BTor((i4)BITS_IN(eqh), (char *) &iom, (char *) &eqh );
		BTor((i4)BITS_IN(jeqh), (char *) &iom, (char *) &jeqh );
	    }
	    if (!subquery->ops_oj.opl_base 
		||
		(np->opn_ojid == OPL_NOOUTER)
		||
		!opl_histogram(subquery, np, lsubtp, leqclp, lrlmp, rsubtp, 
		    reqclp, rrlmp, jordering, jxordering, &jeqh, &byeqcmap,
		    &bfm, &eqh, &jrlmp, jxcnt))	    /* check if outer join
					    ** histogram processing is required
					    */
	    {
		oph_jselect (subquery, np, rlmp, lsubtp, leqclp, lrlmp,
			    rsubtp, reqclp, rrlmp, jordering, jxordering,
			    &jeqh, &byeqcmap, (OPL_OJHISTOGRAM *)NULL); 
					    /* obtain the histograms for 
					    ** equivalence classes in the joined
					    ** relation temporary which are 
					    ** required further up the join tree
					    */
		oph_relselect (subquery, rlmp, *rlmp, &bfm, &eqh, 
			    &selapplied, &sel, jordering, np->opn_nleaves, 
			    jxcnt, OPV_NOVAR);
	    }
	    else
		oph_relselect (subquery, rlmp, jrlmp, &bfm, &eqh,
			    &selapplied, &sel, jordering, np->opn_nleaves,
			    jxcnt, OPV_NOVAR);

					    /* return jrlmp if at root or
					    ** selectivity of 1.  in this
					    ** case *rlmp->opn_ereltups is not
					    ** correct if selectivity was
					    ** not 1. therefore in
					    ** nodecommon calculate correct
					    ** size of new rel, put in
					    ** "blocks" 
					    */
	}
	else
	{
	    selapplied = TRUE;
	    sel = OPN_0PC;
	}
	opn_ncommon (subquery, np, rlmp, eqclp, subtp, eqr, rv, np->opn_nleaves,
	    &blocks, selapplied, sel, &tups);
        if (rsubtp->opn_coforw == (OPO_CO *) rsubtp)
        {
            /* check again for the right and left subtree, since memory management routines
            ** may have deleted all the CO nodes, while analyzing the right subtree
            ** - this occurs when the left subtree was calculated earlier and saved
            ** ... a better plan was found, and the lookup of this old saved subtree
            ** is then is stripped of all CO nodes when memory is exhausted and free
            ** space is collected */
            rrlmp->opn_delflag = TRUE;
            return (TRUE);              /* one of the subtrees is empty */
        }
        if (lsubtp->opn_coforw == (OPO_CO *) lsubtp)
        {
            lrlmp->opn_delflag = TRUE;  /* this RLS structure may be deleted
                                        ** since no cost orderings were
                                        ** returned in one subtree */
            return (TRUE);
        }
	rlp = *rlmp;
	ecp = *eqclp;
	sbp = *subtp;
	if (subquery->ops_mask & OPS_SPATF)
	{
	    if (rkeyjoin>= 0 && rkeyjoin < subquery->ops_eclass.ope_ev &&
		subquery->ops_eclass.ope_base->
			ope_eqclist[rkeyjoin]->ope_mask & OPE_SPATJ) rspat = TRUE;
	    if (lkeyjoin>= 0 && lkeyjoin < subquery->ops_eclass.ope_ev &&
		subquery->ops_eclass.ope_base->
			ope_eqclist[lkeyjoin]->ope_mask & OPE_SPATJ) lspat = TRUE;
	    if (jordering>= 0 && jordering < subquery->ops_eclass.ope_ev &&
		subquery->ops_eclass.ope_base->
		ope_eqclist[jordering]->ope_mask & OPE_SPATJ) jordering = OPE_NOEQCLS;
	}
	if (subquery->ops_mask2 & OPS_HINTPASS)
	    kjhint = opn_kj_hint(subquery, np, &lkjonly, &rkjonly);
					/* set flags for left/right Kjoins */

	if (kjhint && !lkjonly && !rkjonly)
	{
	    /* A kjoin hint is relevant to this join, but can't ... */
	    *sigstat = OPN_SIGEXIT;
	    return(FALSE);
	}

	/* Only do the calcost call if the reverse join order is NOT a spatial join. */
	if ((rspat || !rspat && !lspat) && (lkjonly || !lkjonly && !rkjonly)
		&& !(tproc_flag & BADOITREE))
	*sigstat = opn_calcost (subquery, jordering, rkeyjoin, lsubtp, 
		rsubtp, sbp, lrlmp, rrlmp, rlp, tups, ecp, blocks, leqclp, 
		reqclp, np, np->opn_ojid, lkjonly); /* calculate possible cost 
					** orderings of the left subtree */
	if (*sigstat == OPN_SIGEXIT) 
	    return(FALSE);		/* calcost wants early exit */
	if (!(subquery->ops_mask & OPS_LAENUM) && (lspat || !rspat && !lspat) 
		&& (rkjonly || !lkjonly && !rkjonly) &&
		(!(subquery->ops_mask2 & OPS_HINTPASS) || 
		!(subquery->ops_mask2 & OPS_ORDHINT)) &&
		!(tproc_flag & BADIOTREE))
	*sigstat = opn_calcost (subquery, jordering, lkeyjoin, rsubtp, 
		lsubtp, sbp, rrlmp, lrlmp, rlp, tups, ecp, blocks, reqclp, 
		leqclp, np, np->opn_ojid, rkjonly); /* calculate possible cost
					** orderings of the right subtree */
	if (*sigstat == OPN_SIGEXIT) 
	    return(FALSE);		/* calcost wants early exit */
	lrlmp->opn_delflag = TRUE;
	rrlmp->opn_delflag = TRUE;
    }

    rlp->opn_check = icheck;		/* set flag which forces all
					** sub-structures to be evaluated
					** for this set of relations */
    opn_savest (subquery, np->opn_nleaves, sbp, ecp, rlp); /* FIXME - pass in
					** a bitmap of histograms created for
					** the joining eqcls bit map to be
					** deleted, or perhaps do not save
					** this histogram if created at root */
    return (FALSE);			/* subtree of cost orderings is
                                        ** non-empty so indicate this to the
                                        ** parent
                                        */
}
