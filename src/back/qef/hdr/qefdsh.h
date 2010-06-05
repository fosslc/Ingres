/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: QEFDSH.H - Data segment header
**
** Description:
**      The data segment header (DSH) contains the queries runtime
**  environment and a pointer to the query plan (QP).
**
** History: $Log-for RCS$
**      24-mar-86 (daved)
**          written
**	22-oct-87 (puree)
**	    query constant support.
**	11-mar-88 (puree)
**	    remove DSH_QUEUE structure. DMT_CB's can be linked together
**	    using the standard jupiter control block header.
**	31-aug-88 (carl)
**	    added structure QEE_DDB_CB and field dsh_ddb_cb in QEE_DSH
**	19-sep-88 (puree)
**	    remove QEN_STAT. Use QEN_STATUS instead.
**	    move QEN_HOLD FROM QEFNODE.H here.
**	24-dec-88 (carl)
**	    added new fields to QEE_DDB_CB
**	01-jan-89 (carl)
**	    modified QEE_DDB_CB
**	15-jan-89 (paul)
**	    Add QEE_ACTCTX to contain context for nested action lists.
**	    Add DSH structures for nested actions and nested procedures.
**	14-feb-89 (carl)
**	    added new fields to QEE_DDB_CB
**	22-feb-89 (carl)
**	    added new fields to QEE_DDB_CB
**	31-mar-89 (carl)
**	    added qee_d10_tupdesc_p and qee_d11_ldb_desc to QEE_DDB_CB
**	01-apr-89 (paul)
**	    Add status flag to dsh_qp_status. It is now possible for the
**	    system to try to invoke the same QP in a nested fashion requiring
**	    multiple DSH's for the QP. We need to indicate that a DSH is in use.
**	07-apr-89 (carl)
**	    added qee_d12_ldb_rowcnt to QEE_DDB_CB
**	08-may-89 (carl)
**	    added define QEE_06Q_TDS to QEE_DDB_CB
**      28-jul-89 (jennifer)
**          Added status value to dsh_qp_status.  It indicates that
**          the audit inforamtion has been processed for this QP.
**	04-apr-90 (davebf)
**	    overloaded shd_dups
**	04-apr-90 (davebf)   
**	    added hold_medium, unget_status, unget_buffer
**	17-apr-90 (davebf)
**	    added shd_options
**	20-nov-90 (davebf)
**	    Add DSH_CURSOR_OPEN to dsh_qp_status.  Set in qeq_open. Tested
**	    in qea_fetch when no QEP.
**	28-dec_90 (davebf)
**	    added hold_reset_pending, hold_reset_counter, hold_inner_status. 
**	24-jan-91 (davebf)
**	    added dsh_shd_cnt to qee_dsh.
**	27-mar-92 (rickh)
**	    Removed overloading of shd_dups.  For tid hold files, added new
**	    field:  shd_tid.  Added new status word to qen_hold structure
**	    to describe states needed by right fsmjoins.
**	05-may-92 (rickh)
**	    Defined new flags for qen_status.node_access for use with
**	    fsmjoins.
**	10-aug-1992 (rickh)
**	    Added QEE_TEMP_TABLE.
**	28-oct-92 (rickh)
**	    Replaced references to ADT_CMP_LIST with DB_CMP_LIST.
**	30-nov-92 (anitap)
**	    Added pointers to QEF_RCB & QEE_DSH structures in QEE_DSH.
**	02-apr-93 (jhahn)
**	    Added DSH_CT_CONTINUE2 to QEE_ACTCTX for support of statement
**	    level unique indexes.
**	07-jul-93 (anitap)
**	    Added defines for flag used in qea_schema() to differentiate
**	    between explicit and implicit creation of schemas.
**      20-jul-1993 (pearl)
**          Bug 45912.  Add dsh_kor_nor, array of pointers to QEN_NOR's, to
**          QEE_DSH.
**          Changed name of this file from DSH.H to its real name, QEFDSH.H.
**	5-oct-93 (eric)
**	    Added support for resource lists.
**	18-oct-1993 (rickh)
**	    Changed the states in the node_access field to be flags. This is
**	    the first step toward converting KEY and TID JOIN interpreters
**	    into the same model used by FSM, PSM, and CP.  Added a new flag for
**	    KEY JOINS: this flag indicates whether the last outer tuple
**	    managed to key into the inner relation.
**	13-jan-94 (rickh)
**	    Reduced _QEE_VALID to a struct.  As a typedef it generated
**	    acc compiler warnings.
**	18-may-95 (ramra01)
**	    Added a field in the QEN_SHD to handle sort max for the new sort
**	    implementation.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Add DSH_SORT flag to QEE_DSH.
**	06-jul-1998 (somsa01)
**	    Added dsh_dgtt_cb_no to QEE_DSH, to keep track of the global
**	    temporary table in a "declare global temporary table ... as
**	    select ..." situation.  (Bug #91827)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-sep-2000 (hanch04)
**	    fix bad cross integration
**	01-oct-2001 (devjo01)
**	    Renamed QEN_HASH_DUP to QEF_HASH_DUP and moved it from qefdsh.h 
**	    to qefmain.h so that it will be visible to the OPC functions 
**	    which compile the code used to manipulate it.
**	20-nov-03 (inkdo01)
**	    #defines to distinguish opens to qen_func's at root node,
**	    not at root and already open (for parallel query processing).
**	17-mar-04 (toumi01)
**	    add FUNC_CLOSE for exchange processing
**	8-apr-04 (inkdo01)
**	    Added QEF_CHECK_FOR_INTERRUPT macro (copied from back!hdr!
**	    qefcb.h because it now refs QEE_DSH).
**      12-Apr-2004 (stial01)
**          Define dsh_length as SIZE_TYPE.
**	21-Jul-2007 (kschendel) SIR 122513
**	    Generalized partition qualification additions.
**	25-Jan-2008 (kibro01) b116600
**	    Add original dsh_row pointers so if adjusted we can go back
**	    to those allocated here
**      09-Sep-2008 (gefei01)
**          Added node_tproc_dsh and qex_tprocqual for table procedure.
**      01-Apr-2009 (gefei01)
**          Added DSH_TPROC_DSH to dsh_qp_status.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/* The defines for ADE_1x2 are used by many join routines when the result of
** a CX is interpreted. */

#define	    NEW_EQ_OLD	    ADE_1EQ2
#define	    NEW_LT_OLD	    ADE_1GT2
#define	    NEW_GT_OLD	    ADE_1LT2

#define	    OUTER_EQ_INNER  ADE_1EQ2
#define	    OUTER_LT_INNER  ADE_1LT2
#define	    OUTER_GT_INNER  ADE_1GT2

/* The defines for the position to begin scanning an inner node.  Used in
** sort/merge joined routines */

#define	    SCAN_ALL		0
#define	    SCAN_SAVED_TID		1
#define	    SCAN_CURRENT		2

/* The defines to specify whether explicit/implicit creation of schemas. Used
** in qea_schema() to use the control blocks QEF_RCB and QEUQ_CB.
*/

#define	    EXPLICIT_SCHEMA	0
#define     IMPLICIT_SCHEMA     1
#define     IMPSCH_FOR_TABLE    2	

/* The defines for the function parameter passed in all qen_func calls.
** Note: these are bit settings that are cumulative. Specifically, TOP_OPEN
** and FUNC_RESET may appear together.
*/

#define	    NO_FUNC		0
#define	    MID_OPEN		1
#define	    TOP_OPEN		2
#define	    FUNC_RESET		4
#define	    FUNC_ALLOC		8
#define	    FUNC_NEXTROW	16
#define	    FUNC_CLOSE		32
#define	    FUNC_EOGROUP	64
	/* terminate partition group read - on to next group.
	** EOGROUP says to flush out any in-progress results from this
	** partition group, and prepare to start a new one.  The higher
	** level PC-node (eg a PC-join) will follow up with a RESET
	** as well (since every partition group begins with a RESET).
	** EOGROUP followed by EOGROUP should end the current group
	** and skip the next one.  EOGROUP followed by RESET should end
	** the current group and prepare for the next one.
	*/

/* On interrupt set interrupt_handled condition, set error condition and
** evaluate to E_DB_ERROR otherwise evaluate to E_DB_OK.
*/
#define QEF_CHECK_FOR_INTERRUPT(qef_cb, dsh) \
    ((((QEF_CB*)(qef_cb))->qef_user_interrupt && \
      !((QEF_CB*)(qef_cb))->qef_user_interrupt_handled)? \
     ( \
      ((QEE_DSH*)(dsh))->dsh_error.err_code = E_QE0022_QUERY_ABORTED, \
      E_DB_ERROR \
     ): \
      E_DB_OK)
      
typedef struct _QEE_DSH	    QEE_DSH;


/*}
** Name: QEE_VLT - descriptor for ADF VLUPs and VLTs
**
** Description:
**
** History:
[@history_line@]...
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warnings.
*/

typedef struct _QEE_VLT	    
{
    PTR		vlt_streamid;	/* memory stream id for this VLT */
    i4		vlt_size;	/* size of the VLT buffer */
    PTR		vlt_ptr;	/* address of the VLT buffer */
}	QEE_VLT;


/*}
** Name: QEN_SHD -	In memory Sort and Hold stream descriptor
**
** Description:
**	The arhitecture of QEF in-memory sort/hold data structure is depicted
**	below:
**		
**
**      QEN_SHD
**	+-----+
**      |     |
**      +-----+
**      |     |	      sort_array
**      +-----+       +--------+
**      |     |------>|    ptr |.....    tuple buffer
**      +-----+       +--------+         +-----------+
**                    |    ptr |-------->|           |
**                    +--------+         +-----------+
**		      |        |.....     individual tuple buffer,
**		      +--------+	  may not be contiguous.
**				 
**				 
**
** History:
**	04-apr-90 (davebf)
**	    overloaded shd_dups
**	17-apr-90 (davebf)
**	    added shd_options
**	27-mar-92 (rickh)
**	    Removed overloading of shd_dups.  For tid hold files, added new
**	    field:  shd_tid.
**	18-may-95 (ramra01)
**	    Added a new field to keep the tup max count. Initially this will
**	    be used to maintain the max number of sort tuples that the node
**	    can support in memory before spilling to disk.
**	    At a later time will use this to reimplement reusablilty of HOLD
**	    files as in FSM join node processing.
**	24-nov-97 (inkdo01)
**	    Added free chain ptr, so that buffers may be alloc'ed numerous
**	    at a time, thus significantly reducing ULM calls.
*/
typedef struct _QEN_SHD     
{
    QEN_NODE	*shd_node;	/* ptr to QEN that owns this buffer */
    PTR		shd_streamid;	/* memory stream id for this buffer */
    i4	shd_size;	/* total buffer size for this node */
    PTR		*shd_vector;	/* address of the sort vector (array of
				** pointers to tuples in sort buffer */
    DM_MDATA	*shd_mdata;	/* ptr to list of DM_MDATAs for the tuples */
    i4		shd_tup_cnt;	/* total number of tuples */
    i4		shd_next_tup;	/* the next tuple to be fetched */
    PTR		shd_row;	/* addr of row buffer for the tuple */
    i4		shd_width;	/* the size of the row/tuple */
    i4	shd_options;    /* bit array for special options */
#define SHD1_DSH_POOL	    0X00000001	    /* memory stream from dsh pool   
					    ** bit off indicates sort pool */
    i4	shd_dups;	/* for sort: DMR_NODUPLICATES, or 0 otherwise 
                                ** for hold: eof tuple index: last append + 1   
				*/
    u_i4        shd_tid;        /* at insert time:  highest tid stored in tid
				** holdfile.  at rescan time, highest tid
				** yet read from tid holdfile. */
    i4		shd_tup_max;	/* only for the sort node */
    DM_MDATA	*shd_free;	/* anchor of list of available buffers */
}	QEN_SHD;


