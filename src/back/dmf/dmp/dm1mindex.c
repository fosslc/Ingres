/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmpl.h>
#include    <dmpp.h>
#include    <dmp.h>
#include    <dm1b.h>
#include    <dm1m.h>
#include    <dm0m.h>
#include    <dm0p.h>
#include    <dm0l.h>
#include    <dm1c.h>
#include    <dm1cx.h>
#include    <dm1p.h>
#include    <dmftrace.h>
#include    <dmd.h>
#include    <dm2f.h>

/**
**
**  Name: DM1MINDEX.C - Routines needed to manipulate Rtree indices.
**
**  Description:
**      This file contains all the routines needed to manipulate Rtree
**      index and leaf pages.   Index and leaf pages of Rtrees have
**      records which  consist of a key and tid.  In the case of a
**      leaf page, the tid identifies a tuple.  In the case of an
**      index page, the tid associated with a key identifies a page
**      which contains records with keys less then or equal to this key.
**      Note that in the last entry of a leaf page only the tid is
**      significant since the key can be considered the infinite key.
**
**      This can be seen as follows:
**
**                          index page 0
**                        +-------------------+
**                        | header info       |
**                        |-------------------|
**                        | line table        |
**                        |-------------------|
**                        |r1 key = "DEF"     |-----------------+
**                        |   tid = pg 6,ln 0 |                 |
**                        |-------------------|                 |
**                +-------|r0 key = "ABC"     |                 |
**                |       |   tid = pg 5,ln 0 |                 |
**                |       +-------------------+                 |
**                |       |rn key = <infinite>|                 |
**                |       |   tid = pg 7,ln 0 |                 |
**                v       +-------------------+                 v
**        leaf page 5                                    leaf page 6
**      +-------------------+                          +-------------------+
**      | header info       |                          | header info       |
**      |-------------------|                          |-------------------|
**      | line table        |                          | line table        |
**      |-------------------|                          |-------------------|
**      |r0 key = "AAA"     |                          |r1 key = "DEF"     |
**      |   tid = pg 9,ln 12|                          |   tid = pg 10,ln 4|
**      |-------------------|                          |-------------------|
**      |r1 key = "ABC"     |                          |r0 key = "DEF"     |
**      |   tid = pg 9,ln 2 |                          |   tid = pg 11,ln 1|
**      +-------------------+                          +-------------------+
**
**	For primary relations, the leaf record tid points to a tuple
**      within the same file.	For secondary indexes, it points to a
**      tuple in the associated	primary relation.
**
**	Each page contains a line table (referred to as bt_sequence).
**
**	Note that entries are stored on the page in random order; however, the
**	line table (sequence) offsets for the entries are kept in correct
**	sorted Hilbert value order.
**
**	For Rtrees which do not perform entry compression, the line table
**	contains a set of pointers that are fixed at the time the page is
**	formatted, but are shuffled about on insertions and deletions. An
**	initial segment of this array of pointers represents (key, tid)
**	addresses that are currently in use, in the order of key value.  The
**	length of this segment is recorded in bt_kids.
**
**	Note also that each leaf page contains a low key value and a high
**	key value which define the range of the keys that may possibly
**	fall into that page. These are used in moving to the "next" higher
**	leaf page in an Rtree. They occupy the first 2 positions
**	of the line table.
**
**      The routines contained in this file are:
**
** 	dm1mxadjust	- Adjust the MBR and LHV of an index entry.
** 	dm1mxcompare 	- Perform a compare by minimum bounding rectangle.
**      dm1mxdelete	- Deletes a key,tid pair from a page.
**	dm1mxdistribute - distribute overflowing entries onto sibling pages.
**      dm1mxformat	- Formats an empty page.
**	dm1mxhilbert 	- Return the Hilbert value of this entry.
**      dm1mxinsert	- Inserts a key,tid pair onto a page.
** 	dm1mxkkcmp 	- Compare keys in an R-tree.
**      dm1mxnewroot    - Creates a new root.
**      dm1mxsearch	- Searches for a key or key,tid pair on a page.
**      dm1mxsplit	- Split a page into two pages.
**
**
**  History:
** 	 5-may-95 (wonst02)
** 		Original (loosely cloned from dm1bindex.c)
**		=== ingres!main!generic!back!dmf!dmp dm1bindex.c rev 31 ====
**	25-apr-1996 (wonst02)
**	    Add code for searching.
**	25-may-1996 (wonst02)
**	    Integrate new page format changes.
**	18-nov-1996 (wonst02)
**	    Incorporate use of DMPP_SIZEOF_TUPLE_HDR_MACRO for new page format.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Pass reclaim_space to dm1cxdel()
**          Removed unecessary DMPP_INIT_TUPLE_INFO_MACRO
**	25-nov-1996 (wonst02)
**	    Modified parameters of dm1mxcompare(), dm1mxsearch(), et al.
**	13-dec-1996 (wonst02)
**	    Call dm1m_rcb_update after shifting entries between pages.
**      27-feb-97 (stial01)
**          Init flag param for dm1cxdel()
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      12-jun-97 (stial01)
**          Pass tlv to dm1cxget instead of tcb.
**          Pass new attribute, key parameters to dm1cxformat
**          RTREE does not need key description information on leaf pages.
**      21-apr-98 (stial01)
**          Set DM0L_PHYS_LOCK in log rec if extension table (B90677)
**	01-dec-1998 (nanpr01)
**	    Get rid of DMPP_TUPLE_INFO structure. Use DMPP_TUPLE_HDR instead.
**      07-Dec-1998 (stial01)
**          Added kperleaf to distinguish from kperpage (keys per index page)
**      12-Apr-1999 (stial01)
**          Pass page_type parameter to dmd_prkey
** 	05-apr-1999 (wonst02)
** 	    Fix RTREE bug B95350 in Rel. 2.5. 
** 	    Remove DMPP_SIZEOF_TUPLE_HDR_MACRO from several places.
**	16-aug-1999 (nanpr01)
**	    Implementation of SIR 92299. Enabaling journalling on a
**	    online checkpoint.
**	20-oct-1999 (nanpr01)
**	    Optimized the code to minimize the tuple header copy.
**	04-May-2000 (jenjo02)
**	    Pass blank-suppressed lengths of table/owner names to 
**	    dm0l_rtdel, dm0l_rtput, dm0l_rtrep.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Arg changes for page accessors (Variable Page Types SIR 102955)
**      08-Oct-2002 (hanch04)
**          Removed dm0m_lcopy,dm0m_lfill.  Use MEcopy, MEfill instead.
**      13-feb-2003 (chash01)  x-integrate change#461908
**          in dmbr_search, initialize keylen to 0.
**	22-Jan-2004 (jenjo02)
**	    A host of changes for Partitioning and Global indexes,
**	    particularly row-locking with TID8s (which include
**	    partition number (search for "partno") along with TID).
**	    LEAF pages in Global indexes will contain DM_TID8s 
**	    rather than DM_TIDs; the TID size is now stored on
**	    the page (4 for all but LEAF, 8 for TID8 leaves).
**	    Note, however, the RTREEs don't yet support
**	    Global indexes.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      06-Jul-2004 (hanal04) Bug 112611 INGSRV2895
**          Prevent spurious E_DM9385 errors caused by parameter
**          ordering problems in calls to dm1mxkkcmp().
**	16-dec-04 (inkdo01)
**	    Add various collID's.
**      31-jan-05 (stial01)
**          Added flags arg to dm0p_mutex call(s).
**	13-Apr-2006 (jenjo02)
**	    Prototype changes for unrelated CLUSTERED primary Btree
**	    tables.
**	24-Feb-2008 (kschendel) SIR 122739
**	    Various changes for new rowaccess structure.
**	11-Apr-2008 (kschendel)
**	    RCB's adfcb now typed as such, minor fixes here.
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	03-Dec-2008 (jonj)
**	    SIR 120874: dm1m? functions converted to DB_ERROR *,
**	    use new form uleFormat
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1p_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**      12-oct-2009 (stial01)
**          Use DMPP_PAGE for all page types.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: page accessor parameter changes.
**	    As Rtrees are not supported by either row or crow locking, 
**	    saved a lot of messy destabilizing
**	    code changes that would otherwise be needed by adding
**	    bridge functions dm1m_fix_page(), dm1m_unfix_page(), dm1m_mutex()
**	    dm1m_unmutex(), dm1m_getfree(), so dm1m functions can remain 
**	    unconverted to DMP_PINFO.
*/

/*
**  Forward function references.
*/

static	DB_STATUS binary_search(
    DMP_RCB      *rcb,
    DMPP_PAGE   *page,
    i4      direction,
    char         *key,
    ADF_CB	 *adf_cb,
    DMPP_ACC_KLV *klv,
    i4		 pgtype,
    i4		 pgsize,
    i4		 *pos,
    DB_ERROR	 *dberr);

static	DB_STATUS mbr_search(
    DMP_RCB      *rcb,
    DMPP_PAGE   *page,
    i4      direction,
    char         *key,
    ADF_CB	 *adf_cb,
    DMPP_ACC_KLV *klv,
    i4		 pgtype,
    i4		 pgsize,
    i4		 *pos,
    DB_ERROR	 *dberr);

static void get_range_entries(
    DMPP_PAGE		*cur,
    DMPP_PAGE		*sib,
    DMPP_ACC_KLV	*klv,
    i4          page_type,
    i4		page_size,
    char		*range[],
    i4		range_len[],
    DM_TID		infinity[],
    DB_ERROR	 *dberr);

static DB_STATUS update_range_entries(
    DMPP_PAGE		*cur,
    DMPP_PAGE		*sib,
    ADF_CB	 	*adf_cb,
    DMPP_ACC_KLV	*klv,
    i4		page_type,
    i4		page_size,
    i4	    	compression_type,
    DB_TRAN_ID      	*tran_id,
    u_i2		lg_id,
    i4		seq_number,
    char		*range[],
    i4		range_len[],
    DM_TID		infinity[],
    char		*midkey,
    i4		midkey_len,
    DB_ERROR	 *dberr);

static DB_STATUS put_range_entries(
    DMPP_PAGE		*cur,
    i4	    	compression_type,
    u_i2		hilbertsize,
    i4		page_type,
    i4		page_size,
    DB_TRAN_ID      	*tran_id,
    u_i2		lg_id,
    i4		seq_number,
    char		*range[],
    i4		range_len[],
    DM_TID		infinity[]);


/*{
** Name: dm1mxadjust	- Adjust the MBR/LHV of an index page.
**
** Description:
** 	Log the Adjust to the Rtree Index Page.
** 	Proceed with the adjust--replace the MBR/LHV of the index entry.
**
** Inputs:
**      rcb				Pointer to record access context.
**	tcb				Pointer to table control block.
**	buffer				Pointer to the page containing
**					the record to adjust.
**	page_size			Page size for this Rtree index
**	klen				Length of key entry for this table.
**					This length should NOT include the tid
**					portion of the (key,tid) pair.
** 	newkey				The value of the new key (MBR/LHV).
**      child				A LINE_ID portion of a TID which
**					points to the record to adjust.
**	log_updates			Indicates to log the update.
**	mutex_required			Indicates to mutex page while updating.
**
** Outputs:
**	err_code			Set if this routine returns E_DB_ERROR:
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
** History:
**	15-jul-1996 (wonst02)
**	    New.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
*/
DB_STATUS
dm1mxadjust(
    DMP_RCB	    *rcb,
    DMP_TCB	    *tcb,
    DMPP_PAGE	    *buffer,
    i4	    page_size,
    i4	    klen,
    char	    *newkey,
    i4	    child,
    i4	    log_updates,
    i4	    mutex_required,
    DB_ERROR	    *dberr)
{
    DMP_RCB		*r = rcb;
    DMP_TCB		*t = tcb;
    DMP_DCB     	*d = t->tcb_dcb_ptr;
    DMP_TABLE_IO	*tbio = &t->tcb_table_io;
    DMPP_PAGE		*b = buffer;
    DB_STATUS		status = E_DB_OK;
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    DM_TID		bid;
    DM_TID		tid;
    DB_TRAN_ID	    	*tran_id;
    u_i2		lg_id;
    LG_LSN		lsn;
    i4	    	sequence_number;
    i4		loc_config_id;
    i4		loc_id;
    i4		key_len;
    i4		dm0l_flag;
    i4		compression_type;
    bool		log_required;
    bool		mutex_done = FALSE;
    bool		critical_section = FALSE;
    char		*oldkey;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** Setup information needed for dm1cx calls and the log record write below.
    */
    bid.tid_tid.tid_page = DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, b);
    bid.tid_tid.tid_line = child;
    log_required = (log_updates && r->rcb_logging);
    compression_type = DM1B_INDEX_COMPRESSION(r);

    /*
    ** Get session/statement information for the following updates.
    ** If called during LOAD operations (no rcb available) then no
    ** transaction id or sequence number is written to the page.
    */
    if (r)
    {
	tran_id = &r->rcb_tran_id;
	lg_id = r->rcb_slog_id_id;
	sequence_number = r->rcb_seq_number;
    }
    else
    {
	tran_id = (DB_TRAN_ID *)0;
	lg_id = 0;
	sequence_number = 0;
    }

    /*
    ** Get information on the location to which the update is being made.
    */
    loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count,
		(i4)DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, b));
    loc_config_id = tbio->tbio_location_array[loc_id].loc_config_id;

    /*
    ** Look up pointer to the key to be adjusted.  In addition to
    ** validating the existence of the entry, the key is needed in the
    ** log call below.  Also get the length of the (possibly compressed)
    ** key entry and its tidp.
    */
    dm1cxrecptr(tbio->tbio_page_type, tbio->tbio_page_size, b, child, &oldkey);
    dm1cxtget(tbio->tbio_page_type, tbio->tbio_page_size, b, child,
		&tid, (i4*)NULL);

    key_len = t->tcb_klen;

    for (;;)
    {
	/*
	** Reserve the log space for the Rtree index adjust log record and for
	** its CLR should the transaction be rolled back.
	** This must be done previous to mutexing the page since the reserve
	** request may block.
	*/
	if ((r) && (d->dcb_status & DCB_S_BACKUP))
	{
	    /*
	    ** Online Backup Protocols: Check if BI must be logged.
	    */
	    status = dm0p_before_image_check(r, b, dberr);
	    if (status != E_DB_OK)
		break;
	}
	if (log_required)
	{
	    /* Reserve space for log and CLR  */
	    cl_status = LGreserve(0, r->rcb_log_id, 2,
		(2 * (sizeof(DM0L_RTPUT) - (DB_MAXRTREE_KEY - key_len))),
				  &sys_err);
	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			err_code, 1, 0, r->rcb_log_id);
		SETDBERR(dberr, 0, E_DM93B9_BXINSERT_ERROR);
		break;
	    }
	}

	/*
	** If the buffer resides in the DBMS buffer cache, then we have to mutex
	** it while we change it to prevent other threads from looking at it
	** while it is inconsistent.
	*/
	if (mutex_required)
	{
	    dm1m_mutex(tbio, (i4)0, r->rcb_lk_id, &b);
	    mutex_done = TRUE;
	}

	/*
	** Log the Adjust to the Rtree Index Page.
	** The LSN of the log record must be written to the updated page.
	** If the log write fails then return without updating the page.
	*/
	if (log_required)
	{
	    dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0) | DM0L_PHYS_LOCK;

	    if ( r->rcb_state & RCB_IN_MINI_TRAN )
	        dm0l_flag |= DM0L_MINI;

            status = dm0l_rtrep(r->rcb_log_id, dm0l_flag, &tcb->tcb_rel.reltid,
		    &tcb->tcb_rel.relid, tcb->tcb_relid_len,
		    &tcb->tcb_rel.relowner, tcb->tcb_relowner_len,
		    tcb->tcb_rel.relpgtype, tcb->tcb_rel.relpgsize,
		    tcb->tcb_index_rac.compression_type,
		    tcb->tcb_rel.relloccount, loc_config_id,
		    tcb->tcb_acc_klv->hilbertsize, tcb->tcb_acc_klv->obj_dt_id,
		    &bid, &tid, klen, oldkey, newkey,
		    (r->rcb_ancestor_level+1)*sizeof(DM_TID), r->rcb_ancestors,
		    (LG_LSN *) 0, &lsn,
		    dberr);
	    if (status != E_DB_OK)
		break;

	    /*
	    ** Now that the update has been logged, we must succeed in changing
	    ** the page.  If we encounter an error now, we must crash and let
	    ** the page contents be rebuilt via REDO recovery.
	    */
	    critical_section = TRUE;

	    /*
	    ** Write the LSN of the insert record to the page being updated.
	    ** This marks the version of the page for REDO processing.
	    */
	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, b, lsn);
        }

	/*
	** Proceed with the adjust--replace the MBR/LHV of the index entry.
	*/
        status = dm1cxput(tbio->tbio_page_type, tbio->tbio_page_size, b,
			   compression_type, DM1C_DIRECT, tran_id, lg_id,
		           sequence_number, child,
			   newkey, klen, &tid, (i4)0 );

	if (status != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E4_BAD_INDEX_PUT, r, b, 
		tbio->tbio_page_type, tbio->tbio_page_size, child );
	    break;
	}

	/*
	** Unmutex the leaf page now that we are finished updating it.
	*/
	if (mutex_required)
	    dm1m_unmutex(tbio, (i4)0, r->rcb_lk_id, &b);

	return (E_DB_OK);
    }

    /*
    ** An error occurred while updating the index page leaving the page's
    ** contents possibly trashed.  If we are logging updates, then we're
    ** hosed.  We crash the server and let REDO recovery correctly
    ** rebuild the page contents.  Note that our inconsistent page cannot
    ** be written to the database (since we crash while holding the mutex).
    **
    ** If we are not logging (just doing a load operation), then we can
    ** return an error and presumably let the trashed pages be tossed.
    */

    if (critical_section)
	dmd_check(E_DM93B9_BXINSERT_ERROR);

    if (mutex_done)
	dm1m_unmutex(tbio, (i4)0, r->rcb_lk_id, &b);


    if ((dberr->err_code == 0) || (dberr->err_code > E_DM_INTERNAL))
    {
	if (dberr->err_code)
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	}
	SETDBERR(dberr, 0, E_DM93B9_BXINSERT_ERROR);
    }
    return (E_DB_ERROR);

} /* dm1mxadjust */


