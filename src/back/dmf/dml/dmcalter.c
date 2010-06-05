/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <bt.h>
#include    <cs.h>
#include    <cx.h>
#include    <di.h>
#include    <er.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <lg.h>
#include    <lk.h>
#include    <scf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dm0m.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0c.h>
#include    <dmxe.h>
#include    <dm2d.h>

/**
** Name: DMCALTER.C - Functions used to alter characteristics.
**
** Description:
**      This file contains the functions necessary to alter characteristics.
**      This file defines:
**    
**      dmc_alter() -  Routine to perform the alter
**                     characteristics operation.

** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.      
**	30-sep-88 (rogerk)
**	    Changed dmxe_begin call to use new arguments.
**      27-mar-89 (derek)
**	 4-feb-91 (rogerk)
**	    Added Set [No]Logging support.  Added DMC_C_LOGGING alter option.
**	    Moved "set_lockmode" stuff into a separate routine (set_lockmode).
**	    Added new routine - set_logging.
**	25-feb-91 (rogerk)
**	    Fixed bug in set_logging - init status value so we don't get
**	    error when run "set logging" in non fast_commit server.
**	25-mar-91 (rogerk)
**	    Toggle NOLOGGING state in config file when transaction logging
**	    is enabled/disabled.  This allows us to mark databases inconsistent
**	    if recovery is needed while running with set nologging.
**	    Changed set_logging routine to dmc_set_logging and made it non-local
**	    so it could be called from dmc_end_session.
**	22-apr-91 (rogerk)
**	    Check for online backup processing on database before we switch
**	    a session to Nologging mode.
**	7-jul-1992 (bryanp)
**	    Prototyping DMF.
**	8-jul-1992 (walt)
**	    Prototyping DMF.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	15-mar-1993 (rogerk)
**	    Fix compile warnings.
**	30-feb-1993 (rmuth)
**	    Prototyping DI showed up errors.
**	26-apr-1993 (bryanp)
**	    Add forward declaration of set_worklocs().
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	09-oct-93 (swm)
**	    Bug #56437
**	    Get scf_session from new dmc_session_id rather than dmc_id.
**	15-nov-93 (swm)
**	    Bug #58628
**	    Rounded up mask_size to be a multiple of sizeof(i4) to
**	    ensure that memory allocated for masks is i4-aligned
**	    (This is relied upon in dmcopen()).
**	31-jan-1994 (bryanp) B57823, B57824, B57825
**	    If scf_call fails, log scf_error.err_code.
**	    If LG call fails, log LG error code.
**	    If LGbegin returns LG_EXCEED_LIMIT, return RESOURCE_QUOTA_EXCEED.
**	31-jan-94 (jnash)
**	    When SET NOLOGGING performed, disallow normal rollforwarddb 
** 	    operations from a prior checkpoint.
**	18-mar-1994 (pearl)
**	    Bug 60695. Replaced the E_DM9050_TRANSACTION_NO_LOGGING message
**          with E_DM9060_SESSION_LOGGING_OFF.  The text is similar to the old
**          message's, plus it provides the current log file EOF, and also
**          advises that the journals are incomplete until the next
**          'ckpdb +j'.
**          Added E_DM9061_JNLDB_LOGGING_ON and
**          E_DM9062_NON_JNLDB_LOGGING_ON to mark when logging is turned on,
**          and to mark the log file EOF.  The first message is for journaled
**          databases, and advises the user to run 'ckpdb +j'.  The latter
**          message is for non-journaled databases.  
**      04-apr-95 (dilma04)
**          Add support for SET TRANSACTION ISOLATION LEVEL ... and READLOCK=
**          READ_COMMITTED/REPEATABLE_READ (originally proposed by bryanp)
**	17-may-95 (dilma04)
**          Changed setting of table level lockmode values. 
**      22-nov-96 (dilma04)
**          Row Locking Project:
**          Added comments about DMC_C_ROW value.
**	21-Jan-1997 (jenjo02)
**	    Remove DM0M_ZERO from dm0m_allocate() for SLCBs. 
**	    All SLCB fields are individually initialized.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Change set_lockmode(), set_transaction() and comments for 
**          dmc_alter() to match the new fields of SLCB and SCB to store
**          locking characteristics. 
**      27-feb-97 (stial01)
**          Disallow row locking if multiple servers using different data caches
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Added support for SET SESSION <access mode>
**      21-May-1997 (stial01)
**          Disallow row locking for non-fast commit servers:   
**          Fast commit must be specified for private cache sole server.
**          Row locking is permitted if multiple servers using different
**          data caches when using DMCM protocol (fast commit).
**      12-jun-97 (stial01)
**          set_lockmode() Temporarily disable row locking with DMCM protocol.
**      20-aug-97 (stial01)
**          set_lockmode() Ignore level=row for iidbdb (B84618)
**      14-oct-97 (stial01)
**          dmc_alter() Added support for SET SESSION ISOLATION LEVEL.
**          set_lockmode() readlock -> read lock mode NOT isolation level
**      08-Dec-1997 (hanch04)
**          Use new la_block field when block number is needed instead
**          of calculating it from the offset
**          Keep track of block number and offset into the current block.
**          for support of logs > 2 gig
**      28-may-98 (stial01)
**          set_lockmode() moved code to dm2d_row_lock()
**      21-mar-1999 (nanpr01)
**          Removed extra local variables which are not used.
**	15-Dec-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LGbegin() prototype.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Jan-2001 (jenjo02)
**	    Find DML_SCB with macro instead of calling SCF.
**	29-mar-2002 (devjo01)
**	    Convert requests for row level locking to page level
**	    locking if running under clusters.
**	13-Aug-2002 (jenjo02)
**	    Allow SET LOCKMODE TIMEOUT while in a transaction,
**	    disallow all others. Support TIMEOUT=NOWAIT
**	    (DMC_C_NOWAIT).
**      02-jan-2004 (stial01)
**          Removed temporary trace points DM722, DM723
**          Added support for set [no]bloblogging
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	05-Apr-2004 (hanje04)
**	    BUG 110299
**  	    In set_lockmode(), for Row level locking, only break out of 
**	    main loop if dm2d_row_lock(dcb) == FALSE and we're NOT running
**	    clustered. Otherwise scb_timeout (and others) never get set
**	    for the session.
**      13-may-2004 (stial01)
**          Ignore NOBLOBLOGGING if using FASTCOMMIT protocols
**      27-aug-2004 (stial01)
**          Removed SET NOBLOBLOGGING statement
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	18-Nov-2008 (jonj)
**	    dm0c_?, dm2d_? functions converted to DB_ERROR *
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**       8-Jun-2009 (hanal04) Code Sprint SIR 122168 Ticket 387
**          Block SET NOLOGGING if DCB_S_MUST_LOG is set.
**	19-Aug-2009 (kschendel) 121804
**	    Need cx.h for proper CX declarations (gcc 4.3).
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/* Forward and static declarations. */

    /* Alters lockmode characteristics. */
static DB_STATUS set_lockmode(
    DMC_CB	*dmc_cb,
    DML_SCB	*scb);

