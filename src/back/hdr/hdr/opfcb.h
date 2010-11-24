/*
**Copyright (c) 2004 Ingres Corporation
**
*/

/**
** Name: OPFCB.H - Definition of the Optimizer facility external
**      interface control block
**
** Description:
**      This file contains the definition of the control block used by
**      an external caller for services of the Optimizer facility.
**
** History:
**      31-dec-85 (seputis)
**          initial creation
**	30-apr-87 (mmm)
**	    Changed multi-character constants to CV_C_CONST_MACRO() calls
**	09-jul-89 (stec)
**	    Added defines for error codes in the new version of OPQ routines
**	    (they should really be defined on eropf.h). Also removed
**	    E_OP0928, E_OP0929, changed E_OP092A.
**	15-sep-89 (seputis)
**	    added error messages and trace flags, E_OP0284_RESDOM,
**	    E_OP038E_MAXOUTERJOIN, E_OP038F_OUTERJOINSCOPE, OPT_F124_BLOCKING
**	    OPT_F125_TIMEFACTOR
**	11-oct-89 (stec)
**	    Added the E_OP089A define.
**	30-oct-89 (stec)
**	    Added the E_OP089B define.
**	12-dec-89 (stec)
**	    Added defines in the E_OP0960 range.
**	06-feb-90 (stec)
**	    Added defines:
**		E_OP095E_BAD_UNIQUE
**		E_OP095F_WARN_UNIQUE
**	27-mar-90 (stec)
**	    Added E_OP093F_BAD_COUNT define.
**	04-may-90 (sandyh)
**	    Added E_OP0096_INSF_OPFMEM define.
**	25-jul-90 (stec)
**	    Changed some E_ defines to W_ and I_ since some return codes
**	    identify warnings and informational messages.
**	06-nov-90 (stec)
**	    Added E_OP089D_QSF_INFO, E_OP089E_QSF_UNLOCK,
**	    E_OP089F_QSF_FAILCREATE defines.
**	08-nov-90 (stec)
**	    Added opf_thandle to OPF_CB struct.
**	18-dec-90 (kwatts)
**	    Smart Disk integration:
**	    Added E_OP08A0_SD_COMPILE_FAIL  and new trace point values
**	    OPT_F056_NOSMARTDISK, OPT_F057_WHYNOSMARTDISK,
**	    OPT_F058_SD_CONJUNCT_TRACE.
**	22-jan-91 (stec)
**	    Added define E_OP0928_BAD_COMPLETE.
**	    Added define E_OP08A1_TOO_SHORT.
**	4-feb-91 (seputis)
**	    Add support for AGGREGATE_FLATTEN, COMPLETE flag, and some new
**	    error messages, and trace points.
**	13-feb-91 (stec)
**	    Added defines E_OP0929_ADC_HDEC, E_OP078B_ADC_HDEC,
**	    E_OP0980_ADI_PM_ENCODE, E_OP0981_ADI_FICOERCE,
**	    E_OP0982_ADF_FUNC, E_OP0983_ADC_COMPARE.
**	18-mar-91 (stec)
**	    Added E_OP0990_BAD_RPTFACTOR, W_OP0991_INV_RPTFACTOR.
**	18-mar-91 (seputis)
**	    Added OP187 trace point
**	25-mar-1991 (bryanp)
**	    Add E_OP0AA0_BAD_COMP_VALUE for Btree compression project.
**	16-aug-1991 (bryanp)
**	    Add E_OP000E_CAGGTUP for aggregate temporary which is too large
**      11-oct-91 (seputis)
**          Added OP193 trace point,... added 6.5 error messages to reserve numbers
**      11-dec-1991 (fred)
**          Added #define for OPT_F065_LVCH_TO_VCH which instructs opc
**          to build the descriptor as if long varchar blobs are being
**          returned as varchar.
**	30-apr-92 (bryanp)
**	    Added E_OP0010_RDF_DEADLOCK error message.
**	14-may-92 (rickh)
**	    Added OP199 trace point for printing out QEPs without
**	    cost statistics.
**	11-sep-92 (ed)
**	    Changed OPT_F065_LVCH_TO_VCH to OPT_F073_LVCH_TO_VCH since f065
**	    was already used in 6.4
**      9-nov-92 (ed)
**          add E_OP03A4 as part of FIPS union support
**	24-nov-92 (rickh)
**	    Added E_OP08A3_UNKNOWN_STATEMENT.
**	14-dec-92 (rganski)
**	    Added E_OP092E_CVSN_CERR: Error %0d occurred when converting
**	    STANDARD_CATALOG_LEVEL value of '%1c'.
**	    Added E_OP092F_STD_CAT: <utility> does not match the standard
**	    catalog level in database.
**	    Added E_OP0960_BAD_LENGTH: bad value for optimizedb's -length flag.
**	28-dec-92 (andre)
**	    defined E_OP0011_TREE_PACK and E_OP0012_TEXT_PACK
**	18-jan-93 (rganski)
**	    Added optimizer error
**	    E_OP091E_BAD_VALUE_LENGTH:SS5000B_INTERNAL_ERROR "Wrong input value
**	    '%0d' for statistics on table '%1c', column '%2c'; \nInvalid value
**	    length; does not match length of column or < 1."
**	26-apr-26 (ed)
**	    add new outer join error messages
**	26-apr-93 (rganski)
**	    Added optimizedb error E_OP0975_STAT_COUNT, which is returned when
**	    the number of character set statistics in an input file does not
**	    match the length of the histogram boundary values.
**	07-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Added opf_dbxlate to OPF_CB, which points to case translation mask.
**	    Added opf_cat_own to OPF_CB, which points to catalog owner name
**	21-jun-93 (rganski)
**	    Added consistency check error E_OP03A5_KEYINCELL, which is returned
**	    from oph_interpolate when a key value falls in a histogram cell but
**	    is not equal to either boundary value -indicates problem in cell
**	    splitting.
**	2-nov-93  (robf)
**          Add error E_OP0984_NO_PRIV_TABLE_STATS
**	31-jan-94 (ed)
**	    added new outer join error message codes
**	03-feb-94 (rickh)
**	    Added OPC outer join error messages.
**	15-feb-94 (ed)
**	    add new OPF outer join error messages
**	04-may-95 (nick)
**	    added E_OP009B_STACK_OVERFLOW
**	1-nov-99 (inkdo01)
**	    Added E_OP0985_NOHIST for optimizedb error detection.
**	20-jul-00 (inkdo01)
**	    Added W_OP0992_ROWCNT_INCONS for statdump of histogram whose 
**	    row count differs from current table.
**	10-Jan-2001 (jenjo02)
**	    Added opf_opscb_offset to OPF_CB.
**	17-july-01 (inkdo01)
**	    Added E_OP0993_XRBADMIX for optdb/statd "-xr" cmd line option.
**	9-Feb-2004 (schka24)
**	    Added partition def internal errors
**      12-Apr-2004 (stial01)
**          Define opf_length as SIZE_TYPE.
**      7-oct-2004 (thaju02)
**          Replace i4 with SIZE_TYPE for memory pool > 2Gig.
**	28-Feb-2005 (schka24)
**	    Add generic OP009D to avoid errmsg proliferation for internal
**	    error checks.
**      16-oct-2006 (stial01)
	    Added opf_locator for blob locators.
**	1-Aug-2007 (kschendel) SIR 122513
**	    Swipe dead trace point OP131 for disabling join-time partition
**	    qualification.
**	2-Apr-2008 (kibro01) b120056
**	    Add W_OP0976 warning for lack of precision warning
**	18-Aug-2008 (kibro01) b115336
**	    Add W_OP096B warning for no cells in histogram (from statdump -zq)
**	27-Oct-09 (smeke01) b122794
**	    Correct #define name for OP147. 
**	11-Nov-2009 (kiria01) SIR 121883
**	    Add OPF_CARDCHK and OPF_NOCARDCHK for controlling cardinality
**	    checks.
**	20-Nov-2009 (kschendel) SIR 122890
**	    Redefine dead trace point OP129 for (future!) disable of
**	    insert/select-via-load optimization.
**	03-Dec-2009 (kiria01) b122952
**	    Add .opf_inlist_thresh for control of eq keys on large inlists.
**	01-Feb-2010 (maspa05) b123140
**	    Added E_OP0994 error for multiple -r flags on optimizedb/statdump
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	04-Aug-2010 (wanfr01) b124188
**	    Removed  OPF_ONE_REL_OPT - crossed in accidentally
**     12-Aug-2010 (horda03) b124109
**          Added trace point OP218 which forces optimiser to insert EXCHANGE
**          nodes (when Parallel queries enabled).
**	28-Jun-2010 (smeke01) b123969
**	    Added diagnostic tracepoint OP217 which forces optimiser to use
**	    hash joins. Requires xDEBUG to be defined in opjjoinop.c.
**/

/*}
** Name: OPF_OPCODES - Defines for opcodes
**
** Description:
**      The op codes necessary to call the Optimizer facility. Each
**      op code represents an operation or service which can be performed
**      by the OPF.
**
** History:
**     31-dec-85 (seputis)
**          initial creation
*/

#define                 OPF_STARTUP     1	/* Start up the OPF for server*/
#define                 OPF_SHUTDOWN    2	/* Shutdown OPF for server    */
#define                 OPF_BGN_SESSION 3	/* Begin an OPF session       */
#define                 OPF_END_SESSION	4	/* End an OPF session         */
#define                 OPF_CREATE_QEP	5	/* Create QEP from query tree */
#define                 OPF_ALTER       6       /* alter OPF characteristics  */

/*}
** Name: OPF_QEFTYPE   - QEF capabilities available for optimizer
**
** Description:
**      This definition is used to classify the capabilities of the
**      QEF which the optimizer needs to create plans for
**
** History:
**      29-mar-89 (seputis)
**          initial creation
[@history_template@]...
*/
typedef i4  OPF_QEFTYPE;
#define			OPF_SQEF	1
/* Standard QEF, has all capabilities for joins */
#define                 OPF_LQEF        2
/* Limited QEF, cannot do joins, only can send text */
#define                 OPF_GQEF        3
/* Gateway QEF, some tables in IMS qef cannot reference TIDs and cannot join secondary
** indexes to one another, also updates and deletes need to be by position for
** non-ingres tables
*/

/*}
** Name: OPF_STATE - mask to describe type functionality OPF should assume
**
** Description:
**      Various mask fields describing type of OPF
**
** History:
**      31-mar-89 (seputis)
**          initial creation
**      15-sep-89 (seputis)
**          added OPF_SUBOPTIMIZE to fix a flattening bug 8003
**      12-jul-90 (seputis)
**          fix 30809 - added OPF_RESOURCE
**	27-dec-90 (seputis)
**	    add server startup flag for completeness
**	7-jan-91 (seputis)
**	    added OPF_AGGNOFLAT to support server startup flag
**	16-may-97 (inkdo01)
**	    Added OPF_OLDCNF to request old "forced" conjunctive normal form
**	    behaviour.
**	29-jun-99 (hayke02)
**	    Added OPF_STATS_NOSTATS_MAX, OPT_F077_STATS_NOSTATS_MAX and
**	    OPT_F078_NO_STATS_NOSTATS_MAX for new dbms parameter
**	    opf_stats_nostats_max and trace point op205, op206 respectively
**	    - this new parameter (and op205) will limit tuple estimates
**	    for joins between table with stats and table without by assuming
**	    uniform data distribution. Trace point op206 will switch off
**	    both opf_stats_nostats_max and op205, and existing trace point
**	    op201 will override opf_stats_nostats_max, op205 and op206.
**	    This change fixes bug 97450.
**	16-jul-2002 (toumi01)
**	    Add config parameter for hash join (default is off); this startup
**	    setting is toggled by OP162.
**	22-jul-02 (hayke02)
**	    Added opf_stats_nostats_factor for new dbms parameter
**	    opf_stats_nostats_factor. This is used in the scaling of the
**	    maxonehisttups calculation in oph_join() when opf_stats_nostats_max
**	    is switched ON. This change fixes bug 108327.
**	14-jan-03 (inkdo01)
**	    Changed OPF_HASH_JOIN to make room for change 458644 and added 
**	    OPF_NEW_ENUM for OPF speedup of complex queries.
**	21-mar-03 (hayke02)
**	    Added opf_old_subsel to switch off the 'not in'/'not exists'
**	    subselect to outer join transform. This change fixes bug 109670.
**	30-mar-03 (wanfr01)
**	    Bug 108353, INGSRV 2201
**	    Added OPF_NOAGGCOMBINE to allow reoptimizing the query without
**	    attempting to combine aggregates.  
**	26-jul-04 (hayke02)
**	    Add op211 and opf_old_idxorder to use reverse var order when adding
**	    useful secondary indices. This change fixes problem INGSRV 2919,
**	    bug 112737.
**	30-oct-08 (hayke02)
**	    Add op246 and opf_greedy_factor to adjust the ops_cost/jcost
**	    comparison in opn_corput() for greedy enum only. op246 takes an
**	    integer percentage value i.e. 75 will multiply ops_cost by 0.75,
**	    and opf_greedy_factor takes a float. This change fixes bug 121159.
**	02-Dec-08 (wanfr01)
**	    SIR 121316 - Added op212 to allow enabling table_auto_structure
**	22-mar-10 (smeke01) b123457
**	    Add trace point op214 to display query tree in op146 style, at the
**	    same points in the code as trace point op170. 
**	13-may-10 (smeke01) b123727
**	    Add trace points op215 (OPT_F087_ALLFRAGS) and
**	    op216 (OPT_F088_BESTPLANS).
**	    Trace point op215 prints all query fragments (strictly 'all query
**	    fragments that have made it into an OPO_CO structure'), even if
**	    they are to be discarded, together with all best plans.  
**	    Trace point op216 prints out only the best plans, as they are
**	    encountered.
**	05-aug-2010 (wanfr01) b124202
**	    There are duplicate OPF_STATE flags - that can't be good.
**	    Switching over to the standard hex flags which are more typo resistant.
[@history_line@]...
[@history_template@]...
*/
typedef i4  OPF_STATE;
#define                 OPF_SDISTRIBUTED        0x1
/* TRUE if this is a distributed thread, -valid only on session startup */
#define                 OPF_TIDDUPS             0x2
/* if this is set then TIDs are not available for duplicate handling
** in the optimizer -valid on server startup only */
#define                 OPF_TIDUPDATES          0x4
/* if this is set then TIDs cannot be used for updating a relation
** so that only update by position is allowed -valid on server startup only */
#define                 OPF_INDEXSUB            0x8
/* if this is set then index substitution can only be used if the
** all the attributes can be obtained from one index, i.e. TID joins
** are not allowed -valid on server startup only */
#define			OPF_SUBOPTIMIZE         0x10
/* bit is set if optimization for flattening should be disabled, this should
** only be used for the "sybase bug" in which the subselect references a
** table which does not have any rows in it, this will only affect the
** current query being optimize */
#define                 OPF_RESOURCE            0x20
/* set if resource control is enabled for the server, which implies that
** enumeration should be turned on for all query plans to get accurate
** tuple counts, this mask can be set in opf_smask for either session
** startup or server startup */
#define                 OPF_NOAGGFLAT           0x40
/* AGGREGATE_FLATTEN server startup flag
** - set if corelated aggregates should not be flattened */
#define                 OPF_COMPLETE            0x80
/* COMPLETE server startup flag
** - set if all relations are assumed to be complete if no 
** histogram info is available */
#define			OPF_OLDJCARD		0x100
/* set if pre-2.0 join cardinality computations are to be used */
#define			OPF_OLDCNF		0x200
/* TRUE - ALL where clauses are transformed to conjunctive normal form
** (pre-2.0 behaviour) */
#define			OPF_STATS_NOSTATS_MAX	0x400
/* opf_stats_nostats_max - limit tuple estimates for joins between table
** with stats and table without by assuming uniform data distribution */
#define			OPF_OLDSUBSEL		0x800
/* opf_old_subsel - don't perform 'not in'/'not exists' -> outer join
** transform */
#define			OPF_HASH_JOIN		0x1000
/* TRUE - hash joins enabled server startup flag; OP162 toggles the current
effective setting */
#define			OPF_NEW_ENUM		0x2000
/* TRUE - queries deemed complex enough (with enough tables/indexes) will
** be compiled first with the new optimized enumeration heuristic. */
#define			OPF_OLDIDXORDER		0x4000
/* opf_old_idxorder - use old 'reverse var' order when adding useful secondary
** indices */
#define			OPF_GREEDY_FACTOR	0x8000
/* opf_greedy_factor - adjust ops_cost by this figure when comparing to jcost
** in opn_corput() for greedy enum only */
#define			OPF_DISABLECARDCHK	0x10000
/* Enable legacy ignoring of cardinality checks */
#define			OPF_GSUBOPTIMIZE	0x20000
/* bit is set if on server startup, the "flattening feature" should not
** be used */
#define			OPF_NOAGGCOMBINE	0x40000
/* Set to disable combining aggregates during optimizer retry
** as this may invalidate some query plans */

