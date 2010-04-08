/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: OPMFLAT.H - structures associated with flattening of subselects
**
** Description:
**      This file contains structures defining the characteristics of
**      subselects as used in the optimizer
**
** History:
**      1-dec-91 (seputis)
**         initial creation
[@history_line@]...
**/

/*}
** Name: OPM_FAGG - function aggregate descriptor
**
** Description:
**      This structure describes the function aggregate which is associated
**      with a function attribute.
**      - opm_reqc represents equi-joins which were originally correlated
**      so that during aggregate operator placement, the subtree must
**      for each equivalence class, a relation from the outer contributing
**      and attribute to this equivalence class.  Other placement algorithms
**      will ensure that all relations from the inner will be already in
**      the subtree for the aggregate.  A desirable alternative is to
**	allow a double outer join for this special case, since only
**	equijoins will be possible and not boolean factors from the aggregate
**	on clause.
**	select r.a from r,s where s.a = (select count(t.a) from t
**	    where t.b=r.b and t.c=s.c)
**	Normally a cart prod is required prior to the outer join of r,s to t
**	but it may be desirable to group on t.b and t.c and calculate the
**	count aggregate, and then perform 2 outer joins.  There are boolean
**	factor placement adjustments that need to be made to accomodate this.
**
** History:
**     1-dec-91 (seputis)
**          initial creation
[@history_line@]...
*/
typedef struct _OPM_FAGG
{
#if 0
    struct _OPM_FAGG    *opm_next;  /* linked list of function aggregates for
                                    ** this subquery */
    struct _OPM_FAGG    *opm_concurrent; /* linked list of function aggregates
                                    ** are to be evaluated together */
#endif
    struct _OPM_FAGG	*opm_opcaggp; /* - for OPC - this should be the evaluation
				    ** order of the aggregates, this becomes
				    ** significant for nested aggregates subselects
				    ** - note that opm_bfeval
				    ** should be computed respectively after each
				    ** aggregate is evaluated since it could
				    ** affect the answer */
    OPB_BMBF		*opm_bfeval;/* if not NULL then this is a
				    ** map of boolean factors which need to be
				    ** evaluated immediately after this aggregate
				    ** is evaluated and before the next aggregate
				    ** is evaluated, to be used by OPC 
				    ** - this will typically use an aggregate result
				    ** associated with this function aggregate to
				    ** evaluate a boolean factor */
    OPO_ISORT		opm_sort;   /* - to be used by OPC
				    ** - ordering used to evaluate the function
				    ** aggregate by list, i.e. when this set of
				    ** values changes for this node then a new
				    ** partition for the aggregate should be computed
				    ** and the current partition should be passed
				    ** to the parent */
				    /* OPF - working variable used to determine if
				    ** more than one aggregate can take advantage
				    ** of an existing ordering */
    OPZ_BMATTS		opm_fatts;  /* for OPC - map of joinop attribute numbers for all
                                    ** function attributes which can be computed
                                    ** together in the same filter, can be used to
                                    ** obtain the function attribute numbers and eqcls
                                    ** of all aggregate results needed */
    OPL_IOUTER          opm_oj;     /* outer join identifier associated with
                                    ** this aggregate, if it is necessary */
    OPE_IEQCLS		opm_single; /* if corelation is one eqcls then this
				    ** member contains that value, otherwise
				    ** contains OPE_NOEQCLS */
    OPE_BMEQCLS         opm_coeqc;  /* represents the total
                                    ** set of corelated equivalence classes
				    ** including non-printing resdoms, it is used
				    ** in placement of the aggregate evaluation
				    ** in which non-printing resdoms must be
				    ** included for duplicate handling,
				    ** - in final traversal, it represent the
				    ** available equivalence classes for the
				    ** evaluation of the aggregate */
    OPE_BMEQCLS         opm_pcoeqc; /* represents the printing resdoms only
                                    ** - corelated equivalence classes in which
				    ** any group by ordering to evaluate
				    ** the aggregate must order this set 
				    ** - this must be a subset of opm_coeqc */
    OPE_BMEQCLS		opm_outside; /* map of all equivalence classes of all
				    ** variables which are outside the scope
				    ** of the subselect used to define this
				    ** aggregate, this is used to compute
				    ** orderings needed to evaluate the aggregate 
				    ** - for OPC the map of equivalence classes
				    ** which form the group by list for an
				    ** aggregate */
    OPE_BMEQCLS         *opm_reqc;  /* if not-NULL then it represents the
                                    ** set of equi-join equivalence classes
                                    ** within the scope of the aggregate subselect
				    ** - this can be used to place a double outer
				    ** join for the aggregate, or it can be used
				    ** to restrict the search space so that a
				    ** double outer join is not possible
                                    ** - used to extend the current outer join
                                    ** model to ensure all qualifications are
                                    ** executed before the outer join is evaluated
                                    ** including equi-joins
				    */
#if 0
    OPE_BMEQCLS		*opm_bfeqcmap; /* map of equivalence classes needed from 
				    ** the children if this aggregate has not been
				    ** evaluated,... includes equivalence classes
				    ** used to evaluate the aggregate and any
				    ** equivalences classes of boolean factors
				    ** which contain the aggregate */
#endif
    OPV_BMVARS          opm_ivmap;  /* map of base relations which are required in
                                    ** this subtree prior to aggregate evaluation
				    ** - is a subset of relations in opm_tivmap
				    ** - includes from list of any nested subselects
				    ** defined in the scope of this subselect
				    ** - this variable will change depending on the
				    ** set of relations currently selected for
				    ** opn_arl */
    OPV_BMVARS          opm_tivmap;  /* map of base relations which are required in
                                    ** this subtree prior to aggregate evaluation
				    ** - this should be equivalent to the from list
				    ** of the aggregate subselect, plus any implicitly
				    ** references indexes that may be used in
				    ** the subquery
				    ** - includes from list of any nested subselects
				    ** defined in the scope of this subselect
				    ** - this is static for all enumeration */
    struct _OPM_FAGG    *opm_naggp; /* linked list of function aggregates for
                                    ** this subquery, used for internal OPF
				    ** processing only */
    OPM_IAGGS		opm_aggid;  /* unique identifier for all aggregates which can
				    ** be evaluated together */
    OPM_BMAGGS		opm_comagg; /* map of aggregate IDs which are compatible
				    ** this this aggregate, i.e. given a sort
				    ** node, both aggregates can be evaluated
				    ** given a particular ordering */
    OPM_BMAGGS		opm_wcomp;  /* working map (i.e. temporary) of aggregates 
				    ** which can be computed together at a
				    ** particular node */
    OPE_BMEQCLS		opm_aggresult; /* map of equivalence classes which represent
				    ** the aggregate results associated with this
				    ** descriptor */
    OPB_BMBF		opm_bfmap;  /* map of boolean factors which contain an
				    ** aggregate in this descriptor, i.e. in 
				    ** opm_aggresult */
    i4			opm_mask;  /* mask of boolean values associated with
				    ** the aggregate */
#define                 OPM_AGGDEF      1
/* has opm_comagg been computed */
#define                 OPM_AGGEVAL     2
/* TRUE if aggregate has already been evaluated, this should only occur once 
** per subquery */
#define                 OPM_CHOUTER     4
/* TRUE if aggregate must be evaluated by a changing outer tuple value */
#define                 OPM_DISTINCT    8
/* TRUE if aggregate requires a distinct to evaluate, e.g. count(distinct r.a)
*/
    struct _OPS_SUBQUERY *opm_subquery;	/* subquery originally associated
				    ** with the aggregate temporary */
}   OPM_FAGG;

