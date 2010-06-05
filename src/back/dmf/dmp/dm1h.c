/*
** Copyright (c) 1986, 2010 Ingres Corporation
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
#include    <dmf.h>
#include    <adf.h>
#include    <ulf.h>
#include    <hshcrc.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpl.h>
#include    <dmpp.h>
#include    <dm1p.h>
#include    <dm1c.h>
#include    <dm1h.h>
#include    <dm1r.h>
#include    <dm0p.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm2f.h>
#include    <dm0m.h>
#include    <dmccb.h>
#include    <dmxcb.h>
#include    <dml.h>
#include    <dmxe.h>

/**
**
**  Name: DM1H.C - Routines to allocate and search HASHed tables.
**
**  Description:
**      This file contains the access method specific routines needed to
**      acess HASHed tables.  Thes routines use the ADF routines
**      for datatypes.
**
**      The routines defined in this file are:
**      dm1h_allocate       - Allocates space for a new record.
**      dm1h_search         - Sets the search limits for a scan.
**      dm1h_newhash        - New Hash function.
**
**  History:
**      06-jan-86 (jennifer)
**          Changed for Jupiter.
**      29-oct-87 (jennifer)
**          Added new hash function. 
**      30-sep-1988 (rogerk)
**          Changed file extends to put new page at top of overflow chain.
**          Added DMPP_FULLCHAIN optimizations.
**      01-nov-88 (sandyh)
**          Added check for correct hashing function to use.
**      24-Apr-89 (anton)
**          Added local collation support
**      29-may-89 (rogerk)
**          Check return status from dm1c_get calls.
**	15-aug-1990 (bryanp)
**	    Fixed typo in check of return status from a dm0p_fix_page() call.
**	19-nov-90 (rogerk)
**	    Changed to use the rcb's adf control block rather than allocating
**	    local adf cb's off of the stack.  This was done as part of the DBMS
**	    timezone changes.
**	14-jun-1991 (Derek)
**	    Use new page allocation routines.  Change allocate to just
**	    return TID that can be used for new record, and not to
**	    change the page in any way.
**      15-oct-1991 (mikem)
**          Eliminated unix compile warnings dm1h_allocate().
**      23-oct-1991 (jnash)
**          Within dm1h_allocate, eliminate unused parameter in call 
**	    to dm0p_bicheck.
**	24-oct-1991 (bryanp)
**	    System catalog recovery problems. Fixed the REDO case for system
**	    catalog extend recovery, and added comments for the UNDO case (needs
**	    to be fixed still).
**	25-oct-1991 (bryanp)
**	    More system catalog recovery problems. We must call dm1p_force_map
**	    to force the FMAP page for the newly allocated overflow page before
**	    ending the mini-transaction. Also, the logging should only be done
**	    if rcb_logging is set.
**	 3-nov-191 (rogerk)
**	    Added dm0l_ovfl log record during dm1h_allocate when a new overflow
**	    page is added.  This log record is used to REDO the operation of
**	    linking the new page into the overflow chain.
**      20-Nov-1991 (rmuth)
**          Added DM1P_MINI_TRAN, when inside a mini-transaction pass this
**          flag to dm1p_getfree() so it does not allocate a page deallocated
**          earlier by this transaction. See dm1p.c for explanation why.
**	    Corrected the code to pass the correct flags to dm1p_getfree().
**	 7-jul-1992 (rogerk)
**	    Prototyping DMF.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	14-oct-1992 (jnash)
**	    Reduced logging project.
**	      - New parameters to dmpp_allocate.
**	      - New parameters to dmpp_get.
**	      - Move calls to compression routines up to this level.  dmpp_get
**		now always returns a pointer to the record.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Changed arguments to dm0p_mutex and
**	    dm0p_unmutex calls.
**	26-oct-92 (rickh)
**	    Replaced references to DMP_ATTS with DB_ATTS.  Also replaced
**	    DM_CMP_LIST with DB_CMP_LIST.
**	14-dec-1992 (jnash & rogerk)
**	    Reduced logging project:
**	      - Changed to use new recovery protocols.  Before Images are
**		no longer written and the pages updated by the allocate must
**		be mutexed while the dm0l_ovfl log record is written and
**		the pages are updated.
**	      - Changed to pass new arguments to dm0l_ovfl.
**	      - Add new param's to dmpp_allocate call.
**	      - Add new param's to dmpp_get call.
**	      - Move calls to compression routines up to this level.  dmpp_get
**		now always returns a pointer to the record.
**	      - Removed requirement that file-extend operations on system
**		catalogs must be forced to the database.  Restriction not
**		needed anymore since system catalogs now do Redo Recovery.
**	      - Changed arguments to dm0p_mutex and dm0p_unmutex calls.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	02-jan-1993 (jnash)
**	    More reduced logging work.  Add LGreserve calls.
**	18-jan-1993 (bryanp)
**	    In dm1h_nofull, don't set local_fix to TRUE until fix_page succeeds.
**	08-feb-1993 (rmuth)
**	    If allocate a new page set the page_stat bit DMPP_OVFL as all
**	    pages added after table build time are overflow pages.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed Begin and End Mini log records to use LSN's rather than LAs.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Issue error message E_DM9247_DM1H_ALLOCATE upon error.
**		In dm1h_nofull, if this is a core system catalog, unfix the
**		    overflow page before fixing the primary page, to ensure that
**		    we comply with the cluster node failure recovery rule that
**		    no transaction may fix more than one syscat page at a time.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	 3-jan-1994 (rogerk)
**	    Forced core catalog pages during file extend operations to ensure
**	    that the pages would be physically consistent in the event of 
**	    a crash to ensure that the database can be opened for recovery.
**	    Otherwise, the opendb attempt may fail when the system catalogs
**	    are read.  Changed for bug #56896.
**	    Added cast on assignment of hashtab pointer to quiet the
**	    solaris compiler.  Apparently it things that the hashtab array
**	    and the pointer "t" below are of different types because the
**	    array is declared READONLY.
**	31-jan-1994 (jnash)
**	    Remove #ifndef xDEBUG that generate error messages when 
**	    main page locked.  This is expected.
**      10-apr-1995 (chech02) Bug# 64830 (merged the fixes from 6.4 by nick)
**          unfix any page passed via data before fixing the required page
**          calculated via the hashing algorithm.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass tcb_rel.relpgsize as page_size argument to dmpp_allocate.
**	06-mar-1996 (stial01 for bryanp)
**	    Don't allocate tuple buffers on the stack.
**      06-mar-1996 (stial01)
**          dm1h_hash(), dm1h_newhash() Only alloc tuple buffers if needed.
**          dm1h_allocate() Postpone tuple buffer alloc until needed.
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**      18-jul-1996 (ramra01 for bryanp)
**          For Alter Table support, check for row versions, pass row version
**          to dmpp_uncompress, and call dm1r_cvt_row if necessary.
**      24-jul-96 (nick)
**          Use DM1P_PHYSICAL in calls to dm1p_getfree() for system
**          catalogs.
**	13-sep-1996 (canor01)
**	    Add rcb_tupbuf to dmpp_uncompress call to pass temporary buffer
**	    for uncompression.  Use the temporary buffer for uncompression
**	    when available.
**	16-sep-1996 (canor01)
**	    Pass tuple buffer to dm1h_hash() and dm1h_newhash() instead
**	    of thrashing the dmf memory allocator.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Add key value locking for phantom protection
**          dm1h_allocate(): Set DM0L_ROW_LOCK flag if row locking is enabled.
**          dm1h_allocate(): Call dm1r_allocate() instead of dmpp_allocate
**          dm1h_allocate(): If row locking, extend file in a mini transaction.
**          Added dm1h_keyhash() 
**      12-dec-96 (dilma04)
**          dm1h_allocate: do not lock the key value in case of unique key;
**          dm1h_search: add check for row_locking when locking a value.
**      06-jan-97 (dilma04)
**          In dm1h_allocate() DM1P_PHYSICAL flag is needed only when calling 
**          dm1p_getfree() to allocate system catalog page.
**      27-jan-97 (dilma04)
**          dm1h_search(): do not need phantom protection if searching to
**          update secondary index.  
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Use macro to check for serializable transaction.
**      10-mar-97 (stial01)
**          dm1h_allocate() Pass relwid to dm0m_tballoc()
**          dm1h_allocate() Pass record to dm1r_allocate
**      21-may-97 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**          No more LK_PH_PAGE locks, buffer locked with dm0p_lock_buf_macro()
**          dm1h_search() Skip FULLCHAIN optimization if row locking.
**      29-may-97 (stial01)
**          dm1h_allocate() If row locking user table, we do not need to force
**          the page, in fact it would cause a buffer manager protocol error.
**      24-jun-97 (dilma04)
**          Do not acquire value lock for phantom protection in dm1h_allocate()
**          if no duplicates are allowed, since it has been already acquired 
**          for duplicate checking before calling that routine.
**      29-jul-97 (stial01)
**          dm1h_nofull() does not get called when row locking.
**      21-apr-98 (stial01)
**          Set DM0L_PHYS_LOCK in log rec if extension table (B90677)
**          Extend table in mini transaction if extension table
**      13-Nov-98 (stial01)
**          Renamed DM1C_SI_UPDATE -> DM1C_FIND_SI_UPD
**	01-dec-1998 (nanpr01)
**	    Get rid of DMPP_TUPLE_INFO structure. Use DMPP_TUPLE_HDR instead.
**      7-jan-98 (stial01)
**          Put ATTRIBUTE buffer for adc_hashprep back on stack.
**          Removed buffer parameter for dm1h_hash, dm1h_newhash, dm1h_keyhash.
**	11-Jan-1999 (shero03)
**	    Moved CRC-32 algorithm to glf/hsh
**      05-may-1999 (nanpr01,stial01)
**          Key value locks acquired for duplicate checking are no longer held 
**          for duration of transaction. While duplicate checking, request/wait
**          for row lock if uncommitted duplicate key is found.
**	16-aug-1999 (nanpr01)
**	    Implementation of SIR 92299. Enabaling journalling on a
**	    online checkpoint.
**	21-oct-1999 (nanpr01)
**	    More CPU optimization. We should not copy tuple header when not 
**	    needed.
**	04-May-2000 (jenjo02)
**	    Pass blank-suppressed lengths of table/owner names to 
**	    dm0l_nofull, dm0l_ovfl.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**      30-oct-2000 (stial01)
**          Pass crecord to dm1h_allocate, Pass rcb to dm1r_allocate
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Arg changes for page accessors (Variable Page Type SIR 102955)
**	11-jun-2001 (somsa01)
**	    In dm1h_hash(), to save stack space, we now dynamically
**	    allocate space for the temp buffer.
**      10-jul-2003 (stial01)
**          Fixed row lock error handling during allocate (b110521)
**      15-sep-2003 (stial01)
**          Skip allocate mini trans for etabs if table locking (b110923)
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      25-jun-2004 (stial01)
**          Modularize get processing for compressed, altered, or segmented rows
**      30-jun-2004 stial01
**          Fixed dm1c_get arg
**	16-dec-04 (inkdo01)
**	    Add various collID's.
**      31-jan-05 (stial01)
**          Added flags arg to dm0p_mutex call(s).
**	22-Feb-2008 (kschendel)
**	    Minor changes for new rowaccessor scheme.
**	11-Apr-2008 (kschendel)
**	    RCB's adfcb now typed as such, minor fixes here.
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	02-Dec-2008 (jonj)
**	    SIR 120874: dm1r_?, dm1h_? functions converted to DB_ERROR *, use new
**	    form uleFormat
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1p_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm1c_?, dm0p_? functions converted to DB_ERROR *
**      12-dec-2008 (joea)
**          Replace BYTE_SWAP by BIG/LITTLE_ENDIAN_INT.
**	25-Aug-2009 (kschendel) 121804
**	    Need dmxe.h to satisfy gcc 4.3.
**	12-Nov-2009 (kschendel) SIR 122882
**	    De-support 6.0 tables, delete the old hash algorithm.
**      24-nov-2009 (stial01)
**          Check if table has segmented rows (instead of if pagetype supports)
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace DMPP_PAGE* with DMP_PINFO* where needed.
**	    New macros to lock/unlock pages, LG_LRI replaces LG_LSN,
**	    sensitized to crow_locking().
*/

