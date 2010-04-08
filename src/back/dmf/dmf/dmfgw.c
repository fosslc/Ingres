/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <tr.h>
#include    <cs.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <pc.h>
#include    <dbms.h>
#include    <ulf.h>
#include    <adf.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dmucb.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dml.h>
#include    <dm2t.h>
#include    <dmftrace.h>
#include    <dm1c.h>	    /* need DM1C_BYTID, DM1C_GETNEXT flag values */
#include    <gwf.h>
#include    <dmfgw.h>
#include    <dm0m.h>
/**
**
**  Name: DMFGW.C - Interface to gateway routines.
**
**  Description:
**
**	Function prototypes are defined in DMFGW.H.
**
**	This module contains the routines which interface between DMF and
**	the Gateway Facility.  They format the Gateway control blocks and
**	send off requests to the gateway.
**
**	Contains routines:
**	    dmf_gwf_init - initialize connection to gateway facility.
**	    dmf_gws_term - terminate a session connection to gateway facility.
**	    dmf_gwt_register - register gateway table with Ingres DBMS.
**	    dmf_gwt_remove - remove gateway table from Ingres DMBS.
**	    dmf_gwt_open - open gateway table.
**	    dmf_gwt_close - close gateway table.
**	    dmf_gwi_register - register gateway index with Ingres DBMS.
**	    dmf_gwi_remove - remove gateway index from Ingres DBMS.
**	    dmf_gwr_position - position gateway table for selects.
**	    dmf_gwr_get - select row from gateway table.
**	    dmf_gwr_put - append a row to gateway table.
**	    dmf_gwr_replace - replace a row in a gateway table.
**	    dmf_gwr_delete - delete a row from a gateway table.
**	    dmf_gwx_begin - register transaction begin with gateways.
**	    dmf_gwx_abort - register transaction abort with gateways.
**	    dmf_gwx_commit - register transaction commit with gateways.
** 		checkForInterrupt - check for user interrupt
**
**  History:
**      15-aug-1989 (rogerk)
**          Created for Non-SQL Gateway.
**	9-apr-90 (bryanp)
**	    Made a number of small changes related to the moving of GWF
**	    initialization/termination to SCF and to the introduction of a
**	    standard gwf_call() interface for calling GWF routines:
**	    1) Deleted dmf_gwf_term, since SCF now terminates GWF.
**	    2) Changed dmf_gwf_init to use GWF_SVR_INFO.
**	    3) Changed calls to GWF routines to use gwf_call().
**	11-jun-1990 (bryanp)
**	    Re-map E_GW050D_DATA_CVT_ERROR to E_DM0141_GWF_DATA_CVT_ERROR.
**	    Re-map E_GW050E_ALREADY_REPORTED to D_DM0142_GWF_ALREADY_REPORTED.
**	22-jun-1990 (bryanp)
**	    Added casts to some expressions to clarify code and silence
**	    warnings The 'session_id', 'scf_session_id', and 'qual_arg' in the
**	    GW_RCB are PTR types, and must be cast to such when assigning them.
**	    When referencing them, appropriate casts must be used to define
**	    their types. This file makes a number of assumptions that pointers
**	    and i4s can be interchanged at will. All I did was cast them
**	    to admit that we know we are doing this...
**	14-aug-1990 (rogerk)
**	    Added new parameters to dm2t_get_tcb_ptr call.
**	26-nov-90 (linda)
**	    Deadlock handling.  Translate GWF error code E_GW0327_DMF_DEADLOCK
**	    back into DMF's E_DM0042_DEADLOCK.  This is done in routines:
**	    dmf_gwt_register, dmf_gwt_remove, dmf_gwt_open, dmf_gwt_close;
**	    these routines access the extended system catalogs which may
**	    result in deadlock situations which DMF must handle.
**	27-feb-1991 (bryanp)
**	    BUG: when the RCP attempts to abort a REGISTER TABLE statement, it
**	    crashes. The crash occurs in dmf_get_scb, which it deferences 0.
**	    When this code runs in a non-server (typically, the RCP, but
**	    perhaps also the JSP, etc.), the DML_SCF pointer comes back NULL,
**	    because there is no DMF session in a non-server. For now, until
**	    we decide what it means to call the GWF routines in a non-server,
**	    we'll just fail at this point by returning an error out of this
**	    routine if there is no DMF session.
**      10-oct-91 (jnash) merged 15-oct-90 (linda)
**          Several changes in support of both primary and secondary index
**          access.  (Moved only this comment; no code.)
**	15-oct-1991 (mikem)
**	    Eliminated Sun compiler warning ("illegal pointer combination") by
**	    casting a "i4 *" (&rcb->rcb_timeout) to a "char *".
**      15-may-92 (schang)
**          GW merge.
**	    18-apr-1991 (rickh)
**		Added USER_INTR (user interrupt) support for control-C.  
**		Put this into GET, PUT, DELETE, REPLACE.
**	8-july-1992 (ananth)
**	    Prototyping DMF.
**	30-sep-92 (robf)
**	    Added char_array parm to dmf_gwt_register so it can be passed down
**	    into GWF for further checks rather than the current empty values.
**	7-oct-92 (daveb)
**	    SCF is now doing GWF facility and session init/shutdown.
**	    Do things passively here, looking up rather than creating
**	    or destroying.
**	    Deleted dmf_gws_term, since SCF now terminates GWF sessions.
**	26-oct-1992 (rogerk)
**	    Reduced logging project: changed references to tcb fields to
**	    use new tcb_table_io structure fields.  Changed from old
**	    dm2t_get_tcb_ptr to new dm2t_fix_tcb.
**	26-oct-92 (rickh)
**	    Replaced references to DMP_ATTS with DB_ATTS.  Also replaced
**	    DM_CMP_LIST with DB_CMP_LIST.
**      06-nov-92 (schang)
**          Pass into gateway the access mode for proper testing.
**      10-dec-92 (schang)
**          add one more arg dest_rcb for dmf_gwt_remove.  This is the user
**          table tcb that eventually lead to file name for RMS GW
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to get logging system type definitions.
**      26-apr-93 (schang)
**          convert gwf_user_intr to dmf_user_intr
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to get locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**      08-sep-1993 (smc)
**          Made session ids CS_SID.
**	20-sep-1993 (andys)
**	     Fix arguments to ule_format call.
**	24-jan-1996 (toumi01; from 1.1 axp_osf port) (muhpa01)
**	     In dmf_gwt_open() modified code which calculates mem_needed for
**	     dm0m_allocate() - use ME_ALIGN_MACRO to pad size.	Also, modified
**	     code setting up the addresses in table_entry, column_array, &
**	     index_array - again with ME_ALIGN_MACRO.  This is needed in
**	     64-bit environments to prevent unaligned memory accesses
**	     (axp_osf port).
**	15-oct-96 (mcgem01)
**	    E_DM9A06, E_DM9A07, E_DM9A08 and E_DM9A09 now take 
**	    SystemProductName as a parameter.
**	28-oct-1996 (hanch04)
**	    Fixed typo
**	29-oct-1996 (canor01)
**	    Include <st.h> for definition of STlength().
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm2t_unfix_tcb() calls.
**	11-Aug-1998 (jenjo02)
**	    Extract Table's tuple and page counts from the RCB instead
**	    of the TCB.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Jan-2001 (jenjo02)
**	    Use macro to find *DML_SCB instead of SCU_INFORMATION.
**	    Correctly cast "session_id (GW_SESSION*)" to PTR
**	    instead of CS_SID.
**	    Deleted dmf_gws_init() function. GW_SESSION* now passed
**	    to dmc_start() when session started.
**	14-aug-2003 (jenjo02)
**	    dm2t_unfix_tcb() "tcb" parm now pointer-to-pointer,
**	    obliterated by unfix to prevent multiple unfixes
**	    by the same transaction and corruption of 
**	    tcb_ref_count.
** 	26-aug-03 (chash01): IN dmf_gwt_open():
**          When dealing with 2ndary index Ingres logic is that if the 
**          secondary index is non-unique, TIDP count stays.  Since it is 
**          possible to have non-unique non-hash secondary index, we need to 
**          remove TIDP count from this kind of index so that RMS can deal 
**          with it uniformally. This is part of fix for bug 110758.chash
**	16-dec-04 (inkdo01)
**	    Add a couple of collID's.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
**	21-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dmf_gw? functions converted to DB_ERROR *
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	29-Sept-2009 (troal01)
**	    Added geospatial support.
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**/

/* Static function prototype defintions. */

static DB_STATUS dmf_get_scb(
    DML_SCB	**scb,
    DB_ERROR	*dberr);

static DML_XCB *dmf_get_xcb(
    DML_SCB	*scb,
    DMP_RCB	*rcb);

static VOID checkForInterrupt(
    DMP_RCB	*rcb,
    DB_STATUS   *status,
    DB_ERROR	*dberr);

/*{
** Name: dmf_gwf_init - Initialize DMF Gateway.
**
** Description:
**	Called at DMC_START_SERVER time.
**
**	Returns value (in "gateway_active" parameter) indicating whether there
**	is actually a gateway attached to the server.
**
**	Returns value (in "gw_xacts" parameter) indicating whether any of the
**	gateways require transaction notification.  If so, then all transaction
**	begins and ends must be call to the gateway as well.
**
** Inputs:
**	none.
**
** Outputs:
**	gateway_active	- TRUE if DMF Gateway is present.
**	gw_xacts	- TRUE if gateway requires transactions.
**	err_code	- reason for error:
**			E_DM0112_RESOURCE_QUOTA_EXCEED
**			E_DM0136_GATEWAY_ERROR
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-aug-89 (rogerk)
**          Created for Non-SQL Gateway.
**	9-apr-90 (bryanp)
**	    No longer initializes GWF facility. Instead uses GWF_SVR_INFO.
*/
DB_STATUS
dmf_gwf_init(
i4	    *gateway_active,
i4	    *gw_xacts,
DB_ERROR    *dberr)
{
    GW_RCB	    gw_rcb;
    DB_STATUS	    status;
    i4	    local_error;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (DMZ_TBL_MACRO(20))
	TRdisplay("DMF GATEWAY : Calling GWF_INIT\n");

    /*
    ** Assume there is an active gateway unless gwf_svr_info returns with
    ** different information set in fields in the gw_rcb.
    */
    *gateway_active = TRUE;
    *gw_xacts = TRUE;

    gw_rcb.gwr_type = GWR_CB_TYPE;
    gw_rcb.gwr_length = sizeof(GW_RCB);

    status = gwf_call(GWF_SVR_INFO, &gw_rcb);
    if (status)
    {
	uleFormat(NULL, E_DM9A01_GATEWAY_INIT, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char * )0, (i4)0, (i4 *)NULL, &local_error, 1,
		STlength(SystemProductName), SystemProductName);
	uleFormat(&gw_rcb.gwr_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char * )0, (i4)0, (i4 *)NULL, &local_error, 0);

	switch(gw_rcb.gwr_error.err_code)
	{
	    case E_GW0600_NO_MEM:
		SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
		break;

	    default:
		SETDBERR(dberr, 0, E_DM0136_GATEWAY_ERROR);
	}
    }
    else
    {
	*gateway_active = gw_rcb.gwr_gw_active;
	*gw_xacts       = gw_rcb.gwr_gw_xacts;
    }

    if (DMZ_TBL_MACRO(20))
	TRdisplay("DMF GATEWAY : GWF_INIT returns %d\n", status);

    return (status);
}

