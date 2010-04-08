/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <tm.h>
#include    <cs.h>
#include    <me.h>
#include    <pc.h>
#include    <sr.h>
#include    <sl.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <adp.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmucb.h>
#include    <dmxcb.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dmxe.h>
#include    <dm2t.h>
#include    <dm1c.h>
#include    <dm1b.h>
#include    <dm1cx.h>
#include    <dm1p.h>
#include    <dmu.h>
#include    <sxf.h>
#include    <dma.h>
#include    <dmse.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dmpecpn.h>


/**
** Name: DMUDESTROY.C - Functions used to destroy a table.
**
** Description:
**      This file contains the functions necessary to destroy a permanent table.
**      This file defines:
**    
**      dmu_destroy()     - Routine to perform the normal destroy table
**                          operation.
**
** History:
**      01-sep-85 (jennifer)
**	    Created for Jupiter.          
**	30-sep-1988 (rogerk)
**	    Added capability to delay writing Begin Transaction record until
**	    first write operation.  If DELAYBT flag is set in the XCB, then
**	    call dmxe to write the BT record.
**      27-mar-87 (jennifer)
**          Added code for ORANGE project to test if requesting label is 
**          same as table label.
**	17-aug-1989 (Derek)
**	    Added DAC success auditing.
**	17-jan-1990 (rogerk)
**	    Add support for not writing savepoint records in readonly xacts.
**	22-feb-1990 (fred)
**	    Added peripheral datatype support.  Destroy any extension tables if
**	    there are any.
**	17-jan-1992 (bryanp)
**	    Add support for dropping a temporary table.
**	6-jul-1992 (bryanp)
**	    Prototyping DMF.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	18-nov-92 (robf)
**	    Removed dm0a.h since auditing now handled by SXF
**	    All auditing now thorough dma_write_audit()
**      24-Jun-1993 (fred)
**          Added include of dmpecpn.h to pick up prototypes for dmpe
**          routines. 
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	27-aug-93 (stephenb)
**	    Changed call to dma_write_audit() to distinguish between tables
**	    and indexes. 
**	4-oct-93 (stephenb)
**	    added call to dma_write_audit() to audit drop of temporary table.
**	18-oct-93 (stephenb)
**	    added capability to distinguish gateway tables for auditing. Also
**	    audit on failure because we may fail in dm2u_destroy() due to lack
**	    of security privilege.
**      14-mar-94 (stephenb)
**          Add new parameter to dma_write_audit() to match current prototype.
**	28-jun-94 (robf)
**           Audit table security label on B1.
**	01-feb-96 (kch)
**	     In destroy_temporary_table(), we now test for session temporary
**	     extension tables and call dmpe_free_temps() (via dmpe_call())
**	     to delete them. This change fixes bug 72752.
**	31-jul-96 (kch)
**	     In destroy_temporary_table(), we now call dmpe_call() with the
**	     new flag ADP_FREE_ALL_OBJS to indicate that we want to free all
**	     the temp objects, including the sess temps. This change fixes
**	     bug 78030.
**	17-sep-96 (nick)
**	    Remove compiler warning.
**	 1-nov-96 (kch)
**	    In destroy_temporary_table(), added checks to determine whether
**	    a session temporary table contains BLOBs. This change fixes bug
**	    79184.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_DM006A_TRAN_ACCESS_CONFLICT.
**	14-nov-1998 (somsa01)
**	    In destroy_temporary_table(), since we are using
**	    ADP_FREE_ALL_OBJS, we now need to pass the xcb pointer via the
**	    pop_cb.  (Bug #79203)
**	07-dec-1998 (somsa01)
**	    In destroy_temporary_table(), re-worked the fix for bug #79203.
**	    Update the dmt_cb's of concern with the current transaction id
**	    here. The previous fix needed to modify adp.h, which compromised
**	    spatial objects (unless a change was made to iiadd.h, which we are
**	    hesitant to make).  (Bug #79203)
**	15-apr-1999 (somsa01)
**	    In destroy_temporary_table(), initialize pop_info to NULL.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to dma_write_audit() if not C2SECURE.
**      22-Mar-2001 (stial01)
**          destroy_temporary_table rewritten to destroy session temp etabs
**          before destroying the session temp. (b104317)
**	14-aug-2003 (jenjo02)
**	    dm2t_unfix_tcb() "tcb" parm now pointer-to-pointer,
**	    obliterated by unfix to prevent multiple unfixes
**	    by the same transaction and corruption of 
**	    tcb_ref_count.
**      14-jan-2004 (stial01)
**          ifdef TRdisplay
**      29-nov-2005 (horda03) Bug 48659/INGSRV 2716
**          A TX can now be started with a non-journaled BT record. This
**          leaves the XCB_DELAYBT flag set but sets the XCB_NONJNL_BT
**          flag. Need to write a journaled BT record if the update
**          will be journaled.
**	23-Oct-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_? functions converted to DB_ERROR *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**	08-Dec-2008 (jonj)
**	    SIR 120874: dma_? auditing functions converted to DB_ERROR *
**/

