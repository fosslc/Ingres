/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <pc.h>
#include    <tr.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lk.h>
#include    <lg.h>
#include    <dmf.h>
#include    <scf.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dm.h>
#include    <dmpp.h>
#include    <dmp.h>
#include    <dm0p.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm1c.h>
#include    <dm1cx.h>
#include    <dmd.h>
#include    <dmftrace.h>
#include    <dm0m.h>
#include    <dmxe.h>
#include    <bt.h>

/**
**
** Name: DM1CIDX.C		- Routines to manipulate Btree Index Pages
**
** Description:
**	This file contains routines which manipulate Btree Index Pages. These
**	routines are analogous to the routines in DM1C.C, which manipulate
**	normal data pages. Btree index pages differ in a few ways, but
**	the methods used to access the pages are similar.
**
**	Btree index pages:
**	    Are mapped by DM1B_INDEX, not DMPP_PAGE
**	    Contain entries which are (key,tid) pairs
**	    Have sideways pointers
**	    Have associated data page pointers
**	    Have 2 (DM1B_OFFSEQ) magic reserved entries in the line table
**	    Maintain the line table in a specific order
**	    Keep the line table compacted
**	    Have magic distinguished portions of each record ((key,tid) pairs)
**
**	It is a shame that we cannot use identical routines to manage all
**	Ingres pages. That is a long term goal.
**
**	Routines:
**
**	dm1cxallocate		- Allocates space for an entry on a page
**	dm1cxcomp_rec		- Compress an entry
**	dm1cxdel		- Deletes an entry from a page
**	dm1cxformat		- Format an empty Btree index page
**	dm1cxget		- Gets an uncompressed entry from a page
**	dm1cxhas_room		- Does this page have room for this entry?
**	dm1cxjoinable		- Are these two pages joinable?
**	dm1cxlength		- Calculates length of index entry
**	dm1cxlog_error		- log an error encountered on an index page.
**	dm1cxlshift		- shift entries 'left' during join processing
**	dm1cxmax_length		- Calculate max length of compressed entry
**	dm1cxmidpoint		- Find the midpoint of the page (for splitting)
**	dm1cxput		- Puts entry onto page                    
**	dm1cxrclaim		- Reclaim data space for deleted record(s)
**	dm1cxreplace		- Replace entry on page
**	dm1cxrshift		- Shift entries from one page to another
**	dm1cxtget		- Get the TIDP portion of an entry from a page
**	dm1cxtput		- Put the TIDP portion of an entry onto a page
**
**	The naming convention is, I hope, relatively obvious: a routine named
**	dm1cx* is the index page counterpart of the similar-named dm1c_*
**	routine in dm1c.c. Given the 7 character uniqueness constraint on
**	routine names, we don't have much latitude, and thus some names don't
**	exactly follow the convention (dm1cxlength vs. dm1c_reclen, etc).
**
**	Some routines which are VOID in dm1c.c return a DB_STATUS here. These
**	routines are more defensive about their arguments and perform extra
**	reasonable-ness checks whenever possible, with the goal of increased
**	reliability and robustness.
**
**	I haven't come up with a good name to call the magic 4 byte TID portion
**	of each (key,tid) pair in an index entry. The problem is that on LEAF
**	and OVERFLOW pages this 'tid' is actually a real tuple identifier,
**	which may be an identifier in the same file (for primary relations) or
**	an identifier in a separate file (for secondary index relations), and
**	thus is best referred to as a 'TIDP', whereas on INDEX pages this 'tid'
**	is actually a page number which points down to the appropriate lower
**	page in the index, and thus is best referred to as a 'PAGENO'. Thus
**	you will see various unfortunate code which uses all these different
**	names.
**
**	It is a VERY serious coding error to use one of the new compressed
**	index algorithms on an uncompressed btree index, or, vice versa, to use
**	an uncompressed algorithm on a compressed btree index. It would be nice
**	if the code could protect itself against this sort of misuse. For
**	example, if there were a way to mark each page as either "part of a
**	compressed index btree" or "not part of a compressed index btree", then
**	the dm1cidx layer could check that compressed algorithms were used
**	correctly.
**
**	My first thought along this path was to allocate some space from the
**	"bt_unused" section of the DM1B_INDEX structure.  However, the
**	"bt_unused" section of the DM1B_INDEX structure has NOT been reliably
**	cleared when btrees were built in the past, so we cannot count on its
**	contents being zero in current Btree index pages.
**
**	My second thought was to allocate a new page_stat bit. However, it does
**	not seem reasonable to waste one of the three remaining page_stat bits
**	on a bit of debugging code.
**
**	dm1cxformat() will be written as part of this project to reliably clear
**	all unused parts of the DM1B_INDEX structure with binary zeroes. Note,
**	however, that this will NOT fix old Btrees...
**
** History:
**	3-dec-1990 (bryanp)
**	    Created
**	10-sep-1991 (bryanp)
**	    B37642, B39322: Allow zero-length key to dm1cxreplace (L/R ranges).
**	 7-jul-1992 (rogerk)
**	    Add prototypes - removed ifdefs around prototype code.
**	    Added include of new dm1cx.h
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	26-oct-92 (rickh)
**	    Replaced references to DMP_ATTS with DB_ATTS.  Also replaced
**	    DM_CMP_LIST with DB_CMP_LIST.
**	28-oct-1992 (jnash)
**	    Reduced logging project.  Add unused params to dmppxuncompress.
**	14-dec-1992 (rogerk)
**	    Reduced logging project (phase 2).  Added dm1cxrecptr routine.
**	    Removed TCB parameter from dm1cxformat.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      13-sep-93 (smc)
**          Made TRdisplay use %p for pointers.
**	11-oct-93 (johnst)
**	    Bug #56449
**	    Changed TRdisplay format args from %x to %p where necessary to
**	    display pointer types. We need to distinguish this on, eg 64-bit
**	    platforms, where sizeof(PTR) > sizeof(int).
**	06-mar-1996 (stial01 for bryanp)
**	    Pass page_size argument to the DM1B_FREE macro.
**	    Pass page_size argument to dmpp_format accessor.
**	    Add page_size argument to dm1cxallocate, dm1cxdel, dm1cxformat,
**		dm1cxlength, dm1cxput, dm1cxrclaim, lowest_offset.
**      06-mar-1996 (stial01)
**          dm1cxrclaim() use page_size when validating offset is inside page
**      03-june-1996 (stial01)
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**	20-jun-1996 (thaju02)
**	    Use dm0m_lfill() when zero filling page with page size greater
**	    than max value for u_i2.
**	16-sep-1996 (canor01)
**	    Pass buffer argument to dmppxuncompress.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          dm1cxallocate() When (page_type != TCB_PG_V1):
**             Don't update Page tranid, PAGE seq number
**          dm1cxdel() Added reclaim_space for conditional space reclamation
**          xdelete() Created from dm1cxdelete deletes and reclaim space
**          dm1cxreplace() Check: Always reclaim space for leaf LRANGE/RRANGE
**          dm1cxput() When (page_type != TCB_PG_V1):
**             Copy tran_id into tuple header for leaf pages.
**             Don't update PAGE tranid, PAGE seq number
**          dm1cxget() When (page_type != TCB_PG_V1):
**             Return E_DB_WARN for deleted leaf entries
**          dm1cxrshift() When (page_type != TCB_PG_V1):
**             Copy free flag to entry on sibling.
**             When calling dm1cxdel to delete entries from page being split,
**             specify reclaim_space = TRUE.
**          dm1cxclean() New routine to reclaim space on leaf page
**      04-feb-97 (stial01)
**          Tuple headers are on LEAF and overflow (CHAIN) pages
**          Tuple headers are not on INDEX pages.
**      27-feb-1997 (stial01)
**          dm1cxdel() Set DMPP_CLEAN in page status when we mark tuple deleted.
**          When (page_type != TCB_PG_V1): On leaf pages with overflow
**          chain error if delete/reclaim the last 'duplicated key value'.
**          dm1cxrclaim() Clear DMPP_CLEAN when no more deleted tuples on page.
**          dm1cxclean() changed parameters. Also, on leaf pages with overflow 
**          chain we never delete/reclaim the last 'duplicated key value'.
**          Added dm1cxkeycount()
**      10-mar-97 (stial01)
**          dm1cxclean() xid array is empty if xid_cnt is zero
**          xdelete() Clear DMPP_CLEAN when no more deleted tuples on page.
**      07-apr-97 (stial01)
**          dm1cxdel() NonUnique primary btree (V2) dups span leaf pages,
**          not overflow chain. Remove code to check for delete of overflow key
**          dm1cxclean() NonUnique primary btree (V2) dups span leaf pages,
**          not overflow chain. Remove code skipping clean of overflow key
**      12-jun-97 (stial01)
**          dm1cxformat() new attribute, key parameters
**          dm1cxget() Pass tlv instead of tcb.
**          dm1cxlength() DON'T assume tuple ends at end of page
**          lowest_offset() DON'T assume tuple ends at end of page
**      29-jul-97 (stial01)
**          dm1cxdel() Even if page locking, put non zero tranid into tuple
**          header.
**      15-jan-98 (stial01)
**          dm1cxclean() Removed unused err_code arg, log error if delete fails 
**      07-jul-98 (stial01)
**          Deferred update processing for V2 pages, use page tranid
**          when page locking, tuple header tranid when row locking
**          dm1cxget() Always return tranid of leaf page entry.
**	01-dec-1998 (nanpr01)
**	    Get rid of DMPP_TUPLE_INFO structure. Use DMPP_TUPLE_HDR instead.
**      07-Dec-1998 (stial01)
**          Added kperleaf to distinguish from kperpage (keys per index page)
**          Added dm1cxkperpage
**      09-feb-99 (stial01)
**          dm1cxkperpage() Check relcmptlvl when calculating kperpage for 
**          BTREE index page. Do not limit kperpage on INDEX pages.
**      12-apr-1999 (stial01)
**          Btree secondary index: non-key fields only on leaf pages
**          Different att/key info for LEAF vs. INDEX pages
**      15-Apr-1999 (stial01)
**          No deferred update processing on index pages
**          dm1cxput() Don't copy tuple header from page (it just gets cleared
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      04-may-1999 (hanch04)
**          Change TRformat's print function to pass a PTR not an i4.
**      12-nov-1999 (stial01)
**          Changes to how btree key info is stored in tcb
**	21-nov-1999 (nanpr01)
**	    Code optimization/reorganization based on quantify profiler output.
**      01-dec-1999 (gupsh01)
**          Added check for max allowable rows on a compressed keys,
**          non-index pages. This is determined by the bitsize set for
**          tid_line in tid_tid structure. (bug #b99368 )
**	23-Jun-2000 (jenjo02)
**	    Added fill_option to dmpp_format() prototype, set by caller
**	    to DM1C_ZERO_FILL if dmpp_format should MEfill the page,
**	    DM1C_ZERO_FILLED if the page has already been MEfilled.
**	    The page overhead space is always zero-filled.
**	    Removed superfluous MEfill of page from dm1cxformat().
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
**          Updated for new page types (Variable Page Type SIR 102955)
**          No deferred processing for delete, it is done in opncalcost.c b59679
**	    dm1cxmidpoint() called from split, split_point must cause a key
**	    to move.
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**	10-sep-2001 (somsa01)
**	    Modified previous cross-integration to conform to Alison's
**	    architectural changes.
**      28-may-2003 (stial01)
**          Keep deferred tid list sorted 
**	04-sep-2003 (abbjo03)
**	    Add missing argument to dm1cxkperpage() call that was hidden by
**	    #ifdef xDEBUG.
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	13-Jun-2006 (jenjo02)
**	    Using DM1B_MAXSTACKKEY instead of DM1B_KEYLENGTH, call dm1b_AllocKeyBuf() 
**	    to validate/allocate Leaf work space. DM1B_KEYLENGTH is now only the 
**	    max length of key attributes, not necessarily the length of the Leaf entry.
**      16-nov-2008 (stial01)
**          Redefined name constants without trailing blanks.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**      10-sep-2009 (stial01)
**          If V2+ pages have overflow chain, don't delete/reclaim last dupkey
**      12-oct-2009 (stial01)
**          Use DMPP_PAGE for all page types.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Changes for MVCC
**	23-Feb-2010 (stial01)
**          dm1cxclean() Add rcb param, Skip clean if cursor is positioned on
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**      09-Jun-2010 (stial01)
**          Added dm1cx_txn_get()
**      12-Jul-2010 (stial01) (SIR 121619 MVCC, B124076, B124077)
**          dm1cxclean() if crow_locking row_is_consistent() else IsTranActive
**          dm1cxisnew() Check rcb_new_fullscan, handle the XTABLE case where
**          new tids are added to rcb_new_tids without any update to the page,
**      16-Jul-2010 (stial01) (SIR 121619 MVCC, B124076, B124077)
**          dm1cxisnew() above change dropped a line.
**        
*/

static	i4	    lowest_offset(i4 page_type, i4 page_size, DMPP_PAGE *b);
static	i4	    print(PTR arg, i4 count, char *buffer);
static	VOID	    page_dump(i4 page_type, i4 page_size, DMPP_PAGE *page);
static	VOID	    clear(DMP_RCB *r, DMPP_PAGE *page);
static	VOID	    dm1cxlength(i4 page_type, i4 page_size, DMPP_PAGE *b,
		    i4 child, i4 *entry_size);

GLOBALCONSTREF DMPP_PAGETYPE Dmpp_pgtype[];


/*
** Name: dm1cxallocate		- Allocate space for an entry on a page
**
** Description:
**	This routine allocates space for an index entry on a Btree index
**	page. It assumes that enough space for the entry exists on the
**	page.
**
**	This routine differs from dm1c_allocate in that the caller of this
**	routine indicates a specific position on the page which is to be
**	allocated (line table entries on Btree index pages must be kept in
**	order). This routine shifts any other line table entries over in order
**	to make room for the specified entry.
**
**	Note that this routine returns a DB_STATUS value. We do not simply
**	blindly assume that the page contains enough space for the indicated
**	allocation; rather, we re-check the page (since this is cheap) and
**	complain if the allocation can't be performed.
**
**	The record_size passed in to this routine should include the TidSize
**	size for the tid. That is, pass the size of the entire (key,tid) pair.
**
** Inputs:
**	update_mode			0 or DM1C_DEFERRED
**	compression_type		Value indicating index compression:
**					    DM1CX_UNCOMPRESSED
**					    DM1CX_COMPRESSED
**	tran_id				transaction ID if deferred
**	sequence_number			transaction's seqno if deferred
**	child				The LINEID part of a TID used to
**					indicate where on this page the
**					insertion is to take place
**	b				the DMPP_PAGE page to use
**	page_size			The page size for this table.
**	record_size			the (compressed) size of the entry,
**					including the size for the tid.
**
** Outputs:
**	None, but the page is updated
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	5-dec-1990 (bryanp)
**	    Created.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all the page header access as macro for New Page Format
**	    project.
**	    Pass page_size argument to dmpp_put_offset_macro for tuple header
**	    implementation.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          dm1cxallocate() When (page_type != TCB_PG_V1):
**          Don't update Page tranid, PAGE seq number
**          (seq number is maintained per tuple on large pages)
**      04-feb-97 (stial01)
**          Tuple headers are on LEAF and overflow (CHAIN) pages
**          Tuple headers are not on INDEX pages.
**      07-jul-98 (stial01)
**          dm1cxallocate() Deferred update processing for V2 pages, use page
**          tranid when page locking, tuple header tranid when row locking
**	22-Jan-2004 (jenjo02)
**	    Extract TidSize from page;
**	13-Jun-2006 (jenjo02)
**	    The maximum size of a Leaf is now DM1B_MAXLEAFLEN,
**	    not DM1B_KEYLENGTH.
*/
DB_STATUS
dm1cxallocate(i4 page_type, i4 page_size, DMPP_PAGE *b,
		i4 update_mode, i4 compression_type,
		DB_TRAN_ID *tran_id, i4 sequence_number, i4 child,
		i4 record_size)
{
    i4	    next;
    i4	    pos;
    i4	    entry_length;
    char	    buffer[132];
    bool            is_index;
    i2		    TidSize = DM1B_VPT_GET_BT_TIDSZ_MACRO(page_type, b);

     is_index = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, b) & DMPP_INDEX) != 0);

    if (record_size < TidSize || record_size > (DM1B_MAXLEAFLEN + TidSize))
    {
	TRdisplay("dm1cxallocate: Invalid record size %d\n", record_size);
	return (E_DB_ERROR);
    }

    if (!is_index)
      record_size += Dmpp_pgtype[page_type].dmpp_tuphdr_size;

    /*
    ** locate space for where next record will be put. If the table is not
    ** compressed, then the next line table entry has been pre-formatted with
    ** the correct offset; otherwise we must search for the lowest offset and
    ** then allocate space for this record.
    */
    if (compression_type == DM1CX_COMPRESSED)
    {
	next = lowest_offset(page_type, page_size, b) - record_size;

	/*
	** this would be the place to update the page with the new lowest_offset
	** value if we decide to store it rather than calculate it on demand.
	*/
    }
    else
    {
	next = dmpp_vpt_get_offset_macro(page_type,
		DM1B_VPT_BT_SEQUENCE_MACRO(page_type, b),
		(i4)DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b) + DM1B_OFFSEQ);
    }

    /*
    ** SANITY CHECK: we believe that the new entry should be stored at offset
    ** 'next'. This value should be < DM_PG_SIZE and > the highest currently
    ** used line table offset. If it's wrong, this is a VERY serious error.
    */
    if (next >= page_size || ((char *)b + next) < 
		((char *)(DM1B_VPT_GET_ADDR_BT_SEQUENCE_MACRO(page_type, b, 
			DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b)+ DM1B_OFFSEQ + 1))))
    {
	TRdisplay("dm1cxallocate: Next (%d) out of range on page %d(kids %d)\n",
		   next, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b), 
		   DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b));
	TRdisplay("             : Valid offset range is [%d .. %d]\n",
	    (char *)(DM1B_VPT_GET_ADDR_BT_SEQUENCE_MACRO(page_type, b, 
		DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b) + DM1B_OFFSEQ + 1)) - 
		(char *)b, page_size);

	return (E_DB_ERROR);
    }

    /*
    ** Shift the line table entries over to make room for the new entry.
    ** Always shift the offset of the line table entries to the left
    ** starting with the position of insertion.
    ** This leaves two identical entries starting at position.
    ** Nameley the position where the insertion is to occur and the
    ** next position contain exactly the same data.  
    */

    for (pos = DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b); pos > (i4)child; 
	 pos--)
        DM1B_VPT_SET_BT_SEQUENCE_MACRO(page_type, b, pos + DM1B_OFFSEQ, 
            pos + DM1B_OFFSEQ - 1); 

    /*
    ** pos and child are now identical, and both point to the new line table
    ** entry which we have now allocated. Complete the allocation by recording
    ** the offset which we have assigned to this entry and incrementing the
    ** total number of entries on the page.
    */

    dmpp_vpt_put_offset_macro(page_type, 
			    DM1B_VPT_BT_SEQUENCE_MACRO(page_type, b), 
			    pos + DM1B_OFFSEQ, next);
    dmpp_vpt_clear_flags_macro(page_type, DM1B_VPT_BT_SEQUENCE_MACRO(page_type, b),
			   pos + DM1B_OFFSEQ);

    DM1B_VPT_INCR_BT_KIDS_MACRO(page_type, b);
    DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, b, DMPP_MODIFY);

    if (DMZ_AM_MACRO(11))
    {
	TRformat(print, 0, buffer, sizeof(buffer) - 1,
	"%8* dm1cx: Allocated new entry %d (size %d) on page %d at offset %d.",
		child, record_size, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b), 
		next );
    }

    if (DMZ_AM_MACRO(12))
    {
	if (compression_type == DM1CX_COMPRESSED)
	{
	    /*
	    ** Extra edit checking. Certain post-conditions should be true
	    ** following an allocate
	    **	1) The length of the entry we just allocated should be correct.
	    **	2) The length of each kid should be reasonable
	    */
	    dm1cxlength(page_type, page_size, b, child, &entry_length );
	    if (entry_length != record_size)
	    {
		TRdisplay("dm1cxallocate: new entry is length %d, not %d\n",
			    entry_length, record_size);
		dm1cxlog_error(E_DM93EC_BAD_INDEX_LENGTH, (DMP_RCB *)0, b,
				    page_type, page_size, (i4)child);
		return (E_DB_ERROR);
	    }
	    for (pos = 0; pos < DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b); pos++ )
	    {
		dm1cxlength(page_type, page_size, b, pos, &entry_length );
		if (!is_index)
			entry_length -= Dmpp_pgtype[page_type].dmpp_tuphdr_size;
		if (entry_length < TidSize ||
		    entry_length > (DM1B_MAXLEAFLEN + TidSize))
		{
		    dm1cxlog_error(E_DM93EC_BAD_INDEX_LENGTH, (DMP_RCB *)0, b,
				    page_type, page_size, (i4)pos);
		    return (E_DB_ERROR);
		}
	    }
	}
    }

    return (E_DB_OK);
}