/*{
** Name: dm1h_allocate - Allocates space for a new record. 
**
** Description:
**    This routine finds space for a new record, allocates it, and 
**    returns the tid.  The page is not changes in this routine
**    since its before image has to be logged before changes can
**    be make.  This routine will first try to allocate space
**    on the page that is fixed before looking on another page
**    in the overflow chain.
**    This optimization is only used if duplicates are not being
**    checked and the table is not marked as unique.  Otherwise
**    the entire chain must be read meaning that saving one 
**    unfix page is not important.
**
**    If the DMPP_FULLCHAIN bit is set on the main page, then only
**    the main page and first overflow page will be checked for
**    free space.  If space for the new record is not found there
**    then we will allocate a new page.  If the DMPP_FULLCHAIN bit is
**    not set then we will search to the end of the chain looking for
**    free space.
**
**    When a new page is allocated, it is inserted at the top of the
**    overflow chain.
**
**	When a new page is allocated, there are special code paths for system
**	catalog page allocations. System catalog extends are performed in
**	mini transactions. The page allocation is specially logged so that it
**	is NOT redone by REDO recovery (system catalog operations are not
**	redone, so we can't REDO the alloc). Once the mini-transaction
**	completes, the page allocation is not undone by UNDO recovery, even
**	if the transaction causing the extend aborts.
**
**	There is currently a window in the extend code for system catalogs
**	which can cause the system catalog to become corrupted if a crash occurs
**	at just the right time. When that window is removed, this comment should
**	be removed as well (bryan, 10/91).
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      record                          Pointer to record to insert.
**      crecord				Compressed record (if compression)
**      need                            Value indicating amount of space
**                                      needed, assumes compressed if necessary.
**      dupflag                         A flag indicating if duplicates must be
**                                      checked.  Must be DM1C_DUPLICATES or
**                                      DM1C_NODUPLICATES.
**
** Outputs:
**      data                            Pointer to an area used to return
**                                      pointer to the data page.
**      tid                             Pointer to an area to return TID
**                                      of record allocated.
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**                                      Error codes returned are:
**                                      E_DM003C_BAD_TID, E_DM0044_DELETED_TID.
**					E_DM9247_DM1H_ALLOCATE
**      
**      Returns:
**
**          E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
**      Exceptions:
**          none
**
** Side Effects:
**          none.
**
** History:
**      06-jan-86 (jennifer)
**          Converted for Jupiter.
**      31-aug-88 (rogerk)
**          Fix bug where we compared main_page to relprim instead of relprim-1.
**          Relprim specifies # of buckets and range is 0 to (relprim-1).
**      31-aug-88 (rogerk)
**          Chain new hash ovfl pages to top of ovfl chain instead of bottom.
**          Use/Set CHAINFULL status bit on primary page to determine if rest
**          of overflow chain should be searched for free space.
**       7-nov-88 (rogerk)
**          Send compression type to dm1c_get() routine.
**      24-Apr-89 (anton)
**          Added local collation support
**      29-may-89 (rogerk)
**          Check return status from dm1c_get calls.
**	15-aug-1990 (bryanp)
**	    Fixed typo in check of return status from a dm0p_fix_page() call.
**	19-nov-90 (rogerk)
**	    Changed to use the rcb's adf control block rather than allocating
**	    local adf cb's off of the stack.  This was done as part of the DBMS
**	    timezone changes.
**      15-oct-1991 (mikem)
**          Eliminated unix compile warnings ("warning: statement not reached")
**	    by changing the way "for (;xxx == E_DB_OK;)" loops were coded.
**      23-oct-1991 (jnash)
**          Eliminate unused parameter in call to dm0p_bicheck.
**	24-oct-1991 (bryanp)
**	    System catalog recovery problems. Fixed the REDO case for system
**	    catalog extend recovery, and added comments for the UNDO case (needs
**	    to be fixed still).
**	24-oct-1991 (rogerk)
**	    Added checks for rcb_logging around calls to dm0l_bm/dm0l_em.
**	    This would break Rollforward recovery and Set Nologging.
**	25-oct-1991 (bryanp)
**	    More system catalog recovery problems. We must call dm1p_force_map
**	    to force the FMAP page for the newly allocated overflow page before
**	    ending the mini-transaction.
**	 3-nov-191 (rogerk)
**	    Added dm0l_ovfl log record during dm1h_allocate when a new overflow
**	    page is added.  This log record is used to REDO the operation of
**	    linking the new page into the overflow chain.
**      20-Nov-1991 (rmuth)
**          If this is a system relation pass DM1P_NOREDO and DM1P_MINITRAN
**          do not need to pass DM1P_PHYSICAL as this is set in th
**          rcb's 'rcb_k_duration' field.
**	08-Jun-1992 (kwatts)
**	    6.5 MPF project. Replaced dm1c_allocate, dmpp_get_offset_macros,
**	    and dm1c_get calls with accessor calls. Note the get_offset 
**	    goes completely, instead we check the response from the get
**	    accessor.
**	14-oct-1992 (jnash)
**	    Reduced logging project.
**	      - New parameters to dmpp_allocate.
**	      - New parameters to dmpp_get.
**	      - Move calls to compression routines up to this level.  dmpp_get
**		now always returns a pointer to the record.
**	      - Change type = to == in setting of rcb_hash_nofull.
**	05-nov-1992 (jnash)
**	    Temp hack of "in recovery" flag passed to dmpp_allocate, 
**	    to bypass row locks during recovery.  Will be removed when
**	    new recovery code is in place.	
**	05-nov-1992 (jnash & rogerk)
**	    Reduced logging project:
**	      - Changed to use new recovery protocols.  Before Images are
**		no longer written and the pages updated by the allocate must
**		be mutexed while the dm0l_ovfl log record is written and
**		the pages are updated.
**	      - Changed to pass new arguments to dm0l_ovfl.
**	      - Add new param's to dmpp_allocate call.
**	      - Add new param's to dmpp_get call.
**	      - Move calls to compression routines up to this level.  dmpp_get
**		now always returns a pointer to the record.
**	      - Removed requirement that file-extend operations on system
**		catalogs must be forced to the database.  Restriction not
**		needed anymore since system catalogs now do Redo Recovery.
**	      - Changed arguments to dm0p_mutex and dm0p_unmutex calls.
**	      - Change = to == in setting of rcb_hash_nofull.
**	      - Took out description of recovery problem with system catalogs
**		as those problems are resolved by new recovery methods.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project (phase 2):
**	      - Added dm0p_pagetype call.
**	      - Removed old DM1P_NOREDO action - all allocates are now redone.
**	05-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**	08-feb-1993 (rmuth)
**	    If allocate a new page set the page_stat bit DMPP_OVFL as all
**	    pages added after table build time are overflow pages.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed Begin and End Mini log records to use LSN's rather than LAs.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Issue error message E_DM9247_DM1H_ALLOCATE upon error.
**	26-jul-1993 (rogerk)
**	    Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() call.
**	 3-jan-1994 (rogerk)
**	    Forced core catalog pages during file extend operations to ensure
**	    that the pages would be physically consistent in the event of 
**	    a crash to ensure that the database can be opened for recovery.
**	    Otherwise, the opendb attempt may fail when the system catalogs
**	    are read.  This is a result of the fact that we currently do a
**	    logical open of the database in order to recover it.  This makes
**	    it difficult to recover core catalog data. Changed for bug #56896.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass tcb_rel.relpgsize as page_size argument to dmpp_allocate.
**	06-mar-1996 (stial01 for bryanp)
**	    Don't allocate tuple buffers on the stack.
**	06-may-1996 (nanpr01)
**	    Change all the page header access as macro for New Page Format
**	    project.
**      20-may-1996 (ramra01)
**	    Added DMPP_TUPLE_INFO arg to the get accessor routine 
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**      18-jul-1996 (ramra01 for bryanp)
**          For Alter Table support, check for row versions, pass row version
**          to dmpp_uncompress, and call dm1r_cvt_row if necessary.
**      24-jul-96 (nick)
**          Pass DM1P_PHYSICAL to dm1p_getfree() for system catalogs.
**	13-sep-1996 (canor01)
**	    Add rcb_tupbuf to dmpp_uncompress call to pass temporary buffer
**	    for uncompression.  Use the temporary buffer for uncompression
**	    when available.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Add key value locking for phantom protection
**          Set DM0L_ROW_LOCK flag if row locking is enabled.
**          Call dm1r_allocate() instead of dmpp_allocate
**          If row locking, extend file in a mini transaction.
**      12-dec-96 (dilma04)
**          Optimization: Do not lock the key value in case of unique key,
**          because the lock should have been already acquired during duplicate
**          checking.
**      06-jan-97 (dilma04)
**          DM1P_PHYSICAL flag is needed only when calling dm1p_getfree() to
**          allocate system catalog page.
**      10-mar-97 (stial01)
**          Pass relwid to dm0m_tballoc(), Pass record to dm1r_allocate
**      21-may-97 (stial01)
**          dm1h_allocate() Added flags arg to dm0p_unmutex call(s).
**          No more LK_PH_PAGE locks, buffer locked with dm0p_lock_buf_macro()
**          Note we look at page_page,page_main when the buffer is not locked,
**          since they are only set when the page is formatted.
**      29-may-97 (stial01)
**          dm1h_allocate() If row locking user table, we do not need to force
**          the page, in fact it would cause a buffer manager protocol error.
**      24-jun-97 (dilma04)
**          Do not acquire value lock for phantom protection if no duplicates
**          are allowed, since it has been already acquired for duplicate 
**          checking before calling this routine. 
**	11-aug-97 (nanpr01)
**	    Add the t->tcb_comp_atts_count with relwid in dm0m_tballoc call.
**      21-apr-98 (stial01)
**          dm1h_allocate() Set DM0L_PHYS_LOCK if extension table (B90677)
**          Also, extend table in mini transaction if extension table
**      22-sep-98 (stial01)
**          dm1h_allocate() If row locking, downgrade/convert lock on new page  
**          after mini transaction is done (B92909)
**	11-Feb-1999 (shero03)
**	    Change keyhash routine to not hash the key values.
**      18-feb-99 (stial01)
**          dm1h_allocate() pass buf_locked to dm1r_allocate
**      05-may-1999 (nanpr01,stial01)
**          dm1h_allocate() Key value locks acquired for dupcheck are no longer 
**          held for duration of transaction. While duplicate checking,
**          request/wait for row lock if uncommitted duplicate key is found.
**	13-aug-99 (stephenb)
**	    alter uncompress calls to new proto.
**      28-oct-99 (stial01)
**          If lock row fails don't break with buf_locked set, buf not locked
**	24-Dec-2003 (jenjo02)
**	    Add support for Global Indexes.
**      16-mar-2004 (gupsh01)
**          Modified dm1r_cvt_row to include adf control block in parameter
**          list.
**	10-nov-06 (kibro01) bug 116910
**	    If dm1h_allocate fails in generating an overflow page (such as
**	    when deadlock occurs during rowlock escalation) the error
**	    DM934D_DM0P_BAD_PAGE was generated due to an attempt to unfix the
**	    page twice - once for newpage and once for *data, since the two
**	    were the same.
**	5-Dec-2007 (kibro01) b119563
**	    Don't unfix the buffer if we want it for allocating a new record.  Keep
**	    track of our saved page while we search the overflow chain for dups.
**	3-Jun-2009 (kibro01) b122113
**	    Allocate rcb_tupbuf regardless of compression, since some page types
**	    (e.g. HAS_SEGMENTS) needs it set as well before dup-checking occurs.
**      21-may-2009 (huazh01)
**          dm1h_hash(), dm1h_newhash(), dm1h_keyhash() now includes
**          DMP_RCB and err_code in the parameter list. (b122075)
**      11-jun-2009 (huazh01)
**          call ule_format() with correct parameter when reporting
**          E_DM9245 error. (b122075)
**      10-sep-2009 (stial01)
**          Don't use string compare for relcmptlvl comparisons
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: dm1r_crow_lock() when crow_locking()
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Use NeedPhysLock(r) to determine physical page lock needs.
**	22-Apr-2010 (kschendel) SIR 123485
**	    Process coupons if doing full row duplicate checking and there
**	    are blobs, otherwise dmpe now complains about lack of context.
*/
DB_STATUS
dm1h_allocate(
DMP_RCB        *rcb,
DMP_PINFO      *pinfo,
DM_TID         *tid,
char           *record,
char	       *crecord,
i4        need,
i4        dupflag,
DB_ERROR       *dberr)
{
    DMP_RCB     *r = rcb;                                       
    DMP_TCB     *t = r->rcb_tcb_ptr;
    DMP_DCB     *d = t->tcb_dcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    i4     compare;
    DM_TID      localtid;
    DB_STATUS   s = E_DB_OK, get_status;
    DB_STATUS	local_status;
    STATUS  	cl_status;
    i4     local_err_code;                                  
    DM_PAGENO   mainpageno,pageno,ovflpageno;
    DM_PAGENO   spacepage;
#define             NO_SPACE_FOUND  0xffffffffL
    i4     i;
    ADF_CB      *adf_cb;
    i4          search_chain;
    char        *record2;
    char	*rec_ptr;
    i4	rec_size;
    i4	loc_id;
    i4	loc_config_id;
    i4	new_loc_id;
    i4	new_config_id;
    i4	dm0l_flag;
    i4		row_version = 0;		
    u_i4	row_low_tran = 0;
    u_i2	row_lg_id;
    CL_ERR_DESC	sys_err;
    i4     bucket;
    bool        in_mini_trans = FALSE;
    i4		new_lock;
    i4		flags;
    LK_LKID     new_lkid;
    DM_TID      tid_to_lock;
    DM_TID8	tid8;
    bool	did_lock;
    bool        must_lock = FALSE;
    LK_LOCK_KEY save_lock_key;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;
    LG_LRI	lri;
    DMP_PINFO	newPinfo;
    DMP_PINFO	savePinfo;

    CLRDBERR(dberr);

    DMP_PINIT(&newPinfo);
    DMP_PINIT(&savePinfo);
 
    /*
    ** Compute the hash key.
    **
    ** Use the old hashing algorithm only if the compatibility
    ** level is less than "6.0a" (DMF_S1_VERSION). (sandyh)
    */

    s = dm1h_newhash(rcb, t->tcb_key_atts, t->tcb_keys, record,
                    (i4)t->tcb_rel.relprim, &bucket, dberr); 

    if (s)
    {
       if (*err_code == E_DM9425_MEMORY_ALLOCATE) 
       {
          uleFormat(NULL, E_DM9425_MEMORY_ALLOCATE, (CL_ERR_DESC *)NULL,
                  ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                  &local_err_code, 1, 0, HASH_BUFLEN);
       }
       SETDBERR(dberr, 0, E_DM9247_DM1H_ALLOCATE);
       return (E_DB_ERROR); 
    }

    pageno = bucket;

    if ((t->tcb_rel.relstat & TCB_UNIQUE) == 0 &&
        (dupflag == DM1C_DUPLICATES) && pinfo->page)
    {
        /*
        **  Duplicates are allowed, key is not unique,
        **  see if rcb has any pages fixed.  Now see if this is the 
        **  correct page and if it has any room, if it isn't the correct
        **  page or there is no room just release and use normal code. 
        */

        mainpageno = pageno + 1;
        if (pageno == t->tcb_rel.relprim - 1)
            mainpageno = 0;

        if (DMPP_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype, pinfo->page) == 
	    mainpageno)
        {
	    /*
	    ** Lock buffer for update.
	    **
	    ** This will swap from CR->Root if pinfo->page
	    ** is a CR page.
	    */
	    dm0pLockBufForUpd(r, pinfo);

            /* The pages match, see if there is room. */
	    s = dm1r_allocate(r, pinfo,
			t->tcb_rel.relcomptype == TCB_C_NONE ? record : crecord,
			need, tid, dberr);

	    /* Unlock buffer, restoring CR page pointer */
	    dm0pUnlockBuf(r, pinfo);

            if (s == E_DB_OK || s == E_DB_ERROR)
	    {
                return (s);
	    }
        }
    }

    if (pinfo->page)
    {
        s = dm0p_unfix_page(rcb, DM0P_UNFIX, pinfo, dberr);
    }
    if (s == E_DB_OK)
    {
	i4     fix_action = DM0P_READ;

	if (rcb->rcb_access_mode == RCB_A_WRITE)
	    fix_action = DM0P_WRITE;
	s = dm0p_fix_page(rcb, pageno, fix_action | DM0P_NOREADAHEAD,
            pinfo, dberr);
    }
    if (s != E_DB_OK)
    {
        if (dberr->err_code >= E_DM_INTERNAL)
        {
            uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM9247_DM1H_ALLOCATE);
        }
        return (E_DB_ERROR);
    }

    if (r->rcb_tupbuf == NULL)
    {
        r->rcb_tupbuf = dm0m_tballoc((t->tcb_rel.relwid +
					t->tcb_data_rac.worstcase_expansion));
	if (r->rcb_tupbuf == NULL)
	{
	    SETDBERR(dberr, 0, E_DM923D_DM0M_NOMORE);
	    return (E_DB_ERROR);
	}
    }
    record2 = r->rcb_tupbuf;

    /*
    ** Until we need to look for space on a page,
    ** we only need to read-share lock the buffers.
    */
    dm0pLockRootForGet(r, pinfo);

    /*
    ** Check for duplicates and space.
    ** Don't search overflow chain for free space if fullchain bit is set
    ** in the main page.
    */
    search_chain = TRUE;
    if (DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, pinfo->page) & 
	DMPP_FULLCHAIN)
    {
        if (dupflag == DM1C_DUPLICATES)
            search_chain = FALSE;
    }

    spacepage = NO_SPACE_FOUND;
    adf_cb = r->rcb_adf_cb;
    localtid.tid_tid.tid_page = pageno;
    localtid.tid_tid.tid_line = DM_TIDEOF;
    for (;;)
    {
	/* In case we fixed a overflow page, the local tid needs to be reset */
    	localtid.tid_tid.tid_page = pageno;
        if (dupflag != DM1C_DUPLICATES)
        {
            rec_size = t->tcb_rel.relwid;

            for (i = 0; i < (i4) 
		 DMPP_VPT_GET_PAGE_NEXT_LINE_MACRO(t->tcb_rel.relpgtype, pinfo->page);
		 i++)
            {
		did_lock = FALSE;
		if (must_lock)
		{
		    /* rechecking an uncommitted row, wait for the lock */
		    dm0pUnlockBuf(r, pinfo);

		    if ( crow_locking(r) )
		        s = dm1r_crow_lock(r, LK_PHYSICAL, &tid_to_lock, NULL, dberr);
		    else
		    {
			/* don't clear lock coupling in rcb */
			flags = DM1R_ALLOC_TID | DM1R_LK_PHYSICAL; 
			s = dm1r_lock_row(r, flags, &tid_to_lock,
						&new_lock, &save_lock_key,
						dberr);
		    }

		    dm0pLockRootForGet(r, pinfo);

		    if ( s != E_DB_OK )
		        break;

		    if ( row_locking(r) )
		    {
			/* Check status after relocking the buffer... */
			_VOID_ dm1r_unlock_row(r, &save_lock_key, &local_dberr);
		    }

		    must_lock = FALSE;
		    did_lock = TRUE;
		}

                localtid.tid_tid.tid_line = i;
        	s = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype,
		    t->tcb_rel.relpgsize, pinfo->page, &localtid, &rec_size,
		    &rec_ptr, &row_version, &row_low_tran, 
		    &row_lg_id, (DMPP_SEG_HDR *)0);
		get_status = s;

		if (s == E_DB_WARN)
		{
		    s = E_DB_OK;
		    if (!rec_ptr ||
			DMPP_SKIP_DELETED_ROW_MACRO(r, pinfo->page, row_lg_id, row_low_tran))
			continue;
		}

		/* Additional processing if compressed, altered, or segmented */
		if (s == E_DB_OK && 
		    (t->tcb_data_rac.compression_type != TCB_C_NONE || 
		    row_version != t->tcb_rel.relversion ||
		    t->tcb_seg_rows))
		{
		    s = dm1c_get(r, pinfo->page, &localtid, record2, dberr);
		    rec_ptr = record2;
		}
		if (s == E_DB_OK && t->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS)
		{
		    /* Apply context to short-term part of coupon in row
		    ** that we're checking against.
		    */
		    if (rec_ptr != record2)
		    {
			MEcopy(rec_ptr, t->tcb_rel.relwid, record2);
			rec_ptr = record2;
		    }
		    s = dm1c_pget(t->tcb_atts_ptr, r, rec_ptr, dberr);
		}

		if (s == E_DB_ERROR)
		{
                    dm1c_badtid(r, &localtid);
		    SETDBERR(dberr, 0, E_DM938B_INCONSISTENT_ROW);
                    break;
		}

		/* If dup row checking (base tables only), we may need lock */
                if ((t->tcb_rel.relstat & TCB_UNIQUE) == 0
		    && DMPP_DUPCHECK_NEED_ROW_LOCK_MACRO(r, pinfo->page, row_lg_id, row_low_tran)
		    && !did_lock)
                {
		    s = adt_ktktcmp(adf_cb, (i4)t->tcb_keys,
				t->tcb_key_atts, record, rec_ptr, &compare);
		    /* Don't lock or dup row check if key doesn't match */
		    if (s == E_DB_OK && compare != 0)
			continue;
		    tid_to_lock.tid_i4 = localtid.tid_i4;
		    must_lock = TRUE;
		    i--;
		    continue; /* recheck the row */
		}

                if ((t->tcb_rel.relstat & TCB_UNIQUE) == 0)
                {
		    s = adt_tupcmp(adf_cb, (i4)t->tcb_rel.relatts,
                            t->tcb_atts_ptr, record, rec_ptr, &compare);
                    if (s == E_DB_OK)
                    {
			if (compare == 0)
			{
			    s = E_DB_ERROR;
			    SETDBERR(dberr, 0, E_DM0046_DUPLICATE_RECORD);
			    break;
			}
			continue;
		    }
		}
		else
		{
		    s = adt_ktktcmp(adf_cb, (i4)t->tcb_keys,
				t->tcb_key_atts, record, rec_ptr, &compare);
		    if (s == E_DB_OK)
		    {
			if (compare != 0)
			    continue;

			/* Same key */
			if (DMPP_DUPCHECK_NEED_ROW_LOCK_MACRO(r, pinfo->page, 
						row_lg_id, row_low_tran) 
			    && !did_lock)
			{
			    if (t->tcb_rel.relstat & TCB_INDEX)
			    {
				/* Extract the TID out of the secondary index */
				if ( t->tcb_rel.relstat2 & TCB2_GLOBAL_INDEX )
				{
				    MEcopy(
				    (char *)(rec_ptr + rec_size - sizeof(DM_TID8)),
					sizeof(DM_TID8), &tid8);
				    tid_to_lock.tid_i4 = tid8.tid_i4.tid;
				    /* dm1r_lock_row needs this */
				    r->rcb_partno = tid8.tid.tid_partno;
				}
				else
				{
				    MEcopy(
				    (char *)(rec_ptr + rec_size - sizeof(DM_TID)),
					sizeof(DM_TID), &tid_to_lock.tid_i4);
				}
			    }
			    else
				tid_to_lock.tid_i4 = localtid.tid_i4;

			    must_lock = TRUE;
			    i--;
			    continue; /* recheck the row */
			}
			else if (get_status == E_DB_WARN)
			    continue;

			s = E_DB_ERROR;
			SETDBERR(dberr, 0, E_DM0045_DUPLICATE_KEY);
			break;
		    }
		}               
		dmd_check(E_DMF012_ADT_FAIL);
            }

            /* If duplicate found, finished. */

            if (dberr->err_code != E_DB_OK)
                break;
        }       
        
        /*
        **  Check for free space on this page if we aren't just checking for
        **  duplicates and we have yet to find a page with space.
        */

        if (dupflag != DM1C_ONLYDUPCHECK)
        {
            if (spacepage == NO_SPACE_FOUND)
            {
		/* Now read-excl lock the buffer */
		dm0pLockBufForUpd(r, pinfo);

		s = dm1r_allocate(r, pinfo, 
			t->tcb_rel.relcomptype == TCB_C_NONE ? record : crecord,
			need, tid, dberr);
		if (s == E_DB_ERROR)
		    break;

                if (s == E_DB_OK)
                {
                    spacepage = DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
							 pinfo->page);        
		    savePinfo = *pinfo;
                    if (dupflag == DM1C_DUPLICATES)
                        break;
                }
                s = E_DB_OK;
            }
        }

	/* Get ovfl page while the buffer is locked */
	ovflpageno = 
	    DMPP_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, pinfo->page);

        /*
        ** If reached last page in chain, or we have already checked the
        ** first overflow chain for space and the DMPP_FULLCHAIN bit was
        ** set on the main page, then quit.
        */

	if ((ovflpageno == 0)
			|| ((search_chain == FALSE) &&
                (DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, pinfo->page) >= 
			t->tcb_rel.relprim)))
        {
            break;
        }

        /* Otherwise, get next overflow chain, keep checking. */
	/* Don't unfix this page if it's the one we want for allocation */
        if (spacepage != DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, 
						      pinfo->page))
	{
	    /* NB: unfix_page also releases the lock on the buffer */
            s = dm0p_unfix_page(r, DM0P_UNFIX, pinfo, dberr);
        }
        pageno = ovflpageno;
        if (s == E_DB_OK)
            s = dm0p_fix_page(r, pageno, DM0P_WRITE | DM0P_NOREADAHEAD,
                pinfo, dberr);
        if (s != E_DB_OK)
            break;

	dm0pLockRootForGet(r, pinfo);
    }

    for (;;)
    {
	if (s != E_DB_OK)
	    break;

        if (dupflag == DM1C_ONLYDUPCHECK)
        {
	    /* NB: unfix also release the lock on the buffer */
            s = dm0p_unfix_page(r, DM0P_UNFIX, pinfo, dberr);
            if (s == E_DB_OK)
                return (s);
        }
        else if (spacepage != NO_SPACE_FOUND)
        {
            if (spacepage != DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, 
						      pinfo->page))
            {
                s = dm0p_unfix_page(r, DM0P_UNFIX, pinfo, dberr);
		/* Just reset *data to be our saved page now */
		*pinfo = savePinfo;
                if (s != E_DB_OK)
		{
		    /* Also unfix the saved page */
		    (void) dm0p_unfix_page(r, DM0P_UNFIX, pinfo, &local_dberr);
                    return (s);
		}
            }   
	    else
	        dm0pUnlockBuf(r, pinfo);
        }
        else if (spacepage == NO_SPACE_FOUND)
        {
            LG_LSN	    lsn, bm_lsn;
	    i4	    dm1p_flag = 0;

            /*
            ** Extend the relation by chaining a new overflow page off
            ** of the main page.  If there are already overflow pages,
            ** then this new overflow page becomes the first overflow page
            ** and all others will be chained off of it.
            **
            ** After adding a new overflow page, set the DMPP_FULLCHAIN status
            ** in the main page that indicates that there is no free space
            ** in the overflow chain.  Also, make sure that rcb_hash_nofull
            ** does not point to this chain since it is now full.
            */

	    /*
	    ** Make sure we have the correct parent page fixed.  If our current
	    ** page is an overflow page, then find and fix the main bucket page.
	    */
            if (DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, pinfo->page) >= 
			t->tcb_rel.relprim)
            {
                /*
                ** Page_main points to next bucket, so parent page is at
                ** (next bucket - 1).  If the parent is the last bucket then
                ** page_main is zero.
                */

                mainpageno = t->tcb_rel.relprim - 1;
                if (DMPP_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype, pinfo->page))
                    mainpageno = DMPP_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype,
							  pinfo->page) - 1;
                s = dm0p_unfix_page(rcb, DM0P_UNFIX, pinfo, dberr);

                if (s == E_DB_OK)
                {
                    s = dm0p_fix_page(r, mainpageno,
                        (DM0P_WRITE | DM0P_NOREADAHEAD), pinfo, dberr);
                }
                if (s != E_DB_OK)
                    break;
            }

	    dm0pLockBufForUpd(r, pinfo);

            if (r->rcb_hash_nofull == 
		     DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, pinfo->page))
                r->rcb_hash_nofull = -1;

	    /*
	    ** Allocations for system catalogs are always done within Mini
	    ** Transactions since the page locks will be released as soon as
	    ** the current update is complete.
	    */
	    if ( row_locking(r) || crow_locking(r) || NeedPhysLock(r) )
            {
		if ( r->rcb_logging )
		{
		    s = dm0l_bm(r, &bm_lsn, dberr);
		    if (s != E_DB_OK)
			break;
		    in_mini_trans = TRUE;
		}

		/*
		** Set flags to allocate call to indicate that the action is
		** part of a mini transaction.
		*/
		dm1p_flag |= DM1P_MINI_TRAN;

		if ( NeedPhysLock(r) )
                    dm1p_flag |= DM1P_PHYSICAL;
            }

	    /*
	    ** Call allocation code to get a free page from the free maps.
	    ** This will possibly trigger a File Extend as well.
	    **
	    ** The newpage's buffer might be pinned by dm1p_getfree, and
	    ** will be unpinned by dm0p_unfix_page.
	    */
            s = dm1p_getfree(r, dm1p_flag, &newPinfo, dberr);
            if (s != E_DB_OK)
                break;

	    /* Save lockid for new page */
	    STRUCT_ASSIGN_MACRO(r->rcb_fix_lkid, new_lkid);

	    /* Inform buffer manager of new page's type */
	    dm0p_pagetype(tbio, newPinfo.page, r->rcb_log_id, DMPP_DATA);

	    /*
	    ** Online Backup Protocols: Check if BIs must be logged first.
	    */
	    if (d->dcb_status & DCB_S_BACKUP)
	    {
	        s = dm0p_before_image_check(r, newPinfo.page, dberr);
	        if (s != E_DB_OK)
		    break;
	        s = dm0p_before_image_check(r, pinfo->page, dberr);
	        if (s != E_DB_OK)
		    break;
	    }

	    /*
	    ** Get information on the locations of the updates for the
	    ** log call below.
	    */
	    loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, 
			(i4) DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
							   pinfo->page));
	    loc_config_id = tbio->tbio_location_array[loc_id].loc_config_id;

	    new_loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, 
			(i4) DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
							   newPinfo.page));
	    new_config_id = tbio->tbio_location_array[loc_id].loc_config_id;

	    /*
	    ** Log operation of adding new page on the overflow chain.
	    ** The Log Address of the OVERFLOW record is recorded on both
	    ** the new and old data pages.
	    */
	    if (r->rcb_logging)
	    {
		/* Reserve space for overflow and its clr */
		cl_status = LGreserve(0, r->rcb_log_id, 2, 
		    sizeof(DM0L_OVFL) * 2, &sys_err);
		if (cl_status != OK)
		{
		    (VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)0, 0L, (i4 *)0, err_code, 0);
		    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			ULE_LOG, NULL, (char *)0, 0L, (i4 *)0,
			err_code, 1, 0, r->rcb_log_id);
		    SETDBERR(dberr, 0, E_DM9247_DM1H_ALLOCATE);
		    s = E_DB_ERROR;
		    break;
		}

		/*
		** Mutex the buffers so nobody can look at them while they are 
		** changing.
		*/
		dm0pMutex(tbio, (i4)0, r->rcb_lk_id, pinfo);
		dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &newPinfo);

		/* 
	 	** Include indication that we are setting the 
		** FULLCHAIN status. It will used by recovery to
		** recover the state of the bit.
		*/
		dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);

		if ( r->rcb_state & RCB_IN_MINI_TRAN )
		    dm0l_flag |= DM0L_MINI;

		/* 
		** We use temporary physical locks for catalogs and extension
		** tables. The same protocol must be observed during recovery.
		*/
		if ( NeedPhysLock(r) )
		    dm0l_flag |= DM0L_PHYS_LOCK;
                else if (row_locking(r))
                    dm0l_flag |= DM0L_ROW_LOCK;
		else if ( crow_locking(r) )
		    dm0l_flag |= DM0L_CROW_LOCK;

		/* Get CR info from datapage, pass to dm0l_ovfl */
		DMPP_VPT_GET_PAGE_LRI_MACRO(t->tcb_rel.relpgtype, pinfo->page, &lri);

		s = dm0l_ovfl(r->rcb_log_id, dm0l_flag,
			TRUE, &t->tcb_rel.reltid,
			&t->tcb_rel.relid, t->tcb_relid_len,
			&t->tcb_rel.relowner, t->tcb_relowner_len,
			t->tcb_rel.relpgtype, 
			t->tcb_rel.relpgsize,
			t->tcb_rel.relloccount, loc_config_id, 
			new_config_id, 
			DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, newPinfo.page),
			DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,pinfo->page),
			DMPP_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype,pinfo->page),
			DMPP_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype,pinfo->page),
			(LG_LSN *)0, &lri, dberr);
		if (s != E_DB_OK)
		{
		    dm0pUnmutex(tbio, 0, r->rcb_lk_id, pinfo);
		    dm0pUnmutex(tbio, 0, r->rcb_lk_id, &newPinfo);
		    break;
		}

		/* Store log info about this change on datapage */
		DMPP_VPT_SET_PAGE_LRI_MACRO(t->tcb_rel.relpgtype, pinfo->page, &lri);
		/*
		** "newpage" is new and has no page update history, so just 
		** record the ovfl lsn, leaving the rest of the LRI information
		** on the page zero.
		*/
		DMPP_VPT_SET_PAGE_LOG_ADDR_MACRO(t->tcb_rel.relpgtype, newPinfo.page, lri.lri_lsn);
	    }
	    else
	    {
		dm0pMutex(tbio, (i4)0, r->rcb_lk_id, pinfo);
		dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &newPinfo);
	    }

            /*  Mark new page as on same overflow chain. */

            DMPP_VPT_SET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype, newPinfo.page, 
				DMPP_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype,
							 pinfo->page));
            DMPP_VPT_SET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, newPinfo.page, 
				DMPP_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype,
							 pinfo->page));
            DMPP_VPT_SET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, newPinfo.page, 
					(DMPP_MODIFY | DMPP_OVFL));

            /*  Link old page to new page. */

            DMPP_VPT_SET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, pinfo->page, 
				DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
							 newPinfo.page));
            DMPP_VPT_SET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, pinfo->page,
					(DMPP_MODIFY | DMPP_FULLCHAIN));

	    /*
	    ** Unmutex the buffers after completing the updates.
	    */
	    dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, pinfo);
	    dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &newPinfo);

	    /*
	    ** If this is a system catalog allocation, then complete the
	    ** Mini Transaction used for the allocate.  After this, the
	    ** new page becomes a permanent addition to the table.
	    **
	    ** We must force the pages updated in an overflow operation of a
	    ** core system catalog. System catalogs must remain physically
	    ** consistent at all times because if a crash occurs the database
	    ** must be opened BEFORE recovery can be done; and opening the
	    ** database requires reading the core catalogs.
	    ** 
	    ** If row locking user table, we do not need to force the page,
	    ** in fact it would cause a buffer manager protocol error.
	    */
            if (t->tcb_sysrel == TCB_SYSTEM_REL)
            {
		s = dm0p_unfix_page(r, DM0P_FORCE, pinfo, dberr);
		if (s != E_DB_OK)
		    break;
		s = dm0p_unfix_page(r, DM0P_FORCE, &newPinfo, dberr);
		if (s != E_DB_OK)
		    break;
	    }

	    /*
	    ** Now end the system catalog mini transaction
	    */
	    if (in_mini_trans)
	    {
		s = dm0l_em(r, &bm_lsn, &lsn, dberr);
		if (s != E_DB_OK)
		    break;
	    }

	    spacepage = 
		    DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, newPinfo.page);

            /*  Unfix/unlock oldpage and replace with new page. */
            s = dm0p_unfix_page(r, DM0P_UNFIX, pinfo, dberr);
            if (s != E_DB_OK)
		break;

	    *pinfo = newPinfo;
	    DMP_PINIT(&newPinfo);

	    /*
	    ** Allocate space on new page while we still hold X lock
	    ** and pin on newpage
	    */
	    s = dm1r_allocate(r, pinfo,
			t->tcb_rel.relcomptype == TCB_C_NONE ? record : crecord,
			need, tid, dberr);

	    /*
	    ** Even if allocate failed, do the lock conversion below in
	    ** case we need to rollback 
	    **
	    ** Note dm1r_lk_convert will also remove the pin from newpage.
	    */
	    if ( row_locking(r) || crow_locking(r) )
	    {
		local_status = dm1r_lk_convert(r, 
				DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, pinfo->page),
				pinfo,
				&new_lkid, &local_dberr);
			
		if (local_status != E_DB_OK)
		{
		    if (s == E_DB_OK)
		    {
			s = local_status;
			*dberr = local_dberr;
		    }
		    else
		    {
			uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
				    &local_err_code, 0);
		    }
		}
	    }

	    if (s != E_DB_OK)
		break;
        }

