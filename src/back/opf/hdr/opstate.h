/*
**Copyright (c) 2004 Ingres Corporation
**
*/

/**
** Name: OPSTATE.H - Describes the global state variable for an optimization
**
** Description:
**      Contains structures visible to all phases of query optimization
**      within the optimizer.
**       
**      Some naming conventions used:
**        Current phases of the optimizer
**          OPA - structures used only in the aggregate processing phase
**          OPJ - structures used in joinop phase 
**          OPN - structures used only in the enumeration phase
**          OPC - structures used only in query compilation
**          OPS - structures used for session level management
**          OPT - structures defining debug/trace information
**          OPX - structures defining exception and error handling
**          OPG - structures defining server level management
**          OPU - utility routines
**          OPM - memory management routines
**
**        Partitioning according to datatypes
**          OPB - structures defining boolean factor information
**          OPV - structures defining range variable information
**          OPZ - structures defining attributes, or function attributes
**          OPO - structures defining cost ordering information
**          OPE - structures defining equivalence class information
**          
** History: 
**      20-feb-86 (seputis)
**          initial creation
**	28-jan-89 (paul)
**	    Added state information required for rules processing.
**      26-nov-90 (stec)
**          Modified OPS_STATE structure.
**      28-dec-90 (stec)
**          Removed modification to OPS_STATE struct.
**      26-oct-92 (jhahn)
**          Added ops_inStatementEndRules to OPS_STATE.
**      01-Jan-00 (sarjo01)
**          Bug 99120: Added new #define "OPS_COLLATEDIFF". 
**	24-jan-01 (hayke02)
**	    Added OPS_SUBSEL_AGHEAD. This change fixes bug 103720.
**	28-nov-02 (inkdo01 for hayke02)
**	    Added OPS_DECCOERCION flag for decimal stupidities.
**	08-Jan-2003 (hanch04)
**	    Back out last change.  Causes E_OP0791_ADE_INSTRGEN errors.
**	16-jan-2003 (somsa01 for hayke02/inkdo01)
**	    Re-added OPS_DECCOERCION flag for decimal stupidities.
**      7-oct-2004 (thaju02)
**          Replace i4 with SIZE_TYPE for memory pool > 2Gig.
**	5-oct-2007 (dougi)
**	    Add fields for documenting memory and CPU consumption.
**/

/*}
** Name: OPS_RQUERY - repeat query descriptor
**
** Description:
**      This structure describes a repeat query parameter.  A linked list
**      of these elements is created so that there will be a centralized
**      location in which repeat query information can be found.
**
** History:
**      30-nov-86 (seputis)
**          initial creation
**      16-oct-92 (ed)
**          - fix bug 40835 - wrong number of duplicates when union views and
**          subselects are used
[@history_template@]...
*/
typedef struct _OPS_RQUERY
{
    struct _OPS_RQUERY    *ops_rqnext;  /* next element in list */
    PST_QNODE	     *ops_rqnode;	/* ptr to a constant node
                                        ** which represents the repeat
                                        ** query parameter */
    i4			ops_rqmask;	/* mask describing parameter */
#define                 OPS_RQSPECIAL   1
/* TRUE - if this repeat query parameter was added due to the
** counting tid ii_row_count expression */
}   OPS_RQUERY;

