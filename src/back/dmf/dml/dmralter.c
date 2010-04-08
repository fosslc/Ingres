/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <pc.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <lg.h>
#include    <lk.h>
#include    <dmrcb.h>
#include    <dmccb.h>
#include    <dmxcb.h>
#include    <dmtcb.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dm2t.h>
#include    <dm0m.h>
#include    <dmpp.h>
#include    <dmftrace.h>
#include    <dm1b.h>

/**
** Name: DMRALTER.C - Implements the DMF alter record-access operation.
**
** Description:
**      This file contains the functions necessary to allow a caller to
**	alter the characteristics of its access to a table which it has
**	already opened.
**
**      This file defines:
**
**      dmr_alter - Alter access to a table.
**
** History:
**	 7-jan-1990 (rogerk)
**	    Created.
**	13-apr-1992 (bryanp)
**	    B42225: only use the bottom 16 bits for rcb_seq_number.
**	 8-jul-1992 (walt)
**	    Prototyping DMF.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system typedefs
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	18-may-95 (lewda02/shust01)
**	    RAAT API Project:
**	    Add control over table lock characteristics.
**	 7-jun-95 (shust01)
**	    RAAT API Project:
**	    Add code to get/set RCB rcb_lowtid to force reposition of 
**          current record.  Needed for prefetching.
**          Also added code to set RCB current_tid, in case lock escalation
**          and then fetch current.  RCB must be at what user believes to be
**          current record.
**	31-jan-96 (thaju02)
**          RAAT API Project:
**	    Add code to set DMP_RCB's rcb_logging field, when escalating lock.
**	16-oct-96 (cohmi01)
**	    RAAT API Project: Add DMR_GET_DIRTYREAD request.
**      06-mar-1996 (stial01)
**          Variable Page Size, user relpgsize instead of sizeof(DMPP_PAGE)
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          - replace dm2t_check_setlock() call with  dmt_check_table_lock()
**          and dmt_set_lock_values();
**          - replace lock_duration with iso_level and DM2T_NOREADLOCK with
**          DMC_C_READ_UNCOMMITTED;
**          - set rcb_iso_level in the RCB.
**      10-apr-97 (dilma04)
**          Set Lockmode Cleanup.
**          When altering lock strategy of open table:
**          - initialize dmt_lock_mode field of DMT_CB;
**          - do not pick up transaction isolation level.
**      28-may-97 (dilma04)
**          Added support for RAAT row locking flags.
**      13-jun-97 (dilma04)
**          Removed from dmr_alter() redundant code to support isolation 
**          levels' trace points, because now they are supported in 
**          dmt_set_lock_values().
**      14-oct-97 (stial01)
**          dmr_alter() Changes to distinguish readlock value/isolation level
**      18-nov-97 (stial01)
**          dmr_alter() Added est_pages=0 parameter to dmt_set_lock_values.
**      01-dec-97 (stial01)
**          dmr_alter() Added DMR_SET_LOWKEY for RAAT access
**	10-dec-97 (mcgem01)
**	    Fixed NT compilation error caused by failure to include me.h
**      02-feb-98 (stial01)
**          Back out 18-nov-97 change.
**	24-mar-1998 (somsa01)
**	    At this point, we have a valid rcb for the table (since it's open).
**	    Therefore, rather than re-figuring out the locking scheme for
**	    maxlocks/timeout (which was wrong to begin with, since the session
**	    values were not being taken into account), obtain these values
**	    from the rcb. The values in the rcb were arrived upon by a
**	    "formula" in dmt_open(). Also added lock_level and iso_level.
**	    (Bug 89922)
**      24-mar-98 (thaju02)
**          bug #86326 - RAAT API, using set NOLOGGING, when escalating table
**          lock, neglected to avoid logging if NOLOGGING specified.
**      28-may-98 (stial01)
**          dmr_alter() moved code to dm2t_row_lock()
**      04-May-99 (shust01)
**	    MK user getting 197650 (Ingres error 196650, Corresponds to
**	    E_DM002A) when doing a position, get, get prev on unique btree 
**	    secondary index.  Caused by check in DMR_SET_LOWKEY.  Was checking 
**	    that the size of the key value passed in was equal to tcb_klen 
**	    (which might also include the tidp length). Change to make sure
**	    that the key size does not exceed tcb_klen. bug #96814.
**      25-May-00 (shust01)
**	    When using MK, it is possible to not receive a row level lock,
**	    when one was expected. bug #101373.
**	29-nov-2000 (shust01)
**	    Using RAAT, you can get serializable (SIX) page locks when running,
**	    instead of IX locks.  This causes other users to potentially get
**	    lock timeouts when trying to access the same page, when using
**	    row-level locking. (bug 103127, INGSRV 1306).
**      12-nov-1999 (stial01)
**          Changes to how btree key info is stored in tcb
**      27-jan-2000 (stial01)
**          btree reposition changes (rcb_repos*)
**      05-may-2000 (stial01 for shust01)
**          Changes for B101373
**      05-jun-2000 (stial01)
**          Added consistency check for DMR_SEQUENCE_NUMBER != 0
**      06-jul-2000 (stial01)
**          Changes to btree leaf/index klen in tcb (b102026)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      25-aug-2000 (stial01)
**          Removed 05-jun-2000 consistency check (b102186)
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**      01-feb-2001 (stial01)
**          Init new rcb field (Variable Page Type SIR 102955)
**      08-jul-2002 (shust01)
**          error E_DM93F5_ROW_LOCK_PROTOCOL error received when a lock
**          escalation precedes a delete, using the RAAT interface (b108202).
**      29-sep-04 (stial01)
**          Allocate extra rnl buffer if DMPP_VPT_PAGE_HAS_SEGMENTS
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2t_?, dm2r_? functions converted to DB_ERROR *
**      24-nov-2009 (stial01)
**          Check if table has segmented rows (instead of if pagetype supports)
**      11-jan-2010 (stial01)
**          Use renamed position information in rcb
**/

