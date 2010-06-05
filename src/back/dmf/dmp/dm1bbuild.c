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
#include    <dm1c.h>
#include    <dm1cx.h>
#include    <dm1p.h>
#include    <dmtcb.h>
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
**  Name: DM1BBUILD.C - Routines to build a BTREE table.
**
**  Description:
**      This file contains all the routines needed to build
**      a BTREE table. 
**
**      The routines defined in this file are:
**      dm1bbbegin          - Initializes BTREE file for building.
**      dm1bbend            - Finishes building BTREE table.
**      dm1bbput            - Adds a record to a new BTREE table
**                            and builds index.
**
**  History:
**      03-nov-85 (jennifer)
**          Converted for Jupiter.
**      25-nov_87 (jennifer)
**          Added multiple locations per table support.
**	14-mar-89 (rogerk)
**	    Moved over walt's fix having to do with checking for required
**	    space on page when tuple is compressed.  Need to compress record
**	    first in case compressing it causes it to grow in size.
**	29-may-89 (rogerk)
**	    Check status from dm1c_comp_rec calls.
**	14-jun-1991 (Derek)
**	    Major changes to incorporate th hooks for dealing the allocation
**	    structures during the build process.  A lot of the buffering that
**	    happened in this file was moved to a new file DM1X to be
**	    shared by the other access methods.  Other then that the only
**	    major change to the basic alogorithm was to always keep the
**	    root page at page 0 and deal with it on the fly instead of
**	    relocating it after everything was completed.
**	19-aug-1991 (bryanp)
**	    Added support for compressed index btrees. This basically requires
**	    calling the new dm1cx() routines to perform physical space
**	    management on index pages.
**	10-sep-1991 (bryanp)
**	    B37642,B39322: Check for infinity == FALSE on LRANGE/RRANGE entries.
**	17-oct-1991 (rogerk)
**	    Took out line that turned off SPRIG status in the new index
**	    page when adding a new level beneath the root.
**      23-oct-1991 (jnash)
**          Within dm1bbput, remove apparently superfluous parameter in call to
**          add_to_overflow, detected by LINT.
**	 7-jul-1992 (rogerk)
**	    Prototyping DMF.
**	05_jun_1992 (kwatts)
**	    MPF change, calls to dm1c_xxx now use accessors.
**    	29-August-1992 (rmuth)
**          Add call to dm1x_build_SMS to build the FHDR/FMAP(s).
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	26-oct-92 (rickh)
**	    Replaced references to DMP_ATTS with DB_ATTS.  Also replaced
**	    DM_CMP_LIST with DB_CMP_LIST.
**	30-October-1992 (rmuth)
**	    Change for new DI file extend build paradigm
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project: Add arguments to dm1bxinsert call.
**	    and change arguments to dm1cxformat.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	19-jul-1993 (pearl)
**          Bug 48084: Btree index page LRANGEs are null, resulting in no rows
**          erroneously being returned on key searches.  In dm1bbput():
**          as a btree is being built, if the current index page runs out of
**          room, dm1cxget() is called to get the last key on the current
**          page.  This key is then used as the LRANGE of the new page.  One
**          of the arguments to dm1cxget() is a pointer to a local buffer, in
**          which the key may be returned. In the case of uncompressed indices,
**          the pointer is set to point to the key on the current page, and
**          the local buffer is not used.  The fix is not only to copy the key
**          from the pointer to the local buffer after the return from 
**	    dm1cxget(), which is already done, but also to set the pointer 
**	    (endkey) again to the local buffer (indexkey).  The call to 
**	    newpage() may reinitialize the current page, rendering the 
**	    contents of the pointer null.
**	18-oct-1993 (rmuth)
**	    Use fillfactor for data pages.
**	18-oct-1993 (rogerk)
**	    Initialize tid variable in update_parent() so that we don't insert
**	    garbage in the tid_line portion of index page BID entries.
**      20-may-95 (hanch04)
**          Added include dmtcb.h
**	06-mar-1996 (stial01 for bryanp)
**	    Pass page_size argument to dm1cxhas_room, dm1cxmidpoint.
**	    Pass mct_page_size as the page_size argument to dm1cxformat.
**	    Pass page_size to various other dm1cx* accessor functions.
**	    Use mct_page_size rather than DM_PG_SIZE in kperpage calculations.
**	06-mar-1996 (stial01 for bryanp)
**	    Don't allocate tuple buffers on the stack.
**      06-mar-1996 (stial01)
**          Call dm1bxinsert with page_size of secondary index
**          Limit mct_kperpage to (DM_TIDEOF + 1) - DM1B_OFFSEQ
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**      15-jul-1996 (ramra01 for bryanp)
**          Pass 0 as the current table version to dmpp_load.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          load accessor: changed DMPP_TUPLE_INFO param to table_version
**      27-jan-97 (stial01)
**          For sec. indexes, copy TIDP from (mct->mct_relwid - DM1B_TID_S)
**          instead of (mct->mct_keylen - DM1B_TID_S), since TIDP is no longer 
**          part of the key for unique secondary indexes. 
**      28-jan-97 (stial01)
**          add_to_next_leaf(), add_with_dups(), add_to_overflow():
**          For sec. indexes, copy TIDP from (mct->mct_relwid - DM1B_TID_S)
**          instead of (mct->mct_keylen - DM1B_TID_S), since TIDP is no longer
**          part of the key for unique secondary indexes.
**          Note for non-unique indexes: mct_relwid == mct_keylen
**      10-mar-97 (stial01)
**          dm1bbput: Use mct_crecord to compress a record
**      07-apr-97 (stial01)
**          dm1bbput: always add_to_next_leaf if page_size != DM_COMPAT_PGSIZE
**      12-jun-97 (stial01)
**          Pass tlv to dm1cxget instead of tcb.
**          Include attribute information in kperpage calculation
**          Pass new attribute, key parameters to dm1bxformat/dm1cxformat
**      17-nov-97 (stial01)
**          B87272: keys per page calculation use index mct_page_size
**	01-dec-1998 (nanpr01)
**	    Get rid of DMPP_TUPLE_INFO structure. Use DMPP_TUPLE_HDR instead.
**      07-Dec-1998 (stial01)
**          Added kperleaf to distinguish from kperpage (keys per index page)
**      09-feb-99 (stial01)
**          dm1bbbegin() Pass relcmptlvl (DMF_T_VERSION) to dm1b_kperpage
**      12-apr-1999 (stial01)
**          Btree secondary index: non-key fields only on leaf pages
**          Different att/key info for LEAF vs. INDEX pages
**	20-oct-1999 (nanpr01)
**	    Optimized the code to minimize the tuple header copy.
**      12-nov-1999 (stial01)
**          Changes to how btree key info is stored in tcb
**      06-jul-2000 (stial01)
**          dm1bbbegin() Fixed test for last key == tidp (b102026)
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Arg changes for page accessors (Variable Page Type SIR 102955)
**      02-apr-2001 (stial01)
**          Fixed tidp attr name comparison for SQL92 databases (b104111)
**	19-may-2001 (somsa01)
**	    Added missing parameter to dmpp_load() in add_to_overflow() due
**	    to previous cross-integrations.
**      08-Oct-2002 (hanch04)
**          Removed dm0m_lcopy,dm0m_lfill.  Use MEcopy, MEfill instead.
**      02-jan-2004 (stial01)
**          Added lsn parameter to dm1bxinsert to support NOBLOBLOGGING bulk
**          put blob segments. (data page lsn = leaf lsn for REDO recovery)
**	22-Jan-2004 (jenjo02)
**	    A host of changes for Partitioning and Global indexes,
**	    particularly row-locking with TID8s (which include
**	    partition number (search for "partno") along with TID).
**	    LEAF pages in Global indexes will contain DM_TID8s 
**	    rather than DM_TIDs; the TID size is now stored on
**	    the page (4 for all but LEAF, 8 for TID8 leaves).
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      13-may-2004 (stial01)
**          Remove unecessary code for NOBLOBLOGGING redo recovery
**	13-Apr-2006 (jenjo02)
**	    Build CLUSTERED primary tables, which have no data pages
**	    and in which the leaf entry is the row.
**	13-Jun-2006 (jenjo02)
**	    Using DM1B_MAXSTACKKEY instead of DM1B_KEYLENGTH, call dm1b_AllocKeyBuf() 
**	    to validate/allocate Leaf work space. DM1B_KEYLENGTH is now only the 
**	    max length of key attributes, not necessarily the length of the Leaf entry.
**      20-oct-2006 (stial01)
**          Disable CLUSTERED index build changes for ingres2006r2
**	24-Feb-2008 (kschendel) SIR 122739
**	    Changes throughout for new row-accessor structure.
**	02-Dec-2008 (jonj)
**	    SIR 120874: dm1b? functions converted to DB_ERROR *
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1x?, dm1?b? functions converted to DB_ERROR *,
**	    use new form uleFormat.
**      10-sep-2009 (stial01)
**          Add duplicates to overflow (DMPP_CHAIN) pages again.
**      12-oct-2009 (stial01)
**          Use DMPP_PAGE for all page types.
**      04-dec-2009 (stial01)
**          add_with_dups() limit range of leaf with overflow pages
**	15-Jan-2010 (jonj,stial01)
**	    SIR 121619 MVCC: Replace DMPP_PAGE* with DMP_PINFO* where needed.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/*
** Maximum no of levels: 
** Maximum number of tuples is 2**(sizeof(DM_TID)*8), 
** so with at least 2 pointers per index page we need 
** no more than sizeof(DM_TID)*8 levels of index.
** Note, this value is hard coded into DM2U_M_CONTEXT. 
*/

# define DM1BBMAXLEVEL	(sizeof(DM_TID) * 8 + 1)
# define DM1BBNOPARENT	(0x7fffffff)

static DB_STATUS    update_parent(
				    DM2U_M_CONTEXT	*mct,
				    char		*indexkey,
				    DB_ERROR		*dberr);
static DB_STATUS    add_to_next_leaf(
				    DM2U_M_CONTEXT	*mct,
				    char		*record,
				    char		*rec,
				    i4		rec_size,
				    char		*key,
				    char		*indexkey,
				    DB_ERROR		*dberr);
static DB_STATUS    add_with_dups(
				    DM2U_M_CONTEXT	*mct,
				    char		*record,
				    char		*rec,
				    i4		rec_size,
				    char		*key,
				    char		*indexkey,
				    DB_ERROR		*dberr);
static DB_STATUS    add_to_overflow(
				    DM2U_M_CONTEXT	*mct,
				    char		*record,
				    char		*rec,
				    i4		rec_size,
				    char		*key,
				    DB_ERROR		*dberr);
static VOID	    logErrorFcn(
				i4 		status, 
				DB_ERROR 	*dberr,
				PTR		FileName,
				i4		LineNumber);
#define	log_error(status,dberr) \
	logErrorFcn(status,dberr,__FILE__,__LINE__)

