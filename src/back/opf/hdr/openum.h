/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: OPENUM.H - structures used during the enumeration phase
**
** Description:
**      This structures are needed by the enumeration phase but not needed
**      by joinop.
**
** History: 
**      19-mar-86 (seputis)
**          initial creation
[@history_line@]...
**/

/*}
** Name: OPN_STATUS - result status of certain function calls
**
** Description:
**	This type represents result values passed back from a variety 
**	of functions during OPF enumeration processing. Returning these
**	values replaces the need of several instances of EXsignal, which
**	has proven to be very expensive in some operating systems.
**
** History:
**	17-nov-99 (inkdo01)
**	    Written.
[@history_line@]...
*/
typedef i4 OPN_STATUS;
#define OPN_SIGOK		0
#define OPN_SIGSEARCHSPACE	1
#define OPN_SIGEXIT		2

/*}
** Name: OPN_CHILD - child number of joinop tree
**
** Description:
**      This type is used to represent a child of a node of join tree.
**      The children are number from left to right so that for binary
**      trees this means "0" represent the left node and "1" represents
**      the right node.
**
** History:
**     9-jun-86 (seputis)
**          initial creation
[@history_line@]...
*/
typedef i4  OPN_CHILD;

/*}
** Name: OPN_JEQCOUNT - number of joining equivalence classes
**
** Description:
**	The type contains the number of joining equivalence class found
**      in a particular subtree.  A value >0 indicates more than one
**      joining equivalence class.
**
** History:
**      21-may-86 (seputis)
**          initial creation
[@history_line@]...
*/
typedef i4  OPN_JEQCOUNT;
#define                 OPN_NJEQCLS	((OPN_JEQCOUNT) -1)
/* no joining equivalence classes in this subtree */
#define                 OPN_1JEQCLS     ((OPN_JEQCOUNT) 0)
/* one joining equivalence class found in this subtree */


/*}
** Name: OPN_DIFF - measure of how different two datatypes are
**
** Description:
**      This typedef is used to represent how different two histogram numbers
**      of arbitrary type are.  See routine opn_diff for more description.
**
** History:
**      25-may-86 (seputis)
**          initial creation
[@history_line@]...
*/
typedef f8 OPN_DIFF;

/*}
** Name: OPN_SRESULT - result of a search of the previous work
**
** Description:
**      This typedef represents the result of a search of the enumeration
**      structures which contain intermediate calculations from previous
**      enumerations.
**
** History:
**      21-may-86 (seputis)
**          initial creation
[@history_line@]...
*/
typedef i4  OPN_SRESULT;
#define                 OPN_DST         (OPN_SRESULT)3
/* indicates that the subtree requested was deleted since it was more expensive
** than the current query plan */
#define                 OPN_ORSTEQRLS   (OPN_SRESULT)2
/* return the OPN_SUBTREE, OPN_EQS, and OPN_RLS structures and the ordering
** i.e. everything has been calculated
*/
#define                 OPN_STEQRLS     (OPN_SRESULT)1
/* return the OPN_SUBTREE, OPN_EQS, and OPN_RLS structures but the ordering
** has not been considered in the OPN_SUBTREE structure
*/
#define                 OPN_EQRLS       ((OPN_SRESULT)0)
/* return the OPN_RLS structure and the OPN_EQS structure */
#define                 OPN_RLSONLY     ((OPN_SRESULT)-1)
/* return the OPN_RLS structure only */
#define                 OPN_NONE        ((OPN_SRESULT)-2)
/* this set of relations has not been tried yet */