/*}
** Name: OPF_CB - Control Block used to call the optimizer
**
** Description:
**      This structure defines the OPF control block used by an external
**      caller for services of the OPF Facility.  Note, this is not the
**      internal session control block used by the optimizer to process
**      a query.
**
** History:
**     31-dec-85 (seputis)
**          initial creation
**     18-oct-88 (seputis)
**          added opf_actsess to improve caching cost model for OPF
**      9-aug-89 (seputis)
**          added SET [NO]STANDARD SUBSELECT flags
**	24-jun-90 (seputis)
**	    add error messages for b8410
**      16-jul-90 (seputis)
**          add OPF_ARESOURCE and OPF_ANORESOURE to fix b30809
**	08-nov-90 (stec)
**	    add opf_thandle to pass query text object id for error
**	    handling.
**	2-jan-91 (seputis)
**	    added several server startup flags
**	4-feb-91 (seputis)
**	    Add support for AGGREGATE_FLATTEN, COMPLETE flag, and some new
**	    error messages, and trace points
**	22-apr-91 (seputis)
**	    Added new trace points OP188,OP189,OP191,OP192 and new error messages
**	07-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Added opf_dbxlate to OPF_CB, which points to case translation mask.
**	    Added opf_cat_own to OPF_CB, which points to catalog owner name
**	05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	14-dec-93 (swm)
**          Bug #56447
**	    Changed opf_sid from PTR to CS_SID.
**      06-mar-96 (nanpr01)
**          - Increase tuple size project. Set the max tuple size for the
**            opf session. Added field opf_maxtup, opf_pagesize, opf_tupsize,
**	      opf_maxpage.
**	12-dec-96 (inkdo01)
**	    Added opf_maxmemf parm which defines the proportion of OPF mem 
**	    pool available to any single session.
**      14-mar-97 (inkdo01)
**          Added opf_spat_key to assign default selectivity to spatial preds.
**	16-may-97 (inkdo01)
**	    Add tracepoints to control CNF transform on a session basis, and
**	    a flag to control it on a server-wide basis.
**	31-aug-98 (sarjo01)
**	    Added opf_joinop_timeout, now an installation configurable
**	    parameter.
**	15-sep-98 (sarjo01)
**	    Added opf_joinop_timeoutabort, now an installation configurable
**	    parameter. Added new error condition, E_OP0018_TIMEOUTABORT
**	18-june-99 (inkdo01)
**	    Added opf_default_user for temp table model histogram feature.
**	28-jan-00 (inkdo01)
**	    Added opf_qefhash for hash join implementation.
**	31-aug-00 (inkdo01)
**	    Added opf_rangep_max to allow deprecation of OPC_RMAX.
**	3-nov-00 (inkdo01)
**	    Added opf_cstimeout to make OPN_CSTIMEOUT variable.
**	3-nov-00 (inkdo01)
**	    Added opf_cstimeout to make OPN_CSTIMEOUT variable.
**	10-Jan-2001 (jenjo02)
**	    Added opf_opscb_offset to OPF_CB.
**	22-jul-02 (inkdo01)
**	    Added OPF_[NO]OPTIMIZEONLY to remove need for tp auth.
**	18-oct-02 (inkdo01)
**	    Set up trace points op153, 208 and 209 for the new enumeration mechanism.
**	14-jan-03 (inkdo01)
**	    Moved tp op207 to op210 to make room for change 458644.
**	4-june-03 (inkdo01)
**	    Added support for "set [no]ojflatten", "set [no]joinop newenum"
**	    and "set [no]hash" statements.
**	10-nov-03 (inkdo01)
**	    Add opf_pq_dop, opf_pq_threshold for parallel query support.
**	19-jan-05 (inkdo01)
**	    Add opf_pq_partthreads for parallel/partitioned table support.
**	10-oct-05 (inkdo01)
**	    Add op163 to prevent use of histograms for join cardinality.
**	6-Jul-2006 (kschendel)
**	    Unique dbid should be an i4.
**	13-may-2007 (dougi)
**	    Add opf_autostruct.
**	 5-dec-08 (hayke02)
**	    Add trace point op213 to report non-select list columns missing
**	    stats. This change addresses SIR 121322.
**	01-Jun-2009 (hanje04)
**	    Make opf_level an i2 instead of a bool. bools are an enum on
**	    OSX and therefore cannot differentiate between 1 & 2
**	14-Oct-2010 (kschendel) SIR 124544
**	    Delete RET-INTO and result structure stuff;  it's all done inside
**	    PSF now.
*/
typedef struct _OPF_CB
{
    struct _OPF_CB  *opf_next;		/* forward pointer                    */
    struct _OPF_CB  *opf_prev;		/* backward pointer		      */
    SIZE_TYPE	    opf_length;		/* length of this structure	      */
    i2		    opf_type;		/* type code for this structure	      */
#define                 OPFCB_CB        4   /* 4 until real code is assigned  */
    i2		    opf_s_reserved;	/* possibly used by mem mgmt          */
    PTR		    opf_l_reserved; 
    PTR		    opf_owner;		/* owner for internal memory mgmt     */
    i4		    opf_ascii_id;	/* so as to see in dump		      */
#define                 OPFCB_ASCII_ID  CV_C_CONST_MACRO('O', 'P', 'F', 'B')

/* The following parameter is returned by every call of the OPF facility      */
    DB_ERROR        opf_errorblock;     /* error block			      */

/*
** The following I/O parameters are used by every call to the OPF facility
** except OPF_STARTUP, OPF_SHUTDOWN
*/
    PTR             *opf_scb;           /* session control block for OPF      */
/*
** The remaining components are I/O parameters which are associated with
** specific OPF operations.  
*/

/* I/O parameters for OPF_ALTER                                               */
    i2            opf_level;          /* users affected by this alter       */
#define                 OPF_SERVER     1
/* - the server level alter will alter the characteristics of any session which
** is begun from this point onward - it will NOT affect the characteristics of
** sessions which have already been attached to the server.  
*/
#define                 OPF_SESSION    2
/* alter affects only the current session - (i.e. opf_scb ptr to affected
** session).  This is the default and the only case supported in phase
** one of Jupiter.  This is equivalent to the current semantics
** of the 4.0 backend.
** SET <alter_op>
*/

    bool	    opf_locator;
    i4              opf_alter;          /* operation code for alter           */
#define                 OPF_CPUFACTOR 1
/* SET CPUFACTOR - opf_value is a required parameter
** - use for "cost comparison" - this number is the weight of CPU versus I/O
** where the default is 1 unit of I/O is equivalent to 100 units of CPU.
*/
#define                 OPF_TIMEOUT   2
/* SET JOINOP TIMEOUT - opf_value is required parameter
** - causes optimizer to timeout if current plan is less than time used to find
** it,  if no value is supplied then opf_value should be 0,
*/
#define                 OPF_NOTIMEOUT 3
/* SET JOINOP NOTIMEOUT 
** - causes optimizer to find the best plan without timing out
*/

/*	notused -- 4 */

#define                 OPF_QEP       5
/* SET QEP - no opf_value required
**  - enable the display of the query execution plan
*/
#define                 OPF_NOQEP     6
/* SET NOQEP - no opf_value required
**  - disable the display of the query execution plan
*/
#define                 OPF_MEMORY    7
/* no set command defined yet - opf_value is required parameter
** - amount of memory allowed to be used per session for optimization
*/
#define                 OPF_SUBSELECT 8
/* SET STANDARD SUBSELECT - no opf_value required
** - enable standard conforming error checking in subselects
*/
#define                 OPF_NOSUBSELECT 9
/* SET STANDARD SUBSELECT - no opf_value required
** - disable standard conforming error checking in subselects
*/
#define                 OPF_ARESOURCE   10
/* SET RESOURCE CONTROL - causes enumeration to be used for this session from
** this point on, used in opf_alter with the OPF_ALTER entry point */
#define                 OPF_ANORESOURCE   11
/* SET NORESOURCE CONTROL - causes "fast enumeration" to be used for single 
** variable queries this point on, used in opf_value with the OPF_ALTER entry point */
#define                 OPF_TIMEOUTABORT   12
#define			OPF_OPTIMIZEONLY   13
/* SET OPTIMIZEONLY - eliminate need for tp auth in parser by sending as alter
** flag. */
#define			OPF_NOOPTIMIZEONLY   14
/* SET NOOPTIMIZEONLY - eliminate need for tp auth in parser by sending as alter
** flag. */
#define                 OPF_OJSUBSELECT 15
/* SET OJSUBSELECT - no opf_value required
** - enable "not exists"/"not in"/"not = any" subselect to outer join
**	flattening.
*/
#define                 OPF_NOOJSUBSELECT 16
/* SET NOOJSUBSELECT - no opf_value required
** - disable "not exists"/"not in"/"not = any" subselect to outer join
**	flattening.
*/
#define                 OPF_NEWENUM	17
/* SET JOINOP NEW  - no opf_value required
** - enable new OPF enumeration processing.
*/
#define                 OPF_NONEWENUM	18
/* SET NOJOINOP NEW  - no opf_value required
** - disable new OPF enumeration processing.
*/
#define			OPF_HASH	19
/* SET HASH - no opf_value required (?)
** - enable hash join/aggregation strategies
*/
#define			OPF_NOHASH	20
/* SET NOHASH - no opf_value required (?)
** - disable hash join/aggregation strategies
*/
#define			OPF_PARALLEL	21
/* SET PARALLEL [n] - opf_value is degree of parallelism to set or -1
** - enable parallel query plans with dop n (or default dop if -1)
*/
#define			OPF_NOPARALLEL	22
/* SET NOPARALLEL - no opf_value required
** - disable parallel query plan generation
*/
#define			OPF_CARDCHK	23
/* SET CARDINALITY_CHECK - check for cardinality violations on
** scalar sub-queries.
*/
#define			OPF_NOCARDCHK	24
/* SET NOCARDINALITY_CHECK - legacy behaviour ignore cardinality
** violations on scalar sub-queries.
*/
#define			OPF_INLIST_THRESH 25
/* no set command defined yet - opf_value is required parameter
** - count of IN list elements before switching to range keys from EQ.
*/

    i4         opf_value;          /* alter value (if any)               */
    bool	    opf_autostruct;	/* defines server default for table
					** auto structure option */
/* I/O parameters for OPF_STARTUP - note ADF needs to be initialized!!        */
    i4              opf_version;        /* returns version number of OPF      */
    i4              opf_sesize;         /* returns size of session cntrl block*/
    i4		    opf_opscb_offset;	/* offset from SCF's SCB to
					** any session's OPS_CB,
					** input at OPF_STARTUP.
					*/

/* input parameter for OPF_BGN_SESSION */
    ADF_CB          *opf_adfcb;         /* ptr to ADF control block           */
    i4		    opf_udbid;          /* unique (infodb) db id number */
    PTR             opf_dbid;           /* DMF data base id for this session  */
    CS_SID          opf_sid;            /* session id for this session        */

/* I/O parameters for OPF_CREATE_QEP                                          */
    QSO_OBID        opf_query_tree;     /* QSF ID for query tree              */
    QSO_OBID        opf_qep;            /* QSF ID for the output QEP	      */
    f4              opf_cost;           /* measure of estimated cost for qep  */
    PTR             *opf_repeat;        /* ptr to array of ptrs to repeat
                                        ** query parameters, used as hints for
                                        ** optimization... NULL if not
                                        ** available */
    QSO_OBID        opf_parent_qep;	/* Input QSF ID for parent qep 
					** of this query.
					** Currently, this is only needed for
					** replace cursor commands. If there
					** is no parent qep, then this should
					** zero.
					*/
    PTR             opf_server;         /* ptr to server control block
                                        ** required by every call to OPF
                                        ** (except server startup in which
                                        ** it is returned by OPF) */
    bool            opf_rdf;            /* TRUE indicates that RDF should have
                                        ** a separate memory pool allocated for
                                        ** it, so that machines on which 
                                        ** shared memory is a scarce resource
                                        ** can be tuned 
                                        ** - FALSE will cause one pool to be
                                        ** allocated, which results in more
                                        ** efficient memory usage when no
                                        ** shared memory distinctions exist */
    i2		    opf_cnffact;	/* multiplier applied to # CNF fax
					** to determine if where is too complex
					*/
    i4		    opf_names;		/* TRUE if names SB added to SQLDAs   */
    i4		    opf_mxsess;         /* ACTIVE opf startup flag
					** - max number of sessions active within
                                        ** OPF at any one time; 0 indicates that 
					** OPF should use the default
					*/
    OPF_STATE	    opf_smask;          /* mask used to describe various
					** boolean values, defaults are selected
                                        ** by setting this mask to 0
					** - defines AGGREGATE_FLATTEN,
					** COMPLETE, */
    OPF_QEFTYPE	    opf_qeftype;        /* describes the capability of the QEF
                                        ** that the server has, needs to be defined
                                        ** on server startup only */
    i4		    opf_actsess;	/* SESSION_CONNECTED server startup flag
					** - number of currently active sessions
                                        ** used to help determine DMF caching
                                        ** characteristics for non-repeat queries
                                        */
#if 1
    PTR		    opf_coordinator;	/* ptr to LDB descriptor for coordinator
					** database, only required for session
					** startup, ptr will be saved in OPF's
                                        ** session control block */
    PTR		    opf_ddbsite;       /* this is the node name for the
					** distributed server, which may be
					** different from that in
					** opf_coordinator, this is the
					** translation of ii_ins_vnode using
					** NMgtat, this is only required on
					** server startup since it will be the
					** same for all sessions, ptr will be
					** saved in OPF's server control block */
#else
    DD_1LDB_INFO    *opf_coordinator;	/* ptr to LDB descriptor for coordinator
					** database, only required for session
					** startup, ptr will be saved in OPF's
                                        ** session control block */
    DD_NODE_NAME    *opf_ddbsite;       /* this is the node name for the
					** distributed server, which may be
					** different from that in
					** opf_coordinator, this is the
					** translation of ii_ins_vnode using
					** NMgtat, this is only required on
					** server startup since it will be the
					** same for all sessions, ptr will be
					** saved in OPF's server control block */
#endif
    SIZE_TYPE	    opf_mserver;	/* this is the amount of memory that OPF
					** should allocate on server startup time only
					** in the initial allocation, if the default
					** is to be used then a zero should be placed
					** here */
    PTR		    opf_thandle;	/* this is the handle for the QSF query
					** text object. It is passed to OPC to
					** make the query text available in
					** the error handling routines.
					*/
    double	    opf_cpufactor;	/* CPUFACTOR defined on server startup, this parameter 
					** is used to convert CPU units to DIO units.
					** If 0 then the default will be used */
    double	    opf_timeout;	/* TIMEOUTFACTOR defined on server startup, 
					** this parameter
					** is used to convert the cost units into
					** millisec's, if zero then the default
					** will be used */
    double	    opf_rfactor;	/* REPEATFACTOR defined on server startup, 
					** this parameter
					** is used to affect the timeout for repeat
					** query evaluation, if zero then the default
					** will be used */
    i4	    opf_sortmax;	/* SORTMAX defined on server startup,
					** optimizer will avoid plans in which the
					** estimated size of a sort would involve
					** data which is larger than this number, unless
					** the semantics require that a sort is done, if
					** NEGATIVE then no limit will be used */
    i4	    opf_qefsort;	/* QEFSORT, value of QEF sort size, no default assumed
					** since SCF specifies the default also */
    i4	    opf_qefhash;	/* buffer limit for hash joins */
    double	    opf_exact;		/* EXACTKEY startup flag, value between 0.0 and 1.0
					** which describes the selectivity of exact
					** matches, 0.0 means use the default (of 0.01)*/
    double	    opf_range;		/* RANGEKEY startup flag, value between 0.0 and 1.0
					** which describes the selectivity of inexact
					** matches, 0.0 means use the default (of 0.10)*/
    float           opf_nonkey;		/* NONKEY startup flag, value between 0.0 and 1.0
					** which describes the selectivity of non-keying
					** qualifications, 0.0 means use the default
					** (of 0.50) */
    float           opf_spatkey;	/* NONKEY startup flag, value between 0.0 and 1.0
					** which describes the selectivity of 
					** spatial predicate qualifications, 0.0 
					** means use the default (of 0.50) */
    u_i4	    *opf_dbxlate;	/* Case translation semantics flags
					** See <cuid.h> for masks.
					*/
    DB_OWN_NAME	    *opf_cat_owner;	/* Catalog owner name.  Will be
					** "$ingres" if regular ids are lower;
					** "$INGRES" if regular ids are upper.
					*/
    DB_OWN_NAME	    opf_default_user;	/* default user for compile (used in
					** temp table processing) */
    float	    opf_maxmemf;	/* Fraction of OPF memory pool avail
					** to each session. */
    i4	    opf_maxtup;		/* maximum tuple size for inst.  */
    i4	    opf_maxpage;	/* maximum page size for inst.   */
    i4	    opf_pagesize[DB_MAX_CACHE];	
					/* page sizes available in inst. */
    i4	    opf_tupsize[DB_MAX_CACHE];	
					/* tuple sizes available in inst.*/
    float           opf_joinop_timeout;
					/* default JOINOP TIMEOUT value */
    float           opf_joinop_timeoutabort;
					/* default JOINOP TIMEOUTABORT value */
    i4		    opf_rangep_max;	/* max range preds with RQ parms */
    i2		    opf_cstimeout;	/* # of opn_jintersect calls before 
					** opn_timeout is called - replaces 
					** OPN_CSTIMEOUT constant. */
    u_i2	    opf_inlist_thresh;	/* Count of IN LIST items before switching
					** to range keys from EQ. */
    f4		    opf_stats_nostats_factor;
    i4      opf_pq_dop;			/* degree of parallelism for 
					** parallel query compilation */
    f4	    opf_pq_threshold;		/* threshold plan cost before 
					** parallel plan is attempted */
    i4	    opf_pq_partthreads;		/* max threads for partitioned table/
					** join in parallel query */
    f4		    opf_greedy_factor;	/* adjust ops_cost/jcost comparison
					** in opn_corput() for greedy enum */
} OPF_CB;

