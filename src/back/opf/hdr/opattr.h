/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: OPATTR.H - structures defining attributes of relations
**
** Description:
**      This file contains structures defining the characteristics of
**      attributes as used in the optimizer
**
** History:
**      10-mar-86 (seputis)
**          initial creation
**	20-feb-94 (ed)
**	    remove obsolete defines
[@history_line@]...
**/

/*}
** Name: OPZ_DMFATTR - DMF attribute type
**
** Description:
**      This type is used represent a DMF attribute number.  It should match
**      the type the DB_ATT_ID.db_att_id component.  Used in type casting
**      parameters.
**
** History:
**      15-mar-86 (seputis)
**          initial creation
**      15-dec-91 (seputis)
**          fix 8 char conflict in typedef name
[@history_line@]...
*/
typedef i2 OPZ_DMFATTR;

/*}
** Name: OPZ_ATTS - joinop attribute element
**
** Description:
**      This structure defines the "joinop" attribute.  A joinop attribute
**      is really a (range variable, range variable attribute) pair.  Joinop
**      algorithms (i.e. equivalence class partitioning of attributes) are
**      more naturally implemented by using a joinop attribute rather than
**      a range variable attribute.  The query tree is modified so that the
**      attribute field in the var nodes will contain the joinop attribute
**      which is really the index into the array of pointers to this element.
**
**      There is exactly one attribute element for each (range variable,
**      attribute) pair used in the query tree; i.e. are no duplicates
**      in this array of attribute elements.
** History:
**      15-mar-86 (seputis)
**          initial creation
**      12-jan-91 (seputis)
**          add mask of booleans for attribute descriptor
**      10-jun-91 (seputis)
**          fix for b37172, make histogram estimates more predictable
**	31-jan-94 (rganski)
**	    Removed opz_sargp, due to change to oph_sarglist().
**	13-mar-97 (inkdo01)
**	    Add OPZ_QUAL flag to indicate joinop attrs in where/on clauses.
**	2-june-00 (inkdo01)
**	    Added OPZ_COMP flag to indicate dummy attrs representing 
**	    composite histograms.
**	 6-sep-04 (hayke02)
**	    Added OPZ_COLLATT to indicate that this is a resdom attribute
**	    created in the presence of a colaltion sequence. This change
**	    fixes problem INGSRV 2940, bug 112873.
**	 8-nov-05 (hayke02)
**	    Added OPZ_VAREQVAR to indicate that this attribute is below
**	    an ADI_EQ_OP PST_BOP node with PST_VAR left and right. This
**	    change fixes bug 115420 problem INGSRV 3465.
**	15-aug-05 (inkdo01)
**	    Added opz_eqcmap for multi-EQC attrs (more OJ folliws).
[@history_line@]...
*/
typedef struct _OPZ_ATTS
{
    OPV_IVARS       opz_varnm;          /* variable number - index into
                                        ** the joinop range variable array
                                        */
    DB_ATT_ID       opz_attnm;          /* system catalog attribute number
                                        ** - this is not the same thing as
                                        ** the joinop attribute number
                                        ** - the query tree var nodes originally
                                        ** contained this attribute number
                                        ** prior to having the joinop attribute
                                        ** number substituted
                                        ** - DB_IMTID used for implicit tid 
                                        ** attribute number
                                        */
#define                 OPZ_FANUM       (-1)
/* used to represent a function attribute number ... when function attributes
** are created, this "flag" is used to indicate that opj_addatts should not 
** attempt to lookup the function attribute but instead, unconditionally 
** create it
*/
#define                 OPZ_SNUM        (-2)
/* used to represent a subselect expression result, associated with a virtual
** table, as with OPZ_FANUM, this flag is used to indicate 
** that opz_addatts should not 
** attempt to lookup the subselect attribute but instead, unconditionally 
** create it
*/
#define                 OPZ_CORRELATED  (-3)
/* used to represent a correlated attribute for an aggregate or subselect, and
** is added to the corelated equivalence class, so that this equivalence class
** will be brought to the SEJOIN CO node */
    DB_DATA_VALUE   opz_dataval;        /* datatype information about attribute
                                        */
    OPE_IEQCLS      opz_equcls;         /* the equivalence class to which this
                                        ** attribute belongs
                                        ** - this is an offset into the
                                        ** ope_eqclist array
                                        ** - OPE_NOEQCLS means no equivalence
                                        ** class has been assigned
                                        */
    OPZ_IFATTS      opz_func_att;       /* index into function attribute
                                        ** array opz_fatts
                                        ** - valid only if attribute is
                                        ** a function attribute
                                        */
    OPH_INTERVAL    opz_histogram;      /* contains type and min/max cell
                                        ** interval information for
                                        ** the histogram for this attribute
                                        */
    OPV_IGVARS      opz_gvar;           /* global range variable number 
                                        ** associated with this attribute,
                                        ** - used by query compilation
                                        ** to avoid special case checking
                                        ** for subselect joinop range vars
                                        */
    i4              opz_mask;           /* mask of booleans */
#define                 OPZ_MAINDEX     1
/* TRUE - is at least one multi-attribute index is defined using this
** attribute */
#define			OPZ_SPATJ	2
/* TRUE - this col is involved in a wacky spatial join eqclass */
#define			OPZ_QUALATT	4
/* TRUE - this col is referenced in a where/on clause */
#define			OPZ_COMPATT	8
/* TRUE - this is a dummy OPZ_ATTS to carry OPH_INTERVAL for comp hist. */
#define			OPZ_COLLATT	16
/* TRUE - this is a resdom att, created in the presence of a collation seq */
#define			OPZ_VAREQVAR	32
/* TRUE - this attribute is below an ADI_EQ_OP PST_BOP node with PST_VAR nodes
** left and right */
    OPB_BFARG       *opz_keylistp;      /* ordered list of keys used in the
                                        ** query */
    OPV_BMVARS	    opz_outervars;	/* used for outer joins,
					** - the set of variables which there
					** exists an attribute in the same
					** equivalence class, and that variable
					** is an outer to the variable represented
					** by this attribute
					** - used to determine the set of 
					** attributes which can be used to
					** evaluate a qualification which
					** references this attribute */
/* this would be useful in distinguishing between the following cases
** select * from (r left join s on r.a=s.a) left join t on s.a=t.a and r.a>5
** with the case
** select * from (r left join s on r.a=s.a) left join t on s.a=t.a and s.a>5
** in the first case the associative law cannot be used without new OPC operators
** but in the second case the associative law can be used 
** select * from r left join (s left join t on s.a=t.a and s.a>5)on r.a=s.a
*/
    OPE_BMEQCLS	    opz_eqcmap;		/* Bit map of EQCs that contain this
					** attr. This was introduced to deal
					** with inner attrs of an outer 
					** join that are in equijoin preds 
					** with more than one attr from the
					** outer side of the join. The 2 
					** comparand attrs are NOT in the 
					** same EQC by virtue of OJ 
					** semantics */
}   OPZ_ATTS;

