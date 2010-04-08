/*
**Copyright (c) 2004 Ingres Corporation
*/

/*}
** Name: RDFTREE.H - Structures that RDF needs to unpack different versions
**		     of trees.
**
** Description:
**      This file contains the structure typedefs and definitions of
**	symbolic constants for RDF to use to unpack trees.  This code used
**	to reside in RDFGETINFO.C, but was moved here for understandability.
**
**	CONTAINS THE FOLLOWING STRUCTURES:
**	  unpacking version two trees:
**	    PST_2CNST_NODE - Parameter number for constant
**	    PST_2OP_NODE - Operator Node for version 2 query trees
**	    PST_2RSDM_NODE - RESDOM and BYHEAD nodes
**	  unpacking version three trees:
**	    PST_3OP_NODE - Operator Node for version 3 query trees
**	    PST_3RNGTAB - Range Table for Version 3 and below Query trees.
**	    RDF_V3_QTREE - Version 3 descriptor used to write query trees
**	  Unpacking pre-version 8 trees:
**	    there was a change in the RESDOM/BYHEAD nodes for version 8--
**	    PST_7RSDM_NODE - RESDOM and BYHEAD nodes.
**	  unpacking/packing current version trees
**	    RDF_CHILD - describes number of children at a PST_QNODE
**	    RDF_QTREE - CURRENT VERSION descriptor used to read/write query trees
**	    RDF_TSTATE - keep track of current state of reading tree
**	  unpacking/packing star trees
**	    RDF_TEMPAREA - temporary area to store pointers
**	    PST_5RNGTAB - Range Table for Version 5 (pre-sybil Star) Query trees
**	    PST_5_RNGENTRY - range tables for version 5
**	    PST_7_RNGENTRY - range tables for version 7
**	    PST_7RSDM_NODE - RESDOM and BYHEAD nodes.
**
** History: 
**   21-feb-91 (teresa)
**	Initial creation from rdfgetinfo.c
**   21-feb-91 (teresa)
**	define structure RDF_V2_QTREE
**   26-feb-91 (teresa)
**	Moved in RDF_CHILD and RDF_QTREE here from rdfint.h.
**   27-jan-92 (teresa)
**	created PST_5RNGTAB for SYBIL.
**   23-feb-93 (teresa)
**	added PST_7RSDM_NODE for CONSTRAINTS.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



/*}
** Name: PST_2CNST_NODE - Parameter number for constant
**
** Description:
**	Old constant node format, used to read version 2 query trees.  Two
**      fields were added to this to get to the next version:
**              pst_tparmtype - a i4
**              pst_origtxt   - a ptr to the original text string for the const.
**
** History:
**     11-may-86 (seputis)
**          written
*/
typedef struct _PST_2CNST_NODE
{
    i4              pst_parm_no;        /* param number in saved query plan */
    PST_PMSPEC	    pst_pmspec;		/* pattern-matching specifier */
    DB_LANG	    pst_cqlang;		/* Query language id for this const */
}   PST_2CNST_NODE;

/*}
** Name: PST_2OP_NODE - Operator Node for version 2 query trees
**
** Description:
**	Needed to support version 2 query trees since the escape character
**	fields were added.
**
** History:
**	19-jul-88 (seputis)
**	    initial creation, old version
*/
typedef struct _PST_2OP_NODE
{
    ADI_OP_ID       pst_opno;           /* operator number */
    ADI_FI_ID       pst_opinst;         /* ADF function instance id */
    i4		    pst_distinct;	/* set to TRUE if distinct agg */
    i4		    pst_retnull;	/* set to TRUE if agg should return
					** null value on empty set.
					*/
    ADI_FI_ID       pst_oplcnvrtid;     /* ADF func. inst. id to cnvrt left */
                                        /* not currently used */
    ADI_FI_ID       pst_oprcnvrtid;     /* ADF func. inst. id to cnvrt right */
                                        /* not currently used */
    i4		    pst_opparm;		/* Parameter number for aggregates */
    ADI_FI_DESC     *pst_fdesc;         /* ptr to struct describing operation */
    i4		    pst_opmeta;
} PST_2OP_NODE;

