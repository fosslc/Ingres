/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <me.h>
#include    <pc.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <cs.h>
#include    <lk.h>
#include    <st.h>
#include    <adf.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <ulm.h>

#include    <gwf.h>
#include    <gwfint.h>

/*
** Name: GWDMFTST.C	- source for the "DMF gateway"
**
** Description:
**	This file contains the routines which implement the DMF gateway.
**	The DMF gateway is a special gateway which is included primarily
**	for use in testing the GWF code. The DMF gateway allows you to
**	construct a gateway which accesses a normal ingres table. Commands
**	issued against the gateway acess the actual table by calling the
**	appropriate DMF routines.
**
**	Note that we call the DMF routines through a function pointer. this
**	is done to ease linking problems.
**
** History:
**	27-Mar-90 (bryanp)
**	    Added this comment and converted for use by GWF.
**	28-mar-90 (bryanp)
**	    Created GWDMF_RSB to hold record access id. Removed DML_SCB
**	    garbage and used database and transaction ids from gwx_rcb.
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      23-jul-93 (schang)
**          add me.h
**	23-sep-1996 (canor01)
**	    Moved global data definitions to gwftestdata.c.
**	01-oct-1998 (nanpr01)
**	    Initialized the newest member of the dmr_cb.
**	17-mar-1999 (somsa01)
**	    Added include of pc.h and lk.h to resolve references to locking
**	    ids.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

GLOBALREF   char    *Gwdmftst_version;
static	    DB_STATUS	(*dmf_cptr)();


/*
** Name: GWDMF_RSB  - DMF gateway internal RSB.
**
** Description:
**	This structure is the DMF gateway's private RSB.
**
** History:
**	28-mar-90 (bryanp)
**	    Created.
*/
typedef struct
{
    PTR	    gwdmf_rhandle;  /* Record access id. This value is returned
			    ** to us by dmt_open and must be provided as
			    ** input to all subsequent record-oriented
			    ** dmf calls we make on this table.
			    */
} GWDMF_RSB;

DB_STATUS
gw09_term( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return (E_DB_OK);
}

DB_STATUS
gw09_tabf( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return (E_DB_OK);
}

DB_STATUS
gw09_idxf( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return (E_DB_OK);
}


/*
** Name: gw09_open	- gateway exit to open table
**
** Description:
**	The one major piece of ugliness remaining in this routine is the
**	generation of the ingres table id by adding one to the gateway
**	table id. A better scheme for selecting the ingres table id would
**	be nice...
**
** History:
**	28-mar-90 (bryanp)
**	    Rather than plucking ODCB and XCB pointers from DML_SCB, we
**	    can get them from the gwx_rcb, which is MUCH cleaner...
**	    Also, created and allocated the GWDMF_RSB for use in holding
**	    the DMF record access id.
**	    Re-structured to exit through the bottom, per server conventions.
*/
DB_STATUS
gw09_open( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GW_RSB	*rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    i4	*error = &gwx_rcb->xrcb_error.err_code;
    DMT_CB	dmtcb;
    DB_STATUS	status;
    GWDMF_RSB	*gwdmf_rsb;

    status = E_DB_OK;
    for ( ; ; )
    {
        /*
        ** Allocate and initialize the DMF-gateway specific RSB. This control
        ** information is allocated from the memory stream associated with
        ** the record stream control block.
        */
        rsb->gwrsb_rsb_mstream.ulm_psize = sizeof(GWDMF_RSB);
        status = ulm_palloc(&rsb->gwrsb_rsb_mstream);
        if (status != E_DB_OK)
        {
	    *error = rsb->gwrsb_rsb_mstream.ulm_error.err_code;
	    break;
        }
	gwdmf_rsb = (GWDMF_RSB *)rsb->gwrsb_rsb_mstream.ulm_pptr;
	rsb->gwrsb_internal_rsb = (PTR)gwdmf_rsb;

        dmtcb.type = DMT_TABLE_CB;
        dmtcb.length = sizeof(dmtcb);

        /*
        ** Get database id.
        */
        dmtcb.dmt_db_id = gwx_rcb->xrcb_database_id;
        dmtcb.dmt_tran_id = gwx_rcb->xrcb_xact_id;

        /*
        ** Use the base table + 1 as the gateway table.
        */
	STRUCT_ASSIGN_MACRO(rsb->gwrsb_tcb->gwt_table.tbl_id, dmtcb.dmt_id);
        dmtcb.dmt_id.db_tab_base++;

        /*
        ** This will not work with some complicated queries and MST queries as
        ** the sequence numbers will never vary.
        */
        dmtcb.dmt_sequence = 1;

        dmtcb.dmt_flags_mask = 0;
        dmtcb.dmt_lock_mode = (gwx_rcb->xrcb_access_mode == GWT_READ ?
				 DMT_S : DMT_X);
        dmtcb.dmt_access_mode = (gwx_rcb->xrcb_access_mode == GWT_READ ?
				 DMT_A_READ : DMT_A_WRITE);
        dmtcb.dmt_update_mode = DMT_U_DIRECT;
        dmtcb.dmt_char_array.data_address = NULL;
        dmtcb.dmt_char_array.data_in_size = 0;
        dmtcb.dmt_record_access_id = 0;

        status = (*dmf_cptr)(DMT_OPEN, &dmtcb);
        if (status != E_DB_OK)
        {
	    *error = dmtcb.error.err_code;
	    break;
        }

        gwdmf_rsb->gwdmf_rhandle = dmtcb.dmt_record_access_id;

	break;
    }

    return ( status );
}

