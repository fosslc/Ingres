/*
** Copyright (c) 2004, 2010 Ingres Corporation
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
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 


/**
**
**  Name: OPBPQBF.C - find matching boolean factors for partitioned tables
**
**  Description:
**      These routines will build the OPB_PQBF structures for each 
**	partitioned table in the query. They describe boolean factors 
**	that can be used to reduce the number of partitions to process.
**
**  History:    
**      15-jan-04 (inkdo01)
**	    Written using opbmbf/pmbf/jkeylist as guidelines.
**	31-Jul-2007 (kschendel) SIR 122513
**	    Pretty much overhaul for new generalized partition qualification
**	    capabilities.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void opb_pqbf(
	OPS_SUBQUERY *subquery);
static void opb_pqbf_alloc(
	OPS_STATE *global,
	OPV_VARS *varp,
	DB_PART_DEF *partp);
void opb_pqbf_findeqc(
	DB_PART_DEF *partp,
	OPB_PQBF *pqbfp,
	OPE_IEQCLS eqc,
	i4 *dim,
	i4 *col_index);
static bool opb_pqbf_ok(
	OPS_SUBQUERY *subquery,
	DB_PART_DEF *partp,
	OPB_PQBF *pqbfp,
	PST_QNODE *qnode,
	OPE_IEQCLS reqd_eqc);


/*{
** Name: opb_pqbf - find boolean factors for qualifying table partitions
**
** Description:
**      This routine will build the OPB_PQBF structure for each
**	partitioned table in the query.  This structure identifies
**	which boolean factors are reasonable candidates for partition
**	qualification (a.k.a pruning, elimination).  Later, OPC will
**	combine this info with a join or orig node's boolean-factor
**	bitmap to decide where (and if) partition qualification code
**	can be generated.
**
**	This analysis must include all boolean factors that might
**	contribute to partition qualification, but need not exclude
**	all BF's that ultimately will not contribute.  I.e. just
**	because a BF is included in a dimbfm map doesn't mean that
**	it is 100% qualified, it just means that it passes all the
**	tests that are reasonable to apply before enumeration and OPC.
**
**	The PQBF structure also includes information on which
**	partitioning columns are available in the query as equivalence
**	classes.  Not only does that make our analysis simpler, it
**	will help when it's time for OPC to actually generate code
**	to do partition qualification.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    All partitioned primary tables in the subquery range table
**	    will have a OPB_PQBF structure built and attached to the
**	    OPV_VARS structures.  (Unless none of the partitioning
**	    columns are referenced in the query.)
**
** History:
**	15-jan-04 (inkdo01)
**	    Written.
**	13-may-04 (inkdo01)
**	    Add support for between predicates.
**	28-Feb-2005 (schka24)
**	    Add restrictions because the partition qualification in OPC and
**	    QEF is still pretty limited.  a) all columns in a dim must
**	    be there;  b) no LIKE's or similar range-ish operators;
**	    c) no range operators if multiple columns in a dim, unless
**	    the oper is for the last column.
**	3-Nov-2006 (kschendel)
**	    Make sure that a "single" BF is either a two-operand op, or
**	    IS NULL.  I suspect that nothing else can be generated, but
**	    if something slips through it will either barf out opc, or
**	    produce the wrong answer at runtime.
**	31-Jul-2007 (kschendel) SIR 122513
**	    Redo the partition qualification mechanism completely.
**	    OPC/QEF now have a lot more capability for partition qual, so
**	    no longer need partition qual boolfacts to be sargable BF's.
**	    Thus, instead of operating from the bfglobal suggestions
**	    meant for keying, examine the boolfacts for references to
**	    partitioning columns (eqc's) plus the appropriate structure.
*/
VOID
opb_pqbf(
	OPS_SUBQUERY       *subquery)
{
    OPV_IVARS		pvarno;		/* joinop range table number of base
                                        ** relation being analyzed */
    OPV_RT              *vbase;         /* ptr to base of array of ptrs to
                                        ** joinop range table elements */
    OPZ_AT		*abase;		/* base of joinop attributes array */
    OPB_BFT		*bfbase;	/* base of boolean factor array */
    OPV_VARS		*varp;		/* ptr to range var being analyzed */
    OPB_PQBF		*pqbfp;
    DB_PART_DEF		*partp;
    i4			numbf;		/* # of boolfacts in bitmaps */
    i4			numeqc;		/* # eqclasses in subquery */
    i4			i, j, ndims;


    vbase = subquery->ops_vars.opv_base;
    abase = subquery->ops_attrs.opz_base; 
    bfbase = subquery->ops_bfs.opb_base;
    numbf = subquery->ops_bfs.opb_bv;
    numeqc = subquery->ops_eclass.ope_ev;

    for ( pvarno = 0; pvarno < subquery->ops_vars.opv_prv; pvarno++)
    {
	/* Loop over base tables looking for those that are partitioned. */
	varp = vbase->opv_rt[pvarno];
	if (varp->opv_grv->opv_relation == NULL)
	    continue;
	partp = varp->opv_grv->opv_relation->rdr_parts;
	if (partp == (DB_PART_DEF *) NULL || partp->ndims == 0)
	    continue;			/* skip if not partitionined */
	ndims = partp->ndims;

	/* Allocate a PQBF structure to suit this table */
	opb_pqbf_alloc(subquery->ops_global, varp, partp);
	pqbfp = varp->opv_pqbf;
	if (pqbfp == NULL)
	    continue;			/* All dims were auto! */

	/* Prepare info about the partitioning columns:  their eqclasses,
	** and various bitmaps for easy filtering and analysis of
	** boolean factors later on.
	** Note, a partitioning column can only appear once in the
	** table's overall partitioning scheme.  (You never see column
	** c appear in two different dimensions.)  Likewise, the
	** partitioning column -- joinop att -- eqclass mapping is
	** unique.  (Even if there's a predicate t1.c1 = t1.c2, c1 and
	** c2 are assigned two different eqclasses since they come from
	** the same table.)
	*/
	MEfill(sizeof(OPE_BMEQCLS), 0, (PTR) &pqbfp->opb_sdimeqcs);
	MEfill(sizeof(OPE_BMEQCLS), 0, (PTR) &pqbfp->opb_alldimeqcs);
	for (i = 0; i < ndims; i++)
	{
	    /* Loop over dimensions of table partitioning scheme. */
	    DB_PART_DIM	*dimp = &partp->dimension[i];
	    OPE_IEQCLS eqcls[DB_MAXKEYS];
	    OPE_IEQCLS *eqcptr;
	    OPZ_IATTS attno;		/* Joinop attr no */

	    /* Init # of logical partitions selected to "all" */
	    pqbfp->opb_pqlcount[i] = dimp->nparts;

	    if (dimp->distrule == DBDS_RULE_AUTOMATIC)
		continue;		/* skip the "automatic" ones */

	    /* Translate partitioning def'n (DMF) att to joinop att,
	    ** to eqclass.
	    ** Take this opportunity to exclude composite dims
	    ** which do not list all columns at least somewhere in the
	    ** query.  Eventually, we want to allow partial qualification
	    ** for RANGE (and maybe LIST), but for now OPC can't
	    ** generate the necessary >= and <= combinations.
	    */
	    for (j = 0; j < dimp->ncols; j++)
	    {
		attno = opz_findatt(subquery, pvarno,
			dimp->part_list[j].att_number);
		if (attno == OPZ_NOATTR
		  || abase->opz_attnums[attno]->opz_equcls < 0)
		{
		    pqbfp->opb_mdimbfm[i] = NULL;  /* Forget this dim */
		    j = 0;		/* Exit indicator... */
		    break;
		}
		eqcls[j] = abase->opz_attnums[attno]->opz_equcls;
	    }
	    if (j == 0)
		continue;		/* This dim no good */

	    /* Now that we've OK'ed all column(s) of the dim, record the
	    ** eqclasses for the upcoming boolfactor pass
	    */
	    eqcptr = pqbfp->opb_dimeqc[i];
	    for (j = 0; j < dimp->ncols; j++)
	    {
		BTset(eqcls[j], (char *) &pqbfp->opb_alldimeqcs);
		*eqcptr++ = eqcls[j];
	    }
	    if (dimp->ncols == 1)
		BTset(eqcls[0], (char *) &pqbfp->opb_sdimeqcs);
	} /* dim loop */

	/* Skip to next var if no partition col usage at all in this one */
	if (BTcount((char *) &pqbfp->opb_alldimeqcs, numeqc) == 0)
	{
	    varp->opv_pqbf = NULL;	/* Just lose the useless info */
	    continue;
	}

	/* Look through the boolean factor table, and if one is found
	** that references a partitioning column eqclass, subject it
	** to additional analysis to see if it's potentially useful for
	** partition qualification.
	*/
	for (i = 0; i < subquery->ops_bfs.opb_bv; ++i)
	{
	    i4 dim;
	    i4 toss;
	    OPB_BOOLFACT *bfp;
	    OPE_BMEQCLS	bfeqcs;		/* BF eqc's relevant to partitioning */
	    OPE_IEQCLS reqd_eqc;	/* Required eqc for verification */

	    bfp = bfbase->opb_boolfact[i];
	    MEcopy((PTR) &bfp->opb_eqcmap, sizeof(bfp->opb_eqcmap),
			(PTR) &bfeqcs);
	    BTand(numeqc, (char *) &pqbfp->opb_alldimeqcs, (char *) &bfeqcs);
	    if (BTcount((char *) &bfeqcs, numeqc) == 0)
		continue;		/* No intersection with this BF */

	    reqd_eqc = OPE_NOEQCLS;
	    if (! BTsubset((char *) &bfeqcs, (char *) &pqbfp->opb_sdimeqcs,
			numeqc) )
	    {
		/* If BF involves more than just simple dims, it is
		** constrained to use exactly one composite dim column.
		** Checking for a single bit verifies that (and we know
		** that the one eqc can't be a simple dim eqc, else
		** it would have been a subset of sdimeqcs).
		*/
		if (BTcount((char *) &bfeqcs, numeqc) != 1)
		    continue;		/* Can't ref multi columns */
		reqd_eqc = BTnext(-1, (char *) &bfeqcs, numeqc);
	    }

	    /* BF passes preliminary tests, analyze the query fragment
	    ** to make sure that it passes the other requirements.
	    */

	    if (opb_pqbf_ok(subquery, partp, pqbfp, bfp->opb_qnode, reqd_eqc) )
	    {
		/* BF looks good, record it in the proper bitmaps */
		if (reqd_eqc == OPE_NOEQCLS)
		    BTset(i, (char *) pqbfp->opb_sdimbfm);
		else
		{
		    opb_pqbf_findeqc(partp, pqbfp, reqd_eqc, &dim, &toss);
		    BTset(i, (char *) pqbfp->opb_mdimbfm[dim]);
		}
	    }
	} /* end bf loop */

	/* Take one more pass over the dimensions, looking for
	** composite dimensions that don't have boolean factors to
	** cover all columns of the dimension.  At present, we can't
	** do the equivalent of partial-key matching.  Nor does OPC
	** handle join-node situations where some columns of a composite
	** dim are presented by join eqc's and others by BF conditions.
	** (e.g. t1.c1 = part.c1 and part.c2=5 for dimension (c1,c2);
	** ideally we'd handle this but OPC isn't ready.)
	** This still isn't a 100% foolproof test, since there might
	** have been two BF's for the same column.  There's no point
	** in working harder at this stage, though, since we'd just
	** have to check again at OPC time.  (OPC may decide that
	** a boolfact that looked OK here is in fact not usable.)
	**
	** Also, compute the wild-guess number of physical partitions
	** selected by multiplying out.
	*/
	pqbfp->opb_pqcount = 1;
	for (i = 0; i < ndims; i++)
	{
	    i4 lcount, nparts;

	    nparts = partp->dimension[i].nparts;
	    if (pqbfp->opb_mdimbfm[i] != NULL
	      && BTcount((char *) pqbfp->opb_mdimbfm[i], numbf) < partp->dimension[i].ncols)
	    {
		pqbfp->opb_mdimbfm[i] = NULL;	/* scrap it */
		pqbfp->opb_pqlcount[i] = nparts;
	    }
	    lcount = pqbfp->opb_pqlcount[i];
	    if (lcount < 1) lcount = 1;
	    if (lcount > nparts)
		lcount = nparts;
	    pqbfp->opb_pqcount *= lcount;
	}

	/* Don't make the mistake of pruning vars with no BF's marked in
	** the bitmaps.  There's still a possibility of generating join-time
	** qualification against a join eqc;  join equalities aren't in
	** the boolfact table.
	*/

    }	/* end of primary variables loop */

} /* opb_pqbf */

