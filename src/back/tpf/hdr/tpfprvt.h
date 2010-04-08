/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: TPFPRVT.H - TPF internal data strcutures.
**
** Description:
**     
[@comment_line@]...
**
** History: $Log-for RCS$
**      mar-3-88 (alexh)
**          created.
**	may-15-89 (carl)
**	    added tpt_ddl_cc field to TPT_CB to manage the privileged 
**	    CDB association
**	jun-01-89 (carl)
**	    added tpsp_next to TPFP_LIST to support doubly linked list of
**	    savepoints
**	jun-03-89 (carl)
**	    added tps_sp_cnt to TPD_ENTRY to register number of current
**	    savepoints known to the LDB
**	jul-11-89 (carl)
**		added tpv_ses_ulm and tpv_memleft to TPV_CB for allocating 
**		session streams
**	
**		added tpt_streamid, tpt_streamsize and tpt_ulmptr for
**		allocating and deallocating stream for session
**	jul-25-89 (carl)
**	    added defines TP0_MAX_SVPOINT_COUNT and TP1_MAX_SSESSION_COUNT
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


#define	TP0_MAX_SVPOINT_COUNT	75	/* maximum # of savepoints for ULM 
					** space allocation */

#define	TP1_MAX_SESSION_COUNT	100	/* maximum # of sessions for ULM space
					** allocation */


/*
**  Forward and/or External typedef/struct references.
*/

typedef struct
{
	PID	tpf_txn1;
	PTR	tpf_txn2;
}	TPF_TXN_ID;

/*}
** Name: TPS_ENTRY - slave transaction table entry
**
** Description:
**	Talbe entry for a slave transaction of a distributed transaction.
**
** History:
**      mar-4-88 (alexh)
**          created
*8	jun-03-89 (carl)
**	    added tps_sp_cnt for registering number of currently known 
**	    savepoints
*/

typedef struct _TPS_ENTRY
{
/* TPF table entry mark */
#define	TPS_BEGIN		1
#define	TPS_PREPARE_SENT	2
#define	TPS_WILLING		3
#define	TPS_ABORT		4
#define	TPS_COMMIT		5
#define	TPS_CLOSED		6
#define	TPS_ACTIVE		7
    i4		tps_state;   	/* allocation status of the entry */
    bool	tps_write;	/* indicate whether slave write has occured */
    DB_LANG	tps_txn_type;	/* transaction type DB_QUEL, DB_SQL */
    i4		tps_start_time;	/* starting time of slave - TMsecs */
    TPF_TXN_ID	tps_slave_id;	/* slave transaction id */
    i4		tps_sp_cnt;	/* number of current savepoints known to the
				** LDB, 0 if none */
    DD_LDB_DESC	tps_site;	/* LDBMS id */
}   TPS_ENTRY;

/*}
** Name: TPSP_LIST - distributed savepoint list
**
** Description:
**	A link list that tracks distributed savepoints.
**
** History:
**      dec-27-88 (alexh)
**          created.
**      may-21-89 (carl)
**	    corrected tpsp_prev to point to _TPSP_LIST instead of _TPD_ENTRY
**	jun-01-89 (carl)
**	    added tpsp_next to support doubly linked list of savepoints 
*/

typedef struct _TPSP_LIST
{
  DB_SP_NAME	tpsp_name;	/* save point name */
                           	/* save the important transaction state info */
  i4		tpsp_cnt_slaves;/* LDB stack frame pointer */
  i4		tpsp_txn_type;	/* read-only, 1_write, n_write, etc. */
  struct _TPSP_LIST
	    	*tpsp_prev;	/* previous savepoint, NULL if none */
  struct _TPSP_LIST
	    	*tpsp_next;	/* the next savepoint, NULL if none */
} TPSP_LIST;

/*}
** Name: TPD_ENTRY - distributed transaction table entry
**
** Description:
**	A table entry for a open distributed transaction.
**
** History:
**      mar-3-88 (alexh)
**          created.
**	jun-03-89 (carl)
**	    added tps_sp_cnt to register number of current savepoints 
**	    known to the LDB
*/

