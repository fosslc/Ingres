/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DM2UMXCB.H - Modify indeX Control Block.
**
** Description:
**      This module contains the description of the modify/index control block.
**
** History:
**      26-feb-1986 (derek)
**          Created new for Jupiter.
**	21-jul-1989 (teg)
**	    add mx_reltups to fix bug 6805.
**	17-dec-1990 (bryanp)
**	    Added mx_data_atts, mx_index_comp, mx_comp_katts_count for
**	    Btree index compression project.
**	14-jun-1991 (Derek)
**	    Reorganized a little in support for new build interface.
**	10-mar-1992 (bryanp)
**	    Added fields to support in-memory builds.
**	22-oct-92 (rickh)
**	    Replaced all references to DMP_ATTS with DB_ATTS.  Also replace
**	    all references to DM_CMP_LIST with DB_CMP_LIST.
**	14-dec-92 (jrb)
**	    Removed mx_s_location field from the MXCB since it's no longer used.
**	8-apr-93 (rickh)
**	    Added support for FIPS CONSTRAINTS characteristics:  persists
**	    over modifies, statement level unique, etc..
**	15-may-1993 (rmuth)
**	    Add support for READONLY tables and concurrent indicies.
**	18-oct-1993 (rogerk)
**	    Added mx_buckets to indicate to the build routines the number
**	    of buckets to use in a hash table creation.
**	06-mar-1996 (stial01 for bryanp)
**	    Add mx_page_size to allow specification of page size.
**      10-mar-1997 (stial01)
**          Added mx_crecord, a tuple buffer for compression
**	24-Mar-1997 (jenjo02)
**	    Add mx_tbl_pri to allow specification of Table's cache priority.
**	01-Oct-1997 (shero03)
**	    B85456: added a separate log id it can be passed in lieu of
**	    xcb_log_id (which is not available during recovery).
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      08-Oct-2002 (hanch04)
**          Replace the i4 for length with a SIZE_TYPE (size_t) to allow
**          64-bit memory allocation.
**	20-Apr-2004 (schka24)
**	    Invent modify record header.
**	10-jan-2005 (thaju02)
**	    Online modify of partitioned table.
**	    Added DM2U_OPCB, DM2U_OMCB, DM2U_OSRC_CB.
**	    Removed mx_online_tabid, mx_input_rcb, mx_dupchk_tabid,
**	    mx_rnl_online from DM2U_MXCB. Added MX_ROLLDB_ONLINE_MODIFY.
**	    Added mx_resume_spcb.
**	    Added spcb_dupchk_tabid, spcb_dupchk_rcb, spcb_rnl_tabname, 
**	    spcb_rnl_owner, spcb_rnl_online to DM2U_SPCB. Added 
**	    tpcb_dupchk_tabid, tpcb_dupchk_rcb to DM2U_TPCB.
**	11-mar-2005 (thaju02)
**	    Added opcb_tupcnt_adjust.
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**	23-Oct-2009 (kschendel) SIR 122739
**	    Integrate changes for new rowaccessor scheme.
**/

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _DM2U_MXCB DM2U_MXCB;
typedef struct _DM2U_SPCB DM2U_SPCB;
typedef struct _DM2U_TPCB DM2U_TPCB;
typedef struct _DM2U_RHEADER DM2U_RHEADER;
typedef struct _DM2U_OSRC_CB DM2U_OSRC_CB;
typedef struct _DM2U_OPCB DM2U_OPCB;
typedef struct _DM2U_OMCB DM2U_OMCB;


