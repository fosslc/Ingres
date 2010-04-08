/*
** Copyright (c) 2004 Ingres Corporation
**
*/

/**
** Name: TPFDDB.H - TPF internal data strcutures.
**
** Description:
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
**	oct-15-89 (carl)
**	    adapted from TPFPRVT.H for 2PC
**	jul-07-90 (carl)
**	    added tlx_23_lxid to TPD_LX_CB
**	oct-05-90 (carl)
**	    changed tss_3_dbg_flags to tss_3_trace_vector in TPD_SS_CB
**	aug-13-91 (fpang)
**	    changed max trace point to 119 from 128. 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


GLOBALREF   struct _TPD_SV_CB	*Tpf_svcb_p;
						/* TPF facility control block */
/* global constants */


#define	TPD_0MAX_LX_PER_SESSION	    20	
					/* maximum # of LXs/session for ULM 
					** allocation */

#define	TPD_1MAX_SP_PER_SESSION	    20	
					/* maximum # of savepoints/session for 
					** ULM allocation */

#define	TPD_0SEM_SHARED			(i4) 0
#define	TPD_1SEM_EXCLUSIVE		(i4) 1

#define	TPD_0TO_FE			FALSE	    /* send to the FE */
#define	TPD_1TO_TR			TRUE	    /* write to the log */

#define	TPD_0DO_ABORT			FALSE
#define	TPD_1DO_COMMIT			TRUE

#define	TPD_0LOG_STATE_NO		FALSE
#define	TPD_1LOG_STATE_YES		TRUE

#define	TPD_0RESTART_NIL		(i4)	0
#define	TPD_1RESTART_UNEXPECTED_ERR	(i4)	1
						    /* unexpected error */
#define	TPD_2RESTART_NOT_DX_OWNER	(i4)	2   /* DX adopted by other 
						    ** server */
#define	TPD_3RESTART_LDB_UNKNOWN	(i4)	3   /* LDB not in DX */
#define	TPD_4RESTART_UNKNOWN_DX_ID	(i4)	4   /* DX id not recognized */
#define	TPD_5RESTART_NO_SUCH_LDB	(i4)	5   /* LDB non-existent */
#define	TPD_6RESTART_LDB_NOT_OPEN	(i4)	6   /* LDB starting up */

#define	TPD_0SVR_RECOVER_MAX	3	/* maximum number of server recovery 
					** attempts */

#define	TPD_0TIMER_MAX		20	/* number of restart retries before 
					** incurring next timer increment */



/*}
** Name: TPD_LM_CB - ULM stream CB 
**
** Description:
**	A CB for ULM stream.
**
** History:
**      nov-02-89 (carl)
**          created.
*/

#define	TPD_0LM_LX_CB		(i4) 0	/* for ordering LX CB allocation from
					** the current DX's LX stream */
#define	TPD_1LM_SP_CB		(i4) 1	/* for ordering SP CB allocation from
					** the current DX's SP stream */
#define	TPD_2LM_CD_CB		(i4) 2	/* for ordering CDB CB allocation from
					** the CDB CB stream at server startup 
					** time */
#define	TPD_3LM_OX_CB		(i4) 3	/* for ordering ODX CB allocation from
					** the ODX CB stream at server startup 
					** time */
#define	TPD_4LM_SX_CB		(i4) 4	/* for ordering LX CB allocation from
					** the special LX CB stream at server 
					** startup time */
#define	TPD_5LM_SR_CB		(i4) 5	/* for ordering LX CB allocation from
					** the search CB stream at server 
					** startup time */


typedef struct _TPD_LM_CB
{
    CS_SEMAPHORE
		tlm_0_sema4;	/* for the P and V operations to gain 
				** exclusive/shared access to this CB and
				** everything that hangs off this CB */
    PTR		tlm_1_streamid;	/* id of this ULM stream, NULL if CB is void */
    i4		tlm_2_cnt;	/* number of CBs allocated in this stream,
				** 0 if none */
    PTR	    	tlm_3_frst_ptr;	/* ptr to the first CB in list, NULL if none */
    PTR		tlm_4_last_ptr;	/* ptr to the last CB, NULL if none */
} TPD_LM_CB;


