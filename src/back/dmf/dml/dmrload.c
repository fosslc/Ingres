/*
**Copyright (c) 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <pc.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <sr.h>
#include    <adf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmxcb.h>
#include    <dmtcb.h>
#include    <dmucb.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dm2r.h>
#include    <dm2t.h>
#include    <dm2f.h>
#include    <dm1b.h>
#include    <dmse.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dmxe.h>
#include    <dm0m.h>
#include    <dmpecpn.h>

/**
** Name: DMRLOAD.C - Implements the DMF load record operation.
**
** Description:
**      This file contains the functions necessary to implement the load
**	table operation.
**
**      This file defines:
**
**      dmr_load - Load a record into a table.
**
** History:
**	15-sep-1986 (jennifer)
**	    Created for Jupiter.
**	12-mar-87 (rogerk)
**	    Added user-table load capability.
**	    Added mulit-row interface.
**      22_apr_87 (annie)
**          Added read/nolock support.
**	30-sep-1988 (rogerk)
**	    Added capability to delay writing Begin Transaction record until
**	    first write operation.  If DELAYBT flag is set in the XCB, then
**	    call dmxe to write the BT record.
**	6-June-1989 (ac)
**	    Added 2PC support.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.  Don't allow LOAD operation
**	    on Gateway tables.
**	28-sep-1989 (mikem)
**	    Disallow bulk loading of tables with system maintained logical keys.
**	    Previous to this bulk loading of these tables would just copy in
**	    the old values staight into the "system-maintained" fields, making
**	    it probable that the keys would no-longer be unique.
**	17-jan-1990 (rogerk)
**	    Add support for not writing savepoint records in readonly xacts.
**      1-nov-1990 (bryanp)
**          Add support for not logging loads to temporary tables (b34226).
**      11-Oct-1991 (rmuth) Merge65  17-oct-90 (jennifer)
**          Allow bulk loading of tables with system maintained  logical keys.
**          DM2R.c generates new keys for these fields.
**	11-Oct-1991 (rmuth) Merge65  16-may-1990 (jennifer)
**	    Add support for table empty option.
**	5-may-1992 (bryanp)
**	    Conditionalize BT logging on rcb_logging for temporary tables.
**	7-jul-1992 (bryanp)
**	    Prototyping DMF.
**	8-jul-1992 (walt)
**	    Prototyping DMF.
**	16-feb-1993 (rogerk)
**	    Changed table-in-use check to look at tcb_open_count, not the
**	    tcb_ref_count.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system type definitions.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Delete unused variable 'ctrl_lk_id'.
**	26-July-1993 (rmuth)
**	    The COPY command now takes the following with clauses,
**	    (FILLFACTOR, LEAFFILL, NONLEAFFILL, ALLOCATION, EXTEND,
**	     MINPAGES and MAXPAGES). These are passed in via the
**	    dmr_char_array, add code to process this structure. Also
**	    added the followinh header file, sr.h, adf.h, dmucb.h,
**	    dm2t.h, dm1b.h, dmse.h, dm2umct.h, dm2umxcb.h, dm2u.h
**	27-jul-1993 (rmuth)
**	    Fix compiler warnings show up on VMS.
**	18-oct-1993 (rmuth)
**	    CL prototype, add <dm2f.h>.
**	18-oct-1993 (rogerk)
**	    Changed code to check for empty table state even for heap
**	    tables.  The dm2r_load routine will now sometimes allocate
**	    a new file for the load and then swap files at the end for
**	    heap tables as well as structured tables.  This can be done
**	    only if there are no existing rows.
**	31-jan-1994 (bryanp) B58488
**	    Check status in start_load after exiting the do-while loop.
**	06-mar-1996 (stial01 for bryanp)
**	    Don't allocate tuple buffers on the stack.
**	13-sep-1996 (canor01)
**	    Use the tuple buffer that is part of the rcb.
**	3-jun-97 (nanpr01)
**	    Fix bug 78352 : When transactions are rolled back, temporary
**	    tables that were updated does not get dropped as documented.
**	7-nov-1998 (nanpr01)
**	    When there is norow, it is pointless to call start_load and
**	    end_load when there is norow.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Nov-2000 (jenjo02)
**	    Deleted put_temporary_in_xcb_tq() function. This 
**	    functionality is now handled when RCB is closed.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      29-nov-2005 (horda03) Bug 48659/INGSRV 2716
**          A TX can now be started with a non-journaled BT record. This
**          leaves the XCB_DELAYBT flag set but sets the XCB_NONJNL_BT
**          flag. Need to write a journaled BT record if the update
**          will be journaled.
**	24-Feb-2008 (kschendel) SIR 122739
**	    Minor changes for new rowaccessor scheme.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2r_? functions converted to DB_ERROR *
*/

