/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <pc.h>
#include    <tr.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <di.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmpl.h>
#include    <dmpp.h>
#include    <dmp.h>
#include    <dm2f.h>
#include    <dm0p.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm1c.h>
#include    <dm1cx.h>
#include    <dm1p.h>
#include    <dmftrace.h>
#include    <dmd.h>

/**
**
**  Name: DM1BMERGE.C - Routines needed to "merge" BTREE table.
**
**  Description:
**      This file contains all the routines needed to reclaim BTREE
**      data and index pages which contain little or no data.  This routine 
**      is called by the dmu modify when the merge option is specified.  It 
**      is assumed that the table is exclusively locked, therefore,
**      recovery is not a major issue.
**
**      The routines contained in this file are:
**
**      dm1bmmerge      - Descend the entire BTREE and merge
**                        all siblings to reclaim space.
**
**  History:
**      06-mar-86 (jennifer)
**          Changed for Jupiter.
**	14-jun-1991 (Derek)
**	    Quite a few changes, foremost was the fix the bug that caused
**	    certain types of overflow leaf pages not to be reclaimed during
**	    a merge.  Also, reduced the number of fix/unfix calls required
**	    to perform a merge.  Add call to new before image handling
**	    routines.
**	19-aug-1991 (bryanp)
**	    Added support for Btree index compression.
**	15-oct-1991 (mikem)
**	    Eliminate unix compiler warnings in join_ovfl().
**	07-jul-92 (jrb)
**	    Prototype DMF.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	16-oct-1992 (jnash)
**	    Reduced logging project.
**	      - Use dmpp_tuplecount() instead of dmpp_isempty().
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Changed arguments to dm0p_mutex and
**	    dm0p_unmutex calls.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	02-jan-1993 (jnash)
** 	    More reduced logging work.
**	     - Include LGreserve() calls.
**	30-feb-1993 (rmuth)
**	    - In 6.5 when we first issue this command on pre 6.5 table
**	      we scan the table looking for disassociated data pages to
**	      add to the FHDR/FMAP. As this code is only used this one
**	      time remove the code for simplicity, this means the user
**	      will have to modify table to reclaim these pages.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed obsolete rcb_bt_bi_logging and rcb_merge_logging fields.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	24-may-1993 (rogerk)
**	    Changed operation type on dm0l_bi calls to be JOIN instead of
**	    NEWROOT.  This caused records to be reported incorrectly in
**	    logdump.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() call.
**	06-mar-1996 (stial01 for bryanp)
**	    When reserving space for DM0L_BI and DM0L_AI records, allow for
**		variable sized pages.
**	    Pass page size argument to dm0l_bi, dm0l_ai.
**	    Pass tcb_rel.relpgsize as the page_size argument to dmpp_tuplecount.
**	    Pass page size argument to the dm1cx* accessor functions.
**      01-may-1996 (stial01)
**          LGreserve adjustments for removal of bi_page from DM0L_BI
**          LGreserve adjustments for removal of ai_page from DM0L_AI
**	10-jan-1997 (nanpr01)
**	    Added back the space holders to shield it from misalignment.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      15-jan-1998 (stial01)
**          Clean deleted tuples during JOIN.
**      07-Dec-1998 (stial01)
**          Added kperleaf to distinguish from kperpage (keys per index page)
**      12-apr-1999 (stial01)
**          Btree secondary index: non-key fields only on leaf pages
**          Different att/key info for LEAF vs. INDEX pages
**	16-aug-1999 (nanpr01)
**	    Implementation of SIR 92299. Enabaling journalling on a
**	    online checkpoint.
**      12-nov-1999 (stial01)
**          Changes to how btree key info is stored in tcb
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Arg changes for page accessors (Variable Page Types SIR 102955)
**	22-Jan-2004 (jenjo02)
**	    A host of changes for Partitioning and Global indexes,
**	    particularly row-locking with TID8s (which include
**	    partition number (search for "partno") along with TID).
**	    LEAF pages in Global indexes will contain DM_TID8s 
**	    rather than DM_TIDs; the TID size is now stored on
**	    the page (4 for all but LEAF, 8 for TID8 leaves).
**      31-jan-05 (stial01)
**          Added flags arg to dm0p_mutex call(s).
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	02-Dec-2008 (jonj)
**	    SIR 120874: dm1b_? functions converted to DB_ERROR *, use
**	    new form uleFormat.
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1p_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**      12-oct-2009 (stial01)
**          Use DMPP_PAGE for all page types.
**	15-Jan-2010 (jonj,stial01)
**	    SIR 121619 MVCC: Replace DMPP_PAGE* with DMP_PINFO* where needed.
**	23-Feb-2010 (stial01)
**          join() pass rcb to dm1cxclean
**/


/*
**  Forward and/or External function references.
*/

static	DB_STATUS descend(
    DMP_RCB       *rcb,
    DMP_PINFO	  *parentPinfo,
    i4	  index_fill,
    i4       leaf_fill,
    DB_ERROR      *dberr);

static	DB_STATUS stitch(
    DMP_RCB       *rcb,
    DMP_PINFO     *parentPinfo,
    i4       pos,
    i4	  index_fill,
    i4       leaf_fill,
    DB_ERROR      *dberr);
    
static DB_STATUS join(
    DMP_RCB       *rcb,
    DMP_PINFO	  *currentPinfo,
    i4       curpos,
    DMP_PINFO	  *parentPinfo,
    i4	  index_fill,
    i4       leaf_fill,
    DB_ERROR      *dberr);
    
static DB_STATUS join_ovfl(
    DMP_RCB       *rcb,
    DMP_PINFO	  *parentPinfo,
    i4	  leaf_fill,
    DB_ERROR      *dberr);
    
static DB_STATUS ridroot(
    DMP_RCB       *rcb,
    DMP_PINFO     *oldrootPinfo,
    DB_ERROR      *dberr);