#ifdef xDEBUG
	if (s != E_DB_OK)
	{
	     TRdisplay("dm1h_allocate: dmpp_allocate failure: %d\n", s);
	}
#endif

        return (s);
    }

    dm0pUnlockBuf(r, pinfo);

    /*  Handle error reporting. */

    /* Don't leave page fixed in local buffer */

    if (pinfo->page)
    {
	/* Bug 116910 (kibro01) - If the new page is an overflow page, newpage
	** will be the same as *data, and we don't want to unfix it twice
	*/
	if (pinfo->page == newPinfo.page) 
	    DMP_PINIT(&newPinfo);
	if (pinfo->page == savePinfo.page) 
	    DMP_PINIT(&savePinfo);

        s = dm0p_unfix_page(r, DM0P_UNFIX, pinfo, &local_dberr);
    }
    if (newPinfo.page)
        s = dm0p_unfix_page(r, DM0P_UNFIX, &newPinfo, &local_dberr);
    if (savePinfo.page)
        s = dm0p_unfix_page(r, DM0P_UNFIX, &savePinfo, &local_dberr);

    if (dberr->err_code >= E_DM_INTERNAL)
    {
        uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9247_DM1H_ALLOCATE);
    }

    return (E_DB_ERROR);
}

/*{
** Name: dm1h_search - Sets the scan limits for a HASHED table.
**
** Description:
**      This routine sets the serach limits for a scan of a HASHed table.
**      The low and high tids for the scan are place in the RCB.
**      These are then used to determine which records to retrieve when 
**      get next is called.  This routine will leave the data page fixed.
**
**      If we fix a primary page that does not have DMPP_FULLCHAIN status
**      then set rcb_hash_nofull.  This is an optimization that keeps us
**      from doing unnecessary work if we delete a tuple from this chain.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      tuple                           Pointer to the tuple.
**      mode                            Must be set to indicate type of
**                                      position desired.
**      direction                       A valindicating the direction of
**                                      the search.  Must be DM1C_PREV for
**                                      first or DM1C_NEXT for last or 
**                                      DM1C_EXACT for page position.
**
** Outputs:
**      tid                             A pointer to an area to return
**                                      the tid meeting the search criteria.
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**                                      Error codes returned are:
**                                      E_DM0055_NONEXT, E_DM003C_BAD_TID,
**                                      E_DB0044_DELETED_TID.
**                    
**      Returns:
**
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
**      06-jan-86 (jennifer)
**          Converted for Jupiter.
**      30-sep-1988 (rogerk)
**          When read in a main page, check the DMPP_CHAINFULL bit.  If it
**          is not set, then put this bucket in rcb_hash_nofull.
**          If a delete is done on this hash chain, then we know we do not
**          have to turn off the CHAINFULL bit on the primary page.
**      10-apl-1995 (chech02) Bug# 64830 (merged the fixes from 6.4 by nick)
**          unfix any page passed via data before fixing the required page
**          calculated via the hashing algorithm.
**	06-may-1996 (nanpr01)
**	    Change all the page header access as macro for New Page Format
**	    project.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Add key value locking for phantom protection.
**      12-dec-96 (dilma04)
**          Add check for row_locking when locking a value.
**      27-jan-97 (dilma04)
**          Do not need phantom protection if searching to update secondary
**          index.                               
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Use macro to check for serializable transaction.
**      21-may-97 (stial01)
**          dm1h_search() Skip FULLCHAIN optimization if row locking. 
**      21-may-2009 (huazh01)
**          dm1h_hash(), dm1h_newhash(), dm1h_keyhash() now includes
**          DMP_RCB and err_code in the parameter list. (b122075)
**      11-jun-2009 (huazh01)
**          call ule_format() with correct parameter when reporting
**          E_DM9245 error. (b122075)
*/
DB_STATUS
dm1h_search(
DMP_RCB         *rcb,
DMP_PINFO       *pinfo,
char            *tuple,
i4         mode,
i4         direction,
DM_TID          *tid,
DB_ERROR        *dberr)
{
    DMP_TCB             *t = rcb->rcb_tcb_ptr;
    i4             fix_action;
    DM_PAGENO           pageno;    
    DB_STATUS           s;
    i4		    *err_code = &dberr->err_code;
    i4                  local_err_code;

    CLRDBERR(dberr);

    if (mode == DM1C_EXTREME)
    {
        tid->tid_i4 = -1;
        if (direction == DM1C_PREV)
        {   
            tid->tid_tid.tid_page = 0;
            tid->tid_tid.tid_line = DM_TIDEOF;
        }
        return (E_DB_OK);
    }
   
    /*
    ** Compute the hash key.
    **
    ** Use the old hashing algorithm only if the compatibility
    ** level is less than "6.0a" (DMF_S1_VERSION). (sandyh)
    */

    s = dm1h_newhash(rcb, t->tcb_key_atts, t->tcb_keys, tuple,
                    (i4)t->tcb_rel.relprim, &pageno, dberr); 

    if (s)
    {
       if (*err_code == E_DM9425_MEMORY_ALLOCATE)
       {
           uleFormat(NULL, E_DM9425_MEMORY_ALLOCATE, (CL_ERR_DESC *)NULL,
                     ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                     &local_err_code, 1, 0, HASH_BUFLEN);
       }
       SETDBERR(dberr, 0, E_DM9248_DM1H_SEARCH);
       return (E_DB_ERROR);
    }

    if (pinfo && pinfo->page)
    {
# ifdef XDEBUG
	TRdisplay("Data page fixed in call to dm1h_search(), unfixing ...\n");
# endif
	s = dm0p_unfix_page(rcb, DM0P_UNFIX, pinfo, dberr);
	if (s)
	    return(s);
    }

    tid->tid_tid.tid_page = pageno;
    tid->tid_tid.tid_line = DM_TIDEOF;
    fix_action = DM0P_READ;
    if (rcb->rcb_access_mode == RCB_A_WRITE)
	fix_action = DM0P_WRITE;
    s = dm0p_fix_page(rcb, pageno, fix_action | DM0P_NOREADAHEAD,
        pinfo, dberr);
    if (s == E_DB_OK)
    {
        /*
        ** Check for NOSEARCH bit on main page.  If set, record the fact in
        ** in the rcb so that if a delete is done on this chain, the bit will
        ** be reset.
	**
	** When (c)row locking we are not doing this optimization since
	** space is not reclaimed by a delete, only by a page clean.
        */
	if ( !row_locking(rcb) && !crow_locking(rcb) &&
            (DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, pinfo->page) & 
		DMPP_FULLCHAIN) == 0)
            rcb->rcb_hash_nofull = pageno;

        return (s);
    }

    /*  Handle error reporting. */

    if (dberr->err_code >= E_DM_INTERNAL)
    {
        uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9248_DM1H_SEARCH);
    }
    return (E_DB_ERROR);
}

