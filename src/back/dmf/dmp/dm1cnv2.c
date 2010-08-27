/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <pc.h>
#include    <tr.h>
#include    <st.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpl.h>
#include    <dmpp.h>
#include    <dmppn.h>
#include    <dm0m.h>
#include    <dm0p.h>
#include    <dm1c.h>
#include    <dm1cnv2.h>
#include    <dm1u.h>
#include    <dm1b.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dmxe.h>
#include    <dmftrace.h>

/**
**
**  Name: DM1CNV2.C - Normal format Accessor routines to manipulate records 
**		    and data pages for page sizes > 2k.
**
**  Description:
**      This file contains all the routines needed to manipulate 
**      data pages as a set of accessors. Since the manipulation 
**      is done via accessor functions a set needs to be provided for each
**	supported page type. These are then formed into the two accessor
**	structures referenced by the dm1c_getaccessors function:
**
**	dm1cnv2_plv	- page level vector for normal pages
**	dm1cn_tlv	- tuple level vecor for normal pages
**
**      The normal (dm1cnv2_) functions defined in this file are:
**
**      allocate	- Allocates space for a record on a page.
**	check_page	- Determine if page_next_line is consistent
**	delete		- Deletes a record
**	format		- formats a page
**	get		- gets a record from a data page
**	getfree		- Return amount of freespace on a page
**	get_offset	- Return offset of tuple within a page
**	isnew		- Was this tuple modified by this cursor?
**      ixsearch	- Binary search an ISAM index page.
**	load		- Allocate and put a record for data page load
**	put		- puts a record on a page
**	reallocate	- reallocate space for a record at a given TID
**	reclen		- computes the length of a record on a page
**	tuplecount	- Return number of tuples on a page. 
**
**	Tuple level vector functions for this page type reside in dm1ccmpn.c
**
**	
**  History:
**	06-may-1996 (nanpr01 & thaju02)
**	    Created New Page Format Support:
**		Added page_size argument to dm1cn_clear function prototype.
**		Pass page_size argument to dmppn_get_offset_macro,
**		dmppn_put_offset_macro. Pass page_size, and page pointer
**		arguments to dmppn_test_new_macro, dmppn_set_new_macro, 
**		dmppn_clear_new_macro.(due to tuple header implementation,
**		moving the new and free bits from the line table entry to
**		the tuple header flags field)
**	12-may-1996 (thaju02)
**	    Clear flags field in line table entry when setting new offset.
**	    Added dmppn_clear_flags_macro.  Neglected to remove history
**	    when this file was created - cleaned up after-the-fact.
**      20-jun-1996 (thaju02)
**          Use dm0m_lfill() when filling page with page size greater
**          than max value for u_i2.
**	17-jul-1996 (ramra01)
**	    Store ver number in tuple header as part of Alter Table add/
**	    drop column support in the load and put accessor routines.
**	23-sep-1996 (canor01)
**	    Moved global data definitions to dmpdata.c.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Delete accessor: added reclaim_space 
**          Put/load accessor: changed DMPP_TUPLE_INFO param to table_version
**          dm1cnv2_allocate: Don't assume we will find an available line table
**              entry. With large pages we may run out of line table entries
**              BEFORE we run out of space.
**          dm1cnv2_delete: Can't reclaim space until commit, copy tranid to hdr
**          dm1cnv2_get: Return tran_id of deleted tuples
**          dm1cnv2_put() Copy tran_id into tuple hdr
**          dm1cnv2_clear() Removed this function 
**          dm1cnv2_isnew() new bit not used anymore, use seq# in tuple hdr
**          dm1cnv2_tuplecount() ignore deleted tuples
**      27-feb-1997 (stial01)
**          dm1cnv2_delete() Set DMPP_CLEAN in page status when we mark tuple
**          deleted, Clear DMPP_CLEAN when no more deleted tuples on page.
**          dm1cnv2_get() Init *record = NULL if tid points to empty line table
**          dm1cnv2_put() If put and tid allocated, just clear flags.
**          Removed unecessary reclaim_space function.
**       12-Mar-1997 (linke01)
**          Except default page_size 2K, anyother page_size crashed the
**          database on Backend Access test. Added NO_OPTIM for pym_us5.
**      07-jul-98 (stial01)
**          Deferred update processing for V2 pages, use page tranid
**          when page locking, tuple header tranid when row locking
**          dm1cnv2_get() Always return tranid of tuple
**	01-dec-1998 (nanpr01)
**	    Get rid of DMPP_TUPLE_INFO structure. Use DMPP_TUPLE_HDR instead.
**	    Also there is no need to check the nullability of tuple header
**	    since it is always passed. 
**      09-feb-1999 (stial01)
**          dm1cnv2_get() No more tuple headers on DMPP_DIRECT pages
**          dm1cnv2_ixsearch() Check relcmptlvl to see if tuphdr on index pages 
**          dm1cnv2_load() No more tuple headers on DMPP_DIRECT pages
**      10-feb-99 (stial01)
**          dm1cnv2_load() Do not limit # entries on V2 ISAM INDEX pages
**      15-apr-99 (stial01)
**          dm1cnv2_put() clear entire tuple header (lost on 07-07-98)
**      21-oct-99 (nanpr01)
**          More CPU optimization. Copy the tuple header on a get if tuple
**	    header is passed.
**      05-dec-99 (nanpr01)
**          Added tuple header in prototype to reduce memcopy.
**      05-jun-2000 (stial01)
**          Partial backout of change 436735 for B101708 Changes regarding
**          when to set tranid and sequence number in tuple header.
**	23-Jun-2000 (jenjo02)
**	    Added fill_option to dmpp_format() prototype, set by caller
**	    to DM1C_ZERO_FILL if dmpp_format should MEfill the page,
**	    DM1C_ZERO_FILLED if the page has already been MEfilled.
**	    The page overhead space is always zero-filled.
**      30-oct-2000 (stial01)
**          Pass rcb to allocate,isnew page accessors, 
**          Moved space reclamation code to allocate accessor
**      01-feb-2001 (stial01)
**          Arg changes for page accessors (Variable Page Type SIR 102955)
**          Updated for new page types (V3,V4)
**          No deferred processing for delete, it is done in opncalcost.c b59679
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**      17-jul-2001 (stial01)
**	    dm1cnv2_allocate() compare del_recsize to orig_rec_size (b105256)
**      08-mar-2002 (stial01)
**          dm1cnv2_reallocate() fixed space calculations (b107293)
**      08-Oct-2002 (hanch04)
**          Removed dm0m_lcopy,dm0m_lfill.  Use MEcopy, MEfill instead.
**      28-may-2003 (stial01)
**          Added line_start parameter to dmpp_allocate accessor
**          Keep deferred tid list sorted 
**      02-jul-2003 (stial01)
**          Fixed line_start parameter. (b110521)
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      31-jan-05 (stial01)
**          Added flags arg to dm0p_mutex call(s).
**      21-feb-2005 (stial01)
**          Cross integrate 470401 (inifa01) for INGSRV2569 b111886
**          dmpp_delete: Trim line table if rollback, and data is compressed
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	08-Dec-2008 (jonj)
**	    Replaced CL_SYS_ERR with CL_ERR_DESC missed in last change.
**	25-Aug-2009 (kschendel) 121804
**	    Need dmxe.h to satisfy gcc 4.3.
**      24-nov-2009 (stial01)
**          Check if table has segmented rows (instead of if pagetype supports)
**	15-Jan-2010 (jonj,stial01)
**	    SIR 121619 MVCC: Changes for MVCC
**	18-Feb-2010 (stial01)
**          dm1cnv2_get() fix copying of i2 version to caller param (i4) 
**      12-Jul-2010 (stial01) (SIR 121619 MVCC, B124076, B124077)
**          dm1cnv2_allocate(), dm1cnv2_clean() if crow_locking, call
**          row_is_consistent() else call IsTranActive()
**          dm1cnv2_isnew() Check rcb_new_fullscan, handle the XTABLE case where
**          new tids are added to rcb_new_tids without any update to the page,
**          defer_add_new() Set rcb_new_fullscan if page wasn't updated.
*/

/* NO_OPTIM = pym_us5 */


/*
**  Forward function references (used in accessor definition).
*/

static VOID	    
dm1cnv2_clear(
			DMP_RCB		*r,
			DMPP_PAGE       *page);


/*{
** Name: dm1cnv2_plv - Normal format page level vector of accessors
**
** Description:
**	This the the accessor structure that dm1c_getaccessors will use when
**	passed parameters asking for standard page accessors.
**
** History:
**	06-may-1996 (thaju02)
**	    Created.
**	23-sep-1996 (canor01)
**	    Moved to dmpdata.c.
**	29-may-1998 (nanpr01)
**	    There is really no reason to call this macro rather use
**	    the version 2 macro.
*/

GLOBALREF DMPP_ACC_PLV dm1cnv2_plv;
GLOBALCONSTREF DMPP_PAGETYPE Dmpp_pgtype[];


/*{
** Name: dmpp_allocate - Allocates space for a record on data page.
**
** Description:
**    This checks the current page for space and returns a tid that can be
**    used in a call to dm1cc_put.  If no space is found an alternate status
**    is returned.  The page is not touched in any way.
**
** Inputs:
**       data                           Pointer to the page to allocate from.
**       r                              rcb
**       record_size                    Amount of space needed
**
** Outputs:
**      tid                             Pointer to an area to return the tid
**                                      of the allocated space.
**      
**      
**	Returns:
**
**	    E_DB_OK			Space found, tid set.
**	    E_DB_INFO			No space found.
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**	06-may-1996 (nanpr01 & thaju02)
**	    Created for New Page Format Project.
**	    Change all page header access as macro for New Page Format project.
**	    Pass page_size argument to dmppn_get_offset_macro due to  tuple
**	    header implementation.
**          Account for tuple header in record size.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          dm1cnv2_allocate: Don't assume we will find an available line table
**          entry. With large pages we may run out of line table entries
**          BEFORE we run out of space.
**	29-may-1998 (nanpr01)
**	    There is really no reason to call this macro rather use
**	    the version 2 macro.
**	19-Feb-2008 (kschendel) SIR 122739
**	    Test for compresses-coupons rather than compression type.
*/
DB_STATUS    
dm1cnv2_allocate(
			DMPP_PAGE		*data,
			DMP_RCB                 *r,
			char			*record,
			i4			record_size,
			DM_TID			*tid,
			i4			line_start)

