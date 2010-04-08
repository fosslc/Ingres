/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
#include    <sr.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dmucb.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmse.h>
#include    <dm0m.h>
#include    <dm0p.h>
#include    <dm1b.h>
#include    <dm1m.h>
#include    <dm1c.h>
#include    <dm1cx.h>
#include    <dmtcb.h>
#include    <dm1p.h>
#include    <dm2t.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dm1x.h>
#include    <dmpl.h>
#include    <dmpp.h>
#include    <dm0l.h>
#include    <dmftrace.h>
#include    <dmpbuild.h>
#include    <dmd.h>

/**
**
**  Name: DM1MBUILD.C - Routines to build an Rtree table.
**
**  Description:
**      This file contains all the routines needed to build
**      an Rtree table.
**
**      The routines defined in this file are:
**      dm1mbbegin          - Initializes Rtree file for building.
**      dm1mbend            - Finishes building Rtree table.
**      dm1mbput            - Adds a record to a new Rtree table
**                            and builds index.
**
**  History:
**      12-may-95 (wonst02)
**          Original (cloned from dm1bbuild.c)
**	    === ingres!main!generic!back!dmf!dmp dm1bbuild.c rev 16 ====
**	25-mar-96 (shero03)
**	    Added page size from Variable Page Size project
**	23-apr-96 (wonst02)
**	    Use dmpp_unbr to calculate composite MBR.
**	15-nov-1996 (wonst02) Incorporate: 06-May-1996 (nanpr01)
**	    Changed the page header access as macro. Also changed the kperpage
**	    calculation.
**	15-nov-1996 (wonst02) Incorporate: 16-sep-1996 (nanpr01)
** 	    local variable tid.tid_line needs to be initialized.
**	18-nov-1996 (wonst02)
**	    Incorporate use of DMPP_SIZEOF_TUPLE_HDR_MACRO for new page format.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Removed unecessary DMPP_INIT_TUPLE_INFO_MACRO
**      10-mar-97 (stial01)
**          dm1mbput() Removed unecessary tuple alloc/dealloc
**      12-jun-97 (stial01)
**          Pass tlv to dm1cxget instead of tcb.
**	01-dec-1998 (nanpr01)
**	    Get rid of DMPP_TUPLE_INFO structure. Use DMPP_TUPLE_HDR instead.
**      07-Dec-1998 (stial01)
**          Added kperleaf to distinguish from kperpage (keys per index page)
**      09-feb-99 (stial01)
**          dm1mbbegin() Pass relcmptlvl (DMF_T_VERSION) to dm1b_kperpage
**      12-Apr-1999 (stial01)
**          Pass page_type parameter to dmd_prkey
** 	25-mar-1999 (wonst02)
** 	    Fix RTREE bug B95350 in Rel. 2.5...
** 	    Use mct_page_size in call to dm1m_kperpage(), not tcb page size.
**	20-oct-1999 (nanpr01)
**	    Optimized the code to minimize the tuple header copy.
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
**	16-dec-04 (inkdo01)
**	    Add various collID's.
**	13-Apr-2006 (jenjo02)
**	    Changes to dmd_prkey() prototype for unrelated
**	    CLUSTERED primary Btrees.
**	13-Jun-2006 (jenjo02)
**	    Using DM1B_MAXSTACKKEY instead of DM1B_KEYLENGTH, call dm1b_AllocKeyBuf() 
**	    to validate/allocate Leaf work space. DM1B_KEYLENGTH is now only the 
**	    max length of key attributes, not necessarily the length of the Leaf entry.
**	    (though this doesn't really apply to RTREEs, at least we'll be consistent).
**	24-Feb-2008 (kschendel) SIR 122739
**	    Changes throughout for new row-accessor scheme.
**	11-Apr-2008 (kschendel)
**	    RCB's adfcb now typed as such, minor fixes here.
**	02-Dec-2008 (jonj)
**	    SIR 120874: dm1mx? dm1b_? functions converted to DB_ERROR *
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1x?, dm1?b? functions converted to DB_ERROR *
**      12-oct-2009 (stial01)
**          Use DMPP_PAGE for all page types.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: page accessor prototype changes.
*/

/*
**  Forward function references.
*/

static DB_STATUS    finish_parent(
	DM2U_M_CONTEXT	*mct,
	char		*indexkey,
	DB_ERROR	*dberr);

static DB_STATUS    update_parent(
	DM2U_M_CONTEXT	*mct,
	char		*indexkey,
	DB_ERROR	*dberr);

static DB_STATUS    add_to_next_leaf(
	DM2U_M_CONTEXT	*mct,
	char		*record,
	char		*rec,
	i4		rec_size,
	char		*key,
	char		*indexkey,
	DB_ERROR	*dberr);

static DB_STATUS    put_high_range(
	DM2U_M_CONTEXT	*mct,
    	DMPP_PAGE	*page,
	i4		*infinity,
	char		*endkey);

static DB_STATUS    get_high_range(
	DM2U_M_CONTEXT	*mct,
    	DMPP_PAGE	*page,
	i4		*infinity,
	char		*endkey);

static DB_STATUS    put_low_range(
	DM2U_M_CONTEXT	*mct,
    	DMPP_PAGE	*page,
	i4		*infinity,
	char		*lowkey);

static DB_STATUS    compute_mbr_lhv(
	DM2U_M_CONTEXT	*mct,
    	DMPP_PAGE	*page,
	char		*endkey);

static VOID	    logErrorFcn(
				i4 		status, 
				DB_ERROR 	*dberr,
				PTR		FileName,
				i4		LineNumber);
#define	log_error(status,dberr) \
	logErrorFcn(status,dberr,__FILE__,__LINE__)