/*
** Name: dm1mxcompare - Perform a compare by minimum bounding rectangle
**
** Description:
** 	Examine the Rtree entry to see if its mbr:
** 	    overlaps,
** 	    intersects,
** 	    is inside, or
** 	    contains
** 	the search key.
**
** Inputs:
** 	adf_cb				ADF control block pointed to by rcb.
**	klv				Key-level vector, containing Hilbert
**					size, function pointers, etc.
**      direction                       Value to indicate the type of search:
**					  DM1C_OVERLAPS, DM1C_INTERSECTS,
**					  DM1C_INSIDE, DM1C_CONTAINS.
** 	isleaf				Flag whether this is on a leaf page.
**	key				The search key (MBR).
** 	keylen				Length of the search key (MBR).
** 	key_ptr				Pointer to the index key to test.
** 	compare				Pointer to a boolean for the compare
** 					  result.
**
** Outputs:
** 	compare				Pointer to compare result:
** 					  TRUE - the key_ptr qualifies,
** 					  FALSE - the key_ptr does not qualify.
**
** Returns:
**	E_DB_OK
**
** History:
**	25-nov-1996 (wonst02)
**	    Modified parameters of dm1mxcompare() to eliminate tcb.
*/
DB_STATUS
dm1mxcompare(
    ADF_CB 	 *adf_cb,
    DMPP_ACC_KLV *klv,
    i4      direction,
    bool	 isleaf,
    char         *key,
    i4	 keylen,
    char	 *key_ptr,
    bool	 *compare)
{
    DB_DATA_VALUE	search_mbr;
    DB_DATA_VALUE	index_mbr;
    DB_DATA_VALUE	dresult;
    i2			result;
    i4		op_type;
    DB_STATUS	      	s;

    search_mbr.db_data = key;
    search_mbr.db_length = keylen;
    search_mbr.db_datatype = klv->nbr_dt_id;
    search_mbr.db_prec = 0;
    search_mbr.db_collID = -1;
    index_mbr.db_data = key_ptr;
    index_mbr.db_length = keylen;
    index_mbr.db_datatype = klv->nbr_dt_id;
    index_mbr.db_prec = 0;
    index_mbr.db_collID = -1;
    dresult.db_data = (char *)&result;
    dresult.db_length = sizeof(result);
    dresult.db_datatype = DB_INT_TYPE;
    dresult.db_prec = 0;
    dresult.db_collID = -1;

    if (direction == DM1C_CONTAINS)	/* Indexed obj contains search obj */
        op_type = DM1C_CONTAINS;
    else if (direction == DM1C_INSIDE)	/* Indexed obj inside search obj */
        if (isleaf)
    	    op_type = DM1C_INSIDE;
        else
    	    op_type = DM1C_OVERLAPS;
    else
        op_type = DM1C_OVERLAPS;	/* Indexed obj overlaps search obj */

    if (op_type == DM1C_OVERLAPS || op_type == DM1C_INTERSECTS)
        /*
        **  Test if the two MBRs overlap each other
        */
        s = (*klv->dmpp_overlaps)
		(adf_cb, &search_mbr, &index_mbr, &dresult);
    else if (op_type == DM1C_INSIDE)
        /*
        **  Test if the index MBR is inside the search MBR
        */
        s = (*klv->dmpp_inside)
		(adf_cb, &index_mbr, &search_mbr, &dresult);
    else /* (op_type == DM1C_CONTAINS) */
        /*
        **  Test if the search MBR is inside the index MBR
        */
        s = (*klv->dmpp_inside)
		(adf_cb, &search_mbr, &index_mbr, &dresult);

    if (s == E_DB_OK)
    	if (result)
	    *compare = TRUE;
	else
	    *compare = FALSE;
    return s;
}

/*{
** Name: dm1mxdelete - Deletes a (key,tid) pair from a page.
**
** Description:
**	In doing this delete it must compress the line table, and may or may
**	not reclaim the record space. The space management is handled by the
**	dm1cx* layer.
**
**	If this is an index page, then the keyspace is not lost, but combined
**	with the next lower entry.  That is, if the previous line table entry
**	indicated the highest key on its associated leaf page was "abc", the
**	one being deleted indicated "def" then the previous entry is changed to
**	indicate it's key space includes "def".
**
**	When we are deleting an entry on an index page as a result of a join,
**	we can think of this as combining two entries. We want the tidp from
**	the previous entry combined with the key from this entry. Since
**	shuffling tidp's around can be done easily (they're all fixed length,
**	so we can do this in place), but moving keys around is hard (they may
**	be different sizes, so this may require a delete and an insert to
**	update a key), it is vastly preferable to implement this operation on
**	index pages where child > 0 as "delete the (child-1) entry, but save its
**	tidp and update the child entry to contain that tidp".
**
**	The log_updates flag should be set to TRUE for most invocations of
**	this routine.  It indicates that the index update needs to be logged
**	for recovery purposes.  It should be set to TRUE if the update
**	operation is logged (and recovered) in some other mechanism (such as
**	using Before and After Images on the updated pages) by the caller
**	of this routine.  Note that if the table or session has a nologging
**	attribute, then no log records will be written regardless of the
**	setting of this flag.
**
**	The mutex_requried flag should be set to TRUE for most invocations of
**	this routine.  It indicates that the index page passed in is shared
**	by other threads and must be protected while updated.  It can be
**	set to FALSE if the caller has a private copy of the page (eg during
**	build processing).
**
** Inputs:
**      rcb				Pointer to record access context
**					which contains table control
**					block (TCB) pointer, tran_id,
**					and lock information associated
**					with this request.
**	buffer				Pointer to the page containing
**					the record to delete.
**      child				A LINE_ID portaion of a TID which
**					points to the record to delete.
**	log_updates			Indicates to log the update.
**	mutex_required			Indicates to mutex page while updating.
**
** Outputs:
**	err_code			Set if this routine returns E_DB_ERROR:
**					E_DM93B7_BXDELETE_ERROR
**
**	Returns:
**
**	    E_DB_OK
**	    E_DB_ERROR
**
** History:
**	25-may-1996 (wonst02)
**          LGreserve adjustments for removal of bi_page from DM0L_BI
**          LGreserve adjustments for removal of ai_page from DM0L_AI
**	    Change all the page header access as macro for New Page Format
**	    project.
**	    Subtract size of tuple header from index entry length for leaf
**	    page only.
** 	29-jul-1996 (wonst02)
** 	    Subtract Hilbert size from key length for Rtree leaf pages.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Pass reclaim_space to dm1cxdel()
**      27-feb-97 (stial01)
**          Init flag param for dm1cxdel()
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      21-apr-98 (stial01)
**          dm1mxdelete() Set DM0L_PHYS_LOCK if extension table (B90677)
*/
DB_STATUS
dm1mxdelete(
    DMP_RCB	    *rcb,
    DMPP_PAGE	    *buffer,
    i4		    child,
    i4	    log_updates,
    i4	    mutex_required,
    DB_ERROR	    *dberr)
{
    DMPP_PAGE		*b = buffer;
    DMP_RCB		*r = rcb;
    DMP_TCB		*t = r->rcb_tcb_ptr;
    DMP_DCB     	*d = t->tcb_dcb_ptr;
    DMP_TABLE_IO	*tbio = &t->tcb_table_io;
    DB_STATUS		status = E_DB_OK;
    STATUS		cl_status;
    i4		update_mode;
    i4		dm0l_flag;
    i4		compression_type;
    i4		loc_config_id; i4		loc_id;
    i4		key_len;
    char		*key;
    LG_LSN		lsn;
    DM_TID		prevtid;
    DM_TID		delbid;
    DM_TID		deltid;
    i4			childkey;
    bool		log_required;
    bool		index_update;
    bool		mutex_done = FALSE;
    bool		critical_section = FALSE;
    CL_ERR_DESC		sys_err;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** Setup information needed for dm1cx calls and the log record write below.
    */
    delbid.tid_tid.tid_page = DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, b);
    delbid.tid_tid.tid_line = child;
    compression_type = DM1B_INDEX_COMPRESSION(r);
    index_update = ((DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, b) &
			DMPP_INDEX) != 0);
    log_required = (log_updates && r->rcb_logging);

    /*
    ** Initialize update mode.
    ** Non-leaf page updates are always direct.
    */
    if ((r->rcb_update_mode == RCB_U_DEFERRED) && ( ! index_update))
	update_mode = DM1C_DEFERRED;
    else
	update_mode = DM1C_DIRECT;

    /*
    ** Get information on the location to which the update is being made.
    */
    loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count,
		(i4)DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, b));
    loc_config_id = tbio->tbio_location_array[loc_id].loc_config_id;

    /*
    ** Validate the delete request.  Check that the requested entries
    ** exist on the given page and that they can be deleted.  We do this
    ** because it is very bad to find out AFTER having logged the update
    ** that the requested row does not exist or cannot be deleted.  Note
    ** that we do not expect such a condition to occur here (it would
    ** indicate a bug), but if we catch it before the dm0l call then the
    ** effect of the bug will be to cause the query to fail rather than to
    ** cause the database to go inconsistent.
    */
    {
	/*
	** If child not on this page, do nothing and return. We don't currently
	** treat this as an error.
	**
	** Note: since we don't really think this case should ever occur (and
	** it seems like it would be bad if it did), then write a warning
	** so we can track its occurrence.  We'd take it out but we're afraid.
	*/
	if (child >= DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type, b))
	{
	    uleFormat(NULL, E_DM9C1C_BTDELBYPASS_WARN, (CL_ERR_DESC*)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 3,
		0, child, 0, DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type, b),
		0, DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, b));
	    return (E_DB_OK);
	}

	/*
	** Adjust the key position for non-leaf page updates.
	** On index page deletes, the child value actually indicates
	** the position of the BID to remove.  They key at that position
	** will be preserved however (and collapsed down into the previous
	** Rtree slot).  The key being deleted is the one at (child - 1).
	**
	** If we are deleting from an index page then set the childkey
	** position back one slot (if there is one) to make sure that we
	** get the proper key to log.
	*/
	childkey = child;
	if ((index_update) && (child != 0))
	    childkey--;

	/*
	** Look up pointer to the delete victim's key.  In addition to
	** validating the existence of the entry, the key is needed in the
	** log call below.  Also get the length of the (possibly compressed)
	** key entry and its tidp.
	*/
	dm1cxrecptr(tbio->tbio_page_type, tbio->tbio_page_size, b, childkey, &key); 
	dm1cxtget(tbio->tbio_page_type, tbio->tbio_page_size, b, child, 
			&deltid, (i4*)NULL);

	/* Non-compressed keys: we just use the fixed size key length */
	key_len = t->tcb_klen - DM1B_TID_S;
	if (compression_type != DM1CX_UNCOMPRESSED)
	{
	    dm1cx_klen(tbio->tbio_page_type, tbio->tbio_page_size, b, childkey, &key_len);
	}
	else
	    if (DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, b) & DMPP_LEAF)
		key_len -= t->tcb_hilbertsize;

	if ((child >= DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type, b)) ||
	    (key < ((char*)b + DM1B_VPT_OVER(tbio->tbio_page_type))) ||
	    (key > ((char*)b + tbio->tbio_page_size)) ||
	    (key_len < 0) ||
	    (key_len > DB_MAXRTREE_KEY))
	{
	    uleFormat(NULL, E_DM9C1D_BXBADKEY, (CL_ERR_DESC*)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 5,
		0, child, 0, DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, b),
		0, DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type, b), 0, key_len,
		0, (key - (char *)b));
	    dmd_prindex(rcb, b, 1);
	    uleFormat(NULL, E_DM93B7_BXDELETE_ERROR, (CL_ERR_DESC*)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM93B7_BXDELETE_ERROR);
	    return (E_DB_ERROR);
	}
    }

    for (;;)
    {
	/*
	** Online Backup Protocols: Check if BI must be logged before update.
	*/
	if (d->dcb_status & DCB_S_BACKUP)
	{
	    status = dm0p_before_image_check(r, b, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** Reserve the log space for the Rtree index delete log record and for
	** its CLR should the transaction be rolled back.
	** This must be done previous to mutexing the page since the reserve
	** request may block.
	*/
	if (log_required)
	{
	    /* Reserve space for log and CLR  */
	    cl_status = LGreserve(0, r->rcb_log_id, 2,
		(2 * (sizeof(DM0L_RTDEL) - (DB_MAXRTREE_KEY - key_len))),
		&sys_err);
	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			err_code, 1, 0, r->rcb_log_id);
		SETDBERR(dberr, 0, E_DM93B7_BXDELETE_ERROR);
		break;
	    }
	}

	/*
	** If the buffer resides in the DBMS buffer cache, then we have to mutex
	** it while we change it to prevent other threads from looking at it
	** while it is inconsistent.
	*/
	if (mutex_required)
	{
	    dm1m_mutex(tbio, (i4)0, r->rcb_lk_id, &b);
	    mutex_done = TRUE;
	}

	/*
	** Log the Delete to the Rtree Index Page.
	** The LSN of the log record must be written to the updated page.
	** If the log write fails then return without updating the page.
	*/
	if (log_required)
	{
	    dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);

	    if ( r->rcb_state & RCB_IN_MINI_TRAN )
	        dm0l_flag |= DM0L_MINI;

	    /* 
	    ** We use temporary physical locks for catalogs, extension tables
	    ** and index pages. The same lock protocol must be observed during 
	    ** recovery.
	    */
	    if (t->tcb_sysrel || t->tcb_extended || index_update)
		dm0l_flag |= DM0L_PHYS_LOCK;

	    status = dm0l_rtdel(r->rcb_log_id, dm0l_flag,
		&t->tcb_rel.reltid, 
		&t->tcb_rel.relid, t->tcb_relid_len,
		&t->tcb_rel.relowner, t->tcb_relowner_len,
		t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
	        t->tcb_index_rac.compression_type,
		t->tcb_rel.relloccount, loc_config_id,
		t->tcb_acc_klv->hilbertsize, t->tcb_acc_klv->obj_dt_id,
		&delbid, &deltid, key_len, key,
		(r->rcb_ancestor_level + 1) * sizeof(DM_TID), r->rcb_ancestors,
		(LG_LSN *) 0, &lsn, dberr);
	    if (status != E_DB_OK)
		break;

	    /*
	    ** Now that the update has been logged, we must succeed in changing
	    ** the page.  If we encounter an error now, we must crash and let
	    ** the page contents be rebuilt via REDO recovery.
	    */
	    critical_section = TRUE;

	    /*
	    ** Write the LSN of the insert record to the page being updated.
	    ** This marks the version of the page for REDO processing.
	    */
	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, b, lsn);
        }


	/*
	** Proceed with the delete.  The delete semantics depend on whether
	** this is a leaf or index page.
	**
	** For leaf deletes, simply delete the entry at the 'child' position.
	**
	** INDEX (NON-LEAF) PROCESSING:
	**
	**     If this is an index page, then we are actually combining the
	**     child and (child-1) entries by deleting the tid value from the
	**     'child' position and the key value from the previous position.
	**
	**     As long as child > 0, then we want the key from the child entry
	**     and the tidp from the (child-1) entry to end up as the new
	**     (child-1) entry.
	**
	**     If child == 0, however, then there is no previous entry to
	**     combine with and we simply delete the specified child entry.
	**
	*/
	childkey = child;
	if ((index_update) && (child > 0))
	{
	    /*
	    ** We are merging two child pages, and are deleting the entry
	    ** for the right child from the parent. In this case, the
	    ** entries for BOTH children are now found by following the
	    ** left child's pointer, so we need to combine the entries to
	    ** result in a new entry which indicates that all entries <=
	    ** the right child's key are now found in the left child. That
	    ** is, we must end up with the right child's key and the left
	    ** child's tidp, and must delete one of the entries.
	    **
	    ** Since it's dramatically easier to manipulate tidps than to
	    ** manipulate keys (which may be compressed), we implement
	    ** this as:
	    **     extract the (child-1) entry's tidp
	    **     update the (child) entry's tidp with this value
	    **     delete the (child-1) entry
	    */
	    dm1cxtget(tbio->tbio_page_type, tbio->tbio_page_size, b, (child-1),
			&prevtid, (i4*)NULL );
	    status = dm1cxtput(tbio->tbio_page_type, tbio->tbio_page_size, 
				b, child, &prevtid, (i4)0 );
	    if (status != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93EB_BAD_INDEX_TPUT, r, b,
			    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, child);
		SETDBERR(dberr, 0, E_DM93B7_BXDELETE_ERROR);
		break;
	    }

	    /* Set childkey to (child - 1) for dm1cxdel call below. */
	    childkey--;
	}

	/*
	** Delete the key,tid pair.
	** Use 'childkey' position which will either be the caller supplied
	** child position (if leaf page) or the adjusted position of the
	** delete-key.
	*/
	status = dm1cxdel(tbio->tbio_page_type, tbio->tbio_page_size, b,
			    update_mode, compression_type,
			    &r->rcb_tran_id, r->rcb_slog_id_id, r->rcb_seq_number,
			    (i4)DM1CX_RECLAIM, childkey);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E2_BAD_INDEX_DEL, r, b,
			   t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, child);
	    SETDBERR(dberr, 0, E_DM93B7_BXDELETE_ERROR);
	    break;
	}

	/*
	** Unmutex the leaf page now that we are finished updating it.
	*/
	if (mutex_required)
	    dm1m_unmutex(tbio, (i4)0, r->rcb_lk_id, &b);

	return (E_DB_OK);
    }

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM93B7_BXDELETE_ERROR);
    }

    /*
    ** If an error occurred while updating the index page leaving the page's
    ** contents possibly trashed.  If we are logging updates, then we're
    ** hosed.  We crash the server and hope that REDO recovery will correctly
    ** rebuild the page contents.  Note that our inconistent page cannot
    ** be written to the database (since we crash while holding the mutex).
    **
    ** If we are not logging (just doing a load operation), then we can
    ** return an error and presumably let the trashed pages be tossed.
    */
    if (critical_section)
	dmd_check(E_DM93B7_BXDELETE_ERROR);

    if (mutex_done)
	dm1m_unmutex(tbio, (i4)0, r->rcb_lk_id, &b);

    return (E_DB_ERROR);
}