/*
** Name: dm1cxcomp_rec		- Compress an entry
**
** Description:
**	This routine compresses a Btree index entry.
**	The caller must determine that some sort of compression is in fact
**	needed, because we're going to dispatch to the rowaccessor's
**	compression routine blindly!
**
** Inputs:
**	rac				DMP_ROWACCESS structure with info
**					about atts, compression type, etc.
**      rec                             Pointer to record to compress.
**      rec_size                        Uncompressed record size. 
**      crec                            Pointer to an area to return compressed
**                                      record.
**
** Outputs:
**      crec_size		        Pointer to an area where compressed
**                                      record size can be returned.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR			Tuple could not be compressed.
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**	12-dec-1990 (bryanp)
**	    Created.
**	22-jan-1991 (bryanp)
**	    Changed atts_array from (DB_ATTS *) to (DB_ATTS **). Call
**	    dm1cpcomp_rec rather than dm1c_comp_rec.
**	05_jun_1992 (kwatts)
**	    MPF change, added the TCB as first parameter. Call compression
**	    accessor insead if dm1cpcomp_rec.
**	22-Feb-2008 (kschendel) SIR 122739
**	    Pass row-accessor structure instead of separate bits and pieces.
*/
DB_STATUS
dm1cxcomp_rec(DMP_ROWACCESS *rac,
		char *rec, i4 rec_size, char *crec, i4 *crec_size)
{
    char	    buffer[132];
    DB_STATUS	    status;

    status = (*rac->dmpp_compress)(rac,
				rec, rec_size, crec, crec_size );
    if (DMZ_AM_MACRO(11))
    {
	if (status == E_DB_OK)
	    TRformat(print, 0, buffer, sizeof(buffer) - 1,
		    "%8* dm1cx: Compressed index entry from %d bytes to %d",
		    rec_size, (*crec_size));
	else
	    TRformat(print, 0, buffer, sizeof(buffer) - 1,
		    "%8* dm1cx: ERROR encountered compressing entry!");
    }
    return (status);

}

/*
** Name: dm1cxdel		- Delete an entry from a page
**
** Description:
**	This routine deletes the entry specified by BID from a index page.
**	It is assumed that the BID is valid and that this entry has not
**	already been deleted by this transaction.
**
**	Our current algorithm always compresses the line table and always
**	compacts the entry area if index compression is in effect. It would
**	be nice to eventually migrate toward a scheme where we didn't need
**	to immediately reclaim the space, but rather could garbage collect
**	an entire page after several deletions.
**
** Inputs:
**	update_mode			0 or DM1C_DEFERRED
**	compression_type		Value indicating index compression:
**					    DM1CX_UNCOMPRESSED
**					    DM1CX_COMPRESSED
**	tran_id				transaction ID if deferred
**	sequence_number			transaction's seqno if deferred
**	b				the DMPP_PAGE page to use
**	page_size			The page size for this table.
**	child				The LINEID part of a TID used to
**					indicate where on this page the
**					deletion is to take place
**      reclaim_space                   Conditional space reclamation
**
** Outputs:
**	None, but the record is deleted from the page
**
** Returns:
**	DB_STATUS
**
** History:
**	5-dec-1990 (bryanp)
**	    Created.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
**	06-may-1996 (nanpr01)
**	    Change all the page header access as macro for New Page Format
**	    project.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          dm1cxdel() Added reclaim_space for conditional space reclamation
**      04-feb-97 (stial01)
**          Tuple headers are on LEAF and overflow (CHAIN) pages
**          Tuple headers are not on INDEX pages.
**      27-feb-97 (stial01)
**          dm1cxdel() Set DMPP_CLEAN in page status when we mark tuple deleted.
**          When (page_type != TCB_PG_V1): On leaf pages with overflow
**          chain we error if delete/reclaim the last 'duplicated key value'.
**          Added row locking parameter, if not reclaiming space and not
**          row locking, use zero tranid so space can be reclaimed as soon 
**          as possible
**      07-apr-97 (stial01)
**          dm1cxdel() NonUnique primary btree (V2) dups span leaf pages, not
**          not overflow chain. Remove code to check for delete of overflow key
**      29-jul-97 (stial01)
**          dm1cxdel() Even if page locking, put non zero tranid into tuple
**          header.
**      07-jul-98 (stial01)
**          dm1cxdel() Deferred update processing for V2 pages, use page tranid
**          when page locking, tuple header tranid when row locking
*/
DB_STATUS
dm1cxdel(i4 page_type, i4 page_size, DMPP_PAGE *b,
		i4 update_mode, i4 compression_type,
		DB_TRAN_ID *tran_id, u_i2 lg_id, i4 sequence_number,
		i4 flags, i4 child) 
{
    i4	    pos;
    i4	    entry_length;
    char	    buffer[132];
    bool            is_index;
    bool            is_leaf;
    DB_STATUS	    ret_val = E_DB_OK;
    char	    *tup_hdr;
    DB_STATUS       status;

    is_index = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, b) & DMPP_INDEX) != 0);
    is_leaf = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, b) & DMPP_LEAF) != 0);

    /* If its not our tran, place ours there with invalid sequence 0 */
    if ( tran_id )
    {
	if ((DM1B_VPT_GET_TRAN_ID_HIGH_MACRO(page_type, b) != 
						    tran_id->db_high_tran)
	     || (DM1B_VPT_GET_TRAN_ID_LOW_MACRO(page_type, b) 
						    != tran_id->db_low_tran)) 
	{
	    DM1B_VPT_SET_PAGE_TRAN_ID_MACRO(page_type, b, *tran_id);
	    DM1B_VPT_SET_PAGE_SEQNO_MACRO(page_type, b, 0);
	    DM1B_VPT_SET_PAGE_LG_ID_MACRO(page_type, b, lg_id);
	}
    }

    if (DMZ_AM_MACRO(11))
    {
	TRformat(print, 0, buffer, sizeof(buffer) - 1,
		"%8* dm1cx: Deleting entry %d from page %d",
		(i4)child, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b));
    }

    if (DMZ_AM_MACRO(12))
    {
	if (b == (DMPP_PAGE *)0 || (i4)child < DM1B_LRANGE ||
	    (i4)child >= DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b) ||
	    dmpp_vpt_get_offset_macro(page_type, 
		DM1B_VPT_BT_SEQUENCE_MACRO(page_type, b),
		(i4)child + DM1B_OFFSEQ) < 
				((unsigned) DM1B_VPT_OVER(page_type)) ||
	    dmpp_vpt_get_offset_macro(page_type,
		DM1B_VPT_BT_SEQUENCE_MACRO(page_type, b),
		(i4)child + DM1B_OFFSEQ) > ((unsigned) page_size))
	{
	    TRdisplay("dm1cxdel: b=%p, child=%d\n", b, (i4)child);
	    if (b)
		TRdisplay("       : offset= %d\n",
		  dmpp_vpt_get_offset_macro(page_type,
			DM1B_VPT_BT_SEQUENCE_MACRO(page_type, b),
			(i4)child + DM1B_OFFSEQ));
	    return (E_DB_ERROR);
	}
    }

    if (page_type == TCB_PG_V1)
    {
	status = xdelete(page_type, page_size, b, compression_type, child);
    }
    else
    {
	if (is_index || (flags & DM1CX_RECLAIM))
	{
	    status = xdelete(page_type, page_size, b, compression_type, child);
	}
	else
	{
	    /* Set the free space bit */
	    dmpp_vpt_set_free_macro(page_type, 
				    DM1B_VPT_BT_SEQUENCE_MACRO(page_type, b),
				    (i4)child + DM1B_OFFSEQ);

	    tup_hdr = dm1b_vpt_entry_macro(page_type, b, (i4)child);

	    /*
	    ** This macro clears only fields that are in-use,
	    ** an optimization for V2 pages
	    */
	    DMPP_VPT_DELETE_CLEAR_HDR_MACRO(page_type, tup_hdr);

	    /*
	    ** delete: Put low tran into tuple header
	    ** When marking a key deleted we need to store the low tran 
	    ** The tranid is used to determine when the space can be reclaimed
	    ** The deleted key may also needed for reposition, so
	    ** put the low tran even if page locking
	    */
	    MECOPY_CONST_4_MACRO(&tran_id->db_low_tran,
		(PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_lowtran_offset);

	    if ( Dmpp_pgtype[page_type].dmpp_has_lgid )
	    {
		MECOPY_CONST_2_MACRO(&lg_id,
		    (PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_lgid_offset);
	    }

	    /* Indicate page needs to be cleaned */
	    DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, b, DMPP_CLEAN);

	    status = E_DB_OK;
	}
    }

    DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, b, DMPP_MODIFY);

    return (status);
}

/*
** Name: xdelete - Delete an entry from a page
**
** Description:
**	This routine deletes the entry specified by BID from a index page.
**	It is assumed that the BID is valid and that this entry has not
**	already been deleted by this transaction.
**
**	Our current algorithm always compresses the line table and always
**	compacts the entry area if index compression is in effect. It would
**	be nice to eventually migrate toward a scheme where we didn't need
**	to immediately reclaim the space, but rather could garbage collect
**	an entire page after several deletions.
**
** Inputs:
**	compression_type		Value indicating index compression:
**					    DM1CX_UNCOMPRESSED
**					    DM1CX_COMPRESSED
**	child				The LINEID part of a TID used to
**					indicate where on this page the
**					deletion is to take place
**	b				the DMPP_PAGE page to use
**	page_size			The page size for this table.
**
** Outputs:
**	None, but the record is deleted from the page
**
** Returns:
**	DB_STATUS
**
** History:
**	5-dec-1990 (bryanp)
**	    Created.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
**	06-may-1996 (nanpr01)
**	    Change all the page header access as macro for New Page Format
**	    project.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Created from dm1cxdel()
**      04-feb-97 (stial01)
**          Tuple headers are on LEAF and overflow (CHAIN) pages
**          Tuple headers are not on INDEX pages.
**      10-mar-97 (stial01)
**          xdelete() Clear DMPP_CLEAN when no more deleted tuples on page.
**	22-Jan-2004 (jenjo02)
**	    Extract TidSize from page;
**	13-Jun-2006 (jenjo02)
**	    The maximum size of a Leaf is now DM1B_MAXLEAFLEN,
**	    not DM1B_KEYLENGTH.
*/
DB_STATUS
xdelete(i4 page_type, i4 page_size, DMPP_PAGE *b, i4 compression_type,i4 child)
{
    i4	    	    pos;
    DM_LINEID	    old_btseq;
    i4	    	    entry_length;
    char	    buffer[132];
    bool            is_index;
    DB_STATUS	    ret_val = E_DB_OK;
    char            *tuple_addr;
    i4         	    del_cnt;
    i4		    kids;

    i2		TidSize = DM1B_VPT_GET_BT_TIDSZ_MACRO(page_type, b);

    is_index = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, b) & DMPP_INDEX) != 0);
    kids = DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b) - 1;

    /*
    ** When there is an overflow chain ...
    ** always leave one dupkey on the leaf
    ** So we don't have to read the overflow chain to find out the dupkey
    ** (the dupkey doesn't have to match a range key)
    ** This entry should have tidp 0, so it can't conflict with any real entry
    */

    if (DM1B_VPT_GET_PAGE_OVFL_MACRO(page_type, b) && page_type != TCB_PG_V1
		&& kids == 1)
    {
	DM_TID	tmp_tid;
	i4	tmp_part;
	DB_STATUS status;
	char	*tup_hdr;

	dm1cxtget(page_type, page_size, b, child, &tmp_tid, &tmp_part);
/*
	TRdisplay("xdelete pg %d reset tid %d %d\n",
		DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b),
		tmp_tid.tid_tid.tid_page, tmp_tid.tid_tid.tid_line);
*/
	tmp_tid.tid_i4 = 0;
	tmp_part = 0;
	status = dm1cxtput(page_type, page_size, b, child, &tmp_tid, tmp_part);

	/* Set the free space bit */
	dmpp_vpt_set_free_macro(page_type, 
	    DM1B_VPT_BT_SEQUENCE_MACRO(page_type, b), (i4)child + DM1B_OFFSEQ);

	tup_hdr = dm1b_vpt_entry_macro(page_type, b, (i4)child);
	DMPP_VPT_DELETE_CLEAR_HDR_MACRO(page_type, tup_hdr);

	/* Indicate page needs to be cleaned */
	DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, b, DMPP_CLEAN);

	return (E_DB_OK);
    }

    if (compression_type == DM1CX_COMPRESSED)
    {
	/*
	** if the index entries are compressed, then we must compact the
	** data space, since we are about to compact the line table and will
	** thus lose the location of the deleted record.
	*/
	/*
	** Compact the data space to reclaim the entry portion
	*/
	ret_val = dm1cxrclaim(page_type, page_size, b, child );
	if (ret_val)
	    return (ret_val);

	/*
	** Compress the line table to remove the now deleted entry.
	*/

	for (pos = child; pos < kids; pos++)
	    DM1B_VPT_SET_BT_SEQUENCE_MACRO(page_type, b, pos + DM1B_OFFSEQ, 
	       pos + DM1B_OFFSEQ + 1);
    }
    else
    {
	/*
	** If the index entries are NOT compressed, then we don't need to
	** compact the data space. Instead the pages are pre-formatted, so we
	** just rearrange the offsets so that the deleted offset is moved to
	** the location just after all the offsets which are in use. This
	** offset will then be re-used by the next entry allocation on this
	** page.
	*/

	DM1B_VPT_GET_BT_SEQUENCE_MACRO(page_type, b, child + DM1B_OFFSEQ, 
				old_btseq);

	/* Now compress line table to delete this entry. */
	for (pos = child; pos < kids; pos++)
	    DM1B_VPT_SET_BT_SEQUENCE_MACRO(page_type, b, pos + DM1B_OFFSEQ, 
	       pos + DM1B_OFFSEQ + 1);

	DM1B_VPT_ASSIGN_BT_SEQUENCE_MACRO(page_type, b, pos + DM1B_OFFSEQ, 
				   old_btseq);

	if ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, b) & DMPP_CLEAN) != 0)
	{
	    del_cnt = 0;
	    for (pos = 0; pos < kids; pos++)
	    {
		/* Test for deleted row */
		if (dmpp_vpt_test_free_macro(page_type,  
			DM1B_VPT_BT_SEQUENCE_MACRO(page_type, b),
			(i4)pos + DM1B_OFFSEQ))
		{
		    del_cnt++;
		    break;
		}
	    }
	    if (del_cnt == 0)
		DM1B_VPT_CLR_PAGE_STAT_MACRO(page_type, b, DMPP_CLEAN);
	}
    }

    DM1B_VPT_DECR_BT_KIDS_MACRO(page_type, b);

    if (DMZ_AM_MACRO(12))
    {
	if (compression_type == DM1CX_COMPRESSED)
	{
	    /*
	    ** Extra edit checking. Certain post-conditions should be true
	    ** following a delete
	    **	1) The length of each kid should be reasonable
	    */
	    for (pos = 0; pos < DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b); pos++ )
	    {
		dm1cxlength(page_type, page_size, b, pos, &entry_length );
		if (!is_index)
		    entry_length -= Dmpp_pgtype[page_type].dmpp_tuphdr_size;
		if (entry_length < TidSize ||
		    entry_length > (DM1B_MAXLEAFLEN + TidSize))
		{
		    dm1cxlog_error(E_DM93EC_BAD_INDEX_LENGTH, (DMP_RCB *)0, b,
				    page_type, page_size, (i4)pos);
		    return (E_DB_ERROR);
		}
	    }
	}
    }

    return (E_DB_OK);
}

