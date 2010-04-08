/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DMFRCP.H - RCP definitions.
**
** Description:
**      This file defines the structure and constants used by the
**	recovery control program.
**
** History: 
**      30-Sep-1986 (ac)
**          Created for Jupiter.
**      28-Sep-1989 (ac)
**          Added dual logging support.
**	20-jun-1990 (bryanp)
**	    Removed active_log from the RCONF.
**	    Removed unneeded 'typedef' from structure definitions.
**	07-july-1992 (jnash)
**	    Add DMF prototyping.
**	29-sep-1992 (nandak)
**	    Use one common transaction ID type.
**	15-oct-1992 (jnash)
**	    Add FUNC_EXTERNS for dmr_rcp_init and dmr_log_init.
**	18-jan-1993 (bryanp)
**	    Add prototype for dmr_get_lglk_parms().
**	18-jan-1993 (rogerk)
**	    Reduced Logging Project:  Much of RCP rewritten.
**	      - Removed tracking of transaction completions and savepoints and 
**		mini transactions from redo processing and all the structures
**		and rcp fields associated with them.
**	      - Removed before image applyinging REDO processing and all the
**		structures and rcp fields associated with them.
**	      - Added new db and tx states in database and transaction structs.
**	      - Changed rcp_action values.
**	15-mar-1993 (jnash)
**	    Reassign value of RDMU_CB and RLP_CB to be unique.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Add support for multi-node logging systems.
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Remove header parameter from dmr_recover.
**		Added log record storage areas to the RTX.
**		Add rdb_node_id to RDB structure.
**	21-jun-1993 (bryanp)
**	    Add rtx_lg_rec_la to the RTX for tracking log addresses for tracing.
**	21-jun-1993 (rogerk)
**	    Added RTX_FOUND_ONLY_IN_CP status to indicate that a transaction
**	    was added to the active list by being listed in the BCP record
**	    at the start of the offline recovery.  This status is turned off if
**	    any actual log record is found for that transaction in the analysis
**	    pass.  If the RCP finds a transaction listed as open in a CP but
**	    with no other evidence of the transaction's existence, then it
**	    is possible that the transaction was actually resolved and that
**	    the ET record lies just previous to the CP (a current possible
**	    boundary condition).  The RCP will assume that it is safe to treat
**	    a transaction marked RTX_FOUND_ONLY_IN_CP as resolved if the db
**	    is found to be marked closed or if the tx's last written log record
**	    lies previous to the Log File BOF.
**	26-jul-1993 (rogerk)
**	  - Added rtx_orig_last_lga, used to store the log address of the
**	    last log record written by the transaction before recovery started.
**	    This is used to dump the transaction records in error processing.
**	18-oct-1993 (rogerk)
**	    Added user name to rtx structure.
**	15-apr-1994 (chiku)
**	    Bug56702: added rcp_state to RCP structure.
**      27-feb-1997 (stial01)
**          Added fields to RLP and RDB to keep track of SVDB lock lists and
**          SV_DATABASE locks (online partial recovery of SINGLE mode server)
**	14-Mar-2000 (jenjo02)
**	    Added rtx_clr_type to RTX.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	20-Apr-2004 (jenjo02)
**	    Added "recovery_done" output parm to dmr_recover() prototype.
**      06-Apr-2005 (stial01)
**          Added RTBL.
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define RCP_CB as DM_RCP_CB
**		   RDB_CB as DM_RDB_CB
**		   RTX_CB as DM_RTX_CB
**		   RLP_CB as DM_RLP_CB
**		   RDMU_CB as DM_RDMU_CB
**		   RTBL_CB as DM_RTBL_CB
**	15-Mar-2006 (jenjo02)
**	    Deleted prototypes for dmr_log_statistics(),
**	    dmr_format_transaction(), dmr_format_dis_tran_id(),
**	    functionality moved to dmdlog.c
**      29-nov-2005 (horda03) Bug 48659/INGSRV 2716
**          Defined RTX_JNL_TX to indicate that the transaction being recovered
**          has a journaled BT record.
**      14-jan-2008 (stial01)
**          Added rcp_dmve_cb for dmve exception handling
**      28-jan-2008 (stial01)
**          Redo/undo error handling during offline recovery via rcpconfig.
**	10-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	    Prototyped external function dmrLogErrorFcn, macroized
**	    dmr_log_error.
**      05-may-2009
**          Fixes for II_DMFRCP_PROMPT_ON_INCONS_DB / Db_incons_prompt
**/