/*{
** Name: dm1mxdistribute - distribute overflowing entries onto sibling pages.
**
** Description:
** 	The overflow handling algorithm treats the overflowing nodes either
** 	by moving some of the entries to one of the s cooperating
** 	siblings or by splitting s nodes to s + 1 nodes. This function
** 	distributes the overflowing entries onto the siblings.
**
** 	The method is to start from the rightmost sibling, calculate the
** 	average number of kids per page, and move siblings from the left
** 	page(s) until this page has the average number. If this page cannot
** 	borrow enough kids from its left neighbor, it borrows from the next
** 	one to the left, and so on. If this page already has the average
** 	number of kids or more, skip it. The process repeats with the next
** 	page to the left, starting by recalculating the average number of
** 	kids per page.
**
** Inputs:
**      r				Pointer to record access context
**					which contains table control
**					block (TCB) pointer, tran_id,
**					and lock information associated
**					with this request.
** 	sibling				Array of pointers to cooperating
** 					sibling pages.
** 	siblings			Number of cooperating siblings.
** 	sibkids				Number of key-IDs of the siblings.
**	lsn				Log Sequence Number of overflow
**					mini-transaction.
**
** Outputs:
**      err_code                        Pointer to an area to return
**                                      the following errors when
**                                      E_DB_ERROR is returned:
**
** Returns:
**	E_DB_OK
**      E_DB_ERROR
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
** 	10-jun-1996 (wonst02)
**	    New.
**	13-dec-1996 (wonst02)
**	    Call dm1m_rcb_update after shifting entries between pages.
*/
DB_STATUS
dm1mxdistribute(
	DMP_RCB		*r,
	DMPP_PAGE	*sibling[],
	i4		numsiblings,
	i4		sibkids,
	LG_LSN	    	*lsn,
	DB_ERROR	*dberr)
{
	DMP_TCB		*t = r->rcb_tcb_ptr;	/* Table control block. */
    	ADF_CB	 	*adf_cb = r->rcb_adf_cb;
    	i4	    	compression_type = DM1B_INDEX_COMPRESSION(r);
    	DMPP_PAGE	*page,			/* Page pointers */
			*leftpage,
    			*rightpage;
    	char	    	lrange_buf[DB_MAXRTREE_KEY];
    	char	    	rrange_buf[DB_MAXRTREE_KEY];
    	char            *range[2];
    	i4	    	range_len[2];
    	DM_TID	    	infinity[2];
	bool		changed;
    	i4		right;			/* Sibling number */
    	i4		left;			/* Sibling number */
	i4		allkids = sibkids;	/* Kids of all siblings */
	i4		leftkids;		/* Kids of left page */
	i4		rightkids;		/* Kids of right page */
	i4		avgkids;		/* Average kids per sibling */
	i4		needkids;		/* Num of kids still needed */
	i4		split;			/* Split point */
	DM_TID		lbid;
	DM_TID		rbid;
    	i4		keylen;
    	i4	    	klen;			/* Key length (incl. TID) */
    	char	    	*keyptr;
    	char	    	keybuf[DB_MAXRTREE_KEY];
	DB_STATUS	s = E_DB_OK;

    CLRDBERR(dberr);

    page = sibling[0];
    klen = (DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, page)
	      & DMPP_LEAF) ? t->tcb_klen - t->tcb_hilbertsize
	      		   : t->tcb_klen;
    /*
    ** Loop through all the cooperating siblings, from right to left.
    */
    for (right = numsiblings - 1; right >= 0; right--)
    {
        rightpage = sibling[right];
        rightkids = DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, rightpage);
	avgkids = (allkids + right) / (right + 1);
	needkids = avgkids - rightkids;

        if (needkids > 0)
        {
	    /*
	    ** If this page needs more kids, start borrowing from its
	    ** neighbor(s) to the left, until the average is accumulated.
	    */
            for (left = right - 1; left >= 0; left--)
	    {
		leftpage = sibling[left];
                leftkids = DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype,
						  leftpage);
		if (leftkids == 0)
		    continue;
                split = (leftkids > needkids) ?
			 leftkids - needkids  :
			 0;
		/*
		** Move the appropriate entries from the left page to the
		** right page.
		*/
		s = dm1cxrshift(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
			leftpage, rightpage, DM1B_INDEX_COMPRESSION(r), 
			split, klen);
		if (s != E_DB_OK)
	    	    break;
		/*
		** Check for any oustanding RCB's that may have had their 
		** position pointers affected by our shuffling tuples around.
		** Note that entries on the right page are shifted right by 
		** passing a negative position (split - leftkids).
		*/
		lbid.tid_tid.tid_page = 
		      DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, leftpage);
		lbid.tid_tid.tid_line = 0;
		rbid.tid_tid.tid_page =
		      DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, rightpage);
		rbid.tid_tid.tid_line = 0;
		s = dm1m_rcb_update(r, &rbid, &rbid, split - leftkids, 
				    DM1C_MSPLIT, dberr);
		if (s != E_DB_OK)
	    	    break;
		s = dm1m_rcb_update(r, &lbid, &rbid, split, DM1C_MSPLIT, 
				    dberr);
		if (s != E_DB_OK)
	    	    break;
		needkids -= leftkids - split;
		if (needkids <= 0)
		    break;
	    } /* for (left = right - 1; left >= 0... */
	    if (s != E_DB_OK)
	    	break;
	} /* if (needkids > 0) */

	DM1B_VPT_SET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, rightpage, DMPP_MODIFY);
        allkids -= DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, rightpage);
    } /* for (right = numsiblings - 1; right... */

    for (left = 0; left <= numsiblings - 1; left++ )
    {
	leftpage = sibling[left];
        leftkids = DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype,
						  leftpage);
	/*
	** For either a leaf or index page, update the range keys and 
	** recompute the MBR. The last key on the page becomes the high 
	** range, and the high range of the previous page becomes the low.
	*/
	range[DM1B_LEFT] = lrange_buf;
	range[DM1B_RIGHT] = rrange_buf;
	get_range_entries(leftpage, NULL, t->tcb_acc_klv,
			  t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
			  range, range_len, infinity, dberr);
	if (left > 0)
	{
	    MEcopy(keyptr, keylen, range[DM1B_LEFT]);
	}
	if (left < numsiblings - 1)
	{
	    /*
	    ** Extract the last key on the page to become the high range of
	    ** this page and the low range of the next page.
	    */
    	    dm1cxrecptr(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, leftpage, 
		leftkids - 1, &keyptr);
    	    keylen = klen - DM1B_TID_S;
	    MEcopy(keyptr, keylen, keybuf);
	    keyptr = keybuf;
	    if (DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, leftpage)
			& DMPP_LEAF)
	    {
	    	/* On a leaf page, compute the Hilbert value & append it. */
     	    	s = dm1mxhilbert(adf_cb, t->tcb_acc_klv, keyptr,
	    		     	(char *)keyptr + keylen);
    	    	if (s != E_DB_OK)
	    	    break;
	    	keylen += t->tcb_hilbertsize;
	    }
	    MEcopy(keyptr, keylen, range[DM1B_RIGHT]);
	}
	/*
	** Recompute the total page MBR from all its entries.
	*/
	s = dm1mxpagembr(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
			leftpage, range[DM1B_RIGHT], adf_cb,
			t->tcb_acc_klv, 
			t->tcb_hilbertsize, &changed);
	if (s)
	    break;
	s = put_range_entries(leftpage, compression_type, t->tcb_hilbertsize,
			    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
			    &r->rcb_tran_id, r->rcb_slog_id_id,
			    r->rcb_seq_number, range, range_len, infinity);
	if (s)
	    break;
    }

    return s;
} /* dm1mxdistribute */


/*{
** Name: dm1mxformat - Format an Rtree page.
**
** Description:
**	This routine formats an empty Rtree page. For Rtree index pages
**	this routines calls the index formatting routine.
**
** Inputs:
**      page_type                       Page type for this rtree.
**      page_size                       Page size for this rtree.
**      rcb				Pointer to record access context
**					which contains table control
**					block (TCB) pointer, tran_id,
**					and lock information associated
**					with this request.
**					MAY BE NULL if no rcb available, in
**					which case use zero tranid and seq num.
**	tcb				Pointer to table control block. Even
**					when rcb is NULL, tcb must be
**					supplied.
**	buffer				Pointer to the page to format.
**	kperpage			Value indicating maximum keys per page.
**	klen				Value indicating key length
**					    (including the TID size).
**	page				The PAGENO part of a TID which
**					indicates the logical page number
**					of page formatting.
**	pagetype			Value indicating type of page format:
**					    DM1B_PINDEX  non-leaf index
**					    DM1B_PSPRIG  sprig (just above leaf)
**					    DM1B_PLEAF   leaf
**	compression_type		Value indicating index compression:
**					    DM1CX_UNCOMPRESSED
**					    DM1CX_COMPRESSED
**
** Outputs:
**	err_code			Set to an error code if an error occurs
**
**	Returns:
**
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	25-may-1996 (wonst02)
**	    Change all the page header access as macro for New Page Format
**	    project.
**	    Added loc_plv pointer parameter and pass to dm1cxformat().
**      12-jun-1997 (stial01)
**          Pass new attribute, key parameters to dm1cxformat
**          RTREE does not need key description information on leaf pages.
**	07-Jan-2004 (jenjo02)
**	    Force Rtree tidsize to sizeof(DM_TID) until (if ever)
**	    Global Rtree indexes are supported.
*/
DB_STATUS
dm1mxformat(
    i4			page_type,
    i4			page_size,
    DMP_RCB             *rcb,
    DMP_TCB             *tcb,
    DMPP_PAGE          *buffer,
    i4             kperpage,
    i4             klen,
    DM_PAGENO           page,
    i4             indexpagetype,
    i4		compression_type,
    DMPP_ACC_PLV	*loc_plv,
    DB_ERROR		*dberr)
{
    DMP_RCB	    *r = rcb;
    DMPP_PAGE	    *b = buffer;
    DB_TRAN_ID	    temp_tran_id;
    i4	    sequence_number;
    DB_TRAN_ID	    *tran_id;
    DB_STATUS	    s;

    CLRDBERR(dberr);

    DM1B_VPT_SET_PAGE_MAIN_MACRO(page_type, b, 0);
    DM1B_VPT_SET_PAGE_OVFL_MACRO(page_type, b, 0);

    DM1B_VPT_SET_PAGE_PAGE_MACRO(page_type, b, page);

    if (rcb != 0)
    {
	tran_id = &rcb->rcb_tran_id;
	sequence_number = rcb->rcb_seq_number;
    }
    else
    {
	MEfill(sizeof(DB_TRAN_ID), '\0', (PTR) &temp_tran_id);
	tran_id = &temp_tran_id;
	sequence_number = 0;
    }

    s = dm1cxformat(page_type, page_size, b, 
		    loc_plv, compression_type,
		    kperpage, klen - DM1B_TID_S,
		    (DB_ATTS **)NULL, (i4)0, (DB_ATTS **)NULL, (i4)0,
		    page, indexpagetype, TCB_RTREE,
		    DM1B_TID_S, 0);

    if (s != E_DB_OK)
    {
	dm1cxlog_error( E_DM93E5_BAD_INDEX_FORMAT, r, b,
			page_type, page_size, (DM_LINE_IDX)0 );
	SETDBERR(dberr, 0, E_DM93B8_BXFORMAT_ERROR);
    }
    return (s);
}

/*
** Name: dm1mxhilbert - Return the Hilbert value of this entry
**
** Description:
** 	Calculate the Hilbert value from the MBR
** 	(minimum bounding rectangle) of the entry (MBR).
**
** Inputs:
** 	adf_cb				ADF control block pointed to by rcb.
**	klv				Key-level vector, containing Hilbert
**					size, function pointers, etc.
** 	key_ptr				Pointer to key containing MBR/LHV.
**
** Outputs:
** 	hilbert				Pointer to an area where the Hilbert
** 					is returned.
**
** Returns:
**	    E_DB_OK
**	    E_DB_ERROR			Error calculating the Hilbert value.
**
** Exceptions:
** 	None.
**
** Side Effects:
** 	None.
**
*/
DB_STATUS
dm1mxhilbert(
    ADF_CB		*adf_cb,
    DMPP_ACC_KLV    	*klv,
    char		*key_ptr,
    char		*hilbert)
{

    DB_DATA_VALUE	key_mbr;
    DB_DATA_VALUE	key_hilbert;

	/*
	** compute the Hilbert value from the MBR (minimum bounding rect.).
	*/
	key_mbr.db_data = key_ptr;
	key_mbr.db_length = 2 * klv->hilbertsize;
	key_mbr.db_datatype = klv->nbr_dt_id;
	key_mbr.db_prec = 0;
	key_mbr.db_collID = -1;
	key_hilbert.db_data = hilbert;
	key_hilbert.db_length = klv->hilbertsize;
	key_hilbert.db_datatype = DB_BYTE_TYPE;
	key_hilbert.db_prec = 0;
	key_hilbert.db_collID = -1;
	return (*klv->dmpp_hilbert)(adf_cb, &key_mbr, &key_hilbert);

} /* dm1mxhilbert( */


