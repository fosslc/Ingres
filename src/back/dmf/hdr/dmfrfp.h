/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: DMFRFP.H - Used by Rollforwarddb.
**
** Description:
**	This file contains the header information of the dmfrfp.c
**	module.
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
**      04-jan-1995 (alison)
**          Removed extern for rfp_relocate()
**      16-jan-1995 (stial01)
**          BUG 66577,66581: Added more status bits to tblcb_table_status for 
**          gateway, view, secondary index and blob processing.  
**          BUG 66407: Added more status bits to indicate recovery disallowed 
**          for table.
**	09-mar-1995 (shero03)
**	    Bug b67276
**	    Add location information for ckp, dmp, and jnl
**      10-mar-1995 (stial01)
**          BUG 67411: Added rfp_locmap.
**	20-jul-1995 (thaju02)
**	    bug #69981: Added RFP_IGNORED_ERR.
**	 7-feb-1996 (nick)
**	    Added RFP_FOUND_TABLE to RFP_TBLCB.
**	20-feb-1997(angusm)
**	    Add extra fields to DMF_RFP to allow retention of
**	    locations added since checkpoint (bug 49057)
**	20-sep-1996 (shero03)
**	    Support rtput, rtdel, and rtrep
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-feb-2001 (somsa01)
**	    Synchronized tx_l_reserved and tx_owner with DM_SVCB.
**      08-Oct-2002 (hanch04)
**          Replace the i4 for length with a SIZE_TYPE (size_t) to allow
**          64-bit memory allocation.
**      22-jan-2003 (stial01)
**          Added tx_bt_time, test_dump_err,test_redo_err,test_undo_err
**      27-jan-2003 (stial01)
**          Added more fields for journal record error processing.
**      11-feb-2003 (stial01)
**          Added test_excp_err for testing exception during jnl processing
**          Renamed DMF_RFP fields used to save redo end lsn.
**	5-Apr-2004 (schka24)
**	    Handle rawdata log records for modify rollforward.
**	10-may-2004 (thaju02)
**	    Removed unnecessary octx_lastpage, octx_modify and octx_lsn_wait.
**	30-aug-2004 (thaju02) Bug 111470 INGSRV2635
**	    Added RFP_BQTX struct, rfp_bqtx_count, rfp_bqtxh hash queue. 
**	01-Sep-2004 (bonro01)
**	    Fix x-integration of change 471359; replace longnat with i4
**	10-jan-2005 (thaju02)
**	    Moved RFP_OCTX to dmve.h.
**      07-apr-2005 (stial01)
**          Changes for queue management in rollforwarddb
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**      20-mar-2006 (stial01)
**          Added jnl_history_number
**      12-apr-2007 (stial01)
**          Added support for rollforwarddb -incremental
**      09-oct-2007 (stial01)
**          Added RFP_DID_JOURNALING
**      23-oct-2007 (stial01)
**          Support table level CPP/RFP for partitioned tables (b119315)
**	10-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code.
*/

/*
**  This section includes all forward references required
*/
typedef struct _DMF_RFP		DMF_RFP;	/* Rollforward context. */
typedef struct _RFP_TX		RFP_TX;		/* Transaction hash array. */
typedef struct _RFP_BQTX	RFP_BQTX;	/* Transaction array. */
typedef struct _RFP_TBLHCB	RFP_TBLHCB;
typedef struct _RFP_TBLCB	RFP_TBLCB;
typedef struct _TBL_HASH_ENTRY  TBL_HASH_ENTRY;
typedef struct _RFP_LOC_MASK    RFP_LOC_MASK;
typedef struct _RFP_STATS_BLK   RFP_STATS_BLK;
typedef struct _RFP_SIDEFILE	RFP_SIDEFILE;
typedef struct _RFP_LSN_WAIT	RFP_LSN_WAIT;
typedef struct _RFP_QUEUE	RFP_QUEUE;

