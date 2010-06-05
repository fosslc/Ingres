/*
** Copyright (c) 1992, 2009 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <tr.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dm2r.h>
#include    <dm2f.h>
#include    <dm1c.h>
#include    <dm1b.h>
#include    <dm1cx.h>
#include    <dm1r.h>
#include    <dm1h.h>
#include    <dmve.h>
#include    <dmpp.h>
#include    <dm0l.h>
#include    <dm0m.h>
#include    <dm0p.h>
#include    <dmd.h>
#include    <dmftrace.h>
#include    <dmpepcb.h>

/**
**
**  Name: DMVEBTPUT.C - The recovery of a btree put key operation.
**
**  Description:
**	This file contains the routine for the recovery of a 
**	put key operation to a btree index/leaf page.
**
**          dmve_btput - The recovery of a put key operation.
**
**  History:
**	14-dec-1992 (rogerk)
**	    Written for 6.5 recovery.
**	18-jan-1992 (rogerk)
**	    Add check in redo/undo routines for case when null page pointer is
**	    passed because recovery was found to be not needed.
**	15-mar-1993 (jnash & rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed log record format: added database_id, tran_id and LSN
**		in the log record header.  The LSN of a log record is now
**		obtained from the record itself rather than from the dmve
**		control block.
**	    Check dmve->dmve_logging to determine if logging required.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Correct the message parameter lengths for E_DM9665.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	21-jun-1993 (rogerk)
**	    Changed dmve_btundo_check delete case to pass dummy tid pointer
**	    to dm1bxsearch when calling to find spot at which to restore the 
**	    deleted key.
**	21-jun-1993 (rogerk)
**	    Removed copy of key from btput CLR records.
**	    Added error messages.
**      21-jun-1993 (mikem)
**          su4_us5 port.  Added include of me.h to pick up new macro
**          definitions for MEcopy, MEfill, ...
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	26-jul-1993 (rogerk)
**	    Added tracing of page information during recovery.
**	    Added tracing of btundo_check logical processing.
**	    Include respective CL headers for all cl calls.
**	15-apr-1994 (chiku)
**	    Bug56702: returned logfull indication.
**	23-may-1994 (jnash)
** 	    Consistency check was checking wrong tid during index
**	    update undo.
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**	06-mar-1996 (stial01 for bryanp)
**	    Pass page_size to dm1cxhas_room.
**      06-mar-1996 (stial01)
**          Pass btp_page_size to dmve_trace_page_info
**	06-may-1996 (thaju02 & nanpr01)
**	    New page format support: change page header references to use
**	    macros.
**      03-june-1996 (stial01)
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**      22-jul-1996 (ramra01 for bryanp)
**          Pass row_version to dmppxuncompress.
**	16-sep-1996 (canor01)
**	    Pass tuple buffer to dmppxuncompress.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Do not get page lock if row locking.
**          Check/adjust bid if row locking.
**          When calling dm1cxdel(), pass reclaim_space param
**     12-dec-96 (dilma04)
**          Corrected mistake in dmve_btput() which caused inconsistency 
**          between row_locking value and DM0L_ROW_LOCK flag after lock 
**          escalation.
**      04-feb-97 (stial01)
**          dmv_unbtree_put() Tuple headers are on LEAF and overflow (CHAIN) pgs
**          Tuple headers are not on INDEX pages.
**      27-feb-97 (stial01)
**          dmve_btput() If row locking entries can shift requiring btundo_check
**          Fix page without locking and then mutex it if we have IX page lock.
**          Error if DO/REDO and only IX page lock.
**          Clean ALL deleted leaf entries if X page lock.
**          dmv_unbtree_put() Space is not reclaimed if IX page lock held 
**          or deleting the last entry on leaf page with overflow chain.
**          Log key in CLR, needed for row locking
**          Renamed dmve_btundo_check -> dmve_bid check, as it is now called
**          for UNDO and REDO. Changed this routine for REDO. 
**      07-apr-97 (stial01)
**          dmv_unbtree_put() NonUnique primary btree (V2) dups span leaf pages,
**          not overflow chain. Remove code skipping reclaim of overflow key
**          dmve_bid_check() NonUnique primary btree (V2), dups can span leaf
**      21-apr-97 (stial01)
**          dmve_bid_check: Search mode is DM1C_FIND for unique keys, DM1C_TID 
**          for non-unique keys, since we re-use unique entries in 
**          dm1b_allocate, the tidp part of the leaf entry may not match.
**      30-apr-97 (stial01)
**          dmve_bid_check: When searching for key, don't overwrite logged tidp
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Add lock_id argument to dm0p_lock_page and dm0p_unlock_page.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      29-may-1997 (stial01)
**          dmve_btput: REDO recovery: bid check only if redo necessary
**      18-jun-1997 (stial01)
**          dmve_btput() Request IX page lock if row locking.
**      30-jun-97 (dilma04)
**          Bug 83301. Set DM0P_INTENT_LK fix action if row locking.
**      30-jun-97 (stial01)
**          Use dm0p_lock_buf_macro to lock buffer to reduce mutex duration
**          Don't unfix tabio for bid check, changed args to dmve_bid_check()
**          dmve_bid_check() Use attr/key info on leaf pages
**          Added local routines to search page for key without rcb.
**	21-jul-1997 (canor01)
**	    Include <st.h>.
**      18-aug-1997 (stial01)
**          dmv_build_bt_info: fix assignment of recovery_action for CLR recs
**      29-jul-97 (stial01)
**          Validate/adjust bid if page size != DM_COMPAT_PGSIZE
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm2t_ufx_tabio_cb() calls.
**      06-jan-98 (stial01)
**          If page size 2k, unfix pages,tbio before bid check (B87385) 
**          dmve_bid_check() LOCK pages we fix, unfix AND UNLOCK if LK_PHYSICAL
**      15-jan-98 (stial01)
**          dm1bxclean() Fixed dm1cxclean parameters, error handling
**      19-Jun-2002 (horda03) Bug 108074
**          Prevent FORCE_ABORT transaction filling TX log
**          file by not flushing SCONCUR pages unless we
**          really need to (i.e. the table isn't locked
**          exclusively).
**	23-Jun-1998 (jenjo02)
**	    Added lock_value parm to dm0p_lock_page() and dm0p_unlock_page().
**      07-jul-98 (stial01)
**          New update_mode if row locking, changed paramater to dmve_bid_check
**	14-aug-1998 (nanpr01)
**	    Error code is getting reset to E_DB_OK on error and not making
**	    the database inconsistent.
**	01-dec-1998 (nanpr01)
**	    Get rid of DMPP_TUPLE_INFO structure. Use DMPP_TUPLE_HDR instead.
**      07-Dec-1998 (stial01)
**          Added kperleaf to distinguish from kperpage (keys per index page)
**      09-feb-99 (stial01)
**          dmve_bid_check() Pass relcmptlvl = 0 to dm1cxkperpage
**      12-apr-1999 (stial01)
**          Different att/key info for LEAF vs. INDEX pages
**      12-nov-1999 (stial01)
**          Changes to how btree key info is stored in tcb
**      08-dec-1999 (stial01)
**          Undo change for bug#85156, dmpe routines now get INTENT table locks.
**      21-dec-1999 (stial01)
**          Defer leaf clean until insert, generalize for etabs (PHYS PG locks)
**	04-May-2000 (jenjo02)
**	    Pass blank-suppressed lengths of table/owner names to 
**	    dm0l_btput, eliminating need for dmve_name_unpack().
**	24-may-2000 (somsa01)
**	    Added LEVEL1_OPTIM for HP-UX 11.0. The compiler was generating
**	    incorrect code with '-O' such that, in dmve_btput(),
**	    "log_rec->btp_page_type != TCB_PG_V1" was failing, even
**	    though log_rec->btp_page_size was equal to DM_COMPAT_PGSIZE.
**      06-jul-2000 (stial01)
**          Changes to btree leaf/index klen in tcb (b102026) 
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Arg changes for page accessors (Variable Page Type SIR 102955)
**          Call dm1cxclean to remove deleted keys
**          Pass pg type to dmve_trace_page_info
**      01-may-2001 (stial01)
**          Fixed parameters to ule_format (B104618)
**      19-Jun-2002 (horda03) Bug 108074
**          Prevent FORCE_ABORT transaction filling TX log
**          file by not flushing SCONCUR pages unless we
**          really need to (i.e. the table isn't locked
**          exclusively).
**      02-oct-2002 (stial01)
**          dmv_search() Init adf_ucollation
**	08-nov-2002 (somsa01)
**	    Removed hpb_us5 from LEVEL1_OPTIM.
**      28-mar-2003 (stial01)
**          Child on DMPP_INDEX page may be > 512, so it must be logged
**          separately in DM0L_BTPUT, DM0L_BTDEL records (b109936)
**      02-jan-2004 (stial01)
**          Added dmv_reblob_put for redo recovery when noblobjournaling,
**          and nobloblogging.
**      14-jan-2004 (stial01)
**          Cleaned up 02-jan-2004 changes.
**	19-Jan-2004 (jenjo02)
**	    Added tidsize, partno for global indexes.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	6-Apr-2004 (schka24)
**	    Don't try to open a partition (master may not be open);
**	    open the master instead.
**      13-may-2004 (stial01)
**          Remove unecessary code for NOBLOBLOGGING redo recovery.
**      31-jan-05 (stial01)
**          Added flags arg to dm0p_mutex call(s).
**	21-feb-05 (inkdo01)
**	    Init Unicode normalization flag.
**      26-sep-05 (stial01)
**          dmve_bid_check() Fixed initialization of collID (b115266)
**      29-sep-2006 (stial01)
**          Support multiple RANGE entry formats: leafkey(old-fmt), indexkey(new-fmt)
**	06-Mar-2007 (jonj)
**	    Replace dm0p_cachefix_page() with dmve_cachefix_page()
**	    for Cluster REDO recovery.
**	19-Apr-2007 (jonj)
**	    ule_format actual ADF error code, not E_DMF012_ADT_FAIL, which
**	    reveals nothing.
**      14-jan-2008 (stial01)
**          Call dmve_unfix_tabio instead of dm2t_ufx_tabio_cb
**	20-Feb-2008 (kschendel) SIR 122739
**	    Use get-plv instead of getaccessors.  Dereference page-type once.
**	    Whack bid-check code around for new rowaccessor scheme.
**	17-Apr-2008 (kibro01) b120276
**	    Initialise ADF_CB structure
**	20-may-2008 (joea)
**	    Add relowner parameter to dm0p_lock_page.
**	14-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**	02-Dec-2008 (jonj)
**	    SIR 120874: dm1b_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**      05-feb-2009 (joea)
**          Rename CM_ischarsetUTF8 to CMischarset_utf8.
**	19-Aug-2009 (kschendel) 121804
**	    Need cm.h for proper CM declarations (gcc 4.3).
**      10-sep-2009 (stial01)
**          DM1B_DUPS_ON_OVFL_MACRO true if btree can have overflow pages
**          Temporarily assume this is always true.
**      12-oct-2009 (stial01)
**          Use DMPP_PAGE for all page types.
**          dmve_bid_check() unfix page/tbio only if if PARTIAL tcb.
**      14-oct-2009 (stial01)
**          Call dmve_fix_page, dmve_unfix_page
**	29-Oct-2009 (kschendel) SIR 122739
**	    Kill ixklen, add btflags to log record.
**	12-Nov-2009 (kschendel)
**	    Fix goof in unique SI setup when not using TCB.
**	16-Nov-2009 (kschendel) SIR 122890
**	    Don't include dudbms when not needed.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace dm0p_mutex/unmutex with dmveMutex/Unmutex
**	    macros. Also, mutex page before dm1cxclean.
**	    Replace DMPP_PAGE* with DMP_PINFO* as needed.
**	23-Feb-2010 (stial01)
**          dmve_bid_check() pass rcb to dm1cxclean, dont clean unless needed
**      01-apr-2010 (stial01)
**          Changes for Long IDs, move consistency check to dmveutil
**      15-Apr-2010 (stial01)
**          Added diagnostics for dmve_bid_check failures
*/

/*
** Forward Declarations
*/
typedef struct _DMVE_BT_ATTS DMVE_BT_ATTS;

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

static DB_STATUS	dmv_rebtree_put(
				DMVE_CB             *dmve,
				DMP_TABLE_IO	    *tabio,
				DMP_PINFO	    *pinfo,
				DM_TID		    *bid);

static DB_STATUS	dmv_unbtree_put(
				DMVE_CB             *dmve,
				DMP_TABLE_IO	    *tabio,
				DMP_PINFO	    *pinfo,
				DM_TID		    *bid);

static DB_STATUS        dmv_compare_key(
				i4		page_type,
				i4		page_size,
				DMPP_PAGE	*leaf,
				DMVE_CB		*dmve,
				i4		lineno,
				DMP_ROWACCESS	*rac,
				DB_ATTS		**RangeKeys,
				DB_ATTS		**LeafKeyAtts,
				i4		key_count,
				i4		RangeKlen,
				char		*LeafKey,
				i4		*compare);

static DB_STATUS dmv_search(
				i4		page_type,
				i4		page_size,
				DMPP_PAGE	*page,
				DMVE_CB		*dmve,
				DMVE_BT_ATTS	*btatts,
				i4		mode,
				char		*key,
				DM_TID		*tid,
				DM_LINE_IDX	*pos);