/*{
** Name: dm1mxinsert - Insert a (key,tid) pair into an Rtree index page.
**
** Description:
**      This routine inserts an index record into an index page.
**      The record is a (key,tid) pair.  (Note that a key of an Rtree is
**	an n-dimensional mbr, rather than a scalar.) The position of where
**      to insert the record is indicated with a line table identifier.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**					MAY BE NULL if no rcb available, in
**					which case the update mode is assumed
**					to be direct.
**	tcb				Pointer to table control block. Even
**					when rcb is NULL, tcb must be
**					supplied.
**	page_size			Page size for this Rtree index
**	rac				Pointer to DMP_ROWACCESS struct that
**					describes the key entry.
**	klen				Length of key entry for this table.
**					This length should NOT include the tid
**					portion of the (key,tid) pair. We will
**					allocate an additional DM1B_TID_S amount
**					of space in this routine.
**	buffer				Pointer to the page to format.
**	key				Pointer to an area containing the key:
**					on a leaf page, an mbr
**					(minimum bounding region);
**					on a non-leaf page, an mbr + lhv
**					(largest Hilbert value).
**	tid				Pointer to an area containing
**					the tid part of the record.
**	child				The LINEID part of a TID used
**					to indicate where on this page the
**					insertion is to take place.
**	log_updates			Indicates to log the update.
**	mutex_required			Indicates to mutex page while updating.
**
** Outputs:
**	err_code			Set to an error code if an error occurs
**
**	Returns:
**
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	25-may-1996 (wonst02)
**	    Changed all page header access as macro for New Page Format
**	    project.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      21-apr-98 (stial01)
**          dm1mxinsert() Set DM0L_PHYS_LOCK if extension table (B90677)
*/
DB_STATUS
dm1mxinsert(
    DMP_RCB	    *rcb,
    DMP_TCB	    *tcb,
    i4	    page_type,
    i4	    page_size,
    DMP_ROWACCESS   *rac,
    i4	    klen,
    DMPP_PAGE	    *buffer,
    char	    *key,
    DM_TID	    *tid,
    i4	    	    child,
    i4	    log_updates,
    i4	    mutex_required,
    DB_ERROR	    *dberr)
{
    DMP_RCB		*r = rcb;
    DMP_DCB     	*d = tcb->tcb_dcb_ptr;
    DMP_TABLE_IO	*tbio = &tcb->tcb_table_io;
    DMPP_PAGE		*b = buffer;
    DM_TID		temptid;
    DM_TID		bid;
    DB_STATUS		status = E_DB_OK;
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    i4		update_mode;
    i4		dm0l_flag;
    i4		loc_config_id;
    i4		loc_id;
    char		compressed_key [DB_MAXRTREE_KEY];
    i4		crec_size;
    DB_TRAN_ID		*tran_id;
    u_i2		lg_id;
    LG_LSN		lsn;
    i4		sequence_number;
    i4		reserve_size;
    i4		compression_type;
    bool		index_update;
    bool		logging_needed;
    bool		mutex_done;
    bool		critical_section;
    i4		    *err_code = &dberr->err_code;

    /*
    ** This routine is sometimes called without an RCB argument.  If the
    ** logging or mutex options are specified, this better not be one of
    ** those cases.  If it is, we will certainly blow up.
    */
    if ((r == NULL) && ((log_updates) || (mutex_required)))
    {
	TRdisplay("dm1mxinsert called with zero rcb: bad arguments\n");
	SETDBERR(dberr, 0, E_DM93B9_BXINSERT_ERROR);
	return (E_DB_ERROR);
    }

    index_update = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, b) & DMPP_INDEX) != 0);
    logging_needed = ((log_updates) && (r->rcb_logging));

    /*
    ** Since some of the error paths in this routine do not set an error_code,
    ** we zero out the err_code here so that we can tell when it has a valid
    ** value.
    */
    CLRDBERR(dberr);

    /* set up dm1cx-style "compression type", really yes/no, but whatever... */
    compression_type = DM1CX_UNCOMPRESSED;
    if (rac->compression_type != TCB_C_NONE)
	compression_type = DM1CX_COMPRESSED;

    /*
    ** If inserting to a parent index node as part of a split operation,
    ** save the page pointer at the current insert position.  An insert
    ** to a non-Leaf page actually updates two entries on the page.
    ** The new key is inserted to (possibly the middle of) the page, and
    ** all entries after it are moved down.  But the new page pointer (TID)
    ** is inserted into the next position, as in this example:
    **
    **	 Before Insert	       	 After Insert
    **
    ** LHV 20  TID (10,0)      LHV 20  TID (10,0)
    ** LHV 30  TID (15,0)----->LHV 25  TID (15,0)	(new key, old tid)
    ** LHV 40  TID (20,0)  \-->LHV 30  TID (21,0)	(old key, new tid)
    ** -----------------       LHV 40  TID (20,0)
    **
    ** After calling dm1mxinsert, the caller must recompute the page's MBR.
    */
    if (index_update)
    {
	/*
	** Save tid of the original page that was split (i.e. one inserting
	** next to).  This value needs to be placed with new key.
	*/
        dm1cxtget(page_type, page_size, b, child, &temptid, (i4*)NULL);
    }


    /*
    ** Get session/statement information for the following updates.
    ** If called during LOAD operations (no rcb available) then no
    ** transaction id or sequence number is written to the page.
    */
    if (r)
    {
	if (r->rcb_update_mode == RCB_U_DEFERRED)
	    update_mode = DM1C_DEFERRED;
	else
	    update_mode = 0;
	tran_id = &r->rcb_tran_id;
	lg_id = r->rcb_slog_id_id;
	sequence_number = r->rcb_seq_number;
    }
    else
    {
	update_mode = 0;
	tran_id = (DB_TRAN_ID *)NULL;
	lg_id = 0;
	sequence_number = 0;
    }

    /*
    ** If the index uses compressed key entries then compress the key
    ** to be inserted.
    */
    if (rac->compression_type != TCB_C_NONE)
    {
	status = dm1cxcomp_rec(rac, key, klen,
				compressed_key, &crec_size );
	if (status != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E1_BAD_INDEX_COMP, r, b, 
		page_type, page_size, child );
	    SETDBERR(dberr, 0, E_DM93B9_BXINSERT_ERROR);
	    return (status);
	}

	key = compressed_key;
	klen = crec_size;
    }

    /*
    ** Get information on the location to which the update is being made.
    */
    loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count,
		(i4)DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b));
    loc_config_id = tbio->tbio_location_array[loc_id].loc_config_id;
    bid.tid_tid.tid_page = DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b);
    bid.tid_tid.tid_line = child;

    /*
    ** Validate the insert request.  Check that the space exists for the
    ** requested entry and that the insert can be done.  We do this
    ** because it is very bad to find out AFTER having logged the update
    ** that the row cannot be inserted.  Note that we do not expect such a
    ** condition to occur here (it would indicate a bug), but if we catch
    ** it before the dm0l call then the effect of the bug will be to cause
    ** the query to fail rather than to cause the database to go inconsistent.
    **
    ** (Note that the validation of child value below occurs before the child
    ** bt_kids value is incremented to include the new row being inserted, so
    ** we check for child > bt_kids rather than >=)
    **
    ** This check is bypassed when there is no rcb.  In this case we are
    ** being called from LOAD and the tcb information is not correct (#!%*&@).
    */
    if (r)
    {
    	i4	    kperpage;

    	if (DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, b)
	    & DMPP_LEAF)
	    kperpage = tcb->tcb_kperleaf;
	else
	    kperpage = tcb->tcb_kperpage;
	if ((klen < 0) || (klen > DB_MAXRTREE_KEY) ||
	    (child > DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b)) ||
	    (!dm1cxhas_room(page_type, page_size, b, compression_type, (i4)100,
		   	    kperpage, klen + DM1B_TID_S)))

	{
	    uleFormat(NULL, E_DM9C1E_BXKEY_ALLOC, (CL_ERR_DESC*)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 4,
		0, child, 0, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b),
		0, DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b), 0, klen);
	    dmd_prindex(rcb, b, 1);
	    SETDBERR(dberr, 0, E_DM93B9_BXINSERT_ERROR);
	    return (E_DB_ERROR);
	}
    }


    mutex_done = FALSE;
    critical_section = FALSE;
    for (;;)
    {
	/*
	** Online Backup Protocols: Check & if required: log BI before update.
	** Bypass when insert is called by build routines.
	*/
	if ((r) && (d->dcb_status & DCB_S_BACKUP))
	{
	    status = dm0p_before_image_check(r, b, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** Reserve logfile space for the insert record and its CLR.  This
	** ensures that space will be available for abort processing of
	** this update.  We reserve the space before mutexing the page to
	** avoid stalling (if logfull) while holding any page mutexes.
	*/
	if (logging_needed)
	{
	    /* Don't need space for key in the CLR. */
	    reserve_size = 2 * (sizeof(DM0L_RTPUT) - (DB_MAXRTREE_KEY - klen));
	    reserve_size -= klen;

	    cl_status = LGreserve(0, r->rcb_log_id, 2, reserve_size, &sys_err);
	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			err_code, 1, 0, r->rcb_log_id);
		status = E_DB_ERROR;
		break;
	    }
	}

	/*
	** If the buffer resides in the DBMS buffer cache, then we have to mutex
	** it while we change it to prevent other threads from looking at it
	** while it is inconsistent.
	*/
	if (mutex_required)
	{
	    dm1m_mutex(tbio, (i4)0, r->rcb_lk_id, &b);
	    mutex_done = TRUE;
	}

	/*
	** Log the Insert to the Rtree Index Page.
	** The LSN of the log record must be written to the updated page.
	*/
	if (logging_needed)
	{
	    dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);

	    if ( r->rcb_state & RCB_IN_MINI_TRAN )
	        dm0l_flag |= DM0L_MINI;

	    /* 
	    ** We use temporary physical locks for catalogs, extension tables
	    ** and index pages. The same lock protocol must be observed during 
	    ** recovery.
	    */
	    if (tcb->tcb_sysrel || tcb->tcb_extended || index_update)
		dm0l_flag |= DM0L_PHYS_LOCK;

	    status = dm0l_rtput(r->rcb_log_id, dm0l_flag, &tcb->tcb_rel.reltid,
		&tcb->tcb_rel.relid, tcb->tcb_relid_len,
		&tcb->tcb_rel.relowner, tcb->tcb_relowner_len,
		tcb->tcb_rel.relpgtype, tcb->tcb_rel.relpgsize,
		rac->compression_type,
		tcb->tcb_rel.relloccount, loc_config_id,
		tcb->tcb_acc_klv->hilbertsize, tcb->tcb_acc_klv->obj_dt_id,
		&bid, tid, klen, key,
		(r->rcb_ancestor_level + 1) * sizeof(DM_TID), r->rcb_ancestors,
		(LG_LSN *) 0, &lsn, dberr);
	    if (status != E_DB_OK)
		break;

	    /*
	    ** Now that we have logged the update, we have committed ourselves
	    ** to being able to process the action.  Any failure now causes us
	    ** to crash the server to let REDO recovery attempt to fix the db.
	    */
	    critical_section = TRUE;

	    /*
	    ** Write the LSN of the insert record to the page being updated.
	    ** This marks the version of the page for REDO processing.
	    */
	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, b, lsn);
	}


	/*
	** Allocate space on the page for the new key,tid pair.
	*/
        status = dm1cxallocate(page_type, page_size, b, 
			update_mode, compression_type, tran_id,
			sequence_number, child, klen + DM1B_TID_S );

	if (status != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E0_BAD_INDEX_ALLOC, r, b, 
		page_type, page_size, child );
	    break;
	}

	/*
	** Insert the new key,tid values.
	*/
        status = dm1cxput(page_type, page_size, b,
			compression_type, update_mode, tran_id, lg_id,
			sequence_number, child, key, klen,
			tid, (i4)0 );

	if (status != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E4_BAD_INDEX_PUT, r, b, 
		    page_type, page_size, child );
	    break;
	}

	/*
	** If the insert is to a non-leaf index page then adjust the TID
	** pointers on the new entry and the old one which occupied its
	** spot (as described above).
	*/
	if (index_update)
	{
	    /*
	    ** The tidp of the old 'child' record, saved in temptid, goes
	    ** with the new child record we just inserted.  The tidp passed
	    ** to us goes with the (child+1) entry.
	    */
	    status = dm1cxtput(page_type, page_size, b, child + 1, 
				tid, (i4)0 );
	    if (status != E_DB_OK)
	    {
		dm1cxlog_error( E_DM93EB_BAD_INDEX_TPUT, r, b,
				page_type, page_size, child + 1 );
		break;
	    }
	    status = dm1cxtput(page_type, page_size, b, child, 
				&temptid, (i4)0 );
	    if (status != E_DB_OK)
	    {
		dm1cxlog_error( E_DM93EB_BAD_INDEX_TPUT, r, b,
				page_type, page_size, child );
		break;
	    }
	}
	else if (r && (r->rcb_update_mode == RCB_U_DEFERRED))
	{
	    critical_section = FALSE;
	    status = dm1cx_dput(r, b, child, tid, err_code);
	    if (status != E_DB_OK)
	    {
		SETDBERR(dberr, 0, *err_code);

		dm1cxlog_error( E_DM93E4_BAD_INDEX_PUT, r, b, 
			page_type, page_size, child );
		break;
	    }
	}

	/*
	** Unmutex the leaf page now that we are finished updating it.
	*/
	if (mutex_done)
	    dm1m_unmutex(tbio, (i4)0, r->rcb_lk_id, &b);

	return (E_DB_OK);
    }

    /*
    ** An error occurred while updating the index page leaving the page's
    ** contents possibly trashed.  If we are logging updates, then we're
    ** hosed.  We crash the server and hope that REDO recovery will correctly
    ** rebuild the page contents.  Note that our inconistent page cannot
    ** be written to the database (since we crash while holding the mutex).
    **
    ** If we are not logging (just doing a load operation), then we can
    ** return an error and presumably let the trashed pages be tossed.
    */

    if (critical_section)
	dmd_check(E_DM93B9_BXINSERT_ERROR);

    if (mutex_done)
	dm1m_unmutex(tbio, (i4)0, r->rcb_lk_id, &b);


    if ((dberr->err_code == 0) || (dberr->err_code > E_DM_INTERNAL))
    {
	if (dberr->err_code)
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	}
	SETDBERR(dberr, 0, E_DM93B9_BXINSERT_ERROR);
    }
    return (E_DB_ERROR);
} /* dm1mxinsert */


/*
** Name: dm1mxkkcmp - compare keys in an R-tree
**
** Description:
** 	Compare the keys in an R-tree entry. Since the R-tree index
** 	is maintained by Hilbert sequence, only the Hilbert value is used.
**
** Inputs:
**      page                            Pointer to the page.
** 	adf_cb				ADF control block pointed to by rcb.
**	klv				Key-level vector, containing Hilbert
**					size, function pointers, etc.
** 	key_ptr				Pointer to key containing MBR/LHV.
** 	searchkey			The search key to compare to.
**
** Outputs:
** 	compare				Pointer to a place for the result:
** 					  > 0 : key_ptr > searchkey
** 					  < 0 : key_ptr < searchkey
** 					  = 0 : keys are equal.
**
** Returns:
**	    E_DB_OK
**	    E_DB_ERROR			Error calculating the Hilbert value.
**
** Exceptions:
** 	None.
**
** Side Effects:
** 	None.
**
** History:
**       6-Jul-2004 (hanal04) Bug 112611 INGSRV2895
**          Correct ordering of pgtype and pgsize to match that of all
**          existing callers.
*/
DB_STATUS
dm1mxkkcmp(
    i4		page_type,
    i4		page_size,
    DMPP_PAGE  *page,
    ADF_CB	*adf_cb,
    DMPP_ACC_KLV *klv,
    char	*key_ptr,
    char	*searchkey,
    i4	    	*compare)
{
    u_i2	mbr_size = 2 * klv->hilbertsize;
    char	hilbert[DB_MAXRTREE_KEY];
    DB_STATUS	s;

    if (DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page) & DMPP_INDEX)
    {
    	/*
	** On an index page, copy the Hilbert value from the last part
	** of the key (MBR, LHV).
	*/
    	MEcopy((char *)key_ptr + mbr_size, klv->hilbertsize, hilbert);
    }
    else
    {
    	/*
	** On a leaf page, compute the Hilbert value from the MBR.
	*/
     	s = dm1mxhilbert(adf_cb, klv, key_ptr, hilbert);
    	if (s != E_DB_OK)
	    return s;
    }

    *compare = MEcmp(hilbert, (char *)searchkey + mbr_size, klv->hilbertsize);

    return E_DB_OK;
} /* dm1mxkkcmp */