/*{
** Name: dmf_gwt_register - Register table with the gateway.
**
** Description:
**	Called at DMU_CREATE time when creating a gateway table.  Calls gateway
**	facility to register the table.  No Ingres table actually created.
**	Ingres will have created all the system catalog entries to describe the
**	table and this call registers the foriegn table with the ingres
**	gateway.
**
**	This routine is called to register both BASE tables and INDEX tables.
**	It calls the appropriate gateway routine (gwt_register, gwi_register)
**	depending on the table type.
**
** Inputs:
**	session_id	- gateway session id returned by dmf_gws_init call.
**	table_name	- name of table as registered to Ingres database.
**	table_owner	- owner of table as registered to Ingres database.
**	table_id	- Ingres table id for this table.
**	attr_count	- number of columns in table.
**	attr_list	- list of columns in DMF_ATTR_ENTRY array
**	gwchar_array	- array of gateway-specific table attributes
**	gwattr_array	- array of gateway-specific column attributes
**	xcb		- transaction control block: contains database id and
**			  transaction id.
**	source		- a pointer to the source (file, path, etc) for object
**	gw_id		- gateway id for foreign database owner of table.
**	char_array	- list of table attributes
**
** Outputs:
**	tuple_count	- number of rows in gateway table.
**	page_count	- number of pages in gateway table.
**	err_code	- reason for error:
**			E_DM0112_RESOURCE_QUOTA_EXCEED
**			E_DM0136_GATEWAY_ERROR
**			E_DM0142_GWF_ALREADY_REPORTED
**			E_DM0042_DEADLOCK
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-aug-89 (rogerk)
**          Created for Non-SQL Gateway.
**	24-dec-89 (paul)
**	    A GW_RCB parameter was being overwritten. Fixed.
**	9-apr-90 (bryanp)
**	    New GWF interface, using gwf_call().
**	12-jun-1990 (bryanp)
**	    Re-map E_GW050E_ALREADY_REPORTED to D_DM0142_GWF_ALREADY_REPORTED.
**	16-jun-90 (linda)
**	    ifdef out the assignment to tuple_count.  Gateway specification is
**	    that user supplies rowcount in the register table command.  I have
**	    left the parameter in though, in case future gateways find a use
**	    for it.  At present, calling routine (dm2u_create) passes it in
**	    but does nothing further with it.
**	14-aug-90 (rogerk)
**	    Added new parameters to dm2t_get_tcb_ptr call.  This routine does
**	    not require tcb validation, so pass in novalidate flag.
**	30-sep-92 (robf)
**	    Added char_array parameter so it can be passed down into GWF
**	    for further checks rather than the current empty values.
**	26-oct-1992 (rogerk)
**	    Reduced logging project: changed references to tcb fields to
**	    use new tcb_table_io structure fields.  Changed from old
**	    dm2t_get_tcb_ptr to new dm2t_fix_tcb.
**	20-sep-1993 (andys)
**	     Fix arguments to ule_format call.
**	06-oct-93 (swm)
**	     Bug #56443
**	     Change i4 cast to PTR now that gwr_database_id is a PTR.
**	14-Jan-2004 (jenjo02)
**	    Check db_tab_index > 0 for index, not simply != 0.
*/
DB_STATUS
dmf_gwt_register(
PTR		    session_id,
DB_TAB_NAME	    *table_name,
DB_OWN_NAME	    *table_owner,
DB_TAB_ID	    *table_id,
i4		    attr_count,
DMF_ATTR_ENTRY	    **attr_list,
DM_DATA		    *gwchar_array,
DM_PTR		    *gwattr_array,
DML_XCB		    *xcb,
DMU_FROM_PATH_ENTRY *source,
i4		    gw_id,
i4		    *tuple_count,
i4		    *page_count,
DM_DATA		    *char_array,
DB_ERROR	    *dberr)
{
    GW_RCB		gw_rcb;
    DB_STATUS		status;
    DB_TAB_ID		base_tab_id;
    DMP_MISC		*mem_ptr;
    DMT_ATT_ENTRY	*column_array;
    DMF_ATTR_ENTRY	*attr_ptr;
    DMP_TCB		*tcb;
    i4		mem_needed;
    i4		offset;
    i4			i;
    i4		local_error;
    DML_ODCB		*open_db;

    CLRDBERR(dberr);

    if (DMZ_TBL_MACRO(20))
    {
	TRdisplay("DMF GATEWAY : Calling GWS_REGISTER for table %.12s\n",
	    table_name->db_tab_name);
    }

    gw_rcb.gwr_session_id = session_id;
    gw_rcb.gwr_xact_id = (PTR)xcb;
    gw_rcb.gwr_database_id = (PTR)xcb->xcb_odcb_ptr;
    STRUCT_ASSIGN_MACRO(*table_name, gw_rcb.gwr_tab_name);
    STRUCT_ASSIGN_MACRO(*table_owner, gw_rcb.gwr_tab_owner);
    STRUCT_ASSIGN_MACRO(*table_id, gw_rcb.gwr_tab_id);

    /*
    ** Build attribute list from DMF_ATTR_ENTRY array.  Allocate array to
    ** build list into.
    */
    mem_needed = attr_count * sizeof(DMT_ATT_ENTRY) + sizeof(DMP_MISC);
    status = dm0m_allocate(mem_needed, (i4)0, (i4)MISC_CB,
	(i4)MISC_ASCII_ID, (char *)NULL, (DM_OBJECT**)&mem_ptr, dberr);
    if (status)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char * )0, (i4)0, (i4 *)NULL, &local_error, 0);
	SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
	return (E_DB_ERROR);
    }

    column_array = (DMT_ATT_ENTRY *)((char *)mem_ptr + sizeof(DMP_MISC));
    mem_ptr->misc_data = (char*)column_array;

    for (i = 0, offset = 0; i < attr_count; i++)
    {
	attr_ptr = attr_list[i];
	STRUCT_ASSIGN_MACRO(attr_ptr->attr_name, column_array[i].att_name);
	column_array[i].att_number = i + 1;
	column_array[i].att_offset = offset;
	column_array[i].att_type = attr_ptr->attr_type;
	column_array[i].att_width = attr_ptr->attr_size;
	column_array[i].att_prec = attr_ptr->attr_precision;
	column_array[i].att_collID = attr_ptr->attr_collID;
	column_array[i].att_flags = attr_ptr->attr_flags_mask;
	column_array[i].att_key_seq_number = 0;
	column_array[i].att_geomtype = attr_ptr->attr_geomtype;
	column_array[i].att_srid = attr_ptr->attr_srid;

	offset += attr_ptr->attr_size;
    }

    /*
    ** Fill in gateway specific attribute and table information.
    */
    gw_rcb.gwr_column_cnt = attr_count;
    gw_rcb.gwr_attr_array.ptr_address = (PTR)column_array;
    gw_rcb.gwr_in_vdata1.data_address = gwchar_array->data_address;
    gw_rcb.gwr_in_vdata1.data_in_size = gwchar_array->data_in_size;
    gw_rcb.gwr_in_vdata2.data_address = source->from_buffer;
    gw_rcb.gwr_in_vdata2.data_in_size = source->from_size;
    gw_rcb.gwr_in_vdata_lst.ptr_address = gwattr_array->ptr_address;
    gw_rcb.gwr_in_vdata_lst.ptr_in_count = gwattr_array->ptr_in_count;
    gw_rcb.gwr_in_vdata_lst.ptr_size = gwattr_array->ptr_size;
    if (char_array)
    {
	gw_rcb.gwr_char_array.data_address = char_array->data_address;
	gw_rcb.gwr_char_array.data_in_size = char_array->data_in_size;
    }
    else
    {
	gw_rcb.gwr_char_array.data_address = 0;
	gw_rcb.gwr_char_array.data_in_size = 0;
    }

    /*
    ** If this is a secondary index, then pass in the gateway id of the parent
    ** table.  Look up the parent TCB in order to find the gateway id.  The
    ** parent TCB should be reserved in the TCB cache by the index operation.
    */
    if (table_id->db_tab_index > 0)
    {
	base_tab_id.db_tab_base = table_id->db_tab_base;
	base_tab_id.db_tab_index = 0;
        status = dm2t_fix_tcb(xcb->xcb_odcb_ptr->odcb_dcb_ptr->dcb_id,
	    &base_tab_id, (DB_TRAN_ID *)NULL, xcb->xcb_lk_id, xcb->xcb_log_id,
	    (i4)(DM2T_NOBUILD | DM2T_NOWAIT), (DMP_DCB*)NULL, &tcb, dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char * )NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	    SETDBERR(dberr, 0, E_DM9A05_GATEWAY_REGISTER);
	    return (E_DB_ERROR);
	}
	gw_rcb.gwr_gw_id = tcb->tcb_rel.relgwid;

	/*
	** Release handle to the TCB.
	*/
        status = dm2t_unfix_tcb(&tcb, (i4)0, xcb->xcb_lk_id, xcb->xcb_log_id, dberr);
        if (status != E_DB_OK)
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char * )NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	    SETDBERR(dberr, 0, E_DM9A05_GATEWAY_REGISTER);
	    return (E_DB_ERROR);
	}
    }
    else
    {
	gw_rcb.gwr_gw_id = gw_id;
    }

    gw_rcb.gwr_type = GWR_CB_TYPE;
    gw_rcb.gwr_length = sizeof(GW_RCB);

    /*
    ** Call appropriate register routine.  If this is an index (indicated by a
    ** greater than zero db_tab_index value), then call the index register routine.
    */
    if (table_id->db_tab_index > 0)
	status = gwf_call(GWI_REGISTER, &gw_rcb);
    else
	status = gwf_call(GWT_REGISTER, &gw_rcb);
    if (status)
    {
	switch (gw_rcb.gwr_error.err_code)
	{
	    case E_GW050E_ALREADY_REPORTED:
		SETDBERR(dberr, 0, E_DM0142_GWF_ALREADY_REPORTED);
		break;

	    case E_GW0600_NO_MEM:
		SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
		break;

	    case E_GW0327_DMF_DEADLOCK:
		SETDBERR(dberr, 0, E_DM0042_DEADLOCK);
		break;

	    default:
		uleFormat(NULL, E_DM9A05_GATEWAY_REGISTER, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, (char * )0, (i4)0,
			    (i4 *)NULL, &local_error, 0);
		uleFormat(&gw_rcb.gwr_error, 0, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, (char * )0, (i4)0,
			    (i4 *)NULL, &local_error, 0);

		SETDBERR(dberr, 0, E_DM0136_GATEWAY_ERROR);
		break;
	}
    }

    /*
    ** Return the tuple and page counts for this table.  This gives an
    ** indication of table size to the Ingres DBMS for use by the optimizer.
    */
#if 0
    *tuple_count = gw_rcb.gwr_tuple_count;
#endif
    *page_count = gw_rcb.gwr_page_count;

    /*
    ** Return memory used for attribute array.
    */
    dm0m_deallocate((DM_OBJECT**)&mem_ptr);

    return (status);
}