/*}
** Name: OPX_ERROR - error code type
**
** Description:
**      Error code typedef of optimizer errors; should match the err_code
**      component of the DB_ERROR structure.
**
** History:
**      23-may-86 (seputis)
**          initial creation
**      10-jun-91 (seputis)
**          - added new error messages for bug 37172
**      4-feb-93 (ed)
**          - add new error message E_OP04AB
**	10-feb-93 (teresa)
**	    added E_OP0013_READ_TUPLES
**	10-mar-93 (barbara)
**	    added E_OP08A4_DELIMIT_FAIL
**      17-may-93 (ed)
**          - add new error message E_OP028C
**	8-may-94 (ed)
**	    - add OP130 to skip non-critical consistency checks
[@history_line@]...
*/
typedef i4 OPX_ERROR;
/*
**      Defines of error constants used in the optimizer
**      - least significant byte in range 0 to 127 is used for user errors
**      and is not logged but a message is sent to the user
**      - least significant byte in range 128 to 255 is used for internal
**      errors which are logged and an appropriate message is sent to the
**      user
**      - if the err_data field is non-zero then it be the error
**        code of the facility which returned an error to OPF, unless
**        otherwise indicated in the comments below
*/
#define                 OPE_BASE        (0x00040000L)
/* base of error codes used within the optimizer                              
*/

/*
** GENERAL ERRORS
*/
#define                 E_OP0000_OK             ((OPX_ERROR) OPE_BASE)
/* successful completion
*/
#define                 E_OP0001_USER_ERROR     ((OPX_ERROR) (OPE_BASE+1))
/* error returned to caller of OPF indicating that the query cannot be processed
** and all appropriate messages have been returned to the user and logged
** if appropriate
*/
#define                 E_OP0002_NOMEMORY       ((OPX_ERROR) (OPE_BASE+2))
/* optimizer ran out of non-enumeration memory
** - user error - not logged
*/
#define                 E_OP0003_ASYNCABORT     ((OPX_ERROR) (OPE_BASE+3))
/* asynchronous abort was detected 
** - user error - not logged
*/
#define                 E_OP0004_RDF_GETDESC     ((OPX_ERROR) (OPE_BASE+4))
/* RDF request for relation information failed - possibly because table
** was deleted
*/
#define                 E_OP0005_GRANGETABLE      ((OPX_ERROR) (OPE_BASE+5)) 
/* global range table overflow
*/
#define                 E_OP0006_TIMEOUT          ((OPX_ERROR) (OPE_BASE+6)) 
/* optimization timeout - suboptimal solution found
** - warning status returned
*/
#define                 E_OP0007_INVALID_OPCODE   ((OPX_ERROR) (OPE_BASE+7))
/* opf_call entry point received an invalid operation code
** - user error - not logged
*/
#define                 E_OP0008_NOEXECUTE        ((OPX_ERROR) (OPE_BASE+8))
/* Trace flag set indicating query should not be executed
** - user error - not logged
*/
#define                 E_OP0009_TUPLE_SIZE       ((OPX_ERROR) (OPE_BASE+9))
/* size of tuple for intermediate relation is too large, not possible
** to process this query.
*/
#define			E_OP000A_RETRY		((OPX_ERROR) (OPE_BASE+0xA))
/* this is an indication that SCF should retry the query with flattening
** turned off, much like the QEF retry mechanism, it is used for support
** of outer joins in queries like
** select * from (r left join s on s.a=r.a) 
**    where r.a = (select avg(t.a) from t where t.b=s.b or t.c =5)
** since flattening fails when the a NULL is produced by the outer join but
** no NULLs exist in attribute s.b
*/

#define                 E_OP000B_EXCEPTION	  ((OPX_ERROR) (OPE_BASE+0xB)) 
/* floating point or integer exception occurred during optimizer processing and
** a query plan could not be found 
*/
#define		    E_OP000C_DBPROC_NOT_FOUND	  ((OPX_ERROR) (OPE_BASE+0xC))
/* an attempt to read dbproc description failed because the dbproc no longer
** exists */
#define			E_OP000D_RETRY		  ((OPX_ERROR) (OPE_BASE+0xD))
/* used to indicate to SCF that an retry should be made since a possible
** deadlock situation has been detected; it does not imply flattening should
** be turned on or off */
#define                 E_OP000E_AGGTUP 	  ((OPX_ERROR) (OPE_BASE+0xE))
/* user has specified a target list for an aggregate which has a row width
** greater than the ingres row width and is too large to be sorted 
*/
#define                 E_OP000F_TEMP_SIZE	  ((OPX_ERROR) (OPE_BASE+0xF))
/* temporary file size limits exceeded for all query plans due to SORTMAX
** server startup parameter
*/
#define			E_OP0010_RDF_DEADLOCK     ((OPX_ERROR) (OPE_BASE+0x10))
/* Deadlock encountered when calling RDF for system catalog information
*/
#define			E_OP0011_TREE_PACK	  ((OPX_ERROR) (OPE_BASE+0x11))
/* error occurred in RDF while trying to pack a query tree - possibly because
** the tree was found to be inconsistent
*/
#define                 E_OP0012_TEXT_PACK	  ((OPX_ERROR) (OPE_BASE+0x12))
/* error occurred in RDF while trying to pack query text
*/
#define			E_OP0013_READ_TUPLES	  ((OPX_ERROR) (OPE_BASE+0x13))
/* error occurred in RDF while trying to read tuples from system catalogs
*/
#define                 E_OP0018_TIMEOUTABORT     ((OPX_ERROR) (OPE_BASE+0x18))
/* optimization timeout abort - join optimization timed out, query aborted 
** 
*/
#define                 E_OP0080_ULM_CLOSESTREAM ((OPX_ERROR) (OPE_BASE+0x80))
/* consistency check - error deallocating a memory stream 
*/
#define                 E_OP0081_NOSTATUS       ((OPX_ERROR) (OPE_BASE+0x81))
/* consistency check - no status provided by error handling mechanism 
*/
#define                 E_OP0082_UNEXPECTED_EX  ((OPX_ERROR) (OPE_BASE+0x82))
/* consistency check - unexpected exception occurred
*/
#define                 E_OP0083_UNEXPECTED_EXEX ((OPX_ERROR) (OPE_BASE+0x83)) 
/* consistency check - really bad, got an unexpected exception trying to
** recover from an exception
*/
#define                 E_OP0084_DEALLOCATION   ((OPX_ERROR) (OPE_BASE+0x84))
/* consistency check - resource deallocation error 
*/
#define                 E_OP0085_QSO_LOCK       ((OPX_ERROR) (OPE_BASE+0x85))
/* consistency check - query tree could not be retrieved from QSF 
*/
#define                 E_OP0086_ULM_STARTUP    ((OPX_ERROR) (OPE_BASE+0x86))
/* consistency check - server startup error - cannot startup ULM
*/
#define                 E_OP0087_RDF_INITIALIZE ((OPX_ERROR) (OPE_BASE+0x87))
/* consistency check - server startup error - cannot startup RDF
*/
#define                 E_OP0088_ULM_SHUTDOWN   ((OPX_ERROR) (OPE_BASE+0x88))
/* consistency check - server shutdown error - cannot shutdown ULM
*/
#define                 E_OP0089_ALTER          ((OPX_ERROR) (OPE_BASE+0x89))
/* consistency check - invalid alter opcode
*/
#define                 E_OP008A_SCF_SERVER_MEM ((OPX_ERROR) (OPE_BASE+0x8A))
/* error obtaining memory for OPF server control block from SCF
*/
#define                 E_OP008B_SCF_SERVER_MEM ((OPX_ERROR) (OPE_BASE+0x8B))
/* error releasing memory for OPF server control block to SCF
*/
#define                 E_OP008C_SEMAPHORE      ((OPX_ERROR) (OPE_BASE+0x8C))
/* could not initialize semaphore
*/
#define                 E_OP008D_RDF_UNFIX      ((OPX_ERROR) (OPE_BASE+0x8D))
/* RDF could not unfix a fixed table
*/
#define                 E_OP008E_RDF_INVALIDATE ((OPX_ERROR) (OPE_BASE+0x8E))
/* RDF could not invalidate a table
*/
#define                 E_OP008F_RDF_MISMATCH   ((OPX_ERROR) (OPE_BASE+0x8F))
/* The RDF cache could not be synchronized with the parser time stamp, so that
** SCF should reparse the query in an attempt to update the parser's range
** table
*/
#define                 E_OP0090_PAINE          ((OPX_ERROR) (OPE_BASE+0x90))
/* PAINE handling routine could not be initialized by OPF 
*/
#define                 E_OP0091_AIC            ((OPX_ERROR) (OPE_BASE+0x91))
/* PAINE handling routine could not be initialized by OPF 
*/
#define                 E_OP0092_SEMWAIT        ((OPX_ERROR) (OPE_BASE+0x92))
/* semaphore wait error for optimizer
*/
#define                 E_OP0093_ULM_ERROR      ((OPX_ERROR) (OPE_BASE+0x93))
/* error obtaining memory from memory manager
*/
#define                 E_OP0094_RDF_SHUTDOWN   ((OPX_ERROR) (OPE_BASE+0x94))
/* error shutting down RDF
*/
#define                 E_OP0095_PARSE_TREE     ((OPX_ERROR) (OPE_BASE+0x95))
/* consistency check - invalid parse tree
**    - uninitialized duplicate handling field
*/
#define                 E_OP0096_INSF_OPFMEM    ((OPX_ERROR) (OPE_BASE+0x96))
/*
**    - insufficient memory to start OPF
*/
#define                 E_OP0097_CONDITION	((OPX_ERROR) (OPE_BASE+0x97))
/* consistency check - error calling CS condition, due to OPF ACTIVE count
*/
#define                 E_OP0098_CONDITION_INIT	((OPX_ERROR) (OPE_BASE+0x98))
/* consistency check - error initializing CS condition
*/
#define                 E_OP0099_CONDITION_FREE	((OPX_ERROR) (OPE_BASE+0x99))
/* consistency check - error on CS condition shutdown
*/
#define               E_OP009A_INCONSISTENT_TAB ((OPX_ERROR) (OPE_BASE+0x9A))
/* table explicitly included in range table has inconsistency flag
*/
#define                 E_OP009B_STACK_OVERFLOW   ((OPX_ERROR) (OPE_BASE+0x9B))
/* stack overflow detected */
#define			E_OP009C_RDF_COPY	((OPX_ERROR)(OPE_BASE+0x9C))
/* Partition definition copy call failed */
#define                 E_OP009D_MISC_INTERNAL_ERROR ((OPX_ERROR) (OPE_BASE+0x9D))


/*
** AGGREGATE PROCESSING PHASE ERROR
*/
#define                 E_OP0200_TUPLEOVERFLOW  ((OPX_ERROR) (OPE_BASE+0x200))
/* the intermediate aggregate relation required a tuple size overflow
** greater than DB_MAXTUP
** - user error - not logged
*/
#define                 E_OP0201_ATTROVERFLOW   ((OPX_ERROR) (OPE_BASE+0x201))
/* aggregate processing attempted to allocate an attribute in an intermediate
** relation which caused the number of allocated attributes to exceed the
** limit DB_MAX_COLS number of attributes per relation
** - user error - not logged
*/
#define                 E_OP0202_VAR_OVERFLOW   ((OPX_ERROR) (OPE_BASE+0x202))
/* query too complicated, too many tables referenced, subselects, or
** aggregates specified
*/
#define                 E_OP0280_SCOPE           ((OPX_ERROR) (OPE_BASE+0x280))
/* consistency check - scoping rules for variables used in query were violated
*/
#define                 E_OP0281_SUBQUERY        ((OPX_ERROR) (OPE_BASE+0x281))
/* consistency check - unexpected subquery type
*/
#define                 E_OP0282_FAGG		 ((OPX_ERROR) (OPE_BASE+0x282))
/* consistency check - inconsistent function aggregate descriptor
*/
#define                 E_OP0283_PROJECT	 ((OPX_ERROR) (OPE_BASE+0x283))
/* consistency check - unexpected aggregate projections request
*/
#define			E_OP0284_RESDOM		 ((OPX_ERROR) (OPE_BASE+0x284))
/* consistency check - resdom list not monotonically decreasing 
*/
#define                 E_OP0285_UNIONAGG	 ((OPX_ERROR) (OPE_BASE+0x285)) 
/* consistency check - union or aggregate unexpected in this context */
#define                 E_OP0286_UNEXPECTED_VAR	 ((OPX_ERROR) (OPE_BASE+0x286)) 
/* consistency check - variable could not be mapped when expected */
#define                 E_OP0287_OJVARIABLE	 ((OPX_ERROR) (OPE_BASE+0x287))
/* consistency check - outer join variables not placed as expected */
#define                 E_OP0288_OJAGG  	 ((OPX_ERROR) (OPE_BASE+0x288))
/* consistency check - outer join found using aggregate result when not expected */
#define                 E_OP0289_OJNESTING	 ((OPX_ERROR) (OPE_BASE+0x289))  
/* consistency check - unexpected OJ nesting with outer TID joins */
#define                 E_OP028A_MISSING_CORELATION ((OPX_ERROR) (OPE_BASE+0x28A))
/* consistency check - missing corelation attribute, expecting equi-join attribute */
#define                 E_OP028B_VAR_ELIMINATION ((OPX_ERROR) (OPE_BASE+0x28B))
/* consistency check - expecting variable to be eliminated after substitution */
#define                 E_OP028C_SELFJOIN	 ((OPX_ERROR) (OPE_BASE+0x28C))
/* consistency check - unexpected self join aggregate discovered */
#define			E_OP028D_PARMCYCLE	 ((OPX_ERROR) (OPE_BASE+0x28D))
/* parameter dependency cycle detected among table procedures */