/*}
** Name: OPM_FAT - table of pointers to function aggregate descriptors
**
** Description:
**      Initially, this array of pointers is set to NULL and elements
**      are assigned to first NULL entry when needed.
**
** History:
**     1-dec-91 (seputis)
**          initial creation
[@history_line@]...
*/
typedef struct _OPM_FAT
{
    OPM_FAGG       *opm_fagg[OPM_MAXFAGG]; /* array of pointers to function
                                         ** aggregate elements
                                         */
}   OPM_FAT;

/*}
** Name: OPM_FADESCRIPTOR - structure defining state of function aggregates
**
** Description:
**      This structure contains elements which defines the state of the
**      function aggregates used in the subquery.
**
** History:
**     1-dec-91 (seputis)
**          initial creation
[@history_line@]...
*/
typedef struct _OPM_FADESCRIPTOR
{
    OPM_IAGGS	    opm_fav;            /* next free slot in the array of
                                        ** pointers to function aggregates
                                        ** - elements 0 to (opm_fav-1) of
                                        ** opm_fagg are used
                                        */
    OPM_FAT         *opm_base;          /* ptr to base of array of ptrs
                                        ** to aggregate descriptors
                                        */
    bool	    opm_mask;		/* mask of flags marking function
					** aggregate state */
#define                 OPM_AEXPR       1
/* TRUE - if more than one variable scanned in current subtree */
    OPM_BMAGGS	    opm_beforesort;	/* map of aggregate evaluated before
					** the top sort node used to evaluate
					** all remaining aggregates */
    struct _OPS_SUBQUERY *opm_flagg;	/* linked list of aggregate subquery
					** structures which have been 
					** flattened into a subquery */
    PST_J_ID	    opm_joinid;		/* joinid created since outer joins
					** are required to support flattening
					** associated with this subquery */
    OPV_IVARS	    opm_vno;		/* variable number of last variable
					** to be processed, in this subtree */
    PST_QNODE	    *opm_resdom;	/* used for consistency check, so that
					** non-transformed resdoms are not used
					** on recursive traversal of the
					** parse tree */
    OPM_FAGG	    *opm_tfagg;		/* pointer to function aggregate descriptor
					** for this subquery */
    OPO_COMAPS	    opm_maps;		/* map reserved for top join node of the 
					*/
}   OPM_FADESCRIPTOR;