/*{
** Name: dmf_gwt_remove - Remove register of table with the gateway.
**
** Description:
**	Called at DMU_DESTROY time when destroying the ingres definition of a
**	gateway table.  Calls the gateway to remove any knowledge of the table.
**
**	This routine is called to remove both BASE tables and INDEX tables.  It
**	calls the appropriate gateway routine (gwt_remove, gwi_remove)
**	depending on the table type.
**
** Inputs:
**	gateway_id	- gateway id for foreign database owner of table.
**	session_id	- gateway session id returned by dmf_gws_init call.
**	table_id	- Ingres table id for this table.
**	table_name	- name of table as registered to Ingres database.
**	table_owner	- owner of table as registered to Ingres database.
**      dest_rcb        - table rcb
**	xcb		- transaction control block: contains database id and
**			  transaction id.
**
** Outputs:
**	err_code	- reason for error:
**			E_DM0112_RESOURCE_QUOTA_EXCEED
**			E_DM0136_GATEWAY_ERROR
**			E_DM0042_DEADLOCK
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-aug-89 (rogerk)
**          Created for Non-SQL Gateway.
**	9-apr-90 (bryanp)
**	    New GWF interface, using gwf_call().
**      15-may-92 (schang)
**          GW merge
**	    28-mar-91 (linda)
**	        For either tables or indexes, go through GWF's gwt_remove().
**      10-dec-92 (schang)
**          add one more arg dest_rcb for dmf_gwt_remove.  This is the user
**          table tcb that eventually lead to file name for RMS GW
**	06-oct-93 (swm)
**	     Bug #56443
**	     Change i4 cast to PTR now that gwr_database_id is a PTR.
*/
DB_STATUS
dmf_gwt_remove(
i4		    gateway_id,
PTR		    session_id,
DB_TAB_ID	    *table_id,
DB_TAB_NAME	    *table_name,
DB_OWN_NAME	    *table_owner,
DMP_RCB             *dest_rcb,
DML_XCB		    *xcb,
DB_ERROR	    *dberr)
{
    GW_RCB		gw_rcb;
    DB_STATUS		status;
    i4		local_error;

    CLRDBERR(dberr);

    if (DMZ_TBL_MACRO(20))
    {
	TRdisplay("DMF GATEWAY : Calling GWT_REMOVE for table id: (%d, %d)\n",
	    table_id->db_tab_base, table_id->db_tab_index);
    }

    /*
    ** schang : here we pass on the pointer that will
    **   eventually lead us to the file name so that
    **   we can close file by name at table remove time
    **   we also set gwt_open_noaccess as access_mode
    */
    gw_rcb.gwr_gw_rsb = dest_rcb->rcb_gateway_rsb;
    gw_rcb.gwr_type = GWR_CB_TYPE;
    gw_rcb.gwr_length = sizeof(GW_RCB);
    gw_rcb.gwr_access_mode = GWT_OPEN_NOACCESS;
    gw_rcb.gwr_gw_id = gateway_id;
    gw_rcb.gwr_session_id = session_id;
    gw_rcb.gwr_xact_id = (PTR)xcb;
    gw_rcb.gwr_database_id = (PTR)xcb->xcb_odcb_ptr;
    STRUCT_ASSIGN_MACRO(*table_id, gw_rcb.gwr_tab_id);
    STRUCT_ASSIGN_MACRO(*table_name, gw_rcb.gwr_tab_name);
    STRUCT_ASSIGN_MACRO(*table_owner, gw_rcb.gwr_tab_owner);

    status = gwf_call(GWT_REMOVE, &gw_rcb);

    if (status)
    {
	uleFormat(NULL, E_DM9A06_GATEWAY_REMOVE, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char * )0, (i4)0, (i4 *)NULL, &local_error, 1,
	    STlength(SystemProductName), SystemProductName);
	uleFormat(&gw_rcb.gwr_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char * )0, (i4)0, (i4 *)NULL, &local_error, 0);

	switch (gw_rcb.gwr_error.err_code)
	{
	    case E_GW0600_NO_MEM:
		SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
		break;

	    case E_GW0327_DMF_DEADLOCK:
		SETDBERR(dberr, 0, E_DM0042_DEADLOCK);
		break;

	    default:
		SETDBERR(dberr, 0, E_DM0136_GATEWAY_ERROR);
	}
    }

    return (status);
}