/*{
** Name: dm1mbbegin - Build Begin: Initializes Rtree file for modify.
**
** Description:
**    This routine initializes the Rtree for building.
**    The root, header, and first leaf pages are
**    initialized.  Note the code depends on them being
**    initialized in this order.
**
** Inputs:
**      mct                             Pointer to modify context.
**
**
** Outputs:
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
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
**          none.
**
** History:
**	15-nov-1996 (wonst02) Incorporate: 06-May-1996 (nanpr01)
**	    Changed the page header access as macro. Also changed the kperpage
**	    calculation.
**	15-nov-1996 (wonst02) Incorporate: 16-sep-1996 (nanpr01)
** 	    local variable tid.tid_line needs to be initialized.
**      09-feb-99 (stial01)
**          dm1mbbegin() Pass relcmptlvl (DMF_T_VERSION) to dm1b_kperpage
**	08-aug-2001 (thaju02)
**	    In calculating leaf_klen for page size > 2k, do not account
**	    for tuple header here. Tuple header size is taken into account
**	    in dm1cxformat for non-index pages. (B105635)
*/
DB_STATUS
dm1mbbegin(
	DM2U_M_CONTEXT   *mct,
	DB_ERROR         *dberr)
{
    DB_STATUS       status;
    DM_TID	    tid;
    i4	    infinity;
    DMP_TCB         *t = mct->mct_oldrcb->rcb_tcb_ptr;
    DB_STATUS	    local_err;
    DB_ERROR	    local_dberr;

    CLRDBERR(dberr);

    if (DMZ_AM_MACRO(13))
    {
	TRdisplay("dm1mbbegin: Beginning Rtree table load.\n");
    }

    status = dm1c_rowaccess_setup(&mct->mct_index_rac);
    if (status != E_DB_OK)
	return(status);
    mct->mct_index_maxentry = mct->mct_klen + mct->mct_index_rac.worstcase_expansion;

    /*
    **	Allocate build memory and use SAVE buffers.
    */

    status = dm1xstart(mct, DM1X_SAVE, dberr);

    /*
    ** Set up the Rtree build information.
    **
    ** An array that keeps track of the current index page at each level
    ** (mct->mct_ancestors) must be iniitalized to contain the root page.
    */
    mct->mct_relpages = 0;
    mct->mct_dupstart = 0;
    mct->mct_ancestors[0] = 0;
    mct->mct_top = 1;
    mct->mct_main = 1;
    mct->mct_prim = 1;

    /*
    ** Figure out the Rtree fill factor information:
    **    - how many keys fit on an index page
    **    - how full data pages should be.
    ** Note that an Rtree INDEX page contains entries of the form
    **    (BID, MBR, LHV),
    ** whereas a LEAF page contains entries
    **    (TIDP, MBR).
    ** The mct_klen and mct_kperpage values reflect the key length and
    ** keys per page of an INDEX page. The key length and keys per page
    ** of a LEAF page are calculated by the subtracting the length of
    ** the Hilbert value. Note that mct_klen includes TID size.
    */

    mct->mct_kperpage = dm1m_kperpage(mct->mct_page_type, mct->mct_page_size,
	DMF_T_VERSION, 0, mct->mct_klen, DM1B_PINDEX);
    mct->mct_kperleaf = dm1m_kperpage(mct->mct_page_type, mct->mct_page_size,
	DMF_T_VERSION, 0, mct->mct_klen - mct->mct_hilbertsize, DM1B_PLEAF);

    mct->mct_db_fill = mct->mct_page_size -
			    (mct->mct_page_size * mct->mct_d_fill) / 100;

    if (DMZ_AM_MACRO(13))
    {
	TRdisplay(
    "dm1mbbegin: l-fill=%d(%d), i-fill=%d(%d), l-kperpage=%d, i-kperpage=%d\n",
	    mct->mct_l_fill, 
	    (mct->mct_klen - mct->mct_hilbertsize) * mct->mct_l_fill / 100,
	    mct->mct_i_fill, mct->mct_kperpage * mct->mct_i_fill / 100,
	    mct->mct_kperleaf, mct->mct_kperpage);
    }

    /*
    ** Make up empty root page. Place one index record on it to point
    ** to the leaf page. Don't allocate any key space for this one index
    ** record yet; we'll replace it later when the correct key is known.
    */
    if (status == E_DB_OK)
    {
        status = dm1xnewpage(mct, 0, (DMPP_PAGE **)&mct->mct_curindex,
		dberr);
    }
    if (status == E_DB_OK)
	status = dm1xnewpage(mct, 0, (DMPP_PAGE **)&mct->mct_curleaf, dberr);
    if (status != E_DB_OK)
    {
	(VOID) dm1xfinish(mct, DM1X_CLEANUP, &local_dberr);
	log_error(E_DM925A_DM1B_BEGIN, dberr);
	return (status);
    }

    /*
    ** Format INDEX and allocate one index record on INDEX to point to LEAF.
    */
    status = dm1mxformat(mct->mct_page_type, mct->mct_page_size, 
	    (DMP_RCB *) 0, t,
	    mct->mct_curindex,
	    mct->mct_kperpage,
	    mct->mct_klen, 
	    DM1B_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, mct->mct_curindex),
	    DM1B_PSPRIG,
	    mct->mct_index_comp, mct->mct_acc_plv, dberr);
    if (status != E_DB_OK)
    {
	(VOID) dm1xfinish(mct, DM1X_CLEANUP, &local_dberr);
	log_error(E_DM925A_DM1B_BEGIN, dberr);
	return (status);
    }
    DM1B_VPT_SET_BT_NEXTPAGE_MACRO(mct->mct_page_type,
    			       mct->mct_curindex, (DM_PAGENO) 0);

    status = dm1cxallocate(mct->mct_page_type, mct->mct_page_size, 
			mct->mct_curindex,
			(i4)DM1C_DIRECT, mct->mct_index_comp,
			(DB_TRAN_ID *)NULL, (i4)0, 0, (i4)DM1B_TID_S);
    if (status != E_DB_OK)
    {
	dm1cxlog_error(E_DM93E0_BAD_INDEX_ALLOC, (DMP_RCB *)mct->mct_oldrcb,
	    mct->mct_curindex, mct->mct_page_type, mct->mct_page_size, (i4)0);
	(VOID) dm1xfinish(mct, DM1X_CLEANUP, &local_dberr);
	log_error(E_DM925A_DM1B_BEGIN, dberr);
	return (status);
    }
    tid.tid_tid.tid_page = DM1B_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type,
						    mct->mct_curleaf);
    tid.tid_tid.tid_line = 0;

    /*
    **	Put the TIDP portion of the leaf entry onto the index page
    */
    status = dm1cxtput(mct->mct_page_type, mct->mct_page_size, 
		mct->mct_curindex, 0, &tid, (i4)0 );
    if (status != E_DB_OK)
    {
	dm1cxlog_error(E_DM93EB_BAD_INDEX_TPUT, (DMP_RCB *)mct->mct_oldrcb,
	    mct->mct_curindex, mct->mct_page_type, mct->mct_page_size, (i4)0);
	(VOID) dm1xfinish(mct, DM1X_CLEANUP, &local_dberr);
	log_error(E_DM925A_DM1B_BEGIN, dberr);
	return (status);
    }

    /*	Format leaf and place a -infinity as low key value of leaf. */

    status = dm1mxformat(mct->mct_page_type, mct->mct_page_size,
	    (DMP_RCB *) 0, t, mct->mct_curleaf,
	    mct->mct_kperleaf,
	    mct->mct_klen - mct->mct_hilbertsize,
	    DM1B_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, mct->mct_curleaf),
	    DM1B_PLEAF,
	    mct->mct_index_comp, mct->mct_acc_plv, dberr);
    if (status != E_DB_OK)
    {
	(VOID) dm1xfinish(mct, DM1X_CLEANUP, &local_dberr);
	log_error(E_DM925A_DM1B_BEGIN, dberr);
	return (status);
    }
    infinity = TRUE;
    status = dm1cxtput(mct->mct_page_type, mct->mct_page_size, mct->mct_curleaf,
			DM1B_LRANGE, (DM_TID *)&infinity, (i4)0);
    if (status != E_DB_OK)
    {
	dm1cxlog_error(E_DM93EB_BAD_INDEX_TPUT, (DMP_RCB *)mct->mct_oldrcb,
		    mct->mct_curleaf, mct->mct_page_type, mct->mct_page_size,
		    (i4)DM1B_LRANGE);
	(VOID) dm1xfinish(mct, DM1X_CLEANUP, &local_dberr);
	log_error(E_DM925A_DM1B_BEGIN, dberr);
	return (status);
    }
    mct->mct_lastleaf = DM1B_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type,
						 mct->mct_curleaf);

    return(E_DB_OK);
}