/*{
** Name: dm1bmmerge - Routine to merge entire BTREE.
**
** Description:
**      This routines examines the BTREE pages to determine if pages
**      can be reclaimed.  If so it merges the pages as it descends the
**      tree making several passes if necessary.
**
**	The mission here is to obtain a tree that satisfies the conditions:
**
**	1. No siblings are mergeable: the total page usage for any
**	page and its right sibling exceeds the fill factor. (note that this
**	condition is phrased as 'page usage' to indicate that a different
**	metric is used for fixed format index pages than for variable format
**	index pages).
**
**	2. Either the root is the only index page, or the root has
**	at least two pointers.
**
**	The obvious thing to do is to clean up the subtree of each child
**	recursively, then to join all joinable children of a node.
**	But after the final join, the grandchildren of the node may
**	become joinable. So whenever two siblings are joined, we have
**	to move down the right boundary of the left sibling's subtree, 
**	and "stitch" the two subtrees as far as posible.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      index_fill                      Value indicating percentage full to
**					make index pages during merge.
**      leaf_fill                       Value indicating percentage full to
**					make leaf pages during merge.
**
** Outputs:
**      err_code                        Value indicating the type of error 
**                                      that occurred.  It will be one of the
**                                      following if E_DB_ERROR is returned 
**                                      as status:
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
**	    more free pages
**
** History:
**      06-mar-86 (jennifer)
**          Converted for Jupiter.
**	19-aug-1991 (bryanp)
**	    Support for Btree index compression: pass index_fill and leaf_fill
**	    down to 'descend' so that the percentage values can be available
**	    when we are calculating page usage.
**	30-feb-1993 (rmuth)
**	    In 6.5 when we first issue this command on pre 6.5 table
**	    we scan the table looking for disassociated data pages to
**	    add to the FHDR/FMAP. As this code is only used this one
**	    time remove the code for simplicity, this means the user
**	    will have to modify table to reclaim these pages.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed obsolete rcb_bt_bi_logging and rcb_merge_logging fields.
*/
DB_STATUS
dm1bmmerge(
    DMP_RCB       *rcb,
    i4	      index_fill,
    i4       leaf_fill,
    DB_ERROR      *dberr)
{
    DMP_RCB	*r = rcb;
    DMPP_PAGE	*parent = 0;
    DMP_PINFO	parentPinfo;
    DB_STATUS   status, s;
    i4	local_err_code;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    do
    {
	/*	
	** Fix the root page. 
	*/
	status = dm0p_fix_page( r, (DM_PAGENO)DM1B_ROOT_PAGE, DM0P_WRITE,
			       &parentPinfo, dberr);
	if ( status != E_DB_OK )
	    break;
	
	status = descend(r, &parentPinfo, index_fill, leaf_fill, dberr);
	if ( status != E_DB_OK )
	    break;

	status = dm0p_unfix_page( r, DM0P_UNFIX, &parentPinfo, dberr);

    } while (FALSE);

    if (status == E_DB_OK)
	return (status);

    if (parentPinfo.page)
    {
	s = dm0p_unfix_page(r, DM0P_UNFIX, &parentPinfo, &local_dberr);
	if (s)
	    uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
			&local_err_code, 0);
    }

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9258_DM1B_MERGE);
    }
    return (E_DB_ERROR);
}

/*{
** Name: descend - Routine to descend BTREE looking for pages to merge.
**
** Description:
**      This is the recursive routine which starts at a page and joins all
**      siblings below it( i.e.clean up the subtree of each child).  Then
**      joins all joinable children of a node.
**
** Inputs:
**      rcb                             Pointer to RCB of this table.
**      parent				Pointer to parent page.
**      index_fill                      Value indicating percentage of index
**                                      page to fill during merge.
**      leaf_fill                       Value indicating percentage of leaf
**                                      page to fill during merge.
**
** Outputs:
**      err_code                        Value indicating the type of error 
**                                      that occurred.  It will be one of the
**                                      following if E_DB_ERROR is returned 
**                                      as status:
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
**	    none
**
** History:
**      06-mar-86 (jennifer)
**          Converted for Jupiter.
**	19-aug-1991 (bryanp)
**	    Call dm1cx routines to support Btree index compression
**	06-may-1996 (nanpr01)
**	    Changed all the page header access as macro for New Page Format
**	    project.
*/
static	DB_STATUS
descend(
    DMP_RCB       *rcb,
    DMP_PINFO	 *parentPinfo,
    i4	  index_fill,
    i4       leaf_fill,
    DB_ERROR      *dberr)
{
    DMP_RCB		*r = rcb;
    DMP_TCB		*t = rcb->rcb_tcb_ptr;
    DMPP_PAGE		*next_parent, *current; 
    DMPP_PAGE		*parpage, *curpage; 
    DMP_PINFO		next_parentPinfo;
    DM_TID		pagetid; 
    i4		pos;
    i4		dummy = 0; 
    DB_STATUS		s,status;
    i4             local_err_code;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);
	
    /* check to see if user interrupt occurred. */

    if (*(r->rcb_uiptr) & RCB_USER_INTR)
    {
	SETDBERR(dberr, 0, E_DM0065_USER_INTR);
        return (E_DB_ERROR);
    }
    /* check to see if force abort occurred. */

    if (*(r->rcb_uiptr) & RCB_FORCE_ABORT)
    {
	SETDBERR(dberr, 0, E_DM010C_TRAN_ABORTED);
        return (E_DB_ERROR);
    }

    parpage = parentPinfo->page;

    if (DMZ_AM_MACRO(6))
    {
	s = TRdisplay("descend: Entering merge descend\n");
        dmd_prindex(r, parpage, (i4)0);
    }
    
    /*
    ** If we are at the leaf level, they only work to be performed is a
    ** merge of the overflow chain, if it exists, into a smaller number
    ** of pages.
    */

    if (DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, parpage) & DMPP_LEAF)
    {
	status = join_ovfl(r, parentPinfo, leaf_fill, dberr);
	if (status != E_DB_OK)
	    return (status);	/* ??? What if this is E_DB_INFO ??? */
	return (E_DB_OK);
    }
    
    /*
    ** If this is the root page and it only contains a single child than
    ** reduce the height of the tree by removing the root page until the
    ** root page has more then one child or the LEAF level is encountered.
    */

    while ((DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, parpage) == 0) && 
	   (DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, parpage) == 1))
    {        
	status = ridroot(r, parentPinfo, dberr);
	if (status != E_DB_OK)
	{
	    if (status == E_DB_INFO)
		break;
	    return (E_DB_ERROR);
	}    
    }

    /*
    **	For any non-leaf level index page, recursively attempt to
    **  merge the descendant of this parent.
    */

    for (pos = 0; pos < DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, parpage); 
	 pos++)
    {
	/*  Get decendant from page and descend this tree. */

        dm1cxtget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, parpage, pos,
			&pagetid, (i4*)NULL); 
	status = dm0p_fix_page(r, (DM_PAGENO)pagetid.tid_tid.tid_page,
	    DM0P_WRITE, &next_parentPinfo, dberr);
	if (status != E_DB_OK)
	    return (status);
        status = descend(r, &next_parentPinfo,
				index_fill, leaf_fill, dberr); 
	s = dm0p_unfix_page(r, DM0P_UNFIX, &next_parentPinfo, &local_dberr);

	if (status != E_DB_OK || s != E_DB_OK)
	{
	    /*
	    ** If both failed, log the unfix failure and pass the descend
	    ** error code back up. If just one of them failed, then return
	    ** its error code back up in 'err_code'
	    */
	    if (status == E_DB_OK)
		*dberr = local_dberr;
	    else if (s != E_DB_OK)
		uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
			    &local_err_code, 0);
	    return (E_DB_ERROR);
	}
    }

    /*
    **  Try to merge the children of this parent page.
    **  Parent is already fixed from above.
    */

    for (pos = 0; pos < DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, parpage); )
    {
        status = stitch(r, parentPinfo, pos,
				index_fill, leaf_fill, dberr);
	if (status == E_DB_INFO)
	    pos++;
	else if (status != E_DB_OK)
	    return (E_DB_ERROR);
    }
                    
    /* Eliminate empty roots. */

    while ((DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, parpage) == 0) && 
	   (DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, parpage) == 1))
    {        
	status = ridroot(r, parentPinfo, dberr);
	if (status != E_DB_OK)
	    break;
    }    

    if (status == E_DB_OK || status == E_DB_INFO)
	return (E_DB_OK);
    return (E_DB_ERROR);
}