/*{
** Name: dmf_gwt_open - Open gateway table in the foreign database.
**
** Description:
**	Called at DMT_OPEN time to open a table in the gateway.
**
**	The gateway returns a gateway RSB that may be used for row requests
**	(GET, PUT, DELETE, REPLACE, POSITION) on this table.  Eventually, a
**	corresponding dmf_gwt_close call must be made with the returned RSB to
**	close the table.
**
**	If dmf_gwt_open is called with a secondary index, then do an open on
**	the base table rather than the secondary index.
**
**	This routine puts the RSB into the table RCB for future dmf_gw* calls.
**
** Inputs:
**	rcb		- record control block used by Ingres server to describe
**			  open instance of the table.  Contains pointer to
**			  table control block.
**
** Outputs:
**	err_code	- reason for error:
**			E_DM0054_NONEXISTENT_TABLE
**			E_DM004B_LOCK_QUOTA_EXCEEDED
**			E_DM0112_RESOURCE_QUOTA_EXCEED
**			E_DM0136_GATEWAY_ERROR
**			E_DM0142_GWF_ALREADY_REPORTED
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-aug-89 (rogerk)
**          Created for Non-SQL Gateway.
**	9-apr-90 (bryanp)
**	    New GWF interface, using gwf_call().
**	11-jun-1990 (bryanp)
**	    Re-map GWF errors into DMF errors.
**	18-jun-90 (linda, bryanp)
**	    Add RCB_A_OPEN_NOACCESS and RCB_A_MODIFY flags support.  The
**	    RCB_A_OPEN_NOACCESS flag means that the underlying data file is not
**	    going to be accessed, and need not be really opened; the
**	    RCB_A_MODIFY flag means that we want to extract statistical
**	    information about the file, to store in the system catalogs; but if
**	    the file does not exist it is okay (a warning will be issued, and
**	    default values will be used for some items).
**	25-jan-91 (RickH)
**	    Cram timeout value into gw_rcb.  Temporary fix (!).
**	15-oct-1991 (mikem)
**	    Eliminated Sun compiler warning ("illegal pointer combination") by
**	    casting a "i4 *" (&rcb->rcb_timeout) to a "char *".
**	06-oct-93 (swm)
**	     Bug #56443
**	     Change i4 cast to PTR now that gwr_database_id is a PTR.
**	11-Aug-1998 (jenjo02)
**	    Extract Table's tuple and page counts from the RCB instead
**	    of the TCB.
**	30-May-2006 (jenjo02)
**	    Index attributes upped from DB_MAXKEYS to DB_MAXIXATTS
*/
DB_STATUS
dmf_gwt_open(
DMP_RCB		    *rcb,
DB_ERROR	    *dberr)
{
    GW_RCB		gw_rcb;
    DMP_TCB		*tcb = rcb->rcb_tcb_ptr;
    DMP_TCB		*itcb;
    DML_SCB		*scb;
    DML_XCB		*xcb;
    DB_ATTS		*attr_ptr;
    DMT_ATT_ENTRY	*column_array;
    DMT_IDX_ENTRY	*index_array;
    DMT_TBL_ENTRY	*table_entry;
    DMP_MISC		*mem_ptr;
    DB_STATUS		status;
    i4		local_error;
    i4		mem_needed;
    i4			i, j;

    CLRDBERR(dberr);

    if (DMZ_TBL_MACRO(20))
    {
	TRdisplay("DMF GATEWAY : Calling GWT_OPEN for table %.12s\n",
	    tcb->tcb_rel.relid.db_tab_name);
    }

    STRUCT_ASSIGN_MACRO(tcb->tcb_rel.reltid, gw_rcb.gwr_tab_id);

    /*
    ** If this is a secondary index, then open the base table instead of the
    ** secondary index.  Reset tcb to point to the base tcb.  Everything in the
    ** GW_RCB will refer to the base table execpt for the table id which will
    ** have the tabid pair (base table, index table).
    */

    /*
    ** This does mean opening the base table twice, since qef will issue an
    ** open on the base and then on the 2-ary to perform the restricted
    ** tid-join.  QUESTION:  what about the case of a direct retrieve from the
    ** 2-ary?  ANSWER:  we don't allow them.
    ** NEW ANSWER:  no problem, we know whether or not we're in a tid join
    ** situation; if not then this is a direct retrieve from the secondary and
    ** we do it directly just as for a base table.
    */

    /*
    ** NOTE:  what about key column translation?  ANSWER:  handle during the
    ** position; we build up a list of indexes here and we keep the tab_id for
    ** the index; so we know what to use.  Keep the info during get's from the
    ** base table -- the qualification function expects the columns to be
    ** qualified on to match the layout in the base table.
    */

    /*
    ** NOTE we use the base table name here...  Might be misleading if it
    ** appears in an error message.
    ** NEW NOTE:  copy name & owner before switching tcb's...
    ** Copy name & owner before switching tcb's...
    */
    STRUCT_ASSIGN_MACRO(tcb->tcb_rel.relid, gw_rcb.gwr_tab_name);
    STRUCT_ASSIGN_MACRO(tcb->tcb_rel.relowner, gw_rcb.gwr_tab_owner);

    if (tcb->tcb_rel.relstat & TCB_INDEX)
	tcb = tcb->tcb_parent_tcb_ptr;

    /*
    ** NOTE we get gw_id after switching tcb's; gw_id is 0 for gateway 2-aries
    ** and is always filled in from the parent.
    ** FIXME:  This is not longer true; we keep non-zero gw_id for both base
    ** tables and secondaries.
    */
    gw_rcb.gwr_gw_id = tcb->tcb_rel.relgwid;

    /*
    ** Get session control block so we can get our gateway session id.
    */
    if ( xcb = rcb->rcb_xcb_ptr )
	scb = xcb->xcb_scb_ptr;
    else
    {
	status = dmf_get_scb(&scb, dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(NULL, E_DM9A07_GATEWAY_OPEN, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char * )0, (i4)0, (i4 *)NULL, &local_error, 1,
		STlength (SystemProductName), SystemProductName);
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char * )0, (i4)0, (i4 *)NULL, &local_error, 0);
	    SETDBERR(dberr, 0, E_DM0136_GATEWAY_ERROR);
	    return (E_DB_ERROR);
	}

	xcb = dmf_get_xcb(scb, rcb);
	if (xcb == NULL)
	{
	    uleFormat(NULL, E_DM9A07_GATEWAY_OPEN, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char * )0, (i4)0, (i4 *)NULL, &local_error, 1,
		STlength(SystemProductName), SystemProductName);
	    uleFormat(NULL, E_DM003B_BAD_TRAN_ID, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char * )0, (i4)0, (i4 *)NULL, &local_error, 0);
	    SETDBERR(dberr, 0, E_DM0136_GATEWAY_ERROR);
	    return (E_DB_ERROR);
	}
    }

    gw_rcb.gwr_session_id = scb->scb_gw_session_id;
    gw_rcb.gwr_xact_id = (PTR)xcb;
    gw_rcb.gwr_database_id = (PTR)xcb->xcb_odcb_ptr;
    gw_rcb.gwr_gw_sequence = rcb->rcb_seq_number;

    /*
    ** Allocate arrays to build table, attribute and index information into.
    */
    mem_needed = DB_ALIGN_MACRO(sizeof(DMP_MISC));
    mem_needed += DB_ALIGN_MACRO(sizeof(DMT_TBL_ENTRY));
    mem_needed += tcb->tcb_rel.relatts * DB_ALIGN_MACRO(sizeof(DMT_ATT_ENTRY));
    mem_needed += tcb->tcb_index_count * DB_ALIGN_MACRO(sizeof(DMT_IDX_ENTRY));

    status = dm0m_allocate(mem_needed, (i4)0, (i4)MISC_CB,
	(i4)MISC_ASCII_ID, (char *)NULL, (DM_OBJECT**)&mem_ptr, dberr);
    if (status)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char * )0, (i4)0, (i4 *)NULL, &local_error, 0);
	SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
	return (E_DB_ERROR);
    }
    table_entry =
	(DMT_TBL_ENTRY *)((char *)mem_ptr + DB_ALIGN_MACRO(sizeof(DMP_MISC)));
    mem_ptr->misc_data = (char*)table_entry;

    column_array = (DMT_ATT_ENTRY *)((char *)table_entry +
	DB_ALIGN_MACRO(sizeof(DMT_TBL_ENTRY)));
    /*
    ** 05-feb-04 (chash01)
    **   For unique index in rms gateway, relatts is the number of index table
    **   attributes (which must be the same number of key attributes) plus
    **   1 for TIDP.  Later, when rms gateway allocates memory for its private
    **   use (in gwrms.c), it relies on the fact that last entry of index_array
    **   is for TIDP thus has a 0 in index_array[i].idx_attr_id[ last entry ]
    **   because TIDP is not an attribute in the base table.
    */
    index_array = (DMT_IDX_ENTRY *)((char *)column_array +
	(tcb->tcb_rel.relatts * DB_ALIGN_MACRO(sizeof(DMT_ATT_ENTRY))));

    /*
    ** Fill in table information.
    */
    gw_rcb.gwr_table_entry = table_entry;
    STRUCT_ASSIGN_MACRO(tcb->tcb_rel.relid, table_entry->tbl_name);
    STRUCT_ASSIGN_MACRO(tcb->tcb_rel.relowner, table_entry->tbl_owner);
    STRUCT_ASSIGN_MACRO(tcb->tcb_rel.reltid, table_entry->tbl_id);
    table_entry->tbl_loc_count = 0;
    table_entry->tbl_attr_count = tcb->tcb_rel.relatts;
    table_entry->tbl_index_count = tcb->tcb_index_count;
    table_entry->tbl_width = tcb->tcb_rel.relwid;
    table_entry->tbl_storage_type = tcb->tcb_rel.relspec;
    table_entry->tbl_status_mask = tcb->tcb_rel.relstat;
    table_entry->tbl_create_time = tcb->tcb_rel.relcreate;

    /*
    ** When created, the RCB was primed with the table's current tuple and
    ** page counts, so use those instead of the potentially inaccurate
    ** TCB values.
    */
    table_entry->tbl_record_count = rcb->rcb_reltups;
    table_entry->tbl_page_count   = rcb->rcb_relpages;

    /*
    ** Fill in column list.
    */
    gw_rcb.gwr_attr_array.ptr_address = (PTR)column_array;
    for (i = 0; i < tcb->tcb_rel.relatts; i++)
    {
	attr_ptr = &tcb->tcb_atts_ptr[i + 1];
	STRUCT_ASSIGN_MACRO(attr_ptr->name, column_array[i].att_name);
	column_array[i].att_number = i + 1;
	column_array[i].att_offset = attr_ptr->offset;
	column_array[i].att_type = attr_ptr->type;
	column_array[i].att_width = attr_ptr->length;
	column_array[i].att_prec = attr_ptr->precision;
	column_array[i].att_collID = attr_ptr->collID;
	column_array[i].att_flags = attr_ptr->flag;
	column_array[i].att_key_seq_number = attr_ptr->key;
	column_array[i].att_geomtype = attr_ptr->geomtype;
	column_array[i].att_srid = attr_ptr->srid;
    }

    /*
    ** Fill in index list.
    */
    gw_rcb.gwr_key_array.ptr_address = (PTR)index_array;
    for (i = 0, itcb = tcb->tcb_iq_next;
	 itcb != (DMP_TCB *)&tcb->tcb_iq_next;
	    i++, itcb = itcb->tcb_q_next)
    {
	/* Make sure we are not going to overflow the index array. */
	if (i >= tcb->tcb_index_count)
	    break;

	STRUCT_ASSIGN_MACRO(itcb->tcb_rel.relid, index_array[i].idx_name);
	STRUCT_ASSIGN_MACRO(itcb->tcb_rel.reltid, index_array[i].idx_id);
	index_array[i].idx_dpage_count = 0;
	index_array[i].idx_ipage_count = 0;
	index_array[i].idx_status = 0;
	index_array[i].idx_storage_type = itcb->tcb_rel.relspec;

	/* If not HASH, then discount the TIDP key field. */
	/*
	** NOTE:  we probably always want to discount the tidp field, as the
	** indexes are foreign and so tidp is always irrelevant.  Since RMS
	** doesn't have hash type I'll ignore for now... (linda)
	*/
	index_array[i].idx_key_count = itcb->tcb_rel.relkeys;
        if ( itcb->tcb_rel.relgwid != GW_RMS )
        {
	    if (itcb->tcb_rel.relspec != TCB_HASH)
	        index_array[i].idx_key_count--; 
        }
        /*
        ** 26-aug-03 (chash01): Ingres logic is that if the secondary index
        **   is non-unique, TIDP count stays.  Since it is possible to have
        **   non-unique non-hash secondary index, we need to remove TIDP
        **   count from this kind of index so that RMS can deal with it
        **   uniformally.
        */
	else
        {
           if( !(itcb->tcb_rel.relstat & TCB_UNIQUE) )
	      index_array[i].idx_key_count--; 
        }
	/* Fill in the key map */
	index_array[i].idx_array_count = 0;
	for (j = 0; j < DB_MAXIXATTS; j++)
	{
	    if (index_array[i].idx_attr_id[j] = itcb->tcb_ikey_map[j])
		index_array[i].idx_array_count++;
	}
    }

    gw_rcb.gwr_type = GWR_CB_TYPE;
    gw_rcb.gwr_length = sizeof(GW_RCB);

    switch (rcb->rcb_access_mode)
    {
	case	RCB_A_WRITE:
		    gw_rcb.gwr_access_mode = GWT_WRITE;
		    break;
	case	RCB_A_READ:
		    gw_rcb.gwr_access_mode = GWT_READ;
		    break;
	case	RCB_A_OPEN_NOACCESS:
		    gw_rcb.gwr_access_mode = GWT_OPEN_NOACCESS;
		    break;
	case	RCB_A_MODIFY:
		    gw_rcb.gwr_access_mode = GWT_MODIFY;
		    break;
	default:
		SETDBERR(dberr, 0, E_DM002A_BAD_PARAMETER);
		status = E_DB_ERROR;
		break;
    }

    if (status != E_DB_OK)
	return(status);
	
    /* Cram timeout value into gw_rcb.  Temporary fix (!) (25-jan-91) */

    gw_rcb.gwr_conf_array.data_address = (char *) &rcb->rcb_timeout;
    gw_rcb.gwr_conf_array.data_in_size = sizeof( rcb->rcb_timeout );

    status = gwf_call(GWT_OPEN, &gw_rcb);
    if (status)
    {
	switch (gw_rcb.gwr_error.err_code)
	{
	    case E_GW0503_NONEXISTENT_TABLE:
		SETDBERR(dberr, 0, E_DM0054_NONEXISTENT_TABLE);
		break;

	    case E_GW0504_LOCK_QUOTA_EXCEEDED:
		SETDBERR(dberr, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
		break;

	    case E_GW050E_ALREADY_REPORTED:
		SETDBERR(dberr, 0, E_DM0142_GWF_ALREADY_REPORTED);
		break;

	    case E_GW0600_NO_MEM:
		SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
		break;

	    case E_GW0327_DMF_DEADLOCK:
		SETDBERR(dberr, 0, E_DM0042_DEADLOCK);
		break;

	    default:
		uleFormat(NULL, E_DM9A07_GATEWAY_OPEN, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, (char * )0, (i4)0,
			    (i4 *)NULL, &local_error, 1,
			    STlength(SystemProductName), SystemProductName);
		uleFormat(&gw_rcb.gwr_error, 0, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, (char * )0, (i4)0,
			    (i4 *)NULL, &local_error, 0);

		SETDBERR(dberr, 0, E_DM0136_GATEWAY_ERROR);
		break;
	}
    }

    if (status == E_DB_OK)
    {
	rcb->rcb_gateway_rsb = gw_rcb.gwr_gw_rsb;
	rcb->rcb_tcb_ptr->tcb_rel.relpages = gw_rcb.gwr_page_count;
    }

    /*
    ** Return memory used for attribute array.
    */
    dm0m_deallocate((DM_OBJECT**)&mem_ptr);

    return (status);
}