/*{
** Name: dm1mxnewroot - Creates a new Rtree root due to split.
**
** Description:
**      This routines takes the old root, moves its data to a new
**      page and points root to it. Then it adds the sibling entry
**      to the root. It is assumed that the root of the tree and sibling
**      are locked by the caller. The header page is locked and
**      unlocked at the beginning and end of this routine assuring
**      no other thread(user) can be splitting root at same time.
**      This also serializes access, therefore no other pages need
**      to be locked during split.  It is also assumed that
**      only the root and sibling are fixed upon entry and exit.
**
**      The split occurs as follows:
**      New page is obtained, free list information and page number
**      of new page is saved for recovery. Old root is copied to
**      new page. New page is updated with it correct page number.
**      New page number and saved information is logged for recovery.
**      New page and header are written. Root is updated to indicate it
** 	has one record which points to new page and one to sibling.
** 	Root is written. Log indicating end of split is written.  Old
** 	root is now updated and is new root. New page and sibling are
** 	it's only children and new page has all old information on it.
**
** Inputs:
**      rcb				Pointer to record access context
**					which contains table control
**					block (TCB) pointer, tran_id,
**					and lock information associated
**					with this request.
**      root				Pointer to a pointer to current
**					root page.
** 	sibling				Pointer to the root's new sibling.
**
** Outputs:
**      err_code			Pointer to an area used to return
**					error codes if return value not
**					E_DB_OK.  Error codes returned are:
**					E_DM
**
**	Returns:
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-may-1996 (wonst02)
**          LGreserve adjustments for removal of bi_page from DM0L_BI
**          LGreserve adjustments for removal of ai_page from DM0L_AI
**	    Change all the page header access as macro for New Page Format
**	    project.
** 	27-jul-1996 (wonst02)
** 	    Modified for Rtree.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Pass reclaim_space to dm1cxdel()
**	10-jan-1997 (nanpr01)
**	    Put back the space holder for the page in DM0L_BI, DM0L_AI
**      27-feb-97 (stial01)
**          Init flag param for dm1cxdel()
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
*/
DB_STATUS
dm1mxnewroot(
    DMP_RCB	*rcb,
    DMPP_PAGE   **root,
    DB_ERROR    *dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_DCB         *d = t->tcb_dcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DMPP_PAGE	    *newroot = (DMPP_PAGE *)NULL;
    DMPP_PAGE	    *newpage, *rootpage;
    DM_TID	    son;
    DB_STATUS	    s = E_DB_OK;
    STATUS	    cl_status;
    LG_LSN	    lsn;
    i4	    compression_type;
    i4	    dm0l_flag;
    i4	    alloc_flag;
    i4	    child;
    i4	    loc_id;
    i4	    root_loc_cnf_id;
    i4	    indx_loc_cnf_id;
    i4	    local_error;
    bool	    pages_mutexed = FALSE;
    CL_ERR_DESC	    sys_err;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** Note: root is already exclusively locked by caller.
    */

    if (DMZ_AM_MACRO(5))
    {
	(VOID) TRdisplay("dm1mxnewroot: relation is %t\n",
                       sizeof(DB_TAB_NAME), &r->rcb_tcb_ptr->tcb_rel.relid);
	/* %%%% Place call to debug routine to print locks. */
    }

    compression_type = DM1B_INDEX_COMPRESSION(r);

    for (;;)
    {
	/*
	** Allocate the new root page.
	** The allocate call must indicate that it is performed from within
	** a mini-transation so that we will not be allocated any pages that
	** were previously deallocated in this same transaction.  (If we were
	** and our transaction were to abort it would be bad: the undo of the
	** deallocate would fail when it found the page was not currently free
	** since the split would not have been rolled back.)
	*/
	alloc_flag = (DM1P_MINI_TRAN | DM1P_PHYSICAL);

	s = dm1m_getfree(r, alloc_flag, &newroot, dberr);
	if (s != E_DB_OK)
	    break;

	/* Inform buffer manager of new page's type */
	dm0p_pagetype(tbio, newroot, r->rcb_log_id, DMPP_INDEX);

	newpage = newroot;
	rootpage = *root;

	/*
	** Save the new page number and build TID entry for the sole root page
	** entry (which will point to the new index page).
	*/
	son.tid_tid.tid_page =
		DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, newpage);
	son.tid_tid.tid_line = 0;

	/*
	** Get information on the locations to which the update is being made.
	*/
	loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count,
		(i4)DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type,
						  rootpage));
	root_loc_cnf_id = tbio->tbio_location_array[loc_id].loc_config_id;
	loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count,
		(i4)DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type,
						  newpage));
	indx_loc_cnf_id = tbio->tbio_location_array[loc_id].loc_config_id;


	/*
	** Online Backup Protocols: Check if BIs must be logged before update.
	*/
	if (d->dcb_status & DCB_S_BACKUP)
	{
	    s = dm0p_before_image_check(r, rootpage, dberr);
	    if (s != E_DB_OK)
		break;
	    s = dm0p_before_image_check(r, newpage, dberr);
	    if (s != E_DB_OK)
		break;
	}


	/************************************************************
	**            Log the Newroot operation.
	*************************************************************
	**
	** A NEWROOT operation is logged in two phases:  Before Image
	** Log Records are written prior to updating the pages.  These
	** are used only for UNDO processing.
	**
	** After Image Log Records are written after the operation is
	** complete to record the final contents of the pages.
	** These log records must be written BEFORE the mutexes are
	** released on the pages.  The LSNs of these log records are
	** written to the page_log_addr fields as well as to their split
	** addresses.
	*/

	if (r->rcb_logging & RCB_LOGGING)
	{
	    /* Physical locks are used on Rtree index pages */
	    dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);

	    if ( r->rcb_state & RCB_IN_MINI_TRAN )
	        dm0l_flag |= DM0L_MINI;

	    dm0l_flag |= DM0L_PHYS_LOCK;

	    /*
	    ** Reserve log file space for 2 BIs and their CLRs
	    */
	    cl_status = LGreserve(0, r->rcb_log_id, 4,
		4*(sizeof(DM0L_BI) - sizeof(DMPP_PAGE) + t->tcb_rel.relpgsize),
		&sys_err);
	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		  	(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			err_code, 1, 0, r->rcb_log_id);
		SETDBERR(dberr, 0, E_DM93BD_BXNEWROOT_ERROR);
		break;
	    }

	    /*
	    ** Mutex the pages before beginning the udpates.  This must be done
	    ** prior to the writing of the log record to satisfy logging
	    ** protocols.
	    */
	    pages_mutexed = TRUE;
	    dm1m_mutex(tbio, (i4)0, r->rcb_lk_id, &rootpage);
	    dm1m_mutex(tbio, (i4)0, r->rcb_lk_id, &newpage);

	    s = dm0l_bi(r->rcb_log_id, dm0l_flag,
		  &t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		  t->tcb_rel.relpgtype, t->tcb_rel.relloccount, root_loc_cnf_id,
		  DM0L_BI_NEWROOT, 0, t->tcb_rel.relpgsize,
		  rootpage,
		  (LG_LSN *) 0, &lsn, dberr);
            if (s != E_DB_OK)
		break;

	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, rootpage, lsn);

	    s = dm0l_bi(r->rcb_log_id, dm0l_flag,
		  &t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		  t->tcb_rel.relpgtype, t->tcb_rel.relloccount, root_loc_cnf_id,
		  DM0L_BI_NEWROOT,
		  DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, newpage),
		  t->tcb_rel.relpgsize,
		  newpage,
		  (LG_LSN *) 0, &lsn, dberr);
	    if (s != E_DB_OK)
		break;

	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, newpage, lsn);
	}
	else
	{
	    pages_mutexed = TRUE;
	    dm1m_mutex(tbio, (i4)0, r->rcb_lk_id, &rootpage);
	    dm1m_mutex(tbio, (i4)0, r->rcb_lk_id, &newpage);
	}

	/*
	** Copy old root to the new page.
	*/
	MEcopy((PTR)rootpage, tbio->tbio_page_size, (PTR)newpage);

	/*
	** Make new page point to itself.
	*/
	DM1B_VPT_SET_PAGE_PAGE_MACRO(tbio->tbio_page_type, newpage,
				 son.tid_tid.tid_page);
	DM1B_VPT_SET_PAGE_STAT_MACRO(tbio->tbio_page_type, newpage, DMPP_MODIFY);

	/*
	** Delete all entries (other than the LRANGE/RRANGE entries) on the
	** root page, thus reclaiming space if this index is compressed.
	** Allocate a new first entry and set it to point to the new page.
	** At the end of this, the root page is updated to contain only one
	** child, which is the new page. The new page, in turn, points to all
	** the former children of the root.
	**
	** Note that the root page is now no longer a sprig (it may not have
	** been a sprig before).
	*/
	DM1B_VPT_SET_BT_NEXTPAGE_MACRO(tbio->tbio_page_type,rootpage,(DM_PAGENO)0);
	DM1B_VPT_CLR_PAGE_STAT_MACRO(tbio->tbio_page_type, rootpage, DMPP_SPRIG);
	DM1B_VPT_SET_PAGE_STAT_MACRO(tbio->tbio_page_type, rootpage, DMPP_MODIFY);

	while (DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type, rootpage) > 0)
	{
	    child = DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type, rootpage) - 1;
	    s = dm1cxdel(tbio->tbio_page_type, tbio->tbio_page_size, rootpage,
		    DM1C_DIRECT, compression_type,
		    (DB_TRAN_ID *)NULL, (u_i2)0, (i4) 0,  (i4)DM1CX_RECLAIM, child);
	    if (s != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93E2_BAD_INDEX_DEL, r, rootpage,
			    tbio->tbio_page_type, tbio->tbio_page_size, child);
		SETDBERR(dberr, 0, E_DM93BD_BXNEWROOT_ERROR);
		break;
	    }
	}
	if (s != E_DB_OK)
	    break;

	/*
	** allocate a new entry to point to the new page and set the entry's
	** tid to point to the new page. The new entry has to be long
	** enough to contain a TID and a key (MBR).
	*/
	s = dm1cxallocate(tbio->tbio_page_type, tbio->tbio_page_size, rootpage,
			DM1C_DIRECT, compression_type, (DB_TRAN_ID *)NULL,
			(i4)0, (i4)0, t->tcb_klen);
	if (s != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E0_BAD_INDEX_ALLOC, r, rootpage,
			    tbio->tbio_page_type, tbio->tbio_page_size, (i4)0);
	    SETDBERR(dberr, 0, E_DM93BD_BXNEWROOT_ERROR);
	    break;
	}
	s = dm1cxtput(tbio->tbio_page_type, tbio->tbio_page_size, 
			rootpage, 0, &son, (i4)0);
	if (s != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93EB_BAD_INDEX_TPUT, r, rootpage,
			    tbio->tbio_page_type, tbio->tbio_page_size, (i4)0);
	    SETDBERR(dberr, 0, E_DM93BD_BXNEWROOT_ERROR);
	    break;
	}

	/************************************************************
	**            Log the Newroot operation.
	*************************************************************
	**
	** Complete the Logging for the operation by logging the After
	** images of the updated pages.  These will be used during forward
	** (redo,rollforward) recovery to reconstruct the pages.
	*/
	if (r->rcb_logging & RCB_LOGGING)
	{
	    if (pages_mutexed)
	    {
		dm1m_unmutex(tbio, (i4)0, r->rcb_lk_id, &rootpage);
		dm1m_unmutex(tbio, (i4)0, r->rcb_lk_id, &newpage);
		pages_mutexed = FALSE;
	    }

	    /* Physical locks are used on Rtree index pages */
	    dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);

	    if ( r->rcb_state & RCB_IN_MINI_TRAN )
	        dm0l_flag |= DM0L_MINI;

	    dm0l_flag |= DM0L_PHYS_LOCK;

	    /* Reserve log file space for 2 AIs */
	    cl_status = LGreserve(0, r->rcb_log_id, 2,
		2*(sizeof(DM0L_AI) - sizeof(DMPP_PAGE) + t->tcb_rel.relpgsize),
		&sys_err);
	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		  	(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			err_code, 1, 0, r->rcb_log_id);
		SETDBERR(dberr, 0, E_DM93BD_BXNEWROOT_ERROR);
		break;
	    }

	    dm1m_mutex(tbio, (i4)0, r->rcb_lk_id, &rootpage);
	    dm1m_mutex(tbio, (i4)0, r->rcb_lk_id, &newpage);
	    pages_mutexed = TRUE;

	    s = dm0l_ai(r->rcb_log_id, dm0l_flag,
		&t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		t->tcb_rel.relpgtype, t->tcb_rel.relloccount, root_loc_cnf_id,
		DM0L_BI_NEWROOT, 0, t->tcb_rel.relpgsize,
		rootpage,
		(LG_LSN *) 0, &lsn, dberr);
	    if (s != E_DB_OK)
		break;

	    s = dm0l_ai(r->rcb_log_id, dm0l_flag,
		&t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		t->tcb_rel.relpgtype, t->tcb_rel.relloccount, indx_loc_cnf_id,
		DM0L_BI_NEWROOT,
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, newpage),
		t->tcb_rel.relpgsize,
		newpage,
		(LG_LSN *) 0, &lsn, dberr);
	    if (s != E_DB_OK)
		break;

	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, rootpage, lsn);
	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, newpage, lsn);

	    DM1B_VPT_SET_BT_SPLIT_LSN_MACRO(tbio->tbio_page_type, rootpage, lsn);
	    DM1B_VPT_SET_BT_SPLIT_LSN_MACRO(tbio->tbio_page_type, newpage, lsn);
	}

	/*
	** Unmutex the pages since the update actions described by
	** the newroot log record are complete.
	**
	** Following the unmutexes, these pages may be written out
	** to the database at any time.
	*/
	pages_mutexed = FALSE;
	dm1m_unmutex(tbio, (i4)0, r->rcb_lk_id, &rootpage);
	dm1m_unmutex(tbio, (i4)0, r->rcb_lk_id, &newpage);


#ifdef xDEV_TEST
	if (DMZ_CRH_MACRO(18) || DMZ_CRH_MACRO(22) || DMZ_CRH_MACRO(22))
	{
	    i4	    n;
	    TRdisplay("dm1mxnewroot: simulating crashing during root split\n");
	    TRdisplay("    For table %t in database %t.\n",
		   sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
		   sizeof(DB_DB_NAME), &t->tcb_dcb_ptr->dcb_name);
	    if (DMZ_CRH_MACRO(23))
	    {
		(VOID) dm1m_unfix_page(r, DM0P_FORCE, root, dberr);
		TRdisplay("Old Root page Forced.\n");
	    }
	    if (DMZ_CRH_MACRO(18))
	    {
		(VOID) dm1m_unfix_page(r, DM0P_FORCE, root, dberr);
		(VOID) dm1m_unfix_page(r, DM0P_FORCE, &newroot, dberr);
		TRdisplay("New and Old Root pages Forced.\n");
	    }
	    CSterminate(CS_CRASH, &n);
	}
#endif

	/*
	** Unfix the new index page and leave the root page (page 0) fixed
	** by the caller.
	*/
	s = dm1m_unfix_page(r, DM0P_RELEASE, &newroot, dberr);
	if (s != E_DB_OK)
	    break;

	if (DMZ_AM_MACRO(5))
	{
	    TRdisplay("dm1mxnewroot: returning from newroot: son is %d\n",
			son.tid_tid.tid_page);
	}

	return (E_DB_OK);
    }

    /*
    ** Error handling:
    **
    */

    if (pages_mutexed)
    {
	dm1m_unmutex(tbio, (i4)0, r->rcb_lk_id, &rootpage);
	dm1m_unmutex(tbio, (i4)0, r->rcb_lk_id, &newpage);
    }

    if (newroot)
    {
	s = dm1m_unfix_page(r, DM0P_UNFIX, &newroot, &local_dberr);
	if (s != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM93BD_BXNEWROOT_ERROR);
    }

    return (E_DB_ERROR);
}

/*
** Name: dm1mxpagembr - calculate the composite MBR of the page.
**
** Description:
**	Look at the MBR portion of every entry on the page. Keep track
**	of the smallest and largest (x, y) values, which defines the
**	total MBR of the page.
**
** Inputs:
**      page                            Pointer to the page.
** 	rangekey			Range key containing current MBR.
** 	adf_cb				ADF control block pointed to by rcb.
**	klv				Key-level vector, containing Hilbert
**					size, function pointers, etc.
**
** Outputs:
** 	rangekey			The new composite MBR.
**	changed				Ptr to boolean flag:
**					    TRUE - MBR changed from rangekey
**					    FALSE - MBR did not change.
**
** Returns:
**	    E_DB_OK
**
** Exceptions:
** 	None.
**
** Side Effects:
** 	None.
**
*/
DB_STATUS
dm1mxpagembr(
	i4		page_type,
	i4		page_size,
    	DMPP_PAGE	*page,
	char		*rangekey,
	ADF_CB		*adf_cb,
	DMPP_ACC_KLV    *klv,
	u_i2		hilbertsize,
	bool		*changed)
{
	DB_STATUS	status = E_DB_OK;
	char		rangekeybuf[DB_MAXRTREE_KEY];
	DM_LINE_IDX	kid;
        i4		lastkid;
	DB_DATA_VALUE	rangekey_mbr;
	DB_DATA_VALUE	thiskey_mbr;

	/*
	** Find the composite MBR of the current leaf.  We will use this MBR
	** as the high range value of this leaf, the low range value for the
	** new leaf, and as the key to insert into the parent index for the
	** new leaf.
	*/
	*changed = FALSE;
	lastkid = DM1B_VPT_GET_BT_KIDS_MACRO(page_type, page);
	rangekey_mbr.db_data = rangekeybuf;
	rangekey_mbr.db_length = 2 * hilbertsize;
	rangekey_mbr.db_datatype = klv->nbr_dt_id;
	rangekey_mbr.db_prec = 0;
	rangekey_mbr.db_collID = -1;
	MEcopy(klv->null_nbr, rangekey_mbr.db_length,
	       rangekey_mbr.db_data);
	thiskey_mbr.db_length 	= rangekey_mbr.db_length;
	thiskey_mbr.db_datatype = rangekey_mbr.db_datatype;
	thiskey_mbr.db_prec 	= rangekey_mbr.db_prec;
	thiskey_mbr.db_collID 	= rangekey_mbr.db_collID;

	for (kid = 0; kid < lastkid; kid++)
	{
    	    dm1cxrecptr(page_type, page_size, page, kid, &thiskey_mbr.db_data);
	    /*
	    ** If thiskey is a non-null MBR, use it:
	    **   If the range key MBR is non-null,
	    **     compute the new composite range key;
	    **   otherwise, replace it w/this MBR.
	    */
	    if (MEcmp(thiskey_mbr.db_data, klv->null_nbr,
		      thiskey_mbr.db_length))
	    {
	    	if (MEcmp(rangekey_mbr.db_data, klv->null_nbr,
		      	  rangekey_mbr.db_length))
	    	    status = (*klv->dmpp_unbr)(adf_cb,
		    	      &rangekey_mbr, &thiskey_mbr, &rangekey_mbr);
	    	else
	    	    MEcopy(thiskey_mbr.db_data, rangekey_mbr.db_length,
		    	   rangekey_mbr.db_data);
	    }
	    if (status)
	    	break;
	} /* for (kid = 0; kid < lastkid... */

	if (status != E_DB_OK)
	    return status;

	*changed = (MEcmp(rangekey_mbr.db_data, rangekey,
	    	          rangekey_mbr.db_length) != 0);
	if (*changed)
	{
	    MEcopy(rangekey_mbr.db_data, rangekey_mbr.db_length, rangekey);
	}

	return status;
} /* dm1mxpagembr */