static DB_STATUS set_transaction(
    DMC_CB      *dmc_cb,
    DML_SCB     *scb);
 
static DB_STATUS set_worklocs(
    DMC_CB	    *dmc_cb,
    DML_SCB	    *scb);


/*{
** Name: dmc_alter - Alters control characteristics.
**
**  INTERNAL DMF call format:    status = dmc_alter(&dmc_cb);
**
**  EXTERNAL call format:        status = dmf_call(DMC_ALTER,&dmc_cb);
**
** Description:
**    The dmc_alter function handles the altering of control information about
**    a server, a session, or a database.  This allows the caller to alter the
**    default values associated with each of these objects.  The caller 
**    specifies the object to change and gives a list of characteristics that 
**    should be changed for the named object.  Modifications to the server
**    or session are primarily concerned with configuration parameters such as
**    maximum number of databases that can be handled by one server, the maximum
**    number of session per server, the maximum number of database that can be
**    opened per session, etc.  To set lock mode characteristics about a table
**    for this session an alter session operation is performed with the flag
**    set to DMC_SETLOCKMODE and characteristic entries for level, maxlocks,
**    timeout, and readlock can be given.  If the scope of the setlock 
**    operation is given in dmc_sl_scope and if it indicates table
**    then dmc_table_id and the dmc_db_id must be given.
**    The characteristics associated with a database
**    deal more with the access mode such as read shared or write exclusive
**    and whether or not the datbase is journalled.  All modifications are
**    temporary and exist only as long as the server exists.
**
** Inputs:
**      dmc_cb 
**          .type                           Must be set to DMC_CONTROL_CB.
**          .length                         Must be at least sizeof(DMC_CB).
**          .dmc_op_type                    Must be DMC_SERVER_OP or
**                                          DMC_DB_OP, DMC_SESSION_OP.
**          .dmc_session_id		    Must be a unique server or
**                                          session identifier returned
**                                          from a start server or begin
**                                          session operation.
**          .dmc_flags_mask                 Must be zero or if dmc_op_type
**                                          is DMC_DATABASE_OP 
**                                          one of the following:
**                                          DMC_NOJOURNAL or DMC_JOURNAL 
**                                          or if dmc_op_type is
**					    DMC_SESSION_OP then
**                                          this can be DMC_SETLOCKMODE or
**                                          DMC_SYSCAT_MODE to change 
**                                          the session to allow system 
**                                          catalogs updates or not.
**					    or DMC_SET_SESS_AM to set
**					    the session access mode to
**					    dmc_db_access_mode, either
**					    DMC_A_READ or DMC_A_WRITE.
**          .dmc_db_id                      Must be zero or a valid database
**					    identifier returned from an open
**					    database operation.  This value
**					    is required in the following alter
**					    operations:
**						DMC_DATABASE_OP operation
**						DMC_SETLOCKMODE
**						DMC_C_LOGGING
**          .dmc_sl_scope                   Must be DMC_SL_TABLE or
**                                          DMC_SL_SESSION if DMC_SETLOCKMODE
**                                          is set.
**          .dmc_table_id                   The table identifier for a set
**                                          lock mode operation.
**          .dmc_db_access_mode             Must be zero or if dmc_op_type
**                                          is DMC_DATABASE_OP or
**					       DMC_SET_SESS_AM one of 
**                                          the following:
**                                          DMC_A_READ, DMC_A_WRITE.
**          .dmc_s_type                     Value to indicate the type
**                                          of session must be either
**                                          DMC_S_NORMAL for a session with
**                                          no update privilege on system 
**                                          catalogs or DMC_S_SYSUPDATE for
**                                          a session with privilege
**                                          or DMC_S_SECURITY for a session
**                                          with security privilege or
**                                          DMC_S_AUDIT for a session which
**                                          has all events audited.
**          .dmc_lock_mode                  Must be DMC_L_SHARE or 
**                                          DMC_L_EXCLUSIVE if Modifying
**                                          the database characteristics.
**	    .dmc_names_loc		    Names of locations to added/dropped
**					    for this session (used only if
**					    dmc_flags_mask is DMC_LOC_MODE).
**          .dmc_char_array                 Pointer to an area used to input
**                                          an array of characteristics 
**                                          entries of type DMC_CHAR_ENTRY.
**                                          See below for description of
**                                          <dmc_char_array> entries.
**
**          <dmc_char_array> entries are of type DMC_CHAR_ENTRY and
**          must have the following values:
**          char_id                         Must be one of following:
**                                          DMC_C_DEBUG
**                                          DMC_C_JOURNAL
**                                          DMC_C_DATABASE_MAX
**                                          DMC_C_OPEN_DB_MAX
**                                          DMC_C_S_MAX
**                                          DMC_C_M_MAX
**                                          DMC_C_TRAN_ACCESS_MODE
**                                          DMC_C_TRAN_MAX
**					    DMC_C_LTIMEOUT
**                                             The value for this
**                                             is either DMC_C_SESSION,
**                                             DMC_C_SYSTEM, DMC_C_NOWAIT,
**					       or a positive number.
**					    DMC_C_LLEVEL
**                                             The value for this
**                                             is either DMC_C_SESSION,
**                                             DMC_C_SYSTEM, DMC_C_ROW,
**                                             DMC_C_PAGE or DMC_C_TABLE. 
**					    DMC_C_LMAXLOCKS
**                                             The value for this
**                                             is either DMC_C_SESSION,
**                                             DMC_C_SYSTEM, or a positive
**                                             number. 
**					    DMC_C_LREADLOCK
**                                             The value for this
**                                             is either DMC_C_SESSION,
**                                             DMC_C_SYSTEM, DMC_C_X for
**                                             exclusive readlock,
**                                             DMC_C_S for shared readlock,
**                                             or DMC_C_N for nolock. 
**					    DMC_C_LOGGING
**                                          DMC_C_ISOLATION_LEVEL
**					    DMC_C_ON_LOGFULL
**          char_value                      Must be an integer value or
**                                          one of the following:
**                                          DMC_C_ON
**                                          DMC_C_OFF
**                                          DMC_C_SESSION
**                                          DMC_C_SYSTEM
**				            DMC_C_ROW	
**                                          DMC_C_PAGE
**                                          DMC_C_TABLE
**                                          DMC_C_MVCC
**                                          DMC_C_READ_UNCOMMITTED
**                                          DMC_C_READ_COMMITTED
**                                          DMC_C_REPEATABLE_READ
**                                          DMC_C_SERIALIZABLE
**                                          DMC_C_READ_EXCLUSIVE
**                                          DMC_C_READ_ONLY
**                                          DMC_C_READ_WRITE
**                                          DMC_C_LOGFULL_ABORT
**                                          DMC_C_LOGFULL_COMMIT
**                                          DMC_C_LOGFULL_NOTIFY
**
** Output:
**      dmc_cb 
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM000D_BAD_CHAR_ID
**                                          E_DM000E_BAD_CHAR_VALUE
**                                          E_DM0010_BAD_DB_ID
**                                          E_DM001A_BAD_FLAG
**                                          E_DM002A_BAD_PARAMETER
**                                          E_DM002F_BAD_SESSION_ID
**                                          E_DM003E_DB_ACCESS_CONFLICT
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**                                          E_DM0054_NONEXISTENT_TABLE
**                                          E_DM0065_USER_INTR
**                                          E_DM0064_USER_ABORT
**					    E_DM009F_ILLEGAL_OPERATION
**                                          E_DM0100_DB_INCONSISTENT
**                                          E_DM0107_ERROR_ALTERING_SESSION
**                                          E_DM010C_TRAN_ABORTED
**                                          E_DM010D_ERROR_ALTERING_DATABASE
**                                          E_DM0112_RESOURCE_QUOTA_EXCEEDED
**					    E_DM0147_NOLG_BACKUP_ERROR
**      
**
**          .error.err_data                 Set to characteristic in error 
**                                          by returning index into 
**                                          characteristic array.
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_WARN                       Function completed normally with 
**                                          a termination status which is in 
**                                          dmc_cb.error.err_code.
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dmc_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmc_cb.error.err_code.
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**	30-sep-88 (rogerk)
**	    Changed dmxe_begin call to use new arguments.
**	 8-dec-88 (rogerk)
**	    Don't return zero error code if get a locking error.
**	18-apr-1989 (derek)
**	    Remove support for add location.  This function has moved
**	    to dmm_add_location.
**	 4-feb-91 (rogerk)
**	    Added Set [No]Logging support.  Added DMC_C_LOGGING alter option.
**	    Moved "set_lockmode" stuff into a separate routine (set_lockmode).
**	    Added new routine - set_logging.
**	04-nov-92 (jrb)
**	    Added support for DMC_S_NOAUTH_LOCS, DMC_S_ADD_LOCS,
**	    DMC_S_DROP_LOCS, and DMC_S_TRADE_LOCS for multi-location sort
**	    project.
**	09-oct-93 (swm)
**	    Bug #56437
**	    Get scf_session from new dmc_session_id rather than dmc_id.
**	31-jan-1994 (bryanp) B57823, B57824, B57825
**	    If scf_call fails, log scf_error.err_code.
**	    If LG call fails, log LG error code.
**	23-may-94 (robf)
**          Correct check against DMC_S_NORMAL, this is #define as 0
**	    so we can't do a bit-and against it.
**      04-apr-1995 (dilma04)
**          Add support for SET TRANSACTION ISOLATION LEVEL ... and READLOCK=
**          READ_COMMITTED/REPEATABLE_READ (originally proposed by bryanp)
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Change comments about char_value according to new values. 
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Added support for SET SESSION <access mode>
**      14-oct-1997 (stial01)
**          dmc_alter() Added support for SET SESSION ISOLATION LEVEL.
**	08-Jan-2001 (jenjo02)
**	    Find DML_SCB with macro instead of calling SCF.
**	20-Apr-2004 (jenjo02)
**	    Added support for ON_LOGFULL = ABORT | COMMIT | NOTIFY
*/