/* # defines */
/* typedefs */
/* forward references */
/* externs */
/* statics */
static DB_STATUS destroy_temporary_table(
			DML_ODCB    *odcb,
			DB_TAB_ID   *table_id,
			DML_XCB	    *xcb,
			DB_ERROR    *errcb);

/*{
** Name:  dmu_destroy - Destroy a table.
**
**  INTERNAL DMF call format:       status = dmu_destroy(&dmu_cb);
**
**  EXTERNAL call format:  status = dmf_call(DMU_DESTROY_TABLE,&dmu_cb);
**
** Description:
**      The dmu_destroy function handles the destruction of permanent 
**      tables or views. Note, this only deletes entries from the following 
**      primary system tables:
**
**        RELATION
**        ATTRIBUTE
**        INDEX
**
**      Other system tables such as permits must be updated prior to 
**      calling this routine.  The physical files associated  with the 
**      table and its indices are also destroyed.  If the table being 
**      destroyed is an index, then the base table entry in the system 
**      relation table is updated to indicate that the index has 
**      been destroyed.
**
**	If the table being destroyed has table extensions, then those extensions
**	are destroyed as well.
**
**      Note: For B1 secure server, this routine must make sure
**      that the security label of the requester is exactly the
**      same as the security label on the table.  If is not 
**      identical, the operation is rejected with the error
**      E_DB0125_DDL_SECURITY_ERROR.
**
** Inputs:
**      dmu_cb
**          .type                           Must be set to DMU_UTILIY_CB.
**          .length                         Must be at least sizeof(DMU_CB).
**          .dmu_tran_id                    Must be the current transaction 
**                                          identifier returned from the 
**                                          begin transaction operation.
**           .dmu_flags_mask                DMU flags:
**		DMU_PARTITION		    Dropping a partition (this implies
**					    that we're in a repartitioning
**					    modify).
**		DMU_DROP_NOFILE		    Skip actual disk file operations.
**					    (See flag def in dmucb.h for
**					    a discussion.)
**    
**           .dmu_db_id                     Internal identifier of database.
**           .dmu_tbl_id                    Internal name of table to destroy.
**         .dmu_char_array.data_address     Pointer to an area used to pass 
**                                          an array of entries of type
**                                          DMU_CHAR_ENTRY.
**                                          See below for description of 
**                                          <dmu_char_array> entries.
**         .dmu_char_array.data_in_size     Length of char_array data in bytes.
**
**         <dmu_char_array> entries are of type DMU_CHAR_ENTRY and 
**         must have following values:
**         char_id                          Characteristic identifier.
**                                          Must be one of the following:
**					    DMU_TEMP_TABLE - temporary table
**
** Outputs:
**      dmu_cb
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM0010_BAD_DB_ID
**                                          E_DM001A_BAD_FLAG
**                                          E_DM002A_BAD_PARAMETER
**                                          E_DM003B_BAD_TRAN_ID
**                                          E_DM0042_DEADLOCK
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**                                          E_DM0054_NONEXISTENT_TABLE
**                                          E_DM005D_TABLE_ACCESS_CONFLICT
**                                          E_DM005E_CANT_UPDATE_SYSCAT
**                                          E_DM0064_USER_INTR
**                                          E_DM0065_USER_ABORT
**	    				    E_DM006A_TRAN_ACCESS_CONFLICT
**                                          E_DM009D_BAD_TABLE_DESTROY
**                                          E_DM009F_ILLEGAL_OPERATION
**                                          E_DM0100_DB_INCONSISTENT
**                                          E_DM010C_TRAN_ABORTED
**                                          E_DM0125_DDL_SECURITY_ERROR
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_WARN                       Function completed normally with 
**                                          a termination status which is in 
**                                          dmu_cb.error.err_code.
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dmu_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmu_cb.error.err_code.
**    
** Side Effects:
**       Other tables to destroy such as associated indices are obtained from
**       internal control blocks containing information about the base table.
**
** History:
**      16-jan-86 (jennifer)
**          Created for Jupiter.      
**	30-sep-1988 (rogerk)
**	    Added capability to delay writing Begin Transaction record until
**	    first write operation.  If DELAYBT flag is set in the XCB, then
**	    call dmxe to write the BT record.
**      27-mar-87 (jennifer)
**          Added code for ORANGE project to test if requesting label is 
**          same as table label.
**	31-mar-1989 (EdHsu)
**	    Add param to dmxe_writebt call to support cluster online backup
**	17-jan-1990 (rogerk)
**	    Add support for not writing savepoint records in readonly xacts.
**	    Pass xcb argument to dmxe_writebt routine.
**	22-feb-1990 (fred)
**	    If there are extensions, then destroy any existing extensions to the
**	    table.  (For peripheral datatype support.)
**	17-jan-1992 (bryanp)
**	    Add support for dropping a temporary table.
**	04-nov-1992 (robf)
**	    When dropping a VIEW update auditing message appropriately.
**	1-jul-93 (robf)
**	    Write table security label  to security auditing
**	27-aug-93 (stephenb)
**	    Changed call to dma_write_audit() to distinguish between tables
**	    and indexes.
**	4-oct-93 (stephenb)
**	    added call to dma_write_audit() to audit drop of temporary table.
**	18-oct-93 (stephenb)
**	    changed variable is_view to table_type, it now holds 3 values
**	    (table, view or gateway). Updated dma_write_audit() to distinguish
**	    gateway remove from drop. Also audit on failure because we may fail
**	    in dm2u_destroy() due to lack of security privilege.
**	28-jun-94 (robf)
**           Audit table security label on B1.
**	17-sep-96 (nick)
**	    Remove compiler warning.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_DM006A_TRAN_ACCESS_CONFLICT.
**	14-Jan-2004 (jenjo02)
**	    Check index as db_tab_index > 0, not simply != 0.
**	6-Feb-2004 (schka24)
**	    Get rid of DMU statement count and its limit.
**	1-Mar-2004 (schka24)
**	    Dropping a partition implies we're in modify.
**	10-May-2004 (schka24)
**	    ODCB scb not the same as thread SCB in factotum, remove check.
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
*/
 
