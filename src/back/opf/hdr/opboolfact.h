/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: OPBOOLFACT.H - file contains structures defining boolean factors
**
** Description:
**      The header file contains the structures which defines boolean factors
**      which are of interest to the joinop phase of processing.
**
** History: 
**      10-mar-86 (seputis)
**          initial creation
[@history_line@]...
**/

/*}
** Name: OPB_ATBFLST - list of attributes used for keys
**
** Description:
**      List of joinop attributes to use for keys, and boolean factors
**      that have attributes for them.
**
** History:
**     10-mar-86 (seputis)
**          initial creation
**	12-jan-91 (seputis)
**	    add support for better project restrict keying estimates
**	28-jan-91 (seputis)
**	    fix casting problem
**	22-apr-91 (seputis)
**	    fixed 12-jan-91 problem in different way, removed new field
[@history_line@]...
*/
typedef struct _OPB_ATBFLST
{
    OPZ_IATTS       opb_attno;          /* index into opz_attnums
                                        ** - joinop attribute number 
                                        ** - represents the attribute which
                                        ** is part of the key upon which
                                        ** the relation is ordered
                                        */
    OPB_IBF         opb_gbf;            /* index to opb_boolfact array element
                                        ** - only the first opb_bfcount elements
                                        ** will contain defined data for opb_gbf
                                        ** - in order for a limiting boolean
                                        ** factor to be useful ... all
                                        ** preceeding elements in this array
                                        ** must have a limiting predicate
                                        ** associated with it
                                        ** - the matching boolean factor
                                        ** represents the "best boolean factor";
                                        ** i.e. contains ptr to OPB_GLOBAL data
                                        ** (the max and min values have been
                                        ** determined from all boolean factors)
                                        ** for range type boolean factors
                                        ** only
                                        */
}   OPB_ATBFLST;

/*}
** Name: OPB_KA - array of descriptors for a multi-attribute key
**
** Description:
**      list of joinop attributes contained in the storage structure
**      and involved in the query
**
** History:
**      23-aug-87 (seputis)
[@history_template@]...
*/
typedef struct _OPB_KA
{
    OPB_ATBFLST    opb_keyorder[DB_MAXKEYS]; /* key list of joinop attributes
                                        ** and respective limiting predicates
                                        ** on those attributes, in order of
                                        ** key sequence, followed by any
                                        ** useful attributes not in the key
					** - note that there may be more than
                                        ** DB_MAXKEYS elements allocated for
                                        ** this array
                                        */
}   OPB_KA;

/*}
** Name: OPB_MBF - matching boolean factors associated with a keylist
**
** Description:
**      This structure contains a list of elements which describe the
**      keys upon which a relation is ordered and also contains info on any
**      matching boolean factors.
**
**      This is a keylist of joinop attributes upon which the relation
**      is ordered.  It corresponds to the a subset of the DMF keylist; i.e.
**      it is equivalent to the DMF keylist until an DMF attribute
**      was found which was not referenced in the query ... the remaining DMF
**      keylist attributes are then ignored
**
**      Currently only the limiting predicate associated with opb_keylist[0] 
**      will be analyzed by the enumeration phase although other boolean 
**      factors may be in this list !!
**
**	Query compilation uses this structure to find the boolean factors
**      which contain the keys to be used for lookup
**
** History:
**     2-may-86 (seputis)
**          initial creation
**	5-jan-93 (ed)
**	    bug 48049 - add opb_prmaorder to describe ordering with 
**	    constant equivalence classes
**     06-mar-96 (nanpr01)
**	    Use the width consistently with DMT_TBL_ENTRY definition. 
[@history_line@]...
*/
typedef struct _OPB_MBF
{
    i4             opb_bfcount;         /* first "n" elements in
                                        ** opb_keylist which have boolean
                                        ** factors associated with them
                                        */
    i4             opb_count;           /* total number of elements defined
                                        ** in the opb_keylist structure 
                                        ** whether boolean factors exist or not
                                        */
    OPS_WIDTH      opb_keywidth;        /* the width of the key used for a
                                        ** directory search for BTREE or ISAM
                                        ** which means the entire tuple width
                                        ** for a secondary index, and the width
                                        ** of the key attributes + 4 for a 
                                        ** primary 
                                        */
    i4             opb_mcount;          /* number of attributes in key */
    struct _OPO_EQLIST *opb_maorder;	/* ptr to list of equivalence classes
					** which describe this multi-attribute
					** ordering, NULL if not initialized */
    struct _OPO_EQLIST *opb_prmaorder;	/* ptr to list of equivalence classes
					** which describe this multi-attribute
					** ordering without constant equivalence
					** classes, NULL if not initialized */
    OPB_KA	   *opb_kbase;		/* keylist of joinop attributes
                                        ** and respective limiting predicates
                                        ** on those attributes, in order of
                                        ** key sequence, followed by any
                                        ** useful attributes not in the key
                                        */
}   OPB_MBF;