DB_STATUS
dmc_alter(
DMC_CB    *dmc_cb)
{
    DM_SVCB             *svcb;
    DMC_CB		*dmc = dmc_cb;
    DML_SCB		*scb;
    DML_ODCB		*odcb;
    DMP_DCB             *dcb;
    DB_STATUS		status = 0, local_status = 0;
    i4		error,local_error;
    CL_ERR_DESC		sys_err;
    i4             i;
    DMC_CHAR_ENTRY	*chr;
    i4              chr_count;

    CLRDBERR(&dmc->error);
    
    switch (dmc->dmc_op_type)
    {
    case DMC_SESSION_OP:

	svcb = dmf_svcb;
	for (status = E_DB_ERROR;;)
	{
	    if ( (scb = GET_DML_SCB(dmc->dmc_session_id)) == 0 || 
		  dm0m_check((DM_OBJECT *)scb, (i4)SCB_CB) )
	    {
		SETDBERR(&dmc->error, 0, E_DM002F_BAD_SESSION_ID);
		break;
	    }
    
	    if ((dmc->dmc_flags_mask) &&
		(dmc->dmc_flags_mask != DMC_SETLOCKMODE) &&
	        (dmc->dmc_flags_mask != DMC_SYSCAT_MODE) &&
                (dmc->dmc_flags_mask != DMC_SETTRANSACTION) &&
		(dmc->dmc_flags_mask != DMC_LOC_MODE) &&
		(dmc->dmc_flags_mask != DMC_SET_SESS_AM) &&
		(dmc->dmc_flags_mask != DMC_SET_SESS_ISO))
	    {
		SETDBERR(&dmc->error, 0, E_DM009F_ILLEGAL_OPERATION);
		break;
	    }

	    /*
	    ** Check alter session operation.
	    */
	    if (dmc->dmc_flags_mask == DMC_SETLOCKMODE)
	    {
		/*
		** Process set lockmode command.
		*/
		return( set_lockmode(dmc, scb) );
	    }

            if (dmc->dmc_flags_mask == DMC_SETTRANSACTION)
            {
                /*
                ** Process set transaction command.
                */
                return( set_transaction(dmc, scb) );
            }

	    /*
	    ** Check for alter location request.
	    */
	    if (dmc->dmc_flags_mask == DMC_LOC_MODE)
	    {
		/*
		** Process set work locations command.
		*/
		return( set_worklocs(dmc, scb) );
	    }

	    /*
	    ** Check for update syscats operation.
	    */
	    if (dmc->dmc_flags_mask == DMC_SYSCAT_MODE)
	    {
		if (dmc->dmc_s_type & DMC_S_SYSUPDATE)
		    scb->scb_s_type |= SCB_S_SYSUPDATE;
		if (dmc->dmc_s_type & DMC_S_SECURITY)
		    scb->scb_s_type |= SCB_S_SECURITY;
		if (dmc->dmc_s_type & DMC_S_AUDIT)
		    scb->scb_audit_state = SCB_A_TRUE;

		/* DMC_S_NORMAL is 0, so can't do a bit-and operation */
		if (dmc->dmc_s_type == DMC_S_NORMAL)
		{
                    scb->scb_s_type = SCB_S_NORMAL;
		    scb->scb_audit_state = SCB_A_FALSE;
		}
		return (E_DB_OK);
	    }

	    /*
	    ** Check for SET SESSION <access mode>
	    ** (DMC_A_READ or DMC_A_WRITE).
	    */
	    if (dmc->dmc_flags_mask & DMC_SET_SESS_AM)
	    {
		scb->scb_sess_am = dmc->dmc_db_access_mode;
		return (E_DB_OK);
	    }

	    /*
	    ** Check for SET SESSION ISOLATION
	    ** (DMC_A_READ_UNCOMMITTED, DMC_A_READ_COMMITTED,
	    **  DMC_A_REPEATABLE_READ, DMC_A_SERIALIZABLE)
	    */
	    if (dmc->dmc_flags_mask & DMC_SET_SESS_ISO)
	    {
		scb->scb_sess_iso = dmc->dmc_isolation;
		return (E_DB_OK);
	    }

	    /*
	    ** Check char array for alter options.
	    */
	    chr = (DMC_CHAR_ENTRY*) dmc->dmc_char_array.data_address;
	    chr_count = dmc->dmc_char_array.data_in_size 
                                         / sizeof(DMC_CHAR_ENTRY);

	    if ((chr == NULL) || (chr_count == 0))
	    {
		SETDBERR(&dmc->error, 0, E_DM002A_BAD_PARAMETER);
		break;
	    }

	    for (i = 0; dmc->error.err_code == 0 && (i < chr_count); i++)
	    {
		switch (chr[i].char_id)
		{
		  case DMC_C_LOGGING:
		    /*
		    ** Get database control block for dmc_set_logging call.
		    */
		    odcb = (DML_ODCB *)dmc->dmc_db_id;
		    if ((odcb == NULL) || (odcb->odcb_dcb_ptr == NULL) ||
			(dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB)))
		    {
			SETDBERR(&dmc->error, 0, E_DM0010_BAD_DB_ID);
			break;
		    }

		    dcb = odcb->odcb_dcb_ptr;
		    status = dmc_set_logging(dmc, scb, dcb, chr[i].char_value);
		    break;

		  case DMC_C_ON_LOGFULL:
		    if ( chr[i].char_value == DMC_C_LOGFULL_ABORT )
			scb->scb_sess_mask &= ~(SCB_LOGFULL_COMMIT |
						SCB_LOGFULL_NOTIFY);
		    else if ( chr[i].char_value == DMC_C_LOGFULL_COMMIT )
		    {
			/* Silently commit, no log message */
			scb->scb_sess_mask |= SCB_LOGFULL_COMMIT;
			scb->scb_sess_mask &= ~SCB_LOGFULL_NOTIFY;
		    }
		    else if ( chr[i].char_value == DMC_C_LOGFULL_NOTIFY )
			/* Commit and log message */
			scb->scb_sess_mask |= (SCB_LOGFULL_COMMIT |
					       SCB_LOGFULL_NOTIFY);
		    break;

		  default:
		    SETDBERR(&dmc->error, 0, E_DM002A_BAD_PARAMETER);
		    break;
		}
	    }

	    if ( dmc->error.err_code )
	    {
		dmc->error.err_data = i;
		return(E_DB_ERROR);
	    }

	    return (E_DB_OK);
	}
	return (E_DB_ERROR);

    default:
	SETDBERR(&dmc->error, 0, E_DM002A_BAD_PARAMETER);
	return (E_DB_ERROR);	
    } /* end outer switch. */
}