/*}
** Name: TPD_CD_CB - CDB control block
**
** Description:
**	Used for building linked list of CDB information read off iistar_cdbs.
**
** History:
**      nov-28-89 (carl)
**          created
*/

typedef struct _TPD_CD_CB
{
    CS_SEMAPHORE    tcd_0_sema4;	/* for the P and V operations to gain 
					** exclusive/shared access to this CB 
					** and everything that hangs off this 
					** CB */
  TPC_I1_STARCDBS   tcd_1_starcdbs;	/* information read off iistar_cdbs */
  i4	    tcd_2_flags;	/* control flags */

#define	TPD_0CD_FLAG_NIL	0x0000
#define	TPD_1CD_TO_RECOVER	0x0001	/* ON if DDB is to be recovered, OFF 
					** otherwise requiring NO recovery */
#define	TPD_2CD_RECOVERED	0x0002	/* ON if DDB is recovered, OFF 
					** otherwise */

  struct _TPD_CD_CB *tcd_3_prev_p;	/* predecessor in list, NULL if none */
  struct _TPD_CD_CB *tcd_4_next_p;	/* successor in list, NULL if none */
} TPD_CD_CB;


/*}
** Name: TPD_SP_CB - distributed savepoint list
**
** Description:
**	A link list that tracks distributed savepoints.
**
** History:
**      dec-27-88 (alexh)
**          created.
**      may-21-89 (carl)
**	    corrected tpsp_prev to point to _TPD_SP_CB instead of _TPD_ENTRY
**	jun-01-89 (carl)
**	    added tpsp_next to support doubly linked list of savepoints 
*/

typedef struct _TPD_SP_CB
{
  DB_SP_NAME	tsp_1_name;	/* save point name */
                           	/* save the important transaction state info */
  i4		tsp_2_dx_mode;	/* read-only, 1-write, n-write */
  struct _TPD_SP_CB
	    	*tsp_3_prev_p;	/* previous savepoint, NULL if none */
  struct _TPD_SP_CB
	    	*tsp_4_next_p;	/* the next savepoint, NULL if none */
/*
  i4		tsp_ldb_cnt;	** LDB stack frame pointer **
*/
} TPD_SP_CB;


/*}
** Name: TPD_LX_CB - local transaction 
**
** Description:
**	Structure for a local transaction of a distributed transaction.
**
** History:
**      mar-4-88 (alexh)
**          created
**	jun-03-89 (carl)
**	    added tps_sp_cnt for registering number of currently known 
**	    savepoints
**	oct-29-89 (carl)
**	    modified for 2PC
**	jul-07-90 (carl)
**	    added tlx_23_lxid
*/