/*}
** Name: OPB_PQBF - partition qualification boolean factors associated with 
**	table partitioning columns
**
** Description:
**	This structure is used to indicate which boolean factors are
**	possible candidates for partition qualification (also called
**	partition elimination or partition pruning.)  Each partitioned
**	table points to one of these from the op-variable structure.
**
**	There is one bitmap showing which boolean factors might apply
**	to any simple (single-column) partitioning dimension, and one
**	bitmap per composite (multi-column) dimension showing which
**	boolfacts might apply to that dimension.
**
**	The reason for separating the two is that for a single-column
**	partitioning scheme, it doesn't matter how dimensions are
**	combined in a boolean factor.  A boolfact like
**	    (col1 = 1 OR col2 = 'foo')
**	where col1 and col2 are in two different dimensions can be
**	evaluated just fine.  On the other hand, a composite, multi-column
**	dimension needs all of the (leading) column values to occur
**	together so as to build up a composite partitioning scheme value.
**	Until we arrange some sort of DNF-based analysis of composite
**	schemes, like opckey does with multi-column keys, we impose
**	significant restrictions on the detection of composite dimension
**	qualification.
**
**	The PQBF structure also carries a pair of equivalence-class
**	bitmaps, one showing which eqclasses correspond to simple one-column
**	dimensions, and the other showing eqclasses corresponding to all
**	of the partitioning columns.  In both cases the bitmaps are
**	restricted to columns / eqc's that actually appear somewhere
**	in the subquery.  Composite dimensions that only appear in
**	part are excluded in their entirety.
**
**	Note: the PQBF structure identifies all boolfacts that might
**	assist with partition qualification for the table, but that is
**	not the whole story;  at suitable join nodes, we also need to
**	consider join eqclasses which are implicit "t1.a = t2.b" predicates
**	that do not appear as such in the boolfact table.
**
** History:
**	14-jan-04 (inkdo01)
**	    Created (from OPB_MBF).
**	31-Jul-2007 (kschendel) SIR 122513
**	    Overhaul partition qualification mechanisms, make this struct
**	    into a bunch of bitmaps.
**	8-Aug-2007 (kschendel) SIR 122513
**	    Put a selectivity estimate back in.
[@history_line@]...
*/

typedef struct _OPB_PQBF
{
    OPB_BMBF	*opb_sdimbfm;		/* Bitmap of boolfacts that are
					** candidates for doing qualification
					** against any of the simple
					** dimensions of this table;
					** NULL pointer if none
					*/

    OPB_BMBF	*opb_mdimbfm[DBDS_MAX_LEVELS];  /* Bitmap of boolfacts that
					** are candidates for doing qual
					** against this composite dimension;
					** NULL for single-column dims
                                        */

    OPE_BMEQCLS	opb_sdimeqcs;		/* Bitmap of eqclasses that appear in
					** the subquery (anywhere), and are
					** single-column partitioning cols.
					** (note: inline, not pointer)
                                        */

    OPE_BMEQCLS	opb_alldimeqcs;		/* Bitmap of eqclasses that appear in
					** the subquery (anywhere), and are
					** partitioning scheme columns.
					** (note: inline, not pointer)
					*/

    OPE_IEQCLS	*opb_dimeqc[DBDS_MAX_LEVELS];  /* Pointers to arrays of eqc's
					** that are the partitioning columns,
					** in the order in which they appear
					** in the partitioning scheme.
					** Dim not referenced in the query may
					** have the pointer filled in, but its
					** columns will be listed as NOEQCLS.
					*/

    i4		opb_pqcount;		/* Wild guess at number of physical
					** partitions that will be selected at
					** an ORIG if all boolfacts are applied
					*/

    i4		opb_pqlcount[DBDS_MAX_LEVELS];  /* Work area of selected-
					** so-far in logical partitions for
					** each dimension, used in calculating
					** the above mentioned wild guess.
					*/

    /* This last thing is kind of stupid, but I don't see any other
    ** sensible way...
    ** QEN_PART_QUAL structures are linked such that join-time pqual
    ** blocks point to the ORIG node that they are qualifying for.
    ** (Not only does this establish the relationship, it allows the
    ** join-level pquals to omit certain redundancies, such as extra
    ** copies of the partition definition.)
    ** Unfortunately there appears to be no rational way to locate an
    ** ORIG node given the local range variable.  Anyway, it might not
    ** be a real ORIG, the relevant table might be the inner of a
    ** K-join or T-join.  About the only thing I could think of was
    ** to stick a pointer to the orig-like QP node somewhere, and
    ** this is as good (or bad) a place as any.
    ** By the way, partition qualification doesn't reach past a subquery,
    ** which is a good thing since the PQBF struct is attached to
    ** the local range variable.
    */

    QEN_NODE	*opb_source_node;	/* Filled in at OPC time */

}   OPB_PQBF;

