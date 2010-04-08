/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <cs.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <pc.h>
#include    <lg.h>
#include    <lk.h>
#include    <scf.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dmxcb.h>
#include    <dmrcb.h>
#include    <dmccb.h>
#include    <dmtcb.h>
#include    <dmucb.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dml.h>
#include    <dmxe.h>
#include    <dm0m.h>
#include    <dm2t.h>
#include    <dmfgw.h>
#include    <dmftrace.h>

/**
** Name: DMXBEGIN.C - Functions used to begin a Transaction.
**
** Description:
**      This file contains the functions necessary to begin a transaction.
**      This file defines:
**    
**      dmx_begin()        -  Routine to perform the begin 
**                            transaction operation.
**
** History:
**      01-sep-85 (jennifer)
**          Created.      
**	25-nov-1985 (derek)
**	    Completed code for dmx_begin().
**	30-sep-1988 (rogerk)
**	    Added capability to delay writing Begin Transaction record until
**	    first write operation.
**	12-Jan-1989 (ac)
**	    Added 2PC support.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.
**	11-jan-1990 (rogerk)
**	    Backed out all december 6th changes.  Now all the work for keeping
**	    track of whether a transaction is being done during an online 
**	    checkpoint is being done in dmxe_begin().
**	 4-feb-1991 (rogerk)
**	    Added support for Set Nologging.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5.  In this case,
**	    the 6.4 version of this file was moved directly to 6.5 and the
**	    6.5 changes were merged back in:
**
**	    14-jun-1991 (Derek)
**		Added performance profiling support.
**	8-jul-1992 (walt)
**	    Prototyping DMF.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	31-jan-1994 (bryanp) B58562
**	    Format scf_error.err_code if scf_call fails.
**      14-Apr-1994 (fred)
**          Added initialization of the xcb_seq_no field.  While not
**          strictly necessary since the block is zeron on allocation,
**          this is not a documented effect of the routines, so
**          purposefully initializing the field seems like a good thing.
**      04-apr-1995 (dilma04) 
**          Set DMXE_INTERNAL, XCB_USER_TRAN, according to DMX_USER_TRAN.
**	30-may-96 (stephenb)
**	    init replication input queue RCB pointer in DML_XCB.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction 
**	    access mode> (READ ONLY | READ WRITE) per SQL-92.
**	    Clarify the distinction between READ_ONLY and NOLOGGING 
**	    transactions; READ_ONLY never log, but READ_WRITE normally 
**	    log but won't if SET NOLOGGING is in effect.
**	3-jun-97 (nanpr01)
**	    Fix bug 78352 : When transactions are rolled back, temporary
**	    tables that were updated does not get dropped as documented.
**     12-nov-97 (stial01)
**          Isolation level precedence: transaction, session, system.
**     22-jan-97 (stial01)
**          B88415: access mode read only is implicit when read uncommitted
**          only if (session AND transaction) access mode not specified.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	30-Nov-1999 (jenjo02)
**	    Extract pointer to session's distributed transaction id from
**	    SCF, pass on to dmxe_begin.
**	    Added DB_DIS_TRAN_ID* to dmxe_begin() prototype.
**	08-Nov-2000 (jenjo02)
**	    Deleted xcb_tq_next|prev, xccb_tq_next|prev.
**	11-Nov-2000 (jenjo02)
**	    When starting a transaction, reset all of Session's temporary	
**	    tables savepoint id's to -1 to indicate that no updates
**	    to the table have yet been done by this transaction.
**	08-Jan-2001 (jenjo02)
**	    Get DML_SCB* from ODCB instead of calling SCU_INFORMATION.
**	    dist_tran_id is now pointed to by DML_SCB (scb_dis_tranid).
**	05-Mar-2002 (jenjo02)
**	    Init xcb_seq, xcb_cseq for Sequence Generators.
**      02-jan-2004 (stial01)
**          Added support for SET [NO]BLOBLOGGING
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      13-may-2004 (stial01)
**          Ignore NOBLOBLOGGING if using FASTCOMMIT protocols
**      27-aug-2004 (stial01)
**          Removed SET NOBLOBLOGGING statement
**      16-oct-2006 (stial01)
**          Init new field in XCB for locators.
**      09-feb-2007 (stial01)
**          Moved locator context from DML_XCB to DML_SCB
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_? functions converted to DB_ERROR *
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dmf_gw? functions converted to DB_ERROR *
[@history_template@]...
*/