DB_STATUS
dmu_destroy(DMU_CB    *dmu_cb)
{
    DMU_CB	    *dmu = dmu_cb;
    DML_ODCB	    *odcb;
    DMU_CHAR_ENTRY  *char_entry;
    i4	    char_count;
    i4	    temporary = 0;
    i4	    drop_flags;
    i4	    i;
    i4	    error, local_error, status;
    DML_XCB	    *xcb;
    i4         db_lockmode;
    i4	    table_type = 0;
    i4	    has_extensions = 0;
    DB_OWN_NAME	    table_owner;
    DB_TAB_NAME	    table_name;
    DB_ERROR	    local_dberr;

    CLRDBERR(&dmu->error);

    status = E_DB_ERROR;

    /* Check input parameters. */
    do
    {
	if ((dmu->dmu_flags_mask & (~(DMU_PARTITION | DMU_DROP_NOFILE))) != 0)
	{
	    SETDBERR(&dmu->error, 0, E_DM001A_BAD_FLAG);
	    break;
	}

	xcb = (DML_XCB *)dmu->dmu_tran_id;
	if (dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) != E_DB_OK)
	{
	    SETDBERR(&dmu->error, 0, E_DM003B_BAD_TRAN_ID);
	    break;
	}

	/* Check for external interrupts */
	if ( xcb->xcb_scb_ptr->scb_ui_state )
	    dmxCheckForInterrupt(xcb, &error);
	
	if ( xcb->xcb_state )
	{
	    if (xcb->xcb_state & XCB_USER_INTR)
	    {
		SETDBERR(&dmu->error, 0, E_DM0065_USER_INTR);
		break;
	    }
	    if (xcb->xcb_state & XCB_FORCE_ABORT)
	    {
		SETDBERR(&dmu->error, 0, E_DM010C_TRAN_ABORTED);
		break;
	    }
	    if (xcb->xcb_state & XCB_ABORT)
	    {
		SETDBERR(&dmu->error, 0, E_DM0064_USER_ABORT);
		break;
	    }	    
	}

	odcb = (DML_ODCB *)dmu->dmu_db_id;
	if (dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB) != E_DB_OK)
	{
	    SETDBERR(&dmu->error, 0, E_DM0010_BAD_DB_ID);
	    break;
	}

	/*  Check that this is a update transaction on the database 
        **  that can be updated. */
	if (odcb != xcb->xcb_odcb_ptr)
	{
	    SETDBERR(&dmu->error, 0, E_DM005D_TABLE_ACCESS_CONFLICT);
	    break;
	}

	if (xcb->xcb_x_type & XCB_RONLY)
	{
	    SETDBERR(&dmu->error, 0, E_DM006A_TRAN_ACCESS_CONFLICT);
	    break;
	}


	/*  Check for characteristics. */

	if (dmu->dmu_char_array.data_address && 
		dmu->dmu_char_array.data_in_size)
	{
	    char_entry = (DMU_CHAR_ENTRY*) dmu->dmu_char_array.data_address;
	    char_count = dmu->dmu_char_array.data_in_size / 
			    sizeof(DMU_CHAR_ENTRY);
	    for (i = 0; i < char_count; i++)
	    {
		switch (char_entry[i].char_id)
		{
		case DMU_TEMP_TABLE:
		    temporary = char_entry[i].char_value == DMU_C_ON;
		    break;

		default:
		    SETDBERR(&dmu->error, i, E_DM000D_BAD_CHAR_ID);
		    return (E_DB_ERROR);
		}
	    }
	}

	if (temporary)
	{
	    /*
	    ** To destroy a temporary table, we call dmt_delete_temp.
	    */
	    status = destroy_temporary_table(odcb, &dmu->dmu_tbl_id, xcb,
						&dmu->error);
	    if (status != E_DB_OK)
	    {
		if (dmu->error.err_code > E_DM_INTERNAL)
		{
		    uleFormat(&dmu->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			    (char * )NULL, (i4)0, (i4 *)NULL, &local_error, 0);
		    SETDBERR(&dmu->error, 0, E_DM009D_BAD_TABLE_DESTROY);
		}
	    }
	    else if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
	    {
		/*
		** Audit successful drop of temporary table
		*/
            	status = dma_write_audit(SXF_E_TABLE,
		    SXF_A_SUCCESS | SXF_A_DROP,
		    dmu_cb->dmu_table_name.db_tab_name,	
		    sizeof(dmu_cb->dmu_table_name.db_tab_name),
		    &xcb->xcb_username, I_SX2041_TMP_TBL_DROP, FALSE,
		    &dmu->error, NULL);
	    }
	    return (status);
	}

	/*
	** If this is the first write operation for this transaction,
	** then we need to write the begin transaction record.
	*/
	if (xcb->xcb_flags & XCB_DELAYBT)
	{
	    status = dmxe_writebt(xcb, TRUE, &dmu->error);
	    if (status != E_DB_OK)
	    {
		xcb->xcb_state |= XCB_TRANABORT;
		break;
	    }
	}

	db_lockmode = DM2T_X;

    } while (FALSE);

    if (dmu->error.err_code)
    {
	if (dmu->error.err_code > E_DM_INTERNAL)
	{
	    uleFormat(&dmu->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char * )NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	    SETDBERR(&dmu->error, 0, E_DM009D_BAD_TABLE_DESTROY);
	}
	return (E_DB_ERROR);
    }

    /* If we're dropping a partition, this must be a repartitioning MODIFY
    ** and QEF is attempting to clean up after the repartitioning.
    ** Translate external flags to internal.
    */
    drop_flags = 0;
    if (dmu->dmu_flags_mask & DMU_PARTITION)
	drop_flags |= DM2U_MODIFY_REQUEST;
    if (dmu->dmu_flags_mask & DMU_DROP_NOFILE)
	drop_flags |= DM2U_DROP_NOFILE;

    /*
    **  Table id is valid, delete the table and all
    **  associated indices.
    */

    /* Calls the physical layer to process the rest of the destroy */

    status = dm2u_destroy((DMP_DCB *)odcb->odcb_dcb_ptr, xcb, &dmu->dmu_tbl_id, 
	db_lockmode, drop_flags, &table_name, &table_owner, &table_type,
	&has_extensions, &dmu->error);

    if ((status == E_DB_OK) && has_extensions)
    {
	status = dmpe_destroy(dmu, &dmu->error);
    }

    if (dmu->error.err_code > E_DM_INTERNAL)
    {
	uleFormat(&dmu->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char * )NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	SETDBERR(&dmu->error, 0, E_DM009D_BAD_TABLE_DESTROY);
    }

    /*	Audit drop of TABLE/VIEW. */

    if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
    {
	i4		msg_id;
	SXF_ACCESS	accessmask = SXF_A_SUCCESS;
	SXF_EVENT	ev_type;
	DB_STATUS	local_stat; /* take care to preserve original status
				    ** now we audit in either case */
	if (status != E_DB_OK)
	    accessmask = SXF_A_FAIL;
	/*
	** We must distinguish between a table and an index for the purpose
	** of auditing, the only way of doing that at this level is by relying
	** on the fact that the reltidx (dmu->dmu_tbl_id.db_tab_index) is always
	** zero for a base table and greater than zero for an index.
	*/
	if (dmu->dmu_tbl_id.db_tab_index > 0) /* an index */
	{
	    msg_id = I_SX2010_INDEX_DROP;
	    accessmask |= SXF_A_INDEX;
	    ev_type = SXF_E_TABLE;
	}
	else if (table_type & TCB_VIEW) /* a view */
	{
	    msg_id = I_SX2015_VIEW_DROP;
	    accessmask |= SXF_A_DROP;
	    ev_type = SXF_E_VIEW;
	}
	else if (table_type & TCB_GATEWAY) /* a gateway table */
	{
	    msg_id = I_SX2044_GW_REMOVE;
	    accessmask |= SXF_A_REMOVE;
	    ev_type = SXF_E_TABLE;
	}
	else /* an ingres table */
	{
	    msg_id = I_SX2025_TABLE_DROP;
	    accessmask |= SXF_A_DROP;
	    ev_type = SXF_E_TABLE;
	}
        local_stat = dma_write_audit(
		ev_type,
		accessmask,
		table_name.db_tab_name,	/* Table/view name */
		sizeof(table_name.db_tab_name),	/* Table/view name */
		&table_owner,	/* Table/view owner */
		msg_id, 
		FALSE, /* Not force */
		&local_dberr,
		NULL 	/* database name */
		);
    }
    if (status != E_DB_OK)
    {
	switch (dmu->error.err_code)
	{
	    case E_DM004B_LOCK_QUOTA_EXCEEDED:
	    case E_DM0112_RESOURCE_QUOTA_EXCEED:
	    case E_DM009D_BAD_TABLE_DESTROY:
	    case E_DM006A_TRAN_ACCESS_CONFLICT:
		xcb->xcb_state |= XCB_STMTABORT;
		break;

	    case E_DM0042_DEADLOCK:
	    case E_DM004A_INTERNAL_ERROR:
	    case E_DM0100_DB_INCONSISTENT:
		xcb->xcb_state |= XCB_TRANABORT;
		break;
	    case E_DM0065_USER_INTR:
		xcb->xcb_state |= XCB_USER_INTR;
		break;
	    case E_DM010C_TRAN_ABORTED:
		xcb->xcb_state |= XCB_FORCE_ABORT;
		break;
	}
	return (E_DB_ERROR);
    }
    return (E_DB_OK);    

}

