/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: OPCOTREE.H - Cost ordering node definition
**
** Description:
**      The structures defined in this file define the nodes of a CO
**      (cost ordering) tree.
**
** History:
**      10-mar-86 (seputis)
**          initial creation
**	2-Jun-2009 (kschendel) b122118
**	    Remove unused oj-filter stuff.
[@history_line@]...
**/

/*}
** Name: OPO_PARM - parameters for estimating cost of an operation
**
** Description:
**      This structure isolates the parameters used to evaluate the cost
**      of an operation (i.e. join, project/restrict, sort, reformat).
**      It will eventually include costs for distributed DB purposes also.
**
** History:
**     19-mar-86 (seputis)
**          initial creation
**     06-mar-96 (nanpr01)
**	    variable page size project.
**	13-nov-03 (inkdo01)
**	    Added opo_reduction for exchange nodes in parallel query
**	    processing.
**	13-Nov-2009 (kiria01) SIR 121883
**	    Add opo_card_* to track cardinality check needs.
[@history_line@]...
*/
typedef struct _OPO_PARM
{
    OPO_CPU         opo_cpu;            /* measure of cpu activity
                                        */
    OPO_BLOCKS      opo_dio;            /* cumulative number of disk io's
                                        ** to reach relation herein described
                                        */
    OPO_COST	    opo_reduction;	/* estimated time reduction by 
					** exchange node */
    OPO_BLOCKS      opo_reltotb;        /* total number of pages occupied
                                        ** by relation
                                        */
    union 
    {
	OPO_BLOCKS      opo_relprim;	/* if ORIG node - number of primary pages 
                                        ** -used only if isam or hashed */
	OPO_BLOCKS	opo_cache;	/* if DB_SJ node - number of
                                        ** cached DMF pages in the subtree */
	OPO_BLOCKS	opo_network;	/* - if distributed then in the BEST
                                        ** CO node copied out of enumeration
                                        ** memory, will have an interior
					** node i.e. non-ORIG store the networking
					** costs here */
    }		    opo_cvar;		/* - note overloading is done
					** so that CO structure does not
					** increase in size, FIXME, perhaps
					** the relprim can be stored in the
					** range variable and eliminated
					** entirely...
                                        */
    OPO_BLOCKS      opo_pagestouched;   /* number of disk pages accessed on disk
                                        ** resident relation
                                        ** - used to determine whether to do
                                        ** page level or relation level locking
                                        ** - valid only if this is a project-
                                        ** restrict node, in which case PR node
                                        ** has the number of pages touched for 
                                        ** the child (which is a ORIG node)
                                        ** - OR if this is a join node and the
                                        ** inner is a ORIG node (i.e. keying)
                                        ** then the join node contains the
                                        ** number of pages touched on the inner
                                        ** - note this field is NOT valid for an
                                        ** ORIG node... this was done this way
                                        ** since an ORIG may have multiple 
                                        ** parents in enumeration, some parents
                                        ** may be keyed, or
                                        ** be a project restrict, which means
                                        ** pages touched is different depending
                                        ** on the parent.
                                        */
    OPO_TUPLES      opo_tups;           /* record number of tuples accessed
                                        ** at this node
                                        */
    OPO_TUPLES      opo_adfactor;       /* adjacent duplicate factor
                                        ** - average length of a run of 
                                        ** contiguous tuples
                                        ** with the same value for
                                        ** "ordering equivalence class"
                                        ** i.e. attribute being sorted
                                        ** or indexed on (isam or btree)
                                        */
#define                 OPO_DADFACTOR   ((OPO_TUPLES) 1.1)
/* default adjacent duplicate factor - used in reformat nodes */

    OPO_TUPLES      opo_sortfact;       /* sortedness factor
                                        ** - average length of a run of 
                                        ** of rows which are in order on
                                        ** the ordering equivalence
                                        ** class of the relation 
                                        */
#define                 OPO_DSORTFACT   ((OPO_TUPLES) 3.0)
/* default sortedness factor - used in reformat nodes */

    OPO_TUPLES      opo_tpb;            /* number of tuples per disk block
					** FIXME - remove this and calculate
					** dynamically so that CO structure can
					** be reduced in size */
    i4	    opo_pagesize;       /* page size of the relation */
}   OPO_PARM;