/*{
** Name:  dmr_alter - Alter access characteristics to table.
**
**  INTERNAL DMF call format:      status = dmr_alter(&dmr_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMR_ALTER, &dmr_cb);
**
** Description:
**
**	This routine allows the caller to alter the charateristics of its
**	access to a table which it has already opened.
**
**	When a client of DMF opens a table, it specifies access modes and
**	characteristics to be associated with its access to the table.  The
**	client is returned a record-access id which uniquely identifies that
**	instance of the open table.
**
**	If the caller needs to alter some access characteristic of its
**	open-table instance, it can do so through this routine.  This keeps
**	the caller from having to close and reopen the table to get new
**	access characteristics.
**
**	A particular characteristic is altered by specifying it in a
**	dmr_char_array entry.  The char_value field should specify the
**	new value for the characteristic.
**
**	The characteristics which may be altered through this routine are:
**
**	    DMR_SEQUENCE_NUMBER :
**
**		This is the statement sequence number to use for operations on
**		this table.  The sequence # is used to enforce deferred-cursor
**		semantics.
**
**		Altering the sequence number of a cursor that was opened with
**		the DMT_U_DEFERRED option will make visible rows which were
**		updated using the old sequence number.
**
**		When this option is specified, the char_value should be an
**		integer giving the new sequence number.
**
**
** Inputs:
**      dmr_cb
**          .type                           Must be set to DMR_RECORD_CB
**          .length                         Must be at least 
**					    sizeof(DMR_RECORD_CB) bytes.
**          .dmr_flags_mask                 Must be zero.
**          .dmr_access_id                  Record access identifer returned
**                                          from DMT_OPEN call.
**          .dmr_char_array.data_address    Pointer to array of characteristics.
**          .dmr_char_array.data_in_size    Length of dmr_char_array in bytes.
**
**          <dmr_char_array> is of type DMR_CHAR_ENTRY
**		char_id                     Characteristic identifier.  One of:
**						DMR_SEQUENCE_NUMBER
**		char_value                  Value of characteristic.
**
** Outputs:
**      dmr_cb
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000D_BAD_CHAR_ID
**                                          E_DM000E_BAD_CHAR_VALUE
**                                          E_DM002B_BAD_RECORD_ID
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM0139_ERROR_ALTERING_RCB
**
**      Returns:
**         E_DB_OK                          Function completed normally.
**         E_DB_WARN                        Function completed normally with a 
**                                          termination status which is in
**                                          dmr_cb.err_code.
**         E_DB_ERROR                       Function completed abnormally 
**                                          with a termination status which is
**                                          is dmr_cb.err_code.
**         E_DB_FATAL			    Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmr_cb.err_code.
** History:
**       7-jan-1990 (rogerk)
**	    Created.
**	13-apr-1992 (bryanp)
**	    B42225: only use the bottom 16 bits for rcb_seq_number.
**	18-may-95 (lewda02/shust01)
**	    RAAT API Project:
**	    Add control over table lock characteristics.
**	 7-jun-95 (shust01)
**	    RAAT API Project:
**	    Add code to get/set RCB rcb_lowtid to force reposition of 
**          current record.  Needed for prefetching.
**          Also added code to set RCB current_tid, in case lock escalation
**          and then fetch current.  RCB must be at what user believes to be
**          current record.
**	31-jan-96 (thaju02)
**          RAAT API Project:
**	    Add code to set DMP_RCB's rcb_logging field, 
**	    when escalating to exclusive lock.
**	16-oct-96 (cohmi01)
**	    RAAT API Project: Add DMR_GET_DIRTYREAD request.
**      06-mar-1996 (stial01)
**          Variable Page Size, user relpgsize instead of sizeof(DMPP_PAGE)
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          - replace dm2t_check_setlock() call with  dmt_check_table_lock()
**          and dmt_set_lock_values();
**          - replace lock_duration with iso_level and DM2T_NOREADLOCK with
**          DMC_C_READ_UNCOMMITTED;
**          - set rcb_iso_level in the RCB.
**      10-apr-97 (dilma04)
**          Set Lockmode Cleanup. 
**          When altering lock strategy of open table:
**          - initialize dmt_lock_mode field of DMT_CB;  
**          - do not pick up transaction isolation level. 
**      28-may-97 (dilma04)
**          Added support for RAAT row locking flags. Made the code here
**          consistent with the appropriate code in dm2t.c.             
**      13-jun-97 (dilma04)
**          Removed code to support isolation levels' trace points, because
**          now they are supported in dmt_set_lock_values().
**      14-oct-97 (stial01)
**          dmr_alter() Changes to distinguish readlock value/isolation level
**      18-nov-97 (stial01)
**          dmr_alter() Added est_pages=0 parameter to dmt_set_lock_values.
**      01-dec-97 (stial01)
**          dmr_alter() Added DMR_SET_LOWKEY for RAAT access
**      02-feb-98 (stial01)
**          Back out 18-nov-97 change.
**	24-mar-1998 (somsa01)
**	    At this point, we have a valid rcb for the table (since it's open).
**	    Therefore, rather than re-figuring out the locking scheme for
**	    maxlocks/timeout (which was wrong to begin with, since the session
**	    values were not being taken into account), obtain these values
**	    from the rcb. The values in the rcb were arrived upon by a
**	    "formula" in dmt_open(). Also added lock_level and iso_level.
**	    (Bug 89922)
**	24-mar-98 (thaju02)
**	    bug #86326 - RAAT API, using set NOLOGGING, when escalating table
**	    lock, neglected to avoid logging if NOLOGGING specified.
**      28-may-98 (stial01)
**          dmr_alter() moved code to dm2t_row_lock()
**      15-Jul-98 (wanya01)
**          Bug #68560 - E_US125D Ambiguous Replace message is generated
**          if there are more than 65536 statements in a single transaction
**          Problem is due to page_seq_number is defined as u_i2 for 2k page
**          page_seq_number has been changed to u_i4 for page_size greater 
**          than 2k.  Add if condition to utilize the new page format.
**      12-Apr-1999 (stial01)
**          Distinguish leaf/index info for BTREE/RTREE indexes
**      17-may-99 (stial01)
**          dmr_alter() Added est_pages,total_pages parameters to
**          dmt_set_lock_values.
**      04-May-99 (shust01)
**	    MK user getting 197650 (Ingres error 196650, Corresponds to
**	    E_DM002A) when doing a position, get, get prev on unique btree 
**	    secondary index.  Caused by check in DMR_SET_LOWKEY.  Was checking 
**	    that the size of the key value passed in was equal to tcb_klen 
**	    (which might also include the tidp length). Change to make sure
**	    that the key size does not exceed tcb_klen. bug #96814.
**      25-May-00 (shust01)
**	    When using MK, it is possible to not receive a row level lock,
**	    when one was expected. bug #101373.
**	10-July-2000 (thaju02)
**	    IIraat_record_delete() fails with E_DM9C1D. Reset rcb_s_key_ptr.
**	    (B102074)
**	29-nov-2000 (shust01)
**	    Using RAAT, you can get serializable (SIX) page locks when running,
**	    instead of IX locks.  This causes other users to potentially get
**	    lock timeouts when trying to access the same page, when using
**	    row-level locking. (bug 103127, INGSRV 1306).
**	07-Jan-2002 (thaju02)
**	    IIraat_record_delete() preceded with IIraat_record_get() fails
**	    with E_DM9C1D. Reset rcb_fet_key_ptr. (B106757)
**      08-jul-2002 (shust01)
**          error E_DM93F5_ROW_LOCK_PROTOCOL error received when a lock
**          escalation precedes a delete, using the RAAT interface (b108202).
**	01-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameter to dm2t_lock_table avoid random
**	    implicit lock conversions.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Supply dmt_db_id (DML_ODCB*) to dmt_set_lock_values().
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Set rcb_dmr_opcode here; dmpe bypasses dmf_call,
**	    which used to set it.
**	    Add proposed lock level to dm2t_row_lock() prototype, deleted
**	    DMZ_TBL_MACRO(16) which once forced row-locking.
*/
DB_STATUS
dmr_alter(
DMR_CB  *dmr_cb)
{
    DMR_CB		*dmr = dmr_cb;
    DMP_RCB		*rcb;
    DMP_TCB             *tcb;
    DMR_CHAR_ENTRY	*array;
    DMR_CHAR_ENTRY	*entry;
    i4		array_size;
    i4		error;
    DB_STATUS		status = E_DB_OK;
    i4			entry_number = 0;
    i4                  seqnum;

    CLRDBERR(&dmr->error);

    for (;;)
    {
	rcb = (DMP_RCB *)dmr->dmr_access_id;
	if (dm0m_check((DM_OBJECT *)rcb, (i4)RCB_CB) != E_DB_OK)
	{
	    SETDBERR(&dmr->error, 0, E_DM002B_BAD_RECORD_ID);
	    status = E_DB_ERROR;
	    break;
	}

	rcb->rcb_dmr_opcode = DMR_ALTER;

	tcb = rcb->rcb_tcb_ptr;
	array = (DMR_CHAR_ENTRY *)dmr->dmr_char_array.data_address;
	array_size = dmr->dmr_char_array.data_in_size / sizeof(DMR_CHAR_ENTRY);
	for (entry_number = 0; 
	     array && (status == E_DB_OK) && (entry_number < array_size);
	     entry_number++)
	{
	    entry = &array[entry_number];
	    switch (entry->char_id)
	    {
	      case DMR_SEQUENCE_NUMBER:
		/*
		** Set new sequence number for record access control block.
		*/
                if (rcb->rcb_tcb_ptr->tcb_rel.relpgtype == TCB_PG_V1)
		  seqnum = (entry->char_value & 0x0ffff);
                else
                  seqnum = entry->char_value;
		rcb->rcb_seq_number = seqnum;

		/* Reset deferred update tid list */
		rcb->rcb_new_cnt = 0;

		break;

	      case DMR_LOCK_MODE:
	      {
		/*
		** Alter lock strategy of open table (RAAT only)
		*/
		DMT_CB		dmt_cb;
		DML_XCB		*xcb = rcb->rcb_xcb_ptr;
		DML_SCB		*scb = xcb->xcb_scb_ptr;
                DMP_TCB         *tcb = rcb->rcb_tcb_ptr;
                DMP_DCB         *dcb = tcb->tcb_dcb_ptr;
                DB_TAB_ID       *table_id = &tcb->tcb_rel.reltid;
                i4         lock_id = xcb->xcb_lk_id;
                i4         lock_mode = entry->char_value;
                i4         timeout = rcb->rcb_timeout;
		i4		max_locks = rcb->rcb_lk_limit;
                i4         lock_level = rcb->rcb_lk_type;
		i4		readlock = DMC_C_SYSTEM;
		i4         iso_level = DMC_C_SYSTEM;
                i4         row_lock = FALSE;
                i4         grant_mode;
		LK_LOCK_KEY	lock_key;
		CL_ERR_DESC	sys_err;
                LK_LKID         lockid;

		MEfill(sizeof(LK_LKID), 0, &lockid);
		if (entry->char_value == DMT_N)
		    readlock = DMC_C_READ_NOLOCK;

		dmt_cb.dmt_flags_mask = 0;
		STRUCT_ASSIGN_MACRO(*table_id, dmt_cb.dmt_id);
		if (lock_mode == DMT_X || lock_mode == DMT_IX || 
                    lock_mode == DMT_RIX)
		{
		    dmt_cb.dmt_access_mode = DMT_A_WRITE;
		    if ( !(scb->scb_sess_mask & SCB_NOLOGGING) )
		    	rcb->rcb_logging = RCB_LOGGING;
		}
		else
		    dmt_cb.dmt_access_mode = DMT_A_READ;

                dmt_cb.dmt_lock_mode = lock_mode;
		dmt_cb.dmt_mustlock = FALSE;

		/*
		** Check for user supplied table level locking values.
		*/
		if (scb->scb_kq_next != (DML_SLCB *)&scb->scb_kq_next)
		{
		    dmt_check_table_lock(scb, &dmt_cb, &timeout, &max_locks, 
			    &readlock, &lock_level);
		}

		lock_level = DMC_C_SYSTEM;

		iso_level = scb->scb_sess_iso;
		if (xcb->xcb_flags & XCB_USER_TRAN &&
			scb->scb_tran_iso != DMC_C_SYSTEM)
		    iso_level = scb->scb_tran_iso;

		/* dmt_set_lock_values needs this */
		dmt_cb.dmt_db_id = (PTR)xcb->xcb_odcb_ptr;

                /*
                ** Set final locking characteristics.
                */
		status = dmt_set_lock_values(&dmt_cb, &timeout, &max_locks,
		    &readlock, &lock_level, &iso_level, &lock_mode,
		    DMC_C_SYSTEM, DMC_C_SYSTEM);
                if (status != OK)
                    break;
                
                if (lock_mode == DM2T_RIS)
                {
                    row_lock = TRUE;
                    lock_mode = DM2T_IS;
                }
                else if (lock_mode == DM2T_RIX)
                {
                    row_lock = TRUE;
                    lock_mode = DM2T_IX;
                }

                grant_mode = lock_mode;

                if (lock_mode == DM2T_N)
                {
                    if (!(dcb->dcb_status & DCB_S_PRODUCTION))
                    {
                        status = dm2t_control(dcb, table_id->db_tab_base, 
                            lock_id, LK_S, (i4)0, timeout, &dmr->error);
                        if (status != E_DB_OK)
                            break;
                    }
                }
                else
                {
                    status = dm2t_lock_table(dcb, table_id, lock_mode, lock_id,
                        timeout, &grant_mode, &lockid, &dmr->error);
		    if (status != E_DB_OK)
		        break;
		}

		status = dm2r_unfix_pages(rcb, &dmr->error);
		if (status != OK)
                    break;

                rcb->rcb_k_mode = lock_mode;
 
                if (lock_mode >= DM2T_S)
                {
                    rcb->rcb_k_type = RCB_K_TABLE;
                }
		else if ( row_lock && dm2t_row_lock(tcb, RCB_K_ROW) )
                {
                    rcb->rcb_k_type = RCB_K_ROW;
                }
                else
                {
                    rcb->rcb_k_type = RCB_K_PAGE;
                }
 
                rcb->rcb_lk_mode = grant_mode;
                rcb->rcb_lk_type = rcb->rcb_k_type;
                if ((grant_mode == DM2T_X) ||
                    ((grant_mode >= DM2T_S) && (lock_mode == DM2T_IS)))
                {
                    rcb->rcb_lk_type = RCB_K_TABLE;
                }

                if (lock_mode == DM2T_N)
		{
		    i4	rnl_bufs;
			
		    if (tcb->tcb_seg_rows)
			rnl_bufs = 3;
		    else
			rnl_bufs = 2;

		    rcb->rcb_k_duration = RCB_K_READ;
		    rcb->rcb_lk_type = RCB_K_TABLE;
		    if (tcb->tcb_rel.relspec == TCB_BTREE)
			rcb->rcb_lk_type = RCB_K_PAGE;
 
		    /*
		    **  Allocate a MISC_CB to hold the two pages needed
		    ** to support readlock=nolock.
		    */
		    if (rcb->rcb_rnl_cb == NULL)
		    {
			status = dm0m_allocate(sizeof(DMP_MISC) +
			    tcb->tcb_rel.relpgsize * rnl_bufs, (i4)0,
			    (i4)MISC_CB, (i4)MISC_ASCII_ID,
			    (char *)rcb, (DM_OBJECT **)&rcb->rcb_rnl_cb,
			    &dmr->error);
			if (status != E_DB_OK)
			    break;
			rcb->rcb_rnl_cb->misc_data = 
				(char*)rcb->rcb_rnl_cb + sizeof(DMP_MISC);
		    }
		}
		else
		{
                    rcb->rcb_k_duration = RCB_K_TRAN;
		}

                if (iso_level == DMC_C_READ_UNCOMMITTED)
                    rcb->rcb_iso_level = RCB_READ_UNCOMMITTED;
                else if  (iso_level == DMC_C_READ_COMMITTED)
                    rcb->rcb_iso_level = RCB_CURSOR_STABILITY;
                else if (iso_level == DMC_C_REPEATABLE_READ)
                    rcb->rcb_iso_level = RCB_REPEATABLE_READ;
                else
                    rcb->rcb_iso_level = RCB_SERIALIZABLE;

		rcb->rcb_access_mode = (dmt_cb.dmt_access_mode == DMT_A_WRITE)
		    ?RCB_A_WRITE:RCB_A_READ;

		break;
	      }

	      case DMR_GET_RCB_LOWTID:
	      {
		/* get rcb_lowtid */
                entry->char_value = rcb->rcb_lowtid.tid_i4;
		break;
              }
	   
	      case DMR_SET_RCB_LOWTID:
	      {
		/* set rcb_lowtid with value passed in  */
                rcb->rcb_lowtid.tid_i4 = entry->char_value;
		/* just in case we previously hit end of table, turn flag
		   back on */
		rcb->rcb_state |= RCB_POSITIONED;
		break;
              }
	   
	      case DMR_SET_CURRENTTID:
	      {
		/* set currenttid with value passed in  */
		rcb->rcb_currenttid.tid_i4 = entry->char_value;
		break;
              }

	      case DMR_SET_LOWKEY:
	      {
		/*
		** Reset rcb_repos_* to make sure we reposition using key.
		** NOTE: We expect here that the DMR_ALTER call passed:
		** DMR_SET_RCB_LOWTID, DMR_SET_CURRENTTID, DMR_SET_LOWKEY
		** IN THAT ORDER!!!!!!!
		** That is, rcb_lowtid and rcb_currenttid have already been set
		*/
		if (dmr_cb->dmr_data.data_in_size > tcb->tcb_ixklen)
		{
		    SETDBERR(&dmr->error, entry_number, E_DM002A_BAD_PARAMETER);
		    status = E_DB_ERROR;
		    break;
		}

		/*
		** Set rcb_repos_clean_count = DM1B_CC_REPOSITION
		** to force a reposition to the key,bid,tid we put
		** into rcb_repos*
		*/
		rcb->rcb_pos_info[RCB_P_GET].clean_count = DM1B_CC_REPOSITION;
		rcb->rcb_pos_info[RCB_P_GET].bid.tid_i4 = rcb->rcb_lowtid.tid_i4;

		/*
		** Key provided is index key...
		** We need to build rcb_repos_key_ptr, which is LEAF key
		** See front/embed/raat/raatrget.c send_bkey()
		*/
		dm1b_build_key(tcb->tcb_keys,
			tcb->tcb_key_atts, dmr_cb->dmr_data.data_address,
			tcb->tcb_leafkeys, rcb->rcb_repos_key_ptr);

		dm1b_build_key(tcb->tcb_keys,
			tcb->tcb_key_atts, dmr_cb->dmr_data.data_address,
			tcb->tcb_leafkeys, rcb->rcb_s_key_ptr);

		dm1b_build_key(tcb->tcb_keys,
			tcb->tcb_key_atts, dmr_cb->dmr_data.data_address,
			tcb->tcb_leafkeys, rcb->rcb_fet_key_ptr);

		rcb->rcb_pos_info[RCB_P_FETCH].clean_count = DM1B_CC_REPOSITION;

		/*
		** Non-unique primary btree may need tidp to reposition
		** Move rcb_currenttid into end of leaf entry
		*/
		if ((tcb->tcb_rel.relstat & TCB_INDEX) == 0)
		    rcb->rcb_pos_info[RCB_P_GET].tidp.tid_i4 = rcb->rcb_currenttid.tid_i4;

		break;
	      }

	      case DMR_GET_DIRTYREAD:
	      {
		/* 
		** pass back whether this table is using dirty reads 
		** (readlock=nolock or isolation-level=READ_UNCOMMITTED)
		*/
		entry->char_value = (rcb->rcb_k_duration == RCB_K_READ) ?
		    DMR_C_ON : DMR_C_OFF;
		break;
	      }		

	      default:
		SETDBERR(&dmr->error, entry_number, E_DM000D_BAD_CHAR_ID);
		status = E_DB_ERROR;
		break;
	    }
	}

	if (status != E_DB_OK)
	    break;

	return (status);
    }

    if (dmr->error.err_code > E_DM_INTERNAL)
    {
	uleFormat( &dmr->error, 0, NULL, ULE_LOG , NULL, 
	    (char * )NULL, 0L, (i4 *)NULL, &error, 0);
	SETDBERR(&dmr->error, 0, E_DM0139_ERROR_ALTERING_RCB);
    }

    return (status);
}
