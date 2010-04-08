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

#include    <adf.h>
#include    <ulf.h>

#include    <dmf.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dmucb.h>

#include    <dmp.h>
#include    <dmpl.h>
#include    <dmpp.h>
#include    <dmse.h>
#include    <dm0m.h>
#include    <dm0p.h>
#include    <dm1b.h>
#include    <dm1c.h>
#include    <dm1i.h>
#include    <dm1p.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dm1x.h>
#include    <dmpbuild.h>

/**
**
**  Name: DM1IBUILD.C - Routines to build a ISAM table.
**
**  Description:
**      This file contains all the routines needed to build
**      a ISAM table. 
**
**      The routines defined in this file are:
**      dm1ibbegin          - Initializes ISAM file for building.
**      dm1ibend            - Finishes building ISAM table.
**      dm1ibput            - Adds a record to a new ISAM table
**                            and builds index.
**
**  History:
**      07-feb-86 (jennifer)
**          Created for Jupitor.
**	29-may-89 (rogerk)
**	    Check status from dm1c_comp_rec and dm1c_uncomp_rec calls.
**	14-jun-1991 (Derek)
**	    Changed to use the common build buffering routines that have
**	    knowledge of the new allocation structures.  The algorithm used
**	    to build ISAM files has benn changed.  The changes allow the
**	    table to be built without requiring a second file for overflow
**	    pages.  A range of main pages, enough to map all keys on an
**	    index page are reserved, and overflow pages for each group of
**	    main pages are allocated immediately afterwards.  Thus a file
**	    looks like a range of main pages a set of overflows followed
**	    by a range of main page, and on and on until the index is written.
**	    The side effect of this is on the last range of mainpages that
**	    at least one overflow pages could leave a set of these mainpages
**	    that are not used, but are put into the free list.
**	08-jul-1992 (mikem)
**	    DMF prototypes.
**          Reorganized logic from for (;;) loops to a do/while to get around
**          sun4/acc compiler error "end-of-loop code not reached".
**	08-Jun-1992 (kwatts)
**	    6.5 MPF project. Replaced dm1c_add, dmpp_get_offset_macros,
**	    and dm1c_get calls with accessor calls. 
**    	29-August-1992 (rmuth)
**          Add call to dm1x_build_SMS to add the FHDR/FMAP(s).
**	14-oct-1992 (jnash)
**	    Reduced logging project.
**	      - Remove unused param's on dmpp_get calls.
**	      - Move compression out of dm1c layer, call tlv's here.
**	26-oct-92 (rickh)
**	    Replaced references to DMP_ATTS with DB_ATTS.  Also replaced
**	    DM_CMP_LIST with DB_CMP_LIST.
**	30-October-1992 (rmuth)
**	    Change for new file extend DI paradigm.
**	08-feb-1993 (rmuth)
**	    On overflow pages set DMPP_OVFL instead of DMPP_CHAIN which is
**	    used for overflow leaf pages.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system typedefs
**	21-June-1993 (rmuth)
**	    ISAM overflow chains were built the highest page number at
**	    the head of the list not allowing us to exploit readahead. 
**	    This has been reversed, hence a chain of the form
**	    main_page.page_ovfl->289->288->287->nil, will now be
**	    main_page.page_ovfl->287->288->289->nil.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	28-mar-1994 (mikem)
**	    bug #59831
**	    Fixed dm1bput() to check for errors from dm1xnewpage().  Previous
**	    to this change if you got a hard error (rather than a warning, ie.
**	    out-of-disk space) from dm1xnewpage() you would AV in dm1ibput(),
**	    due to a reference through a nil mct->mct_curdata pointer.
**	30-nov-1994 (shero03)
**	    Use the tuple accessors from the mct.
**	    This way, the routines are for the table we are building
**	    not the table we are coming from.
**	20-may-95 (hanch04)
**	    Added #include<dmtcb.h>
**	06-oct-1995 (nick)
**	    Format any empty data pages as DMPP_FREE when we've finished 
**	    loading / just about to start the index build. #71872
**	06-mar-1996 (stial01 for bryanp)
**	    Pass mct_page_size as the page_size argument to dmpp_format.
**	    Pass mct_page_size as page_size argument to dmpp_getfree.
**	06-mar-1996 (stial01 for bryanp)
**	    Don't allocate tuple buffers on the stack.
**      06-mar-1996 (stial01)
**          Limit mct_kperpage to (DM_TIDEOF + 1)
**          dm1ibend(): use mct->mct_page_size instead of relpgsize of
**          primary relation.
**	06-may-1996 (thaju02 & nanpr01)
**	    New Page Format Support:
**		Change page header references to use macros.
**	    Also change the page accessor vectors.
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**	13-sep-1996 (canor01)
**	    Add NULL buffer to dmpp_uncompress call.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          load accessor: changed DMPP_TUPLE_INFO param to table_version
**      10-mar-97 (stial01)
**          dm1ibput, dm1ibend: Use mct_crecord to compress a record
**	01-dec-1998 (nanpr01)
**	    Get rid of DMPP_TUPLE_INFO structure. Use DMPP_TUPLE_HDR instead.
**      09-feb-1999 (stial01)
**          dm1ibbegin() No more tuple headers on ISAM index entries.
**      10-feb-99 (stial01)
**          dm1ibbegin() Do not limit kperpage for ISAM INDEX pages, any pgsize
**      21-oct-99 (nanpr01)
**          More CPU optimization. Do not copy tuple header when we do not
**	    need to.
**	23-Jun-2000 (jenjo02)
**	    dm1xnewpage() returns with page zero-filled and formatted
**	    by dmpp_format(), therefore we needn't call dmpp_format()
**	    -again- just to set a new page status!
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Arg changes for page accessors (Variable Page Type SIR 102955)
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	21-Feb-2008 (kschendel) SIR 122739
**	    New row-accessor scheme, reflect here.
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1x?, dm1?b? functions converted to DB_ERROR *
**      24-nov-2009 (stial01)
**          Check if table has segmented rows (instead of if pagetype supports)
*/

