/* Copyright (c) 1995, 2005 Ingres Corporation
**
**
*/

/*
** Name: DMVE.H - Recovery interface definitions.
**
** Description:
**      This file describes the interface to the recovery level
**	services.
**
** History:
**      27-jun-1986 (ac)
**          Created new for Jupiter.
**	6-jul-1992 (jnash)
**	    Prototyping DMF.
**	26-oct-1992 (rogerk & jnash)
**	  - Reduced Logging Project: Added new prototype definitions.
**	  - Took out old dmv_(put,del,rep) routines - they are now static
**	    to their dmve modules.
**	  - Took out dmve_fswap FUNC_EXTERN.  We now used dmve_rename
**	    in its place.
**	  - Add dmve_partial_recov_check and dmve_crverify FUNC_EXTERN.
**	22-feb-93 (jnash)
**	    More Reduced logging.  Add dmve_logging.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-jul-1993 (rogerk)
**	    Added dmve_trace_page_info routine.
**	20-sep-1993 (rogerk)
**	    Added rollforward recovery event tracking - structure definitions
**	    and routine prototypes: RFP_REC_ACTION, rfp_add_action,
**	    rfp_lookup_action, rfp_remove_action.
**	18-oct-1993 (jnash)
**	    Added prototypes: dmve_trace_page_header(), dmve_trace_page_lsn(),
**	    dmve_trace_page_contents().
**	18-oct-93 (jrb)
**	    Added prototype for dmve_ext_alter.
**	18-oct-1993 (rogerk)
**	    Added dmve_unreserve_space routine.
**	31-jan-1994 (jnash)
**	    In _RFP_REC_ACTION, add rfp_act_tabname, rfp_act_tabown.
**      06-mar-1996 (stial01)
**          Added page_size to prototype for dmve_trace_page_info and
**          dmve_trace_page_header.
**	06-may-1996 (thaju02)
**	    Added page_size to prototype for dmve_trace_page_lsn().
**	20-sep-1996 (shero03)
**	    Support rtput, rtdel, and rtrep
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          dmve_btundo_check() prototype: Added row_locking,reclaim_space,page
**      27-feb-97 (stial01)
**          Renamed dmve_btundo_check -> dmve_bid_check and modified for REDO
**      10-mar-97 (stial01)
**          Added prototpe for dmve_clean
**      30-jun-97 (stial01)
**          Changed prototype for dmve_bid_check
**	28-aug-97 (nanpr01)
**	    Page size was added in the trace routine.
**	11-sep-97 (thaju02)
**	    Added tbl_lock parameter to dmve_btundo_check() function
**	    prototype as part of fix for bug #85156.
**      06-jan-97 (stial01)
**          Changed prototype for dmve_bid_check (B87385)
**      28-may-98 (stial01)
**          Changed prototype for dmve_clean
**      07-jul-98 (stial01)
**          Changed prototype for dmve_clean
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Arg changes for Variable Page Type SIR 102955)
**      08-Oct-2002 (hanch04)
**          Replace the i4 for length with a SIZE_TYPE (size_t) to allow
**          64-bit memory allocation.
**      20-jun-2003 (stial01)
**          Moved declaration of dmve_rtadjust_mbrs() to dmve.h
**      14-jul-2003
**          Changed declaration of dmve_rtadjust_mbrs to use DMPP_PAGE
**      30-Mar-2004 (hanal04) Bug 107828 INGSRV2701
**          Added prototype for dmve_get_iirel_row().
**	5-Apr-2004 (schka24)
**	    Add rawdata pointers to dmve cb.
**	29-Apr-2004 (gorvi01)
**		Added function defnition for dmve_del_location.
**	14-May-2004 (hanje04)
**	    Removed <<<< from after prototype for dmve_del_location.
**	01-Dec-2004 (jenjo02)
**	    Added fix_action to dmve_rtadjust_mbrs prototype for
**	    bug 108074.
**	10-jan-2005 (thaju02)
**	    Relocated RFP_OCTX from dmfrfp.h.
**      07-Jun-2005 (inifa01) b111211 INGSRV2580
**          Verifydb reports error; S_DU1619_NO_VIEW iirelation indicates that
**          there is a view defined for table a (owner ingres), but none exists,
**          when view(s) on a table are dropped.
**          Were making DMT_ALTER call on DMT_C_VIEW, to turn on iirelation.relstat
**          TCB_VBASE bit, indicating view is defined over table(s).  TCB_VBASE
**          not tested anywhere in 2.6 and not turned off when view is dropped.
**          removeing references to TCB_VBASE here.
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**	06-Mar-2007 (jonj)
**	    Added dmve_cachefix_page() prototype for Cluster REDO.
**      14-jan-2008 (stial01)
**          Added dmve_unfix_tabio() prototype for dmve exception handling
**	14-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**      23-Feb-2009 (hanal04) Bug 121652
**          Added dmve_ufmap().
**      12-oct-2009 (stial01)
**          Changed dmve_bid_check prototype, added ADF_CB to DMVE_CB.
**      14-oct-2009 (stial01)
**          Prototype and structure changes for dmve page fix/unfix
**      29-Apr-2010 (stial01)
**          Added prototypes for dmve_iirel_cmp, dmve_iiseq_cmp
**      10-May-2010 (stial01)
**          Added dmve_bid_check_error
**/


