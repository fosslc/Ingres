/*
**Copyright (c) 2004 Ingres Corporation
**
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cm.h>
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
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0m.h>
#include    <dm0p.h>
#include    <dm1c.h>
#include    <dm1p.h>
#include    <dm2umct.h>
#include    <dm1x.h>
#include    <dm2f.h>
#include    <dmpl.h>
#include    <dmftrace.h>
#include    <dmd.h>

/**
**
**  Name: DM1X.C - Simple buffer manager for build routines.
**
**  Description:
**      This file contains all the routines that perform simple buffer manager
**	like functions for the build routines.  These functions perform
**	multiple block read/write and simple caching.
**
**	The operation of these routines are heavily tied to the specific needs
**	of the build routines.
**
**      The routines defined in this file are:
**
**	    dm1xstart()
**	    dm1xfinish()
**	    dm1xreadpage()
**	    dm1xnewpage()
**	    dm1xreserve()
**	    dm1xnextpage()
**	    dm1x_build_SMS()	- Build the FHDR/FMAP(s) space management
**				  scheme.
**
**  Terminology:
**	Definition for terms used in this file follow:
**
**	Segment - A range of pages.  There can be up to two segments, the data
**	    segment and the overflow segment.
**
**	Group - A set of pages that maps a range of a segment.  There can be
**	    up to two groups, one for each segment.
**
**  Assumptions:
**	This code is tailored for use by thge build routines as written for
**	for the current set of access methods.  Any changes to the usage or
**	the addition of a new access method should note the following:
**
**	    dm1xreapage might need checks to see if a group being read in
**		contains a page in a save buffer.
**
**	    Save pages are only in the data group or the save buffers.
**
**	    Only pages in the reserved area can be allocated by page number.
**
**	    Only pages in the data segment can be reserved.
**
**	    The pointer used in a call is used to determine what exact
**		operation is desired.
**
**  Disk Builds versus Buffer Manager Builds:
**	These routines support two different types of builds. In a disk build,
**	the build routines build a new file on disk. As necessary, the build
**	routines call these routines to read/write pages; we then allocate disk
**	space in the file and read and write pages directly from that file. In
**	a buffer manager build (also known as an "in-memory" build), these
**	routines instead call the buffer manager to get scratch pages in the
**	buffer manager's cache. Buffer manager builds are used for in-memory
**	temporary tables.
**
**  History:
**      08-may-90 (Derek)
**          Created.
**      15-oct-1991 (mikem)
**          Eliminated unix compile warnings in dm1xfinish().
**	17-oct-1991 (rmuth)
**	    Add error checking in 'write_group' to make sure a valid
**	    extend size.
**      24-oct-91 (jnash)
**          Remove & in front of two ule parameers to make LINT happy.
**	24-oct-91 (jnash)
**	    Add a number of fn headers.
**	18-Dec-1991 (rmuth)
**	    Corrected bug in check_header where for a period of time
**	    an invalid FHDR would be resident on disc.
**	13-feb-1992 (bryanp)
**	    Set *err_code when encountering error in write_group.
**	6-mar-1992 (bryanp)
**	    Support build-into-the-buffer-manager.
**	16-apr-1992 (bryanp)
**	    Function prototypes.
**      09-jun-1992 (kwatts)
**          6.5 MPF Changes. Replaced call of dm1c_format with accessor.
**	15-jul-1992 (bryanp)
**          Flush allocations made to temporary tables, and set tbio_lalloc.
**	29-August-1992 (rmuth)
**	    - Changed to use dm1p_init_fhdr and dm1p_add_fmap.
**	    - Added I/O tracing.
**	    - Exposed trace points outside of xDEBUG.
**	    - In correcting a bug with building the FHDR/FMAP(s) changed
**	      the algorithm for building them. Used to build them as the
**	      table was being built, now just add them at the end.
**	    - Add some trace back error messages.
**	05-oct-1992 (rogerk)
**	    Reduced Logging Project.  In preparation for support of Partial
**	    TCB's for the new recovery schemes, changed references to
**	    some tcb fields to use the new tcb_table_io structure fields.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	09-oct-1992 (jnash)
**	    Reduced loggin project
**	     -  Add Page checksum production and checking.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Changed arguments to dm0p_mutex and
**	    dm0p_unmutex calls.
**	30-October-1992 (rmuth)
**	    - Add dm1x_guarantee_space
**	    - Make dm1xfinish call dm1x_build_SMS 
**	    - Use the new mct_guarantee_on_disc flag.
**	    - Set tbio_lpageno and tbio_lalloc correctly.
**	    - Add setting of the new tbio_tmp_hw_pageno field.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	    Removed DMPP_ACC_PLV argument from dm2f_write_file.
**	18-dec-92 (robf)
**	    Removed obsolete dm0a.h
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	21-jun-1993 (rogerk)
**	    Changed read_group routine to use mct_relpages rather than 
**	    mct_allocated to limit how far out to read ahead of the requested 
**	    page. Using mct_allocated caused us to attempt to read pages
**	    allocated but not yet forced.
**	21-June-1993 (rmuth)
**	    Various fixes:
**	    - b49885 : When building a non-temporary table write_group was 
**	      incorrectly setting the old tables tbio_lpageno and tbio_lalloc 
**	      to the new table values as the new table was being built. 
**	      This caused havoc if we are still reading the old table whilst
**	      building the table. This occurs if we bypass first loading the
**	      data into the sorter for example modifing a table to heap.
**	    - Building ISAM tables with overflow chains.
**	      a. Re-scanning these chains caused us to move the mct_ovfl
**	         window backwards. This uncovered bugs where the window
**	         variables 'start', where window begins, and 'finish'
**	         the end of the segment the window can wander across
**	         were being set incorrectly.
**	      b. The above problem caused us not to allocate a page and
**	         hence an AV occurred. Check for the above and return an
**	         error.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Fix a trace statement in dm1xnewpage() -- it was "unreachable".
**	26-july-1993 (rmuth)
**	    Use mct_override_alloc if specified.
**	18-oct-1993 (rmuth)
**	    write_group() : If initial allocation was less the required 
**	    space then we were incorrectly calculating how much extra
**	    space needed to be allocated.
**	17-jan-1994 (rmuth)
**	    - b58605 The display io_trace routines were displaying the incorrect
**	      file name.
**	    - If an inmemory table do not display the I/O trace.
**	18-apr-1994 (jnash)
**	    fsync project.  Eliminate calls to dm2f_force_file().
**	23-may-1994 (andys)
**	    Add cast(s) to quiet qac/lint
**	06-mar-1996 (stial01 for bryanp)
**	    Get the page size using mct_page_size, rather than via sizeof().
**	    Set SMS_page_size in the DM1P_BUILD_SMS_CB.
**	    Pass mct_page_size as the page_size argument to dmpp_format.
**	    mct_pages arrays are not simple arrays anymore, since the size of
**		a page is not known at compile time, so references to members
**		of an mct_pages array must be computed using pointer math.
**	    Use MEcopy, rather than STRUCT_ASSIGN_MACRO, to copy pages.
**      06-mar-1996 (stial01)
**          Pass page_size to dm2f_write_file(), dm2f_read_file()
**      21-may-1996 (thaju02)
**          Pass page size to NUMBER_FROM_MAP_MACRO for 64 bit tid support
**          and multiple fhdr pages.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      26-feb-1998 (stial01)
**          B88434: Add check to make sure table does not grow past 
**          DM1P_MAX_TABLE_SIZE.
**	23-Jun-2000 (jenjo02)
**	    Added fill_option to dmpp_format() prototype, set by caller
**	    to DM1C_ZERO_FILL if dmpp_format should MEfill the page,
**	    DM1C_ZERO_FILLED if the page has already been MEfilled.
**	    The page overhead space is always zero-filled.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**      21-may-2001 (stial01)
**          Changes to fix FHDR fhdr_map initialization (B104754)
**      08-Oct-2002 (hanch04)
**          Removed dm0m_lcopy,dm0m_lfill.  Use MEcopy, MEfill instead.
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      10-mar-2004 (stial01)
**          Make sure we allocate mct_ovsave if DMPP_VPT_PAGE_HAS_SEGMENTS
**          (note mct_structure is not init when called from modify, and
**          in any case mct_ovsave is needed for HASH and ISAM structures.
**      30-mar-2004 (stial01)
**          ifdef TRdisplay
**	12-Aug-2004 (schka24)
**	    Build data structure cleanup: unify oldrcb/buildrcb usage.
**      06-oct-2004 (stial01)
**          Fix num_alloc calculation when dm0m_allocate fails
**      31-jan-05 (stial01)
**          Added flags arg to dm0p_mutex call(s).
**      28-Jun-2005 (kodse01)
**          Corrected the typo in change 477561.
**	06-Sep-2005 (thaju02) B115227
**	    In dm1xbput(), for page-spanning row's first segment
**	    use the entire the page.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	01-Dec-2008 (jonj)
**	    SIR 120874: dm2f_? functions converted to DB_ERROR *
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1p_? functions converted to DB_ERROR *
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1x? functions converted to DB_ERROR *, use
**	    new form uleFormat.
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**      12-oct-2009 (stial01)
**          Use DMPP_PAGE for all page types.
**	21-Oct-2009 (kschendel) SIR 122739
**	    Record "type" in dm1xbput is isam-or-not flag, not compression.
**      24-nov-2009 (stial01)
**          Check if table has segmented rows (instead of if pagetype supports)
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace DMPP_PAGE* with DMP_PINFO* where needed.
*/

/*
**  Forward and/or External function references for statoic functions.
*/

static	    DB_STATUS	allocate(DM2U_M_CONTEXT *mct, 
				i4 pageno,
			        DMPP_PAGE **page, 
				DB_ERROR *dberr);

static      VOID	dm1xLogErrorFcn(i4 status, 
				DB_ERROR *dberr,
				PTR	 FileName,
				i4	 LineNumber);
#define dm1x_log_error(status,dberr) \
	dm1xLogErrorFcn(status,dberr,__FILE__,__LINE__)

static	    DB_STATUS	read_one(DM2U_M_CONTEXT *mct, 
				i4 pageno,
				DMPP_PAGE **page, 
				DB_ERROR *dberr);

static	    DB_STATUS	read_group(DM2U_M_CONTEXT *mct, 
				i4 pageno,
				MCT_BUFFER *group, 
				DB_ERROR *dberr);

static	    DB_STATUS	write_one(DM2U_M_CONTEXT *mct, 
				DMPP_PAGE *page,
				DB_ERROR *dberr);

static	    DB_STATUS	write_group(DM2U_M_CONTEXT *mct,
				MCT_BUFFER *group, 
				DB_ERROR *dberr);

static 	    DB_STATUS 	dm1x_guarantee_space(
				DM2U_M_CONTEXT	    *mct,
				DB_ERROR	    *dberr);

static DB_STATUS 	dm1x_build_SMS(
				DM2U_M_CONTEXT	    *mct,
				DB_ERROR	    *dberr);

