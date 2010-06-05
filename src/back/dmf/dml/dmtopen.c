/*
** Copyright (c) 1985, 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <pc.h>
#include    <me.h>
#include    <tm.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <sxf.h>
#include    <adf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmxcb.h>
#include    <dmccb.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dml.h>
#include    <dm2t.h>
#include    <dmftrace.h>
#include    <dma.h>

/**
** Name: DMTOPEN.C - Implements the DMF open table operation.
**
** Description:
**      This file contains the functions necessary to open a table.
**      This file defines:
**
**      dmt_open - Open a table.
**
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**	17-dec-1985 (derek)
**	    Completed code for dmt_open().
**	23-oct_1988 (teg)
**	    Added check for error condition of trying to open a gateway.
**	15-aug-1989 (rogerk)
**	    Took out error condition of trying to open a gateway.
**	25-sep-1989 (rogerk)
**	    Fixed lockmode bugs in check_setlock.  We were incorrectly
**	    choosing write-mode locks whenever the LEVEL is specified in
**	    in a SET LOCKMODE command.
**	26-dec-89 (paul)
**	    More stuff stopping us from opening gateway tables removed.
**      14-jun-90 (linda, bryanp)
**          Added support for new access mode DMT_A_OPEN_NOACCESS, used by
**          gateways only (for now) to indicate that a table is opened for
**          which no other access will be made, only a close can be done.
**          Used to support remove table when foreign data file does not exist.
**      19-nov-1990 (rogerk)
**	    Set record control block to use the session's timezone
**	    rather than the server's timezone.
**	    This was done as part of the DBMS Timezone changes.
**	21-jan-1991 (rogerk)
**	    Changed order of precendence of locking characteristics so that
**	    user supplied session level lock characteristics do no override
**	    caller supplied lockmode values (while user supplied table level
**	    characteristics will continue to).
**	 4-feb-1991 (rogerk)
**	    Added support for Set Nologging.  If session is running with 
**	    NoLogging then turn off rcb_logging.
**      09-sep-1991 (mikem)
**          bug #39632
**          Add return of # of tuples and pages of a table following an open.
**      10-oct-1991 (jnash) merged 6-June-1989 (ac)
**          Added 2PC support.
**      10-oct-1991 (jnash) merged 18-JUL-89 (JENNIFER)
**          Added a flag to indicate that an internal request
**          for statistical information is being requested.
**          This solves a B1 security problem where statistics
**          could be unpredicatable if B1 labels were checked.
**          For an internal request they aren't checked, for a
**          normal query on the iihistogram and iistatistics
**          table they are checked.
**      10-oct-1991 (jnash) merged 24-sep-1990 (rogerk)
**          Merged 6.3 changes into 6.5.
**      10-oct-1991 (jnash) merged 14-jan-91 (andre)
**          MAde a temporary fix to reduce the number of deadlocks caused by
**          lock escalation on IITREE.  THIS IS A TEMPORARY FIX.
**      10-oct-1991 (jnash) merged 14-jun-1991 (Derek)
**          Added performance profiling support.
**      10-oct-1991 (jnash)
**          Add function header to check_setlock.
**	7-jul-1992 (bryanp)
**	    Prototyping DMF.
**	8-jul-1992 (walt)
**	    Prototyping DMF.
**      25-sep-1992 (stevet)
**          Changed scb_timezone to scb_tzcb for new timezone support.  
**	18-dec-1992 (robf)
**	    Removed dm0a.h
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed old xcb_abt_sp_addr,rcb_abt_sp_addr needed with BI recovery.
**	    Added call to dmxe_writebt if a table is opened in write mode
**		during a transaction which has not yet executed an update.
**		Opening a table for write now escalates the transaction to
**		an update one.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system typedefs
**	28-may-1993 (robf)
**	    Secure 2.0: Rework old ORANGE code
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	26-jul-1993 (rogerk)
**	  - Removed code which escalates read-only transaction to be a write
**	    transaction when a table is open in update mode.
**	  - Include respective CL headers for all cl calls.
**      14-Apr-1994 (fred)
**          Save sequence number of each open in the xcb.  This is
**          used so that large object code can manage to avoid
**          deleting temporary files for queries other than their own.
**      20-jun-1994 (jnash)
**          fsync project.  Add DMT_A_SYNC_CLOSE access mode.  This causes 
**	    the table to be opened in a mode where updates are not performed 
**	    synchronously, but are instead sunk when the table is closed (in 
**	    dm2t_release_tcb()).  Callers must take care that recovery works 
**	    properly whenever this open mode is employed (ususally this
**	    means that individual writes are not independently recovered).
**      19-dec-94 (sarjo01) from 18-feb-94 (markm) 
**          Added dmt_mustlock check to make sure that we are not setting
**          DM2T_NOREADLOCK when the query involved is actually updating,
**          bug 58681.
**      19-dec-94 (sarjo01) from 24-aug-1994 (johna)
**          Changed markm's check for mustlock from (!mustlock) to
**          (mustlock != TRUE). The mustlock element of the DMT_CB is not
**          initialized in many places. The new check will make locking less
**          likely until we catch them all.
**      04-apr-1995 (dilma04)
**          Add support for SET TRANSACTION ISOLATION LEVEL ... and READLOCK=
**          READ_COMMITTED/REPEATABLE_READ (originally proposed by bryanp)
**	17-may-1995 (lewda02/shust01)
**	    RAAT API Project:
**	    Added user control over locking.
**	8-nov-1995 (thaju02)
**          RAAT API bug#72402 - attempting to open table while not in
**          a transaction, causes the dbms server to segv.  Added check
**          to ensure a non-null xcb scb pointer.
**	25-apr-1996 (kch)
**	    In the function dmt_open() we now do not check for user supplied
**	    session level locking values if the table access mode is
**	    DMT_A_SYNC_CLOSE. This change fixes bug 76048.
**	16-nov-1996 (toumi01)
**	    RAAT API bug#78133 - attempting to open table while not in
**	    a transaction, causes the dbms server to segv.  Added check
**	    for valid xcb before referencing it for scb etc.  Fix for
**	    bug#72402 did not work for axp_osf platform.  An invalid xcb
**	    can contain a non-null, invalid scb.  If not xDEBUG, do
**	    minimal block check (instead of full dm0m_check) and return
**	    more user-friendly message (same as for null scb trap).
**      22-nov-96 (dilma04)
**          Add support for row-level locking.
**      12-dec-96 (dilma04)
**          Fixed row locking bugs (79610,79613,79629,79630) in check_setlock.
**          We were incorrectly choosing lock mode if the LEVEL specified for 
**          a session was overridden for a particular table.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          - replaced complicated and potentially buggy dm2t_check_setlock()
**          routine with two very simple routines: dmt_check_table_lock() and
**          dmt_set_lock_values();
**          - changed parameters for check_char() and dm2t_open() calls.
**          This change fixes bugs 79590, 79724 and 80577. It also fixes
**          problems reported as bugs 80563 and 80564 for OI 1.2/01.  
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction 
**          access mode> (READ ONLY | READ WRITE) per SQL-92. Replaced 
**          E_DM005D_TABLE_ACCESS_CONFLICT with E_DM006A_TRAN_ACCESS_CONFLICT.
**	09-apr-97 (cohmi01)
**	    Allow readlock, timeout & maxlocks system defaults to be 
**	    configured. #74045, #8616.
**      10-apr-97 (dilma04)
**          Set Lockmode Cleanup: 
**          - fixed bug 81367 and few other RAAT inconsistencies;
**          - simplified check_char() and dmt_set_lock_values routines.
**      28-may-97 (dilma04)
**          Added handling of DMT_RIS and DMT_RIX lock modes needed for
**          row locking support in RAAT.
**      13-jun-97 (dilma04)
**          To comply with the ANSI standart, enforce serializability if
**          the operation on the table is associated with a constraint.
**          Enforce CS or RR isolation level if an appropriate trace point
**          is set. 
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**      14-oct-97 (stial01)
**          dmt_open() Changes to distinguish readlock value/isolation level
**      12-nov-97 (stial01)
**          dmt_set_lock_values() readonly cursor check for iso level RU.
**      18-nov-97 (stial01)
**          QEF may specify DMT_C_EST_PAGES.
**      02-feb-98 (stial01)
**          Back out 18-nov-97 change.
**      07-jul-98 (stial01)
**          Ifdef TRdisplay
**	11-Aug-1998 (jenjo02)
**	    Extract dmt_record_count, dmt_page_count from RCB instead
**	    of TCB.
**	28-oct-1998 (somsa01)
**	    In the case of DMT_SESSION_TEMP, the lock will live on the
**	    session lock list, not the transaction lock list.  (Bug #94059)
**      18-Oct-1999 (hanal04) Bug 94474 INGSRV633
**          Remove the above change. The locks have been moved back to
**          the TX level. The new flags used to identify a global temporary
**          table have been maintained as they may prove useful at some
**          time in the future.
**      20-May-2000 (hanal04, stial01 & thaju02) Bug 101418 INGSRV 1170
**          Global session temporary table lock is back on the Session lock 
**          list.
**	10-nov-2000 (shust01)
**	    ifdef'd out a TRdisplay.  Can fill up the dbms.log with a message
**	    that is just informative.  This is a partial cross of a stial01
**	    change from main, #436735.  bug #103188.
**	29-nov-2000 (shust01)
**	    Using RAAT, you can get serializable (SIX) page locks when running,
**	    instead of IX locks.  This causes other users to potentially get
**	    lock timeouts when trying to access the same page, when using
**	    row-level locking. (bug 103127, INGSRV 1306).
**	01-dec-2000 (thaju02)
**	    Modified dmt_open(). If dmt_flags_mask DMT_FORCE_NOLOCK bit set, 
**	    then avoid having dmt_set_lock_value override the lock_mode. 
**	    (cont. B103150)
**      17-may-99 (stial01)
**          QEF may specify DMT_C_EST_PAGES, DMT_C_TOTAL_PAGES
**      07-jul-99 (stial01)
**          Escalate only if isolation SERIALIZABLE (B84771)
**      04-nov-99 (stial01)
**          dmt_set_lock_values() row locking for catalogs may be overridden
**      30-may-2000 (stial01)
**          Escalate lock mode for update cursors only if  est pages > maxlocks
**          Escalate only if lock mode = DMC_C_SYSTEM (startrak 8737602,b101794)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      30-Jan-2002 (hanal04) Bug 111710 INGSRV2689
**          Acquire TABLE level LK_SIX locks for update TXs that are going 
**          to hit every page.
**      16-dec-2004 (stial01)
**          Changes for config param system_lock_level
**      12-jan-2005 (stial01)
**          dmt_set_lock_values: Do not escalate to table level LK_SIX (b113732)
**      16-feb-2005 (stial01)
**          DMT_CONSTRAINT processing, use repeatable read lock protocols
**      05-oct-2006 (stial01)
**          dmt_open() lockmode DMT_N -> readlock =  DMC_C_READ_NOLOCK, may be specified by QEF
**          during DDL processing (112560).
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**/