/*{
** Name: dmf_gwt_close - Close gateway table in the foreign database.
**
** Description:
**	Called at DMT_CLOSE time to close a table in the gateway.
**
**	The table must have been opened earlier by a dmf_gwt_open call.  The
**	gateway RSB from that open call must be set in the rcb.
**
** Inputs:
**	rcb		- record control block used by Ingres server to describe
**			  open instance of the table.  Contains pointer to
**			  table and session control blocks.
**
** Outputs:
**	err_code	- reason for error:
**			E_DM004B_LOCK_QUOTA_EXCEEDED
**			E_DM0112_RESOURCE_QUOTA_EXCEED
**			E_DM0136_GATEWAY_ERROR
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-aug-89 (rogerk)
**          Created for Non-SQL Gateway.
**	9-apr-90 (bryanp)
**	    New GWF interface, using gwf_call().
**	11-jun-1990 (bryanp)
**	    Re-map GWF messages into DMF messages.
**	7-dec-90 (linda)
**	    Add a call to dmf_get_xcb(), so we can get the database id for
**	    the gateway.  Used to identify TCB's in the gateway TCB cache.
**      10-dec-92 (schang)
**          pass in the access_mode for proper testing
**	06-oct-93 (swm)
**	     Bug #56443
**	     Change i4 cast to PTR now that gwr_database_id is a PTR.
*/
DB_STATUS
dmf_gwt_close(
DMP_RCB		    *rcb,
DB_ERROR	    *dberr)
{
    GW_RCB		gw_rcb;
    DMP_TCB		*tcb = rcb->rcb_tcb_ptr;
    DML_SCB		*scb;
    DML_XCB		*xcb;
    DB_STATUS		status;
    i4		local_error;

    CLRDBERR(dberr);

    if (DMZ_TBL_MACRO(20))
    {
	TRdisplay("DMF GATEWAY : Calling GWT_CLOSE for table %.12s\n",
	    tcb->tcb_rel.relid.db_tab_name);
    }

    /*
    ** If the RCB has no XCB ptr then need to get session control block.
    */
    if (rcb->rcb_xcb_ptr == NULL || rcb->rcb_xcb_ptr->xcb_scb_ptr == NULL)
    {
	status = dmf_get_scb(&scb, dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(NULL, E_DM9A08_GATEWAY_CLOSE, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char * )0, (i4)0, (i4 *)NULL, &local_error, 1,
		STlength(SystemProductName), SystemProductName);
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char * )0, (i4)0, (i4 *)NULL, &local_error, 0);
	    SETDBERR(dberr, 0, E_DM0136_GATEWAY_ERROR);
	    return (E_DB_ERROR);
	}

	xcb = dmf_get_xcb(scb, rcb);
	if (xcb == NULL)
	{
	    uleFormat(NULL, E_DM9A08_GATEWAY_CLOSE, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char * )0, (i4)0, (i4 *)NULL, &local_error, 1,
		STlength(SystemProductName), SystemProductName);
	    uleFormat(NULL, E_DM003B_BAD_TRAN_ID, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char * )0, (i4)0, (i4 *)NULL, &local_error, 0);
	    SETDBERR(dberr, 0, E_DM0136_GATEWAY_ERROR);
	    return (E_DB_ERROR);
	}
    }
    else
    {
	scb = rcb->rcb_xcb_ptr->xcb_scb_ptr;
	xcb = rcb->rcb_xcb_ptr;
    }


    status = E_DB_OK;
    switch (rcb->rcb_access_mode)
    {
        case	RCB_A_WRITE:
                    gw_rcb.gwr_access_mode = GWT_WRITE;
                    break;
        case	RCB_A_READ:
                    gw_rcb.gwr_access_mode = GWT_READ;
                    break;
        case	RCB_A_OPEN_NOACCESS:
                    gw_rcb.gwr_access_mode = GWT_OPEN_NOACCESS;
                    break;
        case	RCB_A_MODIFY:
                    gw_rcb.gwr_access_mode = GWT_MODIFY;
                    break;
        default:
		    SETDBERR(dberr, 0, E_DM002A_BAD_PARAMETER);
                    status = E_DB_ERROR;
                    break;
    }

    if (status != E_DB_OK)
        return(status);

    gw_rcb.gwr_type = GWR_CB_TYPE;
    gw_rcb.gwr_length = sizeof(GW_RCB);

    gw_rcb.gwr_gw_id = tcb->tcb_parent_tcb_ptr->tcb_rel.relgwid;
    gw_rcb.gwr_session_id = scb->scb_gw_session_id;
    gw_rcb.gwr_gw_rsb = rcb->rcb_gateway_rsb;
    STRUCT_ASSIGN_MACRO(tcb->tcb_rel.reltid, gw_rcb.gwr_tab_id );
    gw_rcb.gwr_gw_sequence = rcb->rcb_seq_number;
    gw_rcb.gwr_xact_id = (PTR)xcb;
    gw_rcb.gwr_database_id = (PTR)xcb->xcb_odcb_ptr;

    status = gwf_call(GWT_CLOSE, &gw_rcb);
    if (status)
    {
	uleFormat(NULL, E_DM9A08_GATEWAY_CLOSE, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char * )0, (i4)0, (i4 *)NULL, &local_error, 1,
	    STlength(SystemProductName), SystemProductName);
	uleFormat(&gw_rcb.gwr_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char * )0, (i4)0, (i4 *)NULL, &local_error, 0);

	switch(gw_rcb.gwr_error.err_code)
	{
	    case E_GW0504_LOCK_QUOTA_EXCEEDED:
		SETDBERR(dberr, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
		break;

	    case E_GW0600_NO_MEM:
		SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
		break;

	    case E_GW0327_DMF_DEADLOCK:
		SETDBERR(dberr, 0, E_DM0042_DEADLOCK);
		break;

	    default:
		SETDBERR(dberr, 0, E_DM0136_GATEWAY_ERROR);
		break;
	}
    }

    rcb->rcb_gateway_rsb = NULL;
    return (status);
}