/*{
** Name: dm1xstart - Allocate and initialize build buffer.
**
** Description:
**	This routine will allocates enough memory to support the
**	build operation.  It can handle multiple output files.
**	The 'reserve' value can be used to reserve contiguous pages
**	(I.E. placement of the FHDR block can be controlled.)
**
**
** Inputs:
**      mct				Pointer to record access context.
**	flag				Options:
**					    DM1X_OVFL	- Use ovfl buffers.
**					    DM1X_SAVE	- Use save buffers.
**					    DM1X_KEEPDATA - Use existing FHDR.
** Outputs:
**      err_code                        Pointer to an area to return
**                                      error code.   May be set to one of the
**                                      following error codes:
**                                      
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**      08-may-90 (Derek)
**          Created.
**	29-August-1992 (rmuth)
**	    Removed FHDR/FMAP calculations as these are now added at the
**	    end of the build.
**	    Make sure we have allocated mct_buffer before we try and
**	    deallocate it.
**	30-October-1992 (rmuth)
**	   - If this is an existing table we use dm2f_sense_file to
**	     find the end of the table set tcb_lalloc to this value.
**	     Note if a temporary table we have to flush first as the
**	     the disc version may be stale and sense returns from disc.
**	15-jan-1999 (nanpr01)
**	    Pass ptr to ptr in dm0m_deallocate functions.
**	02-Apr-2004 (jenjo02)
**	    Allocate page-aligned DMPP_PAGEs for better I/O
**	    performance on certain file systems.
**	5-Nov-2009 (kschendel) SIR 122757
**	    Use direct-io alignment for I/O pages.
*/
DB_STATUS
dm1xstart(
DM2U_M_CONTEXT	    *m,
i4		    flag,
DB_ERROR	    *dberr)
{
    char	*p;
    DM_SVCB	*svcb = dmf_svcb;
    STATUS	status;
    i4		page;
    i4		num_alloc;
    i4		add_alloc = 0;
    i4		block_size = svcb->svcb_mwrite_blocks;
    SIZE_TYPE	size;
    u_i4	directio_align = svcb->svcb_directio_align;

    CLRDBERR(dberr);
    
    /*
    **	Compute number of buffers to allocate for the build.  If DM1X_OVFLBUF
    **	if set and another array of buffers.  If DM1X_SAVEBUF allocate 3
    **	extra buffers to save pages that are still active when an array of
    **	pages is flushed.
    */

    num_alloc = block_size;

    if (flag & DM1X_OVFL)
	num_alloc += num_alloc;
    if (flag & DM1X_SAVE)
	add_alloc += 3;
    if ( flag & DM1X_KEEPDATA )
	add_alloc += 1;

    if (m->mct_seg_rows)
    {
	/* need dtsave */
	if ((flag & DM1X_SAVE) == 0)
	    add_alloc += 1;

	/* Always allocate extra buffer for mct_ovsave */
	add_alloc += 1;
    }
    
    /*	Allocate aligned memory for the build buffers. */
    num_alloc += add_alloc;

    if (svcb->svcb_rawdb_count > 0 && directio_align < DI_RAWIO_ALIGN)
	directio_align = DI_RAWIO_ALIGN;
    do
    {
	size = num_alloc * m->mct_page_size + sizeof(DMP_MISC);
	if (directio_align != 0)
	    size += directio_align;
        status = dm0m_allocate(size,
    	                (i4)0, (i4)MISC_CB, 
			(i4)MISC_ASCII_ID, (char *)m,
    	                (DM_OBJECT **)(&m->mct_buffer), dberr);
	if (status == E_DB_ERROR)
	{
	    /* 
	    ** If we couldn't allocate that much memory, try a smaller amount.
	    */
	    block_size = 1;
	    num_alloc = block_size;
	    if (flag & DM1X_OVFL)
		num_alloc += block_size;
	    num_alloc += add_alloc;

	    size = num_alloc * m->mct_page_size + sizeof(DMP_MISC);
	    if (directio_align != 0)
		size += directio_align;
	    status = dm0m_allocate(size,
			(i4)0, (i4)MISC_CB, (i4)MISC_ASCII_ID,
			(char *)m, (DM_OBJECT **)(&m->mct_buffer), dberr);
	}

	if ( status != E_DB_OK )
	    break;

    	/*
	** Initialize various page pointers.
	*/
    	m->mct_dtsave = 0;
	m->mct_ovsave = 0;
    	m->mct_lfsave = 0;
    	m->mct_nxsave = 0;
    	m->mct_ovfl.mct_pages = 0;
    	m->mct_curdata = 0;
    	m->mct_curleaf = 0;
    	m->mct_curindex = 0;
    	m->mct_curovfl = 0;
    	m->mct_fhdr_pageno = 0;
    	m->mct_last_dpage = 0;
	m->mct_curseg = 0;

	p = (char *) m->mct_buffer;
	p += sizeof(DMP_MISC);
	if (directio_align > 0)
	    p = ME_ALIGN_MACRO(p, directio_align);
	((DMP_MISC *)(m->mct_buffer))->misc_data = p;
	if (flag & DM1X_KEEPDATA)
	{
	    m->mct_last_dpage = (DMPP_PAGE *) p;
	    p += m->mct_page_size;
	}
	if (flag & DM1X_SAVE)
	{
	    m->mct_dtsave = (DMPP_PAGE *) p;
	    p += m->mct_page_size;
	    m->mct_lfsave = (DMPP_PAGE *) p;
	    p += m->mct_page_size;
	    m->mct_nxsave = (DMPP_PAGE *) p;
	    p += m->mct_page_size;
	}
	if (m->mct_seg_rows)
	{
	    if (m->mct_dtsave == NULL)
	    {
		m->mct_dtsave = (DMPP_PAGE *) p;
		p += m->mct_page_size;
	    }
	    m->mct_ovsave = (DMPP_PAGE *) p;
	    p += m->mct_page_size;
	}
	m->mct_data.mct_pages = (DMPP_PAGE *)p;
	p += block_size * m->mct_page_size;
	if (flag & DM1X_OVFL)
	    m->mct_ovfl.mct_pages = (DMPP_PAGE *) p;

    	m->mct_mwrite_blocks = block_size;
    	m->mct_data.mct_start = 0;
    	m->mct_data.mct_last = -1;
    	m->mct_data.mct_finish = MAXI4;
    	m->mct_ovfl.mct_start = 0;
    	m->mct_ovfl.mct_last = -1;
    	m->mct_ovfl.mct_finish = MAXI4;
    	m->mct_relpages = 0;
    	m->mct_main = 1;
    	m->mct_prim = 1;
    	m->mct_oldpages = 0;

    	if (flag & DM1X_KEEPDATA)
    	{
	    m->mct_relpages = m->mct_lastused + 1;
	    m->mct_data.mct_start = m->mct_relpages;
	    m->mct_data.mct_last = m->mct_data.mct_start - 1;

	    /* 
	    ** Read the last data page so that we can update its 
	    ** forward pointer 
	    */
	    status = read_one( m, m->mct_last_data_pageno, 
			       &m->mct_last_dpage, dberr );
	    if ( status != E_DB_OK )
		break;
	}

	/* 
        ** If this is an existing table, see how many pages currently exist
	** If it's a recreate load, ie we are constructing a brand new
	** file, don't do this as there's nothing there yet.
       	*/
       	if (! m->mct_recreate)
        {
	    DMP_LOCATION *loc;
	    DMP_TCB *t = m->mct_buildrcb->rcb_tcb_ptr;
	    i4 loc_count;

	    if (m->mct_inmemory)
	    {
		loc = t->tcb_table_io.tbio_location_array;
		loc_count = t->tcb_table_io.tbio_loc_count;
	    }
	    else
	    {
		loc = m->mct_location;
		loc_count = m->mct_loc_count;
		/*
		** For temporary tables we need to make sure that the disc
		** eof is up to date as sense file returns this information
		** from disc.
		*/
	        if (t->tcb_temporary == TCB_TEMPORARY)
	    	{
		    status = dm2f_flush_file(
			loc, loc_count,
			&t->tcb_rel.relid,
			&t->tcb_dcb_ptr->dcb_name,
			dberr );
		    if ( status != E_DB_OK )
			break;
	        }
	    }
	    status = dm2f_sense_file(loc, loc_count,
			&t->tcb_rel.relid,
			&t->tcb_dcb_ptr->dcb_name,
			&page, dberr);
	    if ( status != E_DB_OK )
		break;

	    t->tcb_table_io.tbio_lalloc = page;

	    m->mct_oldpages = page + 1;
	}

    } while ( FALSE );

    if (status != E_DB_OK)
    {
	dm1x_log_error(E_DM92E0_DM1X_START, dberr);
	if ( m->mct_buffer )
	{
	    dm0m_deallocate((DM_OBJECT **)&m->mct_buffer);
	}
	return (status); 
    }
    
    m->mct_allocated = m->mct_oldpages;

    if (DMZ_AM_MACRO(51))
	TRdisplay("dm1xstart: %v buffers: %d oldpages: %d\n",
    		"OVFL,SAVE,KEEP", flag, num_alloc, m->mct_oldpages);

    return (E_DB_OK);
}