static DB_STATUS dmv_binary_search(
				i4		page_type,
				i4		page_size,
				DMPP_PAGE	*page,
				DMVE_CB		*dmve,
				i4		direction,
				char		*key,
				DMP_ROWACCESS	*rac,
				DB_ATTS		**keyatts,
				i4		key_count,
				i4		klen,
				DM_LINE_IDX	*pos);

static DB_STATUS
dmve_init_atts(
				DMVE_CB             *dmve,
				DMP_TABLE_IO        **tbio,
				DMP_PINFO	    **pinfoP,
				u_i4		    log_btflags,
				i4		    log_cmp_type,
				DMVE_BT_ATTS	    *btatts,
				DM_OBJECT           **misc_buffer);

static VOID
dmve_check_atts(
				DMVE_CB             *dmve,
				DMP_TABLE_IO        **tbio, 
				DMP_PINFO	    **pinfoP,
				u_i4		    log_btflags,
				i4		    log_cmp_type);


/*{
** Name: dmve_btput - The recovery of a put key operation.
**
** Description:
**	This function is used to do, redo and undo a put key
**	operation to a btree index/leaf page. This function adds or
**	deletes the key from the index depending on the recovery mode.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The log record of the system catalogs put 
**				    operation.
**	    .dmve_action	    Should be DMVE_DO, DMVE_REDO, or DMVE_UNDO.
**	    .dmve_dcb_ptr	    Pointer to DCB.
**	    .dmve_tran_id	    The physical transaction id.
**	    .dmve_lk_id		    The transaction lock list id.
**	    .dmve_log_id	    The logging system database id.
**	    .dmve_db_lockmode	    The lockmode of the database. Should be 
**				    DM2T_X or DM2T_S.
**
** Outputs:
**	dmve_cb
**	    .dmve_error.err_code    The reason for error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	    E_DB_FATAL			Operation was partially completed,
**					the transaction must be aborted.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-dec-1992 (rogerk)
**	    Written for 6.5 recovery.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Correct the message parameter lengths for E_DM9665.
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	11-sep-1997 (thaju02) bug#85156 - on recovery lock extension
**	    table with DM2T_S.  Attempting to lock table with DM2T_IX
**	    causes deadlock, dbms resource deadlock error during recovery,
**	    pass abort, rcp resource deadlock error during recovery,
**	    rcp marks database inconsistent.
**	06-may-1996 (thaju02 & nanpr01)
**	    New Page Format Support: 
**		Change page header references to use macros.
**		Pass page size to dm1c_getaccessors().
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Do not get page lock if row locking.
**          Check/adjust bid if row locking.
**     12-dec-96 (dilma04)
**          Corrected mistake which caused inconsistency between row_locking 
**          value and DM0L_ROW_LOCK flag after lock escalation.
**      27-feb-97 (stial01)
**          dmve_btput() If row locking entries can shift requiring btundo_check
**          Fix page without locking and then mutex it if we have IX page lock.
**          Error if DO/REDO and only IX page lock.
**          Clean ALL deleted leaf entries if X page lock.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Add lock_id argument to dm0p_lock_page and dm0p_unlock_page.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      29-may-1997 (stial01)
**          dmve_btput: REDO recovery: bid check only if redo necessary
**      18-jun-1997 (stial01)
**          dmve_btput() Request IX page lock if row locking.
**      30-jun-97 (dilma04)
**          Bug 83301. Set DM0P_INTENT_LK fix action if row locking.
**      30-jun-97 (stial01)
**          Use dm0p_lock_buf_macro to lock buffer to reduce mutex duration
**          Don't unfix tabio for bid check, changed args to dmve_bid_check()
**      29-jul-97 (stial01)
**          Validate/adjust bid if page size != DM_COMPAT_PGSIZE
**      06-jan-98 (stial01)
**          If page size 2k, unfix pages,tbio before bid check (B87385) 
**      19-Jun-2002 (horda03) Bug 108074
**          If the table is locked exclusively, then indicate that SCONCUR
**          pages don't need to be flushed immediately.
**      23-feb-2004 (thaju02) Bug 111470 INGSRV2635
**          For rollforwarddb -b option, do not compare the LSN's on the
**          page to that of the log record.
**	01-Dec-2004 (jenjo02)
**	    Tweak to 108074; call dmve_bid_check with table's 
**	    grant_mode rather than tbl_lock_mode to ensure that
**	    bid-check pages WUNFIX bits are also properly reset.
**	13-Apr-2006 (jenjo02)
**	    CLUSTERED Btree leaf pages don't have attributes - check
**	    that before calling dmve_bid_check.
**	13-Jun-2006 (jenjo02)
**	    Puts of leaf entries may be up to
**	    DM1B_MAXLEAFLEN, not DM1B_KEYLENGTH, bytes.
**	01-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameters that will be passed to LKrequest
**	    to avoid random implicit lock conversions.
**	04-Jan-2007 (kibro01) b117125
**	    Swapped round order of checks in the IF condition so that 
**	    a 0-attribute count in a page only drops the page if the 
**	    page is not from a partition
*/
DB_STATUS
dmve_btput(
DMVE_CB		*dmve)
{
    DM0L_BTPUT		*log_rec = (DM0L_BTPUT *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->btp_header.lsn; 
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    DMP_TABLE_IO	*tbio = NULL;
    DMPP_PAGE		*page = NULL;
    DMPP_PAGE		*page_ptr = NULL;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		tmp_status;
    DM_TID		leaf_bid;
    char		*insert_key;
    i4		recovery_action;
    i4		error;
    i4		loc_error;
    i4		page_type = log_rec->btp_pg_type;
    bool		bid_check_done = FALSE;
    bool                bid_check = FALSE;
    bool                index_update;
    i4             opflag;
    DB_ERROR		local_dberr;
    DMP_TCB		*t = NULL;
    DMP_PINFO		*pinfo = NULL;

    CLRDBERR(&dmve->dmve_error);

    if (dmve->dmve_flags & DMVE_MVCC)
	log_rec->btp_bid.tid_tid.tid_page = 
	DM1B_VPT_GET_PAGE_PAGE_MACRO(log_rec->btp_pg_type, dmve->dmve_pinfo->page);

    /*               
    ** Store BID of insert into a local variable.  The insert BID may
    ** be modified in undo recovery by the dmve_btunto_check routine.
    */
    leaf_bid = log_rec->btp_bid;

    /*
    ** Call appropriate recovery action depending on the recovery type
    ** and record flags.  CLR actions are always executed as an UNDO
    ** operation.
    */
    recovery_action = dmve->dmve_action;
    if (log_rec->btp_header.flags & DM0L_CLR)
	recovery_action = DMVE_UNDO;

    for (;;)
    {
	/* Consistency Check:  check for illegal log records */
	if (log_rec->btp_header.type != DM0LBTPUT)
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
	    break;
	}

	/*
	** Check for partial recovery during UNDO operations.
	** We cannot bypass recovery of index updates even if the page
	** is on a location which is not being recovered.  This is because
	** the logged page number may not be the page that actually needs
	** the UNDO action!!!  If the page which was originally updated
	** has been since SPLIT, then the logged key may have moved to a
	** new index/leaf page.  It is that new page to which this undo
	** action must be applied.  And that new page may be on a location
	** being recovered even if the original page is not.
	**
	** If recovery is being bypassed on the entire table then no recovery
	** needs to be done.
	*/
	if ((dmve->dmve_action == DMVE_UNDO) &&
	    (dmve_location_check(dmve, (i4)log_rec->btp_cnf_loc_id) == FALSE))
	{
            uleFormat(NULL, E_DM9668_TABLE_NOT_RECOVERED, (CL_ERR_DESC *)NULL,
                ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                &loc_error, 2, 0, log_rec->btp_tbl_id.db_tab_base,
                0, log_rec->btp_tbl_id.db_tab_index);
	    SETDBERR(&dmve->dmve_error, 0, E_DM9667_NOPARTIAL_RECOVERY);
	    break;
	}

	/*
	** Get handle to a tableio control block with which to read
	** and write pages needed during recovery.
	**
	** Warning return indicates that no tableio control block was
	** built because no recovery is needed on any of the locations 
	** described in this log record.  Note that check above prevents
	** this case from occurring during UNDO recovery.
	*/
	status = dmve_fix_tabio(dmve, &log_rec->btp_tbl_id, &tbio);
	if (DB_FAILURE_MACRO(status))
	    break;
	if ((status == E_DB_WARN) && (dmve->dmve_error.err_code == W_DM9660_DMVE_TABLE_OFFLINE))
	{
	    CLRDBERR(&dmve->dmve_error);
	    return (E_DB_OK);
	}

	/* This might be partial tcb, but it will always have basic info */
	t = (DMP_TCB *)((char *)tbio - CL_OFFSETOF(DMP_TCB, tcb_table_io));

	/*
	** Lock/Fix the page we need to recover in cache for write.
	*/
	status = dmve_fix_page(dmve, tbio, leaf_bid.tid_tid.tid_page,&pinfo);
	if (status != E_DB_OK)
	    break;
	page = pinfo->page;

	/*
	** Dump debug trace info about pages if such tracing is configured.
	*/
	if (DMZ_ASY_MACRO(15))
	    dmve_trace_page_info(log_rec->btp_pg_type, log_rec->btp_page_size,
		(DMPP_PAGE *) page, dmve->dmve_plv, "Page");

	index_update = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page)
			& DMPP_INDEX) != 0);

	if (dmve->dmve_action == DMVE_UNDO)
	{
	    /*
	    ** UNDO Recovery SPLIT check:
	    **
	    ** If doing UNDO recovery, we need to verify that this is the 
	    ** correct page from which to undo the put operation.
	    ** If the row in question has been moved to a different page by
	    ** a subsequent SPLIT operation, then we have to find that new page
	    ** in order to perform the undo.
	    **
	    ** The bid_check will search for the correct BID to which to do
	    ** the recovery action and return with that value in 'leafbid'.
	    */
	   if ((LSN_GT(DM1B_VPT_ADDR_BT_SPLIT_LSN_MACRO(page_type, page),
			log_lsn)) &&
	    (DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page) & DMPP_LEAF))
	    {
		bid_check = TRUE;
	    }

	    /*
	    ** UNDO: if page type != TCB_PG_V1, validate/adjust bid
	    */
	    if ( (page_type != TCB_PG_V1)
		&& (index_update == FALSE) )
	    {
		bid_check = TRUE;
	    }
	}
	else
	{
	    /*
	    ** REDO recovery, the logged page is correct, but
	    ** due to page cleans the logged bid may be incorrect.
	    ** Only do the bid check if we need to REDO this update
	    */
	    if ( (page_type != TCB_PG_V1) 
		&& (index_update == FALSE)
		&&  (LSN_LT(
		    DM1B_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, page), 
		    log_lsn)))
	    {
		bid_check = TRUE;
	    }
	}

	if (bid_check && !bid_check_done)
	{
	    insert_key = &log_rec->btp_vbuf[log_rec->btp_tab_size + 
							log_rec->btp_own_size];
	    if (recovery_action == DMVE_UNDO)
		opflag = DMVE_FINDKEY;
	    else
		opflag = DMVE_FINDPOS;

	    /* Send Table lock's grant mode, not tbl_lock_mode, to bid_check */
	    status = dmve_bid_check(dmve, opflag,
		&log_rec->btp_tbl_id, &log_rec->btp_bid, 
		&log_rec->btp_tid, insert_key, log_rec->btp_key_size,
		&tbio, &leaf_bid, &pinfo);

	    if (status == E_DB_WARN && (dmve->dmve_flags & DMVE_MVCC))
		return (E_DB_OK);

	    if (status != E_DB_OK)
		break;

	    bid_check_done = TRUE;

	    /* Reset local tcb, page pointers */
	    /* This might be partial tcb, but it will always have basic info */
	    t = (DMP_TCB *)((char *)tbio - CL_OFFSETOF(DMP_TCB, tcb_table_io));
            page = pinfo->page;
	}


	/*
	** Compare the LSN on the page with that of the log record
	** to determine what recovery will be needed.
	**
	**   - During Forward processing, if the page's LSN is greater than
	**     the log record then no recovery is needed.
	**
	**   - During Backward processing, it is an error for a page's LSN
	**     to be less than the log record LSN.
	**
	**   - Currently, during rollforward processing it is unexpected
	**     to find that a recovery operation need not be applied because
	**     of the page's LSN.  This is because rollforward must always
	**     begin from a checkpoint that is previous to any journal record
	**     begin applied.  In the future this requirement may change and
	**     Rollforward will use the same expectations as Redo.
	*/
	switch (dmve->dmve_action)
	{
	case DMVE_DO:
	case DMVE_REDO:
            if (LSN_GTE(
		DM1B_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, page), 
		log_lsn) && ((dmve->dmve_flags & DMVE_ROLLDB_BOPT) == 0))
	    {
		if (dmve->dmve_action == DMVE_DO)
		{
		    uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			&loc_error, 8,
			sizeof(*tbio->tbio_relid), tbio->tbio_relid,
			sizeof(*tbio->tbio_relowner), tbio->tbio_relowner,
			0, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, page),
			0, DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page),
			0, DM1B_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, page),
			0, DM1B_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, page),
			0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		}
		page = NULL;
	    }
	    break;

	case DMVE_UNDO:
            if (LSN_LT(
		DM1B_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, page), 
		log_lsn))
	    {
		uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &loc_error, 8,
		    sizeof(*tbio->tbio_relid), tbio->tbio_relid,
		    sizeof(*tbio->tbio_relowner), tbio->tbio_relowner,
		    0, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, page),
		    0, DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page),
		    0, DM1B_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, page),
		    0, DM1B_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, page),
		    0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		SETDBERR(&dmve->dmve_error, 0, E_DM9666_PAGE_LSN_MISMATCH);
		status = E_DB_ERROR;
	    }
	    break;
	}
	if ( status != E_DB_OK || !page )
	    break;

	switch (recovery_action)
	{
	case DMVE_DO:
	case DMVE_REDO:
	    status = dmv_rebtree_put(dmve, tbio, pinfo, &leaf_bid);
	    break;

	case DMVE_UNDO:
	    status = dmv_unbtree_put(dmve, tbio, pinfo, &leaf_bid);
	    break;
	}

	break;
    }

    if (status != E_DB_OK)
    {
        uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    }

    /*
    ** Error handling: this implies that the abort action failed and the
    ** database will be marked inconsistent by the caller.
    */
    if (pinfo)
    {
	tmp_status = dmve_unfix_page(dmve, tbio, pinfo);
	if (tmp_status > status)
	    status = tmp_status;
    }

    if (tbio)
    {
	tmp_status = dmve_unfix_tabio(dmve, &tbio, 0);
	if (tmp_status > status)
	    status = tmp_status;
    }

    if (status != E_DB_OK)
	SETDBERR(&dmve->dmve_error, 0, E_DM964E_DMVE_BTREE_PUT);

    return(status);
}


