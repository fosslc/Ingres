/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: OPVARIABLE.H - range variable structures
**
** Description:
**      Structures which define range variables.
**
** History:
**      10-mar-86 (seputis)
**          initial creation
**	12-feb-93 (jhahn)
**	    Added support for statement level rules. (FIPS)
**      12-oct-2000 (stial01)
**          Added opv_kpleaf Related change 441299
[@history_line@]...
**/

/*}
** Name: OPV_PRIMARY - information defined for primary relations in range vars
**
** Description:
**      This structure isolates information that is defined for primary
**      relations, but not defined for indexes.  This structure is used
**      only as a component in a range variable structure.
**
** History:
**     11-mar-86 (seputis)
**          initial creation
[@history_line@]...
*/
typedef struct _OPV_PRIMARY
{
    bool            opv_indexcnt;       /* TRUE if this primary relation
                                        ** has any indexes
                                        */
    OPE_IEQCLS      opv_tid;		/* the equivalence class which
                                        ** represents the implicit TID
                                        ** of this primary relation 
                                        ** - OPE_NOEQCLS used if the implicit
                                        ** TID has not been referenced
                                        */
    OPE_BMEQCLS     opv_teqcmp;         /* this is a temporary bitmap used to
                                        ** help determine whether this relation
                                        ** can be replaced by a set of indexes
                                        ** on this relation
                                        */
}   OPV_PRIMARY;

/*}
** Name: OPV_INDEX - information defined for index relations in range variables
**
** Description:
**      This structure isolates information that is defined for index
**      relations, but not defined for primaries.  This structure is used
**      only as a component in the range variable typedef.
**      
** History:
**     11-mar-86 (seputis)
**          initial creation
**	12-may-00 (inkdo01)
**	    added link to index histogram for composite histos.
[@history_line@]...
*/
typedef struct _OPV_INDEX
{
    OPV_IVARS       opv_poffset;        /* the offset into the range table
                                        ** of the primary relation on which
                                        ** this range variable is an index.
                                        */
    OPE_IEQCLS      opv_eqclass;        /* the equivalence class of the primary
                                        ** attribute (first attribute of multi
                                        ** attribute key) of the storage 
                                        ** structure upon which this index is
                                        ** ordered
					** - OPE_NOEQCLS used if index is
                                        ** being considered only if it can
                                        ** substitute the base relation
                                        ** completely i.e. keyed lookup not
                                        ** useful for this index
                                        */
    OPV_IINDEX      opv_iindex;         /* this is the offset into the array
                                        ** of ptrs to index tuples associated
                                        ** with the base relation of the
                                        ** respective RDF info structure
                                        */
    OPH_INTERVAL    *opv_histogram;	/* ptr to histogram info for 
					** composite hisotgram for index
					*/
}   OPV_INDEX;

/*}
** Name: OPV_PCJDESC - describes "partitioning compatible" joins
**
** Description:
**      Identifies joins involving current var and another var (both
**	must be partitioned tables) in which join columns cover
**	a partitioning map for each table.
**
** History:
**      11-feb-04 (inkdo01)
**	    Written for table partiitoning.
*/
typedef struct _OPV_PCJDESC
{
    struct _OPV_PCJDESC	*jn_next;	/* ptr to next PCJ for table */
    OPV_IVARS		jn_var1;	/* this table */
    OPV_IVARS		jn_var2;	/* other table joined by this PCJ */
    u_i2		jn_dim1;	/* partition dimension (this var) */
    u_i2		jn_dim2;	/* partition dimension (other var) */
    OPE_IEQCLS		jn_eqcs[1];	/* array of eqcs involved in join */
}   OPV_PCJDESC;

/*}
** Name: OPV_SEPARM - associates a var node with a repeat query constant
**
** Description:
**      Associates a var node with a repeat query constant, used in
**      subselect parameter substitution of corelated variables
**
** History:
**      8-feb-87 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
typedef struct _OPV_SEPARM
{
    struct _OPV_SEPARM    *opv_senext;  /* ptr to next corelated var */
    PST_QNODE       *opv_sevar;         /* ptr to var node which is
                                        ** the corelated var */
    PST_QNODE       *opv_seconst;       /* ptr to const node which
                                        ** is the repeat query parameter
                                        ** used to substitute the var */
}   OPV_SEPARM;