/*
**  Forward and/or External typedef/struct references.
*/
typedef struct _RCP RCP;
typedef struct _RDB RDB;
typedef struct _RTX RTX;
typedef struct _RDTX RDTX;
typedef struct _RDMU RDMU;
typedef struct _RCP_QUEUE   RCP_QUEUE;
typedef struct _RLP RLP;
typedef struct _RTBL RTBL;

/* Valid values for recovery error action (not all valid during pass abort) */
#define RCP_UNDEFINED_ACTION		0
#define RCP_CONTINUE_IGNORE_DB		1
#define RCP_STOP_ON_INCONS_DB		2
#define RCP_CONTINUE_IGNORE_TABLE	3
#define RCP_CONTINUE_IGNORE_PAGE	4
#define RCP_PROMPT			5

#define RCP_ERROR_ACTION \
/*   0 -   */ ",CONTINUE_IGNORE_DB,STOP_ON_INCONS_DB,CONTINUE_IGNORE_TABLE,CONTINUE_IGNORE_PAGE,PROMPT"


/*}
** Name: RCP_QUEUE - RCP state queue definition.
**
** Description:
**      This structure defines the RCP state queue.
**
** History:
**     24-Apl-1987 (ac)
**          Created for Jupiter.
*/
struct _RCP_QUEUE
{
    RCP_QUEUE		*stq_next;	    /*	Next state queue entry. */
    RCP_QUEUE		*stq_prev;	    /*  Previous state queue entry. */
};


/*}
** Name: RCP - Recovery process control block.
**
** Description:
**      This structure contains the state of the recovery process.
**
** History:
**      30-sep-1986 (ac)
**          Created.
**	18-Jul-1988 (edhsu)
**	    Add field to support fast commit.
**	18-jan-1993 (rogerk)
**	    Changed rcp_action values: added CSP_ONLINE flag and disassociated
**	    CSP and RCP online actions (node failure recovery used to specify
**	    the RCP_R_ONLINE action - it now specifies RCP_R_CSP_ONLINE).
**	15-mar-1993 (bryanp)
**	    Add rcp_lctx array of log file context pointers. Remove rcp_node_id.
**	    Removed rcp_cp, rcp_bof, rcp_eof, rcp_lgblksize. These fields are
**	    all in the LCTX now.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	15-apr-1994 (chiku)
**	    Bug56702: added rcp_state to RCP structure.
**	25-Oct-2006 (kschendel) b122120
**	    With larger logs and CP intervals, the number of nonredo entries
**	    can be large enough to be a recovery problem.  Implement a hash
**	    table for nonredo lookup, similar to what is already done for
**	    db and tx entries.
**	15-Oct-2007 (jonj)
**	    Add rcp_cluster_enabled boolean, RCP_ACTION define.
**	10-Nov-2008 (jonj)
**	    SIR 120874: Add rcp_dberr.
*/
struct _RCP
{
    RCP		    *rcp_next;	            /* Not used. */
    RCP		    *rcp_prev;
    SIZE_TYPE	    rcp_length;		    /* Length of control block. */
    i2              rcp_type;		    /* Type of control block for
                                            ** memory manager. */
#define                 RCP_CB		    DM_RCP_CB
    i2		    rcp_s_reserved;	    /* Reserved for future use. */
    PTR             rcp_l_reserved;         /* Reserved for future use. */
    PTR             rcp_owner;              /* Owner of control block for
                                            ** memory manager. */
    i4         	    rcp_ascii_id;           /* Ascii identifier for aiding
                                            ** in debugging. */
#define                 RCP_ASCII_ID        CV_C_CONST_MACRO('#', 'R', 'C', 'P')
    i4	    	    rcp_action;		    /* Type of recovery to perform. */
#define			RCP_R_ONLINE		1  /* Online RCP recovery */
#define			RCP_R_STARTUP		2  /* RCP startup */
#define			RCP_R_CSP_ONLINE	3  /* Cluster Node failure */
#define			RCP_R_CSP_STARTUP	4  /* Cluster startup */
/* For diags... */
#define		RCP_ACTION "\
0,ONLINE,STARTUP,CSP_ONLINE,CSP_STARTUP"