/*{
** Name: set_lockmode - Alters lockmode characteristics.
**
** Description:
**    This routine parses the dmc control block and extracts Set Lockmode
**    parameters.  The parameters are copied to the session control block
**    for use by future DMT_OPEN calls.
**
**    This routine is called when a DMC_ALTER call is made with the
**    DMC_SETLOCKMODE flag.
**
** Inputs:
**      dmc_cb				Alter control block.
**	scb				Session control block.
**
** Output:
**	dmc_cb
**	   .error.err_code		One of the following error numbers.
**                                      E_DM000D_BAD_CHAR_ID
**                                      E_DM0010_BAD_DB_ID
**                                      E_DM002A_BAD_PARAMETER
**                                      E_DM0054_NONEXISTENT_TABLE
**                                      E_DM0107_ERROR_ALTERING_SESSION
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** History:
**	 4-feb-1991 (rogerk)
**	    Moved set lockmode code from above into its own routine.
**      04-apr-1995 (dilma04)
**          Add support for READLOCK=READ_COMMITTED/REPEATABLE_READ.
**      17-may-95 (dilma04)
**          Changed setting of table level lockmode values.
**	21-Jan-1997 (jenjo02)
**	    Remove DM0M_ZERO from dm0m_allocate() for SLCBs. 
**	    All SLCB fields are individually initialized.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          - store session level settings in the SCB instead of session
**          SLCB that does not exist any more;
**          - removed setting of slcb_k_duration field.
**      27-feb-97 (stial01)
**          Disallow row locking if multiple servers using different data caches
**      21-May-97 (stial01)
**          Disallow row locking for non-fast commit servers:   
**          Fast commit must be specified for private cache sole server.
**          Row locking is permitted if multiple servers using different
**          data caches when using DMCM protocol (fast commit).
**      12-jun-97 (stial01)
**          set_lockmode() Temporarily disable row locking with DMCM protocol.
**      20-aug-97 (stial01)
**          set_lockmode() Ignore level=row for iidbdb (B84618)
**      14-oct-97 (stial01)
**          set_lockmode() readlock -> read lock mode NOT isolation level
**      28-may-98 (stial01)
**          set_lockmode() moved code to dm2d_row_lock()
**	29-mar-2002 (devjo01)
**	    Convert requests for row level locking to page level
**	    locking if running under clusters.
**	13-Aug-2002 (jenjo02)
**	    Allow TIMEOUT within a transaction, nothing else, unless
**	    a system session, for which all are allowed.
**	20-Dec-2002 (jenjo02)
**	    Test xcb_flags & XCB_USER_TRAN rather than
**	    scb->scb_s_type == SCB_S_NORMAL, which is worthless.
**	05-Apr-2004 (hanje04)
**	    BUG 110299
**  	    In set_lockmode(), for Row level locking, only break out of 
**	    main loop if dm2d_row_lock(dcb) == FALSE and we're NOT running
**	    clustered. Otherwise scb_timeout (and others) never get set
**	    for the session.
**	17-Aug-2005 (jenjo02)
**	    Remove db_tab_index from consideration in DML_SLCB.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add lock level DMC_C_MVCC
*/
static DB_STATUS
set_lockmode(
DMC_CB	    *dmc_cb,
DML_SCB	    *scb)
{
    DMC_CB		*dmc = dmc_cb;
    DML_SLCB		*slcb, *table_slcb;
    DMC_CHAR_ENTRY	*chr;
    DB_STATUS		status;
    i4		local_error;
    i4		error;
    i4		chr_count;
    i4             ltimeout;
    i4             llevel;
    i4             lmaxlocks;
    i4             lreadlock;
    i4		i;
    DML_ODCB		*odcb;
    DMP_DCB             *dcb;

    CLRDBERR(&dmc->error);

    for (;;)
    {
	if ((dmc->dmc_sl_scope != DMC_SL_TABLE)
	    && (dmc->dmc_sl_scope != DMC_SL_SESSION)
	   )
	{
	    SETDBERR(&dmc->error, 0, E_DM002A_BAD_PARAMETER);
	    break;	
	}

	if ((dmc->dmc_flags_mask == DMC_SETLOCKMODE) &&
	    (dmc->dmc_sl_scope == DMC_SL_TABLE) &&
	    (dmc->dmc_table_id.db_tab_base == 0))
	{
	    SETDBERR(&dmc->error, 0, E_DM0054_NONEXISTENT_TABLE);
	    break;	
	}

	if ((dmc->dmc_flags_mask == DMC_SETLOCKMODE) &&
	    (dmc->dmc_sl_scope == DMC_SL_TABLE) &&
	    (dmc->dmc_db_id == 0))
	{
	    SETDBERR(&dmc->error, 0, E_DM0010_BAD_DB_ID);
	    break;	
	}

	/*
	** Get user supplied lockmode values. These are 'level',
	** 'maxlocks', 'timeout', and 'readlock'.  If not specified, use
	** SLCB_SE_DEF for table lockmodes, SLCB_SY_DEF for session level.
	*/

	chr = (DMC_CHAR_ENTRY*) dmc->dmc_char_array.data_address;
	chr_count = dmc->dmc_char_array.data_in_size 
				     / sizeof(DMC_CHAR_ENTRY);
	if ( chr_count == 0 || chr == NULL )
	{
	    SETDBERR(&dmc->error, 0, E_DM002A_BAD_PARAMETER);
	    break;
	}

	ltimeout = -1L;
	lmaxlocks = -1L;
	llevel    = -1L;
	lreadlock = -1L;

	for (i = 0; i < chr_count; i++)
	{
	    /*
	    ** If within a user transaction, only 
	    ** TIMEOUT is allowed. Trust that system
	    ** sessions know what they're doing.
	    */
	    if ( scb->scb_x_ref_count &&
		 scb->scb_x_next->xcb_flags & XCB_USER_TRAN &&
	         chr[i].char_id != DMC_C_LTIMEOUT )
	    {
		SETDBERR(&dmc->error, i, E_DM0060_TRAN_IN_PROGRESS);
		break;
	    }

	    switch (chr[i].char_id)
	    {
	    case DMC_C_LTIMEOUT:
		ltimeout = chr[i].char_value;
		continue;

	    case DMC_C_LLEVEL:
		llevel = chr[i].char_value;
		continue;

	    case DMC_C_LMAXLOCKS:
		lmaxlocks = chr[i].char_value;
		continue;

	    case DMC_C_LREADLOCK:
		lreadlock = chr[i].char_value;
		continue;

	    default:
		SETDBERR(&dmc->error, i, E_DM000D_BAD_CHAR_ID);
	    }
	    break;
	}

	if (i < chr_count)
	    break;

	if ( llevel == DMC_C_ROW || llevel == DMC_C_MVCC )
	{
	    odcb = (DML_ODCB *)dmc->dmc_db_id;
	    if ((odcb == NULL) || (odcb->odcb_dcb_ptr == NULL) ||
		(dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB)))
	    {
		SETDBERR(&dmc->error, 0, E_DM0010_BAD_DB_ID);
		break;
	    }

	    /*
	    ** Check if we can do row/mvcc locking with this database/server
	    ** Ignore SET LOCKMODE ... WHERE LEVEL=ROW/MVCC for iidbdb
	    ** IIDBDB always uses non-FASTCOMMIT protocols.
	    */
	    dcb = odcb->odcb_dcb_ptr;
	    if (dcb->dcb_id == 1)
	    {
		llevel    = -1L;
	    }
	    else if (dm2d_row_lock(dcb) == FALSE)
	    {
		/* NB: MVCC has the same criteria as ROW locking */
		if ( CXcluster_enabled() )
		{
		    /*
		    ** Clustering does not currently support row/mvcc
		    ** level locking.  However rather than fail
		    ** (and break existing aplications, as well
		    ** as ingres utilities such as upgrade), the
		    ** finest level of available granularity
		    ** (page) is used instead.  When fast commit
		    ** is enabled for Clustered Ingres, it is
		    ** hoped this restriction will be lifted.
		    */
		    llevel = DMC_C_PAGE;
		}
		else
		{
		    SETDBERR(&dmc->error, 0, E_DM010E_SET_LOCKMODE_ROW);
		    break;
		}
	    }
	}

	switch (dmc->dmc_sl_scope)
	{
	case DMC_SL_SESSION:

	    if (ltimeout != -1L && ltimeout != DMC_C_SESSION)
                scb->scb_timeout = ltimeout;

            if (lmaxlocks != -1L && lmaxlocks != DMC_C_SESSION)
                scb->scb_maxlocks = lmaxlocks;
	    
	    if (llevel != -1L && llevel != DMC_C_SESSION)
                scb->scb_lock_level= llevel;

	    if (lreadlock != -1L && lreadlock != DMC_C_SESSION)
		scb->scb_readlock = lreadlock;
	
	    break;

	case DMC_SL_TABLE:

	    table_slcb = 0;
	    
	    slcb = scb->scb_kq_next;
	    for (;;)
	    {
		if (slcb == (DML_SLCB*) &scb->scb_kq_next)
		    break;
		if ( slcb->slcb_db_tab_base
		     == dmc->dmc_table_id.db_tab_base )
		{
		    table_slcb = slcb;
		    break;
		}
		else
		    slcb = slcb->slcb_q_next;
	    }
		
	    if (table_slcb == 0)
	    {
		/* Allocate and initialize and SLCB. */
		/* Allocate an SLCB and initialize it. */

		status = dm0m_allocate((i4)sizeof(DML_SLCB), 
			    0, 
			    (i4)SLCB_CB,
			    (i4)SLCB_ASCII_ID, 
			    (char *)scb, (DM_OBJECT **)&slcb, &dmc->error);
		if (status != E_DB_OK)
		{
		    uleFormat( &dmc->error, 0, NULL, ULE_LOG , NULL, 
			(char * )NULL, 0L, (i4 *)NULL, 
			&local_error, 0);
		    SETDBERR(&dmc->error, 0, E_DM0107_ERROR_ALTERING_SESSION);
		    break;
		}


		/* Initialize control block with session defaults. */

		slcb->slcb_scb_ptr = scb;
		/*  %%%% add cocde for db_id. */
		slcb->slcb_db_tab_base = 
			      dmc->dmc_table_id.db_tab_base;
		slcb->slcb_timeout = DMC_C_SESSION;
		slcb->slcb_maxlocks = DMC_C_SESSION;
		slcb->slcb_lock_level = DMC_C_SESSION;
		slcb->slcb_readlock = DMC_C_SESSION;

		/* Queue at tail of queue. */

		slcb->slcb_q_next = scb->scb_kq_prev->slcb_q_next;
		slcb->slcb_q_prev = scb->scb_kq_prev;
		scb->scb_kq_prev->slcb_q_next = slcb;
		scb->scb_kq_prev = slcb;
		table_slcb = slcb;		    

	    }		   

	    if (ltimeout != -1L)
                table_slcb->slcb_timeout = ltimeout;

	    if (lmaxlocks != -1L)
                table_slcb->slcb_maxlocks = lmaxlocks;

            if (llevel != -1L)
                table_slcb->slcb_lock_level = llevel;

	    if (lreadlock != -1L)
		table_slcb->slcb_readlock = lreadlock;
	    break;

	} /* end inner switch. */

	break;
    }

    return( (dmc->error.err_code) ? E_DB_ERROR : E_DB_OK );
}


