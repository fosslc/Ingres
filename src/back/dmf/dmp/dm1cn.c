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
#include    <dm1cn.h>
#include    <dm1u.h>
#include    <dm1b.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dmxe.h>
#include    <dmftrace.h>

/**
**
**  Name: DM1CN.C - Normal format Accessor routines to manipulate records 
**		    and data pages.
**
**  Description:
**      This file contains all the routines needed to manipulate 
**      data pages as a set of accessors. Since the manipulation 
**      is done via accessor functions a set needs to be provided for each
**	supported page type. These are then formed into the two accessor
**	structures referenced by the dm1c_getaccessors function:
**
**	dm1cn_plv	- page level vector for normal pages
**	dm1cn_tlv	- tuple level vecor for normal pages
**
**      The normal (dm1cn_) functions defined in this file are:
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
**	03-jun-92 (kwatts)
**	    Created, based on older dm1c.c file.
**	16-sep-1992 (jnash)
**	    Make dm1cn_format and dm1cn_load not static, include
**	    FUNC_EXTERNs in dm1cn.h.  Now used by dm1cs.c.
**	16-sep-1992 (jnash)
**	    Reduced logging project.
**	      - Add record_type, dbid, tabid and indxid params to 
**		dm1cn_allocate for use by LKshow call for tid lock check.
**	      - Move compression routines out of this layer.  
**		- Eliminate atts_array, atts_count params from 
**	   	  dm1cn_get, along w associated code.
**	     	- Move compression routines into dm1ccmpn.c
**	      - Eliminate dmpp_isempty(), collapse into dmpp_tuplecount.
**	26-oct-92 (rogerk)
**	    Fix comparisons of signed and unsigned values that generate ANSI C
**	    compile warnings.
**	26-oct-92 (rickh)
**	    Replaced references to DMP_ATTS with DB_ATTS.  Also replaced
**	    DM_CMP_LIST with DB_CMP_LIST.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project:
**	    Changed code in dm1cn_put which automatically initializes the 
**	    line table entry to not assume that new rows are added to pages 
**	    with incremental tids.  During recovery new rows may be added in
**	    reverse order and we have to correctly adjust page_next_line.
**      17-dec-1992 (jnash)
**          Fix free_space length boundary condition problem in dm1cn_allocate.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system typedefs
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**      13-sep-93 (smc)
**          Made TRdisplay use %p for pointers.
**      20-may-95 (hanch04)
**          Added include dmtcb.h
**	06-mar-1996 (stial01 for bryanp)
**	    Add page_size argument to dmpp_allocate, dmpp_check_page,
**		dmpp_format, dmpp_getfree, dmpp_reallocate,
**		dmpp_load, dmpp_put,
**		dmpp_reclen, and dmpp_tuplecount.
**	    Support 16K, 32K and 64K pages by forcing 4-byte row alignment.
**	06-mar-1996 (stial01 for bryanp)
**	    Don't allocate tuple buffers on the stack.
**	06-may-1996 (nanpr01 & thaju02)
**	    New Page Format Support:
**		Added page_size argument to dm1cn_clear function prototype.
**		Pass page_size argument to dmppn_get_offset_macro,
**		dmppn_put_offset_macro. Pass page_size, and page pointer
**		arguments to dmppn_test_new_macro, dmppn_set_new_macro, 
**		dmppn_clear_new_macro.(due to tuple header implementation,
**		moving the new and free bits from the line table entry to
**		the tuple header flags field)
**	20-may-1996 (ramra01)
**	    Added new param tup_info for the get/put accessor routines
**	20-jun-1996 (thaju02)
**	    Use dm0m_lfill() when filling page with page size greater
**	    than max value for u_i2.
**	23-sep-1996 (canor01)
**	    Moved global data definitions to dmpdata.c.
**	3-july-1997(angusm)
**	    Add new function dmpp_dput() - bug 79623, 80750 
**	24-july-1997(angusm)
**	    Move func proto for dmpp_dput() to dm1cn.h
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Delete accessor: added reclaim_space 
**          Put/load accessor: changed DMPP_TUPLE_INFO param to table_version
**      5-Mar-98 (allmi01/linke01)
**          Added NO_OPTIM for pym_us5 to correct corrupted databases
**      12-Mar-98 (allmi01/linke01)
**          Added NO_OPTIM for pym_us5 to correct corrupted databases
**	06-jun-1998 (nanpr01)
**	    There is no need to check the page size here since only 2k page
**	    can access this routine.
**	01-dec-1998 (nanpr01)
**	    Get rid of DMPP_TUPLE_INFO structure. Use DMPP_TUPLE_HDR instead.
**      09-feb-1998 (stial01)
**          dm1cn_ixsearch() Added relcmptlv param, not used in this page format
**      10-feb-99 (stial01)
**          dm1cn_load() Do not limit # entries on 2k ISAM INDEX pages
**      21-oct-99 (nanpr01)
**          More CPU optimization. Donot initialize version numbers for 
**	    2K pages.
**      05-dec-99 (nanpr01)
**          More CPU optimization. Pass the tuple header to the accssor 
**	    routines if available.
**      05-jun-2000 (stial01) 
**          update_mode parameter is a bit mask
**      25-may-2000 (stial01)
**         Updated accessor routines to be used for user tables and system 
**         catalogs (SIR 101807)
**	23-Jun-2000 (jenjo02)
**	    Added fill_option to dmpp_format() prototype, set by caller
**	    to DM1C_ZERO_FILL if dmpp_format should MEfill the page,
**	    DM1C_ZERO_FILLED if the page has already been MEfilled.
**	    The page overhead space is always zero-filled.
**      16-oct-2000 (stial01)
**          dm1cn_allocate() Fixed test for DM_SCONCUR_MAX (SIR 101807)
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**      30-oct-2000 (stial01)
**          Pass rcb to allocate,isnew page accessors, 
**          Moved space reclamation code to allocate accessor
**      28-nov-2000 (stial01)
**          dm1cn_get() return ptr to deleted row (used in dm1b check_reclaim)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Arg changes for page accessors (Variable Page Type SIR 102955)
**          No deferred processing for delete, it is done in opncalcost.c b59679
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**      28-may-2003 (stial01)
**          Added line_start parameter to dmpp_allocate accessor
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      21-feb-2005 (stial01)
**          Cross integrate 470401 (inifa01) for INGSRV2569 b111886
**          dmpp_delete: Trim line table if rollback, and data is compressed
**	25-Jan-2008 (thaju02)
**	    In recovery when trimming the line table, set the offset of
**	    the next_line entry to zero in dm1cn_delete. (B119823)
**	25-Aug-2009 (kschendel) 121804
**	    Need dmxe.h to satisfy gcc 4.3.
*/