    DMP_LCTX	    **rcp_lctx;		    /* array of ptrs to logfile context
					    ** structures, one for each node
					    ** which is being recoverd */
    RDB		    **rcp_rdb_map;	    /* Map from DB_ID to RDB. */
#define			RCP_DB_HASH	    63
    RCP_QUEUE	    rcp_db_state;	    /* The databae queue. */
    RTX		    **rcp_txh;		    /* Array of hash buckets for
					    ** transactions. */
#define			RCP_TX_HASH	    511
    RCP_QUEUE	    rcp_tx_state;	    /* The transaction state queue. */
    RDMU	    **rcp_dmuh;		    /* Array of hash buckets for
					    ** nonredo entries. */
#define			RCP_DMU_HASH	    199
    RCP_QUEUE	    rcp_rdmu_state;	    /* RDMU (nonredo) queue. */
    i4	    	    rcp_db_count;	    /* Count of databases to recover. */
    i4	    	    rcp_tx_count;	    /* Count of transactions to abort */
    i4	    	    rcp_rdtx_count;	    /* Count of xact to redo */
    i4	    	    rcp_willing_commit;	    /* Count of willing commit xact. */
    RLP		    **rcp_lph;		    /* Array of hash buckets for 
					    ** servers. */
#define			RCP_LP_HASH	    10
    i4	    	    rcp_lock_list;	    /* Recovery lock list */
    i4	    	    rcp_verbose;	    /* Indicates to trace actions */
    bool	    rcp_cluster_enabled;    /* Cluster-enabled */

    /*
     * Bug56702: added this field to indicate the
     *           status of recovery.
     */
    u_i4       	    rcp_state;
#define                 RCP_OK          (0)	/* recovery progressing normally. */
#define                 RCP_LG_GONE     (1)  	/* 
						 * recovery encountered dead logfull
						 * or the LG subsystem is not usable.
						 */
    PTR		    rcp_dmve_cb;
    DB_ERROR	    rcp_dberr;		    /* Error information */

};

/*}
** Name: RDB - Recovery Process Database Control Block.
**
** Description:
**      This structured defines information about a particular database
**	that is opened by the recovery process.
**
**	In a multi-node logging system, such as the VAXCluster, there is one
**	RDB instance per database needing recovery per node. (Actually, we
**	sometimes track database information in RDB's for databases which do
**	not need any recovery.) Thus the proper unique identifier for an RDB
**	instance is db_id + node_id.
**
** History:
**      30-Sep-1986 (ac)
**          Created for Jupiter.
**	15-mar-1993 (bryanp)
**	    Cluster support:
**		rdb_first_lga =>rdb_first_lsn, and it's actually an LG_LSN.
**		Add rdb_node_id to RDB structure.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**          In this case owner is rdb_parent.
**      27-feb-1997 (stial01)
**          Added rdb_svdb_ll_id, rdb_svdb_lkb_id, rdb_svdb_rsb_id for
**          online partial recovery of SINGLE mode server.
*/
struct _RDB
{
    RDB		    *rdb_next;		    /* Next RDB on hash queue. */
    RDB		    *rdb_prev;		    /* Previous RDB on hash queue. */
    SIZE_TYPE	    rdb_length;		    /* Length of control block. */
    i2              rdb_type;		    /* Type of control block for
                                            ** memory manager. */
#define                 RDB_CB		    DM_RDB_CB
    i2              rdb_s_reserved;	    /* Reserved for future use. */
    PTR             rdb_l_reserved;	    /* Reserved for future use. */
    PTR             rdb_parent;		    /* Owner of control block for
                                            ** memory manager. */
    i4         rdb_ascii_id;	    /* Ascii identifier for aiding
                                            ** in debugging. */
#define                 RDB_ASCII_ID        CV_C_CONST_MACRO('R', 'D', 'B', 'C')
    RCP_QUEUE	    rdb_state;		    /* Next RDB on active state 
                                            ** queue. */
    i4	    rdb_refcnt;		    /* Reference count. */
    i4	    rdb_bt_cnt;		    /* Open Transaction count. */
    i4	    rdb_et_cnt;		    /* Completed Transaction count. */
    i4	    rdb_node_id;	    /* node id for the node on which
					    ** this database was being accessed.
					    */
    i4	    rdb_lg_dbid;	    /* LG Internal Database id. */
    i4	    rdb_dbopen_id;	    /* Database open id. */
    DMP_DCB	    *rdb_dcb;		    /* DMF database control block. */
    DMP_LCTX	    *rdb_lctx;
    i4	    rdb_status;		    /* Status of the database. */
#define			    RDB_RECOVER 		0x0001
#define			    RDB_JOURNAL			0x0002
#define			    RDB_REDO			0x0004
#define			    RDB_REDO_PARTIAL		0x0008
#define			    RDB_NOTDB			0x0010
#define			    RDB_FAST_COMMIT		0x0020
#define			    RDB_WILLING_COMMIT		0x0040
#define			    RDB_INVALID			0x0080
#define			    RDB_WILLING_FAIL		0x0100
#define			    RDB_ALREADY_INCONSISTENT	0x0200
#define                     RDB_XROW_LOCK               0x0400
#define                     RDB_XSVDB_LOCK              0x0800
#define			    RDB_TABLE_RECOVERY_ERROR	0x1000
    i4	    rdb_inconst_code;	    /* Reason for RDB_INVALID status */
    DB_DB_NAME	    rdb_name;		    /* Database name. */
    DB_OWN_NAME	    rdb_owner;		    /* Database owner. */
    i4         rdb_ext_dbid;           /* External database id. */
    i4	    rdb_l_root;		    /* Length of root location. */
    DM_PATH	    rdb_root;		    /* Root database location. */
    LG_LA	    rdb_first_lga;	    /* log addr of 1st log rec in db. */
    LG_LSN	    rdb_first_lsn;	    /* LSN of First log record in db. */
    LK_LLID         rdb_svdb_ll_id;         /* lock list id for SV_DATABASE lk*/
    i4         rdb_svdb_lkb_id;        /* lkb id for SV_DATABASE lock */
    i4         rdb_svdb_rsb_id;        /* rsb id for SV_DATABASE lock */
    RCP_QUEUE	    rdb_tbl_invalid;	    /* queue of invalid tables */
};

