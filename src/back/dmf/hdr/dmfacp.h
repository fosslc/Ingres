/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DMFACP.H - ACP definitions.
**
** Description:
**      This file defines the structure and constants used by the
**	archiver control program.
**
** History:
**      22-jul-1986 (Derek)
**          Created for Jupiter.
**     13-nov-1989 (rogerk)
**	    Added end backup record log address to BKDBCB structure for
**	    archiver to use to determine whether records fall between the
**	    BEGIN/END backup window.
**     14-nov-1989 (rogerk)
**	    Added dbcb_copy_done field.
**     11-jan-1989 (rogerk)
**	    Online checkpoint bug fixes.  Added database status's to the
**	    archive database control block (DBCB_{BACKUP,ONLINE_PURGE,
**	    NEW_JNL_NEEDED}) to keep track of addional states added.
**	    Added new field "dbcb_sbackup" to remember the address of the
**	    SBACKUP record.  Added new status's BKDBCB_ERROR and 
**	    BKDBCB_DUMP_FILE to the backup database control block. Added 
**	    fields to the backup database control block to keep track of the
**	    new online checkpoint protocol in which the online checkpoint
**	    forces an online purge (to force out journal records associated
**	    with the old checkpoint) , writes the start backup record, and
**	    then forces another cp after the record.  Also added new error
**	    returns ( E_DM982A_ARCH_SBACKUP_UNEXPTD, 
**	    E_DM9833_ARCH_BACKUP_FAILURE, E_DM9834_ARCH_OLPURGE_ERROR,
**	    E_DM9835_ARCH_NEW_JNL_ERROR).
**	9-jul-1990 (bryanp)
**	    Added two messages:
**		E_DM9836_ARCH_MISMATCHED_ET, E_DM9837_ARCH_ET_CONTINUE
**	24-jul-1990 (bryanp)
**	    Added E_DM9838_ARCH_DB_BAD_LA, E_DM9839_ARCH_JNL_SEQ_ZERO,
**	    E_DM983A_ARCH_OPEN_JNL_ZERO.
**	25-feb-1991 (rogerk)
**	    Changes for Archiver Stability project.
**	    Added definitions for Archiver termination command script.
**	    Added acb_error_code, acb_errtext, acb_errltext and acb_errdb fields
**	    to the archiver control block.  Added archiver shutdown error
**	    code definitions.
**	17-mar-1991 (bryanp)
**	    Bug 36228: Remove acb_current_lps; add dbcb_current_lps.
**	4-mar-1992 (bryanp)
**	    B41286: New TXCB status flags. Documented existing ones.
**	7-jul-1992 (ananth)
**	    Prototyping DMF.
**	21-dec-1992 (jnash)
**	    Reduced Logging Project.  Numerous changes to support new archiver.
**	     - Eliminate TXCB.
**	     - Eliminate lps fields; cluster merge no longer uses LPS records.
**	     - Eliminate ACB transaction fields.  The archiver no longer
**	       tracks transactions.
**	     - Add new FUNC_EXTERNs, eliminate old ones.
**	     - Add new error messages.
**	22-feb-93 (jnash)
**	     - Online backup fixes.  Modify dma_copy func prototype, 
**	       add DBCB_JNL.
**	     - Eliminate use of internal database id.  Add the dbcb_state 
**	       queue and dbcb_ext_id, rename dbcb_id to dbcb_int_id.  
**	       Catalog DBCB information via a hash queue based on the 
**	       external database id.  Add acb_dbh and acb_db_state, 
**	       remove fixed length acb_dbcb_map.  Add argument to alloc_dbcb.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Added dma_offline_context. This is called by the CSP when it
**		    performs archiving on behalf of another node.
**		Added lctx argument to dma_prepare to specify logfile context.
**	26-jul-1993 (rogerk)
**	  - Changed journal and dump window tracking in the logging system.
**	    The journal and dump windows are now tracked using the actual
**	    log addresses of the first and last records which define the
**	    window rather than the address of the CP previous to those
**	    log records.  Added new fields to the archiver control block:
**	    acb_bof_addr, acb_eof_addr, acb_blk_count, acb_blk_size, and 
**	    acb_last_la.
**	  - Added acb_max_cycle_size.  This is the maximum number of blocks
**	    of the logfile to process in one archive cycle.
**	  - Added new archiver error messages.
**	  - Removed dma_copy routine.
**	18-oct-1993 (jnash)
**	    Add E_DM9846_ARCH_WINDOW.
**	28-mar-1994 (bryanp) B59975
**	    Remove node_id from the dma_prepare() prototype.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      08-Oct-2002 (hanch04)
**          Replace the i4 for length with a SIZE_TYPE (size_t) to allow
**          64-bit memory allocation.
**  26-Jul-2005 (fanra01)
**      Bug 90455
**      Add script name definition for acpexit on Windows.
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**  10-dec-2007 (stial01)
**          Added defines for archiving from RCP.
**	10-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**/