/*}
** Name: OPN_SUBTREE - binds relations, tree structure and costs
**
** Description:
**      This structure is part of a linked list of subtrees.  The
**      set of relations attached to the leaves of each subtree in the
**      list is the same.  The subtree structure may vary between elements
**      in this list, or the relations may be bound to the leaves of the
**      subtree in a different order.
**
** History:
**     19-mar-86 (seputis)
**          initial creation
**	 5-mar-01 (inkdo01 & hayke02)
**	    Added opn_reltups, opn_histogram to show that intermediate histos
**	    and join row counts belong to specific tree shapes/leaf node
**	    assignments. The  values are kept here until a useful
**	    RLS/EQS/SUBTREE combo is found, at which time they're copied
**	    back to the RLS. This change fixes bug 103858.
[@history_line@]...
*/
typedef struct _OPN_SUBTREE
{
    /* the following two elements are positionally dependent - do not insert
    ** anything ahead of or in between opn_coback and opn_coforw.  These
    ** structures must have the same relative offset within this typedef
    ** as the corresponding elements in OPO_CO 
    ** FIXME - this dependency should be removed
    */
    OPO_CO          *opn_coforw;        /* backward pointer of doubly linked
                                        ** list
                                        ** - list terminated by pointing to 
                                        ** coercion of itself i.e. (OPO_CO *)
                                        ** of OPN_SUBTREE struct
                                        */
    OPO_CO          *opn_coback;        /* doubly linked list for the cost
                                        ** orderings for this subtree
                                        ** - list terminated by pointing to 
                                        ** coercion of itself i.e. (OPO_CO *)
                                        ** of OPN_SUBTREE struct
                                        */
    struct _OPN_SUBTREE *opn_stnext;    /* next subtree in list */
    OPN_STLEAVES    opn_relasgn;	/* the relations in order as they
                                        ** have been assigned to this subtree
                                        ** - uses joinop range variable numbers
                                        */
    OPN_TDSC        opn_structure;	/* the structure (in a format
                                        ** used by enum's opn_makejtree) in this
                                        ** subtree.
                                        ** - list is null terminated
                                        */
    struct _OPN_EQS *opn_eqsp;		/* parent OPN_EQS structure which
					** contains this node */
    struct _OPN_SUBTREE *opn_aggnext;	/* circular list of subtrees which
					** have same tree shape and relation
					** assignment but have different aggregate
					** evaluation, equivalence classes evaluated
					** and/or outer join filters defined */
    OPH_HISTOGRAM	*opn_histogram;	/* linked list of normalized
					** histograms for all the
					** equivalence classes which might
					** need them for joining or
					** restrictive predicates.
					** - cell counts in this histogram
					** are percentages of the total
					** number of tuples (opn_reltups)
					*/
    OPO_TUPLES		opn_reltups;	/* number of tuples produced
					** by the join of the relations
					** in the relmap
					*/
}   OPN_SUBTREE;

/*}
** Name: OPN_EQS - subtrees used to join an particular set of relations
**
** Description:
**      This structure is the header for a linked list of join trees.  There
**      will be more than one element in this list if there are indexes in
**      the opn_relmap (relation map) of the opn_rls header structure which
**      points to us.
**
**      Different sets of equivalence classes can be returned if indexes
**      are in the subtree.  For example the tid equivalence class may or may
**      not already be used , and as a result may or may not be in opn_eqcmap.
**
** History:
**     19-mar-86 (seputis)
**          initial creation
**     06-mar-96 (nanpr01)
**	    Use the width consistently with DMT_TBL_ENTRY definition.
[@history_line@]...
*/
typedef struct _OPN_EQS
{
    struct _OPN_EQS *opn_eqnext;         /* next opn_eqs header in list which
                                         ** has same set of relations but differ
                                         ** in the equivalence classes returned
                                         */
    OPN_SUBTREE     *opn_subtree;        /* first of a linked list of subtrees
                                         ** which differ in relation order or
                                         ** structure
                                         */
    OPS_WIDTH       opn_relwid;          /* sum of width in bytes of attributes
                                         ** in the result relation
                                         */
    /* the cost of a sort is dependent on the number of tuples and the tuple
    ** width, which can be determined by this structure, i.e. using opn_relwid
    ** the tuple count in OPN_RLS structure */
    OPO_CPU         opn_cpu;		 /* measure of cpu activity for a
                                         ** sort node placed on top of
                                         ** intermediate relations in this
                                         ** set of subtrees
                                         */
    OPO_BLOCKS      opn_dio;             /* measure of disk i/o activity for
                                         ** a sort node placed on top of this
                                         ** set of relations.
                                         */
    OPO_COMAPS	    opn_maps;		 /* replaces opn_eqcmap and opn_aggmap */
				    	 /* bit map of equivalence 
                                         ** classes to be returned in the 
                                         ** "temporary" result relation
                                         ** represented by the opn_subtree
                                         ** list; i.e. all trees in the list
                                         ** return exactly this set of
                                         ** equivalence classes
                                         */
					 /* map of aggregates which have
					 ** been evaluated in the OPN_SUBTREEs
					 ** associated with this structure */
}   OPN_EQS;

