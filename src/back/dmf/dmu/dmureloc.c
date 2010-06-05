/*
**Copyright (c) 2010 Ingres Corporation
*/


#include    <compat.h>
#include    <gl.h>
#include    <tm.h>
#include    <cs.h>
#include    <me.h>
#include    <pc.h>
#include    <sr.h>
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
#include    <dmse.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dmu.h>
#include    <sxf.h>
#include    <dma.h>
#include    <dmpecpn.h>

/**
** Name: DMURELOC.C - Functions used to relocate a table.
**
** Description:
**      This file contains the functions necessary to relocate a table from
**      one location to another location.
**      This file defines:
**    
**      dmu_relocate()     -  Routine to perform the normal relocate table
**                            operation.
**
** History:
**      04-feb-86 (jennifer) 
**          Created for Jupiter.
**      20-nov-87 (jennifer)
**          Added multiple location support.
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
**      11-oct-1991 (jnash) merged 27-mar-87 (jennifer)
**          Added code for ORANGE project to test if requesting label is 
**          same as table label.
**	11-oct-1991 (jnash) merged 17-aug-1989 (Derek)
**	    Added DAC success auditing.
**      23-oct-1991 (jnash)
**          Fill dm0a_write parameter list.
**	6-jul-1992 (bryanp)
**	    Prototyping DMF.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	4-nov-1992 (robf)
**	    Security auditing now uses SXF message ID
**	16-Nov-1992 (fred)
**	    Added support for default relocation of large object tables.
**	18-nov-92 (robf)
**	    Removed dm0a.h since auditing now handled by SXF
**	    All auditing now thorough dma_write_audit()
**      04-dec-1992 (jnash)
**          Eliminate spurious LKrelease call.  X table control locks are
**	    held to end of transaction, and should not be released here.
**	16-feb-1993 (rogerk)
**	    Changed table-in-use check to look at tcb_open_count, not the
**	    tcb_ref_count.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**      24-Jun-1993 (fred)
**          Added include for dmpecpn.h to pick up dmpe prototypes.
**	1-jul-93 (robf)
**	    Pass security label to audit routines
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Replace all uses of DM_LKID with LK_LKID.
**	15-jul-1993 (jrb)
**	    Allow non-core cats to be relocated (formerly we disallowed
**	    relocation of any catalog).
**      16-Sep-1993 (fred)
**          Fixed checking of error flags for dmu_flags_mask.  This is 
**          allowed to be either DMU_EXTTOO or DMU_EXTONLY only.
**      14-mar-94 (stephenb)
**          Add new parameter to dma_write_audit() to match current prototype.
**	02-Oct-1994 (liibi01)
**	    Fix bug 60170
**      10-nov-1994 (cohmi01)
**          Bugs 48713, 45905
**          Obtain and pass timeout value to dm2t_control and dm2t_open.
**	17-sep-96 (nick)
**	    Remove compiler warning.
**	06-dec-1996 (nanpr01)
**	    bug 79484 : to let caller know which location failed in the
**	    location array we need to pass an extra argument.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_DM006A_TRAN_ACCESS_CONFLICT.
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to dma_write_audit() if not C2SECURE.
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


/*
** {
** Name: dmu_relocate - Relocates a table.
**
**  INTERNAL DMF call format:      status = dmu_relocate(&dmu_cb);
**
**  EXTERNAL call format: status = dmf_call(DMU_RELOCATE_TABLE,&dmu_cb);
**
** Description:
**      The dmu_relocate function handles the relocation of table from 
**      one location to another.  This dmu function is not allowed inside 
**      a user specified transaction.  The user must have access to the 
**      designated location.
**
**      Note: For B1 secure server, this routine must make sure
**      that the security label of the requester is exactly the
**      same as the security label on the table.  If is not 
**      identical, the operation is rejected with the error
**      E_DB0125_DDL_SECURITY_ERROR.
**
** Inputs:
**      dmu_cb
**          .type                           Must be set to DMU_UTILITY_CB.
**          .length                         Must be at least sizeof(DMU_CB).
**	    .dmu_flags_mask		    Set to one of
**  	    	    	    	    	    	DMU_EXTTOO_MASK - 
**  	    	    	    	    	    	   relocate extensions 
**  	    	    	    	    	    	   to the  same 
**  	    	    	    	    	    	   location, or 
**  	    	    	    	    	    	DMU_EXT_ONLY - 
**  	    	    	    	    	    	relocate only the 
**  	    	    	    	    	    	extensions. 
**  	    	    	    	    	    	
**	    .dmu_db_id                      Must be the identifier for the 
**                                          odcb returned from open database.
**          .dmu_tran_id                    Must be the current transaction 
**                                          identifier returned from the 
**                                          begin transaction operation.
**          .dmu_tbl_id                     Internal name of table to relocate.
**          .dmu_location.data_address      Pointer to an array of locations
**                                          of type DB_LOC_NAME.  There
**                                          can only be up to DM_LOC_MAX
**                                          entries in the array.
**                                          There must be one or more
**                                          entries specified
**                                          indicating the new
**                                          locations.
**          .dmu_location.data_in_size      The size in bytes of location
**                                          array.  Must be at least
**                                          size of DB_LOC_NAME.
**          .dmu_olocation.data_address     Pointer to an array of locations
**                                          of type DB_LOC_NAME.  There
**                                          can only be up to DM_LOC_MAX
**                                          entries indicating the old 
**                                          locations.  
**                                          If dmu_location
**                                          points to only one entry
**                                          then this list can be zero.  
**                                          If the table has only one 
**                                          location, the operation will
**                                          be performed, if it has more
**                                          than one it will be rejected.
**                                          This was done for compatability
**                                          with old syntax for single location
**                                          tables.
**                                          If dmu_location contains greater
**                                          tna one location, there must 
**                                          be the same number 
**                                          of entries specified for 
**                                          dmu_olocation.  
**          .dmu_olocation.data_in_size      The size in bytes of location
**                                          array.  Must be zero or 
**                                          size of DB_LOC_NAME times number
**                                          of locations.
**
** Output:
**      dmu_cb
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM0010_BAD_DB_ID
**                                          E_DM001A_BAD_FLAG
**                                          E_DM001D_BAD_LOCATION_NAME
**                                          E_DM001E_DUP_LOCATION_NAME
**                                          E_DM001F_LOCATION_LIST_ERROR
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
**                                          E_DM0071_LOCATIONS_TOO_MANY
**                                          E_DM0072_NO_LOCATION
**                                          E_DM0100_DB_INCONSISTENT
**					    E_DM0104_ERROR_RELOCATING_TABLE
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
** History:
**      04-feb-86 (jennifer) 
**          Created for Jupiter.
**      20-nov-87 (jennifer)
**          Added multiple location support.
**	30-sep-1988 (rogerk)
**	    Added capability to delay writing Begin Transaction record until
**	    first write operation.  If DELAYBT flag is set in the XCB, then
**	    call dmxe to write the BT record.
**	31-mar-1989 (EdHsu)
**	    Added param to dmxe_writebt() to support cluster online backup.
**	17-jan-1990 (rogerk)
**	    Add support for not writing savepoint records in readonly xacts.
**      11-oct-1991 (jnash) merged 27-mar-87 (jennifer)
**          Added code for ORANGE project to test if requesting label is 
**          same as table label.
**      11-oct-1991 (jnash) merged 17-aug-1989 (Derek)
**          Added DAC success auditing.
**      23-oct-1991 (jnash)
**          Fill dm0a_write parameter list.
**	16-Nov-1992 (fred)
**  	    Added (what will be an indirect recursive call) to dmpe_relocate() 
**	    when the table contains extensions and the user wants to relocate
**	    those as well.
**      04-dec-1992 (jnash)
**          Eliminate spurious LKrelease call.  X table control locks are
**	    held to end of transaction, and should not be released here.
**	16-feb-1993 (rogerk)
**	    Changed table-in-use check to look at tcb_open_count, not the
**	    tcb_ref_count.  The tcb may legally be referenced by DBMS
**	    background threads.
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	15-jul-1993 (jrb)
**	    Allow non-core cats to be relocated (formerly we disallowed
**	    relocation of any catalog).
**      16-Sep-1993 (fred)
**          Fixed checking of error flags for dmu_flags_mask.  This is 
**          allowed to be either DMU_EXTTOO or DMU_EXTONLY only.  Also
**          improve handling of relocating the extensions.
**	02-Oct-1994 (liibi01)
**	    Fix bug 60170, within dmu_relocate function call, check the
**	    the error value and the xcb flags from the dm2u_relocate call. If 
**	    the error occurs, make the qef_error function call directly and 
**	    pass the invalid location name. Dmu_cb.error will record the 
**	    error message.
**      10-nov-1994 (cohmi01)
**          Bugs 48713, 45905
**          Obtain and pass timeout value to dm2t_control and dm2t_open.
**	17-sep-96 (nick)
**	    Remove compiler warning.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction 
**	    access mode> (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_DM006A_TRAN_ACCESS_CONFLICT.
**	17-apr-97 (pchang)
**	    The timeout parameter was passed in the wrong order to dm2t_control
**	    when it was added in the above change by cohmi01 dtd nov94. (B81415)
**	6-Feb-2004 (schka24)
**	    Get rid of DMU statement count and its limit.
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
**	12-Apr-2010 (kschendel) SIR 123485
**	    Open table no-coupon to avoid unnecessary LOB overhead.
*/  