/*
**  Constants used for call to dma_prepare
*/

#define			DMA_ARCHIVER		1
#define			DMA_CSP			2
#define			DMA_RCP			3

/*
**  Constants used for call to check_start_backup
*/

#define			BKDB_INVALID		3
#define			BKDB_FOUND		1
#define			BKDB_EXIST		2


/*
** Name of the Archiver Exit Command Script - run whenever archiver exits.
**
**  On VMS it is acpexit.com and to execute it, we must prefix
**	it with '@' - ie. "@ii_config:acpexit.com"
**
**  On Unix it is acpexit and can be executed with no prefix.
*/
#ifdef	VMS
#define	    ACP_EXIT_COMFILE		ERx("ACPEXIT.COM")
#define	    ACP_PCCOMMAND_PREFIX	ERx("@")
#else
#ifdef NT_GENERIC
#define	    ACP_EXIT_COMFILE		ERx("acpexit.bat")
#define	    ACP_PCCOMMAND_PREFIX	ERx("")
#else   /* NT_GENERIC */
#define	    ACP_EXIT_COMFILE		ERx("acpexit")
#define	    ACP_PCCOMMAND_PREFIX	ERx("")
#endif  /* NT_GENERIC */
#endif

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _ACB ACB;
typedef struct _ACP_QUEUE ACP_QUEUE;
typedef struct _DBCB DBCB;
typedef struct _SIDEFILE SIDEFILE;


/*}
** Name: ACP_QUEUE - ACB state queue definition.
**
** Description:
**      This structure defines the ACB state queue.
**
** History:
**     22-feb-1993 (jnash)
**          Created for the reduced logging project.
*/
struct _ACP_QUEUE
{
    ACP_QUEUE		*stq_next;	    /*	Next state queue entry. */
    ACP_QUEUE		*stq_prev;	    /*  Previous state queue entry. */
};