/*{
** Name: dm1bbbegin - Initializes BTREE file for modify.
**
** Description:
**    This routine initializes the BTREE for building.
**    The root, header, first leaf and first data pages are
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
**      27-jan-86 (jennifer)
**          Converted for Jupiter.
**	19-aug-1991 (bryanp)
**	    Call dm1cx routines to support index compression.
**    	29-August-1992 (rmuth)
**          Remove call to dm1xheader as FHDR/FMAP are now built by
**          dm1x_build_SMS.
**	18-oct-1993 (rmuth)
**	    Use fillfactor for data pages.
**	06-mar-1996 (stial01 for bryanp)
**	    Use mct_page_size rather than DM_PG_SIZE in kperpage calculations.
**	06-May-1996 (nanpr01)
**	    Changed the page header access as macro. Also changed the kperpage
**	    calculation.
**	16-sep-1996 (nanpr01)
** 	    local variable tid.tid_line needs to be initialized.
**      07-apr-97 (stial01)
**          dm1bbput: always add_to_next_leaf if page_size != DM_COMPAT_PGSIZE
**      12-jun-97 (stial01)
**          dm1bbbegin() Pass new attribute, key parameters to dm1bxformat.
**          dm1bbbegin() Include attribute information in kperpage calculation
**      17-nov-97 (stial01)
**          B87272: keys per page calculation use index mct_page_size
**      09-feb-99 (stial01)
**          dm1bbbegin() Pass relcmptlvl (DMF_T_VERSION) to dm1b_kperpage
**	06-Jan-2004 (jenjo02)
**	    Added mct_partno, mct_tidsize to DM2U_M_CONTEXT.
**	    mct_tidsize replaces DM1B_TID_S define.
**	14-Apr-2006 (kschendel)
**	    Kill mildly annoying leftover debug message.
*/
DB_STATUS
dm1bbbegin(
DM2U_M_CONTEXT   *mct,
DB_ERROR         *dberr)
{
    CL_ERR_DESC	    sys_err;
    DB_STATUS       dbstatus;
    STATUS          status;
    DM_TID	    tid; 
    i4	    infinity;
    DMP_TCB         *t = mct->mct_oldrcb->rcb_tcb_ptr; 
    DB_ATTS         **atts;
    i4              klen;
    i4              i,j;
    i4		    local_err;
    DB_ERROR	    local_dberr;

    CLRDBERR(dberr);
    
    if (DMZ_AM_MACRO(13))
    {
	TRdisplay("dm1bbbegin: Beginning Btree table load.\n");
    }

    /*
    **	Allocate build memory and use SAVE buffers.
    */

    status = dm1xstart(mct, DM1X_SAVE, dberr);
    if (status != E_DB_OK)
	return (status);

    /* 
    ** Set up the btree build information.
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
    ** Init BTREE attr/key info in MCT
    ** We now distinguish atts/key arrays for LEAF/INDEX pages
    **
    ** Global indexes will have TID8s in the LEAF pages, but
    ** not in INDEX pages.
    ** mct_klen is set up by modify / index / load to be the full leaf
    ** row including any non-key data columns (in a secondary index).
    ** For primary btrees, include the tid pointer to the data row.
    ** SI doesn't have a data row and the tidp column is already in klen.
    */
    mct->mct_leaf_keys = mct->mct_key_atts;
    status = dm1c_rowaccess_setup(&mct->mct_leaf_rac);
    if (status != E_DB_OK)
	return (status);
    mct->mct_leaf_maxentry = mct->mct_klen
			+ mct->mct_leaf_rac.worstcase_expansion;
    if (! mct->mct_index)
	mct->mct_leaf_maxentry += mct->mct_tidsize;

    /*
    ** If a secondary index having NON-KEY fields, INDEX entries are 
    ** different from LEAF entries. 
    ** Since key fields always precede NON-KEY fields (except tidp), 
    ** we can re-use the LEAF DB_ATTS entries (except tidp)
    */
    if ( mct->mct_index && mct->mct_keys != mct->mct_leaf_rac.att_count)
    {
	atts = mct->mct_leaf_keys;
	klen = 0;
	for (i = 0, j = 0; i < mct->mct_keys; i++)
	{
	    /*
	    ** last key componant may be tidp which may have different
	    ** offset/key_offset on leaf vs. index pages
	    ** leaf pages include non key atts, index pages do not
	    */
	    if ((i + 1 == mct->mct_keys) && atts[i]->key_offset != klen)
	    {
		/* Consistency check for tidp/TIDP */
		if (STcompare("tidp", atts[i]->attnmstr) &&
		    STcompare("TIDP", atts[i]->attnmstr))
		{
		    TRdisplay("%@ dm1bbbegin att %d is %s instead of tidp, table %~t\n",
			i, atts[i]->attnmstr,
			sizeof(DB_TAB_NAME), &t->tcb_rel.relid);
		    return(E_DB_ERROR);
		}

		/*
		** Last leaf attribute is always tidp
		** Offset of tidp may be different on leaf/index pg
		** due to non-key fields present on leaf page,
		** but not present on index page
		*/ 
		mct->mct_cx_ix_keys[j] = &mct->mct_cx_tidp;
		STRUCT_ASSIGN_MACRO(*atts[i], mct->mct_cx_tidp);
		mct->mct_cx_tidp.offset = klen; 
		mct->mct_cx_tidp.key_offset = klen;
	    }
	    else
		mct->mct_cx_ix_keys[j] = atts[i];	
	    j++;
	    klen += atts[i]->length;
	}

	/* For secondary index, atts = keys on INDEX pages */
	mct->mct_index_rac.att_ptrs = &mct->mct_cx_ix_keys[0];
	mct->mct_index_rac.att_count = j;
	status = dm1c_rowaccess_setup(&mct->mct_index_rac);
	if (status != E_DB_OK)
	    return (status);
	mct->mct_ix_keys = &mct->mct_cx_ix_keys[0];
	mct->mct_ixklen = klen;
	mct->mct_index_maxentry = klen + mct->mct_index_rac.worstcase_expansion
		+ sizeof(DM_TID); /* TID8's only on LEAF page */
    }
    else
    {
	/* SI with everything key, or non-SI primary btree: */
	status = dm1c_rowaccess_setup(&mct->mct_index_rac);
	if (status != E_DB_OK)
	    return (status);
	mct->mct_ix_keys = mct->mct_leaf_keys; 
	mct->mct_ixklen = mct->mct_klen;
	/* Index entry is same as leaf.
	** **** FIXME actually this may be a lie;  INDEX pages always have
	** 4-byte tids even if it's a global index.  This means that the
	** index maxentry of a global index might be 4 too large.
	** However, dm2t makes the same ?mistake in calculating ixklen
	** and I am not about to fret over it now... (kschendel feb 08)
	*/
	mct->mct_index_maxentry = mct->mct_leaf_maxentry;
    }

    /*
    ** Figure out the btree fill factor information:
    **    - how many keys fit on an index page.
    **    - How full data pages should be
    **
    ** Note that TID8's are possible only on LEAF pages.
    */
    mct->mct_kperpage = 
		dm1b_kperpage(mct->mct_page_type, mct->mct_page_size,
				DMF_T_VERSION, mct->mct_index_rac.att_count,
				mct->mct_ixklen, DM1B_PINDEX,
				sizeof(DM_TID), 0);
    mct->mct_kperleaf = 
		dm1b_kperpage(mct->mct_page_type, mct->mct_page_size,
				DMF_T_VERSION, mct->mct_leaf_rac.att_count,
				mct->mct_klen, DM1B_PLEAF,
				mct->mct_tidsize, 0);

    mct->mct_db_fill = mct->mct_page_size -
			    (mct->mct_page_size * mct->mct_d_fill) / 100;

    if (DMZ_AM_MACRO(13))
    {
	TRdisplay("dm1bbbegin: d-fill=%d(%d)\n", mct->mct_d_fill, 
	    mct->mct_db_fill);

	TRdisplay(
	    "Leaf  attcnt=%d klen=%d maxent=%d kperpage %d fill %d(%d), tidsize %d\n",
	    mct->mct_leaf_rac.att_count, mct->mct_klen,
	    mct->mct_leaf_maxentry, mct->mct_kperleaf,
	    mct->mct_l_fill, mct->mct_kperleaf 
				* mct->mct_l_fill / 100,
	    mct->mct_tidsize);

	TRdisplay(
	    "Index attcnt=%d klen=%d maxent=%d kperpage %d fill=%d(%d)\n",
	    mct->mct_index_rac.att_count, mct->mct_ixklen,
	    mct->mct_index_maxentry, mct->mct_kperpage,
	    mct->mct_i_fill, mct->mct_kperpage 
				* mct->mct_i_fill / 100);
    }

    if (DMZ_AM_MACRO(12))
    {
	/* Display att/key arrays for leaf/index entries */
	TRdisplay("dm1bbuild: LEAF atts %d entrylen %d \n", 
		mct->mct_leaf_rac.att_count, mct->mct_klen);
	for (i = 0; i < mct->mct_leaf_rac.att_count; i++)
	    TRdisplay("ATT %s (%d %d)\n",
	    mct->mct_leaf_rac.att_ptrs[i]->attnmstr,
	    mct->mct_leaf_rac.att_ptrs[i]->offset, 
	    mct->mct_leaf_rac.att_ptrs[i]->key_offset);
	for (i = 0; i < mct->mct_keys; i++)
	    TRdisplay("KEY %s (%d %d)\n",
	    mct->mct_leaf_keys[i]->attnmstr,
	    mct->mct_leaf_keys[i]->offset, 
	    mct->mct_leaf_keys[i]->key_offset);

	TRdisplay("dm1bbuild: INDEX atts %d entrylen %d \n", 
		mct->mct_index_rac.att_count, mct->mct_ixklen);
	for (i = 0; i < mct->mct_index_rac.att_count; i++)
	    TRdisplay("ATT %s (%d %d)\n",
	    mct->mct_index_rac.att_ptrs[i]->attnmstr,
	    mct->mct_index_rac.att_ptrs[i]->offset, 
	    mct->mct_index_rac.att_ptrs[i]->key_offset);
	for (i = 0; i < mct->mct_keys; i++)
	    TRdisplay("KEY %s (%d %d)\n",
	    mct->mct_ix_keys[i]->attnmstr,
	    mct->mct_ix_keys[i]->offset, 
	    mct->mct_ix_keys[i]->key_offset);
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
    if (!mct->mct_index)
	status = dm1xnewpage(mct, 0, &mct->mct_curdata, dberr);
    if (status != E_DB_OK)
    {
	(VOID) dm1xfinish(mct, DM1X_CLEANUP, &local_dberr);
	log_error(E_DM925A_DM1B_BEGIN, dberr);
	return (status);
    }

    /*	Format index and place one index record on INDEX to point to leaf. */

    status = dm1bxformat(mct->mct_page_type, mct->mct_page_size,
	    (DMP_RCB *) 0, t,
	    mct->mct_curindex,
	    mct->mct_kperpage,
	    mct->mct_ixklen, 
	    mct->mct_index_rac.att_ptrs,
	    mct->mct_index_rac.att_count,
	    mct->mct_ix_keys,
	    mct->mct_keys,
	    DM1B_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, mct->mct_curindex),
	    DM1B_PSPRIG,
	    mct->mct_index_comp, mct->mct_acc_plv, 
	    sizeof(DM_TID), 0, dberr);
    if (status != E_DB_OK)
    {
	(VOID) dm1xfinish(mct, DM1X_CLEANUP, &local_dberr);
	log_error(E_DM925A_DM1B_BEGIN, dberr);
	return (status);
    }
    status = dm1cxallocate(mct->mct_page_type, mct->mct_page_size, 
			mct->mct_curindex, (i4)DM1C_DIRECT, mct->mct_index_comp,
			(DB_TRAN_ID *)NULL, (i4)0, 0, (i4)sizeof(DM_TID));
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
    status = dm1cxtput(mct->mct_page_type, mct->mct_page_size, 
	mct->mct_curindex, 0, &tid, mct->mct_partno );
    if (status != E_DB_OK)
    {
	dm1cxlog_error(E_DM93EB_BAD_INDEX_TPUT, (DMP_RCB *)mct->mct_oldrcb,
	    mct->mct_curindex, mct->mct_page_type, mct->mct_page_size, (i4)0);
	(VOID) dm1xfinish(mct, DM1X_CLEANUP, &local_dberr);
	log_error(E_DM925A_DM1B_BEGIN, dberr);
	return (status);
    }

    /*	Format leaf and place a -infinity as low key value of leaf. */

    status = dm1bxformat(mct->mct_page_type, mct->mct_page_size,
	    (DMP_RCB *) 0, t, mct->mct_curleaf,
	    mct->mct_kperleaf,
	    mct->mct_klen, 
	    mct->mct_leaf_rac.att_ptrs,
	    mct->mct_leaf_rac.att_count,
	    mct->mct_leaf_keys,
	    mct->mct_keys,
	    DM1B_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, mct->mct_curleaf),
	    DM1B_PLEAF,
	    mct->mct_index_comp, mct->mct_acc_plv,
	    mct->mct_tidsize, 0, dberr);
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
	    mct->mct_curleaf, mct->mct_page_type, mct->mct_page_size, (i4)DM1B_LRANGE);
	(VOID) dm1xfinish(mct, DM1X_CLEANUP, &local_dberr);
	log_error(E_DM925A_DM1B_BEGIN, dberr);
	return (status);
    }
    mct->mct_lastleaf = DM1B_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, 
						 mct->mct_curleaf);

    /*	Set associated data page for leaf if not an index. */

    if (!mct->mct_index)
    {
	DM1B_VPT_SET_BT_DATA_MACRO(mct->mct_page_type, mct->mct_curleaf,
		(DM_PAGENO) DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type,
						     mct->mct_curdata));
	DMPP_VPT_INIT_PAGE_STAT_MACRO(mct->mct_page_type, mct->mct_curdata,
				  DMPP_DATA | DMPP_ASSOC | DMPP_PRIM);
    }

    return(E_DB_OK); 
}