/*}
** Name: OPN_RLS - set of relations to be joined
**
** Description:
**      This is the header structure of a unique set of relations to be 
**      assigned to a subtree.  The number of leaves in the subtree equals 
**      the number of elements in the unique set of relations.
**
** History:
**     20-mar-86 (seputis)
**          initial creation
**      21-feb-91 (seputis)
**          - added opn_boolfact to help make more accurate cost estimate
**          for existence_only check
**	2-feb-93 (ed)
**	    - add mask field to describe possibility of uniqueness in DMF
**	    bug 47377
**	7-mar-96 (inkdo01)
**	    Add cumulative selectivity of this "relation"s predicates.
[@history_line@]...
*/
typedef struct _OPN_RLS
{
    struct _OPN_RLS *opn_rlnext;        /* -next unique set of relations to
                                        ** be assigned to a subtree with nl
                                        ** leaves where Savearr[nl] points
                                        ** to the first rls struct in this
                                        ** list.
                                        */
    OPN_EQS         *opn_eqp;           /* pointer to first opn_eqs structure
                                        ** - only one if no indexes in relation
                                        ** map
                                        */
    OPV_BMVARS      opn_relmap;         /* map of relations (nl of them) which
                                        ** are assigned to the leaves of the
                                        ** subtrees eminating (indirectly)
                                        ** from opn_eqp
                                        */
    OPH_HISTOGRAM    *opn_histogram;    /* linked list of normalized
                                        ** histograms for all the
                                        ** equivalence classes which might
                                        ** need them for joining or
                                        ** restrictive predicates.
                                        ** - cell counts in this histogram
                                        ** are percentages of the total
                                        ** number of tuples (opn_reltups)
                                        */
    OPO_TUPLES      opn_reltups;        /* number of tuples produced
                                        ** by the join of the relations
                                        ** in the relmap
                                        */
    OPN_PERCENT	    opn_relsel;		/* cumulative selectivity of 
					** restriction predicates applicable
					** to this relation.
					*/
    i4		    opn_mask;		/* mask of booleans */
#define                 OPN_UNIQUECHECK 1
/* TRUE - if there is a possibility of DMF supported uniqueness in all
** the relations in this subtree */
    BITFLD          opn_delflag:1;      /* TRUE if this OPN_RLS structure
                                        ** is not being pointed at.
                                        ** - this subtree and OPO_CO
                                        ** structures underneath it may be
                                        ** deleted, but this may cause speed
                                        ** degradation because the deleted
                                        ** data may have to be recalculated
                                        */
    BITFLD          opn_check:1;	/* TRUE if deletion of any sub-structure
                                        ** should be checked */
    BITFLD          opn_boolfact:1;     /* TRUE - if at least one boolean
                                        ** factor is applied for this set
                                        ** of relations */
}   OPN_RLS;

/*}
** Name: OPN_ST - array of pointer to subtrees with cost orderings
**
** Description:
**      An array of pointers to subtree lists.  The N th element of this
**      array points indirectly to all subtrees with N elements.  Work
**      which has been previously computed is saved here; i.e. a subtree
**      with fewer leaves will have been computed earlier.
**
** History:
**     20-mar-86 (seputis)
**          initial creation
**      20-mar-90 (seputis)
**          added opn_ojid to support outer joins
**     17-aug-90 (seputis)
**          add 1 since 30 way join will index out of range of this array
**          b32435
**      11-may-91 (seputis)
**          add support for b37233 fix
[@history_line@]...
*/
typedef struct _OPN_ST
{
    OPN_RLS         *opn_savework[OPN_MAXLEAF+1]; /* pointers to subtrees.
                                         ** - savework[nl] points (indirectly)
                                         ** to all subtrees with nl leaves
                                         */
}   OPN_ST;