/*}
** Name: OPV_SUBSELECT - subselect descriptor for local range variable
**
** Description:
**      Used to contain information about a subselect associated with
**      a joinop range variable.  The existence of a non-null ptr to this 
**      structure in OPV_VARS implies that the range variable represents a 
**      subselect and any PST_VAR node referencing this range variable
**      actually references a subselect target
**      list item. This range variable implies
**      restrictions with respect to placement in the join tree i.e. the
**      range variable must be the inner of an "SEJOIN", and all attributes
**      needed to evaluate the subselect must be available at the immediate
**      parent of this leaf node.  All boolean factors which reference this
**      subselect must also be able to be calculated at the immediate parent.
**         If two subselects are in the same boolean factor then they will
**      also be in this list.  Thus, this list represents a group of
**      subselects which must be evaluated together, because they are
**      connected by boolean factors.  Note that boolean factors can be
**      connected by subselects, and subselects can be connected by boolean
**      factors.
**         This structure describes equivalence classes required to evaluate
**      the subselect.  An SEJOIN is used for this evaluation and in general
**      it is probably a good idea to sort the outer tuples used to SEJOIN
**      to this range variable.  It is assumed that the query compilation
**      will define a multi-attribute sort on the outer, given more than
**      one correlated attribute on the inner (if a sort node is available on
**      the outer).  The only restriction on this multi-attribute sort will
**      be that the first equivalence class is same as that in the outer sort
**      node.  FIXME- need to introduce multi-attribute sort information to
**      CO nodes.
**
** History:
**      8-feb-87 (seputis)
**          initial creation
[@history_template@]...
*/
typedef struct _OPV_SUBSELECT
{
    struct _OPV_SUBSELECT    *opv_nsubselect; /* list of subselects which must
					** be evaluated together since they
                                        ** are in the same conjunct which is to
                                        ** be evaluated at the parent of this
                                        ** node - note that several boolean
                                        ** factors may have subselects in
                                        ** common */
    OPV_IGVARS      opv_sgvar;          /* global range variable number of
                                        ** the subselect represented by this
                                        ** structure */
    OPZ_IATTS       opv_atno;           /* joinop attribute number representing
                                        ** the subselect expression in the
                                        ** target list,  this number will be
                                        ** used in the PST_VAR node which
                                        ** represents this subselect 
                                        ** - FIXME - eventually may want
                                        ** subselects with same qual to be
                                        ** be evaluated together */
    OPV_SEPARM      *opv_parm;          /* list of PST_VAR nodes which represent
                                        ** the correlated variables and
					** corresponds to the list in the
                                        ** global range variable (i.e.opv_sgvar)
                                        ** subselect descriptor, except that
                                        ** joinop range variable numbers and
                                        ** attributes are used */
    OPE_BMEQCLS	    (*opv_eqcrequired);	/* bit map of the equivalence classes
                                        ** required to evaluate the boolean
                                        ** factors which contains the subselects
                                        ** associated with this range variable
                                        ** - this bit map is the same for all
                                        ** elements in the opv_nsubselect list
                                        ** - for a correlated aggregate it will
                                        ** contain the correlated equivalence
                                        ** classes only i.e. it is a ptr to
                                        ** opv_eqcmp
                                        */
    OPO_ISORT       opv_order;          /* this is an ordering associated
                                        ** this this subselect which takes
                                        ** into account correlated variables
                                        ** and key values, so that tuples
                                        ** are sorted to minimize subselect
                                        ** re-evaluation */
    OPE_BMEQCLS     opv_eqcmp;          /* equivalence class map field, 
                                        ** indicating which correlated
					** equivalence classes , if any, should
                                        ** be available to evaluate this
                                        ** correlated subquery
                                        ** - same as map of equivalence classes
                                        ** of attributes in opv_qnode list
                                        */
}   OPV_SUBSELECT;

