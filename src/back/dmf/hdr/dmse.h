/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DMSE.H - Sort Execution
**
** Description:
**      This file describes the sort execution module.
**
** History:
**      07-dec-1986 (derek)
**          Created new for jupiter.
**	17-apr-1991 (bryanp)
**	    B36569: Added SRT_BUFFER_IN_MEMORY to srt_state flags.
**	22-jul-1991 (rogerk)
**	    Integrated Jennifer's fix to avoid pre-allocating super-large
**	    sort files.  Add new sort flag - DMSE_EXTERNAL - which indicates
**	    that the rows are being fed to the sorter from an outside
**	    source.  In this case we do not trust the row_count estimate
**	    and do not try to pre-allocate the space for the sort files.
**	    We start with a small amount (64 pages) and add space at runtime
**	    as required.  This change was made to fix Bug # 34235.
**	16-jan-1992 (bryanp)
**	    Added SRT_HEAP_BECAME_FULL flag for in-memory sorting.
**	6-jul-1992 (bryanp)
**	    Prototyping DMF.
**	22-oct-1992 (rickh)
**	    Replaced all references to DM_CMP_LIST with DB_CMP_LIST.
**	03-nov-92 (jrb)
**	    Major changes for ML Sorts.
**	14-dec-92 (jrb)
**	    Changed proto to reflect VOID return type for dmse_cost.
**	16-oct-93 (jrb)
**          Change proto for dmse_begin to add spill_index parm, and added it
**	    to the sort cb too.
**	25-Aug-1997 (jenjo02)
**	    Added s_log_id to DMS_SRT_CB, log_id parm to dmse_begin() prototype.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-may-2001 (stial01)
**          Added ucollation parameter to dmse_begin
**      08-Oct-2002 (hanch04)
**          Replace the i4 for length with a SIZE_TYPE (size_t) to allow
**          64-bit memory allocation.
**	04-Apr-2003 (jenjo02)
**	    Added table_id to dmse_begin() prototype.
**      09-mar-2004 (stial01)
**          Refine width, size fields in DMS_MERGE_DESC as i4 for 256k support.
**	31-Mar-2004 (jenjo02)
**	    Restructured to use CS_CONDITION objects for 
**	    Parent/Child thread synchronization.
**	14-Apr-2005 (jenjo02)
**	    Added "cmp_count" parm to dmse_begin prototype.
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**	27-july-06 (dougi)
**	    Change some parms to dmse_cost() to float and define 
**	    DMSE_SORT_CPU/IOFACTOR constants.
**	25-Nov-2008 (jonj)
**	    SIR 120874: dmse_? functions converted to DB_ERROR *
**	17-Nov-2009 (kschendel) SIR 122890
**	    Double the max merge per pass.
**	23-Mar-2010 (kschendel) SIR 123448
**	    Add no-parallel-sort request flag, for partitioned bulk-loads.
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    dmse_compare() is obsolete, remove prototype.
**/

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _DMS_SRT_CB	DMS_SRT_CB;
typedef struct _DMS_RUN_DESC	DMS_RUN_DESC;
typedef	struct _DMS_MERGE_DESC	DMS_MERGE_DESC;

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN DB_STATUS dmse_begin( 
			    i4		flags,
			    DB_TAB_ID	*table_id,
			    DB_CMP_LIST		*att_list,
			    i4		att_count,
			    i4		key_count,
			    i4		cmp_count,
			    DMP_LOC_ENTRY	*loc_array,
			    i4		*wrkloc_mask,
			    i4		mask_size,
			    i4		*spill_index,
			    i4		width,
			    i4		records,
			    i4		*uiptr,
			    i4		lock_list,
			    i4		log_id,
			    DMS_SRT_CB		**sort_cb,
			    PTR			collation,
			    PTR			ucollation,
			    DB_ERROR		*dberr);

FUNC_EXTERN VOID      dmse_cost(f4 records, i4 width,
			    f4 *io_cost, f4 *cpu_cost);

FUNC_EXTERN DB_STATUS dmse_end(DMS_SRT_CB *sort_cb, DB_ERROR *dberr);