/*
** Forward declarations of static functions:
*/
    /* check for caller supplied lock characteristics. */
static STATUS check_char(
    DMT_CB	*dmt_cb,
    i4	*timeout,
    i4	*max_locks,
    i4     *lock_level,
    i4     *iso_level,
    i4     *est_pages,
    i4     *total_pages);

/*{
** Name:  dmt_open - Open a table.
**
**  INTERNAL DMF call format:  status = dmt_open(&dmt_cb);
**
**  EXTERNAL call format:      status = dmf_call(DMT_OPEN,&dmt_cb);
**
** Description:
**      This function opens the table in the specified database with the access,
**      update and lock modes provided and associates it with the given active
**      transaction.   If the table is successfully opened, a record access id
**      is returned.  The record access id is used to identify an instance of a
**      table open and is required input for all other table or record 
**      operations against the table.  Also if the table is successfully opened,
**	the tuple and page count of the table are returned to the caller.
**
**      If the table is open in deferred update mode then
**      no other open for this table either deferred or direct may be in 
**      progress in this session.
**      NOTE: Must be called prior to any record operations.
**
**	When a table is opened by a DMF client, dmf directs its internal
**	operations to use the session's timezone rather than the timezone
**	in which the DBMS server is running.  This is done in order to
**	preserve session level timezone semantics - where each session
**	runs in the timezone of its Front End.  Most internal dmf operations
**	run in the timezone of the DBMS server.  Note that there are
**	currently some inconsistencies with this model of handling timezones -
**	these are discussed in further detail in the timezone notes document
**	in the DMF doc directory.
**
** Inputs:
**      dmt_cb
**          .type                           Must be set to DMT_TABLE_CB.
**          .length                         Must be at least
**                                          sizeof(DMT_TABLE_CB) bytes.
**          .dmt_db_id                      Database to search.
**          .dmt_tran_id                    Transaction to associate with.
**          .dmt_id                         Internal table identifier.
**          .dmt_sequence                   The statement (sequence) number
**                                          this open is associated with.
**                                          This value must not be zero.
**          .dmt_flags_mask                 Must be zero or DMT_NOWAIT or
**                                          DMT_SHOW_STAT or DMT_DBMS_REQUEST.
**          .dmt_lock_mode                  Mode to lock table.  Legal values
**                                          are DMT_X, DMT_SIX, DMT_S, DMT_IX,
**                                          DMT_IS, DMT_RIS, DMT_RIX. See 
**                                          DMTCB.h definition for explanation.
**          .dmt_update_mode                Type of update desired. Legal values
**                                          are DMT_U_DIRECT, DMT_U_DEFERRED.
**          .dmt_access_mode                Type of access desired. Legal values
**                                          are DMT_A_READ, DMT_A_WRITE, and
**					    DMT_A_OPEN_NOACCESS.
**          .dmt_char_array.data_address    Optional pointer to characteristic
**                                          array of type DMT_CHAR_ENTRY.
**                                          See below for description of
**                                          <dmt_char_array> entries.
**          .dmt_char_array.data_in_size    Length of dmt_char_array in bytes.
**
**          <dmt_char_array> entries of type DMT_CHAR_ARRAY and
**          must have the following format:
**              .char_id                    The characteristics identifer
**                                          denoting
**                                          the characteristics to set.
**                                          Only valid choices are:
**                                          DMT_TIMEOUT_LOCK (which says
**					    what the lock timeout value
**                                          should be, if it is not set
**                                          it waits forever) or 
**                                          DMT_PG_LOCKS_MAX (which will
**                                          set the number of page locks
**                                          to hold before escalation to
**                                          a table level lock).
**					    DMT_C_LOCKMODE (which indicates to
**					    open with supplied lock mode.)
**              .char_value                 The value to use to set this 
**                                          characteristic.
**
** Outputs:
**      dmt_cb
**          .dmt_record_access_id           Value used to identify this instance
**                                          of an open table to the record and
**                                          table DMF routines.
**	    .dmt_timestamp		    Incarnation of this table.
**	    .dmt_page_count		    # pages in the table.
**	    .dmt_record_count		    # tuples in the table.
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM000D_BAD_CHAR_ID
**                                          E_DM000E_BAD_CHAR_VALUE
**                                          E_DM0010_BAD_DB_ID
**                                          E_DM001A_BAD_FLAG
**                                          E_DM002A_BAD_PARAMETER
**                                          E_DM003B_BAD_TRAN_ID
**                                          E_DM0042_DEADLOCK
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**					    E_DM004D_LOCK_TIMER_EXPIRED
**					    E_DM0052_COMPRESSION_CONFLICT
**                                          E_DM0054_NONEXISTENT_TABLE
**                                          E_DM005E_CANT_UPDATE_SYSCAT
**                                          E_DM0065_USER_INTR
**                                          E_DM0064_USER_ABORT
**	    				    E_DM006A_TRAN_ACCESS_CONFLICT
**					    E_DM008F_ERROR_OPENING_TABLE
**	                                    E_DM009E_CANT_OPEN_VIEW
**                                          E_DM0100_DB_INCONSISTENT
**                                          E_DM010C_TRAN_ABORTED
**                                          E_DM0112_RESOURCE_QUOTA_EXCEEDED
**					    E_DM0114_FILE_NOT_FOUND
**
**      Returns:
**         E_DB_OK                          Function completed normally.
**         E_DB_WARN                        Function completed normally with a 
**                                          termination status which is in
**                                          dmt_cb.err_code.
**         E_DB_ERROR                       Function completed abnormally 
**                                          with a 
**                                          termination status which is in
**                                          dmt_cb.err_code.
**         E_DB_FATAL                       Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmt_cb.err_code.
** History:
**      01-sep-85 (jennifer)
**	    Created new for jupiter.
**	17-dec-1985 (derek)
**	    Completed code.
**	28-Nov-88 (teg)
**	    modified relcmptlvl check to stop bouncing 5.0 and below compat
**	    levels.
**	15-aug-89 (rogerk)
**	    Added support for Non-SQL Gateway.  Took out check that prevents
**	    us from doing an OPEN operation on a gateway table.
**	14-jun-90 (linda, bryanp)
**	    Added support for new access mode DMT_A_OPEN_NOACCESS, used by
**	    gateways only (for now) to indicate that a table is opened for
**	    which no other access will be made, only a close can be done.
**	    Used to support remove table when foreign data file does not exist.
**      19-nov-1990 (rogerk)
**	    Set record control block to use the session's timezone
**	    rather than the server's timezone.
**	    This was done as part of the DBMS Timezone changes.
**	21-jan-1991 (rogerk)
**	    Changed order of precendence of locking characteristics so that
**	    user supplied session level lock characteristics do no override
**	    caller supplied lockmode values (while user supplied table level
**	    characteristics will continue to).  Added type parameter to
**	    check_setlock call to be able to specify TABLE or SESSION lock
**	    parameters and changed dmt_open to make two separate calls to
**	    check_setlock: one to check session level values, and one to
**	    check table level values.
**	 4-feb-1991 (rogerk)
**	    Added support for Set Nologging.  If session is running with 
**	    NoLogging then turn off rcb_logging.
**	09-sep-1991 (mikem)
**	    bug #39632
**	    Add return of # of tuples and pages of a table following an open.
**	    Prior to this change QEF, while validating DBP QEP's would first
**	    call DMT_SHOW for this information and subsequently call DMT_OPEN.
**	    This extra call was expensive on platforms like the RS6000 or 
**	    DECSTATION where cross process semaphores must be implemented as
**	    system calls.  This change allows QEF (qeq_validate()) to eliminate
**	    the extra DMT_SHOW call.
**      10-oct-1991 (jnash) merged 6-June-1989 (ac)
**          Added 2PC support.
**      10-oct-1991 (jnash) merged 14-jan-91 (andre)
**          MAde a temporary fix to reduce the number of deadlocks caused by
**          lock escalation on IITREE.  THIS IS A TEMPORARY FIX.
**      10-oct-1991 (jnash) merged 14-jun-1991 (Derek)
**          Added performance profiling support.
**      10-oct-1991 (jnash) merged 18-JUL-89 (JENNIFER)
**          Added a flag to indicate that an internal request
**          for statistical information is being requested.
**          This solves a B1 security problem where statistics
**          could be unpredicatable if B1 labels were checked.
**          For an internal request they aren't checked, for a
**          normal query on the iihistogram and iistatistics
**          table they are checked.
**      25-sep-1992 (stevet)
**          Changed scb_timezone to scb_tzcb for new timezone support.  
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed old xcb_abt_sp_addr,rcb_abt_sp_addr needed with BI recovery.
**	    Added call to dmxe_writebt if a table is opened in write mode
**		during a transaction which has not yet executed an update.
**		Opening a table for write now escalates the transaction to
**		an update one.  This was done in order to allow Before Images
**		to be written whenever pages are fixed for write during
**		online backup.  Previously we only escalated the transaction
**		when the actual update call was made.
**	28-may-1993 (robf)
**		Secure 2.0: Rework old ORANGE code
**	26-jul-1993 (rogerk)
**	    Removed code which escalates read-only transaction to be a write
**	    transaction when a table is open in update mode.  This escalation
**	    was added to resolve online backup issues since BI records were
**	    being generated before we got to the dmxe_writebt calls in some
**	    update operations.  Now that BI records are no longer generated
**	    at fixpage time, we don't need this escalation here.
**      14-Apr-1994 (fred)
**          Save sequence number of each open in the xcb.  This is
**          used so that large object code can manage to avoid
**          deleting temporary files for queries other than their own.
**      20-jun-1994 (jnash)
**          fsync project.  Add DMT_A_SYNC_CLOSE access mode. 
**      04-apr-1995 (dilma04)
**          Add support for SET TRANSACTION ISOLATION LEVEL ... and READLOCK=
**          READ_COMMITTED/REPEATABLE_READ 
**	17-may-1995 (lewda02/sarjo01)
**	    RAAT API Project:
**	    Added user control over locking.
**	8-nov-1995 (thaju02)
**	    RAAT API bug#72402 - attempting to open table while not in
**	    a transaction, causes the dbms server to segv.  Added check
**	    to ensure a non-null xcb scb pointer.
**	25-apr-1996 (kch)
**	    We now do not check for user supplied session level locking
**	    values (calls to dm2t_check_setlock()) if the table access mode
**	    is DMT_A_SYNC_CLOSE (sync at close) - instead we use the default
**	    locking values which includes setting the lock mode to X table.
**	    Sync at close requires an X table lock - the user may have
**	    specified some other level of locking (this will cause failure
**	    in dm2t_open()). This change fixes bug 76048.
**      22-nov-1996 (dilma04)
**          Row-Level Locking Project:
**          Check for DMT_RIS lock mode if isolation level is READ UNCOMMITTED.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          - replaced dm2t_check_setlock() call to pick up session level
**          locking values with the code which just gets them from the SCB;
**          - replaced dm2t_check_setlock() call to pick up table level 
**          locking values with dmt_check_table_lock() call;
**          - add call to dmt_set_lock_values() which sets final locking 
**          characteristics before calling dm2t_open;
**          - replaced lock_duration parameter in dm2t_open() call with
**          iso_level. 
**          This change fixes bugs 79590, 79724 and 80577. It also fixes
**          problems reported as bugs 80563 and 80564 for OI 1.2/01. 
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction 
**          access mode> (READ ONLY | READ WRITE) per SQL-92. Replaced 
**          E_DM005D_TABLE_ACCESS_CONFLICT with E_DM006A_TRAN_ACCESS_CONFLICT.
**      10-apr-97 (dilma04) 
**          Set Lockmode Cleanup. Allow caller supplied isolation level to 
**          override user supplied transaction isolation level.
**      28-may-97 (dilma04)
**          Added handling of DMT_RIS and DMT_RIX lock modes.
**	3-jun-97 (stephenb)
**	    set up the XCB in the replicator RCBs
**      13-jun-97 (dilma04)
**          Add check for the new DMT_CB flag - DMT_CONSTRAINT.
**      14-oct-97 (stial01)
**          dmt_open() Changes to distinguish readlock value/isolation level
**	11-Aug-1998 (jenjo02)
**	    Extract dmt_record_count, dmt_page_count from RCB instead
**	    of TCB.
**	22-oct-98 (stephenb)
**	    Set logging and journalling state on replicator support tables to
**	    match the base table (and session).
**	28-oct-1998 (somsa01)
**	    In the case of DMT_SESSION_TEMP, the lock will live on the
**	    session lock list, not the transaction lock list.  (Bug #94059)
**	01-dec-2000 (thaju02)
**	    If dmt_flags_mask DMT_FORCE_NOLOCK bit set, then avoid
**	    having dmt_set_lock_value override the lock_mode. (cont. B103150)
**	22-jun-2004 (thaju02)
**	    Online modify and alter table drop col results in deadlock,
**	    involving table lock and tbl control locks. If DMT_LK_ONLN_NEEDED, 
**	    request shared online modify lock. (B112536)
**      29-Sep-2004 (fanch01)
**          Conditionally add LK_LOCAL flag if database/table is confined
**	    to one node.
**	8-Aug-2005 (schka24)
**	    Don't turn off session-temp flag in caller's CB, it can get
**	    duplicated and the dups should look like the original.  There
**	    doesn't appear to be any reason that the flag was cleared in the
**	    first place.
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
**	11-Sep-2006 (jonj)
**	    Don't dmxCheckForInterrupt if internal request.
**	    Transaction may not be at an atomic point
**	    in execution as required for LOGFULL_COMMIT.
**	14-Feb-2006 (kschendel)
**	    Set up session-id in RCB for easy cancel checking.
**	16-Apr-2008 (kschendel)
**	    Copy more settings to RCB's ADF_CB since qualification CX uses
**	    the RCB ADF_CB now, not qef's.
**	24-May-2009 (kiria01) b122051
**	    Swap test order of dmt_char_array to remove uninit warning.
**      01-Dec-2009 (horda03) B103150
**          Handle new flag DMT_NO_LOCK_WAIT. This flags indicstes that
**          a TIMEOUT of DMC_C_NOWAIT is used (don't
**          wait for a busy lock).
**      12-Dec-2009 (horda03) B103150
**          Need to reset RCB's timeout back to the value it would have been,
**          otherwise subsequent calls could timeout unexpectedly.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add support for MVCC lock level.
**	26-Feb-2010 (jonj)
**	    CRIB being used now anchored in RCB. READ_COMMITTED cursors
**	    use distinct CRIBs.
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Replace DMT_MVCC lock mode with DMT_MIS, DMT_MIX.
**	19-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support: dmpe passes base table
**	    CRIB in dmt_crib_ptr, use same CRIB for etabs.
*/