/*}
** Name: ACB - Archiver Control Block.
**
** Description:
**      This structure contains the state of the arhiver.
**
** History:
**      22-jul-1986 (Derek)
**          Created.
**	24-jan-1989 (EdHsu)
**	    Added ACB_BACKUP status to ACB control Structure to support
**	    online backup.
**	25-feb-1991 (rogerk)
**	    Added acb_error_code, acb_errtext, acb_errltext and acb_errdb fields
**	    as part of Archiver Stability changes.  A list of valid error
**	    codes for archiver shutdown was added at the end of this file.
**	    Added acb_test_flag for archiver tests.
**	17-mar-1991 (bryanp)
**	    Bug 36228. Remove acb_current_lps. We don't need a node-wide last
**	    LPS written value. Instead, each database must record the last
**	    LPS value written to that database's journal file.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedefs to quiesce the ANSI C 
**	    compiler warnings.
**	28-dec-1992 (jnash)
**	    Reduced logging project.  
**	     - Eliminate acb_txh, acb_c_txcb, acb_tx_count, acb_bt_count, 
**	       acb_tran_id.  The archiver no longer tracks transactions.
**	     - Change the name of acp_last_cp to acb_stop_addr.  We now 
**	       archive to fixed point (usually the eof).
**	23-feb-1993 (jnash)
**	     Add acb_dbh and acb_db_state, remove fixed length acb_dbcb_map.
**	     Use the external id hashed to a location on state queue rather
**	     than internal logging system id indexed into acb_dbcb_map.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-jul-1993 (rogerk)
**	  - Changed journal and dump window tracking in the logging system.
**	    The journal and dump windows are now tracked using the actual
**	    log addresses of the first and last records which define the
**	    window rather than the address of the CP previous to those
**	    log records.  Added new fields to the archiver control block:
**	    acb_bof_addr, acb_eof_addr, acb_blk_count, acb_blk_size, and 
**	    acb_last_la.
**	  - Added acb_max_cycle_size.  This is the maximum number of blocks
**	    of the logfile to process in one archive cycle.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define ACB_CB as DM_ACB_CB.
**	10-Nov-2008 (jonj)
**	    SIR 120874 Add acb_dberr.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add ACB_MVCC acb_status bit
[@history_template@]...
*/
struct _ACB
{
    ACB		    *acb_next;	            /* Not used, no queue of SVCBs. */
    ACB		    *acb_prev;
    SIZE_TYPE	    acb_length;		    /* Length of control block. */
    i2              acb_type;		    /* Type of control block for
                                            ** memory manager. */
#define                 ACB_CB		    DM_ACB_CB
    i2              acb_s_reserved;         /* Reserved for future use. */
    PTR             acb_l_reserved;         /* Reserved for future use. */
    PTR             acb_owner;              /* Owner of control block for
                                            ** memory manager.  SVCB will be
                                            ** owned by the server. */
    i4         acb_ascii_id;           /* Ascii identifier for aiding
                                            ** in debugging. */
#define                 ACB_ASCII_ID        CV_C_CONST_MACRO('#', 'A', 'C', 'B')
    i4	    acb_status;		    /* Status. */
#define			ACB_CLUSTER	    0x02
#define			ACB_BAD		    0x04
#define			ACB_CSP		    0x10
#define			ACB_ONLINE	    0x40
#define			ACB_RCP		    0x80
#define			ACB_MVCC	    0x100 /* Archiver has one or more
						  ** MVCC databases to service */
    i4	    acb_verbose;
    DB_TRAN_ID 	    acb_tran_id;	    /* Transaction id in progress. */
    i4	    acb_lx_id;		    /* Archiver update transaction 
                                            ** id. */
    i4	    acb_s_lxid;		    /* Session transaction id. */
    i4	    acb_lk_id;		    /* Lock list indentifier. */
    i4	    acb_db_id;		    /* Archiver database id. */
#define			ACB_DB_HASH	    63
    ACP_QUEUE	    acb_db_state;	    /* Queue of databases */
    DBCB	    **acb_dbh;		    /* Array of hash buckets for
					    ** databases. */
    i4	    acb_c_dbcb;		    /* Count of db entries */
    i4	    acb_bk_count;	    /* Count of total bk log records */
    i4	    acb_l_lctx;		    /* Length of context. */
    DBCB            *acb_purge_dbcb;        /* Database to purge. */
    DMP_LCTX	    *acb_lctx;		    /* Log file read context. */
    LG_LA	    acb_bof_addr;	    /* Log File BOF */
    LG_LA	    acb_eof_addr;	    /* Log File EOF */
    LG_LA	    acb_cp_addr;	    /* Current Logging System CP */
    LG_LA	    acb_start_addr;	    /* Start of Archive window. */
    LG_LA	    acb_stop_addr;	    /* Log Addr at which to stop */
    LG_LA	    acb_last_la;	    /* Addr of last record processed */
    LG_LA	    acb_last_cp;	    /* Addr of last CP encountered */
    i4	    acb_blk_count;	    /* Number of blks in the log file */
    i4	    acb_blk_size;	    /* Size of blks in the log file */
    i4	    acb_node_id;	    /* Cluster node identifer. */
    i4	    acb_bkdb_cnt;	    /* Cnt of db being backed up */
    i4	    acb_max_cycle_size;	    /* Max to archive in one cycle. */
    i4	    acb_test_flag;	    /* ACP test trace flag */
#define			ACB_T_ENABLE		0x0001 /* Tests enabled */
#define			ACB_T_DISKFULL		0x0002 /* Simulate diskfull */
#define			ACB_T_BADREC		0x0004 /* Simulate bad record */
#define			ACB_T_COMPLETE_WORK	0x0008 /* Stop at next cp */
#define			ACB_T_BACKUP_ERROR	0x0010 /* Simulate backup err */
#define			ACB_T_MEM_ALLOC		0x0020 /* Simulate no memory */
    SIDEFILE	*acb_sfq_next;		    /* queue of sidefiles */
    SIDEFILE	*acb_sfq_prev;		    
    i4	  	acb_sfq_count;
    DB_ERROR	    acb_dberr;		    /* Error source information */
    i4		    acb_error_code;	    /* Archiver exit reason */
    DB_DB_NAME	    acb_errdb;		    /* Database causing ACP error */
    i4	    acb_errltext;	    /* Length of ACP shutdown message */
    char	    acb_errtext[ER_MAX_LEN];/* Text of ACP shutdown message */
};