/*}
** Name: OPO_STORAGE - storage structure used within the optimizer
**
** Description:
**	This structure is identical to the DMF storage structure, but is
**      defined as a typedef for use with lint inside the OPF.  Also the
**      DMF type is i4 which may not be desirable for the CO structure
**      which is memory intensive.
**
** History:
**      20-may-86 (seputis)
**          initial creation
[@history_line@]...
*/
typedef char OPO_STORAGE;

/*}
** Name: OPO_OJEQCDEF - equivalence class handling for outer joins
**
** Description:
**      Divides equivalence classes into orthogonal sets and indicates
**	particular handling during processing of an outer join, the union
**	of these equivalence class maps should produce opo_ordeqc except
**	for non-special function attributes which need to use values
**	generated by info from below to produce the attribute.
**
** History:
**      20-may-93 (ed)
**          initial creation
**      [@dd-mmm-yy (login)@]
[@history_line@]...
[@history_template@]...
*/
typedef struct _OPO_OJEQCDEF
{
    OPE_BMEQCLS		*opo_nulleqcmap; /* if not NULL - map of eqcls which
				    ** which is a subset of opo_ordeqc and
				    ** represents those eqcls which are
				    ** set to NULL if the outer join fails
				    ** - mostly will be eqcls which originate
				    ** from the inner subtree only 
				    ** - rnull eqcls map */
    OPE_BMEQCLS		*opo_refresheqcmap; /* if not NULL - map of eqcls which
				    ** is a subset of opo_ordeqc and represents
				    ** those eqcls which are copied from the
				    ** outer child if the outer join fails
				    ** - mostly will be eqcls which origingate
				    ** from the outer subtree */
    OPE_BMEQCLS		*opo_zeroeqcmap; /* if not NULL - map of eqcls which
				    ** are integers and will get set to
				    ** 0 if the outer join fails
				    ** - represents special eqcls */
    OPE_BMEQCLS		*opo_oneeqcmap; /* if not NULL - map of eqcls which
				    ** are integers and will get set to 
				    ** 1 if the outer join fails
				    ** - represents special eqcls */
}   OPO_OJEQCDEF;

