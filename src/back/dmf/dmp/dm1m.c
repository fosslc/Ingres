/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <tm.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <ade.h>
#include    <ulf.h>
#include    <sxf.h>
#include    <dmf.h>
#include    <dmacb.h>
#include    <dm.h>
#include    <dmpl.h>
#include    <dmpp.h>
#include    <dmp.h>
#include    <dm1p.h>
#include    <dm1b.h>
#include    <dm1m.h>
#include    <dm0l.h>
#include    <dm0p.h>
#include    <dm1c.h>
#include    <dm1cx.h>
#include    <dm1r.h>
#include    <dmftrace.h>
#include    <dmd.h>
#include    <dm2f.h>
#include    <dma.h>
#include    <dmccb.h>
#include    <dmxcb.h>
#include    <dml.h>

/**
**
**  Name: DM1M.C - Routines to access and update Rtree tables.
**
**  Description:
**      This file contains all the routines needed to access and
**      update Rtree tables.
**
**      The routines defined in this file are:
**      dm1m_allocate       - Allocates space for putting an Rtree record.
**      dm1m_bybid_get      - Gets a record by BID.
**	dm1m_check_range    - Check to see if this key belongs on this page.
**      dm1m_delete         - Deletes a record from an Rtree table.
**      dm1m_get            - Gets a record from an Rtree table.
**	dm1m_get_range 	    - Get the low and high Hilbert values for the page.
**      dm1m_put            - Puts a record to an Rtree table.
**      dm1m_rcb_update     - Updates all RCB's in transaction.
**      dm1m_replace        - Replaces an Rtree record.
** 	dm1m_position	    - Position an Rtree for retrieval/update.
**      dm1m_search         - Searches an Rtree for a given key
**                            to determine positioning information.
** 	dm1m_badtid 	    - Flag bad Rtree tid.
**	dm1m_count	    - Count records in an Rtree table.
**
**  History:
** 	 5-may-95 (wonst02)
** 		Original (loosely cloned from dm1b.c)
**		=== ingres!main!generic!back!dmf!dmp dm1b.c rev 26 ====
** 	25-apr-1996 (wonst02)
** 	    Finish Rtree search.
**	07-may-1996 (wonst02) Incorporate: 30-apr-1996 (kch)
**          In the function dm1b_rcb_update(), if the current RCB deletes a
**          record we now perform the leaf page line table compression for all
**          other RCBs after the test for and marking of a fetched tid as
**          deleted. This change fixes bug 74970.
** 	27-may-1996 (wonst02)
**	    New Page Format Project. Modify page header references to macros.
**      21-aug-1996 (wonst02) Incorporate: 15-jul-1996 (ramra01 for bryanp)
**          Alter Table support: allow for row versions on pages. Extract row
**          version and pass it to dmpp_uncompress. Call dm1r_cvt_row if
**          necessary to convert row to current version.
**	23-Oct-1996 (wonst02)
**	    Added dm1m_count. Incorporated misc. updates from dm1b.c.
**	25-Oct-1996 (shero03)
**	    Added tupbuf on the uncompress call from dm1b
**	06-Nov-1996 (wonst02)
**	    Fix problem finding and locking multiple parent pages.
**	18-nov-1996 (wonst02)
**	    Incorporate use of DMPP_SIZEOF_TUPLE_HDR_MACRO for new page format.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Removed unecessary DMPP_INIT_TUPLE_INFO_MACRO
**	04-dec-1996 (wonst02)
**	    Added ancestor stack parameters to dm1m_replace().
**	    Added call to adjust_mbrs() after delete and before insert.
**	12-dec-1996 (wonst02)
**	    New parameters added to find_next_leaf().
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      12-jun-97 (stial01)
**          Pass tlv to dm1cxget instead of tcb.
**      29-jul-97 (stial01)
**          Pass original operation mode (PUT/DELETE) to dm1r* for logging.
**      17-mar-98 (stial01)
**          dm1m_get() Copy rec to caller buf BEFORE calling dm1c_pget (B86862)
**      21-apr-98 (stial01)
**          Set DM0L_PHYS_LOCK in log rec if extension table (B90677)
**	11-Aug-1998 (jenjo02)
**	    Keep tuple counts in rcb_tup_adds instead of tcb_tup_adds.
**      20-Jul-98 (wanya01)
**          dm1m_get() Copy rec to called buf BEFORE calling dm1c_pget.
**          This is addtional changes for b86862. The previous changes was
**          only made for routine when retrieving record by sequence, not
**          when retrieving record by TID.
**	01-dec-1998 (nanpr01)
**	    Get rid of DMPP_TUPLE_INFO structure. Use DMPP_TUPLE_HDR instead.
**      07-Dec-1998 (stial01)
**          Added kperleaf to distinguish from kperpage (keys per index page)
**      09-feb-1999 (stial01)
**          dm1m_kperpage() relcmptlvl param added for backwards compatability
**      12-Apr-1999 (stial01)
**          Pass page_type parameter to dmd_prkey
** 	25-mar-1999 (wonst02)
** 	    Fix RTREE bug B95350 in Rel. 2.5. Remove DMPP_SIZEOF_TUPLE_HDR_MACRO
** 	    and add dm1cx_isnew() to handle V2 page format.
**          Added handling of deleted leaf entries (status == E_DB_WARN).
**	16-aug-1999 (nanpr01)
**	    Implementation of SIR 92299. Enabaling journalling on a
**	    online checkpoint.
**	20-oct-1999 (nanpr01)
**	    Optimized the code to minimize the tuple header copy.
**	21-oct-1999 (nanpr01)
**	    More CPU optimization. Do not copy tuple header when not needed.
**	05-dec-99 (nanpr01)
**	    Added tuple header in prototype to reduce memcopy.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**      30-oct-2000 (stial01)
**          Pass rcb to dm1r_allocate, isnew routines
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to dma_row_access() if neither
**	    C2 nor B1SECURE.
**      01-feb-2001 (stial01)
**          Arg changes for page accessors (Variable Page Types SIR 102955)
**          dm1m_isnew -> dm1cx_isnew
**      10-jul-2003 (stial01)
**          Fixed row lock error handling during allocate (b110521)
**          Actually these changes don't really apply for rtree because
**          we currently do not support rtree primary indexes.
**	22-Jan-2004 (jenjo02)
**	    A host of changes for Partitioning and Global indexes,
**	    particularly row-locking with TID8s (which include
**	    partition number (search for "partno") along with TID).
**	    LEAF pages in Global indexes will contain DM_TID8s 
**	    rather than DM_TIDs; the TID size is now stored on
**	    the page (4 for all but LEAF, 8 for TID8 leaves).
**	    Note, however, the RTREEs don't yet support
**	    Global indexes.
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      16-mar-2004 (gupsh01)
**          Modified dm1r_cvt_row to include adf control block in 
**	    parameter list.
**      25-jun-2004 (stial01)
**          Modularize get processing for compressed, altered, or segmented rows
**      31-jan-05 (stial01)
**          Added flags arg to dm0p_mutex call(s).
**	13-Apr-2006 (jenjo02)
**	    Tweaks to dmd_prkey(), dm1cxkperpage() for unrelated
**	    CLUSTERED Btree base tables.
**	24-Feb-2008 (kschendel) SIR 122739
**	    Various changes for new rowaccess structure.
**	11-Apr-2008 (kschendel)
**	    RCB's adfcb now typed as such, minor fixes here.
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	02-Dec-2008 (jonj)
**	    SIR 120874: dm1r_? functions converted to DB_ERROR *
**	03-Dec-2008 (jonj)
**	    SIR 120874: dm1m_? functions converted to DB_ERROR *,
**	    use new form uleFormat.
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1p_?, dmf_adf_error functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dma_?, dm1c_?, dm0p_? functions converted to DB_ERROR *
**      12-oct-2009 (stial01)
**          Use DMPP_PAGE for all page types.
**      24-nov-2009 (stial01)
**          Check if table has segmented rows (instead of if pagetype supports)
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace DMPP_PAGE* with DMP_PINFO* where needed.
**	    As Rtrees are not supported by either row or crow locking, 
**	    saved a lot of messy destabilizing
**	    code changes that would otherwise be needed by adding
**	    bridge functions dm1m_fix_page(), dm1m_unfix_page(), dm1m_mutex()
**	    dm1m_unmutex(), dm1m_getfree(), so dm1m functions can remain 
**	    unconverted to DMP_PINFO.
**/


# define MAX_SPLIT_POLICY 4
# define LOG_REQUIRED (i4) 1
# define NO_LOG_REQUIRED (i4) 0
# define MUTEX_REQUIRED (i4) 1
# define NO_MUTEX_REQUIRED (i4) 0

/*
**  Forward function references.
*/
static	DB_STATUS find_first(
    	DMP_RCB	    *rcb,
	DMPP_PAGE   **parent,	
    	DM_TID	    *leaf_bid,
    	DM_TID	    *leaf_tid,
    	DB_ERROR    *dberr);

static	DB_STATUS next(
	DMP_RCB         *rcb,
    	DB_ERROR    *dberr);

static	DB_STATUS find_next_leaf(
    	DMP_RCB	    *rcb,
    	DMPP_PAGE   **leaf,
    	DM_TID	    *leaf_bid,
    	DM_TID	    *leaf_tid,
    	DB_ERROR    *dberr);

static	DB_STATUS pushstack(
    	DMP_RCB	    *r,
    	DM_PAGENO   pageno,
	DMPP_PAGE   **page_ptr,
    	DB_ERROR    *dberr);

static	DB_STATUS popstack(
    	DMP_RCB	    *r,
	DMPP_PAGE   **page_ptr,
    	DB_ERROR    *dberr);

static	DB_STATUS next_index_page(
    	DMP_RCB	    *r,
	DMPP_PAGE   **page_ptr,
    	i4	    fix_type,
    	DB_ERROR    *dberr);

static	DB_STATUS handle_overflow(
    	DMP_RCB		*rcb,
	DMPP_PAGE	***locks,
    	DMPP_PAGE	**sibling,
	i4		*numsiblings,
    	DMPP_PAGE	**newsib,
    	char		*key,
	char		*splitkey,
	LG_LSN	    	*lsn,
    	DB_ERROR        *dberr);

static	DB_STATUS split_page(
	DMP_RCB		*r,
	DMPP_PAGE	*sibling[],
	DMPP_PAGE	**newpage,
	i4		siblings,
	i4		*sibkids,
	char		*key,
	char		*splitkey,
    	DB_ERROR        *dberr);

static	DB_STATUS distribute_entries(
	DMP_RCB		*r,
	DMPP_PAGE	*sibling[],
	i4		siblings,
	i4		sibkids,
	LG_LSN	    	*lsn,
    	DB_ERROR        *dberr);

static	DB_STATUS adjust_tree(
    	DMP_RCB		*rcb,
	DMPP_PAGE	***locks,
    	DMPP_PAGE	**sibling,
	i4		numsiblings,
	DMPP_PAGE	*newsib,
    	char		*key,
	char		*splitkey,
	LG_LSN	    	*prev_lsn,
    	DB_ERROR        *dberr);

static	DB_STATUS unlock_tree(
    	DMP_RCB		*r,
	DMPP_PAGE	**locks,
	DMPP_PAGE	*target,
    	DB_ERROR        *dberr);

static	DB_STATUS adjust_mbrs(
    	DMP_RCB		*r,
    	DMPP_PAGE	*childpage,
    	DB_ERROR        *dberr);

static	DB_STATUS requalify_page(
    	DMP_RCB		*r,
    	DMPP_PAGE	**page,
	char		*key,
	i4         mode,
	i4         direction,
    	DB_ERROR        *dberr);

static	DB_STATUS requalify_parent(
    	DMP_RCB		*r,
    	DMPP_PAGE	**parent,
	DMPP_PAGE	*childpage,
	i4		*ancestor_level,
    	DB_ERROR        *dberr);

static	DB_STATUS log_before_images(
    	DMP_RCB	    	*r,
    	DMPP_PAGE	**sibling,
	i4	    	numsiblings,
	bool	    	*pages_mutexed,
    	DB_ERROR        *dberr);

static	DB_STATUS log_after_images(
    	DMP_RCB	    	*r,
    	DMPP_PAGE	**sibling,
	i4	    	numsiblings,
	bool	    	*pages_mutexed,
    	DB_ERROR        *dberr);

static	DB_STATUS findpage(
	DMP_RCB		*rcb,
	DMPP_PAGE       *leaf,
	DMPP_PAGE       **data,
	char		*record,
	i4         need,
	DM_TID		*tid,
    	DB_ERROR        *dberr);

static	DB_STATUS check_reclaim(
	DMP_RCB	    *rcb,
	DMPP_PAGE   **data,
    	DB_ERROR        *dberr);


/*{
** Name: dm1m_allocate - Allocates position for insertion into an Rtree table.
**
** Description:
**      This routine allocates a position for the insertion of a record into
**      an Rtree table.  It searches for the appropriate leaf page.
**	If an index page is full, it splits the page and updates the
**	parent node, using a "B-link tree" locking scheme.
**
**      Note, the tuple cannot be allocated here, since the
**      page before image must be logged before any changes
**      to the page.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      record                          Pointer to record to insert.
**      need                            Value indicating amount of space
**                                      needed, assumes compressed
**                                      if neccessary.
**      opflag				Flag Options:
**					DM1C_REALLOCATE - used only during UNDO
**					    recovery actions.  It specifies to
**					    allocate space on the leaf but to
**					    ignore the data pages.  No splits
**					    are ever done and duplicate checking
**					    is bypassed.
**
** Outputs:
**      leaf                            Pointer to an area used to return
**                                      pointer to leaf page.
**      data                            Pointer to an area used to return
**                                      pointer to data page.
**      bid                             Pointer to an area to return
**                                      bid indicating where to insert
**                                      index record.
**      tid                             Pointer to tid for allocated record.
**      key                             Pointer to an area to return key.
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**                                      The error codes are:
**                                      E_DM0045_DUPLICATE_KEY.
**
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**          Fixes and locks leaf and data pages.
**
** History:
** 	27-may-1996 (wonst02)
**	    New Page Format Project. Modify page header references to macros.
**	14-nov-1997 (nanpr01)
**	    Tuple header was getting added twice for larger pages.
*/
DB_STATUS
dm1m_allocate(
    DMP_RCB	*rcb,
    DMP_PINFO   *leafPinfo,
    DMP_PINFO   *dataPinfo,
    DM_TID      *bid,
    DM_TID      *tid,
    char        *record,
    i4     need,
    char        *key,
    i4     opflag,
    DB_ERROR    *dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB         *t = r->rcb_tcb_ptr;
    i4		    pgsize = t->tcb_rel.relpgsize;
    i4		    pgtype = t->tcb_rel.relpgtype;
    ADF_CB	    *adf_cb = r->rcb_adf_cb;
    DB_ATTS	    **keyatts = t->tcb_key_atts;
    DB_STATUS       s = E_DB_OK;
    i4		    lineno;
    DM_TID	    localtid;
    i4         keys_given;
    i4         local_error;
    i4	    compression_type;
    i4	    duplicate_insert = FALSE;
    i4	    mode;
    bool	    have_leaf = FALSE;
    bool	    split_required = FALSE;
    bool	    in_range = FALSE;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;
    DMPP_PAGE	    **leaf, **data;

    CLRDBERR(dberr);

    leaf = &leafPinfo->page;
    data = &dataPinfo->page;

    /*
    ** Build Rtree index key from given record.  If the table is a secondary
    ** index then the key consists of the entire record.
    **
    ** Since we are building a full key, the keys_given is exactly the number
    ** of keys in the Rtree.
    */
    keys_given = t->tcb_keys;
    if (t->tcb_rel.relstat & TCB_INDEX)
    {
	MEcopy(record, t->tcb_klen, key);
    }
    else
    {
	char            *pos = key;
	i4		k;

	for (k = 0; k < keys_given; k++)
	{
	    DB_ATTS        *att;

	    att = keyatts[k];
	    MEcopy(record + att->offset, att->length, pos);
	    pos += att->length;
	}
    }
    compression_type = DM1B_INDEX_COMPRESSION(r);


    /*
    ** Search the Rtree Index to find the place for the new record.
    **
    ** Performance Check :  Check the currently fixed leaf page to see if it
    ** is the one to which the new record should belong.  If sequential
    ** inserts are being done, then it is likely that we already have the
    ** correct leaf in memory.
    **
    ** Compare the record's key with the leaf's high and low range keys.  If
    ** it falls in between and there is space on the page, then we already
    ** have the correct leaf fixed.
    */
    while (DB_SUCCESS_MACRO(s) && (*leaf != NULL))
    {
	s = dm1m_check_range(pgtype, pgsize, *leaf, adf_cb, 
			t->tcb_acc_klv, key, &in_range, dberr);
	if (s || ! in_range)
	    break;

	/*
	** If this leaf has no room, then fall through to the search
	** code so a split pass can be done.
	*/

	if (dm1cxhas_room(pgtype, pgsize, *leaf, compression_type,
			    (i4)100 /* fill factor */, 
			    t->tcb_kperleaf, dm1cxmax_length(r, *leaf) - 
		    	    t->tcb_hilbertsize - DM1B_TID_S) == FALSE)
	{
	    split_required = TRUE;
	    break;
	}

	/*
	** We have verified that the correct leaf is fixed.  Find the correct
	** spot on the leaf for the new record.
	**
	** This call may return NOPART if the leaf is empty - in that case
	** it will return with lineno set to the first position (0).
	*/
	s = dm1mxsearch(r, *leaf, DM1C_OPTIM, DM1C_EXACT, key, adf_cb, 
			t->tcb_acc_klv,
			pgtype, pgsize, &localtid, &lineno, dberr);
	if (DB_FAILURE_MACRO(s))
	{
	    if (dberr->err_code == E_DM0056_NOPART)
	     	s = E_DB_OK;
	    else
		break;
	}

	/*
	** Set bid value to spot for the specified record.
	*/
	bid->tid_tid.tid_page =
	    DM1B_VPT_GET_PAGE_PAGE_MACRO(pgtype, *leaf);
	bid->tid_tid.tid_line = lineno;

	have_leaf = TRUE;

	break;
    } /* while (DB_SUCCESS_MACRO(s) && (*... */

    /*
    ** If we found that we did not already have the correct leaf page fixed,
    ** then do a search on the Rtree for where to insert the record.
    **
    ** If dm1m_search returns that there is no room for a new record, then
    ** we will have to do a SPLIT pass below.
    */
    while (DB_SUCCESS_MACRO(s) &&
    	   (!have_leaf || split_required))
    {
	/*
	** Unfix any leaf/data pages left around.
	*/
	if (*leaf != NULL)
	{
	    s = dm1m_unfix_page(r, DM0P_UNFIX, leaf, dberr);
	}
	if (DB_SUCCESS_MACRO(s) && (*data != NULL))
	{
	    s = dm1m_unfix_page(r, DM0P_UNFIX, data, dberr);
	}
	if (DB_FAILURE_MACRO(s))
	    break;

	/*
	** Search the Rtree. This may require a SPLIT.
	**
	** Note that search returns with the leaf to which the given key
	** should be inserted even if the search returns NOROOM.
	*/
	if (opflag == DM1C_REALLOCATE)
	    mode = DM1C_OPTIM;
	else
	    mode = DM1C_SPLIT;

	s = dm1m_search(r, key, keys_given, mode, DM1C_EXACT,
			leafPinfo, bid, &localtid, dberr);
	if (DB_FAILURE_MACRO(s))
	{
	    /*
	    ** If the mode is DM1C_REALLOCATE, then we are attempting to
	    ** reallocate space during UNDO recovery and should never split.
	    ** If no space is found we return NOROOM.
	    */
	    if ((opflag == DM1C_REALLOCATE) || (dberr->err_code != E_DM0057_NOROOM))
		break;

	    s = E_DB_OK;
	}

	break;
    } /* while (DB_SUCCESS_MACRO(s) &&  */

    /*
    ** Have now found the correct portion of the Rtree index for the
    ** new record - now find a spot on a data page.  Don't need to do this
    ** if this is a secondary index or if DM1C_REALLOCATE mode is specified:
    **
    **	    - If this is a secondary index then there are no data pages.
    **	    - If DM1C_REALLOCATE mode, then the caller has requested to not
    **	      search for space on the data page, only an index entry will
    **	      be made.  This is used only in UNDO recovery where the index
    **	      and data page updates are aborted separately.
    */
    if (DB_SUCCESS_MACRO(s) && ((t->tcb_rel.relstat & TCB_INDEX) == 0) &&
	(opflag != DM1C_REALLOCATE))
    {
        s = findpage(r, *leaf, data, record, need, tid, dberr);

	/* Fall through to check return status */
    }

    if (DB_SUCCESS_MACRO(s))
	return (E_DB_OK);

    /*
    ** An error occurred.  Clean up and report errors.
    */
    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM925E_DM1B_ALLOCATE);
    }

    if (*leaf != NULL)
    {
	s = dm1m_unfix_page(r, DM0P_UNFIX, leaf, &local_dberr);
	if (DB_FAILURE_MACRO(s))
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    if (*data != NULL)
    {
	s = dm1m_unfix_page(r, DM0P_UNFIX, data, &local_dberr);
	if (DB_FAILURE_MACRO(s))
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    return (E_DB_ERROR);
}

/*{
** Name: dm1m_bybid_get - Get a record from an Rtree table.
**
** Description:
**      This routine uses the bid provided to determine the
**      the tid of the record and retrieves the record.
**      This routine was added to handle replaces which move
**      records.
**
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      bid                             Bid of record to retrieve.
**      tid                             Pointer to area to return tid.
**
** Outputs:
**      record                          Pointer to an area used to return
**                                      the record.
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**                                      Error codes returned are:
**                                      E_DM0055_NONEXT, E_DM003C_BAD_TID,
**                                      E_DB0044_DELETED_TID, and errors
**                                      returned fro dm1m_next.
**
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
**
** History:
** 	27-may-1996 (wonst02 for thaju02)
**          New Page Format Project. Modify page header references to macros.
**	20-may-1996 (wonst02 for ramra01)
**	    Added DMPP_TUPLE_INFO argument to the get accessor routine
**      03-june-1996 (wonst02 for stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**          Added DMPP_TUPLE_INFO argument to dm1cxget()
**      06-jun-1996 (wonst02 for prida01)
**          There is no check for ambiguous replace for btree's on deferred
**          update. dm1r_get already checks. Add the code for btree's.
**      15-jul-1996 (wonst02 for bryanp)
**          Alter Table support: allow for row versions on pages. Extract row
**          version and pass it to dmpp_uncompress. Call dm1r_cvt_row if
**          necessary to convert row to current version.
**      12-jun-97 (stial01)
**          dm1m_bybid_get() Pass tlv to dm1cxget instead of tcb.
** 	03-apr-1999 (wonst02)
**	    Remove DMPP_SIZEOF_TUPLE_HDR_MACRO--it doesn't belong.
**	13-aug-99 (stephenb)
**	    alter uncompress calls to new proto.
**      16-mar-2004 (gupsh01)
**          Modified dm1r_cvt_row to include adf control block in parameter
**          list.
*/
DB_STATUS
dm1m_bybid_get(
    DMP_RCB         *rcb,
    DM_TID          *bid,
    DM_TID          *tid,
    char	    *record,
    DB_ERROR        *dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DB_STATUS	    s;		/* Variable used for return status. */
    DMPP_PAGE	    *data, *leaf;   /* Pointers to data and leaf pages. */
    DMPP_PAGE      *leafpage;
    i4         fix_action;     /* Type of access to page. */
    i4         record_size;    /* Size of record retrieved. */
    i4	    	    row_version = 0;
    i4		    *row_ver_ptr;
    DM_TID          localtid;       /* Tid used for getting data. */
    char	    *rec_ptr = record;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (t->tcb_rel.relversion)
	row_ver_ptr = &row_version;
    else
	row_ver_ptr = NULL;

    fix_action = DM0P_WRITE;
    if (r->rcb_access_mode == RCB_A_READ)
        fix_action = DM0P_READ;

    for (;;)
    {
	leaf = r->rcb_other.page;
	if (leaf && bid->tid_tid.tid_page !=
	    DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, leaf))
	{
	    /*	When fixing correct leaf page into rcb_other.page,
            **  be sure to zero out "leaf" after unfixing the old page
            **  so we will fix a new one.  03-sep-87 (rogerk)
	    */
	    leaf = (DMPP_PAGE *) NULL;
	    s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other, dberr);
	    if (s != E_DB_OK)
		break;
	}
	if (leaf == NULL)
	{
	    s = dm0p_fix_page(r, (DM_PAGENO  )bid->tid_tid.tid_page,
                    fix_action | DM0P_NOREADAHEAD, &r->rcb_other,
		    dberr);
	    if (s != E_DB_OK)
		break;
	    leaf = r->rcb_other.page;
	}
	leafpage = leaf;

	if (bid->tid_tid.tid_line >=
 	    DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, leafpage))
	{
	    SETDBERR(dberr, 0, E_DM003C_BAD_TID);
	    return (E_DB_ERROR);
	}

	/*
	** Extract the TID pointed to by the entry at the specified BID.
	*/
	dm1cxtget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, leafpage,
			bid->tid_tid.tid_line, &localtid, (i4*)NULL);
	r->rcb_currenttid.tid_i4 = localtid.tid_i4;

	/*
	** Fix the data page specified by the extracted TID (if it is
	** not already fixed.
	*/
	if (r->rcb_data.page && localtid.tid_tid.tid_page !=
	    DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
	    r->rcb_data.page))
	{
	    s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_data, dberr);
	    if (s != E_DB_OK)
		break;
	}
	if (r->rcb_data.page == NULL)
	{
	    s = dm0p_fix_page(r, (DM_PAGENO )localtid.tid_tid.tid_page,
		fix_action | DM0P_NOREADAHEAD, &r->rcb_data, dberr);
	    if (s != E_DB_OK)
		break;
	}
	data = r->rcb_data.page;

	/*
	** Find the row at the indicated TID.  If it is compressed
	** then uncompress it into the caller's buffer.
	*/
	record_size = t->tcb_rel.relwid;
	s = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype,
		t->tcb_rel.relpgsize, data, &localtid, &record_size,
		&rec_ptr, row_ver_ptr, NULL, NULL, (DMPP_SEG_HDR *)NULL);

	/*  Make sure we return CHANGED_TUPLE on an ambiguous replace. */
	if ((r->rcb_update_mode == RCB_U_DEFERRED) &&
	    (*t->tcb_acc_plv->dmpp_isnew)(r, data, (i4)tid->tid_tid.tid_line))
	{
	    SETDBERR(dberr, 0, E_DM0047_CHANGED_TUPLE);
	    return (E_DB_ERROR);
	}

	/* Additional processing if compressed, altered, or segmented */
	if (s == E_DB_OK && 
	    (t->tcb_data_rac.compression_type != TCB_C_NONE || 
	    row_version != t->tcb_rel.relversion ||
	    t->tcb_seg_rows))
	{
	    s = dm1c_get(r, data, &localtid, record, dberr);
	    rec_ptr = record;
	}

	/*
	** If the row was found, return it to the caller.  Copy the row
	** into the caller's buffer (unless it was already uncompressed into
	** the buffer).
	*/
	if (s == E_DB_OK)
	{
	    tid->tid_i4 = localtid.tid_i4;
	    if (rec_ptr != record)
		MEcopy(rec_ptr, record_size, record);
	    return (E_DB_OK);
	}
	else
	{
	    char    keybuf[DB_MAXRTREE_KEY];
	    char    *key_ptr;
	    i4 tmpkey_len;

	    key_ptr = keybuf;
	    tmpkey_len = t->tcb_klen - t->tcb_hilbertsize;
	    _VOID_ dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
			    leafpage, &t->tcb_index_rac,
			    bid->tid_tid.tid_line,
			    &key_ptr, (DM_TID *)NULL, (i4*)NULL,
			    &tmpkey_len,
			    NULL, NULL, r->rcb_adf_cb);
	    dm1m_badtid(r, bid, tid, key_ptr);

	    s = E_DB_ERROR;
	    SETDBERR(dberr, 0, E_DM938B_INCONSISTENT_ROW);
	}
	break;
    }

    /*	Handle error reporting and recovery. */

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM925F_DM1B_GETBYBID);
    }

    return (E_DB_ERROR);
}