DB_STATUS
dmt_open(
DMT_CB   *dmt_cb)
{
    DMT_CB	    *dmt = dmt_cb;
    DML_SCB         *scb;
    DML_XCB	    *xcb;
    DMP_RCB	    *rcb = 0;
    DMP_RCB	    *r;
    DMP_TCB	    *t;
    DML_ODCB	    *odcb;
    ADF_CB	    *adf_cb;
    ADF_CB	    *ses_adfcb;
    i4	    error, local_error;
    DB_STATUS	    status;
    i4	    lock_mode;
    i4	    access_mode;
    i4	    timeout;
    i4      orig_timeout;
    i4	    max_locks;
    i4         readlock;
    i4         lock_level;
    i4         iso_level;
    i4         db_lockmode;
    i4         lk_list_id;
    i4         est_pages = DMC_C_SYSTEM;
    i4         total_pages = DMC_C_SYSTEM;
    STATUS	    cl_status;
    i4	 	    length;
    CL_ERR_DESC	    sys_err;
    LG_CRIB	    *curscrib;
    char	    *cribmem;
    LG_CRIB	    *crib;

    CLRDBERR(&dmt->error);

    status = E_DB_ERROR;
    for (;;)
    {
        if ((dmt->dmt_flags_mask & ~(DMT_DBMS_REQUEST | DMT_SHOW_STAT | 
               DMT_CONSTRAINT | DMT_SESSION_TEMP |
	       DMT_LK_ONLN_NEEDED | DMT_FORCE_NOLOCK | DMT_NO_LOCK_WAIT | 
	       DMT_CURSOR | DMT_CRIBPTR)) == 0)
	{
	    odcb = (DML_ODCB *)dmt->dmt_db_id;

	    /*
	    ** Make sure we have a ODCB at this point
	    */

	    if (dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB) == E_DB_OK)
	    {
		db_lockmode = DM2T_S;
		if (odcb->odcb_lk_mode == ODCB_X)
		    db_lockmode = DM2T_X;
		xcb = (DML_XCB *)dmt->dmt_tran_id;
		if (dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) == E_DB_OK)
		{
		    scb = xcb->xcb_scb_ptr;
		    /* Check for external interrupts */
		    if ( scb && scb->scb_ui_state && !(dmt->dmt_flags_mask & DMT_DBMS_REQUEST) )
			dmxCheckForInterrupt(xcb, &error);
		    if ( xcb->xcb_state )
		    {
			if (xcb->xcb_state & XCB_USER_INTR)
			    SETDBERR(&dmt->error, 0, E_DM0065_USER_INTR);
			else if (xcb->xcb_state & XCB_FORCE_ABORT)
			    SETDBERR(&dmt->error, 0, E_DM010C_TRAN_ABORTED);
			else if (xcb->xcb_state & XCB_ABORT)
			    SETDBERR(&dmt->error, 0, E_DM0064_USER_ABORT);
			else if (xcb->xcb_state & XCB_WILLING_COMMIT)
			    SETDBERR(&dmt->error, 0, E_DM0132_ILLEGAL_STMT);
		    }
		    else if (!scb)  
			SETDBERR(&dmt->error, 0, E_DM0061_TRAN_NOT_IN_PROGRESS);
		}
		else
		    SETDBERR(&dmt->error, 0, E_DM003B_BAD_TRAN_ID);
	    }
	    else
		SETDBERR(&dmt->error, 0, E_DM0010_BAD_DB_ID);
	}
	else
	    SETDBERR(&dmt->error, 0, E_DM001A_BAD_FLAG);
	if (dmt->error.err_code)
	    break;

	switch (dmt->dmt_access_mode)
	{
	    case    DMT_A_READ:
	    case    DMT_A_RONLY:
		access_mode = DM2T_A_READ;
		break;
	    case    DMT_A_WRITE:
		access_mode = DM2T_A_WRITE;
		break;
	    case    DMT_A_OPEN_NOACCESS:
		access_mode = DM2T_A_OPEN_NOACCESS;
		break;
	    case    DMT_A_SYNC_CLOSE:
		access_mode = DM2T_A_SYNC_CLOSE;
		break;
	    default:
		SETDBERR(&dmt->error, 0, E_DM002A_BAD_PARAMETER);
		break;
	}
	if (dmt->error.err_code)
	    break;

	/* Check for lock level, CRIB override (etabs) */
	if ( dmt->dmt_flags_mask & DMT_CRIBPTR )
	{
	    /*
	    ** Use the blob's CRIB information for
	    ** the etabs.
	    */
	    if ( !(crib = (LG_CRIB*)dmt->dmt_crib_ptr) )
	    {
		SETDBERR(&dmt->error, 0, E_DM002A_BAD_PARAMETER);
		break;
	    }

	    /* Reverse-engineer CRIB into dmt parameters for etab */
	    if ( crib->crib_rcb_state & RCB_CURSOR )
	    {
	        dmt->dmt_flags_mask |= DMT_CURSOR;
		dmt->dmt_cursid = crib->crib_cursid;
	    }
	    if ( crib->crib_rcb_state & RCB_CONSTRAINT )
	        dmt->dmt_flags_mask |= DMT_CONSTRAINT;

	    dmt->dmt_sequence = crib->crib_sequence;

	    if ( access_mode == DMT_A_WRITE )
	        dmt->dmt_lock_mode = DMT_MIX;
	    else
	        dmt->dmt_lock_mode = DMT_MIS;
	}

	if (dmt->dmt_lock_mode < DMT_MIX || dmt->dmt_lock_mode > DMT_X ||
            dmt->dmt_update_mode == 0 ||
            dmt->dmt_update_mode > DMT_U_DIRECT ||
            ((dmt->dmt_sequence == 0) 
                && (dmt->dmt_update_mode == DMT_U_DEFERRED)) )
	{
	    SETDBERR(&dmt->error, 0, E_DM002A_BAD_PARAMETER);
	    break;
	}

	/*
	** Check if try to open the table with access mode conflict to
	** the transaction type. 
	**
	** Temporary tables are immune to this test and are always writable.
	** Temp tables are identified by db_tab_base being negative.
	*/

	if ((i4)dmt->dmt_id.db_tab_base >= 0 &&
	     dmt->dmt_access_mode == DMT_A_WRITE &&
	     xcb->xcb_x_type & XCB_RONLY)
	{
	    xcb->xcb_state |= XCB_STMTABORT;
	    SETDBERR(&dmt->error, 0, E_DM006A_TRAN_ACCESS_CONFLICT);
	    break;
	}

	/*
	** Set default locking characteristics.
	*/
	timeout = DMC_C_SYSTEM;
        max_locks = DMC_C_SYSTEM;
        lock_level = DMC_C_SYSTEM;
	iso_level = DMC_C_SYSTEM;
	readlock = DMC_C_SYSTEM;
	est_pages = DMC_C_SYSTEM;
	total_pages = DMC_C_SYSTEM;

	/*
	** Check for overrides of locking defaults.  The following
	** characteristics can be overridden:
	**
	**	Lock mode
	**	Lock level
	**	Lock timeout
	**	Lock escalation threshold
	**      Lock readlock value
	**      Isolation level
	**
	** These characteristics can be specified either by the caller
	** in the dmt_char_array field of the dmt_cb, or by the user
	** by setting table or session level lockmode values.
	**
	** The order of precedence for locking values from lowest to highest is:
	**
	**	- default values
	**	- user supplied session level values
        **      - user supplied transaction isolation level
        **      - caller supplied table level values
	**	- user supplied table level values
	*/

	/*
	** Pick up user supplied session level locking values.
	*/
	if (dmt->dmt_access_mode != DMT_A_SYNC_CLOSE)
	{
	    timeout = scb->scb_timeout;
	    max_locks = scb->scb_maxlocks;
	    lock_level = scb->scb_lock_level;
	    iso_level = scb->scb_sess_iso;
	    readlock = scb->scb_readlock;
	}

        /*
        ** Pick up any SET TRANSACTION ISOLATION LEVEL setting.
        */
        if (xcb->xcb_flags & XCB_USER_TRAN && scb->scb_tran_iso != DMC_C_SYSTEM)
            iso_level = scb->scb_tran_iso;
 
	/*
	** Check for characteristics options specified by caller.
	** On an error, return since the dmt_cb error fields will	
	** have been set correctly in check_char.
	*/
	if (dmt->dmt_char_array.data_in_size &&
		         dmt->dmt_char_array.data_address)
	{
            if (check_char(dmt, &timeout, &max_locks, &lock_level, 
                     &iso_level, &est_pages, &total_pages))
                return (E_DB_ERROR);
	}

	/*
	** Check for user supplied table level locking values.
	*/
	if (scb->scb_kq_next != (DML_SLCB *)&scb->scb_kq_next &&
	    dmt->dmt_access_mode != DMT_A_SYNC_CLOSE)
	{
	   dmt_check_table_lock(scb, dmt, &timeout, &max_locks,
               &readlock, &lock_level);
	}

	/*
	** Unusual for READLOCK NOLOCK to be specified at table open time
	** But we need to let QEF do this for some DDL
	*/
	if (dmt->dmt_lock_mode == DMT_N)
	{
	    readlock = DMC_C_READ_NOLOCK;
#ifdef xDEBUG
	    TRdisplay("reading table %d,%d readlock nolock\n",
		dmt->dmt_id.db_tab_base,dmt->dmt_id.db_tab_index);
#endif
	}

        /*
        ** Set final locking characteristics.
        */
        lock_mode = dmt->dmt_lock_mode; 
        if (dmt_set_lock_values(dmt, &timeout, &max_locks, &readlock,
                   &lock_level, &iso_level, &lock_mode, est_pages, total_pages))
            return (E_DB_ERROR);

	if (dmt->dmt_flags_mask & DMT_FORCE_NOLOCK)
	{
	    lock_mode = dmt->dmt_lock_mode;
	    dmt->dmt_flags_mask &= ~DMT_FORCE_NOLOCK;
	}

	if (dmt->dmt_flags_mask & DMT_SESSION_TEMP)
	{
	    lk_list_id = scb->scb_lock_list;
	}
	else
	    lk_list_id = xcb->xcb_lk_id;

        if (dmt->dmt_flags_mask & DMT_NO_LOCK_WAIT)
        {
           /* Don't wait for lock if busy. */
           orig_timeout = timeout;
           timeout = DMC_C_NOWAIT;
        }
  
	if (dmt->dmt_flags_mask & DMT_LK_ONLN_NEEDED)
	{
	    LK_LOCK_KEY		lock_key;
	    CL_ERR_DESC		sys_err;

	    lock_key.lk_type = LK_ONLN_MDFY;
	    lock_key.lk_key1 = (i4)odcb->odcb_dcb_ptr->dcb_id;
	    lock_key.lk_key2 = dmt->dmt_id.db_tab_base;
	    lock_key.lk_key3 = dmt->dmt_id.db_tab_index;
	    lock_key.lk_key4 = 0;
	    lock_key.lk_key5 = 0;
	    lock_key.lk_key6 = 0;
	    status = LKrequest(odcb->odcb_dcb_ptr->dcb_bm_served == DCB_SINGLE ?
                LK_PHYSICAL | LK_LOCAL :
                LK_PHYSICAL, lk_list_id, &lock_key,
                LK_S, (LK_VALUE *)0, (LK_LKID *)0, timeout, &sys_err);
	    if (status != OK)
	    {
		/* FIX ME */
		return(status);
	    }	
	}

        /*
	**  Call physical layer to process the rest of the open.
        */
	status = dm2t_open(odcb->odcb_dcb_ptr, &dmt->dmt_id, lock_mode,
	    dmt->dmt_update_mode, access_mode, timeout, max_locks,
	    xcb->xcb_sp_id, xcb->xcb_log_id, lk_list_id, dmt->dmt_sequence,
	    iso_level, db_lockmode, &xcb->xcb_tran_id, &dmt->dmt_timestamp,
	    &rcb, scb, &dmt->error);
	if (status != E_DB_OK)
	    break;

	r = rcb;
	t = r->rcb_tcb_ptr;

        /* If the OPEN was done with DMT_NO_LOCK_WAIT, then rest the rcb timeout */
        if (dmt->dmt_flags_mask & DMT_NO_LOCK_WAIT)
        {
           r->rcb_timeout = orig_timeout;
        }

	/* Copy slog_id from XCB to RCB */
	r->rcb_slog_id_id = xcb->xcb_slog_id_id;

	/*
	** ANSI says constraint processing must be SERIALIZABLE. 
	**
	** In Ingres it is sufficient to use REPEATABLE READ lock protocols.
	** Constraint processing in Ingres is serializable because:
	** - Constraint processing is always done after the update
	** - When row locking, sessions will wait to read
	**   uncommitted rows/keys AND uncommitted deleted keys/rows.
	**
	** However, when using MVCC crow locking, the isolation level 
	** must be SERIALIZABLE so that changes made by the constrained
	** statement are visible to the constraint.
	**
	** Non-MVCC SERIALIZABLE lock protocols during constraint processing
	** will generate unecessary lock waits and deadlocks
	** for sessions updating different key values.
	*/
	if (dmt->dmt_flags_mask & DMT_CONSTRAINT)
	{
	    /* Note such in RCB */
	    r->rcb_state |= RCB_CONSTRAINT;

	    if ( row_locking(r) )
	    {
		if (r->rcb_iso_level == DMC_C_READ_UNCOMMITTED ||
			r->rcb_iso_level == DMC_C_READ_COMMITTED)
		    r->rcb_iso_level = DMC_C_REPEATABLE_READ;
	    }
	    else
	    {
		r->rcb_iso_level = DMC_C_SERIALIZABLE;
	    }
	}

	/* Note in RCB if table is opened for a cursor */
	if ( dmt->dmt_flags_mask & DMT_CURSOR )
	    r->rcb_state |= RCB_CURSOR;

        /*
        ** Temporary tables are always internal since they
        ** do not have security lablels or security logical keys.
        ** Also if this is an internal request for getting
        ** statistical information, do not want to
        */

        if ((dmt->dmt_flags_mask & DMT_SHOW_STAT) == 0 &&
             (r->rcb_tcb_ptr->tcb_temporary == 0))
        {
            r->rcb_internal_req = 0;
        }

	/*
	** Check for access conflict errors.
	** Do quick check to avoid MEcmp calls on most tables - this is a
	** quick solution, we should have a more general way to check version
	** numbers quickly - using numeric constants so we can check ranges of
	** values.
	*/
	if ((t->tcb_rel.relstat & TCB_VIEW) ||
	    ((t->tcb_rel.relstat & TCB_COMPRESSED) &&
                (t->tcb_rel.relcmptlvl == DMF_T0_VERSION ||
                 t->tcb_rel.relcmptlvl == DMF_T1_VERSION)))
        {
	    if (t->tcb_rel.relstat & TCB_VIEW)
		SETDBERR(&dmt->error, 0, E_DM009E_CANT_OPEN_VIEW);
	    else
		SETDBERR(&dmt->error, 0, E_DM0052_COMPRESSION_CONFLICT);
	    break;
	}