/*{
** Name: dm1bbend - Finishes building a BTREE file for modify.
**
** Description:
**    This routine finsihes building a BTREE for modify.
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
**      27-jan-86 (jennifer)
**          Converted for Jupiter.
**	19-aug-1991 (bryanp)
**	    Call dm1cx routines to support index compression.
**      29-August-1992 (rmuth)
**          Add call to dm1x_build_SMS.
**	30-October-1992 (rmuth)
**	    Change for new DI file extend build paradigm, where the
**	    fhdr/fmap and space is guaranteed in dm1xfinish.
**	06-May-1996 (nanpr01)
**	    Changed the page header access as macro.
*/
DB_STATUS
dm1bbend(
DM2U_M_CONTEXT	*mct,
DB_ERROR        *dberr)
{
    DM_PAGENO       chained;
    DB_STATUS       s;
    DB_STATUS       status = E_DB_OK;
    i4         infinity;
    DB_ERROR	    local_dberr;

    CLRDBERR(dberr);

    if (DMZ_AM_MACRO(13))
    {
	TRdisplay("dm1bbend: Completing Btree table load.\n");
    }

    /*
    ** Set the last leaf's high range to infinity and its nextpage pointer
    ** to zero.  If the last leaf is an overflow chain, we have to set
    ** these fields on the entire chain.
    */
    if (DM1B_VPT_GET_PAGE_STAT_MACRO(mct->mct_page_type, mct->mct_curleaf) & 
	DMPP_LEAF)
    {
        infinity = TRUE;
        status = dm1cxtput(mct->mct_page_type, mct->mct_page_size, 
			mct->mct_curleaf, DM1B_RRANGE, 
			(DM_TID *)&infinity, (i4)0);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93EB_BAD_INDEX_TPUT, (DMP_RCB *)mct->mct_oldrcb,
		mct->mct_curleaf, mct->mct_page_type, mct->mct_page_size, (i4)DM1B_RRANGE);
	    SETDBERR(dberr, 0, E_DM93C2_BBEND_ERROR);
	    return (E_DB_ERROR);
	}
	DM1B_VPT_SET_BT_NEXTPAGE_MACRO(mct->mct_page_type, mct->mct_curleaf, 
				   (DM_PAGENO) 0);

	if (DMZ_AM_MACRO(10))
	{
	    status = TRdisplay("dm1bbend: Bottom page is\n");
	    dmd_prindex(mct->mct_oldrcb, mct->mct_curleaf, 0);
	}
    }
    else
    {
        chained = mct->mct_lastleaf; 
        while (chained != 0) 
        {
	    status = dm1xreadpage(mct, DM1X_FORUPDATE, chained,
		(DMPP_PAGE **)&mct->mct_curleaf, dberr);
	    if (status != E_DB_OK)
		break;

	    if (DMZ_AM_MACRO(10))
	    {
		status = TRdisplay("dm1bbend: Tracing last chain, page %d\n",
		    chained);
	        dmd_prindex(mct->mct_oldrcb, mct->mct_curleaf, 0);
	    }

	    infinity = TRUE; 
	    status = dm1cxtput(mct->mct_page_type, mct->mct_page_size, 
			    mct->mct_curleaf, DM1B_RRANGE, 
			    (DM_TID *)&infinity, (i4)0);
	    if (status != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93EB_BAD_INDEX_TPUT,
		    (DMP_RCB *)mct->mct_oldrcb, mct->mct_curleaf, 
		    mct->mct_page_type, mct->mct_page_size, (i4)DM1B_RRANGE);
		SETDBERR(dberr, 0, E_DM93C2_BBEND_ERROR);
		break;
	    }
	    DM1B_VPT_SET_BT_NEXTPAGE_MACRO(mct->mct_page_type, mct->mct_curleaf,
				       (DM_PAGENO) 0);
            chained = DM1B_VPT_GET_PAGE_OVFL_MACRO(mct->mct_page_type, 
					       mct->mct_curleaf);
        } /* end while */
    } /* end else */ 

    /*	
    ** Write all build buffers to disk and deallocate the build memory.
    ** Build the FHDR/FMAP and guarantee all dialloc space
    */
    s = dm1xfinish(mct, DM1X_COMPLETE, &local_dberr);
    if (s != E_DB_OK)
    {
        status = s; 
	*dberr = local_dberr;

        (VOID) dm1xfinish(mct, DM1X_CLEANUP, &local_dberr);
    }


    if (status != E_DB_OK)
	log_error(E_DM9259_DM1B_BEND, dberr);

    return (status);
}