/*}
** Name: OPZ_AT - array of pointers to attributes used in query
**
** Description:
**      This structure contains an array of pointers to the attributes used
**      in the query.  Each element of this array will have a unique
**      range variable/ attribute number pair.  
**
** History:
**     7-mar-86 (seputis)
**          initial creation
[@history_line@]...
*/
typedef struct _OPZ_AT
{
    OPZ_ATTS      *opz_attnums[OPZ_MAXATT];/* array of pointers to
                                           ** attributes
                                           */
}   OPZ_AT;

/*}
** Name: OPZ_ATTNUMS - information about attributes used in the query
**
** Description:
**      This structure contains an array of pointer to elements which describe
**      the attributes used in the query.  Also, it contains any indexes which
**      are used to access this array.
**
** History:
**     7-mar-86 (seputis)
**          initial creation
[@history_line@]...
*/
typedef struct _OPZ_ATSTATE
{
    OPZ_IATTS       opz_av;             /* next free slot in the array of
                                        ** pointers to attributes
                                        ** - elements 0 to (opz_av-1)
                                        ** of opz_attnums[] are used
                                        */
    OPZ_AT          *opz_base;          /* pointer to base of array of 
                                        ** pointers to attributes
                                        */
}   OPZ_ATSTATE;

/*}
** Name: OPZ_FACLASS - function attribute class
**
** Description:
**	This structure will describe the type of function attribute created
**      or the current state of processing of this operand; i.e. while
**      analyzing a qualification which contains a function attribute,
**      the other operand may be a simple var.  The OPZ_VAR, and OPZ_NULL
**      and OPZ_QUAL are only used during the initialization of the function
**      attribute array.  There is a decreasing order of priority of defines 
**      which should not be changed.
**          A new class of function attributes has been created to evaluate
**      function aggregates.  This class does not need to follow the rules
**      specified above since rules for the evaluation of function aggregates
**      are different.
**
** History:
**      20-may-86 (seputis)
**	    initial creation.
**      25-sep-91 (seputis)
**          added OPZ_FUNCAGG to support flattened aggregates.
**	20-may-92 (anitap)
**	    added comments for pseudo tids to support union views.
[@history_line@]...
*/
typedef i4  OPZ_FACLASS;