/*}
** Name: DBCB - Archive Database Control Block.
**
** Description:
**      This structured defines information about a particular database
**	that is opened by the archiver.
**
** History:
**      22-jul-1986 (Derek)
**          Created.
**	11-jan-1989 (rogerk)
**	    New status's added (DBCB_{BACKUP,ONLINE_PURGE,NEW_JNL_NEEDED})
**	    to support online checkpoint bug fixes.  Added dbcb_sbackup to
**	    keep track of the log address of the sbackup record associated
**	    with the current online backup.
**	17-mar-1991 (bryanp)
**	    Bug 36228. For each database we're archiving, we must keep track of
**	    the value of the last Log Page Stamp (LPS) which we wrote to the
**	    database's journal file (in the DM0L_LPS record). When we write a
**	    log record with a different LPS, we'll first write a new DM0L_LPS
**	    record containing the new LPS value. Added dbcb_current_lps.
**	22-dec-1992 (jnash)
**	    Reduced logging project.  
**	     - Add DBCB_JNL_DISABLE, a database on which journaling has been 
**	       disabled.
**	     - Eliminate DBCB_NEW_JNL_NEEDED.
**	     - Change name of dbcb_j_first_cp to dbcb_prev_jnl_la,
**	       dbcb_j_last_cp to dbcb_last_jnl_la.  These no longer
**	       necessarily correspond to CP addresses.
**	     - Eliminate dbcb_tx_cnt, add dbcb_jrec_cnt.  The ACP tracks the
**	       number of journal records written, but not transactions.
**	     - Eliminate dbcb_current_lps.  We no longer use lps's.
**	23-feb-1993 (jnash)
**	     Add the dbcb_state queue.  Also add DBCB_JNL, dbcb_ext_id, 
**	     rename dbcb_id to dbcb_int_id.  Catalog DBCB information via 
**	     a hash queue based on the external database id.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**          In this case owner is dbcb_parent.
**	03-Jul-1997 (shero03)
**	    Add JSWITCH status.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define DBCB_CB as DM_DBCB_CB.
**		   SIDEFILE_CB as DM_SIDEFILE_CB
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add DBCB_MVCC dbcb_status bit
[@history_template@]...
*/
struct _DBCB
{
    DBCB	    *dbcb_next;	            /* Not used, no queue of SVCBs. */
    DBCB	    *dbcb_prev;
    SIZE_TYPE	    dbcb_length;	    /* Length of control block. */
    i2              dbcb_type;		    /* Type of control block for
                                            ** memory manager. */
#define                 DBCB_CB		    DM_DBCB_CB
    i2              dbcb_s_reserved;        /* Reserved for future use. */
    PTR             dbcb_l_reserved;        /* Reserved for future use. */
    PTR             dbcb_parent;             /* Owner of control block for
                                            ** memory manager.  SVCB will be
                                            ** owned by the server. */
    i4         dbcb_ascii_id;          /* Ascii identifier for aiding
                                            ** in debugging. */
    ACP_QUEUE       dbcb_state;		    /* Next DBCB on queue */
#define                 DBCB_ASCII_ID       CV_C_CONST_MACRO('D', 'B', 'C', 'B')
    i4	    dbcb_status;	    /* Database status. */
#define			DBCB_PURGE	    0x00001
#define			DBCB_BACKUP	    0x00002
#define			DBCB_ONLINE_PURGE   0x00004
#define			DBCB_JNL_DISABLE    0x00008
#define			DBCB_BCK_ERROR	    0x00010  /* Error during backup */
#define			DBCB_DUMP_FILE_OPEN 0x00020  /* Dump file is open */
#define			DBCB_JNL 	    0x00040  /* DB journaled */
#define			DBCB_JSWITCH 	    0x00080  /* DB switching journal */
#define			DBCB_MVCC 	    0x00100  /* MVCC database */
    i4	    dbcb_refcnt;	    /* Reference count. */
    i4	    dbcb_jrec_cnt;	    /* Number of journal records writ
					    ** thus far. */
    i4	    dbcb_int_db_id;	    /* Internal Database identifier. */
    i4	    dbcb_ext_db_id;	    /* External Database identifier. */
    PTR		    dbcb_dm0j;		    /* Journal file control block. */
    ACB		    *dbcb_acb;		    /* Pointer to ACB. */
    DM_PATH	    dbcb_j_path;	    /* Journal Path */
    i4	    dbcb_lj_path;	    /* Length of journal path. */
    LG_LA	    dbcb_prev_jnl_la;	    /* Spot to which db has previously
					    ** been journaled. */
    LG_LA	    dbcb_first_jnl_la;	    /* Log address from which to start
					    ** journal processing. */
    LG_LA	    dbcb_last_jnl_la;	    /* Log address to which we need to
					    ** do journal processing. */
    LG_LA	    dbcb_jnl_la;	    /* Log address to which we have
					    ** finished journal processing. */
    i4	    dbcb_j_filseq;	    /* Journal file sequence num. */
    i4	    dbcb_j_blkseq;	    /* Journal block sequence. */