#ifdef	DMF_CALL_INTERFACE
	/*
	** Can't update an extended system catalog without special privilege.
        ** Can't update a security table without special privilege.
	*/

	if ((dmt->dmt_access_mode == DMT_A_WRITE) &&
	    (t->tcb_rel.relstat & TCB_CATALOG) &&
	    (t->tcb_rel.relstat & TCB_EXTCATALOG) &&
            ((scb->scb_s_type & SCB_S_SYSUPDATE) ==0))
	{
	    SETDBERR(&dmt->error, 0, E_DM005E_CANT_UPDATE_SYSCAT);
	    break;
	}

        if ((dmt->dmt_access_mode == DMT_A_WRITE) &&
            (t->tcb_rel.relstat & TCB_SECURE) &&
            ((scb->scb_s_type & SCB_S_SECURITY) ==0) &&
            (dmt->dmt_flags_mask & DMT_DBMS_REQUEST) == 0)
        {
	    SETDBERR(&dmt->error, 0, E_DM0129_SECURITY_CANT_UPDATE);
            break;
        }

#endif
	/*  Link RCB's of similar TCB's. */

	{
	    DML_XCB	    *next;

	    /*	Scan queue of open RCB's. */

	    for (next = (DML_XCB*) xcb->xcb_rq_next;
		next != (DML_XCB*) &xcb->xcb_rq_next;
		next = next->xcb_q_next)
	    {
		DMP_RCB	    *next_rcb;

		/*  Calculate RCB starting address. */

		next_rcb = (DMP_RCB *)(
		    (char *)next - ((char *)&((DMP_RCB*)0)->rcb_xq_next));

		/*  Check for same TCB. */

		if (next_rcb->rcb_tcb_ptr == t)
		{
		    DMP_RCB	    *save_next;

		    /*	Insert new RCB into the ring. */

		    save_next = next_rcb->rcb_rl_next;
		    next_rcb->rcb_rl_next = r->rcb_rl_next;
		    r->rcb_rl_next = save_next;
		    break;
		}
	    }
	}

	/*  Queue RCB to XCB. */

	r->rcb_xq_next = xcb->xcb_rq_next;
	r->rcb_xq_prev = (DMP_RCB *) &xcb->xcb_rq_next;
	xcb->xcb_rq_next->rcb_q_prev = (DMP_RCB*) &r->rcb_xq_next;
	xcb->xcb_rq_next= (DMP_RCB *) &r->rcb_xq_next;
	r->rcb_xcb_ptr = xcb;

	/* Set session ID for cancel checks, unless this is a factotum */
	r->rcb_sid = (CS_SID) 0;
	if ((scb->scb_s_type & SCB_S_FACTOTUM) == 0)
	    r->rcb_sid = scb->scb_sid;

	/* point replicator RCBs at the XCB */

	if (r->rep_shad_rcb)
	    r->rep_shad_rcb->rcb_xcb_ptr = xcb;
	if (r->rep_arch_rcb)
	    r->rep_arch_rcb->rcb_xcb_ptr = xcb;
	if (r->rep_shadidx_rcb)
	    r->rep_shadidx_rcb->rcb_xcb_ptr = xcb;
	if (r->rep_prio_rcb)
	    r->rep_prio_rcb->rcb_xcb_ptr = xcb;
	if (r->rep_cdds_rcb)
	    r->rep_cdds_rcb->rcb_xcb_ptr = xcb;

	if ( crow_locking(r) )
	{
	    /*
	    ** If blob etab table, use the same CRIB as the base
	    ** table, set by dmpe in dmt_crib_ptr and vetted earlier.
	    ** Otherwise, the transaction's CRIB is used by all RCBs
	    ** except read-committed cursors, each of which have
	    ** their own CRIB.
	    */

	    /* Use CRIB provided by caller */
	    if ( dmt->dmt_flags_mask & DMT_CRIBPTR )
		r->rcb_crib_ptr = crib;
	    else
		r->rcb_crib_ptr = xcb->xcb_crib_ptr;
	    
	    if ( r->rcb_state & RCB_CURSOR && r->rcb_iso_level == RCB_READ_COMMITTED )
	    {
		/*
		** read-committed cursors establish their CR point in time
		** when opened, and that CR point in time endures until the
		** cursor is closed. This allows non-cursor DML to be interleaved
		** with cursor DML without affecting the result set of the
		** cursor.
		*/
		curscrib = r->rcb_crib_ptr;

		/* If etab, this will match */
		if ( curscrib->crib_cursid != dmt->dmt_cursid )
		{
		    /*
		    ** Search for a CRIB matching this cursor.
		    **
		    ** QEF has provided a unambigous cursor id
		    ** in dmt_cursid. It's actually just a pointer
		    ** to the cursor's QEF DSH, but should suffice
		    ** as the id needs to be unique only in this
		    ** transaction's context.
		    */
		    for ( curscrib = r->rcb_crib_ptr->crib_next;
			  curscrib != r->rcb_crib_ptr &&
			      curscrib->crib_cursid != dmt->dmt_cursid;
			  curscrib = curscrib->crib_next );

		    /*
		    ** If no CRIB for this cursor, allocate one, link
		    ** it to the transaction's CRIB list of cursor CRIBs.
		    **
		    ** Any additional table opens on behalf of this cursor
		    ** will find this CRIB.
		    **
		    ** The cursor CRIB will be delinked and deallocated
		    ** when the RCB is closed and the CRIB inuse count
		    ** reaches zero.
		    */
		    if ( curscrib == r->rcb_crib_ptr )
		    {
			status = dm0m_allocate(sizeof(DM_OBJECT) + sizeof(LG_CRIB)
					    + dmf_svcb->svcb_xid_array_size,
					    0, (i4)CRIB_CB, (i4)CRIB_ASCII_ID,
					    (char*)xcb,
					    (DM_OBJECT**)&cribmem, &dmt->error);
			if ( status )
			    break;

			curscrib = (LG_CRIB*)(cribmem + sizeof(DM_OBJECT));
			curscrib->crib_xid_array =
			    (u_i4*)((char*)curscrib + sizeof(LG_CRIB));

			curscrib->crib_cursid = dmt->dmt_cursid;
			curscrib->crib_inuse = 0;
			curscrib->crib_log_id = xcb->xcb_log_id;

			/* Set CRIB to this point in time */
			cl_status = LGshow(LG_S_CRIB_BT, (PTR)curscrib,
					   sizeof(LG_CRIB), &length, &sys_err);
			if ( cl_status )
			{
			    dm0m_deallocate((DM_OBJECT**)&cribmem);
			    uleFormat(&dmt->error, cl_status, &sys_err, ULE_LOG, NULL,
				(char *)NULL, 0L, (i4 *)NULL, &error, 0);
			    break;
			}
			
			/* Link cursor crib to transaction's cursor crib list */
			curscrib->crib_next = r->rcb_crib_ptr->crib_next;
			curscrib->crib_prev = r->rcb_crib_ptr;
			r->rcb_crib_ptr->crib_next->crib_prev =
			    r->rcb_crib_ptr->crib_next = curscrib;

			/* Show statement on which the CRIB is based */
			curscrib->crib_sequence = dmt->dmt_sequence;
		    }
		}
		/* Count another use, stow CRIB pointer in RCB */
		curscrib->crib_inuse++;
		r->rcb_crib_ptr = curscrib;
	    }
	    else
	    {
		/*
		** If not a constraint and MVCC read_committed 
		** and this begins a new statement,
		** then establish the statement's CR point in time
		** within this transaction.
		**
		** If SERIALIZABLE, we retain the same CRIB snapshot
		** that was acquired at the start of the transaction,
		** but update crib_bos_lsn to record the LSN at the
		** start of this statement.
		**
		** NB: Parallel threads using shared log transactions
		**     share the same CRIB (see dmx_begin).
		*/
		if ( !(r->rcb_state & RCB_CONSTRAINT) &&
		      r->rcb_crib_ptr->crib_sequence != dmt->dmt_sequence )
		{
		    i4		lgShowFlag;

		    /* Provide a handle to LGshow */
		    r->rcb_crib_ptr->crib_log_id = xcb->xcb_log_id;

		    if ( r->rcb_iso_level == RCB_SERIALIZABLE )
			/* Just update crib_bos_lsn */
			lgShowFlag = LG_S_CRIB_BS_LSN;
		    else
		    {
			lgShowFlag = LG_S_CRIB_BS;
			/* Remember statement on which the CRIB is based */
			r->rcb_crib_ptr->crib_sequence = dmt->dmt_sequence;
		    }

		    cl_status = LGshow(lgShowFlag, (PTR)r->rcb_crib_ptr,
				       sizeof(LG_CRIB), &length, &sys_err);
		    if ( cl_status )
		    {
			uleFormat(&dmt->error, cl_status, &sys_err, ULE_LOG, NULL,
			    (char *)NULL, 0L, (i4 *)NULL, &error, 0);
			break;
		    }
		}
	    }
	    /*
	    ** Copy rcb_state to CRIB.
	    **
	    ** This remembers if statement is for a CURSOR or CONSTRAINT
	    ** so etabs can be opened with the same dmt flags.
	    */
	    r->rcb_crib_ptr->crib_rcb_state = r->rcb_state;
	}

	/* 
	** Save sequence number in xcb so that temporary tables can
	** use it if necessary (for large object temporaries).
	*/
	xcb->xcb_seq_no = dmt->dmt_sequence;

	/*
	** Set pointer to interrupt mask so record operations can quickly
	** determine if an interrupt has occurred.
	*/
	r->rcb_uiptr = &scb->scb_ui_state;

	/*
	** Reset record control block's adf cb to use the Front End
	** session's timezone.  This will cause adf comparisons to
	** behave as if they are done in the user's timezone rather
	** than in the server's timezone.
	** This was done as part of the DBMS Timezone changes.
	*/
	adf_cb = r->rcb_adf_cb;
	ses_adfcb = (ADF_CB *) scb->scb_adf_cb;
	adf_cb->adf_tzcb = scb->scb_tzcb;
	/* Shouldn't be null, but be safe */
	if (ses_adfcb != NULL)
	{
	    adf_cb->adf_dfmt = ses_adfcb->adf_dfmt;
	    adf_cb->adf_mfmt = ses_adfcb->adf_mfmt;
	    adf_cb->adf_decimal = ses_adfcb->adf_decimal;
	    adf_cb->adf_outarg = ses_adfcb->adf_outarg;
	    adf_cb->adf_exmathopt = ses_adfcb->adf_exmathopt;
	    adf_cb->adf_qlang = ses_adfcb->adf_qlang;
	    adf_cb->adf_nullstr = ses_adfcb->adf_nullstr;
	    adf_cb->adf_constants = ses_adfcb->adf_constants;
	    adf_cb->adf_slang = ses_adfcb->adf_slang;
	    adf_cb->adf_maxstring = ses_adfcb->adf_maxstring;
	    adf_cb->adf_strtrunc_opt = ses_adfcb->adf_strtrunc_opt;
	    adf_cb->adf_year_cutoff = ses_adfcb->adf_year_cutoff;
	    adf_cb->adf_collation = ses_adfcb->adf_collation;
	    adf_cb->adf_ucollation = ses_adfcb->adf_ucollation;
	    adf_cb->adf_uninorm_flag = ses_adfcb->adf_uninorm_flag;
	    adf_cb->adf_proto_level = ses_adfcb->adf_proto_level;
	    adf_cb->adf_unisub_status = ses_adfcb->adf_unisub_status;
	    MEcopy(&ses_adfcb->adf_unisub_char[0],
		sizeof(ses_adfcb->adf_unisub_char),
		&adf_cb->adf_unisub_char[0]);
	    /* Other stuff already set up */
	}
	/* RCB belongs to this thread, use scb's error message area */
	adf_cb->adf_errcb.ad_ebuflen = sizeof(scb->scb_adf_ermsg);
	adf_cb->adf_errcb.ad_errmsgp = &scb->scb_adf_ermsg[0];

	/*
	** Check for NoLogging state.
	** If session is running with No Logging, then update RCB appropriately.
	** Make sure we also update the RCBs for the replicator support tables
	*/
	if (scb->scb_sess_mask & SCB_NOLOGGING)
	{
	    rcb->rcb_logging = 0;
	    rcb->rcb_journal = 0;
	    if (rcb->rep_shad_rcb)
	    {
		rcb->rep_shad_rcb->rcb_logging = 0;
		rcb->rep_shad_rcb->rcb_journal = 0;
	    }
	    if (rcb->rep_arch_rcb)
	    {
		rcb->rep_arch_rcb->rcb_logging = 0;
		rcb->rep_arch_rcb->rcb_journal = 0;
	    }
	    if (rcb->rep_shadidx_rcb)
	    {
		rcb->rep_shadidx_rcb->rcb_logging = 0;
		rcb->rep_shadidx_rcb->rcb_journal = 0;
	    }
	    if (rcb->rep_prio_rcb)
	    {
		rcb->rep_prio_rcb->rcb_logging = 0;
		rcb->rep_prio_rcb->rcb_journal = 0;
	    }
	    if (rcb->rep_cdds_rcb)
	    {
		rcb->rep_cdds_rcb->rcb_logging = 0;
		rcb->rep_cdds_rcb->rcb_journal = 0;
	    }
	}

	/* 
	** Return # of records and # of pages as part of a successful DMT_OPEN,
	** which allows QEF to validate saved query plans dependant on size of 
	** relations without making DMT_SHOW calls every procedure invocation.
	**
	** dm2r_rcb_allocate() computed the "real-time" values for us and placed them
	** in our RCB.
	**
	** Make sure page/row counts are non-negative.
	** Inaccuracies in page/tuple counts could cause these
	** to become negative - which will cause OPF to die.
	*/

	dmt->dmt_record_count  = r->rcb_reltups;
	if (dmt->dmt_record_count < 0)
	    dmt->dmt_record_count = 0;

	dmt->dmt_page_count = r->rcb_relpages;
	if (dmt->dmt_page_count <= 0)
	    dmt->dmt_page_count = 1;

	dmt->dmt_record_access_id = (char *)r;