/*{
** Name: dm1mxsearch - Search for a key or a key,tid pair on a page.
**
** Description:
**      This routines searches an Rtree index page for a key or
**      key,tid pair. (Note that a key of an Rtree is
**	an n-dimensional mbr, rather than a scalar.)
**
**      This routine returns the line table position where the
**      search ended.  It also assumes that the page pointed to by
**      buffer is only one fixed on entry and exit.  This page
**      may change if search mode is DM1C_TID.
**
**      The semantics of this routine are determined by a combination of the
**	'mode' and 'direction' arguments. The following are the
**	only current legal combinations, together with their meaning:
**
**	DM1C_EXTREME/DM1C_PREV	    Find the lowest entry on the page. Used to
**				    position to the start of the Rtree for a
**				    scan.
**	DM1C_FIND/DM1C_EXACT	    Locate position by LHV on this page where
**				    this entry is located, or the position where
**				    it would be inserted if it's not found. If
**				    the page is empty, or if this entry is
**				    greater than all entries on this page, then
**				    return E_DM0056_NOPART.
**	DM1C_SPLIT/DM1C_EXACT	    Same semantics as FIND/EXACT. Used when the
**				    caller is in a splitting pass.
**	DM1C_OPTIM/DM1C_EXACT	    Same semantics as FIND/EXACT. Used when the
**				    caller is in an optimistic allocation pass.
** 	DM1C_RANGE/DM1C_OVERLAPS    Locate entry that overlaps this MBR.
** 	DM1C_RANGE/DM1C_INTERSECTS  Locate entry that intersects this MBR.
** 	DM1C_RANGE/DM1C_INSIDE      Locate entry that is inside this MBR.
** 	DM1C_RANGE/DM1C_CONTAINS    Locate entry that contains this MBR.
**	DM1C_TID/DM1C_EXACT	    Locate a specific entry on the page. The
**				    entry is located by comparing the TIDP of
**				    the entry on the page with the passed-in
**				    TID.
**
**	TID/EXACT only makes sense on a leaf page, whereas FIND/EXACT is used
**	for non-leaf index pages.
**
**	All other combinations of 'mode' and 'direction' are illegal and are
**	rejected.
**
** Inputs:
**      r				Pointer to record access context
**					(optional, used for log messages).
**	buffer				Pointer to the page to search.
**	mode				Value indicating type of search
**					to perform.
**	direction			Value to indicate the direction
**					of the search or range.
**	key				Pointer to the key value.
** 	adf_cb				ADF control block pointed to by rcb.
**	klv				Key-level vector, containing Hilbert
**					size, function pointers, etc.
**	tid				If mode == DM1C_TID, this value is the
**					TID which we use to search for the
**					exact entry to return.
**
** Outputs:
**	pos				Pointer to an area to return
**					the position of the line table
**					where search ended.
**	tid				Pointer to an area to return
**					the tid of the record pointed to
**					by line table position.
**	err_code			Pointer to an area to return
**					the following errors when
**					E_DB_ERROR is returned.
**					E_DM0056_NOPART
**					E_DM93BA_BXSEARCH_BADPARAM
**
**	Returns:
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    May alter the pages fixed if following leaf chain and get
**          fatal errors.
**
** History:
**      25-may-1996 (wonst02)
**	    Changed all the page header access as macro for New Page Format
**	    project.
**      03-june-1996 (wonst02)
**          Added DMPP_TUPLE_INFO argument to dm1cxget()
** 	27-jul-1996 (wonst02)
** 	    DM1C_TID mode returns first matching key if E_DM0056_NOPART.
**	25-nov-1996 (wonst02)
**	    Modified parameters of dm1mxsearch().
**      15-jun-1999 (hweho01)
**          Corrected argument mismatch in ule_format() call.
*/
DB_STATUS
dm1mxsearch(
    DMP_RCB	*r,
    DMPP_PAGE	*buffer,
    i4		mode,
    i4		direction,
    char	*key,
    ADF_CB 	*adf_cb,
    DMPP_ACC_KLV 	*klv,
    i4		pgtype,
    i4		pgsize,
    DM_TID	*tid,
    i4		*pos,
    DB_ERROR	*dberr)
{
    char	    *keypos;
    DM_TID	    buftid;
    DB_STATUS	    s;
    i4	    compare;
    DMPP_PAGE	    *page;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    s = E_DB_OK;
    page = buffer;

    switch (mode)
    {
    case DM1C_LAST:
	if (direction == DM1C_NEXT)
	{
	    if (DM1B_VPT_GET_BT_KIDS_MACRO(pgtype, page))
	    {
		*pos = DM1B_VPT_GET_BT_KIDS_MACRO(pgtype, page) - 1;
		dm1cxtget (pgtype, pgsize, page, (*pos), tid, (i4*)NULL);
	    }
	    else
		*pos = 0;
	}
	else
	{

	    uleFormat(NULL, E_DM93BA_BXSEARCH_BADPARAM, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    err_code, 2, 0, mode, 0, direction);
	    SETDBERR(dberr, 0, E_DM93BC_BXSEARCH_ERROR);
	    s = E_DB_ERROR;
	}
	break;


    case DM1C_EXTREME:

	/* For the extreme case get first entry on the page */

	if (direction == DM1C_PREV)
	{
	    *pos = 0;
	    if (*pos != DM1B_VPT_GET_BT_KIDS_MACRO(pgtype, page))
		dm1cxtget(pgtype, pgsize, page, (*pos), tid, (i4*)NULL );
	}
	else
	{
	    uleFormat(NULL, E_DM93BA_BXSEARCH_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			err_code, 2, 0, mode, 0, direction );
	    SETDBERR(dberr, 0, E_DM93BC_BXSEARCH_ERROR);
	    s = E_DB_ERROR;
	}
        break;

    case DM1C_FIND:
    case DM1C_SPLIT:
    case DM1C_OPTIM:

	if (direction == DM1C_NEXT)
	{
	    if (DM1B_VPT_GET_BT_KIDS_MACRO(pgtype, page))
	    {
		*pos = DM1B_VPT_GET_BT_KIDS_MACRO(pgtype, page) - 1;
	    	dm1cxtget(pgtype, pgsize, page, (*pos), tid, (i4*)NULL );
	    }
	    else
		*pos = 0;
	}
	if (direction != DM1C_EXACT)
	{
	    uleFormat(NULL, E_DM93BA_BXSEARCH_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			err_code, 2, 0, mode, 0, direction );
	    SETDBERR(dberr, 0, E_DM93BC_BXSEARCH_ERROR);
	    s = E_DB_ERROR;
	    break;
	}

	/*
        ** Binary search the page to find the position of a
        ** key that is greater than or equal to one specified.
        ** If find multiple matching entries, get lowest such entry.
        */

	s = binary_search(r, page, direction, key, adf_cb, klv, pgtype, pgsize,
			    pos, dberr);
	if (s != E_DB_OK)
	{
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM93BC_BXSEARCH_ERROR);
	    break;
	}

        if (*pos != DM1B_VPT_GET_BT_KIDS_MACRO(pgtype, page))
	    dm1cxtget(pgtype, pgsize, page, (*pos), tid, (i4*)NULL );
        break;

    case DM1C_RANGE:

	if (direction != DM1C_OVERLAPS && direction != DM1C_INTERSECTS &&
	    direction != DM1C_INSIDE   && direction != DM1C_CONTAINS)
	{
	    uleFormat(NULL, E_DM93BA_BXSEARCH_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			err_code, 2, 0, mode, 0, direction );
	    SETDBERR(dberr, 0, E_DM93BC_BXSEARCH_ERROR);
	    s = E_DB_ERROR;
	    break;
	}

	/*
        ** Search the page to find the position of a
        ** key that fits the criteria specified.
        ** If find multiple matching entries, get first such entry.
        */

	s = mbr_search(r, page, direction, key, adf_cb, klv, pgtype, pgsize, 
		       pos, dberr);
	if (s != E_DB_OK)
	{
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM93BC_BXSEARCH_ERROR);
	    break;
	}

        if (*pos != DM1B_VPT_GET_BT_KIDS_MACRO(pgtype, page))
	    dm1cxtget(pgtype, pgsize, page, (*pos), tid, (i4*)NULL );
        break;

    case DM1C_TID:
    {
    	i4	savepos;

	if (direction != DM1C_EXACT)
	{
	    uleFormat(NULL, E_DM93BA_BXSEARCH_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			err_code, 2, 0, mode, 0, direction );
	    SETDBERR(dberr, 0, E_DM93BC_BXSEARCH_ERROR);
	    s = E_DB_ERROR;
	    break;
	}

	/*
        ** Binary search the page to find the position of a
        ** key that is greater than or equal to one specified.
        ** If find multiple matching entries, get lowest or highest
        ** depending on direction.
        */

	s = binary_search(r, page, direction, key, adf_cb, klv, pgtype, pgsize,
			  pos, dberr );
	if (s != E_DB_OK)
	{
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM93BC_BXSEARCH_ERROR);
	    break;
	}

	/*
	** If the mode is DM1C_TID, then want the exact row matching the key
	** and TID values, not just the first entry matching the given key.
	** Search from this position onward, until we either locate the entry
	** whose TIDP matches the indicated TID or run out of entries.  
	*/
	savepos = *pos;
	for (; *pos < DM1B_VPT_GET_BT_KIDS_MACRO(pgtype, page); (*pos)++)
	{
	    dm1cxrecptr(pgtype, pgsize, page, *pos, &keypos);
	    s = dm1mxkkcmp(pgtype, pgsize, page, adf_cb, klv, keypos, key, &compare);
	    if (s != E_DB_OK)
	    {
		uleFormat(NULL, E_DMF012_ADT_FAIL, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		SETDBERR(dberr, 0, E_DM93BC_BXSEARCH_ERROR);
		return (s);
	    }
	    if (compare != 0)
	    {
		SETDBERR(dberr, 0, E_DM0056_NOPART);
		*pos = savepos;
		return (E_DB_ERROR);
	    }
	    dm1cxtget(pgtype, pgsize, page, *pos, &buftid, (i4*)NULL);
	    if (tid->tid_i4 == buftid.tid_i4)
		return (E_DB_OK);
	}
	SETDBERR(dberr, 0, E_DM0056_NOPART);
	*pos = savepos;
	return (E_DB_ERROR);
    }

    case DM1C_JOIN:
    case DM1C_POSITION:
    case DM1C_SETTID:
    default:
	uleFormat(NULL, E_DM93BA_BXSEARCH_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			err_code, 2, 0, mode, 0, direction );
	SETDBERR(dberr, 0, E_DM93BC_BXSEARCH_ERROR);
	s = E_DB_ERROR;
	break;
    } /* end case statement */

    if (s != E_DB_OK)
	return (s);

    if (*pos == DM1B_VPT_GET_BT_KIDS_MACRO(pgtype, page))
    {
	SETDBERR(dberr, 0, E_DM0056_NOPART);
        return (E_DB_ERROR);
    }
    return(E_DB_OK);
}