/*{
** Name: dm1cxformat - Format a BTREE INDEX page.
**
** Description:
**      This routine formats an empty BTREE Index page.
**
**	Unformatted portions of the page are MEfill'ed with binary zeroes.
**
**	Non-compressed Btree Index pages are formatted completely at this time.
**	In particular, it initializes the pointers in the line table.  They are
**	initialized to point to fixed size segments of the size (sizeof(key) +
**	sizeof(tid)).
**
**	Compressed Btree Index pages manage their space dynamically, so we
**	do not set up the line table entries. kperpage and klen need not be set.
**
**	NOTE: We need to resolve the issue of dm1cx_allocate for the LRANGE and
**	RRANGE entries. For now, I will allocate each entry a TidSize-sized
**	entry here, and then depend on the caller to call dm1cxreplace() later
**	when the actual key value is known.
**
** Inputs:
**	t			    TCB for access to accessors.
**	compression_type		Value indicating index compression:
**					    DM1CX_UNCOMPRESSED
**					    DM1CX_COMPRESSED
**      b				Pointer to the page to format.
**	page_size			Page size.
**      kperpage                        Value indicating maximum keys per page. 
**      klen                            Value indicating key length.
**      page			        The PAGENO part of a TID which 
**                                      indicates the logical page number
**                                      of page formatting.
**
** Outputs:
**	b				The page is formatted appropriately.
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	6-dec-1990 (bryanp)
**	    Created
**	28-jan-1991 (bryanp)
**	    MEfill() the page with binary zeroes.
**	05_jun_1992 (kwatts)
**	    MPF change, added the TCB as first parameter.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project:
**	    Removed TCB parameter and replaced it with a page accessor
**	    vector. This allows this routine to be called from recovery.
**	    Removed unused DM1B_PCHAINED page type.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	06-mar-1996 (stial01 for bryanp)
**	    Added page_size argument, passed it to dmpp_format accessor.
**	    If page size is large, use dm0m_lfill to fill the page.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all the page header access as macro for New Page Format
**	    project.
**	    Pass page_size argument to dmpp_put_offset_macro for tuple header
**	    implementation.
**	20-jun-1996 (thaju02)
**	    Use dm0m_lfill when zero filling page with page size greater
**	    than max value for u_i2.
**      04-feb-97 (stial01)
**          Tuple headers are on LEAF and overflow (CHAIN) pages
**          Tuple headers are not on INDEX pages.
**      12-jun-97 (stial01)
**          dm1cxformat() new attribute, key parameters
**      07-jul-98 (stial01)
**          dm1cxformat() Deferred update processing for V2 pages, use page 
**          tranid when page locking, tuple header tranid when row locking
**	23-Jun-2000 (jenjo02)
**	    Don't MEfill page. dmpp_format() will take care of it.
**	22-Jan-2004 (jenjo02)
**	    Caller must supply tidsize.
**	13-dec-04 (inkdo01)
**	    Add support of union between prec & collID.
**      26-sep-05 (stial01)
**          dm1cxformat() Fixed initialization of collID (b115266)
**	25-Apr-2006 (jenjo02)
**	    Add Clustered to prototype. If set, add DMPP_CLUSTERED
**	    to Leaf page status bits, zero atts_count.
*/
DB_STATUS
dm1cxformat(i4 page_type, i4 page_size, DMPP_PAGE *b,
DMPP_ACC_PLV	*loc_plv, 
i4		compression_type,
i4		kperpage,
i4		klen,
DB_ATTS         **atts_array,
i4         	atts_count,
DB_ATTS         **key_array,
i4         	key_count,
DM_PAGENO	page,
i4		indexpagetype,
i4         	spec,
i2		tidsize,
i4		Clustered)
{
    i4	    	local_stat;
    i4         	width; 
    i4         	end, address; 
    i4	    	pos, total; 
    DM_TID      tid; 
    char	buffer[132];
    bool        is_index;	
    DB_STATUS	status;
    i4          page_attcnt;
    DB_ATTS     *atr;           /* Ptr to current attribute */
    DB_ATTS     *key;           /* Ptr to current key */
    i4          i;
    i4          keyno;
    DM1B_ATTR   attdesc;
    i2		TidSize = sizeof(DM_TID);
    i2		abs_type;

    if (DMZ_AM_MACRO(11))
    {
	TRformat(print, 0, buffer, sizeof(buffer) - 1,
		"%8* dm1cx: Formatting new page %d as a %w page",
		page, "INDEX,LEAF,DATA,OVERFLOW,CHAINED,SPRIG", indexpagetype);
    }

    switch (indexpagetype)
    {
	case DM1B_PINDEX:
	    local_stat = DMPP_INDEX; 
	    break; 

	case DM1B_PSPRIG:
	    local_stat = (DMPP_INDEX | DMPP_SPRIG); 
	    break; 

	case DM1B_PLEAF:
	    local_stat = DMPP_LEAF; 

	    /*
	    ** Note Clustering only in Leaf.
	    **
	    ** Clustered Leaf pages don't store
	    ** attribute information.
	    */
	    if ( Clustered )
	    {
		local_stat |= DMPP_CLUSTERED;
		atts_count = 0;
	    }
	    /*
	    ** All but LEAF pages have DM_TID-size tids;
	    ** LEAF pages can have either DM_TID or DM_TID8
	    ** TID8 currently used only in Global SI
	    ** LEAF pages.
	    */
	    TidSize = tidsize;
	    break; 

	case DM1B_PDATA:
	    local_stat = (DMPP_DATA|DMPP_MODIFY); 
	    break; 

	case DM1B_POVFLO:
	    local_stat = DMPP_CHAIN; 
	    break; 

	default:
	    /*
	    ** FIX_ME: should ule_format a message here...
	    */
	    return (E_DB_ERROR);
	    break;
    }

    /* Now go ahead and zero-fill and format the page */
    (*loc_plv->dmpp_format)(page_type, page_size, (DMPP_PAGE *)b, 
	    page, local_stat, DM1C_ZERO_FILL);

    /* Initialize the log address to 0 when creating the page, this is
    ** guaranteed to be less than all possible log addresses. Redo recovery
    ** is dependant on this -- fix for bug 4780.
    **
    ** jenjo02:
    ** dmpp_format() zero-filled the page, hence the log address.
    **
    log_addr.lsn_high = 0;
    log_addr.lsn_low  = 0;
    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, b, log_addr); 
    **
    */

    /* If this call was for a data page we are done */

    if (indexpagetype == DM1B_PDATA)
    {
	/* dmpp_format() zero-filled the page, hence page_main/page_ovfl
	DM1B_VPT_SET_PAGE_MAIN_MACRO(page_type, b, NULL); 
	DM1B_VPT_SET_PAGE_OVFL_MACRO(page_type, b, NULL); 
	*/
	return(E_DB_OK);
    }

    DM1B_VPT_SET_BT_ATTCNT(page_type, indexpagetype, b, atts_count);

    /*
    ** Attribute info is kept only on non-Clustered BTREE leaf pages with new page format 
    ** Attribute information is not needed, not kept for RTREE leaf pages
    ** Attribute information must be put on the page before we allocate
    ** the range entries
    **
    ** Note keys are always a prefix of the attributes (except tidp)
    ** 
    */
    if ((page_attcnt = DM1B_VPT_GET_BT_ATTCNT(page_type, b)) != 0)
    {
	keyno = 1;
	key = *(key_array++);
	for (i = 1; i <= page_attcnt; i++)
	{
	    atr = *(atts_array++);
	    attdesc.type = atr->type;
	    attdesc.len = atr->length;
	    abs_type = abs(atr->type);
	    if (abs_type == DB_CHA_TYPE || abs_type == DB_CHR_TYPE
		|| abs_type == DB_VCH_TYPE || abs_type == DB_TXT_TYPE
		|| abs_type == DB_NCHR_TYPE || abs_type == DB_NVCHR_TYPE)
		attdesc.u.collID = atr->collID;
	    else attdesc.u.prec = atr->precision;
	    if ((keyno <= key_count) &&
		!STcompare(atr->attnmstr, key->attnmstr)) 
	    {
		attdesc.key = keyno++;
		key = *(key_array++);
	    }
	    else
		attdesc.key = 0;

	    DM1B_VPT_SET_ATTR(page_type, page_size, b, i, &attdesc);
	}
    }

    /*
    ** Set the size of the TID entries on this page.
    ** While currently of interest only on LEAF pages,
    ** we'll set it on all pages. LEAF pages will be
    ** either TID8 or TID; all others will be TID.
    */
    if ( TidSize != sizeof(DM_TID) && TidSize != sizeof(DM_TID8) )
    {
	if (DMZ_AM_MACRO(11))
	    TRdisplay("%@ dm1cxformat: bad TID size %d, setting to %d\n",
		tidsize, sizeof(DM_TID));
	TidSize = sizeof(DM_TID);
    }
    DM1B_VPT_SET_BT_TIDSZ_MACRO(page_type, b, TidSize);

    /*
    ** If index compression is not in use, go ahead and format the all the
    ** line table entries now. Add in the extra line table entries for the
    ** high and low range.
    */
    is_index = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, b) & DMPP_INDEX) != 0);
    if (compression_type == DM1CX_UNCOMPRESSED)
    {
	total = kperpage + DM1B_OFFSEQ; 
	end = (DM_VPT_SIZEOF_LINEID_MACRO(page_type) * total) + 
		DM1B_VPT_OVER(page_type); 
	width = (klen) + TidSize; 
        if ( !is_index )
            width += Dmpp_pgtype[page_type].dmpp_tuphdr_size;
	tid.tid_tid.tid_page = 0; 
	tid.tid_tid.tid_line = 0; 

#ifdef xDEBUG
	{
	    i4        tmp;

	    tmp = dm1cxkperpage(page_type, page_size, DMF_T_VERSION, spec,
		key_count, klen, indexpagetype, tidsize, Clustered);
	    if (tmp != kperpage)
		TRdisplay("format kperpage error %d %d at %@\n", kperpage, tmp);
	}
#endif

	/* Set pointers in line table. */

	for (pos=0L, address = end; pos < total; pos++, address+=width)
	{
	    dmpp_vpt_put_offset_macro(page_type,
			       DM1B_VPT_BT_SEQUENCE_MACRO(page_type, b), 
			      (i4) pos, address); 
	    dmpp_vpt_clear_flags_macro(page_type,
			       DM1B_VPT_BT_SEQUENCE_MACRO(page_type, b), pos);
	    dm1bvpt_kidput_macro(page_type, b, pos-DM1B_OFFSEQ, &tid, (i4)0);   
	}
    }
    else
    {
	/*
	** Allocate space for the LRANGE and RRANGE entries here. Just allocate
	** enough space for the TIDP for now -- the actual key must be provided
	** later through a dm1cxreplace() call.
	*/
	DM1B_VPT_SET_BT_KIDS_MACRO(page_type, b, -2);
	status = dm1cxallocate(page_type, page_size, b, 
			DM1C_DIRECT, compression_type, (DB_TRAN_ID *)0,
			(i4)0, DM1B_LRANGE, TidSize );
	if (status != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E0_BAD_INDEX_ALLOC, (DMP_RCB *)0, b,
			    page_type, page_size, DM1B_LRANGE );
	    return (status);
	}
	status = dm1cxallocate(page_type, page_size, b,
			DM1C_DIRECT, compression_type, (DB_TRAN_ID *)0,
			(i4)0, DM1B_RRANGE, TidSize );
	if (status != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E0_BAD_INDEX_ALLOC, (DMP_RCB *)0, b,
			    page_type, page_size, DM1B_RRANGE);
	    return (status);
	}
    }

    DM1B_VPT_SET_PAGE_MAIN_MACRO(page_type, b, 0);
    DM1B_VPT_SET_PAGE_OVFL_MACRO(page_type, b, 0);
    DM1B_VPT_SET_BT_DATA_MACRO(page_type, b, 0);
    DM1B_VPT_CLR_PAGE_STAT_MACRO(page_type, b, DMPP_ASSOC);
    DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, b, DMPP_MODIFY);

    return (E_DB_OK);
}

/*
** Name: dm1cxget		- Get an uncompressed entry from a page
**
** Description:
**	This routine gets an uncompressed entry from a index page. The entry
**	is specified by lineid. It is assumed that the lineid is valid.
**
**	A pointer is returned that points to the requested entry. If the
**	index is compressed, then the entry is uncompressed into the buffer
**	provided.
**
**	The pointer passed in must originally point to a buffer into which
**	the entry can be copied (this buffer will be used if the entry must
**	be uncompressed, or otherwise massaged before being returned). On
**	return, the pointer will point to the entry. The pointer may be reset
**	to point directly to the entry on the fixed index page.
**
**	On input, entry_size must be set to the row length as defined in the
**	relation description. If rows are uncompressed, this same length is
**	returned in entry_size. If rows are compressed, then the process of
**	uncompressing the row re-calculates the row length; this length is
**	compared with the length you passed in, and we complain if they do not
**	match.
**
** Inputs:
**	t			    TCB for access to accessors.
**	atts_array		    Pointer to a list of pointers to attribute
**				    descriptors.
**	atts_count		    Count of number of descriptors in array
**	child			    Which record to get
**	b			    index page to get entry from
**	entry			    Pointer to pointer to buffer
**	tidp			    Address of tidp, if tidp is desired
**				    0 if tidp is not desired
**	entry_size		    Set to size of entry from relation info.
**	compression_type	    Value indicate whether index compression
**				    is in use. Legal values are:
**					DM1CX_UNCOMPRESSED
**					DM1CX_COMPRESSED
**
** Outputs:
**	entry			    Filled in with or set to point to entry
**	tidp			    If non-zero, filled in with entry's tidp
**	partnop			    option pointer to where to return
**				    partition number from Global SI
**				    leaf entry.
**	entry_size		    Size of the returned entry. If the index
**				    uses compression, we ensure that the result
**				    size equals the defined size.
**
** Returns:
**	DB_STATUS
**
** History:
**	3-dec-1990 (bryanp)
**	    Created.
**	9-jan-1991 (bryanp)
**	    Made entry_size an input as well as an output and added code to
**	    verify that correct length results from uncompression.
**	22-jan-1991 (bryanp)
**	    Changed atts_array from (DB_ATTS *) to (DB_ATTS **). Call
**	    dm1cpuncomp_rec rather than dm1c_uncomp_rec.
**	05_jun_1992 (kwatts)
**	    MPF change, added the TCB as first parameter. Call accessor for
**	    uncompression.
**	28-oct-1992 (jnash)
**	    Reduced logging project.  Add unused params to dmppxuncompress.
**	06-may-1996 (nanpr01)
**	    Change all page header access as macro for New Page Format project.
**      03-june-1996 (stial01)
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**	15-jul-1996 (ramra01)
**	    Uncompress add row version for alter table project.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          dm1cxget() When (page_type != TCB_PG_V1):
**             Return E_DB_WARN for deleted leaf entries
**      04-feb-97 (stial01)
**          Tuple headers are on LEAF and overflow (CHAIN) pages
**          Tuple headers are not on INDEX pages.
**      12-jun-97 (stial01)
**          Pass tlv to dm1cxget instead of tcb.
**      07-jul-98 (stial01)
**          dm1cxget() Always return tranid of leaf page entry.
**      08-nov-1999 (nanpr01)
**          Do not copy the entire tuple buffer. Just copy the 8byte
**	    transaction id as required.
**      18-nov-1999 (nanpr01)
**          Do copy the entire tuple buffer and not just copy the 8byte
**	    transaction id as required. This will eliminate the need of 
**	    copy of tuple buffer in dm1b_isnew function.
**	22-Jan-2004 (jenjo02)
**	    Added (optional) *partnop (partition number) output parm.
**	13-Jun-2006 (jenjo02)
**	    The maximum size of a Leaf is now DM1B_MAXLEAFLEN,
**	    not DM1B_KEYLENGTH.
**	25-Aug-2009 (kschendel) 121804
**	    Fake out the ADF-cb parameter to xuncompress.  The real fix
**	    involves adding it to the dm1cxget parameter list, which is
**	    out of scope for now.
**	21-Oct-2009 (kschendel) SIR 122739
**	    Integrate new rowaccessor scheme, which includes revising the
**	    call so that the ADF-CB is passed in.
*/
DB_STATUS
dm1cxget(i4 page_type, i4 page_size, DMPP_PAGE *b,
	    DMP_ROWACCESS *rac, 
	    i4 child,
	    char **entry, DM_TID *tidp, i4 *partnop,
	    i4 *entry_size,
	    u_i4 *row_low_tran, u_i2 *row_lg_id, ADF_CB *adfcb)

{
    char	    *key_ptr;
    i4	    uncompress_size;
    DB_STATUS	    local_status;
    DB_STATUS       status;
    char	    buffer[132];
    i4         page_stat;
    bool            is_index;
    char	    *tup_hdr;

    page_stat = DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, b);
    is_index = ((page_stat & DMPP_INDEX) != 0);
    if (DMZ_AM_MACRO(11))
    {
	TRformat(print, 0, buffer, sizeof(buffer) - 1,
		"%8* dm1cx: Retrieving entry %d from page %d",
		child, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b));
    }

    if (DMZ_AM_MACRO(12))
    {
	if (b == (DMPP_PAGE *)0 || entry_size == (i4 *)0 ||
	    (*entry_size) <= 0 || (*entry_size) > DM1B_MAXLEAFLEN ||
	    entry == (char **)0 || (*entry) == (char *)0 ||
	    (i4)child < DM1B_LRANGE || 
	    (i4)child >= DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b))
	{
	    TRdisplay("dm1cxget: b = %p child = %d entry=%p entry_size=%x\n",
			b, (i4)child, entry, entry_size);
	    if (b)
		TRdisplay("       : b->bt_kids = %d\n", 
				DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b));
	    if (entry_size)
		TRdisplay("       : *entry_size = %d\n", *entry_size);
	    if (entry)
		TRdisplay("       : *entry = %p\n", *entry);
	    return (E_DB_ERROR);
	}
    }

    status = E_DB_OK;

    key_ptr = (char *)dm1bvpt_keyof_macro(page_type, b, (i4)child);
    if (tidp)
    {
	dm1bvpt_kidget_macro(page_type, b, (i4)child, tidp, partnop);

	/*
	** The "TID" of a Clustered leaf entry is always its BID,
	** unless this entry has been reserved (0,DM_TIDEOF), or
	** it's a Range entry.
	*/
	if ( page_stat & DMPP_CLUSTERED &&
	     child != DM1B_LRANGE && child != DM1B_RRANGE &&
	     tidp->tid_tid.tid_page != 0  &&
	     tidp->tid_tid.tid_line != DM_TIDEOF )
	{  
	    tidp->tid_tid.tid_page = DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b);
	    tidp->tid_tid.tid_line = child;
	}
    }

    if ((page_type != TCB_PG_V1) && (!is_index))
    {
	/* Test for deleted row */
	if (dmpp_vpt_test_free_macro(page_type,  DM1B_V2_BT_SEQUENCE_MACRO(b),
		(i4)child + DM1B_OFFSEQ))
	{
	    status = E_DB_WARN; /* TID points to deleted record */
	}

	/* Sometimes return low tranid of leaf entries */
	if ( row_low_tran || row_lg_id )
	{
	    tup_hdr = dm1b_vpt_entry_macro(page_type, b, (i4)child);

	    if ( row_low_tran )
	    {
		MECOPY_CONST_4_MACRO(
		    (PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_lowtran_offset,
		    row_low_tran);
	    }

	    /* Ditto for lg_id of leaf entries */
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
	}
    }
    else 
    {
	if ( row_low_tran )
	    *row_low_tran = 0;
	if ( row_lg_id )
	    *row_lg_id = 0;
    }

    if (rac->compression_type != TCB_C_NONE)
    {
	/*
	** Index compression is in use. Uncompress the record into the
	** caller-provided buffer.
	*/
	local_status = (*rac->dmpp_uncompress)(rac, 
			key_ptr, *entry, (*entry_size), &uncompress_size, 
			NULL, (i4)0, adfcb);
	if (local_status != E_DB_OK)
	    return (local_status);

	if (uncompress_size != *entry_size)
	{
	    /* this TRdisplay should become a ule_format...*/
	    TRdisplay("dm1cxget: Entry %d on page %d was length %d, not %d\n",
			(i4)child, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b),
			uncompress_size, *entry_size);
	    return (E_DB_ERROR);
	}
	return (status);
    }
    else
    {
	/*
	** Index compression is not in use. Simply return record & TIDP. Note
	** that in this case we don't worry about record size, just leaving it
	** at its input value.
	*/
	*entry = key_ptr;

	return (status);
    }
}

/*
** Name: dm1cxhas_room		- Does this page have room for this entry?
**
** Description:
**	This routine allows the caller to determine whether this page has
**	room for the indicated entry. For non-compressed index pages, we must
**	merely check bt_kids against tcb_kperpage: if bt_kids is less, we have
**	room.
**
**	For compressed index pages, we must examine both bt_kids AND the
**	available space on the page. The page must have enough room for a new
**	line table entry AND the data portion of the entry.
**
**	For a compressed Btree index, we must compute how much space is
**	available on the page and see if the space is sufficient. In addition,
**	if the page is a leaf page, we have to reserve some extrace space
**	because of the complexity of splits. We must declare this page full
**	unless it has enough room so that after we add an entry there is still
**	enough room to expand the low and high range keys to their maximum
**	worst-case size.
**
**	This is because during split processing, all of the entries which are
**	on the page being split may end up together after the split, and there
**	must still be enough room on the page after the split to allow us to
**	replace the low and high range keys on the page (in the "left-hand"
**	page of a split, we replace the high range key; in the "right-hand"
**	page of a split, we replace the low range key). Thus a page must always
**	have enough room on it that we can replace one of the range keys on the
**	page without overflowing it, and in the worst case each range key
**	expands from a zero-length key (plus or minus infinity are stored as
**	zero-length keys) to a maximum length key.
**
**	Since a fan-out of at least 2 is required (a btree with a fanout of 1
**	is of no use), we always allow at least two records to be added to
**	each page. After a page has at least two entries, we perform the more
**	sophisticated calculations (taking fill factor, etc. into account) to
**	determine whether the page has room.
**
** Inputs:
**	b			- the index page in question
**	compression_type	- Value indicating the index compression type:
**				    DM1CX_UNCOMPRESSED
**				    DM1CX_COMPRESSED
**	fill_factor		- percentage fill factor for this type of page
**				    (integer value between 1 and 100).
**	kperpage		- max entries on this page, if non-compressed
**	entry_size		- size of the maximum entry that might go on the
**				    page, INCLUDING THE TIDP SIZE
**	page_size		- page size of this table.
**
** Outputs:
**	None
**
** Returns:
**	TRUE			- this page has room for this entry
**	FALSE			- this page does NOT have room for this entry
**
** History:
**	12-dec-1990 (bryanp)
**	    Created.
**	5-feb-1991 (bryanp)
**	    Changed algorithm to require TWO worst-case entry sizes be reserved
**	    so that split processing could safely depend on being able to
**	    replace the low and high range keys following a split.
**	8-feb-1991 (bryanp)
**	    Changed algorithm again after thinking about splits some more.
**	4-mar-1991 (bryanp)
**	    Added fill_factor argument, and incorporated fill factor into the
**	    arithmetic.
**	13-mar-1991 (bryanp)
**	    A page with 0 or 1 kids on it ALWAYS has room.
**	19-mar-1991 (bryanp)
**	    Use DM1B_FREE, not DM_PG_SIZE, to allow for page header overhead.
**	06-mar-1996 (stial01 for bryanp)
**	    Add page_size argument to allow for tables with varying page sizes.
**	06-may-1996 (nanpr01)
**	    Change all the page header access as macro for New Page Format
**	    project.
**      04-feb-97 (stial01)
**          Tuple headers are on LEAF and overflow (CHAIN) pages
**          Tuple headers are not on INDEX pages.
**      01-dec-1999 (gupsh01)
**          Added check for max allowable rows on a compressed keys,
**          non-index pages. This is determined by the bitsize set for
**          tid_line in tid_tid structure. (bug #b99368 )
*/
bool
dm1cxhas_room(i4 page_type, i4 page_size, DMPP_PAGE *b,
i4 compression_type,
i4 fill_factor,
i4 kperpage,
i4 entry_size)
{
    i4	avail;
    bool	has_room;
    char	buffer[132];
    i4	additional_room;
    i4	lrange_len;
    i4	rrange_len;
    bool        is_index;

    is_index = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, b) & DMPP_INDEX) != 0);
    if (!is_index)
	entry_size += Dmpp_pgtype[page_type].dmpp_tuphdr_size;

    if (compression_type == DM1CX_UNCOMPRESSED)
    {
#ifdef xDEBUG
	{
	    DM_LINEID       *lp;
	    char            *lrange_addr;
	    i4         alloc_kids;
	    i4         size_lp;

	    /* This assumes LRANGE entry is first entry in data space */
	    lrange_addr = dm1b_vpt_entry_macro(page_type, b, (i4)DM1B_LRANGE);
	    lp = DM1B_VPT_BT_SEQUENCE_MACRO(page_type, b);
	    size_lp = (char *)lrange_addr - (char *)lp;
	    alloc_kids = size_lp / (DM_VPT_SIZEOF_LINEID_MACRO(page_type));

	    if (alloc_kids != kperpage + DM1B_OFFSEQ)
		TRdisplay("kperpage error %d %d at %@\n", kperpage, alloc_kids);
	}
#endif

	if (DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b) < 2)
	    has_room = TRUE;
	else
	    has_room = (DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b) < (kperpage * fill_factor / 100));

	if (DMZ_AM_MACRO(11))
	{
	    TRformat(print, 0, buffer, sizeof(buffer) - 1,
		"%8* dm1cxhas_room: Page %d %s (kids=%d,perpage=%d,fill=%d)",
		DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b),
		(has_room ? "has room" : "IS FULL"),
		DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b), kperpage, fill_factor);
	}

	return (has_room);
    }
    else if (!(is_index) && DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b) >= DM_TIDEOF)
    {
        has_room = FALSE;
        return (has_room);
    }
    else
    {
	/*
	** The available space is the difference between the address of the
	** start of the first entry on the page and the address of the line
	** table entry which would be one past the new line table entry we'd
	** add for this record.
	*/

	if (DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b) < 2)
	{
	    has_room = TRUE;
	}
	else
	{
	    avail = ((char *)b + lowest_offset(page_type, page_size, b)) -
		(char *)(DM1B_VPT_GET_ADDR_BT_SEQUENCE_MACRO(page_type, b, 
		    DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b) + DM1B_OFFSEQ + 1));

	    if (avail > entry_size &&
		avail >= (DM1B_VPT_FREE(page_type, page_size) -
		    (fill_factor * DM1B_VPT_FREE(page_type, page_size) / 100)))
	    {
		has_room = TRUE;
	    }
	    else
	    {
		has_room = FALSE;
	    }

	    additional_room = 0;

	    if ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, b) & DMPP_INDEX) == 0)
	    {
		/*
		** we must also ensure that there's sufficient space for the
		** range keys to expand to their worst-case size
		*/
		if (has_room)
		{
		    dm1cxlength(page_type, page_size, b, DM1B_LRANGE, &lrange_len);
		    dm1cxlength(page_type, page_size, b, DM1B_RRANGE, &rrange_len);

		    additional_room += (entry_size - lrange_len);
		    additional_room += (entry_size - rrange_len);

		    if (avail > (entry_size + additional_room) &&
			(avail-additional_room) >= 
			(DM1B_VPT_FREE(page_type, page_size) -
		    (fill_factor * DM1B_VPT_FREE(page_type, page_size) / 100)))
		    {
			has_room = TRUE;
		    }
		    else
		    {
			has_room = FALSE;
		    }
		}
	    }
	}

	if (DMZ_AM_MACRO(11))
	{
	    TRformat(print, 0, buffer, sizeof(buffer) - 1,
		    "%8* dm1cxhas_room: Page %d %s \
(kids=%d, low=%d, avail=%d, entry_size=%d, fill=%d, addl=%d)",
		    DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b),
		    (has_room ? "has room" : "IS FULL"),
		    DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b), 
		    lowest_offset(page_type, page_size, b), avail, entry_size,
		    fill_factor, additional_room);
	}

	return (has_room);
    }
}

