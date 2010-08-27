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
#define        OPJ_JOINOP      TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>

/**
**
**  Name: OPJJOINOP.C - Initialize structures for path selection
**
**  Description:
**	This file contains the main drivers for the enumeration phase
**	of optimization.  This includes query tree pre-processing,
**	calling the actual enumeration to choose a cost tree,
**	and post-processing of the CO tree to make it suitable
**	for code generation.
**
**  History:    
**      1-mar-86 (seputis)    
**          initial creation
**      3-aug-88 (seputis)    
**          when calling opj_mexact with a sorted outer, need to ignore
**          ordering for joining eqcls if a KEY join is used, i.e.
**          cop->opo_inner->opo_sjpr == DB_ORIG, since keying does not
**          need an ordered outer.... FIXME check the cost model
**          on qccc.qry since the outer should not need to be sorted
**          at all at this point.
**      8-sep-92 (ed)
**          merge sybil into mainline remove ifdefs
**      21-may-93 (ed)
**          - changed opo_var to be a union structure so that
**          the outer join ID can be stored in interior CO nodes
**	    - correct outer join problems when function attributes
**	    are created using attributes from outer joins
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	07-Oct-1993(fpang)
**	    - Fix B53448, SEGV when STAR query has many simple aggregates.
**	      In opj_maordering(), opo_orig is valid if opo_sjpr = DB_ORIG.
**	19-jan-94 (ed)
**	    - correct outer join problem with query such as
**	    select * from (r left join s on s.a=r.a) 
**		where s.a IS NOT NULL and r.a >5
**	    in which the boolean factor can be evaluted at the
**	    leaf node but must also be evaluated at the join node
**	    with an iftrue
**	    - remove obsolete outer join code
**      30-mar-95 (brogr02)
**	    Prevent unecessarily adding the const's eqcls to intermediate nodes 
**	    in the outer tree, in opc_cartprod().
**	30-jan-97 (chech02)
**	    Integreated changes from main(#428372):   Init union_cop struct
**	    to prevent floating point exception in VMS.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	16-jul-2002 (toumi01)
**	    Add config parameter for hash join (default is off); this startup
**	    setting is toggled by OP162.
**	25-May-2005 (schka24)
**	    Track proposed exch depth so that we can build them from the
**	    bottom up.  Otherwise exch adding gets confused.
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**	14-Mar-2007 (kschendel) SIR 122513
**	    Recognize partition-compatible aggregation.
**	3-Jun-2009 (kschendel) b122118
**	    Minor code and comment cleanup.
**     28-Oct-2009 (maspa05) b122725
**          Moved declaration of ult_open_tracefile to ulf.h
**	10-Aug-2010 (kschendel) b124218
**	    Don't parallelize a fetch from the table being updated by
**	    an update-type query, or from any of its secondary indexes.
**	    DMF's btree code (dm1b_rcb_update and friends) assumes that
**	    concurrency issues are handled by locking, which doesn't
**	    apply within a session.  dm1b_rcb_update doesn't see the
**	    child thread RCB and doesn't do necessary position updates.
**	    Allowing it to see the child RCB isn't the answer, because
**	    as of this writing, the other RCB's are updated without
**	    proper concurrency controls.  It's best to just avoid the
**	    situation.
**/

/*}
** Name: OPJ_MASTATE - control block used to modify CO tree
**
** Description:
**      This control block is used to modify the CO tree in order
**      to define node IDs, and help redefine multi-attribute orderings
**
** History:
**      28-apr-88 (seputis)
**          initial creation
[@history_template@]...
*/
typedef struct _OPJ_MASTATE
{
    OPS_SUBQUERY    *opj_subquery;      /* current subquery */
    i4              opj_nodeid;         /* next available ID for
                                        ** labelling a node */
    OPE_IEQCLS      opj_maxeqcls;       /* max equivalence class prior
                                        ** to definition of puesdo
                                        ** equivalence classes for
                                        ** cartesean products */
    OPB_BMBF        opj_bfbm;           /* used to calculate the
                                        ** special equivalence classes
                                        ** from outer joins available
                                        ** at a node */
    OPL_BMOJ	    opj_ojmap;		/* used to help build map of 
					** outer joins entirely evaluated
					** within a subtree */
    OPL_BMOJ	    opj_ojevalmap;	/* used to build map of outer joins
					** which are completely evaluated
					** within a subtree */
    OPL_BMOJ	    opj_ojinnermap;	/* used to build map of outer joins
					** which have at least one join
					** node within this subtree */
} OPJ_MASTATE;

/*}
** Name: EXCHDESC	- control block used to augment QEP with 
**	exchange nodes for parallel query processing
**
** Description:
**      This control block describes the execution time reduction of
**	prospective exchange nodes 
**
** History:
**      13-nov-03 (inkdo01)
**	    Written (for parallel query processing support).
**	24-May-2005 (schka24)
**	    Rename index -> level.
[@history_template@]...
*/
typedef struct _EXCHDESC
{
    OPO_CO	**exch_parent;	/* ptr to ptr to parent of prospective
				** exchange node */
    OPO_COST	exch_reduction; /* estimated reduction in execution
				** due to prospective exchange */
    OPS_SUBQUERY *exch_subqp;	/* ptr to subquery structure for this
				** node */
    i4		exch_count;	/* no. of child threads to attach */
    i2		exch_level;	/* Recursion depth so that we can add
				** exch's from the bottom up */
    bool	exch_outer;	/* TRUE - entry is for outer subtree
				** FALSE - entry is for inner subtree */
    bool	exch_pr;	/* TRUE - exch is ABOVE PR node */
    bool	exch_un;	/* TRUE - exch is ABOVE a union */
} EXCHDESC;

#define OPJ_TJOINPRESORT_THRESHOLD 10000
#define OPJ_KJOINPRESORT_THRESHOLD 5000

static bool
opj_usehashjoin( OPS_SUBQUERY *subquery );

static VOID
opj_hintinit(
	OPS_SUBQUERY	*subquery);

static bool
opj_ckhasheqc(
	OPS_SUBQUERY	*sq,
	OPE_IEQCLS	sjeqc);

static VOID
opj_partition(
	OPS_STATE	*global);

static VOID
opj_partanal(
	OPS_SUBQUERY	*sq,
	OPO_CO		*cop,
	OPV_PCJDESC	**pcjdpp,
	i4		*gcount,
	bool		*istable);

static VOID
opj_partpushg(
	OPO_CO	*cop,
	OPV_PCJDESC *pcjdp,
	i4	gcount);

static void opj_partagg(
	OPS_SUBQUERY *sq,
	OPV_PCJDESC *pcjdp,
	i4	gcount);

static VOID
opj_exchange(
	OPS_STATE	*global);

static void opj_exchupd_anal(
	OPS_STATE	*global);

static bool
opj_excheval(
	OPS_SUBQUERY	*sq,
	OPO_CO		*cop,
	OPO_CO		**pcopp,
	EXCHDESC	*exchp,
	i4		*exchpixp,
	i4		level);

static VOID
opj_exchunion(
	OPS_STATE	*global,
	EXCHDESC	*exchp,
	OPV_VARS	*varp,
	i4		*exchpixp,
	i4		level,
	i4		*ucount,
	OPO_COST	*ureduction);

static VOID
opj_exchadd(
	OPS_STATE	*global,
	EXCHDESC	*exchp,
	i4		exchcount);

/*{
** Name: opj_minop	- optimize MIN aggregate
**
** Description:
**      Search the outer elements of the subquery structure and look at 
**      first tuple if BTREE, or sort node ordering is available
**
** Inputs:
**      subquery                        simple aggregate subquery to be
**                                      optimized
**
** Outputs:
**      subquery->ops_first             set TRUE if MIN aggregate found with
**                                      appropriate ordering
**	Returns:
**	    VOID
**	Exceptions:
**
**
** Side Effects:
**
**
** History:
**      12-oct-87 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opj_minop(
	OPS_SUBQUERY       *subquery)
{
    PST_QNODE           *resdom;
    if (subquery->ops_first		    /* this is an ANY aggregate which
                                            ** already has this flag set */
	||
	!subquery->ops_bestco		    /* check if an ordering is
					    ** available at the top of the
					    ** query plan */
	)
	return;				    

    for (resdom = subquery->ops_root->pst_left;
	resdom->pst_sym.pst_type == PST_RESDOM;
	resdom = resdom->pst_left)
    {	/* look at the target list of this simple aggregate and determine
        ** if only the MIN or ANY aggregates are referenced in this query  
        ** - return to caller if an aggregate is found which requires
        ** more than one tuple to evaluate */

	if (resdom->pst_right->pst_sym.pst_type == PST_AOP)
	{
	    if (resdom->pst_right->pst_sym.pst_value.pst_s_op.pst_opno
		==
		subquery->ops_global->ops_cb->ops_server->opg_any
		)
		continue;		    /* ANY aggregates can be evaluated
                                            ** on first tuple */
	    if (resdom->pst_right->pst_sym.pst_value.pst_s_op.pst_opno
		!=
		subquery->ops_global->ops_cb->ops_server->opg_min
	       )
		return;			    /* need to look at all tuples
					    ** for this aggregate */
	    {	/* check that there is an ordering for this MIN aggregate */
		PST_QNODE	*varnode;
		OPO_CO          *outer;
		OPV_VARS        *varp;
		OPE_IEQCLS      eqcls;

		varnode = resdom->pst_right->pst_left;
		if (varnode->pst_sym.pst_type != PST_VAR)
		    return;		    /* need a single attribute for
					    ** the ordering */
		eqcls = subquery->ops_attrs.opz_base->opz_attnums
		    [varnode->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id]
			->opz_equcls;
		for (outer = subquery->ops_bestco;
		    outer;
		    outer = outer->opo_outer)
		{   /* look for ordering or find the DB_ORIG node */
		    if (outer->opo_ordeqc != OPE_NOEQCLS)
		    {	/* an ordering is available */
			if (outer->opo_ordeqc != eqcls)
			    return;	    /* ordering is not usable for
					    ** the min aggregate */
			break;		    /* break if ordering is
					    ** useful */
		    }
		    if (outer->opo_sjpr == DB_ORIG)
		    {	/* check for case in which ordering is available
			** in ORIG node but not used for keying */
			varp = subquery->ops_vars.opv_base->
			    opv_rt[outer->opo_union.opo_orig];
			if ((varp->opv_mbf.opb_count > 0)
			    &&
			    (varp->opv_tcop->opo_storage == DB_BTRE_STORE)
			    &&
			    (subquery->ops_attrs.opz_base->opz_attnums[varp->
				opv_mbf.opb_kbase->opb_keyorder[0].opb_attno]
				    ->opz_equcls
			    ==
			    eqcls
			    )
			   )
			    break;	    /* the relation is ordered on
					    ** the correct equivalence class
					    ** - FIXME - check for constant
					    ** equivalence classes and
					    ** place this code in the
					    ** enumeration model */
			else
			    return;
		    }
		}
	    }
	}
    }
    subquery->ops_first = TRUE;		    /* all simple aggregates satisfy
					    ** the assertion, and can be
					    ** evaluate on receiving first
                                            ** tuple */
}

/*{
** Name: opj_incopy	- copy info into large array of ptrs
**
** Description:
**      In the second part of joinop processing, the internal arrays 
**      may expand, so that we should copy the array of ptrs back into
**	the large arrays 
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
**	    none
**
** History:
**      28-jun-89 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opj_incopy(
	OPS_SUBQUERY       *subquery,
	bool		   boolfact)
{
    i4		    size;           /* size of copy */
    OPS_STATE	    *global;

    global = subquery->ops_global;
    /* FIXME - try to recycle the memory lost here */
    {
	OPV_RT	    *rt;	    /* exact size OPV_RT struct */

	if (size = sizeof(rt->opv_rt[0]) * subquery->ops_vars.opv_rv)
	    MEcopy ((PTR)subquery->ops_vars.opv_base, size, (PTR)global->ops_mstate.ops_trt);
	subquery->ops_vars.opv_base = global->ops_mstate.ops_trt;
    }
    {
	OPZ_FT	    *ft;	    /* exact size OPZ_FT struct */

	if(size = sizeof(ft->opz_fatts[0]) * subquery->ops_funcs.opz_fv)
	    MEcopy ((PTR)subquery->ops_funcs.opz_fbase, size, 
		(PTR)global->ops_mstate.ops_tft);
	subquery->ops_funcs.opz_fbase = global->ops_mstate.ops_tft;
    }
    {
	OPZ_AT	    *at;	    /* exact size OPZ_AT struct */

	size = sizeof(at->opz_attnums[0]) * subquery->ops_attrs.opz_av;
	if (size)
	    MEcopy ((PTR)subquery->ops_attrs.opz_base, size, 
		(PTR)global->ops_mstate.ops_tat);
	subquery->ops_attrs.opz_base = global->ops_mstate.ops_tat;
    }
    {
	OPE_ET	    *et;	    /* exact size OPV_ET struct */

	size = sizeof(et->ope_eqclist[0]) * subquery->ops_eclass.ope_ev;
	if (size)
	    MEcopy ((PTR)subquery->ops_eclass.ope_base, size, 
		(PTR)global->ops_mstate.ops_tet);
	subquery->ops_eclass.ope_base = global->ops_mstate.ops_tet;
    }
    if (boolfact)
    {
	OPB_BFT	    *bft;	    /* exact size OPV_BFT struct */

	size = sizeof(bft->opb_boolfact[0]) * subquery->ops_bfs.opb_bv;
	if (size)
	    MEcopy ((PTR)subquery->ops_bfs.opb_base, size, 
		(PTR)global->ops_mstate.ops_tbft);
	subquery->ops_bfs.opb_base = global->ops_mstate.ops_tbft;
    }
}

/*{
** Name: opj_outcopy	- copy array of ptrs from subquery tables
**
** Description:
**      Large array of ptrs are allocated so that the maximum number
**      of entries can be referenced; when completing the processing of 
**      a subquery, then just allocate the exact amount and copy the 
**      into out the the larget array, so that this large array can be 
**      used by the next subquery 
**
** Inputs:
**      subquery                        ptr to subquery to copy info out of
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
**      28-jun-89 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opj_outcopy(
	OPS_SUBQUERY       *subquery,
	bool		   boolfact)
{
    i4		    size;           /* size of copy */
    OPS_STATE	    *global;

    global = subquery->ops_global;
    {
	OPV_RT	    *rt;	    /* exact size OPV_RT struct */

	if (size = sizeof(rt->opv_rt[0]) * subquery->ops_vars.opv_rv)
	{
	    rt = (OPV_RT *) opu_memory(global, size);
	    MEcopy ((PTR)subquery->ops_vars.opv_base, size, (PTR)rt);
	}
	else
	    rt = (OPV_RT *)NULL;
	subquery->ops_vars.opv_base = rt;
    }
    {
	OPZ_FT	    *ft;	    /* exact size OPZ_FT struct */

	if(size = sizeof(ft->opz_fatts[0]) * subquery->ops_funcs.opz_fv)
	{
	    ft = (OPZ_FT *) opu_memory(global, size);
	    MEcopy ((PTR)subquery->ops_funcs.opz_fbase, size, (PTR)ft);
	}
	else
	    ft = (OPZ_FT *) NULL;
	subquery->ops_funcs.opz_fbase = ft;
    }
    {
	OPZ_AT	    *at;	    /* exact size OPZ_AT struct */

	size = sizeof(at->opz_attnums[0]) * subquery->ops_attrs.opz_av;
	if (size)
	{
	    at = (OPZ_AT *) opu_memory(global, size);
	    MEcopy ((PTR)subquery->ops_attrs.opz_base, size, (PTR)at);
	}
	else
	    at = (OPZ_AT *) NULL;
	subquery->ops_attrs.opz_base = at;
    }
    {
	OPE_ET	    *et;	    /* exact size OPV_ET struct */

	size = sizeof(et->ope_eqclist[0]) * subquery->ops_eclass.ope_ev;
	if (size)
	{
	    et = (OPE_ET *) opu_memory(global, size);
	    MEcopy ((PTR)subquery->ops_eclass.ope_base, size, (PTR)et);
	}
	else
	    et = (OPE_ET *)NULL;
	subquery->ops_eclass.ope_base = et;
    }
    if (boolfact)
    {
	OPB_BFT	    *bft;	    /* exact size OPV_BFT struct */
	i4	    size;

	size = sizeof(bft->opb_boolfact[0]) * subquery->ops_bfs.opb_bv;
	if (size)
	{
	    bft = (OPB_BFT *) opu_memory(global, size);
	    MEcopy ((PTR)subquery->ops_bfs.opb_base, size, (PTR)bft);
	}
	else
	    bft = (OPB_BFT *) NULL;
	subquery->ops_bfs.opb_base = bft;
    }
}

/*{
** Name: opj_jeqc	- traverse the subqueries to determine the joining equivalence classes
**
** Description:
**      This routine traverses all the subqueries to determine the
**	joining equivalence classes so that boolean factors can be compared
**      using transitivity for copying into unions, projections, partitioned tables
**
** Inputs:
**      subquery                        subquery to analyzed
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
**      19-jun-89 (seputis)
**          initial creation
**	24-nov-95 (inkdo01)
**	    Re-ordered function evaluation (promoted opj_normalize) for 
**	    common factor extraction.
**	6-feb-97 (inkdo01)
**	    Re-ordered again (promoted opj_normalize in front of opv_create),
**	    this time for the MS Access OR transformation.
**	14-jul-99 (inkdo01)
**	    Added code to detect join queries with no where clause (idiot
**	    queries, for which we pick first valid plan).
**	8-sep-99 (inkdo01)
**	    Fixed the above stupid piddling little change.
**	27-may-2008 (dougi)
**	    Search for EQCs in table procedure parm list.
**      10-sep-2009 (huazh01)
**          Enable OPS_IDIOT_NOWHERE flag for a query that only has constant
**          restrictions in its where clause. (b122530)
*/
static VOID
opj_jeqc(
	OPS_SUBQUERY       *subquery)
{
    bool found = FALSE;
    PST_QNODE *nodePtr; 

    opj_normalize(subquery, &subquery->ops_root->pst_right); /* normalize the
                                        ** qualification prior to looking at
                                        ** boolean factors 
                                        */
    opv_create(subquery);		/* create local (joinop) range table 
                                        ** by creating local range variable
                                        ** elements for any global range
                                        ** variable referenced in the subquery
                                        ** - initialize related 
                                        ** subquery->ops_vars structure 
                                        */
    if ((subquery->ops_root->pst_right == NULL || 
	subquery->ops_root->pst_right->pst_sym.pst_type == PST_QLEND ) &&
	subquery->ops_vars.opv_prv > 1) subquery->ops_mask |= OPS_IDIOT_NOWHERE;
					/* if subquery contains joins, but NO where 
					** clause, it is an idiot query and should
					** trigger heuristic to pick first valid plan */

    nodePtr = subquery->ops_root->pst_right; 

    /* b122530: 
    ** check if the tree contains no joins but only constant restrictions and
    ** enable OPS_IDIOT_NOWHERE if it does. 
    */
    while (nodePtr && nodePtr->pst_sym.pst_type != PST_QLEND)
    {
       if (nodePtr->pst_sym.pst_type == PST_AND &&
           nodePtr->pst_left->pst_sym.pst_type == PST_BOP &&
           nodePtr->pst_left->pst_left->pst_sym.pst_type  == PST_VAR &&
           nodePtr->pst_left->pst_right->pst_sym.pst_type == PST_CONST)
           {
              nodePtr = nodePtr->pst_right;
              continue; 
           }

       found = TRUE; 
       break; 
    }

    if (!found && subquery->ops_vars.opv_prv > 1) 
    {
       subquery->ops_mask |= OPS_IDIOT_NOWHERE;
    }
       
    ope_create(subquery);               /* create equivalence class table 
                                        ** - needs to be initialized prior
                                        ** to creation of joinop attributes
                                        ** because equivalence classes
                                        ** are created when a joinop attribute
                                        ** for an implicit tid is created */
    opz_create(subquery);               /* create joinop attributes table 
                                        ** and relabel query tree variable
                                        ** nodes using the joinop attribute
                                        ** number and joinop variable number
                                        ** - init function attributes table
                                        ** - initialize related 
                                        ** subquery->ops_attrs structure 
                                        */
    if (subquery->ops_oj.opl_base)
        opl_ojmaps(subquery);           /* initialize some maps needed to support
                                        ** outer joins */
    /* FIXME - need equivalent to rv>0 || pst_type!=PST_QLEND */
    ope_findjoins(subquery);		/* find join clauses and create 
                                        ** equivalence classes given
                                        ** the qualification of the subquery
                                        ** - remove join clauses from the
                                        ** qualification
                                        ** - create function attributes if they
                                        ** would be useful in a join
                                        */
    /*join_trans_closure(subquery);*/	/* take care of joins that can be
                                        ** inferred by two vars being equal
				        ** to the same constant
                                        */
    ope_aseqcls(subquery, (OPE_BMEQCLS *) NULL, subquery->ops_root->pst_right);
					/* assign
                                        ** equivalence classes for vars in
                                        ** the qualification which have not
                                        ** been assigned yet
                                        */
    if (subquery->ops_mask2 & OPS_TPROC)/* assign eqs for vars in tproc parms */
    {
	OPV_VARS	*varp;
	PST_QNODE	*rootp;
	i4		i;

	/* Look for table procs with non-empty parm lists. */
	for (i = 0; i < subquery->ops_vars.opv_rv; i++)
	 if ((varp = subquery->ops_vars.opv_base->opv_rt[i])->
						opv_mask & OPV_LTPROC &&
	    (rootp = subquery->ops_global->ops_qheader->
			pst_rangetab[varp->opv_grv->opv_qrt]->pst_rgroot))
	    ope_aseqcls(subquery, (OPE_BMEQCLS *) NULL, rootp);
		

    }
    if (subquery->ops_sunion.opu_mask & OPU_REFUNION)
	opj_union(subquery);		/* this subquery contains references to
					** union variables, so process all qualifications
					** to determine whether any can be copied to
					** the union components */
    opj_outcopy(subquery, FALSE);	/* copy the subquery tables out of the "large
					** array of pointers */
}

/*{
** Name: opj_translate	- routine to translate a subquery for enumeration
**
** Description:
**      This routine will transform the query tree associated with the
**      subquery into equivalence classes, boolean factors, local range
**      table and joinop attribute references.  The query tree will be
**      "destroyed" or ripped apart once this phase is complete.
**
** Inputs:
**      subquery                        ptr to subquery to be optimized
**
** Outputs:
**	Returns:
**	    FALSE - if this subquery cannot return any rows
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	19-apr-86 (seputis)
**          initial creation
**	7-may-91 (seputis)
**	    fix b37172 - move oph_histos to enumeration so it is called
**	    only when necessary
**	15-mar-99 (hayke02)
**	    We now do not copy boolean factors into all parts of the union
**	    (call to opj_uboolfact()) if the query contains any outer joins.
**	    This prevents outer join id's from one subquery being used to
**	    access the outer join descriptor array, opl_ojt[], from another
**	    subquery, in opb_bfinit() (resulting in a SIGSEGV). This change
**	    fixes bug 95161.
**	 4-apr-00 (hayke02)
**	    The previous change (fix for bug 95161) has been backed out
**	    - the fix has moved to opb_bfinit(). This change fixes bug
**	    100871.
**	16-jan-04 (inkdo01)
**	    Added call to opb_pqbf to support partitioned tables.
**	7-apr-04 (inkdo01)
**	    Added call to opv_pcjnbl for partitioned tables/|| queries.
**	4-july-05 (inkdo01)
**	    Add BY list columns to EQCs requiring histograms.
[@history_line@]...
*/
static bool
opj_translate(
	OPS_SUBQUERY       *subquery)
{
    opj_incopy(subquery,FALSE);		/* copy info from the subquery
					** into the large arrays so that
					** future expansion of the entries
					** is possible */
    opv_tstats(subquery);		/* update the statistics on temporary
					** tables */
    if ((subquery->ops_sqtype == OPS_UNION) && !subquery->ops_union)
	opj_uboolfact(subquery);	/* this query is a union, so some
					** boolean factors exist from parent queries
					** that can be applied to this subquery */
    if (subquery->ops_sunion.opu_mask & OPU_EQCLS)
	ope_aseqcls(subquery, (OPE_BMEQCLS *)NULL, 
	    subquery->ops_root->pst_right); /* equivalence classes were assigned
					** previously, but some may have been
					** added due to the query modification
					** of union optimization */
    opb_create(subquery);               /* create boolean factors table
					** - set boolean factor maps and
					** init boolfact array
					** -if there is a constant qualification
                                        ** which is always FALSE then return
                                        ** FALSE - FIXME -test ops_qfalse
                                        */
    if (subquery->ops_sunion.opu_mask & OPU_NOROWS)
    {
	if((subquery->ops_sqtype == OPS_UNION)
	   ||
	   subquery->ops_union		/* this check will ensure that at
					** least one of the partitions remain
					** in the union, FIXME - special case
					** the situation in which all partitions
					** are removed by referencing a constant
					** view */
	    )
	    return(FALSE);		/* after analysis of the boolean
					** factors it has been determined that
					** this partition cannot return data */
	subquery->ops_sunion.opu_mask ^= OPU_NOROWS; /* reset this mask, otherwise
					** "set qep" will not work */
    }
    opb_pmbf(subquery);                 /* search for matching boolean factors
                                        ** in the primary (base) relations
                                        ** - this is done before useful 
                                        ** indexes are looked for since if
                                        ** a TID lookup is found, then no
                                        ** index is required
                                        */
    if (subquery->ops_global->ops_gmask & OPS_PARTITION)
	opb_pqbf(subquery);		/* search for BFs that will qualify
					** specific partitions of a partitioned
					** table. Used to perform partition
					** elimination at runtime.
					*/
    if (subquery->ops_byexpr)
	ope_aseqcls(subquery, (OPE_BMEQCLS *) NULL, subquery->ops_byexpr);
					/* add BY list entries to eqc list
					** so histograms will be build and 
					** accurate estimates of aggregate
					** result rows can be made */
    subquery->ops_eclass.ope_maxhist = subquery->ops_eclass.ope_ev; /* all
					** currently defined equilvalence 
					** classes are present in the
					** where clause or as part of a
					** join, so save this point, so that
					** histograms can be read in if
					** needed */
    ope_aseqcls(subquery, &subquery->ops_eclass.ope_maps.opo_eqcmap,
            subquery->ops_root->pst_left); /* assign
                                        ** equivalence classes for vars in
                                        ** the target list which have not
                                        ** been assigned yet, and create map
                                        */
    opv_index(subquery);                /* search for all potentially useful
                                        ** indexes
                                        ** - add the indexes to the joinop
                                        ** range table and also search
                                        ** any matching boolean factors
                                        */
    if (subquery->ops_oj.opl_base)
        opl_ojoin(subquery);            /* process outer join variables if
                                        ** they exist, and assign equivalence
                                        ** classes and "IFTRUE" subsitutions
                                        ** to the target list */
    opv_ijnbl( subquery );              /* init the joinable array which is
                                        ** used by enumeration to tell which
                                        ** relations are connected by joins
                                        */
    opv_pcjnbl(subquery);		/* init the partition compatible
					** join structures for partitioned
					** tables */
    if (subquery->ops_global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
	opd_isub_dist(subquery);	/* initialize distributed structures
					** for a subquery */
    return(TRUE);
}

/*{
** Name: opa_union	- relabel union nodes if they exist
**
** Description:
**      Query compilation requires that all subqueries which are unioned
**      together be labelled by the context that it is used e.g. OPS_MAIN
**      OPS_SUBSELECT, OPS_VIEW
**
** Inputs:
**      subquery                        subquery at head of list to be
**                                      relabelled
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
**      26-mar-87 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opa_union(
	OPS_SUBQUERY       *subquery)
{
    /* relabel union node to type of parent, for query compilation */
    OPS_SQTYPE	    sqtype;		/* type of subquery to relabelled union
                                        ** node */

    sqtype = subquery->ops_sqtype;
    for (subquery = subquery->ops_union;
	subquery; 
	subquery = subquery->ops_union)
    {   /* need to make all subquery types the same for all
	** subqueries involved in the union */
#ifdef E_OP038C_UNION
	if (subquery->ops_sqtype != OPS_UNION)
	{
	    opx_error(E_OP038C_UNION);
	}
#endif
	subquery->ops_sqtype = sqtype;
    }
}

/*{
** Name: opj_cartprod	- do not allow 0 length tuples as output from any node
**
** Description:
**	If a cartesean product exists and duplicates need to be preserved
**	then a special traversal over the CO tree is needed to check
**	if any ORIG node or JOIN node returns 0 attributes, which cannot
**	be handled by hold files in QEF/DMF.  In each such case a new
**	equivalence class is created with a single function attribute
**	inside which returns a constant.  TIDs are not sufficient since
**	a view may be involved.  Subselects are treated as cart products
**	but this kludge should not be applied to them.
**
** Inputs:
**      subquery                        ptr to subquery being processed
**      coptr                           co node currently being analyzed
**      eqcls                           equivalence class created by parent
**                                      to prevent 0 length tuple from
**                                      being defined in a temporary
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      28-apr-87 (seputis)
**          initial creation
**      17-may-89 (seputis)
**          relax restriction so that any equivalence class map is non-zero
**      2-oct-90 (seputis)
**          make sure that Pcartprod will have at least one eqcls from the
**          orig node, i.e. opo_maps->opo_eqcmap cannot be empty
**      20-feb-92 (seputis)
**          fix OPC consistency check problem with occurred in outer join path
**          and perhaps could cause other interface problems, ... the eqcmap
**          from the variable descriptor should match the eqcmap from the ORIG
**          node.
**      15-apr-93 (ed)
**          - fix bug 49506, 3 way cart prod eliminates all rows
**      30-mar-95 (brogr02)
**	    Prevent unecessarily adding the const's eqcls to intermediate nodes 
**	    in the outer tree.
[@history_template@]...
*/
static VOID
opj_cartprod(
	OPS_SUBQUERY       *subquery,
	OPO_CO    	   *coptr,
	OPE_IEQCLS         eqcls)
{
    OPO_CO              *inner;
    OPO_CO              *outer;

    if ((inner = coptr->opo_inner)
	&&
	(   /* check if this is an SEJOIN which should not be processed by this
	    ** routine */
	    (inner->opo_sjpr != DB_ORIG)
	    ||
	    (!subquery->ops_vars.opv_base->opv_rt[inner->opo_union.opo_orig]->
		opv_subselect)
	)
       )
    {
        OPE_IEQCLS          inner_eqcls;

	inner_eqcls = eqcls;		/* use existing equivalence class if
					** possible */
        if (BThigh((char *)&coptr->opo_maps->opo_eqcmap, (i4)BITS_IN(coptr->opo_maps->opo_eqcmap)) < 0)
	{   /* no intersection with child so create an equivalence class in
            ** common */
	    if (eqcls != OPE_NOEQCLS)
		eqcls = OPE_NOEQCLS;	/* do not reuse for inner or join will
					** result */
	    else
		inner_eqcls = ope_neweqcls(subquery, OPZ_NOATTR); /* assign a 
					** new equivalence class */
	    BTset((i4)inner_eqcls, (char *)&coptr->opo_maps->opo_eqcmap);
	    BTset((i4)inner_eqcls, (char *)&inner->opo_maps->opo_eqcmap);
	}

	opj_cartprod(subquery, inner, inner_eqcls); /* traverse the inner
					** CO tree */
    }

    if (outer = coptr->opo_outer)
    {
        if (BThigh((char *)&coptr->opo_maps->opo_eqcmap, 
		   (i4)BITS_IN(coptr->opo_maps->opo_eqcmap)) < 0)
	{   /* no intersection with child so create an equivalence class in
            ** common */
	    if (eqcls == OPE_NOEQCLS)
		eqcls = ope_neweqcls(subquery, OPZ_NOATTR); /* assign a
					** new equivalence class */
	    BTset((i4)eqcls, (char *)&coptr->opo_maps->opo_eqcmap);
	    BTset((i4)eqcls, (char *)&outer->opo_maps->opo_eqcmap);
	}
	opj_cartprod(subquery, outer, eqcls); /* traverse the outer
					** CO tree */
    }
    else 
    {	/* orig node found */
	OPZ_FACLASS	fattr_type;

	fattr_type = OPZ_SVAR;		/* if orig node is empty then create
					** a single variable function attribute */
        if (BThigh((char *)&coptr->opo_maps->opo_eqcmap, 
		   (i4)BITS_IN(coptr->opo_maps->opo_eqcmap)) < 0)
	{   /* need at least one eqcls from the orig node */
	    if (eqcls == OPE_NOEQCLS)
		eqcls = ope_neweqcls(subquery, OPZ_NOATTR); /* assign a
					** new equivalence class */
	    BTset((i4)eqcls, (char *)&coptr->opo_maps->opo_eqcmap);
	}
#if 0
/* make all tuple place holders single variable, also make sure eqcls
** gets set in the variable descriptor map for OPC */
	else
	    fattr_type = OPZ_MVAR;	/* create a multi-variable function
					** attribute if ORIG node is not empty */
#endif
	if (eqcls != OPE_NOEQCLS)
	{	/* this must be an ORIG node so equivalence class needs to
	    ** be defined and associated with this variable */
	    OPZ_FATTS	    *func_attr;	/* ptr to function attribute used
					    ** to contain constant node */
	    PST_QNODE	    *const_ptr; /* ptr to constant node */
            OPZ_IATTS       attr;       /* joinop function attribute */
            bool            success;
            OPV_VARS        *varp;
 
            const_ptr = opv_i1const(subquery->ops_global, 0);
	    func_attr = opz_fmake( subquery, const_ptr, fattr_type);
	    STRUCT_ASSIGN_MACRO( const_ptr->pst_sym.pst_dataval, 
		func_attr->opz_dataval);
	    attr = opz_ftoa( subquery, func_attr, coptr->opo_union.opo_orig, const_ptr,
		&func_attr->opz_dataval, fattr_type);
	    success = ope_addeqcls(subquery, eqcls, attr, FALSE); /* add attribute 
					    ** to equivalence
					    ** class, should be the only attribute
					    ** in the equivalence class */
#ifdef    E_OP0385_EQCLS_MERGE
	    if (!success)
		opx_error(E_OP0385_EQCLS_MERGE);
#endif
            varp = subquery->ops_vars.opv_base->opv_rt[coptr->opo_union.opo_orig];
            BTset((i4)eqcls, (char *)&varp->opv_maps.opo_eqcmap);
            if ((&varp->opv_maps) != varp->opv_tcop->opo_maps)
                BTset((i4)eqcls, (char *)&varp->opv_tcop->opo_maps->opo_eqcmap); /*
                                            ** this probably will never be
                                            ** executed */
	}
    }
}