/*}
** Name: OPD_LOCAL   - optimizer info in CO node for OPC
**
** Description:
**      Contains info which will not be needed during the enumeration
**	phase, but is needed by OPC to process the CO node correctly
**	This structure is attached to the CO node on a final pass
**	after the enumeration phase has completed.
**
** History:
**      4-apr-88 (seputis)
**          initial creation
**      20-mar-90 (seputis)
**          added opo_type, opo_ojid, opo_ojoin, opo_jtype, opo_mask
**	    for outer join support
**      12-aug-91 (seputis)
**          - add opo_fmask, and opo_lang for STAR support of LIKE expressions
**	27-aug-93 (ed)
**	    -added filter descriptor used to correct secondary index 
**	    performance problem with outer joins
**	17-jul-98 (inkdo01)
**	    Add flag OPO_READBACK for descending sort support.
**	14-apr-04 (inkdo01)
**	    Add OPV_PCJDESC for partition compatible joins.
**      20-jul-2006 (huazh01)
**          Add OPO_HASH_TRANSFORM. True is the jtype for a child's outer  
**          node has been changed from a FSM join to a hash join. 
**          This fixes b116402. 
**	10-Apr-2007 (kschendel) SIR 122513
**	    Add nested-PCjoin flag.
**	27-Aug-2007 (kschendel) SIR 122513
**	    Add partition-qualifiable bit map for determining where to
**	    look for join time partition qualification.
**	2-Jun-2009 (kschendel) b122118
**	    Delete NOHOLDFILE and NOUNDEFINED flags, set but never tested
**	    anywhere.  Remove fmask, the only flag in it was never set.
[@history_template@]...
*/
typedef struct _OPD_LOCAL
{
    OPB_BMBF	       *opo_bmbf;   /* ptr to boolean factor bitmap
                                    ** to be evaluated at this node */
    OPV_BMVARS	       *opo_bmvars; /* bitmap of variables contained
                                    ** in this subtree, NULL if not
                                    ** needed */
    i4			opo_id;     /* node ID used for query tree
                                    ** printing routines to identify
                                    ** the node in the SET QEP
                                    ** tree being displayed */
    OPD_ISITE		opo_target; /* distributed site upon which to
				    ** place the result, this can be
				    ** different from the operation site
				    ** since it may be more efficient to
				    ** do the operation elsewhere and
				    ** move the result to this target site */
    OPD_ISITE		opo_operation; /* distributed site upon which to
				    ** execute the operation represented by
				    ** this CO node, the cost model in the
				    ** initial phase will assume that both
				    ** intermediates of a join are on the
				    ** same site for any operation */
    DB_LANG		opo_lang;   /* language to use when generating
				    ** this query, eventually there may be
				    ** site constraints for evaluating
				    ** nodes */
    OPL_OJTYPE		opo_type;   /* type of outer join to be applied
				    ** at this CO node, i.e left, right
				    ** full, inner */
    OPL_IOUTER		opo_ojid;   /* ID of the outer join which must be
				    ** evaluated at this node, or OPL_NOOUTER
				    ** if none is applicable */
    OPE_BMEQCLS		*opo_ojoin;  /* This is the set of joining equivalence classes
				    ** which participate in the outer join,
				    ** - any remaining eqcls
				    ** should be treated as an inner join */
    OPO_JTYPE		opo_jtype;  /* this is the join type for the CO node,
				    ** i.e. FSM, PSM, SE, CART-PROD, KEYED, TID */
    i4			opo_mask;   /* mask of various booleans required by
				    ** OPC */
#define		OPO_PCJ_NESTED  1   /* Set if this (SJ) node is partition
				    ** compatible and is nested under another
				    ** PC join/PC agg;  tells opc to turn on
				    ** the part-nested flag in the QP. */

/* #define		notused		2 */
#define                 OPO_NOORDER     4
/* TRUE - if there exists an aggregate to be evaluated at this node
** which requires the existence of a particular ordering from the subtree */
#define			OPO_READBACK	8
/* TRUE - if this is a top sort removed, descending order by query and this is
** the top left-most ORIG node (better be Btree, too!). */

#define                 OPO_HASH_TRANSFORM 16
/* TRUE - if the jtype of current node's outer/inner node has been converted from 
** OPO_SJFSM to OPO_SJHASH
*/

    OPO_OJEQCDEF	opo_ojlnull; /* equivalence class handling describing
				    ** eqcls handling in a left join condition */
    OPO_OJEQCDEF	opo_ojrnull; /* equivalence class handling describing
				    ** eqcls handling in a right join condition */
    OPO_OJEQCDEF	opo_ojinner; /* equivalence class handling describing
				    ** eqcls handling in an inner join condition */
    OPL_IOUTER		opo_expand; 
    OPE_BMEQCLS		*opo_special;
    OPE_BMEQCLS		*opo_innereqc;
#ifdef OPM_FLATAGGON
    struct _OPM_FAGG *opo_fagg;	/* ptr to an ordered linked list of aggregate
				** descriptors to be evaluated at this node */
    OPZ_BMATTS	*opo_facreate;	/* map of "function attributes" which are NOT
				** aggregate results, but instead are other function
				** attributes which need to be created at this
				** node, i.e. conversions, single variable,
				** multi-variable, puesdo-tid. In 6.4, OPC
				** figures out the function attributes which 
				** need to be evaluated at the node by ANDing
				** equivalence classes available at the node
				** and the equivalence classes which are not 
			   	** given by the child nodes. */
    OPE_BMEQCLS *opo_faskip;    /* if not null, ptr to list of equilvalence
                                ** classes which should be used for the
                                ** "SEjoin optimization", i.e. if the outer
                                ** tuple eqcls of the which this map is a subset
                                ** does not change in value for this list
                                ** then do not execute the join but instead
                                ** return the same result as was returned
                                ** for the previous outer tuple after the
                                ** aggregate filters in opo_fagg were
                                ** executed */
    OPE_BMEQCLS *opo_neqany;    /* if not null, then this is a map of equivalence
                                ** classes which is a subset of the joining
                                ** equivalence classes for which NULL=anything
                                ** semantics are to be used,... this feature
                                ** is used to support more efficient access paths
                                ** for !=ALL and NOT IN, when nullable attributes
                                ** are used in the "select conjunct" */
    PST_QNODE   *opo_nullcheck; /* after each tuple compare at this node
                                ** check for the nullability of this expression
                                ** and set all special equivalence classes
                                ** to TRUE if this expression is NULL,
                                ** - part of !=ALL, NOT IN support */
#endif
    struct _OPV_PCJDESC	*opo_pcjdp; /* ptr to description of partition info
				** for partition compatible join or NULL */
    i4		opo_pgcount;	/* count of groups of logical partitions
				** suitable for partition compatible join */

	/* Opo_partvars is a BMVARS bitmap that indicates which tables
	** underneath the current node are partitioned and might be
	** amenable to join-time partition qualification.
	** An orig sets the bit for that table;  a join presents
	** the OR of the inner and outer, except if it's an outer
	** join.  An outer join only presents the outer side (ie
	** left side if a left join, etc), or zeros if it's a full join.
	** The purpose of this restriction is to prevent join-time pruning
	** against a partitioned inner, which must presumably include
	** an IS NULL-type clause (else the join would degenerate to
	** an inner join).  IS NULL looks like an EQUALS to partition
	** qualification, which doesn't know about OJ-introduced nulls.
	*/
    OPV_BMVARS	opo_partvars;	/* Vars interested in join-time part qual */

}   OPD_LOCAL;