/*{
** Name: dm1h_newhash   - The new hashing function.
**
** Description:
**      This function takes a list of field descriptions and applies the
**      new hashing function to compute the bucket address of where the record
**      should live.
**
**      This routine computes a hash function for a variable length byte
**      stream.  A AUTODIN-II CRC polynomial is computed for the byte
**      stream then this value is returned modulo the number of hash buckets.
**
**      The initial value of the polynomial is the negative of the length.
**
**      The number of hash buckets need not be a relatively prime number. 
**      
**      The CRC is computed using a precomputed table of results using
**      a byte at a time.
**
** Inputs:
**      atts                            Pointer to array of pointers 
**                                      to keyed attributes.
**      count                           The number of keyed attributes
**      tuple                           Pointer to the tuple.
**
** Outputs:
**          bucket                      The bucket number computed 
**                                      by the hash function.
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      29-oct-87 (jennifer) 
**          Created for Jupiter.
**	06-mar-1996 (stial01 for bryanp)
**	    Don't allocate tuple buffers on the stack.
**      10-mar-97 (stial01)
**          dvto.db_length can be set to attribute length.
**          Pass size to dm0m_tballoc
**      7-jan-98 (stial01)
**          dm1h_newhash() Put ATTRIBUTE buffer for adc_hashprep back on stack.
**          Removed buffer parameter.
**	11-Jan-1999 (shero03)
**	    Moved CRC-32 algorithm to glf/hsh
**	17-Apr-2008 (kibro01) b120276
**	    Initialise ADF_CB structure
**      21-may-2009 (huazh01)
**          Allocate hash buffer using MEreqmem(). (b122075)
*/