/*{
** Name: opj_exact	- convert all inexact orderings into exact orderings
**
** Description:
**      Since it is tricky to deal with inexact ordering i.e. unordered
**      sets of equivalence classes, this traversal of the CO tree will 
**      convert any such ordering into an exact list of equivalence
**      classes for query compilation.  The parent's join ordering and
**      output ordering are passed down, and this routine ensure that
**      the current CO node is compatible with these.
**         Note, that this routine is the complement to opj_maorder
**      since opj_maorder moves information from the leaves of the tree
**      to the root, while this routine will move the requirements of the
**      root to the leaves of the tree.
**
**	In addition, this is where we choose the specific join type for
**	simple join nodes: FSM, PSM, or hash join.  Enumeration assumes
**	that it's producing merge joins, as they are the most costly
**	and complex.  The assumption is that if a merge join is cheapest,
**	a hash join is surely even less expensive.
**
[@comment_line@]...
**
** Inputs:
**      subquery                        ptr to subquery state variable
**      cop                             ptr to CO node to make EXACT
**      psjeqc                          exact join ordering of parent
**                                      or OPE_NOEQCLS if it doesn't matter
**      pordeqc                         exact output ordering of parent
**                                      or OPE_NOEQCLS if it doesn't matter
**      valid_sjeqc                     TRUE means try to create ordering
**                                      on psjeqc and report a
**                                      consistency check if this cannot be
**                                      done
**      valid_ordeqc			TRUE means try to create ordering
**                                      on psjeqc and report a
**                                      consistency check if this cannot be
**                                      done
**
** Outputs:
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-mar-88 (seputis)
**          initial creation
**      23-sep-88 (seputis)
**          release key join consistency check rule
**      22-feb-89 (seputis)
**          do not assert ordeqc if outer is to be sorted
**      4-aug-89 (seputis)
**          b5947 - partial sort merge join should not have consistency
**          checking on the outer ordering
**      10-oct-89 (seputis)
**          add consistency checks for b7956
**      13-oct-90 (seputis)
**          fix b33878 - look for valid eqc when asking for ordering
**	19-dec-91 (rickh)
**	    Recursively call self with the correct arguments.
**	3-sep-98 (inkdo01)
**	    Changes to fine tune eqc bit maps for spatial query plans.
**	jan-00 (inkdo01)
**	    Code to enable hash joins under control of trace point op162.
**	23-june-00 (inkdo01)
**	    Code to pre-sort key join values (if enough of them).
**	22-mar-01 (inkdo01)
**	    Further check on hash join conversions to assure "order by"
**	    queries only hash join if there's no int. orderings (i.e. no top
**	    sort removal).
**	09-apr-01 (inkdo01)
**	    Further fine tuning of hash join conditions - no CO outer hash
**	    joins ('cause CO reverses roles and interferes with OJ semantics).
**	22-june-01 (inkdo01)
**	    Tune key join presort to fix bug 105110.
**	23-aug-01 (inkdo01)
**	    Another hash join condition - disallows nulljoins and joins 
**	    involving UDT columns.
**	14-mar-02 (inkdo01)
**	    Slight change to eliminate some sorts under CO hash joins.
**	3-aug-02 (inkdo01)
**	    Changes to allow swapping inner/outer of outer joins to place
**	    smaller source as hash join build source.
**	5-nov-02 (inkdo01)
**	    Minor change to hash join to prevent "sort on" notation in QEP.
**	17-jan-03 (inkdo01)
**	    Change to force FSM join to hash join when inner of FSM is
**	    also a hash join (since there is no predictable order).
**	29-may-03 (golden anniversary of climb of Everest) (inkdo01)
**	    Reinstate dups removal sort for outer source of CO hash join.
**	26-june-03 (inkdo01)
**	    Introduce pre-sort for Tjoins on large numbers of rows.
**	18-may-04 (inkdo01)
**	    Lower threshold for presorting Kjoins.
**	18-june-04 (inkdo01)
**	    Assure CO hash joins have sources reversed (algorithm depends on
**	    original outer source being the probe source of join).
**      11-jan-2005 (huazh01)
**          if TID join is the outer node of an outer join, we need
**          to save OJ's related equivalence class into 'opo_ojoin'. 
**          This fixes b113725, INGSRV3116.
**	2-mar-05 (inkdo01)
**	    Base switch of hash join sources on row sizes, as well as 
**	    row estimates.
**	25-june-05 (inkdo01)
**	    Don't presort Kjoin/Tjoin if "first n" select.
**      20-jul-2006 (huazh01)
**          create sort node on top of FSM join's outer/inner node, if they 
**          contain hash join. 
**          This fixes b116402. 
**	1-Sep-2006 (kschendel)
**	    Break up the hash join test to make it easier to work with.
**	    Allow hash join under OPS_FAGG if there's a top sort.
**	5-Dec-2006 (kschendel) SIR 122512
**	    If we choose hash join, join on the full set of join eqc's;
**	    there's no need to limit ourselves to the ordering restricted
**	    by lower sorts as merge join must, since sorts are irrelevant
**	    to hash joining.
**	8-Feb-2007 (kschendel) b122118
**	    Eliminate dup-removal sorts under the check-only input
**	    of a (CO) hash join.
**	8-jan-2008 (dougi)
**	    Fix "first n" check above for "first n" changes.
**	1-feb-2008 (dougi)
**	    Add op148 to disable T/Kjoin presorting.
**	11-mar-08 (hayke02)
**	    Back out the fix for bug 113725, and allow the fix for bug
**	    114467 to fix it instead. This change fixes bug 120084.
**	20-may-2008 (dougi)
**	    Add table procedure support - transform some joins to CP.
**	17-sep-08 (hayke02)
**	    Switch off hash joins for Ojs containing an equivalence class
**	    with varchar or nvarchar attributes (new boolean vchoj set TRUE).
**	    This change fixes bug 120656.
**	18-sep-08 (hayke02)
**	    Modify previous change so that we use cop->opo_sjeqc if it is
**	    less than maxeqcls (single eqcls join), else we use
**	    cop->opo_maps->opo_eqcmap (multi eqcls join).
**	24-Sep-08 (inkdo01)
**	    Remove change to switch off hash joins for varchar or nvarchar since
**	    it has been fixed elsewhere.
**	13-Nov-2009 (kiria01) SIR 121883
**	    Swap cardinality fields when inner and outer co nodes are reversed
**	    and updated call to opb_gbfbm to hand down co node to pick up
**	    cardinality info from the parse tree boolean factors.
*/
static VOID
opj_exact(
	OPJ_MASTATE        *mastatep,
	OPO_CO             *cop,
	OPO_ISORT          psjeqc,
	bool		   valid_sjeqc,
	OPO_ISORT          pordeqc,
	bool               valid_ordeqc)
{
    OPS_SUBQUERY *subquery;
    OPE_IEQCLS  maxeqcls;
    OPO_ISORT   inner_sjeqc;            /* ordering expected from inner
                                        ** to support join operation of
                                        ** current node */
    OPO_ISORT   outer_sjeqc;            /* ordering expected from outer
                                        ** to support join operation of
                                        ** current node */
    OPO_ISORT   inner_ordeqc;           /* ordering expected from inner
                                        ** to support ordering required
                                        ** by parent of this node */
    OPO_ISORT   outer_ordeqc;           /* ordering expected from outer
                                        ** to support ordering required
                                        ** by parent of this node */
    OPO_ISORT	outer;			/* outer ordering for sort node*/
    OPO_ISORT   inner;                  /* inner ordering for sort node*/
    bool	use_hashjoin;

    subquery = mastatep->opj_subquery;
    maxeqcls = subquery->ops_eclass.ope_ev;

    switch (cop->opo_sjpr)
    {
    case DB_ORIG:
    case DB_PR:
#ifdef	E_OP0496_EXACT
	if ((cop->opo_ordeqc >= maxeqcls)
	    &&
	    (	subquery->ops_msort.opo_base->opo_stable[cop->opo_ordeqc-maxeqcls]
		    ->opo_stype
		!= 
		OPO_SEXACT
	    ))
	    opx_error(E_OP0496_EXACT);  /* since inexact multi-attribute
                                        ** orderings only result from
                                        ** sort nodes, this assertion should
                                        ** execute opx_error */
#endif
	return;

    case DB_RSORT:
    {
	outer_sjeqc = cop->opo_sjeqc;
	outer_ordeqc = cop->opo_ordeqc;
	inner_ordeqc = cop->opo_ordeqc;
	inner_sjeqc = cop->opo_sjeqc;
	break;
    }
    case DB_SJ:
    {
	bool	    inner_orig;		/* TRUE if the inner is an ORIG node */

	cop->opo_tprocjoin = FALSE;	/* init */
	inner_orig = (cop->opo_inner->opo_sjpr == DB_ORIG);
	if ((psjeqc == OPE_NOEQCLS) && (pordeqc == OPE_NOEQCLS))
	{
	    cop->opo_storage = DB_HEAP_STORE;
	    cop->opo_ordeqc = OPE_NOEQCLS;  /* this ordering is not required
					** by parent so reset it, to save
					** computation and confusion */
	}
	if (cop->opo_psort)
	{   /* parent will reset orderings so ignore requirements of parent */
	    cop->opo_ordeqc = OPE_NOEQCLS; 
	    psjeqc = OPE_NOEQCLS;
            pordeqc = OPE_NOEQCLS;	/* reset the ordering if it is not
					** needed by the parent to avoid
					** confusion and computation */
	    cop->opo_storage = DB_HEAP_STORE; /* the current output ordering is
					** not used so reset the storedness
                                        ** indicator */
	}
	else if (cop->opo_ordeqc >= maxeqcls)
	{
	    opo_mexact(subquery, &cop->opo_ordeqc, psjeqc, valid_sjeqc, 
		pordeqc, valid_ordeqc, TRUE);
					/* a multi-attribute inexact ordering 
					** exists so convert this
					** into an exact list */
	    if (cop->opo_ordeqc == OPE_NOEQCLS)
		cop->opo_storage = DB_HEAP_STORE; /* reset output ordering
					** if it is not needed */
	}
	if (cop->opo_sjeqc >= maxeqcls)
	{
	    opo_mexact(subquery, &cop->opo_sjeqc, psjeqc, 
		valid_sjeqc && !inner_orig, /* join ordering
					** need to be compatible only if not a key
					** join, since key join does not change ordering */
		pordeqc, valid_ordeqc && !inner_orig, FALSE);
					/* a multi-attribute inexact ordering 
					** exists so convert this
					** into an exact list */
	}
	if (cop->opo_orig_sjeqc >= maxeqcls)
	{
	    /* The unreduced join ordering is created as an inexact ordering
	    ** by opo-fordering, earlier.  It needs to be an exact ordering
	    ** (ie a list of eqc's instead of just a bitmap), because that
	    ** is the form that opc expects.  Any ordering will do since
	    ** the unreduced ordering is only used by hash join, which
	    ** doesn't care.
	    */
	    opo_mexact(subquery, &cop->opo_orig_sjeqc,
			OPE_NOEQCLS, FALSE, OPE_NOEQCLS, FALSE, FALSE);
	}
	if (cop->opo_osort)		/* is outer sorted */
	{
	    outer_sjeqc = OPE_NOEQCLS;
	    outer_ordeqc = OPE_NOEQCLS; /* do not care about ordering
                                        ** provided by the outer node
                                        ** since sort will be placed
                                        ** on top */
	    outer = opo_fordering(subquery, &cop->opo_outer->opo_maps->opo_eqcmap);
	    if (outer >= maxeqcls)
	    {
		opo_mexact(subquery, &outer, cop->opo_sjeqc, !inner_orig, 
		    cop->opo_ordeqc, valid_ordeqc /* FALSE */, TRUE); 
					/* in a time out situation the parent
					** could request a sorted ordering
					** of this child which is provided
					** by the child */
					/* convert the outer ordering into
					** an exact list to be place into
					** the outer child, by convention
					** for query compilation
					** - since opo_sjeqc,opo_ordeqc have already
					** been determined for the parent, pass this
					** ordering to the child */
		if ((outer == OPE_NOEQCLS)
		    &&
		    (cop->opo_odups == FALSE) /* sort to remove duplicates is
					** OK */
		    )
		    opx_error(E_OP049B_SORT); /* unnecessary sort node specified
					** indicating some problem with enumeration
					** search space pruning */
	    }
	}
	else
	{
	    outer_sjeqc = cop->opo_sjeqc;
	    outer_ordeqc = cop->opo_ordeqc;
	}
	if (cop->opo_isort)		/* is inner sorted */
	{
	    inner_sjeqc = OPE_NOEQCLS;
	    inner_ordeqc = OPE_NOEQCLS;  /* do not care about ordering
                                        ** provided by the inner node
                                        ** since sort will be placed
                                        ** on top */
	    inner = opo_fordering(subquery, &cop->opo_inner->opo_maps->opo_eqcmap);
	    if (inner >= maxeqcls)	/* FIXME - can add consistency check
					** for FALSE case of opo_ordeqc */
	    {
		opo_mexact(subquery, &inner, cop->opo_sjeqc, TRUE, 
		    cop->opo_ordeqc, FALSE, TRUE);
					/* convert the inner ordering into
					** an exact list to be placed into
					** the inner child, by convention
					** for query compilation, note that
                                        ** the inner ordeqc may be required
                                        ** if the outer ordeqc includes all
                                        ** the outer eqcls and is a subset of
                                        ** the inner, in this case the remaining
                                        ** inner eqcls will also be ordered */
		if ((inner == OPE_NOEQCLS)
		    &&
		    (cop->opo_idups == FALSE) /* sort to remove duplicates is
					** OK */
		    )
		    opx_error(E_OP049B_SORT); /* unnecessary sort node specified
					** indicating some problem with enumeration
					** search space pruning */
	    }
	}
	else
	{
	    inner_ordeqc = cop->opo_ordeqc;
					/* inner does not need to be ordered
                                        ** more than the joining eqcls
                                        ** FIXME - there is possible usefulness
                                        ** to having the inner sorted if all
					** attributes of the outer are a subset of the
                                        ** inner */
	    inner_sjeqc = cop->opo_sjeqc;
	}
	break;
    }
    default:
	opx_error(E_OP0495_CO_TYPE);
    }
    {
	/* for outer joins - convert any TID outer join to a different
	** outer join ID since OPC cannot deal with 2 join nodes with
	** the same joinid */
	OPO_CO	    *innerp;
	OPO_CO	    *outerp;

	if (innerp = cop->opo_inner)
	{
	    valid_sjeqc = (innerp->opo_sjpr != DB_ORIG); /* this flag
					** indicates the a key or TID join
                                        ** is being considered so it is not
                                        ** necessary that the outer/inner ordering
                                        ** needs to be compatible with the
                                        ** join ordering */
	    opj_exact(mastatep, innerp, inner_sjeqc, valid_sjeqc, 
		inner_ordeqc, FALSE);   /* do not depend on inner order but
                                        ** try to use it */
	}

        if (outerp = cop->opo_outer)
        {
            opj_exact(mastatep, outerp, outer_sjeqc, FALSE /* valid_sjeqc */,
                outer_ordeqc, (outer_ordeqc != OPE_NOEQCLS) /* TRUE */);
        }
     }


    /* b116402:
    **
    ** child node's FSM join has been replaced by a hash join.
    **
    ** we can just change change the cop->opo_storage from DB_SORT_STORE to
    ** DB_HEAP_STORE. This is because rows coming out from hash join is not 
    ** in sorted order, but doing that may cause some performance issue 
    ** because the parent node will attach PSM join to its 'jtype' once it 
    ** know its child node has a DB_HEAP_STORE storage structure.  
    ** 
    ** instead of setting opo_storage to DB_HEAP_STORE, we now enable 
    ** and preserve the OPO_HASH_TRANSFORM flag until we attach a FSM join 
    ** to the parent node. At that time, we will check
    ** this flag and will create a sort node if it is necessary.  
    */
    if (cop->opo_storage == DB_SORT_STORE &&
        ((cop->opo_outer &&
         (cop->opo_outer->opo_variant.opo_local->opo_jtype == OPO_SJHASH ||
         cop->opo_outer->opo_variant.opo_local->opo_mask & OPO_HASH_TRANSFORM)) 
         ||
        (cop->opo_inner && 
         (cop->opo_inner->opo_variant.opo_local->opo_jtype == OPO_SJHASH ||
         cop->opo_inner->opo_variant.opo_local->opo_mask & OPO_HASH_TRANSFORM))))
    {
        cop->opo_variant.opo_local->opo_mask |=  OPO_HASH_TRANSFORM; 
    }
    

    if (cop->opo_osort)			/* is outer sorted */
	cop->opo_outer->opo_ordeqc = outer; /* place sorting ordering into
                                        ** child for query compilation */
    if (cop->opo_isort)			/* is inner sorted */
	cop->opo_inner->opo_ordeqc = inner; /* place sorting ordering into
                                        ** child for query compilation */
    {
	OPO_CO	    *inner1p;
	OPO_JTYPE   jtype;
	bool	    dep_tproc = FALSE;

	inner1p = cop->opo_inner;
	if (inner1p)
	{   /* this is a join node, so mark the appropriate type of join
	    ** for OPC */

	    /* See if inner is a table procedure with parameter dependencies.
	    ** If so, we set dep_tproc to force a SEjoin. */
	    if (subquery->ops_mask2 & OPS_TPROC)
	    {
		OPO_CO	*i2p = inner1p;
		OPV_BMVARS	*parmmap;
		i4	i;

		if (i2p->opo_sjpr == DB_PR)
		    i2p = i2p->opo_outer;

		if (i2p->opo_sjpr == DB_ORIG && i2p->opo_storage == DB_TPROC_STORE	
		    && (parmmap = subquery->ops_vars.opv_base->opv_rt[
			i2p->opo_union.opo_orig]->opv_parmmap) != NULL
		    && BTcount((char *)parmmap, subquery->ops_vars.opv_rv) > 0)
		    dep_tproc = TRUE;
		/* Next see if TPROC is amongst inners and its parms come 
		** from outers of the join. */
		if (!dep_tproc)
		 for (i = -1; (i = BTnext(i, (PTR)i2p->opo_variant.opo_local->
				opo_bmvars, subquery->ops_vars.opv_rv)) >= 0; )
		 {
		    parmmap = subquery->ops_vars.opv_base->opv_rt[i]->
								opv_parmmap;
		    if (parmmap && BTsubset((PTR)parmmap, (PTR)cop->opo_outer->
			opo_variant.opo_local->opo_bmvars, 
			subquery->ops_vars.opv_rv))
		    {
			dep_tproc = TRUE;
			break;
		    }
		 }
		if (dep_tproc && (subquery->ops_duplicates != OPS_DREMOVE ||
		    subquery->ops_bestco->opo_sjpr == DB_RSORT))
		{
		    cop->opo_osort = cop->opo_isort = FALSE;
		}
	    }

	    if (cop->opo_sjeqc == OPE_NOEQCLS || dep_tproc)
	    {   /* cartesean product */
		if (dep_tproc)
		    cop->opo_tprocjoin = TRUE;
		if (inner1p->opo_sjpr == DB_ORIG)
		{
		    /* Check for subselect join */
		    if (subquery->ops_vars.opv_base->
			opv_rt[inner1p->opo_union.opo_orig]->opv_subselect )
			jtype = OPO_SEJOIN; /* subselect join */
		    else
		    {	/* this is a physical join which OPC deals with as a
			** cartesian product so create a CO node
			** for the project restrict */
			OPO_CO	    *pjoin_cop;
			pjoin_cop = (OPO_CO *) opu_memory(subquery->ops_global,
			    (i4)(sizeof(*pjoin_cop) + sizeof(*pjoin_cop->opo_variant.opo_local)));
			MEcopy((PTR)inner1p, sizeof(*pjoin_cop), (PTR)pjoin_cop);
			pjoin_cop->opo_variant.opo_local = (OPD_LOCAL *)(&pjoin_cop[1]);
			MEcopy((PTR)inner1p->opo_variant.opo_local, 
			    sizeof(*inner1p->opo_variant.opo_local), 
			    (PTR)pjoin_cop->opo_variant.opo_local);
			pjoin_cop->opo_variant.opo_local->opo_id 
			    = ++mastatep->opj_nodeid; /* define
					** a nodeid for this CO node */
			pjoin_cop->opo_sjpr = DB_PR;
			pjoin_cop->opo_outer = inner1p;
			cop->opo_inner = pjoin_cop;
			jtype = OPO_SJCARTPROD; /* cartesean product */
		    }
		}
		else
		    jtype = OPO_SJCARTPROD; /* cartesean product */
	    }
	    else
	    {
		if (inner1p->opo_sjpr == DB_ORIG)
		{   /* keying will be done */
		    OPV_VARS            *var2_ptr;
		    var2_ptr = subquery->ops_vars.opv_base->
			opv_rt[inner1p->opo_union.opo_orig]; /* get ptr to range 
					** table element */
		    if( var2_ptr->opv_subselect )
			jtype = OPO_SEJOIN; /* subselect join */
		    else if( var2_ptr->opv_primary.opv_tid != cop->opo_sjeqc)
			jtype = OPO_SJKEY;	/* key join */
		    else
			jtype = OPO_SJTID;	/* TID join */
		}
		else if (cop->opo_osort)
		    jtype = OPO_SJFSM;
		else if (cop->opo_outer->opo_storage == DB_SORT_STORE)
		{   /* check to make sure that the opo_sjeqc is a subset
		    ** of the sorted ordering provided by the outer */
		    OPO_ST              *obase;         /* base of multi-attribute
							** ordering descriptors */
		    obase = subquery->ops_msort.opo_base; /* ptr to sort descriptors */

		    if (cop->opo_sjeqc < maxeqcls)
		    {
			OPE_IEQCLS	eqcls;

			if (inner1p->opo_ordeqc < maxeqcls)
			    eqcls = inner1p->opo_ordeqc;
			else
			    eqcls = obase->opo_stable[inner1p->opo_ordeqc-maxeqcls]->
				opo_eqlist->opo_eqorder[0];
			if (cop->opo_sjeqc == eqcls)
			    jtype = OPO_SJFSM;
			else
			    jtype = OPO_SJPSM;
		    }
		    else if (inner1p->opo_ordeqc < maxeqcls)
			jtype = OPO_SJPSM;
		    else
		    {	/* compare multiattribute lists and make sure that 
			** sjeqc is a subset of the outer ordering 
			*/
			OPO_ISORT  *eqcls1p;
			OPO_ISORT  *eqcls2p;
			eqcls1p = &obase->opo_stable[inner1p->opo_ordeqc-maxeqcls]->
			    opo_eqlist->opo_eqorder[0];
			eqcls2p = &obase->opo_stable[cop->opo_sjeqc-maxeqcls]->
			    opo_eqlist->opo_eqorder[0];
			for(;(*eqcls1p == *eqcls2p) && (*eqcls2p != OPE_NOEQCLS);
			    eqcls1p++, eqcls2p++)
			    ;
			if (*eqcls2p == OPE_NOEQCLS)
			    jtype = OPO_SJFSM;
			else
			    jtype = OPO_SJPSM;
		    }
		}
		else
		    jtype = OPO_SJPSM;		/* outer is not sorted */
	    }

	    /* Check for hash join in prototype implementation. */
	    use_hashjoin = FALSE;
	    if (subquery->ops_global->ops_cb->ops_alter.ops_hash &&
		(jtype == OPO_SJPSM || jtype == OPO_SJFSM) &&
		!cop->opo_odups && !cop->opo_idups)
	    {
		/* Worth additional testing.  This could be a giant if
		** conditional, but it was getting too hard to work with.
		*/

		/* Don't subvert necessary orderings:  sorted aggs
		** without a topsort, or topsort-needed without a
		** topsort.  This join might be producing the needed
		** ordering.
		*/
		use_hashjoin = TRUE;
		if (subquery->ops_bestco->opo_sjpr != DB_RSORT
		    && (subquery->ops_sqtype == OPS_FAGG
		    || subquery->ops_sqtype == OPS_RFAGG
		    || (subquery->ops_msort.opo_mask & OPO_MTOPSORT)) )
		  use_hashjoin = FALSE;

		if (use_hashjoin && jtype == OPO_SJFSM
		    /* More testing for FSM.  Keep FSM join which sorts
		    ** neither source, and sources are not themselves
		    ** hash joins (transformed from merge joins).  (We
		    ** replace from the bottom up).
		    */
		    && !cop->opo_isort
		    && (cop->opo_inner->opo_sjpr != DB_SJ
		    || cop->opo_inner->opo_variant.opo_local->opo_jtype != OPO_SJHASH)
		    && !cop->opo_osort
		    && (cop->opo_outer->opo_sjpr != DB_SJ
		    || cop->opo_outer->opo_variant.opo_local->opo_jtype != OPO_SJHASH))
		{
		    use_hashjoin = FALSE;
		}
		/* Verify that data types are OK, no nulljoins.
		** Can't (at present) have CO join and outer join at the
		** same time.
		*/
		if (use_hashjoin
		    && (! opj_ckhasheqc(subquery, cop->opo_sjeqc)
		    || (cop->opo_existence && cop->opo_union.opo_ojid != OPL_NOOUTER)) )
		  use_hashjoin = FALSE;
	    }

	    if (use_hashjoin)
	    {
		OPS_WIDTH	outsz, insz;

		/* If all above conditions hold, change to hash join and
		** turn off outer/inner sorts (though that might screw up
		** higher interesting orders - so we'll be more clever later).
		** We can use the full set of joining eqc's, don't need to
		** be restricted to pre-existing sort orders like a merge
		** join would be.
		** Reset the result ordeqc since the output is not ordered;
		** at this late point in the process, nobody cares (I think!),
		** but it prevents bogus "partial(att)" annotations in the
		** QEP display.
		*/
		jtype = OPO_SJHASH;
		cop->opo_storage = DB_HEAP_STORE;
		cop->opo_sjeqc = cop->opo_orig_sjeqc;
		cop->opo_ordeqc = OPE_NOEQCLS;
		/* Dups removal sorts are left under the hash join (unless
		** there's a sort on the very top). All other sorts are 
		** removed (that being the advantage of hash join). */
		if (subquery->ops_duplicates != OPS_DREMOVE ||
		    subquery->ops_bestco->opo_sjpr == DB_RSORT)
		{
		    cop->opo_osort = cop->opo_isort = FALSE;
		}

		outsz = ope_twidth(subquery, &cop->opo_outer->
					opo_maps->opo_eqcmap);
		insz = ope_twidth(subquery, &cop->opo_inner->
					opo_maps->opo_eqcmap);
		/* Make larger source the probe (inner) source.
		** Also, if check-only join, we have the check-only
		** (non-passthru) source on the inner since that's
		** how merge joins like it.  For hash joins, the CO
		** source has to be on the outer (build) side, so
		** that the passthru source can be the probing source.
		*/
		if (cop->opo_existence
		  || (cop->opo_outer->opo_cost.opo_tups*outsz >
				cop->opo_inner->opo_cost.opo_tups*insz
		      && (subquery->ops_mask2 & (OPS_ORDHINT | OPS_TPROC)) == 0))
		{
		    bool	tempsort;
		    OPO_CO	*tempco;
		    u_i4	temp;

		    /* Swap inner for outer */
		    tempco = cop->opo_outer;
		    cop->opo_outer = cop->opo_inner;
		    cop->opo_inner = tempco;
		    /* Swap osort/isort just in case one is still on */
		    tempsort = cop->opo_osort;
		    cop->opo_osort = cop->opo_isort;
		    cop->opo_isort = tempsort;
		    /* Swap cardinality assertions */
		    temp = cop->opo_card_outer;
		    cop->opo_card_outer = cop->opo_card_inner;
		    cop->opo_card_inner = temp;
		    /* Swap outer join style if outer join. */
		    if (cop->opo_union.opo_ojid != OPL_NOOUTER)
		    {
			OPO_OJEQCDEF	*eqc1p;
			OPO_OJEQCDEF	*eqc2p;
			OPO_OJEQCDEF	tempeqc;

			eqc1p = &cop->opo_variant.opo_local->opo_ojlnull;
			eqc2p = &cop->opo_variant.opo_local->opo_ojrnull;
			STRUCT_ASSIGN_MACRO(*eqc1p, tempeqc);
			STRUCT_ASSIGN_MACRO(*eqc2p, *eqc1p);
			STRUCT_ASSIGN_MACRO(tempeqc, *eqc2p);
			/* left join <-> right join; watch out for full join */
			if (cop->opo_variant.opo_local->opo_type == OPL_LEFTJOIN)
			    cop->opo_variant.opo_local->opo_type = OPL_RIGHTJOIN;
			else if (cop->opo_variant.opo_local->opo_type ==
				    OPL_RIGHTJOIN)
			    cop->opo_variant.opo_local->opo_type = OPL_LEFTJOIN;
		    }
		}
		if (cop->opo_existence)
		{
		    /* Eliminate any sort at all under the check-only
		    ** side of a (CO) hash join.  Yes, it might remove
		    ** dups, but the (CO) semantics takes care of that.
		    ** For small inputs the dups removal isn't a big
		    ** deal, while for large inputs the dups-removal
		    ** sort can be a killer.
		    ** Re-check the inner as well, we can kill its sort
		    ** unless it's dups removing.  (Might have missed an
		    ** inner sort above, because of DREMOVE on the entire
		    ** subquery.)
		    */
		    cop->opo_osort = FALSE;
		    if (cop->opo_inner->opo_pdups)
			cop->opo_isort = cop->opo_inner->opo_psort;
		    else cop->opo_isort = FALSE;
		}
		if (!cop->opo_osort)
		{
		    cop->opo_outer->opo_psort = FALSE;
		    cop->opo_outer->opo_ordeqc = OPE_NOEQCLS;
		}
		if (!cop->opo_isort)
		{
		    cop->opo_inner->opo_psort = FALSE;
		    cop->opo_inner->opo_ordeqc = OPE_NOEQCLS;
		}
	    }


            /* bug 116402:
            ** we've picked a FSM join because we think child nodes
            ** are sorted, but if children below this FSM join contains
            ** a hash join, then rows coming out from those nodes may
            ** not be in the sorted order.
            **
            ** creates a sort node on top of the inner/outer node
            ** to ensure FSM works.
            */
            if (jtype == OPO_SJFSM &&
                (cop->opo_outer->opo_variant.opo_local->opo_mask &
                 OPO_HASH_TRANSFORM
                 ||
                 cop->opo_inner->opo_variant.opo_local->opo_mask &
                 OPO_HASH_TRANSFORM))
            {
               if (cop->opo_outer->opo_variant.opo_local->opo_mask &
                   OPO_HASH_TRANSFORM)
                   cop->opo_osort = TRUE;

               if (cop->opo_inner->opo_variant.opo_local->opo_mask &
                   OPO_HASH_TRANSFORM)
                   cop->opo_isort = TRUE;

               cop->opo_outer->opo_variant.opo_local->opo_mask &=
                       ~OPO_HASH_TRANSFORM;

               cop->opo_inner->opo_variant.opo_local->opo_mask &=
                       ~OPO_HASH_TRANSFORM;
            }

	    cop->opo_variant.opo_local->opo_jtype = jtype;

#ifdef    OPT_F020_NOPRESORT
	    if (
		(!subquery->ops_global->ops_cb->ops_check 
		|| 
		!opt_strace( subquery->ops_global->ops_cb, OPT_F020_NOPRESORT )
		)
	    )
#endif
	    {
		/* If op148 is not on ... */

		/* Can't hurt to sort KEY join outer input into key sequence.
		** Just make sure join eqclass is covered by outer source and
		** only do it when the ratio of input rows to target pages 
		** is high (i.e., the kjoin will benefit from presorting). */
		if (jtype == OPO_SJKEY && !(cop->opo_osort) &&
		    cop->opo_ordeqc == OPE_NOEQCLS &&
		    (cop->opo_sjeqc < maxeqcls && 
		    BTtest((i4)cop->opo_sjeqc, 
			(char *)&cop->opo_outer->opo_maps->opo_eqcmap) ||
		    cop->opo_sjeqc >= maxeqcls &&
		    subquery->ops_global->ops_qheader->pst_firstn != NULL &&
		    BTsubset((char *)subquery->ops_msort.opo_base->opo_stable
			[cop->opo_sjeqc-maxeqcls]->opo_bmeqcls,
			(char *)&cop->opo_outer->opo_maps->opo_eqcmap, 
			(i4)BITS_IN(cop->opo_maps->opo_eqcmap))) ) /*&&
		    (cop->opo_inner->opo_cost.opo_reltotb <
			cop->opo_outer->opo_cost.opo_tups*4 ||
		    cop->opo_inner->opo_cost.opo_reltotb >
			OPJ_KJOINPRESORT_THRESHOLD))
*/
		{
		    cop->opo_osort = TRUE;
		    cop->opo_outer->opo_ordeqc = cop->opo_sjeqc;
		}

		/* Possibly pre-sort TID join, too. Again, make sure that
		** join eqclass is covered by outer source and do it when
		** number of target pages is quite large. */
		if (jtype == OPO_SJTID && !(cop->opo_osort) &&
		    cop->opo_ordeqc == OPE_NOEQCLS &&
		    subquery->ops_global->ops_qheader->pst_firstn != NULL &&
		    (cop->opo_sjeqc < maxeqcls && 
		    BTtest((i4)cop->opo_sjeqc, 
			(char *)&cop->opo_outer->opo_maps->opo_eqcmap) ||
		    cop->opo_sjeqc >= maxeqcls &&
		    BTsubset((char *)subquery->ops_msort.opo_base->opo_stable
			[cop->opo_sjeqc-maxeqcls]->opo_bmeqcls,
			(char *)&cop->opo_outer->opo_maps->opo_eqcmap, 
			(i4)BITS_IN(cop->opo_maps->opo_eqcmap))) &&
		    cop->opo_inner->opo_cost.opo_reltotb > OPJ_TJOINPRESORT_THRESHOLD)
		{
		    cop->opo_osort = TRUE;
		    cop->opo_outer->opo_ordeqc = cop->opo_sjeqc;
		}
	    }

	    /* Another ugly Rtree hack - eqc's may appear above a spatial join, but
	    ** below the corresponding TID join to permit other predicates on the 
	    ** Rtree-indexed column from the spatial join. Drop such eqc's until
	    ** the TID join rolls along. */
	    if (subquery->ops_mask & OPS_SPATF && !(jtype == OPO_SJTID ||
		jtype == OPO_SJKEY && cop->opo_inner->opo_sjpr == DB_ORIG &&
		cop->opo_inner->opo_storage == DB_RTRE_STORE))
	    {
		OPE_BMEQCLS	*eqcmap;
		OPE_IEQCLS	eqcno;
		OPZ_IATTS	attno;
		OPZ_ATTS	*attp;
		OPV_IVARS	varno;

		eqcmap = &cop->opo_maps->opo_eqcmap;
		for (eqcno = -1; (eqcno = BTnext((i4)eqcno, (char *)eqcmap,
			(i4)maxeqcls)) != -1; )
		{   /* loop over eqc's in question */
		    for (attno = -1; (attno = BTnext((i4)attno,
		     (char *)&subquery->ops_eclass.ope_base->ope_eqclist[eqcno]->
									ope_attrmap,
		     (i4)subquery->ops_attrs.opz_av)) != -1; )
		    {   /* loop over attrs in eqclass */
			attp = subquery->ops_attrs.opz_base->opz_attnums[attno];
			varno = attp->opz_varnm;
			if (attp->opz_attnm.db_att_id == 1 && 
			subquery->ops_vars.opv_base->opv_rt[varno]->
						opv_mask & OPV_SPATF &&
			BTtest((i4)varno, (char *)cop->opo_variant.opo_local->
									opo_bmvars))
			{	
			    /* If attr is key column of an Rtree ix used lower in
			    ** plan, remove eqc from map */
			    BTclear((i4)eqcno, (char *)eqcmap);
			    break;
			}
		    }
     		}	/* end of eqclass loop for spatial joins */
	    } /* if spatial */
	} /* if join node */
    }
}