/*}
** Name: OPB_SARG - boolean factor classification
**
** Description:
**	The values that this structure may have are obtained directly from
**      the keybuilding routine of the ADF.  Note, that the optimizer depends
**      on the following relative ordering of these constants
**
**	ADC_KNOMATCH - equivalent to "KFALSE"; i.e. key will will always be 
**	    false
**      ADC_KEXACTKEY - equivalent to "KEQ"; i.e. all predicates are 
**          equality sargs on the same equivalence class
**      ADC_KRANGEKEY - equivalent to "KLTGT"; i.e. there is at least one 
**	    inequality min sarg and at least one inequality max sarg ... as 
**	    well as possible equality sargs on the same equivalence class
**	ADC_KHIGHKEY - equivalent to "KLT"; i.e. there is one inequality 
**	    max sarg ... as well as possible equality sargs
**      ADC_KLOWKEY - equivalent to "KGT"; i.e. there is one inequality 
**	    min sarg ... as well as possible equality sargs
**	ADC_KNOKEY - cannot use keyed access - all predicates may be on the 
**	    same equivalence class but not all are equality or inequality sargs
**	    OR more than equivalence class used in boolean factor
**      ADC_KALLMATCH - equivalent to "KTRUE"; i.e. boolean factor useless 
**	    since all tuples will satisfy restriction
**
** History:
**      20-may-86 (seputis)
**          initial creation
[@history_line@]...
*/
typedef i4  OPB_SARG;

/*}
** Name: OPB_BFVALLIST - list of key and histogram values
**
** Description:
**      This structure represents both optimization time and runtime key
**      and histogram values associated with a boolean factor.  The key and
**      histogram values are computed at runtime only if the constants
**      exist.  This would not occur in the case of parameter constants
**      and constant expressions.  Constant expressions are computed at
**      runtime since there may be a possibility that these expressions
**      are time dependent e.g. the current time constant.  A future
**      enhancement would be to compute all constant expressions which are not
**      time dependent.
**
**      If the constant does exist then this element is part of a list of 
**      values with pattern matching and key conversion processing done.
**
** History:
**     20-mar-86 (seputis)
**          initial creation
**	19-oct-00 (inkdo01)
**	    Added opb_connector to help selectivity estimates with composite
**	    histograms.
[@history_line@]...
*/
typedef struct _OPB_BFVALLIST
{
    struct _OPB_BFVALLIST *opb_next;    /* linked list of values 
                                        */
    OPB_SARG        opb_operator;       /* type of key represented here
                                        ** - see definition of OPB_SARG
                                        */
    PTR             opb_keyvalue;       /* Ptr to key value created at
                                        ** at optimization time.
                                        ** -NULL if no key can be created - this
                                        ** would occur if there existed a
                                        ** constant expression or constant
                                        ** parameter.
                                        ** - the length of this element is the
                                        ** same length as the joinop attributes
                                        ** associated with this key;... equal to
                                        ** OPB_BFKEYINFO->opb_bfdt.db_length
                                        ** which is found in the header for 
                                        ** this list of keys
                                        */
    PTR             opb_hvalue;         /* Ptr to histogram value of 
                                        ** *opb_keyvalue
                                        ** - this element is NULL iff
                                        ** opb_keyvalue is NULL
                                        ** - if the histogram length is equal
                                        ** to the key length then this ptr
                                        ** will be equal to opb_keyvalue
                                        ** - the size of this element is
                                        ** found in the histogram length field
                                        ** of the joinop attributes associated
                                        ** with this key list; i.e. equal to
                                        ** OPB_BFKEYINFO->opb_bfhdt.db_length
                                        ** which is found in the header for 
                                        ** this list of keys
                                        */
    ADI_OP_ID	    opb_null;           /* 0 if this clause does not represent
                                        ** a "ISNULL" or an "IS NOT NULL"
                                        ** otherwise the opid is saved
                                        */
    ADI_OP_ID	    opb_opno;		/* operator used to create this
					** key value */
    i4		    opb_connector;	/* for concatenated keys of composite
					** histograms - indicates if this 
					** value is ANDed or ORed */
#define		OPB_BFAND	1
#define		OPB_BFOR	2
}   OPB_BFVALLIST;