/*}
** Name: QEN_OJMASK  - mask field for hash outer joins
**
** Description:
**  A flag field describing OJ requirements of and OJs returned to
**  a hash join.
*/
typedef i4	QEN_OJMASK;
#define OJ_HAVE_LEFT	0x1	/* left join returned */
#define OJ_HAVE_RIGHT	0x2	/* right join returned */


/*}
** Name: QEN_HASH_BASE	- base structure for hash join workareas
**
** Description:
**
** History:
[@history_line@]...
**	3-mar-99 (inkdo01)
**	    Written.
**	jan-00 (inkdo01)
**	    Changes for bit filters, cross product logic.
**	21-aug-01 (inkdo01)
**	    Added HASH_CHECKNULL flag to eliminate "null = null" joins.
**	7-sep-01 (inkdo01)
**	    Added HASH_PROBE_STORED to delay OJ processing for probe rows
**	    whose corr. build is on disk.
**	9-aug-02 (inkdo01)
**	    Added hsh_count1/2/3/4 for debugging purposes.
**	13-jan-03 (inkdo01)
**	    Added 2nd work buffer for enhanced CP algorithm.
**	28-aug-04 (inkdo01)
**	    Added hsh_brptr, hsh_prptr for global base array changes.
**	5-apr-05 (inkdo01)
**	    Added hsh_prevpr for OJ indicator handling.
**	3-Jun-2005 (schka24)
**	    Remove unused subpartition thing;  add another list for
**	    hash-file entries that are unused and available.  Re-type
**	    hashtable to more natural QEF_HASH_DUP **.
**	22-Jun-2005 (schka24)
**	    Added I/O stats, repartition counts.
**	14-Nov-2005 (schka24)
**	    Eliminate STORED flag, remove prevpr, keep probe-side OJ indicator
**	    right here as the NOJOIN flag.
**	7-Dec-2005 (kschendel)
**	    Remove key-list pointer, it's in the node now.
**	5-Aug-2009 (kschendel) SIR 122512
**	    Merge Datallegro hash additions: separate read and write buffer
**	    sizes, add a hash page-size, various additions for the new hash
**	    reservation framework, add RLL-compression-allowed flags.
**	    Make total row counts i8's since they can be very large.
**	    (doesn't really address the many-rows problem but it's a start.)
**	    Change node pointer to point to full node, not just the hash-
**	    join specific part -- access to node stuff is handy for debugging.
*/

/* Hash join and aggregation use various buffer sizes for reading and
** writing, as memory permits.  (For instance, it might be useful to have
** a large read buffer to assist in sequential reading.)  Since the
** bottommost DI layer expects I/O in terms of "pages", define a page size
** that determines the minimum buffer size for hash files.  All hash file
** buffer sizes MUST be a multiple of this basic page size.
*/

#define HASH_PGSIZE	8192	/* Basic hash file page size for DMH / DI */


typedef struct _QEN_HASH_FILE	QEN_HASH_FILE;

typedef struct _QEN_HASH_BASE
{
    QEN_NODE	*hsh_nodep;	/* ptr to hash join node */
    struct _QEN_HASH_LINK  *hsh_toplink;  /* ptr to top partition 
				** recursion level structure */
    struct _QEN_HASH_LINK  *hsh_currlink;  /* ptr to current partition
				** recursion level link structure */
    struct _QEN_HASH_FILE {
      QEN_HASH_FILE	*hsf_nextf;  /* next file on free chain */
      QEN_HASH_FILE	*hsf_nexto;  /* next file on open chain */
      PTR	hsf_file_context;  /* file context for free file */
      u_i4	hsf_filesize;	/* Written filesize in pages */
      u_i4	hsf_currbn;	/* Next block to read or write */
      i2	hsf_locidx;	/* file location index */
      bool	hsf_isbuild;	/* TRUE if build file (for stats) */
    }		*hsh_freefile;  /* ptr to chain of allocated but
				** available partition files */
    /* Note: the openfiles list will include files on the freefile list.
    ** A "free" file still exists and is still open at the low (dmh) level.
    */
    struct _QEN_HASH_FILE  *hsh_openfiles;  /* list of alloced/opened files */
    struct _QEN_HASH_FILE  *hsh_availfiles;  /* Available unused file structs */
    struct _QEN_HASH_CARTPROD *hsh_cartprod;	/* ptr to cart prod struct */
    PTR		hsh_dmhcb;	/* ptr to DMH_CB for partition file ops */

    /* Rows can't split memory buffers when built into a hash table.  This
    ** introduces inexactitude into the hash size calculations, and if we're
    ** unlucky we might need some extra memory for the last few rows.
    ** Extra space is of size hsh_csize+sizeof(pointer), linked thru a
    ** leading pointer, and there are hsh_extra_count of them.
    */
    PTR		hsh_extra;	/* Start of extra hash-table buffers */

    /* Next three used during concurrency calculations, not used at runtime */
    SIZE_TYPE	hsh_concur_size;  /* Reservation of concurrent hashops */
    i4		hsh_concur_num;	/* Number of possibly concurrent hashops */
    i4		hsh_orig_slice;	/* Original 1/N slice for reservation */

    i4		hsh_msize;	/* overall hash table memory size */
    u_i4	hsh_pcount;	/* count of hash partitions */
    u_i4	hsh_csize;	/* cluster (block) size, ie write size */
    u_i4	hsh_rbsize;	/* Read buffer size */
    u_i4	hsh_pagesize;	/* Page size for overflow files; always a
				** multiple of HASH_PGSIZE and a common
				** factor of both hsh_csize and hsh_rbsize.
				** The reason for wanting a larger page size
				** is for (optional) page compression.
				*/
    i4		hsh_bvsize;	/* bit vector size (in bytes) */
    i4		hsh_htsize;	/* hash table size */
    i4		hsh_extra_count;  /* Number of "extra" hash buffers */
    PTR		hsh_streamid;	/* hash storage stream ID */
    QEF_HASH_DUP **hsh_htable;	/* hash table array base address.  The
				** hash table is an array of QEF_HASH_DUP *
				*/
    QEF_HASH_DUP *hsh_brptr;	/* original build row buffer ptr */
    QEF_HASH_DUP *hsh_prptr;	/* original probe row buffer ptr */
    PTR		hsh_rbptr;	/* pointer to partition cluster read buffer */
    QEF_HASH_DUP *hsh_workrow;	/* pointer to buffer for row assembly */
    QEF_HASH_DUP *hsh_workrow2;	/* pointer to 2nd buffer for row assembly,
				** used for rll compression, also when
				** allsame partition requires cart-prod */
    QEF_HASH_DUP *hsh_firstbr;	/* ptr to first build row of a set with
				** same keys */
    QEF_HASH_DUP *hsh_nextbr;	/* ptr to next build row of the set */
    i8		hsh_brcount;	/* Total build row count */
    i8		hsh_prcount;	/* Total probe row count */
    i4		hsh_kcount;	/* count of key columns */
    i4		hsh_keysz;	/* size (in bytes) of key columns */
    i4		hsh_browsz;	/* size (bytes) of build row (uncompressed) */
    i4		hsh_prowsz;	/* size (bytes) of probe row (uncompressed) */
    i4		hsh_min_browsz;	/* Smallest build row (after compression) */
    i4		hsh_min_prowsz;	/* Smallest probe row (after compression) */
    i4		hsh_currpart;	/* current probe partition (work field) */
    i4		hsh_currows_left; /* rows left in curr probe part. */
    i4		hsh_currbucket; /* current hash table bucket - for ojs */
    QEN_OJMASK	hsh_pojmask;	/* Precomputed probing OJ_HAVE_xxx mask for
				** outer join for probe1/2 loops */
    i4		hsh_size;	/* memory alloc'ed from hash pool */
    bool	hsh_reverse;	/* Current pass uses role reversal.
				** (This used to be in hsh_flags but invariably
				** the flag was used to compute a bool.  So
				** it's easiest to just use a bool as almost
				** all hashjoin routines need it.)
				*/
    i4		hsh_flags;	/* flag word */
#define HASH_PROBE_MATCH	0x1	/* This probe row has more matches */
#define HASH_BUILD_OJS		0x2	/* processing hash table oj's */
#define HASH_NOHASHT		0x4	/* last p build produced no hasht */
#define HASH_NO_BUILDROWS	0x8	/* build source was empty */
#define HASH_PROBE_NOJOIN	0x10	/* probe has never joined */
#define HASH_WANT_LEFT		0x20	/* query is left/full join */
#define HASH_WANT_RIGHT		0x40	/* query is right/full join */
#define HASH_CARTPROD		0x80	/* current partition pair requires
					** cross product (all keys sre same) */
#define HASH_BETWEEN_P1P2	0x100	/* done phase1, not started real phase2
					** (may need to flush probe buffs built
					** in phase1) */
#define HASH_CHECKNULL		0x200	/* at least one key col is nullable - 
					** precheck for null before probe match */
#define HASH_CO_KEYONLY		0x400	/* Set for (CO) joins with no post-join
					** qualification.  Enables a small
					** optimization where we can throw away
					** a build row with an identical join
					** key to one that's already there.
					*/

    /* The run-length compression-allowed flags are set if it appears that
    ** the non-key portion of the build or probe side is large enough to
    ** benefit from simple run-length compression.  Note that these are
    ** only "compression allowed" flags, each row has its own flag saying
    ** whether compression actually occurred.  (i.e. we don't want to make
    ** incompressible rows even larger than they were.)
    **
    ** OPC could make this decision, but at the moment it's qee that
    ** decides, for no special reason.
    */
#define HASH_BUILD_RLL		0x800	/* RLL-compression attempt allowed
					** for build (left) side rows.
					*/
#define HASH_PROBE_RLL		0x1000	/* RLL-compression attempt allowed
					** for probe (right) side rows.
					*/
#define HASH_FAST_PROBE2	0x2000	/* Vanilla probe2 partition, use the
					** fast-path to avoid most of the
					** inapplicable probe2 logic
					*/

    /* Define flags that reflect the join attributes rather than the
    ** instant runtime state -- these flags are preserved across resets.
    */
#define HASH_KEEP_OVER_RESET (HASH_WANT_LEFT | HASH_WANT_RIGHT \
		| HASH_CHECKNULL | HASH_BUILD_RLL | HASH_PROBE_RLL)

    /* Only one of parent-node or parent-act will be filled in, and maybe
    ** neither one.  Not used at the moment, but the idea is that when
    ** this join terminates and releases memory, the next hash-op up can
    ** maybe use the space.
    */
    QEN_NODE	*hsh_parent_node;	/* Next hjoin up, if we're its probe */
    QEF_AHD	*hsh_parent_act;	/* or, maybe top is a hashagg */

    /* Counters that follow are for stats, and are nonessential */
    u_i4	hsh_jcount;	/* count of rows joined by hash;  differs from
				** node rcount by post-qual (jqual) rejects
				** and oj rows */
    i4		hsh_ccount;	/* count of partitions joined with 
				** Cartesian product logic */
    i4		hsh_fcount;	/* count of created partition files */
    u_i4	hsh_bwritek;	/* kb written from build side */
    u_i4	hsh_pwritek;	/* kb written from probe side */
    i4		hsh_memparts;	/* # partitions built from memory (unspilled) */
    i4		hsh_rpcount;	/* # of repartitions */
    i4		hsh_bvdcnt;	/* count of rows discarded by bit filter */
    i4		hsh_maxreclvl;	/* max depth of recursion */

    i8		hsh_count1;		/* Spare counters usable for debugging */
    i8		hsh_count2;
    i4		hsh_count3;
    i4		hsh_count4;
}	QEN_HASH_BASE;

/*}
** Name: QEN_HASH_LINK	- hash partition recursion link structure
**
** Description:
**	Represents one recursion level in overflow partitioning scheme. Used both 
**	for hash joins and hash aggregation. When used for the latter, there
**	is only one source (no build/probe distinction) and by convention, 
**	the "b" variables and flags are used (e.g. hsl_brcount is the number
**	of aggregation source rows overflowed at this level).
**
** History:
[@history_line@]...
**	26-mar-99 (inkdo01)
**	    Written.
**	4-Jun-2009 (kschendel) b122118
**	    Delete rowcounts, not used at the link structure level.
*/