/*{
** Name: opj_ojeqcmaps	- init eqc maps for outer join operation
**
** Description:
**      This routine will initialize the eqc maps needed by OPC
**	in order to determine correct processing of each eqc when 
**      the outer join fails 
**
** Inputs:
**      subquery                        ptr to subquery being processed
**      oeqcmap                         ptr to outer equivalence classes
**      ivmap                           map of variables on inner
**      ieqcmap                         ptr to inner equivalence classes
**
** Outputs:
**      ojdefp                          ptr to eqcmaps to initialize
**
**	Returns:
**	    VOID
**	Exceptions:
**
** Side Effects:
**
** History:
**      21-may-93 (ed)
**          initial creation - specify special equivalence handling
[@history_template@]...
*/
static VOID
opj_ojeqcmaps(
	OPS_SUBQUERY	*subquery, 
	OPO_OJEQCDEF	*ojdefp,
	OPE_BMEQCLS	*oeqcmap,
	OPV_BMVARS	*ivmap,
	OPE_BMEQCLS	*ieqcmap)
{
    OPE_IEQCLS	    maxeqcls;
    OPV_RT	    *vbase;
    OPV_IVARS	    maxvar;
    OPV_IVARS	    varno;

    maxvar = subquery->ops_vars.opv_rv;
    maxeqcls = subquery->ops_eclass.ope_ev;
    vbase = subquery->ops_vars.opv_base;

    /* init map of special equivalence classes which need to be
    ** set to 0 */
    ojdefp->opo_zeroeqcmap = (OPE_BMEQCLS *)opu_memory(subquery->
	ops_global, (i4)sizeof(*(ojdefp->opo_zeroeqcmap)));
    for (varno = -1; (varno = BTnext((i4)varno, (char *)ivmap, 
	(i4)maxvar))>=0;)
    {	/* init map of special eqc's which need to be set to
	** zero since outer join failed */
	OPE_IEQCLS	special;
	special = vbase->opv_rt[varno]->opv_ojeqc;
	if (special >= 0)
	    BTset((i4)special, (char *)ojdefp->opo_zeroeqcmap);
    }

    /* init map of equivalence classes which should be set to NULL */
    ojdefp->opo_nulleqcmap = (OPE_BMEQCLS *)opu_memory(subquery->
	ops_global, (i4)sizeof(*(ojdefp->opo_nulleqcmap)));
    MEcopy((PTR)oeqcmap, sizeof(*oeqcmap), 
	(PTR)ojdefp->opo_nulleqcmap);
    BTor((i4)maxeqcls, (char *)ojdefp->opo_zeroeqcmap,
	(char *)ojdefp->opo_nulleqcmap);
    BTnot((i4)BITS_IN(*(ojdefp->opo_nulleqcmap)), 
	(char *)ojdefp->opo_nulleqcmap);
    BTand((i4)maxeqcls, (char *)ieqcmap, 
	(char *)ojdefp->opo_nulleqcmap);

    /* init map of equivalence classes which should be refreshed */
    ojdefp->opo_refresheqcmap = (OPE_BMEQCLS *)opu_memory(subquery->
        ops_global, (i4)sizeof(*(ojdefp->opo_refresheqcmap)));
    MEcopy((PTR)oeqcmap, sizeof(*oeqcmap), 
	(PTR)ojdefp->opo_refresheqcmap);
}

/*{
** Name: opb_bfnewget	- allocate and initialize a new boolfact ptr
**
** Description:
**      In order to process outer joins, new boolean factors need to be
**	added for various filtering purposes
**
** Inputs:
**      subquery                        ptr to subquery being processed
**
** Outputs:
**
**	Returns:
**	    ptr to boolean factor ptr
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      22-apr-94 (ed)
**          initial creation for b59588
[@history_template@]...
*/
static OPB_BOOLFACT *
opb_bfnewget(
	    OPS_SUBQUERY	*subquery)
{
    OPB_BOOLFACT	*new_bfp;
    new_bfp = (OPB_BOOLFACT *)opu_memory(subquery->ops_global, 
	(i4)sizeof(*new_bfp));
    if (!subquery->ops_bfs.opb_newbv)
    {   /* first time new boolean factor allocated */
	subquery->ops_bfs.opb_newbv = subquery->ops_bfs.opb_bv;
    }
    new_bfp->opb_bfindex = subquery->ops_bfs.opb_newbv++;
    if ( subquery->ops_bfs.opb_newbv >= OPB_MAXBF)
	opx_error( E_OP0302_BOOLFACT_OVERFLOW );
    opb_bfget(subquery, new_bfp);
    subquery->ops_bfs.opb_mask |= OPB_NEWOJBF; /* at least one new boolean
				** factor introduced by OJ handling */
    return(new_bfp);
}

/*{
** Name: opl_findiftrue	- routine to support iftrue
**
** Description:
**      This routine helps determine where the iftrue function
**      needs to be inserted and creates new boolean 
**      factor descriptors since the boolean factor is evaluated 
**      at differing points in the co tree with differing 
**      specialeqcls available.
**
**	select * from (r left join (s left join t on t.i=s.i) on r.i=s.i) 
**	    where t.i is null or r.i=0;
**	on the above query the where clause needs to be evaluated at
**	several nodes, and not just on the project restrict for t.
**	At the PR for "t" no iftrue can be present but at subsequent
**	join nodes, the iftrue function must be used to get the
**	correct NULL value for the coalesced attribute, and in fact
**	at every join node including the root the boolean factor needs
**	to be reevaluated
**	
**
** Inputs:
**      subquery                        ptr to subquery to analyze
**      cop                             ptr to cop node containing
**					boolean factors to analyze
**
** Outputs:
**
**	Returns:
**	    TRUE - if a new boolean factor descriptor is needed
**	Exceptions:
**
** Side Effects:
**
** History:
**      5-aug-93 (ed)
**          initial creation
**	14-feb-94 (ed)
**	    - fix bug 59598 - wrong answer outer join problem, caused
**	    by boolean factor not being duplicated properly, and modified
**	    by the iftrue functions
[@history_template@]...
*/
static VOID
opl_findiftrue(
	OPS_SUBQUERY	    *subquery,
	OPO_CO		    *cop)
{
    /* check special eqcls needed to evaluate boolean factor with 
    ** previous cop are the same as the current cop */
    OPZ_IATTS	maxattr;
    OPZ_IATTS	attr;
    OPV_RT	*vbase;
    OPZ_AT	*abase;
    OPE_BMEQCLS	special;
    bool	first_time;
    OPB_BFT	*bfbase;
    OPB_IBF	ibf;
    OPB_BOOLFACT *bfp;
    OPB_IBF	maxbf;
    OPB_BMBF	*bfbmp;
    OPL_OJT	*lbase;

    lbase = subquery->ops_oj.opl_base;
    abase = subquery->ops_attrs.opz_base;
    vbase = subquery->ops_vars.opv_base;
    maxattr = subquery->ops_attrs.opz_av;
    /* for boolean factors which need to have an iftrue function added
    ** in order to obtain the correct NULL semantics */
    bfbase = subquery->ops_bfs.opb_base;
    maxbf =  subquery->ops_bfs.opb_bv;
    bfbmp = cop->opo_variant.opo_local->opo_bmbf;

    for (ibf = -1; (ibf = BTnext((i4)ibf, (char *)bfbmp, (i4)maxbf))>= 0;)
    {
	OPB_BOOLFACT	    *bfp;
	OPB_BOOLFACT	    *new_bfp;

	bfp = bfbase->opb_boolfact[ibf];
	if (bfp->opb_mask & OPB_OJJOIN)
	{   /* do not evaluate boolean factors which are represented
	    ** in equi-joins, these were used for relation placement only */
	    BTclear ((i4)ibf, (char *)bfbmp);
	    continue;
	}
	if (!(bfp->opb_mask & OPB_OJATTREXISTS))
	    continue;

	first_time = TRUE;
	for (attr = -1; (attr = BTnext((i4)attr,
	    (char *)bfp->opb_ojattr, (i4)maxattr))>=0;)
	{
	    bool	    cop_iftrue;
	    OPE_IEQCLS	    ojeqcls;	/* special eqc associated with
				** relation containing this attribute */
	    ojeqcls = vbase->opv_rt[abase->opz_attnums[attr]->opz_varnm]
		->opv_ojeqc;
	    if (ojeqcls == OPE_NOEQCLS)
		continue;	    /* not an inner table to any join */
	    /* verify that this eqcls is available or not available in both
	    ** cases, otherwise different boolean factors are needed,
	    ** one with the special eqcls test and one without */
	    cop_iftrue =
		(cop->opo_sjpr == DB_SJ)
		&&
		(
		    BTtest((i4)ojeqcls, (char *)&cop->opo_outer->opo_maps->opo_eqcmap)
		    ||		/* if special eqc is in outer then
				    ** create an iftrue function */
		    BTtest((i4)ojeqcls, (char *)&cop->opo_inner->opo_maps->opo_eqcmap)
		    ||		    /* if special eqc is in inner then
				    ** create an iftrue function */
		    (   (cop->opo_union.opo_ojid != bfp->opb_ojid)
			&&
			(cop->opo_union.opo_ojid >= 0)
			&&
			BTtest((i4)ojeqcls, (char *)lbase->opl_ojt
				[cop->opo_union.opo_ojid]->opl_innereqc)
				    /* is this is an outer join and the boolean
				    ** factor does not match the outer join
				    ** id, then use the iftrue function
				    ** if the eqcls becomes available
				    ** after evaluating this node */
		    )
		);
	    if (cop_iftrue)
	    {   /* true if iftrue function needed for this boolean factor */
		if (first_time)
		    MEfill(sizeof(special), (u_char)0, (PTR)&special);
		BTset((i4)ojeqcls, (char *)&special);
		first_time =FALSE;
	    }
	}
	for (new_bfp = bfp; new_bfp; new_bfp = new_bfp->opb_bfp)
	{   /* attempt to find boolean factor compatible with this boolean factor
	    ** otherwise create a new one */
	    if (first_time && !(new_bfp->opb_special))
	    {   /* found case of no special equivalence classes being used so
		** reserve this boolean factor */
		new_bfp->opb_mask |= OPB_NOSPECIAL;
		break;
	    }
	    if ((new_bfp->opb_special)
		&&
		!MEcmp((PTR)&special, (PTR)new_bfp->opb_special, sizeof(special)))
		break;	    /* found boolean factor with similiar special
				** eqcls requirements */
	}
	if (new_bfp)
	{   /* found a match so check if the same boolean factor number is being used
	    ** in this case */
	    if (new_bfp != bfp)
	    {
		BTclear((i4)bfp->opb_bfindex, (char *)bfbmp); /* clear old boolean
				** factor */
		BTset((i4)new_bfp->opb_bfindex, (char *)bfbmp);
	    }
	    continue;
	}
	else if (!bfp->opb_special && !(bfp->opb_mask & OPB_NOSPECIAL))
	{	/* boolean factor not reserved so use it */
	    new_bfp = bfp;
	}
	else
	{   /* boolean factors already reserved and cannot find a match */
	    OPB_IBF	save_bfi;

	    new_bfp = opb_bfnewget(subquery);
	    save_bfi = new_bfp->opb_bfindex;
	    MEcopy((PTR)bfp, sizeof(*new_bfp), (PTR)new_bfp);
	    opv_copytree(subquery->ops_global, &new_bfp->opb_qnode);
	    new_bfp->opb_bfindex = save_bfi;
	    BTclear((i4)bfp->opb_bfindex, (char *)bfbmp); /* clear old boolean
			    ** factor */
	    BTset((i4)new_bfp->opb_bfindex, (char *)bfbmp);
	    bfp->opb_bfp = new_bfp;
	}
	if (first_time)
	{	/* case of no special equivalence classes being used */
	    new_bfp->opb_mask |= OPB_NOSPECIAL;
	    new_bfp->opb_special = NULL;
	}
	else
	{	/* at least one special equivalence class is used */
	    new_bfp->opb_mask &= (~OPB_NOSPECIAL);
	    new_bfp->opb_special = (OPE_BMEQCLS *)opu_memory(subquery->ops_global, 
		(i4)sizeof(special));
	    MEcopy((PTR)&special, sizeof(special), (PTR)new_bfp->opb_special);
	    subquery->ops_bfs.opb_mask |= OPB_NEWOJBF; /* at least one boolean
				** factor needs to be traversed to use the iftrue
				** function */
	}
    }
}

/*{
** Name: opl_insertiftrue	- insert iftrue function into boolean factor
**
** Description:
**      traverse the parse tree fragment and insert the iftrue function on
**	all var nodes which reference variables which have special equivalence
**      classes in the provided map. 
**
** Inputs:
**      subquery                        ptr to subquery to analyze
**      qnodepp                         ptr to ptr to parse tree node
**      specialp                        ptr to map of special eqcls
**
** Outputs:
**
**	Returns:
**	    
**	Exceptions:
**
** Side Effects:
**
** History:
**      5-aug-93 (ed)
**          initial creation
[@history_template@]...
*/
static VOID
opl_insertiftrue(
	OPS_SUBQUERY	    *subquery,
	PST_QNODE	    **qnodepp,
	OPB_BOOLFACT	    *bfp)
{
    for (;*qnodepp; qnodepp = &(*qnodepp)->pst_left)
    {	/* use iteration on target list so that stack does not
	** overflow */
	if ((*qnodepp)->pst_sym.pst_type == PST_VAR)
	{
	    OPE_IEQCLS	    special_eqcls;
	    OPZ_IATTS	    attr;

	    special_eqcls = subquery->ops_vars.opv_base->opv_rt
		[(*qnodepp)->pst_sym.pst_value.pst_s_var.pst_vno]->opv_ojeqc;
	    if ((special_eqcls != OPE_NOEQCLS)
		&&
		BTtest((i4)special_eqcls, (char *)bfp->opb_special)
		&&
		(subquery->ops_attrs.opz_base->opz_attnums[(*qnodepp)->
		    pst_sym.pst_value.pst_s_var.pst_atno.db_att_id]->opz_equcls
		    != special_eqcls) /* do not test the special eqcls
				** with itself */
		)
	    {
		
		if ((*qnodepp)->pst_sym.pst_dataval.db_datatype > 0)
		    subquery->ops_oj.opl_smask |= OPL_IFTRUE;  /* if the
				** type is not-nullable, then it becomes
				** nullable and the tree needs to be
				** re-resolved */
		opl_iftrue(subquery, qnodepp, bfp->opb_ojid); /* insert the
				** "IFTRUE" function with the special
				** equivalence class */
	    }
	    break;
	}
	if ((*qnodepp)->pst_right)
	    opl_insertiftrue(subquery, &(*qnodepp)->pst_right, bfp);
    }
}

/*{
** Name: opl_bfiftrue	- insert iftrue into boolean factors
**
** Description:
**      At least one boolean factor needs to be traversed and
**	converted to use iftrue where appropriate
**
** Inputs:
**      subquery                        ptr to subquery being analysed
**
** Outputs:
**
**	Returns:
**	    
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      5-aug-93 (ed)
**          initial creation
[@history_template@]...
*/
static VOID
opl_bfiftrue(
	OPS_SUBQUERY   *subquery)
{
    OPB_IBF	    ibf;
    OPB_BFT	    *bft;	    /* exact size OPV_BFT struct */
    OPB_BOOLFACT    *bfp;
    if (subquery->ops_bfs.opb_newbv)
    {	/* allocate more memory for the increased boolean factor array */
	i4		    size;

	size = sizeof(bft->opb_boolfact[0]) * subquery->ops_bfs.opb_newbv;
	bft = (OPB_BFT *) opu_memory(subquery->ops_global, size);
	MEcopy ((PTR)subquery->ops_bfs.opb_base, 
	    sizeof(bft->opb_boolfact[0]) * subquery->ops_bfs.opb_bv,
	    (PTR)bft);
	subquery->ops_bfs.opb_base = bft;
    }
    else
	bft = subquery->ops_bfs.opb_base;
    for (bfp = subquery->ops_seqctest; bfp; bfp = bfp->opb_bfp)
	bft->opb_boolfact[bfp->opb_bfindex] = bfp; /* place special eqc
				** test boolean factors into array */
    subquery->ops_seqctest = NULL;
    for (ibf = subquery->ops_bfs.opb_bv; --ibf >= 0;)
    {
	bfp = bft->opb_boolfact[ibf];
	if (bfp->opb_bfindex != ibf)
	    opx_error(E_OP04B0_NESTEDOUTERJOIN);
	for (;bfp; bfp = bfp->opb_bfp)
	{
	    bft->opb_boolfact[bfp->opb_bfindex] = bfp;
	    if (bfp->opb_special)
	    {
	    	/* map of special equivalence classes from which any
		** attribute needs to be converted to an iftrue */
		subquery->ops_oj.opl_smask &=  (~OPL_IFTRUE);
		opl_insertiftrue(subquery, &bfp->opb_qnode, bfp);
		if (subquery->ops_oj.opl_smask & OPL_IFTRUE)
		    opl_resolve(subquery, bfp->opb_qnode); /* since
				** some types were changed to NULL
				** the tree needs to be resolved */
	    }
	}
    }
    if (subquery->ops_bfs.opb_newbv)
	subquery->ops_bfs.opb_bv = subquery->ops_bfs.opb_newbv;
}

/*{
** Name: opl_special_eqc_test	- remove tuples which are null
**
** Description:
**      When placing boolean factors and checking for nulls there are
**	two main cases for any particular ON clause.  Those variables
**	which are referenced in the ON clause and also if they are null
**	would cause the ON clause to be FALSE is one main class.  The
**	other class of variables are those which are not referenced in
**	the ON clause or if they are then they are part of a NULL handling
**	case such as IS NULL.  The former case is defined in the opl_onclause
**	map which is used to find degenerate outer joins but it would also
**	be used to insert the iftrue tests when appropriate.  e.g.
**      select * from (r left join s on r.a=s.a) left join t on s.a=t.a
**      can be evaluated as ((r join s) join t)     OR
**      (r  join (s join t)) ... in the former case since the eqcls for a
**	contains the values from r only, we lose the NULL values for s.
**	To compensate, a test on the special eqc for s is made in the ON
**	clause for t i.e.    ((r join s) join t on special(s)=0) 
**      Note that in the case (r join (s join t)) no test is made since
**	the eqc for "s.a" does not have any nulls in it yet for the left
**	join for r...  this is why adding the special eqc test is delayed
**	until after enumeration since otherwise the boolean factor placement
**	will never allow (r join (s join t)) since the special eqc test
**	could not be evaluated until after the join with r, but it would
**	have the wrong join id at that point.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      partialoj                       ptr to map of outer joins which
**					have been evaluated in this subtree
**      cop                             ptr to cop being analyzed
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
**      22-apr-94 (ed)
**          b59588 - initial creation
**	28-apr-94 (ed)
**	    - bug 59928, 59760 - missing eqc problem in OPC, need to make sure
**	    full join eqcls needed for ON boolean factors are available
**	8-may-97 (inkdo01)
**	    Add "complex" parm to opb_bfinit call (part of CNF removal).
**	23-jun-06 (hayke02)
**	    Test for a valid (>= 0) specialeqc. This prevents E_OP0498 for
**	    nested outer join queries on views containing subselects. This
**	    change fixes bug 116191.
[@history_template@]...
*/
static VOID
opl_special_eqc_test(
	OPS_SUBQUERY	    *subquery,
	OPL_BMOJ	    *partialojp,
	OPO_CO		    *cop)
{
    OPL_IOUTER	    current_ojid;
    OPL_OUTER	    *current_ojp;
    OPL_IOUTER	    maxoj;
    OPV_IVARS	    maxvar;
    OPL_OJT	    *lbase;
    OPV_BMVARS	    outer_varmap;   /* map of outer variables which
			** if the special eqc is 0 then the ON
			** clause for this outer join is FALSE */
    OPV_BMVARS	    specialeqc_varmap; /* map of vars having 
			** special eqcs for which
			** the special_eqc=0 test need to be made */
    OPV_IVARS	    varno;
    OPS_STATE	    *global;
    OPL_IOUTER	    ojid1;
    OPV_RT	    *vbase;

    current_ojid = cop->opo_union.opo_ojid;
    if (current_ojid < 0)
	return;
    global = subquery->ops_global;
    lbase = subquery->ops_oj.opl_base;
    vbase = subquery->ops_vars.opv_base;
    maxvar = subquery->ops_vars.opv_rv;
    maxoj = subquery->ops_oj.opl_lv;
    current_ojp = lbase->opl_ojt[current_ojid];
    MEfill(sizeof(specialeqc_varmap), (u_char)0, (PTR)&specialeqc_varmap);
    if (current_ojp->opl_type == OPL_LEFTJOIN)
    {
	MEcopy((PTR)current_ojp->opl_maxojmap, sizeof(outer_varmap),
	    (PTR)&outer_varmap);
	BTnot((i4)BITS_IN(outer_varmap), (char *)&outer_varmap);
	BTand((i4)maxvar, (char *)current_ojp->opl_onclause,
	    (char *)&outer_varmap);
    }
    else
    {	/* for full join all relations are inner so determine if any
	** variables in ON clause  equi joins could eliminate tuples */
	MEcopy((PTR)current_ojp->opl_onclause, sizeof(outer_varmap),
	    (PTR)&outer_varmap);
    }
    BTand((i4)maxvar, (char *)cop->opo_variant.opo_local->opo_bmvars, 
	(char *)&outer_varmap);
    for (ojid1 = -1; (ojid1 = BTnext((i4)ojid1, (char *)partialojp, (i4)maxoj))>=0;)
    {
	OPL_OUTER	*ojp1;
	OPV_BMVARS	temp_varmap;
	if (ojid1 == current_ojid)
	    continue;	/* ignore splitting of current_ojid for tid joins etc. */
	ojp1 = lbase->opl_ojt[ojid1];
	if ((ojp1->opl_type == OPL_FULLJOIN)
	    ||
	    (ojp1->opl_type == OPL_LEFTJOIN))
	{
	    MEcopy((PTR)ojp1->opl_ojtotal, sizeof(temp_varmap),
		(PTR)&temp_varmap);
	    BTand((i4)maxvar, (char *)&outer_varmap, (char *)&temp_varmap);
	    BTor((i4)maxvar, (char *)&temp_varmap, (char *)&specialeqc_varmap);
	}
    }
    for (varno = -1; (varno = BTnext((i4)varno, (char *)&specialeqc_varmap,
	(i4)maxvar))>=0;)
    {	/* add special eqc test for this var to this CO node */
	OPE_IEQCLS	specialeqc;
	OPB_BOOLFACT	*bfp;
	OPV_VARS	*varp;

	varp = vbase->opv_rt[varno];
	specialeqc = varp->opv_ojeqc;
	if (!(specialeqc >= 0
	    &&
	    (BTtest((i4)specialeqc,
			    (char *)&cop->opo_outer->opo_maps->opo_eqcmap)
	    ||
	    BTtest((i4)specialeqc,
			    (char *)&cop->opo_inner->opo_maps->opo_eqcmap))))
	    continue;
	    /* fixme - should be an error
	    opx_error(9999);*/	    /* expecting special eqc to be available */
	for (bfp = subquery->ops_seqctest; bfp; bfp = bfp->opb_bfp)
	{
	    if (BTtest((i4)specialeqc, (char *)&bfp->opb_eqcmap)
		&&
		(bfp->opb_ojid == cop->opo_union.opo_ojid))
		break;		    /* already have boolean factor to
				    ** use for special eqc test */
	}
	if (!bfp)
	{   /* create new boolean factor to test for special eqc */
	    PST_QNODE	    *and_nodep;
	    DB_ATT_ID	    ojattr;
	    bool	    complex = FALSE;
	    /* create a conjunct which tests the special equivalence class
	    ** for one, i.e. nulls will cause ON clause to fail */
	    ojattr.db_att_id = varp->opv_ojattr;
	    and_nodep = opv_opnode(global, PST_AND, (ADI_OP_ID)0, current_ojid);
	    and_nodep->pst_left = opv_opnode(global, PST_BOP, 
		(ADI_OP_ID)global->ops_cb->ops_server->opg_eq, current_ojid);
	    if (ojattr.db_att_id < 0)
		opx_error(E_OP0498_NO_EQCLS_FOUND); /* expecting special equivalence class
					** to be defined */
	    and_nodep->pst_left->pst_left = opv_varnode(global, 
		&subquery->ops_attrs.opz_base->opz_attnums[ojattr.db_att_id]->opz_dataval, 
		(OPV_IGVARS)varno, &ojattr);
	    and_nodep->pst_left->pst_right = global->ops_goj.opl_one;
	    opa_resolve(subquery, and_nodep->pst_left);
	    bfp = opb_bfnewget(subquery); /* get a new boolean factor element if
					** bfp is NULL, otherwise initialize the
					** existing structure.  This structure will
					** be at the head of the list of all
					** limiting predicate (key, histogram) info
					** associated with the boolean factor
					*/
	    bfp->opb_ojid = current_ojid;
	    bfp->opb_bfp = subquery->ops_seqctest;
	    subquery->ops_seqctest = bfp;
	    bfp->opb_qnode = and_nodep->pst_left;    /* save query tree node */

	    subquery->ops_oj.opl_smask &= (~OPL_ISNULL);
	    (VOID) opb_bfinit( subquery, bfp, and_nodep->pst_left, &complex);/* 
					** traverse the query
					** tree fragment associated with the
					** boolean factor and setup histogram
					** information, keyed access information
					** and initialize the equivalence
					** class map for the boolean factor
					*/
	}
	BTset((i4)bfp->opb_bfindex, (char *)cop->opo_variant.opo_local->opo_bmbf);
					/* execute boolean factor at this node */
    }
}