/*}
** Name: PST_2RSDM_NODE - RESDOM and BYHEAD nodes
**
** Description:
**	Old resdom node format, used to read version 2 query trees
**
** History:
**     21-feb-87 (seputis)
**          initial creation
*/
typedef struct _PST_2RSDM_NODE
{
    i4              pst_rsno;           /* result domain number */
    i4		    pst_rsupdt;		/* TRUE iff cursor column for update */
    i4		    pst_print;		/* TRUE iff printing resdom. else
					** user should not see result of
					** resdom.
					*/
    char	    pst_rsname[DB_MAXNAME]; /* name of result column */
} PST_2RSDM_NODE;

/*}
** Name: PST_7RSDM_NODE - RESDOM and BYHEAD nodes
**
** Description:
**	Old resdom node format, used to read version 8 query trees
**
** History:
**     23-feb-93 (teresa)
**          initial creation
*/
#define RD8DB_MAXNAME   32  /* dbmaxname at creation time of this structure */

typedef struct _PST_7RSDM_NODE
{
    /* result domain number, from 1 to the number of resdoms
    */
    i4              pst_rsno;           /* result domain number */
    i4  	    pst_ntargno;
    i4  	    pst_ttargtype;
    i4		    pst_rsupdt;		/* TRUE iff cursor column for update */
    i4		    pst_print;		/* TRUE iff printing resdom. else
					** user should not see result of
					** resdom.
					*/
    char	    pst_rsname[RD8DB_MAXNAME]; /* name of result column */
} PST_7RSDM_NODE;


/*}
** Name: PST_3OP_NODE - Operator Node for version 3 query trees
**
** Description:
**	Needed to support version 3 query trees, which predate the Outter Join
**	(OJ) feature.
**
** History:
**	29-sep-1989 (teg)
**	    initial creation of old version for OJ project.
**	13-Nov-2009 (kiria01) 121883
**	    Distinguish between LHS and RHS card checks for 01 adding
**	    PST_CARD_01L to reflect PST_CARD_01R.
*/
typedef struct _PST_3OP_NODE
{
    ADI_OP_ID       pst_opno;           /* operator number */
    ADI_FI_ID       pst_opinst;         /* ADF function instance id */
    i4		    pst_distinct;	/* set to TRUE if distinct agg */
#define	PST_DISTINCT	1		/* distinct aggregate */
#define PST_NDISTINCT	0		/* regular, non distinct, aggregate */
#define PST_DONTCARE	-1		/* min, max - dups do not affect 
					** semantics
					*/
    i4		    pst_retnull;	/* set to TRUE if agg should return
					** null value on empty set.
					*/
    ADI_FI_ID       pst_oplcnvrtid;     /* ADF func. inst. id to cnvrt left */
                                        /* not currently used */
    ADI_FI_ID       pst_oprcnvrtid;     /* ADF func. inst. id to cnvrt right */
                                        /* not currently used */
    i4		    pst_opparm;		/* Parameter number for aggregates */
    ADI_FI_DESC     *pst_fdesc;         /* ptr to struct describing operation */
	/* Pst_opmeta defines the way an operator to work on the operator
	** pst_opno. Currently this is needed only when the
	** right side of this node is a subselect (subselects can only
	** be on the right side). Instead of adding this field to the
	** op node, some new node type could have been created and
	** placed between this op node and the select. The problem with
	** this is that these modifiers effect both sides of this op node,
	** not just the select.
	*/
    i4		    pst_opmeta;

    /* PST_NOMODIFIER defines no additional semantics. */
#define	    PST_NOMETA		0

    /* PST_CARD_ANY defines that this operator is TRUE for each value
    ** on the left side if the operator defined by pst_opno is TRUE for
    ** any one or more of the values from the subselect on the right side.
    ** Otherwise, this comparison is FALSE or UNKNOWN.
    */
#define	    PST_CARD_ANY	2

    /* PST_CARD_ALL defines that this operator is TRUE for each value
    ** on the left side if the operator defined by pst_opno is TRUE for
    ** all of the values from the subselect on the right side. If the
    ** right side contains no values, then this operator is TRUE for each 
    ** value on the left side.
    */
#define	    PST_CARD_ALL	3

    /* PST_CARD_01R defines that only zero or one value can be returned
    ** from the subselect on the right side. If two or more values are returned
    ** then an error should be returned to the user.
    */
#define	    PST_CARD_01R	1

    /* PST_CARD_01L defines that only zero or one value can be returned
    ** from the subselect on the left side. If two or more values are returned
    ** then an error should be returned to the user.
    */
#define	    PST_CARD_01L	4

	/* Pst_escape is the escape char that the user has specified for the
	** LIKE operator. Pst_escape has valid info in it *only* if
	** pst_isescape == PST_HAS_ESCAPE. If pst_isescape is PST_NO_ESCAPE,
	** then this is a LIKE operator, but without an escape char.  If
	** pst_isescape == PST_DOESNT_APPLY then this is not the LIKE op.
	*/
    char	pst_escape;
    i1		pst_isescape;

#define	    PST_NO_ESCAPE	0	/* LIKE op, no escape char given    */
#define	    PST_HAS_ESCAPE	1	/* valid escape char in pst_escape  */
#define	    PST_DOESNT_APPLY	2       /* not a LIKE operator		    */

    i2		pst_reserve;
} PST_3OP_NODE;