#ifdef xDEBUG
        r->rcb_xcb_ptr->xcb_s_open++;
#endif

	return (E_DB_OK);
    }
    
    if (rcb)
    {
	DB_STATUS	    local_status;
	DB_ERROR	    local_dberr;

	local_status = dm2t_close(rcb, 0, &local_dberr);
	if (local_status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, 
	    	    (char * )NULL, 0L, (i4 *)NULL, &local_error, 0);
	}
    }
    if (dmt->error.err_code > E_DM_INTERNAL)
    {
	uleFormat(&dmt->error, 0, NULL, ULE_LOG, NULL, 
		(char * )NULL, 0L, (i4 *)NULL, &local_error, 0);
	SETDBERR(&dmt->error, 0, E_DM008F_ERROR_OPENING_TABLE);
    }

    if (status == E_DB_OK)
	status = E_DB_ERROR;
    return (status);
}

/*{
** Name: check_char - check for caller supplied lock characteristics.
**
** Description:
**      Check the dmt_show_cb for caller supplied lock characteristics
**      to use during the table control lock request.
**
** Inputs:
**      dmt_shw_cb          - show request control block
**      timeout             - lock timeout value
**      max_locks           - max lock value 
**      lock_level          - lock granularity level
**      iso_level           - isolation level
** 
** Outputs:
**      timeout             - set to new timeout value if caller supplied value
**                            is found in the show request control block.
**      max_locks           - set to new value if appropriate.
**      lock_level          - set to new value if appropriate.
**	iso_level	    - set to new value if appropriate.
**      error.err_code      - set to error status:
**                                  E_DM000D_BAD_CHAR_ID
**      error.err_data      - set to parameter # of bad characteristic
**
**      Returns:
**         E_DB_OK                          Function completed normally.
**         E_DB_ERROR                       Bad control block entry.
**
** History:
**      21-jan-1991 (rogerk)
**          Created
**      10-oct-1991 (jnash)
**          Header added 
**	17-may-1995 (lewda02/sarjo01)
**	    RAAT API Project:
**	    Added control of lock duration and mode.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Replace lock_duration with iso_level, and DM2T_NOREADLOCK with
**          DMC_C_READ_UNCOMMITTED.
**      10-apr-97 (dilma04)
**          Set Lockmode Cleanup:
**          - removed lock_mode parameter because it is passed through DMT_CB;
**          - add explicit setting of lock level according to lock mode;
**          - set isolation level to SERIALIZABLE if lock mode != DMT_N.
**      28-may-97 (dilma04)
**          Added handling of DMT_RIS and DMT_RIX lock modes.
**	29-nov-2000 (shust01)
**	    Using RAAT, you can get serializable (SIX) page locks when running,
**	    instead of IX locks.  This causes other users to potentially get
**	    lock timeouts when trying to access the same page, when using
**	    row-level locking. (bug 103127, INGSRV 1306).
**      17-may-99 (stial01)
**          check_char() QEF may specify DMT_C_EST_PAGES, DMT_C_TOTAL_PAGES
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add support for MVCC lock level.
*/
static STATUS
check_char(
DMT_CB		*dmt_cb,
i4		*timeout,
i4		*max_locks,
i4         *lock_level,
i4         *iso_level,
i4              *est_pages,
i4              *total_pages)
{
    DMT_CB	*dmt = dmt_cb;
    DMT_CHAR_ENTRY  *char_entry =
			(DMT_CHAR_ENTRY *)dmt->dmt_char_array.data_address;
    i4	    char_count = dmt->dmt_char_array.data_in_size / 
				    sizeof(DMT_CHAR_ENTRY);
    i4         lock_mode = dmt->dmt_lock_mode;
    i4	    i;

    for (i = 0; i < char_count; i++)
    {
	switch (char_entry[i].char_id)
	{
	case DMT_C_TIMEOUT_LOCK:
	    *timeout = char_entry[i].char_value;
	    break;

	case DMT_C_PG_LOCKS_MAX:
	    *max_locks = char_entry[i].char_value;
	    break;

	case DMT_C_JOURNALED:
	    break;

	case DMT_C_LOCKMODE:
	    if (lock_mode == DMT_N)
		*iso_level = DMC_C_READ_UNCOMMITTED; 

            /* Set table lock_level value according to lock_mode
            ** to override session level setting.
	    ** Note that char value is ignored.  This characteristic
	    ** is sort of a "really use my dmt_lock_mode and don't piss
	    ** with it" instruction.
            */
            if (lock_mode >= DMT_S)
                *lock_level = DMC_C_TABLE;
            else if (lock_mode == DMT_RIS || lock_mode == DMT_RIX)
                *lock_level = DMC_C_ROW;
            else if ( lock_mode == DMT_MIS || lock_mode == DMT_MIX )
	        *lock_level = DMC_C_MVCC;
            else
                *lock_level = DMC_C_PAGE;
	    break;

	case DMT_C_EST_PAGES:
	    *est_pages = char_entry[i].char_value;
	    break;

	case DMT_C_TOTAL_PAGES:
	    *total_pages = char_entry[i].char_value;
	    break;

	default:
	    SETDBERR(&dmt->error, i, E_DM000D_BAD_CHAR_ID);
	    return (E_DB_ERROR);
	}
    }
    return (E_DB_OK);
}

