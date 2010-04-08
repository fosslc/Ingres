/***********************/
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
#define        OPN_GNPERM      TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>

/**
**
**  Name: OPNNEWENUM.C - drive enumeration with new bottom up algorithms
**
**  Description:
**	Routines that encompass bottom up search space generation with n-join
**	lookahead.
**
**  History:
**      6-sep-02 (inkdo01)
**	    Written.
[@history_line@]...
**/

typedef struct _JDESC
{
    OPV_BMVARS	ojivmap;	/* inner vars to this OJ */
    OPV_BMVARS	ojtotal;	/* inner vars + indexes */
    OPL_OJTYPE	ojtype;		/* type of join */
    i2		ojivcount;	/* count of inner vars to this OJ */
    bool	ojdrop;
    bool	p1_reduced;	/* Reduced p1 side of full join */
    bool	p2_reduced;	/* Reduced p2 side of full join */
    i2		ojp1count;
    i2		ojp2count;
    OPV_BMVARS  p1map;
    OPV_BMVARS  p2map;
} JDESC;

static VOID
opn_clearheld(
	OPO_CO	*cop);

static void
opn_reduce_map(char *elim, char *map, i2 varcount,
			OPN_STLEAVES oemap, i4 ecount, i4 rsltix);

/*{
** Name: opn_ecomb	- generate var list combinations for enumerator
**
** Description:
**	This function simply returns successive combinations of 
**	k variable indexes from the n-variable enumeration range table.
**	K depends on the degree of lookahead and n depends on the
**	stage of enumeration being processed.
**
**	Algorithm L is used from pre-fascicle 2C on combinations,
**	destined to become part of vol. 4 of "The Art of Computer
**	Programming" by Knuth.
**
**	This algorithm generates the combinations in lexicographic sequence 
**	(i.e., sorted sequence) and thus avoids many of the intricacies of
**	the old Ingres partitioning technique.
**
** Inputs:
**      cvector				ptr to vector containing the combinations
**	n				size of range table being selected
**	k				number of entries in each combination
**					(hence the function returns the k-combinations
**					of the n-sized enumeration range table)
**
** Outputs:
**	cvector				ptr to vector containing next combination
**	Returns:
**	    TRUE - if another combination was formed
**	    FALSE - if no more combinations are available
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	9-sep-02 (inkdo01)
**	    Written.
[@history_line@]...
*/
static bool
opn_ecomb(
	OPN_STLEAVES cvector,
	i4	n,
	i4	k)

{
    i4		j;

    /* cvector contains the current combination on entry (the first being {0, 1, ...}).
    ** Using algorithm L from Knuth (vol. 4), generate the next combination and 
    ** return TRUE; else return FALSE. */

    /* NOTE: caller also initializes cvector[k] = n, cvector[k+1] = 0. */

    /* Eventually, some heuristics will be applied by this function (or possibly
    ** by another function called after opn_ecomb by its caller). This will trim
    ** some combinations from the search space without requiring further processing. */

    /* Steps L1 & L2 are contained in caller. */

    /* L3: find j */
    j = 0;
    do {
	while (cvector[j]+1 == cvector[j+1])
	{
	    cvector[j] = j;
	    j++;
	}
    } while (cvector[j+1] == cvector[j]+1);

    /* L4: termination condition */
    if (j >= k)
	return(FALSE);

    /* L5: increment cvector[j] */
    cvector[j]++;

    return(TRUE);
}

/*{
** Name: opn_combgen	- generate valid combinations from enumaration range table
**
** Description:
**	This function produces successive combinations of 
**	k-variables from the n-variable enumeration range table and 
**	applies several heuristics to verify that the combinations 
**	have the potential to generate successful query plan fragments.
**
** Inputs:
**      subquery                        ptr to current subquery being analyzed
**      cvector				ptr to vector containing the combinations
**	evarp				ptr to vector of enumeration range tab entries
**	emap				vector of integers to map comb. vector
**					entries to enumeration range tab entries
**	n				size of range table being selected
**	k				number of entries in each combination
**					(hence the function returns the k-combinations
**					of the n-sized enumeration range table)
**	firstcomb			TRUE - if 1st combination (and it's already 
**					in cvector)
**	allowCPs			TRUE to consider disconnected combos
**
** Outputs:
**	cvector				ptr to vector containing next combination
**	Returns:
**	    TRUE - if another combination was formed
**	    FALSE - if no more combinations are available
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	9-sep-02 (inkdo01)
**	    Written.
**	11-nov-02 (inkdo01)
**	    Added emap parm to provide level of indirection between evar
**	    entries being compiled and numbers 0, 1, 2, ... that the 
**	    combinations are generated from (for OJ support).
**	24-jan-05 (inkdo01)
**	    Added heuristic to prevent 2 replacement indexes for same 
**	    table in same combination.
**	7-mar-06 (dougi)
**	    Fine tune to accept combs with entry that joins outside, but 
**	    not inside when there are mutually exclusive subsets of joinable
**	    entries, but fewer than 6 entries overall.
**	22-sep-2006 (dougi)
**	    Move "2 indexes to same base table" exclusion check out of loop.
**	    CP logic short circuits this test.
**	23-jan-2007 (dougi)
**	    Had wrong test on skipCPs.
**	10-Jun-2008 (kibro01) b120306
**	    If there is an outer join, the valid combinations which may be
**	    joined together are reduced, so don't apply the <=6 limit from the
**	    whole query to restrict CPs.
**     14-Nov-2008 (kschendel) b121492
**          Let caller figure out whether CP's are allowable.  Caller is now
**          doing a connectedness analysis, so all we need to do is detect
**          a disconnected combination and reject it if CP's are not allowed.
*/
static bool
opn_combgen(
	OPS_SUBQUERY *subquery,
	OPN_STLEAVES cvector,
	OPN_EVAR *evarp,
	OPN_STLEAVES emap,
	i4	n,
	i4	k,
	bool	*firstcomb,
	bool	allowCPs)

{
    OPV_RT	*vbase = subquery->ops_vars.opv_base;
    i4		i, j, maxvars;
    OPV_BMVARS	combmap, restmap, workmap;
    OPV_VARS	*varp, *var1p;
    bool	failure = FALSE;
    bool	skipCPs;

    /* Loop over calls to opn_cgen until we run out of combinations or we get one
    ** that passes subsequent heuristic tests. */

    maxvars = subquery->ops_vars.opv_rv;

    for ( ; ; )	/* loop 'til we're done */
    {
	if (!*firstcomb && !opn_ecomb(cvector, n, k))
	    return(FALSE);	/* none left */
	*firstcomb = FALSE;

	/* Eventually, there should be a biiiig bit mask corresponding to all the 
	** combinations/permutations that will indicate whether this one has 
	** already been rejected. This will allow fast rejection on later iterations
	** through the enumeration range table, but the bit mask should be set
	** in this code when a combination is rejected by the following heuristics. */

	/* Prepare for heuristics by building bit map of all sq vars in 
	** combination, then all those not in combination. */
	MEfill(sizeof(OPV_BMVARS), 0, (char *)&combmap);
	for (i = 0; i < k; i++)
	    BTor(maxvars, (char *)&evarp[emap[cvector[i]]].opn_varmap,
		(char *)&combmap);
	MEcopy((char *)&combmap, sizeof(OPV_BMVARS), (char *)&restmap);
	BTnot(maxvars, (char *)&restmap);

	/* Heuristics test conditions:
	**  - var is replacement index and corresponding primary is also
	**    in combination,
	**  - var is Tjoin index (not replacement) and corresponding
	**    primary is NOT in combination,
	**  - 2 indexes (either placement or replacement) in combination 
	**    map same base table, and
	**  - var doesn't join other vars in combination and caller has
	**    decided that disconnected combinations are not necessary
	**    or not allowed.
	** Any of these will cause combination to be skipped. */

	failure = FALSE;

	/* First loop checks first 2 conditions. */
	for (i = -1; (i = BTnext(i, (char *)&combmap, maxvars)) >= 0;)
	{
	    varp = vbase->opv_rt[i];
	    if (varp->opv_mask & OPV_CPLACEMENT && 
		!BTtest(varp->opv_index.opv_poffset, (char *)&combmap) ||
		varp->opv_mask & OPV_CINDEX &&
		BTtest(varp->opv_index.opv_poffset, (char *)&combmap))
		failure = TRUE;
	}
	if (failure)
	    continue;

	/* Second loop checks for 2 indexes on same base table (condition
	** 3, above). */
	for (i = 0; i < k && !failure; i++)
	{
	    if (evarp[emap[cvector[i]]].opn_evmask & OPN_EVTABLE)
	    {
		/* Quick loop to assure 2 indexes for same base table
		** aren't in combination. */
		varp = evarp[emap[cvector[i]]].u.v.opn_varp;
		if (varp->opv_mask & (OPV_CINDEX | OPV_CPLACEMENT))
		 for (j = 0; !failure && j < k; j++)
		 {
		    if (i == j)
			continue;		/* skip same entry */
		    if (!(evarp[emap[cvector[j]]].opn_evmask & OPN_EVTABLE))
			continue;
		    var1p = evarp[emap[cvector[j]]].u.v.opn_varp;

		    if (var1p->opv_mask & (OPV_CINDEX | OPV_CPLACEMENT) &&
			varp->opv_index.opv_poffset ==
			    var1p->opv_index.opv_poffset)
			failure = TRUE;
		 }
	    }
	}

	/* Third loop looks at all enumeration variables in combination
        ** to test final condition (but avoided if CPs are ok).
        ** For each evar, consider the combination with that evar's vars
        ** removed (ie everything else), and make sure that the evar's
        ** joinable map has a nonempty intersection with that everything-else.
        */
        if (!failure && !allowCPs)
	{
            for (i = 0; i < k; i++)
            {
                MEcopy((char *)&combmap, sizeof(OPV_BMVARS), (char *)&workmap);
                BTclearmask(maxvars,
                        (char *)&evarp[emap[cvector[i]]].opn_varmap,
		(char *)&workmap);
	    BTand(maxvars, (char *)&evarp[emap[cvector[i]]].opn_joinable,
                        (char *)&workmap);
                /* workmap is other-vars this evar is joinable to */
                if (BTnext(-1, (char *)&workmap, maxvars) < 0)
                {
                    failure = TRUE;     /* disconnected combination */
                    break;
                }
            }
	}
	if (failure)
	    continue;
	
	/* We only get here if this one is valid. */
	return(TRUE);
    }
}

/*{
** Name: opn_eperm	- generate var list permutations for enumerator
**
** Description:
**	This function simply returns successive permutations of 
**	the current combination of k-variables from the n-variable 
**	enumeration range table. K depends on the degree of lookahead 
**	and n depends on the stage of enumeration being processed.
**
**	Algorithm L is used from pre-fascicle 2B on permutations,
**	destined to become part of vol. 4 of "The Art of Computer
**	Programming" by Knuth.
**
**	This algorithm generates the permutations in lexicographic sequence 
**	(i.e., sorted sequence) and thus avoids many of the intricacies of
**	the old Ingres partitioning technique.
**
** Inputs:
**      pvector				ptr to vector containing the permutations
**	n				number of entries in current combination
**					to be permuted
**
** Outputs:
**	pvector				ptr to vector containing next permutation
**	Returns:
**	    TRUE - if another permutation was formed
**	    FALSE - if no more permutations are available
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	9-sep-02 (inkdo01)
**	    Written.
[@history_line@]...
*/
static bool
opn_eperm(
	OPN_STLEAVES pvector,
	i4	n)

{
    i4		j, k, l, x;

    /* pvector contains the current permutation on entry (the first being the 
    ** initial k-combination selected by opn_ecomb).
    ** Using algorithm L from Knuth (vol. 4), generate the next permutation and 
    ** return TRUE; else return FALSE. */

    /* Steps L1 is contained in caller. */

    /* L2: find j */
    j = n-2;
    while (pvector[j] >= pvector[j+1] && j != -1)
	j--;
    if (j == -1)
	return(FALSE);

    /* L3: increase pvector[j] */
    l = n-1;
    while (pvector[j] >= pvector[l])
	l--;
    x = pvector[l];
    pvector[l] = pvector[j];
    pvector[j] = x;

    /* L4: reverse pvector[j+1], ..., pvector[n-1] */
    k = j+1;
    l = n-1;
    while (k < l)
    {
	x = pvector[l];
	pvector[l] = pvector[k];
	pvector[k] = x;
	k++; l--;
    }

    return(TRUE);
}

/*{
** Name: opn_permgen	- generate valid permutations from enumeration range table
**
** Description:
**	This function produces successive permutations of 
**	the current combination from the enumeration range table and
**	applies several heuristics to verify that the permutations 
**	have the potential to generate successful query plan fragments.
**
** Inputs:
**      subquery                        ptr to current subquery being analyzed
**      pvector				ptr to vector containing the permutations
**	evarp				ptr to vector of enumeration range tab entries
**	n				number of entries in current combination
**					to be permuted
**	firstperm			TRUE - if 1st permutation (and it's already 
**					in pvector)
**
** Outputs:
**	pvector				ptr to vector containing next permutation
**	Returns:
**	    TRUE - if another permutation was formed
**	    FALSE - if no more permutations are available
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	9-sep-02 (inkdo01)
**	    Written.
**	29-oct-02 (inkdo01)
**	    Add permutation heuristics (for outer join and placement indexes).
**	9-feb-03 (inkdo01)
**	    Fix "secondary precedes base table" heuristic.
**	25-apr-05 (inkdo01)
**	    Add support for ops_lawholetree.
**	3-june-05 (inkdo01)
**	    Add heuristics to eliminate some permutations that lead to
**	    CP joins to placement index.
**	5-aug-05 (inkdo01)
**	    Fine tune CP join elimination to permit OJ CP joins if there
**	    may be no alternatives.
**	29-mar-05 (dougi)
**	    Made a correction to 3-june change.
[@history_line@]...
*/
static bool
opn_permgen(
	OPS_SUBQUERY *subquery,
	OPN_STLEAVES pvector,
	OPN_EVAR *evarp,
	i4	n,
	bool	*firstperm)