/*}
** Name: OPO_EQLIST - defines list of equivalence
**
** Description:
**      This list of equivalence classes defines an ordering which is desired.
**
** History:
**      8-feb-87 (seputis)
**          initial creation
[@history_template@]...
*/
typedef struct _OPO_EQLIST
{
    OPO_ISORT        opo_eqorder[DB_MAXKEYS]; /* variable length array of 
                                        ** equivalence classes, at least
					** two elements, including an
					** OPE_NOEQCLS terminator */
}   OPO_EQLIST;

/*}
** Name: OPO_SORT - structure defining multi-attribute orderings
**
** Description:
**      This is a general
**      mechanism for defining interesting orderings based on set of
**      equivalence classes (rather than individual equivalence classes).
**      This would eventually be used to define multi-attributes joins
**      and sorts to the optimizer cost model.  
**
**      Bitmaps of OPO_ISORT types would be difficult to maintain since
**      the structure will grow during the enumeration phase, but 
**      multi-attribute sorts are designed so that bitmaps of OPO_ISORT
**      is not required.
**
**      There are exact and inexact orderings, the inexact orderings
**      are a simple set of equivalence classes.  The exact orderings
**      can include inexact orderings in the list but can never include
**      another exact ordering in its' opo_eqorder list.
**
**      This structure is used to describe multi-attribute orderings. 
**      Logically, it can be considered an extension of the equivalence
**      class array.  Any OPO_ISORT index which is in range of the equivalence
**      class array will be considered to represent an ordering on that 
**      equivalence class only (single attribute).  Any OPO_ISORT out of
**      range of the equivalence class array will represent one of these
**      multi-attribute ordering structures.  This is done so that
**      bit vectors on "interesting orderings" can be defined.
**
** History:
**      9-feb-87 (seputis)
**          initial creation
[@history_template@]...
*/
typedef struct _OPO_SORT
{
    i4              opo_stype;          /* type of ordering */
#define                 OPO_SEXACT      1
/* an exact ordering is one in which all equivalence class must be available
** and the ordering is defined by the opo_eqlist - this could be used
** by a user specified SORT list, or could represent the
** output of reading a BTREE storage structure */
#define                 OPO_SINEXACT    2
/* - an inexact ordering does not require the equivalence classes to be sorted
** in any particular order, so the order of the bits in the opo_bmeqcls
** will be used as a default - this could be used for ordering attributes
** to be SEJOINed to a subselect
** - subset orderings, interested in any orderings which are subsets of this
** equivalence class map which is naturally available from the subtree, however
** a sort node should be generated which contains all the equivalence classes
** - the opo_eqlist could reference OPO_SORT structures so that subsets
** can be selected which must be ordered, but equivalence classes within
** each subset do not need to be ordered
*/

    OPO_EQLIST      *opo_eqlist;        /* this list defines the exact
                                        ** equivalence class ordering
                                        ** represented by this structure 
                                        ** - only used if a OPO_SEXACT defined
                                        ** - this list is terminated by a
                                        ** OPE_NOEQCLS
                                        ** - this will always be specified for
                                        ** OPC since OPO_SINEXACT orderings
                                        ** are only used inside enumeration,
                                        ** also only equivalence classes
                                        ** will be used for OPC, rather
                                        ** than recursively defining other
                                        ** multi-attribute orderings in this
                                        ** list (as is the case for enumeration)
                                        */
    OPE_BMEQCLS     (*opo_bmeqcls);     /* ptr to bit map of equivalence
                                        ** classes involved in this multi
                                        ** attribute ordering, this will
                                        ** always be specified for any
                                        ** ordering ... exact or inexact */
}   OPO_SORT;