DB_STATUS
dm1h_newhash(
DMP_RCB           *rcb,
DB_ATTS           **att,
i4            count,
char               *tuple,
i4            buckets, 
i4            *hash_val, 
DB_ERROR       *dberr)
{
    char                *k;
    i4                  i;
    i4                  length;
    u_i4           init;
    ADF_CB              adf_scb;
    DB_DATA_VALUE       dvfrom;
    DB_DATA_VALUE       dvto;
    STATUS              local_stat = OK; 

    init = -1;
    MEfill(sizeof(ADF_CB),0,(PTR)&adf_scb);
    adf_scb.adf_maxstring = DB_MAXSTRING;

    if (rcb->rcb_hashbuf == NULL)
    {
       rcb->rcb_hashbuf = (char *)MEreqmem(0, HASH_BUFLEN,
                    FALSE, &local_stat);
       if (rcb->rcb_hashbuf == NULL || local_stat)
       {
          SETDBERR(dberr, 0, E_DM9425_MEMORY_ALLOCATE);
          *hash_val = -1; 
          return (E_DB_ERROR);
       }
    }
  
    for (i = 0; i < count; i++)
    {
        k = tuple + att[i]->offset;
        switch (att[i]->type)
        {
        case ATT_INT:
        case ATT_FLT:
        case ATT_MNY:
        case ATT_CHA:
            length = att[i]->length;
            break;

        default:
            dvfrom.db_datatype = att[i]->type;
            dvfrom.db_length = att[i]->length;
            dvfrom.db_prec = att[i]->precision;
            dvfrom.db_data = k;
            dvfrom.db_collID = att[i]->collID;
            dvto.db_length = HASH_BUFLEN;
	    dvto.db_data = rcb->rcb_hashbuf;
            adc_hashprep(&adf_scb, &dvfrom, &dvto);
            length = dvto.db_length;
            k = dvto.db_data;
            break;
        }
    
        /*  Process the next attribute. */

	HSH_CRC32(k, length, &init);
    }

    *hash_val = (init % buckets);
    return (E_DB_OK); 

}