/*{
** Name: opj_maordering	- define ops_msort structure if necessary
**
** Description:
**      Traverse the best CO tree and define any multi-attribute ordering
**      structures, since these were deleted when the enumeration memory
**      stream was closed.  In other words any of the ordering values
**	(such as opo_ordeqc, opo_sjeqc, etc) that indicate multi-attribute
**	ordering need to be reconstructed.  This pass recurses down
**	to the leaves of the CO tree, and reconstructs multi-attribute
**	orderings on the way back up.
**
** Inputs:
**      mastatep->opj_subquery		subquery being optimized
**      cop                             current CO struct being analyzed
**      mastatep->opj_maxeqcls		maximum equivalence class defined
**                                      during enumeration, NOT after
**                                      opj_cartprod is executed
**
** Outputs:
**      subquery->ops_msort             multi-attribute orderings defined
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-feb-88 (seputis)
**          initial creation
**      26-oct-88 (seputis)
**          add corelated parameter to opn_sm1
**      28-nov-88 (seputis)
**          made change to sort corelated paramters
**	    to reduce evaluation of subselects
**	15-nov-89 (seputis)
**	    - fix keying problem which could cause optimizedb to report an
**	    unnecessary sort node error
**	29-dec-89 (seputis)
**	    - fix uninitialized variable problem for unflattened corelated
**	    queries
**      5-sep-90 (seputis)
**          - fix b32560 - recognize when a function attribute needs to be
**          evaluated as a boolean factor
**	12-jul-91 (seputis)
**	    - b7381 - like expressions
**      5-jan-93 (ed)
**          - b45695 - evaluate multi-variable function attributes as boolean
**          factor if it cannot be evaluated as a join
**          - b48049 - simplied interface to opo_combine
**      30-mar-93 (ed)
**          - fixed HP problem, used BITS_IN rather than sizeof for BT routines
**	12-apr-93 (ed)
**	    - fix bug 50646 - tid vs key joins and sorted outer gets confused
**	5-aug-93 (ed)
**	    - use == instead of = in condition
**      07-Oct-1993(fpang)
**          - Fix B53448, SEGV when STAR query has many simple aggregates.
**            opo_orig is valid if opo_sjpr = DB_ORIG.
**	5-dec-02 (inkdo01)
**	    Changes for range table expansion.
**	5-Dec-2006 (kschendel) SIR 122512
**	    Compute multi-attr join ordering if original (unrestricted)
**	    join ordering was multi-attr, not just if sort-order-restricted
**	    ordering is multi-attr.  This allows hash join to use the
**	    full unrestricted join key set if the opj-exact pass decides
**	    to use hash join rather than a merge join.
**	16-Apr-2007 (kschendel) b122118
**	    Remove dead LIKE(expr) code used for Star.
**	14-may-2009 (dougi) bug 122067
**	    Fix to avoid use of part. table base tabnle keys for joins.
*/
static VOID
opj_maordering(
	OPJ_MASTATE	    *mastatep,
	OPO_CO		    *cop)
{
    OPS_SUBQUERY	    *subquery;
    OPO_CO	*innerp;
    OPO_CO	*outerp;
    OPD_ISITE   tempsite;	/* site upon which operation should be performed */
    bool	corelated;	/* set TRUE if a corelated constant was found
				** in the set of boolean factors to be evaluated
				** at this node */
    OPL_OJT     *lbase;         /* ptr to base of outer join descriptor
                                ** table */
    OPE_IEQCLS  maxeqcls;
    OPS_STATE	*global;
    OPV_IVARS       maxvar;
    OPV_RT          *vbase;     /* ptr to base of array of ptrs
				** to joinop variables */
    OPL_BMOJ	    ojoevalmap;	/* the map of outer
				** joins which are completely evaluated
				** within the outer subtree */
    OPL_BMOJ	    ojievalmap; /* the map of outer
				** joins which are completely evaluated
				** within the inner subtree */
    OPL_BMOJ	    ojinnermap; /* used to build map of outer joins which
				** have at least one join node in this
				** subtree */
    bool	    active_oj;	/* TRUE if outer join is evaluated at
				** this node and this is the most senior
				** parent for this outer join */
    OPL_OJTYPE	    ojtype;	/* type of outer join to evaluate at this
				** node */
    OPL_OUTER	    *ojp;	/* ptr to outer join descriptor */
    bool	    full_tidjoin; /* TRUE if this is a left tid join
				** associated with an underlying full join */

    maxeqcls = mastatep->opj_maxeqcls;
    subquery = mastatep->opj_subquery;
    global = subquery->ops_global;
    lbase = subquery->ops_oj.opl_base;
    maxvar = subquery->ops_vars.opv_rv;
    vbase = subquery->ops_vars.opv_base;

    /* FIXME - for TID joins, the indexed attributes are brought up
    ** but may not be needed since the TID is used to probe the
    ** base relation and the attributes are equal by definition
    ** - therefore, it may be useful to eliminate equivalence classes
    ** from the opo_outer->opo_eqcmap and below, this can be done
    ** here but it may be useful to place it into the cost model
    ** by having several OPN_EQS structures defined for the
    ** same tree structure, but some not bringing up the
    ** TID eqcls
    */

    if (lbase && (cop->opo_union.opo_ojid >= 0) && (cop->opo_sjpr == DB_SJ))
    {
	ojp = lbase->opl_ojt[cop->opo_union.opo_ojid];
	ojtype = ojp->opl_type;

	active_oj = !BTtest((i4)cop->opo_union.opo_ojid , 
	    (char *)&mastatep->opj_ojmap);
	if (active_oj)
	{   /* setup the outer join map which contains all outer joins
	    ** which are completely evaluated within this subtree */
	    BTset((i4)cop->opo_union.opo_ojid , (char *)&mastatep->opj_ojmap); /* used
					** to find highest parent which
					** evaluates this outer join */
	}
    }
    else
    {
	active_oj = FALSE;
	ojtype = OPL_INNERJOIN;
    }
    if (outerp = cop->opo_outer)
    {
	opj_maordering(mastatep, outerp);
	if (lbase)
	{   /* setup ojinnermap which contains all outer joins which are
	    ** partially or totally evaluated within this subtree, but 
	    ** not at this node unless it is in the subtree */
	    MEcopy((PTR)&mastatep->opj_ojinnermap, 
		sizeof(mastatep->opj_ojinnermap),
		(PTR)&ojinnermap);
	    if ((outerp->opo_union.opo_ojid >= 0)
		&&
		(outerp->opo_sjpr == DB_SJ))
		BTset((i4)outerp->opo_union.opo_ojid, (char *)&ojinnermap);

	    /* map of outerjoins which are entirely evaluated within
	    ** this subtree */
	    MEcopy((PTR)&mastatep->opj_ojevalmap, sizeof(ojoevalmap),
		(PTR)&ojoevalmap);
	}
    }
    else if (lbase)
    {
	MEfill(sizeof(mastatep->opj_ojinnermap), (u_char)0,
	    (PTR)&mastatep->opj_ojinnermap);
	MEfill(sizeof(mastatep->opj_ojevalmap), (u_char)0,
	    (PTR)&mastatep->opj_ojevalmap);
    }
    if (innerp = cop->opo_inner)
	opj_maordering(mastatep, innerp);

    {	/* allocate and initialize auxiliary CO structure */
	char	    *temp;
	i4	    tempsize;
	tempsite = cop->opo_variant.opo_site; /* save site ID for distributed
					** thread */
	tempsize = sizeof(OPD_LOCAL) + sizeof(OPB_BMBF) + sizeof(OPV_BMVARS);
	temp = (char *)opu_memory(global, (i4)tempsize); /* 
					** allocate room for
					** boolean factor bitmaps */
	MEfill((i4)tempsize, (u_char)0, (PTR)temp);
	cop->opo_variant.opo_local = (OPD_LOCAL *)temp;
	cop->opo_variant.opo_local->opo_bmbf = 
	    (OPB_BMBF *) &temp[sizeof(OPD_LOCAL)]; /* allocate room for boolean
					** factor bitmaps */
	cop->opo_variant.opo_local->opo_bmvars = 
	    (OPV_BMVARS *) &temp[sizeof(OPD_LOCAL)+sizeof(OPB_BMBF)]; /* allocate room for
					** variable bitmaps */
	cop->opo_variant.opo_local->opo_id = ++mastatep->opj_nodeid; /* define
					** a nodeid for this CO node */
	cop->opo_variant.opo_local->opo_operation = tempsite; /* restore site ID
					** for distributed thread */
        cop->opo_variant.opo_local->opo_type = OPL_UNKNOWN; /* default for no outer joins */
        cop->opo_variant.opo_local->opo_ojid = OPL_NOOUTER; /* no outer at this point */
    }

    switch (cop->opo_sjpr)
    {
    case DB_ORIG:
    {
	OPV_VARS	*varp;
	OPV_IVARS	varno;

	varno = cop->opo_union.opo_orig;
	varp = subquery->ops_vars.opv_base->opv_rt[varno];
	opo_orig(subquery, cop);	/* add multi-attribute ordering for
                                        ** DB_ORIG node */
	MEfill(sizeof(OPV_BMVARS), (u_char)0, 
	    (PTR)cop->opo_variant.opo_local->opo_bmvars);
	BTset((i4)varno, (char *)cop->opo_variant.opo_local->opo_bmvars);
        if (varp->opv_grv->opv_created == OPS_SELECT)
            MEfill(sizeof(*cop->opo_variant.opo_local->opo_bmbf), (u_char)0,
                (PTR)cop->opo_variant.opo_local->opo_bmbf); /* subselect subquery does
                                        ** not have any boolean factors to evaluate */
        else
        {
            cop->opo_maps = &varp->opv_maps; /* use
                                        ** eqc map from ORIG node for OPC which needs all attributes
                                        ** of relation, as in "retrieve (x=any(r.relid where
                                        **  r.relowner="$ingres" and r.relid="iirelation")) which would
                                        ** reference 0 eqcls from the orig node */
            opn_sm1(subquery,
                &varp->opv_maps.opo_eqcmap,
                (OPE_BMEQCLS *)NULL,
                cop->opo_variant.opo_local->opo_bmbf,
                cop->opo_variant.opo_local->opo_bmvars);
        }
                                        /* find boolean factors to be evaluated at
                                        ** the leaf node, FIXME this should
                                        ** only define the boolean factors used
                                        ** for keying */
	cop->opo_variant.opo_local->opo_target = tempsite; /* ORIG site ID
					** for distributed thread is same as operation */
	if (lbase)
	{
	    opl_sjij(subquery, varno, &mastatep->opj_ojinnermap, (OPL_BMOJ *)NULL); 
#if 0
/* probably not needed if ojinnermap is set as in opn_jmaps */
	    /* initialize maps for single variable inner joins created to 
	    ** support full joins */
	    OPL_IOUTER	    svojid;
	    svojid = varp->opv_svojid;
	    if ((svojid >= 0) && !BTtest((i4)svojid, (char *)&mastatep->opj_ojmap))
	    {	/* outer join has not been evaluated for this single variable
		** inner join, so init variable maps to indicate inner join
		** is evaluated at this orig node... this mechanism is used
		** to support full joins, since the inner and outer varmaps
		** for full joins are equal so that innerjoins are needed to
		** partition the tables to get correct full join semantics */
		BTset((i4)svojid, (char *)&mastatep->opj_ojmap); 
					/* used to find highest parent which
					** evaluates this outer join */
		BTset((i4)svojid, (char *)&mastatep->opj_ojinnermap);
		BTset((i4)svojid, (char *)&mastatep->opj_ojevalmap);
	    }
#endif
	    opl_findiftrue(subquery, cop); /* check for boolean factors which need
				    ** to be modified for the iftrue function */
	}
	break;
    }
    case DB_PR:
    {
	opo_pr(subquery, cop);		/* add multi-attribute ordering */
	if (outerp->opo_sjpr == DB_ORIG)
	{
	    OPV_VARS	*varp;
	    OPV_IVARS	varno;

	    /* Supposed to be ORIG - now check for partitioned table. If
	    ** so, reset ordering, 'cause it's per-partition base table
	    ** ordering which is useless across partitions. */
	    varno = outerp->opo_union.opo_orig;
	    varp = subquery->ops_vars.opv_base->opv_rt[varno];
	    if (varp->opv_grv && varp->opv_grv->opv_relation &&
		varp->opv_grv->opv_relation->rdr_parts != NULL)
	    {
		cop->opo_ordeqc = OPE_NOEQCLS;
		cop->opo_storage = DB_HEAP_STORE;
	    }
	}

	cop->opo_variant.opo_local->opo_bmbf  
	    = outerp->opo_variant.opo_local->opo_bmbf; /* copy boolean factor
                                        ** bitmap for PR */
	outerp->opo_variant.opo_local->opo_bmbf = NULL; /* no restrictions at
                                        ** orig node */
	cop->opo_variant.opo_local->opo_bmvars
	    = outerp->opo_variant.opo_local->opo_bmvars;
	cop->opo_variant.opo_local->opo_target =  /* PR needs to be
					** placed on this site */
	cop->opo_variant.opo_local->opo_operation; 
	cop->opo_variant.opo_local->opo_operation = 
	    outerp->opo_variant.opo_local->opo_target; /* the actual project
					** restrict needs to be done on the
					** same site as the ORIG node */
	break;
    }
    case DB_RSORT:
    {
	opo_rsort(subquery, cop);       /* add ordering for sort node 
					** probably at top of plan */
	cop->opo_variant.opo_local->opo_bmbf = NULL;
	cop->opo_variant.opo_local->opo_bmvars
	    = outerp->opo_variant.opo_local->opo_bmvars;
	cop->opo_variant.opo_local->opo_target = tempsite; /* Sort nodes can only
					** occur on the top of the query plan */
	cop->opo_variant.opo_local->opo_operation = 
	outerp->opo_variant.opo_local->opo_target = /* sort nodes are always done
					** on the site of the child node operation */
	outerp->opo_variant.opo_local->opo_operation;  /* Sort nodes can only
					** occur on the top of the query plan */
	break;
    }
    case DB_SJ:
    {
	OPO_ISORT	jordering;	/* ordering for joining */
	OPO_ISORT	outer;          /* ordering of the outer */
	OPO_ISORT       inner;          /* ordering of the inner */
	OPO_ISORT	*jresult;       /* ptr to place to place joining
                                        ** equivalence class descriptor
					*/
	bool		inner_corelated; /* TRUE - if the inner cop is
					** a corelated subquery */
	OPO_ISORT	dummy;          /* resultant ordering */
	OPV_VARS	*lvarp;         /* ptr to left variable descriptor
					** if it is a leaf */
	OPV_VARS        *rvarp;		/* ptr to right variable descriptor
                                        ** if it is a leaf */
	OPE_BMEQCLS	bmunion;
    
        MEcopy((PTR)&outerp->opo_maps->opo_eqcmap, sizeof(OPE_BMEQCLS), (PTR)&bmunion);
	BTor((i4)BITS_IN(OPE_BMEQCLS), (char *)&innerp->opo_maps->opo_eqcmap, (char *)&bmunion);
	BTor((i4)BITS_IN(OPE_BMEQCLS), (char *)&cop->opo_maps->opo_eqcmap, (char *)&bmunion); /*
						** some multi-variable function 
						** attributes may be created at this
						** node, which are created as a result
						** of outer joins for nullable attributes */
	MEcopy((PTR)outerp->opo_variant.opo_local->opo_bmvars,
	    sizeof(OPV_BMVARS),
	    (PTR)cop->opo_variant.opo_local->opo_bmvars);
	BTor((i4)BITS_IN(OPV_BMVARS),
	    (char *)innerp->opo_variant.opo_local->opo_bmvars,
	    (char *)cop->opo_variant.opo_local->opo_bmvars);
	if (lbase)
	{   /* create the outer join descriptors if necessary */
	    OPL_IOUTER	    ojid;	/* current outer join ID being analyzed */

	    /* ojinnermap which contains all outer joins which are
	    ** partially or totally evaluated within this subtree, but 
	    ** not at this node unless it is in the subtree */
	    if (subquery->ops_mask & OPS_IJCHECK)
		opl_ijcheck(subquery, &ojinnermap, &mastatep->opj_ojinnermap,
		    &mastatep->opj_ojinnermap, &ojoevalmap, &ojievalmap,
		    cop->opo_variant.opo_local->opo_bmvars);
	    BTor((i4)BITS_IN(mastatep->opj_ojinnermap),
		(char *)&ojinnermap,
		(char *)&mastatep->opj_ojinnermap);    
	    if ((innerp->opo_union.opo_ojid >= 0)
		&&
		(innerp->opo_sjpr == DB_SJ))
		BTset((i4)innerp->opo_union.opo_ojid, (char *)&mastatep->opj_ojinnermap);
	    /* map of outerjoins which are entirely evaluated within
	    ** this subtree */
	    MEcopy((PTR)&mastatep->opj_ojevalmap, sizeof(ojievalmap),
		(PTR)&ojievalmap);
	    BTor((i4)BITS_IN(mastatep->opj_ojevalmap),	(char *)&ojoevalmap,
		(char *)&mastatep->opj_ojevalmap);
	    if (active_oj)
		BTset((i4)cop->opo_union.opo_ojid, 
		    (char *)&mastatep->opj_ojevalmap); /* map of
					** outer joins which are totally
					** evaluated within this subtree */
	    /* set up some values to check for full tid joins */
	    full_tidjoin = FALSE;
	    if (ojtype == OPL_FULLJOIN)
	    {
		if (!(ojp->opl_mask & OPL_FJFOUND))
		    ojp->opl_mask |= OPL_FJFOUND;
		else
		{
		    ojtype = OPL_LEFTJOIN;	/* this is a TID join which
					    ** occurs after the underlying
					    ** full join */
		    full_tidjoin = TRUE;
		}
	    }
	    ojid = cop->opo_union.opo_ojid;
	    if (ojid >= 0)
	    {   /* outer join is evaluated at this node */
		if ((ojtype == OPL_LEFTJOIN)
		    ||
		    (ojtype == OPL_FULLJOIN))
		{
		    /* outer join operation was found at this node so modify the
		    ** available eqc list to include special equivalence classes
		    ** which are created at this node */
		    if ((ojtype == OPL_LEFTJOIN)
			&&
			(ojp->opl_type != OPL_FULLJOIN)
			&&
			BTsubset((char *)cop->opo_outer->opo_variant.opo_local
				->opo_bmvars, (char *)ojp->opl_maxojmap, 
				(i4)BITS_IN(*ojp->opl_maxojmap)))
			ojtype = OPL_RIGHTJOIN;
		    cop->opo_variant.opo_local->opo_type = ojtype;
		    cop->opo_variant.opo_local->opo_ojid = ojid;

		    /* get intersection of inner and outer maps to describe
		    ** the outer join */
		    cop->opo_variant.opo_local->opo_ojoin = (OPE_BMEQCLS *)opu_memory(subquery->ops_global,
			(i4)(sizeof(OPE_BMEQCLS)));
		    MEcopy((PTR)&cop->opo_outer->opo_maps->opo_eqcmap, 
			sizeof(*cop->opo_variant.opo_local->opo_ojoin),
			(PTR)cop->opo_variant.opo_local->opo_ojoin);
		    BTand((i4)subquery->ops_eclass.ope_ev, 
			(char *)&cop->opo_inner->opo_maps->opo_eqcmap,
			(char *)cop->opo_variant.opo_local->opo_ojoin); /* FIXME,
					** when filters are implemented there may
					** be where clause equi-joins here */

		    ojp->opl_mask |= OPL_RESOLVED;  /* mark this outer join as being
					** calculated */
		    if ((ojtype == OPL_FULLJOIN) || (ojtype == OPL_LEFTJOIN))
			opj_ojeqcmaps(subquery, &cop->opo_variant.opo_local->opo_ojlnull,
			    &cop->opo_outer->opo_maps->opo_eqcmap,
			    cop->opo_inner->opo_variant.opo_local->opo_bmvars,
			    &cop->opo_inner->opo_maps->opo_eqcmap);

		    if ((ojtype == OPL_FULLJOIN) || (ojtype == OPL_RIGHTJOIN))
			opj_ojeqcmaps(subquery, &cop->opo_variant.opo_local->opo_ojrnull,
			    &cop->opo_inner->opo_maps->opo_eqcmap,
			    cop->opo_outer->opo_variant.opo_local->opo_bmvars,
			    &cop->opo_outer->opo_maps->opo_eqcmap);

		    /* initialize the inner join eqc handling maps, either an eqc needs
		    ** to be refreshed, or it needs to be set to one, if the inner join
		    ** succeeds */
		    cop->opo_variant.opo_local->opo_ojinner.opo_oneeqcmap = 
			lbase->opl_ojt[ojid]->opl_innereqc;
		    cop->opo_variant.opo_local->opo_ojinner.opo_refresheqcmap = (OPE_BMEQCLS *)
			opu_memory(global, (i4)sizeof(OPE_BMEQCLS));
		    BTor((i4)maxeqcls, (char *)lbase->opl_ojt[ojid]
			->opl_innereqc, (char *)&bmunion);
		    MEcopy((PTR)&bmunion, sizeof(bmunion), (PTR)cop->opo_variant.
			opo_local->opo_ojinner.opo_refresheqcmap);
		    BTxor((i4)maxeqcls, (char *)lbase->opl_ojt[ojid]->opl_innereqc,
			(char *)cop->opo_variant.opo_local->opo_ojinner.opo_refresheqcmap);
		}
		else
		    cop->opo_union.opo_ojid = OPL_NOOUTER; /* inner joins should
					** be ignored by OPC */
	    }
	}

	outerp->opo_variant.opo_local->opo_target = tempsite;
	innerp->opo_variant.opo_local->opo_target = tempsite; /* both outer and
					** inner need to be on the same site for
					** this operation */
	if (cop->opo_osort)
	{   /* it is easier placing all sort information in one location
            ** the opo_ordeqc ordering is in the child also */
	    outerp->opo_psort = TRUE;	/* mark outer opo_ordeqc
					** as originating from parent */
	    outerp->opo_pdups = cop->opo_odups; /* mark outer 
					** duplicate handling sort from parent */
	    outerp->opo_ordeqc = OPE_NOEQCLS; /* since this order is not used
                                        ** by the child, reset it to avoid
                                        ** any confusion in the opo_mexact
                                        ** pass */
	}
	if (cop->opo_isort)
	{   /* it is easier placing all sort information in one location
            ** the opo_ordeqc ordering is in the child also */
	    innerp->opo_psort = TRUE;	/* mark inner opo_ordeqc
					** as originating from parent */
	    innerp->opo_pdups = cop->opo_idups; /* mark inner
					** duplicate handling sort from parent */
	    innerp->opo_ordeqc = OPE_NOEQCLS; /* since this order is not used
                                        ** by the child, reset it to avoid
                                        ** any confusion in the opo_mexact
                                        ** pass */
	}

	if (outerp->opo_sjpr == DB_ORIG)	/* TRUE if left subtree is a leaf */
	{
	    lvarp =subquery->ops_vars.opv_base->opv_rt[outerp->opo_union.opo_orig]; /* get the
					** descriptor for this leaf node */
	}
	else
	    lvarp = NULL;
	if (innerp->opo_sjpr == DB_ORIG)	/* TRUE if right subtree is a leaf*/
	{
	    rvarp =subquery->ops_vars.opv_base->opv_rt[innerp->opo_union.opo_orig]; /* get the
						** descriptor for this leaf node */
	}
	else
	    rvarp = NULL;

	if (subquery->ops_funcs.opz_fmask & OPZ_MAFOUND)
	{
	    /* check that if a function attribute, and the corresponding qualification
	    ** are evaluated on the same node, then a boolean factor is made, fix
	    ** for b32560 */
	    OPZ_IFATTS            fattr; /* function attribute currently
					** being analyzed */
	    OPZ_IFATTS            maxfattr; /* maximum function attribute
					** defined */
	    OPZ_AT                *abase; /* ptr to base of an array
					** of ptrs to attributes */
	    OPZ_FT                *fbase; /* ptr to base of an array
					** of ptrs to function attributes*/
	    OPE_ET		      *ebase;
	    OPZ_IATTS	      maxattr;

	    maxfattr = subquery->ops_funcs.opz_fv; /* maximum number of
					** function attributes defined */
	    fbase = subquery->ops_funcs.opz_fbase; /* ptr to base of array
					** of ptrs to function attribute
					** elements */
	    maxattr = subquery->ops_attrs.opz_av;
	    abase = subquery->ops_attrs.opz_base; /* ptr to base of array
					** of ptrs to joinop attributes
					*/
	    ebase = subquery->ops_eclass.ope_base;
	    /*
	    ** If this equiv class is a function and the function
	    ** is not in the left or right side (as shown above)
	    ** and the function can be computed here, get all the
	    ** attributes in the function
	    */
	    for (fattr = 0; fattr < maxfattr; fattr++)
	    {
		OPZ_FATTS	    *fattrp; /* ptr to current function
					** attribute being analyzed */
		OPE_IEQCLS          feqc; /* equivalence class containing
					** the function attribute */

		fattrp = fbase->opz_fatts[fattr]; /* get ptr to current
					** function attr being analyzed*/
		if ((fattrp->opz_type != OPZ_MVAR)
		    ||
		    (fattrp->opz_mask & OPZ_OJFA))
		    continue;           /* only multi-var function
					** attributes will be calculated
					** at an interior node, so only
					** consider these */
		feqc = abase->opz_attnums[fattrp->opz_attno]->opz_equcls;/*
					** get equivalence class containing
					** function attribute */
		if (BTsubset((char *)&fattrp->opz_eqcm, (char *)&bmunion,
			(i4)maxeqcls)
		    &&
		    !BTsubset((char *)&fattrp->opz_eqcm, (char *)&innerp->opo_maps->opo_eqcmap,
			(i4)maxeqcls)
		    &&
		    !BTsubset((char *)&fattrp->opz_eqcm, (char *)&outerp->opo_maps->opo_eqcmap,
			(i4)maxeqcls)
		    )
		{   /* function attribute is to be calculated at this node, since it
		    ** cannot be calculated any lower
		    ** - look at each other attribute in the equivalence class and
		    ** if it is also available at this node the it would be worthwhile
		    ** to create a boolean factor, since the join may be missed, or
		    ** the restriction can be applied earlier in the tree */
		    OPZ_BMATTS	*fa_attrmap;
		    OPZ_IATTS	attr;
		    fa_attrmap = &ebase->ope_eqclist[feqc]->ope_attrmap;
		    for (attr = -1; (attr = BTnext((i4)attr, (char *)fa_attrmap, 
			(i4)maxattr)) >= 0;)
		    {
			OPZ_ATTS	    *attrp;
                        bool                eval_here;  /* TRUE if a var node needs to
                                                ** created for the second operand of the
                                                ** boolean factor */
			if (attr == fattrp->opz_attno)
			    continue;		/* do not look at same attribute */
			attrp = abase->opz_attnums[attr];
			if (attrp->opz_attnm.db_att_id == OPZ_FANUM)
			{	/* found another function attribute */
			    OPZ_FATTS		*fattrp1; /* ptr to current function
						    ** attribute being analyzed */

			    fattrp1 = fbase->opz_fatts[attrp->opz_func_att]; /* get ptr to current
						    ** function attr being analyzed*/
			    if (fattrp1->opz_mask & OPZ_OJFA)
				continue;
			    if (BTsubset((char *)&fattrp1->opz_eqcm, (char *)&bmunion,
				    (i4)maxeqcls)
				)
			    {   /* create boolean factor since normal join cannot be performed */
				PST_QNODE	    *expr1;
				PST_QNODE	    *expr2;

				expr1 = fattrp1->opz_function->pst_right;
				opv_copytree(global, &expr1);
				expr2 = fattrp->opz_function->pst_right;
				opv_copytree(global, &expr2);
				opb_mboolfact(subquery, expr1, expr2,
				    fattrp->opz_function->pst_sym.pst_value.pst_s_op.pst_joinid);
                                eval_here = FALSE;
			    }
                            else
                                eval_here = (BTtest((i4)feqc, (char *)&bmunion) >= 0); /*
                                                    ** TRUE if the second operand has already
                                                    ** been evaluated */
			}
                        else
                            eval_here = TRUE;
                        if (eval_here)
			{   /* an ingres attribute has been found */
			    if (BTtest((i4)attrp->opz_varnm, (char *)cop->opo_variant.opo_local->opo_bmvars))
			    {
				PST_QNODE	    *qnodep;  /* var node created to represent boolean factor */
				DB_ATT_ID	  attribute;	/* attribute number to be used
							    ** to create var node */
				PST_QNODE	    *expr3;

				attribute.db_att_id = attr;
				qnodep = opv_varnode(global, &attrp->opz_dataval,
					     attrp->opz_varnm, &attribute); /* create a var node to be used
							** in boolean factor expression */
				expr3 = fattrp->opz_function->pst_right;
				opv_copytree(global, &expr3);
				opb_mboolfact(subquery, qnodep, expr3,
				    fattrp->opz_function->pst_sym.pst_value.pst_s_op.pst_joinid);
			    }
			}
		    }
		}
	    }		
	}

	opb_gbfbm(subquery, &bmunion, &outerp->opo_maps->opo_eqcmap, &innerp->opo_maps->opo_eqcmap,
	    &ojoevalmap, &ojievalmap, &mastatep->opj_ojevalmap,
	    maxeqcls, lvarp, rvarp, cop->opo_variant.opo_local->opo_bmbf, (OPE_BMEQCLS *)NULL,
	    (OPE_BMEQCLS *)NULL, (OPE_BMEQCLS *)NULL, (OPE_BMEQCLS *)NULL,
	    outerp->opo_variant.opo_local->opo_bmvars, innerp->opo_variant.opo_local->opo_bmvars,
	    cop->opo_variant.opo_local->opo_bmvars, cop->opo_variant.opo_local->opo_ojid, 
	    &mastatep->opj_ojinnermap, cop); /* get
				    ** the bit map of boolean factors to be
				    ** applied at this node */
	if (lbase)
	{
	    opl_findiftrue(subquery, cop); /* check for boolean factors which need
				    ** to be modified for the iftrue function */
	    if (cop->opo_variant.opo_local->opo_ojid >= 0)
	    {	/* setup OPC maps for special eqc handling */
		OPV_BMVARS  *ojvarmap;
		OPL_OUTER   *oj_outerp;
		OPV_IVARS   ojvar;

		opl_special_eqc_test(subquery, &mastatep->opj_ojinnermap, cop);
		oj_outerp = lbase->opl_ojt[cop->opo_variant.opo_local
		    ->opo_ojid];
		ojvarmap = cop->opo_variant.opo_local->opo_bmvars;
		cop->opo_variant.opo_local->opo_special = (OPE_BMEQCLS *)
		    opu_memory(global, (i4)sizeof(*cop->opo_variant.
			opo_local->opo_special));
		MEfill(sizeof(*cop->opo_variant.opo_local->opo_special),
		    (u_char)0, (PTR)cop->opo_variant.opo_local->opo_special);
		cop->opo_variant.opo_local->opo_innereqc = (OPE_BMEQCLS *)
		    opu_memory(global, (i4)sizeof(*cop->opo_variant.
			opo_local->opo_innereqc));
		MEfill(sizeof(*cop->opo_variant.opo_local->opo_innereqc),
		    (u_char)0, (PTR)cop->opo_variant.opo_local->opo_innereqc);
		if (!full_tidjoin)	    /* current tid joins associated with
					    ** full joins do not affect special
					    ** eqcls values since this join can
					    ** only be used to pickup missing
					    ** attributes from the base relation
					    ** and no boolean factors can be
					    ** evaluated */
		for (ojvar = -1; (ojvar = BTnext((i4)ojvar, (char *)ojvarmap,
		    (i4)maxvar))>= 0;)
		{
		    OPE_IEQCLS	    ojeqcls;
		    ojeqcls = vbase->opv_rt[ojvar]->opv_ojeqc;
		    if ((ojeqcls != OPE_NOEQCLS)
			&&
			BTtest((i4)ojvar, (char *)oj_outerp->opl_maxojmap))
		    {
			BTset((i4)ojeqcls, (char *)cop->opo_variant.opo_local->opo_innereqc);
			if (!BTtest((i4)ojeqcls, (char *)&innerp->opo_maps->opo_eqcmap)
			    &&
			    !BTtest((i4)ojeqcls, (char *)&outerp->opo_maps->opo_eqcmap))
			    BTset((i4)ojeqcls, (char *)cop->opo_variant.opo_local->opo_special);
		    }
		}
	    }
	}
	{
	    OPV_GSUBSELECT	*corelated_sub;
	    inner_corelated = ((cop->opo_inner->opo_sjpr == DB_ORIG)
		&&
		(corelated_sub = subquery->ops_vars.opv_base->
		    opv_rt[cop->opo_inner->opo_union.opo_orig]->opv_grv->opv_gsubselect)
		&& 
		BTcount((char *)&corelated_sub->opv_subquery->ops_correlated, OPV_MAXVAR));
	}
	if (cop->opo_osort)		/* is inner sorted */
	    outer = opo_fordering(subquery, &cop->opo_outer->opo_maps->opo_eqcmap);
	else
	    outer = cop->opo_outer->opo_ordeqc;
	if (cop->opo_isort)		/* is outer sorted */
	    inner = opo_fordering(subquery, &cop->opo_inner->opo_maps->opo_eqcmap);
	else if (!inner_corelated)
	    inner = cop->opo_inner->opo_ordeqc;
	/* reconstruct join eqc ordering.  sjeqc is either the same as
	** orig_sjeqc, or "smaller" due to trimming thanks to child
	** ordering compatibility.  The latter doesn't matter for
	** hash joining.  If orig sjeqc is a single eqclass, sjeqc is too.
	*/
	if (cop->opo_orig_sjeqc < maxeqcls)
	{
	    jordering = cop->opo_sjeqc; /* in the case of a single attribute
					** join, which includes implicit
                                        ** TID, no multi-attribute ordering
                                        ** was created */
	    jresult = &dummy;           /* ignore return result for single
                                        ** equivalence class case */
	    if (cop->opo_ordeqc < maxeqcls)
		break;			/* both ordering and joining
					** eqc are already defined
					** as single eqcls */
	    if ((innerp->opo_sjpr == DB_ORIG)
		&&
		!inner_corelated
		&&
		(jordering >= 0))
		inner = jordering;	/* fix bug 50646 - tid vs key joins
					** and sorted outer gets confused */
	}
	else
	{   /* create a multi-attribute descriptor for the join */
	    OPE_BMEQCLS	    *bmjoinp;	/* intersection of inner and outer
					** bit maps */
	    bmjoinp = (OPE_BMEQCLS *)opu_memory(global,
		(i4)(sizeof(OPE_BMEQCLS)));
	    MEcopy((PTR)&outerp->opo_maps->opo_eqcmap, sizeof(*bmjoinp),
		(PTR)bmjoinp);
	    BTand((i4)maxeqcls, (char *)&innerp->opo_maps->opo_eqcmap,
		(char *)bmjoinp);
	    
	    jordering = opo_fordering(subquery, bmjoinp);
	    if (innerp->opo_sjpr == DB_ORIG)
	    {
		if (inner_corelated)
		    cop->opo_sjeqc = jordering;
		else
		{
		    cop->opo_sjeqc = opo_kordering(subquery,
			subquery->ops_vars.opv_base->opv_rt[innerp->opo_union.opo_orig],
			bmjoinp); 
		    if (cop->opo_sjeqc >= 0)
		    {
			inner = jordering = cop->opo_sjeqc;
		    }
		}
		cop->opo_orig_sjeqc = cop->opo_sjeqc;  /* Should be unused! */
		jresult = &dummy;	    /* since keying is done the inner
					    ** does not need to be in sorted
                                            ** order, so find the key which can
                                            ** be used and ignore sorted
                                            ** orderings */
	    }
	    else
	    {
		cop->opo_orig_sjeqc = jordering;	/* In case hash join */
		jresult = &cop->opo_sjeqc;
	    }
	}
	if (inner_corelated && !cop->opo_isort)
	{
	    inner = jordering;		/* for a corelated subquery the
					** joining equivalence classes are the
					** corelated equivalence classes */
	}

	cop->opo_ordeqc = opo_combine(subquery, jordering, 
	    outer, inner, &cop->opo_maps->opo_eqcmap, jresult); /* get the
					** joining eqcls, and the new
					** orderings */
	break;
    }
    default:
	opx_error(E_OP0495_CO_TYPE);
    }
}