/*{
** Name: dm1xfinish - Flush and dellocate build buffer.
**
** Description:
**	This routine will flush any buffers to disk and then deallocate
**	the memory used for this build operation.  If operation is
**	DM1X_CLEANUP then the buffer is just deallocated, else the dirty
**	buffers in memory are flushed before deallocating any memory.
**
** Inputs:
**      mct				Pointer to record access context.
**	operation			Either DM1X_CLEANUP, or DM1X_COMPLETE.
** Outputs:
**      err_code                        Pointer to an area to return
**                                      error code.   May be set to one of the
**                                      following error codes:
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**      08-may-90 (Derek)
**          Created.
**      15-oct-1991 (mikem)
**          Eliminated unix compile warnings ("warning: statement not reached")
**          by changing the way "for (;xxx == E_DB_OK;)" loops were coded.
**	1-apr-1992 (bryanp)
**	    Declare type for "operation". "err_code" is a "i4 *".
**	29-August-1992 (rmuth)
**	    - Changed for new FHDR/FMAP(s) build algorithm. 
**	    - Make sure we have allocated mct_buffer before we deallocate it.
**	    - Change flow to suppress compiler warnings.
**	30-October-1992 (rmuth)
**	    - Call dm1x_build_SMS from here instead of requiring code that
**	      is building the table do it.
**	    - Call dm1x_guarantee_space.
**	    - Set tcb_tmp_hw_pageno.
**	18-apr-1994 (jnash)
**	    fsync project.  Eliminate calls to dm2f_force_file().
**	17-Nov-2009 (kschendel) SIR 122890
**	    Write last dpage after SMS build, which is ever so slightly
**	    more robust in the face of a crash.
*/
DB_STATUS
dm1xfinish(
DM2U_M_CONTEXT	    *mct,
i4		    operation,
DB_ERROR	    *dberr)
{
    DMP_TCB *t;
    DM2U_M_CONTEXT  *m = mct;
    DB_STATUS	    status = E_DB_OK;

    CLRDBERR(dberr);

    do 
    {
	if (operation == DM1X_CLEANUP)
	    break;

	t = m->mct_buildrcb->rcb_tcb_ptr;

	/*
	** Invalidate page pointers to group pages.
	*/
	if (m->mct_curdata != m->mct_dtsave)
	    m->mct_curdata = 0;
	if (m->mct_curovfl != m->mct_ovsave)
	    m->mct_curovfl = 0;
	if (m->mct_curindex != m->mct_nxsave)
	    m->mct_curindex = 0;
	if (m->mct_curleaf != m->mct_lfsave)
	    m->mct_curleaf = 0;

	/*
	** Force group buffers to disk.
	*/
	status = write_group(m, &m->mct_data, dberr);
	if ( status != E_DB_OK )
	    break;

	/*
	** If used overflow buffer write them to disk
	*/
	if ( m->mct_ovfl.mct_pages)
	{
	    status = write_group(m, &m->mct_ovfl, dberr);
	    if ( status != OK )
		break;
	}


	/*
	** Force any save buffers we have used to disk.
	*/
	if ( m->mct_dtsave && m->mct_curdata == m->mct_dtsave)
	{
	    status = write_one(m, m->mct_curdata, dberr);
	    if ( status != E_DB_OK )
	       break;
	}

	if ( m->mct_ovsave && m->mct_curovfl == m->mct_ovsave)
	{
	    status = write_one(m, m->mct_curovfl, dberr);
	    if ( status != E_DB_OK )
	       break;
	}

	if (m->mct_lfsave && m->mct_curleaf == m->mct_lfsave)
	{
	    status = write_one(m, (DMPP_PAGE *)m->mct_curleaf, dberr);
	    if ( status != OK )
		break;
	}

	if ( m->mct_nxsave && m->mct_curindex == m->mct_nxsave)
	{
	    status = write_one(m, (DMPP_PAGE *)m->mct_curindex, dberr);
	    if ( status != E_DB_OK )
		break;
	}

	/*
	** Build the Space management scheme for the table 
	*/
	status = dm1x_build_SMS( m, dberr );
	if ( status != E_DB_OK )
	    break;

	/* Redirect the last-page overflow link to the newly loaded
	** stuff.  last-dpage is only used for heap nonrebuild loads.
	** Do this after the SMS is done so that the new link doesn't
	** point into space.  We aren't really fooling anyone here,
	** if there's a readlock-nolock reader he's going after cache
	** and we're writing direct.  For maximal paranoia we'd take
	** an X control lock around the finish-up, like all other loads
	** do from the start;  but that would break load into self
	** (insert into t select ... from t).
	**
	** FIXME need a solution that allows nolock readers of heap
	** nonrebuild loads, yet is safe in all cases.
	*/
	if ( mct->mct_last_dpage )
	{
	    status = write_one(m, m->mct_last_dpage, dberr );
	    if ( status != E_DB_OK )
		break;
	}

	/*
	** Table has been built and we are not going to use or 
	** allocate any more pages so set the tcb_lalloc
	** and tcb_lpageno to their final values. For inmemory
	** tables we have been doing this as we have been 
	** using/allocating pages but for other tables we may
	** not have accounted for all pages. This is ok as
	** other tables are not seen by anyone else during the
	** build.
	*/
	t->tcb_table_io.tbio_lalloc  = m->mct_allocated - 1;
	t->tcb_table_io.tbio_lpageno  = m->mct_relpages - 1;
	t->tcb_table_io.tbio_checkeof = FALSE;
	if (! m->mct_inmemory )
	{
	    if ( t->tcb_temporary == TCB_TEMPORARY )
	    {
	    	/*
	    	** For a non inmemory table we will have written to disc all
	    	** pages that have been used, so update the tcb_tmp_hw_pageno
	    	** to reflect this. 
	    	*/
	    	t->tcb_table_io.tbio_tmp_hw_pageno = t->tcb_table_io.tbio_lpageno;
	    }
	}

	/*
	** For permanent tables, guarantee that all logically allocated
	** disk space actually exists.  For temp tables this doesn't
	** matter since this is the only session that can see the
	** table, and the tbio numbers are good enough.
	*/
	if ( t->tcb_temporary == TCB_PERMANENT )
	{
	    /*
	    ** Make sure all allocated space has disc space
	    */

	    if (DMZ_AM_MACRO(51))
		TRdisplay("dm1xfinish:guarantee_on_disk\n");

	    if ( m->mct_allocated > m->mct_oldpages )
	    {
		/*
		** If we have allocated some new space to the file
		** then make sure the disc space for any free
		** pages will exist on disc when needed.
		*/
    	        status = dm2f_guarantee_space(
			m->mct_location, m->mct_loc_count,
			&t->tcb_rel.relid,
			&t->tcb_dcb_ptr->dcb_name,
			m->mct_relpages,
			( m->mct_allocated - m->mct_relpages ),
			dberr );
	    	if ( status != E_DB_OK)
		    break;


            	if (DMZ_AM_MACRO(51))
	            TRdisplay( "dm1xfinish:guarantee: start: %d, end: %d\n", 
		   		mct->mct_relpages, mct->mct_allocated) ;

	    }

	    /*
	    ** make sure file header is on disc
	    */
	    status = dm2f_flush_file(
			m->mct_location, m->mct_loc_count,
			&t->tcb_rel.relid,
			&t->tcb_dcb_ptr->dcb_name,
			dberr );
	    if ( status != E_DB_OK )
		break;
	}


    } while (FALSE );

    /*	Handle error logging. */

    if (status != E_DB_OK)
	dm1x_log_error(E_DM92E1_DM1X_FINISH, dberr);

    /*	Deallocate build memory. */
    if ( m->mct_buffer )
    {
        dm0m_deallocate( (DM_OBJECT **)&m->mct_buffer);
    }
    return (status);
}

/*{
** Name: dm1xnewpage - Return pointer to buffer for new page.
**
** Description:
**	This routine will return a pointer to a buffer that has been
**	formatted as a page.  The page_page field will be set and can be
**	used to determine the page number associated with the page.  The
**	newpage pointer to used to determine if a save page pointer is
**	being changed or to check that a requested page from the main
**	segment is within the allocation window for that segment.
**
** Inputs:
**      mct				Pointer to record access context.
**	page				Page number to aallocate.  If 0
**					then allocate next available page.
**					If not zero allocate a specific page.
**					If -1 allocate page 0.
** Outputs:
**	newpage				Pointer to page pointer.
**      err_code                        Pointer to an area to return
**                                      error code.   May be set to one of the
**                                      following error codes:
**                                      
**	Returns:
**
**	    E_DB_OK
**	    E_DB_INFO			Page is out of bounds.
**          E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**      08-may-90 (Derek)
**          Created.
**	29-August-1992 (rmuth)
**	    Changed for new FHDR/FMAP build algorithms.
**	26-jul-1993 (bryanp)
**	    Fix a trace statement in dm1xnewpage() -- it was "unreachable".
**	06-may-1996 (nanpr01)
**	    Changed the page header access as macro.
*/
DB_STATUS
dm1xnewpage(
DM2U_M_CONTEXT	    *mct,
i4		    page,
DMPP_PAGE	    **newpage,
DB_ERROR	    *dberr)
{
    DM2U_M_CONTEXT  *m = mct;
    DB_STATUS	    status;

    CLRDBERR(dberr);

    for (;;)
    {
	/*	Check if the page to be allocated is currently a saved page. */

	if (*newpage &&
	    (*newpage == m->mct_dtsave ||
	     *newpage == m->mct_ovsave ||
	     *newpage == (DMPP_PAGE *)m->mct_lfsave ||
	     *newpage == (DMPP_PAGE *)m->mct_nxsave))
	{
	    /*  Write the saved page to disk. */

	    status = write_one(m, *newpage, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** The preallocated portion is addressed by passing in the requested 
	** page number instead of 0.
	*/
	if ( page != 0 )
	{
	    /*  Check that page is within the current data segment. */

	    if (newpage == &m->mct_curdata && page > m->mct_data.mct_finish)
		return (E_DB_INFO);
	}

	/*	Just allocate a new page. */

	status = allocate(m, page, newpage, dberr);
	if (status != E_DB_OK)
	    break;

	if (DMZ_AM_MACRO(51))
	    TRdisplay("dm1xnewpage: page %d\n", 
		      DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, *newpage));

	return (E_DB_OK);
    }

    /* Don't log if too-many-pages */
    if (dberr->err_code > E_DM_INTERNAL)
	dm1x_log_error(E_DM92E3_DM1X_NEWPAGE, dberr);
    return (status);
}

/*{
** Name: dm1xnextpage - Return next page number.
**
** Description:
**	This routine will return the next page number that will be assigned
**	on the next dm1xnewpage() operation.  It will only work if the
**	next dm1x call is a dm1xnewpage().
**
**
** Inputs:
**      mct				Pointer to record access context.
** Outputs:
**	nextpage			Pointer to return next page number.
**      err_code                        Pointer to an area to return
**                                      error code.   May be set to one of the
**                                      following error codes:
**                                      
**	Returns:
**
**	    E_DB_OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**      08-may-90 (Derek)
**          Created.
**	29-August-1992 (rmuth)
**	    Changed for new FHDR/FMAP build algorithm.
*/
DB_STATUS
dm1xnextpage(
DM2U_M_CONTEXT	    *mct,
i4		    *nextpage,
DB_ERROR	    *dberr)
{
    DM2U_M_CONTEXT  *m = mct;

    CLRDBERR(dberr);

    *nextpage = m->mct_relpages;

    if (DMZ_AM_MACRO(51))
   	TRdisplay("dm1xnextpage: page %d\n", *nextpage);
    return (E_DB_OK);
}

/*{
** Name: dm1xreadpage - Return pointer to previously used page.
**
** Description:
**	This routine will return a pointer to a previously allocated
**	page.  The current value of page is used to select which buffer
**	to perform to search and which buffer to use as the read buffer.
**	The buffer can either be one of the single page buffers or
**	one of the main or ovfl segment buffers.
**
** Inputs:
**      mct				Pointer to record access context.
**	update				DM1X_FORREAD if just reading, and
**					DM1X_FORUPDATE if updating.
**	pageno				Page number to locate.
**	page				Pointer to current page pointer.
** Outputs:
**	page				Pointer to return page pointer.
**      err_code                        Pointer to an area to return
**                                      error code.   May be set to one of the
**                                      following error codes:
**                                      
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**      08-may-90 (Derek)
**          Created.
**      06-may-96 (nanpr01)
**          Changed all page header access as macro for New Page Format 
**	    project.
*/
DB_STATUS
dm1xreadpage(
DM2U_M_CONTEXT	    *mct,
i4		    update,
i4		    pageno,
DMPP_PAGE	    **page,
DB_ERROR	    *dberr)
{
    DM2U_M_CONTEXT  *m = mct;
    MCT_BUFFER	    *group;
    DB_STATUS	    status;

    CLRDBERR(dberr);

    /*	Handle page is still valid. */

    if (*page && 
	(DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, *page) == pageno))
	return (E_DB_OK);

    if (DMZ_AM_MACRO(51))
	TRdisplay("dm1xreadpage: page %d\n", pageno);
    /*	Check if requested buffer is in the data group segment. */

    if (pageno >= m->mct_data.mct_start &&
	pageno <= m->mct_data.mct_finish &&
	pageno < m->mct_data.mct_start + m->mct_mwrite_blocks)
    {
	if (*page == m->mct_dtsave ||
	    *page == m->mct_ovsave ||
	    *page == (DMPP_PAGE *)m->mct_nxsave ||
	    *page == (DMPP_PAGE *)m->mct_lfsave)
	{
	    /*	If reading over a save buffer, put the save buffer on disk. */

	    status = write_one(m, *page, dberr);
	    if (status != E_DB_OK)
		return (status);
	}
	*page = (DMPP_PAGE *)
	    ((char *)m->mct_data.mct_pages +
		(m->mct_page_size * (pageno - m->mct_data.mct_start)));
	if (update && pageno > m->mct_data.mct_last)
	    m->mct_data.mct_last = pageno;
	return (E_DB_OK);
    }

    /*	Check if requested buffer is in the ovfl group segment. */

    if (m->mct_ovfl.mct_pages && pageno >= m->mct_ovfl.mct_start &&
	pageno <= m->mct_ovfl.mct_finish &&
	pageno < m->mct_ovfl.mct_start + m->mct_mwrite_blocks)
    {
	*page = (DMPP_PAGE *)
	    ((char *)m->mct_ovfl.mct_pages +
		(m->mct_page_size * (pageno - m->mct_ovfl.mct_start)));
	if (update && pageno > m->mct_ovfl.mct_last)
	    m->mct_ovfl.mct_last = pageno;
	return (E_DB_OK);
    }

    /*	Read single page if index or leaf pointer is used. */

    if (page == (DMPP_PAGE **)&m->mct_curindex ||
	page == (DMPP_PAGE **)&m->mct_curleaf)
    {
	return (read_one(m, pageno, page, dberr));
    }

    /*	Read group if any other pointer is requested. */

    group = &m->mct_data;
    if (page != &m->mct_curdata)
	group = &m->mct_ovfl;
    status = read_group(m, pageno, group, dberr);
    if (status != E_DB_OK)
    {
	dm1x_log_error(E_DM92E2_DM1X_READPAGE, dberr);
	return (status);
    }
    if (update == DM1X_FORUPDATE)
	group->mct_last = pageno;
    *page = group->mct_pages;

    if (DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type,group->mct_pages) != pageno)
	TRdisplay("dm1xreadpage: want %d got %d\n", page,
	    DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, group->mct_pages));

    return (E_DB_OK);
}