/*{
** Name: dmx_begin - Begins a transaction.
**
**  INTERNAL DMF call format:      status = dmx_begin(&dmx_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMX_BEGIN,&dmx_cb);
**
** Description:
**    The dmx_begin function handles the initialization of a transaction.
**    An internal transaction identifier will be returned to the caller.  This
**    identifier is required on all table and operations which are to be
**    associated with this transaction.  No table operations other then 
**    dmt_show which describes a table can occur outside a transaction 
**    environment.  Multiple databases can be referenced within a transaction
**    but only one databases can be updated.  The database to be updated
**    is specified in the control block.  This must be used even if only
**    one database is currently open.
**
**    If the session NOLOGGING flag is set from a previous call to
**    DMC_ALTER(DMC_C_LOGGING), then all transactions will be started
**    as NOLOGGING transactions internally - although database update 
**    operations will be allowed.
**
** Inputs:
**      dmx_cb 
**          .type                           Must be set to DMX_TRANSACTION_CB.
**          .length                         Must be at least sizeof(DMX_CB).
**          .dmx_flags_mask                 Must be zero or DMX_READ_ONLY.
**          .dmx_session_id                 Must be a valid session identifier
**                                          obtained from SCF.
**          .dmx_db_id                      The database identifier of the
**                                          database in this transaction that
**                                          can be updated.
**          .dmx_option                     Must be zero, DMX_USER_TRAN and/or
**							  DMX_NOLOGGING.
**					    
** Output:
**      dmx_cb 
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**					    E_DM0010_BAD_DB_ID
**                                          E_DM001A_BAD_FLAG
**                                          E_DM002C_BAD_SAVEPOINT_NAME
**                                          E_DM002F_BAD_SESSION_ID
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**                                          E_DM0065_USER_INTR
**                                          E_DM0060_TRAN_IN_PROGRESS
**                                          E_DM0062_TRAN_QUOTA_EXCEEDED
**                                          E_DM0064_USER_ABORT
**					    E_DM0094_ERROR_BEGINNING_TRAN
**                                          E_DM0100_DB_INCONSISTENT
**                                          E_DM010C_TRAN_ABORTED
**                                          E_DM0112_RESOURCE_QUOTA_EXCEEDED
**                     
**          .dmx_tran_id                    Identifier for this transaction.
**          .dmx_phys_tran_id               Physical transaction identifier.
**
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_WARN                       Function completed normally with 
**                                          a termination status which is in 
**                                          dmx_cb.error.err_code.
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dmx_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmx_cb.error.err_code.
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**	25-nov-1985 (derek)
**	    Completed code.
**	31-aug-1988 (rogerk)
**	    Make sure to return a non-zero error status if encounter an error.
**	30-sep-1988 (rogerk)
**	    Added capability to delay writing Begin Transaction record until
**	    first write operation.  Save journal status and user name in the
**	    XCB in case they are needed later to write the BT record.
**	    Changed dmxe_begin call to use new arguments.
**	12-Jan-1989 (ac)
**	    Added 2PC support.
**	17-mar-1989 (EdHsu)
**	    Added cluster online backup support.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.  If server is connected to a
**	    gateway which requires begin/end transaction notification, then
**	    we must notify the gateway of the begin transaction : just in case
**	    a gateway query is made in this transaction.
**	 4-feb-1991 (rogerk)
**	    Added support for Set Nologging.  Check session scb mask for
**	    SCB_NOLOGGING.  If user has requested No Logging, then all
**	    transactions are begun as READ transactions (while still being
**	    UPDATE xacts).  Mark the Nologging state in the XCB for future 
**	    reference.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5.  In this case,
**	    the 6.4 version of this file was moved directly to 6.5 and the
**	    6.5 changes were merged back in:
**
**	    14-jun-1991 (Derek)
**		Added performance profiling support.
**	31-jan-1994 (bryanp) B58562
**	    Format scf_error.err_code if scf_call fails.
**      14-Apr-1994 (fred)
**          Added initialization of the xcb_seq_no field.  While not
**          strictly necessary since the block is zeron on allocation,
**          this is not a documented effect of the routines, so
**          purposefully initializing the field seems like a good thing.
**      04-apr-1995 (dilma04)
**          Set DMXE_INTERNAL, XCB_USER_TRAN, according to DMX_USER_TRAN.
**	30-may-96 (stephenb)
**	    init replication input queue RCB pointer in DML_XCB.
**	5-aug-96 (stephenb)
**	    initialize xcb_rep_remote_tx for replication.
**	21-Jan-1997 (jenjo02)
**	    Remove DM0M_ZERO from dm0m_allocate() for XCB, manually
**	    initialize remaining fields instead.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction 
**	    access mode> (READ ONLY | READ WRITE) per SQL-92.
**	    Clarify the distinction between READ_ONLY and NOLOGGING 
**	    transactions; READ_ONLY never log, but READ_WRITE normally 
**	    log but won't if SET NOLOGGING is in effect.
**	3-jun-97 (nanpr01)
**	    Fix bug 78352 : When transactions are rolled back, temporary
**	    tables that were updated does not get dropped as documented.
**     12-nov-97 (stial01)
**          Isolation level precedence: transaction, session, system.
**     22-jan-97 (stial01)
**          B88415: access mode read only is implicit when read uncommitted
**          only if (session AND transaction) access mode not specified.
**	30-Nov-1999 (jenjo02)
**	    Extract pointer to session's distributed transaction id from
**	    SCF, pass on to dmxe_begin.
**	    Added DB_DIS_TRAN_ID* to dmxe_begin() prototype.
**	08-Jan-2001 (jenjo02)
**	    Find DML_SCB* with macro instead of calling SCU_INFORMATION.
**	    dist_tran_id is now pointed to by DML_SCB (scb_dis_tranid).
**	29-Jan-2004 (jenjo02)
**	    Added support for DMX_COMMIT to connect to an existing
**	    transaction context.
**	6-Feb-2004 (schka24)
**	    Get rid of DMU count.
**	30-Apr-2004 (schka24)
**	    Set XCB flag for shared transaction, so that disconnect
**	    just disconnects.
**	04-May-2004 (jenjo02)
**	    Copy "connect to" SCB trace bits to this SCB.
**	05-May-2004 (jenjo02)
**	    SCBs for Factotum threads are not allocated an
**	    scb_lock_list until a DMX_CONNECT is issued;
**	    at that time, we connect to the "parent" SCB's
**	    scb_lock_list.
**	10-May-2004 (schka24)
**	    Init PCB cleanup list.
**	01-Jul-2004 (jenjo02)
**	    Add scb_loc_masks to list of things to copy when
**	    CONNECTing.
**	09-Dec-2004 (jenjo02)
**	    Added a bunch of stuff to be propagated to a
**	    CONNECTed SCB from its Parent, notably locking
**	    characteristics, and anything else that could
**	    be changed by SET SESSION.
**	11-Jan-2005 (jenjo02)
**	    Whoops, "|" isn't the same as "|=" when propagating
**	    scb_sess_mask, scb_s_type.
**	18-Feb-2005 (thaju02)
**	    Do not overwrite just-alloc'ed slcb's slcb_length with
**	    that of the cslcb's. (B113921)
**	17-Aug-2005 (jenjo02)
**	    Delete db_tab_index from DML_SLCB. Lock semantics apply
**	    universally to all underlying indexes/partitions.
**	29-Jun-2006 (jonj)
**	    Add support for xa_start(), xa_start(JOIN), explicit
**	    start transaction.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Allocate memory for LG_CRIB, xid array,
**	    initialize xcb_crib_ptr, LG_CRIB when MVCC-ing.
**	26-Feb-2010 (jonj)
**	    Init crib_next, crib_prev, crib_cursid for cursor
**	    cribs.
**	23-Mar-2010 (kschendel) SIR 123448
**	    XCB's XCCB list now has to be mutex protected, init mutex.
*/
DB_STATUS
dmx_begin(
DMX_CB    *dmx_cb)
{
    char		sem_name[10+16+10+1];	/* last +10 is slop */
    DMX_CB		*dmx = dmx_cb;
    DML_SCB		*scb, *cscb;
    DML_ODCB		*odcb;
    DML_XCB		*x, *cx;
    DML_XCB		*xcb = (DML_XCB *)0;
    i4			btflags;
    i4			xact_mode;
    i4             	xact_iso;
    i4			error,local_error;
    DB_STATUS		status;
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    DML_XCCB		*xccb;
    DML_SLCB		*cslcb, *slcb;
    DB_DIS_TRAN_ID	*DisTranId;
    i4			length;
    i4			memsize;
    bool		UseMVCC = FALSE;

    CLRDBERR(&dmx->error);

    for (;;)
    {

	/* Check for valid options */
	if (dmx->dmx_option & ~(DMX_USER_TRAN | DMX_NOLOGGING | DMX_CONNECT |
				DMX_XA_START_XA | DMX_XA_START_JOIN))
	{
	    SETDBERR(&dmx->error, 0, E_DM001A_BAD_FLAG);
	    break;
	}

	/* 
	** Get the DML_SCB for the session; this may not
	** be the same as "odcb_scb_ptr" if CONNECT.
	*/
	if ( !(scb = GET_DML_SCB(dmx->dmx_session_id)) ||
	      dm0m_check((DM_OBJECT *)scb, (i4)SCB_CB) )
	{
	    SETDBERR(&dmx->error, 0, E_DM002F_BAD_SESSION_ID);
	}

	if ( scb->scb_x_ref_count )
	{
	    SETDBERR(&dmx->error, 0, E_DM0060_TRAN_IN_PROGRESS);
	    break;
	}

	/* Assume distributed tran id from "set session with xa_xid=", if any */
	if ( dmx->dmx_option & DMX_USER_TRAN )
	    DisTranId = scb->scb_dis_tranid;
	else
	    DisTranId = (DB_DIS_TRAN_ID*)NULL;

	/*
	** If CONNECTing to an existing transaction context,
	** reverse-engineer the mode and flags for 
	** dmxe_begin(). Note that this may also set some
	** logging flags in the session's SCB...
	*/
	if ( dmx->dmx_option & DMX_CONNECT )
	{
	    if ( (cx = (DML_XCB*)dmx->dmx_tran_id) &&
		 (dm0m_check((DM_OBJECT *)cx, (i4)XCB_CB) == 0) &&
		 (cx->xcb_state & XCB_ABORT) == 0 )
	    {
		btflags = DMXE_CONNECT;

		/* Discard any options but CONNECT */
		dmx->dmx_option &= DMX_CONNECT;

		if ( cx->xcb_flags & XCB_DELAYBT )
		    btflags |= DMXE_DELAYBT;

		if ( (cx->xcb_flags & XCB_USER_TRAN) == 0 )
		    btflags |= DMXE_INTERNAL;

		if ( cx->xcb_x_type & XCB_RONLY )
		    xact_mode = DMXE_READ;
		else
		    xact_mode = DMXE_WRITE;

		/*
		** Modify SCB to inherit any changes that
		** have have been made to the Parent's 
		** session via SET SESSION/TRANSACTION, etc.
		**
		** Additions to dmc_alter should be
		** reflected here...
		*/

		/* cscb = the "parent" session's SCB */
		cscb = cx->xcb_scb_ptr;

		/* Copy the session trace bits */
		MEcopy((char*)&cscb->scb_trace, 
			sizeof(cscb->scb_trace),
			(char*)&scb->scb_trace);
		scb->scb_loc_masks = cscb->scb_loc_masks;
		scb->scb_lock_level = cscb->scb_lock_level;
		scb->scb_timeout = cscb->scb_timeout;
		scb->scb_maxlocks = cscb->scb_maxlocks;
		scb->scb_readlock = cscb->scb_readlock;
		scb->scb_tran_iso = cscb->scb_tran_iso;
		scb->scb_tran_am = cscb->scb_tran_am;
		scb->scb_sess_iso = cscb->scb_sess_iso;
		scb->scb_sess_am = cscb->scb_sess_am;
		scb->scb_audit_state = cscb->scb_audit_state;
		scb->scb_sess_mask |= (cscb->scb_sess_mask &
					(SCB_NOLOGGING |
					 SCB_LOGFULL_COMMIT |
					 SCB_LOGFULL_NOTIFY));
		scb->scb_s_type |= (cscb->scb_s_type & 
					(SCB_S_SYSUPDATE |
					 SCB_S_SECURITY));

		/* 
		** If the Parent has a list of SLCBs
		** (table-level lock overrides), make
		** a copy of each for this SCB.
		*/
		for ( cslcb = cscb->scb_kq_next;
		      cslcb != (DML_SLCB*)&cscb->scb_kq_next;
		      cslcb = cslcb->slcb_q_next )
		{
		    status = dm0m_allocate(
				(i4)sizeof(DML_SLCB),
				0, 
				(i4)SLCB_CB,
				(i4)SLCB_ASCII_ID, 
				(char *)scb, (DM_OBJECT **)&slcb, &dmx->error);
		    if ( status == E_DB_OK )
		    {
		 	slcb->slcb_scb_ptr = cslcb->slcb_scb_ptr;
			slcb->slcb_db_tab_base = cslcb->slcb_db_tab_base;
			slcb->slcb_timeout = cslcb->slcb_timeout;
			slcb->slcb_maxlocks = cslcb->slcb_maxlocks;
			slcb->slcb_lock_level = cslcb->slcb_lock_level;
			slcb->slcb_readlock = cslcb->slcb_readlock;

			/* Queue at tail of queue. */
			slcb->slcb_q_next = scb->scb_kq_prev->slcb_q_next;
			slcb->slcb_q_prev = scb->scb_kq_prev;
			scb->scb_kq_prev->slcb_q_next = slcb;
			scb->scb_kq_prev = slcb;
		    }
		    else
		    {
			uleFormat( &dmx->error, 0, NULL, ULE_LOG , NULL, 
			    (char * )NULL, 0L, (i4 *)NULL, 
			    &error, 0);
			SETDBERR(&dmx->error, 0, E_DM0094_ERROR_BEGINNING_TRAN);
			break;
		    }
		}
	    }
	    else
	    {
		SETDBERR(&dmx->error, 0, E_DM003B_BAD_TRAN_ID);
		break;
	    }
	}
	else
	{
	    /*
	    ** Set delay bt flag to delay the writing of the 
	    ** Begin Transaction record until
	    ** the first write operation is done.
	    */
	    btflags = DMXE_DELAYBT;
	    if ((dmx->dmx_option & DMX_USER_TRAN) == 0)
		btflags |= DMXE_INTERNAL;
	    if (dmx->dmx_option & DMX_NOLOGGING)
		btflags |= DMXE_NOLOGGING;

	    /* Pass xa_start flags, dist tran id, to dmxe */
	    if ( dmx->dmx_option & DMX_XA_START_XA )
	    {
		/* Distributed tran id supplied by xa_start param */
		DisTranId = &dmx->dmx_dis_tran_id;

		btflags |= DMXE_XA_START_XA;
		if ( dmx->dmx_option & DMX_XA_START_JOIN )
		    btflags |= DMXE_XA_START_JOIN;
	    }
     
	    /*
	    ** Unless caller explicitly asked for READ_ONLY,
	    ** set access mode to READ_WRITE.
	    ** If a set session/transaction READ_ONLY was issued, this
	    ** will be overriden below.
	    */
	    if (dmx->dmx_flags_mask == 0)
		xact_mode = DMXE_WRITE;
	    else if (dmx->dmx_flags_mask == DMX_READ_ONLY)
		xact_mode = DMXE_READ;
	    else
	    {
		SETDBERR(&dmx->error, 0, E_DM001A_BAD_FLAG);
		break;
	    }
	}

	/*
	** If the session is running with SET NOLOGGING, then
	** log this transaction as NOLOGGING.  This will
	** disallow the writing of any log records.
	*/
	if (scb->scb_sess_mask & SCB_NOLOGGING)
	    btflags |= DMXE_NOLOGGING;

	if ( (odcb = (DML_ODCB *)dmx->dmx_db_id) &&
	     (dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB) == 0) )
	{
	    /* Clear user interrupt state in SCB. */
	    scb->scb_ui_state &= ~SCB_USER_INTR;

	    if (odcb->odcb_dcb_ptr->dcb_status & DCB_S_JOURNAL)
		btflags |= DMXE_JOURNAL;

	    /*
	    ** If starting a user transaction in a MVCC-capable 
	    ** database, include space for a LG_CRIB and xid_array,
	    ** but only if any lock level that might be picked
	    ** for this transaction is MVCC.
	    **
	    ** By the time a transaction is started, all of this
	    ** set lockmode stuff has been figured out and can't be
	    ** changed during the life of the transaction.
	    **
	    ** If connecting to an extant transaction, the CRIB,
	    ** if any, will be shared.
	    */
	    UseMVCC = FALSE;
	    memsize = sizeof(DML_XCB);

	    if ( odcb->odcb_dcb_ptr->dcb_status & DCB_S_MVCC &&
	         dmx->dmx_option & DMX_USER_TRAN &&
		 !(btflags & DMXE_CONNECT) )
	    {
		/* Check system default or session override */
		if ( (dmf_svcb->svcb_lk_level == DMC_C_MVCC &&
		      scb->scb_lock_level == DMC_C_SYSTEM)
		    ||
		      scb->scb_lock_level == DMC_C_MVCC )
		{
		    UseMVCC = TRUE;
		}
		else for (slcb = scb->scb_kq_next;
			  slcb != (DML_SLCB*)&scb->scb_kq_next;
			  slcb = slcb->slcb_q_next )
		{
		    /* Check for table-specific override */
		    if ( slcb->slcb_lock_level == DMC_C_MVCC )
		    {
			UseMVCC = TRUE;
			break;
		    }
		}

		if ( UseMVCC )
		    memsize += sizeof(LG_CRIB) + 
		    	       dmf_svcb->svcb_xid_array_size;
	    }

	    status = dm0m_allocate(memsize, 0,
		(i4)XCB_CB, (i4)XCB_ASCII_ID, (char *)scb, 
		(DM_OBJECT **)&xcb, &dmx->error);

	    if ( status == E_DB_OK )
	    {
		x = xcb;
		dm0s_minit(&x->xcb_cq_mutex,
			STprintf(sem_name, "XCB cq %p", x));

		/*
		** If the user has issued a 
		** "set transaction <access mode>" or
		** "set session <access mode>",
		** then override any earlier-set mode,
		** but only for user transactions.
		** Set access modes do not apply 
		** to internal transactions.
		**
		** Transaction-level mode overrides session-level.
		**
		** If transaction access mode has not been 
		** specified and transaction isolation mode 
		** is READ_UNCOMMITTED, then READ_ONLY is implicit.
		**
		** Note that scb_sess_am and scb_tran_am
		** values of DMC_C_READ_ONLY and DMC_C_READ_WRITE
		** (must) match DMXE_READ and DMXE_WRITE.
		*/
		if ( (dmx->dmx_option & (DMX_USER_TRAN | DMX_CONNECT))
			== DMX_USER_TRAN )
		{
		    /* Make sure scb_tran_iso is set */
		    xact_iso = scb->scb_tran_iso;
		    if (xact_iso == DMC_C_SYSTEM)
		    {
			xact_iso = scb->scb_sess_iso;
			if (xact_iso == DMC_C_SYSTEM)
			    xact_iso = dmf_svcb->svcb_isolation;
		    }

		    if (scb->scb_sess_am || scb->scb_tran_am)
		    {
			if (scb->scb_sess_am)
			    xact_mode = scb->scb_sess_am;
			if (scb->scb_tran_am)
			    xact_mode = scb->scb_tran_am;
		    }
		    else if (xact_iso == DMC_C_READ_UNCOMMITTED)
			xact_mode = DMXE_READ;
		}

		/* 
		** If begin a user transaction, then pass the
		** distributed transaction id to the logging system.
		*/			

		/* 
		** If CONNECTing to an existing context, pass
		** the log/lock contexts to dmxe_begin().
		**
		** On return, these will be set to
		** this session's handles to the contexts.
		**
		** Note that the connected XCB duplicates the 
		** context's physical transaction id.
		*/
		if ( btflags & DMXE_CONNECT )
		{
		    x->xcb_log_id = cx->xcb_log_id;
		    x->xcb_lk_id = cx->xcb_lk_id;
		    x->xcb_tran_id = cx->xcb_tran_id;

		    /*
		    ** Connect this session to the parent session's
		    ** scb_lock_list. Sanity check both.
		    */
		    if ( cscb->scb_lock_list && scb->scb_lock_list == 0 )
		    {
			cl_status = LKconnect((LK_LLID)cscb->scb_lock_list,
					      (LK_LLID*)&scb->scb_lock_list,
					      &sys_err);
			if (cl_status != OK)
			{
			    /*  Log the locking error. */

			    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
				(char *)NULL, 0L, (i4 *)NULL, &error, 0);
			    uleFormat(NULL, E_DM901A_BAD_LOCK_CREATE, &sys_err, 
				ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL, 
				&error, 0);
			    SETDBERR(&dmx->error, 0, E_DM0094_ERROR_BEGINNING_TRAN);
			    break;
			}
		    }
		}
    
		status = dmxe_begin(xact_mode, btflags,
		    odcb->odcb_dcb_ptr, odcb->odcb_dcb_ptr->dcb_log_id,
		    &scb->scb_user, scb->scb_lock_list, &x->xcb_log_id, 
		    &x->xcb_tran_id, &x->xcb_lk_id, 
		    DisTranId, &dmx->error);

		if ( status == E_DB_OK )
		{
		    /*
		    ** If this server is connected to a gateway, then notify
		    ** the gateway of the transaction begin.
		    */
		    if (dmf_svcb->svcb_gw_xacts)
		    {
			status = dmf_gwx_begin(scb, &dmx->error);
			if (status != E_DB_OK)
			    break;
		    }

		    x->xcb_x_type = XCB_RONLY;
		    if (xact_mode == DMXE_WRITE)
			x->xcb_x_type = XCB_UPDATE;
		    x->xcb_state = 0;
		    x->xcb_flags = 0;
		    if (btflags & DMXE_JOURNAL)
			x->xcb_flags |= XCB_JOURNAL;
		    if ((btflags & DMXE_INTERNAL) == 0)
			x->xcb_flags |= XCB_USER_TRAN;
		    if (dmx_cb->dmx_option & DMX_CONNECT)
			x->xcb_flags |= XCB_SHARED_TXN;
		    x->xcb_q_next = scb->scb_x_next;
		    x->xcb_q_prev = (DML_XCB*)&scb->scb_x_next;
		    scb->scb_x_next->xcb_q_prev = xcb;
		    scb->scb_x_next = x;
		    x->xcb_scb_ptr = scb;
		    x->xcb_seq_no = 0;
		    x->xcb_rq_next = (DMP_RCB*) &x->xcb_rq_next;
		    x->xcb_rq_prev = (DMP_RCB*) &x->xcb_rq_next;
		    x->xcb_sq_next = (DML_SPCB*) &x->xcb_sq_next;
		    x->xcb_sq_prev = (DML_SPCB*) &x->xcb_sq_next;
		    x->xcb_cq_next = (DML_XCCB*) &x->xcb_cq_next;
		    x->xcb_cq_prev = (DML_XCCB*) &x->xcb_cq_next;
		    x->xcb_odcb_ptr = odcb;
		    dmx->dmx_tran_id = (char *)x;
		    STRUCT_ASSIGN_MACRO(x->xcb_tran_id, 
				    dmx->dmx_phys_tran_id);
		    scb->scb_x_ref_count++;

		    /*
		    ** If the session is running with SET NOLOGGING, then
		    ** mark the transaction accordingly.
		    */
		    if (scb->scb_sess_mask & SCB_NOLOGGING)
			x->xcb_flags |= XCB_NOLOGGING;

		    /* Initialize remaining XCB fields */
		    if ( DisTranId )
			STRUCT_ASSIGN_MACRO(*DisTranId, x->xcb_dis_tran_id);
		    else
			MEfill(sizeof(x->xcb_dis_tran_id), 0, &x->xcb_dis_tran_id);
		    x->xcb_sp_id = 0;
		    x->xcb_s_open = 0;
		    x->xcb_s_fix = 0;
		    x->xcb_s_get = 0;
		    x->xcb_s_replace = 0;
		    x->xcb_s_delete = 0;
		    x->xcb_s_insert = 0;
		    x->xcb_s_cpu = 0;
		    x->xcb_s_dio = 0;
		    x->xcb_s_rc_retry = 0;
		    x->xcb_seq  = (DML_SEQ*)NULL;
		    x->xcb_cseq = (DML_CSEQ*)NULL;
		    x->xcb_pcb_list = NULL;
		    x->xcb_crib_ptr = (LG_CRIB*)NULL;
		    x->xcb_lctx_ptr = (DM_OBJECT*)NULL;
		    x->xcb_jctx_ptr = (DM_OBJECT*)NULL;

		    if ( btflags & DMXE_CONNECT )
		    {
			/* Point to parent session's CRIB, if any */
			x->xcb_crib_ptr = cx->xcb_crib_ptr;
			/* Copy parent session's log_id.id_id */
			x->xcb_slog_id_id = LOG_ID_ID(cx->xcb_log_id);
		    }
		    else
		    {
			/* Not shared txn, slog_id_id == log_id.id_id */
			x->xcb_slog_id_id = LOG_ID_ID(x->xcb_log_id);
			/*
			** Reset savepoint ids in all of session's
			** temporary tables. If any of these tables
			** get updated by this transaction, this
			** savepoint id will get updated as well.
			*/
			for ( xccb = odcb->odcb_cq_next;
			      xccb != (DML_XCCB*)&odcb->odcb_cq_next;
			      xccb = xccb->xccb_q_next )
			{
			    if ( xccb->xccb_operation == XCCB_T_DESTROY )
				xccb->xccb_sp_id = -1;
			}

			/*
			** If starting a user transaction with MVCC potential,
			** point xcb_crib_ptr to the LG_CRIB allocated
			** after the XCB, then point the CRIB to the
			** xid_array memory just after the CRIB.
			**
			** Set the start of transaction point-in-time
			** in the CRIB.
			**
			** When the isolation level is READ_COMMITTED,
			** the CRIB is refreshed at the beginning of each statement
			** (see dmt_open).
			*/
			if ( UseMVCC )
			{
			    /* Point to the CRIB */
			    x->xcb_crib_ptr = (LG_CRIB*)((PTR)x + sizeof(DML_XCB));

			    /* Initialize those parts not init'd by LG_S_CRIB_BT */
			    x->xcb_crib_ptr->crib_next =
				x->xcb_crib_ptr->crib_prev = xcb->xcb_crib_ptr;
			    x->xcb_crib_ptr->crib_cursid = NULL;
			    x->xcb_crib_ptr->crib_inuse = 0;

			    /* Point the CRIB to its xid_array */
			    x->xcb_crib_ptr->crib_xid_array = 
			    	(u_i4*)((PTR)x->xcb_crib_ptr + sizeof(LG_CRIB));

			    /* Provide a txn handle to LGshow */
			    x->xcb_crib_ptr->crib_log_id = x->xcb_log_id;

			    cl_status = LGshow(LG_S_CRIB_BT, (PTR)x->xcb_crib_ptr,
					       sizeof(LG_CRIB), &length, &sys_err);
			    if ( cl_status )
			    {
				uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
				    (char *)NULL, 0L, (i4 *)NULL, &error, 0);
				SETDBERR(&dmx->error, 0, E_DM0094_ERROR_BEGINNING_TRAN);
				break;
			    }
			}
		    }

#ifdef xDEBUG
		    /*
		    ** Initialize transaction statistics.
		    */
		    if (DMZ_SES_MACRO(20))
			dmd_txstat(x, 0);
#endif

		    /*
		    ** initialize replication input queue rcb and remote
		    ** TX id
		    */
		    x->xcb_rep_input_q = NULL;
		    x->xcb_rep_remote_tx = 0;
		    /*
		    ** Init sequence counter for replicator
		    */
		    x->xcb_rep_seq = 1;
		    /*
		    ** If we have not written the Begin Transaction record
		    ** then save away the username so that it can be used
		    ** when the BT record is written.
		    */
		    if (btflags & DMXE_DELAYBT)
		    {
			x->xcb_flags |= XCB_DELAYBT;
			STRUCT_ASSIGN_MACRO(scb->scb_user, x->xcb_username);
		    }
		    else
			MEfill(sizeof(x->xcb_username), 0, &x->xcb_username);
		}
	    }
	}
	else
	    SETDBERR(&dmx->error, 0, E_DM0010_BAD_DB_ID);

	break;
    }

    if ( dmx->error.err_code )
    {
	if ( xcb )
	{
	    dm0s_mrelease(&xcb->xcb_cq_mutex);
	    dm0m_deallocate((DM_OBJECT **)&xcb);
	}
	
	if (dmx->error.err_code > E_DM_INTERNAL)
	{
	    uleFormat( &dmx->error, 0, NULL, ULE_LOG , NULL, 
		(char * )NULL, 0L, (i4 *)NULL, 
		&local_error, 0);
	    SETDBERR(&dmx->error, 0, E_DM0094_ERROR_BEGINNING_TRAN);
	}
    }

    return ((dmx->error.err_code) ? E_DB_ERROR : E_DB_OK);
}