/*}
** Name: OPO_ST - array of ptrs to multi-attribute sort descriptors
**
** Description:
**      ptr to array of ptrs to sort descriptors, note that the initial
**      allocation may not be enough so subsequent references may be out of
**      bounds as the structure is copied after extending it.
**
** History:
**      30-jul-87 (seputis)
**          initial creation
[@history_template@]...
*/
typedef struct _OPO_ST
{
    OPO_SORT        *opo_stable[OPE_MAXEQCLS]; /* array of ptrs to sort 
					** descriptors */
}   OPO_ST;

/*}
** Name: OPO_STSTATE - structure defining multi-attribute sort definitions
**
** Description:
**      This structure defines all the multi-attribute sorts used within this
**      subquery.  
**
** History:
**      30-jul-87 (seputis)
**	    initial creation
**      16-jul-90 (seputis)
**          add fields to assist in removing top sort nodes
**	16-jul-98 (inkdo01)
**	    Add OPO_MDESC in an attempt to support read backwards for 
**	    descending sorts.
**	30-Aug-2006 (kschendel)
**	    Remove redundant opo_topsort boolean.

[@history_template@]...
*/
typedef struct _OPO_STSTATE
{
    OPO_ISORT       opo_sv;             /* number of sort definition defined */
    OPO_ST          *opo_base;          /* base of array of ptrs to multi-attr
                                        ** sort definitions */
    OPO_ISORT       opo_maxsv;          /* number of elements in array of ptrs
                                        ** defined by opo_base, it may be
                                        ** extended if the initial allocation
                                        ** is not enough */
    OPO_EQLIST	    *opo_tsortp;        /* if non NULL then this is a pointer
					** to a multi-attribute ordering which
					** is defined outside enumeration memory
					** and is used to describe an ordering
					** required by the top sort node
					** which is a user specified ordering
					** - top sort node can be avoided if
					** this user specified ordering is
					** provided */
    i4		    opo_mask;		/* mask containing booleans which describe
					** sortedness requirements and states */
#define                 OPO_MREMOVE     1
/* TRUE - if the top sort node can be removed if an appropriate ordering is
** obtained from storage structures */
#define                 OPO_MTOPSORT    2
/* TRUE - if a top sort node is required either for the user, or for duplicate
** semantics, or to cause row prefetching */
#define			OPO_MRDUPS	4
/* TRUE - if opo_rdups should be checked in order to see if a top sort node 
** is required */
#define			OPO_MDESC	8
/* TRUE - if sort order is descending. Under some (limited) circumstances,
** a descending sort can be achieved by reading backwards through the index. */
    OPO_ISORT	    opo_singleeqcls;	/* this is the equivalent of opo_tsortp
					** except it represents a single
					** equivalence class */
    OPV_BMVARS	    opo_rdups;		/* bitmap of variable for which duplicates
					** should be removed, used to check if a
					** top sort node is need, or if the sorting
					** has been done earlier in the QEP */
}   OPO_STSTATE;