typedef struct _QEN_HASH_LINK
{
    struct _QEN_HASH_LINK  *hsl_next;	/* ptr to nested recursion link */
    struct _QEN_HASH_LINK  *hsl_prev;	/* ptr to outer recursion link */
    struct _QEN_HASH_PART  *hsl_partition;  /* ptr to partition descriptor
				** array for this recursion level */
    i4		hsl_level;	/* recursion nesting level */
    i4		hsl_pno;	/* partition no. being processed */
    i4		hsl_flags;	/* flag word */
#define HSL_BOTH_LOADED 0x1	/* both sources have been loaded into
				** partitions at this level */
}	QEN_HASH_LINK;

/*}
** Name: QEN_HASH_PART	- hash partition table entry
**
** Description:
**	Represents one partition of overflow rows in a hash join or hash
**	aggregation. Anchors file structures used to offload partitions to
**	disk (if necessary). When used for hash aggregation, there is only 
**	one source (as noted above) and by convention, the "b" variables and
**	flags are used (e.g. hsp_brcount, HSP_BSPILL).
**
**	Build and probe rows both spill into the same file;  all of one
**	side, then all of the other.  Some of the info carried around
**	for spillage is a bit redundant, but it makes the calculations
**	easier.
**
** History:
[@history_line@]...
**	3-mar-99 (inkdo01)
**	    Written.
**	jan-00 (inkdo01)
**	    Changes for bit filters, cross product logic.
**	6-May-2005 (schka24)
**	    Hash values must be unsigned.
**	3-Jun-2005 (schka24)
**	    Remove unused subpartition ptr.
**	31-may-07 (hayke02 for schka24)
**	    Split HCP_BOTH into HCP_IALLSAME (inner ALLSAME) and HCP_OALLSAME
**	    (outer ALLSAME). This change fixes bug 118200.
**	11-Jun-2008 (kschendel) SIR 122512
**	    Combine build and probe files: add b/pstarts_at, size.
**	18-Jun-2008 (kschendel) SIR 122512
**	    More additions for spill tracking when RLL-compressed.
**	    Make row counts unsigned for a smidgen more range.
*/

typedef struct _QEN_HASH_PART
{
    PTR		hsp_bptr;	/* block pointer */
    QEN_HASH_FILE  *hsp_file;	/* ptr to file context for partition spill */
    PTR		hsp_bvptr;	/* bit vector pointer -- note that hash build
				** code requires that bit vector immediately
				** follows the partition buffer! */
    i8		hsp_bbytes;	/* Build total bytes */
    i8		hsp_pbytes;	/* Probe total bytes */
    i4		hsp_offset;	/* offset to next entry in block */
    u_i4	hsp_brcount;	/* build partition row count */
    u_i4	hsp_prcount;	/* probe partition row count */
    u_i4	hsp_bstarts_at;	/* Page number that build spill starts at */
    u_i4	hsp_pstarts_at;	/* Page number that probe spill starts at */
    u_i4	hsp_buf_page;	/* First page number currently in the partition
				** buffer when partitioning, used for the hash
				** join don't-spill-last-buffer optimization
				*/
    u_i4	hsp_prevhash;	/* hash key of previous row */
    i4		hsp_flags;	/* flag word */
#define HSP_BSPILL	0x1	/* build partition has spilled to disk */
#define HSP_PSPILL	0x2	/* probe partition has spilled to disk */
#define HSP_BINHASH	0x4	/* build partition is in hash table */
#define HSP_PINHASH	0x8	/* probe partition is in hash table */
#define HSP_BFLUSHED	0x10	/* build file hash been flushed */
#define HSP_PFLUSHED	0x20	/* probe file hash been flushed */
#define HSP_BALLSAME 	0x40	/* all build rows have same hashkey */
#define HSP_PALLSAME 	0x80	/* all probe rows have same hashkey */
#define HSP_DONE	0x100	/* partition has been processed */
#define HSP_BCANDIDATE	0x200	/* Spilled partition might fit into hasht */
#define HSP_PCANDIDATE	0x400	/* Probe side might fit into hasht */
}	QEN_HASH_PART;


/*}
** Name: QEN_HASH_CARTPROD	- structure for cart prods in hash joins	
**
** Description:	The ugliest hash join may have an embedded cartesian product
**	which occurs when the rows of a whole partition have the same hash
**	number, and repartitioning won't change it. A pair of partitions in
**	which one displays this characteristic must be cross producted. This
**	structure contains elements not required for more benign hash joins.
**
**	There are separate copies of a bunch of build vs probe things, because
**	the cartesian product inner/outer might be the reverse of the
**	hash partition build/probe.  For the cart-prod, "build" is the
**	cart-prod outer, and "probe" is the cart-prod inner.
**
** History:
[@history_line@]...
**	10-jan-00 (inkdo01)
**	    Written.
**	13-jan-03 (inkdo01)
**	    Changes for enhanced cross product algorithm.
**	6-May-2005 (schka24)
**	    Hashes must be unsigned.
**	17-Jul-2006 (kschendel)
**	    Don't need ALLOJ flag, ONLYOJ does the job.
**	5-Apr-2007 (kschendel)
**	    Remove browsleft, not used.  Add flag for probe OJ indicator.
**	5-Aug-2009 (kschendel) SIR 122512
**	    Separate read/write buffer sizes; don't need row sizes;
**	    move of OJ indicator to header requires more hoops-through-jumping
**	    when probe OJ row split across buffer loads.
*/

typedef struct _QEN_HASH_CARTPROD {
    PTR		hcp_bbptr;	/* ptr to build source file buffer */
    PTR		hcp_pbptr;	/* ptr to probe source file buffer */
    QEN_HASH_PART *hcp_partition;  /* ptr to cartprod partition descriptor */
    QEN_HASH_FILE *hcp_file;	/* ptr to spill file descriptor */
    u_i4	hcp_hashno;	/* hashno of CARTPROD partition */
    i4		hcp_boffset;	/* buffer offset to current outer row */
    i4		hcp_poffset;	/* buffer offset to current inner row */
    u_i4	hcp_browcnt;	/* count of outer rows of cartprod */
    u_i4	hcp_prowcnt;	/* count of inner rows of cartprod */
    u_i4	hcp_prowsleft;	/* count of inner rows remaining this pass*/
    u_i4	hcp_rwsize;	/* I/O size to use, smaller of hash read
				** and write buffer size since we use a
				** write buf and the read buf for I/O.
				** (no longer strictly necessary, but using
				** a single io size simplifies the code a
				** very little bit.)
				*/
    u_i4	hcp_bstarts_at;	/* Page number that outer spill starts at */
    u_i4	hcp_pstarts_at;	/* Page number that inner spill starts at */
    u_i4	hcp_bcurrbn;	/* Track file position for outers */
    u_i4	hcp_pcurrbn;	/* Track file position for inners */
    i4		hcp_flags;	/* flag field */
#define HCP_FIRST	0x1	/* haven't processed first call yet */
#define HCP_REVERSE	0x2	/* cart prod with role reverse */
#define HCP_IALLSAME	0x4	/* Inner source is ALLSAME */
#define HCP_OALLSAME	0x8	/* Outer source is ALLSAME */
#define HCP_ONLYOJ	0x10	/* just outer joins left */
#define HCP_READ_OUTER	0x20	/* need to read a new "outer" */
#define HCP_COPY_WORK_OJ 0x40	/* Probe OJ indicator is in workrow2, needs
				** copied back to disk buffer */
    /* For use when writing back changed OJ indicator into previous buffer
    ** when row is split across bufferfuls (i.e. copy-work-OJ is set)
    */
    i4		hcp_pojoffs;	/* poffset where row started */
    i4		hcp_pojrbn;	/* Buffer block # holding start of row */
}	QEN_HASH_CARTPROD;


/*}
** Name: QEN_HASH_AGGREGATE	- root structure for hash aggregate processing
**
** Description:	This structure is allocated to execute a hash aggregate. It
**	anchors all other structures for the aggregation and contains
**	other various and sundry counters, size values, etc. It is allocated
**	and formatted in qee_cract, but used by qea_haggf.
**
** History:
[@history_line@]...
**	26-jan-01 (inkdo01)
**	    Written.
**	23-feb-01 (inkdo01)
**	    Additions for (partitioned) overflow handling.
**	27-mar-01 (inkdo01)
**	    Add hshag_pcount (partition count) with (for now) fixed value
**	    of 17.
**	5-Aug-2009 (kschendel) SIR 122512
**	    Merge datallegro additions: separate read/write buffer sizes;
**	    add hash-file CB avail list so we don't lose context blocks
**	    allocated for inner recursion levels; stuff for new hash
**	    reservation framework.  Add max-chunks-allocated, for stats.
**	    Htmax isn't the hash table max, it's the group count max.
**	    Rename to fit the function.
*/

typedef struct _QEN_HASH_AGGREGATE {
    PTR		*hshag_htable;	/* ptr to hash table */
    PTR		hshag_streamid;	/* hash aggregation memory stream ID */
    PTR		hshag_1stchunk;	/* ptr to first chunk */
    QEN_HASH_LINK *hshag_toplink; /* ptr to top partition recursion level */
    QEN_HASH_LINK *hshag_currlink; /* ptr to current partition recursion
				** level */
    QEN_HASH_FILE *hshag_openfiles; /* list of opened overflow files */
    QEN_HASH_FILE *hshag_freefile; /* anchor of free (but alloc'ed) files */
    QEN_HASH_FILE *hshag_availfiles; /* available not-in-use file CB's */
    PTR		hshag_dmhcb;	/* ptr to DMH_CB for overflow file ops */
    QEF_HASH_DUP *hshag_lastrow; /* ptr to last work row processed
				** (during FINIT cycle) */
    DB_CMP_LIST	*hshag_keylist;	/* ptr to list of hash key descriptors */
    PTR		hshag_workrow;	/* pointer to buffer for overflow
				** work row assembly */
    PTR		hshag_workrow2;	/* pointer to buffer for overflow
				** work row (de)compression */
    PTR		hshag_rbptr;	/* overflow file read buffer ptr */
    SIZE_TYPE	hshag_estmem;	/* Total estimated worst case memory usage */
				/* (i.e. hashagg reservation) */

    /* Next three used during concurrency calculations, not used at runtime */
    SIZE_TYPE	hshag_concur_size;  /* Reservation of concurrent hashops */
    i4		hshag_concur_num; /* Number of possibly concurrent hashops */
    i4		hshag_orig_slice; /* Original 1/N slice for reservation */

    u_i4	hshag_rbsize;	/* Overflow file read buffer size */
    u_i4	hshag_pagesize;	/* Page size for overflow files; always a
				** multiple of HASH_PGSIZE and a common
				** factor of both pbsize and rbsize.
				** The reason for wanting a larger page size
				** is for (optional) page compression.
				*/
    i4		hshag_htsize;	/* count of hash table buckets */
    i4		hshag_grpmax;	/* maximum number of result groups that will
				** fit in total chunks */
    i4		hshag_chsize;	/* size of work storage chunk */
    i4		hshag_chmax;	/* max no. of chunks allowed */
    i4		hshag_challoc;	/* Actual no. of chunks allocated */
    u_i4	hshag_pbsize;	/* partition buffer/block (write) size */
    i4		hshag_halloc;	/* total hash pool memory alloc'ed */
    i4		hshag_wbufsz;	/* size of agg. work buffer row */
    i4		hshag_hbufsz;	/* size of agg. hash buffer row */
    i4		hshag_obufsz;	/* size of agg. overflow buffer row */
    i4		hshag_rcount;	/* count of rows input to aggregation */
    i4		hshag_gcount;	/* count of groups (output rows) */
    i4		hshag_ocount;	/* count of overflow rows */
    i4		hshag_fcount;	/* count of created overflow files */
    i4		hshag_pcount;	/* number of partitions per recursion level */
    i4		hshag_curr_bucket;  /* no. of bucket currently being
				** processed (during FINIT cycle) */
    i4		hshag_keycount;	/* count of hash keys (group exprs) */
    i4		hshag_keysz;	/* size (in bytes) of hash-key string */
    i4		hshag_maxreclvl;  /* Max recursion level for stats */
    i4		hshag_flags;	/* flag field */
#define HASHAG_LOAD1	1	/* load phase 1 (initial) in progress */
#define HASHAG_LOAD2P1	2	/* phase 2 load of non-spilled partitions */
#define HASHAG_LOAD2P2	4	/* phase 2 load of spilled partitions */
#define HASHAG_OVFLW	8	/* overflow processing required */
#define	HASHAG_RECURSING 0x10	/* a LOAD2P2 execution is doing recursive 
				** partitioning */
#define	HASHAG_RETURN	0x20	/* output phase in progress */
#define HASHAG_EOG	0x40	/* End-of-group reached during a partition
				** compatible hash aggregation.  EOG stays
				** on until the agg is flushed out and is
				** reset for the next partition group from
				** below.
				*/
#define HASHAG_DONE	0x80	/* all done */
#define HASHAG_RLL	0x100	/* Disk spill should try RLL compression */

    /* Define flags that reflect the hashagg attributes rather than the
    ** instant runtime state -- these flags are preserved across resets.
    */
#define HASHAG_KEEP_OVER_RESET (HASHAG_RLL)

}	QEN_HASH_AGGREGATE;


