/* Copyright (c) 1995, 2005 Ingres Corporation
**
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <pc.h>
#include    <tm.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <lk.h>
#include    <lg.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmccb.h>
#include    <dmxcb.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dml.h>
#include    <dm2t.h>
#include    <dm2r.h>
#include    <sxf.h>
#include    <dma.h>
#include    <dmxe.h>
#include    <dm1b.h>
#include    <dm0l.h>

/**
** Name:  DMTALTER.C - Implement the DMF alter table operation.
**
** Description:
**      This file contains the function that implements the DMF
**      alter table operation.
**      This file defines:
**
**      dmt_alter - Alter table information.
**
** History:
**      26-feb-86 (jennifer)
**          Created.
**      16-may-86 (jennifer)
**          Modified dmt_alter to check for valid ODCB and XCB.
**      16-may-86 (jennifer)
**          Call to dm2t_open changed.
**      18-may-86 (jennifer)
**          Modified dmt_alter to allow a view query id to be set
**          in relation record for the view.
**      18-may-86 (jennifer)
**           Call to dm2t_open changed to include a db_lockmode
**           parameter used for optimizing access to databases
**           locked exclusively.
**      03-sep-86 (jennifer)
**           Fix dmt_alter to set TCB_VBASE instead of TCB_VIEW
**           when DMT_C_VIEWS is altered.
**      03-dec-86 (jennifer)
**           Change the DMT_C_PROTECTION bits to two separate fields.
**      13-feb-87 (jennifer)
**          Updated code for error handling.
**      17-feb-87 (jennifer)
**          Changed timestamp when table is altered.
**	19-jun-87 (rogerk)
**	    Allow alter on syscats, look for journaling flag.
**	22-jun-87 (rogerk)
**	    Added logging of alter.
**	30-sep-1988 (rogerk)
**	    Added capability to delay writing Begin Transaction record until
**	    first write operation.  If DELAYBT flag is set in the XCB, then
**	    call dmxe to write the BT record.
**	17-mar-1989 (EdHsu)
**	    Added cluster online backup support.
**	26-mar-89 (neil)
**          Rules: Added ability to turn on/off the "rules" relstat bit.
**	6-June-1989 (ac)
**	    Added 2PC support.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.  Disallow "set journaling"
**	    operation on gateway tables and all secondary index tables.
**	17-jan-1990 (rogerk)
**	    Add support for not writing savepoint records in readonly xacts.
**	20-jun-1990 (linda, bryanp)
**	    Add code to close rcb if we have reached the error condition with
**	    an open rcb.
**	25-jun-1990 (linda)
**	    Initialize rcb to 0 so we don't try to close one that isn't real.
**       4-feb-1991 (rogerk)
**          Add support for Set Nologging.
**      15-mar-1991 (bryanp)
**          Bug #36199:
**          Fixed my fix of 20-jun: After closing the RCB, we must set the
**          'rcb' variable to 0 so that we don't attempt to close it again in
**          the error-cleanup code.
**      10-oct-1991 (jnash) merged 13-june-89 (jennifer)
**          For B1, don't update relation tuple when a view
**          is being defined.  A base table will always indicate
**          a view exists, so that views can combine tables
**          with different lables.
**	10-oct-1991 (jnash) merged 13-feb-90 (carol)
**	    Added ability to turn on the "comments" relstat bit.
**	10-oct-1991 (jnash) merged 23-apr-90 (andre)
**	    Added ability to turn on the "synonyms" relstat bit.
**	10-oct-1991 (jnash) merged 24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	16-jan-1992 (bryanp)
**	    B41659,B41669(?): Take Table Control Lock in dmt_alter().
**	3-feb-1992 (bryanp)
**	    Disallow all dmt_alter calls on a temporary table.
**	8-jul-1992 (walt)
**	    Prototyping DMF.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	01-dec-1992 (jnash)
**	    Reduced logging project.  New params to dm0l_alter.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	19-nov-1992 (robf)
**	    Replace DMF auditing by SXF auditing. Audit significant changes
**	    to the table (SAVE date, JOURNALING status change).
**	16-feb-1993 (rogerk)
**	    Changed table-in-use check to look at tcb_open_count, not the
**	    tcb_ref_count.  The tcb may legally be referenced by DBMS
**	    background threads.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	28-may-1993 (robf)
**	   Secure 2.0: Reworked old ORANGE code.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	23-aug-1993 (rogerk)
**	    Changed alter log record to be journaled.
**	    Changes to handling of non-journaled tables.  ALL system catalog 
**	    updates in a journaled database are now journaled regardless of
**	    whether or not the user table being affected is.
**	    Took out setting of the rcb_journal state dependent upong the
**	    journal state of this table.
**	20-sep-1993 (rogerk)
**	    Final handling for non-journaled tables.
**	18-oct-1993 (rogerk)
**	    Add check for an attempt to alter a table already open by the user.
**	31-jan-1994 (bryanp) B58530, B58531, B58532
**	    Check dm2t_close status when returning TABLE_ACCESS_CONFLICT
**	    Check status from dma_write_audit.
**	    Check status from dm2t_close(rel_rcb) during error cleanup.
**	    Set rel_rcb to 0 after closing iirelation so that we don't try to
**		close it a second time during error cleanup.
**      14-mar-94 (stephenb)
**          Add new parameter to dma_write_audit() to match current prototype.
**      17-feb-95 (chech02) bug# 66569
**          Get tiemout from dm2t_get_timeout() and then pass timeout to
**          dm2t_control() and dm2t_open().
**	14-aug-96 (somsa01)
**	    Added setting of JOURNAL (or JON) bits for etab tables if the base
**	    table was being modified by the SET command in this way, and if
**	    the base table contained blob columns.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_DM006A_TRAN_ACCESS_CONFLICT.
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	17-dec-1999 (gupsh01) Moved the error checking for readonly 
**	    data-bases to correct position.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to dma_write_audit() if not C2SECURE.
**      02-jan-2004 (stial01)
**          Added support for SET [NO]BLOBJOURNALING ON table.
**      13-may-2004 (stial01)
**          Removed support for SET [NO]BLOBJOURNALING ON table. 
**      29-nov-2005 (horda03) Bug 48659/INGSRV 2716
**          A TX can now be started with a non-journaled BT record. This
**          leaves the XCB_DELAYBT flag set but sets the XCB_NONJNL_BT
**          flag. Need to write a journaled BT record if the update
**          will be journaled.
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2t_?, dm2r_? functions converted to DB_ERROR *
**	08-Dec-2008 (jonj)
**	    SIR 120874: dma_? auditing functions converted to DB_ERROR *
**/