/*}
** Name: OPN_JTREE - subtree structure descriptor
**
** Description:
**      This structure is used to keep track of the structural unique subtrees
**      which have been generated during the enumeration procedures.  Each time
**      a structural unique tree has been generated, it is passed to the cost 
**      ordering procedures which produces a CO tree.
**
**      This structure was initially designed to support n-ary trees but joinop
**      will only use binary trees.  As a result, this structure could be
**      simplified considerably (as well as some associated routines).
**
** History:
**     20-mar-86 (seputis)
**          initial creation
**      20-mar-90 (seputis)
**          added opn_ojid to support outer joins
**	26-mar-94 (ed)
**	    bug 49814 E_OP0397 consistency check
**	16-sep-03 (hayke02)
**	    Added OPN_OJINNERIDX to opn_jmask to indicate that an outer TID
**	    join has an outer join further down the query tree involving
**	    a coverqual inner index. We need to check for placement of
**	    this inner index outer join in opn_jmaps().
[@history_line@]...
[@history_template@]...
*/
typedef struct _OPN_JTREE
{
    struct _OPN_JTREE *opn_next;        /* linked list of all OPN_JTREE
                                        ** structures created
                                        */
    struct _OPN_JTREE *opn_child[OPN_MAXDEGREE];/* pointers to all the 
                                        ** subtrees 
                                        */
#define                 OPN_LEFT        0
/* represents the left child of this subtree */
#define                 OPN_RIGHT       1
/* represents the right child of this subtree */

    OPN_LEAVES      opn_nleaves;        /* number of leaves in this subtree */
    OPN_CHILD       opn_nchild;         /* number of subtrees eminating from
                                        ** this node (values 0,1,2) 
                                        */
    OPN_CHILD       opn_npart;          /* number of unique subtrees (or 
                                        ** children) eminating
                                        ** from this node (values 1,2)
                                        */
    OPN_STLEAVES    opn_pra;		/* indexes into Rangev array, current
                                        ** partitioning of relations among the
                                        ** partitions eminating from this node.
                                        ** each partition corresponds to all
                                        ** subtrees with the same structure 
                                        */
    OPN_STLEAVES    opn_prb;		/* indexes into Rangev array, current
                                        ** partitioning of relations among the
                                        ** subtrees
                                        ** - looks like this array is the same
                                        ** as opn_rlasg ... could it be
                                        ** eliminated?
                                        */
    OPN_ISTLEAVES   opn_pi[OPN_MAXDEGREE]; /* partition index into opn_pra, 
                                        ** and opn_prb
                                        ** - start of relations belonging to 
                                        ** i'th subtree is opn_pi[i-1] 
                                        ** - opn_pi[0] = 0 always
                                        */
    OPN_PARTSZ      opn_csz;  		/* number of relations belonging to
                                        ** i'th child subtree 
                                        */
    OPN_PARTSZ      opn_psz;  		/* number of relations in the i'th
                                        ** partition, there may be fewer
                                        ** partitions than subtrees because
                                        ** identically structured subtrees
                                        ** belong to the same partition 
                                        */
    OPN_CHILD	    opn_cpeqc[OPN_MAXDEGREE];/* number of subtrees (or children)
                                        ** in this partition - (0,1,2 
                                        ** for binary trees)
                                        */
    OPE_BMEQCLS	    opn_eqm;		/* eqm,rlasg and rlmap are reset
                                        ** for every assignment of relations to
                                        ** this tree (opn_arl) 
                                        ** - provides equivalence classes made
                                        ** available by all relations attached
                                        ** to the subtree of this node
                                        */
    OPN_STLEAVES    opn_rlasg;		/* order in which relations in 
                                        ** opn_rlmap are assigned to leaves
                                        ** - e.g. a leaf node would contain one
                                        ** element
                                        ** - FIXME - same as opn_prb
                                        */
    OPN_TDSC	    opn_sbstruct;	/* descriptor of subtree
                                        ** which we are currently generating
                                        ** - the description is terminated
                                        ** by a 0
                                        */
/* - the tree description is different from that found in opn_tdsc
** - this array contains descriptions of the subtree not including this node
** - if the same example given in OPN_SUNIQUE is used then the description
** for the root node here would be (3,1,2,1,1,4,2,1,1,2,1,1,0) which would
** represent nodes (C,E,D,G,F,B,I,K,J,H,M,L) terminated by null
*/
    OPV_BMVARS	    opn_rlmap;		/* bitmap of joinop range variables
                                        ** representing relations in the subtree
                                        */
    OPV_BMVARS      opn_tidplace;       /* bitmap of joinop range variables
                                        ** representing relations in the subtree
                                        */
#ifdef OPM_FLATAGGON
    OPM_BMAGGS	    opn_aggmap;		/* map of aggregate ID's which can be evaluated
					** at this node */
#endif
    OPL_IOUTER	    opn_ojid;		/* outer join ID which should be evaluated
					** at this node, due to variable positioning
					** - since an OPC restriction is that only
					** one join ID per CO node, and to the OPF
					** search space restriction of placement of
					** inner relations, this can be fixed
					** when a partition is selected for the
					** structurally unique tree */
    OPL_BMOJ	    opn_ojinnermap;	/* map of outer joins which are evaluated
					** in this subtree, except at this node
					** (but including any partially evaluated
					** outer join ) */
    OPL_BMOJ	    opn_ojevalmap;	/* map of outer joins which are entirely
					** evaluated at this node, i.e. no parent
					** has an outer join id which is in this
					** map */
    i4		    opn_jmask;		/* mask of booleans describing state */
#define                 OPN_JSUCCESS    1
/* At least one successful partitioning was found at this node,... in the
** case of b37233, tree shapes need to be skipped quickly if numerous
** secondary indexes are used and placement restrictions occur due to tree
** shape,... if there is a successful partition at this node which does
** not produce at least one query plan then chances are the tree shape is
** poor for this set of relations */
#define                 OPN_JREACHED     2
/* TRUE - if the parent had successfully passed the hueristics and given a
** partition to the child */
#define                 OPN_JFORKS      4
/* TRUE - if a balanced tree is required for index placement */
#define                 OPN_AGGEVAL     32
/* TRUE - if at least one aggregate can be evaluated at this
** node */
#define			OPN_FULLOJ	64
/* TRUE - if this node has full join semantics, and is not a tid join above
** a full join with a full join id */
#define			OPN_OJINNERIDX	128
}   OPN_JTREE;