/*
** Name: dm1cxjoinable		- Are these two pages joinable?
**
** Description:
**	This routine is called by "modify to merge" when it wants to decide
**	whether two pages can be combined within the limits set by the fill
**	factor.
**
**	If index compression is NOT in use, this decision is made by summing
**	the number of entries on each page (NOT counting the LRANGE & RRANGE
**	entries) and checking this against the tcb_kperpage maximum adjusted
**	by the fill factor.
**
**	If index compression IS in use, this decision is made by examining the
**	space usage on each page, and checking this against DM_PG_SIZE adjusted
**	by the fill factor.
**
** Inputs:
**	r			    Record Control Block
**	compression_type	    Value indicating compression type:
**					DM1CX_UNCOMPRESSED
**					DM1CX_COMPRESSED
**	curpage			    current page we're working with
**	sibpage			    page we're considering joining to curpage
**	fill			    The fill factor to use (percentage)
**					(integer value between 1 and 100)
**
** Outputs:
**	None
**
** Returns:
**	TRUE			    These two pages can be joined
**	FALSE			    These two pages CANNOT be joined
**
** History:
**	18-dec-1990 (bryanp)
**	    Created.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
**	    Pass page_size argument to DM1B_VPT_FREE macro.
*/
bool
dm1cxjoinable(DMP_RCB *r, i4 compression_type,
		DMPP_PAGE *curpage, DMPP_PAGE *sibpage,
		i4 page_size,
		i4 fill)
{
    i4	avail, cur_usage, sib_usage;
    bool        is_index;
    i4     kperpage;
    i4     page_type = r->rcb_tcb_ptr->tcb_rel.relpgtype;

    is_index = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, sibpage) & DMPP_INDEX) != 0);
    if (is_index)
	kperpage = r->rcb_tcb_ptr->tcb_kperpage;
    else
	kperpage = r->rcb_tcb_ptr->tcb_kperleaf;

    if (compression_type == DM1CX_UNCOMPRESSED)
    {
	if ((DM1B_VPT_GET_BT_KIDS_MACRO(page_type, curpage) + 
	     DM1B_VPT_GET_BT_KIDS_MACRO(page_type, sibpage)) >
		(fill * kperpage / 100))
	{
	    return (FALSE);
	}
	else
	{
	    return (TRUE);
	}
    }
    else
    {
	/*
	** Calculate space usage of each page, not counting header portion.
	**
	** Sum the space usages and compare this against
	**	(fill * DM_PG_SIZE / 100)
	**  or  (fill * DM1B_VPT_FREE / 100)
	**
	** ... and what do we do about the LRANGE and RRANGE...
	**
	** and how could we extend this to model page-level compression?
	*/
	avail = ((char *)curpage + lowest_offset(page_type, page_size, curpage)) -
		(char *)(DM1B_VPT_GET_ADDR_BT_SEQUENCE_MACRO(page_type, curpage,
		     DM1B_VPT_GET_BT_KIDS_MACRO(page_type, curpage) + DM1B_OFFSEQ));
	cur_usage = DM1B_VPT_FREE(page_type, page_size) - avail;

	avail = ((char *)sibpage + lowest_offset(page_type, page_size, sibpage)) -
		(char *)(DM1B_VPT_GET_ADDR_BT_SEQUENCE_MACRO(page_type, sibpage,
		     DM1B_VPT_GET_BT_KIDS_MACRO(page_type, sibpage) + DM1B_OFFSEQ));
	sib_usage = DM1B_VPT_FREE(page_type, page_size) - avail;

	if ( (cur_usage + sib_usage) <
		(fill * DM1B_VPT_FREE(page_type, page_size) / 100))
	    return (TRUE);
	else
	    return (FALSE);
    }
}

/*
** Name: dm1cxlength		- Calculates length of index entry
**
** Description:
**	This routine computes the length of a entry specified by LINEID on a
**	page. It is assumed that the entry is valid.
**
**	If index compression is not used, then all entries on the page have
**	the same length. If index compression is used, then each entry on
**	the page may have a different length.
**
**	Note that the entry_size returned includes the length of the entire
**	(key,tid) entry. If you just want the length of the key, subtract
**	TidSize from the entry_size after calling this routine.
**
**	The algorithm used here is identical to the one used by dm1c_reclen:
**	    find the offset to this entry. find the offset to the next entry.
**	    (if there is no next entry, use the page size). Subtract the two.
**	This algorithm requires checking every line table entry on the page,
**	including the DM1B_LRANGE and DM1B_RRANGE entries.
**
**	Currently, this routine does not work properly for uncompressed index
**	pages. You can call it and it does not crash, but it does not return
**	the correct entry length. Instead of calling this routine, the caller
**	should use tcb_klen + TidSize for the length of an uncompressed
**	index page entry.
**
** Inputs:
**	child			LINEID number of the child to get length of
**	b			Btree index page
**	page_size		Page size for this table.
**
** Outputs:
**	entry_size		Pointer to an area to return entry size. This
**				entry size includes the entire (key,tid) length.
**
** Returns:
**	VOID
**
** History:
**	3-dec-1990 (bryanp)
**	    Created.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
**	06-may-1996 (nanpr01)
**	    Change all page header access as macro for New Page Format project.
**      12-jun-97 (stial01)
**          DON'T assume tuple ends at end of page, that is where attr info is.
*/
static VOID
dm1cxlength(i4 page_type, i4 page_size, DMPP_PAGE *b,
    i4 child, i4 *entry_size)
{
    DM_LINEID  	    *lp;
    i4	    	    i,j;
    u_i4	    lineoff;
    u_i4	    nextoff;
    u_i4	    thisoff;


    /* Point to line number table. */

    lp = DM1B_VPT_BT_SEQUENCE_MACRO(page_type, b);

    /* Find offset for t.id */

    lineoff = dmpp_vpt_get_offset_macro(page_type,lp, (i4)child + DM1B_OFFSEQ);

    /* Assume tuple ends at end of page. */

    nextoff = page_size;
    if (page_type != TCB_PG_V1)
	nextoff -= DM1B_VPT_GET_ATTSIZE_MACRO(page_type, b);

    /* Look for the line offset following lineoff. */

    for (j = 0, i = DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b) + DM1B_OFFSEQ; 
	 --i >= 0; j++)
    {
        thisoff = dmpp_vpt_get_offset_macro(page_type, lp, (i4)j);
	if ( thisoff <= lineoff )
            continue;
	if ( thisoff < nextoff )
	    nextoff = thisoff;
    }

    *entry_size = (nextoff - lineoff);

    return;
}

/*
** Name: dm1cx_klen		- Calculates length of index KEY 
** 
**      If index page, the key length = entry length - DM1B_TID_S
**      If leaf page, the key length = entry length - (TidSize + sizeof hdr)
**
** Description:
**	This routine computes the length of a KEY specified by LINEID on a
**	page. It is assumed that the entry is valid.
**
**	If index compression is not used, then all KEYS on the page have
**	the same length. If index compression is used, then each KEY on
**	the page may have a different length.
**
**	The algorithm used here is identical to the one used by dm1c_reclen:
**	    find the offset to this entry. find the offset to the next entry.
**	    (if there is no next entry, use the page size). Subtract the two.
**	This algorithm requires checking every line table entry on the page,
**	including the DM1B_LRANGE and DM1B_RRANGE entries.
**
**	Currently, this routine does not work properly for uncompressed index
**	pages. You can call it and it does not crash, but it does not return
**	the correct entry length. Instead of calling this routine, the caller
**	should use tcb_klen + TidSize for the length of an uncompressed
**	index page entry.
**
** Inputs:
**	child			LINEID number of the child to get length of
**	b			Btree index page
**	page_size		Page size for this table.
**
** Outputs:
**	key_size		Pointer to an area to return key size. This
**				does not include DM1B_TID_S or tuple header size
**
** Returns:
**	VOID
**
** History:
**      1-feb-2001 (stial01)
**          Created from dm1cxlength
**	22-Jan-2004 (jenjo02)
**	    Extract TidSize from page.
*/
VOID
dm1cx_klen(i4 page_type, i4 page_size, DMPP_PAGE *b,
    i4 child, i4 *key_size)
{
    DM_LINEID  	    *lp;
    i4	    i,j;
    u_i4	    lineoff;
    u_i4	    nextoff;
    u_i4	    thisoff;
    i2		    TidSize = DM1B_VPT_GET_BT_TIDSZ_MACRO(page_type, b);


    /* Point to line number table. */

    lp = DM1B_VPT_BT_SEQUENCE_MACRO(page_type, b);

    /* Find offset for t.id */

    lineoff = dmpp_vpt_get_offset_macro(page_type,lp, (i4)child + DM1B_OFFSEQ);

    /* Assume tuple ends at end of page. */

    nextoff = page_size;
    if (page_type != TCB_PG_V1)
	nextoff -= DM1B_VPT_GET_ATTSIZE_MACRO(page_type, b);

    /* Look for the line offset following lineoff. */

    for (j = 0, i = DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b) + DM1B_OFFSEQ; 
	 --i >= 0; j++)
    {
        thisoff = dmpp_vpt_get_offset_macro(page_type, lp, (i4)j);
	if ( thisoff <= lineoff )
	    continue;
	if ( thisoff < nextoff )
	    nextoff = thisoff;
    }

    *key_size = (nextoff - lineoff);
    if (DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, b) & DMPP_INDEX)
	*key_size -= TidSize;
    else
	*key_size -= (TidSize + Dmpp_pgtype[page_type].dmpp_tuphdr_size);

    return;
}

/*
** Name: dm1cxLogErrorFcn	- log an error encountered on an index page
**
** Description:
**	This routine can be called by access method code when a problem is
**	encountered on a Btree index page. It attempts to log as much
**	information as possible about the page and the type of error which
**	occurred so that we can track down the problem.
**
** Inputs:
**	errorno			- the error number to log
**	rcb			- Record Control Block (may be 0)
**	page			- page on which error was encountered (may be 0)
**	child			- position within page where error occurred
**	FileName		- pointer to file name of error source
**	LineNumber		- line number of error source
**
** Outputs:
**	None
**
** Returns:
**	VOID
**
** History:
**	7-jan-1990 (bryanp)
**	    Created.
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	06-may-1996 (nanpr01)
**	    Change all the page header access as macro for New Page Format
**	    project.
**	17-Nov-2008 (jonj)
**	    SIR 120874: Renamed to dm1cxLogErrorFcn(), invoked by 
**	    dm1cxlog_error macro, passing error source information.
*/
VOID
dm1cxLogErrorFcn(i4 errorno, DMP_RCB *rcb, 
		DMPP_PAGE *page, i4 page_type, i4 page_size, i4 child,
		PTR FileName, i4 LineNumber)
{
    DB_DB_NAME		db; 
    DB_OWN_NAME		own;
    DB_TAB_NAME		tab;
    char		*db_name = db.db_db_name;
    char		*tbl_name = tab.db_tab_name;
    char		*own_name = own.db_own_name;
    i4		page_number = -1;
    i4		err_code;
    i4		page_stat = 0;
    i4		kids = -1;
    i4		offset = -1;
    DB_ERROR		dberr;

    /* Populate DB_ERROR with error source information */
    dberr.err_code = errorno;
    dberr.err_data = 0;
    dberr.err_file = FileName;
    dberr.err_line = LineNumber;

    STmove(DB_UNKNOWN_DB, ' ', sizeof(db), db.db_db_name);
    STmove(DB_NOT_TABLE, ' ', sizeof(tab), tab.db_tab_name);
    STmove(DB_UNKNOWN_OWNER, ' ', sizeof(own), own.db_own_name);

    if (rcb)
    {
	if (rcb->rcb_tcb_ptr)
	{
	    tbl_name = (char *) &rcb->rcb_tcb_ptr->tcb_rel.relid;
	    own_name = (char *) &rcb->rcb_tcb_ptr->tcb_rel.relowner;
	    if (rcb->rcb_tcb_ptr->tcb_dcb_ptr)
		db_name = (char *) &rcb->rcb_tcb_ptr->tcb_dcb_ptr->dcb_name;
	}
    }

    if (page)
    {
	page_number = DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, page);
	page_stat   = DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page);
	kids        = DM1B_VPT_GET_BT_KIDS_MACRO(page_type, page);
	if ((i4)child >= DM1B_LRANGE && 
	    (i4)child < DM1B_VPT_GET_BT_KIDS_MACRO(page_type, page))
	    offset = dmpp_vpt_get_offset_macro(page_type,
		DM1B_VPT_BT_SEQUENCE_MACRO(page_type, page), 
		(i4)child + DM1B_OFFSEQ);
    }

    uleFormat(&dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, NULL, 0, NULL, &err_code,
		8,
		sizeof(DB_DB_NAME), db_name,
		sizeof(DB_TAB_NAME), tbl_name,
		sizeof(DB_OWN_NAME), own_name,
		0, page_number,
		0, page_stat,
		0, child,
		0, kids,
		0, offset);

    if (DMZ_AM_MACRO(14))
    {
	/*
	** If both page and RCB are available, try to print the failing
	** page to the DBMS trace log. Perhaps it will help...
	*/
	if (rcb != 0 && page != 0)
	{
	    TRdisplay("dm1cxlog_error: failing page follows:\n");
	    dmd_prindex(rcb, page, (i4)0);
	}
	/*
	** If page is available, hex-dump the page to the DBMS trace log.
	*/
	if (page != 0)
	    page_dump(page_type, page_size, page);
    }

    return;
}

/*
** Name: dm1cxlshift		- shift entries 'left' during join processing
**
** Description:
**	This routine is called by join processing to shift all the entries
**	from the sibling page back to the current page.
**
**	If the index page is uncompressed, all entries on the page are the
**	same size. This size must be passed in, as dm1cxlength() does not work
**	on uncompressed index pages. If the index page is compressed, each
**	entry may be a different size. The passed-in entry_len field is
**	ignored; instead, this routine figures out the length of each entry
**	as it shifts it.
**
** Inputs:
**	compression_type	- Value indicating compression type:
**				    DM1CX_UNCOMPRESSED
**				    DM1CX_COMPRESSED
**	cur			- current page (page being shifted TO)
**	sib			- sibling page (page being shifted FROM)
**	page_size		- page size for this table.
**	entry_len		- size of the (key,tid) pair if compression_type
**				    is DM1CX_UNCOMPRESSED
**
** Outputs:
**	None
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	19-dec-1990 (bryanp)
**	    Created.
**	14-jan-1991 (bryanp)
**	    Added entry_len argument, documented problems with length of
**	    entries on uncompressed index pages.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
**	06-may-1996 (nanpr01 & thaju02)
**	  - Changed all page header access to macro for New Page Format 
**	    project.
**	  - Changed dm1bvpt_kido_macro to dm1b_vpt_entry_macro in MEcopy call. 
**	    Need to copy tuple header along with key, tid pair.
**	12-may-1996 (nanpr01)
**	   Tuple header length is getting added twice.
**      04-feb-97 (stial01)
**          Tuple headers are on LEAF and overflow (CHAIN) pages
**          Tuple headers are not on INDEX pages.
**	12-Jul-2001 (thaju02)
**	    (hanje04) X-Integ of 451698
**	    Upon shifting uncompressed entries from sibling to current
**	    ensure that the free space bit (DM_FREE_SPACE) is cleared.
**	    (B105235/INGSRV1493)
*/
DB_STATUS
dm1cxlshift(i4 page_type, i4 page_size, DMPP_PAGE *cur, DMPP_PAGE *sib,
			    i4 compression_type, i4 entry_len)
{
    i4	    runner;	    /* position we're copying to on curpage */
    i4	    kid;	    /* position we're copying from on sibpage */
    i4	    siblen;	    /* length of the kid we're copying	    */
    DB_STATUS	    ret_val;
    char	    buffer[132];
    bool            is_index;
 
    is_index = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, cur) & DMPP_INDEX) != 0);
    if (!is_index)
      entry_len += Dmpp_pgtype[page_type].dmpp_tuphdr_size;

    if (DMZ_AM_MACRO(11))
    {
	TRformat(print, 0, buffer, sizeof(buffer) - 1,
		"%8* dm1cx: Left-shifting from page %d to page %d",
		DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, sib), 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, cur));
    }

    if (compression_type == DM1CX_UNCOMPRESSED)
    {
	/*
	** Shifting uncompressed entries is easy, becuase all entries have
	** the same length and the page offsets are all pre-formatted.
	** Find the length of the entry, copy each entry, and then adjust
	** the bt_kids value.
	*/
	runner = DM1B_VPT_GET_BT_KIDS_MACRO(page_type, cur);

	/*
	** dm1cxlength doesn't work on uncompressed pages.
	**
	** if (sib->bt_kids > 0)
	**    dm1cxlength( 0, sib, &siblen );
	*/
	siblen = entry_len;

	for (kid = 0; kid < DM1B_VPT_GET_BT_KIDS_MACRO(page_type, sib); 
	     kid++, runner++)
	{
	    MEcopy((char *)dm1b_vpt_entry_macro(page_type, sib, kid), siblen,
		       (char *)dm1b_vpt_entry_macro(page_type, cur, runner));
	    dmpp_vpt_clear_free_macro(page_type, 
			DM1B_VPT_BT_SEQUENCE_MACRO(page_type, cur),
			(i4)runner + DM1B_OFFSEQ);
	}

	DM1B_VPT_SET_BT_KIDS_MACRO(page_type, cur, 
				(DM1B_VPT_GET_BT_KIDS_MACRO(page_type, cur) + 
			 	 DM1B_VPT_GET_BT_KIDS_MACRO(page_type, sib))); 

	ret_val = E_DB_OK;
    }
    else
    {
	i4	alloc_len;
	/*
	** Shifting compressed entries is harder, because we must dynamically
	** manage the space on the page. For each entry on the sibling page,
	** we get its length, allocate that much space on the current page,
	** and copy it over. We try to avoid uncompressing each entry as we
	** copy it, for performance reasons, although that decision will need
	** to be re-thought when we try to implement page-level compression.
	*/
	runner = DM1B_VPT_GET_BT_KIDS_MACRO(page_type, cur);
	ret_val = E_DB_OK;

	for (kid = 0; kid < DM1B_VPT_GET_BT_KIDS_MACRO(page_type, sib); 
	     kid++, runner++)
	{
	    dm1cxlength( page_type, page_size, sib, kid, &siblen );

	    /* Adjust the siblen before passing to dm1cxallocate */
	    if (is_index)
		alloc_len = siblen;
	    else
		alloc_len = siblen - Dmpp_pgtype[page_type].dmpp_tuphdr_size;
	    
	    ret_val = dm1cxallocate(page_type, page_size, cur,
				    DM1C_DIRECT, compression_type,
				    (DB_TRAN_ID *)0, (i4)0,
				    DM1B_VPT_GET_BT_KIDS_MACRO(page_type, cur), 
				    alloc_len );
	    if (ret_val)
	    {
		/* should log something here... */
		dm1cxlog_error( E_DM93E0_BAD_INDEX_ALLOC, (DMP_RCB *)0, cur,
			page_type, page_size,
			DM1B_VPT_GET_BT_KIDS_MACRO(page_type, cur));
		dm1cxlog_error( E_DM93E0_BAD_INDEX_ALLOC, (DMP_RCB *)0, sib,
			page_type, page_size, kid);
		return (ret_val);
	    }
	    MEcopy((char *)dm1b_vpt_entry_macro(page_type, sib, kid), siblen,
		       (char *)dm1b_vpt_entry_macro(page_type, cur, runner));
	}
	/*
	** note that dm1cxallocate adjusted the cur->bt_kids value on each
	** allocation, so we don't need to adjust it here.
	*/
    }

    return (ret_val);
}