/*
** Name: opb_pqbf_alloc - Allocate PQBF structure for table
**
** Description:
**
**	This routine allocates an OPB_PQBF structure plus space for
**	the necessary related bitmaps and eqc arrays, from opu memory.
**	All the bitmaps are initialized to zero.
**
**	If all partitioning dimensions are AUTOMATIC, meaning that they
**	don't depend on any column values, no PQBF is allocated and
**	opv_pqbf is returned NULL.
**
** Inputs:
**
**	global			Optimizer state (session) CB
**	varp			OPV_VARS * variable info
**	partp			DB_PART_DEF * partition definition ptr
**
** Outputs:
**	varp->opv_pqbf set to new PQBF structure (or NULL)
**	Returns nothing
**
** History:
**	31-Jul-2007 (kschendel) SIR 122513
**	    Written.
*/

static void
opb_pqbf_alloc(OPS_STATE *global, OPV_VARS *varp,
	DB_PART_DEF *partp)
{
    DB_PART_DIM *dimp;			/* Dimension info pointer */
    i4 i,j;
    i4 num_sdims, num_mdims;		/* Num of single- and multi-col dims */
    i4 psize;				/* Memory size needed */
    i4 total_cols;			/* Total partitioning columns */
    OPB_PQBF *pqbfp;
    OPE_IEQCLS *eqcp;
    PTR p;

    num_sdims = num_mdims = 0;
    total_cols = 0;
    varp->opv_pqbf = NULL;

    /* Scan table's partitioning dimensions and count things up.
    ** Don't bother with AUTO dimensions, though.
    */
    for (dimp = &partp->dimension[partp->ndims-1];
	 dimp >= &partp->dimension[0];
	 --dimp)
    {
	if (dimp->distrule != DBDS_RULE_AUTOMATIC)
	{
	    total_cols += dimp->ncols;
	    if (dimp->ncols == 1)
		++num_sdims;
	    else
		++num_mdims;
	}
    }
    if (total_cols == 0)
	return;				/* All were AUTO, stop now */

    /* We need the PQBF itself, plus one OPE_IEQCLS per column,
    ** plus one OPB_BMBF if there were any simple one-column dims, plus
    ** one OPB_BMBF per composite multi-column dim.
    */
    if (num_sdims > 0)
	num_sdims = 1;			/* just zero or one */

    psize = DB_ALIGN_MACRO(sizeof(OPB_PQBF))
		+ sizeof(OPE_IEQCLS) * total_cols
		+ sizeof(OPB_BMBF) * num_sdims
		+ sizeof(OPB_BMBF) * num_mdims;
    pqbfp = varp->opv_pqbf = (OPB_PQBF *) opu_memory(global, psize);
    MEfill(psize, 0, (PTR) pqbfp);
    p = (PTR) pqbfp + sizeof(OPB_PQBF);
    p = ME_ALIGN_MACRO(p, DB_ALIGN_SZ);
    /* Put all the eqclass arrays next, as they might need alignment.
    ** The bitmaps can go after that, they can go anywhere.
    */
    for (i = 0; i < partp->ndims; ++i)
    {
	dimp = &partp->dimension[i];
	if (dimp->distrule != DBDS_RULE_AUTOMATIC)
	{
	    eqcp = pqbfp->opb_dimeqc[i] = (OPE_IEQCLS *) p;
	    p = p + sizeof(OPE_IEQCLS) * dimp->ncols;
	    /* Make sure that dims not referenced or not useful
	    ** don't bogusly map to eqclass 0.
	    */
	    j = dimp->ncols;
	    do {
		*eqcp++ = OPE_NOEQCLS;
	    } while (--j > 0);
	}
    }
    if (num_sdims == 1)
    {
	pqbfp->opb_sdimbfm = (OPB_BMBF *) p;
	p = p + sizeof(OPB_BMBF);
    }
    for (i = 0; i < partp->ndims; ++i)
    {
	dimp = &partp->dimension[i];
	if (dimp->distrule != DBDS_RULE_AUTOMATIC && dimp->ncols > 1)
	{
	    pqbfp->opb_mdimbfm[i] = (OPB_BMBF *) p;
	    p = p + sizeof(OPB_BMBF);
	}
    }

} /* opb_pqbf_alloc */