/*
**  Forward references
*/

typedef struct  _DMVE_TAB		DMVE_TAB;
typedef struct	_DMVE_CB		DMVE_CB;
typedef struct	_DMVE_FIX_HISTORY	DMVE_FIX_HISTORY;
typedef struct	_RFP_REC_ACTION		RFP_REC_ACTION;
typedef struct	_RFP_OCTX		RFP_OCTX;
typedef struct	_DMVE_BT_ATTS		DMVE_BT_ATTS;



/*
**  Defines of other constants.
*/
#define			DMVE_BYTID	    0x0001L
#define			DMVE_BYKEY	    0x0002L
#define			DMVE_NODUPLICATES   0x0004L
#define			DMVE_DUPLICATES     0x0008L
#define			DMVE_UPDATE_SINDEX  0x0010L
#define			DMVE_SINDEX_CHK	    0x0020L

/*
** Parameters to rfp_lookup_action calls.
*/
#define			RFP_REC_LSN	    0x0001L
#define			RFP_REC_NEXT	    0x0002L

/*
** DMVE_FIX_HISTORY 
**
**
*/

#define DMVE_PGFIX_MAX	3 /* max pages locked/fixed/buflocked in dmve */

struct _DMVE_FIX_HISTORY
{
    DMP_PINFO		pinfo; 
    DM_PAGENO		page_number;
    i4			fix_action;
    LK_LKID		page_lockid;
};



/*}
** Name:  DMVE_BT_ATTS - DMVE attribute information
**
** Description: DMVE attribute information
**        This may point to attribute information in the tcb
**        or if we have a partial tcb this may point to attribute information
**        constructed from attribute information on the leaf page.
**
** History:
**      10-May-2010 (stial01)
**          Moved from dmvebtpt.c
*/
struct _DMVE_BT_ATTS
{
    /* fields in DMVE_BT_ATTS correspond so similarly named field in DMP_TCB */

    i4		    bt_keys;		/* Number of attributes in key. */
    i4		    bt_klen;		/* Length of key in bytes, sort of;
					** in btree, length of leaf entry
					** including any non-key bytes.
					*/
    DB_ATTS         **bt_leafkeys;	/* Array of LEAF key attribute ptrs */
    DMP_ROWACCESS   bt_leaf_rac;	/* Row-accessor for LEAF entry */
    i4		    bt_kperleaf;	/* Max btree entries per LEAF */
					/* The length of a LEAF is bt_klen */