/*}
** Name: OPB_BFARG - part of list of sargable clauses used for histogramming
**
** Description:
**      In order to more accurately use histogram processing, the cells will
**	be split immediately after being read from RDF, along boundaries which
**	define
[@comment_line@]...
**
** History:
**      7-may-91 (seputis)
**          initial creation for b37172
**      15-dec-91 (seputis)
**          fix 8 character conflict in typedef name
[@history_template@]...
*/
typedef struct _OPB_BFARG
{
    OPB_BFVALLIST    *opb_valp;         /* ptr to descriptor for sargable clause */
    struct _OPB_BFARG *opb_bfnext;     /* ptr to next element is list, in order */
}   OPB_BFARG;

/*}
** Name: OPB_BFGLOBAL - global portion of keyinfo structure
**
** Description:
**      This structure contains information which applies to
**      the all sargable boolean factors which apply to the same 
**      equivalence class (i.e. only one equivalence class is referenced in
**      the boolean factor).  It contains a "summary" of information on this
**      group of boolean factors.  
**
**      The opb_pmin will point to the OPB_BFVALLIST struct found in opb_keylist
**      which is a range type boolean factor i.e. OPB_BFKEYINFO.opb_sargable
**      is ADC_KLOWKEY or ADC_KRANGEKEY.  opb_pmin will point to
**      the maximum of all the minimums for all boolean factors with the same
**      equivalence class, datatype, and length.  This is the value which will
**      be used in histogram processing is eventually in query compilation
**      for keyed lookup (if no parameter constants exists for this attribute).
**   
**      A similiar argument applies for opb_pmax except that it operates on
**      ADC_KHIGHKEY ADC_KRANGEKEY.
**
**      The opb_pmin, and opb_pmax values are used during histogram
**      processing in ordering to estimate the cost of a restrict on the
**      base relation.  The assumption is that these values for parameter
**      constants are characteristic of the future values.
**      Since it includes parameter constants which may
**      change, these values may not be used in query compilation if parameter
**      constants exist.  In this case, the QEF will need to be given a 
**      list of values which will be used to search for the min and max keys 
**      at execution time.  If no parameter constants exist then opb_pmin and
**      opb_pmax can be used.
**
** History:
**     20-jun-86 (seputis)
**          initial creation
**     10-jul-88 (seputis)
**          added fields for union optimization
**	26-apr-93 (ed)
**	    add mask to indicate that ON clause boolean factors cannot
**	    eliminate partitions within a union view bug 49821
[@history_line@]...
*/
typedef struct _OPB_BFGLOBAL
{
    OPB_BFVALLIST     	*opb_pmin;	/* if m1, m2 ... mn are the 
                                        ** minimums of all constants (including
                                        ** any available parameter constants)
                                        ** that can be used for keyed lookup
                                        ** in the n'th sarg boolean factor with
                                        ** the same eqcls class, type and length
                                        ** where opb_sargtype is ADC_KLOWKEY
                                        ** or ADC_KRANGEKEY
                                        ** - this value is a ptr to the maximum
                                        ** of all the m1, m2 ... mn
                                        */
    OPB_BFVALLIST      	*opb_pmax;	/* if m1, m2 ... mn are the 
                                        ** maximums of all constants (including
                                        ** any available parameter constants)
                                        ** that can be used for keyed lookup
                                        ** in the n'th boolean factor with the
                                        ** same eqcls class, type and length
                                        ** where opb_sargtype is ADC_KHIGHKEY
                                        ** or ADC_KRANGE KEY
                                        ** - this value is a ptr to the minimum
                                        ** of all the m1, m2 ... mn
                                        */
    OPB_BFVALLIST	*opb_xpmin;	/* min used for union optimization */
    OPB_BFVALLIST	*opb_xpmax;	/* max used for union optimization */
    OPB_IBF          opb_single;        /* sargable boolean factor element 
                                        ** which is an ADC_KEXACTKEY with one
                                        ** value for this equivalence
                                        ** class type and length
                                        ** - OPB_NOBF - if none exists
                                        */
    OPB_IBF          opb_multiple;      /* sargable boolean factor element 
                                        ** which is a ADC_KEXACTKEY with one 
                                        ** or more values for this equivalence
                                        ** class type and length
                                        ** - OPB_NOBF - if none exists
                                        */
    OPB_IBF          opb_range;         /* sargable boolean factor element 
                                        ** which is a range type boolean factor
                                        ** ADC_KHIGHKEY, ADC_KLOWKEY
                                        ** ADC_KRANGEKEY
                                        ** - OPB_NOBF - if none exists
                                        */
    i4		    opb_bfmask;		/* mask of booleans */
#define                 OPB_OJSARG      1
/* TRUE - if an outer join conjunct was used to help create the ranges
** in this structures,... this implies some partition elimination algorithms
** cannot be used since outer joins do not eliminate all tuples */
}   OPB_BFGLOBAL;