/*{
** Name: set_transaction - Alters transaction characteristics.
**
** Description:
**    This routine parses the dmc control block and extracts Set transaction
**    parameters.  The parameters are copied to the session control block
**    for use by future DMT_OPEN calls.
**
**    This routine is called when a DMC_ALTER call is made with the
**    DMC_SETTRANSACTION flag.
**
** Inputs:
**      dmc_cb                          Alter control block.
**      scb                             Session control block.
**
** Output:
**	dmc_cb
**	   .error.err_code		One of the following error numbers.
**                                      E_DM000D_BAD_CHAR_ID
**                                      E_DM0010_BAD_DB_ID
**                                      E_DM002A_BAD_PARAMETER
**                                      E_DM0054_NONEXISTENT_TABLE
**                                      E_DM0107_ERROR_ALTERING_SESSION
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** History:
**      23-aug-1993 (bryanp)
**          Created.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Change scb_isolation_level to scb_tran_iso.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
*/
static DB_STATUS
set_transaction(
DMC_CB      *dmc_cb,
DML_SCB     *scb)
{
    DMC_CB              *dmc = dmc_cb;
    DMC_CHAR_ENTRY      *chr;
    i4             chr_count;
    i4             i;

    CLRDBERR(&dmc->error);
 
    for (;;)
    {
 
        chr = (DMC_CHAR_ENTRY*) dmc->dmc_char_array.data_address;
        chr_count = dmc->dmc_char_array.data_in_size
                                     / sizeof(DMC_CHAR_ENTRY);
        if (chr_count == 0 || chr == NULL)
        {
	    SETDBERR(&dmc->error, 0, E_DM002A_BAD_PARAMETER);
            break;
        }
 
        for (i = 0; i < chr_count; i++)
        {
            switch (chr[i].char_id)
            {
            case DMC_C_ISOLATION_LEVEL:
		/*
		** Get user supplied transaction isolation values. These are
		** read_committed, read_uncommitted, repeatable_read, and
		** serializable.
		*/
                scb->scb_tran_iso = chr[i].char_value;
                continue;

            case DMC_C_TRAN_ACCESS_MODE:
		/*
		** Allow any combination of ISOLATION MODE and ACCESS MODE
		*/
		scb->scb_tran_am = chr[i].char_value;
		continue;
 
            default:
		SETDBERR(&dmc->error, i, E_DM000D_BAD_CHAR_ID);
                break;
            }
            break;
        }
 
        break;
    }
 
    return( (dmc->error.err_code) ? E_DB_ERROR : E_DB_OK );
}
 

