/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <me.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <cs.h>
#include    <adf.h>
#include    <scf.h>
/* #include    <dm.h> */
#include    <dmf.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <sxf.h>
#include    <gwf.h>
#include    <gwfint.h>

/**
** Name: GWFTAB.C - table-level generic gateway functions.
**
** Description:
**      This file contains table-level functions for the gateway facility.
**
**      This file defines the following external functions:
**    
**	gwt_register() - register table to extended catalogs
**	gwt_remove() - remove table registration from extended catalogs
**	gwt_open() - gateway table open for a session
**	gwt_close() - gateway table close for a session
**
** History:
**	04-apr-90 (linda)
**	    Created -- broken out from gwf.c which is now obsolete.
**	5-apr-90 (bryanp)
**	    Cleaned up error handling in gwt_open and gwt_close, reworked code
**	    to server conventions.
**	9-apr-90 (bryanp)
**	    Upgraded error handling to new GWF scheme.
**	16-apr-90 (linda)
**	    Added a parameter to gwu_deltcb() call indicating whether the tcb
**	    to be deleted must exist or not.
**	18-apr-90 (bryanp)
**	    Re-map exit-level 'file not found' messages into GWF-level
**	    'nonexistent table' message. This allows DMF to handle this error
**	    in a 'nice' way.
**	12-jun-1990 (bryanp)
**	    Handle E_GW050E_ALREADY_REPORTED from the register table exit. Such
**	    a return code means that there was a syntax error in the register
**	    table command and no more error logging or traceback is needed.
**	14-jun-1990 (bryanp)
**	    It turns out that returning E_GW0503_NONEXISTENT_TABLE from gwt_open
**	    may not be such a good idea. Higher levels (QEF, PSF) think that
**	    they "know" what NONEXISTENT_TABLE means, and it isn't what we
**	    want them to think. Instead, report the exit open error to the
**	    frontend with gwf_errsend() and return E_GW050E_ALREADY_REPORTED.
**	26-sep-90 (linda)
**	    Code cleanup.  Changed the following references:
**	      gw_rcb->gwr_gw_id ==> variable "gw_id"
**	      Gwf_facility->gwf_gw_info[].gwf_xrel_size ==> variable "xrel_sz"
**	      Gwf_facility->gwf_gw_info[].gwf_xatt_size ==> variable "xatt_sz"
**	      Gwf_facility->gwf_gw_info[].gwf_xidx_size ==> variable "xidx_sz"
**      18-may-92 (schang)
**          GW merge. And fixes to distinguish bad dir/file name from 
**          nonexisting one.
**	    4-feb-91 (linda)
**	        Code cleanup to silence compiler warnings on unix; add deadlock
**	        handling; use keyed access to extended system catalogs.
**	    8-aug-91 (rickh)
**	        When gateway exits have themselves reported problems
**	        to the user, spare us the alarming yet uninformative
**	        error-babble of higher facilities.
**	8-oct-92 (daveb)
**	    prototyped
**      10-dec-92 (schang)
**          RMS GW now closes base file during table remove and optionally
**          remove TCB. 
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	27-jul-93 (ralph)
**	    DELIM_IDENT:
**	    Use gws_xrel_tab_name instead of gwf_xrel_tab_name.
**	    Use gws_xatt_tab_name instead of gwf_xatt_tab_name.
**	    Use gws_xidx_tab_name instead of gwf_xidx_tab_name.
**	17-nov-93 (stephenb)
**	    Audit destruction of SXA gateway table as a security event.
**	17-oct-95 (emmag)
**	    Add include for me.h
**      19-may-99 (chash01)
**          integrate changes from oping12
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
*/

/*
**  Forward declarations and external functions.
*/
GW_RSB	*gwu_newrsb();