static VOID	    logErrorFcn(
				i4 		status, 
				DB_ERROR 	*dberr,
				PTR		FileName,
				i4		LineNumber);
#define	log_error(status,dberr) \
	logErrorFcn(status,dberr,__FILE__,__LINE__)


/*{
** Name: dm1ibbegin - Initializes ISAM file for modify.
**
** Description:
**    This routine initializes the ISAM file for building.
**    The first overflow segment is reserved and the first page is allocated
**     here.
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
**	07-feb-86 (jennifer)
**          Created for Jupiter.
**	08-jul-1992 (mikem)
**          Reorganized logic from for (;;) loops to a do/while to get around
**          sun4/acc compiler error "end-of-loop code not reached".
**	06-mar-1996 (stial01 for bryanp)
**	    Pass mct_page_size as page_size argument to dmpp_getfree.
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
**	18-aug-1997 (nanpr01)
**	    Index corruption because of wrong kperpage calculation - b84518.
**      09-feb-1999 (stial01)
**          dm1ibbegin() No more tuple headers on ISAM index entries.
**      10-feb-99 (stial01)
**          dm1ibbegin() Do not limit kperpage for ISAM INDEX pages, any pgsize
*/
DB_STATUS
dm1ibbegin(
DM2U_M_CONTEXT	*mct,
DB_ERROR        *dberr)
{
    STATUS          status;
    i4		    num_alloc;
    DB_ERROR	    local_dberr;

    CLRDBERR(dberr);
    
    /*	Allocate build memory. */

    status = dm1xstart(mct, DM1X_OVFL, dberr);
    if (status != E_DB_OK)
	return (status);

    do /* loop executed only once, just to break out of on errors ... */
    {
	/*
	**  Reserve space for the first set of pages that will be mapped by
	**  the first index page.
	**  We won't limit kperpage to (DM_TIDEOF + 1) for any pagesize
	*/

	mct->mct_kperpage = ((*mct->mct_acc_plv->dmpp_getfree)(
			(DMPP_PAGE *) NULL, mct->mct_page_size)) /
	    		(DM_VPT_SIZEOF_LINEID_MACRO(mct->mct_page_type) + 
			 mct->mct_klen);

	mct->mct_startmain = 0;
	status = dm1xreserve(mct, mct->mct_kperpage, dberr);
	if (status != E_DB_OK)
	    break;

	/*  Get the first page. */

	status = dm1xnewpage(mct, -1, &mct->mct_curdata, dberr);
	if (status != E_DB_OK)
	    break;

	/*  Initialize the ISAM specific parts of the MCT. */
	DMPP_VPT_SET_PAGE_STAT_MACRO(mct->mct_page_type,mct->mct_curdata,DMPP_DATA);
	DMPP_VPT_SET_PAGE_MAIN_MACRO(mct->mct_page_type, mct->mct_curdata, 1);
	mct->mct_db_fill = mct->mct_page_size -
			    (mct->mct_page_size * mct->mct_d_fill) / 100;

	if (status == E_DB_OK)
	    return (E_DB_OK);

    } while (FALSE);

    (VOID) dm1xfinish(mct, DM1X_CLEANUP, &local_dberr);
    log_error(E_DM9251_DM1I_BADD, dberr);
    return (status);
}