/*
** JOINOP PROCESSING PHASE
*/
#define                 E_OP0300_ATTRIBUTE_OVERFLOW ((OPX_ERROR)(OPE_BASE+0x300))
/* the joinop attributes array overflowed - query too complex
** - user error not logged
*/
#define                 E_OP0301_EQCLS_OVERFLOW ((OPX_ERROR) (OPE_BASE+0x301))
/* equivalence class array overflowed - query too complex
*/
#define                 E_OP0302_BOOLFACT_OVERFLOW ((OPX_ERROR) (OPE_BASE+0x302))
/* too many boolean factors defined in query - query too complex 
*/
#define                 E_OP0303_RTOVERFLOW     ((OPX_ERROR) (OPE_BASE+0x387))
/* joinop range table does not have sufficient room for variables - query
** is too complex - should not happen normally 
** since table is larger than parser table
** but can happen if aggregate temporaries are created
*/
#define                 E_OP0304_FUNCATTR_OVERFLOW ((OPX_ERROR) (OPE_BASE+0x304))
/* function attribute table overflowed - query too complex
*/
#define                 E_OP0305_NOCONVERSION   ((OPX_ERROR) (OPE_BASE+0x305))
/* cannot convert constant to key - warning keyed access cannot be used 
*/
#define                 E_OP0380_JNCLAUS        ((OPX_ERROR) (OPE_BASE+0x380))
/* Consistency check - left and right part of conditional were
** unexpectedly the same var, OR the left and right part had mismatched
** types
*/
#define                 E_OP0381_FUNCATTR       ((OPX_ERROR) (OPE_BASE+0x381))
/* consistency check - inconsistent typing error in function attributes 
*/
#define                 E_OP0382_FATTR_CREATE   ((OPX_ERROR) (OPE_BASE+0x382))
/* consistency check - function attribute should have been created 
*/
#define                 E_OP0383_TYPEMISMATCH   ((OPX_ERROR) (OPE_BASE+0x383))
/* consistency check - unexpected type mismatch in equivalence class
*/
#define                 E_OP0384_NOATTS         ((OPX_ERROR) (OPE_BASE+0x384))
/* consistency check - no attribute in equivalence class 
*/
#define                 E_OP0385_EQCLS_MERGE    ((OPX_ERROR) (OPE_BASE+0x385))
/* consistency check - more than one equivalence class in merge 
*/
#define                 E_OP0386_BFCREATE       ((OPX_ERROR) (OPE_BASE+0x386))
/* consistency check - assumption about ordering of boolean factor list
** failed 
*/
#define                 E_OP0387_VARNO           ((OPX_ERROR) (OPE_BASE+0x387))
/* consistency check - varno from parser range table out of range
*/
#define                 E_OP0388_VARBITMAP        ((OPX_ERROR) (OPE_BASE+0x388)) 
/* consistency check - var bit map is invalid
*/
#define                 E_OP0389_EQUIJOIN         ((OPX_ERROR) (OPE_BASE+0x389)) 
/* consistency check - equi join , unexpected types for operands
*/
#define                 E_OP038A_REPEAT          ((OPX_ERROR) (OPE_BASE+0x38A)) 
/* consistency check - repeat query parameter not available */
#define                 E_OP038B_NO_SUBSELECT    ((OPX_ERROR) (OPE_BASE+0x38B)) 
/* consistency check - cannot find subselect for corelated variable */
#define                 E_OP038C_UNION           ((OPX_ERROR) (OPE_BASE+0x38C)) 
/* consistency check - union subquery expected */
#define                 E_OP038D_CORRELATED	 ((OPX_ERROR) (OPE_BASE+0x38D)) 
/* consistency check - correlated aggregate expected */
#define                 E_OP038E_MAXOUTERJOIN	 ((OPX_ERROR) (OPE_BASE+0x38E)) 
/* consistency check - outer join count for subquery exceeded */
#define                 E_OP038F_OUTERJOINSCOPE	 ((OPX_ERROR) (OPE_BASE+0x38F)) 
/* consistency check - same outer join referenced in 2 subqueries */
#define                 E_OP0390_ATTRIBUTE	 ((OPX_ERROR) (OPE_BASE+0x390)) 
/* consistency check - attribute not found when expected */
#define                 E_OP0391_HISTLEN	 ((OPX_ERROR) (OPE_BASE+0x391)) 
/* consistency check - unexpected histogram length */
#define                 E_OP0392_HISTINCREASING	 ((OPX_ERROR) (OPE_BASE+0x392)) 
/* consistency check - histogram read from statistics catalogs is not monotonically
** increasing */
#define                 E_OP0393_HISTOUTOFRANGE	 ((OPX_ERROR) (OPE_BASE+0x393)) 
/* consistency check - histogram read from statistics catalogs has selectivity
** value out of range 0.0 <= selectivity <= 1.0 */
#define                 E_OP0394_NOATTR_DEFINED ((OPX_ERROR) (OPE_BASE+0x394)) 
/* consistency check - an attribute should be defined for the outer join at this
** point */
#define                 E_OP0395_PSF_JOINID	((OPX_ERROR) (OPE_BASE+0x395)) 
/* consistency check - joinid provided by PSF is out of range of query tree header
** join count */
#define                 E_OP0396_DUPLICATE_OJID	((OPX_ERROR) (OPE_BASE+0x396)) 
/* consistency check - attempt to evaluate 2 joinid's at the same node */
#define                 E_OP0397_MISPLACED_OJ	((OPX_ERROR) (OPE_BASE+0x397)) 
/* consistency check - attempt to evaluate outer join at unsupported node */
#define                 E_OP0398_OJ_SCOPE	((OPX_ERROR) (OPE_BASE+0x398)) 
/* consistency check - cannot find outer join associated with view */
#define                 E_OP0399_FJ_VARSETS	((OPX_ERROR) (OPE_BASE+0x399)) 
/* consistency check - full join does not create 2 variable sets */
#define                 E_OP039A_OJ_TYPE	((OPX_ERROR) (OPE_BASE+0x39A)) 
/* consistency check - unexpected outer join type */
#define                 E_OP039B_OJ_LEFTRIGHT	((OPX_ERROR) (OPE_BASE+0x39B)) 
/* consistency check - cannot resolve between left or right outer join types */
#define                 E_OP039C_OJ_HIST	((OPX_ERROR) (OPE_BASE+0x39C)) 
/* consistency check - mismatched histograms in outer join */
#define                 E_OP039D_OJ_COVAR	((OPX_ERROR) (OPE_BASE+0x39D)) 
/* consistency check - unexpected correlated variable expression */
#define                 E_OP039E_EXPECTING_VAR	 ((OPX_ERROR) (OPE_BASE+0x39E)) 
/* consistency check - there must be at least one inner for an outer join */
#define                 E_OP039F_BOOLFACT	 ((OPX_ERROR) (OPE_BASE+0x39F)) 
/* consistency check - all elements of boolean factor do not have same join id */
#define                 E_OP03A0_OJBITMAP	 ((OPX_ERROR) (OPE_BASE+0x3A0)) 
/* consistency check - sum of maps must equal parent */
#define                 E_OP03A1_OJMAP		((OPX_ERROR) (OPE_BASE+0x3A1)) 
/* consistency check - outer join variable map is not a subset of parent */
#define                 E_OP03A2_HISTINCREASING  ((OPX_ERROR) (OPE_BASE+0x3A2))
/* consistency check - histogram read from statistics catalogs is not monotonically
** increasing , with parameters*/
#define                 E_OP03A3_FLATAGGVAR	 ((OPX_ERROR) (OPE_BASE+0x3A3))
/* consistency check - variable expected when flattening aggregate */
#define                 E_OP03A4_SUBQUERY	 ((OPX_ERROR) (OPE_BASE+0x3A4))
/* consistency check - missing partition in union */
#define                 E_OP03A5_KEYINCELL	 ((OPX_ERROR) (OPE_BASE+0x3A5))
/* consistency check - key falls in histo cell but not equal to boundaries */
#define                 E_OP03A6_OUTER_JOIN	 ((OPX_ERROR) (OPE_BASE+0x3A6)) 
/* consistency check - internal processing error related to outer joins */
#define                 E_OP03A7_OJ_SUBSELECT	 ((OPX_ERROR) (OPE_BASE+0x3A7)) 
/* consistency check - internal error, multiple subselect not outer join compatible */

/*
** ENUMERATION ERRORS
*/
#define                 E_OP0400_MEMORY          ((OPX_ERROR) (OPE_BASE+0x400))
/* ran out of enumeration memory
** - user error not logged
*/
#define                 E_OP0401_SEARCHSPACE     ((OPX_ERROR) (OPE_BASE+0x401))
/* used to alter the traversal of the search space, it should not be
** returned to the caller, it is only used for internal OPF purposes */
# if 0
#define                 E_OP0480_MISSING_TEMP_HISTO ((OPX_ERROR) (OPE_BASE+0x480))
/* consistency check - histogram from temporary relation expected
** FIXME - not implemented yet so turn off check
*/
#endif
#define                 E_OP0481_KEYINFO        ((OPX_ERROR) (OPE_BASE+0x481))
/* Consistency check - cannot find OPB_BFKEYINFO pointer when expected
*/
#define                 E_OP0482_HISTOGRAM      ((OPX_ERROR) (OPE_BASE+0x482))
/* Consistency check - cannot find OPH_HISTOGRAM structure for equivalence class
*/
#define                 E_OP0483_HIST_BUFFER    ((OPX_ERROR) (OPE_BASE+0x483))
/* Consistency check - overflow of temporary histogram buffer 
*/
#define                 E_OP0484_HISTTYPE       ((OPX_ERROR) (OPE_BASE+0x484))
/* Consistency check - unexpected histogram type 
*/
#define                 E_OP0485_HISTAND        ((OPX_ERROR) (OPE_BASE+0x485))
/* consistency check - attempt to AND two incompatible histograms 
*/
#define                 E_OP0486_NOKEY          ((OPX_ERROR) (OPE_BASE+0x486))
/* consistency check - no key found when one expected 
*/
#define                 E_OP0487_NOEQCLS        ((OPX_ERROR) (OPE_BASE+0x487))
/* consistency check - no joining equivalence class found when expected 
*/
#define                 E_OP0488_JOINNOEQCLS    ((OPX_ERROR) (OPE_BASE+0x488))
/* consistency check - no joining equivalence class found when expected 
*/
#define                 E_OP0489_NOFUNCHIST     ((OPX_ERROR) (OPE_BASE+0x489))
/* consistency check - no histogram found for function attribute when expected 
*/
#define                 E_OP048A_NOCARTPROD     ((OPX_ERROR) (OPE_BASE+0x48A))
/* consistency check - no joining equivalence class and there is not a
** cartesean product 
*/
#define                 E_OP048B_COST           ((OPX_ERROR) (OPE_BASE+0x48B))
/* consistency check - cpu or disk i/o cost is negative 
*/
#define                 E_OP048C_SORTCOST       ((OPX_ERROR) (OPE_BASE+0x48C))
/* consistency check - unexpected DMF sort cost error */
#define                 E_OP048D_QUERYTYPE      ((OPX_ERROR) (OPE_BASE+0x48D))
/* consistency check - unknown subquery type
*/
#define                 E_OP048E_COTYPE         ((OPX_ERROR) (OPE_BASE+0x48E))
/* consistency check - unknown CO node type
*/
#define                 E_OP048F_NULL           ((OPX_ERROR) (OPE_BASE+0x48F))
/* consistency check - IS NULL or IS NOT NULL operator ID expected
*/
#define                 E_OP0490_CSSTATISTICS   ((OPX_ERROR) (OPE_BASE+0x490))
/* consistency check - CSstatistics called failed, so enumeration phase cannot
** be bounded
*/
#define                 E_OP0491_NO_QEP		((OPX_ERROR) (OPE_BASE+0x491))
/* consistency check - no query plan was found */
#define                 E_OP0492_ORIG_NODE      ((OPX_ERROR) (OPE_BASE+0x492))
/* consistency check - orig node expected on end of CO list
*/
#define                 E_OP0493_MULTIATTR_SORT ((OPX_ERROR) (OPE_BASE+0x493))
/* consistency check - invalid multi-attribute sort type
*/
#define                 E_OP0494_ORDERING       ((OPX_ERROR) (OPE_BASE+0x494))
/* consistency check - unexpected ordering present 
*/
#define                 E_OP0495_CO_TYPE	((OPX_ERROR) (OPE_BASE+0x495))
/* consistency check - unexpected CO node type
*/
#define                 E_OP0496_EXACT		((OPX_ERROR) (OPE_BASE+0x496))
/* consistency check - exact multi-attribute ordering expected
*/
#define                 E_OP0497_INCOMPATIBLE	((OPX_ERROR) (OPE_BASE+0x497))
/* consistency check - incompatible orderings found in CO node
*/
#define                 E_OP0498_NO_EQCLS_FOUND	((OPX_ERROR) (OPE_BASE+0x498))
/* consistency check - equivalence class not found when expected
*/
#define                 E_OP0499_NO_CO		((OPX_ERROR) (OPE_BASE+0x499))
/* consistency check - no CO is available out of enumeration memory as expected
*/
#define                 E_OP049A_CSALTR_SESSION	((OPX_ERROR) (OPE_BASE+0x49A))
/* consistency check - could not turn on CPU account via CSaltr_session
*/
#define                 E_OP049B_SORT	((OPX_ERROR) (OPE_BASE+0x49B))
/* consistency check - unexpected sort node found
*/
#define                 E_OP049C_INEXACT ((OPX_ERROR) (OPE_BASE+0x49C))
/* consistency check - inexact ordering expected
*/
#define                 E_OP049D_ON	((OPX_ERROR) (OPE_BASE+0x49D))
/* consistency check - estimate for outer join ON clause incorrect
*/
#define                 E_OP049E_FLOAT_UNDERFLOW ((OPX_ERROR) (OPE_BASE+0x49E))
/* consistency check - floating point underflow
*/
#define                 E_OP049F_FLOAT_DIVIDE	((OPX_ERROR) (OPE_BASE+0x49F))
/* consistency check - floating point divide by zero
*/
#define                 E_OP04A0_FLOAT_OVERFLOW ((OPX_ERROR) (OPE_BASE+0x4A0))
/* consistency check - floating point overflow
*/
#define                 E_OP04A1_INT_OVERFLOW	((OPX_ERROR) (OPE_BASE+0x4A1))
/* consistency check - integer overflow
*/
#define                 E_OP04A2_INT_DIVIDE	((OPX_ERROR) (OPE_BASE+0x4A2))
/* consistency check - integer divide by zero
*/
#define                 E_OP04A3_ATTR_FOUND	((OPX_ERROR) (OPE_BASE+0x4A3))
/* consistency check - keyed attribute not found
*/
#define                 E_OP04A4_ONLY_ONE	((OPX_ERROR) (OPE_BASE+0x4A4))
/* consistency check - only one histogram expected
*/
#define                 E_OP04A5_SHAPE          ((OPX_ERROR) (OPE_BASE+0x4A5))
/* consistency check - unexpected query plan shape
*/
#define                 E_OP04A5_SHAPE          ((OPX_ERROR) (OPE_BASE+0x4A5))
/* consistency check - unexpected query plan shape
*/
#define                 E_OP04A6_SECINDEX       ((OPX_ERROR) (OPE_BASE+0x4A6))
/* consistency check - expected to find secondary index in tree pruning
** algorithm */
#define                 E_OP04A7_CONODE         ((OPX_ERROR) (OPE_BASE+0x4A7))
/* consistency check - no co node found as expected */
#define                 E_OP04A8_USAGEOVFL      ((OPX_ERROR) (OPE_BASE+0x4A8))
/* consistency check - internal data structure usage count overflow */
#define                 E_OP04A9_OJEST		((OPX_ERROR) (OPE_BASE+0x4A9))
/* consistency check - unexpected tuple estimate in outer join */
#define                 E_OP04AA_HISTOMOD	((OPX_ERROR) (OPE_BASE+0x4AA))
/* Histogram modification failed. Original histogram will be used. */
#define                 E_OP04AB_NOPRIMARY      ((OPX_ERROR) (OPE_BASE+0x4AB))
/* consistency check - no primary variable available */
#define                 E_OP04AC_JOINID		((OPX_ERROR) (OPE_BASE+0x4AC))
/* consistency check - unexpected outer join type found */
#define                 E_OP04AD_CORRELATED	((OPX_ERROR) (OPE_BASE+0x4AD))
/* consistency check - correlated outer join attribute inconsistency */
#define                 E_OP04AE_OBSOLETE	((OPX_ERROR) (OPE_BASE+0x4AE))
/* consistency check - code removed which should not be executed, so replace
** with a consistency check */
#define                 E_OP04AF_JOINEQC	((OPX_ERROR) (OPE_BASE+0x4AF))
/* consistency check - unexpected joining attributes found */
#define                 E_OP04B0_NESTEDOUTERJOIN ((OPX_ERROR) (OPE_BASE+0x4B0))
/* consistency check - unexpected internal nested outer join problem detected */
#define                 E_OP04B1_AGGUV		((OPX_ERROR) (OPE_BASE+0x4B1))
/* consistency check - aggregate union view optimization internal error */
#define                 E_OP04B2_TUPLEID	((OPX_ERROR) (OPE_BASE+0x4B2))
/* consistency check - internal error creating tuple id for outer join 
** processing */
#define                 E_OP04B3_HISTOUTERJOIN	((OPX_ERROR) (OPE_BASE+0x4B3))
/* consistency check - internal error processing outer join histograms */
#define                 E_OP04B4_PLACEOUTERJOIN	((OPX_ERROR) (OPE_BASE+0x4B4))
/* consistency check - internal error placing outer joins in query plan */
#define                 E_OP04B5_FILTEREOUTERJOIN ((OPX_ERROR) (OPE_BASE+0x4B5))
/* consistency check - internal error placing outer joins filters*/
#define                 E_OP04B6_SPECIALOUTERJOIN ((OPX_ERROR) (OPE_BASE+0x4B6))
/* consistency check - internal error placing defining outer join special attributes */
#define                 E_OP04C0_NEWENUMFAILURE ((OPX_ERROR) (OPE_BASE+0x4C0))
/* new numeration technique failed to find a valid plan - reverts to old technique */
#define                 E_OP04C1_GREEDYCONSISTENCY ((OPX_ERROR) (OPE_BASE+0x4C1))
/* greedy enumeration found plan that doesn't cover all tables - consistency check */