/*{
** Name: dm1m_delete - Deletes a record from an Rtree table.
**
** Description:
**	This routine deletes the record identified by tid from Rtree table.
**
**	It can be used for both Rtree base tables and Secondary Indexes.
**	On Secondary Indexes only the Leaf page is updated; on Base Tables
**	the Data page is updated as well.
**
**	The pages described by BID and TID will be fixed into the
**	rcb_other.page and rcb_data.page pointers, if they are not
**	already.  Except in the case when a data page is reclaimed (see
**	next paragraph), these pages will be left fixed upon return.
**
**	If the row deleted is the last row on a disassociated data page
**	then that data page is made FREE and put into the table's free
**	space maps.  The page remains locked and cannot actually be reused
**	by another transaction until this transaction commits.  The page
**	also unfixed by this routine.
**
**	This routine has been coded to handle the deletion of a record
**	which has been fetched, but has already been deleted or
**	replaced by another opener during this same transaction (cursor
**	case).
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      bid                             The bid of the leaf entry of the
**					record to delete.
**      tid                             The tid of the record to delete.
**
** Outputs:
**      err_code			Error status code:
**					    E_DM003C_BAD_TID
**					    E_DM0044_DELETED_TID
**					    E_DM9260_DM1B_DELETE
**	Returns:
**	    E_DB_OK
**          E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
** 	27-may-1996 (wonst02 for thaju02 & nanpr01)
**          New Page Format Project. Modify page header references to macros.
**	    Subtract tuple header size in index entry length for leaf pages.
** 	29-jul-1996 (wonst02)
** 	    Call adjust_mbrs to adjust the MBRs after deleting from the leaf.
**	11-oct-1996 (wonst02)
**	    Update the rcb ancestor stack with the delete bid.
**      29-jul-97 (stial01)
**          Pass original operation mode DM1C_DELETE to dm1r_delete for logging.
*/
DB_STATUS
dm1m_delete(
    DMP_RCB	*rcb,
    DM_TID	*bid,
    DM_TID	*tid,
    DB_ERROR	*dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB         *t = r->rcb_tcb_ptr;
    DMPP_PAGE	    *leafpage;	    /* Pointer to page of type index. */
    DM_TID	    localbid;	    /* BID pointer to key entry to delete. */
    DM_TID	    localtid;	    /* TID of data record to delete. */
    DB_STATUS	    s;
    bool	    base_table;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    base_table = (t->tcb_rel.relstat & TCB_INDEX) == 0;
    localbid.tid_i4 = bid->tid_i4;
    localtid.tid_i4 = tid->tid_i4;

    for (;;)
    {
	/*
	** Make sure we have the correct leaf and data pages fixed.
	*/
	if ((r->rcb_other.page == NULL) ||
	    (DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
	    r->rcb_other.page) != localbid.tid_tid.tid_page))
	{
	    if (r->rcb_other.page != NULL)
	    {
		s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other, dberr);
		if (s != E_DB_OK)
		    break;
	    }

	    s = dm0p_fix_page(r, (DM_PAGENO )localbid.tid_tid.tid_page,
		(DM0P_WRITE | DM0P_NOREADAHEAD), &r->rcb_other, dberr);
	    if (s != E_DB_OK)
		break;
	}
	leafpage = r->rcb_other.page;

	/*
	** Make sure we have the correct data page fixed (if base table).
	** Note requirement by dm1r_delete that the data page pointer must be
	** fixed into rcb_data.page and not locally.
	*/
	if ((base_table) &&
	    ((r->rcb_data.page == NULL) ||
	    (DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
	    r->rcb_data.page) != localtid.tid_tid.tid_page)))
	{
	    if (r->rcb_data.page != NULL)
	    {
		s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_data, dberr);
		if (s != E_DB_OK)
		    break;
	    }

	    s = dm0p_fix_page(r, (DM_PAGENO)localtid.tid_tid.tid_page,
		(DM0P_WRITE | DM0P_NOREADAHEAD), &r->rcb_data, dberr);
	    if (s != E_DB_OK)
		break;
	}

	/*
	** Verify that the row we're positioned at is the correct one to delete.
	*/
	if (base_table)
	{
	    /*
	    ** Look on the leaf page to get the TID of the row to which our BID
	    ** points. If the returned tid does not match the one passed into
	    ** this routine then we can't proceed.
	    **
	    ** This case occurs when we were positioned to a row which was
	    ** deleted by another RCB on the same table (presumably owned by
	    ** the same session).  When this occurs then we will be left
	    ** positioned to the NEXT leaf entry.  Checking the TID pointer
	    ** in the current leaf entry catches this.
	    */
	    dm1cxtget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, leafpage,
			localbid.tid_tid.tid_line, &localtid, (i4*)NULL);
	    if ( localtid.tid_i4 != tid->tid_i4 )
	    {
		SETDBERR(dberr, 0, E_DM0044_DELETED_TID);
		break;
	    }

	}

	/*
	** Sanity check on the leaf page bid.
	*/
	if (localbid.tid_tid.tid_line >=
	    DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype,
	    leafpage))
	{
	    SETDBERR(dberr, 0, E_DM003C_BAD_TID);
	    break;
	}
	r->rcb_ancestors[r->rcb_ancestor_level] = localbid;

	if (DMZ_AM_MACRO(2))
	{
	    TRdisplay("dm1m_delete: (%d,%d) bid = %d,%d, tid = %d,%d. \n",
		     t->tcb_rel.reltid.db_tab_base,
		     t->tcb_rel.reltid.db_tab_index,
		     localbid.tid_tid.tid_page, localbid.tid_tid.tid_line,
		     localtid.tid_tid.tid_page, localtid.tid_tid.tid_line);
	    TRdisplay("dm1m_delete: Leaf of deletion\n");
	    dmd_prindex(r, leafpage, (i4)0);
	}

	/*
	** The the table is a Base Table (not a Secondary Index), then delete
	** the data row.  Also check if the data page can then be reclaimed.
	*/
	if (base_table)
	{
	    /*
	    ** Delete record in data page
	    */
	    s = dm1r_delete(r, &localtid, (i4)0, (i4)DM1C_MDELETE, dberr);
	    if (s != E_DB_OK)
		break;

	    /*
	    ** Check whether page can be reclaimed.  If the last row has just
	    ** been deleted from a disassociated data page, then we can add
	    ** the page back to the free space.  Note: this call may unfix the
	    ** data page pointer.
	    */
	    s = check_reclaim(r, &r->rcb_data.page, dberr);
	    if (s != E_DB_OK)
		break;
	}

	/*
	** Delete the entry from the Rtree Index.
	*/
	s = dm1mxdelete(r, leafpage, localbid.tid_tid.tid_line,
			LOG_REQUIRED, MUTEX_REQUIRED, dberr);
	if (s != E_DB_OK)
	    break;

	if (DMZ_AM_MACRO(2))
	{
	    TRdisplay("dm1m_delete: Leaf of deletion after deletion.\n");
	    dmd_prindex(r, leafpage, (i4)0);
	}

	/*
	** Find any other RCB's whose table position pointers may have been
	** affected by this delete and update their positions accordingly.
	*/
	s = dm1m_rcb_update(r, &localbid, (DM_TID *)NULL, (i4)0,
			 DM1C_MDELETE, dberr);

#ifdef xDEV_TEST
	/*
	** Rtree Crash Tests
	*/
	if (DMZ_CRH_MACRO(1) && ( ! base_table))
	{
	    i4	    n;
	    TRdisplay("dm1m_delete: CRASH! Secondary Index, Leaf not forced\n");
	    CSterminate(CS_CRASH, &n);
	}
	if (DMZ_CRH_MACRO(2) && ( ! base_table))
	{
	    i4	    n;
	    TRdisplay("dm1m_delete: CRASH! Secondary Index, Leaf forced.\n");
	    (VOID)dm0p_unfix_page(r,DM0P_FORCE,&r->rcb_other,dberr);
	    CSterminate(CS_CRASH, &n);
	}
	if (DMZ_CRH_MACRO(3) && (base_table))
	{
	    i4	    n;
	    TRdisplay("dm1m_delete: CRASH! Updates not forced.\n");
	    CSterminate(CS_CRASH, &n);
	}
	if (DMZ_CRH_MACRO(4) && (base_table))
	{
	    i4	    n;
	    TRdisplay("dm1m_delete: CRASH! Leaf forced.\n");
	    (VOID)dm0p_unfix_page(r,DM0P_FORCE,&r->rcb_other,dberr);
	    CSterminate(CS_CRASH, &n);
	}
#endif

	/*
	** Secondary index handling:
	** For secondary index updates, we must increment rcb_tup_adds to
	** reflect our delete.  For base tables, this was done by dm1r_delete.
	*/
	if ( ! base_table)
	    r->rcb_tup_adds--;
	/*
	** Adjust the MBR of this page and of the parent page, which could
	** percolate all the way to the root level.
	*/
	s = adjust_mbrs(r, leafpage, dberr);
	if (s != E_DB_OK)
	    break;

	return (E_DB_OK);
    }

    /*
    ** Handle error reporting and recovery.
    */
    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9260_DM1B_DELETE);
    }
    return (E_DB_ERROR);
} /* dm1m_delete */

/*{
** Name: dm1m_get - Get a record from an Rtree table.
**
** Description:
**      This routine takes the high range bid and the low range bid
**      for the next record want to retieve or the TID of the actual
**      record.  This routine also requires a flag indicating the
**      type of retrieval desired.
**
**	A "cursor position" in an Rtree is a "bid", a "btree indentifier",
**	as opposed to a tid. A bid identifies a leaf page entry in an Rtree.
**	A bid has the same structure as a tid.	The tid_tid.tid_page component
**	identifies a leaf page; the tid_tid.tid_line component is a line table
**	position in the leaf.
**
**      This routine assumes that the following variables in the RCB are
**      for the use of scanning and will not be destroyed by other operations:
**      rcb->rcb_other.page	 - A pointer to current leaf.
**      rcb->rcb_data.page	 - A pointer to last data page assessed.
**      rcb->rcb_lowtid          - A pointer to current leaf (bid) position.
**      rcb->rcb_currentid       - A pointer to return actual tid of
**                                 retrieved record.  If opflag is DM1C_BYTID
**                                 then this points to exact tid want to
**                                 retrieve.
**      rcb->rcb_hl_ptr          - A key used to qualify retrieved records.
**                                 all values must be less than or equal to
**                                 this key.
**      rcb->rcb_hl_given        - A value indicating how many attributes are
**                                 in the key used to qualify records.
**                                 (May be partial key for ranges).
**
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      tid                             Tid of record to retrieve if opflag
**                                      is set to DM1C_BYTID.
**      opflag                          Value indicating type of retrieval.
**                                      Must be either DM1C_GETNEXT or
**                                      DM1C_BYTID.
**
** Outputs:
**      record                          Pointer to an area used to return
**                                      pointer to the record.
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**                                      Error codes returned are:
**                                      E_DM0055_NONEXT, E_DM003C_BAD_TID,
**                                      E_DB0044_DELETED_TID, and errors
**                                      returned from dm1m_next.
**
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**          Traverses the tree fixing, unfixing and locking pages and
**          updating scan information in the RCB.
**
** History:
** 	27-may-1996 (wonst02)
**          New Page Format Project. Modify page header references to macros.
**	06-jun-1996 (shero03)
**	    Return the NBR, Hilbert, and Tid when sequentially scanning
**      21-aug-1996 (wonst02) Incorporate: 15-jul-1996 (ramra01 for bryanp)
**          Alter Table support: allow for row versions on pages. Extract row
**          version and pass it to dmpp_uncompress. Call dm1r_cvt_row if
**          necessary to convert row to current version.
**	06-Nov-1996 (wonst02)
**	    Use dm1mxhilbert instead of calling the klv routine directly.
**      12-jun-97 (stial01)
**          dm1m_get() Pass tlv to dm1cxget instead of tcb.
**      17-mar-98 (stial01)
**          dm1m_get() Copy rec to caller buf BEFORE calling dm1c_pget (B86862)
**      20-Jul-98 (wanya01)
**          dm1b_get() Copy rec to called buf BEFORE calling dm1c_pget.
**          This is addtional changes for b86862. The previous changes was
**          only made for routine when retrieving record by sequence, not
**          when retrieving record by TID.
** 	06-apr-1999 (wonst02)
**	    Remove DMPP_SIZEOF_TUPLE_HDR_MACRO--it doesn't belong.
** 	    Use dm1cx_isnew() to prevent ambiguous updates.
**	13-aug-99 (stephenb)
**	    alter uncompress calls to new proto.
**	10-aug-2001 (thaju02)
**	    leaf_klen and rec_len are used to determine how much of the
**	    tuple to copy into the record buffer.
**	    In calculating leaf_klen and rec_len, do not account for
**	    tuple header, since dm1cxget() returns us the record pointer,
**	    pointing to the 'true' start of the record. (B105639)
**      16-mar-2004 (gupsh01)
**          Modified dm1r_cvt_row to include adf control block in parameter
**          list.
**	05-Aug-2004 (jenjo02)
**	    Don't do BYTE_ALIGN copy if rec_ptr == record.
**	20-nov-2006 (wanfr01)
**	    SIR 117586
**	    changed arguments for dmd_prkey
**	14-Apr-2010 (kschendel) SIR 123485
**	    Updated pget call.
*/
DB_STATUS
dm1m_get(
    DMP_RCB         *rcb,
    DM_TID          *tid,
    char	    *record,
    i4	    opflag,
    DB_ERROR        *dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DML_SCB	    *scb;
    ADF_CB	    *adf_cb = r->rcb_adf_cb;
    ADF_CB	    *dummy;
    ADF_CB	    **qual_adfcb;	/* SCB's qfun_adfcb or dummy */
    DB_STATUS	    s;		/* Variable used for return status. */
    DB_STATUS       get_status;
    DB_STATUS       data_get_status;
    DMPP_PAGE	    *data, *leaf;   /* Pointers to data and leaf pages. */
    DMPP_PAGE      *leafpage;
    i4         fix_action;     /* Type of access to page. */
    i4         record_size;    /* Size of record retrieved. */
    i4         local_err_code;
    DM_TID          localtid;       /* Tid used for getting data. */
    DM_TID	    temptid;
    i4	    row_version = 0;
    i4		    *row_ver_ptr;
    i4	    rec_len;
    i4	    leaf_klen;
    char	    *rec_ptr = record;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (t->tcb_rel.relversion)
	row_ver_ptr = &row_version;
    else
	row_ver_ptr = NULL;

    fix_action = DM0P_WRITE;
    if (r->rcb_access_mode == RCB_A_READ)
        fix_action = DM0P_READ;

    if (DMZ_AM_MACRO(1))
    {
	s = TRdisplay("dm1m_get: Starting tid = %d,%d,flag = %x\n",
                r->rcb_lowtid.tid_tid.tid_page,
                r->rcb_lowtid.tid_tid.tid_line,
                opflag);
	s = TRdisplay("dm1m_get: limit key is\n");
	if (r->rcb_hl_given > 0)
	    dmd_prkey(r, r->rcb_hl_ptr, DM1B_PLEAF, (DB_ATTS**)NULL, (i4)0, (i4)0);
	else
	    s = TRdisplay("no limit key\n");
    }

    scb = NULL;
    qual_adfcb = &dummy;
    if (r->rcb_xcb_ptr)
    {
	scb = r->rcb_xcb_ptr->xcb_scb_ptr;
	qual_adfcb = &scb->scb_qfun_adfcb;
    }

    /*	Loop through records that don't pass the qualification. */

    for (s = OK;opflag == DM1C_GETNEXT;)
    {
	leaf = r->rcb_other.page;
	if (leaf && r->rcb_lowtid.tid_tid.tid_page !=
	    DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, leaf))
	{
	    /*	03-sep-87 (rogerk)
	    **	When fixing correct leaf page into rcb_other.page,
            **  be sure to zero out "leaf" after unfixing the old page
            **  so we will fix a new one.
	    */
	    leaf = (DMPP_PAGE *) NULL;
	    s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other, dberr);
	    if (s != E_DB_OK)
		break;
	}
	if (leaf == NULL)
	{
	    s = dm0p_fix_page(r, (DM_PAGENO)r->rcb_lowtid.tid_tid.tid_page,
		fix_action, &r->rcb_other, dberr);
	    if (s != E_DB_OK)
		break;
	}

	/* check to see if user interrupt occurred. */

	if (*(r->rcb_uiptr) & RCB_USER_INTR)
	{
	    SETDBERR(dberr, 0, E_DM0065_USER_INTR);
	    s = E_DB_ERROR;
	    break;
	}
	/* check to see if force abort occurred. */

	if (*(r->rcb_uiptr) & RCB_FORCE_ABORT)
	{
	    SETDBERR(dberr, 0, E_DM010C_TRAN_ABORTED);
	    s = E_DB_ERROR;
	    break;
	}

	s = next(r, dberr);

	get_status = s;
	if (t->tcb_rel.relpgtype != TCB_PG_V1 && s == E_DB_WARN)
	{
	    s = E_DB_OK;
	}

	if (s != E_DB_OK)
	    break;

	/*
	** Now that we are here, whether it is a secondary
        ** index or not, then the leaf is pointing to 
        ** the next index record of the next record to obtain.
	*/
	if (t->tcb_rel.relstat & TCB_INDEX)
	{
	    localtid.tid_i4 = r->rcb_lowtid.tid_i4;
	    leaf = r->rcb_other.page;
	    leafpage = leaf;
	    rec_ptr = record;

	    /*
	    ** Rtree does not have the TIDP as the last column; ensure that
	    ** the resulting record contains a TIDP in the last column
	    ** by manufacturing one here.
	    */
	    leaf_klen = t->tcb_klen - t->tcb_hilbertsize;
	    rec_len = leaf_klen - DM1B_TID_S;
	    s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, leafpage,
			    &t->tcb_data_rac,
			    localtid.tid_tid.tid_line,
			    &rec_ptr, &temptid, (i4*)NULL,
			    &rec_len,
			    NULL, NULL, adf_cb);
	    if (s != E_DB_OK)
	    {
		dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, r, leafpage,
				t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
				localtid.tid_tid.tid_line );
		break;
	    }
	    if (rec_ptr != record)
	    {
		MEcopy(rec_ptr, rec_len, record);
		rec_ptr = record;
	    }

	    if (rec_len != leaf_klen)
	    {
	      if (r->rcb_hl_given == 0)	/* a sequential scan */
	      {
		/*
		** Return: Nbr, Hilbert, Tidp
		*/
     	    	s = dm1mxhilbert(adf_cb, t->tcb_acc_klv, record,
	    	     	     	 (char *)record + 2 * t->tcb_hilbertsize);
    	    	if (s != E_DB_OK)
		    break;
		MEcopy((char *)&temptid, DM1B_TID_S,
		       (char *)record + 3 * t->tcb_hilbertsize);
	      }
	      else
		MEcopy((char *)&temptid, DM1B_TID_S,
			record + rec_len);
	    }

	    r->rcb_currenttid.tid_i4 = localtid.tid_i4;
	    if (leaf != r->rcb_other.page)
	    {
		s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other, dberr);
		if (s != E_DB_OK)
		    break;
	    }
	    tid->tid_i4 = localtid.tid_i4;
	}
	else
	{
	    /*
	    ** Normal get next.
	    */

	    localtid.tid_i4 = r->rcb_currenttid.tid_i4;

	    /*
	    ** If we need a different data page, unfix the old one
	    ** before we lock the new page/row
	    */
	    if (r->rcb_data.page &&
		    (localtid.tid_tid.tid_page !=
		    DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, 
		    r->rcb_data.page)))
	    {
		s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_data, dberr);
		if (s != E_DB_OK)
		    break;
	    }

	    if (r->rcb_data.page == NULL)
	    {
		s = dm0p_fix_page(r, (DM_PAGENO )localtid.tid_tid.tid_page,
		    fix_action, &r->rcb_data, dberr);
		if (s != E_DB_OK)
		    break;
	    }

	    data = r->rcb_data.page;

	    /*
	    ** Get primary btree data record
	    */
	    record_size = t->tcb_rel.relwid;

	    s = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype,
		t->tcb_rel.relpgsize, data, &localtid, &record_size,
		&rec_ptr, row_ver_ptr, NULL, NULL, (DMPP_SEG_HDR *)NULL);
	    data_get_status = s;

	    /*
	    ** Skip deleted records
	    */
	    if (data_get_status == E_DB_WARN)
	    {
	    	continue;
	    }

	    /* Additional processing if compressed, altered, or segmented */
	    if (s == E_DB_OK && 
		(t->tcb_data_rac.compression_type != TCB_C_NONE || 
		row_version != t->tcb_rel.relversion ||
		t->tcb_seg_rows))
	    {
		s = dm1c_get(r, data, &localtid, record, dberr);
		rec_ptr = record;
	    }

	    if (s != E_DB_OK)
	    {
		/*
		** DM1C_GET or UNCOMPRESS returned an error - this indicates
		** that something is wrong with the tuple at the current tid.
		**
		** If user is running with special PATCH flags, then take
		** appropriate action - skip the row or return garbage,
		** otherwise return with an error.
		*/

		char	key_buf [DB_MAXRTREE_KEY];
		char	*key_ptr;
		i4	key_len;
		DB_STATUS   local_status;

		/*
		** If we reach this point while reading nolock, just go on
		** to the next record.
		*/
		if (r->rcb_k_duration & RCB_K_READ)
		    continue;

		leafpage = r->rcb_other.page;
		key_ptr = key_buf;
		key_len = t->tcb_klen - t->tcb_hilbertsize;
		local_status = dm1cxget(t->tcb_rel.relpgtype, 
			    t->tcb_rel.relpgsize, leafpage,
			    &t->tcb_index_rac,
			    r->rcb_lowtid.tid_tid.tid_line,
			    &key_ptr, (DM_TID *)NULL, (i4*)NULL,
			    &key_len,
			    NULL, NULL, adf_cb);
		if (t->tcb_rel.relpgtype != TCB_PG_V1 
				&& get_status == E_DB_WARN
				&& local_status == E_DB_WARN)
		{
		    local_status = E_DB_OK;
		}
		if (local_status != E_DB_OK)
		{
		    dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, r, leafpage,
				t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
				r->rcb_lowtid.tid_tid.tid_line );
		    key_ptr = "<UNKNOWN KEY>";
		    key_len = 13;
		}
		dm1m_badtid(r, &r->rcb_lowtid, &localtid, key_ptr);
		/*
		** If the user is running with special PATCH flag, then skip
		** this row and go on to the next.
		*/
		if (DMZ_REC_MACRO(1))
		    continue;

		/*
		** If the user is running with special PATCH flag to ignore
		** errors, then continue anyway - setting localtid to be
		** entry 0 on the page so that at least we probably won't
		** AV - although we will be returning a garbage row.
		*/
		if (DMZ_REC_MACRO(2))
		{
		    localtid.tid_tid.tid_line = 0;
		}
		else
		{
		    s = E_DB_ERROR;
		    SETDBERR(dberr, 0, E_DM9350_BTREE_INCONSIST);
		    break;
		}
	    }
	    tid->tid_i4 = localtid.tid_i4;
	}

	/*
	** At this point you have in record either the index record or
        ** data record whichever is appropriate.
        */

	/*
	** If the table being examined has extensions,
	** then we must fill in the indirect pointers that
	** exist on the page.  For large objects (the only
	** ones which exist at present), the coupons used
	** in the server contain the rcb of the table open
	** instance from which they are fetched.  These
	** are filled in, for tables with extensions, below.
	** This must be done before the rcb_f_qual
	** function is invoked, since it may contain
	** requests to examine the large objects in this record.
	**
	** NOTE:  Invoking the following rcb_f_qual
	** function may involve indirect recursion within
	** DMF (in that a qualification involving a large
	** object will go read some collection of the
	** extension tables).  Any internal concurrency
	** issues must take this into account.
	*/

	if (t->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS)
	{
	    /*
	    ** We must not pass dm1c_pget a pointer into the page
	    ** dm1c_pget() will change the rcb pointer in the coupon.
	    ** This causes recovery errors when we do consistency
	    ** checks on the tuple vs logged tuple. 
	    ** It is also bad to update the page without a mutex. (B86862)
	    */
	    if (rec_ptr != record)
	    {
		MEcopy(rec_ptr, record_size, record);
		rec_ptr = record;
	    }

	    s = dm1c_pget(t->tcb_atts_ptr,
			  r,
			  rec_ptr,
			  dberr);
	    if (s)
	    {
		uleFormat(dberr, 0,
			   (CL_ERR_DESC *)NULL,
			   ULE_LOG, NULL,
			   (char *)NULL,
			   (i4)0,
			   (i4 *)NULL,
			   err_code,
			   0);
		SETDBERR(dberr, 0, E_DM9B01_GET_DMPE_ERROR);
	        break;
	    }
	}

	/*
	** Check for caller qualification function.
	**
	** This qualification function requires that tuples start on aligned
	** boundaries.  If the tuple is not currently aligned, then we need to
	** make it so.
	**
	** THIS IS A TEMPORARY SOLUTION:  We check here if the tuple is aligned
	** on an ALIGN_RESTRICT boundary, and if not we copy it to the tuple
	** buffer, which we know is aligned.  But we only make this check if
	** the machine is a BYTE_ALIGN machine - since even on non-aligned
	** machines, ALIGN_RESTRICT is usually defined to some restrictive
	** value for best performance.  We do not want to copy tuples here when
	** we don't need to.  The better solution is to have two ALIGN_RESTRICT
	** boundaries, one for best performance and one which necessitates
	** copying of data.
	*/
#ifdef xDEBUG
	r->rcb_s_scan++;