/*}
** Name: OPV_VARS - one joinop range table element (a table descriptor)
**
** Description:
**      The structure contains the information describing either a table
**      or index used in a query.  Note, that whether an element describes
**      a primary relation or an index relation is contained implicitly
**      within its position in the range table; i.e. elements 0 to (opv_prv-1)
**      in the range table are primary relations.
**      
**      The relation descriptor information is accessed via the same structure
**      as the PSF (parser facility) so that a global cache can easily be
**      introduced at a later date.  The current design is such that no
**      memory will be owned by a particular facility concurrently.  This
**      will prevent inter-facility bugs.  As Jupiter stabilizes, it may
**      become desirable to have a common global cache.  Using the same
**      relation description typedef will make this much easier.
**
** History:
**     10-mar-86 (seputis)
**          initial creation
**     15-aug-88 (seputis)
**          creation opv_itemp temporary table ID for query compilation
**     20-mar-90 (seputis)
**	    added opv_ojmap, opv_ijmap, opv_ojeqc, opv_ojattr to support
**	    outer joins
**     26-mar-90 (seputis)
**          added OPV_CINDEX, OPV_CBF, OPV_FIRST, OPV_SECOND to fix b20632
**	16-jul-90 (seputis)
**	    fix sequent floating point exception
**	26-dec-90 (seputis)
**	    add cart prod support for metaphor problem
**	22-apr-91 (seputis)
**	    added opv_keysel for more accurate keying estimates for 
**	    project restricts
**      15-dec-91 (seputis)
**          fix b33551 - make a better initial guess of relations for enumeration
**      1-apr-92 (seputis)
**          fix b40687 - part of tid join estimation fix.
**      16-oct-92 (ed)
**          - fix bug 40835 - wrong number of duplicates when union views and
**          subselects are used
**	2-feb-93 (ed)
**	    - bug 47377 - add masks to help quickly determine applicability of
**	    DMF unique index checks
**      15-feb-94 (ed)
**          - bug 59598 - correct mechanism in which boolean factors are
**          evaluated at several join nodes, added opv_svojid
**	1-apr-94 (ed)
**	    - bug 60125 - cart prod incorrectly removed
**	10-nov-98 (inkdo01)
**	    Added OPV_BTSSUB flag for base table structure optimization
**	3-nov-99 (inkdo01)
**	    Added flag OPV_HISTKEYEST for better readahead predictability.
**	19-jul-00 (hayke2)
**	    Added OPV_EXPLICIT_IDX.
**	20-nov-00 (hayke02)
**	    Added OPV_VCARTPROD.
**	22-nov-01 (inkdo01)
**	    Changed value of VCARTPROD (already being used).
**	12-feb-03 (hayke02)
**	    Added OPV_OJINNERIDX.
**	14-jan-04 (inkdo01)
**	    Added opv_pqbf for table partitioning.
**	7-apr-04 (inkdo01)
**	    Added opv_pcjdp for table partitioning/parallel query processing.
**	2-may-2008 (dougi)
**	    Added opv_parmmap for TPROC variables.
*/
typedef struct _OPV_VARS
{
    struct _OPV_GRV *opv_grv;           /* ptr to global range variable
                                        ** element corresponding to this
                                        ** local range variable, note that
                                        ** this value may be inaccurate for
                                        ** subselects since a multi to one map
                                        ** may exist, if opv_subselect has more
                                        ** than one element in the list
                                        */
    OPV_SUBSELECT   *opv_subselect;     /* ptr to list of subselect descriptors
                                        ** which are represented by this
                                        ** range variable entry 
                                        ** - NULL if not a subselect */
    OPV_PRIMARY     opv_primary;        /* used only if element is a primary
                                        ** relation
                                        */
    OPV_INDEX       opv_index;          /* used only if element is an index   */

    OPB_MBF         opv_mbf;            /* - this structure defines a key list
                                        ** upon which this range variable is
                                        ** ordered... 
                                        ** and matching boolean factors
                                        ** associated with the keys in the list
                                        */
    OPO_COMAPS	    opv_maps; 		/* bit array representing the
                                        ** equivalence classes needed from
                                        ** this joinop range table element
                                        ** - each bit in this array has a
                                        ** corresponding element in the
                                        ** ope_eqclist array
					** - uses OPO_COMAPS in order to
					** be compatible with various usages
					** of OPO_CO.opo_maps
                                        */
    OPV_MVARS       opv_joinable; 	/* map of vars which are
                                        ** joinable to this var i.e.
                                        ** opv_joinable[i] is TRUE is range
                                        ** var "i" has an equivalence
                                        ** class in common with this var
                                        ** - this structure can be put
                                        ** into global range table to save
                                        ** space FIXME
                                        */
    struct _OPN_RLS *opv_trl;           /* Contains information about
                                        ** this relation in a form useful
                                        ** for the enumeration phase...
                                        ** There is a list of histograms which
                                        ** are available for all the attributes
                                        ** of this range variable which are
                                        ** used in the subquery.  
                                        ** - this list contains histograms
                                        ** for elements visible only to this
                                        ** subquery whereas the global range
                                        ** table will contain the other
                                        ** attributes ... the global range
                                        ** table histo list is searched prior
                                        ** to creating a new element ... all
                                        ** elements are then placed in the
                                        ** global histogram list after
                                        ** processing this subquery has
                                        ** completed
                                        */
    OPO_CO          *opv_tcop;          /* each range variable has cost ordering
                                        ** node created prior to enumeration
                                        **-used to describe the characteristics
                                        ** of a disk resident scan
                                        */
    OPZ_BMATTS      opv_attrmap;        /* map of the joinop attributes found
                                        ** in the relation
                                        */
    OPV_IGVARS      opv_gvar;           /* global range variable number for this
                                        ** local range variable, note that this
                                        ** value is inaccurate for subselects
                                        ** since there may be a multi to one
                                        ** map for subselects i.e. if 
                                        ** opv_subselect list is > 1 */
    OPD_ITEMP	    opv_itemp;		/* query compilation temporary table
					** identifier
					*/
    i4		    opv_mask;		/* mask of values used to indicate
					** various states of this variable */
#define                 OPV_CINDEX      0x00000001L
/* TRUE - if this index can provide all the attributes needed for the variable
** so that this index should not be placed into the same plan as the base
** relation since the TID join will not provide any restrictivity */
#define                 OPV_EXPLICIT    0x00000002L
/* TRUE - if an index was explicitly referenced in the query */
#define			OPV_CBF		0x00000004L
/* TRUE - if a boolean factor map should be generated for opv_bfmap
** which will be used to determine if joining two secondaries provides 
** restrictivity */
#define                 OPV_FIRST       0x00000008L
/* TRUE - if at least one secondary index has been used on this table */
#define                 OPV_SECOND      0x00000010L
/* TRUE - if at least two secondary indexes has been used on this table */
#define                 OPV_CPLACEMENT  0x00000020L
/* TRUE - if this is an index and cannot be used for index substitution so
** that it must be used with a TID join and the base relation must be
** placed in the tree appropriately as the inner of a parent node */
#define                 OPV_CARTPROD    0x00000040L
/* TRUE - if this variable has a storage structure with more than attribute
** in the key, and if this relation was eliminated from the query then
** the remaining variables are not joinable */
#define			OPV_KESTIMATE	0x00000080L
/* TRUE - if keying estimates have been computed */
#define                 OPV_TIDVIEW     0x00000100L
/* TRUE - if tid function attribute has been defined for this
** view */
#define                 OPV_1TUPLE      0x00000200L
/* TRUE - if there exists a dmf enforced unique key, and all attributes are
** equated to a constant */
#define                 OPV_UNIQUE      0x00000400L
/* TRUE - if there exists a dmf enforced unique key, on the base relation or
** any of the related secondary indexes */
#define                 OPV_AGGSINGLE   0x00000800L
/* TRUE - if the current join tree does not reference more than one relation
** from the set of this relation and all secondary indexes on this relation */
#define                 OPV_NOATTRIBUTES 0x00001000L
/* TRUE - if the relation does not have any attributes referenced in the
** query, but just appears in the from list */
#define			OPV_SPATF	0x00002000L
/* TRUE - this variable maps a spatial function Rtree */
#define			OPV_BTSSUB	0x00004000L
/* TRUE - the base table structure of this table covers all columns ref'd
** in query. Access to data pages is not required. */
#define			OPV_HISTKEYEST	0x00008000L
/* TRUE - opv_keysel was determined using non-default histograms and should
** be reasonably accurate. */
#define			OPV_EXPLICIT_IDX 0x00010000L
/* TRUE - if this secondary index is explicitly referenced in the query */
#define			OPV_COMPHIST	0x00020000L
/* TRUE - this variable has composite histograms. */
#define			OPV_VCARTPROD	0x00040000L
/* TRUE - if this var is part of a key join cart prod */
#define			OPV_OJINNERIDX	0x00080000L
/* TRUE - if this var has a secondary index chosen for use in the inner of
** an outer join, but does not cover the on and where clauses, nor is
** suitable for index substitution. Used to indicate that any outer TID
** join with this var as an inner should be rejected */
#define			OPV_LTPROC	0x00100000L
/* TRUE - if local var corresponds to table procedure invocation. */
    OPB_BMBF	    *opv_bfmap;		/* map of boolean factors which will
					** be evaluated at the project restrict
					** of the variable, used to determine
					** if joining two indexes on a TID join
					** is worthwhile in terms of getting
					** more restrictivity */
    OPL_BMOJ	    *opv_ojmap;		/* map of join ids for which this
					** table is the outer relation of an
					** outer join, NULL if not applicable */
    OPL_BMOJ	    *opv_ijmap;		/* map of join ids for which this
					** table is the inner relation of an
					** outer join, NULL if not applicable */
    OPE_IEQCLS	    opv_ojeqc;		/* - special equivalence class for
					** this table if applicable
					** - if this variable is an inner for
					** at least one outer join, then this
					** equivalence class describes the
					** "nullability" of other equivalence
					** classes containing attributes in this
					** table, i.e. if the outer join causes
					** a NULL row to be used for this table
					** then any qualification which directly
					** reference attributes of this relation
					** should use NULLs even if a value exists
					** from the outer table in the joining
					** equivalence class */
    OPZ_IATTS	    opv_ojattr;		/* this is the function attribute which
					** corresponds to the above equivalence
					** class */
    OPO_CO	    *opv_pr;		/* project restrict node, warning, used
					** only for preliminary index selection,
					** this may be deallocated if OPF runs
					** out of enumeration memory */
    OPO_BLOCKS	    opv_dircost;	/* cost of a directory search if a storage
					** structure exists */
    OPO_BLOCKS	    opv_leafcount;	/* number of leaves in index for
					** this table if it has a storage structure */
    OPO_TUPLES	    opv_kpb;		/* # keys per block for index page */ 
    OPO_TUPLES      opv_kpleaf;         /* # keys per btree/rtree leaf page */
    OPN_PERCENT	    opv_keysel;		/* selectivity of boolean factors which can
					** be used for keying in a project restrict */
    OPN_PERCENT	    opv_kbfcount;	/* number of keyable boolean factors which
					** exist */
    OPO_ACOST       *opv_jcosts;        /* array of best costs of joining from another
                                        ** relation to this relation, i.e. this relation
                                        ** is on the inner */
    OPO_ACOST       *opv_jtuples;       /* array of tuple counts of joining to another
                                        ** relation */
    OPO_TUPLES      opv_prtuples;       /* tuple estimate at project restrict node */
    OPZ_IATTS	    opv_viewtid;	/* set only if this is a view and OPS_TIDVIEW
					** mask is TRUE,... joinop attribute of puesdo
					** tid for this view used for duplicate handling
					*/
    OPE_BMEQCLS	    *opv_uniquemap;	/* if not NULL then a ptr to a map of
					** equivalence classes which are guaranteed
					** unique by DMF */
    OPL_IOUTER	    opv_svojid;		/* used for single variable inner joins
					** support of full joins */
    OPB_PQBF	    *opv_pqbf;		/* list of partitions for table along
					** with partition qualifying BF info */
    OPV_PCJDESC	    *opv_pcjdp;		/* list of descriptions of partition
					** compatible joins involving this
					** variable */
    OPV_BMVARS	    *opv_parmmap;	/* ptr to map of vars ref'ed by TPROC's
					** parameter list */
}   OPV_VARS;