/*{
** Name: dmv_rebtree_put - Redo the Put of a btree key 
**
** Description:
**      This function adds a key to a btree index for the recovery of a
**	put record operation.
**
** Inputs:
**      dmve				Pointer to dmve control block.
**      tabio				Pointer to table io control block
**      page				Pointer to the page to which to insert
**	log_record			Pointer to the log record
**
** Outputs:
**	error				Pointer to Error return area
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-dec-1992 (rogerk)
**	    Written for 6.5 recovery.
**	18-jan-1992 (rogerk)
**	    Add check in redo routine for case when null page pointer is
**	    passed because redo was found to be not needed.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	06-may-1996 (thaju02 & nanpr01)
**	    New page format support: change page header references to use
**	    macros.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
*/
static DB_STATUS
dmv_rebtree_put(
DMVE_CB             *dmve,
DMP_TABLE_IO	    *tabio,
DMP_PINFO	    *pinfo,
DM_TID		    *bid)
{
    DM0L_BTPUT		*log_rec = (DM0L_BTPUT *)dmve->dmve_log_rec;
    DB_STATUS		status = E_DB_OK;
    DM_LINE_IDX		child;
    DM_TID		temptid;
    i4			temppartno;
    i4			page_type = log_rec->btp_pg_type;
    i4			ix_compressed;
    char		*key;
    bool		index_update;
    i2			TidSize;
    LG_LRI		lri;
    DMPP_PAGE		*page = pinfo->page;

    CLRDBERR(&dmve->dmve_error);

    /*
    ** If there is nothing to recover, just return.
    */
    if (page == NULL)
	return (E_DB_OK);

    key = &log_rec->btp_vbuf[log_rec->btp_tab_size + log_rec->btp_own_size];

    index_update = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page) & 
	DMPP_INDEX) != 0);

    if (index_update)
	child = log_rec->btp_bid_child;
    else
	child = bid->tid_tid.tid_line;

    ix_compressed = DM1CX_UNCOMPRESSED;
    if (log_rec->btp_cmp_type != TCB_C_NONE)
	ix_compressed = DM1CX_COMPRESSED;

    /* Extract the TID size from the page */
    TidSize = DM1B_VPT_GET_BT_TIDSZ_MACRO(page_type, page);
    
    /*
    ** We assume here that there is sufficient space on the page.  If not,
    ** then the dm1cx calls below will catch the error.
    ** The "repeating history" rules of this recorvery should guarantee
    ** that the row will fit since it must have fit originally.  If there
    ** is no room, it will be a fatal recovery error.
    **
    ** We trust that the indicated BID position at which to re-insert the
    ** key is correct.  We have no way to verify this since we don't have
    ** the logical context required to do key comparisons.
    */

    /*
    ** Mutex the page while updating it.
    */
    dmveMutex(dmve, pinfo);

    /*
    ** If insert is to a non-leaf index page, then save the tidp at the
    ** position to which we are about to insert the new key.  This tidp
    ** will be restored following the dm1cxinsert call below.  Index page
    ** updates actually update the tid of the following position, not
    ** position at which the key is put.
    */
    if (index_update)
    {
        dm1cxtget(page_type, log_rec->btp_page_size, page, child, 
			&temptid, &temppartno); 
    }

    /*
    ** Redo the insert operation.
    */
    for (;;)
    {
	/*
	** Reallocate space on the page for the key,tid pair.
	** During REDO we always need to do the allocate (even for CLRs)
	**    When finding insert position, we cleaned all deleted tuples
	**    (Also during REDO delete we reclaim space immediately)
	*/
	status = dm1cxallocate(page_type, log_rec->btp_page_size,
			page, DM1C_DIRECT, ix_compressed,
			&dmve->dmve_tran_id, (i4)0, child,
			log_rec->btp_key_size + TidSize);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E0_BAD_INDEX_ALLOC, (DMP_RCB *)NULL, page, 
			   page_type, log_rec->btp_page_size, child);
	    break;
	}

	/*
	** Reinsert the key,tid values.
	*/
	status = dm1cxput(page_type, log_rec->btp_page_size, page,
			ix_compressed, DM1C_DIRECT, 
			&dmve->dmve_tran_id, LOG_ID_ID(dmve->dmve_log_id),
			(i4)0, child,
			key, log_rec->btp_key_size, &log_rec->btp_tid,
			log_rec->btp_partno);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E4_BAD_INDEX_PUT, (DMP_RCB *)NULL, page,
			   page_type, log_rec->btp_page_size, child);
	    break;
	}

	/*
	** If the insert is to a non-leaf index page, then the logged tid
	** value must actually be inserted to the position after the one
	** to which we just put the key.  Replace the old tidp from that
	** position and insert the new one to the next entry.
	*/
	if (index_update)
	{
	    status = dm1cxtput(page_type, log_rec->btp_page_size,
				page, child + 1, &log_rec->btp_tid,
				log_rec->btp_partno);
	    if (status != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93EB_BAD_INDEX_TPUT, (DMP_RCB *)NULL, page,
			page_type, log_rec->btp_page_size, child + 1);
		break;
	    }
	    status = dm1cxtput(page_type, log_rec->btp_page_size,
				page, child, &temptid, temppartno);
	    if (status != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93EB_BAD_INDEX_TPUT, (DMP_RCB *)NULL, page,
			page_type, log_rec->btp_page_size, child);
		break;
	    }
	}

	break;
    }

    /*
    ** Write the LSN, etc, of the Put log record to the updated page
    */
    DM0L_MAKE_LRI_FROM_LOG_RECORD(&lri, log_rec);
    DM1B_VPT_SET_PAGE_LRI_MACRO(page_type, page, &lri);
    DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, page, DMPP_MODIFY);

    dmveUnMutex(dmve, pinfo);
    
    if (status != E_DB_OK)
    {
	SETDBERR(&dmve->dmve_error, 0, E_DM9650_REDO_BTREE_PUT);
	return(E_DB_ERROR);
    }

    return(E_DB_OK);
}

/*{
** Name: dmv_unbtree_put - UNDO of a put key operation.
**
** Description:
**
** Inputs:
**      dmve				Pointer to dmve control block.
**      tabio				Pointer to table io control block
**      page				Pointer to page on which row was insert
**	log_record			Pointer to the log record
**
** Outputs:
**	error				Pointer to Error return area
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-dec-1992 (rogerk)
**	    Written for 6.5 recovery.
**	18-jan-1992 (rogerk)
**	    Add check in redo/undo routines for case when null page pointer is
**	    passed because recovery was found to be not needed.
**	15-mar-1993 (jnash)
**	    Check dmve->dmve_logging to determine if logging required.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	21-jun-1993 (rogerk)
**	    Removed copy of key from btput CLR records.  Changed consistency
**	    checks to not try to verify key contents if the key was not logged.
**	    Added new error messages.
**	15-apr-1994 (chiku)
**	    Bug56702: returned logfull indication.
**	23-may-1994 (jnash)
** 	    Consistency check was checking wrong tid during index
**	    update undo.
**	06-may-1996 (thaju02 & nanpr01)
**	    New page format support: change page header references to use
**	    macros.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          When calling dm1cxdel(), pass reclaim_space param
**      04-feb-97 (stial01)
**          Tuple headers are on LEAF and overflow (CHAIN) pages
**          Tuple headers are not on INDEX pages.
**      27-feb-97 (stial01)
**          dmv_unbtree_put() Space is not reclaimed if IX page lock held 
**          or deleting the last entry on leaf page with overflow chain.
**          Log key in CLR, needed for row locking
**          Init flag param for dm1cxdel()
**      07-apr-97 (stial01)
**          dmv_unbtree_put() NonUnique primary btree (V2) dups span leaf pages,
**          not overflow chain. Remove code skipping reclaim of overflow key
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      21-apr-1998 (stial01)
**          dmv_unbtree_put() Removed setting unecessary DM1CX_K_ROW_LOCKING.
**	13-Jun-2006 (jenjo02)
**	    Clustered TIDs may mismatch, and that's OK as long as the
**	    keys match.
*/
static DB_STATUS
dmv_unbtree_put(
DMVE_CB             *dmve,
DMP_TABLE_IO	    *tabio,
DMP_PINFO	    *pinfo,
DM_TID		    *bid)
{
    DM0L_BTPUT		*log_rec = (DM0L_BTPUT *)dmve->dmve_log_rec;
    DB_STATUS		status = E_DB_OK;
    DM_LINE_IDX		child;
    DM_TID		deltid;
    i4			delpartno;
    LG_LSN		lsn;
    char		*key;
    char		*key_ptr;
    i4		key_len;
    i4		flags;
    i4		loc_id;
    i4		loc_config_id;
    i4		page_type = log_rec->btp_pg_type;
    i4		ix_compressed;
    bool	index_update;
    bool        is_leaf;
    bool        Clustered;
    bool	mutex_done;
    i4		reclaim_space;
    i4		dmcx_flag = 0;
    i4		update_mode;
    i4		local_err;
    i4		*err_code = &dmve->dmve_error.err_code;
    LG_LRI		lri;
    DMPP_PAGE		*page = pinfo->page;

    CLRDBERR(&dmve->dmve_error);

    /*
    ** If there is nothing to recover, just return.
    */
    if (page == NULL)
	return (E_DB_OK);

    key = &log_rec->btp_vbuf[log_rec->btp_tab_size + log_rec->btp_own_size];

    index_update = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page) & 
	DMPP_INDEX) != 0);
    is_leaf = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page) &
	DMPP_LEAF) != 0);
    Clustered = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page) &
	DMPP_CLUSTERED) != 0);

    if (index_update)
	child = log_rec->btp_bid_child;
    else
	child = bid->tid_tid.tid_line;

    ix_compressed = DM1CX_UNCOMPRESSED;
    if (log_rec->btp_cmp_type != TCB_C_NONE)
	ix_compressed = DM1CX_COMPRESSED;

    /*
    ** Get information on the location to which the update is being made.
    */
    loc_id = DM2F_LOCID_MACRO(tabio->tbio_loc_count, 
	(i4) DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, page));
    loc_config_id = tabio->tbio_location_array[loc_id].loc_config_id;

    /*
    ** Lookup the key/tid pair to delete.
    */
    dm1cxrecptr(page_type, log_rec->btp_page_size, page, child, &key_ptr);
    dm1cxtget(page_type, log_rec->btp_page_size, page, child, 
		&deltid, &delpartno);

    /* 
    ** For consistency check, check the deleted tid at the new position.
    */
    if (index_update)
	dm1cxtget(page_type, log_rec->btp_page_size, page, 
		child + 1, &deltid, &delpartno);

    /*
    ** Consistency Checks:
    **
    ** Verify the deltid value. 
    ** Verify the that the key matches the one we are about to delete.
    **    As of release OpenIngres 2.0, we log the key value in CLRs as well,
    **    since we need it if row locking.
    ** 
    ** If this is a non-leaf page update, verify that there is an entry at
    ** the following position in which to update the TID value.
    ** XXXX TEST ME XXXX
    */

    /*
    ** We can only validate the key size on compressed tables; otherwise
    ** we must assume that the logged value was the correct table key length.
    */
    key_len = log_rec->btp_key_size;
    if (ix_compressed != DM1CX_UNCOMPRESSED)
    {
	dm1cx_klen(page_type, log_rec->btp_page_size, page, 
		child, &key_len);
    }

    /*
    ** Compare the key,tid pair we are about to delete with the one we logged
    ** to make sure they are identical.
    **
    ** If the keys don't match then we make an assumption here that the 
    ** mismatch is most likely due to this check being wrong (we have 
    ** garbage at the end of the tuple buffer or we allowed some sort of 
    ** non-logged update to the row) and we continue with the operation
    ** after logging the unexpected condition.
    */
    if (((log_rec->btp_key_size != 0) && 
	    ((log_rec->btp_key_size != key_len) ||
	     (MEcmp((PTR)key, (PTR)key_ptr, key_len) != 0))) ||
	(log_rec->btp_tid.tid_i4 != deltid.tid_i4 && !Clustered))
    {
	uleFormat(NULL, E_DM966A_DMVE_KEY_MISMATCH, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 8, 
	    sizeof(DB_DB_NAME), tabio->tbio_dbname->db_db_name,
	    log_rec->btp_tab_size, &log_rec->btp_vbuf[0],
	    log_rec->btp_own_size, &log_rec->btp_vbuf[log_rec->btp_tab_size],
	    0, bid->tid_tid.tid_page, 0, bid->tid_tid.tid_line,
	    5, (index_update ? "INDEX" : "LEAF "),
	    0, log_rec->btp_bid.tid_tid.tid_page,
	    0, log_rec->btp_bid.tid_tid.tid_line);
	uleFormat(NULL, E_DM966B_DMVE_KEY_MISMATCH, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 7, 
	    0, key_len, 0, log_rec->btp_key_size,
	    0, deltid.tid_tid.tid_page, 0, deltid.tid_tid.tid_line,
	    0, log_rec->btp_tid.tid_tid.tid_page,
	    0, log_rec->btp_tid.tid_tid.tid_line,
	    0, dmve->dmve_action);
	dmd_log(1, (PTR) log_rec, 4096);
	uleFormat(NULL, E_DM964F_UNDO_BTREE_PUT, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0); 
    }

    /* 
    ** Mutex the page.  This must be done prior to the log write.
    */
    dmveMutex(dmve, pinfo);

    /*
    ** During UNDO (rollback) don't reclaim space if we have IX lock on page
    ** We may need the space/bid if this transaction deleted/inserted the same
    ** key.
    */ 
    update_mode = DM1C_DIRECT;
    if (!index_update && page_type != TCB_PG_V1 &&
	((dmve->dmve_lk_type == LK_ROW || dmve->dmve_lk_type == LK_CROW) || 
	(dmve->dmve_lk_type == LK_PAGE && (log_rec->btp_header.flags & DM0L_PHYS_LOCK))))
    {
	reclaim_space = FALSE;
	update_mode |= DM1C_ROWLK;
    }
    else
	reclaim_space = TRUE;

    if (reclaim_space == TRUE)
	dmcx_flag |= DM1CX_RECLAIM;

    /*
    ** Check direction of recovery operation:
    **
    **     If this is a normal Undo, then we log the CLR for the operation
    **     and write the LSN of this CLR onto the newly updated page (unless
    **     dmve_logging is turned off - in which case the rollback is not
    **     logged and the page lsn is unchanged).
    **
    **     As of release OpenIngres 2.0, we log the key value in CLRs as well,
    **     since we need it if row locking.
    **
    **     If the record being processed is itself a CLR, then we are REDOing
    **     an update made during rollback processing.  Updates are not relogged
    **     in redo processing and the LSN is moved forward to the LSN value of
    **     of the original update.
    **
    ** The CLR for a BTPUT need not contain the key, just the bid.
    */
    if ((log_rec->btp_header.flags & DM0L_CLR) == 0)
    {
	if (dmve->dmve_logging)
	{
	    flags = (log_rec->btp_header.flags | DM0L_CLR);

	    /* Extract previous page change info */
	    DM1B_VPT_GET_PAGE_LRI_MACRO(log_rec->btp_pg_type, page, &lri);

	    status = dm0l_btput(dmve->dmve_log_id, flags,
		&log_rec->btp_tbl_id, 
		(DB_TAB_NAME*)&log_rec->btp_vbuf[0], log_rec->btp_tab_size, 
		(DB_OWN_NAME*)&log_rec->btp_vbuf[log_rec->btp_tab_size], log_rec->btp_own_size, 
		log_rec->btp_pg_type, log_rec->btp_page_size,
		log_rec->btp_cmp_type, 
		log_rec->btp_loc_cnt, loc_config_id,
		bid, child, &log_rec->btp_tid, log_rec->btp_key_size, key,
		&log_rec->btp_header.lsn, &lri, log_rec->btp_partno, 
		log_rec->btp_btflags, &dmve->dmve_error);
	    if (status != E_DB_OK)
	    {
		dmveUnMutex(dmve, pinfo);
		/*
		 * Bug56702: returned logfull indication.
		 */
		dmve->dmve_logfull = dmve->dmve_error.err_code;

		uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0); 
		SETDBERR(&dmve->dmve_error, 0, E_DM964F_UNDO_BTREE_PUT);
		return(E_DB_ERROR);
	    }
	}
    }
    else
    {
	/*
	** If we are processing recovery of an Insert CLR (redo-ing the undo
	** of an insert) then we don't log a CLR but instead save the LSN
	** of the log record we are processing with which to update the
	** page lsn's.
	*/
	DM0L_MAKE_LRI_FROM_LOG_RECORD(&lri, log_rec);
    }

    /* 
    ** Write the LSN, etc, of the delete record onto the page, unless nologging
    */
    if (dmve->dmve_logging)
	DM1B_VPT_SET_PAGE_LRI_MACRO(page_type, page, &lri);

    /*
    ** Undo the insert operation.
    */
    for (;;)
    {
	/*
	** If the insert was to a non-leaf index page, then we must actually
	** delete the Key at the logged position, but the tid of the following
	** entry.  We take the tid at the position we are about to delete
	** and write it into the next entry.
	*/
	if (index_update)
	{
	    dm1cxtget(page_type, log_rec->btp_page_size,
		page, child, &deltid, &delpartno);
	    status = dm1cxtput(page_type, log_rec->btp_page_size, 
		page, child + 1, &deltid, delpartno);
	    if (status != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93EB_BAD_INDEX_TPUT, (DMP_RCB *)NULL, page, 
			page_type, log_rec->btp_page_size, child + 1);
		break;
	    }
	}

	status = dm1cxdel(page_type, log_rec->btp_page_size, page,
			update_mode, ix_compressed,
			&dmve->dmve_tran_id, LOG_ID_ID(dmve->dmve_log_id),
			(i4)0, dmcx_flag, child);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E2_BAD_INDEX_DEL, (DMP_RCB *)NULL, page, 
			   page_type, log_rec->btp_page_size, child);
	    break;
	}

	break;
    }

    DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, page, DMPP_MODIFY);
    dmveUnMutex(dmve, pinfo);
    
    if (status != E_DB_OK)
    {
	SETDBERR(&dmve->dmve_error, 0, E_DM964F_UNDO_BTREE_PUT);
	return(E_DB_ERROR);
    }

    return(E_DB_OK);
}

