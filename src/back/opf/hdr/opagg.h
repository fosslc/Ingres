/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: OPAGGREGATE.H - aggregate processing structures for sub-query
**
** Description:
**      This file contains the structure which is the aggregate processing
**      phase component of the sub-query.
**
** History:
**      1-mar-86 (seputis)
**          initial creation
[@history_line@]...
**/

/*
**  Defines of other constants.
*/

/*
**      Constants associated with function aggregates
*/
/* - associated with the function aggregate is a "count field attribute" and
** the "aggregate result attribute".
** - these attributes are assigned result domain numbers depending on the
** largest result domain number contained in the bylist node of the aggregate
** function
** - the count attribute is assigned "largest + OPA_COUNT" result domain number
** - the result attribute is assigned "largest + OPA_RESULT" result domain 
** number
*/
#define                 OPA_COUNT       1
#define                 OPA_RESULT      2

#define                 OPA_CSIZE       4
/* the size in bytes of the "count field attribute" for a function aggregate */

/*}
** Name: OPA_SUBLIST - list of subquery structures
**
** Description:
**      Used to create a list of subquery structures
[@comment_line@]...
**
** History:
**      11-may-89 (seputis)
**          initial creation to fix b5655
[@history_template@]...
*/
typedef struct _OPA_SUBLIST
{
    struct _OPA_SUBLIST     *opa_next;	/* ptr to next element in list */
    struct _OPS_SUBQUERY    *opa_subquery; /* ptr to subquery which is a correlated  
					** AGHEAD */
}   OPA_SUBLIST;