/*}
** Name: OPV_RT - joinop range variable table (local to one sub-query)
**
** Description:
**
**      Range tables is an overloaded term in ingres which has different
**      meanings depending on the context.  At the user level, a range
**      variable is explicitly declared and the parser maintains a
**      corresponding table .  The parser will expand views so that parser's
**      range table will also contain implicit definitions. The query tree 
**      header contains a copy of this which is input to the optimizer.  The
**      optimizer then creates two other tables which it calls a global range
**      table and a local joinop table.
**
**      The joinop range table has a different organization from the
**      global range table (or the parser range table).  The variable map
**      supplied by the root node of the query is used to determine which
**      global range variables are used.  These range variables are
**      then used to build the local range table which is more convenient for 
**      the optimization process (i.e. it includes a number of state variables
**      useful for optimization.  The global range table is sparse, the
**      joinop range table is dense (i.e. elements of this array are allocated
**      from element 0).  
**      The global range table includes only explicitly reference base 
**      relations.  However, since indexes are modelled as tables in ingres,
**      joinop will add any indexes which may be useful in evaluating the
**      query to the local joinop range table.  
**
**      The query tree var nodes are renamed to joinop local range table
**      naming scheme.  This means a range variable is represented by an
**      index to local array of pointers.
**
**      There may be some indexes defined in the range table that
**      will not be used.  The optimization process will select all indexes
**      that could possibly be useful, and then eliminate those that are
**      not beneficial.  For this reason, the size of the optimizer range
**      table must be larger than the query tree range table (which only
**      specifies base relations).  Currently, there will be a fixed limit
**      on the size of the range table.  The eventual goal is to allow this
**      to be a dynamic structure (in a later phase).
**
**      There are many joinop local range tables (one for each subquery) but
**      only one global range table shared among all the subqueries.
**      There was thought given to using only one range table for all subqueries
**      but this would be difficult since the joinop range table contains
**      information which is unique to the subquery i.e. equivalence classes...
**      Equivalence classes and joinop attributes nodes would be 
**      difficult (and undesirable) to share between sub-queries.
** History:
**     7-mar-86 (seputis)
**          initial creation
[@history_line@]...
*/
typedef struct _OPV_RT
{
    OPV_VARS      *opv_rt[OPV_MAXVAR+1];   /* array of pointers to range
                                           ** variables
                                           ** - opv_rt[OPV_MAXVAR] is for the
                                           ** result relation
                                           */
}   OPV_RT;