/*}
** Name: PST_3RNGTAB - Range Table for Version 3 and below Query trees.
**
** Description:
**	Needed to support version 3 query trees since the pst_outer_rel and
**	pst_inner_rel fields were added.
**
** History:
**	2-oct-89 (teg)
**	    initial creation, old version
*/

#define		PST_3NUMVARS	    30

typedef struct _PST_3_RNGENTRY
{
	/* Pst_rgtype tells whether this range variable represents a
	** phsyical relation or a relation that must be materialized from a
	** qtree. 
	*/
    i4			pst_rgtype;
#define	    PST_UNUSED	0
#define	    PST_TABLE	1
#define	    PST_RTREE	2

    PST_QNODE		*pst_rgroot;	/* root node of qtree */

	/* Pst_rgparent is a number of the range var that uses this range var
	** and caused it to be added. If this range var has no parent then
	** pst_rgparent will be set to -1. Parent range vars will always have
	** their pst_rgtype set to PST_QTREE.
	*/
    i4			pst_rgparent;
    DB_TAB_ID		pst_rngvar;     /* Rng tab = array of table ids. */
    DB_TAB_TIMESTAMP    pst_timestamp;  /* For checking whether up to date */
} PST_3_RNGENTRY;

typedef PST_3_RNGENTRY PST_3RNGTAB[PST_3NUMVARS];

/*}
** Name: PST_4_RNGENTRY - single element of a range table
**
** Description:
**      Describes a single element of the version 4 PST_RNGTAB structure
**
**	Differences from version 3:
**	    Added pst_outer_rel and pst_inner_rel to keep track of outer and
**	    inner relations of outer joins.
**  History:
**	11-12-91 (teresa)
**	    Initial creation from 6.5 psfparse.h
[@history_template@]...
*/
#define		PST_4NUMVARS	    30
typedef struct _PST_4_RNGENTRY
{
	/* Pst_rgtype tells whether this range variable represents a
	** phsyical relation or a relation that must be materialized from a
	** qtree. 
	*/
    i4			pst_rgtype;
#define	    PST_UNUSED	0
#define	    PST_TABLE	1
#define	    PST_RTREE	2

    PST_QNODE		*pst_rgroot;	/* root node of qtree */

	/* Pst_rgparent is a number of the range var that uses this range var
	** and caused it to be added. If this range var has no parent then
	** pst_rgparent will be set to -1. Parent range vars will always have
	** their pst_rgtype set to PST_QTREE.
	*/
    i4			pst_rgparent;
    DB_TAB_ID		pst_rngvar;     /* Rng tab = array of table ids. */
    DB_TAB_TIMESTAMP    pst_timestamp;  /* For checking whether up to date */
    PST_J_MASK		pst_outer_rel;  /*
					** if n-th bit is ON, this relation is
					** an outer relation in the join with ID
					** equal to n
					*/
    PST_J_MASK		pst_inner_rel;  /*
					** if n-th bit is ON, this relation is
					** an inner relation in the join with ID
					** equal to n
					*/
} PST_4_RNGENTRY;

typedef PST_4_RNGENTRY PST_4RNGTAB[PST_4NUMVARS];