/*}
** Name: DM2U_MXCB - Modify indeX COntrol BLock
**
** Description:
**      This structure defines the control block needed to manage a
**	modify or index operation.
**
** History:
**     24-feb-1986 (derek)
**          Created new for Jupiter.
**	17-dec-1990 (bryanp)
**	    Added mx_index_comp, mx_data_atts and mx_comp_katts_count 
**	    for Btree index compression project.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warnings.
**	10-mar-1992 (bryanp)
**	    Added fields to support in-memory builds.
**	14-dec-92 (jrb)
**	    Removed mx_s_location field since it's no longer used.
**	8-apr-93 (rickh)
**	    Added support for FIPS CONSTRAINTS characteristics:  persists
**	    over modifies, statement level unique, etc..
**	15-may-1993 (rmuth)
**	    Add support for READONLY tables and concurrent indicies.
**	18-oct-1993 (rogerk)
**	    Added mx_buckets to indicate to the build routines the number
**	    of buckets to use in a hash table creation.
**	05-nov-1993 (smc/swm)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	08-may-1995 (shero03)
**	    Support RTREE 
**	06-mar-1996 (stial01 for bryanp)
**	    Add mx_page_size to allow specification of page size.
**	24-Mar-1997 (jenjo02)
**	    Add mx_tbl_pri to allow specification of Table's cache priority.
**	01-Oct-1997 (shero03)
**	    B85456: added a separate log id it can be passed in lieu of
**	    xcb_log_id (which is not available during recovery).
**      24-jul-2002 (hanal04) Bug 108330 INGSRV 1847
**          Added mx_dmveredo to flag that we are in dmve redo. 
**	28-Feb-2004 (jenjo02)
**	    Relocated some MXCB fields to (new) DM2U_TPCB, added
**	    DM2U_TPCB, DM2U_SPCB structures for partitioning,
**	    added thread synchronization stuff to MXCB for 
**	    dm2u_load_table() parallel partition creates/loads.
**	03-Mar-2004 (jenjo02)
**	    Added MX_REORG for parallel modify to reorg,
**	    externalized SpawnDMUThreads, DM2UReorg functions.
**	10-Mar-2004 (jenjo02)
**	    Add mx_Mlocations, mx_Mloc_count, mx_Moloc_count.
**	11-Mar-2004 (jenjo02)
**	    Add MX_CLOSED, MX_SYMMETRIC state flags,
**	    mx_tpcb_ptrs.
**	12-Mar-2004 (jenjo02)
**	    Add MX_TERMINATED state for TPCB threads.
**	31-Mar-2004 (schka24)
**	    Separate busy from parent-wakeup.
**	20-Apr-2004 (schka24)
**	    Add source record size, that's what goes thru the CUT buffers.
**	    Add pointer to target-signal record.
**	22-apr-2004 (thaju02)
**	    For online modify, added MX_ONLINE_MODIFY and MX_ONLINE_INDEX_BUILD.
**	    Added mx_online_tabid, mx_wloc_mask and mx_dupchk_rcb. 
**	10-may-2004 (thaju02)
**	    Added MX_ROLLDB_RESTART_LOAD. Removed unnecessary mx_wloc_mask.
**	15-Jul-2004 (jenjo02)
**	    Added MX_PRESORT state flag.
**	21-Jul-2005 (jenjo02)
**	    Add mx_phypart.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define MXCB_CB as DM_MXCB_CB.
**	13-Apr-2006 (jenjo02)
**	    Add mx_clustered.
**	30-May-2006 (jenjo02)
**	    mx_idom_map expanded to DB_MAXIXATTS to support
**	    indexes on clustered tables.
**	    Add mx_tidsize.
**	04-Nov-2008 (jonj)
**	    SIR 120874 Change i4 mx_err_code to DB_ERROR mx_dberr.
**	   
*/
struct _DM2U_MXCB
{
    DM2U_MXCB       *mx_next;           /* Next modify/index control block. */
    DM2U_MXCB	    *mx_prev;		/* Previous modify/index 
                                        ** control block. */
    SIZE_TYPE	    mx_length;		/* Length of control block. */
    i2		    mx_type;		/* Type of control block. */
#define			MXCB_CB		DM_MXCB_CB
    i2		    mx_i2_reserved;	/* Reserved for future use. */
    PTR		    mx_i4_reserved;	/* Reserved for future use. */
    PTR		    mx_owner;		/* The owner of this control block. */
    i4		    mx_ascii_id;	/* Ascii id to aid in debugging. */
#define			MXCB_ASCII_ID	CV_C_CONST_MACRO('M', 'X', 'C', 'B')
    i4	    	    mx_operation;	/* Either MX_MODIFY or MX_INDEX. */
#define			MX_MODIFY	1L
#define			MX_INDEX	2L
    DB_TAB_ID	    mx_table_id;	/* Table being operated on. */
    i4	    	    mx_lk_id;		/* Transaction lock list ID for utility routines. */
    i4	    	    mx_log_id;		/* Transaction log list ID for utility routines. */
    i4		    mx_db_lk_mode;	/* DB lock mode of source table(s) */
    i4		    mx_tbl_lk_mode;	/* Lock mode of source table(s) */
    i4		    mx_tbl_access_mode;	/* Access mode of source table(s) */
    i4		    mx_timeout;		/* Timeout value for dm2t_open() */
    DB_TRAN_ID	    *mx_tranid;		/* Transaction id */
    DMP_DCB	    *mx_dcb;		/* The DCB for the database. */
    DML_XCB	    *mx_xcb;		/* The XCB for this transaction. */