#endif
	if (r->rcb_f_qual)
	{
	    ADF_CB *adfcb = r->rcb_adf_cb;

#ifdef BYTE_ALIGN
            /* lint truncation warning if size of ptr > int, but code valid */
	    if ( rec_ptr != record &&
	        (i4)rec_ptr & (i4)(sizeof(ALIGN_RESTRICT) - 1) )
	    {
		MEcopy(rec_ptr, record_size, record);
		rec_ptr = record;
	    }
#endif
#ifdef xDEBUG
	    r->rcb_s_qual++;
#endif

	    *qual_adfcb = adfcb;
	    /* Clean these in case non-ADF error */
	    adfcb->adf_errcb.ad_errclass = 0;
	    adfcb->adf_errcb.ad_errcode = 0;
	    *(r->rcb_f_rowaddr) = rec_ptr;
	    *(r->rcb_f_retval) = ADE_NOT_TRUE;
	    s = (*r->rcb_f_qual)(adfcb, r->rcb_f_arg);
	    *qual_adfcb = NULL;
	    if (s != E_DB_OK)
	    {
		s = dmf_adf_error(&adfcb->adf_errcb, s, scb, dberr);
		if (s != E_DB_OK)
		    break;
	    }
	    if (*r->rcb_f_retval != ADE_TRUE)
	    {
		/* Row does not qualify, go to next one */
		continue;
	    }
	}


	if (DMZ_AM_MACRO(1))
	{
	    TRdisplay("dm1m_get: Ending bid = %d,%d,currenttid = %d,%d\n",
                r->rcb_lowtid.tid_tid.tid_page,
                r->rcb_lowtid.tid_tid.tid_line,
                localtid.tid_tid.tid_page,
                localtid.tid_tid.tid_line);
	    dmd_prrecord(r, rec_ptr);
	}

	/*
	** Notify security system of row access, and request if
	** we can access it. This does any row-level security auditing
	** as well as MAC arbitration.
	**
	** If the internal rcb flag is set then bypass
	** security label checking.  This is only set for
	** DDL operation such as modify and index where you
	** want all records, not just those you dominate.
	*/
	if ( r->rcb_internal_req == 0 && dmf_svcb->svcb_status & SVCB_IS_SECURE )
	{
	    i4       access;
	    i4	     compare;

	    access = DMA_ACC_SELECT;
	    /*
	    ** If get by exact key tell DMA this
	    */
	    if (  (r->rcb_ll_given) &&
		  (r->rcb_ll_given == r->rcb_hl_given) &&
		  (r->rcb_ll_given == t->tcb_keys) &&
		  (r->rcb_ll_op_type == RCB_EQ) &&
		  (r->rcb_hl_op_type == RCB_EQ)
	       )
		    access |= DMA_ACC_BYKEY;

	    s = dma_row_access(access,
			r,
			(char*)rec_ptr,
			(char*)NULL,
			&compare,
			dberr);
	    if (s != E_DB_OK)
		break;
	    if (compare != DMA_ACC_PRIV)
	    {
		continue;
	    }
	}
	/*
	** Copy the row to the caller's buffer if has not already been
	** copied there.  It should have already been copied:
	**     If the table is compressed
	**     If the table is a secondary index
	**     If byte-align restrictions required us to copy it before
	**         making adf compare calls.
	*/
	if (rec_ptr != record)
	    MEcopy(rec_ptr, record_size, record);

	r->rcb_fetchtid.tid_i4 = r->rcb_lowtid.tid_i4;
	r->rcb_state &= ~RCB_FETCH_REQ;
	return (E_DB_OK);
    }

    if (s != E_DB_OK)
    {
	/*	Handle error reporting and recovery. */

	if (dberr->err_code > E_DM_INTERNAL)
	{
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM9261_DM1B_GET);
	}
	return (E_DB_ERROR);
    }

    /*	Must be a get by TID. */

#ifdef xDEBUG
	r->rcb_s_scan++;
#endif

    for (;;)
    {
	/*	26-jun-87 (rogerk)
	**	If retrieve by tid, check to see if the tuple has already been
	**	touched by the current cursor/statement, or if the tuple has
	**	been moved.
	**	If retrieve by tid and fix_page returns error, return BAD_TID.
	**	If retrieve by tid and page is not data page, return BAD_TID.
	**	If retrieve by tid on secondary index and page is not leaf page,
	**	return BAD_TID.
	*/
    	
	if (t->tcb_rel.relstat & TCB_INDEX)
	{
	    localtid.tid_i4 = tid->tid_i4;
	    leaf = r->rcb_other.page;
	    if (leaf && tid->tid_tid.tid_page !=
		DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, leaf))
	    {
		s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other, dberr);
		if (s != E_DB_OK)
		    break;
		leaf = (DMPP_PAGE *) NULL;
	    }
	    if (leaf == NULL)
	    {
	    	s = dm0p_fix_page(r, (DM_PAGENO)tid->tid_tid.tid_page,
		    fix_action | DM0P_NOREADAHEAD, &r->rcb_other, dberr);
		if (s != E_DB_OK)
		{
		    if (dberr->err_code > E_DM_INTERNAL)
			SETDBERR(dberr, 0, E_DM003C_BAD_TID);
		    break;
		}
		leaf = r->rcb_other.page;
	    }
	    leafpage = leaf;

	    /*
	    ** If page is not a leaf page, or if not that many tids on the
	    ** page, then return BAD_TID.
	    */
	    if (((DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, leafpage)  &
		  DMPP_LEAF) == 0) ||
		(tid->tid_tid.tid_line >=
		 DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype,
						 leafpage)))
	    {
		SETDBERR(dberr, 0, E_DM003C_BAD_TID);
		break;
	    }

	    /*
	    ** Retrieve the key and tid from this index entry. 
	    ** Ensure that the resulting record contains
	    ** a TIDP in the last column by manufacturing one here.
	    */
	    rec_ptr = record;
	    leaf_klen = t->tcb_klen - t->tcb_hilbertsize;
	    rec_len = leaf_klen - DM1B_TID_S;
	    s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
			    leafpage, &t->tcb_data_rac,
			    localtid.tid_tid.tid_line,
			    &rec_ptr, &temptid, (i4*)NULL,
			    &rec_len,
			    NULL, NULL, adf_cb);

	    if (s != E_DB_OK)
	    {
		dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, r, leafpage,
				t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
				localtid.tid_tid.tid_line );
		break;
	    }

	    /*
	    ** If this RCB is in deferred update mode and this record indicates
	    ** it is new, then this must be a bad tid.
	    */

	    if ((r->rcb_update_mode == RCB_U_DEFERRED)&&
		dm1cx_isnew(r, leafpage, (i4)tid->tid_tid.tid_line))
	    {
		SETDBERR(dberr, 0, E_DM003C_BAD_TID);
		break;
	    }

	    if (rec_ptr != record)
		MEcopy(rec_ptr, rec_len, record);
	    if (rec_len != leaf_klen)
		MEcopy((char *)&temptid, DM1B_TID_S,
			record + leaf_klen - DM1B_TID_S);

	    r->rcb_currenttid.tid_i4 = localtid.tid_i4;
	    if (leaf != r->rcb_other.page)
	    {
		s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other, dberr);
		if (s != E_DB_OK)
		    break;
	    }
	    tid->tid_i4 = localtid.tid_i4;
	    r->rcb_fetchtid.tid_i4 = r->rcb_lowtid.tid_i4;
	    r->rcb_state &= ~RCB_FETCH_REQ;
	    /*
	    ** If the table being examined has extensions,
	    ** then we must fill in the indirect pointers that
	    ** exist on the page.  For large objects (the only
	    ** ones which exist at present), the coupons used
	    ** in the server contain the rcb of the table open
	    ** instance from which they are fetched.  These
	    ** are filled in, for tables with extensions, below.
	    ** This must be done before the rcb_f_qual
	    ** function is invoked, since it may contain
	    ** requests to examine the large objects in this record.
	    **
	    ** NOTE:  Invoking the following rcb_f_qual
	    ** function may involve indirect recursion within
	    ** DMF (in that a qualification involving a large
	    ** object will go read some collection of the
	    ** extension tables).  Any internal concurrency
	    ** issues must take this into account.
	    */

	    if (t->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS)
	    {
                if (rec_ptr != record)
                {
                  MEcopy(rec_ptr, record_size, record);
                  rec_ptr = record;
                }
		s = dm1c_pget(t->tcb_atts_ptr,
			      r,
			      rec_ptr,
			      dberr);
		if (s)
		{
		    uleFormat(dberr, 0,
			       (CL_ERR_DESC *)NULL,
			       ULE_LOG, NULL,
			       (char *)NULL,
			       (i4)0,
			       (i4 *)NULL,
			       err_code,
			       0);
		    SETDBERR(dberr, 0, E_DM9B01_GET_DMPE_ERROR);
		    break;
		}
	    }

	    /*
	    ** Notify security system of row access, and request if
	    ** we can access it. This does any row-level security auditing
	    ** as well as MAC arbitration.
	    **
	    ** If the internal rcb flag is set then bypass
	    ** security label checking.  This is only set for
	    ** DDL operation such as modify and index where you
	    ** want all records, not just those you dominate.
	    */
	    if ( r->rcb_internal_req == 0 && dmf_svcb->svcb_status & SVCB_IS_SECURE )
	    {
		i4 compare;
		s = dma_row_access(DMA_ACC_SELECT|DMA_ACC_BYTID,
			r,
			(char*)rec_ptr,
			(char*)NULL,
			&compare,
			dberr);

		if ((s != E_DB_OK) || (compare != DMA_ACC_PRIV))
		{
			uleFormat(NULL, E_DM938A_BAD_DATA_ROW, (CL_ERR_DESC *)NULL,
				ULE_LOG,
				NULL, (char *)NULL, (i4)0,
				(i4 *)NULL, err_code, 4,
				sizeof(DB_DB_NAME),
				&t->tcb_dcb_ptr->dcb_name,
				sizeof(DB_TAB_NAME),
				&t->tcb_rel.relid,
				sizeof(DB_OWN_NAME),
				&t->tcb_rel.relowner,
				0, localtid.tid_i4);
			SETDBERR(dberr, 0, E_DM003C_BAD_TID);
			return (E_DB_ERROR);
		}
	    }
	    return (E_DB_OK);
	}

	/*  Not a secondary index. */

	leaf = (DMPP_PAGE *) NULL;
	localtid.tid_i4 = tid->tid_i4;
	if (r->rcb_data.page == NULL)
	{
	    s = dm0p_fix_page(r, (DM_PAGENO )localtid.tid_tid.tid_page,
		(fix_action | DM0P_USER_TID), &r->rcb_data,
		dberr);
	    if (s != E_DB_OK)
	    {
		if (dberr->err_code > E_DM_INTERNAL)
		    SETDBERR(dberr, 0, E_DM003C_BAD_TID);
		break;
	    }
	}
	else if (localtid.tid_tid.tid_page !=
 	    DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
	    r->rcb_data.page))
	{
	    s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_data, dberr);
	    if (s != E_DB_OK)
		break;
	    s = dm0p_fix_page(r, (DM_PAGENO )localtid.tid_tid.tid_page,
		(fix_action | DM0P_USER_TID), &r->rcb_data, dberr);
	    if (s != E_DB_OK)
	    {
		if (dberr->err_code > E_DM_INTERNAL)
		    SETDBERR(dberr, 0, E_DM003C_BAD_TID);
		break;
	    }
	}
	data = r->rcb_data.page;
	record_size = t->tcb_rel.relwid;

	/*
	** Make sure the tid is valid.
	** If the tid specifies an invalid position on the page,
	**   return BAD_TID.
	** If this is not a data page, return BAD_TID.
	** If the tuple has been changed, moved or removed by the same
	**   cursor/statement, return CHANGED_TUPLE.
	** If the tuple at tid used to exist, but has been moved or removed
	**   by a previous cursor/statement, return DELETED_TID.
	*/
	if (((DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, data) &
	      DMPP_DATA) == 0) || (localtid.tid_tid.tid_line >=
	     (u_i2)DMPP_VPT_GET_PAGE_NEXT_LINE_MACRO(t->tcb_rel.relpgtype, data)))
	{
	    SETDBERR(dberr, 0, E_DM003C_BAD_TID);
	    break;
	}

	if (s == E_DB_OK)
	{
	    s = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype,
		t->tcb_rel.relpgsize, data, &localtid, &record_size,
		&rec_ptr, row_ver_ptr, NULL, NULL, (DMPP_SEG_HDR *)NULL);
	}

	if ((r->rcb_update_mode == RCB_U_DEFERRED) &&
	 (*t->tcb_acc_plv->dmpp_isnew)(r, data, (i4)localtid.tid_tid.tid_line))
	{
	    SETDBERR(dberr, 0, E_DM0047_CHANGED_TUPLE);
	    s = E_DB_ERROR;
	    break;
	}

	if (s == E_DB_WARN)
	{
	    SETDBERR(dberr, 0, E_DM0044_DELETED_TID);
	    break;
	}

	/* Additional processing if compressed, altered, or segmented */
	if (s == E_DB_OK && 
	    (t->tcb_data_rac.compression_type != TCB_C_NONE || 
	    row_version != t->tcb_rel.relversion ||
	    t->tcb_seg_rows))
	{
	    s = dm1c_get(r, data, &localtid, record, dberr);
	    rec_ptr = record;
	}

	if (s == E_DB_OK)
	{
	    /*
	    ** Row found at TID.  Copy to row into the callers buffer
	    ** (unless it was already uncompressed into the buffer).
	    */
	    if (rec_ptr != record)
		MEcopy(rec_ptr, record_size, record);

	    tid->tid_i4 = localtid.tid_i4;
	    r->rcb_fetchtid.tid_i4 = r->rcb_lowtid.tid_i4;
	    r->rcb_state &= ~RCB_FETCH_REQ;

	    /*
	    ** If the table being examined has extensions,
	    ** then we must fill in the indirect pointers that
	    ** exist on the page.  For large objects (the only
	    ** ones which exist at present), the coupons used
	    ** in the server contain the rcb of the table open
	    ** instance from which they are fetched.  These
	    ** are filled in, for tables with extensions, below.
	    ** This must be done before the rcb_f_qual
	    ** function is invoked, since it may contain
	    ** requests to examine the large objects in this record.
	    **
	    ** NOTE:  Invoking the following rcb_f_qual
	    ** function may involve indirect recursion within
	    ** DMF (in that a qualification involving a large
	    ** object will go read some collection of the
	    ** extension tables).  Any internal concurrency
	    ** issues must take this into account.
	    */

	    if (t->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS)
	    {
		rec_ptr = record;
		s = dm1c_pget(t->tcb_atts_ptr,
			      r,
			      rec_ptr,
			      dberr);
		if (s)
		{
		    uleFormat(dberr, 0,
			       (CL_ERR_DESC *)NULL,
			       ULE_LOG, NULL,
			       (char *)NULL,
			       (i4)0,
			       (i4 *)NULL,
			       err_code,
			       0);
		    SETDBERR(dberr, 0, E_DM9B01_GET_DMPE_ERROR);
		    break;
		}
	    }

	    /*
	    ** Notify security system of row access, and request if
	    ** we can access it. This does any row-level security auditing
	    ** as well as MAC arbitration.
	    **
	    ** If the internal rcb flag is set then bypass
	    ** security label checking.  This is only set for
	    ** DDL operation such as modify and index where you
	    ** want all records, not just those you dominate.
	    */
	    if ( r->rcb_internal_req == 0 && dmf_svcb->svcb_status & SVCB_IS_SECURE )
	    {
		i4 compare;
		s = dma_row_access(DMA_ACC_SELECT|DMA_ACC_BYTID,
			r,
			(char*)rec_ptr,
			(char*)NULL,
			&compare,
			dberr);

		if ((s != E_DB_OK) || (compare != DMA_ACC_PRIV))
		{
			uleFormat(NULL, E_DM938A_BAD_DATA_ROW, (CL_ERR_DESC *)NULL,
				ULE_LOG,
				NULL, (char *)NULL, (i4)0,
				(i4 *)NULL, err_code, 4,
				sizeof(DB_DB_NAME),
				&t->tcb_dcb_ptr->dcb_name,
				sizeof(DB_TAB_NAME),
				&t->tcb_rel.relid,
				sizeof(DB_OWN_NAME),
				&t->tcb_rel.relowner,
				0, localtid.tid_i4);
			SETDBERR(dberr, 0, E_DM003C_BAD_TID);
			break;
		}
	    }
	    return (E_DB_OK);
	}
	else
	{
	    /*
	    ** DM1C_GET returned an error - this indicates that
	    ** something is wrong with the tuple at the current tid.
	    **
	    ** If user is running with special PATCH flag, then return
	    ** with garbage row, but at least no error.  This should allow
	    ** us to do a delete by tid.
	    */
	    if (DMZ_REC_MACRO(2))
	    {
		tid->tid_i4 = localtid.tid_i4;
		r->rcb_fetchtid.tid_i4 = r->rcb_lowtid.tid_i4;
		r->rcb_state &= ~RCB_FETCH_REQ;
		return (E_DB_OK);
	    }
	    dm1c_badtid(rcb, tid);
	    SETDBERR(dberr, 0, E_DM938B_INCONSISTENT_ROW);
	    if (DMZ_REC_MACRO(1))
		SETDBERR(dberr, 0, E_DM003C_BAD_TID);
	    return (E_DB_ERROR);
	}
    }

    /*	Handle error reporting and recovery. */

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9261_DM1B_GET);
    }
    if (leaf && leaf != r->rcb_other.page)
	s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other, &local_dberr);
    return (E_DB_ERROR);
}

/*{
** Name: dm1m_put - Puts a record into an Rtree table.
**
** Description:
**	This routine puts a record into an Rtree table.
**
**	It can be used for both Rtree base tables and Secondary Indexes.
**	On Secondary Indexes only the Leaf page is updated; on Base Tables
**	the Data page is updated as well.
**
**	The BID parameter specifies the position in the Leaf Page to add
**	the new key and the TID parameter specifies the position Data page
**	where the actual record is to be put (for base tables only). The
**	Leaf and Data pointers must fix the pages identified by the passed
**	in BID and TID values.  These pages will be left fixed upon completion
**	of this routine.
**
**	If the table uses data compression, then the supplied 'record' must
**	have already been compressed.
**
**	The supplied 'key' is expected to NOT be compressed even if key
**	compression is used.
**
**	The 'tid' parameter is required as input only for base tables. While
**	the TID of the related base table row is required by inserts to
**	secondary indexes, the value is computed from the secondary index
**	row rather than passed as a parameter.
**
**	On exit, the 'tid' parameter holds the spot at which the new row
**	was insert.  For base tables this is always the same value was was
**	passed in.  For secondary indexes the 'tid' value returned is actually
**	the BID at which the secondary index key was insert.
**
**      This routine leaves its supplied page pointers fixed upon exit
**	to optimize for set appends which cause consective keys to be updated.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**	bid                             The BID indicating where the
**                                      index record is to be placed.
**      tid                             The TID of the record to insert.
**                                      Only set for data pages.
**      record                          Pointer to the record to insert.
**      record_size                     Size in bytes of record.
**      key                             Pointer to the key for index.
**
** Outputs:
**      tid				Tid of the record for base tables;
**					Bid of the key for secondary indices.
**      err_code			Error status code:
**					E_DM9262_DM1B_PUT
**	Returns:
**	    E_DB_OK
**          E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
** 	27-may-1996 (wonst02)
**          New Page Format Project. Modify page header references to macros.
** 	16-jul-1996 (wonst02)
** 	    Call adjust_mbrs to adjust the parent MBRs after adding to the leaf.
**	11-oct-1996 (wonst02)
**	    Update the rcb ancestor stack with the put bid.
**      29-jul-1997 (stial01)
**          Pass original operation mode DM1C_PUT to dm1r_put for logging.
** 	03-apr-1999 (wonst02)
**	    Remove DMPP_SIZEOF_TUPLE_HDR_MACRO--it doesn't belong.
*/
DB_STATUS
dm1m_put(
    DMP_RCB	    *rcb,
    DMP_PINFO	    *leafPinfo,
    DMP_PINFO	    *dataPinfo,
    DM_TID	    *bid,
    DM_TID	    *tid,
    char	    *record,
    char	    *key,
    i4	    record_size,
    DB_ERROR	    *dberr)
{
    DMP_RCB		*r = rcb;
    DMP_TCB		*t = r->rcb_tcb_ptr;
    DMP_ROWACCESS	*rac;
    DB_STATUS		s;
    bool		base_table;
    i4		    *err_code = &dberr->err_code;
    DMPP_PAGE	    **leaf;

    CLRDBERR(dberr);

    leaf = &leafPinfo->page;

    base_table = (t->tcb_rel.relstat & TCB_INDEX) == 0;

    /*
    ** Get key attribute array for index searches.  For Secondary indexes,
    ** we use the actual data attribute array (since the entire row is
    ** stored in the index).
    **
    ** Also for secondary indexes, we need to calculate the TID of the
    ** actual data row being inserted to the base table.  This TID is
    ** needed for the tid portion of the (key,tid) pair in the leaf update.
    */
    if (base_table)
    {
	rac = &t->tcb_index_rac;
    }
    else
    {
	rac = &t->tcb_data_rac;

	/*
	** Extract the TID value out of the secondary index row.
	*/
	I4ASSIGN_MACRO((*(i4*)(record + record_size - DM1B_TID_S)),tid->tid_i4);
    }

    for (;;)
    {
	/*
	** Verify that the correct leaf is fixed for this insert.
	*/
	if ((*leaf == NULL) ||
	    (DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf) !=
	     bid->tid_tid.tid_page))
	{
	    uleFormat(NULL, E_DM9396_BT_WRONG_PAGE_FIXED, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
		sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
		sizeof(DB_OWN_NAME), &t->tcb_rel.relowner,
		0, bid->tid_tid.tid_page, 0,
		(*leaf ?
		 DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf) :
		 -1), 0, bid->tid_i4, 0, tid->tid_i4);
	    SETDBERR(dberr, 0, E_DM9262_DM1B_PUT);
	    break;
	}
	r->rcb_ancestors[r->rcb_ancestor_level] = *bid;

	/*
	** Do the insertion by placing the key on the leaf page and
	** the record on the data page.
	*/

	/*
	** Insert the record to the leaf's associated data page (if this
	** is a base table).
	*/
	if (base_table)
	{
	    s = dm1r_put(r, dataPinfo, tid, record, (char*)NULL, record_size, 
			(i4)DM1C_MPUT, dberr);
	    if (s != E_DB_OK)
		break;
	}

	/*
	** For both secondary indices and data records, update the Rtree index.
	*/
	s = dm1mxinsert(r, t, t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
		    rac,
		    t->tcb_klen - t->tcb_hilbertsize - DM1B_TID_S,
		    *leaf, key, tid, bid->tid_tid.tid_line,
		    LOG_REQUIRED, MUTEX_REQUIRED, dberr);
	if (s != E_DB_OK)
	    break;

	/*
	** Adjust any other cursors we have on this table that may have had
	** their position pointers affected by this insert.
	*/
	s = dm1m_rcb_update(r, bid, (DM_TID *)NULL, (i4)0,
		    DM1C_MPUT, dberr);
	if (s != E_DB_OK)
	    break;

	if (DMZ_AM_MACRO(3))
	{
	    TRdisplay("dm1m_put: (%d,%d) bid = %d,%d, tid = %d,%d. \n",
		 t->tcb_rel.reltid.db_tab_base, t->tcb_rel.reltid.db_tab_index,
		 bid->tid_tid.tid_page, bid->tid_tid.tid_line,
		 tid->tid_tid.tid_page, tid->tid_tid.tid_line);
	    TRdisplay("dm1m_put: Leaf after insertion\n");
	    dmd_prindex(r, *leaf, (i4)0);
	}

#ifdef xDEV_TEST
	/*
	** Rtree Crash Tests
	*/
	if (DMZ_CRH_MACRO(6))
	{
	    i4		n;
	    TRdisplay("dm1m_put: CRASH! Updates not forced.\n");
	    CSterminate(CS_CRASH, &n);
	}
	if (DMZ_CRH_MACRO(5))
	{
	    i4		n;
	    TRdisplay("dm1m_put: CRASH! Leaf page forced.\n");
	    (VOID)dm0p_unfix_page(r,DM0P_FORCE,&r->rcb_other,dberr);
	    CSterminate(CS_CRASH, &n);
	}
#endif

	/*
	** Secondary index handling:
	**
	** dm1m_put returns the TID of the just-inserted record.  For secondary
	** indexes we return the BID of the entry on the leaf page.
	**
	** For secondary index updates, we must increment rcb_tup_adds to
	** reflect our insert.  For base tables, this was done by dm1r_put.
	*/
	if ( ! base_table)
	{
	    tid->tid_i4 = bid->tid_i4;
	    r->rcb_tup_adds++;
	}
	/*
	** Adjust the MBR of this page and of the parent page, which could
	** percolate all the way to the root level.
	*/
	s = adjust_mbrs(r, *leaf, dberr);
	if (s != E_DB_OK)
	    break;

	return(E_DB_OK);
    }

    /*
    ** Handle error reporting and recovery.
    */
    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9262_DM1B_PUT);
    }
    return (E_DB_ERROR);
} /* dm1m_put */