/*{
** Name: dm1mxsplit - Splits an Rtree index page.
**
** Description:
**	This routine takes an Rtree index page and splits it.
**	The sibling is locked after it is obtained from the free list.
**	At the end of dm1mxsplit, the sibling may have changed to
**	the current page passed down by the caller (depending on the
**	key value being inserted.)  Note: always splits to the right.
**
**	We perform split processing in two phases, because there are actually
**	two types of splits: index splits, in which an index (non-leaf) page
**	is split into two index pages, and leaf splits. This
**	routine performs the processing common to all the splits, and calls
**	either idxdiv or lfdiv to perform the type-specific split processing.
**
** Inputs:
**      rcb				Pointer to record access context
**					which contains table control
**					block (TCB) pointer, tran_id,
**					and lock information associated
**					with this request.
**      current                         Pointer to a pointer to a page to split.
** 	key				Key being inserted that caused split.
**      keyatts                         Pointer to a list describing
**                                      attributes of key.
** 	split				The line ID of the split position.
**
** Outputs:
** 	sibling				Ptr to area where new page ptr is put.
** 	split				Updated line ID of the split position.
** 	splitkey			The discriminator key at the split.
**      err_code                        Pointer to an area used to return
**                                      error codes if return value not
**                                      E_DB_OK.  Error codes returned are:
**                                      E_DM
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
**	    Can lock/unlock pages
**
** History:
**      25-may-1996 (wonst02)
**	    Change all the page header access as macro for New Page Format
**	    project.
**	    Added plv parameter to dm1bxformat routine call.
**      03-june-1996 (wonst02)
**          Added DMPP_TUPLE_INFO argument to dm1cxget()
** 	09-jul-1996 (wonst02)
** 	    Nearly a complete rewrite from the B-tree code.
**	21-sep-1996 (wonst02)
**	    Call dm1mxpagembr to compute both pages' MBRs after the split.
**	10-jan-1997 (nanpr01)
**	    Put back the space holder for the page in DM0L_BI, DM0L_AI
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      12-jun-97 (stial01)
**          Pass tlv to dm1cxget instead of tcb.
**      21-apr-98 (stial01)
**          dm1mxsplit() Set DM0L_PHYS_LOCK if extension table (B90677)
**	20-nov-2006 (wanfr01)
**	    SIR 117586
**	    changed arguments for dmd_prkey
*/
DB_STATUS
dm1mxsplit(
    DMP_RCB     *r,
    DMPP_PAGE   **current,
    DMPP_PAGE	**sibling,
    char	*key,
    DB_ATTS     *keyatts[],
    i4		*split,
    char	*splitkey,
    DB_ERROR    *dberr)
{
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_DCB         *d = t->tcb_dcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DMPP_PAGE	    *data = (DMPP_PAGE *)NULL;
    DMPP_PAGE	    *sibpage, *curpage;
    ADF_CB	    *adf_cb = r->rcb_adf_cb;
    DMP_ROWACCESS   *rac;
    LG_LSN	    lsn;
    DM_TID	    bid, newbid;
    DB_STATUS	    s = E_DB_OK;
    STATUS	    cl_status;
    CL_ERR_DESC	    sys_err;
    char	    *keyptr;
    char	    *disc_key = NULL;
    char	    *log_disc_key = NULL;
    char	    keybuf[DB_MAXRTREE_KEY];
    char	    compress_buf[DB_MAXRTREE_KEY];
    char	    lrange_buf[DB_MAXRTREE_KEY];
    char	    rrange_buf[DB_MAXRTREE_KEY];
    char            *range_keys[2];
    i4	    range_lens[2];
    DM_TID	    infinities[2];
    i4	    keylen;
    i4	    klen;
    i4	    kperpage;
    i4	    ptype = 0;
    i4	    compare;
    i4	    alloc_flag;
    i4	    dm0l_flag;
    i4	    compression_type = DM1B_INDEX_COMPRESSION(r);
    i4	    insert_direction;
    i4	    loc_id;
    i4	    cur_loc_cnf_id;
    i4	    sib_loc_cnf_id;
    i4	    dat_loc_cnf_id;
    i4	    local_error;
    bool	    leaf_split;
    bool	    pages_mutexed = FALSE;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    curpage = *current;
    leaf_split = (DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, curpage)
		  & DMPP_LEAF) != 0;
    if (leaf_split)
    {

	klen = t->tcb_klen - t->tcb_hilbertsize;
	kperpage = t->tcb_kperleaf;
    }
    else
    {
	klen = t->tcb_klen;
	kperpage = t->tcb_kperpage;
    }
    /*
    ** Set up rowaccess structure correctly, depending on whether
    ** this is a secondary index or not
    */
    if (r->rcb_tcb_ptr->tcb_rel.relstat & TCB_INDEX)
    {
	rac = &t->tcb_data_rac;
    }
    else
    {
	rac = &t->tcb_index_rac;
    }

    /*
    ** Choose page type of the new sibling page based on the type of the page
    ** being split.
    */
    if (DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, curpage) & DMPP_SPRIG)
	ptype = DM1B_PSPRIG;
    else if (DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, curpage) &
		DMPP_INDEX)
	ptype = DM1B_PINDEX;
    else if (DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, curpage) &
		DMPP_LEAF)
	ptype = DM1B_PLEAF;

    if (DMZ_AM_MACRO(5))
    {
	TRdisplay("dm1mxsplit:table is %~t, current is %d\n",
                       sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
		       DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, curpage));
	/* %%%% Place call to debug routine to print locks. */
    }

    for (;;)
    {
	/*
	** Rtree page splits are different from Btree splits in the following
	** ways:
	**       - Rtree index pages (like leaf pages) have high and low range
	** 	   key values.
	**
	**       - Rtree pages may have duplicate key values on them. But,
	**         unlike Btree, it is not necessary that all duplicate keys
	**         remain on the same leaf.
	**
	**       - Both leaf and index pages have sideways pointers which must
	** 	   be updated.
	*/

	/*
	** extract the Low and High Range Keys.
	*/
	range_keys[DM1B_LEFT] = lrange_buf;
	range_keys[DM1B_RIGHT] = rrange_buf;
	get_range_entries(curpage, NULL, t->tcb_acc_klv,
			  tbio->tbio_page_type, tbio->tbio_page_size,
			  range_keys, range_lens, infinities, dberr);

    	if (DMZ_AM_MACRO(5))
    	{
	    TRdisplay("dm1mxsplit: before split, keyrange is:\n");
	    dmdprbrange(r, curpage);
    	}

	if (leaf_split)
	{
	    /*
	    ** Special check for inserts of increasing key values.
	    **
	    ** If this split is to the Highest range LEAF page in the table
	    ** (the one with the infinity Rrange value) and our new key is
	    ** greater than the highest valued key currently on the leaf, then
	    ** adjust the split position to be at the highest value (bt_kids)
	    ** so that a new empty leaf will be created for the new key and no
	    ** old keys will be moved over in the split.
	    **
	    ** Use 2nd key buffer to avoid overwriting our current discriminator
	    ** key if it turns out that the last key on the page is greater
	    ** than the insert key.
	    */
	    if (infinities[DM1B_RIGHT].tid_i4 && (*split != 0) &&
		(*split != DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type,curpage)))
	    {
		/* Get highest valued key on the page */

		keyptr = compress_buf;
		keylen = klen;
		s = dm1cxget(tbio->tbio_page_type, tbio->tbio_page_size,
		    curpage, rac,
		    DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type, curpage) - 1,
		    &keyptr, (DM_TID *)NULL, (i4*)NULL,
		    &keylen, NULL, NULL, adf_cb);
		if (s != E_DB_OK)
		{
		    dm1cxlog_error(E_DM93E3_BAD_INDEX_GET, r, curpage,
		     tbio->tbio_page_type, tbio->tbio_page_size,
		     DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type, curpage) - 1);
		    SETDBERR(dberr, 0, E_DM93B2_BXSPLIT_ERROR);
		    break;
		}

		/*
		** Compare the high key on leaf with the insert key.  If the
		** high key is less, then adjust the split position.
		*/
		s = dm1mxkkcmp(tbio->tbio_page_type, tbio->tbio_page_size,
			curpage, adf_cb, t->tcb_acc_klv,
			keyptr, key, &compare);
	        if (s != E_DB_OK)
	        {
		    uleFormat(NULL, E_DMF012_ADT_FAIL, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		        (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		    SETDBERR(dberr, 0, E_DM93B2_BXSPLIT_ERROR);
		    break;
	        }
		if (compare < 0)
		{
		    *split = t->tcb_kperleaf * t->tcb_rel.rellfill / 100;
		    insert_direction = DM1B_RIGHT;
		    MEcopy((PTR) keyptr, keylen, (PTR) keybuf);
		    disc_key = keybuf;
		}
	    }
	}

	/*
	** Extract the key just before the the split position to use as the
	** discriminator key.  It will become the high range key for the
	** current page and the low range key for the new sibling, and it
	** will be inserted into the parent page.
	*/
	if (disc_key == NULL)
	{
	    keyptr = keybuf;
	    keylen = klen;
	    s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, curpage,
			rac, *split - 1,
			&keyptr, (DM_TID *)NULL, (i4*)NULL,
			&keylen,
			NULL, NULL, adf_cb);
	    if (s != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93E3_BAD_INDEX_GET, r, curpage,
			t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, *split - 1);
		SETDBERR(dberr, 0, E_DM93B2_BXSPLIT_ERROR);
		break;
	    }
	    disc_key = keyptr;
	}
	MEcopy(disc_key, klen - DM1B_TID_S, splitkey);
	disc_key = splitkey;
	if (leaf_split)
	{
	    /* On a leaf page, compute the Hilbert value and append it. */
     	    s = dm1mxhilbert(adf_cb, t->tcb_acc_klv, disc_key,
	    		     (char *)disc_key + klen - DM1B_TID_S);
    	    if (s != E_DB_OK)
	    	break;
	}

	/*
	**************************************************************
	**           Start Split Transaction
	**************************************************************
	**
	** Splits are performed within the overflow Mini-Transaction.
	** Should the transaction which is updating this table abort after
	** we have completed the split, then the split will NOT be backed
	** out, even though the insert which caused the split will be.
	**
	** We commit splits so that we can release the index page locks
	** acquired during the split and gain better concurrency.
	*/

	/*
	** Allocate the new sibling page.
	** The allocate call must indicate that it is performed from within
	** a mini-transation so that we will not be allocated any pages that
	** were previously deallocated in this same transaction.  (If we were
	** and our transaction were to abort it would be bad: the undo of the
	** deallocate would fail when it found the page was not currently free
	** since the split would not have been rolled back.)
	*/
	alloc_flag = DM1P_MINI_TRAN;
	if (DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, curpage) &
			DMPP_INDEX)
	    alloc_flag |= DM1P_PHYSICAL;

	s = dm1m_getfree(r, alloc_flag, sibling, dberr);
	if (s != E_DB_OK)
	    break;
	sibpage = *sibling;

	/* Inform buffer manager of new page's type */
	dm0p_pagetype(tbio, *sibling, r->rcb_log_id,
		(DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, curpage) &
		DMPP_LEAF) ? DMPP_LEAF
			   : DMPP_INDEX);

	/*
	** If the split adds a new leaf page to a base table, then we also
	** need to allocate a new data page.
	*/
	if (leaf_split &&
	    (r->rcb_tcb_ptr->tcb_rel.relstat & TCB_INDEX) == 0)
	{
	    s = dm1m_getfree(r, DM1P_MINI_TRAN, &data, dberr);
	    if (s != E_DB_OK)
		break;

	    /* Inform buffer manager of new page's type */
	    dm0p_pagetype(tbio, data, r->rcb_log_id, DMPP_DATA);
	}

	if (DMZ_AM_MACRO(5))
	{
	    TRdisplay("Rtree Split: page %d, sibling %d, (data %d)\n",
		DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, curpage),
		DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, sibpage),
		(data ?
		 DMPP_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, data) : -1));
	    TRdisplay("     : split pos %d, insert dir %w, page type %v\n",
		*split, "LEFT,RIGHT", insert_direction,
		",,,,,DATA,LEAF,,,,,,CHAIN,INDEX,,SPRIG",
		DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, curpage));
	    TRdisplay("     : Insert Key is:\n");
	    dmd_prkey(r, key, leaf_split ? DM1B_PLEAF : DM1B_PINDEX, (DB_ATTS**)NULL, (i4)0, (i4)0);
	    TRdisplay("     : Key at Split position is:\n");
	    dmd_prkey(r, disc_key, leaf_split ? DM1B_PLEAF : DM1B_PINDEX, (DB_ATTS**)NULL, (i4)0, (i4)0);
	}

	/*
	** Get information on the locations to which the update is being made.
	*/
	loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count,
		 (i4)DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type,
						   sibpage));
	sib_loc_cnf_id = tbio->tbio_location_array[loc_id].loc_config_id;
	loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count,
		 (i4)DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type,
						   curpage));
	cur_loc_cnf_id = tbio->tbio_location_array[loc_id].loc_config_id;
	dat_loc_cnf_id = 0;
	if (data)
	{
	    loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count,
			(i4)DMPP_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type,
							  data));
	    dat_loc_cnf_id = tbio->tbio_location_array[loc_id].loc_config_id;
	}

	/*
	** Online Backup Protocols: Check if BIs must be logged before update.
	*/
	if (d->dcb_status & DCB_S_BACKUP)
	{
	    s = dm0p_before_image_check(r, sibpage, dberr);
	    if (s != E_DB_OK)
		break;
	    s = dm0p_before_image_check(r, curpage, dberr);
	    if (s != E_DB_OK)
		break;
	    if (data)
	    {
	        s = dm0p_before_image_check(r, data, dberr);
	        if (s != E_DB_OK)
		    break;
	    }
	}

	/************************************************************
	**            Log the Split operation.
	*************************************************************
	**
	** A split operation is logged in two phases:  Before Image
	** Log Records are written prior to updating the pages.  These
	** are used only for UNDO processing.
	**
	** After Image Log Records are written after the operation is
	** complete to record the final contents of the pages.
	** These log records must be written BEFORE the mutexes are
	** released on the pages.  The LSNs of these log records are
	** written to the page_log_addr fields as well as to their split
	** addresses.
	*/

	if (r->rcb_logging & RCB_LOGGING)
	{
	    /* Physical locks are used on Rtree index pages */
	    dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);

	    if ( r->rcb_state & RCB_IN_MINI_TRAN )
	        dm0l_flag |= DM0L_MINI;

	    /* 
	    ** We use temporary physical locks for catalogs, extension tables
	    ** and index pages. The same lock protocol must be observed during 
	    ** recovery.
	    */
	    if (t->tcb_sysrel || t->tcb_extended || ( ! leaf_split))
		dm0l_flag |= DM0L_PHYS_LOCK;

	    /*
	    ** Reserve log file space for 2 BIs and their CLRs
	    */
	    cl_status = LGreserve(0, r->rcb_log_id, 4,
		4*(sizeof(DM0L_BI) - sizeof(DMPP_PAGE) + t->tcb_rel.relpgsize),
		&sys_err);
	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		  	(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			err_code, 1, 0, r->rcb_log_id);
		SETDBERR(dberr, 0, E_DM93B2_BXSPLIT_ERROR);
		break;
	    }

	    /*
	    ** Mutex the pages before beginning the udpates.  This must be done
	    ** prior to the writing of the log record to satisfy logging
	    ** protocols.
	    */
	    pages_mutexed = TRUE;
	    dm1m_mutex(tbio, (i4)0, r->rcb_lk_id, &sibpage);
	    dm1m_mutex(tbio, (i4)0, r->rcb_lk_id, &curpage);
	    if (data)
		dm1m_mutex(tbio, (i4)0, r->rcb_lk_id, &data);

	    s = dm0l_bi(r->rcb_log_id, dm0l_flag,
		  &t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		  t->tcb_rel.relpgtype, t->tcb_rel.relloccount,
		  cur_loc_cnf_id, DM0L_BI_SPLIT,
		  DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, curpage),
		  t->tcb_rel.relpgsize, curpage,
		  (LG_LSN *) 0, &lsn, dberr);
            if (s != E_DB_OK)
		break;

	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, curpage, lsn);

	    s = dm0l_bi(r->rcb_log_id, dm0l_flag,
		  &t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		  t->tcb_rel.relpgtype, t->tcb_rel.relloccount,
		  sib_loc_cnf_id, DM0L_BI_SPLIT,
		  DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, sibpage),
		  t->tcb_rel.relpgsize, sibpage,
		  (LG_LSN *) 0, &lsn, dberr);
	    if (s != E_DB_OK)
		break;

	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, sibpage, lsn);

	    if (data)
	    {
	    	s = dm0l_bi(r->rcb_log_id, dm0l_flag,
		    &t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		    t->tcb_rel.relpgtype, t->tcb_rel.relloccount,
		    dat_loc_cnf_id, DM0L_BI_SPLIT,
		    DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, data),
		    t->tcb_rel.relpgsize, data,
		    (LG_LSN *) 0, &lsn, dberr);
	    	if (s != E_DB_OK)
		    break;

	    	DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, data, lsn);
	    } /* if (data) */
	}
	else
	{
	    pages_mutexed = TRUE;
	    dm1m_mutex(tbio, (i4)0, r->rcb_lk_id, &sibpage);
	    dm1m_mutex(tbio, (i4)0, r->rcb_lk_id, &curpage);
	    if (data)
		dm1m_mutex(tbio, (i4)0, r->rcb_lk_id, &data);
	}

	/*
	** Format the new sibling page.
	*/
	s = dm1mxformat(tbio->tbio_page_type, tbio->tbio_page_size, 
			r, t, sibpage, kperpage, klen,
	    		DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, sibpage),
	    		ptype, compression_type, t->tcb_acc_plv, dberr);
	if (s != E_DB_OK)
	    break;

	DM1B_VPT_SET_PAGE_TRAN_ID_MACRO(tbio->tbio_page_type, sibpage,
	    DM1B_VPT_GET_PAGE_TRAN_ID_MACRO(tbio->tbio_page_type, curpage));
	DM1B_VPT_SET_PAGE_STAT_MACRO(tbio->tbio_page_type, sibpage, DMPP_MODIFY);

	/*
	** Move the appropriate entries from the current page to the
	** sibling page.
	*/
	s = dm1cxrshift(tbio->tbio_page_type, tbio->tbio_page_size,
		curpage, sibpage, compression_type, *split, klen);
	if (s != E_DB_OK)
	    break;

	/*
	** If a new associated data page was allocated, then format it as an
	** associated data page and link it to the newly allocated leaf page.
	*/
	if (data)
	{
	    DMPP_VPT_INIT_PAGE_STAT_MACRO(tbio->tbio_page_type, data,
			DMPP_DATA | DMPP_ASSOC | DMPP_PRIM | DMPP_MODIFY);
	    DM1B_VPT_SET_BT_DATA_MACRO(tbio->tbio_page_type, sibpage,
		DMPP_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, data));
	}
	DM1B_VPT_SET_PAGE_STAT_MACRO(tbio->tbio_page_type, curpage, DMPP_MODIFY);

	/*
	** If splitting a leaf or index page, then update the range keys on the
	** current and sibling pages. The current page's range keys span from
	** its old lrange value to the new discriminator key value.  The new
	** sibling page's range keys span from the discriminator key value to
	** the current page's old rrange value. The MBRs of both pages need to
	** be recomputed.
	*/
	s = update_range_entries(curpage, sibpage, adf_cb,
				 t->tcb_acc_klv,
				 tbio->tbio_page_type, tbio->tbio_page_size, 
				 compression_type,
				 &r->rcb_tran_id, r->rcb_slog_id_id,
				 r->rcb_seq_number,
				 range_keys, range_lens, infinities,
				 disc_key, klen - DM1B_TID_S, dberr);
	if (s != E_DB_OK)
	    break;
	/*
	** Also set the new and old page's sideways pointers.  Since we always
	** split to the right, it's always: curpage -> sibling -> nextpage.
	*/
	DM1B_VPT_SET_BT_NEXTPAGE_MACRO(tbio->tbio_page_type, sibpage,
		DM1B_VPT_GET_BT_NEXTPAGE_MACRO(tbio->tbio_page_type, curpage));
	DM1B_VPT_SET_BT_NEXTPAGE_MACRO(tbio->tbio_page_type, curpage,
		DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, sibpage));

	if (DMZ_AM_MACRO(5))
	{
	    TRdisplay("dm1mxsplit: after split, curpage keyrange is:\n");
	    dmdprbrange(r, curpage);
	    TRdisplay("dm1mxsplit: after split, sibpage keyrange is:\n");
	    dmdprbrange(r, sibpage);
	}

	/************************************************************
	**            Log the completed split after images
	*************************************************************
	**
	** Complete the Logging for the operation by logging the After
	** Images of the updated pages.  These will be used during forward
	** (redo,rollforward) recovery to reconstruct the pages.
	*/
	if (r->rcb_logging & RCB_LOGGING)
	{
	    if (pages_mutexed)
	    {
		dm1m_unmutex(tbio, (i4)0, r->rcb_lk_id, &curpage);
		dm1m_unmutex(tbio, (i4)0, r->rcb_lk_id, &sibpage);
		pages_mutexed = FALSE;
	    }

	    /* Physical locks are used on Rtree index pages */
	    dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);

	    if ( r->rcb_state & RCB_IN_MINI_TRAN )
	        dm0l_flag |= DM0L_MINI;

	    if (! leaf_split)
		dm0l_flag |= DM0L_PHYS_LOCK;

	    /* Reserve log file space for 2 AIs */
	    cl_status = LGreserve(0, r->rcb_log_id, 2,
		2*(sizeof(DM0L_AI) - sizeof(DMPP_PAGE) + t->tcb_rel.relpgsize),
		&sys_err);
	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		  	(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			err_code, 1, 0, r->rcb_log_id);
		SETDBERR(dberr, 0, E_DM93B2_BXSPLIT_ERROR);
		break;
	    }

	    dm1m_mutex(tbio, (i4)0, r->rcb_lk_id, &curpage);
	    dm1m_mutex(tbio, (i4)0, r->rcb_lk_id, &sibpage);
	    pages_mutexed = TRUE;

	    s = dm0l_ai(r->rcb_log_id, dm0l_flag,
		&t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		t->tcb_rel.relpgtype, t->tcb_rel.relloccount, cur_loc_cnf_id,
		DM0L_BI_SPLIT,
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, curpage),
		t->tcb_rel.relpgsize, curpage,
		(LG_LSN *) 0, &lsn, dberr);
	    if (s != E_DB_OK)
		break;
	
	    s = dm0l_ai(r->rcb_log_id, dm0l_flag,
		&t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		t->tcb_rel.relpgtype, t->tcb_rel.relloccount, sib_loc_cnf_id,
		DM0L_BI_SPLIT,
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, sibpage),
		t->tcb_rel.relpgsize, sibpage,
		(LG_LSN *) 0, &lsn, dberr);
	    if (s != E_DB_OK)
		break;

	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, curpage, lsn);
	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, sibpage, lsn);

	    DM1B_VPT_SET_BT_SPLIT_LSN_MACRO(tbio->tbio_page_type, curpage, lsn);
	    DM1B_VPT_SET_BT_SPLIT_LSN_MACRO(tbio->tbio_page_type, sibpage, lsn);
	}

	/*
	** Unmutex the pages since the update actions described by
	** the split log record are complete.
	**
	** Following the unmutexes, these pages may be written out
	** to the database at any time.
	*/
	pages_mutexed = FALSE;
	dm1m_unmutex(tbio, (i4)0, r->rcb_lk_id, &sibpage);
	dm1m_unmutex(tbio, (i4)0, r->rcb_lk_id, &curpage);
	if (data)
	    dm1m_unmutex(tbio, (i4)0, r->rcb_lk_id, &data);

#ifdef xDEV_TEST
	if (DMZ_CRH_MACRO(19) || DMZ_CRH_MACRO(20) || DMZ_CRH_MACRO(21))
	{
    	    i4	    n;
	    TRdisplay("dm1mxsplit: CRASH! Splitting table %t, db %t.\n",
		       sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
		       sizeof(DB_DB_NAME), &t->tcb_dcb_ptr->dcb_name);
	    if (DMZ_CRH_MACRO(20))
	    {
		(VOID) dm1m_unfix_page(r, DM0P_FORCE, current, dberr);
		(VOID) dm1m_unfix_page(r, DM0P_FORCE, sibling, dbder);
		TRdisplay("Current and Sibling Forced.\n");
	    }
	    if (DMZ_CRH_MACRO(21))
	    {
		(VOID) dm1m_unfix_page(r, DM0P_FORCE, current, dberr);
		(VOID) dm1m_unfix_page(r, DM0P_FORCE, sibling, dberr);
		TRdisplay("Current and Sibling Forced.\n");
	    }
	    CSterminate(CS_CRASH, &n);
	}
#endif

	/*
	** Unfix the data page. We leave this routine with the current page and
	** sibling still fixed.
	*/
	if (data)
	{
	    s = dm1m_unfix_page(r, DM0P_UNFIX, &data, dberr);
	    if (s != E_DB_OK)
		break;
	}

	/*
	** Store current and sibling page numbers for rcb_update call, below.
	*/
	bid.tid_tid.tid_page = DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type,
							curpage);
	bid.tid_tid.tid_line = 0;
	newbid.tid_tid.tid_page =
			DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type,sibpage);
	newbid.tid_tid.tid_line = 0;

	/*
	** Check for any oustanding RCB's that may have had their position
	** pointers affected by our shuffling tuples around.
	*/
	s = dm1m_rcb_update(r, &bid, &newbid, *split, DM1C_MSPLIT, dberr);
	if (s != E_DB_OK)
	    break;

	if (DMZ_AM_MACRO(5))
	{
	    TRdisplay("dm1mxsplit: returning from split, ");
	    TRdisplay("table is %t, current is %d, \n",
                       sizeof(DB_TAB_NAME), &r->rcb_tcb_ptr->tcb_rel.relid,
		       DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, curpage));
	    /* %%%% Place call to debug routine to print locks. */
	}

	return (E_DB_OK);
    }

    /*
    ** Error handling:
    */
    if (pages_mutexed)
    {
	dm1m_unmutex(tbio, (i4)0, r->rcb_lk_id, &sibpage);
	dm1m_unmutex(tbio, (i4)0, r->rcb_lk_id, &curpage);
	if (data)
	    dm1m_unmutex(tbio, (i4)0, r->rcb_lk_id, &data);
    }

    if (*sibling)
    {
	s = dm1m_unfix_page(r, DM0P_UNFIX, sibling, &local_dberr);
	if (s != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }
    if (data)
    {
	s = dm1m_unfix_page(r, DM0P_UNFIX, &data, &local_dberr);
	if (s != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM93B2_BXSPLIT_ERROR);
    }
    return E_DB_ERROR;
}


/*
** Name: mbr_search - Perform a search by minimum bounding rectangle
**
** Description:
** 	Examine the Rtree index or leaf page for an mbr that:
** 	    overlaps,
** 	    intersects,
** 	    is inside, or
** 	    contains
** 	the search key.
**
** Inputs:
**      rcb                             Pointer to record access context
**      page                            Pointer to the page to search
**      direction                       Value to indicate the direction
**                                      of search when duplicate keys are
**                                      found:
**					    DM1C_NEXT
**					    DM1C_PREV
**					    DM1C_EXACT
**	key				The specified key to search for.
** 	adf_cb				ADF control block pointed to by rcb.
**	klv				Key-level vector, containing Hilbert
**					size, function pointers, etc.
**
** Outputs:
**      pos                             Pointer to an area to return result.
**					See description above for the value
**					which is returned under various
**					combinations of input key and direction.
**	err_code			Set to an error code if
**					E_DB_ERROR is returned:
**					    E_DM0056_NOPART
**
** Returns:
**	E_DB_OK
** 	E_DB_ERROR
**
** History:
**      25-may-1996 (wonst02)
**	    Changed all page header access as macro for New Page Format
**	    project.
**	25-nov-1996 (wonst02)
**	    Changed parameters of mbr_search() to eliminate need for rcb.
**      03-jan-2003 (chash01)
**          Initialize keylen to 0.
*/
static DB_STATUS
mbr_search(
    DMP_RCB      *rcb,
    DMPP_PAGE   *page,
    i4      direction,
    char         *key,
    ADF_CB	 *adf_cb,
    DMPP_ACC_KLV *klv,
    i4		 pgtype,
    i4		 pgsize,
    i4		 *pos,
    DB_ERROR	 *dberr)
{
    char		*key_ptr;
    i4		keylen = 0;
    i4		kid;
    DB_STATUS	      	s;
    bool		compare;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /* If page has no records, nothing to search. */

    if (DM1B_VPT_GET_BT_KIDS_MACRO(pgtype, page) == 0)
    {
        *pos = 0;
	return E_DB_OK;
    }

    /*
    ** Search the entries on the index or leaf page.
    */
    for (kid = 0;
    	 kid < DM1B_VPT_GET_BT_KIDS_MACRO(pgtype, page);
    	 kid++)
    {
	dm1cxrecptr(pgtype, pgsize, page, kid, &key_ptr);

	s = dm1mxcompare(adf_cb, klv, direction,
			 DM1B_VPT_GET_PAGE_STAT_MACRO(pgtype, page)
		  	  & DMPP_LEAF,
			 key, keylen, key_ptr, &compare);

	if (s != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E3_BAD_INDEX_GET, rcb, page,
	    		   pgtype, pgsize, kid);
	    SETDBERR(dberr, 0, E_DM93B3_BXBINSRCH_ERROR);
	    return s;
	}
	if (compare)
	{
	    *pos = kid;
	    return E_DB_OK;
	}
    } /* for (kid = 0; kid < DM1B_GET_BT_... */


    *pos = DM1B_VPT_GET_BT_KIDS_MACRO(pgtype, page);
    return E_DB_OK;
} /* mbr_search( */