/*{
** Name: dm1mbend - Build End: Finishes building an Rtree file for modify.
**
** Description:
**    This routine finsihes building an Rtree for modify.
**
** Inputs:
**      mct                             Pointer to modify context.
**
**
** Outputs:
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
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
**          none.
**
** History:
*/
DB_STATUS
dm1mbend(
	DM2U_M_CONTEXT	*mct,
	DB_ERROR        *dberr)
{
    DB_STATUS       s;
    DB_STATUS       status = E_DB_OK;
    DB_STATUS	    local_err_code;
    i4         	    infinity;
    char            Key[DM1B_MAXSTACKKEY];
    char	    *AllocKbuf = NULL, *EndKey;
    DB_ERROR	    local_dberr;

    CLRDBERR(dberr);

    if (DMZ_AM_MACRO(13))
    {
	TRdisplay("dm1mbend: Completing Rtree table load.\n");
    }

    /*
    ** Set the last leaf's high range to infinity and its nextpage pointer
    ** to zero.
    */
    if (DM1B_VPT_GET_PAGE_STAT_MACRO(mct->mct_page_type,
		mct->mct_curleaf) & DMPP_LEAF)
    {
	status = dm1b_AllocKeyBuf(mct->mct_klen, Key,
				    &EndKey, &AllocKbuf, dberr);
	/*
	** Find the composite MBR of the current leaf.  We will use this MBR
	** as the high range value of this leaf and as the key to insert into
	** the parent index for the new leaf.
	*/
	if ( status == E_DB_OK )
	    status = compute_mbr_lhv(mct, mct->mct_curleaf, EndKey);

	/*
	** Fill in high range for the current leaf.
	*/
	infinity = TRUE;

	if ( status == E_DB_OK )
	{
	    status = put_high_range(mct, mct->mct_curleaf, &infinity, EndKey);

	    if ( status == E_DB_OK )
	    {
		DM1B_VPT_SET_BT_NEXTPAGE_MACRO(mct->mct_page_type,
			mct->mct_curleaf, (DM_PAGENO) 0);

		status = finish_parent( mct, EndKey, dberr );

		if ( status == E_DB_OK && DMZ_AM_MACRO(10))
		{
		    status = TRdisplay("dm1mbend: Bottom page is\n");
		    dmd_prindex(mct->mct_oldrcb, mct->mct_curleaf, 0);
		}
	    }
	}
    }
    else
	status = E_DB_ERROR;

    /* Discard any allocated key buffer */
    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &EndKey);

    if ( status == E_DB_OK )
    {
	/*
	** Write all build buffers to disk and deallocate the build memory.
	** Build the FHDR/FMAP and guarantee all dialloc space
	*/
	status = dm1xfinish(mct, DM1X_COMPLETE, dberr);
    }

    if ( status )
    {
	log_error(E_DM9259_DM1B_BEND, dberr);
	SETDBERR(dberr, 0, E_DM93C2_BBEND_ERROR);
	(VOID) dm1xfinish(mct, DM1X_CLEANUP, &local_dberr);
    }

    return (status);
}

/*{
** Name: finish_parent	- update the last index entry after adding last leaf
**
** Description:
** 	After the last leaf is added, the last entry on each index level
** 	must be updated to contain the mbr of the last leaf.
**
** Inputs:
**      mct                             Pointer to modify context.
**	indexkey			The discriminator key to be added
**					to the parent page.
**
** Outputs:
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
** History:
** 	31-may-1996 (wonst02)
** 	    New.
*/
static DB_STATUS
finish_parent(
	DM2U_M_CONTEXT	*mct,
	char		*indexkey,
	DB_ERROR	*dberr)
{
    DMP_TCB     *t = mct->mct_oldrcb->rcb_tcb_ptr;
    i4	level;
    i4	keylen;
    DB_STATUS	status;

    CLRDBERR(dberr);

    keylen = mct->mct_klen - DM1B_TID_S;

    for (level = 0; level < mct->mct_top; level++)
    {
	/*
	** Get parent page to update the last entry.
	*/

	status = dm1xreadpage(mct, DM1X_FORUPDATE,
	    (i4)mct->mct_ancestors[level],
	    (DMPP_PAGE **)&mct->mct_curindex, dberr);
	if (status != E_DB_OK)
	{
	    SETDBERR(dberr, 0, E_DM93C2_BBEND_ERROR);
	    break;
	}

	/*
	** Replace the last entry on the index page with the updated MBR/LHV.
	*/
	status = dm1cxreplace(mct->mct_page_type, mct->mct_page_size,
			mct->mct_curindex, DM1C_DIRECT, mct->mct_index_comp,
			(DB_TRAN_ID *)NULL, (u_i2)0, (i4)0,
			DM1B_VPT_GET_BT_KIDS_MACRO(mct->mct_page_type,
				mct->mct_curindex) - 1,
			indexkey, keylen, (DM_TID *)NULL, (i4)0 );
	if (status != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E6_BAD_INDEX_REPLACE,
		    (DMP_RCB *)mct->mct_oldrcb, mct->mct_curindex,
		    mct->mct_page_type, mct->mct_page_size,
		    DM1B_VPT_GET_BT_KIDS_MACRO(mct->mct_page_type,
			       			   mct->mct_curindex) - 1 );
	    SETDBERR(dberr, 0, E_DM93C2_BBEND_ERROR);
	    return (E_DB_ERROR);
	}

	/*
	** Find the composite MBR and the LHV of the current page. Use this
	** as the high range value of this page and as the key to replace on
	** its parent.
	*/
	status = compute_mbr_lhv(mct, mct->mct_curindex, indexkey);

	if (status != E_DB_OK)
	{
	    SETDBERR(dberr, 0, E_DM93C2_BBEND_ERROR);
	    return E_DB_ERROR;
	}

	/*
	** Fill in high range for the current index page.
	*/
	status = put_high_range(mct, mct->mct_curindex, 0, indexkey);
	if (status != E_DB_OK)
	{
	    SETDBERR(dberr, 0, E_DM93C2_BBEND_ERROR);
	    return E_DB_ERROR;
	}

    } /* end for */

    return (status);
} /* finish_parent( */