DB_STATUS
gw09_close( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GW_RSB	    *rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    i4	    *error = &gwx_rcb->xrcb_error.err_code;
    GWDMF_RSB	    *gwdmf_rsb = (GWDMF_RSB *)rsb->gwrsb_internal_rsb;
    DMT_CB	dmtcb;
    DB_STATUS	status;

    dmtcb.type = DMT_TABLE_CB;
    dmtcb.length = sizeof(dmtcb);
    dmtcb.dmt_flags_mask = 0;
    dmtcb.dmt_record_access_id = gwdmf_rsb->gwdmf_rhandle;

    status = (*dmf_cptr)(DMT_CLOSE, &dmtcb);
    if (status != E_DB_OK)
    {
	*error = dmtcb.error.err_code;
	return (E_DB_ERROR);
    }

    gwdmf_rsb->gwdmf_rhandle = 0;
    return (E_DB_OK);
}

DB_STATUS
gw09_position( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GW_RSB	    *rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    i4	    *error = &gwx_rcb->xrcb_error.err_code;
    GWDMF_RSB	    *gwdmf_rsb = (GWDMF_RSB *)rsb->gwrsb_internal_rsb;
    DMR_CB		dmrcb;
    DB_STATUS		status;
    DMR_ATTR_ENTRY	*ptr_array[2];
    DMR_ATTR_ENTRY	attr_array[4];

    dmrcb.type = DMR_RECORD_CB;
    dmrcb.length = sizeof(dmrcb);

    dmrcb.dmr_flags_mask = 0;
    dmrcb.dmr_access_id = gwdmf_rsb->gwdmf_rhandle;
    dmrcb.dmr_q_fcn = rsb->gwrsb_qual_func;
    dmrcb.dmr_q_arg = rsb->gwrsb_qual_arg;

    dmrcb.dmr_attr_desc.ptr_address = NULL;
    dmrcb.dmr_attr_desc.ptr_in_count = 0;
    dmrcb.dmr_position_type = DMR_ALL;
    if (rsb->gwrsb_low_key.data_in_size || rsb->gwrsb_high_key.data_in_size)
    {
	/*
	** Can do key qual on one column.
	*/
	dmrcb.dmr_position_type = DMR_QUAL;
	dmrcb.dmr_attr_desc.ptr_address = (PTR)ptr_array;
	ptr_array[0] = &attr_array[0];
	ptr_array[1] = &attr_array[1];
	if ((rsb->gwrsb_low_key.data_in_size ==
		rsb->gwrsb_high_key.data_in_size) &&
	    (MEcmp(rsb->gwrsb_low_key.data_address,
		   rsb->gwrsb_high_key.data_address,
		    rsb->gwrsb_low_key.data_in_size) == 0))
	{
	    dmrcb.dmr_attr_desc.ptr_in_count = 1;
	    attr_array[0].attr_number = 1;
	    attr_array[0].attr_operator = DMR_OP_EQ;
	    attr_array[0].attr_value = rsb->gwrsb_low_key.data_address;
	}
	else
	{
	    dmrcb.dmr_attr_desc.ptr_in_count = 2;
	    attr_array[0].attr_number = 1;
	    attr_array[0].attr_operator = DMR_OP_GTE;
	    attr_array[0].attr_value = rsb->gwrsb_low_key.data_address;
	    attr_array[1].attr_number = 1;
	    attr_array[1].attr_operator = DMR_OP_LTE;
	    attr_array[1].attr_value = rsb->gwrsb_high_key.data_address;
	}
    }

    status = (*dmf_cptr)(DMR_POSITION, &dmrcb);
    if (status)
    {
	*error = dmrcb.error.err_code;
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}

DB_STATUS
gw09_get( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GW_RSB	    *rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    i4	    *error = &gwx_rcb->xrcb_error.err_code;
    GWDMF_RSB	    *gwdmf_rsb = (GWDMF_RSB *)rsb->gwrsb_internal_rsb;
    DMR_CB		dmrcb;
    DB_STATUS		status;

    dmrcb.type = DMR_RECORD_CB;
    dmrcb.length = sizeof(dmrcb);

    dmrcb.dmr_flags_mask = DMR_NEXT;
    dmrcb.dmr_access_id = gwdmf_rsb->gwdmf_rhandle;
    dmrcb.dmr_data.data_address = gwx_rcb->xrcb_var_data1.data_address;
    dmrcb.dmr_data.data_in_size = gwx_rcb->xrcb_var_data1.data_in_size;

    status = (*dmf_cptr)(DMR_GET, &dmrcb);
    if (status)
    {
	gwx_rcb->xrcb_var_data1.data_out_size = 0;
	*error = dmrcb.error.err_code;
	if (dmrcb.error.err_code == E_DM0055_NONEXT)
	    *error = E_GW0641_END_OF_STREAM;
	return (E_DB_ERROR);
    }

    gwx_rcb->xrcb_var_data1.data_out_size = dmrcb.dmr_data.data_out_size;
    return (E_DB_OK);
}

/*
** Name: gw09_put	- gateway 'put record' exit
**
** History:
**	28-mar-90 (bryanp)
**	    Deleted return of tid, as gwf code doesn't need tids.
*/
DB_STATUS
gw09_put( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GW_RSB	    *rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    i4	    *error = &gwx_rcb->xrcb_error.err_code;
    GWDMF_RSB	    *gwdmf_rsb = (GWDMF_RSB *)rsb->gwrsb_internal_rsb;
    DMR_CB	    dmrcb;
    DB_STATUS	    status;

    dmrcb.type = DMR_RECORD_CB;
    dmrcb.length = sizeof(dmrcb);

    dmrcb.dmr_flags_mask = 0;
    dmrcb.dmr_access_id = gwdmf_rsb->gwdmf_rhandle;
    dmrcb.dmr_data.data_address = gwx_rcb->xrcb_var_data1.data_address;
    dmrcb.dmr_data.data_in_size = gwx_rcb->xrcb_var_data1.data_in_size;

    status = (*dmf_cptr)(DMR_PUT, &dmrcb);
    if (status)
    {
	*error = dmrcb.error.err_code;
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}


DB_STATUS
gw09_replace( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GW_RSB	    *rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    i4	    *error = &gwx_rcb->xrcb_error.err_code;
    GWDMF_RSB	    *gwdmf_rsb = (GWDMF_RSB *)rsb->gwrsb_internal_rsb;
    DMR_CB		dmrcb;
    DB_STATUS		status;

    dmrcb.type = DMR_RECORD_CB;
    dmrcb.length = sizeof(dmrcb);

    dmrcb.dmr_flags_mask = DMR_CURRENT_POS;
    dmrcb.dmr_access_id = gwdmf_rsb->gwdmf_rhandle;
    dmrcb.dmr_data.data_address = gwx_rcb->xrcb_var_data1.data_address;
    dmrcb.dmr_data.data_in_size = gwx_rcb->xrcb_var_data1.data_in_size;
    dmrcb.dmr_attset = (char *) 0;

    status = (*dmf_cptr)(DMR_REPLACE, &dmrcb);
    if (status)
    {
	*error = dmrcb.error.err_code;
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}


DB_STATUS
gw09_delete( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    GW_RSB	    *rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    i4	    *error = &gwx_rcb->xrcb_error.err_code;
    GWDMF_RSB	    *gwdmf_rsb = (GWDMF_RSB *)rsb->gwrsb_internal_rsb;
    DMR_CB		dmrcb;
    DB_STATUS		status;

    dmrcb.type = DMR_RECORD_CB;
    dmrcb.length = sizeof(dmrcb);

    dmrcb.dmr_flags_mask = DMR_CURRENT_POS;
    dmrcb.dmr_access_id = gwdmf_rsb->gwdmf_rhandle;

    status = (*dmf_cptr)(DMR_DELETE, &dmrcb);
    if (status)
    {
	*error = dmrcb.error.err_code;
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}

DB_STATUS
gw09_begin( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return (E_DB_OK);
}


DB_STATUS
gw09_abort( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return (E_DB_OK);
}


DB_STATUS
gw09_commit( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return (E_DB_OK);
}


DB_STATUS
gw09_info( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    gwx_rcb->xrcb_var_data1.data_address = Gwdmftst_version;
    gwx_rcb->xrcb_var_data1.data_in_size = STlength(Gwdmftst_version) + 1;

    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return (E_DB_OK);
}

/*
** Name: gw09_init	- DMF Gateway initialization
**
** Description:
**	this routine initializes the DMF Gateway.
**
** History:
**	27-mar-90 (bryanp)
**	    Created.
*/
DB_STATUS
gw09_init( gwx_rcb )
GWX_RCB	    *gwx_rcb;
{
    if (gwx_rcb->xrcb_gwf_version != GWX_VERSION)
    {
	gwx_rcb->xrcb_error.err_code = E_GW0654_BAD_GW_VERSION;
	return (E_DB_ERROR);
    }

    dmf_cptr = gwx_rcb->xrcb_dmf_cptr;	/* address of dmf_call() */

    (*gwx_rcb->xrcb_exit_table)[GWX_VTERM]	= gw09_term;
    (*gwx_rcb->xrcb_exit_table)[GWX_VTABF]	= gw09_tabf;
    (*gwx_rcb->xrcb_exit_table)[GWX_VOPEN]	= gw09_open;
    (*gwx_rcb->xrcb_exit_table)[GWX_VCLOSE]	= gw09_close;
    (*gwx_rcb->xrcb_exit_table)[GWX_VPOSITION]	= gw09_position;
    (*gwx_rcb->xrcb_exit_table)[GWX_VGET]	= gw09_get;
    (*gwx_rcb->xrcb_exit_table)[GWX_VPUT]	= gw09_put;
    (*gwx_rcb->xrcb_exit_table)[GWX_VREPLACE]	= gw09_replace;
    (*gwx_rcb->xrcb_exit_table)[GWX_VDELETE]	= gw09_delete;
    (*gwx_rcb->xrcb_exit_table)[GWX_VBEGIN]	= gw09_begin;
    (*gwx_rcb->xrcb_exit_table)[GWX_VCOMMIT]	= gw09_commit;
    (*gwx_rcb->xrcb_exit_table)[GWX_VABORT]	= gw09_abort;
    (*gwx_rcb->xrcb_exit_table)[GWX_VINFO]	= gw09_info;
    (*gwx_rcb->xrcb_exit_table)[GWX_VIDXF]	= gw09_idxf;

    gwx_rcb->xrcb_exit_cb_size = 0;
    gwx_rcb->xrcb_xrelation_sz = 0;
    gwx_rcb->xrcb_xattribute_sz = 0;
    gwx_rcb->xrcb_xindex_sz = 0;

    gwx_rcb->xrcb_error.err_code = 0;
    return (E_DB_OK);
}