/*}
** Name: QEA_FIRSTN	- structure to house offsetn/firstn info
**
** Description:	This structure contains dynamic variables used in the 
**	processing of queries with offset "n"/first "n" parameters. 
**	Because a dbproc can have multiple SELECTs with firstn specs,
**	this stuff needs to be tracked at the action header level rather
**	than globally for an executing query. So one of these is allocated
**	and maintained for each GET/FOR action header with a firstn spec.
**
** History:
**	13-dec-2007 (dougi)
**	    Written.
*/

typedef struct _QEA_FIRSTN {
    i4		get_offsetn;	/* offset "n" value for this execution */
    i4		get_firstn;	/* first "n" value for this execution */
    i4		get_count;	/* count of rows returned so far */
}	QEA_FIRSTN;


/*}
** Name: QEN_STATUS  - status block for QEF processing nodes
**
** Description:
**      The QEN_STATUS structure is used to contain the runtime
**      status for a processing node. The information in here
**      changes on a per row basis.
**
**	All QEN_STATUS blocks are cleared en masse at the start of
**	query execution in qee.
**
** History:
**	19-sep-88 (puree)
**	    Adopted from QEN_STAT with the following fields renamed:
**		stat_status	->  node_status
**		stat_unget	removed, use hold_buffer_status
**		stat_indone     ->  node_inner_status
**		stat_ptr	->  node_ahd_ptr
**		stat_rcount	->  node_rcount
**	04-apr-90 (davebf)
**	    added node_access, node_outer_status, node_hold_stream 
**	05-may-92 (rickh)
**	    Defined new flags for qen_status.node_access for use with
**	    fsmjoins.
**	18-oct-1993 (rickh)
**	    Changed the states in the node_access field to be flags. This is
**	    the first step toward converting KEY and TID JOIN interpreters
**	    into the same model used by FSM, PSM, and CP.  Added a new flag for
**	    KEY JOINS: this flag indicates whether the last outer tuple
**	    managed to key into the inner relation.
**	23-jul-98 (inkdo01)
**	    Added QEN_READ_PREV flag to node_access for topsort-less 
**	    descending sort.
**	2-mar-99 (inkdo01)
**	    Added flags for hash joins (QEN13_PROBE_P1, QEN14_PROBE_P2).
**	20-nov-03 (inkdo01)
**	    Added node_open_status for parallel query processing.
**	2-jan-04 (inkdo01)
**	    Changed node_open_status to node_status_flags to allow bit
**	    flags rather than one value at a time (what a crock!). Added
**	    node_part_xxx's to track partition numbers in qen_orig().
**	    Finally, union'ed some of the elements to cap structure size.
**	17-mar-04 (toumi01)
**	    add QEN8_NODE_CLOSED for exchange processing
**	1-apr-04 (inkdo01)
**	    Added QEN16_ORIG_FIRST for parallel processing.
**	21-apr-04 (inkdo01)
**	    Added node_sort_flags to return partition group info.
**	27-apr-04 (inkdo01)
**	    Added QEN32_PGROUP_DONE for combo PC joins/partition quals.
**	14-Jul-2004 (schka24)
**	    Remove unused kjoin node-access flags.
**	    Add kjoin stuff for partitioned kjoin inners.
**	21-july-04 (inkdo01)
**	    Add QEN64_ORIG_NORESET to help guide reset/partition group
**	    logic in qenorig.c.
**	6-sep-04 (inkdo01)
**	    Add node_outer_count to refine group merge join logic.
**	18-Jul-2005 (schka24)
**	    Move physical partition number to generic area.
**	17-Aug-2005 (schka24)
**	    More flags, orig status stuff to support orig partition
**	    qualification on physical partitions, early hash join exit at
**	    inner/outer EOF.
**	27-Jul-2007 (kschendel) SIR 122513
**	    Remove groupmap, is a dsh row now.
**	    Add stupid orig-needs-open flag until this whole open vs
**	    reset vs fetch nonsense is straightened out more.
**	9-Jan-2008 (kschendel) SIR 122513
**	    Remove kjoin nomatch-bits, K-joins use the standard partition
**	    qualification framework now (finally).
**	20-may-2008 (dougi)
**	    Add substructure for table procedures.
**	30-Jun-2008 (kschendel) b122118
**	    Add fast-getnext flags for orig.
*/
typedef struct _QEN_STATUS  
{
    i4          node_status;	/* current state of this node */
#define	QEN0_INITIAL		    0
	/* a new or reset node, has not been executed since */
#define QEN1_EXECUTED		    1
	/* the node has been executed */
#define	QEN2_NEED_NEW_OUTER	    2
	/* need a new outer tuple */
#define QEN3_GET_NEXT_INNER	    3
	/* continue with the next inner tuple */
#define QEN4_NO_MORE_ROWS	    4
	/* no more rows from this node */
#define	QEN6_SUCCEEDED		    6
	/* the last join succeeded */
#define	QEN7_FAILED		    7
	/* the last join failed */
#define QEN_ORIG_FASTNEXT	8	/* Ask for fast get-next in ORIG */

#define QEN20_EOF_NOJOIN	20
	/* the last join failed and the inner is at EOF */
#define	QEN21_EOF_JOINED	21
	/* the last join succeeded and the innerr is at EOF */
#define	QEN22_LT_NOJOIN		22
	/* the last join failed and the inner is not at EOF */
#define	QEN23_LT_JOINED		23
	/* the last join succeeded and the inner is not at EOF */

    i4          node_access;	/* sequence information */
	/* Notused for T-joins */
	/* For K-joins:  node_access remembers whether an outer has
	** joined, and whether we need a new outer row on the next call.
	** (Note that K-joins can only do left outer joins.)
	** At most one of these flags may be on at any given time.
	*/
#define QEN31_IJ_OUTER          1
	/* last return was inner join,  next get new outer */
#define QEN32_OJ_OUTER          2
	/* last return was outer join,  next get new outer */
#define QEN33_IJ_INNER          4
	/* last return was inner join,  next get new inner */

#define	QEN_JOIN_STATE_MASK	( QEN31_IJ_OUTER |	\
				  QEN32_OJ_OUTER |	\
				  QEN33_IJ_INNER )

	/*
	** The following macro sets the KEY or TID join state to one of
	** the above states
	*/

#define	QEN_SET_JOIN_STATE( flagsWord, newJoinState )	\
	{ flagsWord = ( flagsWord & ~QEN_JOIN_STATE_MASK ) | newJoinState; }

	/* More kjoin/tjoin access state flags... */

#define QEN_OUTER_NEW_KEY	0x08
	/* This outer has a different key from the previous outer */

#define QEN_KEY_IN_SOME_PART	0x10
	/* This key has been found in some partition; irrelevant for
	** unpartitioned inners.  If a key is not-found in ALL partitions,
	** we'll light the "key won't probe" flag as a better hint.
	*/

#define	QEN_THIS_KEY_WONT_PROBE	0x40
	/* the last outer tuple failed to key into the inner relation */
	/* For a partitioned inner, this key is not in any partition. */

	/*
	** The above values are used for Key joins.
	** Instead, the following flags in node_access are used for merge
	** joins:
	*/
#define	QEN_READ_NEXT_OUTER		1
#define	QEN_READ_NEXT_INNER		2
#define	QEN_LOOKING_FOR_RIGHT_JOINS	4
#define	QEN_OUTER_HAS_JOINED		8
#define	QEN_RESCAN_MARKED		0x10
#define QEN_TID_FILE_EOF		0x20

	/*
	** To complete the confusion, the following flag in node_access is 
	** used in ORIG nodes, to guide topsort-less descending sorts.
	*/
#define QEN_READ_PREV			1

    i4		node_status_flags;    /* bit flags for accumulating info
				     ** (introduced during parallel query
				     ** implementation) */
#define	QEN1_NODE_OPEN		     1
	/* node has been opened (currently relevant for exchange nodes) */
#define QEN2_OPART_END		     2
	/* current outer has reached end of partition grouping, NOT
	** end of file */
#define QEN4_IPART_END		     4
	/* current inner has reached end of partition grouping, NOT
	** end of file */
#define	QEN8_NODE_CLOSED	     8
	/* node has been close (currently relevant for exchange nodes) */
#define	QEN16_ORIG_FIRST	     0x0010
	/* ORIG node has been opened but not executed for 1st time */
#define QEN32_PGROUP_DONE	     0x0020
	/* partition group is finished - possibly because no 
	** qualification bits are on in this group. */
#define QEN64_ORIG_NORESET	     0x0040
	/* end of a partition group in qen_orig call. Distinguishes 
	** reset calls to qen_orig that start at beginning of whole table
	** from resets that just start the next partition group. */
#define QEN128_PGROUP_SETUP	     0x0080
	/* Orig is doing grouped access, next call to read a row has to
	** first validate that the group isn't empty, and open the partition.
	*/

/* The ipart-opart EOF flags are currently only used by hash join */
#define QEN256_OPART_EOF	    0x0100
	/* Current outer has reached actual EOF, not just end-of-group */
#define QEN512_IPART_EOF	    0x0200
	/* Current inner has reached actual EOF, not just end-of-group */

#define QEN_ORIG_NEEDS_OPEN	    0x0400
	/* ORIG: "mid-open" received, do actual open the next time
	** we're called.  The reason for this is to allow higher QP nodes
	** to run and possibly do partition qualification before we actually
	** need to return anything.
	*/