/*{
** Name: dm1m_replace - Replaces a record in an Rtree table.
**
** Description:
**	This routine replaces a record identified by tid from Rtree
**	table.  This routine assumes that the new space has already
**	been allocated and that duplicates have been checked.
**
**	It can be used for both Rtree base tables and Secondary Indexes.
**	On Secondary Indexes only the Leaf page is updated; on Base Tables
**	the Data page is updated as well.
**
**	If the 'inplace_udpate' flag is specified, then the routine assumes
**	that only the data row needs to be updated.  No change is made to
**	the Rtree index.  This is legal only when the udpate is only to
**	non-keyed columns of the table and the row does not need to move
**	to satisfy the replace.
**
**	The BID parameter specifies the position in the Leaf Page to add
**	the new key and the TID parameter specifies the position Data page
**	where the actual record is to be put (for base tables only). The
**	Leaf and Data pointers must fix the pages identified by the passed
**	in BID and TID values.  Except in the case when a data page is
**	reclaimed (see next paragraph) these pages will be left fixed
**	upon completion of this routine.
**
**	If the row replaced is the last row on a disassociated data page
**	and is moved off of that page, then that data page is made FREE
**	and put into the table's free space maps.  The page remains locked
**	and cannot actually be reused by another transaction until this
**	transaction commits.  The page is also unfixed by this routine.
**
**	If the table uses data compression, then the supplied 'record' must
**	have already been compressed.
**
**	The supplied 'key' is expected to NOT be compressed even if key
**	compression is used.
**
**	If bid and newbid indicate the same leaf page, then the caller need
**	not fix a page into the 'newleaf' parameter (the parameter may point
**	to null, but may not be null itself).  Ditto for tid, newtid and
**	newdata.
**
*******************************************************************************
**	The following comment was in the original version of this file.
**	Its meaning is somewhat unclear, but is left here for archeological
**	reasons - perhaps someday it will shed light on some odd behaviour.
**
**	   "One other case must be handled by this routine. Since it
**	    is possible that a record to be replaced has already been
**	    replaced in a previous call to replace for the current replace
**	    transaction. In that case, if the new record exists and has
**	    been added in the current transaction, it is assumed
**	    optimistically that the present old record(rather than some
**	    other old record was replaced previously to the current new
**	    record. Otherwise, the old record was changed to something else,
**	    and the statement is ambiguous."
**
**	I think this means that it should be OK for a query plan to generate
**	two identical dmr_replace requests (both replaces of the same row
**	to the same new value) during processing of a single update statement.
**	This would presumably occur when some cartesion product was built
**	during Query Processing and no sort is done to eliminate duplicates.
**	This comment seems to say that if two replaces are encountered in
**	the same statement that update the same row to the same new value that
**	no error should be generated.  But if two replaces are encountered in
**	the same statement that update the same row to different new values
**	then an ambiguous replace error should be generated.  In any case,
**	there seems to be no handling of this case in this code.
*******************************************************************************
**
** Inputs:
**      rcb				Record Control Block.
**      bid				Position of old entry on leaf page.
**      tid				Position of old record (if base table).
**      record				A pointer to an area containing
**					the record to replace.
**      record_size			Size of old record in bytes.
**	newleafpage			Leaf page of insertion
**	newdatapage			Data page of insertion
**      newbid				Position of new entry on leaf page.
**      newtid				Position of new record (if base table).
**      nrec				A pointer to an area containing
**					the new record.
**      nrec_size			Size of new record.
**      newkey				Pointer to newkey of nrec.
**
** Outputs:
**      err_code			Error status code:
**					    E_DM9263_DM1B_REPLACE
**	Returns:
**	    E_DB_OK
**          E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
** 	27-may-1996 (wonst02)
**          New Page Format Project. Modify page header references to macros.
**	    Subtract tuple header size in index entry length for leaf pages.
**	15-oct-1996 (wonst02)
**	    Update the rcb ancestor stack with the replace bid.
**	04-dec-1996 (wonst02)
**	    Added ancestor stack parameters. 
**	    Added call to adjust_mbrs() after delete and before insert.
** 	03-apr-1999 (wonst02)
**	    Remove DMPP_SIZEOF_TUPLE_HDR_MACRO--it doesn't belong.
** 	    Adjust ancestor stack when insert position changes.
*/
DB_STATUS
dm1m_replace(
    DMP_RCB         *rcb,
    DM_TID          *bid,
    DM_TID          *tid,
    char            *record,
    i4         record_size,
    DM_TID	    ancestors[],
    i4		    ancestor_level,
    DMP_PINFO	    *newleafPinfo,
    DMP_PINFO	    *newdataPinfo,
    DM_TID          *newbid,
    DM_TID          *newtid,
    char            *nrec,
    i4         nrec_size,
    char            *newkey,
    i4	    inplace_update,
    DB_ERROR        *dberr)
{
    DMP_RCB		*r = rcb;
    DMP_TCB		*t = r->rcb_tcb_ptr;
    DMP_ROWACCESS	*rac;
    DM_TID		datatid;
    u_i4		insert_position;
    DB_STATUS		s;
    bool		base_table;
    DM_TID		new_ancestors[RCB_MAX_RTREE_LEVEL];
    i4			new_ancestor_level;
    i4		    *err_code = &dberr->err_code;
    DMPP_PAGE		**leaf, **newleaf, **data, **newdata;

    CLRDBERR(dberr);

    newleaf = &newleafPinfo->page;
    newdata = &newdataPinfo->page;
    leaf = &r->rcb_other.page;
    data = &r->rcb_data.page;

    base_table = (t->tcb_rel.relstat & TCB_INDEX) == 0;

    /*
    ** Get key attribute array for index searches.  For Secondary indexes,
    ** we use the actual data attribute array (since the entire row is
    ** stored in the index).
    **
    ** Also for secondary indexes, we need to calculate the TID of the
    ** actual data row being inserted to the base table.  This TID is
    ** needed for the tid portion of the (key,tid) pair in the leaf update.
    */
    if (base_table)
    {
	rac = &t->tcb_index_rac;
	datatid = *newtid;
    }
    else
    {
	rac = &t->tcb_data_rac;

	/*
	** Extract the TID value out of the new secondary index row.
	*/
	I4ASSIGN_MACRO((*(i4*)(nrec + nrec_size - DM1B_TID_S)), datatid.tid_i4);
    }

    if (DMZ_AM_MACRO(4))
    {
	TRdisplay("dm1m_replace: oldtid (%d,%d), newtid (%d,%d)\n",
		tid->tid_tid.tid_page, tid->tid_tid.tid_line,
		newtid->tid_tid.tid_page, newtid->tid_tid.tid_line);
	if (t->tcb_data_rac.compression_type == TCB_C_NONE)
	{
	    s = TRdisplay("dm1m_replace: Old record. ");
	    dmd_prrecord(r, record);
	}
    }

    for (;;)
    {
	/*
	** Verify that correct leaf and data page pointers are fixed if
	** they are required:
	**
	**     Old leaf page - must be fixed in rcb_other.page
	**     New leaf page - must be fixed in newleaf
	**           - unless INPLACE replace requires no index update
	**
	**     Old data page - must be fixed in rcb_data.page
	**           - unless table is a secondary index
	**
	**     New data page - must be fixed in newdata
	**           - unless table is a secondary index OR
	**           - unless old and new TIDs point to the same page
	*/

	if (( ! inplace_update) &&
	    ((*leaf == NULL) ||
	     (DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf) !=
	      bid->tid_tid.tid_page)))
	{
	    uleFormat(NULL, E_DM9396_BT_WRONG_PAGE_FIXED, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
		sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
		sizeof(DB_OWN_NAME), &t->tcb_rel.relowner,
		0, bid->tid_tid.tid_page, 0,
		(*leaf ?
	         DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf) :
		 -1),
		0, bid->tid_i4, 0, tid->tid_i4);
	    SETDBERR(dberr, 0, E_DM9263_DM1B_REPLACE);
	    break;
	}

	if (( ! inplace_update) &&
	    ((*newleaf == NULL) ||
	     (DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *newleaf) !=
	      newbid->tid_tid.tid_page)))
	{
	    uleFormat(NULL, E_DM9396_BT_WRONG_PAGE_FIXED, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
		sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
		sizeof(DB_OWN_NAME), &t->tcb_rel.relowner,
		0, newbid->tid_tid.tid_page, 0,
		(*newleaf ?
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *newleaf) : -1),
		0, newbid->tid_i4, 0, newtid->tid_i4);
	    SETDBERR(dberr, 0, E_DM9263_DM1B_REPLACE);
	    break;
	}

	if ((base_table) &&
	    ((*data == NULL) ||
	     (DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *data) !=
	      tid->tid_tid.tid_page)))
	{
	    uleFormat(NULL, E_DM9396_BT_WRONG_PAGE_FIXED, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
		sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
		sizeof(DB_OWN_NAME), &t->tcb_rel.relowner,
		0, tid->tid_tid.tid_page, 0,
		(*data ?
		 DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *data) : -1),
		0, bid->tid_i4, 0, tid->tid_i4);
	    SETDBERR(dberr, 0, E_DM9263_DM1B_REPLACE);
	    break;
	}

	/*
	** Update the record on the data pages (if this is a base table).
	** The dm1r_replace call handles inplace replaces as well as those
	** which move the row across data pages.
	*/
	if (base_table)
	{
	    s = dm1r_replace(r, tid, record, record_size, newdataPinfo, newtid,
			     nrec, nrec_size, (i4)0, 
			     (i4) 0, (i4) 0, dberr);
	    if (s != E_DB_OK)
		break;

	    /*
	    ** Check whether page can be reclaimed.  If the last row has just
	    ** been deleted from a disassociated data page (and we are not
	    ** about to add a new row to the same page), then we can add
	    ** the page back to the free space.  Note: this call may unfix the
	    ** data page pointer.
	    */
	    if (tid->tid_tid.tid_page != newtid->tid_tid.tid_page)
	    {
		s = check_reclaim(r, &r->rcb_data.page, dberr);
		if (s != E_DB_OK)
		    break;
	    }
	}

	/*
	** If an index update is required (not inplace replace) then delete
	** the old leaf entry and insert the new key into its new spot.
	*/
	if ( ! inplace_update)
	{
	    if (DMZ_AM_MACRO(4))
	    {
		TRdisplay("dm1m_replace: Leaf of deletion\n");
		dmd_prindex(r, *leaf, (i4)0);
		if (*newleaf)
		{
		    TRdisplay("dm1m_replace: Leaf of insertion\n");
		    dmd_prindex(r, *newleaf, (i4)0);
		}
	    }

	    new_ancestor_level = r->rcb_ancestor_level;
	    MEcopy(r->rcb_ancestors, sizeof r->rcb_ancestors, new_ancestors);
	    r->rcb_ancestor_level = ancestor_level;
	    MEcopy(ancestors, sizeof r->rcb_ancestors, r->rcb_ancestors);
	    STRUCT_ASSIGN_MACRO(*bid, r->rcb_ancestors[r->rcb_ancestor_level]);
	    s = dm1mxdelete(r, r->rcb_other.page, bid->tid_tid.tid_line,
			LOG_REQUIRED, MUTEX_REQUIRED, dberr);
	    if (s != E_DB_OK)
		break;

#ifdef xDEV_TEST
	    /*
	    ** Rtree Crash Tests
	    */
	    if (DMZ_CRH_MACRO(7))
	    {
		i4		n;
		(VOID) dm0p_unfix_page(r,DM0P_FORCE,&r->rcb_other,dberr);
		TRdisplay("dm1m_replace: CRASH! Mid Update, Old Leaf forced\n");
		CSterminate(CS_CRASH, &n);
	    }
#endif
	    /*
	    ** If the delete happens on a different leaf than the insert,
	    ** adjust the MBR of this page and of the parent page, which could
	    ** percolate all the way to the root level.
	    */
	    if (*leaf != *newleaf)
	    {
	    	s = adjust_mbrs(r, *leaf, dberr);
	    	if (s != E_DB_OK)
    	    	    break;
	    }

	    /*
	    ** Possibly adjust the BID at which to insert the new key.
	    ** If the new and old keys both end up on the same leaf page,
	    ** we must adjust newbid to account for deletion of old bid.
	    */
	    insert_position = newbid->tid_tid.tid_line;
	    if ((DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *newleaf) ==
		 DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf)) &&
		(newbid->tid_tid.tid_line > bid->tid_tid.tid_line))
	    {
		insert_position--;
	    }

	    r->rcb_ancestor_level = new_ancestor_level;
	    MEcopy(new_ancestors, sizeof r->rcb_ancestors, r->rcb_ancestors);
	    r->rcb_ancestors[r->rcb_ancestor_level].tid_tid.tid_page = 
	    	newbid->tid_tid.tid_page;
	    r->rcb_ancestors[r->rcb_ancestor_level].tid_tid.tid_line = 
	    	insert_position;
	    s = dm1mxinsert(r, t, t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
			rac,
			t->tcb_klen - t->tcb_hilbertsize - DM1B_TID_S,
			*newleaf, newkey, &datatid, insert_position,
			LOG_REQUIRED, MUTEX_REQUIRED, dberr);
	    if (s != E_DB_OK)
		break;

	    if (DMZ_AM_MACRO(4))
	    {
		TRdisplay("dm1m_replace: Leaf after deletion\n");
		dmd_prindex(r, *leaf, (i4)0);
		if (*newleaf)
		{
		    TRdisplay("dm1m_replace: Leaf after insertion\n");
		    dmd_prindex(r, *newleaf, (i4)0);
		}
	    }
	    /*
	    ** Adjust the MBR of this page and of the parent page, which could
	    ** percolate all the way to the root level.
	    */
	    if (*leaf != *newleaf)
	    {
	    	s = adjust_mbrs(r, *newleaf, dberr);
	    	if (s != E_DB_OK)
    	    	    break;
	    }
	}

	/*
	** Find any other RCB's whose table position pointers may have been
	** affected by this replace and update their positions accordingly
	** (and/or any RCB's which must refetch the updated row).
	**
	** Note that this is called with the original "newbid" value rather
	** than the possibly-adjusted "insert_position" value.  The rcb_update
	** code takes same-page replaces into account in its calculations.
	*/
	s = dm1m_rcb_update(r, bid, newbid, (i4)0, DM1C_MREPLACE,dberr);
	if (s != E_DB_OK)
    	    break;

#ifdef xDEV_TEST
	/*
	** Rtree Crash Tests
	*/
	if (DMZ_CRH_MACRO(8))
	{
	    i4		n;
	    if ( ! inplace_update)
	    {
		(VOID) dm0p_unfix_page(r,DM0P_FORCE,&r->rcb_other,dberr);
		(VOID) dm0p_unfix_page(r,DM0P_FORCE,newleafPinfo,dberr);
	    }
	    TRdisplay("dm1m_replace: CRASH! Leafs forced\n");
	    CSterminate(CS_CRASH, &n);
	}
	if (DMZ_CRH_MACRO(9))
	{
	    i4		n;
	    TRdisplay("dm1m_replace: CRASH! Updates not forced\n");
	    CSterminate(CS_CRASH, &n);
	}
#endif

	return (E_DB_OK);
    }

    /*
    ** Handle error reporting and recovery.
    */
    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9263_DM1B_REPLACE);
    }
    return (E_DB_ERROR);
}

/*{
** Name: dm1m_rcb_update - Updates all RCB's of a single transaction.
**
** Description:
**    This routine updates all the RCB's associated with one transaction.
**    The information in the RCB relating to current postion may change
**    depending upon the action taken with the current RCB.  For example
**    if the current RCB deletes a record, the leaf page line table is
**    compressed.  Therefore, all other RCB's associated with this transaction
**    must have their current bid pointers updated if they were pointing to
**    the line table entry deleted or to any other line table entry
**    on this page greater than the one deleted.  A similar operation must occur
**    for inserts and splits.  Only the current position pointer
**    is affected since the determination of when the search(scan) is
**    completed is based on a key value and not a limit bid.
**
**    In addition this is the routine which determines if the record
**    previously fetched is still valid.  If one cursor fetched
**    a record (tid = 2,3) and another cursor fetches the same record
**    each is pointing to 2,3 for the next operation.  If the first one
**    does a replace which moves it to 14,4, them the other one must
**    know to refetch the record at 14,4 instead of using the one
**    it fetched previously.  The is handled by tracking the fetched tid
**    in rcb_fetchtid and by indicating the state with rcb_state
**    set to RCB_FETCH_REQ on.  These are both cleared by the
**    next get.
**
**    This routine has code to handle tracking replaces and
**    deletes of records already fetched.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      bid                             A pointer to a TID indicating
**                                      position on leaf page for
**                                      index record changed.
**      newbid                          A pointer to a TID indicating
**                                      position on leaf page for
**                                      replaces which move the record.
**                                      Or the newbid after a split.
**	split_pos			Entry position to split on if opflag
**					is DM1C_MSPLIT.
**      opflag                          A value indicating type of operation
**                                      which changed an index record. One of:
**                                          DM1C_MDELETE
**                                          DM1C_MPUT
**                                          DM1C_MREPLACE
**                                          DM1C_MSPLIT
**
** Outputs:
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**                                      The error codes are:
**
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
** 	27-may-1996 (wonst02)
**          New Page Format Project. Modify page header references to macros.
*/
DB_STATUS
dm1m_rcb_update(
    DMP_RCB        *rcb,
    DM_TID         *bid,
    DM_TID         *newbid,
    i4         split_pos,
    i4         opflag,
    DB_ERROR       *dberr)
{
    DMP_RCB	*r = rcb;
    DM_TID      localbid;
    DM_TID      localnewbid;
    DMP_RCB     *next_rcb = r;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    localbid = *bid;

    switch (opflag)
    {
    case DM1C_MDELETE:
    {
	do
	{
	    /*
	    ** If this is a readnolock RCB that is positioned on its own
	    ** copy of the changed page, then don't update the position
	    ** pointers since the readnolock copy was not changed.
	    */
	    if ((next_rcb->rcb_k_duration & RCB_K_READ) &&
		(next_rcb->rcb_other.page != NULL) &&
		(next_rcb->rcb_other.page == next_rcb->rcb_1rnl_page ||
		 next_rcb->rcb_other.page == next_rcb->rcb_2rnl_page) &&
		(DMPP_VPT_GET_PAGE_PAGE_MACRO(r->rcb_tcb_ptr->tcb_rel.relpgtype,
		next_rcb->rcb_other.page) == localbid.tid_tid.tid_page))
	    {
		 continue;
	    }
	    if (next_rcb->rcb_fetchtid.tid_tid.tid_page
                                      == localbid.tid_tid.tid_page
             && next_rcb->rcb_fetchtid.tid_tid.tid_line
                                      == localbid.tid_tid.tid_line)
	    {
		/* For any rcb which has already fetched this
		** tid, then mark it as deleted by placing an
                ** illegal tid in the fetched tid.
                */
		next_rcb->rcb_fetchtid.tid_tid.tid_page = 0;
		next_rcb->rcb_fetchtid.tid_tid.tid_line = DM_TIDEOF;
		next_rcb->rcb_state |= RCB_FETCH_REQ;
	    }

	    /* Adjust the next line number for next get. */

	    if (next_rcb->rcb_lowtid.tid_tid.tid_page
                                      == localbid.tid_tid.tid_page
	     && next_rcb->rcb_lowtid.tid_tid.tid_line != DM_TIDEOF
             && next_rcb->rcb_lowtid.tid_tid.tid_line
                                      >= localbid.tid_tid.tid_line)
		next_rcb->rcb_lowtid.tid_tid.tid_line--;
	    if (next_rcb->rcb_fetchtid.tid_tid.tid_page
                                      == localbid.tid_tid.tid_page
	     && next_rcb->rcb_fetchtid.tid_tid.tid_line != DM_TIDEOF
             && next_rcb->rcb_fetchtid.tid_tid.tid_line
                                      > localbid.tid_tid.tid_line)
		next_rcb->rcb_fetchtid.tid_tid.tid_line--;

	} while ((next_rcb = next_rcb->rcb_rl_next) != r);
        return (E_DB_OK);
    }
    case DM1C_MPUT:
    {
	do
	{
	    /*
	    ** If this is a readnolock RCB that is positioned on its own
	    ** copy of the changed page, then don't update the position
	    ** pointers since the readnolock copy was not changed.
	    */
	    if ((next_rcb->rcb_k_duration & RCB_K_READ) &&
		(next_rcb->rcb_other.page != NULL) &&
		(next_rcb->rcb_other.page == next_rcb->rcb_1rnl_page ||
		 next_rcb->rcb_other.page == next_rcb->rcb_2rnl_page) &&
		(DMPP_VPT_GET_PAGE_PAGE_MACRO(r->rcb_tcb_ptr->tcb_rel.relpgtype,
		next_rcb->rcb_other.page) == localbid.tid_tid.tid_page))
	    {
		 continue;
	    }

	    if (next_rcb->rcb_lowtid.tid_tid.tid_page
                                      == localbid.tid_tid.tid_page
	     && next_rcb->rcb_lowtid.tid_tid.tid_line != DM_TIDEOF
             && next_rcb->rcb_lowtid.tid_tid.tid_line
                                      >= localbid.tid_tid.tid_line)
		next_rcb->rcb_lowtid.tid_tid.tid_line++;
	    if (next_rcb->rcb_fetchtid.tid_tid.tid_page
                                      == localbid.tid_tid.tid_page
	     && next_rcb->rcb_fetchtid.tid_tid.tid_line != DM_TIDEOF
             && next_rcb->rcb_fetchtid.tid_tid.tid_line
				      >= localbid.tid_tid.tid_line)
		next_rcb->rcb_fetchtid.tid_tid.tid_line++;
	} while ((next_rcb = next_rcb->rcb_rl_next) != r);
        return (E_DB_OK);
    }
    case DM1C_MSPLIT:
    {
	localnewbid.tid_i4 = newbid->tid_i4;
	do
	{
	    /*
	    ** If this is a readnolock RCB that is positioned on its own
	    ** copy of the changed page, then don't update the position
	    ** pointers since the readnolock copy was not changed.
	    */
	    if ((next_rcb->rcb_k_duration & RCB_K_READ) &&
		(next_rcb->rcb_other.page != NULL) &&
		(next_rcb->rcb_other.page == next_rcb->rcb_1rnl_page ||
		 next_rcb->rcb_other.page == next_rcb->rcb_2rnl_page) &&
		(DMPP_VPT_GET_PAGE_PAGE_MACRO(r->rcb_tcb_ptr->tcb_rel.relpgtype,
		next_rcb->rcb_other.page) == localbid.tid_tid.tid_page))
	    {
		 continue;
	    }

	    if (next_rcb->rcb_lowtid.tid_tid.tid_page
                                           == localbid.tid_tid.tid_page
	     && next_rcb->rcb_lowtid.tid_tid.tid_line != DM_TIDEOF
             && (i4)next_rcb->rcb_lowtid.tid_tid.tid_line
	    				   >= (i4)split_pos)
	    {
		next_rcb->rcb_lowtid.tid_tid.tid_line -= split_pos;
                next_rcb->rcb_lowtid.tid_tid.tid_page =
                             localnewbid.tid_tid.tid_page;
	    }
	    if (next_rcb->rcb_fetchtid.tid_tid.tid_page
                                      == localbid.tid_tid.tid_page
	     && next_rcb->rcb_fetchtid.tid_tid.tid_line != DM_TIDEOF
             && (i4)next_rcb->rcb_fetchtid.tid_tid.tid_line
	                              >= (i4)split_pos)
	    {
		next_rcb->rcb_fetchtid.tid_tid.tid_line -= split_pos;
                next_rcb->rcb_fetchtid.tid_tid.tid_page =
                             localnewbid.tid_tid.tid_page;
	    }
	} while ((next_rcb = next_rcb->rcb_rl_next) != r);
        return (E_DB_OK);
    }
    case DM1C_MREPLACE:
    {
	/* This is very tricky code.  The line table in
        ** Rtrees is kept sorted, therefore it compresses
        ** or expands depending on the value of old and new
        ** bids.  If the old and new bids end up on the
        ** same page it is even more complicated since
        ** the first part of the replace (old) can
        ** cause the linetable entries to compress, thereby
        ** changing the scan and fetch bids.  Then if
        ** it is placed on same page, it causes the line
        ** table to expand, thereby changing the scan
	** and fetch bids again.  In any case unless you
        ** understand this code very well, you should
        ** probably not try to change it.
	*/

	do
	{
	    /*
	    ** If this is a readnolock RCB that is positioned on its own
	    ** copy of the changed page, then don't update the position
	    ** pointers since the readnolock copy was not changed.
	    */
	    if ((next_rcb->rcb_k_duration & RCB_K_READ) &&
		(next_rcb->rcb_other.page != NULL) &&
		(next_rcb->rcb_other.page == next_rcb->rcb_1rnl_page ||
		 next_rcb->rcb_other.page == next_rcb->rcb_2rnl_page) &&
		(DMPP_VPT_GET_PAGE_PAGE_MACRO(r->rcb_tcb_ptr->tcb_rel.relpgtype,
		next_rcb->rcb_other.page) == localbid.tid_tid.tid_page))
	    {
		 continue;
	    }

	    localnewbid.tid_i4 = newbid->tid_i4;
	    if (localbid.tid_i4 != localnewbid.tid_i4)
	    {

		/* If the fetchtid is set to the one we are
                ** replacing, then update fetch to new one.
                */

		if  (next_rcb->rcb_fetchtid.tid_tid.tid_page
                              == localbid.tid_tid.tid_page
                                     &&
			 next_rcb->rcb_fetchtid.tid_tid.tid_line
			      == localbid.tid_tid.tid_line)
		{
		    next_rcb->rcb_fetchtid.tid_tid.tid_page =
                                       localnewbid.tid_tid.tid_page;
		    next_rcb->rcb_fetchtid.tid_tid.tid_line =
                                       localnewbid.tid_tid.tid_line;

		    /* Update low only depending on values of
                    ** old and new. */

		    if (next_rcb->rcb_lowtid.tid_tid.tid_page ==
                               localbid.tid_tid.tid_page
			&& next_rcb->rcb_lowtid.tid_tid.tid_line != DM_TIDEOF
			&& next_rcb->rcb_lowtid.tid_tid.tid_line >=
                               localbid.tid_tid.tid_line)
		    {
			next_rcb->rcb_lowtid.tid_tid.tid_line--;
		    }
		    /* If old and new are on the same page
                    ** and new is > old, then new would have
                    ** been shifted by the delete, so do it.
                    */
		    if (localnewbid.tid_tid.tid_page ==
                             localbid.tid_tid.tid_page
			&&
			localnewbid.tid_tid.tid_line >
			     localbid.tid_tid.tid_line)
		    {
			localnewbid.tid_tid.tid_line--;
		    }
		    if ((next_rcb->rcb_lowtid.tid_tid.tid_page ==
                               localnewbid.tid_tid.tid_page) &&
			(next_rcb->rcb_lowtid.tid_tid.tid_line != DM_TIDEOF) &&
			(next_rcb->rcb_lowtid.tid_tid.tid_line >=
                               localnewbid.tid_tid.tid_line))

		    {
			next_rcb->rcb_lowtid.tid_tid.tid_line++;
		    }


		}
                else
		{
		    /* If fetch tid not the same, update both
                    ** low and fetch based on old and new values. */

		    if (next_rcb->rcb_lowtid.tid_tid.tid_page ==
                               localbid.tid_tid.tid_page
			&& next_rcb->rcb_lowtid.tid_tid.tid_line != DM_TIDEOF
			&& next_rcb->rcb_lowtid.tid_tid.tid_line >=
                               localbid.tid_tid.tid_line)
		    {
			next_rcb->rcb_lowtid.tid_tid.tid_line--;
		    }

		    /* If old and new are on the same page
                    ** and new is > old, then new would have
                    ** been shifted by the delete, so do it.
                    */

		    if (next_rcb->rcb_fetchtid.tid_tid.tid_page ==
                               localbid.tid_tid.tid_page
			&& next_rcb->rcb_fetchtid.tid_tid.tid_line != DM_TIDEOF
			&& next_rcb->rcb_fetchtid.tid_tid.tid_line >=
                               localbid.tid_tid.tid_line)
		    {
			next_rcb->rcb_fetchtid.tid_tid.tid_line--;
		    }
		    if (localnewbid.tid_tid.tid_page ==
                             localbid.tid_tid.tid_page
			&&
			localnewbid.tid_tid.tid_line >
			     localbid.tid_tid.tid_line)
		    {
			localnewbid.tid_tid.tid_line--;
		    }
		    if ((next_rcb->rcb_lowtid.tid_tid.tid_page ==
                               localnewbid.tid_tid.tid_page) &&
			(next_rcb->rcb_lowtid.tid_tid.tid_line != DM_TIDEOF) &&
			(next_rcb->rcb_lowtid.tid_tid.tid_line >=
                               localnewbid.tid_tid.tid_line))
		    {
			next_rcb->rcb_lowtid.tid_tid.tid_line++;
		    }

		    if ((next_rcb->rcb_fetchtid.tid_tid.tid_page ==
                               localnewbid.tid_tid.tid_page) &&
			(next_rcb->rcb_fetchtid.tid_tid.tid_line != DM_TIDEOF)
			&& (next_rcb->rcb_fetchtid.tid_tid.tid_line >=
                               localnewbid.tid_tid.tid_line))
		    {
			next_rcb->rcb_fetchtid.tid_tid.tid_line++;
		    }
		}
		next_rcb->rcb_state |= RCB_FETCH_REQ;
	    }
	    else
	    {

		/* Just doing a replace in place, update
                ** the rcb state in those who have it fetched.
                */
		if ((next_rcb->rcb_lowtid.tid_tid.tid_page ==
                             localbid.tid_tid.tid_page
			&& next_rcb->rcb_lowtid.tid_tid.tid_line ==
                             localbid.tid_tid.tid_line)
		    ||
		     (next_rcb->rcb_fetchtid.tid_tid.tid_page ==
                             localbid.tid_tid.tid_page
			&& next_rcb->rcb_fetchtid.tid_tid.tid_line ==
                             localbid.tid_tid.tid_line)
		    )
		    next_rcb->rcb_state |= RCB_FETCH_REQ;
	    }
	} while ((next_rcb = next_rcb->rcb_rl_next) != r);
        return (E_DB_OK);
    }
    default:
	TRdisplay("DM1B_RCB_UPDATE called with unknown mode\n");
	SETDBERR(dberr, 0, E_DM9C25_DM1B_RCB_UPDATE);
	return (E_DB_ERROR);
    }
}