/*{
** Name: dm1h_nofull    - Mark overflow chain as being not full.
**
** Description:
**      This routine is called after a tuple has been deleted on a Hash
**      overflow chain.  If the chain was marked as full - indicated by
**      the DMPP_FULLCHAIN bit on the primary page - then that bit should
**      now be turned off.
**
**      The primary page, indicated by 'bucket', is read in and if the
**      DMPP_FULLCHAIN bit is on, then it is turned off.  There is no
**      logging done, therefore the bit is not guarenteed to be reset
**      if the transaction aborts.
**
**      NOTE that the DMPP_FULLCHAIN bit is used for performance reasons
**      only.  The integrity of the hash table should remain intact if
**      even if the bit is asserted when the chain is not full, or not
**      asserted when the chain is full.
**
**      In order to avoid deadlock situations, if the primary page is locked
**      by another transaction when requested here, this routine will return
**      without updating the page.
**
**      Unexpected errors in this routine will be logged, but will not prevent
**      this transaction from continuing - they will only prevent the primary
**      page from being updated.
**
**	If this is a core system catalog, we may need to unfix and then refix
**	the overflow chain page so that we comply with the cluster node failure
**	recovery rule that no transaction may fix more than one core system
**	catalog page at a time.
**
** Inputs:
**      rcb         - record control block
**      bucket      - primary page of overflow chain
**
** Outputs:
**      Returns:
**          none
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**       5-oct-1988 (rogerk)
**          Written for Hash Overflow optimizations.
**	18-oct-1992 (jnash)
**	    6.5 Recovery project.  Write fullchain log record to 
**	    permit recovery of fullchain status.  Note that this
**	    log actually records "nofull", with a bit in the
**	    overflow log record recording "full".
**	05-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**	18-jan-1993 (bryanp)
**	    In dm1h_nofull, don't set local_fix to TRUE until fix_page succeeds.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		In dm1h_nofull, if this is a core system catalog, unfix the
**		    overflow page before fixing the primary page, to ensure that
**		    we comply with the cluster node failure recovery rule that
**		    no transaction may fix more than one syscat page at a time.
**		Added a missing "break" at the end of the for loop.
**		It bothers me that this routine is "VOID".
**	26-jul-1993 (rogerk)
**	    Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() call.
**	31-jan-1994 (jnash)
**	    Remove #ifndef xDEBUG that generate error messages when 
**	    main page locked.  This is expected.
**	06-may-1996 (nanpr01)
**	    Change all the page header access as macro for New Page Format
**	    project.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Set DM0L_ROW_LOCK flag if row locking is enabled.
**      21-may-97 (stial01)
**          dm1h_nofull() Added flags arg to dm0p_unmutex call(s).
**      29-jul-97 (stial01)
**          dm1h_nofull() does not get called when row locking.
**      21-apr-98 (stial01)
**          dm1h_nofull() Set DM0L_PHYS_LOCK if extension table (B90677)
**	09-Jan-2009 (jonj)
**	    Fix up mishandling of error/local_dberr.
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Use NeedPhysLock(r) to determine physical page lock needs.
*/
VOID
dm1h_nofull(
DMP_RCB     *rcb,
DM_PAGENO   bucket)
{
    DMP_TCB     	*t = rcb->rcb_tcb_ptr;
    DMP_DCB     	*d = t->tcb_dcb_ptr;
    DMP_TABLE_IO    	*tbio = &t->tcb_table_io;
    DMP_PINFO		local_pinfo;
    DM_PAGENO		overflow_page_no;
    i4			must_refix_overflow_page = 0;
    DMPP_PAGE   	*page;
    DB_STATUS   	status = E_DB_OK;
    STATUS   		cl_status;
    CL_ERR_DESC		sys_err;
    i4     	local_fix = FALSE;
    i4		loc_id;
    i4		loc_config_id;
    i4		dm0l_flag;
    LG_LRI		lri;
    DB_ERROR		local_dberr;
    i4			*err_code = &local_dberr.err_code;

    CLRDBERR(&local_dberr);

    local_pinfo = rcb->rcb_data;
    page = rcb->rcb_data.page;

    for (;;)
    {
	/*
	** If (c)row locking we don't check/update the FULLCHAIN status,
	** since deletes do not reclaim space and there may be
	** concurrent activity on the page.
	*/
	if (row_locking(rcb) || crow_locking(rcb))
	    break;

	/*
	** Make sure we have the right main page.  If not, fix the main
	** page locally.  We request the page NOLOCK to avoid deadlock problems
	** if another transaction is holding the main page and is trying to
	** traverse the overflow chain.  If we are not granted the main page
	** lock, then we just return without updating the FULLCHAIN state.
	**
	** Once we successfully fix the page, set local_fix to ensure that we'll
	** unfix it before leaving this routine. Note that the fix_page call may
	** fail (deadlock, busy, whatever), so we must wait til after it
	** succeeds to set local_fix, else we'll erroneously try to unfix a page
	** we didn't fix in the first place.
	*/
	if ( NeedPhysLock(rcb) )
	{
	    if (page != 0 && 
		DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, page) != bucket)
	    {
		overflow_page_no = 
			DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, page);
		status = dm0p_unfix_page(rcb, DM0P_UNFIX,
					&rcb->rcb_data, &local_dberr);
		if (status != E_DB_OK)
		    break;
		must_refix_overflow_page = 1;
	    }
	}

	if ((page == 0) || 
	    (DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, page)!= bucket))
	{
	    local_pinfo.page = page;
	    status = dm0p_fix_page(rcb, bucket, 
		(i4)(DM0P_WRITE | DM0P_NOWAIT), &local_pinfo, &local_dberr);
	    if (status != E_DB_OK)
		break;
	    local_fix = TRUE;
	}

	/*
	** If the page does not have the FULLCHAIN bit asserted, then there
	** is no work required.
	*/
	if ( ! (DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, page) & 
		DMPP_FULLCHAIN))
	    break;

	/*
	** Online Backup Protocols: Check if BI must be logged before update.
	*/
	if (d->dcb_status & DCB_S_BACKUP)
	{
	    status = dm0p_before_image_check(rcb, page, &local_dberr);
	    if (status != E_DB_OK)
	        break;
	}

	/*
	** Get information on the location of the update for the log call below.
	*/
	loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, 
		(i4) DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, page));
	loc_config_id = tbio->tbio_location_array[loc_id].loc_config_id;

	/*
	** Log the operation of unsetting the fullchain bit.
	** Write the LSN of the log record to the main page.
	*/
	if (rcb->rcb_logging)
	{
	    /* Reserve logfile space for log and its clr */
	    cl_status = LGreserve(0, rcb->rcb_log_id, 2, 
	 	sizeof(DM0L_NOFULL) * 2, &sys_err);
	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		    (char *)0, 0L, (i4 *)0, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
		    ULE_LOG, NULL, (char *)0, 0L, (i4 *)0,
		    err_code, 1, 0, rcb->rcb_log_id);
		SETDBERR(&local_dberr, 0, E_DM9349_DM1H_NOFULL);
		status = E_DB_ERROR;
		break;
	    }

	    /*
	    ** Lock the buffer so nobody else can look at it while we change it.
	    */
	    dm0pMutex(tbio, (i4)0, rcb->rcb_lk_id, &local_pinfo);

	    dm0l_flag = (is_journal(rcb) ? DM0L_JOURNAL : 0);

	    if ( rcb->rcb_state & RCB_IN_MINI_TRAN )
	        dm0l_flag |= DM0L_MINI;

	    /* Get CR info from page, pass to dm0l_nofull */
	    DMPP_VPT_GET_PAGE_LRI_MACRO(t->tcb_rel.relpgtype, page, &lri);

	    /* 
	    ** We use temporary physical locks for catalogs and extension
	    ** tables. The same protocol must be observed during recovery.
	    */
	    if ( NeedPhysLock(rcb) )
		dm0l_flag |= DM0L_PHYS_LOCK;
            else if (row_locking(rcb))
                dm0l_flag |= DM0L_ROW_LOCK;
	    else if ( crow_locking(rcb) )
                dm0l_flag |= DM0L_CROW_LOCK;

	    status = dm0l_nofull(rcb->rcb_log_id, 
		dm0l_flag, &t->tcb_rel.reltid,
		&t->tcb_rel.relid, t->tcb_relid_len,
		&t->tcb_rel.relowner, t->tcb_relowner_len,
		t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		(i4)DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, page), 
		loc_config_id, t->tcb_rel.relloccount, 
		(LG_LSN *)0, &lri, &local_dberr);
	    if (status != E_DB_OK)
	    {
		dm0pUnmutex(tbio, (i4)0, rcb->rcb_lk_id, &local_pinfo);
		break;
	    }

	    /* Store log info about this change on the page */
	    DMPP_VPT_SET_PAGE_LRI_MACRO(t->tcb_rel.relpgtype, page, &lri);
	}
	else
	    dm0pMutex(tbio, (i4)0, rcb->rcb_lk_id, &local_pinfo);

	/*
	** Turn off the DMPP_FULLCHAIN bit in the page status and mark the
	** page as dirty.
	*/
        DMPP_VPT_CLR_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, page, DMPP_FULLCHAIN);
        DMPP_VPT_SET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, page, DMPP_MODIFY);

	/*
	** Unlock the buffer after completing the update.
	*/
	dm0pUnmutex(tbio, (i4)0, rcb->rcb_lk_id, &local_pinfo);

	/*
	** Set rcb_hash_nofull to this bucket.  If we delete another tuple
	** from this buckets overflow chain we won't need to turn off the
	** FULLCHAIN bit.
	*/
	rcb->rcb_hash_nofull = bucket;

	break;
    }

    /*
    ** If we failed to update the FULLCHAIN status, log the error.
    ** Again, this is not treated as a fatal error, we just don't udpate
    ** the FULLCHAIN hint.
    **
    ** Failure to lock the main page is not logged as it is an expected error.
    */
    if (status != E_DB_OK)
    {
	if (local_dberr.err_code != E_DM004C_LOCK_RESOURCE_BUSY)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)0,
		(i4)0, (i4 *)0, err_code, 0);
	    uleFormat(NULL, E_DM9349_DM1H_NOFULL, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)0, (i4)0, (i4 *)0, err_code, 0);
	}
    }

    /*
    ** Unfix the main page if we fixed it locally.
    */
    if (local_fix)
    {
        status = dm0p_unfix_page(rcb, DM0P_UNFIX, &local_pinfo, &local_dberr);
        if (status != E_DB_OK)
        {
            uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)0,
                (i4)0, (i4 *)0, err_code, 0);
            uleFormat(NULL, E_DM9349_DM1H_NOFULL, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
                (char *)0, (i4)0, (i4 *)0, err_code, 0);
        }
    }

    if (must_refix_overflow_page)
    {
	status = dm0p_fix_page(rcb, overflow_page_no, 
				DM0P_WRITE, &rcb->rcb_data, &local_dberr);
	if (status != E_DB_OK)
        {
            uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)0,
                (i4)0, (i4 *)0, err_code, 0);
            uleFormat(NULL, E_DM9349_DM1H_NOFULL, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
                (char *)0, (i4)0, (i4 *)0, err_code, 0);
        }
    }

    return;
}