/*{
** Name: dm1xreserve - Place header at current point in file.
**
** Description:
**	This routine will reserve a contiguous segement of space
**	starting at the current location in the table. This
**	enables the placement of FHDR/FMAP(s) to be controlled, for
**	example in a HASH table the FHDR/FMAP(s) must come after the
**	hash bucket pages.
**
**
** Inputs:
**      mct				Pointer to record access context.
**	reserve				The number of pages to reserve.
**
** Outputs:
**      err_code                        Pointer to an area to return
**                                      error code.   May be set to one of the
**                                      following error codes:
**                                      
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**      08-may-90 (Derek)
**          Created.
**	29-August-1992 (rmuth)
**	    Changed Generic header into one which actually describes this
**	    routine, christmas.
*/
DB_STATUS
dm1xreserve(
DM2U_M_CONTEXT	    *mct,
i4		    reserve,
DB_ERROR	    *dberr)
{
    DM2U_M_CONTEXT  *m = mct;
    DB_STATUS	    status;

    CLRDBERR(dberr);

    for (;;)
    {

	/*	Write overflow group before changing overflow window. */

	if (m->mct_ovfl.mct_pages)
	{
	    status = write_group(m, &m->mct_ovfl, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/*	Write data group before changing overflow window. */

	if (m->mct_data.mct_pages)
	{
	    status = write_group(m, &m->mct_data, dberr);
	    if (status != E_DB_OK)
		break;
	}

    	if (DMZ_AM_MACRO(51))
	    TRdisplay("dm1xreserve: relpages = %d reserve = %d\n", 
		m->mct_relpages,  
    		reserve);

	m->mct_data.mct_start = m->mct_relpages;
	m->mct_data.mct_finish = reserve - 1;
	m->mct_data.mct_last = m->mct_data.mct_start - 1;
	m->mct_ovfl.mct_start = reserve;
	m->mct_ovfl.mct_last = reserve - 1;
	m->mct_relpages = reserve;
	return (E_DB_OK);
    }

    dm1x_log_error(E_DM92E4_DM1X_RESERVE, dberr);
    return (status);
}

/*{
** Name: allocate - allocate a page
**
** Desciption:
**      Allocate and format a page.
**
** Inputs:
**      mct                            Pointer to record access context.
**      pageno
**
** Outputs:
**      page 			       Pointer to pointer to allocate page
**      err_code                       Pointer to an area to return
**                                     error code.   May be set to one of the
**                                     following error codes:
**
**      Returns:
**
**         E_DB_OK
**         E_DB_ERROR
**
**      Exceptions:
**          none
**
** SideEffects:
**
** History:
**      08-may-90 (Derek)
**          Created.
**      24-oct-91 (jnash)
**          Header created.
**      09-jun-1992 (kwatts)
**          6.5 MPF Changes. Replaced call of dm1c_format with accessor.
**	21-June-1993 (rmuth)
**	    - Building ISAM tables with overflow chains.
**	      a. Re-scanning these chains caused us to move the mct_ovfl
**	         window backwards. This uncovered bugs where the window
**	         variables 'start', where window begins, and 'finish'
**	         the end of the segment the window can wander across
**	         were being set incorrectly. In more detail when re-setting 
**	         mct_start for the data and overflow buffers,the code does not 
**	         account for the fact that this buffers could have moved 
**	         backwards within the table. Hence the 'next_page' may be 
**	         greater than nwrite_blocks away so mct_start should be set 
**	   	 to 'next_page' not mct_start + nwrite_blocks;
**	      b. The above problem caused us not to allocate a page and
**	         hence an AV occurred. Check for the above and return an
**	         error.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass mct_page_size as the page_size argument to dmpp_format.
**	06-may-1996 (nanpr01)
**	    instead of old_rcb's page accessor function use the m->mct_acc_plv
**	    functions.
**      26-feb-1998 (stial01)
**          B88434: Add check to make sure table does not grow past 
**          DM1P_MAX_TABLE_SIZE.
**	23-Jun-2000 (jenjo02)
**	    Added fill_option to dmpp_format() prototype, set by caller
**	    to DM1C_ZERO_FILL if dmpp_format should MEfill the page,
**	    DM1C_ZERO_FILLED if the page has already been MEfilled.
**	    The page overhead space is always zero-filled.
**	13-May-2005 (schka24)
**	    Return something better than "error allocating a page" when
**	    the 8M page limit is reached.
*/
static DB_STATUS
allocate(
DM2U_M_CONTEXT	    *mct,
i4	    	    pageno,
DMPP_PAGE	    **page,
DB_ERROR	    *dberr)
{
    DM2U_M_CONTEXT  *m = mct;
    i4	    next_page = m->mct_relpages;
    DB_STATUS	    status = E_DB_OK;

    CLRDBERR(dberr);

    /*	Handle reserved area, different from overflow area. */

    if (pageno)
    {
	next_page = pageno;
	if (pageno == -1)
	    next_page = 0;
    }
    else
	m->mct_relpages++;
    *page = 0;

    /*	Check that is in data segment or overflow segment. */

    if (next_page >= m->mct_data.mct_start &&
	next_page <= m->mct_data.mct_finish)
    {
	/*  
	** Allocate and write if outside of window. 
	*/
	if (next_page >= m->mct_data.mct_start + m->mct_mwrite_blocks)
	{
	    /*
	    ** Write out the current window as we will re-use for the
	    ** next m->mct_mwrite_blocks pages
	    */
	    status = write_group(m, &m->mct_data, dberr);
	    if (status == E_DB_OK)
	    {
		/*
		** First page in this window is next_page
		*/
		m->mct_data.mct_start = next_page;
	    }
	}
	*page = (DMPP_PAGE *)
	    ((char *)m->mct_data.mct_pages +
		(m->mct_page_size * (next_page - m->mct_data.mct_start)));
	m->mct_data.mct_last = next_page;
    }
    else if (next_page >= m->mct_ovfl.mct_start &&
	next_page <= m->mct_ovfl.mct_finish)
    {
	/*  
	** Allocate and write if outside of window. 
	*/
	if (next_page >= m->mct_ovfl.mct_start + m->mct_mwrite_blocks)
	{
	    /*
	    ** Write out the current window as we will re-use for the
	    ** next m->mct_mwrite_blocks pages
	    */
	    status = write_group(m, &m->mct_ovfl, dberr);
	    if (status == E_DB_OK)
	    {
		/*
		** First page in this window is next_page
		*/
		m->mct_ovfl.mct_start = next_page;
	    }
	}
	*page = (DMPP_PAGE *)
	    ((char *)m->mct_ovfl.mct_pages +
		(m->mct_page_size * (next_page - m->mct_ovfl.mct_start)));
	m->mct_ovfl.mct_last = next_page;
    }
    else
    {
	/*
	** The mct_start and mct_finish variables for the buffers have
	** somehow been set incorrectly. Hence we cannot determine where
	** to allocate the page from, trap this condition and return an
	** error
	*/
	SETDBERR(dberr, 0, E_DM92E5_DM1X_ALLOCATE);
	status = E_DB_ERROR;
    }

    /*
    ** Make sure extending the table is not going to cause us to
    ** exceed the maximum table size.
    */
    if ( next_page > 
	DM1P_VPT_MAXPAGES_MACRO(m->mct_page_type, m->mct_page_size)) 
    {
	status 	  = E_DB_ERROR;
	SETDBERR(dberr, 0, E_DM0138_TOO_MANY_PAGES);
	return (status);		/* Don't need to log this one */
    }

    if (status != E_DB_OK)
    {
	dm1x_log_error(E_DM92E5_DM1X_ALLOCATE, dberr);
	return (status);
    }

    /*	Format page and return. */

    (*m->mct_acc_plv->dmpp_format)(m->mct_page_type, m->mct_page_size, *page,
				    next_page, (DMPP_DATA | DMPP_MODIFY),
				    DM1C_ZERO_FILL);
    return (E_DB_OK);
}

/*{
** Name: read_one - read a page
**
** Desciption:
**
** Inputs:
**      mct                            Pointer to record access context.
**      pageno			       Pageno of page to read
** Outputs:
**      page			       Pointer to pointer to page read
**      err_code                       Pointer to an area to return
**                                     error code.   May be set to one of the
**                                     following error codes:
**
**      Returns:
**
**          E_DB_OK
**          E_DB_ERROR
**
**      Exceptions:
**          none
**
** Sid Effects:
**
** History:
**      08-may-90 (Derek)
**          Created.
**	24-oct-91 (jnash)
**	    Header created
**	29-August-1992 (rmuth)
**	    - Add I/O Tracing.
**	    - Add trace back error message.
**	09-oct-1992 (jnash)
**	   6.5 Reduced logging project
**	     -  Validate page checksum
**	17-jan-1994 (rmuth)
**	    b58605 The display io_trace routines were displaying the incorrect
**	    file name.
**	20-mar-1996 (nanpr01)
**	    Pass page_size parameter to checksum routines for computing
** 	    checksum.
**	06-May-1996 (nanpr01)
**          Changed all page header access as macro for New Page Format 
**	    project.
*/
static DB_STATUS
read_one(
DM2U_M_CONTEXT	    *mct,
i4		    pageno,
DMPP_PAGE	    **page,
DB_ERROR	    *dberr)
{
    DM2U_M_CONTEXT  *m = mct;
    i4	    num_read = 1;
    DMPP_PAGE	    *save_buffer = *page;
    DB_STATUS	    status = E_DB_OK;
    DMP_PINFO	    pinfo;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (page == &m->mct_curdata)
    {
	save_buffer = m->mct_dtsave;
	if (m->mct_curdata == save_buffer)
	    status = write_one(m, save_buffer, dberr);
    }
    else if (page == &m->mct_curovfl)
    {
	save_buffer = m->mct_ovsave;
	if (m->mct_curovfl == save_buffer)
	    status = write_one(m, save_buffer, dberr);
    }
    else if (page == (DMPP_PAGE **)&m->mct_curindex)
    {
	save_buffer = (DMPP_PAGE *)m->mct_nxsave;
	if (m->mct_curindex == (DMPP_PAGE *)save_buffer)
	    status = write_one(m, save_buffer, dberr);
    }
    else if (page == (DMPP_PAGE **)&m->mct_curleaf)
    {
	save_buffer = (DMPP_PAGE *)m->mct_lfsave;
	if (m->mct_curleaf == (DMPP_PAGE *)save_buffer)
	    status = write_one(m, (DMPP_PAGE *)m->mct_lfsave, dberr);
    }
    if (status != E_DB_OK)
	return (status);

    if (m->mct_inmemory)
    {
	/* Get the page from the buffer manager */
	status = dm0p_fix_page(m->mct_buildrcb, (i4)pageno,
				DM0P_READ, &pinfo, dberr);

	if (status == E_DB_OK)
	{
	    MEcopy((PTR)pinfo.page, m->mct_page_size,
		    (PTR)save_buffer);
	    status = dm0p_unfix_page(m->mct_buildrcb, DM0P_UNFIX, &pinfo,
		    dberr);
	}
    }
    else
    {
	DMP_TCB *t;

	t = m->mct_buildrcb->rcb_tcb_ptr;
	if ( DMZ_SES_MACRO(10))
	{
	    dmd_build_iotrace( 
	     DI_IO_READ,
	     (DM_PAGENO)pageno,
	     num_read,
	     &t->tcb_rel.relid,
     	     &m->mct_location[0].loc_fcb->fcb_filename,
	     &t->tcb_dcb_ptr->dcb_name );
	}
	status = dm2f_read_file(m->mct_location, m->mct_loc_count,  
	    m->mct_page_size,
	    &t->tcb_rel.relid,
	    &t->tcb_dcb_ptr->dcb_name,
	    &num_read, (i4)pageno, (char *)save_buffer, dberr);
	if (status == E_DB_OK)
	{
	    status = dm0p_validate_checksum(save_buffer, m->mct_page_type,
					    (DM_PAGESIZE) m->mct_page_size);
	    if (status != E_DB_OK)
	    {
		uleFormat(NULL, E_DM930C_DM0P_CHKSUM, (CL_ERR_DESC *)NULL, 
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
		    err_code, 3, 
		    sizeof(DB_DB_NAME), 
		    &t->tcb_dcb_ptr->dcb_name,
		    sizeof(DB_TAB_NAME), 
		    &t->tcb_rel.relid,
		    0, 
		    DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, save_buffer));
	    }
	}

    }

    if (status != E_DB_OK)
    {
	dm1x_log_error(E_DM92E8_DM1X_READ_ONE, dberr);
	return (status);
    }

    if (DMZ_AM_MACRO(51))
	TRdisplay("DM1X:read_one : page %d buffer %w%w%w%w\n",
	    pageno,
	    ",DATA", save_buffer == m->mct_dtsave,
	    ",OVFL", save_buffer == m->mct_ovsave,
	    ",LEAF", save_buffer == (DMPP_PAGE *)m->mct_lfsave,
	    ",INDEX", save_buffer == (DMPP_PAGE *)m->mct_nxsave);

    *page = save_buffer;
    return (E_DB_OK);
}

/*{
** Name: write_one - Write a page
**
** Desciption:
**
** Inputs:
**     mct                            Pointer to record access context.
**     page                           Page to write
** Outputs:
**     err_code                       Pointer to an area to return
**                                    error code.   May be set to one of the
**                                    following error codes:
**
**      Return:
**
**          E_DB_OK
**          E_DB_ERROR
**
**      Exceptions:
**         none
**
** Side Effects:
**
** History:
**      08-may-90 (Dere)
**          Created.
**      24-oct-91 (jnash)
**          Header created
**	29-August-1992 (rmuth)
**	   - Added I/O tracing.
**	   - Add trace back error message.
**	09-oct-1992 (jnash)
**	   6.5 reduced logging project
**	     -  Insert checksum on page
**	14-dec-1992 (jnash & rogerk)
**	    Removed DMPP_ACC_PLV argument from dm2f_write_file.
**	17-Jan-1994 (rmuth)
**	    b58605 The display io_trace routines were displaying the incorrect
**	    file name.
**	20-mar-1996 (nanpr01)
**	    Pass page_size parameter to checksum routines for computing
** 	    checksum.
**	06-may-1996 (nanpr01)
**	    Changed the page header access to macro for New Page Format.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
*/
static DB_STATUS
write_one(
DM2U_M_CONTEXT	    *mct,
DMPP_PAGE	    *page,
DB_ERROR	    *dberr)
{
    DM2U_M_CONTEXT  *m = mct;
    DMP_TCB	    *t;
    i4	    num_write = 1;
    DB_STATUS	    status;
    DMP_PINFO	    pinfo;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    t = m->mct_buildrcb->rcb_tcb_ptr;
    if (m->mct_inmemory)
    {
	/* move page into buffer manager */
	status = dm0p_fix_page(m->mct_buildrcb, 
		      (i4)DMPP_VPT_GET_PAGE_PAGE_MACRO(m->mct_page_type, page),
		      DM0P_WRITE, &pinfo, dberr);

	if (status == E_DB_OK)
	{
	    dm0pMutex(&t->tcb_table_io, (i4)0,
		    m->mct_buildrcb->rcb_lk_id, &pinfo);
	    MEcopy((PTR)page, m->mct_page_size, (PTR)pinfo.page);
	    DMPP_VPT_SET_PAGE_STAT_MACRO(m->mct_page_type, pinfo.page, 
					DMPP_MODIFY);
	    dm0pUnmutex(&t->tcb_table_io, 
		    (i4)0, m->mct_buildrcb->rcb_lk_id, &pinfo);
	    status = dm0p_unfix_page(m->mct_buildrcb, DM0P_UNFIX, &pinfo,
		    dberr);
	}
    }
    else
    {
	if ( DMZ_SES_MACRO(10))
	{
	    dmd_build_iotrace( 
	     DI_IO_WRITE,
	     DMPP_VPT_GET_PAGE_PAGE_MACRO(m->mct_page_type, page), 1,
	     &t->tcb_rel.relid,
     	     &m->mct_location[0].loc_fcb->fcb_filename,
	     &t->tcb_dcb_ptr->dcb_name );
	}
	dm0p_insert_checksum(page, m->mct_page_type, (DM_PAGESIZE) m->mct_page_size);
	status = dm2f_write_file(m->mct_location, m->mct_loc_count,  
	    m->mct_page_size,
	    &t->tcb_rel.relid,
	    &t->tcb_dcb_ptr->dcb_name,
            &num_write, DMPP_VPT_GET_PAGE_PAGE_MACRO(m->mct_page_type, page), 
	    (char *)page, dberr);
    }

    if (status != E_DB_OK )
    {
    	dm1x_log_error(E_DM92E7_DM1X_WRITE_ONE, dberr);
    	return (status);
    }

    if (DMZ_AM_MACRO(51))
	TRdisplay("DM1X:write_one : page %d\n", 
			DMPP_VPT_GET_PAGE_PAGE_MACRO(m->mct_page_type, page));

    return (E_DB_OK);
    
}

/*{
** Name: read_group - read a group of pages
**
** Desciption:
**
** Inputs:
**     mct                            Pointer to record access context.
**     pageno                         Pageno of page to read
** Outputs:
**     group                          Pointer to group read
**     err_code                       Pointer to an area to return
**                                    error code.   May be set to one of the
**                                    following error codes:
**
**      Returs:
**
**          E_DB_OK
**          E_DB_ERROR
**
**      Exceptions:
*          none
**
** Sid Effects:
**
** History:
**      08-may-90 (Derk)
**          Created.
**      24-oct-91 (jnash)
**          Header creted
**	29-August-1992 (rmuth)
**	    - Add I/O tracing.
**	    - Add trace back error message.
**	09-oct-1992 (jnash)
**	   Reduced logging project
**	     -  Validate page checksum
**	21-jun-1993 (rogerk)
**	    Changed to use mct_relpages rather than mct_allocated to limit how
**	    far out to read ahead of the requested page.  Using mct_allocated
**	    caused us to attempt to read pages allocated but not yet forced.
**	21-June-1993 (rmuth)
**	    Building ISAM tables with overflow chains, whilst re-scanning 
**	    these chains caused us to move the mct_ovfl window backwards. 
**	    This uncovered bugs where the window variables 'start', where 
**	    window begins, and 'finish' the end of the segment the window 
**	    can wander across were being set incorrectly. In more detail 
**	    when re-reading pages we were resetting the 'finish' mark which is
**	    incorrect as the segment we can wander over has not changed just
**	    the window position within the segment. Note the window is a fixed
**	    size and we know the starting pageno so easy to work out the
**	    window end.
**	17-Jan-1994 (rmuth)
**	    b58605 The display io_trace routines were displaying the incorrect
**	    file name.
**	20-mar-1996 (nanpr01)
**	    Pass page_size parameter to checksum routines for computing
** 	    checksum.
**	06-May-1996 (nanpr01)
**	    Changed all page header access as macro for New Page Format 
**	    project.
*/
static DB_STATUS
read_group(
DM2U_M_CONTEXT	    *mct,
i4		    pageno,
MCT_BUFFER	    *group,
DB_ERROR	    *dberr)
{
    DM2U_M_CONTEXT  *m = mct;
    i4	    num_read = m->mct_mwrite_blocks;
    DB_STATUS	    status;
    i4	    i;
    DMP_PINFO	    pinfo;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    status = write_group(m, group, dberr);
    if (status != E_DB_OK)
	return (status);

    group->mct_start = pageno;
    group->mct_last = pageno - 1;

    /*
    ** Check if num_read pages will take us past the EOF.  If so, then reduce
    ** the number of pages to read.  (In determining EOF use the number of
    ** pages formatted in the table structure (mct_relpages), not the pages 
    ** allocated to the file (mct_allocated).  The latter page count may
    ** include pages allocated but not written which cannot be read.
    */
    if (group->mct_start + num_read >= m->mct_relpages)
	num_read = m->mct_relpages - group->mct_start;

    /*
    ** Check if num_read pages will take us into the overflow page range.
    ** We do not want to read overflow pages into the data page group buffers.
    */
    if (group == &mct->mct_data && mct->mct_ovfl.mct_pages &&
	    group->mct_start + num_read - 1 >= mct->mct_ovfl.mct_start)
    {
	num_read = mct->mct_ovfl.mct_start - group->mct_start;
    }

    if (m->mct_inmemory)
    {
	/* read group from buffer manager */
	for (i = 0, status = E_DB_OK; i < num_read && status == E_DB_OK; i++)
	{
	    status = dm0p_fix_page(m->mct_buildrcb, (i4)pageno + i,
				DM0P_READ, &pinfo, dberr);

	    if (status == E_DB_OK)
	    {
		MEcopy((PTR)pinfo.page, m->mct_page_size,
			(PTR)((char *)group->mct_pages+(i*m->mct_page_size)));
		status = dm0p_unfix_page(m->mct_buildrcb, DM0P_UNFIX,
			&pinfo, dberr);
	    }
	}
    }
    else
    {
	DMP_TCB *t;

	t = m->mct_buildrcb->rcb_tcb_ptr;
	if ( DMZ_SES_MACRO(10))
	{
	    dmd_build_iotrace( 
	     DI_IO_READ,
	     (DM_PAGENO)pageno,
	     num_read,
	     &t->tcb_rel.relid,
     	     &m->mct_location[0].loc_fcb->fcb_filename,
	     &t->tcb_dcb_ptr->dcb_name );
	}

	status = dm2f_read_file(m->mct_location, m->mct_loc_count,  
	    m->mct_page_size,
	    &t->tcb_rel.relid,
	    &t->tcb_dcb_ptr->dcb_name,
	    &num_read, (i4)pageno, (char *)group->mct_pages, dberr);

	if (status == E_DB_OK)
	{
	    for (i = 0; i < num_read; i++)
	    {
	        status = dm0p_validate_checksum(
		 (DMPP_PAGE *)((char *)group->mct_pages+(i*m->mct_page_size)),
		 m->mct_page_type, (DM_PAGESIZE) m->mct_page_size);
		if (status != E_DB_OK)
		{
		    DMPP_PAGE 	*tmp_page = (DMPP_PAGE *)
			    ((char *)group->mct_pages+(i*m->mct_page_size));

		    uleFormat(NULL, E_DM930C_DM0P_CHKSUM, (CL_ERR_DESC *)NULL, 
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			err_code, 3, sizeof(DB_DB_NAME), 
			&t->tcb_dcb_ptr->dcb_name,
			sizeof(DB_TAB_NAME), 
			&mct->mct_oldrcb->rcb_tcb_ptr->tcb_rel.relid, 0, 
			DMPP_VPT_GET_PAGE_PAGE_MACRO(m->mct_page_type, tmp_page));
		}
	    }
	}
    }

    if (DMZ_AM_MACRO(51))
	TRdisplay("DM1X:read_group: page %d..%d\n", 
		  pageno, pageno + num_read - 1);

    if ( status == E_DB_OK )
	return( E_DB_OK );

    dm1x_log_error(E_DM92EA_DM1X_READ_GROUP, dberr);
    return (status);
}

/*{
** Name: write_group - Write a group of pages
**
** Desciption:
**
** Inputs:
**     mct                            Pointer to record access context.
**     group			      Pointer to MCT buffer of group
** 
** Outputs:
**     err_code                       Pointer to an area to return
**                                    error code.   May be set to one of the
**                                    following error codes:
**				      E_DM92F0_DM1X_WRITE_GROUP
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
**      Exceptions:
**         none
**
** Side Effects:
**
** History:
**      08-may-90 (Derek)
**          Created.
**      24-oct-91 (jnash)
**          Header created
**      24-oct-91 (jnash)
**	    Remove & in front of two ule parameters to make LINT happy.
**	13-feb-1992 (bryanp)
**	    Set *err_code when encountering error in write_group. Initialize
**	    'need' to 0. After logging error code, set return err_code
**	    correctly.
**      15-jul-1992 (bryanp)
**          We must flush the allocations for temporary tables, just as we
**          do for permanent tables. We want to flush the allocation before
**          we start filling in the allocated pages in the buffer manager, in
**          case it is these particular pages which fill up the cache and must
**          get bumped out to disk.
**	29-August-1992 (rmuth)
**	    Add I/O tracing.
**	09-oct-1992 (jnash)
**	   6.5 Reduced logging project
**	     -  Insert checksum on each page.
**	30-August-1992 (rmuth)
**	    Remove call to dm2f_flush_file we only call this at the end
**	    of the build operation and if the flush flag is set.
**	30-October-1992 (rmuth)
**	    - Set tbio_lpageno and tbio_lalloc correctly.
**	    - Remove call to dm2f_flush_file this is now only performed
**	      as the end of the build if mct_guarantee_space flag is set.
**	10-nov-1992 (rogerk)
**	    Changed dm0p_mutex calls to follow new buffer manager rules.
**	14-dec-1992 (jnash & rogerk)
**	    Removed DMPP_ACC_PLV argument from dm2f_write_file.
**	21-June-1993 (rmuth)
**	    When building a non-temporary table write_groyp was incorrectly
**	    setting the old tables tbio_lpageno and tbio_lalloc to the new
**	    table values as the new table was being built. This caused havoc
**	    if we are still reading the old table, for example modifing a 
**	    table to heap.
**	26-jul-1993 (rmuth)
**	    If caller has sepcified mct_override_alloc then use this value 
**	    instead of mct_allocation.
**	18-oct-1993 (rmuth)
**	    write_group() : If initial allocation was less the required 
**	    space then we were incorrectly calculating how much extra
**	    space needed to be allocated.
**	17-Jan-1994 (rmuth)
**	    b58605 The display io_trace routines were displaying the incorrect
**	    file name.
**	23-may-1994 (andys)
**	    Add cast(s) to quiet qac/lint
**	06-mar-1996 (stial01 for bryanp)
**	    Use MEcopy, rather than STRUCT_ASSIGN_MACRO, to copy pages.
**	20-mar-1996 (nanpr01)
**	    Pass page_size parameter to checksum routines for computing
** 	    checksum.
**	06-may-1996 (nanpr01)
**	    Changed the page header access to macro for New Page Format
**	    project.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
*/
static DB_STATUS
write_group(
DM2U_M_CONTEXT	    *mct,
MCT_BUFFER	    *group,
DB_ERROR	    *dberr)
{
    DM2U_M_CONTEXT  *m = mct;
    DMP_TCB *t;
    i4	    num_alloc;
    i4	    num_write;
    i4	    page;
    i4	    need = 0;
    i4	    extend;
    DB_STATUS	    status = E_DB_OK;
    i4	    i;
    DMP_PINFO	    pinfo;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    t = m->mct_buildrcb->rcb_tcb_ptr;
    /*
    **	Copy pages to save buffers if the group to be written contains
    **	any active pages.
    */
    if (m->mct_dtsave && m->mct_curdata >= group->mct_pages &&
	(char *)m->mct_curdata <
	    ((char *)group->mct_pages+(m->mct_mwrite_blocks*m->mct_page_size)))
    {
	MEcopy((PTR)m->mct_curdata,m->mct_page_size, (PTR)m->mct_dtsave);
	m->mct_curdata = m->mct_dtsave;
    }
    if (m->mct_ovsave && m->mct_curovfl >= group->mct_pages &&
	(char *)m->mct_curovfl <
	    ((char *)group->mct_pages+(m->mct_mwrite_blocks*m->mct_page_size)))
    {
	MEcopy((PTR)m->mct_curovfl,m->mct_page_size, (PTR)m->mct_ovsave);
	m->mct_curovfl = m->mct_ovsave;
    }
    if (m->mct_lfsave && (DMPP_PAGE *)m->mct_curleaf >= group->mct_pages &&
	(char *)m->mct_curleaf <
	    ((char *)group->mct_pages+(m->mct_mwrite_blocks*m->mct_page_size)))
    {
	MEcopy((PTR)m->mct_curleaf,m->mct_page_size, (PTR)m->mct_lfsave);
	m->mct_curleaf = m->mct_lfsave;
    }
    if (m->mct_nxsave && (DMPP_PAGE *)m->mct_curindex >= group->mct_pages &&
	(char *)m->mct_curindex <
	    ((char *)group->mct_pages+(m->mct_mwrite_blocks*m->mct_page_size)))
    {
	MEcopy((PTR)m->mct_curindex,m->mct_page_size, (PTR)m->mct_nxsave);
	m->mct_curindex = m->mct_nxsave;
    }

    /*	
    ** End limit is either end of window or end of file. 
    */
    num_write = group->mct_last;
    if (m->mct_relpages < num_write)
	num_write = m->mct_relpages;

    /*	
    ** Convert to number of blocks from beginning of window. 
    */
    num_write -= group->mct_start - 1;

    /*
    ** Make sure there is something to write
    */
    if (num_write <= 0)
	return (E_DB_OK);


    /*	
    ** Minimize with the maximum transfer size. 
    */
    if (num_write > m->mct_mwrite_blocks)
	num_write = m->mct_mwrite_blocks;

    /*
    ** If necessary allocate more space to the table and then 
    ** write the data.
    */
    do
    {
    	/*	
    	** Check whether physical space needs to be allocated. 
    	*/
    	if (group->mct_start + num_write > m->mct_allocated)
    	{
	    /*  
	    ** Compute last page touched by this transfer. 
	    */
	    need = group->mct_start + num_write - m->mct_allocated;
	    num_alloc = 0;
	    extend = m->mct_extend;

	    /*  
	    ** If this is the first allocation to this file then use the
	    ** tables initial allocation size
	    */
	    if (m->mct_allocated == 0)
	    {
		num_alloc = mct->mct_override_alloc;
		if ( num_alloc == 0 )
	    	    num_alloc = m->mct_allocation;
	    }

	    /*  
	    ** if need more space increase in multiples of the extend unit. 
	    */
	    if (num_alloc < need)
	    	num_alloc += 
		    (((need - num_alloc) + extend - 1) / extend) * extend;

	    /*  
	    ** Perform the actual allocation. 
	    */
	    page = m->mct_allocated;

	    if (m->mct_inmemory)
	    {
	    	status = dm2f_alloc_file(
		 	t->tcb_table_io.tbio_location_array,
		 	t->tcb_table_io.tbio_loc_count,
		 	&t->tcb_rel.relid,
		 	&t->tcb_dcb_ptr->dcb_name,
		 	num_alloc, &page, dberr);
		if ( status != E_DB_OK )
		    break;

		/*
		** As we are building into the BM make sure we increase
		** this value. As if the build code re-reads this page
		** the BM will check the page is valid using this
		*/

	     	t->tcb_table_io.tbio_lalloc = page;
	    }
	    else
	    {
	    	status = dm2f_alloc_file( 
			    m->mct_location, m->mct_loc_count,
		    	    &t->tcb_rel.relid,
		 	    &t->tcb_dcb_ptr->dcb_name,
		 	    num_alloc, &page, dberr);	    
		if (status != E_DB_OK )
		    break;

	    }

	    if (DMZ_AM_MACRO(51))
	    	TRdisplay("DM1X:allocate : page %d..%d\n", 
			  m->mct_allocated, page);

	    m->mct_allocated = page + 1;
	}

        /*
    	** Enough space has been allocated so write the information
    	*/
	if (m->mct_inmemory)
	{
	    /* 
	    ** write pages into buffer manager 
	    */
	    for (i = 0; i < num_write ; i++)
	    {
		status = dm0p_fix_page(m->mct_buildrcb,
			(i4)(DMPP_VPT_GET_PAGE_PAGE_MACRO(m->mct_page_type, 
							group->mct_pages) + i),
				DM0P_SCRATCH, &pinfo, dberr);
		if ( status != E_DB_OK )
		    break;

		/*
		** As we are building into the BM make sure we increase
		** this value. As if the build code re-reads this page
		** the BM will check the page is valid using this
		*/
	        t->tcb_table_io.tbio_lpageno = 
		      DMPP_VPT_GET_PAGE_PAGE_MACRO(m->mct_page_type, pinfo.page);

		dm0pMutex(&t->tcb_table_io, (i4)0,
		    m->mct_buildrcb->rcb_lk_id, &pinfo);

	    	MEcopy(((char *)group->mct_pages+(i*m->mct_page_size)),
			m->mct_page_size, (char *)pinfo.page);

		DMPP_VPT_SET_PAGE_STAT_MACRO(m->mct_page_type, pinfo.page, 
					 DMPP_MODIFY);

		dm0pUnmutex(&t->tcb_table_io, 
		    (i4)0, m->mct_buildrcb->rcb_lk_id, &pinfo);

		status = dm0p_unfix_page(m->mct_buildrcb, DM0P_UNFIX,
					 &pinfo, dberr);
		if ( status != E_DB_OK )
		    break;
	    }

	    /*
	    ** Make sure moved into the BM ok
	    */
	    if ( status != E_DB_OK )
		break;
	}
	else
	{
	    if ( DMZ_SES_MACRO(10))
	    {
	    	dmd_build_iotrace( 
	     	    DI_IO_WRITE, 
		    DMPP_VPT_GET_PAGE_PAGE_MACRO(m->mct_page_type,group->mct_pages),
		    num_write, &t->tcb_rel.relid,
     	    	    &m->mct_location[0].loc_fcb->fcb_filename,
	     	    &t->tcb_dcb_ptr->dcb_name );
	    }

	    /* Insert a checksum on each page */
	    for (i = 0; i < num_write; i++)
	        dm0p_insert_checksum((DMPP_PAGE *)
			((char *)group->mct_pages+(i*m->mct_page_size)),
			m->mct_page_type, (DM_PAGESIZE) m->mct_page_size);

	    status = dm2f_write_file(m->mct_location, m->mct_loc_count,  
		m->mct_page_size,
		&t->tcb_rel.relid,
		&t->tcb_dcb_ptr->dcb_name,
		&num_write, 
		DMPP_VPT_GET_PAGE_PAGE_MACRO(m->mct_page_type, group->mct_pages), 
		(char *)group->mct_pages, dberr);
	     if (status != E_DB_OK )
		 break;

	}

    } while ( FALSE );

    /*	Handle any I/O errors. */

    if (status != E_DB_OK)
	dm1x_log_error(E_DM92F0_DM1X_WRITE_GROUP, dberr);


    if (DMZ_AM_MACRO(51))
	TRdisplay("DM1X: write_group: page %d..%d\n", group->mct_start,
    		group->mct_start + num_write -1);

     return (status);
}

/*{
** Name: dm1xLogErrorFcn - call uleformat to log an error
**
** Description:
**
** Inputs:
**     status				status to log
**     err_code				err_code to log
** Outputs:
**     None
**
**      Returns:
**          VOID
**
**      Exceptions:
*          none
**
** Sid Effects:
**
** History:
**      08-may-90 (Derk)
**          Created.
**      24-oct-91 (jnash)
**          Header created
**	04-Dec-2008 (jonj)
**	    Renamed to dm1xLogErrorFcn, invoked by dm1x_log_error macro
*/
static VOID
dm1xLogErrorFcn(
i4	    terr_code,
DB_ERROR    *dberr,
PTR	    FileName,
i4	    LineNumber)
{
    i4		l_err_code;
    DB_ERROR	local_dberr;

     /* 
     ** Is error message a DMF internal error message if so issue the
     ** error. Otherwise preserve err_code and issue the trace back 
     ** message.
     */
    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
	    (i4)0, (i4 *)NULL, &l_err_code, 0);

	dberr->err_code = terr_code;
	dberr->err_file = FileName;
	dberr->err_line = LineNumber;
    }
    else
    {
	local_dberr.err_code = terr_code;
	local_dberr.err_data = 0;
	local_dberr.err_file = FileName;
	local_dberr.err_line = LineNumber;

	uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
		    (i4)0, (i4 *)NULL, &l_err_code, 0);
    }
}