/*}
** Name: PST_5RNGTAB - Range Table for Version 5 Query trees.
**
** Description:
**	Needed to support version 5 query trees for SYBIL.  Version 5 rangetable
**	looks like version 3 range tables except that STAR has 1 additional
**	field: pst_dd_obj_type.
**
** History:
**	27-jan-92 (teresaa)
**	    initial creation, old version
*/

#define		PST_5NUMVARS	    30

typedef struct _PST_5_RNGENTRY
{
	/* Pst_rgtype tells whether this range variable represents a
	** phsyical relation or a relation that must be materialized from a
	** qtree. 
	*/
    i4			pst_rgtype;
#define	    PST_UNUSED	0
#define	    PST_TABLE	1
#define	    PST_RTREE	2

    PST_QNODE		*pst_rgroot;	/* root node of qtree */

	/* Pst_rgparent is a number of the range var that uses this range var
	** and caused it to be added. If this range var has no parent then
	** pst_rgparent will be set to -1. Parent range vars will always have
	** their pst_rgtype set to PST_QTREE.
	*/
    i4			pst_rgparent;
    DB_TAB_ID		pst_rngvar;     /* Rng tab = array of table ids. */
    DB_TAB_TIMESTAMP    pst_timestamp;  /* For checking whether up to date */
    DD_OBJ_TYPE		pst_dd_obj_type;
} PST_5_RNGENTRY;

typedef PST_5_RNGENTRY PST_5RNGTAB[PST_5NUMVARS];

/*}
** Name: PST_6_RNGENTRY - Range table entry
**
** Description:
**      This structure defines the range table entry for version 6 trees.
**
**	Difs from version 3:
**	    - renamed PST_RNGTAB to PST_RNGENTRY (and changed this from an array
**	      of structures to a single structure)
**	    - a new field, pst_corr_name was added to PST_RNGENTRY to improve
**	      readability of QEPs
**
** History:
**	11-12-91 (teresa)
**	    Initial creation for 6.4 FCS psfparse.h
*/
#define		PST_6NUMVARS	    30
typedef struct _PST_6_RNGENTRY
{
	/* Pst_rgtype tells whether this range variable represents a
	** phsyical relation or a relation that must be materialized from a
	** qtree. 
	*/
    i4			pst_rgtype;
#define	    PST_UNUSED	0
#define	    PST_TABLE	1
#define	    PST_RTREE	2

    PST_QNODE		*pst_rgroot;	/* root node of qtree */

	/* Pst_rgparent is a number of the range var that uses this range var
	** and caused it to be added. If this range var has no parent then
	** pst_rgparent will be set to -1. Parent range vars will always have
	** their pst_rgtype set to PST_QTREE.
	*/
    i4			pst_rgparent;
    DB_TAB_ID		pst_rngvar;     /* Rng tab = array of table ids. */
    DB_TAB_TIMESTAMP    pst_timestamp;  /* For checking whether up to date */
					/*
					** will contain correlation name if one
					** was supplied by the user; otherwise
					** will contain table name
					*/
    DB_TAB_NAME		pst_corr_name;	
} PST_6_RNGENTRY;