/*{
** Name: dm1bbput - Adds a record to BTREE & build index.
**
** Description:
**    This routine builds a BTREE table.  Called by modify. All
**    records are assumed to be in sorted order by key.
**    This routine also assumes that the record is not compressed.
**    Duplicates are noted by an input parameter.  The record
**    is first added to the data page, then to the index.
**    Currently the TCB of the table modifying is used for
**    attribute information needed by the low level routines.
**    If attributes are allowed to be modified, then these
**    build routines will not work. 
**    When building for INDEX command, can't use information in
**    mct_oldrcb as this describes the base table, not the new
**    index being built.  Created new macros, dm1bkbget and dm1bkbput to
**    use instead of dm1bkey_get, dm1bkey_put which required an rcb.
**    When building secondary INDEX, use entire record as key, even if
**    only some attributes are keyed.  We need to put the entire record
**    into the btree index, as there are no data pages in a sec index.
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
**      27-jan-86 (jennifer)
**          Converted for Jupiter.
**	14-mar-89 (walt)
**	    Fixed bug #4941 where a "compressed" record gets longer and
**	    the check in dm1bbput() for whether another row will fit on
**	    a page succeeds even though there's not enough room for the
**	    new longer row to fit.  This damages the line table.  First
**	    observed while modifying a table with a lot of CHAR cols to
**	    cbtree.
**	29-may-89 (rogerk)
**	    Check status from dm1c_comp_rec calls.
**	19-aug-1991 (bryanp)
**	    Call dm1cx routines to support index compression.
**      23-oct-1991 (jnash)
**          Remove apparently superfluous parameter in call to 
**	    add_to_overflow detected by LINT.
**	18-oct-1993 (rmuth)
**	    Use fillfactor for data pages.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass page_size argument to dm1cxhas_room, dm1cxmidpoint.
**	06-mar-1996 (stial01 for bryanp)
**	    Don't allocate tuple buffers on the stack.
**	06-May-1996 (nanpr01 & thaju02)
**	    Changed the page header access as macro.
**	    Use mct->mct_acc_plv to access page level vector.
**	20-may-1996 (ramra01)
**	    New param added to the dmpp_load accessor
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          load accessor: changed DMPP_TUPLE_INFO param to table_version
**      27-jan-97 (stial01)
**          For sec. indexes, copy TIDP from (mct->mct_relwid - DM1B_TID_S)
**          instead of (mct->mct_keylen - DM1B_TID_S), since TIDP is no longer 
**          part of the key for unique secondary indexes. 
**      10-mar-97 (stial01)
**          dm1bbput: Use mct_crecord to compress a record
**	28-Feb-2001 (thaju02)
**	    Pass mct->mct_ver_number to dmpp_load. (B104100)
**	06-Jan-2004 (jenjo02)
**	    Use mct_tidsize instead of DM1B_TID_S
**	20-nov-2006 (wanfr01)
**	    SIR 117586
**	    changed arguments for dmd_prkey
**        
*/
DB_STATUS
dm1bbput(
DM2U_M_CONTEXT   *mct,
char            *record,
i4         record_size,
i4         dup,
DB_ERROR         *dberr)
{
    char	    *rec = record;
    i4         rec_size = record_size;
    char	    *key;
    DM_PAGENO       pageno ,root, child, page, newparent;
    i4         level; 
    DM_TID          temptid,tid; 
    DM_TID8	    tid8;
    i4		    partno;
    i4         infinity; 
    DB_STATUS       status;
    i4	    k; 
    char	    *pos; 
    DB_ATTS	    *att; 
    i4         save_infinity[2]; 
    DM_PAGENO       mainpageno; 
    char            *endkey, indkey[DM1B_KEYLENGTH]; 
    char            key_buf[DM1B_KEYLENGTH];
    char            bounds[2][DM1B_KEYLENGTH]; 
    i4	    has_room;
    DMP_TCB         *t = mct->mct_oldrcb->rcb_tcb_ptr; 
    DMP_PINFO	    pinfo;

    DMP_PINIT(&pinfo);

    CLRDBERR(dberr);
 
    if (DMZ_AM_MACRO(13))
    {
	TRdisplay("dm1bbput: Adding record to Btree\n");
    }

    /*
    ** If not secondary index, make key from record.
    ** If secondary index, use entire record as key.
    */

    key = rec;
    if ( ! mct->mct_index)
    {
	key = key_buf;
	for (k = 0; k < mct->mct_keys; k++) 
	{
            att = mct->mct_key_atts[k];
            MEcopy((PTR)(rec + att->offset), att->length,
		(PTR) &key[att->key_offset]);
	}

	/*  Compress record if required. */

	if (mct->mct_data_rac.compression_type != TCB_C_NONE)
	{
	    rec = mct->mct_crecord;
	    status = (*mct->mct_data_rac.dmpp_compress)(&mct->mct_data_rac,
				record, record_size,
				rec, &rec_size);
	    if (status != E_DB_OK)
	    {
		SETDBERR(dberr, 0, E_DM938B_INCONSISTENT_ROW);
		return (status);
	    }
	}
    }

    /* 
    ** Can we insert in the current bottom level leaf page? 
    ** This is determined by the leaf fill factor which is always 
    ** less than or equal to mct->mct_kperleaf and by the type
    ** of the page and the duplicate indication for the record.  If the
    ** current page is leaf any record, duplicate or not can be placed
    ** on the page.  Only duplicate records can be stored on the page
    ** if this is a overflow leaf page, aka DMPP_CHAIN.
    */

    has_room = dm1cxhas_room(mct->mct_page_type, mct->mct_page_size,
				mct->mct_curleaf, mct->mct_index_comp,
				mct->mct_l_fill,
				mct->mct_kperleaf,
				mct->mct_leaf_maxentry);
    if ( has_room &&
         ((DM1B_VPT_GET_PAGE_STAT_MACRO(mct->mct_page_type, mct->mct_curleaf) & 
	   DMPP_LEAF) || dup == TRUE))
    {
	/*
	** Insert the record on the data page and get the tid to insert on
	** the leaf page. If this is a secondary index, just use
	** the tid specified in the record.
	**
	** If this is not a secondary index, then check if there is room
	** on the associated data page.  If not, get a new data page.
	*/

	if ( ! mct->mct_index)
	{
	    partno = 0;

	    while (dm1xbput(mct, mct->mct_curdata, rec, rec_size,
			DM1C_LOAD_NORMAL, mct->mct_db_fill, &tid,
			mct->mct_ver_number, dberr) == E_DB_WARN)
	    {
		/* 
		** Mark old data page as not associated and get a new one.
		** Update current leaf's bt_data pointer to point to the new
		** data page.
		*/

		DMPP_VPT_CLR_PAGE_STAT_MACRO(mct->mct_page_type, mct->mct_curdata,
					 DMPP_ASSOC);
		status = dm1xnewpage(mct, 0, &mct->mct_curdata, dberr);
		if (status != E_DB_OK)
		{
		    log_error(E_DM925B_DM1B_BPUT, dberr);
		    return (status);
		}
		DMPP_VPT_INIT_PAGE_STAT_MACRO(mct->mct_page_type, mct->mct_curdata,
		    DMPP_DATA | DMPP_ASSOC | DMPP_PRIM );
		DM1B_VPT_SET_BT_DATA_MACRO(mct->mct_page_type, mct->mct_curleaf, 
		       (DM_PAGENO) DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, 							    mct->mct_curdata));
	    }
	}
	else
	{
	    /*
	    ** This is a secondary index. We 'magically' know that the TIDP
	    ** is the last column in the original uncompressed index entry, so
	    ** we now make a copy of the TIDP into the local variable 'tid'.
	    */
	    if ( mct->mct_tidsize == sizeof(DM_TID8) )
	    {
		/* Extract TID and partition number from index entry */
		MEcopy(record + mct->mct_relwid - mct->mct_tidsize,
			mct->mct_tidsize, (char*)&tid8);
		tid.tid_i4 = tid8.tid_i4.tid;
		partno = tid8.tid.tid_partno;
	    }
	    else
	    {
		/* TID only */
		MEcopy(record + mct->mct_relwid - mct->mct_tidsize, 
		      mct->mct_tidsize, (char *)&tid);
		partno = 0;
	    }
	}

	/* Insert the key,tid pair on the leaf */
	pinfo.page = mct->mct_curleaf;

	status = dm1bxinsert((DMP_RCB *) 0, t, 
		    mct->mct_page_type, mct->mct_page_size,
		    &mct->mct_leaf_rac,
		    mct->mct_klen,
		    &pinfo, key, &tid, partno,
		    (DM_LINE_IDX)DM1B_VPT_GET_BT_KIDS_MACRO(mct->mct_page_type,
		    mct->mct_curleaf), (i4) FALSE, (i4) FALSE, dberr);
	if (status != E_DB_OK)
	{
	    return (status);
	}
        if (dup == FALSE)
            mct->mct_dupstart = DM1B_VPT_GET_BT_KIDS_MACRO(mct->mct_page_type, 
						       mct->mct_curleaf) - 1;

	if (DMZ_AM_MACRO(10))
	{
	    status = TRdisplay("dm1bbput: added key\n"); 
	    dmd_prkey(mct->mct_oldrcb, key, DM1B_PLEAF, (DB_ATTS**)NULL, (i4)0, (i4)0);
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
    **
    ** If this record is a NOT a duplicate of the last record, then we can
    ** just get a new leaf and put this record in the first position on it.
    **
    ** If this is a duplicate of the last record, then this record must go
    ** on the same leaf (or on an overflow page connected to the same leaf)
    ** as the last record.
    **
    ** If the current leaf is already full of duplicates of this record
    ** (indicated by mct->mct_dupstart == 0), then allocate an overflow page
    ** and put the current record on it.
    **
    ** If the current leaf is not full of duplicates, then allocate a new leaf,
    ** move all the duplicates from the current leaf to the new one and insert
    ** the new record into the next available spot.
    */

    if (DMZ_AM_MACRO(13))
    {
	TRdisplay("dm1bbput: Page %d (stat %x) is full\n",
		DM1B_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, mct->mct_curleaf), 
		DM1B_VPT_GET_PAGE_STAT_MACRO(mct->mct_page_type, mct->mct_curleaf));
    }

    if (dup == FALSE)
	status = add_to_next_leaf(mct, record, rec, rec_size, key,
				    indkey, dberr);
    else if (mct->mct_dupstart != 0)
	status = add_with_dups(mct, record, rec, rec_size, key,
				    indkey, dberr);
    else
	status = add_to_overflow(mct, record, rec, rec_size, key,
/*	    			indkey, dberr);   */
                                 dberr);


    if (DB_FAILURE_MACRO(status))
	return (status);

    if (status == E_DB_INFO)
	return (E_DB_OK);

    status = update_parent( mct, indkey, dberr );

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
**	19-aug-1991 (bryanp)
**	    Created.
**	17-oct-1991 (rogerk)
**	    Took out line that turned off SPRIG status in the new index
**	    page when adding a new level beneath the root.  If the old
**	    root had been a sprig level page, then when we copied the contents
**	    of the old root to the new index page, the new index page would
**	    now be a sprig.  We were then turning off the sprig value, but
**	    it should have been correct to leave it on.  This line caused
**	    us to create btrees that had the leftmost sprig not marked SPRIG.
**	18-oct-1993 (rogerk)
**	    Initialize temptid so that we don't insert garbage in the tid_line
**	    portion of index page BID entries.  As well as being sloppy, it
**	    causes recovery to complain when it does not see identical values
**	    inserted during rollforward processing.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass page_size argument to dm1cxhas_room, dm1cxmidpoint.
**      12-jun-97 (stial01)
**          update_parent() Pass new attribute, key parameters to dm1bxformat.
**	20-nov-2006 (wanfr01)
**	    SIR 117586
**	    changed arguments for dmd_prkey
*/
static DB_STATUS
update_parent(
DM2U_M_CONTEXT	*mct,
char		*leafkey,
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
    DM_TID          temptid;
    DMP_TCB         *t = mct->mct_oldrcb->rcb_tcb_ptr; 
    char            temp_buf[DM1B_KEYLENGTH];
    char            *indexkey;
    DMP_PINFO	    pinfo;

    DMP_PINIT(&pinfo);

    CLRDBERR(dberr);

    if (mct->mct_leaf_rac.att_count != mct->mct_index_rac.att_count)
    {
	indexkey = temp_buf;
	dm1b_build_key(mct->mct_keys, mct->mct_leaf_keys, leafkey,
			mct->mct_ix_keys, indexkey);
    }
    else
	indexkey = leafkey;

    if (DMZ_AM_MACRO(10))
    {
	TRdisplay("dm1bbput: added new leaf: top = %d, bottom is \n", 
                        mct->mct_top);
    	dmd_prindex(mct->mct_oldrcb, mct->mct_curleaf, 0);
    }

    if ((indexbuf = (DMPP_PAGE *)dm0m_pgalloc(mct->mct_page_size)) == 0)
    {
	SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
	return (E_DB_ERROR);
    }

    /* Initialize temptid to set tid_line portion of the tid value. */
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
				mct->mct_i_fill,
				mct->mct_kperpage,
				mct->mct_index_maxentry);
	if (has_room)
	    break;

	/*
	** Add a new index page at this level. Allocate a single entry, with no
	** keyspace at the moment, and set that entry to point to our new
	** index page's sole child.
	*/
	status = dm1xnewpage(mct, 0, (DMPP_PAGE **)&mct->mct_curindex,
								dberr);
	if (status != E_DB_OK)
	    break;

	newparent = DM1B_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, 
					     mct->mct_curindex);

	status = dm1bxformat(mct->mct_page_type, mct->mct_page_size,
				(DMP_RCB *) 0, t, mct->mct_curindex,
				mct->mct_kperpage,
				mct->mct_ixklen, 
				mct->mct_index_rac.att_ptrs,
				mct->mct_index_rac.att_count,
				mct->mct_ix_keys,
				mct->mct_keys,
				DM1B_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, 
							 mct->mct_curindex),
				(level == 0 ? DM1B_PSPRIG : DM1B_PINDEX),
				mct->mct_index_comp, mct->mct_acc_plv, 
				sizeof(DM_TID), 0, dberr);
	if (status != E_DB_OK)
	    break;

	status = dm1cxallocate(mct->mct_page_type, mct->mct_page_size,
			mct->mct_curindex, (i4)DM1C_DIRECT, mct->mct_index_comp,
			(DB_TRAN_ID *)NULL, (i4)0, 0, (i4)sizeof(DM_TID));
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
	TRdisplay("dm1bbput: updated index; status=%d, level=%d, child=%d\n",
			status, level, child);
	TRdisplay("index key is\n");
    	dmd_prkey(mct->mct_oldrcb, indexkey, DM1B_PINDEX, (DB_ATTS**)NULL, (i4)0, (i4)0);
    }

    /* If ancestor was found with some room, add discr to it. */
    if (status == E_DB_OK && level < mct->mct_top)
    {
        temptid.tid_tid.tid_page = child;
	pinfo.page = mct->mct_curindex;
        status = dm1bxinsert((DMP_RCB *) 0, t, 
		mct->mct_page_type, mct->mct_page_size,
		&mct->mct_index_rac,
		mct->mct_ixklen, &pinfo,
		indexkey, &temptid, (i4)0,
		(DM_LINE_IDX)(DM1B_VPT_GET_BT_KIDS_MACRO(mct->mct_page_type,
		mct->mct_curindex) - 1), (i4) FALSE, (i4) FALSE, dberr);

	/*
	** We are done updating Btree index, we can return now.
	*/
	if (status == E_DB_OK)
	{
	    dm0m_pgdealloc((char **)&indexbuf);
	    return (status);
	}
    }

    /*
    ** If we got here, then all the indexes in the current leaf's ancestor
    ** chain were full and we had to add new leaf's at their level.  That
    ** means we had to add a new index at the root level.  So now we add a
    ** new root.
    */
    if (mct->mct_top == DM1BBMAXLEVEL)
    {
	SETDBERR(dberr, 0, E_DM0057_NOROOM);
	dm0m_pgdealloc((char **)&indexbuf);
	return (E_DB_ERROR);
    }

    /*	Read current root page and save it's contents. */

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
    **	contain previous root content's and insert the key that split
    **	the previous root.
    */

    status = dm1bxformat(mct->mct_page_type, mct->mct_page_size,
				(DMP_RCB *) 0, t,
				mct->mct_curindex,
				mct->mct_kperpage,
				mct->mct_ixklen, 
				mct->mct_index_rac.att_ptrs,
				mct->mct_index_rac.att_count,
				mct->mct_ix_keys,
				mct->mct_keys,
				DM1B_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, 
							 mct->mct_curindex),
				DM1B_PINDEX, mct->mct_index_comp, 
				mct->mct_acc_plv, 
				sizeof(DM_TID), 0, dberr);
    if (status != E_DB_OK)
    {
	log_error(E_DM925B_DM1B_BPUT, dberr);
	dm0m_pgdealloc((char **)&indexbuf);
	return (status);
    }

    status = dm1cxallocate(mct->mct_page_type, mct->mct_page_size,
			mct->mct_curindex, (i4)DM1C_DIRECT, mct->mct_index_comp,
			(DB_TRAN_ID *)NULL, (i4)0, 0, (i4)sizeof(DM_TID));
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
    pinfo.page = mct->mct_curindex;
    status = dm1bxinsert((DMP_RCB *) 0, t, 
		mct->mct_page_type, mct->mct_page_size,
		&mct->mct_index_rac,
		mct->mct_ixklen, &pinfo,
		indexkey, &temptid, (i4)0, 0, 
		(i4) FALSE, (i4) FALSE, dberr);
    if (status != E_DB_OK)
    {
	log_error(E_DM925B_DM1B_BPUT, dberr);
	dm0m_pgdealloc((char **)&indexbuf);
	return (status);
    }

    mct->mct_ancestors[mct->mct_top++] = DM1B_ROOT_PAGE;

    /*	Now create the previous root as a leaf in a new spot. */

    status = dm1xnewpage(mct, 0, (DMPP_PAGE **)&mct->mct_curindex, dberr);
    if (status == E_DB_OK)
    {
	/*
	** Copy contents of the old root page into the new index page.
	** Reset its page_page to be the correct page number.
	*/
	MEcopy((PTR)indexbuf, mct->mct_page_size,
			(PTR)mct->mct_curindex);
	DM1B_VPT_SET_PAGE_PAGE_MACRO(mct->mct_page_type, mct->mct_curindex, page);
    }

    if (DMZ_AM_MACRO(10))
    {
	TRdisplay("dm1bbput: created new root: level = %d, root = %d\n",
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
}

/*{
** Name: add_to_next_leaf	- add this index entry to next leaf page.
**
** Description:
**	Since this record is a NOT a duplicate of the last record, we can
**	just get a new leaf and put this record in the first position on it.
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
**	19-aug-1991 (bryanp)
**	    Created.
**      19-jul-1993 (pearl)
**          Bug 48084: Btree index page LRANGEs are null, resulting in no rows
**          erroneously being returned on key searches.  In dm1bbput():
**          as a btree is being built, if the current index page runs out of
**          room, dm1cxget() is called to get the last key on the current
**          page.  This key is then used as the LRANGE of the new page.  One
**          of the arguments to dm1cxget() is a pointer to a local buffer, in
**          which the key may be returned. In the case of uncompressed indices,
**          the pointer is set to point to the key on the current page, and
**          the local buffer is not used.  The fix is not only to copy the key
**          from the pointer to the local buffer after the return from
**          dm1cxget(), which is already done, but also to set the pointer
**	    (endkey) again to the local buffer (indexkey).  The call to 
**	    newpage() may reinitialize the current page, rendering the 
**	    contents of the pointer null.
**	06-May-1996 (nanpr01 & thaju02)
**	    Changed the page header access as macro.
**	    Use mct->mct_acc_plv to access page level vector.
**	20-may-1996 (ramra01)
**	    Added new param tup_info to load accessor
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          load accessor: changed DMPP_TUPLE_INFO param to table_version
**      28-jan-97 (stial01)
**          For sec. indexes, copy TIDP from (mct->mct_relwid - DM1B_TID_S)
**          instead of (mct->mct_keylen - DM1B_TID_S), since TIDP is no longer 
**          part of the key for unique secondary indexes. 
**      12-jun-97 (stial01)
**          add_to_next_leaf() Pass tlv to dm1cxget instead of tcb.
**          Pass new attribute, key parameters to dm1bxformat.
**	28-Feb-2001 (thaju02)
**	    Pass mct->mct_ver_number to dmpp_load. (B104100)
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
    char	cindexkey_buf[DM1B_KEYLENGTH];
    char	*endkey;
    i4	endkey_len;
    i4	indkey_len;
    DM_LINE_IDX	lastkid;
    i4	infinity;
    DM_PAGENO	page;
    DM_TID	tid;
    DM_TID8	tid8;
    i4		partno;
    DMP_TCB     *t = mct->mct_oldrcb->rcb_tcb_ptr; 
    DMP_PINFO	pinfo;

    DMP_PINIT(&pinfo);

    CLRDBERR(dberr);
 
    for (;;)
    {
	if (DMZ_AM_MACRO(13))
	{
	    TRdisplay("dm1bbput: Not a duplicate; will go on next leaf.\n");
	}

	/*
	** Find the highest key on the current leaf.  We will use this key
	** as the high range value of this leaf, the low range value for the
	** next leaf, and as the key to insert into the parent index for the
	** new leaf.
	*/
	lastkid = DM1B_VPT_GET_BT_KIDS_MACRO(mct->mct_page_type, mct->mct_curleaf)
				 - 1;
	endkey = indexkey;
	endkey_len = mct->mct_klen;
	status = dm1cxget(mct->mct_page_type, mct->mct_page_size, 
		    mct->mct_curleaf, &mct->mct_leaf_rac,
		    lastkid, &endkey, (DM_TID *)NULL, (i4*)NULL,
		    &endkey_len,
		    NULL, NULL, mct->mct_oldrcb->rcb_adf_cb);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, (DMP_RCB *)mct->mct_oldrcb,
		mct->mct_curleaf, mct->mct_page_type, mct->mct_page_size, lastkid );
	    SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
	    break;
	}
	if (endkey != indexkey)
	{
	    MEcopy(endkey, endkey_len, indexkey);
	    endkey = indexkey;
	}
	if (mct->mct_index_comp != DM1CX_UNCOMPRESSED)
	{
	    status = dm1cxcomp_rec(&mct->mct_leaf_rac,
		    indexkey,
		    mct->mct_klen, cindexkey_buf, &indkey_len);
	    if (status != E_DB_OK)
	    {
		/*FIX_ME -- need to log something here */

		SETDBERR(dberr, 0, E_DM93E7_INCONSISTENT_ENTRY);
		break;
	    }
	    endkey = cindexkey_buf;
	    endkey_len = indkey_len;
	}

	/*
	** Fill in high range for the current leaf.
	*/
	infinity = FALSE;
	status = dm1cxreplace(mct->mct_page_type, mct->mct_page_size,
			mct->mct_curleaf, DM1C_DIRECT, mct->mct_index_comp,
			(DB_TRAN_ID *)NULL, (u_i2)0, (i4)0,
			DM1B_RRANGE, endkey, endkey_len, 
			(DM_TID *)&infinity, (i4)0 );
	if (status != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E6_BAD_INDEX_REPLACE,
		    (DMP_RCB *)mct->mct_oldrcb, mct->mct_curleaf,
		    mct->mct_page_type, mct->mct_page_size, DM1B_RRANGE );
	    SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
	    break;
	}

	/*
	** Set the current leaf's nextpage pointer.
	*/
	status = dm1xnextpage(mct, (i4 *)&page, dberr);
	if (status != E_DB_OK)
	    break;
	DM1B_VPT_SET_BT_NEXTPAGE_MACRO(mct->mct_page_type, mct->mct_curleaf, page);

	/*
	** If the current page is a CHAIN, then read the whole chain
	** so that the bt_next can be set now that it is known.
	*/

	if (DM1B_VPT_GET_PAGE_STAT_MACRO(mct->mct_page_type, mct->mct_curleaf) & 
	    DMPP_CHAIN)
	{
	    status = dm1xreadpage(mct, DM1X_FORUPDATE,
		mct->mct_lastleaf, (DMPP_PAGE **)&mct->mct_curleaf, dberr);
	    if (status == E_DB_OK)
	    {
		DM1B_VPT_SET_BT_NEXTPAGE_MACRO(mct->mct_page_type, 
					   mct->mct_curleaf, page);
		while (DM1B_VPT_GET_PAGE_OVFL_MACRO(mct->mct_page_type, 
						mct->mct_curleaf))
		{
		    status = dm1xreadpage(mct, DM1X_FORUPDATE,
			DM1B_VPT_GET_PAGE_OVFL_MACRO(mct->mct_page_type, 
						 mct->mct_curleaf),
			(DMPP_PAGE **)&mct->mct_curleaf, dberr);
		    if (status != E_DB_OK)
			break;
		    DM1B_VPT_SET_BT_NEXTPAGE_MACRO(mct->mct_page_type, 
					       mct->mct_curleaf, page);
		}
	    }
	}
	if (status)
	    break;
	    
	/*
	** Get and initialize new leaf.
	*/
	status = dm1xnewpage(mct, 0, (DMPP_PAGE **)&mct->mct_curleaf, dberr);
	if (status)
	    break;
	status = dm1bxformat(mct->mct_page_type, mct->mct_page_size,
				(DMP_RCB *) 0, t,
				mct->mct_curleaf,
				mct->mct_kperleaf,
				mct->mct_klen, 
				mct->mct_leaf_rac.att_ptrs,
				mct->mct_leaf_rac.att_count,
				mct->mct_leaf_keys,
				mct->mct_keys,
				DM1B_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, 
							 mct->mct_curleaf),
				DM1B_PLEAF, mct->mct_index_comp, 
				mct->mct_acc_plv, 
				mct->mct_tidsize, 0, dberr);
	if (status)
	    break;

	/*
	** Get an associated data page for this leaf (unless secondary index)
	** and put the new record on it. If secondary index, get tid specified
	** in record for leaf's key,tid pair.
	*/
	if ( ! mct->mct_index)
	{
	    partno = 0;

	    status = dm1xnewpage(mct, 0, &mct->mct_curdata, dberr);
	    if (status == E_DB_OK)
	    {
		DMPP_VPT_INIT_PAGE_STAT_MACRO(mct->mct_page_type, mct->mct_curdata,
		    DMPP_DATA | DMPP_ASSOC | DMPP_PRIM );
		DM1B_VPT_SET_BT_DATA_MACRO(mct->mct_page_type, mct->mct_curleaf, 
				DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type,
							 mct->mct_curdata));

		dm1xbput (mct, mct->mct_curdata, rec, rec_size,
			DM1C_LOAD_NORMAL, 0, &tid,
			mct->mct_ver_number, dberr);
	    }
	}
	else
	{
	    /*
	    ** This is a secondary index -- fetch base table TID from magic
	    ** last column of uncompressed index entry
	    */
	    if ( mct->mct_tidsize == sizeof(DM_TID8) )
	    {
		/* Extract TID and partition number from index entry */
		MEcopy(record + mct->mct_relwid - mct->mct_tidsize,
			mct->mct_tidsize, (char*)&tid8);
		tid.tid_i4 = tid8.tid_i4.tid;
		partno = tid8.tid.tid_partno;
	    }
	    else
	    {
		/* TID only */
		MEcopy(record + mct->mct_relwid - mct->mct_tidsize, 
		      mct->mct_tidsize, (char *)&tid);
		partno = 0;
	    }
	}

	if (status)
	    break;

	/*
	** Fill in the low range value for the new leaf.
	*/
        infinity = FALSE;
	status = dm1cxreplace(mct->mct_page_type, mct->mct_page_size,
			mct->mct_curleaf, DM1C_DIRECT, mct->mct_index_comp,
			(DB_TRAN_ID *)NULL, (u_i2)0, (i4)0,
			DM1B_LRANGE, endkey, endkey_len, 
			(DM_TID *)&infinity, (i4)0 );
	if (status != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E6_BAD_INDEX_REPLACE,
			(DMP_RCB *)mct->mct_oldrcb, mct->mct_curleaf,
			mct->mct_page_type, mct->mct_page_size, DM1B_LRANGE );
	    SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
	    break;
	}

	/* Put new record on new leaf. */
	pinfo.page = mct->mct_curleaf;
        status = dm1bxinsert((DMP_RCB *) 0, t,
		    mct->mct_page_type, mct->mct_page_size,
		    &mct->mct_leaf_rac,
		    mct->mct_klen, &pinfo,
		    key, &tid, partno, 0, 
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
** Name: add_with_dups	    - add this rec, and dups from prev page, to page
**
** Description:
**	This is a duplicate key but the current leaf is not full of
**	duplicates.  Move all the duplicate keys to a new leaf page.
**
**	The discriminator key on the current leaf is returned in "indexkey" to
**	be inserted into the parent index page.
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
**	indexkey			Discriminator key is returned
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
**	19-aug-1991 (bryanp)
**	    Created.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project: Removed TCB parameter from dm1cxformat.
**      19-jul-1993 (pearl)
**          Bug 48084: Btree index page LRANGEs are null, resulting in no rows
**          erroneously being returned on key searches.  In dm1bbput():
**          as a btree is being built, if the current index page runs out of
**          room, dm1cxget() is called to get the last key on the current
**          page.  This key is then used as the LRANGE of the new page.  One
**          of the arguments to dm1cxget() is a pointer to a local buffer, in
**          which the key may be returned. In the case of uncompressed indices,
**          the pointer is set to point to the key on the current page, and
**          the local buffer is not used.  The fix is not only to copy the key
**          from the pointer to the local buffer after the return from
**          dm1cxget(), which is already done, but also to set the pointer
**          (endkey) again to the local buffer (indexkey).  The call to
**          newpage() may reinitialize the current page, rendering the
**          contents of the pointer null.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass mct_page_size as the page_size argument to dm1cxformat.
**	    Don't allocate pages on the stack -- allocate them dynamically.
**	06-May-1996 (nanpr01 & thaju02)
**	    Changed the page header access as macro.
**	    Use mct->mct_acc_plv to access page level vector.
**	21-may-1996 (ramra01)
**	    Added new param to the get accessor routine
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**          Added DMPP_TUPLE_INFO argument to dm1cxget()
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          load accessor: changed DMPP_TUPLE_INFO param to table_version
**      28-jan-97 (stial01)
**          For sec. indexes, copy TIDP from (mct->mct_relwid - DM1B_TID_S)
**          instead of (mct->mct_keylen - DM1B_TID_S), to be consistent with
**          other code. For non-unique indexes, mct_relwid == mct_keylen
**          anyway.
**      12-jun-97 (stial01)
**          add_with_dups() Pass tlv to dm1cxget instead of tcb.
**          add_with_dups() Pass new attribute, key parameters to dm1cxformat.
**	28-Feb-2001 (thaju02)
**	    Pass mct->mct_ver_number to dmpp_load. (B104100)
*/
static DB_STATUS
add_with_dups(
DM2U_M_CONTEXT	*mct,
char		*record,
char		*rec,
i4		rec_size,
char		*key,
char		*indexkey,
DB_ERROR	*dberr)
{
    DB_STATUS	status = E_DB_OK;
    DMPP_PAGE	*index = 0;
    char	cindexkey_buf[DM1B_KEYLENGTH];
    char	*endkey;
    i4	endkey_len;
    i4	indkey_len;
    DM_LINE_IDX	lastkid;
    i4	infinity;
    DM_PAGENO	page;
    DM_TID	tid;
    DM_TID8	tid8;
    i4		partno;
    DMP_TCB     *t = mct->mct_oldrcb->rcb_tcb_ptr; 
    char	    *decrkey, *AllocKbuf = NULL;
    char	decr_kbuf[DM1B_KEYLENGTH];
    ADF_CB	*adf_cb = mct->mct_oldrcb->rcb_adf_cb;
    i4		compare;
    DMP_PINFO	pinfo;

    DMP_PINIT(&pinfo);

    CLRDBERR(dberr);
 
    for (;;)
    {
	if (DMZ_AM_MACRO(13))
	{
	    TRdisplay("dm1bbput: Shifting all duplicates to new leaf page\n");
	}
	/*
	** Format a temporary buffer as an index page and move all the
	** duplicates to it for storage until we have formatted our new
	** leaf page. Since we won't actually use this page, format it as
	** page number 0.
	*/
	if ((index = (DMPP_PAGE *)dm0m_pgalloc(mct->mct_page_size)) == 0)
	{
	    status = E_DB_ERROR;
	    SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
	    break;
	}

	status = dm1cxformat(mct->mct_page_type, mct->mct_page_size, index,
			    mct->mct_acc_plv, mct->mct_index_comp,
			    mct->mct_kperleaf,
			    mct->mct_klen, 
			    mct->mct_leaf_rac.att_ptrs,
			    mct->mct_leaf_rac.att_count,
			    mct->mct_leaf_keys,
			    mct->mct_keys,
			    (DM_PAGENO)0, DM1B_PLEAF, TCB_BTREE,
			    mct->mct_tidsize, 0);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E5_BAD_INDEX_FORMAT,
			(DMP_RCB *)mct->mct_oldrcb, index,
			mct->mct_page_type, mct->mct_page_size, (DM_LINE_IDX)0);
	    SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
	    break;
	}
	status = dm1cxrshift(mct->mct_page_type, mct->mct_page_size, 
				mct->mct_curleaf, index, mct->mct_index_comp,
				mct->mct_dupstart,
				mct->mct_klen + mct->mct_tidsize); 
	if (status != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E9_BAD_INDEX_RSHIFT,
		    mct->mct_oldrcb, mct->mct_curleaf, 
		    mct->mct_page_type, mct->mct_page_size, mct->mct_dupstart);
	    dm1cxlog_error( E_DM93E9_BAD_INDEX_RSHIFT,
		    mct->mct_oldrcb, index,
		    mct->mct_page_type, mct->mct_page_size, mct->mct_dupstart);
	    SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
	    break;
	}

	/*
	** Find the highest key on the current leaf.  We will use this key
	** as the high range value of this leaf, the low range value for the
	** next leaf, and as the key to insert into the parent index for the
	** new leaf.
	*/
	lastkid = mct->mct_dupstart - 1;
	endkey = indexkey;
	endkey_len = mct->mct_klen;
	status = dm1cxget(mct->mct_page_type, mct->mct_page_size, 
			    mct->mct_curleaf, &mct->mct_leaf_rac,
			    lastkid, &endkey, (DM_TID *)NULL, (i4*)NULL,
			    &endkey_len, 
			    NULL, NULL, adf_cb);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, (DMP_RCB *)mct->mct_oldrcb,
		mct->mct_curleaf, mct->mct_page_type, mct->mct_page_size, lastkid );
	    SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
	    break;
	}

	/*
	** Try to decrement the dupkey to use for
	** the rrange of this leaf and the lrange of next leaf
	** instead of endkey
	*/
	if (DMZ_AM_MACRO(18))
	{
	    TRdisplay("add_with_dups: dupkey\n");
	    dmd_print_key(key, mct->mct_page_type, mct->mct_leaf_keys, 0,
		 mct->mct_keys, adf_cb);
	    TRdisplay("add_with_dups: endkey\n");
	    dmd_print_key(endkey, mct->mct_page_type, mct->mct_leaf_keys, 0,
		 mct->mct_keys, adf_cb);
	}

	if ( status = dm1b_AllocKeyBuf(t->tcb_klen, decr_kbuf,
				    &decrkey, &AllocKbuf, dberr) )
	    break;
	MEcopy(key, mct->mct_klen, decrkey);
	status = adt_key_decr(adf_cb,
	    mct->mct_keys, mct->mct_leaf_keys, mct->mct_klen, decrkey);
	if (status == E_DB_OK)
	{
	    if (DMZ_AM_MACRO(18))
	    {
		TRdisplay("add_with_dups: decrkey\n");
		dmd_print_key(decrkey, mct->mct_page_type, mct->mct_leaf_keys, 0,
		mct->mct_keys, adf_cb);
	    }

	    /*
	    ** if  endkey < decrkey < key (dupkey)
	    ** then set endkey = decrkey
	    */

	    status = adt_kkcmp(adf_cb, mct->mct_keys, mct->mct_leaf_keys, key, decrkey, &compare);
	    if (status == E_DB_OK && compare > DM1B_SAME)
	    {
		status = adt_kkcmp(adf_cb, mct->mct_keys, mct->mct_leaf_keys, decrkey, endkey, &compare);
		if (status == E_DB_OK && compare > DM1B_SAME)
		    endkey = decrkey;
	    }
	}

	if (DMZ_AM_MACRO(18) && endkey != decrkey)
	{
	    TRdisplay("can't decrement, or no need to \n");
	}

	if (endkey != indexkey)
	{
	    MEcopy(endkey, endkey_len, indexkey);
	    endkey = indexkey;
	}
	if (mct->mct_index_comp != DM1CX_UNCOMPRESSED)
	{
	    status = dm1cxcomp_rec(&mct->mct_leaf_rac,
		    indexkey,
		    mct->mct_klen, cindexkey_buf, &indkey_len );
	    if (status != E_DB_OK)
	    {
		/*FIX_ME -- need to log something here */

		SETDBERR(dberr, 0, E_DM93E7_INCONSISTENT_ENTRY);
		break;
	    }
	    endkey = cindexkey_buf;
	    endkey_len = indkey_len;
	}

	/*
	** Fill in high range for the current leaf and set the nextpage
	** pointer to the next leaf.
	*/
	infinity = FALSE;
	status = dm1cxreplace(mct->mct_page_type, mct->mct_page_size, 
			mct->mct_curleaf, DM1C_DIRECT, mct->mct_index_comp,
			(DB_TRAN_ID *)NULL, (u_i2)0, (i4)0,
			DM1B_RRANGE, endkey, endkey_len, 
			(DM_TID *)&infinity, (i4)0);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E6_BAD_INDEX_REPLACE,
			(DMP_RCB *)mct->mct_oldrcb, mct->mct_curleaf,
			mct->mct_page_type, mct->mct_page_size, DM1B_RRANGE );
	    SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
	    break;
	}

        status = dm1xnextpage(mct, (i4 *)&page, dberr);
	if (status != E_DB_OK)
	    break;

	DM1B_VPT_SET_BT_NEXTPAGE_MACRO(mct->mct_page_type, mct->mct_curleaf, page);

	/*
	** Get and initialize new leaf that will hold the dup keys.
	*/

	status = dm1xnewpage(mct, 0, (DMPP_PAGE **)&mct->mct_curleaf, dberr);
	if (status)
	    break;
	status = dm1bxformat(mct->mct_page_type, mct->mct_page_size,
				(DMP_RCB *) 0, t,
				mct->mct_curleaf,
				mct->mct_kperleaf,
				mct->mct_klen, 
				mct->mct_leaf_rac.att_ptrs,
				mct->mct_leaf_rac.att_count,
				mct->mct_leaf_keys,
				mct->mct_keys,
				DM1B_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type,
							 mct->mct_curleaf),
				DM1B_PLEAF, mct->mct_index_comp, 
				mct->mct_acc_plv, 
				mct->mct_tidsize, 0, dberr);
	if (status)
	    break;

	/*
	** Get an associated data page for this leaf (unless secondary index)
	** and put the new record on it. If secondary index, get tid specified
	** in record for leaf's key,tid pair.
	*/
	if ( ! mct->mct_index)
	{
	    partno = 0;
	    
	    status = dm1xnewpage(mct, 0, &mct->mct_curdata, dberr);
	    if (status == E_DB_OK)
	    {
		DMPP_VPT_INIT_PAGE_STAT_MACRO(mct->mct_page_type, mct->mct_curdata,
		    DMPP_DATA | DMPP_ASSOC | DMPP_PRIM );
		DM1B_VPT_SET_BT_DATA_MACRO(mct->mct_page_type, mct->mct_curleaf,
		            DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, 
						     mct->mct_curdata));
		dm1xbput (mct, mct->mct_curdata, rec, rec_size,
			DM1C_LOAD_NORMAL, 0, &tid,
			mct->mct_ver_number, dberr);
	    }
	}
	else
	{
	    /*
	    ** This is a secondary index -- fetch base table TID from magic
	    ** last column of uncompressed index entry
	    */
	    if ( mct->mct_tidsize == sizeof(DM_TID8) )
	    {
		/* Extract TID and partition number from index entry */
		MEcopy(record + mct->mct_relwid - mct->mct_tidsize,
			mct->mct_tidsize, (char*)&tid8);
		tid.tid_i4 = tid8.tid_i4.tid;
		partno = tid8.tid.tid_partno;
	    }
	    else
	    {
		/* TID only */
		MEcopy(record + mct->mct_relwid - mct->mct_tidsize, 
		      mct->mct_tidsize, (char *)&tid);
		partno = 0;
	    }
	}

	if (status)
	    break;

	/*
	** Fill in the low range value for the new leaf.
	*/
        infinity = FALSE;
	status = dm1cxreplace(mct->mct_page_type, mct->mct_page_size,
			mct->mct_curleaf, DM1C_DIRECT, mct->mct_index_comp,
			(DB_TRAN_ID *)NULL, (u_i2)0, (i4)0,
			DM1B_LRANGE, endkey, endkey_len, 
			(DM_TID *)&infinity, (i4)0);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E6_BAD_INDEX_REPLACE,
			(DMP_RCB *)mct->mct_oldrcb, mct->mct_curleaf,
			mct->mct_page_type, mct->mct_page_size, DM1B_RRANGE );
	    SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
	    break;
	}

	/*
	** Move saved keys from temp page to current page and put new record
	** on new leaf.
	*/
	status = dm1cxrshift(mct->mct_page_type, mct->mct_page_size,
			    index, mct->mct_curleaf, mct->mct_index_comp,
			    (DM_LINE_IDX)0,
			    mct->mct_klen + mct->mct_tidsize);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E9_BAD_INDEX_RSHIFT,
			(DMP_RCB *)mct->mct_oldrcb, index,
			mct->mct_page_type, mct->mct_page_size, (DM_LINE_IDX)0);
	    dm1cxlog_error( E_DM93E9_BAD_INDEX_RSHIFT,
			(DMP_RCB *)mct->mct_oldrcb, mct->mct_curleaf, 
			mct->mct_page_type, mct->mct_page_size, (DM_LINE_IDX)0);
	    SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
	    break;
	}
	pinfo.page = mct->mct_curleaf;
	status = dm1bxinsert((DMP_RCB *) 0, t, 
		    mct->mct_page_type, mct->mct_page_size,
		    &mct->mct_leaf_rac,
		    mct->mct_klen, &pinfo,
		    key, &tid, partno,
		    (DM_LINE_IDX)DM1B_VPT_GET_BT_KIDS_MACRO(mct->mct_page_type,
		    mct->mct_curleaf),
		    (i4) FALSE, (i4) FALSE, dberr);
	if (status != E_DB_OK)
	    break;

	/* Mark this leaf as the current leaf page, and
	** note that we have moved dups off to their own leaf.
	** (If this leaf fills with dups we'll start a chain.)
	*/
	mct->mct_lastleaf = page; 
	mct->mct_dupstart = 0; 

	break;
    } 

    if (index)
	dm0m_pgdealloc((char **)&index);

    /* Discard any allocated key buffer */
    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &decrkey);

    if (status)
	log_error(E_DM925B_DM1B_BPUT, dberr);
    return (status);
}