{
    i4		i, j, k, maxvars;
    OPL_OJT	*lbase = subquery->ops_oj.opl_base;
    OPV_RT	*vbase = subquery->ops_vars.opv_base;
    OPV_VARS	*varp;
    OPV_BMVARS	j0, j1, onclause;
    bool	failure = FALSE;

    /* Loop over calls to opn_cgen until we run out of permutations or we get one
    ** that passes subsequent heuristic tests. */

    for ( ; ; )	/* loop 'til we're done */
    {
	if (!*firstperm && !opn_eperm(pvector, n))
	    return(FALSE);	/* none left */
	*firstperm = FALSE;
	failure = FALSE;

	/* Eventually, there should be a biiiig bit mask corresponding to all the 
	** combinations/permutations that will indicate whether this one has 
	** already been rejected. This will allow fast rejection on later iterations
	** through the enumeration range table, but the bit mask should be set
	** in this code when a combination is rejected by the following heuristics. */

	/* Permutation heuristics are applied after the tree is constructed and
	** leaf nodes assigned. Once the eventual bit mask test is performed,
	** permutations drop through and are accepted. */

	/* First heuristic is to verify that, if the query contains outer joins,
	** the current permutation doesn't "orphan" an inner variable of an outer
	** join. That is, any permutation that places outer variables from either 
	** side of an outer join together without the inner variable(s) in between
	** is disallowed. */
	if (FALSE && lbase)
	{
	    /* This heuristic is temporarily disabled - it may be replaced by
	    ** opn_jintersect heuristics and by newer OJ handling logic. */
	    OPV_BMVARS	varmaps[4], combmap;
	    OPV_BMVARS	*map1, *map2;
	    OPL_OUTER	*ojp;
	    i4		maxvars, maxojs, x;

	    maxvars = subquery->ops_vars.opv_prv;
	    maxojs = subquery->ops_oj.opl_lv;

	    /* Copy the variable maps from the permutation. */
	    for (i = 0; i < n; i++)
		MEcopy((char *)&evarp[pvector[i]].opn_primmap, 
				sizeof(OPV_BMVARS), (char *)&varmaps[i]);

	    /* We now perform pairwise analysis of successive entries in the
	    ** permutation. */
	    for (i = 0; i < n-1 && !failure; i++)
	    {
		/* For each sq variable in 1st map, examine each OJ in its
		** opv_ojmap (outer joins that it is an outer variable for). */
		MEcopy((char *)&varmaps[i], sizeof(OPV_BMVARS), (char *)&combmap);
		BTor(maxvars, (char *)&varmaps[i+1], (char *)&combmap);
						/* map[i] | map[i+1] */
		for (x = 0, map1 = &varmaps[i], map2 = &varmaps[i+1]; x <= 1 && 
			!failure; x++, map1 = map2, map2 = &varmaps[i])
						/* test goes both ways */
		 for (j = -1; (j = BTnext(j, (char *)map1, maxvars)) >= 0
					&& !failure; )
		 {
		    varp = vbase->opv_rt[j];
		    if (varp->opv_ojmap)
		     for (k = -1; (k = BTnext(k, (char *)varp->opv_ojmap, maxojs)) >= 0
					&& !failure; )
		    {
			ojp = lbase->opl_ojt[k];
			/* If 1st and/or 2nd varmaps contain OJ's opl_ivmap
			** (variables inner to the outer join), we're ok. Else,
			** if 2nd varmap is NOT a subset of OJ's opl_ovmap, it is
			** an error. */
			if (BTsubset((char *)ojp->opl_ivmap, (char *)&combmap,
				maxvars))
			    continue;		/* passes 1st test */
			if (BTsubset((char *)map2, (char *)ojp->opl_ovmap,
				maxvars))
			    continue;		/* passes 2nd test */
			failure = TRUE;		/* fails - discard the permutation */
		    }
		}
	    }
	}	/* end of outer join permutation heuristic */
	if (failure)
	    continue;

	/* The following heuristic used to limit permutations involving 
	** placement indexes to those in which the index was the first 
	** entry and the base table was immediately next. This excluded
	** a very useful class of plans in which a join is done through
	** a placement index (by Kjoin) to the underlying base table
	** (by Tjoin). So the heuristic now assures that placement indexes
	** are either first or second and immediately followed by the
	** base table. If the index is 2nd, the ops_lawholetree flag is
	** set to flag the fact that all 3 nodes of the plan fragment are
	** a single unit to be saved in any resulting composite. */

	if (n > 2 && evarp[pvector[2]].opn_evmask & OPN_EVTABLE &&
		evarp[pvector[2]].u.v.opn_varp->opv_mask & OPV_CPLACEMENT)
	 failure = TRUE;			/* placement indexes must be
						** first in permutation */
	if (failure)
	    continue;

	if ((evarp[pvector[0]].opn_evmask & OPN_EVTABLE &&
	    (varp = evarp[pvector[0]].u.v.opn_varp)->opv_mask & OPV_CPLACEMENT &&
	    (!(evarp[pvector[1]].opn_evmask & OPN_EVTABLE) ||
	     varp->opv_index.opv_poffset != evarp[pvector[1]].u.v.opn_varnum))
	    || (evarp[pvector[1]].opn_evmask & OPN_EVTABLE &&
	    (varp = evarp[pvector[1]].u.v.opn_varp)->opv_mask & OPV_CPLACEMENT &&
	    n > 2 && (!(evarp[pvector[2]].opn_evmask & OPN_EVTABLE) ||
	     varp->opv_index.opv_poffset != evarp[pvector[2]].u.v.opn_varnum)))
	 failure = TRUE;			/* placement index must be 
						** followed by corr. primary */
	if (failure)
	    continue;

	/* This heuristic eliminates permutations in which 2nd entry
	** is placement index to 3rd entry and there are no joins from
	** 1st entry to 2nd. This is possible if the 3rd entry joins the
	** 1st, but can run afoul of a opn_jintersect() heuristic later
	** in the process. */

	if (n > 2 && evarp[pvector[1]].opn_evmask & OPN_EVTABLE &&
	    (varp = evarp[pvector[1]].u.v.opn_varp)->opv_mask & OPV_CPLACEMENT &&
	    evarp[pvector[2]].opn_evmask & OPN_EVTABLE &&
	    varp->opv_index.opv_poffset == evarp[pvector[2]].u.v.opn_varnum &&
	    !BTtest(evarp[pvector[1]].u.v.opn_varnum, 
		(char *)&evarp[pvector[0]].opn_joinable))
	    continue;				/* failure */

	/* This heuristic eliminates another class of permutations that 
	** may lead to CP joins. It checks to see if the 1st and 2nd
	** entries only join to the 3rd entry and not to each other. */

	if (n > 2)
	{
	    maxvars = subquery->ops_vars.opv_rv;

	    /* j0 is pvector[1] tables that join with pvector[0]. */
	    MEcopy((char *)&evarp[pvector[0]].opn_joinable, sizeof(j0), 
		(char *)&j0);
	    BTand(maxvars, (char *)&evarp[pvector[1]].opn_varmap,
		(char *)&j0);		/* [1] tables joined by [0] */
	    /* j1 is pvector[1] tables that join with pvector[0]/[1]. */
	    MEcopy((char *)&evarp[pvector[0]].opn_joinable, sizeof(j1), 
		(char *)&j1);
	    BTand(maxvars, (char *)&evarp[pvector[1]].opn_joinable,
		(char *)&j1);
	    BTand(maxvars, (char *)&evarp[pvector[2]].opn_varmap,
		(char *)&j1);
	    if (BTcount((char *)&j0, maxvars) == 0 &&
		BTcount((char *)&j1, maxvars) != 0)
	    {
		/* We have a CP join. Drop the permutation unless
		** there's an outer join that needs this perm. */
		failure = TRUE;			/* until shown otherwise */
		for (i = 0; i < subquery->ops_oj.opl_lv; i++)
		{
		    MEcopy((char *)subquery->ops_oj.opl_base->
			opl_ojt[i]->opl_onclause, sizeof(onclause), 
			(char *)&onclause);
		    BTand(maxvars, (char *)&evarp[pvector[0]].opn_varmap,
			(char *)&onclause);
		    if (BTcount((char *)&onclause, maxvars) == 0)
			continue;
		    MEcopy((char *)subquery->ops_oj.opl_base->
			opl_ojt[i]->opl_onclause, sizeof(onclause), 
			(char *)&onclause);
		    BTand(maxvars, (char *)&evarp[pvector[1]].opn_varmap,
			(char *)&onclause);
		    if (BTcount((char *)&onclause, maxvars) == 0)
			continue;
		    MEcopy((char *)subquery->ops_oj.opl_base->
			opl_ojt[i]->opl_onclause, sizeof(onclause), 
			(char *)&onclause);
		    BTand(maxvars, (char *)&evarp[pvector[2]].opn_varmap,
			(char *)&onclause);
		    if (BTcount((char *)&onclause, maxvars) == 0)
			continue;

		    /* An OJ may depend on this potential CP join. */
		    failure = FALSE;
		    break;
		}
		if (failure)
		    continue;			/* failure */
	    }
	}

	if (evarp[pvector[1]].opn_evmask & OPN_EVTABLE &&
		varp->opv_mask & OPV_CPLACEMENT)
	    subquery->ops_lawholetree = TRUE;	/* set this funny flag */

	/* We only get here if this one is valid. */
	return(TRUE);
    }
}

/*{
** Name: opn_ojanalyze	- generate JDESC array to describe outer join nesting
**	required
**
** Description:
**      This function analyzes the ops_oj outer join descriptions to determine 
**	any precedences required when compiling plan fragments. Outer joins that 
**	only have one inner table should compile correctly with the permutation
**	heuristics. But when an outer join contains 2 or more inner tables, the
**	plan fragment to join the inners must be built into the enumeration range
**	table before the rest of the query is addressed to avoid the possibility
**	that the tables are added separately (thus leaving no way to add the 
**	other). 
**
**	Upon exit from this function, only those jdesc entries corresponding to
**	joins that must be pre-computed have their ojdrop bits OFF.
**
** Inputs:
**      subquery                        ptr to current subquery being analyzed
**      jdesc				ptr to join descriptor array built by this
**					function
**	ojcount				count of JDESC entries (one per join
**					plus one extra for each full join)
**
** Outputs:
**      jdesc				ptr to filled in join descriptor array
**      Returns:
**          TRUE - if there are subsets for which plans must be precomputed
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      4-nov-02 (inkdo01)
**          Written.
**	9-june-03 (inkdo01)
**	    Drop one of the ojdrop conditions (not fully understood and it 
**	    results in a OP04C0).
**	25-oct-04 (inkdo01)
**	    Full joins use opl_p1/p2 to get ojivmap. However, they also 
**	    contain applicable indexes - the indexes are now removed.
**	3-nov-05 (inkdo01)
**	    Drop another ojdrop condition that can cause OP0287.
[@history_line@]...
*/
static bool
opn_ojanalyze(
        OPS_SUBQUERY    *subquery,
	JDESC		*jdesc,
	i4		ojcount)

{
    OPV_RT		*vbase = subquery->ops_vars.opv_base;
    i4			i, j, k, maxprims = subquery->ops_vars.opv_prv;
    i4			maxvars = subquery->ops_vars.opv_rv;
    JDESC		*jdp1, *jdp2;
    bool		ojanal = FALSE;


    /* Set droppable bits in all outer joins that immediately contain
    ** another outer join (i.e., one has the same inners as the other
    ** plus exactly one more. opn_jintersect heuristics will suffice 
    ** in building plans for those). */
    for (i = 1; i < maxprims; i++)
     for (j = 0; j < ojcount; j++)
    {
	if ((jdp1 = &jdesc[j])->ojivcount == i && !(jdp1->ojdrop))
	 for (k = 0; k < ojcount; k++)
	{
	    if (j == k || (jdp2 = &jdesc[k])->ojdrop)
		continue;
	    /* Two entries with same ivcount should drop one - try the 
	    ** outer loop one first to reduce processing. */
	    if (jdp2->ojivcount == i)
	     if (jdp1->ojtype == OPL_INNERJOIN ||
		jdp1->ojtype == OPL_FOLDEDJOIN)
	    {
		jdp1->ojdrop = TRUE;
		break;
	    }
	     else;
	    /* It makes no sense to drop one, just because it has
	    ** the same number of entries as another. And this may
	    ** interfere with the OJ pre-computation logic and result
	    ** in OP04C0 errors. So the following bit is removed (though
	    ** left inside comments, should I remember why it was there
	    ** in the first place).
	    {
		/* Drop the second (kth) and continue. * 
		jdp2->ojdrop = TRUE;
		continue;
	    } end of removed code */
	    else if (jdp2->ojivcount != i+1)
		continue;

	    /* If jth is subset of kth, kth may be droppable. 
	    **
	    ** Once again, we're dropping one of the ojdrop conditions. 
	    ** Dropping one that contains exactly the inners of some other
	    ** OJ plus one more isn't legit. The contained OJ will generate
	    ** one composite which, added to the remaining inner, results in
	    ** an OJ that still has 2 inners and must therefore be pre-
	    ** enumerated. As with the preceding test that was commented out, 
	    ** the original code is left in case we figure out what I had
	    ** in mind to begin with.

	    if ((jdp2->ojtype == OPL_LEFTJOIN || jdp2->ojtype == OPL_RIGHTJOIN ||
			jdp2->ojtype == OPL_FULLJOIN)
		&& BTsubset((char *)&jdp1->ojivmap, (char *)&jdp2->ojivmap,
					maxprims))
	     jdp2->ojdrop = TRUE;
	    ** End of removed code. */
	}
    }

    /* Once the first pass is done, flag all non-OJs as droppable, too. And
    ** while we're in the loop, locate full joins and fix their ojtotal's 
    ** and take note of whether there are any joins to pre-compute. */
    for (i = 0; i < ojcount; i++)
     if ((jdp1 = &jdesc[i])->ojtype == OPL_FOLDEDJOIN ||
			jdp1->ojtype == OPL_INNERJOIN)
	jdp1->ojdrop = TRUE;
     else if (!(jdp1->ojdrop))
    {
	ojanal = TRUE;
	if (jdp1->ojtype == OPL_FULLJOIN)
	{
	    /* This is half of a full join. Edit its ojtotal to include
	    ** only the indexes for its tables by eliminating those for
	    ** tables in the other partition. */
	    for (j = maxprims-1; 
		(j = BTnext(j, (char *)&jdp1->ojtotal, maxvars)) >= 0; )
	     if (!BTtest(vbase->opv_rt[j]->opv_index.opv_poffset, 
				(char *)&jdp1->ojivmap))
		BTclear(k, (char *)&jdp1->ojtotal);

	    /* opl_p1/2 seem to have indexes as well. Get rid of them
	    ** from ojivmap. */
	    for (j = maxprims; j < maxvars; j++)
		BTclear(j, (char *)&jdp1->ojivmap);
	}
    }

    return(ojanal);
}

/*{
** Name: opn_jtco_merge - generate JTREE fragment from CO-tree fragment corresponding
**      to enumeration range table composite entry
**
** Description:
**      This function recursively builds a JTREE fragment corresponding to the CO-tree
**      fragment addressed by an enumeration range table composite entry. It descends
**      the CO-tree building JTREE nodes from ORIG and SJ nodes and fills in opn_pra/b
**      and opn_sbstruct in the process.
**
** Inputs:
**      subquery                        ptr to current subquery being analyzed
**      np                              ptr to join tree node addr that will anchor
**                                      JTREE fragment
**      freep                           ptr to ptr to next free JTREE node
**      cop                             ptr to CO-tree fragment from which JTREE is
**                                      to be built
**
** Outputs:
**      np                              ptr to join tree segment generated by call
**      Returns:
**          none
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      13-sep-02 (inkdo01)
**          Written.
[@history_line@]...
*/
static void
opn_jtco_merge(
        OPS_SUBQUERY    *subquery,
        OPN_JTREE       *np,
        OPN_JTREE       **freep,
        OPO_CO          *cop)

{

    OPN_JTREE   *np1;
    i4          i, j, k;


    /* Allocate/format JTREE nodes corresponding to DB_ORIG, DB_SJ nodes in CO-tree
    ** fragment (DB_PR, DB_SORT, etc. don't generate JTREE nodes). Nodes are built
    ** as we recursively descend CO-tree and opn_nleaves, opn_pra/b, opn_sbstruct are
    ** filled in upon return. */

    /* Search for DB_ORIG/DB_SJ node. */
    while (cop && cop->opo_sjpr != DB_ORIG && cop->opo_sjpr != DB_SJ)
        cop = cop->opo_outer;

    if (cop == (OPO_CO *) NULL)
        return;                                 /* this should NEVER happen */

    /* We get here with a DB_ORIG or DB_SJ node. */
    if (cop->opo_sjpr == DB_ORIG)
    {
        /* ORIG node - these are trivial. */
        np->opn_nleaves = 1;
        np->opn_nchild = 0;
        np->opn_prb[0] = np->opn_pra[0] = cop->opo_union.opo_orig;
                                                /* sq range table index */
        np->opn_sbstruct[0] = 0;                /* empty */
	np->opn_ojid = OPL_NOOUTER;
	np->opn_csz[0] = np->opn_csz[1] = 0;
	np->opn_jmask = 0;
        return;                                 /* that's all! */
    }

    /* We can only get here with a DB_SJ node. Fill it in, then in turn get
    ** each of left and right child nodes and recurse on them. Upon return,
    ** fill in this node's opn_nleaves, opn_pra/b and opn_sbstruct. */
    {
        np->opn_nchild = 2;
	np->opn_ojid = OPL_NOOUTER;
	np->opn_jmask = 0;
        np1 = *freep;				/* next free JTREE node */
        *freep = np1->opn_next;
        np->opn_child[0] = np1;
        opn_jtco_merge(subquery, np1, freep, cop->opo_outer);
                                                /* recurse on left */

        np1 = *freep;				/* next free JTREE node */
        *freep = np1->opn_next;
        np->opn_child[1] = np1;
        opn_jtco_merge(subquery, np1, freep, cop->opo_inner);
                                                /* recurse on right */

        /* Update current JTREE node from children. */
        np->opn_nleaves = np->opn_child[0]->opn_nleaves +
                np->opn_child[1]->opn_nleaves;

        /* Fill in opn_pra/b by sucking opn_prb's from children. */
        for (i = 0, j = 0; i < np->opn_nchild; i++)
         for (k = 0, np1 = np->opn_child[i]; k < np1->opn_nleaves; k++, j++)
            np->opn_prb[j] = np->opn_pra[j] = np1->opn_prb[k];

	/* Fill in opn_csz from children's opn_nleaves. */
	np->opn_csz[0] = np->opn_child[0]->opn_nleaves;
	np->opn_csz[1] = np->opn_child[1]->opn_nleaves;

        /* Then fill in opn_sbstruct by backing through node's children, sucking
        ** their opn_sbstruct's, but prepending with each child's numleaves. */
        for (i = np->opn_nchild, j = 0; i > 0; i--)
        {
            np1 = np->opn_child[i-1];
            k = 0;
            np->opn_sbstruct[j++] = np1->opn_nleaves;   /* prepend */
            while (np1->opn_sbstruct[k++] > 0)
               np->opn_sbstruct[j++] = np1->opn_sbstruct[k-1];
        }
	np->opn_sbstruct[j] = 0;			/* terminate list */

        return;
    }

}