/*}
** Name: OPB_BFKEYINFO - boolean factor key information
**
** Description:
**      This structure is used as an element of a list which describes
**      a key which can be used to restrict values indicated by the boolean
**      factor.
**
**      If the opb_sargable value is ADC_KEXACTKEY then the opb_keylist items
**      will be sorted by value.
**
** History:
**     15-mar-86 (seputis)
**          initial creation
*/
typedef struct _OPB_BFKEYINFO
{
    struct _OPB_BFKEYINFO    *opb_next; /* a list of these, one for each type
                                        ** and length of the attributes in
                                        ** the equivalence class associated
                                        ** with the boolean factor
                                        */
    OPE_IEQCLS	    opb_eqc;            /* boolean factor equivalence class
                                        ** - i.e. offset into ope_eqclist array
                                        */
    DB_DATA_VALUE   *opb_bfdt;          /* ptr to ADF datatype associated 
                                        ** with this keylist
                                        */
    DB_DATA_VALUE   *opb_bfhistdt;      /* ptr to ADF histogram datatype
                                        ** associated with this keylist
                                        */
    i4              opb_used;           /* TRUE if this boolean factor has
                                        ** been considered in selectivity
                                        ** - used when trying to find minimum
                                        ** value pointer and maximum value
                                        ** pointer among all boolean factors
                                        ** - also used inside clauseval to
                                        ** indicate boolean factor is processed
                                        */
    OPB_SARG        opb_sargtype;       /* classify boolean factor as to
                                        ** what type of keyed relation access
                                        ** can be done
                                        ** - the ADF codes are used directly
                                        ** here.
                                        */
    OPB_BFVALLIST    *opb_keylist;      /* list of values to be used by
                                        ** histogram processing or for keyed
                                        ** relation access.
                                        ** - necessary type conversions and
                                        ** pattern matching character
                                        ** processing have been done
                                        */
    OPB_BFGLOBAL     *opb_global;       /* contains information which
                                        ** applies to all OPB_BFKEYINFO
                                        ** structures of the same equivalence
                                        ** class, datatype and length.
                                        */
}   OPB_BFKEYINFO;

/*}
** Name: OPB_VIRTUAL - virtual table summary for boolean factor
**
** Description:
**      This structure is defined when the boolean factor contains a variable
**      which represents a SELECT, or a correlated aggregate.  The SELECT
**      could restrict placement of the respective variable on the inner, and
**      must have ALL attributes available for evaluation of the boolean factor
**      at the immediate parent of the SELECT leaf node.  The
**      correlated aggregate must also be evaluated on the inner, but does
**      not need to have the boolean factor as its immediate parent, because
**      the attribute representing the aggregate result can be returned.
**
** History:
**      21-jul-87 (seputis)
**          initial creation
[@history_template@]...
*/
typedef struct _OPB_VIRTUAL
{
    bool            opb_subselect;      /* TRUE if boolean factor contains
                                        ** a subselect node, note that each
                                        ** boolean factor can reference only
                                        ** one subselect variable */
    bool            opb_aggregate;      /* TRUE if boolean factor contains
                                        ** a correlated aggregate */
}   OPB_VIRTUAL;