    /*
    ** Online Backup Fields
    */
    i4	    dbcb_drec_cnt;	    /* Backup log record count */
    LG_LA	    dbcb_sbackup;	    /* start backup lga */
    LG_LA	    dbcb_ebackup;	    /* end backup lga */
    LG_LA	    dbcb_prev_dmp_la;	    /* Spot to which db has previously
					    ** been dumped. */
    LG_LA	    dbcb_first_dmp_la;	    /* Log address from which to start
					    ** journal dumped. */
    LG_LA	    dbcb_last_dmp_la;	    /* Log address to which we need to
					    ** do journal dumped. */
    LG_LA	    dbcb_dmp_la;	    /* Log address to which we have
					    ** finished dump processing. */
    PTR		    dbcb_dm0d;		    /* terminator */
    DM_PATH	    dbcb_d_path;	    /* Dump path. */
    i4	    dbcb_ld_path;	    /* Length of dump path. */
    i4	    dbcb_d_blkseq;	    /* Dump block sequence. */
    i4	    dbcb_d_filseq;	    /* File sequence nummber. */
    i4	    dbcb_backup_complete;   /* Archiver portion of backup is
					    ** complete - all records have been
					    ** written to the dump files. */

    DMP_DCB	    dbcb_dcb;		    /* DCB for this database. */
    i4		    dbcb_sidefile;
};