    i4		    bt_rngklen;		/* LEAF range klen */
    DB_ATTS	    **bt_rngkeys;	/* LEAF range keys */
    DMP_ROWACCESS   bt_rng_rac;		/* Row-accessor for LEAF range entry */
    char	    bt_temp[2048];	/* temp buffer, used for att array,
					** att pointer arrays, compression
					** control arrays.  If arrays don't
					** fit, will dm0m-allocate instead.
					*/
};


/*}
** Name:  DMVE_CB - DMF recovery control block
**
** Description:
**      This typedef defines the control block required for all the
**      dmve_*() DMF recovery routines. These routines are given the
**	DMVE_CB which contains the information necessary for the dmve_*
**	routines to do the work. The action of the recovery are UNDO,
**	DO or REDO. UNDO means to backout a change. DO means to perform
**	a change when doing a roll forward from a restored checkpoint.
**	REDO means to perform a change when doing a roll forward from a
**	consistency point.
**
**	There is one dmve_cb for each transaction to recover.
**
** History:
**      27-jun-86 (ac)
**          Created for Jupiter.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warnings.
**	22-feb-93 (jnash)
**	    Reduced logging project.  Add dmve_logging to indicate whether
**	    Compensation log records should be written during undo recovery.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	5-Apr-2004 (schka24)
**	    Add rawdata info pointers.
**	22-apr-2004 (thaju02)
**	    Online Modify. Added dmve_octx.
**	30-aug-2004 (thaju02) Bug 111470 INGSRV2635
**	    Added dmve_flags & DMVE_ROLLDB_BOPT.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define DMVECB_CB as DM_DMVECB_CB.
**	11-Oct-2007 (jonj)
**	    Add DMVE_CSP dmve_flag - recovery being
**	    driven by CSP.
**	03-Nov-2008 (jonj)
**	    Changed type of dmve_error from obsolete DM_ERR to DB_ERROR.
**	17-Nov-2008 (jonj)
**	    Added dmve_logfull to use instead of dmve_error.err_data
**	    when preserving and returning E_DM9070_LOG_LOGFULL for
**	    Bug 56702.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add DMVE_MVCC, dmve_page.
**	23-Feb-2010 (stial01)
**          Added dmve_rcb to DMVE_CB
**	15-Apr-2010 (stial01)
**          Added mvcc trace fields to DMVE_CB
*/
struct _DMVE_CB
{
    DMVE_CB	    *dmve_next;		    /* Next dmve_cb. */
    DMVE_CB	    *dmve_prev;		    /* Previous dmve_cb. */
    SIZE_TYPE	    dmve_length;            /* Length of control block. */
    i2		    dmve_type;              /* Control block type. */
#define                 DMVECB_CB		    DM_DMVECB_CB
    i2		    dmve_s_reserved;	    /* Reserved for future use. */
    PTR		    dmve_l_reserved;	    /* Reserved for future use. */
    PTR   	    dmve_owner;		    /* Owner of this control block. */
    i4		    dmve_ascii_id;	    /* Ascii id to aid in debugging. */
#define			DMVE_ASCII_ID	    CV_C_CONST_MACRO('D', 'M', 'V', 'E')
    DB_ERROR	    dmve_error;		    /* The error block. */
    i4		    dmve_logfull;	    /* E_DM9070_LOG_LOGFULL if log is full */
    PTR		    dmve_log_rec;	    /* The log record. */
    i4	    	    dmve_action;	    /* Action of the recovery. */
#define                 DMVE_UNDO	1L
#define			DMVE_DO		2L
#define			DMVE_REDO	3L
    LG_LA	    dmve_lg_addr;    	    /* Log address of the log record. */
    DMP_DCB	    *dmve_dcb_ptr;	    /* Pointer to DCB. */
    ADF_CB	    dmve_adf_cb;
    DB_TRAN_ID	    dmve_tran_id;	    /* The transaction id. */
    i4	    	    dmve_lk_id;		    /* The transaction lock list id. */
    i4	    	    dmve_log_id;	    /* The logging system transaction
					    ** context id.*/
    BITFLD	    dmve_logging:1;	    /* Logging needed flag. */
    BITFLD	    dmve_bitfld:31;	    /* Unused */
    i4	    	    dmve_db_lockmode;	    /* The lockmode of the database. */
    RFP_REC_ACTION  *dmve_rolldb_action;    /* Rollforward action list. */
    u_i4	    dmve_flags;
#define			DMVE_ROLLDB_BOPT	0x0001	/* rolldb -b specified */
#define			DMVE_CSP		0x0002	/* CSP doing recovery */
#define			DMVE_MVCC		0x0004  /* MVCC doing recovery*/
    DMP_PINFO	    *dmve_pinfo;    	    /* CR undo page info */

#define DMVE_PGFIX_MAX		3	/* max pages fixed by dmve */
    DMVE_FIX_HISTORY	dmve_pages[DMVE_PGFIX_MAX];
    i4			dmve_lk_type;

