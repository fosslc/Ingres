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
#include    <dmccb.h>
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
#define        OPN_PRRLEAF      TRUE
#include    <bt.h>
#include    <opxlint.h>

/**
**
**  Name: OPNPRLEAF.C - routines for project-restrict-reformat for leafs
**
**  Description:
**      routines that do a project restrict, reformat for leafs
**
**  History:    
**      26-may-86 (seputis)    
**	    initial creation
**      21-may-93 (ed)
**          - changed opo_var to be a union structure so that
**          the outer join ID can be stored in interior CO nodes
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
{@history_line@}...
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      12-oct-2000 (stial01)
**          opn_prleaf() get keys per page info from dmf instead of calculating
**      16-jan-2003 (stial01)
**          opn_prleaf() fix cost calculations for distributed tables (b109467)
**/

/*{
** Name: opn_adfsf	- new adjacent duplicate factor and sort factor
**
** Description:
**      (blocks/pblocks) is the selectivity of predicate from those blocks 
**      read.  Multiply this times the original adjacent duplicate factor
**      (opo_adfactor) gives us an estimate of the new opo_adfactor.
**
**      Suppose we assume the number of sorted runs stays the same after
**      a restriction.  There were pblocks/opo_sortfact runs in those blocks 
**      read.  So the new sortedness factor is 
**      blocks/(pblocks/opo_sortfact) = blocks*opo_sortfact/pblocks.
**
** Inputs:
**      cp                              ptr to original CO node
**      pblocks                         number of blocks from the original
**                                      relation which must be read
**      blocks                          number of blocks in the 
**                                      relation after restrict project
**      tuples                          estimated number of tuples in "cp"
**
** Outputs:
**      newcp                           ptr to new CO node
**         .opo_adfactor                new adjacent duplicate factor
**         .opo_sortfact                new sortedness factor
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-may-86 (seputis)
**          initial creation
{@history_line@}
[@history_line@]...
*/
static VOID
opn_adfsf(
	OPO_CO             *ncp,
	OPO_CO             *cp,
	OPO_BLOCKS	   blocks,
	OPO_BLOCKS         pblocks,
	OPO_TUPLES         tuples)
{
    if (pblocks < 1.0)
	    pblocks = 1.0;

    /* calculate new adjacent duplicate factor */
    ncp->opo_cost.opo_adfactor = cp->opo_cost.opo_adfactor * blocks / pblocks;
    if ( ncp->opo_cost.opo_adfactor < 1.0 )
	ncp->opo_cost.opo_adfactor = 1.0;

    /* calculate new sortedness factor */
    if ((ncp->opo_cost.opo_sortfact = 
	    cp->opo_cost.opo_sortfact * blocks / pblocks) > tuples )
        ncp->opo_cost.opo_sortfact = tuples;

    if (ncp->opo_cost.opo_sortfact < 2.0 )
        ncp->opo_cost.opo_sortfact = 2.0;
}