/*{
** Name: dm1mbput - Build Put: Adds a record to Rtree & build index.
**
** Description:
**    This routine builds an Rtree index.  Called by modify or create.
**    All records are assumed to be in sorted order by Hilbert value.
**    This routine also assumes that the record is not compressed.
**    The record is first added to the leaf page, then to the index.
**    Currently the TCB of the table modifying is used for
**    attribute information needed by the low level routines.
**    If attributes are allowed to be modified, then these
**    build routines will not work.
**    When building for INDEX command, can't use information in
**    mct_oldrcb as this describes the base table, not the new
**    index being built.
**    The key for INDEX pages is the mbr and lhv both. For LEAF pages,
**    the key is the mbr only.
**
** Inputs:
**      mct                             Pointer to modify context.
**      record                          Pointer to an area containing
**                                      record to add.
**      record_size                     Size of record in bytes.
**      dup                             A flag indicating if this
**                                      record is a duplicate key.
**
** Outputs:
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
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
**          none.
**
** History:
**      10-mar-97 (stial01)
**          dm1mbput() Removed unecessary tuple alloc/dealloc
**	20-nov-2006 (wanfr01)
**	    SIR 117586
**	    changed arguments for dmd_prkey
*/
DB_STATUS
dm1mbput(
	DM2U_M_CONTEXT   *mct,
	char            *record,
	i4         record_size,
	i4         dup,
	DB_ERROR        *dberr)
{
    char	    *rec = record;
    i4         	    rec_size = record_size;
    DM_TID          tid;
    DB_STATUS       status;
    char	    Key[DM1B_MAXSTACKKEY];
    char	    *AllocKbuf, *IndKey;
    i4	    	    has_room;
    i4         	    leaf_klen;
    DMP_TCB         *t = mct->mct_oldrcb->rcb_tcb_ptr;

    CLRDBERR(dberr);

    if (DMZ_AM_MACRO(13))
    {
	TRdisplay("dm1mbput: Adding record to Rtree\n");
    }

    /*
    ** Can we insert in the current bottom level leaf page?
    ** This is determined by the leaf fill factor, which is always
    ** less than or equal to mct_kperpage.
    */

    leaf_klen = mct->mct_klen - mct->mct_hilbertsize;
    has_room = dm1cxhas_room(mct->mct_page_type, mct->mct_page_size,
			mct->mct_curleaf, mct->mct_index_comp,
			mct->mct_l_fill, mct->mct_kperleaf,
			mct->mct_index_maxentry - mct->mct_hilbertsize);
    if ( has_room )
    {
	/*
	** This is a secondary index. We 'magically' know that the TIDP
	** is the last column in the original uncompressed index entry
	** (mbr, lhv, TIDP), so make a copy of the TIDP into the
	** local variable 'tid'.
	*/
	MEcopy(record + mct->mct_klen - DM1B_TID_S, DM1B_TID_S, (char *)&tid);

	/* Insert the key,tid pair on the leaf */

	status = 
	    dm1mxinsert((DMP_RCB *)NULL, t, mct->mct_page_type, mct->mct_page_size,
			    &mct->mct_index_rac,
			    leaf_klen - DM1B_TID_S,
			    mct->mct_curleaf, rec, &tid,
			    DM1B_VPT_GET_BT_KIDS_MACRO(mct->mct_page_type,
					mct->mct_curleaf),
			    (i4) FALSE, (i4) FALSE, dberr);
	if (status != E_DB_OK)
	{
	    return (status);
	}

        if (!dup)
            mct->mct_dupstart = DM1B_VPT_GET_BT_KIDS_MACRO(mct->mct_page_type,
					mct->mct_curleaf) - 1;

	if (DMZ_AM_MACRO(10))
	{
	    status = TRdisplay("dm1mbput: added key\n");
	    dmd_prkey(mct->mct_oldrcb, rec, DM1B_PLEAF, (DB_ATTS**)NULL, (i4)0, (i4)0);
	    status = TRdisplay("to bottom page\n");
            dmd_prindex(mct->mct_oldrcb, mct->mct_curleaf, 0);
            status = TRdisplay("dupstart %d\n", mct->mct_dupstart);
	}

	/*
	** We've successfully inserted the new record, we can return now.
	*/
        return (E_DB_OK);
    }

    /*
    ** Can't insert into bottom level page because it's full.  We have to
    ** allocate a new leaf page.
    */

    if (DMZ_AM_MACRO(13))
    {
	TRdisplay("dm1mbput: Page %d (stat %x) is full\n",
	    DM1B_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, mct->mct_curleaf),
	    DM1B_VPT_GET_PAGE_STAT_MACRO(mct->mct_page_type, mct->mct_curleaf));
    }

    status = dm1b_AllocKeyBuf(mct->mct_klen, Key,
				&IndKey, &AllocKbuf, dberr);

    if ( status = E_DB_OK )
    {
	status = add_to_next_leaf(mct, record, rec, rec_size, rec,
				    IndKey, dberr);

	if ( status == E_DB_OK )
	    status = update_parent( mct, IndKey, dberr );
    }

    /* Discard any allocated key buffer */
    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &IndKey);

    return (status);
}