/*
** QUERY TREE INCONSISTENCIES
*/
#define                 E_OP0680_TARGETLIST     ((OPX_ERROR) (OPE_BASE+0x680))
/* consistency check - target list has non PST_RESDOM nodes in it
*/
#define                 E_OP0681_UNKNOWN_NODE   ((OPX_ERROR) (OPE_BASE+0x681))
/* consistency check - unknown query tree node type found
*/
#define                 E_OP0682_UNEXPECTED_NODE ((OPX_ERROR) (OPE_BASE+0x682))
/* consistency check - unexpected query tree node - PST_CURVAL
*/
#define                 E_OP0683_DUMPQUERYTREE   ((OPX_ERROR) (OPE_BASE+0x683))
/* consistency check - error dumping query tree 
*/
#define                 E_OP0684_DUMPQUERYNODE   ((OPX_ERROR) (OPE_BASE+0x684)) 
/* consistency check - error dumping query tree node 
*/
#define                 E_OP0685_RESOLVE         ((OPX_ERROR) (OPE_BASE+0x685)) 
/* consistency check - error resolving query tree node 
*/
#define                 E_OP0686_CQMODE         ((OPX_ERROR) (OPE_BASE+0x686))
/* consistency check - Illegal query mode for compilation.
*/
#define                 E_OP0687_NOQTREE         ((OPX_ERROR) (OPE_BASE+0x687))
/* consistency check - No query tree when expected.
*/
#define                 E_OP0687_NOQTREE         ((OPX_ERROR) (OPE_BASE+0x687))
/* consistency check - No query tree when expected.
*/
#define                 E_OP0688_INEXACT	((OPX_ERROR) (OPE_BASE+0x688))
/* consistency check - expecting inexact ordering nesting in an exact
** ordering
*/
#define                 E_OP0689_BAD_TREE        ((OPX_ERROR) (OPE_BASE+0x689))
/* consistency check - bad query tree node found when pushing NOTs down tree
** during normalization process */
#define                 E_OP068A_BAD_TREE        ((OPX_ERROR) (OPE_BASE+0x68A))
/* error converting query tree to text */
#define                 E_OP068B_BAD_VAR_NODE	 ((OPX_ERROR) (OPE_BASE+0x68B))
/* error converting query tree to text */
#define			E_OP068C_BAD_TARGET_LIST ((OPX_ERROR) (OPE_BASE+0x68C))
/* A bad select (or subselect) target list has been found */
#define			E_OP068D_QTREE		 ((OPX_ERROR) (OPE_BASE+0x68D))
/* error copying query tree nodes */
#define                 E_OP068E_NODE_TYPE	 ((OPX_ERROR) (OPE_BASE+0x68E))
/* unexpected node type */
#define                 E_OP068F_MISSING_RESDOM	 ((OPX_ERROR) (OPE_BASE+0x68F))
/* consistency check - particular resdom number expected but not found */

/*
** ADF ERRORS
*/
#define                 E_OP0700_ADC_CVINTO     ((OPX_ERROR) (OPE_BASE+0x700))
/* cannot build key because conversion does not exist
** - user error not logged
*/
#define                 E_OP0701_ADF_EXCEPTION  ((OPX_ERROR) (OPE_BASE+0x701))
/* ADF exception - see error block for info, ADF exception warning
*/
#define			E_OP0702_ADF_EXCEPTION  ((OPX_ERROR) (OPE_BASE+0x702))
/* ADF exception - user error number between 120 and 16F returned in error block
*/
#define                 E_OP0780_ADF_HISTOGRAM  ((OPX_ERROR) (OPE_BASE+0x780))
/* consistency check - the ADF returned an unexpected error when retrieving 
** the histogram type and length (adc_hg_dtln)
*/
#define                 E_OP0781_ADI_FIDESC     ((OPX_ERROR) (OPE_BASE+0x781))
/* consistency check - error calling adi_fidesc 
*/
#define                 E_OP0782_ADI_CALCLEN    ((OPX_ERROR) (OPE_BASE+0x782))
/* consistency check - error calling adi_calclen 
*/
#define                 E_OP0783_ADI_FICOERCE       ((OPX_ERROR) (OPE_BASE+0x783))
/* consistency check - error calling adi_ficoerce
*/
#define                 E_OP0784_ADC_KEYBLD     ((OPX_ERROR) (OPE_BASE+0x784))
/* consistency check - error calling adc_keybld
*/
#define                 E_OP0785_ADC_HELEM      ((OPX_ERROR) (OPE_BASE+0x785))
/* consistency check - error calling adc_helem
*/
#define                 E_OP0786_ADC_DHMAX      ((OPX_ERROR) (OPE_BASE+0x786))
/* consistency check - default upper bound for uniform histogram error 
** from ADF 
*/
#define                 E_OP0787_ADC_DHMIN      ((OPX_ERROR) (OPE_BASE+0x787))
/* consistency check - default lower bound for uniform histogram error 
** from ADF 
*/
#define                 E_OP0788_ADC_CVINTO     ((OPX_ERROR) (OPE_BASE+0x788))
/* consistency check - conversion of histogram element unexpectedly failed
*/
#define                 E_OP0789_ADC_HMIN       ((OPX_ERROR) (OPE_BASE+0x789))
/* consistency check - error calling min histogram value function routine
*/
#define                 E_OP078A_ADC_HMAX       ((OPX_ERROR) (OPE_BASE+0x78A))
/* consistency check - error calling max histogram value function routine
*/
#define                 E_OP078B_ADC_HDEC	((OPX_ERROR) (OPE_BASE+0x78B))
/* consistency check - ADF histogram decrement routine failed
*/
#define                 E_OP078C_ADC_COMPARE    ((OPX_ERROR) (OPE_BASE+0x78C))
/* consistency check - ADF compare routine failed 
*/
#define                 E_OP078D_ADI_OPID       ((OPX_ERROR) (OPE_BASE+0x78D))
/* consistency check - ADF operator ID routine failed in startup
*/
#define			E_OP078E_ADE_CXSPACE	((OPX_ERROR) (OPE_BASE+0x78E))
/* Error estimating the size of a CX.
*/
#define			E_OP078F_ADE_BGNCOMP	((OPX_ERROR) (OPE_BASE+0x78F))
/* Error beginning a compilation.
*/
#define			E_OP0790_ADE_CONSTGEN	((OPX_ERROR) (OPE_BASE+0x790))
/* Error generating a constant.
*/
#define			E_OP0791_ADE_INSTRGEN	((OPX_ERROR) (OPE_BASE+0x791))
/* Error generating an instruction.
*/
#define			E_OP0792_ADE_INFORMSP	((OPX_ERROR) (OPE_BASE+0x792))
/* Error informing ADF of a larger CX.
*/
#define			E_OP0793_ADE_ENDCOMP	((OPX_ERROR) (OPE_BASE+0x793))
/* Error ending a compilation.
*/
#define			E_OP0794_ADI_OPNAME     ((OPX_ERROR) (OPE_BASE+0x794))
/* cannot get operator name from operator ID
*/
#define                 E_OP0795_ADF_EXCEPTION  ((OPX_ERROR) (OPE_BASE+0x795))
/* ADF exception - non-user error, ADF error return code returned in error block
*/
#define                 E_OP0796_NO_COMPLEMENT  ((OPX_ERROR) (OPE_BASE+0x796))
/* no complement exists for function when one was expected */
#define                 E_OP0797_OP_USE		((OPX_ERROR) (OPE_BASE+0x797))
/* unexpected operator construction PREFIX, POSTFIX, or INFIX mismatch */
#define                 E_OP0798_OP_TYPE	((OPX_ERROR) (OPE_BASE+0x798))
/* unexpected operator type */
#define                 E_OP0799_LENGTH		((OPX_ERROR) (OPE_BASE+0x799))
/* unexpected length for instrinsic datatype */
#define                 E_OP079A_OPID           ((OPX_ERROR) (OPE_BASE+0x79A))
/* unexpected operator id in algebraic manipulation */
#define                 E_OP079B_ADI_RESOLVE    ((OPX_ERROR) (OPE_BASE+0x79B))
/* could not resolve datatypes */

/*
**  OPC (OPF Compilation) Errors
*/
#define			E_OP0800_NODEFAULT	((OPX_ERROR) (OPE_BASE+0x800))
/* The user has not specified all of the required columns for insertion.
*/
#define			E_OP0880_NOT_READY	((OPX_ERROR) (OPE_BASE+0x880))
/* This feature or section of code is not yet implemented.
*/
#define			E_OP0881_DMUFUNC	((OPX_ERROR) (OPE_BASE+0x881))
/* Illegal DMU function type for compilation.
*/
#define			E_OP0882_QSF_CREATE	((OPX_ERROR) (OPE_BASE+0x882))
/* Error calling QSF to create a new object.
*/
#define			E_OP0883_QSF_SETROOT	((OPX_ERROR) (OPE_BASE+0x883))
/* Error calling QSF to set the root of an object.
*/
#define			E_OP0884_RELEQC		((OPX_ERROR) (OPE_BASE+0x884))
/* Consistency error: Orig node returns an eqc that is not found in the
**	originated relation.
*/
#define			E_OP0885_COTYPE		((OPX_ERROR) (OPE_BASE+0x885))
/* Consistency error: Illegal CO node type.
*/
#define			E_OP0886_QSF_PALLOC	((OPX_ERROR) (OPE_BASE+0x886))
/* Error calling QSF to allocate a piece of memory.
*/
#define			E_OP0887_CONSTTYPE	((OPX_ERROR) (OPE_BASE+0x887))
/* Illegal datatype or length for a constant in a query tree.
*/
#define			E_OP0888_ATT_NOT_HERE	((OPX_ERROR) (OPE_BASE+0x888))
/* Attribute is not available at a CO node.
*/
#define			E_OP0889_EQ_NOT_HERE	((OPX_ERROR) (OPE_BASE+0x889))
/* Eqc is not available at a CO node.
*/
#define			E_OP088A_EST_TUPLES	((OPX_ERROR) (OPE_BASE+0x88A))
/* Estimated number of tuples returned from an orig node is equal to MAXI4.
*/
#define			E_OP088B_BAD_NODE	((OPX_ERROR) (OPE_BASE+0x88B))
/* Keyset scrollable cursor should have PST_VAR node but doesn't.
*/
#define			E_OP0890_MAX_BASE	((OPX_ERROR) (OPE_BASE+0x890))
/* Too many bases added to a QEN_ADF struct.
*/
#define			E_OP0891_MAX_ROWS	((OPX_ERROR) (OPE_BASE+0x891))
/* Too large a row number used to add a base to a QEN_ADF struct.
*/
#define			E_OP0892_MAX_REPEATS	((OPX_ERROR) (OPE_BASE+0x892))
/* Too large a repeat query number used to add a base to a QEN_ADF struct.
*/
#define			E_OP0893_NV_REPEAT	((OPX_ERROR) (OPE_BASE+0x893))
/* A repeat query number that is not valid is being added to a QEN_BASE.
*/
#define			E_OP0894_EXCESSATTS	((OPX_ERROR) (OPE_BASE+0X894))
/* Too many attributes are being returned from a subselect.
*/
#define			E_OP0895_NO_SEJQTREE	((OPX_ERROR) (OPE_BASE+0X895))
/* Consistency Check: No query tree for a subselect join was found
*/
#define			E_OP0896_MODIFY_ATTR	((OPX_ERROR) (OPE_BASE+0X896))
/* Consistency Check: default attribute number 1 to modify does not exist
*/
#define			E_OP0897_BAD_CONST_RESDOM ((OPX_ERROR) (OPE_BASE+0X897))
/* Consistency Check: a query tree constant or resdom node uses an illegal
** source/destination type.
*/
#define			E_OP0898_ILL_LVARNO	((OPX_ERROR) (OPE_BASE+0X898))
/* Consistency Check: a DB procedure local variable number was referenced
** in a query tree that is out of range.
*/
#define			E_OP0899_BAD_TARGNO	((OPX_ERROR) (OPE_BASE+0X899))
/* A target number was found (in a query tree or otherwise) that is illegal
*/
#define			E_OP089A_DIFF_BOOL_FACT ((OPX_ERROR) (OPE_BASE+0X89A))
/* The old and new boolean factor bitmaps differ.
*/
#define			E_OP089B_PJOIN_FOUND    ((OPX_ERROR) (OPE_BASE+0X89B))
/* The PJOIN implied by qep; PJOIN is not supported any longer.
*/
#define			E_OP089C_BAD_VALID_LIST	((OPX_ERROR) (OPE_BASE+0X89C))
/* Consistency check: valid list is illegal.
*/
#define                 E_OP089D_QSF_INFO       ((OPX_ERROR) (OPE_BASE+0X89D))
/* Error when calling QSO_INFO.
*/
#define			E_OP089E_QSF_UNLOCK	((OPX_ERROR) (OPE_BASE+0X89E))
/* Error when calling QSO_UNLOCK.
*/
#define			E_OP089F_QSF_FAILCREATE	((OPX_ERROR) (OPE_BASE+0X89F))
/* Error when calling QSO_CREATE.
*/
#define			E_OP08A0_SD_COMPILE_FAIL ((OPX_ERROR) (OPE_BASE+0x8A0))
/* unexpected error in call of SDcompile for table: <p1>. Result: <p2>
*/
#define			E_OP08A1_TOO_SHORT	((OPX_ERROR) (OPE_BASE+0X8A1))
/* DSH buffer being reused is too short.
*/
#define			E_OP08A2_EXCEPTION	((OPX_ERROR) (OPE_BASE+0X8A2))
/* Unexpected exception occurred in query compilation
*/
#define         E_OP08A3_UNKNOWN_STATEMENT	((OPX_ERROR) (OPE_BASE+0X8A3))
/* The Compiler does not understand a statement type generated by the Parser.
*/
#define		E_OP08A4_DELIMIT_FAIL		((OPX_ERROR) (OPE_BASE+0X8A4))
/* Error generating delimited identifier
*/
#define		E_OP08A5_DATATYPE_MISMATCH	((OPX_ERROR) (OPE_BASE+0X8A5))
/*  Join attributes have different datatypes
*/
#define		E_OP08A6_NO_ATTRIBUTE		((OPX_ERROR) (OPE_BASE+0X8A6))
/* Equivalence class not represented by an attribute
*/
#define		E_OP08A7_BARREN			((OPX_ERROR) (OPE_BASE+0X8A7))
/* Parent node has no child when expected
*/
#define		E_OP08A8_MISSING_TID		((OPX_ERROR) (OPE_BASE+0X8A8))
/* TID attribute not found
*/
#define		E_OP08A9_BAD_RELATION		((OPX_ERROR) (OPE_BASE+0X8A9))
/* Unknown relation
*/
#define		E_OP08AA_EMPTY_NODE		((OPX_ERROR) (OPE_BASE+0X8AA))
/* node has no equivalence classes
*/
#define		E_OP08AB_BAD_JOIN_TYPE		((OPX_ERROR) (OPE_BASE+0X8AB))
/* Unsupported join strategy
*/
#define		E_OP08AC_UNKNOWN_OPERATOR	((OPX_ERROR) (OPE_BASE+0X8AC))
/* Unknown operator found in an expression
*/
#define		E_OP08AD_SPATIAL_ERROR		((OPX_ERROR) (OPE_BASE+0X8AD))
/* Some sort of problem compiling a spatial query plan (with Rtree's in it)
*/
#define		E_OP08AE_NOTSETI_PROC		((OPX_ERROR) (OPE_BASE+0X8AE))
/* Db procedure called with global temptab parameter, yet does not have
** "set of" formal parameter */
#define		E_OP08AF_GTTPARM_MISMATCH	((OPX_ERROR) (OPE_BASE+0X8AF))
/* Statement rule procedure has "set of" element defined NOT NULL NOT DEFAULT
** with no matching actual parameter */
#define		E_OP08B0_NODEFAULT		((OPX_ERROR) (OPE_BASE+0X8B0))
/* Db procedure called with rule invocation parm which doesn't have a match
** in corresponding "set of" parameter */
#define		E_OP08B1_PARAM_INVALID		((OPX_ERROR) (OPE_BASE+0X8B1))
/* Consistency check in compiling topsort-less descending sort. */
#define		E_OP08B2_DESC_ERROR		((OPX_ERROR) (OPE_BASE+0X8B2))

