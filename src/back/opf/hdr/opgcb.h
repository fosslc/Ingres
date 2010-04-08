/*
**Copyright (c) 2004 Ingres Corporation
**
*/

/**
** Name: OPG_CB.H - Server control block for optimizer
**
** Description:
**      This file contains the server control block and related structures
**      defined for the optimizer
**
** History:
**      1-mar-86 (seputis)
**          initial creation
**	16-aug-91 (seputis)
**	    add opg_like to support distributed
**	10-Jan-2001 (jenjo02)
**	    Added OPG_CB *Opg_cb GLOBALREF, opg_opscb_offset,
**	    both used by GET_OPS_CB macro.
**      7-oct-2004 (thaju02)
**          Replace i4 with SIZE_TYPE for memory pool > 2Gig.
[@history_line@]...
**/

/**
** Name: OPGCB.H - Server Control block for the optimizer
**
** Description:
**	This structure contains the global variables for the optimizer.
**
** History:
**      20-feb-86 (seputis)
**          initial creation
**/

/*}
** Name: OPH_HDEC - global data structure for histdec().
**
** Description:
**      Consolidates floating point values used by
**	histdec() proc.
**
** History:
**      11-mar-90 (stec)
**          Written.
**	20-apr-90 (seputis)
**	    move into optimizer code
**	01-sep-93 (jhahn)
**	    Added opg_psession_id for maintaining server
**	    lifetime unique id's.
*/
typedef struct _OPH_HDEC
{
	f4	opq_f4neg;  /* multiplication factor
			    ** to be used in histdec()
			    ** when f4 negative
			    */
	f4	opq_f4nil;  /* value
			    ** to be used in histdec()
			    ** when f4 is zero
			    */
	f4	opq_f4pos;  /* multiplication factor
			    ** to be used in histdec()
			    ** when f4 positive
			    */
	f8	opq_f8neg;  /* multiplication factor
			    ** to be used in histdec()
			    ** when f8 negative
			    */
	f8	opq_f8nil;  /* value
			    ** to be used in histdec()
			    ** when f8 is zero
			    */
	f8	opq_f8pos;  /* multiplication factor
			    ** to be used in histdec()
			    ** when f8 positive
			    */
} OPH_HDEC;

/*}
** Name: OPT_GLOBAL - server-wide trace/timing flags
**
** Description:
**	This structure defines the global trace/timing flags for the server
**
** History:
**      29-jun-86 (seputis)
**          initial creation
[@history_line@]...
*/
#define                 OPT_BMGLOBAL      128
/* maximum number of bit entries in trace/timing array found in server */
#define                 OPT_GVALUES       7
/* number of values that can be associated with a trace vector at one time */
#define                 OPT_GWVALUES      7
/* number of trace flags with values associated with them */

typedef ULT_VECTOR_MACRO(OPT_BMGLOBAL, OPT_GVALUES) OPT_GLOBAL;

