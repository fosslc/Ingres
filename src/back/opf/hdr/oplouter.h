/*
**Copyright (c) 2004 Ingres Corporation
** All Rights Reserved
*/

/**
** Name: OPLOUTER.H - outer join structures
**
** Description:
**      This file contains new structures related to the outer join 
**      project.
**
** History:
**      11-nov-92 (ed)
**          initial creation
**	2-Jun-2009 (kschendel) b122118
**	    Delete ojfilter things, not much chance of it being implemented
**	    now.  (Also, unclear whether it was ever a good idea.)
[@history_template@]...
**/

/*}
** Name: OPL_PARSER - array of parser bitmaps
**
** Description:
**      This array will initially contain copies of the parser range
**      table bitmaps for outer joins.  These maps will be modified
**      as flattening of subselects occurs.  More important is that
**	link back of aggregates will  not modify these structure so
**	that outer joins do not inadvertently get executed.
**
** History:
**      6-feb-90 (seputis)
**          initial creation to support outer joins
*/
typedef struct _OPL_PARSER
{
    PST_J_MASK      opl_parser[OPV_MAXVAR]; /* each element corresponds to
					** a range table element, and each
					** bit set indicates an outer join ID
					** for which this range variable is
					** the inner or outer, depending
					** on context */
}   OPL_PARSER;