/*{
** Name: opn_jtreegen	- generate leaf node-assigned join tree for current 
**	permutation/tree shape pair
**
** Description:
**	This function recursively builds the JTREE for the current leaf node permutation
**	and tree shape combination. At exit, the leaf nodes have been assigned
**	and the tree is ready for heuristic processing by opn_jintersect.
**
** Inputs:
**      subquery                        ptr to current subquery being analyzed
**	evarp				ptr to vector of enumeration range tab entries
**	np				ptr to join tree node addr for node built
**					by this call
**      pvector				ptr to vector containing the permutations
**	pindex				ptr to pvector index of next leaf number
**
** Outputs:
**	np				ptr to join tree segment generated by call
**	Returns:
**	    TRUE - eventually if trimming heuristics are added to this function, it
**	    may also return FALSE.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	13-sep-02 (inkdo01)
**	    Written.
**	24-may-05 (inkdo01)
**	    Tidy up - wasn't "return"ing anything, even though it is function.
[@history_line@]...
*/
static bool
opn_jtreegen(
	OPS_SUBQUERY	*subquery,
	OPN_EVAR	*evarp,
	OPN_JTREE	**np,
	OPN_STLEAVES	pvector,
	i4		*pindex,
	OPN_JTREE	**freep)
{
    i4		i, j, k;
    OPN_JTREE	*np1;


    /* This function is called recursively to descend the original n-leaf
    ** join tree shape and visit its node descriptors. For composite nodes,
    ** the function keeps recursing, but fills in opn_pra/b and opn_sbstruct
    ** upon return. For leaf nodes, if the corresponding enumeration range 
    ** table variable (as indicated in the current permutation) is an atomic
    ** subquery range table entry, opn_pra/b and opn_sbstruct are again 
    ** initialized. If the enumeration range table entry is itself composite,
    ** a call is made to opn_jtco_merge to analyze the CO-tree fragment to
    ** create a corresponding JTREE fragment anchored in the current JTREE 
    ** node. */

    if ((*np)->opn_nleaves > 1)
    {
	/* Composite JTREE node. */

	/* Recurse on left subtree. */
	if (!opn_jtreegen(subquery, evarp, &(*np)->opn_child[0], pvector, 
		pindex, freep))
	    return(FALSE);

	/* Recurse on right subtree. */
	if (!opn_jtreegen(subquery, evarp, &(*np)->opn_child[1], pvector, 
		pindex, freep))
	    return(FALSE);

	/* Update node's nleaves from children. */
        (*np)->opn_nleaves = (*np)->opn_child[0]->opn_nleaves +
                (*np)->opn_child[1]->opn_nleaves;
	(*np)->opn_ojid = OPL_NOOUTER;
	(*np)->opn_jmask = 0;
    }
    else if (evarp[pvector[(*pindex)]].opn_evmask & OPN_EVTABLE)
    {
	/* JTREE leaf node corresponding to sq range table variable. */
	(*np)->opn_nchild = 0;
	(*np)->opn_ojid = OPL_NOOUTER;
	(*np)->opn_sbstruct[0] = 0;	/* no entries - null terminate */
	(*np)->opn_prb[0] = (*np)->opn_pra[0] = 
				evarp[pvector[(*pindex)]].u.v.opn_varnum;
	(*np)->opn_csz[0] = (*np)->opn_csz[1] = 0;
	(*np)->opn_jmask = 0;
	(*pindex)++;
	return(TRUE);
    }
    else if (evarp[pvector[(*pindex)]].opn_evmask & OPN_EVCOMP)
    {
	/* JTREE leaf node corresponds to plan fragment. Call opn_jtco_merge
	** to expand to JTREE subtree. */
	opn_jtco_merge(subquery, *np, freep, evarp[pvector[(*pindex)]].u.opn_cplan);
	(*pindex)++;
	return(TRUE);
    }

    /* We get here for intermediate nodes in original JTREE. Fill in opn_pra/b,
    ** opn_sbstruct from nodes below. */

    /* First fill in opn_pra/b by sucking opn_prb's from children. */
    for (i = 0, j = 0; i < (*np)->opn_nchild; i++)
     for (k = 0, np1 = (*np)->opn_child[i]; k < np1->opn_nleaves; k++, j++)
	(*np)->opn_prb[j] = (*np)->opn_pra[j] = np1->opn_prb[k];

    /* Fill in opn_csz from children's opn_nleaves. */
    (*np)->opn_csz[0] = (*np)->opn_child[0]->opn_nleaves;
    (*np)->opn_csz[1] = (*np)->opn_child[1]->opn_nleaves;

    /* Then fill in opn_sbstruct by backing through node's children, sucking
    ** their opn_sbstruct's, but prepending with each child's numleaves. */
    for (i = (*np)->opn_nchild, j = 0; i > 0; i--)
    {
	np1 = (*np)->opn_child[i-1];
	k = 0;
	(*np)->opn_sbstruct[j++] = np1->opn_nleaves;	/* prepend */
	while (np1->opn_sbstruct[k++] > 0)
	   (*np)->opn_sbstruct[j++] = np1->opn_sbstruct[k-1];
    }
    (*np)->opn_sbstruct[j] = 0;				/* terminate list */

    return(TRUE);		/* always for now - until we introduce 
				** space trimming heuristics */
}

/*{
** Name: opn_treeshape	- generate basic n-leaf join tree shape
**
** Description:
**	This function generates the basic join tree shape with n leaves. Initially
**	this will just be a 3-leaf tree. But when 4-leaf trees are eventually also
**	supported, two trees will be required (one left deep, one bushy).
**	4- and 5-leaf trees are now supported, but just left deep for now.
**
** Inputs:
**      freep				ptr to ptr to next available JTREE node
**
** Outputs:
**	freep				ptr to ptr of next available JTREE node after
**					construction of tree shape
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-sep-02 (inkdo01)
**	    Written.
**	23-oct-02 (inkdo01)
**	    Modified to support 4- and 5-leaf trees.
**	13-dec-04 (inkdo01)
**	    Added 2 leaf tree for OJ subset case.
[@history_line@]...
*/
static void 
opn_treeshape(
	OPN_JTREE	**freep,
	i4		count)

{
    OPN_JTREE	*jtreep;

    /* This is pretty Mickey Mouse. No nifty algorithms, just take the
    ** appropriate number nodes needed for the 3-, 4- or 5-leaf left deep
    ** trees (for now) and hook 'em all up. Switch is used to separate code
    ** for each - maybe we'll get fancier later. */

    switch (count) {
      case 2:
	jtreep = (*freep);
	jtreep->opn_nleaves = 2;		/* top node */
	jtreep->opn_nchild = 2;
	jtreep->opn_child[0] = (*freep)->opn_next;
	jtreep->opn_child[1] = (*freep)->opn_next->opn_next;
	(*freep) = jtreep->opn_child[1]->opn_next;

	jtreep->opn_child[0]->opn_nchild = 0;
	jtreep->opn_child[0]->opn_nleaves = 1;
	jtreep->opn_child[1]->opn_nchild = 0;
	jtreep->opn_child[1]->opn_nleaves = 1;
	break;

      case 3:
	jtreep = (*freep);
	jtreep->opn_nleaves = 3;		/* top node */
	jtreep->opn_nchild = 2;
	jtreep->opn_child[0] = (*freep)->opn_next;
	jtreep->opn_child[1] = (*freep)->opn_next->opn_next;
	(*freep) = jtreep->opn_child[1]->opn_next;

	jtreep->opn_child[1]->opn_nchild = 0;
	jtreep->opn_child[1]->opn_nleaves = 1;

	jtreep = jtreep->opn_child[0];
	jtreep->opn_nleaves = 2;		/* 2nd level join node */
	jtreep->opn_nchild = 2;
	jtreep->opn_child[0] = (*freep);
	jtreep->opn_child[1] = (*freep)->opn_next;
	(*freep) = jtreep->opn_child[1]->opn_next;  /* free list ptr after 
						** basic 3-leaf tree */

	jtreep->opn_child[0]->opn_nchild = 0;
	jtreep->opn_child[0]->opn_nleaves = 1;
	jtreep->opn_child[1]->opn_nchild = 0;
	jtreep->opn_child[1]->opn_nleaves = 1;
	break;

      case 4:
	jtreep = (*freep);
	jtreep->opn_nleaves = 4;		/* top node */
	jtreep->opn_nchild = 2;
	jtreep->opn_child[0] = (*freep)->opn_next;
	jtreep->opn_child[1] = (*freep)->opn_next->opn_next;
	(*freep) = jtreep->opn_child[1]->opn_next;

	jtreep->opn_child[1]->opn_nchild = 0;
	jtreep->opn_child[1]->opn_nleaves = 1;

	jtreep = jtreep->opn_child[0];
	jtreep->opn_nleaves = 3;		/* 2nd level join node */
	jtreep->opn_nchild = 2;
	jtreep->opn_child[0] = (*freep);
	jtreep->opn_child[1] = (*freep)->opn_next;
	(*freep) = jtreep->opn_child[1]->opn_next;  

	jtreep->opn_child[1]->opn_nchild = 0;
	jtreep->opn_child[1]->opn_nleaves = 1;

	jtreep = jtreep->opn_child[0];
	jtreep->opn_nleaves = 2;		/* 3rd level join node */
	jtreep->opn_nchild = 2;
	jtreep->opn_child[0] = (*freep);
	jtreep->opn_child[1] = (*freep)->opn_next;
	(*freep) = jtreep->opn_child[1]->opn_next;  /* free list ptr after 
						** basic 4-leaf tree */

	jtreep->opn_child[0]->opn_nchild = 0;
	jtreep->opn_child[0]->opn_nleaves = 1;
	jtreep->opn_child[1]->opn_nchild = 0;
	jtreep->opn_child[1]->opn_nleaves = 1;
	break;

      case 5:
	jtreep = (*freep);
	jtreep->opn_nleaves = 5;		/* top node */
	jtreep->opn_nchild = 2;
	jtreep->opn_child[0] = (*freep)->opn_next;
	jtreep->opn_child[1] = (*freep)->opn_next->opn_next;
	(*freep) = jtreep->opn_child[1]->opn_next;

	jtreep->opn_child[1]->opn_nchild = 0;
	jtreep->opn_child[1]->opn_nleaves = 1;

	jtreep = jtreep->opn_child[0];
	jtreep->opn_nleaves = 4;		/* 2nd level join node */
	jtreep->opn_nchild = 2;
	jtreep->opn_child[0] = (*freep);
	jtreep->opn_child[1] = (*freep)->opn_next;
	(*freep) = jtreep->opn_child[1]->opn_next;  

	jtreep->opn_child[1]->opn_nchild = 0;
	jtreep->opn_child[1]->opn_nleaves = 1;

	jtreep = jtreep->opn_child[0];
	jtreep->opn_nleaves = 3;		/* 3rd level join node */
	jtreep->opn_nchild = 2;
	jtreep->opn_child[0] = (*freep);
	jtreep->opn_child[1] = (*freep)->opn_next;
	(*freep) = jtreep->opn_child[1]->opn_next;

	jtreep->opn_child[1]->opn_nchild = 0;
	jtreep->opn_child[1]->opn_nleaves = 1;

	jtreep = jtreep->opn_child[0];
	jtreep->opn_nleaves = 2;		/* 4th level join node */
	jtreep->opn_nchild = 2;
	jtreep->opn_child[0] = (*freep);
	jtreep->opn_child[1] = (*freep)->opn_next;
	(*freep) = jtreep->opn_child[1]->opn_next;  /* free list ptr after 
						** basic 5-leaf tree */

	jtreep->opn_child[0]->opn_nchild = 0;
	jtreep->opn_child[0]->opn_nleaves = 1;
	jtreep->opn_child[1]->opn_nchild = 0;
	jtreep->opn_child[1]->opn_nleaves = 1;
	break;
    }

}

/*{
** Name: opn_jintcaller	- perform opn_jintersect heuristics
**
** Description:
**	This function recursively descends current JTREE, calling opn_jintersect
**	to apply heuristics for each join node.
**
** Inputs:
**	subquery			ptr to subquery control structure
**      jtreep				ptr to jtree fragment to validate
**	sigstat				ptr to return status
**
** Outputs:
**	jtreep				ptr to jtree fragment with extra stuff
**					added (e.g. opn_ojid)
**	sigstat				ptr to result status
**	Returns:
**	    TRUE			if jtree passes muster
**	    FALSE			jtree fails heuristics
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-oct-02 (inkdo01)
**	    Written.
[@history_line@]...
*/
static bool
opn_jintcaller(
	OPS_SUBQUERY	*subquery,
	OPN_JTREE	*jtreep,
	OPN_STATUS	*sigstat)

{
    /* This function replaces the builtin recursive descent analysis of 
    ** Jtrees done by the interleaved logic of opn_arl and opn_jintersect.
    ** It simply calls opn_jintersect with the current Jtree, then recurses
    ** on left and right subtrees. Just exit if not a composite (join) node.
    ** Anyone that returns FALSE, causes this call to return FALSE, as well. */

    *sigstat = OPN_SIGOK;		/* default result */

    if (jtreep->opn_nleaves <= 1)
	return(TRUE);

    if (opn_jintersect(subquery, jtreep, sigstat) == FALSE)
	return(FALSE);			/* call jintersect for this node */

    if (opn_jintcaller(subquery, jtreep->opn_child[0], sigstat) == FALSE)
	return(FALSE);			/* recurse on left subtree */

    if (opn_jintcaller(subquery, jtreep->opn_child[1], sigstat) == FALSE)
	return(FALSE);			/* recurse on right subtree */

    return(TRUE);
    
}

/*{
** Name: opn_plantest	- analyze individual CO nodes
**
** Description:
**	This function examines nodes in the current phase "bestplan" and clears
**	bits from a bit mask corresponding to the base tables referenced by orig 
**	nodes. The bit mask is post-tested to see if the current "bestplan"
**	covers the whole set of primaries for this subquery.
**
** Inputs:
**	subquery			ptr to subquery control structure
**      cop				ptr to current CO-node being analyzed
**	primset				ptr to bit map of primaries
**
** Outputs:
**	primset				bit map with entries for table ref'ed in 
**					orig nodes turned off
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	3-oct-02 (inkdo01)
**	    Written.
**	23-Jun-08 (kibro01)
**	    Correct opv_prv check to >= from >
[@history_line@]...
*/
static void 
opn_plantest(
	OPS_SUBQUERY	*subquery,
	OPO_CO		*cop,
	OPV_BMVARS	*primset)