struct _SIDEFILE
{
    SIDEFILE	    *sf_next;	            
    SIDEFILE	    *sf_prev;
    SIZE_TYPE	    sf_length;	    /* Length of control block. */
    i2              sf_type;		    /* Type of control block for
                                            ** memory manager. */
#define                 SIDEFILE_CB	   DM_SIDEFILE_CB
    i2              sf_s_reserved;        /* Reserved for future use. */
    PTR             sf_l_reserved;        /* Reserved for future use. */
    PTR             sf_parent;             /* Owner of control block for
                                            ** memory manager.  SVCB will be
                                            ** owned by the server. */
    i4		    sf_ascii_id;          /* Ascii identifier for aiding
                                            ** in debugging. */
#define        SIDEFILE_ASCII_ID       CV_C_CONST_MACRO('S', 'F', 'I', 'L')
    DM0L_BSF		sf_bsf;
    PTR			sf_dm0j;	    /* Journal file control block. */
    i4			sf_bksz;	    /* SF Block size */
    i4			sf_flags;
#define SIDEFILE_CREATE		0x01
#define SIDEFILE_OPEN		0x02
#define	SIDEFILE_HAS_RECORDS	0x04

    i4			sf_error;
    i4			sf_status;
    i4			sf_rec_cnt;
};


/*
**  Error message codes:
*/