/*}
** Name: RFP_QUEUE - RFP queue definition.
**
** Description:
**      This structure defines the RFP queue.
**
** History:
**     07-apr-2005 (stial01)
**          Created.
*/
struct _RFP_QUEUE
{
    RFP_QUEUE		*q_next;	    /*	Next queue entry. */
    RFP_QUEUE		*q_prev;	    /*  Previous queue entry. */
};


/*
** Name: RFP_TBLCB - Rollforward control block holds info on a table being
**		     recovered.
**
** Description:
**
**
** History
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**	  	Created.
**      20-jul-1995 (thaju02)
**          bug #69981: Added RFP_IGNORED_ERR.
**	 7-feb-1996 (nick)
**	    Added RFP_FOUND_TABLE
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define TBL_CB as DM_TBL_CB.
*/
struct    _RFP_TBLCB
{
    RFP_TBLCB	    *tblcb_q_next;	     /* TBL hash queue for base */
    RFP_TBLCB	    *tblcb_q_prev;
    SIZE_TYPE	    tblcb_length;
    i2		    tblcb_type;
#define	    TBL_CB			DM_TBL_CB
    i2		    tblcb_s_reserved;
    PTR		    tblcb_l_reserved;
    PTR		    tblcb_owner;
    i4	    tblcb_ascii_id;
#define	RFP_TBLCB_ASCII_ID		 CV_C_CONST_MACRO('#', 'T', 'B', 'L')
    RFP_QUEUE	    tblcb_totq;			/* Queue of all tables */
    RFP_QUEUE	    tblcb_invq;			/* Queue of invalid TCB's */
    DB_TAB_NAME     tblcb_table_name;
    DB_OWN_NAME     tblcb_table_owner;
    DB_TAB_ID       tblcb_table_id;
    DMP_TCB	    *tblcb_tcb_ptr;
    DB_STATUS       tblcb_table_err_status;
    i4         tblcb_table_err_code;
    u_i4       tblcb_table_status;
#define	       RFP_TABLE_OK	       0x0000L  /* Default status */
#define        RFP_RECOVER_TABLE       0x0001L  /* Ok to recover */
#define        RFP_INVALID_TABLE       0x0002L  /* No info avail */
#define        RFP_FAILED_TABLE        0x0004L  /* Failed recovering */
#define	       RFP_CATALOG             0x0008L  /* System Catalog table */
#define	       RFP_USER_SPECIFIED      0x0010L  /* User specified table */
#define        RFP_VIEW                0x0020L  /* View                 */
#define        RFP_GATEWAY             0x0040L  /* Gateway table        */
#define        RFP_RECOV_DISALLOWED    0x0080L  /* Recov disallowed     */
#define        RFP_INDEX_REQUIRED      0x0100L  /* This is an index for */
						/* a user specified tbl */
#define        RFP_BLOB_REQUIRED       0x0200L  /* This is a blob for   */
						/* a user specified tbl */
#define        RFP_IGNORED_ERR         0x0400L  /* tbl rolldb -ignore   */
#define        RFP_FOUND_TABLE         0x1000L	/* found tbl in list file */
#define        RFP_IGNORED_DMP_JNL     0x2000L  /* ignore jnl/dmp records */
#define        RFP_IS_PARTITIONED      0x4000L  /* Partitioned table */
#define        RFP_PART_REQUIRED       0x8000L  /* This is a partition for */
						/* a user specified tbl */
#define        RFP_PARTITION          0x10000L  /* Table is a partition */
     DB_TAB_TIMESTAMP
                     tblcb_stamp12;             /* Create or modify Timestamp */
     i4              tblcb_moddate;             /* Date table last modified. */
};