/*{
** Name: update_parent			- update index after adding new leaf
**
** Description:
**	If a new leaf was created, then the parent (index) must be updated.
**	which may cause all the index record above it to be updated.
**
** Inputs:
**      mct                             Pointer to modify context.
**	indexkey			The discriminator key to be added
**					to the parent page.
**
** Outputs:
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
** History:
**	30-Jul-1997 (shero03)
**	    Ensure the next page number is cleared after mxformat.
**      25-Mar-2004 (hanal04) Bug 111989 INGSRV2762
**          Correct 2nd parameter in call to dm1cxallocate(). Passing
**          page_type in place of page_size would lead to E_DM93E0.
**	20-nov-2006 (wanfr01)
**	    SIR 117586
**	    changed arguments for dmd_prkey
*/
static DB_STATUS
update_parent(
	DM2U_M_CONTEXT	*mct,
	char		*indexkey,
	DB_ERROR	*dberr)
{
    i4	level;
    DM_PAGENO	child;
    DM_PAGENO	oldparent;
    DM_PAGENO	newparent;
    DM_PAGENO	page;
    DMPP_PAGE	*indexbuf;
    DB_STATUS	status;
    i4	has_room;
    DM_TID      temptid;
    DMP_TCB     *t = mct->mct_oldrcb->rcb_tcb_ptr;

    CLRDBERR(dberr);

    if (DMZ_AM_MACRO(10))
    {
	TRdisplay("dm1mbput: added new leaf: top = %d, bottom is \n",
                        mct->mct_top);
    	dmd_prindex(mct->mct_oldrcb, mct->mct_curleaf, 0);
    }

    if ((indexbuf = (DMPP_PAGE *)dm0m_pgalloc(mct->mct_page_size)) == 0)
    {
	SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
	return (E_DB_ERROR);
    }

    /* Initialize temptid to set the tid line portion of the tid. */
    temptid.tid_i4 = 0;

    for (level = 0, child = mct->mct_lastleaf; level < mct->mct_top; level++)
    {
	/*
	** Get parent page to insert new key,tid entry on.
	*/

	status = dm1xreadpage(mct, DM1X_FORUPDATE,
	    (i4)mct->mct_ancestors[level],
	    (DMPP_PAGE **)&mct->mct_curindex, dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** Check if there is enough room on this page to add a new record.
	** Use the index fill factor which is always less than or equal to
	** mct->mct_kperpage.
	**
	** If there is enough room, break out of this loop and insert the
	** new key below.  If there is not enough room, split the current
	** index and continue the loop to add the new split key into the
	** current index's parent.
	*/
	has_room = dm1cxhas_room(mct->mct_page_type, mct->mct_page_size, 
			mct->mct_curindex, mct->mct_index_comp,
			mct->mct_i_fill, mct->mct_kperpage, 
			mct->mct_index_maxentry);
	if (has_room)
	    break;

	/*
	** Replace last entry on the index page with its updated mbr/lhv.
	*/
	status = dm1cxreplace(mct->mct_page_type, mct->mct_page_size,
			mct->mct_curindex, DM1C_DIRECT, mct->mct_index_comp,
			(DB_TRAN_ID *)NULL, (u_i2)0, (i4)0,
			DM1B_VPT_GET_BT_KIDS_MACRO(mct->mct_page_type,
					mct->mct_curindex) - 1,
			indexkey, mct->mct_klen - DM1B_TID_S, 
			(DM_TID *)NULL, (i4)0 );
	if (status != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E6_BAD_INDEX_REPLACE,
			   (DMP_RCB *)mct->mct_oldrcb, mct->mct_curindex,
			   mct->mct_page_type, mct->mct_page_size,
			   DM1B_VPT_GET_BT_KIDS_MACRO(mct->mct_page_type,
			        		  mct->mct_curindex) - 1 );
	    SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
	    return E_DB_ERROR;
	}

	/*
	** Find the composite MBR and the LHV of the current page. Use as
	** the high range value of this page and the index key of its parent.
	*/
	status = compute_mbr_lhv(mct, mct->mct_curindex, indexkey);

	if (status != E_DB_OK)
	{
	    SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
	    return E_DB_ERROR;
	}

	/*
	** Fill in high range for the current index page.
	*/
	status = put_high_range(mct, mct->mct_curindex, 0, indexkey);
	if (status != E_DB_OK)
	{
	    SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
	    return E_DB_ERROR;
	}

	/*
	** Add a new index page at this level. Allocate a single entry, with no
	** keyspace at the moment, and set that entry to point to our new
	** index page's sole child.
	*/
	status = dm1xnextpage(mct, (i4 *)&page, dberr);
	if (status != E_DB_OK)
	    break;
	/*
	** Set the current index page's nextpage pointer.
	*/
	DM1B_VPT_SET_BT_NEXTPAGE_MACRO(mct->mct_page_type,
				   mct->mct_curindex, page);

	/*
	** Get and initialize new index page.
	*/
	status = dm1xnewpage(mct, 0, (DMPP_PAGE **)&mct->mct_curindex,
								dberr);
	if (status != E_DB_OK)
	    break;

	newparent = DM1B_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type,
					     mct->mct_curindex);

	status = dm1mxformat(mct->mct_page_type, mct->mct_page_size,
			    (DMP_RCB *) 0, t, mct->mct_curindex,
			     mct->mct_kperpage,
			     mct->mct_klen,
			     DM1B_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type,
						      mct->mct_curindex),
			     (level == 0 ? DM1B_PSPRIG : DM1B_PINDEX),
			     mct->mct_index_comp,
			     mct->mct_acc_plv, dberr);
	if (status != E_DB_OK)
	    break;
	DM1B_VPT_SET_BT_NEXTPAGE_MACRO(mct->mct_page_type,
				   mct->mct_curindex, (DM_PAGENO) 0);

	status = dm1cxallocate(mct->mct_page_type, mct->mct_page_size,
			mct->mct_curindex, (i4)DM1C_DIRECT, mct->mct_index_comp,
			(DB_TRAN_ID *)NULL, (i4)0, 0, (i4)DM1B_TID_S);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E0_BAD_INDEX_ALLOC, (DMP_RCB *)mct->mct_oldrcb,
	      mct->mct_curindex, mct->mct_page_type, mct->mct_page_size, (i4)0);
	    SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
	    break;
	}

	temptid.tid_tid.tid_page = child;
	status = dm1cxtput(mct->mct_page_type, mct->mct_page_size, 
			mct->mct_curindex, 0, &temptid, (i4)0);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93EB_BAD_INDEX_TPUT, (DMP_RCB *)mct->mct_oldrcb,
	     mct->mct_curindex, mct->mct_page_type, mct->mct_page_size, (i4)0);
	    SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
	    break;
	}

	oldparent = mct->mct_ancestors[level];
	mct->mct_ancestors[level] = newparent;
	child = newparent;

	/* Continue loop to insert new key into parent */
    } /* end for */

    if (DMZ_AM_MACRO(10))
    {
	TRdisplay("dm1mbput: updated index; status=%d, level=%d, child=%d\n",
			status, level, child);
	TRdisplay("index key is\n");
    	dmd_prkey(mct->mct_oldrcb, indexkey, DM1B_PINDEX, (DB_ATTS**)NULL, (i4)0, (i4)0);
    }

    /* If ancestor was found with some room, add discr to it. */
    if (status == E_DB_OK && level < mct->mct_top)
    {
        temptid.tid_tid.tid_page = child;
        status = 
	    dm1mxinsert((DMP_RCB *)NULL, t, mct->mct_page_type, mct->mct_page_size,
			    &mct->mct_index_rac,
			    mct->mct_klen - DM1B_TID_S, mct->mct_curindex,
			    indexkey, &temptid,
			    (DM1B_VPT_GET_BT_KIDS_MACRO(mct->mct_page_type,
				mct->mct_curindex) - 1),
			    (i4) FALSE, (i4) FALSE, dberr);

	/*
	** We are done updating Rtree index, we can return now.
	*/
	if (status == E_DB_OK)
	{
	    dm0m_pgdealloc((char **)&indexbuf);
	    return (status);
	}
    } /* if (status == E_DB_OK && level <... */

    /*
    ** If we got here, then all the indexes in the current leaf's ancestor
    ** chain were full and we had to add new nodes at their level.  That
    ** means we had to add a new index at the root level.  So now we add a
    ** new root.
    */
    if (mct->mct_top == RCB_MAX_RTREE_LEVEL)
    {
	dm0m_pgdealloc((char **)&indexbuf);
	SETDBERR(dberr, 0, E_DM0057_NOROOM);
	return (E_DB_ERROR);
    }

    /*	Read current root page and save its contents. */

    if (status == E_DB_OK)
    {
	status = dm1xreadpage(mct, DM1X_FORUPDATE, 0,
			(DMPP_PAGE **)&mct->mct_curindex, dberr);
    }
    if (status == E_DB_OK)
    {
	MEcopy((PTR)mct->mct_curindex, mct->mct_page_size,
		   (PTR)indexbuf);
	status = dm1xnextpage(mct, (i4 *)&page, dberr);
    }
    if (status != E_DB_OK)
    {
	log_error(E_DM925B_DM1B_BPUT, dberr);
	dm0m_pgdealloc((char **)&indexbuf);
	return (status);
    }

    /*
    **	Re-format as the new root page.  Insert pointer to new page to
    **	contain previous root contents and insert the key that split
    **	the previous root.
    */

    status = dm1mxformat(mct->mct_page_type, mct->mct_page_size,
				(DMP_RCB *) 0, t,
				mct->mct_curindex,
				mct->mct_kperpage,
				mct->mct_klen,
				DM1B_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type,
					mct->mct_curindex),
				DM1B_PINDEX, mct->mct_index_comp,
				mct->mct_acc_plv, dberr);
    if (status != E_DB_OK)
    {
	log_error(E_DM925B_DM1B_BPUT, dberr);
	dm0m_pgdealloc((char **)&indexbuf);
	return (status);
    }

    DM1B_VPT_SET_BT_NEXTPAGE_MACRO(mct->mct_page_type,
    			       mct->mct_curindex, (DM_PAGENO) 0);

    status = dm1cxallocate(mct->mct_page_type, mct->mct_page_size, 
		    mct->mct_curindex, (i4)DM1C_DIRECT, mct->mct_index_comp,
		    (DB_TRAN_ID *)NULL, (i4)0, 0, (i4)DM1B_TID_S);
    if (status != E_DB_OK)
    {
	dm1cxlog_error(E_DM93E0_BAD_INDEX_ALLOC, (DMP_RCB *)mct->mct_oldrcb,
	    mct->mct_curindex, mct->mct_page_type, mct->mct_page_size, (i4)0);
	SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
	log_error(E_DM925B_DM1B_BPUT, dberr);
	dm0m_pgdealloc((char **)&indexbuf);
	return (E_DB_ERROR);
    }

    temptid.tid_tid.tid_page = page;
    status = dm1cxtput(mct->mct_page_type, mct->mct_page_size, 
		mct->mct_curindex, 0, &temptid, (i4)0);
    if (status != E_DB_OK)
    {
	dm1cxlog_error(E_DM93EB_BAD_INDEX_TPUT, (DMP_RCB *)mct->mct_oldrcb,
	    mct->mct_curindex, mct->mct_page_type, mct->mct_page_size, (i4)0);
	SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
	log_error(E_DM925B_DM1B_BPUT, dberr);
	dm0m_pgdealloc((char **)&indexbuf);
	return (E_DB_ERROR);
    }
    temptid.tid_tid.tid_page = newparent;
    status = dm1mxinsert((DMP_RCB *)NULL, t, mct->mct_page_type,mct->mct_page_size,
			    &mct->mct_index_rac,
			    mct->mct_klen - DM1B_TID_S, mct->mct_curindex,
			    indexkey, &temptid, 0,
			    (i4) FALSE, (i4) FALSE, dberr);
    if (status != E_DB_OK)
    {
	log_error(E_DM925B_DM1B_BPUT, dberr);
	dm0m_pgdealloc((char **)&indexbuf);
	return (status);
    }

    mct->mct_ancestors[mct->mct_top++] = DM1B_ROOT_PAGE;

    /*	Now create the previous root as a child in a new spot. */

    status = dm1xnewpage(mct, 0, (DMPP_PAGE **)&mct->mct_curindex, dberr);
    if (status == E_DB_OK)
    {
	/*
	** Copy contents of the old root page into the new index page.
	** Reset its page_page to be the correct page number.
	*/
	MEcopy((PTR)indexbuf, mct->mct_page_size,
		(PTR)mct->mct_curindex);
	DM1B_VPT_SET_PAGE_PAGE_MACRO(mct->mct_page_type, mct->mct_curindex,
		page);
    } /* if (status == E_DB_OK) */

    if (DMZ_AM_MACRO(10))
    {
	TRdisplay("dm1mbput: created new root: level = %d, root = %d\n",
			    level, DM1B_ROOT_PAGE);
	status = TRdisplay("index key\n");
	dmd_prkey(mct->mct_oldrcb, indexkey, DM1B_PINDEX, (DB_ATTS**)NULL, (i4)0, (i4)0);
	status = TRdisplay("the root page is\n");
	dmd_prindex(mct->mct_oldrcb, mct->mct_curindex, 0);
    }

    if (status != E_DB_OK)
	log_error(E_DM925B_DM1B_BPUT, dberr);
    dm0m_pgdealloc((char **)&indexbuf);
    return (status);
} /* update_parent( */