/*}
** Name: RTX - Recovery Process Transaction Control Block
**
** Description:
**      This structure describes information about a transaction against
**	a database.
**
** History:
**      30-Sep-1986 (ac)
**          Created for Jupiter.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV: Added rtx_clr_lsn.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Track both rtx_last_lga and rtx_last_lsn for cluster support.
**		Save pointer to logfile context block.
**		Added log record storage areas to the RTX.
**	21-jun-1993 (bryanp)
**	    Add rtx_lg_rec_la to the RTX for tracking log addresses for tracing.
**	21-jun-1993 (rogerk)
**	    Added RTX_FOUND_ONLY_IN_CP status to indicate that a transaction
**	    was added to the active list by being listed in the BCP record
**	    at the start of the offline recovery.
**	26-jul-1993 (rogerk)
**	    Added rtx_orig_last_lga, used to store the log address of the
**	    last log record written by the transaction before recovery started.
**	    This is used to dump the transaction records in error processing.
**	18-oct-1993 (rogerk)
**	    Added user name to rtx structure.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	14-Mar-2000 (jenjo02)
**	    Added rtx_clr_type.
*/
struct _RTX
{
    RTX		    *rtx_next;		    /* Next RTX on hash queue. */
    RTX		    *rtx_prev;		    /* Previous RTX on hash queue. */
    SIZE_TYPE	    rtx_length;		    /* Length of control block. */
    i2              rtx_type;		    /* Type of control block for
                                            ** memory manager. */
#define                 RTX_CB		    DM_RTX_CB
    i2              rtx_s_reserved;	    /* Reserved for future use. */
    PTR             rtx_l_reserved;	    /* Reserved for future use. */
    PTR             rtx_owner;		    /* Owner of control block for
					    ** memory manager. */
    i4         rtx_ascii_id;	    /* Ascii identifier for aiding
                                            ** in debugging. */
#define                 RTX_ASCII_ID        CV_C_CONST_MACRO('R', 'T', 'X', 'C')

    RCP_QUEUE	    rtx_state;		    /* Next RTX on active state queue.*/
    DB_TRAN_ID	    rtx_tran_id;	    /* Physical transaction id. */
    LG_LA	    rtx_first_lga;	    /* Lga of first log record. */
    LG_LA	    rtx_orig_last_lga;	    /* Lga of Last log record. */
    LG_LA	    rtx_last_lga;	    /* Lga of Last log rec processed. */
    LG_LSN	    rtx_last_lsn;	    /* log seq number of last log rec */
    LG_LA	    rtx_cp_lga; 	    /* CP before first log record. */
    RDB		    *rtx_rdb;		    /* Database for this RTX. */
    DMP_LCTX	    *rtx_lctx;		    /* logfile context block. */
    LK_LLID	    rtx_ll_id;		    /* The transaction lock list id */
    LG_LXID	    rtx_id;		    /* The LG system transaction id. */
    i4	    rtx_status;		    /* The transaction status. */
#define			RTX_ABORT_INPROGRESS	0x0001
#define			RTX_ABORT_FINISHED	0x0002
#define			RTX_WC_FINISHED		0x0004
#define			RTX_TRAN_COMPLETED	0x0008
#define			RTX_CLR_JUMP		0x0010
#define			RTX_MAN_ABORT		0x0020
#define			RTX_PASS_ABORT		0x0040
#define			RTX_FOUND_ONLY_IN_CP	0x0080
#define			RTX_COMPLETE		(RTX_ABORT_FINISHED | \
						 RTX_WC_FINISHED | \
						 RTX_TRAN_COMPLETED)