/*{
** Name: opj_uveliminate	- eliminate partition
**
** Description:
**      This partition of a union view will not return any rows so 
**      eliminate it from the union view definition 
**
** Inputs:
**      subqpp                          ptr to point in linked list
**					of subquery partition to eliminate
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
**      1-sep-92 (seputis)
**          initial creation for union views
**	13-May-2009 (kschendel) b122041
**	    Comment out bogus else-if, dups can't be ALL and NO at the same
**	    time.  No idea what seputis intended to write here.
*/
static VOID
opj_uveliminate(
	OPS_SUBQUERY       **subqpp)
{
    OPS_SUBQUERY	*subquery;
    subquery = *subqpp;
    *subqpp = subquery->ops_next; /* remove from main list, FIXME
				** memory is lost here */
    /* adjust the duplicate handling if necessary for a combination
    ** of union all and union operators */
    if ((subquery->ops_next->ops_union == subquery)
	&&
	(   (subquery->ops_next->ops_next != subquery->ops_next)
	    /* only 2 partitions left, and one will be eliminated so
	    ** mark the last partition with the correct duplicate handling */
	    ||
	    (subquery->ops_next->ops_next->ops_root->pst_sym.
		pst_value.pst_s_root.pst_union.pst_dups == PST_ALLDUPS)
	    /* possibly one partition remains which requires duplicates to be removed */
	)
	&&
	(subquery->ops_next->ops_root->pst_sym.
	    pst_value.pst_s_root.pst_union.pst_dups == PST_NODUPS)
	&&
	(   !subquery->ops_union
	    ||
	    (subquery->ops_root->pst_sym.
		pst_value.pst_s_root.pst_union.pst_dups == PST_ALLDUPS)
	))
	{
	    /* this covers the case in which the next partition is being
	    ** reduced to a single partition from which duplicates need to be
	    ** removed, i.e. adjacent partitions have independent duplicate
	    ** handling */
	    subquery->ops_next->ops_root->pst_sym.
		pst_value.pst_s_root.pst_union.pst_dups = PST_ALLDUPS;
	    subquery->ops_next->ops_root->pst_sym.pst_value.pst_s_root.pst_dups = PST_NODUPS;
	    subquery->ops_next->ops_duplicates = OPS_DREMOVE;
				/* only one partition left so
				** remove duplicates at subquery
				** level */
	}
    else if (subquery->ops_union
	&&
	(	(subquery->ops_union->ops_root->pst_sym.pst_value.
		pst_s_root.pst_union.pst_dups == PST_ALLDUPS)
	    ||
	    !subquery->ops_union->ops_union
	)
	&&
	(subquery->ops_root->pst_sym.
	    pst_value.pst_s_root.pst_union.pst_dups == PST_NODUPS)
	&&
	(	(subquery->ops_next->ops_union != subquery)
	    ||
	    (subquery->ops_next->ops_root->pst_sym.pst_value.pst_s_root.pst_union.pst_dups == PST_ALLDUPS)
	))
	{
	    /* this covers the case in which the current partition is being
	    ** reduced to a single partition from which duplicates need to be
	    ** removed, i.e. adjacent partitions have independent duplicate
	    ** handling */
	    subquery->ops_union->ops_root->pst_sym.
		pst_value.pst_s_root.pst_union.pst_dups = PST_ALLDUPS;
	    subquery->ops_union->ops_root->pst_sym.pst_value.pst_s_root.pst_dups = PST_NODUPS;
	    subquery->ops_union->ops_duplicates = OPS_DREMOVE;
				/* only one partition left so
				** remove duplicates at subquery
				** level */
	}
#if 0
/** this ELSE is clearly "false".  I'm leaving it commented out, maybe
*** someone can figure out what on earth was meant in the first place.
**	else if ((subquery->ops_next->ops_union == subquery)
**	    &&
**	    (subquery->ops_next->ops_root->pst_sym.
**		pst_value.pst_s_root.pst_union.pst_dups == PST_NODUPS)
**	    &&
**	    (subquery->ops_next->ops_root->pst_sym.
**		pst_value.pst_s_root.pst_union.pst_dups == PST_ALLDUPS)
**	    )
**	    /* the remaining case is one in which the next partition has
**	    ** duplicates to be removed but it is not isolated to exactly
**	    ** one partition */
**	    subquery->ops_next->ops_root->pst_sym.
**		pst_value.pst_s_root.pst_union.pst_dups = PST_ALLDUPS;
#endif

    if (subquery->ops_next->ops_union == subquery)
    {   /* since we are not at the head of the list we can
	** use the next subquery as part of the union */
	subquery->ops_next->ops_union = subquery->ops_union; /* remove
				    ** this subquery from the union
				    ** list */
#ifdef E_OP038C_UNION
	if (subquery->ops_next->ops_root->pst_sym.
	    pst_value.pst_s_root.pst_union.pst_next
	    !=
	    subquery->ops_root)
	    opx_error(E_OP038C_UNION); /* expected the root nodes to
				    ** match */
#endif
	subquery->ops_next->ops_root->pst_sym.
	    pst_value.pst_s_root.pst_union.pst_next
	    = subquery->ops_root->pst_sym.
		pst_value.pst_s_root.pst_union.pst_next; /*
				** remove query from the view list
				*/
    }
    else
    {   /* we are at the head of the list so mark the current
	** query differently, since it appears in several sensitive
	** tables */
	OPV_GRV	*union_grv;
	OPS_STATE   *global;

	global = subquery->ops_global;
	union_grv = global->ops_rangetab.opv_base->opv_grv[subquery->ops_gentry];
	subquery->ops_union->ops_sqtype = OPS_VIEW;
#ifdef E_OP038C_UNION
	if (
	    (subquery->ops_gentry < 0)
	    ||
	    (   global->ops_qheader->pst_rangetab[subquery->ops_gentry]->pst_rgroot 
		!=
		subquery->ops_root
	    )
	    ||
	    (union_grv->opv_gsubselect->opv_subquery != subquery)
	    )
	    opx_error(E_OP038C_UNION); /* expected the root nodes to
				    ** match, and a union result value */
#endif
	global->ops_qheader->pst_rangetab[subquery->ops_gentry]->pst_rgroot = 
	    subquery->ops_root->pst_sym.pst_value.
	    pst_s_root.pst_union.pst_next;
	union_grv->opv_gsubselect->opv_subquery = 
	union_grv->opv_gquery = subquery->ops_union;
	if (global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
	{
	    *subqpp = subquery->ops_union; /* for distributed this element should
				** be linked back into the subquery list, FIXME
				** needs to be fixed in a local/star single thread
				** architecture */
	    subquery->ops_union->ops_next = subquery->ops_next;
	    subqpp = &subquery->ops_union->ops_next;
	}
	subquery = subquery->ops_union; /* mark actual head
				** of union view */
    }
}

/*{
** Name: OPJ_USEHASHJOIN - check whether hash join is in use
**
** Description:
**	This function examines the hash join startup parameter and OP162
**	trace point to determine whether hash joins should be used.
**	The config.dat parameter turns "opf_hash_join" ON and OFF, and
**	the trace point toggles the run-time behavior.
**
** Inputs:
**	subquery - Subquery state info.  
**
** Outputs:
**
**	Returns:
**	    TRUE - if hash joins should be used
**	    FALSE - if hash joins should NOT be used
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-jul-02 (toumi01)
**	    Created
**	11-june-03 (inkdo01)
**	    Add support of "set [no]hash" statement.
**	10-sep-04 (inkdo01)
**	    Remove op162 as hash toggle, now equivalent to
**	    SET PARALLEL
**	3-feb-06 (dougi)
**	    Add support for override hints.
[@history_template@]...
*/
static bool
opj_usehashjoin( OPS_SUBQUERY *subquery )
{
    bool use_hash_join_config = FALSE;
    bool use_hash_join_trace  = FALSE;
    bool use_hash_join;

    if (subquery->ops_global->ops_cb->ops_alter.ops_hash)
	use_hash_join_config = TRUE;
    use_hash_join = ( use_hash_join_config != use_hash_join_trace );
    if (subquery->ops_global->ops_cb->ops_override & PST_HINT_HASH)
	use_hash_join = TRUE;
    else if (subquery->ops_global->ops_cb->ops_override & PST_HINT_NOHASH)
	use_hash_join = FALSE;
    return(use_hash_join);
}

/*{
** Name: OPJ_CKHASHEQC	- check potential hash join for nulljoin and
**	join on UDT columns (neither of which is allowed).
**
** Description:
**      This function examines join eqclass(es) to determine if it's a 
**	nulljoin or if any of the columns in the attrlist are UDTs. Neither
**	are permitted for hash joins.
**
** Inputs:
**	sq	- Subquery state info.  
**	sjeqc	- join eqclass
**
** Outputs:
**
**	Returns:
**	    TRUE - if the join passes the eqc tests
**	    FALSE - if it's a nulljoin or a join on one or more UDTs
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      23-aug-01 (inkdo01)
**          Created
**	16-jan-02 (inkdo01)
**	    Added date type to list.
**	2-may-02 (inkdo01)
**	    Removed date after assuring date status bits are consistent.
**	23-jan-03 (inkdo01)
**	    Added C data type, because blanks are removed before they
**	    compare and it's too hard to insert a function call to do it.
**	21-mar-03 (inkdo01)
**	    Check hash key length isn't too big.
**	1-apr-03 (inkdo01)
**	    Add Unicode types and alternate collated char/varchar to the 
**	    list (because more than one value may collate equal).
**	    Future fix should obviate the need for these exceptions.
[@history_template@]...
*/
static bool
opj_ckhasheqc(
	OPS_SUBQUERY	*sq,
	OPE_IEQCLS	sjeqc)

{
    OPE_IEQCLS	eqcno;
    OPO_EQLIST	*joinlist;
    OPZ_AT	*abase = sq->ops_attrs.opz_base;
    i4		maxattr = sq->ops_attrs.opz_av;
    OPZ_BMATTS	*attrmap;
    OPZ_ATTS	*attrp;
    OPZ_IATTS	attno;
    i4		hashlen = 0;
    i2		i;
    bool	single = TRUE;


    /* Set up loop to work with single or multi eqc join. */
    if (sjeqc < sq->ops_eclass.ope_ev)
	eqcno = sjeqc;		/* single attr join */
    else
    {
	/* Multiple attr join - prepare variables for for-loop. */
	single = FALSE;
	joinlist = sq->ops_msort.opo_base->opo_stable[sjeqc -
			sq->ops_eclass.ope_ev]->opo_eqlist;
				/* array of eqc's */
	eqcno = joinlist->opo_eqorder[0];
    }

    for (i = 0; eqcno != OPE_NOEQCLS; i++, 
				eqcno = joinlist->opo_eqorder[i])
    {
	/* First check for nulljoin - only need eqc for that. */
	if (sq->ops_eclass.ope_base->ope_eqclist[eqcno]->ope_nulljoin)
	    return(FALSE);	/* not allowed */

	/* Now loop over attributes in this eqclass checking for 
	** UDTs. */
	attrmap = &sq->ops_eclass.ope_base->ope_eqclist[eqcno]
		->ope_attrmap;	/* address attrs in eqclass */
	for (attno = -1; (attno = BTnext((i4)attno, (char *)attrmap,
		maxattr)) >= 0; )
	{
	    attrp = abase->opz_attnums[attno];
				/* address each attr in map */
	    if (abs(attrp->opz_dataval.db_datatype) >= DB_DT_ILLEGAL ||
		abs(attrp->opz_dataval.db_datatype) == DB_CHR_TYPE ||
		abs(attrp->opz_dataval.db_datatype) == DB_NCHR_TYPE ||
		abs(attrp->opz_dataval.db_datatype) == DB_NVCHR_TYPE ||
		(sq->ops_global->ops_adfcb->adf_collation != NULL &&
		(abs(attrp->opz_dataval.db_datatype) == DB_CHA_TYPE ||
		abs(attrp->opz_dataval.db_datatype) == DB_VCH_TYPE)))
		
		return(FALSE);	/* do the UDT/C check */
	}

	hashlen += attrp->opz_dataval.db_length;
				/* cumulative hash key length */
	if (hashlen > DB_MAXSTRING)
	    return(FALSE);	/* assure hash key isn't too big */

	if (single)
	    return(TRUE);	/* if single eqc, we're done */
    }

    return(TRUE);	/* successful multi-eqc join check */
}

/*{
** Name: OPJ_SORTEDAGG - Try to convert hash aggregation into sorted
**
** Description:
**      This function determines whether the ordering of the underlying
**	CO tree is consistent with the grouping columns of the hash
**	function aggregates being computed by the current subquery. The
**	ordering need only be a permutation of the grouping columns.
**
**	If the ordering exists and is consistent, we can do this
**	aggregation as a sorted aggregate instead of hashed.  (As
**	long as no additional sort is needed, sorted aggregation is
**	faster than hashed aggregation, and there's no possibility
**	of overflow to disk.)
**
** Inputs:
**	sq	- Subquery state info.  
**
** Outputs:
**	None.
**	Subquery is converted to RFAGG if appropriate.
**
** Side Effects:
**	    none
**
** History:
**      19-jan-01 (inkdo01)
**          Created
**	22-aug-01 (inkdo01)
**	    Added code to exclude UDTs from hash aggregation (no guarantee
**	    that "=" values have same underlying bit value so hash might not
**	    work).
**	16-jan-02 (inkdo01)
**	    Add date types to above list - they're not yet guaranteed to
**	    be normalized.
**	2-may-02 (inkdo01)
**	    Removed date after assuring date status bits are consistent.
**	23-jan-03 (inkdo01)
**	    Added C data type, because blanks are removed before they
**	    compare and it's too hard to insert a function call to do it.
**	21-mar-03 (inkdo01)
**	    Check hash key length isn't too big.
**	1-apr-03 (inkdo01)
**	    Add Unicode types and alternate collated char/varchar to the 
**	    list (because more than one value may collate equal).
**	    Future fix should obviate the need for these exceptions.
**	1-Sep-2006 (kschendel)
**	    Adapted Doug's original opj_groupord to new situation, converting
**	    hash agg to sorted agg. and renaming function in the process.
**	7-nov-2006 (dougi)
**	    All these years, the wrong flag was being returned for the 
**	    data type tests - fix it!
**	6-feb-2008 (dougi)
**	    Slight fix to Karl's change to drop useless sorts.
**	27-Jun-2008 (kibro01 on behalf of jgramling) b120623
**          Condition was treating partial ISAM case (opo_ordeqc=0 and
**          opo_storage=DB_HEAP_STORE) as if it were were fully sorted.
**          Only do hfagg->rfagg transformation if data is guaranteed to be
**          sorted.
*/
static void
opj_sortedagg(
	OPS_SUBQUERY	*sq)

{
    OPO_CO	*cop = sq->ops_bestco;
    OPE_BMEQCLS	byeqcmap;
    PST_QNODE	*byexpr;
    PST_QNODE	*varp;
    OPE_IEQCLS	eqcno;
    OPZ_IATTS	attno;
    i2		bycount, i;


    /* Make sure there is a CO tree (sanity check) and that there is an
    ** ordering on the CO-tree. */

    if (cop == (OPO_CO *) NULL)
       return;		/* No joy */

    /* Even if there is no ordering, we might have left a "sort" node that
    ** does nothing - just get rid of it. */
    if (cop->opo_ordeqc < 0 || cop->opo_storage != DB_SORT_STORE)
    {
	if (cop->opo_sjpr == DB_RSORT && cop->opo_storage == DB_SORT_STORE &&
		cop->opo_outer)
	    sq->ops_bestco = sq->ops_bestco->opo_outer;
	return;
    }

    /* Clear the by-list eqcmap, then fill with eqc's of VAR's in
    ** by list.  Note that we can't handle arbitrary exprs!
    ** we'd have to do some kind of "are these expr parse tree
    ** fragments identical" trick.
    */

    MEfill((u_i2)sizeof(byeqcmap), (u_char)0x00, (PTR)&byeqcmap);
    for (byexpr = sq->ops_byexpr, bycount = 0; 
		byexpr && byexpr->pst_sym.pst_type == PST_RESDOM;
		bycount++, byexpr = byexpr->pst_left)
    {
	varp = byexpr->pst_right;

	if (varp->pst_sym.pst_type != PST_VAR)
	    return;
	attno = varp->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
	eqcno = sq->ops_attrs.opz_base->opz_attnums[attno]
			->opz_equcls;	/* get eqcno from VAR node */
	BTset(eqcno, (PTR)&byeqcmap);	/* set eqc in map for later */
    }

    /* Check for single attr ordering and if it matches by list. */
    if (sq->ops_eclass.ope_ev > cop->opo_ordeqc)	/* single att */
    {
	if (bycount != 1 || eqcno != cop->opo_ordeqc)
	   return;                     /* No match, leave agg alone */
    }
    else
    {
	OPO_EQLIST      *sortlist;

	/* Multi-attr ordering - loop over bylist eqcmap to assure it is 
	** covered by the high order subset of ordering. */

	sortlist = sq->ops_msort.opo_base->opo_stable[
	    cop->opo_ordeqc - sq->ops_eclass.ope_ev]->opo_eqlist;

	for (i = 0; i < bycount; i++)
	{
	    if ((eqcno = sortlist->opo_eqorder[i]) == OPE_NOEQCLS)
		return;			/* more byexprs than ord cols */
	    if (!BTtest(sortlist->opo_eqorder[i], (PTR)&byeqcmap))
		return;			/* high order ord col NOT in 
					** by list */
	}
    }
    /* Looks like we can do this aggregation using sorted aggs */
    sq->ops_sqtype = OPS_RFAGG;
    sq->ops_global->ops_rangetab.opv_base->
			opv_grv[sq->ops_gentry]->opv_created = OPS_RFAGG;

}	/* end of opj_sortedagg */

/*{
** Name: opj_enum	- prepare for enumeration
**
** Description:
**      Prepare for the enumeration phase, and perform some cleanup
**	afterwards.
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
**	    none
**
** History:
**      1-sep-92 (ed)
**          initial creation for union view support
**      1-feb-95 (inkdo01)
**          added code to alloc new CO node for unions to accumulate 
**          costs of constituent selects
**	27-feb-96 (inkdo01)
**	    Minor fix to above code to correct bug 74924 (fromless selects 
**	    have no CO tree)
**	18-dec-97 (nanpr01)
**	    I missed a place where opc_pagesize is not getting set.
**	    Unicenter union query revealed this problem.
**	27-sep-00 (inkdo01)
**	    Move op156 call to opn_enum, to pick up composite histograms.
**	24-jan-01 (inkdo01)
**	    Added code to enable hash aggregation.
**	15-aug-02 (inkdo01)
**	    Restored op162 logic to prior to "no hash" CBF parameter introduction.
**	    op162 will turn off hash aggregation as before, leaving it enabled by
**	    default. This differs from hash join which is disabled by default and 
**	    toggled by op162. Hash aggregation has not had the problems of hash join
**	    and should remain enabled by default.
**	14-apr-03 (toumi01)
**	    Because of SEGVs and incorrect aggregate query results with hash
**	    aggregation, which could not be handled easily from an operational
**	    point of view with OP162, again make this feature subject to
**	    config.dat's opf_hash_join parameter.
**	 8-feb-05 (hayke02)
**	    Initialise the union_cop opo_pagesize to DB_MINPAGESIZE because
**	    we will not have a ops_bestco if we have a union of simple
**	    aggregates, and we were not initialising opo_pagesize for a NULL
**	    ops_bestco. This change fixes bug 113822, problem INGSRV 3142.
**	17-may-05 (inkdo01)
**	    Fine tune conditions for using hash aggregation.
**	5-july-05 (inkdo01)
**	    Modify hash aggregation tuple, page estimates.
**	20-mar-06 (dougi)
**	    Call hint initialization, if required.
**	30-Jun-2006 (kschendel)
**	    Do agg modcost here, after enumeration, and store modified
**	    estimates in subquery global range var.  This leaves bestco
**	    reflecting the agg input, which can be useful.
**	1-Sep-2006 (kschendel)
**	    Invert hash/sorted aggregation conversion;  opa now produces
**	    hash aggs when it can, we'll change them to sorted if appropriate.
**	17-oct-2006 (dougi)
**	    Only do opn_modcost() call for non-Star queries.
**	21-nov-2006 (dougi)
**	    Move opn_modcost() call to opn_enum(). Enumeration memory has 
**	    already been freed by the time we get here, leading to SEGVs.
*/
static OPS_SUBQUERY **
opj_enum(
	OPS_SUBQUERY       **subqpp)
{
    OPS_SUBQUERY        **nextpp;           /* next subquery to analyze */
    OPS_SUBQUERY	*subquery;
    OPS_STATE		*global;
    
    subquery = *subqpp;
    nextpp = &subquery->ops_next;
    global = subquery->ops_global;
    if (global->ops_qheader->pst_hintcount > 0 ||
	global->ops_cb->ops_override & PST_HINT_ORDER)
	opj_hintinit(subquery);		/* init sq hint array/flags */
    opn_enum( subquery );               /* produce the best CO tree 
					** by enumeration of all possible
					** plans (equivalent to old "enum"
					** routine)
					*/
    {   /* post processing phase of enumeration,
	** 1) add constant function attributes for certain cartesean 
	**    products
	** 2) define multi-attribute orderings for sort nodes, joins
	**    etc. since this info was deleted when the enumeration
	**    memory stream was removed 
	*/
	OPE_IEQCLS		    maxeqcls;
	maxeqcls = subquery->ops_eclass.ope_ev; /* save OLD maxeqcls for
					    ** opj_maordering, since
					    ** opj_cartprod could change this
					    */
	subquery->ops_msort.opo_sv = 0;
	subquery->ops_msort.opo_base = NULL; /* this was allocated out of
					** enumeration memory which has
					** been deallocated */
	if ((   (    subquery->ops_vars.opv_cart_prod 
#if 0
/*
need this no matter what the duplicate semantics e.g. the follow would fail
select * from iiprotect p where EXISTS
*/
( select r.relid from iirelation r, iiqrytext q where
    r.relid = 'test38' and
    r.reltid = p.protabbase and
    r.reltidx = p.protabidx and
    p.proqryid1 = q.txtid1 and
    p.proqryid2 = q.txtid2	)

**		    &&
**		    (subquery->ops_duplicates == OPS_DKEEP)
#endif
		    &&
		    subquery->ops_bestco
		)
		||
		(
		    (maxeqcls == 0)	/* this could occur for a single variable
				    ** query */
		    &&
		    (subquery->ops_vars.opv_rv > 0) /* make sure it is not a
				    ** constant expression */
		)
	    )
	    )
	{   /* special case processing needed for cart product which keep
	    ** duplicates ,... also create constant function attribute
	    ** if there is no equivalence class and some range variables
	    ** are defined */
	    opj_cartprod(subquery, subquery->ops_bestco, OPE_NOEQCLS);
	}
	{
	    OPJ_MASTATE	mastate; /* used to define state of
				    ** CO tree processing */
	    global->ops_mstate.ops_usemain = TRUE; /* need to allocate new
				    ** memory out of long-term
				    ** memory stream */
	    mastate.opj_subquery = subquery;
	    mastate.opj_maxeqcls = maxeqcls;
	    if ((global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
		&&
		subquery->ops_dist.opd_bestco)
	    {   /* process array of plans for distributed */
		OPD_ISITE	    site;
		for (site = global->ops_gdist.opd_dv; --site >= 0;)
		{
		    OPO_CO	*site_cop;
		    site_cop = subquery->ops_dist.opd_bestco->opd_colist[site];
		    if (site_cop)
		    {
			mastate.opj_nodeid = 0;
			MEfill(sizeof(mastate.opj_ojmap), (u_char)0,
			    (PTR)&mastate.opj_ojmap);
			opj_maordering(&mastate, site_cop);
			site_cop->opo_variant.opo_local->opo_target = site;
			opj_exact(&mastate, site_cop, OPE_NOEQCLS, FALSE,
			    OPE_NOEQCLS, FALSE); /* convert all inexact orderings to
						** exact orderings for query
						** compilation */
		    }
		}
	    }
	    else if (subquery->ops_bestco)
	    {
		bool	    valid;
		OPO_ISORT	    jordering;

		mastate.opj_nodeid = 0;
		MEfill(sizeof(mastate.opj_ojmap), (u_char)0,
		    (PTR)&mastate.opj_ojmap);
		opj_maordering(&mastate, subquery->ops_bestco); /* call
				    ** after opj_cartprod since
				    ** new eqcls may be defined
				    ** by it, also pass the OLD
				    ** maxeqcls in order to
				    ** determine if multi-attribute
				    ** orderings need to be defined
				    */
		if (subquery->ops_bfs.opb_mask & OPB_NEWOJBF)
		    opl_bfiftrue(subquery); /* traverse boolean factors
				    ** and insert iftrue functions */
		subquery->ops_bestco->opo_variant.opo_local->opo_target = 
		    subquery->ops_dist.opd_target; 
		if ((subquery->ops_msort.opo_mask & OPO_MREMOVE)
		    &&
		    (subquery->ops_bestco->opo_sjpr != DB_RSORT))
		{
		    /* top sort node has been remove so make sure that
		    ** the correct ordering is requested */
		    OPO_EQLIST	*tsortp;
		    tsortp = subquery->ops_msort.opo_tsortp;
		    if (tsortp->opo_eqorder[1] == OPE_NOEQCLS)
			jordering = subquery->ops_msort.opo_tsortp->opo_eqorder[0]; /* single
						** attribute ordering for sortedness */
		    else
			jordering = opo_cordering(subquery, 
			    subquery->ops_msort.opo_tsortp, FALSE); /* find
						** the exact ordering associated with this
						** list */
		    valid = TRUE;
		    if (subquery->ops_msort.opo_mask & OPO_MDESC)
		    {	/* Descending sort with no top sort - look for 1st
			** btree orig node down left hand side of CO tree. It
			** must be read backwards.
			*/
			OPO_CO	*cop;
			bool	found = FALSE;

			for (cop = subquery->ops_bestco; cop; cop = cop->opo_outer)
			 if (cop->opo_sjpr == DB_ORIG)
			 {
			    cop->opo_variant.opo_local->opo_mask |= OPO_READBACK;
						/* Flag the OPO_CO */
			    found = TRUE;
			    break;		/* exit loop */
			 }
		/*	 else if (cop != subquery->ops_bestco &&
						 cop->opo_sjpr != DB_SJ) break; */
						/* must be one or the other */
			if (!found) opx_error(E_OP08B2_DESC_ERROR);
		    }
		}
		else
		{
		    jordering = OPE_NOEQCLS;
		    valid = FALSE;	
		}
		opj_exact(&mastate, subquery->ops_bestco, jordering, valid,
		    jordering, valid);  /* convert all inexact orderings to
					** exact orderings for query
					** compilation */
	    }
	    global->ops_mstate.ops_usemain = FALSE; /* disable memory
						** redirection */
	}
    }

    switch (subquery->ops_sqtype)
    {
    case OPS_MAIN:
	break;			    /* if the main query is reached
					** then no storage needs to be
					** re-allocated */
    case OPS_RFAGG:
    case OPS_HFAGG:
    {   
	if (global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
	{	/* for distributed, always treat these queries as FAGG type since
	    ** the temporary will be instantiated */
	    subquery->ops_sqtype = OPS_FAGG;
	    subquery->ops_mode = PSQ_RETINTO;
	    subquery->ops_result = subquery->ops_gentry;
	    global->ops_rangetab.opv_base->opv_grv[subquery->ops_result]->
		opv_gsubselect = NULL;      /* remove subselect descriptor*/
	    global->ops_rangetab.opv_base->opv_grv[subquery->ops_result]->
		opv_created = OPS_FAGG;     /* assign new creatioin type to
					    ** global range variable */
	    goto fixmerfagg;
	}
    }
    case OPS_VIEW:
    case OPS_RSAGG:
    case OPS_SELECT:
    case OPS_UNION:
    {
	OPS_SUBQUERY	*removesq;  /* subquery which will be removed */
	removesq = *subqpp;
	*subqpp = removesq->ops_next;   /* remove subquery from execution
					** list since it will only be
					** executed as part of an SEJOIN */
	removesq->ops_next = NULL;      /* subquery is now
					** only referenced as part of the
					** global range variable */
	nextpp = subqpp;		    /* query has been removed, so do not
					** skip over next query */
	/* fall through */
    }
    case OPS_FAGG:
    case OPS_PROJECTION:
    case OPS_SAGG:
    {
	OPO_PARM	*gcostp;	/* Result cost in global rv */
        OPS_SUBQUERY	*worksqp;
        /* -a function aggregate produces a temporary relation which
	** has a global range variable associated with it
	** - save a pointer to the cost ordering node for statistics
	** to be used in subsequent optimizations
	*/
	/* case in which global range variable would not exist would
	** be a union in the main query.
	*/
	/* Skip estimates if they were already produced by opn_modcost() 
	** for aggregate queries.
	*/
fixmerfagg: 
	if ( (subquery->ops_gentry != OPV_NOGVAR)
	    &&
	     (subquery->ops_sqtype != OPS_UNION)
	    &&
	    !(subquery->ops_mask2 & OPS_AGGCOST)
	    )
	{
	    gcostp = &global->ops_rangetab.opv_base->opv_grv[subquery->ops_gentry]->opv_gcost;
	    MEfill(sizeof(OPO_PARM), 0, (PTR) gcostp);
	    gcostp->opo_pagesize = DB_MINPAGESIZE;
	    gcostp->opo_reltotb = 1;
	    gcostp->opo_tups = 0;
	    if (subquery->ops_bestco)
		STRUCT_ASSIGN_MACRO(subquery->ops_bestco->opo_cost, *gcostp);
            if (subquery->ops_union)  /* accumulate for unions */
            {
            	for (worksqp = subquery->ops_union; worksqp != 
			(OPS_SUBQUERY *) NULL; worksqp = worksqp->ops_union)
                 if (worksqp->ops_bestco)
            	{
                    gcostp->opo_tups += worksqp->ops_bestco->opo_cost.opo_tups;
                                        /* roll in useful cost values */
                    gcostp->opo_dio += worksqp->ops_bestco->opo_cost.opo_dio;
                    gcostp->opo_cpu += worksqp->ops_bestco->opo_cost.opo_cpu;
		    if (worksqp->ops_bestco->opo_cost.opo_pagesize > 
			gcostp->opo_pagesize)  
		        gcostp->opo_pagesize = 
			    worksqp->ops_bestco->opo_cost.opo_pagesize;
            	}
            }
        }
	break;
    }
#ifdef E_OP048D_QUERYTYPE
    default:
	opx_error(E_OP048D_QUERYTYPE);
#endif
    }

    if ((subquery->ops_sqtype != OPS_MAIN) || (global->ops_procedure->pst_isdbp))
    {
	/* each subquery has large preallocated arrays of ptrs which should
	** be reused for the next subquery.  This can be avoided for the
	** main subquery which should be last one in the list */
	/* if not a DB procedure then it can be left in place, otherwise
	** copy out even for main query since it can be used for subsequent
	** queries */
	opj_outcopy(subquery, TRUE);

    }
    if ((subquery->ops_sqtype == OPS_RSAGG)
	||
	(subquery->ops_sqtype == OPS_SAGG)
       )
    {   /* check for min optimization to see if ordering is available
	** on outer which will require only the first tuple to be read */
	opj_minop(subquery);
    }
    if (subquery->ops_union && (subquery->ops_sqtype != OPS_UNION))
	opa_union(subquery);	    /* relabel union subqueries once
					** the head of the list is found
					*/

    /* Attempt to convert hash aggregations (back) into sorted aggregations,
    ** if the output of the subquery just happens to be properly sorted.
    */
    if (subquery->ops_sqtype == OPS_HFAGG)
    {
	opj_sortedagg(subquery);
    }
    return (nextpp);
}

/*{
** Name: opj_hintinit	- initialize sq hint structures
**
** Description:
**      If query has array of hint descriptors, copy them to subquery
**	and initialize the flags
**
** Inputs:
**      subquery                        ptr to subquery being processed
**
** Outputs:
**
**	Returns:
**	    
** History:
**	20-mar-06 (dougi)
**	    Written for optimizer hints project.
*/
static VOID
opj_hintinit(
	OPS_SUBQUERY	*subquery)

{
    OPS_STATE		*global = subquery->ops_global;
    OPS_TABHINT		*hintp;
    PST_HINTENT		*tabhp;
    OPV_RT		*vbase = subquery->ops_vars.opv_base;
    OPV_VARS		*varp;
    i4			i, j;


    /* Allocate memory for the sq-based hint array, then copy 'em. */
    subquery->ops_hintcount = global->ops_qheader->pst_hintcount;
    tabhp = global->ops_qheader->pst_tabhint;
    if (subquery->ops_hintcount)
	hintp = subquery->ops_hints = (OPS_TABHINT *)opu_memory(
		global, subquery->ops_hintcount * sizeof(OPS_TABHINT));
    else hintp = (OPS_TABHINT *) NULL;

    for (i = 0; i < subquery->ops_hintcount; i++, tabhp++, hintp++)
    {
	MEcopy((char *)&tabhp->pst_dbname1, sizeof(DB_TAB_NAME), 
				(char *)&hintp->ops_name1);
	hintp->ops_vno1 = OPV_NOVAR;
	for (j = 0; j < subquery->ops_vars.opv_rv; j++)
	{
	    /* Loop to find match, then assign index to ops_vno1. */
	    varp = vbase->opv_rt[j];
	    if (varp->opv_grv->opv_qrt >= 0 &&
		MEcmp((char *)&hintp->ops_name1,
		    (char *)&global->ops_qheader->
			pst_rangetab[varp->opv_grv->opv_qrt]->pst_corr_name,
		    sizeof(DB_TAB_NAME)) == 0)
	    {
		hintp->ops_vno1 = j;
		break;
	    }
	}

	MEcopy((char *)&tabhp->pst_dbname2, sizeof(DB_TAB_NAME), 
				(char *)&hintp->ops_name2);
	hintp->ops_vno2 = OPV_NOVAR;
	for (j = 0; j < subquery->ops_vars.opv_rv; j++)
	{
	    /* Loop to find match, then assign index to ops_vno2. */
	    varp = vbase->opv_rt[j];
	    if (varp->opv_grv->opv_qrt >= 0 &&
		MEcmp((char *)&hintp->ops_name2,
		    (char *)&global->ops_qheader->
			pst_rangetab[varp->opv_grv->opv_qrt]->pst_corr_name,
		    sizeof(DB_TAB_NAME)) == 0)
	    {
		hintp->ops_vno2 = j;
		break;
	    }
	}

	hintp->ops_hintcode = tabhp->pst_hintcode;
	hintp->ops_hintused = FALSE;
	if (hintp->ops_hintcode == PST_THINT_INDEX)
	    subquery->ops_mask2 |= OPS_IXHINTS;
	else if (hintp->ops_hintcode == PST_THINT_FSMJ ||
		hintp->ops_hintcode == PST_THINT_HASHJ ||
		hintp->ops_hintcode == PST_THINT_PSMJ)
	    subquery->ops_mask2 |= OPS_NKJHINTS;
	else if (hintp->ops_hintcode == PST_THINT_KEYJ)
	    subquery->ops_mask2 |= OPS_KJHINTS;
    }

    if (subquery->ops_global->ops_cb->ops_override & PST_HINT_ORDER)
	subquery->ops_mask2 |= OPS_ORDHINT;
    subquery->ops_mask2 |= OPS_HINTPASS;

}

/*{
** Name: opj_subuvvar	- substitute union view vars
**
** Description:
**      Traverse the target list and substitute references to the
**	union view by the respective resdom expression.
**
** Inputs:
**      subquery                        ptr to subquery partition
**					of union view currently
**					being processed 
**	fatherp				ptr to subquery aggregate
**					defining joinop attrs in
**					qnodepp target list
**      qnodepp                         ptr to query tree node
**					being processed
**
** Outputs:
**
**	Returns:
**	    
**	Exceptions:
**
** Side Effects:
**
** History:
**      29-jan-94 (ed)
**          initial creation to support aggregate union view optimization
[@history_template@]...
*/
static VOID
opj_subuvvar(
	OPS_SUBQUERY	*subquery,
	OPS_SUBQUERY	*fatherp,
	PST_QNODE	**qnodepp)
{
    for (;*qnodepp; qnodepp = &(*qnodepp)->pst_left)
    {
	if ((*qnodepp)->pst_sym.pst_type == PST_VAR)
	{
	    PST_QNODE	    *uvp;
	    OPZ_IATTS	    attr;
	    attr = fatherp->ops_attrs.opz_base->opz_attnums[(*qnodepp)->
		pst_sym.pst_value.pst_s_var.pst_atno.db_att_id]->
		opz_attnm.db_att_id;	/* get resdom number of attribute
					** as defined in view */
	    for (uvp = subquery->ops_root->pst_left; 
		uvp && (uvp->pst_sym.pst_type == PST_RESDOM); 
		uvp = uvp->pst_left)
	    {
		if (uvp->pst_sym.pst_value.pst_s_rsdm.pst_rsno == attr)
		{
		    if (uvp->pst_right->pst_sym.pst_type == PST_VAR)
		    {	/* copy var node info */
			STRUCT_ASSIGN_MACRO(uvp->pst_right->pst_sym.pst_value.pst_s_var,
			    (*qnodepp)->pst_sym.pst_value.pst_s_var);
		    }
		    else
		    {	/* make copy of expression from resdom */
			*qnodepp = uvp->pst_right;
			opv_copytree (subquery->ops_global, qnodepp);
		    }
		    return;
		}
	    }
	    opx_error(E_OP04B1_AGGUV);	/* expecting to find union view definition */
	}
	else
	    opj_subuvvar(subquery, fatherp, &(*qnodepp)->pst_right);
    }
}

/*{
** Name: opj_uvagg	- check for union view aggregate optimizations
**
** Description:
**      Check if union view aggregate optimizations are semantically
**	possible.  If so then force the aggregate into the union view
**	and apply a compensating action at the subquery using the union
**	view.  The aggregate cannot eliminate duplicates except for min/max
**	and there cannot be more than one relation in the subquery referencing
**	the aggregate.
**
** Inputs:
**      subquery                        partition of union view to
**					which the aggregate optimization
**					can be applied
**	terminatep			last partition of union view
**
** Outputs:
**
**	Returns:
**	    
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      19-jan-94 (ed)
**          initial creation for star union view aggregate optimization
[@history_template@]...
*/
static VOID
opj_uvagg(
	OPS_SUBQUERY	**subquerypp,
	OPS_SUBQUERY	*terminatep)
{
    OPS_STATE	    *global;
    OPV_GRT	    *gbase;
    OPV_IGVARS	    iuv_var;/* index into global range table of
			    ** union view var */
    OPV_GRV	    *gvarp; /* ptr to global var descriptor
			    ** union view */
    OPS_SUBQUERY    *fatherp; /* ptr to parent subquery which
			    ** evaluates aggregate and references
			    ** union view */
    PST_QNODE	    *resdomp; /* traverse the target list */
    OPS_SUBQUERY    *unionp; /* used to traverse partitions of
			    ** the union */
    OPS_SUBQUERY    *last_unionp;   /* ptr to last union in list */
    OPS_SUBQUERY    *subquery;
    OPS_SUBQUERY    *const_subquery; /* create a constant view
			    ** subquery to hold the results
			    ** of the simple aggregate */

    subquery = *subquerypp;
    global = subquery->ops_global;
    iuv_var = subquery->ops_gentry;
    gbase = global->ops_rangetab.opv_base;
    fatherp = subquery->ops_agg.opa_father;
    /* first subquery in list, so check for possible aggregate
    ** optimization and apply to remainder of partitions as well */
    if ((iuv_var < 0)
	||
	!fatherp
	||
	(fatherp->ops_sqtype != OPS_SAGG) /* for now look
			    ** for simple aggregates only */
	||
	(fatherp->ops_vars.opv_rv > 1) /* only allow one table
			    ** (the view)in aggregate calculation */
	||
	(fatherp->ops_duplicates == OPS_DREMOVE)
	) 
	return;
    gvarp = gbase->opv_grv[iuv_var];
    if (gvarp->opv_gmask & OPV_MULTI_REFERENCE)
			    /* avoid changing union view used in
			    ** multiple parent subqueries */
	return;
    for (unionp = *subquerypp; unionp != terminatep; 
	unionp = unionp->ops_next)  /* find head of list for
			    ** processing since newly defined parameter
			    ** constants in star OPC need to be
			    ** defined in order , FIXME star QEF
			    ** should be able to initialize all
			    ** parameters before beginning execution
			    ** and not rely on lower parameter constants
			    ** being initialized */
    {
	subquery = unionp;
    }

    for (resdomp = fatherp->ops_root->pst_left;
	resdomp && (resdomp->pst_sym.pst_type == PST_RESDOM);
	resdomp = resdomp->pst_left)
    {
	if (!(resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT)
	    ||
	    (resdomp->pst_right->pst_sym.pst_type != PST_AOP)
	    ||
	    (resdomp->pst_right->pst_sym.pst_value.pst_s_op.pst_distinct
		== PST_DISTINCT)
	    ||
	    (resdomp->pst_right->pst_sym.pst_value.pst_s_op.pst_opno 
		==  global->ops_cb->ops_server->opg_avg) /*
			    ** FIXME - for avg, create a count attribute
			    ** and sum attribute, and calculate avg
			    ** by sum / ifnull(0, count) ... in case
			    ** the total count is zero */
	    ||
	    (resdomp->pst_right->pst_right->pst_sym.pst_type != PST_CONST))
	    return;	    /* duplicates need to be removed */
    }
    /* - move the aggregate evaluation to each partition
    ** - take aggregate target list of the father and copy to each
    ** partition, dropping the old target list, but using it to evaluate
    ** any expressions in the target list of the aggregates
    ** - replace father target list with new references to the appropriate
    ** var of the modified union view, making special modifications
    ** for the average aggregate
    */
    gvarp->opv_gmask &= ~OPV_UVPROJECT;	/* since union view query modified
				** reset projection bits */
    gvarp->opv_attrmap = NULL;
    const_subquery = NULL;
    for (unionp = *subquerypp; unionp != terminatep; 
	unionp = unionp->ops_next)
    {
	PST_QNODE	*copy;
	PST_QNODE	*const_qnode; /* constant view target list */
	PST_QNODE	**const_qnodepp; /* used to initialize constant
				    ** target list */
	unionp->ops_union = NULL;   /* this subquery becomes a simple
				    ** aggregate */
	unionp->ops_root->pst_sym.pst_value.pst_s_root.pst_union
	    .pst_next = NULL;
	const_qnode = fatherp->ops_root; /* init list for const view */
	copy = const_qnode->pst_left; /* get ptr to copy of
				    ** target list */
	opv_copynode(global, &const_qnode); /* make copy of root node
				    ** only */
	const_qnode->pst_right = opv_qlend(global); /* empty
				    ** where clause for constant view */
	const_qnode->pst_left = NULL;
	MEfill(sizeof(const_qnode->pst_sym.pst_value.pst_s_root),
	    (u_char)0, (PTR)&const_qnode->pst_sym.pst_value.pst_s_root);
				    /* zero target list for constant view */
	const_qnode->pst_sym.pst_value.pst_s_root.pst_qlang =
	    fatherp->ops_root->pst_sym.pst_value.pst_s_root.pst_qlang;
	const_qnode->pst_sym.pst_value.pst_s_root.pst_union.pst_dups =
	const_qnode->pst_sym.pst_value.pst_s_root.pst_dups = PST_ALLDUPS;
	const_qnodepp = &const_qnode->pst_left;	/* add resdoms for
				    ** constant view */
	/* work on converting partition into simple aggregate */
	opv_copytree(global, &copy); /* copy target list for this
				    ** aggregate partition */
	opj_subuvvar(unionp, fatherp, &copy); /* substitute the var nodes
				    ** with dereferenced partition values */

	unionp->ops_dist.opd_dv_base = global->ops_parmtotal+1; /* this will be
					** the first constant allocated for
					** the subquery */
	for (resdomp = copy;
	    resdomp && (resdomp->pst_sym.pst_type == PST_RESDOM);
	    resdomp = resdomp->pst_left)
	{   /* relabel the constant nodes for the aggregate results */
	    resdomp->pst_right->pst_right->pst_sym.pst_value.pst_s_cnst.
		pst_parm_no = ++global->ops_parmtotal; /* allocate new
					    ** parameter constant */
	    *const_qnodepp = resdomp; /* make copy of resdom for constant
				    ** view */
	    opv_copynode(global, const_qnodepp);
	    (*const_qnodepp)->pst_right = opv_constnode(global, 
		&resdomp->pst_sym.pst_dataval,
		global->ops_parmtotal); /* allocate constant referencing
				    ** simple aggregate result */
	    const_qnodepp = &(*const_qnodepp)->pst_left;
	}
	*const_qnodepp = resdomp;    /* this should be the end of
				    ** target list marker */

	unionp->ops_union = NULL;   /* partition becomes a simple aggregate */
	unionp->ops_sqtype = OPS_SAGG;
	unionp->ops_dist.opd_cost = NULL;
	unionp->ops_dist.opd_bestco = NULL;
	unionp->ops_dist.opd_sagg_cnt = global->ops_parmtotal -
	    unionp->ops_dist.opd_dv_base + 1; /* this is the number
				    ** of simple aggregate results
				    ** which will be computed at this
				    ** node, since all simple aggregates
				    ** to be evaluated together have been
				    ** determined */
	if (fatherp->ops_duplicates == OPS_DUNDEFINED)
	    unionp->ops_duplicates = OPS_DUNDEFINED; /* inherit
				    ** duplicate handling from parent
				    ** since aggregate may be a less
				    ** restrictive "don't care" such
				    ** as min or max */
	MEfill(sizeof(subquery->ops_eclass.ope_maps.opo_eqcmap),
	    (u_char)0, (PTR)&subquery->ops_eclass.ope_maps.opo_eqcmap);
	ope_aseqcls(unionp, &subquery->ops_eclass.ope_maps.opo_eqcmap,
            unionp->ops_root->pst_left); /* reassign
                                        ** equivalence classes for vars in
                                        ** the target list since a reduced set
					** may be available */
	if ((BTnext((i4)-1, (char *)&subquery->ops_eclass.ope_maps.opo_eqcmap,
	    (i4)unionp->ops_eclass.ope_ev)< 0)
	    &&
	    (unionp->ops_vars.opv_prv <= 1))
	    unionp->ops_vars.opv_cart_prod = TRUE;   /* special case of count(*) in which
				    ** no eqc in orig node error is reported, 
				    ** FIXME - union view projection code should
				    ** probably reset this eqc map and set the
				    ** opv_cart_prod flag or a new flag to get
				    ** the creation of a placeholder attribute */
	if (unionp->ops_duplicates == OPS_DREMOVE)
	{   /* copy non-printing resdoms for duplicate semantics if necessary*/
	    for (resdomp = unionp->ops_root->pst_left;
		resdomp && (resdomp->pst_sym.pst_type == PST_RESDOM);)
	    {
		if (resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT)
		    resdomp = resdomp->pst_left;
		else
		{   /* place non printing resdom into target list */
		    PST_QNODE	    *temp_resdom;
		    temp_resdom = resdomp;
		    resdomp = resdomp->pst_left;
		    temp_resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno
			= copy->pst_sym.pst_value.pst_s_rsdm.pst_rsno+1;
		    temp_resdom->pst_left = copy;
		    copy = temp_resdom;
		}
	    }
	}
	unionp->ops_root->pst_left = copy;

	{
	    OPS_SUBQUERY	*new_subquery;
	    new_subquery = opa_initsq(global, &const_qnode,
		(OPS_SUBQUERY *)NULL);
	    new_subquery->ops_next = const_subquery;
	    const_subquery = new_subquery;
	    const_subquery->ops_dist.opd_dv_cnt = global->ops_parmtotal;/*
				    ** since simple aggregate values
				    ** where just created, send all
				    ** parameters, even undefined ones
				    ** to the ldb */
	}
	last_unionp = unionp;

    }
    last_unionp->ops_next = const_subquery;

    {
	OPS_SUBQUERY	    *previous_subquery;
	previous_subquery = NULL;
	for (unionp = const_subquery; unionp; unionp = unionp->ops_next)
	{
	    unionp->ops_sqtype = OPS_UNION;
	    unionp->ops_mode = PSQ_RETRIEVE;
	    unionp->ops_gentry = iuv_var;
	    unionp->ops_union = previous_subquery;
	    previous_subquery = unionp;
	}
	previous_subquery->ops_next = terminatep; /* insert const union 
				    * view into subquery linked list */
	previous_subquery->ops_sqtype = OPS_VIEW;
    	gvarp->opv_gquery = previous_subquery;
	gvarp->opv_gsubselect->opv_subquery = previous_subquery;

	for (unionp = const_subquery; unionp != terminatep;
	    unionp = unionp->ops_next)
	{   /* need to wait for processing until all links
	    ** for the union have been initialized */
	    opj_jeqc(unionp); /* execute first part of joinop 
			    ** processing for constant view */
	    opj_translate(unionp); /* execute second part of joinop 
			    ** processing for constant view */
	    opj_outcopy(fatherp, TRUE);/* save copies of union view
			    ** variable length joinop structures */
	}
    }

    /* convert evaluation of simple aggregate evaluation to
    ** reference constant view results */
    for (resdomp = fatherp->ops_root->pst_left;
	resdomp && (resdomp->pst_sym.pst_type == PST_RESDOM);
	resdomp = resdomp->pst_left)
    {
	PST_QNODE	*qnodep;

	qnodep = resdomp->pst_right->pst_left;
	if ((	resdomp->pst_right->pst_sym.pst_value.pst_s_op.pst_opno
		==
		global->ops_cb->ops_server->opg_scount) /* count (*) does
			    ** not have an expression */
	    ||
		(qnodep->pst_sym.pst_type != PST_VAR))
	{   /* create var node, can use resdom for template since it
	    ** was copied into every partition of union view */
	    DB_ATT_ID	    dmfattr;
	    dmfattr.db_att_id = resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
	    resdomp->pst_right->pst_left = opv_varnode(global,
		&resdomp->pst_sym.pst_dataval, iuv_var, &dmfattr);
	}
	else
	{   /* use var node directly */
	    qnodep->pst_sym.pst_value.pst_s_var.pst_vno = iuv_var;
	    qnodep->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id 
		= resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
	    STRUCT_ASSIGN_MACRO(resdomp->pst_sym.pst_dataval,
		qnodep->pst_sym.pst_dataval); /* use datatype of
			    ** result of simple aggregate, which can
			    ** be different from original var datatype */
	}
	if ((resdomp->pst_right->pst_sym.pst_value.pst_s_op.pst_opno
	    == global->ops_cb->ops_server->opg_count)
	    ||
	    (resdomp->pst_right->pst_sym.pst_value.pst_s_op.pst_opno
	    == global->ops_cb->ops_server->opg_scount))
	{   /* convert count aggregate into a sum of counts */
	    resdomp->pst_right->pst_sym.pst_value.pst_s_op.pst_opno
		= global->ops_cb->ops_server->opg_sum;
	}
	opa_resolve(subquery, resdomp->pst_right);
    }
    /* remove where clause from father query since it should have been
    ** copied into every partition of the union */
    fatherp->ops_root->pst_right = opv_qlend(global);

    /* translate the subquery again since it has been converted to
    ** a query with global range variables */
    MEfill(sizeof(fatherp->ops_vars), 0, (PTR)&fatherp->ops_vars);
    MEfill(sizeof(fatherp->ops_attrs), 0, (PTR)&fatherp->ops_attrs);
    MEfill(sizeof(fatherp->ops_eclass), 0, (PTR)&fatherp->ops_eclass);
    opj_jeqc(fatherp); /* reexecute first part of joinop processing 
		    ** since query tree has global variables again */
    opj_outcopy(fatherp, FALSE);/* save copies of union view
		    ** variable length joinop structures */
}

/*{
** Name: opj_uvsubview	- create sub views if necessary
**
** Description:
**      In the case of a union view being defined as a combination 
**      of union and union all operators, there may be a need to 
**      create a subview in which duplicates are removed as a subgroup
**      within the overall view. 
**
** Inputs:
**      subquery                        ptr to head of union view definition
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
**      1-sep-92 (ed)
**          initial creation to support union views
[@history_template@]...
*/
static OPS_SUBQUERY **
opj_uvsubview(
	OPS_SUBQUERY       *subquery,
	bool		    norows)
{
    OPS_STATE		    *global;
    /* if a mixture of union/union all is used in this view then
    ** create a "union subview" for any contiguous group of two
    ** or more partitions which require duplicates removed within 
    ** this union view,  FIXME - the case of a mixture being reduced via
    ** partition removal will still require a sort node on top of
    ** the view definition... as opposed to the current non unionall view
    ** handling which does not require a sort node but instead places
    ** non-printing resdoms in the parent query and sorts after possible
    ** tuple elimination has occurred */
    OPS_SUBQUERY	    *uvp;
    OPS_SUBQUERY	    *terminatep; /* subquery used to terminate
				    ** the enumeration processing */
    OPS_SUBQUERY	    **insertpp; /* insertion point of
				    ** the new subquery */
    OPV_IGVARS		    old_var;

    old_var = subquery->ops_gentry;
    terminatep = subquery->ops_next; /* first subquery which is not part 
				    ** of the union */
    if (norows)
	subquery = subquery->ops_union;	/* the original partition was
				    ** eliminated, so use the first available
				    ** partition */
    else
	opj_outcopy(subquery, TRUE);    /* save copies of union view
				    ** variable length joinop structures */
    global = subquery->ops_global;
    {
	OPS_SUBQUERY    *lastp; /* last partition in the
		    ** union view */
	for (lastp = subquery; 
	    lastp->ops_union; 
	    lastp = lastp->ops_union)
	    ;	/* find last element of union */
	for (insertpp = &global->ops_subquery;
	    *insertpp != lastp;
	    insertpp = &(*insertpp)->ops_next)
	    ;		    /* find subquery pointing to
			    ** last element of union, will be used
			    ** as insertion point of new subqueries */
    }
    if ((subquery->ops_mask & OPS_SUBVIEW)
	&&
	subquery->ops_union)	/* duplicate handling required since
				** a mixture of union and union all
				** views exists */
    for (uvp = subquery; uvp && uvp->ops_union; uvp = uvp->ops_union)
    {   /* note last partition in union has undefined pst_dups */
	if (uvp->ops_root->pst_sym.pst_value.
	    pst_s_root.pst_union.pst_dups == PST_NODUPS)
	{	/* this partition and the following ones with PST_NODUPS require
	    ** duplicates to be removed as a whole */
	    OPS_SUBQUERY	*enduvp; /* last contiguous union partition in
				    ** the list */
	    PST_QNODE		*rootp; /* root of parse tree which references
				    ** the new union view */
	    OPS_SUBQUERY	**removepp; /* ptr to last partition of
				    ** portion of union view to be removed */
	    OPS_SUBQUERY	*new_subquery; /* new subquery for 
				    **subview */
	    OPV_GRV		*gvarp; /* ptr to new global range var
				    ** descriptor for new subview */

	    rootp = uvp->ops_root;
	    for (enduvp = uvp->ops_union; 
		    enduvp->ops_union 
		    && 
		    (enduvp->ops_root->pst_sym.pst_value.
			pst_s_root.pst_union.pst_dups == PST_NODUPS);
		enduvp = enduvp->ops_union)
		;	    /* find end of the group of partitions
			    ** which require duplicates to be 
			    ** removed */
	    {
		for (removepp = insertpp;
		    *removepp != enduvp;
		    removepp = &(*removepp)->ops_next)
		    ;	    /* find subquery pointing
			    ** to end of sub view to be
			    ** removed */
	    }
	    *removepp = uvp->ops_next;  /* remove new union view
			    ** (subview) definition from query */
	    {
		/* define the new parse tree partition and replace/remove
		** the old parse tree partitions */
		PST_QNODE	    *irootp;	/* insertion point
			    ** of root node */
		OPV_IGVARS	    new_gvar;	/* global range table number
			    ** for newly created subview */

		irootp = enduvp->ops_root->pst_sym.pst_value.pst_s_root.
		    pst_union.pst_next; /* save ptr to remaining partitions */
		enduvp->ops_root->pst_sym.pst_value.pst_s_root.
			pst_union.pst_next = NULL; /* terminate set of
			    ** partitions with duplicates to be
			    ** removed */
		new_gvar = opa_cunion(global, &rootp, FALSE);  /* create union view 
			    ** definition based on the PST_NODUPS 
			    ** partitions */
		if (subquery != uvp)
		    uvp->ops_next->ops_root->pst_sym.pst_value.
			pst_s_root.pst_union.pst_next = rootp; /* set up
			    ** back link unless it is at the head of the
			    ** list */
		rootp->pst_sym.pst_value.pst_s_root.pst_dups = PST_NODUPS; /* remove
			    ** duplicates when reading this new union view partition */
		rootp->pst_sym.pst_value.pst_s_root.pst_union.pst_next = irootp;
		rootp->pst_sym.pst_value.pst_s_root.pst_union.pst_dups = PST_ALLDUPS;
			    /* keep duplicates within the context of the original
			    ** union view */
		(VOID)opv_parser(global, new_gvar, OPS_VIEW, 
		    FALSE,		    /* no RDF info */
		    FALSE,		    /* do not look in PSF range table */
		    TRUE);		    /* abort if an error occurs */
		gvarp = global->ops_rangetab.opv_base->opv_grv[new_gvar];
		uvp->ops_result =
		uvp->ops_gentry = new_gvar;
		opa_gsub(uvp);	    /* new head of union view */
	    }
	    new_subquery = opa_initsq( global, &rootp, subquery->ops_agg.opa_father); 
			    /* allocate and initialize a new subquery 
			    ** element which will replace the set of
			    ** partitions with duplicate removed */
		
	    gvarp->opv_gquery = uvp;
	    new_subquery->ops_mode = PSQ_RETRIEVE;
	    new_subquery->ops_agg.opa_graft = NULL;	/* just in case */
	    new_subquery->ops_next = *removepp;
	    *removepp = new_subquery; /* insert the new partition */
	    new_subquery->ops_result =
	    new_subquery->ops_gentry = old_var;
	    if (subquery == uvp)
	    {
		if (global->ops_qheader->pst_rangetab[old_var]->pst_rgroot 
		    ==
		    subquery->ops_root)
		    global->ops_qheader->pst_rangetab[old_var]->pst_rgroot
			= new_subquery->ops_root; /* change parse tree table
			    ** to be consistent with modified union view
			    ** - not really needed, but makes OP161 look
			    ** consistent*/
		global->ops_rangetab.opv_base->opv_grv[old_var]->opv_gquery 
		    = new_subquery;
		global->ops_rangetab.opv_base->opv_grv[old_var]->
		    opv_gsubselect->opv_subquery = new_subquery;
		new_subquery->ops_sqtype = OPS_VIEW;	/* if at head of list then
			    ** mark new view partition */
	    }
	    else
	    {
		new_subquery->ops_sqtype = OPS_UNION;
		new_subquery->ops_next->ops_union = new_subquery; /* otherwise
			    ** link into list of current unions */
	    }
	    uvp->ops_next = *insertpp;
	    *insertpp = enduvp; /* insert the new union view list */
	    new_subquery->ops_union = enduvp->ops_union;
	    enduvp->ops_union = NULL;
	    uvp->ops_sqtype = OPS_VIEW; /* mark this new union view as
			    ** head of the list */
	    opj_jeqc(new_subquery); /* first part of joinop processing */
	    global->ops_trace.opt_subquery = new_subquery; /* save in opt_subquery
			    ** so that format routine 
			    ** can find it using the session
			    ** control block path - used for
			    ** debugging and tracing only*/
	    new_subquery->ops_onext = uvp->ops_onext; 
	    uvp->ops_onext = new_subquery; /* place into list just
			    ** after view definition */
	    (VOID) opj_translate( new_subquery ); /* second part
			    ** of joinop processing
			    ** prepare the subquery for 
			    ** the enumeration phase
			    ** (equivalent to the old
			    ** "joinop" routine )
			    */
	    opj_outcopy(new_subquery, TRUE);/* save copies of union view
			    ** variable length joinop structures */
	    uvp = new_subquery;
	}
    }
#ifdef    OPT_F074_UVAGG
    if (
	    (subquery->ops_global->ops_cb->ops_check 
	    && 
	    opt_strace( subquery->ops_global->ops_cb, OPT_F074_UVAGG)
	    )
	)
	opj_uvagg(insertpp, terminatep); /* check if union view aggregate
			    ** optimizations can be performed */
#endif
    for (;*insertpp != terminatep;)
    {	/* process all partitions except head of union */
	opj_incopy(*insertpp, TRUE);
	insertpp = opj_enum(insertpp);
    }
    return (insertpp);
}

/*{
** Name: opj_partition	- search for partition compatible joins
**
** Description:
**      Search QEP for partition compatible joins (joins between 2
**	tables partitioned on their join columns with matching 
**	partitioning schemes). These joins can be performed as a 
**	sequence of small joins of their partitions rather than a
**	single very large join. Aside from being faster even as a 
**	serially executed plan, these joins can be executed in parallel.
**
**	We also search for partition compatible aggregations, aggs
**	which group by on a superset of partitioning columns.
**
** Inputs:
**      global				ptr to global state structure
**					which addresses subq structure 
**					containing QEP
**
** Outputs:
**      global->ops_subquery		subquery structures addressing QEPs
**					now adorned (as appropriate) with 
**					information guiding the code generation
**					for partition compatible joins
**	Returns:
**	    VOID
**	Exceptions:
**
**
** Side Effects:
**
**
** History:
**      14-apr-04 (inkdo01)
**	    Written (for partitioned table/parallel query processing support).
**	27-Aug-2007 (kschendel) b122118
**	    Looping on ops-union with var-subquery diving managed to miss
**	    subqueries that were preceded by an independent SAGG (e.g. the
**	    TPC-H query 22).  Revise the subquery looping to hit them all.
**	8-Jan-2010 (kschendel) b123122
**	    Subquery can show up multiple places, only process it once.
*/

/* First, the routine that does the analysis for a subquery, which might
** be a union subquery.  This is the bit that can be called recursively
** when analyzing a var orig that is actually a subquery.
*/

static void
opj_sq_partition(OPS_SUBQUERY *sq)
{
    OPO_CO		*bestco;
    OPV_PCJDESC		*wpcjdp;
    i4			wcount;
    bool		istable;

    do
    {
	bestco = sq->ops_bestco;
	if ((sq->ops_sunion.opu_mask & (OPU_NOROWS|OPU_PCJ_PROCESSED)) == 0
	  && bestco != NULL)
	{
	    sq->ops_sunion.opu_mask |= OPU_PCJ_PROCESSED;
	    wpcjdp = NULL;
	    wcount = 0;
	    opj_partanal(sq, bestco, &wpcjdp, &wcount, &istable);
	    /* If subquery is a hash aggregation, also check if we can do
	    ** partition-compatible aggregation.
	    */
	    if (sq->ops_sqtype == OPS_HFAGG)
		opj_partagg(sq, wpcjdp, wcount);
	}
	sq = sq->ops_union;
    } while (sq != NULL);
} /* opj_sq_partition */


/* The main driving checker just loops over the outer subquery list. */

static VOID
opj_partition(
	OPS_STATE	*global)
{
    OPS_SUBQUERY	*outer_sq;


    /* Loop over subqueries in the QEP performing the partitioned
    ** table analysis. Some subqueries are hidden inside range variables
    ** but will be found and analyzed by opj_partanal(). */
    for (outer_sq = global->ops_subquery; outer_sq != NULL; outer_sq = outer_sq->ops_next)
    {
	opj_sq_partition(outer_sq);
    }

}


/*{
** Name: opj_partanal	- Analyze a CO subtree for partition 
**	compatible joins
**
** Description:
**      Recursively analyze CO tree fragment in search of partition
**	compatible joins. Add partition info to ORIG and join nodes
**	as pc joins are found.
**
**	In addition to processing the CO subtree, if we find a
**	subquery hidden away inside some range variable, apply
**	PC-join analysis and PC-aggregation analysis to it too.
**
** Inputs:
**      sq				ptr to subquery structure
**					containing CO tree fragment under 
**					analysis
**	cop				ptr to root of CO tree fragment
**	pcjdpp				ptr to ptr to partition compatible
**					join description (if found)
**	istable				ptr to boolean, an output
**
** Outputs:
**      cop				ptr to root of CO tree fragment
**					updated with pc join info
**	pcjdpp				returns pc join definitions 
**					for partition compatible joins
**	istable				Returns TRUE if cop pointed to a
**					table, ie an ORIG.  Returns FALSE
**					for joins.  Irrelevant if CO tree
**					fragment is not PC-joinable, ie
**					*pcjdpp is returned null.
**
**	Returns:
**	    VOID
**	Exceptions:
**
**
** Side Effects:
**
**
** History:
**      14-apr-04 (inkdo01)
**	    Written (for partitioned table/parallel query processing support).
**	10-may-04 (inkdo01)
**	    Fix computation of opo_pgcount.
**	21-july-04 (inkdo01)
**	    Disable PC joins on Kjoins until we figure out how to do 'em.
**	4-dec-2006 (dougi)
**	    Fix stupid coding error that causes bug 117148 (mixed jn_var's
**	    with jn_dim's).
**	10-Apr-2007 (kschendel) SIR 122513
**	    Rework a bit to detect nested PC-joins.
**	16-Aug-2007 (kschendel) SIR 122513
**	    Pick the dimension with the most logical partitions, if
**	    more than one dimension matches (at the orig level).
**	    This is far from the best choice, but it's better than nothing.
**	    Alternatives that would be better, but I don't know how yet:
**	    - pick the dim that can be used by joins higher up as well;
**	    - pick a dim that does NOT have any elimination predicates.
**	25-Sep-2007 (kschendel) SIR 122513
**	    We're still missing pc-join opportunities in a union.
**	    Rework subquery looping a bit more.
*/

static VOID
opj_partanal(
	OPS_SUBQUERY	*sq,
	OPO_CO		*cop,
	OPV_PCJDESC	**pcjdpp,
	i4		*gcount,
	bool		*istable)

{
    OPS_SUBQUERY *ssq;
    OPV_VARS	*varp, *var1p, *var2p;
    OPV_IVARS	varno;
    OPV_PCJDESC	*lpcjdp = (OPV_PCJDESC *) NULL;
    OPV_PCJDESC	*rpcjdp = (OPV_PCJDESC *) NULL;
    DB_PART_DIM	*wdimp;
    i4		lcount, rcount, tcount;
    bool	ltable, rtable;
    bool	omatch;


    if (cop == (OPO_CO *) NULL)
    {
	*gcount = 0;
	*pcjdpp = (OPV_PCJDESC *) NULL;
	*istable = FALSE;
	return;
    }

    /* Stay in loop analyzing CO nodes until returned from. */
    while (TRUE)
    {
	lcount = rcount = 0;
	/* Check out the CO nodes by type. */
	switch (cop->opo_sjpr) {

	  case DB_RSORT:
	    /* These aren't of much use because they must
	    ** order the whole output of the subtree.
	    ** FIXME actually I don't think that is true;  if the
	    ** ordering matches the partitioning, we can set part-separate
	    ** on the topsort and do a "PC sort".   It probably isn't
	    ** common enough to worry about, but ultimately should be done.
	    */
	    opj_partanal(sq, cop->opo_outer, &lpcjdp, &lcount, &ltable);
				/* might find something below */
	    *pcjdpp = NULL;
	    *gcount = 0;
	    *istable = FALSE;
	    return;

	  case DB_PR:
	    /* Ignore this and loop to find (hopefully) an ORIG
	    ** node below. */
	    cop = cop->opo_outer;
	    break;

	  case DB_SJ:
	    /* Join logic is further broken down by join type. */
	    switch (cop->opo_variant.opo_local->opo_jtype) {
	      case OPO_SJFSM:
	      case OPO_SJHASH:
	      /* case OPO_SJKEY: */
		/* These are the joins we are interested in. Check
		** the outer ... */
		opj_partanal(sq, cop->opo_outer, &lpcjdp, &lcount, &ltable);
		rpcjdp = lpcjdp;
		omatch = (lpcjdp != NULL);

		/* Now check the inner.
		** If the outer was a table, pass the pcjdp from the outer
		** so that if the inner is a table, it can figure out
		** the proper partition compatibility.
		** If the outer was a join, don't pass anything, just
		** ask for inner's pcjdp list (if any) and we'll match
		** it up here.
		*/
		if (omatch && !ltable)
		    rpcjdp = NULL;
		opj_partanal(sq, cop->opo_inner, &rpcjdp, &rcount, &rtable);
		if (!omatch || rpcjdp == (OPV_PCJDESC *) NULL)
		    *pcjdpp = (OPV_PCJDESC *) NULL;
		else
		{
		    if (ltable && rtable)
		    {
			/* Table joins table with partition compatible
			** join. Record final match in outer ORIG. */
			*pcjdpp = rpcjdp;
			var1p = sq->ops_vars.opv_base->opv_rt[rpcjdp->jn_var1];
			var2p = sq->ops_vars.opv_base->opv_rt[rpcjdp->jn_var2];
			*gcount = var1p->opv_grv->opv_relation->rdr_parts->
				dimension[rpcjdp->jn_dim1].nparts;
			/* save smaller partition count in join */
			if ((tcount = var2p->opv_grv->opv_relation->
				rdr_parts->dimension[rpcjdp->jn_dim2].nparts)
				< *gcount)
			    *gcount = tcount;
			/* Hash joins can operate as the target of a higher
			** PC-join, but FSM joins can't.  (This is purely a
			** QEF implementation restriction that can be lifted,
			** if someone wants to figure out the mysteries of
			** the FSM join code.)
			*/
			if (cop->opo_variant.opo_local->opo_jtype != OPO_SJHASH)
			    *pcjdpp = NULL;
		    }
		    else
		    {
			/* Table joins join, or join joins join.
			** For t join j, search t's joinable list for
			** a match with either {v1,dim1} or {v2,dim2}
			** from the join pcjdesc.  For the join joins
			** join case, pick any table from the outer
			** and search that table's joinable list against
			** the inner join's pcjdesc.  Use the (inner)
			** joins pcjdesc as the result.
			**
			** Swap variables around so that rpcjdp is the join's.
			*/
			if (rtable)
			{
			    i4 tmp;
			    OPV_PCJDESC *tpcj;

			    tpcj = lpcjdp;
			    tmp = lcount;
			    lpcjdp = rpcjdp;
			    lcount = rcount;
			    rpcjdp = tpcj;
			    rcount = tmp;
			    ltable = TRUE;
			    rtable = FALSE;
			}
			else if (!ltable)
			{
			    /* Must be j join j, pick a table from the left */
			    varp = sq->ops_vars.opv_base->opv_rt[lpcjdp->jn_var1];
			    lpcjdp = varp->opv_pcjdp;
			}
			/* Starting with lpcjdp search for a match with
			** either of the join tables from rpcjdp.
			*/
			while (lpcjdp != NULL)
			{
			    if ((lpcjdp->jn_var2 == rpcjdp->jn_var1
				 && lpcjdp->jn_dim2 == rpcjdp->jn_dim1)
			      || (lpcjdp->jn_var2 == rpcjdp->jn_var2
                                 && lpcjdp->jn_dim2 == rpcjdp->jn_dim2))
				break;
			    lpcjdp = lpcjdp->jn_next;
			}
			if (lpcjdp == NULL)
			{
			    /* No match, this join isn't PC */
			    *pcjdpp = NULL;
			    *gcount = 0;
			}
			else
			{
			    *pcjdpp = rpcjdp;
			    if (ltable)
			    {
				/* partanal return from table can't set count
				** since it doesn't know which dim to use, so
				** if one side was a table get its count.
				*/
				var1p = sq->ops_vars.opv_base->opv_rt[lpcjdp->jn_var1];
				lcount = var1p->opv_grv->opv_relation->rdr_parts->
					dimension[lpcjdp->jn_dim1].nparts;
			    }
			    if (lcount < rcount)
				rcount = lcount;
			    *gcount = rcount;	/* The smaller of the two */
			}
		    }

		    /* Last step is to push group count out to ORIG nodes
		    ** and save in SJ node.  (Don't however push zeros).
		    */
		    if (*gcount > 0)
		    {
			opj_partpushg(cop->opo_outer, *pcjdpp, *gcount);
			opj_partpushg(cop->opo_inner, *pcjdpp, *gcount);
		    }
		    cop->opo_variant.opo_local->opo_pgcount = *gcount;
		    cop->opo_variant.opo_local->opo_pcjdp = *pcjdpp;
		    /* One more tweak, if this is an OUTER join, turn off
		    ** any possibility of doing PC-aggregation above this
		    ** join.  It won't work any more because of null
		    ** introduction, and nulls matter to aggregation.
		    ** (FIXME this is too pessimistic because nulls are
		    ** only introduced on the inner side, and the partitioning
		    ** and bylist eqc's might all come from the outer; but
		    ** partagg doesn't know how to do that analysis yet.)
		    ** 
		    */
		    if (cop->opo_union.opo_ojid != OPL_NOOUTER)
			sq->ops_mask |= OPS_NOPCAGG;
		}
		*istable = FALSE;
		return;

	      default:
		/* The others (ISjoin, Tjoin, SEjoin, CPjoin) derive no 
		** benefit from partition compatibility. Just analyze
		** their subtrees and return no match. */
		opj_partanal(sq, cop->opo_outer, &lpcjdp, &lcount, &ltable);
		opj_partanal(sq, cop->opo_inner, &rpcjdp, &rcount, &rtable);
		*pcjdpp = (OPV_PCJDESC *) NULL;
		*gcount = 0;
		*istable = FALSE;
		return;
	    }

	  case DB_ORIG:
	    /* These are the interesting guys. First look for var's
	    ** that are actually subquery's themselves, and recurse. */

	    *istable = TRUE;
	    cop->opo_variant.opo_local->opo_pcjdp = NULL; /* paranoia */
	    cop->opo_variant.opo_local->opo_pgcount = 0;
	    varno = cop->opo_union.opo_orig;
	    varp = sq->ops_vars.opv_base->opv_rt[varno];
	    if (varp->opv_subselect)
	    {
		ssq = sq->ops_global->ops_rangetab.opv_base->opv_grv[
			varp->opv_subselect->opv_sgvar]->
			opv_gsubselect->opv_subquery;
		opj_sq_partition(ssq);
		*pcjdpp = NULL;
		*gcount = 0;
		return;
	    }
	    /* Apply conditions to disqualify unsuitable tables. */
	    if (varp->opv_pcjdp == (OPV_PCJDESC *) NULL)
	    {
		*pcjdpp = NULL;
		*gcount = 0;
		return;
	    }

	    /* Search for partition compatibility of this table. */
	    if (*pcjdpp == (OPV_PCJDESC *)NULL)
		*pcjdpp = varp->opv_pcjdp;	/* return this var's
						** pc join descs */
	    else
	    {
		OPV_PCJDESC *best_jdp;
		i4 nparts, best_nparts;

		/* We have been passed a list of descriptors for another
		** var so this must be an inner table. Search the list
		** for an entry involving our table and return it.
		** If multiple dims match, take the one with the most
		** partitions, which hopefully will result in the smallest
		** joins.
		** (Note:  even better would be the dimension that is PCJ
		** compatible with higher joins in the tree, but at the moment
		** there is no good way to do that sort of global analysis.)
		*/
		best_nparts = 0;
		best_jdp = NULL;
		for (lpcjdp = *pcjdpp; lpcjdp; lpcjdp = lpcjdp->jn_next)
		{
		    if (lpcjdp->jn_var2 == varno)
		    {
			wdimp = &varp->opv_grv->opv_relation->rdr_parts->
					dimension[lpcjdp->jn_dim2];
			/* If we have a reduced estimate based on partition
			** elimination, use that, else use dim's logical nparts
			*/
			nparts = wdimp->nparts;
			if (varp->opv_pqbf != NULL)
			    nparts = varp->opv_pqbf->opb_pqlcount[lpcjdp->jn_dim2];
			if (nparts > best_nparts)
			{
			    best_nparts = nparts;
			    best_jdp = lpcjdp;
			}
		    }
		}
		*pcjdpp = best_jdp;	/* return this one (maybe NULL) */
		if (best_jdp != (OPV_PCJDESC *) NULL)
		{
		    /* Return inner's nparts for chosen dimension, caller
		    ** will resolve proper gcount for outer vs inner.
		    */
		    *gcount = best_nparts;
		}
	    }

	    return;

	}	/* end of CO node switch */
    }	/* end of TRUE loop */

} /* opj_partanal */

/*{
** Name: opj_partpushg	- push (possibly) updated group count for
**	partition compatible joins to the ORIG nodes.
**
** Description:
**      Recursively descend CO tree fragment to update group count
**	in partition compatible joins and ORIG nodes participating
**	in partition compatible joins.
**
**	Update the PC joinable descriptor too but for orig's only.
**	opj_partanal sets the descriptor for joins.
**
** Inputs:
**	cop				ptr to root of CO tree fragment
**	pcjdp				pcjdesc to place in orig
**	gcount				group count to place in join/orig
**
** Outputs:
**
**	Returns:
**	    VOID
**	Exceptions:
**
**
** Side Effects:
**
**
** History:
**      15-apr-04 (inkdo01)
**	    Written (for partitioned table/parallel query processing support).
**	10-may-04 (inkdo01)
**	    Forgot DB_PR on top of ORIG.
**	15-Nov-2006 (kschendel)
**	    let partpushg push the pcjdp for orig's as well.
**	10-Apr-2007 (kschendel) SIR 122513
**	    Turn on nested-pc-join flag as needed.
*/
static VOID
opj_partpushg(
	OPO_CO	*cop,
	OPV_PCJDESC *pcjdp,
	i4	gcount)
{

    /* Simply recursively descend looking for ORIG nodes to
    ** update with group count. It stops when it finds anything
    ** but a DB_PR node or a DB_SJ with a non-zero group count. All
    ** the rest effectively interrupt a partition compatible join. */

    switch (cop->opo_sjpr) {
      case DB_SJ:
	if (cop->opo_variant.opo_local->opo_pgcount == 0)
	    return;		/* not a PC join */
	cop->opo_variant.opo_local->opo_pgcount = gcount;
	cop->opo_variant.opo_local->opo_mask |= OPO_PCJ_NESTED;
	opj_partpushg(cop->opo_outer, pcjdp, gcount);
	opj_partpushg(cop->opo_inner, pcjdp, gcount);
				/* drop to ORIG to set gcount */
      case DB_ORIG:
	/* Always update the count, may have gotten smaller.
	** Only update the pcjdesc if it wasn't set yet.
	*/
	cop->opo_variant.opo_local->opo_pgcount = gcount;
	if (cop->opo_variant.opo_local->opo_pcjdp == NULL)
	    cop->opo_variant.opo_local->opo_pcjdp = pcjdp;
	return;			/* return from ORIG & SJ */

      case DB_PR:
	opj_partpushg(cop->opo_outer, pcjdp, gcount);
				/* and drop to return */

      default:			/* all the rest + SJ/ORIG */
	return;
    }

} /* opj_partpushg */

/*
** Name: opj_partagg - Analyze hash aggregation for partition compatibility
**
** Description:
**	When the input to a hash aggregation is partitioned, we have the
**	potential to do "partition compatible aggregation".  The idea
**	is that if the group-by list is equal to (or a superset of) the
**	partitioning column(s), we know that rows from different input
**	partitions must necessarily belong to different output groups.
**	Thus, instead of aggregating the whole mess at once, we can
**	aggregate partition by partition.
**
**	There are two cases where we can do a PC agg:
**
**	Case 1 occurs if we have an orig (actually a proj-rest with
**	an orig underneath) immediately below the aggregation.  In this
**	case, we need to dig out the partitioning info for the table being
**	read, and compare it to the group-by list.
**
**	Case 2 occurs if there is a PC-join immediately below the aggregation.
**	In this case, the caller has passed in the join result's PCJDESC
**	structure, and we can compare the group-by list against that.
**	This case is slightly easier to handle, since the PCJDESC contains
**	the dimension used for PC-joining.  (It also contains an eqc list
**	that we could compare directly to, but it's just as easy to use
**	the partition definition just like we do for case 1.)
**
**	If a PC aggregation is detected, for case 1 set the pgcount down in
**	the orig so that it reads partition-group-wise;  and set the PCAGG
**	flag in the subquery header for opc.  (This flag also suppresses
**	EXCH generation at the top of the subquery plan, since that would
**	lose the partitioning.  A 1:N EXCH could be generated ABOVE the
**	pcagg subquery, in theory.
**
**	No recursive analysis of the subquery is needed.  We'll be called
**	from PC-join analysis if any subquery vars happen to themselves
**	be subselects.
**
**	Note that all of this happens before EXCH placement.
**
** Inputs:
**	sq		Pointer to HFAGG subquery to analyze
**	pcjdp		OPV_PCJDESC pointer if subquery's CO plan ends in
**			a PC-join, or NULL if it doesn't
**	gcount		Group count from PC-join if pcjdp not null
**
** Outputs:
**	sq and its CO tree updated if PC aggregation possible
**
** History:
**	13-Mar-2007 (kschendel) SIR 122513
**	    Initial PC aggregation implementation.
**	13-Apr-2007 (kschendel)
**	    Extend to PC agg over PC join.
**	19-Jun-2007 (kschendel)
**	    Oops, un-initialized varno in the not-table case!  managed to
**	    work by accident most of the time.
**	13-Oct-2008 (kschendel)
**	    For agg over table, make sure orig sees a gcount of nparts,
**	    as we're aggregating (logical) partition by partition.  Variable
**	    mix-up caused bogus gcounts.  If the bad gcount got into an
**	    orig node, we could even skip partitions at the end (bad!).
*/

static void
opj_partagg(OPS_SUBQUERY *sq, OPV_PCJDESC *pcjdp, i4 gcount)
{
    bool isTable;
    DB_PART_DEF *partdef;		/* Table's partition definition */
    DB_PART_DIM *partdim;		/* Ptr to one dimension info */
    i4 bestdim, bestnparts;		/* Best dimension so far */
    i4 bestgcount;			/* Est # of logical partitions
					** for the best dimension so far;
					** for isTable only */
    i4 dim, j;
    i4 nparts;
    OPE_BMEQCLS bylist_map;		/* Equiv classes in BY-list */
    OPE_BMEQCLS dim_map;		/* Partitioning equiv classes */
    OPE_IEQCLS eqcno;			/* Equiv class number */
    OPO_CO *cop = sq->ops_bestco;
    OPV_IVARS varno;
    OPV_VARS *varp;
    OPZ_IATTS attno;			/* Joinop attr number */
    PST_QNODE *byexpr;			/* By-list resdom pointer */
    PST_QNODE *byvar;			/* By-list VAR pointer */

    /* If no CO tree (?), nothing to see here */
    if (cop == NULL)
	return;

    /* If outer join underneath, we can't count on the partitioning. */
    if (sq->ops_mask & OPS_NOPCAGG)
	return;

    /* Decide whether we have an orig (actually should be a DB_PR) or a
    ** PC-join underneath.  If the source looks partitioned, set up
    ** various things (varno, dim, etc etc) for the eqc analysis.
    */
    if (cop->opo_sjpr == DB_PR || cop->opo_sjpr == DB_ORIG)
    {
	/* Really shouldn't have a bare orig under an agg! expect the PR */
	if (cop->opo_sjpr == DB_PR)
	    cop = cop->opo_outer;	/* Get to the orig */

	/* Maybe it's a partitioned table */
	varno = cop->opo_union.opo_orig;
	varp = sq->ops_vars.opv_base->opv_rt[varno];
	if (varp->opv_subselect)
	    return;			/* Not if it's a subselect */
	partdef = varp->opv_grv->opv_relation->rdr_parts;
	if (partdef == NULL)
	    return;			/* Not partitioned */
	isTable = TRUE;
	dim = 0;			/* Start search at first dimension */
    }
    else
    {
	if (pcjdp == NULL || gcount == 0)
	    return;			/* No PC-join underneath */

	/* Pick either var of the join descriptor and analyze it
	** against the BY-list.  We can pick either side since it
	** must be an inner join; outer joins disable PC-aggregation
	** because of null introduction.
	*/
	varno = pcjdp->jn_var1;
	varp = sq->ops_vars.opv_base->opv_rt[varno];
	dim = pcjdp->jn_dim1;
	if (varp->opv_subselect)
	{
	    /* Insist on real var for ease of analysis */
	    varno = pcjdp->jn_var2;
	    varp = sq->ops_vars.opv_base->opv_rt[varno];
	    if (varp->opv_subselect)
		return;
	    dim = pcjdp->jn_dim2;
	}
	partdef = varp->opv_grv->opv_relation->rdr_parts;
	if (partdef == NULL)
	    return;			/* Huh? */
	isTable = FALSE;
    }

    /* There is partitioning underneath, so check further.
    ** Build a bit-map of the bylist eqclasses that are simple
    ** columns (not expressions).  (By-list order is of no importance.)
    ** Then, build a bitmap of the partitioning columns as eqclasses.
    ** (If some column isn't in an eqclass, obviously that dim fails.)
    ** This is done for each non-AUTOMATIC dimension if the source is
    ** just a table, or for the PC-joining dimension otherwise.
    ** If the partitioning bitmap is a subset of the bylist bitmap,
    ** we have a winner.   For multi-dimensional partitioning,
    ** try all the dimensions and pick the one with the most logical
    ** partitions, in hopes that that will provide the finest-grained
    ** input to the aggregation.
    ** FIXME even better would be to check for partition qualification
    ** and avoid qualified dimensions if there is an unqualified
    ** dimension available.  That is sort of gilding the lily though.
    */

    MEfill(sizeof(OPE_BMEQCLS), 0, (PTR) &bylist_map);
    for (byexpr = sq->ops_byexpr; 
	 byexpr != NULL && byexpr->pst_sym.pst_type == PST_RESDOM;
	 byexpr = byexpr->pst_left)
    {
	byvar = byexpr->pst_right;
	/* FIXME skipping all expressions is too harsh, but I don't know
	** how to detect bijective expressions (that preserve partitioning)
	*/
	if (byvar->pst_sym.pst_type != PST_VAR)
	    continue;
	attno = byvar->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
	eqcno = sq->ops_attrs.opz_base->opz_attnums[attno]
			->opz_equcls;	/* get eqcno for VAR */
	BTset(eqcno, (PTR)&bylist_map);
    }

    bestdim = -1;
    bestnparts = 0;
    /* "dim" is the required dimension if we're over a PC-join, or
    ** search all dims starting at zero if we're over a base table.
    ** (If over a join, this loop doesn't loop.)
    */
    do
    {
	partdim = &partdef->dimension[dim];
	if (partdim->distrule == DBDS_RULE_AUTOMATIC)
	    continue;		/* no partitioning column */
	MEfill(sizeof(OPE_BMEQCLS), 0, (PTR) &dim_map);
	for (j = 0; j < partdim->ncols; ++j)
	{
	    attno = opz_findatt(sq, varno,
			    partdim->part_list[j].att_number);
	    if (attno == OPZ_NOATTR)
		break;		/* Column isn't in the query */
	    eqcno = sq->ops_attrs.opz_base->opz_attnums[attno]
			    ->opz_equcls;
	    BTset(eqcno, (PTR)&dim_map);
	}
	if (j != partdim->ncols)
	    continue;		/* Skip dim with unusable column */
	if (BTsubset((char *)&dim_map, (char *) &bylist_map,
			    sq->ops_eclass.ope_ev) )
	{
	    /* Got a candidate, remember dim if it's best.  If the variable
	    ** has been analyzed for partition qualification, use the
	    ** estimate from the pqbf structure;  this is usually the same
	    ** as partdim->nparts, but might be less if pqbf thinks that
	    ** some partitions in that dim are restricted away.
	    */
	    nparts = partdim->nparts;
	    if (nparts > bestnparts)
	    {
		bestdim = dim;
		bestnparts = nparts;
		if (isTable)
		{
		    /* Record estimated number of logical partitions,
		    ** which is usually the dimension's total, but if
		    ** partition qualification has run and come up with
		    ** a better (smaller) number, use it instead.  This
		    ** bestgcount is ONLY used to adjust the aggregate
		    ** total-groups estimate so that runtime can better
		    ** allocate its data structures.
		    */
		    bestgcount = nparts;
		    if (varp->opv_pqbf != NULL)
			bestgcount = varp->opv_pqbf->opb_pqlcount[dim];
		}
	    }
	}
    } while (isTable && ++dim < partdef->ndims);

    if (bestdim == -1)
	return;

    /* Zowie, looks like we found one. */
    if (isTable)
    {
	/* In order to properly decorate the ORIG node so that opc
	** generates all the partition control crap, fake up a
	** OPV_PCJDESC with the proper stuff in it.  Just set
	** both vars/dims, and orig doesn't care about the eqcs.
	*/
	pcjdp = (OPV_PCJDESC *) opu_memory(sq->ops_global,
				sizeof(OPV_PCJDESC));
	pcjdp->jn_next = NULL;
	pcjdp->jn_var1 = pcjdp->jn_var2 = varno;
	pcjdp->jn_dim1 = pcjdp->jn_dim2 = bestdim;
	pcjdp->jn_eqcs[0] = OPE_NOEQCLS;
	opj_partpushg(cop, pcjdp, bestnparts);
	gcount = bestgcount;
    }
    else
    {
	/* For PCagg above join, no need to push (we're using all of the
	** info sent up by the join);  but do mark the top node, which
	** must have been a join, as nested.
	*/
	cop->opo_variant.opo_local->opo_mask |= OPO_PCJ_NESTED;
    }
    /* Flag for opc, and also so that we don't try to parallelize
    ** this subquery below the aggregation.
    ** Set aggregate gcount for result estimate adjustment.  i.e. runtime
    ** will do estgroups / gcount output groups per cycle.
    */
    sq->ops_mask |= OPS_PCAGG;
    sq->ops_gcount = gcount;

} /* opj_partagg */


/*{
** Name: opj_exchange	- add exchange nodes to QEP (if appropriate)
**
** Description:
**      Analyze QEP for suitability of parallel execution and add 
**	exchange nodes where they will be most beneficial in reducing
**	the elapsed time taken to execute query
**
** Inputs:
**      global				ptr to global state structure
**					which addresses subq structure 
**					containing QEP
**
** Outputs:
**      global->ops_subquery		subquery structures addressing QEPs
**					now adorned (as appropriate) with 
**					exchange nodes for parallel 
**					execution
**	Returns:
**	    VOID
**	Exceptions:
**
**
** Side Effects:
**
**
** History:
**      13-nov-03 (inkdo01)
**	    Written (for parallel query processing support).
**	3-mar-04 (inkdo01)
**	    Changes to support 1:n exchange nodes above PR nodes.
**	13-Mar-2007 (kschendel) SIR 122513
**	    Don't look for exch in partition compatible aggregations, as
**	    that would just screw up the PC-ness of the aggregate.
**	8-Jan-2010 (kschendel) b123122
**	    Make sure that a subquery is only processed once, lest
**	    duplicate exch requests be registered.  This can lead to
**	    a bad query plan that falls over at runtime.
*/
static VOID
opj_exchange(
	OPS_STATE	*global)
{
    OPS_SUBQUERY	*sq;
    OPO_COST		plancost;
    OPO_CO		*bestco;
    EXCHDESC		*exchp = NULL;
    i4			dop = global->ops_cb->ops_alter.ops_pq_dop;
					/* CBF degree of parallelism */
    f4			thres = global->ops_cb->ops_alter.ops_pq_threshold;
					/* CBF threshold cost of || plan */
    i4			exchpix = 0;


    /* Loop over plan fragments looking for one expensive enough to be
    ** eligible for parallelization. */
    if (dop <= 1)
	return;				/* but first see if || is enabled */

    /* Before analyzing for cost reductions, mark nodes that are not
    ** permitted to be parallelized because of DMF restrictions on
    ** update-type queries.
    */
    opj_exchupd_anal(global);

    /* This set of subqueries may not be right - further checking is
    ** required to make sure. For example, by this time in the process, 
    ** union queries are buried in range tables and other nesting. */
    for (sq = global->ops_subquery; sq; sq = sq->ops_next)
    {
	if (sq->ops_sunion.opu_mask & (OPU_NOROWS | OPU_EXCH_PROCESSED)
	  || sq->ops_mask & OPS_PCAGG)
	    continue;			/* empty or partition agg sq */
	sq->ops_sunion.opu_mask |= OPU_EXCH_PROCESSED;
	bestco = sq->ops_bestco;
	if (bestco == NULL || (plancost = opn_cost(sq, 
		bestco->opo_cost.opo_dio, 
		bestco->opo_cost.opo_cpu)) < thres)
	    continue;			/* estimate must be > threshold */

	/* We have a fragment worth checking. If exchange structure array
	** not yet allocated, do that first. */
	if (exchp == NULL)
	    exchp = (EXCHDESC *)opu_memory(global, 2*dop*sizeof(EXCHDESC));

	opj_excheval(sq, bestco, &sq->ops_bestco, exchp, &exchpix, 0);
    }

    /* If there are entries in the exchange descriptor array, add them
    ** to the QEP. */
    if (exchpix)
	opj_exchadd(global, exchp, exchpix);

}


/*
** Name: opj_exchupd_anal - mark plan nodes that mustn't be parallelized
**
** Description:
**	DMF was designed before parallel query, and some key parts of
**	DMF expect to do concurrency control based on locking.  For
**	instance, as of this writing (summer 2010), the btree code
**	expects at most one cursor (RCB) from a session will be in DMF at a
**	time;  so it does things like update all the other session RCB's
**	with position-is-valid information, and it does it without
**	any mutexing or concurrency control.  This fails when the
**	query is run as a parallel plan, with the being-updated
**	table (or one of its indexes) being fetched in a child thread
**	under an EXCH.
**
**	The cure (for now) is to analyze the proposed plan before we
**	compute parallel-query reductions, and mark those nodes which
**	must not be placed under an exch.  Any plan subtree which contains
**	an ORIG reading from the result table, or any of its indexes,
**	is marked do-not-parallelize.
**
**	An exception can be made for fragments which are run to completion
**	before any rows hit the top of the plan (the updating action header).
**	For example, given the simple plan:
**
**	      HJOIN
**             / \
**	    ORIG  ORIG
**
**	the left (build) orig could be parallelized, because it's run to
**	completion before any rows are emitted from the hash join and
**	thence to DMF.  (Of course, this same property makes that orig
**	very very undesirable for parallelizing, but there might be some
**	benefit if the ORIG were instead some more complex plan fragment.)
**	No great amount of effort is expended to find this sort of thing.
**
**	The analysis is run "in the usual manner", with a stub driver to
**	loop through the subquery and union lists, and a recursive
**	analyzer to examine nodes and deal with orig's which are in fact
**	subqueries themselves.
**
**	Eventually, one can hope that DMF is fixed up to be more aware of
**	parallel query possibilities, and when that happens, this code
**	can be dropped.
**
** Inputs:
**	global			Optimizer global state
**
** Outputs:
**	returns nothing
**
**	Non-parallelizable nodes are marked with opo_no_exch set.
**
** History:
**	10-Aug-2010 (kschendel) b124218
**	    Written.
*/

/* This is the worker routine that does the analysis and any subquery
** diving needed.
*/
static void
opj_exchupd_worker(OPS_SUBQUERY *sq, OPO_CO *cop, i4 result_baseid)
{

    OPO_CO *inner = cop->opo_inner;
    OPO_CO *outer = cop->opo_outer;

    switch (cop->opo_sjpr)
    {
    case DB_ORIG:
	{
	    OPV_VARS	*varp;
	    OPV_IVARS	varno;

	    varno = cop->opo_union.opo_orig;
	    varp = sq->ops_vars.opv_base->opv_rt[varno];
	    /* Recursively process var's subquery if var is a subselect */
	    if (varp->opv_subselect)
	    {
		OPS_SUBQUERY *ssq;

		ssq = sq->ops_global->ops_rangetab.opv_base->opv_grv[
				varp->opv_subselect->opv_sgvar]->
			opv_gsubselect->opv_subquery;
		if (ssq->ops_bestco != NULL)
		    opj_exchupd_worker(ssq, ssq->ops_bestco, result_baseid);
		return;
	    }
	    /* If this orig is the table being updated, or one of its
	    ** secondary indexes, it mustn't be parallelized.
	    */
	    if (varp->opv_grv->opv_relation
	      && varp->opv_grv->opv_relation->rdr_rel->tbl_id.db_tab_base == result_baseid)
		cop->opo_no_exch = 1;
	    return;		/* Nothing underneath */
	}

    case DB_RSORT:
	/* Top sort consumes its input before emitting output */
	outer = NULL;
	break;

    case DB_SJ:
	/* Various joins.  Don't bother analyzing the left (build) side
	** of a hash join, it's run to completion before any rows are
	** emitted.  (unless it's a partition compatible join.)
	** A similar test could be done for merge joins with actually-
	** sorted inputs (not implicitly sorted), but I can't be bothered
	** at the moment (kschendel).
	** It goes without saying that this analysis is being run after
	** the opj pass that assigns real join types...
	*/
	if (cop->opo_variant.opo_local->opo_jtype == OPO_SJHASH
	  && cop->opo_variant.opo_local->opo_pgcount == 0)
	    outer = NULL;	/* Don't analyze build input */
	break;

    default:
	break;

    } /* switch */

    /* Recurse to analyze left and right inputs.  (Above code may have
    ** decided that one or the other is a don't-care.)
    ** If either subtree comes back "no exch", this node must also be "no exch".
    */
    if (outer != NULL)
    {
	opj_exchupd_worker(sq, outer, result_baseid);
	if (outer->opo_no_exch)
	{
	    cop->opo_no_exch = 1;
	}
    }
    if (inner != NULL)
    {
	opj_exchupd_worker(sq, inner, result_baseid);
	if (inner->opo_no_exch)
	    cop->opo_no_exch = 1;
    }
} /* opj_exchupd_worker */



/* And here is the actual analysis driver */
static void
opj_exchupd_anal(OPS_STATE *global)
{
    i4 result_baseid;
    i4 mode;
    OPS_SUBQUERY *outer_sq;
    OPS_SUBQUERY *sq;

    /* Before doing any of this, see if we need to analyze.  Only
    ** worry about delete, insert, update queries.  Retrieve and retinto
    ** are fine.
    */
    mode = global->ops_qheader->pst_mode;
    if (mode != PSQ_DELETE && mode != PSQ_REPLACE && mode != PSQ_APPEND)
	return;
    result_baseid = global->ops_qheader->pst_restab.pst_restabid.db_tab_base;
    for (outer_sq = global->ops_subquery;
	 outer_sq != NULL;
	 outer_sq = outer_sq->ops_next)
    {
	sq = outer_sq;
	do
	{
	    if (sq->ops_bestco != NULL)
		opj_exchupd_worker(sq, sq->ops_bestco, result_baseid);
	    sq = sq->ops_union;
	} while (sq != NULL);
    }
} /* opj_exchupd_anal */

/*{
** Name: opj_excheval	- analyze nodes of a plan fragment for parallel
**	excecution suitablility
**
** Description:
**      Examines nodes of a plan fragment determining the estimated
**	reductions in execution times if exchange nodes were placed
**	at various places in the plan
**
** Inputs:
**      subquery			ptr to subquery defining plan
**					fragment under consideration
**	cop				ptr to current CO-tree node
**					under examination
**	pcopp				ptr to ptr to parent of cop
**	exchp				ptr to array of exchange node 
**					descriptors
**	exchpixp			ptr to index of current entry
**					exchange descriptor array
**	level				Depth in the CO tree (top = 0)
**
** Outputs:
**      exchp				exchange descriptor array with 
**					information about various 
**					prospective exchange nodes
**	Returns:
**	    TRUE if cop is a proj-rest node, else FALSE
**	Exceptions:
**
**
** Side Effects:
**
**
** History:
**      13-nov-03 (inkdo01)
**	    Written (for parallel query processing support).
**	2-mar-04 (inkdo01)
**	    Add in 1:n exchanges for partitioned tables.
**	5-mar-04 (inkdo01)
**	    Add logic to pursue UNIONed selects.
**	23-apr-04 (inkdo01)
**	    Add 1:n exchanges for partition compatible joins.
**	12-aug-04 (inkdo01)
**	    Decrement *exchpixp after it exceeds its limit.
**	19-jan-05 (inkdo01)
**	    Add support for CBF parm for max threads of partitioned
**	    table/join for parallel query.
**	23-May-2005 (schka24)
**	    Fix minor goof that prevented full-array replacement from working.
**	    Track CO tree depth for exch candidates.
**	23-Jun-2005 (schka24)
**	    Last edit could cause looping, clarify loop structure to fix.
**	    Insist on at least some trivial reduction for any exch (the
**	    threshold parameter applies to the plan, not to individual
**	    exch nodes).
**	24-Aug-2007 (kschendel) SIR 122513
**	    Use pqbf estimate for number of partitions selected.
**	    Fool with cost reductions a little, e.g. discourage exch
**	    on hash join build side since it's a collection point.
**	10-Aug-2010 (kschendel) b124216
**	    Obey no-exchange flags set by prior analysis pass.
**	    Fine-tune calculations so that we don't put exch under
**	    pre-sorted K/T join (the sort wastes most of the exch effort),
**	    nor on the inner of a PSM join (inner reset would throw away
**	    what the exch has prefetched).
*/

#define MINEXCH 50.0		/* Minimum reduction for any exch */

static bool
opj_excheval(
	OPS_SUBQUERY	*sq,
	OPO_CO		*cop,
	OPO_CO		**pcopp,
	EXCHDESC	*exchp,
	i4		*exchpixp,
	i4		level)

{
    OPV_VARS	*varp;
    OPB_PQBF	*pqbfp;
    OPO_CO	*inner, *outer;
    OPO_COST	costr, costo, costi;	/* cost estimates for node result,
					** outer and inner subplans */
    OPO_COST	tro = 0.0, tri = 0.0;	/* time reduction estimates for
					** node outer and inner subplans */
    OPO_COST	tr, smallest, ureduction;
    i4		dop = sq->ops_global->ops_cb->ops_alter.ops_pq_dop;
					/* CBF degree of parallelism */
    i4		pthreads = sq->ops_global->ops_cb->ops_alter.ops_pq_partthreads;
    i4		i, j, ucount;
    i4		nparts, threads;
    i4		pass;
    bool	pr = FALSE;
    bool	pro, pri;
    bool	punion = FALSE;
    bool	outer_noexch = FALSE, inner_noexch = FALSE;


    inner = cop->opo_inner;
    outer = cop->opo_outer;

    /* Time reduction estimates from the introduction of exchange nodes
    ** are made from bottom (leaf nodes) to top of the query tree. Start
    ** by recursively descending tree and compute reductions on the way
    ** back up.
    **
    ** NOTE: ORIG nodes have no time estimates - they're built into the
    ** PR, KJOIN or TJOIN nodes above them. So we only descend to the
    ** nodes above the leaf (ORIG) nodes. Actually, we simply don't 
    ** analyze the inner sides of KJOINs and TJOINs. We also don't 
    ** descend through partition compatible joins - exchange nodes are
    ** placed above them, not below. */

    if (outer != NULL && outer->opo_sjpr != DB_ORIG &&
	(cop->opo_sjpr != DB_SJ || 
		cop->opo_variant.opo_local->opo_pgcount == 0))
    {
	pro = opj_excheval(sq, outer, &cop->opo_outer, 
						exchp, exchpixp, level+1);

	if (inner != NULL && inner->opo_sjpr != DB_ORIG &&
	    cop->opo_variant.opo_local->opo_jtype != OPO_SJKEY &&
	    cop->opo_variant.opo_local->opo_jtype != OPO_SJTID)
	    pri = opj_excheval(sq, inner, &cop->opo_inner, 
						exchp, exchpixp, level+1);
    }
    else if (cop->opo_sjpr == DB_PR && outer && outer->opo_sjpr == DB_ORIG)
	pr = TRUE;
    else if (cop->opo_sjpr == DB_SJ)
	pr = FALSE;
    else return(FALSE);		/* this shouldn't happen */

    /* Now do the analysis - compute costs of current node compared 
    ** with subtrees. Then the impact of an exchange node between them
    ** can be assessed. */

    costi = costo = 0.0;
    if (outer != NULL)
    {
	if (!pr)
	    costo = opn_cost(sq, outer->opo_cost.opo_dio, outer->opo_cost.opo_cpu);
	outer_noexch = (outer->opo_no_exch != 0);
    }
    if (inner != NULL)
    {
	costi = opn_cost(sq, inner->opo_cost.opo_dio, inner->opo_cost.opo_cpu);
	inner_noexch = (inner->opo_no_exch != 0);
    }
    costr = opn_cost(sq, cop->opo_cost.opo_dio, cop->opo_cost.opo_cpu);
    costr = costr - costo - costi;	/* cost of executing this node */
    threads = 1;

    /* Estimate the minimum cost reduction of adding an exchange node
    ** below the current node. */
    switch (cop->opo_sjpr) {
      case DB_SJ:
	if (cop->opo_variant.opo_local->opo_pgcount > 0)
	{
	    if (! cop->opo_no_exch)
	    {
		/* Partition compatible join - no need to recurse
		** further. Propose 1:n exchange node here. */
		threads = pthreads;
		if (cop->opo_variant.opo_local->opo_pgcount < pthreads)
		    threads = cop->opo_variant.opo_local->opo_pgcount;
		tro = (((float)threads - 1.0) / (float)threads) * costr * 0.7;
				/* these should save lots! */
	    }
	    break;
	}

	/* Else, different computations depending on join type. */
	switch (cop->opo_variant.opo_local->opo_jtype) {
	  case OPO_SJKEY:
	  case OPO_SJTID:
	    /* Key and TID joins effectively only have one input -
	    ** the outer source. So compute the reduction from placing
	    ** exchange between the join and the outer source nodes.
	    ** Don't put an exchange below this join if there's a pre-sort,
	    ** though;  the sort is a collection point and you can't flow
	    ** rows through it effectively.
	    */
	    if (! outer_noexch && ! cop->opo_osort)
	    {
		tro = (costr > costo) ? costo : costr;
		tro *= 0.3;	/* estimated reduction is 30% of lesser
				** of input time and join time */
	    }
	    if (pro)
		return(FALSE);	/* No reduction from K/T-join over orig */
	    break;

	  case OPO_SJCARTPROD:
	  case OPO_SJFSM:
	  case OPO_SJPSM:
	  case OPO_SJHASH:
	    tro = (costr > costo) ? costo : costr;
	    tri = (costr > costi) ? costi : costr;
	    /* Minimal reduction if hash build side, or if we have to sort. */
	    if (cop->opo_variant.opo_local->opo_jtype == OPO_SJHASH
	      || cop->opo_osort)
		tro *= 0.10;
	    else if (pro)
		tro *= 0.01;	/* Generally avoid exch over orig */
	    else
		tro *= 0.40;	/* else use 40% of lesser time */
	    if (cop->opo_variant.opo_local->opo_jtype != OPO_SJPSM)
	    {
		if (cop->opo_isort)
		    tri *= 0.10;  /* Minimal reduction if have to sort */
		else if (pri)
		    tri *= 0.01;  /* Avoid if feed from orig */
		else
		    tri *= 0.50;  /* est inner reduction is 50% of lesser
				** of inner input time and join time */
	    }
	    else
		tri = 0.0;	/* No reduction on inner of PSM, reset of
				** inner will waste exch work
				*/
	    /* No reduction if not allowed */
	    if (outer_noexch)
		tro = 0.0;
	    if (inner_noexch)
		tri = 0.0;
	    break;
	}
	break;

      case DB_PR:
	varp = sq->ops_vars.opv_base->opv_rt[outer->opo_union.opo_orig];
				/* OPV_VARS for ORIG node table */
	if (varp->opv_subselect)
	{
	    opj_exchunion(sq->ops_global, exchp, varp, exchpixp, level+1,
						&ucount, &ureduction);
	    if (ucount == 0 || outer_noexch)
		return(pr);
	    tro = ureduction;
	    threads = ucount;
	    punion = TRUE;
	    break;
	}
	else if (!varp->opv_grv->opv_relation || 
		!varp->opv_grv->opv_relation->rdr_parts || outer_noexch)
	    return(pr);		/* not partitioned - we're not interested */

	nparts = varp->opv_grv->opv_relation->rdr_parts->nphys_parts;

	pqbfp = varp->opv_pqbf;
	if (pqbfp != NULL)
	    nparts = pqbfp->opb_pqcount;
	if (nparts >= pthreads)
	    threads = pthreads;
	else
	    threads = nparts;
	if (threads <= 1)
	    return(pr);

	tro = (((float)threads - 1) / (float)threads) * costr * 0.8;
				/* reduction assumption is that threads
				** will scale, less 20% overhead */
	break;

      default:
	/* Probably a DB_RSORT */
	/* Do nothing immediately below a top sort, wouldn't help
	** anyway because the sort can't run until it has all of its
	** input.
	*/
	break;

    }	/* end of opo_sjpr switch */

    /* Add entries to exchange descriptor array (where appropriate). */
    if (tri <= MINEXCH && tro <= MINEXCH)
	return(pr);

    tr = tro;
    for (pass = 1;  pass >= 0;  tr = tri, --pass)
    {
	/* Loop once for outer, then once for inner. */
	if (tr <= MINEXCH)
	    continue;			/* Loop if no reduction this side */

	/* If there's room in the array, add another entry. Otherwise,
	** replace the smallest reduction unless our reduction is
	** smallest of all.
	*/
	smallest = 0.0;
	if ((i = (*exchpixp)++) >= 2*dop)
	{
	    (*exchpixp)--;
	    smallest = FMAX;
	    i = 0;			/* Paranoia */
	    for (j = 0; j < 2*dop; j++)
		if (exchp[j].exch_reduction < smallest)
		{
		    smallest = exchp[j].exch_reduction;
		    i = j;		/* Replace this one, so far */
		}
	}
	if (tr <= smallest)
	    continue;			/* tr smaller than smallest, loop */

	/* "i" now indexes the entry we're inserting to. */
	exchp[i].exch_subqp = sq;
	exchp[i].exch_parent = pcopp;
	exchp[i].exch_reduction = tr;
	exchp[i].exch_count = threads;
	exchp[i].exch_level = level;
	exchp[i].exch_outer = (pass != 0);
	exchp[i].exch_pr = pr;
	exchp[i].exch_un = punion;
    }
    return(pr);

}


/*{
** Name: opj_exchunion	- recurse on union selects in main query
**
** Description:
**      Traverses union structure in query plan, calling opj_excheval 
**	to place exchange nodes into the union selects. Also determines
**	the value in placing exchange above the unioned selects.
**
** Inputs:
**      global				ptr to global state structure
**	varp				ptr to RT entry for union selects
**	exchp				ptr to array of exchange node 
**					descriptors
**	exchpixp			ptr to current EXCHDESC array index
**	level				depth in the CO tree (top = 0)
**	ucount				ptr to count of union'ed selects
**	ureduction			ptr to estimated cost reduction 
**					from exchange palced above union
**
** Outputs:
**      global->ops_subquery		subquery structures addressing QEPs
**					now adorned (as appropriate) with 
**					exchange nodes for parallel 
**					execution
**	Returns:
**	    VOID
**	Exceptions:
**
**
** Side Effects:
**
**
** History:
**	5-mar-04 (inkdo01)
**	    Written to process unioned selects for || queries.
**	11-june-04 (inkdo01)
**	    Fix improper calc. of 2nd costliest select.
**	25-May-2005 (schka24)
**	    Track CO tree depth.
**	13-Mar-2007 (kschendel) SIR 122513
**	    No exch's in partition compatible agg subquery.
**	8-Jan-2010 (kschendel) b123122
**	    Don't reprocess an already processed subquery.
*/
static VOID
opj_exchunion(
	OPS_STATE	*global,
	EXCHDESC	*exchp,
	OPV_VARS	*varp,
	i4		*exchpixp,
	i4		level,
	i4		*ucount,
	OPO_COST	*ureduction)

{
    OPS_SUBQUERY	*sq;
    OPO_CO		*bestco;
    OPO_COST		plancost, maxcost, max2cost;
    OPO_COST		thres = global->ops_cb->ops_alter.ops_pq_threshold;
					/* CBF threshold cost of || plan */

    sq = global->ops_rangetab.opv_base->opv_grv[
	varp->opv_subselect->opv_sgvar]->opv_gsubselect->opv_subquery;
					/* OPS_SUBQUERY corresponding to varp */
    maxcost = max2cost = 0.0;
    for ((*ucount) = 0; sq; sq = sq->ops_union, (*ucount)++)
    {
	/* ***** FIXME?? In a perfect world, we would be able to put an
	** exch over the QP that runs a partition compatible hash agg,
	** with each thread running different partition groups (just like
	** an exch driven PC-join).  If that ever happens, we would want
	** to deal with the PC-agg, not ignore it.
	*/
	if (sq->ops_sunion.opu_mask & (OPU_NOROWS | OPU_EXCH_PROCESSED)
	  || sq->ops_mask & OPS_PCAGG
	  || sq->ops_bestco == NULL)
	    continue;			/* empty or PC-agg sq */
	sq->ops_sunion.opu_mask |= OPU_EXCH_PROCESSED;
	bestco = sq->ops_bestco;
	plancost = opn_cost(sq, bestco->opo_cost.opo_dio,
					bestco->opo_cost.opo_cpu);
	if (plancost > maxcost)
	{
	    max2cost = maxcost;
	    maxcost = plancost;
	}
	else if (plancost > max2cost)
	    max2cost = plancost;
	if (plancost < thres)
	    continue;			/* estimate must be > threshold */

	/* We have a fragment worth checking. If exchange structure array
	** not yet allocated, do that first. */

	opj_excheval(sq, bestco, &sq->ops_bestco, exchp, exchpixp, level);
    }

    /* max2cost is the 2nd most expensive of the union'ed selects. It
    ** becomes the basis for the cost reduction of parallel execution
    ** of the unions. */
    if (max2cost < thres)
    {
	/* No point in bothering with the exchange node. */
	*ucount = 0;
	*ureduction = 0.0;
    }
    else
    {
	*ureduction = max2cost * 0.9;
    }
}


/*{
** Name: opj_exchadd	- add exchange nodes according to contents of
**	exchange descriptor array
**
** Description:
**      Orders exchange descriptor array by estimated execution time
**	savings and inserts exchange nodes into query plan
**
**	Exch nodes are added from the bottom up.  They have to be
**	added in a consistent order.  Otherwise, one exch addition could
**	put an exch where a later exch addition expects to see the
**	original CO node, causing segv's or other problems.
**
** Inputs:
**      global				ptr to global state structure
**	exchp				ptr to array of exchange node 
**					descriptors
**	exchcount			count of exchange descriptors
**
** Outputs:
**      global->ops_subquery		subquery structures addressing QEPs
**					now adorned (as appropriate) with 
**					exchange nodes for parallel 
**					execution
**	Returns:
**	    VOID
**	Exceptions:
**
**
** Side Effects:
**
**
** History:
**      13-nov-03 (inkdo01)
**	    Written (for parallel query processing support).
**	2-mar-04 (inkdo01)
**	    Add in 1:n exchanges for partitioned tables.
**	23-apr-04 (inkdo01)
**	    Add 1:n exchanges for partition compatible joins.
**	20-may-04 (inkdo01)
**	    Synchronize partition compatible join group and thread counts.
**	9-june-04 (inkdo01)
**	    Un-synchronize PCJ group from threads - QEF can manage it.
**	22-july-04 (inkdo01)
**	    Add support for CO opo_punion flag.
**	30-oct-04 (inkdo01)
**	    Add count parm to opn_cpmemory() call.
**	25-May-2005 (schka24)
**	    Insert from the bottom up, and avoid placing an exch over an
**	    exch.
[@history_template@]...
*/
static VOID
opj_exchadd(
	OPS_STATE	*global,
	EXCHDESC	*exchp,
	i4		exchcount)

{
    OPS_SUBQUERY	*sq;
    EXCHDESC		tempex, *wexchp;
    OPO_PERM		*permp;
    OPO_CO		**copp;
    i4			dop = global->ops_cb->ops_alter.ops_pq_dop;
					/* CBF degree of parallelism */
    i4			threadcount;
    i4			i, j, k;


    /* Bubble sort the exchange descriptors into descending sequence 
    ** on exch_reduction. */
    for (i = 0, threadcount = 0; i < exchcount; i++)
	threadcount += exchp[i].exch_count;

    if (threadcount > dop)
    {
	/* Keep number of exchange nodes <= requested degree of 
	** parallelism. This is done by sorting them on estimated
	** time reduction and discarding the least effective.
	*/
	for (i = 0; i < exchcount; i++)
	 for (j = i+1; j < exchcount; j++)
	  if (exchp[i].exch_reduction < exchp[j].exch_reduction)
	  {		/* swap 'em */
	    STRUCT_ASSIGN_MACRO(exchp[i], tempex);
	    STRUCT_ASSIGN_MACRO(exchp[j], exchp[i]);
	    STRUCT_ASSIGN_MACRO(tempex, exchp[j]);
	  }

	/* Pick first "k" nodes whose added threads total <= dop. */
	/* **** FIXME this is excessively primitive (as is the initial
	** **** loop above) because it doesn't allow for collection points
	** in the tree, where threads below must have all completed.
	** (e.g. sorts, maybe hash-join builds.)
	*/
	for (k = 0, j = 0; j <= dop && k < exchcount; 
				j+= exchp[k++].exch_count);

    }
    else k = exchcount;

    /* (Re-) sort on decreasing tree depth, so that we add from
    ** the bottom up.
    */
    for (i = 0; i < k; i++)
	for (j = i+1; j < k; j++)
	    if (exchp[i].exch_level < exchp[j].exch_level)
	    {		/* swap 'em */
		STRUCT_ASSIGN_MACRO(exchp[i], tempex);
		STRUCT_ASSIGN_MACRO(exchp[j], exchp[i]);
		STRUCT_ASSIGN_MACRO(tempex, exchp[j]);
	    }

    /* Take first "dop" entries and introduce exchange nodes. */
    for (i = 0; i < k; i++)
    {
	wexchp = &exchp[i];
	sq = wexchp->exch_subqp;
	copp = wexchp->exch_parent;
	/* Make sure that we aren't sticking an exch above an exch,
	** can happen if lower node does a PC join and upper one
	** decides on an inner/outer kid exch.
	** NOTE: 1:n exchange nodes are placed above CO nodes. 1:1
	** exchanges are placed below the CO node.
	*/
	if (wexchp->exch_count == 1)
	{
	    if ((wexchp->exch_outer && (*copp)->opo_outer->opo_sjpr == DB_EXCH)
	      || (!wexchp->exch_outer && (*copp)->opo_inner->opo_sjpr == DB_EXCH) )
		continue;
	}
	if ((*copp)->opo_storage == DB_SORT_STORE &&
	    (*copp)->opo_ordeqc > OPE_NOEQCLS)
	    continue;		/* no exchange if results are sorted */

	/* Copy subnode into "exchange", then set to DB_EXCH. */
	permp = opn_cpmemory(sq, 1);
	if (wexchp->exch_count > 1)
	{
	    /* The following code segment reduces the number of PC join
	    ** join groupings to match the number of threads available. 
	    ** QEF is capable of managing threaded joins with several
	    ** groups per thread so this isn't necessary. The code 
	    ** fragment is left in the event that reducing the join 
	    ** groupings turns out to be a good approach. */
	    if (FALSE && (*copp)->opo_variant.opo_local->opo_pgcount > 
			exchp->exch_count)
	    {
		/* Not enough threads for number of PCJ groups -
		** change group size to match threads. */
		for (j = exchp->exch_count; j > 1; j--)
		    if ((*copp)->opo_variant.opo_local->
						opo_pgcount % j == 0)
			break;
		if (j <= 1)
		    continue;	/* if no common denominator, skip it */

		opj_partpushg(*copp, NULL, j);	/* propagate new gcount */
		exchp->exch_count = j;
	    }

	    STRUCT_ASSIGN_MACRO(*(*copp), permp->opo_conode);
	    permp->opo_conode.opo_outer = *copp;
	    *copp = &permp->opo_conode;
	}
	else if (wexchp->exch_outer)
	{
	    STRUCT_ASSIGN_MACRO(*((*copp)->opo_outer), permp->opo_conode);
	    permp->opo_conode.opo_outer = (*copp)->opo_outer;
	    (*copp)->opo_outer = &permp->opo_conode;
	    permp->opo_conode.opo_psort = (*copp)->opo_osort;
	}
	else 
	{
	    STRUCT_ASSIGN_MACRO(*((*copp)->opo_inner), permp->opo_conode);
	    permp->opo_conode.opo_outer = (*copp)->opo_inner;
	    (*copp)->opo_inner = &permp->opo_conode;
	    permp->opo_conode.opo_psort = (*copp)->opo_isort;
	}
	STRUCT_ASSIGN_MACRO(*(permp->opo_conode.opo_maps),permp->opo_mapcopy);
	permp->opo_conode.opo_inner = (OPO_CO *) NULL;
	permp->opo_conode.opo_maps = &permp->opo_mapcopy;
	permp->opo_conode.opo_storage = DB_HEAP_STORE;
	permp->opo_conode.opo_cost.opo_reduction = wexchp->exch_reduction;
	permp->opo_conode.opo_ccount = wexchp->exch_count;
	permp->opo_conode.opo_sjpr = DB_EXCH;
	permp->opo_conode.opo_outer->opo_psort = FALSE;
	permp->opo_conode.opo_punion = (wexchp->exch_un == TRUE);

	/* Set flag to generate parallel aggregation. */
	if (copp == &sq->ops_bestco && !(permp->opo_conode.opo_punion) &&
	    wexchp->exch_count > 1 &&
	    (sq->ops_sqtype == OPS_SAGG || sq->ops_sqtype == OPS_HFAGG ||
	     sq->ops_sqtype == OPS_RFAGG))
	    permp->opo_conode.opo_exchagg = TRUE;
	else permp->opo_conode.opo_exchagg = FALSE;
    }

}


/*
** Name: OPJ_PARTVARS - Generate partition qualification request maps
**
** Description:
**	Each CO node has a bitmap that indicates which variables
**	at that node and below might be interested in partition
**	qualification at join time.  This routine walks the subquery
**	lists, walks each subquery CO tree from the bottom up, and
**	sets the opo_partvars bitmaps to indicate which variables might
**	be interested in partition qualification from joins above that
**	CO node.
**
**	The rules are very simple:
**	For orig, if it's a partitioned table with partition qualification
**	possibilities as indicated by the existence of an opv_pqbf
**	structure for the variable, set the var's bit in the bitmap.
**	(Note that this excludes qualification possibilities from an
**	orig that is really a subquery.  We don't propagate join qual
**	across a QP in the final query plan.)
**
**	For P/R and sort, just copy the outer's bitmap along.
**
**	For EXCH, FIXME FIXME zero out the bitmap.  Join-time partition
**	pruning is not allowed across an exch, for a couple reasons:
**	a) we currently have no way to open a child thread without
**	starting it running right away, before the bitmaps are ready;
**	b) joins above the (1:n) exch wouldn't know how to update all
**	of the (n) child bitmap copies.
**	For now, parallel query trumps join-time partition qual.
**	
**
**	For joins, we return the OR of the two inputs, unless the
**	join is an outer join.  Outer joins introduce nulls, which
**	will screw things up;  a higher qualification predicate would
**	have to allow for the null with an IS NULL or equivalent, and
**	to the partition qualifier IS NULL looks like "= NULL", certainly
**	not the partition restriction we want.  So until the semantics
**	can be analyzed in depth, the partitioned-var map from
**	the inner side of an outer join is tossed.
**
** Inputs:
**	global			Optimizer state CB
**
** Outputs:
**	none
**
** Side effects:
**	Turns on relevant bits in opo_partvars bitmaps.
**
** History:
**	27-Aug-2007 (kschendel) SIR 122513
**	    Move partvars pass from opc to opj, is a little cleaner.
**	8-Jan-2008 (kschendel)
**	    K/T-joins are now in the game.
**	19-May-2010 (kschendel) b123759
**	    Comment update only re parallel query issues.
*/

/* First, the routine that actually processes a CO node */

static void
opj_partvarco(OPS_SUBQUERY *sq, OPO_CO *cop)
{

    i4 nvars = sq->ops_vars.opv_rv;
    OPO_CO *inner = cop->opo_inner;
    OPO_CO *outer = cop->opo_outer;
    OPV_BMVARS *ppartvars = &cop->opo_variant.opo_local->opo_partvars;

    if (outer != NULL)
	opj_partvarco(sq, outer);
    if (inner != NULL)
	opj_partvarco(sq, inner);

    switch (cop->opo_sjpr)
    {
    case DB_ORIG:
	{
	    OPV_VARS	*varp;
	    OPV_IVARS	varno;

	    varno = cop->opo_union.opo_orig;
	    varp = sq->ops_vars.opv_base->opv_rt[varno];
	    /* Recursively process var's subquery if var is a subselect */
	    if (varp->opv_subselect)
	    {
		OPS_SUBQUERY *ssq;

		ssq = sq->ops_global->ops_rangetab.opv_base->opv_grv[
				varp->opv_subselect->opv_sgvar]->
			opv_gsubselect->opv_subquery;
		if (ssq->ops_bestco != NULL)
		    opj_partvarco(ssq, ssq->ops_bestco);
		return;
	    }
	    /* Indicate that this variable might be interested in join-time
	    ** partition qualification, if there's pqbf info around.
	    ** Bitmap is initially cleared by maordering.
	    */
	    if (varp->opv_pqbf != NULL)
		BTset(varno, (char *) ppartvars);
	    break;
	}

    case DB_PR:
    case DB_RSORT:
	/* Propagate outer (orig or sort source) on up */
	MEcopy( (PTR) &outer->opo_variant.opo_local->opo_partvars,
		sizeof(OPV_BMVARS), (PTR) ppartvars);
	break;

    case DB_EXCH:
	/* Suppress join qual across parallelizing point, until the
	** QEF issues can be figured out (see intro).
	*/
	MEfill(sizeof(OPV_BMVARS), 0, (PTR) ppartvars);
	break;

    case DB_SJ:
	/* Various joins, OR the maps or apply outer-join rules. */
	if (cop->opo_variant.opo_local->opo_ojid == OPL_NOOUTER)
	{
	    MEcopy( (PTR) &outer->opo_variant.opo_local->opo_partvars,
		sizeof(OPV_BMVARS), (PTR) ppartvars);
	    BTor(nvars, (char *) &inner->opo_variant.opo_local->opo_partvars,
		(char *) ppartvars);
	}
	else
	{
	    switch (cop->opo_variant.opo_local->opo_type)
	    {
		case OPL_RIGHTJOIN:
		    MEcopy( (PTR) &inner->opo_variant.opo_local->opo_partvars,
			sizeof(OPV_BMVARS), (PTR) ppartvars);
		    break;
		case OPL_LEFTJOIN:
		    MEcopy( (PTR) &outer->opo_variant.opo_local->opo_partvars,
			sizeof(OPV_BMVARS), (PTR) ppartvars);
		    break;
		case OPL_FULLJOIN:
		    MEfill(sizeof(OPV_BMVARS), 0, (PTR) ppartvars);
		    break;
		default:
		    opx_error( E_OP039A_OJ_TYPE );
		    break;
	    }
	}
	break;

    default:
	opx_error(E_OP0495_CO_TYPE);

    } /* switch */
} /* opj_partvarco */


/* The driver loops through subqueries and unions */
static void
opj_partvars(OPS_STATE *global)
{
    OPS_SUBQUERY *outer_sq;
    OPS_SUBQUERY *sq;

    for (outer_sq = global->ops_subquery;
	 outer_sq != NULL;
	 outer_sq = outer_sq->ops_next)
    {
	sq = outer_sq;
	do
	{
	    if (sq->ops_bestco != NULL)
		opj_partvarco(sq, sq->ops_bestco);
	    sq = sq->ops_union;
	} while (sq != NULL);
    }
} /* opj_partvars */

/*{
** Name: opj_joinop - entry point to access path selection phase
**
** Description:
**      This procedure is the entry point to the phase which transforms the
**      query tree into a form useful for the access path selection
**	(enumeration) phase of the optimization.
**
**	The aggregate processing phase is completed and this phase can
**      perform any necessary tranformations on the query tree without
**      being concerned with affecting earlier phases of the optimization.
**
**	The input structure should contain a list of aggregate free sub-queries
**      which are ordered according to the expected evaluation.
**
** Inputs:
**      global                          ptr to global state variable
**	    .ops_subquery		ptr to linked list of aggregate free
**                                      subqueries (using .ops_next as
**                                      a link to the next subquery)
**
** Outputs:
**      global
**          .ops_subquery               ptr to a list of aggregate free
**                                      subqueries which have been analyzed
**                                      and a best co tree found
**              .ops_bestco             best co tree
**              .ops_cost               cost of best co tree
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	1-mar-86 (seputis)
**          initial creation
**	11-aug-88 (seputis)
**          moved cartprod call for 0 eqcls from opn_enum to here to fix
**	    set qep\g select count(*) from s\g problem
**	18-aug-88 (seputis)
**          -add distributed info
**	28-nov-88 (seputis)
**          -need to call cart prod routine in all multi-variable query cases
**	7-dec-89 (seputis)
**	    - fix op180 trace flag bug
**	26-feb-91 (seputis)
**	    - improved diagnostics for b35862
**	12-apr-93 (ed)
**	    - b49821 - incorrect partition elimination since NO_MATCH clauses in
**	    outer join do not necessarily eliminate all tuples
**	10-sep-04 (inkdo01)
**	    Remove op162 as hash toggle, now equivalent to
**	    SET PARALLEL
**	3-feb-06 (inkdo01)
**	    Implement query level hints.
**	10-july-06 (dougi)
**	    Remove op162 toggle on parallel queries.
**	19-Aug-2009 (kibro01) b122509
**	    Add flag to trigger sc930 tracing of query plans
**	27-Aug-2007 (kschendel) SIR 122513
**	    Set join-time partition pruning request bitmaps before opc.
**      26-apr-2010 (huazh01)
**          Disable parallel query processing on Ingres Star. (b123637)
[@history_line@]...
*/
VOID
opj_joinop(
	OPS_STATE          *global)
{
    OPS_SUBQUERY        **subqpp;	    /* ptr to ptr needed to remove
                                            ** subselects from execution list */
    OPS_CB		*opscb = ops_getcb();

    global->ops_osubquery = global->ops_subquery;

    {	/* traverse all the subqueries to determine the equivalence
	** classes, and use transitivity to find boolean factors which can
	** be copied to child queries */
	OPS_SUBQUERY	    *next1_subquery;
	for (next1_subquery = global->ops_subquery;
	    next1_subquery; next1_subquery = next1_subquery->ops_next)
	{
	    opj_jeqc(next1_subquery);
	}
    }

    for ( subqpp = &global->ops_subquery;   /* get beginning of subquery list */
	*subqpp;                          /* if not NULL then process the 
                                            ** subquery */ 
        )
    {
	OPS_SUBQUERY        *subquery;	    /* used to traverse the list of 
					    ** subqueries */
	bool		    norows;	    /* TRUE if no rows can be returned
					    ** from the subquery */
	do
	{   /* loop to eliminate union view partitions if possible */
	    subquery =
	    global->ops_trace.opt_subquery = *subqpp; /* save in opt_subquery
						** so that format routine 
						** can find it using the session
						** control block path - used for
						** debugging and tracing only*/
	    subquery->ops_onext = subquery->ops_next; /* save original link to next 
						** subquery to be processed */
	    norows = !opj_translate( subquery ); /* prepare the subquery for 
						** the enumeration phase
						** (equivalent to the old
						** "joinop" routine )
						*/
	    if (norows)
		opj_uveliminate(subqpp);	/* eliminate partition from union 
						** view since no rows can be
						** returned from this partition */
	    else if (subquery->ops_sqtype == OPS_UNION)
	    {
		/* processing of union views is delayed in order to determine
		** if enough partitions have been eliminated that a sort node(s)
		** within the remaining partitions is sufficient to satisfy the
		** union view semantics */
		opj_outcopy(subquery, TRUE);	/* save copies of union view
						** variable length joinop structures */
		if (subquery->ops_sqtype == OPS_UNION)
		    subqpp = &(*subqpp)->ops_next;
						/* delay processing a union view
						** with "mixed" duplicate handling
						** i.e. OPS_SUBVIEW set
						** until the head of the list of
						** the union view has been reached */
	    }
	} while (subquery->ops_sqtype == OPS_UNION);

	if (subquery->ops_sqtype == OPS_VIEW)
	    subqpp = opj_uvsubview(subquery, norows);	/* create a sub view for every
					    ** occurrence of a pst_union.pst_dups
					    ** == PST_NODUPS, and process the
					    ** partitions of the union view */
	else if (!norows)		    /* if the final partition of union view
					    ** returns no rows then do not optimize
					    ** this subquery since it has not been set
					    ** up yet */
	    subqpp = opj_enum(subqpp);	    /* find best plan and 
					    ** ptr to next subquery to optimize */
    }

    /* Call partition analyzer to break up partitioned table
    ** joins (if possible) and prepare for parallelization of QEP. */
    if (global->ops_gmask & OPS_PARTITION)
	opj_partition(global);

    /* Call exchange node analyzer to parallelize the query. */
    if (((global->ops_cb->ops_override & PST_HINT_PARALLEL) ||
	global->ops_cb->ops_alter.ops_parallel) &&
        !(global->ops_cb->ops_smask & OPS_MDISTRIBUTED) &&
	!(global->ops_cb->ops_override & PST_HINT_NOPARALLEL))
        opj_exchange(global);

    /* Propagate partition-qualification possibilities from the bottom to
    ** the top of the CO tree.
    */
    if (global->ops_gmask & OPS_PARTITION)
	opj_partvars(global);

    opscb->sc930_trace = NULL;
    if (((global->ops_cb->ops_override & PST_HINT_QEP) ||
	global->ops_cb->ops_alter.ops_qep
	||
	    (global->ops_cb->ops_check 
	    && 
	    opt_strace( global->ops_cb, OPT_FCOTREE)
	    )) &&
	!(global->ops_cb->ops_override & PST_HINT_NOQEP))
    {	/* print out query plans after all have been printed since
	** the site selection has not been determined until all
	** QEPs have been found */
	OPS_SUBQUERY	    *next_subquery;
	for (next_subquery = global->ops_osubquery;
	    next_subquery; next_subquery = next_subquery->ops_onext)
	{
	    if (next_subquery->ops_sunion.opu_mask & OPU_NOROWS)
		continue;
	    global->ops_trace.opt_subquery = next_subquery;
	    if ((global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
		&&
		next_subquery->ops_dist.opd_bestco)
	    {
		OPD_ISITE	target;
		target = next_subquery->ops_dist.opd_target;
		if (target >= 0)
		    next_subquery->ops_bestco = next_subquery->ops_dist.opd_bestco->
			opd_colist[target];
		else
		    opx_error( E_OP0A81_SITE);	/* site has not been selected
					** for the query plan */
	    }
	    opt_cotree( next_subquery->ops_bestco ); /* print CO if required 
					** - subquery context is found
					** using opt_subquery which is
					** set at beginning of this proc
					*/
	    if ( (global->ops_cb->ops_check && 
		opt_strace( global->ops_cb, OPT_F082_QEPDUMP))) 
					opt_qep_dump(global, next_subquery);
					/* spiffy new QEP dumper */
	}
    }
    if (ult_trace_qep())
    {
	OPS_SUBQUERY	    *next_subquery;
	opscb->sc930_trace = ult_open_tracefile((PTR)opscb->ops_sid);
	if (opscb->sc930_trace)
	{
	  for (next_subquery = global->ops_osubquery;
	    next_subquery; next_subquery = next_subquery->ops_onext)
	  {
	    if (next_subquery->ops_sunion.opu_mask & OPU_NOROWS)
		continue;
	    global->ops_trace.opt_subquery = next_subquery;
	    if ((global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
		&&
		next_subquery->ops_dist.opd_bestco)
	    {
		OPD_ISITE	target;
		target = next_subquery->ops_dist.opd_target;
		if (target >= 0)
		    next_subquery->ops_bestco = next_subquery->ops_dist.opd_bestco->
			opd_colist[target];
		else
		    opx_error( E_OP0A81_SITE);	/* site has not been selected
					** for the query plan */
	    }
	    opt_cotree( next_subquery->ops_bestco ); /* print CO if required */
	  }
	  ult_close_tracefile(opscb->sc930_trace);
	  opscb->sc930_trace = NULL;
	}
    }
}