/*}
** Name: PST_7_RNGENTRY - Range table entry
**
** Description:
**      This structure defines the range table entry for version 7 trees.
**
**	Difs from version 3:
**	    - renamed PST_RNGTAB to PST_RNGENTRY (and changed this from an array
**	      of structures to a single structure)
**	    - a new field, pst_corr_name was added to PST_RNGENTRY to improve
**	      readability of QEPs
**	    - two new fields were added for outer joins: pst_inner_rel and
**	      and pst_outer_rel
**	(Difs from version 6):
**	    - two new fields were added for outer joins: pst_inner_rel and
**	      and pst_outer_rel
**
** History:
**	27-jan-92 (teresa)
**	    Initial creation for SYBIL
*/
#define		PST_7NUMVARS	    30
typedef struct _PST_7_RNGENTRY
{
	/* Pst_rgtype tells whether this range variable represents a
	** phsyical relation or a relation that must be materialized from a
	** qtree. 
	*/
    i4			pst_rgtype;
#define	    PST_UNUSED	0
#define	    PST_TABLE	1
#define	    PST_RTREE	2

    PST_QNODE		*pst_rgroot;	/* root node of qtree */

	/* Pst_rgparent is a number of the range var that uses this range var
	** and caused it to be added. If this range var has no parent then
	** pst_rgparent will be set to -1. Parent range vars will always have
	** their pst_rgtype set to PST_QTREE.
	*/
    i4			pst_rgparent;
    DB_TAB_ID		pst_rngvar;     /* Rng tab = array of table ids. */
    DB_TAB_TIMESTAMP    pst_timestamp;  /* For checking whether up to date */
					/*
					** will contain correlation name if one
					** was supplied by the user; otherwise
					** will contain table name
					*/
    PST_J_MASK		pst_outer_rel;  /*
					** if n-th bit is ON, this relation is
					** an outer relation in the join with ID
					** equal to n
					*/
    PST_J_MASK		pst_inner_rel;  /*
					** if n-th bit is ON, this relation is
					** an inner relation in the join with ID
					** equal to n
					*/
					/*
					** will contain correlation name if one
					** was supplied by the user; otherwise
					** will contain table name
					*/
    DB_TAB_NAME		pst_corr_name;	
} PST_7_RNGENTRY;

/*}
** Name: RDF_V3_QTREE - Version 3 descriptor used to write query trees
**
** Description:
**
**	This is a copy of RDF_QTREE before a bugfix was added to 
**	fix an alignment problem.  This is used only for STAR:
**
**      This descriptor needs to match the beginning of the
**      PST_QTREE structure so that pst_ver is included, this
**      is used to determine the type of processing routine which
**      will be used to read in a query tree.  Query tree headers
**      are over 1K long, so try to compress the amount of data
**      actually written out.
**
** History:
**      21-feb-91 (teresa)
**	    create from RDF_QTREE
[@history_template@]...
*/
typedef struct _RDF_V3_QTREE
{
    i2              mode;               /* match pst_mode type */
    i2              vsn;                /* match pst_version type */
    i2		    mask;               /* mask to be written out */
#define                 RDF_AGINTREE    1
/* TRUE if pst_agintree is TRUE */
#define                 RDF_SUBINTREE   2
/* TRUE if pst_subintree is TRUE */
#define                 RDF_RNGTREE     4
/* TRUE if pst_rngtree is TRUE */
#define                 RDF_WCHK        8
/* TRUE if pst_wchk is TRUE */
    i2              resvno;             /* result range number, used
                                        ** by parser in qrymod,
                                        ** probably not needed but
                                        ** code is too hard to change
                                        */
    i2		    range_size;         /* number of elements in the
                                        ** query tree header range
                                        ** table */
    i4		    pst_mask1;		/* internal used by PSF, just save it */
    char	    expansion[46];      /* expansion in check of
                                        ** PST_QTREE changes */
}   RDF_V3_QTREE;