/*}
** Name: OPV_RANGEV - range table descriptor
**
** Description:
**      Structure which describes the range table, as well as indexes which
**      reference the range table.
**
** History:
**     7-mar-86 (seputis)
**          initial creation
[@history_line@]...
*/
typedef struct _OPV_RANGEV
{
    OPV_IVARS       opv_prv;            /* index of the first non PRIMARY
                                        ** relation in the range table
                                        ** - elements 0 to (opv_prv-1)
                                        ** specify base relation used in
                                        ** the query
                                        */
    OPV_IVARS       opv_rv;             /* next free slot in the range table
                                        ** - elements 0 to (opv_rv-1) are
                                        ** used for base relations and indexes
                                        */
    OPV_RT          *opv_base;          /* pointer to base of array of pointers
                                        ** to the joinop range table elements
                                        */
    bool            opv_cart_prod;      /* TRUE if this query has a cartesean
                                        ** product in it somewhere ( it is not
                                        ** connected as determined by
                                        ** opv_chkconnect)
                                        */
    OPV_GBMVARS     opv_gbmap;          /* bit map of the global range variable
                                        ** which are being used in this subquery
                                        */
}   OPV_RANGEV;

/*}
** Name: OPV_GSUBSELECT - subselect descriptor
**
** Description:
**      This descriptor contains information on the subselect which is
**      common to all subqueries.  Subselects will be substituted in the
**      query by a PST_VAR node which has an attribute number of 1 which
**      will be used to represent the subselect.  This number of 1 may
**      be changed after joinop processing, to take into account subselects
**      which may be executed together (see opv_atno in OPV_SUBSELECT
**      structure).
**
** History:
**      9-feb-87 (seputis)
**          initial creation
[@history_template@]...
*/
typedef struct _OPV_GSUBSELECT
{
    struct _OPS_SUBQUERY *opv_subquery;	/* contains information on query
				        ** plan to be used to evaluate the
                                        ** subselect */
    OPV_SEPARM      *opv_parm;	        /* list of PST_VAR nodes using
                                        ** pst_right, corresponding to 
                                        ** repeat query parameters to be
					** substituted into the subquery
                                        ** - The PST_VAR nodes will use
                                        ** global range variable numbers and
                                        ** dmf attribute numbers
                                        ** - the subquery will refer to these
                                        ** vars by using PST_CONST nodes
                                        ** and using the appropriate pst_parm_no
                                        ** value */
    QEF_AHD         *opv_ahd;           /* ptr to query plan if it has been 
                                        ** compiled, NULL otherwise */
}   OPV_GSUBSELECT;