/*
** Name: opb_pqbf_findeqc - Find partitioning dimension/column given eqc
**
** Description:
**
**	This utility is given an eqclass that purports to be some
**	table partitioning column.  We're also given the table
**	partitioning info (DB_PART_DEF and OPB_PQBF).  A simple
**	search through the PQBF eqclass arrays returns the desired
**	dimension and column index (ie 0..dimp->ncols-1).
**
**	If the eqclass isn't found, we crash the query with an error.
**
** Inputs:
**	partp			Table partitioning definition
**	pqbfp			OPB_PQBF partition qualification info
**	eqc			The eqclass number we're looking for
**	dim			(i4 *) an output
**	col_index		(i4 *) an output
**
** Outputs:
**	dim			Returned dimension number (0..ndims-1)
**	col_index		Column index within partitioning columns
**				of that dimension, 0..dimp->ncols-1
**
** History:
**	31-Jul-2007 (kschendel) SIR 122513
**	    New partition qualification stuff.
*/

void
opb_pqbf_findeqc(DB_PART_DEF *partp, OPB_PQBF *pqbfp,
	OPE_IEQCLS eqc, i4 *dim, i4 *col_index)
{
    i4 i, j;
    OPE_IEQCLS *eqcp;

    for (i = 0; i < partp->ndims; ++i)
    {
	eqcp = pqbfp->opb_dimeqc[i];
	for (j = 0; j < partp->dimension[i].ncols; ++j)
	{
	    if (*eqcp == eqc)
	    {
		*dim = i;		/* Found it */
		*col_index = j;
		return;
	    }
	    ++eqcp;
	}
    }

    /* Caller error if not found, fall over. */
    opx_1perror(E_OP009D_MISC_INTERNAL_ERROR, "eqc not found in partitioning scheme");

} /* opb_pqbf_findeqc */