typedef struct _TPD_ENTRY
{
/* TPF table entry mark */
#define	TPD_INIT		2
#define	TPD_BEGIN		3
#define	TPD_POLL		4
#define	TPD_ABORT		5
#define	TPD_COMMIT		6
#define	TPD_CLOSED		7
    i4		tpd_state;	/* allocation status of the tpD entry */
    TPF_TXN_ID	tpd_id;		/* distributed transaction id */
#define	TPD_NO_ACTION	0	/* no dml action on txn yet */
#define	TPD_READ_ONLY	1	/* only read sites*/
#define	TPD_1_WRITE	2	/* one update site */
#define	TPD_N_WRITE	3	/* multiple update sites */
    i4		tpd_lang_type;	/* transaction language type */
    i4		tpd_txn_type;	/* transaction type */
    i4		tpd_log_addr;	/* transaction log address */
    i4		tpd_cnt_slaves;	/* number of slaves */
    i4		tpd_sp_cnt;	/* number of current savepoints known to
				** the LDB, 0 if none */
    TPSP_LIST	*tpd_first_sp;	/* ptr to first of savepoint list */
    TPSP_LIST	*tpd_last_sp;	/* ptr to last of savepoint list */
    i4	tpd_memleft;	/* ULM memleft */
    ULM_RCB	tpd_ulm_sp;	/* savepoint ulm stream */
#define	TPD_MAX_SLAVES	32
    TPS_ENTRY	tpd_head_slave[TPD_MAX_SLAVES];	/* transaction slaves */
}   TPD_ENTRY;

/*}
** Name: TPL_FILE - TP Log file info
**
** Description:
**      Logging related information.
**
** History:
**      Apr-19-88 (alexh)
**		created
*/

#define	TPL_PREFIX	"TPFLOG"
#define	TPL_PAGE_SIZE	sizeof(TPD_ENTRY)

typedef struct
{
	i4		tpl_page_size;	/* log record size */
	PID		tpl_orig_pid;	/* original pid */
	PID		tpl_recov_pid;	/* recovering pid */
}	TPL_HEADER;

typedef struct _TPL_FILE
{
	TPL_HEADER	tpl_header;	/* log file header page */
	char		tpl_pad[TPL_PAGE_SIZE-sizeof(TPL_HEADER)];
	i4		tpl_page_cnt;	/* log size */
	DI_IO		tpl_io;		/* DI context block */
	CL_SYS_ERR	tpl_err;	/* error record */
	char		tpl_path[DI_PATH_MAX];
	char		tpl_file[DI_FILENAME_MAX];
	i4		tpl_recover_cnt;/* log file recovered */
	LOCATION	tpl_location;
#define	TPF_MAX_PAGES	100
	char		tpl_bitmap[TPF_MAX_PAGES/BITSPERBYTE+1];
}   TPL_FILE;

/*}
** Name: TPV_CB - TPF server control block
**
** Description:
**      TPF server control block. One such block is required for a TPF
**	server.
**
** History:
**      Apr-19-88 (alexh)
**		created
**	Jul-11-89 (carl)
**		added tpv_ses_ulm and tpv_memleft for allocating and 
**		deallocating session 
**		streams
*/
typedef struct _TPV_SCB
{
    TPF_HEADER	tpv_header; /* standard control block header */
#define	TPVCB_ASCII_ID	CV_CONST_MACRO('T', 'P', 'V', 'B')
    TPL_FILE	tpv_log;	/* coord transaction log info */
    i4		tpv_txn_cnt;	/* number of active transactions */
    PTR		tpv_pool_id;	/* ulm pool id */
    bool	print_flag;	/* to log into dbms_log or not */
    ULM_RCB	tpv_ses_ulm;	/* ulm pool for (de)allocating session 
				** streams */
    i4	tpv_memleft;	/* for calling ULM for streams */
}   TPV_CB;

/*}
** Name: TPT_CB - TPF session control block
**
** Description:
**      TP session control block. One such block is required for each server
**	session.
**
** History:
**      mar-3-88 (alexh)
**		created
**	nov-8-88 (alexh)
**		add tpt_auto_commit field to manage autocommit
**	may-15-89 (carl)
**		add tpt_ddl_cc field to manage the privileged CDB
**		association
**	jul-11-89 (carl)
**		added tpt_streamid, tpt_streamsize and tpt_ulmptr for
**		allocating and deallocating stream for session
*/

typedef struct _TPT_CB
{
#define	TPTCB_ASCII_ID	CV_CONST_MACRO('T', 'P', 'T', 'B')
    TPF_HEADER	tpt_header; 	/* standard control block header */
    PTR		tpt_rqf;	/* RQF session context */
    TPD_ENTRY	tpt_txn;	/* distributed transaction entry */
    bool	tpt_auto_commit;/* transaction auto-commit */
    bool	tpt_ddl_cc;	/* TRUE if DDL concurrency is ON, FALSE
				** otherwise; set and reset by direct 
				** calls to TPF */
    PTR		tpt_streamid;	/* stream id for this session */
    i4		tpt_streamsize;	/* for stream size accounting */
    PTR		tpt_ulmptr;	/* ptr to ulm memory for this CB */
}   TPT_CB;