    /* These pointers are used to pass random data logged as RAWDATA
    ** log records, to the recovery action.  RAWDATA will typically only
    ** be of interest to journal DO.
    ** Note: the dmve_cb is an intermediary, the (de)allocation of the raw
    ** data holding tanks will typically be managed elsewhere (eg rfp).
    */
    PTR		    dmve_rawdata[DM_RAWD_TYPES];  /* Raw data info if any */
    i4		    dmve_rawdata_size[DM_RAWD_TYPES];  /* Size of each */
    RFP_OCTX	    *dmve_octx;		    /* rolldb online modify context */

    DMPP_ACC_PLV    *dmve_plv;

    DMP_TABLE_IO    *dmve_tbio;             /* for offline recovery */
    DMP_TCB	    *dmve_tcb;
    DMP_RCB	    *dmve_rcb;
    char	    *dmve_prev_rec;	/* for tracing mvcc undo failure */
    i4		    *dmve_prev_cnt;     /* for tracing mvcc undo failure */
    i4		    dmve_prev_size;    /* for tracing mvcc undo failure */

};

/*}
** Name: RFP_REC_ACTION - Rollforward action list entry.
**
** Description:
**	This structure defines a member of the rollforward action list.  
**	It describes work done by recovery processing in rollforward that
**	is needed to be remembered so that appropriate actions can be taken
**	later.
**
**	Actions that are currently added to this list:
**
**	    FRENAME operations that rename a file to a delete file name.
**		These operations are added so we can remember to remove the
**		physical file when the End Transaction record is found.
**
**	    DELETE operations on system catalog rows corresponding to non
**		journaled tables.  These operations are added so that if/when
**		undo of such an action is required (or a CLR of such an action
**		is encountered) we can tell whether or not to restore the
**		deleted row.  See non-journaled table handling notes in DMFRFP.
**
**	    REPLACE operations on iirelation rows corresponding to non
**		journaled tables.  These operations are added so that if/when
**		an undo of such an action is required we can tell what items
**		of the iirelation row need to be restored.  See non-journaled
**		table handling notes in DMFRFP.
**
** History:
**	20-sep-1993 (rogerk)
**	    Created.
**	31-jan-1994 (jnash)
**	    Add rfp_act_tabname, rfp_act_tabown.
**	20-sep-1996 (shero03)
**	    Support rtput, rtdel, and rtrep
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define RFP_OCTX_CB as DM_RFP_OCTX_CB.
**	23-nov-2006 (dougi)
**	    Add TCB_VBASE back into RFP_RELSTAT_VALUES.
[@history_template@]...
*/
struct _RFP_REC_ACTION
{
    RFP_REC_ACTION	*rfp_act_next;
    RFP_REC_ACTION	*rfp_act_prev;
    DB_TRAN_ID		rfp_act_xid;
    i4		rfp_act_action;
#define			    RFP_FRENAME		1
#define			    RFP_DELETE		2
#define			    RFP_REPLACE		3
    i4		rfp_act_dbid;
    DB_TAB_NAME		rfp_act_tabname;
    DB_OWN_NAME 	rfp_act_tabown;
    DB_TAB_ID		rfp_act_tabid;
    DB_TAB_ID		rfp_act_usertabid;
    DM_TID		rfp_act_tid;
    LG_LSN		rfp_act_lsn;
    bool		rfp_act_index_drop;	/* Indicates that the action
						** was a DELETE of iirelation
						** row for a secondary index */
    bool		rfp_act_idxbit_changed; /* Indicates that the TCB_IDXD
						** bit of an iirelation relstat
						** column was updated during
						** recovery of an iirelation
						** replace. */
    bool		rfp_act_idxcnt_changed; /* Indicates that relidxcount
						** field of an iirelation row
						** was updated during recovery
						** of an iirelation replace. */
    i4		rfp_act_oldidx_count;	/* If the index bit is turned
						** off as part of rollforward
						** of an iirelation replace,
						** this stores the old value of
						** the relidxcount field so it
						** can be restored in undo. */
    i4		rfp_act_p_len;		/* FILE information for      */
    i4		rfp_act_f_len;		/*   FRENAME operations.     */
    DM_PATH		rfp_act_p_name;	
    DM_FILE		rfp_act_f_name;
};