/*{
** Name: dmve_bid_check - Check that correct page is fixed for Btree Recovery.
**
** Description:
**
** Inputs:
**      dmve				Pointer to dmve control block.
**	opflags
**	tableid				Table Id.
**	log_bid				Original Bid of key:
**					  - if put undo, bid of key to delete
**					  - if del undo, bid at which to insert
**					  - if free undo, bid of previous page
**	log_tid				Tid of data page row
**	log_key				Key value
**	log_key_size			Key length
**
** Outputs:
**	found_bid			May be updated to show new bid for key:
**					  - if put undo, bid of key to delete
**					  - if del undo, bid at which to insert
**					  - if free undo, bid of previous page
**	error
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-dec-1992 (rogerk)
**	    Written for 6.5 recovery.
**	18-jan-1993 (rogerk)
**	    Fix argument to ule_format being passed by value, not ref.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	21-jun-1993 (rogerk)
**	    Changed undo delete case to pass dummy tid pointer to dm1bxsearch
**	    when calling to find spot at which to restore the deleted key.
**	    We can't pass the "log_tid" argument pointer because its value
**	    is overwritten by the search routine.
**	    Add error messages.
**	26-jul-1993 (rogerk)
**	    Added tracing of btundo_check logical processing.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass page_size to dm1cxhas_room.
**	06-may-1996 (thaju02 & nanpr01)
**	    New page format support: change page header references to use
**	    macros.
**      03-june-1996 (stial01)
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**      22-jul-1996 (ramra01 for bryanp)
**          Pass row_version to dmppxuncompress.
**	16-sep-1996 (canor01)
**	    Pass tuple buffer to dmppxuncompress.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Do not get page lock if row locking.
**      27-feb-97 (stial01)
**          Renamed dmve_btundo_check -> dmve_bid check, as it is now called
**          for UNDO and REDO. Changed this routine for REDO. 
**          Clean ALL deleted leaf entries if X page lock.
**      07-apr-97 (stial01)
**          dmve_bid_check() NonUnique primary btree (V2), dups can span leaf
**      21-apr-97 (stial01)
**          dmve_bid_check: Search mode is DM1C_FIND for unique keys, DM1C_TID 
**          for non-unique keys, since we re-use unique entries in 
**          dm1b_allocate, the tidp part of the leaf entry may not match.
**      30-apr-97 (stial01)
**          dmve_bid_check: When searching for key, don't overwrite logged tidp
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**          Non unique index: if our key matches RRANGE, check next leaf
**          (This routine used to do a dm1b_search (DM1C_TID) which would have
**          brought us to the correct leaf.)
**      30-jun-97 (dilma04)
**          Bug 83301. Set DM0P_INTENT_LK fix action if row locking.
**      30-jun-97 (stial01)
**          dmve_bid_check() Use attr/key info on leaf pages
**          Use dm0p_lock_buf_macro to lock buffer to reduce mutex duration
**      22-aug-97 (stial01)
**          dmve_bid_check() if dmv_search returns err_code == E_DM0056_NOPART,
**          we should check next page if duplicate keys and the log key
**          matches rrange key.
**      06-jan-98 (stial01)
**          dmve_bid_check() LOCK pages we fix, unfix AND UNLOCK if LK_PHYSICAL
**      09-feb-99 (stial01)
**          dmve_bid_check() Pass relcmptlvl = 0 to dm1cxkperpage
**      19-Jun-2002 (horda03) Bug 108074
**          If the table is locked exclusively, then indicate that SCONCUR
**          pages don't need to be flushed immediately. We know when that the
**          table is locked exclusively if the pg_lock_mode = LK_N.
**	18-Aug-2003 (devjo01)
**	    In crossing HORDA's bug 108074 change, needed to modify it
**	    to remove pg_lock_mode reference, since this is no longer a
**	    parameter.  Simply check for either exclusive table or 
**	    exclusive database access as "pg_lock_mode == LK_N" equivalence.
**	13-dec-04 (inkdo01)
**	    Support union in DM1B_ATTR.
**	13-Apr-2006 (jenjo02)
**	    Range attributes in the leaf can no longer be assumed
**	    to be leafatts; they may be keys only.
**	13-Jun-2006 (jenjo02)
**	    Using DM1B_MAXSTACKKEY instead of DM1B_KEYLENGTH, call dm1b_AllocKeyBuf() 
**	    to validate/allocate Leaf work space. DM1B_KEYLENGTH is now only the 
**	    max length of key attributes, not necessarily the length of the Leaf entry.
**	19-Dec-06 (kibro01) b117125
**	    In a partitioned table where we opened the master table to check
**	    attributes, the page we passed in (from the partition) is still
**	    valid, so leave it alone (don't unfix it at this stage).
**	13-Jul-2009 (thaju02) (B122114)
**	    Rollback of delete fails with E_DM9665 on compressed secondary
**	    index. klen is the sum of key lens. attlen is sum of att lens.
**	    RangeKlen is either klen or attlen depending on if range entries
**	    include nonkey cols.  If attrs length is not equal to klen then 
**	    non-key cols are included in range entries (RangeKlen).
**	25-Aug-2009 (kschendel) b121804
**	    Call dmppxuncompress properly!  Sort of.  The real fix will involve
**	    passing an ADF-CB around throughout dmvebtpt.  Fortunately, the
**	    bad "buffer" that was being passed didn't matter, since only
**	    hidata-uncompress uses it, and you can't hidata-compress
**	    btree keys (just data).
*/
DB_STATUS
dmve_bid_check(
DMVE_CB             *dmve,
i4             	    opflag,
DB_TAB_ID	    *tableid,
DM_TID		    *log_bid,
DM_TID		    *log_tid,
char		    *log_key,
i4		    log_key_size,
DMP_TABLE_IO        **tbio_ptr,
DM_TID		    *found_bid,
DMP_PINFO	    **pinfoP)
{
    DMVE_BT_ATTS	btatts;
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    DM0L_HEADER		*log_hdr = (DM0L_HEADER *)dmve->dmve_log_rec;
    DMP_TCB		*t;
    DM_PAGENO		pageno;
    DM_LINE_IDX		pos;
    DM_TID		leaf_bid;
    DM_TID		temp_tid;
    char		*logkey_ptr;
    bool                init_done = FALSE;
    i4			uncompress_size;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS           local_status;
    i4             	local_error;
    bool                have_leaf;
    i4                  lcompare;
    i4                  rcompare;
    i4             	mode;
    DM_TID              temp_log_tid;
    char		*KeyBuf, *AllocKbuf = NULL;
    char                keybuf[DM1B_MAXSTACKKEY];
    DM_OBJECT           *misc_buffer = NULL;
    i4                  log_pgtype;
    i4             	log_pgsize;
    i4             	log_cmp_type;
    u_i4		log_btflags;
    i4			ix_compressed;
    char                *str;
    i4             	recovery_action;
    DB_TAB_NAME		table_name;
    DB_OWN_NAME		table_owner;
    i4			update_mode;
    i4			*err_code = &dmve->dmve_error.err_code;
    DB_ERROR		local_dberr;
    bool                bt_unique;
    bool		pgtype_has_ovfl_chain;
    DMP_PINFO		*pinfo = *pinfoP;
    DMPP_PAGE		**page = &pinfo->page;
    DMP_RCB		*rcb;

    CLRDBERR(&dmve->dmve_error);

    switch (log_hdr->type)
    {
	case DM0LBTPUT:
	    log_cmp_type = ((DM0L_BTPUT *)log_hdr)->btp_cmp_type;
	    log_btflags = ((DM0L_BTPUT *)log_hdr)->btp_btflags;
	    break;
	case DM0LBTDEL:
	    log_cmp_type = ((DM0L_BTDEL *)log_hdr)->btd_cmp_type;
	    log_btflags = ((DM0L_BTDEL *)log_hdr)->btd_btflags;
	    break;
	case DM0LBTFREE:
	    log_cmp_type = ((DM0L_BTFREE *)log_hdr)->btf_cmp_type;
	    log_btflags = ((DM0L_BTFREE *)log_hdr)->btf_btflags;
	    break;
    }
    ix_compressed = DM1CX_UNCOMPRESSED;
    if (log_cmp_type != TCB_C_NONE)
	ix_compressed = DM1CX_COMPRESSED;

    bt_unique = (log_btflags & DM0L_BT_UNIQUE) != 0;
    pgtype_has_ovfl_chain = (log_btflags & DM0L_BT_DUPS_ON_OVFL) != 0;

    if (log_hdr->flags & DM0L_CLR)
        recovery_action = DMVE_UNDO;
    else
	recovery_action = dmve->dmve_action;

    /* Look at the TCB to see if it already has the att info */
    t = (DMP_TCB *)((char *)*tbio_ptr - CL_OFFSETOF(DMP_TCB, tcb_table_io));
    log_pgtype = t->tcb_rel.relpgtype; /* always init in TCB */
    log_pgsize = t->tcb_rel.relpgsize; /* always init in TCB */
    STRUCT_ASSIGN_MACRO(t->tcb_rel.relid, table_name);
    STRUCT_ASSIGN_MACRO(t->tcb_rel.relowner, table_owner);
    if (dmve->dmve_flags & DMVE_MVCC)
	rcb = dmve->dmve_rcb;
    else
	rcb = NULL;

    if ((t->tcb_status & TCB_PARTIAL) == 0)
    {
	btatts.bt_keys = t->tcb_keys;
	btatts.bt_klen = t->tcb_klen;
	btatts.bt_leafkeys = t->tcb_leafkeys;
	btatts.bt_kperleaf = t->tcb_kperleaf;
	btatts.bt_rngkeys = t->tcb_rngkeys;
	btatts.bt_rngklen = t->tcb_rngklen;
	STRUCT_ASSIGN_MACRO(t->tcb_leaf_rac, btatts.bt_leaf_rac);
	STRUCT_ASSIGN_MACRO(*t->tcb_rng_rac, btatts.bt_rng_rac);

	if (DMZ_ASY_MACRO(6))
	    dmve_check_atts(dmve, tbio_ptr, &pinfo, log_btflags, log_cmp_type);
    }
    else
    {
	status = dmve_init_atts(dmve, tbio_ptr, &pinfo, log_btflags, 
		log_cmp_type, &btatts, &misc_buffer);
	if (status)
	    return (status);

	/* reset tcb ptr, since tbio/tcb may have been unfixed/fixed */
	/* This might be partial tcb, but it will always have basic info */
	t = (DMP_TCB *)((char *)*tbio_ptr - CL_OFFSETOF(DMP_TCB, tcb_table_io));
    }


    pageno = DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *page);

    for (;;)
    {
        /*
        ** Determine if the current page is still the correct page
        ** If the logged key is compressed, then we need to uncompress it to
        ** be able to do key comparisons below.
        */
        logkey_ptr = log_key;
        if (log_cmp_type != TCB_C_NONE)
        {
	    if ( status = dm1b_AllocKeyBuf(btatts.bt_rngklen, keybuf, &KeyBuf, 
						&AllocKbuf, &dmve->dmve_error) )
	    {
		break;
	    }
            status = (*btatts.bt_leaf_rac.dmpp_uncompress)(&btatts.bt_leaf_rac,
		    logkey_ptr, KeyBuf, btatts.bt_rngklen,
		    &uncompress_size, &KeyBuf, 0, &dmve->dmve_adf_cb);
            if (status != E_DB_OK)
                break;
 
            logkey_ptr = KeyBuf;
        }

	/*
	** For unique indexes, if you are row locking and delete and insert
	** the same key, the insert will re-use the deleted leaf entry.
	** So for unique keys the search mode is DM1C_FIND, as the tidp
	** portion of the leaf may not match.
	*/
	if (bt_unique)
	    mode = DM1C_FIND;
	else
	    mode = DM1C_TID;

        break;
    }

    if (status != E_DB_OK)
    {
	uleFormat(NULL, E_DM968D_DMVE_BTREE_INIT, (CL_ERR_DESC *)NULL, ULE_LOG,
	    NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 3,
	    sizeof(DB_DB_NAME), &dcb->dcb_name,
	    sizeof(DB_TAB_NAME), table_name.db_tab_name,
	    sizeof(DB_OWN_NAME), table_owner.db_own_name);
    }
    else
    {
	init_done = TRUE;
    }


    /*
    ** Start with logged page, and if necessary follow forward leaf pointers
    **
    ** If looking for a key and logged page is an overflow page, we should
    ** always find the key on that overflow page, since overflow pages don't
    ** split
    */
    for ( ;status == E_DB_OK;)
    {
	if (*page && DM1B_VPT_GET_PAGE_PAGE_MACRO(log_pgtype, *page) != pageno)
	{
	    status = dmve_unfix_page(dmve, *tbio_ptr, pinfo);
	    if (status)
		break;
	}

	if (*page == NULL)
	{
	    status = dmve_fix_page(dmve, *tbio_ptr, pageno, pinfoP);
	    if (status)
		break;
	}

	/*
	** Compare the record's key with the leaf's high and low range keys.
	*/
	status = dmv_compare_key(log_pgtype, log_pgsize, *page, dmve,
		DM1B_LRANGE, &btatts.bt_rng_rac,
		btatts.bt_rngkeys, btatts.bt_leafkeys, btatts.bt_keys,
		btatts.bt_rngklen, logkey_ptr,
		&lcompare);

	if (status != E_DB_OK)
	    break;

	status = dmv_compare_key(log_pgtype, log_pgsize, *page, dmve,
		DM1B_RRANGE, &btatts.bt_rng_rac,
		btatts.bt_rngkeys, btatts.bt_leafkeys, btatts.bt_keys,
		btatts.bt_rngklen, logkey_ptr,
		&rcompare);

	if (status != E_DB_OK)
	    break;

	if (lcompare < DM1B_SAME && (dmve->dmve_flags & DMVE_MVCC) == 0)
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM0055_NONEXT);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Compare the record's key with the leaf's high and low range keys.
	** If it falls in between then we already have the correct leaf fixed.
	** Note, a key inserted onto an overflow page can't move
	*/
	if (bt_unique || (tableid->db_tab_index > 0) || 
	    (log_pgtype == TCB_PG_V1) || pgtype_has_ovfl_chain)
	{
	    if ((rcompare > DM1B_SAME) || (lcompare <= DM1B_SAME))
		have_leaf = FALSE;
	    else
		have_leaf = TRUE;
	}
	else
	{
	    /*
	    ** Non unique primary btree (V2), dups span leaf pages
	    */
	    if ((rcompare > DM1B_SAME) || (lcompare < DM1B_SAME))
		have_leaf = FALSE;
	    else
		have_leaf = TRUE;
	}

	if (DMZ_ASY_MACRO(6)) /* DM1306 */
	{
	    dmd_print_key(logkey_ptr, btatts.bt_leafkeys, 0,
		 btatts.bt_keys, &dmve->dmve_adf_cb);
	    dmd_print_brange(log_pgtype, log_pgsize,
		&btatts.bt_rng_rac, btatts.bt_rngklen, 
		btatts.bt_keys, &dmve->dmve_adf_cb, *page);
	}

	if (have_leaf == FALSE)
	{
	    /* MVCC undo never allocates misc_buffer, just return */
	    if (dmve->dmve_flags & DMVE_MVCC)
		return (E_DB_WARN);

	    pageno = DM1B_VPT_GET_BT_NEXTPAGE_MACRO(log_pgtype, *page);
	    if (pageno == 0)
	    {
		SETDBERR(&dmve->dmve_error, 0, E_DM0055_NONEXT);
		status = E_DB_ERROR;
		break;
	    }
	    else
	    {
		continue;
	    }
	}

	if (DMZ_ASY_MACRO(6))
	    TRdisplay("        Key falls within range of logged page.\n");

	if (dmve->dmve_lk_type == LK_TABLE ||
		(dmve->dmve_lk_type == LK_PAGE
		&& log_hdr->flags & DM0L_PHYS_LOCK) == 0)
	    update_mode = DM1C_DIRECT;
	else
	    update_mode = DM1C_DIRECT | DM1C_ROWLK;

	/*
	**  Bid check prior to redo insert
	**  Clean committed deleted tuples from this leaf page
	**
	**  MVCC: crow_locking may do GET/SAVE_POSITION/DELETE on root-leaf
	**  (its possible to use the root page if it is consistent!)
	**  GET-NEXT may find the RootPageIsInconsistent and create a CRleaf.
	**  MVCC undo on CRleaf uses lock mode table because it owns the CRleaf
	**  But we shouldn't clean a key a mvcc cursor is positioned on.
	**  There is code in dm1cxclean for this, but another way to 
	**  avoid reposition errors is to defer the page clean until it 
	**  dm1cxhas_room returns FALSE.
	*/
	if (opflag == DMVE_FINDPOS &&
	   log_pgtype != TCB_PG_V1 &&
	   (DM1B_VPT_GET_PAGE_STAT_MACRO(log_pgtype, *page) & DMPP_CLEAN) &&
	    dm1cxhas_room(log_pgtype, log_pgsize, *page, ix_compressed, 
		(i4)100, btatts.bt_kperleaf, log_key_size + 
		DM1B_VPT_GET_BT_TIDSZ_MACRO(log_pgtype, *page)) == FALSE)
	{
	    dmveMutex(dmve, pinfo);

	    status = dm1cxclean(log_pgtype, log_pgsize, *page,
		    ix_compressed, update_mode, rcb);

	    dmveUnMutex(dmve, pinfo);

	    if (status != E_DB_OK)
		break;
	}

	if (opflag == DMVE_FINDLEAF)
	{
	    /*
	    ** Since the range keys matched, we have the correct page.
	    */
	    leaf_bid.tid_tid.tid_page = 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(log_pgtype, *page);
	    leaf_bid.tid_tid.tid_line = 0;
	    break;
	}

	if (opflag == DMVE_FINDKEY)
	{
	    /*
	    ** For unique indexes, if you are row locking and delete and insert
	    ** the same key, the insert will re-use the deleted leaf entry.
	    ** So for unique keys the search mode is DM1C_FIND, as the tidp
	    ** portion of the leaf may not match. In this case be careful
	    ** not to overwrite the logged tidp with the tidp in the leaf entry
	    */
	    temp_log_tid.tid_i4 = log_tid->tid_i4;
	    status = dmv_search(log_pgtype, log_pgsize, *page, dmve, &btatts,
		mode, logkey_ptr, &temp_log_tid, &pos);
	    if (status == E_DB_OK)
	    {
		/*
		** Found logged key: Set BID to point to the entry just found
		*/
		leaf_bid.tid_tid.tid_page = 
			DM1B_VPT_GET_PAGE_PAGE_MACRO(log_pgtype, *page);
		leaf_bid.tid_tid.tid_line = pos;
		break;
	    }
	    else if (dmve->dmve_error.err_code == E_DM0056_NOPART )
	    {
		if (bt_unique)
		{
		    break;
		}

		/*
		** Non unique btree:
		** If our key matches RRANGE, we should check the next leaf
		*/
		if (rcompare != DM1B_SAME)
		{
		    SETDBERR(&dmve->dmve_error, 0, E_DM0055_NONEXT);
		    status = E_DB_ERROR;
		    break;
		}

		status = E_DB_OK;
		CLRDBERR(&dmve->dmve_error);
		pageno = DM1B_VPT_GET_BT_NEXTPAGE_MACRO(log_pgtype, *page);
		continue;
	    }
	    else
	    {
		/* A serious error */
		break;
	    }
	}

	if (opflag == DMVE_FINDPOS)
	{
	    /*
	    ** Check for available space on the logged leaf page.
	    ** Finding the correct page but finding that it does not
	    ** have sufficient space for the insert is an unexpected
	    ** problem and will cause this recovery action to fail.
	    */
	    if (dm1cxhas_room(log_pgtype, log_pgsize, *page, ix_compressed, 
		    (i4)100, btatts.bt_kperleaf, log_key_size + 
			DM1B_VPT_GET_BT_TIDSZ_MACRO(log_pgtype, *page))
				== FALSE)
	    {
		if (bt_unique || (dmve->dmve_flags & DMVE_MVCC))
		{
		    uleFormat(NULL, E_DM966F_DMVE_BTOVFL_NOROOM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			err_code, 9,
			sizeof(DB_DB_NAME), &dcb->dcb_name,
			sizeof(DB_TAB_NAME), table_name.db_tab_name,
			sizeof(DB_OWN_NAME), table_owner.db_own_name,
			0, DM1B_VPT_GET_PAGE_PAGE_MACRO(log_pgtype, *page),
			0, DM1B_VPT_GET_SPLIT_LSN_HIGH_MACRO(log_pgtype, *page),
			0, DM1B_VPT_GET_SPLIT_LSN_LOW_MACRO(log_pgtype, *page),
			0, DM1B_VPT_GET_BT_KIDS_MACRO(log_pgtype, *page),
			0, btatts.bt_kperleaf, 0, log_key_size +
			    DM1B_VPT_GET_BT_TIDSZ_MACRO(log_pgtype, *page));
		    SETDBERR(&dmve->dmve_error, 0, E_DM0055_NONEXT);
		    status = E_DB_ERROR;
		    break;
		}

		/*
		** Non unique btree:
		** If our key matches RRANGE, we should check the next leaf
		*/
		if (rcompare != DM1B_SAME)
		{
		    SETDBERR(&dmve->dmve_error, 0, E_DM0055_NONEXT);
		    status = E_DB_ERROR;
		    break;
		}

		pageno = DM1B_VPT_GET_BT_NEXTPAGE_MACRO(log_pgtype, *page);
		continue;
	    }
	    else
	    {
		/*
		** Leaf page has room for the key to be re-inserted.  
		** Search the page to find the proper position at which
		** to place it.
		**
		** This call may return NOPART if the leaf is empty.
		** In that case 'pos' will have been correctly set to 
		** the 1st position.
		*/
		status = dmv_search(log_pgtype, log_pgsize, *page, dmve,
		    &btatts,
	            DM1C_OPTIM, logkey_ptr, &temp_tid, &pos);
		if ((status != E_DB_OK) && (dmve->dmve_error.err_code == E_DM0056_NOPART))
		{
		    CLRDBERR(&dmve->dmve_error);
		    status = E_DB_OK;
		}

		if (status == E_DB_OK)
		{
		    leaf_bid.tid_tid.tid_page = 
			DM1B_VPT_GET_PAGE_PAGE_MACRO(log_pgtype, *page);
		    leaf_bid.tid_tid.tid_line = pos;
		    break;
		}
	    }
	}
    }

    if (status != E_DB_OK)
    {
	i4		lgid = 0;
	i4		i;
	DMP_POS_INFO    *pos;
	char		*prevrec;

        if (dmve->dmve_flags & DMVE_MVCC)
	{
	    lgid = rcb->rcb_crib_ptr->crib_lgid_low;
	    pos = &rcb->rcb_pos_info[RCB_P_GET];
	    if (DM1B_POSITION_VALID_MACRO(rcb, RCB_P_GET))
	    {
		/* Print key we are positioned on since it can't be cleaned */
		TRdisplay("GETPOS leaf %d,%d TID %d,%d lsn %x %x cc %d stat %x next %d\n",
		    pos->bid.tid_tid.tid_page, pos->bid.tid_tid.tid_line,
		    pos->tidp.tid_tid.tid_page, pos->tidp.tid_tid.tid_line,
		    pos->lsn.lsn_low, pos->lsn.lsn_high,
		    pos->clean_count, pos->page_stat, pos->nextleaf);
		TRdisplay("RCB lowtid %d,%d\n", 
		    rcb->rcb_lowtid.tid_tid.tid_page,
		    rcb->rcb_lowtid.tid_tid.tid_line);
		dmd_print_key(rcb->rcb_repos_key_ptr, btatts.bt_leafkeys, 0,
		     btatts.bt_keys, &dmve->dmve_adf_cb);
	    }
	}
	TRdisplay("BTREE undo %d %x error for tran %x %x lgid %d\n", 
	    log_hdr->type, log_hdr->flags,
	    dmve->dmve_tran_id.db_low_tran, dmve->dmve_tran_id.db_high_tran,
	    lgid);
	dmd_print_key(logkey_ptr, btatts.bt_leafkeys, 0,
	     btatts.bt_keys, &dmve->dmve_adf_cb);
	if (*page)
	{
	    TRdisplay("LEAF %d has page_stat %x cc %d lsn %x %x CRPAGE %d kids %d\n",
		DM1B_VPT_GET_PAGE_PAGE_MACRO(log_pgtype, *page),
		DM1B_VPT_GET_PAGE_STAT_MACRO(log_pgtype, *page),
		DM1B_VPT_GET_BT_CLEAN_COUNT_MACRO(log_pgtype,*page),
		DM1B_VPT_GET_LOG_ADDR_LOW_MACRO(log_pgtype, *page),
		DM1B_VPT_GET_LOG_ADDR_HIGH_MACRO(log_pgtype, *page),
		DMPP_VPT_IS_CR_PAGE(log_pgtype, *page),
		DM1B_VPT_GET_BT_KIDS_MACRO(log_pgtype, *page));

	    dmd_print_brange(log_pgtype, log_pgsize,
		&btatts.bt_rng_rac, btatts.bt_rngklen, 
		btatts.bt_keys, &dmve->dmve_adf_cb, *page);
	    dmdprentries(log_pgtype, log_pgsize,
		&btatts.bt_leaf_rac, btatts.bt_klen, 
		btatts.bt_keys, &dmve->dmve_adf_cb, *page);
	    if (dmve->dmve_flags & DMVE_MVCC)
	    {
		for (i = 0, prevrec = dmve->dmve_prev_rec; 
		    i < *dmve->dmve_prev_cnt; 
			i++, prevrec += dmve->dmve_prev_size)
		{
		    TRdisplay("Make consistent previous undo to this page:\n");
		    dmd_log(TRUE, prevrec,  ((DM0L_HEADER *)prevrec)->length);
		}
	    }
	}
    }

    if (misc_buffer)
	dm0m_deallocate((DM_OBJECT **)&misc_buffer);

    /* Discard any allocated key buffer */
    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &KeyBuf);

    if (status == E_DB_OK)
    {
	if (DMZ_ASY_MACRO(6))
	{
	    TRdisplay("    Bid_check: Returns BID (%d,%d).\n",
		leaf_bid.tid_tid.tid_page, leaf_bid.tid_tid.tid_line);
	}

	*found_bid = leaf_bid;
	return (E_DB_OK);
    }

    /*
    ** If one of the above calls returned an unexpected error, log it
    ** If initialization error, it was already printed
    */
    if (init_done == TRUE)
    {
	uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
    }

    /*
    ** If the quest for the sacred page has failed, then we're hosed.
    */
    if (opflag == DMVE_FINDLEAF)
	str = "PAGE";
    else if (opflag == DMVE_FINDPOS) 
	str = "POSITION";
    else
	str = "ENTRY";

    uleFormat(NULL, E_DM968E_DMVE_BTREE_FIND, (CL_ERR_DESC *)NULL, ULE_LOG,
	NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 9,
	4, (recovery_action == DMVE_UNDO) ? "UNDO" : "REDO",
	sizeof(DB_DB_NAME), &dcb->dcb_name,
	sizeof(DB_TAB_NAME), table_name.db_tab_name,
	sizeof(DB_OWN_NAME), table_owner.db_own_name,
	(i4)STlength(str), str,
	0, log_bid->tid_tid.tid_page,
	0, log_bid->tid_tid.tid_line,
	4, (log_cmp_type != TCB_C_NONE) ? "ON, " : "OFF,",
	11, (bt_unique == TRUE) ? "UNIQUE.    " : "NOT UNIQUE.");


    if (init_done == TRUE)
    {
	/* Using key information, print the logged key */
    }

    SETDBERR(&dmve->dmve_error, 0, E_DM968F_DMVE_BID_CHECK);
    return (E_DB_ERROR);
}

