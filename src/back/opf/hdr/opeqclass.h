/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: OPEQCLASS.H - structures defining joinop equivalence classes
**
** Description:
**      These structures define joinop equivalence classes.  See Bob
**      Kooi's dissertation for an explanation of an equivalence class
**
** History: 
**      10-mar-86 (seputis)
**          initial creation
[@history_line@]...
**/

/*}
** Name: OPE_EQCLIST - equivalence class element
**
** Description:
**      This element represents an equivalence class; i.e. a partition
**      of joinop attributes referenced directly and indirectly in the query.
**      Two attributes are in the same equivalence class if there is
**      an equality between them.  This information is used to determine
**      the validity of a join during enumeration.  The optimizer
**      will rip apart the query tree, and place the semantic information about
**      equi-joins into the concept of an equivalence class.  In fact, the
**      binary operator "=" is "thrown away" in this case.  The model of
**      equivalence classes is much more convenient for join optimizations.
**      Note that the same attribute cannot belong to two equivalence classes
**      since if this were true, the equivalence classes would be merged into
**      one.
**
** History:
**     15-mar-86 (seputis)
**          initial creation
**     24-oct-88 (seputis)
**          added ope_nulljoin field
**     10-jun-91 (seputis)
**	    fix for b37172 - make histogram estimates more predictable
**	12-aug-91 (seputis)
**	    -use tag field for OPZ_ATTS to attempt to remove lint warnings
**	7-dec-93 (ed)
**	    b56139 - add OPE_HIST_NEEDED to mark attributes which
**	    need histograms,... needed since target list is traversed
**	    earlier than before
**	31-jan-94 (rganski)
**	    Removed ope_sargp, due to change to oph_sarglist().
**	28-may-96 (inkdo01)
**	    Add flag to identify spatial join eqclasses
**	8-aug-05 (inkdo01)
**	    Add ope_ojs to aid the placement of join predicates from
**	    ON clauses, as well as OPE_NOTOUTER (for same purpose).
**	24-aug-05 (inkdo01)
**	    Remove above change - problem was fixed differently.
**	21-jul-06 (hayke02)
**	    Re-introduce ope_ojs to keep track of which outer joins created
**	    which eq classes. This change fixes bug 116406.
[@history_line@]...
*/
typedef struct _OPE_EQCLIST
{
    i4              ope_eqctype;        /* NONTID or TID indicator */
#define                 OPE_NONTID      0
/* an equivalence class which is not on TID's */

#define                 OPE_TID         1
/* an equivalence class which is on TID's */

    OPB_IBF         ope_bfindex;        /* index into the opb_boolfact array
                                        ** - referencing  the boolean factor
                                        ** which supplies constant value for 
                                        ** this equivalence class
                                        ** - used in query compilation for
                                        ** building keys
					** FIXME - field is not needed since
                                        ** OPB_GLOBAL contains this info and
                                        ** ope_nbf can be used to reference this
                                        */
    OPZ_BMATTS      ope_attrmap;	/* bit map of attributes which
                                        ** belong to this equivalence class
                                        */
    OPB_IBF         ope_nbf;            /* boolean factor element
                                        ** which can be used for keyed
                                        ** lookup of the attribute
                                        ** - if equivalence class 
                                        ** has any sargable
                                        ** predicates then this boolean factor
                                        ** is one of them, OPB_GLOBAL can then
                                        ** be used to obtain info on the others
                                        ** - OPB_NOBF - implies that no limiting
                                        ** predicates which can be used as
                                        ** keys exist for this equivalence
                                        ** class
                                        */
    OPL_BMOJ	    ope_ojs;		/* bit map of outer joins
					** whose ON clauses contain equijoin
					** preds defining this EQC (used for
					** placement of preds)
					*/
    bool	    ope_nulljoin;	/* Determines the type of join to use
					** for this equivalence class
                                        ** TRUE- indicates NULLs should be
					** kept for any join since all members
                                        ** were added "implicitly", since it is
					** a join between base table and its'
                                        ** implicitly referenced secondary 
                                        ** indexes; explicitly referenced
					** secondary indexes will remove NULLs
                                        ** for now FIXME,... or a joinback
                                        ** for quel function aggregates
                                        ** FALSE- indicates NULLs should be
					** removed since an explicit join
					** was added to this equivalence class
                                        */
    i4		    ope_mask;		/* mask of booleans to describe an
					** equivalence class */
#define                 OPE_CTID        1
/* TRUE if this equivalence class is a TID equivalence class for a variable
** which has secondaries with attributes in common */
#define			OPE_HIST_NEEDED 2
/* TRUE if this equivalence class was involved in implicit index creation
** and requires histograms */
#define			OPE_SPATJ	0x00000004
/* TRUE if this eqclass was created to describe a spatial join (a conjunctive
** factor containing a spatial function on 2 cols from diff tables, at least 
** one of which has a Rtree index defined on it) */
#define			OPE_MIXEDCOLL	8
/* TRUE if this eqclass contains string exprs with more than one collation */
}   OPE_EQCLIST;

/*}
** Name: OPE_ET - equivalence class table
**
** Description:
**      This is an array of pointers to equivalence class elements.  The
**      array of pointers is initialized to NULL and as new equivalence classes
**      are assigned the first NULL entry is used to define element.  If two
**      equivalence classes are merged then one of the entries is set to NULL
**      so it can be reused.  Note the the total number of equivalence classes
**      must be less than or equal to the total number of joinop attributes
**      defined.
**
** History:
**     15-mar-86 (seputis)
**          initial creation
[@history_line@]...
*/
typedef struct _OPE_ET
{
    OPE_EQCLIST     *ope_eqclist[OPE_MAXEQCLS];/* array of pointers to
                                        ** equivalence class elements
                                        ** NULL - means element is not
                                        ** defined.
                                        */
}   OPE_ET;

/*}
** Name: OPE_EQUIVALENCE - data structures defining equivalence classes
**
** Description:
**      This structure contains information pertaining to the concept
**      of equivalence classes.  See Bob Kooi's dissertation for more
**      detailed information.  Basically, an equivalence class is a
**      partitioning of joinop attributes.  Two attributes are in the same
**      equivalence class if and only if there is a equi-joining
**      clause between them.  This equi-joining clause may be explicit
**      or implicit; i.e. due to the way indexes are modelled, the optimizer
**      may generate implicit join clauses.  
**
** History:
**     15-mar-86 (seputis)
**          initial creation
**	7-may-91 (seputis)
**	    fix b37233 - add ope_maxhist, so that the call to oph_histos
**	    can be delayed until it can be determined it is really needed
[@history_line@]...
*/
typedef struct _OPE_EQUIVALENCE
{
    OPE_IEQCLS	    ope_ev;             /* number of elements in the array
                                        ** of ptrs ope_base->ope_eqclist
                                        */
    OPE_ET          *ope_base;          /* base of array of pointers to
                                        ** equivalence class elements
                                        */
    OPO_COMAPS	    ope_maps;		/* Bit map of equivalence classes
                                        ** in the target list, plus aggregate
					** map for completeness
                                        */
    OPE_BMEQCLS     (*ope_subselect);   /* ptr to bitmap of equivalence classes
                                        ** representing subselects */
    OPE_IEQCLS	    ope_maxhist;	/* number of equilvalence classes
					** defined which require histograms */
}   OPE_EQUIVALENCE;