/*
** Name: opb_pqbf_ok - Validate BF qtree fragment for use with
**			partition qualification
**
** Description:
**
**	This recursive routine walks part-way down a boolean factor's
**	query tree, looking for situations that would preclude the
**	BF from being used in partition qualification.  It also does
**	a very rough-cut estimate of the partition selectivity (only
**	because this is a convenient place to do it).
**
**	For a boolean factor to be a candidate, it has to pass
**	various tests:
**
**	- For an initial version, we'll require that the boolfact
**	  be CNF, i.e. not contain any AND's or NOT's internally.
**	  While in theory a boolfact like:
**	    c1=value OR (c2=value AND dontcare1=dontcare2)
**	  can participate in partition qualification, the complications
**	  of both analysis and code generation will have to wait for
**	  another day.
**
**	- All of the disjuncts (OR'ed terms) must have the form
**	  col RELOP expr (or expr RELOP col), where either:
**	    o  col is the partitioning column for any single-column
**	       dimension of the table being analyzed, or
**	    o  ALL occurrences of col are for the SAME column from a
**	       composite dimension of the table.
**	  Any non-partitioning-column or different-table col reference
**	  on the VAR side of the relop disqualifies the BF.
**	  (The top level is able to determine whether a BF references
**	  exactly one column of a composite dim;  the corresponding
**	  eqclass is passed in so that we can double check.)
**
**	- The allowable RELOP's are EQ, LT, LE, GT, GE, NE, IS NULL, and
**	  IS NOT NULL, with the additional restrictions:
**	    o  hash partitioning only allows EQ and IS NULL;
**	    o  NE and IS NOT NULL are only allowed for list partitions;
**	    o  in a composite dimension, only EQ is allowed unless
**	       the column is the last one of the dimension columns.
**
**	- If the query language is QUEL, the partitioning-column data
**	  type must not be character type.  (This is to avoid all the
**	  wacky quel implied-LIKE semantics, which aren't worth checking
**	  for.)
**
**	At this stage, we're not interested in what lies below a
**	predicate op (BOP or UOP), other than extracting the relevant
**	column reference.  The other side of a BOP can be any arbitrary
**	expression, even one referencing other table columns.  We'll
**	let such things through, to be excluded at OPC time if they
**	still aren't appropriate.
**
**	As mentioned in the driver, the goal here is to exclude
**	obviously useless candidates, and NOT to try to achieve 100%
**	accurate inclusion or exclusion (which isn't possible at this
**	stage anyway;  we don't know the ultimate shape of the query plan.)
**
**	The selectivity guess is extremely crude:  if we see an EQ operator,
**	assume it selects one (logical) partition.  (One per value if
**	the left side is an IN.)  If we see anything else, assume that
**	it selects half of what's been selected so far.  (the idea
**	being to end up with 1/4 of total logical partitions for BETWEEN.)
**
** Inputs:
**	subquery		Pointer to subquery being compiled
**	partp			Table partitioning definition
**	pqbfp			OPB_PQBF * descriptor for table
**	qnode			PST_QNODE * query tree fragment to analyze
**	reqd_eqc		If OPE_NOEQCLS, predicate VAR can reference
**				any partitioning column in the table;
**				otherwise, it must reference this EQC exactly.
**				(If NOEQCLS, caller has verified that the
**				BF only references single-column dimensions.)
**
** Outputs:
**	Returns TRUE if qnode subtree fragment is OK, FALSE if not.
**
** History:
**	31-Jul-2007 (kschendel) SIR 122513
**	    Written.
**	27-Aug-2007 (kschendel)
**	    Re-establish some sort of partition selectivity guess, used
**	    by later pcjoin and exch analysis in opj.
**	19-Sep-2007 (kschendel)
**	    Special-case testing missed the combination of IS NOT NULL
**	    on a non-nullable column.  IS NOT NULL usually gets pitched
**	    anyway, but for LIST dimensions it turns into a != NULL.
**	    OPC throws a fit if you try <non-nullable> != NULL, and it
**	    evaluates to TRUE anyway which is no help.  So toss that
**	    case here.  (Caused E_OP0897 in opc.)
*/