static DB_STATUS
dmv_compare_key(
i4		page_type,
i4		page_size,
DMPP_PAGE  	*leaf,
DMVE_CB     	*dmve,
i4      	lineno,
DMP_ROWACCESS	*rac,
DB_ATTS      	**RangeKeys,
DB_ATTS      	**LeafKeyAtts,
i4      	key_count,
i4      	RangeKlen,
char        	*LeafKey,
i4          	*compare)
{
    DMP_DCB             *dcb = dmve->dmve_dcb_ptr;
    DB_STATUS           s;
    char                *tmpkey;
    char		*KeyBuf, *AllocKbuf;
    char                keybuf[DM1B_MAXSTACKKEY];
    i4             	tmpkey_len;
    i4             	infinity;
    i4			*err_code = &dmve->dmve_error.err_code;

    CLRDBERR(&dmve->dmve_error);

    /* This fcn is ONLY called when lineno is a range */
    if ( (lineno == DM1B_LRANGE) || (lineno == DM1B_RRANGE))
    {
	if (lineno == DM1B_LRANGE)
	    *compare = DM1B_SAME + 1;
	else
	    *compare = DM1B_SAME - 1;
	
	dm1cxtget(page_type, page_size, leaf, lineno, 
			(DM_TID *)&infinity, (i4*)NULL);
	if (infinity == TRUE)
	{
	    /* not same */
	    return (E_DB_OK);
	}
    }

    /* Get LRANGE/RRANGE Index key */
    if ( s = dm1b_AllocKeyBuf(RangeKlen, keybuf,
				&KeyBuf, &AllocKbuf, &dmve->dmve_error) )
    {
	return(s);
    }

    tmpkey = KeyBuf;
    tmpkey_len = RangeKlen;
    s = dm1cxget(page_type, page_size, leaf, rac,
		    lineno, &tmpkey, (DM_TID *)NULL, (i4*)NULL,
		    &tmpkey_len, NULL, NULL, &dmve->dmve_adf_cb);

    if (s == E_DB_WARN && (page_type != TCB_PG_V1) )
	s = E_DB_OK;

    if (s == E_DB_OK)
    {
	/* Compare leaf::range */
	s = adt_ixlcmp(&dmve->dmve_adf_cb, key_count,
			LeafKeyAtts, LeafKey,
			RangeKeys, tmpkey, compare);
	if (s != E_DB_OK)
	{
	    uleFormat(&dmve->dmve_adf_cb.adf_errcb.ad_dberror, 0, 
		(CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(&dmve->dmve_error, 0, E_DMF012_ADT_FAIL);
	}
    }
    else if (s == E_DB_ERROR)
    {
	dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, (DMP_RCB *)NULL, leaf,
			    page_type, page_size, lineno );
    }

    /* Discard any allocated key buffer */
    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &KeyBuf);

    return (s);
}
/*{
** Name: dmv_search - Search for a key or a key,tid pair on a page.
**
** Description:
**      This routines searches a BTREE index page for a key or key,tid pair.
**
**      This routine returns the line table position where the 
**      search ended.  It also assumes that the page pointed to by 
**      buffer is only one fixed on entry and exit.  This page
**      may change if search mode is DM1C_TID.
**
**	DM1C_FIND		    Locate the position on this page where
**      DM1C_OPTIM                  this entry is located, or the position where
**				    it would be inserted if it's not found. If
**				    the page is empty, or if this entry is
**				    greater than all entries on this page, then
**				    return E_DM0056_NOPART.
**
**	DM1C_TID                    Locate a specific entry on the page. The
**				    entry is located by comparing the TIDP of
**				    the entry on the page with the passed-in
**				    TID.
**
** Inputs:
**      dmve_cb
**      mode                            Value indicating type of search:
**                                          DM1C_FIND, DM1C_OPTIM or DM1C_TID
**      key                             The specified key to search for.
**      buffer                          Pointer to the page to search
**      page_size
**	tid				If mode == DM1C_TID, this value is the
**					TID which we use to search for the
**					exact entry to return.
**
** Outputs:
**      pos                             Pointer to an area to return
**                                      the position of the line table 
**                                      where search ended.
**      tid                             Pointer to an area to return
**                                      the tid of the record pointed to
**                                      by line table position.
**      err_code                        Pointer to an area to return 
**                                      the following errors when 
**                                      E_DB_ERROR is returned.
**                                      E_DM0056_NOPART
**					E_DM93BA_BXSEARCH_BADPARAM
**                                      
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**      30-jun-97 (stial01)
**          Created similar to dm1bxsearch(), without rcb dependency 
**          This code assumes search direction = DM1C_EXACT.
**	13-Jun-2006 (jenjo02)
**	    Using DM1B_MAXSTACKKEY instead of DM1B_KEYLENGTH, call dm1b_AllocKeyBuf() 
**	    to validate/allocate Leaf work space. DM1B_KEYLENGTH is now only the 
**	    max length of key attributes, not necessarily the length of the Leaf entry.
**	11-may-2007 (gupsh01)
**	   Initialize adf_utf8_flag.
*/  
static DB_STATUS
dmv_search(
i4		page_type,
i4		page_size,
DMPP_PAGE	*page,
DMVE_CB		*dmve,
DMVE_BT_ATTS	*btatts,
i4		mode,
char		*key,
DM_TID		*tid,
DM_LINE_IDX	*pos)
{
    DM0L_HEADER		*log_hdr = (DM0L_HEADER *)dmve->dmve_log_rec;
    DMP_DCB             *dcb = dmve->dmve_dcb_ptr;
    char	        *keypos; 
    char		*KeyBuf, *AllocKbuf = NULL;
    char	        keybuf[DM1B_MAXSTACKKEY];
    i4	        	keylen;
    DM_TID	        buftid; 
    DB_STATUS	        s; 
    i4	        	compare;
    CL_ERR_DESC	        sys_err;
    i4			*err_code = &dmve->dmve_error.err_code;

    CLRDBERR(&dmve->dmve_error);

    s = E_DB_OK;

    do
    {
	/* 
        ** Binary search the page to find the position of a
        ** key that is greater than or equal to one specified.
        ** If find multiple matching entries, get lowest such entry.
        */
        
	s = dmv_binary_search(page_type, page_size, page, dmve, 
			DM1C_EXACT, key, 
			&btatts->bt_leaf_rac,
			btatts->bt_leafkeys, btatts->bt_keys, 
			btatts->bt_rngklen, /* FIX ME bt_klen ?*/
			pos);
	if (s != E_DB_OK)
	{
	    uleFormat(&dmve->dmve_error, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(&dmve->dmve_error, 0, E_DM93BC_BXSEARCH_ERROR);
	    break;
	}

	if (mode != DM1C_TID)
	{
            if (*pos != DM1B_VPT_GET_BT_KIDS_MACRO(page_type, page))
		dm1cxtget(page_type, page_size, page, (*pos), 
				tid, (i4*)NULL );

	    break;
	}

	/* 
	** If the mode is DM1C_TID, then want the exact row matching the key
	** and TID values, not just the first entry matching the given key.
	** Search from this position onward, until we either locate the entry 
	** whose TIDP matches the indicated TID or run out of entries.  Note
	** that we extract the index entry using atts_array and atts_count which	
	** describe the entire entry, but compare keys using keyatts and 
	** key_count, which describe only the key if this is a secondary index.
	*/
	if ( s = dm1b_AllocKeyBuf(btatts->bt_rngklen, keybuf,
				    &KeyBuf, &AllocKbuf, &dmve->dmve_error) )
	{
	    break;
	}

	for (; (i4)*pos < 
		(i4)DM1B_VPT_GET_BT_KIDS_MACRO(page_type, page);
		(*pos)++)
	{
	    keypos = KeyBuf;
	    keylen = btatts->bt_rngklen;
	    s = dm1cxget(page_type, page_size, page,
			 &btatts->bt_leaf_rac, *pos, 
			 &keypos, (DM_TID *)&buftid, (i4*)NULL,
			 &keylen, NULL, NULL, &dmve->dmve_adf_cb);

	    if (page_type != TCB_PG_V1 && s == E_DB_WARN)
	    {
		s = E_DB_OK;
		/*
		** Nonuniq primary btree may contain deleted keys
		** with matching tidp.
		** UNDO DM0LBTPUT, skip delete etries
		*/
		if (log_hdr->type == DM0LBTPUT &&
		(dmve->dmve_action == DMVE_UNDO || (log_hdr->flags & DM0L_CLR)))
		    continue;
	    }

	    if (s != E_DB_OK)
	    {
		dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, (DMP_RCB *)NULL, page, 
				page_type, page_size, *pos);
		SETDBERR(&dmve->dmve_error, 0, E_DM93BC_BXSEARCH_ERROR);
	    }

	    if ( s == E_DB_OK )
	    {
		s = adt_kkcmp(&dmve->dmve_adf_cb, 
		   btatts->bt_keys, btatts->bt_leafkeys, keypos, key, &compare);

		if (s != E_DB_OK)
		{
		    uleFormat(&dmve->dmve_adf_cb.adf_errcb.ad_dberror, 0,
			(CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		    SETDBERR(&dmve->dmve_error, 0, E_DM93BC_BXSEARCH_ERROR);
		}
		if (compare != DM1B_SAME)
		{
		    SETDBERR(&dmve->dmve_error, 0, E_DM0056_NOPART);
		    s = E_DB_ERROR;
		}      
	    }

	    if ( s || (tid->tid_tid.tid_page == buftid.tid_tid.tid_page 
			&& tid->tid_tid.tid_line == buftid.tid_tid.tid_line) )
	    {
		/* Discard any allocated key buffer */
		if ( AllocKbuf )
		    dm1b_DeallocKeyBuf(&AllocKbuf, &KeyBuf);
		return (s); 
	    }
	}

	break;

    } while (FALSE);

    /* Discard any allocated key buffer */
    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &KeyBuf);

    if (s != E_DB_OK)
	return (s);

    if (*pos == DM1B_VPT_GET_BT_KIDS_MACRO(page_type, page))
    {
	SETDBERR(&dmve->dmve_error, 0, E_DM0056_NOPART);
        return (E_DB_ERROR);
    }    
    return(E_DB_OK); 
}


/*{
** Name: dmv_binary_search - Perform a binary search on a BTREE index page
**
** Description:
**      This routines examines the BTREE index page to determine the 
**      position of the supplied key. This routine has some complexity due to
**	the fact that a page may have multiple duplicate keys.
**
**	The semantics of this routine depend on whether the provided key
**	matches one of the entries on the page or not.
**
**	If the provided key does NOT match any of the entries on the page, then
**	this routine returns the location on the page where this key would be
**	inserted. This will be a value between 0 and bt_kids. As a special case
**	of this, if the page is empty we return *pos == 0.
**
**	If the provided key DOES match one or more of the entries on the page,
**	then the value returned in 'pos' depends on the value of 'direction':
**
**	    DM1C_NEXT               Return the position immediately FOLLOWING 
**                                  the last occurrence of the key on the page.
**                                  This will be a value between 1 and bt_kids
**
**	    DM1C_EXACT,DM1C_PREV    Return the position of the first occurrence
**                                  of the key on the page. This will be
**                                  a value between 0 and (bt_kids-1).
**
**	Note that an INDEX page never has duplicate entries; only leaf and
**	overflow pages have duplicate entries.
**
** Inputs:
**      dmve_cb
**      direction                       Value to indicate the direction
**                                      of search when duplicate keys are 
**                                      found:
**					    DM1C_NEXT
**					    DM1C_PREV
**					    DM1C_EXACT
**	key				The specified key to search for.
**	rac				Row-accessor structure for entry.
**      keyatts				Pointer to a structure describing
**                                      the attributes making up the key.
**      key_count                       Value indicating how many attributes
**                                      are in key.
**      klen				key length
**      page                            Pointer to the page to search
**      page_size
**
** Outputs:
**      pos                             Pointer to an area to return result.
**					See description above for the value
**					which is returned under various
**					combinations of input key and direction.
**	err_code			Set to an error code if an error occurs
**                                      
**	Returns:
**
**	    E_DB_OK
**	    E_DB_ERROR			Error uncompressing an entry
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-jun-97 (stial01)
**          Created similar to binary_search(), without rcb dependency 
**	13-Jun-2006 (jenjo02)
**	    Using DM1B_MAXSTACKKEY instead of DM1B_KEYLENGTH, call dm1b_AllocKeyBuf() 
**	    to validate/allocate Leaf work space. DM1B_KEYLENGTH is now only the 
**	    max length of key attributes, not necessarily the length of the Leaf entry.
*/
static DB_STATUS
dmv_binary_search(
i4		page_type,
i4		page_size,
DMPP_PAGE	*page,
DMVE_CB		*dmve,
i4		direction,
char		*key,
DMP_ROWACCESS	*rac,
DB_ATTS		**keyatts,
i4		key_count,
i4		klen,
DM_LINE_IDX	*pos)
{
    i4			right, left, middle;
    i4			compare;
    DB_STATUS		s;
    char		*KeyBuf, *AllocKbuf = NULL;
    char		key_buf[DM1B_MAXSTACKKEY];
    char		*key_ptr;
    i4			keylen;
    i4			*err_code = &dmve->dmve_error.err_code;

    CLRDBERR(&dmve->dmve_error);

    
    /* If page has no records, nothing to search. */

    if (DM1B_VPT_GET_BT_KIDS_MACRO(page_type, page) == 0)
    {
        *pos = 0;
        return (E_DB_OK);
    }

    /* 
    ** Start binary search of keys in record on page.  This binary_search is
    ** different from those you'll see in textbooks because we continue
    ** searching when a matching key is found.
    **
    ** Note that we use atts_array and atts_count to extract the index entry,
    ** since these describe the entire entry, but use keyatts and key_count 
    ** to perform the comparison, since these describe the search criteria.
    */

    if ( s = dm1b_AllocKeyBuf(klen, key_buf,
				&KeyBuf, &AllocKbuf, &dmve->dmve_error) )
    {
	return(s);
    }
   
    left = 0;
    right = DM1B_VPT_GET_BT_KIDS_MACRO(page_type, page) - 1;
    if (DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page) & DMPP_INDEX)
        right--;
    
    while (left <= right && s == E_DB_OK )
    {
        middle = (left + right)/2;

	key_ptr = KeyBuf;
	keylen = klen;
	s = dm1cxget(page_type, page_size, page, rac, 
			middle, &key_ptr, (DM_TID *)NULL, (i4*)NULL,
			&keylen,
			NULL, NULL, &dmve->dmve_adf_cb);

	if (page_type != TCB_PG_V1 && s == E_DB_WARN)
	    s = E_DB_OK;

	if (s != E_DB_OK)
	    dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, (DMP_RCB *)NULL, page, 
				page_type, page_size, middle);

	if ( s == E_DB_OK )
	{
	    s = adt_kkcmp(&dmve->dmve_adf_cb, (i4)key_count, keyatts, key_ptr, key, &compare);
	    if (s != E_DB_OK)
	    {
	        uleFormat(&dmve->dmve_adf_cb.adf_errcb.ad_dberror, 0, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    }
	    else
	    {
		if (compare > DM1B_SAME)
		    right = middle - 1;
		else if (compare < DM1B_SAME)
		    left = middle + 1;
		else if (direction == DM1C_NEXT)
		    left = middle + 1;
		else
		    right = middle - 1;
	    }
	}
    }

    /* Discard any allocated key buffer */
    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &KeyBuf);

    if ( s )
	SETDBERR(&dmve->dmve_error, 0, E_DM93B3_BXBINSRCH_ERROR);
    else
	*pos = left;

    return (s);
}