/*{
** Name: add_to_next_leaf	- add this index entry to next leaf page.
**
** Description:
**	Get a new leaf and put this record in the first position on it.
**
**	The highest key on the current leaf is returned in "indexkey" to be
**	inserted into the parent index page.
**
** Inputs:
**      mct                             Pointer to modify context.
**	record				The record being added
**	rec				"record", compressed if necessary
**	rec_size			record size, possibly after compression
**	key				The key being added.
**	indexkey			buffer into which index key should be
**					placed.
**
** Outputs:
**	indexkey			index key is placed here.
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
** History:
*/
static DB_STATUS
add_to_next_leaf(
	DM2U_M_CONTEXT	*mct,
	char		*record,
	char		*rec,
	i4		rec_size,
	char		*key,
	char		*indexkey,
	DB_ERROR	*dberr)
{
    DB_STATUS	status = E_DB_OK;
    i4	infinity;
    DM_PAGENO	page;
    DM_TID	tid;
    DMP_TCB     *t = mct->mct_oldrcb->rcb_tcb_ptr;
    ADF_CB	*adf_cb = mct->mct_oldrcb->rcb_adf_cb;
    i4     leaf_klen;

    CLRDBERR(dberr);

    for (;;)
    {
	if (DMZ_AM_MACRO(13))
	{
	    TRdisplay("dm1mbput: will go on next leaf.\n");
	}

	/*
	** Find the composite MBR of the current leaf.  We will use this MBR
	** as the high range value of this leaf, the low range value for the
	** new leaf, and as the key to insert into the parent index for the
	** new leaf.
	*/
	status = compute_mbr_lhv(mct, mct->mct_curleaf, indexkey);

	if (status != E_DB_OK)
	    break;

	/*
	** Fill in high range for the current leaf.
	*/
	infinity = FALSE;
	status = put_high_range(mct, mct->mct_curleaf, &infinity, indexkey);
	if (status != E_DB_OK)
	{
	    SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
	    break;
	}

	/*
	** Set the current leaf's nextpage pointer.
	*/
	status = dm1xnextpage(mct, (i4 *)&page, dberr);
	if (status != E_DB_OK)
	    break;
	DM1B_VPT_SET_BT_NEXTPAGE_MACRO(mct->mct_page_type,
		mct->mct_curleaf, page);

	/*
	** Get and initialize new leaf.
	*/
	status = dm1xnewpage(mct, 0, (DMPP_PAGE **)&mct->mct_curleaf, dberr);
	if (status)
	    break;
	status = dm1mxformat(mct->mct_page_type, mct->mct_page_size,
				(DMP_RCB *) 0, t, mct->mct_curleaf,
				mct->mct_kperleaf,
				mct->mct_klen - mct->mct_hilbertsize,
				DM1B_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type,
					mct->mct_curleaf),
				DM1B_PLEAF, mct->mct_index_comp,
				mct->mct_acc_plv, dberr);
	if (status)
	    break;

	/*
	** This is a secondary index -- fetch base table TID from magic
	** last column of uncompressed index entry for leaf's key,tid pair.
	*/
	MEcopy(record + mct->mct_klen - DM1B_TID_S, DM1B_TID_S, (char *)&tid);

	/*
	** Fill in the low range value for the new leaf.
	*/
        infinity = FALSE;
	status = put_low_range(mct, mct->mct_curleaf, &infinity, indexkey);
	if (status != E_DB_OK)
	{
	    SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
	    break;
	}

	/* Put new record on new leaf. */
	leaf_klen = mct->mct_klen - mct->mct_hilbertsize;
        status = 
	    dm1mxinsert((DMP_RCB *)NULL, t, mct->mct_page_type,mct->mct_page_size,
			    &mct->mct_index_rac,
			    leaf_klen - DM1B_TID_S, mct->mct_curleaf,
			    key, &tid, 0,
			    (i4) FALSE, (i4) FALSE, dberr);
	if (status != E_DB_OK)
	    break;

	/* Mark this leaf as the current leaf page. */
        mct->mct_lastleaf = DM1B_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type,
				mct->mct_curleaf);
        mct->mct_dupstart = 0;
	break;
    }

    if (status)
	log_error(E_DM925B_DM1B_BPUT, dberr);
    return (status);
}