/*
** Name: dm1cxmax_length	- Calculate max length of compressed entry
**
** Description:
**	The top down splitting algorithm which we currently use needs to be
**	able to look at a page and determine if the page is full. If so, the
**	page will be split. Unfortunately, at the time that it examines the
**	page it does not know the length of the entry which it might have to
**	add to the page if a split of a child becomes necessary.
**
**	Therefore, we created this routine which returns the maximum possible
**	length of a compressed entry in this table, plus the overhead required
**	by the tid portion of the (key,tid) pair.
**
** Inputs:
**	r			- Record Control Block for this table
**
** Outputs:
**	None
**
** Returns:
**	maximum length of a compressed (key,tid) pair
**
** History:
**	11-jan-1991 (bryanp)
**	    created.
**	06-may-1996 (nanpr01)
**	    Account for tuple headers for New Page Format project.
**      04-feb-97 (stial01)
**          Tuple headers are on LEAF and overflow (CHAIN) pages
**          Tuple headers are not on INDEX pages.
**	14-nov-1996 (nanpr01)
**	    Tuple header is being added twice once in here and other
**	    in dm1cxhas_room causing page to be not full.
**	22-Jan-2004 (jenjo02)
**	    Extract TidSize from page;
*/
i4
dm1cxmax_length(DMP_RCB *r, DMPP_PAGE *b)
{
    DMP_TCB     *t = r->rcb_tcb_ptr;
    i2		TidSize;

    TidSize = DM1B_VPT_GET_BT_TIDSZ_MACRO(t->tcb_rel.relpgtype, b);
 
    if (DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, b) & DMPP_INDEX)
	return ((t->tcb_ixklen + t->tcb_index_rac.worstcase_expansion + TidSize));
    else
	return ((t->tcb_klen + t->tcb_leaf_rac.worstcase_expansion + TidSize));
}

/*
** Name: dm1cxmidpoint		- Find the midpoint of the page (for splitting)
**
** Description:
**	This routine calculates the midpoint of the page, for use in splitting.
**
**	If index compression is not in use, this is simple: we just divide the
**	number of entries on the page in half (since each entry is the same
**	size).
**
**	If index compression IS in use, things are more complicated. We wish
**	to find a dividing spot such that the sum of the sizes of the entries
**	to the 'left' of the dividing spot is roughly equal to the sum of the
**	sizes of the entries to the 'right' of the dividing spot. We proceed
**	as follows:
**
**	    1) Determine the amount of space which is 'half' the amount of
**		used space on the page.
**	    2) Work from the left, and keep adding entries as long as the
**		accumulated space is less than half the amount.
**	    3) Pick either the number of entries from step 2, or one more entry,
**		depending on which answer gives a result closer to half the
**		amount of used space.
**
** Inputs:
**	b			- page to work with
**	compression_type	- index compression information:
**				    DM1CX_UNCOMPRESSED
**				    DM1CX_COMPRESSED
**	page_size		- page size of this table.
**
** Outputs:
**	split_point		- position on the page where the split should
**				  take place. All entries, starting with this
**				  one, should be shifted right to the sibling.
**
** Returns:
**	VOID
**
** History:
**	18-mar-1991 (bryanp)
**	    Created.
**	06-mar-1996 (stial01 for bryanp)
**	    Added page_size argument to allow for varying length pages.
**	06-may-1996 (nanpr01)
**	    Changed all the page header access as macro for New Page Format
**	    project.
*/
VOID
dm1cxmidpoint(i4 page_type, i4 page_size, DMPP_PAGE *b,
i4 compression_type,
i4 *split_point)
{
    i4	avail;
    i4	half_used;
    i4	lrange_len;
    i4	rrange_len;
    i4	lesser_amount;
    i4	greater_amount;
    i4	pos;
    i4	amount;
    i4	entry_size;
    char	buffer [132];

    if (compression_type == DM1CX_UNCOMPRESSED)
    {
	*split_point = DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b) / 2;
    }
    else
    {
	/*
	** Yuck.
	**  1) Determine the amount of space which is 'half' the amount of
	**	used space on the page.
	**  2) Work from the left, and keep adding entries as long as the
	**  	accumulated space is less than half the amount.
	**  3) Pick either the number of entries from step 2, or one more entry,
	**	depending on which answer gives a result closer to half the
	**	amount of used space.
	*/
	
	avail = ((char *)b + lowest_offset(page_type, page_size, b)) -
			(char *)(DM1B_VPT_GET_ADDR_BT_SEQUENCE_MACRO(page_type, b,
			    DM1B_VPT_GET_BT_KIDS_MACRO(page_type,b) + DM1B_OFFSEQ));
	dm1cxlength(page_type, page_size, b, DM1B_LRANGE, &lrange_len);
	dm1cxlength(page_type, page_size, b, DM1B_RRANGE, &rrange_len);

	half_used = (DM1B_VPT_FREE(page_type, page_size) - 
			(avail + lrange_len + rrange_len)) / 2;

	if (DMZ_AM_MACRO(11))
	{
	    TRformat(print, 0, buffer, sizeof(buffer) - 1,
		    "%8* dm1cx: Page %d has %d avail, ranges %d,%d, so half=%d",
		    DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b), avail, lrange_len, 
		    rrange_len, half_used);
	}

	amount = 0;
	for (pos = 0; pos < DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b); pos++)
	{
	    dm1cxlength(page_type, page_size, b, pos, &entry_size);

	    amount += entry_size;
	    if (amount > half_used)
		break;
	}
	if (pos >= DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b))
	{
	    /*
	    ** Bug in the algorithm.
	    */
	    TRdisplay("dm1cxmidpoint: couldn't find midpoint of page %d\n",
			DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b));
	    *split_point = DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b) / 2;
	}
	else
	{
	    /*
	    ** Assert:
	    **	amount		    -- sum of the sizes of entries 0 .. pos,
	    **			       inclusive. Greater than half_used.
	    **	pos		    -- points at the last entry summed
	    **	entry_size	    -- size of entry 'pos'.
	    **	(amount-entry_size) -- less than half_used
	    **	half_used	    -- amount of space we'd like to free as
	    **			       part of the split.
	    **
	    ** The correct splitpoint is either pos or (pos+1). It is (pos+1) if
	    ** the amount of space including entry pos is closer to half_used
	    ** than the amount of space not including entry pos. It is pos
	    ** otherwise.
	    */
	    if ((amount - half_used) < (half_used - (amount - entry_size)))
		*split_point = pos + 1;
	    else
		*split_point = pos;

	    /*
	    ** Note we are here because there is not enough space on this
	    ** page so make sure we actually choose a split point that
	    ** will result in some keys moving!
	    */
	    if (*split_point == DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b))
		*split_point -= 1;

	    if (DMZ_AM_MACRO(11))
	    {
		TRformat(print, 0, buffer, sizeof(buffer) - 1,
			"%8* dm1cx: Amount=%d pos=%d entry_size=%d split=%d",
			amount, pos, entry_size, *split_point);
	    }
	}
    }

    if (DMZ_AM_MACRO(11))
    {
	TRformat(print, 0, buffer, sizeof(buffer) - 1,
		"%8* dm1cx: Midpoint of page %d (kids %d) is position %d",
		DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b), 
		DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b), *split_point );
    }

    return;
}

/*
** Name: dm1cxput		- Puts entry onto page
**
** Description:
**	This routine puts an index entry onto an index page. It is assumed
**	that the BID is valid and has not already been placed here by the
**	same transaction.
**
**	If called after dm1cxallocate, this routine accomplishes the physical
**	insertion of a new entry onto a page. If the space is not new, then
**	this routine can update an existing entry (i.e., it can operate as a
**	replace). Note, however, that an in-place replace can only be 
**	performed if the caller knows that the record sizes are identical and
**	that the new record and old record have the same key.
**
** Inputs:
**	compression_type		Index page compression options:
**					    DM1CX_UNCOMPRESSED
**					    DM1CX_COMPRESSED
**	update_mode			0 or DM1C_DEFERRED
**	tran_id				transaction ID if deferred
**	sequence_number			transaction's seqno if deferred
**	child				The LINEID part of a TID used to
**					indicate where on this page the
**					entry is to be put
**	b				the DMPP_PAGE page to use
**	page_size			Page size for this table.
**	key				the key (entry) to be put on the page
**	klen				the length of the (compressed) key
**	tidp				the TIDP to place on the record.
**
** Outputs:
**	None, but the page is updated
**
** Returns:
**	DB_STATUS
**
** History:
**	5-dec-1990 (bryanp)
**	    Created.
**	14-jan-1991 (bryanp)
**	    Added compression_type argument in order to know when dm1cxlength()
**	    can be safely called (ugh!).
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all the page header access as macro for New Page Format
**	    project.
**	    Pass page_size argument to dm1bvpt_kido_macro, dm1bvpt_kidput_macro,
**	    dmpp_get_offset_macro, dmpp_set_new_macro. Pass page pointer
**	    argument to dmpp_set_new_macro.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          dm1cxput() When (page_type != TCB_PG_V1):
**          Copy tran_id into tuple header for leaf pages.
**          Don't update PAGE tranid, PAGE seq number
**          (seq number is maintained per tuple on large pages)
**      04-feb-97 (stial01)
**          Tuple headers are on LEAF and overflow (CHAIN) pages
**          Tuple headers are not on INDEX pages.
**      07-jul-98 (stial01)
**          dm1cxput() Deferred update processing for V2 pages, use page tranid
**          when page locking, tuple header tranid when row locking
**	22-Jan-2004 (jenjo02)
**	    Extract TidSize from page, caller must supply partno.
*/
DB_STATUS
dm1cxput(i4 page_type, i4 page_size, DMPP_PAGE *b,
	    i4 compression_type, i4 update_mode, DB_TRAN_ID *tran_id,
	    u_i2 lg_id, i4 sequence_number, i4 child,
	    char *key, i4 klen, DM_TID *tidp, i4 partno)
{
    char	    buffer[132];
    i4		    pos = (i4)child;
    char	    *tup_hdr;
    bool	    is_index;
    i4		    offset;
    DM_LINEID	    *lp = DM1B_VPT_BT_SEQUENCE_MACRO(page_type, b);
    i2		    TidSize = DM1B_VPT_GET_BT_TIDSZ_MACRO(page_type, b);

    is_index = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, b) & DMPP_INDEX) != 0);


    /* If its not our tran, place ours there with invalid sequence 0 */
    if ( tran_id  )
    {
	if ((DM1B_VPT_GET_TRAN_ID_HIGH_MACRO(page_type, b) != 
						    tran_id->db_high_tran)
	     || (DM1B_VPT_GET_TRAN_ID_LOW_MACRO(page_type, b) 
						    != tran_id->db_low_tran)) 
	{
	    DM1B_VPT_SET_PAGE_TRAN_ID_MACRO(page_type, b, *tran_id);
	    DM1B_VPT_SET_PAGE_SEQNO_MACRO(page_type, b, 0);
	    DM1B_VPT_SET_PAGE_LG_ID_MACRO(page_type, b, lg_id);
	}
    }

    /* TIDs of Clustered Leaf entries are the entry's BID at the time of the put */
    if ( tidp && DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, b) & DMPP_CLUSTERED &&
	 pos != DM1B_LRANGE && pos != DM1B_RRANGE )
    {
	/* But don't override reserved entry's funny TID */
	if ( tidp->tid_tid.tid_page != 0 && tidp->tid_tid.tid_line != DM_TIDEOF )
	{
	    tidp->tid_tid.tid_page = DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b);
	    tidp->tid_tid.tid_line = pos;
	}
    }

    /*
    ** Sanity check the arguments so we don't scribble all over memory
    */
    if (pos < DM1B_LRANGE || 
	pos >= DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b) ||
	dmpp_vpt_get_offset_macro(page_type, lp, pos + DM1B_OFFSEQ)
					> ((unsigned) page_size) ||
	dmpp_vpt_get_offset_macro(page_type, lp, pos + DM1B_OFFSEQ)
					< ((unsigned) DM1B_VPT_OVER(page_type)))
    {
	/* This TRdisplay should become a ule_formatted error msg. */
	TRdisplay("dm1cxput: child %d, page %d, kids %d, offset %d\n",
		pos, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b), 
		DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b),
		(pos >= DM1B_LRANGE && 
		pos < DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b)) ?
		dmpp_vpt_get_offset_macro(page_type, lp, pos + DM1B_OFFSEQ) : -1);
	return (E_DB_ERROR);
    }

    if (DMZ_AM_MACRO(11))
    {
	TRformat(print, 0, buffer, sizeof(buffer) - 1,
		"%8* dm1cx: Put entry %d (len %d tid=(%d,%d) partno %d) on pg %d (at %d)",
		pos, klen, 
		(tidp != (DM_TID *)0 ? tidp->tid_tid.tid_page : -1),
		(tidp != (DM_TID *)0 ? tidp->tid_tid.tid_line : -1),
		partno,
		DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b),
		dmpp_vpt_get_offset_macro(page_type, lp, pos + DM1B_OFFSEQ));
    }

    /*
    ** If extra edit checks are enabled, and this index page is compressed,
    ** then double-check the length of the entry to ensure that the allocated
    ** length and the put length are the same. We can't perform this check for
    ** a non-compressed index page because dm1cxlength() doesn't work on those
    ** pages.
    */
    if (DMZ_AM_MACRO(12))
    {
	i4	    allocated_size;

	if (compression_type != DM1CX_UNCOMPRESSED)
	{
	    dm1cxlength(page_type, page_size, b, pos, &allocated_size );
	    if (!is_index)
		allocated_size -= Dmpp_pgtype[page_type].dmpp_tuphdr_size;
	    if (allocated_size != (klen + TidSize))
	    {
		TRdisplay("dm1cxput: Entry %d on page %d is size %d, not %d\n",
			(i4)pos, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b), 
			allocated_size, klen + TidSize);
		/*
		** a common bug during development was to allocate space for
		** the key and forget to allow for TidSize extra for the tid:
		*/
		if (allocated_size == klen)
		    TRdisplay("        : Space wasnt allocated for the tid!\n");
		return (E_DB_ERROR);
	    }
	}
    }

    /* 
    ** Point to where the new record is to be added
    ** and add the key. If the tidp is given, store that in the record as
    ** well.
    */

    MEcopy((char *)key, klen,
	    (char *)(dm1bvpt_kido_macro(page_type, b, pos) + TidSize));

    if (tidp)
	dm1bvpt_kidput_macro(page_type, b, pos, tidp, partno); 

    DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, b, DMPP_MODIFY);
    if (is_index)
	return (E_DB_OK);

    /* Extra Leaf page processing */

    /* Get tuple hdr ptr */
    if (page_type == TCB_PG_V1)
	tup_hdr = NULL;
    else
    {
	/* Clear the free bit */
	dmpp_vpt_clear_free_macro(page_type, lp, (i4)pos + DM1B_OFFSEQ);

	/* Clear the tuple header */
	tup_hdr = dm1b_vpt_entry_macro(page_type, b, (i4)pos);
	DMPP_VPT_CLEAR_HDR_MACRO(page_type, tup_hdr);
    }

    /*
    ** Row locking: store LOW tranid 
    ** This is necessary so we can do most dup checking without locking
    ** and we can do nonkey quals on committed data without locking
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

    return (E_DB_OK);
}

/*
** Name: dm1cxrclaim		- Reclaim data space for deleted record(s)
**
** Description:
**	When an entry is deleted from a Btree index page, and compressed index
**	storage is being used, we need to reclaim the space which is in use.
**	Initially, we will reclaim the space on every delete. Later, we may
**	change this to reclaim space on an as-needed basis (more of a garbage
**	collection type of approach).
**
**	Garbage collection would be more efficient, but in order to allow this
**	we'd have to retain the old offsets rather then zero-ing them out and
**	compressing them. This will require some more thought (one proposal is
**	to invent a 'deleted' bit in the line table entry and use that).
**
**	The deleted line table entry is not removed by this routine. However,
**	we do go through and adjust any other line table entries which are
**	affected so that their offsets are corrected.
**
**	In order to perform the compaction, we use a page-sized buffer on the
**	stack. If we had an overlapping-copy ME routine we could call, we could
**	do away with this buffer.
**
**	This routine should NOT be called for an uncompressed index page. It
**	has no way of protecting itself against this. The result would be
**	corruption of the page. Instead, uncompressed pages do NOT need to
**	reclaim space -- "holes" are left on the page and re-used by subsequent
**	entries added to the page.
**
** Inputs:
**	b			- page to be compacted
**	page_size		- Page size for this table.
**	child			- sequence number of the record which was
**				  just deleted and may now be reclaimed
**
** Outputs:
**	None
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** Side Effects:
**	The entries on the page are moved about. Line table offsets are adjusted
**
** History:
**	5-dec-1990 (bryanp)
**	    Created.
**	8-feb-1991 (bryanp)
**	    Changed from VOID to DB_STATUS and added extra error checking.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
**	    Don't allocate page buffers on the stack.
**	    Use dm0m_lcopy to copy large amounts around.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all the page header access as macro for New Page Format
**	    project.
**	    Pass page_size argument to dmpp_put_offset_macro for tuple
**	    header implementation.
**      04-feb-97 (stial01)
**          Tuple headers are on LEAF and overflow (CHAIN) pages
**          Tuple headers are not on INDEX pages.
**      27-feb-1997 (stial01)
**         dm1cxrclaim() Clear DMPP_CLEAN when no more deleted tuples on page.
**	22-Jan-2004 (jenjo02)
**	    Extract TidSize from page;
**	13-Jun-2006 (jenjo02)
**	    The maximum size of a Leaf is now DM1B_MAXLEAFLEN,
**	    not DM1B_KEYLENGTH.
*/
DB_STATUS
dm1cxrclaim(i4 page_type, i4 page_size, DMPP_PAGE *b, i4 child)
{
    i4	del_offset;
    i4	low_off;
    char	*start_pt;
    i4	record_size;
    i4	entry_len;
    i4	pos;
    i4	i;
    char	*temp;
    bool	is_index;
    i4     del_cnt;
    i2		TidSize = DM1B_VPT_GET_BT_TIDSZ_MACRO(page_type, b);

    is_index = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, b) & DMPP_INDEX) != 0);
    if ((temp = dm0m_pgalloc(page_size)) == 0)
	return (E_DB_ERROR);

    if (child >= DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b) || child < DM1B_LRANGE)
    {
	TRdisplay("dm1cxrclaim: child %d out of range (%d..%d) on page %d\n",
		    child, DM1B_LRANGE, DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b), 
		    DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b));
	dm0m_pgdealloc(&temp);
	return (E_DB_ERROR);
    }

    /*
    ** Find out where the record to be deleted begins, and determine where
    ** the lowest record on the page begins. The bytes between these two
    ** locations must be shifted up by the record size.
    */

    del_offset = dmpp_vpt_get_offset_macro(page_type,
					DM1B_VPT_BT_SEQUENCE_MACRO(page_type, b),
					(i4)child + DM1B_OFFSEQ);

    low_off = lowest_offset(page_type, page_size, b);
    start_pt = (char *)b + low_off;

    /*
    ** Determine the size of this record.
    */
    dm1cxlength(page_type, page_size, b, child, &entry_len);

    if (DMZ_AM_MACRO(12))
    {
	record_size = entry_len;
	if (!is_index)
		record_size -= Dmpp_pgtype[page_type].dmpp_tuphdr_size;
	if (del_offset > page_size || del_offset < DM1B_VPT_OVER(page_type) ||
	    start_pt < (char *)b + DM1B_VPT_OVER(page_type) ||
	    start_pt > (char *)b + page_size ||
	    record_size < TidSize || record_size > (DM1B_MAXLEAFLEN + TidSize))
	{
	    TRdisplay("dm1cxrclaim: del_off=%d, low_off=%d, rec_len=%d\n",
			del_offset, low_off, record_size );
	    dm0m_pgdealloc(&temp);
	    return (E_DB_ERROR);
	}
    }

    /*
    ** Update affected line numbers. Any line table entries which point to
    ** records starting before this record must be adjusted by the size of
    ** this record (since we'll be shifting them that far). Note that we
    ** examine the LRANGE and RRANGE offsets as well as ordinary entry offsets
    ** during this pass.
    */
    del_cnt = 0;
    for (pos = DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b) + DM1B_OFFSEQ; --pos >= 0; )
    {
	if (pos != ((i4)child + DM1B_OFFSEQ))
	{
	    if (dmpp_vpt_get_offset_macro(page_type,
			DM1B_VPT_BT_SEQUENCE_MACRO(page_type, b), 
			pos) < (unsigned) del_offset)
	    {
	      dmpp_vpt_put_offset_macro(page_type,
		 	DM1B_VPT_BT_SEQUENCE_MACRO(page_type, b), pos,
	         	(dmpp_vpt_get_offset_macro(page_type,
				DM1B_VPT_BT_SEQUENCE_MACRO(page_type, b), pos) + 
				entry_len));
	    }
	    if (dmpp_vpt_test_free_macro(page_type,
				    DM1B_VPT_BT_SEQUENCE_MACRO(page_type, b),
				    pos + DM1B_OFFSEQ))
		del_cnt++;
	}
    }

    if (page_type != TCB_PG_V1 && del_cnt == 0)
    {
	DM1B_VPT_CLR_PAGE_STAT_MACRO(page_type, b, DMPP_CLEAN);
    }

    /*
    ** If we have deleted a tuple from the middle of the data portion of the
    ** page then compact the page.
    **
    ** Checking that 'i' is > 0 rather than just non-zero adds the extra safety
    ** of not trashing the page if this routine happens to be called with a TID
    ** that specifies a non-existent tuple. If i == 0 then we are deleting the
    ** entry with the lowest offset on the page and no memory copying is
    ** needed.
    */
    if ((i = (char *)b + del_offset - start_pt) > 0)
    {
	/* 
	** Since the source and the destination overlap we move the 
	** source somewhere else first.
	*/

	MEcopy(start_pt, i, temp);
	MEcopy(temp, i, start_pt + entry_len);
    }
    else if (i < 0)
    {
	TRdisplay("dm1cxrclaim: i out of range: i=%d, del_off=%d, low_off=%d\n",
		    i, del_offset, low_off);
	dm0m_pgdealloc(&temp);
	return (E_DB_ERROR);
    }

    DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, b, DMPP_MODIFY);

    dm0m_pgdealloc(&temp);

    return (E_DB_OK);
}