/*{
** Name: dm1m_search - Rtree traversal routine.
**
** Description:
**      This routine searches the Rtree index pages starting from
**      the root page and finds the leaf page that should contain the
**      key that is specified.   The mode and direction of the
**      search can significantly alter the searching technique
**      and may alter the types of locks obtained during the search.
**
**	As you descend the tree, you request a lock on the page only
**	while looking at it (no ladder locking). You release the lock
**	before locking the child page. After obtaining the index or
**	leaf page, you must check to make sure it still contains the
**	key value searched for. If it does not, then the sideways
**      pointer must be used to obtain the correct page.  This works
**      because we never do joins, we always split to the right,
**      and both index and leaf pages have sideways pointers.
**
**      If the mode indicates pages should
**      be split as you are searching, then write_physical
**      locks are set, otherwise read_physical  locks are set on the
**      index pages.   Read or write locks are set on the leaf pages
**      depending on the accessmode indicated in the RCB.
**
**      At the completion of this routine only the leaf page
**      associated with the lowbid remains locked.
**      The lowbid is the tid of the first leaf page entry associated
**      with the key specified.
**
**      If mode is FIND, then this routine determines if an exact
**      match was found.  Since the leaf can contain entries for a
**      range of keys the leaf page may be the one fitting the
**      search criteria, but it may not contain an existing entry
**      for the key.  If the page does not have an exact match
**      then an error is returned indicating that the key was
**      not found.  It still keeps this page locked, since it is
**      the leaf page the key should be placed on or found on.
**
**      If mode is OPTIM for optimistic, the search progresses
**      assuming that splits will not be required.  If the leaf
**      page is full then an error indicating no more room is
**      returned to the caller.
**
**      In all cases only the page indicated by leaf is fixed
**      on exit.  A page is fixed into 'leaf' under the following
**	conditions:
**	    E_DB_OK			page is fixed into leaf
**	    E_DB_ERROR/E_DM0056_NOPART	page is fixed into leaf
**	    E_DB_ERROR/E_DM0057_NOROOM	page is fixed into leaf
**	    <anything else>		no page is fixed.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      key                             Pointer to the key value.
**      keys_given                      Value indicating number of
**                                      attributes in key. Note that this is
**					the number of attributes to be used for
**					searching, which--for Rtree--is the
**					same as the number of attributes in the
**					index entry.
**      mode                            Value indicating type of search
**                                      to perform.  Must be DM1C_SPLIT,
**                                      DM1C_JOIN, DM1C_FIND,
**                                      DM1C_EXTREME, DM1C_RANGE,
**                                      DM1C_OPTIM, DM1C_SETTID,
**                                      DM1C_POSITION or DM1C_TID.
**					(JOIN,SETTID, and POSITION are obsolete)
**      direction                       Value indicating direction of
**                                      search.  Must be DM1C_PREV,
**                                      DM1C_NEXT, or DM1C_EXACT.
**
** Outputs:
**      leaf                            Pointer to a pointer used to
**                                      identify leaf page locked and
**                                      fixed into memory.
**      bid                          	Pointer to an area to return
**                                      bid of qualifying leaf record.
**      tid                             Pointer to an area to return
**                                      the tid of the matching tuple
**                                      on an exact search.  If mode
**                                      is DM1C_SETTID, then it is the tid
**                                      want to position to.
**      err_code                        Pointer to an area to return
**                                      the following errors when
**                                      E_DB_ERROR is returned.
**                                      E_DM0056_NOPART
**                                      E_DM0057_NOROOM
**
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**          E_DB_WARN
**	Exceptions:
**	    none
**
** Side Effects:
**	    Leaves the page returned in leaf fixed and locked.
**
** History:
** 	27-may-1996 (wonst02 for thaju02)
**          New Page Format Project. Modify page header references to macros.
**      03-june-1996 (wonst02 for stial01)
**          Added DMPP_TUPLE_INFO argument to dm1cxget()
**	23-Oct-1996 (wonst02)
**	    Requalify each page, in case it split while waiting for it.
**	12-dec-1996 (wonst02)
**	    New parameters added to find_next_leaf().
**      12-jun-97 (stial01)
**          dm1m_search() Pass tlv to dm1cxget instead of tcb.
**	14-nov-1997 (nanpr01)
**	    Tuple header was getting added twice for larger pages.
** 	03-apr-1999 (wonst02)
**	    Remove DMPP_SIZEOF_TUPLE_HDR_MACRO--it doesn't belong.
** 	    Subtract hilbert size and tid size from dm1cxmax_length().
**/
DB_STATUS
dm1m_search(
    DMP_RCB            *rcb,
    char               *key,
    i4            keys_given,
    i4            mode,
    i4            direction,
    DMP_PINFO          *leafPinfo,
    DM_TID             *bid,
    DM_TID             *tid,
    DB_ERROR           *dberr)
{
    DMP_RCB	 *r = rcb;
    DMP_TCB      *t = r->rcb_tcb_ptr;   /* Table control block. */
    i4		 pgsize = t->tcb_rel.relpgsize;
    i4           pgtype = t->tcb_rel.relpgtype;
    DM_TID       *tidp;                 	/* TID pointer. */
    DM_TID       child;                 /* TID of child. */
    i4      local_err_code;        /* Local error code. */
    i4      index_fix_action;	/* Type of action for fixing index
                                        ** pages into memory. */
    i4	 leaf_fix_action;	/* action for fixing leaf pages. */
    i4		 pos;                   /* TID line position. */
    DB_ATTS      **keyatts = t->tcb_key_atts;
                                        /* Attributes used in key. */
    DB_STATUS    s, state;     		/* Routine return status. */
    i4      xmode;                 /* local search mode. */
    char	 rangekey[DB_MAXRTREE_KEY];/* Range key of parent */
    i4	 compare;
    ADF_CB	 *adf_cb;
    DMP_ROWACCESS *rac;
    i4	 at_sprig_level;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;
    DMP_PINFO	parentPinfo, currentPinfo, leafpagePinfo;
    DMPP_PAGE	**parent, **current, **leafpage;

    CLRDBERR(dberr);

    /* Initialize pointers in case of errors. */

    adf_cb = r->rcb_adf_cb;

    parent = &parentPinfo.page;
    current = &currentPinfo.page;
    leafpage = &leafpagePinfo.page;
    *current = NULL;
    *parent = NULL;
    *leafpage = NULL;



    /*
    ** Set up attributes array and count correctly, depending on whether
    ** this is a secondary index or not
    */
    if (t->tcb_rel.relstat & TCB_INDEX)
    {
	rac = &t->tcb_data_rac;
    }
    else
    {
	rac = &t->tcb_index_rac;
    }

    /*
    ** Index pages will be temporarily locked for reading.
    ** Index reads use NOREADAHEAD, because the index is not
    ** sequential, and LOCKREAD, to lock the index page even if
    ** read = nolock is specified for this access.
    */
    index_fix_action = DM0P_RPHYS | DM0P_NOREADAHEAD | DM0P_LOCKREAD;

    leaf_fix_action = DM0P_READ;
    if (r->rcb_access_mode == RCB_A_WRITE)
	leaf_fix_action = DM0P_WRITE;
    if ((r->rcb_state & RCB_READAHEAD) == 0)
	leaf_fix_action |= DM0P_NOREADAHEAD;

    if ( leafPinfo && leafPinfo->page )
    {
# ifdef XDEBUG
	TRdisplay("Data page fixed in call to dm1m_search, unfixing...\n");
# endif
	s = dm0p_unfix_page(r, DM0P_UNFIX, leafPinfo, dberr);
	if (s)
	    return(s);
    }

    for (;;)
    {
	/* Get root page to start search. */

	s = dm0p_fix_page(r, (DM_PAGENO)DM1B_ROOT_PAGE,
	    		  index_fix_action, &parentPinfo, dberr);
	if (s != E_DB_OK)
	    break;
	r->rcb_ancestor_level = 0;
	tidp = &r->rcb_ancestors[r->rcb_ancestor_level];
	tidp->tid_tid.tid_page = (DM_PAGENO)DM1B_ROOT_PAGE;
	tidp->tid_tid.tid_line = DM_TIDEOF;

	if (mode == DM1C_RANGE)
	{
	    currentPinfo = parentPinfo;
	    bid->tid_i4 = tidp->tid_i4;
	    s = find_first(r, current, bid, tid, dberr);
	    if (s != E_DB_OK)
		break;
	    if ( leafPinfo )
		*leafPinfo = currentPinfo;     
	    return E_DB_OK;
	}

	/*
	** Search down tree until find the index record indicating which
	** leaf page should contain this key.  The index page is the
	** parent page, and the leaf is the child.
	*/

	xmode = mode;
	if (mode == DM1C_TID)
	    xmode = DM1C_FIND;
	do
	{
	    s = dm1mxsearch(r, *parent, xmode, direction, key, adf_cb, 
	    		    t->tcb_acc_klv,
			    pgtype, pgsize, &child, &pos, dberr);
	    if (s != E_DB_OK && dberr->err_code != E_DM0056_NOPART)
		break;
	    tidp->tid_tid.tid_line = pos;

	    /*
	    ** Get the child page and fix it for read. We release
	    ** the index page while we wait for the child page lock.
	    */
	    at_sprig_level =
		(DM1B_VPT_GET_PAGE_STAT_MACRO(pgtype, *parent) & DMPP_SPRIG) != 0;

	    s = dm0p_unfix_page(r, DM0P_RELEASE, &parentPinfo, dberr);
	    if (s != E_DB_OK)
		break;

	    s = dm0p_fix_page(r, (DM_PAGENO)child.tid_tid.tid_page,
			(at_sprig_level ? leaf_fix_action : index_fix_action),
			&currentPinfo, dberr);
	    if (s != E_DB_OK)
		break;

	    tidp = &r->rcb_ancestors[++r->rcb_ancestor_level];
	    tidp->tid_tid.tid_page = child.tid_tid.tid_page;
	    tidp->tid_tid.tid_line = DM_TIDEOF;

	    /*
	    ** If the parent page was a sprig page, then this page had better
	    ** be a leaf page; otherwise, it should be a sprig or index page.
	    */
	    if (at_sprig_level)
	    {
		if ((DM1B_VPT_GET_PAGE_STAT_MACRO(pgtype, *current) &DMPP_LEAF) == 0)
		{
		    dm1cxlog_error(E_DM93EA_NOT_A_LEAF_PAGE, r, *current,
				   pgtype, pgsize, 0);
		    s = E_DB_ERROR;
		    SETDBERR(dberr, 0, E_DM9264_DM1B_SEARCH);
		    break;
		}
	    } /* if */
	    else if ((DM1B_VPT_GET_PAGE_STAT_MACRO(pgtype, *current) &
		      (DMPP_SPRIG|DMPP_INDEX)) == 0)
	    {
		dm1cxlog_error(E_DM93ED_WRONG_PAGE_TYPE, r, *current, 
				pgtype, pgsize, 0);
		SETDBERR(dberr, 0, E_DMF013_DM1B_CURRENT_INDEX);
		s = E_DB_ERROR;
		break;
	    } /* else if */
	    /*
	    ** Make sure this is the page containing the key. If the page
	    ** split, the key may be on the next page.
	    */
	    s = requalify_page(r, current, key, mode, direction, dberr);
	    if (s)
	    	break;
	    /*
	    ** Now advance to the next level and iterate back until
	    ** we find the leaf page.
	    */
	    parentPinfo = currentPinfo;
	    *current = NULL;
	} while (!(DM1B_VPT_GET_PAGE_STAT_MACRO(pgtype, *parent) & DMPP_LEAF));

	if (s != E_DB_OK)
	    break;

	currentPinfo = parentPinfo;
	*parent = NULL;
	leafpagePinfo = currentPinfo;

	if (DMZ_AM_MACRO(9))
	{
	    TRdisplay("dm1m_search: reached bottom of search, leaf is %d\n",
		     DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *current));
	    dmd_prindex(r, *current, (i4)0);
	}

	/*
	** For exact searches, find the leaf and check if the key
	** actually exists on the leaf page.  (The previous search only
	** qualified that this is the leaf which might contain the key).
	**
	** If the key is found then the position is set to point to it.  If
	** no key is found then position is set to the spot at which that
	** key value would belong.
	*/
	state = E_DB_OK;
	if (direction == DM1C_EXACT)
	{
	    for (;;)
	    {
		DM_PAGENO	nextpageno;

	    	s = dm1mxsearch(r, *leafpage, mode, direction, key, adf_cb, 
				t->tcb_acc_klv,
				pgtype, pgsize, tid, &pos, dberr);
	    	if ((mode != DM1C_TID) || 	/* Not looking for exact TID */
		    (s == E_DB_OK) || 		     /* or Found exact match */
		    (dberr->err_code != E_DM0056_NOPART))   /* or Something rotten */
		    break;
		/*
		** Searching for a TID, but it wasn't on this leaf page...
		** If the Hilbert value of the search key = LHV of this page,
		** keep looking on next page.
		*/
	    	nextpageno = DM1B_VPT_GET_BT_NEXTPAGE_MACRO(pgtype, *leafpage);
	    	if (nextpageno == 0)
		    break;
	    	dm1m_get_range(pgtype, pgsize, *leafpage,t->tcb_acc_klv, NULL, rangekey);
	    	s = dm1mxkkcmp(pgtype, pgsize, *leafpage, adf_cb,
			 t->tcb_acc_klv, key, rangekey, &compare);
	    	if (s != E_DB_OK)
	    	{
		   uleFormat(NULL, E_DMF012_ADT_FAIL, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		   	      (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		    SETDBERR(dberr, 0, E_DM93D0_BALLOCATE_ERROR);
		   break;
	    	}
 	    	if (compare < 0)
		{
		    SETDBERR(dberr, 0, E_DM0056_NOPART);
		    s = E_DB_ERROR;
	    	    break;
		}
	    	s = dm0p_unfix_page(r, DM0P_UNFIX, &currentPinfo, dberr);
	    	if (s)
		    break;
	    	tidp->tid_tid.tid_page = nextpageno;
	    	tidp->tid_tid.tid_line = (DM_TIDEOF);
	    	s = dm0p_fix_page(r, tidp->tid_tid.tid_page, leaf_fix_action,
	            		  &currentPinfo, dberr);
	    	if (s)
		    break;
	    	leafpagePinfo = currentPinfo;
	    } /* for (;;) */

	    /*
	    ** If the above search failed with an unexpected error, then
	    ** branch down to the error handling.
	    */
	    if ((s != E_DB_OK) && (dberr->err_code != E_DM0056_NOPART))
		break;

	    /* Save error code for return. */

	    state = dberr->err_code;
	    leafpagePinfo = currentPinfo;
	} /* if (direction == DM1C_EXACT) */

	CLRDBERR(dberr);

	/*
	** If mode is EXTREME, then the position must be set
	** to the first on the page for PREV and the last
	** on page for NEXT.
	*/

	if (mode == DM1C_EXTREME)
	{
	    if (direction == DM1C_PREV)
		pos = 0;
	    else
		pos = DM1B_VPT_GET_BT_KIDS_MACRO(pgtype, *leafpage) - 1;
	}
	else if (mode == DM1C_LAST)
	{
	    pos = DM1B_VPT_GET_BT_KIDS_MACRO(pgtype, *leafpage);
	}
	tidp->tid_tid.tid_line = pos;

	/*
	** If doing an optimistic search and there is no more room on the leaf
	** page selected, return an error indicating this.
	**
	** If doing a split search and there is no more room on the leaf
	** page selected, initiate a leaf overflow. The overflow attempts
	** to push some of this leaf's entries onto one of its siblings.
	** If all the siblings are full, then we must split the leaf page(s).
	** This may percolate upward, causing a split at a higher index level.
	*/
	if (mode == DM1C_OPTIM || mode == DM1C_SPLIT)
	{
	    if (! dm1cxhas_room(pgtype, pgsize, *leafpage, 
		    DM1B_INDEX_COMPRESSION(r),
		    (i4)100 /* fill factor */,
		    t->tcb_kperleaf, dm1cxmax_length( r, *leafpage ) - 
		    	t->tcb_hilbertsize - DM1B_TID_S))
	    {
    	    	DMPP_PAGE	*sibling[MAX_SPLIT_POLICY + 1];
	    						/* Sibling page array */
# define 	MAX_LOCKS 	RCB_MAX_RTREE_LEVEL * (MAX_SPLIT_POLICY + 1)
		DMPP_PAGE	*locks[MAX_LOCKS + 1];	/* Locked page array  */
		DMPP_PAGE	**locked;
	    	i4		numsiblings;	        /* Number of siblings */
	    	DMPP_PAGE	*newsib = NULL;		/* Newly split sibling*/
		char		splitkey[DB_MAXRTREE_KEY];/* Key at split pos.*/
		LG_LSN	    	lsn;			/* Log Seq. Num.      */

	    	if (mode == DM1C_OPTIM)
		{
		    SETDBERR(dberr, 0, E_DM0057_NOROOM);
		    return(E_DB_ERROR);
		}

		for (locked = locks; locked <= locks + MAX_LOCKS; locked++)
		{
		    *locked = (DMPP_PAGE *) NULL;
		}
		sibling[0] = *current;
		*current = NULL;
		locked = locks;
		s = handle_overflow(r, &locked, sibling, &numsiblings, &newsib,
				    key, splitkey, &lsn, dberr);
		if (s != E_DB_OK)
		    break;

		s = adjust_tree(r, &locked, sibling, numsiblings, newsib, key,
				splitkey, &lsn, dberr);
		{
		    DB_STATUS	local_s =
			unlock_tree(r, locks, sibling[0], dberr);
		    if (s == E_DB_OK)
		    {
		    	s = local_s;
		    }
		}
		if (s != E_DB_OK)
		    break;
		/*
		** Adjust & unlock_tree leave the target leaf page fixed and
		** locked and place the pointer to it in sibling[0].
		*/
		*current = sibling[0];
		*leafpage = *current;
	    	s = dm1mxsearch(r, *leafpage, DM1C_FIND, DM1C_EXACT, key, adf_cb,
				t->tcb_acc_klv,
				pgtype, pgsize, tid, &pos, dberr);
	    	/*
	    	** If the above search failed with an unexpected error, then
	    	** branch down to the error handling.
	    	*/
	    	if ((s != E_DB_OK) && (dberr->err_code != E_DM0056_NOPART))
		    break;
	    }
	}

	/*
	** Now in all cases the position on the leaf page
	** is known and can be used to set limits.
	*/

        bid->tid_tid.tid_page = tidp->tid_tid.tid_page = 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(pgtype, *leafpage);
	bid->tid_tid.tid_line = tidp->tid_tid.tid_line = pos;

	/*
	** Now that we have the final value for the leaf, pass it back
	** to the caller. Error handling following this point is a bit
	** complex. If we 'break', we will fall to the bottom and unfix fixed
	** pages; if we simply return (whether E_DB_OK or E_DB_ERROR),
	** we must have the correct page fixed.
	*/
	*leafPinfo = currentPinfo;

	/*
	** If looking for an exact match, then make sure leaf record
	** key matches key we asked for.
	*/

	if (mode == DM1C_FIND )
	{
	    i4	    compare;
	    char	    *entry_ptr;
	    char	    entry_buf [DB_MAXRTREE_KEY];
	    i4	    entry_len;

	    if (pos == DM1B_VPT_GET_BT_KIDS_MACRO(pgtype, *leafpage))
	    {
		SETDBERR(dberr, 0, E_DM0056_NOPART);
	        return(E_DB_ERROR);
	    }
	    entry_ptr = entry_buf;
	    entry_len = t->tcb_klen - t->tcb_hilbertsize;
	    s = dm1cxget(pgtype, pgsize, *leafpage, 
			rac, pos, 
	    		&entry_ptr, (DM_TID *)NULL, (i4*)NULL,
			&entry_len, NULL, NULL, adf_cb);
	    if (t->tcb_rel.relpgtype != TCB_PG_V1 
	    		    && s == E_DB_WARN)
	    {
	        s = E_DB_OK;
	    }

	    if (s != E_DB_OK)
	    {
		    dm1cxlog_error(E_DM93E3_BAD_INDEX_GET, r, *leafpage,
				   pgtype, pgsize, pos );
		SETDBERR(dberr, 0, E_DM9264_DM1B_SEARCH);
	        break;
	    }
	    adt_kkcmp(adf_cb, keys_given, keyatts, entry_ptr, key, &compare);
	    if (compare != DM1B_SAME)
	    {
		SETDBERR(dberr, 0, E_DM0056_NOPART);
	        return(E_DB_ERROR);
	    }
	}
	else
	{
	    if (mode == DM1C_TID)
		if ((dberr->err_code = state) != E_DB_OK)
		    return (E_DB_ERROR);
	}
	return(E_DB_OK);
    } /* for (;;) */

    /*	Handle error reporting and recovery. */

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9264_DM1B_SEARCH);
    }

    /*
    ** If current is still fixed, unfix it before returning. If
    ** it is an index page, we can release the lock.
    */

    if (*current)
    {
	s = dm0p_unfix_page(r,
	    ((DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, *current) & DMPP_LEAF) || 
	     (mode == DM1C_SPLIT)) ? DM0P_UNFIX : DM0P_RELEASE,
	    &currentPinfo, &local_dberr);
	if (s)
	{
	    uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
		    &local_err_code, 0);
	}
    }
    return (E_DB_ERROR);

}

/*{
** Name: find_first - Positions to first qualifying entry in Rtree.
**
** Description:
**      This routines searches the Rtree index pages starting from
**      the page and record indicated in the rcb ancestor stack.
**
**      As you progress through the tree, old index and leaf pages are
**      unfixed in memory as you acquire new ones.  Leaf pages are
**      left locked.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**	current				Address of pointer to locked page.
**	leaf_bid			Index TID of current position.
**
** Outputs:
**	current				Address of (updated) ptr to locked page.
**	leaf_bid			Index TID of updated position.
**	leaf_tid			Data TID at updated position.
**      err_code                        Pointer to an area to return
**                                      the following errors when
**                                      E_DB_ERROR is returned:
**					    E_DM0056_NOPART
**
** Returns:
**
**	E_DB_OK
**      E_DB_ERROR
**
** Exceptions:
**	none
**
** Side Effects:
**	Leaves last leaf page accessed fixed and locked and
**      updates the scan information in rcb.
**
** History:
** 	30-may-1996 (wonst02)
**	    New.
**	12-dec-1996 (wonst02)
**	    New parameters added to find_next_leaf().
*/
static	DB_STATUS
find_first(
    DMP_RCB     *rcb,
    DMPP_PAGE	**current,
    DM_TID	*leaf_bid,
    DM_TID	*leaf_tid,
    DB_ERROR    *dberr)
{
    DMP_RCB	*r = rcb;
    DMP_TCB	*t = r->rcb_tcb_ptr;	/* Table control block. */
    DB_STATUS   s;			/* Routine return status. */
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if ((DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, *current)
	  & DMPP_LEAF)
    ||  (leaf_bid->tid_tid.tid_page !=
	 DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *current)))
    {
	SETDBERR(dberr, 0, E_DM002A_BAD_PARAMETER);
	return E_DB_ERROR;
    }

    /* Get first qualifying record, or return NOPART. */

    s = find_next_leaf(r, current, leaf_bid, leaf_tid, dberr);

    /*	Handle error reporting and recovery. */

    if (s != E_DB_OK)
    {
    	if (dberr->err_code == E_DM0055_NONEXT)
    	{
	    SETDBERR(dberr, 0, E_DM0056_NOPART);
	}
        else if (dberr->err_code > E_DM_INTERNAL)
    	{
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM9264_DM1B_SEARCH);
    	}
    } /* if (s != E_DB_OK) */

    return s;
} /* find_first( */


/*{
** Name: next - Gets next qualifying entry in Rtree.
**
** Description:
**      This routines searches the Rtree index pages starting from
**      the page and record indicated from the rcb->rcb_lowtid(bid).
**      The leaf page associated with this bid is assumed to be fixed on
**      entry and exit in rcb->rcb_other.page. The direction to search
**      assumed to be next.  The limit of the search is based on high key
**      value stored in rcb->rcb_hl_ptr. If an entry is found within
**      the limits of search then the bid is updated to point to the
**      new record.  When the next record's key does not qualify, regress
** 	one level back up the tree and find the next qualifying record.
** 	If we reach the top of the tree, then the end of scan is reached.
**
**      As you progress through the tree, old leaf pages are
**      unfixed in memory as you acquire new ones.  They are
**      left locked.
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**
** Outputs:
**      err_code                        Pointer to an area to return
**                                      the following errors when
**                                      E_DB_ERROR is returned.
**                                      E_DM0055_NONEXT
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
**	    Leaves last leaf page accessed fixed and locked and
**          updates the scan information in rcb.
**
** History:
** 	27-may-1996 (wonst02)
**	    New Page Format Support:
**		Change page header references to use macros.
**	12-dec-1996 (wonst02)
**	    New parameters added to find_next_leaf().
*/
static	DB_STATUS
next(
    DMP_RCB            *rcb,
    DB_ERROR           *dberr)
{
    DMP_RCB	*r = rcb;
    DMP_TCB	*t = r->rcb_tcb_ptr;	/* Table control block. */
    DB_STATUS    s;			/* Routine return status. */
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (!(DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, r->rcb_other.page)
	  & DMPP_LEAF)
    || (r->rcb_lowtid.tid_tid.tid_page !=
	DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, r->rcb_other.page)))
    {
	SETDBERR(dberr, 0, E_DM002A_BAD_PARAMETER);
	return (E_DB_ERROR);
    }

    /* Get next qualifying record, or return end of scan. */

    s = find_next_leaf(r, &r->rcb_other.page, &r->rcb_lowtid, 
    		       &r->rcb_currenttid, dberr);

    /*	Handle error reporting and recovery. */

    if (s != E_DB_OK &&
        dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9261_DM1B_GET);
    	return E_DB_ERROR;
    }

    return s;
} /* next */


