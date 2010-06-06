/*
** Copyright (c) 1989, 2010 Ingres Corporation
*/

# include   <compat.h>
# include   <gl.h>
# include   <cm.h>
# include   <cs.h>
# include   <st.h>
# include   <me.h>
# include   <pc.h>
# include   <iicommon.h>
# include   <dbdbms.h>
# include   <sr.h>
# include   <tr.h>
# include   <ulf.h>

# include   <adf.h>
# include   <adp.h>

# include   <dmf.h>
# include   <dmccb.h>
# include   <dmrcb.h>
# include   <dmtcb.h>
# include   <dmucb.h>
# include   <dmxcb.h>
# include   <lg.h>
# include   <lk.h>
# include   <scf.h>

# include   <dm.h>
# include   <dml.h>
# include   <dmp.h>
# include   <dmu.h>
# include   <dmpepcb.h>
# include   <dmpecpn.h>
# include   <dmpp.h>
# include   <dm1b.h>
# include   <dm0l.h>
# include   <dm0m.h>
# include   <dm0s.h>
# include   <dm1p.h>
# include   <dm1c.h>

# include   <dm2r.h>
# include   <dm2t.h>
# include   <dmse.h>
# include   <dm2umct.h>
# include   <dm2umxcb.h>
# include   <dm2u.h>
# include   <dmxe.h>

# include   <dmftrace.h>

# include   <cui.h>
# include   <dm0p.h>
# include   <dm0pbmcb.h>
#include    <sxf.h>
#include    <dmd.h>

/**
**
**  Name: DMPE.C - DMF routines to aid in large object management
**
**  Description:
**      This file contains the DMF routine used to aid in large object
**	management.  These routines are separated from the normal dmf_call()
**	routine because ADF is not capable of supporting the more complex
**	interface necessary to use that routine.  ADF requires, for large object
**	management, a routine with a specified interface, regardless of the
**	product in which it is embedded (i.e. DBMS or frontend).  These routines
**	provide the interface for the INGRES local DBMS.
**
**	Public Routines:
**          dmpe_peripheral() - Provide std interface
**				for large object management.
**
**          dmpe_put() - Add new [portion] of a peripheral object.
**          dmpe_get() - Retrieve a portion of a peripheral object.
**          dmpe_delete() - Remove an entire peripheral object.
**	    dmpe_move()	- Move an entire peripheral object to a new place
**          dmpe_information() - Obtain information about peripheral object
**			    management.
**	    dmpe_check_args() - Check parameters by function for these routines
**	    dmpe_allocate() - Allocate pcb and initialize operations for a
**			    particular peripheral operation.
**	    dmpe_deallocate() - Undo the effects of the aforementioned
**			    dmpe_allocate().
**	    dmpe_tcb_populate() - Add extended table catalog info to tcb.
**	    dmpe_add_extension() - Create new table extension for this blob.
**          dmpe_create_catalog() - Create ii_extended_relation catalog
**			    for use by large object code.
**	    dmpe_cat_scan() - Scan extended table catalog to find candidate
**			    locations for this blob entry.
**
**
**  History:
**      21-Dec-1989 (fred)
**          Created.
**	05-oct-1992 (rogerk)
**	    Reduced logging project: changed references to tcb fields to
**	    use new tcb_table_io structure fields.
**	16-oct-1992 (rickh)
**	    Initialize default ids for attributes.
**	09-nov-1992 (fred)
**	    Added changes throughout to correctly deal with NULL large objects.
**	24-nov-92 (rickh)
**	    Recast default ids as table ids.  Most excellent code hygeine.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**      23-Mar-1993 (fred)
**          Remove reference to pop_tab_id.  The field was removed
**          from the pop_cb since it was only set, never used.  Not
**          needed, and complicated the splitting of <dbms.h>.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Include <me.h> since we use ME routines in this file.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**       1-Jul-1993 (fred)
**          Added function prototypes for dmpe routines.  Also,
**          changed default size of extension tables.
**      13-Jul-1993 (fred)
**          Made system more tolerant of lack of disk space.  See
**          dmpe_put().
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**      11-aug-93 (ed)
**          added missing includes
**      12-Aug-1993 (fred)
**          Add capability to delete temporary large objects.
**      23-Aug-1993 (fred)
**          Add CSswitch() calls at interesting points.
**	13-sep-93 (swm)
**	    Moved cs.h include to pickup definition of CS_SID before scf.h
**	    and lg.h includes.
**      27-Oct-1993 (fred)
**          Externalized dmpe_deallocate().  This is so that dm2r()
**          can use it to properly deallocate PCB's when necessary.
**          Allowed for a list of pcb's associated with a base table.
**          This allows for multiple streams of operation during
**          processing.  Also, allowd for random segment access
**          during dmpe_get() operations.
**	11-oct-93 (johnst)
**	    Bug #56449
**	    Changed TRdisplay format args from %x to %p where necessary to
**	    display pointer types. We need to distinguish this on, eg 64-bit
**	    platforms, where sizeof(PTR) > sizeof(int).
**	15-nov-93 (swm)
**	    Bug #58633
**	    Rounded up under_dv.db_length to guarantee pointer-alignment
**	    after dm0m_allocate() memory allocation.
**      31-jan-1994 (jnash)
**          Set journaling on when creating iiextended_relation.
**	31-jan-1994 (bryanp)
**	    B58463, B58464, B58465, B58466, B58467, B58468, B58469, B58470,
**	    B58471, B58475, B58476, B58477, B58478, B58479, B58480, B58481,
**	    B58482, B58483, B58484, B58485, B58486:
**	    Check return code from adc_isnull.
**	    Check return code from dm2t_close.
**	    Check return code from dmpe_allocate.
**	    Check return code from dmpe_qdata.
**	    Return proper error codes in cleanup cases.
**	15-Feb-1994 (chiku) B59178
**	    System catalog should be created in accordance with case
**          specifications.
**	25-feb-94 (robf)
**          Check for E_DM008E as well as E_DM0074 returns from dmr_position
**      13-Apr-1994 (fred)
**          Fixed bug 59700 wherein numerous errors were logged when
**          user interrupts occurred.  No real damage was done, but
**          the error log file was polluted.  This change was made by
**          altering the places that mapping from internal DMF errors
**          to ADP_* errors was done.  Rather than doing this in a
**          variety of places, this processing has now been coalesced
**          at the entry point routines to this file, namely
**          dmpe_{call,delete,move,replace,destroy,modify,relocate}().
**      15-Apr-1994 (fred)
**          Altered dmpe_free_temps() to allow for deleting temporary
**          tables for specific queries.
**	1-Feb-1996 (kch)
**	    In dmpe_temp(), we now save the creation data for session
**	    temporary tables as well.
**	    In dmpe_free_temps(), we now do not unlink or deallocate the
**	    DMT_CBs if we are deleting session temp extension tables
**	    (pop_cb->pop_s_reserved == 1, set in destroy_temporary_table()).
**	    Both these changes will allow specific session temp extension
**	    tables in II_WORK to be destroyed when a 'drop table session.temp'
**	    is issued. These change fixes bug 72752.
**	06-mar-1996 (stial01 for bryanp)
**	    When creating a table to hold a blob, choose a sufficiently large
**          page size.
**	25-Jul-1996 (kch)
**	    In dmpe_free_temps(), we now test for pop_cb->pop_s_reserved != TRUE
**	    rather than !pop_cb->pop_s_reserved, because pop_s_reserved might
**	    be unintialized.
**	31-Jul-1996 (kch)
**	    In dmpe_free_temps(), added the new argument sess_temp_flag to
**	    indicate whether to free sess temp objects or not. In
**	    dmpe_call(), we now call dmpe_free_temps() with sess_temp_flag set
**	    to FALSE if op_code is ADP_FREE_NOSESS_OBJS and sess_temp_flag set
**	    to TRUE if op_code is ADP_FREE_ALL_OBJS. In dmpe_temp(), we set
**	    dup_dmt_cb->s_reserved to session_temp so that sess temp objects
**	    are freed correctly in dmpe_free_temps(). All these changes fix
**	    bug 78030. Also, in dmpe_delete() we now test for the existence
**	    of a list of extended tables before attempting to operate on it.
**	    This prevents a 'delete from session.temp' causing a SEGV if the
**	    session.temp contains a blob column.
**      05-aug-96 (sarjo01)
**          Bug 77690: References to ROLLBACKed extended tables can persist
**          in active TCBs for the base table. If dmt_open() fails due
**          to E_DM0054_NONEXISTENT_TABLE, do not take fatal error. Rather,
**          mark this table as invalid and continue searching for a
**          candidate table.
**      07-aug-96 (i4jo01)
**          Performing a LEFT on a blob field where the number of characters
**          retrieved is less than the total number in the field, then
**          the page in the extension table is not unfixed. Added switch
**          to dmpe_call to allow the extension table to be closed before
**          all segments have been read. Fixes bug #77189.
**	13-aug-96 (somsa01)
**	    In dmpe_add_extension(), if the base table has the TCB_JON flag
**	    set in its relstat, I set this flag for the corresponding etab
**	    table as well.
**	22-Aug-1996 (prida01)
**	    Change dmpe_get to open extended table in read mode not
**	    write mode. Stops problems with pages being fixed when we
**	    commit the transaction.
**      19-aug-96 (sarjo01)
**          Bug 78019: In dmpe_put(), open extended table in IX rather than
**          S mode since we intend to insert.
**	04-sep-1996 (kch)
**	    In dmpe_put(), we now open extended tables in X rather than IX
**	    mode to avoid escalation to table level locking which in turn
**	    causes deadlock. This change fixes bugs 77097, 77099 and 77914.
**	24-Oct-1996 (somsa01)
**	    In dmpe_put(), changed the opening of extended tables back to S
**	    mode. However, physical page X locks are taken in extended tables
**	    when needed, since the structure of an extended table is a hash.
**	    Also, in dmpe_free_temps(), if the error E_DM0054_NONEXISTENT_TABLE
**	    is returned from dmt_delete_temp(), this is OK, since the blob temp
**	    table was probably destroyed by some other "natural" cleanup
**	    action.
**	18-mar-1997 (rosga02)
**          Integrated changes for SEVMS
**          18-jan-95 (harch02)
**            Bug #74213
**            dmpe_add_extension() altered to set the extension table
**            security label id to the base table label id. They must match.
**            Otherwise the extension table inherits its security label
**	      from the user who inserts the first row.
**      24-mar-1998 (thaju02)
**          Bug #87130 - Holding tcb_et_mutex while retrieving tuples to be
**          deleted from extension table. Modified dmpe_delete().
**	24-mar-1998 (thaju02)
**	    Bug #87880 - inserting or copying blobs into a temp table chews
**	    up cpu, locks and file descriptors. Modified dmpe_put(),
**	    dmpe_allocate().
**	29-Jun-98 (thaju02)
**          Regression bug: with autocommit on, temporary tables created
**          during blob insertion, are not being destroyed at statement
**          commital, but are being held until session termination.
**          Regression due to fix for bug 87880. (B91469)
**	02-jul-1998 (shust01)
**	    Initialized cpn_rcb for fake peripheral in dmpe_destroy(),
**	    dmpe_modify(), and dmpe_relocate() to avoid potential SEGV.
**	28-jul-1998 (somsa01)
**	    Added another parameter to dmpe_put(). This parameter will
**	    tell us whether or not we are bulk loading blobs into
**	    extension tables. Also added dmpe_start_load(), which
**	    initiates a load operation on an extension table, as well as
**	    cleaning up some compiler warnings.  (Bug #92217)
**	03-aug-1998 (somsa01)
**	    Minor cleanup to dmpe_start_load().
**	03-aug-1998 (somsa01)
**	    Put part of last removal back.
**	04-aug-1998 (somsa01)
**	    In dmpe_put(), etlist_ptr needs to also get initialized right
**	    before an input happens.
**	12-aug-1998 (somsa01)
**	    In dmpe_temp(), we also need to set up the proper structure
**	    of the global temporary extension table.  (Bug #92435)
**	18-aug-1998 (somsa01)
**	    In dmpe_put(), etlist_ptr should be set to the node based upon
**	    prd_r_next.
**	20-aug-1998 (somsa01)
**	    In dmpe_modify(), there is no reason to run dmpe_cat_scan() for
**	    temporary extension tables. Instead, we have to manually walk
**	    the tcb_et_list and grab the temporary extension table ids from
**	    there.
**	27-aug-1998 (somsa01)
**	    In dmpe_temp(), refined the check for a global temporary
**	    extension table when modifying it to its proper table structure.
**	03-sep-1998 (somsa01)
**	    In dmpe_get(), only set run_petrace if we have a valid RCB.
**	10-sep-1998 (somsa01)
**	    In dmpe_get(), dm0m_check() takes two parameters.
**	28-oct-1998 (somsa01)
**	    When dealing with Global Temporary Extension Tables, make sure
**	    that these tables are placed on the session's lock list, not
**	    the transaction list.  (Bug #94059)
**	14-nov-1998 (somsa01)
**	    In dmpe_free_temps(), if sess_temp_flag is true, then it may have
**	    been another transaction which created this temp table in this
**	    session. Thus, pass the xcb for the current transaction to
**	    dmt_delete_temp().  (Bug #79203)
**	07-dec-1998 (somsa01)
**	    In dmpe_free_temps(), re-worked the fix for bug #79203. Update
**	    the dmt_cb's of concern with the current transaction id in
**	    dmudestroy.c rather than here. The previous fix needed to modify
**	    adp.h, which compromised spatial objects (unless a change was
**	    made to iiadd.h, which we are hesitant to make).  (Bug #79203)
**	01-feb-1999 (somsa01)
**	    In dmpe_temp(), set DMT_LO_LONG_TEMP if ADP_POP_LONG_TEMP is
**	    set. This will signify that the temporary table will last for
**	    more than 1 transaction (internal or external), yet is not a
**	    session-wide temporary table.
**	17-feb-1999 (somsa01)
**	    In dmpe_replace(), if there is nothing to update, set the
*8	    pop_error.err_code to E_DM0154_DUPLICATE_ROW_NOTUPD.
**	12-apr-1999 (somsa01)
**	    In dmpe_put(), if pcb->pcb_base_dmtcb.dmt_record_access_id is
**	    not set, then someone else closed out the table. Thus, there is
**	    no reason to call dmt_close(). Also, in dmpe_allocate(), we need
**	    to initialize pcb->pcb_fblk.adf_fi_id.
**	15-apr-1999 (somsa01)
**	    In places where we locally allocate an ADP_POP_CB, initialize
**	    the pop_info field to a NULL pointer.
**	7-may-99 (stephenb)
**	    Add function dmpe_cleanup() to cleanup peripheral operations
**	    environment, at this point it's simply a synonym for
**	    dmpe_deallocate.
**      30-jul-99 (stial01)
**          Fix dmpe_allocate error handling, fix pop_temporary initialization
**      03-aug-99 (stial01)
**          Added dmpe_create_extension, called from dmu, dmpe_add_extension
**      12-aug-99 (stial01)
**          Changes to support varying segment sizes (SIR 92574)
**          Also removed dmpe_create_catalog (should have done in 03-aug-99)
**      31-aug-99 (stial01)
**          VPS etab support
**      10-sep-99 (stial01)
**          dmpe_replace() call dmpe_copy instead of gets/put. (SIR 92574)
**          Use ADP_INFO_* defines instead of constants
**          Removed incorrect references to DM_COMPAT_PGSIZE (VPS etab support)
**      20-sep-99 (stial01)
**          Get intent (IS/IX) etab table locks.
**      02-nov-99 (stial01)
**          dmpe_allocate() Set SCB_BLOB_OPTIM only if BLOBWKSP_TABLENAME
**      08-dec-99 (stial01)
**         Backout of previous changes for vps etab
**      21-dec-99 (stial01)
**         VPS etab support (again)
**      10-jan-2000 (stial01)
**          Added support for 64 bit RCB pointer in DMPE_COUPON
**      12-jan-2000 (stial01)
**         dmpe_allocate() dmpe_allocate only go through optimization if passed
**         tablename, otherwise we will increment logical key unecessarily
**      12-jan-2000 (stial01)
**         Byte swap per_key (Sir 99987)
**      07-mar-2000 (stial01)
**          Disable blob optimization for spatial  (B100776)
**      09-mar-2000 (stial01)
**          PCB memory leak from dmpe_delete (B100829)
**          Restore MAX_DMPE_RECORD_SIZE DB_MAXSTRING -> sizeof(DMPE_RECORD)
**      27-mar-2000 (stial01)
**          Changes for b101046,b101047. Pass DMT_BLOB_OPTIM to close base table
**          Also do not require pop_info to close base table. Save base table
**          attribute in pcb. Cleanup unused VPS etab code.
**      18-may-2000 (hweho01)
**         Fixed the type mismatch in argument list of ule_format() call.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**      28-nov-2000 (stial01)
**          Removed dmpe_lock_value, row tranid used for data page reclamation
**	08-Jan-2001 (jenjo02)
**	   Use macro to find *DML_SCB instead of SCU_INFORMATION.
**      01-feb-2001 (stial01)
**          Deleted dmpe_space_reclaim (Variable Page Type SIR 102955)
**      12-feb-2001 (mosjo01)
**	   Fixed C syntax error that only surfaced when xDEBUG was turned on.
**      02-mar-2001 (stial01)
**         dmpe_temp() create temp etabs with no duplicates (SIR 98978)
**      28-feb-2001 (stial01)
**         dmpe_allocate reset dmt_record_access_id if base table closed b104113
**	08-mar-2001 (somsa01)
**	    Changed usage of DB_MAXSTRING to DMPE_SEGMENT_LENGTH, which
**	    accurately depicts the segment size.
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**      22-mar-2001 (stial01)
**          dmpe_temp() don't put session temp etabs on scb_lo_next queue
**          dmpe_free_temps() Don't use dmt_sequence (cursor number) to
**          determine which etabs can be destroyed. They all can, now that
**          session temp etabs don't go on scb_lo_next queue (b104317)
**      01-may-2001 (stial01)
**          Init adf_ucollation
**      21-may-2001 (stial01)
**          Added comment about use of DM1P_MAX_TABLE_SIZE (B104754)
**      20-mar-2002 (stial01)
**          Updates to changes for 64 bit RCB pointer in DMPE_COUPON
**          Removed att_id from coupon, making room for 64 bit RCB ptr in
**          5 word coupons. (b103156)
**      15-may-2003 (stial01)
**          Holding tcb_et_mutex while calling dmr_put (b110258)
**      30-may-2003 (stial01)
**          Always use table locking for DMPE Temp tables (b110331)
**      15-sep-2003 (stial01)
**          Open etab with table locking if base table page locking (b110923)
**      16-sep-2003 (stial01)
**          Use svcb_etab_type to determine where to put blob (b110923)
**      02-jan-2004 (stial01)
**          Removed temporary trace points DM722, DM723
**          Added dmpe_etab_but() to bulk load into btree disassoc data pages
**          Added dmpe_journaling to support SET [NO]BLOBJOURNALING ON table
**          dmpe_modify() support MODIFY to ADD_EXTEND with BLOB_EXTEND
**      14-jan-2004 (stial01)
**          Cleaned up 02-jan-2004 changes.
**	15-Jan-2004 (schka24)
**	    Partitioned tables project.
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**	11-Feb-2004 (hanje04)
**	    Added (DMPP_SEG_HDR *)0 as missing argument in calls to dmpp_get.
**	    Support for 256K rows changes added extra argument in dm1c.h but
**	    didn't change the function calls.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      18-mar-2004 (stial01)
**          dmpe_buffered_put, DON'T update prd_r_next in last segment
**      25-mar-2004 (stial01)
**          Added dmpe_close() to avoid double deallocation of pcb
**      26-mar-2004 (stial01)
**          dmpe_get() Fixed up positioning code, so we don't do unecessary
**          repositions
**      02-apr-2004 (stial01)
**          dmpe_modify() Init dmu_cb.dmu_location.data_in_size (b112085)
**      22-apr-2004 (stial01)
**          Reset pop_user_arg after dmpe_cleanup (dmpe_deallocate) (b112190)
**	6-May-2004 (schka24)
**	    DMPE has been passing around RCB's in coupons based on the
**	    assumption that a query executes entirely in one thread, with
**	    one DMF transaction context.  This assumption is no longer
**	    true with parallel query.  Revise to use thread-local context,
**	    and to pass the TCB instead of the RCB in a coupon where
**	    it's still necessary.  Move PCB cleanup chain from RCB to XCB.
**	    Simplify temporary table business.
**      13-may-2004 (stial01)
**          Remove unecessary code for NOBLOBLOGGING redo recovery.
**      03-jun-2004 (stial01)
**          Undo minor part of 6-May-2004 change so that we don't skip the
**          insert of a blob segment when it doesn't fit in the current buffer.
**	21-Jun-2004 (schka24)
**	    Get rid of SCB blob-cleanup list, use XCCB list instead so
**	    that we don't have to keep two lists in sync.  Which we weren't
**	    doing anyway, causing problems.
**      8-jul-2004 (stial01)
**          dmpe_get() Remove last reference to tcb in coupon,
**          as of changes on 6-May-2004, dmpe_get->dmpe_allocate always calls
**          CSget_sid, GET_DML_SCB for context
**       8-Jul-2004 (hanal04) Bug 112558 INGSRV2879
**          Prevent memory leak in dmpe_get() when a caller does not
**          retrieve the whole blob.
**     14-dec-2004 (stial01)
**          dmpe_move() fixed USER_INTR error handling
**	21-feb-05 (inkdo01)
**	    Init adf_uninorm_flag values.
**      16-oct-2006 (stial01)
**          New routines for blob locators.
**      30-oct-2006 (stial01)
**          dmpe_cpn_to_locator() Fixed Fixed declaration of local variable.
**      14-dec-2006 (stial01)
**	    dmpe_get() btree etabs position >= seg1 (for first or random seg)
**          Also, don't log length mismatch error if seek to random seg fails.
**      04-jan-2007 (stial01)
**          Open iiextended_relation LK_IX when updating, ifdef TRdisplay
**      09-feb-2007 (stial01)
**          if xccb_blob_temp_flags & BLOB_HAS_LOCATOR, temp persists after qry
**          Moved locator context from DML_XCB to DML_SCB
**          Added ADU_FREE_LOCATOR, dmpe_free_locator()
**      17-may-2007 (stial01)
**          dmpe_free_locator changes for FREE LOCATOR statement
**          Create LOCATOR_TEMPORARY_TABLE with session lifetime
**      20-jul-2007 (stial01)
**          dmpe_cpn_to_locator - create locator for zero length blob (b118794)
**	22-Feb-2009 (kschendel) SIR 122739
**	    Row-accessor changes, reflect here.  (no record type in put, etc).
**    20-Oct-2008 (jonj)
**        SIR 120874 Modified to use new DB_ERROR based uleFormat
**        and functions.
**    21-Nov-2008 (jonj)
**        SIR 120874: dmxe_? functions converted to DB_ERROR *
**    24-Nov-2008 (jonj)
**        SIR 120874: dm0m_? functions converted to DB_ERROR *
**    26-Nov-2008 (jonj)
**        SIR 120874: dm2t_?, dm2r_? functions converted to DB_ERROR *
**    05-Dec-2008 (jonj)
**        SIR 120874: dm1c_? functions converted to DB_ERROR *
**      05-feb-2009 (joea)
**          Rename CM_ischarsetUTF8 to CMischarset_utf8.
**    11-May-2009 (kschendel) b122041
**        Compiler warning fixes.
**      16-Jun-2009 (hanal04) Bug 122117
**          Treat E_DM016B_LOCK_INTR_FA the same as a user interrupt.
**	19-Aug-2009 (kschendel) 121804
**	    Need cm.h for proper CM declarations (gcc 4.3).
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Prototype changes for dmpp_get/put/delete
**	26-Mar-2010 (toumi01) SIR 122403
**	    For encryption project add attr_encflags, attr_encwid.
**	12-Apr-2010 (kschendel) SIR 123485
**	    Change short-term part of DMF coupon to be baseid, attid, flags.
**	    Implement the DMPE_BQCB (blob query cb) to communicate context
**	    info for the duration of a query.  This allows coupons to be
**	    redeemed with proper locking, and incoming blobs to be couponified
**	    more efficiently.
**	    Get rid of adc_isnull, use inline check.
**	    Fix "table too large" (DM0138) when bulk-loading etabs. (b120497)
**      28-Apr-2010 (stial01)
**          dmpe_buffered_put() Allocate LONGTERM memory for rcb_bulk_misc
**/

/*
**  Definition of static variables and forward static functions.
*/
static DB_STATUS dmpe_information(ADP_POP_CB *pop_cb );

static DB_STATUS dmpe_put(ADP_POP_CB	*pop_cb);

static DB_STATUS dmpe_get(i4	    	op_code ,
			  ADP_POP_CB    *pop_cb );

static DB_STATUS dmpe_allocate(ADP_POP_CB   *pop_cb ,
			       DMPE_PCB     **pcb_ptr);

static DB_STATUS dmpe_begin_dml(ADP_POP_CB *pop_cb,
			 DMPE_PCB **pcbp, bool is_put);

static DB_STATUS dmpe_check_args(i4        op_code,
				 ADP_POP_CB *pop_cb);

static DB_STATUS dmpe_tcb_populate(ADP_POP_CB *pop_cb );

static DB_STATUS dmpe_add_extension(ADP_POP_CB *pop_cb,
			DMP_TCB *base_tcb,
			i4      att_id,
			u_i4	*new_etab);

static DB_STATUS dmpe_temp( ADP_POP_CB *pop_cb, i4 att_id, u_i4 *new_temp_etab);

static DB_STATUS dmpe_qdata(ADP_POP_CB	*pop_cb ,
			    DMT_CB	*dmtcb );

static DB_STATUS dmpe_nextchain_fixup(ADP_POP_CB *pop_cb );

static DB_STATUS dmpe_cat_scan(ADP_POP_CB       *pop_cb ,
			       DB_TAB_ID	*parent_id ,
			       DMP_ETAB_CATALOG	*etab_record );

static DB_STATUS dmpe_get_etab(
		DML_XCB                 *xcb,
		DMP_RELATION          *rel,
		u_i4     *etab_extension,
		bool     create_flag,
		DB_ERROR *dberr);


static STATUS dmpe_start_load(
			DMP_RCB		*rcb,
			DB_ERROR	*error);

static DB_STATUS dmpe_buffered_put(
			ADP_POP_CB	*pop_cb,
			DMR_CB *dmr_cb);

static DB_STATUS etab_open(
	DMT_CB		*dmtcb,
	ADP_POP_CB	*pop_cb);

static u_i4 dmpe_load_etab_limit(
	DMP_TCB		*et);

static DB_STATUS dmpe_cpn_to_locator(
ADP_POP_CB  *pop_cb);

static DB_STATUS dmpe_create_locator_temp(
	DML_SCB		*scb,
	DB_ERROR	*error);

static DB_STATUS dmpe_locator_to_cpn(
	ADP_POP_CB	*pop_cb);

/* Not used, should this be deleted?
**static DB_STATUS dmpe_destroy_heap_etab(
**	DMP_TCB		*base_tcb,
**	u_i4		etab_extension,
**	DB_ERROR	*dberr);
*/


/* File wide # defines */

#define		    DMPE_KEY_COUNT	3

/*
** DMPE_DEF_TBL_SIZE
**
** This value is the minpages that will be set for large object files.
** (schka24) There used to be a comment here about defaulting to 1000
** overflow pages, but the actual default did not reflect that; it
** was 100 times smaller.  Set the default minpages to a small-ish
** value.  One hopes that more intelligence will be applied to the
** choice, if there's any additional information available at runtime.
**
** Note these calculations reference DM1P_MAX_TABLE_SIZE which is currently
** 8388608 for most page sizes and page types.
** Maxpages is only 3849344 for 2k V2/V3/V4 tables, however 2k etabs MUST be V1.
** An exact maxpages calculation can be done using the DM1P_VPT_MAXPAGES_MACRO,
** but you must provide page type and page size information.
** Since page type is currently not determined here, and currently
** max pages is 8388608 for all valid etab pagetype/pagesize combinations,
** the DM1P_MAX_TABLE_SIZE have not been changed to DM1P_VPT_MAXPAGES_MACRO.
*/

# define DMPE_DEF_TBL_SIZE  	    	128
# define DMPE_DEF_TBL_EXTEND            128
# define DMPE_DEFAULT_ATTID		1  /* default attid for temp etabs */

FUNC_EXTERN VOID dmd_petrace(
			char		*operation,
			DMPE_RECORD	*record,
			i4		base,
			i4		extension );



/*{
** Name: dmpe_call	- Perform extended table management for ADF.
**
** Description:
**      This routine is primarily a dispatch routine for the ADF large object
**	management extensions.  That is, based on the operation code, the
**	appropriate subroutine is called.
**
** Inputs:
**      op_code                         Operation code
**      pop_cb                          Ptr to ADF Peripheral Operation Control
**					    Block
**
** Outputs:
**      (None)
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-Dec-1989 (fred)
**          Created.
[@history_template@]...
**      13-Apr-1994 (fred)
**          Centralize the error management for ADP_PUT & _GET calls
**          here.  We need to log dmf errors and convert them to
**          something that our callers (ADF) understand, but there
**          needs to be some specialized handling of interrupts, etc.,
**          to avoid pollution of the error log file.  To make this
**          easier, we move a bunch of duplicated code here...
**	17-Apr-1998 (thaju02)
**	    Added call to ule_format().
**	28-jul-1998 (somsa01)
**	    Added another parameter to dmpe_put() to tell us if we are
**	    bulk loading blobs into extension tables.  (Bug #92217)
**	10-may-99 (stephenb)
**	    Add case ADP_CLEANUP (cleanup DMPE memory).
**	6-May-2004 (schka24)
**	    Delete CLOSE, superseded by cleanup added above.  Have cleanup
**	    close if necessary.
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Insert error_override = E_AD7008_ADP_DELETE_ERROR
**	    after dmpe_delete(). Check for MVCC incompatibility.
**	    Use SETDBERR more judiciously.
*/
DB_STATUS
dmpe_call(i4        op_code ,
           ADP_POP_CB     *pop_cb )

{
    DB_STATUS           status = E_DB_ERROR;
    DB_ERRTYPE          error_override = 0;
    i4            err_code;

    CLRDBERR(&pop_cb->pop_error);

    do	    /* Need something to break out of */
    {
	status = dmpe_check_args(op_code, pop_cb);
	if (DB_FAILURE_MACRO(status))
	    break;

#ifdef	    xDEBUG
	if (DMZ_CLL_MACRO(1))
	{
	    TRdisplay("DMPE_CALL(ADP_%w (%<200 + %x), %p)\n",
		"?,INFORMATION,GET,PUT,DELETE,COPY,FREE_TEMPS", op_code - 0x200,
		    pop_cb);
	}
#endif
	switch (op_code)
	{
	    case ADP_INFORMATION:
		status = dmpe_information(pop_cb);
		break;

	    case ADP_PUT:
		status = dmpe_put(pop_cb);
		error_override = E_AD7006_ADP_PUT_ERROR;
		break;

	    case ADP_GET:
		status = dmpe_get(ADP_GET, pop_cb);
		error_override =  E_AD7007_ADP_GET_ERROR;
		break;

	    case ADP_DELETE:
		status = dmpe_delete(pop_cb);
		error_override =  E_AD7008_ADP_DELETE_ERROR;
		break;

	    case ADP_COPY:
		/* Don't delete source, who knows whether adf code will
		** refer to it again.
		*/
		status = dmpe_move(pop_cb, FALSE);
		error_override = E_AD7009_ADP_MOVE_ERROR;
		break;

	    case ADP_FREE_NONSESS_OBJS:
		status = dmpe_free_temps((DML_ODCB *) pop_cb->pop_user_arg,
				&pop_cb->pop_error);
		break;

	    /* The CLEANUP function is used when a get is terminated early
	    ** by a filter function.  It signals the end of the POP.
	    */
	    case ADP_CLEANUP:
		{
		    DMPE_PCB   *pcb = (DMPE_PCB *) pop_cb->pop_user_arg;

		    if ( pcb )
		    {
			if (pcb->pcb_dmtcb->dmt_record_access_id != NULL)
			{
			    pcb->pcb_dmtcb->dmt_flags_mask = 0;
			    status = dmt_close(pcb->pcb_dmtcb);
			}
			dmpe_deallocate(pcb);
			pop_cb->pop_user_arg = NULL;
		    }
		    status = E_DB_OK;
		}
		break;

	    case ADP_CPN_TO_LOCATOR:
		status = dmpe_cpn_to_locator(pop_cb);
		error_override =  E_AD7013_ADP_CPN_TO_LOCATOR;
		break;

	    case ADP_LOCATOR_TO_CPN:
		status = dmpe_locator_to_cpn(pop_cb);
		error_override =  E_AD7014_ADP_LOCATOR_TO_CPN;
		break;

	    case ADP_FREE_LOCATOR:
		status = dmpe_free_locator(pop_cb);
		error_override = E_AD7015_ADP_FREE_LOCATOR;
		break;

	    default:
		SETDBERR(&pop_cb->pop_error, 0, E_AD7005_ADP_BAD_POP);
		break;
	}
    } while (0);
#ifdef	xDEBUG

    if (DMZ_CLL_MACRO(2))
	TRdisplay("DMPE_CALL returning E_DB_%w. (%<%d.)\n",
	    "OK,?,INFO,?,WARN,ERROR,?,SEVERE,?,FATAL", status);
#endif
    if (DB_FAILURE_MACRO(status) && error_override)
    {
	if ((pop_cb->pop_error.err_code == E_DM0065_USER_INTR) ||
            (pop_cb->pop_error.err_code == E_DM016B_LOCK_INTR_FA))
	{
	    SETDBERR(&pop_cb->pop_error, 0, E_AD7010_ADP_USER_INTR);
	    status = E_DB_ERROR;
	}
	else if ( pop_cb->pop_error.err_code == E_DM0012_MVCC_INCOMPATIBLE )
	{
	    SETDBERR(&pop_cb->pop_error, 0, E_AD7016_ADP_MVCC_INCOMPAT);
	    status = E_DB_ERROR;
	}
	else if (pop_cb->pop_error.err_code != E_AD7001_ADP_NONEXT)
	{
	    uleFormat(&pop_cb->pop_error, 0, NULL, ULE_LOG,
					NULL, (char *) NULL, 0L, (i4 *) NULL,
					&err_code, 0);
	    uleFormat(&pop_cb->pop_error, error_override, NULL, ULE_LOG,
					NULL, (char *) NULL, 0L, (i4 *) NULL,
					&err_code, 0);
	}
    }
    return(status);
}

/*
**{
** Name: dmpe_put	- Put a peripheral object.
**
** Description:
**	This routine appends one segment of a peripheral object (LOB) to
**	another LOB, and fills in the output LOB description as a coupon.
**
**	From a high level perspective, the operation is "move LOB data to
**	either a table row or a holding area."  The row / holding area
**	is identified by a coupon, which dmpe_put fills in.  Since LOBs
**	come in different forms, couponified or data streams, some caller
**	will have to fetch and disassemble the incoming LOB data.
**	dmpe_put only deals with one segment at a time.  The higher level may
**	call dmpe_put multiple times, indicating "first segment" and
**	"last segment".
**
**	Peripheral objects are actually stored in tables in the system.  These
**	tables have the look as if created by
**
**	    create table <extended_table>
**	    (
**		per_key	    table_key,
**		per_segment0 integer,
**		per_segment1 integer,
**		per_next    integer,
**		per_object  <underlying datatype>(MAXTUP -
**						    (sizeof(per_key) +
**							 sizeof(per_segment)))
**	    )
**
**	<extended_table> will be the name of the table, constructed as
**	    iilo_<base_table_id>_<attribute_number>_<this_tables_id>.
**
**	<underlying datatype> will take from the caller, and describes how the
**	underlying data should be represented.  For example, long varchar is
**	stored as varchar segments, long bit is stored as bit segments.  (This
**	information is not decided or controlled by this routine -- I just
**	happen to know it at the time of this writing).
**
**	If conditions are right, the data segments may be bulk-loaded into
**	the target extension tables instead of stored row by row.
**	Bulk-loading poses a unique problem, which is dealing with a
**	full etab. For normal row-by-row puts to an etab, we can simply
**	check the DMR_PUT return status for "table too large", and move
**	on to a new etab.  The catch is that each LOB segment has to
**	indicate the etab number of the next segment, which means that
**	we have to go back and update the last-written LOB segment.
**	This is easy enough for regular etabs, but when we're bulk-loading,
**	rows are going to the sorter and there is no such thing as
**	updating a row in the sorter.
**
**	The way this is fixed is to write one segment behind.  As each
**	segment arrives, if we predict it fits into an etab, the PRIOR
**	segment is written (to the sorter).  If the current segment won't
**	fit, a new etab load is started, and the PRIOR segment is updated
**	before being written.  Then, the current segment is made the prior.
**	Care is needed to get the details right (e.g. a new bulk-load on
**	the first segment, and getting the END segment out right, and
**	LOBs with just one segment, etc).
**
**	When bulk-loading, the BQCB will indicate whether to attempt a
**	bulk-load.  The current etab's DMP_ET_LIST entry will indicate
**	whether the etab is really being bulk-loaded or not.
**
**	By the way, this write-behind idea doesn't work so well for
**	ordinary row-puts.  The best way to tell if an etab is full if
**	NOT bulk-loading it, is to simply do the put and see what happens.
**	When bulk-loading we can (indeed, have to) predict when a etab
**	will fill, by counting rows.  Counting doesn't work for an existing
**	partly full etab with ??? rows in it already.
**
** Inputs:
**      pop_cb                          ADP_POP_CB Ptr for controlling the
**					peripheral operation
**	    pop_continuation		state of the world.
**					ADP_C_BEGIN_MASK   - the first of a
**					multicall operation.
**					ADP_C_END_MASK - the last of one.
**					Either/both/neither can be set.
**	    pop_segment			DB_DATA_VALUE which points to a segment
**					of input data to send to the lob.
**					(Historical:  NULL used to be allowed
**					as a sort of "oops, last one was the
**					end, I don't have any more, go back and
**					fix up the last segment" but nobody
**					calls it that way any more.)
**	    pop_coupon			DB_DATA_VALUE pointing to the output
**					coupon which is to be filled in.
**	    pop_underdv			A pointer to a DB_DATA_VALUE which
**					describes the datatype to be used
**					for each segment.
**	    pop_temporary		Indicator as to whether this is
**					put into a temporary file.  If the
**					caller says "temp", dmpe *might* be
**					able to do better;  but if the caller
**					says "permanent", the target has to be
**					resolvable into a real etab.
**	    pop_user_arg		Workspace area used by DMF.  The caller
**					knows nothing of the contents here, but
**					is expected to preserve its contents
**					across multipass calls.  Failure to do
**					so will result in errors.
**	    pop_info			Optional pointer to DB_BLOB_WKSP which
**					can be filled in by the caller to
**					supply target information when the
**					operation is beginning (i.e.
**					ADP_C_BEGIN_MASK).  See dmpe_begin_dml.
**
** Outputs:
**      pop_cb
**	    pop_coupon			Will have the put-portions of the coupon
**					filled in.  For temporaries, will be
**					completely filled.  This portion may not
**					be complete until the last call of a
**					multipass operation is completed.
**	    pop_user_arg		Will be fill in with dmpe_put() specific
**					information to be preserved and returned
**					by the caller if pop_input is not a
**					coupon, and pop_continuation is not
**					ADP_COMPLETE.
**	    pop_error			DB_ERROR will be filled appropriately.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-Dec-1989 (fred)
**          Created.
**      13-Jul-1993 (fred)
**          Made the system more tolerant of lack of disk space.  If
**          we attempt to allocate space and file more than once, then
**          give up rather than continuing to attempt all over the
**          place.
**      20-Oct-1993 (fred)
**          Added support for working with session temporary tables.
**          If the blob is being added to a session temporary table,
**          make sure that the underlying table is also temporary.
**	31-jan-1994 (bryanp) B58463, B58464
**	    Check return code from adc_isnull.
**	    If dmpe_close fails, return dmtcb->error as the error code.
**      13-Apr-1994 (fred)
**          Alter error handling to return error directly to the
**          caller.  Our caller will than translate this appropriately.
**      26-jul-96 (sarjo01)
**          Bug 77690: References to ROLLBACKed extended tables can persist
**          in active TCBs for the base table. If dmt_open() fails due
**          to E_DM0054_NONEXISTENT_TABLE, do not take fatal error. Rather,
**          mark this table as invalid and continue searching for a
**          candidate table.
**      19-aug-96 (sarjo01)
**          Bug 78019: In dmpe_put(), open extended table in IX rather than
**          S mode since we intend to insert.
**	04-sep-1996 (kch)
**	    We now open extended tables in X (exclusive table lock) rather than
**	    IX (exclusive page lock) mode to avoid escalation to table level
**	    locking which in turn causes deadlock. This change fixes bugs
**	    77097, 77099 and 77914.
**	24-Oct-1996 (somsa01)
**	    Changed the opening of extended tables back to S mode. However,
**	    physical page X locks are taken in extended tables when needed,
**	    since the structure of an extended table is a hash.
**	24-Mar-1998 (thaju02)
**	    Bug #87880 - inserting or copying blobs into a temp table chews
**	    up cpu, locks and file descriptors. If we fail to insert and need
**	    to find find a place to put the record, test pop_cb->pop_temporary.
**	    If pop_cb->pop_temporary designates ADP_POP_SHORT_TEMP then
**	    create short term temporary table to store record, else if
**	    pop_cb->pop_temporary designates ADP_POP_LONG_TEMP then
**	    scan base table tcb's tcb_et_list for global temporary session
**	    extension table in which we can put record. If nothing found
**	    in tcb_et_list, then create new temporary table (as a global
**	    temporary session extension table).
**	29-Jun-98 (thaju02)
**	    Regression bug: with autocommit on, temporary tables created
**	    during blob insertion, are not being destroyed at statement
**	    commital, but are being held until session termination.
**	    Regression due to fix for bug 87880. (B91469)
**	28-jul-1998 (somsa01)
**	    Added another parameter to dmpe_put() to tell us if we are to
**	    bulk load blob segments into extension tables. This is set if
**	    we are coming from dm2r_load(). Also added the appropriate
**	    code to do bulk loads into extension tables. Also, fixed up
**	    when to run dmd_petrace().  (Bug #92217)
**	04-aug-1998 (somsa01)
**	    etlist_ptr needs to also get initialized right before an input
**	    happens.
**	18-aug-1998 (somsa01)
**	    etlist_ptr should initially be set to the node based upon
**	    prd_r_next.
**	28-oct-1998 (somsa01)
**	    In the case of Global Temporary Extension Tables, make sure that
**	    they are placed on the sessions lock list, not the transaction
**	    lock list.  (Bug #94059)
**	24-feb-98 (stephenb)
**	    add code to allow peripheral inserts to load directly into
**	    the target table.
**	12-apr-1999 (somsa01)
**	    If pcb->pcb_base_dmtcb.dmt_record_access_id is not set, then
**	    someone else closed out the table. Thus, there is no reason
**	    to call dmpe_close().
**	27-apr-99 (stephenb)
**	    Init RCB pointer for null values.
**	11-May-2004 (schka24)
**	    Clean up code that selects or creates etab, and don't hold
**	    the tcb et mutex so much.  When an optimized "temp" put
**	    succeeds in going to the final etab, tag the coupon when
**	    we finish, so that the ultimate base-record put knows to not
**	    do the move.
**	23-Sep-2009 (kiria01) b122578
**	    Initialise the ADF_FN_BLK .adf_fi_desc and adf_dv_n members.
**	15-Apr-2010 (kschendel) SIR 123485 b120497
**	    load-blob no longer passed in, we decide here.
**	    Delete code dealing with pop_segment NULL, doesn't seem to do
**	    anything rational and is never called that way.
**	    Fix long-standing problem with bulk-load mode getting "table
**	    too large":  the sorter doesn't return E_DM0138 so we didn't
**	    realize when too much data is being bulk-loaded into an etab.
**	    Keep track by hand and start loading a new etab when nearing
**	    the danger point.
*/
static DB_STATUS
dmpe_put(ADP_POP_CB	*pop_cb)
{
    DB_STATUS           status, close_status;
    DB_DATA_VALUE	*input = pop_cb->pop_segment;
    ADP_PERIPHERAL	*p = (ADP_PERIPHERAL *) pop_cb->pop_coupon->db_data;
    DMPE_COUPON		*output = DMPE_CPN_FROM_DBV_MACRO(pop_cb->pop_coupon);
    DMPE_PCB		*pcb = (DMPE_PCB *) NULL;
    DMP_RCB		*er;		/* etab RCB */
    DMP_TCB		*et;		/* etab TCB */
    DMP_TCB		*tcb;		/* Base table TCB */
    i4			err_code;
    i4			tried;
    i4			failed;
    DMP_ET_LIST		*etlist_ptr;
    i4                  att_id;
    u_i4		new_etab;

    CLRDBERR(&pop_cb->pop_error);

    if (ADF_ISNULL_MACRO(pop_cb->pop_coupon))
    {
	p->per_tag = ADP_P_COUPON;
	p->per_length0 = p->per_length1 = 0;
	output->cpn_flags = 0;
	return(E_DB_OK);
    }

    do /* Break-out-of control structure */
    {
	/*
	** Now that the input is OK, we need to go put the peripheral record.
	** To do this, we need to go find space if this is the first time.
	*/

	if (pop_cb->pop_continuation & ADP_C_BEGIN_MASK)
	{
	    status = dmpe_begin_dml(pop_cb, &pcb, TRUE);
	    if (status != E_DB_OK)
		return(status);

	    /* If we haven't assigned a pcb-att-id, the target must be a
	    ** holding temp, use a fake att id.
	    */
	    if (pcb->pcb_att_id == 0)
		pcb->pcb_att_id = DMPE_DEFAULT_ATTID;
	}
	else
	{
	    pcb = (DMPE_PCB *) pop_cb->pop_user_arg;
	}
	att_id = pcb->pcb_att_id;

	/* Build the tuple based on the pointers in the pcb */

	/*
	**  First update segment number.  Since this is i8 arithmetic,
	**  if, after being incremented, the first segment = 0,
	**  increment the second segment number.
	*/

	pcb->pcb_record->prd_segment1 += 1;
	if (pcb->pcb_record->prd_segment1 == 0)
	{
	    pcb->pcb_record->prd_segment0 += 1;
	}
	/* check if we need to coerce */
	if (pcb->pcb_fblk.adf_fi_id > 0 && pcb->pcb_fblk.adf_r_dv.db_data)
	{
	    ADF_CB *adfcb = pcb->pcb_xcb->xcb_scb_ptr->scb_adf_cb;

	    pcb->pcb_fblk.adf_fi_desc = NULL;
	    pcb->pcb_fblk.adf_dv_n = 1;
	    STRUCT_ASSIGN_MACRO(*input, pcb->pcb_fblk.adf_1_dv);
	    pcb->pcb_fblk.adf_r_dv.db_length = input->db_length;
	    status = adf_func(adfcb, &pcb->pcb_fblk);
	    if (status != E_DB_OK)
	    {
		pop_cb->pop_error = adfcb->adf_errcb.ad_dberror;
		break;
	    }
	    MEcopy((PTR) pcb->pcb_fblk.adf_r_dv.db_data,
		pcb->pcb_fblk.adf_r_dv.db_length,
		(PTR) pcb->pcb_record->prd_user_space);
	}
	else
	    MEcopy((PTR) input->db_data, input->db_length,
		(PTR) pcb->pcb_record->prd_user_space);
	tried = failed = 0;

	/* Now we insert the tuple into a relation. */

	status = E_DB_OK;
	for ( ; /* Until broken out of */ ; )
	{
	    if (pcb->pcb_dmrcb->dmr_access_id)
	    {
		etlist_ptr = pcb->pcb_et;	/* NULL if holding temp */
		if (pop_cb->pop_continuation & ADP_C_END_MASK)
		{
		    pcb->pcb_record->prd_r_next = 0;
		}
		else
		{
		    pcb->pcb_record->prd_r_next =
			pcb->pcb_dmtcb->dmt_id.db_tab_base;
		}

		tried++;	    /* Mark that we attempted to put a record */

		if (etlist_ptr == NULL || (etlist_ptr->etl_status & ETL_LOAD) == 0)
		{
		    /* Ordinary row being sent to an etab.
		    ** Attempt to use a "bulk put" that gathers up a bunch of
		    ** rows before writing.  (Low level details are not our
		    ** problem here.)
		    */
#ifdef	xDEBUG
		    /* If in test mode, then only put one record per object per
		    ** extension table.  This will forcibly test the full table
		    ** code.
		    */

		    if (DMZ_TBL_MACRO(11)
			&& (tried == 1)
			&& ((pop_cb->pop_continuation & ADP_C_BEGIN_MASK) == 0))
		    {
			status = E_DB_ERROR;
			SETDBERR(&pcb->pcb_dmrcb->error, 0, E_DM0138_TOO_MANY_PAGES);
		    }
		    else
#endif
		    status = dmpe_buffered_put(pop_cb, pcb->pcb_dmrcb);
		    if (status == E_DB_OK)
		    {
			if (DMZ_SES_MACRO(11) && pcb->pcb_tcb)
			{
			    dmd_petrace("DMPE_PUT: ", pcb->pcb_record,
				pcb->pcb_tcb->tcb_rel.reltid.db_tab_base,
				pcb->pcb_dmtcb->dmt_id.db_tab_base);
			}
			if (((pop_cb->pop_continuation & ADP_C_BEGIN_MASK) == 0)
			  &&  failed)
			{
			    /* We ended up placing a not-first segment in an
			    ** etab which was not the same as the previous
			    ** segment's etab.  So that we can find this
			    ** segment, we must go back and fix up the previous
			    ** row to point to this new table.
			    **
			    ** To accomplish this, we will open the
			    ** previous table, fetch the record, alter its next
			    ** pointer, rewrite it, and close the table.
			    */

			    if (DMZ_SES_MACRO(11))
				dmd_petrace("DMPE_PUT: Fixup required", 0, 0, 0);
			    status = dmpe_nextchain_fixup(pop_cb);
			    if (status)
				break;
			}
			/* Record the last successful put-to etab for any
			** future nextchain-fixup.
			*/
			STRUCT_ASSIGN_MACRO(pcb->pcb_dmtcb->dmt_id,
						pcb->pcb_table_previous);
		    }
		    else
		    {
			/* The put failed */

			if ((pcb->pcb_dmrcb->error.err_code !=
				    E_DM0112_RESOURCE_QUOTA_EXCEED)
			    &&  (pcb->pcb_dmrcb->error.err_code !=
				    E_DM0138_TOO_MANY_PAGES)
			   )
			{
			    pop_cb->pop_error = pcb->pcb_dmrcb->error;
			}
			pcb->pcb_dmtcb->dmt_flags_mask = 0;
			close_status = dmt_close(pcb->pcb_dmtcb);
			pcb->pcb_dmtcb->dmt_record_access_id = 0;
			pcb->pcb_dmrcb->dmr_access_id = 0;
			if (close_status)
			{
			    uleFormat(&pcb->pcb_dmtcb->error, 0, NULL, ULE_LOG,
					    NULL, (char *) NULL, 0L, (i4 *) NULL,
					    &err_code, 0);
			    uleFormat(NULL, E_DM9B05_DMPE_PUT_CLOSE_TABLE, NULL , ULE_LOG,
					    NULL, (char *) NULL, 0L, (i4 *) NULL,
					    &err_code, 0);
			    pop_cb->pop_error = pcb->pcb_dmtcb->error;
			    break;
			}
			if ((pcb->pcb_dmrcb->error.err_code !=
				    E_DM0112_RESOURCE_QUOTA_EXCEED)
			    &&  (pcb->pcb_dmrcb->error.err_code !=
				    E_DM0138_TOO_MANY_PAGES)
			   )
			{
			    /* Messages logged above... */
			    break;
			}
			else
			{
			    /* If we arrive and find that we've failed a */
			    /* couple of times to allocate space, don't */
			    /* keep trying.  System is out of space. */

			    failed++;
			    if (etlist_ptr != NULL)
				etlist_ptr->etl_status |= ETL_FULL_MASK;
			    if (failed >= 2)
			    {
				SETDBERR(&pop_cb->pop_error, 0,
				    pcb->pcb_dmrcb->error.err_code);
				break;
			    }
			    /* Else continue below with this etab closed,
			    ** trying to find a new etab
			    */
			}
		    } /* status != OK */
		}
		else
		{
		    DM_MDATA mdata;
		    i4 row_count;

		    /* We're doing a bulk-load.  If not first time, write
		    ** the held record (which may have been updated) using
		    ** the held access ID.  If at the end, write the
		    ** current record, else move current to hold.
		    ** Then, count off rows to see if the etab might be
		    ** full;  if so, mark this etab full, and signal for
		    ** a new etab (if more segments are to arrive).
		    */

		    status = E_DB_OK;
		    if ((pop_cb->pop_continuation & ADP_C_BEGIN_MASK) == 0)
		    {
			er = (DMP_RCB *) pcb->pcb_held_access_id;
			et = er->rcb_tcb_ptr;
			mdata.data_next = NULL;
			mdata.data_address = (char *) pcb->pcb_held_record;
			mdata.data_size = et->tcb_rel.relwid;
			row_count = 1;
			status = dm2r_load( er, et, DM2R_L_ADD,
					0, &row_count,
					&mdata, 0,
					(DM2R_BUILD_TBL_INFO *) NULL,
					&pcb->pcb_dmrcb->error);
		    }
		    if ((pop_cb->pop_continuation & ADP_C_END_MASK) == 0)
		    {
			MEcopy((PTR) pcb->pcb_record, sizeof(DMPE_RECORD),
				(PTR) pcb->pcb_held_record);
			pcb->pcb_held_access_id = pcb->pcb_dmrcb->dmr_access_id;
		    }
		    else if (status == E_DB_OK)
		    {
			er = (DMP_RCB *) pcb->pcb_dmrcb->dmr_access_id;
			et = er->rcb_tcb_ptr;
			mdata.data_next = NULL;
			mdata.data_address = (char *) pcb->pcb_record;
			mdata.data_size = et->tcb_rel.relwid;
			row_count = 1;
			status = dm2r_load( er, et, DM2R_L_ADD,
					0, &row_count,
					&mdata, 0,
					(DM2R_BUILD_TBL_INFO *) NULL,
					&pcb->pcb_dmrcb->error);
		    }
		    if (status != E_DB_OK)
		    {
			/* (one of) the writes failed.  Not much can be done
			** about it in bulk-load mode.
			** Null out dmtcb access ID so that error cleanup
			** outside the loop doesn't close anything.
			*/
			pop_cb->pop_error = pcb->pcb_dmrcb->error;
			pcb->pcb_dmtcb->dmt_record_access_id = NULL;
			break;
		    }
		    /* See if the etab can take more rows, or if the next
		    ** segment (either in this LOB or a new one) has to go
		    ** into a new etab.
		    */
		    if (++pcb->pcb_loaded >= pcb->pcb_per_etab)
		    {
			etlist_ptr->etl_status |= ETL_FULL_MASK;
			pcb->pcb_dmrcb->dmr_access_id = NULL;
			pcb->pcb_dmtcb->dmt_record_access_id = NULL;
			/* Leave dmt_id.db_tab_base alone for coupon */
			pcb->pcb_et = NULL;
		    }
		} /* if bulk-load */

		/* If we managed to put a row, and it's BEGIN, make sure
		** that the initial segment etab is recorded in the coupon.
		** Then, break out of the FOR loop.
		*/
		if (status == E_DB_OK)
		{
		    if (pop_cb->pop_continuation & ADP_C_BEGIN_MASK)
			output->cpn_etab_id = pcb->pcb_dmtcb->dmt_id.db_tab_base;
		    break;	/* Normal for-loop exit */
		}
	    } /* if dmr-access-id */


	    /* If we get this far, then we failed to insert the record,
	    ** or perhaps no etab has been opened yet.
	    */

	    /*
	    ** This is where things get tricky.  Here, we need
	    ** to go find a place to put the record.  This works as follows
	    **
	    **	1) Hunt down the base tcb's (found in the coupon) list
	    **	    of peripheral tables for this column.  Whenever one
	    **	    is found, open it and attempt an insert.  If that
	    **	    works, fine.  If not, then keep going.
	    **
	    **	2) If there are no tables into which the new tuple will
	    **	    fit, then search the catalog for other tables.  This
	    **	    has the side affect of adding new entries to the
	    **	    base tcb's list of extended tables -- for this
	    **	    column and others.  Once this search is complete,
	    **	    repeat step 1.
	    **
	    **	    Note that during step 2, if the catalog doesn't
	    **	    exist, it will be created.  This will happen in a
	    **	    separate transaction, so that once it is created, it
	    **	    will not be destroyed.  This means that databases which do
	    **	    not contain BLOBS will not have the catalog, and no database
	    **	    conversion will be necessary to use them.  This will be nice
	    **	    if marketting decides to price these optionally -- it
	    **	    needn't be an ongoing conversion issue.
	    **
	    **	3) If there is still no fit, then create a new
 	    **	    peripheral table for this column.  This will be
	    **	    added to the TCB's list of tables, so that other
	    **	    sessions will be able to make use of it.
	    */

	    if (pop_cb->pop_temporary == ADP_POP_TEMPORARY)
	    {
		/* Put to an anonymous holding temp.  Just grab a temp
		** table.  dmpe_temp will know to NOT put this one on any
		** et list, and to flag the table as a holding temp.
		*/
		status = dmpe_temp(pop_cb, att_id, &new_etab);
		if (status)
		    break;
		pcb->pcb_record->prd_r_next = new_etab;
		etlist_ptr = NULL;
	    }
	    else
	    {
		/* Put is to permanent etab.  Keep in mind that a
		** "permanent" etab might be a temporary table, if the
		** base table is a session temp!
		*/
		tcb = pcb->pcb_tcb;

		/* Protect ET list (even if gtt, because of || query) */
		dm0s_mlock(&tcb->tcb_et_mutex);

		etlist_ptr = pcb->pcb_et;
		if (etlist_ptr == NULL)
		    etlist_ptr = tcb->tcb_et_list->etl_next;
		else
		    etlist_ptr = etlist_ptr->etl_next;

		while (	(etlist_ptr != tcb->tcb_et_list)
		  && ((etlist_ptr->etl_etab.etab_att_id != att_id)
		      || (etlist_ptr->etl_etab.etab_type != DMP_LO_ETAB)
		      || (etlist_ptr->etl_status & (ETL_INVALID_MASK | ETL_FULL_MASK | ETL_LOAD))))
		{
		    /*
		    **	Skip over tables which are known to be full or are not
		    **	for this attribute.
		    */

		    etlist_ptr = etlist_ptr->etl_next;
		}
		dm0s_munlock(&tcb->tcb_et_mutex);

		/*
		**	It is important to note here that this scan list is
		**	safe to remember without semaphores.  This is true
		**	because the list is essentially non-decreasing.  Tables
		**	are not removed; they may shrink or become empty, but
		**	they are not removed.  Therefore, once they have
		**	appeared in memory, they are OK, and will not be
		**	removed.  Thus, one must only have the semaphore to
		**	move from one place to another.
		*/

		if (etlist_ptr != tcb->tcb_et_list)
		{
		    /* Found one, use it */
		    pcb->pcb_record->prd_r_next =
			    etlist_ptr->etl_etab.etab_extension;
		}
		else
		{
		    /*
		    **  Didn't find a usable one.
		    ** If we're dealing with a session temp base table,
		    ** create a new temp etab.  Otherwise we'll do a bit
		    ** fancier stuff for a new real permanent etab.
		    */
		    if (tcb->tcb_temporary)
		    {
			/* Create a temp etab.  Since pop_temporary isn't
			** set, a real session-temp etab will be created
			** and hooked onto the END of the TCB etab list.
			*/
			status = dmpe_temp(pop_cb, att_id, &new_etab);
			if (status)
			    break;
		    }
		    else
		    {
			/* Permanent base table.  If we haven't yet scanned
			** the et catalog for this attempt, scan it.  This
			** will add any new entries to the end of the TCB's
			** et-list.
			** Note that tcb-populate uses the PCB dmt/dmrcb's
			** to read the iiextended_relation catalog.
			*/

			if (!pcb->pcb_cat_scan)
			{
			    pcb->pcb_cat_scan = 1;
			    status = dmpe_tcb_populate(pop_cb);
			    if (status)
				break;
			    /* Maybe we added some, and it's no big deal to
			    ** just loop back around and look.
			    */
			    pcb->pcb_et = NULL;	/* Look at 'em all */
			    continue;
			}

			/* Looks like we'll need a new etab */
			status = dmpe_add_extension(pop_cb, tcb,
					att_id, &new_etab);
			if (status)
			    break;
		    }

		    /* Other sessions might be adding ET entries too, so
		    ** look for the newly added one (by table ID).
		    */
		    dm0s_mlock(&tcb->tcb_et_mutex);
		    etlist_ptr = tcb->tcb_et_list->etl_next;
		    while (etlist_ptr->etl_etab.etab_extension != new_etab
			   && etlist_ptr != tcb->tcb_et_list)
			etlist_ptr = etlist_ptr->etl_next;

		    dm0s_munlock(&tcb->tcb_et_mutex);
		    /* Double check that someone else didn't fill it? */
		    if (etlist_ptr->etl_status & (ETL_INVALID_MASK | ETL_FULL_MASK))
		    {
			/* huh?  whirl around to try another */
			pcb->pcb_et = NULL;
			continue;
		    }

		    pcb->pcb_record->prd_r_next = new_etab;
		} /* if found another etab */
	    } /* temporary/permanent */

	    /*
	    ** Now open the table indicated by prd_r_next.  If the table is
	    ** a real etab, etlist_ptr will point to its etlist entry.
	    */

	    pcb->pcb_et = etlist_ptr;
	    pcb->pcb_dmtcb->dmt_id.db_tab_base = pcb->pcb_record->prd_r_next;
	    pcb->pcb_dmtcb->dmt_flags_mask = DMT_DBMS_REQUEST;

	    pcb->pcb_dmtcb->dmt_update_mode = DMT_U_DIRECT;
	    pcb->pcb_dmtcb->dmt_access_mode = DMT_A_WRITE;
	    status = etab_open(pcb->pcb_dmtcb, pop_cb);
	    if (status)
	    {
		if (pcb->pcb_dmtcb->error.err_code != E_DM0054_NONEXISTENT_TABLE
		  || etlist_ptr == NULL )
		{
		    pop_cb->pop_error = pcb->pcb_dmtcb->error;
		    break;
		}
		else
		{
		    /* Extended table disappeared, probably because of
		    ** ROLLBACK. Mark it invalid and find another.
		    */
		    etlist_ptr->etl_status |= ETL_INVALID_MASK;
		    pcb->pcb_et = NULL;		/* Consider all of them */
		    continue;
		}
	    }
	    pcb->pcb_dmrcb->dmr_access_id =
			pcb->pcb_dmtcb->dmt_record_access_id;
	    if (pop_cb->pop_temporary == ADP_POP_PERMANENT
	       && pcb->pcb_bqcb->bqcb_load_etabs == BQCB_LOAD_YES)
	    {
		/* If we're bulk-loading etabs, start a bulk-load on this
		** newly chosen etab.  If that fails, give up.
		** If it works, record the bulk-loading state in the ETL
		** entry and in the pcb.
		** Note that once ETL_LOAD is lit, the table can't be closed
		** until we finish all pending loads at query end.
		*/
		er = (DMP_RCB *) pcb->pcb_dmrcb->dmr_access_id;
		status = dmpe_start_load(er, &pop_cb->pop_error);
		if (status != E_DB_OK)
		    break;
		etlist_ptr->etl_status |= ETL_LOAD;
		etlist_ptr->etl_access_id = er;
		etlist_ptr->etl_pcb = pcb;
		etlist_ptr->etl_sid = pcb->pcb_sid;
		/* If there's a held record waiting, fix its "next".
		** This is why we do the held record thing.
		*/
		if ((pop_cb->pop_continuation & ADP_C_BEGIN_MASK) == 0)
		{
		    pcb->pcb_held_record->prd_r_next = etlist_ptr->etl_etab.etab_extension;
		}
		pcb->pcb_loaded = 0;
		pcb->pcb_per_etab = dmpe_load_etab_limit(er->rcb_tcb_ptr);
	    } /* If starting bulk load */

	    /* Loop around and try inserting into table just chosen */
	} /* infinite for */

    } while (0);

    /* If error, or no-error and the last segment just went out, close
    ** the (current) etab unless we were bulk-loading it.  Bulk-loaded
    ** etabs are left open since more rows may be added.  (And, if error,
    ** multiple etabs might need to be cleaned up.)  Eventually, someone
    ** will call dmpe-query-end which will close out all of the
    ** outstanding etab loads (moving rows from sorter to etab, etc).
    ** Likewise, don't deallocate the PCB if we might be bulk-loading.
    */
    if ( (status != E_DB_OK || pop_cb->pop_continuation & ADP_C_END_MASK)
      && pcb != NULL
      && (pcb->pcb_bqcb == NULL
	  || pcb->pcb_bqcb->bqcb_load_etabs != BQCB_LOAD_YES))
    {
	if (pcb->pcb_dmtcb->dmt_record_access_id != NULL)
	{
	    pcb->pcb_dmtcb->dmt_flags_mask = 0;
	    close_status = dmt_close(pcb->pcb_dmtcb);
	    pcb->pcb_dmtcb->dmt_record_access_id = 0;
	    pcb->pcb_dmrcb->dmr_access_id = 0;
	    if (close_status)
	    {
		uleFormat(&pcb->pcb_dmtcb->error, 0, NULL, ULE_LOG,
			    NULL, (char *) NULL, 0L, (i4 *) NULL,
			    &err_code, 0);
		uleFormat(NULL, E_DM9B05_DMPE_PUT_CLOSE_TABLE, NULL , ULE_LOG,
			    NULL, (char *) NULL, 0L, (i4 *) NULL,
			    &err_code, 0);
	    }
	}
	dmpe_deallocate(pcb);
    }
    if (status != E_DB_OK || pop_cb->pop_continuation & ADP_C_END_MASK)
	pop_cb->pop_user_arg = NULL;

    return(status);

} /* dmpe_put */

/*
**{
** Name: dmpe_get	- Get a [portion of a] peripheral object.
**
** Description:
**      This routine fetches the next segment of a peripheral object.  This
**	is done using the saved information in the peripheral control block,
**	which is passed in in the pop_cb's user argument parameter.
**
**	The basic flow of control here is as follows:
**
**	    1) Check that the arguments are correct.
**	    2) If this is the first call,
**		    2) Allocate the PCB
**		    2a) Set the current segment number to NULL
**	       Else
**		    2) Increment the segment number
**	    3) If the current table is not the one out of which to get the next
**		segment,
**		3a) Close the current table,
**		3b) and open the new table.
**	    4) Get the next segment.  The next segment is the one for which the
**		logical key's match and has the lowest segment number >= the
**		incremented segment number mentioned above.
**	    5) If there is no next segment,
**		5a) Close the current table.
**		5b) Deallocate the PCB
**		5c) Clear the continuation indicator in the pop_cb.
**		5d) set return status to NO_MORE_ROWS-ish thing
**	    6) Return.
**
**	Note that gaps in the segment numbers may be possible.  This may happen
**	if all segments are not renumbered when a change to a large object
**	results in the deletion of a segment.  This routine does not require
**	that the update algorithm introduce gaps;  however, it seems prudent to
**	allow for the possibility.  Good defensiver programming and all that.
**
** Inputs:
**      pop_cb                              ADP_POP_CB control block...
**	    pop_continuation                 Continuation indicator...
**						ADP_C_BEGIN_MASK -- first call,
**                                              ADP_C_RANDOM_MASK --
**                                              want the segment
**                                              indicated in the
**                                              pop_segno# fields.
**					    Calls to dmpe-get should not use
**					    ADP_C_END_MASK, as that would mean
**					    that the operation is already over.
**          pop_segno0
**          pop_segno1                      Segment number to get if
**                                              ADP_C_RANDOM_MASK set
**                                              as above.
**	    pop_coupon			    Ptr to DB_DATA_VALUE containing the
**					    coupon to be used to retrieve the
**					    object.
**	    pop_segment                     Ptr to DB_DATA_VALUE to receive the
**					    output.  An error will occur if the
**					    output is of insufficient size (see
**					    dmpe_information for size
**					    determination.)
**	    pop_user_arg                    DMF's internal state.  This is set
**					    on the first call (see
**					    pop_continuation above), and must be
**					    presented unchanged for subsequent
**					    calls.
**	    pop_info			    Pointer to optional DB_BLOB_WKSP,
**					    used for passing info about the
**					    context, base table, and LOB att.
**					    If omitted, dmpe-get will use what
**					    it can find in the short-term part
**					    of the DMF coupon.
**
** Outputs:
**      pop_cb...
**	    *pop_output->db_data            Filled with the next segment.
**	    (pop_user_arg)                  Filled iff appropriate.
**
**	Returns:
**	    E_DB_WARN and E_AD7001_ADP_NONEXT if we're returning the last
**	    segment, or RANDOM requested and segment not found.
**	    E_DB_ERROR and E_AD7001 if we already returned the last segment
**	    and there's nothing more to return.
**	    E_DB_OK if segment returned and there's more.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      02-Jan-1990 (fred)
**          Created.
**      30-Jun-1993 (fred)
**          Fixed up null and zero length object handling.
**      23-Aug-1993 (fred)
**          Added CSswitch() call.  This will keep a session mucking
**          with a large object from consuming the whole server.  It
**          is currently called only from dmpe_get.  It is assumed
**          that dmpe_put()'s will either alternate with gets (in
**          dmpe_copy's) or will be called because of some higher
**          level, which will be doing CSswitch()'s as well.
**      27-Oct-1993 (fred)
**          Added random segment fetching.  Allow, by setting the
**          random mask in the continuation field, access to segments
**          in some random order.
**	31-jan-1994 (bryanp) B58465, B58466
**	    Check return code from adc_isnull.
**	    Check dmpe_close return code during error cleanup.
**      13-Apr-1994 (fred)
**          Removed alteration of error to ADP_GET_ERROR.  This is now
**          handled by our caller.  Removing this allows us to
**          centralize some error handling so that interrupts can be
**          managed correctly.
**      14-Apr-1994 (fred)
**          Save sequence number away in the xcb_seq_no field when
**          reading a table.  Later, when deleting large object
**          temporaries, we can be specific about the temporaries we
**          trash.  The assumption here is that we will tend to read
**          real tables (extant blobs) before creating temporaries.
**          Although this is not always true, it is true more often
**          than not, except during inserts.  And for inserts, the
**          temporaries are of a different variety, not subject to
**          these random deletions.
**	22-Aug-1996 (prida01)
**	    Open the extended table in read mode. Stops problems with
**	    pages still being fixed in the cache when we commit.
**	29-May-1997 (shero03)
**	    Set the ptr to the pcb null after deallocating it.
**	3-Apr-1998 (thaju02)
**	    Return an error, if coupon specifies a non-zero length, yet
**	    we have not retrieved any blob segments from the extension
**	    table.
**	29-jul-1998 (somsa01)
**	    Only run dmd_petrace() if log_trace is set and logging is on.
**	03-sep-1998 (somsa01)
**	    Only set run_petrace if we have a valid RCB.
**	10-sep-1998 (somsa01)
**	    dm0m_check() takes two parameters.
**	28-oct-1998 (somsa01)
**	    In the case of Global Temporary Extension Tables, make sure that
**	    they are placed on the sessions lock list, not the transaction
**	    lock list.  (Bug #94059)
**	 3-mar-1998 (hayke02)
**	    We now test for TCB_SESSION_TEMP in relstat2 rather than relstat.
**      13-jan-2000 (stial01)
**          Init pop_info to null for get
**	11-May-2004 (schka24)
**	    All temp etabs get locked on the session list now.
**       8-Jul-2004 (hanal04) Bug 112558 INGSRV2879
**          Provide a mechanism for a caller to indicate that they are
**          finished with the GET operation that they previously
**          initiated. This will ensure we free the pcb and close the
**          associated file, without the need to GET the entire blob.
**	13-Feb-2007 (kschendel)
**	    Remove the CSswitch call, there's a useful one in dmr_get now.
**	16-Apr-2010 (kschendel) SIR 123485
**	    Call dml-begin routine to get context set up.  Use context
**	    for opening etabs.  Using txn-logging as a key for DM11 tracing
**	    on gets is weird, delete.
*/
static DB_STATUS
dmpe_get(i4	  op_code ,
          ADP_POP_CB     *pop_cb )

{
    DMPE_PCB		*pcb;
    DB_STATUS		status, local_status;
    i4		err_code;
    ADP_PERIPHERAL	*p = (ADP_PERIPHERAL *) pop_cb->pop_coupon->db_data;
    DMPE_COUPON		*input = DMPE_CPN_FROM_DBV_MACRO(pop_cb->pop_coupon);
    i4			repositioned;
    u_i4		current_low_segment;
    DMP_RCB		*etab_rcb;

    if (ADF_ISNULL_MACRO(pop_cb->pop_coupon))
    {
	p->per_tag = ADP_P_COUPON;
	p->per_length0 = p->per_length1 = 0;
	SETDBERR(&pop_cb->pop_error, 0, E_AD7001_ADP_NONEXT);
	return(E_DB_ERROR);
    }
    else if ((p->per_length0 == 0) && (p->per_length1 == 0))
    {
	/* If it has no length, there's not much to get... */
	SETDBERR(&pop_cb->pop_error, 0, E_AD7001_ADP_NONEXT);
	return(E_DB_ERROR);
    }

    for (repositioned = 0;;)
    {
	if (pop_cb->pop_continuation & ADP_C_END_MASK)
	{
	    pcb = (DMPE_PCB *) pop_cb->pop_user_arg;
            status = E_DB_ERROR;
	    SETDBERR(&pop_cb->pop_error, 0, E_AD7001_ADP_NONEXT);
	    break;
	}

	if (	(pop_cb->pop_continuation & ADP_C_BEGIN_MASK)
	    ||  (pop_cb->pop_user_arg == NULL))
	{
	    status = dmpe_begin_dml(pop_cb, &pcb, FALSE);
	    if (status != E_DB_OK)
		return(status);
	}
	else
	{
	    pcb = (DMPE_PCB *) pop_cb->pop_user_arg;
	    if (    ((pop_cb->pop_continuation & ADP_C_RANDOM_MASK) ==  0)
		&&  (pcb->pcb_record->prd_r_next == 0))
	    {
		/* Then that's it. */
		status = E_DB_ERROR;
		SETDBERR(&pop_cb->pop_error, 0, E_AD7001_ADP_NONEXT);
		break;
	    }
	}

	if (pop_cb->pop_continuation & ADP_C_RANDOM_MASK)
	{
	    pcb->pcb_seg1_next = pop_cb->pop_segno1;
	    if (pcb->pcb_seg0_next = pop_cb->pop_segno0)
	    {
		/* There are no non-zero seg0's today. */
		status = E_DB_ERROR;
		SETDBERR(&pop_cb->pop_error, 0, E_AD7001_ADP_NONEXT);
		break;
	    }

	}
	else
	{
	    pcb->pcb_seg1_next = pcb->pcb_record->prd_segment1 + 1;
	    if (pcb->pcb_seg1_next == 0)
	    {
		pcb->pcb_seg0_next += 1;
	    }
	}
	if (!pcb->pcb_dmtcb->dmt_id.db_tab_base)
	{
	    pcb->pcb_record->prd_r_next = input->cpn_etab_id;
	}
	else if (pcb->pcb_dmtcb->dmt_id.db_tab_base !=
				pcb->pcb_record->prd_r_next)
	{
	    pcb->pcb_dmtcb->dmt_flags_mask = 0;
	    status = dmt_close(pcb->pcb_dmtcb);
	    pcb->pcb_dmtcb->dmt_record_access_id = NULL;
	    pcb->pcb_dmrcb->dmr_access_id = NULL;
	    if (status)
	    {
		pop_cb->pop_error = pcb->pcb_dmtcb->error;
		break;
	    }
	}

	etab_rcb = ((DMP_RCB *)pcb->pcb_dmrcb->dmr_access_id);
	if (etab_rcb == NULL)
	{
	    /*
	    **	If we have no table open, then open the next table,
	    **	whichever that is, and position based on our logical key.
	    */

	    pcb->pcb_dmtcb->dmt_id.db_tab_base = pcb->pcb_record->prd_r_next;
	    pcb->pcb_dmtcb->dmt_flags_mask = DMT_DBMS_REQUEST;

	    pcb->pcb_dmtcb->dmt_update_mode = DMT_U_DIRECT;
	    if (op_code == ADP_GET)
	    {
	    	pcb->pcb_dmtcb->dmt_access_mode = DMT_A_READ;
	    }
	    else
	    {
	    	pcb->pcb_dmtcb->dmt_access_mode = DMT_A_WRITE;
	    }

	    status = etab_open(pcb->pcb_dmtcb, pop_cb);
	    if (status)
	    {
		pop_cb->pop_error = pcb->pcb_dmtcb->error;
		break;
	    }
	    pcb->pcb_dmrcb->dmr_access_id =
		    pcb->pcb_dmtcb->dmt_record_access_id;

	    etab_rcb = ((DMP_RCB *)pcb->pcb_dmrcb->dmr_access_id);

	    pcb->pcb_dmrcb->dmr_flags_mask = 0;

	    if (etab_rcb->rcb_tcb_ptr->tcb_rel.relspec == TCB_HEAP)
		pcb->pcb_dmrcb->dmr_position_type = DMR_ALL;
	    else
	    {
		DMR_ATTR_ENTRY	**attr_entry =
				    (DMR_ATTR_ENTRY **)
				      pcb->pcb_dmrcb->dmr_attr_desc.ptr_address;

		pcb->pcb_dmrcb->dmr_position_type = DMR_QUAL;
		attr_entry[0]->attr_operator = DMR_OP_EQ;
		attr_entry[0]->attr_number = 1;
		attr_entry[0]->attr_value = (PTR) &input->cpn_log_key;

		attr_entry[1]->attr_operator = DMR_OP_EQ;
		attr_entry[1]->attr_number = 2;
		attr_entry[1]->attr_value = (PTR) &pcb->pcb_seg0_next;

		attr_entry[2]->attr_operator = DMR_OP_EQ;
		attr_entry[2]->attr_number = 3;
		attr_entry[2]->attr_value = (PTR) &pcb->pcb_seg1_next;

		/*
		** Position to pcb_seg1_next
		**
		** Warning: if you position to seg1 == pcb_seg1_next
		** this code will reposition for every segment!!!
		**
		** For BTREE etabs, we can position seg1 >= pcb_seg1_next,
		** This works when positioning to the first or a RANDOM segment
		** (and subsequent segments will be returned in segment order)
		**
		** For HASH we need to position to seg1 == pcb_seg1_next
		** (must specify all HASH keys)
		*/
		if (etab_rcb->rcb_tcb_ptr->tcb_rel.relspec == TCB_BTREE)
		    attr_entry[2]->attr_operator = DMR_OP_GTE;
	    }

	    status = dmr_position(pcb->pcb_dmrcb);

	    if (status)
	    {
		if (pcb->pcb_dmrcb->error.err_code == E_DM0074_NOT_POSITIONED
		 || pcb->pcb_dmrcb->error.err_code == E_DM008E_ERROR_POSITIONING)
		{
		    pcb->pcb_dmrcb->dmr_position_type = DMR_ALL;
		    repositioned++;
		    pcb->pcb_dmrcb->dmr_attr_desc.ptr_in_count = DMPE_KEY_COUNT;
		    status = dmr_position(pcb->pcb_dmrcb);
		}
		if (status)
		{
		    pop_cb->pop_error = pcb->pcb_dmrcb->error;
		    break;
		}
	    }
	}

	pcb->pcb_dmrcb->dmr_flags_mask = DMR_NEXT;

	do
	{
	    status = dmr_get(pcb->pcb_dmrcb);
	    if (status)
	    {
		repositioned += 1;
		if ((repositioned <= 2)
			&& (pcb->pcb_dmrcb->error.err_code == E_DM0055_NONEXT))
		{
		    /*
		    **	In this case, then we restart our scan at the begining
		    **	of this set.
		    */

		    pcb->pcb_dmrcb->dmr_flags_mask = DMR_REPOSITION;
		    pcb->pcb_dmrcb->dmr_attr_desc.ptr_in_count = DMPE_KEY_COUNT;
		    status = dmr_position(pcb->pcb_dmrcb);
		    if (status == E_DB_OK)
		    {
			pcb->pcb_dmrcb->dmr_flags_mask = DMR_NEXT;
			if (repositioned == 1)
			{
			    /*
			    **	Then we must keep track of the lowest segment
			    **	number found.  This information is used if the
			    **	segment numbers get fragmented, which can
			    **	happen in blobs updated via the cursor
			    **	mechanism.
			    */

				current_low_segment = MAXI4;
			}
			else if (repositioned == 2)
			{
			    /*
			    ** Then start find the segment of the element
			    ** mentioned above
			    */

			    pcb->pcb_seg1_next = current_low_segment;
			}
			continue;
		    }
		}
		else if (pcb->pcb_dmrcb->error.err_code == E_DM0055_NONEXT)
		{
		    status = E_DB_ERROR;
		    if (pop_cb->pop_continuation & ADP_C_RANDOM_MASK)
			status = E_DB_WARN;

		    SETDBERR(&pop_cb->pop_error, 0, E_AD7001_ADP_NONEXT);
		    /*
		    ** if we have repositioned twice and no blob
		    ** segment has been retrieved, yet the coupon
		    ** specifies a non-zero blob length, there is an
		    ** inconsistency, flag an error
		    */
		    if ( (op_code == ADP_GET) &&
				(p->per_length0 || p->per_length1) )
		    {
			/* No errors for ADP_C_RANDOM_MASK */
			if ((pop_cb->pop_continuation & ADP_C_RANDOM_MASK) == 0)
			    uleFormat(NULL, E_DM9B0B_DMPE_LENGTH_MISMATCH, NULL,
				ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
				0, p->per_length1, 0, p->per_length0);
			SETDBERR(&pop_cb->pop_error, 0, E_DM9B01_GET_DMPE_ERROR);
		    }
		    break;
		}
		pop_cb->pop_error = pcb->pcb_dmrcb->error;
		break;
	    }
	    if (repositioned == 1)
	    {
		/*
		**	Then we must keep track of the lowest segment
		**	number found.  This information is used if the
		**	segment numbers get fragmented, which can
		**	happen in blobs updated via the cursor
		**	mechanism.
		*/

		if (pcb->pcb_record->prd_segment1
				< current_low_segment)
		{
		    current_low_segment =
			pcb->pcb_record->prd_segment1;
		}
	    }
	} while (   (pcb->pcb_record->prd_segment1 != pcb->pcb_seg1_next)
		||  (pcb->pcb_record->prd_segment0 != pcb->pcb_seg0_next)
		||  (pcb->pcb_record->prd_log_key.tlk_high_id !=
					    input->cpn_log_key.tlk_high_id)
		||  (pcb->pcb_record->prd_log_key.tlk_low_id !=
					    input->cpn_log_key.tlk_low_id));

	if (status == E_DB_OK)
	{
	    if (DMZ_SES_MACRO(11))
	    {
		dmd_petrace("DMPE_GET", pcb->pcb_record,
		    0,  /* base table id */
		    pcb->pcb_dmtcb->dmt_id.db_tab_base);
	    }

	    /* Return the segment number fetched */

	    pop_cb->pop_segno0 = pcb->pcb_seg0_next;
	    pop_cb->pop_segno1 = pcb->pcb_seg1_next;

	    if (pop_cb->pop_segment)
	    {
		MEcopy((PTR) pcb->pcb_record->prd_user_space,
		    min(pop_cb->pop_segment->db_length,
				sizeof(pcb->pcb_record->prd_user_space)),
		    (PTR) pop_cb->pop_segment->db_data);
	    }
	    if (pcb->pcb_record->prd_r_next == 0)
	    {
		status = E_DB_WARN;
		SETDBERR(&pop_cb->pop_error, 0, E_AD7001_ADP_NONEXT);
	    }
	}
	break;
    }

    if (status)
    {
	if (pop_cb->pop_error.err_code != E_AD7001_ADP_NONEXT)
	{
	    if (pcb->pcb_dmtcb->dmt_record_access_id)
	    {
		pcb->pcb_dmtcb->dmt_flags_mask = 0;
		local_status = dmt_close(pcb->pcb_dmtcb);
		pcb->pcb_dmtcb->dmt_record_access_id = 0;
		if (local_status)
		    uleFormat( &pcb->pcb_dmtcb->error, 0,
			    (CL_ERR_DESC *)NULL, ULE_LOG,
			    NULL, (char *) NULL, 0L, (i4 *) NULL,
			    &err_code, 0);
	    }
    	}
	else
	{
	    if (pop_cb->pop_continuation & ADP_C_END_MASK)
	    {
		status = E_DB_OK;
		CLRDBERR(&pop_cb->pop_error);
            }
	    if ((op_code == ADP_GET) &&
		    (pcb->pcb_dmtcb->dmt_record_access_id))
	    {
		pcb->pcb_dmtcb->dmt_flags_mask = 0;
		local_status = dmt_close(pcb->pcb_dmtcb);
		pcb->pcb_dmtcb->dmt_record_access_id = 0;
		if (local_status)
		    uleFormat( &pcb->pcb_dmtcb->error, 0,
			    (CL_ERR_DESC *)NULL, ULE_LOG,
			    NULL, (char *) NULL, 0L, (i4 *) NULL,
			    &err_code, 0);
	    }
	}

	if (op_code == ADP_GET)
	{
	    dmpe_deallocate(pcb);
	    pop_cb->pop_user_arg = NULL;
	}
    }
    return(status);
}

/*
**{
** Name: dmpe_delete	- Delete a peripheral object
**
** Description:
**      This routine deletes the tuples in the various extension tables which
**	make up the peripheral objects.  Only the coupon can be passed in;  this
**	routine will delete all extension tuples.
**
** Inputs:
**      pop_cb                          ADF's peripheral operation control block
**	    pop_input			DB_DATA_VALUE describing the coupon
**					whose component parts are to be deleted.
**
** Outputs:
**      pop_cb
**	    pop_error			Indicator of problems
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      03-Jan-1990 (fred)
**          Created.
**      30-Jun-1993 (fred)
**          Fixedup null and zero length object handling.
**      12-Aug-1993 (fred)
**          Add capability to delete temporary large objects.
**	31-jan-1994 (bryanp) B58467, B58468
**	    Check return code from adc_isnull in cpn==-1 case.
**	    Check dmpe_close return code during error cleanup.
**      13-Apr-1994 (fred)
**          Alter error processing to pass on interrupt messages.
**          This will allow calling code to avoid logging of [non]
**          errors in this case.
**	31-Jul-1996 (kch)
**	    We now check for the existence of a list of extended tables
**	    (pcb->pcb_rcb->rcb_tcb_ptr->tcb_et_list) before attempting to
**	    operate on it. This prevents a 'delete from session.temp' causing
**	    a SEGV if the sesion.temp contains a blob column.
**      24-mar-1998 (thaju02)
**          Bug #87130 - Holding tcb_et_mutex while retrieving tuples to be
**          deleted from extension table. Modified dmpe_delete().
**	15-apr-1999 (somsa01)
**	    For my_pop_cb, initialize pop_info to NULL.
**	11-May-2004 (schka24)
**	    When evaporating an anonymous temp, take it off the temp-cleanup
**	    list.  Only mark et list entry not-full once per etab, and
**	    look for the etab ID, not the base table ID!
**	21-Jun-2004 (schka24)
**	    Get rid of separate temp-cleanup list completely.
**	16-Apr-2010 (kschendel) SIR 123485
**	    Caller will supply blob info, don't ignore it.
**	    Delete bogus extra loop around segment delete loop.
*/
DB_STATUS
dmpe_delete(ADP_POP_CB     *pop_cb )
{
    DB_STATUS           status, local_status;
    ADP_POP_CB		my_pop_cb;
    i4		err_code;
    DMPE_PCB		*pcb;
    DMP_ET_LIST		*list;
    ADP_PERIPHERAL	*p = (ADP_PERIPHERAL *) pop_cb->pop_coupon->db_data;
    DMPE_COUPON		*cpn;
    i4			last_etab_base, cur_etab_base;
    bool                first_seg = TRUE;
    DMP_TCB             *t;
    DMP_TCB		*base_tcb;
    DMP_RCB		*etab_rcb;

#ifdef xDEBUG
    if (DMZ_SES_MACRO(11))
	dmd_petrace("DMPE_DELETE requested", 0, 0 , 0);
#endif

    CLRDBERR(&pop_cb->pop_error);

    if (ADF_ISNULL_MACRO(pop_cb->pop_coupon))
    {
	return(E_DB_OK);
    }
    else if ((p->per_length0 == 0) && (p->per_length1 == 0))
    {
	/* If it has no length, there's not much to delete... */
	return(E_DB_OK);
    }

    CLRDBERR(&my_pop_cb.pop_error);
    my_pop_cb.pop_segment = NULL;
    my_pop_cb.pop_user_arg = NULL;
    my_pop_cb.pop_info = pop_cb->pop_info;

    cpn = (DMPE_COUPON *) &p->per_value.val_coupon;

    if (cpn->cpn_flags & DMPE_CPN_TEMP)
    {
	DMT_CB		dmt_cb;

	/* Then object is a temporary -- just evaporate it */

	MEfill(sizeof(DMT_CB), 0, (PTR) &dmt_cb);
	dmt_cb.length = sizeof(DMT_CB);
	dmt_cb.type = DMT_TABLE_CB;
	dmt_cb.ascii_id = DMT_ASCII_ID;

	status = dmpe_qdata(&my_pop_cb, &dmt_cb);

	if (DB_SUCCESS_MACRO(status))
	{
	    dmt_cb.dmt_id.db_tab_base = cpn->cpn_etab_id;
	    dmt_cb.dmt_id.db_tab_index = 0;

	    status = dmt_delete_temp(&dmt_cb);
	    my_pop_cb.pop_error.err_code = dmt_cb.error.err_code;
	    if (status != E_DB_OK &&
		dmt_cb.error.err_code == E_DM0054_NONEXISTENT_TABLE)
	    {
		/* temp probably destroyed in query clean up */
		status = E_DB_OK;
	    }
	}
	/* Tidy up, just in case. */
	cpn->cpn_flags &= ~DMPE_CPN_TEMP;
	cpn->cpn_etab_id = 0;
    }
    else
    {
	STRUCT_ASSIGN_MACRO(*pop_cb, my_pop_cb);
	my_pop_cb.pop_continuation = ADP_C_BEGIN_MASK;
	my_pop_cb.pop_segment = NULL;
	my_pop_cb.pop_user_arg = NULL;
	last_etab_base = 0;

	/*
	 **  Mark that we don't really want the input tuple back.
	 **  We don't really care what it is, we just need to position
	 **  ourselves so that the dmr_delete() will work.
	 */

	do
	{
	    status = dmpe_get(ADP_DELETE, &my_pop_cb);
	    if (status)
	    {
		if (status != E_DB_WARN
		  || my_pop_cb.pop_error.err_code != E_AD7001_ADP_NONEXT)
		    break;
		my_pop_cb.pop_continuation |= ADP_C_END_MASK;
	    }
	    my_pop_cb.pop_continuation &= ~ADP_C_BEGIN_MASK;

	    pcb = (DMPE_PCB *) my_pop_cb.pop_user_arg;

	    /* Here if we must delete each segment */
	    pcb->pcb_dmrcb->dmr_flags_mask = DMR_CURRENT_POS;

	    status = dmr_delete(pcb->pcb_dmrcb);
	    if (status)
	    {
		my_pop_cb.pop_error = pcb->pcb_dmrcb->error;
		break;
	    }
	    cur_etab_base = ((DMP_RCB *)pcb->pcb_dmrcb->dmr_access_id)->
				    rcb_tcb_ptr->tcb_rel.reltid.db_tab_base;
	    t = pcb->pcb_tcb;
	    if (t->tcb_et_list && cur_etab_base != last_etab_base)
	    {
		/* Mark this etab not-full */
		dm0s_mlock(&t->tcb_et_mutex);

		for (list = t->tcb_et_list->etl_next;
		    list != t->tcb_et_list;
		    list = list->etl_next)
		{
		    if (list->etl_etab.etab_type == DMP_LO_ETAB
		       && list->etl_etab.etab_extension == cur_etab_base)
		    {
			list->etl_status &= ~ETL_FULL_MASK;
			break;
		    }
		}

		dm0s_munlock(&t->tcb_et_mutex);
		last_etab_base = cur_etab_base;
	    }
	} while ((my_pop_cb.pop_continuation & ADP_C_END_MASK) == 0);
    }
    if (status)
    {
	if ((my_pop_cb.pop_error.err_code == E_DM0065_USER_INTR) ||
            (my_pop_cb.pop_error.err_code == E_DM016B_LOCK_INTR_FA))
	{
	    STRUCT_ASSIGN_MACRO(my_pop_cb.pop_error, pop_cb->pop_error);
	}
	else if (my_pop_cb.pop_error.err_code != E_AD7001_ADP_NONEXT)
	{
	    uleFormat(&my_pop_cb.pop_error, 0, NULL , ULE_LOG,
			    NULL, (char *) NULL, 0L, (i4 *) NULL,
			    &err_code, 0);
	    SETDBERR(&pop_cb->pop_error, 0, E_AD7008_ADP_DELETE_ERROR);
	}
	else
	{
	    status = E_DB_OK;
	}
    }
    pcb = (DMPE_PCB *) my_pop_cb.pop_user_arg;

    if (pcb)
    {
	if (pcb->pcb_dmtcb->dmt_record_access_id)
	{
	    pcb->pcb_dmtcb->dmt_flags_mask = 0;
	    local_status = dmt_close(pcb->pcb_dmtcb);
	    if (local_status)
		uleFormat( &pcb->pcb_dmtcb->error, 0,
			(CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *) NULL, 0L, (i4 *) NULL,
			&err_code, 0);
	    pcb->pcb_dmtcb->dmt_record_access_id =
		pcb->pcb_dmrcb->dmr_access_id = 0;
	}
	dmpe_deallocate(pcb);
    }

    return(status);
}

/*
** {
** Name: dmpe_move	- Move an entire peripheral object
**
** Description:
**      This routine moves a peripheral object from one place to another.  This
**	is implemented by performing gets and puts on each object.
**
** Inputs:
**      pop_cb                          The peripheral operations control block
**	    pop_segment			The input object -- in this case, it is
**					a coupon.
**	    pop_coupon			The coupon to which to move.
**	    pop_info			Target table info (attid, etc)
**	cleanup_source			TRUE if this is a final move of an
**					object into its resting etab upon a
**					put or replace, and the source can
**					be cleaned up if it's a holding temp.
**					FALSE means that the source might
**					possibly be referenced again and should
**					be left alone, someone else will clean
**					up eventually.
**
** Outputs:
**      pop_cb.pop_error                Set if appropriate.
**	      .pop_coupon		Filled in with a completed coupon.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-Jan-1990 (fred)
**          Created.
**	15-nov-93 (swm)
**	    Bug #58633
**	    Rounded up under_dv.db_length to guarantee pointer-alignment
**	    after dm0m_allocate() memory allocation.
**	31-jan-1994 (bryanp) B58469, B58470
**	    Check return code from adc_isnull.
**	    If adi_per_under fails, set pop_error before returning.
**      13-Apr-1994 (fred)
**          Alter error processing to correctly reflect interrupt
**          errors back to caller.
**	28-jul-1998 (somsa01)
**	    Pass load_blob to dmpe_put().  (Bug #92217)
**	8-apr-99 (stephenb)
**	    init pop_info fields in pop cb's
**	11-May-2004 (schka24)
**	    Add cleanup-source parameter.
**	11-May-2007 (gupsh01)
**	    Added support for UTF8 character set.
**	17-Apr-2008 (kibro01) b120276
**	    Initialise ADF_CB structure
[@history_template@]...
*/
DB_STATUS
dmpe_move(ADP_POP_CB *pop_cb, bool cleanup_source)
{
    DB_STATUS           status;
    DB_DATA_VALUE	under_dv;
    DB_DATA_VALUE	seg_dv;
    ADF_CB		adf_cb;
    DM_OBJECT		*object = 0;
    char		*tuple;
    ADP_POP_CB		*get_pop;
    ADP_POP_CB		*put_pop;
    i4			err_code;

    CLRDBERR(&pop_cb->pop_error);

    if (DMZ_SES_MACRO(11))
	dmd_petrace("DMPE_MOVE requested", 0, 0 , 0);

    if ( (((ADP_PERIPHERAL *) pop_cb->pop_segment->db_data)->per_length0 == 0)
	&& (((ADP_PERIPHERAL *) pop_cb->pop_segment->db_data)->per_length1 == 0)
       )
    {
	/* If nothing to move, get out fast */
	return(E_DB_OK);
    }

    if (ADF_ISNULL_MACRO(pop_cb->pop_segment))
    {
	((ADP_PERIPHERAL *) pop_cb->pop_coupon->db_data)->per_length0 = 0;
	((ADP_PERIPHERAL *) pop_cb->pop_coupon->db_data)->per_length1 = 0;
	/* Wonder why we don't set NULL if coupon dv is nullable? */
	return(E_DB_OK);
    }

    MEfill(sizeof(ADF_CB),0,(PTR)&adf_cb);
    adf_cb.adf_maxstring = DMPE_SEGMENT_LENGTH;

    if (CMischarset_utf8())
      adf_cb.adf_utf8_flag = AD_UTF8_ENABLED;
    else
      adf_cb.adf_utf8_flag = 0;

    do
    {
	/*
	**	Determine what underlying format ADF thinks is good
	**	dmpe-temp needs underdv set too.
	*/
	pop_cb->pop_underdv = &under_dv;
	status = adi_per_under(&adf_cb,
				pop_cb->pop_segment->db_datatype,
				&under_dv);
	if (status)
	{
	    pop_cb->pop_error = adf_cb.adf_errcb.ad_dberror;
	    break;
	}

	status = dm0m_allocate(sizeof(DM_OBJECT)
			+ DB_ALIGN_MACRO(under_dv.db_length)
			+ (2 * sizeof(ADP_POP_CB)),
			(i4) 0,
			(i4) PCB_CB,
			(i4) PCB_ASCII_ID,
			(i4) 0,
			(DM_OBJECT **) &object,
			&pop_cb->pop_error);
	if (status)
	    break;
	tuple = (char *) ((char *) object + sizeof(DM_OBJECT));
	get_pop = (ADP_POP_CB *) (tuple +
			DB_ALIGN_MACRO(under_dv.db_length));
	put_pop = (ADP_POP_CB *) ((char *) get_pop + sizeof(ADP_POP_CB));
	STRUCT_ASSIGN_MACRO(*pop_cb, *get_pop);
	STRUCT_ASSIGN_MACRO(*pop_cb, *put_pop);

	get_pop->pop_coupon = pop_cb->pop_segment;
	get_pop->pop_segment = &seg_dv;
	put_pop->pop_segment = &seg_dv;
	get_pop->pop_continuation = ADP_C_BEGIN_MASK;
	put_pop->pop_continuation = ADP_C_BEGIN_MASK;
	CLRDBERR(&get_pop->pop_error);
	CLRDBERR(&put_pop->pop_error);
	STRUCT_ASSIGN_MACRO(under_dv, seg_dv);
	seg_dv.db_data = (PTR) tuple;
	get_pop->pop_info = NULL;
	put_pop->pop_info = pop_cb->pop_info;

	do
	{
	    status = dmpe_get(ADP_GET, get_pop);
	    if (status)
	    {
		if (get_pop->pop_error.err_code != E_AD7001_ADP_NONEXT)
		    break;
		/* Normal exit from the loop after the next put */
		CLRDBERR(&get_pop->pop_error);
		put_pop->pop_continuation |= ADP_C_END_MASK;
	    }

	    status = dmpe_put(put_pop);
	    if (status)
		break;
	    get_pop->pop_continuation &= ~ADP_C_BEGIN_MASK;
	    put_pop->pop_continuation &= ~ADP_C_BEGIN_MASK;
	} while ((put_pop->pop_continuation & ADP_C_END_MASK) == 0);
	if (get_pop->pop_error.err_code || put_pop->pop_error.err_code)
	{
	    if (get_pop->pop_error.err_code)
	    {
		if ((get_pop->pop_error.err_code != E_DM0065_USER_INTR) &&
                    (get_pop->pop_error.err_code != E_DM016B_LOCK_INTR_FA))
		    uleFormat(&get_pop->pop_error, 0, NULL , ULE_LOG,
			    NULL, (char *) NULL, 0L, (i4 *) NULL,
			    &err_code, 0);
		STRUCT_ASSIGN_MACRO(get_pop->pop_error, pop_cb->pop_error);
	    }

	    if (put_pop->pop_error.err_code)
	    {
		if ((put_pop->pop_error.err_code != E_DM0065_USER_INTR) &&
                    (put_pop->pop_error.err_code != E_DM016B_LOCK_INTR_FA))
		    uleFormat(&put_pop->pop_error, 0, NULL , ULE_LOG,
			    NULL, (char *) NULL, 0L, (i4 *) NULL,
			    &err_code, 0);
		STRUCT_ASSIGN_MACRO(put_pop->pop_error, pop_cb->pop_error);
	    }
	    break;
	}
	else
	{
	    if (cleanup_source)
	    {
		/* Caller says we can clean up source, see if it's an
		** anonymous holding tank.  If we don't clean up, it
		** happens eventually (at query end via adu_free_objects,
		** or session end), but early cleanup reduces the
		** number of temp files floating around.
		*/

		DMPE_COUPON *cpn = DMPE_CPN_FROM_DBV_MACRO(get_pop->pop_coupon);

		if (cpn->cpn_flags & DMPE_CPN_TEMP)
		{
		    /* Delete of anon object is just a temp file drop */
		    (void) dmpe_delete(get_pop);
		}
	    }
	    status = E_DB_OK;
	}
    } while (0);

    if (object)
	(VOID) dm0m_deallocate(&object);

    return(status);
}

/*{
** Name: dmpe_information	- Supply caller with operational characteristics
**
** Description:
**      This routine supplies the caller with operational characteristics about
**	the peripheral datatype service system.
**
** Inputs:
**      pop_cb                          ADP_POP_CB for calling
**	    pop_coupon			Coupon about which characteristics
**					are required.
**
** Outputs:
**      pop_cb
**          pop_underdv.db_length	Maximum length for underlying portions
**					of the datatype.
**	    pop_error			Indication of success
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      03-Jan-1990 (fred)
**          Created.
[@history_template@]...
*/
static DB_STATUS
dmpe_information(ADP_POP_CB     *pop_cb )

{
    static  i4             dmpe_information_used = 0;
    static  i4             dmpe_length = DMPE_CPS_COMPONENT_PART_SIZE;

    CLRDBERR(&pop_cb->pop_error);

    /*
    ** When multiple page sizes are supported, then this routine will have to
    ** adjust for the underlying page size in use.  That's why the input
    ** coupon is required.  In the mean time, just return a constant w/in the
    ** DBMS.
    */

    /*
    **	If compiled for debugging, and this routine hasn't been used before in
    **	this server, then if trace point DM1430 is set, then set the size to be
    **	used in this server to the test size declared in the header.  If the
    **	routine has already been used, then leave the setting alone.  This trace
    **	point may only be used in a DB which currently has no blobs.  Failure to
    **	observe this protocol will cause unpredictable results.
    */

#ifdef	xDEBUG
    if ((!dmpe_information_used++)
				 && DMZ_TBL_MACRO(12))
	dmpe_length = DMPE_TCPS_TST_COMP_PART_SIZE;
#endif
    pop_cb->pop_underdv->db_length = dmpe_length;

    return(E_DB_OK);
}

/*{
** Name: dmpe_destroy	- Destroy a table's extensions
**
** Description:
**      This routine destroys all extension tables to a given table.
**	It is given the table id of the parent table.  The parent table
**	will be a regular table, not a session temp table.  (etabs for
**	session temporary tables are handled in dmu.)
**
**	Based upon this id, the routine scans the iiextended_relation catalog.
**	Any table whose parent matches the table id passed in is destroyed.
**
**	The base TCB et-list is not maintained, because this is called when
**	the base table itself is being deleted.
**
** Inputs:
**	base_dmu			Ptr to dmu_cb for which this is being
**					performed.
**      error                           Ptr to i4 to contain any error
**					encountered.
**
** Outputs:
**      *error                          Filled if appropriate.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      28-feb-1990 (fred)
**          Created.
**	31-jan-1994 (bryanp) B58475
**	02-jul-1998 (shust01)
**	    Initialized cpn_rcb for fake peripheral, since it
**	    it is used further down and has the potential to SEGV.
**	15-Apr-2010 (kschendel) SIR 123485
**	    Don't need a fake any more.
*/
DB_STATUS
dmpe_destroy(DMU_CB	  *base_dmu ,
              DB_ERROR	  *dberr )

{
    DB_STATUS           status;
    ADP_POP_CB		pop_cb;
    DMPE_PCB		*pcb;
    DMP_ETAB_CATALOG	etab_record;
    DMU_CB		dmu_cb;

    CLRDBERR(&pop_cb.pop_error);

    if (DMZ_SES_MACRO(11))
	dmd_petrace("DMPE_DESTROY requested", 0, 0 , 0);

    pop_cb.pop_temporary = ADP_POP_TEMPORARY;
    pop_cb.pop_coupon = NULL;
    pop_cb.pop_info = NULL;

    dmu_cb.type = DMU_UTILITY_CB;
    dmu_cb.length = sizeof(dmu_cb);
    dmu_cb.dmu_tran_id = base_dmu->dmu_tran_id;
    dmu_cb.dmu_db_id = base_dmu->dmu_db_id;
    dmu_cb.dmu_flags_mask = 0;
    dmu_cb.dmu_char_array.data_address = (PTR) 0;
    dmu_cb.dmu_char_array.data_in_size = 0;
    dmu_cb.dmu_char_array.data_out_size = 0;

    status = dmpe_allocate(&pop_cb, &pcb);
    if (status)
    {
	*dberr = pop_cb.pop_error;
	return(status);
    }

    while (status == E_DB_OK)
    {
	status = dmpe_cat_scan(&pop_cb, &base_dmu->dmu_tbl_id, &etab_record);

	if (status)
	    break;
	pcb->pcb_dmrcb->dmr_flags_mask = DMR_CURRENT_POS;
	status = dmr_delete(pcb->pcb_dmrcb);
	if (status)
	{
	    pop_cb.pop_error = pcb->pcb_dmrcb->error;
	    break;
	}
	dmu_cb.dmu_tbl_id.db_tab_base = etab_record.etab_extension;
	dmu_cb.dmu_tbl_id.db_tab_index = 0;
	status = dmu_destroy(&dmu_cb);
	pop_cb.pop_error = dmu_cb.error;
	*dberr = pop_cb.pop_error;
    }
    if (status == E_DB_WARN && pop_cb.pop_error.err_code == E_DM0055_NONEXT)
    {
	status = E_DB_OK;
	CLRDBERR(&pop_cb.pop_error);
    }

    dmpe_deallocate(pcb);

    *dberr = pop_cb.pop_error;
    return(status);
}

/*{
** Name: dmpe_modify	- This routine modifies all extension table to their
**			    appropriate structure.
**
** Description:
**      This routine modifies extension tables to a given table.
**	It is based on the table id of the parent table.
**
**	Based upon this id, the routine scans the iiextended_relation catalog.
**	Any table whose parent matches the table id passed in is modified.
**
**	The modification takes place to the internally known structure required
**	of extension tables (hash on logical key).  However, there are two
**	special cases.  If the base table is being relocated, then the extension
**	tables are relocated to the same locations.  If the base table is being
**	modified to truncated, then the extensions are first truncated, then
**	remodified to hash on logical key.
**
** Inputs:
**	base_dmu			Ptr to dmu_cb for which this is being
**					performed.
**      error                           Ptr to i4 to contain any error
**					encountered.
**
** Outputs:
**      *error                          Filled if appropriate.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      28-feb-1990 (fred)
**          Created.
**	18-Nov-1992 (fred)
**	    Added support for modify to truncated.
**	31-jan-1994 (bryanp) B58476
**	    Check return code from dmpe_allocate.
**	02-jul-1998 (shust01)
**	    Initialized cpn_rcb for fake peripheral, since it
**	    it is used further down and has the potential to SEGV.
**	20-aug-1998 (somsa01)
**	    There is no reason to run dmpe_cat_scan() for temporary
**	    extension tables. Instead, we have to manually walk the
**	    tcb_et_list and grab the temporary extension table ids
**	    from there.
**      06-oct-1998 (stial01)
**          Disallow BTREE extension tables (B92468)
**	28-oct-1998 (somsa01)
**	    In the case of Global Temporary Extension Tables, make sure that
**	    they are placed on the sessions lock list, not the transaction
**	    lock list.  (Bug #94059)
**	14-dec-1998 (thaju02)
**	    Moved switch case for check to disallow btree extension tables
**	    into scdopt.c. If the dmf_svcb->svcb_blob_etab_struct == BTREE,
**	    we were setting the dmf_svcb->svcb_blob_etab_struct to hash,
**	    but were neglecting to change the char_entry structure value,
**	    which was set to btree.
**	15-Apr-2010 (kschendel) SIR 123485
**	    Don't need a fake coupon any more.
*/
DB_STATUS
dmpe_modify(DMU_CB	  *base_dmu ,
             DMP_DCB	  *dcb ,
             DML_XCB	  *xcb ,
             DB_TAB_ID	  *tbl_id ,
             i4	  db_lockmode ,
             i4	  temporary ,
             i4	  truncated ,
	     i4   base_page_size,
	     i4   blob_add_extend,
             DB_ERROR	  *dberr )

{
    DB_STATUS           status;
    ADP_POP_CB		pop_cb;
    DMPE_PCB		*pcb;
    DMP_ETAB_CATALOG	etab_record;
    DMU_CB		dmu_cb;
    DMU_KEY_ENTRY	key_ents[DMPE_KEY_COUNT];
    DMU_KEY_ENTRY	*keys[DMPE_KEY_COUNT];
    DMU_CHAR_ENTRY	char_entry[5];
    i4			i;
    i4		ind = 0;
    DMP_RCB		*rcb = (DMP_RCB *)0;
    DMP_TCB		*t;
    DMP_ET_LIST		*etlist_ptr = (DMP_ET_LIST *)0;
    i4		local_error;
    bool		base_open = FALSE;
    DB_ERROR		local_dberr;

    CLRDBERR(&pop_cb.pop_error);

    if (DMZ_SES_MACRO(11))
	dmd_petrace("DMPE_MODIFY requested", 0, 0 , 0);

    pop_cb.pop_temporary = ADP_POP_TEMPORARY;
    pop_cb.pop_coupon = NULL;
    pop_cb.pop_info = NULL;

    MEfill(sizeof(DMU_CB), 0, (PTR) &dmu_cb);
    dmu_cb.type = DMU_UTILITY_CB;
    dmu_cb.length = sizeof(dmu_cb);
    dmu_cb.dmu_tran_id = base_dmu->dmu_tran_id;
    dmu_cb.dmu_db_id = base_dmu->dmu_db_id;
    dmu_cb.dmu_flags_mask = 0;
    dmu_cb.dmu_part_def = NULL;
    dmu_cb.dmu_ppchar_array.data_address = NULL;
    dmu_cb.dmu_ppchar_array.data_in_size = 0;

    if (blob_add_extend)
    {
	char_entry[0].char_id = DMU_ADD_EXTEND;
	char_entry[0].char_value = DMU_C_ON;
	char_entry[1].char_id = DMU_EXTEND;
	char_entry[1].char_value = blob_add_extend;
	dmu_cb.dmu_char_array.data_address = (char *) char_entry;
	dmu_cb.dmu_char_array.data_in_size = sizeof(DMU_CHAR_ENTRY)*2;
	dmu_cb.dmu_key_array.ptr_address = NULL;
	dmu_cb.dmu_key_array.ptr_size = 0;
	dmu_cb.dmu_key_array.ptr_in_count = 0;
	dmu_cb.dmu_location.data_in_size = 0;
    }
    else
    {
	for (i = 0; i < DMPE_KEY_COUNT; i++)
	{
	    keys[i] = &key_ents[i];
	}

	MEmove(7, "per_key", ' ',
		sizeof(key_ents[0].key_attr_name),
	       (PTR) &key_ents[0].key_attr_name);
	key_ents[0].key_order = DMU_ASCENDING;	/* Useless! */

	MEmove(12, "per_segment0", ' ',
		sizeof(key_ents[1].key_attr_name),
	       (PTR) &key_ents[1].key_attr_name);
	key_ents[1].key_order = DMU_ASCENDING;	/* Useless! */

	MEmove(12, "per_segment1", ' ',
		sizeof(key_ents[2].key_attr_name),
	       (PTR) &key_ents[2].key_attr_name);
	key_ents[2].key_order = DMU_ASCENDING;	/* Useless! */

	dmu_cb.dmu_location.data_address = base_dmu->dmu_location.data_address;
	dmu_cb.dmu_location.data_in_size = base_dmu->dmu_location.data_in_size;

	char_entry[0].char_id = DMU_STRUCTURE;
	char_entry[0].char_value = dmf_svcb->svcb_blob_etab_struct;
	char_entry[1].char_id = DMU_EXTEND;
	char_entry[1].char_value = DMPE_DEF_TBL_EXTEND;
	ind = 1;

	switch (dmf_svcb->svcb_blob_etab_struct) {
	    case DB_HASH_STORE :
		    char_entry[++ind].char_id = DMU_MINPAGES;
		    char_entry[ind].char_value = DMPE_DEF_TBL_SIZE;
		    break;
	    case DB_BTRE_STORE :
	    case DB_ISAM_STORE :
		    break;
	}

	if (temporary)
	{
	    char_entry[++ind].char_id = DMU_TEMP_TABLE;
	    char_entry[ind].char_value = DMU_C_ON;
	}

	if (base_page_size)
	{
	    char_entry[++ind].char_id = DMU_PAGE_SIZE;
	    char_entry[ind].char_value = base_page_size;
	}

	dmu_cb.dmu_char_array.data_address = (char *) char_entry;
	dmu_cb.dmu_char_array.data_in_size = sizeof(DMU_CHAR_ENTRY)*(ind+1);

	dmu_cb.dmu_key_array.ptr_address = (PTR) keys;
	dmu_cb.dmu_key_array.ptr_size = sizeof(DMU_KEY_ENTRY);
	dmu_cb.dmu_key_array.ptr_in_count = DMPE_KEY_COUNT;
    }

    status = dmpe_allocate(&pop_cb, &pcb);
    if (status)
    {
	*dberr = pop_cb.pop_error;
	return(status);
    }

    while (status == E_DB_OK)
    {
	if (!temporary)
	{
	    status = dmpe_cat_scan(&pop_cb, &base_dmu->dmu_tbl_id,
		&etab_record);
	}
	else
	{
	    /*
	    ** In the case of temporary base tables, we must open the
	    ** table, examine its tcb_et_list, then close it.
	    */
	    if (!base_open)
	    {
		i4 timeout = dm2t_get_timeout(xcb->xcb_scb_ptr, tbl_id);
		DB_TAB_TIMESTAMP timestamp;

		status = dm2t_open( dcb, tbl_id, DM2T_S, DM2T_UDIRECT,
				    DM2T_A_READ_NOCPN, timeout, (i4)20,
				    (i4)0, xcb->xcb_log_id,
				    xcb->xcb_scb_ptr->scb_lock_list,
				    (i4)0, (i4)0, db_lockmode,
				    &xcb->xcb_tran_id, &timestamp, &rcb,
				    (DML_SCB *)0, &local_dberr);
		if (!status)
		{
		    t = rcb->rcb_tcb_ptr;
		    rcb->rcb_xcb_ptr = xcb;
		    base_open = 1;
		}
	    }

	    if (base_open)
	    {
		if (!etlist_ptr)
		    etlist_ptr = t->tcb_et_list->etl_next;
		else
		    etlist_ptr = etlist_ptr->etl_next;
		if (etlist_ptr != t->tcb_et_list)
		{
		    status = E_DB_OK;
		    etab_record.etab_extension =
			etlist_ptr->etl_etab.etab_extension;
		}
		else
		{
		    status = dm2t_close(rcb, DM2T_NOPURGE, &local_dberr);
		    rcb = 0;
		    status = E_DB_WARN;
		    SETDBERR(&pop_cb.pop_error, 0, E_DM0055_NONEXT);
		}
	    }
	}
	if (status == E_DB_OK)
	{
	    dmu_cb.dmu_tbl_id.db_tab_base = etab_record.etab_extension;
	    dmu_cb.dmu_tbl_id.db_tab_index = 0;

	    if (truncated)
	    {
		char_entry[0].char_id = DMU_TRUNCATE;
		status = dmu_modify(&dmu_cb);
		char_entry[0].char_id = DMU_STRUCTURE;
	    }

	    if (status == E_DB_OK)
	    {
		status = dmu_modify(&dmu_cb);
	    }
	    pop_cb.pop_error = dmu_cb.error;
	    *dberr = dmu_cb.error;
	}
    }
    if (status == E_DB_WARN && pop_cb.pop_error.err_code == E_DM0055_NONEXT)
    {
	status = E_DB_OK;
	CLRDBERR(&pop_cb.pop_error);
    }

    dmpe_deallocate(pcb);
    *dberr = pop_cb.pop_error;

    return(status);
}
/*{
** Name: dmpe_relocate	- This routine relocates all extension table to their
**			    appropriate locations.
**
** Description:
**      This routine relocates the extension tables of a given table.
**	It is based on the table id of the parent table.
**
**	Based upon this id, the routine scans the iiextended_relation catalog.
**	Any table whose parent matches the table id passed in is moved.
**
** Inputs:
**	base_dmu			Ptr to dmu_cb for which this is being
**					performed.
**      error                           Ptr to i4 to contain any error
**					encountered.
**
** Outputs:
**      *error                          Filled if appropriate.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-Nov-1992 (fred)
**          Created.
**      17-Sep-1993 (fred)
**          Fixup support for relocation.  Make sure to reset the
**          dmu_flags_mask to not include any extension related masks
**          for this modify.  If this is not done, then we sometimes
**          skip the movement of this extension.
**	31-jan-1994 (bryanp) B58477
**	    Check return code from dmpe_allocate.
**	02-jul-1998 (shust01)
**	    Initialized cpn_rcb for fake peripheral, since it
**	    it is used further down and has the potential to SEGV.
**	15-Apr-2010 (kschendel) SIR 123485
**	    Don't need a fake coupon any more.
*/
DB_STATUS
dmpe_relocate(DMU_CB	  *base_dmu ,
               DB_ERROR	  *dberr )

{
    DB_STATUS           status;
    ADP_POP_CB		pop_cb;
    DMPE_PCB		*pcb;
    DMP_ETAB_CATALOG	etab_record;
    DMU_CB		dmu_cb;

    CLRDBERR(&pop_cb.pop_error);

    if (DMZ_SES_MACRO(11))
	dmd_petrace("DMPE_RELOCATE requested", 0, 0 , 0);

    pop_cb.pop_temporary = ADP_POP_TEMPORARY;
    pop_cb.pop_coupon = NULL;
    pop_cb.pop_info = NULL;

    STRUCT_ASSIGN_MACRO(*base_dmu, dmu_cb);
    dmu_cb.dmu_flags_mask &= ~(DMU_EXTTOO_MASK | DMU_EXTONLY_MASK);

    status = dmpe_allocate(&pop_cb, &pcb);
    if (status)
    {
	*dberr = pop_cb.pop_error;
	return (status);
    }

    while (status == E_DB_OK)
    {
	status = dmpe_cat_scan(&pop_cb, &base_dmu->dmu_tbl_id, &etab_record);
	if (status == E_DB_OK)
	{
	    dmu_cb.dmu_tbl_id.db_tab_base = etab_record.etab_extension;
	    dmu_cb.dmu_tbl_id.db_tab_index = 0;

	    status = dmu_relocate(&dmu_cb);

	    pop_cb.pop_error = dmu_cb.error;
	    *dberr = dmu_cb.error;
	}
    }
    if (status == E_DB_WARN && pop_cb.pop_error.err_code == E_DM0055_NONEXT)
    {
	status = E_DB_OK;
	CLRDBERR(&pop_cb.pop_error);
    }

    dmpe_deallocate(pcb);
    *dberr = pop_cb.pop_error;

    return(status);
}

/*
** Name: dmpe_find_or_create_bqcb - Find or Create Blob Context Block
**
** Description:
**
**	The BQCB (blob query context block) is created when a table
**	containing blobs is first opened during a query.  This is
**	the routine, called by table open, which locates the existing
**	BQCB, or creates a new one.
**
**	BQCB's are linked in a simple singly linked list off of the
**	session DML_SCB.  It ought to be quite unusual to have more than
**	a handful of tables open per query that have LOB's in them,
**	so an ordinary list search should suffice.
**
**	The caller should ensure that the table has LOB's in it before
**	calling, and the passed-in RCB should have all of the lockmode
**	information set up (since we'll copy table-open hints to the
**	BQCB).
**
** Inputs:
**	r		RCB that has just been opened for read or write
**	dberr		DB_ERROR that is filled in if error
**
** Outputs:
**	r->rcb_bqcb_ptr gets filled in;
**	BQCB is created or updated.
**	Returns E_DB_OK or error status.
**
** History:
**	15-Apr-2010 (kschendel) SIR 123485
**	    Created.
*/

/* First, a helper routine to extract common code.  This routine
** fills in interesting info from the RCB into the BQCB.
** Should be called with the BQCB mutex held, so that multiple
** threads opening tables don't crash into one another.
*/
static void
dmpe_update_bqcb(DMPE_BQCB *bqcb, DMP_RCB *r)
{
    r->rcb_bqcb_ptr = bqcb;
    if (r->rcb_lk_type == RCB_K_CROW)
    {
	bqcb->bqcb_table_lock = FALSE;
	bqcb->bqcb_x_lock = FALSE;
	bqcb->bqcb_crib = r->rcb_crib_ptr;
    }
    else if (r->rcb_lk_type == RCB_K_TABLE)
    {
	bqcb->bqcb_crib = NULL;
	bqcb->bqcb_table_lock = TRUE;
	/* Set X lock if RCB is LK_X, but don't clear if it's not.
	** A statement such as insert/select (if using LOAD) might open
	** the same table X and S.
	*/
	if (r->rcb_lk_mode == LK_X)
	    bqcb->bqcb_x_lock = TRUE;
    }
    else
    {
	/* Looks like ordinary row or page locking */
	bqcb->bqcb_table_lock = FALSE;
	bqcb->bqcb_x_lock = FALSE;
	bqcb->bqcb_crib = NULL;
    }
} /* dmpe_update_bqcb */


DB_STATUS
dmpe_find_or_create_bqcb(DMP_RCB *r, DB_ERROR *dberr)
{

    DB_ATTS *att;		/* Pointer to TCB attribute array */
    DB_STATUS status;
    DML_SCB *scb;		/* Parent (session) thread's SCB */
    DMPE_BQCB *bqcb;		/* Found or created BQCB */
    DMPE_BQCB *new_bqcb;	/* Created BQCB */
    DMPE_BQCB_ATT *bqcb_att;
    DMP_TCB *t;			/* Base table's TCB */
    i4 nlobs;			/* Number of blob columns */

    t = r->rcb_tcb_ptr;
    scb = r->rcb_xcb_ptr->xcb_odcb_ptr->odcb_scb_ptr;
    dm0s_mlock(&scb->scb_bqcb_mutex);
    bqcb = scb->scb_bqcb_next;
    while (bqcb != NULL && bqcb->bqcb_base_id != t->tcb_rel.reltid.db_tab_base)
	bqcb = (DMPE_BQCB *) bqcb->hdr.obj_next;

    if (bqcb != NULL)
    {
	dmpe_update_bqcb(bqcb, r);
	dm0s_munlock(&scb->scb_bqcb_mutex);
	return (E_DB_OK);
    }
    dm0s_munlock(&scb->scb_bqcb_mutex);

    /* Looks like we need to create one, count LOB attributes */
    nlobs = 0;
    for (att = &t->tcb_atts_ptr[t->tcb_rel.relatts];
	 att >= &t->tcb_atts_ptr[0];
	 --att)
    {
	if (att->flag & ATT_PERIPHERAL)
	    ++nlobs;
    }

    /* Note that the BQCB is (by default) a LONGTERM allocation, because
    ** it might be created in a factotum thread that the BQCB might
    ** outlast.  ("session term" memory might be nice!)
    */
    status = dm0m_allocate( sizeof(DMPE_BQCB) + nlobs * sizeof(DMPE_BQCB_ATT),
		DM0M_ZERO,
		DM_BQCB_CB, BQCB_ASCII_ID,
		(char *) scb, (DM_OBJECT **) &bqcb, dberr);
    if (status != E_DB_OK)
	return (status);

    /* Initialize anything nonzero */
    bqcb->bqcb_base_id = t->tcb_rel.reltid.db_tab_base;
    bqcb->bqcb_natts = nlobs;
    bqcb_att = &bqcb->bqcb_atts[0];
    for (att = &t->tcb_atts_ptr[1];
	 att <= &t->tcb_atts_ptr[t->tcb_rel.relatts];
	 ++att)
    {
	if (att->flag & ATT_PERIPHERAL)
	{
	    bqcb_att->bqcb_att_id = att - t->tcb_atts_ptr;
	    if (att->geomtype != -1)
		bqcb_att->bqcb_srid = att->srid;
	    ++bqcb_att;
	}
    }

    /* Before hooking into the BQCB list, make sure that some other
    ** session thread hasn't snuck in and created the darn thing first.
    ** Do the search thing all over again.
    */
    new_bqcb = bqcb;
    scb = r->rcb_xcb_ptr->xcb_odcb_ptr->odcb_scb_ptr;
    dm0s_mlock(&scb->scb_bqcb_mutex);
    bqcb = scb->scb_bqcb_next;
    while (bqcb != NULL && bqcb->bqcb_base_id != t->tcb_rel.reltid.db_tab_base)
	bqcb = (DMPE_BQCB *) bqcb->hdr.obj_next;

    if (bqcb != NULL)
    {
	dmpe_update_bqcb(bqcb, r);
	dm0s_munlock(&scb->scb_bqcb_mutex);
	dm0m_deallocate((DM_OBJECT **) &new_bqcb);  /* All for naught */
    }
    else
    {
	new_bqcb->hdr.obj_next = (DM_OBJECT *) scb->scb_bqcb_next;
	scb->scb_bqcb_next = new_bqcb;	/* Link in front */
	dmpe_update_bqcb(new_bqcb, r);
	dm0s_munlock(&scb->scb_bqcb_mutex);
    }
    return (E_DB_OK);

} /* dmpe_find_or_create_bqcb */

/*
** Name: dmpe_purge_bqcb - Purge Blob Query Context (BQCB) for a table
**
** Description:
**	If a DDL operation causes a base table's TCB to be forcibly
**	purged, it's necessary to make sure that no BQCB for that table
**	exists either.  A BQCB has a list of blob attributes in it,
**	and if the table now has a different definition, that list may
**	be obsolete.
**
** Inputs:
**	base_id			The base table base ID.
**
** Outputs:
**	None.
**
** History:
**	22-Apr-2010 (kschendel) SIR 123485
**	    Write, for alter table add/drop/alter long varchar column.
*/

void
dmpe_purge_bqcb(i4 base_id)
{
    CS_SID sid;
    DML_SCB *thread_scb, *scb;		/* Thread and session-parent SCB's */
    DMPE_BQCB *bqcb, *prev_bqcb;

    CSget_sid(&sid);
    thread_scb = GET_DML_SCB(sid);
    if (thread_scb == NULL)
	return;				/* Probably rollforward */
    scb = thread_scb->scb_oq_next->odcb_scb_ptr;
    dm0s_mlock(&scb->scb_bqcb_mutex);
    prev_bqcb = NULL;
    bqcb = scb->scb_bqcb_next;		/* Look for matching BQCB */
    while (bqcb != NULL && bqcb->bqcb_base_id != base_id)
    {
	prev_bqcb = bqcb;
	bqcb = (DMPE_BQCB *) bqcb->hdr.obj_next;
    }
    if (bqcb != NULL)
    {
	/* Found it, take it off the list */
	if (prev_bqcb == NULL)
	    scb->scb_bqcb_next = (DMPE_BQCB *) bqcb->hdr.obj_next;
	else
	    prev_bqcb->hdr.obj_next = bqcb->hdr.obj_next;
    }
    dm0s_munlock(&scb->scb_bqcb_mutex);
    if (bqcb != NULL)
	dm0m_deallocate((DM_OBJECT **) &bqcb);

} /* dmpe_purge_bqcb */

/*
** {
** Name: dmpe_allocate	- Allocate a peripheral control block.
**
** Description:
**      This routine allocates a peripheral control block, DMPE_PCB.
**	As an adjunct to the PCB, we also allocate space for a DMT_CB,
**	a DMR_CB, one DMPE_RECORD (aka etab row), and enough PTR's
**	and DMR_ATTR_ENTRY's to make up an etab key array for DMR_POSITION
**	type calls.  Thus, the PCB is useful not only for multi-call
**	operations like put/get, but also for DDL/DMU type operations
**	that require dmr / dmt calls.
**
**	Upon return, the non-LOB-specific parts of the PCB are initialized,
**	and the DMT/DMR cb's partially set up.  Any parts of the PCB that
**	are dependent on the base table or specific LOB column are
**	zeroed;  to set those up (as for get or put), use dmpe_begin_dml
**	instead of dmpe_allocate.
**
** Inputs:
**      pop_cb			ADP_POP_CB for which to allocate...
**      pcb_ptr                 Ptr to ptr to pcb to allocate
**
** Outputs:
**      *pcb_ptr                Filled with the result
**      pop_cb...
**	    pop_user_arg        Filled with result
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      03-Jan-1990 (fred)
**          Created.
**      27-Oct-1993 (fred)
**          Zero'd pcb_q_next field to allow for queuing of these
**          objects in cases where multiple streams are necessary.
**	24-Mar-1998 (thaju02)
**          Bug #87880 - inserting or copying blobs into a temp table chews
**          up cpu, locks and file descriptors.
**      29-Jun-98 (thaju02)
**          Regression bug: with autocommit on, temporary tables created
**          during blob insertion, are not being destroyed at statement
**          commital, but are being held until session termination.
**          Regression due to fix for bug 87880. (B91469)
**	feb-mar-99 (stephenb)
**	    Add code to allow peripheraal inserts to load directly into
**	    the target table.
**	1-apr-99 (stephenb)
**	    Inint pcb_base_dmtcb field in pcb.
**	5-apr-99 (stephenb)
**	    Set coupon key value from table key.
**	12-apr-1999 (somsa01)
**	    We need to initialize pcb->pcb_fblk.adf_fi_id.
**      30-jul-1999 (stial01)
**          Fix up error handling, initialization of pop_cb->pop_temporary
**	14-oct-99 (stephenb)
**	    Set SCB_BLOB_OPTIM in the DML_SCB session mask if we used the
**	    base etab to store the segments.
**      07-mar-2000 (stial01)
**          Disable blob optimization for spatial  (B100776)
**	7-May-2004 (schka24)
**	    Use current thread transaction and DB context.
**      29-nov-2005 (horda03) Bug 48659/INGSRV 2716
**          A TX can now be started with a non-journaled BT record. This
**          leaves the XCB_DELAYBT flag set but sets the XCB_NONJNL_BT
**          flag. Need to write a journaled BT record if the update
**          will be journaled.
**	17-Apr-2008 (kibro01) b120276
**	    Initialise ADF_CB structure
**	12-Apr-2010 (kschendel) SIR 123485
**	    Split out blob optim stuff to dmpe_begin_dml.
*/
static DB_STATUS
dmpe_allocate(ADP_POP_CB     *pop_cb ,
               DMPE_PCB  **pcb_ptr)
{
    DB_STATUS           status;
    DMPE_PCB		*pcb = (DMPE_PCB *)0;
    CS_SID		sid;
    DML_SCB		*scb;
    DML_XCB		*xcb;
    DMR_ATTR_ENTRY	**ptr_place;
    DMR_ATTR_ENTRY	*attr_entry;

    do
    {
	if (sizeof(DMPE_COUPON) != sizeof(ADP_COUPON))
	{
	    TRdisplay("ERROR: dmpe_allocate DMPE_COUPON %d ADP_COUPON %d\n",
		sizeof(DMPE_COUPON),sizeof(ADP_COUPON));
	}

	/*
	**  To operate, we need the pcb, plus the record to read in & out of,
	**  plus a DMR_CB and a DMT_CB to perform table manipulations with,
	*/

	status = dm0m_allocate(	(sizeof(DMPE_PCB)
				    + sizeof(DMPE_RECORD)
				    + sizeof(DMR_CB)
				    + sizeof(DMT_CB)
				    + ((sizeof(DMR_ATTR_ENTRY) + sizeof(PTR))
						 * DMPE_KEY_COUNT)
				),
			(i4) 0,
			(i4) PCB_CB,
			(i4) PCB_ASCII_ID,
			(char *) pop_cb,
			(DM_OBJECT **) pcb_ptr,
			&pop_cb->pop_error);
	if (status)
	    break;

	pop_cb->pop_user_arg = (PTR) (*pcb_ptr);
	pcb = *pcb_ptr;

	pcb->pcb_tcb = NULL;
	pcb->pcb_fixed_tcb = FALSE;
	pcb->pcb_self = pcb;
	pcb->pcb_fblk.adf_r_dv.db_data = NULL;
	pcb->pcb_fblk.adf_fi_id = ADI_NOFI;
	pcb->pcb_xcb = NULL;
	pcb->pcb_db_id = 0;
	pcb->pcb_xq_next = NULL;
	pcb->pcb_bqcb = NULL;
	pcb->pcb_att_id = 0;
	pcb->pcb_et = NULL;
	pcb->pcb_held_record = NULL;
	pcb->pcb_loaded = 0;
	pcb->pcb_per_etab = 0;

	/* The transaction ID for any operations from this PCB is the
	** current thread's tranid.  In a parallel query situation we
	** expect that the current thread either owns the transaction or
	** has attached to it via shared transaction connect.
	*/
	CSget_sid(&sid);
	pcb->pcb_sid = sid;
	scb = GET_DML_SCB(sid);

	if (scb->scb_x_ref_count > 0)
	{
	    xcb = scb->scb_x_next;
	    pcb->pcb_xcb = xcb;
	    pcb->pcb_db_id = (PTR) xcb->xcb_odcb_ptr;
	}
	else
	{
	    SETDBERR(&pop_cb->pop_error, 0, E_DM9B08_DMPE_NEED_TRAN);
	    status = E_DB_ERROR;
	    break;
	}

	/* Queue PCB to thread's XCB so that PCB stuff can be cleaned up
	** upon thread interrupt/abort.
	*/
	pcb->pcb_xq_next = xcb->xcb_pcb_list;
	xcb->xcb_pcb_list = pcb;
	pcb->pcb_record = (DMPE_RECORD *) ((char *) pcb + sizeof(DMPE_PCB));
	pcb->pcb_record->prd_segment0 = 0;
	pcb->pcb_record->prd_segment1 = 0;

	pcb->pcb_cat_scan = 0;
	pcb->pcb_seg0_next = 0;
	pcb->pcb_seg1_next = 0;
	pcb->pcb_table_previous.db_tab_base = 0;
	pcb->pcb_table_previous.db_tab_index = 0;

	pcb->pcb_dmrcb = (DMR_CB *) ((char *) pcb->pcb_record +
						sizeof(DMPE_RECORD));
	MEfill(sizeof(DMR_CB), 0, (PTR) pcb->pcb_dmrcb);
	pcb->pcb_dmrcb->length = sizeof(DMR_CB);
	pcb->pcb_dmrcb->type = DMR_RECORD_CB;
	pcb->pcb_dmrcb->ascii_id = DMR_ASCII_ID;
	pcb->pcb_dmrcb->dmr_access_id = NULL;
	pcb->pcb_dmrcb->dmr_data.data_address = (char *) pcb->pcb_record;
	pcb->pcb_dmrcb->dmr_data.data_in_size = sizeof(DMPE_RECORD);
	pcb->pcb_dmrcb->dmr_attr_desc.ptr_in_count = DMPE_KEY_COUNT;
	pcb->pcb_dmrcb->dmr_attr_desc.ptr_size = sizeof(DMR_ATTR_ENTRY);
	pcb->pcb_dmrcb->dmr_attr_desc.ptr_address = (PTR)
			((char *) pcb->pcb_dmrcb + sizeof(DMR_CB));
	ptr_place =
	    (DMR_ATTR_ENTRY **) pcb->pcb_dmrcb->dmr_attr_desc.ptr_address;
	*ptr_place = (DMR_ATTR_ENTRY *)
	    ((char *) pcb->pcb_dmrcb->dmr_attr_desc.ptr_address
	                      + (DMPE_KEY_COUNT * sizeof(PTR)));

	attr_entry = (DMR_ATTR_ENTRY *) *ptr_place;
	attr_entry->attr_operator = DMR_OP_EQ;
	attr_entry->attr_number = 1;
	attr_entry->attr_value = (char *) &pcb->pcb_record->prd_log_key;

	ptr_place += 1;
	attr_entry += 1;
	*ptr_place = attr_entry;
	attr_entry->attr_operator = DMR_OP_EQ;
	attr_entry->attr_number = 2;
	attr_entry->attr_value = (char *) &pcb->pcb_record->prd_segment0;

	ptr_place += 1;
	attr_entry += 1;
	*ptr_place = attr_entry;
	attr_entry->attr_operator = DMR_OP_EQ;
	attr_entry->attr_number = 3;
	attr_entry->attr_value = (char *) &pcb->pcb_record->prd_segment1;

	pcb->pcb_dmtcb = (DMT_CB *) (attr_entry + 1);

	MEfill(sizeof(DMT_CB), 0, (PTR) pcb->pcb_dmtcb);
	pcb->pcb_dmtcb->length = sizeof(DMT_CB);
	pcb->pcb_dmtcb->type = DMT_TABLE_CB;
	pcb->pcb_dmtcb->ascii_id = DMT_ASCII_ID;
	pcb->pcb_dmtcb->dmt_record_access_id = NULL;
	pcb->pcb_dmtcb->dmt_tran_id = (PTR) pcb->pcb_xcb;
	pcb->pcb_dmtcb->dmt_db_id = pcb->pcb_db_id;

    } while (0);

    if (status && pcb)
    {
	dmpe_deallocate(pcb);
	pop_cb->pop_user_arg = NULL;
    }

    return(status);
}

/*{
** Name: dmpe_deallocate	- Deallocate a dmpe_pcb
**
** Description:
**      This routine simply deallocates the pcb.  Nothing complicated at this
**	point.
**
** Inputs:
**      pcb                             Pcb to deallocate
**
** Outputs:
**      (none)
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      03-Jan-1990 (fred)
**          Created.
**	10-May-2004 (schka24)
**	    Take PCB off of cleanup list when it's deallocated..
**	6-Apr-2010 (kschendel) SIR 123485
**	    Deallocate coercion data area here, so we don't leak memory if
**	    the dmpe operation is interrupted.
**	    Clean up ET list entries if any point to this PCB.
**	    Unfix tcb if we fixed it in dmpe-begin-dml.
*/
void
dmpe_deallocate(DMPE_PCB      *pcb )

{
    DMPE_PCB *pcb1, *pcb2;		/* Singly-linked list chasers */

    if (pcb)
    {
	if (pcb->pcb_xcb != NULL)
	{
	    /* Unlink from XCB list of PCB's */
	    pcb1 = pcb->pcb_xcb->xcb_pcb_list;
	    pcb2 = NULL;
	    while (pcb1 != pcb && pcb1 != NULL)
	    {
		pcb2 = pcb1;
		pcb1 = pcb1->pcb_xq_next;
	    }
	    if (pcb1 == pcb)
		if (pcb2 != NULL)
		    pcb2->pcb_xq_next = pcb->pcb_xq_next;
		else
		    pcb->pcb_xcb->xcb_pcb_list = pcb->pcb_xq_next;
	}
	if (pcb->pcb_tcb != NULL)
	{
	    DMP_ET_LIST *etl;

	    /* Make sure that no ET list entries were pointing at this PCB.
	    ** Shouldn't happen, but make sure!
	    ** Don't close any etabs, let session or query or abort cleanup
	    ** that closes all open tables do that part.
	    */
	    dm0s_mlock(&pcb->pcb_tcb->tcb_et_mutex);
	    etl = pcb->pcb_tcb->tcb_et_list;
	    if (etl != NULL)
	    {
		do
		{
		    if (etl->etl_etab.etab_type == DMP_LO_ETAB
		      && etl->etl_pcb == pcb)
		    {
			etl->etl_pcb = NULL;
			etl->etl_access_id = NULL;
			etl->etl_sid = 0;
			etl->etl_status &= ~(ETL_LOAD | ETL_FULL_MASK);
		    }
		    etl = etl->etl_next;
		} while (etl != pcb->pcb_tcb->tcb_et_list);
	    }
	    dm0s_munlock(&pcb->pcb_tcb->tcb_et_mutex);
	}
	if (pcb->pcb_fblk.adf_r_dv.db_data)
	    (VOID)MEfree(pcb->pcb_fblk.adf_r_dv.db_data);
	if (pcb->pcb_fixed_tcb)
	{
	    DB_ERROR toss_error;
	    i4 lk_list_id = pcb->pcb_xcb->xcb_lk_id;
	    if (pcb->pcb_tcb->tcb_rel.reltid.db_tab_base < 0)
		lk_list_id = pcb->pcb_xcb->xcb_scb_ptr->scb_lock_list;
	    (void) dm2t_unfix_tcb(&pcb->pcb_tcb, DM2T_VERIFY,
			lk_list_id, pcb->pcb_xcb->xcb_log_id,
			&toss_error);
	}
	if (pcb->pcb_held_record != NULL)
	    dm0m_tbdealloc((char **) &pcb->pcb_held_record);

	dm0m_deallocate((DM_OBJECT **) &pcb);
    }
    return;
}

/*
** Name: dmpe_begin_dml - Setup for GET or PUT operations
**
** Description:
**
**	This routine sets up context data needed for dmpe
**	operations that actually read or write LOB data, i.e.
**	the dmpe_get and dmpe_put operations.  It allocates a PCB
**	(unless we can find one from a continued bulk-load), and
**	determines the query context if known.
**
**	For GET, we need to determine the query context (so that
**	etabs can be opened properly).  This might come from a
**	DB_BLOB_WKSP hooked to the pop_cb, or it might arrive
**	as data in the short-term part of the DMF coupon.
**
**	For PUT, the query context is also needed, but in the put
**	case the only available source is a DB_BLOB_WKSP.  If no
**	base table info is suppled, the put will have to go to a
**	holding temp.  Additionally, for put (if not to a temp),
**	the coupon might need a logical key assigned to it.  And
**	finally, if no bulk-load decision was made for this query
**	yet, see if it would be possible to try bulk-loading into etabs.
**
**	For either operation, again if not to a holding temp, the
**	get/put code will need safe access to the etab list in the
**	base TCB.  So, if there is no assurance that the base TCB is
**	fixed / opened already, fix it so that dmpe get/put can be
**	assured that the base TCB won't be reclaimed out from under (!).
**	The corresponding unfix will happen in dmpe_deallocate.
**	"Assurance" here means either a) the caller passed a wksp
**	with access-id (ie RCB) filled in, or b) the BQCB we end up
**	with has an RCB open against the base table for logical-key
**	generation.
**
**	Note:  while there's a lot of code here, most of it is for
**	first-time-thru-the-query setup.  Once the query is off and
**	running, the normal path should be fairly quick.
**
** Inputs:
**	pop_cb			Pointer to ADP_POP_CB
**	    pop_coupon		DB_DATA_VALUE pointing to coupon
**	    pop_info		Optional pointer to DB_BLOB_WKSP
**	pcbp			Pointer to pointer to DMPE_PCB
**	is_put			TRUE if put direction, else get
**
** Outputs:
**	(if put) coupon filled in with flags, logical key.
**	*pcbp points to allocated or retrieved PCB.
**	pcb_bqcb, pcb_tcb filled in unless coupon is for holding temp.
**	Returns E_DB_OK or error
**
** History:
**	15-Apr-2010 (kschendel) SIR 123485
**	    Written.
**	24-May-2010 (kschendel) b123798
**	    Clear base-id when an attempt at optimizing a put coupon fails
**	    for some valid reason (e.g. multiple blob atts).  Otherwise,
**	    the later cross-check gets confused.
*/

static DB_STATUS
dmpe_begin_dml(ADP_POP_CB *pop_cb, DMPE_PCB **pcbp, bool is_put)
{

    DB_BLOB_WKSP        *wksp;
    DB_STATUS		status = E_DB_OK;
    DML_SCB		*ses_scb;	/* Session thread SCB */
    DML_XCB		*xcb;
    DMPE_BQCB		*bqcb;
    DMPE_COUPON		*cpn;
    DMPE_PCB		*pcb;
    DMP_RCB		*rcb;		/* Logical-key generator RCB */
    i4			att_num;
    i4			base_id;
    i4			natts;		/* Attribute count */

    cpn = DMPE_CPN_FROM_DBV_MACRO(pop_cb->pop_coupon);
    wksp = (DB_BLOB_WKSP *) pop_cb->pop_info;
    bqcb = NULL;
    att_num = 0;
    base_id = 0;

    /* First, try to figure out what the base table is, directly or
    ** indirectly.
    */
    if (!is_put)
    {
	/* Get: if we have a workspace that passed access-id (and att-id),
	** use that.  Otherwise, use the coupon.
	** Get doesn't currently have any long-term operations that re-use
	** a PCB, so start by creating one.
	*/
	status = dmpe_allocate(pop_cb, pcbp);
	if (status != E_DB_OK)
	    return (status);
	pcb = *pcbp;
	xcb = pcb->pcb_xcb;
	ses_scb = xcb->xcb_odcb_ptr->odcb_scb_ptr;
	if (wksp != NULL
	  && (wksp->flags & (BLOBWKSP_ACCESSID | BLOBWKSP_ATTID)) == (BLOBWKSP_ACCESSID | BLOBWKSP_ATTID))
	{
	    bqcb = ((DMP_RCB *)(wksp->access_id))->rcb_bqcb_ptr;
	    att_num = wksp->base_attid;
	}
	if ((bqcb == NULL || att_num == 0)
	  && (cpn->cpn_flags & DMPE_CPN_TEMP) == 0)
	{
	    base_id = cpn->cpn_base_id;
	    att_num = cpn->cpn_att_id;
	}
	/* Gotta have something by now, or it's an error */
	if ( ((bqcb == NULL && base_id == 0) || att_num == 0)
	  && (cpn->cpn_flags & DMPE_CPN_TEMP) == 0)
	{
	    /* NOTE: this is now a lot stricter than it used to be.
	    ** A non-temporary coupon *must* have short term context.
	    ** I've managed to fix all the known places that was not
	    ** setting up coupon context, but at the cost of much
	    ** weeping, gnashing of teeth, and hair-pulling.  And I
	    ** don't have that much hair left...
	    ** So, if this error turns up in some situation where a fix
	    ** seems intractable, consider relaxing this to not be an error,
	    ** and either a) teach dmpe-get/put to operate without a tcb,
	    ** bqcb, etc etc as a fallback, OR b) search the world using
	    ** the etab table ID and find the base ID/attnum that way.
	    ** (b) would be better, but (much) more expensive since it might
	    ** involve an iiextended_relation catalog search.
	    ** (kschendel Apr 2010)
	    */
	    TRdisplay("%@ dmpe_begin_dml: Can't resolve GET\nbqcb: %p, base_id %d, att %d, wksp-flags %x\n",
			bqcb, base_id, att_num, wksp ? wksp->flags : -1);
	    SETDBERR(&pop_cb->pop_error, 0, E_AD700B_ADP_BAD_POP_CB);
	    dmpe_deallocate(pcb);
	    pop_cb->pop_user_arg = NULL;
	    return (E_DB_ERROR);
	}
    }
    else
    {
	/* Put: it's the workspace or nothing. */
	/* If there's a passed access-id in the workspace, it might be a
	** load, and we might be re-using a PCB.  Otherwise, allocate one.
	*/
	cpn->cpn_flags = 0;		/* We'll compute this */
	pcb = NULL;
	if (wksp != NULL
	  && (wksp->flags & (BLOBWKSP_ACCESSID | BLOBWKSP_ATTID)) == (BLOBWKSP_ACCESSID | BLOBWKSP_ATTID))
	{
	    DMP_RCB *r = (DMP_RCB *) (wksp->access_id);

	    /* Most common case, base table RCB passed in. */
	    bqcb = r->rcb_bqcb_ptr;
	    att_num = wksp->base_attid;
	    /* If we know we might be bulk-loading an etab, look for an
	    ** etab marked LOAD (and not full).  Since an X lock on the
	    ** base table is a prerequisite, nobody else can be loading.
	    ** If we find a LOAD etab it must be us.
	    */
	    if (r->rcb_lk_mode == LK_X
	      && bqcb->bqcb_load_etabs == BQCB_LOAD_YES)
	    {
		DMP_TCB *t = r->rcb_tcb_ptr;
		DMP_ET_LIST *etl;
		dm0s_mlock(&t->tcb_et_mutex);
		etl = t->tcb_et_list->etl_next;
		while (etl != t->tcb_et_list
		  && (etl->etl_etab.etab_type != DMP_LO_ETAB
		      || etl->etl_etab.etab_att_id != att_num
		      || (etl->etl_status & (ETL_FULL_MASK | ETL_INVALID_MASK | ETL_LOAD)) != ETL_LOAD) )
		    etl = etl->etl_next;
		if (etl == t->tcb_et_list)
		{
		    /* Do it again, but this time don't disallow FULL etabs. */
		    etl = etl->etl_next;
		    while (etl != t->tcb_et_list
		      && (etl->etl_etab.etab_type != DMP_LO_ETAB
			  || etl->etl_etab.etab_att_id != att_num
			  || (etl->etl_status & (ETL_INVALID_MASK | ETL_LOAD)) != ETL_LOAD) )
			etl = etl->etl_next;
		}
		dm0s_munlock(&t->tcb_et_mutex);
		if (etl != t->tcb_et_list)
		{
		    /* Found it, continue loading.  Do some start-of-blob
		    ** cleanup on the PCB before re-using it.
		    */
		    pcb = *pcbp = etl->etl_pcb;
		    pop_cb->pop_user_arg = (PTR) pcb;
		    pcb->pcb_record->prd_segment0 = 0;
		    pcb->pcb_record->prd_segment1 = 0;
		    pcb->pcb_seg0_next = 0;
		    pcb->pcb_seg1_next = 0;
		    if ((etl->etl_status & ETL_FULL_MASK) == 0)
		    {
			pcb->pcb_dmtcb->dmt_record_access_id = (PTR) etl->etl_access_id;
			pcb->pcb_dmtcb->dmt_id.db_tab_base = etl->etl_etab.etab_extension;
			pcb->pcb_dmrcb->dmr_access_id = (PTR) etl->etl_access_id;
			pcb->pcb_et = etl;
		    }
		    else
		    {
			/* Tell dmpe-put we'll need to start with a new etab */
			pcb->pcb_dmtcb->dmt_record_access_id = NULL;
			pcb->pcb_dmrcb->dmr_access_id = NULL;
			pcb->pcb_et = NULL;
		    }
		}
	    }
	}
	if (pcb == NULL)
	{
	    /* Non-load case, allocate a PCB */
	    status = dmpe_allocate(pop_cb, pcbp);
	    if (status != E_DB_OK)
		return (status);
	    pcb = *pcbp;
	}
	xcb = pcb->pcb_xcb;
	ses_scb = xcb->xcb_odcb_ptr->odcb_scb_ptr;
	if (bqcb == NULL && wksp != NULL)
	{
	    natts = 0;
	    if (wksp->flags & BLOBWKSP_TABLEID)
	    {
		base_id = wksp->base_id;
	    }
	    else if (wksp->flags & BLOBWKSP_TABLENAME)
	    {
		bool owner_given;
		DMT_SHW_CB dmt_shw;
		DMT_TBL_ENTRY dmt_tbl_entry;

		/* Get the table info from the name */
		dmt_shw.type = DMT_SH_CB;
		dmt_shw.length = sizeof(DMT_SHW_CB);
		dmt_shw.dmt_char_array.data_in_size = 0;
		dmt_shw.dmt_flags_mask = DMT_M_TABLE | DMT_M_ATTR | DMT_M_NAME;
		dmt_shw.dmt_table.data_address = (PTR) &dmt_tbl_entry;
		dmt_shw.dmt_table.data_in_size = sizeof(DMT_TBL_ENTRY);
		dmt_shw.dmt_char_array.data_address = NULL;
		dmt_shw.dmt_char_array.data_in_size = 0;
		dmt_shw.dmt_char_array.data_out_size  = 0;
		dmt_shw.error.err_code = 0;

		dmt_shw.dmt_session_id = pcb->pcb_sid;
		dmt_shw.dmt_db_id = pcb->pcb_db_id;

		MEcopy(wksp->table_name.db_tab_name, sizeof(DB_TAB_NAME),
		    &dmt_shw.dmt_name);
		owner_given = FALSE;
		if (wksp->table_owner.db_own_name[0] &&
		    wksp->table_owner.db_own_name[0] != ' ')
		{
		    /* owner provided, use it */
		    owner_given = TRUE;
		    MEcopy(wksp->table_owner.db_own_name, sizeof(DB_OWN_NAME),
			&dmt_shw.dmt_owner);
		}
		else
		{
		    /* try user first */
		    MEcopy(&xcb->xcb_scb_ptr->scb_user, sizeof(DB_OWN_NAME),
			&dmt_shw.dmt_owner);
		}

		status = dmt_show(&dmt_shw);
		if (status != E_DB_OK && ! owner_given &&
		    dmt_shw.error.err_code == E_DM0054_NONEXISTENT_TABLE)
		{
		    /* try again with DBA */
		    MEcopy(&xcb->xcb_odcb_ptr->odcb_dcb_ptr->dcb_db_owner,
			sizeof(DB_OWN_NAME), &dmt_shw.dmt_owner);
		    dmt_shw.error.err_code = 0;
		    status = dmt_show(&dmt_shw);
		}
		if (status == E_DB_OK && dmt_tbl_entry.tbl_id.db_tab_index == 0)
		{
		    /* Success so far... */
		    base_id = dmt_tbl_entry.tbl_id.db_tab_base;
		    natts = dmt_tbl_entry.tbl_attr_count;
		}
		status = E_DB_OK;
	    }
	    if (att_num == 0 && wksp->flags & BLOBWKSP_ATTID)
		att_num = wksp->base_attid;
	    if (att_num == 0 || wksp->flags & BLOBWKSP_COERCE)
	    {
		DMT_ATT_ENTRY *att, *base_att, **ptrs, **base;
		DMT_SHW_CB dmt_shw;
		DMT_TBL_ENTRY dmt_tbl_entry;
		i4 i, size;
		STATUS clstat;

		/* Either we still don't know where the LOB is going, or
		** the caller isn't sure that the incoming data type matches
		** the LOB itself.  Either way, we need to SHOW the table
		** columns and do some more whirling around.
		*/
		if (natts == 0)
		    natts = DB_MAX_COLS;	/* Not worth an extra show */
		size = natts * (sizeof(DMT_ATT_ENTRY) + sizeof(DMT_ATT_ENTRY *));
		base = NULL;

		do	/* break-out dummy */
		{
		    /* Set up att pointer area for SHOW to fill in.  If
		    ** something goes wrong, don't error out, just say
		    ** that we can't resolve the column.
		    */
		    base = (DMT_ATT_ENTRY **) MEreqmem(0, size, FALSE, &clstat);
		    if (base == NULL)
			break;
		    base_att = (DMT_ATT_ENTRY *) ((char *) base + natts * sizeof(DMT_ATT_ENTRY *));
		    att = base_att;
		    ptrs = base;
		    i = natts;
		    do
		    {
			*ptrs++ = att++;
		    } while (--i > 0);

		    dmt_shw.type = DMT_SH_CB;
		    dmt_shw.length = sizeof(DMT_SHW_CB);
		    dmt_shw.dmt_char_array.data_in_size = 0;
		    dmt_shw.dmt_flags_mask = DMT_M_ATTR;
		    dmt_shw.dmt_table.data_address = NULL;
		    dmt_shw.dmt_table.data_in_size = 0;
		    dmt_shw.dmt_char_array.data_address = NULL;
		    dmt_shw.dmt_char_array.data_in_size = 0;
		    dmt_shw.dmt_char_array.data_out_size  = 0;
		    dmt_shw.dmt_attr_array.ptr_address = (PTR) base;
		    dmt_shw.dmt_attr_array.ptr_in_count = natts;
		    dmt_shw.dmt_attr_array.ptr_out_count = 0;
		    dmt_shw.error.err_code = 0;

		    dmt_shw.dmt_session_id = pcb->pcb_sid;
		    dmt_shw.dmt_db_id = pcb->pcb_db_id;

		    status = dmt_show(&dmt_shw);
		    if (status != E_DB_OK)
			break;

		    /* If we don't know the LOB att yet, there may only be
		    ** one of 'em.
		    */
		    if (att_num == 0)
		    {
			att = base_att;
			i = natts;
			do
			{
			    if (att->att_flags & DMU_F_PERIPHERAL)
			    {
				if (att_num != 0)
				{
				    /* Oops, too many, ambiguous */
				    att_num = 0;
				    break;
				}
				att_num = (att - base_att) + 1;
			    }
			    ++att;
			} while (--i > 0);
			if (att_num == 0)
			    break;	/* Nogo */
		    }
		    if (wksp->flags & BLOBWKSP_COERCE)
		    {
			ADF_CB *adf_scb = xcb->xcb_scb_ptr->scb_adf_cb;
			DB_DT_ID abs_source, abs_att;
			DB_DT_ID source_under, att_under;
			i4 dt_bits;

			att = base_att + att_num - 1;
			abs_source = abs(wksp->source_dt);
			abs_att = abs(att->att_type);
			if (abs_source != abs_att)
			{
			    /* Might need a coercion.  Skip coercion for
			    ** varchar vs varbyte, they are the same as far
			    ** as dmpe is concerned.
			    */
			    source_under = abs_source;
			    status = adi_dtinfo(adf_scb, source_under, &dt_bits);
			    if (status != E_DB_OK)
				break;
			    if (dt_bits & AD_PERIPHERAL)
			    {
				status = adi_per_under(adf_scb, abs_source, &source_under);
				if (status != E_DB_OK)
				    break;
			    }
			    status = adi_per_under(adf_scb, abs_att, &att_under);
			    if (status != E_DB_OK)
				break;
			    /* Note, don't need abs here, non-nullable dt is
			    ** returned by adi-per-under.
			    */
			    if (source_under == att_under
			      || (source_under == DB_VCH_TYPE && att_under == DB_VBYTE_TYPE)
			      || (source_under == DB_VBYTE_TYPE && att_under == DB_VCH_TYPE) )
				break;	/* status OK, no coercion */
			    status = adi_ficoerce(adf_scb,
					source_under, att_under,
					&pcb->pcb_fblk.adf_fi_id);
			    if (status != E_DB_OK)
				break;
			    pcb->pcb_fblk.adf_r_dv.db_datatype = att_under;
			    /* Actual target type returned for sequencer */
			    wksp->source_dt = att->att_type;
			    /* We'll need a coercion temp area */
			    pcb->pcb_fblk.adf_r_dv.db_data =
				MEreqmem(0, DMPE_SEGMENT_LENGTH, FALSE, &clstat);
			    if (clstat != OK)
			    {
				status = E_DB_ERROR;
				break;
			    }
			    pcb->pcb_fblk.adf_r_dv.db_length = DMPE_SEGMENT_LENGTH;
			}
		    } /* if coercion */
		} while (0);
		/* If broke out with error, it means ambiguity in LOB
		** atts, or couldn't set up coercions.
		*/
		if (status != E_DB_OK)
		{
		    att_num = 0;
		    status = E_DB_OK;
		}
		if (base != NULL)
		    MEfree(base);
	    } /* if need to show att stuff */
	} /* if wksp != NULL */
	if ((bqcb == NULL && base_id == 0) || att_num == 0)
	{
	    /* Don't know where it's going, send to temp.  Clean out
	    ** any junk in the coupon too.  Zero the base-id so that
	    ** the cross-check immediately below doesn't puke.
	    */
	    base_id = 0;
	    MEfill(sizeof(DMPE_COUPON), 0, (PTR) cpn);
	    cpn->cpn_flags = DMPE_CPN_TEMP;
	    if (pop_cb->pop_temporary == ADP_POP_PERMANENT)
	    {
		/* This would be a screw-up, if the caller requires a real
		** result but didn't pass enough info to figure it out.
		** For GET, it doesn't matter, but for PUT it certainly does.
		*/
		TRdisplay("%@ dmpe_begin_dml: Holding temp but caller asked for ADP_POP_PERMANENT\n");
		SETDBERR(&pop_cb->pop_error, 0, E_AD700B_ADP_BAD_POP_CB);
		return (E_DB_ERROR);
	    }
	}
    }

    /* At this point we should know: bqcb or base_id, and att_num.
    ** Or, it's a temp.  Or, the caller screwed up.
    */

    if (cpn->cpn_flags & DMPE_CPN_TEMP)
    {
	/* Double check that the caller didn't supply something bogus,
	** then get out now ... nothing else to set up if read or write.
	** Holding temps are heaps and the logical key is unused / junk.
	** Do however make sure that pcb_record has the same zero/junk.
	*/
	if (bqcb != NULL || base_id != 0)
	{
	    TRdisplay("%@ dmpe_begin_dml: Holding temp but BQCB resolved to %p, base_id to %d (put? %d)\n",
			bqcb, base_id, is_put);
	    SETDBERR(&pop_cb->pop_error, 0, E_AD700B_ADP_BAD_POP_CB);
	    status = E_DB_ERROR;
	}
	STRUCT_ASSIGN_MACRO(cpn->cpn_log_key, pcb->pcb_record->prd_log_key);
	return (status);
    }

    /* So, it's not a holding temp.  If we have base table ID but not
    ** a BQCB, find a BQCB for the base table.  Cause one to be invented
    ** if none is found.
    */
    if (bqcb == NULL)
    {
	/* We'll have a base_id then, look for BQCB */
	dm0s_mlock(&ses_scb->scb_bqcb_mutex);
	bqcb = ses_scb->scb_bqcb_next;
	while (bqcb != NULL && bqcb->bqcb_base_id != base_id)
	    bqcb = (DMPE_BQCB *) bqcb->hdr.obj_next;
	dm0s_munlock(&ses_scb->scb_bqcb_mutex);
	if (bqcb == NULL)
	{
	    DMT_CB dmt;

	    /* No BQCB either.  Open/close the base table to create one. */
	    MEfill(sizeof(DMT_CB), 0, (PTR) &dmt);
	    dmt.length = sizeof(DMT_TABLE_CB);
	    dmt.type = DMT_TABLE_CB;
	    dmt.ascii_id = DMT_ASCII_ID;
	    dmt.dmt_db_id = (PTR) pcb->pcb_db_id;
	    dmt.dmt_tran_id = (PTR) xcb;
	    dmt.dmt_lock_mode = DMT_IS;
	    dmt.dmt_update_mode = DMT_U_DIRECT;
	    dmt.dmt_access_mode = DMT_A_READ;
	    /* "internal request" to suppress the check for interrupts. */
	    dmt.dmt_flags_mask = DMT_DBMS_REQUEST;
	    if (base_id < 0)
		dmt.dmt_flags_mask |= DMT_SESSION_TEMP;	/* Base table is GTT */
	    dmt.dmt_id.db_tab_base = base_id;
	    dmt.dmt_id.db_tab_index = 0;
	    dmt.dmt_record_access_id = NULL;
	    status = dmt_open(&dmt);
	    if (status != E_DB_OK)
	    {
		pop_cb->pop_error = dmt.error;
		return (status);
	    }
	    /* Now there will be a BQCB ptr in the returned RCB. */
	    rcb = (DMP_RCB *) dmt.dmt_record_access_id;
	    bqcb = rcb->rcb_bqcb_ptr;
	    dmt.dmt_flags_mask = 0;
	    status = dmt_close(&dmt);
	    if (status != E_DB_OK)
	    {
		pop_cb->pop_error = dmt.error;
		return (status);
	    }
	}
    }

    /* We now have a BQCB pointer, if this is PUT we'll probably need to
    ** generate logical keys (only exception would be some kinds of update
    ** statements).  So, make sure there's a key generator RCB open against
    ** the base table.  Mutex protect this check against multi threads.
    */
    pcb->pcb_bqcb = bqcb;
    dm0s_mlock(&ses_scb->scb_bqcb_mutex);
    rcb = bqcb->bqcb_logkey_rcb;
    dm0s_munlock(&ses_scb->scb_bqcb_mutex);
    if (is_put && rcb == NULL)
    {
	DMT_CB dmt;

	/* Open the base table noaccess/nolock, this will only be
	** used to generate logical keys.
	** * Note * We more or less assume that real puts are only done
	** in the main session thread, not ||-query or copy children.
	** That is true enough, but it's a bit dirty.  If random child
	** threads were to start doing puts, there would be mutexing and
	** query-end issues to deal with.
	*/

	MEfill(sizeof(DMT_CB), 0, (PTR) &dmt);
	dmt.length = sizeof(DMT_TABLE_CB);
	dmt.type = DMT_TABLE_CB;
	dmt.ascii_id = DMT_ASCII_ID;
	dmt.dmt_db_id = (PTR) pcb->pcb_db_id;
	dmt.dmt_tran_id = (PTR) xcb;
	dmt.dmt_lock_mode = DMT_N;
	dmt.dmt_update_mode = DMT_U_DIRECT;
	dmt.dmt_access_mode = DMT_A_OPEN_NOACCESS;
	dmt.dmt_flags_mask = DMT_DBMS_REQUEST;
	if (bqcb->bqcb_base_id < 0)
	    dmt.dmt_flags_mask |= DMT_SESSION_TEMP;	/* Base table is GTT */
	dmt.dmt_id.db_tab_base = bqcb->bqcb_base_id;
	dmt.dmt_id.db_tab_index = 0;
	dmt.dmt_record_access_id = NULL;
	status = dmt_open(&dmt);
	if (status != E_DB_OK)
	{
	    pop_cb->pop_error = dmt.error;
	    return (status);
	}
	dm0s_mlock(&ses_scb->scb_bqcb_mutex);
	if (bqcb->bqcb_logkey_rcb == NULL)
	{
	    rcb = bqcb->bqcb_logkey_rcb = (DMP_RCB *) dmt.dmt_record_access_id;
	    dm0s_munlock(&ses_scb->scb_bqcb_mutex);
	}
	else
	{
	    /* Oops, a parallel thread snuck in */
	    rcb = bqcb->bqcb_logkey_rcb;
	    dm0s_munlock(&ses_scb->scb_bqcb_mutex);
	    dmt.dmt_flags_mask = 0;
	    (void) dmt_close(&dmt);
	}
    }

    /* It's time to get the base table TCB for the PCB, fixing it if
    ** necessary (see the intro).  "rcb" contains the logical key
    ** generator if any.
    */
    if (rcb != NULL)
    {
	pcb->pcb_tcb = rcb->rcb_tcb_ptr;
    }
    else if (wksp && wksp->flags & BLOBWKSP_ACCESSID)
    {
	pcb->pcb_tcb = ((DMP_RCB *)(wksp->access_id))->rcb_tcb_ptr;
    }
    else
    {
	DB_TAB_ID tabid;
	DMP_DCB *dcb = ses_scb->scb_oq_next->odcb_dcb_ptr;
	i4 lk_list_id;

	/* Better fix it ourselves, just in case nothing else has it open.
	** This is a bit unlikely, but it might be an asynchronous redeem
	** done after the base table is closed -- who knows what evil lurks
	** in the sequencer!
	*/
	tabid.db_tab_base = bqcb->bqcb_base_id;
	tabid.db_tab_index = 0;
	lk_list_id = xcb->xcb_lk_id;
	if (tabid.db_tab_base < 0)
	    lk_list_id = ses_scb->scb_lock_list;
	status = dm2t_fix_tcb(dcb->dcb_id, &tabid, &xcb->xcb_tran_id,
			lk_list_id, xcb->xcb_log_id,
			DM2T_VERIFY, dcb,
			&pcb->pcb_tcb, &pop_cb->pop_error);
	if (status != E_DB_OK)
	    return (status);
	pcb->pcb_fixed_tcb = TRUE;
    }

    if (!is_put)
    {
	STRUCT_ASSIGN_MACRO(cpn->cpn_log_key, pcb->pcb_record->prd_log_key);
	return (status);	/* Done if GET */
    }

    /* For PUT, generate a logical key for the coupon (and the row) if
    ** necessary, and look into the bulk-load question.
    */
    pop_cb->pop_temporary = ADP_POP_PERMANENT;
    pcb->pcb_att_id = att_num;
    if (cpn->cpn_log_key.tlk_high_id == 0 && cpn->cpn_log_key.tlk_low_id == 0)
    {
	u_i4 high, low;

	/* rcb was set above */
	if (rcb->rcb_logkey.olk_high_id == 0 && rcb->rcb_logkey.olk_low_id == 0)
	{
	    DMP_TCB *t = rcb->rcb_tcb_ptr;
	    i4 error;

	    /* Haven't generated a key yet, or the last row "ended" and we
	    ** need a new key for this row.
	    */
	    status = dm1c_get_high_low_key(t, rcb,
				&high, &low, &pop_cb->pop_error);
	    if (status != E_DB_OK)
	    {
		if (pop_cb->pop_error.err_code == E_DM9380_TAB_KEY_OVERFLOW)
		{
		    /* error can be returned from dm1c, not
		    ** logged there because it doesn't have the
		    ** necessary context for the error parameters.
		    */
		    uleFormat(&pop_cb->pop_error, 0,
			(CL_ERR_DESC *) NULL, ULE_LOG,
			NULL, (char *) NULL, (i4) 0, (i4 *) NULL,
			&error, 3,
			sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
			sizeof(DB_OWN_NAME), &t->tcb_rel.relowner,
			sizeof(DB_DB_NAME), &t->tcb_dcb_ptr->dcb_name);

		    SETDBERR(&pop_cb->pop_error, 0, E_DM9381_DM2R_PUT_RECORD);
		}
		return (status);
	    }
	}
	else
	{
	    /* Use the same key, remember to do the bswap nonsense. */
	    high = rcb->rcb_logkey.olk_high_id;
	    low = rcb->rcb_logkey.olk_low_id;
	    if (rcb->rcb_tcb_ptr->tcb_rel.relstat2 & TCB2_BSWAP)
	    {
		char c = *((char *)&low);
		*((char *)&low) = *((char *)&low + 3);
		*((char *)&low + 3) = c;
	    }
	}
	cpn->cpn_log_key.tlk_high_id = high;
	cpn->cpn_log_key.tlk_low_id = low;
    }
    STRUCT_ASSIGN_MACRO(cpn->cpn_log_key, pcb->pcb_record->prd_log_key);

    /* Since we're not going into a temp etab, it must be a real one */
    cpn->cpn_flags |= DMPE_CPN_FINAL;
    if (wksp != NULL)
	wksp->flags |= BLOBWKSP_FINAL;

    /* And last, if we haven't decided on bulk-load-ability, do it now.
    ** Bulk-load an etab if a) the base table was opened with X (not IX)
    ** locking, b) the base table appears to have no rows, and
    ** c) someone higher up has marked the query as "multi-row".
    */
    if (bqcb->bqcb_load_etabs == BQCB_LOAD_UNKNOWN)
    {
	bqcb->bqcb_load_etabs = BQCB_LOAD_NO;	/* Assume nope. */
	if (bqcb->bqcb_multi_row
	  && bqcb->bqcb_x_lock
	  && pcb->pcb_tcb->tcb_rel.reltups == 0
	  && (pcb->pcb_tcb->tcb_rel.relstat & (TCB_JOURNAL | TCB_GATEWAY)) == 0)
	{
	    bqcb->bqcb_load_etabs = BQCB_LOAD_YES;
	}
    }
    /* If we're starting a load on a new column, make sure there is a "held
    ** record" area.  There might already be one from a prior row.
    */
    if (bqcb->bqcb_load_etabs == BQCB_LOAD_YES && pcb->pcb_held_record == NULL)
    {
	pcb->pcb_held_record = (DMPE_RECORD *) dm0m_tballoc(sizeof(DMPE_RECORD));
    }
    return (E_DB_OK);

} /* dmpe_begin_dml */

/*
** Name: dmpe_end_row -- End of a row with LOBs
**
** Description:
**	At the end of processing for a row, we need to reset the
**	logical key used for LOBs in that row.  Each row gets a
**	new logical key.
**
** Inputs:
**	r		DMP_RCB opened against the base table
**
** Outputs:
**	None
**
** History:
**	16-Apr-2010 (kschendel) SIR 123485
**	    Written.
*/

void
dmpe_end_row(DMP_RCB *r)
{

    DMP_RCB *key_rcb;

    /* The base table RCB should point to the BQCB, which should point
    ** to the logical-key-generator RCB.  If the latter isn't there,
    ** just exit, although it could be an error.
    */

    if (r->rcb_bqcb_ptr == NULL)
	return;			/* Strange, but whatever. */

    key_rcb = r->rcb_bqcb_ptr->bqcb_logkey_rcb;
    if (key_rcb != NULL)
    {
	key_rcb->rcb_logkey.olk_high_id = 0;
	key_rcb->rcb_logkey.olk_low_id = 0;
    }

} /* dmpe_end_row */

/*
** Name: dmpe_query_end -- End of query that uses LOBs
**
** Description:
**
**	When a query or transaction is finished, this routine is called
**	to clean up any in-progress LOB handling.
**	- If there were any in-flight etab bulk-loads that our session was
**	  doing, finish (or abort) them.
**	- Toss the blob query context blocks (BQCB's), unless there are
**	  locators around, in which case reset them instead of tossing
**	  them (in case a locator is referenced).
**	- Delete all holding temps belonging to the session, unless the
**	  caller specifically asks not to.  (which might be the case if
**	  the query is still in progress at an outer level, e.g. an
**	  outer DB procedure calling an inner one that did a commit or
**	  rollback.)
**
**	In any situation where all tables opened by a session are going
**	to be closed, dmpe-query-end should be called first.  If the
**	query happens to continue, that is OK, we'll just rebuild the
**	query context as if it were a new query.  (Which in effect it
**	is, outside of kept blob holding temps.)
**
**	Query-end must also be called before any dangling-PCB cleanup loop.
**
** Inputs:
**	was_error		TRUE if overall query had error
**				This is a shortcut hint to toss any in-
**				progress bulk-loads instead of finishing
**				them (and then rolling them back).
**	delete_temps		TRUE to delete holding temps, which is the
**				normal call;  FALSE to not delete them.
**	dberr			DB_ERROR error status block
**
** Outputs:
**	Returns E_DB_OK or error with dberr updated.
**	All BQCB's deleted or reset.
**
** History:
**	16-Apr-2010 (kschendel) SIR 123485
**	    Written.
*/

DB_STATUS
dmpe_query_end(bool was_error, bool delete_temps, DB_ERROR *dberr)
{
    CS_SID sid;
    DB_STATUS status = E_DB_OK;
    DML_SCB *scb;		/* Main session thread's SCB */
    DMPE_BQCB *bqcb, *bqcb_next;
    DMT_CB dmt;

    CLRDBERR(dberr);

    CSget_sid(&sid);
    scb = GET_DML_SCB(sid);
    if (scb == NULL || scb != scb->scb_oq_next->odcb_scb_ptr)
	return (E_DB_OK);	/* Not the main thread, just return */

    if (delete_temps)
    {
	status = dmpe_free_temps(scb->scb_oq_next, dberr);
	if (status != E_DB_OK)
	    return (status);
    }
    /* Child threads should be done at this stage, or the call is no good
    ** anyway.  Don't bother with the bqcb mutex.
    **
    ** First pass is to finish any loads, close logical-key generators.
    */
    bqcb_next = scb->scb_bqcb_next;
    while (bqcb_next != NULL)
    {
	bqcb = bqcb_next;
	bqcb_next = (DMPE_BQCB *) bqcb->hdr.obj_next;
	if (bqcb->bqcb_logkey_rcb != NULL)
	{
	    /* If there's a logical key generator RCB, we're doing PUTs,
	    ** which might mean that etab bulk-loads were in progress.
	    */
	    if (bqcb->bqcb_load_etabs == BQCB_LOAD_YES)
	    {
		DMPE_PCB *pcb;
		DMP_ET_LIST *etl;
		DMP_TCB *bt;		/* Base table TCB */
		i4 toss;

		bt = bqcb->bqcb_logkey_rcb->rcb_tcb_ptr;
		for (;;)
		{
		    dm0s_mlock(&bt->tcb_et_mutex);
		    etl = bt->tcb_et_list->etl_next;
		    while (etl != bt->tcb_et_list
		      && (etl->etl_etab.etab_type != DMP_LO_ETAB
			  || (etl->etl_status & ETL_LOAD) == 0
			  || etl->etl_access_id == NULL
			  || etl->etl_sid != sid) )
			etl = etl->etl_next;
		    dm0s_munlock(&bt->tcb_et_mutex);
		    if (etl == bt->tcb_et_list)
			break;

		    /* Looks like an etab load that our session was doing.
		    ** (Note, etab loads are always done by the main session
		    ** thread at the present time!)
		    ** Either end the load normally, or close the load RCB
		    ** thus tossing the load.
		    ** There must be a PCB for this bulk-load, use it.
		    */
		    pcb = etl->etl_pcb;
		    if (! was_error)
		    {
			pcb->pcb_dmrcb->dmr_access_id = etl->etl_access_id;
			pcb->pcb_dmrcb->dmr_flags_mask = DMR_ENDLOAD;
			pcb->pcb_dmrcb->dmr_count = 0;
			pcb->pcb_dmrcb->dmr_mdata = NULL;
			status = dmr_load(pcb->pcb_dmrcb);
			if (status != E_DB_OK)
			{
			    *dberr = pcb->pcb_dmrcb->error;
			    return (status);
			}
		    }
		    /* Close the load table */
		    pcb->pcb_dmtcb->dmt_record_access_id = etl->etl_access_id;
		    pcb->pcb_dmtcb->dmt_flags_mask = 0;
		    status = dmt_close(pcb->pcb_dmtcb);
		    if (status != E_DB_OK)
		    {
			uleFormat(&pcb->pcb_dmtcb->error, 0, NULL,
					ULE_LOG, NULL, (char *)NULL, 0L,
					(i4 *)NULL, &toss, 0);
			SETDBERR(dberr, 0, E_DM9B04_LOAD_DMPE_ERROR);
		    }
		    etl->etl_access_id = NULL;
		    /* Leave etl_pcb, etl_sid, etl_status alone so a post-
		    ** pass can deallocate the PCB's.  One pcb might have
		    ** loaded multiple etabs, so don't do it yet!
		    **
		    ** Loop back to look for another etab to end.
		    */
		} /* for */

		/* Now, look for etabs that we *were* loading, and close
		** the PCB's.  As each one is closed, dmpe-deallocate will
		** clean up all ET list entries using that PCB.
		*/
		for (;;)
		{
		    dm0s_mlock(&bt->tcb_et_mutex);
		    etl = bt->tcb_et_list->etl_next;
		    while (etl != bt->tcb_et_list
		      && (etl->etl_etab.etab_type != DMP_LO_ETAB
			  || (etl->etl_status & ETL_LOAD) == 0
			  || etl->etl_pcb == NULL
			  || etl->etl_sid != sid) )
			etl = etl->etl_next;
		    dm0s_munlock(&bt->tcb_et_mutex);
		    if (etl == bt->tcb_et_list)
			break;
		    pcb = etl->etl_pcb;
		    dmpe_deallocate(pcb);
		}
	    } /* if might have been doing LOADs */

	    /* Close the logical-key generator RCB */
	    MEfill(sizeof(DMT_CB), 0, &dmt);
	    dmt.length = sizeof(DMT_TABLE_CB);
	    dmt.type = DMT_TABLE_CB;
	    dmt.ascii_id = DMT_ASCII_ID;
	    dmt.dmt_db_id = (PTR) scb->scb_oq_next;
	    dmt.dmt_tran_id = (PTR) &scb->scb_x_next->xcb_tran_id;
	    dmt.dmt_flags_mask = 0;
	    dmt.dmt_record_access_id = (PTR) bqcb->bqcb_logkey_rcb;
	    (void) dmt_close(&dmt);
	    bqcb->bqcb_logkey_rcb = NULL;
	}
	/* Reset the BQCB for end-of-query */
	bqcb->bqcb_load_etabs = BQCB_LOAD_UNKNOWN;
	bqcb->bqcb_multi_row = FALSE;
	bqcb->bqcb_table_lock = FALSE;
	bqcb->bqcb_x_lock = FALSE;
	bqcb->bqcb_crib = NULL;
    }

    /* Now, delete BQCB's if allowable.  We'll double check the XCB / RCB
    ** list for any open RCB's that reference a BQCB;  such a thing might
    ** possibly mean that QEF has goofed.
    */
    if (scb->scb_lloc_cxt == NULL && scb->scb_bqcb_next != NULL)
    {
	DML_XCB *xcb = scb->scb_x_next;
	DMP_RCB *r;

	if (xcb != (DML_XCB *) &scb->scb_x_next)
	{
	    r = xcb->xcb_rq_next;
	    while (r != (DMP_RCB *) &xcb->xcb_rq_next)
	    {
		r = (DMP_RCB *) ((char *)r - CL_OFFSETOF(DMP_RCB, rcb_xq_next));
		if (r->rcb_bqcb_ptr != NULL)
		    return (E_DB_OK);	/* Whoops, stop now */
		r = r->rcb_xq_next;
	    }
	}
	/* Looks OK, delete 'em */
	while (scb->scb_bqcb_next != NULL)
	{
	    bqcb = scb->scb_bqcb_next;
	    scb->scb_bqcb_next = (DMPE_BQCB *) bqcb->hdr.obj_next;
	    dm0m_deallocate((DM_OBJECT **) &bqcb);
	}
    }
    return (E_DB_OK);

} /* dmpe_query_end */

/*
** {
** Name: dmpe_check_args - Check parameters for consistency
**
** Description:
**      This routine checks the ADP_POP_CB for consistency based on
**	the operation being performed.
**
** Inputs:
**      op_code                         ADP operation being performed
**	pop_cb				ADP_POP_CB (ptr to) describing the
**					operation
**
** Outputs:
**      pop_cb->pop_error               Filled appropriately.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      03-Jan-1990 (fred)
**          Created.
**      12-Nov-1993 (fred)
**          Fixup check for rcb address.  It should be checked for
**          only for a non-zero length coupon, since a zero length one
**          may have never created a file.
**	6-May-2004 (schka24)
**	    Clean up some can't-get-here tests.
*/
static DB_STATUS
dmpe_check_args(i4                op_code,
		ADP_POP_CB         *pop_cb)
{
    DB_STATUS           status = E_DB_ERROR;
    ADP_PERIPHERAL      *p;

    do
    {
	if (!pop_cb)
	    break;

	CLRDBERR(&pop_cb->pop_error);

	if ((pop_cb->pop_type != ADP_POP_TYPE) ||
	    (pop_cb->pop_length < sizeof(ADP_POP_CB)))
	{
	    SETDBERR(&pop_cb->pop_error, 0, E_AD700B_ADP_BAD_POP_CB);
	    break;
	}

	if (op_code == ADP_LOCATOR_TO_CPN)
	{
	    /* FIX ME check args for ADP_LOCATOR_TO_CPN */
	}
	else if (op_code == ADP_CPN_TO_LOCATOR)
	{
	    /* FIX ME check args for ADP_CPN_TO_LOCATOR */
	}
	else if ((op_code != ADP_INFORMATION)
		&& (op_code != ADP_FREE_NONSESS_OBJS)
		&& (op_code != ADP_FREE_LOCATOR))
	{
	    if (op_code != ADP_COPY)
	    {
		/* Operations other than info, free-objs, and copy,
		** if not first time through,
		** require a valid looking PCB in pop_user_arg if
		** there's anything there at all.
		*/
		if (	pop_cb->pop_user_arg
		    &&	((pop_cb->pop_continuation & ADP_C_BEGIN_MASK) == 0)
		    &&	(pop_cb->pop_user_arg != (PTR)
			    ((DMPE_PCB *) pop_cb->pop_user_arg)->pcb_self))
		{
		    SETDBERR(&pop_cb->pop_error, 0, E_AD700C_ADP_BAD_POP_UA);
		    break;
		}
	    }
	    /* All except info and free-objs require a coupon
	    ** style ADP_PERIPHERAL value
	    */
	    p = (ADP_PERIPHERAL *) pop_cb->pop_coupon->db_data;
	    if (p == NULL || p->per_tag != ADP_P_COUPON)
	    {
		SETDBERR(&pop_cb->pop_error, 0, E_AD700D_ADP_NO_COUPON);
		break;
	    }

	    /* removed unhelpful test for  E_AD700E_ADP_BAD_COUPON */
	}
	status = E_DB_OK;
    } while (0);

    return(status);
}

/*
** {
** Name: dmpe_tcb_populate	- Add extended table catalog info to TCB
**
** Description:
**      This routine scans the extended table catalog (ii_extended_relation)
**	to find any extended relations which belong to the table involved.  This
**	can be found by looking at the coupon for this peripheral operation
**	(POP).
**
**	This routine will open the catalog, and scan it based on the key (which
**	is the table id of the base table).  Any extended relations which are
**	found (and were not found before) are added to the end of the TCB's scan
**	list.  If the caller didn't get the TCB ET mutex, we get it here;
**	we always release it before returning.
**
** Inputs:
**      pop_cb				The Peripheral OPerations control
**					block
**
** Outputs:
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      08-Jan-1990 (fred)
**          Created.
**      13-Apr-1994 (fred)
**          Avoid losing the interrupt status.  It needs to get out to
**          avoid getting logged...
**	7-May-2004 (schka24)
**	    Use TCB pointer in PCB, has to be set by the time we get here.
**	    Don't hold et mutex during catalog scanning, makes me nervous.
**	19-Apr-2010 (kschendel) SIR 123485
**	    Clean up mutex handling.  Make sure pop_error is clean when
**	    we return OK.
*/
static DB_STATUS
dmpe_tcb_populate(ADP_POP_CB   *pop_cb )

{
    DB_STATUS           status;
    DMPE_PCB            *pcb = (DMPE_PCB *) pop_cb->pop_user_arg;
    PTR		    	save_ptr;
    i4			err_code;
    DB_TAB_ID		tab_id;
    DMP_TCB		*tcb = pcb->pcb_tcb;
    DMP_ET_LIST		*list;
    DMP_ETAB_CATALOG	etab_record;
    i4			error;

    CLRDBERR(&pop_cb->pop_error);

    for (;;)
    {
	status = E_DB_OK;
	save_ptr = pcb->pcb_dmrcb->dmr_attr_desc.ptr_address;

	tab_id.db_tab_index = 0;
	tab_id.db_tab_base = tcb->tcb_rel.reltid.db_tab_base;

	while (status == E_DB_OK)
	{
	    status = dmpe_cat_scan(pop_cb, &tab_id, &etab_record);
	    if (status)
	    {
		break;
	    }
	    dm0s_mlock(&tcb->tcb_et_mutex);

	    list = tcb->tcb_et_list->etl_next;

	    /* Do we already know about this table? */

	    while (	(list != tcb->tcb_et_list)
		  &&
			(etab_record.etab_extension !=
				    list->etl_etab.etab_extension))
	    {
		list = list->etl_next;
	    }

	    /* Don't add HEAP etabs to the tcb_et_list */
	    if (etab_record.etab_type == DMP_LO_HEAP)
	    {
		dm0s_munlock(&tcb->tcb_et_mutex);
		continue;
	    }

	    /* If not, add the table to the list of known tables */

	    if (list == tcb->tcb_et_list)
	    {
		/* Holding the tcb ET mutex, a bit non-optimal but easier */
		status = dm0m_allocate(	sizeof(DMP_ET_LIST),
			(i4) 0,
			(i4) ETL_CB,
			(i4) ETL_ASCII_ID,
			(char *) tcb,
			(DM_OBJECT **) &list,
			&pop_cb->pop_error);
		if (status)
		{
		    dm0s_munlock(&tcb->tcb_et_mutex);
		    break;
		}
		list->etl_status = 0;
		STRUCT_ASSIGN_MACRO(etab_record, list->etl_etab);
		list->etl_next = tcb->tcb_et_list;
		list->etl_prev = tcb->tcb_et_list->etl_prev;
		tcb->tcb_et_list->etl_prev = list;
		list->etl_prev->etl_next = list;
	    }
	    dm0s_munlock(&tcb->tcb_et_mutex);
	} /* while more cat entries */
	if (status)
	    break;
    }
    if (status)
    {
	if (	(pop_cb->pop_error.err_code != E_DM0054_NONEXISTENT_TABLE)
	    &&  (pop_cb->pop_error.err_code != E_DM0065_USER_INTR)
	    &&	(pop_cb->pop_error.err_code != E_DM0055_NONEXT)
            &&  (pop_cb->pop_error.err_code != E_DM016B_LOCK_INTR_FA))
	{
	    uleFormat(&pop_cb->pop_error, 0, NULL , ULE_LOG,
			NULL, (char *) NULL, 0L, (i4 *) NULL,
			&err_code, 0);
	    SETDBERR(&pop_cb->pop_error, 0, E_DM9B06_DMPE_BAD_CAT_SCAN);
	}
	else if ((pop_cb->pop_error.err_code != E_DM0065_USER_INTR) &&
                 (pop_cb->pop_error.err_code != E_DM016B_LOCK_INTR_FA))
	{
	    /*
	    **	This is "OK" because the table will be created later.
	    **	This situation happens once per DB, since the
	    **	iiextended_relation catalog is created only if/when it is
	    **	needed.  But we want to leave the interrupt status so
	    **	it gets correctly reflected back...
	    */

	    status = E_DB_OK;
	    CLRDBERR(&pop_cb->pop_error);
	}
    }

    pcb->pcb_dmrcb->dmr_attr_desc.ptr_address = save_ptr;
    pcb->pcb_dmrcb->dmr_data.data_address = (char *) pcb->pcb_record;
    pcb->pcb_dmrcb->dmr_data.data_in_size = sizeof(DMPE_RECORD);
    return(status);
}

/*
** {
** Name: dmpe_add_extension	- Add a table extension for a table
**
** Description:
**      This routine adds a table extension for a particular table.
**	This entails the following:
**
**          1) Call dmpe_create_extension to create the etab and insert
**             a new record into the ii_extended_relation catalog.
**	    2) Add the new table to the base tcb's list
**		 (ala dmpe_tcb_populate()).
**	    3) Return.
**
**	This routine is not used to add etabs for session temporary tables.
**	Session temp etabs are created in dmpe_temp.  This routine is
**	only for permanent etabs.
**
** Inputs:
**      pop_cb                          Peripheral Control Block.
**	base_tcb			Base table TCB (since this is only used
**					from dmpe-put, there will be a valid
**					pcb_tcb and this is it)
**	att_id				LOB column we're adding an etab for
**	etab_id				An output
**
** Outputs:
**	etab_id				Returned etab table db_tab_base.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      08-jan-1990 (fred)
**          Created.
**	05-oct-1992 (rogerk)
**	    Reduced logging project: changed references to tcb fields to
**	    use new tcb_table_io structure fields.
**      15-Jul-1993 (fred)
**          Added correct default value indicators for table creation.
**	31-jan-1994 (bryanp) B58478, B58479, B58480, B58481
**	    Initialize dmucb to 0 to indicate no DMU_CB has been allocated.
**	    Check dmpe_qdata return code.
**	    Use accurate error code if dmpe_create_catalog fails.
**	    Check dmpe_close return code during error cleanup.
**      13-Apr-1994 (fred)
**          Allow interrupt status to get out unscathed.  This will
**          avoid unnecessary logging.
**	7-jul-94 (robf)
**          Add internal flag to modify request.
**	06-mar-1996 (stial01 for bryanp)
**	    When creating a table to hold a blob, choose a sufficiently large
**          page size.
**	13-aug-96 (somsa01)
**	    If the TCB_JON flag is set for the base table, we enable this for
**	    the extended table as well. In terms of journaling, this keeps both
**	    the base table and its extended table in sync with each other.
**	11-may-99 (stephenb)
**	    Allow duplicates in extension tables, this prevents an un-necessary
**	    duplicate search when adding records.
**      03-aug-99 (stial01)
**          Moved some of the code into dmpe_create_extension
**	7-May-2004 (schka24)
**	    PCB knows where its XCB is now.  Mutex ET list manipulations.
**	19-Apr-2010 (kschendel) SIR 123485
**	    Clean up ET mutex handling a little.
*/
static DB_STATUS
dmpe_add_extension(ADP_POP_CB  *pop_cb, DMP_TCB *base_tcb, i4 att_id,
	u_i4 *new_etab)
{
    DB_STATUS           status;
    DMPE_PCB            *pcb = (DMPE_PCB *) pop_cb->pop_user_arg;
    DML_XCB             *xcb;
    i4                  err_code;
    DMP_TCB             *t;
    DMP_ET_LIST         *list;
    DMP_ETAB_CATALOG    etab_record;

    CLRDBERR(&pop_cb->pop_error);

    do
    {
	xcb = pcb->pcb_xcb;

	t = pcb->pcb_tcb;
	status = dmpe_create_extension(xcb, &t->tcb_rel,
		/* pgsize ignored */0, t->tcb_table_io.tbio_location_array,
		t->tcb_table_io.tbio_loc_count,
		att_id,
		t->tcb_atts_ptr[att_id].type,
		&etab_record, &pop_cb->pop_error);
	if (status != E_DB_OK)
	    break;


	/* Add the new etab record to the tcb extension info */

	status = dm0m_allocate(	sizeof(DMP_ET_LIST),
		(i4) 0,
		(i4) ETL_CB,
		(i4) ETL_ASCII_ID,
		(char *) t,
		(DM_OBJECT **) &list,
		&pop_cb->pop_error);
	if (status)
	    break;

	list->etl_status = 0;
	STRUCT_ASSIGN_MACRO(etab_record, list->etl_etab);

	dm0s_mlock(&t->tcb_et_mutex);
	list->etl_next = t->tcb_et_list;
	list->etl_prev = t->tcb_et_list->etl_prev;
	t->tcb_et_list->etl_prev = list;
	list->etl_prev->etl_next = list;
	dm0s_munlock(&t->tcb_et_mutex);
	*new_etab = etab_record.etab_extension;
    } while (FALSE);

    if (DB_FAILURE_MACRO(status))
    {
	if ((pop_cb->pop_error.err_code != E_DM0065_USER_INTR) &&
            (pop_cb->pop_error.err_code != E_DM016B_LOCK_INTR_FA))
	{
	    uleFormat(&pop_cb->pop_error, 0, NULL , ULE_LOG,
			NULL, (char *) NULL, 0L, (i4 *) NULL,
			&err_code, 0);
	    SETDBERR(&pop_cb->pop_error, 0, E_DM9B07_DMPE_ADD_EXTENSION);
	}
    }
    pcb->pcb_dmrcb->dmr_data.data_address = (char *) pcb->pcb_record;
    pcb->pcb_dmrcb->dmr_data.data_in_size = sizeof(DMPE_RECORD);
    return(status);
}


/*
** {
** Name: dmpe_create_extension	- Create a table extension for a table
**
** Description:
**      This routine creates a table extension for a particular table.
**	This entails the following:
**
**	    1) Open the table extension catalog (exclusively)
**	    2) Build up the create block for the new table.
**	    3) Create a new table, marking it as a table extension.
**	    4) Insert the new tuple into the ii_extended_relation catalog.
**	    5) Return.
**
**
**	The table is created as if:
**
**	    CREATE TABLE iietab_<base_id>_<extension_id>
**	    (
**		per_key		TABLE_KEY NOT SYSTEM_MAINTAINED,
**		per_segment0    INTEGER,
**		per_segment1    INTEGER,
**		per_next	INTEGER,
**		per_value	    <underlying dv>
**	    )
**		WITH JOURNALING = that of base table,
**		     LOCATIONS = (those of base table);
**
**	This routine is for real etabs, not session temp etabs.
**	See dmpe_temp for session temp etabs.
**
** Inputs:
**      xcb                             The transaction id.
**      rel_record                      The base table iirelation record
**      etab_pagesize                   Suggested page size
**      loc_array                       Locations to create etab with
**      loc_count                       Location count
**      attr_id                         Base table attribute id
**      attr_type                       Base table attribute type
**
** Outputs:
**      etab_record                     The record that was inserted into
**                                      the iiextended_relation catalog
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      03-aug-1999 (fred)
**          Created from dmpe_add_extension
**      07-Jan-2000 (fanra01)
**          Add apersand operator on rel_record->relid.
**	11-Feb-2004 (schka24)
**	    Make sure no partition def junk in create dmucb.
**	13-dec-04 (inkdo01)
**	    Init attr_collID.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Use MISC_CB to hold DMU/DMR/DMT, etc to conform
**	    to dm0m types.
**       25-Apr-2007 (kibro01) b115996
**          Ensure the misc_data pointer in a MISC_CB is set correctly and
**          that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**          since that's what we're actually using (the type is MISC_CB)
**	29-Sept-09 (troal01)
**		Init attr_geomtype and attr_srid.
*/
DB_STATUS
dmpe_create_extension(
	DML_XCB                 *xcb,
	DMP_RELATION            *rel_record,
	i4                      etab_pagesize,
	DMP_LOCATION            *loc_array,
	i4                      loc_count,
	i4                      attr_id,
	i2                      attr_type,
	DMP_ETAB_CATALOG        *etab_record,
	DB_ERROR                *dberr)
{
    DB_STATUS           status, local_status;
    i4                  local_error;
    i4			i;
    i4		        ind = 0;
    DMP_MISC		*misc_cb = NULL;
    DMU_CB		*dmucb;
    DMR_CB              dmrcb;
    DMT_CB              dmtcb;
    DMT_CB		jon_dmt_cb;
    DMU_CHAR_ENTRY	char_entry[7];
    DB_LOC_NAME		*locs;
    DB_DATA_VALUE       underdv;
    ADF_CB              adf_cb;
    u_i4                etab_extension;
    DMT_SHW_CB          dmt_shw;
    DMT_TBL_ENTRY       dmt_tbl_entry;
    CS_SID              sid;
    bool                etl_opened = FALSE;
    i4                  create_pagesize = 0;
    i4			perm_etab_struct;
    i4			etab_tbl_status;
    i4			error;

    /*
    ** Number of attributes in an extended table:
    */

#define             DMPE_ATT_COUNT      5

    DMF_ATTR_ENTRY	*atts[DMPE_ATT_COUNT];
    DMF_ATTR_ENTRY	att_ents[DMPE_ATT_COUNT];
    DMU_KEY_ENTRY	key_ents[DMPE_KEY_COUNT];
    DMU_KEY_ENTRY	*keys[DMPE_KEY_COUNT];

    for (;;)
    {
	/* Consistency check, should not come here to create temp etab */
	if ((i4)rel_record->reltid.db_tab_base < 0)
	    dmd_check(E_AD9999_INTERNAL_ERROR);

	perm_etab_struct = dmf_svcb->svcb_blob_etab_struct;

	/* See if this is first etab created for this table */
	status = dmpe_get_etab(xcb, rel_record, &etab_extension,
				TRUE, dberr);
	if (status && dberr->err_code != E_DM0055_NONEXT)
	    break;

	/* Clear residual error code */
	CLRDBERR(dberr);

	if (status)
	{
	    /*
	    ** This is only possible from create table, alter add column,
	    ** upgradedb
	    ** Tell adi_per_under the page size we want to use
	    ** Caller page size is only used if this is the first etab
	    */
	    if (etab_pagesize)
		create_pagesize = etab_pagesize;
	    else if (dmf_svcb->svcb_etab_pgsize)
		create_pagesize = dmf_svcb->svcb_etab_pgsize;
	    else
		create_pagesize = rel_record->relpgsize; /* same as base */

	    /*
	    ** Create etab with same journaling status as base table
	    */
	    etab_tbl_status = rel_record->relstat;

#ifdef xDEBUG
	    TRdisplay("Creating first etab for %~t with pagesize %d journal %d jon %d\n",
		sizeof(rel_record->relid), &rel_record->relid, create_pagesize,
		(etab_tbl_status & TCB_JOURNAL),
		(etab_tbl_status & TCB_JON));
#endif
	}
	else
	{
	    /* Find pagesize info for existing etab */
	    dmt_shw.type = DMT_SH_CB;
	    dmt_shw.length = sizeof(DMT_SHW_CB);
	    dmt_shw.dmt_char_array.data_in_size = 0;
	    dmt_shw.dmt_flags_mask = DMT_M_TABLE;
	    dmt_shw.dmt_tab_id.db_tab_base = etab_extension;
	    dmt_shw.dmt_tab_id.db_tab_index = 0;
	    dmt_shw.dmt_table.data_address = (PTR) &dmt_tbl_entry;
	    dmt_shw.dmt_table.data_in_size = sizeof(DMT_TBL_ENTRY);
	    dmt_shw.dmt_char_array.data_address = (PTR)NULL;
	    dmt_shw.dmt_char_array.data_in_size = 0;
	    dmt_shw.dmt_char_array.data_out_size  = 0;
	    dmt_shw.error.err_code = 0;
	    CSget_sid(&sid);
	    dmt_shw.dmt_session_id	= sid;
	    dmt_shw.dmt_db_id = (PTR)xcb->xcb_odcb_ptr;
	    status = dmt_show(&dmt_shw);
	    if (status != E_DB_OK)
		break;

	    create_pagesize = dmt_tbl_entry.tbl_pgsize;
	    /*
	    ** Create etab with same journaling status as other etabs
	    */
	    etab_tbl_status = dmt_tbl_entry.tbl_status_mask;

#ifdef xDEBUG
	    TRdisplay("Creating new etab for %~t with pagesize %d journal %d jon %d\n",
		sizeof(rel_record->relid), &rel_record->relid, create_pagesize,
		(etab_tbl_status & TCB_JOURNAL),
		(etab_tbl_status & TCB_JON));
#endif

	}

	/*
	** Get the underlying datatype/length
	*/
	MEfill(sizeof(ADF_CB),0,(PTR)&adf_cb);
	adf_cb.adf_maxstring = DMPE_SEGMENT_LENGTH;

        if (CMischarset_utf8())
          adf_cb.adf_utf8_flag = AD_UTF8_ENABLED;
        else
          adf_cb.adf_utf8_flag = 0;

	status = adi_per_under(&adf_cb, attr_type, &underdv);

	if (DB_FAILURE_MACRO(status))
	    break;

	/* Try to open iiextended_relation */
	MEfill(sizeof(DMT_CB), 0, (PTR) &dmtcb);
	dmtcb.length = sizeof(DMT_TABLE_CB);
	dmtcb.type = DMT_TABLE_CB;
	dmtcb.ascii_id = DMT_ASCII_ID;
	dmtcb.dmt_db_id = (PTR) xcb->xcb_odcb_ptr;
	dmtcb.dmt_tran_id = (PTR) xcb;
	dmtcb.dmt_sequence = 0;  /* only need sequence# for deferred cursor */
	dmtcb.dmt_lock_mode = DMT_IX;
	dmtcb.dmt_update_mode = DMT_U_DIRECT;
	dmtcb.dmt_access_mode = DMT_A_WRITE;
	dmtcb.dmt_id.db_tab_base = DM_B_ETAB_TAB_ID;
	dmtcb.dmt_id.db_tab_index = DM_I_ETAB_TAB_ID;
	dmtcb.dmt_record_access_id = 0;
	status = dmt_open(&dmtcb);

	if (DB_FAILURE_MACRO(status))
	{
	    *dberr = dmtcb.error;
	    break;
	}

	etl_opened = TRUE;
	status = dm0m_allocate(	(sizeof(DMP_MISC) +
			sizeof(DMU_CB) +
			(sizeof(DB_LOC_NAME) * loc_count)),
			(i4) 0,
			(i4) MISC_CB,
			(i4) MISC_ASCII_ID,
			(char *) 0,
			(DM_OBJECT **) &misc_cb,
			dberr);
	if (status)
	    break;

	/* Create etab for this peripheral attribute */
	dmucb = (DMU_CB*)((char*)misc_cb + sizeof(DMP_MISC));
	misc_cb->misc_data = (char*) dmucb;
	dmucb->type = DMU_UTILITY_CB;
	dmucb->length = sizeof(DMU_UTILITY_CB);
	dmucb->dmu_flags_mask = DMU_INTERNAL_REQ;

	STRUCT_ASSIGN_MACRO(rel_record->relowner, dmucb->dmu_owner);
	dmucb->dmu_db_id = (PTR) xcb->xcb_odcb_ptr;
	dmucb->dmu_tran_id = (PTR) xcb;
	dmucb->dmu_tbl_id.db_tab_base = rel_record->reltid.db_tab_base;
	dmucb->dmu_tbl_id.db_tab_index = 0;
	dmucb->dmu_location.data_address = (PTR)
				((char *) dmucb + sizeof(DMU_CB));
	dmucb->dmu_location.data_in_size = sizeof(DB_LOC_NAME) * loc_count;
	locs = (DB_LOC_NAME *) dmucb->dmu_location.data_address;

	dmucb->dmu_key_array.ptr_address = 0;
	dmucb->dmu_key_array.ptr_size = 0;
	dmucb->dmu_key_array.ptr_in_count = 0;

	dmucb->dmu_char_array.data_address = (char *) char_entry;
	dmucb->dmu_char_array.data_in_size = 4 * sizeof(char_entry[0]);

	/*
	** If creating first etab, create using journaling status of base table
	** Otherwise create this etab using journaling status of other etabs
	*/
	char_entry[0].char_id = DMU_JOURNALED;
	char_entry[0].char_value = (etab_tbl_status & TCB_JOURNAL)
					? DMU_C_ON : DMU_C_OFF;

	char_entry[1].char_id = DMU_EXT_CREATE;
	char_entry[1].char_value = DMU_C_ON;

	/*
	** For VPS etab support use the same page size as base table
	** Or override with config parameter (like dmf_svcb->svcb_etab_pgsize)
	*/
	char_entry[2].char_id = DMU_PAGE_SIZE;
	char_entry[2].char_value = create_pagesize;

	/* allow duplicates */
	char_entry[3].char_id = DMU_DUPLICATES;
	char_entry[3].char_value = DMU_C_ON;

	for (i = 0; i < loc_count; i++)
	{
	    STRUCT_ASSIGN_MACRO(loc_array[i].loc_name, locs[i]);
	}

	dmucb->dmu_attr_array.ptr_in_count = DMPE_ATT_COUNT;
	dmucb->dmu_attr_array.ptr_size = sizeof(DMF_ATTR_ENTRY);
	dmucb->dmu_attr_array.ptr_address = (PTR) atts;

	for (i = 0; i < DMPE_ATT_COUNT; i++)
	{
	    atts[i] = &att_ents[i];
	    att_ents[i].attr_defaultTuple = (DB_IIDEFAULT *) 0;
	    SET_CANON_DEF_ID(att_ents[i].attr_defaultID, DB_DEF_NOT_DEFAULT);
	}

	for (i = 0; i < DMPE_KEY_COUNT; i++)
	{
	    keys[i] = &key_ents[i];
	}


	MEmove(7, "per_key", ' ', sizeof(att_ents[0].attr_name),
	       (PTR) &att_ents[0].attr_name);
	att_ents[0].attr_type = DB_TABKEY_TYPE;
	att_ents[0].attr_size = sizeof(DB_TAB_LOGKEY_INTERNAL);
	att_ents[0].attr_precision = 0;
	att_ents[0].attr_collID = 0;
	att_ents[0].attr_geomtype = -1;
	att_ents[0].attr_srid = -1;
	att_ents[0].attr_flags_mask = DMU_F_NDEFAULT;
	att_ents[0].attr_encflags = 0;
	att_ents[0].attr_encwid = 0;

	STRUCT_ASSIGN_MACRO(att_ents[0].attr_name, key_ents[0].key_attr_name);
	key_ents[0].key_order = DMU_ASCENDING;	/* Useless! */

	MEmove(12, "per_segment0", ' ', sizeof(att_ents[1].attr_name),
	       (PTR) &att_ents[1].attr_name);
	att_ents[1].attr_type = DB_INT_TYPE;
	att_ents[1].attr_size = 4;
	att_ents[1].attr_precision = 0;
	att_ents[1].attr_collID = 0;
	att_ents[1].attr_geomtype = -1;
	att_ents[1].attr_srid = -1;
	att_ents[1].attr_flags_mask = DMU_F_NDEFAULT;
	att_ents[1].attr_encflags = 0;
	att_ents[1].attr_encwid = 0;

	STRUCT_ASSIGN_MACRO(att_ents[1].attr_name, key_ents[1].key_attr_name);
	key_ents[1].key_order = DMU_ASCENDING;	/* Useless! */

	MEmove(12, "per_segment1", ' ', sizeof(att_ents[2].attr_name),
	       (PTR) &att_ents[2].attr_name);
	att_ents[2].attr_type = DB_INT_TYPE;
	att_ents[2].attr_size = 4;
	att_ents[2].attr_precision = 0;
	att_ents[2].attr_collID = 0;
	att_ents[2].attr_geomtype = -1;
	att_ents[2].attr_srid = -1;
	att_ents[2].attr_flags_mask = DMU_F_NDEFAULT;
	att_ents[2].attr_encflags = 0;
	att_ents[2].attr_encwid = 0;

	STRUCT_ASSIGN_MACRO(att_ents[2].attr_name, key_ents[2].key_attr_name);
	key_ents[2].key_order = DMU_ASCENDING;	/* Useless! */

	MEmove(8, "per_next", ' ', sizeof(att_ents[3].attr_name),
	       (PTR) &att_ents[3].attr_name);
	att_ents[3].attr_type = DB_INT_TYPE;
	att_ents[3].attr_size = 4;
	att_ents[3].attr_precision = 0;
	att_ents[3].attr_collID = 0;
	att_ents[3].attr_geomtype = -1;
	att_ents[3].attr_srid = -1;
	att_ents[3].attr_flags_mask = DMU_F_NDEFAULT;
	att_ents[3].attr_encflags = 0;
	att_ents[3].attr_encwid = 0;

	MEmove(9, "per_value", ' ', sizeof(att_ents[4].attr_name),
	       (PTR) &att_ents[4].attr_name);
	att_ents[4].attr_type = underdv.db_datatype;
	att_ents[4].attr_size = underdv.db_length;
	att_ents[4].attr_precision = underdv.db_prec;
	att_ents[4].attr_collID = 0;
	att_ents[4].attr_geomtype = -1;
	att_ents[4].attr_srid = -1;
	att_ents[4].attr_flags_mask = DMU_F_NDEFAULT;
	att_ents[4].attr_encflags = 0;
	att_ents[4].attr_encwid = 0;

	dmucb->dmu_part_def = NULL;
	dmucb->dmu_ppchar_array.data_address = NULL;
	dmucb->dmu_ppchar_array.data_in_size = 0;
	dmucb->dmu_partno = 0;
	dmucb->dmu_nphys_parts = 0;
	dmucb->dmu_enc_flags = 0;
	dmucb->dmu_enc_flags2 = 0;

 	status = dmu_create(dmucb);
	if (status)
	{
	    *dberr = dmucb->error;
	    break;
	}

	/*
	** Now modify the table to hash on the logical key
	*/
	dmucb->dmu_location.data_address = 0;
	dmucb->dmu_location.data_in_size = 0;

    	char_entry[0].char_id = DMU_STRUCTURE;
    	char_entry[0].char_value = perm_etab_struct;
	char_entry[1].char_id = DMU_PAGE_SIZE;
	char_entry[1].char_value = create_pagesize;
	char_entry[2].char_id = DMU_EXTEND;
	char_entry[2].char_value = DMPE_DEF_TBL_EXTEND;
	ind = 2;

    	switch (perm_etab_struct) {
            case DB_HASH_STORE :
		char_entry[++ind].char_id = DMU_MINPAGES;
		char_entry[ind].char_value = DMPE_DEF_TBL_SIZE;
		break;
            case DB_BTRE_STORE :
            case DB_ISAM_STORE :
		break;
        }

	dmucb->dmu_char_array.data_in_size = sizeof(DMU_CHAR_ENTRY)*(ind+1);
	dmucb->dmu_key_array.ptr_address = (PTR) keys;
	dmucb->dmu_key_array.ptr_size = sizeof(DMU_KEY_ENTRY);
	dmucb->dmu_key_array.ptr_in_count = DMPE_KEY_COUNT;

	if (perm_etab_struct != DB_HEAP_STORE)
	{
	    status = dmu_modify(dmucb);
	    if (status)
	    {
		*dberr = dmucb->error;
		break;
	    }
	}

	/*
	** If creating first etab, check if TCB_JON for base table
	** Otherwise check if TCB_JON for other etabs
	*/
	if (etab_tbl_status & TCB_JON)
	{
	    MEfill(sizeof(DMT_CB), 0, &jon_dmt_cb);
	    jon_dmt_cb.length = sizeof(DMT_CB);
	    jon_dmt_cb.type = DMT_TABLE_CB;
	    jon_dmt_cb.ascii_id = DMT_ASCII_ID;
	    jon_dmt_cb.dmt_db_id = (PTR)xcb->xcb_odcb_ptr;
	    jon_dmt_cb.dmt_tran_id = (PTR)xcb;
	    jon_dmt_cb.dmt_mustlock = FALSE;
	    jon_dmt_cb.dmt_char_array.data_in_size = sizeof(DMT_CHAR_ENTRY);
	    jon_dmt_cb.dmt_char_array.data_address = (char *)char_entry;
	    jon_dmt_cb.dmt_id.db_tab_base = dmucb->dmu_tbl_id.db_tab_base;
	    jon_dmt_cb.dmt_id.db_tab_index = 0;
	    char_entry[0].char_id = DMT_C_JOURNALED;
	    char_entry[0].char_value = DMT_C_ON;
	    status = dmt_alter(&jon_dmt_cb);
	    if (status)
	    {
		*dberr = jon_dmt_cb.error;
		break;
	    }
	}

	/* Insert record into iiextended_relation */
	MEfill(sizeof(DMR_CB), 0, (PTR) &dmrcb);
	dmrcb.length = sizeof(DMR_CB);
	dmrcb.type = DMR_RECORD_CB;
	dmrcb.ascii_id = DMR_ASCII_ID;
	dmrcb.dmr_access_id = dmtcb.dmt_record_access_id;
	etab_record->etab_base = rel_record->reltid.db_tab_base;
	etab_record->etab_extension = dmucb->dmu_tbl_id.db_tab_base;
	if (perm_etab_struct == DB_HEAP_STORE)
	    etab_record->etab_type = DMP_LO_HEAP;
	else
	    etab_record->etab_type = DMP_LO_ETAB;
	etab_record->etab_att_id = attr_id;
	MEfill(sizeof(etab_record->etab_filler), '\0', &etab_record->etab_filler);

	dmrcb.dmr_data.data_address = (char *) etab_record;
	dmrcb.dmr_data.data_in_size = sizeof(DMP_ETAB_CATALOG);

	status = dmr_put(&dmrcb);
	if (status)
	{
	    *dberr = dmrcb.error;
	    break;
	}

	break;
    }

    /* Close iiextended_relation if we opened it */
    if (etl_opened)
    {
	dmtcb.dmt_flags_mask = 0;
	local_status = dmt_close(&dmtcb);
	if (local_status)
	{
	    uleFormat(&dmtcb.error, 0, NULL , ULE_LOG,
			NULL, (char *) NULL, 0L, (i4 *) NULL, &local_error, 0);
	    status = local_status;
	}
    }

    if (DB_FAILURE_MACRO(status))
    {
	if ((dberr->err_code != E_DM0065_USER_INTR) &&
            (dberr->err_code != E_DM016B_LOCK_INTR_FA))
	{
	    uleFormat(dberr, 0, NULL , ULE_LOG,
			NULL, (char *) NULL, 0L, (i4 *) NULL, &local_error, 0);
	    SETDBERR(dberr, 0, E_DM9B07_DMPE_ADD_EXTENSION);
	}
    }
    if (misc_cb)
	(VOID) dm0m_deallocate((DM_OBJECT **) &misc_cb);
    return(status);
}

/*
** {
** Name: dmpe_temp	- Create temporary peripheral table
**
** Description:
**      This routine creates a temporary peripheral table.  This is performed
**	just as a regular add extension, with the following exceptions.  First,
**	the table created is a dmf temporary table (dmt_create_temp);  as such,
**	it will disappear at the end of the session (if not sooner).
**	Second, no entries are made the in iiextended_relation for the
**	temporary table.
**
**	Temporary peripheral tables are those used by the system to store large
**	objects which are components of queries.  They are not associated with
**	any tables.  They are necessary if we wish to allow the use of queries
**	of the form
**
**	    insert into <table> (...<large column>...)
**		    values (...<large object value>);
**
**	At the time the insert statement arrives, SCF has no notion of where
**	the object is going to belong, so it has no place to put it.
**	(And that in turn is because the embedded compilers and libq
**	conspire to send a query's data before the query itself!)
**	dmpe_temp's fill this need.
**
**	dmpe_temp is also used to create "permanent" etabs for session
**	temporary tables.  Because etabs for session temps are themselves
**	temporary, it's better to do the work here instead of trying to
**	trick out the regular etab creater.  The way we tell what we're
**	doing is by looking at pop_temporary.  If pop_temporary is set
**	to "permanent", we're creating a session temp etab.
**
**	Anonymous temp etabs are flagged as such.  Ultimately this flag
**	appears in the XCCB for the temp table.  The temp etab is given
**	session lifetime so that it appears on the ODCB temp xccb list.
**	A specific cleanup routine call can find all such flagged
**	temp etabs and delete them.  (dmpe_free_temps, called directly
**	or indirectly via ADF and the dmpe-call external call machinery.)
**
**	The reason for session lifetime, rather than transaction lifetime,
**	is mostly because of autocommit.  The sequencer likes to start and
**	end worker transactions, seemingly at random.  That's why there
**	is an explicit cleanup flag and routine call, so that anonymous
**	temps can be removed at query end (and not sit around for
**	session end.)
**
**	Pseudo-permanent session temp etabs are not flagged as holding
**	temps, but they ARE placed on the base table TCB ET list so that
**	all threads of the session can see them.  Because they are pseudo-
**	permanent, they are not to be dropped until the owning session
**	temp is dropped.
**
** Inputs:
**      pop_cb                          The peripheral operation control block
**	    .pop_coupon                      pointer to coupon to fill
**	    .pop_temporary		ADP_POP_PERMANENT for "perm" etab
**					for session temp table;
**					ADP_POP_TEMPORARY for anonymous temp
**					etab
**
** Outputs:
**      pop_cb.pop_coupon               Filled with the coupon for the temporary
**					table.
**	new_temp_etab			Set to new (temporary) table base-ID
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-Jan-1990 (fred)
**          Created.
**	16-oct-1992 (rickh)
**	    Initialize default ids for attributes.
**	24-nov-92 (rickh)
**	    Recast default ids as table ids.  Most excellent code hygeine.
**      23-Mar-1993 (fred)
**          Remove reference to pop_tab_id.  The field was removed
**          from the pop_cb since it was only set, never used.  Not
**          needed, and complicated the splitting of <dbms.h>.
**      20-Oct-1993 (fred)
**          Added support for working with session temporary tables.
**          If the blob is being added to a session temporary table,
**          make sure that the underlying table is also temporary of
**          appropriate lifetime.
**       5-Nov-1993 (fred)
**          Added support for keeping track of the short lived
**          temporary tables within DMF.  This is used so that these
**          tables can be destroyed when they have outlived their
**          usefulness.
**	 1-Feb-1996 (kch)
**	    Save the creation data for session temporary tables as well.
**	    This will allow specific session temp extension tables in II_WORK
**	    to be destroyed when a 'drop table session.temp' is issued.
**	    This change fixes bug 72752.
**	06-mar-1996 (stial01 for bryanp)
**	    When creating a table to hold a blob, choose a sufficiently large
**          page size.
**	31-Jul-1996 (kch)
**	    Set dup_dmt_cb->s_reserved to session_temp so that sess temp
**	    objects are not freed erroneously in dmpe_free_temps() (eg. after
**	    an insert), but are correctly freed when a 'drop sess.temp' is
**	    issued. This change fixes bug 78030.
**      24-Mar-1998 (thaju02)
**          Bug #87880 - inserting or copying blobs into a temp table chews
**          up cpu, locks and file descriptors.
**      29-Jun-98 (thaju02)
**          Regression bug: with autocommit on, temporary tables created
**          during blob insertion, are not being destroyed at statement
**          commital, but are being held until session termination.
**          Regression due to fix for bug 87880. (B91469)
**	12-aug-1998 (somsa01)
**	    We also need to set up the proper table structure of the
**	    global temporary extension table.  (Bug #92435)
**	27-aug-1998 (somsa01)
**	    Refined the check for a global temporary extension table when
**	    modifying it to its proper table structure.
**	28-oct-1998 (somsa01)
**	    In the case of Global Temporary Extension Tables, make sure that
**	    they are placed on the sessions lock list, not the transaction
**	    lock list.  (Bug #94059)
**	01-feb-1999 (somsa01)
**	    Set DMT_LO_LONG_TEMP if ADP_POP_LONG_TEMP is set. This will
**	    signify that the temporary table will last for more than 1
**	    transaction (internal or external), yet is not a session-wide
**	    temporary table.
**	2-Mar-2004 (schka24)
**	    Clean out partitioning stuff in dmu cb.
**	10-May-2004 (schka24)
**	    List of temps for cleanup has to live in the parent SCB,
**	    lock inserts into the list.
**	21-Jun-2004 (schka24)
**	    Get rid of separate cleanup list entirely, pass create flag for
**	    xccb instead.  Fix up premature dummy-loop end, I probably
**	    messed it up with above edit.
**	13-dec-04 (inkdo01)
**	    Init attr_collID.
**	29-Sept-09 (troal01)
**		Init attr_geomtype and attr_srid.
[@history_template@]...
*/
static DB_STATUS
dmpe_temp( ADP_POP_CB         *pop_cb, i4 att_id, u_i4 *new_temp_etab)
{
    DB_STATUS           status;
    DMPE_PCB            *pcb = (DMPE_PCB *) pop_cb->pop_user_arg;
    i4			i;
    i4		err_code, ind = 0;
    DMT_CB		*dmtcb;
    DB_LOC_NAME		loc_name;
    DMT_CB  	    	*dup_dmt_cb;
    DML_SCB         	*parent_scb;
    i4                 session_temp = 0;
    DMT_CHAR_ENTRY	characteristics[3];
    DMP_ETAB_CATALOG    etab_record;
    DMP_ET_LIST         *list;
    DMP_TCB             *tcb;
    DMU_CB		dmucb;
    DMU_CHAR_ENTRY	char_entry[5];

    /*
    ** Number of attributes in an extended table:
    */

#define             DMPE_ATT_COUNT      5

    DMF_ATTR_ENTRY	*atts[DMPE_ATT_COUNT];
    DMF_ATTR_ENTRY	att_ents[DMPE_ATT_COUNT];
    DMU_KEY_ENTRY	*keys[DMPE_KEY_COUNT];
    DMU_KEY_ENTRY	key_ents[DMPE_KEY_COUNT];

    CLRDBERR(&pop_cb->pop_error);

    do
    {
	dmtcb = pcb->pcb_dmtcb;

	dmtcb->type = DMU_UTILITY_CB;
	dmtcb->length = sizeof(DMT_TABLE_CB);

	/* DMT_LO_TEMP asks for: etab default page size; a different
	** internal name saying DMPE temp; the etab flag in relstat2;
	** locking against the session locklist;  and session lifetime.
	** If this is a holding table, not a perm etab, also set the
	** DMT_LO_HOLDING_TEMP flag so that the table XCCB is flagged
	** as a blob holding temp.
	**
	** Note, no need to set "session-temp", we don't need the drop-
	** on-rollback stuff because the base table will drive it.
	*/
	dmtcb->dmt_flags_mask = DMT_DBMS_REQUEST | DMT_LO_TEMP;
	if (pop_cb->pop_temporary != ADP_POP_PERMANENT)
	    dmtcb->dmt_flags_mask |= DMT_LO_HOLDING_TEMP;

	MEmove(22, (PTR) "<DMPE_TEMPORARY_TABLE>", ' ',
		sizeof(dmtcb->dmt_table),
		(PTR) dmtcb->dmt_table.db_tab_name);

	status = dmpe_qdata(pop_cb, dmtcb);
	if (status)
	    break;

	MEmove(8, "$default", ' ',
	    sizeof(DB_LOC_NAME), (PTR) loc_name.db_loc_name);

	dmtcb->dmt_location.data_address = (PTR) &loc_name;
	dmtcb->dmt_location.data_in_size = sizeof(loc_name);

	dmtcb->dmt_key_array.ptr_address = 0;
	dmtcb->dmt_key_array.ptr_size = 0;
	dmtcb->dmt_key_array.ptr_in_count = 0;

	characteristics[0].char_id = DMT_C_PAGE_SIZE;
	characteristics[0].char_value = dmf_svcb->svcb_etab_tmpsz;

	/* allow duplicates */
	characteristics[1].char_id = DMT_C_DUPLICATES;
	characteristics[1].char_value = DMT_C_ON;

	dmtcb->dmt_char_array.data_address = (PTR)characteristics;
	dmtcb->dmt_char_array.data_in_size = 2 * sizeof(DMT_CHAR_ENTRY);

	dmtcb->dmt_attr_array.ptr_in_count = DMPE_ATT_COUNT;
	dmtcb->dmt_attr_array.ptr_size = sizeof(DMF_ATTR_ENTRY);
	dmtcb->dmt_attr_array.ptr_address = (PTR) atts;

	for (i = 0; i < DMPE_ATT_COUNT; i++)
	{
	    atts[i] = &att_ents[i];
	    att_ents[i].attr_defaultTuple = (DB_IIDEFAULT *) 0;
	    SET_CANON_DEF_ID( att_ents[i].attr_defaultID, DB_DEF_NOT_DEFAULT );
	}

	for (i = 0; i < DMPE_KEY_COUNT; i++)
	{
	    keys[i] = &key_ents[i];
	}


	MEmove(7, "per_key", ' ',
		sizeof(att_ents[0].attr_name),
	       (PTR) &att_ents[0].attr_name);
	att_ents[0].attr_type = DB_TABKEY_TYPE;
	att_ents[0].attr_size = sizeof(DB_TAB_LOGKEY_INTERNAL);
	att_ents[0].attr_precision = 0;
	att_ents[0].attr_collID = 0;
	att_ents[0].attr_geomtype = -1;
	att_ents[0].attr_srid = -1;
	att_ents[0].attr_flags_mask = 0;
	att_ents[0].attr_encflags = 0;
	att_ents[0].attr_encwid = 0;

	STRUCT_ASSIGN_MACRO(att_ents[0].attr_name, key_ents[0].key_attr_name);
	key_ents[0].key_order = DMU_ASCENDING;

	MEmove(12, "per_segment0", ' ',
		sizeof(att_ents[1].attr_name),
	       (PTR) &att_ents[1].attr_name);
	att_ents[1].attr_type = DB_INT_TYPE;
	att_ents[1].attr_size = 4;
	att_ents[1].attr_precision = 0;
	att_ents[1].attr_collID = 0;
	att_ents[1].attr_geomtype = -1;
	att_ents[1].attr_srid = -1;
	att_ents[1].attr_flags_mask = 0;
	att_ents[1].attr_encflags = 0;
	att_ents[1].attr_encwid = 0;

	STRUCT_ASSIGN_MACRO(att_ents[1].attr_name, key_ents[1].key_attr_name);
	key_ents[1].key_order = DMU_ASCENDING;

	MEmove(12, "per_segment1", ' ',
		sizeof(att_ents[2].attr_name),
	       (PTR) &att_ents[2].attr_name);
	att_ents[2].attr_type = DB_INT_TYPE;
	att_ents[2].attr_size = 4;
	att_ents[2].attr_precision = 0;
	att_ents[2].attr_collID = 0;
	att_ents[2].attr_geomtype = -1;
	att_ents[2].attr_srid = -1;
	att_ents[2].attr_flags_mask = 0;
	att_ents[2].attr_encflags = 0;
	att_ents[2].attr_encwid = 0;

	STRUCT_ASSIGN_MACRO(att_ents[2].attr_name, key_ents[2].key_attr_name);
	key_ents[2].key_order = DMU_ASCENDING;

	MEmove(8, "per_next", ' ',
		sizeof(att_ents[3].attr_name),
	       (PTR) &att_ents[3].attr_name);
	att_ents[3].attr_type = DB_INT_TYPE;
	att_ents[3].attr_size = 4;
	att_ents[3].attr_precision = 0;
	att_ents[3].attr_collID = 0;
	att_ents[3].attr_geomtype = -1;
	att_ents[3].attr_srid = -1;
	att_ents[3].attr_flags_mask = 0;
	att_ents[3].attr_encflags = 0;
	att_ents[3].attr_encwid = 0;

	MEmove(9, "per_value", ' ',
		sizeof(att_ents[4].attr_name),
	       (PTR) &att_ents[4].attr_name);
	att_ents[4].attr_type = pop_cb->pop_underdv->db_datatype;
	att_ents[4].attr_size = pop_cb->pop_underdv->db_length;
	att_ents[4].attr_precision = pop_cb->pop_underdv->db_prec;
	att_ents[4].attr_collID = 0;
	att_ents[4].attr_geomtype = -1;
	att_ents[4].attr_srid = -1;
	att_ents[4].attr_flags_mask = 0;
	att_ents[4].attr_encflags = 0;
	att_ents[4].attr_encwid = 0;

	status = dmt_create_temp(dmtcb);
	if (status)
	{
	    pop_cb->pop_error = dmtcb->error;
	    break;
	}

	if (pop_cb->pop_temporary == ADP_POP_PERMANENT)
	{
	    /*
	    ** Etab for session temporary table.
	    ** Now modify the table to the appropriate structure.
	    */
	    dmucb.length = sizeof(DMU_CB);
	    dmucb.type = DMU_UTILITY_CB;
	    dmucb.dmu_flags_mask = DMU_INTERNAL_REQ;
	    STRUCT_ASSIGN_MACRO(dmtcb->dmt_owner, dmucb.dmu_owner);
	    dmucb.dmu_db_id = dmtcb->dmt_db_id;
	    dmucb.dmu_tran_id = dmtcb->dmt_tran_id;
	    STRUCT_ASSIGN_MACRO(dmtcb->dmt_id, dmucb.dmu_tbl_id);
	    dmucb.dmu_location.data_address = 0;
	    dmucb.dmu_location.data_in_size = 0;
	    dmucb.dmu_part_def = NULL;
	    dmucb.dmu_partno = 0;
	    dmucb.dmu_nphys_parts = 0;
	    dmucb.dmu_ppchar_array.data_address = 0;
	    dmucb.dmu_ppchar_array.data_in_size = 0;
	    dmucb.dmu_enc_flags = 0;
	    dmucb.dmu_enc_flags2 = 0;

	    char_entry[0].char_id = DMU_STRUCTURE;
	    char_entry[0].char_value = dmf_svcb->svcb_blob_etab_struct;
	    char_entry[1].char_id = DMU_PAGE_SIZE;
	    char_entry[1].char_value = dmf_svcb->svcb_etab_tmpsz;
	    char_entry[2].char_id = DMU_EXTEND;
	    char_entry[2].char_value = DMPE_DEF_TBL_EXTEND;
	    char_entry[3].char_id = DMU_TEMP_TABLE;
	    char_entry[3].char_value = DMU_C_ON;
	    ind = 3;

	    /* Note svcb_blob_etab_struct == DB_HEAP_STORE is invalid */
	    switch (dmf_svcb->svcb_blob_etab_struct)
	    {
		case DB_HASH_STORE :
		    char_entry[++ind].char_id = DMU_MINPAGES;
		    char_entry[ind].char_value = DMPE_DEF_TBL_SIZE;
		    break;
		case DB_BTRE_STORE :
		case DB_ISAM_STORE :
		    break;
	    }

	    dmucb.dmu_char_array.data_address = (char *) char_entry;
	    dmucb.dmu_char_array.data_in_size = sizeof(DMU_CHAR_ENTRY)*(ind+1);
	    dmucb.dmu_key_array.ptr_address = (PTR) keys;
	    dmucb.dmu_key_array.ptr_size = sizeof(DMU_KEY_ENTRY);
	    dmucb.dmu_key_array.ptr_in_count = DMPE_KEY_COUNT;

	    status = dmu_modify(&dmucb);
	    if (status)
	    {
		STRUCT_ASSIGN_MACRO(dmucb.error, pop_cb->pop_error);
		break;
	    }
	}

	dmtcb->dmt_attr_array.ptr_in_count = 0;
	dmtcb->dmt_attr_array.ptr_size = 0;
	dmtcb->dmt_attr_array.ptr_address = 0;
	dmtcb->dmt_char_array.data_address = 0;
	dmtcb->dmt_char_array.data_in_size = 0;

	*new_temp_etab = dmtcb->dmt_id.db_tab_base;

	if ( pop_cb->pop_temporary == ADP_POP_PERMANENT )
	{
	    /* For gtt etab, more work:
	    ** Light a hint flag in the parent SCB that says that some
	    ** GTT has etabs (tells gtt drop to check);
	    ** Hook this new etab onto the base table ET list, since there's
	    ** no etab catalog for gtt's and no other way for the et list to
	    ** get filled in.
	    */
	    /* *Important* This is the parent thread SCB if we're in a
	    ** factotum child.
	    */
	    parent_scb = ((DML_ODCB *) dmtcb->dmt_db_id)->odcb_scb_ptr;
	    parent_scb->scb_global_lo = TRUE;
	    tcb = pcb->pcb_tcb;
	    etab_record.etab_base = tcb->tcb_rel.reltid.db_tab_base;
	    etab_record.etab_extension = dmtcb->dmt_id.db_tab_base;
	    etab_record.etab_type = DMP_LO_ETAB;
	    etab_record.etab_att_id = att_id;
	    MEfill(sizeof(etab_record.etab_filler), '\0', &etab_record.etab_filler);

	    status = dm0m_allocate( sizeof(DMP_ET_LIST),
		(i4) 0,
		(i4) ETL_CB,
		(i4) ETL_ASCII_ID,
		(char *) tcb,
		(DM_OBJECT **) &list,
		&pop_cb->pop_error);
	    if ( status == E_DB_OK )
	    {
		dm0s_mlock(&tcb->tcb_et_mutex);
		list->etl_status = 0;
		STRUCT_ASSIGN_MACRO(etab_record, list->etl_etab);
		list->etl_next = tcb->tcb_et_list;
		list->etl_prev = tcb->tcb_et_list->etl_prev;
		tcb->tcb_et_list->etl_prev = list;
		list->etl_prev->etl_next = list;
		dm0s_munlock(&tcb->tcb_et_mutex);
	    }
	} /* If perm etab */
	/* Drop out, success or failure */
    } while (0);

    if (DB_FAILURE_MACRO(status))
    {
	if ((pop_cb->pop_error.err_code != E_DM0065_USER_INTR) &&
            (pop_cb->pop_error.err_code != E_DM016B_LOCK_INTR_FA))
	{
	    uleFormat(&pop_cb->pop_error, 0, NULL , ULE_LOG,
			NULL, (char *) NULL, 0L, (i4 *) NULL,
			&err_code, 0);
	    SETDBERR(&pop_cb->pop_error, 0, E_DM9B09_DMPE_TEMP);
	}
    }
    return(status);

}

/*
** {
** Name: dmpe_qdata	- Obtain Query data
**
** Description:
**	This routine obtains information about the current query.  This
**	information includes the database and transaction id's.  Usually, this
**	information is contained in the open rcb found in the peripheral object
**	coupon, but for temporary objects, there is no rcb around.  In these
**	cases, we must ask SCF for our session CB pointer, then obtain the open
**	database control block address (the database id) and the transaction
**	control block address (the transaction id) from there.
**
** Inputs:
**      pop_cb                          Peripheral Operations Control block for
**					this operation
**      dmtcb                           Table control block for which this
**					operation is desired.  This may be
**					different from the pcb_dmtcb field
**					obtainable from the pop_cb->pop_user_arg
**					field.
**
** Outputs:
**      pop_cb->pop_error               Filled with error iff appropriate.
**      dmtcb->dmt_db_id                The database id.
**      dmtcb->dmt_tran_id              The transaction id.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    Not really a side effect, but a warning, sort of.  This routine
**	    assumes that any session has at most one database open, and one
**	    transaction open.  If these rules-of-the-game are changed, work will
**	    have to be invested here to determine the correct database and/or
**	    transaction identifiers.
**
** History:
**      15-feb-1990 (fred)
**          Created.
**      14-Apr-1994 (fred)
**          Set the dmt_sequence number from the xcb_seq_no field,
**          which has been squirrelled away by previous calls.  Using
**          this varying value (rather than the preivous zero) will
**          allow us to be a bit more discriminating when dumping
**          temporary tables at query termination.
**	08-Jan-2001 (jenjo02)
**	   Use macro to find *DML_SCB instead of SCU_INFORMATION.
[@history_template@]...
*/
static DB_STATUS
dmpe_qdata(ADP_POP_CB	*pop_cb ,
            DMT_CB		*dmtcb )

{
    DML_SCB		*scb;
    CS_SID		sid;

    CSget_sid(&sid);
    scb = GET_DML_SCB(sid);

    if (scb->scb_x_ref_count == 1)
    {
	dmtcb->dmt_tran_id = (PTR) scb->scb_x_next;
	dmtcb->dmt_db_id = (PTR) ((DML_XCB *) dmtcb->dmt_tran_id)->xcb_odcb_ptr;
	return(E_DB_OK);
    }
    else
    {
	SETDBERR(&pop_cb->pop_error, 0, E_DM9B08_DMPE_NEED_TRAN);
	return(E_DB_ERROR);
    }
}

/*{
** Name: dmpe_nextchain_fixup	- Set 'next' pointer for the previous record.
**
** Description:
**      This routine correctly sets the `next' pointer for the previous record
**	in a peripheral table.  Peripheral objects are permitted to span
**	peripheral table extensions.  The may be necessary when a particular
**	relation extension either fills up or the device(s) on which it sits
**	runs out of space.
**
**	When this occurs, dmpe_put() will first locate a place for the new
**	record.  Having done so, if dmpe_put() determines that the new record
**	did not go into the place the previous record expected, it will call
**	dmpe_nextchain_fixup() to fixup the previous record.
**
**	The fixup is not complicated.  The extension relation in which the
**	previous record is found is opened (using the pcb_table_previous table
**	id), the old record read and updated, and the previous table is closed.
**
**	The calling of this routine is expected to be `unlikely'.  In most
**	cases, the entire peripheral object will fit in a single table.
**
**	Nextchain-fixup is not used when bulk-loading etabs.  We can't
**	get or update a row already in the sorter, so bulk-load uses a
**	write behind method instead.  See dmpe-put.
**
**
** Inputs:
**      pop_cb                          Peripheral Operations Control Block
**	    pop_coupon			Coupon specifying the object being
**					altered.
**	    pop_temporary		Indicator that the object is a
**					temporary.
**	    pop_user_arg		Address of PCB in use.
**		pcb->pcb_record->prd_segment0,
**		pcb->pcb_record->prd_segment1	- segment to update
**		pcb->pcb_tcb			- tcb of table for coupon
**		pcb->pcb_table_previous		- table in which record
**						  to be updated is found
**		pcb->pcb_dmtcb->dmt_id		- table id of new next record
**
** Outputs:
**      pop_cb->pop_error               Filled as necessary.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none.
**
** Side Effects:
**	    none
**
** History:
**      15-Feb-1990 (fred)
**          Created.
**	31-jan-1994 (bryanp) B58483, B58484, B58485
**	    Check dmpe_qdata return code.
**	    Fix "if (!status)" line -- should have been "if (status)".
**	    Set pop_error if dmpe_close fails during cleanup.
**	28-oct-1998 (somsa01)
**	    In the case of Global Temporary Extension Tables, make sure that
**	    they are placed on the sessions lock list, not the transaction
**	    lock list.  (Bug #94059)
**	 3-mar-1998 (hayke02)
**	    We now test for TCB_SESSION_TEMP in relstat2 rather than relstat.
**	8-Aug-2005 (schka24)
**	    While looking for session-temp flag usage, noticed that this
**	    routine was setting it in the wrong dmt_cb -- fix.
**       25-Apr-2007 (kibro01) b115996
**          Ensure the misc_data pointer in a MISC_CB is set correctly and
**          that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**          since that's what we're actually using (the type is MISC_CB)
[@history_template@]...
*/
static DB_STATUS
dmpe_nextchain_fixup(ADP_POP_CB     *pop_cb )

{
    DMPE_PCB            *pcb = (DMPE_PCB *) pop_cb->pop_user_arg;
    DMP_MISC		*misc_cb = NULL;
    DMT_CB		*dmt_cb;
    DMR_CB		*dmr_cb;
    DMPE_RECORD		*record;
    DB_STATUS		status;
    DMR_ATTR_ENTRY	*attr_entry[DMPE_KEY_COUNT];
    DMR_ATTR_ENTRY	attr[DMPE_KEY_COUNT];
    DMPE_COUPON		*input = DMPE_CPN_FROM_DBV_MACRO(pop_cb->pop_coupon);
    i4			open = 0;
    i4			repositioned = 0;
    i4                  i;
    i4		err_code;
    u_i4		segment0;
    u_i4		segment1;
    DMP_RCB		*r;
    i4			error;

    CLRDBERR(&pop_cb->pop_error);

    do /* dummy loop */
    {
	segment0 = pcb->pcb_record->prd_segment0;
	segment1 = pcb->pcb_record->prd_segment1 - 1;
	if (segment1 > pcb->pcb_record->prd_segment1)
	{
	    segment0 = pcb->pcb_record->prd_segment0 - 1;
	    segment1 = 0;
	}

	status = dm0m_allocate(	(sizeof(DMP_MISC)
				    +
				 sizeof(DMT_CB)
				    +
				 sizeof(DMR_CB)
				    +
				 sizeof(DMPE_RECORD)
			        ),
			(i4) 0,
			(i4) MISC_CB,
			(i4) MISC_ASCII_ID,
			(char *) pop_cb,
			(DM_OBJECT **) &misc_cb,
			&pop_cb->pop_error);
	if (status)
	    break;
	dmt_cb = (DMT_CB *) ((char *) misc_cb + sizeof(DMP_MISC));
	misc_cb->misc_data = (char*)dmt_cb;
	MEfill(sizeof(DMT_CB), 0, (PTR) dmt_cb);
	dmt_cb->length = sizeof(DMT_CB);
	dmt_cb->type = DMT_TABLE_CB;
	dmt_cb->ascii_id = DMT_ASCII_ID;
	STRUCT_ASSIGN_MACRO(pcb->pcb_table_previous, dmt_cb->dmt_id);
	dmt_cb->dmt_db_id = pcb->pcb_db_id;
	dmt_cb->dmt_tran_id = (PTR) pcb->pcb_xcb;
	dmt_cb->dmt_flags_mask = DMT_DBMS_REQUEST;

	dmt_cb->dmt_update_mode = DMT_U_DIRECT;
	dmt_cb->dmt_access_mode = DMT_A_WRITE;
	dmt_cb->dmt_key_array.ptr_address = 0;
	dmt_cb->dmt_key_array.ptr_size = 0;
	dmt_cb->dmt_key_array.ptr_in_count = 0;

	dmt_cb->dmt_char_array.data_address = 0;
	dmt_cb->dmt_char_array.data_in_size = 0;

	status = etab_open(dmt_cb, pop_cb);

	if (DB_FAILURE_MACRO(status))
	{
	    STRUCT_ASSIGN_MACRO(dmt_cb->error, pop_cb->pop_error);
	    break;
	}
	open = 1;
	dmr_cb = (DMR_CB *) ((char *) dmt_cb + sizeof(DMT_CB));
	MEfill(sizeof(DMR_CB), 0, (PTR) dmr_cb);
	dmr_cb->length = sizeof(DMR_CB);
	dmr_cb->type = DMR_RECORD_CB;
	dmr_cb->ascii_id = DMR_ASCII_ID;

	dmr_cb->dmr_access_id = dmt_cb->dmt_record_access_id;
	r = (DMP_RCB *)dmt_cb->dmt_record_access_id;

	dmr_cb->dmr_position_type = DMR_QUAL;

	for (i = 0; i < DMPE_KEY_COUNT; i++)
	{
	    attr_entry[i] = &attr[i];
	}

	attr[0].attr_operator = DMR_OP_EQ;
	attr[0].attr_number = 1;
	attr[0].attr_value = (PTR) &input->cpn_log_key;

	attr[1].attr_operator = DMR_OP_EQ;
	attr[1].attr_number = 2;
	attr[1].attr_value = (PTR) &segment0;

	attr[2].attr_operator = DMR_OP_EQ;
	attr[2].attr_number = 3;
	attr[2].attr_value = (PTR) &segment1;

	dmr_cb->dmr_attr_desc.ptr_in_count = DMPE_KEY_COUNT;
	dmr_cb->dmr_attr_desc.ptr_size = sizeof(DMR_ATTR_ENTRY);
	dmr_cb->dmr_attr_desc.ptr_address = (PTR) attr_entry;
	status = dmr_position(dmr_cb);

	if (status)
	{
	    if (dmr_cb->error.err_code == E_DM0074_NOT_POSITIONED ||
	        dmr_cb->error.err_code == E_DM008E_ERROR_POSITIONING)
	    {
		dmr_cb->dmr_position_type = DMR_ALL;
		repositioned++;
		status = dmr_position(dmr_cb);
	    }
	    if (status)
	    {
		STRUCT_ASSIGN_MACRO(dmr_cb->error,
					    pop_cb->pop_error);
		break;
	    }
	}

	dmr_cb->dmr_flags_mask = DMR_NEXT;
	dmr_cb->dmr_data.data_address = (char *)
		    ((char *) dmr_cb + sizeof(DMR_CB));
	record = (DMPE_RECORD *) dmr_cb->dmr_data.data_address;
	dmr_cb->dmr_data.data_in_size = sizeof(DMPE_RECORD);
	do
	{
	    status = dmr_get(dmr_cb);
	    if (status)
	    {
		if ((!repositioned++)
			&& (dmr_cb->error.err_code == E_DM0055_NONEXT))
		{
		    /*
		    **	In this case, then we restart our scan at the begining
		    **	of this set.
		    */

		    dmr_cb->dmr_flags_mask = DMR_REPOSITION;
		    status = dmr_position(dmr_cb);
		    if (status == E_DB_OK)
		    {
			dmr_cb->dmr_flags_mask = DMR_NEXT;
			continue;
		    }
		}
		else if (dmr_cb->error.err_code == E_DM0055_NONEXT)
		{
		    status = E_DB_ERROR;
		    SETDBERR(&pop_cb->pop_error, 0, E_AD7001_ADP_NONEXT);
		    break;
		}
		STRUCT_ASSIGN_MACRO(dmr_cb->error, pop_cb->pop_error);
		break;
	    }
	} while (   (record->prd_segment1 != segment1)
		||  (record->prd_segment0 != segment0)
		||  (record->prd_log_key.tlk_high_id !=
					    input->cpn_log_key.tlk_high_id)
		||  (record->prd_log_key.tlk_low_id !=
					    input->cpn_log_key.tlk_low_id));

	if (status == E_DB_OK)
	{
	    /*
	    **	Now that we have the record, we need to correctly update it.
	    **	If there is a next segment (i.e. pop_segment is non-zero), then
	    **	the next segment is the table id of the currently open table.
	    **	Otherwise, it is zero (i.e. if there is no next).
	    */

	    if (pop_cb->pop_segment)
	    {
		record->prd_r_next = pcb->pcb_dmtcb->dmt_id.db_tab_base;
	    }
	    else
	    {
		record->prd_r_next = 0;
	    }
	    dmr_cb->dmr_flags_mask = DMR_CURRENT_POS;
	    status = dmr_replace(dmr_cb);
	    if (status)
	    {
		STRUCT_ASSIGN_MACRO(dmr_cb->error, pop_cb->pop_error);
		break;
	    }

	}
    } while (0);  /* dummy loop */
    if (misc_cb)
    {
	if (open)
	{
	    dmt_cb->dmt_flags_mask = 0;
	    err_code = dmt_close(dmt_cb);
	    if (err_code)
	    {
		if (status == E_DB_OK)
		{
		    status = err_code;
		    STRUCT_ASSIGN_MACRO(dmt_cb->error, pop_cb->pop_error);
		}
		uleFormat(&dmt_cb->error, 0, NULL , ULE_LOG,
			NULL, (char *) NULL, 0L, (i4 *) NULL,
			&err_code, 0);
	    }
	}
	(VOID) dm0m_deallocate((DM_OBJECT **) &misc_cb);
    }
    if (DB_FAILURE_MACRO(status))
    {
	if ((pop_cb->pop_error.err_code != E_DM0065_USER_INTR) &&
            (pop_cb->pop_error.err_code != E_DM016B_LOCK_INTR_FA))
	{
	    uleFormat(&pop_cb->pop_error, 0 , NULL , ULE_LOG,
			NULL, (char *) NULL, 0L, (i4 *) NULL,
			&err_code, 0);
	    SETDBERR(&pop_cb->pop_error, 0, E_DM9B09_DMPE_TEMP);
	}
    }
    return(status);
}

/*{
** Name: dmpe_cat_scan	- Find extended table catalog row for this POP
**
** Description:
**      This routine scans the extended table catalog (ii_extended_relation)
**	to find any extended relations which belong to the table involved.  This
**	can be found by looking at the coupon for this peripheral operation
**	(POP).
**
**	This routine will open the catalog, and scan it based on the key (which
**	is the table id of the base table).
**
** Inputs:
**      pop_cb				The Peripheral OPerations control
**					block
**
** Outputs:
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      08-Jan-1990 (fred)
**          Created.
**	31-jan-1994 (bryanp) B58486
**	    Check dmpe_close status during cleanup.
[@history_template@]...
*/
static DB_STATUS
dmpe_cat_scan(ADP_POP_CB	*pop_cb ,
               DB_TAB_ID	*parent_id ,
               DMP_ETAB_CATALOG	*etab_record )

{
    DB_STATUS           status;
    DB_STATUS		internal_status;
    DMPE_PCB            *pcb = (DMPE_PCB *) pop_cb->pop_user_arg;
    DMR_ATTR_ENTRY	*array_of_one;
    DMR_ATTR_ENTRY	attr;
    i4		err_code;

    CLRDBERR(&pop_cb->pop_error);

    for (;;)
    {
	if (!pcb->pcb_dmrcb->dmr_access_id)
	{
	    pcb->pcb_dmtcb->dmt_lock_mode = DMT_S;
	    pcb->pcb_dmtcb->dmt_update_mode = DMT_U_DIRECT;
	    pcb->pcb_dmtcb->dmt_access_mode = DMT_A_WRITE;
	    pcb->pcb_dmtcb->dmt_id.db_tab_base = DM_B_ETAB_TAB_ID;
	    pcb->pcb_dmtcb->dmt_id.db_tab_index = DM_I_ETAB_TAB_ID;
	    status = dmt_open(pcb->pcb_dmtcb);
	    if (status)
	    {
		pop_cb->pop_error = pcb->pcb_dmtcb->error;
		break;
	    }
	    pcb->pcb_dmrcb->dmr_access_id =
			    pcb->pcb_dmtcb->dmt_record_access_id;

	    array_of_one = &attr;
	    pcb->pcb_dmrcb->dmr_position_type = DMR_QUAL;
	    pcb->pcb_dmrcb->dmr_attr_desc.ptr_address = (PTR) &array_of_one;
	    pcb->pcb_dmrcb->dmr_attr_desc.ptr_in_count = DMP_ET_KEY_COUNT;
	    attr.attr_operator = DMR_OP_EQ;
	    attr.attr_number = DM_1_ETAB_KEY;
	    attr.attr_value = (char *)
		    &parent_id->db_tab_base;

	    status = dmr_position(pcb->pcb_dmrcb);
	    if (status)
	    {
		if (pcb->pcb_dmrcb->error.err_code == E_DM0074_NOT_POSITIONED
		 || pcb->pcb_dmrcb->error.err_code == E_DM008E_ERROR_POSITIONING)
		{
		    pcb->pcb_dmrcb->dmr_position_type = DMR_ALL;
		    status = dmr_position(pcb->pcb_dmrcb);
		}
		if (status)
		{
		    pop_cb->pop_error = pcb->pcb_dmrcb->error;
		    break;
		}
	    }
	}
	pcb->pcb_dmrcb->dmr_data.data_address = (char *) etab_record;
	pcb->pcb_dmrcb->dmr_data.data_in_size = sizeof(*etab_record);
	pcb->pcb_dmrcb->dmr_flags_mask = DMR_NEXT;

	do
	{
	    status = dmr_get(pcb->pcb_dmrcb);
	    if (status)
	    {
		pop_cb->pop_error = pcb->pcb_dmrcb->error;
		pcb->pcb_dmtcb->dmt_flags_mask = 0;
		internal_status = dmt_close(pcb->pcb_dmtcb);
		if (internal_status)
		    uleFormat( &pcb->pcb_dmtcb->error, 0,
			    (CL_ERR_DESC *)NULL, ULE_LOG,
			    NULL, (char *) NULL, 0L, (i4 *) NULL,
			    &err_code, 0);
		pcb->pcb_dmtcb->dmt_record_access_id = 0;
		pcb->pcb_dmrcb->dmr_access_id = 0;
		break;
	    }
	} while (status == E_DB_OK &&
		    etab_record->etab_base != parent_id->db_tab_base);
	break;
    }
    if (status)
    {
	if (pcb->pcb_dmtcb->dmt_record_access_id)
	{
	    pcb->pcb_dmtcb->dmt_flags_mask = 0;
	    internal_status = dmt_close(pcb->pcb_dmtcb);
	    if (internal_status)
		uleFormat( &pcb->pcb_dmtcb->error, 0,
			(CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *) NULL, 0L, (i4 *) NULL,
			&err_code, 0);
	    pcb->pcb_dmtcb->dmt_record_access_id = 0;
	    pcb->pcb_dmrcb->dmr_access_id = 0;
	}
	if (	(pop_cb->pop_error.err_code != E_DM0054_NONEXISTENT_TABLE)
	    &&	(pop_cb->pop_error.err_code != E_DM0065_USER_INTR)
	    &&	(pop_cb->pop_error.err_code != E_DM0055_NONEXT)
            &&  (pop_cb->pop_error.err_code != E_DM016B_LOCK_INTR_FA))
	{
	    uleFormat(&pop_cb->pop_error, 0, NULL , ULE_LOG,
			NULL, (char *) NULL, 0L, (i4 *) NULL,
			&err_code, 0);
	    SETDBERR(&pop_cb->pop_error, 0, E_DM9B06_DMPE_BAD_CAT_SCAN);
	}
	else if ((pop_cb->pop_error.err_code != E_DM0065_USER_INTR) &&
                 (pop_cb->pop_error.err_code != E_DM016B_LOCK_INTR_FA))
	{
	    /*
	    **	This is "OK" because the table will be created later.
	    **	This situation happens once per DB, since the
	    **	iiextended_relation catalog is created only if/when it is
	    **	needed.  However, if the case is interrupt, we need to
	    **  reflect that back to the caller.
	    */

	    status = E_DB_WARN;
	    SETDBERR(&pop_cb->pop_error, 0, E_DM0055_NONEXT);
	}
    }
    return(status);
}

/*
** {
** Name: dmpe_free_temps -- free any large object temps associated
**                          with this session.
**
** Description:
**      This routine scans the list of session-lifetime temporaries
**      associated with this session, destroying any that are flagged
**	as large-object holding temporaries.
**
**      It is expected that this routine will be called when it is safe
**      to do so -- meaning that there are no blobs "in flight".
**
**	Session temp table etabs are not considered "temporary", and
**	are not flagged as such.  Therefore they are not deleted
**	by this routine.  Only anonymous holding temp tables are deleted.
**
** Inputs:
**	odcb			Open database ID
**
** Outputs:
**	err_code		error code if any errors
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    Session XCCB list pruned of any deleted tmp tables.
**
** History:
**       5-Nov-1993 (fred)
**          Created.
**      15-Apr-1994 (fred)
**          Altered to allow for deleting temporary tables for
**          specific queries.
**	1-Feb-1996 (kch)
**	    Do not deallocate or unlink DMT_CBs if we are destroying session
**	    temp extension tables (pop_cb->pop_s_reserved == 1, set in
**	    destroy_temporary_table()). This change fixes bug 72752.
**	25-Jul-1996 (kch)
**	    We now test for pop_cb->pop_s_reserved != TRUE rather than
**	    !pop_cb->pop_s_reserved, because pop_s_reserved might be
**	    unintialized.
**	31-Jul-1996 (kch)
**	    Added new argument sess_temp_flag to indicate whether sess temp
**	    objects should be freed (ie. a 'drop session.temp'). Added logic to
**	    to detect a sess temp DMT_CB (dmt_cb->s_reserved is TRUE, set in
**	    dmpe_temp()). This change fixes bug 78030.
**	28-Oct-1996 (somsa01)
**	    If the error E_DM0054_NONEXISTENT_TABLE is returned from
**	    dmt_delete_temp(), this is OK, since the blob temp table
**	    was probably destroyed by some other "natural" cleanup
**	    action.
**	14-nov-1998 (somsa01)
**	    If sess_temp_flag is true, then it may have been another
**	    transaction which created this temp table in this session. Thus,
**	    pass the xcb for the current transaction to dmt_delete_temp().
**	    (Bug #79203)
**	07-dec-1998 (somsa01)
**	    Re-worked the fix for bug #79203. Update the dmt_cb's of
**	    concern with the current transaction id in dmudestroy.c rather
**	    than here. The previous fix needed to modify adp.h, which
**	    compromised spatial objects (unless a change was made to iiadd.h,
**	    which we are hesitant to make).  (Bug #79203)
**	10-May-2004 (schka24)
**	    Temp list is hooked to parent SCB for parallel query.
**	    Only free temps when the parent thread asks for it.
**	    Pass odcb instead of pop cb.
**	21-Jun-2004 (schka24)
**	    Rather than maintain a separate blob cleanup list, which we
**	    weren't keeping in sync with the main XCCB temp-cleanup list,
**	    run this routine off of the session XCCB list and simply look
**	    for a flag that says this temp table is a blob holding temp.
**
[@history_template@]...
*/
DB_STATUS
dmpe_free_temps(DML_ODCB *odcb, DB_ERROR *dberr)
{
    CS_SID		sid;
    DB_STATUS		status = E_DB_OK;
    DML_SCB		*parent_scb;
    DML_SCB		*thread_scb;
    DML_XCCB		*xccb, *next_xccb;
    DM_OBJECT           *object;
    i4			error;
    i4			local_error;

    CLRDBERR(dberr);

    CSget_sid(&sid);
    thread_scb = GET_DML_SCB(sid);

    /* This is the PARENT scb pointer */
    parent_scb = odcb->odcb_scb_ptr;

    /* Only free temps from the parent thread.  The rationale here
    ** is that (at least in some situations) the parent thread is
    ** the controller thread and knows when it's safe to free up temps.
    */
    if (parent_scb != thread_scb)
	return (E_DB_OK);

    /* Don't actually need a transaction to delete session temps,
    ** remove the check.
    */

    /* Even though we're the parent, protect against any factotums */
    dm0s_mlock(&odcb->odcb_cq_mutex);
    for (xccb = odcb->odcb_cq_next;
	 xccb != (DML_XCCB *) &odcb->odcb_cq_next;
	 xccb = next_xccb)
    {
	/* Save next in case we deallocate this xccb */
	next_xccb = xccb->xccb_q_next;

	if (xccb->xccb_operation == XCCB_T_DESTROY
	  && (xccb->xccb_blob_temp_flags & BLOB_HOLDING_TEMP)
	  && (xccb->xccb_blob_temp_flags & BLOB_HAS_LOCATOR) == 0)
	{
	    /* Found one to get rid of.  Take it off the queue so that
	    ** no other thread tries to get rid of it via any sort of
	    ** cleanup.
	    */
	    xccb->xccb_q_prev->xccb_q_next = xccb->xccb_q_next;
	    xccb->xccb_q_next->xccb_q_prev = xccb->xccb_q_prev;
	    dm0s_munlock(&odcb->odcb_cq_mutex);
	    /* Delete, but not with dmt_delete_temp since it expects
	    ** a valid transaction which we may not have.
	    */
	    status = dmt_destroy_temp(parent_scb, xccb, dberr);
	    if (status != E_DB_OK && dberr->err_code != E_DM0054_NONEXISTENT_TABLE)
	    {
		/* If it didn't work, log the problem */
		uleFormat(dberr, 0, NULL, ULE_LOG, NULL,
				NULL, 0, NULL, &local_error, 0);
		SETDBERR(dberr, 0, E_DM9B09_DMPE_TEMP);
	    }
	    /* Works or not, get rid of the XCCB we unlinked */
	    dm0m_deallocate((DM_OBJECT **) &xccb);
	    dm0s_mlock(&odcb->odcb_cq_mutex);
	}
    }
    dm0s_munlock(&odcb->odcb_cq_mutex);

    return(status);
}

/*
** Name: dmpe_start_load	- Initiate the load of an extension table.
**
** Description:
**	This routine (which is based upon the start_load() routine in
**	dmrload.c) sets up the appropriate variables for a call to
**	dm2r_load() with the DM2R_L_BEGIN flag.
**
**	Etabs being loaded are never heaps, so we can skip a couple
**	start-load cases that only have to do with heaps.
**
** Inputs:
**	rcb				The RCB of the extension table.
**
** Outputs:
**	error				Set if an error occurred in here.
**
**	Returns:
**	    STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	none
**
** History:
**	27-jul-1998 (somsa01)
**	    Created from start_load() in dmrload.c .
**	03-aug-1998 (somsa01)
**	    Removed recbuf stuff; it is not used.
**	03-aug-1998 (somsa01)
**	    Put part of last removal back.
**	13-Apr-2010 (kschendel) SIR 123485
**	    Do "noparallel" sorts on etab loads, mostly because there might
**	    be a bunch of 'em and we don't want to flood the OS with a
**	    zillion sort child threads.  Force 100% fillfactors, there is
**	    little to no benefit in less.  Start hash etabs out at something
**	    vaguely resembling a useful size (these are LOBs, after all).
*/
static STATUS
dmpe_start_load(
DMP_RCB		*rcb,
DB_ERROR	*error)
{
    DMP_TCB	*tcb = rcb->rcb_tcb_ptr;
    i4		flag;
    DB_STATUS	status = E_DB_OK;
    i4		row_count;
    i4		row_estimate;
    DM2R_BUILD_TBL_INFO  build_tbl_info;
    i4		min_pages, max_pages;
    i4		local_error;

    /* Get the tuple buffer from the rcb */
    if ( rcb->rcb_tupbuf == NULL )
	rcb->rcb_tupbuf = dm0m_tballoc((tcb->tcb_rel.relwid +
					     tcb->tcb_data_rac.worstcase_expansion));
    if (rcb->rcb_tupbuf == NULL)
    {
	SETDBERR(error, 0, E_DM923D_DM0M_NOMORE);
	return (E_DB_ERROR);
    }

    /* Start the load process. */

    flag = DM2R_NOPARALLEL;

    /*
    ** Pass on estimated number of rows (zero for now).
    */
    row_estimate = 0;

    /*
    ** Setup information on how to build the table. These are set
    ** from the table's TCB.
    */
    build_tbl_info.dm2r_fillfactor   = 100;
    build_tbl_info.dm2r_nonleaffill  = 100;
    build_tbl_info.dm2r_leaffill     = 100;
    build_tbl_info.dm2r_hash_buckets = 0;
    build_tbl_info.dm2r_extend       = tcb->tcb_rel.relextend;
    build_tbl_info.dm2r_allocation   = tcb->tcb_rel.relallocation;

    /*
    ** If a HASH table determine the number of HASH buckets
    ** which are required.
    ** Without an estimated row count, calling dm2u_calc_hash_buckets is
    ** a waste of time.  For a bulk-load, one expects a certain volume,
    ** so use a special hard-coded value if the etab doesn't have a
    ** usefully large looking minpages already.
    ** It would be nice to use the base table estimate, if there is one,
    ** but I don't know any way to get at it.
    */
    if ( tcb->tcb_rel.relspec == TCB_HASH )
    {
	build_tbl_info.dm2r_hash_buckets = tcb->tcb_rel.relmin;
	if (build_tbl_info.dm2r_hash_buckets < 512)
	    build_tbl_info.dm2r_hash_buckets = 4096;	/* 32 meg at 8K page */
    }

    row_count = 0;

    status = dm2r_load( rcb, tcb, DM2R_L_BEGIN, flag,
	                &row_count, (DM_MDATA *)NULL, row_estimate,
			&build_tbl_info, error);
    return (status);
}

static DB_STATUS
dmpe_get_etab(DML_XCB                 *xcb,
		DMP_RELATION          *rel,
		u_i4     *etab_extension,
		bool     create_flag,
		DB_ERROR *dberr)
{
    DB_STATUS           status;
    DB_STATUS		internal_status;
    DMR_ATTR_ENTRY	*array_of_one;
    DMR_ATTR_ENTRY	attr;
    DMT_CB              dmtcb;
    DMR_CB              dmrcb;
    DMP_ETAB_CATALOG    etab_record;
    i4			local_error;

    /*
    ** At this point we should just get all etab info into the tcb,
    ** but we'll just get the first etab record for now
    */
    do
    {
	MEfill(sizeof(DMT_CB), 0, (PTR) &dmtcb);
	dmtcb.length = sizeof(DMT_TABLE_CB);
	dmtcb.type = DMT_TABLE_CB;
	dmtcb.ascii_id = DMT_ASCII_ID;
	dmtcb.dmt_db_id = (PTR) xcb->xcb_odcb_ptr;
	dmtcb.dmt_tran_id = (PTR) xcb;
	dmtcb.dmt_sequence = 0;  /* only need sequence# for deferred cursor */
	dmtcb.dmt_record_access_id = 0;
	dmtcb.dmt_lock_mode = DMT_S;
	dmtcb.dmt_update_mode = DMT_U_DIRECT;
	dmtcb.dmt_access_mode = DMT_A_WRITE;
	dmtcb.dmt_id.db_tab_base = DM_B_ETAB_TAB_ID;
	dmtcb.dmt_id.db_tab_index = DM_I_ETAB_TAB_ID;
	status = dmt_open(&dmtcb);
	if (status)
	{
	    *dberr = dmtcb.error;
	    break;
	}

	MEfill(sizeof(DMR_CB), 0, (PTR) &dmrcb);
	dmrcb.length = sizeof(DMR_CB);
	dmrcb.type = DMR_RECORD_CB;
	dmrcb.ascii_id = DMR_ASCII_ID;
	array_of_one = &attr;
	dmrcb.dmr_access_id = dmtcb.dmt_record_access_id;
	dmrcb.dmr_position_type = DMR_QUAL;
	dmrcb.dmr_attr_desc.ptr_address = (PTR) &array_of_one;
	dmrcb.dmr_attr_desc.ptr_in_count = DMP_ET_KEY_COUNT;
	dmrcb.dmr_attr_desc.ptr_size = sizeof(DMR_ATTR_ENTRY);
	attr.attr_operator = DMR_OP_EQ;
	attr.attr_number = DM_1_ETAB_KEY;
	attr.attr_value = (char *)&rel->reltid.db_tab_base;

	status = dmr_position(&dmrcb);
	if (status)
	{
	    *dberr = dmrcb.error;
	    break;
	}

	dmrcb.dmr_data.data_address = (char *) &etab_record;
	dmrcb.dmr_data.data_in_size = sizeof(etab_record);
	dmrcb.dmr_flags_mask = DMR_NEXT;

	status = dmr_get(&dmrcb);
	if (status)
	{
	    *dberr = dmrcb.error;
	    break;
	}
	*etab_extension = etab_record.etab_extension;
    } while (0);

    if (dmtcb.dmt_record_access_id)
    {
	internal_status = dmt_close(&dmtcb);
	if ( internal_status && status == E_DB_OK )
	{
	    *dberr = dmtcb.error;
	    status = E_DB_ERROR;
	}
    }

    if (status && !create_flag)
    {
	uleFormat( dberr, E_DM9B06_DMPE_BAD_CAT_SCAN,
		(CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *) NULL, 0L, (i4 *) NULL,
		&local_error, 0);
    }
    return(status);
}


/*
**{
** Name: dmpe_buffered_put	- Do a buffered put of blob segments.
**
** Description:
**      This routine performs a buffered put of blob segments when
**      the etab has page type V2, table structure BTREE, and compression NONE.
**      The intention is to store blob segments on new disassociated btree
**      data pages in which case we don't need to do page allocations within
**      mini transactions.
**
**      When using nojournaling, NOBLOBLOGGING protocols,
**      all segments must be inserted onto new disassociated data pages,
**      to support rollback of blobs when nojournaling, NOBLOBLOGGING.
**
**      Currently if the blob has only one segment we are doing normal puts
**      which may use the associated data page, and therefore must be logged.
**      If this is not the desired behavior, we can change the code to
**      do buffered puts for single segment blobs as well.
**
**      The bulk of the work for buffered puts is in dm1b_bulk_put,
**      which will get called when rcb_bulk_misc has been initialized.
**
** Inputs:
**      pop_cb                          ADP_POP_CB Ptr for controlling the
**					peripheral operation
**      dmr_cb				Pointer to etab DMR_CB for put
**
** Outputs:
**      dmr_cb
**		.error.err_code		Error number if status != E_DB_OK
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      02-jan-2004 (stial01)
**	27-Jul-2009 (thaju02) B122383
**       iietab data filesize grows very large; For fairly small blobs
**	 as pagesize increases disassociated data pages have more
**	 unused space.
**	6-Apr-2010 (kschendel) SIR 123485
**	    short-term is OK for rcb-bulk-misc.
**
*/
static DB_STATUS
dmpe_buffered_put(
ADP_POP_CB	*pop_cb,
DMR_CB		*dmr_cb)
{
    DMPE_RECORD		dmpe_seg_buf;
    DMPE_RECORD		*put_rec;
    DMPE_RECORD		*first_rec;
    DMPE_RECORD		*fix_rec;
    bool		last_seg;
    DMP_RCB		*r = (DMP_RCB *)dmr_cb->dmr_access_id;
    DMP_TCB		*t = r->rcb_tcb_ptr;
    i4			error;
    DMPP_PAGE		*page;
    DM_TID		put_tid;
    DM_TID		first_tid;
    DM_TID		fix_tid;
    DB_STATUS		status = E_DB_OK;
    DM_PAGENO		pageno;
    DMR_CB		dmr_bulk_cb;
    i4			record_size = sizeof(DMPE_RECORD);
    bool		done;

    for (done = FALSE;!done;)
    {
	/*
	** Bulk put implemented only if etab is V2, BTREE and compression NONE.
	*/
	if (t->tcb_rel.relpgtype == DM_COMPAT_PGTYPE
		|| t->tcb_rel.relspec != TCB_BTREE
		|| t->tcb_rel.relcomptype != TCB_C_NONE
		|| ((pop_cb->pop_continuation & ADP_C_BEGIN_MASK) &&
			(pop_cb->pop_continuation & ADP_C_END_MASK)))
	{
	    /*
	    ** Make sure rcb_bulk_misc is not set as it is a condition
	    ** for nobloblogging which cannot be done for single segment
	    ** blobs unless we force them onto disassociated btree data pages
	    */
	    if (r->rcb_bulk_misc)
	    {
		dm0m_deallocate((DM_OBJECT **)&r->rcb_bulk_misc);
	    }
	    status = dmr_put(dmr_cb);
	    break;
	}

	put_rec = (DMPE_RECORD *)dmr_cb->dmr_data.data_address;
	if (put_rec->prd_r_next == 0)
	    last_seg = TRUE;
	else
	    last_seg = FALSE;

	/* First segment should need to allocate memory */
	if (!r->rcb_bulk_misc)
	{
	    i4		size;

	    size = sizeof(DMP_MISC) + t->tcb_rel.relpgsize;
	    /*
	    ** The allocation for rcb_bulk_misc should be LONGTERM (same as RCB)
	    ** (Explicitly request DM0M_LONGTERM because the default
	    ** for MISC_CB is short term 
	    */
	    status = dm0m_allocate( size, DM0M_ZERO | DM0M_LONGTERM,
			    (i4) MISC_CB,
			    (i4) MISC_ASCII_ID, (char *) 0,
			    (DM_OBJECT **) &r->rcb_bulk_misc, &dmr_cb->error);
	    if ( status != E_DB_OK )
	    {
		uleFormat(&dmr_cb->error, 0, NULL, ULE_LOG , NULL,
			(char * )NULL, 0L, (i4 *)NULL, &error, 0);
	        SETDBERR(&dmr_cb->error, 0, E_DM9B02_PUT_DMPE_ERROR);
		break;
	    }
	    r->rcb_bulk_misc->misc_data = (char *)(r->rcb_bulk_misc + 1);
	    r->rcb_bulk_cnt = 0;
	}

	page = (DMPP_PAGE *)(r->rcb_bulk_misc + 1);

	/*
	** Insert segment onto page buffer
	**
	** If rcb etab page is full, insert leaf entries for each
	** or if this is the last segment, insert leaf entries for each
	*/
	if (r->rcb_bulk_cnt == 0)
	{
	    pageno = 0; /* for now */

	    (*t->tcb_acc_plv->dmpp_format)(t->tcb_rel.relpgtype,
				    t->tcb_rel.relpgsize,
				    page,
				    pageno,
				    (DMPP_DATA | DMPP_MODIFY),
				    DM1C_ZERO_FILL);

	}

	put_tid.tid_tid.tid_page = 0;
	status = (*t->tcb_acc_plv->dmpp_allocate)
		(page, r, (char *)put_rec, sizeof(DMPE_RECORD), &put_tid, 0);

	if (status == E_DB_INFO)
	{
	    /*
	    ** This page is full
	    ** Write the page and generate leaf entries
	    */
#ifdef xDEBUG
	    TRdisplay("bulk put segments %d\n", r->rcb_bulk_cnt);
#endif

	    /* Always pass pointer to first record to be inserted */
	    first_rec = &dmpe_seg_buf;
	    first_tid.tid_tid.tid_page = 0;
	    first_tid.tid_tid.tid_line = 0;

	    status = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype,
		t->tcb_rel.relpgsize, page, &first_tid,
		&record_size, (PTR *)&first_rec, NULL, NULL, NULL, (DMPP_SEG_HDR *)0);
	    if (status != E_DB_OK)
	    {
		SETDBERR(&dmr_cb->error, 0, E_DMF011_DM1R_BAD_TID);
		break;
	    }

	    /*
	    ** We may have done dmpe_add_extension and dmpe_nextchain_fixup
	    ** after we buffered these rows onto this page
	    ** If so, fix prd_r_next for all rows on this page
	    ** if (prd_r_next == 0) this is last segment indicator
	    ** DON'T update prd_r_next in the last segment
	    */
	    if (first_rec->prd_r_next != t->tcb_rel.reltid.db_tab_base)
	    {
		fix_tid.tid_tid.tid_page = 0;
		for (fix_tid.tid_tid.tid_line = 0;
			fix_tid.tid_tid.tid_line < r->rcb_bulk_cnt;
				fix_tid.tid_tid.tid_line++)
		{
		    status = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype,
			t->tcb_rel.relpgsize, page, &fix_tid,
			&record_size, (PTR *)&fix_rec, NULL, NULL, NULL, (DMPP_SEG_HDR *)0);

		    if (fix_rec->prd_r_next != 0)
		    {
			fix_rec->prd_r_next = t->tcb_rel.reltid.db_tab_base;

			(*t->tcb_acc_plv->dmpp_put)(t->tcb_rel.relpgtype,
			t->tcb_rel.relpgsize, page, DM1C_DIRECT,
			&r->rcb_tran_id, r->rcb_slog_id_id, &fix_tid,
			sizeof(DMPE_RECORD), (char *)fix_rec,
			t->tcb_rel.relversion, (DMPP_SEG_HDR *)0);
		    }
		}

		/* re-get first record */
		status = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype,
		    t->tcb_rel.relpgsize, page, &first_tid,
		    &record_size, (PTR *)&first_rec, NULL, NULL, NULL,
		    (DMPP_SEG_HDR *)0);
	    }

	    STRUCT_ASSIGN_MACRO(*dmr_cb, dmr_bulk_cb);
	    dmr_bulk_cb.dmr_data.data_address = (PTR)first_rec;

	    status = dmr_put(&dmr_bulk_cb);
	    if (status != E_DB_OK)
	    {
		dmr_cb->error = dmr_bulk_cb.error;

		/*
		** We haven't allocated tid for current row yet
		** We may need to dmpe_add_extension and dmpe_nextchain_fixup
		** and then retry
		*/
		break;
	    }

	    r->rcb_bulk_cnt = 0;
	    continue;
	}

	/* assumes relcomptype != TCB_C_NONE */
	/* Pass pointer to 1st record inserted on rcb_bulk_misc data page */
	(*t->tcb_acc_plv->dmpp_put)(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		page, DM1C_DIRECT, &r->rcb_tran_id, r->rcb_slog_id_id, &put_tid,
		sizeof(DMPE_RECORD), (char *)put_rec,
		t->tcb_rel.relversion, (DMPP_SEG_HDR *)0);
	r->rcb_bulk_cnt++;

	if (last_seg)
	{
	    /*
	    ** Last segment
	    ** Write the data page and generate leaf entries
	    */

#ifdef xDEBUG
	    TRdisplay("bulk put segments %d\n", r->rcb_bulk_cnt);
#endif

	    /* Always pass pointer to first record to be inserted */
	    first_rec = &dmpe_seg_buf;
	    first_tid.tid_tid.tid_page = 0;
	    first_tid.tid_tid.tid_line = 0;

	    status = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype,
		t->tcb_rel.relpgsize, page, &first_tid,
		&record_size, (PTR *)&first_rec, NULL, NULL, NULL, (DMPP_SEG_HDR *)0);
	    if (status != E_DB_OK)
	    {
		SETDBERR(&dmr_cb->error, 0, E_DMF011_DM1R_BAD_TID);
		break;
	    }

	    /*
	    ** We may have done dmpe_add_extension and dmpe_nextchain_fixup
	    ** after we buffered these rows onto this page
	    ** If so, fix prd_r_next for all rows on this page
	    */
	    if (first_rec->prd_r_next != t->tcb_rel.reltid.db_tab_base)
	    {
		fix_tid.tid_tid.tid_page = 0;
		for (fix_tid.tid_tid.tid_line = 0;
			fix_tid.tid_tid.tid_line < r->rcb_bulk_cnt;
				fix_tid.tid_tid.tid_line++)
		{
		    status = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype,
			t->tcb_rel.relpgsize, page, &fix_tid,
			&record_size, (PTR *)&fix_rec, NULL, NULL, NULL,
			(DMPP_SEG_HDR *)0);

		    if (fix_rec->prd_r_next != 0)
		    {
			fix_rec->prd_r_next = t->tcb_rel.reltid.db_tab_base;

			(*t->tcb_acc_plv->dmpp_put)(t->tcb_rel.relpgtype,
			    t->tcb_rel.relpgsize, page, DM1C_DIRECT,
			    &r->rcb_tran_id, r->rcb_slog_id_id, &fix_tid,
			    sizeof(DMPE_RECORD), (char *)fix_rec,
			    t->tcb_rel.relversion, (DMPP_SEG_HDR *)0);
		    }
		}

		/* re-get first record */
		status = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype,
		    t->tcb_rel.relpgsize, page, &first_tid,
		    &record_size, (PTR *)&first_rec, NULL, NULL, NULL,
		    (DMPP_SEG_HDR *)0);
	    }

            if (r->rcb_bulk_cnt < t->tcb_tperpage)
            {
                i4              save_bulk_cnt = r->rcb_bulk_cnt;

                /* not enough buffered rows to fill a page
                ** resort to put rec the old-way
                */
#ifdef xDEBUG
		TRdisplay("put segments %d with normal put\n", r->rcb_bulk_cnt);
#endif

                fix_tid.tid_tid.tid_page = fix_tid.tid_tid.tid_line = 0;
                for (dmr_cb->dmr_data.data_address = (PTR)first_rec; ;
		     dmr_cb->dmr_data.data_address = (PTR)fix_rec)
                {
                    r->rcb_bulk_cnt = 0;
                    status = dmr_put(dmr_cb);
                    if (status != E_DB_OK)
                        break;

                    if (++fix_tid.tid_tid.tid_line == save_bulk_cnt)
                        break;

                    status = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype,
                        t->tcb_rel.relpgsize, page, &fix_tid,
                        &record_size, (PTR *)&fix_rec, NULL, NULL, NULL,
                        (DMPP_SEG_HDR *)0);
                    if (status != E_DB_OK)
                        break;
                }
                if (status != E_DB_OK)
                    break;
            }
            else
            {
	    	STRUCT_ASSIGN_MACRO(*dmr_cb, dmr_bulk_cb);
	    	dmr_bulk_cb.dmr_data.data_address = (PTR)first_rec;

	    	status = dmr_put(&dmr_bulk_cb);
	    	if (status != E_DB_OK)
	    	{
		    dmr_cb->error.err_code = dmr_bulk_cb.error.err_code;

		    /*
		    ** Delete newly allocated row
		    ** We may need to dmpe_add_extension and dmpe_nextchain_fixup
		    ** And then retry
		    */
		    (*t->tcb_acc_plv->dmpp_delete)(t->tcb_rel.relpgtype,
		    	t->tcb_rel.relpgsize, page, DM1C_DIRECT, TRUE,
		    	&r->rcb_tran_id, r->rcb_slog_id_id, &put_tid,
			sizeof(DMPE_RECORD));
		    r->rcb_bulk_cnt--;
		    break;
		}
	    }
	}

	done = TRUE;
	break; /* done, so break */
    }

    if (status != E_DB_OK)
    {
	uleFormat(&dmr_cb->error, 0, NULL, ULE_LOG , NULL, (char * )NULL, 0L,
		(i4 *)NULL, &error, 0);
    }
    return (status);
}


/*
** Name: dmpe_journaling - This routine updates the journaling status
**				for all extension tables for a given table.
**
** Description:
**      This routine updates the journaling status for all extension tables
**      for a given table.
**	It is based on the table id of the parent table.
**
**	Based upon this id, the routine scans the iiextended_relation catalog.
**	Any table whose parent matches the table id passed in has its
**      journaling status changed.
**
**	Journaling doesn't apply to session temps, so this is easier than
**	(say) dmpe-modify.
**
** Inputs:
**      blob_journaling			DMT_C_ON or DMT_C_OFF
**      xcb				The transaction id
**      base_tab_id			base table id
**
** Outputs:
**	err_code			Set if an error occurred in here.
**
**	Returns:
**	    STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	none
**
** History:
**      02-jan-2004 (stial01)
**	    Created.
**	15-Apr-2010 (kschendel) SIR 123485
**	    Don't need a fake coupon any more.
*/
DB_STATUS
dmpe_journaling(
i4		blob_journaling,
DML_XCB		*xcb,
DB_TAB_ID	*base_tab_id,
DB_ERROR	*dberr)
{
    DB_STATUS           status;
    ADP_POP_CB		pop_cb;
    DMPE_PCB		*pcb;
    DMP_ETAB_CATALOG	etab_record;
    DMU_CHAR_ENTRY	char_entry[1];
    i4			i;
    DMP_ET_LIST		*etlist_ptr = (DMP_ET_LIST *)0;
    DMT_CB		dmt_cb;
    i4		local_error;

    CLRDBERR(&pop_cb.pop_error);

    if (DMZ_SES_MACRO(11))
	dmd_petrace("DMPE_JOURNAL requested", 0, 0 , 0);

    MEfill(sizeof(DMT_CB), 0, (PTR) &dmt_cb);
    dmt_cb.length = sizeof(DMT_CB);
    dmt_cb.type = DMT_TABLE_CB;
    dmt_cb.ascii_id = DMT_ASCII_ID;
    dmt_cb.dmt_db_id = (PTR)xcb->xcb_odcb_ptr;
    dmt_cb.dmt_tran_id = (PTR)xcb;
    dmt_cb.dmt_mustlock = FALSE;
    dmt_cb.dmt_char_array.data_in_size = sizeof(DMT_CHAR_ENTRY);
    dmt_cb.dmt_char_array.data_address = (char *)char_entry;
    char_entry[0].char_id = DMT_C_JOURNALED;
    char_entry[0].char_value = blob_journaling; /* DMT_C_ON or DMT_C_OFF */

    pop_cb.pop_temporary = ADP_POP_TEMPORARY;
    pop_cb.pop_coupon = NULL;
    pop_cb.pop_info = NULL;

    status = dmpe_allocate(&pop_cb, &pcb);
    if (status)
    {
	*dberr = pop_cb.pop_error;
	return(status);
    }

    while (status == E_DB_OK)
    {
	status = dmpe_cat_scan(&pop_cb, base_tab_id, &etab_record);
	if (status == E_DB_OK)
	{
	    dmt_cb.dmt_id.db_tab_base = etab_record.etab_extension;
	    dmt_cb.dmt_id.db_tab_index = 0;

	    status = dmt_alter(&dmt_cb);
	    pop_cb.pop_error = dmt_cb.error;
	    *dberr = dmt_cb.error;
	}
    }
    if (status == E_DB_WARN && pop_cb.pop_error.err_code == E_DM0055_NONEXT)
    {
	status = E_DB_OK;
	CLRDBERR(&pop_cb.pop_error);
    }

    dmpe_deallocate(pcb);
    *dberr = pop_cb.pop_error;

    return(status);
}

/*
** Name: etab_open
**
** Description:
**	This routine does a dmt_open on an extension table, and marks
**	the RCB with the parent table's base-ID (if we know it).  This
**	is mainly a convenience for table close, which wants to close
**	dmpe-opened etabs if the parent table is being closed.
**	(typically etabs are closed by hand here in dmpe, but interrupts
**	or premature POP end might leave an etab open.)
**
**	We'll also figure out the extension table lock level.  If the original
**	POP CB passed along an open table reference, use that RCB to open
**	the etabs in a compatible manner.  Otherwise, use the hints in
**	the operation's query context (BQCB).
**	bqcb-table-lock -> S or X locking
**	bqcb-crib -> MVCC locking, possibly against a cursor CRIB.
**
**	etab_open is only used for GET and PUT operations, so there will
**	always be a valid query context (BQCB) unless the etab is a holding.
**	temp.
**
** Inputs:
**	dmtcb			DMT_CB describing etab to open
**	pop_cb			ADP_POP_CB for this blob operation
**
** Outputs:
**	Returns E_DB_xxx, error details in DMT_CB
**
** History:
**	12-May-2004 (schka24)
**	    Written.
**	24-Mar-2010 (jonj)
**	    Tell dmt_open to use CRIB in dmt_crib_ptr; dmt_open
**	    figures everything else out.
**	16-Apr-2010 (kschendel) SIR 123485
**	    Figure out the lock level and mvcc-ness from the query context
**	    (BQCB), which the caller will provide.
*/

static DB_STATUS
etab_open(DMT_CB *dmtcb, ADP_POP_CB *pop_cb)
{
    bool table_lock;
    DB_BLOB_WKSP *wksp;			/* Possible caller supplied info */
    DB_STATUS status;
    DMPE_BQCB *bqcb;			/* May be NULL if holding temp */
    DMPE_PCB *pcb;
    DMP_RCB *r;				/* Possible RCB from wksp */
    DMP_TCB *t;
    LG_CRIB *crib;
    i4 lockmode;

    pcb = (DMPE_PCB *) pop_cb->pop_user_arg;
    bqcb = pcb->pcb_bqcb;
    r = NULL;
    crib = NULL;
    table_lock = FALSE;
    wksp = (DB_BLOB_WKSP *) pop_cb->pop_info;
    if (wksp != NULL && wksp->flags & BLOBWKSP_ACCESSID)
    {
	/* Use lock modes compatible with base RCB */
	r = (DMP_RCB *) wksp->access_id;
	if (r->rcb_lk_type == RCB_K_CROW)
	    crib = r->rcb_crib_ptr;
	else if (r->rcb_lk_type == RCB_K_TABLE)
	    table_lock = TRUE;
    }
    else if (bqcb != NULL)
    {
	/* Use lock modes as remembered in the crib from the most
	** recent base table open.
	*/
	table_lock = bqcb->bqcb_table_lock;
	crib = bqcb->bqcb_crib;
    }

    dmtcb->dmt_crib_ptr = NULL;
    if (dmtcb->dmt_access_mode == DMT_A_WRITE)
    {
	/* Session temps always use table locking */
	if (dmtcb->dmt_id.db_tab_base < 0)
	{
	    dmtcb->dmt_flags_mask |= DMT_SESSION_TEMP;
	    lockmode = DMT_X;
	}
	else
	{
	    lockmode = DMT_IX;
	    if (table_lock)
		lockmode = DMT_X;
	    else if (crib != NULL)
	    {
		lockmode = DMT_MIX;	/* Kind-of unnecessary with cribptr */
		dmtcb->dmt_crib_ptr = (PTR) crib;
		dmtcb->dmt_flags_mask |= DMT_CRIBPTR;
	    }
	}
    }
    else
    {
	/* Session temps always use table locking */
	if (dmtcb->dmt_id.db_tab_base < 0)
	{
	    dmtcb->dmt_flags_mask |= DMT_SESSION_TEMP;
	    lockmode = DMT_S;
	}
	else
	{
	    lockmode = DMT_IS;
	    if (table_lock)
		lockmode = DMT_S;
	    else if (crib != NULL)
	    {
		lockmode = DMT_MIS;	/* Kind-of unnecessary with cribptr */
		dmtcb->dmt_crib_ptr = (PTR) crib;
		dmtcb->dmt_flags_mask |= DMT_CRIBPTR;
	    }
	}
    }
    dmtcb->dmt_lock_mode = lockmode;

    status = dmt_open(dmtcb);		/* First do the open */
    if (status != E_DB_OK)
	return (status);

    if (pcb->pcb_tcb != NULL)
    {
	((DMP_RCB *) (dmtcb->dmt_record_access_id))->rcb_et_parent_id =
		pcb->pcb_tcb->tcb_rel.reltid.db_tab_base;
    }

    return (E_DB_OK);
} /* etab_open */

/*
** Name: dmpe_load_etab_limit - Calculate max rows per etab for bulk-load
**
** Description:
**	When bulk-loading an etab, we need to keep track of when too
**	many rows have been sent to the sorter, because the sorter may
**	not tell us in time to switch the load to another etab.
**	This routine takes a whack at calculating the max number of
**	rows that can fit into one etab.
**
**	This is meant to be a conservative calculation.  Take off about
**	1% of the limit for slop and fmaps, etc.  (Even in the worst case,
**	the 2K page etab, we only need 500 or so fmaps to map 8 million
**	pages.)  If the etab is btree, account for leaf and index pages,
**	take them away from the total, and recalculate.  This procedure
**	may slightly under-estimate the number of rows, but that's better
**	than over-estimating.
**
**	Etab bulk-loads always run at 100% fillfactors, regardless of
**	etab definition, so don't worry about fillfactors.
**
** Inputs:
**	et		TCB pointer for the etab
**
** Outputs:
**	Returns # of rows that we estimate can fit into the etab.
**
** History:
**	19-Apr-2010 (kschendel) SIR 123485
**	    New.
*/

static u_i4
dmpe_load_etab_limit(DMP_TCB *et)
{
    u_i4 pagelimit, pages, rows;

    /* Take a whack at calculating max rows that can go into the
    ** table before it fills and we have to start another.
    ** This is meant to be a conservative calculation!
    ** Subtract about 1% for slop.  We don't bother calculating
    ** fmaps;  the 2K fmap (smallest) maps 16000 pages,
    ** needing 524 fmaps which is well within slop.
    */
    pages = pagelimit = DM1P_MAX_TABLE_SIZE - 80000;
    rows = pages * et->tcb_tperpage;
    if (et->tcb_rel.relspec == TCB_ISAM || et->tcb_rel.relspec == TCB_BTREE)
    {
	for (;;)
	{
	    u_i4 extra_pages, keys, p;

	    /* Calculate overhead pages, then take that away from
	    ** initial rows guess, until it all fits.  This does
	    ** not result in the max possible rows, but it's close
	    ** enough and won't get us into trouble.  I hope.
	    */
	    extra_pages = 0;
	    keys = rows;
	    if (et->tcb_rel.relspec == TCB_BTREE)
	    {
		/* Add double the number of calculated leaf pages, because
		** each new leaf starts a new data page as its associated
		** data page.
		*/
		p = (keys + et->tcb_kperleaf - 1) / et->tcb_kperleaf;
		extra_pages += p + p;
		keys = p;
	    }
	    do
	    {
		p = (keys + et->tcb_kperpage - 1) / et->tcb_kperpage;
		extra_pages += p;
		keys = p;
	    } while (keys > 1);
	    if (pages + extra_pages <= pagelimit)
		break;
	    rows = rows - (pages + extra_pages - pagelimit) * et->tcb_tperpage;
	    pages = (rows + et->tcb_tperpage - 1) / et->tcb_tperpage;
	}
    }
#ifdef xDEBUG
    /* Fake up lower limit for testing */
    if (DMZ_TBL_MACRO(11)
	rows = 50;
#endif

    return (rows);
} /* dmpe_load_etab_limit */

/*
** Name: dmpe_cpn_to_locator 	Return a locator for input coupon
**
** Description:
**      This routine creates a blob locator for the input coupon.
**      When the column is null do not create a locator.
**      A locator can never point to a null value.
**
** Inputs:
**	pop_cb				    ADP_POP_CB control block...
**	    pop_coupon			    Ptr to DB_DATA_VALUE containing the
**					    coupon to create a locator for.
**
** Outputs:
**	pop_cb				    ADP_POP_CB control block...
**	    pop_segment			    Ptr to DB_DATA_VALUE containing the
**					    locator.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      16-oct-2006 (stial01)
**	    created.
**	24-jun-2009 (gupsh01)
**	    Fix setting the locator values at constant offset of
**	    ADP_HDR_SIZE in the ADP_PERIPHERAL structure, irrespective
**	    of the platform. This was not happening for 64 bit platforms
**	    where per_value was aligned on ALIGN_RESTRICT boundary, and
**	    this causes problems for API / JDBC etc as it expects it to
**	    to be at the constant offset irrespective of the platform.
**	5-Apr-2010 (kschendel) SIR 123485
**	    Remove deprecated CSswitch call.
*/
static DB_STATUS
dmpe_cpn_to_locator(ADP_POP_CB	*pop_cb)
{
    DB_STATUS		status;
    i4			err_code;
    DB_STATUS		loc_stat;
    i4			loc_err;
    DML_SCB		*parent_scb, *thread_scb;
    DML_SCB		*scb;
    DML_XCB		*xcb = NULL;
    CS_SID		sid;
    ADP_PERIPHERAL	*cpn_p = (ADP_PERIPHERAL *)pop_cb->pop_coupon->db_data;
    DMPE_COUPON		*cpn = (DMPE_COUPON *)&cpn_p->per_value.val_coupon;

    ADP_PERIPHERAL	*loc_p = (ADP_PERIPHERAL *)pop_cb->pop_segment->db_data;
    ADP_LOCATOR		locator;
    DM_TID		loc_tid;
    DMR_CB              dmrcb;
    DMP_RCB		*loc_rcb = NULL;
    DMPE_LLOC_CXT	*lloc_cxt;
    DML_ODCB		*odcb;
    DML_XCCB		*xccb = NULL;
    bool		found;
    DB_ERROR		local_dberr;

    CLRDBERR(&pop_cb->pop_error);

    if (ADF_ISNULL_MACRO(pop_cb->pop_coupon))
    {
	/* Init output ADP_PERIPHERAL (ADP_P_LOCATOR) */
	loc_p->per_tag = ADP_P_LOCATOR;
	loc_p->per_length0 = cpn_p->per_length0;
	loc_p->per_length1 = cpn_p->per_length1;
        MEfill(sizeof(ADP_LOCATOR), 0,
		((char *)loc_p + ADP_HDR_SIZE));

	/* We do not create a locator for null columns */
	return(E_DB_OK);
    }

    /* Need to get context to anchor locator to this session */
    CSget_sid(&sid);
    thread_scb = GET_DML_SCB(sid);

    if (thread_scb->scb_x_ref_count > 0)
	xcb = thread_scb->scb_x_next;
    else
    {
	SETDBERR(&pop_cb->pop_error, 0, E_DM9B08_DMPE_NEED_TRAN);
	return (E_DB_ERROR);
    }

    /* Always create locator context off of parent scb */
    /* FIX ME do we need to mutex updates to LLOC_CXT */
    odcb =  xcb->xcb_odcb_ptr;
    parent_scb = odcb->odcb_scb_ptr;
    scb = parent_scb;

    /*
    ** If we just created a locator for a coupon that references a temp etab,
    ** that temp etab should NOT be dropped at the end of the current query.
    */
    if ((cpn_p->per_length0 != 0 || cpn_p->per_length1 != 0) &&
		cpn->cpn_etab_id < 0)
    {
	dm0s_mlock(&odcb->odcb_cq_mutex);
	for ( xccb = odcb->odcb_cq_next, found = FALSE;
	      xccb != (DML_XCCB*) &odcb->odcb_cq_next;
	      xccb = xccb->xccb_q_next )
	{
	    if ( xccb->xccb_operation == XCCB_T_DESTROY &&
		 xccb->xccb_t_table_id.db_tab_base == cpn->cpn_etab_id)
	    {
		/* Found temp holding the blob this locator references */
		found = TRUE;
		break;
	    }
	}
	dm0s_munlock(&odcb->odcb_cq_mutex);
	if (!found)
	{
	    SETDBERR(&pop_cb->pop_error, 0, E_AD700E_ADP_BAD_COUPON);
	    return (E_DB_ERROR);
	}
    }

    /* Keep small number of locators in memory */
    lloc_cxt = scb->scb_lloc_cxt;
    if (!lloc_cxt)
    {
	i4	max;

	max = 50; /* this is arbitrary, can really be any size */
	status = dm0m_allocate(sizeof(DMPE_LLOC_CXT) +
		(max * sizeof(ADP_PERIPHERAL)),
	    0, LLOC_CB, LLOC_ASCII_ID, (char *)xcb,
	    (DM_OBJECT **)&lloc_cxt, &pop_cb->pop_error);
	if (status != E_DB_OK)
	    return (E_DB_ERROR);
	scb->scb_lloc_cxt = lloc_cxt;
	lloc_cxt->lloc_cnt = 0;
	lloc_cxt->lloc_max = max;
	lloc_cxt->lloc_tbl = NULL;
	lloc_cxt->lloc_mem_pgcnt = 0;
	lloc_cxt->lloc_odcb = (PTR)xcb->xcb_odcb_ptr;
    }

    if (lloc_cxt->lloc_cnt < lloc_cxt->lloc_max)
    {
	/* Map tid to locator, lloc_adp indexed from 0, locators start at 1 */
	lloc_cxt->lloc_cnt++;
	locator = lloc_cxt->lloc_cnt;
	STRUCT_ASSIGN_MACRO(*cpn_p, lloc_cxt->lloc_adp[locator - 1]);

	/* Init output ADP_PERIPHERAL (ADP_P_LOCATOR) */
	loc_p->per_tag = ADP_P_LOCATOR;
	loc_p->per_length0 = cpn_p->per_length0;
	loc_p->per_length1 = cpn_p->per_length1;
        MEcopy(&locator,  sizeof(ADP_LOCATOR),
		((char *)loc_p + ADP_HDR_SIZE));
	if (xccb)
	{
	    /* Update xccb for temp holding this blob */
	    xccb->xccb_blob_temp_flags |= BLOB_HAS_LOCATOR;
	    xccb->xccb_blob_locator = locator;
	}
	return (E_DB_OK);
    }

    if (!lloc_cxt->lloc_tbl)
    {
	/* create session temp to hold locators */
	status = dmpe_create_locator_temp(scb, &pop_cb->pop_error);
	if (status)
	    return (status);
    }

    /* Insert record into locator temp */
    loc_rcb = (DMP_RCB *)lloc_cxt->lloc_tbl;
    MEfill(sizeof(DMR_CB), 0, &dmrcb);
    dmrcb.length = sizeof(DMR_CB);
    dmrcb.type = DMR_RECORD_CB;
    dmrcb.ascii_id = DMR_ASCII_ID;
    dmrcb.dmr_access_id = scb->scb_lloc_cxt->lloc_tbl;
    dmrcb.dmr_data.data_address = (char *) cpn_p;
    dmrcb.dmr_data.data_in_size = sizeof(ADP_PERIPHERAL);

    status = dmr_put(&dmrcb);
    if (status)
        pop_cb->pop_error = dmrcb.error;

    loc_stat = dm2r_unfix_pages(loc_rcb, &local_dberr);
    if (loc_stat != E_DB_OK && status == E_DB_OK)
    {
	status = loc_stat;
	pop_cb->pop_error = local_dberr;
    }

    if (status == E_DB_OK)
    {
	i4		page;

	loc_tid.tid_i4 = dmrcb.dmr_tid;
	page = loc_tid.tid_tid.tid_page;

	/* Map tid to locator */
	page += lloc_cxt->lloc_mem_pgcnt;
	locator = (page * loc_rcb->rcb_tcb_ptr->tcb_tperpage)
		+ (loc_tid.tid_tid.tid_line);

	/* Init output ADP_PERIPHERAL (ADP_P_LOCATOR) */
	loc_p->per_tag = ADP_P_LOCATOR;
	loc_p->per_length0 = cpn_p->per_length0;
	loc_p->per_length1 = cpn_p->per_length1;
        MEcopy(&locator,  sizeof(ADP_LOCATOR),
		((char *)loc_p + ADP_HDR_SIZE));

	if (xccb)
	{
	    /* Update xccb for temp holding this blob */
	    xccb->xccb_blob_temp_flags |= BLOB_HAS_LOCATOR;
	    xccb->xccb_blob_locator = locator;
	}

	return (E_DB_OK);
    }

    return (status);
}


/*
** Name: dmpe_create_locator_temp 	Create temp table to hold blob locators
**
** Description:
**      This routine creates a session temp table to store blob locators.
**
** Inputs:
**      scb				    Pointer to corresponding scb
**
** Outputs:
**      err_code			    The reason for error status.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** History:
**      16-oct-2006 (stial01)
**	    created.
**      17-Feb-2010 (hanal04) Bug 123160
**          Add scb reference to the LOCATOR temp table name. Otherwise
**          dm2t_lookup_tabid() may match on owner.tabname from other
**          sessions.
*/
static DB_STATUS
dmpe_create_locator_temp(
DML_SCB		*scb,
DB_ERROR	*error)
{
    DB_STATUS           status;
    DMT_CB		dmtcb;
    DB_LOC_NAME		loc;
    DMT_CHAR_ENTRY	characteristics[3];
    DMP_TCB             *tcb;
    DMU_CB		dmucb;
    DB_TAB_ID		loc_tab_id;
    DML_XCB		*xcb;
    DMPE_LLOC_CXT	*lloc_cxt;
    i4			err_code;

#define             DMPE_LOC_ATTS      1

    DMF_ATTR_ENTRY	*atts[DMPE_LOC_ATTS];
    DMF_ATTR_ENTRY	att_ents[DMPE_LOC_ATTS];
    DB_TAB_TIMESTAMP	timestamp;
    DMP_RCB		*loc_rcb;

    do
    {
	xcb = scb->scb_x_next;
	lloc_cxt = scb->scb_lloc_cxt;

	dmtcb.type = DMU_UTILITY_CB;
	dmtcb.length = sizeof(DMT_TABLE_CB);

	/* LOCATOR_TEMPORARY_TABLE gets session lifetime */
	dmtcb.dmt_flags_mask = DMT_LOCATOR_TEMP;

	dmtcb.dmt_tran_id = (PTR)xcb;
	dmtcb.dmt_db_id = (PTR)xcb->xcb_odcb_ptr;

        STprintf(dmtcb.dmt_table.db_tab_name, "<LOC_TEMP_SID:%p>", scb);
        STmove(dmtcb.dmt_table.db_tab_name, ' ',
               sizeof(dmtcb.dmt_table),
               dmtcb.dmt_table.db_tab_name);

	MEmove(8, "$default", ' ', sizeof(DB_LOC_NAME), (PTR) loc.db_loc_name);
	dmtcb.dmt_location.data_address = (PTR) &loc;
	dmtcb.dmt_location.data_in_size = sizeof(loc);

	dmtcb.dmt_key_array.ptr_address = 0;
	dmtcb.dmt_key_array.ptr_size = 0;
	dmtcb.dmt_key_array.ptr_in_count = 0;

	characteristics[0].char_id = DMT_C_PAGE_SIZE;
	characteristics[0].char_value = dmf_svcb->svcb_page_size;
	characteristics[1].char_id = DMT_C_DUPLICATES;
	characteristics[1].char_value = DMT_C_ON;
	dmtcb.dmt_char_array.data_address = (PTR)characteristics;
	dmtcb.dmt_char_array.data_in_size = 2 * sizeof(DMT_CHAR_ENTRY);

	dmtcb.dmt_attr_array.ptr_in_count = DMPE_LOC_ATTS;
	dmtcb.dmt_attr_array.ptr_size = sizeof(DMF_ATTR_ENTRY);
	dmtcb.dmt_attr_array.ptr_address = (PTR) atts;

	atts[0] = &att_ents[0];
	att_ents[0].attr_defaultTuple = (DB_IIDEFAULT *) 0;
	SET_CANON_DEF_ID( att_ents[0].attr_defaultID, DB_DEF_NOT_DEFAULT );

	MEmove(6, "coupon", ' ',
		sizeof(att_ents[0].attr_name), (PTR) &att_ents[0].attr_name);
	att_ents[0].attr_type = DB_BYTE_TYPE;
	att_ents[0].attr_size = sizeof(ADP_PERIPHERAL);
	att_ents[0].attr_precision = 0;
	att_ents[0].attr_collID = 0;
	att_ents[0].attr_flags_mask = 0;

	status = dmt_create_temp(&dmtcb);
	if (status)
	{
	    *error = dmtcb.error;
	    break;
	}

	STRUCT_ASSIGN_MACRO(dmtcb.dmt_id, loc_tab_id);

	MEfill(sizeof(DMT_CB), 0, (PTR)&dmtcb);
	dmtcb.length = sizeof(DMT_TABLE_CB);
	dmtcb.type = DMT_TABLE_CB;
	dmtcb.ascii_id = DMT_ASCII_ID;
	dmtcb.dmt_db_id = (PTR) xcb->xcb_odcb_ptr;
	dmtcb.dmt_tran_id = (PTR) xcb;
	dmtcb.dmt_sequence = 0; /* only need sequence# if deferred cursor */
	dmtcb.dmt_lock_mode = DMT_S;
	dmtcb.dmt_update_mode = DMT_U_DIRECT;
	dmtcb.dmt_access_mode = DMT_A_WRITE;
	dmtcb.dmt_flags_mask = DMT_SESSION_TEMP;
	STRUCT_ASSIGN_MACRO(loc_tab_id, dmtcb.dmt_id);
	dmtcb.dmt_record_access_id = 0;

	/*
	** Open the locator temp now... and keep it open
	** FIX ME should we open/close for each put/get operation
	*/
	status = dmt_open(&dmtcb);
	if (DB_FAILURE_MACRO(status))
	{
	    *error = dmtcb.error;
	    break;
	}

	lloc_cxt->lloc_tbl = dmtcb.dmt_record_access_id;
	loc_rcb = (DMP_RCB *)lloc_cxt->lloc_tbl;

	lloc_cxt->lloc_mem_pgcnt = (lloc_cxt->lloc_max /
				loc_rcb->rcb_tcb_ptr->tcb_tperpage) + 1;

	/* Drop out, success or failure */
    } while (0);

    if (DB_FAILURE_MACRO(status))
    {
	if ((error->err_code != E_DM0065_USER_INTR) &&
            (error->err_code != E_DM016B_LOCK_INTR_FA))
	{
	    uleFormat(error, 0, NULL , ULE_LOG,
			NULL, (char *) NULL, 0L, (i4 *) NULL, &err_code, 0);
	    SETDBERR(error, 0, E_DM9B09_DMPE_TEMP);
	}
    }

    return(status);
}


/*
** Name: dmpe_locator_to_cpn 	Return coupon for input blob locator
**
** Description:
**      This routine takes a blob locator and returns the corresponding coupon.
**
** Inputs:
**	pop_cb				    ADP_POP_CB control block...
**	    pop_segment			    Ptr to DB_DATA_VALUE containing the
**					    locator.
**
** Outputs:
**	pop_cb				    ADP_POP_CB control block...
**	    pop_coupon			    Ptr to DB_DATA_VALUE containing the
**					    coupon to be used to retrieve the
**					    object.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** History:
**      16-oct-2006 (stial01)
**	    created.
**	24-jun-2009 (gupsh01)
**	    Fix setting the locator values at constant offset of
**	    ADP_HDR_SIZE in the ADP_PERIPHERAL structure, irrespective
**	    of the platform. This was not happening for 64 bit platforms
**	    where per_value was aligned on ALIGN_RESTRICT boundary, and
**	    this causes problems for API / JDBC etc as it expects it to
**	    to be at the constant offset irrespective of the platform.
**	5-Apr-2010 (kschendel) SIR 123485
**	    Remove deprecated CSswitch call.
*/

static DB_STATUS
dmpe_locator_to_cpn(ADP_POP_CB	*pop_cb)
{
    DB_STATUS           status;
    i4			err_code;
    DML_ODCB		*odcb;
    DML_SCB		*parent_scb, *thread_scb;
    DML_SCB		*scb;
    DML_XCB		*xcb = NULL;
    CS_SID		sid;
    ADP_PERIPHERAL	*loc_p = (ADP_PERIPHERAL *)pop_cb->pop_segment->db_data;
    ADP_PERIPHERAL	*cpn_p = (ADP_PERIPHERAL *) pop_cb->pop_coupon->db_data;
    DMPE_PCB		*pcb = (DMPE_PCB *) 0;
    DMP_RCB		*loc_rcb;
    DM_TID		loc_tid;
    ADP_LOCATOR		locator;
    ADP_PERIPHERAL	tmp_p;
    DMR_CB              dmrcb;
    DMPE_LLOC_CXT	*lloc_cxt;

    CLRDBERR(&pop_cb->pop_error);

    /* Need to get context to anchor locator to this session */
    CSget_sid(&sid);
    thread_scb = GET_DML_SCB(sid);

    if (thread_scb->scb_x_ref_count > 0)
	xcb = thread_scb->scb_x_next;
    else
    {
	SETDBERR(&pop_cb->pop_error, 0, E_DM9B08_DMPE_NEED_TRAN);
	return (E_DB_ERROR);
    }

    /* Always use locator context off of parent scb */
    /* FIX ME do we need to mutex updates to LLOC_CXT */
    odcb =  xcb->xcb_odcb_ptr;
    parent_scb = odcb->odcb_scb_ptr;
    scb = parent_scb;

    if (scb->scb_x_ref_count > 0)
	xcb = scb->scb_x_next;
    else
    {
	SETDBERR(&pop_cb->pop_error, 0, E_DM9B08_DMPE_NEED_TRAN);
	return (E_DB_WARN);
    }

    lloc_cxt = scb->scb_lloc_cxt;
    if (!lloc_cxt)
    {
	SETDBERR(&pop_cb->pop_error, 0, E_DM9B08_DMPE_NEED_TRAN); /* FIX ME */
	return (E_DB_WARN);
    }

    /* Extract locator from pop */
    MEcopy( ((char *)loc_p + ADP_HDR_SIZE),
		sizeof(ADP_LOCATOR), &locator);

    /* See if coupon is in cache */
    if (locator <= lloc_cxt->lloc_max)
    {
	STRUCT_ASSIGN_MACRO(lloc_cxt->lloc_adp[locator - 1], *cpn_p);

	if (cpn_p->per_tag == -1)
	{
	    /* this locator has been freed */
	    return (E_DB_ERROR);
	}

	return (E_DB_OK);
    }

    /*
    ** to create a locator we only need the coupon
    ** the short term tcb pointer in DMPE_COUPON is no longer needed during get
    ** because dmpe_allocate gets context by calling CSget_sid, GET_DML_SCB.
    */
    loc_rcb = (DMP_RCB *)lloc_cxt->lloc_tbl;
    if (!loc_rcb)
    {
	SETDBERR(&pop_cb->pop_error, 0, E_DM9B08_DMPE_NEED_TRAN); /* FIX ME */
	return (E_DB_WARN);
    }

    /* Map locator to tid in locator temp table */
    loc_tid.tid_tid.tid_page = locator / loc_rcb->rcb_tcb_ptr->tcb_tperpage;
    loc_tid.tid_tid.tid_page -= lloc_cxt->lloc_mem_pgcnt;
    loc_tid.tid_tid.tid_line = locator % loc_rcb->rcb_tcb_ptr->tcb_tperpage;

    MEfill(sizeof(DMR_CB), 0, (PTR) &dmrcb);
    dmrcb.length = sizeof(DMR_CB);
    dmrcb.type = DMR_RECORD_CB;
    dmrcb.ascii_id = DMR_ASCII_ID;
    dmrcb.dmr_access_id = lloc_cxt->lloc_tbl;
    dmrcb.dmr_flags_mask = DMR_BY_TID;
    dmrcb.dmr_data.data_address = (char *) &tmp_p;
    dmrcb.dmr_data.data_in_size = sizeof(ADP_PERIPHERAL);
    dmrcb.dmr_tid = loc_tid.tid_i4;

    status = dmr_get(&dmrcb);
    if (status)
    {
	pop_cb->pop_error = dmrcb.error;
	if (dmrcb.error.err_code ==  E_DM003C_BAD_TID)
	    return (E_DB_WARN);
	else
	    return (E_DB_ERROR);
    }

    MEcopy((PTR)&tmp_p, sizeof(ADP_PERIPHERAL), cpn_p);
    if (cpn_p->per_tag == -1)
    {
	/* this locator has been freed */
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}


/*
** {
** Name: dmpe_free_locator -- free any large object temps associated
**                          with this session for the specified locator(s).
**
** Description:
**      This routine scans the list of session-lifetime temporaries
**      associated with this session, destroying any that are flagged
**	as large-object holding temporaries for the specified locator(s).
**
**      It is expected that this routine will be called when it is safe
**      to do so -- meaning that there are no blobs "in flight".
**
**	Session temp table etabs are not considered "temporary", and
**	are not flagged as such.  Therefore they are not deleted
**	by this routine.  Only anonymous holding temp tables are deleted.
**
** Inputs:
**	odcb			Open database ID
**
** Outputs:
**	err_code		error code if any errors
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    Session XCCB list pruned of any deleted locator tmp tables.
**
** History:
**      09-feb-2007 (stial01)
**          Created from dmpe_free_temps()
**      27-Mar-2009 (hanal04) Bug 121857
**          Remove redundant xDEBUG checks on the input session_id which
**          is no longer passed into this function.
**	24-jun-2009 (gupsh01)
**	    Fix setting the locator values at constant offset of
**	    ADP_HDR_SIZE in the ADP_PERIPHERAL structure, irrespective
**	    of the platform. This was not happening for 64 bit platforms
**	    where per_value was aligned on ALIGN_RESTRICT boundary, and
**	    this causes problems for API / JDBC etc as it expects it to
**	    to be at the constant offset irrespective of the platform.
**
[@history_template@]...
*/
DB_STATUS
dmpe_free_locator(ADP_POP_CB *pop_cb)
{
    CS_SID		sid;
    DB_STATUS		status = E_DB_OK;
    DML_SCB		*thread_scb;
    DML_SCB		*input_scb;
    DML_XCCB		*xccb, *next_xccb;
    DMPE_LLOC_CXT	*lloc_cxt;
    DM_OBJECT           *object;
    i4			error;
    i4			local_error;
    DMP_RCB		*loc_rcb = NULL;
    DMT_CB              dmtcb;
    DMR_CB              dmrcb;
    DML_ODCB            *odcb;
    DB_DATA_VALUE	*loc_dv = pop_cb->pop_segment;
    ADP_LOCATOR         locator;
    ADP_PERIPHERAL	invalid_per;
    DMPE_COUPON		*invalid_cpn = NULL;
    DM_TID		loc_tid;
    DB_ERROR		local_dberr;

    /*
    ** Get current thread
    ** We don't worry if this parent thread... since this routine
    ** should not be called as part of query processing,
    ** instead it is currently part of commit (or explicit drop locator)
    */
    if (loc_dv)
        MEcopy( ((char *)loc_dv->db_data + ADP_HDR_SIZE),
		sizeof(ADP_LOCATOR), &locator);
    else
	locator = 0;

    CLRDBERR(&pop_cb->pop_error);

    CSget_sid(&sid);
    thread_scb = GET_DML_SCB(sid);

    /* Check if any locators created by this session */
    lloc_cxt = thread_scb->scb_lloc_cxt;
    if (!lloc_cxt)
	return (E_DB_OK);

    odcb = (DML_ODCB *)lloc_cxt->lloc_odcb;

    /* Don't actually need a transaction to delete session temps,
    ** remove the check.
    */

    if (locator == 0)
    {
	/* If free ALL locators, close LOCATOR_TEMPORARY_TABLE */
	if (lloc_cxt->lloc_tbl)
	{
#ifdef xDEBUG
	    TRdisplay("%@ Close locator temp table\n");
#endif

	    loc_rcb = (DMP_RCB *)lloc_cxt->lloc_tbl;
	    MEfill(sizeof(DMT_CB), 0, (PTR) &dmtcb);
	    dmtcb.length = sizeof(DMT_TABLE_CB);
	    dmtcb.type = DMT_TABLE_CB;
	    dmtcb.ascii_id = DMT_ASCII_ID;
	    dmtcb.dmt_record_access_id = (PTR)lloc_cxt->lloc_tbl;
	    lloc_cxt->lloc_tbl = NULL;
	    status = dmt_close(&dmtcb);
	    if (status)
	    {
		pop_cb->pop_error = dmtcb.error;
		return (status);
	    }
	}

	dm0m_deallocate((DM_OBJECT **)&thread_scb->scb_lloc_cxt);
	lloc_cxt = NULL;
    }
    else
    {
	/* Invalidate input locator */
	if (locator <= lloc_cxt->lloc_max)
	{
	    lloc_cxt->lloc_adp[locator - 1].per_tag = -1;
	    STRUCT_ASSIGN_MACRO(lloc_cxt->lloc_adp[locator - 1], invalid_per);
	    invalid_cpn = (DMPE_COUPON *)&invalid_per.per_value.val_coupon;
	}
	else
	{
	    loc_rcb = (DMP_RCB *)lloc_cxt->lloc_tbl;
	    if (!loc_rcb)
	    {
		SETDBERR(&pop_cb->pop_error, 0, E_DM9B08_DMPE_NEED_TRAN); /* FIX ME */
		return (E_DB_WARN);
	    }

	    /* Map locator to tid in locator temp table */
	    loc_tid.tid_tid.tid_page = locator / loc_rcb->rcb_tcb_ptr->tcb_tperpage;
	    loc_tid.tid_tid.tid_page -= lloc_cxt->lloc_mem_pgcnt;
	    loc_tid.tid_tid.tid_line = locator % loc_rcb->rcb_tcb_ptr->tcb_tperpage;

	    MEfill(sizeof(DMR_CB), 0, &dmrcb);
	    dmrcb.length = sizeof(DMR_CB);
	    dmrcb.type = DMR_RECORD_CB;
	    dmrcb.ascii_id = DMR_ASCII_ID;
	    dmrcb.dmr_access_id = lloc_cxt->lloc_tbl;
	    dmrcb.dmr_flags_mask = DMR_BY_TID;
	    dmrcb.dmr_data.data_address = (char *) &invalid_per;
	    dmrcb.dmr_data.data_in_size = sizeof(ADP_PERIPHERAL);
	    dmrcb.dmr_tid = loc_tid.tid_i4;

	    status = dmr_get(&dmrcb);
	    if (status)
	    {
		pop_cb->pop_error = dmrcb.error;
		if (dmrcb.error.err_code ==  E_DM003C_BAD_TID)
		    return (E_DB_WARN);
		else
		    return (E_DB_ERROR);
	    }

	    invalid_per.per_tag = -1;
	    invalid_cpn = (DMPE_COUPON *)&invalid_per.per_value.val_coupon;
	    dmrcb.dmr_flags_mask = DMR_CURRENT_POS;
	    status = dmr_replace(&dmrcb);
	    if (status)
	    {
		pop_cb->pop_error = dmrcb.error;
		if (dmrcb.error.err_code == E_DM003C_BAD_TID)
		    return (E_DB_WARN);
		else
		    return (E_DB_ERROR);
	    }
	}
    }

    /* If FREE one locator, we're done unless there is a temp for it */
    if (locator != 0 && invalid_cpn && invalid_cpn->cpn_etab_id > 0)
    {
	return (E_DB_OK);
    }

    /* Even though we're the parent, protect against any factotums */

    /*
    ** Note the following code assumes:
    ** LOCATOR_TEMPORARY_TABLE has session lifetime
    ** Blob temps get session lifetime, see explanations in dmpe_temp
    */
    dm0s_mlock(&odcb->odcb_cq_mutex);
    for (xccb = odcb->odcb_cq_next;
	 xccb != (DML_XCCB *) &odcb->odcb_cq_next;
	 xccb = next_xccb)
    {
	/* Save next in case we deallocate this xccb */
	next_xccb = xccb->xccb_q_next;
	if (xccb->xccb_operation != XCCB_T_DESTROY ||
		xccb->xccb_blob_temp_flags == 0)
	    continue;

	if (( (xccb->xccb_blob_temp_flags & BLOB_HOLDING_TEMP)
	  && (xccb->xccb_blob_temp_flags & BLOB_HAS_LOCATOR)
	  && (locator == 0 || locator == xccb->xccb_blob_locator))
	  ||
	  (locator == 0 && (xccb->xccb_blob_temp_flags & BLOB_LOCATOR_TBL)))
	{
	    /* Found one to get rid of.  Take it off the queue so that
	    ** no other thread tries to get rid of it via any sort of
	    ** cleanup.
	    */
#ifdef xDEBUG
	    TRdisplay("%@ DESTROY blob holding temp %x\n",
		xccb->xccb_blob_temp_flags);
#endif
	    xccb->xccb_q_prev->xccb_q_next = xccb->xccb_q_next;
	    xccb->xccb_q_next->xccb_q_prev = xccb->xccb_q_prev;
	    dm0s_munlock(&odcb->odcb_cq_mutex);
	    /* Delete, but not with dmt_delete_temp since it expects
	    ** a valid transaction which we may not have.
	    */
	    status = dmt_destroy_temp(thread_scb, xccb, &local_dberr);
	    if (status != E_DB_OK &&
		local_dberr.err_code != E_DM0054_NONEXISTENT_TABLE)
	    {
		/* If it didn't work, log the problem */
		uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL,
				NULL, 0, NULL, &local_error, 0);
		SETDBERR(&pop_cb->pop_error, 0, E_DM9B09_DMPE_TEMP);
	    }
	    /* Works or not, get rid of the XCCB we unlinked */
	    dm0m_deallocate((DM_OBJECT **) &xccb);
	    dm0s_mlock(&odcb->odcb_cq_mutex);
	}
    }
    dm0s_munlock(&odcb->odcb_cq_mutex);

    return(status);
}