/*
** Name: dm1cxrecptr		- Return a pointer to a btree index record.
**
** Description:
**	This routine returns a pointer to a btree index page key.
**
**	If index compression is in use then the returned key will be
**	compressed.  It will be up to the caller to uncompress the record
**	if necessary.
**
** Inputs:
**	b			    Index page to get entry from
**	child			    Which record to get
**	entry			    Pointer to pointer to buffer
**
** Outputs:
**	entry			    Set to point to entry
**
** Returns:
**	DB_STATUS
**
** History:
**	14-dec-1992 (rogerk)
**	    Created for Reduced logging project.
**	06-may-1996 (nanpr01 & thaju02)
**	    Pass the page_size as parameter to keep it consistent with
**	    other routines for New Page Format project.
**	    Pass page_size argument to dm1bkeyof_macro for new page format proj.
*/
VOID
dm1cxrecptr(i4 page_type, i4 page_size, DMPP_PAGE *b, i4 child, char **key)
{
    *key = (char *)dm1bvpt_keyof_macro(page_type, b, (i4)child);
}

/*
** Name: dm1cxreplace		- Replace entry on page
**
** Description:
**	This routine updates an entry on a page. This routine is called when
**	the caller is not sure that the (compressed) lengths of the entries
**	will be identical. If the index is known to be non-compressed, or if
**	the caller can ensure that the lengths are identical, then dm1cxput()
**	should be used as it is vastly simpler.
**
**	If the index is compressed, the caller must compress the key ahead of
**	time and provide the compressed entry and its length.
**
**	If the index is not compressed, this routine operates just like
**	dm1cxput -- it writes the entry and tidp (if given) to the indicated
**	position on the page.
**
**	If the index is compressed, this routine extracts the length of the
**	current entry. If the lengths are identical, then this routine can
**	simply overwrite the previous entry without having to re-arrange space
**	on the page.
**
**	If the lengths are different, then this routine deletes the old entry,
**	allocates a new entry of the proper size, and writes the new entry.
**
** Inputs:
**	update_mode			0 or DM1C_DEFERRED
**	compression_type		Value indicating index compression:
**					    DM1CX_UNCOMPRESSED
**					    DM1CX_COMPRESSED
**	tran_id				transaction ID if deferred
**	sequence_number			transaction's seqno if deferred
**	page				The page to work with
**	page_size			The page size for this table.
**	position			Which entry on the page to replace
**	key				The new key to replace (compressed)
**	klen				The length of the (compressed) key
**					This is the KEY length, NOT the entry
**					length; it should NOT include the
**					tid size.
**	tidp				The tid portion of the new (key,tid)
**					pair -- may be 0 if it is desired to
**					replace the key only.
**
** Outputs:
**	None
**
** Returns:
**	DB_STATUS
**
** History:
**	14-dec-1990 (bryanp)
**	    Created.
**	10-sep-1991 (bryanp)
**	    B37642, B39322: Allow zero-length key as input. When replacing the
**	    LRANGE or RRANGE entry on a page, if the entry is "infinity", it
**	    has no actual key value, in which case the klen is zero and the
**	    "key" argument should not be dereferenced.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project:  Added safety check for non DIRECT update
**	    mode since deferred is not really handled here.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all the page header access as macro for New Page Format
**	    project.
**	    Pass page_size argument to dm1bvpt_kido_macro, dm1bvpt_kidput_macro.
**	12-may-1996 (nanpr01)
**	    Fix the current_len check.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          dm1cxreplace() This routine appears to be called for LRANGE,
**          RRANGE leaf entries. Always reclaim space for LRANGE,RRANGE leaf
**          entries when the delete is done.
**      04-feb-97 (stial01)
**          Tuple headers are on LEAF and overflow (CHAIN) pages
**          Tuple headers are not on INDEX pages.
**	22-Jan-2004 (jenjo02)
**	    Extract TidSize from page, caller must supply partno.
**	13-Jun-2006 (jenjo02)
**	    The maximum size of a Leaf is now DM1B_MAXLEAFLEN,
**	    not DM1B_KEYLENGTH.
*/
DB_STATUS
dm1cxreplace(i4 page_type, i4 page_size, DMPP_PAGE *page,
		i4 update_mode, i4 compression_type, DB_TRAN_ID *tran_id,
		u_i2 lg_id, i4 sequence_number,
		i4 position, char *key, i4 klen, 
		DM_TID *tidp, i4 partno)
{
    DB_STATUS	    ret_val;
    i4	    current_length;
    i4	    entry_len;
    DMPP_PAGE	    *b = page;
    DM_TID	    old_tid;
    i4		    old_partno;
    char	    buffer[132];
    bool            is_index;
    i2		    TidSize = DM1B_VPT_GET_BT_TIDSZ_MACRO(page_type, b);

    /* If its not our tran, place ours there with invalid sequence 0 */
    if ( tran_id )
    {
	if ((DM1B_VPT_GET_TRAN_ID_HIGH_MACRO(page_type, b) != 
						    tran_id->db_high_tran)
	     || (DM1B_VPT_GET_TRAN_ID_LOW_MACRO(page_type, b) 
						    != tran_id->db_low_tran)) 
	{
	    DM1B_VPT_SET_PAGE_TRAN_ID_MACRO(page_type, b, *tran_id);
	    DM1B_VPT_SET_PAGE_SEQNO_MACRO(page_type, b, 0);
	    DM1B_VPT_SET_PAGE_LG_ID_MACRO(page_type, b, lg_id);
	}
    }

    is_index = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, b) & DMPP_INDEX) != 0);
    if (DMZ_AM_MACRO(11))
    {
	TRformat(print, 0, buffer, sizeof(buffer) - 1,
		"%8* dm1cx: Replacing entry %d on page %d",
		position, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, page));
    }

    if (DMZ_AM_MACRO(12))
    {
	if (page == (DMPP_PAGE *)0 || (i4)position < DM1B_LRANGE ||
	    (i4)position >= DM1B_VPT_GET_BT_KIDS_MACRO(page_type, page) ||
	    (key == (char *)0 && klen != 0) ||
	    klen < 0 || klen > DM1B_MAXLEAFLEN)
	{
	    TRdisplay("dm1cxreplace: page= %p, position=%d key=%p klen=%d\n",
			page, (i4)position, key, klen);
	    if (page)
		TRdisplay("            : page_page=%d, bt_kids=%d\n",
			    DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, page), 
			    DM1B_VPT_GET_BT_KIDS_MACRO(page_type, page));
	    return (E_DB_ERROR);
	}
    }
	    
#ifdef xDEBUG
    /*
    ** Deferred semantics are not yet implemented in this routine.  They
    ** are probably not hard to add, but have not yet been done so.  Its
    ** not important for the moment since actual leafpage keys are never
    ** updated via dm1cxreplace, only range keys and index page entries.
    **
    ** If we add the ability to do in-place replace operations on leaf pages
    ** then we have to fix deferred semantics here.
    */
    if (update_mode & DM1C_DEFERRED)
    {
	TRdisplay("DM1CXREPLACE called with non direct update mode!!!\n");
	return (E_DB_ERROR);
    }
#endif

    /*
    ** The "TID" of a Clustered leaf entry is always its BID,
    ** unless the entry has been reserved (0,DM_TIDEOF):
    */
    if ( tidp &&
         DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, b) & DMPP_CLUSTERED &&
	 position != DM1B_LRANGE && position != DM1B_RRANGE &&
	 tidp->tid_tid.tid_page != 0  &&
	 tidp->tid_tid.tid_line != DM_TIDEOF )
    {  
	 tidp->tid_tid.tid_page = DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b);
	 tidp->tid_tid.tid_line = position;
    }


    if (compression_type == DM1CX_UNCOMPRESSED)
    {
	/*
	** Replacing an uncompressed entry is easy, because all entries on
	** an uncompressed Btree index page are the same length. We simply
	** copy the supplied key and tid over the key and tid on the page.
	*/
	ret_val = E_DB_OK;

	MEcopy((char *)key, klen,
		(char *)(dm1bvpt_kido_macro(page_type, b, (i4)position) + 
		TidSize));

	if (tidp)
	    dm1bvpt_kidput_macro(page_type, b, (i4)position, tidp, partno); 

	/* FIX_ME -- should we call dm1cxput() to set update_mode, etc.? */
    }
    else
    {
	/*
	** Replacing a compressed entry is significantly harder, because the
	** lengths of the old and new entries are typically not the same and
	** thus we must re-arrange the data space on the page to make the
	** correct amount of room available.
	*/
	entry_len = klen + TidSize;

	dm1cxlength(page_type, page_size, b, position, &current_length );
        if (!is_index)
          current_length -= Dmpp_pgtype[page_type].dmpp_tuphdr_size;

	if (current_length < TidSize ||
	    current_length > (DM1B_MAXLEAFLEN + TidSize ))
	{
	    dm1cxlog_error(E_DM93EC_BAD_INDEX_LENGTH, (DMP_RCB *)0, b,
			    page_type, page_size, (i4)position);
	    return (E_DB_ERROR);
	}

	if (current_length == entry_len)
	{
	    /*
	    ** Great! We can process this as easily as we process an
	    ** uncompressed entry; we simply copy over the indicated entry.
	    */
	    ret_val = E_DB_OK;

	    if (klen)
		MEcopy((char *)key, klen,
		    (char *)(dm1bvpt_kido_macro(page_type, b,
				(i4)position) + TidSize));

	    if (tidp)
		dm1bvpt_kidput_macro(page_type, b, (i4)position, tidp, partno); 

	    /* FIX_ME -- should we call dm1cxput() to set update_mode, etc.? */
	}
	else
	{
	    /*
	    ** Ugly. The new entry is a different size than the old entry was.
	    ** We must delete the old entry and reclaim its space, then
	    ** allocate the new entry and copy into it.Note that we must save
	    ** the old_tid if the new tid is not provided.
	    */
	    if (tidp == (DM_TID *)0)
	    {
		dm1cxtget(page_type, page_size, b, position, 
			    &old_tid, &old_partno );
		tidp = &old_tid;
	    }

	    if (position != DM1B_LRANGE && position != DM1B_RRANGE &&
		(!is_index))
	    {
		TRdisplay("dm1cxreplace: leaf unexpected entry");
		return (E_DB_ERROR);

	    }

	    ret_val = dm1cxdel(page_type, page_size, b, 
				update_mode, compression_type,
				tran_id, lg_id, sequence_number,
				(i4)DM1CX_RECLAIM, position);
	    if (ret_val != E_DB_OK)
		return (ret_val);

	    ret_val = dm1cxallocate(page_type, page_size, b, 
				    update_mode, compression_type,
				    tran_id, sequence_number,
				    position, entry_len);
	    if (ret_val)
	    {
		dm1cxlog_error(E_DM93E0_BAD_INDEX_ALLOC, (DMP_RCB *)0, b,
				page_type, page_size, (i4)position);
		return (ret_val);
	    }

	    if (klen)
		MEcopy((char *)key, klen,
			(char *)(dm1bvpt_kido_macro(page_type, b, position) + 
			TidSize));

	    if (tidp)
		dm1bvpt_kidput_macro(page_type, b, position, tidp, partno); 

	    ret_val = E_DB_OK;
	}
    }

    if (ret_val == E_DB_OK)
	DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, b, DMPP_MODIFY);

    return (ret_val);
}