/*{
** Name: set_worklocs - Alters list of locations being used.
**
** Description:
**    This routine parses the dmc control block and extracts Set Work Location
**    parameters and then alters the scb to reflect the changes in the work
**    location bit masks.
**
**    This routine is called when a DMC_ALTER call is made with the
**    DMC_LOC_MODE flag.
**
** Inputs:
**      dmc_cb				Alter control block.
**	scb				Session control block.
**
** Output:
**	dmc_cb
**	   .error.err_code		One of the following error numbers.
**					E_DM001D_BAD_LOCATION_NAME
**					E_DM002A_BAD_PARAMETER
**					E_DM0107_ERROR_ALTERING_SESSION
**					E_DM013B_LOC_NOT_AUTHORIZED
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** History:
**	 04-nov-92 (jrb)
**	    Created for ML Sorts project.
**	 15-nov-93 (swm)
**	    Bug #58628
**	    Rounded up mask_size to be a multiple of sizeof(i4) to
**	    ensure that memory allocated for masks is i4-aligned
**	    (This is relied upon in dmcopen()).
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
*/
static DB_STATUS
set_worklocs(
DMC_CB	    *dmc_cb,
DML_SCB	    *scb)
{
    DMC_CB		    *dmc = dmc_cb;
    DB_STATUS		    status;
    i4		    local_error;
    i4		    error;
    DMP_EXT		    *ext;
    DMC_LLIST_ITEM	    *q;
    DMP_LOC_ENTRY	    *p;
    i4		    np;
    DML_LOC_MASKS	    *lm;
    DMP_MISC		    *misc;
    DML_LOC_MASKS	    *tmplm;
    i4		    mask_size;
    char		    *tmp_w_allow;
    char		    *tmp_w_use;
    char		    *tmp_d_allow;

    CLRDBERR(&dmc->error);

    lm = scb->scb_loc_masks;
    mask_size = lm->locm_bits / BITSPERBYTE;
    /* lint truncation warning if size of ptr > int, but code valid */
    mask_size = DB_ALIGNTO_MACRO(mask_size, sizeof(i4));

    /* first make copy of bit fields in dml_loc_masks block */
    status = dm0m_allocate(
	(i4)(sizeof(DMP_MISC) + mask_size * 3),
	(i4)0, (i4)MISC_CB, (i4)MISC_ASCII_ID,
	(char *)scb, (DM_OBJECT **)&misc, &dmc->error);

    if (status != E_DB_OK)
    {
	uleFormat(&dmc->error, 0, NULL, ULE_LOG, NULL, (char *)NULL, 0L,
	    (i4 *)NULL, &local_error, 0);

	SETDBERR(&dmc->error, 0, E_DM0107_ERROR_ALTERING_SESSION);
	return(E_DB_ERROR);
    }
    tmplm = (DML_LOC_MASKS *) &misc[1];   /* point at memory after MISC block */
    misc->misc_data = (char*)tmplm;
    MEcopy((PTR)lm->locm_w_allow, (mask_size * 3),(PTR)tmplm);

    tmp_w_allow = (char *)tmplm;
    tmp_w_use   = (char *)tmplm + mask_size;
    tmp_d_allow = (char *)tmplm + mask_size + mask_size;

    /* for DMC_S_TRADE_LOCS we need tmp_w_use zeroed out (to drop
    ** all locations before doing any adds)
    */
    if (dmc->dmc_s_type == DMC_S_TRADE_LOCS)
	MEfill(mask_size, (u_char)0, (PTR)tmp_w_use);

    /* look through list of locations caller gave us */
    ext = ((DML_ODCB *)dmc->dmc_db_id)->odcb_dcb_ptr->dcb_ext;

    for(q = dmc->dmc_names_loc; q != NULL; q = q->dmc_llink)
    {
	p = ext->ext_entry;

	/* try and find matching entry in extent array */
	for (np = 0; np < ext->ext_count; np++, p++)
	{
	    if (MEcmp(  (PTR)q->dmc_lname.db_loc_name,
			(PTR)p->logical.db_loc_name,
			sizeof(DB_LOC_NAME)) == 0
	       )
	    {
		break;
	    }
	}
	if (np >= ext->ext_count)
	{
	    /* didn't find matching location in the extent array;
	    ** return this location to caller and set error code
	    */
	    i4		i;
	    
	    SETDBERR(&dmc->error, 0, E_DM001D_BAD_LOCATION_NAME);
	    dmc->dmc_error_loc = &q->dmc_lname;
	    for (i=sizeof(DB_LOC_NAME); i > 0; i--)
	    {
		if (((char *)dmc->dmc_error_loc)[i-1] != ' ')
		{
		    dmc->dmc_errlen_loc = i;
		    break;
		}
	    }
	    break;
	}

	/* now manipulate bits in tmplm according to requested op */
	switch (dmc->dmc_s_type)
	{
	  case DMC_S_TRADE_LOCS:
	  case DMC_S_ADD_LOCS:
	    if (BTtest((i4)np, (char *)lm->locm_w_allow))
	    {
		BTset((i4)np, (char *)tmp_w_use);
	    }
	    else
	    {
		/* attempting to add location which is either not
		** the correct type or unauthorized
		*/
		i4		i;
	    
		SETDBERR(&dmc->error, 0, E_DM013B_LOC_NOT_AUTHORIZED);
		dmc->dmc_error_loc = &q->dmc_lname;
		for (i=sizeof(DB_LOC_NAME); i > 0; i--)
		{
		    if (((char *)dmc->dmc_error_loc)[i-1] != ' ')
		    {
			dmc->dmc_errlen_loc = i;
			break;
		    }
		}
	    }
	    break;

	  case DMC_S_DROP_LOCS:
	    BTclear((i4)np, (char *)tmp_w_use);
	    break;

	  case DMC_S_NOAUTH_LOCS:
	    BTclear((i4)np, (char *)tmp_w_allow);
	    BTclear((i4)np, (char *)tmp_d_allow);
	    break;

	  default:
	    SETDBERR(&dmc->error, 0, E_DM002A_BAD_PARAMETER);
	    break;
	}
	if ( dmc->error.err_code )
	    break;
    }

    /* if no errors, copy temp bit masks back to scb */
    if ( dmc->error.err_code == 0 )
    {
	MEcopy((PTR)tmplm,
		(mask_size * 3),
		(PTR)lm->locm_w_allow);
    }

    dm0m_deallocate((DM_OBJECT **)&misc);

    return( (dmc->error.err_code) ? E_DB_ERROR : E_DB_OK );
}