/*}
** Name: OPS_MEMORY - OPF memory management state variable
**
** Description:
**      The structure will describe the state of the memory management
**      streams within OPF.  All variables which deal with memory management
**      are place in one structure.
**
** History:
**      7-dec-87 (seputis)
[@history_template@]...
*/
typedef struct _OPS_MEMORY
{
    ULM_RCB         ops_ulmrcb;         /* the ULM_RCB control block used for
                                        ** all calls to the memory manager
                                        */
    PTR             ops_streamid;       /* - ULM stream id for memory used by 
                                        ** the optimizer during
                                        ** this optimization.  This memory is
                                        ** obtained from the opg_memory pool in
                                        ** the server control block.
                                        ** - this is "long term" memory in that
                                        ** it must exist until the end of the
                                        ** optimization
                                        ** - as opposed to "short term" memory
                                        ** such as CO lists which can be reused
                                        ** after a subquery has been optimized
                                        */
    bool            ops_usemain;        /* TRUE - if ops_streamid should be
                                        ** used instead of enumeration
                                        ** memory */
    PTR             ops_procmem;        /* memory stream used if a DB 
                                        ** procedure is defined, to contain
                                        ** information which is required
                                        ** between query optimizations */
    PTR             ops_tstreamid;      /* stream id associated with a temporary
                                        ** buffer 
                                        ** - this stream will be destroyed
                                        ** and recreated each time a larger temp
                                        ** buffer is needed
                                        */
    PTR             ops_sstreamid;      /* stream id associated with stack style
					** style usage. ULMs mark and reclaim
					** can only be used on this stream.
                                        */
    PTR             ops_tptr;           /* ptr to the temp buffer associated
                                        ** with the ops_tstreamid
                                        */
    i4              ops_tsize;          /* size of the temp buffer ops_tptr */
    SIZE_TYPE       ops_memleft;        /* amount of memory that this session
                                        ** still has available from the OPF
                                        ** memory pool
                                        */
    OPV_RT          *ops_trt;           /* save ptr to full size OPV_RT here
                                        ** this array of ptr will be reused
                                        ** for each subquery, and copied to
                                        ** an exact size structure after
                                        ** the enumeration has completed
                                        */
    OPZ_FT          *ops_tft;           /* ptr to full size OPZ_FT */
    OPZ_AT          *ops_tat;           /* ptr to full size OPZ_AT */
    OPE_ET          *ops_tet;           /* ptr to full size OPZ_ET */
    OPB_BFT         *ops_tbft;          /* ptr to full size OPB_BFT */
    i4         ops_mlimit;         /* this is a memory limit at which
                                        ** some garbage collection routines
                                        ** will be triggered... it will be
                                        ** some fraction of the memory
                                        ** initially allocated to this session
                                        ** for optimization */
    SIZE_TYPE	    ops_totalloc;	/* Total memory allocated in this
					** compile. */
    SIZE_TYPE	    ops_countalloc;	/* count of opu_memory() calls */
    SIZE_TYPE	    ops_count2kalloc;	/* count of 2K+ allocations */
    SIZE_TYPE	    ops_recover;	/* count of opn_recover() calls */
}   OPS_MEMORY;
/*}
** Name: OPS_STATE  - Information that persists for duration of one optimization
**
** Description:
**      This structure points to all state information regarding the
**      optimization of one particular query i.e. once the query optimization
**      is completed all information contained in this structure is discarded.
**      This structure is allocated on the stack upon a request for a
**      query optimization.  
**       
**      This structure contains a pointer to the session control block.  Note,
**      that the session control block also contains a pointer to this 
**      structure.  This redundancy occurs for the following reason.
**      If an interrupt occurs then the session control block is the
**      handle used to access any state information.  However, during
**      normal processing the OPS_STATE structure is passed to each phase
**      of optimization.
**
**      Within each phase of optimization, there is
**      a local state variable created for that phase.  That state variable
**      will contain a pointer to this global state variable.  The local state
**      variable will be passed around to the individual routines of the 
**      respective phase.  This approach will reduce the visibility of any 
**      variables not required by other phases.
**
** History:
**     24-feb-86 (seputis)
**          initial creation
**      19-july-87 (eric)
**          Added ops_sstream_id for stack style memory allocation of ULM
**          memory.
**	28-jan-89 (paul)
**	    Added state information required for processing rule action lists.
**	20-sep-89 (seputis)
**	    Added outer join structure ops_goj
**	16-may-90 (seputis)
**	    - b21536, move variables which apply to enumeration only
**      26-nov-90 (stec)
**          Added a ptr to print buffer for routines dealing with debug
**          output for trace points like op150.
**      26-dec-90 (seputis)
**          - add support for OPC so that the update tid can be found without
**            a local variable defined
**          - added variables to calculate keyed lookup costs
**      28-dec-90 (stec)
**          Removed previous change; ptr will be placed in OPC_STATE struct.
**      18-apr-91 (seputis)
**          - fix for b36920 - added new defines
**      10-jun-91 (seputis)
**          - fix for b37233
**      27-mar-92 (seputis)
**          - fix for b43339
**      26-oct-92 (jhahn)
**          Added ops_inStatementEndRules.
**	25-mar-94 (ed)
**	    bug 59355 - an outer join can change the result of a resdom
**	    expression from non-null to null, this change needs to be propagated
**	    for union view target lists to all parent query references in var nodes
**	    or else OPC/QEF will not allocate the null byte and access violations
**	    may occur
**	24-may-02 (inkdo01)
**	    Added OPS_MEMTAB_XFORM flag to control MS Access where clause transform.
**	28-nov-02 (inkdo01 for hayke02)
**	    Added OPS_DECCOERCION flag for decimal stupidities.
**	15-june-06 (dougi)
**	    Added ops_inBeforeRule & ops_firstBeforeRule for "before" triggers.
**	14-Mar-2007 (kschendel) SIR 122513
**	    Rename pcjoin flag to just indicate partitions are around.
**	17-april-2008 (dougi)
**	    Added OPS_TPROCS flag for table procedure support.
*/
typedef struct _OPS_STATE
{
    OPS_CB          *ops_cb;            /* ptr to the session control block   
                                        */
    OPF_CB          *ops_caller_cb;     /* ptr to the OPF caller's control block
                                        */
    ADF_CB          *ops_adfcb;         /* ptr to ADF session control block
                                        ** obtained from scf */
    PST_QTREE       *ops_qheader;       /* header of main query tree, obtained
                                        ** from QSF using object id found
                                        ** in caller's control block
                                        */
    i4              ops_lk_id;          /* QSF lock id associated with the
                                        ** query tree, and the QSF object id
                                        ** in ops_caller_cb->opf_query_tree
                                        */
    QSF_RCB         ops_qsfcb;          /* QSF control block used to fetch
                                        ** query tree, and also to
                                        ** build QEP object */
    i4              ops_parmtotal;      /* total number of "query parameters"
                                        ** - initially set to 
                                        ** ops_qheader->pst_numparm but may
                                        ** be incremented as simple aggregate
                                        ** substitutions are made.
                                        */
    OPS_RQUERY      *ops_rqlist;        /* beginning of a list of repeat
                                        ** query parameter descriptors
                                        ** - this list will be sorted in
                                        ** descending order of repeat
                                        ** query number
                                        */
    bool            ops_qpinit;         /* TRUE - if OPC has allocated a
                                        ** query plan */
    i4              ops_qplk_id;        /* QSF lock ID for query plan generated
                                        ** by OPF
                                        */
    OPO_CO          *ops_copiedco;	/* This is set to the base of a CO tree which
                                        ** has been copied out of enumeration memory
                                        ** - it will be useful for garbage collection
                                        ** of CO nodes, if not NULL */
    OPV_GLOBAL_RANGE ops_rangetab;      /* the global range table - one element
                                        ** for every table, index, or temp table
                                        ** referenced in the query 
                                        */
    PST_STATEMENT   *ops_statement;	/* ptr to current statement being analyzed
                                        ** - each query is optimized totally and a
                                        ** QP is produced, then memory is reclaimed
                                        ** and used by the next optimization, this
                                        ** points to the current statement containing
                                        ** the current query tree to be optimized
					*/
    PST_STATEMENT   *ops_trigger;	/* ptr to the statement that	    */
					/* triggered rule compilation.	    */
					/* Valid only when compiling a rule */
					/* list (ops_inrulels == TRUE) */
    PST_STATEMENT   *ops_firstAfterRule; /* ptr to first statement of the   */
					/* after list when compiling a rule */
					/* list. Used to link the rule	    */
					/* actions back together after the  */
					/* last statement has been	    */
					/* compiled. Valid only when	    */
					/* compiling a rule list. */
    PST_STATEMENT   *ops_firstBeforeRule; /* ptr to first statement of the  */
					/* before list of triggered rules   */
    PST_PROCEDURE   *ops_procedure;     /* ptr to procedure definition for this
					** optimization */
    OPS_SUBQUERY    *ops_subquery;      /* -the aggregate analysis phase will
                                        ** break the main query tree into a list
                                        ** of aggregate-free subqueries 
                                        ** rooted in this node
                                        ** -each subquery in this list will 
                                        ** then be analyzed by joinop and
                                        ** a best cost tree placed in the
                                        ** respective subquery node
                                        ** -the query compilation phase will
                                        ** use this subquery list to produce
                                        ** a QEP.
                                        */
    bool	    ops_inAfterRules;	/* FALSE compiling regular	    */
					/* statement.			    */
					/* TRUE compiling after rule        */
					/* statement list. */
    bool	    ops_inBeforeRules;	/* FALSE compiling regular	    */
					/* statement.			    */
					/* TRUE compiling before rule       */
					/* statement list. */
    bool	    ops_inAfterStatementEndRules; /* TRUE iff compiling */
					/* after statement end rule list */
    bool	    ops_inBeforeStatementEndRules; /* TRUE iff compiling */
					/* before statement end rule list */
    bool	    ops_union;          /* TRUE if any union views need to be
                                        ** processed */
    bool            ops_terror;         /* set TRUE if at least one query
                                        ** plan was skipped because of a tuple
                                        ** being too wide, used for error
                                        ** reporting if no query plan can be
                                        ** found */
    OPA_AGGSTATE    ops_astate;         /* aggregate processing phase global
                                        ** state variable... the elements
                                        ** in this structure are only visible
                                        ** to the aggregate processing phase
                                        */
    OPN_ESTATE	    ops_estate;         /* enumeration phase global state var
                                        ** - element in this structure is
                                        ** only visible to the joinop
                                        ** processing phase
                                        */
    OPC_STATE	    ops_cstate;		/* Compilation phase global state var
					** - elements in the structure are only
					** visable to OPC, the compilation
					** part of OPF.
					*/
    OPS_MEMORY      ops_mstate;         /* memory management state variable */
    OPT_TRACE       ops_trace;          /* structure contains elements used
                                        ** only during tracing
                                        */
    OPD_DOPT	    ops_gdist;          /* global distributed optimizer state , all sites will
                                        ** be considered in all subqueries, even though
                                        ** it may not be referenced in the subquery, but
                                        ** the parent subquery may want the result there 
                                        */
    PTR		    *opn_expand;	/* for expansion */
    OPL_GJOIN	    ops_goj;		/* ptr to a global outer join descriptor used for
					** translation of PSF outer join IDs to internal
					** OPF outer join IDs */
    i4	    ops_gmask;		/* mask of booleans which describe the query */
#define                 OPS_QUERYFLATTEN    0x00000001L
/* set TRUE if query flattening should be suppressed */
#define                 OPS_BALANCED	    0x00000002L
/* set TRUE if balanced trees are being analyzed */
#define                 OPS_SKBALANCED	    0x00000004L
/* set TRUE if first structurally unique tree should be skipped */
#define                 OPS_GUESS	    0x00000008L
/* set TRUE if special enumeration code to look at first guess of best
** set of relations is completed */
#define                 OPS_PLANFOUND	    0x00000010L
/* set TRUE if a plan was found which pasted the hueristic checks and
** now needs to be analyzed, used to make sure a valid tree orientation is
** found before going onto the next tree with higher cardinality */
#define                 OPS_UV		    0x00000020L
/* set TRUE is this was a union query which was converted to a union view */
#define                 OPS_IDSAME	    0x00000040L
/* set TRUE if at least 2 tables in the query have the same table ID */
#define			OPS_IDCOMPARE	    0x00000080L
/* set TRUE implies that the tree compare routine can use the mapping
** rules to check if 2 tables with the same table IDs are used in a
** compatible way */
#define                 OPS_FLINT	    0x00000100L
/* TRUE if a float or integer exception has occurred in the optimizer */
#define                 OPS_FLSPACE	    0x00000200L
/* TRUE if part of the search space has been skipped due to a float or
** integer exception in the optimizer */
#define			OPS_TCHECK	    0x00000400L
/* used in aggregate substitution to indicate that the main query is being
** processed */
#define			OPS_FPEXCEPTION	    0x00000800L
/* TRUE - if exception has occurred during analysis of this plan
*/
#define			OPS_BFPEXCEPTION    0x00001000L
/* TRUE - current best plan was found with floating point exceptions
*/
#define                 OPS_JSKIPTREE	    0x00002000L
/* TRUE - if current shape of the tree is inappropriate for this set of relations */
#define                 OPS_OPCEXCEPTION    0x00004000L
/* TRUE - if optimizer is currently in OPC - used for error reporting */
#define                 OPS_CIDONLY	    0x00008000L
/* set TRUE implies "compare ID only", it also imples that OPS_IDCOMPARE is set,
** b43339 - it indicates that translation
** of common ID's should occur but no new variable maps should be set */
#define                 OPS_NULLCHANGE      0x00010000L
/* TRUE if outer joins are used, and a view resdom list has changed so that
** a non-nullable type becomes nullable */
#define                 OPS_COLLATEDIFF     0x00020000L
/* TRUE if collation nstate == 0, can use with opn_diff */
#define                 OPS_SUBSEL_AGHEAD   0x00040000L
/* TRUE if PST_AGHEAD node under subselect */
#define			OPS_MEMTAB_XFORM    0x00080000L
/* True if at least one of the MS Access memory contant table transforms has
** been done. We can only cope with one per query. */
#define			OPS_DECCOERCION	    0x00100000L
/* TRUE if we're compiling a binary operation in which exactly one operand
** is DECIMAL. A coercion will then be needed and this flag is a massive 
** kludge to get opcadf.c to coerce to a more meaningful scale than 15
** (which is the stupid default). */
#define			OPS_PARTITION	    0x00200000L
/* TRUE if query has at least one partitioned table */
#define			OPS_TPROCS	    0x00400000L
/* TRUE if query has at least one table procedure. */
    OPS_SUBQUERY    *ops_osubquery;     /* this is the original list of subqueries, as
                                        ** opposed to the list which is given to OPC */
    i4		    ops_expan;		/* for expansion */
    DB_ATT_ID	    ops_tattr;		/* attribute of temporary which is the tid
					** to be used for updating */
    OPV_IGVARS	    ops_tvar;		/* range table number of temporary which contains tid
					** to be used for updating */
    i4		    ops_startcpu;	/* process CPU (in msec) at start of
					** compile */

}   OPS_STATE;