/*
** Name: dm1cxrshift		- Shift entries from one page to another
**
** Description:
**      This routine right shifts data from one BTREE index page into 
**      another. 
**
**	Both pages should be mutex locked upon entry to this routine (assuming
**	they are buffer manager pages). Both pages are modified by this routine
**	and both are marked DMPP_MODIFY.
**
** Inputs:
**	compression_type		Value indicating the compression type:
**					    DM1CX_UNCOMPRESSED
**					    DM1CX_COMPRESSED
**      current                         Pointer to the page which contains
**                                      data to be shifted.
**      sibling			        Pointer to the page to shift the
**                                      data to.
**	page_size			The page size for this table.
**      split                           Value indicating where on the 
**                                      current page
**                                      to start split.
**
** Outputs:
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
**	12-dec-1990 (bryanp)
**	    Created out of old dm1bxrshift to support compressed indexes.
**	14-jan-1991 (bryanp)
**	    Added entry_len argument and documented its use.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project (phase 2):  Removed setting of page
**	    Modify Bits.  This was previously done because page mutexes
**	    where dropped during the split process allowing background
**	    threads to write out pages which were modified - thus losing
**	    the modify status.  This is no longer required.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all page header access as macro for New Page Format project.
**	    Pass page_size, and page pointer to dmpp_test_new_macro,
**	    dmpp_set_new_macro, dmpp_clear_new_macro for tuple header
**	    implementation.
**	    Change dm1bvpt_kido_macro to dm1b_vpt_entry_macro in MEcopy call.
**          Need to copy tuple header along with key, tid pair.
**	12-may-1996 (nanpr01)
**	    Tuple Header length was getting added twice.
** 	26-jul-1996 (wonst02)
** 	    Allow shifting onto a non-empty page (uncompressed only) for Rtree.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          dm1cxrshift() When (page_type != TCB_PG_V1):
**          Copy free flag to entry on sibling.
**          When calling dm1cxdel to delete entries from page being split,
**          specify reclaim_flag = TRUE.
**      04-feb-97 (stial01)
**          Tuple headers are on LEAF and overflow (CHAIN) pages
**          Tuple headers are not on INDEX pages.
**	22-Jan-2004 (jenjo02)
**	    Extract TidSize from page;
**	13-Jun-2006 (jenjo02)
**	    The maximum size of a Leaf is now DM1B_MAXLEAFLEN,
**	    not DM1B_KEYLENGTH.
*/
DB_STATUS
dm1cxrshift(i4 page_type, i4 page_size, DMPP_PAGE *current, DMPP_PAGE *sibling,
		    i4 compression_type, i4 split, i4 entry_len)
{
    DMPP_PAGE		*cur = current;
    DMPP_PAGE		*sib = sibling;
    i4 		pos, sibpos; 
    i4		entry_length;
    i4		original_kids;
    DB_STATUS		ret_val = E_DB_ERROR;
    char		buffer[132];
    bool                is_index;
    i2		TidSize = DM1B_VPT_GET_BT_TIDSZ_MACRO(page_type, current);

    is_index = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, current) & DMPP_INDEX) 
			!= 0);
    if (!is_index)
      entry_len += Dmpp_pgtype[page_type].dmpp_tuphdr_size;

    if (DMZ_AM_MACRO(11))
    {
	TRformat(print, 0, buffer, sizeof(buffer) - 1,
		"%8* dm1cx: Right-shifting from page %d (at %d) to page %d",
		DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, cur), split, 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, sib));
    }

    if (compression_type == DM1CX_UNCOMPRESSED)
    {
	/*
	** Shifting uncompressed entries is easy, because all entries have
	** the same length and the page offsets are all pre-formatted.
	** Find the length of the entry, copy each entry, and then adjust
 	** the bt_kids value. First, shift any existing entries on the sibling
 	** page, then move entries from the current to the sibling.
	*/
 	pos = DM1B_VPT_GET_BT_KIDS_MACRO(page_type, sib) - 1;
 	sibpos = pos + (DM1B_VPT_GET_BT_KIDS_MACRO(page_type,cur) - (i4)split);
 	for (; pos >= 0; sibpos--, pos--)
 	{
 	    MEcopy( (char * )dm1b_vpt_entry_macro(page_type, sib, pos),
 		    entry_len,
 		    (char * )dm1b_vpt_entry_macro(page_type, sib, sibpos) );
 	    if (dmpp_vpt_test_new_macro(page_type,
 				    DM1B_VPT_BT_SEQUENCE_MACRO(page_type, sib),
 				    pos + DM1B_OFFSEQ))
 	    {
 		dmpp_vpt_set_new_macro(page_type,
 				    DM1B_VPT_BT_SEQUENCE_MACRO(page_type, sib),
 			     	    sibpos + DM1B_OFFSEQ);
 	    }
 	    else
 		dmpp_vpt_clear_new_macro(page_type,
 				    DM1B_VPT_BT_SEQUENCE_MACRO(page_type, sib),
 			      	    sibpos + DM1B_OFFSEQ);
 	}
	for (sibpos = 0, pos = split; 
	     pos < DM1B_VPT_GET_BT_KIDS_MACRO(page_type, cur); sibpos++, pos++)
	{
	    MEcopy( (char * )dm1b_vpt_entry_macro(page_type, cur, pos), 
		    entry_len, 
		    (char * )dm1b_vpt_entry_macro(page_type, sib, sibpos) );
	    dmpp_vpt_clear_flags_macro(page_type,
					DM1B_VPT_BT_SEQUENCE_MACRO(page_type, sib),
					sibpos + DM1B_OFFSEQ);
	    if (dmpp_vpt_test_new_macro(page_type, 
				    DM1B_VPT_BT_SEQUENCE_MACRO(page_type, cur), 
				    pos + DM1B_OFFSEQ))
	    {
		dmpp_vpt_set_new_macro(page_type,
				    DM1B_VPT_BT_SEQUENCE_MACRO(page_type, sib), 
			     	    sibpos + DM1B_OFFSEQ);
	    }
	    if (dmpp_vpt_test_free_macro(page_type,
				    DM1B_VPT_BT_SEQUENCE_MACRO(page_type, cur),
				    pos + DM1B_OFFSEQ))
	    {
		dmpp_vpt_set_free_macro(page_type,
				    DM1B_VPT_BT_SEQUENCE_MACRO(page_type, sib),
				    sibpos + DM1B_OFFSEQ);
	    }
	}
	DM1B_VPT_SET_BT_KIDS_MACRO(page_type, sib, 
 		DM1B_VPT_GET_BT_KIDS_MACRO(page_type, sib)
 		+ (DM1B_VPT_GET_BT_KIDS_MACRO(page_type, cur) - (i4)split));
	DM1B_VPT_SET_BT_KIDS_MACRO(page_type, cur, split); 

	ret_val = E_DB_OK;
    }
    else
    {
	/*
	** Shifting compressed entries is harder, because we must dynamically
	** manage the space on the page. For each entry on the sibling page,
	** we get its length, allocate that much space on the current page,
	** and copy it over. We try to avoid uncompressing each entry as we
	** copy it, for performance reasons, although that decision will need
	** to be re-thought when we try to implement page-level compression.
	*/
	ret_val = E_DB_OK;
	original_kids = DM1B_VPT_GET_BT_KIDS_MACRO(page_type, cur);

	for (sibpos = 0, pos = split; 
	     pos < DM1B_VPT_GET_BT_KIDS_MACRO(page_type, cur); sibpos++, pos++)
	{
	    i4 adj_length;

	    dm1cxlength(page_type, page_size, cur, pos, &entry_length );
	    if (is_index)
		adj_length = entry_length;
	    else
		adj_length = entry_length - 
				Dmpp_pgtype[page_type].dmpp_tuphdr_size;

	    if (adj_length < TidSize ||
		adj_length > (DM1B_MAXLEAFLEN + TidSize))
	    {
		dm1cxlog_error(E_DM93EC_BAD_INDEX_LENGTH, (DMP_RCB *)0, cur,
				page_type, page_size, (i4)pos);
		return (E_DB_ERROR);
	    }

	    if (DMZ_AM_MACRO(11))
	    {
		TRformat(print, 0, buffer, sizeof(buffer) - 1,
		    "%8* dm1cx: Shifting entry %d, of length %d",
		    pos, entry_length);
	    }

	    ret_val = dm1cxallocate(page_type, page_size, sib, 
				    DM1C_DIRECT, compression_type,
				    (DB_TRAN_ID *)0, (i4)0,
				    sibpos, adj_length );
	    if (ret_val)
	    {
		dm1cxlog_error(E_DM93E0_BAD_INDEX_ALLOC, (DMP_RCB *)0, sib,
				page_type, page_size, (i4)sibpos);
		return (ret_val);
	    }

	    MEcopy( (char * )dm1b_vpt_entry_macro(page_type, cur, pos), 
		    entry_length,
		    (char * )dm1b_vpt_entry_macro(page_type, sib, sibpos) );

	    dmpp_vpt_clear_flags_macro(page_type,
					DM1B_VPT_BT_SEQUENCE_MACRO(page_type, sib),
					sibpos + DM1B_OFFSEQ);
	    if (dmpp_vpt_test_new_macro(page_type, 
				    DM1B_VPT_BT_SEQUENCE_MACRO(page_type, cur), 
				    pos + DM1B_OFFSEQ))
	    {
		dmpp_vpt_set_new_macro(page_type,
				    DM1B_VPT_BT_SEQUENCE_MACRO(page_type, sib), 
			     	    sibpos + DM1B_OFFSEQ);
	    }
	    if (dmpp_vpt_test_free_macro(page_type,
				    DM1B_VPT_BT_SEQUENCE_MACRO(page_type, cur),
				    pos + DM1B_OFFSEQ))
	    {
		dmpp_vpt_set_free_macro(page_type,
				    DM1B_VPT_BT_SEQUENCE_MACRO(page_type, sib),
				    sibpos + DM1B_OFFSEQ);
	    }
	}

	/*
	** Now we go and delete all the entries we just copied. This current
	** method is a performance hit, because it results in a space compacting
	** on each entry, whereas we'd like to optimize the number of times
	** that we move entries around.
	*/
	for (pos = split; pos < DM1B_VPT_GET_BT_KIDS_MACRO(page_type, cur); )
	{
	    ret_val = dm1cxdel(page_type, page_size, cur,
				DM1C_DIRECT, compression_type,
				(DB_TRAN_ID *)0, (u_i2)0, (i4)0,
				(i4)DM1CX_RECLAIM, pos);
	    if (ret_val)
	    {
		dm1cxlog_error( E_DM93E2_BAD_INDEX_DEL, (DMP_RCB *)0, cur,
				page_type, page_size, pos);
		return (ret_val);
	    }
	}

	if (DMZ_AM_MACRO(12))
	{
	    /*
	    ** Extra edit checking. Certain post-conditions should be true
	    ** following an rshift:
	    **	1) sum of the kids on the two pages should be precisely the
	    **	    number of kids we started with.
	    **	2) The length of each kid should be reasonable
	    */
	    if ((DM1B_VPT_GET_BT_KIDS_MACRO(page_type, cur) + 
		 DM1B_VPT_GET_BT_KIDS_MACRO(page_type, sib)) != original_kids)
	    {
		TRdisplay("dm1cxrshift: kids error: cur (%d)+sib (%d) != %d\n",
			DM1B_VPT_GET_BT_KIDS_MACRO(page_type, cur) + 
			DM1B_VPT_GET_BT_KIDS_MACRO(page_type, sib), original_kids);
		return (E_DB_ERROR);
	    }
	    for (pos = 0; pos < DM1B_VPT_GET_BT_KIDS_MACRO(page_type, cur); pos++ )
	    {
		i4 adj_length;

		dm1cxlength(page_type, page_size, cur, pos, &entry_length );

		if (is_index)
		    adj_length = entry_length;
		else
		    adj_length = entry_length - 
					Dmpp_pgtype[page_type].dmpp_tuphdr_size;
		if (adj_length < TidSize ||
		    adj_length > (DM1B_MAXLEAFLEN + TidSize))
		{
		    dm1cxlog_error(E_DM93EC_BAD_INDEX_LENGTH, (DMP_RCB *)0, cur,
				    page_type, page_size, (i4)pos);
		    return (E_DB_ERROR);
		}
	    }
	    for (pos = 0; pos < DM1B_VPT_GET_BT_KIDS_MACRO(page_type, sib); pos++ )
	    {
		i4 adj_length;

		dm1cxlength(page_type, page_size, sib, pos, &entry_length );

		if (is_index)
		    adj_length = entry_length;
		else
		    adj_length = entry_length - 
					Dmpp_pgtype[page_type].dmpp_tuphdr_size;
		if (adj_length < TidSize ||
		    adj_length > (DM1B_MAXLEAFLEN + TidSize))
		{
		    dm1cxlog_error(E_DM93EC_BAD_INDEX_LENGTH, (DMP_RCB *)0, sib,
				    page_type, page_size, (i4)pos);
		    return (E_DB_ERROR);
		}
	    }
	}
    }

    DM1B_VPT_SET_PAGE_TRAN_ID_MACRO(page_type, sib, 
		DM1B_VPT_GET_PAGE_TRAN_ID_MACRO(page_type, cur));
    DM1B_VPT_SET_PAGE_SEQNO_MACRO(page_type, sib, 
			DM1B_VPT_GET_PAGE_SEQNO_MACRO(page_type, cur));

    return (ret_val);
}

/*
** Name: dm1cxtget		- Get the TIDP portion of an entry from a page
**
** Description:
**	This routine fetches the TIDP portion of an entry. Perhaps it should
**	instead be something like "get partial entry" and should be told to
**	fetch the 4 bytes at offset 0. The question is at what layer do we
**	"know" that the TIDP is always at the start of the entry?
**
**	It is assumed that the entry is valid.
**
** Inputs:
**	b			- the index page to use
**	child			- the lineid portion of the entry to be fetched
**
** Outputs:
**	tidp			- set to the TIDP of this record.
**	partnop			- option pointer to where to return
**				  partition number from Global SI
**				  leaf entry.
**
** Returns:
**	VOID
**
** History:
**	5-dec-1990 (bryanp)
**	    Created.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all page header access as macro for New Page Format project.
**	    Pass page_size argument to dm1bvpt_kidget_macro.
**	22-Jan-2004 (jenjo02)
**	    Added (optional) *partnop (partition number) output parm.
*/
VOID
dm1cxtget(i4 page_type, i4 page_size, 
DMPP_PAGE *b, i4 child, 
DM_TID *tidp, i4 *partnop)
{
    char	buffer[132];


    if (DMZ_AM_MACRO(12))
    {
	if (b == (DMPP_PAGE *)0 || (i4)child < DM1B_LRANGE ||
	    (i4)child >= DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b) || 
	    tidp == (DM_TID *)0)
	{
	    TRdisplay("dm1cxtget: b=%p child=%d tidp=%p, partnop=%p\n",
			b, (i4)child, tidp, partnop);
	    if (b)
		TRdisplay("         : page_page=%d bt_kids=%d\n",
			    DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b), 
			    DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b));
	    return; /* no way to return an error at this point... */
	}
    }

    dm1bvpt_kidget_macro(page_type, b, (i4)child, tidp, partnop);

    /*
    ** The "TID" of a Clustered leaf entry is always its BID,
    ** unless the entry has been reserved (0,DM_TIDEOF), or
    ** it's a Range entry.
    */
    if ( DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, b) & DMPP_CLUSTERED &&
	 child != DM1B_LRANGE && child != DM1B_RRANGE &&
	 tidp->tid_tid.tid_page != 0  &&
	 tidp->tid_tid.tid_line != DM_TIDEOF )
    {  
	tidp->tid_tid.tid_page = DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b);
	tidp->tid_tid.tid_line = child;
    }

    if (DMZ_AM_MACRO(11))
    {
	TRformat(print, 0, buffer, sizeof(buffer) - 1,
		"%8* dm1cx: Tid {%d,%d} Part %d retrieved from entry %d of page %d",
		tidp->tid_tid.tid_page, tidp->tid_tid.tid_line,
		(partnop) ? *partnop : -1,
		(i4)child, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b));
    }

    return;
}

/*
** Name: dm1cxtput		- Put the TIDP portion of an entry onto a page
**
** Description:
**	This routine updates the TIDP portion of an entry. Perhaps it should
**	instead be something like "put partial entry" and should be told to
**	write the 4 bytes at offset 0. The question is at what layer do we
**	"know" that the TIDP is always at the start of the entry?
**
**	The entry should already have been allocated by dm1cxallocate. If the
**	entry has not been allocated, then an error is returned.
**
** Inputs:
**	b			- the index page to use
**	child			- the lineid portion of the entry to be fetched
**	tidp			- set to the TIDP of this record.
**
** Outputs:
**	The TIDP on the page is changed.
**
** Returns:
**	E_DB_OK			- successfully updated tid
**	E_DB_ERROR		- No such entry on this page.
**
** History:
**	5-dec-1990 (bryanp)
**	    Created.
**	14-jan-1991 (bryanp)
**	    Changed from VOID to DB_STATUS return.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all page header access as macro for New Page Format project.
**	    Pass page_size argument to dm1bvpt_kidput_macro.
**	22-Jan-2004 (jenjo02)
**	    Caller must supply partno.
*/
DB_STATUS
dm1cxtput(i4 page_type, i4 page_size, DMPP_PAGE *b, i4 child, 
DM_TID *tidp,
i4	partno)
{
    char	buffer [132];

    if (DMZ_AM_MACRO(12))
    {
	if (b == (DMPP_PAGE *)0 || (i4)child < DM1B_LRANGE ||
	    (i4)child >= DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b) || 
	    tidp == (DM_TID *)0)
	{
	    TRdisplay("dm1cxtput: b=%p child=%d tidp=%p\n",
			b, (i4)child, tidp);
	    if (b)
		TRdisplay("         : page_page=%d bt_kids=%d\n",
			    DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b), 
			    DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b));
	    if (tidp)
		TRdisplay("         : tidp->tid_page=%d, tidp->tid_line=%d\n",
			    tidp->tid_tid.tid_page, tidp->tid_tid.tid_line);
	    return (E_DB_ERROR);
	}
    }

    if ((i4)child < DM1B_LRANGE || 
	(i4)child >= DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b))
	return (E_DB_ERROR);

    dm1bvpt_kidput_macro(page_type, b, (i4)child, tidp, partno);

    if (DMZ_AM_MACRO(11))
    {
	TRformat(print, 0, buffer, sizeof(buffer) - 1,
		"%8* dm1cx: Tid {%d,%d} Part %d written into entry %d of page %d",
		tidp->tid_tid.tid_page, tidp->tid_tid.tid_line,
		partno,
		(i4)child, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b));
    }

    return (E_DB_OK);
}

/*
** Name: lowest_offset		- locate the lowest offset on this page
**
** Description:
**	When the index entries are compressed, several space management
**	algorithms require the location of the lowest entry offset currently
**	in use on this page. This routine returns that value.
**
**	Currently, we calculate this value on demand. Since the DMPP_PAGE
**	structure has available space, we could change this in the future to
**	keep this value up to date somewhere in the page header and then just
**	return it.
**
**	Alternatively, we could mimic the Ingres data page technique of
**	reserving an extra line table entry to hold this value.
**
** Inputs:
**	b			- the page to use
**	page_size		- size of this table.
**
** Outputs:
**	None
**
** Returns:
**	The lowest offset currently in use on this page
**
** History:
**	5-dec-1990 (bryanp)
**	    Created.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change page header references to use macros for New Page Format
**	    Project.
**      12-jun-1997 (stial01)
**          lowest_offset() DON'T assume tuple ends at end of page
*/
static	i4
lowest_offset(i4 page_type, i4 page_size, DMPP_PAGE *b)
{
    i4	    i, kids;
    u_i4    min_offset;
    u_i4    this_offset;

    min_offset = page_size - DM1B_VPT_GET_ATTSIZE_MACRO(page_type, b);
    kids = DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b) + DM1B_OFFSEQ;

    for ( i = 0; i < kids; i++ )
    {
	this_offset = dmpp_vpt_get_offset_macro(page_type,
			DM1B_VPT_BT_SEQUENCE_MACRO(page_type, b), (i4) i);
	if ( this_offset <  min_offset )
	    min_offset = this_offset;
    }

    return (min_offset);
}

/*
** Name: print		    - send a line of trace output
**
** Description:
**	This routine is called from TRformat to send trace output.
**
** Inputs:
**	arg		    - (unused) argument from TRformat
**	count		    - number of bytes in the TRformat buffer
**	buffer		    - the TRformat buffer
**
** Outputs:
**	None
**
** Returns:
**	VOID
**
** History:
**	8-jan-1991 (bryanp)
**	    Created.
*/
static i4
print(PTR arg, i4 count, char *buffer)
{
    SCF_CB	scf_cb;

    buffer[count] = '\n';
    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_DMF_ID;
    scf_cb.scf_nbr_union.scf_local_error = 0;
    scf_cb.scf_len_union.scf_blength = (u_i4)count + 1;
    scf_cb.scf_ptr_union.scf_buffer = buffer;
    scf_cb.scf_session = DB_NOSESSION;
    scf_call(SCC_TRACE, &scf_cb);

    /*
    ** probably we should add some code which makes this routine do EITHER
    ** the scc_trace, or the TRdisplay, but not both. Note that we must
    ** explicitly pass the length to TRdisplay, since buffer may not be
    ** null terminated (and we don't want to write into buffer[count+1], since
    ** the buffer may not contain that many characters...)
    */
    TRdisplay("%.#s\n", count, buffer);
}

/*
** Name: page_dump	- debugging routine
**
** Description:
**	This routine dumps a Btree index page, in hex, to the trace log, for
**	problem determination purposes.
**
** Inputs:
**	Page to dump
**
** Outputs:
**	None
**
** Returns:
**	VOID
**
** History:
**	4-feb-1991 (bryanp)
**	    Created.
**	06-may-1996 (nanpr01)
**	    Change all the page header access as macro for New Page Format
**	    project.
*/
static VOID
page_dump(i4 page_type, i4 page_size, DMPP_PAGE *page)
{
    i4  i;

    TRdisplay("================== PAGE DUMP INVOKED ===============\n");

    TRdisplay("Page dump of page @ %p\n", page);

    TRdisplay("   page=%x main=%x ovfl=%x stat=%x next-page=%d, kids=%d\n",
		DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, page), 
		DM1B_VPT_GET_PAGE_MAIN_MACRO(page_type, page), 
		DM1B_VPT_GET_PAGE_OVFL_MACRO(page_type, page),
		DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page), 
		DM1B_VPT_GET_BT_NEXTPAGE_MACRO(page_type, page), 
		DM1B_VPT_GET_BT_KIDS_MACRO(page_type, page));

    TRdisplay("============ page contents hex dump ================\n");
    for (i=0; i < page_size; i += 16)
	TRdisplay("   0x%x:%< %4.4{%x%}%2< >%16.1{%c%}<\n",
		    ((char *)page) + i, 0);

    TRdisplay("================== END OF PAGE DUMP ================\n");

    return;
}


/*{
** Name: clear - Clears the line table and transaction bits.
**
** Description:
**    This routine clears all the new bits in line and sets
**    the transaction id and sequence number if this is a different transaction.
**
** Inputs:
**      tran_id                         Pointer to structure containing
**                                      transaction id.
**      sequence_number                 Value indicating which sequence
**                                      this update is associated with.
**       p				Pointer to page.
**                                      to store retrieved record.
**	 page_size			page size.
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
**      08-nov-85 (jennifer)
**          Converted for Jupiter.
**	09-jun-1992 (kwatts)
**	    6.5 MPF Changes. Stolen from dm1c.c where it was dm1c_clear. Now
**	    only used locally by this module.
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
**		Pass page_size, page pointer arguments to dmpp_clear_new_macro.
**      07-jul-98 (stial01)
**          clear() Deferred update processing for V2 pages, use page tranid
**          when page locking, tuple header tranid when row locking
*/
static VOID
clear(DMP_RCB *r, DMPP_PAGE *b)
{
    i4		    i; 
    i4		    page_type = r->rcb_tcb_ptr->tcb_rel.relpgtype;
    i4		    length = DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b);
    DM_LINEID	    *lines;

    if (page_type == TCB_PG_V2 
	|| (DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, b) & DMPP_INDEX))
    {
	TRdisplay("Invalid clear for page type %d\n", page_type);
	dmd_check(E_DM93F5_ROW_LOCK_PROTOCOL);
    }

    /* Update the pages transaction id and sequence number. */
    lines = DM1B_VPT_GET_ADDR_BT_SEQUENCE_MACRO(page_type, b, DM1B_OFFSEQ);
    if ((DM1B_VPT_GET_TRAN_ID_HIGH_MACRO(page_type, b) != 
						r->rcb_tran_id.db_high_tran)
         || (DM1B_VPT_GET_TRAN_ID_LOW_MACRO(page_type, b) 
						!= r->rcb_tran_id.db_low_tran) 
	 || (DM1B_VPT_GET_PAGE_SEQNO_MACRO(page_type, b) 
						!= r->rcb_seq_number))
    {
	DM1B_VPT_SET_PAGE_TRAN_ID_MACRO(page_type, b, r->rcb_tran_id);
	DM1B_VPT_SET_PAGE_SEQNO_MACRO(page_type, b, r->rcb_seq_number);
	DM1B_VPT_SET_PAGE_LG_ID_MACRO(page_type, b, r->rcb_slog_id_id);
        for (i = 0; i < length; i++ )
	    dmpp_vpt_clear_new_macro(page_type, lines, i); 
    }
}