/*}
** Name: OPL_OUTER - outer join descriptor
**
** Description:
**      Outer join descriptor which is defined within the context of a OPS_SUBQUERY
**	structure.  This descriptor contains information used in the processing
**	and placement of outer joins within the query plan.
**
** History:
**      14-sep-89 (seputis)
**          initial creation for outer joins
**	15-feb-93 (ed)
**	    fix cart prod outer join placement problem
**	15-feb-94 (ed)
**          - bug 59598 - correct mechanism in which boolean factors are
**          evaluated at several join nodes, added OPL_SINGLEVAR
**      15-feb-94 (ed)
**          bug 49814 - E_OP0397 consistency check, added opl_rmaxojmap
**	10-mar-06 (hayke02)
**	    Add OPL_OOJCARTPROD to indicate that this outer join has an outer
**	    cart prod. This change fixes bug 115599.
[@history_template@]...
*/
typedef struct _OPL_OUTER
{
    OPE_BMEQCLS     *opl_innereqc;	/* ptr to set of "special equivalence classes"
					** which represent all the inner relations
					** to this outer join */
					/* a special equivalence class is created
					** which describes the result of the 
					** outer join, this will be a function attribute
					** - QEF needs to do some special
					** handling when this outer join ID is
					** processed in order to create the
					** "special equivalence class" which
					** is a boolean attribute, which is TRUE
					** if the join was successful and FALSE
					** if the inner did not exist,... 
                                        ** e.g. "r, (s left join t) where t.a > 5 or r.b=6"
                                        ** note t.a > 5 should not only eliminate tuples
                                        ** which fail this qualification, but also eliminate
                                        ** tuples which did  not match the outer relation "s"
					** - this equivalence class may not always
					** be necessary, and will be OPE_NOEQCLS
					** if it is not necessary
					** - there will be modifications to the
					** query tree which use this equivalence class
					** e.g. in the above example it could be
					** (t.a > 5 and t.new = 1) or r.b = 6, however
					** in most cases the optimizer can detect that
					** this is not necessary */
    OPV_BMVARS	    *opl_ojtotal;	/* map of all variables which are considered 
					** "inner" to this join ID, this map is used
					** for legal placement of variables, this
					** includes all secondaries etc, this is 
					** different from opl_ivmap in that it
					** includes any indexes which are to be 
					** considered */
    OPV_BMVARS	    *opl_onclause;	/* bitmap of variables referenced in the ON
					** clause associated with this join ID, used
					** to determine degenerate joins,
					** special case is made for IS NULL in that
					** variables which are defined in the subtree
					** are not included, and also for "OR" which
					** require variables to be defined on all
					** subtrees is the disjunct prior to being 
					** included in this map
					*/
    OPV_BMVARS	    *opl_ivmap;		/* map of all variables which have this
					** join ID marked as an inner */
    OPV_BMVARS	    *opl_ovmap;		/* map of all variables which have this
					** join ID marked as an outer */
    OPV_BMVARS	    *opl_bvmap;		/* map of all variables which have this
					** join ID marked as either inner or outer */
    OPV_BMVARS	    *opl_ojbvmap;	/* map of all variables which have this
					** join ID marked as either inner or outer
					** and including any secondary indexes referenced
					** implicitly on relations marked in opl_bvmap 
					*/
    OPL_BMOJ	    *opl_idmap;		/* map of all outer join IDs which where defined
					** within the scope of this outer join */
    i4		    opl_mask;		/* mask of various booleans */
#define                 OPL_RESOLVED    1
/* SET if this outer join has been correctly placed at a node */
#define                 OPL_BFEXISTS    2
/* TRUE if at least one boolean factor with this join ID exists
** in the query, does not include equi-joins which become equivalence classes */
#define                 OPL_FJMAP       4
/* TRUE if outer join maps (opl_ojmap) minus the base relations have been set
** in order to check for outer joins */
#define                 OPL_OJCARTPROD  8
/* TRUE if this outer join is not connected among the inner relations so that
** special placement heuristics are needed to ensure correct outer join semantics
** by placing a cart prod lower in the tree */
#define                 OPL_BOJCARTPROD  16
/* TRUE if this outer join is not connected among all relations in the OJ scope so that
** special placement heuristics are needed to ensure correct outer join semantics
** by placing a cart prod lower in the tree */
#define                 OPL_FJFOUND     32
/* TRUE if the full join has been processed lower in the QEP, implying
** all subsequent references to this ojid are left tid joins */
#define                 OPL_SINGLEVAR   64
/* TRUE if this is an inner join, which only contains a single var, it could
** contain implicitly referenced secondary indices */
#define                 OPL_FULLOJFOUND 128
/* TRUE - if this is a full join and the join node for the full join operation has
** been assigned, used by opn_arl, note that a TID join can use this join id but this
** mask is not set until child node performing the full join is assigned */
#define                 OPL_OOJCARTPROD  256
/* TRUE if this outer join is not connected among the outer relations so that
** special placement heuristics are needed to ensure correct outer join
** semantics by placing a cart prod lower in the tree */
    OPL_IOUTER	    opl_id;		/* ojid - query tree ID associated with this
					** outer join, which should be found in
					** the op node of appropriate qualifications
					** - this is the offset in the opl_oj array 
					** of this element */
    OPL_IOUTER	    opl_gid;		/* global ID used in query tree produced
					** by parser, this is different from opl_id
					** since opl_id is local to the subquery
					** and the op nodes were renamed to have
					** opl_id which is local to a subquery
					*/
    OPL_OJTYPE	    opl_type;		/* type of outer join , INNER, LEFT, FULL */
    OPL_IOUTER	    opl_translate;	/* used to eliminate inner joins from
					** the subquery and fold them into the parent
					** - this is the join ID of the parent into
					** which to map the boolean factors, if this
					** inner join is to be eliminated */
#if 0
    OPE_BMEQCLS	    *opl_iftrue;	/* this is a map of special equivalence classes
					** for those variables which do not require
					** the iftrue function to be used for determining
					** a null result */
#endif
    OPV_BMVARS	    *opl_p1;		/* A full join divides variables into 2 partitions
					** - opl_p1 and opl_p2 represent this division */
    OPV_BMVARS	    *opl_p2;
    OPV_BMVARS	    *opl_agscope;	/* This is a map of variables which were originally defined
					** as correlated in an aggregate, and must be resolved
					** to be inner to this joinid but not inner to any other
					** joinid within the scope of this joinid i.e. in opl_idmap 
					** otherwise a retry without flattening is required
					** to deal with a possible NULL correlation
					** - an example of a query which this map helps
					** fix is   select * from (r left join s on s.a=r.a)
					**    where r.a = (select count(*) from t where s.b=t.b)
					*/


    /* FIXME the placement of relations can be relaxed further with more work
    ** to keep duplicate semantics correct, 
    ** in a query like
    ** select * from r right join ( s right join t on s.c=t.c ) on r.a=s.a and r.b=t.b;
    ** in which s.c=t.c is always false and the other qualifications are always true
    ** and we want the join tree shape is (t left join r) left join s which is not
    ** currently possible,... then if the iftrue function is used on r.tid in which
    ** input is the product of the special eqc for r and s, and a sort node is used
    ** to remove duplicates then the above tree shape can be allowed,... so that
    ** any duplicates introduced by r.b=t.b can be eliminated if the subsequent evaluation
    ** of the outer join to s fails 
    */
    OPB_BMBF	    *opl_bfmap;		/* map of boolean factors associated with the
					** ON clause of this outer join, used to help
					** in relation placement */
    struct _OPN_JTREE *opl_nodep;	/* if not NULL, then pointer to first node
					** at which an outer join operation for this
					** descriptor is executed, used to help in
					** relation placement */
    OPV_BMVARS	    *opl_minojmap;	/* map of variables which define the inner
					** relations of this outer join and had
					** a reference within the ON clause, i.e. if only
					** these relations exist on an inner then
					** this outer join must be executed if an 
					** outer relation exists in the outer branch
					*/
    OPV_BMVARS	    *opl_maxojmap;	/* map of variables which define the possible inner
					** relations of this outer join, i.e. opl_minojmap
					** is a subset of this map, but this includes variables
					** which where defined as inner to this ojid whether
					** or not an on clause attribute was referenced
					*/
    OPZ_BMATTS	    *opl_ojattr;	/* map of all attributes reference in the ON clause
					** for this outer join */
    OPL_BMOJ	    *opl_reqinner;	/* map of outer joins which need to be entirely 
					** evaluated before this outer join is evaluated */
    OPV_BMVARS	    *opl_rmaxojmap;	/* this is what really should be in opl_maxojmap
					** but cannot be used except when filters are
					** implemented... in the meantime a consistency
					** check requires this map */
}   OPL_OUTER;