/*{
** Name: opn_prleaf	- project restrict for leaf
**
** Description:
**
**      This routine generates a CO-tree forest for a single leaf.  The
**      forest will contain a tree with just an ORIG in it, and another
**      tree with PR + ORIG.  (unless the caller requests that the latter
**      be omitted, i.e. wants keyed strategies only.)  The bare ORIG
**      will typically be used for K-join or T-join plans, while the
**      PR/ORIG subtree can be used to cost out simple-join plans.
**
**      [the below apparently dates from the Jurassic age, and it's not
**      entirely clear to me how much of it is accurate or useful...]
**      
**       General:
**      
**       an index is useful if it is hash (and single attribute) or isam and
**	       a) it matches a joining clause
**          or b) it matches a boolean factor
**      
**       To match a joining clause-
**      	attribute of index must equal join attribute. if isam, the
**      	attribute must be the most significant. if hash, the rel must
**      	be keyed on only this attribute.
**      
**       To match a boolean factor-
**      	there must exist a boolean factor such that
**      	a) for hash, all preds are equality sargs on the attribute or
**      	b) for isam, all preds are sargs on the most significant 
**      	   attribute.  We do not worry about sort because it can 
**                 only exist as an intermediate relation and hence will 
**                 never need a project-restrict (since it would have 
**		   been done when the tuple was created).
**      
**       Operation of project-restrict when relation is isam or hash:
**      
**       a) hash: find a matching boolean factor (all preds are equality sargs)
**      	   then for each sarg hash it's value and for all elements 
**                 in the resulting linked list apply the whole predicate
**      
**       b) isam: if there is a boolean factor where all preds are equality 
**      	   sargs then process similar to hash. the only difference 
**      	   is that a directory search is performed instead of 
**      	   hashing to determine the beginning and ending of the 
**                 linked list.
**      
**      	otherwise, for all matching boolean factors find a max and 
**      		a min value as follows:
**      	. if all preds in this boolean factor are <, <= or = then set
**      	  mx to max of the constants. if mx is greater then Gmax, set
**      	  Gmax to mx.
**      	. if all preds in this bf are >, >= or = then set mn to min of 
**      	  the constants. if mn is greater than Gmin set Gmin to mn.
**      	. do a directory search with Gmin and Gmax.
**      	. for all elements in this linked list apply the whole 
**                predicate.
**      
**      
**       COST
**      
**       cost of project-restrict-reformat is composed of the
**      	project-restrict cost and
**      	reformat cost.
**      
**       for the reformat cost see procedure "costreformat.c"
**      
**       project restrict cost is:
**      	hash and matching boolean factor
**		    ((#_pgs)/(#_primary) + hashcost) * (#_sargs_in_bf)
**      			hashcost is usually 0.
**      	isam and matching boolean factor
**		    ((#_pgs)/(#_primary) + dirsearchcost) * (#_sargs_in_bf)
**      	isam and matching boolean factor with range (at least one
**      	inequality sarg)
**		    2 * (dirsearchcost) + integrate that part of hist to find
**      			that portion of relation which must be accessed
**      
**      
**       SPECIFIC
**      
**       operation of prr-
**      
**       We are given a list of co's. this list contains one element if we are
**       working on a leaf. if we are working on the result of a join then
**       there may be zero or more co's. (there could be zero if for every co 
**       generated by calcost, there was a cheaper one in an associated co
**       list with the same opo_storage and ordeqc.)
**      
**       let stp refer to this node in the tree
**       if stp is a leaf then there is only one co. it represents the original
**       relation as it is stored on disk. there can be no associated
**       co lists because for a leaf there is only one structure and relation
**       assignment possible. so for this stp's co, do simple pr and
**       reformat to isam, hash and sort on all interesting orders.
**       the reformat co's point to the pr co which points to the original co
**       (the couter pointer does the pointing).
**      
**       If stp is the output of a join then the procedure is more complex.
**       If there are no co's then nothing is done, so return
**       otherwise find the minimum cost co in stp's co list.
**       Find the minimum cost from all the other associated co's, if any.
**       if there are other co's and a minimum cost is found which is less
**       than stp's minimum cost then do nothing, return
**       if there are other co's and a they have a higher mincost, then
**       delete all reformat co's from them cause they can be done cheaper here
**      	specifically:
**       using the minimum cost co as a base, do reformats to isam, hash
**       and sort on all interesting orders. for each structure/ordering 
**       check this stp and all associated co lists.  If this co is the best use
**       it and delete the other with same struct/ord. if this is not the
**       cheapest then do not use it, try next struct/ord combo.
**
**	 Default case will be to not do reformat to isam or hash unless the 
**       trace flags are turned on.  Also, do not reformat to isam or hash 
**       if the tuple is too wide (>MAXTUP) cause modify cannot handle it.
** 
**	 ctups is multiplied by sel so QEP will print correctly and 
**       set_qep_lock will know how many result tuples there are so it can 
**       set result relation locking appropriately.
**
**	 Always create PR nodes so that the cost of their use can be evaluated.
**       This fixes a bug where cross products would become excessively large 
**       because the restriction on the outer node would only happen
**	 after the join.  In the query "ret (r.a)where s.a =5"
**	 s.a will be the outer. We should do the restriction before the 
**       cross product.
**
**       There will be no eqclasses in eqsp->eqcmp at this point if
**       this query is from a single variable integrity constraint (right
**       now all integrity constraints are single variable). This is because
**       integrity constraints have no target list because they just want to
**       see if there exists at least one element the query and do not want
**       to return any attributes. We could do a
**       special case in the optimizer to find the fastest way to get the
**       first tuple (this could also be useful for QBF) but this is not done
**       now.
**
** Note on trace points op188/op215/op216.
**	Trace points op188/op215/op216 and op145 (set qep) both use opt_cotree().
**	op188/op215/op216 expect opt_conode to point to the fragment being
**	traced, and op145 expects it to be NULL (so that ops_bestco is used).
**	The NULL/non-NULL value of opt_conode also results in different
**	display behaviour. For these reasons opt_conode is also NULLed
**	after the call to opt_cotree().
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      jsbtp                           set of cost orderings already calculated
**          .opn_coforw                 co tree representing leaf
**      eqsp                            equivalence class list header structure
**      rlsp                            represents the relation being restricted
**                                      and projected
**      blocks                          estimated number of blocks in relation
**                                      represented by this leaf after restrict
**                                      project applied
**      selectivity                     selectivity of predicates on relation
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
**	28-may-86 (seputis)
**          initial creation
**	19-sep-88 (seputis)
**          fix double application of selectivity
**	17-nov-88 (seputis)
**          non-zero PR cpu cost required
**	29-mar-90 (seputis)
**	    fix metaphor performance problem - be smarter about search space
**	20-dec-90 (seputis)
**	    add top sort node removal checks
**	22-apr-91 (seputis)
**	    - fix float exception handling problem b36920
**      15-dec-91 (seputis)
**          fix b33551 make better guess for initial set of relations to enumerate
**	26-dec-91 (seputis)
**	    - 41526 - initialize sortrequired since opn_checksort now needs this
**	    initialized as part of bug fix
**	28-jan-91 (seputis)
**	    - fix 40923 - remove parameter to opn_prcost
**	1-apr-92 (seputis)
**	    - b40687 - part of tid join estimate fix.
**      8-may-92 (seputis)
**          - b44212, check for overflow in opo_pointercnt
**      5-jan-93 (ed)
**          - bug 48049 - move definition of ordering to make it more localized
**	30-mar-94 (ed)
**	    - bug 59461 - correct estimate of index pages in fanout causing
**	    overestimates and potential overflows
**	06-mar-96 (nanpr01)
**          - Changes for variable page size. Do not use DB_MAXTUP constant
**	      rather use the opn_maxreclen function to calculate the 
**	      max tuple size given the page size.
**	06-may-96 (nanpr01)
**	    Changes for New Page Format project - account for tuple header.
**	17-jun-96 (inkdo01)
**	    Treat Rtree like Btree for relprim computation.
**	21-jan-1997 (nanpr01)
**	    opo_pagesize was not getting set and consequently in opn_prcost/
**	    opn_readahead was getting floating point exception in VMS.
**	 1-apr-99 (hayke02)
**	    Use tbl_id.db_tab_index and tbl_storage_type rather than
**	    tbl_dpage_count <= 1 to check for a btree or rtree index. As of
**	    1.x, dmt_show() returns a tbl_dpage_count of 1 for single page
**	    tables, rather than 2 in 6.x. This change fixes bug 93436.
**	17-aug-99 (inkdo01)
**	    Minor, but ugly, change to detect descending top sort where 
**	    first column of multi-attr key struct is also in constant BF. 
**	    read backwards doesn't work in this case.
**	28-sep-00 (inkdo01)
**	    Code for composite histograms (avoid using them when attr no 
**	    is expected in histogram)
**	13-nov-00 (inkdo01)
**	    Added CO-tree addr to opn_checksort call. Seems no reason to
**	    omit it and new unique index top sort removal heuristic (100482)
**	    requires it.
**      12-aug-2002 (huazh01)
**          Do not mark ncop->opo_storage as sorted on a hash structure base
**          table. This fixes bug 107351, INGSRV 1727 
**      26-aug-2003 (wanfr01)
**          Bug 111062, INGSRV 2545
**          Do not mark storage sorted unless it is a btree.
**	5-july-05 (inkdo01)
**	    Call opn_modcost() to alter row count for agg queries.
**	30-Jun-2006 (kschendel)
**	    Remove modcost, done now after subquery enumerated so that we
**	    don't affect the agg-input co tree.
**      30-Aug-2006 (kschendel)
**          Eliminate redundant opo_topsort bool.
**	31-Aug-2006 (kschendel)
**	    Watch for HFAGG as well as RFAGG.
**	22-Oct-2007 (kibro01) b119313
**	    Ensure that pr_tuples does not fall below 1.
**      7-Mar-2008 (kschendel)
**          Always generate PR/ORIG co tree if new enumeration, regardless
**          of plan ops_cost considerations.  Failure to do this can cause
**          a new enum pass to not find any valid plans until it clears the
**          subtree cache with opn-recover.  See inline.
**	04-nov-08 (hayke02 for kschendel)
**	    Modify the previous change to prevent single var substitution index
**	    QEPs always being chosen, despite the base table QEP being cheaper.
**	    This change fixes bug 121051.
**	3-Jun-2009 (kschendel) b122118
**	    OJ filter cleanup, fix here.
**	26-feb-10 (smeke01) b123349 b123350
**	    Copy over the op255 and op188 trace point code from opn_corput().
**	    Amend opn_prleaf() to return OPN_STATUS in order to make trace point
**	    op255 work with set joinop notimeout.
**	01-mar-10 (smeke01) b123333 b123373
**	    Improve op188. Replace call to uld_prtree() with call to
**	    opt_cotree() so that the same formatting code is used in op188 as
**	    in op145 (set qep). Save away the current fragment number and
**	    the best fragment number so far for use in opt_cotree(). Also
**	    set ops_trace.opt_subquery to make sure we are tracing the current
**	    subquery.
**	13-may-10 (smeke01) b123727
**	    Add new trace point op215 to output trace for all query plan
**	    fragments and best plans considered by the optimizer. Add a new
**	    trace point op216 that prints out only the best plans as they are
**	    Also cleaned up some confused variable usage: replaced dsortcop
**	    with sortcop; and removed usage of ncop after it has been assigned
**	    to cop, replacing it with cop.
**	01-jun-10 (smeke01) b123838
**	    Prevent oph_origtuples/oph_odefined from being set for the
**	    histograms of a multi-attribute key when there are no boolean
**	    factors for the key.  This in turn prevents the
**	    'delayed multi-attribute' code in oph_jtuples() from being skewed
**	    by an unreasonably high number for oph_origtuples (up to 50% of
**	    the number of rows in the base table).
*/
OPN_STATUS
opn_prleaf(
	OPS_SUBQUERY       *subquery,
	OPN_SUBTREE        *jsbtp,
	OPN_EQS            *eqsp,
	OPN_RLS            *rlsp,
	OPO_BLOCKS	   blocks,
	bool		   selapplied,
	OPN_PERCENT        selectivity,
	OPV_VARS           *varp,
	bool               keyed_only)
{
    OPS_STATE		*global;
    OPO_BLOCKS          dio;        /* estimated number of disk block
                                    ** reads/writes to produce tuples
                                    ** represented by this cost ordering
                                    ** - after the project restrict */
    OPO_CPU             cpu;        /* estimate of cpu cost to produce
                                    ** this cost ordering */
    OPO_TUPLES		pr_tuples;  /* number of tuples in the project
				    ** restrict */
    OPB_SARG            sargtype;   /* type of keyed access which can be
                                    ** used on relation */
    OPO_BLOCKS		pblk;       /* estimated number of blocks read including
                                    ** blocks in primary index */
    OPH_HISTOGRAM       *hp;        /* histogram returned only if TID access */
    OPO_CO              *cop;       /* ptr to cost-ordering representing
                                    ** the leaf node */
    OPO_CO              *ncop;      /* ptr to new cost-ordering of a project
                                    ** - restrict of "*cop" */
    bool		pt_valid;   /* TRUE if pblk can be used as an estimate
				    ** for pages touched, which is used for
				    ** lock mode calculations */
    bool		imtid;      /* TRUE if implicit TID instead of
                                    ** keyed attributes were used */
    bool		distributed; /* TRUE is this is a distributed
				    ** thread */
    OPO_COST		prcost;	    /* project restrict cost of node */
    i4 		pagesize;   /* estimate the page size     */ 
    OPO_CO		*sortcop = NULL;	/* set to not NULL if a sort node is required */
    bool 		op188 = FALSE;
    bool 		op215 = FALSE;
    bool 		op216 = FALSE;

    global = subquery->ops_global;

    if (global->ops_cb->ops_check)
    {
	if (opt_strace( global->ops_cb, OPT_F060_QEP))
	    op188 = TRUE;
	if (opt_strace( global->ops_cb, OPT_F087_ALLFRAGS))
	    op215 = TRUE;
	if (opt_strace( global->ops_cb, OPT_F088_BESTPLANS))
	    op216 = TRUE;
    }

    distributed = global->ops_cb->ops_smask & OPS_MDISTRIBUTED;
    cop = jsbtp->opn_coback = jsbtp->opn_coforw = opn_cmemory( subquery ); /* 
				    ** get a new CO node */
    STRUCT_ASSIGN_MACRO(*(varp->opv_tcop), *(jsbtp->opn_coback));
				    /* copy the node since it may be 
				    ** changed */
    jsbtp->opn_coback->opo_coforw = jsbtp->opn_coback->opo_coback = 
	(OPO_CO *) jsbtp;		    /* ptr to jsbtp used to terminate list */
    jsbtp->opn_coback->opo_maps = &eqsp->opn_maps; /* save a ptr to the 
				    ** equivalence classes supplied by this
				    ** CO subtree (i.e. node) */
    if (varp->opv_grv->opv_relation)
    {
	DMT_TBL_ENTRY          *dmfptr;	    /* ptr to DMF table info */

	dmfptr = varp->opv_grv->opv_relation->rdr_rel; /* get ptr to RDF 
					** relation description info */
	pagesize = dmfptr->tbl_pgsize;

	/*
	** Note if distributed, the DMT_TBL_ENTRY is not totally initialized
	** (it was filled in by RDF rdd_alterdate -- which did 
	** 'select ... from iitables...' against the local database
	** However the information available by querying iitables does
	** not include all information put in the DMT_TBL_ENTRY by DMT_SHOW
	*/
	if (distributed &&
		((dmfptr->tbl_storage_type == DB_BTRE_STORE) ||
		 (dmfptr->tbl_storage_type == DB_RTRE_STORE) ||
		 (dmfptr->tbl_storage_type == DB_ISAM_STORE)) )
	{
	    i4  pgtype;

	    varp->opv_dircost = 1.0;

	    /*
	    ** Make assumptions about page type -> page/row overheads
	    ** The overheads used are data page, not btree index page overheads
	    ** That's okay - we're just looking for rough estimates
	    */
	    pgtype = (dmfptr->tbl_pgsize == DB_COMPAT_PGSIZE) ?
			DB_PG_V1: DB_PG_V3;
	    varp->opv_kpb = 
		(dmfptr->tbl_pgsize - DB_VPT_PAGE_OVERHEAD_MACRO(pgtype))
	       / (varp->opv_mbf.opb_keywidth + DB_VPT_SIZEOF_LINEID(pgtype));

	    if (varp->opv_kpb < 2.0)
		varp->opv_kpb = 2.0;
	    if (dmfptr->tbl_storage_type == DB_ISAM_STORE)
	    {
		varp->opv_kpleaf = 0;
		varp->opv_leafcount = 0;
	    }
	    else
	    {
		/* kpleaf calculation assumes no non-key columns */
		varp->opv_kpleaf = varp->opv_kpb;
		if (dmfptr->tbl_record_count)
		{
		    varp->opv_leafcount = 
				dmfptr->tbl_record_count/varp->opv_kpleaf;
		    if (varp->opv_leafcount < 1.0)
			varp->opv_leafcount = 1.0;
		}
		else
		    varp->opv_leafcount = 1.0;
	    }
	}
	else if ((dmfptr->tbl_storage_type == DB_BTRE_STORE) ||
	    (dmfptr->tbl_storage_type == DB_RTRE_STORE))
	{
	    /* 
	    ** Let dmf work this out, since it knows about page and row 
	    ** overhead and also takes into consideration potentially
	    ** different entry lengths for leaf and index pages
	    */
	    varp->opv_kpb = dmfptr->tbl_kperpage;
	    varp->opv_kpleaf = dmfptr->tbl_kperleaf;
	    varp->opv_dircost = dmfptr->tbl_ixlevels;
	    varp->opv_leafcount = dmfptr->tbl_lpage_count;
	}
	else if (dmfptr->tbl_storage_type == DB_ISAM_STORE)
	{
	    OPO_TUPLES	    tupleperblock; /* estimated number of tuples in an index
					** block where size of one tuple is the
					** length of the attribute being ordered
					** on ... plus the size a block ptr */
	    OPO_BLOCKS      blocks;     /* number of blocks accessed in index */
	    OPO_BLOCKS      previous;   /* total blocks accessed in the index */
	    i4	    dirsearch;
	    OPO_BLOCKS	    indexpages;
	    OPO_BLOCKS	    root_blocks;  /* number of index pages which are not
					** on the final fanout level */
	    OPO_BLOCKS	    index_blocks; /* number of index pages for btree */
	    OPO_BLOCKS	    leaf_blocks; /* number of index and leaf pages for
					** btree */

	    indexpages = dmfptr->tbl_ipage_count;
	    if (indexpages < 1.0)
		indexpages = 1.0;

	    /* 
	    ** Let dmf work out keys-per-page since it know about
	    ** page and row overhead for different table types
	    */
	    varp->opv_kpb = dmfptr->tbl_kperpage;
	    tupleperblock = dmfptr->tbl_kperpage;

	    root_blocks =
	    index_blocks = 0.0;
	    previous = 1.0;
	    leaf_blocks =
	    blocks = tupleperblock;
	    for (dirsearch = 1; previous < indexpages; )
	    {
		root_blocks = index_blocks;
		index_blocks = previous;    /* previous level contain only index blocks */
		previous += blocks;	    /* calculate total blocks used so far */
		leaf_blocks = blocks;	    /* lowest level contain leaf pages for btree */
		blocks = blocks * blocks;
		dirsearch += 1;		    /* probably faster than using logs */
	    }
	    blocks = leaf_blocks;	    /* get previous value, i.e. leaf page
					    ** level for btree */
	    varp->opv_dircost = dirsearch - (previous - indexpages)/blocks;  /* 
					    ** calculate the average directory 
					    ** search cost; if fanout is incomplete
					    ** then subtract the probabilistic fraction
					    ** in which one less directory search would be
					    ** made - FIXME
					    ** get fill factor involved in this
					    ** calculation */
	    blocks = (dmfptr->tbl_dpage_count < 1.0) ? 1.0 :
						dmfptr->tbl_dpage_count;
	    varp->opv_leafcount = blocks;

					    /* opv_leafcount is probably not used anymore since DMF
					    ** returns an leaf page count in the tbl_dpage_count field
					    ** for a secondary btree index now */
					    /* used for BTREE secondary indexes which have no leaf pages
					    ** to put an upper bound on the number of pages that
					    ** can be hit on any particular level of the btree, which
					    ** can be thought of as data pages */
	}
    }
    else { 				    /* Temporary relation   */
      /* 
      ** Determine the best possible page size for this relation
      ** Try to fit at least 20 rows in the temp relation if possible.
      */
      pagesize = opn_pagesize(global->ops_cb->ops_server, eqsp->opn_relwid);
    }
    opo_orig(subquery, cop);	    /* initialize the ordeqc of the DB_ORIG
				    ** node */
    if (distributed)
	opd_orig(subquery, cop);    /* initialize the distributed portion of
				    ** the orig node */
    if (keyed_only)
	return(OPN_SIGOK);

    if (cop->opo_cost.opo_pagesize == 0)
      cop->opo_cost.opo_pagesize = pagesize;

    /* FIXME - is not opo_dio 0 here ? */
    {
        OPO_ISORT               ordering;   /* this is the ordering which can be
                                    ** assumed after keying has been done in
                                    ** the relation, - OPE_NOEQLCS if no
                                    ** ordering can be assumed
                                    ** not used here */
        opn_prcost (subquery, cop, &sargtype, &pblk, &hp,
            &imtid, FALSE, &ordering,
            &pt_valid, &dio, &cpu);
    }
    dio += cop->opo_cost.opo_dio; /* add disk i/o 
				    ** cost to restrict/project
                                    ** i.e. cost of reading tuples using
                                    ** keyed access or cost of reading
                                    ** entire relation */
    if (selapplied)
	pr_tuples = rlsp->opn_reltups; /* duplicate this info here
				    ** to help lock chooser.  We will 
				    ** use this to guess how many
				    ** pages in the result relation
				    ** will be touched.  It is also
				    ** printed out in the QEP */
    else
	pr_tuples = rlsp->opn_reltups * selectivity; /* do not
					    ** apply selectivity twice */

    if (pr_tuples < 1.0)
	pr_tuples = 1.0;

    /* FIXME - same thing */
    cpu += cop->opo_cost.opo_cpu; /* get CPU time used to project and
                                    ** restrict the node */

    prcost = opn_cost(subquery, dio, cpu);

    /* count another fragment for trace point op188/op215/op216 */
    subquery->ops_currfragment++;

    /* With traditional enum, the only way to have an ops_cost is to have
    ** a best plan, which implies we looked at all leaves already.  So
    ** presumably enum memory was dumped so that the search could continue.
    ** It's safe to assume that a PR/ORIG subtree that costs more than the
    ** entire best plan so far is not useful, and we might as well reduce
    ** the search space.
    **
    ** Things are very different with the new enumeration.  New enumeration
    ** can and will look for best-plans that do NOT encompass all leaves.
    ** An early pass may generate a very cheap best-fragment that does not
    ** involve some expensive leaf.  A later pass (that includes that
    ** fragment) may require the expensive leaf to be able to form a plan.
    ** If all we make is the ORIG, it makes it look like the var can only
    ** be keyed to, which may in fact not be right.  This could prevent
    ** new enum from finding a valid plan for the larger set of tables, until
    ** it flushes enumeration memory and tries again.  Avoid this by always
    ** saving both the ORIG and PR/ORIG CO-tree forest.
    **
    ** (If somehow we got here as root, act normally;  shouldn't happen
    ** during greedy enum.)
    */
    if ((global->ops_estate.opn_rootflg
	||
	(subquery->ops_mask & OPS_LAENUM) == 0)
	&&
	prcost >= subquery->ops_cost)
    {
	if (op215)
	{
	    global->ops_trace.opt_subquery = subquery;
	    global->ops_trace.opt_conode = cop;
	    opt_cotree(cop);
	    global->ops_trace.opt_conode = NULL; 
	}
	return(OPN_SIGOK);	    /* do not do anything if the cost is larger
                                    ** than the current best plan
                                    */
    }
    ncop = opn_cmemory(subquery);   /* get space for a project-restrict CO node 
                                    */
    ncop->opo_storage = DB_HEAP_STORE; /* no reformat for this CO node */
    {
	OPV_GRV		*grvp;	    /* ptr to global range var descriptor
				    ** for ORIG node */
	if ((cop->opo_storage == DB_HEAP_STORE)  
	    &&
	    (grvp = varp->opv_grv)
	    )
	{
	    if (grvp->opv_gsubselect
		&&
		(
		    (grvp->opv_gsubselect->opv_subquery->ops_sqtype == OPS_RFAGG)
		    ||
		    (grvp->opv_gsubselect->opv_subquery->ops_sqtype == OPS_HFAGG)
		    ||
		    (grvp->opv_gsubselect->opv_subquery->ops_sqtype == OPS_RSAGG)
		    ||
		    (grvp->opv_gsubselect->opv_subquery->ops_sqtype == OPS_VIEW)
		)
	       )
# if 0
	    check for noncorrelated rfagg removed,
	    removed since correlated aggregates need an SEJOIN and these need
            to be projected/restrict always because OPC has a restriction
            in which the parent node of an aggregate result needs to be a
            PST_BOP with a "subselect indicator"
# endif
	    {   /* eliminate node from list to reduce cost calculations needed */
		jsbtp->opn_coback =
		jsbtp->opn_coforw = (OPO_CO *)jsbtp;
	    }
	    if (grvp->opv_gquery 
		&& 
		(   (grvp->opv_created == OPS_RFAGG)
		    ||
		    (grvp->opv_created == OPS_FAGG)
		))
		ncop->opo_storage = DB_SORT_STORE; /* function aggregate has
					** ordered by lists */	    		
	}
    }
    /* create a new CO project restrict node ncop which points to cop */
    cop->opo_pointercnt++;          /* cop has one more CO pointer to it */
    if (!(cop->opo_pointercnt))
	opx_error(E_OP04A8_USAGEOVFL); /* report error is usage count overflows */
    ncop->opo_ordeqc = OPE_NOEQCLS;

    switch (sargtype)		    /* define adjacent duplicate factor and
                                    ** sortedness factor */
    {
    case ADC_KEXACTKEY:		    /* isam or hash or explicit equality TID 
                                    ** clause in the qualification, (assume the
				    ** boolfact sarg constants are sorted and 
                                    ** then used to access the relation, this is
				    ** done by opb_create) */
    {
	if (hp)
	    ncop->opo_cost.opo_adfactor = hp->oph_reptf; /* this may happen 
				    ** with tid access */
	else				
	    ncop->opo_cost.opo_adfactor = 1.0; /* we do not have an ordering
                                    ** equivalence class yet so worst case
                                    ** adjacent duplicate factor */
	ncop->opo_cost.opo_sortfact = rlsp->opn_reltups; /* number of
                                    ** tuples in the relation */
        /* b107351 */
        if ( varp->opv_grv->opv_relation->rdr_rel->tbl_storage_type ==
	     DB_HASH_STORE )
	     ncop->opo_storage = DB_HASH_STORE;
        else
	{
	  if (varp->opv_grv->opv_relation->rdr_rel->tbl_storage_type == DB_BTRE_STORE)
	     ncop->opo_storage = DB_SORT_STORE;  /* mark node as sorted on the
				    ** ordering equivalence class if we
				    ** can guarantee it, this will cause
				    ** opo_pr to lookup the ordering which
				    ** is contained in range var descriptor
                                    ** FIXME- need to check multi-attribute
                                    ** ordering portion which is not ordered
				    */
	  else
	    ncop->opo_storage = varp->opv_grv->opv_relation->rdr_rel->tbl_storage_type;
	}	
	break;
    }

    case ADC_KRANGEKEY:	    /* must be isam or btree */
    {
	/*static*/opn_adfsf (ncop, cop, blocks, pblk, rlsp->opn_reltups);
	break;
    }

      default: 				/* read whole relation */
	    /*static*/opn_adfsf (ncop, cop, blocks, cop->opo_cost.opo_reltotb, 
                rlsp->opn_reltups);
    }

    ncop->opo_outer	= cop;              /* outer node is leaf i.e. original
                                            ** relation */
    ncop->opo_sjpr	= DB_PR;	    /* project restrict node */
    ncop->opo_union.opo_ojid = OPL_NOOUTER; /* not an outer join */
    ncop->opo_maps	= &eqsp->opn_maps;  /* save ptr to equivalence class
                                            ** map of attributes which will
                                            ** be returned */
    ncop->opo_cost.opo_pagesize = pagesize;
    ncop->opo_cost.opo_tpb	= (i4)opn_tpblk(global->ops_cb->ops_server,
						 pagesize, eqsp->opn_relwid); 
					    /* tuples per block */
    ncop->opo_cost.opo_cvar.opo_cache = OPN_SCACHE; /* project restrict will use
					    ** the single page cache prior to
					    ** accessing the block reads */
    ncop->opo_cost.opo_reltotb	= blocks;   /* estimated number of blocks in
                                            ** relation after project restrict*/
    ncop->opo_cost.opo_dio	= dio;      /* disk i/o to do this project
                                            ** restrict */
    ncop->opo_cost.opo_cpu	= cpu;	    /* cpu required for project 
                                            ** restrict*/
    if (pt_valid)
	ncop->opo_cost.opo_pagestouched = pblk; /* number of primary blocks 
					    ** read is an accurate estimate of 
					    ** pages touched */
    else
	ncop->opo_cost.opo_pagestouched = 0.0; /* if it is not accurate then
					    ** estimate 0.0 pages touched so
					    ** that page level locking will
					    ** be used */
    ncop->opo_cost.opo_tups	= pr_tuples; /* number of tuples in PR result */
    if(	(cop->opo_storage == DB_BTRE_STORE) /* if a btree */
	||
    	(cop->opo_storage == DB_ISAM_STORE) /* if a isam, then ordering
                                            ** can be used for partial
                                            ** sort merge, note that
                                            ** ncop->opo_storage is
                                            ** DB_HEAP_STORE so that
                                            ** ordering is not used for
                                            ** a full sort merge */
	||
	(ncop->opo_storage == DB_SORT_STORE)) /* or sorted due to exact
					    ** keys */
	opo_pr(subquery, ncop);		    /* initialize the ordering
					    ** for the project restrict
                                            ** node */

    /* In the following condition we check first whether the relation:
    ** (1) has a multi-attribute key with matching attributes in the
    ** query (opb_count > 1).
    ** (2) has a multi-attribute key with at least one matching boolean
    ** factor in the query (opb_bfcount > 0).
    ** If either is false then there is either no multi-attribute key,
    ** or a file scan is preferable to using the multi-attribute key.
    */
    if ( varp->opv_mbf.opb_count > 1 && varp->opv_mbf.opb_bfcount > 0 )
    {	/* look at all histograms in the relation and determine which
	** have a requirement for special processing due to multi-attribute
	** index restrictivity rules, FIXME, this should be replaced
	** by TRUE multi-attribute processing */
	OPH_HISTOGRAM	    *histp;
	OPZ_AT                 *abase;  /* base of array of ptrs to joinop
					** attribute elements */
	abase = subquery->ops_attrs.opz_base; /* base of array of ptrs to
					    ** joinop attribute elements */
	for (histp = rlsp->opn_histogram; histp; histp = histp->oph_next)
	    if (!(histp->oph_mask & OPH_COMPOSITE) && 
		abase->opz_attnums[histp->oph_attribute]->opz_mask & OPZ_MAINDEX)
	    {
		histp->oph_origtuples = rlsp->opn_reltups;
		histp->oph_odefined = TRUE;
	    }
  
    }
    cop		= ncop;			    /* all reformats go from cop
					    ** (we must make a copy of the
					    ** original relation before
					    ** reformatting) */
    varp->opv_pr = ncop;		    /* save project-restrict for calculation 
					    ** of initial guess of best set of
					    ** relations */
    varp->opv_prtuples = pr_tuples;
    {
	bool		root_flag;

	root_flag = global->ops_estate.opn_rootflg;
	if (distributed)
	{
	    bool		useful;	    /* TRUE if distributed plan
					    ** is useful for at least one site */
	    if (root_flag)
	    {
		OPO_COST	cpu_sortcost;   /* total cpu cost for sort node */
		OPO_COST	dio_sortcost;   /* total dio cost for sort node */
		if (subquery->ops_msort.opo_mask & OPO_MTOPSORT)
		{
		    cpu_sortcost = eqsp->opn_cpu;
		    dio_sortcost = eqsp->opn_dio;
		}
		else
		{
		    cpu_sortcost = 0.0;
		    dio_sortcost = 0.0;
		}
		useful = opd_prleaf(subquery, rlsp, cop, eqsp->opn_relwid, 
			cpu_sortcost, dio_sortcost); /* add 
					    ** distributed costs for this node*/
		if (!useful 
		    ||
		    opd_bestplan(subquery, cop,
			((subquery->ops_msort.opo_mask & OPO_MTOPSORT) != 0),
			cpu_sortcost, dio_sortcost, &sortcop))
		{
		    if (op215)
		    {
			global->ops_trace.opt_subquery = subquery;
			global->ops_trace.opt_conode = cop;
			opt_cotree(cop);
			global->ops_trace.opt_conode = NULL; 
		    }
		    return(OPN_SIGOK);
		}
		if (subquery->ops_msort.opo_mask & OPO_MTOPSORT)
		    opn_createsort(subquery, sortcop, cop, eqsp); /* initialize the fields of
					    ** the top sort node, opd_bestplan has already
					    ** placed the top sort node in the appropriate
					    ** CO plan locations */
	    }
	    else
	    {
		op216 = FALSE; /* op216 traces only the best plans */
		useful = opd_prleaf(subquery, rlsp, cop, eqsp->opn_relwid, 
		    (OPO_CPU)0.0, (OPO_BLOCKS)0.0); /* add 
					    ** distributed costs for this node*/
	    }
	}
	else if (root_flag)
	{
	    OPO_COST		prcost1;

	    prcost1 = opn_cost(subquery, dio, cpu);
	    if (subquery->ops_msort.opo_mask & OPO_MTOPSORT)
	    {	/* FIXME - can detect user specified orderings here and
		** avoid a sort node */
		OPO_ISORT	sordering;
		bool		sortrequired;
		if (cop->opo_storage == DB_SORT_STORE)
		    sordering = cop->opo_ordeqc;
		else
		    sordering = OPE_NOEQCLS;
		sortrequired = FALSE;	/* no deferred semantics problem for
					** single table lookups */

		if (subquery->ops_msort.opo_mask & OPO_MDESC)
		{
		    /* Single node and descending order by. Assure the key
		    ** key structure isn't multi-attr in which the 1st column
		    ** is in a constant BF. Read backwards doesn't work. */
		    OPO_CO	*orig = cop;
		    OPO_EQLIST	*orig_list;
		
		    if ((orig->opo_sjpr == DB_ORIG || (orig = orig->opo_outer)
			&& orig->opo_sjpr == DB_ORIG) &&
			!(orig->opo_ordeqc < subquery->ops_eclass.ope_ev ||
			subquery->ops_bfs.opb_bfeqc == NULL) &&
			(orig->opo_ordeqc > OPE_NOEQCLS ||
			(orig_list = subquery->ops_msort.opo_base->opo_stable
			[orig->opo_ordeqc-subquery->ops_eclass.ope_ev]->opo_eqlist) 
			== NULL || BTtest((i4)orig_list->opo_eqorder[0],
			(char *)subquery->ops_bfs.opb_bfeqc))) sortrequired = TRUE;
		}
		if (opn_checksort(subquery, &subquery->ops_bestco, subquery->ops_cost,
			    dio, cpu, eqsp, &prcost1, sordering, &sortrequired, 
			    cop) )
		{
		    if (op215)
		    {
			global->ops_trace.opt_subquery = subquery;
			global->ops_trace.opt_conode = cop;
			opt_cotree(cop);
			global->ops_trace.opt_conode = NULL; 
		    }
		    return(OPN_SIGOK);	/* return if added cost of sort node, or creating
					** relation is too expensive */
		}
		if (sortrequired)
		    sortcop = opn_cmemory(subquery); /* allocate memory here so that
					** we do  not run out of memory prior to
					** calling opn_dcmemory, which would delete
					** the previous best plan */
	    }
	    opn_dcmemory(subquery, subquery->ops_bestco);
	    if (sortcop)
	    {
		opn_createsort(subquery, sortcop, cop, eqsp);
		subquery->ops_bestco = sortcop;
	    }
	    else
		subquery->ops_bestco	= cop;
	    subquery->ops_besthisto = rlsp->opn_histogram;

	    if (global->ops_gmask & OPS_FPEXCEPTION)
		global->ops_gmask |= OPS_BFPEXCEPTION; /* a plan was found which
					** did not have a floating point exception
					** so skip over subsequent plans with
					** floating point exceptions */
	    else
		global->ops_gmask &= (~OPS_BFPEXCEPTION); /* reset exception
					** flag if plan was found to be free
					** of float exceptions */
            subquery->ops_tcurrent++;	/* increment plan number */
	    subquery->ops_tplan = subquery->ops_tcurrent; /* since this is
					**a single leaf tree, opn_ceval
					** will detect a timeout */
	    /* save the best fragment so far for trace point op188/op215/op216 */
	    subquery->ops_bestfragment = subquery->ops_currfragment;
	    subquery->ops_cost	= prcost1;
	    global->ops_estate.opn_search = FALSE; /* new best CO
					** found so memory garbage
					** collection routines may be
					** useful */
	}

	if (!root_flag)
	{
	    op216 = FALSE; /* op216 traces only the best plans */
	    opn_coinsert (jsbtp, cop);	/* put in co list, do not place into CO
					** if at the root since it may be deallocated
					** when a new best plan is found, and the PR
					** is worthless if it was once the best plan
					** so it does not need to be in the linked list
					** of CO nodes */
	}
    }

    if (op188 || op215 || op216)
    {
	global->ops_trace.opt_subquery = subquery;
	if (!sortcop)
	{
	    global->ops_trace.opt_conode = cop;
	    opt_cotree(cop);
	}
	else
	{
	    global->ops_trace.opt_conode = sortcop;
	    opt_cotree(sortcop);
	}
	global->ops_trace.opt_conode = NULL; 
    }

    if (global->ops_cb->ops_check)
    {
	i4	    first;
	i4	    second;

	/* If we have a usable plan, check for trace point op255 timeout setting */
	if( subquery->ops_bestco &&
	    opt_svtrace( global->ops_cb, OPT_F127_TIMEOUT, &first, &second) &&
	    !(subquery->ops_mask & OPS_LAENUM) &&
	    (first <= subquery->ops_tcurrent) &&
	    /* check if all subqueries should be timed out OR a particular subquery is being searched for */
	    ( (second == 0) || (second == subquery->ops_tsubquery) ) )
	{
	    opx_verror(E_DB_WARN, E_OP0006_TIMEOUT, (OPX_FACILITY)0);
	    opn_exit(subquery, FALSE);
	    /*
	    ** At this point we return the subquery->opn_bestco tree.
	    ** This could also happen in freeco and enumerate 
	    */
	    return(OPN_SIGEXIT);
	}
    }
    return(OPN_SIGOK);
}
