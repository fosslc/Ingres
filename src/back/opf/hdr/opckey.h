/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: OPCKEY.H - Data structures needed for keying into relations.
**
** Description:
[@comment_line@]...
**
** History: $Log-for RCS$
**      31-aug-86 (eric)
**          created
**	31-aug-92 (rickh)
**	    Removed func_externs, since OPCLINT.H contains these now.
[@history_template@]...
**/

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _OPC_QUAL    OPC_QUAL;
typedef struct _OPC_QSTATS  OPC_QSTATS;
typedef struct _OPC_KCONST  OPC_KCONST;
typedef struct _OPC_QCONST  OPC_QCONST;
typedef struct _OPC_QAND    OPC_QAND;
typedef struct _OPC_QCMP    OPC_QCMP;
typedef struct _OPC_KOR	    OPC_KOR;
typedef struct _OPC_KAND    OPC_KAND;
typedef struct _OPC_KDATA   OPC_KDATA;

/*
**  Defines of other constants.
*/

#define	    OPC_UPPER_BOUND	-3
#define	    OPC_LOWER_BOUND	3

#define	    OPC_TOP		1
#define	    OPC_NOT_TOP		2

#define	    OPC_TID		10
#define	    OPC_SANS_TID	11

/*
**  Forward and/or External function references.
*/




/*}
** Name: OPC_QSTATS - This holds general info about a key qualification.
**
** Description:
**      This structure, which is used in the OPC_QUAL struct, holds information
**      of a general nature about the key qualification that is/has/will be
**      built up.
[@comment_line@]...
**
** History:
**      1-Aug-87 (eric)
**          defined.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedefs to quiesce the ANSI C 
**	    compiler warnings.
[@history_template@]...
*/
struct _OPC_QSTATS
{
	/* Opc_nparms gives the number of repeat query parameters that are
	** used in opc_qand_tree. 
	** Opc_hiparm will be greater than the highest possible repeat query 
	** parameter number that might be used in opc_qand_tree. This means
	** that it will be at least one greater than the largest parameter,
	** so the opc_hiparm can be used for allocating parameter bit maps.
	** Opc_nconsts refers to the number of non-repeat query parameter
	** constants that are used in
	** opc_qand_tree. 
	*/
    i4		opc_nparms;
    i4		opc_hiparm;
    i4		opc_nconsts;

	/* Opc_kptype will hold either PST_RPPARAMNO or PST_LOCALVARNO, and 
	** will indicate whether the bits in opc_kphi and opc_kplo refer
	** to repeat query parameters or DBP local vars.
	*/
    i4		opc_kptype;

	/* Opc_nkeys tells how many keys are in the relation being keyed */
    i4		opc_nkeys;

	/* Opc_inequality will be TRUE if any comparison operator other than
	** an = is used or parameter matching chars are used.
	*/
    i4		opc_inequality;

	/* Opc_tidkey is TRUE if we are using the tid to key into the
	** relation, otherwise we are using the primary key.
	*/
    i4		opc_tidkey;

	/* Opc_storage_type gives the storage stucture of the relation. */
    i4	opc_storage_type;
};


/*}
** Name: OPC_KCONST - This is a list of constants for a key attribute.
**
** Description:
**      This structure contains a list of constants, both repeat query
**      parameters and not, that can be used to key into a key attribute.
**      This structure is used in the OPC_QUAL struct. Which key attribute
**      these consts can be used for is determined by the array index that
**      is used to get to this structure.
[@comment_line@]...
**
** History:
**      1-aug-87 (eric)
**          defined
**	2-Jun-2009 (kschendel) b122118
**	    Delete unused "countof" members.
[@history_template@]...
*/
struct _OPC_KCONST
{
	/* The ADC_KEY_BLK for this key attribute. */
    ADC_KEY_BLK	    opc_keyblk;

	/* Opc_qconsts is a linked list of non-repeat query parameter 
	** constants.
	*/
    OPC_QCONST	    *opc_qconsts;