/*{
** Name: stitch - Routine to descend BTREE stitching two subtrees.
**
** Description:
**	This is the routine which stitches two adjacent subtrees top-down.
**      It moves down the boundary of two subtrees and joins pages as 
**      far as possible.
**
** Inputs:
**      tcb                             Pointer to TCB of this table.
**	pos				Location in parent page of boundary
**					to stitch
**      parentptr                       Pointer to parent page.
**      index_fill                      Value indicating percentage of index
**                                      page to fill during merge.
**      leaf_fill                       Value indicating percentage of leaf
**                                      page to fill during merge.
**
** Outputs:
**      err_code                        Value indicating the type of error 
**                                      that occurred.  It will be one of the
**                                      following if E_DB_ERROR is returned 
**                                      as status:
**                                      
**	Returns:
**
**	    E_DB_OK
**	    E_DB_INFO			No Join performed.
**          E_DB_ERROR
**          E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      06-mar-86 (jennifer)
**          Converted for Jupiter.
**	19-aug-1991 (bryanp)
**	    Call dm1cx routines to support Btree index compression
**	06-may-1996 (nanpr01)
**	    Changed all the page header access as macro for New Page Format
**	    project.
*/
static	DB_STATUS
stitch(
    DMP_RCB       *rcb,
    DMP_PINFO     *parPinfo,
    i4       pos,
    i4	  index_fill,
    i4       leaf_fill,
    DB_ERROR      *dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = rcb->rcb_tcb_ptr;
    DMP_PINFO	    parentPinfo, currentPinfo;
    DM_TID          pagetid; 
    i4         savepos; 
    DB_STATUS       s,status; 
    i4         local_err_code;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    currentPinfo.page = NULL;

    
    parentPinfo = *parPinfo;

    if (DMZ_AM_MACRO(6))
    {
	s = TRdisplay("stitch:Enter stitching, child is %d, parent is\n",
                         pos);
        dmd_prindex(r, parentPinfo.page, (i4)0);
    }

    status = E_DB_OK;
    for (;;)
    {
        dm1cxtget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, parentPinfo.page,
			pos, &pagetid, (i4*)NULL); 
	s = dm0p_fix_page(r, pagetid.tid_tid.tid_page, DM0P_WRITE,
	    &currentPinfo, dberr);
	if (s != E_DB_OK)
	    return (s);

        savepos = DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, currentPinfo.page) - 1; 
	status = join(r, &currentPinfo, pos, &parentPinfo, index_fill, leaf_fill,
	    dberr);

	if (DMZ_AM_MACRO(6))
	{
	    s = TRdisplay("stitch: attempted to join, success = %d\n", 
			    status == E_DB_INFO);
	    dmd_prindex(r, currentPinfo.page, (i4)0);
	}

	if (status != E_DB_OK && status != E_DB_INFO)
	    break;
	if (DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, currentPinfo.page) & DMPP_LEAF)
	    break;
	if (parentPinfo.page != parPinfo->page)
	{
	    status = dm0p_unfix_page(r, DM0P_UNFIX, &parentPinfo, dberr);
	    if (status != E_DB_OK)
		break;
	}
	parentPinfo = currentPinfo;
	currentPinfo.page = NULL;
        pos = savepos; 
    }

    if (currentPinfo.page)
    {
	s = dm0p_unfix_page(r, DM0P_UNFIX, &currentPinfo, dberr);
	if (s != E_DB_OK && status != E_DB_OK)
	    status = s;
    }
    if (parentPinfo.page != parPinfo->page && parentPinfo.page )
    {
	s = dm0p_unfix_page(r, DM0P_UNFIX, &parentPinfo, dberr);
	if (s != E_DB_OK && status != E_DB_OK)
	    status = s;
    }
    return (status);
}