/*{
** Name: dm1cxclean - Remove committed deleted entries from leaf page.
**
** Description:
**    This routine removes committed deleted entries from leaf page.
**
** Inputs:
**	b			- the page to use
**	page_size		- size of this table.
**	compression_type		Value indicating the compression type:
**					    DM1CX_UNCOMPRESSED
**					    DM1CX_COMPRESSED
**      xid                     - Array of commited transaction ids
**      xid_cnt                 - number of entries in xid array 
**
** Outputs:
**	Returns:
**	    E_DB_OK
**          E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Created.
**      27-feb-97 (stial01)
**          Instead of bitmap of bids, we are now called with list of 
**          committed tran ids.
**          Also, on leaf pages with overflow chain we never delete/reclaim
**          the last 'duplicated key value'.
**      10-mar-97 (stial01)
**          xid array is empty if xid_cnt is zero
**      07-apr-97 (stial01)
**          dm1cxclean() NonUnique primary btree (V2) dups span leaf pages,
**          not overflow chain. Remove code skipping clean of overflow key
**      15-jan-98 (stial01)
**          dm1cxclean() Removed unused err_code arg, log error if delete fails
*/
DB_STATUS
dm1cxclean(i4 page_type, i4 page_size, DMPP_PAGE *b,
		i4 compression_type,
		i4 update_mode,
		DMP_RCB *r)
{
    i4		    pos;
    DB_STATUS       status = E_DB_OK;
    char	    *tup_hdr;
    i4		    i;
    bool            clean_page = FALSE;
    DM_LINEID       *lp = DM1B_VPT_BT_SEQUENCE_MACRO(page_type, b);
    i4		    low_tran;
    i4		    tmpkids;
    u_i2	    lg_id = 0;
    DM_TID	    clean_tid;
    i4		    clean_partno;
    DMP_POS_INFO    *getpos;

    /* Deleted keys not supported on V1 btree pages */
    if (page_type == TCB_PG_V1)
	return(E_DB_OK);

    if (r && DM1B_POSITION_VALID_MACRO(r, RCB_P_GET))
	getpos = &r->rcb_pos_info[RCB_P_GET];
    else
	getpos = NULL;

    for (pos = 0; ; )
    {
	if (pos >= DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b))
	    break;

	/* Test for deleted row */
	if (dmpp_vpt_test_free_macro(page_type,
		DM1B_VPT_BT_SEQUENCE_MACRO(page_type, b), 
		(i4)pos + DM1B_OFFSEQ) == FALSE)
	{
	    pos++;
	    continue;
	}

	/* If row locking, make sure delete is committed */
	if (update_mode & DM1C_ROWLK)
	{
	    /*
	    ** Check for current transaction id (which is alive)
	    ** For space reclamation we only need to look at low tran
	    ** which is guaranteed to be unique for live transactions
	    */
	    tup_hdr = dm1b_vpt_entry_macro(page_type, b, (i4)pos);
	    MECOPY_CONST_4_MACRO(
		 (PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_lowtran_offset,
		 &low_tran);

	    if ( Dmpp_pgtype[page_type].dmpp_has_lgid )
	    {
	        MECOPY_CONST_2_MACRO(
		 (PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_lgid_offset,
		 &lg_id);
	    }

	    /* Check if delete is committed */
	    if (crow_locking(r))
	    {
		/*
		** Don't reclaim unless committed before consistent read
		** point in time described by the transactions "crib"
		*/
		if (!row_is_consistent(r, low_tran, lg_id))
		{
		    /* skip and don't delete */
		    pos++;
		    continue;
		}
	    }
	    else if ( IsTranActive(lg_id, low_tran) )
	    {
		pos++;
		continue;
	    }
	    /* committed/consistent, ok to reclaim */
	}

	if (getpos && DMPP_VPT_IS_CR_PAGE(page_type, b))
	{
	    /* Do NOT clean a key we are positioned on */
	    dm1cxtget(page_type, page_size, b, pos, 
			&clean_tid, &clean_partno);
	    
	    if (clean_tid.tid_i4 == getpos->tidp.tid_i4)
	    {
#ifdef xDEBUG
		TRdisplay("SKIP/Do not CLEAN (%d,%d) (%d,%d)\n",
		    DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b), pos,
		    clean_tid.tid_tid.tid_page, clean_tid.tid_tid.tid_line); 
#endif

		/* skip and don't delete */
		pos++;
		continue;
	    }
	}
	    
	tmpkids = DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b);
	dmpp_vpt_clear_free_macro(page_type, lp, (i4)pos + DM1B_OFFSEQ);
	clean_page = TRUE;
	status = xdelete(page_type, page_size, b, compression_type, (i4)pos);
	DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, b, DMPP_MODIFY);

	/*
	** sometimes xdelete won't delete the last dupkey
	** increment pos if we didn't xdelete did not decrement kids
	*/
	if (tmpkids == DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b))
	    pos++;

	if (status != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E2_BAD_INDEX_DEL, (DMP_RCB *)0, b,
			    page_type, page_size, (i4)pos);
	    break;
	}
    }

    /*
    ** Increment clean count if we reclaimed space, 
    ** causing entries to shift
    */
    if (clean_page)
	DM1B_VPT_INCR_BT_CLEAN_COUNT_MACRO(page_type, b);
    return (status);
}


/*{
** Name: dm1cxkeycount - Count keys on leaf page.
**
** Description:
**    This routine counts keys on leaf page. It assumes that a page lock is 
**    held. All deleted tuples are committed or were deleted by the
**    current transaction (in both cases, deleted keys should not be counted)
**
** Inputs:
**	b			- the page to use
**	page_size		- size of this table.
**
** Outputs:
**	Returns:
**          key count
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**      27-feb-97 (stial01)
**          Written.
*/
i4
dm1cxkeycount(i4 page_type, i4 page_size, DMPP_PAGE *b) 
{
    i4	child;
    i4     count = 0;

    for (child = 0; child < DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b); child++)
    {
	/* Test for deleted row */
	if (dmpp_vpt_test_free_macro(page_type, 
		DM1B_VPT_BT_SEQUENCE_MACRO(page_type, b),
		(i4)child + DM1B_OFFSEQ) == 0)
	{
	    count++;
	}
    }

    return (count);
}

/*{
** Name: dm1cxkperpage - Calculate keys per LEAF/INDEX Page.
**
** Description:
**    This routine calculates keys per page.
**
** Inputs:
**	page_size		- size of this table.
**
**      relcmptlvl              - DMF version of table
**
**                                This is currently only needed when
**                                calculating kperpage for INDEX pages.
** 
**                                During recovery of certain btree LEAF 
**                                operations (put/del/ovfl/free), we need
**                                but do not have kperleaf. Since there is
**                                currently no compatibility issue when
**                                calculating kperleaf, this routine can
**                                get called with an invalid relcmptlvl (0)
**                                for leaf pages only.
**               
**      spec                    - table structure: TCB_BTREE or TCB_RTREE
**      atts_cnt                - Number of attributes in entry
**      key_len                 - Length of key
**      indexpagetype           - leaf or index
**
** Outputs:
**	Returns:
**          keys per page
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**      07-Dec-98 (stial01)
**          Written.
**      09-feb-99 (stial01)
**          dm1cxkperpage() Check relcmptlvl when calculating kperpage for 
**          BTREE index page. Do not limit kperpage on INDEX pages.
**	22-Jan-2004 (jenjo02)
**	    Caller must supply tidsize, especially for LEAF/OVFLO.
**	25-Apr-2006 (jenjo02)
**	    Add Clustered parm. Clustered Leaf pages do not contain
**	    attribute information.
*/
i4
dm1cxkperpage(i4 page_type, i4 page_size,
	i4      relcmptlvl,
	i4 spec,
	i4 atts_cnt,
	i4 key_len,
	i4 indexpagetype,
	i2 tidsize,
	i4 Clustered)
{
    i4        key_over;
    i4        kperpage;
    i4        page_over;
    bool           is_leaf;
    i2	      TidSize;

    if (spec != TCB_BTREE && spec != TCB_RTREE)
    {
	TRdisplay(
	"kperpage page_size %d spec %d atts_cnt %d key_len %d indexpagetype %d\n",
	    page_size, spec, atts_cnt, key_len, indexpagetype);
	return (0);
    }
    if (indexpagetype != DM1B_PLEAF && indexpagetype != DM1B_POVFLO && 
	indexpagetype != DM1B_PSPRIG && indexpagetype != DM1B_PINDEX)
    {
	TRdisplay(
	"kperpage page_size %d spec %d atts_cnt %d key_len %d indexpagetype %d\n",
	    page_size, spec, atts_cnt, key_len, indexpagetype);
	return (0);
    }

    if (indexpagetype == DM1B_PLEAF || indexpagetype == DM1B_POVFLO)
    {
	is_leaf = TRUE;
	if ( tidsize == sizeof(DM_TID) || tidsize == sizeof(DM_TID8) )
	    TidSize = tidsize;
	else
	{
	    TRdisplay("%@ dm1cxkperpage: tidsize %d won't fly, setting to DM_TID\n",
			tidsize);
	    TidSize = sizeof(DM_TID);
	}
    }
    else
    {
	is_leaf = FALSE; 
	TidSize = sizeof(DM_TID);
	if (relcmptlvl == 0)
	{
	    TRdisplay("kperpage for index page must specify relcmptlvl\n");
	    return (0);
	}
    }

    key_over = key_len + TidSize + DM_VPT_SIZEOF_LINEID_MACRO(page_type);
    if ( is_leaf )
	key_over += Dmpp_pgtype[page_type].dmpp_tuphdr_size;
    else /* INDEX page */
    {
	/*
	** Ingres 2.0 mistakenly included tuple header into kperpage calcs
	** (BTREE only)
	*/
	if (relcmptlvl == DMF_T4_VERSION && spec == TCB_BTREE)
	    key_over += Dmpp_pgtype[page_type].dmpp_tuphdr_size;
    }

    /* Calculate page overhead */
    page_over = DM1B_VPT_OVER(page_type);

    /* Add overhead for attribute info on V2 BTREE non-Clustered leaf pages */
    if (spec == TCB_BTREE && page_type != TCB_PG_V1)
    {
	if ( is_leaf && !Clustered )
	{
	    page_over += (atts_cnt * sizeof (DM1B_ATTR)); 
	}
	else
	{
	    /* Ingres 2.0 mistakenly included attr info into kperpage calcs */
	    if (relcmptlvl == DMF_T4_VERSION)
		page_over += (atts_cnt * sizeof (DM1B_ATTR));
	}
    }

    kperpage = ((page_size - page_over) / key_over) - DM1B_OFFSEQ;

    /*
    ** Limit kperpage on Leaf pages
    ** For compatibility, limit kperpage on V2 DMF_T4_VERSION leaf and index pgs
    ** Note kperpage can never exceed 512 if page size is 2048,
    ** since even with the minimum key size (1), the tuple overhead is 7.
    */
    if ((kperpage > (DM_TIDEOF + 1) - DM1B_OFFSEQ)
	&& (is_leaf || (relcmptlvl == DMF_T4_VERSION)))
    {
	kperpage = (DM_TIDEOF + 1) - DM1B_OFFSEQ;
    }

    return (kperpage);
}


/*{
** Name: dm1cx_isnew - Was leaf entry modified by this cursor?
**
** Description:
**	Given a page and line table number, this accessor returns a boolean
**	flag stating whether the leaf entry was modified by the current cursor
**	or statement.  The leaf entry need not exist any longer; that is, it
**	could have been deleted.
**
** Inputs:
**      r				RCB containing tranid stored if
** 					deferred update, and sequence number
**					associated with this cursor
**      leafpage                      	Pointer to the leaf page.
**	child    			Line number of the leaf entry.
**
** Returns:
**	Whether the leaf entry was modified by the current statement.
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Created.
**          
*/
bool	    
dm1cx_isnew(
		DMP_RCB		*r,
		DMPP_PAGE      *leaf,
		i4		child)
{
    DMP_TCB	*t = r->rcb_tcb_ptr;
    i4		page_type = t->tcb_rel.relpgtype;
    DM_LINEID	*lp = DM1B_VPT_BT_SEQUENCE_MACRO(page_type, leaf);
    DM_TID	get_tid;
    char	*tup_hdr;
    u_i4	low_tran;
    u_i4	high_tran;
    i4		seq_num;
    DMP_RCB	*curr;
    i4		i;
    i4		partno;
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
    if (r->rcb_new_fullscan == FALSE && r->rcb_logging &&
	!LSN_GTE(DM1B_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, leaf),
			&r->rcb_oldest_lsn))
	return (FALSE);
    */

    if (page_type == TCB_PG_V1)
    {
	return (
	(dmpp_vpt_test_new_macro(page_type, lp, (i4)child + DM1B_OFFSEQ))
	&& (DM1B_V1_GET_TRAN_ID_HIGH_MACRO(leaf)
	    == r->rcb_tran_id.db_high_tran)
	&& (DM1B_V1_GET_TRAN_ID_LOW_MACRO(leaf)
	    == r->rcb_tran_id.db_low_tran)
	&& (DM1B_V1_GET_PAGE_SEQNO_MACRO(leaf) == r->rcb_seq_number));
    }
    else if (Dmpp_pgtype[page_type].dmpp_has_rowseq)
    {
	tup_hdr = dm1b_vpt_entry_macro(page_type, leaf, (i4)child);

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
	if (dmpp_vpt_test_new_macro(page_type, lp, (i4)child + DM1B_OFFSEQ)
	    && (DM1B_V2_GET_TRAN_ID_HIGH_MACRO(leaf)
		== r->rcb_tran_id.db_high_tran)
	    && (DM1B_V2_GET_TRAN_ID_LOW_MACRO(leaf)
		== r->rcb_tran_id.db_low_tran)
	    && (DM1B_V2_GET_PAGE_SEQNO_MACRO(leaf) == r->rcb_seq_number))
	    return (TRUE);

	tup_hdr = dm1b_vpt_entry_macro(page_type, leaf, (i4)child);

	/* Page tuple header may not be aligned! Extract low tran */
	MECOPY_CONST_4_MACRO(
	  (PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_lowtran_offset,
	  &low_tran);

	if (low_tran == r->rcb_tran_id.db_low_tran || r->rcb_new_fullscan)
	    check_newtids = TRUE;
    }

    if (!check_newtids)
	return (FALSE);

    /*
    ** MUST check rcb_new_tids
    **
    ** In dm1bindex we stored the TID, not the BID in the rcb
    ** compare the TID for this entry
    */
    dm1bvpt_kidget_macro(page_type, leaf, (i4)child, &get_tid, &partno);

    /*
    ** The "TID" of a Clustered leaf entry is always its BID,
    ** unless the entry has been reserved (0,DM_TIDEOF), or
    ** it's a Range entry.
    */
    if ( DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, leaf) & DMPP_CLUSTERED &&
	 child != DM1B_LRANGE && child != DM1B_RRANGE &&
	 get_tid.tid_tid.tid_page != 0  &&
	 get_tid.tid_tid.tid_line != DM_TIDEOF )
    {  
	get_tid.tid_tid.tid_page = DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type,leaf);
	get_tid.tid_tid.tid_line = child;
    }

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
** Name: dm1cx_dput - Perform deferred update processing
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
**	b				Pointer to the page
**      pos				position in line table
**      tid				tid for record updated
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
**     01-feb-2001 (stial01)
**	    Created (moved deferred update code from dm1cxput)
*/
dm1cx_dput(
		DMP_RCB *r,
		DMPP_PAGE *b,
		i4	pos,
		DM_TID  *tid,
		i4 *err_code)
{
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    i4		    page_type = t->tcb_rel.relpgtype;
    DM_LINEID	    *lp;
    char	    *tup_hdr;
    DB_STATUS	    status = E_DB_OK;

     /* flag page as dirty */
    DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, b, DMPP_MODIFY);

    /*
    ** Deferred update processing only on leaf pages (page type specific)
    */
    if (Dmpp_pgtype[page_type].dmpp_has_rowseq)
    {
	tup_hdr = dm1b_vpt_entry_macro(page_type, b, (i4)pos);
	/* put: if deferred, put tranid, sequence number in tuple header */
	MECOPY_CONST_4_MACRO(&r->rcb_tran_id.db_low_tran, 
	    (PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_lowtran_offset);
	if (page_type == TCB_PG_V2)
	{
	    MECOPY_CONST_4_MACRO(&r->rcb_tran_id.db_high_tran, 
	    (PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_hightran_offset);
	}
	MECOPY_CONST_4_MACRO(&r->rcb_seq_number, 
	    (PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_seqnum_offset);
	if ( Dmpp_pgtype[page_type].dmpp_has_lgid )
	{
	    MECOPY_CONST_2_MACRO(&r->rcb_slog_id_id,
	        (PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_lgid_offset);
	}
	return (E_DB_OK);
    }
    else if (page_type != TCB_PG_V1 
	    && (row_locking(r) || crow_locking(r) || 
	        t->tcb_sysrel || t->tcb_extended))
    {
	tup_hdr = dm1b_vpt_entry_macro(page_type, b, (i4)pos);
	MECOPY_CONST_4_MACRO(&r->rcb_tran_id.db_low_tran,
		(PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_lowtran_offset);

	if ( Dmpp_pgtype[page_type].dmpp_has_lgid )
	{
	    MECOPY_CONST_2_MACRO(&r->rcb_slog_id_id,
	        (PTR)tup_hdr + Dmpp_pgtype[page_type].dmpp_lgid_offset);
	}

	/* V3, V4, V5, V6, V7 pages keep new tids in rcb when row locking */
	status = defer_add_new(r, tid, err_code);
	return (status);
    }

    /* else  do clear operation and set new bit */

    /* Update the pages transaction id and sequence number */
    clear(r, b);

    /* Mark the line table entry to indicate that it was changed */
    lp = DM1B_VPT_BT_SEQUENCE_MACRO(page_type, b);
    dmpp_vpt_set_new_macro(page_type, lp, pos + DM1B_OFFSEQ); 
    return (E_DB_OK);
}


/*
** Name: dm1cx_txn_get		- Get transaction info for a leaf entry
**
** Description:
**	This routine fetches the transaction information for an entry
**	It is assumed that the entry is valid.
**
** Inputs:
**      pgtype
**	b			- the index page to use
**	child			- the lineid portion of the entry to be fetched
**
** Outputs:
**      row_low_tran
**      row_lg_id
**
** Returns:
**      DB_STATUS	
**
** History:
**      09-Jun-2010 (stial01)
**          Created.
*/
DB_STATUS
dm1cx_txn_get(i4 pgtype, DMPP_PAGE *b, i4 child, 
i4 *row_low_tran, u_i2 *row_lg_id)
{
    char	    *key_ptr;
    DB_STATUS       status;
    char	    *tup_hdr;

    status = E_DB_OK;
    if ( row_low_tran )
	*row_low_tran = 0;
    if ( row_lg_id )
	*row_lg_id = 0;
    if ((i4)child >= DM1B_VPT_GET_BT_KIDS_MACRO(pgtype, b))
	return (E_DB_ERROR);

    key_ptr = (char *)dm1bvpt_keyof_macro(pgtype, b, (i4)child);
    if ((pgtype != TCB_PG_V1) && 
	(DM1B_VPT_GET_PAGE_STAT_MACRO(pgtype, b) & DMPP_INDEX) == 0)
    {
	/* Test for deleted row */
	if (dmpp_vpt_test_free_macro(pgtype,  DM1B_V2_BT_SEQUENCE_MACRO(b),
		(i4)child + DM1B_OFFSEQ))
	{
	    status = E_DB_WARN; /* TID points to deleted record */
	}

	if ( row_low_tran || row_lg_id )
	{
	    tup_hdr = dm1b_vpt_entry_macro(pgtype, b, (i4)child);

	    if ( row_low_tran )
	    {
		MECOPY_CONST_4_MACRO(
		    (PTR)tup_hdr + Dmpp_pgtype[pgtype].dmpp_lowtran_offset,
		    row_low_tran);
	    }

	    if ( row_lg_id && Dmpp_pgtype[pgtype].dmpp_has_lgid )
	    {
		MECOPY_CONST_2_MACRO(
		    (PTR)tup_hdr + Dmpp_pgtype[pgtype].dmpp_lgid_offset,
		    row_lg_id);
	    }
	}
    }

    return (status);
}