/*{
** Name: dmf_gwr_position - position gateway table.
**
** Description:
**	Called at DMR_POSITION time to position a table in the gateway.
**
**	The table must have been opened earlier by a dmf_gwt_open call.  The
**	gateway RSB from that open call must be set in the rcb.
**
**	Note, when the optimizer chooses a gateway secondary, the resulting
**	node implements a so-called "restricted tid join" to accomplish this.
**	This means among other things that the position is called only for the
**	secondary, with keys appropriate to that secondary.  No position will
**	be called on the base table (although an open was issued for both the
**	base and the secondary during qeq_validate() processing).
**
** Inputs:
**	rcb		- record control block used by Ingres server to
**			  describe open instance of the table.  Contains
**			  pointer to table and session control blocks.
**
** Outputs:
**	err_code	- reason for error:
**			E_DM0042_DEADLOCK
**			E_DM004B_LOCK_QUOTA_EXCEEDED
**			E_DM004D_LOCK_TIMER_EXPIRED
**			E_DM0055_NONEXT
**			E_DM0112_RESOURCE_QUOTA_EXCEED
**			E_DM0136_GATEWAY_ERROR
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-aug-89 (rogerk)
**          Created for Non-SQL Gateway.
**	9-apr-90 (bryanp)
**	    New GWF interface, using gwf_call().
**	11-jun-1990 (bryanp)
**	    Re-map GWF errors to DMF errors.
**      06-nov-92 (schang)
**          Pass into gateway the access mode for proper testing.
**	12-Apr-2008 (kschendel)
**	    Update the qualification stuff passed to gwf to match
**	    the expanded DMF mechanism.
*/
DB_STATUS
dmf_gwr_position(
DMP_RCB		    *rcb,
DB_ERROR	    *dberr)
{
    GW_RCB		gw_rcb;
    DMP_TCB		*tcb = rcb->rcb_tcb_ptr;
    DML_SCB		*scb = rcb->rcb_xcb_ptr->xcb_scb_ptr;
    DB_STATUS		status;
    i4		local_error;

    CLRDBERR(dberr);

    if (DMZ_TBL_MACRO(20))
    {
	TRdisplay("DMF GATEWAY : Calling GWT_POSITION for table %.12s\n",
	    tcb->tcb_rel.relid.db_tab_name);
    }

    STRUCT_ASSIGN_MACRO(tcb->tcb_rel.reltid, gw_rcb.gwr_tab_id);

    gw_rcb.gwr_type = GWR_CB_TYPE;
    gw_rcb.gwr_length = sizeof(GW_RCB);

    gw_rcb.gwr_gw_id = tcb->tcb_rel.relgwid;
    gw_rcb.gwr_session_id = scb->scb_gw_session_id;
    gw_rcb.gwr_gw_rsb = rcb->rcb_gateway_rsb;

    gw_rcb.gwr_in_vdata1.data_address = NULL;
    gw_rcb.gwr_in_vdata2.data_address = NULL;
    gw_rcb.gwr_in_vdata1.data_in_size = 0;
    gw_rcb.gwr_in_vdata2.data_in_size = 0;
    if (rcb->rcb_ll_given)
    {
	gw_rcb.gwr_in_vdata1.data_address = rcb->rcb_ll_ptr;
	/*
	** NOTE:  rather than assigning tcb->tcb_klen, which is the key length
	** for a whole key, just supply the number of keys given and let the
	** gateway exits compute the length while converting each key part to
	** its corresponding gateway type.  24-jul-90 (linda)
	*/

	/*	gw_rcb.gwr_in_vdata1.data_in_size = tcb->tcb_klen;	*/

	gw_rcb.gwr_in_vdata1.data_in_size = rcb->rcb_ll_given;
    }
    if (rcb->rcb_hl_given)
    {
	gw_rcb.gwr_in_vdata2.data_address = rcb->rcb_hl_ptr;

	/*	gw_rcb.gwr_in_vdata2.data_in_size = tcb->tcb_klen;	*/

	gw_rcb.gwr_in_vdata2.data_in_size = rcb->rcb_hl_given;
    }

    /*
    ** Here we supply the key attributes, for use by gateway exits when
    ** decoding the key.  24-jul-90 (linda)
    **
    ****************************************************************************
    **	The tcb_att_key_ptr has values corresponding to the attributes
    **	described for the secondary index.  Must account for this fact when
    **	translating keys and qualifying records.  Do we need to save a pointer
    **	to the secondary index tcb?  we still have it at this level, what does
    **	the gateway do?  we could translate here.
    ****************************************************************************
    */

    gw_rcb.gwr_in_vdata_lst.ptr_address = (PTR)tcb->tcb_att_key_ptr;
    gw_rcb.gwr_in_vdata_lst.ptr_in_count = tcb->tcb_keys;

    gw_rcb.gwr_qual_func = rcb->rcb_f_qual;
    gw_rcb.gwr_qual_arg = rcb->rcb_f_arg;
    gw_rcb.gwr_qual_rowaddr = rcb->rcb_f_rowaddr;
    gw_rcb.gwr_qual_retval = rcb->rcb_f_retval;

    /*
    ** If this is a secondary index, then switch tcb's here.  NOTE that we must
    ** do this after the assignments done above to determine key values.
    */

    if (tcb->tcb_rel.relstat & TCB_INDEX)
	tcb = tcb->tcb_parent_tcb_ptr;

    gw_rcb.gwr_gw_id = tcb->tcb_parent_tcb_ptr->tcb_rel.relgwid;
    STRUCT_ASSIGN_MACRO(tcb->tcb_rel.relid, gw_rcb.gwr_tab_name);
    STRUCT_ASSIGN_MACRO(tcb->tcb_rel.relowner, gw_rcb.gwr_tab_owner);

    status = E_DB_OK;
    switch (rcb->rcb_access_mode)
    {
        case	RCB_A_WRITE:
                    gw_rcb.gwr_access_mode = GWT_WRITE;
                    break;
        case	RCB_A_READ:
                    gw_rcb.gwr_access_mode = GWT_READ;
                    break;
        case	RCB_A_OPEN_NOACCESS:
                    gw_rcb.gwr_access_mode = GWT_OPEN_NOACCESS;
                    break;
        case	RCB_A_MODIFY:
                    gw_rcb.gwr_access_mode = GWT_MODIFY;
                    break;
        default:
		    SETDBERR(dberr, 0, E_DM002A_BAD_PARAMETER);
                    status = E_DB_ERROR;
                    break;
    }

    if (status != E_DB_OK)
        return(status);

    status = gwf_call(GWR_POSITION, &gw_rcb);
    if (status)
    {
	switch(gw_rcb.gwr_error.err_code)
	{
	    /* possible FIXME, we do have DM0142 but it takes some goofy
	    ** path thru QEF that I haven't untangled.  DM0023 turns into
	    ** QE0025 which is what QEF does for adf already-reported's.
	    ** It may be possible to consolidate these.
	    ** (also see gwr-get return handling.)
	    */
	    case E_GW0023_USER_ERROR:
		SETDBERR(dberr, 0, E_DM0023_USER_ERROR);
		break;

	    case E_GW0504_LOCK_QUOTA_EXCEEDED:
		SETDBERR(dberr, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
		break;

	    case E_GW0508_DEADLOCK:
		SETDBERR(dberr, 0, E_DM0042_DEADLOCK);
		break;

	    case E_GW0509_LOCK_TIMER_EXPIRED:
		SETDBERR(dberr, 0, E_DM004D_LOCK_TIMER_EXPIRED);
		break;

	    case E_GW0600_NO_MEM:
		SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
		break;

	    case E_GW0641_END_OF_STREAM:
		SETDBERR(dberr, 0, E_DM0055_NONEXT);
		break;

	    default:
		uleFormat(NULL, E_DM9A09_GATEWAY_POSITION, (CL_ERR_DESC *)NULL, ULE_LOG,
			   NULL, (char * )0, (i4)0, (i4 *)NULL,
			   &local_error, 1, STlength (SystemProductName),
			   SystemProductName);
		uleFormat(&gw_rcb.gwr_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
			   NULL, (char * )0, (i4)0, (i4 *)NULL,
			   &local_error, 0);
		SETDBERR(dberr, 0, E_DM0136_GATEWAY_ERROR);
		break;
	}
    }

    return (status);
}

/*{
** Name: dmf_gwr_get - get record from gateway table.
**
** Description:
**	Called at DMR_GET time to get a record from a table in the gateway.
**
**	The table must have been opened earlier by a dmf_gwt_open call.  The
**	gateway RSB from that open call must be set in the rcb.
**
**	The table must also have been positioned by a call to dmf_gwr_position.
**
** Inputs:
**	rcb		- record control block used by Ingres server to describe
**			  open instance of the table.  Contains pointer to
**			  table and session control blocks.
**	record		- buffer to write record into.
**	flag		- get option:
**			    DM1C_GETNEXT : Get next record from current
**					   query position.
**
** Outputs:
**	tid		- tuple identifier of the record returned.
**	err_code	- reason for error:
**			E_DM0042_DEADLOCK
**			E_DM004B_LOCK_QUOTA_EXCEEDED
**			E_DM004D_LOCK_TIMER_EXPIRED
**			E_DM0055_NONEXT
**			E_DM0112_RESOURCE_QUOTA_EXCEED
**			E_DM0136_GATEWAY_ERROR
**			E_DM0141_GWF_DATA_CVT_ERROR
**			E_DM0142_GWF_ALREADY_REPORTED
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-aug-89 (rogerk)
**          Created for Non-SQL Gateway.
**	9-apr-90 (bryanp)
**	    New GWF interface, using gwf_call().
**	11-jun-1990 (bryanp)
**	    Re-map E_GW050D_DATA_CVT_ERROR to E_DM0141_GWF_DATA_CVT_ERROR.
**	18-jun-1990 (linda, bryanp)
**	    Re-map E_GW050E_ALREADY_REPORTED to E_DM0142_GWF_ALREADY_REPORTED.
**	24-jan-91 (linda)
**	    Pass down the information that we are performing a restricted tid
**	    join if that is the case.
**      26-may-92 (schang)
**        GW merge.
**	  18-apr-91 (rickh)
**	    Check for user interrupt (control-C).
**	  30-apr-1991 (rickh)
**	    Removed tidjoin logic added on 24-jan-91.
**	  14-jun-1991 (rickh)
**	    Changed name of GWR_BYPOS to GWR_GETNEXT since that's how it's
**	    used.
**	  22-jul-1991 (rickh)
**	   Moved up the logic that sets the width of the input buffer.  This
**	   is actually set at OPF time.  We want this buffer to be the right
**	   size for secondary indices.
**      06-nov-92 (schang)
**          Pass into gateway the access mode for proper testing.
**      26-apr-93 (schang)
**          convert gwf_user_intr to dmf_user_intr
**	27-dec-06 (kibro01) b117076
**	    Only log error message if there is one to log
*/
DB_STATUS
dmf_gwr_get(
DMP_RCB		    *rcb,
DM_TID		    *tid,
char		    *record,
i4		    flag,
DB_ERROR	    *dberr)
{
    GW_RCB		gw_rcb;
    DMP_TCB		*tcb = rcb->rcb_tcb_ptr;
    DML_SCB		*scb = rcb->rcb_xcb_ptr->xcb_scb_ptr;
    DMR_CHAR_ENTRY	gwr_char_entry;
    DB_STATUS		status;
    i4		local_error;

    CLRDBERR(dberr);

    if (DMZ_TBL_MACRO(20))
    {
	TRdisplay("DMF GATEWAY : Calling GWR_GET for table %.12s\n",
	    tcb->tcb_rel.relid.db_tab_name);
    }

    STRUCT_ASSIGN_MACRO(tcb->tcb_rel.reltid, gw_rcb.gwr_tab_id);

    gw_rcb.gwr_type = GWR_CB_TYPE;
    gw_rcb.gwr_length = sizeof(GW_RCB);

    gw_rcb.gwr_session_id = scb->scb_gw_session_id;
    gw_rcb.gwr_gw_rsb = rcb->rcb_gateway_rsb;

    /*
    ** Describe record buffer to write record into.  If this is a secondary
    ** index, then the buffer passed by QEF is only big enough for the keys
    ** and the tid.
    */
    gw_rcb.gwr_out_vdata1.data_address = record;
    gw_rcb.gwr_out_vdata1.data_in_size = tcb->tcb_rel.relwid;

    gw_rcb.gwr_char_array.data_address = 0;
    gw_rcb.gwr_char_array.data_in_size = 0;

    /* now we can reset the tcb for indices */

    if (tcb->tcb_rel.relstat & TCB_INDEX)
	tcb = tcb->tcb_parent_tcb_ptr;

    gw_rcb.gwr_gw_id = tcb->tcb_parent_tcb_ptr->tcb_rel.relgwid;
    STRUCT_ASSIGN_MACRO(tcb->tcb_rel.relid, gw_rcb.gwr_tab_name);
    STRUCT_ASSIGN_MACRO(tcb->tcb_rel.relowner, gw_rcb.gwr_tab_owner);

    /*
    ** Set flag indicating whether this is a regular get by position, or by
    ** "tid" (record id).
    */
    if (flag & DM1C_BYTID)
    {
	gw_rcb.gwr_flags = GWR_BYTID;
	/* Pass down the tid value. */
	gw_rcb.gwr_tid.tid_i4 = tid->tid_i4;
    }
    else
    {
	gw_rcb.gwr_flags = GWR_GETNEXT;
    }

    status = E_DB_OK;
    switch (rcb->rcb_access_mode)
    {
        case	RCB_A_WRITE:
                    gw_rcb.gwr_access_mode = GWT_WRITE;
                    break;
        case	RCB_A_READ:
                    gw_rcb.gwr_access_mode = GWT_READ;
                    break;
        case	RCB_A_OPEN_NOACCESS:
                    gw_rcb.gwr_access_mode = GWT_OPEN_NOACCESS;
                    break;
        case	RCB_A_MODIFY:
                    gw_rcb.gwr_access_mode = GWT_MODIFY;
                    break;
        default:
		    SETDBERR(dberr, 0, E_DM002A_BAD_PARAMETER);
                    status = E_DB_ERROR;
                    break;
    }

    if (status != E_DB_OK)
        return(status);

    status = gwf_call(GWR_GET, &gw_rcb);
    if (status)
    {
	switch(gw_rcb.gwr_error.err_code)
	{
	    /* Possible FIXME, see gwr-position return */
	    case E_GW0023_USER_ERROR:
		SETDBERR(dberr, 0, E_DM0023_USER_ERROR);
		/* Give iierrornumber info to calling RCB if we're in a
		** ordinary dmr-get.  (DMF logical layer sets this error
		** pointer to the DMR cb for returning stuff to QEF.)
		*/
		if (scb->scb_qfun_errptr != NULL)
		    *scb->scb_qfun_errptr = gw_rcb.gwr_error;
		break;

	    case E_GW0504_LOCK_QUOTA_EXCEEDED:
		SETDBERR(dberr, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
		break;

	    case E_GW0508_DEADLOCK:
		SETDBERR(dberr, 0, E_DM0042_DEADLOCK);
		break;

	    case E_GW0509_LOCK_TIMER_EXPIRED:
		SETDBERR(dberr, 0, E_DM004D_LOCK_TIMER_EXPIRED);
		break;

	    case E_GW050D_DATA_CVT_ERROR:
		SETDBERR(dberr, 0, E_DM0141_GWF_DATA_CVT_ERROR);
		break;

	    case E_GW050E_ALREADY_REPORTED:
		SETDBERR(dberr, 0, E_DM0142_GWF_ALREADY_REPORTED);
		break;

	    case E_GW0600_NO_MEM:
		SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
		break;

	    case E_GW0641_END_OF_STREAM:
		SETDBERR(dberr, 0, E_DM0055_NONEXT);
		status = E_DB_ERROR;
		break;

            case E_GW0341_USER_INTR:
		SETDBERR(dberr, 0, E_DM0065_USER_INTR);
                break;

	    default:
		uleFormat(NULL, E_DM9A0A_GATEWAY_GET, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, (char * )0, (i4)0,
			    (i4 *)NULL, &local_error, 1,
			    STlength (SystemProductName), SystemProductName);
		if (gw_rcb.gwr_error.err_code)
			uleFormat(&gw_rcb.gwr_error, 0, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, (char * )0, (i4)0,
			    (i4 *)NULL, &local_error, 0);
		SETDBERR(dberr, 0, E_DM0136_GATEWAY_ERROR);
		break;
	}
    }

    /*
    ** Return tid of selected row.
    ** Gateways do not currently return tid values.
    */
    tid->tid_i4 = gw_rcb.gwr_tid.tid_i4;

	checkForInterrupt( rcb, &status, dberr );
    return (status);
}

/*{
** Name: dmf_gwr_put - append record to gateway table.
**
** Description:
**	Called at DMR_PUT time to insert a record into a table in the gateway.
**
**	The table must have been opened earlier by a dmf_gwt_open call.  The
**	gateway RSB from that open call must be set in the rcb.
**
** Inputs:
**	rcb		- record control block used by Ingres server to describe
**			  open instance of the table.  Contains pointer to
**			  table and session control blocks.
**	record		- record to insert.
**	record_size	- size of record.
**
** Outputs:
**	tid		- tuple identifier of the record inserted.
**	err_code	- reason for error:
**			E_DM0045_DUPLICATE_KEY
**			E_DM0112_RESOURCE_QUOTA_EXCEED
**			E_DM0136_GATEWAY_ERROR
**			E_DM0141_GWF_DATA_CVT_ERROR
**			E_DM0142_GWF_ALREADY_REPORTED
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-aug-89 (rogerk)
**          Created for Non-SQL Gateway.
**	9-apr-90 (bryanp)
**	    New GWF interface, using gwf_call().
**	11-jun-1990 (bryanp)
**	    Re-map E_GW050D_DATA_CVT_ERROR to E_DM0141_GWF_DATA_CVT_ERROR.
**	    Re-map E_GW050E_ALREADY_REPORTED to E_DM0142_GWF_ALREADY_REPORTED
**      26-may-1992 (schang)
**          GW merge
**	    18-apr-1991 (rickh)
**		Check for user interrupt (control-C).
**      06-nov-92 (schang)
**          Pass into gateway the access mode for proper testing.
*/
DB_STATUS
dmf_gwr_put(
DMP_RCB		    *rcb,
char		    *record,
i4		    record_size,
DM_TID		    *tid,
DB_ERROR	    *dberr)
{
    GW_RCB		gw_rcb;
    DMP_TCB		*tcb = rcb->rcb_tcb_ptr;
    DML_SCB		*scb = rcb->rcb_xcb_ptr->xcb_scb_ptr;
    DB_STATUS		status;
    i4		local_error;

    CLRDBERR(dberr);

    if (DMZ_TBL_MACRO(20))
    {
	TRdisplay("DMF GATEWAY : Calling GWR_PUT for table %.12s\n",
	    tcb->tcb_rel.relid.db_tab_name);
    }

    STRUCT_ASSIGN_MACRO(tcb->tcb_rel.reltid, gw_rcb.gwr_tab_id);

    gw_rcb.gwr_type = GWR_CB_TYPE;
    gw_rcb.gwr_length = sizeof(GW_RCB);

    gw_rcb.gwr_gw_id = tcb->tcb_rel.relgwid;
    gw_rcb.gwr_session_id = scb->scb_gw_session_id;
    gw_rcb.gwr_gw_rsb = rcb->rcb_gateway_rsb;

    if (tcb->tcb_rel.relstat & TCB_INDEX)
	tcb = tcb->tcb_parent_tcb_ptr;

    gw_rcb.gwr_gw_id = tcb->tcb_rel.relgwid;
    STRUCT_ASSIGN_MACRO(tcb->tcb_rel.relid, gw_rcb.gwr_tab_name);
    STRUCT_ASSIGN_MACRO(tcb->tcb_rel.relowner, gw_rcb.gwr_tab_owner);

    /*
    ** Describe record buffer.
    */
    gw_rcb.gwr_in_vdata1.data_address = record;
    gw_rcb.gwr_in_vdata1.data_in_size = record_size;

    status = E_DB_OK;
    switch (rcb->rcb_access_mode)
    {
        case	RCB_A_WRITE:
                    gw_rcb.gwr_access_mode = GWT_WRITE;
                    break;
        case	RCB_A_READ:
                    gw_rcb.gwr_access_mode = GWT_READ;
                    break;
        case	RCB_A_OPEN_NOACCESS:
                    gw_rcb.gwr_access_mode = GWT_OPEN_NOACCESS;
                    break;
        case	RCB_A_MODIFY:
                    gw_rcb.gwr_access_mode = GWT_MODIFY;
                    break;
        default:
		    SETDBERR(dberr, 0, E_DM002A_BAD_PARAMETER);
                    status = E_DB_ERROR;
                    break;
    }

    if (status != E_DB_OK)
        return(status);

    status = gwf_call(GWR_PUT, &gw_rcb);
    if (status)
    {
	switch (gw_rcb.gwr_error.err_code)
	{
	    case E_GW050B_DUPLICATE_KEY:
		SETDBERR(dberr, 0, E_DM0045_DUPLICATE_KEY);
		break;

	    case E_GW050D_DATA_CVT_ERROR:
		SETDBERR(dberr, 0, E_DM0141_GWF_DATA_CVT_ERROR);
		break;

	    case E_GW050E_ALREADY_REPORTED:
		SETDBERR(dberr, 0, E_DM0142_GWF_ALREADY_REPORTED);
		break;

	    case E_GW0600_NO_MEM:
	    case E_GW050A_RESOURCE_QUOTA_EXCEED:
		SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
		break;

	    default:

		uleFormat(NULL, E_DM9A0B_GATEWAY_PUT, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, (char * )0, (i4)0,
			    (i4 *)NULL, &local_error, 1,
			    STlength(SystemProductName), SystemProductName);
		uleFormat(&gw_rcb.gwr_error, 0, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, (char * )0, (i4)0,
			    (i4 *)NULL, &local_error, 0);
		SETDBERR(dberr, 0, E_DM0136_GATEWAY_ERROR);
		break;
	}
    }

    /*
    ** Return tid of appended row.  Gateways do not currently return tid
    ** values.
    */
    tid->tid_i4 = 0;

	checkForInterrupt( rcb, &status, dberr );
    return (status);
}

/*{
** Name: dmf_gwr_replace - replace record in gateway table.
**
** Description:
**	Called at DMR_REPLACE time to update a record in a gateway table.
**
**	The table must have been opened earlier by a dmf_gwt_open call.  The
**	gateway RSB from that open call must be set in the rcb.
**
**	The table must also have been positioned by a call to dmf_gwr_position.
**	The record updated will be the one at which the table is currently
**	positioned.
**
** Inputs:
**	rcb		- record control block used by Ingres server to describe
**			  open instance of the table.  Contains pointer to
**			  table and session control blocks.
**	oldrecord	- old record which is being updated.
**	oldlength	- length of old record.
**	newrecord	- new record to write to table.
**	newlength	- length of new record.
**	oldtid		- tuple identifier of record being updated.
**
** Outputs:
**	newtid		- tuple identifier of the updated record.
**	err_code	- reason for error:
**			E_DM0045_DUPLICATE_KEY
**			E_DM0074_NOT_POSITIONED
**			E_DM0112_RESOURCE_QUOTA_EXCEED
**			E_DM0136_GATEWAY_ERROR
**			E_DM0141_GWF_DATA_CVT_ERROR
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-aug-89 (rogerk)
**          Created for Non-SQL Gateway.
**	9-apr-90 (bryanp)
**	    New GWF interface, using gwf_call().
**	11-jun-1990 (bryanp)
**	    Re-map E_GW050D_DATA_CVT_ERROR to E_DM0141_GWF_DATA_CVT_ERROR.
**      26-may-1992 (schang)
**          GW merge.
**	    18-apr-1991 (rickh)
**		Check for user interrupt (control-C).
**      06-nov-92 (schang)
**          Pass into gateway the access mode for proper testing.
*/
DB_STATUS
dmf_gwr_replace(
DMP_RCB		    *rcb,
char		    *oldrecord,
i4		    oldlength,
char		    *newrecord,
i4		    newlength,
DM_TID		    *oldtid,
DM_TID		    *newtid,
DB_ERROR	    *dberr)
{
    GW_RCB		gw_rcb;
    DMP_TCB		*tcb = rcb->rcb_tcb_ptr;
    DML_SCB		*scb = rcb->rcb_xcb_ptr->xcb_scb_ptr;
    DB_STATUS		status;
    i4		local_error;

    CLRDBERR(dberr);

    if (DMZ_TBL_MACRO(20))
    {
	TRdisplay("DMF GATEWAY : Calling GWR_REPLACE for table %.12s\n",
	    tcb->tcb_rel.relid.db_tab_name);
    }

    STRUCT_ASSIGN_MACRO(tcb->tcb_rel.reltid, gw_rcb.gwr_tab_id);

    gw_rcb.gwr_type = GWR_CB_TYPE;
    gw_rcb.gwr_length = sizeof(GW_RCB);

    gw_rcb.gwr_gw_id = tcb->tcb_rel.relgwid;
    gw_rcb.gwr_session_id = scb->scb_gw_session_id;
    gw_rcb.gwr_gw_rsb = rcb->rcb_gateway_rsb;

    /*
    ** Describe record buffers with old and new tuples.
    */
    gw_rcb.gwr_in_vdata1.data_address = newrecord;
    gw_rcb.gwr_in_vdata1.data_in_size = newlength;
    gw_rcb.gwr_in_vdata2.data_address = oldrecord;
    gw_rcb.gwr_in_vdata2.data_in_size = oldlength;

    if (tcb->tcb_rel.relstat & TCB_INDEX)
	tcb = tcb->tcb_parent_tcb_ptr;

    gw_rcb.gwr_gw_id = tcb->tcb_rel.relgwid;
    STRUCT_ASSIGN_MACRO(tcb->tcb_rel.relid, gw_rcb.gwr_tab_name);
    STRUCT_ASSIGN_MACRO(tcb->tcb_rel.relowner, gw_rcb.gwr_tab_owner);

    status = E_DB_OK;
    switch (rcb->rcb_access_mode)
    {
        case	RCB_A_WRITE:
                    gw_rcb.gwr_access_mode = GWT_WRITE;
                    break;
        case	RCB_A_READ:
                    gw_rcb.gwr_access_mode = GWT_READ;
                    break;
        case	RCB_A_OPEN_NOACCESS:
                    gw_rcb.gwr_access_mode = GWT_OPEN_NOACCESS;
                    break;
        case	RCB_A_MODIFY:
                    gw_rcb.gwr_access_mode = GWT_MODIFY;
                    break;
        default:
		    SETDBERR(dberr, 0, E_DM002A_BAD_PARAMETER);
                    status = E_DB_ERROR;
                    break;
    }

    if (status != E_DB_OK)
        return(status);

    status = gwf_call(GWR_REPLACE, &gw_rcb);
    if (status)
    {
	switch (gw_rcb.gwr_error.err_code)
	{
	    case E_GW050B_DUPLICATE_KEY:
		SETDBERR(dberr, 0, E_DM0045_DUPLICATE_KEY);
		break;

	    case E_GW050C_NOT_POSITIONED:
		SETDBERR(dberr, 0, E_DM0074_NOT_POSITIONED);
		break;

	    case E_GW050D_DATA_CVT_ERROR:
		SETDBERR(dberr, 0, E_DM0141_GWF_DATA_CVT_ERROR);
		break;

	    case E_GW050E_ALREADY_REPORTED:
		SETDBERR(dberr, 0, E_DM0142_GWF_ALREADY_REPORTED);
		break;

	    case E_GW0600_NO_MEM:
		SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
		break;

	    default:

		uleFormat(NULL, E_DM9A0C_GATEWAY_REPLACE, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, (char * )0, (i4)0,
			    (i4 *)NULL, &local_error, 1,
			    STlength(SystemProductName), SystemProductName);
		uleFormat(&gw_rcb.gwr_error, 0, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, (char * )0, (i4)0,
			    (i4 *)NULL, &local_error, 0);
		SETDBERR(dberr, 0, E_DM0136_GATEWAY_ERROR);
		break;
	}
    }

    /*
    ** Return tuple identifier of new tuple.  Gateways do not currently return
    ** tid values.
    */
    newtid->tid_i4 = 0;

	checkForInterrupt( rcb, &status, dberr );
    return (status);
}

/*{
** Name: dmf_gwr_delete - delete record from gateway table.
**
** Description:
**	Called at DMR_DELETE time to remove a record from a gateway table.
**
**	The table must have been opened earlier by a dmf_gwt_open call.  The
**	gateway RSB from that open call must be set in the rcb.
**
**	The table must also have been positioned by a call to dmf_gwr_position.
**	The record removed will be the one at the current position.
**
** Inputs:
**	rcb		- record control block used by Ingres server to describe
**			  open instance of the table.  Contains pointer to
**			  table and session control blocks.
**	tid		- tuple identifier of the record to delete.
**
** Outputs:
**	err_code	- reason for error:
**			E_DM0074_NOT_POSITIONED
**			E_DM0112_RESOURCE_QUOTA_EXCEED
**			E_DM0136_GATEWAY_ERROR
**			E_DM0142_GWF_ALREADY_REPORTED
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-aug-89 (rogerk)
**          Created for Non-SQL Gateway.
**	9-apr-90 (bryanp)
**	    New GWF interface, using gwf_call().
**	11-jun-1990 (bryanp)
**	    Re-map GWR errors into DMF errors.
**      26-may-1992 (schang)
**	    18-apr-1991 (rickh)
**		Check for user interrupt (control-C).
**      06-nov-92 (schang)
**          Pass into gateway the access mode for proper testing.
*/
DB_STATUS
dmf_gwr_delete(
DMP_RCB		    *rcb,
DM_TID		    *tid,
DB_ERROR	    *dberr)
{
    GW_RCB		gw_rcb;
    DMP_TCB		*tcb = rcb->rcb_tcb_ptr;
    DML_SCB		*scb = rcb->rcb_xcb_ptr->xcb_scb_ptr;
    DB_STATUS		status;
    i4		local_error;

    CLRDBERR(dberr);

    if (DMZ_TBL_MACRO(20))
    {
	TRdisplay("DMF GATEWAY : Calling GWR_DELETE for table %.12s\n",
	    tcb->tcb_rel.relid.db_tab_name);
    }

    STRUCT_ASSIGN_MACRO(tcb->tcb_rel.reltid, gw_rcb.gwr_tab_id);

    gw_rcb.gwr_type = GWR_CB_TYPE;
    gw_rcb.gwr_length = sizeof(GW_RCB);

    gw_rcb.gwr_gw_id = tcb->tcb_rel.relgwid;
    gw_rcb.gwr_session_id = scb->scb_gw_session_id;
    gw_rcb.gwr_gw_rsb = rcb->rcb_gateway_rsb;

    if (tcb->tcb_rel.relstat & TCB_INDEX)
	tcb = tcb->tcb_parent_tcb_ptr;

    gw_rcb.gwr_gw_id = tcb->tcb_rel.relgwid;
    STRUCT_ASSIGN_MACRO(tcb->tcb_rel.relid, gw_rcb.gwr_tab_name);
    STRUCT_ASSIGN_MACRO(tcb->tcb_rel.relowner, gw_rcb.gwr_tab_owner);

    status = E_DB_OK;
    switch (rcb->rcb_access_mode)
    {
        case	RCB_A_WRITE:
                    gw_rcb.gwr_access_mode = GWT_WRITE;
                    break;
        case	RCB_A_READ:
                    gw_rcb.gwr_access_mode = GWT_READ;
                    break;
        case	RCB_A_OPEN_NOACCESS:
                    gw_rcb.gwr_access_mode = GWT_OPEN_NOACCESS;
                    break;
        case	RCB_A_MODIFY:
                    gw_rcb.gwr_access_mode = GWT_MODIFY;
                    break;
        default:
		    SETDBERR(dberr, 0, E_DM002A_BAD_PARAMETER);
                    status = E_DB_ERROR;
                    break;
    }

    if (status != E_DB_OK)
        return(status);

    status = gwf_call(GWR_DELETE, &gw_rcb);
    if (status)
    {
	switch(gw_rcb.gwr_error.err_code)
	{
	    case E_GW050C_NOT_POSITIONED:
		SETDBERR(dberr, 0, E_DM0074_NOT_POSITIONED);
		break;

	    case E_GW050E_ALREADY_REPORTED:
		SETDBERR(dberr, 0, E_DM0142_GWF_ALREADY_REPORTED);
		break;

	    case E_GW0600_NO_MEM:
		SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
		break;

	    default:
		uleFormat(NULL, E_DM9A0D_GATEWAY_DELETE, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char * )0, (i4)0, (i4 *)NULL, &local_error,
		    1, STlength(SystemProductName), SystemProductName);
		uleFormat(&gw_rcb.gwr_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char * )0, (i4)0, (i4 *)NULL, &local_error,
		    0);

		SETDBERR(dberr, 0, E_DM0136_GATEWAY_ERROR);
		break;
	}
    }

	checkForInterrupt( rcb, &status, dberr );
    return (status);
}