/*}
** Name: OPA_ASUBQUERY - aggregate component of sub-query header structure
**
** Description:
**      This structure contains the variables used during the aggregate
**      analysis phase of the optimization.  This structure should not
**      be used outside of that phase.  This is part of the descriptor header
**      for an AGHEAD node which will eventually become a subquery.
**
** History:
**     21-feb-86 (seputis)
**          initial creation
**     26-dec-90 (seputis)
**          fix 32738
**      15-dec-91 (seputis)
**          fix b40402 - corelated group by aggregate gives wrong answer
[@history_line@]...
*/
typedef struct _OPA_ASUBQUERY
{
    PST_QNODE       **opa_graft;        /* this is a pointer to the position
                                        ** in the original query tree at
                                        ** which to graft (substitute) a new 
                                        ** result node of the sub-query.
                                        ** - a new result node will be grafted 
                                        ** into the outer aggregate or the main
                                        ** query in order to "link" the result
                                        */
    struct _OPS_SUBQUERY *opa_father;   /* NULL if this aggregate is not
                                        ** nested, otherwise points to the
                                        ** OPS_SUBQUERY descriptor which 
                                        ** is the outer aggregate
                                        */
    struct _OPS_SUBQUERY *opa_concurrent;/* list of subqueries which can be
					** executed concurrently with each
					** other... note that if a subquery
					** occurs in this list then it has
					** been removed from the
					** OPS_SUBQUERY.ops_next list
					*/
    OPA_SUBLIST	    *opa_agcorrelated;	/* list of correlated agheads which
					** orignated from views, fix for
					** b5655 */
    PST_QNODE       *opa_byhead;        /* NULL - indicates simple aggregate
                                        ** or main query being processed
                                        ** not NULL - indicates function 
                                        ** aggregate is being processed and
                                        ** points to byhead node of query
                                        ** tree
                                        */
    PST_QNODE       *opa_aop;           /* This tree contains type and expr     
                                        ** for aggregates being run together    
                                        ** - ptr to PST_AOP operator node
                                        ** associated with this subquery
                                        ** - either left branch of simple       
                                        ** AGHEAD node, or the right branch of  
                                        ** the BYHEAD node for function agg     
                                        */
    PST_QNODE	    *opa_corelated;     /* list of corelated attributes which
					** need to be placed into the by list
					** of this query */
    bool	    opa_redundant;      /* TRUE - if this aggregate has become
                                        ** redundant, because there is another
                                        ** aggregrate in the concurrent list
                                        ** which is identical to this one
                                        ** - this allows a func aggregate to be
                                        ** kept in the opa_concurrent list so
                                        ** that it can be linked to the outer
                                        ** aggregate properly
                                        */
    i4              opa_nestcnt;        /* ref count of nested aggs contained
                                        ** -if non zero , it implies that this
                                        ** aggregate will not be executed
                                        ** simultaneously with another aggregate
                                        */
    i4              opa_attlast;        /* - the largest attribute number
                                        ** associated with the temporary
                                        ** relation (i.e. range variable)
                                        ** - used to assign resdom numbers in
                                        ** opa_resdom
                                        ** - initialized to the number
                                        ** of elements in bylist, + 1 for
                                        ** count, + 1 for result
                                        ** - subsequently, this value is
                                        ** incremented whenever two aggregates
                                        ** are combined, to represent the
                                        ** "extra" aggregate result 
                                        */
    OPV_GBMVARS     opa_views;          /* bit map of views referenced in the
                                        ** query - SQL only */
    OPV_GBMVARS     opa_bviews;         /* bit map of variables brought in
                                        ** because of views - SQL only */
    OPV_GBMVARS     opa_uviews;         /* bit map of views which require the
                                        ** target list to be copied to the
                                        ** subquery to maintain proper duplicate
                                        ** handling */
    OPV_GBMVARS     opa_rview;          /* bitmap of views referenced in this
                                        ** subquery for which a var node exists
                                        ** - used to eliminate simple aggregate
                                        ** variable references in aggregate
                                        ** parent */
    OPV_GBMVARS     opa_linked;         /* bitmap of inner func aggregates which
                                        ** have been linked to this subquery
                                        ** - used to prevent a subquery from
                                        ** being linked twice */
    OPV_GBMVARS     opa_keeptids;       /* global variable bit map of TIDs 
                                        ** needed to be brought up from base
                                        ** relations */
    OPV_GBMVARS	    opa_aview;		/* map of view IDs for which an aghead
					** node was found in the context this query
					*/
    OPV_GBMVARS	    opa_flatten;	/* map of relations which were flattened
					** into the subquery */
    OPV_GBMVARS	    opa_exmap;		/* map for expansion */
    bool            opa_tidactive;      /* TRUE - if ops_keeptids map is 
                                        ** active, and TIDs need to be brought
                                        ** up from the base relations,
                                        ** otherwise ops_keeptids can be
                                        ** ignored */
/* the following fields are only used for function attributes */
    bool            opa_projection;     /* TRUE if the byvals require 
                                        ** projection ... used only for
                                        ** function aggregates
                                        */
    i4		    opa_mask;		/* mask of boolean states for this
					** subquery */
#define                 OPA_NOLINK	1
/* SET if no link back of function aggregate is required, used when flattening
** SQL aggregates which are ORed, since the link back was imbedded in the
** OR predicate containing the flattened query, the alternative would be
** to use a projection for SQL and use the regular link-back... 
*/
#define			OPA_APROJECT	2
/* in the local case it is probably better to define a projection for
** flattening SQL, since NULL joins are detected in ingres 
*/
    bool            opa_multi_parent;   /* TRUE if computing function aggs 
                                        ** from different parents   
                                        */
    OPV_GBMVARS	    opa_blmap;		/* variable map of the bylist
					** associated with the aggregate
					** function
					*/
    OPV_GBMVARS     opa_visible;        /* this is a variable map of the
					** inner function aggregates
					** which are visible to this aggregate
					** - i.e. the inner aggregates which are
					** nested in the bylist attributes 
					** - used to decide whether to link
					** an inner to an outer aggregate
					*/
    DB_ATT_ID	    opa_atno;		/* only valid for function aggregates
					** - contains attribute number of the
					** original grafted node, part of
					** b32738 fix */
    PST_QNODE       *opa_fbylist;       /* pointer to beginning of list of
					** by expression values which have been
					** added due to corelated attributes
					*/
}   OPA_ASUBQUERY;

/*}
** Name: OPA_AGGSTATE - Control block for aggregate processing
**
** Description:
**     This structure is the part of the global state variable that is
**     only visible to the aggregate processing phase of the optimizer.
**
** History:
**     21-feb-86 (seputis)
**          initial creation
**     6-nov-88 (seputis)
**          add subquery ID for tracing
**     8-feb-89(seputis)
**          add ops_uview to fix query printing bug
**     9-nov-89 (seputis)
**	    added opa_vid for b8160, a view/aggregate duplicate problem
[@history_line@]...
*/
typedef struct _OPA_AGGSTATE
{
    bool            opa_funcagg;        /* TRUE if there exists at least
                                        ** one function aggregate in the
                                        ** entire query tree
                                        */
    OPV_GBMVARS     opa_allviews;       /* bitmap of all the views in
                                        ** query */
    i4	    opa_subcount;       /* current number of subqueries defined
                                        ** used for subquery ID */
    PTR		    junk;
#if 0
    struct _OPS_SUBQUERY *ops_uview;    /* used to place union views at end
					** of query */
#endif
    OPV_IGVARS	    opa_viewid;		/* current global range number of
					** view being substituted, used to keep
					** duplicate semantics correct */
    OPA_VID	    opa_vid;		/* a unique ID used to tag all implicitly
					** referenced variables originating from the
					** same explicitly referenced view... used
					** to fix b8160, quel duplicate handling */
    char	    opa_expansion[2];	/* expansion*/
}   OPA_AGGSTATE;