    /* The following are used when loading/reorging partitions */
    i4		    mx_spcb_count;	/* Number of sources */
    i4		    mx_spcb_threads;	/* Number of Source threads */
    i4		    mx_spcb_per_thread;
    DM2U_SPCB	    *mx_spcb_next;	/* List of sources */
    i4		    mx_tpcb_count;	/* Number of targets */
    i4		    mx_tpcb_threads;	/* Number of Target threads */
    i4		    mx_tpcb_per_thread;
    DM2U_TPCB	    *mx_tpcb_next;	/* List of targets */
    DM2U_TPCB	    **mx_tpcb_ptrs;	/* To find a TPCB by partno */
    DB_PART_DEF	    *mx_part_def;	/* Partitioning info for ADF */
    PTR 	    mx_phypart;		/* DMU partition definitions */
    DB_STATUS	    mx_status;		/* Collective thread status */
    DB_ERROR	    mx_dberr;		/* ...and error code */
    CS_SID	    mx_sid;		/* SID of query session */
    volatile i4	    mx_state;		/* State of Threading: */
#define		MX_PRESORT	0x00000002
#define		MX_PUTTOSORT	0x00000004
#define		MX_LOADFROMSORT	0x00000008
#define		MX_LOADFROMSRC	0x00000010
#define		MX_OPENED	0x00000020
#define		MX_CLOSED	0x00000040
#define		MX_REORG	0x00000080
#define		MX_ABORT	0x00000100
#define		MX_END		0x00000200
#define		MX_TERMINATED	0x00000400
#define		MX_THREADED	0x00008000	/* In a TPCB, parent only */
#define		MX_SYMMETRIC	0x00010000
    volatile i4	    mx_threads;		/* Number of threads */
    volatile i4	    mx_new_threads;	/* Number of initializing threads */
    CS_SEMAPHORE    mx_cond_sem;	/* For synchronizing
					** Source/Target threads */
    CS_CONDITION    mx_cond;
    DM2U_RHEADER    *mx_signal_rec;	/* Ptr to dummy record for stuffing
					** synchronous state at the targets */


    DMP_LOCATION    *mx_Mlocations;	/* Master Table new locations */
    i4		    mx_Mloc_count;	/* ...and their count */
    i4		    mx_Moloc_count;	/* Count of Master's old locations */

    DMP_RCB	    *mx_rcb;		/* RCB of Master or single source */
    DMP_RCB	    *mx_buildrcb;	/* RCB for table to build in-memory */
    DB_CMP_LIST	    *mx_cmp_list;	/* Pointer to list of attributes 
                                        ** for sorting. */
    i4	    	    mx_c_count;		/* Count of compare list entries. */
    DB_ATTS	    **mx_b_key_atts;	/* Pointer to list of key attribute
					** pointers for the base table. */ 
    DB_ATTS	    **mx_i_key_atts;	/* Pointer to list of key attribute
					** pointers in the index table. */
    DB_ATTS         *mx_atts_ptr;       /* Pointer to attributes for index 
					** table. */

    /* NOTE!  The ROWACCESS structures in the MXCB are used to hold att
    ** pointer arrays / counts, and compression types, but at present
    ** that is all -- the row accessors are never "set up".
    ** The reason for this is that BTREE index schemes can involve revising
    ** the attribute list for index vs leaf entries (due to non-key columns
    ** not being in the index).  That revising and jiggering is done in
    ** the MCT, not the MXCB, because dm2r_load needs it done too and
    ** dm2r doesn't use an MXCB!
    ** So the MXCB rowaccessors are copied to the MCT(s) and setup in
    ** the MCT.  (Every MCT for partitioned targets.)
    ** Unfortunately this leads to redundant setups (one per partition).
    ** Until LOAD and MODIFY/INDEX are further unified, we'll just have to
    ** live with this.
					*/