/*{
** Name: add_to_overflow	- put this entry on an overflow page
**
** Description:
**	This is a duplicate key and the current leaf is full of duplicates.
**	Get an overflow page to put this key into.
**
** Inputs:
**      mct                             Pointer to modify context.
**	record				The record being added
**	rec				"record", compressed if necessary
**	rec_size			record size, possibly after compression
**	key				The key being added.
**
** Outputs:
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**      
**	Returns:
**
**	    E_DB_OK
**	    E_DB_INFO		- New overflow added, no parent update needed
**          E_DB_ERROR
**
** History:
**	19-aug-1991 (bryanp)
**	    Created.
**	10-sep-1991 (bryanp)
**	    B37642,B39322: Check for infinity == FALSE on LRANGE/RRANGE entries.
**	    When extracting the LRANGE or RRANGE entry from a page, we must
**	    first extract the tid portion of the entry and then only extract
**	    the key portion of the entry if (infinity == FALSE).
**	06-May-1996 (nanpr01 & thaju02)
**	    Changed the page header access as macro.
**	    Use mct->mct_acc_plv to access page level vector.
**	20-May-1996 (ramra01)
**	    Added new param tup_info to the get accessor routine
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**          Added DMPP_TUPLE_INFO argument to dm1cxget()
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          load accessor: changed DMPP_TUPLE_INFO param to table_version
**      28-jan-97 (stial01)
**          For sec. indexes, copy TIDP from (mct->mct_relwid - DM1B_TID_S)
**          instead of (mct->mct_keylen - DM1B_TID_S), to be consistent with
**          other code. For non-unique indexes, mct_relwid == mct_keylen
**          anyway.
**      12-jun-97 (stial01)
**          add_to_overflow() Pass tlv to dm1cxget instead of tcb.
**	28-Feb-2001 (thaju02)
**	    Pass mct->mct_ver_number to dmpp_load. (B104100)
**	11-Aug-2004 (schka24)
**	    mct_sibling used but never set, usage is bogus; fix.
*/
static DB_STATUS
add_to_overflow(
DM2U_M_CONTEXT	*mct,
char		*record,
char		*rec,
i4		rec_size,
char		*key,
DB_ERROR	*dberr)
{
    DB_STATUS	status = E_DB_OK;
    char	indexkey[DM1B_KEYLENGTH];
    char	cindexkey_buf[DM1B_KEYLENGTH];
    char	*endkey;
    i4	endkey_len;
    i4	indkey_len;
    DM_LINE_IDX	lastkid;
    char	*bounds[2];
    i4	bounds_len[2];
    char	bounds_buf[2][DM1B_KEYLENGTH];
    char	cbounds_buf[2][DM1B_KEYLENGTH];
    i4	save_infinity[2];
    i4	mainpageno;
    DM_TID	tid;
    DM_TID8	tid8;
    i4		partno;
    i4	infinity;
    DM_PAGENO	page;
    DMP_TCB     *t = mct->mct_oldrcb->rcb_tcb_ptr; 
    DMP_PINFO	pinfo;

    DMP_PINIT(&pinfo);

    CLRDBERR(dberr);
 
    for (;;)
    {
	if (DMZ_AM_MACRO(13))
	{
	    TRdisplay("dm1bbput: Duplicate on page full of duplicates.\n");
	}

	/*
	** If the current page is a leaf rather than an overflow page, then
	** set its high range and infinity values.  If this is an overflow
	** page, then those values were already copied down from the main
	** page.
	**
	** Also get page numbers for overflow page and next leaf page.
	** If the current leaf is not an overflow page, then we will reserve
	** the next leaf so we can point nextpage pointers on this overflow
	** chain to it.
	*/
	if (DMPP_VPT_GET_PAGE_STAT_MACRO(mct->mct_page_type, mct->mct_curleaf) & 
	    DMPP_LEAF)
	{
	    lastkid = DM1B_VPT_GET_BT_KIDS_MACRO(mct->mct_page_type, 
					     mct->mct_curleaf) - 1;
	    endkey = indexkey;
	    endkey_len = mct->mct_klen;
	    status = dm1cxget(mct->mct_page_type, mct->mct_page_size, 
			    mct->mct_curleaf, &mct->mct_leaf_rac,
			    lastkid, &endkey, (DM_TID *)NULL, (i4*)NULL,
			    &endkey_len, 
			    NULL, NULL, mct->mct_oldrcb->rcb_adf_cb);
	    if (status != E_DB_OK)
	    {
		dm1cxlog_error( E_DM93E3_BAD_INDEX_GET,
			    (DMP_RCB *)mct->mct_oldrcb,mct->mct_curleaf,
			    mct->mct_page_type, mct->mct_page_size, lastkid );
		SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
		break;
	    }
	    if (endkey != indexkey)
		MEcopy(endkey, endkey_len, indexkey);

	    if (mct->mct_index_comp != DM1CX_UNCOMPRESSED)
	    {
		status = dm1cxcomp_rec(&mct->mct_leaf_rac,
				indexkey, mct->mct_klen,
				cindexkey_buf, &indkey_len );
		if (status != E_DB_OK)
		{
		    /*FIX_ME -- need to log something here */

		    SETDBERR(dberr, 0, E_DM93E7_INCONSISTENT_ENTRY);
		    break;
		}
		endkey = cindexkey_buf;
		endkey_len = indkey_len;
	    }

	    /*
	    ** Fill in high range for the current leaf and set the nextpage
	    ** pointer to the next leaf.
	    */
	    infinity = FALSE;
	    status = dm1cxreplace(mct->mct_page_type, mct->mct_page_size,
			mct->mct_curleaf, DM1C_DIRECT, mct->mct_index_comp,
			(DB_TRAN_ID *)NULL, (u_i2)0, (i4)0,
			DM1B_RRANGE, endkey, endkey_len, 
			(DM_TID *)&infinity, (i4)0);
	    if (status != E_DB_OK)
	    {
		dm1cxlog_error( E_DM93E6_BAD_INDEX_REPLACE,
			(DMP_RCB *)mct->mct_oldrcb, mct->mct_curleaf, 
			mct->mct_page_type, mct->mct_page_size, DM1B_RRANGE );
		SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
		break;
	    }

	    mainpageno = DM1B_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, 
						  mct->mct_curleaf);
	}
	else
	{
	    mainpageno = DM1B_VPT_GET_PAGE_MAIN_MACRO(mct->mct_page_type, 
						  mct->mct_curleaf);	    
	}

	status = dm1xnextpage(mct, (i4 *)&page, dberr);
	if (status != E_DB_OK)
	    break;

	DM1B_VPT_SET_PAGE_OVFL_MACRO(mct->mct_page_type, mct->mct_curleaf, page); 

	/* We don't set the next-page link here, the entire chain will
	** be fixed up either when we quit overflowing (add_to_next_leaf)
	** or if building ends, at build finish time.
	*/

	/*
	** Save information from current page that we will need to write to
	** overflow page.
	*/
	endkey = bounds[0] = bounds_buf[0];
	bounds_len[0] = mct->mct_klen;
	/*
	** If the LRANGE key is "infinity", then there is no key value; for a
	** compressed index Btree, set the key length to 0 (for a non-compressed
	** index Btree, a garbage key will be stored). If the LRANGE key is
	** NOT "infinity", then extract the value and save it.
	*/
	dm1cxtget(mct->mct_page_type, mct->mct_page_size, mct->mct_curleaf, 
			DM1B_LRANGE, (DM_TID *)&save_infinity[0], (i4*)NULL ); 
	if (save_infinity[0] == FALSE)
	{
	    status = dm1cxget(mct->mct_page_type, mct->mct_page_size, 
			    mct->mct_curleaf, &mct->mct_leaf_rac,
			    DM1B_LRANGE, &endkey, (DM_TID *)NULL, (i4*)NULL,
			    &bounds_len[0],
			    NULL, NULL, mct->mct_oldrcb->rcb_adf_cb);
	    if (status != E_DB_OK)
	    {
		dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, 
			(DMP_RCB *)mct->mct_oldrcb, mct->mct_curleaf, 
			mct->mct_page_type, mct->mct_page_size, DM1B_LRANGE);
		SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
		return (status);
	    }
	    if (endkey != bounds[0])
		MEcopy(endkey, bounds_len[0], bounds[0]);
	}
	else if (mct->mct_index_comp != DM1CX_UNCOMPRESSED)
	{
	    bounds_len[0] = 0;
	}

	endkey = bounds[1] = bounds_buf[1];
	bounds_len[1] = mct->mct_klen;
	/*
	** If the RRANGE key is "infinity", then there is no key value; for a
	** compressed index Btree, set the key length to 0 (for a non-compressed
	** index Btree, a garbage key will be stored). If the RRANGE key is
	** NOT "infinity", then extract the value and save it.
	*/
	dm1cxtget(mct->mct_page_type, mct->mct_page_size, mct->mct_curleaf, 
			DM1B_RRANGE, (DM_TID *)&save_infinity[1], (i4*)NULL );
	if (save_infinity[1] == FALSE)
	{
	    status = dm1cxget(mct->mct_page_type, mct->mct_page_size, 
			    mct->mct_curleaf, &mct->mct_leaf_rac,
			    DM1B_RRANGE, &endkey, (DM_TID *)NULL, (i4*)NULL,
			    &bounds_len[1],
			    NULL, NULL, mct->mct_oldrcb->rcb_adf_cb);
	    if (status != E_DB_OK)
	    {
		dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, 
			(DMP_RCB *)mct->mct_oldrcb, mct->mct_curleaf, 
			mct->mct_page_type, mct->mct_page_size, DM1B_RRANGE);
		SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
		return (status);
	    }
	    if (endkey != bounds[1])
		MEcopy(endkey, bounds_len[1], bounds[1]);
	}
	else if (mct->mct_index_comp != DM1CX_UNCOMPRESSED)
	{
	    bounds_len[1] = 0;
	}

	if (mct->mct_index_comp != DM1CX_UNCOMPRESSED)
	{
	    /*
	    ** Compress the bounds keys and reset the lengths and pointers to
	    ** point to the compressed versions. Note that we don't do this
	    ** for the "infinity" keys, since these have no associated value.
	    */
	    if (save_infinity[0] == FALSE)
	    {
		status = dm1cxcomp_rec(&mct->mct_leaf_rac,
				bounds_buf[0],
				bounds_len[0], cbounds_buf[0], &endkey_len );
		if (status != E_DB_OK)
		{
		    dm1cxlog_error( E_DM93E1_BAD_INDEX_COMP,
					(DMP_RCB *)mct->mct_oldrcb,
					(DMPP_PAGE *)NULL, 0, 0, 0 );
		    SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
		    break;
		}
		bounds[0] = cbounds_buf[0];
		bounds_len[0] = endkey_len;
	    }

	    if (save_infinity[1] == FALSE)
	    {
		status = dm1cxcomp_rec(&mct->mct_leaf_rac,
				bounds_buf[1],
				bounds_len[1], cbounds_buf[1], &endkey_len );
		if (status != E_DB_OK)
		{
		    dm1cxlog_error( E_DM93E1_BAD_INDEX_COMP,
					(DMP_RCB *)mct->mct_oldrcb,
					(DMPP_PAGE *)NULL, 0, 0, 0 );
		    SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
		    break;
		}
		bounds[1] = cbounds_buf[1];
		bounds_len[1] = endkey_len;
	    }
	}

	/*
	** Allocate and format overflow page.
	*/
	status = dm1xnewpage(mct, 0, (DMPP_PAGE **)&mct->mct_curleaf, dberr);
	if (status != E_DB_OK)
	    break;

	status = dm1bxformat(mct->mct_page_type, mct->mct_page_size,
				(DMP_RCB *) 0, t,
				mct->mct_curleaf,
				mct->mct_kperleaf,
				mct->mct_klen, 
				mct->mct_leaf_rac.att_ptrs,
				mct->mct_leaf_rac.att_count,
				mct->mct_leaf_keys,
				mct->mct_keys,
				DM1B_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type,
							 mct->mct_curleaf),
				DM1B_POVFLO, mct->mct_index_comp, 
				mct->mct_acc_plv, 
				mct->mct_tidsize, 0, dberr);
	if (status)
	    break;

	/*
	** Get an associated data page for this leaf (unless secondary index)
	** and put the new record on it.
	** If secondary index, get tid specified in record for leaf's key,tid
	** pair.
	*/
	if ( ! mct->mct_index)
	{
	    partno = 0;

	    status = dm1xnewpage(mct, 0, &mct->mct_curdata, dberr);
	    if (status == E_DB_OK)
	    {
		DMPP_VPT_INIT_PAGE_STAT_MACRO(mct->mct_page_type, mct->mct_curdata,
		    DMPP_DATA | DMPP_ASSOC | DMPP_PRIM );
		DM1B_VPT_SET_BT_DATA_MACRO(mct->mct_page_type, mct->mct_curleaf, 
		                   DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type,
							    mct->mct_curdata));
		dm1xbput (mct, mct->mct_curdata, rec, rec_size,
			DM1C_LOAD_NORMAL, 0, &tid,
			mct->mct_ver_number, dberr);
	    }
	}
	else
	{
	    /* This is a secondary index */
	    if ( mct->mct_tidsize == sizeof(DM_TID8) )
	    {
		/* Extract TID and partition number from index entry */
		MEcopy(record + mct->mct_relwid - mct->mct_tidsize,
			mct->mct_tidsize, (char*)&tid8);
		tid.tid_i4 = tid8.tid_i4.tid;
		partno = tid8.tid.tid_partno;
	    }
	    else
	    {
		/* TID only */
		MEcopy(record + mct->mct_relwid - mct->mct_tidsize, 
		      mct->mct_tidsize, (char *)&tid);
		partno = 0;
	    }
	}
	if (status)
	    break;

	/*
	** Update the new overflow page with saved values from previous.
	*/
	DM1B_VPT_SET_PAGE_MAIN_MACRO(mct->mct_page_type, mct->mct_curleaf,
				 mainpageno);

	status = dm1cxreplace(mct->mct_page_type, mct->mct_page_size, 
			    mct->mct_curleaf, DM1C_DIRECT, mct->mct_index_comp,
			    (DB_TRAN_ID *)NULL, (u_i2)0, (i4)0,
			    DM1B_LRANGE, bounds[0], bounds_len[0],
			    (DM_TID *)(&save_infinity[0]), (i4)0);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E6_BAD_INDEX_REPLACE,
			(DMP_RCB *)mct->mct_oldrcb, mct->mct_curleaf, 
			mct->mct_page_type, mct->mct_page_size, DM1B_LRANGE );
	    SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
	    break;
	}
	status = dm1cxreplace(mct->mct_page_type, mct->mct_page_size,
			    mct->mct_curleaf, DM1C_DIRECT, mct->mct_index_comp,
			    (DB_TRAN_ID *)NULL, (u_i2)0, (i4)0,
			    DM1B_RRANGE, bounds[1], bounds_len[1],
			    (DM_TID *)(&save_infinity[1]), (i4)0);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E6_BAD_INDEX_REPLACE,
			(DMP_RCB *)mct->mct_oldrcb, mct->mct_curleaf, 
			mct->mct_page_type, mct->mct_page_size, DM1B_RRANGE );
	    SETDBERR(dberr, 0, E_DM93C0_BBPUT_ERROR);
	    break;
	}

	/*
	** Insert new record on overflow page.
	*/
	pinfo.page = mct->mct_curleaf;
	status = dm1bxinsert((DMP_RCB *) 0, t, 
		    mct->mct_page_type, mct->mct_page_size,
		    &mct->mct_leaf_rac,
		    mct->mct_klen, &pinfo,
		    key, &tid, partno, 0, 
		    (i4) FALSE, (i4) FALSE, dberr); 
	if (status != E_DB_OK)
	    break;

	/*
	** Since we have added a new overflow page instead of a new leaf, there
	** is nothing to insert in the parent index - so we can just return.
	*/
	mct->mct_dupstart = 0; 
	return (E_DB_INFO);
    }

    if (status)
	log_error(E_DM925B_DM1B_BPUT, dberr);
    return (status);
}

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