#define			OPZ_NULL        0
/* - indicates that this element is not being used 
*/
#define			OPZ_VAR		1
/* indicates that the branch of the qualification is a simple VAR node
*/
#define                 OPZ_TCONV       2
/* type conversion function 
*/
#define                 OPZ_SVAR        3
/* function whose parameters reference attributes from a single range 
** variable 
*/
#define                 OPZ_MVAR        4
/* function whose parameters reference attributes from multiple range 
** variables 
*/
#define                 OPZ_QUAL	5
/* - this value is used during the building of function attributes
** - if a node contains this type then opz_function will contain
** the "and" node of the qualification i.e. the function attributes
** have not been created
** - any qualification which cannot be successfully converted to a function
** attribute will be returned into the qualification list.
*/
#define                 OPZ_FUNCAGG     6
/* this function attribute represents a function aggregate, opz_function
** will contain the resdom, aop and aggregate expression */
#define                 OPZ_PSEUDOTID   7
/* this represents a unique identifier attribute which is used only
** for duplicate semantics involving relations without TIDs or unique
** identifiers (e.g. union views),... it is manufactured by QEF by
** incrementing a counter while creating the project restrict action for
** the virtual relation... opz_function will contain the resdom and 
** increment instruction. This should be recursive and OPC should take
** care of putting this count into the tuple buffer before passing it on
** to the PR node. It can then use this value to increment for the tuple.
** Right now, a combination of target list attributes (printing resdoms
** and other attributes (non-printing resdoms) are used as tids. But for
** union all, we need to keep all the duplicates and so we will need unique
** tids.
*/

/*}
** Name: OPZ_FOPERAND - information about a branch of a qualification
**
** Description:
**      This structure is used to contain information useful in processing
**      function attributes.  It will contain information about the left
**      or right branch of a qualification.
**
** History:
**     22-apr-86 (seputis)
**          initial creation
**     21-may-93 (ed)
**	    add opz_opmask to help describe operand
**     6-may-94 (ed)
**          2 conjuncts exist for the null join and both need to be
**	    replaced in case the outer join test needs to be made or
**	    the null join insertion test fails.
[@history_line@]...
*/
typedef struct _OPZ_FOPERAND
{
    OPV_GBMVARS     opz_varmap;         /* varmap of this branch of the
					** qualification
					*/
    OPZ_FACLASS     opz_class;          /* type of function attribute
					** operand
                                        */
    PST_QNODE       *opz_nulljoin;      /* if this is part of a null join then
                                        ** this fragment is the OR which is used
                                        ** to reconstruct the boolean factor
                                        ** in case the NULL join cannot be
                                        ** made */
    i4		    opz_opmask;		/* mask of values describing
					** operand */
#define                 OPZ_OPIFTRUE    1
/* TRUE if this is a NULL join and an outer join check indicates that
** an iftrue function needs to be placed on top of this operand */
}   OPZ_FOPERAND;