/*}
** Name: PST_8RT_NODE - Structure for AGHEAD and ROOT nodes
**
** Description:
**	Needed to support version 8 root/aghead nodes because pst_xvrm
**	fields changed from i4 to PST_J_MASK (128 bytes) for range table
**	expansion.
**
** History:
**     6-dec-02 (inkdo01)
**	    Written for range table expansion.
*/
typedef struct _PST_8RT_NODE
{
	/*     For SQL pst_tvrc and pst_tvrm, are derived directly from the
        ** tables defined in the "FROM list", note that a table does not
        ** need to be referenced in a var node, to be in the from list map.
        ** Thus, it is not necessarily true that pst_tvrm== pst_lvrm || pst_rvrm
        ** For QUEL pst_tvrc and pst_tvrm give the count and map of all the 
        ** range variables that are directly used in this tree. By 
        ** 'directly used' I mean that there aren't any root, aghead, 
        ** or subquery nodes between all the var nodes and this node.  
	**     It is important to note that the range table in the
	** query header in NOT equivilent to the 'from' list, since it will
	** hold the range vars for all the subqueries.
	*/
    i4              pst_tvrc;
    i4		    pst_tvrm;	    /* SQL from list */

	/* The # and bitmap of range vars referenced directly in the left
	** tree (target list).
	*/
    i4              pst_lvrc;
    i4		    pst_lvrm;

    i4              pst_mask1;
	/* In order to avoid a new query tree version for corelated subselects
	** in views stored in the catalogs, this flag must be set for the
	** PST_2CORR_VARS_FOUND to be valid */
#define		PST_1CORR_VAR_FLAG_VALID    0x01
	/* This flag indicates that the subselect or aggregate associated with
	** this root node has corelated variables */
#define		PST_2CORR_VARS_FOUND	    0x02
	/*
        ** this flag indicates that this ROOT/SUBSEL node is the nearest
	** such node to AGHEAD node whose left child is a BYHEAD
	*/
#define         PST_3GROUP_BY               0x04
	/* 
	** this flag indicates that this (sub)tree should ignore the duplicate
	** key/row insert error of dmf and just return "0" rows as the result
	** with no error.  This is the QUEL semantic already in placed 
	** "triggered" by pst_qlang.
	*/
#define		PST_4IGNORE_DUPERR	    0x08
	/*
	* this flag indicates a "...having [not] exists..." style query
	*/
#define		PST_5HAVING		    0x10
	/* The bitmap of range vars referenced directly in the right
	** tree (where clause).
	*/
    i4		    pst_rvrm;		/* used in OPF to track range var
					** refs on RHS of tree */

    DB_LANG	    pst_qlang;		/* query language for this subtree. */

    i4              pst_rtuser;         /* TRUE if root of user-generated qry */
    i4		    pst_project;	/* TRUE means project aggregates */
    	/* Pst_dups defines whether or not duplicates should be removed from
	** the results of this (sub)tree, or if it matters.
	*/
    i2		    pst_dups;
#define	PST_ALLDUPS		1
#define	PST_NODUPS		2   /* ie UNIQUE or DISTINCT */
#define	PST_DNTCAREDUPS	3

    PST_UNION	    pst_union;		/* groups union related info */
} PST_8RT_NODE;

/*}
** Name: PST_8_RNGENTRY - single element of a range table
**
** Description:
**      This structure defines the range table entry for version 8 trees.
**
**	Difs from version 9:
**	    - PST_J_MASK (pst_outer/inner_rel) changed from i4 to 
**	    128-byte array (char, I think)
**
[@comment_line@]...
**
** History:
**      6-dec-02 (inkdo01)
**	    Written for range table expansion.
[@history_template@]...
*/
#define		PST_8NUMVARS	    30
typedef struct _PST_8_RNGENTRY
{
	/* Pst_rgtype tells whether this range variable represents a
	** phsyical relation or a relation that must be materialized from a
	** qtree. 
	*/
    i4			pst_rgtype;
#define	    PST_UNUSED	0
#define	    PST_TABLE	1
#define	    PST_RTREE	2
#define	    PST_SETINPUT 3	/* temporary table, used in set-input dbprocs;
				** tab_id will be filled in, but will only be
				** valid when RECREATEing the procedure
				*/

    PST_QNODE		*pst_rgroot;	/* root node of qtree */

	/* Pst_rgparent is a number of the range var that uses this range var
	** and caused it to be added. If this range var has no parent then
	** pst_rgparent will be set to -1. Parent range vars will always have
	** their pst_rgtype set to PST_QTREE.
	*/
    i4			pst_rgparent;
    DB_TAB_ID		pst_rngvar;     /* Rng tab = array of table ids. */
    DB_TAB_TIMESTAMP    pst_timestamp;  /* For checking whether up to date */
    i4			pst_outer_rel;  /*
					** if n-th bit is ON, this relation is
					** an outer relation in the join with ID
					** equal to n
					*/
    i4			pst_inner_rel;  /*
					** if n-th bit is ON, this relation is
					** an inner relation in the join with ID
					** equal to n
					*/
					/*
					** will contain correlation name if one
					** was supplied by the user; otherwise
					** will contain table name
					*/
    DB_TAB_NAME		pst_corr_name;	
    DD_OBJ_TYPE		pst_dd_obj_type;
} PST_8_RNGENTRY;

typedef PST_8_RNGENTRY PST_8RNGTAB[PST_8NUMVARS];