typedef struct _TPD_LX_CB
{
    DB_LANG	tlx_txn_type;	/* transaction type DB_QUEL, DB_SQL */
    i4		tlx_start_time;	/* starting time of LX - TMsecs */
    bool	tlx_write;	/* indicate whether update has occured */
    i4		tlx_sp_cnt;	/* number of current savepoints known to the
				** LDB, 0 if none */
    DD_LDB_DESC	tlx_site;	/* LDB id */
    i4		tlx_state;   	/* state of the LX */

#define	LX_0STATE_NIL		(i4) 0
#define	LX_1STATE_INITIAL	(i4) 1	/* LX hasn't done anything yet */
#define	LX_2STATE_POLLED	(i4) 2	/* SECURE message sent, reply pending */
#define	LX_3STATE_WILLING	(i4) 3	/* replied willing to commit */
#define	LX_4STATE_ABORTED	(i4) 4	/* LX aborted */
#define	LX_5STATE_COMMITTED	(i4) 5	/* LX committed */
#define	LX_6STATE_CLOSED	(i4) 6	/* LX closed, either committed or 
					** aborted */
#define	LX_7STATE_ACTIVE	(i4) 7	/* LX is alive */
#define	LX_8STATE_REFUSED	(i4) 8	/* LDB has refused to commit */

    i4		tlx_ldb_status;	/* LX's current LDB status */

#define	LX_00LDB_OK		    (i4) 0
#define	LX_01LDB_UNEXPECTED_ERR	    (i4) 1
				/* unexpected (unrecognized) error */
#define	LX_02LDB_DX_ID_NOTUNIQUE    (i4) 2
				/* rejected a non-unique DX id */
#define	LX_03LDB_DX_ID_UNKNOWN	    (i4) 3
				/* rejected unrecognized DX id */
#define	LX_04LDB_DX_ALREADY_ADOPTED (i4) 4
				/* the DX is already owned by another server */
#define	LX_05LDB_NOSECURE	    (i4) 5
				/* rejected a SECURE message due to the LDB's
				** clustering environment */
#define	LX_06LDB_ILLEGAL_STMT	    (i4) 6
				/* rejected an illegal statement; only ROLLBACK
				** or COMMIT in SECURE state */
#define	LX_07LDB_DB_UNKNOWN	    (i4) 7
				/* connected LDB not in the specified DX (LX
				** may have been manually terminated) */
#define	LX_08LDB_NO_SUCH_LDB	    (i4) 8
				/* LDB does not exist */
#define	LX_09LDB_ASSOC_LOST	    (i4) 9
				/* no association to LDB */

    i4	tlx_20_flags;	/* information about this LDB/LX */

#define	LX_00FLAG_NIL	0x0000
#define	LX_01FLAG_REG	0x0001
				/* ON if the LDB has been registered, i.e.,
				** has executed a subquery and its LX is 
				** part of the DX; OFF if the LDB has an
				** LX CB in the LX list but not yet registered
				** in the DX */
#define	LX_02FLAG_2PC	0x0002
				/* ON if the LDB supports the 2PC protocol,
				** OFF otherwise meaning that this is a 1PC 
				** site (the first update performed by such 
				** a site turns a DX into 1PC) */

#define	LX_03FLAG_RESTARTED	0x0004
				/* ON if LDB has restarted; OFF otherwise */

    struct _TPD_LX_CB
		*tlx_21_prev_p;	/* previous LX CB in DX's list, NULL if this
				** is the first */
    struct _TPD_LX_CB
		*tlx_22_next_p;	/* next LX CB in DX's list, NULL if this is
				** the last */
    DD_DX_ID	tlx_23_lxid;	/* LX id */
}   TPD_LX_CB;


/*}
** Name: TPD_DX_CB - distributed transaction table entry
**
** Description:
**	A table entry for an open distributed transaction.
**
** History:
**      mar-3-88 (alexh)
**          created.
**	jun-03-89 (carl)
**	    added tps_sp_cnt to register number of current savepoints 
**	    known to the LDB
**	oct-29-89 (carl)
**	    modified for 2PC
*/

typedef struct _TPD_DX_CB
{
    i4		tdx_lang_type;		/* transaction language type */

    i4		tdx_state;		/* DX state */

#define	DX_0STATE_INIT		(i4) 0
#define	DX_1STATE_BEGIN		(i4) 1
#define	DX_2STATE_SECURE	(i4) 2
#define	DX_3STATE_ABORT		(i4) 3
#define	DX_4STATE_COMMIT	(i4) 4
#define	DX_5STATE_CLOSED	(i4) 5

    i4		tdx_mode;		/* DX type */

#define	DX_0MODE_NO_ACTION	(i4) 0	/* no dml action on txn yet */
#define	DX_1MODE_READ_ONLY	(i4) 1	/* read-only */
#define	DX_2MODE_2PC		(i4) 2	/* multiple update sites */
#define	DX_3MODE_HARD_1PC	(i4) 3	/* hard 1PC -- owner site does not
					** support 2PC */
#define	DX_4MODE_SOFT_1PC	(i4) 4	/* soft 1PC -- owner site does 
					** support 2PC */
#define DX_5MODE_1PC_PLUS	(i4) 5 /* multiple update sites, exactly
					** one does not support 2PC */

    i4	tdx_flags;		/* status bits */

#define DX_00FLAG_NIL		    0x0000
#define DX_01FLAG_DDB_IN_RECOVERY   0x0001	
					/* ON if the DDB is being recovered,
					** OFF otherwise, i.e. DDB is 
					** fully operational */
    DD_DX_ID	tdx_id;			/* DX id */
    DD_DDB_DESC	tdx_ddb_desc;		/* DDB descriptor, initialized when 
					** TPF_SET_DDBINFO is called */
    i4		tdx_ldb_cnt;		/* number of registered LDBs, 0 if 
					** none */

    /* streams for savepoints, registered and pre-registered LX CBs */
/*
    TPD_LM_CB	tdx_21_pre_lx_ulmcb;	** stream CB for registered LX CB 
					** list **
*/
    TPD_LM_CB	tdx_22_lx_ulmcb;	/* stream CB for pre-registered LX CB 
					** list */
    TPD_LM_CB	tdx_23_sp_ulmcb;	/* stream CB for savepoint list */
    TPD_LX_CB  *tdx_24_1pc_lxcb_p;	/* ptr to the registered LX CB if this 
					** DX is in 1PC, NULL if DX is read-
					** only or in 2PC */
/*
***************************************************************************

    PTR		tdx_11_sp_streamid;	** id of stream for savepoints,
					** NULL if none **
    i4		tdx_12_sp_cnt;		** number of current savepoints, 0 
					** if none **
    TPD_SP_CB	*tdx_13_frst_sp_p;	** ptr to first of savepoint list, 
					** NULL if none **
    TPD_SP_CB	*tdx_14_last_sp_p;	** ptr to last of savepoint list, 
					** NULL if none **
#define	TPD_32DX_MAX_SLAVES	10

    TPD_LX_CB	tdx_head_slave[TPD_32DX_MAX_SLAVES];	
					** local transaction (to be replaced
					** by tdx_22_reg_lx_ulmcb **

****************************************************************************
*/
}   TPD_DX_CB;