#define                 RTX_XROW_LOCK           0x0100
#define                 RTX_JNL_TX              0x0200
    LG_LSN	    rtx_clr_lsn; 	    /* LSN of compensated log record */
    i4		    rtx_clr_type;	    /* Type of compensation log record */
    i4	    rtx_recover_type;	    /* Transaction Recovery needed. */
#define			RTX_REDO_UNDO	    0
#define			RTX_UNDO_ONLY	    1	/* Not Used */
#define			RTX_WILLING_COMMIT  2
    DB_OWN_NAME	    rtx_user_name;	    /* User name of xact owner. */
    LG_LA	    rtx_lg_rec_la;	    /* the LG_LA log address of the
					    ** record currently stored in
					    ** rtx_lg_record and rtx_dm0l_record
					    */
    LG_RECORD	    rtx_lg_record;	    /* the LG_RECORD portion of the
					    ** transaction's last log record.
					    */
    char	    rtx_dm0l_record[LG_MAX_RSZ];
					    /*
					    ** the full contents of the
					    ** transaction's last log record.
					    */
};


/*}
** Name: RLP - Recovery Process Process Control Block
**
** Description:
**      This structure describes information about a server(process).
**
** History:
**      30-Sep-1986 (ac)
**          Created for Jupiter.
**	15-mar-1993 (jnash)
**	    Reassign value of RLP_CB to be unique.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**      27-feb-1997 (stial01)
**          Added rlp_svdb_ll_id for online partial recovery SINGLE mode server.
*/
struct _RLP
{
    RLP		    *rlp_next;		    /* Next RLP on hash queue. */
    RLP		    *rlp_prev;		    /* Previous RLP on hash queue. */
    SIZE_TYPE	    rlp_length;		    /* Length of control block. */
    i2              rlp_type;		    /* Type of control block for
                                            ** memory manager. */
#define                 RLP_CB		    DM_RLP_CB
    i2              rlp_s_reserved;	    /* Reserved for future use. */
    PTR             rlp_l_reserved;	    /* Reserved for future use. */
    PTR             rlp_owner;		    /* Owner of control block for
					    ** memory manager. */
    i4         rlp_ascii_id;	    /* Ascii identifier for aiding
                                            ** in debugging. */
#define                 RLP_ASCII_ID        CV_C_CONST_MACRO('R', 'L', 'P', 'C')
    i4	    rlp_lpb_id;		    /* The logging system id of the 
					    ** server. */
    i4	    rlp_pid;		    /* The process id of the server. */
    i4         rlp_status;
#define                     RLP_XSVDB_LOCK              0x0001
    LK_LLID         rlp_svdb_ll_id;         /* lock list id for SV_DATABASE lk*/
};

/*}
** Name: RDMU	- RCP Non-redo Table.
**
** Description:
**      This structure defines the RCP Non-redo table.
**
** History:
**     9-june-1988 (EdHsu)
**          Created for Fast Commit.
**	15-mar-1993 (jnash)
**	    Reassign value of RDMU_CB to be unique.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
*/
struct _RDMU
{
    RDMU	    *rdmu_next;	            /* Hash list links */
    RDMU	    *rdmu_prev;
    SIZE_TYPE	    rdmu_length;	    /* Length of control block. */
    i2              rdmu_type;		    /* Type of control block for
                                            ** memory manager. */
#define                 RDMU_CB		    DM_RDMU_CB
    i2              rdmu_s_reserved;        /* Reserved for future use. */
    PTR             rdmu_l_reserved;        /* Reserved for future use. */
    PTR             rdmu_owner;             /* Owner of control block for
                                            ** memory manager. */
    i4         rdmu_ascii_id;          /* Ascii identifier for aiding
                                            ** in debugging. */
    RCP_QUEUE	    rdmu_state;		    /* Next RDMU on active state queue */
#define                 RDMU_ASCII_ID       CV_C_CONST_MACRO('R', 'D', 'M', 'U')
    i4		    dbid;	    /* Database Identifier  */
    DB_TAB_ID	    tabid;	    /* Table Identifier	    */
    DB_TAB_NAME	    tabname;
    DB_OWN_NAME	    tabowner;
    LG_LSN	    lsn;	    /* Last Log Address	    */
};