/*{
** Name: put_high_range - store the high range value on the page.
**
** Description:
**	If this is an index (not a leaf) page, simply store the range key
**	in the RRANGE slot on the page; it contains the MBR and LHV of the page.
**
**	If this is a leaf page (no LHVs in the keys), store the MBR in the
**	RRANGE slot and put the high LHV into the right half of the LRANGE slot.
**
** Inputs:
**      mct                             Pointer to modify context.
**	page				Pointer to the current page.
**	infinity			Infinity flag (size of a TID).
**	endkey				The high range key (MBR + LHV).
**
** Outputs:
**
**	Returns:
**	    E_DB_OK
**          E_DB_ERROR
**
** History:
**      12-jun-97 (stial01)
**          put_high_range() Pass tlv to dm1cxget instead of tcb.
*/
static DB_STATUS
put_high_range(
	DM2U_M_CONTEXT	*mct,
    	DMPP_PAGE	*page,
	i4		*infinity,
	char		*endkey)
{
	DMP_TCB		*t = mct->mct_oldrcb->rcb_tcb_ptr;
	i4		key_len;
	char		key[DB_MAXRTREE_KEY];
	char		*key_ptr;
	DB_STATUS	status = E_DB_OK;

	/*
	** Fill in high range for the current page.
	*/
	key_len = (DM1B_VPT_GET_PAGE_STAT_MACRO(mct->mct_page_type, page)
	    & DMPP_LEAF) ? mct->mct_klen - mct->mct_hilbertsize - DM1B_TID_S
	    		 : mct->mct_klen - DM1B_TID_S;
	status = dm1cxput(mct->mct_page_type, mct->mct_page_size, page,
		    mct->mct_index_comp, DM1C_DIRECT, (DB_TRAN_ID *)NULL,
		    (u_i2)0, (i4)0, DM1B_RRANGE, endkey, key_len, 
		    (DM_TID *)infinity, (i4)0);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E6_BAD_INDEX_REPLACE,
			(DMP_RCB *)mct->mct_oldrcb, page,
			mct->mct_page_type, mct->mct_page_size, DM1B_RRANGE);
	    return E_DB_ERROR;
	}
	/*
	** If this is a index page--not a leaf page--we're done!
	*/
	if (!(DM1B_VPT_GET_PAGE_STAT_MACRO(mct->mct_page_type, page)
	    		  & DMPP_LEAF))
	    return status;

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
	key_ptr = key;
	status = dm1cxget(mct->mct_page_type, mct->mct_page_size,
			page, &mct->mct_index_rac,
			DM1B_LRANGE, &key_ptr, (DM_TID *)NULL, (i4*)NULL,
			&key_len, NULL, NULL, mct->mct_oldrcb->rcb_adf_cb);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E3_BAD_INDEX_GET,
			(DMP_RCB *)mct->mct_oldrcb, page,
			mct->mct_page_type, mct->mct_page_size, DM1B_LRANGE);
	    return E_DB_ERROR;
	}

	if (key_ptr != key)
	    MEcopy(key_ptr, key_len, key);

	/*
	** Store the Largest Hilbert Value (LHV) in the rightmost
	** portion of the LRANGE key.
	*/
	MEcopy((char *)endkey + (2 * mct->mct_hilbertsize), mct->mct_hilbertsize,
	       (char *)key + mct->mct_hilbertsize);

	status = dm1cxput(mct->mct_page_type, mct->mct_page_size, page,
			mct->mct_index_comp, DM1C_DIRECT, (DB_TRAN_ID *)NULL,
			(u_i2)0, (i4)0, DM1B_LRANGE, key, key_len, 
			(DM_TID *)NULL, (i4)0 );
	if (status != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E6_BAD_INDEX_REPLACE,
			(DMP_RCB *)mct->mct_oldrcb, page,
			mct->mct_page_type, mct->mct_page_size, DM1B_LRANGE);
	    return E_DB_ERROR;
	}

	return status;

} /* put_high_range */


static DB_STATUS
get_high_range(
	DM2U_M_CONTEXT	*mct,
    	DMPP_PAGE	*page,
	i4		*infinity,
	char		*endkey)
{
	DMP_TCB		*t = mct->mct_oldrcb->rcb_tcb_ptr;
	ADF_CB		*adfcb = mct->mct_oldrcb->rcb_adf_cb;
	i4		key_len;
	char		key[DB_MAXRTREE_KEY];
	char		*key_ptr;
	DB_STATUS	status = E_DB_OK;

	/*
	** Get the Minimum Bounding Rectange (MBR) and the infinity
	** flag from the RRANGE key. If not a leaf page, the LHV, too.
	*/
	key_len = (DM1B_VPT_GET_PAGE_STAT_MACRO(mct->mct_page_type, page)
	    & DMPP_LEAF) ? mct->mct_klen - mct->mct_hilbertsize - DM1B_TID_S
	    		 : mct->mct_klen - DM1B_TID_S;
	key_ptr = endkey;
	status = dm1cxget(mct->mct_page_type, mct->mct_page_size,
			page, &mct->mct_index_rac,
			DM1B_RRANGE, &key_ptr, 
			(DM_TID *)infinity, (i4*)NULL,
			&key_len, NULL, NULL, adfcb);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E3_BAD_INDEX_GET,
			   (DMP_RCB *)mct->mct_oldrcb, mct->mct_curleaf,
			   mct->mct_page_type, mct->mct_page_size, DM1B_RRANGE);
	    return E_DB_ERROR;
	}
	if (key_ptr != endkey)
	    MEcopy(key_ptr, key_len, endkey);

	/*
	** If this is a index page--not a leaf page--we're done!
	*/
	if (!(DM1B_VPT_GET_PAGE_STAT_MACRO(mct->mct_page_type, page)
	    		  & DMPP_LEAF))
	    return status;

	/*
	** Get the Largest Hilbert Value (LHV) from the rightmost
	** portion of the LRANGE key in the leaf page.
	*/
	key_ptr = key;
	status = dm1cxget(mct->mct_page_type, mct->mct_page_size,
			page, &mct->mct_index_rac,
			DM1B_LRANGE, &key_ptr, (DM_TID *)NULL, (i4*)NULL,
			&key_len, NULL, NULL, adfcb);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E3_BAD_INDEX_GET,
			   (DMP_RCB *)mct->mct_oldrcb, mct->mct_curleaf,
			   mct->mct_page_type, mct->mct_page_size, DM1B_LRANGE);
	    return E_DB_ERROR;
	}

	MEcopy((char *)key_ptr + mct->mct_hilbertsize, mct->mct_hilbertsize,
	       (char *)endkey + (2 * mct->mct_hilbertsize));

	return status;
} /* get_high_range */