FUNC_EXTERN DB_STATUS dmse_get_record(DMS_SRT_CB *sort_cb,
			    char **record, DB_ERROR *dberr);

FUNC_EXTERN DB_STATUS dmse_input_end(DMS_SRT_CB *sort_cb, DB_ERROR *dberr);

FUNC_EXTERN DB_STATUS dmse_put_record(DMS_SRT_CB *sort_cb,
			    char *record, DB_ERROR *dberr);

/*
**  Defines of other constants.
*/

#define		    MIN_MERGE		    2
#define		    MAX_MERGE		    30
#define		    MIN_BLOCK_SIZE	    4096
#define		    FILENAME_LENGTH	    12
#define		    MAX_SRT_NAME	    99

#define		    SRT_F_INPUT		    0
#define		    SRT_F_OUTPUT	    1

#define		    SRT_F_READ		    0
#define		    SRT_F_WRITE		    1

#define DMSE_SORT_CPUFACTOR		1100.0
#define DMSE_SORT_IOFACTOR		1.95

/*
**      Flag values for dmse_begin.
*/
#define                 DMSE_CHECK_KEYS		0x01
#define			DMSE_ELIMINATE_DUPS	0x02
#define			DMSE_EXTERNAL		0x04
						/* Row estimates were generated
						** externally, so don't trust
						** them to be accurate.
						*/ 
#define			DMSE_PARALLEL		0x08
						/* Caller is parent thread 
						** trying to start parallel sort.
						*/
#define			DMSE_CHILD		0x10
						/* Caller is child thread
						*/
#define			DMSE_NOPARALLEL		0x20
						/* Don't do parallel sort */