/*{
** Name: find_next_leaf - Finds next qualifying leaf page
**
** Description:
**      This routines finds the next leaf page that meets the search
** 	qualification and positions to the first (leftmost) record that
** 	qualifies.
**
** 	This routine assumes that rcb_ancestors[ ] array contains a stack
** 	of the ancestors (index pages) of the current leaf page, with
** 	rcb_ancestor_level containing the current stack pointer.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains: rcb_ancestors,
** 					rcb_ancestor_level, etc.
**	leaf				Address of ptr to locked page.
**	leaf_bid			Index TID of current position.
**
** Outputs:
**	leaf				Address of (updated) ptr to locked page.
**	leaf_bid			Index TID of updated position.
**	leaf_tid			Data TID at updated position.
**      err_code                        Pointer to an area to return
**                                      error code.
**
** Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**          E_DB_FATAL
**
** Exceptions:
**	    none
**
** Side Effects:
**          Unfixes and fixes index pages as it goes. Leaves next
** 	    qualifying leaf page fixed. Modifies fields in the RCB.
**
** History:
** 	27-may-1996 (wonst02)
**	    New Page Format Support:
**		Change page header references to use macros.
**	12-dec-1996 (wonst02)
**	    New parameters added to find_next_leaf().
**      12-jun-97 (stial01)
**          find_next_leaf() Pass tlv to dm1cxget instead of tcb.
** 	06-apr-1999 (wonst02)
** 	    Use dm1cx_isnew() to prevent ambiguous updates.
*/
static DB_STATUS
find_next_leaf(
    DMP_RCB	    *rcb,
    DMPP_PAGE	    **page_ptr,
    DM_TID	    *leaf_bid,
    DM_TID	    *leaf_tid,
    DB_ERROR        *dberr)
{
    DMP_RCB	    	*r = rcb;
    DMP_TCB	    	*t = r->rcb_tcb_ptr;
    DMP_TABLE_IO	*tbio = &t->tcb_table_io;
    ADF_CB	 	*adf_cb = r->rcb_adf_cb;
    DM_TID       	*bid;	/* just a shortcut to rcb_ancestors stack */
    DMPP_PAGE		*page;
    DM_PAGENO    	savepageno;
    char		rtree_entry[DB_MAXRTREE_KEY];
    char         	*key;
    i4		keylen;
    i4		searchlen;
    DMP_ROWACCESS	*rac;
    DB_STATUS	    	s;
    DB_STATUS		get_status;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** Set up rowaccess pointer, depending on whether
    ** this is a secondary index or not
    */
    if (t->tcb_rel.relstat & TCB_INDEX)
    {
	rac = &t->tcb_data_rac;
    }
    else
    {
	rac = &t->tcb_index_rac;
    }

    page = *page_ptr;
    bid = &r->rcb_ancestors[r->rcb_ancestor_level];
    bid->tid_i4 = leaf_bid->tid_i4;
    searchlen = rac->att_ptrs[0]->length; /* NBR length */

    for (;;)
    {
    	bid->tid_tid.tid_line++;
	if (DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, page) & DMPP_LEAF)
	    leaf_bid->tid_i4 = bid->tid_i4;

	/*
	** See if there are more entries on this page
	*/
	if (bid->tid_tid.tid_line <
		DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, page))
	{
	    /* Check entry to see if it is valid record. */

	    key = r->rcb_s_key_ptr;
	    keylen = searchlen;

	    s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, page, 
			    rac, bid->tid_tid.tid_line,
			    &key, leaf_tid, (i4*)NULL,
			    &keylen,
			    NULL, NULL, adf_cb);

	    get_status = s;
	    if (t->tcb_rel.relpgtype != TCB_PG_V1 && s == E_DB_WARN)
	    {
		continue;
	    }

	    if (s != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93E3_BAD_INDEX_GET, r, page,
		       t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
		       bid->tid_tid.tid_line );
		SETDBERR(dberr, 0, E_DM9261_DM1B_GET);
		return (s);
	    }
	    if (key != r->rcb_s_key_ptr)
		MEcopy(key, keylen, r->rcb_s_key_ptr );

	    /*
	    ** Compare this record's key (key) with high key (rcb->rcb_hl_ptr).
	    ** If the record's key does not qualify, continue looking.
	    ** If the high key was not given (rcb->rcb_hl_given = 0) then
            ** this is a scan all, don't compare.
	    */

	    if (r->rcb_hl_given > 0)
	    {
		bool		compare;

		s = dm1mxcompare(adf_cb, t->tcb_acc_klv,
				 r->rcb_hl_op_type,
		    		 DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype,
						 	  page) & DMPP_LEAF,
				 r->rcb_hl_ptr, keylen, key, &compare);
		if (s != E_DB_OK)
		{
	    	    dm1cxlog_error(E_DM93E3_BAD_INDEX_GET, r, page,
				   t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
				   bid->tid_tid.tid_line);
		    SETDBERR(dberr, 0, E_DM9261_DM1B_GET);
	    	    return s;
		}
		if (!compare)
		{
		    continue;	/* continue looking for qualifying key */
		}
	    } /* if (r->rcb_hl_given > 0) */

	    if (DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, page)
		& DMPP_LEAF)
	    {
	    	if ((r->rcb_update_mode == RCB_U_DEFERRED)       &&
		    dm1cx_isnew(r, page, (i4)bid->tid_tid.tid_line))
	    	{
		    continue;       /* continue looking for qualifying key */
	    	}
		else
	    	/*
	    	** else found a leaf that qualifies:
	    	** 	return OK.
	    	*/
	    	{
			return(get_status);
	    	}
	    } /* if (DM1B_GET_PAGE_STAT_MACRO(... */

	    /*
	    ** Found an index node that qualifies - now get a leaf
	    */
	    s = pushstack(r, leaf_tid->tid_tid.tid_page, page_ptr, dberr);
	    if (s != E_DB_OK)
	    	return s;

	    bid = &r->rcb_ancestors[r->rcb_ancestor_level];
	    page = *page_ptr;
	    continue;       /* continue looking for qualifying key */
	} /* if (bid->tid_tid.tid_line <  */

	/*
	** If at end of all pages--unfix page and exit
	*/
	if (DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype, page) == 0)
	{
    	    s = dm1m_unfix_page(r,
			(DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, page)
			 & DMPP_LEAF) ? DM0P_UNFIX : DM0P_RELEASE,
                        page_ptr, dberr);
    	    if (s != E_DB_OK)
       		return s;

	    SETDBERR(dberr, 0, E_DM0055_NONEXT);
	    return E_DB_ERROR;
	}

	/*
	** Past end of entries--pop up to parent index level.
	*/
	savepageno = DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, page);

	s = popstack(r, page_ptr, dberr);
	if (s != E_DB_OK)
	{
	    return s;
	}

	bid = &r->rcb_ancestors[r->rcb_ancestor_level];
	page = *page_ptr;

	/*
	** See if entry on parent node still points to previous position.
	** If it does, simply increment to the next kid slot; if not,
	** an insert occurred in the parent node, so search for the
	** entry last used, then increment just beyond it to the next.
	*/
	key = rtree_entry;
	keylen = searchlen;

	s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, page, 
		    rac, bid->tid_tid.tid_line,
		    &key, leaf_tid, (i4*)NULL,
		    &keylen,
		    NULL, NULL, adf_cb);
	if (s != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E3_BAD_INDEX_GET, r, page,
			   t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
			   bid->tid_tid.tid_line );
	    SETDBERR(dberr, 0, E_DM9261_DM1B_GET);
	    return s;
	}

	while (leaf_tid->tid_tid.tid_page != savepageno)
	{
	    if (++bid->tid_tid.tid_line <
	    	DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, page))
	    {
	    	key = rtree_entry;
		keylen = searchlen;

	    	s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, page, 
			    rac, bid->tid_tid.tid_line,
			    &key, leaf_tid, (i4*)NULL,
			    &keylen, NULL, NULL, adf_cb);
	    	if (s != E_DB_OK)
	    	{
		    dm1cxlog_error(E_DM93E3_BAD_INDEX_GET, r, page,
		    		  t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
				  bid->tid_tid.tid_line );
		    SETDBERR(dberr, 0, E_DM9261_DM1B_GET);
		    return s;
	    	}
	    }
	    else
	    {
	    	/*
	    	**  Past last valid entry on the page--an index split
	    	**  must have occurred. Look on next index page.
	    	*/
	    	s = next_index_page(r, page_ptr,
			DM0P_RPHYS | DM0P_NOREADAHEAD | DM0P_LOCKREAD,
			dberr);
	    	if (s != E_DB_OK)
		{
	    	    return s;
		}

	    	page = *page_ptr;
	    	bid->tid_tid.tid_page =
		    DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, page);
	    	bid->tid_tid.tid_line = (DM_TIDEOF);
	    }
	} /* while */

	continue;	/* continue searching next entry */

    } /* for (;;) */

} /* find_next_leaf */


/*{
** Name: pushstack - push an entry onto the rcb->rcb_ancestors[ ] stack
**
** Description:
**      This routine unlocks the current page and pushes the child
** 	page onto the rcb_ancestors stack, then it locks the child page.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
** 	pageno				The page number of the child to
** 					be pushed onto the stack.
** Outputs:
**      err_code                        Pointer to an area to return
**                                      error code.
**
** Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**          E_DB_FATAL
**
** Exceptions:
**	    none
**
** Side Effects:
**          Unfixes and fixes index pages as it goes. Leaves an
** 	    index or leaf page fixed.
**
** History:
*/
static DB_STATUS
pushstack(
    DMP_RCB	    *r,
    DM_PAGENO       pageno,
    DMPP_PAGE	    **page_ptr,
    DB_ERROR        *dberr)
{
    DMP_TCB	    	*t = r->rcb_tcb_ptr;
    DM_TID       	*bid;
    i4		fix_type;
    DMPP_PAGE		*page;
    DB_STATUS	    	s;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    page = *page_ptr;
    if (DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, page) & DMPP_SPRIG)
    {
        fix_type = (r->rcb_access_mode == RCB_A_READ) ? DM0P_READ :
    						    	DM0P_WRITE;
        if (! (r->rcb_state & RCB_READAHEAD))
    	    fix_type |= DM0P_NOREADAHEAD;
    }
    else
        fix_type = DM0P_RPHYS | DM0P_NOREADAHEAD | DM0P_LOCKREAD;

    s = dm1m_unfix_page(r, DM0P_RELEASE, page_ptr, dberr);
    if (s != E_DB_OK)
       	return s;

    if (r->rcb_ancestor_level < RCB_MAX_RTREE_LEVEL)
    {
	bid = &r->rcb_ancestors[++r->rcb_ancestor_level];
	bid->tid_tid.tid_page = pageno;
	bid->tid_tid.tid_line = (DM_TIDEOF);

	s = dm1m_fix_page(r, pageno, fix_type, page_ptr, dberr);
    }
    else
    {
        s = E_DB_FATAL;
	SETDBERR(dberr, 0, E_DM9261_DM1B_GET);
    }

    return s;
} /* pushstack */


/*{
** Name: popstack - pop an entry off the rcb->rcb_ancestors[ ] stack
**
** Description:
**      This routine unlocks the current leaf and pops the
** 	rcb_ancestor stack, then it locks the index page at
** 	the stack's new position.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
** Outputs:
**      err_code                        Pointer to an area to return
**                                      error code.
**
** Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
** Exceptions:
**	    none
**
** Side Effects:
**          Unfixes and fixes index pages as it goes. Leaves the next
** 	    page fixed.
**
** History:
*/
static DB_STATUS
popstack(
    DMP_RCB	*r,
    DMPP_PAGE   **page_ptr,
    DB_ERROR    *dberr)
{
    DMP_TCB	    	*t = r->rcb_tcb_ptr;
    DM_TID       	*bid;
    DMPP_PAGE	    	*page;
    DB_STATUS	    	s;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    page = *page_ptr;
    s = dm1m_unfix_page(r,
			(DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, page)
			& DMPP_LEAF) ? DM0P_UNFIX : DM0P_RELEASE,
                        page_ptr, dberr);
    if (s != E_DB_OK)
       	return s;

    if (r->rcb_ancestor_level > 0)
    {
	bid = &r->rcb_ancestors[--r->rcb_ancestor_level];

	s = dm1m_fix_page(r, bid->tid_tid.tid_page,
			  DM0P_RPHYS | DM0P_NOREADAHEAD | DM0P_LOCKREAD,
	    		  page_ptr, dberr);
    }
    else
    {
        s = E_DB_ERROR;
	SETDBERR(dberr, 0, E_DM0055_NONEXT);
    }

    return s;
} /* popstack( */


/*{
** Name: next_index_page - unfix current page and fix next in chain
**
** Description:
**      This routine unlocks the current index page and
** 	locks the next one (its sibling).
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
** Outputs:
**      err_code                        Pointer to an area to return
**                                      error code.
**
** Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
** Exceptions:
**	    none
**
** Side Effects:
**      Unfixes and fixes index pages as it goes. Leaves an index page fixed.
**
** History:
*/
static DB_STATUS
next_index_page(
    DMP_RCB	    *r,
    DMPP_PAGE       **page_ptr,
    i4	    fix_type,
    DB_ERROR        *dberr)
{
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMPP_PAGE	    *page;
    DM_PAGENO       savepageno;
    DB_STATUS	    s;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    page = *page_ptr;
    savepageno = DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype, page);

    s = dm1m_unfix_page(r,
			(fix_type & DM0P_RPHYS) ? DM0P_RELEASE : DM0P_UNFIX,
                        page_ptr, dberr);
    if (s != E_DB_OK)
       	return s;

    if (savepageno)
    {
	s = dm1m_fix_page(r, savepageno, fix_type, page_ptr, dberr);
    }
    else
    {
        s = E_DB_ERROR;
	SETDBERR(dberr, 0, E_DM0055_NONEXT);
    }

    return s;
} /* next_index_page( */


/*{
** Name: handle_overflow - Move some entries to a sibling or split the node
**
** Description:
** 	The overflow handling algorithm treats the overflowing nodes either
** 	by moving some of the entries to one of the s cooperating
** 	siblings or by splitting s nodes to s + 1 nodes. The index or leaf
** 	fill factor (which determines the split policy), the value of s, and
** 	its effect on avg. disk accesses is shown by the following table:
**
** 	| Fill factor | S | Split policy | Avg. space  | Disk accesses |
** 	|             |   |              | utilization | per insertion |
** 	|-------------+---+--------------+-------------|---------------|
** 	| < 75%       | 1 |    1-to-2    |     66%     |     3.23      |
** 	|   75 - 85%  | 2 |    2-to-3    |     82%     |     3.55      |
** 	|   86 - 90%  | 3 |    3-to-4    |     89%     |     4.09      |
** 	| > 90%       | 4 |    4-to-5    |     92%     |     4.72      |
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
** 	locked				Set of pages that are locked.
** 	sibling				Array of pointers to pages. The first
** 					points to currently locked leaf.
** 	bid				Pointer to the bid indicating the
** 					position of the key being inserted.
**      key                             Pointer to the key value.
**
** Outputs:
** 	numsiblings			Number of siblings affected.
** 	newsib				Pointer to newly added page, if split.
** 	splitkey			Key at split position; this will
** 					be used to update the parent.
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
** 	Locks sibling pages. May cause a page split, which adds a new page.
**
** History:
** 	10-jun-1996 (wonst02)
**	    New.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**	14-nov-1997 (nanpr01)
**	    Tuple header was getting added twice for larger pages.
*/
static	DB_STATUS
handle_overflow(
    	DMP_RCB		*rcb,
	DMPP_PAGE	***locked,
    	DMPP_PAGE	**sibling,
	i4		*numsiblings,
	DMPP_PAGE	**newsib,
    	char		*key,
	char		*splitkey,
	LG_LSN	    	*lsn,
    	DB_ERROR	*dberr)
{
    	DMP_RCB		*r = rcb;		/* Record context block. */
    	DMP_TCB		*t = r->rcb_tcb_ptr;	/* Table control block. */
	DMP_TABLE_IO	*tbio = &t->tcb_table_io;
    	ADF_CB	 	*adf_cb = r->rcb_adf_cb;
    	DMPP_PAGE	*curpage;
    	bool		changes = FALSE;	/* Any changes made */
    	bool		hasroom = FALSE;	/* Page has room for entry */
	bool		pages_mutexed = FALSE;	/* Pages are mutexed */
    	i2		fill;			/* Fill factor (0-100%) */
    	i4		kids;			/* Number of key-ID's on page */
    	i4		S;			/* Split policy number */
	i4		newpages = 0;		/* Num of new pages: 0 or 1 */
	DMPP_PAGE	**sib;			/* Ptr to array of siblings */
    	DMPP_PAGE	**cursib;		/* Page to be split */
    	i4		avgkids;		/* Average kids per sibling */
    	i4	    	kperpage;
	i4		unfix_type;
	DM_PAGENO	nextpage;
    	DB_STATUS	status = E_DB_OK;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    *numsiblings = 1;
    curpage = sibling[0];
    kids = DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, curpage);

    /*
    ** Use the leaf/index fill factor to determine the splitting policy
    ** (see the table in the description, above).
    */
    fill = (DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, curpage)
	    & DMPP_LEAF) ?
		  t->tcb_rel.rellfill :
		  t->tcb_rel.relifill ;
    S = (fill ==  0) ? 2 :	/* 2-to-3 */
    	(fill <  75) ? 1 :	/* 1-to-2 */
    	(fill <= 85) ? 2 :	/* 2-to-3 */
    	(fill <= 90) ? 3 :	/* 3-to-4 */
    	               4 ;	/* 4-to-5 */

    if (DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, curpage)
	& DMPP_LEAF)
	kperpage = t->tcb_kperleaf;
    else
	kperpage = t->tcb_kperpage;

    /*************************************************************
    **           Start Overflow Transaction
    **************************************************************
    **
    ** Overflows are performed within a Mini-Transaction.
    ** Should the transaction which is updating this table abort after
    ** we have completed the overflow, then the overflow will NOT be backed
    ** out, even though the insert which caused the overflow will be.
    **
    ** We commit overflows so that we can release the index page locks
    ** acquired during the overflow and gain better concurrency.
    */
    if (r->rcb_logging & RCB_LOGGING)
    {   /* Begin mini-transaction */
        status = dm0l_bm(r, lsn, dberr);
        if (status != E_DB_OK)
	    goto ErrorExit;
    }
    *(*locked)++ = *sibling;
    /*
    ** See if there's room on any of the S - 1 siblings.
    */
    for (sib = sibling + 1; sib < sibling + S; sib++)
    {
	nextpage = DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype, curpage);
	if (nextpage == 0)
	    break;

	status =  dm1m_fix_page(r, nextpage,
			(DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, curpage)
		 	 & DMPP_LEAF) ? DM0P_WRITE
		  	      	      : DM0P_WPHYS,
			sib, dberr);
	/*
	** If error, unlock pages (since no change was made yet) and return.
	*/
	if (status != E_DB_OK)
	    goto ErrorExit;

	++*numsiblings;
	*(*locked)++ = *sib;
	curpage = *sib;
	kids += DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, curpage);

	if (dm1cxhas_room(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, curpage, 
			DM1B_INDEX_COMPRESSION(r),
			(i4)100 /* fill factor */,
			kperpage, dm1cxmax_length( r, curpage ) - 
			t->tcb_hilbertsize - DM1B_TID_S))
	{
	    hasroom = TRUE;
	}
    } /* for (sib...) */

    /*
    ** If the average number of entries (rounded up) does not leave at least
    ** one slot per sibling, then do a split instead of re-distributing.
    */
    avgkids = (kids + *numsiblings - 1) / *numsiblings;
    if (avgkids + 1 >
    	 DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, *sibling))
	hasroom = FALSE;

    if (!hasroom)
    {
    	/*
	** Split the last sibling into two pages and distribute the entries
	** between the two pages.
	*/
    	cursib = sibling + *numsiblings - 1;
	changes = TRUE;
	status = split_page(r, cursib, newsib, *numsiblings, &kids, key,
			    splitkey, dberr);
	if (status != E_DB_OK)
	    goto ErrorExit;
	*(cursib + 1) = *newsib;
	*(*locked)++ = *newsib;
	/*
	** Finished with the split. If no other siblings involved, exit now.
	*/
	newpages = 1;
    	if (*numsiblings < 2)
	    goto Exit;
    }

    changes = TRUE;
    status = log_before_images(r, sibling, *numsiblings, &pages_mutexed,
    			       dberr);
    if (status != E_DB_OK)
	goto ErrorExit;

    /*
    ** Distribute the entries across the siblings (not including the
    ** new page that was split--it was done during split processing).
    */
    status = dm1mxdistribute(r, sibling, *numsiblings, kids, lsn, dberr);
    if (status != E_DB_OK)
        goto ErrorExit;
    /*
    ** Complete the Logging for the operation by logging the After
    ** Images of the updated pages.  These will be used during forward
    ** (redo,rollforward) recovery to reconstruct the pages.
    */
    status = log_after_images(r, sibling, *numsiblings, &pages_mutexed,
    			      dberr);
    if (status != E_DB_OK)
    	goto ErrorExit;

    goto Exit;

ErrorExit:
    if (pages_mutexed)
    {
	for (sib = sibling; sib < sibling + *numsiblings; sib++)
	    dm1m_unmutex(tbio, (i4)0, r->rcb_lk_id, sib);
	pages_mutexed = FALSE;
    }
    /*
    ** Handle errors. If no changes made, unlock pages and return.
    ** Otherwise, leave the pages locked for recovery.
    */
    if (changes)
	unfix_type = DM0P_UNFIX;
    else
	unfix_type = DM0P_RELEASE;

    for (sib = sibling; sib < sibling + *numsiblings + newpages; sib++)
    {
	status = dm1m_unfix_page(r, unfix_type, sib, dberr);
	if (status != E_DB_OK)
	    break;
    }

Exit:
    *numsiblings += newpages;
    return status;

} /* handle_overflow */


/*{
** Name: split_page - split a full Rtree index or leaf page
**
** Description:
** 	The overflow handling algorithm treats the overflowing nodes either
** 	by moving some of the entries to one of the S cooperating
** 	siblings or by splitting S nodes to S + 1 nodes. This function
** 	handles the S-to-(S+1) split.
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
** 	key				The key that caused the split.
**
** Outputs:
** 	splitkey			Key at split position; this will
** 					be used to update the parent.
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
** 	13-jun-1996 (wonst02)
**	    New.
*/
static	DB_STATUS
split_page(
	DMP_RCB		*r,
	DMPP_PAGE	**current,
	DMPP_PAGE	**newone,
	i4		numsiblings,
	i4		*sibkids,
	char		*key,
	char		*splitkey,
	DB_ERROR	*dberr)
{
	DMP_TCB		*t = r->rcb_tcb_ptr;	/* Table control block. */
    	DB_ATTS		**keyatts = t->tcb_key_atts;/* Key attribute array */
    	DMPP_PAGE	*curpage;		/* Page pointer */
	i4		kids;			/* Kids on the page */
	i4		avgkids;		/* Average kids per page */
	i4		split;			/* Split point */
	DB_STATUS	s = E_DB_OK;

    CLRDBERR(dberr);

	/*
	** Compute the average kids per page with one more page. Use that
	** to determine where to split the last sibling to a new page.
	*/
    	avgkids = (*sibkids + numsiblings)/(numsiblings + 1);
    	curpage = *current;
	kids = DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, curpage);
	split = kids - avgkids;

	s = dm1mxsplit(r, current, newone, key, keyatts, &split, splitkey,
		       dberr);
	if (s)
	    return s;
	*sibkids -= avgkids;
	return s;
} /* split_page( */