static bool
opb_pqbf_ok(OPS_SUBQUERY *subquery,
	DB_PART_DEF *partp, OPB_PQBF *pqbfp,
	PST_QNODE *qnode, OPE_IEQCLS reqd_eqc)
{
    ADI_OP_ID op;			/* Predicate operator */
    DB_PART_DIM *dimp;			/* Dimension descriptor */
    i4 col_index;			/* Column no within part dim */
    i4 dim;				/* Dimension number */
    OPE_IEQCLS eqc;
    OPG_CB *ops_server;			/* oper ID's are in here */
    OPZ_AT *abase;			/* base of joinop attributes array */
    OPZ_ATTS *ap;			/* Ptr to joinop attr info */
    PST_QNODE *var_node;

    abase = subquery->ops_attrs.opz_base; 
    ops_server = subquery->ops_global->ops_cb->ops_server;

    /* If we have a (top-level) OR, recurse on right, loop down left.
    ** The parser likes to make left-deep OR trees.  (The CNF transform
    ** seems to produce randomly leaning OR trees.)
    */
    while (qnode->pst_sym.pst_type == PST_OR)
    {
	if (!opb_pqbf_ok(subquery, partp, pqbfp, qnode->pst_right, reqd_eqc))
	    return (FALSE);
	qnode = qnode->pst_left;
    }

    /* We're interested in BOP's and UOP's underneath any OR's that might
    ** exist.  Anything else (AND, NOT, whatever) makes the BF not-ok.
    ** (There aren't any COP or MOP predicates, and even if there are,
    ** we only know how to use the standard built-in relops.)
    */

    if (qnode->pst_sym.pst_type != PST_UOP
      && qnode->pst_sym.pst_type != PST_BOP)
	return (FALSE);

    /* Predicate operator check, more checks later */

    op = qnode->pst_sym.pst_value.pst_s_op.pst_opno;
    if (qnode->pst_sym.pst_type == PST_UOP)
    {
	if (op != ops_server->opg_isnull && op != ops_server->opg_isnotnull)
	    return (FALSE);
    }
    else
    {
	if (op != ops_server->opg_eq && op != ops_server->opg_ne
	  && op != ops_server->opg_lt && op != ops_server->opg_le
	  && op != ops_server->opg_gt && op != ops_server->opg_ge)
	    return (FALSE);
    }
    

    /* Insist on "UOP var", or in the case of a BOP, either "var BOP x"
    ** or "x BOP var".  I.e. one of the operands must be a bare VAR
    ** that translates into a usable partitioning column reference.
    ** N.B. the var's "dmf-att-id" is now of course a joinop att id.
    */
    var_node = qnode->pst_left;		/* Var is probably the left */
    eqc = OPE_NOEQCLS;
    if (var_node->pst_sym.pst_type == PST_VAR
      && var_node->pst_sym.pst_value.pst_s_var.pst_vno != OPV_NOVAR)
    {
	ap = abase->opz_attnums[var_node->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id];
	eqc = ap->opz_equcls;
	if (eqc < 0
	  || ! BTtest(eqc, (char *) &pqbfp->opb_alldimeqcs)
	  || (reqd_eqc != OPE_NOEQCLS && eqc != reqd_eqc))
	    eqc = OPE_NOEQCLS;	/* This one fails... */
    }
    if (eqc == OPE_NOEQCLS && qnode->pst_sym.pst_type == PST_BOP)
    {
	/* Try the other side if it's a BOP */
	var_node = qnode->pst_right;
	eqc = OPE_NOEQCLS;
	if (var_node->pst_sym.pst_type == PST_VAR
	  && var_node->pst_sym.pst_value.pst_s_var.pst_vno != OPV_NOVAR)
	{
	    ap = abase->opz_attnums[var_node->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id];
	    eqc = ap->opz_equcls;
	    if (eqc < 0
	      || ! BTtest(eqc, (char *) &pqbfp->opb_alldimeqcs)
	      || (reqd_eqc != OPE_NOEQCLS && eqc != reqd_eqc))
		eqc = OPE_NOEQCLS;	/* This one fails... */
	}
    }
    if (eqc == OPE_NOEQCLS)
	return (FALSE);

    /* So far, so good.  Now do checks that need the knowledge of
    ** what the partitioning column / dimension is.
    */

    /* QUEL char-equality test:  char OP const in quel uses pseudo-
    ** LIKE semantics, so skip those.  (quel doesn't know about new
    ** unicode types.)
    ** (By the way, not good enough to look at the adfcb qlang; quel
    ** views in SQL queries work the same way.  Look at the operator
    ** target itself.)
    */
    if (qnode->pst_sym.pst_type == PST_BOP)
    {
	i4 abs_type;
	PST_QNODE *exp_node;

	abs_type = var_node->pst_sym.pst_dataval.db_datatype;
	if (abs_type < 0)
	    abs_type = -abs_type;
	if (abs_type == DB_CHA_TYPE || abs_type == DB_CHR_TYPE
	  || abs_type == DB_VCH_TYPE || abs_type == DB_TXT_TYPE)
	{
	    exp_node = qnode->pst_right;
	    if (exp_node == var_node)
		exp_node = qnode->pst_left;	/* Get the non-VAR side */
	    if (exp_node->pst_sym.pst_type == PST_CONST
	      && exp_node->pst_sym.pst_value.pst_s_cnst.pst_cqlang == DB_QUEL)
		return (FALSE);			/* Quel string compare! */
	}
    }

    /* Find the partitioning dimension related to this col reference. */
    opb_pqbf_findeqc(partp, pqbfp, eqc, &dim, &col_index);
    dimp = &partp->dimension[dim];

    /* Caller can verify that if there are any single-col dim refs,
    ** there are no composite dim refs;  don't need to verify that.
    */

    /* Hash partitioning only allows EQ, IS NULL */
    if (dimp->distrule == DBDS_RULE_HASH
      && (op != ops_server->opg_eq && op != ops_server->opg_isnull))
	return (FALSE);

    /* NE, IS NOT NULL are only allowed for list dimensions */
    if ( (op == ops_server->opg_ne || op == ops_server->opg_isnotnull)
      && dimp->distrule != DBDS_RULE_LIST)
	return (FALSE);

    /* Composite dim only allows EQ (or is null) except on the last
    ** column of the dimension.
    */
    if (reqd_eqc != OPE_NOEQCLS
      && (op != ops_server->opg_eq && op != ops_server->opg_isnull)
      && col_index != dimp->ncols-1)
	return (FALSE);

    /* IS NULL for non-nullable column is not allowed for composite
    ** dims.  (Checks in OPC should exclude it too, but try to catch now.)
    ** Is OK for simple dimensions because we generate a FALSE equivalent.
    */
    if (op == ops_server->opg_isnull && reqd_eqc != OPE_NOEQCLS
      && var_node->pst_sym.pst_dataval.db_datatype > 0)
	return (FALSE);

    /* Similar, but slightly different, test for IS NOT NULL on a
    ** non-nullable column.  This time it's equivalent to TRUE, so
    ** discard this boolfact.  (We could announce a partial-key
    ** for composite dims but that sort of thing isn't handled yet.)
    */

    if (op == ops_server->opg_isnotnull
      && var_node->pst_sym.pst_dataval.db_datatype > 0)
	return (FALSE);

    /* Apply wild guess for selectivity.  Only bother with BOP's, and
    ** assume we have a constant expression.  The main use for the
    ** resulting wild guess is for parallelizing fudge factors, so
    ** inaccuracy is hardly likely to be a big problem.
    ** Also, only bother for the last column of a composite dimension.
    */
    if (qnode->pst_sym.pst_type == PST_BOP && col_index == dimp->ncols-1)
    {
	if (op != ops_server->opg_eq)
	{
	    /* Take 1/2 of what is selected so far */
	    pqbfp->opb_pqlcount[dim] = (pqbfp->opb_pqlcount[dim] + 1) / 2;
	}
	else
	{
	    i4 lpcount = pqbfp->opb_pqlcount[dim];
	    i4 clip;
	    PST_QNODE *exp_node;

	    if (lpcount == dimp->nparts)
		lpcount = 1;		/* First EQ seen */
	    else
		++ lpcount;
	    /* Do a quick check for an IN and run down the IN-list counting */
	    exp_node = qnode->pst_right;
	    if (exp_node == var_node)
		exp_node = qnode->pst_left;	/* Get the non-VAR side */
	    while (exp_node->pst_sym.pst_type == PST_CONST && exp_node->pst_left != NULL)
	    {
		++ lpcount;
		exp_node = exp_node->pst_left;
	    }
	    /* Make another wild guess here and limit the number of
	    ** logical partitions selected by IN/EQ to 90% of all
	    ** of them, or N-1 max.  (If we let it go to N it will look
	    ** like no EQ's at all, and one more will set it back to 1!)
	    */
	    clip = dimp->nparts - dimp->nparts/10;
	    if (clip == dimp->nparts)
		clip = dimp->nparts-1;
	    if (lpcount > clip)
		lpcount = clip;
	    pqbfp->opb_pqlcount[dim] = lpcount;
	}
    }

    /* So far, so good! */
    return (TRUE);

} /* opb_pqbf_ok */