/*
**  OPTIMIZEDB errors
*/
#define			I_OP0900_INTERRUPT	((OPX_ERROR) (OPE_BASE+0x900))
/* user interrupt detected
*/
#define			E_OP0901_UNKNOWN_EXCEPTION ((OPX_ERROR) (OPE_BASE+0x901))
/* unknown exception occurred in optimizedb
*/
#define			E_OP0902_ADFSTARTUP	((OPX_ERROR) (OPE_BASE+0x902))
/* error starting up the ADF
** %s: internal error - starting up Abstact Data Type Facility
*/
#define			E_OP0903_ADFSESSION     ((OPX_ERROR) (OPE_BASE+0x903))
/* error starting an ADF SESSION
** %s: internal error - starting up Abstact Data Type Facility session
*/
#define                 E_OP0904_OPENFILE       ((OPX_ERROR) (OPE_BASE+0x904))
/* error dereferencing user location name or opening file
** %s: cannot open input file:%s, os status:%d
*/
#define                 E_OP0905_ARGUMENTS      ((OPX_ERROR) (OPE_BASE+0x905))
/* too many arguments for optimizedb
** %s: more than %d arguments
*/
#define                 E_OP0906_STACK          ((OPX_ERROR) (OPE_BASE+0x906))
/* ran out of memory
** %s: ran out of stack space
*/
#define                 E_OP0907_UNIQUECELLS	((OPX_ERROR) (OPE_BASE+0x907))
/* incorrectly specified number of unique cells -zu#
** %s: bad uniquecells value, -zu# range allowed 1<#<%d, but got :%d
*/
#define                 E_OP0908_HISTOCELLS	((OPX_ERROR) (OPE_BASE+0x908))
/* incorrectly specified number of unique cells -zr#
** %s: bad histogram cell count, -zr# range allowed 1<#<%d, but got :%d
*/
#define                 E_OP0909_INGRESFLAGS	((OPX_ERROR) (OPE_BASE+0x909))
/* too many ingres flags (or unrecognizable flags)
** %s: more than %d ingres flags, or unrecognizable flags
*/
#define                 E_OP090A_DATABASE	((OPX_ERROR) (OPE_BASE+0x90A))
/* more than one database name, or unrecognized parameter
** %s: unexpected parameter :%s
*/
#define                 E_OP090B_DBLENGTH	((OPX_ERROR) (OPE_BASE+0x90B))
/* data base name length too long
** %s: database name length must be <= %d :%s
*/
#define                 E_OP090C_RELLENGTH	((OPX_ERROR) (OPE_BASE+0x90C))
/* relation name length too long
** %s: relation name length must be <= %d :%s
*/
#define                 E_OP090D_ATTLENGTH	((OPX_ERROR) (OPE_BASE+0x90D))
/* attribute name length too long
** %s: attribute name length must be <= %d :%s
*/
#define                 E_OP090E_PARAMETER	((OPX_ERROR) (OPE_BASE+0x90E))
/* unrecognized parameter
** %s: unrecognized parameter : %s
*/
#define                 E_OP090F_TABLES		((OPX_ERROR) (OPE_BASE+0x90F))
/* too many tables too process
** optimizedb: more than %d tables for database  '%s'
*/
#define                 E_OP0910_DUPTABID	((OPX_ERROR) (OPE_BASE+0x910))
/* duplicate table ID in system catalogs
** optimizedb: duplicate table IDs in database %s, table %s
*/
#define                 E_OP0911_NOTABLE	((OPX_ERROR) (OPE_BASE+0x911))
/* cannot find table
** optimizedb: database %s, table %s cannot be found
*/
#define                 E_OP0912_VIEW		((OPX_ERROR) (OPE_BASE+0x912))
/* view is ignored
** optimizedb: database %s, table %s is a view and will be ignored
*/
#define                 E_OP0913_ATTRIBUTES	((OPX_ERROR) (OPE_BASE+0x913))
/* more attributes defined for table than is allowed
** %s: database %s, table %s, more than %d columns defined
*/
#define                 E_OP0914_ATTRTUPLES	((OPX_ERROR) (OPE_BASE+0x914))
/* more than one attribute tuple for attribute ID
** %s: database %s, table %s, column %s inconsistent sys catalog
*/
#define                 E_OP0915_NOATTR		((OPX_ERROR) (OPE_BASE+0x915))
/* attribute not found
** optimizedb: database %s, table %s, column %s not found
*/
#define                 E_OP0916_TYPE		((OPX_ERROR) (OPE_BASE+0x916))
/* invalid TYPE id found in attribute catalog
** optimizedb: bad datatype id =%d in iiattribute.attfrmt, column name %s
*/
#define                 E_OP0917_HTYPE		((OPX_ERROR) (OPE_BASE+0x917))
/* call to adc_hg_dtln() returned an error
** %s: histogram type=%d len=%d in iiattribute, column name %s, 
**    error->%s
*/
#define                 E_OP0918_NOCOLUMNS	((OPX_ERROR) (OPE_BASE+0x918))
/* no columns to gather statistics on
** optimizedb: no columns for database %s, table %s
*/
#define                 E_OP0919_READ_ERR	((OPX_ERROR) (OPE_BASE+0x919))
/* error when reading input file.
*/
#define                 E_OP091A_STACK		((OPX_ERROR) (OPE_BASE+0x91A))
/* ran out of dynamic memory to allocate.
*/
#define                 E_OP091B_TUPLECOUNT	((OPX_ERROR) (OPE_BASE+0x91B))
/* only one tuple expected from min/max aggregate
** optimizedb: bad retrieval, database '%s', table '%s', i:%d",
*/
#define                 I_OP091C_NOROWS		((OPX_ERROR) (OPE_BASE+0x91C))
/* only one tuple expected from min/max aggregate
** optimizedb: no rows for database '%s', table '%s'
*/
#define                 E_OP091D_HISTOGRAM	((OPX_ERROR) (OPE_BASE+0x91D))
/* ADF histogram conversion failed 
** optimizedb: histogram conversion failed, column %s, code = %d
*/
#define                 E_OP091E_BAD_VALUE_LENGTH ((OPX_ERROR) (OPE_BASE+0x91E))
/* Wrong input value '%0d' for statistics on table '%1c', column '%2c';
** Invalid value length; does not match length of column or < 1.
*/
#define                 E_OP091F_DATATYPE	((OPX_ERROR) (OPE_BASE+0x91F))
/* error calling adc_tmcvt, or adc_tmlen
** %s: error displaying type %d, length %d
*/
#define                 E_OP0920_DATATYPE	((OPX_ERROR) (OPE_BASE+0x920))
/* unsupported histogram type
** optimizedb: unsupported histogram datatype; type %d, length %d
*/
#define                 E_OP0921_ERROR		((OPX_ERROR) (OPE_BASE+0x921))
/* too many parameters to error formatting routine - consistency check
** optimizedb: error parameter count too large
*/
#define                 E_OP0922_BAD_ERLOOKUP	((OPX_ERROR) (OPE_BASE+0x922))
/* ERlookup routine failed, internal inconsistency
** optimizedb: cannot find error message 
*/
#define                 E_OP0923_DMF_BIT	((OPX_ERROR) (OPE_BASE+0x923))
/* consistency check - statistics bit in iirelation not set, but statistics exist
** statdump: statistics exist, but iirelation bit not set
*/
#define                 E_OP0924_NON_BASE	((OPX_ERROR) (OPE_BASE+0x924))
/* consistency check - non base relation has statistics
** %s: non-base relation has statistics %s, data base %s
*/
#define                 W_OP0925_NOSTATS	((OPX_ERROR) (OPE_BASE+0x925))
/* statdump: database %s, table %s, column %s statistics not found
*/
#define                 E_OP0926_DUPLICATES	((OPX_ERROR) (OPE_BASE+0x926))
/* statdump: database %s, table %s, column %s ... %d duplicates stats found
*/
#define                 E_OP0927_DUPLICATES	((OPX_ERROR) (OPE_BASE+0x927))
/* statdump: database %s, table %s, column %s ... %d duplicates histograms found
*/
#define                 E_OP0928_BAD_COMPLETE 	((OPX_ERROR) (OPE_BASE+0x928))
/* optimizedb: invalid value of complete flag.
*/
#define                 E_OP0929_ADC_HDEC 	((OPX_ERROR) (OPE_BASE+0x929))
/* optimizedb: error occurred when calling adc_hdec().
*/
#define                 I_OP092A_STATSDELETED	((OPX_ERROR) (OPE_BASE+0x92A))
/* optimizedb: no rows for database %s, table %s, column %s\n 
*/
#define                 E_OP092B_COMPARE	((OPX_ERROR) (OPE_BASE+0x92B))
/* ADF ADC_COMPARE failed
** optimizedb: data comparison failed, column %s, code = %d
*/
#define                 I_OP092C_NULL		((OPX_ERROR) (OPE_BASE+0x92C))
/* optimizedb: NULL value not expected, column %s, code = %d
*/
#define                 I_OP092D_REL_NOT_FOUND	((OPX_ERROR) (OPE_BASE+0x92D))
/* optimizedb: database %s, table %s not found
*/
#define                 E_OP092E_CVSN_CERR	((OPX_ERROR) (OPE_BASE+0x92E))
/* Error %0d occurred when converting STANDARD_CATALOG_LEVEL value of '%1c'.
*/
#define                 E_OP092F_STD_CAT	((OPX_ERROR) (OPE_BASE+0x92F))
/* %s version does not match the standard catalog level in database %s
*/