struct _RFP_OCTX
{
    RFP_OCTX        *octx_next;
    RFP_OCTX        *octx_prev;
    SIZE_TYPE       octx_length;
    i2	            octx_type;
#define         RFP_OCTX_CB     	DM_RFP_OCTX_CB
    i2              octx_s_reserved;    /* Reserved for future use. */
    PTR             octx_l_reserved;    /* Reserved for future use. */
    PTR             octx_owner;         /* Owner of control block */
    i4              octx_ascii_id;
    #define  RFP_OCTX_ASCII_ID            CV_C_CONST_MACRO('O', 'C', 'T', 'X')
    DB_TRAN_ID      octx_bsf_tran_id;
    LG_LSN          octx_bsf_lsn;
    DB_TAB_ID       octx_bsf_tbl_id;
    DB_TAB_NAME     octx_bsf_name;
    PTR             octx_mxcb;          /* ptr to DM2U_MXCB block for load */
    PTR             octx_osrc_next;
};

/*
** List of iirelation relstat fields that DO get rolled forward when
** updates are logged to iirelation for a table is not journaled.  All
** other relstat fields are left the way they were in the original
** iirelation row brought back from the checkpoint.
*/
#define	RFP_RELSTAT_VALUES	(TCB_JOURNAL | TCB_JON |  TCB_VBASE | \
				 TCB_INTEGS | TCB_RULE | TCB_COMMENT | \
				 TCB_SYNONYM | TCB_PRTUPS | TCB_ZOPTSTAT | \
				 TCB_PROALL | TCB_PROTECT)

/* Macros to mutex "page", excluding CR pages */
#define dmveMutex(dmve, pinfo) \
    if ( !(dmve->dmve_flags & DMVE_MVCC) ) \
        dm0pMutex(dmve->dmve_tbio, 0, dmve->dmve_lk_id, (DMP_PINFO*)pinfo);
#define dmveUnMutex(dmve, pinfo) \
    if ( !(dmve->dmve_flags & DMVE_MVCC) ) \
        dm0pUnmutex(dmve->dmve_tbio, 0, dmve->dmve_lk_id, (DMP_PINFO*)pinfo);
 
/*
**  External function references.
*/