/*{
** Name: join - Routine to join two adjacent pages of a BTREE.
**
** Description:
**	This is the routine which joins two adjacent pages of a BTREE.
**
** Inputs:
**      rcb                             Pointer to RCB of this table.
**      current                         Pointer to current page.
**      curpos                          Value indicating current position of
**                                      child in parent page.
**      parent                          Pointer to parent of current page.
**      index_fill                      Value indicating percentage of index
**                                      page to fill during merge.
**      leaf_fill                       Value indicating percentage of leaf
**                                      page to fill during merge.
**
** Outputs:
**      err_code                        Value indicating the type of error 
**                                      that occurred.  It will be one of the
**                                      following if E_DB_ERROR is returned 
**                                      as status:
**                                      
**	Returns:
**
**	    E_DB_OK
**	    E_DB_INFO			No Join was performed.
**          E_DB_ERROR
**          E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      06-mar-86 (jennifer)
**          Converted for Jupiter.
**	19-aug-1991 (bryanp)
**	    Call dm1cx routines to support Btree index compression
**	16-jul-1992 (kwatts)
**	    MPF change. Check for empty page using accessor.
**	16-oct-1992 (jnash)
**	    Reduced logging project.  Substitute dmpp_tuplecount() == 0
**          for dmpp_isempty().
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project:  Rewritten with new logging protocols.
**	      - Removed Before Image generation.
**	      - Added logging of the data page disassociation.
**	05-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-jul-1993 (rogerk)
**	    Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() call.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass tcb_rel.relpgsize as the page_size argument to dmpp_tuplecount.
**	06-may-1996 (nanpr01)
**	    Changed all the page header access as macro for New Page Format
**	    project.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      15-jan-1998 (stial01)
**          Clean deleted tuples during JOIN.
**	07-May-2006 (jenjo02)
**	    CLUSTERED leaf pages don't have data pages.
*/
static DB_STATUS
join(
    DMP_RCB       *rcb,
    DMP_PINFO	  *currentPinfo,
    i4       curpos,
    DMP_PINFO	  *parentPinfo,
    i4	  index_fill,
    i4       leaf_fill,
    DB_ERROR      *dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_DCB	    *d = t->tcb_dcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DMPP_PAGE      *parpage, *sibpage, *curpage; 
    DMPP_PAGE	    *data = NULL;
    DMP_PINFO	    dataPinfo;
    DMPP_PAGE	    *sibling = NULL; 
    DMP_PINFO	    siblingPinfo;
    LG_LSN	    lsn;
    DM_TID	    sibtid; 
    i4	    sibtype; 
    i4         fill;
    i4         loc_id;
    i4         loc_config_id;
    i4         dm0l_flag;
    i4         local_error;
    DB_STATUS	    status = E_DB_OK;    
    DB_STATUS	    s;
    STATUS	    cl_status;
    CL_ERR_DESC	    sys_err;
    bool	    can_join;
    bool	    page_mutexed = FALSE;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    parpage = parentPinfo->page;
    curpage = currentPinfo->page;

    dataPinfo.page = NULL;
    siblingPinfo.page = NULL;

    if ( (DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, curpage) && DMPP_LEAF) 
	&& (tbio->tbio_page_type != TCB_PG_V1))
    {
	/* 
	** Mutex the page.
	*/
	dm0pMutex(tbio, (i4)0, r->rcb_lk_id, currentPinfo);

	status = dm1cxclean(tbio->tbio_page_type, tbio->tbio_page_size, curpage,
		    DM1B_INDEX_COMPRESSION(r), DM1C_DIRECT, r);
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, currentPinfo);
	if (status != E_DB_OK)
	{
	    uleFormat(NULL, E_DM93C7_BXCLEAN_ERROR, NULL, ULE_LOG, NULL, 
		0, 0, 0, err_code, 0);
	    SETDBERR(dberr, 0, E_DM93BB_BXJOIN_ERROR);
	    return (E_DB_ERROR);
	}
    }

    /* Can't join if on last entry. */

    if ((curpos == DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type, parpage) - 1) ||
	(DM1B_VPT_GET_PAGE_OVFL_MACRO(tbio->tbio_page_type, curpage) != 0))
	return (E_DB_INFO); 

    for (;;)
    {
	/* Can join, get sibling. */

	dm1cxtget(tbio->tbio_page_type, tbio->tbio_page_size, parpage, 
		curpos+1, &sibtid, (i4*)NULL); 

	/*	Fix sibling page to join to parent. */

	status = dm0p_fix_page(r, sibtid.tid_tid.tid_page, DM0P_WRITE, 
				&siblingPinfo, dberr);
	if (status != E_DB_OK)
	    break;

	/*	From page type set fill factor. */

	sibpage = siblingPinfo.page;
	sibtype = DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, sibpage); 
	fill = index_fill;
	if (sibtype & DMPP_LEAF)
	{
	    fill = leaf_fill;
	    if (tbio->tbio_page_type != TCB_PG_V1)
	    {
		/* 
		** Mutex the page.
		*/
		dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &siblingPinfo);

		status = dm1cxclean(tbio->tbio_page_type, tbio->tbio_page_size, 
			sibpage, DM1B_INDEX_COMPRESSION(r), DM1C_DIRECT, r);
		dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &siblingPinfo);
		if (status != E_DB_OK)
		{
		    uleFormat(NULL, E_DM93C7_BXCLEAN_ERROR, NULL, ULE_LOG, NULL, 
			0, 0, 0, err_code, 0);
		    SETDBERR(dberr, 0, E_DM93BB_BXJOIN_ERROR);
		    break;
		}
	    }
	}

	/*
	** Calculate whether the combined usage of the two pages would exceed
	** the indicated fill factor. If it would, the join cannot be performed.
	** Also, a sibling join cannot be performed if the sibling page has an
	** overflow page attached to it (we must remove the overflow first).
	*/

	can_join = dm1cxjoinable( r, DM1B_INDEX_COMPRESSION(r), curpage, 
				sibpage, tbio->tbio_page_size, fill );

	/*
	** A join is possible if the sum of the keys on both pages is
	** if less than the fill amount for the page, and if the page is
	** a LEAF page that no overflow pages exist.
	*/
	if ((sibtype & DMPP_LEAF) && 
	    (DM1B_VPT_GET_PAGE_OVFL_MACRO(tbio->tbio_page_type, sibpage) != 0) ||
	    can_join == FALSE)
	{ 
	    status = E_DB_INFO;
	    break;
	}

	/*	Join sibpage into parpage. */

	status = dm1bxjoin(r, parentPinfo, curpos, currentPinfo, &siblingPinfo, 
				DM1B_SIBJOIN, dberr); 
	if (status)
	    break;

	/*
	** For non-index table leaf index pages, either disassociate the data
	** page or free the data page depending if there are rows on the page.
	**
	** Like indexes, CLUSTERED primary tables don't have data pages 
	*/
	if ((sibtype & DMPP_LEAF) &&
	    (t->tcb_rel.relstat & (TCB_INDEX | TCB_CLUSTERED)) == 0)
	{
	    /*  Get the associated data page. */

	    status = dm0p_fix_page(r, 
			DM1B_VPT_GET_BT_DATA_MACRO(tbio->tbio_page_type, sibpage), 
			DM0P_WRITE, &dataPinfo, dberr); 
	    if (status != E_DB_OK)
		break;

	    data = dataPinfo.page;

	    /*
	    ** Disassociate the leaf's data page from the btree index.
	    ** If the leaf is empty, it will then be deallocated.
	    */

	    /* Get location information for log record. */
	    loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, 
			DMPP_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, data));
	    loc_config_id = tbio->tbio_location_array[loc_id].loc_config_id;

	    /*
	    ** Online Backup Protocols: Check if BI must be logged first.
	    */
	    if (d->dcb_status & DCB_S_BACKUP)
	    {
		status = dm0p_before_image_check(r, data, dberr);
		if (status != E_DB_OK)
		    break;
	    }

	    /*
	    ** Log the disassociate action and write the log record's
	    ** LSN to the updated page.
	    */
	    if (r->rcb_logging & RCB_LOGGING)
	    {
		/* Reserve space for log and CLR  */
		cl_status = LGreserve(0, r->rcb_log_id, 2, 
		    sizeof(DM0L_DISASSOC) * 2, &sys_err);
		if (cl_status != OK)
		{
		    (VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			    (char *)0, 0L, (i4 *)0, err_code, 0);
		    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			    ULE_LOG, NULL, (char *)0, 0L, (i4 *)0,
			    err_code, 1, 0, r->rcb_log_id);
		    SETDBERR(dberr, 0, E_DM93BB_BXJOIN_ERROR);
		    status = E_DB_ERROR;
		    break;
		}

		/*
		** Mutex the page before logging and updating it.
		*/
		dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &dataPinfo);
		page_mutexed = TRUE;

		dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);

		if ( r->rcb_state & RCB_IN_MINI_TRAN )
		    dm0l_flag |= DM0L_MINI;

		if (t->tcb_sysrel)
		    dm0l_flag |= DM0L_PHYS_LOCK;

		status = dm0l_disassoc(r->rcb_log_id, dm0l_flag,
		    &t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		    t->tcb_rel.relloccount, loc_config_id, 
		    DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, data),
		    (LG_LSN *)0, &lsn, dberr);
		if (status != E_DB_OK)
		    break;

		DMPP_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, data, lsn);
	    }
	    else
		dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &dataPinfo);

	    DMPP_VPT_CLR_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, data, DMPP_ASSOC);
	    DMPP_VPT_SET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, data, DMPP_MODIFY);

	    /*
	    ** Unmutex the page as the action is complete.
	    */
	    dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &dataPinfo);
	    page_mutexed = FALSE;

	    /*
	    ** If the page is empty then deallocate it.  Otherwise just
	    ** unfix it (note that deallocating it causes it to be unfixed.
	    */
	    if ((*t->tcb_acc_plv->dmpp_tuplecount)(data,t->tcb_rel.relpgsize)
				== 0)
	    {
		status = dm1p_putfree(r, &dataPinfo, dberr);
		if (status != E_DB_OK)
		    break;
	    }
	    else
	    {
		status = dm0p_unfix_page(r, DM0P_UNFIX, &dataPinfo, dberr);
		if (status != E_DB_OK)
		    break;
	    }
	}

	/*	Return sibling to the free list. */

	status = dm1p_putfree(r, &siblingPinfo, dberr); 
	if (status != E_DB_OK)
	    break;

	return (E_DB_OK);
    }

    /*
    ** Error handling : Leave with only the original current page fixed.
    **
    ** Note that expected condition of E_DB_INFO returns through here.
    */
    if (page_mutexed)
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &dataPinfo);

    if (siblingPinfo.page)
    {
	s = dm0p_unfix_page(r, DM0P_UNFIX, &siblingPinfo, &local_dberr);
	if (s)
	    uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, 
	    		NULL, 0, NULL, &local_error, 0);
    }

    if (dataPinfo.page)
    {
	s = dm0p_unfix_page(r, DM0P_UNFIX, &dataPinfo, &local_dberr);
	if (s)
	    uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, 
	    		NULL, 0, NULL, &local_error, 0);
    }

    return (status); 
}