/*{
** Name: gwt_register - register a gateway table
**
** Description:
**	This function registers extended gateway catalog information.  A DMF
**	external interface will be used to update,  namely put, the extended
**	gateway catalogs. This request must be made before  gateway table
**	registration can be considered complete.
**
**	Note that no internal GWF tcb is created for this operation.
**
**	Note also that some gateways do not have extended system catalogs.
**	These gateways may be recognized as such due to the 0 in xrel_sz,
**	xatt_sz, and xidx_sz. For these gateways, no extended system catalog
**	updates are performed.
**
** Inputs:
**	gw_rcb->		gateway request control block
**	    gwr_session_id	an established session id
**	    gwr_database_id
**	    gwr_xact_id
**	    gwr_tab_name	table name
**	    gwr_tab_owner	table owner
**	    gwr_tab_id		table id (for ingres table)
**	    gwr_column_cnt	number of columns in this table
**	    gwr_dmf_chars	address of table options structure.
**	    gwr_attr_array	array of column_cnt entries describing the
**				ingres table for this gateway object.
**	    gwr_in_vdata1	contains the gateway specific table options
**	    gwr_in_vdata2	source of the gateway object (file or path)
**	    gwr_in_vdata4	contains table specific options
**	    gwr_in_vdata_lst	contains the gateway specific column options
**	    gwr_gw_id		gateway id, derived from DBMS type
** Output:
**	gw_rcb->
**	    gwr_error.err_code	One of the following error numbers.
**				E_GW0204_GWT_REGISTER_ERROR
**				E_GW050E_ALREADY_REPORTED
**				E_GW0600_NO_MEM
**
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
** History:
**      21-Apr-1989 (alexh)
**          Created.
**	24-dec-89 (paul)
**	    Attempted to make some sense out of this thing!
**	27-mar-90 (linda)
**	    Added error handling; single return point.
**	2-apr-90 (bryanp)
**	    Added support for gateways without extended system catalogs. At the
**	    current time, this routine does NOT support the situation in which
**	    a gateways has some extended system catalogs, but not all. If such
**	    a gateway ever comes along, changes will be needed in this routine.
**	9-apr-90 (bryanp)
**	    Upgraded error handling to new GWF scheme.
**	12-jun-1990 (bryanp)
**	    Handle E_GW050E_ALREADY_REPORTED from the register table exit. Such
**	    a return code means that there was a syntax error in the register
**	    table command and no more error logging or traceback is needed.
**	23-sep-92 (robf)
**	    Handle error return from gwu_copen (can't open extended
**	    catalogs) cleanly - this could cause an AV in DMF later on
**	    when attempting to put a record into an unopened catalog.
**	7-oct-92 (daveb)
**	    Pass exit gw_session for access to private SCB.  Prototyped.
**	    Cast db_id arg to gwu_copen; it's a PTR.
**      05-apr-99 (chash01) need a default page count.
**	12-Oct-2010 (kschendel) SIR 124544
**	    dmu_char_array replaced with DMU_CHARACTERISTICS.
*/
DB_STATUS
gwt_register( GW_RCB *gw_rcb )
{
    DB_STATUS	status = E_DB_OK;
    DB_STATUS   retcode= E_DB_OK;
    GWX_RCB	gwx;
    ULM_RCB	ulm_rcb;
    GW_SESSION	*session = (GW_SESSION *)gw_rcb->gwr_session_id;
    DMT_CB	dmt;
    DMR_CB	dmr;
    i4	gw_id = gw_rcb->gwr_gw_id;
    i4		xrel_sz = Gwf_facility->gwf_gw_info[gw_id].gwf_xrel_sz;
    i4		xatt_sz = Gwf_facility->gwf_gw_info[gw_id].gwf_xatt_sz;
    i4		xidx_sz = Gwf_facility->gwf_gw_info[gw_id].gwf_xidx_sz;
    DB_ERROR	err;

    for (;;)
    {
	if ((gw_id <= 0) || (gw_id >= GW_GW_COUNT) ||
	    (Gwf_facility->gwf_gw_info[gw_id].gwf_gw_exist == 0))
	{
	    gwf_error(E_GW0400_BAD_GWID, GWF_INTERR, 1,
		      sizeof(gw_id), &gw_id);
	    gw_rcb->gwr_error.err_code = E_GW0204_GWT_REGISTER_ERROR;
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Registration of a gateway table involves building the entries for
	** the gateway's extended catalogs. This is done as follows:
	**
	**	    1. Build a request block containg the description of the
	**	       registration requested for the gateway exit.
	**
	**	    2. Ask the gateway to format this information into rows to
	**	       be placed in the extended catalogs.
	**
	**	    3. Insert these records into iigwXX_relation and
	**	       iigwXX_attribute.
	**
	** If the gateway in question has no extended system catalogs (as is
	** the case for, e.g., the LG_TEST gateway, then there is nothing to do
	** here.
	*/

	if ((xrel_sz == 0) && (xatt_sz == 0) && (xidx_sz == 0))
	{
	    status = E_DB_OK;
	    break;
	}

	/*
	** We allocate a memory stream for use by the registration request.
	** Note that the session control block holds the identifier for the ULF
	** pool used throughout GWF.  We need to think about whether we should
	** change the setting of memleft in the ulm_rcb...
	*/
	STRUCT_ASSIGN_MACRO(session->gws_ulm_rcb, ulm_rcb);

	/*
	** Build the GWX_RCB needed to format the extended catalog information
	*/
	gwx.xrcb_tab_name = &gw_rcb->gwr_tab_name;
	gwx.xrcb_tab_id = &gw_rcb->gwr_tab_id;
	gwx.xrcb_gw_id = gw_id;
	STRUCT_ASSIGN_MACRO(gw_rcb->gwr_in_vdata1, gwx.xrcb_var_data1);
	STRUCT_ASSIGN_MACRO(gw_rcb->gwr_in_vdata2, gwx.xrcb_var_data2);
	gwx.xrcb_dmu_chars = gw_rcb->gwr_dmf_chars;
	gwx.xrcb_column_cnt = gw_rcb->gwr_column_cnt;
	gwx.xrcb_column_attr =
	    (DMT_ATT_ENTRY *)gw_rcb->gwr_attr_array.ptr_address;
	gwx.xrcb_var_data_list = gw_rcb->gwr_in_vdata_lst;
    
	/*
	** Allocate space in which the gateway can place the formatted
	** iigwXX_relation entry and the iigwXX_attribute entries.  The total
	** space required is
	**
	**	sizeof(iigwXX_relation) + column_cnt * sizeof(iigwXX_attribute)
	**
	** The relation entry will be placed in gwx.var_data3.  The attribute
	** entries are stored in gwx.mtuple_bufs
	*/

	if ((status = ulm_openstream(&ulm_rcb)) != E_DB_OK)
	{
	    gwf_error(ulm_rcb.ulm_error.err_code, GWF_INTERR, 0);
	    gwf_error(E_GW0312_ULM_OPENSTREAM_ERROR, GWF_INTERR, 3,
		      sizeof(*ulm_rcb.ulm_memleft),
		      ulm_rcb.ulm_memleft,
		      sizeof(ulm_rcb.ulm_sizepool),
		      &ulm_rcb.ulm_sizepool,
		      sizeof(ulm_rcb.ulm_blocksize),
		      &ulm_rcb.ulm_blocksize);
	    if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
		gw_rcb->gwr_error.err_code = E_GW0600_NO_MEM;
	    else
		gw_rcb->gwr_error.err_code = E_GW0204_GWT_REGISTER_ERROR;
	    break;
	}

	/*
	** First create the iigwXX_relation row buffer
	*/
	ulm_rcb.ulm_psize = xrel_sz;

	if ((status = ulm_palloc(&ulm_rcb)) != E_DB_OK)
	{
	    gwf_error(ulm_rcb.ulm_error.err_code, GWF_INTERR, 0);
	    gwf_error(E_GW0314_ULM_PALLOC_ERROR, GWF_INTERR, 4,
		      sizeof(*ulm_rcb.ulm_memleft),
		      ulm_rcb.ulm_memleft,
		      sizeof(ulm_rcb.ulm_sizepool),
		      &ulm_rcb.ulm_sizepool,
		      sizeof(ulm_rcb.ulm_blocksize),
		      &ulm_rcb.ulm_blocksize,
		      sizeof(ulm_rcb.ulm_psize),
		      &ulm_rcb.ulm_psize);
	    if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
		gw_rcb->gwr_error.err_code = E_GW0600_NO_MEM;
	    else
		gw_rcb->gwr_error.err_code = E_GW0204_GWT_REGISTER_ERROR;
	    break;
	}

	gwx.xrcb_var_data3.data_in_size = xrel_sz;
	gwx.xrcb_var_data3.data_address = ulm_rcb.ulm_pptr;

	/*
	** Create the iigwXX_attribute row buffers
	**
	**** This kind of stuff will cause alignment problems if the fields
	**** in the catalog are not all aligned on the boundary of our most
	**** restrictive machine and the length of the structures is also that
	**** of the alignment modulus of the most restrictive machine. I don't
	**** believe either constraint is either met or documented.
	*/
	ulm_rcb.ulm_psize = xatt_sz * gw_rcb->gwr_column_cnt;

	if (ulm_palloc(&ulm_rcb) != E_DB_OK)
	{
	    gwf_error(ulm_rcb.ulm_error.err_code, GWF_INTERR, 0);
	    gwf_error(E_GW0314_ULM_PALLOC_ERROR, GWF_INTERR, 4,
		      sizeof(*ulm_rcb.ulm_memleft),
		      ulm_rcb.ulm_memleft,
		      sizeof(ulm_rcb.ulm_sizepool),
		      &ulm_rcb.ulm_sizepool,
		      sizeof(ulm_rcb.ulm_blocksize),
		      &ulm_rcb.ulm_blocksize,
		      sizeof(ulm_rcb.ulm_psize),
		      &ulm_rcb.ulm_psize);
	    if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
		gw_rcb->gwr_error.err_code = E_GW0600_NO_MEM;
	    else
		gw_rcb->gwr_error.err_code = E_GW0204_GWT_REGISTER_ERROR;
	    break;
	}

	gwx.xrcb_mtuple_buffs.ptr_size = xatt_sz;
	gwx.xrcb_mtuple_buffs.ptr_in_count = gw_rcb->gwr_column_cnt;
	gwx.xrcb_mtuple_buffs.ptr_address = ulm_rcb.ulm_pptr;
	gwx.xrcb_gw_session = session;

	/* request exit formatting of the extended catalog tuples */
	status =
	    (*Gwf_facility->gwf_gw_info[gw_id].gwf_gw_exits[GWX_VTABF])(&gwx);

	if (status == E_DB_OK)
	{
	    /* initialize dmt_sequence */
	    dmt.dmt_sequence = gw_rcb->gwr_gw_sequence;

	    status = gwu_copen(&dmt, &dmr, session->gws_scf_session_id, 
		(PTR)gw_rcb->gwr_database_id, gw_id, gw_rcb->gwr_xact_id,
		&gw_rcb->gwr_tab_id,
		&session->gws_gw_info[gw_id].gws_xrel_tab_name,
		DMT_A_WRITE, FALSE, &err);
	    if (status != E_DB_OK)
	    {
		switch (err.err_code)
		{
		    case    E_GW0327_DMF_DEADLOCK:
			gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
			break;

		    default:
			gwf_error(err.err_code, GWF_INTERR, 0);
			gw_rcb->gwr_error.err_code =
			    E_GW0204_GWT_REGISTER_ERROR;
			break;
		}
		break;
	    }
	}
	else
	{
	    /*
	    ** If the exit already reported the error to the user, then we need
	    ** no more reporting. Just return the exit's error code.
	    */
	    if (gwx.xrcb_error.err_code == E_GW050E_ALREADY_REPORTED)
	    {
		gw_rcb->gwr_error.err_code = E_GW050E_ALREADY_REPORTED;
	    }
	    else
	    {
		gwf_error(gwx.xrcb_error.err_code, GWF_INTERR, 0);
		gw_rcb->gwr_error.err_code = E_GW0204_GWT_REGISTER_ERROR;
	    }
	    break;
	}

	/* put the iigwX_relation tuple */
	/*
	** Initialize the DMR control block with the information required for
	** an insert.
	*/
	dmr.type = DMR_RECORD_CB;
	dmr.length = sizeof(dmr);
	dmr.dmr_data.data_in_size = xrel_sz;
	dmr.dmr_data.data_address = gwx.xrcb_var_data3.data_address;

	if ((status = (*Dmf_cptr)(DMR_PUT, &dmr)) != E_DB_OK)
	{
	    switch (err.err_code)
	    {
		case    E_DM0042_DEADLOCK:
		    gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
		    break;

		default:
		    /***note need args here to specify which catalog***/
		    gwf_error(dmr.error.err_code, GWF_INTERR, 0);
		    gwf_error(E_GW0320_DMR_PUT_ERROR, GWF_INTERR, 2,
			      sizeof(gw_rcb->gwr_tab_id.db_tab_base),
			      &gw_rcb->gwr_tab_id.db_tab_base,
			      sizeof(gw_rcb->gwr_tab_id.db_tab_index),
			      &gw_rcb->gwr_tab_id.db_tab_index);
		    gwf_error(E_GW0623_XCAT_PUT_FAILED, GWF_INTERR, 0);
		    gw_rcb->gwr_error.err_code = E_GW0204_GWT_REGISTER_ERROR;
		    break;
	    }
	    break;
	}

	if ((status = (*Dmf_cptr)(DMT_CLOSE, &dmt)) != E_DB_OK)
	{
	    switch (err.err_code)
	    {
		case    E_DM0042_DEADLOCK:
		    gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
		    break;

		default:
		    gwf_error(dmt.error.err_code, GWF_INTERR, 0);
		    gwf_error(E_GW0325_DMT_CLOSE_ERROR, GWF_INTERR, 2,
			      sizeof(dmt.dmt_id.db_tab_base),
			      &dmt.dmt_id.db_tab_base,
			      sizeof(dmt.dmt_id.db_tab_index),
			      &dmt.dmt_id.db_tab_index);
		    gwf_error(E_GW062C_XREL_CLOSE_FAILED, GWF_INTERR, 0);
		    gw_rcb->gwr_error.err_code = E_GW0204_GWT_REGISTER_ERROR;
		    break;
	    }
	    break;
	}

	/* initialize dmt_sequence */
	dmt.dmt_sequence = gw_rcb->gwr_gw_sequence;

	/* open iigwX_attribute catalog */
	status = gwu_copen(&dmt, &dmr, session->gws_scf_session_id, 
	     (PTR)gw_rcb->gwr_database_id, gw_id, gw_rcb->gwr_xact_id,
	     &gw_rcb->gwr_tab_id,
	     &session->gws_gw_info[gw_id].gws_xatt_tab_name,
	     DMT_A_WRITE, FALSE, &err);

	if (status != E_DB_OK)
	{
		switch (err.err_code)
		{
		    case    E_GW0327_DMF_DEADLOCK:
			gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
			break;

		    default:
			gwf_error(err.err_code, GWF_INTERR, 0);
			gw_rcb->gwr_error.err_code =
			    E_GW0204_GWT_REGISTER_ERROR;
			break;
		}
	}

	/* if catalog open was successful, put all iigwX_attribute tuples */
	if (status == E_DB_OK)
	{
	    i4	i;

	    dmr.dmr_data.data_in_size = xatt_sz;
	    dmr.dmr_data.data_address =
		(char *)gwx.xrcb_mtuple_buffs.ptr_address;

	    for (i = 0;i < gw_rcb->gwr_column_cnt; i++)
	    {
		if ((status = (*Dmf_cptr)(DMR_PUT, &dmr)) != E_DB_OK)
		{
		    switch (err.err_code)
		    {
			case    E_DM0042_DEADLOCK:
			    gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
			    break;

			default:
			    /*
			    ** iigwXX_attribute INSERT failed, abort the
			    ** registration.
			    ** NOTE need args here to specify which catalog...
			    */
			    gwf_error(dmr.error.err_code, GWF_INTERR, 0);
			    gwf_error(E_GW0320_DMR_PUT_ERROR, GWF_INTERR, 2,
				      sizeof(gw_rcb->gwr_tab_id.db_tab_base),
				      &gw_rcb->gwr_tab_id.db_tab_base,
				      sizeof(gw_rcb->gwr_tab_id.db_tab_index),
				      &gw_rcb->gwr_tab_id.db_tab_index);
			    gwf_error(E_GW0623_XCAT_PUT_FAILED, GWF_INTERR, 0);
			    gw_rcb->gwr_error.err_code =
					E_GW0204_GWT_REGISTER_ERROR;
			    break;
		    }
		    break;
		}
		dmr.dmr_data.data_address += dmr.dmr_data.data_in_size;
	    }
#if 0
	    /*
	    **	Still need to close the table
	    */
	    if (status != E_DB_OK)
		break;	/* error logged above; break from outer for loop */
#endif

	    if ((status = (*Dmf_cptr)(DMT_CLOSE, &dmt)) != E_DB_OK)
	    {
		switch (err.err_code)
		{
		    case    E_DM0042_DEADLOCK:
			gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
			break;

		    default:
			gwf_error(dmt.error.err_code, GWF_INTERR, 0);
			gwf_error(E_GW0325_DMT_CLOSE_ERROR, GWF_INTERR, 2,
				  sizeof(dmt.dmt_id.db_tab_base),
				  &dmt.dmt_id.db_tab_base,
				  sizeof(dmt.dmt_id.db_tab_index),
				  &dmt.dmt_id.db_tab_index);
			gwf_error(E_GW062E_XATT_CLOSE_FAILED, GWF_INTERR, 0);
			gw_rcb->gwr_error.err_code =
				    E_GW0204_GWT_REGISTER_ERROR;
			break;
		}
		break;
	    }
	}
	retcode=status;
	/* close the temporary stream */
	if ((status = ulm_closestream(&ulm_rcb)) != E_DB_OK)
	{
	    gwf_error(ulm_rcb.ulm_error.err_code, GWF_INTERR, 0);
	    gwf_error(E_GW0313_ULM_CLOSESTREAM_ERROR, GWF_INTERR, 1,
		      sizeof(*ulm_rcb.ulm_memleft),
		      ulm_rcb.ulm_memleft);
	    gw_rcb->gwr_error.err_code = E_GW0204_GWT_REGISTER_ERROR;
	    break;
	}
	/* Put something in so catalogs don't have a random number */
	gw_rcb->gwr_page_count =  GWF_DEFAULT_PAGE_COUNT;

	/*
	**	Preseve any error status from earlier
	*/
	status=retcode;
	break;


    }

    return(status);
}

/*{
** Name: gwt_remove - remove a gateway table
**
** Description:
**
**	This function removes extended gateway catalog information.  A DMF
**	external interface will be used to update, namely delete, the extended
**	gateway catalogs.
**
**	All extended catalog tuples related to the table are removed.
**
** Inputs:
**	gw_rcb->		gateway request control block
**	  gwr_gw_id		gateway id, derived from DBMS type
**	  gwr_session_id	an established session id
**	  gwr_database_id	db id of the table
**	  gwr_xact_id		transaction id
**	  gwr_tab_name		name to the table
**	  gwr_tab_id		INGRES table id
**	  gwr_attr_array	address of DMT_ATT_ENTRY array
**
** Output:
**	gw_rcb->
**	  gwr_error.err_code	One of the following error numbers.
**				E_GW0205_GWT_REMOVE_ERROR
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
**
** History:
**      28-Apr-1989 (alexh)
**          Created.
**	2-apr-90 (bryanp)
**	    Add support for gateways with no extended system catalogs. For
**	    these gateways, just delete the TCB and return.
**	5-apr-90 (bryanp)
**	    Considerable re-working for error-handling improvements.
**	25-sep-92 (robf)
**	    If no iigwXX_index catalog then detect dmt_show error and
**	    stop, rather then trying to delete non-opened catalog (typically
**	    resulting in a DMF AV later on)
**	7-oct-92 (daveb)
**	    prototyped.  Cast db_id arg to gwu_copen; it's a PTR.
**      11-dec-92 (schang)
**          close file if removing base table in RMS GW
**	17-nov-93 (stephenb)
**	    call SXF to audit table destruction if this is an SXA gateway
**	    table.
**	14-Jan-2004 (jenjo02)
**	    Check db_tab_index > 0 for index, not simply != 0.
*/
DB_STATUS
gwt_remove(GW_RCB *gw_rcb)
{
    DB_STATUS	    status;
    DB_STATUS	    cleanup_status;
    GW_SESSION	    *session = (GW_SESSION *)gw_rcb->gwr_session_id;
    DMT_CB	    dmt;
    DMR_CB	    dmr;
    ULM_RCB	    ulm_rcb;
    bool	    stream_opened = FALSE;
    DM_DATA	    xrel_buff;
    DM_DATA	    xatt_buff;
    DM_DATA	    xidx_buff;
    i4	    gw_id = gw_rcb->gwr_gw_id;
    i4		    xrel_sz = Gwf_facility->gwf_gw_info[gw_id].gwf_xrel_sz;
    i4		    xatt_sz = Gwf_facility->gwf_gw_info[gw_id].gwf_xatt_sz;
    i4		    xidx_sz = Gwf_facility->gwf_gw_info[gw_id].gwf_xidx_sz;
    DMR_ATTR_ENTRY  attkey[2];
    DMR_ATTR_ENTRY  *attkey_ptr[2];
    DB_ERROR	    err;

    for (;;)
    {
	if ((gw_id <= 0) || (gw_id >= GW_GW_COUNT) ||
	    (Gwf_facility->gwf_gw_info[gw_id].gwf_gw_exist == 0))
	{
	    gwf_error(E_GW0400_BAD_GWID, GWF_INTERR, 0);
	    gw_rcb->gwr_error.err_code = E_GW0205_GWT_REMOVE_ERROR;
	    status = E_DB_ERROR;
	    break;
	}

        /*  schang: SPECIAL FOR RMS GW
        **  if this is an RMS remove table, we must close file so that an open
        **  file has an oppertunity to be closed as needed, that is if a file
        **  has no other table associated, it will be closed by last remove
        **  table on this file.  We only go this route if the table to be
        **  removed is a base table.
        */
        if (gw_id == GW_RMS && gw_rcb->gwr_tab_id.db_tab_index <= 0)
        {
            GWX_RCB     gwx_rcb;
            GW_RSB	*rsb;
            PTR         save_ptr;

	    gwx_rcb.xrcb_rsb = gw_rcb->gwr_gw_rsb;
            gwx_rcb.xrcb_access_mode = GWT_OPEN_NOACCESS;
	    gwx_rcb.xrcb_database_id = (PTR)gw_rcb->gwr_database_id;
	    gwx_rcb.xrcb_tab_name = &gw_rcb->gwr_tab_name;
	    gwx_rcb.xrcb_tab_id = &gw_rcb->gwr_tab_id;
	    gwx_rcb.xrcb_gw_session = session;
	    gwx_rcb.xrcb_gw_id = gw_id;
            STRUCT_ASSIGN_MACRO(gw_rcb->gwr_in_vdata2, gwx_rcb.xrcb_var_data2);
            gwx_rcb.xrcb_xhandle = 
                       Gwf_facility->gwf_gw_info[gw_id].gwf_xhandle;
            gwx_rcb.xrcb_xbitset = 
                       Gwf_facility->gwf_gw_info[gw_id].gwf_xbitset;
	    status =
	        (*Gwf_facility->gwf_gw_info[gw_id].gwf_gw_exits[GWX_VCLOSE])
                  (&gwx_rcb);
	    if (status != E_DB_OK)
	    {
	        /*
	        ** This error reporting is in addition to that done by the exit
	        ** routine. The exit routine reports errors so that we can produce
	        ** a more detailed diagnositc. Need to know the exact VMS error we
	        ** received as well as any parameters.
	        */
	        switch (gwx_rcb.xrcb_error.err_code)
	        {
		    case E_GW540C_RMS_ENQUEUE_LIMIT:
		      gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR, 0);
		      gw_rcb->gwr_error.err_code = E_GW0504_LOCK_QUOTA_EXCEEDED;
		      break;

		    case E_GW540A_RMS_FILE_PROT_ERROR:
		      gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR, 0);
		      gw_rcb->gwr_error.err_code = E_GW0505_FILE_SECURITY_ERROR;
		      break;

		    case E_GW5401_RMS_FILE_ACT_ERROR:
		      gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR, 0);
		      gw_rcb->gwr_error.err_code = E_GW0507_FILE_UNAVAILABLE;
		      break;

		    case E_GW5404_RMS_OUT_OF_MEMORY:
		      gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR, 0);
		      gw_rcb->gwr_error.err_code = E_GW0600_NO_MEM;
		      break;

		    case E_GW050E_ALREADY_REPORTED:
		      gw_rcb->gwr_error.err_code = E_GW050E_ALREADY_REPORTED;
		      break;

		    default:
		      gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR, 0);
		      gw_rcb->gwr_error.err_code = E_GW0207_GWT_CLOSE_ERROR;
		      break;
	        }
	        break;
	    }
	
        }

	if ((xrel_sz == 0) && (xatt_sz == 0) && (xidx_sz == 0))
	{
	    /*
	    ** This gateway has no extended catalogs.
	    */
	    status = E_DB_OK;
	    break;
	}

	/* set up temporary memory stream buff retrieve buffers */

	/* copy ulm_rcb */
	STRUCT_ASSIGN_MACRO(session->gws_ulm_rcb, ulm_rcb);
	/* set buff sizes */
	xrel_buff.data_in_size = xrel_sz;
	xatt_buff.data_in_size = xatt_sz;
	xidx_buff.data_in_size = xidx_sz;

	/* allocate the buffers from a temporary stream */
	/***
	****    A better strategy would be to open a single memory stream but
	****    allocate each record buffer separately. This will guarantee
	****    that each buffer starts on an aligned boundary. Once this is
	****    done the fields of the catalogs only need be properly
	****    aligned.
	****
	****	Note also that we allocate our stream using the session's
	****	ulm_rcb, which contains a pointer to memleft in the session,
	****	and thus we may be affecting the session's GWF memleft value
	****	without intending to.
	***/
	ulm_rcb.ulm_blocksize = xrel_sz + xatt_sz + xidx_sz;
	if (ulm_openstream(&ulm_rcb) != E_DB_OK)
	{
	    gwf_error(ulm_rcb.ulm_error.err_code, GWF_INTERR, 0);
	    gwf_error(E_GW0312_ULM_OPENSTREAM_ERROR, GWF_INTERR, 3,
		      sizeof(*ulm_rcb.ulm_memleft),
		      ulm_rcb.ulm_memleft,
		      sizeof(ulm_rcb.ulm_sizepool),
		      &ulm_rcb.ulm_sizepool,
		      sizeof(ulm_rcb.ulm_blocksize),
		      &ulm_rcb.ulm_blocksize);
	    if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
		gw_rcb->gwr_error.err_code = E_GW0600_NO_MEM;
	    else
		gw_rcb->gwr_error.err_code = E_GW0205_GWT_REMOVE_ERROR;
	    break;
	}
	stream_opened = TRUE;

	ulm_rcb.ulm_psize = ulm_rcb.ulm_blocksize;
	if (ulm_palloc(&ulm_rcb) != E_DB_OK)
	{
	    gwf_error(ulm_rcb.ulm_error.err_code, GWF_INTERR, 0);
	    gwf_error(E_GW0314_ULM_PALLOC_ERROR, GWF_INTERR, 4,
		      sizeof(*ulm_rcb.ulm_memleft),
		      ulm_rcb.ulm_memleft,
		      sizeof(ulm_rcb.ulm_sizepool),
		      &ulm_rcb.ulm_sizepool,
		      sizeof(ulm_rcb.ulm_blocksize),
		      &ulm_rcb.ulm_blocksize,
		      sizeof(ulm_rcb.ulm_psize),
		      &ulm_rcb.ulm_psize);
	    if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
		gw_rcb->gwr_error.err_code = E_GW0600_NO_MEM;
	    else
		gw_rcb->gwr_error.err_code = E_GW0205_GWT_REMOVE_ERROR;
	    break;
	}
  
	/* set buffer pointers */
	xrel_buff.data_address = ulm_rcb.ulm_pptr;
	xatt_buff.data_address = ((char *)ulm_rcb.ulm_pptr) + xrel_sz;
	xidx_buff.data_address = ((char *)xatt_buff.data_address) + xatt_sz;

	/* initialize dmt_sequence */
	dmt.dmt_sequence = gw_rcb->gwr_gw_sequence;

	/* position to get if this is a DMT_A_READ */
	attkey[0].attr_number = 1;
	attkey[0].attr_operator = DMR_OP_EQ;
	attkey[0].attr_value = (char *)&gw_rcb->gwr_tab_id.db_tab_base;
	attkey[1].attr_number = 2;
	attkey[1].attr_operator = DMR_OP_EQ;
	attkey[1].attr_value = (char *)&gw_rcb->gwr_tab_id.db_tab_index;

	attkey_ptr[0] = &attkey[0];
	attkey_ptr[1] = &attkey[1];

	dmr.type = DMR_RECORD_CB;
	dmr.length = sizeof(DMR_CB);
	dmr.dmr_flags_mask = 0;
	dmr.dmr_position_type = DMR_QUAL;
	dmr.dmr_attr_desc.ptr_in_count = 2;
	dmr.dmr_attr_desc.ptr_size = sizeof(DMR_ATTR_ENTRY);
	dmr.dmr_attr_desc.ptr_address = (PTR)&attkey_ptr[0];
	dmr.dmr_access_id = dmt.dmt_record_access_id;
	dmr.dmr_q_fcn = NULL;

	/* open the iigwX_relation catalog, then delete tuples */
	if ((status = gwu_copen(&dmt, &dmr, session->gws_scf_session_id,
		(PTR)gw_rcb->gwr_database_id, gw_id, gw_rcb->gwr_xact_id,
		&gw_rcb->gwr_tab_id, 
		&session->gws_gw_info[gw_id].gws_xrel_tab_name,
		DMT_A_WRITE, TRUE, &err)) != E_DB_OK)
	{
	    switch (err.err_code)
	    {
		case    E_GW0327_DMF_DEADLOCK:
		    gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
		    break;

		default:
		    gwf_error(err.err_code, GWF_INTERR, 0);
		    gw_rcb->gwr_error.err_code =
			E_GW0205_GWT_REMOVE_ERROR;
		    break;
	    }
	    break;
	}
	STRUCT_ASSIGN_MACRO(xrel_buff, dmr.dmr_data);
	status = gwu_cdelete(&dmr, &xrel_buff, gw_rcb, &err);
	if (status != E_DB_OK)
	{
	    gwf_error(err.err_code, GWF_INTERR, 0);
	    gw_rcb->gwr_error.err_code = E_GW0205_GWT_REMOVE_ERROR;
	    break;
	}
	if ((status = (*Dmf_cptr)(DMT_CLOSE, &dmt)) != E_DB_OK)
	{
	    switch (err.err_code)
	    {
		case    E_DM0042_DEADLOCK:
		    gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
		    break;

		default:
		    gwf_error(dmt.error.err_code, GWF_INTERR, 0);
		    gwf_error(E_GW0325_DMT_CLOSE_ERROR, GWF_INTERR, 2,
			      sizeof(dmt.dmt_id.db_tab_base),
			      &dmt.dmt_id.db_tab_base,
			      sizeof(dmt.dmt_id.db_tab_index),
			      &dmt.dmt_id.db_tab_index);
		    gwf_error(E_GW062C_XREL_CLOSE_FAILED, GWF_INTERR, 0);
		    gw_rcb->gwr_error.err_code = E_GW0205_GWT_REMOVE_ERROR;
		    break;
	    }
	    break;
	}

	/* initialize dmt_sequence */
	dmt.dmt_sequence = gw_rcb->gwr_gw_sequence;

    /*  NOTE:  dmr fields have already been set up for correct positioning. */

	/* open the iigwX_attribute catalog, then delete tuples */
	if ((status = gwu_copen(&dmt, &dmr, session->gws_scf_session_id, 
		(PTR)gw_rcb->gwr_database_id, gw_id, gw_rcb->gwr_xact_id,
		&gw_rcb->gwr_tab_id,
		&session->gws_gw_info[gw_id].gws_xatt_tab_name,
		DMT_A_WRITE, TRUE, &err)) != E_DB_OK)
	{
		switch (err.err_code)
		{
		    case    E_GW0327_DMF_DEADLOCK:
			gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
			break;

		    default:
			gwf_error(err.err_code, GWF_INTERR, 0);
			gw_rcb->gwr_error.err_code =
			    E_GW0205_GWT_REMOVE_ERROR;
			break;
		}
	}
	STRUCT_ASSIGN_MACRO(xatt_buff, dmr.dmr_data);
	status = gwu_cdelete(&dmr, &xatt_buff, gw_rcb, &err);
	if (status != E_DB_OK)
	{
	    gwf_error(err.err_code, GWF_INTERR, 0);
	    gw_rcb->gwr_error.err_code = E_GW0205_GWT_REMOVE_ERROR;
	    break;
	}
	else if (gw_id == GW_SXA) /* must audit successfull destruction of
				  ** security audit gateway table */
	{
	    SXF_RCB	sxfrcb;
	    MEfill(sizeof(sxfrcb), 0, (PTR)&sxfrcb);
	    sxfrcb.sxf_cb_type = SXFRCB_CB;
	    sxfrcb.sxf_length = sizeof(sxfrcb);
	    sxfrcb.sxf_force   = 0;
	    /*
	    ** We have to get at the filename somehow for this next field,
	    ** So what I've done is taken the address containing the 
	    ** iigw06_relation tuple and ofset it by the sizeof the first
	    ** two collumns thus giving the address of the third (the 
	    ** filename).
	    */
	    sxfrcb.sxf_objname = xrel_buff.data_address + sizeof(DB_TAB_ID);
	    sxfrcb.sxf_objlength= sizeof(DB_TAB_NAME);
	    sxfrcb.sxf_objowner= NULL;
	    sxfrcb.sxf_msg_id   = I_SX2042_REMOVE_TABLE;
	    sxfrcb.sxf_accessmask = (SXF_A_SUCCESS | SXF_A_REMOVE);
	    sxfrcb.sxf_auditevent = SXF_E_SECURITY;
	    status = sxf_call(SXR_WRITE, &sxfrcb);
	}

	if ((status = (*Dmf_cptr)(DMT_CLOSE, &dmt)) != E_DB_OK)
	{
	    switch (err.err_code)
	    {
		case    E_DM0042_DEADLOCK:
		    gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
		    break;

		default:
		    gwf_error(dmt.error.err_code, GWF_INTERR, 0);
		    gwf_error(E_GW0325_DMT_CLOSE_ERROR, GWF_INTERR, 2,
			      sizeof(dmt.dmt_id.db_tab_base),
			      &dmt.dmt_id.db_tab_base,
			      sizeof(dmt.dmt_id.db_tab_index),
			      &dmt.dmt_id.db_tab_index);
		    gwf_error(E_GW062E_XATT_CLOSE_FAILED, GWF_INTERR, 0);
		    gw_rcb->gwr_error.err_code = E_GW0205_GWT_REMOVE_ERROR;
		    break;
	    }
	    break;
	}

	/* initialize dmt_sequence */
	dmt.dmt_sequence = gw_rcb->gwr_gw_sequence;

    /*  NOTE:  dmr fields have already been set up for correct positioning. */

	/* Only do this if there should be a catalog */
	if( xidx_sz>0 )
	{
		/* open the iigwX_index catalog, then delete tuples */
		if ((status = gwu_copen(&dmt, &dmr,
			session->gws_scf_session_id, 
			(PTR) gw_rcb->gwr_database_id,
			gw_id,  gw_rcb->gwr_xact_id,
			&gw_rcb->gwr_tab_id,
			&session->gws_gw_info[gw_id].gws_xidx_tab_name,
			DMT_A_WRITE, TRUE, &err)) != E_DB_OK)
		{
			switch (err.err_code)
			{
			    case    E_GW0327_DMF_DEADLOCK:
				gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
				break;

			    default:
				gwf_error(err.err_code, GWF_INTERR, 0);
				gw_rcb->gwr_error.err_code =
				    E_GW0205_GWT_REMOVE_ERROR;
				break;
			}
			break;
		}
		STRUCT_ASSIGN_MACRO(xidx_buff, dmr.dmr_data);
		status = gwu_cdelete(&dmr, &xidx_buff, gw_rcb, &err);
		if (status != E_DB_OK)
		{
		    gwf_error(err.err_code, GWF_INTERR, 0);
		    gw_rcb->gwr_error.err_code = E_GW0205_GWT_REMOVE_ERROR;
		    break;
		}
		if ((status = (*Dmf_cptr)(DMT_CLOSE, &dmt)) != E_DB_OK)
		{
		    switch (err.err_code)
		    {
			case    E_DM0042_DEADLOCK:
			    gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
			    break;

			default:
			    gwf_error(dmt.error.err_code, GWF_INTERR, 0);
			    gwf_error(E_GW0325_DMT_CLOSE_ERROR, GWF_INTERR, 2,
				      sizeof(dmt.dmt_id.db_tab_base),
				      &dmt.dmt_id.db_tab_base,
				      sizeof(dmt.dmt_id.db_tab_index),
				      &dmt.dmt_id.db_tab_index);
			    gwf_error(E_GW0631_XIDX_CLOSE_FAILED, GWF_INTERR, 0);
			    gw_rcb->gwr_error.err_code = E_GW0205_GWT_REMOVE_ERROR;
			    break;
		    }
		    break;
		}
	} /* If xidx_sz >0 */
	break;	/** Should get here if all goes well. **/
    }

    /*
    ** The deletion of the TCB is outside of the FOR loop because it is
    ** performed for both error and success situations. The closing of the
    ** memory stream is performed only if we actually opened the memory stream.
    ** The 'status' variable, at this point, contains the success or failure of
    ** the gwt_remove operation; however, if we otherwise succeeded but
    ** encounter an error in either of these cleanup operations, we change
    ** status to reflect that an error occurred.
    */

    /* delete table's tcb from GWF cache */
    if ((cleanup_status =
	    gwu_deltcb(gw_rcb, GWU_TCB_OPTIONAL, &err)) != E_DB_OK)
    {
	gwf_error(err.err_code, GWF_INTERR, 0);
	if (status == E_DB_OK)
	{
	    status = cleanup_status;
	    gw_rcb->gwr_error.err_code = E_GW0205_GWT_REMOVE_ERROR;
	}
	/* but keep going, need to release memory... */
    }

    if (stream_opened &&
	(cleanup_status = ulm_closestream(&ulm_rcb)) != E_DB_OK)
    {
	gwf_error(ulm_rcb.ulm_error.err_code, GWF_INTERR, 0);
	gwf_error(E_GW0313_ULM_CLOSESTREAM_ERROR, GWF_INTERR, 1,
		  sizeof(*ulm_rcb.ulm_memleft),
		  ulm_rcb.ulm_memleft);
	if (status == E_DB_OK)
	{
	    status = cleanup_status;
	    gw_rcb->gwr_error.err_code = E_GW0205_GWT_REMOVE_ERROR;
	}
    }

    return(status);
}