	/* This is a pointer to an array of pointers to OPC_QCONST structs
	** for repeat query parameter constants. The size of the array
	** is determined by opc_szkparam. The zeroth element of this array
	** is used to hold the OPC_QCONSTs for expressions, which OPC treats
	** parameters.
	*/
    OPC_QCONST	    **opc_qparms;

	/* This is a pointer to an array of pointers to OPC_QCONST structs
	** for DBP local variable constants. The size of the array
	** is determined by opc_szkparam. The zeroth element of this array
	** is used to hold the OPC_QCONSTs for expressions, which OPC treats
	** parameters.
	*/
    OPC_QCONST	    **opc_dbpvars;

	/* Opc_nuops gives the number of comparisons for this key att that
	** don't require values.
	*/
    i4		    opc_nuops;
};


/*}
** Name: OPC_QUAL - Top structure that holds keying qualification info
**
** Description:
**      This structure holds all of the structure that OPC defines for
**      solving the relation keying puzzle. The information falls into four
**      groups: 1) The conjunctive qualification. This is very similiar to
**      a PST_QNODE tree, that holds only clauses that are usefull for keying.
**      The difference is that OPC's conjunctive qual is optimized for a rapid
**      keying solution. 2) The disjunctive qualification. This is very
**	similiar to a QEF_KOR structure. Here again, OPC's version has been
**      optimized for a rapid solution. Maybe OPC's and QEF's versions can
**      be merged someday. 3) The data that is used by the two quals above.
**      The data will be sorted so that there is no duplication representation
**      of constants, either repeat query parameter or not. 4) General info
**      about the keying task at hand (ie. are we using the primary or tid key,
**      is the key an equality key or not?, etc).
[@comment_line@]...
**
** History:
**      1-aug-87 (eric)
**          defined
[@history_template@]...
*/
struct _OPC_QUAL
{
	/* Opc_cnf_qual is a converted form of the query tree. The conversion
	** is a simple one, ie. there is ia one-to-one mapping between the
	** trees. It has been converted to aid in the generation of 
	** opc_dnf_qual.
	*/
    OPC_QAND	*opc_cnf_qual;

	/* Opc_dnf_qual is a converted form of opc_cnf_qual, only it's in
	** disjunctive normal form in a format that directly indicates
	** what sort of keying can be done on the relation. Opc_end_dnf
	** points to the last disjunctive clause on the opc_dnf_qual
	** list.
	*/
    OPC_KOR	*opc_dnf_qual;
    OPC_KOR	*opc_end_dnf;

	/* Opc_qsinfo holds general status information about this
	** keying job.
	*/
    OPC_QSTATS	opc_qsinfo;

	/* Opc_kconsts is an array of constants that are used for keying 
	** information. There will be one element from each key attribute.
	*/
    OPC_KCONST	opc_kconsts[1];
};


/*}
** Name: OPC_QCONST - Hold a const to be used by OPC cnf and dnf quals.
**
** Description:
**      This structure holds a single constant that is used by OPC's 
**      conjunctive and disjunctive normal form qualifications. The const
**      may either be a repeat query parameter or not. If it is, then the
**      opc_constno field will be < 0; otherwise this is a non-RQP.
**       
**      You will find that each OPC_QCONST is pointed to by several different
**      objects. First, the OPC_KCONST structure has a linked list of all the
**      OPC_QCONST structs that can be used to key into a given key attribute.
**      Next, one or more OPC_QCMP structs may point at this const indicating
**      the this value is either the upper or lower (or both) bounds for the 
**      comparison.
[@comment_line@]...
**
** History:
**      1-aug-87 (eric)
**          defined
**      30-july-91 (ERIC)
**          Changed opc_data from a PTR to a DB_DATA_VALUE, thus making
**          this structure more self sufficient.
[@history_line@]...
[@history_template@]...
*/
struct _OPC_QCONST
{
	/* Opc_keydata holds the data in question. If this is a repeat query
	** parameter, then the opc_pstnode field should be used and will
	** point to the query tree expression. Otherwise, the opc_data field
	** should be used and will point to an array of bytes that is large
	** enough for this constant. Note that we don't need to store the
	** type and length of the data that opc_data points to since it only
	** points to values that adc_keybld() has produced. Hence, the type
	** and length is the same as the type and length for the key attribute
	** that this will be used to key into.
	*/
    union
    {
        DB_DATA_VALUE	opc_data;
	PST_QNODE	*opc_pstnode;
    } opc_keydata;