/*{
** Name: dm1x_build_SMS - Build a tables FHDR/FMAP(s) space management
**			  scheme.
**
** Description:
**	This routine will build a tables FHDR/FMAP(s) by if necessary
**	adding the FHDR/FMAP(s) and then marking the appropriate pages
**	as either free or used.
**
**	It uses the mct control block to determine the following
**
**	a. Use exiting FHDR and FMAP, this is done when we load data
**	   in a heap table which contains data and we have not asked
**	   for the table to be overwritten.
**	b. If the table is an in-memory table or not, in the former
**	   case we build/update the FHDR/FMAP into the buffer cache
**	   otherwise we do it on disc.
**
**
** Inputs:
**      mct				Pointer to record access context.
** Outputs:
**      err_code                        Pointer to an area to return
**                                      error code.   May be set to one of the
**                                      following error codes:
**                                      
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	29-August-1992 (rmuth)
**	    Created.
**	30-October-1992 (rmuth)
**	    Made static as now called from dm1xfinish.
**	17-Jan-1993 (rmuth)
**	    If an inmemory table do not display the I/O trace.
**	06-mar-1996 (stial01 for bryanp)
**	    Set SMS_page_size in the DM1P_BUILD_SMS_CB.
**	20-Jul-2000 (jenjo02)
**	    Pass oldrcb to SMS build function so that its
**	    TCB's tbio_lalloc can be updated if need be.
**	    This is required for RAW files.
*/
static DB_STATUS
dm1x_build_SMS(
DM2U_M_CONTEXT	    *mct,
DB_ERROR	    *dberr)
{
    DB_STATUS		status;
    DM1P_BUILD_SMS_CB	SMS_build_cb;

    DM_PAGENO	start_pageno; /* Used if io_trace is on */

    CLRDBERR(dberr);

    /* If I/O Tracing record some information */
    if ( DMZ_SES_MACRO(10))
    {
	start_pageno = mct->mct_relpages;
    }

    /*
    ** Build the Space mangement information block
    */
    SMS_build_cb.SMS_pages_allocated= mct->mct_allocated;
    SMS_build_cb.SMS_pages_used	    = mct->mct_relpages;
    SMS_build_cb.SMS_extend_size    = mct->mct_extend;
    SMS_build_cb.SMS_page_type      = mct->mct_page_type;
    SMS_build_cb.SMS_page_size	    = mct->mct_page_size;
    SMS_build_cb.SMS_fhdr_pageno    = mct->mct_fhdr_pageno;
    SMS_build_cb.SMS_loc_count	    = mct->mct_loc_count;
    SMS_build_cb.SMS_plv	    = mct->mct_acc_plv;
    SMS_build_cb.SMS_inmemory_rcb   = mct->mct_buildrcb;

    if ( mct->mct_inmemory )
    {
        SMS_build_cb.SMS_build_in_memory = TRUE;
	SMS_build_cb.SMS_location_array  = 
	    mct->mct_buildrcb->rcb_tcb_ptr->tcb_table_io.tbio_location_array;
    }
    else
    {
        SMS_build_cb.SMS_build_in_memory = FALSE;
        SMS_build_cb.SMS_location_array  = mct->mct_location;
    }

    STRUCT_ASSIGN_MACRO( mct->mct_oldrcb->rcb_tcb_ptr->tcb_rel.relid, 
						SMS_build_cb.SMS_table_name);
    STRUCT_ASSIGN_MACRO( mct->mct_oldrcb->rcb_tcb_ptr->tcb_dcb_ptr->dcb_name,
						SMS_build_cb.SMS_dbname);

    /*
    ** See if we are appending to the end of an existing table
    */
    if ( ! mct->mct_rebuild )
    {
        SMS_build_cb.SMS_start_pageno = mct->mct_lastused + 1;
	SMS_build_cb.SMS_new_table    = FALSE;
	SMS_build_cb.SMS_fhdr_pageno  = 
				mct->mct_oldrcb->rcb_tcb_ptr->tcb_rel.relfhdr;
    }
    else
    {
	SMS_build_cb.SMS_start_pageno = 0;
	SMS_build_cb.SMS_new_table    = TRUE;
	SMS_build_cb.SMS_fhdr_pageno  = DM_TBL_INVALID_FHDR_PAGENO;
    }

    /*
    ** build the space management scheme
    */
    status = dm1p_build_SMS( &SMS_build_cb, dberr );

    if ( status == E_DB_OK)
    {
    	/*
    	** Copy back the relevent information
    	*/
    	mct->mct_relpages    = SMS_build_cb.SMS_pages_used;
    	mct->mct_allocated   = SMS_build_cb.SMS_pages_allocated;
    	mct->mct_fhdr_pageno = SMS_build_cb.SMS_fhdr_pageno;


        if ((!mct->mct_inmemory) && ( DMZ_SES_MACRO(10)))
    	{
    	    dmd_build_iotrace( 
     	    	DI_IO_WRITE,
	    	start_pageno,
	    	mct->mct_relpages - start_pageno,
     	    	&mct->mct_oldrcb->rcb_tcb_ptr->tcb_rel.relid,
     	    	&mct->mct_location[0].loc_fcb->fcb_filename,
     	    	&mct->mct_oldrcb->rcb_tcb_ptr->tcb_dcb_ptr->dcb_name );
    	}

    	if (DMZ_AM_MACRO(51))
		TRdisplay( "dm1x_build_SMS: fhdr_pageno: %d, relpages: %d, allocated: %d, New Table: %v, In Memory: %v\n", 
		   mct->mct_fhdr_pageno,
		   mct->mct_relpages,
		   mct->mct_allocated,
		   "FALSE,TRUE", SMS_build_cb.SMS_new_table,
		   "FALSE,TRUE", SMS_build_cb.SMS_build_in_memory);

	return( status );

    }

    dm1x_log_error( E_DM92E9_DM1X_BUILD_SMS, dberr );
    return( status );
}