    DMP_ROWACCESS   mx_data_rac;	/* Data row row-accessor */
    DMP_ROWACCESS   mx_leaf_rac;	/* Leaf entry row-accessor (btree) */
    DMP_ROWACCESS   mx_index_rac;	/* Index entry row-accessor */
    i4	    	    mx_ab_count;	/* Count of base table key 
                                        ** attributes. */
    i4	    	    mx_ai_count;	/* Count of index key attributes. */
    i4	    	    *mx_att_map;	/*  New key sequence for column. */
    i4         	    mx_idom_map[DB_MAXIXATTS];
					/* Map of attributes in base table. */
    i4	    	    mx_width;		/* Width of record to load. */
    i4	    	    mx_kwidth;		/* Width of the key. */
    i4		    mx_src_width;	/* Width of sourced (base) record
					** This is what goes thru CUT */
    i4	    	    mx_structure;	/* The new storage structure. */
    i4	    	    mx_index_comp;	/* specifies index compression info */
    i4	    	    mx_unique;		/* ALL: Unique key. */
    i4      	    mx_dmveredo;        /* We are in rollforward redo */
    i4         	    mx_truncate;        /* ALL: convert to heap and truncate. */
    i4         	    mx_duplicates;      /* ALL: convert to allow duplicates. */
    i4         	    mx_reorg;            /* ALL: Reorganize only to different
                                        ** number of locations. */
    i4	    	    mx_dfill;		/* ALL:  Data page fill factor. */
    i4	    	    mx_ifill;		/* BTREE: Index page fill factor. */
    i4	    	    mx_lfill;		/* BTREE: Leaf page fill factor. */
    i4		    mx_clustered;	/* BTREE: CLUSTERED primary */
    i4	    	    mx_maxpages;	/* HASH: Maximum number of buckets. */
    i4	    	    mx_minpages;	/* HASH: Minimum number of buckets. */
    i4	    	    mx_buckets;		/* HASH: Number of buckets to create */
    i4		    mx_reltups;		/* number of tuples in the table to be
					** modified IF the table is S_CONCUR*/
    i4	    	    mx_allocation;	/* File allocation information. */
    i4	    	    mx_extend;		/* File extend information. */
    i4	    	    mx_page_size;	/* page size information */
    i4      	    mx_page_type;	/* page type information */
    i4	    	    mx_newrelstat;	/* New relation status. */
    i4	    	    mx_newtup_cnt;	/* New relation tuple counts. */
    i4		    mx_inmemory;	/* Indicator of inmemory build yes/no */
    u_i4	    mx_new_relstat2;	/* new relstat2 bits */
    i2		    mx_readonly;	/* Make table a readonly table */
#define		MXCB_SET_READONLY	1L	/* Zero is do nothing */
#define		MXCB_UNSET_READONLY	2L
    i2		    mx_pseudotemp;	/* Make table be pseudotemporary */
#define		MXCB_SET_PSEUDOTEMP	1L	/* Zero is do nothing */
#define		MXCB_UNSET_PSEUDOTEMP	2L
    i4		    mx_concurrent_idx;	/* concurrent index yes/no */
    i4		    mx_dimension;	/* RTREE: Dimension */
    i4		    mx_hilbertsize;	/* RTREE: hilbert size */
    f8		    mx_range[DB_MAXRTREE_DIM*2]; /* RTREE: Range */
    DMPP_ACC_KLV    *mx_acc_klv;	/* RTREE: Key level vectors */
    i4	    	    mx_tbl_pri;		/* Table cache priority */
    i4		    mx_tidsize;		/* Size of Target TID */

    /* The following fields are for online modify */
    i4              mx_flags;
#define         MX_ONLINE_MODIFY                0x0001L
#define         MX_ONLINE_INDEX_BUILD           0x0002L
#define         MX_ROLLDB_ONLINE_MODIFY		0x0004L
#define         MX_ROLLDB_RESTART_LOAD		0x0008L
    LG_LSN          mx_bsf_lsn;
    LG_LSN          mx_esf_lsn;
    DM2U_SPCB       *mx_resume_spcb;    /* used for rolldb of online 
					   modify only */
};