/*{
** Name:  dmt_check_table_lock - Check user supplied table locking values 
**
** Description:
**      Check the "set lockmode" list for lock charachteristics values on
**      the table specified.
**
** Inputs:
**      scb             - session control block 
**      dmt             - table access control block
**      timeout         - lock timeout value
**      max_locks       - maxlocks value
**      readlock        - readlock value
**      lock_level      - lock granularity level
**
** Outputs:
**      timeout         - updated lock timeout value
**      max_locks       - updated maxlocks value
**      lock_level      - updated lock granularity level
**      readlock        - updated readlock level
** 
**      Returns:
**         none
**
** History:
**      12-feb-97 (dilma04)
**          Created. 
**      14-oct-97 (stial01)
**          dmt_check_table_lock() Changes to distinguish readlock value from 
**          isolation level. READLOCK may be specified on table basis,
**          isolation level cannot.
**	17-Aug-2005 (jenjo02)
**	    Delete db_tab_index from DML_SLCB. Lock semantics apply
**	    universally to all underlying indexes/partitions.
*/
VOID
dmt_check_table_lock(
DML_SCB         *scb,
DMT_CB          *dmt,
i4         *timeout,
i4         *maxlocks,
i4         *readlock,
i4         *lock_level)     
{
    DML_SLCB        *slcb;

    /*
    ** Loop through the slcb list looking for lockmode parameters
    ** that apply to this table. 
    ** If a lockmode value is SESSION, just leave the lockmode
    ** value alone and assume it was set to the right thing when
    ** the session level settings were processed.
    */

    for (slcb = scb->scb_kq_next;
        slcb != (DML_SLCB*) &scb->scb_kq_next;
        slcb = slcb->slcb_q_next)
    {
        /* Check if this SLCB applies to the current table. */
        if ( slcb->slcb_db_tab_base == dmt->dmt_id.db_tab_base )
        {
            if (slcb->slcb_timeout != DMC_C_SESSION)
                *timeout = slcb->slcb_timeout;
            if (slcb->slcb_maxlocks != DMC_C_SESSION)
                *maxlocks = slcb->slcb_maxlocks;
            if (slcb->slcb_lock_level != DMC_C_SESSION)
                *lock_level = slcb->slcb_lock_level;
            if (slcb->slcb_readlock != DMC_C_SESSION)
                *readlock = slcb->slcb_readlock;
	    break;
        } 
    }
}

