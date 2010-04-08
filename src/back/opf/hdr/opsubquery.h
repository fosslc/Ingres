/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: OPSUBQUERY.H - Describes the structure used to define subquery objects
**
** Description:
**      Contains structures visible to all phases of query optimization
**      within the optimizer.
**       
**      Some naming conventions used:
**        Current phases of the optimizer
**          OPA - structures used only in the aggregate processing phase
**          OPJ - structures used in joinop phase 
**          OPN - structures used only in the enumeration phase
**          OPC - structures used only in query compilation
**          OPS - structures used for session level management
**          OPT - structures defining debug/trace information
**          OPX - structures defining exception and error handling
**          OPG - structures defining server level management
**          OPU - utility routines
**          OPM - memory management routines
**
**        Partitioning according to datatypes
**          OPB - structures defining boolean factor information
**          OPV - structures defining range variable information
**          OPZ - structures defining attributes, or function attributes
**          OPO - structures defining cost ordering information
**          OPE - structures defining equivalence class information
**          
** History: $Log-for RCS$
**      20-feb-86 (seputis)
**          initial creation
[@history_line@]...
**/

/*}
** Name: OPJ_SUBQUERY - variables used locally in joinop phase of processing
**
** Description:
**      Contains variables used locally in the joinop phase of processing
**
** History:
**      26-feb-87 (seputis)
**          initial creation
[@history_template@]...
*/
typedef struct _OPJ_SUBQUERY
{
    OPV_IVARS       opj_select;         /* local range variable which contains
					** all the subselect attributes which
                                        ** will be SEJOINed with the query */
    OPV_IVARS       opj_nselect;        /* current range var to be associated
                                        ** with subselects being analyzed */
    bool            opj_virtual;        /* TRUE if a virtual table is
                                        ** defined in the subquery */
}   OPJ_SUBQUERY;

/*}
** Name: OPU_UNION - variable used to process union optimization
**
** Description:
**      Contains state used to optimize unions
**
** History:
**      28-jun-89 (seputis)
**          initial creation
**	8-Jan-2010 (kschendel) b123122
**	    Define "processed" flags for pc-join, exchange analysis.
*/
typedef struct _OPU_UNION
{
    PST_QNODE       *opu_qual;          /* list of qualifications which can be
					** applied to the union */
    i4              opu_mask;           /* list of flags used to process unions */
#define                 OPU_REFUNION       1
/* TRUE if this subquery references a union or union view */
#define			OPU_QUAL	   2
/* TRUE if the qualifications for the union view have more than one parent */
#define			OPU_NOROWS	   4
/* TRUE if the qualifications for this subquery is such that no rows can
** be returned, for unions it means that this partition can be eliminated */
#define			OPU_EQCLS	   8
/* TRUE if equivalence classes need to be recalculated for the target list */
#define                 OPU_PROCESSED      0x0010
/* TRUE if the union has been processed for qualification subsitution */
#define			OPU_PCJ_PROCESSED  0x0020
/* TRUE if this subquery has been processed for PC join analysis */
#define			OPU_EXCH_PROCESSED 0x0040
/* TRUE if this subquery has been processed for exchange placement */
    struct _OPS_SUBQUERY *opu_agglist;      /* ptr to list of aggregates needed to
				    ** evaluate this query */
}   OPU_UNION;

/*}
** Name: OPS_TABHINT	- table hint (index/join)
**
** Description:
**      Contains one table hint with table/index name(s).
**
** History:
**      20-mar-06 (dougi)
**	    Written for optimizer hints project.
*/
typedef struct _OPS_TABHINT
{
    DB_TAB_NAME		ops_name1;	/* 1st table name */
    DB_TAB_NAME		ops_name2;	/* 2nd table name (join hint) or 
					** index name (index hint) */
    OPV_IVARS		ops_vno1;	/* local RT no of 1st table (or -1) */
    OPV_IVARS		ops_vno2;	/* local RT no of 2nd table (or -1) */
    i4			ops_hintcode;	/* hint types - see PST_HINTENT in
					** back!hdr!psfparse.h */
    bool		ops_hintused;	/* TRUE - this hint is being used
					** in current OPS_SUBQUERY */
} OPS_TABHINT;