{
    OPV_RT	*vbase = subquery->ops_vars.opv_base;
    OPV_VARS	*varp;
    OPV_IVARS	primary;
	
    /* Loop over plan (iterate on opo_outer, recurse on opo_inner) 
    ** looking for ORIG nodes. For those, determine primary table
    ** index (if it is a primary or substitute) and clear corresponding
    ** bit from the mask. */

    do {
	if (cop->opo_sjpr == DB_ORIG)
	{
	    primary = cop->opo_union.opo_orig;
	    if ((varp = vbase->opv_rt[primary])->
				opv_mask & OPV_CINDEX)
		primary = varp->opv_index.opv_poffset;
	    else if (primary >= subquery->ops_vars.opv_prv)
		return;
	    BTclear((i4)primary, (char *)primset);
	    return;
	}

	/* Not ORIG, recurse on inner (if non-null), then iterate on outer. */
	if (cop->opo_inner)
	    opn_plantest(subquery, cop->opo_inner, primset);

    } while (cop = cop->opo_outer);

}

/*{
** Name: opn_fullplan	- test current bestplan for completeness
**
** Description:
**	This function determines if the ops_bestco returned by the current 
**	phase is a "complete" plan. That is, it determines if all primary
**	variables of the subquery are covered by the ORIG nodes of the plan.
**	This allows the enumeration to exit before completion of all phases if
**	the plan suffices (if a complete plan is found in phase n-1, it must be
**	better than any complete plan found in later phases).
**
** Inputs:
**	subquery			ptr to subquery control structure
**	emap				integer vector describing enumeration
**					range table entries in this plan
**					fragment
**
** Outputs:
**	    none
**	Returns:
**	    TRUE	if the plan is complete
**	    FALSE	otherwise
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	3-oct-02 (inkdo01)
**	    Written.
**	11-nov-02 (inkdo01)
**	    Added 2 parms for OJ support.
**	22-sep-2006 (dougi)
**	    Dropped evarp parm - it served no purpose.
*/
static bool 
opn_fullplan(
	OPS_SUBQUERY	*subquery,
	OPV_BMVARS	*primset)

{
    OPV_BMVARS	primset1;
    i4		i;

    /* Start by copying primary variable bitmask to local variable.
    ** The function opn_plantest will recursively analyze the plan
    ** and clear all bits for primary variables that appear in the 
    ** plan's ORIG nodes. If the mask is empty upon return, the 
    ** plan is complete. */

    MEcopy((char *)primset, sizeof(OPV_BMVARS),(char *)&primset1);

    opn_plantest(subquery, subquery->ops_bestco, &primset1);

    if (BTnext(-1, (char *)&primset1, subquery->ops_vars.opv_prv) >= 0)
	return(FALSE);		/* still bits set - not complete */
    return(TRUE);		/* ta da! */
}

/*{
** Name: opn_bestfrag	- locate CO fragment from current bestco
**
** Description:
**	This function uses the current ops_bestco to locate the CO-fragment
**	that would be saved in the current enumeration cycle. It just descends
**	the left side of the CO-tree an appropriate number of levels (depending
**	on the lookahead degree) and saves the fragment's CO addr into
**	ops_bestfragco. It is best to externalize this logic from opncalcost.c 
**	since it depends on the lookahead degree.
**
** Inputs:
**	subquery			ptr to subquery control structure
**
** Outputs:
**	subquery->ops_bestfragco	ptr to CO fragment
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	23-oct-02 (inkdo01)
**	    Written.
**	11-nov-02 (inkdo01)
**	    Modified to handle tiny trees (2-leaf) for OJ support.
**	17-may-04 (inkdo01)
**	    Support "newbestfrag" flag.
**	4-nov-04 (inkdo01)
**	    Release perm nodes of previous best fragment (if any).
**	25-apr-05 (inkdo01)
**	    Add support for ops_lawholetree.
[@history_line@]...
*/
VOID
opn_bestfrag(
	OPS_SUBQUERY	*subquery)

{
    OPO_CO	*cop;
    i4		lacount = subquery->ops_lacount;

    /* Simply descend ops_bestco to the 2nd join on left and place in 
    ** ops_bestfragco. Next step is to use ops_.... to parameterize how
    ** far to search (once more than 1 lookahead degree/scheme is 
    ** supported). */

    if (subquery->ops_bestfragco != (OPO_CO *) NULL &&
	subquery->ops_bestfragco->opo_held)
	opn_clearheld(subquery->ops_bestfragco);

    subquery->ops_fraginperm = FALSE;
    if ((subquery->ops_bestfragco = cop = subquery->ops_bestco) == NULL)
	return;				/* this is essentially impossible */

    subquery->ops_mask |= OPS_NEWBESTFRAG;
					/* tell opn_newenum we've a new best */
    subquery->ops_bestwholetree = subquery->ops_lawholetree;
					/* indicate tree status */
    if (lacount == 2 || subquery->ops_lawholetree)
	return;				/* 2-leaf tree from OJ plan frag or
					** frag with placement index in 						** 2nd leaf, not 1st */

    /* For 1 join lookahead - 3 table joins - just nab the first join down
    ** on the outer side. */
    cop = cop->opo_outer;
    if (cop->opo_sjpr != DB_SJ)
	cop = cop->opo_outer;		/* must be DB_PR - descend one more */

    if (lacount > 3)
    {
	cop = cop->opo_outer;		/* drop to next join for 4 tables */
	if (cop->opo_sjpr != DB_SJ)
	    cop = cop->opo_outer;	/* again, skip DB_PR */
	
	if (lacount > 4)
	{
	    cop = cop->opo_outer;	/* Try up to 5 tables */
	    if (cop->opo_sjpr != DB_SJ)
		cop = cop->opo_outer;
	}
    }

    subquery->ops_bestfragco = cop;

}

/*{
** Name: opn_clearheld	- reset opo_held flag of plan fragments
**	subsumed by others.
**
** Inputs:
**      cop	ptr to CO-subtree in which held flag is to be cleared
**
** History:
**	29-oct-04 (inkdo01)
**	    Written as part of permanent CO memory management.
[@history_line@]...
*/
static VOID
opn_clearheld(
	OPO_CO	*cop)
{

    /* Loop over CO-nodes clearing opo_held flag to allow them to
    ** be reused. */

    while (cop != (OPO_CO *) NULL)
    {
	/* Iterate on outer's, recurse on inner's. */
	cop->opo_held = FALSE;

	/* Patch OPN_SUBTREE chain around this node. */
	if (cop->opo_coforw)
	    cop->opo_coforw->opo_coback = cop->opo_coback;
	if (cop->opo_coback)
	    cop->opo_coback->opo_coforw = cop->opo_coforw;
	cop->opo_coforw = cop->opo_coback = (OPO_CO *) NULL;
	if (cop->opo_inner)
	    opn_clearheld(cop->opo_inner);

	cop = cop->opo_outer;
    }

}

/*
** Name: opn_evconnected - Analyze evar-connectedness of plan fragment
**
** Description:
**     In order to make proper decisions about allowing cartesian
**     products, we need to understand the connectedness of the
**     enumeration variables left to combine.  The combination
**     generator is only allowed to propose a disconnected evar
**     combination if there are in fact disconnected evars (or,
**     if there are "sufficiently few" evars left and the user
**     has chosen to encourage cart-prod's with OP180.)
**
**     In the case of an actually disconnected set of evars, we return
**     the size (number of evars) of the largest connected subset.
**     The planner can use that to reduce the lookahead value so as
**     to encourage the combination of everything joinable first,
**     pushing any necessary cart-prod's to the top of the plan.
**     (The same notion is implemented in opnarl for standard enumeration,
**     although of course it works differently there.)
**
**     Note that we're interested in evar connectedness, not variable
**     connectedness -- if a cart-prod is "frozen" into an existing
**     composite evar, it's of no interest any more.
**
** Inputs:
**     subquery                The subquery being analyzed
**     evars                   Pointer to the evar array
**     iemap                   Map 0..nvars-1 to evar entries; might not be
**                             the identity map if doing an OJ fragment
**     nvars                   Number of evars being considered
**     num_partitions          An output
**     largest_partition       An output
**
** Outputs:
**     num_partitions          Number of distinct connected regions;
**                             1 implies entirely evar-connected query
**     largest_partition       The number of evars in the largest
**                             connected region
**
** History:
**     12-Nov-2008 (kschendel)
**         Written as part of cart-prod prevention for data warehouse
**         star or nearly-star queries.
*/

static void
opn_evconnected(OPS_SUBQUERY *subquery,
       OPN_EVAR *evarp, OPN_STLEAVES iemap, i4 nvars,
       i4 *num_partitions, i4 *largest_partition)
{
    i4         evnum;
    i4         i,j;
    i4         num_parts, largest_part;
    i4         varcount;               /* Highest [e]var number in query */
    OPV_BMVARS allevars;               /* All evars still relevant */
    OPV_BMVARS allvars;                /* Variables covered by allevars */
    OPV_BMVARS done;                   /* evars already dealt with */
    OPV_BMVARS pending;                /* evars joinable to, not yet traced */
    OPV_BMVARS reached;                /* evars reached via joinability */
    OPV_BMVARS workmap;

    num_parts = 0;
    largest_part = 0;
    varcount = subquery->ops_vars.opv_rv;  /* Max for both vars and evars */

    /* Start by collecting all the evars (table or composites) that are
    ** going to be considered by neweplan in the current pass.
    */
    MEfill(sizeof(OPV_BMVARS), 0, (char *)&allevars);
    for (i=0; i < nvars; i++)
       BTset(iemap[i], (char *)&allevars);

    /* Outer loop repeats for each partition found.  Inner loop uses
    ** joinability to find all of the evars reached from a starting point,
    ** i.e. a partition.
    ** As we find partitions, those evars are removed from allevars,
    ** until nothing is left.
    */
    for (;;)
    {
       /* The joinability maps operate on variables, not evars, and we
       ** wish to ignore vars not part of the evar set being considered.
       ** Build a map of variables in this evar set.
       */
       MEfill(sizeof(OPV_BMVARS), 0, (char *)&allvars);
       evnum = -1;
       while ( (evnum = BTnext(evnum, (char *)&allevars, varcount)) >= 0)
           BTor(varcount, (char *)&evarp[evnum].opn_varmap, (char *)&allvars);

       MEfill(sizeof(OPV_BMVARS), 0, (char *)&pending);
       MEfill(sizeof(OPV_BMVARS), 0, (char *)&reached);
       MEfill(sizeof(OPV_BMVARS), 0, (char *)&done);
       /* Start with the first evar in the map */
       evnum = BTnext(-1, (char *)&allevars, varcount);
       if (evnum == -1)
           break;                      /* We are done */
       BTset(evnum, (char *)&pending);

       /* The inner loop takes an evar off the pending map, and indicates
       ** that we've "reached" this evar.  It then translates that
       ** evar's var-joinables to evar-joinables, and marks those
       ** joinable evars (that we haven't already seen) as pending.
       ** In this manner we visit and record all of the evars that
       ** are join-reachable from the first one.  This set is one
       ** partition of the total evar set.
       */

       for (;;)
       {
           evnum = BTnext(-1, (char *)&pending, varcount);
           if (evnum == -1)
               break;                  /* Done with this partition */
           BTclear(evnum, (char *)&pending);
           if (BTtest(evnum, (char *)&done))
               continue;               /* Already saw this evar, move along */
           BTset(evnum, (char *)&done);
           BTset(evnum, (char *)&reached);

           /* We're interested in vars joinable from this evar, but
           ** we don't care about anything out of current eplan scope,
           ** and we can ignore anything already part of this evar.
           ** (The second condition is relevant for composite evars.)
           */
           MEcopy((char *)&evarp[evnum].opn_joinable, sizeof(OPV_BMVARS),
                       (char *)&workmap);
           BTand(varcount, (char *)&allvars, (char *)&workmap);
           BTclearmask(varcount, (char *)&evarp[evnum].opn_varmap, (char *)&workmap);
           i = -1;
           while ( (i = BTnext(i, (char *)&workmap, varcount)) >= 0)
           {
               /* Find the evar containing variable # i; it should be one
               ** of the ones in allemaps thanks to the masking above.
               */
               j = -1;
               while ( (j = BTnext(j, (char *)&allevars, varcount)) >= 0)
                   if (BTtest(i, (char *)&evarp[j].opn_varmap))
                       break;
               if (j < 0)
               {
                   /* Hmm, not sure what to make of this, log a trace msg
                   ** and just keep going.  Hope it can't happen.
                   */
                   TRdisplay("%@ opn_evconnected expected to find var %d, not in evars\n",i);
                   continue;
               }
               if (! BTtest(j, (char *)&done))
                   BTset(j, (char *)&pending);
           }
       } /* inner for */

       /* Now the evars in the partition are in "reached", record its size
       ** if it's the largest, and clear the partition evars from allevars.
       ** Then loop back to see if there's anything left.
       */
       ++ num_parts;
       i = BTcount((char *)&reached, varcount);
       if (i > largest_part)
           largest_part = i;
       BTclearmask(varcount, (char *)&reached, (char *)&allevars);
    } /* outer for */

    /* All done, return results */

    *num_partitions = num_parts;
    *largest_partition = largest_part;

} /* opn_evconnected */