#define		    E_DM9800_ARCH_TERMINATE		(E_DM_MASK + 0x9800L)
#define		    E_DM9801_ARCH_PHASE_ZERO		(E_DM_MASK + 0x9801L)
#define		    E_DM9802_ARCH_PHASE_ONE		(E_DM_MASK + 0x9802L)
#define		    E_DM9803_ARCH_PHASE_TWO		(E_DM_MASK + 0x9803L)
#define		    E_DM9804_ARCH_PHASE_THREE		(E_DM_MASK + 0x9804L)
#define		    E_DM9805_ARCH_BCP_FORMAT		(E_DM_MASK + 0x9805L)
#define		    E_DM9806_ARCH_BAD_DBID		(E_DM_MASK + 0x9806L)
#define		    E_DM9807_ARCH_REUSED_DBID		(E_DM_MASK + 0x9807L)
#define		    E_DM9808_ARCH_MISSING_DB		(E_DM_MASK + 0x9808L)
#define		    E_DM9809_ARCH_LOG_SHOW		(E_DM_MASK + 0x9809L)
#define		    E_DM980A_ARCH_LOG_READ		(E_DM_MASK + 0x980AL)
#define		    E_DM980B_ARCH_LOG_FORCE		(E_DM_MASK + 0x980BL)
#define		    E_DM980C_ARCH_LOG_EVENT		(E_DM_MASK + 0x980CL)
#define		    E_DM980D_ARCH_LOG_ALTER		(E_DM_MASK + 0x980DL)
#define		    E_DM980E_ARCH_LOG_ADD		(E_DM_MASK + 0x980EL)
#define		    E_DM980F_ARCH_LOG_REMOVE		(E_DM_MASK + 0x980FL)
#define		    E_DM9810_ARCH_LOG_BEGIN		(E_DM_MASK + 0x9810L)
#define		    E_DM9811_ARCH_LOG_END		(E_DM_MASK + 0x9811L)
#define		    E_DM9812_ARCH_OPEN_LOG		(E_DM_MASK + 0x9812L)
#define		    E_DM9813_ARCH_CLOSE_LOG		(E_DM_MASK + 0x9813L)
#define		    E_DM9814_ARCH_LOCK_CREATE		(E_DM_MASK + 0x9814L)
#define		    E_DM9815_ARCH_SHUTDOWN		(E_DM_MASK + 0x9815L)
#define		    E_DM9816_ARCH_LOCK_JNL		(E_DM_MASK + 0x9816L)
#define		    E_DM9817_ARCH_UNLOCK_JNL		(E_DM_MASK + 0x9817L)
#define		    E_DM9818_ARCH_ADD_DBID		(E_DM_MASK + 0x9818L)
#define		    E_DM9819_ARCH_ADD_JOURNAL		(E_DM_MASK + 0x9819L)
#define		    E_DM981A_ARCH_DEL_JOURNAL		(E_DM_MASK + 0x981AL)
#define		    E_DM981B_ARCH_DEL_DBID		(E_DM_MASK + 0x981BL)
#define		    E_DM981C_ARCH_PURGE_JOURNAL		(E_DM_MASK + 0x981CL)
#define		    E_DM981D_ARCH_PURGE_DBID		(E_DM_MASK + 0x981DL)
#define		    E_DM981E_ARCH_READ_BCP		(E_DM_MASK + 0x981EL)
#define		    E_DM981F_ARCH_CK_SBACKUP		(E_DM_MASK + 0x981FL)
#define		    E_DM9820_ARCH_UPDATE_CTX		(E_DM_MASK + 0x9820L)
#define		    E_DM9821_ARCH_BACKUP_DBID		(E_DM_MASK + 0x9821L)
#define		    E_DM9822_ARCH_P1_EBACKUP		(E_DM_MASK + 0x9822L)
#define		    E_DM9823_ARCH_ONLINE_BACKUP		(E_DM_MASK + 0x9823L)
#define		    E_DM9824_ARCH_PHASE_B2		(E_DM_MASK + 0x9824L)
#define		    E_DM9825_ARCH_INIT_DUMP		(E_DM_MASK + 0x9825L)
#define		    E_DM9826_ARCH_COPY_DUMP		(E_DM_MASK + 0x9826L)
#define		    E_DM9827_ARCH_LOG2DUMP		(E_DM_MASK + 0x9827L)
#define		    E_DM9828_ARCH_PHASE_B3		(E_DM_MASK + 0x9828L)
#define		    E_DM9829_ARCH_BAD_LOG_TYPE		(E_DM_MASK + 0x9829L)
#define		    E_DM982A_ARCH_SBACKUP_UNEXPTD	(E_DM_MASK + 0x982AL)
#define		    E_DM9830_ARCH_DWRITE		(E_DM_MASK + 0x9830L)
#define		    E_DM9831_ARCH_JWRITE		(E_DM_MASK + 0x9831L)
#define		    E_DM9832_ARCH_OBCLEAN		(E_DM_MASK + 0x9832L)
#define		    E_DM9833_ARCH_BACKUP_FAILURE	(E_DM_MASK + 0x9833L)
#define		    E_DM9834_ARCH_OLPURGE_ERROR		(E_DM_MASK + 0x9834L)
#define		    E_DM9835_ARCH_NEW_JNL_ERROR		(E_DM_MASK + 0x9835L)
#define		    E_DM9836_ARCH_MISMATCHED_ET		(E_DM_MASK + 0x9836L)
#define		    E_DM9837_ARCH_ET_CONTINUE		(E_DM_MASK + 0x9837L)
#define		    E_DM9838_ARCH_DB_BAD_LA		(E_DM_MASK + 0x9838L)
#define		    E_DM9839_ARCH_JNL_SEQ_ZERO		(E_DM_MASK + 0x9839L)
#define		    E_DM983A_ARCH_OPEN_JNL_ZERO		(E_DM_MASK + 0x983AL)
#define		    E_DM983B_ARCH_EXIT_ERROR		(E_DM_MASK + 0x983BL)
#define		    E_DM983C_ARCH_EXIT_CMD_OVFL		(E_DM_MASK + 0x983CL)
#define		    E_DM983D_ARCH_JNL_DISABLE		(E_DM_MASK + 0x983DL)
#define		    E_DM983E_ACP_DBID_INFO		(E_DM_MASK + 0x983EL)
#define		    E_DM983F_ARCH_BAD_DBID		(E_DM_MASK + 0x983FL)
#define		    E_DM9840_ARCH_BAD_DBID		(E_DM_MASK + 0x9840L)
#define		    E_DM9841_ARCH_BAD_DBID		(E_DM_MASK + 0x9841L)
#define		    E_DM9842_ARCH_SOC			(E_DM_MASK + 0x9842L)
#define		    E_DM9843_ARCH_COPY			(E_DM_MASK + 0x9843L)
#define		    E_DM9844_ARCH_EOC			(E_DM_MASK + 0x9844L)
#define		    E_DM9845_ARCH_WINDOW		(E_DM_MASK + 0x9845L)
#define		    E_DM9846_ARCH_WINDOW		(E_DM_MASK + 0x9846L)