/*{
** Name: dm1ibend - Finishes building a ISAM file for modify.
**
** Description:
**    This routine finsihes building a ISAM for modify.
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
**	07-feb-86 (jennifer)
**          Created for Jupiter.
**	29-may-89 (rogerk)
**	    Check status from dm1c_uncomp_rec calls.
**	08-jul-1992 (mikem)
**          Reorganized logic from for (;;) loops to a do/while to get around
**          sun4/acc compiler error "end-of-loop code not reached".
**	08-Jun-1992 (kwatts)
**	    6.5 MPF project. Replaced dm1c_add, dmpp_get_offset_macros,
**	    and dm1c_get calls with accessor calls. 
**    	29-August-1992 (rmuth)
**          Add call to dm1x_build_SMS to add the FHDR/FMAP(s).
**	14-oct-1992 (jnash)
**	    Reduced logging project.
**	      - Remove unused param's on dmpp_get calls.  
**	      - Move compression out of dm1c layer, call tlv's here.
**		dmpp_get now always returns a pointer to the buffer, never 
**		filling it in.
**	30-October-1992 (rmuth)
**	    Change for new DI file extend paradigm, just call dm1xfinish
**	    as it will build FHDR/FMAP and guarantee_space.
**	06-oct-1995 (nick)
**	    Whilst we ensured any pages unused in the last main data page
**	    allocation were formatted, they were formatted as DMPP_DATA.  This
**	    causes verifydb to barf as a) they are marked as free in the FMAP
**	    and b) they are orphaned.  Format as DMPP_FREE instead.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass mct_page_size as the page_size argument to dmpp_format.
**	06-mar-1996 (stial01 for bryanp)
**	    Don't allocate tuple buffers on the stack.
**	06-may-1996 (thaju02 & nanpr01)
**	    New Page Format Support:
**		Change page header references to use macros.
**	    Fix typo error.
** 	20-may-1996 (ramra01)
**	    Added argument DMPP_TUPLE_INFO to get load accessor routines
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**      18-jul-1996 (ramra01 for bryanp)
**          When re-reading rows from data pages to build index, pass 0 as the
**          row version number to dmpp_uncompress. The rows have just been
**          loaded, so they are guaranteed to be in version 0 format.
**          Pass 0 as the current table version to dmpp_load.
**	13-sep-1996 (canor01)
**	    Add NULL buffer to dmpp_uncompress call.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          load accessor: changed DMPP_TUPLE_INFO param to table_version
**      22-nov-1996 (nanpr01)
**          init the version with the current table version to dmpp_load.
**      10-mar-1997 (stial01)
**          dm1ibend: Use mct_crecord to compress a record
**	18-aug-1997 (nanpr01)
**	    b80775 - Higher level index corruption. The problem is that
**	    dm1xreadpage is returning a pointer to mct_ovfl pages if the
**	    page is in private cache. Then it is finding the low key on 
**	    the page and try to load it. If load  returns an out of space
**	    condition, it gets new page in the mct_ovfl, which might cause
**	    the key to be lost. So we just allocate a key area in this routine
**	    and copy the key into this buffer for load. mainsol code fix
**	    for this also will work, but we thought this will be a more
**	    general solution.
**	19-apr-2004 (gupsh01)
**	    Pass in adf control block in the call to dmpp_uncompress.
**	13-Feb-2008 (kschendel) SIR 122739
**	    Revise uncompress call, remove record-type and tid.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: parameter changes to page accessors
*/
dm1ibend(
DM2U_M_CONTEXT	*mct,
DB_ERROR        *dberr)
{
    DB_STATUS	    dbstatus;
    STATUS	    status;
    DM_TID	    tid;
    DB_ATTS	    *att;
    DMPP_PAGE	    *ip, *dp, *op;
    i4	    level, start, stop, newstop;
    i4	    pageno, mainpageno, next_page;
    i4	    offset, free;
    i4	    record_size;
    i4	    uncompressed_length;
    i4	    start_free, end_free;
    i4		    k;
    char	    *key;
    char	    *rec;
    char	    *record;
    char	    keytuple[DB_MAXTUP/2];
    DMPP_SEG_HDR seg_hdr;
    DMPP_SEG_HDR *seg_hdr_ptr;
    i4		    local_err;
    DB_ERROR	    local_dberr;

    CLRDBERR(dberr);

    if (mct->mct_seg_rows)
	seg_hdr_ptr = &seg_hdr;
    else
	seg_hdr_ptr = NULL;
 
    record = mct->mct_crecord;

    do  /* loop is executed once, just there to break on errors */
    {
	/*
	** Mark current data page as end of main pages and update overflow
	** page for this chain.
	*/

	DMPP_VPT_SET_PAGE_MAIN_MACRO(mct->mct_page_type, mct->mct_curdata, 0);
	start_free = DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, 
	    mct->mct_curdata) + 1;
	end_free = mct->mct_startmain + mct->mct_kperpage - 1;
	status = E_DB_OK;
	for (next_page = 
	        DMPP_VPT_GET_PAGE_OVFL_MACRO(mct->mct_page_type, mct->mct_curdata);
	    next_page;
	    next_page = 
		DMPP_VPT_GET_PAGE_OVFL_MACRO(mct->mct_page_type, mct->mct_curovfl))
	{
	    status = dm1xreadpage(mct, DM1X_FORUPDATE, next_page,
		&mct->mct_curovfl, dberr);
	    if (status != E_DB_OK)
		break;
	    DMPP_VPT_SET_PAGE_MAIN_MACRO(mct->mct_page_type, mct->mct_curovfl, 0);
	}
	if (status != E_DB_OK)
	    break;

	mct->mct_main = 
	    DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, mct->mct_curdata) + 1;

	if ((mct->mct_curovfl == 0 ||
	    DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, mct->mct_curovfl) < 
	        DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, mct->mct_curdata))
		&&
	    (mct->mct_curseg == 0 ||
	    DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, mct->mct_curseg) <
		DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, mct->mct_curdata)))
	{
	    /*
	    **	Truncate reserve window if no overflow pages have been
	    **	allocated.
	    */

	    end_free = start_free;
	    status = dm1xreserve(mct, 
		DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, 
		mct->mct_curdata) + 1, dberr);
	    if (status != E_DB_OK)
		break;
	}
	else
	{
	    /*
	    **  Allocate, initialize a write empty pages to end of segment so
	    **  that the page will be formatted if read, because the high
	    **  water mark  will be placed after the index.
	    */

	    do
	    {
		status = dm1xnewpage(mct, 
		    DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type,
		    mct->mct_curdata) + 1,
		    &mct->mct_curdata, dberr);
		/*
		** dm1xnewpage() returns with the page zero-filled
		** and formated with status DMPP_MODIFY | DMPP_DATA.
		** All we need to do is reset the status to
		** DMPP_FREE | DMPP_MODIFY.
		*/
		if (status == E_DB_OK)
		    DMPP_VPT_INIT_PAGE_STAT_MACRO(mct->mct_page_type,
					     mct->mct_curdata,
					     DMPP_FREE | DMPP_MODIFY);
	    } while (status == E_DB_OK);
	    if (status != E_DB_INFO)
		break;
	}

	/*
	** Build the ISAM index on top of the data pages.  Do this by reading
	** through the just-written data pages and placing one key entry for 
	** each data page into an index page (if index page gets full, go to 
	** a new one).
	**
	** After reading through all data pages, start over reading through the
	** just-written level of index pages, placing one key entry for each 
	** page into an index page on a new level.  Continue this until all  
	** of one level's keys fit into one index page.  That one index page 
	** is the ISAM root page.
	*/

	/*
	**  Read the primary pages by following the main page pointers.
	**	From the first key on every primary page construct an index key.
	**	This builds the first level of the index.
	*/

	status = dm1xnewpage(mct, 0, &mct->mct_curovfl, dberr);
	if (status != E_DB_OK)
	    break;

	/* Reformat as an INDEX page */

	ip = mct->mct_curovfl;
	/*
	** dm1xnewpage() returns with the page zero-filled
	** and formated with status DMPP_MODIFY | DMPP_DATA.
	** All we need to do is reset the status to
	** DMPP_DIRECT instead of calling dmpp_format again
	** to reformat the entire page.
	*/
	DMPP_VPT_INIT_PAGE_STAT_MACRO(mct->mct_page_type,
				  ip,
				  DMPP_DIRECT);

	DMPP_VPT_SET_PAGE_OVFL_MACRO(mct->mct_page_type, ip, 0);
	DMPP_VPT_SET_PAGE_MAIN_MACRO(mct->mct_page_type, ip, 0);
	level = 0;
	start = DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, mct->mct_curovfl);

	pageno = 0;
	do
	{
	    status = dm1xreadpage(mct, DM1X_FORREAD, pageno, &mct->mct_curdata,
		dberr);
	    if (status != E_DB_OK)
		break;

	    /*
	    ** If the relation is empty, then page 0 will
	    ** contain no records. Form a blank tuple
	    ** and use it to create a one tuple directory
	    */

	    dp = mct->mct_curdata;
	    key = keytuple;
	    if (DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, dp) == 0 && 
		DMPP_VPT_GET_PAGE_NEXT_LINE_MACRO(mct->mct_page_type, dp) == 0)
	    {
		MEfill(mct->mct_klen, 0, key);
	    }
	    else
	    {
		DM_TID	    tid;

		tid.tid_tid.tid_line = 0;
		record_size = mct->mct_relwid;
		status = (*mct->mct_acc_plv->dmpp_get)(mct->mct_page_type,
			mct->mct_page_size, dp, &tid, &record_size,
			&rec, NULL, NULL, NULL, seg_hdr_ptr);

		if (seg_hdr_ptr && seg_hdr_ptr->seg_next)
		{
		    if ( mct->mct_data_rac.compression_type != TCB_C_NONE )
		    {
			status = dm1x_get_segs(mct, seg_hdr_ptr, rec,
			    mct->mct_segbuf, dberr);
			rec = mct->mct_segbuf;
		    }
		    else
		    {
			status = dm1x_get_segs(mct, seg_hdr_ptr, rec, record, dberr);
			rec = record;
		    }
		}

		if (status == E_DB_OK)
		{
		    if (mct->mct_data_rac.compression_type != TCB_C_NONE)
		    {
		    /* Note that the following accessor comes from the MCT
		    ** and not the TCB.
		    ** and not the TCB.  In the case of a compression change
		    ** (eg data to hidata), the TCB rac is the old way,
		    ** while the MCT rac is the new way.
		    */
			status = (*mct->mct_data_rac.dmpp_uncompress)(
			    &mct->mct_data_rac,
			    rec, record, record_size, &uncompressed_length,
			    NULL, (i4)0 , (mct->mct_oldrcb)->rcb_adf_cb);
			if ( (status != E_DB_OK) || 
			   (record_size != uncompressed_length) )
			{
			    if (status != E_DB_OK)
			    {
				uncompressed_length = 0;
				record_size = 0;
			    }
			    uleFormat(NULL, E_DM942C_DMPP_ROW_UNCOMP, 
			     (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL, 
			     (i4)0, (i4 *)NULL, &local_err, 7, 
			     sizeof(DB_DB_NAME), &mct->mct_oldrcb->
			     rcb_tcb_ptr->tcb_dcb_ptr->dcb_name,
			     sizeof(DB_TAB_NAME), 
			     &mct->mct_oldrcb->rcb_tcb_ptr->tcb_rel.relid,
			     sizeof(DB_OWN_NAME), 
			     &mct->mct_oldrcb->rcb_tcb_ptr->tcb_rel.relowner,
			     0, tid.tid_tid.tid_page, 0, tid.tid_i4,
			     0, record_size, 0, uncompressed_length);

			    status = E_DB_ERROR;
			}
			rec = record;
		    }
		}

		if (status != E_DB_OK)
		{
		    SETDBERR(dberr, 0, E_DM9252_DM1I_BEND);
		    break;
		}

		/* Make key from record. */

		for (k = 0; k < mct->mct_keys; k++) 
		{
		    att = mct->mct_key_atts[k]; 
		    MEcopy(rec + att->offset, att->length,
			&key[att->key_offset]); 
		}
	    }

	    /*
	    ** Now put key on the current index page.  If there is no room,
	    ** go to next index page.
	    */
	    while (dm1xbput(mct, ip, key, mct->mct_klen,
		    DM1C_LOAD_ISAMINDEX, 0, 0,
		    mct->mct_ver_number, dberr) == E_DB_WARN)
	    {
		status = dm1xnewpage(mct, 0, &mct->mct_curovfl, dberr);
		if (status != E_DB_OK)
		    break;

		/* Reformat as an INDEX page */
		
		ip = mct->mct_curovfl;
		/*
		** dm1xnewpage() returns with the page zero-filled
		** and formated with status DMPP_MODIFY | DMPP_DATA.
		** All we need to do is reset the status to
		** DMPP_DIRECT instead of calling dmpp_format again
		** to reformat the entire page.
		*/
		DMPP_VPT_INIT_PAGE_STAT_MACRO(mct->mct_page_type,
					  ip,
					  DMPP_DIRECT);

		DMPP_VPT_SET_PAGE_OVFL_MACRO(mct->mct_page_type, ip, pageno);
		DMPP_VPT_SET_PAGE_MAIN_MACRO(mct->mct_page_type, ip, level);
	    }
	}   while (status == E_DB_OK &&
		(pageno = DMPP_VPT_GET_PAGE_MAIN_MACRO(mct->mct_page_type, 
		mct->mct_curdata)));
	if (status != E_DB_OK)
	    break;

	/*
	**  Build the second and successive levels of the index.
	*/

	for (level++, stop = DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, 
		mct->mct_curovfl);
	     start < stop;
	     stop = DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, 
		mct->mct_curovfl), level++)
	{
	    /* 
	    ** Format the first page of the overflow page buffer as an ISAM
	    ** directory page.  Level is indicated in page_main, and the page
	    ** containing the first key is indicated by page_ovfl.
	    */

	    status = dm1xnewpage(mct, 0, &mct->mct_curovfl, dberr);
	    if (status != E_DB_OK)
		break;
	    pageno = start;
	    start = DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, 
		mct->mct_curovfl);

	    /* Reformat as an INDEX page */

	    ip = mct->mct_curovfl;
	    /*
	    ** dm1xnewpage() returns with the page zero-filled
	    ** and formated with status DMPP_MODIFY | DMPP_DATA.
	    ** All we need to do is reset the status to
	    ** DMPP_DIRECT instead of calling dmpp_format again
	    ** to reformat the entire page.
	    */
	    DMPP_VPT_INIT_PAGE_STAT_MACRO(mct->mct_page_type,
				      ip,
				      DMPP_DIRECT);

	    DMPP_VPT_SET_PAGE_OVFL_MACRO(mct->mct_page_type, ip, pageno);
	    DMPP_VPT_SET_PAGE_MAIN_MACRO(mct->mct_page_type, ip, level);

	    for (; pageno <= stop; pageno++)
	    {
		status = dm1xreadpage(mct, DM1X_FORREAD, pageno,
		    &mct->mct_curdata, dberr);
		if (status != E_DB_OK)
		    break;

		/*
		** Now put key on the current index page.  If there is no room,
		** go to next index page.
		** Even if mct_seg_rows
		** There should not be segments on index pages
		*/

		tid.tid_tid.tid_page = 
		    DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type,
		    mct->mct_curdata);
		tid.tid_tid.tid_line = 0;
		(*mct->mct_acc_plv->dmpp_get)(mct->mct_page_type,
		    mct->mct_page_size, mct->mct_curdata, &tid,
		    &record_size, (char **)&key, 
		    NULL, NULL, NULL, NULL);

		/* copy the key from the page to the local buffer */
		MEcopy(key, mct->mct_klen, keytuple);
		while (dm1xbput(mct, ip, keytuple, mct->mct_klen,
			    DM1C_LOAD_ISAMINDEX, 0, 0,
			    (u_i2)0, dberr) == E_DB_WARN)
		{
		    status = dm1xnewpage(mct, 0, &mct->mct_curovfl, dberr);
		    if (status != E_DB_OK)
			break;

		    /* Reformat as an INDEX page */
		    ip = mct->mct_curovfl;
		    /*
		    ** dm1xnewpage() returns with the page zero-filled
		    ** and formated with status DMPP_MODIFY | DMPP_DATA.
		    ** All we need to do is reset the status to
		    ** DMPP_DIRECT instead of calling dmpp_format again
		    ** to reformat the entire page.
		    */
		    DMPP_VPT_INIT_PAGE_STAT_MACRO(mct->mct_page_type,
					      ip,
					      DMPP_DIRECT);

		    DMPP_VPT_SET_PAGE_OVFL_MACRO(mct->mct_page_type, ip, pageno);
		    DMPP_VPT_SET_PAGE_MAIN_MACRO(mct->mct_page_type, ip, level);
		}
		if (status != E_DB_OK)
		    break;
	    }
	    if (status != E_DB_OK)
		break;
	}
	if (status != E_DB_OK)
	    break;

	mct->mct_prim = DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, 
	    mct->mct_curovfl);

        /* 
	** Write build buffer to disk, deallocate build memory.
	** build the fhdr/fmap and guarantee the space
	*/
        status = dm1xfinish(mct, DM1X_COMPLETE, dberr);
        if (status != E_DB_OK)
            break;


	if (start_free < end_free)
	{
	    status = dm1xfree(mct, start_free,
		mct->mct_startmain + mct->mct_kperpage - 1, dberr);
	    if (status != E_DB_OK)
		break;
	}

    } while (FALSE);

    if (status == E_DB_OK)
	return(status);

    /*	Deallocate build memory and return error. */

    (VOID) dm1xfinish(mct, DM1X_CLEANUP, &local_dberr);
    log_error(E_DM9252_DM1I_BEND, dberr);
    return (status);
}