#define QEN_ORIG_FASTNEXT_OK	    0x0800
	/* ORIG: orig satisfies qualifications for attempting a fastpath
	** get-next, once a get succeeds.  This flag is set when the orig
	** is reset or opened, since it depends on both runtime and compile
	** time conditions.  (See orig for a list of conditions.)
	*/

    QEF_AHD	*node_ahd_ptr;	/* current action header ptr.  used in
				** QENQP */

	/* A variety of orig-ish nodes (orig, kjoin, tjoin) need to
	** set the current partition number.  It's simplest to put it
	** here for everybody to use and look at.
	*/
    i4	node_ppart_num;		/* Current physical partition number */

    i4	node_rcount;	/* number of tuples retrieved from node */
    i4	node_cpu;	/* CPU cost for this node */
    i4	node_dio;	/* DIO cost for this node */
    i4	node_wc;	/* Wall clock for this node */
    i4	node_pcount;	/* number of partitions opened */

	/* Node_nbcost and node_necost give the number of times that
	** Qen_bcost_begin() and Qen_ecost_end() have been called. This
	** is used to correctly report the true amount of time that this
	** node has spent (cpu and dio), since the two cost routines take
	** time. We keep the numbers for both begin and end as a consistency
	** check to insure that they are both called the correct number of
	** times.
	*/
    i4	node_nbcost;
    i4	node_necost;

	/* The remainder of QEN_STATUS consists of unions of elements used 
	** for node-specific purposes. The union allows a reduction in
	** memory required to hold the fields.
	*/
    union {
	struct {
	    i4		node_sort_status;   /* status of the sorter */
#define QEN0_QEF_SORT		    0
	/* The sort is done in QEF buffer */
#define QEN9_DMF_SORT		    9
	/* The sort is done in DMF sorter */
	    i4		node_sort_flags;    /* supplementary flags */
#define QEN1_SORT_LAST_PARTITION    1
	/* Sort input was from group of table partitions and this
	** was the last. */
	} node_sort;

	struct {
	    i4		node_inner_status;  /* the status of the inner node */
#define QEN10_ROWS_AVAILABLE	    10
	/* a row is waiting in the inner node */
#define QEN11_INNER_ENDS	    11
	/* the inner relation has been fully scanned */
#define QEN13_PROBE_P1		    13
	/* executing probe phase 1 of hash join */
#define QEN14_PROBE_P2		    14
	/* executing probe phase 2 of hash join */
#define QEN15_MORE_ROWS             15

	    i4          node_outer_status;  /* the status of the outer node */
#define QEN8_OUTER_EOF               8
        /* the outer relation has been fully scanned */
#define QEN12_OUTER_JOINED	    12
	/* current outer row has inner joined */

	    i4          node_hold_stream;   /* the status of both hold and inner */
#define QEN5_HOLD_STREAM_EOF         5
        /* both hold and stream are at eof */
	    i4		node_outer_count;   /* count of outer rows read */
	} node_join;

        struct {
            QEE_DSH         *node_tproc_dsh;    /* dsh used by CALLPROC */ 
        } node_tproc;
            

	/* When the orig is for a PC-join (ie grouped partition access),
	** the following members mean:
	** node_gstart = first logical partition of the current group
	** node_grouppl = logical partitions left to do in current group
	** node_groupsleft = groups left to do
	** node_partsleft = physical partitions left in current logical part
	** node_lpart_num = current logical partition being read
	**
	** For non-PC-join orig, only node_partsleft is meaningful, and
	** it's the number of physical partitions left for this thread.
	*/
	struct {
	    QEN_NOR	*node_keys;	/* key structure */
	    i4		node_lpart_num;	/* current log. partition number */
	    i4		node_partsleft;	/* remaining partitions for thread */
	    i4		node_groupsleft; /* Remaining groups for thread */
	    i4		node_grouppl;	/* remaining partitions for group */
	    i4		node_gstart;	/* 1st log. partition of cur. group */
	    i4		node_dmr_flags;	/* Saved dmr flags for fast-getnext */
	} node_orig;
    } node_u;
}	QEN_STATUS;


/*}
** Name: QEN_HOLD - hold file structure for QEP nodes
**
** Description:
**      The hold file is used to store tuples read from a base relations.
**	This help avoid re-reading the base relation in certain cases.
**	A hold file is implemented as a DMF temporary table.  The only 
**	processing nodes that use hold files are the join nodes.
**
**	The DMF control blocks in this structure are only initialized for
**	the generic QEN_JOIN node. The QEN_SIJOIN uses the DMR_CB from the
**	QEN_SORT node which is its inner child.
**
** History:
**      2-sep-86 (daved)
**          written
**	19-sep-88 (puree)
**	    moved from QEFNODE.H.  Change prefix from qen to hold.
**	    rename qen_low_tid to hold_tid
**	    rename qen_hi_tid  tp hold_status
**	04-apr-90 (davebf)   
**	    added hold_medium, unget_status, unget_buffer
**	28-dec_90 (davebf)
**	    added hold_reset_pending, hold_reset_counter, hold_inner_status
**	27-mar-92 (rickh)
**	    Added new status word to qen_hold structure to describe states
**	    needed by right fsmjoins.
**	06-mar-96 (nanpr01)
**	    Added new page_size field for hold file to use increased
**	    tuple size.
**	21-apr-04 (inkdo01)
**	    Defined HFILE_LAST_PARTITION for partitioned tables.
*/
typedef struct _QEN_HOLD    
{
    DMT_CB	    *hold_dmt_cb;   /* DMT_CB for the hold file */
    DMR_CB	    *hold_dmr_cb;   /* DMR_CB for the hold file. */
    u_i4	    hold_tid;	    /* tid with current set of values */
    i4              hold_medium;    /* hold file in memory or on DMF */
#define HMED_ON_DMF         1
#define HMED_IN_MEMORY      0
    i4		    hold_status;    /* status of the hold file */
#define HFILE0_NOFILE	    0
	/* The DMF hold file has not been created */
#define	HFILE1_EMPTY	    1
	/* There are no tuples in the hold file */
#define HFILE2_AT_EOF	    2
	/* There are tuples from the inner node in the hold file.  However,
	** the hold file is at the "end-of-file".  Must be positioned to
	** read tuples from it. */
#define HFILE3_POSITIONED   3
	/* There are tuples from the inner node in the hold file.  The hold
	** file is position and is ready to read the next tuple from it. */

    i4		    hold_status2;    /* additional status bits */
#define	HFILE_REPOSITIONED	1
	/* Hold file has been backed up for rescanning	*/
#define HFILE_LAST_PARTITION	2
	/* Hold file contains sorted rows from partition groups and
	** this is the last group. */

    i4		    hold_buffer_status; /* status of tuple buffer */
#define HFILE6_BUF_EMPTY    6
	/* The tuple buffer is empty */
#define HFILE7_FROM_HOLD    7
	/* The tuple buffer has the tuple from the hold file */
#define HFILE8_FROM_INNER   8
	/* The tuple buffer has the tuple from the inner node */

    u_i4	    hold_reset_counter;	/* count of resets -- qensejoin */
    bool	    hold_reset_pending;	/* inner reset pending -- qensejoin */
    i4		    hold_inner_status;	/* eof, etc. on inner -- qensejoin */
	/* uses defines from qen_status */

    i4              unget_status; /*status of unget buffer */
#define HFILE9_OCCUPIED         9
    PTR             unget_buffer;
    i4	    hold_pagesize;      /* hold file page_size if DMF used */
}	QEN_HOLD;

/*}
** Name:    QEE_ACTCTX
**
** Description:
**	Structure for saving an action list context when a nested action
**	is called. Only the current action pointer and a restart status
**	is required.
**
** History:
**	15-jan-89 (paul)
**	    written to support rules processing.
**	02-apr-93 (jhahn)
**	    Added DSH_CT_CONTINUE2 for support of statement
**	    level unique indexes.
**	14-june-06 (dougi)
**	    Added DSH_CT_CONTINUE3 to support "before" triggers.
*/

typedef struct _QEE_ACTCTX  
{
    QEF_AHD	*dsh_ct_ptr;	/* Pointer to action to execute on return   */
				/* from nested action list. This is the	    */
				/* address of the calling action.	    */
# define	DSH_CT_INITIAL		0
# define	DSH_CT_CONTINUE		1
# define	DSH_CT_CONTINUE2	2
# define	DSH_CT_CONTINUE3	3

    i4		dsh_ct_status;	/* Continuation status for the calling	    */
				/* action when action list processing is    */
				/* resumed at dsh_ct_ptr. This is	    */
				/* DSH_CT_INITIAL when an action is first   */
				/* called. The action may use		    */
				/* DSH_CT_CONTINUE and DSH_CT_CONTINUE2     */
				/* as a convenient value to 		    */
				/* specify on return from a nested action.  */
				/* Action specific values are allowed.	    */
				/* CONTINUE3 was added to support before    */
				/* triggers.				    */
}	QEE_ACTCTX;

/*}
** Name: QEE_DDB_CB - Data segment control block for DDB query plans
**
** Description:
**
** History:
**	31-aug-88 (carl)
**          written
**	24-dec-88 (carl)
**	    added new fields 
**	01-jan-89 (carl)
**	    modified
**	14-feb-89 (carl)
**	    added qee_d6_dv_cnt, qee_d7_dv_p 
**	22-feb-89 (carl)
**	    added new fields
**	31-mar-89 (carl)
**	    added new fields qee_d10_tupdesc_p and qee_d11_ldb_desc
**	07-apr-89 (carl)
**	    added qee_d12_ldb_rowcnt 
**	08-may-89 (carl)
**	    added define QEE_06Q_TDS to QEE_DDB_CB
**
**
**  qee_d8_attr_pp			
**  |
**  |				       +---------+<------qee_d7_dv_p
**  +-->+-----+			       |         |
**      |     |------>+--------+       +---------+
**      +-----+	      |        |<------|         |
**      |     |	 ..   +--------+       +---------+
**      +-----+		  :		    :
**      |     |------>+--------+       +---------+
**      +-----+       |        |       |	 |
**                    |        |       +---------+
**                    +--------+
*/


typedef struct _QEE_DDB_CB
{
    DD_TAB_NAME	    *qee_d1_tmp_p;		/* ptr to array for temporary-
						** table names generated by
						** QEF when the DSH is created,
						** NULL if none */
    i4	    *qee_d2_sts_p;		/* ptr to array of status words
						** for above temporary tables,
						** NULL if none */
#define	QEE_00M_NIL	    0x0000L		/* initial status */
#define	QEE_01M_CREATED	    0x0001L		/* set if temporary table 
						** already created */
    i4	     qee_d3_status;		/* query status word */

#define	QEE_00Q_NIL	0x0000L			/* initial status */
#define	QEE_01Q_CSR	0x0001L			/* SET if cursor query */
#define	QEE_02Q_RPT	0x0002L			/* SET if repeat query */
#define	QEE_03Q_DEF	0x0004L			/* SET if query already defined 
						** to the LDB */
#define	QEE_04Q_EOD	0x0008L			/* SET if end of data */
#define	QEE_05Q_CDB	0x0010L			/* SET if retrieval via CDB
						** association needs to be
						** committed at end of qeuery 
						*/
#define	QEE_06Q_TDS	0x0020L			/* SET if RQF has given SCF
						** the LDB's tuple descriptor */
#define	QEE_07Q_ATT	0x0040L			/* SET if the tuple descriptor
						** carries attribute names, OFF
						** otherwise */
    DB_CURSOR_ID     qee_d4_given_qid;		/* cursor or repeat query id 
						** constructed by QEF */
    DB_CURSOR_ID     qee_d5_local_qid;		/* cursor or repeat query id 
						** returned by LDB */
    i4		     qee_d6_dynamic_cnt;	/* number of dynamic
						** DB_DATA_VALUEs for query
						** plan */
    DB_DATA_VALUE   *qee_d7_dv_p;		/* ptr to array of 
						** DB_DATA_VALUEs for entire
						** query plan */
    char	   **qee_d8_attr_pp;		/* ptr to array of ptrs to
						** attribute space */
    PTR		     qee_d9_bind_p;		/* ptr to union RQR_BIND 
						** structure large enough
						** for each QEA_D6_AGG action 
						** in the query plan */
    PTR		     qee_d10_tupdesc_p;		/* ptr to cursor's tuple
						** descriptor kept in RQF's
						** cache, returned by RQF and
						** presented to QF for fetches 
						*/
    DD_LDB_DESC	     qee_d11_ldbdesc;		/* LDB desc saved from REPEAT
						** or CURSOR subquery */
    i4	     qee_d12_tds_size;		/* size of tuple descriptor
						** space */
}   QEE_DDB_CB;