/*}
** Name: OPV_GRV - Global range variable (global to all sub-queries)
**
** Description:
**      This range table element contains information which is global to all the
**      joinop range vars.  This structure is only used to store information
**      which is not local to one subquery (such as the relation descriptor).
**      This structure will grow to contain histogram information, attribute
**      information, cost ordering for a disk resident scan.
**
** History:
**     15-mar-86 (seputis)
**          initial creation
**     9-nov-89 (seputis)
**	    added opa_gvid for fix to b8160
**     26-dec-90(seputis)
**	    add support for common table ID detection
**      18-sep-92 (ed)
**          bug 44850 - removed opv_lrv to support different mapping of global
**          to local variables
**	12-feb-93 (jhahn)
**	    Added support for statement level rules. (FIPS)
**      7-dec-93 (ed)
**          b56139 - add opv_attrmap to keep track of union view attributes
**	    used in parent queries
**      25-mar-94 (ed)
**          bug 59355 - an outer join can change the result of a resdom
**          expression from non-null to null, this change needs to be propagated
**          for union view target lists to all parent query references in var
**          nodes or else OPC/QEF will not allocate the null byte and access
**          violations may occur
**	18-june-99 (inkdo01)
**	    Added OPV_TEMP_NOSTATS flag and opv_ttmodel for temp table model
**	    histograms feature.
**	3-Sep-2005 (schka24)
**	    Remove "global histogram" pointer, nothing ever set it.
**	30-Jun-2006 (kschendel)
**	    Replace gcop with gcost so that we have someplace to modify
**	    aggregate-result size/cost info without altering the bestco.
**	    Bestco is the input, gcost is the output.
**	17-april-2008 (dougi)
**	    Added OPV_TPROC flag for table procedures.
*/
typedef struct _OPV_GRV
{
    RDR_INFO        *opv_relation;      /* relation descriptor information 
                                        ** - contains table, attribute
                                        ** index and/or histogram information
                                        ** NULL - indicates temporary table
                                        ** i.e. if no RDF information has been
                                        ** fixed for this global range table
                                        ** element ... this would occur when
                                        ** a temporary table has been allocated
                                        ** within the range table (e.g. for a
                                        ** function aggregate result)
                                        */
    OPV_GSUBSELECT  *opv_gsubselect;    /* ptr to a subselect descriptor
                                        ** if this range var represents a
                                        ** subselect, NULL otherwise
                                        */
    OPO_PARM	    opv_gcost;		/* Estimated size info for result table
	** Meaningful only for temp results produced for function aggregates.
	** It provides the estimated size of the intermediate relation
	** produced.  Taken from subquery's bestco, modified for aggregation.
	** This modified cost could be kept in the subquery struct, but
	** since it's irrelevant for any subquery that doesn't correspond
	** to a global range variable, this is a better place.  (e.g. simple
	** aggregates "look like" scalars, not intermediate tables, and
	** thus no need to store the size estimate.
	*/

    OPV_IGVARS      opv_qrt;            /* index into the respective element
                                        ** of the parser query tree range
                                        ** table.  
                                        ** - OPV_NOVARS if a temporary table
                                        ** or an implicitly referenced index
                                        */

	/* Opv_topvalid is a pointer to the first QEF_VALID structure that OPC
	** builds and puts onto the valid lists in action headers.
	** Opv_ptrvalid, on the other hand, points to a pointer that needs to
	** point to the next QEF_VALID struct that is allocated for this 
	** range var.
	*/
    QEF_VALID	    *opv_topvalid;
    QEF_VALID	    **opv_ptrvalid;

    OPS_SQTYPE      opv_created;        /* the type of subquery which
                                        ** created this global range
                                        ** variable where OPS_MAIN
                                        ** is used to represent
                                        ** range variables in the
                                        ** parser range table
                                        ** - OPS_FAGG used for function
                                        ** aggregates temporaries, 
					** and OPS_SELECT
                                        ** used for subselects */
    struct _OPS_SUBQUERY *opv_gquery;	/* contains information on query
				        ** plan to be used to produce the
                                        ** global range variable, either
                                        ** the function aggregate, view, or
                                        ** subselect */
    OPD_ISITE	    opv_siteid;		/* only inititalized for a
					** distributed thread to the offset
					** in the site array containing this
					** variable */
    OPA_VID	    opv_gvid;		/* global view ID used to identify
					** implicitly referenced tables which
					** were querymoded from views */
    OPV_IGVARS      opv_same;           /* global range variable number for the
					** set of variables with the same table
					** ID */
    OPV_IGVARS      opv_compare;        /* global range variable to use in case
					** of multiple mappings to the same
					** table id, this changes whenever a new
					** subquery to be compared for concurrent
					** execution is tested */
    i4		    opv_gmask;		/* contains the values of various
					** booleans */
#define                 OPV_GOJVAR			0x01
/* TRUE if this variable is an outer, i.e. PST_RNGTAB entry has pst_outer_rel
** set for at least one outer join ID */
#define			OPV_SETINPUT			0x02
/* TRUE if this variable is the set input parameter of a dbp */
#define			OPV_STATEMENT_LEVEL_UNIQUE	0x04
/* TRUE if this variable has a statement level unique index */
#define                 OPV_UVPROJECT			0x08
/* TRUE if this is a UNION ALL view in which projection is possible */
#define                 OPV_ONE_REFERENCE		0x10
/* TRUE if this variable is referenced by one or more subqueries */
#define                 OPV_MULTI_REFERENCE		0x20
/* TRUE if this variable is referenced by more than one subquery */
#define			OPV_UVEVALUATED			0x40
/* TRUE if distributed OPC has evaluated the union view */
#define                 OPV_NULLCHANGE                  0x80
/* TRUE if this view has had resdoms nullability changed due to outer join
** null handling */
#define			OPV_MCONSTTAB			0x00000100L
/* TRUE - this variable is an in-memory constant table constructed as part 
** of the MS Access OR transformation */
#define			OPV_TEMP_NOSTATS		0x00000200L
/* TRUE if this is a global temp table with NO model table to use for
** histograms */
#define			OPV_TPROC			0x00000400L
/* TRUE if this variable is a table procedure invocation */
    OPZ_BMATTS	    *opv_attrmap;	/* map of attributes referenced in this
					** union view by parent subqueries */
    PST_QNODE	    **opv_orrows;	/* ptr to vector of PST_CONST ptrs,
					** each of which anchors a "row" of
					** the MS Access OR transformed temp
					** table */
    RDR_INFO	    *opv_ttmodel;	/* ptr to RDR_INFO of persistent table 
					** which provides histogram models for
					** this temp table - or NULL */

}   OPV_GRV;