/*}
** Name: DM2U_SPCB - Source Partition Control Block
**
** Description:
**	Isolates partition-dependent values from the DM2U_MXCB
**	for MODIFY and CREATE INDEX, one for each Source
**	table or partition.
**
** History:
**	05-Feb-2004 (jenjo02)
**	    Created.
**	04-Mar-2004 (jenjo02)
**	    Added spcb_count;
**	08-Mar-2004 (jenjo02)
**	    Added spcb_locations, spcb_loc_count;
**	20-Apr-2004 (schka24)
**	    Remove unused tcb pointer;  make record point to rec header.
**	30-apr-2004 (thaju02)
**	    Added spcb_dupchk_rcb, spcb_dupchk_tabid, spcb_input_rcb.
*/
struct _DM2U_SPCB
{
    DM2U_SPCB	    *spcb_next;		/* Next in MXCB list */
    DM2U_MXCB	    *spcb_mxcb;		/* Resultant table's
					** common characteristics */
    DB_TAB_ID	    spcb_tabid;		/* Source's table id */
    i4		    spcb_partno;	/* Source's partition number */
    DMP_RCB	    *spcb_rcb;		/* Source's RCB, once opened */
    i2		    spcb_positioned;	/* TRUE when RCB positioned */
    i2		    spcb_count;		/* Number of SPCBs DMUSource
					** is responsible for */
    i4		    spcb_lk_id;		/* Lock id */
    i4		    spcb_log_id;	/* Log id */
    DM2U_RHEADER    *spcb_record;	/* Pointer to a space to
					** hold a source header+row */
    DMP_LOCATION    *spcb_locations;	/* Source locations, */
    i4		    spcb_loc_count;	/* ... and their number */

    /* following are for online modify */
    DMP_RCB	    *spcb_input_rcb;    
    DB_TAB_NAME     spcb_rnl_tabname;
    DB_OWN_NAME     spcb_rnl_owner;
    DMP_RNL_ONLINE  spcb_rnl_online;
};

/*}
** Name: DM2U_TPCB - Target Partition Control Block
**
** Description:
**	Isolates partition-dependent values from the DM2U_MXCB
**	for MODIFY and CREATE INDEX, one for each Target
**	table or partition.
**
**	Depending on the Source and Target configurations
**	and the flavor of MODIFY or CREATE INDEX,
**	there may be one or more Sources (SPCB) feeding
**	one or more Targets (TPCB).
**
**	The TPCB as it stands really should be split up, since
**	a target thread may actually be handling multiple targets.
**	The first TPCB for a thread is the controller (parent).
**	The conditional semaphore, condvar, and waiter count are
**	only meaningful in the parent TPCB, even though they exist
**	in all of them.
**
** History:
**	05-Feb-2004 (jenjo02)
**	    Created.
**	04-Mar-2004 (jenjo02)
**	    Added tpcb_count;
**	08-Mar-2004 (jenjo02)
**	    Deleted tpcb_olocations, tpcb_oloc_count;
**	    Added tpcb_parent.
**	31-Mar-2004 (schka24)
**	    Comment note re controller (parent) TPCB vs ordinary ones.
**	20-Apr-2004 (schka24)
**	    Deleted stuff taken over by CUT interfacing.
**	17-May-2004 (jenjo02)
**	    Deleted tpcb_tid8, no longer used.
*/
struct _DM2U_TPCB
{
    DM2U_TPCB	    *tpcb_next;		/* Next in MXCB list */
    DM2U_MXCB	    *tpcb_mxcb;		/* Resultant table's
					** common characteristics */
    DB_TAB_ID	    tpcb_tabid;		/* Target's table id */
    DM2U_TPCB	    *tpcb_parent;	/* Responsible threaded TPCB */
    i4		    tpcb_partno;	/* Target's partition */
    i4		    tpcb_lk_id;		/* Lock id */
    i4		    tpcb_log_id;	/* Log id */
    DMP_RCB	    *tpcb_buildrcb;	/* RCB for table to build
					** in memory */
    DMS_SRT_CB	    *tpcb_srt;		/* Sort context */
    i4		    *tpcb_srt_wrk;	/* Sort work locations */
    i4		    tpcb_new_locations; /* New location(s)? */
    i4		    tpcb_newtup_cnt;	/* Tuple count */
    i4		    tpcb_oloc_count;	/* Count of old locations */
    DMP_LOCATION    *tpcb_locations;	/* Locations of the target
					** table */
    i4		    tpcb_loc_count;	/* ...and their number */
    DB_LOC_NAME	    *tpcb_loc_list;	/* Location names */
    i2		    tpcb_count;		/* Number of TPCBs DMUThread
					** is responsible for */
    i2		    tpcb_name_id;	/* For constructing */
    i4		    tpcb_name_gen;	/* filenames */
    char	    *tpcb_crecord;	/* Pointer to a space to
					** make a compressed row */
    char	    *tpcb_irecord;	/* Pointer to a space to
					** make an index row */
    i4		    tpcb_state;		/* State of Target,
					** same defines as mx_state, set
					** by thread, not outside world
					*/
    /* The next few things are *ONLY* meaningful in the controller/parent */
    CS_SID	    tpcb_sid;		/* If threaded, thread's SID */
    PTR		    tpcb_buf_handle;	/* Handle to input buffer */
    DM2U_RHEADER    *tpcb_srecord;	/* Holder for record out of input
					** buffer, or from sort */
    /* End of stuff that applies only to the parent TPCB */