	/* The next constant on the linked list of constants that hangs off of
	** the OPC_KCONST struct.
	*/
    OPC_QCONST	*opc_qnext;

	/* Opc_const_no is the constant number for this constant.
	** Whether opc_const_no is a repeat query parameter, or a DBP local
	** var should be determined by OPC_QSTATS.opc_kptype
	*/
    i4		opc_const_no;
    i4		opc_tconst_type;

	/* Opc_qlang tells whether this is an SQL or QUEL constant. */
    DB_LANG	opc_qlang;

	/* This is the PSF comparison operator node that is used with this 
	** constant. This is filled in only for repeat query parameters (ie
	** opc_constno < 0). This is because the keybld for repeat query params
	** don't happen until the plan is executed, and the opid is needed for
	** keyblding.
	*/
    PST_QNODE	*opc_qop;

	/* Opc_dshlocation is the location of this constant when it has been
	** compiled into QEF's DSH key array. If this constant hasn't been 
	** compiled yet then opc_dshlocation will be -1.
	*/
    i4		opc_dshlocation;

	/* Opc_key_type holds the key type returned from ADC_KEYBLD that
	** this repeat query parameter is involved in. If this isn't a
	** repeat query parameter then this field should not be used (but it
	** will be initialized to ADC_NOKEY, just for completeness).
	*/
    i4		opc_key_type;
};


/*}
** Name: OPC_QCMP - Describe a comparison for a CNF qual
**
** Description:
**      This struct is a translated version of a PST_QNODE comparison between
**      a var node and a const node or an expression of const nodes. We have
**      gone to the effort of doing the translation because this node allows
**      us to refer to data that has been run through adc_keybld() (assuming
**      that the it's not a RQP const node or an expression of const nodes).
**      This means that all of the data for a given key attribute is of the
**      same type and length, which means that all of the data can be ordered 
**      and numbered. This means that when we want to compare two pieces of 
**      data that we can do it using thier numbers rather than the data itself.
**      This structure is only used in the OPC_QAND struct.
[@comment_line@]...
**
** History:
**      1-aug-87 (eric)
**          defined
**      20-june-91 (eric)
**          Removed the opc_opid field from this struct because: 1) it was
**          not used except in one place that could use a stack variable
**          instead, and 2) The manipulations that opc_range() does with
**          the structure make the opc_opid field meaningless.
[@history_template@]...
*/
struct _OPC_QCMP
{
	/* opc_lo_const, opc_hi_const, and opc_keyno give the 
	** information about this comparison. 
	** Opc_lo_const and opc_hi_const are pointers to constant info for
	** this key number. Opc_lo_const gives the lo key, and
	** opc_hi_const gives the hi key for this comparison.
	** Opc_keyno gives the key number that this comparison is on.
	*/
    i4		opc_keyno;
    OPC_QCONST	*opc_lo_const;
    OPC_QCONST	*opc_hi_const;
};


/*}
** Name: OPC_QAND - Hold a conjunctive keying clause
**
** Description:
**      This structure is a translated version of an AND PST_QNODE.
[@comment_line@]...
**
** History:
**      1-aug-87 (eric)
**          define
**	2-Jun-2009 (kschendel) b122118
**	    Delete unused struct members.
[@history_template@]...
*/
struct _OPC_QAND
{
	/* Opc_anext and opc_aprev are pointers to the next and previous
	** conjuncts. This is a straight, rather than a curcular, linked
	** list. That is to say, opc_aprev in the first element of the list
	** points to NULL, as does opc_anext in the last element.
	*/
    OPC_QAND	*opc_anext;
    OPC_QAND	*opc_aprev;

	/* Opc_ncomps gives the real number of elements in the opc_comps
	** array. Opc_curcomp is the opc_comps element number that is currently
	** being processed. Opc_curcomp should be >= -1 and < opc_ncomps.
	*/
    i4		opc_ncomps;
    i4		opc_curcomp;