/*{
** Name: opn_neweplan	- generate plan from varset
**
** Description:
**	This function generates a plan from a supplied set of enumeration
**	variables. It provides a level of indirection that permits plan
**	fragments to be built from subsets of the subquery range table 
**	in support of outer join queries for which plan fragments must
**	be built in predefined sequences.
**
**	The new join order enumeration technique uses a bottom up
**	approach that only examines joins with n tables/indexes (where
**	n is 3, 4 or 5, depending on conditions). It iterates over this
**	process, successively replacing pairs of tables/indexes with
**	the lowest cost join, until the whole plan has been constructed.
**	for large numbers of tables, this technique can be many orders of 
**	magnitude cheaper to run.
**
**	This function is the secondary driver of the new join order
**	enumerator. It contains 4 nested loops:
**	    1. outer loop that produces one join at a time (replacing
**	    2 tables/joins from the current range table),
**	    2. loop over all combinations of n tables/indexes/joins
**	    to cost next join added to table,
**	    3. loop to build all permutations of current combination,
**	    4. loop to assign current permutation to each order n binary
**	    tree shape.
**
**	At the inside of the loop, calls are made to join order heuristic
**	functions, then to opn_ceval to produce a cost ordering for the 
**	current combination/permutation/tree shape.
**	
**	At the end of the outer loop, it edits the enumeration range 
**	table to replace the sources of the entries from which the current
**	cycle's join is built by the composite result. Secondary indexes on
**	the tables in the join are dropped and the enumeration table is 
**	consolidated.
**
**	Outer joins: A significant complication is introduced by outer joins.
**	It is important to avoid separating tables that are inner to the same
**	outer join because the heuristics will not guarantee an eventual
**	ordering in which they are together in the outer join (unlike the
**	old heuristic that looks at the whole query). Accordingly, an
**	analysis is made of outer join relationships in the query and any
**	subsets of the query that must be compiled as units to guarantee
**	outer join success are passed to opn_neweplan one at a time before 
**	the final call to compile the whole query.
**
**	This requirement to pre-compile OJ plan fragments may also lead
**	to a request to compile a 2-node tree. Since opn_neweplan was 
**	written to compile 3-node and higher trees, this requires a bit of 
**	extra logic sprinkled throughout the function. At some point, it
**	would be good to add code to compile both a 2-node plan and a
**	n-node plan for OJ fragments in anticipation that a subplan with
**	secondary indexes might be better or worse than a simple 2 node
**	plan (on the base tables or replacement indexes). For now, if there
**	is at least one applicable secondary (but not replacement) index,
**	the plan fragment must have at least 3 nodes.
**
** Inputs:
**      subquery                        ptr to current subquery being analyzed
**	evarp				ptr to vector of enumeration range 
**					table descriptors
**	iemap				integer vector that maps 
**					enumeration range table entries to
**					combinations/permutations generated
**					in this function
**	oemap				map showing changed enumeration range
**					table locations due to compaction
**	freep				ptr to first free OPN_JTREE ptr
**					(following pre-formatted tree)
**	evarcount			count of OPN_EVAR active entries
**	pvarcount			count of prim vars for OJ plan
**					fragments, or -1
**	rsltix				ptr to integer evar array index of
**					result composite of this call
**
** Outputs:
**	Returns:
**	    OPN_SIGOK if a plan fragment was successfully generated
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	11-nov-02 (inkdo01)
**	    Copied from opn_newenum and re-engineered for OJ support.
**	14-jan-03 (inkdo01)
**	    Changed failing error message to 4C0 and make it return 
**	    condition to force into old enumeration.
**	12-feb-03 (inkdo01)
**	    Added AND of mask to eliminate subselect result EQCs from 
**	    EQC map used to compile plan fragments.
**	10-june-03 (inkdo01)
**	    Pre-compute bit map of vars that must be in final plan (doing
**	    it in opn_fullplan is too late because of evar entry permutations.
**	    Also permit a plan to be built from only 2 variables (as can 
**	    be the case for an OJ plan fragment). (This is incomplete -
**	    new parm is passed, but nothing is yet done.)
**	2-july-03 (inkdo01)
**	    Map secondary indexes from each combination back to corresponding
**	    base vars prior to setting OJ special EQC bit (bug 110518).
**	13-july-03 (inkdo01)
**	    Slight change to above because it re-broke other OP0889 fixes.
**	13-aug-04 (inkdo01)
**	    Move some debug display and fix adjustments to iemap array for
**	    OJ subset calls (fixes a OP0287).
**	25-oct-04 (inkdo01)
**	    Tidy assignments of -1 to end of maps and compute ert_degree 
**	    properly (when indexes are removed from OJsubsets).
**	13-dec-04 (inkdo01)
**	    Fix handling of 2-leaf join trees (from OJsubset) and drop
**	    opn_jtreetidy in favour of opn_treeshape (fixes 113615).
**	24-jan-05 (inkdo01)
**	    Don't copy to permanent when final plan has been constructed.
**	27-jan-05 (inkdo01)
**	    Fix construction of prmiset from varmaps and secondary ixes and
**	    fix for plans with only 2 nodes (as can happen in OJ subsets).
**	10-mar-05 (inkdo01)
**	    Fine tuning of OJ subsets. Assure that 3 (or more) table subsets
**	    reduce properly.
**	25-apr-05 (inkdo01)
**	    Add support for ops_lawholetree to fine tune IX heuristics.
**      20-may-2005 (huazh01)
**          check the boundary of iemap to prevent SEGV. 
**          INGSRV3310, b114536.
**	22-sep-2006 (dougi)
**	    Add consistency check to assure neweplan generates a plan that
**	    actually covers the subquery.
**	15-oct-2006 (dougi)
**	    If we're in last pass and a base table and a replacement ix for
**	    the base table are still in evar[], only take entries la_count-1
**	    at a time.
**	8-mar-2007 (dougi)
**	    Added CPU metric to op209 debug display.
**	16-apr-07 (hayke02)
**	    Partial back out of the fix for bug 110518. This change fixes bug
**	    117868.
**      05-mar-08 (kschendel)
**          If, on exiting combination loop, no query fragment has been found
**          for some reason, memory is cleared (opn_recover), and the loop is
**          attempted again. In this situation, evarcount wasn't being returned
**          to its previous value, which can lead to a corrupted qp and
**          E_OP0889 errors later on during query compilation.
**	23-Jun-08 (kibro01) b120537
**	    If on the last loop through (la_count==3) we end up with a 
**	    combination, a base table, and two or more replacement indices,
**	    we can't combine them.  Detect this case and reduce la_count so
**	    we can produce a valid 2-part plan.
**	23-Jun-08 (kibro01)
**	    Correct opv_prv test to >= from >
**	24-jun-08 (hayke02)
**	    Correct indexing of ixmap oemap condensing from j and j+1 to k and
**	    k+1. This change fixes bug 119326.
**	    Also, change E_OP04C1 so that it behaves like E_OP04C0, soft
**	    failing and retrying with non-greedy enumeration, unless op247 is
**	    set.
**      7-Nov-2008 (kschendel)
**          Unify replacement-index conflict tests, and make them work
**          properly when not operating on the identity iemap (eg when dealing
**          with an oj subset).
**      13-Nov-2008 (kschendel)
**          Analyze connectedness of the current combination candidates (evars)
**          at the start of each pass.  This allows better decisions as to
**          whether to allow cart-prod combinations.  It also is used
**          to twiddle the lookahead, so that any CP's that are essential
**          can get pushed to the top of the plan (which is usually where
**          they do the least damage).  A side effect of this change is that
**          the OP180 trace point now has a similar meaning for greedy
**          enumeration as it does for traditional, i.e. include gratuitous
**          cart-prod's in the plan search space.
**	16-Dec-2008 (kibro01) b119685/118962
**	    Need to copy equivalence class list for full joins as well as
**	    just left and right.
**	27-Apr-2009 (kibro01) b122001
**	    Also allow CPs in the case where we've tried already without them
**	    and found nothing.  This can occur when an outer join query
**	    ends up requiring cart-prods, but which must be done prior to
**	    joining the final base table.
**	    Never set threeleaf if there are only two elements, or else you
**	    can end up adding an erroneous node to the JTREE.
**	27-Apr-2009 (kibro01) b121896
**	    Start checking for full plan completeness when the
**	    objects-already-combined-count plus the number of objects combined
**	    in one iteration reaches the base table count, not when it reached
**	    the count of all the objects possible in the plan.  Otherwise you
**	    can have a 3-base-table query with many indices which doesn't
**	    recognise that the plan is complete and fails with E_OP04C0.
**	26-May-2009 (kibro01) b122112
**	    Correct the "adjust la_count due to small partitions" logic.
**	3-Jun-2009 (kschendel) b122118
**	    Fix some garbled indents.
**	    Reinit bfpexception flag when starting a new bestco search.
**      31-jul-2009 (huazh01)
**          Correc the fix to bug 122001. 'first_noplan' gets set to FALSE,
**          not TRUE, if we tried but couldn't find a plan.
**          This fixes bug 122378.
**      10-sep-2009 (huazh01)
**          break out the combination loop if we've got a plan fragment
**          for an OPS_IDIOT_NOWHERE query.
**          (b122530)
[@history_line@]...
*/
static OPN_STATUS
opn_neweplan(
	OPS_SUBQUERY    *subquery,
	OPN_EVAR	*evarp,
	OPN_STLEAVES	iemap,
	OPN_STLEAVES	oemap,
	i4		evarcount,
	i4		pvarcount,
	i4		*rsltix)