/*}
** Name: QEE_TEMP_TABLE - temporary table descriptor
**
** Description:
**	Query execution uses temporary tables to buffer information
**	flowing up from the tuple streams.  These light-weight tables
**	serve the function formerly provided by Sort/Hold files.  The
**	grand vision is that all hold files will eventually be replaced
**	with temporary tables.
**
**	A temporary table holds some number of tuples (currently 0) in
**	QEF memory before spilling to disk.
**
**
** History:
**	10-aug-1992 (rickh)
**	    Creation.
**
[@history_line@]...
*/
typedef struct _QEE_TEMP_TABLE	
{
    i4		tt_statusFlags;

#define	TT_EMPTY		0x00000001L
#define	TT_CREATED		0x00000002L
#define	TT_DONE_LOADING		0x00000004L
#define	TT_PLEASE_SORT		0x00000008L
#define TT_NO_DUPLICATES	0x00000010L

    DMT_CB	*tt_dmtcb;	/* control block for table operations */
    DMR_CB	*tt_dmrcb;	/* control block for record operations */
    i4		tt_tuple;	/* index into dsh rows for this table's tuple.
				** puts into this table are done from this
				** tuple.  gets from this table are done into
				** this tuple */

/* default number of tuples in the table.  this number is fed to the DMF
** loader.  DMF uses it if it has to sort the table.  if you want to sort
** this table, you may want to figure out how big it's going to be. */
#define	TT_TUPLE_COUNT_GUESS	0x0000001L
}	QEE_TEMP_TABLE;

/*}
** Name: QEE_VALID - Linked list of QEQ_VALID structs
**
** Description:
**      This struct forms a linked list of QEQ_VALID structs that is used
**	in the QEE_TBL_RESOURCE struct.
**
** History:
**      5-oct-93 (eric)
**          created
**	13-jan-94 (rickh)
**	    Reduced _QEE_VALID to a struct.  As a typedef it generated
**	    acc compiler warnings.
[@history_template@]...
*/
typedef struct _QEE_VALID QEE_VALID;
struct _QEE_VALID
{
    QEE_VALID	*qevl_next;
    QEE_VALID	*qevl_prev;

    PTR		qevl_qefvl; /* pointer to QEF_VALID */
}   ;

/*}
** Name: QEE_MEM_CONSTTAB - defines MS Access OR transformed in-memory 
**	constants table
**
** Description:
**      This holds the address of the in-memory table, along with the
**	number of "rows" and row size. It also holds the running count 
**	of rows scanned so far. This allows qen_orig to locate the next
**	row on each call.
[@comment_line@]...
**
** History:
**	13-feb-97 (inkdo01)
**	    Written.
[@history_line@]...
*/
typedef struct _QEE_MEM_CONSTTAB 
{
    PTR		qer_tab_p;		/* address of table. Rows are 
					** stored in contiguous memory */
    i4		qer_rowcount;		/* number of "rows" in table */
    i4		qer_scancount;		/* number of rows scanned so far
					** (maintained by qen_orig) */
    i4		qer_rowsize;		/* size of each row (in bytes) */
} QEE_MEM_CONSTTAB;

/*}
** Name: QEE_TBL_RESOURCE - Description of open instances of a table
**
** Description:
**      This struct describes the state of open instances of a table.
**	An open instance can be in one of three states:
**	    - Open and being used by an action
**	    - Open and not being used by an action
**	    - Closed (ie. an empty struct to be used).
**	These states are useful because tables can be opened many times
**	during the processing of a query. This allows us to reuse
**	open instances of a table, reducing the number of times that
**	we open a table, reducing our cpu cost.
**	This is also a part of the resource list processing. When a resource
**	list is validated, we open the tables as we can table ids. The
**	tables are put on the open/not used list, and are used by the
**	next action that uses the table.
**
**	Currently, we keep track of open instances of valid structs. As
**	soon as we get some DMF support we will move to keeping track of
**	dmr_cb/dmt_cb's. This change will allow us to make the maximum
**	reduction in cpu cost
**
** History:
**      5-oct-93 (eric)
**          created.
**	13-feb-97 (inkdo01)
**	    Added pointer to MS Access OR transformed constant table.
[@history_template@]...
*/
typedef struct _QEE_TBL_RESOURCE
{
	/* Qer_inuse_vl is a linked list of valid structs that have been
	** opened and are currently in use.
	*/
    QEE_VALID	*qer_inuse_vl;

	/* Qer_free_vl is a linked list of valid structs that have been
	** opened and are available for use.
	*/
    QEE_VALID	*qer_free_vl;

    /* Qer_empty_valid is a list of empty qee valid structs that are available
    ** for use in the free or inuse list.
    */
    QEE_VALID	*qer_empty_vl;
    /* If TBL_RESOURCE defines a MS Access OR transformed in-memory constant
    ** table, qer_cnsttab_p is the address of its defining structure (used 
    ** by qen_orig to "read" the "rows"). 
    */
    QEE_MEM_CONSTTAB	*qer_cnsttab_p;
}   QEE_TBL_RESOURCE;

/*}
** Name:  QEE_RESOURCE - Struct to describe the state of an active resource.
**
** Description:
**      This struct lives in the DSH, and holds information about the 
**      the state of the QEF_RESOURCE that it's associated with.
**      The information currently consists of a pointer to the 
**      associated QEF_RESOURCE, and info specific to the resource type
**      (see above for more info)
[@comment_line@]...
**
** History:
**      5-oct-93 (eric)
**          created
[@history_line@]...
[@history_template@]...
*/
typedef struct _QEE_RESOURCE
{
	/* Qer_qefresource points to the QEF_RESOURCE in the query plan that
	** this struct is paired with
	*/
    PTR	    qer_qefresource; /* pointer to QEF_RESOURCE */

    union
    {
        QEE_TBL_RESOURCE  qer_tbl;
    } qer_resource;      /* union of possible resources */
}   QEE_RESOURCE;

/*}
** Name: QEE_RSET_SPOOL	- structure for runtime scrollable cursor processing
**
** Description:	This structure contains fields relevant to the execution of
**	a scrollable cursor.
**
** History:
**	24-jan-2007 (dougi)
**	    Written.
**	17-may-2007 (dougi)
**	    Added sp_fmapcount for TID computation.
*/

typedef struct _QEE_RSET_SPOOL {
    DMR_CB	*sp_wdmrcb;		/* ptr to write DMR_CB of spool temp */
    DMR_CB	*sp_rdmrcb;		/* ptr to read DMR_CB of spool temp */
    DMT_CB	*sp_dmtcb;		/* ptr to DMT_CB of spool temp */
    PTR		sp_rowbuff;		/* row buffer addr. */
    i4		sp_rowcount;		/* no. of rows in result set (so far) */
    i4		sp_current;		/* current row in result set */
    i4		sp_pagecount;		/* no. of pgs in result set (so far) */
    i4		sp_curpage;		/* current page in result set */
    i4		sp_rowspp;		/* number of rows per page */
    i4		sp_fmapcount;		/* frequency of fmap pages for psize */
    i4		sp_flags;		/* flag field */
#define	SP_ALLREAD	0x01		/* all the rows have been read */
#define	SP_BEGIN	0x02		/* cursor precedes 1st row */
#define	SP_END		0x04		/* cursor is after last row */
#define	SP_BEFORE	0x08		/* cursor precedes row at sp_position
					** (happens when a row is deleted from
					** result set) - otherwise cursor is
					** on row at sp_position */
}	QEE_RSET_SPOOL;


/*
** Name: QEE_PART_QUAL - Partition Qualification Runtime Info
**
** Description:
**
**	Partition qualification is mostly described by the QEN_PART_QUAL
**	data structure that's part of the query plan.  Since it's
**	part of the QP, no runtime state can be held in the
**	QEN_PART_QUAL.  Some nodes (namely fsm/hash joins) might
**	qualify a number of inner-side tables, so having one set of
**	runtime state in (say) the qen-status doesn't fly.
**
**	So, in the "usual manner", each QEN_PART_QUAL generated into the
**	query plan is assigned an index number, which is used as an
**	array index to get at the corresponding runtime entity, QEE_PART_QUAL.
**	The QEE_PART_QUAL array is pointed to by the DSH.
**
**	All of the QEE_PART_QUAL's related to a given ORIG (or ORIG-like
**	node, eg K/T-join) are linked, with the ORIG's info block at
**	the start of the list.  (This is easy since qee traverses the
**	QP node tree in postorder, i.e. bottom up.)  Doing this makes
**	it possible for higher-up nodes to compute and hold on to their
**	qualification bitmaps;  when the ORIG is eventually reached, it
**	can grab and AND all the bitmap via this linked list.
**
**	(Doing things the other way, having the upper nodes know where
**	the orig is and immediately ANDing each partial result, works
**	fine until we try to send a reset thru the qp.  The top nodes
**	see the reset and have a new bitmap ready before the orig
**	even gets the reset!  so it's much easier to let each join keep
**	its own result and let the ORIG reach out for all the results.)
**
**	One piece of fairly essential runtime state kept here is an
**	optimization that's only needed for fsm/hash join qualifications.
**	Namely, whether to bother!  Successive outer rows are inspected for
**	partition restrictions and the resulting bit-maps are OR'ed
**	together.  If we ever reach the point where the result is all
**	ones, this join does not restrict the table at all, and so there
**	is no point in doing all the qualification processing for the
**	remaining input rows at this node.
**
**	The rest of the QEE_PART_QUAL structure is mainly for convenience.
**	Join quals refer to the qual struct in the orig node, which has
**	DSH row indexes for various key result bitmaps.  These row numbers
**	are constant for the life of the query, so it's reasonable to
**	resolve all this stuff to addresses at query setup time.
**
** History:
**	21-Jul-2007 (kschendel) SIR 122513
**	    Generalized partition qualification.
*/

/* Keep in mind that some of these row and bitmap pointers might be
** NULL in cases where they aren't relevant.  (e.g. a hash-join's part-qual
** doesn't need the constant bitmap.)
*/

struct _QEE_PART_QUAL
{
	DB_PART_DEF	*qee_qdef;	/* Partition definition ptr */
	struct _QEE_PART_QUAL *qee_qnext;  /* Next related rqual, starting
					** with ORIG and going up the tree */
	QEN_PART_QUAL	*qee_orig_pqual; /* Address of "orig"'s pqual info;
					** keep this because the orig might
					** really be a kjoin or tjoin too,
					** so figure it out once and store it
					*/
	PTR		qee_lresult;	/* Local-result bitmap */
	PTR		qee_constmap;	/* Constant-qual bitmap */
	PTR		qee_work1;	/* LP-to-PP work row */
	i2		qee_mapbytes;	/* Size (bytes) of a bitmap, for
					** convenience.  (for size in bits,
					** use qee_qdef->nphys_parts.)
					*/

/* The qualifying / enabled flags are both used to optimize away join-time
** qualifying that can't help any.
** qee_qualifying is turned off if a join outer has lit all of the partition
** bits, i.e. nothing is restricted any more.  This flag is reset when
** the join node is reset, and it's reset to qee_enabled.
** qee_enabled is normally TRUE, but if the orig node's constant
** predicates already restrict the table sufficiently (e.g. to a single
** partition), running all the join time stuff is no longer worth the
** effort, in which case enabled becomes FALSE and stays FALSE even
** across simple resets.  (A full QP revalidate from say an SE-join
** re-runs qeq-validate, which can in fact turn enabled back on.)
*/

	bool		qee_qualifying;	/* Resettable qualifying enabler,
					** looked at in join node */
	bool		qee_enabled;	/* Enabler override, set FALSE if
					** orig level says it's not worth it.
					** Not reset until re-validate. */
};

typedef struct _QEE_PART_QUAL QEE_PART_QUAL;