/*}
** Name: OPB_BOOLFACT - boolean factor element
**
** Description:
**      This structure contains information gathered about a particular
**      boolean factor.
**
**      The opb_keylist has been initialized so that the order of elements
**      of a unique (type, length) within the same equivalence class, will
**      have the same relative position within the keylist, for all boolean
**      factor elements.  Thus, the elements in the opb_next list will
**      have (type,length) ordering which is identical.  This will be used
**      to process "min max" values of range type boolean factors and
**      to sort "EQ" type boolean factors.
**
** History:
**     15-mar-86 (seputis)
**          initial creation
**      26-oct-88 (seputis)
**          added opb_corelated to support distributed search space restrictions
**      4-apr-91 (seputis)
**	    - added fields to correct problems with project restrict estimates
**          - added mask used to indicate if conjunct can be used for keying subranges
**	    for enhanced project restrict keying estimates
**      18-sep-92 (ed)
**          - bug 45728 - add mask to keep track of equal key values which can
**          affect the OR count
**	21-mar-95 (brogr02)
**	    - bug 59112 - add flag to keep track of translation of boolean factors 
**		into text, for STAR
**	03-jan-96 (wilri01)
**	    Added opb_selectivity
**	19-may-97 (inkdo01)
**	    Added flags to opb_mask (OPB_COMPLEX, OPB_KEXACT).
**	3-sep-98 (inkdo01)
**	    Added flag OPB_SPATP for proper placement of spatial predicates in
**	    rtree query plans.
**	12-may-00 (inkdo01)
**	    Replaced unused OPB_COMPLEX by OPB_COMPOSITE to indicate factor 
**	    is one of several which map on a composite histogram.
**	6-jul-00 (inkdo01)
**	    Added OPB_RANDOM flag for factors w. random func refernece.
**	16-oct-00 (inkdo01)
**	    Added OPB_BFCOMPUSED to help with composite histogram processing.
**	29-may-02 (inkdo01)
**	    Changed OPB_RANDOM to OPB_NOTCONST to reflect other parm-less/const
**	    parm funcs that don't belong in constant CX.
**	 8-apr-05 (hayke02)
**	    Added OPB_PCLIKE for "like '%<string>'" predicates.
**	12-jan-06 (dougi)
**	    Added OPB_COLCOMPARE to support column comparison stats.
**	22-may-08 (hayke02)
**	    Added OPB_KNOMATCH to opb_mask. TRUE if this factor has an
**	    adc_tykey of ADC_KNOMATCH e.g. LIKE '<string>', where <string>
**	    contains no pattern matching characters and row estimate should be
**	    0% (see bug 21160). This change fixes bug 120294.
**	2-Jun-2009 (kschendel) b122118
**	    Remove FALIKE flag, was never set.
**      18-oct-2010 (huazh01)
**          OPB_RESOLVE is no longer being referenced. Rename it to 
**          OPB_NOCOMPHIST as part of fix to b124287.
[@history_line@]...
[@history_template@]...
*/
typedef struct _OPB_BOOLFACT
{
    PST_QNODE       *opb_qnode;         /* query tree node represented
                                        ** by this boolean factor element
                                        */
    OPB_VIRTUAL     *opb_virtual;       /* not NULL if this boolean factor
                                        ** contains a reference to a SELECT
                                        ** or a correlated aggregate which
                                        ** could restrict placement of leaves
                                        ** in the join tree and the location
                                        ** of evaluation of this boolean factor
                                        */
    OPH_HISTOGRAM   *opb_histogram;     /* list of normalized histograms,
                                        ** - one for each equivalence class
                                        ** referenced in a sargable predicate
                                        ** - the cell counts in this histogram
                                        ** are percentages of the cell count
                                        ** in a corresponding "opn_histogram"
                                        */
    i4              opb_orcount;        /* count of the number of or clauses
                                        ** in the boolean factor, this may
                                        ** go negative due to fix for b45728
                                        ** check for this case
                                        */
    OPE_BMEQCLS	    opb_eqcmap;		/* bit map of equivalence 
                                        ** classes referenced in the query tree
                                        ** fragment represented by this
                                        ** boolean factor element, this includes
                                        ** all equivalence classes required
                                        ** to evaluate a subselect
                                        */
    OPE_IEQCLS	    opb_eqcls;          /* offset into equivalence class array
                                        ** - if only one equivalence class in
                                        ** this boolean factor then set this
                                        ** offset to select that one, else
                                        ** set to OPB_BFMULTI if more than one
                                        ** - if more than one then cannot use
                                        ** the boolean factor as a search clause
                                        ** since more than one relation
                                        ** involved
                                        */
    bool            opb_parms;          /* TRUE if at least one parameter
                                        ** constant was found in the
                                        ** boolean factor
                                        */
    bool            opb_truequal;       /* TRUE if the boolean factor will
                                        ** always evaluate to TRUE, but has
                                        ** some parameter constants in it
                                        */
    i4		    opb_torcount;	/* temporary or count which represents
                                        ** non-sargable disjuncts,  this may
                                        ** go negative due to fix for b45728
                                        ** check for this case */
    OPB_BFKEYINFO    *opb_keys;         /* list of boolean factor keys found
                                        */
    bool	    opb_corelated;      /* TRUE - if this boolean factor contains
					** any repeat query constants which represent
					** co-related variables */
    OPL_IOUTER	    opb_ojid;		/* contains the outer join ID if it applies
					** to this boolean factor, otherwise
					** contains OPL_NOOUTER which indicates
					** this boolean factor belongs to the
					** where clause */
    OPL_BMOJ	    opb_ojmap;		/* map of outer joins which must be evaluated
					** before this boolean factor can be evaluated
					** - non-zero only if OPB_OJMAPACTIVE is set
					** in opb_mask
					** - used for nested queries like
					** select * from (r left join (s left join t
					**  on s.a = t.a) on r.a=t.a and t.a IS NULL
					** so that "t.a IS NULL" is not pushed to the
					** inner most outer join */
    OPZ_BMATTS	    *opb_ojattr;	/* map of joinop attributes referenced in this
					** boolean factor, used for placing relations
					** with respect to outer joins
					*/
    OPB_IBF	    opb_bfindex;	/* index into boolfact array of this
					** descriptor */
    i4	    opb_mask;		/* mask describing various characteristics
					** of this boolean factor */
#define                 OPB_NOCOMPHIST  0x00000001L
/* TRUE if this boolean factor can't be used to build composite  
** histogram */
#define                 OPB_ORFOUND     0x00000002L
/* TRUE - is at least one OR is involved in this query */
#define			OPB_KESTIMATE	0x00000004L
/* TRUE - if this boolean factor should be used in the keying
** estimate for the relation */
#define                 OPB_DNOKEY      0x00000008L
/* TRUE - if at least one disjunct of this boolean factor
** is not sargable */
#define                 OPB_NOHASH      0x00000010L
/* TRUE - if all clauses are of the form attr <relop> const */
#define                 OPB_ALLMATCH	0x00000020L
/* TRUE - if boolean factor has no selectivity at all */
#define			OPB_BFUSED	0x00000040L
/* TRUE	- if boolean factor has been used in selectivity calculations */
/* #define		notused		0x00000080L */
/* unused, used to be a flag for "expr LIKE pattern" for star, was never
** set and the issue is obsolete anyway */
#define                 OPB_DECOR       0x00000100L
/* TRUE - indicates a key was not inserted due to equality with an
** existing key which means the OR count should be decremented
** or else selectivity include a phantom 50% selectivity */
#define			OPB_OJMAPACTIVE 0x00000200L
/* TRUE - if opb_ojmap has at least one member */
#define                 OPB_OJATTREXISTS 0x00000400L
/* TRUE - if at least one outer join attribute exists in the boolean
** factor */
#define			OPB_OJJOIN	0x00000800L
/* TRUE - if this is an equi-join boolean factor, to be used for
** relation placement but should not participate in histogramming
** since special equi-join histogramming code exists for this */
#define			OPB_NOSPECIAL	0x00001000L
/* TRUE - if this is a boolean factor which contains outer join attributes
** and no iftrue conversions need to be made even though some of the attributes
** referenced come from nested outer joins */
#define 		OPB_TRANSLATED  0x00002000L
/* TRUE - if this boolean factor has been translated by opc_treetext() (STAR) */
#define			OPB_SPATF	0x00004000L
/* TRUE - if this boolean factor is a qualifying spatial function and might
** be used for Rtree index access */
#define			OPB_SPATJ	0x00008000L
/* TRUE - if this boolean factor is a qualifying spatial function on 2 columns
** and might be used for Rtree index access to solve a spatial join */
#define			OPB_COMPOSITE	0x00010000L
/* TRUE - if this Boolean factor is one of several which can be estimated using
** a composite histogram */
#define			OPB_KEXACT	0x00020000L
/* TRUE - if at least one keylist entry for this BF is EXACT (used inside
** ophrelse.c to guide key selectivity computation). */
#define			OPB_SPATP	0x00040000L
/* TRUE - if this boolean factor is a spatial function involving a column on
** which a separate spatial join has been done. It is now pending application
** at the TID join node which retrieves the underlying base table rows */
#define			OPB_NOTCONST	0x00080000L
/* TRUE - if this factor contains a function (such as random() or uuid_create())
** that produces distinct values, even though parms are constant or non-existant.
** This prevents the factor from being flagged as constant. */
#define			OPB_BFCOMPUSED	0x00100000L
/* TRUE - if this factor is single eqclass but mapped onto a composite
** histogram */
#define			OPB_PCLIKE	0x00200000L
/* TRUE - if this factor is a "like '%<string>'" predicate */
#define			OPB_COLCOMPARE	0x00400000L
/* TRUE - if this factor is a column comparison for which stats 
** were used to determine selectivity */
#define			OPB_KNOMATCH	0x00800000L
/* TRUE - if this factor has an adc_tykey of ADC_KNOMATCH e.g. LIKE '<string>',
** where <string> contains no pattern matching characters and row estimate
** should be 0% (see bug 21160) */
    struct _OPB_BOOLFACT *opb_bfp;	/* ptr to linked list of boolean
					** factor identical except for
					** presence of special eqcls tests
					*/
    OPE_BMEQCLS		*opb_special;	/* map of special eqcls which
					** need to be added with the
					** iftrue function to var nodes
					** of the boolean factor */
    OPN_PERCENT		opb_selectivity;/* A copy of opb_histogram->oph_prodarea
    					** for opc after opb_histogram is freed. */					
}   OPB_BOOLFACT;