/*{
** Name: dm1xfree - Mark a range of pages as free in the FHDR/FMAP.
**
** Description:
**	This routine will mark a range of pages as free in the FHDR
**	FMAP. Create the routine here as oppose to dm1p as it can
**	deal better with building in and not in memory.
**	The routine is currently only sed when building isam tables
**	which have overflow pages.
**
**
** Inputs:
**      mct				Pointer to record access context.
**	start				Starting bit number to mark as free.
**	end				Ending bit number to mark as free.
** Outputs:
**      err_code                        Pointer to an area to return
**                                      error code.   May be set to one of the
**                                      following error codes:
**                                      
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**      08-may-90 (Derek)
**          Created.
**	29-August-1992 (rmuth)
**	     Changed Generic header to describe what this routine really
**	     does.
**      30-October-1992 (rmuth)
**          Rewrote for new build routines.
**      06-mar-1996 (stial01)
**          Don't allocate FMAP/FHDR pages on stack, Instead allocate
**          them from dynamic memory using page_size.
**	06-may-1996 (thaju02 & nanpr01)
**	    Pass added page size parameter to dm1p_single_fmap_free() routine.
**      21-may-1996 (thaju02)
**          Pass page size to NUMBER_FROM_MAP_MACRO for 64 bit tid support 
**	    and multiple fhdr pages.
**	16-jul-1998 (thaju02)
**	    map_index incorrectly calculated for page size > 2k. Changed
**	    DM1P_FSEG to DM1P_FSEG_MACRO. (B92102)
**	02-Apr-2004 (jenjo02)
**	    Allocate page-aligned memory for improved I/O
**	    performance on certain file systems.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
*/
DB_STATUS
dm1xfree(
DM2U_M_CONTEXT	    *mct,
DM_PAGENO	    start,
DM_PAGENO	    end,
DB_ERROR	    *dberr)
{
    DM2U_M_CONTEXT  *m = mct;
    DM_PAGENO	    new_map;
    DB_STATUS	    status, s;
    DM1P_FHDR	    *fhdr_ptr;
    DM1P_FMAP	    *fmap_ptr;
    i4		map_index;
    u_i4	size, directio_align;
    bool	read_fhdr = FALSE;
    bool	read_fmap = FALSE;
    bool	pages_to_mark = TRUE;
    DMP_MISC	*misc_pages = 0;
    DB_ERROR	local_dberr;

    CLRDBERR(dberr);

    if (DMZ_AM_MACRO(51))
	TRdisplay("DM1X_FREE: mark free %d..%d\n", start, end);

    do
    {
	directio_align = dmf_svcb->svcb_directio_align;
	if (dmf_svcb->svcb_rawdb_count > 0 && directio_align < DI_RAWIO_ALIGN)
	    directio_align = DI_RAWIO_ALIGN;
	size = 2 * m->mct_page_size + sizeof(DMP_MISC);
	if (directio_align != 0)
	    size += directio_align;
	/* Allocate aligned space for FMAP/FHDR pages */ 
	status = dm0m_allocate((SIZE_TYPE)size,
		(i4)0, (i4)MISC_CB, (i4)MISC_ASCII_ID,
		 (char *)NULL, (DM_OBJECT **)&misc_pages, dberr);

	if (status != E_DB_OK)
	    break;

	/* Initialize FMAP/FHDR page pointers */
	fhdr_ptr = (DM1P_FHDR *) ((char *)misc_pages + sizeof(DMP_MISC));
	if (directio_align != 0)
	    fhdr_ptr = (DM1P_FHDR *) ME_ALIGN_MACRO(fhdr_ptr, directio_align);
	misc_pages->misc_data = (char*)fhdr_ptr;
	fmap_ptr = (DM1P_FMAP *)((char *)fhdr_ptr + m->mct_page_size);

	/*
	** Read the FHDR page
	*/
	status = read_one( m, m->mct_fhdr_pageno,
			   (DMPP_PAGE **)&fhdr_ptr, dberr);
	if ( status != E_DB_OK )
	    break;

	read_fhdr = TRUE;
	/*
	** Work out first FMAP for this range of pages.
	*/
	map_index = 
	    start / DM1P_FSEG_MACRO(mct->mct_page_type, mct->mct_page_size);
	if ( map_index > DM1P_VPT_GET_FHDR_COUNT_MACRO(mct->mct_page_type, 
						   fhdr_ptr) )
	{
	    status = E_DB_ERROR;
	    SETDBERR(dberr, 0, E_DM92E6_DM1X_FREE);
	    break;
	}

	new_map = VPS_NUMBER_FROM_MAP_MACRO(mct->mct_page_type,
	  DM1P_VPT_GET_FHDR_MAP_MACRO(mct->mct_page_type, fhdr_ptr, map_index));

	/*
	** Read the FMAP
	*/
	status = read_one( m, new_map,
			   (DMPP_PAGE **)&fmap_ptr, dberr);
	if ( status != E_DB_OK )
	    break;

	read_fmap = TRUE;


	/*
	**  Mark pages free from start..end.  If return STATUS is E_DB_INFO
	**  then the FMAP page is incorrect.  Use new_map as the FMAP page
	**  to use.
	*/

	while ( pages_to_mark )
	{
	    status = dm1p_single_fmap_free( fhdr_ptr, 
			fmap_ptr, m->mct_page_type, m->mct_page_size, 
			&start, &end, &new_map, dberr);
	    if ( status == E_DB_OK )
	    {
		pages_to_mark = FALSE;
	    }
	    else
		if ( status == E_DB_INFO )
		{
		    /*
		    ** The range of pages spans more than one FMAP
		    ** so write curretn FMAP and read the next one
		    */
		    status = write_one(m, (DMPP_PAGE *)fmap_ptr, dberr);
		    if (status != E_DB_OK)
	    	        break;

		    read_fmap = FALSE;

		    status = read_one( m, new_map,
			   		(DMPP_PAGE **)&fmap_ptr, dberr);
		    if ( status != E_DB_OK )
	    		break;

		    read_fmap = TRUE;
		}
		else
		{
		    /* Error */
		    break;
		}
	}

	if ( status != E_DB_OK )
	    break;

	/*  
	** Write current FMAP page. 
	*/
	status = write_one(m, (DMPP_PAGE *)fmap_ptr, dberr);
	if (status != E_DB_OK)
	    break;
	
	read_fmap = FALSE;

	/* 
	** Write the FHDR
	*/
	status = write_one(m, (DMPP_PAGE *)fhdr_ptr, dberr);
	if (status != E_DB_OK)
	    break;
	
	read_fhdr = FALSE;

    } while ( FALSE );

    if (misc_pages)
	dm0m_deallocate((DM_OBJECT **)&misc_pages);

    if ( status == E_DB_OK )
	return( E_DB_OK );

    /*
    ** Error condition
    */
    if (read_fmap || read_fhdr)
    {
	if ( read_fmap )
	    s = write_one(m, (DMPP_PAGE *)fmap_ptr, &local_dberr);

	if ( read_fhdr )
	    s = write_one(m, (DMPP_PAGE *)fhdr_ptr, &local_dberr);
    }

    dm1x_log_error(E_DM92E6_DM1X_FREE, dberr);
    return (status);
}