FUNC_EXTERN DB_STATUS dmve_undo(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_redo(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_alloc(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_alter(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_assoc(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_bipage(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_crdb(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_create(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_dealloc(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_del(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_destroy(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_dmu(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_extend(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_fcreate(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_frename(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_index(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_jnleof(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_load(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_location(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_ext_alter(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_modify(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_ovfl(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_put(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_relocate(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_rep(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_sbipage(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_sdel(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_setbof(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_sm0_closepurge(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_sm1_rename(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_sm2_config(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_sput(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_sput(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_srep(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS dmv_rel_get(
			DMP_RELATION	*key_tuple,
			DMP_RELATION    *record );

FUNC_EXTERN DB_STATUS dmv_idx_get(
			DMP_RINDEX	*key_tuple,
			DMP_RINDEX	*record );

FUNC_EXTERN DB_STATUS dmve_fix_tabio(
			DMVE_CB         *dmve,
			DB_TAB_ID       *table_id,
			DMP_TABLE_IO    **table_io);

FUNC_EXTERN DB_STATUS dmve_unfix_tabio(
			DMVE_CB		*dmve,
			DMP_TABLE_IO	**table_io,
#define DMVE_INVALID		0x01
			i4		flag);

FUNC_EXTERN bool      dmve_location_check(
			DMVE_CB		*dmve,
			i4		location_cnf_id);

FUNC_EXTERN VOID      dmve_name_unpack(
			char            *tab_name_ptr,
			i4         tab_name_size,
			DB_TAB_NAME     *tab_name_buf,
			char            *own_name_ptr,
			i4         own_name_size,
			DB_OWN_NAME     *own_name_buf);

FUNC_EXTERN DB_STATUS dmve_fmap(
			DMVE_CB         *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_ufmap(
			DMVE_CB         *dmve_cb);

FUNC_EXTERN DB_STATUS dmve_nofull(
			DMVE_CB         *dmve);

FUNC_EXTERN bool 	dmve_partial_recov_check(
			DMVE_CB         *dmve);

FUNC_EXTERN DB_STATUS 	dmve_crverify(
			DMVE_CB		*dmve_cb);

FUNC_EXTERN DB_STATUS 	dmve_aipage(
			DMVE_CB		*dmve_cb);

FUNC_EXTERN DB_STATUS 	dmve_btput(
			DMVE_CB		*dmve_cb);

FUNC_EXTERN DB_STATUS 	dmve_btdel(
			DMVE_CB		*dmve_cb);

FUNC_EXTERN DB_STATUS 	dmve_btsplit(
			DMVE_CB		*dmve_cb);

FUNC_EXTERN DB_STATUS 	dmve_btovfl(
			DMVE_CB		*dmve_cb);

FUNC_EXTERN DB_STATUS 	dmve_btfree(
			DMVE_CB		*dmve_cb);

FUNC_EXTERN DB_STATUS 	dmve_btupdovfl(
			DMVE_CB		*dmve_cb);

FUNC_EXTERN DB_STATUS 	dmve_disassoc(
			DMVE_CB		*dmve_cb);

FUNC_EXTERN DB_STATUS	dmve_bid_check(
			DMVE_CB             *dmve,
			i4		    opflag,
#define DMVE_FINDLEAF 0x01
#define DMVE_FINDKEY  0x02
#define DMVE_FINDPOS  0x04
			DB_TAB_ID	    *tableid,
			DM_TID		    *log_bid,
			DM_TID		    *log_tid,
			char		    *log_key,
			i4		    log_key_size,
			DMP_TABLE_IO        **tbio,
			DM_TID		    *found_bid,
			DMP_PINFO           **pinfoP);

FUNC_EXTERN DB_STATUS	dmve_bid_check_error(
			DMVE_CB             *dmve,
			DMP_TCB             *t,
			DMVE_BT_ATTS	    *btatts,
			i4		    opflag,
			DM_TID		    *log_bid,
			DM_TID		    *log_tid,
			char		    *logkey_ptr,
			i4		    log_key_size,
			DMPP_PAGE	    *page);

FUNC_EXTERN DB_STATUS 	dmve_rtput(
			DMVE_CB		*dmve_cb);

FUNC_EXTERN DB_STATUS 	dmve_rtdel(
			DMVE_CB		*dmve_cb);

FUNC_EXTERN DB_STATUS 	dmve_rtrep(
			DMVE_CB		*dmve_cb);

FUNC_EXTERN DB_STATUS dmve_rtadjust_mbrs(
			DMVE_CB		*dmve,
			ADF_CB		*adf_scb,
			DMP_TABLE_IO	*tbio,
			DB_TAB_ID	tbl_id,
			DM_TID		*ancestors,
			i4		ancestor_level,
			i4		page_type,
			i4		page_size,
			u_i2		hilbert_size,
			i2		key_size,
			i2		compression,
			DMP_PINFO	*pinfo,
			DMPP_ACC_PLV	*plv,
			DMPP_ACC_KLV	*klv,
			i4		fix_action);

FUNC_EXTERN VOID	dmve_trace_page_info(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE	    *page,
			DMPP_ACC_PLV 	    *plv,
			char		    *type);

FUNC_EXTERN DB_STATUS	rfp_add_action(
			DMVE_CB		    *dmve,
			DB_TRAN_ID	    *xid,
			LG_LSN		    *lsn,
			i4		    type,
			RFP_REC_ACTION	    **item);

FUNC_EXTERN VOID	rfp_lookup_action(
			DMVE_CB		    *dmve,
			DB_TRAN_ID	    *xid,
			LG_LSN		    *lsn,
			i4		    flag,
			RFP_REC_ACTION	    **item);

FUNC_EXTERN VOID	rfp_remove_action(
			DMVE_CB		    *dmve,
			RFP_REC_ACTION	    *item);

FUNC_EXTERN VOID 	dmve_trace_page_lsn(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE	*page,
			DMPP_ACC_PLV 	*plv,
			char		*type);

FUNC_EXTERN VOID	dmve_trace_page_header(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE	*page,
			DMPP_ACC_PLV 	*plv, 
			char		*type);

FUNC_EXTERN VOID 	dmve_trace_page_contents(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE	*page, 
			char		*type);

FUNC_EXTERN VOID	dmve_unreserve_space(
			DMVE_CB		    *dmve,
			i4		    log_blocks);

FUNC_EXTERN DB_STATUS   dmve_get_iirel_row(
			DMVE_CB             *dmve,
			DB_TAB_ID           *rel_id,
			DMP_RELATION        *rel_row);

FUNC_EXTERN DB_STATUS   dmve_clean(
			DMVE_CB		    *dmve,
			DMP_PINFO           *pinfo,
			DMP_TABLE_IO        *tabio);

FUNC_EXTERN DB_STATUS dmve_del_location(DMVE_CB *dmve_cb);

FUNC_EXTERN DB_STATUS   dmve_cachefix_page(
			DMVE_CB		*dmve,
			LG_LSN		*log_lsn,
			DMP_TABLE_IO	*tbio,
			DM_PAGENO	page_number,
			i4		fix_action,
			DMPP_ACC_PLV	*plv,
			DMP_PINFO	**pinfo);

FUNC_EXTERN DB_STATUS dmve_fix_page(
			DMVE_CB		*dmve,
			DMP_TABLE_IO	*tbio,
			DM_PAGENO	pageno,
			DMP_PINFO	**pinfo);

FUNC_EXTERN DB_STATUS dmve_unfix_page(
			DMVE_CB		*dmve,
			DMP_TABLE_IO	*tbio,
			DMP_PINFO	*pinfo);

FUNC_EXTERN VOID dmve_cleanup(
DMVE_CB		*dmve);

FUNC_EXTERN DB_STATUS dmve_range_check(
DMVE_CB             *dmve,
DM_TID		    *log_bid,
DM_TID		    *log_tid,
char		    *log_key,
i4		    log_key_size,
DM_TID		    *bidp);

FUNC_EXTERN bool dmve_iirel_cmp(
DMVE_CB		*dmve,
char		*rec1,
i4		size1,
char		*rec2,
i4		size2);

FUNC_EXTERN bool dmve_iiseq_cmp(
DMVE_CB		*dmve,
i2		comptype,
char		*rec1,
i4		size1,
char		*rec2,
i4		size2);