{
    OPN_STLEAVES	cvector;		/* index vectors to map current */
    OPN_STLEAVES	pvector;		/* combination and permutation
						** var set to OPN_EVAR vector */
    OPS_STATE	*global = subquery->ops_global;
    OPN_JTREE	*jtreep, *freep;
    i4		i, j, k, temp;
    i4		ert_degree, pindex, fvarcount;
    i4		varcount = subquery->ops_vars.opv_rv;
    i4		la_count = subquery->ops_lacount;
    i4		old_lacount = la_count;
    i4		num_partitions, largest_partition;
    OPL_OJT	*lbase = subquery->ops_oj.opl_base;
    OPV_BMVARS	ixmap, pvarmap, primset;
    OPL_BMOJ	tempmap, *ojmap = &tempmap;
    OPL_OUTER	*ojp;
    OPE_BMEQCLS eqcmap;
    OPN_STATUS	sigstat;
    i2		bestix[3];
    OPO_CO	*ocop, *icop;
    bool	firstcomb, firstperm, ixfolded;
    bool	doneCO = FALSE;
    bool	first_noplan = TRUE;
    bool	final = (oemap[0] == -1);	/* TRUE - if last (or only) call */
    bool	identity = TRUE;		/* TRUE - if only call */
    bool	twotab = FALSE;
    bool	OJsubset = (pvarcount > 0);
    bool	threeleaf;
    i4		base_table_count = 0;
    bool	allowCPs;	/* Allow cart-prod combos */

    for (fvarcount = 0; fvarcount < OPN_MAXLEAF && iemap[fvarcount] >= 0; 
		    fvarcount++)
    {
        if (iemap[fvarcount] != fvarcount)
	    identity = FALSE;	/* count no. of evar's to compile */
	if ((evarp[iemap[fvarcount]].u.v.opn_varp->opv_mask &
		 (OPV_CPLACEMENT|OPV_CINDEX)) == 0)
	    base_table_count++;
    }

    if (fvarcount < varcount)			/* and determine if this is       */
	identity = FALSE;			/* identity mapping */
    if (la_count > fvarcount)
	subquery->ops_lacount = la_count = fvarcount;

    if (global->ops_cb->ops_check		/* but first check for dump */
	    &&
	    opt_strace( global->ops_cb, OPT_F081_DUMPENUM))
    {
	/* A little more debuggery. */
	TRformat(opt_scc, (i4 *)global,
	    (char *)&global->ops_trace.opt_trformat[0],
	    (i4)sizeof(global->ops_trace.opt_trformat),
	    "NEWEPLAN call - evc: %d, fvc: %d, pvc: %d, vc: %d, lac: %d\n\n",
	    evarcount, fvarcount, pvarcount, varcount, la_count);
    }

    if (FALSE && pvarcount == 2)
    {
	/* OJ plan fragment with exactly 2 base tables. We now play a trick
	** to force compile of a fragment with ONLY those tables, then revert
	** to compiling with their indexes, too. That way the best subplan
	** is chosen, whether or not it uses indexes. */
	/* Haven't figured out how to do this, yet. */
	twotab = TRUE;
	subquery->ops_lacount = la_count = 2;	/* force 1st loop to do 
						** prim var plans only */
    }

    /* Prepare bit map of vars that must be covered by eventual plan. This
    ** is passed to opn_fullplan to allow potential final plans to be 
    ** assessed. Simply loop over vars in current permutation, accumulating
    ** bits from their varmaps. */
    MEfill(sizeof(OPV_BMVARS), 0, (char *)&primset);

    for (i = 0; i < OPN_MAXLEAF && iemap[i] >= 0; i++)
    {
	if (evarp[iemap[i]].opn_evmask & OPN_EVTABLE)
	 if (evarp[iemap[i]].u.v.opn_varnum < subquery->ops_vars.opv_prv)
	    BTset(evarp[iemap[i]].u.v.opn_varnum, (char *)&primset);
	 else;
	else if (evarp[iemap[i]].opn_evmask & OPN_EVCOMP)
	{
	    /* Loop over varmap setting corresponding primvars into primset. */
	    for (j = -1; (j = BTnext(j, (char *)&evarp[iemap[i]].opn_varmap,
			varcount)) >= 0; )
	    {
		if ((k = j) >= subquery->ops_vars.opv_prv)
		{
		    OPV_VARS	*varp;

		    varp = subquery->ops_vars.opv_base->opv_rt[j];
		    k = varp->opv_index.opv_poffset;
		}
		BTset(k, (char *)&primset);
	    }
	}
    }

    /* Loop 1: generate best n-join plan from current enumeration
    ** set. */
    for (ert_degree = fvarcount; ert_degree >= la_count; ert_degree--, evarcount--)
    {
        /* Analyze connectedness of the (remaining) evars.
        ** This allows proper decisions to be made as to whether to allow
        ** cart-prod fragments, as well as fiddling the lookahead to try to
        ** push any necessary cart-prod's up to the top.
        ** Returns number of "partitions" (connected evar sets), and the
        ** size of the largest one.
        */
        opn_evconnected(subquery, evarp, iemap, ert_degree,
                &num_partitions, &largest_partition);
 
        if (num_partitions > 1)
        {
            /* Try to reduce la_count for disconnected query fragment in
            ** an attempt to get all joinable fragments joined first,
            ** pushing the cart-prod to the top.  This is similar to what
            ** opnarl tries to do in standard enumeration, and is
            ** often the best way.
            ** If we are faced with individual disconnected fragments, it's
            ** probably cart-prod time, just operate pairwise.
            */
	    if (largest_partition <= 2)
		la_count = 2;
	    else if (largest_partition < la_count)
                la_count = largest_partition;
        }

	if ((global->ops_cb->ops_check		/* more possible displays */
	    &&
	    opt_strace( global->ops_cb, OPT_F081_DUMPENUM)))
	{
	    STATUS	csstat;
	    TIMERSTAT	timer_block;
	    i4		cpudiff;

	    csstat = CSstatistics(&timer_block, (i4)0);
	    if (csstat == E_DB_OK)
		cpudiff = timer_block.stat_cpu - 
				global->ops_estate.opn_startime;
	    else cpudiff = 0;

	    TRformat(opt_scc, (i4 *)global,
		(char *)&global->ops_trace.opt_trformat[0],
		(i4)sizeof(global->ops_trace.opt_trformat), "Iemap:  ");
	    for (i = 0; i < 10 && iemap[i] >= 0; i++)
	     TRformat(opt_scc, (i4 *)global,
		(char *)&global->ops_trace.opt_trformat[0],
		(i4)sizeof(global->ops_trace.opt_trformat),
		" %d ", iemap[i]);
	    TRformat(opt_scc, (i4 *)global,
		(char *)&global->ops_trace.opt_trformat[0],
		(i4)sizeof(global->ops_trace.opt_trformat), "\n\n");
	    TRformat(opt_scc, (i4 *)global,
		(char *)&global->ops_trace.opt_trformat[0],
		(i4)sizeof(global->ops_trace.opt_trformat),
		"EVAR dump at degree:%d lac:%d   %d partitions, largest %d   cpu:%d\n\n",
                ert_degree, la_count, num_partitions, largest_partition, cpudiff);

	    opt_evp(evarp, /* (ert_degree > 0) ? ert_degree :*/ evarcount);
	}
	firstcomb = TRUE;
	subquery->ops_bestfragco = (OPO_CO *) NULL;
	subquery->ops_bestco = (OPO_CO *) NULL;
	subquery->ops_cost = OPO_LARGEST;
	global->ops_gmask &= ~(OPS_BFPEXCEPTION|OPS_FLINT|OPS_FLSPACE);
	subquery->ops_fraginperm = FALSE;
	subquery->ops_bestwholetree = FALSE;
	if (ert_degree == la_count && !OJsubset)
	    subquery->ops_lastlaloop = TRUE;	/* flag last time through */

        /* Ensure that there are at least la_count distinct combinations
        ** available.  In particular, a replacement index and its base
        ** table count as one "thing" for combining purposes, because
        ** a combination with a base table and its replacement index is
        ** not valid.  So, for example (assuming la_count == 3), if
        ** we have 4 items X T rI1 rI2, where X=anything, T=some base
        ** table, and rI = replacement indexes on that base table, there
        ** are in effect only 2 distinct items (X, and one of T rI1 rI2),
        ** and so we must reduce la_count to get a valid combination.
        **
        ** Another similar situation occurs when multiple ordinary (placement)
        ** indexes exist;  T I1 I2 only has valid two-way combinations,
        ** not three-way combinations.  So only consider one placement
        ** index per base table.
        **
        ** FIXME there's no #define for largest la_count allowed, it's
        ** hardcoded as 5 in various places.  Including here.
	*/
        if (la_count > 2 && la_count <= 5)
        {
	    bool        regindex[5];
            i4          num_baseno, num_composite, num_regindex;
            OPN_EVAR    *evp;
            OPV_VARS    *varp;
	    OPV_IVARS   baseno[5];
	    OPV_IVARS   tabno;
 
            num_baseno = num_composite = num_regindex = 0;
            /* Usage note, num_regindex is the number of TRUE regindex
            ** entries.  It's not the next regindex entry to use.
            */
            for (i=0; i<5; i++)
                regindex[i] = FALSE;
 
	    for (i = 0; i < ert_degree; i++)
	    {
		evp = &evarp[iemap[i]];
		if (evp->opn_evmask & OPN_EVCOMP)
		{
		    ++num_composite;
		}
		else
		{
                    tabno = evp->u.v.opn_varnum;
                    varp = subquery->ops_vars.opv_base->opv_rt[tabno];
                    if (varp->opv_mask & OPV_CPLACEMENT)
                    {
                        /* Only allow ONE placement index per base table.
                        ** A combination like T x1 x2 won't get thru the
                        ** standard enumeration heuristics.
                        ** Search for index to known base table, or enter
                        ** base table in the tracking array if not seen yet.
			*/
                        tabno = varp->opv_index.opv_poffset;
                        for (j=0; j<num_baseno; j++)
			{
                            if (tabno == baseno[j])
			    {
                                if (!regindex[j])
				{
                                    regindex[j] = TRUE;
                                    ++num_regindex;
                                }
                                break;
                            }
                        }
                        if (j >= num_baseno)
			{
                            /* New placement index, record base table too */
                            baseno[num_baseno++] = tabno;
                            regindex[num_baseno] = TRUE;
                            ++num_regindex;
                        }
                    }
                    else
                    {
                        /* Pick up base table if it's a replacement index */
                        if (varp->opv_mask & OPV_CINDEX)
                            tabno = varp->opv_index.opv_poffset;
                        /* record base table number in tracking array; ignore
                        ** if we already have it.
			*/
                        for (j=0; j<num_baseno; j++)
                            if (tabno == baseno[j])
                                break;
                        if (j >= num_baseno)
                        {
                            /* New table or replacement index, record it */
                            baseno[num_baseno++] = tabno;
			}
                    }
                }
                if (num_composite + num_baseno + num_regindex >= la_count)
                    break;
            }
            /* Normal case will break out of the loop early.  If the loop
            ** exits normally, we didn't have enough distinct combinations,
            ** so readjust la_count.
            */
            if (i == ert_degree)
	    {
                la_count = num_composite + num_baseno + num_regindex;
                if ((global->ops_cb->ops_check  /* more possible displays */
                    &&
                    opt_strace( global->ops_cb, OPT_F081_DUMPENUM)))
                {
                    TRformat(opt_scc, (i4 *)global,
                        (char *)&global->ops_trace.opt_trformat[0],
                        (i4)sizeof(global->ops_trace.opt_trformat),
                        "la_count reduced to %d; nb=%d, nc=%d, ni=%d\n\n",
                        la_count, num_baseno, num_composite, num_regindex);
		}
	    }
	}

        /* Done checking lookahead restrictions, make sure everyone is using
        ** the la-count we decided on...
        */
        subquery->ops_lacount = la_count;

	/* Init. combination vector. */
	for (i = 0; i < la_count; i++) cvector[i] = i;
	cvector[i++] = ert_degree;		/* init last 2 for comb alg. */
	cvector[i] = 0;

        /* Decide whether to allow disconnected (cart-prod) combinations
        ** in combgen.  Normally we disallow them entirely.
        ** We allow them if it's necessary, or if the search is
        ** sufficiently well along and trace point OP180 is set (allowing
        ** cart-prod's in the query plan search space).
        ** This translates to:
        ** - allow cart-prod if largest_partition is < la_count, ie any
        **   combination is going to have to include a cart-prod;
        ** - otherwise, only allow is ert-degree is 5 or less, and OP180
        **   is set.
	**
	** Also allow CPs if we've already tried through without allowing
	** them and found nothing suitable (first_noplan) (kibro01) b122001
        */
 
        allowCPs = FALSE;
        if (largest_partition < la_count
          || (!first_noplan)
          || (global->ops_cb->ops_check
              && opt_strace(global->ops_cb, OPT_F052_CARTPROD)) )
            allowCPs = TRUE;

	bestix[0] = bestix[1] = bestix[2] = -1;

	/* Loop 2: generate all k-combinations from current enumeration
	** set. */
	while (opn_combgen(subquery, cvector, evarp, iemap, ert_degree, 
			la_count, &firstcomb, allowCPs))
	{
	    firstperm = TRUE;
	    MEfill(sizeof(OPE_BMEQCLS), 0, (char *)&eqcmap);
	    MEfill(sizeof(OPV_BMVARS), 0, (char *)&pvarmap);

	    /* Init. permutation vector and combination's eqcmap. Note,
	    ** the permutation vector entries are the actual enumeration 
	    ** range table indexes, whereas the combination vector contains
	    ** 0, 1, 2, ..., later to be mapped to the range table using
	    ** the contents of iemap. */
	    for (i = 0; i < la_count; i++) 
	    {
		pvector[i] = j = iemap[cvector[i]];
		BTor(subquery->ops_eclass.ope_ev, (char *)&evarp[j].opn_eqcmap,
		    (char *)&eqcmap);
		BTand(subquery->ops_eclass.ope_ev, (char *)&evarp[j].opn_snumeqcmap,
		    (char *)&eqcmap);		/* eliminate subselect result 
						** EQCs from target EQC list */
		BTor(varcount, (char *)&evarp[j].opn_varmap, 
		    (char *)&pvarmap);
	    }

	    /* Must loop over bits in pvarmap and change bits for index vars 
	    ** to corresponding primary vars for following OJ tests. */
	    for (i = -1; (i = BTnext(i, (char *)&pvarmap, varcount)) >= 0; )
	     if (i >= subquery->ops_vars.opv_prv)
	    {
		OPV_IVARS	pvar = subquery->ops_vars.opv_base->
					opv_rt[i]->opv_index.opv_poffset;

		BTclear(i, (char *)&pvarmap);
		BTset(pvar, (char *)&pvarmap);
	    }

	    /* If there are outer joins in this permutation, add OJ eqc for
	    ** each into the eqcmap. */
	    if (lbase)
	     for (i = 0; i < subquery->ops_oj.opl_lv; i++)
	    {
		OPV_BMVARS	tempvmap;
		OPV_IVARS	innervar;
		OPE_IEQCLS	ojeqcls;
		ojp = lbase->opl_ojt[i];

		if (ojp->opl_type != OPL_LEFTJOIN &&
			ojp->opl_type != OPL_RIGHTJOIN &&
			ojp->opl_type != OPL_FULLJOIN)
		    continue;

		/* Now set bits from OJs. */
		if (BTsubset((char *)ojp->opl_bvmap, (char *)&pvarmap,varcount))
		    BTor(subquery->ops_eclass.ope_ev, 
				(char *)ojp->opl_innereqc, (char *)&eqcmap);
	    }
	    subquery->ops_lawholetree = FALSE;

	    /* Loop 3: generate all permutations of current combination. */
	    while (opn_permgen(subquery, pvector, evarp, la_count, &firstperm))
	    {
		/* May at some time insert another loop for different 
		** degree k binary rtree shapes. For now, call function to 
		** assign leaf nodes to degree k left deep join tree. This
		** function then applies join tree heuristics. If it returns
		** TRUE, we can immediately call opn_ceval to build a plan. */
		jtreep = freep = global->ops_estate.opn_sroot;
		opn_treeshape(&freep, la_count);
					/* generate requested tree shape */
		subquery->ops_jtreep = jtreep->opn_child[0]; 
					/* JTREE anchor of "bottom left" */
		pindex = 0;

		if (!opn_jtreegen(subquery, evarp, &jtreep, 
					pvector, &pindex, &freep))
		    continue;

		/* Calls to opn_jintersect and opn_jmaps apply further 
		** trimming heuristics. First reset various things for outer
		** join queries. */
        	if (subquery->ops_oj.opl_base)
		{
	    	OPL_IOUTER	    oj;		/* traverse oj descriptors */

	    	    /* reset outer join values in join tree */
	    	    subquery->ops_oj.opl_smask |= OPL_OJCLEAR;
	    	    opn_cjtree(subquery, jtreep);
	    	    subquery->ops_oj.opl_smask &= (~OPL_OJCLEAR);

	    	    for (oj = subquery->ops_oj.opl_lv; --oj >= 0;)
	    	    {
			subquery->ops_oj.opl_base->opl_ojt[oj]->opl_mask &= 
								(~OPL_FULLOJFOUND);
			subquery->ops_oj.opl_base->opl_ojt[oj]->opl_nodep = NULL;
	    	    }
		}
		if (!opn_jintcaller(subquery, jtreep, &sigstat))
		    continue;			/* if FALSE, loop for next perm */
		if (subquery->ops_oj.opl_lv > 0)
		    MEfill(sizeof(OPL_BMOJ), 0, (char *)&tempmap);
		else ojmap = NULL;		/* prepare for jmaps call */
		if (!opn_jmaps(subquery, jtreep, ojmap))
		    continue;			/* if FALSE, loop for next perm */

		/* Finally, do the opn_ceval call to generate CO-tree. */
		subquery->ops_laeqcmap = &eqcmap;
		sigstat = opn_ceval(subquery);
		if (sigstat == OPN_SIGEXIT)
		    return(sigstat);

		/* Process the resulting plan (save the cheapest, etc.). */
		if (subquery->ops_mask & OPS_NEWBESTFRAG) 
		{
		    subquery->ops_mask &= ~OPS_NEWBESTFRAG;
		    bestix[0] = pvector[0];
		    bestix[1] = pvector[1];
		    bestix[2] = pvector[2];
		}

	    }	/* end of permutation loop */

            /* b122530: 
            ** break from the combination loop if we got a plan fragment 
            ** for a OPS_IDIOT_NOWHERE query. Not worth to spend too much
            ** on it.
            */
            if (subquery->ops_mask & OPS_IDIOT_NOWHERE &&
                subquery->ops_bestfragco)
                break; 
                
	}	/* end of combination loop */

	/* Upon exit from the combination loop, we should have a plan fragment for
	** the best n-table join from this iteration through the enumeration range
	** table. The CO-tree fragment is from the bottom left join and it will now
	** replace the 1st of the 2 enumeration range table entries it joins. The
	** 2nd of the entries will simply be removed (following entries will be 
	** copied over top). */

	/* Test for presence of a plan fragment. If there isn't one, we're in 
	** really big trouble! */
	if (FALSE && global->ops_cb->ops_check		/* but first check for dump */
	    &&
	    opt_strace( global->ops_cb, OPT_F081_DUMPENUM))
	    opt_enum();

	if (subquery->ops_bestfragco == (OPO_CO *) NULL)
	{
	    if (first_noplan)
	    {
		/* If no plan is returned, clear memory and try once more. 
                ** This isn't an error, exactly, especially if outer joining.
                ** While most non-OJ causes of this have been fixed, some
                ** unfortunate combination of chosen fragments and cached
                ** subtrees could perhaps cause a no-plan.  As a safety net,
                ** retry once.
                */
		first_noplan = FALSE;
		if (OJsubset && la_count == 3)
		{
		    /* Might just not be able to find a plan with
		    ** all 3 remaining tables in OJ subset. */
		    subquery->ops_lacount = la_count = 2;
		}
		opn_recover(subquery);
		ert_degree++;
                evarcount++;
		continue;
	    }

	    /* Couldn't find one. Send a message and return to let it
	    ** try the old enumeration technique. */
	    if (global->ops_cb->ops_check &&
		opt_svtrace( global->ops_cb, OPT_F119_NEWENUM, &temp, &temp))
	     opx_error(E_OP04C0_NEWENUMFAILURE);
						/* if tp op247 on, just
						** fail the query */
	    opx_rerror(global->ops_caller_cb, E_OP04C0_NEWENUMFAILURE);
	    return(OPN_SIGEXIT);		/* otherwise return to try 
						** the old way */
	}

	sigstat = OPN_SIGOK;
	if (ert_degree == la_count)
	{
	    doneCO = TRUE;			/* last iteration, we must have 
						** complete plan */
	} else if ( (fvarcount + la_count - ert_degree) >= base_table_count)
	{
	    /* If the total variables done plus the possible number done in
	    ** this loop is within range of completing the base tables, which
	    ** is the minimum number after which we should check for plan done.
	    ** Previously this was checking against total objects, not basetabs.
	    ** (kibro01) b121896
	    */
	    doneCO = opn_fullplan(subquery, &primset);
	}

	if (doneCO && final)
	{
	    if (FALSE && global->ops_cb->ops_check
		&&
		opt_strace( global->ops_cb, OPT_F081_DUMPENUM))
	    {
		/* Removing the FALSE above adds a very useful bit 
		** of debugging info to the op209 output. */
		bool	init = FALSE;
		char	temp[OPT_PBLEN + 1];

		if (global->ops_cstate.opc_prbuf == NULL)
		{
		    /* Set up print buffer, if necessary. */
		    init = TRUE;
		    global->ops_cstate.opc_prbuf = temp;
		}

		opt_fcodisplay(global, subquery, subquery->ops_bestco);
		if (init)
		    global->ops_cstate.opc_prbuf = NULL;
	    }

	    break;				/* whole plan done - skip the 
						** rest. Note: final copy of
						** bestco to PERM is done in
						** exception code of opnenum */
	}

	if (!subquery->ops_fraginperm || doneCO)
	{
	    OPO_CO	*tempco;

	    /* Must copy fragment from this loop iteration to permanent
	    ** memory (if not already there). If last iteration of neweplan
	    ** call, it'll be in ops_bestco. In either event, result addr 
	    ** goes into ops_bestfragco. */
	    if (doneCO)
		tempco = subquery->ops_bestco;
	    else tempco = subquery->ops_bestfragco;
	    opo_copyfragco(subquery, &tempco, TRUE);
	    subquery->ops_bestfragco = tempco;
	}

	if (la_count == 2)
	    bestix[2] = -1;

	i = bestix[0];				/* save enumeration rt indexes */
	j = bestix[1];
	k = bestix[2];

	if (i > j)
	{
	    /* Arrange so that higher range number combines into lower. */
	    i = j;
	    j = bestix[0];
	}

	/* If this is the last iteration for an outer join subset - there
	** may be 3 evar entries to consolidate. So make sure they're 
	** sorted. Also, if it is a plan fragment in which a placement
	** index is the 2nd leaf (not the 1st), the whole tree must be
	** saved. */
	/* threeleaf must never be set if we only have 2 elements (kibro01) */
	threeleaf = (la_count > 2) && (doneCO || subquery->ops_bestwholetree);

	if (threeleaf && ert_degree >= 3 && k >= 0)
	{
	    if (i > k)
	    {
		temp = i;
		i = k;
		k = temp;
	    }
	    if (j > k)
	    {
		temp = j;
		j = k;
		k = temp;
	    }
	}
		
	bestix[0] = i;				/* stick back into bestix */
	bestix[1] = j;
	bestix[2] = k;
	*rsltix = i;				/* in case CO frag is done */

	/* Check for composite entries being consolidated into other 
	** composites. CO-nodes of old composites must be flagged un-held
	** for PERM memory management. */
	if (evarp[i].opn_evmask & OPN_EVCOMP)
	    opn_clearheld(evarp[i].u.opn_cplan);
	if (evarp[j].opn_evmask & OPN_EVCOMP)
	    opn_clearheld(evarp[j].u.opn_cplan);
	if (threeleaf && ert_degree >= 3 && k >= 0 &&
				evarp[k].opn_evmask & OPN_EVCOMP)
	    opn_clearheld(evarp[k].u.opn_cplan); /* only if OJ subset */
	
	/* Build list of indexes/primaries eliminated by any new table/indexes
	** included in this composite. Once a primary is in a composite, no
	** replacement index for it may be considered. Likewise, once a 
	** replacement index is included, the corresponding primary may not
	** be considered. */
	MEfill(sizeof(OPV_BMVARS), 0, (char *)&ixmap);
	if (evarp[i].opn_evmask & OPN_EVTABLE)
	    BTor(varcount, (char *)&evarp[i].u.v.opn_ixmap, (char *)&ixmap);
	if (evarp[j].opn_evmask & OPN_EVTABLE)
	    BTor(varcount, (char *)&evarp[j].u.v.opn_ixmap, (char *)&ixmap);
	if (threeleaf && ert_degree >= 3 && k >= 0 &&
				evarp[k].opn_evmask & OPN_EVTABLE)
	    BTor(varcount, (char *)&evarp[k].u.v.opn_ixmap, (char *)&ixmap);

	/* Merge info in entries about to be combined. */
	BTor(varcount, (char *)&evarp[j].opn_varmap, (char *)&evarp[i].opn_varmap);
	BTor(varcount, (char *)&evarp[j].opn_primmap, (char *)&evarp[i].opn_primmap);
						/* merge varmaps */
	BTor(varcount, (char *)&evarp[j].opn_joinable, (char *)&evarp[i].opn_joinable);
						/* merge join maps */
	BTor(subquery->ops_eclass.ope_ev, (char *)&evarp[j].opn_eqcmap, 
	    (char *)&evarp[i].opn_eqcmap);	/* merge eqc maps */
	BTand(subquery->ops_eclass.ope_ev, (char *)&evarp[j].opn_snumeqcmap,
	    (char *)&evarp[i].opn_snumeqcmap);	/* merge anti-eqc maps */
	evarp[i].u.opn_cplan = subquery->ops_bestfragco;
						/* save CO-tree fragment */
	evarp[i].opn_evmask &= ~OPN_EVTABLE;	/* show as composite */
	evarp[i].opn_evmask |= OPN_EVCOMP;

	/* If last iteration for a OJ subset, combine 3rd entry, too. */
	if (threeleaf && ert_degree >= 3 && k >= 0)
	{
	    BTor(varcount, (char *)&evarp[k].opn_varmap, 
					(char *)&evarp[i].opn_varmap);
	    BTor(varcount, (char *)&evarp[k].opn_primmap, 
					(char *)&evarp[i].opn_primmap);
						/* merge varmaps */
	    BTor(varcount, (char *)&evarp[k].opn_joinable, 
					(char *)&evarp[i].opn_joinable);
						/* merge join maps */
	    BTor(subquery->ops_eclass.ope_ev, (char *)&evarp[k].opn_eqcmap, 
		(char *)&evarp[i].opn_eqcmap);	/* merge eqc maps */
	    BTand(subquery->ops_eclass.ope_ev, (char *)&evarp[k].opn_snumeqcmap, 
		(char *)&evarp[i].opn_snumeqcmap); /* merge anti-eqc maps */
	}

	if (FALSE && (global->ops_cb->ops_check	/* display CO details */
	    &&
	    opt_strace( global->ops_cb, OPT_F081_DUMPENUM)))
	{
	    /* Removing the FALSE above adds a very useful bit 
	    ** of debugging info to the op209 output. */
	    bool	init = FALSE;
	    char	temp[OPT_PBLEN + 1];

	    if (global->ops_cstate.opc_prbuf == NULL)
	    {
		/* Set up print buffer, if necessary. */
		init = TRUE;
		global->ops_cstate.opc_prbuf = temp;
	    }

	    opt_fcodisplay(global, subquery, (doneCO) ? subquery->ops_bestco :
				subquery->ops_bestfragco);
	    if (init)
		global->ops_cstate.opc_prbuf = NULL;
	}

	/* Copy range table entries following j on top of it (effectively 
	** decrementing enumeration range table size in preparation for next 
	** iteration). We also update output evar map for caller. */
	for (j = bestix[1]; j < evarcount; j++)
	{
	    MEcopy((char *)&evarp[j+1], sizeof(OPN_EVAR), (char *)&evarp[j]);
	    if (!final)
		oemap[j] = oemap[j+1];
	}
	evarp[j].opn_evmask = OPN_EVDONE;	/* flag last as condensed */

	/* Do again for k, if last iteration of OJ subset. */
	if (ert_degree >= 3 && k >= 0 && threeleaf)
	{
	    for (k = bestix[2]-1; k < evarcount; k++)
	    {
		MEcopy((char *)&evarp[k+1], sizeof(OPN_EVAR), (char *)&evarp[k]);
		if (!final)
		    oemap[k] = oemap[k+1];
	    }
	    evarp[k].opn_evmask = OPN_EVDONE;	/* flag last as condensed */
	    j = k;
	}
	else j = bestix[1];

	/* Enumeration range table map (for OJs) must now be altered to match
	** condensed range table. */
	if (!identity)
	{
	    for (i = 0; i < ert_degree; i++)
	     if (iemap[i] == j) 
		break;				/* stop at consolidation point */
	    for (; iemap[i] >= 0; i++)
		iemap[i] = --iemap[i+1];	/* and condense the map */
	    if (iemap[i] < 0)
		iemap[i] = -1;
	}

	/* Eliminate range table entries dictated by ixmap. */
	ixfolded = FALSE;
	for (i = -1; (i = BTnext(i, (char *)&ixmap, varcount)) >= 0; )
	{
	    ixfolded = TRUE;			/* there was at least one */
	    /* Again, must also condense the enumeration range table map. */
	    if (!identity)
	    {
		for (j = 0; j < OPN_MAXLEAF && iemap[j] >= 0; j++)
		 if ((evarp[iemap[j]].opn_evmask & OPN_EVTABLE) && 
					i == evarp[iemap[j]].u.v.opn_varnum)
		    break;
		for (; j < OPN_MAXLEAF && iemap[j] >= 0; j++)
		    iemap[j] = --iemap[j+1];
		if (iemap[j] < 0)
		    iemap[j] = -1;
	    }

	    /* Then condense the range table itself, as well as the output
	    ** range table map. */
	    for (j = 0; j < evarcount; j++)
	     if ((evarp[j].opn_evmask & OPN_EVTABLE) && 
				i == evarp[j].u.v.opn_varnum)
	     {
		for (k = j; k < evarcount-1; k++)
		{
		    MEcopy((char *)&evarp[k+1], sizeof(OPN_EVAR),
							(char *)&evarp[k]);
		    if (!final)
			oemap[k] = oemap[k+1];
		}
		evarp[k].opn_evmask = OPN_EVDONE; /* flag last as condensed */

		    ert_degree--;
		evarcount--;
		break;
	     }
	}

	/* If any entries were eliminated, remove them from all opn_joinable's. */
	if (ixfolded)
	{
	    BTnot(varcount, (char *)&ixmap);
	    for (i = 0; i < evarcount; i++)
		BTand(varcount, (char *)&ixmap, (char *)&evarp[i].opn_joinable);
	}

	first_noplan = TRUE;		/* reset flag */

	if (threeleaf)
	{
	    /* Decrement once more for three-leaf. */
	    ert_degree--;
	    evarcount--;
	    if (ert_degree <= la_count)
		subquery->ops_lacount = --la_count;
						/* last pass added 2 vars and
						** left final var out in the 
						** cold. One more loop */
	}

	if (doneCO)
	    break;				/* plan frag is done - exit */
    }		/* end of enumeration set degree loop */

    /* If we get here, just return. */
    subquery->ops_lacount = old_lacount;	/* restore in case of 2 tab */
    if ((global->ops_cb->ops_check		/* more possible displays */
	&&
	opt_strace( global->ops_cb, OPT_F081_DUMPENUM)))
    {
	STATUS		csstat;
	TIMERSTAT	timer_block;
	i4		cpudiff;

	csstat = CSstatistics(&timer_block, (i4)0);
	if (csstat == E_DB_OK)
	    cpudiff = timer_block.stat_cpu - global->ops_estate.opn_startime;
	else cpudiff = 0;

	TRformat(opt_scc, (i4 *)global,
	    (char *)&global->ops_trace.opt_trformat[0],
	    (i4)sizeof(global->ops_trace.opt_trformat),
	    "EVAR dump at degree:%d - end of fragment\tcpu: %d\n\n", 
	    ert_degree, cpudiff);
	opt_evp(evarp, evarcount);
    }

    if (sigstat == OPN_SIGOK && !opn_fullplan(subquery, &primset))
    {
	/* Consistency check failed - plan doesn't cover all the 
	** tables it was supposed to. This isn't supposed to happen
	** and indicates some sort of problem with greedy enumeration. */
	subquery->ops_bestco = (OPO_CO *) NULL;
	subquery->ops_cost = OPO_LARGEST;
	if (global->ops_cb->ops_check &&
	    opt_svtrace( global->ops_cb, OPT_F119_NEWENUM, &temp, &temp))
	    opx_error(E_OP04C1_GREEDYCONSISTENCY);
	opx_rerror(global->ops_caller_cb, E_OP04C1_GREEDYCONSISTENCY);
	return(OPN_SIGEXIT);
    }

    return(sigstat);
}