/*}
** Name: OPL_OJT - array of pointers to outer join descriptors
**
** Description:
**      This is defined as a structure so that it can be dereferenced
**      in the debugger, otherwise this intermediate structure is not
**      necessary
**
** History:
**      14-sep-89 (seputis)
**          initial creation
[@history_template@]...
*/
typedef struct _OPL_OJT
{
    OPL_OUTER       *opl_ojt[OPL_MAXOUTER];	/* array of pointers to outer
					** join descriptors for a subquery */
}   OPL_OJT;

/*}
** Name: OPL_GOJT - array of translation offsets to outer join descriptors
**
** Description:
**	Used to translate parser join IDs into OPF join IDs which are global
**	to all subqueries
** History:
**      14-sep-89 (seputis)
**          initial creation
[@history_template@]...
*/
typedef struct _OPL_GOJT
{
    OPL_IOUTER       opl_gojt[OPL_MAXOUTER]; /* array of local outer join IDs */
}   OPL_GOJT;

/*}
** Name: OPL_OJOIN - root of data structures for outer join description
**
** Description:
**      This structure defines the elements necessary to support outer
**      joins.  It consists mainly of an array of outer join
**      descriptors each with a unique ID which can be used to cross
**	reference the query tree nodes, plus some general description variables.
**	Some of these variables are "global" in the sense that they are
**	used to keep track of changing state during a query tree traversal.
[@comment_line@]...
**
** History:
**      12-sep-89 (seputis)
**          initial creation for outer joins
**      26-apr-93 (ed)
**          add elements to help detect equi-joins used in implicit link-backs
**	    of aggregate temporaries, used in outer join flattening
**	 8-jul-99 (hayke02)
**	    Added OPL_IFNULL in opl_smask in order to indicate that we have
**	    found a PST_BOP node that performs the ifnull() function. This
**	    new mask value is used to indicate that a boolean factor cannot
**	    be evaluated until the outer join is evaluated (by calling 
**	    opl_bfnull()). This change fixes bug 94375.
**	2-dec-02 (inkdo01)
**	    Added OPL_CASEOP in opl_smask to indicate that we're traversing
**	    a case expression in opzcreate.c. The flag indicates that even
**	    though an OJ inner var may be referenced here, the expression isn't
**	    to be evaluated until after the OJ (fixes bug 109174).
**	14-Nov-08 (kibro01) b121225
**	    Added OPL_UNDEROR to determine if during tree traversal we are
**	    underneath an OR statement, whereupon "is not null" must be dealt
**	    with the same as "is null" - i.e. it cannot be determined until
**	    the relevant OJ parts are assembled.  It only degrades the join
**	    in the case where it is not used under an OR.
[@history_line@]...
[@history_template@]...
*/
typedef struct _OPL_OJOIN
{
    OPL_OJT	    *opl_base;	    /* ptr to array of pointers describing
				    ** outer joins, NULL if none */
    OPL_IOUTER	    opl_lv;	    /* number of outer joins currently
				    ** defined in this subquery */
    PST_J_MASK	    opl_jmap;	    /* map of PSF join IDs which are included
				    ** in this subquery */
    OPL_IOUTER	    opl_cojid;	    /* used to determine which variables are
				    ** referenced in a conjunct associated with
				    ** a join ID, so that when view subsitution
				    ** occurs, or flattening, the OP nodes can
				    ** be relabelled with the correct joinid  */
    OPV_BMVARS	    *opl_where;	    /* bitmap of variables referenced in the
				    ** the where clause, NULL if no outer joins
				    ** exist in the query, used to find
				    ** degenerate joins */
    OPV_BMVARS	    *not_needed;    /* for expansion */
    i4		    opl_smask;	    /* mask of subquery booleans for outer joins */
#define                 OPL_ISNULL      0x00000001L
/* TRUE - if during the query tree traversal, a parent node is the "IS NULL" operator */
#define                 OPL_SUBEQC      0x00000002L
/* TRUE - means that the "null indicator" equivalence class should be substituted if necessary */
#define                 OPL_IFTRUE	0x00000004L
/* TRUE - means that the RESDOM needs to be resolved since an IFTRUE was inserted into
** the expression */
#define                 OPL_TIDJOIN     0x00000008L
/* TRUE - means that an outer TID join was found */
#define                 OPL_IMPVAR	0x00000010L
/* TRUE - implicit link backs to aggregate temps are being processed */
#define			OPL_OJCLEAR	0x00000020L
/* TRUE - if initialization of enumeration structure is needed */

/* #define		notused		0x00000040L */

#define			OPL_IFNULL	0x00000080L
/* TRUE - if during the query tree traversal, a BOP node is the "IFNULL" operator */
#define			OPL_CASEOP	0x00000100L
/* TRUE - if during query tree traversal, a "case" expression is encountered */
#define			OPL_UNDEROR	0x00000200L
/* TRUE - if during query tree traversal we are under an "OR" clause */

    OPV_BMVARS	    *opl_subvar;    /* ptr to bitmap of variables which need
				    ** the special equivalence class referenced 
				    ** in a parent AND */
    OPL_PARSER	    *opl_pinner;    /* copy of parser range table for inner join IDs
				    ** used to make sure aggregate link backs do not
				    ** add spurious outer joins to the subquery */
    OPL_PARSER	    *opl_pouter;    /* copy of parser range table for outer join IDs 
				    ** used to make sure aggregate link backs do not
				    ** add spurious outer joins to the subquery */
    OPV_BMVARS	    *opl_agscope;   /* same definition as opl_agscope in OPL_OUTER
				    ** except that correlation is made to an aggregate
				    ** in the where clause */
    OPL_IOUTER	    opl_aggid;	    /* parser joinid in which the aggregate was
				    ** defined */
    struct _OPS_SUBQUERY *opl_parent;    /* used in aggregate optimization to discover
				    ** degenerate joins, points to parent subquery
				    ** being optimized */
    OPV_BMVARS	    opl_implicit;   /* global variable in parent subquery opl_parent 
				    ** which were corelated to this child */
}   OPL_OJOIN;