/*
** Name: RFP_TBLHCB - Rollforward internal control block holding information
**                    on tables being recovered.
**
** Description:
**
**
** History
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define TBL_HCB_CB as DM_TBL_HCB_CB.
*/
struct      _RFP_TBLHCB
{
    RFP_TBLCB	    *tblhcb_q_next;	
    RFP_TBLCB	    *tblhcb_q_prev;
    SIZE_TYPE	    tblhcb_length;
    i2		    tblhcb_type;
#define		TBL_HCB_CB		DM_TBL_HCB_CB
    i2              tblhcb_s_reserved;         /* Reserved for future use. */
    PTR             tblhcb_l_reserved;         /* Reserved for future use. */
    PTR             tblhcb_owner;              /* Owner of control block for
                                               ** memory manager.  HCB will be
                                               ** owned by the server. 
					       */
    i4         tblhcb_ascii_id;           
#define  RFP_TBLHCB_ASCII_ID    	CV_C_CONST_MACRO('#', 'H', 'C', 'B')
    i4	    tblhcb_no_hash_buckets;
    RFP_QUEUE	    tblhcb_htotq;		/* Queue of all tables */
    RFP_QUEUE	    tblhcb_hinvq;		/* Queue of invalid TCB's */
    struct    _TBL_HASH_ENTRY
    {
	RFP_TBLCB   *tblhcb_headq_next;
	RFP_TBLCB   *tblhcb_headq_prev;
    } tblhcb_hash_array[1];
};

/*
** Name: RFP_TBLHCB - Rollforward internal control block holding information
**                    on tables being recovered.
**
** Description:
**
**
** History
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define RFP_LOCM_CB as DM_RFP_LOCM_CB.
*/
struct      _RFP_LOC_MASK
{
    i4         *rfp_locm_next;         /* Not used. */
    i4         *rfp_locm_prev;         /* Not used. */
    SIZE_TYPE       rfp_locm_length;        /* Length of control block. */
    i2              rfp_locm_type;          /* Type of control block for
                                            ** memory manager. 
					    */
#define                 RFP_LOCM_CB         DM_RFP_LOCM_CB
    i2              rfp_locm_s_reserved;    /* Reserved for future use. */
    PTR             rfp_locm_l_reserved;    /* Reserved for future use. */
    PTR             rfp_locm_owner;         /* Owner of control block for
                                            ** memory manager.  SVCB will be
                                            ** owned by the server. 
					    */
    i4         rfp_locm_ascii_id;      
#define           RFP_LOCM_ASCII_ID       CV_C_CONST_MACRO('L', 'O', 'C', 'M')

/*
** The folloing field points to a bitmap of locations which will be used
** during recovery.
** The map should be accessed using the BT calls in the CL; bit 0 corresponds
** to extent array element 0, and so forth.
**
** The space for the bit maps will be allocated dynamically at the end of this
** structure as soon as the number of config file extents is known.
*/
    i4     locm_bits;          /* number of bits in each following field */
    i4     *locm_r_allow;      /* locs used during the recovery */
};

/*
** Name: RFP_STATS_BLK - Hold interesting stats about the rollforward
**
** Description:
**
**
** History
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**	 	Created.
*/
struct    _RFP_STATS_BLK
{
    i4		rfp_total_dump_recs;
    i4		rfp_applied_dump_recs;
    i4		rfp_total_recs;
    i4		rfp_applied_recs;
    i4	rfp_bt;
    i4	rfp_et;
    i4 rfp_assoc;
    i4	rfp_alloc;
    i4	rfp_extend;
    i4	rfp_ovfl;
    i4	rfp_nofull;
    i4	rfp_fmap;
    i4	rfp_put;
    i4	rfp_del;
    i4	rfp_btput;
    i4	rfp_btdel;
    i4	rfp_btsplit;
    i4	rfp_btovfl;
    i4	rfp_btfree;
    i4	rfp_btupdovfl;
    i4	rfp_dealloc;
    i4	rfp_ai;
    i4	rfp_bi;
    i4	rfp_rep;
    i4	rfp_create;
    i4	rfp_crverify;
    i4	rfp_destroy;
    i4	rfp_index;
    i4	rfp_modify;
    i4	rfp_relocate;
    i4	rfp_alter;
    i4	rfp_fcreate;
    i4	rfp_frename;
    i4	rfp_dmu;
    i4 rfp_location;
    i4	rfp_extalter;
    i4	rfp_sm0closepurge;
    i4	rfp_sm1rename;
    i4	rfp_sm2config;
    i4	rfp_rtput;
    i4	rfp_rtdel;
    i4	rfp_rtrep;
    i4	rfp_other;
	};