/*{
** Name: opn_newenum	- drive bottom up join order enumerator
**
** Description:
**	The original Ingres join order enumeration technique is 
**	top down with an exhaustive search through all possible 
**	tree shape/primary table & index combination/leaf node 
**	permutations that takes prohibitive CPU time to complete 
**	for even moderately complex queries (10 or more tables/indexes).
**
**	The new join order enumeration technique uses a bottom up
**	approach that only examines joins with n tables/indexes (where
**	n is 3, 4 or 5, depending on conditions). It iterates over this
**	process, successively replacing pairs of tables/indexes with
**	the lowest cost join, until the whole plan has been constructed.
**	for large numbers of tables, this technique can be many orders of 
**	magnitude cheaper to run.
**
**	This function is the main driver of the new join order
**	enumerator. It starts by preparing a variety of structures
**	including the enumeration range table whose entries may be 
**	either base tables/indexes or intermediate composite plan fragments
**	of the evolving plan, and an array describing outer joins (if
**	there are any in the query) so that the plan fragments will be
**	constructed in a sequence that allows an overall plan to be 
**	built.
**
**	After the preparation phase, a loop is executed to build plan
**	fragments for each outer join input needing to be pre-built (if
**	such a requirement exists). The final (or only) loop iteration
**	builds the eventual query plan.
**
** Inputs:
**      subquery                        ptr to current subquery being analyzed
**
** Outputs:
**	Returns:
**	    OPN_SIGOK if a plan was successfully generated
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	6-sep-02 (inkdo01)
**	    Written.
**	12-nov-02 (inkdo01)
**	    Re-engineered for OJ support.
**	27-jul-04 (inkdo01)
**	    Fine tune separation of SEJOIN atts from the rest (fixes a 889
**	    error in a query with non-correlated subselect join).
**	25-oct-04 (inkdo01)
**	    Reenable code to add secondary indexes to OJ subsets. It isn't
**	    clear why it was disabled.
**	30-oct-04 (inkdo01)
**	    Allocate a bunch of OPO_PERMs at the start for fragment stge.
**	17-jan-05 (inkdo01)
**	    Update ixmap to assure we drop all ixes on a table when it or
**	    a replacement index are folded into a composite.
**	6-july-05 (inkdo01)
**	    Add call to opn_modcost() to alter estimates for agg sqs.
**	30-Jun-2006 (kschendel)
**	    Remove the above, done at a later point now.
**      20-may-2005 (huazh01)
**          check the boundary of iemap[] before terminate
**          them with '-1'. This prevents errors such SEGV, stack 
**          overflow or other exceptions. 
**          INGSRV3310, b114536.
**	7-jan-2008 (dougi)
**	    Remove code that sets plan fragment size to other than 3 - didn't
**	    offer much improvement and op247 value has been stolen to control
**	    number of range table entries used to trigger greedy heuristic.
**      11-Mar-2008 (kschendel)
**          Fix misplaced init of gotone flag, needs to be false if no OJ.
**          (Probably no harm done as ojcount ought to be zero.)
**      10-Nov-2008 (kschendel)
**          Include secondaries with OJ subsets even if there are only two
**         primaries.  (original code required i>=3 for some reason.)
**	16-Dec-2008 (kibro01) b119685/118962
**	    When evaluating a full outer join, it isn't just necessary to
**	    reduce the left hand (inner) side, it can be necessary to reduce
**	    both left and right sides if they contain inner joins or other
**	    full joins.
**	1-Jun-2009 (kibro01) b122115
**	    Remove the check for OPS_CPLACEMENT, since you still need the
**	    logic for copying the ixmap values from CINDEX to CINDEX (via the
**	    base table) in the event that you don't have any placement indices
**	    at all - only replacement ones (CINDEX).
*/
OPN_STATUS
opn_newenum(
	OPS_SUBQUERY       *subquery,
	i4		   enumcode)
{
    OPN_STLEAVES iemap, oemap;			/* maps cvector entries to enum.
						** range table entries */
    OPS_STATE	*global = subquery->ops_global;
    OPN_JTREE	*jtreep;
    i4		i, j, k, rsltix, ojcount, cocount;
    OPN_EVAR	*evarp;				/* address of OPN_EVAR vector */
    i4		varcount = subquery->ops_vars.opv_rv;
    i4		la_count;			/* no. tables in each join */
    OPV_RT	*vbase = subquery->ops_vars.opv_base;
    OPL_OJT	*lbase = subquery->ops_oj.opl_base;
    OPV_VARS	*varp;
    OPL_OUTER	*ojp;
    OPZ_ATTS	*attrp;
    OPO_PERM	*permp;
    OPN_STATUS	sigstat;
    JDESC	jdesc[OPL_MAXOUTER+2] = {0}; /* outer join descriptor stuff */
    bool	ojanal = FALSE, done;
    bool	p1_reduce;
    bool	p2_reduce;


    /* Lots of initialization to do. Allocate memory for OPN_EVAR vector &
    ** initialize entries from subquery range table. */

    subquery->ops_mask |= OPS_LAENUM;		/* indicate use of new algorithm */
/*  subquery->ops_lacount = (enumcode >= 3 && enumcode <= 5) ? enumcode : 3; */
						/* deprecate variable fragment
						** tree size code */
    la_count = subquery->ops_lacount = 3;	/* number of tables in jtree */

    evarp = (OPN_EVAR *)opu_memory (global, (varcount+1)*sizeof(OPN_EVAR));
    for (i = 0; i < varcount; i++)
    {
	varp = vbase->opv_rt[i];
	evarp[i].u.v.opn_varp = varp;
	evarp[i].u.v.opn_varnum = i;
	MEfill(sizeof(OPV_BMVARS), 0, (char *)&evarp[i].opn_joinable);
	for (j = 0; j < varcount; j++)	/* loop over joinable array setting bits
					** though not for variable i, itself */
	{
	    if (varp->opv_joinable.opv_bool[j] && i != j)
	    {
		OPV_VARS	*var1p = vbase->opv_rt[j];

		/* Joinable entries are copied when both are 
		** primaries, one is primary and the other a replacement
		** index, or one is primary and the other a placement
		** index for that primary. This prevents us from 
		** trying plans with the index in a different branch 
		** from the corresponding primary (a jintersect check). **
		if ((i < subquery->ops_vars.opv_prv &&
		    j < subquery->ops_vars.opv_prv) ||
		    (j < subquery->ops_vars.opv_prv &&
		    varp->opv_mask & OPV_CINDEX) ||
		    (i < subquery->ops_vars.opv_prv &&
		    var1p->opv_mask & OPV_CINDEX) ||
		    (varp->opv_mask & OPV_CPLACEMENT &&
		    varp->opv_index.opv_poffset == j) ||
		    (var1p->opv_mask & OPV_CPLACEMENT &&
		    var1p->opv_index.opv_poffset == i)) */
		BTset(j, (char *)&evarp[i].opn_joinable);
	    }
	}
	MEfill(sizeof(OPV_BMVARS), 0, (char *)&evarp[i].u.v.opn_ixmap);
	if (varp->opv_mask & OPV_CINDEX)
	{
	    /* This is a replacement index - set corr. primary in its
	    ** ixmap and set it in primary's ixmap. */
	    BTset((j = varp->opv_index.opv_poffset), 
					(char *)&evarp[i].u.v.opn_ixmap);
	    BTset(i, (char *)&evarp[j].u.v.opn_ixmap);
	}
	else if (varp->opv_mask & OPV_CPLACEMENT)
	{
	    /* This is a non-replacement secondary index, set it in
	    ** primary's ixmap. */
	    BTset(i, (char *)&evarp[varp->opv_index.opv_poffset].u.v.opn_ixmap);
	}
	MEfill(sizeof(OPV_BMVARS), 0, (char *)&evarp[i].opn_varmap);
	MEfill(sizeof(OPV_BMVARS), 0, (char *)&evarp[i].opn_primmap);
	MEfill(sizeof(OPE_BMEQCLS), 0xff, (char *)&evarp[i].opn_snumeqcmap);
	BTset(i, (char *)&evarp[i].opn_varmap);
	if (i >= subquery->ops_vars.opv_prv)
	    BTset(varp->opv_index.opv_poffset, (char *)&evarp[i].opn_primmap);
	 else BTset(i, (char *)&evarp[i].opn_primmap);
					/* primmap is map of primaries corr.
					** to this SQ range entry (for OJs) */
	MEcopy((char *)&varp->opv_maps.opo_eqcmap, sizeof(OPE_BMEQCLS),
	    (char *)&evarp[i].opn_eqcmap);  /* copy map of var's eqc's */
	if (varp->opv_subselect)
	{
	    /* This var represents one or more subselect results. Loop over 'em
	    ** accumulating list of result eqc's to remove from intermediate
	    ** join results. These EQCs never get into the joins at all with
	    ** the old enumeration algorithm. */
	    OPV_SUBSELECT	*subsp;

	    for (subsp = varp->opv_subselect; subsp != (OPV_SUBSELECT *) NULL;
		subsp = subsp->opv_nsubselect)
	    {
		attrp = subquery->ops_attrs.opz_base->opz_attnums[subsp->
						opv_atno];
		if (attrp->opz_attnm.db_att_id == OPZ_SNUM)
						/* limit to subselects
						** (& avoid union views) */
		BTclear(subquery->ops_attrs.opz_base->opz_attnums[
		    subsp->opv_atno]->opz_equcls, 
		    (char *)&evarp[i].opn_snumeqcmap);
	    }
	}
	evarp[i].opn_evmask = OPN_EVTABLE;
    }
    MEfill(sizeof(OPN_EVAR), 0, &evarp[varcount]);	/* marker at end */
    evarp[varcount].opn_evmask = OPN_EVDONE;

    /* One more loop over range table inserting CPLACEMENT indexes into
    ** opn_ixmap of CINDEX indexes on same primary, and CINDEX indexes into
    ** the opn_ixmap of corresponding CPLACEMENT indexes. This allows us to 
    ** avoid compiling some composites with replacement indexes and others with 
    ** the corresponding base table or secondary indexes. */
     for (i = 0; i < varcount; i++)
     {
	varp = vbase->opv_rt[i];
	if (varp->opv_mask & (OPV_CPLACEMENT | OPV_CINDEX))
	{
	    j = varp->opv_index.opv_poffset;
	    for (k = -1; (k = BTnext(k, (char *)&evarp[j].u.v.opn_ixmap, 
			varcount)) >= 0; )
	    {
		BTset(i, (char *)&evarp[k].u.v.opn_ixmap);
		BTset(k, (char *)&evarp[i].u.v.opn_ixmap);
	    }
	    BTclear(i, (char *)&evarp[i].u.v.opn_ixmap);
				/* make sure it isn't on in its own map */
	}
     }

    /* Loop over outer join descriptors (if any), copying info to local
    ** structure array for further analysis. */
    ojcount = subquery->ops_oj.opl_lv;
    if (lbase)
     for (i = 0; i < subquery->ops_oj.opl_lv; i++)
    {
	ojp = lbase->opl_ojt[i];
	jdesc[i].ojtype = ojp->opl_type;
	jdesc[i].ojdrop = FALSE;
	MEcopy((char *)ojp->opl_ivmap, sizeof(OPV_BMVARS), 
					(char *)&jdesc[i].ojivmap);
	MEcopy((char *)ojp->opl_ojtotal, sizeof(OPV_BMVARS), 
					(char *)&jdesc[i].ojtotal);
	jdesc[i].ojivcount = BTcount((char *)ojp->opl_ivmap, varcount);
	if (jdesc[i].ojtype == OPL_FULLJOIN)
	{
	    /* Full join has 2 partitions - opl_p1, opl_p2. They are
	    ** copied to a jdesc ojivmap. If both have more than one table, 
	    ** an extra JDESC will be needed. */
	    MEcopy((char *)ojp->opl_p1, sizeof(OPV_BMVARS),
					(char *)&jdesc[i].ojivmap);
	    MEcopy((char *)ojp->opl_ojtotal, sizeof(OPV_BMVARS), 
					(char *)&jdesc[i].ojtotal);
	    jdesc[i].ojivcount = BTcount((char *)&jdesc[i].ojivmap,
								varcount);
	    jdesc[i].p1_reduced = FALSE;
	    jdesc[i].p2_reduced = FALSE;
	    MEcopy((char *)ojp->opl_p1, sizeof(OPV_BMVARS),
					(char *)&jdesc[i].p1map);
	    MEcopy((char *)ojp->opl_p2, sizeof(OPV_BMVARS),
					(char *)&jdesc[i].p2map);
	    jdesc[i].ojp1count = BTcount((char *)&jdesc[i].p1map, varcount);
	    jdesc[i].ojp2count = BTcount((char *)&jdesc[i].p2map, varcount);

	    if (jdesc[i].ojivcount <= 1)
	    {
		MEcopy((char *)ojp->opl_p2, sizeof(OPV_BMVARS),
					(char *)&jdesc[i].ojivmap);
		MEcopy((char *)ojp->opl_ojtotal, sizeof(OPV_BMVARS), 
					(char *)&jdesc[i].ojtotal);
		jdesc[i].ojivcount = BTcount((char *)&jdesc[i].ojivmap,
								varcount);
	    }
	    else
	    {
		MEcopy((char *)ojp->opl_p2, sizeof(OPV_BMVARS),
					(char *)&jdesc[ojcount].ojivmap);
		MEcopy((char *)ojp->opl_ojtotal, sizeof(OPV_BMVARS), 
					(char *)&jdesc[ojcount].ojtotal);
		jdesc[ojcount].ojivcount = 
			BTcount((char *)&jdesc[ojcount].ojivmap, varcount);
		jdesc[ojcount].ojtype = OPL_FULLJOIN;
		if (jdesc[ojcount].ojivcount > 1)
		{
		    jdesc[ojcount].ojdrop = TRUE;
		    ojcount++;
		}
	    }
	}
	if (jdesc[i].ojivcount <= 1)
	    jdesc[i].ojdrop = TRUE;
	if ((jdesc[i].ojtype == OPL_LEFTJOIN || 
		jdesc[i].ojtype == OPL_RIGHTJOIN ||
		jdesc[i].ojtype == OPL_FULLJOIN) && jdesc[i].ojivcount > 1)
	    ojanal = TRUE;
    }
    if (ojanal)
	ojanal = opn_ojanalyze(subquery, jdesc, ojcount);

    opn_jtalloc(subquery, &jtreep);		/* allocate OPN_JTREE node pool */
    global->ops_estate.opn_sroot = jtreep;
    global->ops_estate.opn_corequired = 3 * varcount;  /* PERM nodes to alloc. 
						** for whole plan */

    /* Count available permanent CO nodes on fragment list. If not enough,
    ** call for more. */
    for (i = 0, permp = global->ops_estate.opn_fragcolist; permp;
			i++, permp = permp->opo_next)
	permp->opo_conode.opo_held = FALSE;	/* reset held flags, too */
    if (i < 6 * varcount)
	permp = opn_cpmemory(subquery, 6 * varcount - i);

    /* Loop to compile composite bits until whole plan is produced. Execute
    ** once for each OJ subset required to be pre-compiled, then once for
    ** the final plan. If no OJs to precompile, just execute once. */
    do {
	OPV_BMVARS ivmap;
	JDESC	*jptr;
	i4	miniv, ecount, pcount;
	bool	gotone, setres;

	subquery->ops_lacount = la_count;	/* reset - OJ subset might
						** have decremented it */

	for (ecount = 0; ecount < varcount && 
		!(evarp[ecount].opn_evmask & OPN_EVDONE); ecount++);
					/* count active evar entries */
	gotone = FALSE;
	if (!ojanal)
	{
	    /* No OJs. Just create the identity mapping. */
	    for (i = 0; i < varcount; i++)
		iemap[i] = i;
            if (i < OPN_MAXLEAF)
                iemap[i] = -1;	/* terminate map with -1 */
	    oemap[0] = -1;	/* show no need to build output map */
	    pcount = -1;
	    done = TRUE;
	}
	else
	{
	    /* Scan OJ descriptors, looking for next subset to reduce. */
	    done = FALSE;
	    miniv = OPV_MAXVAR;
	    for (i = 0; i < ecount; i++)
		oemap[i] = i;
	    oemap[i] = -1;			/* init output map */

	    for (i = 0; i < ojcount; i++)
	     if (( jptr = &jdesc[i])->ojdrop)
		continue;
	     else
	    {
		/* Pick the smallest reductions, first. */
		if (jptr->ojtype == OPL_FULLJOIN)
		{
		    if (!jptr->p1_reduced && jptr->ojp1count < miniv && 
					     jptr->ojp1count > 1)
		    {
			gotone = TRUE;
			p1_reduce = TRUE;
			p2_reduce = FALSE;
			miniv = jptr->ojp1count;
			j = i;
		    }
		    if (!jptr->p2_reduced && jptr->ojp2count < miniv && 
					     jptr->ojp2count > 1)
		    {
			gotone = TRUE;
			p1_reduce = FALSE;
			p2_reduce = TRUE;
			miniv = jptr->ojp2count;
			j = i;
		    }
		} else
		{
		    if (jptr->ojivcount >= miniv)
		        continue;
		    p1_reduce = p2_reduce = FALSE;
		    gotone = TRUE;
		    miniv = jptr->ojivcount;
		    j = i;
		}
	    }

	    if (!gotone)
	    {
		/* Last pass - build identity emap on remaining evars. */
		for (i = 0; !(evarp[i].opn_evmask & OPN_EVDONE); i++)
		    iemap[i] = i;
                if (i < OPN_MAXLEAF)
                   iemap[i] = -1;	/* terminate map with -1 */
		oemap[0] = -1;	/* show last call */
		pcount = -1;
		ojanal = FALSE;
		done = TRUE;
	    }
	    else
	    {
		char *map;
		/* Build emap from evars in OJ subset. */
		jptr = &jdesc[j];		/* reset to min partition */
		pcount = miniv;			/* set frag's prim var count */
		if (jptr->ojtype == OPL_FULLJOIN && (p1_reduce || p2_reduce))
		{
		    if (p1_reduce)
			map = (char*)&jptr->p1map;
		    else
			map = (char*)&jptr->p2map;
		} else
		{
		    map = (char *)&jptr->ojivmap;
		}
		for (i = 0, j = -1; (j = BTnext(j, map, varcount)) >= 0;)
			iemap[i++] = j;

		/* Save ivmap (and inverse) for later. */
		MEcopy(map, sizeof(OPV_BMVARS), (char *)&ivmap);

		/* Tack on any still available potentially useful
		** secondary indexes. This is done by searching the 
		** evar array for table entries matching sq varnos. */
		/* Don't do this for full joins, since the p1 and p2 arrays
		** already contain the secondary indices we might need 
		*/
		if (i >= 2 && (jptr->ojtype != OPL_FULLJOIN))
		{
		  /* If this is a full join, and there are joins below it,
		  ** the list of items to pull down into this preliminary
		  ** join evaluation is only the ones related to the p1 map,
		  ** whereas ojivmap includes the outer half too.
		  ** (kibro01) b119685/118962
		  */
		  char *oj_list = (char *)&jptr->ojtotal;

		  for (j = subquery->ops_vars.opv_prv-1;
			(j = BTnext(j, oj_list, varcount)) >= 0; )
		  {
		    for (k = 0; !(evarp[k].opn_evmask & OPN_EVDONE); k++)
		    {
		      if (evarp[k].opn_evmask & OPN_EVTABLE &&
			  j == evarp[k].u.v.opn_varnum)
		      {
		        /* Found one of them - record evar array index. */
		        iemap[i] = k;
		        i++;		/* next iemap position */
		        break;		/* exit inner loop */
		      }
		    }
		  }
		}
                
		if (i < OPN_MAXLEAF)
                   iemap[i] = -1;	/* terminate map with -1 */

		if (jptr->ojtype == OPL_FULLJOIN)
		{
		    if (p1_reduce)
			jptr->p1_reduced = TRUE;
		    if (p2_reduce)
			jptr->p2_reduced = TRUE;
		    if (jptr->p1_reduced && jptr->p2_reduced)
			jptr->ojdrop = TRUE;
		} else
		{
		    jptr->ojdrop = TRUE;
		}
	    }
	}

	/* Build plan (fragment) from mapped evars. */
	sigstat = opn_neweplan(subquery, evarp, iemap, oemap,
					ecount, pcount, &rsltix);
	if (sigstat != OPN_SIGOK)
	    break;

	/* If we just compiled an OJ subset, the jdesc array must be
	** post-processed so that the ojivmap's of the remaining 
	** entries contain indexes to the modified evar array. */
	if (gotone)
	{
	    for (i = 0; i < ojcount; i++)
	    {
		jptr = &jdesc[i];
		if (jptr->ojdrop)
		    continue;
		
		if (jptr->ojtype == OPL_FULLJOIN)
		{
		    if (!jptr->p1_reduced)
		        opn_reduce_map((char*)&ivmap, (char*)&jptr->p1map,
			    varcount, oemap, ecount, rsltix);
		    if (!jptr->p2_reduced)
		        opn_reduce_map((char*)&ivmap, (char*)&jptr->p2map,
			    varcount, oemap, ecount, rsltix);
		}
		opn_reduce_map((char*)&ivmap, (char*)&jptr->ojivmap,
			varcount, oemap, ecount, rsltix);
	    }
	}
    } while (!done);

    subquery->ops_mask &= ~OPS_LAENUM;			/* extinguish flag */
    return(sigstat);
}