/*}
** Name: QEE_XADDRS - Execution Time Addresses
**
** Description:
**
**	Finding QP things at execution time is a bit convoluted,
**	mostly because the QP itself has to be read-only, thread-
**	safe, and multiple-query-safe.  So anything in a node (say)
**	that is meant to be a pointer to some run-time data structure,
**	such as an ADE_EXCB or DMR_CB, may not be a pointer.  Instead,
**	it's usually a control block index.  At runtime, we can
**	find the proper address by doing something like:
**	    addr = dsh->dsh_cbs[control_block_number];
**
**	In some cases, particularly for node CX control blocks, doing
**	this over and over is a pain.  There are lots of different
**	control block numbers, themselves often accessed through
**	various QP sub-structures.  Once a DSH is set up for a thread
**	to run a (sub)query, the control block addresses don't change.
**	So, for some of the most common ones which are directly related
**	to QP things like node CX's, we'll resolve the addresses at
**	DSH setup time and store them in a QEE_XADDR structure for
**	the node.  Thus, the runtime address resolution becomes:
**	    addr = dsh->dsh_xaddrs[node_num]->qex_whatever;
**
**	which doesn't look simpler, but actually is, when you consider
**	the complexities of getting at some control-block-number's.
**
**	One QEE_XADDRS is allocated for each node and action in
**	the QP; in fact, we allocate "qp_stat_cnt" of 'em, just
**	like the QEN_STATUS structures.
**
**	I'm not a big fan of the prefix-style member naming club, but
**	in this case I'm doing it because the same names are used in
**	too many other contexts, and it's hard to grep.
**
**	Side note:  QEN_STATUS is inappropriate for holding resolved
**	addresses, because it's cleared at the start of every execution.
**	The QEE_XADDR is set up with the DSH and lives unchanged through
**	multiple (repeated) executions.
**
** History:
**	12-Jan-2006 (kschendel)
**	    Create to make runtime CB accessing easier.
**	21-Jul-2007 (kschendel) SIR 122513
**	    Add partition qual address for orig, K/T-joins.
**
*/

typedef struct _QEE_XADDRS
{
    /* ADE_EXCB addresses for various node CX's.
    ** Not all nodes have all of these, but it's too small to union-ize.
    */

    /* "outer tuple materialize": btmat for hash join, various sort-mat's */
    struct _ADE_EXCB *qex_otmat;

    /* "inner tuple materialize": ptmat for hash join, itmat for sjoin
    ** and sejoin
    */
    struct _ADE_EXCB *qex_itmat;

    /* "Key prepare" for key joins */
    struct _ADE_EXCB *qex_keyprep;

    /* "outer key materialize": simple and se-joins */
    struct _ADE_EXCB *qex_okmat;

    /* "outer key compare": kcompare or simple join's okcompare */
    struct _ADE_EXCB *qex_kcompare;

    /* "pre-qualification": isnull in t-joins, oqual (not the outer join
    ** oqual) in se-joins.
    */
    struct _ADE_EXCB *qex_prequal;

    /* "correlation compare" for sejoin only */
    struct _ADE_EXCB *qex_ccompare;

    /* "join key compare": joinkey in simple join, kqual in k/se join */
    struct _ADE_EXCB *qex_joinkey;

    /* "orig qualification": orig_qual, or iqual in kjoins.  Not normally
    ** executed directly unless tid comparisons are involved;  normally this
    ** qual is pushed down into DMF.
    */
    struct _ADE_EXCB *qex_origqual;

    /* "outer join ON-qual": oj_oqual in any outer join */
    struct _ADE_EXCB *qex_onqual;

    /* "outer join LNULL": materialize left-join plus nulls */
    struct _ADE_EXCB *qex_lnull;

    /* "outer join RNULL": materialize right-join plus nulls */
    struct _ADE_EXCB *qex_rnull;

    /* "outer join innerjoin materialize": materialize inner join in the
    ** context of an outer join, setting flag ("special") eqcs properly.
    ** Called "equal" in the qef nodes (very misleading!).
    */
    struct _ADE_EXCB *qex_eqmat;

    /* "post-join qualification": jqual in all joins except se-join,
    ** also qp_qual in QP's
    */
    struct _ADE_EXCB *qex_jqual;

    /* "function attribute generation" in any node */
    struct _ADE_EXCB *qex_fatts;

    /* "print row" for any node, generated only if op173 turned on */
    struct _ADE_EXCB *qex_prow;

    /* tproc qualification */
    struct _ADE_EXCB *qex_tprocqual;

    /* partition elimination qual?  (what about multi-dimension qual?)
    ** other partition-sensitive stuff too, eg dmrcb, dmtcb
    ** maybe put them at the end, varying length?  (or maybe not....)
    */

    /* We'll keep the address of the qen-status area for this node here. */
    QEN_STATUS *qex_status;

    /* For orig's of partitioned tables, if partition qualification is
    ** applied, there will be exactly one QEE_PART_QUAL for the orig.
    ** Keep its address here as a minor convenience.
    ** Same for K/T-joins with partitioned inners.  Note however that
    ** fsm/hash joins may qualify multiple sources, those nodes have to
    ** locate the relevant runtime control block the hard way.
    */
    QEE_PART_QUAL *qex_rqual;

    /*
    ** maybe there's some way to use this to optimize CX base-address
    ** referencing, too.
    ** sort-holds?  dmr/dmt cb's?  (partitioning could be a problem)
    */

} QEE_XADDRS;

/*}
** Name: QEE_DSH - data segment header
**
** Description:
**
** History:
**	24-mar-86 (daved)
**          written
**	22-oct-87 (puree)
**	    added dsh_qconst_cb for query constants support.
**	10-may-88 (puree)
**	    added dsh_qp_status to keep the status of the QP.  Also move
**	    QEQP_OBSOLETE flag from QP to DSH since QP is a read-only object.
**	31-aug-88 (carl)
**	    added field dsh_ddb_cb
**	01-sep-88 (puree)
**	    add dsh_vlt to be the pointer to array of QEE_VLT.
**	    remove dsh_adf, dsh_hadf, dsh_sadf.
**	    rename the following fields:
**		dsh_stat    to	dsh_qen_status
**		dsh_shandle to	dsh_qp_handle
**		dsh_sid	    to	dsh_qp_id
**		dsh_qpt_qp  to	dsh_qp_ptr
**		dsh_act_id  to	dsh_act_ptr
**		dsh_adf_pt  to	dsh_qe_adf
**		dsh_assoc_handle to dsh_aqp_dsh
**		dsh_aqps    to	dsh_dep_dsh
**		dsh_stmt    to	dsh_stmt_no
**		dsh_dmt_cbs to	dsh_open_dmtcbs
**	19-sep-88 (puree)
**	    change dsh_hold from the address to the ptr array for QEN_HOLD
**	    to be the address of the QEN_HOLD array.
**	    rename dsh_dmt_cbs to dsh_open_dmtcbs.
**	15-jan-89 (paul)
**	    Added support for rules processing. The DSH can now be considered
**	    the stack "frame" for nested procedure calls. Also add state
**	    information for nested action list execution.
**	01-apr-89 (paul)
**	    Add status flag to dsh_qp_status. It is now possible for the
**	    system to try to invoke the same QP in a nested fashion requiring
**	    multiple DSH's for the QP. We need to indicate that a DSH is in use.
**      28-jul-89 (jennifer)
**          Added status value to dsh_qp_status.  It inidicates that
**          the audit inforamtion has been processed for this QP.
**	20-nov-90 (davebf)
**	    Add DSH_CURSOR_OPEN to dsh_qp_status.  Set in qeq_open. Tested
**	    in qea_fetch when no QEP.
**	24-jan-91 (davebf)
**	    added dsh_shd_cnt
**	10-aug-92 (rickh)
**	    Added temporary table array.
**	30-nov-92 (anitap)
**	    Added pointers to QEF_RCB and QEE_DSH structures for CREATE SCHEMA. 
**      20-jul-1993 (pearl)
**          Bug 45912.  Add dsh_kor_nor, array of pointers to QEN_NOR's, to
**          QEE_DSH.
**      5-oct-93 (eric)
**          added support for resources.
**	05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with
**	    DM0M_OBJECT.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Add DSH_SORT flag.
**	06-jul-1998 (somsa01)
**	    Added dsh_dgtt_cb_no to keep track of the global temporary
**	    table in a "declare global temporary table ... as select ..."
**	    situation.  (Bug #91827)
**	28-oct-98 (inkdo01)
**	    Added dsh_rowcount for row-producing procs.
**	18-june-01 (inkdo01)
**	    Added DSH_FORINPROC flag to avoid resetting qef_rowcount for
**	    callproc's.
**	18-dec-03 (inkdo01)
**	    Added a few fields for parallel query execution.
**	3-feb-04 (inkdo01)
**	    Added dsh_admrcb to replace the need to use ahd_odmr_cb 
**	    for posn'd upd/del. Necessitated by table partitioning.
**	23-feb-04 (inkdo01)
**	    More fields to support thread safe || execution. Also replaced
**	    several embedded structures with ptrs to reduce DSH size.
**	25-feb-04 (inkdo01)
**	    Add dsh_qefcb to stow QEF_CB ptr so DSH ptr can be placed
**	    in dmr_q_arg for thread safety.
**	27-feb-04 (inkdo01)
**	    Changed dsh_hold, dsh_shd, dsh_qen_status, dsh_hash, dsh_hashagg
**	    from ptrs to arrays of structs to ptrs to arrays of ptrs to
**	    the structs (allows better management of DSH memory).
**	24-mar-04 (inkdo01)
**	    Added DSH_DONE_1STFETCH to guide processing of cursors.
**	26-mar-04 (inkdo01)
**	    Added dsh_ahdvalid to hold valid chain for top AHD.
**	15-Jul-2004 (jenjo02)
**	    Deprecated dsh_child, dsh_cnext.
**	19-Jul-2004 (schka24)
**	    Added place to hold execution-time ULM stream.
**	30-Jul-2004 (jenjo02)
**	    Added dsh_dmt_id to hold thread transaction context,
**	    initialized to qef_dmt_id and overridden by
**	    Child thread's context.
**	15-Aug-2005 (schka24)
**	    Add more row-producing-procedure output context so that we can
**	    be sure that we're tracking the output buffers properly.
**	2-Dec-2005 (kschendel)
**	    Remove sortkey list pointer, not needed.
**	14-Dec-2005 (kschendel)
**	    Add ADF CB pointer.
**	12-Jan-2006 (kschendel)
**	    Add resolved-address (XADDR) pointers.
**	13-Feb-2007 (kschendel)
**	    Added session ID for cancel checking.
**	11-dec-2007 (dougi)
**	    Dropped dsh_srowcount, now moved to action header-specific 
**	    block QEA_FIRSTN.
**	25-Jan-2008 (kibro01) b116600
**	    Add original dsh_row pointers so if adjusted we can go back
**	    to those allocated here
**	22-Feb-2008 (kschendel) SIR 122513
**	    Remove ahdvalid, better and safer to stash vl backpointer
**	    into the dmrcb itself for partition opening.
**	4-Sep-2008 (kibro01) b120693
**	    Add dsh_ahdflags to save the action's flag for the same reason
**	    as saving dsh_ahdvalid, above.
**	8-Sep-2008 (kibro01) b120693
**	    Rework fix to b120693, avoiding need to extra flag.
**	11-Apr-2008 (kschendel)
**	    Remove dsh_qe_adf, new dmrcb stuff makes it obsolete.
**	4-Jun-2009 (kschendel) b122118
**	    Add bools for quick tests for qe90 (stats) and qe89 (hash debug).
**	    Add qe11 (suppress output) info.
*/