/*}
** Name: OPO_CO - cost ordering nodes
**
** Description:
**      This structure describes an estimate of the cost in terms of CPU and 
**      disk I/O of evaluating a particular joinop subtree, with a particular
**      assignment of relations and indexes to the leaves of the tree.  
**      The cost of a joinop subtree is the cost to create the "relation"
**      which would exist at the root of the subtree, after evaluation.
**
** History:
**     19-mar-86 (seputis)
**          initial creation
**      31-jan-92 (seputis)
**          rearrange field names to allow more efficient packing of
**          data for this frequently used structure
**      8-may-92 (seputis)
**          change opo_pointer to i2, since overflow can occur and this
**          can result in overwriting a highly pointed at CO node, and
**          can result in access violations from an unintentioned graft.
**	21-may-93 (ed)
**	    created union for opo_var, opo_ojid so that outer join id
**	    can be stored in CO node without increasing size of CO node
**	10-nov-03 (inkdo01)
**	    Added DB_EXCH for parallel query processing.
**	2-mar-04 (inkdo01)
**	    Added opo_ccount for parallel processing partitioned tables.
**	22-july-04 (inkdo01)
**	    Added opo_punion flag to define parallel unions under exch.
**	29-oct-04 (inkdo01)
**	    Replaced unused opo_memory by opo_held for greedy enumeration
**	    memory management.
**	19-jan-05 (inkdo01)
**	    Added opo_exchagg flag to implement parallel aggregation.
**	5-Dec-2006 (kschendel) SIR 122512
**	    Add unrestricted join ordering for use by hash join.
**	30-may-2008 (dougi)
**	    Added opo_tprocjoin flag for table procedures.
*/
typedef struct _OPO_CO
{
    /* the following two elements are positionally dependent - do not insert
    ** anything ahead of or in between opo_coback and opo_coforw.  These
    ** structures must have the same relative offset within this typedef
    ** as the corresponding elements in OPN_SUBTREE
    ** FIXME - this dependency should be removed
    */
    struct _OPO_CO    *opo_coforw;      /* forward link of doubly linked
                                        ** list of OPO_CO structures for
                                        ** relation
                                        */
    struct _OPO_CO    *opo_coback;      /* backward link of doubly linked
                                        ** list of OPO_CO structures
                                        */
    struct _OPO_CO    *opo_outer;       /* -pointer to "outer" relation in the
                                        ** case of a simple join
                                        ** -or a pointer to the project/restrict
                                        ** or reformat relation from which 
                                        ** this one was created
                                        */
    struct _OPO_CO    *opo_inner;       /* pointer to cost node representing
                                        ** "inner" relation of a simple join
                                        */
    OPO_COMAPS	    (*opo_maps);	/* - pointer to structure containing
					** an equivalence class map and an
					** aggregate map associated with this
					** co node 
					** - the equivalence
                                        ** class map field, indicating which
                                        ** equivalence classes should be
                                        ** returned, a slight exception is
                                        ** the DB_ORIG node in which the
                                        ** equivalence classes are the same
                                        ** as the project-restrict
                                        ** instead of the "total set"
                                        ** of attributes referenced from the
                                        ** relation, i.e. this is not = opv_eqcmp
					** - the aggregate map represents the
					** aggregates which can be evaluated at this
					** node
                                        */
    OPO_PARM        opo_cost;           /* contains floating point numbers
                                        ** which are parameters to the cost
                                        ** equations for this operation
                                        */
    OPO_ISORT       opo_ordeqc;         /* multi-attribute ordering
                                        ** - "ordering equivalence class"
                                        ** - for a DB_ORIG node
                                        ** joinop attribute on which
                                        ** the relation represented by
                                        ** this node is ordered (used with
                                        ** isam, hash, sort, btree), note
                                        ** this does not imply sortness
                                        ** but all other node types do imply
                                        ** sortedness if >= 0
                                        ** - for a DB_PR node
                                        ** this is the ordering for a BTREE
                                        ** tree with (DB_SORT_STORE), or
                                        ** ISAM ordering with DB_HEAP_STORE
					** or OPE_NOEQCLS
                                        ** else
                                        ** - for a DB_SJ node, it is the
					** available ordering which can be
                                        ** used as sorted by a parent
                                        ** - in OPC this is ignored, except
                                        ** in the case of the parent having
                                        ** a sort node specified (see opo_isort
                                        ** and opo_osort)... in which case
                                        ** this ordering should be used to
                                        ** define the sort node, an extra pass
                                        ** is made after enumeration to define
                                        ** this field for OPC.
                                        ** - for top sort node the sort list
                                        ** is in the PSF query tree header
                                        ** this value will be OPE_NOEQCLS
                                        */
    OPO_ISORT       opo_sjeqc;          /* multi-attribute ordering
                                        ** - joining equivalence class
                                        ** - used only if opo_sjpr is a
                                        ** simple join
                                        ** - OPE_NOEQCLS if a cartesean prod
                                        */
    OPO_ISORT	    opo_orig_sjeqc;	/* joining ordering excluding
					** sortedness considerations, used
					** for hash join transformation since
					** hash join need not be constrained
					** by existing orderings */
    i2              opo_pointercnt;     /* - number of CO nodes pointing to this
                                        ** node through opo_outer and opo_inner
                                        ** - if zero and opo_deleteflag is true
                                        ** then this struc can be returned to
                                        ** free space.
                                        */
    i2			opo_ccount;	/* if node is DB_EXCH, this is 
					** child thread count */
    union
    {
	OPV_IVARS       opo_orig;	/* if node is DB_ORIG type then this
					** is the variable represented by this
					** CO node */
	OPL_IOUTER	opo_ojid;	/* if this is a non-leaf node then
					** this outer join ID applies */
    }	opo_union;
    OPO_STORAGE	      opo_storage;      /* organization of the relation at
                                        ** this node which could be
                                        ** DB_SORT_STORE, DB_HEAP_STORE,
                                        ** DB_ISAM_STORE, DB_HASH_STORE,
                                        ** DB_BTRE_STORE
                                        */
    char            opo_sjpr;		/* operation performed to create
                                        ** this relation
                                        ** - can be one of DB_ORIG, DB_PR
                                        ** DB_SJ, DB_RSORT, DB_RISAM, DB_RHASH,
                                        ** DB_RBTREE.
                                        */
/* until we define this nodes for the BCO trees */
#define                 DB_ORIG         1    /* original relation */
#define                 DB_PR           2    /* project restrict */
#define                 DB_SJ           3    /* simple join */
#define			DB_EXCH		4    /* change node */
#define                 OPO_REFORMATBASE 5   /* base of reformat operators */
/* this numerical ordering of the following elements must stay the same */
#define                 DB_RSORT        (OPO_REFORMATBASE + OPN_IOSORT)
/* reformat to sort */
#define                 DB_RISAM        (OPO_REFORMATBASE + OPN_IOISAM)
/* reformat to isam */
#define                 DB_RHASH        (OPO_REFORMATBASE + OPN_IOHASH)
/* reformat to hash */
#define                 DB_RBTREE       (OPO_REFORMATBASE + OPN_IOBTREE)
/* reformat to btree */
    BITFLD          opo_deleteflag:1;   /* TRUE - if this CO node can not be
                                        ** part of an optimal solution
                                        */
    BITFLD          opo_existence:1;    /* if we do not need to pass any 
                                        ** attributes from the inner backup
                                        ** then we only need to check for the
                                        ** existence of a match on the inner
                                        ** and (for the given outer row) we
                                        ** can stop scanning as soon as one
                                        ** matching row is found.  This is
                                        ** suggested by " An architecture for
                                        ** query optimization" by Arnon
                                        ** Rosenthal & David Reiner, SIGMOD
                                        ** 1982
                                        */
    BITFLD          opo_odups:1;        /* used to override the default
                                        ** duplicate removal in a sort node 
                                        ** or outer opo_osort flag */
    BITFLD          opo_idups:1;        /* used to override the default
                                        ** duplicate removal in inner
                                        ** opo_isort sort node */
#define                 OPO_DREMOVE     1
/* The sort node duplicates should be removed */
#define                 OPO_DDEFAULT	0
/* This could be any node, but if it is a sort node then use the ops_duplicates
** flag in the subquery structure to determine if duplicates should be
** removed */
    BITFLD          opo_osort:1;        /* TRUE indicates that a sort node 
                                        ** should be placed on the outer 
                                        ** subtree 
                                        ** - in OPF enumeration it means that
                                        ** the cost model can sort attributes
				        ** in any order
                                        ** - in OPC it means that
                                        ** the order in which to sort is the
                                        ** opo_outer->opo_ordeqc , and the
                                        ** duplicate removal semantics are
                                        ** in opo_outer->opo_dups */
    BITFLD          opo_isort:1;        /* TRUE indicates that a sort node 
                                        ** should be placed on the inner
                                        ** subtree 
                                        ** - in OPF enumeration it means that
                                        ** the cost model can sort attributes
				        ** in any order
                                        ** - in OPC it means that
                                        ** the order in which to sort is the
                                        ** opo_inner->opo_ordeqc and the
                                        ** duplicate removal semantics are in
                                        ** opo_inner->opo_dups */
    BITFLD          opo_psort:1;        /* TRUE - if the opo_ordeqc field
                                        ** is defined by a sort node in the
                                        ** parent, used only by the CO tree
                                        ** printing routines, for more
                                        ** informative explanations as to
                                        ** where the sortedness is orignating
                                        ** from */
    BITFLD          opo_pdups:1;        /* Duplicate handling for sort if
					** opo_psort is TRUE, set to
					** OPO_DREMOVE or OPO_DDEFAULT
  					** - the only interesting case is
   					** if the duplicates need to be
   					** kept for the subquery, and if this
   					** particular subtree needs duplicates
   					** removed, then OPO_DREMOVE will be
   					** set */
    BITFLD          opo_held:1;		/* TRUE if CO node is part of active
					** "best fragment" in new enumeration
					** scheme
                                        */
    BITFLD	    opo_punion:1;	/* TRUE if CO node is an EXCHANGE
					** for a parallel union */
    BITFLD	    opo_exchagg:1;	/* TRUE if CO node is an EXCHANGE
					** for partitioning right under an
					** aggregate action */
    BITFLD	    opo_tprocjoin:1;	/* TRUE if CO node is CP join with
					** TPROC on inner that's dependent
					** on outer */
    /* Cardinality assertion bit fields: */
#define OPO_CARD_NONE	0	/* none	    = PST_NOMETA */
#define OPO_CARD_01	1	/* 0 or 1   = PST_CARD_01R or PST_CARD_01L */
#define OPO_CARD_ANY	2	/* any	    = PST_CARD_ANY */
#define OPO_CARD_ALL	3	/* all	    = PST_CARD_ALL */
				/* (PST_CARD_01L becomes an outer PST_CARD_01R */
    BITFLD	    opo_card_own:2;	/* Card assrtion for own */
    BITFLD	    opo_card_outer:2;	/* Card assrtion for outer */
    BITFLD	    opo_card_inner:2;	/* Card assrtion for inner */
    BITFLD	    opo_is_swapped:1;	/* Flag to track if inner&outer swapped */
    union 
    {
	OPD_SCOST       *opo_scost;	/* site cost information 
					** - one cost element per site
                                        ** - used during enumeration phase
					** of distributed optimization
					*/
	OPD_ISITE	opo_site;	/* site upon which the join
					** should take place
                                        ** - used only when a best CO tree
					** is copied out of enumeration
					** memory, opo_scost is lost, this
                                        ** will be the case when opo_memory
                                        ** is set to TRUE */
	OPD_QCOMP       *opo_comp;	/* not used */
	OPD_LOCAL	*opo_local;	/* optimization info for OPC */
    }   	    opo_variant;	/* information
					** that OPC needs to compile this
					** CO node.
                                        ** - also used to contain distributed
                                        ** database cost information
					*/

}   OPO_CO;

/*}
** Name: OPO_PERM - CO structures allocated outside of enumeration memory
**
** Description:
**      This structure exists in order to recover from out-of-memory errors.
**      Prior to enumeration, a linked list of these structures will be
**      allocated, so that if memory is exhausted, these structures can be
**      used to copy the best CO tree out of enumeration memory.  Thus,
**      further optimizations, and query compilation can be completed.
**
** History:
**      28-oct-86 (seputis)
**          initial creation
[@history_template@]...
*/
typedef struct _OPO_PERM
{
    struct _OPO_PERM *opo_next;         /* next perm structure in list 
					** - terminated by NULL */
    OPO_CO          opo_conode;         /* CO structure reserved outside
                                        ** of enumeration memory */
    OPO_COMAPS	    opo_mapcopy;	/* contains equivalence class map
					** as well as aggregate map for
					** CO node */
					/* map of equivalence classes
                                        ** associated with this node, pointed
                                        ** at by opo_conode.opo_maps
                                        ** - needed since the equivalence class
                                        ** map may also be stored in enumeration
                                        ** memory
                                        */
}   OPO_PERM;