{
    i4     i;
    i4     nextlno;
    i4	space;
    DM_LINEID	*lp = DMPPN_V2_PAGE_LINE_TAB_MACRO(data);
    DM_TID    del_tid;
    i4        del_recsize;
    DMP_TCB   *t = r->rcb_tcb_ptr;
    DB_STATUS status;
    char	*tup_hdr;
    i4  orig_rec_size = record_size;
    bool	reclaim_same_tran;
    u_i4	row_low_tran;
    u_i2	row_lg_id = 0;
    i4		clean_bytes = 0;

    record_size += Dmpp_pgtype[t->tcb_rel.relpgtype].dmpp_tuphdr_size;

    /* Find empty line table entry. */
    
    tid->tid_tid.tid_page = DMPP_V2_GET_PAGE_PAGE_MACRO(data);
    nextlno = DMPP_V2_GET_PAGE_NEXT_LINE_MACRO(data);

    /* Check whether there is enough space */
    space = dm1cnv2_getfree(data, t->tcb_rel.relpgsize) 
	+ DM_V2_SIZEOF_LINEID_MACRO;
    if (space >= record_size)
    {
	/* Scan for empty line table entry. */
	for (i = line_start; i < nextlno; i++)
	{
	    if (dmppn_v2_get_offset_macro(lp, (i4)i))
		continue;
	    tid->tid_tid.tid_line = i;
	    return (E_DB_OK);
	}

	/*
	** No free line numbers, use nextlno assuming we have space
	** and another entry available
	*/
	if ((space - DM_V2_SIZEOF_LINEID_MACRO >= record_size)
	    && nextlno <= DM_TIDEOF)
	{
	    tid->tid_tid.tid_line = nextlno;
	    return (E_DB_OK);
	}
    }

    /* Try to reclaim space */ 
    if ((DMPP_V2_GET_PAGE_STAT_MACRO(data) & DMPP_CLEAN) == 0)
	return (E_DB_INFO);

    /*
    ** There are restrictions when we can reclaim space from current transaction
    */
    if ( (row_locking(r) || crow_locking(r)) && 
	(t->tcb_rel.relspec == TCB_HASH ||
	t->tcb_rel.relspec == TCB_ISAM ||
	t->tcb_data_rac.compresses_coupons))
    {
	/*
	** DISALLOW reclaim space by same transaction in HASH/ISAM table/index
	** This is so that we can do key comparisons on uncommitted data
	** without a row lock
	** Disallow reclaim space by same transaction if coupons are
	** compressed (without even checking whether the row contains
	** any LOB's!  FIXME!)  The real coupon may not compress as much
	** as the fake coupon initially put into the row.
	** (The reserve space may be greater than the put size... and dm1r_put
	** will dmpp_delete, dmpp_put... giving back some space to the page,
	** no longer reserved for this user in case of rollback
	*/
	reclaim_same_tran = FALSE;
    }
    else
	reclaim_same_tran = TRUE;

    for (i = line_start; i < nextlno; i++)
    {
	if (dmppn_v2_test_free_macro(lp, i) == 0)/* reclaimable row? */
	    continue;

	/*
	** Row previously deleted, check if committed
	** Allocate does not reclaim space until delete is committed
	**
	** ROW LOCKING SPACE RESERVATION PROTOCOL (dm1cnv2_allocate)
	** Allocate does not reclaim space until delete is committed
	** Row locking deletes put lowtran in tuphdr (dm1cnv2_delete)
	*/
	if (row_locking(r) || crow_locking(r) || t->tcb_sysrel || t->tcb_extended)
	{
	    tup_hdr = ((char*)data + dmppn_v2_get_offset_macro(lp, (i4)i));

	    /*
	    ** Check for current transaction id (which is alive)
	    ** For space reclamation we only need to look at low tran
	    ** which is guaranteed to be unique for live transactions
	    */
	    if ( Dmpp_pgtype[t->tcb_rel.relpgtype].dmpp_has_lgid )
	    {
		MECOPY_CONST_2_MACRO(
		 (PTR)tup_hdr + Dmpp_pgtype[t->tcb_rel.relpgtype].dmpp_lgid_offset,
		     &row_lg_id);
	    }
	    else
		row_lg_id = 0;

	    MECOPY_CONST_4_MACRO(
		 (PTR)tup_hdr + Dmpp_pgtype[t->tcb_rel.relpgtype].dmpp_lowtran_offset,
		 &row_low_tran);
	    if (row_low_tran == r->rcb_tran_id.db_low_tran)
	    {
		if (!reclaim_same_tran)
		    continue;
	    }
	    else
	    {
		/* Check if deleted row is committed */

		if (crow_locking(r))
		{
		    /*
		    ** Don't reclaim unless committed before consistent read
		    ** point in time described by the transactions "crib"
		    */
		    if (!row_is_consistent(r, row_low_tran, row_lg_id))
		    {
			/* skip and don't delete */
			continue;
		    }
		}
		else if ( IsTranActive(row_lg_id, row_low_tran) )
		    continue;
	    }
	}

	if (t->tcb_rel.relcomptype == TCB_C_NONE && !t->tcb_rel.relversion &&
		t->tcb_seg_rows == 0)
	{
	    /*
	    ** No compression, no table version, rowsize < pagesize
	    ** Assume all entries the same
	    */
	    tid->tid_tid.tid_line = i;
	    return (E_DB_OK);
	}
	else
	{
	    /* compressed or altered tables: get entry size */
	    dm1cnv2_reclen(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, data, i, &del_recsize);
	}

	if (del_recsize == orig_rec_size)
	{
	    tid->tid_tid.tid_line = i;
	    return (E_DB_OK);
	}

	/*
	** The page isn't mutexed during allocate so we can't delete/clean here
	** Keep looking for space and keep track of #bytes that can be cleaned
	** (Possible for tables having data compression or altered tables)
	*/
	clean_bytes += del_recsize;
    }

    /*
    ** FIX ME
    ** This is needed for rollback if DMPP_VPT_PAGE_HAS_SEGMENTS and compression
    ** But I think it is a general problem rollback compressed data
    ** We shouln't reclaim space for same transaction if the data is 
    ** compressed --- one large row may be replaced by many small rows
    ** and then rollback won't be able to put the large row back due
    ** to the line table expansion which doesn't get undone
    ** NOTE reclaim_same_tran only applies to row locking transactions
    ** page locking always assumes reclaim_same_tran, 
    ** dm1cnv2_put doesn't even store tran id in hdr
    */
    if (t->tcb_seg_rows
	&& t->tcb_rel.relcomptype != TCB_C_NONE
	&& DMPP_V2_GET_TRAN_ID_HIGH_MACRO(data) == r->rcb_tran_id.db_high_tran
	&& DMPP_V2_GET_TRAN_ID_LOW_MACRO(data) == r->rcb_tran_id.db_low_tran)
	return (E_DB_INFO);

    if (clean_bytes >  record_size)
    {
	/*
	** Possible optimization for tables having data compression
	** Return the number of bytes that page clean might reclaim
	** The caller could then skip the page clean if it wouldn't help
	*/
    }

    return(E_DB_INFO);
}

/*{
** Name: dmpp_isempty - Check whether page is empty.
**
** Description:
**    This routine returns non-zero if page is empty, otherwise returns 0.
**    Given a page, this accessor returns if the page is empty.
**    This routine is called to check if the page can be returned 
**    to the free list.
**    This routine is not called during recovery routines
**
** Inputs:
**      data                      	Pointer to the page.
**      rcb				Rcb pointer
**
** Outputs:
** 	None      
**
** Returns:
**      FALSE           page was not empty
**      TRUE            page is empty
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**      30-jan-2001 (stial01)
**          created.
*/
bool    
dm1cnv2_isempty(
			DMPP_PAGE	*data,
			DMP_RCB		*rcb)
{
    register i4 i;
    DM_LINEID	*lp = DMPPN_V2_PAGE_LINE_TAB_MACRO(data);
    DMP_TCB	*t = rcb->rcb_tcb_ptr;
    char       *del_row;
    DB_STATUS   status;
    i4          err_code;
    u_i4	row_low_tran;
    u_i2	row_lg_id = 0;
    char	*tup_hdr;

    for (i = 0; i != DMPP_V2_GET_PAGE_NEXT_LINE_MACRO(data); ++i)
    {
	if (dmppn_v2_get_offset_macro(lp, i) == 0)
	    continue;
	
	if (! dmppn_v2_test_free_macro(lp, i) )
	    return (FALSE); /* not empty */
	
	/* Row previously deleted, check if committed */
	if (row_locking(rcb) || crow_locking(rcb) || t->tcb_sysrel || t->tcb_extended)
	{
	    tup_hdr = ((char*)data + dmppn_v2_get_offset_macro(lp, (i4)i));

	    MECOPY_CONST_4_MACRO(
		(PTR)tup_hdr + Dmpp_pgtype[t->tcb_rel.relpgtype].dmpp_lowtran_offset,
		 &row_low_tran);

	    /* Same low tranid, skip deleted tuple */
	    if (row_low_tran == rcb->rcb_tran_id.db_low_tran)
		continue;

	    if ( Dmpp_pgtype[t->tcb_rel.relpgtype].dmpp_has_lgid )
	    {
		MECOPY_CONST_2_MACRO(
		 (PTR)tup_hdr + Dmpp_pgtype[t->tcb_rel.relpgtype].dmpp_lgid_offset,
		     &row_lg_id);
	    }
	    else
		row_lg_id = 0;

	    /* Check if deleted row is committed */
	    if ( IsTranActive(row_lg_id, row_low_tran) )
		return (FALSE); /* not empty if uncommitted deletes */
	    else
		continue;
	}
    }

    return(TRUE); /* empty */
}