/*}
** Name: OPS_ALTER - characteristics of optimizer control by SET command
**
** Description:
**      This structure controls the characteristics of the optimizer that
**      can be altered by the SET command.  There will be a definition of
**      this structure in the server control block which will affect any
**      new session which is begun.  There will a definition in each
**      session control block which can be modified by individual sessions.
**
** History:
**     1-jul-86 (seputis)
**          initial creation
**	15-sep-89 (seputis)
**          add ops_amask to support flattening option
**	12-jul-90 (seputis)
**	    add OPS_ARESOURCE to fix bug 30809
**	28-jan-91 (seputis)
**	    added support for OPF startup flags
**	15-sep-98 (sarjo01)
**	    Added ops_timeoutabort 
**	31-aug-00 (inkdo01)
**	    Added ops_rangep_max for deprecation of OPC_RMAX constant.
**	3-nov-00 (inkdo01)
**	    Added ops_cstimeout to make OPN_CSTIMEOUT variable.
**	22-jul-02 (hayke02)
**	    Added ops_stats_nostats_factor. This change fixes bug 108327.
**	10-june-03 (inkdo01)
**	    Added ops_newenum, ops_hash to influence some new features.
**	12-nov-03 (inkdo01)
**	    Added ops_pq_dop/threshold to support parallel query processing.
**	4-mar-04 (inkdo01)
**	    Added ops_parallel to support "set [no]parallel [n]".
**	19-jan-05 (inkdo01)
**	    Added ops_pq_partthreads for partitioning/parallel query.
**	30-oct-08 (hayke02)
**	    Added ops_greedy_factor to adjust the ops_cost/jcost comparison
**	    in opn_corput() for greedy enum only. This change fixes bug 121159.
**	11-Nov-2009 (kiria01) SIR 121883
**	    Pass on cardinality_check setting in ops_nocardchk.
**	03-Dec-2009 (kiria01) b122952
**	    Add .opf_inlist_thresh for control of eq keys on large inlists.
[@history_line@]...
*/
typedef struct _OPS_ALTER
{
    SIZE_TYPE  ops_maxmemory;      /* maximum amount of memory which
                                        ** can be bound to a session
                                        ** - SET OPTIMIZER MEMORY <value>
                                        */
#define                 OPG_MEMSESMAX   200000
/* default maximum number of bytes that can be allocated at one time to an
** optimization request of any one session
*/

    i4         ops_sestime;        /* maximum amount of time which can
                                        ** be used by one optimization
                                        ** - value of 0 means do not timeout
                                        ** - SET JOINOP TIMEOUT <value>
                                        */
#define                 OPG_TIMSESMAX   0
/* default is that optimization should timeout based on estimated optimization
** - if this value is positive then it provides an absolute max on the query
** processing time */

    OPO_COST	    ops_cpufactor;      /* CPUFACTOR to be used
                                        ** - SET CPUFACTOR
                                        */
#define                 OPG_CPUFACTOR   ((OPO_COST)100.0)
/* This is used in the calculation of the cost of a particular query plan.
** It means that for cost "1 disk i/o" is equal to "cpu * OPG_CPUFACTOR"
** where cpu is the number of tuples compared or moved
*/
    bool            ops_timeout;        /* TRUE if optimizer should timeout
                                        ** default is FALSE
                                        */
    bool            ops_noproject;      /* TRUE if the session requires
                                        ** no projection be made for aggregate
                                        ** function by domain lists
                                        ** - SET AGGREGATE NOPROJECT
                                        ** - SET AGGREGATE PROJECT
                                        */
#define                 OPG_NOPROJECT   FALSE
/* default is to project for aggregates 
*/
    bool	    ops_newenum;	/* TRUE if session should use new
					** OPF enumeration heuristic
					** else FALSE
					*/
    bool	    ops_hash;		/* FALSE if hash join/aggregation 
					** are to be inhibited 
					*/
    bool	    ops_parallel;	/* TRUE if parallel plans are to
					** be generated */
    bool	    ops_autostruct;	/* TRUE if session default is to use
					** table auto structure options */
    i4         ops_qsmemsize;      /* size of memory buffer used for
                                        ** quicksorts - should be changable
                                        ** by SET command
                                        ** - initially set to DB_QSMEMSIZE
                                        */
#define                 OPG_QSMEMSIZE   DB_QSMEMSIZE
/* default number of bytes in a quicksort buffer
*/
    i4              ops_storage;        /* default storage structure to use
                                        ** when creating temporaries
                                        ** - SET RET_INTO
                                        ** - default is DB_SORT_STORE (sorted
                                        ** heap)
                                        */
    bool            ops_compressed;     /* TRUE - if compressed storage
                                        ** structure is required */
    bool            ops_qep;            /* TRUE - if QEP should be printed
                                        ** i.e. if SET QEP command executed */
    i2		    ops_cnffact;	/* Factor applied to estimated #fax
					** after CNF transform to determine if
					** where is too complex */
    i4		    ops_amask;		/* Alter mask describing various booleans */
#define                 OPS_ANOFLATTEN  1
/* TRUE if flattening should be suppressed */
#define			OPS_ARESOURCE	2
/* TRUE if enumeration should be entered for all queries so that resource
** control can get more accurate histogram calculations */
#define                 OPS_ACOMPLETE   4
/* TRUE - if the complete flag should be turned on by default, allow
** a backward compatibility path for 6.3/3 */
#define                 OPS_ANOAGGFLAT  8
/* TRUE - avoid flattening aggregates if this flag is turned on */
    i4		    ops_scanblocks[DB_NO_OF_POOL];	
					/* number of blocks read per DIO when
					** scanning a relation */
    OPO_COST	    ops_tout;		/* used to convert the cost units into
					** millisec's, if zero then the default
					** will be used */
    OPO_COST	    ops_repeat;		/* used to affect the timeout for repeat
					** query evaluation, if zero then the default
					** will be used */
    f4		    ops_sortmax;	/* optimizer will avoid plans in which the
					** estimated size of a sort would involve
					** data which is larger than this number, unless
					** the semantics require that a sort is done, if
					** zero then no limit will be used */
    f4		    ops_exact;		/* default selectivity for exact keys */
    f4		    ops_range;		/* default selectivity for inexact keys */
    f4		    ops_nonkey;		/* default selectivity for non-sargable keys*/
    f4		    ops_spatkey;	/* default selectivity for spatial preds */
    i4         ops_timeoutabort;   /* SET JOINOP TIMEOUTABORT value */ 
    i4		    ops_rangep_max;	/* max range preds with RQ parms */
    i2		    ops_cstimeout;	/* count of calls to opn_jintersect for
					** each call to opn_timeout. */
    u_i2	ops_inlist_thresh;	/* Count at which IN/NOT IN list drop to range keys
					** or 0 to disable. */

    f4		    ops_stats_nostats_factor;
    i4		    ops_pq_dop;		/* degree of parallelism for parallel
					** queries */
    f4		    ops_pq_threshold;	/* threshold plan cost to invoke
					** parallel processing */
    i4		    ops_pq_partthreads;	/* max threads per partitioned table/
					** join in parallel query */
    f4		    ops_greedy_factor;	/* adjust ops_cost/jcost comparison
					** in opn_corput() for greedy enum */
    bool	ops_nocardchk;		/* Set if in legacy 'ignore' mode */
}   OPS_ALTER;