/*}
** Name: OPL_OJHISTOGRAM - describes outer join histograms for joining attributes
**
** Description:
**      This structure contains outer join histogram information used
**	for the "merge" phase of outer join histogram processing.  Given
**	the histogram which results when the ON clause is included,
**	and the histogram which results when the ON clause is not included,
**	and the original histograms of the joining attributes, estimates can
**	be made on the NULLs which will result in each cell.
**
** History:
**      28-feb-90 (seputis)
**          initial creation for outer joins
**	24-oct-05 (inkdo01)
**	    Added opl_douters for null outer estimation.
[@history_template@]...
*/
typedef struct _OPL_OJHISTOGRAM
{
    OPL_IOUTER      opl_ojtype;         /* outer join type at this node */
    OPO_TUPLES	    tlnulls;		/* adjusted total number of 
					** tuples which are included
					** due to outer join semantics from left after all
					** joining attributes are evaluated */
    OPO_TUPLES	    trnulls;		/* adjusted total number of tuples 
					** which are included
					** due to outer join semantics from right after all
					** joining attributes are evaluated */
    OPL_OUTER	    *opl_outerp;	/* ptr to outer join descriptor for this
					** node */
    OPL_IOUTER	    opl_ojid;		/* join ID being processed */
    i4		    opl_ojmask;		/* mask of boolean values */
#define                 OPL_OJONCLAUSE  1
/* TRUE - if ON clause boolean factors exist and the more elaborate histogram
** processing is required in which inner join semantics are used, but null
** estimates are saved, as opposed to the fast path in which outer join
** histograms are created directly */
    OPH_DOMAIN	    opl_douters;	/* count of distinct values of join 
					** columns in outer rows */
} OPL_OJHISTOGRAM;