/*{
** Name: dmpp_check_page	- Determine if page_next_line is  consistent
**
** Description:
**
**      This routine attempts to determine if page_next_line is valid. The
**      logic to do this is influenced STRONGLY by the patch table approach
**	specified by the user.  (The user may specify the save_data (forgiving)
**	approach that attempts to save as much user data as possible, or the
**	strict (force) approach that throws away all tuples on any data page
**	with structural damage.)  ALSO this routine is used by the check table
**	operation as thought the strict approach had been specified.  (THIS
**	ROUTINE WILL NOT MODIFY THE PAGE IF THE STRICT OPTION IS SELECTED, SO IT
**	IS SAFE FOR THE CHECK OPERATION LOGIC TO USE.)
**
**	If the strict option is specified, then processing will terminate as 
**	soon as page_next_line is found to be invalid, and return_status=FALSE.
**
**	This routine should only be called for a DATA page. If it is called for
**	a non-data page (an index page in a btree, for example), it may well
**	report the page as structurally unsound. This is usually OK, since
**	patch table typically throws away all non-data pages anyway, but you
**	should be aware of this behavior.
**
** Inputs:
**	dm1u			dm1u control block pointer
**	page			a fixed data page
**	page_size		page size.
**
** Outputs:
**      page->page_next_line            value on page may be reassigned
**	page->page_contents		values may be reassigned
**
**	Returns:
**	    DM1U_GOOD		page->page_next_line is valid
**	    DM1U_FIXED		page->page_next_line was fixed to look valid
**	    DM1U_BAD		page->page_next_line is NOT valid
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-may-1996 (nanpr01 & thaju02)
**	    Created for New Page Format Project:
**	    Change all page header access as macro for New Page Format project.
**	    Pass page_size to dmppn_get_offset_macro for implementing tuple
**	    header.
*/
/* 
** "Traditional" page format implementation
*/
DB_STATUS    
dm1cnv2_check_page(
			DM1U_CB			*dm1u,
			DMPP_PAGE		*page,
			i4			page_size)
{
    DM_LINEID	*lp = DMPPN_V2_PAGE_LINE_TAB_MACRO(page);
    i4	    min_offset;
    i4	    min_offset_line = 0;
    i4	    page_next_line = -1;
    i4	    last_line = 0;
    i4	    i;

    /*  Compute a value for page_next_line and it's offset. */

    for (min_offset = page_size, i = 0; i <= DM_TIDEOF; i++)
    {
	i4	offset;
	
	offset = dmppn_v2_get_offset_macro(lp, i);
	if (offset)
	{
	    if (offset < (DMPPN_V2_OVERHEAD - DM_V2_SIZEOF_LINEID_MACRO) ||
		offset > page_size)
	    {
		break;
	    }
	    if (offset == min_offset)
	    {
		page_next_line = i;
		break;
	    }
	    else if (offset < min_offset)
		min_offset = offset, min_offset_line = i;
	    else
		last_line = i;
	}
    }

    /*	Check that there is agreement on the page next line value. */

    if (page_next_line < 0 || 
	(page_next_line != DMPP_V2_GET_PAGE_NEXT_LINE_MACRO(page)))
    {
	if ((dm1u->dm1u_op & DM1U_OPMASK) == DM1U_VERIFY)
	    return (E_DB_ERROR);

	/*
	**  Use calculated page_next_line.  This corrects for page_next_line
	**  being out of date because of partial disk transfer.  Otherwise
	**  toss the contents of the page.
	*/

	if ((dm1u->dm1u_op & DM1U_OPMASK) == DM1U_PATCH)
	{
	    if (page_next_line >= 0)
	    {
		/*  Use viable page_next_line. */

		DMPP_V2_SET_PAGE_NEXT_LINE_MACRO(page, page_next_line);
		return (E_DB_WARN);
	    }
	    else if (i <= DM_TIDEOF && last_line)
	    {
		for (i = 0; i < last_line; i++)
		    if (dmppn_v2_get_offset_macro(lp, i) ==
			dmppn_v2_get_offset_macro(lp, last_line))
		    {
			/*  Use viable duplicate last line. */
			DMPP_V2_SET_PAGE_NEXT_LINE_MACRO(page, last_line);
			return (E_DB_WARN);
		    }
	    }
	}

	return (E_DB_ERROR);
    }

   return (E_DB_OK);
}

