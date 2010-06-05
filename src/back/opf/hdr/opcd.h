/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: OPCD.H - Distributed optimizer compiler header
**
** Description:
**      Contains data structures used only by distributed OPC.
**
** History: $Log-for RCS$
**      18-oct-88 (robin)
**          Created
**      02-may-89 (robin)
**	    Added opq_q14_need_close flag to signal closing
**	    parenthesis required when opcu_tree_to_text
**	    is finishing a flattened subquery.
**	06-jun-89 (robin)
**	    Lengthened opcudname to allow for possible quotes
**	    and terminating null.  Partial fix for bug 5485.
**	12-jul-91 (seputis)
**	    Added opq_q16_expr_like flag to handle the special
**	    case where instead of a column name, the object of
**	    a LIKE qualification is an expression.  This can
**	    only happen through view substitution.
**	14-jul-92 (fpang)
**	    Fixed compiler warnings, OPCUDNAME, OPCQHD, 
**	    OPCCOLHD, OPCTABHD.
**	01-mar-93 (barbara)
**	    Lengthened opcudname to allow both owner and table names
**	    to be unnormalized and delimited.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
[@history_line@]...
[@history_template@]...
**/

/*  Forward typedef/struct references */

typedef struct	_opcqhd	    OPCQHD;
typedef	struct	_opctabhd   OPCTABHD;
typedef	struct	_opccolhd   OPCCOLHD;
typedef struct  _opcudname  OPCUDNAME;

/*
**  Defines of other constants
**
**  Defines for opq_q2_type:
*/
#define	OPQ_NOTYPE  0		    /* default type */
#define	OPQ_XFER    1		    /* data transfer */

/*
**  Defines for the conjunct type flag parameter to
**  opcu_tree_to_text.  This flag is used to determine
**  whether any conjuncts should be added to the
**  query text being built, and, if so, what type
**  of conjunct to look for.
*/

#define OPCNOCONJ   0		    /* no conjuncts */
#define OPCJOINCONJ 1		    /* join conjuncts */
#define OPCCONJ	    2		    /* other conjuncts, including
				    ** boolean factors */

/*
**  Defines for the opq_q11_duplicate flag that indicates that a temp 
**  is being/has been created to handle duplicate semantics correctly.
**
**  If opq_q11_duplicate = OPQ_1DUP, then a temp is being created
**  that will include all printing AND non-printing resdoms in the
**  target list, as well as any by-vars.  If there are aggregates in
**  the resdom list, they will NOT be evaluated, but instead any
**  expressions below them will be returned as that resdom.
**
**  if opq_q11_duplicate = OPQ_2DUP, this is the second stage of
**  duplicate processing.  The result (either temp table or query)
**  will be created as a retrieval from the temp previously created
**  at the OPQ_1DUP stage.  Only printing resdoms will be returned,
**  and no WHERE clause will be generated (since it was applied when
**  the temp was created.  Aggregates in the resdom list will be
**  evaluated on the expression results in the temp.
*/
#define OPQ_NODUP   0
#define OPQ_1DUP    1		    /* build a 'create as select'
				    ** temp to preserve duplicates */
#define OPQ_2DUP    2		    /* use a previously created temp
				    ** to preserve correct dup sem */
#define OPQ_NORES   -1		    /* no current resdom number is
				    ** defined */

/*
**  Defines for the opq_q16_expr_like flag.
*/

#define OPQ_NOELIKE 0	    
#define OPQ_1ELIKE  1
#define	OPQ_2ELIKE  2

/*
**  Defines for the trace routines that dump the query text for
**  each CO tree node.
*/

#define OPQT_NONE   0		    /* no text generated */
#define	OPQT_TEXT   1		    /* complete qry generated */
#define OPQT_FRGMT  2		    /* where clause fragment only */


struct _opcudname
{
    char    buf[(DB_MAXNAME *4) +6];  /* buffer large enough to hold
				    ** OWNER.TABLENAME, lengthened to
				    ** allow quotes and terminating
				    ** null (partial fix for bug 5485)
				    ** ....Lengthened again to hold
				    ** unnormalized, delimited owner and
				    ** tablename.
				    */
};


/*}
** Name: opcqhd - query head
**
** Description:
[@comment_line@]...
**
** History:
**      26-apr-93 (ed)
**          remove obsolete code after fix to bug 49609
[@history_line@]...
[@history_template@]...
*/
struct _opcqhd
{
    i4		opq_q1_ntemps;		/* no. of temp tables referenced */
    i4		opq_q2_type;		/* type of query */
    DB_LANG	opq_q3_qlang;		/* query language */
    bool	opq_q4_retrow_flg;	/* TRUE if query returns rows */
    DD_LDB_DESC	*opq_q5_src_ldb;	/* LDB where query will run */
    DD_LDB_DESC	*opq_q6_dest_ldb;	/* LDB where result will go */
    OPCTABHD	*opq_q7_tablist;	/* ptr to table list */
    OPCTABHD	*opq_q8_curtable;	/* current table in the list */
    bool	opq_q9_flattened;	/* query contains flatted subselect */
    OPO_CO	*opq_q10_sort_node;	/* sort node for this query or the
					** current subselect, if any */
    i4		opq_q11_duplicate;	/* duplicate handling flag */
    bool	opq_q13_projfagg;	/* func. agg follows a projection */
    bool	opq_q14_need_close;	/* add closing paren to finish nested
					** subselect */
    bool	opq_q15_union;		/* this is a UNION query */
    i4		opq_q16_expr_like;	/* expression-LIKE state flag */
};