/*{
** Name: dm1h_keyhash    - Hash key to value.
**
** Description:
**      This routine hashes a key to a value.
**
** Inputs:
**      att
**      count
**      tuple
**
** Outputs:
**      key4
**      key5
**      key6
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Created.
**      10-mar-97 (stial01)
**          dvto.db_length can be set to attribute length.
**          New buffer param.
**      7-jan-98 (stial01)
**          dm1h_keyhash() Put ATTRIBUTE buffer for adc_hashprep back on stack.
**          Removed buffer parameter.
**	11-Jan-1999 (shero03)
**	    Moved CRC-32 algorithm to glf/hsh
**	11-Feb-1999 (shero03)
**	    Change routine to fold, but not hash the key values.
**	17-Apr-2008 (kibro01) b120276
**	    Initialise ADF_CB structure
**      21-may-2009 (huazh01)
**          Allocate hash buffer using MEreqmem(). (b122075)
*/

#define LK_KEY_WORDS 3

DB_STATUS
dm1h_keyhash(
DMP_RCB           *rcb, 
DB_ATTS           **att,
i4            count,
char               *tuple,
i4                 *key4,
i4                 *key5,
i4                 *key6, 
DB_ERROR       *dberr)
{
    char                *k;
    i4                  i;
    i4                  length;
    u_i4		init[LK_KEY_WORDS];
    unsigned char	*v;
    i4			tot_length = 0;
    ADF_CB              adf_scb;
    DB_DATA_VALUE       dvfrom;
    DB_DATA_VALUE       dvto;
    STATUS              local_stat = OK; 

    init[0] = init[1] = init[2] = -1;

    MEfill(sizeof(ADF_CB),0,(PTR)&adf_scb);
    adf_scb.adf_maxstring = DB_MAXSTRING;

    if (rcb->rcb_hashbuf == NULL)
    {
       rcb->rcb_hashbuf = (char *)MEreqmem(0, HASH_BUFLEN,
                    FALSE, &local_stat);
       if (rcb->rcb_hashbuf == NULL || local_stat)
       {
          SETDBERR(dberr, 0, E_DM9425_MEMORY_ALLOCATE);
          return (E_DB_ERROR);
       }
    }

    for (i = 0; i < count; i++)
    {
        k = tuple + att[i]->offset;
        switch (att[i]->type)
        {
        case ATT_INT:
        case ATT_FLT:
        case ATT_MNY:
        case ATT_CHA:
            length = att[i]->length;
            break;

        default:
            dvfrom.db_datatype = att[i]->type;
            dvfrom.db_length = att[i]->length;
            dvfrom.db_prec = att[i]->precision;
            dvfrom.db_data = k;
            dvfrom.db_collID = att[i]->collID;
	    dvto.db_length = HASH_BUFLEN;
	    dvto.db_data = rcb->rcb_hashbuf;
            adc_hashprep(&adf_scb, &dvfrom, &dvto);
            length = dvto.db_length;
            k = dvto.db_data;
            break;
        }

        /*  Process this attribute. */
	for (; length; length--, tot_length++)
	{
	    if ( (tot_length % (LK_KEY_WORDS * 4)) == 0)
               v = (unsigned char*)&init;
	    *v ^= *k; v++; k++;
	}		      
    }

    *key4 = (i4)init[0];
    *key5 = (i4)init[1];
    *key6 = (i4)init[2];
    return (E_DB_OK);
}