/*{
** Name: dmpp_delete - Deletes a record from a page.
**
** Description:
**    This accessor deletes the record specified by tid from a 
**    page.  It is assumed that the tid is valid and that
**    this record has not already been deleted by this
**    transaction.
**
** Inputs:
**       update_mode                    update mode
**       tran_id                        Transaction Id.
**       tid                            Pointer to the TID for record to
**                                      delete.
**       p                              Pointer to the page.
**       record_size                    Value indicating size of record 
**                                      in bytes.
**
** Outputs:
**      
**	Returns:
**
**	    E_DB_OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**	06-may-1996 (nanpr01 & thaju02)
**	    Created for New Page Format Project:
**	    Change all page header access as macro for New Page Format project.
**	    Pass page_size to dmppn_get_offset_macro, dmppn_put_offset_macro,
**	    for tuple header implementation. Pass page_size and page pointer
**	    to dmppn_set_new_macro.
**          Account for tuple header.
**	12-may-1996 (thaju02)
**	    Clear flags field in line entry for deleted record.  Added 
**	    dmppn_clear_flags_macro.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Delete accessor: added reclaim_space 
**          Can't reclaim space until commit, Copy tran_id into tuple header
**      27-feb-1997 (stial01)
**          dm1cnv2_delete() Set DMPP_CLEAN in page status when we mark tuple
**          deleted, Clear DMPP_CLEAN when no more deleted tuples on page.
**      07-jul-98 (stial01)
**          dm1cnv2_delete() Deferred update processing for V2 pages, use page 
**          tranid when page locking, tuple header tranid when row locking
*/
VOID	    
dm1cnv2_delete(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE	*page,
			i4		update_mode,
			i4		reclaim_space,
			DB_TRAN_ID	*tran_id,
			u_i2		lg_id,
			DM_TID		*tid,
			i4		record_size)
{
    DMPP_PAGE	*p  = page;
    DM_LINEID	*lp = DMPPN_V2_PAGE_LINE_TAB_MACRO(page);
    i4		next_line = DMPP_V2_GET_PAGE_NEXT_LINE_MACRO(p);
    i4		offset;
    i4		i;
    char	*tup_hdr;
    i4		del_cnt;

    /* If its not our tran, place ours there with invalid sequence 0 */
    if ( tran_id )
    {
	if ((DMPP_V2_GET_TRAN_ID_HIGH_MACRO(p) != tran_id->db_high_tran)
	   || (DMPP_V2_GET_TRAN_ID_LOW_MACRO(p) != tran_id->db_low_tran))
	{
	    DMPP_V2_SET_PAGE_TRAN_ID_MACRO(p, *tran_id);
	    DMPP_V2_SET_PAGE_SEQNO_MACRO(p, 0);
	    DMPP_V2_SET_PAGE_LG_ID_MACRO(p, lg_id);
	}
    }

    record_size += Dmpp_pgtype[page_type].dmpp_tuphdr_size;
    offset = dmppn_v2_get_offset_macro(lp, (i4)tid->tid_tid.tid_line);

    if (reclaim_space)
    {
	char          *temp;
	char          *endpt;
	i4       end_offset, ith_offset;

	/* consistency check, reclaim space must be false if row locking */
	if (update_mode & DM1C_ROWLK)
	    dmd_check(E_DMF026_DMPP_PAGE_INCONS);

	if ((temp = dm0m_pgalloc(page_size)) == 0)
	    return;

	end_offset = dmppn_v2_get_offset_macro(lp, (i4)next_line);

	endpt = (char *) p + end_offset;

	/* Update affected line numbers. */

	for (i = next_line, i++, del_cnt = 0; --i >= 0; )
	{
	    ith_offset = dmppn_v2_get_offset_macro(lp, (i4)i);

	    if (i < next_line && i != tid->tid_tid.tid_line && ith_offset != 0) 
	    {
		if (dmppn_v2_test_free_macro(lp, (i4)i) )
		    del_cnt++;
	    }

	    if (ith_offset <= (u_i4)offset && ith_offset != 0)
	    {
		ith_offset += record_size;
		dmppn_v2_put_offset_macro(lp, (i4)i, ith_offset);
	    }
	}
	dmppn_v2_put_offset_macro(lp, (i4)tid->tid_tid.tid_line, 0);

	dmppn_v2_clear_flags_macro(lp, (i4)tid->tid_tid.tid_line);

	/*
	** If we have deleted a tuple from the middle of the data 
	** portion of the page then compact the page.
	**
	** Checking that 'i' is > 0 rather than just non-zero adds the 
	** extra safety of not trashing the page if this routine happens   
	** to be called with a TID that specifies a non-existent tuple.
	*/
	if ((i = (char *)p + offset - endpt) > 0)
	{
	    /* 
	    ** Since the source and the destination overlap we move the 
	    ** source somewhere else first.
	    */

	    MEcopy(endpt, i, temp);
	    MEcopy(temp, i, endpt + record_size);
	}
	
	/*
	** INGSRV 2569
	** Rollback compressed data may need to trim the line table
	*/
	if ((update_mode & DM1C_DMVE_COMP) &&
		tid->tid_tid.tid_line == next_line - 1)
	{
	    i4		next_offset;

	    next_offset = dmppn_v2_get_offset_macro(lp, next_line);
	    DMPP_V2_SET_PAGE_NEXT_LINE_MACRO(page, tid->tid_tid.tid_line);
	    dmppn_v2_put_offset_macro(lp, (i4)tid->tid_tid.tid_line, next_offset);
	    dmppn_v2_clear_flags_macro(lp, (i4)tid->tid_tid.tid_line);
	}

	dm0m_pgdealloc(&temp);

	/* If no more deleted tuples clear DMPP_CLEAN */
	if (del_cnt == 0)
	    DMPP_V2_CLR_PAGE_STAT_MACRO(p, DMPP_CLEAN);

        DMPP_V2_SET_PAGE_STAT_MACRO(p, DMPP_MODIFY);
    }
    else
    {
	/* Set the free space bit */
	dmppn_v2_set_free_macro(lp, (i4)tid->tid_tid.tid_line);

	tup_hdr = ((char *)page + offset);

	/*
	** This macro clears only fields that are in-use, an optimization for
	** V2 pages
	*/
	DMPP_VPT_DELETE_CLEAR_HDR_MACRO(page_type, tup_hdr);
	
	if (update_mode & DM1C_ROWLK)
	{
	    /*
	    ** delete: Put low tran into tuple header when row/phys lock
	    **
	    ** ROW LOCKING DUPLICATE CHECKING PROTOCOL (dm1cnv2_delete)
	    ** Dup checking skips committed deleted rows
	    ** Row locking deletes put lowtran in tuphdr (dm1cnv2_delete)
	    **
	    ** ROW LOCKING SPACE RESERVATION PROTOCOL (dm1cnv2_delete)
	    ** Allocate does not reclaim space until delete is committed
	    ** Row locking deletes put lowtran in tuphdr (dm1cnv2_delete)
	    */
	    MECOPY_CONST_4_MACRO(&tran_id->db_low_tran,
		(PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_lowtran_offset);

	    if ( Dmpp_pgtype[page_type].dmpp_has_lgid )
	    {
		MECOPY_CONST_2_MACRO(&lg_id,
		 (PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_lgid_offset);
	    }
	}
	/* else hdr low tran cleared by DMPP_VPT_DELETE_CLEAR_HDR_MACRO */

	/* Indicate page needs to be cleaned */
	DMPP_V2_SET_PAGE_STAT_MACRO(p, DMPP_CLEAN | DMPP_MODIFY);
    }
}

/*{
** Name: dmpp_format - Formats a page.
**
** Description:
**	This accessor formats a page for a table. Page may be
**	zero filled.
**
**  	Note that this routine is also used by the system catalog 
**	page level accessors.
**
** Inputs:
**       page                           Pointer to the page.
**       pageno                         Number of page to format.
**       stat				Value to be set into page stat field
**	 page_size			Page size
**	 fill_option			DM1C_ZERO_FILL if page needs
**					to be zero-filled,
**					DM1C_ZERO_FILLED if caller
**					assures that it need not be.
**
** Outputs:
**      
**	Returns:
**
**	    E_DB_OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**	06-may-1996 (nanpr01 & thaju02)
**	    Created for New Page Format Project:
**	    Change all page header access as macro for New Page Format project.
**	    Pass page_size arg to dmppn_put_offset_macro for tuple
**	    header implementation.
**	12-may-1996 (thaju02)
**	    Clear flags field in line table entry for first entry.
**	    Added dmppn_clear_flags_macro.
**      20-jun-1996 (thaju02)
**          Use dm0m_lfill() when filling page with page size greater
**          than max value for u_i2.
**	23-Jun-2000 (jenjo02)
**	    Added fill_option to dmpp_format() prototype, set by caller
**	    to DM1C_ZERO_FILL if dmpp_format should MEfill the page,
**	    DM1C_ZERO_FILLED if the page has already been MEfilled.
**	    The page overhead space is always zero-filled.
*/
/*
** "Traditional" page format implementation:
*/
VOID	    
dm1cnv2_format(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE	*page,
			DM_PAGENO       pageno,
			i4		stat,
			i4		fill_option)

{
    DM_LINEID	    *lp = DMPPN_V2_PAGE_LINE_TAB_MACRO(page);

    if ( fill_option == DM1C_ZERO_FILL )
    {
	MEfill(page_size, '\0',  (char * ) page);
    }
    else
        MEfill(DMPPN_V2_OVERHEAD, '\0',  (char * ) page);

    dmppn_v2_put_offset_macro(lp, (i4)0, page_size);
    dmppn_v2_clear_flags_macro(lp, (i4)0);
    DMPP_V2_SET_PAGE_PAGE_MACRO(page, pageno);
    DMPP_V2_SET_PAGE_STAT_MACRO(page, stat);
    DMPP_V2_SET_PAGE_SZTYPE_MACRO(page_type, page_size, page);
}

/*{
** Name: dmpp_get - Gets a record from a page.
**
** Description:
**    This accessor gets the record specified by tid from a 
**    page.  It is assumed that the tid is valid.
**
**    A pointer is returned that points to the requested tuple.  
**
** Inputs:
**      atts_array                      Pointer to an array containing 
**                                      attribute descriptors.
**      atts_count                      Count of number of attributes in array.
**      page                            Pointer to the page.
**      tid                             Pointer to the TID for record to 
**					delete.
**      record_size                     Not used.
**
** Outputs:
**       record                         Pointer to an area for returned pointer
**					to the record.
**
**	Returns:
**
**	    E_DB_OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**	06-may-1996 (nanpr01 & thaju02)
**	    Created for New Page Format Project:
**	    Change all page header access as macro for New Page Format project.
**          Pass page_size to dmppn_get_offset_macro for tuple header
**          implementation.
**          Account for tuple header.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          dm1cnv2_get: Return tran_id of deleted tuples
**      27-feb-97 (stial01)
**          dm1cnv2_get() Init *record = NULL if tid points to empty line table
**      07-jul-98 (stial01)
**          dm1cnv2_get() Always return tranid of tuple
**      09-feb-99 (stial01)
**          dm1cnv2_get() No more tuple headers on DMPP_DIRECT pages
*/
DB_STATUS    
dm1cnv2_get(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE	*page,
			DM_TID     	*tid,
			i4		*record_size,
			char       	**record,
			i4		*row_version,
			u_i4		*row_low_tran,
			u_i2		*row_lg_id,
			DMPP_SEG_HDR	*seg_hdr)
{
    i4	offset = 1;	/*Anything non-zero to show whether set */
    DB_STATUS	status;
    DM_LINEID	*lp = DMPPN_V2_PAGE_LINE_TAB_MACRO(page);
    i4		next_line = DMPP_V2_GET_PAGE_NEXT_LINE_MACRO(page);
    char	*tup_hdr;

    /*	Check for bad line entry. */
    if ( tid->tid_tid.tid_line >= (u_i2)next_line)
	return (E_DB_ERROR);    /* TID out of range */

    offset = dmppn_v2_get_offset_macro(lp, (i4)tid->tid_tid.tid_line);

    if (offset == 0)
    {
	*record = NULL;         /* To distinguish from tid 'marked' deleted */
	return (E_DB_WARN);     /* TID points to empty line table entry */
    }

    tup_hdr = ((char *)page + offset);

    if ((DMPP_V2_GET_PAGE_STAT_MACRO(page) & DMPP_DIRECT))
    {
	/* Always return record pointer */
	*record = (char *)page + offset;
    }
    else
    {
	/* Always return record pointer */
	*record = (char *)page + offset + 
			Dmpp_pgtype[page_type].dmpp_tuphdr_size;

	if ( row_version )
	{
	    u_i2	ver = 0;
	    if ( Dmpp_pgtype[page_type].dmpp_has_versions )
	    {
		MECOPY_CONST_2_MACRO(
		    (PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_vernum_offset, 
		    	&ver);
	    }
	    *row_version = ver;
	}

	if (seg_hdr && Dmpp_pgtype[page_type].dmpp_has_seghdr)
	{
	    MEcopy((PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_seghdr_offset,
			sizeof(DMPP_SEG_HDR), (char *)seg_hdr);
	}
    }

    /* Always return row low tranid when requested */
    if (row_low_tran)
    {
	MECOPY_CONST_4_MACRO(
	    (PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_lowtran_offset,
	    row_low_tran);
    }

    /* Always return row lg_id when requested */
    if ( row_lg_id )
    {
	if ( Dmpp_pgtype[page_type].dmpp_has_lgid )
	{
	    MECOPY_CONST_2_MACRO(
	     (PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_lgid_offset,
		     row_lg_id);
	}
	else
	    *row_lg_id = 0;
    }

    if (dmppn_v2_test_free_macro(lp, (i4)tid->tid_tid.tid_line))
	return (E_DB_WARN);     /* TID points to deleted record */
	
    return (E_DB_OK);

}

/*{
** Name: dmpp_getfree - Return amount of free space on a page.
**
** Description:
**    Given a page, this accessor returns the number of free bytes
**    (available for tuple storage) on the page.
**
** Inputs:
**      page                      	Pointer to the page.
**	page_size			Page size
**
** Returns:
**	Number of free bytes. 
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**	06-may-1996 (nanpr01 & thaju02)
**	    Created for New Page Format Project:
**	    Change all page header access as macro for New Page Format project.
**          Pass page_size to dmppn_get_offset_macro for tuple header
**          implementation.
*/
/*
** "Traditional" page format implementation:
*/
i4	    
dm1cnv2_getfree(
			DMPP_PAGE	*page,
			i4		page_size)
{
    i4  nextline;
    DM_LINEID	*lp = DMPPN_V2_PAGE_LINE_TAB_MACRO(page);
    i4	offset;

    if (page == (DMPP_PAGE *) NULL)
	/* Return the standard free space on an empty page */
	return(page_size - DMPPN_V2_OVERHEAD);

    /* OK, we have a real page to work with */

    nextline = DMPP_V2_GET_PAGE_NEXT_LINE_MACRO(page);
    if (nextline >= DM_TIDEOF)
	return 0;

    offset = dmppn_v2_get_offset_macro(lp, nextline);

    return (offset - (nextline * DM_V2_SIZEOF_LINEID_MACRO) - 
	  DMPPN_V2_OVERHEAD - DM_V2_SIZEOF_LINEID_MACRO);
}

/*{
** Name: dmpp_get_offset - Return offset of data within page.
**
** Description:
**		This accessor returns the offset of data within the page for 
** 		a specified line table entry.
**
** Inputs:
**		page --  Address of the page
**              item --  index within line table.
**
** Outputs:
**              None
**
** Returns:
** 		The offset of the data within the page.
**
** Exceptions:
**		None
**
** Side effects:
**		None
**
** History:
**	06-may-1996 (nanpr01 & thaju02)
**          Created for New Page Format Project:
**	    Change all page header access as macro for New Page Format project.
**          Pass page_size to dmppn_get_offset_macro for tuple header
**          implementation.
**          Account for tuple header.
*/
/* 
** "Traditional" page format implementation
*/
i4	    
dm1cnv2_get_offset(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE	*page,
			i4		item)
{
    DM_LINEID	*lp = DMPPN_V2_PAGE_LINE_TAB_MACRO(page);
    i4	offset;

    offset = dmppn_v2_get_offset_macro(lp, item) + 
				Dmpp_pgtype[page_type].dmpp_tuphdr_size;

    return( offset );
}

/*{
** Name: dmpp_isnew - Was this tuple modified by this cursor?
**
** Description:
**	Given a page and line table number, this accessor returns a boolean
**	flag stating whether the tuple was modified by the current cursor
**	or statement.  The tuple need not exist any longer; that is, it
**	could have been deleted.
**
**  	Note that this routine is also used by the system catalog 
**	page level accessors.
**
** Inputs:
**      r				RCB containing tranid stored if
** 					deferred update, and sequence number
**					associated with this cursor
**      page                      	Pointer to the page.
**	line_num			Line number of the tuple on the page.
**
** Returns:
**	Whether the tuple was modified by the current statement.
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**	06-may-1996 (nanpr01 & thaju02)
**          Created for New Page Format Project:
**	    Change all page header access as macro for New Page Format project.
**	    Pass page_size and page pointer args to dmppn_test_new_macro
**	    due to tuple header implementation.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          New bit not used anymore, just look at sequence_num in tuple hdr
**          
*/
i4	    
dm1cnv2_isnew(
		DMP_RCB		*r,
		DMPP_PAGE	*page,
		i4		line_num)
{
    DMP_TCB	*t = r->rcb_tcb_ptr;
    DM_TID	get_tid;
    DM_LINEID   *lp = DMPPN_V2_PAGE_LINE_TAB_MACRO(page);
    i4		offset;
    char	*tup_hdr;
    u_i4	low_tran;
    u_i4	high_tran;
    i4		seq_num;
    DMP_RCB	*curr;
    i4		i;
    DM_TID	*tidp;
    i4		page_type = t->tcb_rel.relpgtype;
    i4		isnew;
    bool	check_newtids = FALSE;

    /*
    ** FIX ME can't do this optimization because it doesn't work for
    ** cross table updates which can call dm1cnv2_dput from dm2r_replace
    ** for no-change updates. In this case no update, NO LOGGING is done
    ** but deferred processing is done.
    ** Before this optimization can be done, we need to pass the DMR_XTABLE
    ** flag down from dmr_get to dm2r_get to dm1r/dm1b get, down to the isnew
    ** accessor (See bug scripts for b103961)
    ** (unless we always call defer_add_new for XTABLE case in dm2r_replace)
    if (r->rcb_new_fullscan == FALSE && r->rcb_logging
	&& !LSN_GTE(DMPP_VPT_ADDR_PAGE_LOG_ADDR_MACRO(
	    t->tcb_rel.relpgtype, page), &r->rcb_oldest_lsn))
	return (FALSE);
    */

    if (Dmpp_pgtype[page_type].dmpp_has_rowseq)
    {
	offset = dmppn_v2_get_offset_macro(lp, (i4)line_num);
	tup_hdr = ((char *)page + offset);
	/* Page tuple header may not be aligned! Extract tran,seq# */
	MECOPY_CONST_4_MACRO(
	    (PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_lowtran_offset,
	    &low_tran);
	MECOPY_CONST_4_MACRO(
	    (PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_seqnum_offset,
	    &seq_num);

	if (low_tran == r->rcb_tran_id.db_low_tran 
		&& seq_num == r->rcb_seq_number)
	    return (TRUE);
	check_newtids = r->rcb_new_fullscan;
    }
    else
    {
	/* If new bit, page tranid, and sequence num match, isnew = TRUE */
	if (dmppn_v2_test_new_macro(lp, line_num) &&
		(DMPP_V2_GET_TRAN_ID_HIGH_MACRO(page) == 
					r->rcb_tran_id.db_high_tran) &&
		(DMPP_V2_GET_TRAN_ID_LOW_MACRO(page) == 
					r->rcb_tran_id.db_low_tran) && 
		(DMPP_V2_GET_PAGE_SEQNO_MACRO(page) == r->rcb_seq_number))
	    return (TRUE);

	offset = dmppn_v2_get_offset_macro(lp, (i4)line_num);
	tup_hdr = ((char *)page + offset);
	/* Page tuple header may not be aligned! Extract low tran */
	MECOPY_CONST_4_MACRO(
	  (PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_lowtran_offset,
	  &low_tran);

	if (low_tran == r->rcb_tran_id.db_low_tran || r->rcb_new_fullscan)
	    check_newtids = TRUE;
    }

    if (!check_newtids)
	return (FALSE);

    /* MUST check rcb_new_tids */
    get_tid.tid_tid.tid_line = line_num;
    get_tid.tid_tid.tid_page = DMPP_V2_GET_PAGE_PAGE_MACRO(page);

    curr = r;
    do
    {
       /* check for matching table, sequence number */
       if (curr->rcb_update_mode == RCB_U_DEFERRED
	    && curr->rcb_seq_number == r->rcb_seq_number
	    && curr->rcb_new_cnt)
       {
	    i4	left, right, middle;
	    DM_TID  *midtidp;

	    left = 0;
	    right = curr->rcb_new_cnt - 1;
	    
	    while (left <= right)
	    {
		middle = (left + right)/2;

		midtidp = curr->rcb_new_tids + middle;
		if (get_tid.tid_i4 == midtidp->tid_i4)
		    return (TRUE);
		else if (get_tid.tid_i4 < midtidp->tid_i4)
		    right = middle - 1;
		else
		    left = middle + 1;
	    }
       }
    } while ((curr = curr->rcb_rl_next) != r);

    return (FALSE);
}

/*{
** Name: dmpp_ixsearch - Binary search an ISAM index page
**
** Description:
**	Given a page and a (possibly partial) key, this accessor
**	binary searches the page for a matching tuple.  The tuple
**	must consist only of keys, in key order; that is the case
**	for ISAM index pages.
**
** Inputs:
**      page			Pointer to the page.
**	keyatts			Vector of key attribute descriptors
**	keys_given		Number of key attributes provided
**	key			Key to search for
**	partialkey		Flag:  is the key complete or not?
**	direction		Scanning forward or backward
**      relcmptlvl              DMF version of table
**
** Returns:
**	Line number of the qualifying tuple.
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**	06-may-1996 (nanpr01 & thaju02)
**          Created for New Page Format Project:
**	    Change all page header access as macro for New Page Format project.
**          Pass page_size to dmppn_get_offset_macro for tuple header
**          implementation.
**          Account for tuple header in pointer to tuple data record passed
**          to adt_kkcmp().
**      09-feb-1999 (stial01)
**          dm1cnv2_ixsearch() Check relcmptlvl to see if tuphdr on index pages 
*/
/*
** "Traditional" page format implementation:
*/
i4	    
dm1cnv2_ixsearch(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE	*page,
			DB_ATTS		**keyatts,
			i4		keys_given,
			char		*key,
			bool		partialkey,
			i4		direction,
			i4              relcmptlvl,
			ADF_CB 		*adf_cb)
{
    i4  bottom, mid, top;
    i4  compare;
    i4	offset;
    DB_STATUS s;
    DM_LINEID	*lp = DMPPN_V2_PAGE_LINE_TAB_MACRO(page);
    bool        have_tuphdr;

    /*
    ** Ingres 2.0 mistakenly included tuple headers on ISAM index entries
    ** Another way to determine if DMPP_DIRECT page has tuple headers is to 
    ** compare
    **     pagekeylen = page_size - dmppn_v2_get_offset_macro(lp, 0)
    ** to
    **     keylen + Dmpp_pgtype[page_type].dmpp_tuphdr_size 
    */
    if (relcmptlvl == DMF_T4_VERSION)
	have_tuphdr = TRUE;
    else
	have_tuphdr = FALSE;

    for (bottom = 0, top = (DMPP_V2_GET_PAGE_NEXT_LINE_MACRO(page) - 1);
	 top != bottom;)
    {
	mid = ((1 + top - bottom) >> 1) + bottom;
	offset = dmppn_v2_get_offset_macro(lp, mid);
	if (have_tuphdr)
	    offset += Dmpp_pgtype[page_type].dmpp_tuphdr_size;
	s = adt_kkcmp(adf_cb, keys_given, keyatts, key, 
			(char *) page + offset, &compare);
	if (s != E_DB_OK)
	    dmd_check(E_DMF012_ADT_FAIL);

	/* If key is below directory, then we are in the bottom half. */

	if (compare < 0 ||
	    (compare == 0 && partialkey && direction == DM1C_PREV))
	{
	    top = mid - 1;
	}
	else
	{
	    bottom = mid;
	    if (compare == 0 && !partialkey)
		break;
	}
    }
    return bottom;
}

/*{
** Name: dmpp_load - Allocates and put record on data page.
**
** Description:
**    This routine allocates space for a record on a data page.
**    It then put the record onto the page.  If there is no space
**    an error is returned.  This routine can be used by any
**    place in the code that doesn't need to know the tid before
**    the record is placed on the page, and the page has no
**    unallocated tids.
**
**    Note that this routine is also used by the system catalog 
**    page level accessors.
**
** Inputs:
**       data                           Pointer to the page to allocate from.
**	 record				Pointer to record.
**       record_size                    Value indicating amount of space
**                                      needed.
**	 record_type			DM1C_LOAD_xxx record type to indicate
**					whether an ISAM index record or not.
**	 fill				Number of bytes to allocate on page.
**					Use all of page if 0.
**
** Outputs:
**	 tid				Pointer to tid for record. If not NULL.
**	Returns:
**
**	    E_DB_OK
**	    E_DB_WARN			No space on page.
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**	06-may-1996 (nanpr01 & thaju02)
**          Created for New Page Format Project:
**	    Change all page header access as macro for New Page Format project.
**          Pass page_size to dmppn_get_offset_macro, dmppn_put_offset_macro
**	    for tuple header implementation.
**          Account for tuple header.
**	12-may-1996 (thaju02)
**	    Clear flags field in line table entry. Added 
**	    dmppn_clear_flags_macro.
**	20-may-1996 (ramra01)
**	    Added new param tup_info to load accessor routine
**	17-jul-1996 (ramra01)
**	    Store version number in the tuple header as part of the
**	    Alter Table Project. 
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          load accessor: changed DMPP_TUPLE_INFO param to table_version
**      09-feb-1999 (stial01)
**          dm1cnv2_load() No more tuple headers on DMPP_DIRECT pages  
**      10-feb-99 (stial01)
**          dm1cnv2_load() Do not limit # entries on V2 ISAM INDEX pages
**	22-Feb-2008 (kschendel) SIR 122739
**	    record type is a DM1C flag, not a compression type.
*/
DB_STATUS    
dm1cnv2_load(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE       *data,
			char		*record,
			i4         	record_size,
			i4		record_type,
			i4		fill,
			DM_TID		*tid,
			u_i2		current_table_version,
			DMPP_SEG_HDR	*seg_hdr)
{
    DM_LINEID	*lp = DMPPN_V2_PAGE_LINE_TAB_MACRO(data);
    i4     nextlno = DMPP_V2_GET_PAGE_NEXT_LINE_MACRO(data);
    i4	offset;
    i4	free;
    char	*rec_location;
    char	*tup_hdr;
    i4	orig_rec_size = record_size;
    
    if (record_type != DM1C_LOAD_ISAMINDEX)
	record_size += Dmpp_pgtype[page_type].dmpp_tuphdr_size;

    offset = dmppn_v2_get_offset_macro(lp, (i4) nextlno);

    free = offset   - (nextlno * DM_V2_SIZEOF_LINEID_MACRO)
		    - (DMPPN_V2_OVERHEAD + DM_V2_SIZEOF_LINEID_MACRO);

    /*	Check for available space. */

    if (free < record_size ||
	(fill && nextlno && free < fill))
    {
	return (E_DB_WARN);
    }

    /*
    ** Limit the number of entries on DATA pages
    ** Do not limit # entries on V2 ISAM INDEX pages
    */
    if ((nextlno >= DM_TIDEOF) && (record_type != DM1C_LOAD_ISAMINDEX))
    {
	return (E_DB_WARN);
    }

    /*	Add record to end of line table. */
    
    offset -= record_size;
    rec_location = (char * )data + offset;

    dmppn_v2_put_offset_macro(lp, (i4)nextlno, offset);
    dmppn_v2_clear_flags_macro(lp, (i4)nextlno);
    dmppn_v2_put_offset_macro(lp, (i4)(nextlno + 1), offset);
    dmppn_v2_clear_flags_macro(lp, (i4)(nextlno + 1));
    DMPP_V2_INCR_PAGE_NEXT_LINE_MACRO(data);
    DMPP_V2_SET_PAGE_STAT_MACRO(data, DMPP_MODIFY);
    if (tid)
    {
	tid->tid_tid.tid_page = DMPP_V2_GET_PAGE_PAGE_MACRO(data);
	tid->tid_tid.tid_line = nextlno;
    }

    if (record_type != DM1C_LOAD_ISAMINDEX)
    {
	tup_hdr = rec_location;
	DMPP_VPT_CLEAR_HDR_MACRO(page_type, tup_hdr);
	if (Dmpp_pgtype[page_type].dmpp_has_versions && current_table_version)
	    MECOPY_CONST_2_MACRO(&current_table_version,
		(PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_vernum_offset);

	if (Dmpp_pgtype[page_type].dmpp_has_seghdr && seg_hdr)
	{
	    MEcopy((char *)seg_hdr, sizeof(DMPP_SEG_HDR),
		    (PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_seghdr_offset);
	}

	rec_location += Dmpp_pgtype[page_type].dmpp_tuphdr_size;
    }

    MEcopy(record, orig_rec_size, rec_location);

    return (E_DB_OK);
}

/*{
** Name: dmpp_put - Puts a record on a page.
**
** Description:
**    This accessor puts the compressed record specified by tid on a 
**    page.  It is assumed that the tid is valid and has not
**    already been placed here by same transaction.
**
** Inputs:
**	 update_mode			update mode
**					DM1C_DIRECT or DM1C_DEFERRED may be set,
** 					however deferred processing is now
**					done with a separate call to dmpp_put
** 	 tran_id			Transaction ID stored if deferred
**					update.
**       tid                            Pointer to the TID for record to
**                                      put.
**       p                              Pointer to the page.
**       record_size                    Value indicating size of record in 
**                                      bytes.
**       record                         Pointer to record.
**
** Outputs:
**      
**	Returns:
**
**	    E_DB_OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**	06-may-1996 (nanpr01 & thaju02)
**          Created for New Page Format Project:
**	    Change all page header access as macro for New Page Format project.
**          Pass page_size to dmppn_get_offset_macro, dmppn_put_offset_macro
**	    for tuple header implementation. Pass page_size and page pointer
**	    to dmppn_set_new_macro.
**          Account for tuple header in record.
**	12-may-1996 (thaju02)
**	    Clear flags field for put record's line table entry. Added
**	    dmppn_clear_flags_macro. 
**	17-jul-1996 (ramra01)
**	    Alter Table project: store the ver number in tup header.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Put accessor: changed DMPP_TUPLE_INFO param to table_version
**          dm1cnv2_put() Copy tran_id into tuple hdr
**      27-feb-97 (stial01)
**          If put and tid allocated, just clear flags, space is reclaimed
**          only when the tuple size is the same.
**      07-jul-98 (stial01)
**          dm1cnv2_put() Deferred update processing for V2 pages, use page 
**          tranid when page locking, tuple header tranid when row locking
**      15-apr-99 (stial01)
**          dm1cnv2_put() clear entire tuple header (lost on 07-07-98)
**	22-Feb-2008 (kschendel) SIR 122739
**	    Remove unused record type param.
*/
VOID	    
dm1cnv2_put(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE	*page,
			i4		update_mode,
			DB_TRAN_ID  	*tran_id,
			u_i2		lg_id,
			DM_TID	    	*tid,
			i4	    	record_size,
			char	    	*record,
			u_i2		current_table_version,
			DMPP_SEG_HDR	*seg_hdr)
{
    DMPP_PAGE		*p = page;
    DM_LINEID		*lp = DMPPN_V2_PAGE_LINE_TAB_MACRO(page);
    i4		next_line = DMPP_V2_GET_PAGE_NEXT_LINE_MACRO(p);
    char		*record_location;
    i4		offset;
    i4		orig_rec_size = record_size;
    char		*tup_hdr;
    bool                replace = FALSE; /* in place replace ? */
    DB_STATUS           status;

    /* If its not our tran, place ours there with invalid sequence 0 */
    if ( tran_id )
    {
	if ((DMPP_V2_GET_TRAN_ID_HIGH_MACRO(p) != tran_id->db_high_tran)
	   || (DMPP_V2_GET_TRAN_ID_LOW_MACRO(p) != tran_id->db_low_tran))
	{
	    DMPP_V2_SET_PAGE_TRAN_ID_MACRO(p, *tran_id);
	    DMPP_V2_SET_PAGE_SEQNO_MACRO(p, 0);
	    DMPP_V2_SET_PAGE_LG_ID_MACRO(p, lg_id);
	}
    }
					
    record_size += Dmpp_pgtype[page_type].dmpp_tuphdr_size;

    /*
    ** If insert is to beyond where the current page_next_line value is
    ** stored, then move the next_line value and pointer up to one position
    ** past where the new row is being insert.
    */
    if ((i4)tid->tid_tid.tid_line >= next_line)
    {
	/*
	** Tuple being added at new line table entry.
	**
	** Update the page_next_line pointer and its offset.
	*/
	offset = dmppn_v2_get_offset_macro(lp, (i4)next_line);

	offset -= record_size;
	record_location = (char *)p + offset;

	DMPP_V2_SET_PAGE_NEXT_LINE_MACRO(p, tid->tid_tid.tid_line + 1);
	dmppn_v2_put_offset_macro(lp, 
		(i4)DMPP_V2_GET_PAGE_NEXT_LINE_MACRO(p), offset);
	dmppn_v2_clear_flags_macro(lp, 
		(i4)DMPP_V2_GET_PAGE_NEXT_LINE_MACRO(p)); 

	/*
	** Initialize the offset to the new entry.
	*/
	dmppn_v2_put_offset_macro(lp, (i4)tid->tid_tid.tid_line, offset);
	dmppn_v2_clear_flags_macro(lp, (i4)tid->tid_tid.tid_line);

	/*
	** If the old page_next_line slot was not the same spot as the one 
	** just allocated, then we have to zero out that old slot.
	*/
	if (next_line != tid->tid_tid.tid_line)
	{
	    dmppn_v2_put_offset_macro(lp, (i4)next_line, 0);
	    dmppn_v2_clear_flags_macro(lp, (i4)next_line);
	}

    }
    else if (dmppn_v2_get_offset_macro(lp, tid->tid_tid.tid_line) == 0)
    {
	/*  Tuple using currently empty line table entry. */

	offset = dmppn_v2_get_offset_macro(lp, (i4)next_line);

	offset -= record_size;
	record_location = (char *)p + offset;

	dmppn_v2_put_offset_macro(lp, (i4)next_line, offset);
	dmppn_v2_put_offset_macro(lp, (i4) tid->tid_tid.tid_line, offset);
	dmppn_v2_clear_flags_macro(lp, (i4)tid->tid_tid.tid_line);
    }
    else
    {
	/*
	** In-place replace or reclaiming space from deleted record
	**
	** In both cases the old record and new record should have already
	** been determined to be the same size
	*/
	if (dmppn_v2_test_free_macro(lp, tid->tid_tid.tid_line))
	{
	    dmppn_v2_clear_flags_macro(lp, (i4)tid->tid_tid.tid_line);
	}
	else
	    replace = TRUE;

	offset = dmppn_v2_get_offset_macro(lp, (i4)tid->tid_tid.tid_line);
	record_location = (char *)p + offset;
    }

    tup_hdr = record_location;
    if (replace)
    {
	u_i4	low_tran;

	/* Extract low tran from tuple header */
	MECOPY_CONST_4_MACRO(
	  (PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_lowtran_offset, &low_tran);
	if (tran_id->db_low_tran != low_tran)
	    DMPP_VPT_CLEAR_HDR_MACRO(page_type, tup_hdr);
    }
    else
    {
	DMPP_VPT_CLEAR_HDR_MACRO(page_type, tup_hdr);
    }

    /* Store table version */
    if (current_table_version && Dmpp_pgtype[page_type].dmpp_has_versions)
	MECOPY_CONST_2_MACRO(&current_table_version,
	  (PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_vernum_offset);

    /*
    ** Row locking: store LOW tranid 
    **
    ** ROW LOCKING DUPLICATE CHECKING PROTOCOL (dm1cnv2_put)
    ** Dup (key) checking on committed rows without locking
    ** Row locking inserts put lowtran in tuphdr (dm1cnv2_put)
    **
    ** ROW LOCKING NON-KEY-QUAL PROTOCOL (dm1cnv2_put)
    ** Non key qualifications are applied to committed data without locking
    ** Row locking inserts put lowtran in tuphdr (dm1cnv2_put)
    */
    if (update_mode & DM1C_ROWLK)
    {
	MECOPY_CONST_4_MACRO(&tran_id->db_low_tran,
	    (PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_lowtran_offset);

	if ( Dmpp_pgtype[page_type].dmpp_has_lgid )
	{
	    MECOPY_CONST_2_MACRO(&lg_id,
		(PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_lgid_offset);
	}
    }

    if (seg_hdr && Dmpp_pgtype[page_type].dmpp_has_seghdr)
    {
	MEcopy((char *)seg_hdr, sizeof(DMPP_SEG_HDR),
		(PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_seghdr_offset);
    }

    record_location += Dmpp_pgtype[page_type].dmpp_tuphdr_size;

    /* Move the tuple. */
    MEcopy(record, orig_rec_size, record_location);

    /* Mark page as dirty */

    DMPP_V2_SET_PAGE_STAT_MACRO(p, DMPP_MODIFY);

}

/*{
** Name: dmpp_reallocate - Reallocate space for a record at a given TID
**
** Description:
**	Verify that there is sufficient space to insert a record of the
**	given size at the specified tid, or in the free space area, so 
**	that we can be assured that dmpp_get will work.  Do not modify the page.
**
**	Note that this routine is also used by the system catalog 
**	page level accessors.
**
** Inputs:
**      page			Pointer to the page on which to allocate.
**	line_num		Line number entry to make valid.
**	reclen			Amount of space to allocate if the
**				line number is not valid.
**
** Side Effects:
**          Space may be allocated on the page.
**
** History:
**	06-may-1996 (nanpr01 & thaju02)
**          Created for New Page Format Project:
**	    Change all page header access as macro for New Page Format project.
**          Pass page_size to dmppn_get_offset_macro for tuple header
**          implementation.
**      23-dec-1999 (stial01)
**          dm1cnv2_relallocate DO NOT add tuple header size to reclen
*/
DB_STATUS 
dm1cnv2_reallocate(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE	*page,
			i4		line_num,
			i4		reclen)
{
    i4 	ret_len;
    i4 	free_space;
    i4	offset;
    i4	nextlno = DMPP_V2_GET_PAGE_NEXT_LINE_MACRO(page);
    DM_LINEID	*lp = DMPPN_V2_PAGE_LINE_TAB_MACRO(page);

    /*
    ** If line number is valid and space has been allocated there, 
    ** check that that space is sufficient.  Note this also works
    ** for system catalog rows.
    ** DO NOT add tuple header size to reclen, because dm1cnv2_reclen
    ** returns entry size - tuple header size
    */
    if ( (line_num < nextlno) &&
         (dmppn_v2_get_offset_macro(lp, line_num) != 0) ) 
    {
	dm1cnv2_reclen(page_type, page_size, page, line_num, &ret_len);
 	if (reclen == ret_len)
	    return(E_DB_OK);

	return(E_DB_WARN);
    }

    /*
    ** Either the offset is zero or the line number points beyond the
    ** end of the page, check for sufficient space in the free space area.
    ** DO add tuple header size when comparing reclen to free space
    */
    reclen += Dmpp_pgtype[page_type].dmpp_tuphdr_size;
    offset = dmppn_v2_get_offset_macro(lp, nextlno);

    free_space = offset	- (nextlno * DM_V2_SIZEOF_LINEID_MACRO) - DMPPN_V2_OVERHEAD;

    if (reclen > free_space)
    {
#ifdef xDEBUG
	TRdisplay(
	"dm1cnv2_reallocate fail: page: %d, lineno %d, reclen: %d, free: %d\n",
	    DMPP_V2_GET_PAGE_PAGE_MACRO(page), line_num, reclen, 
	    free_space);
#endif 
	return(E_DB_WARN);
    }

    return(E_DB_OK);
}

/*{
** Name: dmpp_reclen - Computes the length of a record on a page.
**
** Description:
**    This accessor computes the length of a record specified 
**    by tid on a page.  It is assumed that the tid is valid.
**
**    Note that this routine is also used by the system catalog 
**    page level accessors.
**
** Inputs:
**      tid                             Pointer to the TID for record to
**                                      calculate length.
**       page                           Pointer to the page.
**
** Outputs:
**       record_size                    Pointer to an area to return record
**                                      size.     
**    
**	Returns:
**
**	    E_DB_OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**	06-may-1996 (nanpr01 & thaju02)
**          Created for New Page Format Project:
**	    Change all page header access as macro for New Page Format project.
**          Pass page_size to dmppn_get_offset_macro for tuple header
**          implementation.
*/
/*
** "Traditional" page format:
*/
VOID	    
dm1cnv2_reclen(
		i4		page_type,
		i4		page_size,
		DMPP_PAGE	*page,
		i4		lineno,
		i4		*record_size)
{
    i4	    i,j;
    i4	    lineoff;
    i4	    nextoff;
    i4	    jth_off;
    DM_LINEID	    *lp = DMPPN_V2_PAGE_LINE_TAB_MACRO(page);

    /* Find offset for line number */
    lineoff = dmppn_v2_get_offset_macro(lp, lineno);

    /* Find smallest tuple offset larger than lineoff; if none, use
    ** page_size (end of page).
    */
    nextoff = page_size;
    for (j = 0, i = DMPP_V2_GET_PAGE_NEXT_LINE_MACRO(page); --i >= 0; 
	 j++)
    {
	jth_off = dmppn_v2_get_offset_macro(lp, (i4)j);

        if (jth_off <= (u_i4)lineoff)
            continue;
        if (jth_off < (u_i4)nextoff)
            nextoff = jth_off;
    }
    
    *record_size = (nextoff - lineoff - Dmpp_pgtype[page_type].dmpp_tuphdr_size);
}

/*{
** Name: dmpp_tuplecount - Return number of tuples on a page.
**
** Description:
**    Given a page, this accessor returns the number of tuples currently
**    on the page.
**    WARNING: THIS ROUTINE ASSUMES A PAGE/TABLE LOCK IS HELD
**
** Inputs:
**      page                      	Pointer to the page.
**	page_size			Page size
**
** Outputs:
** 	None      
**
** Returns:
**	Number of tuples on the page.
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**	06-may-1996 (nanpr01 & thaju02)
**          Created for New Page Format Project:
**	    Change all page header access as macro for New Page Format project.
**          Pass page_size to dmppn_get_offset_macro for tuple header
**          implementation.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          dm1cnv2_tuplecount() ignore deleted tuples
**          This routine will only return accurate count if page locked
*/
/*
** "Traditional" page format implementation:
*/
i4	    
dm1cnv2_tuplecount(
			DMPP_PAGE	*page,
			i4		page_size)
{
    register i4  i;
    register i4  count = 0;
    DM_LINEID	*lp = DMPPN_V2_PAGE_LINE_TAB_MACRO(page);
    i4           offset;
    bool	      xid_eq;

    if (DM_V2_GET_LINEID_MACRO(lp, 
		DMPP_V2_GET_PAGE_NEXT_LINE_MACRO(page)) == page_size)
	return(0);

    for (i = 0; i != DMPP_V2_GET_PAGE_NEXT_LINE_MACRO(page); ++i)
    {
	if (dmppn_v2_get_offset_macro(lp, i) != 0)
	{
	    /*
	    ** Ignore tuples marked deleted
	    ** This routine should always be called with a page lock
	    */
	    if (dmppn_v2_test_free_macro(lp, i) == 0)
		count++;
	}
    }
    return(count);
}

/*{
** Name: dmpp_dput - Perform deferred update processing
**
** Description:
**
** 	Deferred update processing. To be called after put or replace
** 	or in the case of cross table updates, this routine is called
**      for no-change updates so that ambiguous replace checking is 
**	correctly performed
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**	page				Pointer to the page
**      tid				Pointer to the TID for record to
**					perform deferred update processing
**
** Outputs:
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
**          none
**
** History:
**	3-july-1997(angusm)
**	    Created.
**     05-oct-1998 (stial01)
**          dmpp_dput: Added update parameter - See 07-jul-98 changes for 
**          deferred updates
*/
DB_STATUS
dm1cnv2_dput(
DMP_RCB		*rcb,
DMPP_PAGE 	*page, 
DM_TID 		*tid,
i4		*err_code)
{
    DMP_TCB	    *t = rcb->rcb_tcb_ptr;
    i4		    page_type = t->tcb_rel.relpgtype;
    DM_LINEID	    *lp = DMPPN_V2_PAGE_LINE_TAB_MACRO(page);
    char	    *tup_hdr;
    DB_STATUS	    status = E_DB_OK;

    /* flag page as dirty */
    DMPP_V2_SET_PAGE_STAT_MACRO(page, DMPP_MODIFY);

    if (Dmpp_pgtype[page_type].dmpp_has_rowseq)
    {
	tup_hdr  = ((char *)page +
		dmppn_v2_get_offset_macro(lp, (i4)tid->tid_tid.tid_line));
	/* put: if deferred, put tranid, sequence number in tuple header */
	MECOPY_CONST_4_MACRO(&rcb->rcb_tran_id.db_low_tran, 
	    (PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_lowtran_offset);

	/* only V2 has high_tran */
	if (page_type == TCB_PG_V2)
	    MECOPY_CONST_4_MACRO(&rcb->rcb_tran_id.db_high_tran, 
		(PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_hightran_offset);

	MECOPY_CONST_4_MACRO(&rcb->rcb_seq_number, 
	    (PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_seqnum_offset);

	if ( Dmpp_pgtype[page_type].dmpp_has_lgid )
	{
	    MECOPY_CONST_2_MACRO(&rcb->rcb_slog_id_id,
		(PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_lgid_offset);
	}
	return (E_DB_OK);
    }
    else
    {
	if (row_locking(rcb) || crow_locking(rcb) || t->tcb_sysrel || t->tcb_extended)
	{
	    tup_hdr  = ((char *)page +
		    dmppn_v2_get_offset_macro(lp, (i4)tid->tid_tid.tid_line));
	    MECOPY_CONST_4_MACRO(&rcb->rcb_tran_id.db_low_tran,
		(PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_lowtran_offset);
	    if ( Dmpp_pgtype[page_type].dmpp_has_lgid )
	    {
		MECOPY_CONST_2_MACRO(&rcb->rcb_slog_id_id,
		    (PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_lgid_offset);
	    }

	    /*
	    ** V3, V4 pages do not keep sequence number per row 
	    ** Remember tid in rcb
	    */
	    status = defer_add_new(rcb, tid, TRUE, err_code);
	}
	else
	{
	    dm1cnv2_clear(rcb, page);
	    dmppn_v2_set_new_macro(lp, tid->tid_tid.tid_line);
	}
    }

    return (status);
}

/*{
** Name: dmpp_clear - Clears the line table and transaction bits.
**
** Description:
**    This accessor clears all the new bits in line and sets
**    the transaction id and sequence number if this is a different transaction.
**
** Inputs:
**       tran_id                        Pointer to structure containing
**                                      transaction id.
**       sequence_number                Value indicating which sequence
**                                      this update is associated with.
**       p				Pointer to page.
**                                      to store retrieved record.
**       lines                          Pointer to line table.
**       length                         A value indicating length of line
**                                      table.
**
** Outputs:
**	Returns:
**
**	    E_DB_OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**	06-may-1996 (nanpr01 & thaju02)
**          Created for New Page Format Project:
**	    Change all page header access as macro for New Page Format project.
**	    Pass page_size and page pointer to dmppn_clear_new_macro.
**      07-jul-98 (stial01)
**          dm1cnv2_clear() Deferred update processing for V2 pages, use page 
**          tranid when page locking, tuple header tranid when row locking
*/
/*
** "Traditional" page format:
*/
static VOID	    
dm1cnv2_clear(
			DMP_RCB		*r,
			DMPP_PAGE       *page)
{
    i4		i; 
    i4		length;
    DM_LINEID   *lp = DMPPN_V2_PAGE_LINE_TAB_MACRO(page);

    /*
    ** debug: We shouldn't do clear operation if page type is TCB_PG_V2,
    ** in which case tuple header tranid and sequence number are used
    */
    if (r->rcb_tcb_ptr->tcb_rel.relpgtype == TCB_PG_V2)
	dmd_check(E_DMF026_DMPP_PAGE_INCONS);

    length = (i4) DMPP_V2_GET_PAGE_NEXT_LINE_MACRO(page);

    /*
    ** Update the page's transaction id and sequence number
    ** If this is the first update by this transaction, clear the new bits
    **
    ** NOTE  dm1cnv2_clear() should not update any part of the tuple header
    ** (See ROW LOCKING DUPLICATE CHECKING and NON-KEY-QUAL protocols
    ** which assume uncommitted records will always have low tran
    ** set in the tuple header by row locking transactions)
    */
    if ((DMPP_V2_GET_TRAN_ID_HIGH_MACRO(page) != r->rcb_tran_id.db_high_tran)
       || (DMPP_V2_GET_TRAN_ID_LOW_MACRO(page) != r->rcb_tran_id.db_low_tran)
       || (DMPP_V2_GET_PAGE_SEQNO_MACRO(page) != r->rcb_seq_number))
    {
	DMPP_V2_SET_PAGE_TRAN_ID_MACRO(page, r->rcb_tran_id);
	DMPP_V2_SET_PAGE_SEQNO_MACRO(page, r->rcb_seq_number);

        for (i = 0; i < length; i++ )
            dmppn_v2_clear_new_macro(lp, (i4)i); 
    }
}

DB_STATUS
defer_add_new(
DMP_RCB *rcb,
DM_TID  *put_tid,
i4	page_updated,	
i4	*err_code)
{
    DMP_RCB             *r = rcb;
    DMP_RCB             *curr;
    i4                  *ip;
    i4                  i;
    DM_TID              *tidp;
    DM_TID              *alloc_tids;
    i4                  alloc_cnt;
    DB_STATUS           status;
    DMP_MISC            *misc;
    i4			mem_needed;
    i4			left, right, middle;
    DM_TID		*itidp, *jtidp, *midtidp, *insert_tidp;
    DB_ERROR		local_dberr;

#ifdef xDEBUG
    TRdisplay("defer_add_new for %~t tid (%d,%d)\n",
    sizeof(r->rcb_tcb_ptr->tcb_rel.relid),  &r->rcb_tcb_ptr->tcb_rel.relid,
    put_tid->tid_tid.tid_page, put_tid->tid_tid.tid_line);
#endif

    /*
    ** Add to all tid list for this table, this sequence number
    ** Note this code adds the tid to rcb_new_tids for all cursors
    ** on this table having the same sequence number.
    ** (although most likely there is only one cursor open on this table)
    */
    curr = r;
    do
    {
       if (curr->rcb_update_mode == RCB_U_DEFERRED
	    && curr->rcb_seq_number == r->rcb_seq_number)
	{
	    /* If the page/row header tran/sequence wasnt updated,
	    ** isnew() routines must always check for tid in rcb_new_tids
	    ** before returning FALSE
	    ** NOTE: rcb_new_fullscan does not mean all deferred update
	    ** tids are in rcb_new_tids, just the ones where the page
	    ** was not updated. (the DM2R_XTABLE case)
	    */
	    if (page_updated == FALSE)
		curr->rcb_new_fullscan = TRUE;

	    /* Add the tid */
	    if (curr->rcb_new_cnt >= curr->rcb_new_max)
	    {
		/*
		** Allocate a bigger tid array - it will only need to go as big
		** as locks per transaction before we will just escalate and 
		** start doing deferred update processing on the page
		*/
		alloc_cnt = curr->rcb_new_cnt + 500;

		mem_needed = sizeof(DMP_MISC) + (alloc_cnt * sizeof (DM_TID));
		/* Get LongTerm memory */
		status = dm0m_allocate(mem_needed,
			    (i4)DM0M_LONGTERM, (i4)MISC_CB, 
			    (i4)MISC_ASCII_ID, (char *)r,
			    (DM_OBJECT **)&misc, 
			    &local_dberr);
#ifdef xDEBUG
		TRdisplay("Alloc tid list for table %d size %d status %d %x\n", 
		    curr->rcb_tcb_ptr->tcb_rel.reltid.db_tab_base, alloc_cnt,
		    status, misc);
#endif
		if (status != E_DB_OK)
		{
		    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
			    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		    uleFormat(NULL, E_DM9425_MEMORY_ALLOCATE, (CL_ERR_DESC *)NULL, ULE_LOG, 
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
			    0, mem_needed);
		    *err_code = E_DM9425_MEMORY_ALLOCATE;
		    return(E_DB_ERROR);
		}
		alloc_tids = (DM_TID *)((char *)misc + sizeof(DMP_MISC));
		misc->misc_data = (char*)alloc_tids;

		/* Copy old tids to new tid array */
		MEcopy((PTR)curr->rcb_new_tids, 
			curr->rcb_new_cnt * sizeof(DM_TID), 
			(PTR)alloc_tids);

		curr->rcb_new_max = alloc_cnt;
		if (curr->rcb_new_tids != &curr->rcb_tmp_tids[0])
		{
		    misc = (DMP_MISC *)((char *)curr->rcb_new_tids - sizeof(DMP_MISC));
		    dm0m_deallocate((DM_OBJECT **)&misc);
		}
		curr->rcb_new_tids = alloc_tids;
	    }

	    /* Add the tid - keep list sorted */
	    if (curr->rcb_new_cnt == 0)
	    {
		tidp = curr->rcb_new_tids;
		tidp->tid_i4 = put_tid->tid_i4;
		curr->rcb_new_cnt = 1;
	    }
	    else
	    {
		left = 0;
		right = curr->rcb_new_cnt - 1;
		
		while (left <= right)
		{
		    middle = (left + right)/2;

		    midtidp = curr->rcb_new_tids + middle;
		    if (put_tid->tid_i4 == midtidp->tid_i4)
			return (E_DB_OK);
		    else if (put_tid->tid_i4 < midtidp->tid_i4)
			right = middle - 1;
		    else
			left = middle + 1;
		}
		
		insert_tidp = curr->rcb_new_tids + left;
		if (left != curr->rcb_new_cnt)
		{
		    jtidp = curr->rcb_new_tids + curr->rcb_new_cnt;
		    itidp = jtidp - 1;

		    for (  ; itidp >= insert_tidp; jtidp--, itidp--)
			jtidp->tid_i4 = itidp->tid_i4;
		}

		insert_tidp->tid_i4 = put_tid->tid_i4;
		curr->rcb_new_cnt++;

	    }
	}
    } while ((curr = curr->rcb_rl_next) != r);

    return(E_DB_OK);
}


/*{
** Name: dmpp_clean - Remove committed deleted rows from a page
**
** Description:
**    This accessor removes committed delete rows from a page
**    It should only be necessary to call this routine when the
**    table has data compression or has been altered.
**    Otherwise, space reclamation can be done in place in dm1cnv2_allocate
**
** Inputs:
**	page_type			Page type
**	page_size			Page size
**      page                      	Pointer to the page.
**      tranid				Transaction Id.
**      lk_type
**
** Outputs:
**      
**	Returns:
**
**	    E_DB_OK
**          avail_space			number of bytes deleted
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**	15-Jan-2010 (stial01)
*/
DB_STATUS	    
dm1cnv2_clean(
DMP_RCB		*r,
DMPP_PAGE	*page,
i4		*avail_space)
{
    i4		page_type = r->rcb_tcb_ptr->tcb_rel.relpgtype;
    i4		page_size = r->rcb_tcb_ptr->tcb_rel.relpgsize;
    DB_TRAN_ID  *tranid = &r->rcb_tran_id;
    i4     i;
    i4     nextlno;
    i4	space;
    DM_LINEID	*lp = DMPPN_V2_PAGE_LINE_TAB_MACRO(page);
    DM_TID    del_tid;
    i4        del_recsize;
    DB_STATUS status;
    char	*tup_hdr;
    u_i4	row_low_tran;
    u_i2	row_lg_id = 0;
    i4		clean_bytes = 0;
    i4		xlock = FALSE; /* FIX ME */

    /* Try to reclaim space */ 
    if ((DMPP_V2_GET_PAGE_STAT_MACRO(page) & DMPP_CLEAN) == 0)
	return (E_DB_INFO);

    /* Reclaim space from committed transactions only */
    for (i = 0; i < nextlno; i++)
    {
	if (dmppn_v2_test_free_macro(lp, i) == 0)/* reclaimable row? */
	    continue;

	if (!xlock)
	{
	    tup_hdr = ((char*)page + dmppn_v2_get_offset_macro(lp, (i4)i));

	    /*
	    ** Check for current transaction id (which is alive)
	    ** For space reclamation we only need to look at low tran
	    ** which is guaranteed to be unique for live transactions
	    */
	    if ( Dmpp_pgtype[page_type].dmpp_has_lgid )
	    {
		MECOPY_CONST_2_MACRO(
		 (PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_lgid_offset,
		     &row_lg_id);
	    }
	    else
		row_lg_id = 0;

	    MECOPY_CONST_4_MACRO(
		 (PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_lowtran_offset,
		 &row_low_tran);

	    /* do not reclaim if row deleted by same transaction */
	    if (tranid && row_low_tran == tranid->db_low_tran)
		continue;
	    else if (crow_locking(r))
	    {
		/*
		** Don't reclaim unless committed before consistent read
		** point in time described by the transactions "crib"
		*/
		if (!row_is_consistent(r, row_low_tran, row_lg_id))
		{
		    /* skip and don't delete */
		    continue;
		}
	    }
	    else if ( IsTranActive(row_lg_id, row_low_tran) )
		continue;
	    /* committed/consistent, ok to reclaim */
	}

	/* compressed or altered tables: get entry size */
	dm1cnv2_reclen(page_type, page_size, page, i, &del_recsize);
	del_tid.tid_tid.tid_page = DMPP_V2_GET_PAGE_PAGE_MACRO(page);
	del_tid.tid_tid.tid_line = i;

	dm1cnv2_delete (page_type,  page_size, page, 
		(i4)DM1C_DIRECT, (i4)TRUE, (DB_TRAN_ID *)0, (u_i2)0, 
		&del_tid, del_recsize);

	clean_bytes += del_recsize;

    }

    *avail_space = clean_bytes;
    return (E_DB_OK);
}