/*}
** Name: OPZ_FATTS - function attribute element
**
** Description:
**      Function attributes are created for type conversions of attributes
**      as well as functions of attributes.  Currently, there is no constant
**      folding of expressions, or dynamic creation of keys once the QEP
**      has been defined.  Expressions that are detected on one side of
**      a join are "converted" to an function attribute, since the joinop
**      model can deal with this.
**          The use of function attributes has been extended to allow function
**      aggregates to be evaluated at interior QEP nodes.
**
** History:
**     17-mar-86 (seputis)
**         initial creation.
**     20-mar-90 (seputis)
**	   added opz_mask with OPZ_OJFA and OPZ_SOJQUAL to support outer joins.
**     12-sep-91 (seputis)
**         added opm_fagg to support function aggregate optimizations.
**     13-apr-92 (seputis)
**          - bug 43569 - added opz_nulljoin
**     20-may-92 (anitap)
**	   added comments for pseudo tids.	
**	8-jul-97 (inkdo01)
**	    added OPZ_OJSVAL to indicate SVARs inner to an outer join, and 
**	    opz_ojid to indicate ON clause funcattr is part of.
**	23-sep-05 (hayke02)
**	    Added opz_ijmap. This is varp->opv_ijmap if this is an OJSVAR
**	    func att in the WHERE clause (OPL_NOOUTER opz_ojid). This change	
**	    fixes bug 114912.
[@history_line@]...
*/
typedef struct _OPZ_FATTS
{
    PST_QNODE       *opz_function;      /* - query tree fragment representing
                                        ** the function
					** - will contain a resdom node which
                                        ** contains the result datatype, and
                                        ** the expression to evaluate on the 
					** right branch
                                        ** - if opz_fagg is not null then this will
                                        ** contain the PST_AOP with the associated
                                        ** parse tree aggregate expression to
                                        ** evaluate in the aggregate filter.
					** - will contain the resdom and 
					** increment instruction.
					**		\
					** 		+
					**	     /     \ 
					**        PST_VAR   1	
                                        */
    OPZ_IATTS       opz_attno;          /* index into opz_attnums array
                                        ** - joinop attribute number 
                                        ** representing the function result
                                        */
    OPZ_IFATTS	    opz_fnum;		/* index into function attribute
					** array of this element
					*/
    OPZ_FACLASS	    opz_type;           /* class of function attribute */
    bool            opz_computed;       /* TRUE if the function been computed
                                        ** - used by query compilation
                                        */
    OPL_IOUTER	    opz_ojid;		/* ID of outer join to which this 
					** func attr applies. Used to determine
					** where to compute func attrs.
					*/
    OPL_BMOJ	    *opz_ijmap;		/* if this is an OPZ_OJSVAR func att
					** in the WHERE clause then this is
					** varp->opv_ijmap, otherwise NULL
					*/
    OPE_BMEQCLS     opz_eqcm;		/* equivalence classes in this 
                                        ** function
                                        ** - all these equivalence classes
                                        ** must be available before the
                                        ** function attribute can be calculated
                                        */

/* the following elements help to determine whether a function attribute
** is necessary... these values are only valid if this node is a OPZ_QUAL
*/
    DB_DATA_VALUE   opz_dataval;	/* result data type of the
					** qualification
					*/
    OPZ_FOPERAND    opz_left;           /* information about the left branch
					** of the qualification
					*/
    OPZ_FOPERAND    opz_right;          /* information about the right branch
					** of the qualification
					*/
    i4		    opz_mask;		/* mask of booleans used for defining
					** state of function attribute */
#define                 OPZ_OJFA        1
/* TRUE if this function attribute was created for the "null indicator"
** for an outer join, in which case it should not be used as a joining
** attribute, except when the "any aggregate" eventually gets optimized 
** - i.e. this is the so-called "special equivalence class" indicator in
** the function attribute */
#if 0
not needed since strength reduction should solve this problem
#define                 OPZ_SOJQUAL     2
/* TRUE if outer join requires that this qualification remain a boolean factor
** even if it is used for an equivalence class */
#endif
#define                 OPZ_FASORT      4
/* TRUE - if this is a function aggregate and a mini-sort is needed due to 
** a combination of distinct and non-distinct aggregates being evaluated on
** tables in same from list */
#define                 OPZ_NOEQCLS     8
/* TRUE - if equivalence classes are not available for use in the function
** attribute definition */
#define			OPZ_OJSVAR	16
/* TRUE - if fattr is OPZ_SVAR, but ref'd column is in table inner to an
** outer join. Must wait 'til the oj to project the fattr to get right value */
    struct _OPM_FAGG	*opz_fagg;  /* NULL if not a function aggregate
                                    ** - ptr to descriptor of function aggregate
                                    */
}   OPZ_FATTS;

/*}
** Name: OPZ_FT - table of pointers to function attributes
**
** Description:
**      Initially, this array of pointers is set to NULL and elements
**      are assigned to first NULL entry when needed.
**
** History:
**     16-mar-86 (seputis)
**          initial creation
[@history_line@]...
*/
typedef struct _OPZ_FT
{
    OPZ_FATTS       *opz_fatts[OPZ_MAXATT]; /* array of pointers to function
                                         ** attribute elements
                                         */
}   OPZ_FT;

/*}
** Name: OPZ_SFUNCTION - structure defining state of function attributes
**
** Description:
**      This structure contains elements which defines the state of the
**      function attributes used in the subquery.
**
** History:
**     20-apr-86 (seputis)
**          initial creation
**      24-jul-88 (seputis)
**          init opz_fmask, partial fix for b6570
[@history_line@]...
*/
typedef struct _OPZ_SFUNCTION
{
    OPZ_IFATTS	    opz_fv;             /* next free slot in the array of
                                        ** pointers to function attributes
                                        ** - elements 0 to (opz_fv-1) of
                                        ** opz_fatts are used
                                        */
    OPZ_FT          *opz_fbase;         /* ptr to base of array of ptrs
                                        ** to attributes
                                        */
    bool	    opz_fmask;		/* mask of flags marking function
					** attribute state */
#define                 OPZ_MAFOUND     1
/* bit is set if a multi-variable function attribute is found */
#define			OPZ_FAFOUND	2
/* bit is set if an aggregate function attribute has been defined */
}   OPZ_SFUNCTION;