/*{
** Name:  dmt_set_lock_values -  Sets locking characteristics for a table.
**
** Description: This routine first checks if default values should be assigned 
** to lock characteristics. Then it sets lockmode according to the lock level, 
** access mode and readlock protocol chosen.
**
** Inputs:
**      dmt_cb          - pointer to the table access control block 
**      timeout         - lock timeout value
**      max_locks       - maxlocks value
**      readlock        - readlock value
**      lock_level      - lock granularity level
**      iso_level       - isolation level
**      lock_mode       - table lock mode
**
** Outputs:
**      timeout         - updated lock timeout value
**      max_locks       - updated maxlocks value
**      readlock        - updated readlock value
**      lock_level      - updated lock granularity level
**      iso_level       - updated isolation level
**      lock_mode       - updated table lock mode
**
**      Returns:
**         E_DB_OK                          Function completed normally.
**         E_DB_ERROR                       Bad lock level value.
**
** History:
**      12-feb-97 (dilma04)
**          Created.
**	09-apr-97 (cohmi01)
**	    Allow readlock, timeout & maxlocks system defaults to be 
**	    configured. #74045, #8616.
**      10-apr-97 (dilma04)
**          Simplified lock_mode handling because of RAAT code change.
**      28-may-97 (dilma04)
**          Added handling of DMT_RIS and DMT_RIX lock modes.
**      13-jun-97 (dilma04)
**          To comply with the ANSI standart, enforce serializability for
**          system catalogs or if the operation on the table is associated 
**          with a constraint processing. 
**          If DM717 is set, force CS overriding higher isolation levels.
**          If DM718 is set, enforce RR isolation instead of serializable.
**      14-oct-97 (stial01)
**          dmt_set_lock_values() Changes to distinguish readlock value from 
**          isolation level.
**	10-nov-2000 (shust01)
**	    ifdef'd out a TRdisplay.  Can fill up the dbms.log with a message
**	    that is just informative.  This is a partial cross of a stial01
**	    change from main, #436735.  bug #103188.
**	12-nov-97 (stial01)
**	    Readonly cursor: check for isolation level READ UNCOMMITTED
**	07-apr-99 (nanpr01)
**	    For write cursor with READ UNCOMMITTED isolation, upgrade
**	    the isolation level to READ COMMITTED.
**      17-may-99 (stial01)
**          dmt_set_lock_values() New est_pages, total_pages parameters
**      07-jul-99 (stial01)
**          dmt_set_lock_values() Escalate only if isolation SERIALIZABLE
**      04-nov-99 (stial01)
**          dmt_set_lock_values() row locking for catalogs may be overridden
**      30-may-2000 (stial01)
**          Escalate lock mode for update cursors only if  est pages > maxlocks
**          Escalate only if lock mode = DMC_C_SYSTEM (startrak 8737602,b101794)
**      30-Jan-2002 (hanal04) Bug 111710 INGSRV2689
**          The above change stopped TABLE level LK_X locks being acquired by
**          an update that would hit all pages as the page locks may
**          be downgradable to LK_S locks. However, the lockmode
**          droped to PAGE level LK_IX locks and this introduced unexpected 
**          deadlocks when an update which hits every page sets PAGE level 
**          LK_IX locks and is followed by a select of the same data.
**          The select sets TABLE level LK_S locks which are a higher lock
**          mode than the update. Amended the above change so that the update
**          will acquire TABLE level LK_SIX locks. 
**	31-Oct-2006 (kschendel) SIR 122890
**	    Do better at maintaining serializable iso level for core catalogs.
**	9-Feb-2007 (kschendel) SIR 122890
**	    A lock mode request of DMT_X should imply table level locking,
**	    no matter what.  Callers only supply DMT_X when they think they
**	    are getting a true X table lock, otherwise they would supply
**	    DMT_IX and let this code figure it out.  If we don't obey
**	    the DMT_X request, and a LOAD operation is attempted, it fails.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add support for MVCC lock level.
**	19-Mar-2010 (jonj)
**	    SIR 121619 MVCC: Check lock mode MIX regardless of lock level.
**	24-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support: 
**	    Check explicit MVCC lock modes when lock_level is DMC_C_SYSTEM.
**	15-Apr-2010 (jonj)
**	    Don't let isolation level READ_UNCOMMITTED override
**	    lock level MVCC, change to READ_COMMITTED.
*/
STATUS
dmt_set_lock_values(
DMT_CB          *dmt,
i4         *timeout_pt,
i4         *max_locks_pt,
i4         *readlock_pt,
i4         *lock_level_pt,
i4         *iso_level_pt,
i4         *lock_mode_pt,
i4         est_pages,
i4         total_pages)
{
    i4         timeout    = *timeout_pt;
    i4         max_locks  = *max_locks_pt;
    i4         readlock   = *readlock_pt;
    i4         lock_level = *lock_level_pt;
    i4         iso_level  = *iso_level_pt;
    i4         lock_mode  = *lock_mode_pt;
    i4         err_code;
    i4         orig_lock_level = lock_level;

    /*
    ** Check if system default values should be assigned to lock characteristics
    */
    if (timeout == DMC_C_SYSTEM)
        timeout = dmf_svcb->svcb_lk_waitlock;  /* default system timeout */ 
    if (max_locks == DMC_C_SYSTEM)
        max_locks = dmf_svcb->svcb_lk_maxlocks;  /* default system maxlocks */
    if (readlock == DMC_C_SYSTEM)
	readlock = dmf_svcb->svcb_lk_readlock; /* default readlock */
    if (iso_level == DMC_C_SYSTEM)
	iso_level = dmf_svcb->svcb_isolation; /* default isolation level */
    if (lock_level == DMC_C_SYSTEM)
	lock_level = dmf_svcb->svcb_lk_level; /* default lock level */

    /*
    ** Unless user has overridden the default lock level...
    ** use ROW locking on catalogs whenever possible
    ** Do not change lock mode for SCONCUR catalogs, they use PHYSICAL
    ** page locking.  iisequence is really SCONCUR although it's not
    ** in the table number ordering.
    */
    if (lock_level == DMC_C_SYSTEM &&
        dmt->dmt_id.db_tab_base > DM_SCONCUR_MAX &&
	dmt->dmt_id.db_tab_base <= DM_SCATALOG_MAX &&
	dmt->dmt_id.db_tab_base != DM_B_SEQ_TAB_ID &&
	(lock_mode == DMT_IS || lock_mode == DMT_IX))
    {
	lock_level = DMC_C_ROW;
    }

    /* No row locking in core catalogs, ever */
    if (lock_level == DMC_C_ROW
      && (dmt->dmt_id.db_tab_base <= DM_SCONCUR_MAX
	  || dmt->dmt_id.db_tab_base == DM_B_SEQ_TAB_ID) )
    {
	lock_level = DMC_C_PAGE;
    }

    if (lock_level == DMC_C_SYSTEM)    
    {                                   
	/*
	** Default lock_level is the one that 
	** matches lock_mode provided by caller.
	*/
        if (lock_mode >= DMT_S)
            lock_level = DMC_C_TABLE;
        else if (lock_mode == DMT_RIS || lock_mode == DMT_RIX)
            lock_level = DMC_C_ROW;     
        else if (lock_mode == DMT_MIS || lock_mode == DMT_MIX)
            lock_level = DMC_C_MVCC;     
        else
            lock_level = DMC_C_PAGE;
    } 

    /* Caller request of DMT_X needs to always mean table locking */
    if (lock_mode == DMT_X)
	lock_level = DMC_C_TABLE;

    /*
    ** Unless user has overridden the default lock level...
    ** If page locking (only), see if we should escalate.
    ** No escalation test for SCONCUR catalogs that use physical locking.
    **
    ** Use the same criteria for read/write cursors - so that select after
    ** update does not get a deadlock. (b111710, b113732)
    **
    ** If est_pages == total pages (at optimization time)
    ** OR est_pages > maxlocks:
    **
    ** If REPEATABLE READ, only pages that qualify remain locked.
    **    Since we don't know at table open time if there is/is not
    **    a qualification, don't escalate at table open time.
    **    For now, just wait until we request MAXLOCKS locks to escalate
    **
    **    (If QEF were changed to send info about qualification at table open,
    **    escalate if there is NO qualification.)
    **    Qualifications are sent during the POSITION, at which time we could 
    **    escalate if scanning without any key or user qualification.
    **
    ** If READ COMMITTED, only current page remains locked, do not escalate
    **
    ** If READ UNCOMMITTED, no pages are locked, do not escalate
    */
    if (orig_lock_level == DMC_C_SYSTEM &&
	lock_level == DMC_C_PAGE &&
        est_pages != DMC_C_SYSTEM  &&
	est_pages != 0 &&
	total_pages != DMC_C_SYSTEM &&
	iso_level == DMC_C_SERIALIZABLE &&
	dmt->dmt_id.db_tab_base > DM_SCONCUR_MAX &&
	dmt->dmt_id.db_tab_base != DM_B_SEQ_TAB_ID)
    {
	if (est_pages > max_locks || est_pages == total_pages)
	    lock_level = DMC_C_TABLE;
    }

    /*
    ** Set table lockmode according to the lock level chosen.
    **
    ** See also dm2t_open().
    */
    if (lock_level == DMC_C_TABLE)
    {
        if (lock_mode == DMT_MIX || lock_mode == DMT_RIX ||
	    lock_mode == DMT_IX  || lock_mode == DMT_X)
	{
	    lock_mode = DM2T_X;
	}
        else 
            lock_mode = DM2T_S;
    }
    else if (lock_level == DMC_C_PAGE)
    {
        if (lock_mode == DMT_MIX || lock_mode == DMT_RIX ||
	    lock_mode == DMT_IX  || lock_mode == DMT_X)
	{
	    lock_mode = DM2T_IX;
	}
        else
	    lock_mode = DM2T_IS;
    }
    else if (lock_level == DMC_C_ROW)
    {
        if (lock_mode == DMT_MIX || lock_mode == DMT_RIX ||
	    lock_mode == DMT_IX  || lock_mode == DMT_X)
	{
            lock_mode = DM2T_RIX;
	}
        else
            lock_mode = DM2T_RIS;
    }
    else if (lock_level == DMC_C_MVCC)
    {
        if (lock_mode == DMT_MIX || lock_mode == DMT_RIX ||
	    lock_mode == DMT_IX  || lock_mode == DMT_X)
	{
            lock_mode = DM2T_MIX;
	}
        else
            lock_mode = DM2T_MIS;
    }
    else
        return (E_DB_ERROR);

    /*
    ** Check for readlock protocol
    ** Readonly cursor: check for readlock=nolock, readlock=exclusive
    **
    ** Ignore readlock=nolock if MVCC.
    */
    if (dmt->dmt_access_mode == DMT_A_READ)
    {
	if (readlock == DMC_C_READ_NOLOCK)
	{
	    if ( lock_level != DMC_C_MVCC && !dmt->dmt_mustlock )
		lock_mode = DM2T_N;
	}
	else if (readlock == DMC_C_READ_EXCLUSIVE)
	{
	    lock_mode = DM2T_IX;
	    if (lock_level == DMC_C_MVCC)
		lock_mode = DM2T_MIX;
	    if (lock_level == DMC_C_ROW)
		lock_mode = DM2T_RIX;
	    else if (lock_level == DMC_C_TABLE)
		lock_mode = DM2T_X;
	}
    }

    if (DMZ_TBL_MACRO(17))
    {
	iso_level = DMC_C_READ_COMMITTED;
    }
    else if (DMZ_TBL_MACRO(18))
    {
	iso_level = DMC_C_REPEATABLE_READ;
    }

    if (iso_level != DMC_C_SERIALIZABLE
      && dmt->dmt_id.db_tab_base <= DM_SCATALOG_MAX)
    {
	/* Maintain serializable for physical-locked core catalogs, RR for
	** others.  (RR seems to work with sconcur given a fix for CSRR
	** physical lock conversion, but who knows what other assumptions
	** core catalogs make -- and RR is mostly for allowing row locking,
	** which core catalogs don't use anyway.)
	**
	** Temp tables are -not- catalogs
	*/
	if ( dmt->dmt_id.db_tab_base > 0 )
	{
	    if (dmt->dmt_id.db_tab_base <= DM_SCONCUR_MAX
	      || dmt->dmt_id.db_tab_base == DM_B_SEQ_TAB_ID)
		iso_level = DMC_C_SERIALIZABLE;
	    else
		iso_level = DMC_C_REPEATABLE_READ;
	}
    }

    if ((iso_level == DMC_C_READ_UNCOMMITTED) && 
	(dmt->dmt_access_mode == DMT_A_WRITE))
	iso_level = DMC_C_READ_COMMITTED;

    /*
    ** Readonly cursor: check for isolation level READ UNCOMMITTED
    **
    ** If lock level is MVCC, isolation READ UNCOMMITTED cannot
    ** override MVCC semantics, change to READ COMMITTED.
    */
    if (dmt->dmt_access_mode == DMT_A_READ && 
	iso_level == DMC_C_READ_UNCOMMITTED &&
	dmt->dmt_mustlock != TRUE)
    {
	if ( lock_level == DMC_C_MVCC )
	    iso_level = DMC_C_READ_COMMITTED;
	else
	    lock_mode = DM2T_N;
    }

    /*
    ** Open iitree with maximum number of page locks set to at least 50.
    */
    if (dmt->dmt_id.db_tab_base == DM_B_TREE_TAB_ID &&
        max_locks < (i4) 50)
    {
        max_locks = (i4) 50;
    }
  
    *timeout_pt    = timeout;
    *max_locks_pt  = max_locks;
    *lock_level_pt = lock_level;
    *readlock_pt   = readlock;
    *iso_level_pt  = iso_level;
    *lock_mode_pt  = lock_mode;

    if (lock_mode == DM2T_N && iso_level != DMC_C_READ_UNCOMMITTED)
    {
	if (dmf_svcb->svcb_log_err & SVCB_LOG_READNOLOCK)
	{
	    uleFormat(NULL, E_DM9065_READNOLOCK_READUNCOMMITTED, (CL_ERR_DESC *)NULL,
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
		&err_code, 0);
	}
    }
    return (E_DB_OK);
}