/*{
** Name: dm1ibput - Adds a record to ISAM file. 
**
** Description:
**    This routine builds a ISAM table.  Called by modify. 
**    The records are assumed to be in sorted order by key.
**    There are two phases to building an ISAM file.  First
**    it builds only the main pages.  Duplicates that exceed the 
**    main page space are written to pages of an overflow file.
**    these overflow pages point back to the main pages they 
**    need to be linked after the entire data file with index 
**    has been built.
**    After all main pages have been processed, the ISAM index
**    is built.  When this is finished, the overflow pages are
**    read from the overflow file, written to the end of the
**    main ISAM file and linked to the appropriate main page.
**
**    This routine also assumes that the record is not compressed.
**    It also assumes that the DUP flag will be set if this a 
**    duplicate key. The record is added to the page if there is 
**    room otherwise a new page is added(based on criteria above).
**    Currently the TCB of the table modifying is used for
**    attribute information needed by the low level routines.
**    If attributes are allowed to be modified, then these
**    build routines will not work. 
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
**	07-feb-86 (jennifer)
**          Created for Jupiter.
**	29-may-89 (rogerk)
**	    Check status from dm1c_comp_rec calls.
**	08-Jun-1992 (kwatts)
**	    6.5 MPF project. Replaced dm1c_add with dmpp_load calls and a
**	    dm1c_comp_rec with a call to the dmpp_compress accessor.
**	08-feb-1993 (rmuth)
**	    On overflow pages set DMPP_OVFL instead of DMPP_CHAIN which is
**	    used for overflow leaf pages.
**	21-June-1993 (rmuth)
**	    ISAM overflow chains were built the highest page number at
**	    the head of the list not allowing us to exploit readahead. 
**	    This has been reversed, hence a chain of the form
**	    main_page.page_ovfl->289->288->287->nil, will now be
**	    main_page.page_ovfl->287->288->289->nil.
**	28-mar-1994 (mikem)
**	    bug #59831
**	    Fixed dm1bput() to check for errors from dm1xnewpage().  Previous
**	    to this change if you got a hard error (rather than a warning, ie.
**	    out-of-disk space) from dm1xnewpage() you would AV in dm1ibput(),
**	    due to a reference through a nil mct->mct_curdata pointer.
**	06-mar-1996 (stial01 for bryanp)
**	    Don't allocate tuple buffers on the stack.
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
**	20-may-1996 (ramra01)
**	    Added new param to the load accessor
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**      18-jul-1996 (ramra01 for bryanp)
**          Pass 0 as the current table version to dmpp_load.
**	09-oct-1996 (nanpr01)
**	    Donot return E_DB_OK always rather return status. Otherwise
**	    diskfull condition on modify of isam get segmentation fault.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          load accessor: changed DMPP_TUPLE_INFO param to table_version
**      10-mar-97 (stial01)
**          dm1ibput: Use mct_crecord to compress a record
**	28-Feb-2001 (thaju02)
**	    Pass mct->mct_ver_number to dmpp_load. (B104100)
*/
DB_STATUS
dm1ibput(
DM2U_M_CONTEXT  *mct,
char            *record,
i4         record_size,
i4         dup,
DB_ERROR        *dberr)
{
    DB_STATUS       status;
    char	    *rec = record;
    i4	    next_page;
    i4         rec_size;
    DB_ERROR	    local_dberr;

    CLRDBERR(dberr);

    /* 
    ** See if there is room on current page,
    ** there is always room for one record. 
    */

    rec_size = record_size;
    if (mct->mct_data_rac.compression_type != TCB_C_NONE)
    {
	rec = mct->mct_crecord;
	/* Note that the following accessor comes from the MCT and not the
	** TCB.  This is because we want the compression scheme of the table
	** we are building, not the one we are building from.
	*/
	status = (*mct->mct_data_rac.dmpp_compress)(&mct->mct_data_rac, 
		record, record_size, rec, &rec_size);
	if (status != E_DB_OK)
	{
	    SETDBERR(dberr, 0, E_DM938B_INCONSISTENT_ROW);
	    return (status);
	}
    }

    /*	Add record to current main page. */

    while ((status = dm1xbput(mct, mct->mct_curdata, rec, rec_size,
			DM1C_LOAD_NORMAL, mct->mct_db_fill, 0,
			mct->mct_ver_number, dberr))
	    == E_DB_WARN)
    {
	if (dup == 0)
	{
	    /*
	    **	Record is not a duplicate of the previous and the main
	    **  page is full, start a new main page.
	    */
	    status = dm1xnewpage(mct, 
			DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type,
			mct->mct_curdata) + 1,
			&mct->mct_curdata, dberr);
	    if (status == E_DB_INFO)
	    {
		/*  Current reserved area is full, allocate a new area. */

		status = dm1xnextpage(mct, &mct->mct_startmain, dberr);
		if (status != E_DB_OK)
		    break;

		/*  Update main page pointer for current chain. */

		DMPP_VPT_SET_PAGE_MAIN_MACRO(mct->mct_page_type, mct->mct_curdata, 
		    mct->mct_startmain);
		for (next_page = DMPP_VPT_GET_PAGE_OVFL_MACRO(mct->mct_page_type, 
			mct->mct_curdata);
		    next_page;
		    next_page = DMPP_VPT_GET_PAGE_OVFL_MACRO(mct->mct_page_type,
			mct->mct_curovfl))
		{
		    status = dm1xreadpage(mct, DM1X_FORUPDATE, next_page,
			&mct->mct_curovfl, dberr);
		    if (status != E_DB_OK)
			break;
		    DMPP_VPT_SET_PAGE_MAIN_MACRO(mct->mct_page_type, 
			mct->mct_curovfl, mct->mct_startmain);
		}

		/*  Get new main page. */

		if (status == E_DB_OK)
		    status = dm1xreserve(mct,
			mct->mct_startmain + mct->mct_kperpage, dberr);
		if (status == E_DB_OK)
		    status = dm1xnewpage(mct, mct->mct_startmain,
			&mct->mct_curdata, dberr);
		if (status != E_DB_OK)
		    break;
	    }
	    else if (status)
	    {
		break;
	    }

	    DMPP_VPT_SET_PAGE_MAIN_MACRO(mct->mct_page_type, mct->mct_curdata, 
		DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, 
		mct->mct_curdata) + 1);
	    continue;
	}

	/*
	**  Record is a duplicate, keep adding to the overflow page for
	**  this main page.
	*/
	status = E_DB_WARN;

	/*
	** If first overflow page then link it to the main page
	*/
	if ( DMPP_VPT_GET_PAGE_OVFL_MACRO(mct->mct_page_type, mct->mct_curdata) == 
	    0 )
	{
	    status = dm1xnewpage(mct, 0, &mct->mct_curovfl, dberr);
	    if ( status != E_DB_OK )
		break;
	    DMPP_VPT_SET_PAGE_MAIN_MACRO(mct->mct_page_type, mct->mct_curovfl,
		DMPP_VPT_GET_PAGE_MAIN_MACRO(mct->mct_page_type, mct->mct_curdata)); 
	    DMPP_VPT_SET_PAGE_OVFL_MACRO(mct->mct_page_type, mct->mct_curovfl,
		DMPP_VPT_GET_PAGE_OVFL_MACRO(mct->mct_page_type, mct->mct_curdata));
	    DMPP_VPT_SET_PAGE_STAT_MACRO(mct->mct_page_type, mct->mct_curovfl, 
		DMPP_DATA | DMPP_OVFL );
	    DMPP_VPT_SET_PAGE_OVFL_MACRO(mct->mct_page_type, mct->mct_curdata,
		DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, mct->mct_curovfl));
	}

	/*
	** If the current buffered overflow page is for this main data
	** page then add the data.
	*/
	if ( DMPP_VPT_GET_PAGE_MAIN_MACRO(mct->mct_page_type, mct->mct_curovfl) == 
	    DMPP_VPT_GET_PAGE_MAIN_MACRO(mct->mct_page_type, mct->mct_curdata))
	{
	    status = dm1xbput(mct, mct->mct_curovfl, rec, rec_size,
			DM1C_LOAD_NORMAL, 0, 0,
			mct->mct_ver_number, dberr);
	}

	/*
	** If current overflow page is full then allocate a new overflow
	** page, link it to the end of the overflow chain and add the
	** data to the page
	*/
	if (status == E_DB_WARN)
	{
	    /*
	    ** Find out next page number we will allocate so that
	    ** can fix up the overflow chain ptr on current overflow
	    ** page before we release it
	    */
	    status = dm1xnextpage( mct, &next_page, dberr );
	    if ( status != E_DB_OK )
		break;
	    
	    DMPP_VPT_SET_PAGE_OVFL_MACRO(mct->mct_page_type, mct->mct_curovfl, 
		next_page);

	    status = dm1xnewpage(mct, 0, &mct->mct_curovfl, dberr);
	    if ( status != E_DB_OK )
		break;
	    DMPP_VPT_SET_PAGE_MAIN_MACRO(mct->mct_page_type, mct->mct_curovfl, 
		DMPP_VPT_GET_PAGE_MAIN_MACRO(mct->mct_page_type, mct->mct_curdata));
	    DMPP_VPT_SET_PAGE_OVFL_MACRO(mct->mct_page_type, mct->mct_curovfl, 0); /* end of chain */
	    DMPP_VPT_SET_PAGE_STAT_MACRO(mct->mct_page_type, mct->mct_curovfl, 
		DMPP_DATA | DMPP_OVFL );
	    /*
	    ** Add data to new page
	    */
	    status = dm1xbput(mct, mct->mct_curovfl, rec, rec_size,
			DM1C_LOAD_NORMAL, 0, 0,
			mct->mct_ver_number, dberr);
	}

	break;

    }

    if (status != E_DB_OK)
	log_error(E_DM9253_DM1I_BOTTOM, dberr);
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