/*}
** Name: TPD_SS_CB - TPF session control block
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
**	oct-05-90 (carl)
**	    changed tss_3_dbg_flags to tss_3_trace_vector 
*/

typedef struct _TPD_SS_CB
{

#define	TPTCB_ASCII_ID	CV_CONST_MACRO('T', 'P', 'T', 'B')

    TPF_HEADER	tss_header;		/* standard control block header */
    PTR		tss_rqf;		/* RQF session context */
    TPD_DX_CB	tss_dx_cb;		/* distributed transaction CB */
    bool	tss_auto_commit;	/* transaction auto-commit */
    bool	tss_ddl_cc;		/* TRUE if DDL concurrency is ON, FALSE
					** otherwise; set and reset by direct 
					** calls to TPF */
#define	TSS_119_TRACE_POINTS	119	

    ULT_VECTOR_MACRO(TSS_119_TRACE_POINTS, 1)
		tss_3_trace_vector;	/* trace point vector */

#define	TSS_TRACE_END_DX_100	100	/* ON if DX consummation tracing is
					** desired */
#define	TSS_TRACE_TPFCALL_101	101	/* ON if TPF call tracing is desired */

#define	TSS_TRACE_QUERIES_102	102	/* ON if internal-query tracing is
					** desired */

#define	TSS_TRACE_SVPT_103	103	/* ON if savepoint tracing is desired */

#define	TSS_TRACE_ERROR_104	104	/* ON if error tracing is desired */

#define	TSS_TRACE_LDBS_105	105	/* ON if LDB tracing is desired */

    i4		tss_4_dbg_2pc;		/* debug enumeration as defined below,
					** initialized to 0 */

/* trace points 60..63 for crashing around the session's SECURE actions */

#define	TSS_60_CRASH_BEF_SECURE	    (i4) 60
					/* simulate server crash immediately
					** before logging the next SECURE */
#define	TSS_61_CRASH_IN_SECURE	    (i4) 61
					/* simulate server crash after
					** logging the next SECURE */
#define	TSS_62_CRASH_AFT_SECURE	    (i4) 62
					/* simulate server crash immediately
					** after the next SECURE is committed */
#define	TSS_63_CRASH_IN_POLLING	    (i4) 63
					/* simulate server crash in the middle
					** of the polling phase after 1 or more
					** sites have reponded willing to
					** commit */

/* trace points 65..68 for suspending around the session's SECURE actions */

#define	TSS_65_SUSPEND_BEF_SECURE   (i4) 65
					/* suspend the session immediately
					** before logging the next SECURE */
#define	TSS_66_SUSPEND_IN_SECURE    (i4) 66
					/* suspend the session after
					** the next SECURE is logged */
#define	TSS_67_SUSPEND_AFT_SECURE   (i4) 67
					/* suspend the session immediately
					** after the next SECURE is committed 
					** with the CDB */
#define	TSS_68_SUSPEND_IN_POLLING   (i4) 68
					/* suspend the session in the middle
					** of the polling phase after 1 or more
					** sites have reponded willing to
					** commit */

/* trace points 70..72 for crashing around the session's COMMIT actions */

#define	TSS_70_CRASH_BEF_COMMIT	    (i4) 70
					/* simulate server crash immediately
					** before logging the next 2PC COMMIT */
#define	TSS_71_CRASH_IN_COMMIT	    (i4) 71
					/* simulate server crash after
					** the the next 2PC COMMIT is logged */
#define	TSS_72_CRASH_AFT_COMMIT	    (i4) 72
					/* simulate server crash immediately
					** after the next 2PC COMMIT is
					** committed with the CDB */
#define	TSS_73_CRASH_LX_COMMIT	    (i4) 73
					/* simulate server crash after
					** committing 1 or more sites */

/* trace points 75..77 for suspending around the session's COMMIT actions */

#define	TSS_75_SUSPEND_BEF_COMMIT   (i4) 75
					/* suspend the session immediately
					** before logging the next 2PC 
					** COMMIT */
#define	TSS_76_SUSPEND_IN_COMMIT    (i4) 76
					/* suspend the session after 
					** logging the next 2PC COMMIT */
#define	TSS_77_SUSPEND_AFT_COMMIT   (i4) 77
					/* suspend the session immediately
					** after the next 2PC COMMIT is
					** committed with the CDB */
#define	TSS_78_SUSPEND_LX_COMMIT    (i4) 78
					/* suspend session after
					** committing 1 or more sites */


/* trace points 80..82 for crashing around the session's SECURE/ABORT actions */

#define	TSS_80_CRASH_BEF_ABORT	    (i4) 80
					/* simulate server crash immediately
					** before logging the next 2PC ABORT */
#define	TSS_81_CRASH_IN_ABORT	    (i4) 81
					/* simulate server crash after
					** logging the next 2PC ABORT */
#define	TSS_82_CRASH_AFT_ABORT	    (i4) 82
					/* simulate server crash immediately
					** after the next 2PC ABORT is committed
					** with the CDB */
#define	TSS_83_CRASH_LX_ABORT	    (i4) 83
					/* simulate server crash after
					** aborting 1 or more sites */


/* trace points 85..87 for suspending around the session's COMMIT actions */

#define	TSS_85_SUSPEND_BEF_ABORT    (i4) 85
					/* suspend the session immediately
					** before logging the next 2PC ABORT */
#define	TSS_86_SUSPEND_IN_ABORT	    (i4) 86
					/* suspend the session after
					** logging of the next 2PC ABORT */
#define	TSS_87_SUSPEND_AFT_ABORT    (i4) 87
					/* suspend the session immediately
					** after the next 2PC ABORT is committed
					** with the CDB */
#define	TSS_88_SUSPEND_LX_ABORT	    (i4) 88
					/* suspend session after
					** aborting 1 or more sites */


/* trace points 90..91 for suspending around the next CDB actions */

#define	TSS_90_CRASH_STATE_LOOP	    (i4) 90
					/* crash the server immediately 
					** before executing loop to log the
					** session's next DX state with the
					** CDB */

#define	TSS_91_CRASH_DX_DEL_LOOP    (i4) 91
					/* crash the server immediately 
					** before executing loop to delete
					** session's DX from the CDB */

#define	TSS_95_SUSPEND_STATE_LOOP   (i4) 95
					/* suspend the session immediately 
					** before executing loop to log the
					** session's next DX state with the
					** CDB */

#define	TSS_96_SUSPEND_DX_DEL_LOOP  (i4) 96
					/* suspend the session immediately 
					** before executing loop to delete
					** session's DX from the CDB */

    i4	tss_5_dbg_timer;	/* suspend-timer in seconds for above
					** trace points */

    /* stream for this session CB */

    PTR		tss_6_sscb_streamid;    /* id of stream for this session CB,
					** NULL if none */
    PTR		tss_7_sscb_ulmptr;	/* ptr to this CB in ulm memory */

}   TPD_SS_CB;