/*}
** Name: RDF_CHILD - describes number of children at a PST_QNODE
**
** Description:
**      Used in tree writing and reading routine to define the number
**      of children in a tree
**
** History:
**      17-feb-88 (seputis)
**          initial creation
[@history_template@]...
*/
typedef i1 RDF_CHILD;
#define                 RDF_0CHILDREN   0
/* PST_QNODE has 0 children */
#define                 RDF_LCHILD      1
/* PST_QNODE has a left child only */
#define                 RDF_RCHILD      2
/* PST_QNODE has a right child only */
#define                 RDF_LRCHILD     3
/* PST_QNODE has a left and right child */

/*}
** Name: RDF_QTREE - CURRENT VERSION descriptor used to read/write query trees
**
** Description:
**      This descriptor needs to match the beginning of the
**      PST_QTREE structure so that pst_ver is included, this
**      is used to determine the type of processing routine which
**      will be used to read in a query tree.  Query tree headers
**      are over 1K long, so try to compress the amount of data
**      actually written out.
**
** History:
**      17-feb-88 (seputis)
**          initial creation
**      24-mar-89 (mings)
**          added pst_mask1 
**	29-sep-90  (teg)
**	    added numjoins for OJ.
**	11-jan-90 (teresa)
**	    fixed alignment bug 35350.
**	30-dec-92 (andre)
**	    defined RDF_AGG_IN_OUTERMOST_SUBSEL and
**	    RDF_GROUP_BY_IN_OUTERMOST_SUBSEL
**	31-dec-92 (andre)
**	    defined RDF_SINGLETON_SUBSEL
**	17-nov-93 (andre)
**	    defined RDF_CORR_AGGR, RDF_SUBSEL_IN_OR_TREE, RDF_ALL_IN_TREE, and
**	    RDF_MULT_CORR_ATTRS
**	17-jul-00 (hayke02)
**	    Added RDF_INNER_OJ.
**	10-dec-2007 (smeke01) b118405
**	    Added RDF_AGHEAD_MULTI, RDF_IFNULL_AGHEAD, RDF_IFNULL_AGHEAD_MULTI.
[@history_template@]...
*/
typedef struct _RDF_QTREE
{
    i2              mode;               /* match pst_mode type */
    i2              vsn;                /* match pst_version type */
    i2		    mask;               /* mask to be written out */
#define                 RDF_AGINTREE    1
/* TRUE if pst_agintree is TRUE */
#define                 RDF_SUBINTREE   2
/* TRUE if pst_subintree is TRUE */
#define                 RDF_RNGTREE     4
/* TRUE if pst_rngtree is TRUE */
#define                 RDF_WCHK        8
/* TRUE if pst_wchk is TRUE */
#define	    RDF_AGG_IN_OUTERMOST_SUBSEL	16
/* set if PST_AGG_IN_OUTERMOST_SUBSEL is set */
#define	    RDF_GROUP_BY_IN_OUTERMOST_SUBSEL	32
/* set if PST_GROUP_BY_IN_OUTERMOST_SUBSEL is set */
#define	    RDF_SINGLETON_SUBSEL	64
/* set if PST_SINGLETON_SUBSEL is set */
#define	    RDF_CORR_AGGR		128
/* set if PST_CORR_AGGR is set */
#define     RDF_SUBSEL_IN_OR_TREE	256
/* set if PST_SUBSEL_IN_OR_TREE is set */
#define     RDF_ALL_IN_TREE		512
/* set if PST_ALL_IN_TREE is set */
#define     RDF_MULT_CORR_ATTRS		1024
/* set if PST_MULT_CORR_ATTRS is set */
#define	    RDF_INNER_OJ		2048
/* set if PST_INNER_OJ is set */
#define	    RDF_AGHEAD_MULTI		4096
/* set if PST_AGHEAD_MULTI is set */
#define	    RDF_IFNULL_AGHEAD		8192
/* set if PST_IFNULL_AGHEAD is set */
#define	    RDF_IFNULL_AGHEAD_MULTI	16384
/* set if PST_IFNULL_AGHEAD_MULTI is set */
    i2              resvno;             /* result range number, used
                                        ** by parser in qrymod,
                                        ** probably not needed but
                                        ** code is too hard to change
                                        */
    i2		    range_size;         /* number of elements in the
                                        ** query tree header range
                                        ** table */
    i2              numjoins;           /* NOT CURRENTLY USED FOR STAR, BUT IN
					** 6.5 corresponds to pst_numjoins.  put
					** here to keep compatible with 6.5 */
    i4		    pst_mask1;		/* internal used by PSF, just save it */
    i4		    pst_1basetab_id;	/* base portion of pst_basetab_id */
    i4		    pst_2basetab_id;	/* index portion of pst_basetab_id */
    char	    expansion[36];      /* expansion in check of
                                        ** PST_QTREE changes */
}   RDF_QTREE;