DB_STATUS
dm1xbput(
DM2U_M_CONTEXT	*mct,
DMPP_PAGE       *data,
char		*record,
i4         	rec_size,
i4		record_type,
i4		fill,
DM_TID		*tid,
u_i2		current_table_version,
DB_ERROR	*dberr)
{
    DB_STATUS		status;
    i4			max;
    i4			rem;
    i4			orig_size;
    i4			first_seg_size;
    DMPP_SEG_HDR	load_seg_hdr;
    DMPP_SEG_HDR	*seg_hdr;
    char		*seg;
    i4			seg_cnt;
    i4			nextpage;
    DM_TID		segtid;

    CLRDBERR(dberr);

    if (mct->mct_seg_rows == 0 ||
	record_type == DM1C_LOAD_ISAMINDEX)
    {
	return (*mct->mct_acc_plv->dmpp_load)(mct->mct_page_type,
		mct->mct_page_size, data, record, rec_size,
		record_type, fill, tid, current_table_version,
		(DMPP_SEG_HDR *)NULL);
    }

    seg_cnt = 0;
    seg_hdr = &load_seg_hdr;
    orig_size = rec_size;

    /* Determine size of 1st segment */
    max = dm2u_maxreclen(mct->mct_page_type, mct->mct_page_size);
    rem = rec_size % max;
    if (rem)
	rec_size = rem;
    else
	rec_size = max;
    first_seg_size = rec_size;
    seg_hdr->seg_len = first_seg_size;

    /*
    ** Find out next page number we will allocate so that we
    ** can put it into seg_hdr
    */
    if (orig_size > max)
    {
	status = dm1xnextpage(mct, &nextpage, dberr);
	if (status != E_DB_OK)
	{
	    dm1x_log_error(E_DM924D_DM1S_BPUT, dberr);
	    return (status);
	}
	seg_hdr->seg_next = nextpage;
    }
    else
	seg_hdr->seg_next = 0;

   status = (*mct->mct_acc_plv->dmpp_load)(mct->mct_page_type,
		mct->mct_page_size, data, record, max,
		record_type, fill, tid, current_table_version, seg_hdr);

#ifdef xDEBUG
    if (status == E_DB_OK)
	TRdisplay("dm1x load seg 1 page %d seg_hdr len %d next %d\n",
	    DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, data),
	    seg_hdr->seg_len, seg_hdr->seg_next);