/*{
** Name: join_ovfl - Routine to join overflow leaf pages of a BTREE.
**
** Description:
**	This is the routine which joins a LEAF level index page and it's
**	overflow chain pages.
**
** Inputs:
**      rcb                             Pointer to RCB of this table.
**      parent                          Pointer to LEAF index page.
**	leaf_fill			Leaf page fill factor
**
** Outputs:
**      err_code                        Value indicating the type of error 
**                                      that occurred.  It will be one of the
**                                      following if E_DB_ERROR is returned 
**                                      as status:
**                                      
**	Returns:
**
**	    E_DB_OK
**	    E_DB_INFO			No Join was performed.
**          E_DB_ERROR
**          E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      06-mar-86 (jennifer)
**          Converted for Jupiter.
**	19-aug-1991 (bryanp)
**	    Call dm1cx routines to support Btree index compression
**	15-oct-1991 (mikem)
**	    Eliminate unix compiler warnings ("illegal structure pointer 
**	    combination") by casting parent to a "(DMPP_PAGE *).
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project:  Rewritten with new logging protocols.
**	      - Removed Before Image generation.
**	      - Added logging of the data page disassociation.
**	05-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-jul-1993 (rogerk)
**	    Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() call.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass tcb_rel.relpgsize as the page_size argument to dmpp_tuplecount.
**	06-may-1996 (nanpr01)
**	    Changed all the page header access as macro for New Page Format
**	    project.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**	07-May-2006 (jenjo02)
**	    CLUSTERED leaf pages don't have data pages.
*/
static DB_STATUS
join_ovfl(
    DMP_RCB       *rcb,
    DMP_PINFO     *parentPinfo,
    i4	  leaf_fill,
    DB_ERROR      *dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_DCB	    *d = t->tcb_dcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DMPP_PAGE	    *data;
    i4         fill;
    DB_STATUS       s,status;    
    STATUS	    cl_status;
    CL_ERR_DESC 	    sys_err;
    i4         dm0l_flag;
    i4         loc_id;
    i4         loc_config_id;
    i4         local_err_code;
    LG_LSN	    lsn;
    i4		    can_join;
    bool	    page_mutexed = FALSE;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;
    DMP_PINFO	    siblingPinfo, dataPinfo, currentPinfo;
    DMPP_PAGE      *sibpage, *current, *sibling, *parent;


    CLRDBERR(dberr);

    /*	If no overflow pages, then no work to perform. */

    if (DMPP_VPT_GET_PAGE_OVFL_MACRO(tbio->tbio_page_type, parentPinfo->page) == 0)
	return(E_DB_OK);

    currentPinfo = *parentPinfo;
    current = parent = parentPinfo->page;

    siblingPinfo.page = NULL;
    dataPinfo.page = NULL;

    /*	Continue while overflow pages are still available for this leaf. */

    while (DM1B_VPT_GET_PAGE_OVFL_MACRO(tbio->tbio_page_type, current))
    {
	/*	Fix sibling page to join to current page. */

	status = dm0p_fix_page(r, 
		DM1B_VPT_GET_PAGE_OVFL_MACRO(tbio->tbio_page_type, current), 
		DM0P_WRITE, &siblingPinfo, dberr);
	if (status != E_DB_OK)
	    break;

	sibling = siblingPinfo.page;

	/*
	** Calculate whether the combined usage of the two pages would exceed
	** the indicated fill factor. If it would, the join cannot be performed.
	** Also, a sibling join cannot be performed if the sibling page has an
	** overflow page attached to it (we must remove the overflow first).
	*/

	can_join = dm1cxjoinable( r, DM1B_INDEX_COMPRESSION(r), current,
				sibling, tbio->tbio_page_size, leaf_fill );

	/*  Determine if a join is possible. */

	if ((DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type, current) + 
	    DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type, sibling)) > 
	    t->tcb_kperleaf)
	{ 
	    /*  Join is not possible. Move to next page in chain. */

	    if (current != parent)
	    {
		/*  Only unfix current if it's not the parent page. */

		status = dm0p_unfix_page(r, DM0P_UNFIX, &currentPinfo, &local_dberr);
		if (status != E_DB_OK)
		    break;
	    }	

	    /*	Move current up to sibling. */

	    currentPinfo = siblingPinfo;
	    current = sibling; 
	    sibling = siblingPinfo.page = (DMPP_PAGE *)NULL;
	    continue;
	}

	/*  Join sibling into current. */

	status = dm1bxjoin(r, (DMP_PINFO*)NULL, 0, &currentPinfo, &siblingPinfo,
			    DM1B_OVFJOIN, dberr); 
	if (status)
	    break;

	/*  Unassociate data page associated with non-index table LEAF page. */

	if ((DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, current) & 
	     (DMPP_LEAF | DMPP_CHAIN)) && 
	    !(t->tcb_rel.relstat & (TCB_INDEX | TCB_CLUSTERED)) &&
	    DM1B_VPT_GET_BT_DATA_MACRO(tbio->tbio_page_type, sibling))
	{
	    /*  Fix the data page. */

	    status = dm0p_fix_page(r, 
			DM1B_VPT_GET_BT_DATA_MACRO(tbio->tbio_page_type, sibling), 
			DM0P_WRITE, &dataPinfo, dberr);
	    if (status != E_DB_OK)
		break;

	    data = dataPinfo.page;

	    /* Get location information for log record. */
	    loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, 
			DMPP_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, data));
	    loc_config_id = tbio->tbio_location_array[loc_id].loc_config_id;

	    /*
	    ** Online Backup Protocols: Check if BI must be logged first.
	    */
	    if (d->dcb_status & DCB_S_BACKUP)
	    {
	        status = dm0p_before_image_check(r, data, dberr);
	        if (status != E_DB_OK)
		    break;
	    }

	    /*
	    ** Log the disassociate action and write the log record's
	    ** LSN to the updated page.
	    */
	    if (r->rcb_logging & RCB_LOGGING)
	    {
		/* Reserve space for log and CLR  */
		cl_status = LGreserve(0, r->rcb_log_id, 2, 
		    sizeof(DM0L_DISASSOC) * 2, &sys_err);
		if (cl_status != OK)
		{
		    (VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			    (char *)0, 0L, (i4 *)0, err_code, 0);
		    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			    ULE_LOG, NULL, (char *)0, 0L, (i4 *)0,
			    err_code, 1, 0, r->rcb_log_id);
		    SETDBERR(dberr, 0, E_DM93BB_BXJOIN_ERROR);
		    status = E_DB_ERROR;
		    break;
		}

		/*
		** Mutex the page before logging and updating it.
		*/
		dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &dataPinfo);
		page_mutexed = TRUE;

		dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);

		if ( r->rcb_state & RCB_IN_MINI_TRAN )
		    dm0l_flag |= DM0L_MINI;

		if (t->tcb_sysrel)
		    dm0l_flag |= DM0L_PHYS_LOCK;

		status = dm0l_disassoc(r->rcb_log_id, dm0l_flag,
		    &t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		    t->tcb_rel.relloccount, loc_config_id, 
		    DMPP_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, data),
		    (LG_LSN *)0, &lsn, dberr);
		if (status != E_DB_OK)
		    break;

		DMPP_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, data, lsn);
	    }
	    else
		dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &dataPinfo);

	    DMPP_VPT_CLR_PAGE_STAT_MACRO(tbio->tbio_page_type, data, DMPP_ASSOC);
	    DMPP_VPT_SET_PAGE_STAT_MACRO(tbio->tbio_page_type, data, DMPP_MODIFY);

	    /*
	    ** Unmutex the page as the action is complete.
	    */
	    dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &dataPinfo);
	    page_mutexed = FALSE;

	    /*
	    ** If the page is empty then deallocate it.  Otherwise just
	    ** unfix it (note that deallocating it causes it to be unfixed.
	    */
	    if ((*t->tcb_acc_plv->dmpp_tuplecount)(data, t->tcb_rel.relpgsize)
				== 0)
	    {
		status = dm1p_putfree(r, &dataPinfo, dberr);
		if (status != E_DB_OK)
		    break;
	    }
	    else
	    {
		status = dm0p_unfix_page(r, DM0P_UNFIX, &dataPinfo, dberr);
		if (status != E_DB_OK)
		    break;
	    }
	}

	/*	Return sibling to the free list. */

	status = dm1p_putfree(r, &siblingPinfo, dberr); 
	if (status != E_DB_OK)
	    break;
    }

    /*	Perpare to return if everything was okay. */

    if (status == E_DB_OK)
    {
	if (current != parent)
	{
	    status = dm0p_unfix_page(r, DM0P_UNFIX, &currentPinfo, dberr);
	}
	if (status == E_DB_OK)
	    return (status);
    }

    /*	Clean up after any errors. */

    if (page_mutexed)
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &dataPinfo);

    if (dataPinfo.page)
	status = dm0p_unfix_page(r, DM0P_UNFIX, &dataPinfo, &local_dberr);
    if (siblingPinfo.page)
	status = dm0p_unfix_page(r, DM0P_UNFIX, &siblingPinfo, &local_dberr);
    return (E_DB_ERROR); 
}