/* NO_OPTIM = pym_us5 */


/*
**  Forward function references (used in accessor definition).
*/

/* Internal routines */

static VOID	    dm1cn_clear(
			DMP_RCB		*r,
			DMPP_PAGE       *page);


/*{
** Name: dm1cn_plv - Normal format page level vector of accessors
**
** Description:
**	This the the accessor structure that dm1c_getaccessors will use when
**	passed parameters asking for standard page accessors.
**
** History:
**      03_jun_92 (kwatts)
**	    Created.
**	16-oct-1992 (jnash)
**	    Reduced logging project.  Eliminate dm1cn_isempty.
**	03-nov-1992 (jnash)
**	    Reduced logging project.  Change params to dm1cn_reclen,
**	    modify dm1cn_reallocate.
**	23-sep-1996 (canor01)
**	    Moved to dmpdata.c.
*/

GLOBALREF DMPP_ACC_PLV dm1cn_plv;


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
**       record_size                    Amount of space needed.
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
**      08-nov-85 (jennifer)
**          Created for Jupiter.
**      03_jun_92 (kwatts)
**	    Adapted for MPF project.
**	16-sep-1992 (jnash)
**	    Reduced logging project.
**	    --  Add record_type, dbid and tabid params.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all page header access as macro for New Page Format project.
**	    Pass page_size argument to dmppn_get_offset_macro due to  tuple
**	    header implementation.
**      25-may-2000 (stial01)
**          dm1cn_allocate:allocate adapted from dm1cs_allocate 
**          -- Reclaim space from deleted rows.
**          -- If row locking (or phys locking) make sure delete is committed
*/
DB_STATUS    
dm1cn_allocate(
			DMPP_PAGE		*data,
			DMP_RCB                 *r,
			char			*record,
			i4			record_size,
			DM_TID			*tid,
			i4			line_start)
{
    DMP_TCB     *t = r->rcb_tcb_ptr;
    DM_LINEID   *lp = DMPPN_V1_PAGE_LINE_TAB_MACRO(data); 
    i4     i;
    i4     nextlno;
    i4	space;
    i4	ret_len;
    i4	offset;
    char       *del_row;
    DB_STATUS   status;
    i4          err_code;
    DMPP_TUPLE_HDR_V1 *hdr;
    u_i4	low_tran;

    tid->tid_tid.tid_page = DMPP_V1_GET_PAGE_PAGE_MACRO(data);
    nextlno = DMPP_V1_GET_PAGE_NEXT_LINE_MACRO(data);

    /*
    ** Check for the page pointer == NULL necessary?
    */
    if (data == (DMPP_PAGE *) NULL)
	space = t->tcb_rel.relpgsize - DMPPN_V1_OVERHEAD;
    else
    {
	offset = dmppn_v1_get_offset_macro(lp, nextlno);
	space = offset - (nextlno + nextlno) - DMPPN_V1_OVERHEAD;
    }

    /* 
    ** If slot is available and there is space on the page, use it 
    */
    if ( (space - 2 >= record_size) && (nextlno < DM_TIDEOF) )
    {
	tid->tid_tid.tid_line = nextlno;
	return (E_DB_OK);
    }
       
    /* 
    ** If enuf space and we are at TIDEOF, check for free tid.
    */
    if ( (space >= record_size) && (nextlno == DM_TIDEOF) )
    {
	for (i = 0; i < DM_TIDEOF; i++)
	{
	    if (dmppn_v1_get_offset_macro(lp, i))
	  	continue;
	    tid->tid_tid.tid_line = i;
	    return (E_DB_OK);
	}
    }

    /*
    ** Either not enuf space or no tids, must check for reclaimable space. 
    ** Note that algorithm checks for the total amount of free space
    ** on the page, but does not return a list of tids where
    ** the space resides.  For fixed length rows a single tid 
    ** is okay, but when compressed rows are included, "put" logic 
    ** will need to find and squish free space to make room for the new 
    ** row, reexecuting this algorithm.
    */

    for (i = line_start; i < nextlno; i++)
    {
	if (dmppn_v1_test_free_macro(lp, i) == 0)/* reclaimable row? */
	    continue;

	/* Row previously deleted, check if still locked. */
	/*
	** PSEUDO ROW LOCKING for catalogs and etabs
	** DELETE overlays row with low tran id
	** NOTE we cannot do real row locking with V1 pages
	*/
	if (t->tcb_sysrel || t->tcb_extended)
	{
	    /* Get tranid which overlays the deleted tuple */
	    del_row = (char *)data + dmppn_v1_get_offset_macro(lp, (i4)i);
	    hdr = (DMPP_TUPLE_HDR_V1 *)del_row;
	    MECOPY_CONST_4_MACRO(&hdr->low_tran, &low_tran);

	    /* If not same tranid, check if delete is committed */
	    if (low_tran != r->rcb_tran_id.db_low_tran)
	    {
		if ( IsTranActive(0, low_tran) )
		    continue;
	    }
	}

	/* 
	** This call is not strictly necessary for fixed length records
	** The current implementation only sets free bit if no compression
	*/
#ifdef xDEBUG
	dm1cn_reclen (t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, data, i, &ret_len);
	if (ret_len != record_size)   
	    dmd_check(E_DMF026_DMPP_PAGE_INCONS);
#endif

	tid->tid_tid.tid_line = i;  
	return (E_DB_OK);
    }

    /* No space */
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
dm1cn_isempty(
			DMPP_PAGE	*data,
			DMP_RCB		*rcb)
{
    register i4 i;
    DM_LINEID	*lp = DMPPN_V1_PAGE_LINE_TAB_MACRO(data);
    DMP_TCB	*t = rcb->rcb_tcb_ptr;
    char       *del_row;
    DB_STATUS   status;
    i4          err_code;
    DMPP_TUPLE_HDR_V1 *hdr;
    u_i4	row_low_tran;

    for (i = 0; i != DMPP_V1_GET_PAGE_NEXT_LINE_MACRO(data); ++i)
    {
	if (dmppn_v1_get_offset_macro(lp, i) == 0)
	    continue;
	
	if (! dmppn_v1_test_free_macro(lp, i) )
	    return (FALSE); /* not empty */
	
	/*
	** PSEUDO ROW LOCKING for catalogs and etabs
	** DELETE overlays row with low tran id
	** NOTE we cannot do real row locking with V1 pages
	*/
	if (t->tcb_sysrel || t->tcb_extended)
	{
	    /* Get tranid which overlays the deleted tuple */
	    del_row = (char *)data + dmppn_v1_get_offset_macro(lp, (i4)i);
	    hdr = (DMPP_TUPLE_HDR_V1 *)del_row;
	    MECOPY_CONST_4_MACRO(&hdr->low_tran, &row_low_tran);

	    /* Same low tranid, skip deleted tuple */
	    if (row_low_tran == rcb->rcb_tran_id.db_low_tran)
		continue;

	    /* Check if deleted row is committed */
	    if ( IsTranActive(0, row_low_tran) )
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
**      11-aug-89 (teg)
**          initial creation
**	02-oct-90 (teresa)
**	    fixed bug where page_next_line was set to the offset from 
**	    page_line_tab[page_next_line] instead of to page_next_line
**	    found via set_page_next_line().  Also fix bug where this routine
**	    failed to notice/report if this page was previously patched and
**	    we are doing the strict algorithm.
**	03-jun-92 (kwatts)
**          Re-created from local function in dm1u.c, above is dm1u.c
**	    history.
**	06-mar-1996 (stial01 for bryanp)
**	    Added page_size argument.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all page header access as macro for New Page Format project.
**	    Pass page_size to dmppn_get_offset_macro for implementing tuple
**	    header.
*/
/* 
** "Traditional" page format implementation
*/
DB_STATUS    
dm1cn_check_page(
			DM1U_CB			*dm1u,
			DMPP_PAGE		*page,
			i4			page_size)
{
    DM_LINEID	*lp = DMPPN_V1_PAGE_LINE_TAB_MACRO(page);
    i4	    min_offset;
    i4	    min_offset_line = 0;
    i4	    page_next_line = -1;
    i4	    last_line = 0;
    i4	    i;

    /*  Compute a value for page_next_line and it's offset. */

    for (min_offset = page_size, i = 0; i <= DM_TIDEOF; i++)
    {
	i4	offset;
	
	offset = dmppn_v1_get_offset_macro(lp, i);
	if (offset)
	{
	    if (offset < (DMPPN_V1_OVERHEAD - DM_V1_SIZEOF_LINEID_MACRO) ||
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
	(page_next_line != DMPP_V1_GET_PAGE_NEXT_LINE_MACRO(page)))
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

		DMPP_V1_SET_PAGE_NEXT_LINE_MACRO(page, page_next_line);
		return (E_DB_WARN);
	    }
	    else if (i <= DM_TIDEOF && last_line)
	    {
		for (i = 0; i < last_line; i++)
		    if (dmppn_v1_get_offset_macro(lp, i) ==
			dmppn_v1_get_offset_macro(lp, last_line))
		    {
			/*  Use viable duplicate last line. */

			DMPP_V1_SET_PAGE_NEXT_LINE_MACRO(page, last_line);
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
**    This routine assumes that when row locking, the caller owns a "row lock" 
**    on deleted rows.
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
**      08-nov-85 (jennifer)
**          Converted for Jupiter.
**	29-may-89 (rogerk)
**	    Added safety check in checking for amount of data to move when
**	    compressing out deleted space, since the check is likely to be free.
**	10-jul-89 (jas)
**	    Added multiple implementations for multiple page formats.
**	26-oct-92 (rogerk)
**	    Fix comparisons of signed and unsigned values that generate ANSI C
**	    compile warnings.
**	06-mar-1996 (stial01 for bryanp)
**	    Don't allocate page buffers on the stack.
**	    Use dm0m_lcopy to copy large amounts around.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all page header access as macro for New Page Format project.
**	    Pass page_size to dmppn_get_offset_macro, dmppn_put_offset_macro,
**	    for tuple header implementation. Pass page_size and page pointer
**	    to dmppn_set_new_macro.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Delete accessor: added reclaim_space 
**      25-may-2000 (stial01)
**          dm1cn_delete: adapted from dm1cs_delete:
**          --  If reclaim_space = false, just set the deleted bit.
**          --  If row locking protocols... overlay row with tranid
**	25-Jan-2008 (thaju02) 
**	    In recovery when trimming the line table, set the offset of
**	    the next_line entry to zero. (B119823)
*/
VOID	    
dm1cn_delete(
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
    DMPP_PAGE	    *p  = page;
    DM_LINEID	    *lp = DMPPN_V1_PAGE_LINE_TAB_MACRO(page);
    i4	    next_line = DMPP_V1_GET_PAGE_NEXT_LINE_MACRO(p);
    char            *endpt;
    i4         offset, end_offset, ith_offset;
    i4         i;
    i4         row_len;      
    char       *del_row;
    DMPP_TUPLE_HDR_V1 * hdr;

    /* If its not our tran, place ours there with invalid sequence 0 */
    if ( tran_id )
    {
	if ((DMPP_V1_GET_TRAN_ID_HIGH_MACRO(p) != tran_id->db_high_tran)
	   || (DMPP_V1_GET_TRAN_ID_LOW_MACRO(p) != tran_id->db_low_tran))
	{
	    DMPP_V1_SET_PAGE_TRAN_ID_MACRO(p, *tran_id);
	    DMPP_V1_SET_PAGE_SEQNO_MACRO(p, 0);
	}
    }

    if (!reclaim_space)
    {
	/* Set the free_space bit */
	dmppn_v1_set_free_macro(lp, (i4)tid->tid_tid.tid_line);
	

	/* Save the tran id for space reclamation checking */
	offset = dmppn_v1_get_offset_macro(lp, (i4)tid->tid_tid.tid_line);
	del_row = (char *)p + offset;

	/*
	** Put low tranid in only if row locking (or physical locking)....
	** NOTE: recovery must specify DM1C_ROWLK when phys or row locking
	*/
	if (update_mode & DM1C_ROWLK)
	{
	    /* Consistency check: row size must be >= sizeof tranid */
	    if (record_size < sizeof (DB_TRAN_ID))
		dmd_check(E_DMF026_DMPP_PAGE_INCONS);
	    hdr = (DMPP_TUPLE_HDR_V1 *)del_row;
	    MECOPY_CONST_4_MACRO(&tran_id->db_low_tran, &hdr->low_tran);
	}
    }
    else
    {
	char          *temp;

	/* consistency check, reclaim space must be false if row locking */
	if (update_mode & DM1C_ROWLK)
	    dmd_check(E_DMF026_DMPP_PAGE_INCONS);

	if ((temp = dm0m_pgalloc(page_size)) == 0)
	    return;

	offset = dmppn_v1_get_offset_macro(lp, (i4)tid->tid_tid.tid_line);

	end_offset = dmppn_v1_get_offset_macro(lp, (i4)next_line);

	endpt = (char *) p + end_offset;

	/* Update affected line numbers. */

	for (i = next_line, i++; --i >= 0; )
	{
	    ith_offset = dmppn_v1_get_offset_macro(lp, (i4)i);

	    if (ith_offset <= (u_i4)offset && ith_offset != 0)
	    {
		ith_offset += record_size;
		dmppn_v1_put_offset_macro(lp, (i4)i, ith_offset);
	    }
	}
	dmppn_v1_put_offset_macro(lp, (i4)tid->tid_tid.tid_line, 0);

	/*
	** If we have deleted a tuple from the middle of the data portion of the
	** page then compact the page.
	**
	** Checking that i > 0 rather than just non-zero adds the extra safety
	** of not trashing the page if this routine happens to be called with a
	** TID that specifies a non-existent tuple.
	*/
	if ((i = (char *)p + offset - endpt) > 0)
	{
	    /* 
	    ** Since the source and the destination overlap we move the 
	    ** source somewhere else first.
	    */
	    MEcopy(endpt, i, (PTR)temp);
	    MEcopy((PTR)temp, i, endpt + record_size);
	}

	/*
	** INGSRV 2569
	** Rollback compressed data may need to trim the line table
	*/
	if ((update_mode & DM1C_DMVE_COMP) &&
		tid->tid_tid.tid_line == next_line - 1)
	{
	    i4		next_offset;

	    next_offset = dmppn_v1_get_offset_macro(lp, next_line);
	    DMPP_V1_SET_PAGE_NEXT_LINE_MACRO(page, tid->tid_tid.tid_line);
	    dmppn_v1_put_offset_macro(lp, (i4)tid->tid_tid.tid_line, next_offset);
	    dmppn_v1_clear_flags_macro(lp, (i4)tid->tid_tid.tid_line);
	    dmppn_v1_put_offset_macro(lp, next_line, 0);   /* b119823 */
	}

	dm0m_pgdealloc(&temp);
    }

    DMPP_V1_SET_PAGE_STAT_MACRO(p, DMPP_MODIFY);

    return;
}

/*{
** Name: dmpp_format - Formats a page.
**
** Description:
**	This accessor formats a page for a table. Page may be
**	zero filled.
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
**      08-nov-85 (jennifer)
**          Converted for Jupiter.
**	12-jul-89 (jas)
**	    Converted to multiple implementations for multiple page formats.
**      19-jun-91, (paull)
**          Adjusted for MPF development
**	16-sep-1992 (jnash)
**	    Reduced logging project.
**	    -   Update header to relate that same routine is used
**		by syscat page accessors.
**	06-mar-1996 (stial01 for bryanp)
**	    Added page_size argument.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all page header access as macro for New Page Format project.
**	    Pass page_size arg to dmppn_put_offset_macro for tuple
**	    header implementation.
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
dm1cn_format(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE	*page,
			DM_PAGENO       pageno,
			i4		stat,
			i4		fill_option)

{
    DM_LINEID	    *lp = DMPPN_V1_PAGE_LINE_TAB_MACRO(page);

    if ( fill_option == DM1C_ZERO_FILL )
	MEfill(page_size, '\0',  (char * ) page);
    else
	MEfill(DMPPN_V1_OVERHEAD, '\0',  (char * ) page);

    dmppn_v1_put_offset_macro(lp, (i4)0, page_size);
    dmppn_v1_clear_flags_macro(lp, (i4)0);
    DMPP_V1_SET_PAGE_PAGE_MACRO(page, pageno);
    DMPP_V1_SET_PAGE_STAT_MACRO(page, stat);
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
**      08-nov-85 (jennifer)
**          Converted for Jupiter.
**	 7-nov-88 (rogerk)
**	    Check for record type DM1C_OCOMPRESSED and call dm1c_oldget.
**	29-may-89 (rogerk)
**	    Changed from being VOID to DB_STATUS.  Return status values
**	    from uncompress calls.
**	14-jul-89 (jas)
**	    Changed to multiple versions for multiple page format support.
**	7-mar-1991 (bryanp)
**	    Check passed-in record size against calculated size if table is
**	    compressed and complain if lengths differ. Pass additional argument
**	    to dm1c_uncomp_rec so that it can detect buffer overflow.
**      19-jun-91 (paull)
**          Adjusted for MPF development 
**	12-oct-1992 (jnash)
**	    Reduced logging project.
**	      - In moving compression out of this layer, eliminate 
**		atts_array and atts_count parameters and associate 
**		compression code.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all page header access as macro for New Page Format project.
**          Pass page_size to dmppn_get_offset_macro for tuple header
**          implementation.
**	20-may-1996 (ramra01)
**	    Added new param tup_info to the get accessor routine
**      25-may-2000 (stial01)
**          dm1cn_get: adapted from dm1cs_get 
**          -- Add check of free_space bit to ensure row exists.
**	15-Jan-2010 (jonj)
**	    For compatiblity with dm1cnv2_get, zero row_version,
**	    row_low_tran if present.
*/
DB_STATUS    
dm1cn_get(
	    i4			page_type,
	    i4			page_size,
	    DMPP_PAGE		*page,
	    DM_TID		*tid,
	    i4			*record_size,
	    char		**record,
	    i4			*row_version,
	    u_i4		*row_low_tran,
	    u_i2		*row_lg_id,
	    DMPP_SEG_HDR	*seg_hdr)
{
    i4	offset;
    DM_LINEID   *lp = DMPPN_V1_PAGE_LINE_TAB_MACRO(page);

    /* Zero return items not available on V1 pages */
    if ( row_version )
        *row_version = 0;
    if ( row_low_tran )
        *row_low_tran = 0;
    if ( row_lg_id )
        *row_lg_id = 0;

    /*	Check for bad line entry. */

    if ( tid->tid_tid.tid_line < 
		(u_i2) DMPP_V1_GET_PAGE_NEXT_LINE_MACRO(page)
	&& (offset = dmppn_v1_get_offset_macro(lp, 
		(i4)tid->tid_tid.tid_line))
	&& (! dmppn_v1_test_free_macro(lp, (i4)tid->tid_tid.tid_line)) )
    {
	*record = (char *)page + offset;
	return (E_DB_OK);
    }

    /* free_space bit indicates row deleted, not bad tid */
    if ( tid->tid_tid.tid_line >= (u_i2)DMPP_V1_GET_PAGE_NEXT_LINE_MACRO(page))
	return (E_DB_ERROR);	/* TID out of range */
    else
    {
	*record = NULL;
	return (E_DB_WARN);	/* TID points to empty line table entry*/
    }
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
**      18-jul-89 (jas)
**          Written for CAFS project.
**      20-jun-91 (paull)
**          Amended for MPF development
**	17-dec-1992 (jnash)
**	    Make function static to quiet compiler.
**	06-mar-1996 (stial01 for bryanp)
**	    Added page_size argument.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all page header access as macro for New Page Format project.
**          Pass page_size to dmppn_get_offset_macro for tuple header
**          implementation.
*/
/*
** "Traditional" page format implementation:
*/
i4	    
dm1cn_getfree(
			DMPP_PAGE	*page,
			i4		page_size)
{
    i4  nextline;
    DM_LINEID	*lp;
    i4	offset;

    if (page == (DMPP_PAGE *) NULL)
	/* Return the standard free space on an empty page */
	return(page_size - DMPPN_V1_OVERHEAD);

    /* OK, we have a real page to work with */

    nextline = DMPP_V1_GET_PAGE_NEXT_LINE_MACRO(page);
    if (nextline >= DM_TIDEOF)
	return 0;

    lp = DMPPN_V1_PAGE_LINE_TAB_MACRO(page);
    offset = dmppn_v1_get_offset_macro(lp, nextline);

    return (offset - (nextline * DM_V1_SIZEOF_LINEID_MACRO) - 
	  DMPPN_V1_OVERHEAD - DM_V1_SIZEOF_LINEID_MACRO);
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
**		20-jun-91 (paull)
**               Re-created from local function in dm1u.c
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all page header access as macro for New Page Format project.
**          Pass page_size to dmppn_get_offset_macro for tuple header
**          implementation.
**      25-may-2000 (stial01)
**          dm1cn_get_offset: adapted from dm1cs_get_offset
**          -- Add check of free_space bit to ensure row exists.
*/
/* 
** "Traditional" page format implementation
*/
i4	    
dm1cn_get_offset(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE	*page,
			i4		item)
{
    DM_LINEID	*lp = DMPPN_V1_PAGE_LINE_TAB_MACRO(page);
    i4	offset;

    if (dmppn_v1_test_free_macro(lp, (i4)item))
	return( 0 );

    offset = dmppn_v1_get_offset_macro(lp, item);

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
**      18-jul-89 (jas)
**          Written for CAFS project.
**      20-jun-91 (paull)
**          Adapted for MPF development
**	16-sep-1992 (jnash)
**	    Reduced logging project.
**	    -   Update header to relate that same routine is used
**		by syscat page accessors.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all page header access as macro for New Page Format project.
**	    Pass page_size and page pointer args to dmppn_test_new_macro
**	    due to tuple header implementation.
*/
i4	    
dm1cn_isnew(
		DMP_RCB		*r,
		DMPP_PAGE	*page,
		i4		line_num)
{
    DM_LINEID	*lp = DMPPN_V1_PAGE_LINE_TAB_MACRO(page);

    return (dmppn_v1_test_new_macro(lp, line_num) &&
	    (DMPP_V1_GET_TRAN_ID_HIGH_MACRO(page) == 
					r->rcb_tran_id.db_high_tran) &&
	    (DMPP_V1_GET_TRAN_ID_LOW_MACRO(page) == 
					r->rcb_tran_id.db_low_tran) && 
	    (DMPP_V1_GET_PAGE_SEQNO_MACRO(page) == r->rcb_seq_number));
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
**      relcmptlvl              DMF version of table (Not used for this pg fmt)
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
**      18-jul-89 (jas)
**          Swiped from the ISAM code for the CAFS project.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all page header access as macro for New Page Format project.
**          Pass page_size to dmppn_get_offset_macro for tuple header
**          implementation.
**      09-feb-1998 (stial01)
**          dm1cn_ixsearch() Added relcmptlv param, not used in this page format
*/
/*
** "Traditional" page format implementation:
*/
i4	    
dm1cn_ixsearch(
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
    DM_LINEID	*lp = DMPPN_V1_PAGE_LINE_TAB_MACRO(page);

    for (bottom = 0, top = (DMPP_V1_GET_PAGE_NEXT_LINE_MACRO(page) - 1);
	 top != bottom;)
    {
	mid = ((1 + top - bottom) >> 1) + bottom;
	offset = dmppn_v1_get_offset_macro(lp, mid);
	s = adt_kkcmp(adf_cb, keys_given, keyatts, key, (char *) page + offset,
			&compare);
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
**	 record_type			DM1C_LOAD_xxx record type, indicates
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
**      08-nov-85 (jennifer)
**          Created for Jupiter.
**	16-sep-1992 (jnash)
**	    Reduced logging project.
**	    -   Update header to relate that same routine is used
**		by syscat page accessors.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all page header access as macro for New Page Format project.
**          Pass page_size to dmppn_get_offset_macro, dmppn_put_offset_macro
**	    for tuple header implementation.
**	21-may-1996 (ramra01)
**	    Added new param to the load accessor routine
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          load accessor: changed DMPP_TUPLE_INFO param to table_version
**      10-feb-99 (stial01)
**          dm1cn_load() Do not limit # entries on 2k ISAM INDEX pages
**	22-Feb-2009 (kschendel) SIR 122739
**	    record type is not a compression type, pass a distinct
**	    DM1C-xxx flag to say isam or not.
*/
DB_STATUS    
dm1cn_load(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE       *data,
			char		*record,
			i4		record_size,
			i4		record_type,
			i4		fill,
			DM_TID		*tid,
			u_i2		current_table_version,
			DMPP_SEG_HDR	*seg_hdr)
{
    DM_LINEID	*lp = DMPPN_V1_PAGE_LINE_TAB_MACRO(data);
    i4     nextlno = DMPP_V1_GET_PAGE_NEXT_LINE_MACRO(data);
    i4	offset;
    i4	free;
    char	*rec_location;
    
    offset = dmppn_v1_get_offset_macro(lp, (i4) nextlno);

    free = offset   - (nextlno * DM_V1_SIZEOF_LINEID_MACRO)
		    - (DMPPN_V1_OVERHEAD + DM_V1_SIZEOF_LINEID_MACRO);

    /*	Check for available space. */
    if (free < record_size ||
	(fill && nextlno && free < fill))
    {
	return (E_DB_WARN);
    }

    /*
    ** Limit the number of entries on DATA pages
    ** Do not limit # entries on 2k ISAM INDEX pages
    */
    if ((nextlno >= DM_TIDEOF) && (record_type != DM1C_LOAD_ISAMINDEX))
    {
	return (E_DB_WARN);
    }

    /*	Add record to end of line table. */
    
    offset -= record_size;
    rec_location = (char * )data + offset;
    dmppn_v1_put_offset_macro(lp, (i4)nextlno, offset);
    dmppn_v1_put_offset_macro(lp, (i4)(nextlno + 1), offset);
    DMPP_V1_INCR_PAGE_NEXT_LINE_MACRO(data);
    DMPP_V1_SET_PAGE_STAT_MACRO(data, DMPP_MODIFY);
    if (tid)
    {
	tid->tid_tid.tid_page = DMPP_V1_GET_PAGE_PAGE_MACRO(data);
	tid->tid_tid.tid_line = nextlno;
    }

    MEcopy(record, record_size, rec_location);

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
**    The put routine is complicated by the possibility that the allocate
**    routine has selected a previously deleted tid for which the delete bit
**    in the line table was set, but the data was not shifted. At this time
**    we assume that delete without shifting the data is only possible for
**    uncompressed data, and that the previously deleted row is exactly 
**    the same size as the one we are inserting. We will fail (dmd_check'ing)
**    if this is not the case.
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
**      08-nov-85 (jennifer)
**          Converted for Jupiter.
**	14-jul-89 (jas)
**	    Added multiple versions for multiple page format support.
**	14-jun-1991 (Derek)
**	    Automatically initialize new line table entry on put.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project:
**	    Changed code which automatically initializes the line table
**	    entry to not assume that new rows are added to pages with
**	    incremental tids.  During recovery new rows may be added in
**	    reverse order and we have to correctly adjust page_next_line.
**	17-dec-1992 (jnash)
**	    Cast tid_line to i4 to quiet compiler.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all page header access as macro for New Page Format project.
**          Pass page_size to dmppn_get_offset_macro, dmppn_put_offset_macro
**	    for tuple header implementation. Pass page_size and page pointer
**	    to dmppn_set_new_macro.
**      20-may-1996 (ramra01)
**          Added new param tup_info to the put accessor routine
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Put accessor: changed DMPP_TUPLE_INFO param to table_version
**	22-Feb-2008 (kschendel) SIR 122739
**	    Remove unused record type param.
*/
VOID	    
dm1cn_put(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE	*page,
			i4     	update_mode,
			DB_TRAN_ID  	*tran_id,
			u_i2		lg_id,
			DM_TID	    	*tid,
			i4	    	record_size,
			char	    	*record,
			u_i2		current_table_version,
			DMPP_SEG_HDR	*seg_hdr)
{
    DMPP_PAGE		*p = page;
    DM_LINEID		*lp = DMPPN_V1_PAGE_LINE_TAB_MACRO(page);
    i4		next_line = DMPP_V1_GET_PAGE_NEXT_LINE_MACRO(p);
    char		*record_location;
    i4		offset;
    i4	free_space;

    /* If its not our tran, place ours there with invalid sequence 0 */
    if ( tran_id )
    {
	if ((DMPP_V1_GET_TRAN_ID_HIGH_MACRO(p) != tran_id->db_high_tran)
	   || (DMPP_V1_GET_TRAN_ID_LOW_MACRO(p) != tran_id->db_low_tran))
	{
	    DMPP_V1_SET_PAGE_TRAN_ID_MACRO(p, *tran_id);
	    DMPP_V1_SET_PAGE_SEQNO_MACRO(p, 0);
	}
    }

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
	offset = dmppn_v1_get_offset_macro(lp, (i4)next_line);

	offset -= record_size;
	record_location = (char *)p + offset;

	DMPP_V1_SET_PAGE_NEXT_LINE_MACRO(p, tid->tid_tid.tid_line + 1);
	dmppn_v1_put_offset_macro(lp, 
		(i4)DMPP_V1_GET_PAGE_NEXT_LINE_MACRO(p), offset);

	/*
	** Initialize the offset to the new entry.
	*/
	dmppn_v1_put_offset_macro(lp, 
		(i4)tid->tid_tid.tid_line, offset);

	/*
	** If the old page_next_line slot was not the same spot as the one 
	** just allocated, then we have to zero out that old slot.
	*/
	if (next_line != tid->tid_tid.tid_line)
	    dmppn_v1_put_offset_macro(lp, (i4)next_line, 0);

    }
    else if (dmppn_v1_get_offset_macro(lp, tid->tid_tid.tid_line) == 0)
    {
	/* 
	** Tuple using currently empty line table entry.
	** Allocate space from free space area.
 	*/
	offset = dmppn_v1_get_offset_macro(lp, (i4)next_line);

	free_space = offset - (next_line + next_line) - DMPPN_V1_OVERHEAD;
	offset -= record_size;
	record_location = (char *)p + offset;

	if (free_space < record_size)
	{
	    /* This should not happen (yet) */
	    TRdisplay("dm1cs_put: Insufficient space for insert\n");
	    dmd_check(E_DMF026_DMPP_PAGE_INCONS);       
	}

	dmppn_v1_put_offset_macro(lp, (i4)next_line, offset);
	dmppn_v1_put_offset_macro(lp, (i4) tid->tid_tid.tid_line, 
		offset);
    }
    else if (dmppn_v1_test_free_macro(lp, (i4)tid->tid_tid.tid_line))
    {
	/* 
	** Tid points to deleted row.  
	** We just overlay new row on top of old, first checking that 
	** the lengths are the same.  When we introduce support for 
	** variable length rows we will need to scavenge here.
	*/
	i4	 row_len;

   	dm1cn_reclen(page_type, page_size, p, tid->tid_tid.tid_line, &row_len);   
	if (record_size != row_len)
	{
	    TRdisplay("dm1cs_put: tid row len not equal to put row len\n");
	    TRdisplay("   tid row size: %d, new record len: %d\n", 
			row_len, record_size);
	    dmd_check(E_DMF026_DMPP_PAGE_INCONS);       
	}
    	offset = dmppn_v1_get_offset_macro(lp, (i4)tid->tid_tid.tid_line);
	record_location = (char *)p + offset;
    } 
    else
    {
	/* in place replace */
	offset = dmppn_v1_get_offset_macro(lp, (i4)tid->tid_tid.tid_line);
	record_location = (char *)p + offset;
    }

    /* Move the tuple. */
    MEcopy(record, record_size, record_location);

    /* Mark page as dirty */
    DMPP_V1_SET_PAGE_STAT_MACRO(p, DMPP_MODIFY);

    dmppn_v1_clear_free_macro(lp, (i4)tid->tid_tid.tid_line);

    /* 
    ** Update the pages transaction id and sequence number.
    ** Do this only RCB's openned as deferred update.
    */

    if ((update_mode & DM1C_DEFERRED) == 0)
	return;

    return;
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
**      23-aug-89 (jas)
**          Swiped from recovery code.
**      20-jun-91, (paull)
**          Amended for MPF development.
**	26-oct-92 (rogerk)
**	    Fix comparisons of signed and unsigned values that generate ANSI C
**	    compile warnings.
**      03-nov-1992 (jnash)
**          Rewritten for reduced logging project.  
**      17-dec-1992 (jnash)
**          Fix free_space length boundary condition problem, add diagnostic
**	    TRdisplay.
**	06-mar-1996 (stial01 for bryanp)
**	    Add page_size argument to dm1cn_reallocate.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all page header access as macro for New Page Format project.
**          Pass page_size to dmppn_get_offset_macro for tuple header
**          implementation.
*/
DB_STATUS 
dm1cn_reallocate(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE	*page,
			i4		line_num,
			i4		reclen)
{
    i4 	ret_len;
    i4 	free_space;
    i4	offset;
    i4	nextlno = DMPP_V1_GET_PAGE_NEXT_LINE_MACRO(page);
    DM_LINEID	*lp = DMPPN_V1_PAGE_LINE_TAB_MACRO(page);
    DMPP_PAGE   *p = page;
    /*
    ** If line number is valid and space has been allocated there, 
    ** check that that space is sufficient.  Note this also works
    ** for system catalog rows.
    */

    if ( (line_num < nextlno) &&
         (dmppn_v1_get_offset_macro(lp, line_num) != 0) ) 
    {
	dm1cn_reclen(page_type, page_size, page, line_num, &ret_len);
 	if (reclen == ret_len)
	    return(E_DB_OK);

	return(E_DB_WARN);
    }

    /*
    ** Either the offset is zero or the line number points beyond the
    ** end of the page, check for sufficient space in the free space area.
    */
    offset = dmppn_v1_get_offset_macro(lp, nextlno);

    free_space = offset	- (nextlno + nextlno) - DMPPN_V1_OVERHEAD;

    if (reclen > free_space) 
    {
#ifdef xDEBUG
	TRdisplay(
	"dm1cn_reallocate fail: page: %d, lineno %d, reclen: %d, free: %d\n",
	    DMPP_V1_GET_PAGE_PAGE_MACRO(page), line_num, reclen, 
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
**      08-nov-85 (jennifer)
**          Converted for Jupiter.
**	14-jul-89 (jas)
**	    Added multiple versions for multiple page format support.
**      19-jun-91 (paull)
**          Adjusted for MPF development.
**	26-oct-92 (rogerk)
**	    Fix comparisons of signed and unsigned values that generate ANSI C
**	    compile warnings.
**	03-nov-1992 (jnash)
**	    Reduced logging project.  Change "tid" param to "lineno" to allow
**	    routine to be used within dm1cs.c.
**	06-mar-1996 (stial01 for bryanp)
**	    Added page_size argument.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all page header access as macro for New Page Format project.
**          Pass page_size to dmppn_get_offset_macro for tuple header
**          implementation.
*/
/*
** "Traditional" page format:
*/
VOID	    
dm1cn_reclen(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE	*page,
			i4	    lineno,
			i4     *record_size)
{
    i4	    i,j;
    i4	    lineoff;
    i4	    nextoff;
    i4	    jth_off;
    DM_LINEID	    *lp = DMPPN_V1_PAGE_LINE_TAB_MACRO(page);

    /* Find offset for line number */
    lineoff = dmppn_v1_get_offset_macro(lp, lineno);

    /* Find smallest tuple offset larger than lineoff; if none, use
    ** page_size (end of page).
    */
    nextoff = page_size;
    for (j = 0, i = DMPP_V1_GET_PAGE_NEXT_LINE_MACRO(page); --i >= 0; 
	 j++)
    {
	jth_off = dmppn_v1_get_offset_macro(lp, (i4)j);

        if (jth_off <= (u_i4)lineoff)
            continue;
        if (jth_off < (u_i4)nextoff)
            nextoff = jth_off;
    }
    *record_size = (nextoff - lineoff );
}

/*{
** Name: dmpp_tuplecount - Return number of tuples on a page.
**
** Description:
**    Given a page, this accessor returns the number of tuples currently
**    on the page.
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
**      18-jul-89 (jas)
**          Written for CAFS project.
**      20-jun-91 (paull)
**          Amended for MPF development.
**	16-oct-1992 (jnash)
**	    Reduced logging project.  Optimize for zero tuples on page,
**	    now this routine is also called where dmpp_isempty used to
**	    be called.
**	06-mar-1996 (stial01 for bryanp)
**	    Added page_size argument.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all page header access as macro for New Page Format project.
**          Pass page_size to dmppn_get_offset_macro for tuple header
**          implementation.
**      25-may-2000 (stial01)
**          dm1cn_tuplecount: adapted from dm1cs_tuplecount 
*/
/*
** "Traditional" page format implementation:
*/
i4	    
dm1cn_tuplecount(
			DMPP_PAGE	*page,
			i4		page_size)
{
    register i4  i;
    register i4  count = 0;
    DM_LINEID	*lp = DMPPN_V1_PAGE_LINE_TAB_MACRO(page);

    if (DM_V1_GET_LINEID_MACRO(lp,
                DMPP_V1_GET_PAGE_NEXT_LINE_MACRO(page)) == page_size)
        return(0);

    for (i = 0; i != DMPP_V1_GET_PAGE_NEXT_LINE_MACRO(page); ++i)
	if ( (dmppn_v1_get_offset_macro(lp, i) != 0) &&
	     (! dmppn_v1_test_free_macro(lp, i) ) )   
							/* NOT a deleted row */
	    ++count;

    return(count);
}

/*{
** Name: dmpp_dput - Perform deferred update processing
**
** Description:
** 	Do deferred update processing. To be called after put or replace
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
*/
DB_STATUS
dm1cn_dput(
DMP_RCB		*rcb,
DMPP_PAGE 	*page, 
DM_TID 		*tid,
i4		*err_code)
{
    
    DM_LINEID   *lp = DMPPN_V1_PAGE_LINE_TAB_MACRO(page);

    /* flag page as dirty */

    DMPP_V1_SET_PAGE_STAT_MACRO(page, DMPP_MODIFY);
    
    dm1cn_clear(rcb, page);
    dmppn_v1_set_new_macro(lp, tid->tid_tid.tid_line);

    return(E_DB_OK);
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
**      08-nov-85 (jennifer)
**          Converted for Jupiter.
**	10-jul-89 (jas)
**	    Converted to multiple versions for multiple page formats.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all page header access as macro for New Page Format project.
**	    Pass page_size and page pointer to dmppn_clear_new_macro.
*/
/*
** "Traditional" page format:
*/
static VOID	    
dm1cn_clear(
			DMP_RCB		*r,
			DMPP_PAGE       *page)
{
    i4	i; 
    i4  length;
    DM_LINEID   *lp;

    /* Update the page's transaction id and sequence number. */
    if ((DMPP_V1_GET_TRAN_ID_HIGH_MACRO(page) != r->rcb_tran_id.db_high_tran)
       || (DMPP_V1_GET_TRAN_ID_LOW_MACRO(page) != r->rcb_tran_id.db_low_tran)
       || (DMPP_V1_GET_PAGE_SEQNO_MACRO(page) != r->rcb_seq_number))
    {
	length = (i4) DMPP_V1_GET_PAGE_NEXT_LINE_MACRO(page);
	lp = DMPPN_V1_PAGE_LINE_TAB_MACRO(page);

	DMPP_V1_SET_PAGE_TRAN_ID_MACRO(page, (r->rcb_tran_id));
	DMPP_V1_SET_PAGE_SEQNO_MACRO(page, r->rcb_seq_number);
        for (i = 0; i < length; i++ )
            dmppn_v1_clear_new_macro(lp, (i4)i); 
    }
}

DB_STATUS	    
dm1cn_clean(
i4		page_type,
i4		page_size,
DMPP_PAGE	*page,
DB_TRAN_ID	*tranid,
i4		lk_type,
i4		*avail_space)
{
    return (E_DB_OK);
}