/*}
** Name: OPB_BFT - array of pointers to boolean factor elements
**
** Description:
**      This structure defines an array of pointers to boolean factor
**      elements.
**
** History:
**     15-mar-86 (seputis)
**          initial creation
*/
typedef struct _OPB_BFT
{
    OPB_BOOLFACT    *opb_boolfact[OPB_MAXBF]; /* array of pointers to
                                             ** boolean factor elements
                                             */
}   OPB_BFT;

/*}
** Name: OPB_BFSTATE - boolean factors used in query
**
** Description:
**      This structure defines information gathered about boolean factors
**      used in the query.
**
** History:
**     15-mar-86 (seputis)
**          initial creation
**	5-jan-93 (ed)
**	    bug 48049 add opb_mbfeqc as a mask to remove all constant equivalence
**	    classes from a bitmap
**	5-jan-97 (inkdo01)
**	    Added flag to indicate presence of constant eqclasses originating 
**	    in single valued subselects. This will help solve bug 87509.
[@history_line@]...
*/
typedef struct _OPB_BFSTATE
{
    OPB_IBF         opb_bv;             /* number of boolean factors found
                                        ** - the elements from 0 - (opb_bv-1) 
                                        ** are defined in opb_boolfact[]
                                        */
    OPB_IBF	    opb_newbv;		/* new boolean factors may need to be
					** created in order to handle iftrue
					** processing for outer joins */
    OPB_BFT         *opb_base;          /* pointer to base of boolean factor
                                        ** table
                                        */
    OPE_BMEQCLS	    *opb_bfeqc;		/* Bit map of those equivalence
                                        ** classes whose value is a constant
                                        ** i.e. those boolean factors with
                                        ** a single value equality sarg.
                                        ** These boolean factors can be used
                                        ** as a key in a join.
                                        */
    OPE_BMEQCLS	    *opb_mbfeqc;	/* defined if opb_bfeqc is defined,
					** contains the complement of opb_bfeqc
					** to be used to mask out any constant
					** equivalence classes from an eqc map
                                        */
    PST_QNODE	    *opb_bfconstants;	/* this is a qualification list with
					** constants only, with at least one
					** parameter node
					** - if NULL then there are no
					** such qualifications in the query.
					** - these constant expressions will
					** be evaluated prior to executing
					** the query, and if any evaluate to
					** FALSE then the query will not need
					** to be executed since no tuples will
					** be returned
					*/
    OPE_BMEQCLS	    *opb_njoins;	/* used to process each conjunct
					** - ptr to bitmap of eqcls which exist
					** in each predicate of a conjunct */
    OPE_BMEQCLS	    *opb_tjoins;	/* used to process each conjunct
					** - temp used to build opn_njoins */
    bool	    opb_ncheck;         /* used to process each conjunct
					** - TRUE if conjuncts should be
					** analyzed to determine if any tuple
					** which qualifies for the conjunct can
					** contain NULLs in the equivalence classes
					** which make up the conjunct */
    bool            opb_qfalse;         /* TRUE -if the qualification has a
                                        ** conjunct which is always false
                                        */
    i4		    opb_mask;
#define                 OPB_NEWOJBF     1
/* TRUE if at least one boolean factor needs to be converted to use the iftrue
** function for outer join handling */
#define			OPB_GOTSPATF	2
/* TRUE if at least one spatial function boolean factor */
#define			OPB_GOTSPATJ	4
/* TRUE if at least one spatial function boolean factor on 2 cols (i.e. 
** a possible spatial join) */
#define			OPB_SUBSEL_CNST	8
/* TRUE if at least one of the constant eqclasses is the result of a
** single valued subselect */
}   OPB_BFSTATE;