/*}
** Name: DMF_RFP - Rollforward context.
**
** Description:
**      This structure contains information needed by the rollforward routines.
**
** History:
**       4-nov-1988 (Sandyh)
**          Created for Jupiter.
**	10-jan-1990 (rogerk)
**	    Added jnl_count, dmp_count, rfp_jnl_history, and rfp_dmp_history
**	    so that entire journal and dump histories could be saved rather
**	    than just the values in the last entry.
**	    Added jnl_fil_seq and dmp_fil_seq to save the current file
**	    sequence numbers rather than just the value in the last history
**	    entry, which may be zero.
**	7-may-90 (bryanp)
**	    Added ckp_seq to store the checkpoint sequence number which is
**	    being used. This is different from ckp_jnl_sequence and
**	    ckp_dmp_sequence in the case where we are rolling back to other
**	    than the most recent checkpoint (i.e., if #c is used).
**	    Changed the meaning and usage of 'jnl_first' and 'jnl_last' in the
**	    RFP structure to support rolling forward multiple journal histories.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warnings.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Added support for UNDO phase at end of rollforward.  Added
**	        new fields to the transaction information: tx_status,
**		tx_clr_lsn, tx_id.  Changed rfp_tx to be a list of dynamically
**		allocated structures rather than a fixed-size array.
**		
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	21-jun-1993 (rogerk)
**	    Add jnl_la value (address to which the database has been journaled)
**	    and dmp_la value to the config information that is saved to be 
**	    copied into the restored version of the config file.
**	26-jul-1993 (rogerk)
**	    Added end transaction action list to the tx struct.
**	23-aug-1993 (jnash)
**	    Added RFP_TX_ABS_JUMP to flag abortsave jumps, now distinguished
**	    from CLR jumps.
**	20-sep-1993 (rogerk)
**	    Removed RFP_ETA list, it is now attached to the dmve control block
**	    and contains more than just end-transaction events.
**	31-jan-1994 (jnash)
**	    Fix RFP_AFTER_ETIME, RFP_EXACT_ETIME bit settings.
**	21-feb-1994 (jnash)
**	    Add RFP_BEFORE_BLSN, RFP_AFTER_ELSN, RFP_EXACT_LSN, rfp_etime_lsn.
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery project:
**	    Table level rollforward, move into dmfrfp.h add the following
**	    - rfp_tblhcb_ptr, points to data structure containing all 
**	      information about tables being recovered.
**	    - rfp_loc_mask_ptr, bit mask holding indicating which locations
**	      are of interest. The bit location is tied to the configuration
**	      file file location id.
**	    - last_tbl_id, holds dsc_last_tbl_id from the old version of the
**	      config file. Moved into this structure for ease.
**	    - db_status, original status of the db used during database
**	      level recovery.
**	    - open_db, status of the database being recovered.
**	    - rfp_line_buffer, rfp_line_date all through the code we keep 
**	      allocating these on the stack. I.e every record processed.
**	      Put here to avoid the above.
**	09-mar-1995 (shero03)
**	    Bug b67276
**	    Add location information for current ckp, dmp, and jnl
**	20-feb-1997(angusm)
**	    Add database extent information (bug 49057)
**	5-Apr-2004 (schka24)
**	    Add rawdata pointers to TX structure.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define RFPTX_CB as DM_RFPTX_CB.
**	           RFPBQTX_CB as DM_RFPBQTX_CB
**	           RFP_LSN_WAIT_CB as DM_RFP_LSN_WAIT_CB
**	
[@history_template@]...
*/
struct _DMF_RFP
{
    i4		rfp_status;	/* RFP options */
#define			    RFP_NOROLLBACK	0x0001 /* Skip undo phase */
#define			    RFP_NO_CLRLOGGING	0x0002 /* Don't log CLR's */
#define			    RFP_BEFORE_BTIME	0x0004 /* Prevous to -b time */
#define			    RFP_AFTER_ETIME	0x0008 /* After -e time */
#define			    RFP_EXACT_ETIME	0x0010 /* At -e time */
#define			    RFP_BEFORE_BLSN	0x0020 /* Prior to -start_lsn*/
#define			    RFP_AFTER_ELSN	0x0040 /* After -end_lsn */
#define			    RFP_EXACT_LSN	0x0080 /* Exact match on 
							*  -end_lsn */
#define			    RFP_STRADDLE_BTIME	0x0100
#define			    RFP_DID_JOURNALING  0x0200

#define		RFP_MAX_LINE_BUFFER		132
    char		rfp_line_buffer[RFP_MAX_LINE_BUFFER];   /*
						** Used to display messages
					        */
#define		RFP_MAX_DATE		32
    char		rfp_line_date[RFP_MAX_DATE];      /*
						** Used to hold the date when
						** displaying messages
						*/
    DM0C_JNL		old_jnl;	/* config prior to restore */
    DM0C_DMP		old_dmp;
    DMP_LOC_ENTRY	cur_ckp_loc;	/* current locations */
    DMP_LOC_ENTRY	cur_dmp_loc;	
    DMP_LOC_ENTRY	cur_jnl_loc;
    DMF_JSX		*rfp_jsx;	/* Pointer to journal context. */
    DB_TAB_TIMESTAMP	ckp_date;	/* Current checkpoint date */
    i4		ckp_mode;	/* either offline or online */
    i4		ckp_jnl_sequence;/* ckp seq as seen by jnl */
    i4		ckp_dmp_sequence;/* ckp seq as seen by dmp */
    i4		ckp_seq;	/* Actual ckp sequence number which
					** will be used for the restore of the
					** db. It may or may not be the same
					** as ckp_[jnl,dmp]_sequence.
					*/
    i4		jnl_count;	/* Number entries in journal history */
    i4		jnl_first;	/* First journal history to rollforward
					** some or all of the journal entries
					** in this history are rolled forward.
					*/
    i4		jnl_last;	/* Last journal history to rollforward
					** will typically be == jnl_first unless
					** we rollforward multiple journal
					** histories (e.g., from an old CKP).
					*/
    i4		jnl_history_number;	/* Current Journal history */
    i4		jnl_fil_seq;	/* Journal file sequence. */
    i4		jnl_blk_seq;	/* Journal block sequence */
    i4		jnl_block_size;	/* Journal block size. */
    LG_LA		jnl_la;		/* LA to which db has been archived */
    i4		dmp_count;	/* Number of entries in dump history */
    i4		dmp_first;	/* First dump file. */
    i4		dmp_last;	/* Last dump file. */
    i4		dmp_fil_seq;	/* Dump file sequence. */
    i4		dmp_blk_seq;	/* Dump block sequence */
    i4		dmp_block_size;	/* Dump block size. */
    LG_LA		dmp_la;		/* LA to which dmp info was archived */
    u_i4		last_tbl_id;    /* Holds version from old config file
					** used to restore value in new config
					** file during database recovery.
					*/
    i4		db_status;      /* Orginal db status */
    i4		open_db;	/* Status of the DB */
#define		DB_LOCKED	0x000001L
#define		DB_OPENED	0x000002L
    BITFLD		rfp_point_in_time:1; /* Point in time recovery */
    BITFLD		rfp_bits_free:31;    /* Unused */
    RFP_TX		*rfp_tx;	     /* Active TX list. */
    i4		rfp_tx_count;	     /* Count of Active TX's. */
    DMVE_CB		*rfp_dmve;	     /* Rollforward recovery context. */
    LG_LSN		rfp_etime_lsn;	     /* LSN of end time ET record */
    DMCKP_CB		rfp_dmckp;	     /* Holds CK context */
    RFP_TBLCB		*rfp_cur_tblcb;      /*
					     ** Holds the tblcb for the current 
					     ** journal or dump record that is
					     ** being applied. Only relevent for
					     ** table level rollforward.
					     */
    i4		rfp_no_tables;	     /* 
					     ** Number of tables being processed 
					     */
    i4		rfp_no_inv_tables;   /*
					     ** Number of invalid tables 
					     */
    RFP_TBLHCB		*rfp_tblhcb_ptr;     /* Structure containing all 
					     ** information on tables
					     */
    RFP_LOC_MASK	*rfp_loc_mask_ptr;   /* Ptr to mask indicating what
					     ** locations we are interested
					     ** in
					     */
    RFP_STATS_BLK	rfp_stats_blk;       /*	
					     ** Holds rollforward stats
					     */
#define		    RFP_TXH_CNT	128
    struct _RFP_TX			     /* Transaction description. */
    {
	RFP_TX		    *tx_next;	     /* Next active transaction. */
	RFP_TX  	    *tx_prev;        /* Previous transaction. */
	SIZE_TYPE	    tx_length;	     /* Length of control block. */
	i2		    tx_type;	     /* Control Block Identifier. */
#define	RFPTX_CB			DM_RFPTX_CB
	i2		    tx_s_reserved;   /* Reserved for future use. */
	PTR		    tx_l_reserved;   /* Reserved for future use. */
	PTR		    tx_owner; 	     /* Owner of control block. */
	i4		    tx_ascii_id;     /* Ascii Block Identifier. */
#define RFPTX_ASCII_ID   		CV_C_CONST_MACRO('R','F','P','X')
	RFP_TX		    *tx_hash_next;   /* Next TX on hash queue.*/
	RFP_TX		    *tx_hash_prev;   /* Prev TX on hash queue.*/
	DM0L_BT		    tx_bt;	     /* DM0LBT record */
	i4		    tx_status;	     /* Transaction (undo) status */
#define	RFP_TX_COMPLETE		        0x0001
#define	RFP_TX_CLR_JUMP		        0x0002
#define	RFP_TX_ABS_JUMP		        0x0004
	LG_LXID		    tx_id;	     /* The LG internal xact id. */
	LG_LSN		    tx_clr_lsn;	     /* LSN of compensated record 
					     ** if CLR_JUMP is asserted 
					     */
	i4		    tx_process_cnt;
	i4		    tx_undo_cnt;
	/* Note that the rawdata pointer points to the misc-cb;  the actual
	** rawdata starts at tx_rawdata->misc_data.  On the other hand,
	** the size is just the rawdata size, not counting the misc_cb.
	*/
	DMP_MISC	    *tx_rawdata[DM_RAWD_TYPES];  /* Raw data tanks */
	i4		    tx_rawdata_size[DM_RAWD_TYPES];  /* data sizes */
    }		    *rfp_txh[RFP_TXH_CNT];
    struct _JNL_CKP rfp_jnl_history[CKP_HISTORY];
    struct _DMP_CKP rfp_dmp_history[CKP_HISTORY];
    DMP_MISC        *rfp_locmap;              /* for table relocation   */
					      /* remap location ids     */
    i4	    rfp_db_loc_count;	     /* number of data locations */
    DM0C_EXT	    *rfp_db_ext;	     /* save current db extents */
    bool	    rfp_redo_err;
    LG_LSN	    rfp_redo_end_lsn;
    LG_LSN	    *rfp_redoerr_lsn;
    RFP_OCTX	    *rfp_octx;  /* online modify context list */
    i4		    rfp_octx_count;
    RFP_LSN_WAIT    *rfp_lsn_wait;  /* online modify pg pending list */
    i4		    rfp_lsn_wait_cnt;
    i1		    test_dump_err;
    i1		    test_redo_err;
    i1		    test_undo_err;
    i1		    test_excp_err;
    i1		    test_indx_err;
    RFP_BQTX	    *rfp_bqtx;		     /* -b qualifying xaction list */
    i4		    rfp_bqtx_count;	     
    struct _RFP_BQTX    
    {
        RFP_BQTX            *tx_next;        /* Next transaction. */
        RFP_BQTX            *tx_prev;        /* Previous transaction. */
        i4                  tx_length;       /* Length of control block. */
        i2                  tx_type;         /* Control Block Identifier. */
#define RFPBQTX_CB                      DM_RFPBQTX_CB
        i2                  tx_s_reserved;   /* Reserved for future use. */
        i4                  tx_l_reserved;   /* Reserved for future use. */
        i4                  tx_owner;        /* Owner of control block. */
        i4                  tx_ascii_id;     /* Ascii Block Identifier. */
#define RFPBQT_ASCII_ID                 CV_C_CONST_MACRO('R','F','P','B')
        RFP_BQTX            *tx_hash_next;   /* Next TX on hash queue.*/
        RFP_BQTX            *tx_hash_prev;   /* Prev TX on hash queue.*/
        DB_TRAN_ID          tx_tran_id;      /* Transaction id. */
    }               *rfp_bqtxh[RFP_TXH_CNT];
};