/*}
** Name: opccolhd - column head
**
** Description:
**
** History:
**      18-oct-88 (robin)
**          Created
*/
struct _opccolhd
{
    char	    *opq_c1_col_name;
    i4		    opq_c2_col_atnum;	/* attribute no. of the column
					** in the subquery attribute
					** array, ops_attrs */
};



/*}
** Name: opctabhd - table head
**
** Description:
**
** History:
**      18-oct-88 (robin)
**          Created
[@history_template@]...
*/
struct _opctabhd
{
    DD_TAB_NAME opq_t1_rname;		    /* temp table range name */
    i4		opq_t2_tnum;		    /* temp table number */
    DD_LDB_DESC	*opq_t3_ldb;		    /* ldb where table resides */
    OPCTABHD	*opq_t4_tnext;		    /* next table in query */
    i4		opq_t5_ncols;		    /* no. of cols in table */
    OPCCOLHD	opq_t6_tcols[ OPZ_MAXATT];  /* ptr to the col name array */
};

/*}
** Name: OPC_TTCB - query tree to text conversion state
**
** Description:
**      This control block contains the state of the query tree
**      to text conversion.
**
** History:
**      4-apr-88 (seputis)
**          initial creation
[@history_line@]...
**      20-sep-88 (robin)
**	    added fields for distributed support - opt_aname, opt_adesc,
**	    makeresnm.
[@history_line@]...
**      04-oct-88 (robin)
**          Added more distributed fields - state, seglist
**	31-aug-92 (rickh)
**	    The comment following opt_adesc wasn't correctly concluded.
**	    The effect of this was to comment out the following field.
[@history_line@]...
[@history_template@]...
*/
typedef struct _OPC_TTCB
{
    OPS_SUBQUERY    *subquery;		/* ptr to subquery state variable */
    OPS_STATE	    *state;		/* ptr to optimizer state variable */
    QEQ_TXT_SEG	    *seglist;		/* currently active text segment list,
					** normally either query_stmt or 
					** where_cl */
    QEQ_TXT_SEG	    *query_stmt;	/* complete SQL or QUEL query stmt.
					** NULL if only the where clause is
					** being built.  Will include the
					** where clause from where_cl */
    QEQ_TXT_SEG	    *where_cl;		/* where clause - will be included
					** in final query in query_stmt */
    OPCQHD	    *qryhdr;		/* query table use information */
    ULM_RCB	    *opt_ulmrcb;
    OPO_CO	    *cop;		/* current COP node being analyzed */
    OPB_IBF	    bfno;		/* current boolean factor number
					** being evaluated */
    i4		    joinstate;		/* used when processing joining
					** conjunct to determine which attribute
                                        ** in the equivalence class to use */
#define             OPT_JOUTER          3
/* assume attribute comes from outer so use outer variable bitmap */
#define             OPT_JINNER          2
/* assume attribute comes from inner so use inner variable bitmap */
#define             OPT_JNOTUSED	1
/* attribute used in qualification so use current bitmap */
#define             OPT_JBAD		0
/* this state should not be reached */
    OPE_IEQCLS	    jeqcls;		/* equivalence class of last joining
                                        ** attribute processed or OPE_NOEQCLS
                                        ** if all attributes are processed */
    OPE_BMEQCLS     bmjoin;             /* bitmap of equivalence classes
                                        ** which are joined but not part of
                                        ** the sorted inner list; i.e. they
                                        ** will act like normal conjuncts */
    OPE_IEQCLS	    *jeqclsp;           /* pointer to a list of attributes
					** which will be involved in the join
					*/
    OPE_IEQCLS	    reqcls;             /* next resdom equilvalence class
                                        ** to be returned */
    OPV_IVARS	    opt_vno;            /* last range variable to be considered
                                        ** for from list or range table */
    ULD_TSTRING	    opt_tdesc;          /* temporary descriptor for the table
                                        ** name to be supplied to the tree to
                                        ** text routine */
    OPCUDNAME	    opt_tname;          /* temporary location for table name
                                        ** associated with opt_tdesc */

    ULD_TSTRING	    opt_adesc;		/* temporary descriptor for the alias
					** name to be supplied to the tree to
					** text routine */
    bool	    nobraces;		/* don't put braces around target
					** list equivalence class */
    bool	    noalias;		/* don't append alias to variable
					** names */
    bool	    remdup;		/* remove duplicates */
    bool	    anywhere;		/* there is a WHERE clause */
    bool	    nulljoin;		/* a null join is required */
    QEQ_TXT_SEG	    *hold_seg;
    QEQ_TXT_SEG	    *hold_where;
    QEQ_TXT_SEG	    *hold_targ;
    struct
    {
	PST_QNODE	    eqnode;	/* used as temp equals node for joins */
	PST_QNODE	    leftvar;	/* used as temp left node for joins */
	PST_QNODE	    rightvar;	/* used as temp right node for joins */

	PST_QNODE	    resnode;	/*node used to create temporary resdom*/
	PST_QNODE           resexpr;    /* var node used for expression */
    }   qnodes;
} OPC_TTCB;