/*
** Name: destroy_temporary_table	- destroy a temporary table
**
** Description:
**	This routine calls dmt_delete_temp to destroy a temporary table.
**
** Inputs:
**	odcb		    - Open Database Control Block
**	table_id	    - table ID
**	xcb		    - transaction control block
**
** Outputs:
**	errcb
**	    .err_code	    - set if an error is returned
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	20-sep-1991 (bryanp)
**	    Created.
**	01-feb-1996 (kch)
**	    Test for session temporary extension tables and call
**	    dmpe_free_temps() (via dmpe_call()) to delete them. This
**	    change fixes bug 72752.
**	31-jul-96 (kch)
**	    We now call dmpe_call() with the new flag ADP_FREE_ALL_OBJS to
**	    indicate that we want to free all the temp objects, including the
**	    sess temps. This change fixes bug 78030.
**	 1-nov-96 (kch)
**	    We now check for a complete traversal of the pop_dmt_cb linked
**	    list and return if this is the case. We also return if the
**	    difference between the db_tab_bases is not 2. Both these cases
**	    indicate that this session temporary table does not contain BLOBs.
**	    This change fixes bug 79184.
**	14-nov-1998 (somsa01)
**	    Since we are using ADP_FREE_ALL_OBJS, we now need to pass the
**	    xcb pointer via the pop_cb.  (Bug #79203)
**	07-dec-1998 (somsa01)
**	    Re-worked the fix for bug #79203. Update the dmt_cb's of
**	    concern with the current transaction id here. The previous fix
**	    needed to modify adp.h, which compromised spatial objects (unless
**	    a change was made to iiadd.h, which we are hesitant to make).
**	    (Bug #79203)
**	15-apr-1999 (somsa01)
**	    Initialize pop_info to NULL.
**	10-May-2004 (schka24)
**	    Make sure we look at parent SCB for etab hint.
*/
static DB_STATUS
destroy_temporary_table(
DML_ODCB    *odcb,
DB_TAB_ID   *table_id,
DML_XCB	    *xcb,
DB_ERROR    *errcb)
{
    DB_STATUS	    status;
    DB_STATUS	    local_status;
    DMT_CB	    dmt_cb;
    DB_TRAN_ID	    tran_id;
    LK_LLID         lock_list;
    LG_LXID         log_id;
    DMP_ET_LIST	    *list;
    DMP_TCB	    *tcb = NULL;
    i4		    error;

    /* Init common parts of DMT_CB for calling dmt_delete_temp */
    MEfill(sizeof(DMT_CB), '\0', (PTR)&dmt_cb);
    dmt_cb.length = sizeof(DMT_CB);
    dmt_cb.type = DMT_TABLE_CB;
    dmt_cb.dmt_db_id = (PTR)odcb;
    dmt_cb.dmt_tran_id = (PTR)xcb;

    /*
    ** If session has global temp etabs...
    ** Fix the tcb and find out if the temp we are destroying has extensons
    ** Since this is temp table, we don't need a control lock
    ** NOTE: We are looking at the parent SCB here!
    */
    if (odcb->odcb_scb_ptr->scb_global_lo)
    {
	tran_id.db_high_tran = 0;
	tran_id.db_low_tran = 0;
	lock_list = odcb->odcb_scb_ptr->scb_lock_list;
	log_id = 0;

	status = dm2t_fix_tcb(odcb->odcb_dcb_ptr->dcb_id, table_id, &tran_id,
	    lock_list, log_id, (DM2T_NOWAIT | DM2T_NOBUILD),
	    odcb->odcb_dcb_ptr, &tcb, errcb);
	if (status)
	    return(status);
	if (tcb->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS)
	{
	    /*
	    ** Temp we are destroying has etab(s),
	    ** Call dmt_delete_temp for each
	    */
	    for (list = tcb->tcb_et_list->etl_next;
			list != tcb->tcb_et_list;
				list = list->etl_next)
	    {
#ifdef xDEBUG
		TRdisplay("destroy_temporary_table: base %d etab %d\n",
		    list->etl_etab.etab_base, list->etl_etab.etab_extension);
#endif
		
		dmt_cb.dmt_id.db_tab_base = list->etl_etab.etab_extension;
		dmt_cb.dmt_id.db_tab_index = 0;
		status = dmt_delete_temp(&dmt_cb);
		if (status)
		{
		    *errcb = dmt_cb.error;
		    break;
		}
	    }
	}

	local_status = dm2t_unfix_tcb(&tcb, (i4)0, lock_list, log_id, errcb);
	if (local_status != E_DB_OK)
	    return (local_status);

	if (status != E_DB_OK)
	    return (status);
    }

    /*
    ** Set up DMT_CB control block appropriately and call dmt_delete_temp
    */
    STRUCT_ASSIGN_MACRO(*table_id, dmt_cb.dmt_id);
    status = dmt_delete_temp(&dmt_cb);

    if (status)
	*errcb = dmt_cb.error;

    return (status);
}