    DM2U_M_CONTEXT  tpcb_mct;		/* The context used by the 
                                        ** load routines. */
    /* online modify stuff */
    DB_TAB_ID	    *tpcb_dupchk_tabid;
    DMP_RCB	    *tpcb_dupchk_rcb;
};


/*}
** Name: DM2U_RHEADER - Record header for DM2U operations
**
** Description:
**	When dm2u-load-table is passing data from a source thread to a
**	target thread, each record is prefixed by a header.  This header
**	allows the source to pass some pre-computed stuff (such as a
**	partition number) to the target, so that the target doesn't need
**	to recompute it.  The header also contains some state flags,
**	which are used to control the targets synchronously.
**
**	The state flags are a subset of the MX_xxx flags defined for
**	mx_state in the MXCB.  The relevant flags are:
**	MX_PUTTOSORT	Put this record to the sorter.
**	MX_LOADFROMSRC	Put this record directly to a target.
**	MX_LOADFROMSORT	Always sent separately, prefixed to a dummy
**			record.  This indicates EOF on all sources relevant
**			to this target, and tells the thread to complete
**			sorting, and dump the sort to the result.
**	MX_END		Always sent separately, prefixed to a dummy record.
**			This indicates EOF on all sources relevant to this
**			target, and tells the thread to close up shop.
**
**	MX_LOADFROMSORT and MX_END apply to all targets controlled by a
**	target-thread, so you only send them once per target thread.
**
**	The MX_ABORT signal is not sent via the record header, it's sent
**	asynchronously via CUT's signal-status mechanism.
**
**	The partition number in the record header simply indicates which
**	target partition the row is for.  (Partition numbers fit in a
**	u_i2, but we'll declare an i4 to keep things nicely aligned.)
**
**	The tid8 in the record header is needed when the row is being
**	sent to the sorter, since it may be needed for constructing
**	index tuples.
**
** History:
**	19-Apr-2004 (schka24)
**	    Invent for sending modify data thru CUT.
*/

struct _DM2U_RHEADER
{
	i4	rhdr_state;		/* MX_xxx state indicator */
	i4	rhdr_partno;		/* Partition number if relevant */
	DM_TID8	rhdr_tid8;		/* Tid of source row if relevant */
};

/* External functions for DMU threading */
FUNC_EXTERN i4		SpawnDMUThreads(
				DM2U_MXCB	*m,
#define		SOURCE_THREADS		1
#define		TARGET_THREADS		2
				i4		thread_type);

FUNC_EXTERN DB_STATUS	DM2UReorg(
				DM2U_SPCB	*sp,
				DB_ERROR	*dberr);

struct _DM2U_OSRC_CB
{
    DM2U_OSRC_CB    *osrc_next;
    DMP_RCB         *osrc_master_rnl_rcb;
    DMP_RCB         *osrc_rnl_rcb;
    i4              osrc_partno;
    DB_TAB_ID       osrc_tabid;
    DB_TAB_NAME     osrc_rnl_tabname;
    DB_OWN_NAME     osrc_rnl_owner;
    DMP_RNL_ONLINE  osrc_rnl_online;
    DMP_MISC        *osrc_misc_cb;
};

struct _DM2U_OPCB
{
    DM2U_OPCB       *opcb_next;
    DMP_RCB         *opcb_master_rcb;  /* $online master rcb */
    DMP_RCB         *opcb_rcb;	   /* $online partition rcb */
    DB_TAB_ID       opcb_tabid;	   /* partition tabid */
    i4		    opcb_tupcnt_adjust;
};

struct _DM2U_OMCB
{
    DM2U_OSRC_CB    *omcb_osrc_next;  /* input list of partitions */
    DM2U_OPCB       *omcb_opcb_next;
    DB_TAB_ID       *omcb_dupchk_tabid; /* list of duplicate check tbl tabids,
				       indexed by partno */
    LG_LSN          omcb_bsf_lsn;
    PTR             omcb_octx;
};