static DB_STATUS
put_low_range(
	DM2U_M_CONTEXT	*mct,
    	DMPP_PAGE	*page,
	i4		*infinity,
	char		*lowkey)
{
	DMP_TCB		*t = mct->mct_oldrcb->rcb_tcb_ptr;
	i4		key_len;
	char		key[DB_MAXRTREE_KEY];
	char		*key_ptr;
	DB_STATUS	status = E_DB_OK;

	if (!(DM1B_VPT_GET_PAGE_STAT_MACRO(mct->mct_page_type, page)
	    		  & DMPP_LEAF))
	{
	    key_ptr = lowkey;
	    key_len = mct->mct_klen - DM1B_TID_S;
	}
	else
	{
	    key_ptr = key;
	    key_len = mct->mct_klen - mct->mct_hilbertsize - DM1B_TID_S;
	    status = dm1cxget(mct->mct_page_type, mct->mct_page_size,
			page, &mct->mct_index_rac,
	    		DM1B_LRANGE, &key_ptr, (DM_TID *)NULL, (i4*)NULL,
			&key_len,
			NULL,NULL,  mct->mct_oldrcb->rcb_adf_cb);
	    if (status != E_DB_OK)
	    {
	        dm1cxlog_error( E_DM93E3_BAD_INDEX_GET,
			(DMP_RCB *)mct->mct_oldrcb, page,
			mct->mct_page_type, mct->mct_page_size, DM1B_LRANGE);
	        return E_DB_ERROR;
	    }
	    if (key_ptr != key)
	        MEcopy(key_ptr, key_len, key);
	    /*
	    ** Store the smallest Hilbert Value and the infinity flag in the
	    ** leftmost portion of the LRANGE key.
	    */
	    MEcopy((char *)lowkey + (2 * mct->mct_hilbertsize), mct->mct_hilbertsize,
	           (char *)key);
	    key_ptr = key;
	}

	status = dm1cxreplace(mct->mct_page_type, mct->mct_page_size, page,
			DM1C_DIRECT, mct->mct_index_comp,
			(DB_TRAN_ID *)NULL, (u_i2)0, (i4)0, DM1B_LRANGE,
			key_ptr, key_len, (DM_TID *)infinity, (i4)0 );
	if (status != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E6_BAD_INDEX_REPLACE,
			(DMP_RCB *)mct->mct_oldrcb, page,
			mct->mct_page_type, mct->mct_page_size, DM1B_LRANGE);
	    return E_DB_ERROR;
	}

	return status;

} /* put_low_range */


static DB_STATUS
compute_mbr_lhv(
	DM2U_M_CONTEXT	*mct,
    	DMPP_PAGE	*page,
	char		*endkey)
{
	DB_STATUS	status = E_DB_OK;
	char		thiskeybuf[DB_MAXRTREE_KEY];
	i4		endkey_len;
	bool		good_mbr;
	i4		kid;
        i4		lastkid;
	DB_DATA_VALUE	endkey_mbr;
	DB_DATA_VALUE	thiskey_mbr;
	DB_DATA_VALUE	endkey_hilbert;
	DMP_TCB		*t = mct->mct_oldrcb->rcb_tcb_ptr;
	ADF_CB		*adf_cb = mct->mct_oldrcb->rcb_adf_cb;

	/*
	** Find the composite MBR of the current leaf.  We will use this MBR
	** as the high range value of this leaf, the low range value for the
	** new leaf, and as the key to insert into the parent index for the
	** new leaf.
	*/
	lastkid = DM1B_VPT_GET_BT_KIDS_MACRO(mct->mct_page_type, page);
	endkey_mbr.db_data = endkey;
	endkey_mbr.db_length = mct->mct_klen - mct->mct_hilbertsize
				- DM1B_TID_S;
	endkey_mbr.db_datatype = mct->mct_acc_klv->nbr_dt_id;
	endkey_mbr.db_prec = 0;
	endkey_mbr.db_collID = -1;
	MEcopy(mct->mct_acc_klv->null_nbr, endkey_mbr.db_length,
	       endkey_mbr.db_data);
	thiskey_mbr = endkey_mbr;
	good_mbr = FALSE;

	for (kid = 0; kid < lastkid && status == E_DB_OK; kid++)
	{
	    /*
	    ** Reset the db_data address each iteration: dm1cxget may
	    ** change it to point directly into the fixed page buffer.
	    */
	    thiskey_mbr.db_data = &thiskeybuf[0];
	    endkey_len = (DM1B_VPT_GET_PAGE_STAT_MACRO(mct->mct_page_type, page)
	    		  & DMPP_LEAF) ?
	    		   endkey_mbr.db_length :
	    		   endkey_mbr.db_length + mct->mct_hilbertsize;
	    status = dm1cxget(mct->mct_page_type, mct->mct_page_size,
			page, &mct->mct_index_rac,
			kid, &thiskey_mbr.db_data,
			(DM_TID *)NULL, (i4*)NULL,
			&endkey_len, NULL, NULL, adf_cb);
	    if (status != E_DB_OK)
	    {
	    	dm1cxlog_error( E_DM93E3_BAD_INDEX_GET,
				(DMP_RCB *)mct->mct_oldrcb, mct->mct_curleaf,
				mct->mct_page_type, mct->mct_page_size, kid );
	    	break;
	    }
	    /*
	    ** First good mbr: save the mbr as the index page mbr.
	    ** For successive mbrs, recompute the total index page mbr.
	    */
	    if (MEcmp(thiskey_mbr.db_data, mct->mct_acc_klv->null_nbr,
		      thiskey_mbr.db_length) != 0)
	    {
	    	if (!good_mbr)
	    	{
	    	    MEcopy(thiskey_mbr.db_data, endkey_mbr.db_length, endkey);
		    good_mbr = TRUE;
	    	}
	    	else
	    	    status = (*mct->mct_acc_klv->dmpp_unbr)(adf_cb,
		    	      &endkey_mbr, &thiskey_mbr, &endkey_mbr);
	    } /* if (MEcmp(thiskey_mbr.db_data, m... */
	} /* for (kid = 0; kid < lastkid && s... */

	if (status != E_DB_OK)
	    return status;

	if (!good_mbr)
	{
	    /*
	    ** If no valid keys (and, hence, no valid MBRs) on the page,
	    **   just reuse the existing LHV from the high range key.
	    */
	    thiskey_mbr.db_data = &thiskeybuf[0];
	    status = get_high_range(mct, page, NULL, thiskey_mbr.db_data);
	    MEcopy((char *)thiskey_mbr.db_data + thiskey_mbr.db_length,
	    	   mct->mct_hilbertsize,
		   (char *)endkey + thiskey_mbr.db_length);
	} /* if (!good_mbr) */
        else if (DM1B_VPT_GET_PAGE_STAT_MACRO(mct->mct_page_type, page) & DMPP_LEAF)
	{
	    /*
	    ** Compute the Hilbert value of the last key on the leaf page
	    ** and append it to the parent index key as the LHV.
	    */
	    endkey_hilbert.db_data = (char *)endkey + endkey_len;
	    endkey_hilbert.db_length = mct->mct_hilbertsize;
	    endkey_hilbert.db_datatype = DB_BYTE_TYPE;
	    endkey_hilbert.db_prec = 0;
	    endkey_hilbert.db_collID = -1;
	    status = (*mct->mct_acc_klv->dmpp_hilbert)(adf_cb,
	    	      &thiskey_mbr, &endkey_hilbert);
	}
	else
	{
	    /*
	    ** Copy the LHV of the last key on the index page
	    ** and append it to the parent index key as the LHV.
	    */
	    MEcopy((char *)thiskey_mbr.db_data + thiskey_mbr.db_length,
	    	   mct->mct_hilbertsize,
		   (char *)endkey + thiskey_mbr.db_length);
	}

	return status;
} /* compute_mbr_lhv */


static VOID
logErrorFcn(
i4	    status,
DB_ERROR    *dberr,
PTR	    FileName,
i4	    LineNumber)
{
    i4		local_err;

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
	    (i4)0, (i4 *)NULL, &local_err, 0);

	dberr->err_code = status;
	dberr->err_file = FileName;
	dberr->err_line = LineNumber;
    }
}