/*{
** Name: dmf_gwx_begin - notify gateway of begin transaction.
**
** Description:
**	Called at DMX_BEGIN time to notify the gateway of a new transaction.
**
** Inputs:
**	scb		- session control block.  Holds gateway session id.
**
** Outputs:
**	err_code	- reason for error:
**			E_DM0112_RESOURCE_QUOTA_EXCEED
**			E_DM0136_GATEWAY_ERROR
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-aug-89 (rogerk)
**          Created for Non-SQL Gateway.
**	9-apr-90 (bryanp)
**	    New GWF interface, using gwf_call().
*/
DB_STATUS
dmf_gwx_begin(
DML_SCB		    *scb,
DB_ERROR	    *dberr)
{
    GW_RCB		gw_rcb;
    DB_STATUS		status;
    i4		local_error;

    CLRDBERR(dberr);

    if (DMZ_TBL_MACRO(20))
    {
	TRdisplay("DMF GATEWAY : Calling GWX_BEGIN for session %d\n",
	    scb->scb_gw_session_id);
    }

    gw_rcb.gwr_type = GWR_CB_TYPE;
    gw_rcb.gwr_length = sizeof(GW_RCB);

    gw_rcb.gwr_session_id = scb->scb_gw_session_id;
    status = gwf_call(GWX_BEGIN, &gw_rcb);
    if (status)
    {
	uleFormat(NULL, E_DM9A0E_GATEWAY_BEGIN, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char * )0, (i4)0, (i4 *)NULL, &local_error, 1,
	    STlength (SystemProductName), SystemProductName);
	uleFormat(&gw_rcb.gwr_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char * )0, (i4)0, (i4 *)NULL, &local_error, 0);
	SETDBERR(dberr, 0, E_DM0136_GATEWAY_ERROR);
    }

    return (status);
}