	/* Opc_nnochange holds information that is used
	** to reduce the number of iterations that are required when converting
	** a CNF qual to a DNF qual. Opc_nnochange give the number of
	** comparisons in the current clause that add no usefull information
	** to the DNF clause being built. All but the first comparison that
	** make no change can be skipped over when building the DNF qual.
	** The first comparison that makes no change cannot be skipped over
	** because although that comparison has no effect, there may be 
	** comparisons in other clauses that have some effect. With that
	** done however, the rest of the comparisons in this cause that
	** make no change can be skipped over, since they cannot possibly
	** provide new information to the DNF qual.
	*/
    i4		opc_nnochange;

	/* Opc_klo_old and opc_khi_old are used for removing the current
	** operation from an OPC_KOR struct while calculating the opc_dnf_qual.
	*/
    OPC_QCONST	*opc_klo_old;
    OPC_QCONST	*opc_khi_old;

	/* Opc_conjunct is the qtree conjunct equivilent of opc_comps. 
	** Opc_comps is the form that is used by OPCs keying code (for 
	** performance reasons). Opc_conjunct is what opc_comps is build from.
	*/
    PST_QNODE	*opc_conjunct;

	/* An array of one or more key comparisons that are 
	** logically or'ed together. Opc_ncomps gives the real size of
	** this array.
	*/
    OPC_QCMP	opc_comps[1];
};


/*}
** Name: OPC_KOR - This holds a disjunctive clause.
**
** Description:
**      This structure holds a disjunctive clause and, as such, describes a
**      single range to search over a relation. This structure is a direct
**      equivilent of the QEF_KOR structure. This has been created, rather
**      than using QEF_KOR, since OPC_KOR is more efficient for the job of
**      building keys. The main reason is that this contains an
**      array of OPC_KAND structs rather than the linked list of QEF_ANDs.
[@comment_line@]...
**
** History:
**      1-aug-87 (eric)
**          defined
[@history_template@]...
*/
struct _OPC_KOR
{
	/* Opc_ands is a pointer to an array of OPC_KAND's. There will be one
	** OPC_KAND for each key attribute of the relation referred to by the
	** parent OPC_KEY.opc_rel. This array should be indexed using a key
	** number, not an attribute number.
	*/
    OPC_KAND	*opc_ands;

	/* Opc_ornext and opc_orprev form a doubly linked list of OPC_KOR's */
    OPC_KOR	*opc_ornext;
    OPC_KOR	*opc_orprev;
};


/*}
** Name: OPC_KAND - This holds a range for one key to search a relation.
**
** Description:
**      This structure is the equivilent of the QEF_AND structure. As such
**      it holds a single key attributes range to search an attribute. The
**      array of these that is a part of the OPC_KOR struct defines a range
**      for all of a relation's key attributes.
[@comment_line@]...
**
** History:
**      1-aug-87 (eric)
**          defined
[@history_template@]...
*/
struct _OPC_KAND
{
	/* Opc_khi and opc_klo are pointers to single key values that represent
	** the maximum, or less than or equal to, range and minimum, or
	** greater than or equal to, range of search for this key attribute.
	*/
    OPC_QCONST	*opc_khi;
    OPC_QCONST	*opc_klo;

	/* Opc_kphi and opc_kplo are pointers to bit maps that tell either
	** which repeat query parameters or which DBP local var numbers
	** are used in this range. The opc_kptype field in OPC_QSTATS
	** tells whether these bits refer to repeat query parameter numbers
	** or to DBP local vars. The size of the bit maps in stored in
	** opc_szkparm in OPC_QSTATS.
	*/
    PTR		opc_kphi;
    PTR		opc_kplo;

	/* Opc_krange_type holds the constants ADE_1RANGE_DONTCARE, 
	** ADE_2RANGE_YES, or ADE_3RANGE_NO. It indicates whether
	** this comparison represents a range or an exact match.
	*/
    i4		opc_krange_type;
};