/*}
** Name: OPV_GRT - Global range table
**
** Description:
**      The global range table is an array of pointers to global range table
**      elements.  The global range table exists only for the duration of one
**      optimization; i.e. a new one is built every time.  The respective
**      element of the joinop local range tables will point to this element in a
**      multi to one map.  An element in the range table is not assigned
**      by indicating using a NULL value.  The elements of this table
**      are initially set by looking at the variable bit map of the root node.
**
**      Temporary tables will be assigned slots in the global range table
**      by using the first non-NULL entry in the table.  Once an entry is
**      made, it is not deallocated from this table.  Thus at the end of
**      the optimization, this table will contain elements representing all 
**      tables referenced in the query.
**
**      The maximum number of elements in the global range table
**      is larger than the parser limit of PST_NUMVARS, since we can create
**      temporary relations from aggregate processing.
**
**      The indexes which are implicitly referenced are not found in this
**      range table.
**      
** History:
**     10-mar-86 (seputis)
**          initial creation
[@history_line@]...
*/
typedef struct _OPV_GRT
{
    OPV_GRV         *opv_grv[OPV_MAXVAR];/* array of pointers to global
                                         ** range table elements
                                         */
}   OPV_GRT;

/*}
** Name: OPV_GLOBAL_RANGE - global range table definition
**
** Description:
**      This structure contains all definitions which are related to the
**      global range table for OPF; i.e. the ptr to the base of the table
**      as well as indexes which reference the table.
**
** History:
**     7-apr-86 (seputis)
**          initial creation
**      18-sep-92 (ed)
**          bug 44850 - added opv_lrvbase to support different mapping of global
**          to local variables
[@history_line@]...
*/
typedef struct _OPV_GLOBAL_RANGE
{
    OPV_GRT         *opv_base;           /* ptr to base array of pointers 
                                         ** to global range table elements
                                         */
    RDF_CB          opv_rdfcb;           /* RDF control block which is
                                         ** initialized for this user session
                                         ** and database - used for all calls
                                         ** to RDF for information which is
                                         ** inserted into the global range table
                                         */
    OPV_IGVARS      opv_gv;              /* 0 to opv_gv-1 are the "active"
                                         ** elements of the global range table
                                         ** pointed to by opv_base
                                         */
    OPV_GBMVARS     opv_mrdf;            /* bit map of global range variables
                                         ** which have RDF information 
                                         ** associated with them to be unfixed
                                         */
    OPV_GBMVARS     opv_msubselect;      /* bit map of global range variables
                                         ** which represent subselects */
    OPV_IVARS       *opv_lrvbase;        /* ptr to array of mapping variables
                                         ** from global to local range table
                                         ** elements */
                                         /* scratch variable used to rename
                                         ** var nodes in query tree from
                                         ** referencing global range table
                                         ** to referencing local range table
                                         ** - temporarily contains an index
                                         ** into the local range table of
                                         ** current subquery being analyzed
                                         */
}   OPV_GLOBAL_RANGE;