/*
**  Defines below will eventually migrate to eropf.msg. They were put in there
**  originally, in eropf.h was included, but that created build problems, so
**  they were moved here. No comments are provided.
**
**  08-jul-89 (stec)
**	Added.
*/
#define                 E_OP0930_NOT_BASE	    ((OPX_ERROR) (OPE_BASE+0x930))
#define                 E_OP0931_NO_FILE	    ((OPX_ERROR) (OPE_BASE+0x931))
#define                 E_OP0932_BAD_PREC	    ((OPX_ERROR) (OPE_BASE+0x932))
#define                 E_OP0933_BAD_USER	    ((OPX_ERROR) (OPE_BASE+0x933))
#define                 E_OP0934_BAD_SCAT_TYPE	    ((OPX_ERROR) (OPE_BASE+0x934))
#define                 E_OP0935_LVSN_CERR	    ((OPX_ERROR) (OPE_BASE+0x935))
#define                 E_OP0936_ADI_TYID	    ((OPX_ERROR) (OPE_BASE+0x936))
#define                 E_OP0937_OPEN_ERROR	    ((OPX_ERROR) (OPE_BASE+0x937))
#define                 E_OP0938_CLOSE_ERROR	    ((OPX_ERROR) (OPE_BASE+0x938))
#define                 E_OP0939_TOO_MANY_COLS	    ((OPX_ERROR) (OPE_BASE+0x939))
#define                 E_OP093A_ADC_LENCHK	    ((OPX_ERROR) (OPE_BASE+0x93A))
#define                 E_OP093B_TOO_MANY_ROWS	    ((OPX_ERROR) (OPE_BASE+0x93B))
#define                 E_OP093C_INP_FAIL	    ((OPX_ERROR) (OPE_BASE+0x93C))
#define                 E_OP093D_MINMAX_NOT_ALLOWED ((OPX_ERROR) (OPE_BASE+0x93D))
#define                 I_OP093E_NO_STATS	    ((OPX_ERROR) (OPE_BASE+0x93E))
#define                 E_OP093F_BAD_COUNT	    ((OPX_ERROR) (OPE_BASE+0x93F))
#define                 E_OP0940_READ_ERR	    ((OPX_ERROR) (OPE_BASE+0x940))
#define                 E_OP0941_TEXT_NOTFOUND	    ((OPX_ERROR) (OPE_BASE+0x941))
#define                 E_OP0942_ADI_PM_ENCODE	    ((OPX_ERROR) (OPE_BASE+0x942))
#define                 E_OP0943_ADI_FICOERCE	    ((OPX_ERROR) (OPE_BASE+0x943))
#define                 E_OP0944_ADF_FUNC	    ((OPX_ERROR) (OPE_BASE+0x944))
#define                 E_OP0945_HISTVAL_NOT_INC    ((OPX_ERROR) (OPE_BASE+0x945))
#define                 E_OP0946_CELL_NO	    ((OPX_ERROR) (OPE_BASE+0x946))
#define                 E_OP0947_WRONG_COUNT	    ((OPX_ERROR) (OPE_BASE+0x947))
#define                 E_OP0948_CELL_TOO_FEW	    ((OPX_ERROR) (OPE_BASE+0x948))
#define                 E_OP0949_CELL_TOO_MANY	    ((OPX_ERROR) (OPE_BASE+0x949))
#define                 E_OP094A_TRUNC		    ((OPX_ERROR) (OPE_BASE+0x94A))
#define                 E_OP094B_COL_NO_MISMATCH    ((OPX_ERROR) (OPE_BASE+0x94B))
#define                 E_OP094C_ERR_CRE_SAMPLE	    ((OPX_ERROR) (OPE_BASE+0x94C))
#define                 E_OP094D_CANT_SAMPLE	    ((OPX_ERROR) (OPE_BASE+0x94D))
#define                 E_OP094E_INV_SAMPLE	    ((OPX_ERROR) (OPE_BASE+0x94E))
#define                 E_OP094F_SAMPLE_NOT_ALLOWED ((OPX_ERROR) (OPE_BASE+0x94F))
#define                 W_OP0950_NOSAMPLING	    ((OPX_ERROR) (OPE_BASE+0x950))
#define                 E_OP0951_BAD_NO_UNIQUE	    ((OPX_ERROR) (OPE_BASE+0x951))
#define                 E_OP0952_BAD_NO_ROWS	    ((OPX_ERROR) (OPE_BASE+0x952))
#define                 E_OP0953_BAD_NO_PAGES	    ((OPX_ERROR) (OPE_BASE+0x953))
#define                 E_OP0954_BAD_NO_OVFLOW	    ((OPX_ERROR) (OPE_BASE+0x954))
#define                 E_OP0955_BAD_NULL_COUNT	    ((OPX_ERROR) (OPE_BASE+0x955))
#define                 E_OP0956_TOTAL_COUNT_OFF    ((OPX_ERROR) (OPE_BASE+0x956))
#define                 E_OP0957_NO_CAPABILITY	    ((OPX_ERROR) (OPE_BASE+0x957))
#define                 I_OP0958_COUNTING_ROWS	    ((OPX_ERROR) (OPE_BASE+0x958))
#define                 E_OP0959_CAP_ERR	    ((OPX_ERROR) (OPE_BASE+0x959))
#define                 E_OP095A_TUPLE_OVFLOW	    ((OPX_ERROR) (OPE_BASE+0x95A))
#define                 E_OP095B_SYSHWEXC	    ((OPX_ERROR) (OPE_BASE+0x95B))
#define			E_OP095C_FCONV_ERR	    ((OPX_ERROR) (OPE_BASE+0x95C))
#define			W_OP095D_UDT		    ((OPX_ERROR) (OPE_BASE+0x95D))
#define			E_OP095E_BAD_UNIQUE	    ((OPX_ERROR) (OPE_BASE+0x95E))
#define			W_OP095F_WARN_UNIQUE	    ((OPX_ERROR) (OPE_BASE+0x95F))
#define			E_OP0960_BAD_LENGTH	    ((OPX_ERROR) (OPE_BASE+0x960))
#define			W_OP0961_SQLWARN1	    ((OPX_ERROR) (OPE_BASE+0x961))
#define			W_OP0962_SQLWARN2	    ((OPX_ERROR) (OPE_BASE+0x962))
#define			W_OP0963_SQLWARN3	    ((OPX_ERROR) (OPE_BASE+0x963))
#define			W_OP0964_SQLWARN4	    ((OPX_ERROR) (OPE_BASE+0x964))
#define			W_OP0965_SQLWARN5	    ((OPX_ERROR) (OPE_BASE+0x965))
#define			W_OP0966_SQLWARN6	    ((OPX_ERROR) (OPE_BASE+0x966))
#define			W_OP0967_SQLWARN7	    ((OPX_ERROR) (OPE_BASE+0x967))
#define			E_OP0968_OPTDB_IANDO	    ((OPX_ERROR) (OPE_BASE+0x968))
#define			E_OP0969_OPTIX_NOATTS	    ((OPX_ERROR) (OPE_BASE+0x969))
#define			W_OP096A_OPTIX_1ATT	    ((OPX_ERROR) (OPE_BASE+0x96A))
#define			W_OP096B_NO_HISTOGRAM	    ((OPX_ERROR) (OPE_BASE+0x96B))
#define			W_OP0970_SERIALIZATION	    ((OPX_ERROR) (OPE_BASE+0x970))
#define			E_OP0971_TOO_MANY_RETRIES   ((OPX_ERROR) (OPE_BASE+0x971))
#define			E_OP0972_SCALE_ERR	    ((OPX_ERROR) (OPE_BASE+0x972))
#define                 W_OP0973_TOTAL_COUNT_OFF    ((OPX_ERROR) (OPE_BASE+0x973))
#define                 E_OP0974_BAD_MAX_COLS	    ((OPX_ERROR) (OPE_BASE+0x974))
#define                 E_OP0975_STAT_COUNT	    ((OPX_ERROR) (OPE_BASE+0x975))
#define			W_OP0976_PREC_WARNING       ((OPX_ERROR) (OPE_BASE+0x976))
#define                 E_OP0980_ADI_PM_ENCODE	    ((OPX_ERROR) (OPE_BASE+0x980))
#define                 E_OP0981_ADI_FICOERCE	    ((OPX_ERROR) (OPE_BASE+0x981))
#define                 E_OP0982_ADF_FUNC	    ((OPX_ERROR) (OPE_BASE+0x982))
#define                 E_OP0983_ADC_COMPARE 	    ((OPX_ERROR) (OPE_BASE+0x983))
#define                 E_OP0984_NO_PRIV_TABLE_STATS ((OPX_ERROR) (OPE_BASE+0x984))
#define                 E_OP0985_NOHIST		    ((OPX_ERROR) (OPE_BASE+0x985))
#define                 E_OP0990_BAD_RPTFACTOR	    ((OPX_ERROR) (OPE_BASE+0x990))
#define                 W_OP0991_INV_RPTFACTOR	    ((OPX_ERROR) (OPE_BASE+0x991))
#define                 W_OP0992_ROWCNT_INCONS	    ((OPX_ERROR) (OPE_BASE+0x992))
#define                 E_OP0993_XRBADMIX     	    ((OPX_ERROR) (OPE_BASE+0x993))
#define                 E_OP0994_DUP_RELS     	    ((OPX_ERROR) (OPE_BASE+0x994))

/* DISTRIBUTED OPTIMIZER ERRORS */
#define                 E_OP0A00_MAXSITE	((OPX_ERROR) (OPE_BASE+0xA00))
/* maximum number of sites exceeded OPD_MAXSITE
*/
#define                 E_OP0A01_USELESS_PLAN	((OPX_ERROR) (OPE_BASE+0xA01))
/* plan expected to be useful but it was not
*/
#define                 E_OP0A80_UNSUPPORTED	((OPX_ERROR) (OPE_BASE+0xA80))
/* this type of distributed query not supported - ORIG node
*/
#define                 E_OP0A81_SITE		((OPX_ERROR) (OPE_BASE+0xA81))
/* site has not been selected for query plan
*/
#define                 E_OP0A82_RESULT_VAR	((OPX_ERROR) (OPE_BASE+0xA82))
/* unexpected result variable number for update statement
*/
#define                 E_OP0A83_CONSTRAINT	((OPX_ERROR) (OPE_BASE+0xA83))
/* consistency check - unexpected state for corelated subselect constraint
*/
#define                 E_OP0A84_TEMP_REQ	((OPX_ERROR) (OPE_BASE+0xA84))
/* consistency check - temp table required but not a data transfer
*/
#define                 E_OP0A85_UPDATE_VAR	((OPX_ERROR) (OPE_BASE+0xA85))
/* consistency check - expecting update variable to be defined
*/
#define                 E_OP0A86_EQCLASS	((OPX_ERROR) (OPE_BASE+0xA86))
/* consistency check - cannot add attribute to equivalence class for update
*/
#define                 E_OP0A87_UPDT_MATCH	((OPX_ERROR) (OPE_BASE+0xA87))
/* consistency check - new range variable for target relation expected to be
** the last item in base relation list
*/
#define                 E_OP0A88_REPEAT		((OPX_ERROR) (OPE_BASE+0xA88))
/* consistency check - repeat query parameter is out of order
*/
#define                 E_OP0A89_SAGG		((OPX_ERROR) (OPE_BASE+0xA89))
/* consistency check - simple aggregate constant count is inconsistent
*/
#define                 E_OP0A8A_OPCSAGG	((OPX_ERROR) (OPE_BASE+0xA8A))
/* consistency check - unexpected tree structure found for simple aggregate
*/
#define                 E_OP0A8B_OPCPARM	((OPX_ERROR) (OPE_BASE+0xA8B))
/* consistency check - parameter nodes out of order for simple aggregate
*/
#define                 E_OP0A8C_COST		((OPX_ERROR) (OPE_BASE+0xA8C))
/* consistency check - cost resolution for final query plan cannot be found
*/
#define                 E_OP0A8D_VARIABLE	((OPX_ERROR) (OPE_BASE+0xA8D))
/* consistency check - OPF variable list is not ordered
*/
#define                 E_OP0A8E_SUB_TYPE	((OPX_ERROR) (OPE_BASE+0xA8E))
/* consistency check - unexpected subquery type
*/
#define                 E_OP0A8F_UPDATE_MODE	((OPX_ERROR) (OPE_BASE+0xA8F))
/* consistency check - unknown update mode
*/
#define                 E_OP0A90_NO_ATTRIBUTES	((OPX_ERROR) (OPE_BASE+0xA90))
/* consistency check - no attributes in the for update list of the cursor
*/
#define                 E_OP0A91_CORELATED	((OPX_ERROR) (OPE_BASE+0xA91))
/* consistency check - query tree corelation inconsistent
*/
#define			E_OP0A92_BAD_LANGUAGE	((OPX_ERROR) (OPE_BASE+0xA92))
/* bad language ID found for query tree to text convertion */
#define			E_OP0A93_BAD_QUERYMODE	((OPX_ERROR) (OPE_BASE+0xA93))
/* consistency check - unexpected query mode */
#define			E_OP0A94_OPCFPROJ	((OPX_ERROR) (OPE_BASE+0xA94))
/* consistency check - projection/function aggregate error
*/
#define			E_OP0A95_OPCBYTARGET	((OPX_ERROR) (OPE_BASE+0xA95))
/* consistency check - function aggregate has no by/target list
*/
#define			E_OP0A96_OPCSUBQTYPE	((OPX_ERROR) (OPE_BASE+0xA96))
/* consistency check - unrecognized subquery type
*/
#define                 E_OP0A97_OPCSORTTYPE    ((OPX_ERROR) (OPE_BASE+0xA97))
/* Consistency Check -- sort requires unsortable datatype */
#define                 E_OP0A97_OPCTIDIDX	((OPX_ERROR) (OPE_BASE+0xA97))
/* Consistency Check -- sort requires unsortable datatype */
#define			E_OP0A98_OPCTIDEQC	((OPX_ERROR) (OPE_BASE+0xA98))
/* consistency check - no tid equivalence class found when tid join required
*/

#define			E_OP0AA0_BAD_COMP_VALUE ((OPX_ERROR) (OPE_BASE+0xAA0))
/* Bad compresion value passed from parser to optimizer */

/* DB PROCEDURE ERRORS */
#define                 E_OP0B80_UNKNOWN_STATEMENT ((OPX_ERROR) (OPE_BASE+0xB80))
/* consistency check - unknown statement type
*/
#define                 E_OP0B81_VARIABLE_DEC	 ((OPX_ERROR) (OPE_BASE+0xB81))
/* consistency check - a variable declaration must be at the beginning of any
**  procedure
*/
#define                 E_OP0B82_MISSING_PARAMETER  ((OPX_ERROR) (OPE_BASE+0xB82))
/* consistency check - internally generated variables for procedures are not in
**  order, or are missing
*/

/* GATEWAY optimzer errors */
#define                 E_OP0C80_NOTID		    ((OPX_ERROR) (OPE_BASE+0xC80))
/* consistency check - no tid is available when one was expected
*/


/* The following defines select pieces of code which may or may not be include
** in a production system
*/

#define                 OPT_OPERRORS            TRUE
/* causes optimizer errors to be logged to TRdisplay */

/*
** The following defines are indexes into the trace/timing array ops_tt which
** will select the respective function.  The defines will
** be used as the trace flag number, AND as the parameter to the #ifdef
** to select code for the trace flag.  Thus, the trace code can be
** conditionally compiled by defining the trace flag below in the proper
** context (i.e. inside a xDEBUG, or xDEV_TEST block )
*/
#define                 OPT_GMAX        128
/* this value defines the end of the global trace flags and the beginning
** of the session level trace flags
** - values 0 to OPT_GMAX-1 are reserved for global trace flags
** - values OPT_GMAX and greater are session level trace flags
*/
#define			OPT_T129_NO_LOAD	1
/* SET TRACE POINT OP129
** disables the APPEND -> LOAD optimization for insert/select.
** (create-as-select still does LOAD if appropriate, since that's the way
** we've done it for a long time.
*/

#define                 OPT_F002_SKIPCONSISTENCY	2
/* SET TRACE POINT OP130
** skip non-critical consistency checking
*/
#define                 OPT_T131_NO_JOINPQUAL   3
/* SET TRACE POINT OP131
** Disable generation of join-time partition qualifications.  Partition
** qualification in the orig is unaffected.
*/

#define                 OPT_F004_FLATTEN 4
/* SET TRACE POINT OP132
** causes flattening of subselects to occur, with slight variations from
** correct results but with large performance gains,
** - EXISTS are flatten
** - corelated variables are turned into BY list elements
** Incorrect results known - exact subselect e.g. r.a = SELECT ... will
** not report an error if more than one tuple is returned from the select 
*/

#define                 OPT_F005_PARSER 5
/* SET TRACE POINT OP133
** ingore the parser flag which causes flattening to be avoided */

#define                 OPT_F006_NONOTEX 6
/* SET TRACE POINT OP134
** do NOT apply outer join "not exists"/"not in" subselect flattening
** algorithm
*/

#define                 OPT_AGSNAME     7
/* SET TRACE POINT OP135
** print the name of the source range variable for the aggregate about to 
** be processed */                                              

#define                 OPT_AGSIMPLE    8
/* SET TRACE POINT OP136
** print a message if a simple aggregate is found                         
*/

#define                 OPT_AGSUB       9
/* SET TRACE POINT OP137
** print the query subtree associated with the aggregate to be processed  
*/

#define                 OPT_MAKAVAR     10
/* SET TRACE POINT OP138
** print message when a new variable is created e.g. when an aggregate    
** function is replaced by a new source variable                          
*/
#define                 OPT_AGCANDIDATE 11
/* SET TRACE POINT OP139
** trace the processing which decides that aggregates can be computed     
** together                                                               
*/

#define                 OPT_AGREPLACE   12
/* SET TRACE POINT OP140
** trace the replacement in the query tree of the result of an aggregate  */

#define                 OPT_PROJECTION  13
/* SET TRACE POINT OP141
** this flag affects semantics - if TRUE - do not project aggregate bydoms
** thus the default case if to project aggregrate bydoms
*/

#define                 OPT_TIMER       14
/* SET TRACE POINT OP142
** print the time required to perform the optimization                    
*/