/*}
** Name: OPN_EVAR - bottom up join order range table entry descriptor
**
** Description:
**      Structure (allocated in array) that describes a range table variable
**	as manipulated by bottom up lookahead join order heuristic. Each variable
**	is either a composite (the join of other tables in the query) or an
**	actual table/index from the subquery range table. As joins are composed
**	during the enumeration process, the constituent table/index variables
**	are replaced by the resulting composites.
**
** History:
**	6-sep-02 (inkdo01)
**	    Written.
**	29-oct-02 (inkdo01)
**	    Added opn_primmap for OJ permutation heuristic.
**	12-nov-02 (inkdo01)
**	    Changed CV flags to EV and added OPN_EVDONE for OJ stuff.
**	10-feb-03 (inkdo01)
**	    Add map of EQCs for subselect results that mustn't be copied
**	    to join result.
[@history_line@]...
*/
typedef struct _OPN_EVAR
{
    union {
	struct {
	    OPV_VARS	*opn_varp;	/* this entry is a sq range table entry */
	    OPV_IVARS	opn_varnum;	/* its index in sq range table */
	    OPV_BMVARS	opn_ixmap;	/* indexes that can replace this primary,
					** or primary replaced by this index */
	} v;
	OPO_CO		*opn_cplan;	/* for composites: ptr to root of
					** plan fragment */
    } u;
    OPV_BMVARS	    opn_joinable;	/* cumulative joinable bit map for 
					** constituent tables */
    OPV_BMVARS	    opn_varmap;		/* map of sq range table entries in 
					** this enum range table entry */
    OPV_BMVARS	    opn_primmap;	/* map of primary sq range table 
					** entries corresponding to this 
					** enum range table entry */
    OPE_BMEQCLS	    opn_eqcmap;		/* map of eqc's found in this enum
					** range table entry */
    OPE_BMEQCLS	    opn_snumeqcmap;	/* map of eqc's of subselect results
					** - not to be copied to join result */
#define OPN_EVTABLE	0x01	/* this entry is a sq range table entry */
#define OPN_EVCOMP	0x02	/* this entry is a join composite */
#define OPN_EVDONE	0x04	/* this entry has been collapsed */
    i4		    opn_evmask;		/* flag */
}   OPN_EVAR;