static DB_STATUS
dmve_init_atts(
DMVE_CB             *dmve,
DMP_TABLE_IO        **tbio_ptr,  /* may unfix/refix tbio */
DMP_PINFO	    **pinfoP,  /* may unfix/refix page_ptr */
u_i4		    log_btflags,
i4		    log_cmp_type,
DMVE_BT_ATTS	    *btatts,
DM_OBJECT           **misc_buffer)
{
    DM0L_HEADER		*log_hdr = (DM0L_HEADER *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_hdr->lsn; 
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    DMP_TCB		*t;
    DMP_TCB		*tmp_tcb = NULL;
    DM_PAGENO		pageno;
    DB_STATUS		status = E_DB_OK;
    i4			pgtype;
    i4			pgsize;
    DB_ATTS             *atr;           /* Ptr to current attribute */
    DB_ATTS		*tidp_att, *extra_tidp_att;
    DM1B_ATTR           attdesc;
    i4             	attno;
    i2			abs_type;
    i4			koffset;
    i4			attlen;
    DB_TAB_ID		master_id;
    DB_TAB_NAME		table_name;
    DB_OWN_NAME		table_owner;
    DB_TAB_ID		tableid;
    DM_OBJECT           *obj = NULL;
    char		*buffer;
    i4			size_needed;
    i4			*err_code = &dmve->dmve_error.err_code;
    DMP_PINFO		*pinfo = *pinfoP;
    DMPP_PAGE		**page = &pinfo->page;
    i4			attnmsz;
    char		*attnames;
    char		*nextattname;

    MEfill(sizeof(DMVE_BT_ATTS), 0, btatts);

    /* This might be partial tcb, but it will always basic table info */
    t = (DMP_TCB *)((char *)*tbio_ptr - CL_OFFSETOF(DMP_TCB, tcb_table_io));

    /* PARTIAL tcb, get attribute, key information from leaf page  */
    if (*page && DM1B_VPT_GET_BT_ATTCNT(t->tcb_rel.relpgtype, *page))
    {
	i4 ctl_size;
	i4 leafattcnt = DM1B_VPT_GET_BT_ATTCNT(t->tcb_rel.relpgtype, *page);

	btatts->bt_leaf_rac.att_count = leafattcnt;
	/* The atts themselves; one pointer array for all atts, one for
	** keys as they are on the leaf, one for keys as they are on the
	** index / range entry.  Leaf keys and index/range keys are the
	** same (because SI non-keys always follow SI keys), EXCEPT for
	** the tidp offset being different in the index/range key (if
	** tidp is a key).
	** Allow one extra att entry in case we need a jiggered tidp att.
	*/
	attnmsz = (leafattcnt+1) * 32; /* generated att names */
	size_needed = DB_ALIGN_MACRO(attnmsz +
		+ (leafattcnt+1) * sizeof(DB_ATTS))
		+ DB_ALIGN_MACRO((leafattcnt * sizeof(DB_ATTS *)))
		+ DB_ALIGN_MACRO((leafattcnt * sizeof(DB_ATTS *)))
		+ DB_ALIGN_MACRO((leafattcnt * sizeof(DB_ATTS *)));
 
	/* Build compression-control arrays for leaf and range keys.
	** Assume the worst and allocate one for leaf and one for range;
	** the leaf is larger (if different).
	** Won't need any row-versioning poop here.
	*/
	ctl_size = 0;
	btatts->bt_leaf_rac.compression_type = log_cmp_type;
	btatts->bt_rng_rac.compression_type = log_cmp_type;
	if (log_cmp_type != TCB_C_NONE)
	{
	    ctl_size = dm1c_cmpcontrol_size(log_cmp_type, leafattcnt, 0);
	    ctl_size = DB_ALIGN_MACRO(ctl_size);
	    size_needed += 2*ctl_size;
	}

	/*
	** Allocate attribute arrays if necessary.
	** There is a limit of 32 key componants for secondary btrees.
	** There is no limit for primary btree indexes
	*/
	buffer = btatts->bt_temp;
	if (size_needed > sizeof(btatts->bt_temp)) 
	{
	    status = dm0m_allocate(sizeof(DMP_MISC) + size_needed,
		(i4)0, (i4)MISC_CB, (i4)MISC_ASCII_ID,
		(char *)NULL, (DM_OBJECT **)misc_buffer, &dmve->dmve_error);
	    if (status != OK)
		return (E_DB_ERROR);
	    buffer = (char *)misc_buffer + sizeof(DMP_MISC);
	    ((DMP_MISC *)misc_buffer)->misc_data = buffer;
	}

	buffer = ME_ALIGN_MACRO(buffer, sizeof(PTR));
	attnames = (char *) buffer;
	nextattname = attnames;
	btatts->bt_leaf_rac.att_ptrs = (DB_ATTS **)(buffer + attnmsz);
	btatts->bt_leafkeys = btatts->bt_leaf_rac.att_ptrs + leafattcnt;
	btatts->bt_rngkeys = btatts->bt_leafkeys + leafattcnt;
	atr = (DB_ATTS *)(btatts->bt_rngkeys + leafattcnt);
	for (attno = 0; attno < leafattcnt; attno++)
	{
	    btatts->bt_leaf_rac.att_ptrs[attno] = atr++;
	}
	extra_tidp_att = atr;	/* may or may not need this */
	if (ctl_size > 0)
	{
	    btatts->bt_leaf_rac.control_count = ctl_size;
	    btatts->bt_rng_rac.control_count = ctl_size;
	    btatts->bt_leaf_rac.cmp_control = (char *) atr;
	    btatts->bt_rng_rac.cmp_control = (char *) atr + ctl_size;
	}

	btatts->bt_klen = 0;
	btatts->bt_keys = 0;

	attlen = 0;
	koffset = 0;
	for (attno = 0; attno < leafattcnt; attno++)
	{
	    atr = btatts->bt_leaf_rac.att_ptrs[attno];

	    /*
	    ** Init attr name so dm1cxformat can compare atts with keys
	    */
	    atr->attnmstr = nextattname;
	    STprintf(atr->attnmstr, "%d", attno);
	    atr->attnmlen = STlength(atr->attnmstr);
	    nextattname += (atr->attnmlen + 1);

	    DM1B_VPT_GET_ATTR(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 			*page, (attno+1), &attdesc);
	    atr->precision = 0;
	    atr->collID = -1;
	    atr->type = attdesc.type;
	    atr->length = attdesc.len;
	    abs_type = abs(atr->type);
	    if (abs_type == DB_CHA_TYPE || abs_type == DB_CHR_TYPE 
		|| abs_type == DB_VCH_TYPE || abs_type == DB_TXT_TYPE 
		|| abs_type == DB_NCHR_TYPE || abs_type == DB_NVCHR_TYPE)
		atr->collID = attdesc.u.collID;
	    else atr->precision = attdesc.u.prec;
	    atr->offset = attlen;
	    atr->flag = 0;
	    atr->key = 0;
	    atr->key_offset = 0;
	    atr->ver_added = 0;
	    atr->ver_dropped = 0;
	    atr->val_from = 0;

	    if (attdesc.key)
	    {
		btatts->bt_rngkeys[btatts->bt_keys] =
			btatts->bt_leafkeys[btatts->bt_keys] = btatts->bt_leaf_rac.att_ptrs[attno];
		++ btatts->bt_keys;
		atr->key = btatts->bt_keys;
		atr->key_offset = attlen;
		koffset += atr->length;
	    }

	    /* 
	    ** Primary index: each attribute in leaf entry is 
	    ** a key componant and included in klen.
	    ** Secondary index: all attributes in secondary index,
	    ** including non key fields and tidp are in klen
	    */
	    attlen += atr->length;
	}

	/* Assume range key is pure key without any non-key columns */
	btatts->bt_rngklen = koffset;
	btatts->bt_rng_rac.att_ptrs = btatts->bt_rngkeys;
	btatts->bt_rng_rac.att_count = btatts->bt_keys;

	btatts->bt_klen = koffset;
	if (t->tcb_rel.reltid.db_tab_index > 0)
	{
	    /* For secondary index, leaf key is everything including
	    ** any non-key atts.
	    ** Index and maybe range key excludes the non-key atts.
	    ** If there are non-keys and the (trailing) tidp is a key,
	    ** tidp will be at a different offset in the index (key-only)
	    ** entry than on the leaf entry.
	    */
	    tidp_att = btatts->bt_leaf_rac.att_ptrs[leafattcnt-1];
	    btatts->bt_klen = attlen;
	    if (attlen != koffset && tidp_att->key != 0)
	    {
		*extra_tidp_att = *tidp_att;
		extra_tidp_att->offset = koffset - extra_tidp_att->length;
		extra_tidp_att->key_offset = extra_tidp_att->offset;
		btatts->bt_rngkeys[btatts->bt_keys-1] = extra_tidp_att;
	    }
	    if (log_btflags & DM0L_BT_RNG_HAS_NONKEY)
	    {
		/* Somewhere between 9.1 and 10.0 the range entry was changed
		** to NOT include the non-key atts.  For older range entries,
		** include non-key atts, ie range key == leaf key.
		*/
		btatts->bt_rngkeys = btatts->bt_leafkeys;
		btatts->bt_rngklen = btatts->bt_klen;
		btatts->bt_rng_rac.att_ptrs = btatts->bt_leaf_rac.att_ptrs;
		btatts->bt_rng_rac.att_count = btatts->bt_leaf_rac.att_count;
	    }
	}

	/* Set up rowaccessors, e.g. compression control */
	status = dm1c_rowaccess_setup(&btatts->bt_leaf_rac);
	if (status == E_DB_OK)
	    status = dm1c_rowaccess_setup(&btatts->bt_rng_rac);
	if (status != E_DB_OK)
	    return (status);

	btatts->bt_kperleaf = dm1cxkperpage(t->tcb_rel.relpgtype, 
	    t->tcb_rel.relpgsize, 0, 
	    TCB_BTREE, btatts->bt_leaf_rac.att_count, btatts->bt_klen, DM1B_PLEAF, 
	    DM1B_VPT_GET_BT_TIDSZ_MACRO(t->tcb_rel.relpgtype, *page), 
	    0	/* Never CLUSTERED */ );

        return (E_DB_OK);
    }

    /*
    ** PARTIAL tcb and no attribute information on leaf...
    ** Unfix page, unfix tbio, fix NOPARTIAL tcb, fix tbio, unfix tcb, fix page
    ** First copy PARTIAL table information
    */
    pgtype = t->tcb_rel.relpgtype;
    pgsize = t->tcb_rel.relpgsize;
    STRUCT_ASSIGN_MACRO(t->tcb_rel.relid, table_name);
    STRUCT_ASSIGN_MACRO(t->tcb_rel.relowner, table_owner);
    STRUCT_ASSIGN_MACRO(t->tcb_rel.reltid, tableid);
    pageno = DMPP_VPT_GET_PAGE_PAGE_MACRO(pgtype, *page);

    for (  ; ; )
    {
	/* Don't unfix page/tbio if this is partitioned table */
	if (tableid.db_tab_index >= 0)
	{
	    /* uncache_fix, but leave the page locked */
	    status = dm0p_uncache_fix(*tbio_ptr, DM0P_UNFIX, dmve->dmve_lk_id, 
		dmve->dmve_log_id, &dmve->dmve_tran_id, pinfo, &dmve->dmve_error);
	    if (status != E_DB_OK)
		break;

	    status = dmve_unfix_tabio(dmve, tbio_ptr, 0);
	    if (status != E_DB_OK)
		break;
	}

	STRUCT_ASSIGN_MACRO(tableid, master_id);
	if (master_id.db_tab_index < 0)
	    master_id.db_tab_index = 0;
	status = dm2t_fix_tcb(dcb->dcb_id, &master_id, 
		&dmve->dmve_tran_id, dmve->dmve_lk_id, dmve->dmve_log_id,
		(DM2T_VERIFY | DM2T_NOPARTIAL | DM2T_SHOWONLY),
		dcb, &tmp_tcb, &dmve->dmve_error);
	if (status != E_DB_OK)
	    break;

	/* fix tbio (if we unfixed it */
	if (*tbio_ptr == NULL)
	{
	    status = dmve_fix_tabio(dmve, &tableid, tbio_ptr);
	    if (status != E_DB_OK)
		break;
	}

	/* re-fix page (if we unfixed it) */
	if (*page == NULL)
	{
	    status = dmve_fix_page(dmve, *tbio_ptr, pageno, pinfoP);
	    if (status != E_DB_OK)
		break;
	}

	/* unfix tmp_tcb we don't need it, NOPARIATL tcb pinned by tbio */
        status = dm2t_unfix_tcb(&tmp_tcb, (i4)DM2T_VERIFY, 
		dmve->dmve_lk_id, dmve->dmve_log_id, &dmve->dmve_error);
	if (status != E_DB_OK)
	    break;
	
	/* This might be partial tcb, but it will always basic table info */
	t = (DMP_TCB *)((char *)*tbio_ptr - CL_OFFSETOF(DMP_TCB, tcb_table_io));
	if (t->tcb_status & TCB_PARTIAL)
	{
	    /* This error shouldn't happen */
	    SETDBERR(&dmve->dmve_error, 0, E_DM9664_DMVE_INFO_UNKNOWN);
	    status = E_DB_ERROR;
	    break;
	}

	btatts->bt_keys = t->tcb_keys;
	btatts->bt_klen = t->tcb_klen;
	btatts->bt_leafkeys = t->tcb_leafkeys;
	btatts->bt_kperleaf = t->tcb_kperleaf;
	btatts->bt_rngkeys = t->tcb_rngkeys;
	btatts->bt_rngklen = t->tcb_rngklen;
	STRUCT_ASSIGN_MACRO(t->tcb_leaf_rac, btatts->bt_leaf_rac);
	STRUCT_ASSIGN_MACRO(*t->tcb_rng_rac, btatts->bt_rng_rac);

	break; /* Always */
    }

    if (tmp_tcb)
    {
	DB_STATUS	local_status;

        local_status = dm2t_unfix_tcb(&tmp_tcb, (i4)DM2T_VERIFY, 
	    dmve->dmve_lk_id, dmve->dmve_log_id, &dmve->dmve_error);
	if (local_status)
	    status = E_DB_ERROR;
    }

    if (status)
    {
	uleFormat(NULL, E_DM968D_DMVE_BTREE_INIT, (CL_ERR_DESC *)NULL, ULE_LOG,
	    NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 3,
	    sizeof(DB_DB_NAME), &dcb->dcb_name,
	    sizeof(DB_TAB_NAME), table_name.db_tab_name,
	    sizeof(DB_OWN_NAME), table_owner.db_own_name);
    }

    return (status);
}

static VOID
dmve_check_atts(
DMVE_CB             *dmve,
DMP_TABLE_IO        **tbio,  /* may unfix/refix tbio */
DMP_PINFO	    **pinfoP,  /* may unfix/refix page_ptr */
u_i4		    log_btflags,
i4		    log_cmp_type)
{
    DB_STATUS		status;
    DMVE_BT_ATTS	checkatts;
    DM_OBJECT           *misc_buffer = NULL;
    DMP_TCB		*t;

    status = dmve_init_atts(dmve, tbio, pinfoP, log_btflags,
		log_cmp_type, &checkatts, &misc_buffer);

    t = (DMP_TCB *)((char *)*tbio - CL_OFFSETOF(DMP_TCB, tcb_table_io));
    TRdisplay("CHECK tbl(%d,%d) keys %d %d klen %d %d leafattcnt %d %d kperleaf %d %d rngklen %d %d rngattcnt %d %d \n",
	    t->tcb_rel.reltid.db_tab_base,
	    t->tcb_rel.reltid.db_tab_index,
	    checkatts.bt_keys, t->tcb_keys,
	    checkatts.bt_klen, t->tcb_klen,
	    checkatts.bt_leaf_rac.att_count, t->tcb_leaf_rac.att_count,
	    checkatts.bt_kperleaf, t->tcb_kperleaf,
	    checkatts.bt_rngklen, t->tcb_rngklen,
	    checkatts.bt_rng_rac.att_count, t->tcb_rng_rac->att_count);

    /* compare DMVE_BT_ATTS (from page) to DMP_TCB attinfo */
    if (checkatts.bt_keys != t->tcb_keys
	    || checkatts.bt_klen != t->tcb_klen
	    || checkatts.bt_leaf_rac.att_count != t->tcb_leaf_rac.att_count
	    || checkatts.bt_kperleaf != t->tcb_kperleaf
	    || checkatts.bt_rngklen != t->tcb_rngklen
	    || checkatts.bt_rng_rac.att_count != t->tcb_rng_rac->att_count)
    {
	TRdisplay("ERROR tbl(%d,%d) keys %d %d klen %d %d leafattcnt %d %d kperleaf %d %d rngklen %d %d rngattcnt %d %d \n",
	    t->tcb_rel.reltid.db_tab_base,
	    t->tcb_rel.reltid.db_tab_index,
	    checkatts.bt_keys, t->tcb_keys,
	    checkatts.bt_klen, t->tcb_klen,
	    checkatts.bt_leaf_rac.att_count, t->tcb_leaf_rac.att_count,
	    checkatts.bt_kperleaf, t->tcb_kperleaf,
	    checkatts.bt_rngklen, t->tcb_rngklen,
	    checkatts.bt_rng_rac.att_count, t->tcb_rng_rac->att_count);
    }

    if (misc_buffer)
	dm0m_deallocate((DM_OBJECT **)&misc_buffer);
}