#define                 OPT_CONCURRENT  15
/* SET TRACE POINT OP143
** print info about aggregates which will be executed together            
*/

#define			OPT_F016_OPFSTATS	16
/* SET TRACE POINT OP144
** display OPF session stats in DBMS log
*/

#define                 OPT_FCOTREE     17
/* SET TRACE POINT OP145
** print the best CO tree - used by SET QEP 
*/

#define                 OPT_F001_DUMP_QTREE 18
/* SET TRACE POINT OP146
** enables dumping of query trees at the beginning of aggregate processing    
*/

#define                 OPT_F019_AGGCOMPAT 19
/* SET TRACE POINT OP147
** code to trace the checking of whether an aggregate can be combined
** with another aggregate in the query
*/

#define                 OPT_F020_NOPRESORT	20
/* SET TRACE POINT OP148
** avoid presort of TIDs, keys for Tjoin, Kjoin.
*/

#define                 OPT_F021_QPTREE		21
/* SET TRACE POINT OP149
** print action hdr/node tree bit of op150
*/

#define			OPT_F022_FULL_QP	22
/* set [no]trace point op150
** trace a full QEF_QP_CB struct in OPC, useful for
** debugging OPC problems
*/

#define                 OPT_F023_NOCOMP         23
/* set [no]trace point op151
** suppress query compilation phase of optimizer so
** no query plan is produced
** - good for determining if problem is in OPF or OPC
*/

#define                 OPT_F024_NOTIMEOUT      24
/* SET TRACE POINT OP152
** equivalent to set joinop notimeout */

#define                 OPT_F025_RANGETAB	25
/* SET TRACE POINT OP153
** Print the subquery range table (a subset of op156) */

#define                 OPT_F026_FULL_OPKEY	26
/* SET TRACE POINT OP154
** Print the full OPC keying information */

#define                 OPT_F027_FUSED_OPKEY	27
/* SET TRACE POINT OP155
** Print the FUSED OPC keying info */

#define                 OPT_F028_JOINOP		28
/* SET TRACE POINT OP156
** Print all the joinop tables */

#define                 OPT_F029_HISTMAP	29
/* SET TRACE POINT OP157
** Print map of equivalence classes for which histograms are required 
** as calculated by opn_sm1 
*/

#define                 OPT_F030_SQLNAMES	30
/* SET TRACE POINT OP158
** Use the new SQLDA2 structure with names
*/
#define                 OPT_F031_WARNINGS       31
/* SET TRACE POINT OP159
** send warning messages to user
*/
#define                 OPT_F032_NOEXECUTE      32
/* set [no]trace point op160
** optimize but do not execute this query, this is
** equivalent to "set noqueryrun" in 5.0
*/
#define			OPT_F033_OPF_TO_OPC	33
/* SET TRACE POINT OP161
** Trace the data structs that opf passes to opc
*/
#define			OPT_F034_NEWFEATS	34
/* SET TRACE POINT OP162 
** enable any tp'ed new feature in OPF code (saves having individual 
** tp's to activate work in progress)
*/
#define			OPT_F035_NO_HISTJOIN	35
/* SET TRACE POINT OP163
** Prevents use of histograms in join cardinality estimation
*/
#define			OPT_F036_OPC_DISTRIBUTED 36
/* SET TRACE POINT OP164
** Dump distributed query text for each CO tree node
** (distributed only)
*/
#define			OPT_F037_STATISTICS     37
/* set trace point op165
** 
** Disables use of statistics for this session, this is
** equivalent to the "set nostatistics" flag in 5.0
*/
#define			OPT_F038_FAKE_GATEWAY	38
/* SET TRACE POINT OP166
** enables a fake gateway optimize optimizer (simulate
** gateway OPF)
*/
#define			OPT_F039_NO_KEYJOINS	39
/* SET TRACE POINT OP167
** disable the selection of keyjoins in query plan
*/
#define			OPT_F040_SQL_TEXT	40
/* SET TRACE POINT OP168
** create SQL text for CO nodes when SET QEP is enabled
*/
#define			OPT_F041_MULTI_SITE	41
/* SET TRACE POINT OP169
** treat all distributed queries as multi-site (distributed only)
*/
#define                 OPT_F042_PTREEDUMP      42
/* SET TRACE POINT OP170
** Doug's modified parse tree display
*/
#define			OPT_F043_CONSTANTS	43
/* SET TRACE POINT OP171
** Destroys text for all PST_CONSTANT nodes so they will
** be sent as ~V (distributed only)
*/
#define			OPT_F044_NO_RTREE	44
/* SET TRACE POINT OP172
** inhibit use of Rtree TID joins for queries with spatial predicates
*/
#define			OPT_F045_QENPROW	45
/* SET TRACE POINT OP173
** enable the building of the qen_prow CXs in the QEN_NODE structure
*/
#define			OPT_F046_CONTROLC       46
/* SET TRACE POINT OP174
** change behaviour of control C to exit with best
** query plan */
#define			OPT_F047_CHK_SEG	47
/* SET TRACE POINT OP175 
** Check all text segments for printable characters, 
** useful to trap memory trashing. (distributed only)
*/
#define                 OPT_F048_FLOAT		48
/* SET TRACE POINT OP176
** do not attempt to recover from float exceptions in the optimizer
** but simply continue 
*/
#define                 OPT_F049_KEYING         49
/* SET TRACE POINT OP177
** use keying strategies whenever possible, no matter how expensive
** over other join strategies
*/
#define                 OPT_F050_COMPLETE	50
/* SET TRACE POINT OP178
** use TRUE as the default when the complete flag referenced */
#define                 OPT_F051_QPDUMP         51
/* SET TRACE POINT OP179
** hex dump of query plan memory stream */
#define			OPT_F052_CARTPROD	52
/* SET TRACE POINT OP180
** Look at cartesean product search space, useful for multi-attribute
** keying structures, in which the cart prod of small relations could
** be used to key into the large relation
*/
#define                 OPT_F053_FASTOPT        53
/* SET TRACE POINT OP181
** - look at reduced search space given hueristic estimates
** of index usage */
#define                 OPT_F054_OLDCNF		54
/* SET TRACE POINT OP182
** - use old conjunctive normal form transformation logic; i.e. transform
** all Boolean expressions. This may lead to OP0002/0302 error conditions
** for excessively complex queries */
#define                 OPT_F055_NOCNF		55
/* SET TRACE POINT OP183
** - do NOT perform the conjunctive normal transformation for Boolean 
** expressions. This overrides the 2.0 logic of performing the transform
** unless the query is deemed to be too complex. */
#define			OPT_F056_NOSMARTDISK	56
/* SET TRACE POINT OP184
** -Don't use a Smart Disk scan, even if it is available 
*/
#define			OPT_F057_WHYNOSMARTDISK	57
/* SET TRACE POINT OP185
** -Print (non-obvious) reasons for rejecting Smart Disk scan. 
*/
#define			OPT_F058_SD_CONJUNCT_TRACE 58
/* SET TRACE POINT OP186
** -Trace qualification conjuncts using Smart Disk 
*/
#define                 OPT_F059_CARTPROD 59
/* SET TRACE POINT OP187
** - Turn off cart prod keying optimization
*/
#define                 OPT_F060_QEP    60
/* SET TRACE POINT OP188
** - print QEP's as new subplans are discovered to improve
** performance
*/
#define                 OPT_F061_CORRELATION 61
/* SET TRACE POINT OP189
** - on project restricts, independence of attributes is assumed, which
** causes selectivities to become small rapidly, this flag assumes
** some correlation exists between attributes, so that selectivities
** do not become small as rapidly */
#define                 OPT_F062_OLDJCARD 62
/* SET TRACE POINT OP190
** - uses pre-1.3(2.0) join cardinality computation */
#define                 OPT_F063_CORRELATION 63
/* SET TRACE POINT OP191
** - on project restricts, independence of attributes is assumed, which
** causes selectivities to become small rapidly, this flag turns off
** some code which assumes correlation exists between attributes, 
** so that selectivities become smaller more rapidly
** - this is slightly different from OP191, as it affects processing
** of multi-equivalence class boolean factors, where the default is
** to assume correlation between attributes
*/
#define                 OPT_F064_NOKEY  64
/* SET TRACE POINT OP192
** - previous to 6.4 keying was performed based on the first attribute
** only, in order to provide some backward compatibility, this flag
** will turn off the enhanced keying estimates, since there are
** contrived cases in which what OPF thinks and OPC uses may not
** be synced */
#define                 OPT_F065_8CHAR  65
/* SET TRACE POINT OP193
** - A new optimization for > 8 character histograms which ignores
** the histogram in cases of an =<constant> key and uses the repitition
** factor instead is not always the best choice, this trace point
** turns it off - b36652 */
#define                 OPT_F066_GUESS  66
/* SET TRACE POINT OP194
** Do not perform initial guess for indexes but instead use
** full enumeration immediately */
#define			OPT_F067_PQ 67
/* SET TRACE POINT OP195
** Enable parallel query compilations */
#define                 OPT_F068_OLDFLATTEN 68
/* SET TRACE POINT OP196
** This is a 6.5 trace point which will disable the new flattening
** algorithms implemented in 6.5 and use the algorithms present in
** 6.4 */
#define                 OPT_F069_ALLPLANS	69
/* SET TRACE POINT OP197
** - look at all query plans, ignoring cost, used in combination
** with OP255
*/
#define                 OPT_F070_RIGHT  	70
/* SET TRACE POINT OP198
** Currently, right and full key joins are disabled
** in the optimizer, this will enable them for the OPC/QEF
** future development
*/
#define                 OPT_F071_QEP_WITHOUT_COST  	71
/* SET TRACE POINT OP199
** Print out a QEP without cost statistics.  This is useful for verifying
** during run_all that certain QEN strategies are being tested.  The nice
** part is that the highly variable cost statistics, which cause so many
** difs won't be generated.
*/
#define                 OPT_F072_LVCH_TO_VCH    72
/*
**  Set Trace Point Op200
**      Instructs OPC to set up any descriptors for long varchar
**      as varchar.  Useful for debugging BLOB problems.
**  Especially before FE BLOB support is available :-).
*/
#define			OPT_F073_NODEFAULT	73
/* SET TRACE POINT OP201
** Turn off optimization which assumes that if one side of a
** join has a histogram and the other side does not, then the
** distributions of both side are similar and thus the same
** histogram can be used for the join 
*/
#define			OPT_F074_UVAGG		74
/* SET TRACE POINT OP202
** Turn on union view optimization for simple aggregates
*/
#define			OPT_F075_FLOATQEP	75
/* SET TRACE POINT OP203
** Turn on float display of QEP cost values for greater accuracy
*/
#define			OPT_F076_BARF 		76
/* SET TRACE POINT OP204
** Enable function instance execution count dumping     
*/
#define		OPT_F077_STATS_NOSTATS_MAX	77
/* SET TRACE POINT OP205
** Switch on opf_stats_nostats_max
*/
#define		OPT_F078_NO_STATS_NOSTATS_MAX	78
/* SET TRACE POINT OP206
** Switch off opf_stats_nostats_max
*/
#define			OPT_F081_DUMPENUM	81
/* SET TRACE POINT OP209
** Dump enumeration memory structures
*/
#define			OPT_F082_QEPDUMP	82
/* SET TRACE POINT OP210
** Switch on QEP dumping - more stuff than SET QEP 
*/
#define			OPT_F083_OLDIDXORDER	83
/* SET TRACE POINT OP211
** use old 'reverse var' order when adding useful secondary indices
*/
#define			OPT_F084_TBLAUTOSTRUCT	84
/* SET TRACE POINT OP212
** Turn on 'table_auto_structure' dynamically
*/
#define			OPT_F085_MISSINGSTATS	85
/* SET TRACE POINT OP213
** report those attributes in the WHERE and ON cluases that are missing
** statistics
*/
#define			OPT_F086_DUMP_QTREE2	86
/* SET TRACE POINT OP214
** Display query tree in op146 style, at op170 points in the code
*/
#define			OPT_F087_ALLFRAGS	87
/* SET TRACE POINT OP215
** - print all query fragments (strictly 'all query fragments that have
** made it into an OPO_CO structure), even if they are to be discarded,
** together with all best plans.  
*/
#define			OPT_F088_BESTPLANS	88
/* SET TRACE POINT OP216
** - print just the best query plans in the order they are found.
*/
#define			OPT_F089_FORCE_HASHJOIN 89
/* SET TRACE POINT OP217
** Force the optimiser to use hash joins.
** (Diagnostic use only - requires xDEBUG defined in opjjoinop.c)
*/
#define			OPT_F090_FORCE_EXCHANGE 90
/* SET TRACE POINT OP218
** Force the optimiser to insert EXCH nodes.
*/


/* note that currently only 10 trace points can take values */
#define			OPT_F118_GREEDY_FACTOR	118
/* SET TRACE POINT OP246 <greedy jcost factor>
** adjust ops_cost by this figure when comparing to jcost in opn_corput() for
** greedy enum only. op246 takes an integer percentage value i.e. 75 will
** multiply ops_cost by 0.75.
*/
#define			OPT_F119_NEWENUM	119
/* SET TRACE POINT OP247 [<lookahead code>]
** Force opn to use new enumeration logic (bottom up lookahead.
** It defaults to performing 1-join lookahead (3 tables at a time)
** but non-zero values induce it to perform variations on the theme.
*/
#define			OPT_F120_SORTCPU	120
/* SET TRACE POINT OP248 <cpu reduction factor>
** reduces sort CPU estimates by the supplied value (divides 
** by the value).
*/
#define                 OPT_F122_MEMORY         122
/* SET TRACE POINT OP250 <memory>
** number of bytes of OPF memory which this session can
** use */
#define                 OPT_F123_SCAN		123
/* set trace point op251 <blocking>
** normally on a scan there is a cost of 1 DIO associated
** with an 8 page read, since most of the overhead in a read
** is associated with the system call, rather than the number
** of pages read.  This trace value will override the default
** of 8, but it should only be used to find bugs in the
** optimizer, i.e. if a lower value produces a significantly
** faster plan then it should be reported as a bug 
*/
#define                 OPT_F124_BLOCKING       124
/* set trace point op252 <blocking>
** normally the blocking factor used is 4 pages per heap block
** read on a scan, 1/2 of this for BTREE, and a more involved
** formula for hash and isam, involving overflow pages
** this value will be used to override the default base value
** of 4
*/
#define			OPT_F125_TIMEFACTOR	125
/* set trace point op253 <change> 
** This value will adjust the conversion factor used to
** make "time" units out of "cost" units.  The value used
** will be a percentage change out of 100%, where 100 will
** cause the default to be used and 150% will cause the
** optimizer to look for better plans 50% longer than
** the default
*/
#define		        OPT_F126_SITE_FACTOR    126
/* set trace point op254 factor <dif-site> [<same-site>]
**
** the defaults are 8 for dif-site and 1024 for same-site
** - dif-site is the factor which will be used for distributed
** to determine the cost of moving data between sites, the
** default is 8, but a user may want to change this to 1
** if all nodes are in the same cluster
** - same-site is the cost per byte of transfer between two
** LDBs on the same node, where same-site is divided into 1.0
** and the default is 1.0/1024.0, a higher value means that
** it is relatively less cost to transfer data
*/

#define			OPT_F127_TIMEOUT	127
/* set trace point op255 <plan number> [<subquery number>]
**
** Timeout optimization after <plan number> has been evaluated
** - if <subquery number> is specified then timeout on <plan number>
** only for that subquery
** 
*/

FUNC_EXTERN DB_STATUS opf_call(
	i4		   opcode,
	OPF_CB             *opf_cb);	/* entry point for OPF */
FUNC_EXTERN DB_STATUS opx_call(
	i4		   eventid,
	PTR		   opscb);	/* entry point for cntrl C handler */
FUNC_EXTERN DB_STATUS opt_call(
	DB_DEBUG_CB        *debug_cb);	/* entry point for tracing */