/*
** Archiver shutdown messages - range 9850-986F
**
** These error numbers are used as archiver shutdown error codes and have
** corresponding error messages which give reasons for the shutdown and
** actions to perform to continue archiving.
**
** The error numbers here may passed to the acpexit script at shutdown time and 
** thus must be documented externally in the Ingres DBA Guide.
*/

#define		    E_DM9850_ACP_NORMAL_EXIT		(E_DM_MASK + 0x9850L)
#define		    E_DM9851_ACP_INITIALIZE_EXIT	(E_DM_MASK + 0x9851L)
#define		    E_DM9852_ACP_JNL_WRITE_EXIT		(E_DM_MASK + 0x9852L)
#define		    E_DM9853_ACP_RESOURCE_EXIT		(E_DM_MASK + 0x9853L)
#define		    E_DM9854_ACP_JNL_ACCESS_EXIT	(E_DM_MASK + 0x9854L)
#define		    E_DM9855_ACP_LG_ACCESS_EXIT		(E_DM_MASK + 0x9855L)
#define		    E_DM9856_ACP_DB_ACCESS_EXIT		(E_DM_MASK + 0x9856L)
#define		    E_DM9857_ACP_JNL_RECORD_EXIT	(E_DM_MASK + 0x9857L)
#define		    E_DM9858_ACP_ONLINE_BACKUP_EXIT	(E_DM_MASK + 0x9858L)
#define		    E_DM9859_ACP_EXCEPTION_EXIT		(E_DM_MASK + 0x9859L)
#define		    E_DM985A_ACP_JNL_PROTOCOL_EXIT	(E_DM_MASK + 0x985AL)
#define		    E_DM985B_ACP_INTERNAL_ERR_EXIT	(E_DM_MASK + 0x985BL)
#define		    E_DM985C_ACP_DMP_WRITE_EXIT		(E_DM_MASK + 0x985CL)
#define		    E_DM985F_ACP_UNKNOWN_EXIT		(E_DM_MASK + 0x985FL)





/* Function prototypes definitions. */

FUNC_EXTERN DB_STATUS dma_alloc_dbcb(
    ACB	    	*acb,
    i4	db_id,
    i4	lg_db_id,
    DB_DB_NAME	*db_name,
    DB_OWN_NAME	*db_owner,
    DM_PATH	*db_root,
    i4	db_l_root,
    LG_LA	*f_jnl_addr,
    LG_LA	*l_jnl_addr,
    LG_LA	*f_dmp_addr,
    LG_LA	*l_dmp_addr,
    LG_LA	*sbackup_addr,
    DBCB	**dbcb);

FUNC_EXTERN DB_STATUS dma_online_context(
    ACB	    	*acb);

FUNC_EXTERN DB_STATUS dma_offline_context(
    ACB	    	*acb);

FUNC_EXTERN DB_STATUS dma_archive(
    ACB	    	*acb);

FUNC_EXTERN DB_STATUS dma_complete(
    ACB	    **acb,
    DB_ERROR	*dberr);

FUNC_EXTERN DB_STATUS dma_eoc(
   ACB	    	*acb);

FUNC_EXTERN DB_STATUS dma_prepare(
    i4	flag,
    ACB		**acb,
    DMP_LCTX	*lctx,
    DB_ERROR	*dberr);

FUNC_EXTERN DB_STATUS dma_soc(
    ACB	 	*acb);

FUNC_EXTERN i4 check_start_backup(
    ACB		*acb,
    i4	dbid,
    LG_LA	*la);

FUNC_EXTERN DB_STATUS copy_dump(
    ACB	    *acb);

#ifdef NOTUSED
FUNC_EXTERN DB_STATUS acp_s_d_p2(
    ACB		*acb,
    DBCB	*db,
    i4	db_id,
    LG_RECORD	*header,
    DM0L_HEADER	*record);
#endif /* NOTUSED */