/*
** Forward declarations of static functions:
*/
static STATUS start_load(
    DMR_CB	*dmr_cb,
    DMP_RCB	*rcb);


/*{
** Name:  dmr_load - Load a record.
**
**  INTERNAL DMF call format:      status = dmr_load(&dmr_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMR_LOAD, &dmr_cb);
**
**
**	DMR_LOAD is used as a fast way to add records to an empty table.
**	The table may be a temporary HEAP or a user table of any storage
**	structure.  User tables cannot be loaded with DMR_LOAD unless the
**	following conditions are met:
**
**		1) The table is not journaled.
**		2) The table has no secondary index.
**		3) The table is empty or is a heap.
**		4) The caller has an exclusive lock on the table.
**
**	While the table is being loaded, no other record level calls may
**	be performed it.  When all rows have been added, DMR_LOAD should
**	be called with the DMR_ENDLOAD flag.  This will make the table
**	available for normal record access.
**
**	The caller should not issue a SAVEPOINT command while in the process of
**	loading a table.
**
**	The table to be loaded can be a user table (real or GTT), or a
**	work temporary (eg qef sort temp).  User tables are indicated by
**	setting DMR_TABLE_EXISTS.  This flag is somewhat vestigial now,
**	since the physical layer doesn't really pay attention to it any
**	more;  but it does control whether or not we pay attention to the
**	DMR_EMPTY flag when starting the load.  (See below.)
**
**	TUPLES ADDED WITH DMR_LOAD ARE NOT LOGGED individually.  If the
**	DMR_TABLE_EXISTS flag is specified, then backout of the load will
**	consist of deleting all rows in the table.  If DMR_TABLE_EXISTS
**	flag is not specified then the the table must be a temporary.
**      If the table is a temporary, then no recovery is performed. No logging
**      is done, and no backout is possible. If the query aborts, the contents
**      of the table are unpredictable (since it is a temporary table, it is
**      assumed that it will be thrown away). We enforce this restriction in
**      the 'start_load()' subroutine by ensuring that if DMR_TABLE_EXISTS is
**      off, tcb_temporary must equal TCB_TEMPORARY.
**
**      If the DMR_NOSORT flag is specified for a temporary table, then the
**      records are put directly into the table on each call to DMR_LOAD.
**
**      For structured user tables, the first call to DMR_LOAD caused the
**	sorter to be initialized.  On subsequent calls to DMR_LOAD, records
**	are handed to the sorter.  When the DMR_ENLOAD flag is given, all
**	records are transferred from the sorter to the table being loaded.
**
**	Heap tables are loaded NOSORT by default.  Only if the DMR_HEAPSORT
**	flag is specified will the rows be sorted.
**
**	For non-heaps:
**	DMR_LOAD will make sure that a table is empty before attempting to
**	load it.  If it thinks that the table is not empty, or thinks it
**	is too costly to scan the table for tuples, DMR_LOAD will return
**	E_DM0050_TABLE_NOT_LOADABLE.  DMR_LOAD will not scan a table that
**	has more than 20 pages.
**
**	If the caller is sure that a table is empty, it can specify the
**	DMR_EMPTY flag.  This will cause DMR_LOAD to bypass the scanning of
**	the table to check for any tuples.  If this flag is specified to
**	load a non-empty table, all previous rows in the table will be lost.
**     
**	This routine sflag to remove duplicates if user table 
**      and doesn't allow dups, checks for NOSORT flag to avoid sort,  
**      sets nosort flag if user heap table.  It also 
**	returns E_DM0050_TABLE_NOT_LOADABLE if table is not empty, has
**	secondary indexes or is journaled.
**      
**	The load routine can be called with 0 rows so the caller 
**      can initialize the loader
**	before it is ready to start sending rows.
**	If the dmr_s_estimated_records value is in the dmr_cb.  This value
**	is passed on dm2r_load for the estimated number of rows to be added.
**
** Inputs:
**      dmr_cb
**          .type                       Must be set to DMR_RECORD_CB.
**          .length                     Must be at least
**                                      sizeof(DMR_RECORD_CB) bytes.
**          .dmr_flags_mask             Zero or an or'd combination of:
**					  DMR_ENDLOAD - end of load.
**					  DMR_NOSORT - tuples are already in
**						sorted order.
**					  DMR_HEAPSORT - sort heap tuples.
**					  DMR_NODUPLICATES - remove
**						duplicate rows while sorting.
**						If table exists and does not
**						allow duplicates, dups will be
**						removed regardless of flag.
**					  DMR_TABLE_EXISTS - table already
**						exists and is formatted as
**						an Ingres Table.
**					  DMR_EMPTY - don't check to see if
**						table is empty, we know that
**						it is.
**					All flags except DMR_ENDLOAD are checked
**					only on the first call to dmr_load.
**
**          .dmr_access_id              Record access identifer returned from
**                                      DMT_OPEN that identifies table to load.
**	    .dmr_count			Number of rows in dmr_mdata list below.
**          .dmr_mdata			Pointer to list of DM_MDATA structs
**					holding records to put into table.
**	    .dmr_s_estimated_records	Estimate of number of rows to be loaded.
**					Can be zero.
**
** Outputs:
**      dmr_cb
**	    .dmr_count			On DMR_ENDLOAD call, this is set to
**					the number of rows actually loaded.
**          .error.err_data                 If error, set to show on which
**					    record in the dmr_mdata list the
**					    error occured.
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM001A_BAD_FLAG
**                                          E_DM002B_BAD_RECORD_ID
**                                          E_DM0045_DUPLICATE_KEY
**                                          E_DM0046_DUPLICATE_RECORD
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**					    E_DM004D_LOCK_TIMER_EXPIRED
**					    E_DM0050_TABLE_NOT_LOADABLE
**                                          E_DM0065_USER_INTR
**                                          E_DM0064_USER_ABORT
**                                          E_DM009F_ILLEGAL_OPERATION
**                                          E_DM0100_DB_INCONSISTENT
**                                          E_DM010C_TRAN_ABORTED
**                                          E_DM0112_RESOURCE_QUOTA_EXCEEDED
**					    E_DM0109_ERROR_LOADING_DATA
**      Returns:
**         E_DB_OK                          Function completed normally.
**         E_DB_WARN                        Function completed normally with a 
**                                          termination status which is in
**                                          dmr_cb.err_code.
**         E_DB_ERROR                       Function completed abnormally 
**                                          with a 
**                                          termination status which is in
**                                          dmr_cb.err_code.
**         E_DB_FATAL			    Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmr_cb.err_code.
** History:
**	15-sep-1986 (jennifer)
**	    Created.
**	12-mar-87 (rogerk)
**	    Added support for user tables.
**	    Added multi-row interface.
**	01-Apr-87 (ac)
**          Added read/nolock support.
**	30-sep-1988 (rogerk)
**	    Added capability to delay writing Begin Transaction record until
**	    first write operation.  If DELAYBT flag is set in the XCB, then
**	    call dmxe to write the BT record.
**	17-mar-1989 (EdHsu)
**	    Added cluster online backup support.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.  Don't allow LOAD operation
**	    on Gateway tables.
**	28-sep-1989 (mikem)
**	    Disallow bulk loading of tables with system maintained logical keys.
**	    In the case of copies into an empty relation, bulk loading would
**	    load the old logical key into the system maintained field, rather
**	    than produce a new one.  This fix will disallow these "BLAST"
**	    copies.  In the next major release of the system we will allow
**	    these "BLAST" copies, and do the right thing with system maintained
**	    logical keys.
**	17-jan-1990 (rogerk)
**	    Add support for not writing savepoint records in readonly xacts.
**	    Pass xcb argument to dmxe_writebt routine.
**      17-oct-90 (jennifer)
**          Allow bulk loading of tables with system maintained  logical keys.
**          DM2R.c generates new keys for these fields.
**      1-nov-1990 (bryanp)
**          Add support for not logging loads to temporary tables (b34226).
**	5-may-1992 (bryanp)
**	    Conditionalize BT logging on rcb_logging for temporary tables.
**	16-feb-1993 (rogerk)
**	    Changed table-in-use check to look at tcb_open_count, not the
**	    tcb_ref_count.  The tcb may legally be referenced by DBMS
**	    background threads.
**	3-jun-97 (nanpr01)
**	    Fix bug 78352 : When transactions are rolled back, temporary
**	    tables that were updated does not get dropped as documented.
**	19-dec-97 (inkdo01)
**	    Changes to support sorts whose results are NOT copied to a temp
**	    table.
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
**      29-nov-2005 (horda03) Bug 48659/INGSRV 2716
**          A TX can now be started with a non-journaled BT record. This
**          leaves the XCB_DELAYBT flag set but sets the XCB_NONJNL_BT
**          flag. Need to write a journaled BT record if the update
**          will be journaled.
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Set rcb_dmr_opcode here; dmpe bypasses dmf_call,
**	    which used to set it.
*/
DB_STATUS
dmr_load(
DMR_CB  *dmr_cb)
{
    DMR_CB		*dmr = dmr_cb;
    DMP_RCB		*rcb;
    DMP_TCB		*tcb;
    DML_XCB		*xcb;
    DM_MDATA		*mdata;
    DB_STATUS		status;
    i4		flag;
    i4		i;
    i4		error;
    i4		row_count = 0;

    CLRDBERR(&dmr->error);

    for (status = E_DB_ERROR;;)
    {
	rcb = (DMP_RCB *)dmr->dmr_access_id;
	if (dm0m_check((DM_OBJECT *)rcb, (i4)RCB_CB) == E_DB_OK)
	{
	    rcb->rcb_dmr_opcode = DMR_LOAD;

	    flag = 0;
	    if (dmr->dmr_flags_mask)
	    {
		if (dmr->dmr_flags_mask & 
                    ~(DMR_ENDLOAD | DMR_NOSORT | DMR_NOPARALLEL |
		    DMR_NODUPLICATES | DMR_TABLE_EXISTS | DMR_SORT_NOCOPY |
                    DMR_HEAPSORT | DMR_EMPTY | DMR_CHAR_ENTRIES))
		{
		    SETDBERR(&dmr->error, 0, E_DM001A_BAD_FLAG);
		    break;
		}
	    }
	    
	    xcb = rcb->rcb_xcb_ptr;
	    if ( dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) )
	    {
		uleFormat(NULL, E_DM00E0_BAD_CB_PTR, (CL_ERR_DESC *)NULL, ULE_LOG, 
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    sizeof("transaction")-1, "transaction");
		SETDBERR(&dmr->error, 0, E_DM002A_BAD_PARAMETER);
		break;
	    }
	    if ( dm0m_check((DM_OBJECT *)xcb->xcb_scb_ptr, (i4)SCB_CB) )
	    {
		uleFormat(NULL, E_DM00E0_BAD_CB_PTR, (CL_ERR_DESC *)NULL, ULE_LOG, 
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    sizeof("session")-1, "session");
		SETDBERR(&dmr->error, 0, E_DM002A_BAD_PARAMETER);
		break;
	    }

	    /* Check for external interrupts */
	    if ( xcb->xcb_scb_ptr->scb_ui_state )
		dmxCheckForInterrupt(xcb, &error);

	    /*
	    ** If this is the first write operation for this transaction,
	    ** then we need to write the begin transaction record.  However,
	    ** if the table being loaded is a temporary table, then no recovery
	    ** is neded so we can skip writing of these log records. (b34226)
	    */

	    if ( xcb->xcb_state == 0 &&
	        (xcb->xcb_flags & XCB_DELAYBT) &&
		(rcb->rcb_logging != 0) &&
		(dmr->dmr_flags_mask & DMR_TABLE_EXISTS))
	    {
                i4 journal_bt = is_journal(rcb);

		status = dmxe_writebt(xcb, journal_bt, &dmr->error);
		if (status != E_DB_OK)
		{
		    xcb->xcb_state |= XCB_TRANABORT;
		    break;
		}
	    }

	    if (xcb->xcb_state == 0)
	    {
		/*
		** If it is not open or it is ended, can't do this operation.
		** User tables (indicated by DMR_TABLE_EXISTS) 
                ** can be loaded twice
		** in one transaction.
		*/

		tcb = rcb->rcb_tcb_ptr;
		if ((rcb->rcb_state & RCB_OPEN) == 0 ||
		    ((rcb->rcb_state & RCB_LEND) &&
			(dmr->dmr_flags_mask & DMR_TABLE_EXISTS) == 0))
		{
		    SETDBERR(&dmr->error, 0, E_DM009F_ILLEGAL_OPERATION);
		    break;
		}

		if ((rcb->rcb_state & RCB_LSTART) == 0)
		{
		    if (dmr->dmr_flags_mask & DMR_ENDLOAD)
		    {
			rcb->rcb_state |= RCB_LEND;
			status = E_DB_OK;
			break;
		    }
		    else
		    {
		        status = start_load(dmr, rcb);
		        if (status != E_DB_OK)
			    break;
		    }
		}
		if (dmr->dmr_flags_mask & DMR_ENDLOAD) 
		{
		    /*
		    ** End the load sequence.  Extract the rows 
                    ** from the sorter and
		    ** build the table with them.
		    */
		    if ((rcb->rcb_state & RCB_LSTART) == 0)
		    {
			status = E_DB_ERROR;
			SETDBERR(&dmr->error, 0, E_DM009F_ILLEGAL_OPERATION);
			break;
		    }

		    status = dm2r_load(rcb, tcb, DM2R_L_END, (i4)0,
			&row_count, (DM_MDATA *)0, (i4)0, 
			(DM2R_BUILD_TBL_INFO *) 0, &dmr->error);
		    if (status == E_DB_OK)
		    {
			dmr->dmr_count = row_count;
			return (status);
		    }
		}
		else if (dmr->dmr_count > 0)
		{
		    /* This is a normal request, put record to sorter. */

		    row_count = dmr->dmr_count;
		    mdata = dmr->dmr_mdata;
		    for (i = 0; i < row_count; i++)
		    {
			if (mdata == 0 || mdata->data_address == 0 ||
			    mdata->data_size < tcb->tcb_rel.relwid)
			{
			    break;
			}
			mdata = mdata->data_next;
		    }
		    if (i < row_count)
		    {
			row_count = i + 1;
			SETDBERR(&dmr->error, 0, E_DM002A_BAD_PARAMETER);
			status = E_DB_ERROR;
			break;
		    }
		    status = dm2r_load(rcb, tcb, DM2R_L_ADD, (i4)0,
			&row_count, dmr->dmr_mdata, (i4)0,
			(DM2R_BUILD_TBL_INFO *) 0, &dmr->error);
		    if (status == E_DB_OK)
		    {
			dmr->dmr_count = row_count;
			return (status);
		    }
		}
	    }
	    else
	    {
		if (xcb->xcb_state & XCB_USER_INTR)
		    SETDBERR(&dmr->error, 0, E_DM0065_USER_INTR);
		else if (xcb->xcb_state & XCB_FORCE_ABORT)
		    SETDBERR(&dmr->error, 0, E_DM010C_TRAN_ABORTED);
		else if (xcb->xcb_state & XCB_ABORT)
		    SETDBERR(&dmr->error, 0, E_DM0064_USER_ABORT);
		else if (xcb->xcb_state & XCB_WILLING_COMMIT)
		    SETDBERR(&dmr->error, 0, E_DM0132_ILLEGAL_STMT);
		status = E_DB_ERROR;
	    }
	}
	else
	    SETDBERR(&dmr->error, 0, E_DM002B_BAD_RECORD_ID);
	break;
    }

    if (dmr->error.err_code == E_DM004B_LOCK_QUOTA_EXCEEDED ||
	dmr->error.err_code == E_DM0112_RESOURCE_QUOTA_EXCEED)
    {
	rcb->rcb_xcb_ptr->xcb_state |= XCB_STMTABORT;
    }
    else if (dmr->error.err_code == E_DM0042_DEADLOCK ||
	dmr->error.err_code == E_DM004A_INTERNAL_ERROR ||
	dmr->error.err_code == E_DM0100_DB_INCONSISTENT)
    {
	rcb->rcb_xcb_ptr->xcb_state |= XCB_TRANABORT;
    }
    else if (dmr->error.err_code == E_DM010C_TRAN_ABORTED)
    {
	rcb->rcb_xcb_ptr->xcb_state |= XCB_FORCE_ABORT;
    }
    else if (dmr->error.err_code == E_DM0065_USER_INTR)
    {
	rcb->rcb_xcb_ptr->xcb_state |= XCB_USER_INTR;
	rcb->rcb_state &= ~RCB_POSITIONED;
	*(rcb->rcb_uiptr) &= ~SCB_USER_INTR;
    }
    else if (dmr->error.err_code > E_DM_INTERNAL)
    {
	uleFormat(&dmr->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char * )NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(&dmr->error, 0, E_DM008B_ERROR_PUTTING_RECORD);
    }

    dmr->error.err_data = row_count;
    return (status);
}