/*{
** Name: ridroot - Routine to merge a root node in a BTREE.
**
** Description:
**      This routine takes the current root which is empty (i.e. contains 
**      a single pointer) and merges it with its child. The data will be
**      copies into the old root.  This routine expects the oldroot to be fixed 
**      on entry, and will leave it fixed on exit.
**
** Inputs:
**      rcb                             Pointer to RCB of this table.
**      oldroot                         Pointer to old root page.
**
** Outputs:
**      err_code                        Value indicating the type of error 
**                                      that occurred.  It will be one of the
**                                      following if E_DB_ERROR is returned 
**                                      as status:
**                                      
**	Returns:
**
**	    E_DB_OK
**	    E_DB_INFO			No join performed.
**          E_DB_ERROR
**          E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      06-mar-86 (jennifer)
**          Converted for Jupiter.
**	18-apr-1990 (Derek)
**	    When reducing a 3 or more high tree to a 2 high tree the SPRIG
**	    status from the newpage (soon to be root) was being ignored.
**	    Removed code that saved the oldroot status, and copied only 
**	    the SPRIG status into the newroot.
**	19-aug-1991 (bryanp)
**	    Call dm1cx routines to support Btree index compression
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project:  Rewritten with new logging protocols.
**	05-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	24-may-1993 (rogerk)
**	    Changed operation type on dm0l_bi calls to be JOIN instead of
**	    NEWROOT.  This caused records to be reported incorrectly in
**	    logdump.
**	26-jul-1993 (rogerk)
**	    Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() call.
**	06-mar-1996 (stial01 for bryanp)
**	    Use dm0m_lcopy to copy pages around, since they may be larger than
**		can be copied by MEcopy.
**	06-may-1996 (nanpr01)
**	    Changed all the page header access as macro for New Page Format
**	    project.
**      01-may-1996 (stial01)
**          LGreserve adjustments for removal of bi_page from DM0L_BI
**          LGreserve adjustments for removal of ai_page from DM0L_AI
**	10-jan-1997 (nanpr01)
**	    Added back the space holders to shield it from misalignment.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
*/
static DB_STATUS
ridroot(
    DMP_RCB       *rcb,
    DMP_PINFO	  *oldrootPinfo,
    DB_ERROR      *dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_DCB	    *d = t->tcb_dcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DM_TID	    kid; 
    LG_LSN	    lsn;
    DMPP_PAGE	    *header, *newroot; 
    DMPP_PAGE      *oldpage; 
    DMPP_PAGE      *newpage; 
    DMP_PINFO	    newrootPinfo;
    DB_STATUS	    status = E_DB_OK;
    DB_STATUS	    s;
    STATUS	    cl_status;
    CL_ERR_DESC	    sys_err;
    i4         dm0l_flag;
    i4         loc_id;
    i4         old_loc_cnf_id;
    i4         new_loc_cnf_id;
    i4         local_error;
    bool	    pages_mutexed = FALSE;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    oldpage = oldrootPinfo->page;

    if ((DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, oldpage) != 
		DM1B_ROOT_PAGE) || 
	(DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type, oldpage) != 1))
    {
	/* FIX_ME: log an error message here. */
	/* *err_code = */
	return (E_DB_ERROR);
    }


    for (;;)
    {
	/*
	** Fix join page - its the one pointed to by the only bid in the root.
	*/
	dm1cxtget(tbio->tbio_page_type, tbio->tbio_page_size, oldpage, 0,
			&kid, (i4*)NULL); 
	status = dm0p_fix_page(r, kid.tid_tid.tid_page, DM0P_WRITE, 
				&newrootPinfo, dberr);
	if (status != E_DB_OK)
	    break;

	newpage = newrootPinfo.page;

	/*
	** If the last page is a LEAF, then we cannot get rid of the root.
	** The smallest a btree can shrink to is a root and one leaf page.
	*/
	if (DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, newpage) & DMPP_LEAF)
	{
	    status = E_DB_INFO;
	    break;
	}

	/*
	** Get information on the locations to which the update is being made.
	*/
	loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count,
	    (i4) DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, oldpage));
	old_loc_cnf_id = tbio->tbio_location_array[loc_id].loc_config_id;
	loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count,
	    (i4) DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, newpage));
	new_loc_cnf_id = tbio->tbio_location_array[loc_id].loc_config_id;


	/*
	** Online Backup Protocols: Check if BI must be logged before update.
	*/
	if (d->dcb_status & DCB_S_BACKUP)
	{
	    status = dm0p_before_image_check(r, oldpage, dberr);
	    if (status != E_DB_OK)
		break;
	    status = dm0p_before_image_check(r, newpage, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/************************************************************
	**            Log the Join operation.
	*************************************************************
	**
	** A JOIN operation is logged in two phases:  Before Image
	** Log Records are written prior to updating the page.  These
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
	    /* Reserve log file space for 2 BIs and their CLRs */
	    cl_status = LGreserve(0, r->rcb_log_id, 4,
		4*(sizeof(DM0L_BI) - sizeof(DMPP_PAGE) + t->tcb_rel.relpgsize),
		&sys_err);
	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		  	(char *)0, 0L, (i4 *)0, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			ULE_LOG, NULL, (char *)0, 0L, (i4 *)0,
			err_code, 1, 0, r->rcb_log_id);
		SETDBERR(dberr, 0, E_DM9258_DM1B_MERGE);
		status = E_DB_ERROR;
		break;
	    }

	    /*
	    ** Mutex the pages before beginning the udpates.  This must be done 
	    ** prior to the writing of the log record to satisfy logging 
	    ** protocols.
	    **
	    ** Note that only the current and sibling are updated as part of
	    ** this "operation".  The update to the parent page is done below
	    ** and is logged and recovered separately.
	    */
	    pages_mutexed = TRUE;
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, oldrootPinfo);
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &newrootPinfo);


	    /* Physical locks are used on btree index pages */
	    dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);

	    if ( r->rcb_state & RCB_IN_MINI_TRAN )
	        dm0l_flag |= DM0L_MINI;

	    dm0l_flag |= DM0L_PHYS_LOCK;

	    status = dm0l_bi(r->rcb_log_id, dm0l_flag,
		&t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		t->tcb_rel.relpgtype, t->tcb_rel.relloccount, old_loc_cnf_id,
		DM0L_BI_JOIN, 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, oldpage), 
		t->tcb_rel.relpgsize, oldpage, (LG_LSN *) 0, 
		&lsn, dberr);
	    if (status != E_DB_OK)
		break;

	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, oldpage, lsn);

	    status = dm0l_bi(r->rcb_log_id, dm0l_flag,
		&t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		t->tcb_rel.relpgtype, t->tcb_rel.relloccount, new_loc_cnf_id,
		DM0L_BI_JOIN, 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, newpage), 
		t->tcb_rel.relpgsize, newpage, (LG_LSN *) 0, 
		&lsn, dberr);
	    if (status != E_DB_OK)
		break;

	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, newpage, lsn);
	}
	else
	{
	    pages_mutexed = TRUE;
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, oldrootPinfo);
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &newrootPinfo);
	}

	/*	Copy newroot into oldroot, preserving newroot's SPRIG status. */

	MEcopy((char *)newpage, tbio->tbio_page_size, (char *)oldpage);
	DM1B_VPT_SET_PAGE_STAT_MACRO(tbio->tbio_page_type, oldpage,
		 (DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, oldpage) & 
		  DMPP_SPRIG) | DMPP_INDEX);
	DM1B_VPT_SET_PAGE_PAGE_MACRO(tbio->tbio_page_type, oldpage, DM1B_ROOT_PAGE);

	DM1B_VPT_SET_PAGE_STAT_MACRO(tbio->tbio_page_type, oldpage, DMPP_MODIFY);
	DM1B_VPT_SET_PAGE_STAT_MACRO(tbio->tbio_page_type, newpage, DMPP_MODIFY);

	/************************************************************
	**            Log the Join operation.
	*************************************************************
	**
	** Complete the Logging for the operation by logging the After
	** images of the updated pages.  These will be used during forward
	** (redo,rollforward) recovery to reconstruct the pages.
	*/
	if (r->rcb_logging & RCB_LOGGING)
	{
	    dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, oldrootPinfo);
	    dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &newrootPinfo);
	    pages_mutexed = FALSE;

	    /* Reserve log file space for 2 AIs */
	    cl_status = LGreserve(0, r->rcb_log_id, 2,
		2*(sizeof(DM0L_AI) - sizeof(DMPP_PAGE) + t->tcb_rel.relpgsize),
		&sys_err);
	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		  	(char *)0, 0L, (i4 *)0, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			ULE_LOG, NULL, (char *)0, 0L, (i4 *)0,
			err_code, 1, 0, r->rcb_log_id);
		SETDBERR(dberr, 0, E_DM9258_DM1B_MERGE);
		status = E_DB_ERROR;
		break;
	    }

	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, oldrootPinfo);
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &newrootPinfo);
	    pages_mutexed = TRUE;

	    /* Physical locks are used on btree index pages */
	    dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);

	    if ( r->rcb_state & RCB_IN_MINI_TRAN )
	        dm0l_flag |= DM0L_MINI;

	    dm0l_flag |= DM0L_PHYS_LOCK;

	    status = dm0l_ai(r->rcb_log_id, dm0l_flag,
		&t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		t->tcb_rel.relpgtype, t->tcb_rel.relloccount, old_loc_cnf_id,
		DM0L_BI_NEWROOT, 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, oldpage), 
		t->tcb_rel.relpgsize, oldpage, (LG_LSN *) 0, 
		&lsn, dberr);
	    if (status != E_DB_OK)
		break;

	    status = dm0l_ai(r->rcb_log_id, dm0l_flag,
		&t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		t->tcb_rel.relpgtype, t->tcb_rel.relloccount, new_loc_cnf_id,
		DM0L_BI_NEWROOT, 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, newpage), 
		t->tcb_rel.relpgsize, newpage, (LG_LSN *) 0, 
		&lsn, dberr);
	    if (status != E_DB_OK)
		break;

	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, oldpage, lsn);
	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, newpage, lsn);

	    DM1B_VPT_SET_BT_SPLIT_LSN_MACRO(tbio->tbio_page_type, oldpage, lsn);
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
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, oldrootPinfo);
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &newrootPinfo);


	/*
	** Place newroot onto the free list.
	** This will cause newroot to be unfixed.
	*/

	status = dm1p_putfree(r, &newrootPinfo, dberr); 
	if (status != E_DB_OK)
	    break;

	return (E_DB_OK);
    }
	
    /*
    ** Error handling.
    ** 
    ** Note that E_DB_INFO status is returned here if we notice that the
    ** child page is a LEAF.
    */
    if (pages_mutexed)
    {
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, oldrootPinfo);
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &newrootPinfo);
    }

    if (newrootPinfo.page)
    {
	s = dm0p_unfix_page(r, DM0P_UNFIX, &newrootPinfo, &local_dberr);
	if (s != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, 
	    		NULL, 0, NULL, &local_error, 0);
	}
    }

    if (DB_FAILURE_MACRO(status) && (dberr->err_code > E_DM_INTERNAL))
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9258_DM1B_MERGE);
    }

    return(status); 
}