/*}
** Name: OPO_STATS - statistics for a particular set of equivalence classes
**
** Description:
**      Every set of equivalence classes will have different uniqueness, repitition
**	factor, this structure is used to describe these numbers for each set.
**
** History:
**      8-aug-89 (seputis)
**          initial creation
[@history_template@]...
*/
typedef struct _OPN_SDESC
{
    struct _OPN_SDESC	*mas_masterp;	/* read only list of ptrs used for
					** traversing unallocated OPN_SDESC
					** descriptors */
    struct _OPN_SDESC	*mas_next;	/* ptr to next structure of
					** statistics for a set of
					** equivalence classes */
    OPO_ISORT		mas_sorti;	/* ID of sort descriptor */
    bool		mas_unique;     /* TRUE - if all the 
					** equivalence classes from the
					** sort descriptor form a unique key */
    OPO_TUPLES		mas_reptf;      /* multi-attribute repetition factor
					** for equivalence classes in sort
					** descriptor */
    OPH_DOMAIN		mas_nunique;    /* number of unique values for
					** equivalence classes in sort
					** descriptor */
} OPN_SDESC;

/*}
** Name: OPN_ENUM - Joinop enumeration header structure
**
** Description:
**      This structure defines the header structure used during the enumeration
**      phase of optimization.  This structure is contained in global state
**      variable so that it will be reused by each subsequent subquery
**      enumeration.  However, there are no variables in this structure
**      which persist between enumerations; i.e. no info from the inner
**      aggregate enumeration (within this structure) is used by an
**      outer aggregate enumeration.  Information that will persist
**      is contained in the subquery structure; i.e. the best CO, and histogram
**      and statistics of the temporary relation.
**
** History:
**     19-mar-86 (seputis)
**          initial creation
**	16-may-90 (seputis)
**	    - b21582 - create counter for timeout
**	    - move variables which belong in enumeration phase
**	06-mar-96 (nanpr01)
**	    - Variable page size project. For multiple buffer
**	      pools now we have multiple scan factors.
**	3-nov-00 (inkdo01)
**	    Reduced OPN_CSTIMEOUT to 10 to increase timeslice cycle.
**	30-oct-04 (inkdo01)
**	    Added opn_fragcolist for greedy enumeration.
[@history_line@]...
*/
typedef struct _OPN_ESTATE
{
    OPN_JTREE       *opn_sroot;         /* root of "structurally unique"
                                        ** join tree begin analyzed
                                        ** - this also points to the beginning
                                        ** of a linked list of OPN_JTREE structs
                                        ** of 2n-1 elements, where n is the
                                        ** number of relations in the joinop
                                        ** range table... this is the max number
                                        ** of nodes required for optimization
                                        ** of a query with n relations
                                        */
    OPN_ST          *opn_sbase;         /* base of array of pointers to subtrees
                                        ** with associated costs
                                        */
    bool	    opn_statistics;     /* TRUE - if the cpu accounting has been
					** turned on */
    bool	    opn_reset_statistics; /* TRUE - if on exit from OPF, cpu
					** statistics should be turned off since
					** it is too expensive */
    OPO_BLOCKS      opn_maxcache[DB_NO_OF_POOL];       
					/* 0.0 means uninitalized,
                                        ** > 0.0 means number of disk blocks
                                        ** in the DMF cache */
    OPO_BLOCKS      opn_sblock[DB_NO_OF_POOL];		
					/* the default sort blocking factor
                                        ** used by DMF */
    i4         opn_startime;       /* millisecs used by process when
                                        ** enumeration phase has begun
                                        ** - to see if we should stop the search
                                        */
    bool            opn_rootflg;        /* TRUE if working on the root
                                        ** of the join operator tree 
                                        */
    bool            opn_slowflg;        /* we have had to delete intermediate
                                        ** results which may later have to be
                                        ** recalculated 
                                        */
    bool            opn_singlevar;      /* TRUE if only one range variable
                                        ** referenced in query
                                        */
    /* the following elements are free space management components which
    ** should be eliminated when joinop's freespace manager gets integrated
    ** into ULM
    */
    PTR             opn_streamid;       /* this is the memory stream that
                                        ** all enumeration data structures
                                        ** will be allocated from - this will
                                        ** be allocated when enumeration of
                                        ** a subquery has begun and deallocated
                                        ** every time an enumeration has 
                                        ** completed
                                        */
    OPN_SDESC	    *opn_sfree;		/* free list of statistics descriptors
					** used inside opn_calcost */
    OPN_RLS         *opn_rlsfree;       /* free list of rls structures */
    OPN_EQS         *opn_eqsfree;       /* free list of eqs structures */
    OPN_SUBTREE     *opn_stfree;        /* free list of subtree structures */
    OPO_CO          *opn_cofree;        /* free list of co structures */
    OPH_HISTOGRAM   *opn_hfree;         /* free list of histogram structures*/
    OPH_COUNTS      opn_ccfree;         /* this is a free list of cell count
                                        ** arrays associated with this histogram
                                        ** - size equal to "sizeof(f4) *
                                        ** oph_numcells" bytes
                                        ** - this will be used until
                                        ** the free space manager is integrated
                                        ** into the jupiter memory manager
                                        ** (which does not allow deallocation
                                        ** of portions of a "stream")
                                        */
    PTR             opn_cbstreamid;     /* memory stream id which stores info
                                        ** on state of a temporary "cell
                                        ** boundary" array - special case
                                        ** since this may need to be allocated
                                        ** and deallocated several times during
                                        ** an enumeration */
    PTR             opn_cbptr;          /* ptr to free cell boundary element 
                                        ** allocated from opn_cbstreamid */
    i4              opn_cbsize;         /* size in bytes of opn_cbptr */
    bool            opn_search;         /* TRUE - if memory garbage collection
                                        ** routines where executed and a new 
                                        ** best CO tree has not been found for
                                        ** the query (which may free some
                                        ** more memory)
                                        */
    OPN_LEAVES      opn_swlevel;        /* index into the opn_savework array
                                        ** indicating the level at which
                                        ** the last deletion of a useful subtree
                                        ** occurred, decremented by the
                                        ** memory management routines, which
                                        ** will future slow processing
                                        ** - reset to number of leaves each
                                        ** time the number of leaves being
                                        ** considered is increased
                                        */
    OPN_LEAVES      opn_swcurrent;      /* index into the opn_savework array
                                        ** indicating the current level at which
                                        ** all useful subtrees have been
                                        ** evaluated
                                        ** - this value is always <= opn_swlevel
                                        */
    bool            opn_tidrel;         /* TRUE if a temporary relation needs
                                        ** to be created for a replace, or
                                        ** delete, in DEFERRED mode so that
                                        ** the halloween problem does not occur
                                        */
    OPV_BMVARS      (* opn_halloween);  /* ptr to bitmap of variables referenced
                                        ** in the query which may be victimized
                                        ** by reappearing tuples
                                        ** - NULL implies problem will not
                                        ** occur for this query */
    PST_QNODE       *opn_sortnode;      /* ptr to sort node needed for TID
                                        ** attribute in case of halloween
                                        ** problem...  allocated early so no
                                        ** out of memory problems occur */
    PTR             opn_jtreeid;        /* memory stream id which stores 
                                        ** the OPN_JTREE structures 
                                        ** - this is part of enumeration memory
                                        ** but needs to be separated so out-of
                                        ** memory errors can be recovered from
                                        ** - otherwise, it will be handled
                                        ** in the same way as opn_steamid
                                        */
    i4              opn_cocount;        /* number of CO nodes allocated outside
                                        ** of the enumeration memory stream,
                                        ** available for copying, prior to
                                        ** deleting the enumeration memory
                                        ** stream */
    i4              opn_corequired;     /* maximum number of CO nodes possible
                                        ** for one tree, equal to 3 * numleaves
                                        */
    OPO_PERM        *opn_colist;        /* beginning of a CO list located
                                        ** outside of enumeration memory, used
                                        ** to copy the resultant CO tree, prior
                                        ** to deleting the enumeration memory
                                        ** stream */
    OPO_PERM	    *opn_fragcolist;	/* beginning of CO list reserved for
					** caching "best fragment" plans for
					** greedy enumeration */
    OPN_SSTLEAVES   *ops_arlguess;	/* guess as to first best set of relations
					** to attach to leaves */
    OPV_IVARS	    ops_arlcount;	/* number of relations in ops_arlguess */
    i4		    opn_mask;		/* mask of enumeration booleans */
					/* not used yet */
    i4		    opn_cscount;	/* counter used to call CSswitch so that
					** unix can time out */
#define                 OPN_CSTIMEOUT   10
/* used to count number of times opn_arl routine is called before checking for a
** context switch for unix 
*/
}   OPN_ESTATE;