/*{
** Name: dmt_alter - Alter information about a table.
**
**  INTERNAL DMF call format:  status = dmt_alter(&dmt_cb);
**
**  EXTERNAL call format:      status = dmf_call(DMT_ALTER, &dmt_cb);
**
** Description:
**      The alter table operation allows the caller to change specific system
**      table information about a table.  The table must be closed for alter to
**      work.  Only the following information can be altered:
**
**            Indication of existence of Permits on a table.
**            Indication of existence of Integrities on a table.
**            Indication of existence of Rules on a table.
**            Indication of existence of Views on a table.
**            Indication of existence of Optimizer statistics on a table.
**	      Indication of whether table is journaled.
**            The table savedate.
**	      Indication that Comments have ever existed on a table, or
**		on a column of that table.
**	      Indication that Synonyms have ever existed on a table.
**
**                  
**      At this time no other information can be altered.
**	This function may also be invoked to change table's timestamp without
**	changing any other table characteristics.
**
**	No dmt_alter() calls are currently supported for temporary tables.
**
** Inputs:
**      dmt_cb
**          .type                           Must be set to DMT_TABLE_CB.
**          .length                         Must be at least
**                                          sizeof(DMT_TABLE_CB) bytes.
**          .dmt_db_id                      Open database id retruned
**                                          from DMC_OPEN_DB operation.
**                                          Database to search.
**          .dmt_tran_id                    Transaction to associate with.
**          .dmt_id                         Internal table identifier.
**          .dmt_flags_mask                 Must be zero or DMT_TIMESTAMP_ONLY
**          .dmt_lock_mode                  Not used for alter.
**          .dmt_update_mode                Not used for alter.
**          .dmt_access_mode                Not used for alter.
**          .dmt_char_array.data_address    Optional pointer to characteristic
**                                          array of type DMT_CHAR_ENTRY.
**                                          See below for description of
**                                          <dmt_char_array> entries.
**					    MUST be NULL if
**					    (dmt_flags_mask&DMT_TIMESTAMP_ONLY)
**          .dmt_char_array.data_in_size    Length of dmt_char_array in bytes.
**					    Must be 0 if
**					    (dmt_flags_mask&DMT_TIMESTAMP_ONLY)
**
**          <dmt_char_array> entries of type DMT_CHAR_ARRAY and
**          must have the following format:
**              .char_id                    The characteristics identifer
**                                          denoting
**                                          the characteristics to set.
**                                          For alter these must be one of the
**                                          following:
**                                          DMT_C_VIEWS, DMT_C_PERMITS,
**                                          DMT_C_INTEGRITIES, DMT_C_ZOPSTATS,
**                                          DMT_C_SAVEDATE, DMT_C_PROALL,
**                                          DMT_C_RETALL, DMT_C_JOURNALED
**					    DMT_C_RULE
**              .char_value                 The value to use to set this 
**                                          characteristic.
**                                          For alter this will be either
**                                          DMT_C_ON or DMT_C_OFF for all
**                                          but DMT_C_SAVEDATE which contains
**                                          an i4 value indicating the date.
**
** Outputs:
**      dmt_cb
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM000D_BAD_CHAR_ID
**                                          E_DM0010_BAD_DB_ID
**                                          E_DM001A_BAD_FLAG
**                                          E_DM002A_BAD_PARAMETER
**                                          E_DM003B_BAD_TRAN_ID
**                                          E_DM0042_DEADLOCK
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**					    E_DM004D_LOCK_TIMER_EXPIRED
**                                          E_DM0054_NONEXISTENT_TABLE
**                                          E_DM005D_TABLE_ACCESS_CONFLICT
**                                          E_DM0064_USER_ABORT
**                                          E_DM0065_USER_INTR
**	    				    E_DM006A_TRAN_ACCESS_CONFLICT
**                                          E_DM010A_ERROR_ALTERING_TABLE
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
**      26-feb-86 (jennifer)
**          Created.
**      16-may-86 (jennifer)
**          Modified dmt_alter to check for valid ODCB and XCB.
**      16-may-86 (jennifer)
**          Call to dm2t_open changed.
**      18-may-86 (jennifer)
**          Modified dmt_alter to allow a view query id to be set
**          in relation record for the view.
**      18-may-86 (jennifer)
**           Call to dm2t_open changed to include a db_lockmode
**           parameter used for optimizing access to databases
**           locked exclusively.
**      03-sep-86 (jennifer)
**           Fix dmt_alter to set TCB_VBASE instead of TCB_VIEW
**           when DMT_C_VIEWS is altered.
**      03-dec-86 (jennifer)
**           Change the DMT_C_PROTECTION bits to two separate fields.
**      13-feb-87 (jennifer)
**           Update code for new error handling.
**      17-feb-87 (jennifer)
**          Changed timestamp when table is altered.
**	19-jun-87 (rogerk)
**	    Don't return TABLE_ACCESS_CONFLICT when trying to alter iirelation,
**	    iiattribute, iiindexes, or iirel_idx if the table reference count
**	    is greater than 1.
**	19-jun-87 (rogerk)
**	    Look for attribute DMT_C_JOURNALED and set journal bit accordingly.
**	    Don't allow journaling to be set on iirelation, iiattribute,
**	    iiindexes, or iirel_idx.
**	22-jun-87 (rogerk)
**	    Added call to dm0l_alter to log alter action.  This is so the TCB
**	    of the altered table can be tossed during backout.
**	14-aug-87 (rogerk)
**	    Set the TCB_JON flag instead of the TCB_JOURNALED flag when 
**	    journaling is turned on for a table.  Don't set the
**	    rel_rcb->rcb_journaled flag on since journaling is not enabled
**	    until the next checkpoint.
**	30-sep-1988 (rogerk)
**	    Added capability to delay writing Begin Transaction record until
**	    first write operation.  If DELAYBT flag is set in the XCB, then
**	    call dmxe to write the BT record.
**	    Consolidated some of the error handling code.
**	10-feb-1989 (rogerk)
**	    Update TCB if altering an SCONCUR catalog since the TCB is not
**	    purged and the changes are not normally updated in the TCB until
**	    the server is brought down.
**	17-mar-1989 (EdHsu)
**	    Added cluster online backup support.
**	26-mar-89 (neil)
**          Rules: Added capability to turn TCB_RULE on/off.  This is done
**	    exactly like integrities. Removed 2 unused variables.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.  Disallow "set journaling"
**	    operation on gateway tables and all secondary index tables.
**	17-jan-1990 (rogerk)
**	    Add support for not writing savepoint records in readonly xacts.
**	    Pass xcb argument to dmxe_writebt routine.
**	20-jun-1990 (linda, bryanp)
**	    Add code to close rcb if we have reached the error condition with
**	    an open rcb.
**	25-jun-1990 (linda)
**	    Initialize rcb to 0 so we don't try to close one that isn't real.
**	25-jun-1990 (linda)
**	    Change access code at dm2t_open() call from DM2T_A_WRITE to
**	    DM2T_A_MODIFY (meaning, open underlying file if possible but if
**	    not, it's a user-type error -- relevant for gateway tables only,
**	    allows users to create a view on a registered table even if the
**	    foreign data file does not exist).
**       4-feb-1991 (rogerk)
**          Add support for Set Nologging.  If transaction is Nologging, then
**          mark table-opens done here as nologging.  Also, don't write
**          Alter log record.
**      15-mar-1991 (bryanp)
**          Bug #36199:
**          Fixed my fix of 20-jun: After closing the RCB, we must set the
**          'rcb' variable to 0 so that we don't attempt to close it again in
**          the error-cleanup code.
**      10-oct-1991 (jnash) merged 13-june-89 (jennifer)
**          For B1, don't update relation tuple when a view
**          is being defined.  A base table will always indicate
**          a view exists, so that views can combine tables
**          with different lables.
**      10-oct-1991 (jnash) merged 13-feb-90 (carol)
**          Added ability to turn on the "comments" relstat bit.
**      10-oct-1991 (jnash) merged 23-apr-90 (andre)
**          Added ability to turn on/off the "synonyms" relstat bit.
**      10-oct-1991 (jnash) merged 14-jun-90 (andre)
**          Made changes to allow calling dmt_alter() to change a timestamp
**          alone.
**	16-jan-1992 (bryanp)
**	    B41659,B41669(?): Take Table Control Lock in dmt_alter().
**	3-feb-1992 (bryanp)
**	    Disallow all dmt_alter calls on a temporary table.
**	01-dec-1992 (jnash)
**	    Reduced logging project.  New param to dm0l_alter.
**	16-feb-1993 (rogerk)
**	    Changed table-in-use check to look at tcb_open_count, not the
**	    tcb_ref_count.  The tcb may legally be referenced by DBMS
**	    background threads.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	28-may-1993 (robf)
**	   Secure 2.0: Reworked old ORANGE code.
**	23-aug-1993 (rogerk)
**	    Changed alter log record to be journaled.
**	    Changes to handling of non-journaled tables.  ALL system catalog 
**	    updates in a journaled database are now journaled regardless of
**	    whether or not the user table being affected is.
**	    Took out setting of the rcb_journal state dependent upong the
**	    journal state of this table.
**	20-sep-1993 (rogerk)
**	    Final handling for non-journaled tables.  System catalog updates
**	    associated with non-journaled tables are now journaled for DROP
**	    and ALTER operations.  The journal direction is given by setting
**	    the rcb_usertab_jnl field in the system catalog rcb.  Made sure
**	    that the field is initialized for all types of alters.
**	14-oct-93 (robf)
**         If MAC access fails, reset rcb after the dm2t_close() so we
**	   don't close the table twice.
**	18-oct-1993 (rogerk)
**	    Move up check for table reference count to above the close-purge
**	    request.  The close-purge will hang if the table is open by
**	    the current session.
**	18-oct-93 (stephenb)
**	    Changed call to dma_write_audit() to test for SAVE (sd_specified)
**	    and audit SXF_A_CONTROL instead of SXF_A_ALTER.
**	8-dec-93 (robf)
**         Add support for DMT_C_ALARM for security alarms on tables.
**	31-jan-1994 (bryanp) B58530, B58531, B58532
**	    Check dm2t_close status when returning TABLE_ACCESS_CONFLICT
**	    Check status from dma_write_audit.
**	    Check status from dm2t_close(rel_rcb) during error cleanup.
**	    Set rel_rcb to 0 after closing iirelation so that we don't try to
**		close it a second time during error cleanup.
**	23-may-94 (robf)
**	    When checking access pass SXF_A_CONTROL rather than SXF_A_ALTER
**	    to dma_table_access(). This results is more consistent auditing
**	    and less confusion. 
**	14-aug-96 (somsa01)
**	    Added setting of JOURNAL (or JON) bits for etab tables if the base
**	    table was being modified by the SET command in this way, and if
**	    the base table contained blob columns.
**	03-jan-1997 (nanpr01)
**	    Use the #define values rather than hard-coded constants.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction 
**	    access mode> (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_DM006A_TRAN_ACCESS_CONFLICT.
**	30-jun-1997 (nanpr01)
**	    Extended relation catalogs only gets created after the first
**	    BLOB column is inserted. If it is not, set journaling will
**	    get error. I was tempted to change the createdb to create
**	    this catalogue and resolving this problem.
**	01-oct-1998 (nanpr01)
**	    Added the new parameter in dm2r_replace for update performance 
**	    enhancement.
**      17-dec-1999 (gupsh01) Moved the error checking for readonly 
**	    data-bases to correct position.
**	07-Jun-2005 (inifa01) b111211 INGSRV2580
**          Verifydb reports error; S_DU1619_NO_VIEW iirelation indicates that
**          there is a view defined for table a (owner ingres), but none exists,
**          when view(s) on a table are dropped.
**          Were making DMT_ALTER call on DMT_C_VIEW, to turn on iirelation.relstat
**          TCB_VBASE bit, indicating view is defined over table(s).  TCB_VBASE
**          not tested anywhere in 2.6 and not turned off when view is dropped.
**          qeu_cview() call to dmt_alter() to set bit has been removed ,
**          removeing references to TCB_VBASE & DMT_C_VIEW here.
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
**	23-nov-2006 (dougi)
**	    Replace DMT_C_VIEWS logic.
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
**	24-Jul-2009 (smeke01) b122317
**	    Allow processing of partitions. Specifically enable journalling 
**	    state to be flagged on iirelation records for partitions.
**	11-Nov-2009 (smeke01) b113524
**	    Enable journalling state to be flagged on iirelation records 
**	    for indexes (including primary key and foreign key constraints
**	    since these are implemented as indexes).
*/
DB_STATUS
dmt_alter(
DMT_CB	    *dmt_cb)
{
    DMT_CB	    *dmt = dmt_cb;
    DML_XCB	    *xcb;
    DMP_RCB	    *rcb = 0;
    DML_ODCB	    *odcb;
    DMT_CHAR_ENTRY  *char_entry;
    i4	    char_count;
    i4	    error, local_error;
    DB_STATUS	    status;
    i4	    i;
    i4         proall= -1;    
    i4         retall= -1;    
    i4	    permits = -1;
    i4	    integrities = -1;
    i4	    rules = -1;
    i4	    views = -1;
    i4	    zoptstats= -1;
    i4	    journaling = -1;
    i4	    commentson = -1;
    i4	    synonymson = -1;
    i4	    alarmson = -1;
    i4	    sd_specified = 0;
    i4	    save_date;
    i4         qid_specified = 0;
    i4		    relupdate = FALSE;
    DB_QRY_ID       qid;
    i4         ref_count;
    DB_TAB_TIMESTAMP timestamp;
    DB_TAB_ID       table_id;
    DMP_RELATION    relrecord;
    DM_TID          tid;
    DM2R_KEY_DESC   key_desc[2];
    DMP_RCB         *rel_rcb = 0;
    i4         db_lockmode;
    LG_LSN	    lsn;
    i4		    sconcur;
    i4		    jparm;
    i4		    index;
    i4		    gateway;
    DB_TAB_NAME	    table_name;
    DB_OWN_NAME	    table_owner;
    i4	    msgid=0;
    i4	    journal_flag;
    i4         timeout;
    DML_SCB         *scb;
    i4		    has_extensions = 0;
    DB_ERROR	    local_dberr;

    qid.db_qry_high_time = 0;
    qid.db_qry_low_time = 0;

    CLRDBERR(&dmt->error);

    status = E_DB_ERROR;
    for (;;)
    {
	if (dmt->dmt_flags_mask && ~dmt->dmt_flags_mask & DMT_TIMESTAMP_ONLY)
	{
	    SETDBERR(&dmt->error, 0, E_DM001A_BAD_FLAG);
	    break;
	}

	odcb = (DML_ODCB *)dmt->dmt_db_id;
	if (dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB) != E_DB_OK)
	{
	    SETDBERR(&dmt->error, 0, E_DM0010_BAD_DB_ID);
	    break;
	}

	db_lockmode = DM2T_S;
	if (odcb->odcb_lk_mode == ODCB_X)
	    db_lockmode = DM2T_X;

	xcb = (DML_XCB *)dmt->dmt_tran_id;
	if (dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) != E_DB_OK)
	{
	    SETDBERR(&dmt->error, 0, E_DM003B_BAD_TRAN_ID);
	    break;
	}

	/* Check for external interrupts */
	if ( xcb->xcb_scb_ptr->scb_ui_state )
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
	    break;
	}

	if (odcb != xcb->xcb_odcb_ptr)
	{
	    SETDBERR(&dmt->error, 0, E_DM005D_TABLE_ACCESS_CONFLICT);
	    break;
	}

	/*
	** If this is the first write operation for this transaction,
	** then we need to write the begin transaction record.
	*/
	if (xcb->xcb_flags & XCB_DELAYBT)
	{
            /* Write BT record don't know if following updates are
            ** journaled, so have to be safe.
            */
	    status = dmxe_writebt(xcb, TRUE, &dmt->error);
	    if (status != E_DB_OK)
	    {
		xcb->xcb_state |= XCB_TRANABORT;
		break;
	    }
	}

	/*
	** data_address and data_in_size must be both non-zero unless
	** dmt_flags_mask & DMT_TIMESTAMP_ONLY, in which case they both must be
	** 0.
	*/
	if (~dmt->dmt_flags_mask & DMT_TIMESTAMP_ONLY &&
	    dmt->dmt_char_array.data_address == 0
	    ||
	    dmt->dmt_flags_mask & DMT_TIMESTAMP_ONLY  &&
	    dmt->dmt_char_array.data_in_size != 0
	    ||
	    dmt->dmt_char_array.data_address != 0     &&
	    dmt->dmt_char_array.data_in_size == 0
	   )
	{
	    SETDBERR(&dmt->error, 0, E_DM002A_BAD_PARAMETER);
	    break;
	}

	/*  Check for characteristics. */

	char_entry = (DMT_CHAR_ENTRY *)dmt->dmt_char_array.data_address;
	char_count = dmt->dmt_char_array.data_in_size / sizeof(DMT_CHAR_ENTRY);
	for (i = 0; i < char_count; i++)
	{
	    switch (char_entry[i].char_id)
	    {
	    case DMT_C_VIEWS:
		views = char_entry[i].char_value;
		continue;
	    case DMT_C_INTEGRITIES:
		integrities = char_entry[i].char_value;
		continue;
	    case DMT_C_PERMITS:
		permits = char_entry[i].char_value;
		continue;
	    case DMT_C_RULE:
		rules = char_entry[i].char_value;
		continue;
	    case DMT_C_ALARM:
		alarmson = char_entry[i].char_value;
		continue;
	    case DMT_C_COMMENT:
		commentson = char_entry[i].char_value;
		continue;
	    case DMT_C_SYNONYM:
		synonymson = char_entry[i].char_value;
		continue;
	    case DMT_C_ZOPTSTATS:
		zoptstats = char_entry[i].char_value;
		continue;
	    case DMT_C_ALLPRO:
		proall = char_entry[i].char_value;
		continue;
	    case DMT_C_RETPRO:
		retall = char_entry[i].char_value;
		continue;
	    case DMT_C_SAVEDATE:
		save_date = char_entry[i].char_value;
		sd_specified++;
		continue;
	    case DMT_C_HQID_VIEW:
		qid.db_qry_high_time = char_entry[i].char_value;
		qid_specified++;
		continue;
	    case DMT_C_LQID_VIEW:
		qid.db_qry_low_time = char_entry[i].char_value;
		qid_specified++;
		continue;
	    case DMT_C_JOURNALED:
		journaling = char_entry[i].char_value;
		jparm = i;
		continue;
	    default:
		SETDBERR(&dmt->error, i, E_DM000D_BAD_CHAR_ID);
		return (E_DB_ERROR);
	    }
	}
	if (i < char_count)
	    break;
        
	scb= xcb->xcb_scb_ptr;
	timeout = dm2t_get_timeout(scb, &dmt->dmt_id);

	/*
	** B41659, B41669: Must take table control lock here to lock out
	** readlock-nolock accessors of the table while we are close-purging
	** it. Otherwise, the close purge may not work correctly (you may get
	** "table access conflict" (41659) if the readlock-nolock accessor is
	** in this server, or the close purge may not properly propagate to
	** other servers (41669) if the readlock-nolock accessor is in another
	** server.
	*/
	status = dm2t_control(odcb->odcb_dcb_ptr, dmt->dmt_id.db_tab_base,
				xcb->xcb_lk_id, LK_X, (i4)0, timeout,
				&dmt->error);
	if (status != E_DB_OK)
	    break;

	/* 
	** Now alter the table characteristics, if no one is using. 
	** Now try to open the table indicated by table id.  This will
	** place an exclusive lock on the table.  
	** Use error from open.
        */

	status = dm2t_open(odcb->odcb_dcb_ptr, &dmt->dmt_id, DM2T_X,
	    DM2T_UDIRECT, DM2T_A_MODIFY, timeout, (i4)20,
	    xcb->xcb_sp_id, xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0,
	    (i4)0, db_lockmode, &xcb->xcb_tran_id, &dmt->dmt_timestamp,
	    &rcb, scb, &dmt->error);
	if (status)
	    break;
	rcb->rcb_xcb_ptr = xcb;
	
	if (xcb->xcb_x_type & XCB_RONLY && 
		rcb->rcb_tcb_ptr->tcb_temporary == TCB_PERMANENT)  
	{
	    xcb->xcb_state |= XCB_STMTABORT;
	    SETDBERR(&dmt->error, 0, E_DM006A_TRAN_ACCESS_CONFLICT);
	    break;
	}

	if (rcb->rcb_tcb_ptr->tcb_temporary)
	{
	    status = dm2t_close(rcb, DM2T_NOPURGE, &local_dberr);
	    rcb = 0;
	    SETDBERR(&dmt->error, 0, E_DM005D_TABLE_ACCESS_CONFLICT);
	    break;
	}		

	ref_count  = rcb->rcb_tcb_ptr->tcb_open_count;
	sconcur = (rcb->rcb_tcb_ptr->tcb_rel.relstat & TCB_CONCUR);
	index = (rcb->rcb_tcb_ptr->tcb_rel.relstat & TCB_INDEX);
	gateway = (rcb->rcb_tcb_ptr->tcb_rel.relstat & TCB_GATEWAY);
	table_name = rcb->rcb_tcb_ptr->tcb_rel.relid;
	table_owner = rcb->rcb_tcb_ptr->tcb_rel.relowner;
	has_extensions = rcb->rcb_tcb_ptr->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS;

	/*
	** Check validity of alter request.
	** Temporary tables cannot be altered, nor can the user alter a table
	** which is already opened  (core cats are handled specially since
	** they are always open).
	*/
	if ((rcb->rcb_tcb_ptr->tcb_temporary) ||
	    ((ref_count > 1) && (sconcur == 0)))
	{
	    status = dm2t_close(rcb, DM2T_NOPURGE, &local_dberr);
	    rcb = 0;
	    if (status != E_DB_OK)
	    {
		uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, 
			(char * )NULL, 0L, (i4 *)NULL, &local_error, 0);
	    }
	    SETDBERR(&dmt->error, 0, E_DM005D_TABLE_ACCESS_CONFLICT);
	    break;
	}

	/*
	** Check table journal state.
	*/
	journal_flag = 0;
	if (odcb->odcb_dcb_ptr->dcb_status & DCB_S_JOURNAL)
	    journal_flag = DM0L_JOURNAL;

	/* Now close the table, use the purge flag to insure
        ** that the physical file is closed and the tcb deallocated. 
        */

	status = dm2t_close(rcb, DM2T_PURGE, &dmt->error);
	if (status != E_DB_OK)
	    break;

        /*
        ** Bug #36199:
        ** Set rcb to 0 so that errors subsequent to this will know not to
        ** close the table again.
        */
        rcb = 0;

	/*
	** If trying to set journaling on iirelation, iiattribute,
	** iiindexes or iirel_idx, then error.
 	** Also cannot set journaling on secondary index or gateway table.
	*/
	if ((journaling == 1) && (sconcur || index || gateway))
	{
	    if ( gateway )
	        SETDBERR(&dmt->error, jparm, E_DM0137_GATEWAY_ACCESS_ERROR);
	    else
	        SETDBERR(&dmt->error, jparm, E_DM000D_BAD_CHAR_ID);
	    break;
	}

	/*
	** Unless we were just changing the timestamp, log the alter - during
	** backout, we have to throw away the tcb associated with the altered
	** table.
        **
        ** Don't log anything if running in NOLOGGING mode.
	*/

	if ((~dmt->dmt_flags_mask & DMT_TIMESTAMP_ONLY) &&
            (xcb->xcb_flags & XCB_NOLOGGING) == 0)
	{
	    status = dm0l_alter(xcb->xcb_log_id, journal_flag, &dmt->dmt_id,
		&table_name, &table_owner, char_count, char_entry[0].char_id,
		(LG_LSN *)0, &lsn, &dmt->error);
	    if (status)
		break;
	}

	/* 
        ** It is alright to alter table, read the relation record
        ** for this table and update it. 
        */
	table_id.db_tab_base = DM_B_RELATION_TAB_ID;
	table_id.db_tab_index = DM_I_RELATION_TAB_ID;
   
	status = dm2t_open(odcb->odcb_dcb_ptr, &table_id, DM2T_IX,
	    DM2T_UDIRECT, DM2T_A_WRITE, (i4)0, (i4)20, xcb->xcb_sp_id,
	    xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0,(i4)0, 
            db_lockmode, 
            &xcb->xcb_tran_id, &timestamp,
	    &rel_rcb, scb, &dmt->error);
	if (status)
	    break;
	relupdate = TRUE;

        /*
        ** Check for NOLOGGING - no updates should be written to the log
        ** file if this session is running without logging.
        */
        if (xcb->xcb_flags & XCB_NOLOGGING)
            rel_rcb->rcb_logging = 0;

	/* 
	**  Read the relation table records  for the base table.
	*/
    
	/*  Position table using base table id. */

	table_id.db_tab_base = dmt->dmt_id.db_tab_base;
	table_id.db_tab_index = dmt->dmt_id.db_tab_index;
	key_desc[0].attr_operator = DM2R_EQ;
	key_desc[0].attr_number = DM_1_RELATION_KEY;
	key_desc[0].attr_value = (char *) &table_id.db_tab_base;

	status = dm2r_position(rel_rcb, DM2R_QUAL, key_desc, (i4)1,
                          (DM_TID *)0, &dmt->error);
	if (status != E_DB_OK)
	    break;

	for (;;)
	{
	    /* 
            ** Get a qualifying relation record.  This will retrieve
	    ** all records for a relation, including partitions & indexes 
	    ** if it has any.
            */

	    status = dm2r_get(rel_rcb, &tid, DM2R_GETNEXT, 
                              (char *)&relrecord, &dmt->error);

	    if (dmt->error.err_code == E_DM0055_NONEXT)
	    {
		status = dm2t_close(rel_rcb, DM2T_NOPURGE, &dmt->error);
		if (status != E_DB_OK)
		    break;
		return (E_DB_OK);
	    }

	    if (status != E_DB_OK)
		break;

	    if (relrecord.reltid.db_tab_base != table_id.db_tab_base)
		continue;
	    if (relrecord.reltid.db_tab_index != table_id.db_tab_index)
	    {
		/* only let partitions and indexes through from this condition */
		if (!(relrecord.relstat2 & TCB2_PARTITION) && !(relrecord.relstat & TCB_INDEX))
		    continue;	    
	    }
          
	    /*
	    ** Set rcb_usertab_jnl according to the journaling mode
	    ** of the table being altered.  This should be done prior
	    ** to updating the relrecord.relstat journal mode with
	    ** the affects of this alter operation.
	    **
	    ** The journaling mode set here is not affected by the results 
	    ** of this particular alter operation (the journaling mode will
	    ** normally be OFF for a "set journaling" alter operation
	    ** and ON for "set nojournaling").
	    **
	    ** For partitioned tables this needs to be done for the base 
	    ** table and the partitions.
	    */
	    rel_rcb->rcb_usertab_jnl = 
		    (relrecord.relstat & TCB_JOURNAL) ? 1 : 0;

	    /*
	    ** Found the relation record, update it. If the only change is in
	    ** the timestamp, there is no reason to check for other changes
	    */

	    /* We don't set these on partitions or indexes (currently at any rate) */
	    if (~dmt->dmt_flags_mask & DMT_TIMESTAMP_ONLY && 
		!(relrecord.relstat2 & TCB2_PARTITION) && 
		!(relrecord.relstat & TCB_INDEX))
	    {
		if (views ==  0)
		    relrecord.relstat &= ~TCB_VBASE; 
		else if (views == 1)
		    relrecord.relstat |= TCB_VBASE;
		if (integrities ==  0)
		    relrecord.relstat &= ~TCB_INTEGS; 
		else if (integrities == 1)
		    relrecord.relstat |= TCB_INTEGS;
		if (rules ==  0)
		    relrecord.relstat &= ~TCB_RULE; 
		else if (rules == 1)
		    relrecord.relstat |= TCB_RULE;
		if (commentson ==  0)
		    relrecord.relstat &= ~TCB_COMMENT; 
		else if (commentson == 1)
		    relrecord.relstat |= TCB_COMMENT;
		if (synonymson ==  0)
		    relrecord.relstat &= ~TCB_SYNONYM;
		else if (synonymson == 1)
		    relrecord.relstat |= TCB_SYNONYM;
		if (permits ==  0)
		    relrecord.relstat &= ~TCB_PRTUPS; 
		else if (permits == 1)
		    relrecord.relstat |= TCB_PRTUPS;
		if (zoptstats ==  0)
		    relrecord.relstat &= ~TCB_ZOPTSTAT; 
		else if (zoptstats == 1)
		    relrecord.relstat |= TCB_ZOPTSTAT;
		if (proall == 1)
		    relrecord.relstat |= TCB_PROALL;
		else if (proall == 0)
		    relrecord.relstat &= ~TCB_PROALL;
		if (retall == 1 )
		    relrecord.relstat |= TCB_PROTECT;
		else if (retall == 0)
		    relrecord.relstat &= ~TCB_PROTECT;
		if (alarmson ==  0)
		    relrecord.relstat2 &= ~TCB_ALARM; 
		else if (alarmson == 1)
		    relrecord.relstat2 |= TCB_ALARM;
		if (sd_specified)
		{
		    relrecord.relsave = save_date;
		    msgid=I_SX2719_TABLE_SAVE;
		}
		if (qid_specified)
		{
		    relrecord.relqid1 = qid.db_qry_high_time;
		    relrecord.relqid2 = qid.db_qry_low_time;
		}
	    }

	    /* We set this for base tables & partitions & indexes */
	    if (~dmt->dmt_flags_mask & DMT_TIMESTAMP_ONLY)
	    {
		/*
		** Update IIRELATION journal status.  If journaling is being
		** turned on for a non-journaled table, then we turn on the
		** JON bit (journaling enabled at next checkpoint) rather
		** than the JOURNAL bit.
		*/
		if (journaling == 1 && (relrecord.relstat & TCB_JOURNAL) == 0)
		{
		    relrecord.relstat |= TCB_JON;
		    msgid=I_SX2717_TABLE_JNLON;
		}
		else if (journaling == 0)
		{
		    relrecord.relstat &= ~(TCB_JOURNAL | TCB_JON);
		    msgid=I_SX2718_TABLE_JNLOFF;
		}
	    }

	    /* Change timestamp. */

	    TMget_stamp((TM_STAMP *)&relrecord.relstamp12);

	    status = dm2r_replace(rel_rcb, &tid, 
                       DM2R_BYPOSITION, (char *)&relrecord, (char *)0, &dmt->error);
	    if (status != E_DB_OK )
		break;

	    /* Only base table beyond this point */
	    if (relrecord.relstat2 & TCB2_PARTITION || relrecord.relstat & TCB_INDEX)
		continue;

	    /*
	    ** set journaling on t
	    ** Any time you change journaling status of base table,
	    ** all the etabs inherit the new journaling status of base table
	    */
	    if (has_extensions && journaling != -1)
	    {
		/* blobs inherit base table journaling */
		status = dmpe_journaling(journaling, xcb,
				&dmt->dmt_id, &dmt->error);
		if (status)
		    break;
	    }

	    /*
	    ** If the alter was on an SCONCUR catalog, then we have to
	    ** propagate the changes to the TCB directly.  Normally,
	    ** the close-purge done above would toss the TCB and the
	    ** next time the table is referenced a new TCB would be
	    ** built that would reflect the ALTER changes.  The TCB's
	    ** for SCONCUR tables are never tossed so we have to update
	    ** them directly.
	    */
	    if (sconcur)
	    {
		status = dm2t_open(odcb->odcb_dcb_ptr, &dmt->dmt_id, DM2T_X,
		    DM2T_UDIRECT, DM2T_A_WRITE, (i4)0, (i4)20,
		    xcb->xcb_sp_id, xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0,
		    (i4)0, db_lockmode, &xcb->xcb_tran_id,
		    &dmt->dmt_timestamp, &rcb, scb, &dmt->error);
		if (status)
		    break;

		/* Copy the new relrecord into the TCB */
		STRUCT_ASSIGN_MACRO(relrecord, rcb->rcb_tcb_ptr->tcb_rel);

		status = dm2t_close(rcb, DM2T_NOPURGE, &dmt->error);
		if (status != E_DB_OK)
		    break;
		rcb = 0;
	    }
	    /*
	    **	Operation succeeded, so audit this as a TABLE CONTROL operation
	    **  *if* we altered something security related (when msgid will
	    **  be >0)
	    */
	    if( msgid>0 && dmf_svcb->svcb_status & SVCB_C2SECURE )
	    {
		    status = dma_write_audit(
			SXF_E_TABLE,
		        SXF_A_CONTROL | SXF_A_SUCCESS,
			(char*)&relrecord.relid,	 /* Tablename */
			sizeof(relrecord.relid), 
			&relrecord.relowner, /* Table owner */
			msgid,			/* Detail message */
			FALSE, /* Not force */
			&dmt->error,
			NULL);
		if (status != E_DB_OK)
		{
		    uleFormat(&dmt->error, 0, NULL, ULE_LOG, NULL, 
		    	(char * )NULL, 0L, (i4 *)NULL, &local_error, 0);
		    break;
		}
	    }
	} /* End query loop */
	break;
    } /* end for */

    if (rcb)
    {
        DB_STATUS           local_status;

        /*
        ** If we opened the table, close it. Note: This can ONLY occur if we
        ** encountered an error in dm2t_open after it had already allocated
        ** an RCB for us. In such a case, we are required to call dm2t_close
        ** so that it can release the RCB resources.
        */
	local_status = dm2t_close(rcb, 0, &local_dberr);
	if (local_status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, (char * )NULL, 0L, 
		(i4 *)NULL, &local_error, 0);
	}
    }

    if (dmt->error.err_code > E_DM_INTERNAL)
    {
	uleFormat(&dmt->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char * )NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	if (relupdate)
	{
	    uleFormat(NULL, E_DM9026_REL_UPDATE_ERR, (CL_ERR_DESC *)NULL, ULE_LOG, 
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 1,
		sizeof(DB_DB_NAME),
		&rel_rcb->rcb_tcb_ptr->tcb_dcb_ptr->dcb_name);
	}
	SETDBERR(&dmt->error, 0, E_DM010A_ERROR_ALTERING_TABLE);
    }

    if (rel_rcb)
    {
	status = dm2t_close(rel_rcb, DM2T_NOPURGE, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, (char * )NULL, 0L, 
		(i4 *)NULL, &local_error, 0);
	}
    }
    return (E_DB_ERROR);
}