/*
** History:
**	26-jul-1993 (bryanp)
**	    Delete unused variable 'ctrl_lk_id'.
**	27-jul-1993 (rmuth)
**	    Fix compiler warnings shown up on VMS.
**	18-oct-1993 (rogerk)
**	    Changed code to check for empty table state even for heap
**	    tables.  The dm2r_load routine will now sometimes allocate
**	    a new file for the load and then swap files at the end for
**	    heap tables as well as structured tables.  This can be done
**	    only if there are no existing rows.
**	31-jan-1994 (bryanp) B58488
**	    Check status in start_load after exiting the do-while loop.
**	    Initialize "status" so that the check works properly.
**	06-mar-1996 (stial01 for bryanp)
**	    Don't allocate tuple buffers on the stack.
**	13-sep-1996 (canor01)
**	    Use the tuple buffer that is part of the rcb.
**      10-mar-1997 (stial01)
**          Pass relwid to dm0m_tballoc()
**	3-jun-97 (nanpr01)
**	    Fix bug 78352 : When transactions are rolled back, temporary
**	    tables that were updated does not get dropped as documented.
**	11-aug-97 (nanpr01)
**	    Add t->tcb_comp_atts_count with relwid in dm0m_tballoc call.
**      16-oct-98 (stial01)
**          start_load() For temporary HEAP tables, check for sort or nosort 
**	8-nov-99 (hayke02)
**	    Previous change did not remove tcb_open_count > 1 test. This
**	    change fixes bug 39343, 93437 and 99360.
**	17-jan-2002 (hayke02)
**	    Update the [min,max]pages values (relmin, relmax) for hash tables
**	    for when non-default values are used in the copy. This will lead
**	    to iirelation being updated in update_load_tab(), and then
**	    subsequent unloaddb/copydb runs will retain these [min,max]pages
**	    values in the copy statement. This change fixes bug 106799.
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
**	23-Mar-2010 (kschendel) SIR 123448
**	    Define no-parallel-sort flag for load.
**	20-Apr-2010 (kschendel) SIR 123485
**	    A table bulk-load is definitely a multi-row operation in the
**	    LOB sense, reflect it in the BQCB if there is one.
*/
static STATUS
start_load(
DMR_CB		*dmr_cb,
DMP_RCB		*rcb)
{
    DMR_CB		 *dmr = dmr_cb;
    DMP_TCB		 *tcb = rcb->rcb_tcb_ptr;
    i4		 flag = 0;
    DM_TID		 tid;
    DB_STATUS		 status = E_DB_OK;
    i4		 row_count;
    i4		 row_estimate;
    char		 *rec_buf = NULL;
    DM2R_BUILD_TBL_INFO  build_tbl_info;
    i4		 chr_count, i;
    DMU_CHAR_ENTRY	 *chr;
    i4		 min_pages = 0, max_pages = 0;
    bool		 table_empty = TRUE;
    i4			 error;

    /* Get the tuple buffer from the rcb */
    if ( rcb->rcb_tupbuf == NULL )
	rcb->rcb_tupbuf = dm0m_tballoc( (tcb->tcb_rel.relwid +
					     tcb->tcb_data_rac.worstcase_expansion));
    if ((rec_buf = rcb->rcb_tupbuf) == 0)
    {
	SETDBERR(&dmr->error, 0, E_DM923D_DM0M_NOMORE);
	return (E_DB_ERROR);
    }

    /* Start the load process. */

    if (dmr->dmr_flags_mask & DMR_NODUPLICATES)
	flag |= DM2R_ELIMINATE_DUPS;
    if (dmr->dmr_flags_mask & DMR_NOSORT)
	flag |= DM2R_NOSORT;
    if (dmr->dmr_flags_mask & DMR_EMPTY)
	flag |= DM2R_EMPTY;
    if (dmr->dmr_flags_mask & DMR_SORT_NOCOPY)
	flag |= DM2R_SORTNC;
    if (dmr->dmr_flags_mask & DMR_NOPARALLEL)
	flag |= DM2R_NOPARALLEL;

    if (dmr->dmr_flags_mask & DMR_TABLE_EXISTS)
    {
	flag |= DM2R_TABLE_EXISTS;
	if ((tcb->tcb_rel.relspec != TCB_HEAP) &&
	    ((tcb->tcb_rel.relstat & TCB_UNIQUE) ||
	     (tcb->tcb_rel.relstat & TCB_DUPLICATES) == 0))
	{
	    flag |= DM2R_ELIMINATE_DUPS;
	}

	/* Default is to not sort HEAP tables */

	if ((tcb->tcb_rel.relspec == TCB_HEAP) &&
	    ((dmr->dmr_flags_mask & DMR_HEAPSORT) == 0))
	{
	    flag |= DM2R_NOSORT;
	}

	/*
	** Table must be opened exclusive, and cannot be journaled
	** or indexed in order to use load.
	** Gateway tables cannot be loaded.
	*/
	if ((rcb->rcb_lk_mode != RCB_K_X) ||
	    (tcb->tcb_rel.relstat & (TCB_IDXD | TCB_JOURNAL | TCB_GATEWAY)))
	{
	    SETDBERR(&dmr->error, 0, E_DM0050_TABLE_NOT_LOADABLE);
	    return (E_DB_ERROR);
	}

	/*
	** Make sure that bulk loading the table will not wipe out already
	** existing rows.  The table must either be empty or a heap table
	** (which can be bulk loaded without overwriting the existing pages
	** of the table).
	*/

	/*
	** If the table has > 20 pages then we make the assumption that the
	** table is not empty to avoid scanning lots of pages rather than
	** just going and doing a non-bulk copy.
	*/
	if ((dmr->dmr_flags_mask & DMR_EMPTY) == 0 &&
	    ((tcb->tcb_rel.reltups + tcb->tcb_tup_adds > 0) ||
		    (tcb->tcb_rel.relpages + tcb->tcb_page_adds > 20)))
	{
	    table_empty = FALSE;
	}

	/*
	** Position and attempt to fetch one row from the table to tell
	** if it is really empty or not.
	*/
	if (table_empty && ((dmr->dmr_flags_mask & DMR_EMPTY) == 0))
	{
	    status = dm2r_position(rcb, DM2R_ALL, (DM2R_KEY_DESC *) 0,
		0, &tid, &dmr->error);
	    if (status != E_DB_OK)
		return (E_DB_ERROR);

	    status = dm2r_get(rcb, &tid, DM2R_GETNEXT, rec_buf, &dmr->error);
	    if ((status != E_DB_OK) && (dmr->error.err_code != E_DM0055_NONEXT))
		return (E_DB_ERROR);

	    if (status == E_DB_OK)
		table_empty = FALSE;

	    status = E_DB_OK;
	}

	if ( ! table_empty && tcb->tcb_rel.relspec != TCB_HEAP)
	{
	    SETDBERR(&dmr->error, 0, E_DM0050_TABLE_NOT_LOADABLE);
	    return (E_DB_ERROR);
	}

	/*
	** If the table was found to be empty, pass the empty table flag.
	*/
	if (table_empty)
	    flag |= DM2R_EMPTY;
    }
    else
    {
        /*
        ** If DMR_TABLE_EXISTS is off, the table being loaded must be a
        ** temporary table, since we will NOT be performing any logging and
        ** will NOT be able to undo any aborted actions.
        */
        if (tcb->tcb_temporary != TCB_TEMPORARY)
        {
	    SETDBERR(&dmr->error, 0, E_DM002A_BAD_PARAMETER);
            return (E_DB_ERROR);
        }

	/*
	** For temporary HEAP tables there is no default
	** You must specify sort OR no sort.
	*/
	if (tcb->tcb_rel.relspec == TCB_HEAP)
	{
	    if (dmr->dmr_flags_mask & DMR_NOSORT)
	    {
		if (dmr->dmr_flags_mask &
	       (DMR_HEAPSORT | DMR_NODUPLICATES | DMR_SORT_NOCOPY))
		{
		    TRdisplay("NOSORT and SORT specified (%@)\n");
		    SETDBERR(&dmr->error, 0, E_DM002A_BAD_PARAMETER);
		    return (E_DB_ERROR);
		}
		flag |= DM2R_NOSORT;
	    }
	    else if ((dmr->dmr_flags_mask &
		(DMR_HEAPSORT | DMR_NODUPLICATES | DMR_SORT_NOCOPY)) == 0)
	    {
		TRdisplay("Neither SORT or NOSORT specified (%@)\n");
		SETDBERR(&dmr->error, 0, E_DM002A_BAD_PARAMETER);
		return (E_DB_ERROR);
	    }
	}

	/*
	** Internal temp tables are always loaded from scratch.
	*/
	flag |= DM2R_EMPTY;
    }

    /*
    ** Pass on estimated number of rows
    */
    row_estimate = dmr->dmr_s_estimated_records;
    if (row_estimate < 0)
	row_estimate = 0;
    do
    {
	/*
	** Setup information on how to build the table. Initially set these
	** from the tables TCB, then scan the dmr_char_array to see if the
	** user specifed and override.
	*/
	build_tbl_info.dm2r_fillfactor   = tcb->tcb_rel.reldfill;
	build_tbl_info.dm2r_nonleaffill  = tcb->tcb_rel.relifill;
	build_tbl_info.dm2r_leaffill     = tcb->tcb_rel.rellfill;
	build_tbl_info.dm2r_hash_buckets = tcb->tcb_rel.relprim;
	build_tbl_info.dm2r_extend       = tcb->tcb_rel.relextend;
	build_tbl_info.dm2r_allocation   = tcb->tcb_rel.relallocation;

	if ( dmr_cb->dmr_flags_mask & DMR_CHAR_ENTRIES )
	{
	    chr = (DMU_CHAR_ENTRY *)dmr_cb->dmr_char_array.data_address;
	    chr_count = dmr->dmr_char_array.data_in_size / 
						sizeof(DMU_CHAR_ENTRY);

	    if ( chr_count && chr == NULL )
	    {
		SETDBERR(&dmr->error, 0, E_DM002A_BAD_PARAMETER);
		status = E_DB_ERROR;
		break;
	    }

	    for ( i = 0; i < chr_count; i++)
	    {
		switch (chr[i].char_id)
		{
		case DMU_IFILL:
		    build_tbl_info.dm2r_nonleaffill = chr[i].char_value;
		    if ( build_tbl_info.dm2r_nonleaffill > 100 )
			build_tbl_info.dm2r_nonleaffill = 100;
		    continue;
			
		case DMU_LEAFFILL:
		    build_tbl_info.dm2r_leaffill = chr[i].char_value;
		    if ( build_tbl_info.dm2r_leaffill > 100 )
			build_tbl_info.dm2r_leaffill = 100;
		    continue;

		case DMU_DATAFILL:
		    build_tbl_info.dm2r_fillfactor = chr[i].char_value;
		    if ( build_tbl_info.dm2r_fillfactor > 100 )
			build_tbl_info.dm2r_fillfactor = 100;
		    continue;

		case DMU_MINPAGES:
		    min_pages = chr[i].char_value;
		    continue;

		case DMU_MAXPAGES:
		    max_pages = chr[i].char_value;
		    continue;

		case DMU_ALLOCATION:
		    build_tbl_info.dm2r_allocation = chr[i].char_value;
		    /*
		    ** If a mult-location table then round allocation size
		    */
		    if ( tcb->tcb_table_io.tbio_loc_count > 1 )
			dm2f_round_up(& build_tbl_info.dm2r_allocation );
		    continue;

		case DMU_EXTEND:
		    build_tbl_info.dm2r_extend = chr[i].char_value;
		    /*
		    ** If a mult-location table then round extend size
		    */
		    if ( tcb->tcb_table_io.tbio_loc_count > 1 )
			dm2f_round_up(& build_tbl_info.dm2r_extend );
		    continue;

		default:
		    SETDBERR(&dmr->error, i, E_DM000D_BAD_CHAR_ID);
		}
		break;
	    }

	    /*
	    ** See if we found an invalid parameter
	    */
	    if ( i < chr_count )
	    {
		status = E_DB_ERROR;
		break;
	    }
	}

	    
    } while (FALSE);

    if (status)
	return (status);

    /*
    ** If a HASH table determine the number of HASH buckets
    ** required
    */
    if ( tcb->tcb_rel.relspec == TCB_HASH )
    {
	/*
	** If user never set the min or max pages values then 
	** use the default values,  same as dmu_modify code.
	*/

	if ( min_pages == 0 )
	{
	    if (tcb->tcb_rel.relstat & TCB_COMPRESSED )
		min_pages = 1;
	    else
		min_pages = 10;

	    if ( min_pages > max_pages )
		min_pages = max_pages;

	}

	if ( max_pages == 0 )
	    max_pages = 8388607;

	build_tbl_info.dm2r_hash_buckets = dm2u_calc_hash_buckets( 
			    tcb->tcb_rel.relpgtype,
			    tcb->tcb_rel.relpgsize,
			    build_tbl_info.dm2r_fillfactor,
			    (i4)tcb->tcb_rel.relwid,
			    row_estimate,
			    min_pages,
			    max_pages );

	tcb->tcb_rel.relmin = min_pages;
	tcb->tcb_rel.relmax = max_pages;
    }

    row_count = 0;

    status = dm2r_load( rcb, tcb, DM2R_L_BEGIN, flag,
	                &row_count, dmr->dmr_mdata, row_estimate, 
			&build_tbl_info, &dmr->error);

    /* If we have LOB's around, make sure that it's marked as being
    ** part of a multi-row query.  Callers might set multi-row
    ** for their own purposes, but for some queries it's easier to
    ** set it here.  (e.g. create table as select)
    */
    if (status == E_DB_OK && rcb->rcb_bqcb_ptr != NULL)
	rcb->rcb_bqcb_ptr->bqcb_multi_row = TRUE;

    return (status);
}