/*}
** Name: DMS_SRT_CB - The sort control block.
**
** Description:
**      Contains the contexted needed to manage a sort.
**
** History:
**     05-feb-1986 (derek)
**          Created new for jupiter.
**	17-apr-1991 (bryanp)
**	    B36569: Added SRT_BUFFER_IN_MEMORY to srt_state flags.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warnings.
**	18-feb-1992 (bryanp)
**	    Added SRT_HEAP_BECAME_FULL flag for in-memory sorting.
**	04-nov-92 (jrb)
**	    Removed some fields related to single input and output merge files
**	    and added fields for multiple location sorting.
**      16-oct-93 (jrb)
**	    Added srt_spill_index to support using a subset of work locs.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**      23-may-1994 (markm)
**          Added field "blkno" to DMS_RUN_DESC structure to allow
**          sort file total to be greater than MAXI4 (60023).
**	10-dec-97 (inkdo01)
**	    Added sevberal new fields - srt_c_comps for dm501 statistics,
**	    srt_c_free, srt_free for free chain support and srt_inmem_rem
**	    to support the algorithmic modifications for in-memory sorts.
**	15-jan-98 (inkdo01)
**	    Added SRT_PARALLEL flag to indicate a parallel sort execution.
**	    Gobs of other changes are to come.
**	23-nov-98 (inkdo01)
**	    Added srt_sem as semaphore to coordinate parent/child flag
**	    setting and suspend/resume.
**	04-Apr-2003 (jenjo02)
**	    Added srt_sid (session id).
**	    Added srt_pstate and flags for parallel sort.
**	14-Apr-2005 (jenjo02)
**	    Added srt_c_count.
**	28-Apr-2005 (jenjo02)
**	    Added srt_rac_count.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define SORT_CB as DM_SORT_CB
**	28-Jan-2006 (kschendel)
**	    Reshuffle a couple things for better LP64 alignment.
**	12-Feb-2007 (kschendel)
**	    Added needs-csswitch flag so that cancels are seen during
**	    sort merging.  The csswitch check is only needed if it's a
**	    top-level, non-parallel sort.
**	25-Nov-2008 (jonj)
**	    SIR 120874: srt_err_code replaced with srt_dberr,
**	    srt_perr_code replaced with srt_pdberr.
*/
struct _DMS_SRT_CB
{
    DMS_SRT_CB      *srt_q_next;        /* Next sort control block. */
    DMS_SRT_CB	    *srt_q_prev;	/* Previous sort control block. */
    SIZE_TYPE	    srt_length;		/* Length of the control block. */
    i2		    srt_type;		/* Type of control block. */
#define			SORT_CB		DM_SORT_CB
    i2		    srt_s_reserved;
    PTR		    srt_l_reserved;
    PTR    	    srt_owner;		/*  Owner of the control block. */
    i4	    	    srt_ascii_id;	/*  Ascii identifier for debugging. */
#define			SORT_ASCII_ID	CV_C_CONST_MACRO('S', 'O', 'R', 'T')
    i4	    	    srt_state;		/*  The state of the SORT CB. */
#define			SRT_PUT			0x0001L
#define			SRT_OUTPUT		0x0002L
#define			SRT_GET			0x0004L
#define			SRT_GETFIRST		0x0008L
#define			SRT_DUPLICATE		0x0010L
#define			SRT_CHECK_DUP		0x0020L
#define			SRT_CHECK_KEY		0x0040L
#define			SRT_ELIMINATE		0x0080L
#define			SRT_BUFFER_IN_MEMORY	0x0100L
#define			SRT_HEAP_BECAME_FULL	0x0200L
#define			SRT_PARALLEL		0x0400L
#define			SRT_NEED_CSSWITCH	0x0800L
    CS_SID  	    srt_sid;		/* Session owning this CB */
    DB_ERROR   	    srt_dberr;		/*  Error return code from 
                                        **  internal routine. */
    i4	    	    srt_blocks;		/*  Initial estimate of number 
                                        **  of blocks. */ 
    i4	    	    **srt_heap;		/*  Pointer to the heap pointer 
                                        **  array. */
    char	    *srt_record;	/*  Pointer to the record space. */
    char	    *srt_duplicate;	/*  Next to last record for 
                                        **  duplicate elimination. */
    i4	    	    srt_next_record;	/*  Next available record slot. */
    i4	    	    srt_last_record;	/*  Last slot avaiable. */
    DB_CMP_LIST	    *srt_atts;		/*  Pointer to array of 
                                        **  comparison attributes. */
    DB_CMP_LIST	    *srt_ratts;		/*  Pointer to array of 
                                        **  comparison attributes including
					**  the run number.  
                                        **  Used for initial run creation. */
    i4	    	    srt_a_count;	/*  Number of comparison attributes. */
    i4	    	    srt_k_count;	/*  Number of attributes in key. */
    i4	    	    srt_c_count;	/*  Number of "compare" attributes
					**  to consider when SRT_ELIMINATE */
    i4	    	    srt_ra_count;	/*  Number of run comparision 
                                        **  attributes. */
    i4	    	    srt_rac_count;	/*  Number of run compare attributes
					**  to consider when SRT_ELIMINATE */
    i4	    	    srt_width;		/*  Callers record size. */
    i4	    	    srt_iwidth;		/*  Internal record size. */
    i4	    	    srt_run_number;	/*  Current run number. */
    char	    *srt_o_buffer;	/*  Address of sort output buffer. */
    i4	    	    srt_o_size;		/*  Size of the sort output buffer. */
    i4	    	    srt_o_offset;	/*  Offset into the output buffer. */
    i4	    	    srt_o_blkno;	/*  Block number for this buffer. */
    i4	    	    srt_o_records;	/*  Number of records write to run. */
    i4	    	    srt_o_rnext;	/*  Next output run number. */
    DB_TAB_ID	    srt_table_id;	/*  Id of base table being sorted */
    i4	    	    srt_i_size;		/*  Size of merge input buffer. */
    i4	    	    srt_i_rnext;	/*  Next input run number. */
    DMP_MISC	    *srt_misc;		/*  Run creation/Merge  work area. */
    ADF_CB	    srt_adfcb;		/*  Preinitialized ADF CB. */
    struct _DMS_MERGE_DESC
    {
      char	    *record;		/*  The returned record. */
      char	    *in_buffer;		/*  Input buffer pointer. */
      struct _DMS_SRT_CB
		    *sort_cb;		/*  Pointer back to SORT_CB. */
      char	    *rec_buffer;	/*  Scratch buffer for read. */
      i4	    in_offset;		/*  Offset into input buffer. */
      i4	    in_count;		/*  Number of records in run. */
      i4	    in_blkno;		/*  Input block number. */
      i4	    in_width;		/*  Input record width. */
      i4	    in_size;		/*  Input block size. */
    }		    *srt_i_merge;	/*  merge input run descriptions. */
    struct _DMS_RUN_DESC
    {
      i4	    next_block;		/*  Block number of next RUN_DESC. */
      i4	    run_count;		/*  Count of runs in the descriptor. */
      struct
      {
	i4	    offset;		/*  Offset to first record. */
	i4     	    blkno;              /*  Block number */
	i4	    records;		/*  Count of records. */
      }		    runs[MAX_MERGE];
    }		    srt_orun_desc;	/*  Output run descriptor. */
    DMS_RUN_DESC    srt_irun_desc;	/*  Input run descriptor. */