#endif

    if (status == E_DB_OK && seg_hdr && seg_hdr->seg_next != 0)
    {
	seg_cnt = 1;
	seg_hdr->seg_len = max;
	for (seg = record + first_seg_size; ;seg += max)
	{
	    seg_cnt++;

	    status = dm1xnewpage(mct, 0, &mct->mct_curseg, dberr);
	    if (status != E_DB_OK)
	    {
		dm1x_log_error(E_DM924D_DM1S_BPUT, dberr);
		return (status);
	    }

	    /* FIX ME Consistency check, page number == seg_hdr->seg_next */

	    /*  Set the page status. */
	    DMPP_VPT_INIT_PAGE_STAT_MACRO(mct->mct_page_type, 
			mct->mct_curseg, DMPP_DATA);

	    /* Init DMPP_SEG_HDR for this segment */
	    if (seg + max >= record + orig_size)
		seg_hdr->seg_next = 0;
	    else
	    {
		status = dm1xnextpage(mct, &nextpage, dberr);
		if (status != E_DB_OK)
		{
		    dm1x_log_error(E_DM924D_DM1S_BPUT, dberr);
		    return (status);
		}
		seg_hdr->seg_next = nextpage;
	    }

#ifdef xDEBUG
	    TRdisplay("dm1x - load seg %d page %d seg_hdr len %d next %d\n",
		seg_cnt,
		DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type, mct->mct_curseg),
		seg_hdr->seg_len, seg_hdr->seg_next);
#endif

	    /* Caller does not want segment tid */
	    status = (*mct->mct_acc_plv->dmpp_load)(mct->mct_page_type,
		mct->mct_page_size, mct->mct_curseg, seg, max,
		record_type, fill, &segtid, current_table_version, seg_hdr);

	    if (status != E_DB_OK)
	    {
		dm1x_log_error(E_DM924D_DM1S_BPUT, dberr);
		return (status);
	    }

	    if (!seg_hdr->seg_next)
		break;
	}
    }

    return (status);
}

DB_STATUS
dm1x_get_segs(
DM2U_M_CONTEXT	*mct,
DMPP_SEG_HDR	*first_seg_hdr,
char		*first_seg,
char		*record,
DB_ERROR	*dberr)
{
    DMPP_PAGE		*segpage = NULL;
    DMPP_SEG_HDR	seg_hdr;
    DM_TID		seg_tid;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		local_stat;
    char		*seg;
    char		*rec;
    char		*endrec;
    i4			record_size;
    i4			seg_num;

    CLRDBERR(dberr);

    /* More segments for this row to get */
    rec = record;
    endrec = record + mct->mct_relwid;

#ifdef xDEBUG
    TRdisplay("dm1x_get_segs 1st seg (seghdr %d,%d) \n", 
	first_seg_hdr->seg_len, first_seg_hdr->seg_next);
#endif

    /* Process first segment */
    STRUCT_ASSIGN_MACRO(*first_seg_hdr, seg_hdr);
    MEcopy(first_seg, first_seg_hdr->seg_len, rec);
    rec += seg_hdr.seg_len;

    for (seg_num = 2; seg_hdr.seg_next; seg_num++)
    {
	status = dm1xreadpage(mct, DM1X_FORREAD, seg_hdr.seg_next,
			&mct->mct_curseg, dberr);
	if (status != E_DB_OK)
	    break;

	seg_tid.tid_tid.tid_page = seg_hdr.seg_next;
	seg_tid.tid_tid.tid_line = 0;

	status = (*mct->mct_acc_plv->dmpp_get)(mct->mct_page_type,
	    mct->mct_page_size, mct->mct_curseg, &seg_tid, 
	    &record_size, &seg, NULL, NULL, NULL, &seg_hdr);
	if (status != E_DB_OK)
	{
	    SETDBERR(dberr, 0, E_DM938B_INCONSISTENT_ROW);
	    break;
	}

#ifdef xDEBUG
	TRdisplay("dm1x_get_segs seg %d (seghdr %d,%d) tid (%d,%d)\n", 
	    seg_num, seg_hdr.seg_len, seg_hdr.seg_next,
	    seg_tid.tid_tid.tid_page, seg_tid.tid_tid.tid_line);
#endif

	/* consistency check before MEcopy */
	if (rec + seg_hdr.seg_len > record + mct->mct_relwid)
	{
	    SETDBERR(dberr, 0, E_DM938B_INCONSISTENT_ROW);
	    status = E_DB_ERROR;
	    break;
	}

	MEcopy(seg, seg_hdr.seg_len, rec);
	rec += seg_hdr.seg_len;
    }

    return (status);
}