/*}
** Name: OPG_CB - Server control block
**
** Description:
**      This structure contains the variables which are global to 
**      all sessions within the optimizer. These variables are initialized
**      at server startup time and are active for the life of the server.
**
** History:
**	1-mar-86 (seputis)
**          initial creation
**	14-nov-88 (seputis)
**          add opg_first in order to help remove ADF_CB in opsstartup.c
**	26-dec-90 (seputis)
**	    add support for new server startup flags
**	15-dec-91 (seputis)
**          add parameter to reduce floating point computations for MHexp
**      18-may-92 (seputis)
**          - bug 44107 - check for null join should not be eliminated
**          when ifnull function is used
**	8-sep-92 (ed)
**          make star structures visible
**      16-oct-92 (ed)
**          - fix bug 40835 - wrong number of duplicates when union views and
**          subselects are used
**      06-mar-96 (nanpr01)
**          - Increase tuple size project. Set the max tuple size for the
**	      opf session. Added field opg_maxtup, opg_pagesize, opg_tupsize,
**	      opg_maxpage.
**	10-Jan-2001 (jenjo02)
**	    Added opg_opscb_offset.
[@history_line@]...
*/
typedef struct _OPG_CB
{
    PTR             opg_memory;         /* This is memory which as been
                                        ** allocated for the optimizer by
                                        ** the memory management routines.
                                        */
    PTR             opg_rdf_memory;     /* This is memory which may be bound to
                                        ** an object for a long period of time.
                                        ** All memory allocated for relation
                                        ** descriptors will eventually become
                                        ** part of long term memory since
                                        ** caching will be used.  RDF pool
                                        ** uses same id as opg_memory for now.
                                        */
#define                 OPG_MTRDF       300
/* maximum number of tables which can be contained in the RDF cache */

    PTR             opg_rdfhandle;      /* this is the handle received when RDF
                                        ** was started and required as input
                                        ** to any request for RDF information
                                        */
    ADI_OP_ID       opg_eq;             /* the ADF operator id for the
                                        ** "=" operator
                                        */
    ADI_OP_ID       opg_ne;             /* the ADF operator id for the
                                        ** "!=" operator
                                        */
    ADI_OP_ID       opg_notlike;	/* the ADF operator id for the
                                        ** "not like" operator
                                        */
    ADI_OP_ID       opg_exists;		/* the ADF operator id for the
                                        ** SQL EXISTS */
    ADI_OP_ID       opg_nexists;	/* the ADF operator id for the
                                        ** SQL NOT EXISTS */
    ADI_OP_ID       opg_any;		/* the ADF operator id for the
                                        ** QUEL ANY */
    ADI_OP_ID       opg_isnull;		/* the ADF operator id for the
                                        ** SQL "IS NULL" */
    ADI_OP_ID       opg_isnotnull;	/* the ADF operator id for the
                                        ** SQL "IS NOT NULL" */
    ADI_OP_ID       opg_lt;             /* the ADF operator id for the
                                        ** "<" operator
                                        */
    ADI_OP_ID       opg_ge;             /* the ADF operator id for the
                                        ** ">=" operator
                                        */
    ADI_OP_ID       opg_gt;             /* the ADF operator id for the
                                        ** ">" operator
                                        */
    ADI_OP_ID       opg_min;		/* the ADF operator id for the
                                        ** QUEL MIN*/
    ADI_OP_ID       opg_max;		/* the ADF operator id for the
                                        ** QUEL MAX*/
    ADI_OP_ID       opg_count;		/* the ADF operator id for the
                                        ** COUNT */
    ADI_OP_ID       opg_avg;		/* the ADF operator id for the
                                        ** AVG */
    ADI_OP_ID       opg_sum;		/* the ADF operator id for the
                                        ** sum */
    ADI_OP_ID       opg_le;             /* the ADF operator id for the
                                        ** "<=" operator
                                        */
    ADI_OP_ID       opg_scount;		/* the ADF operator id for the
                                        ** COUNT(*) */
    ADI_OP_ID       opg_like;		/* the ADF operator id for the
                                        ** LIKE */
    ADI_OP_ID       opg_iftrue;		/* the ADF operator id for the
                                        ** iftrue function used to ensure
					** proper NULL handling in outer joins
					*/
    ADI_OP_ID       opg_ifnull;         /* the ADF operator id for the
                                        ** ifnull function */
    ADI_OP_ID       opg_iirow_count;	/* the ADF operator id for the
                                        ** increment integer function */
    PTR		    opg_expand1;	/* expansion */
    i4		    opg_mxsess;         /* maximum number of sessions which
					** OPF was started with */
    i4		    opg_actsess;        /* active number of sessions which
					** can execute in DMF, used for cache
					** estimation purposes */
    bool	    opg_first;		/* TRUE if the first session has
					** been started */
    OPF_QEFTYPE	    opg_qeftype;	/* this is the type of QEF which is
					** available for this server */
    OPF_STATE	    opg_smask;		/* server wide booleans affecting operation
					** of OPF */
    DD_NODELIST	    *opg_nodelist;	/* relative power of various nodes */
    DD_NETLIST	    *opg_netlist;	/* relative network speeds */
    RDD_CLUSTER_INFO *opg_cluster;	/* list of nodes on the same cluster
					** as the star server */
    SCF_SEMAPHORE   opg_semaphore;      /* semaphore used to protect concurrent
                                        ** access to server control block 
                                        ** - only the following structures can
                                        ** be updated after server startup
                                        */
    CS_CONDITION    opg_condition;      /* condition used to limit users within
					** OPF
                                        */
    i4	    opg_activeuser;	/* number of active OPF users which should be
					** less than or equal to opg_mxsess */
    OPS_ALTER       opg_alter;          /* - contains all characteristics used
                                        ** to initialize a new session
                                        ** - the values in this structure can
                                        ** be modified by a SET SERVER command
                                        ** and should be protected by semaphores
                                        */
    bool            opg_check;          /* TRUE if any of the
                                        ** of the global trace/timing flags
                                        ** are set.  Used for quick check.
                                        */
    OPT_GLOBAL	    opg_tt;             /* global trace/timing flags */
#define                 OPT_GSTARTUP    1
/* trace server startup */
    DD_LDB_DESC     opg_ddbsite;        /* node name for the location
					** of the server, other fields in 
					** this structure should be ignored
                                        ** - used to define the target site
					** for a retrieve in distributed
					*/
    OPH_HDEC	    opg_hvadjust;	/* used to help process histograms
					** in a machine independent way */
    i4	    opg_swait;		/* Number of times a sessions had to
					** wait for a slot in OPF */
    i4	    opg_slength;	/* Maximum length of the wait queue
					** for OPF */
    i4	    opg_sretry;		/* Number of retry's due to suspected
					** deadlock */
    i4	    opg_dmfconnected;	/* number of DMF sessions which are
					** connected to server */
    i4		    opg_mask;		/* mask of server wide booleans */
#define                 OPG_CONDITION   1
/* TRUE - if condition code should be used to limit users of OPF */
    i4	    opg_waitinguser;	/* number of OPF users which are in OPF
					** or waiting to entery OPF */
    i4	    opg_mwaittime;	/* max wait time */
    f8		    opg_avgwait;	/* average wait time */
    f8              opg_lnexp;          /* precomputed constant MHln(FMIN) */
    i4	    opg_psession_id;	/* Count of total number of sessions
					** opened during life of server.
					*/
    i4	    opg_maxtup;		/* maximum tuple size available   
					** based on dmf buffer pool page
					** availability.
					*/
    i4	    opg_maxpage;	/* maximum page size available   
					** based on dmf buffer pool page
					** availability.
					*/
    i4	    opg_pagesize[DB_MAX_CACHE];	
					/* page sizes available   
					** based on dmf buffer pool page
					** availability.
					*/
    i4	    opg_tupsize[DB_MAX_CACHE];	
					/* tuple sizes available   
					** based on dmf buffer pool page
					** availability.
					*/
    i4	    opg_opscb_offset;		/* Offset from SCF's SCB to
					** session's OPS_CB
					*/
}   OPG_CB;

GLOBALREF	OPG_CB	*Opg_cb;	/* Global pointer for all */