    i4	    	    srt_c_reads;	/*  Number of read I/O's. */
    i4	    	    srt_c_writes;	/*  Number of write I/O's. */
    i4	    	    srt_c_records;	/*  Number of records. */
    u_i4    	    srt_c_comps;  	/*  Number of comparisons */
    i4	    	    srt_p_length;	/*  Length of path name. */
    char	    srt_path[128];	/*  Path name of sort area. */
    i4         	    *srt_uiptr;         /*  Pointer to area to look to 
                                        **  see if user interrupt occurred. */
    i4	    	    srt_lk_id;		/*  Lock list of current transaction */
    i4	    	    srt_log_id;		/*  Log transaction id of current transaction */
    i4	    	    srt_swapped;	/*  Initially FALSE; if TRUE then
					**  input and output files are swapped
					*/
    i4	    	    srt_num_locs;	/* Number of sort locations available */
    i4	    	    srt_split_loc;	/* Location which is split between
    					** input and output run files (set
					** to -1 if none)
					*/
    DMP_LOC_ENTRY   *srt_array_loc;	/* Array of locations in DCB */
    i4	    	    *srt_wloc_mask;	/* Bit mask for above giving wrk locs */
    i4	    	    srt_mask_size;	/* Number of bits in the above */
    i4	    	    srt_spill_index;	/* Which work loc to use first */
    SR_IO	    *srt_sr_files;	/* Pointer to array of SR contexts */
    char	    *srt_free;		/* anchor of free chain (if dups
					** elimination sort) */
    i4	    	    srt_c_free;		/* count of elements on free chain */
    i4	    	    srt_inmem_rem;	/* count of remaining in mem rows */
    struct _DM_STECB *srt_stecbp[MAX_MERGE];  /* DM_STECB ptr vector (for 
					** parallel sorts) */
    i4	    	    srt_nthreads;	/* # child threads created */
    i4		    srt_threads;	/* # threads still alive */
    i4		    srt_init_active;	/* # threads initializing */
    i4		    srt_puts_active;	/* # threads in "PUTs" */
    i4	    	    srt_childix;	/* index of DM_STECB last put to */

    DB_STATUS	    srt_pstatus;	/* Collective status */
    DB_ERROR  	    srt_pdberr;		/* ...and error info */
    i4	    	    srt_secs;
    i4	    	    srt_msecs;
    i4	    	    srt_pstate;		/* Parallel sort state: */
#define		SRT_PERR	0x0001L /* Parent error */
#define		SRT_CERR	0x0002L /* Some child had an error */
#define		SRT_PCERR	(SRT_PERR | SRT_CERR) /* Doom */
#define		SRT_PEND	0x0004L /* Parent at end of sort */
#define		SRT_PEOF	0x0008L /* Parent at end of input */
#define		SRT_PWAIT	0x0010L /* Parent waiting on child(ren) */
    CS_SEMAPHORE    srt_cond_sem;	/* semaphore ... */
    CS_CONDITION    srt_cond;		/* ... and cond for synch */
};