/*}
** Name: OPS_SUBQUERY - Define the header structure for a sub query
**
** Description:
**      This structure defines the header for a subquery.  The ops_next
**      field will provide a linked list of the subqueries in the order that
**      processing should occur.  Consider an "aggregate tree" i.e. the
**      query tree with all the nodes except aggregates removed then this
**      list would be a post-fix ordering (approximetely - aggregates are
**      calculated together in the same subquery where possible).  Each of
**      the subqueries is considered to be a retrieve into temporary table
**      except the last one which is the main query.
**       
**      The original query tree will not exist after it has been modified
**      into a list of subqueries i.e. the original tree will be modified
**      as opposed to being copied and modified.
**
**      This structure is passed to most routines in joinop and enumeration
**      as the "state variable" since routines in these phases will operate
**      within the context of a subquery (e.g. an equivalence class in this
**      subquery has no meaning in the next subquery).  On the other hand
**      the aggregate processing phase operates on subqueries as entities
**      within a global context, so the global state variable is passed to
**      routines in the aggregate processing phase (i.e. OPS_STATE).
**
** History:
**     20-feb-86 (seputis)
**          initial creation
**     6-nov-88 (seputis)
**          add tracing counters for best plans found
**      20-sep-89 (seputis)
**          added outer join structure
**      22-sep-89 (seputis)
**          added indicator ZEROTUP to fix sybase bug
**      20-mar-90 (seputis)
**          added ops_oj to support outer joins
**      26-mar-90 (seputis)
**          -added OPS_MULTI_INDEX, OPS_CBF for b20632
**	24-jun-90 (seputis)
**	    -added opa_fcorelated to fix shape consistency check
**      26-dec-90 (seputis)
**          add defines, and change conflicting define for cart prod support
**	18-apr-91 (seputis)
**	    - fix for b36920 - added new defines
**	5-jun-91 (seputis)
**	    - fix STAR access violation, and local problem in which nested
**	    subselect with union views could result in a query plan not
**	    found error
**      12-aug-91 (seputis)
**          - fix b38691 - print error if aggregate temp too large
**      26-dec-91 (seputis)
**          - 41526 - fix count(distinct expr) problem with top sort node
**          removal
**      16-oct-92 (ed)
**          - fix bug 40835 - wrong number of duplicates when union views and
**          subselects are used
**	5-jan-93 (ed)
**	    - bug 47541, 47912, add new defines to indicate datatype mismatch
**	    has occurred
**	    - convert all masks to hex for easier maintenance
**      4-feb-93 (ed)
**          - bug 47377 - add OPN_IEMAPS
**	26-apr-93 (ed)
**	    - remove obsolete define
**	17-may-93 (ed)
**	    - b51879 new mask to indicate 2 aggregates can be computed together
**	    but require different range variables
**	19-jan-94 (ed)
**	    - added OPS_UVREFERENCE to help remove a control flow difference
**	    between distributed and local, i.e. views are retained in the main
**	    query list in distributed
**      31-jan-94 (markm)
**          - b57875 added new mask to indicate that the best plan so far 
**          is a cart-prod, OPS_CPFOUND.
**      25-mar-94 (ed)
**          bug 59355 - an outer join can change the result of a resdom
**          expression from non-null to null, this change needs to be propagated
**          for union view target lists to all parent query references in var nodes
**          or else OPC/QEF will not allocate the null byte and access violations
**          may occur
**      11-apr-94 (ed)
**          bug 59937 - E_OP0489 consistency check due to inner join ojids not
**          being set in the boolean factor placement maps
**	    bug 59588 - change type of ops_seqctest, since special eqc test
**	    added after enumeration rather than before in order to avoid
**	    predetermining relation placement strategies
**	2-may-94 (ed)
**	    remove obsolete variable
**	06-mar-96 (nanpr01)
**	    Use ops_width consistently with DMT_TBL_ENTRY definition. 
**	14-may-97 (inkdo01)
**	    Defined OPS_NONCNF flag to indicate predicates NOT transformed
**	    to CNF.
**	14-jul-99 (inkdo01)
**	    Removed OPS_VFIXUP which had been ifdef 0'd out, and replaced by
**	    OPS_IDIOT_NOWHERE to identify join queries with no where clauses.
**	16-sep-02 (inkdo01)
**	    Added ops_jtreep and replaced unused OPS_BOR with OPS_LAENUM
**	    for bottom up lookahead join enumeration technique. Also added ops_tempco
**	    and ops_bestfragco.
**	 6-sep-04 (hayke02)
**	    Added OPS_COLLATION to indicate that a collation sequence is
**	    present, and that OPZ_COLLATT attributes need to be added in the
**	    resdom list for character datatypes. This change fixes problem
**	    2940, bug 112873.
**	17-may-04 (inkdo01)
**	    Added OPS_NEWBESTFRAG to assist tracking of greedy enumeration 
**	    heuristic.
**	25-apr-05 (inkdo01)
**	    Added ops_lawholetree to fine tune some greedy enumeration 
**	    heuristics.
**	6-july-05 (inkdo01)
**	    Added ops_bestrlp for cost alterations of aggregate sq's.
**	 8-nov-05 (hayke02)
**	    Added ops_mask1 (ops_mask is full) and also added OPS_VAREQVAR
**	    to indicate that we are processing an ADI_EQ_OP PST_BOP node with
**	    PST_VAR left and right. This change fixes bug 115420 problem
**	    INGSRV 3465. - During XIP change to using ops_mask2 which was
**	    added as an overflow for ops_mask
**	20-mar-06 (dougi)
**	    Added OPS_HINTPASS flag & OPS_TABHINT array.
**	may-06 (dougi)
**	    Added ops_bygsets and OPS_ROLLUP for group by enhancements.
**	30-Jun-2006 (kschendel)
**	    Replace bestrlp with besthisto.
**	21-nov-2006 (dougi)
**	    Added OPS_AGGCOST to indicate opn_modcost() generated costs.
**	16-Apr-2007 (kschendel) SIR 122513
**	    Define no-PC-agg allowed flag.
**	5-may-2008 (dougi)
**	    Added OPS_TPROC to ops_mask2 for table procedures.
**	2-Jun-2009 (kschendel) b122118
**	    Remove FALIKE flag, ops_tempco -- both are dead code.
**	27-Oct-2009 (kiria01) SIR 121883
**	    Scalar sub-selects -  corrected ops_mode type
**	01-Mar-2010 (smeke01) b123333
**	    Added counters for current and best plan fragment, for trace
**	    point op188.
*/
typedef struct _OPS_SUBQUERY
{
    struct _OPS_STATE *ops_global;      /* ptr to global state variable since
                                        ** most routines in the optimizer
                                        ** will work within the "local state
                                        ** variable" OPS_SUBQUERY, but will
                                        ** also need information global to
                                        ** all subqueries
                                        */
    PST_QNODE       *ops_root;          /* root of query tree defining the
                                        ** sub-query
                                        ** - this node was detached from the
                                        ** original query and now will be
                                        ** dealt with as a sub-query.  The
                                        ** position occupied in the original
                                        ** query tree by this node was replaced
                                        ** by a node representing the result 
                                        ** component of this structure.
                                        */
    struct _OPS_SUBQUERY *ops_next;     /* next subquery in list
                                        ** - this list is in "execution order"
                                        ** NULL if end of list
                                        ** - NULL indicates the main query
                                        ** - if non-zero then this will be
                                        ** a retrieve into an aggregate 
                                        ** temporary relation
                                        */
    struct _OPS_SUBQUERY *ops_union;    /* list of subquery structures which
                                        ** are to be unioned together */
    OPS_SQTYPE	   ops_sqtype;          /* this indicates the type of
                                        ** subquery represented by this
				        ** node 
                                        */
    OPV_IGVARS      ops_result;         /* global range variable number of
                                        ** the result relation for this
                                        ** subquery OR
                                        ** OPV_NOGVAR - indicates that
                                        ** no result relation is produced
                                        ** e.g. simple aggregate.
                                        ** or global range variable number
                                        ** assigned to subselect
                                        ** - every result relation is temporary
                                        ** unless it is the main query; i.e. it
                                        ** is the last subquery in the list
                                        ** ops_next == NULL
                                        */
    OPV_IGVARS      ops_gentry;         /* global range variable number used
                                        ** to find global range table entry
                                        ** corresponding to this subquery, same
                                        ** as ops_result, except that ops_result
                                        ** is set only when a "real" relation
                                        ** is to be created, whereas this value
                                        ** will be set in all cases in which a
                                        ** global range variable exists, e.g.
                                        ** subselects will have a global range
                                        ** variable entry but a relation is
                                        ** never instantiated, OPC uses 
                                        ** ops_result being set in order to
                                        ** determine whether to create a
                                        ** temporary relation */
    OPV_IVARS       ops_localres;       /* - local joinop range table number of
                                        ** result
                                        ** - logically references same relation
                                        ** as ops_result
                                        ** - set only for deletes and replaces
                                        ** otherwise set to OPV_NOVAR
                                        */
    OPV_IGVARS      ops_projection;     /* global range variable number of
                                        ** of the projection of the bylist
                                        ** (if required by the function
                                        ** aggregate) ... this variable is
                                        ** probably not going to be used,
                                        ** instead see definition of 
                                        ** OPS_PROJECTION
                                        ** - OPV_NOGVAR indicates this is a
                                        ** simple aggregate or no projection
                                        ** is required for function aggregate
                                        */
    PST_QNODE       *ops_byexpr;        /* NULL if this subquery
                                        ** is a not a OPS_FAGG type
                                        ** - ptr to beginning of list of
                                        ** PST_RESDOM nodes which represent
                                        ** the "by expressions" of the aggregate
                                        ** function
                                        ** - also can be considered to be the
                                        ** terminating node for the list of
                                        ** PST_RESDOM nodes representing the
                                        ** AOP nodes beginning at the root
                                        */
    PST_QNODE	    *ops_bygsets;	/* NULL if this subquery doesn't 
					** have a grouping sets group by
					** clause, else it addresses the 
					** original group by list and ops_byexpr
					** addresses the colsolidated list.
					*/
    PSQ_MODE        ops_mode;           /* subquery mode - retrieve, delete,
                                        ** append, replace ... uses same
                                        ** constants as PST_QTREE.pst_mode i.e.
                                        ** PSQ_RETRIEVE, PSQ_RETINTO, PST_APPEND
                                        ** PSQ_DELETE, PSQ_REPLACE
                                        */
    OPS_DUPLICATES  ops_duplicates;     /* handling of duplicates for subquery
                                        ** OPS_DUNDEFINED - do not care
                                        ** OPS_DKEEP - keep duplicates ... means
                                        ** add tids to query so duplicates are
                                        ** not discarded
                                        ** OPS_DREMOVE - remove duplicates ...
                                        ** means add sort node to top of query
                                        ** plan to remove duplicates
                                        */
    OPV_GBMVARS     ops_correlated;     /* bitmap of corelated variables which
                                        ** are contained in this subquery, note
                                        ** that these variables may be 
                                        ** indirectly referenced in a child
                                        ** subquery of this node */
    OPV_GBMVARS	    ops_fcorelated;	/* these are corelated variables which
					** where changed to from list variables
					** due to flattening */
    OPV_GBMVARS     ops_aggmap;         /* valid for SELECT only, map of
                                        ** variables which have been defined
                                        ** in the from list of an aggregate
                                        ** contained in this subquery, used to
                                        ** determine which variables in the
                                        ** from list of the SELECT need to be
                                        ** included in the query plan */
    OPS_WIDTH       ops_width;          /* width of a tuple returned from this
                                        ** subquery
                                        **-sum of bytes needed to store the  
                                        ** simple aggregate results  OR
                                        **-number of bytes needed to store the  
                                        ** function aggregate results,
                                        ** bylist values, + 4 for    
                                        ** the count field
                                        */
    bool            ops_enum;           /* TRUE - if enumeration should be
                                        ** used to estimate the cost of OVQPs
                                        ** - useful for function aggregates
                                        ** in which table size and histogram
                                        ** information could be used when
                                        ** joining to the outer */
    bool	    ops_fraginperm;	/* TRUE - fragment plan from new 
					** enumeration is in permanent memory.
					** New best fragment should return 
					** those nodes to opn_colist
					*/
    i2		    ops_lacount;	/* lookahead count for new enumeration
					** technique
					*/
    OPN_JTREE	    *ops_jtreep;	/* ptr to JTREE node of plan fragment 
					** to be saved using lookahead 
					** enumeration
					*/
    OPO_CO	    *ops_bestfragco;	/* lower left join CO-node addr for
					** best plan in current enumeration
					** iteration
					*/
    OPE_BMEQCLS	    *ops_laeqcmap;	/* map of eqcs from current lookahead
					** enumeration iteration
					*/
    OPO_COST        ops_cost;           /* estimated cost of evaluating
                                        ** ops_bestco, variable is presented
                                        ** to caller for possible future
                                        ** scheduling enhancements
                                        */
    OPO_CO          *ops_bestco;        /* ptr to root of best cost (ordering)
                                        ** tree found by enumeration for this
                                        ** sub-query, which will be used
                                        ** by the query compilation module
                                        ** to produce part of the QEP
                                        ** - NULL indicates query tree
                                        ** does not contain table references
                                        */
    OPH_HISTOGRAM   *ops_besthisto;	/* ptr to OPH_HISTOGRAM for best
					** OPN_RLP that corresponds to bestco.
					** Used for agg estimate mods
					*/
    u_i4	    ops_mask;		/* set of booleans used to define various
					** conditions for this subquery */
#define                 OPS_BYPOSITION	    0x000000001L
/* TRUE is only update by position is allowed for this query, i.e. update
** by TIDs are not allowed as in the case of IMS gateways
*/
#define                 OPS_ZEROTUP_CHECK   0x000000002L
/* TRUE if this query should be retried without flattening if 0 tuples
** have been returned */
#define                 OPS_CINDEX	    0x000000004L
/* TRUE if an index is used which can substitute the base relation */
#define                 OPS_CTID	    0x000000008L
/* TRUE if two secondaries on the same table exist, and those indexes
** share common attributes */
#define                 OPS_MULTI_INDEX	    0x000000010L
/* TRUE if at least one table has more than one secondary index */
#define                 OPS_CBF		    0x000000020L
/* TRUE if boolean factors should be checked to determine if joining
** two secondaries together provides added restrictivity */
#define                 OPS_COAGG	    0x000000040L
/* TRUE if a corelated variable was found in this aggregate */
#define                 OPS_CPLACEMENT	    0x000000080L
/* TRUE - if at least one index cannot be used for index substitution so
** that it must be used with a TID join and the base relation must be
** placed in the tree appropriately as the inner of a parent node */
#define                 OPS_PCAGG	    0x000000100L
/* TRUE - if this is a "partition compatible" aggregation query.
** The aggregation by-list is equal to or a superset of partitioning
** coming up from the aggregation source, which means that we can
** aggregate the partitions independently.
** See also NOPCAGG below.
*/
#define			OPS_TMAX	    0x000000200L
/* TRUE - if search space was reduced due to sort/hold file temporary
** maximum limits being exceeded, perhaps causing no query plan to be found */
#define                 OPS_SFPEXCEPTION    0x000000400L
/* TRUE - if this subquery had floating point exceptions during
** the optimization */
#define                 OPS_NOTIDCHECK	    0x000000800L
/* TRUE - if at least 2 relations exist in which duplicates need to be removed
** and at least one relations exists in which duplicates need to be kept and
** that relation has no TIDs */
#define                 OPS_NTCARTPROD	    0x000001000L
/* TRUE - if OPS_NOTIDCHECK is TRUE, and the relations which require duplicates
** to be removed are not connected to one another, if relations which require duplicates
** to be kept and have no TIDs are ignored but at least one of the duplicate-free
** relations is connected to a relation which does not have TIDs and is required
** to have duplicates kept; this means a cart prod will need to be introduced
** above a join in order to have a legal set of relation placements */
#define                 OPS_CAGGTUP	    0x000002000L
/* TRUE - if the target list is longer than the max width of a tuple, for
** function aggregates only, so that a top sort node cannot be used since it
** will cause an error, with aggregates the width of the expression used to
** evalute the aggregate is used, not the aggregate result */
#define                 OPS_NOPCAGG	    0x000004000L
/* TRUE - if this aggregation is not permitted to be "partition compatible",
** even if it looks like it might otherwise qualify.
** For instance, NOPCAGG is set if some PC-join underneath is an outer
** join, which could introduce nulls and mess up the partitioning.
*/
#define                 OPS_UVREFERENCE	    0x000008000L
/* TRUE - if subquery references at least one union view */
#define                 OPS_AGEXPR	    0x000010000L
/* TRUE - if this is an aggregate and at least one expression is found in the
** the target list */
#define                 OPS_LAENUM	    0x00020000L
/* TRUE - if new lookahead join enumeration technique is being used */
#define			OPS_FLSQL	    0x00040000L
/* TRUE - if this is a function aggregate subquery created from a subselect 
** via some form of corelation */
#define                 OPS_TIDVIEW	    0x00080000L
/* TRUE - if at least one relation in this query is a view which requires
** duplicates to be kept */
#define                 OPS_SUBVIEW	    0x00100000L
/* TRUE - if list of union view partitions need to be traversed and any group
** of PST_NODUP partition need to be extracted and made a subview in which
** duplicates are to be removed */
#define                 OPS_VOPT	    0x00200000L
/* TRUE - if a variable was eliminated from a flattened query since it was
** a self-join, and the underlying datatype of the target list of the aggregate
** temporary could cause overflows, or NULL errors */
#define                 OPS_IDIOT_NOWHERE   0x00400000L
/* TRUE - if this subquery contains multiple tables, but no where clause. This would
** result in a cartesian product, and any tree shape or leaf node assignment would 
** cost the same - so we just pick the first plan. Implemented for an idiot query 
** from QA. */
#define                 OPS_IEMAPS          0x00800000L
/* TRUE - if this subquery has initialized maps related to unique DMF keys */
#define                 OPS_AGGNEWVAR       0x01000000L
/* TRUE - if this subquery is evaluated concurrently with another subquery
** within the same parent, but needs a new range variable */
#define                 OPS_CPFOUND         0x02000000L
/* TRUE - if the best plan so far is a cartprod */
#define                 OPS_CLEARNULL       0x04000000L
/* TRUE - if resdom node needs to be made nullable since nulls exist in
** the resdom expression */
#define                 OPS_IJCHECK         0x08000000L
/* TRUE - if inner join ojids exist which require special handling of the
** boolean factor placement maps */
#define			OPS_SPATF	    0x10000000L
/* TRUE - a possible spatial join (using Rtree access) is present */
#define			OPS_SPATJ	    0x20000000L
/* TRUE - a possible spatial join (using Rtree access) is present */
#define			OPS_NONCNF	    0x40000000L
/* TRUE - Boolean factors were NOT transformed to conjunctive normal form */
#define			OPS_NEWBESTFRAG	    0x80000000L
/* TRUE - opn_ceval call produced new best fragment cost for greedy 
** enumeration heuristic. */
    u_i4	    ops_mask2;		/* more booleans used to define various
					** conditions for this subquery */
#define                 OPS_COLLATION	    0x00000001L
/* TRUE - collation sequence present - add joinop attribtues for character
** datatype resdoms */
#define			OPS_HINTPASS	    0x00000002L
/* TRUE - we're in hint processing pass of enumeration. If plan is found,
** it is taken with no further enumeration. Otherwise, flag is extinguished 
** and another enumeration pass is made. */
#define			OPS_IXHINTS	    0x00000004L
/* TRUE - there is at least one index hint in this subquery. */
#define			OPS_KJHINTS	    0x00000008L
/* TRUE - there is at least one Kjoin hint in this subquery. */
#define			OPS_NKJHINTS	    0x00000010L
/* TRUE - there is at least one non-Kjoin join hint in this subquery. */
#define			OPS_ORDHINT	    0x00000020L
/* TRUE - if the PST_HINT_ORDER flag is on. */
#define			OPS_GSETS	    0x00000040L
/* TRUE - if the subquery has a GROUP BY with grouping sets. */
#define			OPS_ROLLUP	    0x00000080L
/* TRUE - if the subquery has a GROUP BY with a ROLLUP list. */
#define			OPS_VAREQVAR	    0x00000100L
/* TRUE - processing an ADI_EQ_OP PST_BOP node with PST_VAR nodes left/right */
#define			OPS_AGGCOST	    0x00000200L
/* TRUE - if aggregate result cost estimate produced by opn_modcost(). */
#define			OPS_TPROC	    0x00000400L
/* TRUE - if subquery references a table proc */
    i4         ops_tsubquery;      /* ID of subquery plan */
    i4         ops_tplan;          /* ID of best CO plan */
    i4         ops_tcurrent;       /* ID of current CO plan */
    u_i4       ops_currfragment;   /* # of the current fragment */
    u_i4       ops_bestfragment;   /* # of the best fragment (so far) */
    i4		    ops_gcount;		/* Partition group count for PC
					** aggregation; for size estimating
					** and qep printing
					*/
    bool	    ops_timeout;        /* TRUE - if enumeration timed out
					** to find best plan otherwise FALSE */

    bool            ops_vmflag;         /* TRUE if varmap associated with
                                        ** subquery is valid.  This flag is
                                        ** reset every time some modification
                                        ** is made in the query tree.  It is
                                        ** used to avoid unnecessary traversals
                                        ** of the query tree
                                        */
    bool            ops_normal;         /* TRUE if subquery qualification is
                                        ** normalized */
    bool            ops_first;          /* TRUE - if processing can stop on the
                                        ** first tuple returned from this
                                        ** subquery, ... used for processing
                                        ** simple aggregates like MIN which can
                                        ** use a BTREE ordering and ANY which
                                        ** can stop after the first tuple found
                                        */
    bool	    ops_lastlaloop;	/* TRUE - if new lookahead enumeration is
					** being used and this is the last loop.
					** This means the "bestco" plan really
					** is the final plan */
    bool	    ops_lawholetree;	/* TRUE - if current subtree must be
					** saved entirely, not just the left 
					** subtree of it */
    bool	    ops_bestwholetree;	/* TRUE - if the current ops_bestfragco
					** was a ops_lawholetree */
    OPV_BMVARS      *ops_keep_dups;     /* ptr to a bit map of the tables
                                        ** which require duplicates kept
                                        ** but do not have TIDs available
                                        ** for this purpose, moreover
                                        ** duplicates for the entire query
                                        ** are to be removed, e.g.
                                        ** views used with aggregates, or
                                        ** distribute views,
                                        ** - NULL if this is not a concern
                                        ** for this subquery */
    OPV_BMVARS      *ops_remove_dups;   /* ptr to bitmap of tables requiring
                                        ** duplicates to be removed, this is
                                        ** only defined if ops_keep_dups is
                                        ** defined, and NULL otherwise */
    i4		    ops_pcount;		/* number of non-connected partitions
					** involving relations referenced in
					** ops_remove_dups */
    OPA_ASUBQUERY   ops_agg;           /* structure containing variables
                                        ** used only during aggregate
                                        ** analysis phase
                                        ** - these variables are NOT
                                        ** visible to other phases
                                        */
    OPJ_SUBQUERY    ops_joinop;         /* structure containing variables
                                        ** used only during joinop
                                        ** analysis phase
                                        ** - these variables are NOT
                                        ** visible to other phases
                                        */
    OPC_SUBQUERY    ops_compile;	/* structure containing variables
                                        ** used only during query compilation
                                        ** phase
                                        ** - these variables are NOT visible
                                        ** to other phases
                                        */
    i4		    ops_hintcount;	/* count of table hints
					*/
    OPS_TABHINT	    *ops_hints;		/* ptr to array of table hints (or
					** NULL)
					*/


/*      The following structures are visible to all phases of optimization.
**      The structures have been broken into distinct groupings of major 
**      related functionality.  The utilities which initialize and otherwise
**      operate on these structures will also be separated into related groups
**      so that an "abstraction" of these structures is achieved to some degree.
**
**      The previous 4.0 global variables contained arrays of pointers to 
**      objects.  These arrays of pointers were used so that memory would be 
**      allocated to only those slots which were needed.  However, these arrays
**      of pointers had some arbitrary limits which should be eliminated.  This
**      will be the eventual goal; i.e. have structures so that there will 
**      not be any limits in the optimizer except for time and memory.
**      Currently, the initialization of the optimizer structures in some cases
**      deals with the maximum number of entries and in others it deals exactly
**      with the entries which are used, and in other cases it ignores
**      the limit since it is assumed large enough to handle all cases. 
**      The goal of eliminating the internal limits will force a consistent
**      scheme to be used.  This approach would have the advantage of
**      eliminating an arbitrary (hard to document) limit on the queries
**      to be optimized.
** 
**      It will not be possible to eliminate the arbitrary limits due to the
**      time constraints of the initial phase.  The current phase
**      will set up the structures in the anticipation of this, but will still
**      allocate the maximum number of elements.  This approach will reduce
**      the risk since only editing changes are needed which are required
**      in any case.  The idea is to have a pointer to the base of an array 
**      of pointers or bits, where only the necessary number of elements is 
**      allocated.  
*/
    OPV_RANGEV      ops_vars;           /* local range table 
                                        ** - each element will contain info
                                        ** about a table or useful index
                                        ** (since indexes are modelled as
                                        ** a table)
                                        */
    OPZ_ATSTATE     ops_attrs;          /* information about attributes used
                                        ** in this subquery, each attribute will
                                        ** will reference ( in a many to one
                                        ** map) an element of the range table
                                        ** ops_vars.
                                        */
    OPE_EQUIVALENCE ops_eclass;         /* structure defining the equivalence
                                        ** class partitioning of the attributes
                                        ** used in this subquery
                                        */
    OPB_BFSTATE     ops_bfs;            /* information about boolean factors
                                        ** used in the subquery.
                                        */
    OPZ_SFUNCTION   ops_funcs;          /* information about function attributes
                                        ** used in the query
                                        */
    OPO_STSTATE     ops_msort;          /* information about multi-attribute
                                        ** sorts */
    OPD_SUBDIST     ops_dist;           /* distributed optimizer information which
                                        ** is local to this subquery */
    struct _OPS_SUBQUERY *ops_onext;	/* this linked list of subqueries forms
					** the original list, as opposed to the
					** list given to OPC in ops_next */
    OPG_SGATEWAY    ops_gateway;	/* gateway optimizer information which is
					** local to this subquery */
    OPU_UNION	    ops_sunion;		/* info used to optimize unions, and
					** union views */
    OPE_BMEQCLS	    *ops_tonly;		/* if top sort node removal is a possibility
					** then this is the set of equivalence classes
					** which belong to the relations which are
					** required to have duplicates kept */
    OPL_OJOIN	    ops_oj;		/* structure defining outer join operators
					** related to a particular group ID */
    OPB_BOOLFACT    *ops_seqctest;      /* list of special eqc tests used to
                                        ** implement correct null handling while
                                        ** still having the ability to do key
                                        ** joins */
#ifdef OPM_FLATAGGON
    OPM_FADESCRIPTOR ops_flat;		/* structure defining flattened aggregates
					** which will be evaluated within this
					** subquery
					*/
#endif
}   OPS_SUBQUERY;