/*}
** Name: OPL_GOJOIN - root of global data structures for outer join description
**
** Description:
**	Currently, this contains an array which is used for translation from 
**      the global ID produced by PSF to an ID local to a subquery defined by OPF
[@comment_line@]...
**
** History:
**      12-sep-89 (seputis)
**          initial creation for outer joins
[@history_template@]...
*/
typedef struct _OPL_GJOIN
{
    OPL_GOJT	    *opl_gbase;	    /* ptr to array of joinid's */
    PST_J_ID	    opl_glv;	    /* number of outer joins currently
				    ** defined in the entire query */
    i4		    opl_mask;	    /* mask of booleans used to describe
				    ** outer join state */
#define                 OPL_OJFOUND     1
/* TRUE - if at least one outer join is found in this query */
#define                 OPL_QUAL        2
/* TRUE - if the qualification is being processed */
    PST_QNODE	    *opl_one;	    /* represents an i1 which contains
				    ** a one, used for initialization of
				    ** the special equivalence class */
    PST_QNODE	    *opl_zero;	    /* represents an i1 which contains
				    ** a zero, used for initialization of
				    ** the special equivalence class */
    OPL_GOJT	    *opl_view;	    /* ptr to array of counts of outer
				    ** joins, used for view joinid extraction */
    OPL_GOJT	    *opl_fjview;    /* ptr to array of joinid's created for
				    ** purposes of flattening views into FULL
				    ** JOINs */
}   OPL_GJOIN;