/*}
** Name: OPM_AFSTATE - aggregate flattening state
**
** Description:
**      Structure used to retain reusable portions of aggregate flattening
**	calculations. 
**
** History:
**      4-dec-91 (seputis)
**          initial creation
[@history_template@]...
*/
typedef struct _OPM_AFSTATE
{
    OPO_CO		*aggcop;	/* CO node of plan to be considered
					** for aggregate evaluation */
    OPM_BMAGGS		i4uralmap;	/* map of aggregates which can be
					** evaluated with the current ordering
					** and CO tree structure without a
					** sort node */
    OPM_FAGG		*nat_aggp;	/* ptr to beginning of list of
					** function aggregates which can be
					** evaluated using this ordering */
    OPM_FAGG		*best_aggp;	/* for top aggregate node evaluation, this
					** represents the best alternative
					** of possibly many choices */
    OPO_COST		pcost;		/* cost of evaluating plan represented
					** by aggcop */
    OPN_JTREE		*np;		/* join tree node descriptor */
    OPN_SUBTREE		*sbtp;		/* subtree describing all CO node with 
					** all aggregates evaluated */
    OPN_EQS		*eqsp;		/* equivalence descriptor of CO nodes with 
					** all aggregates evaluated */
    OPN_RLS		*rlsp;		/* relation descriptor of CO nodes with 
					** all aggregates evaluated */
    OPE_BMEQCLS		available;	/* map of available equivalence classes
					** which can be used to evaluate boolean 
					** factors at this node, this would include
					** multi-variable function attributes and
					** equivalence classes from left and right
					** children, but not any aggregates which
					** can be computed at this node */
    bool		sort_needed;	/* TRUE if a sort node is required
					** to evaluate aggregates at this node */
    bool		cartprod;	/* TRUE if cart product can be considered
					** for insertion into the cost tree */
} OPM_AFSTATE;