/*{
** Name: adjust_tree - Adjust MBR and LHV of entries in parent pages
**
** Description:
** 	The routine ascends from leaf level towards the root, adjusting
** 	the MBR and LHV of pages that cover the pages in the set of
** 	siblings. It ends the current mini-transaction, unlocks pages,
** 	and propagates any splits.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
** 	locked				Set of pages that are locked.
** 	sibling				Set of pages that contains the page
** 					being updated, sibling[0], its co-
** 					operating siblings (if overflowed),
** 					and a newly created page (if split).
** 	numsiblings			Number of siblings.
** 	newsibling			Ptr to newly created page (if split).
**      key                             Pointer to the key being inserted.
** 	splitkey			Key at split position to update parent.
** 	prevlsn				Ptr to Log Sequence Number of the
** 					handle_overflow that began the split.
**
** Outputs:
** 	sibling[0]                      Sibling[0] contains the target leaf page
** 					for the key to be inserted.
**      err_code                        Pointer to an area to return errors.
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
** 	11-jul-1996 (wonst02)
**	    New.
**	06-Nov-1996 (wonst02)
**	    Fix problem finding and locking multiple parent pages.
**      12-jun-97 (stial01)
**          adjust_tree() Pass tlv to dm1cxget instead of tcb.
*/
static	DB_STATUS
adjust_tree(
    	DMP_RCB		*rcb,
	DMPP_PAGE	***locked,
    	DMPP_PAGE	**sibling,
	i4		numsibs,
	DMPP_PAGE	*newsib,
    	char		*key,
	char		*splitkey,
	LG_LSN	    	*prev_lsn,
    	DB_ERROR	*dberr)
{
    	DMP_RCB		*r = rcb;		/* Record context block */
    	DMP_TCB		*t = r->rcb_tcb_ptr;	/* Table control block */
	DMP_TABLE_IO	*tbio = &t->tcb_table_io;
    	ADF_CB	 	*adf_cb = r->rcb_adf_cb;
    	i4	    	parkeylen = t->tcb_klen - DM1B_TID_S;
	i4		nbrsize = 2 * t->tcb_hilbertsize;
	DM_TID		tid;			/* A Tuple-ID */
    	DM_TID      	*tidp;                 	/* TID pointer */
    	i4		pos;			/* TID line position. */
	DMPP_PAGE	**sibptr;		/* Ptr to a sibling page */
	DMPP_PAGE	*sibpage;		/* Sibling page */
	DMPP_PAGE	**target = sibling;	/* Ptr to target page of key */
	DMPP_PAGE	*targpage;		/* Target (leaf) page of key */
    	DMPP_PAGE	*parent[MAX_SPLIT_POLICY + 1];
	    					/* Set of parent pages */
	DMPP_PAGE	*parpage;		/* Parent page */
	i4		numsiblings = numsibs;	/* Number of siblings */
	i4		numparents;	        /* Number of parents */
	DMPP_PAGE	*newparent;		/* Newly split parent */
	DMPP_PAGE	**parptr;		/* Ptr to sibling's parent */
	LG_LSN	    	lsn;			/* Log Sequence Number */
	LG_LSN	    	parlsn;			/* Parent Log Sequence Number */
	i4		loc_ancestor_level;	/* Local ancestor stack level */
	char 		sibkey[DB_MAXRTREE_KEY];/* Range Key of sibling page */
	char		rangekey[DB_MAXRTREE_KEY];/* Range key of parent */
	char		*sibsplitkey = splitkey;     /* Split key of sibling */
	char 		parkey[DB_MAXRTREE_KEY];/* Range Key of parent page */
	char		parsplitkey[DB_MAXRTREE_KEY];/* Split key of parent */
	bool		in_range;
	bool		changed;
	bool		parchanged;
    	i4	    	klen;
	char		*key_ptr;
	DMP_ROWACCESS	*rac;
	DM_PAGENO	next;
	DB_STATUS	s = E_DB_OK;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** Set up rowaccess structure pointer, depending on whether
    ** this is a secondary index or not
    */
    if (t->tcb_rel.relstat & TCB_INDEX)
    {
	rac = &t->tcb_data_rac;
    }
    else
    {
	rac = &t->tcb_index_rac;
    }
    /*
    ** Find the page where "key" should be inserted, by Hilbert value.
    ** Note that all the sibling pages are still locked and fixed.
    */
    for (target = sibling; target < sibling + numsiblings; target++)
    {
        targpage = *target;
        s = dm1m_check_range(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		targpage, adf_cb, t->tcb_acc_klv,
		key, &in_range, dberr);
        if (s || in_range)
            break;
    }
    if (s)
        return s;

    /*
    ** Ascend the tree toward the root, adjusting the MBRs and LHVs of the
    ** entries in the parents of the siblings involved.
    */
    for (loc_ancestor_level = r->rcb_ancestor_level - 1;
    	 ;
    	 loc_ancestor_level--)
    {
    	sibpage = *sibling;
	/*
	** Lock the parent page of the first sibling.
	*/
	tidp = &r->rcb_ancestors[loc_ancestor_level];
	s = dm1m_fix_page(r, tidp->tid_tid.tid_page, DM0P_WPHYS, parent,
			  dberr);
	if (s)
	    break;
	s = requalify_parent(r, parent, sibpage, &loc_ancestor_level, dberr);
	if (s)
	    break;
	tidp = &r->rcb_ancestors[loc_ancestor_level];
	*(*locked)++ = *parent;
	parptr = parent;
	parpage = *parptr;
	numparents = 1;
	/*
	** Lock the parent pages of all cooperating siblings.
	*/
	for (sibptr = sibling + 1, pos = tidp->tid_tid.tid_line + 1; 
	     sibptr < sibling + numsiblings; ++sibptr)
	{
	    if (*sibptr == newsib)
		break;
	    sibpage = *sibptr;
	    for (;;)
	    {
		if (pos >= 
			DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, parpage))
		{
	    	    next = DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype,
		    				      parpage);
	    	    if (next == 0)		/* no more index pages */
	    	    {
			SETDBERR(dberr, 0, E_DM0055_NONEXT);
		    	s = E_DB_ERROR;
		    	break;
	    	    }
	    	    s = dm1m_fix_page(r, next, DM0P_WPHYS, ++parptr, dberr);
	    	    if (s)
			break;
	    	    ++numparents;
	    	    *(*locked)++ = *parptr;
	    	    parpage = *parptr;
		    pos = 0;
		}
	    	dm1cxtget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, parpage, pos++, 
				&tid, (i4*)NULL);
	    	if (tid.tid_tid.tid_page ==
	            DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, sibpage))
		    break;
	    }
	    if (s)
	    	break;
	    if (tid.tid_tid.tid_page ==
	        DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, sibpage))
	        continue;
	}
	if (s)
	    break;
	newparent = NULL;
	if (newsib)
	{
	    /*
	    ** See if there is room for the new key on the parent page.
	    ** If no room...spill onto other pages.
	    */
	    if (! dm1cxhas_room(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
			parpage, DM1B_INDEX_COMPRESSION(r),
			(i4)100 /* fill factor */,
			t->tcb_kperpage, dm1cxmax_length( r, parpage ) - 
		    	t->tcb_hilbertsize - DM1B_TID_S))
	    {
		/* Only the last parent participates in the overflow. */
		numparents -= (parptr - parent);
		/*
		** The page has already been put in the "locked" list,
		** but handle_overflow will do it again, so back it off.
		*/
		--*locked;
		s = handle_overflow(r, locked, parptr, &numparents, &newparent,
				    NULL, parsplitkey, &parlsn, dberr);
		numparents += (parptr - parent);
		if (s)
		    break;
	    } /* if (! dm1cxhas_room...) */
	} /* if (newsib) */

	/*
	** Adjust the MBRs and LHVs of entries in the parent level.
	*/
	parptr = parent;
	parpage = *parptr;
	pos = tidp->tid_tid.tid_line;
	for (sibptr = sibling; sibptr < sibling + numsiblings; ++sibptr)
	{
	    sibpage = *sibptr;
	    if (*sibptr != newsib)
	      /*
	      ** Search for the parent where the sibling belongs.
	      ** Do this by matching the TID of an entry with the sibling page.
	      ** Resume searching from the position after the prior entry.
	      */
	      for ( ; ; )
	      {
	    	if (pos < 
			DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, parpage))
		{
		    dm1cxtget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, parpage, pos,
				&tid, (i4*)NULL);
		    if (tid.tid_tid.tid_page ==
		        DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, sibpage))
		    	break;
		    ++pos;
		    continue;
		}
		if (++parptr >= parent + numparents) /* no more index pages */
	    	{
		    SETDBERR(dberr, 0, E_DM0055_NONEXT);
		    s = E_DB_ERROR;
		    break;
	    	}
		/*
		** Before going to the next parent page, recompute this parent's
		** MBR and get the LHV to store in its high range key.
		*/
		dm1m_get_range(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
			parpage, t->tcb_acc_klv, NULL, parkey);
		s = dm1mxpagembr(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
				parpage, rangekey, adf_cb, t->tcb_acc_klv,
				t->tcb_hilbertsize, &parchanged);
		if (s)
		    break;
	    	key_ptr = rangekey;
		klen = parkeylen;
	    	s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
			    parpage, rac,
		    	    DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, 
			     			    parpage) - 1,
			    &key_ptr, (DM_TID *)NULL, (i4*)NULL,
			    &klen,
			    NULL, NULL, adf_cb);
	    	if (s != E_DB_OK)
	    	{
		    dm1cxlog_error(E_DM93E3_BAD_INDEX_GET, r, parpage,
		    		   t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
				   DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, 
			     			    	  parpage) - 1 );
		    SETDBERR(dberr, 0, E_DM9261_DM1B_GET);
		    break;
	    	}
		if (parchanged || MEcmp((char *)key_ptr + nbrsize,
					(char *)parkey + nbrsize,
					t->tcb_hilbertsize))
		{
		    MEcopy((char *)key_ptr + nbrsize,
		    	   t->tcb_hilbertsize,
			   (char *)parkey + nbrsize);
		    s = dm1mxadjust(r, t, parpage, t->tcb_rel.relpgsize,
				    parkeylen, parkey, DM1B_RRANGE,
				    NO_LOG_REQUIRED, MUTEX_REQUIRED, dberr);
		    if (s)
		    	break;
		}
	    	parpage = *parptr;
		pos = 0;
	      }
	    else
	    {
	    	/*
	    	** Insert the (key,tid) of the newly split page into its parent.
	    	*/
	    	tid.tid_tid.tid_line = 0;
	    	tid.tid_tid.tid_page =
	    		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
						 newsib);
	    	s = dm1mxinsert(r, t, t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
				rac,
		    	    	parkeylen, parpage, sibkey, &tid, pos - 1,
				LOG_REQUIRED,MUTEX_REQUIRED, dberr);
	    	if (s)
		    break;
	    }
	    /*
	    ** Found the entry on the parent. Now adjust MBR and LHV of entry.
	    */
	    dm1m_get_range(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
			sibpage, t->tcb_acc_klv, NULL, sibkey);
	    s = dm1mxadjust(r, t, parpage, t->tcb_rel.relpgsize,
	    		    parkeylen, sibkey, pos++,
	    		    LOG_REQUIRED, MUTEX_REQUIRED, dberr);
	    if (s)
		break;
	} /* for (sibptr = ...) */
	/*
	** Move up to the parent level and iterate. Stop when:
	**   a) only one node is involved (no split or redistribution took
	**      place and all siblings belonged to one parent node), or
	**   b) we reach the root.
    	*/
	for (parptr = parent, sibptr = sibling;
	     parptr < parent + numparents;
	     ++parptr, ++sibptr)
	{
	    *sibptr = *parptr;
	}
	numsiblings = numparents;
	sibsplitkey = parsplitkey;
	newsib = newparent;
	if ((numsiblings == 1) || (loc_ancestor_level == 0))
	    break;
	/*
	** If more than one parent was involved, use the prior parent's high 
	** range key as this parent's low range key and recompute this parent's
	** MBR to store in its high range key.
	*/
	if (parchanged)
	{
	    s = dm1mxadjust(r, t, parpage, t->tcb_rel.relpgsize, parkeylen,
			    parkey, DM1B_LRANGE, NO_LOG_REQUIRED, 
			    MUTEX_REQUIRED, dberr);
	    if (s)
	    	break;
	}
	dm1m_get_range(t->tcb_rel.relpgsize, t->tcb_rel.relpgtype, parpage, 
			t->tcb_acc_klv, NULL, rangekey);
	s = dm1mxpagembr(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
			parpage, rangekey, adf_cb, t->tcb_acc_klv,
			t->tcb_hilbertsize, &changed);
	if (s)
	    break;
	if (changed) 
	{
	    s = dm1mxadjust(r, t, parpage, t->tcb_rel.relpgsize, parkeylen,
			    rangekey, DM1B_RRANGE,
			    NO_LOG_REQUIRED, MUTEX_REQUIRED, dberr);
	    if (s)
	    	break;
	}
    } /* for (loc_ancestor_level ...) */
    if (s)
    {
	return s;
    }

    /*
    ** If the root split, build a new root.
    */
    if (newsib)
      for (;;){
	sibpage = newsib;
        s = dm1mxnewroot(r, sibling, dberr);
        if (s)
    	    break;
	/*
	** Insert the (key,tid) of the newly split page into the root.
	*/
	tid.tid_tid.tid_line = 0;
	tid.tid_tid.tid_page = DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
	    			 		        sibpage);
	parpage = *sibling;
	s = dm1mxinsert(r, t, t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
			rac,
	        	parkeylen, parpage, sibsplitkey,
	    	    	&tid, 0,
	    	    	LOG_REQUIRED, MUTEX_REQUIRED, dberr);
	if (s)
	    break;
	/*
	** Now adjust MBR and LHV of new entry.
	*/
	dm1m_get_range(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, sibpage, 
			t->tcb_acc_klv, NULL, sibkey);
	s = dm1mxadjust(r, t, parpage, t->tcb_rel.relpgsize,
			parkeylen, sibkey, 1,
			LOG_REQUIRED, MUTEX_REQUIRED, dberr);
	if (s)
	    break;
	dm1m_get_range(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, parpage, 
			t->tcb_acc_klv, NULL, rangekey);
	s = dm1mxpagembr(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
			parpage, rangekey, adf_cb, t->tcb_acc_klv,
			t->tcb_hilbertsize, &changed);
	if (s)
	    break;
	if (changed)
	    s = dm1mxadjust(r, t, parpage, t->tcb_rel.relpgsize, parkeylen,
			    rangekey, DM1B_RRANGE,
			    NO_LOG_REQUIRED, MUTEX_REQUIRED, dberr);
	break;
    } /* if () for() */
    if (s)
        return s;

    /*
    ** End the mini-transaction for the overflow/split operation.
    */
    if (r->rcb_logging & RCB_LOGGING)
    {
        s = dm0l_em(r, prev_lsn, &lsn, dberr);
        if (s)
	    return s;
    }
    *sibling = targpage;
    return s;
} /* adjust_tree */


/*{
** Name: unlock_tree - Unlock the pages locked during overflow and adjust.
**
** Description:
** 	Unlock the pages locked during overflow and adjust, but leave the
** 	target (leaf) page fixed and locked.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
** 	locked				Set of pages that are locked.
** 	sibling				Set of pages that contains the page
** 					being updated, sibling[0], its co-
** 					operating siblings (if overflowed),
** 					and a newly created page (if split).
**
** Outputs:
** 	sibling[0]                      Sibling[0] contains the target leaf page
** 					for the key to be inserted.
**      err_code                        Pointer to an area to return errors.
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
** 	18-sep-1996 (wonst02)
**	    New.
*/
static	DB_STATUS
unlock_tree(
    	DMP_RCB		*r,
	DMPP_PAGE	**locks,
	DMPP_PAGE	*target,
    	DB_ERROR	*dberr)
{
    	DMP_TCB		*t = r->rcb_tcb_ptr;	/* Table control block */
	DMPP_PAGE	**page;
	i4		unfix_index;
	DB_STATUS	s = E_DB_OK;
    i4		    *err_code = &dberr->err_code;

    /*
    ** If no errors occurred, release (unfix and unlock) index pages;
    ** otherwise, unfix but leave the pages locked for recovery.
    */
    if (dberr->err_code)
	unfix_index = DM0P_UNFIX;
    else
	unfix_index = DM0P_RELEASE;

    /*
    ** Unfix and unlock the pages (except the target leaf page) and return.
    */
    for (page = locks; *page; page++)
    {
	if (*page == target)
	    continue;
	s = dm1m_unfix_page(r,
		(DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, *page)
		    & DMPP_LEAF) ? DM0P_UNFIX : unfix_index,
		page, dberr);
    }
    return s;
} /* unlock_tree */


/*{
** Name: adjust_mbrs - Adjust MBRs of parent pages
**
** Description:
** 	The routine ascends from leaf level towards the root, adjusting
** 	the MBR of the entry and page that points to this page.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
** 	child				Ptr to the page being updated.
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
** 	11-jul-1996 (wonst02)
**	    New.
** 	03-apr-1999 (wonst02)
**	    Remove DMPP_SIZEOF_TUPLE_HDR_MACRO--it doesn't belong.
*/
static	DB_STATUS
adjust_mbrs(
    	DMP_RCB		*r,
	DMPP_PAGE	*childpage,
    	DB_ERROR	*dberr)
{
    	DMP_TCB		*t = r->rcb_tcb_ptr;	/* Table control block */
	DMP_TABLE_IO	*tbio = &t->tcb_table_io;
    	ADF_CB	 	*adf_cb = r->rcb_adf_cb;
    	DM_TID      	*tidp;                 	/* TID pointer */
    	i4		pos;			/* TID line position. */
	DMPP_PAGE	**parent;		/* Ptr to parent page */
	DMPP_PAGE	*parpage;		/* Parent page */
	i4		loc_ancestor_level;	/* Local ancestor stack level */
	char 		rangekey[DB_MAXRTREE_KEY];/* Range Key of child page */
	bool		changed;
    	i4	    	klen = t->tcb_klen - t->tcb_hilbertsize - DM1B_TID_S;
    	i4		local_err_code;        	/* Local error code. */
	DB_STATUS	s = E_DB_OK;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    parent = ((DMPP_PAGE **)&parpage);

    for (loc_ancestor_level = r->rcb_ancestor_level - 1;
    	 ;
	 loc_ancestor_level--)
    {
	/*
	** Get the high range key of the updated page. 
	** Recompute the page's total MBR, then adjust the page's range key.
	** If the total MBR of the page is unchanged, we're done!
	*/
	dm1m_get_range(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, childpage, 
			t->tcb_acc_klv, NULL, rangekey);
	s = dm1mxpagembr(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
			childpage, rangekey, adf_cb, t->tcb_acc_klv,
			t->tcb_hilbertsize, &changed);
	if (s || !changed)
	    break;
	s = dm1mxadjust(r, t, childpage, t->tcb_rel.relpgsize, klen, rangekey,
			DM1B_RRANGE, NO_LOG_REQUIRED, MUTEX_REQUIRED, dberr);
	if (s)
	    break;
    	/*
    	** Continue adjusting until reaching the root level.
    	*/
    	if (DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, childpage)
    		== (DM_PAGENO)DM1B_ROOT_PAGE)
	    break;
	/*
	** Lock the parent page.
	** Make sure this is still the parent of the updated page--a split
	** could have moved this page's entry onto another parent.
	** Requalify page returns with the correct parent locked and fixed
	** and with rcb_ancestors[ancestor_level] pointing to the child.
	*/
	tidp = &r->rcb_ancestors[loc_ancestor_level];
	s = dm1m_fix_page(r, tidp->tid_tid.tid_page, DM0P_WPHYS, parent,
			  dberr);
	if (s)
	    break;
	s = requalify_parent(r, parent, childpage, &loc_ancestor_level, 
			     dberr);
	if (s)
	    break;
	tidp = &r->rcb_ancestors[loc_ancestor_level];
	pos = tidp->tid_tid.tid_line;
	/*
	** Found the entry on the parent.
	** Release the index page--leave the leaf page fixed and locked.
	*/
	if (!(DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, childpage)
	      & DMPP_LEAF))
	{
	    s = dm1m_unfix_page(r, DM0P_RELEASE, ((DMPP_PAGE **)&childpage),
	    			&local_dberr);
	    if (s)
	    	uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
		    	   &local_err_code, 0);
	}
	/*
	** Now adjust MBR of the entry on the parent.
	*/
	s = dm1mxadjust(r, t, parpage, t->tcb_rel.relpgsize, klen, rangekey,
			pos, NO_LOG_REQUIRED, MUTEX_REQUIRED, dberr);
	if (s)
	    break;
	/*
	** Move up to the parent level and iterate.
	*/
	childpage = parpage;
    } /* for (loc_ancestor_level...) */
    /*
    ** Release the index page--leave the leaf page fixed and locked.
    */
    if (s == E_DB_OK)
    {
	if (!(DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, childpage)
	      & DMPP_LEAF))
	{
	    s = dm1m_unfix_page(r, DM0P_RELEASE, ((DMPP_PAGE **)&childpage),
	    			&local_dberr);
	    if (s)
	    	uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
		    	   &local_err_code, 0);
	}
    }
    return s;
} /* adjust_mbrs */


/*{
** Name: requalify_page - make sure this page contains the key (after a split)
**
** Description:
** 	Make sure this is still the page containing the key. A split
** 	could have moved the key's entry onto another page.
**
** Inputs:
**      r				Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**	page				Ptr to page pointer.
**	key				Search key.
**	ancestor_level			Pointer to ancestor level on rcb stack.
**
** Outputs:
**	page				Ptr to (possibly different) page.
**      err_code                        Pointer to an area to return errors.
**
** Returns:
**	E_DB_OK
**      E_DB_ERROR
**
** Exceptions:
**	none
**
** Side Effects:
** 	May unfix the page and return with another page fixed.
**
** History:
** 	23-Oct-1996 (wonst02)
**	    New.
*/
static	DB_STATUS
requalify_page(
    	DMP_RCB		*r,
    	DMPP_PAGE	**page,
	char		*key,
	i4         mode,
	i4         direction,
    	DB_ERROR	*dberr)
{
    	DMP_TCB		*t = r->rcb_tcb_ptr;	/* Table control block */
    	ADF_CB	 	*adf_cb = r->rcb_adf_cb;
	i4		pgsize = t->tcb_rel.relpgsize;
	i4		pgtype = t->tcb_rel.relpgtype;
	DMPP_PAGE	*curpage;
    	DM_TID      	*tidp;
	DM_PAGENO	nextpageno;
	i4		fix_type;
	i4		unfix_type;
	bool		in_range;
	DB_STATUS	s = E_DB_OK;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

	/* 
	** Must requalify the page, since it could have split
	** while you were waiting for it.  If searching for low, the
	** one you have is the lowest since always split to the right.
	*/
	if (mode == DM1C_EXTREME && direction == DM1C_PREV)
	    return s;

	tidp = &r->rcb_ancestors[r->rcb_ancestor_level];
	curpage = *page;
    	/*
    	** Index pages will be temporarily locked for reading.
    	** Index reads use NOREADAHEAD, because the index is not
    	** sequential, and LOCKREAD, to lock the index page even if
    	** read = nolock is specified for this access.
    	*/
	if (DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, curpage)
	    & DMPP_LEAF)
	{
    	    if (r->rcb_access_mode == RCB_A_WRITE)
	    	fix_type = DM0P_WRITE;
	    else
    	    	fix_type = DM0P_READ;
    	    if ((r->rcb_state & RCB_READAHEAD) == 0)
	    	fix_type |= DM0P_NOREADAHEAD;
	    unfix_type = DM0P_UNFIX;
	}
	else
	{
    	    fix_type = DM0P_RPHYS | DM0P_NOREADAHEAD | DM0P_LOCKREAD;
	    unfix_type = DM0P_RELEASE;
	}

	for (;;)
	{
	    nextpageno = 
		DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype, curpage);
	    if (nextpageno == 0)
		break;
	    if (mode != DM1C_EXTREME && mode != DM1C_LAST)
	    {
            	s = dm1m_check_range(pgtype, pgsize, curpage, adf_cb,
			t->tcb_acc_klv,  key, &in_range, dberr);
            	if (s || in_range)
            	    break;
	    }
	    s = dm1m_unfix_page(r, unfix_type, page, dberr);
	    if (s)
		break;
	    tidp->tid_tid.tid_page = nextpageno;
	    tidp->tid_tid.tid_line = (DM_TIDEOF);
	    s = dm1m_fix_page(r, tidp->tid_tid.tid_page, fix_type,
	        	      page, dberr);
	    if (s)
		break;
	} /* for (;;) */

    	return s;
} /* requalify_page */


/*{
** Name: requalify_parent - make sure this is still the parent (after a split)
**
** Description:
** 	Make sure this is still the parent of the child page. A split
** 	could have moved the child page's entry onto another parent.
**
** Inputs:
**      r				Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**	parent				Ptr to parent page pointer.
**	childpage			Ptr to child page.
**	ancestor_level			Pointer to ancestor level on rcb stack.
**
** Outputs:
**	parent				Ptr to (possibly different) parent page.
**	ancestor_level			(Possibly) adjusted ancestor level.
**      err_code                        Pointer to an area to return errors.
**
** Returns:
**	E_DB_OK
**      E_DB_ERROR
**
** Exceptions:
**	none
**
** Side Effects:
**	If a root split is detected, this routine fixes the rcb_ancestor_stack.
**
** History:
** 	11-jul-1996 (wonst02)
**	    New.
** 	01-apr-1999 (wonst02)
** 	    Need to set tid_line = 0 for the root, if a root split occurred.
*/
static	DB_STATUS
requalify_parent(
    	DMP_RCB		*r,
    	DMPP_PAGE	**parent,
	DMPP_PAGE	*childpage,
	i4		*ancestor_level,
    	DB_ERROR	*dberr)
{
    	DMP_TCB		*t = r->rcb_tcb_ptr;	/* Table control block */
	DMPP_PAGE	*parpage;
    	DM_TID      	*tidp;
    	DM_TID		childtid;
    	DM_PAGENO       nextpageno;
	DB_STATUS	s = E_DB_OK;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

	tidp = &r->rcb_ancestors[*ancestor_level];
	/*
	** See if entry on parent node still points to child page.
	** If not, search for the child entry.
	*/
	parpage = *parent;

	for (;;)
	{
	    if (tidp->tid_tid.tid_line <
		DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, parpage))
	    {
	        dm1cxtget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, parpage,
	        	  tidp->tid_tid.tid_line, &childtid, (i4*)NULL);
	        if (childtid.tid_tid.tid_page ==
	              DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, childpage))
	       	    break;
		++tidp->tid_tid.tid_line;
	        continue;
	    }
	    /*
	    **  Past last valid entry on the page--an index split
	    **  must have occurred. Look on next index page.
	    */
	    nextpageno = DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype,
	    					    parpage);
	    if (nextpageno)
	    {
		tidp->tid_tid.tid_page = nextpageno;
		tidp->tid_tid.tid_line = 0;
	    }
	    else
	    {
		if (*ancestor_level == 0)
	        {
	    	    /* 
	    	    ** If a root split happened, add the extra level in the 
	    	    **  ancestor stack and resume the search below the root.
		    ** Start with the leftmost (first) entry on the root.
		    */
		    for ((*ancestor_level) = ++(r->rcb_ancestor_level);
	        	 (*ancestor_level) > 1;
	    	     	 (*ancestor_level)--)
	    	    	r->rcb_ancestors[*ancestor_level] =
	    	    		r->rcb_ancestors[*ancestor_level - 1];
		    tidp->tid_tid.tid_line = 0;
	    	    *ancestor_level = 1;
	    	    tidp = &r->rcb_ancestors[1];
	            dm1cxtget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
			parpage, 0, tidp, (i4*)NULL);
	    	    tidp->tid_tid.tid_line = 0;
	        }
	        else
		{
	    	    dm1cxlog_error(E_DM93ED_WRONG_PAGE_TYPE, r, parpage,
			       t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 0);
		    SETDBERR(dberr, 0, E_DMF014_DM1B_PARENT_INDEX);
	    	    s = E_DB_ERROR;
	    	    return s;
		}
	    }
	    s = dm1m_unfix_page(r, DM0P_RELEASE, parent, dberr);
	    if (s)
	        return s;
	    if (tidp->tid_tid.tid_page == 0)
	    {
	        s = E_DB_ERROR;
		SETDBERR(dberr, 0, E_DM0055_NONEXT);
	        return s;
	    }
	    s = dm1m_fix_page(r, tidp->tid_tid.tid_page, DM0P_WPHYS,
	        	      parent, dberr);
	    if (s)
	        return s;
	    parpage = *parent;
	} /* for (;;) */
    	return s;
} /* requalify_parent */