/*}
** Name: TPD_OX_CB - orphaned-DX control block
**
** Description:
**	Used for building linked list of orphaned-DX information read off 
**	iidd_ddb_dxlog.
**
** History:
**      nov-28-89 (carl)
**          created
*/

typedef struct _TPD_OX_CB
{
  TPD_SS_CB	    tox_0_sscb;		/* private session CB for this ODX */
  TPC_D1_DXLOG	    tox_1_dxlog;	/* information read off iidd_ddb_dxlog 
					*/
  i4	    tox_2_flags;	/* control flags */

#define	TPD_0OX_FLAG_NIL    0
#define	TPD_1OX_FLAG_DONE   0x0001	/* ON if DX recovered, OFF otherwise
					** requiring recovery */

  struct _TPD_OX_CB *tox_3_prev_p;	/* predecessor in list, NULL if none */
  struct _TPD_OX_CB *tox_4_next_p;	/* successor in list, NULL if none */
} TPD_OX_CB;


/*}
** Name: TPD_S4_CB - semaphore control block
**
** Description:
**	Used for a i4 that must be accessed under a semaphore.
**
** History:
**      jan-25-99 (carl)
**          created
*/

typedef struct _TPD_S4_CB
{
  CS_SEMAPHORE	    ts4_1_sema4;	/* semaphore for the P and V operations
					*/
  SIZE_TYPE	    ts4_2_i4;	/* a i4 */
} TPD_S4_CB;