DB_STATUS
dmu_relocate(DMU_CB    *dmu_cb)
{
    DMU_CB	    	*dmu = dmu_cb;
    DML_XCB	    	*xcb;
    DMP_RCB	    	*relocate_rcb = (DMP_RCB *) 0;
    DML_ODCB	    	*odcb;
    DMP_TCB         	*tcb;    
    i4	    	error, local_error;
    i4	    	status;
    i4         	ref_count;
    DB_TAB_ID       	table_id;
    DB_LOC_NAME     	location;
    DB_LOC_NAME     	*oldlocation;
    DB_TAB_TIMESTAMP 	timestamp;
    i4         	db_lockmode;
    i4             lk_mode;
    DB_LOC_NAME     	*newlocation;
    i4         	loc_count, oloc_count;
    DB_OWN_NAME	    	table_owner;
    DB_TAB_NAME	    	table_name;
    i4                  has_extensions = 0;
    i4             timeout = 0;
    DB_ERROR		local_dberr;

    CLRDBERR(&dmu->error);

    lk_mode = DM2T_X;
    if (dmu->dmu_tbl_id.db_tab_base <= 4)
	lk_mode = DM2T_IX;

    status = E_DB_ERROR;
    do
    {
	if ( (dmu->dmu_flags_mask != 0) &&
	     ( (dmu->dmu_flags_mask &
		 ~(DMU_EXTTOO_MASK | DMU_EXTONLY_MASK | DMU_PARTITION | DMU_MASTER_OP)) != 0))
	{
	    SETDBERR(&dmu->error, 0, E_DM001A_BAD_FLAG);
	    break;
	}

	xcb = (DML_XCB*) dmu->dmu_tran_id;
	
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

	odcb = (DML_ODCB*) dmu->dmu_db_id;
	if (dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB) != E_DB_OK)
	{
	    SETDBERR(&dmu->error, 0, E_DM0010_BAD_DB_ID);
	    break;
	}
	if (xcb->xcb_scb_ptr != odcb->odcb_scb_ptr)
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
	/* Obtain timeout from set lockmode */
        timeout = dm2t_get_timeout(xcb->xcb_scb_ptr, &dmu->dmu_tbl_id);

    } while (FALSE);

    if (dmu->error.err_code)
    {
	if (dmu->error.err_code > E_DM_INTERNAL)
	{
	    uleFormat(&dmu->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char * )NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	    SETDBERR(&dmu->error, 0, E_DM0104_ERROR_RELOCATING_TABLE);
	}
	return (E_DB_ERROR);
    }

    /* Check the location names for duplicates, too many. */

    newlocation = 0;
    if (dmu->dmu_location.data_address && 
	(dmu->dmu_location.data_in_size >= sizeof(DB_LOC_NAME))
	)
    {
	newlocation = (DB_LOC_NAME *) dmu->dmu_location.data_address;
	loc_count = dmu->dmu_location.data_in_size/sizeof(DB_LOC_NAME);
    
    }
    else
    {
	if (dmu->dmu_location.data_address && dmu->dmu_location.data_in_size)
	    SETDBERR(&dmu->error, 0, E_DM001F_LOCATION_LIST_ERROR);
	else
	    SETDBERR(&dmu->error, 0, E_DM0072_NO_LOCATION);
	return (E_DB_ERROR);
    }

    oldlocation = 0;
    if (dmu->dmu_olocation.data_address && 
	(dmu->dmu_olocation.data_in_size >= sizeof(DB_LOC_NAME))
	)
    {
	oldlocation = (DB_LOC_NAME *) dmu->dmu_olocation.data_address;
	oloc_count = dmu->dmu_olocation.data_in_size/sizeof(DB_LOC_NAME);
	if (loc_count != oloc_count)
	{
	    SETDBERR(&dmu->error, 0, E_DM001F_LOCATION_LIST_ERROR);
	    return (E_DB_ERROR);
	}
    
    }
    else
    {
	oldlocation = 0;
	oloc_count = 0;
    }

    /* 
    ** All parameters are acceptable, now check the table id.
    */

    do
    {
	/* 
	** Request a table control lock for exclusively control of the
	** table.
	*/

	status = dm2t_control(odcb->odcb_dcb_ptr, dmu->dmu_tbl_id.db_tab_base,
		    xcb->xcb_lk_id, LK_X, (i4)0, timeout, &dmu->error);
	if (status != E_DB_OK)
	    break;

	/* 
	** Now try to open the table indicated by table id.  This will
	** place an exclusive lock on the table.  
	** Use error from open.
        */


	table_id.db_tab_base = dmu->dmu_tbl_id.db_tab_base;
	table_id.db_tab_index = dmu->dmu_tbl_id.db_tab_index;
   
	status = dm2t_open(odcb->odcb_dcb_ptr, &table_id, lk_mode,
	    DM2T_UDIRECT, DM2T_A_WRITE_NOCPN, timeout, (i4)20, xcb->xcb_sp_id,
	    xcb->xcb_log_id, xcb->xcb_lk_id, 0, 0, db_lockmode, 
            &xcb->xcb_tran_id, &timestamp,
	    &relocate_rcb, (DML_SCB *)0, &dmu->error);
	if (status)
	    break;

	/* Check the tcb to insure that the table to relocate is not
        ** currently open by this same transaction.  Save the old location.
        */

	tcb = relocate_rcb->rcb_tcb_ptr;
	ref_count = tcb->tcb_open_count;
	if (oloc_count == 0) 
	{
	    if ((tcb->tcb_rel.relstat & TCB_MULTIPLE_LOC)== 0)
	    {
		MEcopy((char *)&tcb->tcb_rel.relloc, 
		    sizeof(DB_LOC_NAME), (char *)&location);	
		oldlocation = &location;
	    }		
	    else
	    {
		SETDBERR(&dmu->error, 0, E_DM001F_LOCATION_LIST_ERROR);
		break;
	    }	    
	}

	/*
	** Note if the table has any extensions.  We will need the
	** info later.
	*/

	has_extensions = tcb->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS;
	
	/* Now close the table, use the purge flag to insure
        ** that the physical file is closed and the tcb deallocated. 
        */
	if (ref_count > 1)
	{
	    SETDBERR(&dmu->error, 0, E_DM005D_TABLE_ACCESS_CONFLICT);
	    break;
	}		

	/* Can't relocate a core system catalog; we used to disallow relocation
	** of ANY catalogs, but we are now relaxing that restriction.
	*/
	if ((tcb->tcb_rel.relstat & TCB_CONCUR) != 0)
	{
	    SETDBERR(&dmu->error, 0, E_DM009F_ILLEGAL_OPERATION);
	    break;

	}
	status = dm2t_close(relocate_rcb, DM2T_PURGE, &dmu->error);
	if (status != E_DB_OK)
	    break;
	relocate_rcb = (DMP_RCB *) 0;

    } while (FALSE);

    if (dmu->error.err_code)
    {
	if (relocate_rcb)
	    status = dm2t_close(relocate_rcb, DM2T_NOPURGE, &local_dberr);
	return(E_DB_ERROR);
    }

    /* Calls the physical layer to process the rest of the destroy */

    if (!(dmu->dmu_flags_mask & (DMU_EXTONLY_MASK)))
    {
	status = dm2u_relocate(odcb->odcb_dcb_ptr, xcb, &dmu->dmu_tbl_id, 
			    newlocation, oldlocation, loc_count, 
			    db_lockmode, &table_name, &table_owner, 
			    &dmu->error);
    }
    
    /* If there are extensions and they are to be relocated too, then do it */

    if (DB_SUCCESS_MACRO(status))
    {
	if (has_extensions &&
	        (dmu->dmu_flags_mask & (DMU_EXTTOO_MASK | DMU_EXTONLY_MASK)))
	{
	    status = dmpe_relocate(dmu, &dmu->error);
	}
    }

    /*	Audit successful creation of TABLE/VIEW. */

    if ( status == E_DB_OK && dmf_svcb->svcb_status & SVCB_C2SECURE )
    {
	/*
	**	Audit success
	*/
        status = dma_write_audit(
		SXF_E_TABLE,
		SXF_A_SUCCESS | SXF_A_RELOCATE,
		table_name.db_tab_name,	/* Table/view name */
		sizeof(table_name.db_tab_name),	/* Table/view name */
		&table_owner,	/* Table/view owner */
		I_SX2710_TABLE_RELOCATE,
		FALSE, /* Not force */
		&dmu->error, NULL);
    }

    if (status != E_DB_OK)
    {
	if (dmu->error.err_code > E_DM_INTERNAL)
	{
	    uleFormat(&dmu->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char * )NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	    SETDBERR(&dmu->error, 0, E_DM0104_ERROR_RELOCATING_TABLE);
	}
	switch (dmu->error.err_code)
	{
	    case E_DM004B_LOCK_QUOTA_EXCEEDED:
	    case E_DM0112_RESOURCE_QUOTA_EXCEED:
	    case E_DM0104_ERROR_RELOCATING_TABLE:
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