/* typedef struct _QEE_DSH	    QEE_DSH; */
struct	_QEE_DSH
{
    /* standard header for Jupiter structures */
    QEE_DSH        *dsh_next;       /* The next control block */
    QEE_DSH        *dsh_prev;       /* The previous one */
    SIZE_TYPE       dsh_length;     /* size of the control block */
    i2              dsh_type;       /* type of control block */
#define QEDSH_CB    2
    i2              dsh_s_reserved;
    PTR             dsh_l_reserved;
    PTR             dsh_owner;
    i4              dsh_ascii_id;   /* to view in a dump */
#define QEDSH_ASCII_ID  CV_C_CONST_MACRO('Q', 'E', 'D', 'S')
    /* user's data space */
    i4		    dsh_qp_status;	    /* The status bit map of the DSH */
#define	DSH_QP_LOCKED	0x01
    /* The corresponding QP for this DSH is locked by the current session */
#define DSH_QP_OBSOLETE	0x02
    /* The DSH and its QP is obsolete. Must be destroyed */
#define DSH_DEFERRED_CURSOR 0x04
    /* This query contains a deferred cursor */
#define	DSH_IN_USE	    0x08
    /* This DSH is in use by a currently executing query plan */
#define DSH_CPUSTATS_OFF    0x10
    /* CPU thread stat collection was turned on at the beginning of this query
    ** and should be turned off when it is finished */
#define	DSH_AUDITED	    0x20
    /* This DSH has already procesed audit information. */
#define DSH_CURSOR_OPEN	    0x40
    /* This is a cursor query.  Turned on my qeq_open */
#define DSH_SORT            0x80
    /* This flag indicates that a temporary sort table is being loaded. */
#define DSH_DBPARMS_DONE    0x100
    /* This indicates that a procedure has had its parm initialization 
    ** done (used to avoid double init in row producing procs). */
#define DSH_FORINPROC	    0x200
    /* This procedure has a for-loop - don't reset qef_rowcount */
#define	DSH_DONE_1STFETCH   0x400
    /* Cursor has executed fetch for 1st time (used to guide reset flag
    ** in cursor'd queries). */
#define DSH_TPROC_DSH       0x800
    /* This query contains a table procedure. Set only to the top DSH */
    QEF_CB	   *dsh_qefcb;	    /* PTR to qef_cb */
    QEE_DSH	   *dsh_root;	    /* PTR to root DSH for query */
    PTR		    dsh_handle;	    /* handle to memory for this object */
    PTR             *dsh_param;     /* base of parameter list array */
    PTR             dsh_result;     /* base of result area */
    PTR             dsh_key;        /* ptr to key area */
    QEE_VLT	    *dsh_vlt;	    /* addr of array of QEE_VLTs.  There are 
				    ** as many elements of the array as the
				    ** number of CXs with user parameter
				    ** (QEF_QP_CB.qp_pcx_cnt).  An element
				    ** is indexed by qen_pcx_idx in QEN_ADF */
    QEN_SHD	    **dsh_shd;	    /* addr of array of pointers to QEN_SHDs */
    QEN_HOLD	    **dsh_hold;     /* addr of array of ptrs to QEN_HOLDs */
    QEE_XADDRS	    **dsh_xaddrs;   /* Base of array of ptrs to node QEE_XADDRS
				    ** indexed by node qen_num */
    PTR             *dsh_row;       /* addr of array of pointers to row 
				    ** buffers */
    PTR             *dsh_row_orig;  /* original dsh_row values */
    QEN_NOR         **dsh_kor_nor;  /* addr of array of pointers to QEN_NOR's
                                    ** indexed by kor_id field of QEF_KOR's;
                                    ** fix for bug 45912.
                                    */
    PTR             *dsh_cbs;	    /* addr of array of pointers to various
				    ** control blocks, mostly DMR_CB, DMT_CB,
				    ** and ADE_EXCB, depending on the
				    ** action/node. */
    QEN_HASH_BASE   **dsh_hash;	    /* addr of array of pointers to
				    ** QEN_HASH_BASE's (for hash joins) */
    QEN_HASH_AGGREGATE **dsh_hashagg; /* addr of array of pointers to
				    ** QEN_HASH_AGGREGATE's (for hash 
				    ** aggregate actions) */
    QEE_PART_QUAL   *dsh_part_qual; /* Array of QEE_PART_QUAL structs, indexed
				    ** by a QEN_PART_QUAL's part_pqual_ix */
    DMT_CB	    *dsh_open_dmtcbs; /* Pointer to first of a doubly linked
				    ** list of DMT_CB's opened for this
				    ** thread. (list is thru q_next, q_prev)
				    */

    ADF_CB	    *dsh_adf_cb;	/* Pointer to ADF control block.
					** **** FIXME This is the session's
					** ADF cb, the same one that the
					** QEF_CB points to.  This is wrong
					** in a parallel query environment,
					** we really ought to have an adfcb
					** per thread to handle errors!
					*/

    /* This session ID is for cancel checking, and therefore can be / will be
    ** zero in a non-parent thread.  If you need the session ID and you
    ** might be in a parallel-query child thread, don't look here!
    */
    CS_SID	    dsh_sid;		/* Session ID for cancel checks */

    /* Nested action list state information.				    */
    /* When a nested action list is invoked, save the current action	    */
    /* pointer and an action specific status value that can be used by the  */
    /* suspended action to signal the nested action has completed.	    */
    i4		    dsh_depth_act;  /* Current action list nesting depth    */
    QEE_ACTCTX	    dsh_ctx_act[QEF_ACT_DEPTH_MAX];

    /* DSH stack information.						    */
    /* For nested procedure calls, the DSH structures are now organized as  */
    /* a stack, one for each currently active procedure invocation.	    */
    /* The stack is represented as a linked list through the field	    */
    /* dsh_stack. An area is allocated for saving the current RCB contents  */
    /* on a procedure invocation and an area is reserved for the actual	    */
    /* parameter list.							    */
    QEE_DSH	    *dsh_stack;	    /* Pointer to next DSH on stack. */
    QEF_RCB	    *dsh_saved_rcb; /* ptr to save area for QEF_RCB context */
    PTR		    *dsh_usr_params;/* Pointert to an array of		    */
				    /* QEF_USR_PARAM structures in which to */
				    /* place ctual parameters for each	    */
				    /* procedure call in this QP. Each	    */
				    /* entry is sized for the number of	    */
				    /* actual parameters in a specific	    */
				    /* procedure call statement. */
    /* We might have nested QEA_EXEC_IMM actions. The DSH structures will need
    ** to be organized as a stack for each currently active QEA_EXEC_IMM action.
    ** The stack is represented as a linked list through the field dsh_exeimm.
    ** QEF_RCB also needs to be saved for QEA_EXECUTE_IMM actions. The context
    ** is saved so that we can continue with the next QEA_EXEC_IMM action 
    ** after returning from the processing of the current action.
    */
    QEF_RCB	   *dsh_exeimm_rcb;  /* prt to save area for QEF_RCB context */
    QEE_DSH	   *dsh_exeimm;	    /* Pointer to next DSH on stack */
    /* query plan information */
    PTR		    dsh_qp_handle;  /* QSF handle for QP */
    DB_CURSOR_ID    dsh_qp_id;	    /* name of query */
    QEF_QP_CB       *dsh_qp_ptr;    /* pointer to the corresponding QP */
    QEF_AHD         *dsh_act_ptr;    /* pointer to current action 
				    ** This is set to zero until the query
				    ** is opened.
				    */
    QEE_DSH	    *dsh_aqp_dsh;   /* PTR to DSH of associated QP 
				    ** if this query plan is UPDATE
				    */
    QEE_DSH	    *dsh_dep_dsh;   /* list of DSHs of QP that are dependent
				    ** on this one's existence. This is
				    ** used for cascading destruction.
				    */
    i4	    dsh_stmt_no;    /* The statement number.  It is
				    ** obtained from the statement number
				    ** in the qef_cb when the QP is opened.
				    ** It is particularly useful for 
				    ** identifying different cursors
				    */
    i4	    dsh_update_mode;/* is statement deferred or direct
				    ** The DMF constants are used here 
				    */
    ADK_CONST_BLK   *dsh_qconst_cb; /* ptr to query constants block */

    i4		    dsh_trace_rows; /* Zero is normal, otherwise QE11 NN
				    ** trace point in effect.  +N is output
				    ** N more rows, -1 is toss rows until
				    ** no-more-rows signaled.
				    */
    bool	    dsh_positioned; /* TRUE if cursor is positioned */

    bool	    dsh_hash_debug; /* TRUE for QE89 hash debugging.
				    ** (easier to test a bool than doing
				    ** ult-check-macro calls all over in
				    ** the hot path */
    bool	    dsh_qp_stats;   /* TRUE for QE90 and related stats.
				    ** (another ult-check-macro avoider) */

	/* Dsh_cpuoverhead and dsh_diooverhead hold the cpu and dio
	** required to gather stats for the QEN_NODE actual (vs. estimated)
	** cost gathering. See qen_bcost_begin() and qen_ecost_end().
	*/
    f4		    dsh_cpuoverhead;
    f4		    dsh_diooverhead;

	/* The following fields hold the total cpu, dio, and wall clock for 
	** the query.
	*/
    i4		    dsh_tcpu;
    i4		    dsh_tdio;
    i4		    dsh_twc;

    i4		    dsh_shd_cnt;	/* potential # of in-memory sort/holds */

    QEE_DDB_CB	    *dsh_ddb_cb;	/* ptr to CB for DDB query plan */

    QEE_TEMP_TABLE  **dsh_tempTables;	/* array of temporary table ptrs */

	/* Dsh_resources is a pointer to an array of QEE_RESOURCE structs
	** the size of the array is determined by qp_cnt_resources from
	** the associated query plan.
	*/
    QEE_RESOURCE    *dsh_resources;

	/* Stream id that dsh was allocated out of.
	** For a repeated query this is a ULH stream.
	** In any case the stream is from the QEF DSH pool.
	*/
    PTR		    dsh_streamid;
    i4		    dsh_dgtt_cb_no;	/* In the case of a global
					** temporary table creation as
					** a select, this keeps track
					** of the cb_no in the dsh_cbs
					** array of the temp table.
					*/
    /* Row-producing procedure stuff.  "return row" only emits one row at
    ** a time, unlike the usual GET which runs until eof or buffer-full.
    ** Since return-row can be intermixed with all sorts of other DBP
    ** actions, some of which may not play nice with the usual qef_count/
    ** qef_rowcount variables, track RPP output buffering here:
    */
    i4		    dsh_rpp_rowmax;	/* Number of rows which SCF is prepared
					** to accept.  (Not the same as the
					** number of output buffers passed in,
					** which could be more but not less.)
					*/
    i4		    dsh_rpp_rows;	/* Running total of rows generated by
					** return-row since SCF last passed us
					** fresh output buffers.
					*/
    i4		    dsh_rpp_bufs;	/* Running total of buffers used by
					** return-row since SCF last passed us
					** fresh output buffers.  Usually the
					** same as rpp_rows, but could be more
					** when blobs are around.
					*/
    i4		    dsh_threadno;	/* number of child thread under
					** parent using this DSH */

    /* Execution time stream ID.  This stream differs from dsh_streamid
    ** in that it's specific to an execution and is closed at end of
    ** execution.  (dsh_streamid lives as long as the DSH in the case
    ** of a repeated query.)
    ** For a non-repeated query, qstreamid is the SAME as streamid.
    ** For a repeated query, qstreamid is initially NULL and a stream
    ** is opened if anyone needs the memory.
    ** The stream is opened PRIVATE, so it's not thread-safe.
    ** It's allocated from the DSH pool, not the sort-hash pool.
    */
    PTR		    dsh_qstreamid;	/* Query time stream if open */

    /*
    ** Thread's DMF transaction context (DML_XCB*). 
    ** Initially set to query's context from qef_dmt_id,
    ** overwritten when Child thread creates a new context.
    */
    PTR		    dsh_dmt_id;		/* DMF transaction context */

	/* Copies of various fields from QEF_RCB (& QEF_CB?) that are
	** required to make parallel query execution thread safe. */
    i4		dsh_qef_rowcount;
    i4		dsh_qef_targcount;
    QEF_DATA	*dsh_qef_output;
    QEF_DATA	*dsh_qef_nextout;
    i4		dsh_qef_count;
    i4		dsh_qef_remnull;
    DB_ERROR	dsh_error;
};