/*}
** Name: TPD_SV_CB - TPF server control block
**
** Description:
**      TPF server control block. One such block is required for a TPF
**	server.
**
** History:
**      apr-19-88 (alexh)
**	    created
**	jul-11-89 (carl)
**	    added tpv_ses_ulm and tpv_memleft for allocating and deallocating 
**	    session streams
*/

typedef struct _TPD_SV_CB
{
    TPF_HEADER	tsv_header; /* standard control block header */

#define	TPVCB_ASCII_ID	CV_CONST_MACRO('T', 'P', 'V', 'B')

    bool	tsv_1_printflag;	/* TRUE if to log to the dbms log is 
					** desired, FALSE otherwise */
    bool	tsv_2_skip_recovery;	/* TRUE to skip recovery at server
					** startup time, FALSE otherwise;
					** initialized to FALSE */
    i4	tsv_4_flags;		/* various server flags as defined below
					*/

#define	TSV_01_IN_RECOVERY	0x0001	/* ON if recovery is in progress, OFF 
					** otherwise */
    SIZE_TYPE	tsv_5_max_poolsize;	/* server's memory pool size, 
					** initialized once and only once at 
					** server startup time and hence cannot
					** be updated subsequently */
    PTR		tsv_6_ulm_poolid;	/* server's ulm pool id, initialized 
					** once and only once at server
					** startup time and hence cannot
					** be updated subsequently */
    TPD_S4_CB	tsv_7_ulm_memleft;	/* available ULM stream space, used 
					** for stream allocation, semaphore
					** protocols NOT necessary for a 
					** single-processor system */
    TPD_LM_CB	tsv_8_cdb_ulmcb;	/* stream CB for all CDBs, used only 
					** by recovery during server startup 
					** time (no semaphore protocol) */
    TPD_LM_CB	tsv_9_odx_ulmcb;	/* stream CB for orphan DXs of a CDB
					** that's being recovered, used ony
					** by recovery during server startup 
					** time (no semaphore protocol) */
    TPD_LM_CB  *tsv_10_slx_ulmcb_p;	/* special stream CB for all the LXs of 
					** a DX that's being recovered, used ony
					** by recovery during server startup 
					** time (no semaphore protocol) */
    CS_SEMAPHORE
		tsv_11_srch_sema4;	/* semaphore for the P and V operations
					** on the following CDB CB */
    TPD_LM_CB	tsv_12_srch_ulmcb;	/* stream CB for CDBs, used for
					** finding out whether a certain
					** DDB is under recovery before
					** allowing the session to access the
					** DDB in question (semaphore protocol
					** required) */
}   TPD_SV_CB;