/*{
** Name: gwt_open - table open
**
** Description:
**
**	This function opens the table and defines a record stream for the
**	session.  It is possible (and sometimes desirable) to define  more than
**	one record stream on a table for a  session. NOT ALL GATEWAYS WILL
**	SUPPORT THIS FUNCTION?
**	
**	If the table information does not exist, then catalog reads  will be
**	done and the read information will be cached in the Gwf_facility for
**	future reference.   A call to gws_init() must have preceded this
**	request.
**
**	Note that table to be opened by gwt_open() may be either a base table
**	or an index table. The parameter tab_id will contain an identifier that
**	indicates the type of the table being opened.
**
**	gwt_open() must be called before any record manipulations can be done
**	on a table.  gwt_open() returns gw_rsb, the environment of the table
**	open.  All other table and record manipulations must indicate the
**	proper gw_rsb.
**
** Inputs:
**	gw_rcb->
**	  gwr_session_id	current GW session id
**	  gwr_gw_id		GW id
**	  gwr_tab_id		id of the ingres table being opened. base id
**				if we are opening a base tabel and index id
**				if we are opening an index.
**	  gwr_access_mode	Gateway table access mode
**	    GWT_READ		readonly access to the gateway table
**	    GWT_WRITE		read/write access
**	    GWT_OPEN_NOACCESS	no record access allowed; foreign file not
**				opened
**	    GWT_MODIFY		no record access allowed; if foreign file
**				exists it is opened and statistics are
**				obtained; if it does not exist, defaults are
**				returend to the caller.
**	  gwr_database_id	database id
**	  gwr_xact_id		transaction id
**	  gwr_tab_id		table id
**	  gwr_gw_sequence
**	  gwr_table_entry	base table info
**	  gwr_attr_array	array of attribute info
**
** Output:
**	gw_rcb->
**	  gwr_gw_rsb		GW table info. Needed for future table
**				operations. NULL if error occurs.
**	  gwr_error.err_code	One of the following error numbers.
**				E_GW0206_GWT_OPEN_ERROR
**				E_GW0503_NONEXISTENT_TABLE (disabled)
**				E_GW0504_LOCK_QUOTA_EXCEEDED
**				E_GW0505_FILE_SECURITY_ERROR
**				E_GW0506_FILE_SYNTAX_ERROR
**				E_GW0507_FILE_UNAVAILABLE
**				E_GW050E_ALREADY_REPORTED
**				E_GW0600_NO_MEM
**
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
** History:
**      21-Apr-1989 (alexh)
**          Created.
**	23-dec-89 (paul)
**	    Modified exit call to pass GW_RSB. We will need access to it to
**	    allocate gateway specific buffers, etc.
**	5-apr-90 (bryanp)
**	    Restructured to have only one return point, added error handling.
**	18-apr-90 (bryanp)
**	    Re-map exit-level 'file not found' messages into GWF-level
**	    'nonexistent table' message. This allows DMF to handle this
**	    error in a 'nice' way.
**	14-jun-1990 (bryanp)
**	    It turns out that returning E_GW0503_NONEXISTENT_TABLE from
**	    gwt_open may not be such a good idea. Higher levels (QEF, PSF)
**	    think that they "know" what NONEXISTENT_TABLE means, and it isn't
**	    what we want them to think. For example, somebody thinks that the
**	    only way a non-existent table can occur is for a database re-org to
**	    render a query plan invalid... Instead, report the exit open error
**	    to the frontend with gwf_errsend() and return
**	    E_GW050E_ALREADY_REPORTED.
**	18-jun-90 (linda, bryanp)
**	    Add support for GWT_OPEN_NOACCESS and GWT_MODIFY.
**	25-jan-91 (RickH)
**	    Pluck timeout value from gw_rcb and cram it into the gw_rsb.
**	    This is a temporary fix (!).  Also, map E_GW5406_RMS_FILE_LOCKED
**	    into E_GW050E_ALREADY_REPORTED.
**	8-aug-91 (rickh)
**	    When gateway exits have themselves reported problems to the user,
**	    spare us the alarming yet uninformative error-babble of higher
**	    facilities.
**      16-apr-92 (schang)
**          fix the problem of reporting "file not found" as error, it is
**          just a warning for RMS. GW bug 40955
**	30-jun-92 (daveb)
**	    By spec, the exit gets the xrcb_tab_name too, and the IMA
**	    gateway needs it, so hand it in.
**	24-sep-92 (robf)
**	    Need gw_id in xrcb for gateway (SXA checks it), so make sure
**	    that its in there.
**	7-oct-92 (daveb)
**	    Hand exit gw_session for access to private SCB.  Remove
**	    dead var cl_status.
**      26-oct-92 (schang)
**          get xhandle from Gwf_facility and assign to xrcb.xrcb_xhandle
**      10-dec-92 (schang)
**          if called when removing tables(determined by the access_mode,
**          GWT_READ_NOACCESS), do nothing
*/
DB_STATUS
gwt_open(GW_RCB	*gw_rcb)
{
    GW_SESSION	*session = (GW_SESSION *)gw_rcb->gwr_session_id;
    GW_RSB	*rsb = 0;
    GWX_RCB	xrcb;
    DB_STATUS	status = E_DB_OK;
    i4	gw_id = gw_rcb->gwr_gw_id;
    DB_ERROR	err;

    for (;;)
    {
	if ((gw_id <= 0) || (gw_id >= GW_GW_COUNT) ||
	    (Gwf_facility->gwf_gw_info[gw_id].gwf_gw_exist == 0))
	{
	    gwf_error(E_GW0400_BAD_GWID, GWF_INTERR, 1,
		      sizeof(gw_id), &gw_id);
	    gw_rcb->gwr_error.err_code = E_GW0206_GWT_OPEN_ERROR;
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Create a record stream control block for this table access.  Do this
	** for base tables and for secondary indexes.  We will arrange for gets
	** from secondaries to put the appropriate data in the appropriate
	** fields.
	*/

	rsb = gwu_newrsb(session, gw_rcb, &err);

	if (!rsb)
	{
	    /* was not successful at getting an rsb */
	    gwf_error(err.err_code, GWF_INTERR, 0);
	    gw_rcb->gwr_error.err_code = E_GW0206_GWT_OPEN_ERROR;
	    status = E_DB_ERROR;
	    break;
	}
    
	/*
	** cram the timeout value down one more level.
	** temporary fix (!).  25-jan-91
	*/

	rsb->gwrsb_timeout =
	* ( ( i4 * ) ( gw_rcb->gwr_conf_array.data_address ) );

	/* make sure gateway transaction has begun */
	status = gwu_atxn(rsb, gw_id, &gw_rcb->gwr_error);
	if (status != E_DB_OK)
	{
	    gwf_error(gw_rcb->gwr_error.err_code, GWF_INTERR, 0);
	    gw_rcb->gwr_error.err_code = E_GW0206_GWT_OPEN_ERROR;
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** If open occurrs during remove table processing, skip the exit open.
	*/
	if (gw_rcb->gwr_access_mode == GWT_OPEN_NOACCESS)
	{
	    gw_rcb->gwr_gw_rsb = (PTR) rsb;
	    status = E_DB_OK;
	    break;
	}

	/*
	** Allow gateway to perform any gateway-specific open processing.  The
	** primary information needed at open time is already available in 
	** the RSB and TCB for this table.  Fill the necessary parameters in 
	** the GWX_RCB and call the gateway exit routine.
	*/
	xrcb.xrcb_access_mode = gw_rcb->gwr_access_mode;
	xrcb.xrcb_rsb = (PTR) rsb;
	xrcb.xrcb_database_id = (PTR)gw_rcb->gwr_database_id;
	xrcb.xrcb_xact_id = gw_rcb->gwr_xact_id;
	xrcb.xrcb_tab_name = &gw_rcb->gwr_tab_name;
	xrcb.xrcb_tab_id = &gw_rcb->gwr_tab_id;
	xrcb.xrcb_column_cnt = gw_rcb->gwr_table_entry->tbl_attr_count;
	xrcb.xrcb_table_desc = gw_rcb->gwr_table_entry;
	xrcb.xrcb_gw_id = gw_id;
	xrcb.xrcb_gw_session = session;
        xrcb.xrcb_xhandle = Gwf_facility->gwf_gw_info[gw_id].gwf_xhandle;
        xrcb.xrcb_xbitset = Gwf_facility->gwf_gw_info[gw_id].gwf_xbitset;
    
    	status =
	    (*Gwf_facility->gwf_gw_info[gw_id].gwf_gw_exits[GWX_VOPEN])(&xrcb);

	if (status == E_DB_OK)
	{
	    rsb->gwrsb_flags |= GWRSB_FILE_IS_OPEN;
	}
	else
	{
	    /*
	    ** Open failed in gateway-specific code. There are many possible
	    ** reasons for the exit-level open to fail, and some of them, such
	    ** as 'table not found', now need to get re-mapped into standard
	    ** GWF-level error codes which DMF will check for and handle.
	    **
	    **** At some point, we need to decide whether ALL exit-level errors
	    **** should get logged, or only those that we don't handle.  That
	    **** is, perhaps some of the gwf_error calls should get removed...
	    **
	    ** Finally, a third class of errors are now reported to the front
	    ** end via scf_errsend(), and are converted to "ALREADY_REPORTED",
	    ** which tells our caller not to do any more error logging of this
	    ** error.
	    */

	    switch (xrcb.xrcb_error.err_code)
	    {
                /* schang :  These three cases are errors, thus must separate out from the
                 **          next three possibilities
                */
		case E_GW5408_RMS_BAD_FILE_NAME:
		case E_GW5402_RMS_BAD_DEVICE_TYPE:
		case E_GW5403_RMS_BAD_DIRECTORY_NAME:
			if (gwf_errsend( xrcb.xrcb_error.err_code ) == E_DB_OK)
			{
			    gw_rcb->gwr_error.err_code =
				E_GW050E_ALREADY_REPORTED;
			}
			else
			{
			    gwf_error(xrcb.xrcb_error.err_code, GWF_INTERR, 0);
			    gw_rcb->gwr_error.err_code =
				E_GW0206_GWT_OPEN_ERROR;
			}
                        break;

		case E_GW5405_RMS_DIR_NOT_FOUND:
		case E_GW5407_RMS_FILE_NOT_FOUND:
		case E_GW5409_RMS_LOGICAL_NAME_ERROR:
		case E_GW0503_NONEXISTENT_TABLE:
		    /*gw_rcb->gwr_error.err_code=E_GW0503_NONEXISTENT_TABLE;*/

		    if (gw_rcb->gwr_access_mode == GWT_MODIFY)
		    {
			/*
			** file doesn't exist, but that's OK, since access is
			** only for statistical purposes, and we can make those
			** up...
			*/
			if (gwf_errsend( E_GW050F_DEFAULT_STATS_USED )
				!= E_DB_OK)
			{
			    gw_rcb->gwr_error.err_code =
				E_GW0206_GWT_OPEN_ERROR;
			}
			else
			{
			    rsb->gwrsb_page_cnt = GWF_DEFAULT_PAGE_COUNT;
			    status = E_DB_OK;
			}
			break;
		    }
		    else
		    {
			/* File not found, but it should have been found */

			if (gwf_errsend( xrcb.xrcb_error.err_code ) == E_DB_OK)
			{
			    gw_rcb->gwr_error.err_code =
				E_GW050E_ALREADY_REPORTED;
			}
			else
			{
			    gwf_error(xrcb.xrcb_error.err_code, GWF_INTERR, 0);
			    gw_rcb->gwr_error.err_code =
				E_GW0206_GWT_OPEN_ERROR;
			}
		    }
		    break;

		case E_GW540C_RMS_ENQUEUE_LIMIT:
		    gwf_error(xrcb.xrcb_error.err_code, GWF_INTERR, 0);
		    gw_rcb->gwr_error.err_code = E_GW0504_LOCK_QUOTA_EXCEEDED;
		    break;

		case E_GW540A_RMS_FILE_PROT_ERROR:
		    if (gwf_errsend( xrcb.xrcb_error.err_code ) == E_DB_OK)
		    {
			/* privilege error during register table is warning
			** only, allowing table registration to complete.
			*/
			if (gw_rcb->gwr_access_mode == GWT_MODIFY)
			    status = E_DB_OK;
			else
			    gw_rcb->gwr_error.err_code =
				E_GW050E_ALREADY_REPORTED;
		    }
		    else
		    {
			gwf_error(xrcb.xrcb_error.err_code, GWF_INTERR, 0);
			gw_rcb->gwr_error.err_code = E_GW0206_GWT_OPEN_ERROR;
		    }
		    break;

		case E_GW5406_RMS_FILE_LOCKED:
		    gwf_error(xrcb.xrcb_error.err_code, GWF_INTERR, 0);
		    gw_rcb->gwr_error.err_code = E_GW050E_ALREADY_REPORTED;
		    break;

		case E_GW5401_RMS_FILE_ACT_ERROR:
		    gwf_error(xrcb.xrcb_error.err_code, GWF_INTERR, 0);
		    gw_rcb->gwr_error.err_code = E_GW0507_FILE_UNAVAILABLE;
		    break;

		case E_GW5404_RMS_OUT_OF_MEMORY:
		case E_GW541B_RMS_INTERNAL_MEM_ERROR:
		    gwf_error(xrcb.xrcb_error.err_code, GWF_INTERR, 0);
		    gw_rcb->gwr_error.err_code = E_GW0600_NO_MEM;
		    break;

		case E_GW050E_ALREADY_REPORTED:
		    gw_rcb->gwr_error.err_code = E_GW050E_ALREADY_REPORTED;
		    break;

		default:
		    gwf_error(xrcb.xrcb_error.err_code, GWF_INTERR, 0);
		    gw_rcb->gwr_error.err_code = E_GW0206_GWT_OPEN_ERROR;
		    break;
	    }
	    if (status != E_DB_OK)  /* may have been reset inside switch */
		break;
	}

	/*
	** Successful completion of gwt_open:
	*/    
	gw_rcb->gwr_gw_rsb = (PTR) rsb;
	gw_rcb->gwr_page_count = rsb->gwrsb_page_cnt;

	break;
    }

    if (status != E_DB_OK)
    {
	/*
	** gwt_open has failed. If we acquired a new rsb, delete it before
	** returning. If the rsb got a tcb, cleanup the reference to the TCB.
	*/
	if (rsb)
	{
	    if (rsb->gwrsb_tcb)
	    {
		if (gwu_deltcb(gw_rcb, GWU_TCB_MANDATORY, &err) != E_DB_OK)
		{
		    gwf_error(err.err_code, GWF_INTERR, 0);
		}
	    }

	    if (gwu_delrsb(session, rsb, &err) != E_DB_OK)
	    {
		gwf_error(err.err_code, GWF_INTERR, 0);
		/*
		** status and error code are already appropriate, so we
		** don't change them here
		*/
	    }
	}
    }

    return(status);
}

/*{
** Name: gwt_close - GW table close
**
** Description:
**
**	This function performs the table close function. GW tcb information is
**	not necessarily erased.  But session-specific table-related resources
**	are freed.
**
** Inputs:
**	gw_rcb->
**	    gwr_gw_rsb		environment of the table close,
**				which was returned by gwt_open().
** Output:
**	gw_rcb->
**	    gwr_error.err_code  One of the following error numbers.
**				E_GW0203_GWT_CLOSE_ERROR
**				E_GW0504_LOCK_QUOTA_EXCEEDED
**				E_GW0505_FILE_SECURITY_ERROR
**				E_GW0507_FILE_UNAVAILABLE
**				E_GW0600_NO_MEM
**				E_GW050E_ALREADY_REPORTED
**
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
** History:
**      24-Apr-1989 (alexh)
**          Created.
**	23-dec-89 (paul)
**	    Modified gateway exit call to pass GW_RSB. We may need some of this
**	    information just like we did in the open call.
**	5-apr-90 (bryanp)
**	    If GW_TCB use count drops to 0, delete the TCB. If it is re-opened
**	    later, it can be re-allocated. This helps avoid an ever growing
**	    memory usage, but is only a short-term solution to that problem.
**	18-apr-90 (bryanp)
**	    Remap exit errors into GWF-level errors where appropriate.
**	8-aug-91 (rickh)
**	    When gateway exits have themselves reported problems to the user,
**	    spare us the alarming yet uninformative error-babble of higher
**	    facilities.
**	25-sep-92 (robf)
**	    Pass gw_id and tab_name to gateway exit in case it checks it 
**	    or wants to print it (like SXA)
**	7-oct-92 (daveb)
**	    Hand gw_session to exit for access to private SCB.  Prototyped
**	    Remove dead vars:  index, last_rsb, cl_status.
**      26-oct-92 (schang)
**          get xhandle from Gwf_facility and assign to xrcb.xrcb_xhandle
**      30-nov-92 (schang)
**          If OPEN_NOACCESS (as called from REMOVE TABLE) then optionally 
**          remove tcb.
*/
DB_STATUS
gwt_close(GW_RCB *gw_rcb)
{
    DB_STATUS	status = E_DB_OK;
    GW_RSB	*rsb = (GW_RSB *)gw_rcb->gwr_gw_rsb;
    GW_SESSION	*session = (GW_SESSION *)gw_rcb->gwr_session_id;
    GWX_RCB	gwx_rcb;
    i4	gw_id = gw_rcb->gwr_gw_id;
    DB_ERROR	err;

    for (;;)
    {
	if ((gw_id <= 0) || (gw_id >= GW_GW_COUNT) ||
	    (Gwf_facility->gwf_gw_info[gw_id].gwf_gw_exist == 0))
	{
	    gwf_error(E_GW0400_BAD_GWID, GWF_INTERR, 1,
		      sizeof(gw_id), &gw_id);
	    gw_rcb->gwr_error.err_code = E_GW0207_GWT_CLOSE_ERROR;
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Close is a simple operation.  We need only the RSB for the gateway
	** object to perform the close.  Validate argument first.
	*/
	if (!rsb)
	{
	    gwf_error(E_GW0401_NULL_RSB, GWF_INTERR, 0);
	    gw_rcb->gwr_error.err_code = E_GW0207_GWT_CLOSE_ERROR;
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Ask the gateway to close the table. However, if this table was
	** opened for no access (this action is performed only by remove
	** table), then we didn't actually call the exit to open the table, so
	** we need not close it. (also, we may have tried to open the table,
	** but failed, but proceeded to close anyway, and we don't want to
	** stupidly close the un-opened file. Such a path is taken by modify).
	*/
	if ((rsb->gwrsb_flags & GWRSB_FILE_IS_OPEN) == 0)
	{
	    status = E_DB_OK;
	}
	else
	{
	    gwx_rcb.xrcb_rsb = (PTR)rsb;
	    gwx_rcb.xrcb_gw_session = session;
	    gwx_rcb.xrcb_tab_id = &gw_rcb->gwr_tab_id;
	    gwx_rcb.xrcb_tab_name = &gw_rcb->gwr_tab_name;
	    gwx_rcb.xrcb_gw_id = gw_id;
	    gwx_rcb.xrcb_access_mode = gw_rcb->gwr_access_mode;
            gwx_rcb.xrcb_xhandle = Gwf_facility->gwf_gw_info[gw_id].gwf_xhandle;
            gwx_rcb.xrcb_xbitset = Gwf_facility->gwf_gw_info[gw_id].gwf_xbitset;

	    status =
	(*Gwf_facility->gwf_gw_info[gw_id].gwf_gw_exits[GWX_VCLOSE])(&gwx_rcb);
	}

	if (status != E_DB_OK)
	{
	    /*
	    ** This error reporting is in addition to that done by the exit
	    ** routine. The exit routine reports errors so that we can produce
	    ** a more detailed diagnositc. Need to know the exact VMS error we
	    ** received as well as any parameters.
	    */
	    switch (gwx_rcb.xrcb_error.err_code)
	    {
		case E_GW540C_RMS_ENQUEUE_LIMIT:
		    gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR, 0);
		    gw_rcb->gwr_error.err_code = E_GW0504_LOCK_QUOTA_EXCEEDED;
		    break;

		case E_GW540A_RMS_FILE_PROT_ERROR:
		    gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR, 0);
		    gw_rcb->gwr_error.err_code = E_GW0505_FILE_SECURITY_ERROR;
		    break;

		case E_GW5401_RMS_FILE_ACT_ERROR:
		    gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR, 0);
		    gw_rcb->gwr_error.err_code = E_GW0507_FILE_UNAVAILABLE;
		    break;

		case E_GW5404_RMS_OUT_OF_MEMORY:
		    gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR, 0);
		    gw_rcb->gwr_error.err_code = E_GW0600_NO_MEM;
		    break;

		case E_GW050E_ALREADY_REPORTED:
		    gw_rcb->gwr_error.err_code = E_GW050E_ALREADY_REPORTED;
		    break;

		default:
		    gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR, 0);
		    gw_rcb->gwr_error.err_code = E_GW0207_GWT_CLOSE_ERROR;
		    break;
	    }
	    break;
	}

	/*
        ** schang : open_noaccess indicates 1. file corresponding to a 
        **     register table may not exist, or 2. this close is called
        **     from remove table
        */ 
        if (gw_rcb->gwr_access_mode == GWT_OPEN_NOACCESS)
	    status = gwu_deltcb(gw_rcb, GWU_TCB_OPTIONAL, &err);
        else
	    status = gwu_deltcb(gw_rcb, GWU_TCB_MANDATORY, &err);

	if (status != E_DB_OK)
	{
	    gwf_error(err.err_code, GWF_INTERR, 0);
	    gw_rcb->gwr_error.err_code = E_GW0207_GWT_CLOSE_ERROR;
	    break;
	}
	
	/*
	** We no longer need this RSB, so we remove it.  This simply involves
	** closing the memory stream associated with it.  If the gateway-
	** specific code conformed to our protocol, any memory it allocated for
	** this RSB will also be deallocated by this call.
	*/
	status = gwu_delrsb(session, rsb, &err);
	if (status != OK)
	{
	    gwf_error( err.err_code, GWF_INTERR, 0);
	    gw_rcb->gwr_error.err_code = E_GW0207_GWT_CLOSE_ERROR;
	    break;
	}

	/*
	** Successful completion of gwt_close:
	*/
	gw_rcb->gwr_error.err_code = E_DB_OK;
	break;
    }

    return(status);
}