/*{
** Name: dmc_set_logging - Alters session's logging state.
**
** Description:
**	This routine is used to alter the session's logging state.  It is
**	called via the statement "Set [No]Logging".
**
**	When Logging is disabled (DMC_C_OFF mode), an error message is logged
**	to the Ingres Error Log indicating that the database is in use with
**	NoLogging.
**
**	When Logging is re-enabled (DMC_C_ON mode), then if this server is
**	using Fast Commit, a Consistency Point will be taken to ensure that all
**	previous NOLOGGING updates are committed to the database.  This
**	requires us to start an internal transaction in order to wait for
**	the consistency point to complete.
**
** Inputs:
**	dmc_cb				A DMC_CONTROL_CB pointer.
**	scb				Session control block.
**	dcb				Database control block.
**	mode				Logging mode: DMC_C_ON/DMC_C_OFF.
**
** Output:
**	dmc_cb
**	    .error.err_code		One of the following error numbers.
**                                      E_DM0107_ERROR_ALTERING_SESSION
**					E_DM0147_NOLG_BACKUP_ERROR
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** History:
**	 4-feb-1991 (rogerk)
**	    Wrote for "Set Nologging" project.
**	25-feb-91 (rogerk)
**	    Fixed bug in set_logging - init status value so we don't get
**	    error when run "set logging" in non fast_commit server.
**	25-mar-91 (rogerk)
**	    Toggle NOLOGGING state in config file when transaction logging
**	    is enabled/disabled.  Took out static definition and changed
**	    routine name to dmc_set_logging to allow it to be called by
**	    dmc_end_session.
**	22-apr-91 (rogerk)
**	    Check for online backup processing on database before we switch
**	    a session to Nologging mode.  Since a checkpoint will be totally
**	    screwed by a session which runs in Set Nologging mode, we disallow
**	    using Set Nologging while running a checkpoint.
**	14-dec-92 (rogerk)
**	    Cast arguments to LGbegin, LGalter properly.
**	31-jan-1994 (bryanp) B57823, B57824, B57825
**	    If LG call fails, log LG error code.
**	    If LGbegin returns LG_EXCEED_LIMIT, return RESOURCE_QUOTA_EXCEED.
**	31-jan-94 (jnash)
**	    When SET NOLOGGING performed, disallow rollforwarddb
** 	    from a prior checkpoint.  
**      18-mar-1994 (pearl)
**          Bug 60695. Replaced the E_DM9050_TRANSACTION_NO_LOGGING message 
**          with E_DM9060_SESSION_LOGGING_OFF.  The text is similar to the old
**          message's, plus it provides the current log file EOF, and also
**          advises that the journals are incomplete until the next
**          'ckpdb +j'.
**          Added E_DM9061_JNLDB_LOGGING_ON and
**          E_DM9062_NON_JNLDB_LOGGING_ON to mark when logging is turned on,
**          and to mark the log file EOF.  The first message is for journaled
**          databases, and advises the user to run 'ckpdb +j'.  The latter
**          message is for non-journaled databases.  
**	20-Oct-2008 (jonj)
**	    SIR 120874 pass DMC_CB rather than err_code.
*/
DB_STATUS
dmc_set_logging(
DMC_CB	    *dmc,
DML_SCB	    *scb,
DMP_DCB	    *dcb,
i4	    mode)
{
    CL_ERR_DESC	    sys_err;
    DM0C_CNF	    *cnf;
    DB_OWN_NAME	    user_name;
    DB_TRAN_ID	    tran_id;
    LG_HEADER	    lgh;
    DB_STATUS	    dbstatus = E_DB_OK;
    STATUS	    status = OK;
    i4	    	    log_id;
    i4	    	    item;
    i4	    	    event;
    i4		    length;
    i4		    local_error;
    bool	    xact_open = FALSE;
    bool	    db_is_journaled = FALSE;
    bool	    logging_off = FALSE;

    CLRDBERR(&dmc->error);

    if (status = LGshow(LG_A_HEADER, (PTR)&lgh, sizeof (LG_HEADER), &length, 
		&sys_err))
    {
	uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG,
                    (DB_SQLSTATE *) NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                    &local_error, 0);
	SETDBERR(&dmc->error, 0, E_DM0107_ERROR_ALTERING_SESSION);
	return(E_DB_ERROR);
    }
    if (mode == DMC_C_OFF)
    {
	for (;;)
	{
	    /*
	    ** Check database backup state.  We do not allow the Set Nologging
	    ** statement while the database is being checkpointed.  This is
	    ** because if we make updates without logging them, then rollforward
	    ** will not be able to back out the updates to bring the database
	    ** to a consistent state.
	    */
	    dbstatus = dm2d_check_db_backup_status(dcb, &dmc->error);
	    if (dbstatus)
		break;

            if (dcb->dcb_status & DCB_S_MUST_LOG)
            {
                SETDBERR(&dmc->error, 0, E_DM0201_NOLG_MUSTLOG_ERROR);
                dbstatus = E_DB_ERROR;
                break;
            }

	    if (dcb->dcb_status & DCB_S_BACKUP)
	    {
		SETDBERR(&dmc->error, 0, E_DM0147_NOLG_BACKUP_ERROR);
		dbstatus = E_DB_ERROR;
		break;
	    }

	    /*
	    ** Log error message indicating NoLogging use on the database.
	    */
	    uleFormat(NULL, E_DM9060_SESSION_LOGGING_OFF, (CL_ERR_DESC *)NULL, 
		ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0,
		(i4 *)NULL, &local_error, 4, 
		sizeof(dcb->dcb_name.db_db_name), dcb->dcb_name.db_db_name,
		0, lgh.lgh_end.la_sequence,
                0, lgh.lgh_end.la_block,
                0, lgh.lgh_end.la_offset);
	    /*
	    ** Update config file to show NoLogging use on the database.
	    */
            dbstatus = dm0c_open(dcb, 0, scb->scb_lock_list, &cnf, &dmc->error);
            if (dbstatus != E_DB_OK)
        	break;

	    cnf->cnf_dsc->dsc_status |= DSC_NOLOGGING;

	    /*
	    ** Disable normal rollforwarddb's from a point prior to this
	    ** (except by rollforwarddb #f).
	    */
	    cnf->cnf_jnl->jnl_first_jnl_seq = 0;

	    dbstatus = dm0c_close(cnf, (DM0C_UPDATE | DM0C_COPY), &dmc->error);
	    break;
	}

	if ( dmc->error.err_code )
	{
	    if (dmc->error.err_code > E_DM_INTERNAL)
	    {
		uleFormat(&dmc->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
		    (DB_SQLSTATE *) NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &local_error, 0);
		SETDBERR(&dmc->error, 0, E_DM0107_ERROR_ALTERING_SESSION);
	    }
	    return(E_DB_ERROR);
	}

	/*
	** Set NOLOGGING status in the session control block.
	*/
	scb->scb_sess_mask |= SCB_NOLOGGING;
    }
    else
    {
	/*
	** Execute Consistency Point to make sure that all updates performed
	** using NoLogging are flushed to the database (this is only required
	** if the server is using Fast Commit).
	**
	** We must start a temporary transaction in order to wait for the
	** consistency point to complete.
	*/
	while (dcb->dcb_status & DCB_S_FASTCOMMIT)
	{
	    /*
	    ** Signal Consistency Point.
	    */
	    item = 1;
	    status = LGalter(LG_A_CPNEEDED, (PTR)&item, sizeof(item), &sys_err);
	    if (status != OK)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, 
		    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
		    &local_error, 0);
		uleFormat(&dmc->error, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, 
		    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
		    &local_error, 1, 0, LG_A_CPNEEDED);
		break;
	    }

	    /*
	    ** Start a transaction in order to wait for the CP to complete.
	    */
	    MEmove(10, ERx("$DMC_ALTER"), ' ', 
				sizeof(DB_OWN_NAME), (PTR) &user_name);
	    status = LGbegin(LG_NOPROTECT, dcb->dcb_log_id, &tran_id, &log_id,
			DB_OWN_MAXNAME, user_name.db_own_name, 
			(DB_DIS_TRAN_ID*)NULL, &sys_err);
	    if (status != E_DB_OK)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, 
		    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
		    &local_error, 0);
		uleFormat(&dmc->error, E_DM900C_BAD_LOG_BEGIN, &sys_err, ULE_LOG, 
		    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
		    &local_error, 1, 0, dcb->dcb_log_id);
		if (status == LG_EXCEED_LIMIT)
		    SETDBERR(&dmc->error, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
		break;
	    }
	    xact_open = TRUE;

	    /*
	    ** Wait until the Consistency Point is complete.
	    */
	    status = LGevent(0, log_id, LG_E_ECPDONE, &event, &sys_err);
	    if (status != OK)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, 
		    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
		    &local_error, 0);
		uleFormat(&dmc->error, E_DM900F_BAD_LOG_EVENT, &sys_err, ULE_LOG,
		    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
		    &local_error, 1, 0, LG_E_ECPDONE);
		break;
	    }

	    /*
	    ** Deallocate the temporary transaction.
	    */
	    status = LGend(log_id, 0, &sys_err);
	    if (status != OK)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, 
		    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
		    &local_error, 0);
		uleFormat(&dmc->error, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG,
		    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &local_error, 1, 0, log_id);
		break;
	    }
	    xact_open = FALSE;

	    break;
	}

	/*
	** Update Config File to show that database is no longer being
	** updated in set nologging mode.
	*/
	if (status == OK)
	{
	    dbstatus = dm0c_open(dcb, 0, scb->scb_lock_list, &cnf, &dmc->error);
	    if (dbstatus == E_DB_OK)
	    {
		if (cnf->cnf_dsc->dsc_status & DSC_NOLOGGING)
		{
		    cnf->cnf_dsc->dsc_status &= ~DSC_NOLOGGING;
		    logging_off = TRUE;
		}
		db_is_journaled = cnf->cnf_dsc->dsc_status & DSC_JOURNAL;
		dbstatus = dm0c_close(cnf, (DM0C_UPDATE | DM0C_COPY), &dmc->error);
	    }
	}

	/*
	** Log message noting that SET LOGGING command has been issued.
	** Text of the message will advise the user to perform a ckpdb on
	** the database if the db is journaled.
	*/
	
	if (logging_off)
	{
	    local_error = E_DM9061_JNLDB_SESSION_LOGGING_ON;
	    if (! db_is_journaled)
	    {
		local_error = E_DM9062_NON_JNLDB_SESSION_LOGGING_ON;
	    }
	    uleFormat(NULL, local_error, (CL_ERR_DESC *)NULL,
                	ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0,
                	(i4 *)NULL, &local_error, 4,
                	sizeof(dcb->dcb_name.db_db_name), 
			dcb->dcb_name.db_db_name,
                	0, lgh.lgh_end.la_sequence,
                	0, lgh.lgh_end.la_block,
                	0, lgh.lgh_end.la_offset);
	}

	/*
	** Check for errors from config file update.
	*/
	if (dbstatus != E_DB_OK)
	{
	    uleFormat(&dmc->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
		(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		&local_error, 0);

	    /* set status failure to trip error handling check just below. */
	    status = FAIL;
	}

	/*
	** Check for errors above - make sure we don't leave the temporary
	** transaction hanging around.
	*/
	if (status != OK)
	{
	    if (xact_open)
	    {
		status = LGend(log_id, 0, &sys_err);
		if (status != OK)
		{
		    uleFormat(NULL, status, &sys_err, ULE_LOG, 
			(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0,
			(i4 *)NULL, &local_error, 0);
		    uleFormat(&dmc->error, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG,
			(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0,
			(i4 *)NULL, &local_error, 1, 0, log_id);
		}
	    }
	    if (dmc->error.err_code != E_DM0112_RESOURCE_QUOTA_EXCEED)
		SETDBERR(&dmc->error, 0, E_DM0107_ERROR_ALTERING_SESSION);
	    return(E_DB_ERROR);
	}

	/*
	** Turn off NoLogging status in the session control block.
	*/
	scb->scb_sess_mask &= ~(SCB_NOLOGGING);
    }

    return (E_DB_OK);
}