struct _RFP_SIDEFILE
{
    PTR             sf_dm0j;
    i4              sf_bksz;
    i4              sf_flags;
    i4              sf_error;
    i4              sf_status;
    i4              sf_rec_cnt;
}; 

struct _RFP_LSN_WAIT
{
    RFP_LSN_WAIT    *wait_next;
    RFP_LSN_WAIT    *wait_prev;
    SIZE_TYPE       wait_length;
    i2              wait_type;
#define         RFP_LSN_WAIT_CB     DM_RFP_LSN_WAIT_CB
    i2              wait_s_reserved;    /* Reserved for future use. */
    PTR             wait_l_reserved;    /* Reserved for future use. */
    PTR             wait_owner;         /* Owner of control block */
    i4              wait_ascii_id;
    #define  RFP_LSN_WAIT_ASCII_ID     CV_C_CONST_MACRO('R', 'W', 'A', 'I')
    DB_TAB_ID	    wait_tabid;
    DMP_RNL_LSN	    *wait_rnl_lsn;
    RFP_OCTX	    *wait_octx;
};

    FUNC_EXTERN DB_STATUS reloc_tbl_process(
    DMF_RFP             *rfp,
    DMF_JSX             *jsx,
    DMP_DCB             *dcb,
    #define     RELOC_PREPARE_TABLES       0x000001L
    #define     RELOC_FINISH_TABLES        0x000002L
    u_i4           phase);

    FUNC_EXTERN DB_STATUS reloc_loc_process(
    DMF_JSX             *jsx,
    DMP_DCB             *dcb,
    #define     RELOC_PREPARE_LOCATIONS       0x000001L
    #define     RELOC_FINISH_LOCATIONS        0x000002L
    u_i4           phase);

    FUNC_EXTERN bool reloc_this_loc(
    DMF_JSX         *jsx,
    DB_LOC_NAME     *locname,
    DB_LOC_NAME     **new_locname);

    FUNC_EXTERN DB_STATUS find_dcb_ext(
    DMF_JSX         *jsx,
    DMP_DCB         *dcb,
    DB_LOC_NAME     *loc,
    i4          usage,
    DMP_LOC_ENTRY   **ret_ext);