/*{
** Name: log_before_images - Log before images of the pages
**
** Description:
**
** Inputs:
**      r			Pointer to record access context
**					which contains table control
**					block (TCB) pointer, tran_id,
**					and lock information associated
**					with this request.
** 	sibling			Array of page pointers.
** 	numsiblings		Number of pages in the array.
** 	pages_mutexed		Ptr to flag: are pages mutexed?
**      err_code                        Pointer to an area to return
**                                      the following errors when
**                                      E_DB_ERROR is returned:
**
** Outputs:
**
** Returns:
**	E_DB_OK
**      E_DB_ERROR
**
** Exceptions:
**	none
**
** Side Effects:
**	none
**
** History:
** 	09-jul-1996 (wonst02)
**	    New.
**	10-jan-1997 (nanpr01)
**	    Added back the space holder for the page for preventing 
**	    misalignment.
**      21-apr-98 (stial01)
**          log_before_images() Set DM0L_PHYS_LOCK if extension table (B90677)
*/
static DB_STATUS
log_before_images(
    	DMP_RCB	    	*r,
    	DMPP_PAGE	**sibling,
	i4	    	numsiblings,
	bool	    	*pages_mutexed,
	DB_ERROR     	*dberr)
{
	DMP_TCB	    	*t = r->rcb_tcb_ptr;
	DMP_TABLE_IO    *tbio = &t->tcb_table_io;
	DMPP_PAGE	**page;
	DMPP_PAGE	**endsib = sibling + numsiblings;
	DMPP_PAGE	*curpage;
    	i4		loc_id;
    	i4		cur_loc_cnf_id;
	i4	    	dm0l_flag;
	i4	    	num;
	LG_LSN	    	lsn;
    	CL_ERR_DESC	sys_err;
	DB_STATUS	status = E_DB_OK;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

	if (numsiblings == 0)
	    return status;

	*pages_mutexed = FALSE;
	if (!(r->rcb_logging & RCB_LOGGING))
	{
	    *pages_mutexed = TRUE;
	    for (page = sibling; page < endsib; page++)
	    {
	    	dm1m_mutex(tbio, (i4)0, r->rcb_lk_id, page);
	    }
	}
	else
	{
	    /*
	    ** Reserve log file space for BIs and their CLRs
	    */
	    num = 2 * numsiblings;
	    status = LGreserve(0, r->rcb_log_id, num,
		       num * (sizeof(DM0L_BI) - sizeof(DMPP_PAGE) + 
		       t->tcb_rel.relpgsize), &sys_err);
	    if (status != OK)
	    {
		(VOID) uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		  		(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
				ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
				err_code, 1, 0, r->rcb_log_id);
		SETDBERR(dberr, 0, E_DM93B2_BXSPLIT_ERROR);
	    	return E_DB_ERROR;
	    }

	    /*
	    ** Mutex the pages before beginning the udpates.  This must be
	    ** done prior to the writing of the log record to satisfy
	    ** logging protocols.
	    */
	    *pages_mutexed = TRUE;
	    for (page = sibling; page < endsib; page++)
	    {
	    	dm1m_mutex(tbio, (i4)0, r->rcb_lk_id, page);
	    }

	    for (page = sibling; page < endsib; page++)
	    {
	    	curpage = *page;

    	    	/* Get information on the location where the update is.*/
    	    	loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count,
			(i4) DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type,
						   	   curpage));
    	    	cur_loc_cnf_id= tbio->tbio_location_array[loc_id].loc_config_id;


		dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);

		if ( r->rcb_state & RCB_IN_MINI_TRAN )
		    dm0l_flag |= DM0L_MINI;

		/* 
		** We use temporary physical locks for catalogs, extension
		** tables and index pages. The same lock protocol must be
		** observed during recovery.
		*/
	    	if (t->tcb_sysrel || t->tcb_extended || 
	            (! DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, curpage) &
		       DMPP_LEAF))
		    dm0l_flag |= DM0L_PHYS_LOCK;

	    	status = dm0l_bi(r->rcb_log_id, dm0l_flag, &t->tcb_rel.reltid,
	    		     	&t->tcb_rel.relid, &t->tcb_rel.relowner,
		  	     	t->tcb_rel.relpgtype, t->tcb_rel.relloccount,
		  	     	cur_loc_cnf_id, DM0L_BI_SPLIT,
		  	     	DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
			     			      	 curpage),
		  	     	t->tcb_rel.relpgsize, *page,
		  	     	(LG_LSN *) 0, &lsn, dberr);
            	if (status != E_DB_OK)
	    	    return E_DB_ERROR;

	    	DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, curpage,lsn);
	    }
	}

	return E_DB_OK;
}

/*{
** Name: log_after_images - Log after images of the pages
**
** Description:
**
** Inputs:
**      r			Pointer to record access context
**					which contains table control
**					block (TCB) pointer, tran_id,
**					and lock information associated
**					with this request.
** 	sibling			Array of page pointers.
** 	numsiblings		Number of pages in the array.
** 	pages_mutexed		Ptr to flag: are pages mutexed?
**      err_code                        Pointer to an area to return
**                                      the following errors when
**                                      E_DB_ERROR is returned:
**
** Outputs:
**
** Returns:
**	E_DB_OK
**      E_DB_ERROR
**
** Exceptions:
**	none
**
** Side Effects:
**	none
**
** History:
** 	10-jul-1996 (wonst02)
**	    New.
**	10-jan-1997 (nanpr01)
**	    Added back the space holder for the page for preventing 
**	    misalignment.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
*/
static DB_STATUS
log_after_images(
    	DMP_RCB	    	*r,
    	DMPP_PAGE	**sibling,
	i4	    	numsiblings,
	bool	    	*pages_mutexed,
	DB_ERROR     	*dberr)
{
	DMP_TCB	    	*t = r->rcb_tcb_ptr;
	DMP_TABLE_IO    *tbio = &t->tcb_table_io;
	DMPP_PAGE	**page;
	DMPP_PAGE	*curpage;
	i4	    	dm0l_flag;
    	i4		loc_id;
    	i4		cur_loc_cnf_id;
	LG_LSN	    	lsn;
    	CL_ERR_DESC	sys_err;
    	STATUS	    	cl_status;
	DB_STATUS	s = E_DB_OK;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (numsiblings == 0)
	return E_DB_OK;

    if (r->rcb_logging & RCB_LOGGING)
    {
	if (*pages_mutexed)
	{
	    for (page = sibling; page < sibling + numsiblings; page++)
	    {
	    	dm1m_unmutex(tbio, (i4)0, r->rcb_lk_id, page);
	    }
	    *pages_mutexed = FALSE;
	}

	/* Reserve log file space for AIs */
	cl_status = LGreserve(0, r->rcb_log_id, numsiblings,
	    	        numsiblings * (sizeof(DM0L_AI) - sizeof(DMPP_PAGE) + 
			t->tcb_rel.relpgsize), &sys_err);
	if (cl_status != OK)
	{
	    (VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    	      (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
		    	      ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    	      err_code, 1, 0, r->rcb_log_id);
	    SETDBERR(dberr, 0, E_DM93B2_BXSPLIT_ERROR);
	    return E_DB_ERROR;
	}

	*pages_mutexed = TRUE;
	for (page = sibling; page < sibling + numsiblings; page++)
	{
	     dm1m_mutex(tbio, (i4)0, r->rcb_lk_id, page);
	}

	for (page = sibling; page < sibling + numsiblings; page++)
	{
	    curpage = *page;

    	    /* Get information on the location where the update is being made.*/
    	    loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count,
		(i4) DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type,
						   curpage));
    	    cur_loc_cnf_id = tbio->tbio_location_array[loc_id].loc_config_id;

	    /* Physical locks are used on btree index pages */
	    dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);

	    if ( r->rcb_state & RCB_IN_MINI_TRAN )
	        dm0l_flag |= DM0L_MINI;

	    if (! DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, curpage) &
		    DMPP_LEAF)
	    	dm0l_flag |= DM0L_PHYS_LOCK;

	    s = dm0l_ai(r->rcb_log_id, dm0l_flag,
		    &t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		    t->tcb_rel.relpgtype, t->tcb_rel.relloccount, cur_loc_cnf_id,
		    DM0L_BI_NEWROOT,
		    DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, curpage),
		    t->tcb_rel.relpgsize, curpage,
		    (LG_LSN *) 0, &lsn, dberr);
	    if (s != E_DB_OK)
	    	break;

	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, curpage, lsn);
	    DM1B_VPT_SET_BT_SPLIT_LSN_MACRO(tbio->tbio_page_type, curpage, lsn);
	}
    }
    /*
    ** Unmutex the pages since the update actions described by
    ** the log record are complete.
    **
    ** Following the unmutexes, these pages may be written out
    ** to the database at any time.
    */
    if (*pages_mutexed)
    {
        for (page = sibling; page < sibling + numsiblings; page++)
            dm1m_unmutex(tbio, (i4)0, r->rcb_lk_id, page);
        *pages_mutexed = FALSE;
    }
    return s;
}

/*{
** Name: dm1m_check_range - Check to see if this key belongs on this page
**
** Description:
**
** Inputs:
**      r			Pointer to record access context
**				which contains table control
**				block (TCB) pointer, tran_id,
**				and lock information associated
**				with this request.
** 	page			Pointer to the page.
** 	key			Key to check.
**
** Outputs:
**
** Returns:
**	E_DB_OK
**      E_DB_ERROR
**
** Exceptions:
**	none
**
** Side Effects:
**	none
**
** History:
** 	11-jul-1996 (wonst02)
**	    New.
**	05-dec-1996 (wonst02)
**	    Allow left_range <= key <= right_range, instead of
**		  left_range <  key <= right_range.
*/
DB_STATUS
dm1m_check_range(
	i4		page_type,
	i4		page_size,
    	DMPP_PAGE	*page,
    	ADF_CB	 	*adf_cb,
    	DMPP_ACC_KLV	*klv,
	char		*key,
	bool		*result,
	DB_ERROR        *dberr)
{
    	i4	    	compare;
    	i4         left_inf, right_inf;
    	char	    	left_key[DB_MAXRTREE_KEY];
    	char	    	right_key[DB_MAXRTREE_KEY];
	DB_STATUS	s = E_DB_OK;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    for (;;)
    {
    	*result = FALSE;
	/*
	** Only a leaf page has meaningful infinity flags.
	*/
	if (DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page) & DMPP_LEAF)
	{
	    dm1cxtget(page_type, page_size, page, 
		DM1B_RRANGE, (DM_TID *)&right_inf, (i4*)NULL);
	    dm1cxtget(page_type, page_size, page, 
		DM1B_LRANGE, (DM_TID *)&left_inf, (i4*)NULL);
	}
	/*
	** An index page at the right edge: key must be in range.
	*/
	else if (DM1B_VPT_GET_BT_NEXTPAGE_MACRO(page_type, page) == 0)
	{
	    *result = TRUE;
	    break;
	}
	else
	{
	    right_inf = 0;
	    left_inf = 0;
	}

	if (! (right_inf && left_inf))
	{
	    dm1m_get_range(page_type, page_size, page, klv, left_key, right_key);
	}

	if (right_inf == 0) 
	{
	    s = dm1mxkkcmp(page_type, page_size, page, adf_cb, klv,
	    		   key, right_key, &compare);
	    if (s != E_DB_OK)
	    {
		uleFormat(NULL, E_DMF012_ADT_FAIL, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		SETDBERR(dberr, 0, E_DM93D0_BALLOCATE_ERROR);
		break;
	    }
 	    if (compare > 0)
	    	break;
	} /* if (right_inf == 0) */

	/*
	** If not a leaf page, do not compare the left range.
	*/
	if ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page) & DMPP_LEAF) &&
	    (left_inf == 0))
	{
	    s = dm1mxkkcmp(page_type, page_size, page, adf_cb, klv,
	    		   key, left_key, &compare);
	    if (s != E_DB_OK)
	    {
		uleFormat(NULL, E_DMF012_ADT_FAIL, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		SETDBERR(dberr, 0, E_DM93D0_BALLOCATE_ERROR);
		break;
	    }
 	    if (compare < 0)
	    	break;
	} /* if (left_inf == 0) */

	*result = TRUE;
	break;
    } /* for (;;) */

    return s;
}


/*{
** Name: dm1m_get_range - Get the low and high Hilbert values for the page
**
** Description:
**
** Inputs:
**      r			Pointer to record access context
**					which contains table control
**					block (TCB) pointer, tran_id,
**					and lock information associated
**					with this request.
** 	page			Pointer to the page.
** 	lowkey			Ptr to area for returning low key.
** 	highkey			Ptr to area for returning high key.
**
** 		NOTE: Lowkey and highkey should each point to an area that is
** 		large enough for an entire R-tree key, i.e., the MBR, LHV, and
** 		TIDP. For a 2-dimensional R-tree, the area is 22 bytes:
** 		 ______________________________________________
** 		|    MBR (12 bytes)      |   LHV (6)  |TIDP (4)|
** 		|________________________|____________|________|
**
** Outputs:
** 	lowkey			Ptr to area for returning low key.
** 	highkey			Ptr to area for returning high key.
**
** Returns:
**	E_DB_OK
**      E_DB_ERROR
**
** Exceptions:
**	none
**
** Side Effects:
**	none
**
** History:
** 	10-jun-1996 (wonst02)
**	    New.
** 	07-sep-1996 (wonst02)
** 	    Manufacture a Hilbert value if the infinity flag is set.
*/
void
dm1m_get_range(
	i4		page_type,
	i4		page_size,
    	DMPP_PAGE	*page,
    	DMPP_ACC_KLV	*klv,
	char		*lowkey,
	char		*highkey)
{
    	u_i2		mbr_size = 2 * klv->hilbertsize;
	char		*lkey_ptr;
	char		*rkey_ptr;
    	DM_TID		infinity[2];

	if (DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page) & DMPP_LEAF)
	{
	    dm1cxtget(page_type, page_size, page, 
		DM1B_LRANGE, &infinity[DM1B_LEFT], (i4*)NULL);
	    dm1cxtget(page_type, page_size, page, 
		DM1B_RRANGE, &infinity[DM1B_RIGHT], (i4*)NULL);
	}
	else
	{
	    infinity[DM1B_LEFT].tid_i4 = infinity[DM1B_RIGHT].tid_i4 = 0;
	}

	/*
	** Leaf page: get both low and high Hilbert values from the LRANGE key.
	** The low LHV is in the leftmost portion; the high LHV, the rightmost.
	** If cur and sib are different pages, get the high LHV from the sib.
	** Get the Minimum Bounding Region (MBR) from the RRANGE key.
	*/
	dm1cxrecptr(page_type, page_size, page, DM1B_LRANGE, &lkey_ptr);
	dm1cxrecptr(page_type, page_size, page, DM1B_RRANGE, &rkey_ptr);

	if (lowkey)
	{
	    MEcopy(rkey_ptr, mbr_size, lowkey);
	    if (infinity[DM1B_LEFT].tid_i4)
	    {
		MEfill( klv->hilbertsize, '\0',
		       ((PTR) (char *)lowkey + mbr_size));
	    }
	    else
	    {
	    	MEcopy(lkey_ptr, klv->hilbertsize, (char *)lowkey + mbr_size);
	    }
	}

	if (highkey)
	{
	    if (infinity[DM1B_RIGHT].tid_i4)
	    {
	    	MEcopy(rkey_ptr, mbr_size, highkey);
		MEfill( klv->hilbertsize, 0xFF,
		       ((PTR) (char *)highkey + mbr_size));
	    }
	    else if (DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page)
	    	     & DMPP_LEAF)
	    {
	    	MEcopy(rkey_ptr, mbr_size, highkey);
	    	MEcopy((char *)lkey_ptr + klv->hilbertsize, klv->hilbertsize,
	       	       (char *)highkey + mbr_size);
	    }
	    else
	    	MEcopy(rkey_ptr, mbr_size + klv->hilbertsize, highkey);
	}
} /* dm1m_get_range */


/*{
** Name: findpage - Finds a page for an Rtree data insertion.
**
** Description:
**      This routines finds the associated data page of the found leaf
**      level page.  If this page has room for the record, it returns.
**      Otherwise, a new page is allocated and associated with the leaf
**      page.  The old full data page is diassociated.
**      The allocation of a new data page is logged for recovery.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      leaf                            Pointer to the leaf page.
**      need                            Value indicating how much space
**                                      is needed on the data page.
**      record
**
** Outputs:
**      data                            Pointer to an area where data
**                                      page pointer can be returned.
**	tid				Pointer to tid for allocated record.
**      err_code                        Pointer to an area to return
**                                      error code.
**                                      The error codes are:
**                                      E_DM0019_BAD_FILE_WRITE and
**                                      errors from dm1mpgetfree.
**
**	Returns:
**
**	    E_DB_OK
**	    E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**          Fixes and locks data page.
**
** History:
** 	27-may-1996 (wonst02)
**	    New Page Format Support:
**		Change page header references to use macros.
*/
static DB_STATUS
findpage(
    DMP_RCB	    *rcb,
    DMPP_PAGE       *leaf,
    DMPP_PAGE       **data,
    char	    *record,
    i4         need,
    DM_TID	    *tid,
    DB_ERROR        *dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DMPP_PAGE	    *newdata = 0;
    DMPP_PAGE	    *leafpage;
    DB_STATUS	    s;
    i4		    *err_code = &dberr->err_code;
    DMP_PINFO	    pinfo;

    DMP_PINIT(&pinfo);

    CLRDBERR(dberr);

    /*	See if the current data page, if any, is for the correct leaf. */

    leafpage = leaf;
    if (*data && DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *data) !=
	DM1B_VPT_GET_BT_DATA_MACRO(t->tcb_rel.relpgtype, leafpage))
    {
	/*	Unfix this unusable page. */

	s = dm1m_unfix_page(r, DM0P_UNFIX, data, dberr);
	if (s != E_DB_OK)
	    return (s);
    }
    if (*data == 0)
    {
	/* Fix the associated data page.    */

	s = dm1m_fix_page(r,
	    DM1B_VPT_GET_BT_DATA_MACRO(t->tcb_rel.relpgtype, leafpage),
	    DM0P_WRITE | DM0P_NOREADAHEAD, data, dberr);
	if (s != E_DB_OK)
	    return (s);
    }

    /* Allocate space for record. */
    pinfo.page = *data;

    return(dm1r_allocate(r, &pinfo, record, need, tid, dberr));

} /* findpage( */


/*{
** Name: check_reclaim - Determines if a data page can be reclaimed.
**
** Description:
**      This routines checks the data page that is paased in to determine
**	if the page is a DISASSOCiated data page and if the pages has no
**	more records.  If this is the case, the page is returned to the
**	free list and any rcb's that reference the page will have the
**	page UNFIXED.
**
**	If data == rcb->rcb_data.page then data must be a copy of
**	the pointer.  It can't be fixed independently.  This code will
**	zero rcb->rcb_data.page when it equals data, and it will
**	unfix data.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      data				Pointer to the data page.
**
** Outputs:
**      err_code                        Pointer to an area to return
**                                      error code.
**                                      The error codes are:
**                                      E_DM0019_BAD_FILE_WRITE and
**                                      errors from dm1p_putfree.
**
**	Returns:
**
**	    E_DB_OK
**	    E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**          Fixes and locks data page.
**
** History:
** 	27-may-1996 (wonst02)
**	    New Page Format Support:
**		Change page header references to use macros.
*/
static DB_STATUS
check_reclaim(
    DMP_RCB	*rcb,
    DMPP_PAGE   **data,
    DB_ERROR    *dberr)
{
    DMP_RCB	*r = rcb;
    DMP_TCB	*t = rcb->rcb_tcb_ptr;
    DMP_RCB	*next_rcb;
    DB_STATUS	status;
    i4		    *err_code = &dberr->err_code;
    DMP_PINFO   pinfo;

    CLRDBERR(dberr);

    DMP_PINIT(&pinfo);
    pinfo.page = *data;

    /*  Check to see if this is an empty disassociated data page. */

    if ((DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, (*data))
	 & DMPP_ASSOC) ||
	((*t->tcb_acc_plv->dmpp_tuplecount)(*data, t->tcb_rel.relpgsize) != 0))
    {
	return (E_DB_OK);
    }

    /*
    ** Check that the page is not fixed in any related RCB.  If it is
    ** then unfix it for that caller.
    */
    for (next_rcb = r->rcb_rl_next;
	next_rcb != r;
	next_rcb = next_rcb->rcb_rl_next)
    {
	if (next_rcb->rcb_data.page == *data)
	{
	    status = dm0p_unfix_page(next_rcb, DM0P_UNFIX,
		&next_rcb->rcb_data, dberr);
	    if (status != E_DB_OK)
		return (status);
	}
    }

    /*	Return page to the free list. */

    status = dm1p_putfree(r, &pinfo, dberr);

    return (status);
} /* check_reclaim */

/*{
** Name: dm1m_badtid - Flag bad Rtree tid
**
** Description:
**	Called when Rtree access code finds a bad TID pointer on a leaf
**	page - logs information about the leaf.
**
** Inputs:
**     rcb                             Pointer to record access context
**                                     which contains table control
**                                     block (TCB) pointer, tran_id,
**                                     and lock information associated
**                                     with this request.
**     bid
**     tid				Pointer to the bad one
**     key
**
** Outputs:
**     None
**
** History:
*/
VOID
dm1m_badtid(
    DMP_RCB		*rcb,
    DM_TID		*bid,
    DM_TID		*tid,
    char		*key)
{
    DMP_RCB	*r = rcb;
    DMP_TCB     *t = r->rcb_tcb_ptr;
    DB_STATUS	err_code;
    char	local_1buf[13];
    char	local_2buf[25];
    i4		k;

    /*
    ** Rtree is inconsistent.  Give error message indicating bad
    ** index entry and return BAD_TID.  Try to give useful
    ** information by printing out 1st 12 bytes of Key.  Would be
    ** nice if ule_format would allow us to send a btye array
    ** argument to be printed out in HEX format.
    */
    MEmove(t->tcb_klen, (PTR)key, ' ', 12, (PTR)local_1buf);
    MEfill(24, ' ', (PTR)local_2buf);
    for (k = 0; (k < 12) && (k < t->tcb_klen); k++)
    {
	if (!CMprint(&local_1buf[k]))
	    local_1buf[k] = '.';
	STprintf(&local_2buf[k*2], "%02x", (i4)key[k]);
    }
    local_1buf[12] = EOS;
    local_2buf[24] = EOS;

    uleFormat(NULL, E_DM9351_BTREE_BAD_TID, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	(char *)NULL, (i4)0, (i4 *)NULL, &err_code, 9,
	sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
	sizeof(DB_OWN_NAME), &t->tcb_rel.relowner,
	sizeof(DB_DB_NAME), &t->tcb_dcb_ptr->dcb_name,
	0, bid->tid_tid.tid_page, 0, bid->tid_tid.tid_line,
	0, tid->tid_tid.tid_page, 0, tid->tid_tid.tid_line,
	12, local_1buf, 24, local_2buf);
} /* dm1m_badtid( */


/*{
** Name: dm1m_count - count records in an Rtree table.
**
** Description:
**
** 	This function counts how many records there are in an Rtree
**      table, for use by count(*) agg function, as part of DMR_COUNT
**      request processing.
** 	Data pages are not read; the bt_kids fields on the leaf pages are
**	used to count how many records the leaves point to.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      direction                       Ignored, just for compatibility
**                                      with dm2r_get parms since dm1r_count
**                                      may be called interchangably via ptr.
**      record                          Ignored, just for compatibility
**                                      with dm2r_get parms since dm1r_count
**                                      may be called interchangably via ptr.
**
** Outputs:
**      countptr                        Ptr to where to put total count of
**                                      records in the table.
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**                                      Error codes returned are:
**                                      E_DM008A_ERROR_GETTING_RECORD
**                                      E_DM93A2_DM1R_GET
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	23-Oct-1996 (wonst02)
**	    Cloned from dm1b_count.
**	01-Dec-2008 (jonj)
**	    SIR 120874: dm2a_?, dm1?_count functions converted to DB_ERROR *
*/

DB_STATUS
dm1m_count(
    DMP_RCB         *rcb,
    i4	    *countptr,
    i4         direction,
    void            *record,
    DB_ERROR        *dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB       *t = r->rcb_tcb_ptr;
    DB_STATUS	    s = E_DB_OK;	/* Variable used for return status. */
    DM_PAGENO	    nextleaf;
    i4	    count = 0;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** Loop through leaf pages to add up bt_kids. This loop
    ** assumes that a previous position call already fixed the first leaf pg.
    */

    while (r->rcb_other.page)
    {
	/* Housekeeping - check for user interrupt or force abort */
	if (*(r->rcb_uiptr) & RCB_USER_INTR)
	{
	    SETDBERR(dberr, 0, E_DM0065_USER_INTR);
	    s = E_DB_ERROR;
	    break;
	}

	if (*(r->rcb_uiptr) & RCB_FORCE_ABORT)
	{
	    SETDBERR(dberr, 0, E_DM010C_TRAN_ABORTED);
	    s = E_DB_ERROR;
	    break;
	}

	/* keep count of tuples each leaf points to */
	count += DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, 
			r->rcb_other.page);

	/* Make note of next leaf number. Release pg */
	nextleaf = DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype, 
			r->rcb_other.page);
	s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other, dberr);
	if (s != E_DB_OK)
	    break;

	/* now read in next leaf page if any */
	if (nextleaf == 0)
	    break;	/* end of scan, all entries counted */

	s = dm0p_fix_page(r, nextleaf, DM0P_READ, &r->rcb_other, dberr);
	if (s != E_DB_OK)
	    break;
    }

    if (s == E_DB_OK)
    {
	*countptr = count;
	return (E_DB_OK);
    }

    /*	Handle error reporting and recovery. */

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9261_DM1B_GET);
    }

    return (s);
} /* dm1m_count */


/*{
** Name: dm1m_kperpage - Calculate keys per page
**
** Description:
**      This routine calculates keys per page which varies depending on
**      page size, attribute count, key length and page type (index vs. leaf).
**      Also the DMF table version is required for backwards compatibility.
**
** Inputs:
**      page_size          Page size
**      relcmptlvl         DMF table version
**      atts_cnt           Number of attributes
**      key_len            Key length
**      page_type          index vs. leaf
** 
** Outputs:
**      keys per page
**
**
** Exceptions:
**		None
**
** History:
**      07-Dec-98 (stial01)
**          Written.
**      09-Feb-99 (stial01)
**          dm1m_kperpage() relcmptlvl param added for backwards compatability
*/
i4
dm1m_kperpage(i4 page_type,
	i4 page_size,
	i4      relcmptlvl,
	i4 atts_cnt,
	i4 key_len,
	i4 indexpagetype)
{
    /*
    ** Rtree incorrectly included TIDP in mct_klen, tcb_klen
    ** HOWEVER IT IS ACTUALLY NOT PART OF THE KEY
    ** This is one more place where the key length must be
    ** adjusted to what the key length really is.
    ** (Search for DM1B_TID_S in dm1m, dm1mindex)
    ** At some point mct_klen, tcb_klen should be correctly calculated 
    ** for RTREE.
    */
    key_len -= DM1B_TID_S;
    return (dm1cxkperpage(page_type, page_size, relcmptlvl, TCB_RTREE, 0, 
    		key_len, indexpagetype, sizeof(DM_TID), 0));
}

DB_STATUS
dm1m_fix_page(
DMP_RCB		*r,
DM_PAGENO	page_number,
i4		action,
DMPP_PAGE	**page,
DB_ERROR	*dberr)
{
    DB_STATUS	status;
    DMP_PINFO	pinfo;

    status = dm0p_fix_page(r, page_number, action, &pinfo, dberr);
    *page = pinfo.page;
    return(status);
}

DB_STATUS
dm1m_unfix_page(
DMP_RCB		*r,
i4		action,
DMPP_PAGE	**page,
DB_ERROR	*dberr)
{
    DMP_PINFO	pinfo;

    DMP_PINIT(&pinfo);
    pinfo.page = *page;
    *page = NULL;

    return(dm0p_unfix_page(r, action, &pinfo, dberr));
}

VOID
dm1m_mutex(
DMP_TABLE_IO	*tbio,
i4		flags,
i4		lock_list,
DMPP_PAGE	**page)
{
    DMP_PINFO	pinfo;

    DMP_PINIT(&pinfo);
    pinfo.page = *page;

    dm0pMutex(tbio, flags, lock_list, &pinfo);
}

VOID
dm1m_unmutex(
DMP_TABLE_IO	*tbio,
i4		flags,
i4		lock_list,
DMPP_PAGE	**page)
{
    DMP_PINFO	pinfo;

    DMP_PINIT(&pinfo);
    pinfo.page = *page;

    dm0pUnmutex(tbio, flags, lock_list, &pinfo);
}

DB_STATUS
dm1m_getfree(
DMP_RCB		*r,
i4		flags,
DMPP_PAGE	**page,
DB_ERROR	*dberr)
{
    DB_STATUS	status;
    DMP_PINFO	pinfo;

    status = dm1p_getfree(r, flags, &pinfo, dberr);
    *page = pinfo.page;
    return(status);
}