/*
** To understand this structure picture the query tree stripped of all nodes
** except AGHEAD nodes and the ROOT node.  The AGG_LIST provides a mechanism
** by which this "aggregate tree" can be traversed and manipulated.  The 
** original query tree may be manipulated as simple aggregates are calculated.
**
**    Example:
**       The basic aggregate structure in the query tree (by ignoring
**       the intermediate query tree nodes) may be as follows
**
**                         AGHEAD4 (=ROOT)
**                           | |
**                  +--------+ +--------+
**                  |                   |
**                  V                   V
**               AGHEAD3             AGHEAD0
**                |   |
**          +-----+   +-----+
**          |               |
**          V               V
**       AGHEAD2         AGHEAD1
**

**    The OPA_ASUBQUERY structure built on the above tree would be:
**
**    notation: sq3 would represent a subquery node of OPS_SUBQUERY type
**              so that sq4->opa_agg would reference opa_agg of "node 4"
**
**    PST_QTREE STRUCTURE                       OPA_ASUBQUERY STRUCTURE
**
**    ptr to query tree +------------------sq4.opa_agg.opa_graft
**    aggregate node    |        +---------sq4.ops_root        
**    i.e. may be a     |        |         sq4.opa_agg.opa_father------->NULL
**    PST_QNODE->left orV        V         sq4.opa_agg.opa_nestcnt = 4
**    PST_QNODE->right  o----> AGHEAD4     ^^
**                                         ||
**                                         |+--------------------------+
**                                         +------------------------+  |
**    ptr to query tree +------------------sq3.opa_agg.opa_graft    |  |
**    aggregate node    |        +---------sq3.ops_root             |  |
**    i.e. may be a     |        |         sq3.opa_agg.opa_father---+  |
**    PST_QNODE->left orV        V         sq3.opa_agg.opa_nestcnt = 2 |
**    PST_QNODE->right  o----> AGHEAD3     ^^                          |
**                                         ||                          |
**                                         |+-------------------------+|
**                                         +-------------------------+||
**    ptr to query tree +------------------sq2.opa_agg.opa_graft     |||
**    aggregate node    |        +---------sq2.ops_root              |||
**    i.e. may be a     |        |         sq2.opa_agg.opa_father----+||
**    PST_QNODE->left orV        V         sq2.opa_agg.opa_nestcnt = 1||
**    PST_QNODE->right  o----> AGHEAD2                                ||
**                                                                    ||
**    ptr to query tree +------------------sq1.opa_agg.opa_graft      ||
**    aggregate node    |        +---------sq1.ops_root               ||
**    i.e. may be a     |        |         sq1.opa_agg.opa_father-----+|
**    PST_QNODE->left orV        V         sq1.opa_agg.opa_nestcnt = 0 |
**    PST_QNODE->right  o----> AGHEAD1                                 |
**                                                                     |
**    ptr to query tree +------------------sq0.opa_agg.opa_graft       |
**    aggregate node    |        +---------sq0.ops_root                |
**    i.e. may be a     |        |         sq0.opa_agg.opa_father------+
**    PST_QNODE->left orV        V         sq0.opa_agg.opa_nestcnt = 0
**    PST_QNODE->right  o----> AGHEAD0

**                                                                
**      If the aggregrate
**      example is used then the following order would be used.  Note that the
**      subquery list is rooted in the session control block.
**
**      ops_cb.ops_subquery                list rooted in session control block
**                      |
**      +---------------+
**      |
**      v
**      op2.ops_next--------+
**                          |
**      +-------------------+
**      |
**      v
**      op1.ops_next----+
**                      |
**      +---------------+
**      |
**      v
**      op3.ops_next--------+
**                          |
**      +-------------------+
**      |
**      v
**      op0.ops_next----+
**                      |
**      +---------------+
**      |
**      v
**      op4.ops_next------------->NULL                    
**       
*/