/*{
** Name: opn_reduce_map	- remove items from enumeration maps and shuffle
**
**
** Inputs:
**      elim	- map of items to be eliminated
**	map	- map of items from which things should be eliminated
**	varcount- total count of items
**	oemap	- output enumeration map from opn_neweplan
**	ecount	- length of oemap
**	rsltix	- location of new composite item in map
**
** Outputs:
**	none
**
** Side Effects:
**	none
**
** History:
**	12-Dec-2008 (kibro01)
**	    Written.
**	2-Jan-2009 (kibro01)
**	    Amended to use simpler BTclearmask function.
*/

static void
opn_reduce_map(char *elim, char *map, i2 varcount,
			OPN_STLEAVES oemap, i4 ecount, i4 rsltix)
{
    bool setres = FALSE;
    i4 j,k;
    OPV_BMVARS workmap, tmp_map;

    MEcopy(map, sizeof(OPV_BMVARS), (char *)&tmp_map);
    BTclearmask(varcount, elim, map);

    if (MEcmp(map, (char *)&tmp_map, sizeof(OPV_BMVARS)))
    {
	/* This join overlapped the one just compiled - remove
	** inner join inputs from this join and (later) replace
	** by composite result. */
	setres = TRUE;
    }
    /* Now look at each remaining entry in ojivmap and re-map it using oemap. */
    MEfill(sizeof(OPV_BMVARS), 0, (char *)&workmap);
    for (j = -1; (j = BTnext(j, map, ecount)) >= 0; )
    {
	for (k = 0; k < ecount; k++)
	{
	    if (oemap[k] == j)
		 break;
	}
	if (k >= ecount || oemap[k] == k)
	    continue;		/* skip if unchanged */

	/* Clear old entry and set to mapped entry. 
	** Note: new values are set in work map to be ORed
	** later into ivmap. This way, new bits aren't later
	** extinguished by the same loop. */
	BTclear(j, map);
	BTset(k, (char *)&workmap);
    }
    BTor(varcount, (char *)&workmap, map);

    if (setres)
	BTset(rsltix, map);
}