/*}
** Name: RDF_TSTATE - read query tree state variable
**
** Description:
**      This structure defines the state of RDF while reading tuples to
**      create a query tree 
[@comment_line@]...
**
** History:
**      4-nov-87 (seputis)
**          initial creation
**	01-feb-90 (teg)
**	    add adfcb to hold ptr to adf's control block. The rdr_tree routine
**	    is highly recursive and calls adi_fidesc.  So, we need to get the
**	    address of adfcb before entering the highly recursive routine.  This
**	    is where we give the address of the adfcb to rdr_tree.
**	12-Feb-90 (teg)
**	    removed rdf_compressed flag.  RDF only supports compressed, so it
**	    is not necessary to have this flag any longer.
**      27-feb-91 (teresa)
**	    Merged STAR and local's rdf_state.  This required adding back
**	    rdf_compressed, since star still uses it.  But this field is not
**	    used for local.  Also, field rdf_3op_node added from 6.5 for support
**	    of Outer Joins (not used in STAR or 6.3).
[@history_template@]...
*/
typedef struct _RDF_TSTATE
{
    i4		    rdf_version;        /* version of query tree to be
					** read */
    bool	    rdf_compress;       /* TRUE if query tree is compressed
                                        ** for each variant */
    i4              rdf_tcount;         /* number of tuple in linked list
                                        ** of query tree tuples read from
                                        ** RDF */
    i4              rdf_toffset;        /* byte offset in current tuple
                                        ** being converted */
    i4		    rdf_seqno;          /* sequence number of tuple for
                                        ** validity check */
    bool	    rdf_eof;		/* used to store EOF condition on
					** tuples read from QEF */
    QEF_DATA        *rdf_qdata;         /* PTR to current qef descriptor
                                        ** which contains data being converted
                                        ** - NULL if no data has been read
                                        ** or is available */
    i4		    rdf_treetype;       /* type of tree being read */
    PST_SYMBOL	    rdf_temp;           /* temp node to read in parse tree
                                        ** info, originally this was in the
                                        ** local stack, but this caused
                                        ** stack overflow errors so it is
                                        ** placed in this global structure
                                        ** instead */
    PST_2OP_NODE    rdf_2op_node;	/* temp for building symbol */
    PST_2RSDM_NODE  rdf_2rsdm_node;	/* temp for building symbol */
    PST_7RSDM_NODE  rdf_7rsdm_node;	/* temp for building symbol */
    PST_2CNST_NODE  rdf_2cnst_node;	/* temp for building symbol */
    PST_3OP_NODE    rdf_3op_node;	/* temp for building symbol */
    PST_8RT_NODE    rdf_8rt_node;	/* temp for building symbol */
    ADF_CB	    *adfcb;		/* ptr to adf control block.  This is
					** initialized by rdu_rtree just before
					** calls to highly recursive rdr_tree.*/
} RDF_TSTATE;

/*}
** Name: RDF_TEMPAREA - temporary area to store pointers
**
** Description:
**      This structure is used in tree retrieval process to store PST_QNODE pointers.
**
** History:
**      17-mar-89 (mings)
**          initial creation
[@history_template@]...
*/
typedef struct _RDF_TEMPAREA
{
    struct _RDF_TEMPAREA    *rdf_next_p;        /* point to the next element */
    PST_QNODE		    *rdf_qnode_p;       /* point to PST_QNODE */
    i4		    rdf_pst_type;	/* descendant mask */
#define RDF_0_NONE	0x0000L		
#define RDF_1_RIGHT	0x0001L 
#define RDF_2_LEFT	0x0002L 

} RDF_TEMPAREA;