/*}
** Name: RTBL	- RCP Invalid Table.
**
** Description:
**      This structure defines an RCP invalid table.
**
** History:
**     06-apr-2005 (stial01)
**          Created.
*/
struct _RTBL
{
    RTBL	    *rtbl_next;	            /* Not used. */
    RTBL	    *rtbl_prev;
    i4		    rtbl_length;	    /* Length of control block. */
    i2              rtbl_type;		    /* Type of control block for
                                            ** memory manager. */
#define                 RTBL_CB		    DM_RTBL_CB
    i2              rtbl_s_reserved;        /* Reserved for future use. */
    PTR             rtbl_l_reserved;        /* Reserved for future use. */
    PTR             rtbl_owner;             /* Owner of control block for
                                            ** memory manager. */
    i4		    rtbl_ascii_id;          /* Ascii identifier for aiding
                                            ** in debugging. */
    RCP_QUEUE	    rtbl_state;		    /* Next RTBL on active state queue */
#define                 RTBL_ASCII_ID       CV_C_CONST_MACRO('R', 'T', 'B', 'L')
    DB_TAB_ID	    tabid;	    /* Table Identifier	    */
    DB_TAB_NAME	    tabname;
    DB_OWN_NAME     ownname;
    i4		    redo_cnt; 
    i4		    undo_cnt;
    i4		    rtbl_flag;
#define RTBL_IGNORE_TABLE			0x01
    DB_TAB_NAME	    table_name;
    LG_LSN	    lsn;		    /* lsn at time of first error */
    DB_TRAN_ID      tran_id;		    /* tranid at time of first error */
    DB_TAB_TIMESTAMP bt_time;		    /* Time transaction started. */
#define MAX_ERR_PAGES 10                   
    DM_PAGENO	    err_pages[MAX_ERR_PAGES];
    i4		    err_page_cnt;
};


 
/*
**  External function references.
*/
FUNC_EXTERN	DB_STATUS	dmr_recover(
					i4		type,
					bool		*recovery_done);

FUNC_EXTERN	STATUS		dmr_alloc_rcp(
					RCP		**result_rcp,
					i4		type,
					DMP_LCTX	**lctx_array,
					i4		num_nodes );

FUNC_EXTERN	DB_STATUS	dmr_online_context(
					RCP		*rcp);

FUNC_EXTERN	DB_STATUS	dmr_offline_context(
					RCP		*rcp,
					i4		node_id);

FUNC_EXTERN	DB_STATUS	dmr_init_recovery(
					RCP		*rcp);

FUNC_EXTERN	DB_STATUS	dmr_complete_recovery(
					RCP		*rcp,
					i4		node_id);

FUNC_EXTERN	DB_STATUS	dmr_analysis_pass(
					RCP		*rcp,
					i4		node_id);

FUNC_EXTERN	DB_STATUS	dmr_redo_pass(
					RCP		*rcp,
					i4		node_id);

FUNC_EXTERN	DB_STATUS	dmr_undo_pass(
					RCP		*rcp);

FUNC_EXTERN	DB_STATUS	dmr_get_cp(
					RCP		*rcp,
					i4		node_id);

FUNC_EXTERN	VOID		dmr_dump_log(
					RCP 	*rcp,
					i4	node_id);

FUNC_EXTERN	DB_STATUS	dmr_check_event(
					RCP	*rcp,
					i4	flag,
					LG_LXID lx_id);

FUNC_EXTERN	DB_STATUS 	dmr_rcp_init(LG_LGID    *recovery_log_id);

FUNC_EXTERN	DB_STATUS 	dmr_log_init(VOID);

FUNC_EXTERN	VOID		dmr_cleanup(RCP 	*rcp);

FUNC_EXTERN	DB_STATUS	dmrLogErrorFcn(
					i4		err_code,
					CL_ERR_DESC	*syserr,
					i4		p,
					i4		ret_err_code,
					PTR		FileName,
					i4		LineNumber);
#define	dmr_log_error(err_code,syserr,p,ret_err_code) \
	dmrLogErrorFcn(err_code,syserr,p,ret_err_code,__FILE__,__LINE__)