/*
** OPV_STK - stack structure to support flattening of heavily recursive
** functions. Function pst_push and pst_pop allow for the retention of the
** backtracking points. Function pst_pop_all allows for any cleanup of the
** stack should the traversal be terminated early.
** The stack is initially located on the runtime stack but extensions
** will be placed on the heap - hence the potential for cleanup needed.
** The size of 50 is unlikely to be tripped by any but the most complex
** queries and in tests optimiser memory ran out first.
**
** History:
**	27-Oct-2009 (kiria01) SIR 121883
**	    Created to support reducing recursion
**	
*/
typedef struct _OPV_STK
{
    struct OPV_STK1 *free;      /* Stack block head. */
    struct OPV_STK1 {
        struct OPV_STK1 *link;  /* Stack block link. */
        i4 sp;                  /* 'sp' index into list[] at next free slot */
#define OPV_N_STK 50
        PTR list[OPV_N_STK];    /* Stack list */
    } stk;
} OPV_STK;

VOID
opv_push_item(struct _OPS_STATE *,OPV_STK *, PTR);
PTR
opv_pop_item(OPV_STK *);
VOID
opv_pop_all(OPV_STK *);

#define OPV_STKDECL OPV_STK _opv_stk;
#define OPV_STKINIT (_opv_stk.free = NULL,\
		_opv_stk.stk.link = NULL,\
		_opv_stk.stk.sp = 0);
#define OPV_STKPUSH(_v) opv_push_item(global, &_opv_stk, (PTR)(_v))
#define OPV_STKPOP() opv_pop_item(&_opv_stk)
#define OPV_STKRESET opv_pop_all(&_opv_stk);