/*{
** Name: binary_search - Perform a binary search on an Rtree index page
**
** Description:
**      This routines examines the Rtree index page to determine the
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
**	    DM1C_NEXT		Return the position immediately FOLLOWING the
**				last occurrence of the key on the page. This
**				will be a value between 1 and bt_kids
**	    DM1C_PREV		Return the position of the first occurrence
**				of the key on the page. This will be a value
**				between 0 and (bt_kids-1).
**
**	A direction value of DM1C_EXACT may be passed to this routine; it is
**	treated identically to DM1C_PREV.
**
** Inputs:
**      r				Pointer to record access context
**      page                            Pointer to the page to search
**      direction                       Value to indicate the direction
**                                      of search when duplicate keys are
**                                      found:
**					    DM1C_NEXT
**					    DM1C_PREV
**					    DM1C_EXACT
**	key				The specified key to search for.
** 	adf_cb				ADF control block pointed to by rcb.
**	klv				Key-level vector, containing Hilbert
**					size, function pointers, etc.
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
**      25-may-1996 (wonst02)
**	    Changed all page header access as macro for New Page Format
**	    project.
**      22-Dec-2004 (hanal04) Bug 113671 INGSRV3103
**          Use pgtype instead of referencing through the rcb which may
**          have been explicitly passed as NULL from dmve_rtdel(). This
**          prevents a SIGSEGV and DBMS crash.
**	20-nov-2006 (wanfr01)
**	    SIR 117586
**	    changed arguments for dmd_prkey
*/
static	DB_STATUS
binary_search(
    DMP_RCB      *r,
    DMPP_PAGE   *page,
    i4      direction,
    char         *key,
    ADF_CB	 *adf_cb,
    DMPP_ACC_KLV *klv,
    i4		 pgtype,
    i4		 pgsize,
    i4		 *pos,
    DB_ERROR	 *dberr)
{
    i4                	right, left, middle;
    i4           	compare;
    DB_STATUS	      	s;
    char		*key_ptr;
    bool		is_index;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /* If page has no records, nothing to search. */

    if (DM1B_VPT_GET_BT_KIDS_MACRO(pgtype, page) == 0)
    {
        *pos = 0;
        return (E_DB_OK);
    }

    is_index = ((DM1B_VPT_GET_PAGE_STAT_MACRO(pgtype, page) & DMPP_INDEX) != 0);

    /*
    ** Start binary search of keys in record on page.  This binary search is
    ** different from those you'll see in textbooks because we continue
    ** searching when a matching key is found.
    */

    left = 0;
    right = DM1B_VPT_GET_BT_KIDS_MACRO(pgtype, page) - 1;
    /*
    ** Don't bother looking at the last entry on an index page: by
    ** definition, it contains all the Hilbert values greater than
    ** or equal to those on the prior entry.
    */
    if (DM1B_VPT_GET_PAGE_STAT_MACRO(pgtype, page) & DMPP_INDEX)
        right--;

    while (left <= right)
    {
        middle = (left + right)/2;
	dm1cxrecptr(pgtype, pgsize, page, middle, &key_ptr);

	/*
	** Use only the Hilbert value for the key compare in Rtree.
	*/
	s = dm1mxkkcmp(pgtype, pgsize, page, adf_cb, klv, key_ptr, key, &compare);
	if (s != E_DB_OK)
	{
	    uleFormat(NULL, E_DMF012_ADT_FAIL, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    	       (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM93B3_BXBINSRCH_ERROR);
	    return (s);
	}

        if (compare > 0)
            right = middle - 1;
        else if (compare < 0)
            left = middle + 1;
        else if (direction == DM1C_NEXT)
            left = middle + 1;
        else
            right = middle - 1;
    } /* while (left <= right) */

    *pos = left;

    if (DMZ_AM_MACRO(9))
    {
	s = TRdisplay("binary_search: searched for %s occurrence of key\n",
                   (direction == DM1C_NEXT) ? "last" : "first");
        dmd_prkey(r, key, is_index ? DM1B_PINDEX : DM1B_PLEAF, (DB_ATTS**)NULL, (i4)0, (i4)0);
        s = TRdisplay("\n in \n");
        dmd_prindex(r, page, (i4)0);
        s = TRdisplay("position found is %d\n", *pos);
    }

    return (E_DB_OK);
} /* binary_search */


/*
** Name: get_range_entries	- Retrieve the LRANGE/RRANGE entries from page
**
** Description:
**	This little subroutine simply extracts the LRANGE and RRANGE entries
**	from the indicated page.
**
**	The LRANGE entry is placed in array member 0, which is also referred
**	to as DM1B_LEFT. The RRANGE entry is placed in array member 1, which
**	is also referred to as DM1B_RIGHT.
**
** Inputs:
**	r			Record Control Block
**	cur			Page to extract entries from
**	sib			Sibling to extract entries from (may be NULL)
**	range			Array of two key buffers
**	range_len		Array of two key lengths
** 		NOTE: Each range array entry should point to an area that is
** 		large enough for an entire R-tree key, i.e., the MBR and LHV.
** 		For a 2-dimensional R-tree, the area is 18 bytes:
** 		 _____________________________________
** 		|    MBR (12 bytes)      |   LHV (6)  |
** 		|________________________|____________|
**
**	infinity		Array of two tids
**
** Outputs:
**	err_code		Set if an error occurs
**	range[0]
**	range_len[0]
**	infinity[0]		Gets the LRANGE entry
**	range[1]
**	range_len[1]
**	infinity[1]		Gets the RRANGE entry
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**      25-may-1996 (wonst02)
**	    Added page_size parameter in dm1cxlog_error routine for New Page
**	    format project.
** 	22-jul-1996 (wonst02)
** 	    Changed completely for Rtree.
**	28-sep-1996 (wonst02)
**	    Added capability for a sibling page.
*/
static void
get_range_entries(
    DMPP_PAGE		*cur,
    DMPP_PAGE		*sib,
    DMPP_ACC_KLV *klv,
    i4		page_type,
    i4		page_size,
    char		*range[],
    i4		range_len[],
    DM_TID		infinity[],
    DB_ERROR		*dberr)
{
    u_i2		mbr_size = 2 * klv->hilbertsize;
    u_i2		key_len = mbr_size + klv->hilbertsize;
    char		*key_ptr;

	if (sib == (DMPP_PAGE *)NULL)
	    sib = cur;
	if (DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, cur) & DMPP_LEAF)
	{
	    dm1cxtget(page_type, page_size, cur, DM1B_LRANGE,
		       &infinity[DM1B_LEFT], (i4*)NULL );
	    dm1cxtget(page_type, page_size, sib, DM1B_RRANGE,
		       &infinity[DM1B_RIGHT], (i4*)NULL );
	}
	else
	{
	    infinity[DM1B_LEFT].tid_i4 = infinity[DM1B_RIGHT].tid_i4 = 0;
	}

	range_len[DM1B_LEFT] = range_len[DM1B_RIGHT] = key_len;

	/*
	** The MBR portion of the low key is not available--make it null.
	*/
	MEcopy(klv->null_nbr, mbr_size, range[DM1B_LEFT]);

	/*
	** Get the Minimum Bounding Region (MBR) from the RRANGE key of cur.
	*/
	dm1cxrecptr(page_type, page_size, cur, DM1B_RRANGE, &key_ptr);
	MEcopy(key_ptr, mbr_size, range[DM1B_RIGHT]);

	/*
	** Leaf page: get both low and high Hilbert values from the LRANGE key.
	** The low LHV is in the leftmost portion; the high LHV, the rightmost.
	** If cur and sib are different pages, get the high LHV from the sib.
	*/
    	if (DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, cur)
	    & DMPP_LEAF)
	{
	    dm1cxrecptr(page_type, page_size, cur, DM1B_LRANGE, &key_ptr);
	    MEcopy(key_ptr, klv->hilbertsize, range[DM1B_LEFT] + mbr_size);
	    if (sib != cur)
	    	dm1cxrecptr(page_type, page_size, sib, DM1B_LRANGE, &key_ptr);
	    MEcopy(key_ptr + klv->hilbertsize,
	    	   klv->hilbertsize, range[DM1B_RIGHT] + mbr_size);
	}
	else
	{
	    dm1cxrecptr(page_type, page_size, cur, DM1B_LRANGE, &key_ptr);
	    MEcopy(key_ptr + mbr_size, klv->hilbertsize, 
	    	   range[DM1B_LEFT] + mbr_size);
	    dm1cxrecptr(page_type, page_size, sib, DM1B_RRANGE, &key_ptr);
	    MEcopy(key_ptr + mbr_size, klv->hilbertsize, 
	    	   range[DM1B_RIGHT] + mbr_size);
	}
}

/*
** Name: update_range_entries - update the range entries following split
**
** Description:
**	When a page split is performed, the LRANGE and RRANGE entries on the
**	resulting pages must be updated to reflect the new key ranges contained
**	on those pages. That action is performed here.
**
** Inputs:
**	cur			Current (left-hand) page of the split
**	sib			New sibling (right-hand) page of the split
** 	adf_cb				ADF control block pointed to by rcb.
**	klv				Key-level vector, containing Hilbert
**					size, function pointers, etc.
**	range			Low and high range keys
**	range_len		Low and high range key lengths
**	infinity		Infinity values for low and high range keys
**	midkey			Middle key of the division
**	midkey_len		Length of midkey
**
** Outputs:
**	midkey			The MBR portion of midkey is modified to be the
**				MBR of the cur page; the LHV is unmodified.
**	err_code		Set if an error occurs.
**
** Returns:
**	E_DB_OK			range entries updated
**	E_DB_ERROR		error occurred
**
** History:
**      25-may-1996 (wonst02)
**	    Changed all the page header access as macro for New Page Format
**	    project.
** 	22-jul-1996 (wonst02)
** 	    Changed completely for Rtree.
*/
static DB_STATUS
update_range_entries(
    DMPP_PAGE		*cur,
    DMPP_PAGE		*sib,
    ADF_CB	 	*adf_cb,
    DMPP_ACC_KLV        *klv,
    i4		page_type,
    i4		page_size,
    i4	    	compression_type,
    DB_TRAN_ID      	*tran_id,
    u_i2		lg_id,
    i4		seq_number,
    char		*range[],
    i4		range_len[],
    DM_TID		infinity[],
    char		*midkey,
    i4		midkey_len,
    DB_ERROR		*dberr)
{
    u_i2	    hilbertsize = klv->hilbertsize;
    DB_STATUS	    s = E_DB_OK;
    char	    *sibrange[2];
    i4	    sibrange_len[2];
    DM_TID	    sibinfinity[2];
    bool            changed;

    CLRDBERR(dberr);

    for (;;)
    {
	/*
	** Update minimum and maximum key values on old and new pages. The
	** previous range of values on the page which we split is now
	** broken into two subranges, separated by the split key, 'midkey'.
	** The old page, 'cur', now contains the lower half of the range,
	** which is {range[DM1B_LEFT] .. midkey}. The new page, 'sib',
	** now contains the upper half of the range, which is
	** {midkey .. range[DM1B_RIGHT]}.
	**
	** The midkey is known to be neither -infinity nor +infinity.
	** The MBR portion of midkey is modified to be the MBR of the cur page.
	*/
	sibrange[DM1B_LEFT] = midkey;
	sibrange_len[DM1B_LEFT] = midkey_len;
	sibinfinity[DM1B_LEFT].tid_i4 = 0;
	sibrange[DM1B_RIGHT] = range[DM1B_RIGHT];
	sibrange_len[DM1B_RIGHT] = range_len[DM1B_RIGHT];
	sibinfinity[DM1B_RIGHT].tid_i4 = infinity[DM1B_RIGHT].tid_i4;
	range[DM1B_RIGHT] = midkey;
	range_len[DM1B_RIGHT] = midkey_len;
	infinity[DM1B_RIGHT].tid_i4 = 0;

	/*
	** Recompute the total page MBR from all its entries.
	*/
	s = dm1mxpagembr(page_type, page_size, cur, 
		    range[DM1B_RIGHT], adf_cb, klv, hilbertsize, &changed);
	if (s)
	    break;
	s = put_range_entries(cur, compression_type, hilbertsize,
			      page_type, page_size, tran_id, lg_id, seq_number,
			      range, range_len, infinity);
	if (s)
	    break;

	/*
	** Recompute the total page MBR from all its entries.
	*/
	s = dm1mxpagembr(page_type, page_size, sib, 
		    sibrange[DM1B_RIGHT], adf_cb, klv, hilbertsize, &changed);
	if (s != E_DB_OK)
	    break;
	s = put_range_entries(sib, compression_type, hilbertsize,
			      page_type, page_size, tran_id, lg_id, seq_number,
			      sibrange, sibrange_len, sibinfinity);
	if (s)
	    break;

	break;
    } /* for (;;) */
    return s;
} /* update_range_entries */


/*
** Name: put_range_entries - put new range entries on the page
**
** Description:
** 	Replace the LRANGE and RRANGE keys on the page.
**
** Inputs:
**	r			Record Control Block
**	cur			page to extract entries from
**	range			Array of two key buffers
**	range_len		Array of two key lengths
** 		NOTE: Each range array entry should point to an area that is
** 		large enough for an entire R-tree key, i.e., the MBR and LHV.
** 		For a 2-dimensional R-tree, the area is 18 bytes:
** 		 _____________________________________
** 		|    MBR (12 bytes)      |   LHV (6)  |
** 		|________________________|____________|
**
**	infinity		Array of two tids
**
** Outputs:
**
** Returns:
**	E_DB_OK			range entries updated
**	E_DB_ERROR		error occurred
**
** History:
** 	22-jul-1996 (wonst02)
** 	    Added for Rtree.
*/
static DB_STATUS
put_range_entries(
    DMPP_PAGE	*cur,
    i4		compression_type,
    u_i2	hilbertsize,
    i4		page_type,
    i4		page_size,
    DB_TRAN_ID      	*tran_id,
    u_i2		lg_id,
    i4		seq_number,
    char		*range[],
    i4		range_len[],
    DM_TID		infinity[])
{
    DB_STATUS	    s = E_DB_OK;
    u_i2	    mbr_size = 2 * hilbertsize;
    u_i2	    keylen;
    char	    *lkeyptr;
    char	    *rkeyptr;
    char	    keybuf[DB_MAXRTREE_KEY];

    if (!(DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, cur)
	  & DMPP_LEAF))
    {
	keylen = mbr_size + hilbertsize;
	lkeyptr = range[DM1B_LEFT];
	rkeyptr = range[DM1B_RIGHT];
    }
    else
    {
	/*
	** On an Rtree leaf page (sizes shown for 2 dimensions):
	** 	          +---------+-------+-------+
	** 	LRANGE -> | INFlow  |LHVlow |LHVhigh|    INF  = infinity flag
	** 	          | 4 bytes |3 bytes|3 bytes|    LHV  = Largest Hilbert
	** 	          +---------+-------+-------+           Value
	**                                           	 MBR  = Minimum Bounding
	** 	          +---------+---------------+           Rectangle
	**  	RRANGE -> | INFhigh |      MBR      |    high = high range
	** 	          | 4 bytes |    6 bytes    |    low  = low range
	** 	          +---------+---------------+
	*/
	MEcopy((char *)range[DM1B_LEFT] + mbr_size, hilbertsize, 
		(char *)keybuf);
	MEcopy((char *)range[DM1B_RIGHT] + mbr_size, hilbertsize,
	       (char *)keybuf + hilbertsize);
	keylen = mbr_size;
	lkeyptr = keybuf;
	rkeyptr = range[DM1B_RIGHT];
    }

    s = dm1cxput(page_type, page_size, cur, compression_type, 
		DM1C_DIRECT, tran_id, lg_id, seq_number, DM1B_LRANGE,
    	      	lkeyptr, keylen, &infinity[DM1B_LEFT], (i4)0 );
    if (s)
    {
        dm1cxlog_error( E_DM93E4_BAD_INDEX_PUT, (DMP_RCB *)NULL, cur,
        		    page_type, page_size, DM1B_LRANGE );
        return s;
    }
    s = dm1cxput(page_type, page_size, cur, compression_type, 
		DM1C_DIRECT, tran_id, lg_id, seq_number, DM1B_RRANGE,
    	      	rkeyptr, keylen, &infinity[DM1B_RIGHT], (i4)0 );
    if (s)
    {
        dm1cxlog_error( E_DM93E4_BAD_INDEX_PUT, (DMP_RCB *)NULL, cur,
        		    page_type, page_size, DM1B_RRANGE );
        return s;
    }
    return s;
} /* put_range_entries */