/*{
** Name: dmf_gwx_commit - notify gateway of end transaction.
**
** Description:
**	Called at DMX_COMMIT time to notify the gateway of a transaction commit.
**
** Inputs:
**	scb		- session control block.  Holds gateway session id.
**
** Outputs:
**	err_code	- reason for error:
**			E_DM0112_RESOURCE_QUOTA_EXCEED
**			E_DM0136_GATEWAY_ERROR
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-aug-89 (rogerk)
**          Created for Non-SQL Gateway.
**	9-apr-90 (bryanp)
**	    New GWF interface, using gwf_call().
*/
DB_STATUS
dmf_gwx_commit(
DML_SCB		    *scb,
DB_ERROR	    *dberr)
{
    GW_RCB		gw_rcb;
    DB_STATUS		status;
    i4		local_error;

    CLRDBERR(dberr);

    if (DMZ_TBL_MACRO(20))
    {
	TRdisplay("DMF GATEWAY : Calling GWX_COMMIT for session %d\n",
	    scb->scb_gw_session_id);
    }

    gw_rcb.gwr_type = GWR_CB_TYPE;
    gw_rcb.gwr_length = sizeof(GW_RCB);

    gw_rcb.gwr_session_id = scb->scb_gw_session_id;
    status = gwf_call(GWX_COMMIT, &gw_rcb);
    if (status)
    {
	uleFormat(NULL, E_DM9A0F_GATEWAY_COMMIT, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char * )0, (i4)0, (i4 *)NULL, &local_error, 1,
	    STlength (SystemProductName), SystemProductName);
	uleFormat(&gw_rcb.gwr_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char * )0, (i4)0, (i4 *)NULL, &local_error, 0);

	switch(gw_rcb.gwr_error.err_code)
	{
	    case E_GW0600_NO_MEM:
		SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
		break;

	    default:
		SETDBERR(dberr, 0, E_DM0136_GATEWAY_ERROR);
		break;
	}
    }

    return (status);
}

/*{
** Name: dmf_gwx_abort - notify gateway of abort transaction.
**
** Description:
**	Called at DMX_ABORT time to notify the gateway of a transaction abort.
**
** Inputs:
**	scb		- session control block.  Holds gateway session id.
**
** Outputs:
**	err_code	- reason for error:
**			E_DM0112_RESOURCE_QUOTA_EXCEED
**			E_DM0136_GATEWAY_ERROR
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-aug-89 (rogerk)
**          Created for Non-SQL Gateway.
**	9-apr-90 (bryanp)
**	    New GWF interface, using gwf_call().
*/
DB_STATUS
dmf_gwx_abort(
DML_SCB		    *scb,
DB_ERROR	    *dberr)
{
    GW_RCB		gw_rcb;
    DB_STATUS		status;
    i4		local_error;

    CLRDBERR(dberr);

    if (DMZ_TBL_MACRO(20))
    {
	TRdisplay("DMF GATEWAY : Calling GWX_ABORT for session %d\n",
	    scb->scb_gw_session_id);
    }

    gw_rcb.gwr_type = GWR_CB_TYPE;
    gw_rcb.gwr_length = sizeof(GW_RCB);

    gw_rcb.gwr_session_id = scb->scb_gw_session_id;
    status = gwf_call(GWX_ABORT, &gw_rcb);
    if (status)
    {
	uleFormat(NULL, E_DM9A10_GATEWAY_ABORT, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char * )0, (i4)0, (i4 *)NULL, &local_error, 1,
	    STlength (SystemProductName), SystemProductName);
	uleFormat(&gw_rcb.gwr_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char * )0, (i4)0, (i4 *)NULL, &local_error, 0);
	SETDBERR(dberr, 0, E_DM0136_GATEWAY_ERROR);
    }

    return (status);
}

/*{
** Name: dmf_get_scb - get session control block.
**
** Description:
**	Used to get session control block for gateway operation so we can
**	extract the gateway session id.  Some calls to record operations to not
**	provide an RCB that has the XCB attached.  In this case we do not have
**	access to the SCB and must call SCU to get it.
**
** Inputs:
**	none
**
** Outputs:
**	scb		- session control block.  Holds gateway session id.
**	err_code	- reason for error:
**			E_DM002F_BAD_SESSION_ID
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-aug-89 (rogerk)
**          Created for Non-SQL Gateway.
**	27-feb-1991 (bryanp)
**	    When this code runs in a non-server (typically, the RCP, but
**	    perhaps also the JSP, etc.), the DML_SCF pointer comes back NULL,
**	    because there is no DMF session in a non-server. For now, until
**	    we decide what it means to call the GWF routines in a non-server,
**	    we'll just fail at this point by returning an error out of this
**	    routine if there is no DMF session.
**	08-Jan-2001 (jenjo02)
**	    Use macro to find *DML_SCB instead of SCU_INFORMATION.
*/
static DB_STATUS
dmf_get_scb(
DML_SCB		    **scb,
DB_ERROR	    *dberr)
{
    CS_SID		sid;

    CLRDBERR(dberr);

    CSget_sid(&sid);

    if ( (*scb = GET_DML_SCB(sid)) )
	return (E_DB_OK);
    else
    {
	/*
	** This must not be a server (e.g., it might be the RCP), since there
	** is no DMF-SCF connection but scf_call returned OK. We don't want
	** to return a NULL pointer, so we'll return an error instead.
	*/
	SETDBERR(dberr, 0, E_DM002F_BAD_SESSION_ID);
	return (E_DB_ERROR);
    }

}

/*{
** Name: dmf_get_xcb - get transaction control block.
**
** Description:
**	Used to get transaction control block for gateway operation so we can
**	extract the database and trasaction id's.  Some calls to record
**	operations to not provide an RCB that has the XCB attached.
**
** Inputs:
**	scb		- session control block.
**	rcb		- record control block for this operation.
**
** Outputs:
**	Returns:
**	    xcb		- transaction control block - NULL if not found.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-aug-89 (rogerk)
**          Created for Non-SQL Gateway.
*/
static DML_XCB *
dmf_get_xcb(
DML_SCB		*scb,
DMP_RCB		*rcb)
{
    DML_XCB	    *xcb;

    /*
    ** Search for xcb that matches the rcb's lock list identifier.
    */
    for (xcb = scb->scb_x_next; xcb != (DML_XCB *)&scb->scb_x_next;
	 xcb = xcb->xcb_q_next)
    {
	if (xcb->xcb_lk_id == rcb->rcb_lk_id)
	    return (xcb);
    }

    return (NULL);
}

/*{
** Name: checkForInterrupt - check for user interrupt
**
** Description:
**	Check DMF'S SessionControlBlock to see whether a user interrupt
**	(generated by hitting control-C) has occurred.
**
** Inputs:
**	rcb		- record control block for this operation.
**
** Outputs:
**	Returns:
**	status		=	E_DB_ERROR if a user interrupt occurred
**	errorCode	=	E_DM0065_USER_INTR if a user interrupt occurred
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-may-1992 (schang)
**          GW merge.
**	    18-apr-1991 (rickh)
**		Created.
*/
static VOID
checkForInterrupt(
DMP_RCB	    *rcb,
DB_STATUS   *status,
DB_ERROR    *dberr)
{
	if (*(rcb->rcb_uiptr) & RCB_USER_INTR)
	{
	    SETDBERR(dberr, 0, E_DM0065_USER_INTR);
	    *status = E_DB_ERROR;
	}
}
