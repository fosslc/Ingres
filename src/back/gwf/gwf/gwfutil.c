/*
**Copyright (c) 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <cs.h>
#include    <lk.h>
#include    <adf.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dudbms.h>
#include    <gwf.h>
#include    <gwfint.h>
#include    <gwftrace.h>


/**
** Name: GWFUTIL.C - generic gateway utility functions.
**
** Description:
**      This file contains the utility functions for the gateway facility.
**
**      This file defines the following external functions:
**    
**	gwu_copen()	-   Gateway extended catalog open
**	gwu_deltcb()	-   Delete Gateway TCB for specified table
**	gwu_newrsb()	-   Get a new Record Stream Block
**	gwu_delrsb()	-   Delete specified RSB
**	gwu_info()	-   GW information request
**	gwu_atxn()	-   Ensure transaction begin
**	gwu_cdelete()	-   Delete tuples from Gateway extended catalog
**	gwu_validgw()	-   Check if the specified gw_id is valid for
**	                    this server.
**
**	This file defines the following internal (static) functions:
**
**	gwu_gettcb()		-   Get a new Gateway TCB
**	gwu_xattr_build_info()	-   Add extended attribute catalog info to TCB
**	gwu_xidx_build_info()	-   Add extended index catalog info to TCB
**
** History:
**  18-may-92 (schang)
**      GW merge.  too many changes since code divergeed from mainline. Just
**      roll in new code.  
**      4-feb-91 (linda)
**	    Code cleanup; deadlock handling; keyed access to extended catalogs;
**          bug fixes for TCB list handling.
**  9-sep-92 (robf)
**	Check attid on xattribute to prevent an AV if bad catalog attid.
**	Make gwu_dmptcb dependent on trace point.
**  8-oct-92 (daveb)
**	    prototyped; removed dead refs to gwu_idxrsb().
** 18-dec-92 (robf)
**	Add gwu_validgw call.
** 18-jan-93 (schang)
**      remove two lines that has uninitialize rhs in assignment
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      23-jul-93 (schang)
**          add me.h,st.h,tr.h
** 	27-jul-93 (ralph)
**	    DELIM_IDENT:
**	    Use gws_xrel_tab_name instead of gwf_xrel_tab_name.
**	    Use gws_xatt_tab_name instead of gwf_xatt_tab_name.
**	    Use gws_xidx_tab_name instead of gwf_xidx_tab_name.
**	    Use gws_cat_owner instead of DU_DBA_DBDB.
**	11-oct-93 (swm)
**	    Bug #56448
**	    Made gwf_display() portable. The old gwf_display()
**	    took an array of variable-sized arguments. It could
**	    not be re-written as a varargs function as this
**	    practice is outlawed in main line code.
**	    The problem was solved by invoking a varargs function,
**	    TRformat, directly.
**	    The gwf_display() function has now been deleted, but
**	    for flexibilty gwf_display has been #defined to
**	    TRformat to accomodate future change.
**	    All calls to gwf_display() changed to pass additional
**	    arguments required by TRformat:
**	        gwf_display(gwf_scctalk,0,trbuf,sizeof(trbuf) - 1,...)
**	    this emulates the behaviour of the old gwf_display()
**	    except that the trbuf must be supplied.
**	    Note that the size of the buffer passed to TRformat
**	    has 1 subtracted from it to fool TRformat into NOT
**	    discarding a trailing '\n'.
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in gwu_dmptcb().
**	17-mar-1999 (somsa01)
**	    Added include of pc.h and lk.h to resolve references to locking
**	    ids.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      19-may-1999(chash01)
**        integrate changes from oping12
**        25-jun-96 (schang) merge 64 change
**        17-mar-94 (schang)
**          in gwu_gettcb, provide tlist->gwt_table.tbl_storage_type value from 
**          gwrcb->gwr_table_entry->tlb_storage type
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      7-oct-2004 (thaju02)
**          Replace i4 with SIZE_TYPE for memory pool > 2Gig.
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/

static DB_STATUS gwu_xattr_build_info( GW_TCB	    *tlist,
				      GW_RCB	    *gw_rcb,
				      ULM_RCB	    *ulm_rcb,
				      DB_ERROR    *err );

static DB_STATUS gwu_xidx_build_info( GW_TCB	*tlist,
				     GW_RCB	*gw_rcb,
				     ULM_RCB	*ulm_rcb,
				     DB_ERROR	*err );

static GW_TCB *gwu_gettcb(GW_RCB *gw_rcb, DB_ERROR *err );

static VOID gwu_dmptcb(CS_SID sess);

GLOBALREF	GW_FACILITY	*Gwf_facility;

/*
** Name: gwu_copen - extended catalog open
**
** Description:
**
**	This is a utility routine for opening a gateway extended catalog for a
**	gateway table.  It returns the contents of DMT_CB and DMR_CB, which
**	may be used by the caller to do record manipulation and table closing.
**
** Inputs:
**	dmt		a DMT request control block
**	dmr		a DMR request control block
**	sid		SCF session id
**	db_id		session's database id
**	gw_id		table's gateway id
**	txn_id		transaction id of this operation
**	tab_id		the gateway way table id
**	cat_name	catalog name to be opened
**	access_mode	required access mode on the catalog
**	pos		whether or not to position the table (TRUE or FALSE)
**
** Output:
**	dmr		ready to get record
**
**      Returns:
**		E_DB_OK		catalog open successful
**		E_DB_ERROR	catalog open failed
**
** History:
**      10-May-1989 (alexh)
**          Created.
**	15-Dec-1989 (linda)
**	17-Jun-92 (daveb)
**	    Fixed some more bugs related to incomplete inputs to the DMT_SHOW
**	    call:  zero dmt_{attr,index,loc,char}_array parameters!
**	    Fixed the following bugs:  (1) shw_cb.length was wrong; needed to
**	    use sizeof(DMT_SHW_CB), not sizeof(DMT_SH_CB); (2)
**	    shw_cb.dmt_flags_mask did not have mandatory DMT_M_TABLE; (3)
**	    shw_cb.dmt_session_id needed to be filled in; (4) shw_cb needed
**	    space to receive table info.; (5) had to add owner name to shw_cb,
**	    also had to blank-pad both relation & owner name; (6) for DMT_OPEN
**	    operation, dmt had bad length (needed sizeof(DMT_CB), not
**	    sizeof(DMT_TABLE_CB); (7) also for DMT_OPEN, dmt->dmt_id was
**	    incorrect.
**	01-Oct-92 (robf)
**	    If the POSITION fails, close the table before returning an
**	    error status (i.e. table isn't left open after an error)
**	8-oct-92 (daveb)
**	    prototyped.
**	17-nov-92 (robf)
**	    Changed calls to gwf_display() to use array rather than
**	    arg list.
**	04-jun-93 (ralph)
**	    DELIM_IDENT:
**	    Use gws_cat_owner instead of DU_DBA_DBDB if GW_SESSION can be
**	    obtained from gws_gt_gw_sess().
**	25-oct-93 (vijay)
**	    Cast sid to CS_SID.
**	06-oct-93 (swm)
**	    Bug #56447
**	    Added PTR cast to eliminate compiler warning.
*/
DB_STATUS
gwu_copen(DMT_CB	*dmt,
	  DMR_CB	*dmr,
	  PTR		sid,
	  PTR		db_id,
	  i4	gw_id,
	  PTR		txn_id,
	  DB_TAB_ID	*tab_id,
	  DB_TAB_NAME	*cat_name,
	  i4	access_mode,
	  i4		pos,
	  DB_ERROR	*err )
{
    DMT_SHW_CB	    shw_cb;
    DMT_TBL_ENTRY   tbl_entry;
    GW_SESSION	    *gw_sess;
    DB_STATUS	    status = E_DB_OK;

    /*
    ** Opening an extended catalog table.  We do this in several steps.
    **
    **	    1. Get the table ID using a DMT_SHOW operation
    **	    2. Open the table using a DMT_OPEN. DMT_OPEN requires the table id
    **	       we fetched from DMT_SHOW.
    **
    **	    3. If we are planning to read the catalog for information about a
    **	       specific table, we also do a DMR_POSITION so that the open table
    **	       is ready for a GET operation.
    */

    for (;;)	/* just for a place to break from... */
    {
	/*
	** Set up to issue the DMT_SHOW request.  Although a fair amount of
	** code, the basic idea is to pass in the name of the catalog and
	** return the table_id for that catalog. Note that the table_id comes
	** back in the DMT_TBL_ENTRY structure provided as a parameter.
	shw_cb.dmt_attr_array.ptr_address = NULL;
	shw_cb.dmt_attr_array.ptr_in_count = 0;
	shw_cb.dmt_attr_array.ptr_size = 0;
	shw_cb.dmt_index_array.ptr_address = NULL;
	shw_cb.dmt_index_array.ptr_in_count = 0;
	shw_cb.dmt_index_array.ptr_size = 0;
	shw_cb.dmt_loc_array.ptr_address = NULL;
	shw_cb.dmt_loc_array.ptr_in_count = 0;
	shw_cb.dmt_loc_array.ptr_size = 0;
	shw_cb.dmt_char_array.data_address = NULL;
	shw_cb.dmt_char_array.data_in_size = 0;
	*/

	shw_cb.type = DMT_SH_CB;
	shw_cb.length = sizeof(DMT_SHW_CB);
	shw_cb.dmt_flags_mask = DMT_M_TABLE | DMT_M_NAME;
	shw_cb.dmt_db_id = db_id;
	shw_cb.dmt_session_id = (CS_SID) sid;
	shw_cb.dmt_table.data_address = (char *)&tbl_entry;
	shw_cb.dmt_table.data_in_size = sizeof(DMT_TBL_ENTRY);
	shw_cb.dmt_table.data_out_size = 0;
	shw_cb.dmt_char_array.data_address = 0;

	/*
	** fill in catalog name & owner -- must be blank padded
	*/
	STmove((char *)&cat_name->db_tab_name[0], ' ', (i4)DB_TAB_MAXNAME,
	       (char *)&shw_cb.dmt_name.db_tab_name[0]);
	if( (gw_sess = gws_gt_gw_sess()) == NULL )
	    STmove(DU_DBA_DBDB, ' ', (i4)DB_OWN_MAXNAME,
			(char *)&shw_cb.dmt_owner);
	else
	    (VOID)MEcopy(gw_sess->gws_cat_owner, DB_OWN_MAXNAME,
			(char *)&shw_cb.dmt_owner);

	if ((status = (*Dmf_cptr)(DMT_SHOW, &shw_cb)) != E_DB_OK)
	{
	    switch (dmt->error.err_code)
	    {
		case    E_DM0042_DEADLOCK:
		    err->err_code = E_GW0327_DMF_DEADLOCK;
		    break;

		default:
		    /* cannot get info on the catalog table id */
		    (VOID) gwf_error(shw_cb.error.err_code, GWF_INTERR, 0);
		    (VOID) gwf_error(E_GW0326_DMT_SHOW_ERROR, GWF_INTERR, 2,
				 sizeof(shw_cb.dmt_name.db_tab_name),
				 &shw_cb.dmt_name.db_tab_name[0],
				 sizeof(shw_cb.dmt_owner),
				 &shw_cb.dmt_owner);
		    err->err_code = E_GW0213_GWU_COPEN_ERROR;
		    break;
	    }
	    break;
	}

	/* open the extended catalog table */
	dmt->type = DMT_TABLE_CB;
	dmt->length = sizeof(DMT_CB);
	dmt->dmt_db_id = db_id;
	dmt->dmt_tran_id = txn_id;
	STRUCT_ASSIGN_MACRO(tbl_entry.tbl_id, dmt->dmt_id);
	/****
	**** dmt_sequence was provided in the gw_rcb by the caller.  should use
	**** it whenever we request DMF operations.
	****/
	dmt->dmt_sequence = 1;	/**** NOT SURE WHAT THIS SHOULD BE? */
	dmt->dmt_flags_mask = 0;
	if ((dmt->dmt_access_mode = access_mode) == DMT_A_READ)
	{
	    dmt->dmt_lock_mode = DMT_IS;
	    dmt->dmt_update_mode = DMT_U_DEFERRED;
	}
	else
	{
	    dmt->dmt_lock_mode = DMT_SIX;
	    dmt->dmt_update_mode = DMT_U_DIRECT;
	}
	dmt->dmt_char_array.data_address = NULL;
	dmt->dmt_char_array.data_in_size = 0;
	if ((status = (*Dmf_cptr)(DMT_OPEN, dmt)) != E_DB_OK)
	{
	    switch (dmt->error.err_code)
	    {
		case    E_DM0042_DEADLOCK:
		    err->err_code = E_GW0327_DMF_DEADLOCK;
		    break;

		default:
		    (VOID) gwf_error(dmt->error.err_code, GWF_INTERR, 0);
		    (VOID) gwf_error(E_GW0324_DMT_OPEN_ERROR, GWF_INTERR, 2,
			    (i4)4, (PTR)&dmt->dmt_id.db_tab_base,
			    (i4)4, (PTR)&dmt->dmt_id.db_tab_index);
		    err->err_code = E_GW0213_GWU_COPEN_ERROR;
		    break;
	    }
	    break;
	}
    
	dmr->dmr_access_id = dmt->dmt_record_access_id;

	/* position to get if this is a DMT_A_READ */

	if (pos == TRUE)
	{
	    status = (*Dmf_cptr)(DMR_POSITION, dmr);
	    if (status != E_DB_OK)
	    {
		switch (dmt->error.err_code)
		{
		    case    E_DM0042_DEADLOCK:
			err->err_code = E_GW0327_DMF_DEADLOCK;
			break;

		    default:
			(VOID) gwf_error(dmr->error.err_code, GWF_INTERR, 0);
			(VOID) gwf_error(E_GW0323_DMR_POSITION_ERROR,
				GWF_INTERR, 2,
				(i4)4, (PTR)&dmt->dmt_id.db_tab_base,
				(i4)4, (PTR)&dmt->dmt_id.db_tab_index);
			err->err_code = E_GW0213_GWU_COPEN_ERROR;
			break;
		}
		/*
		**	Close the table since caller will think gwc_open
		**	failed.
		*/
		if ((status = (*Dmf_cptr)(DMT_CLOSE, dmt)) != E_DB_OK)
		{
		    switch (dmt->error.err_code)
		    {
			case    E_DM0042_DEADLOCK:
			    err->err_code = E_GW0327_DMF_DEADLOCK;
			    break;

			default:
			    (VOID) gwf_error(dmt->error.err_code, GWF_INTERR, 0);
			    (VOID) gwf_error(E_GW0325_DMT_CLOSE_ERROR, GWF_INTERR, 2,
					     sizeof(dmt->dmt_id.db_tab_base),
					     &dmt->dmt_id.db_tab_base,
					     sizeof(dmt->dmt_id.db_tab_index),
					     &dmt->dmt_id.db_tab_index);
			    err->err_code = E_GW0213_GWU_COPEN_ERROR;
			    break;
		    }
		}
		status=E_DB_ERROR;
		break;
	    }
	}
	else
	{
	    /* no position requested; so set up dmr_cb here. */
	    dmr->type = DMR_RECORD_CB;
	    dmr->length = sizeof(DMR_CB);
	    dmr->dmr_flags_mask = 0;
	    dmr->dmr_position_type = DMR_ALL;
	    dmr->dmr_q_fcn = NULL;
	}
#if 0
	if ((status = (*Dmf_cptr)(DMR_POSITION, dmr)) != E_DB_OK)
	{
	    switch (dmt->error.err_code)
	    {
		case    E_DM0042_DEADLOCK:
		    err->err_code = E_GW0327_DMF_DEADLOCK;
		    break;

		default:
		    (VOID) gwf_error(dmr->error.err_code, GWF_INTERR, 0);
		    (VOID) gwf_error(E_GW0323_DMR_POSITION_ERROR, GWF_INTERR, 2,
				    (i4)4, (PTR)&dmt->dmt_id.db_tab_base,
				    (i4)4, (PTR)&dmt->dmt_id.db_tab_index);
		    err->err_code = E_GW0213_GWU_COPEN_ERROR;
		    break;
	    }
	    break;
	}
#endif	/* 0 */
	break;
    }

    return(status);
}

/*
** Name: gwu_deltcb - delete tcb
**
** Description:
**
**	The GWF tcb with the specified table id is deleted from cache list.
**	This action is part of gateway table removal, or index registration or
**	removal, because these operations make existing tcb information
**	obsolete.
**
**	The protocol to follow for concurrent access to TCB's is described with
**	the GW_TCB structure definition.
**
**	TCB's are not removed except if a remove is done on the gateway object
**	represented by this TCB. This means ever increasing memory allocations
**	in long running servers accessing lots of gateway objects.  A max (or
**	preferred max) should be set and TCB's LRU'd when free.
**
***CORRECTION:  AT PRESENT, TCB'S ARE ALSO REMOVED WHEN A TABLE IS CLOSED.
***SINCE THIS IS A FREQUENT OPERATION, TCB'S ARE REMOVED AND RECREATED A LOT.
***THIS WAS DEEMED BETTER THAN GROWING MEMORY INDEFINITELY.
**
**	TCB's are also managed on a sequential list. This is anti-social
**	behavior in a large server. The TCB's list should be a hash structure.
**
** Inputs:
**	tab_id	table id of the tcb
**
** Output:
**	None
**
**      Returns:
**		E_DB_OK		if delete done
**		E_DB_ERROR	tcb not found && exist_flag==GWU_TCB_MANDATORY
**
** History:
**      10-May-1989 (alexh)
**          Created.
**	28-dec-89 (paul)
**	    Create working version.
**	21-mar-90 (bryanp)
**	    changed GW_TCB memory allocation from a single giant SCF block
**	    to a ULM stream per GW_TCB, with each of the TCB components in
**	    its own piece. To deallocate the GW_TCB, we now just close
**	    the stream.
**	16-apr-90 (linda)
**	    Added parameter to indicate whether the tcb to be deleted must
**	    exist or not, changed logic to accomodate this.
**	8-oct-92 (daveb)
**	    prototyped.  Removed dead var "session"
**	06-oct-93 (swm)
**	    Bug #56443
**	    Changed tcb_dbid, rcb_dbid to PTR as gwr_database_id is now PTR.
**	11-Jun-2010 (kiria01) b123908
**	    Remove possibility of updating released data in ulm_closestream
*/
DB_STATUS
gwu_deltcb(GW_RCB *gw_rcb, i4  exist_flag, DB_ERROR *err )
{
    DB_TAB_ID	*tab_id = &gw_rcb->gwr_tab_id;
    GW_TCB	*tlist;
    GW_TCB	*last_tcb;
    i4		tcb_in_use = FALSE;
    PTR		tcb_dbid;
    PTR		rcb_dbid;
    DB_STATUS	status = E_DB_OK;
    STATUS	cl_status = OK;

    for (;;)
    {
	if ((exist_flag != GWU_TCB_MANDATORY)
	    &&
	    (exist_flag != GWU_TCB_OPTIONAL))
	{
	    (VOID) gwf_error(E_GW0666_BAD_FLAG, GWF_INTERR, 1,
			     sizeof(exist_flag), &exist_flag);
	    err->err_code = E_GW0214_GWU_DELTCB_ERROR;
	    status = E_DB_ERROR;
	    break;
	}

	/* lock the tcb list exclusively */
	if (cl_status = CSp_semaphore(TRUE, &Gwf_facility->gwf_tcb_lock))
	{
	    (VOID) gwf_error(cl_status, GWF_INTERR, 0);
	    (VOID) gwf_error(E_GW0330_CSP_SEMAPHORE_ERROR, GWF_INTERR, 0);
	    err->err_code = E_GW0214_GWU_DELTCB_ERROR;
	    status = E_DB_ERROR;
	    break;
	}

	/* search through the tcb list and remove tcb from the list*/
	tlist = Gwf_facility->gwf_tcb_list;

	if (tlist == NULL)
	{
	    if (exist_flag == GWU_TCB_MANDATORY)
	    {
		(VOID) gwf_error(E_GW0612_NULL_TCB_LIST, GWF_INTERR, 0);
		err->err_code = E_GW0214_GWU_DELTCB_ERROR;
		status = E_DB_ERROR;
		break;
	    }
	    else
	    {
		err->err_code = E_DB_OK;
		status = E_DB_OK;
		break;
	    }
	}
	else 
	{
	    tcb_dbid = tlist->gwt_db_id;
	    rcb_dbid = gw_rcb->gwr_database_id;

	    if ((tcb_dbid == rcb_dbid)
		&&
		(!MEcmp((PTR)tab_id, &tlist->gwt_table.tbl_id,
			sizeof(DB_TAB_ID))))
	    {
		/*
		** The first TCB on the list is the one we are looking for.
		*/
		tlist->gwt_use_count--;

		if (tlist->gwt_use_count > 0)
		{
		    status = E_DB_OK;
		    tcb_in_use = TRUE;
		    break;
		}
		else
		{
		    Gwf_facility->gwf_tcb_list = tlist->gwt_next;
		}
	    }
	    else
	    {
		/*
		** The first TCB wasn't the right one.  Search the rest of the
		** list for the TCB.
		*/
		status = E_DB_ERROR;
		last_tcb = tlist;
		tlist = tlist->gwt_next;
		while (tlist)
		{
		    tcb_dbid = tlist->gwt_db_id;
		    rcb_dbid = gw_rcb->gwr_database_id;

		    if ((tcb_dbid == rcb_dbid)
			&&
			(!MEcmp((PTR)tab_id, (PTR)&tlist->gwt_table.tbl_id,
			      sizeof(DB_TAB_ID))))
		    {
			/* This is the right TCB. */
			tlist->gwt_use_count--;

			if (tlist->gwt_use_count > 0)
			{
			    status = E_DB_OK;
			    tcb_in_use = TRUE;
			    break;
			}

			/* Remove the TCB from the TCB list so we can delete */
			last_tcb->gwt_next = tlist->gwt_next;
			status = E_DB_OK;
			break;
		    }
		    last_tcb = tlist;
		    tlist = tlist->gwt_next;
		}
		if (tcb_in_use == TRUE)
		    break;
	    }
	}

	if (status == E_DB_OK)
	{
	    ULM_RCB	ulm_rcb;
	    SIZE_TYPE	memleft;
	    /*** First deallocate any gateway specific tcb */
	    /* The copy and reassignment of memleft is to avoid
	    ** an illegal memory access in ulm_closestream
	    ** which will cause a released block to be updated with the
	    ** memleft details. */
	    memleft = tlist->gwt_tcb_memleft;
	    STRUCT_ASSIGN_MACRO(tlist->gwt_tcb_mstream, ulm_rcb);
	    ulm_rcb.ulm_memleft = &memleft;
	    if ((status = ulm_closestream(&ulm_rcb))
		    != E_DB_OK)
	    {
		(VOID) gwf_error(ulm_rcb.ulm_error.err_code,
				 GWF_INTERR, 0);
		(VOID) gwf_error(E_GW0313_ULM_CLOSESTREAM_ERROR, GWF_INTERR, 1,
				 sizeof(*ulm_rcb.ulm_memleft),
				 ulm_rcb.ulm_memleft);
		err->err_code = E_GW0214_GWU_DELTCB_ERROR;
		break;
	    }
	}
	else
	{
	    if (exist_flag == GWU_TCB_MANDATORY)
	    {
		/* The TCB was not found return error */
		(VOID) gwf_error(E_GW0610_BAD_TCB_CACHE, GWF_INTERR, 0);
		err->err_code = E_GW0214_GWU_DELTCB_ERROR;
		status = E_DB_ERROR;
		break;
	    }
	    else
	    {
		err->err_code = E_DB_OK;
		status = E_DB_OK;
		break;
	    }
	}

	break;
    }

    _VOID_ gwu_dmptcb((CS_SID)0);

    /* always release tcb lock if we got it. */
    if (cl_status == OK)
    {
	if (cl_status = CSv_semaphore(&Gwf_facility->gwf_tcb_lock))
	{
	    (VOID) gwf_error(cl_status, GWF_INTERR, 0);
	    (VOID) gwf_error(E_GW0331_CSV_SEMAPHORE_ERROR, GWF_INTERR, 0);
	    err->err_code = E_GW0214_GWU_DELTCB_ERROR;
	    status = E_DB_ERROR;
	}
    }
    return(status);
}

/*
** Name: gwu_newrsb - obtain a new rsb for current session
**
** Description:
**	A new rsb is required.  A new rsb is allocated from the session memory
**	pool and the tcb_lst is searched for a cached tcb.  Note that the tcb
**	is not structurally copied since tcb information in the tcb cache is
**	stable.
**
**	RSB's are the control block that describe a specifc record stream
**	(open, followed by record operations, followed by close).  They are
**	allocated when a gwt_open request is made and deallocated on a
**	gwt_close request.
**
**	RSB's contain a slot in which a gateway may attach a gateway-specific
**	control block or set of control blocks. These blocks should be
**	allocated from the RSB's memory stream so that they may be deallocated
**	without intervention from the gateway itself.
**	
** Inputs:
**	session		session control block
**	gw_rcb		the request control block
**
** Output:
**	error->
**          err_code		One of the following error numbers.
**                              E_GW0001_NO_MEM
**      Returns:
**		GW_RSB *	if rsb is allocated and initialized ok.
**		NULL		cannot get proper rsb
**
** History:
**      9-May-1989 (alexh)
**          Created.
**	28-dec-89 (paul)
**	    working version created.
**	21-mar-90 (bryanp)
**	    Fixed a bug in gwu_newrsb where an error in ulm_openstream was
**	    mistakenly returning a DB_STATUS instead of a (GW_RSB *).
**	14-jun-90 (linda, bryanp)
**	    Initialize gwrsb_access_mode field.
**	18-jun-90 (linda, bryanp)
**	    Add a flags field to rsb, gwrsb_flags, indicating (for now) whether
**	    or not the file is actually opened.
**	8-oct-92 (daveb)
**	    prototyped.  removed unused "status" var.
*/
GW_RSB *
gwu_newrsb( GW_SESSION	*session,
	   GW_RCB	*gw_rcb,
	   DB_ERROR	*err )
{
    ULM_RCB	ulm_rcb;
    GW_RSB	*rsb;

    for (;;)
    {    
	/*
	** Open a memory stream for this RSB.  All information associated with
	** this access to this table will allocate memory from this stream.
	*/
	STRUCT_ASSIGN_MACRO(session->gws_ulm_rcb, ulm_rcb);
	if (ulm_openstream(&ulm_rcb) != E_DB_OK)
	{
	    (VOID) gwf_error(ulm_rcb.ulm_error.err_code, GWF_INTERR, 0);
	    (VOID) gwf_error(E_GW0312_ULM_OPENSTREAM_ERROR, GWF_INTERR, 0);
	    err->err_code = E_GW0215_GWU_NEWRSB_ERROR;
	    rsb = NULL;
	    break;
	}

	/*
	** Allocate GWF RSB.  Note that we place the handle for the open memory
	** stream from which the RSB was allocated in the RSB itself. This will
	** allow the gateway to allocate its own control blocks from the same
	** stream. The memory will be deallocated when the RSB is closed by
	** closing the stream. Thus, the specific gateway does not have to
	** worry about memory management.
	*/
	ulm_rcb.ulm_psize = sizeof(GW_RSB);
	if (ulm_palloc(&ulm_rcb) != E_DB_OK)
	{
	    (VOID) gwf_error(ulm_rcb.ulm_error.err_code, GWF_INTERR, 0);
	    (VOID) gwf_error(E_GW0314_ULM_PALLOC_ERROR, GWF_INTERR, 0);
	    err->err_code = E_GW0215_GWU_NEWRSB_ERROR;
	    rsb = NULL;
	    break;
	}

	rsb = (GW_RSB *)ulm_rcb.ulm_pptr;

	rsb->gwrsb_internal_rsb = NULL;
	rsb->gwrsb_session = session;
	STRUCT_ASSIGN_MACRO(ulm_rcb, rsb->gwrsb_rsb_mstream);
	rsb->gwrsb_access_mode = gw_rcb->gwr_access_mode;
	rsb->gwrsb_flags = 0;
	rsb->gwrsb_qual_func = 0;

	/* get table's tcb */
	if ((rsb->gwrsb_tcb = gwu_gettcb(gw_rcb, err)) == NULL)
	{
	    /*
	    ** The TCB for the table could not be found.
	    */
	    (VOID) gwf_error(err->err_code, GWF_INTERR, 0);
	    if (ulm_closestream(&ulm_rcb) != E_DB_OK)
	    {
		(VOID) gwf_error(ulm_rcb.ulm_error.err_code, GWF_INTERR, 0);
		(VOID) gwf_error(E_GW0313_ULM_CLOSESTREAM_ERROR, GWF_INTERR, 1,
				 sizeof(*ulm_rcb.ulm_memleft),
				 ulm_rcb.ulm_memleft);
	    }
	    err->err_code = E_GW0215_GWU_NEWRSB_ERROR;
	    rsb = NULL;
	    break;
	}	
	/* Update the session control block RSB information */
	rsb->gwrsb_next = session->gws_rsb_list;
	session->gws_rsb_list = rsb;

	err->err_code = E_DB_OK;
	break;
    }

    return(rsb);
}

/*
** Name: gwu_delrsb - delete an rsb
**
** Description:
**	Delete an RSB that is currently attached to an active gateway session.
**
**	This is done by removing the rsb from the session's rsb list and then
**	closing the rsb's memory stream to delete it.
**
** Inputs:
**	session		- session from which to remove rsb
**	rsb		- rsb to remove
**
** Output:
**	None
**
**      Returns:
**	    E_DB_OK
**
** History:
**      27-dec-89 (paul)
**	    Written to support new RSB allocation strategy.
**	8-oct-92 (daveb)
**	    prototyped
**      07-jul-99 (chash01)
**        21-jan-94 (schang)
**          rewite the list delete portion
*/
DB_STATUS
gwu_delrsb( GW_SESSION  *session, GW_RSB *rsb, DB_ERROR *err )
{
    GW_RSB	*last_rsb;
    DB_STATUS	status = E_DB_OK;
    ULM_RCB	ulm_rcb;
    GW_RSB      *rsb_list;	
	
    for (;;)
    {
	/*
	** Remove RSB from the session's active RSB list.
	*/
        rsb_list = session->gws_rsb_list;
        while (rsb_list != NULL)
        {
            if (rsb_list != rsb)
            {
                last_rsb = rsb_list;
                rsb_list = rsb_list->gwrsb_next;
            }
            else
            {
                if (rsb_list == session->gws_rsb_list)
	            session->gws_rsb_list = rsb_list->gwrsb_next;
                else 
                    last_rsb->gwrsb_next = rsb_list->gwrsb_next;
                break;
            }
        }
        /*
        **  schang : one of the problem of the above code is that
        **           how about if we don't find rsb in the list.
        **           Answer : we close stream anyway.  I pray
        **           this never happen.
        */
	STRUCT_ASSIGN_MACRO(rsb->gwrsb_rsb_mstream, ulm_rcb);
	if ((status = ulm_closestream(&ulm_rcb)) != E_DB_OK)
	{
	    (VOID) gwf_error(ulm_rcb.ulm_error.err_code,
			     GWF_INTERR, 0);
	    (VOID) gwf_error(E_GW0313_ULM_CLOSESTREAM_ERROR, GWF_INTERR, 1,
			     sizeof(*ulm_rcb.ulm_memleft),
			     ulm_rcb.ulm_memleft);
	    err->err_code = E_GW0216_GWU_DELRSB_ERROR;
	    break;
	}

	/* If we got here, everything is okay. */
	err->err_code = E_DB_OK;
	break;
    }    
    return (status);
}

/*{
** Name: gwu_info - GWF information on a gateway
**
** Description:
**	This function returns information on a gateway.  Currently this
**	includes the gateway internal version id string,  number of table
**	descriptions cached and counts on types of GWF requests.
**
**	Note that this is accumulative information on a gateway type. A call to
**	gwx_init() is the only prerequisite.
**
** Inputs:
**	gw_rcb			GWF request control block
**		gw_id
**
** Output:
**	gw_rcb->
**		out_vdata1	gateway internal version id
**		open_cnt	number of table opens
**		pos_cnt		number of position requests
**		get_cnt		number of get requests
**		put_cnt		number of put requests
**		del_cnt		number of delete requests
**		rep_cnt		number of replace requests
**		error.err_code	E_GW0002_BAD_GWID
**
**
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
** History:
**      9-May-1989 (alexh)
**          Created.
**	27-dec-89 (paul)
**	    Add comments on work remaining to complete.
**	7-oct-92 (daveb)
**	    prototyped; remove unused argument err.
**	7-oct-92 (robf)
**	    Call the appropriate gateway exit to make the function work.
**	18-dec-92 (robf)
**	    Remove check on idx_sz, which has nothing to do with an
**	    info request.
*/
DB_STATUS
gwu_info(GW_RCB *gw_rcb)
{
    DB_STATUS	status = E_DB_OK;
    GWX_RCB     gwx_rcb;
    i4  gw_id=gw_rcb->gwr_gw_id;


    for (;;)
    {
	if ((gw_rcb->gwr_gw_id <= 0) ||
	    (gw_rcb->gwr_gw_id >= GW_GW_COUNT) ||
	    (Gwf_facility->gwf_gw_info[gw_rcb->gwr_gw_id].gwf_gw_exist == 0))
	{
	    (VOID) gwf_error(E_GW0400_BAD_GWID, (i4)0, GWF_INTERR, 0);
	    gw_rcb->gwr_error.err_code = E_GW0212_GWU_INFO_ERROR;
	    status = E_DB_ERROR;
	    break;
	}

	gwx_rcb.xrcb_gw_id=gw_id;
        gwx_rcb.xrcb_var_data1.data_address=0;
        status= (*Gwf_facility->gwf_gw_info[gw_id].gwf_gw_exits[GWX_VINFO])(&gwx_rcb);
        if( status!=E_DB_OK)
                break;
        /*
        **      Output points to input value.
        */
        STRUCT_ASSIGN_MACRO(gwx_rcb.xrcb_var_data1,gw_rcb->gwr_in_vdata1);
        break;

    }

    return(status);
}

/*
** Name: gwu_atxn - ascertain transaction begin
**
** Description:
**
**	This is a utility routine for making sure that the current session has
**	a gateway transaction set. All gateway data manipulations must make
**	sure that a transaction has begun.
**
**	Optimization: to minimize the number of calls to this routine, some
**	record manipulation routines do not need to call this routine since
**	table open is required before those manipulations, and a transaction is
**	always begun at table open.
**
** Inputs:
**	session		GWF session control block
**	gw_id		gateway id
**	error
**
** Output:
**	rsb->in_use		is set to FALSE if transaction begin fails
**	rsb->session->txn_begun	is set to TRUE if transaction begin succedes
**	error->err_code		E_GW0003_TX_BEGIN_FAILED
**
**      Returns:
**		E_DB_OK		transaction begin ascertained
**		E_DB_ERROR	transaction begin failed
**
** History:
**      9-Jun-1989 (alexh)
**          Created.
**	7-oct-92 (daveb)
**	    Hand exit gw_session for access to private SCB.
**	8-oct-92 (daveb)
**	    prototyped
*/
DB_STATUS
gwu_atxn(GW_RSB	*rsb, i4 gw_id, DB_ERROR *err )
{
    GWX_RCB	gwx_rcb;
    DB_STATUS	status = E_DB_OK;

    for (;;)
    {
	gwx_rcb.xrcb_rsb = (PTR)rsb;
	gwx_rcb.xrcb_gw_session = rsb->gwrsb_session;
	if (rsb->gwrsb_session->gws_txn_begun == FALSE)
	{
	    /* session is not in transaction yet. put it in transaction */
	    if ((status =
		    (*Gwf_facility->gwf_gw_info[gw_id].gwf_gw_exits
						[GWX_VBEGIN])
			(&gwx_rcb)) != E_DB_OK)
	    {
		/* cannot begin transaction */
		(VOID) gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR, 0);
		err->err_code = E_GW0217_GWU_ATXN_ERROR;
		break;
	    }
	    else
	    {
		rsb->gwrsb_session->gws_txn_begun = TRUE;
		err->err_code = E_DB_OK;
	    }
	}

	break;
    }

  return(status);
}

/*
** Name: gwu_cdelete - delete tuples from iigwXX_xxxxxx catalogs
**
** Description:
**
**	This is a utility routine for deleting tuples from the extended gateway
**	catalogs.  Called from gwt_remove(), separated out since same for each
**	catalog.
**
** Inputs:
**	dmrcb		DMR_CB:  dmf record call control block
**
** Output:
**	dmr->error.err_code	gets any error codes generated
**
**      Returns:
**		E_DB_OK		tuples successfully deleted
**		E_DB_ERROR	an error occurred & was logged; error code is
**				in dmr->error.err_code
**		E_DB_WARN	no tuples were found to delete
**
** History:
**      5-jan-1990 (linda)
**          Created.
**	8-oct-92 (daveb)
**	    prototyped.
*/

DB_STATUS
gwu_cdelete(DMR_CB	*dmrcb,
	    DM_DATA	*dmdata,
	    GW_RCB	*gwrcb,
	    DB_ERROR	*err )
{
    DB_STATUS	status;

    for (;;)
    {
	/* Search iigwXX_xxxxxx catalog for relevant tuple(s) */

	dmrcb->dmr_flags_mask = DMR_NEXT;
	dmrcb->dmr_char_array.data_address = 0;

	if ((status = (*Dmf_cptr)(DMR_GET, dmrcb)) == E_DB_OK)
	{
	    if (!MEcmp((PTR)dmdata->data_address,
		       (PTR)&gwrcb->gwr_tab_id,
		       sizeof(DB_TAB_ID)))
	    {
		/* Found a match, now try to delete the tuple; break on error */
		dmrcb->dmr_flags_mask = DMR_CURRENT_POS;
		if ((status = (*Dmf_cptr)(DMR_DELETE, dmrcb)) != E_DB_OK)
		{
		    (VOID) gwf_error(dmrcb->error.err_code, GWF_INTERR, 0);
		    (VOID) gwf_error(E_GW0322_DMR_DELETE_ERROR, GWF_INTERR, 2,
			    (i4)4, (PTR)&gwrcb->gwr_tab_id.db_tab_base,
			    (i4)4, (PTR)&gwrcb->gwr_tab_id.db_tab_index);
		    err->err_code = E_GW0218_GWU_CDELETE_ERROR;
		    break;
		}

	    }
	}
	else
	{
	    if (dmrcb->error.err_code == E_DM0055_NONEXT)
	    {
		status = E_DB_OK;
		dmrcb->error.err_code = E_DB_OK;
	    }
	    else    /* handle the error from DMR_GET */
	    {
		switch (dmrcb->error.err_code)
		{
		    case    E_DM0042_DEADLOCK:
			err->err_code = E_GW0327_DMF_DEADLOCK;
			break;

		    default:
			(VOID) gwf_error(dmrcb->error.err_code, GWF_INTERR, 0);
			(VOID) gwf_error(E_GW0321_DMR_GET_ERROR, GWF_INTERR, 2,
					 (i4)4, (PTR)&gwrcb->gwr_tab_id.db_tab_base,
					 (i4)4, (PTR)&gwrcb->gwr_tab_id.db_tab_index);
			err->err_code = E_GW0218_GWU_CDELETE_ERROR;
			break;
		}
	    }
	    break;  /* Either way, we're done. */
	}
	/* No break here; it's done in the else clause above. */
    }

    if (status == E_DB_OK)
	err->err_code = E_DB_OK;

    return (status);
}

/*
** Name: gwu_gettcb - obtain the required table control block
**
** Description:
**
**	The GWF tcb cache list is searched for the desired tcb.  If it is not
**	in the cache then a new tcb is allocated,  extended catalog information
**	is retrieved and put into the new tcb, and the new tcb is prepended to
**	the cache list.
**
**	Note that separate TCB's are allocated for a base table and each index
**	reference since each has a separate TAB_ID.
**
**	The GW_TCB currently gets laid out as follows:
**	Piece 1:    GW_TCB structure, contains the ULM stream ID in it
**	Piece 2:    Gateway extended relation information.
**	Piece 3:    Array of GW_ATT_ENTRY structures, one more than the
**		    number of attributes in the table. These blocks
**		    contain DMT_ATT_ENTRY information, as well as pointers
**		    to the extended information for each attribute
**	Piece 4:    Array of extended attribute information, same number
**		    of members as Piece 3. Each member is a catalog tuple.
**	Piece 5:    GW_IDX_ENTRY structure for the secondary index.
**	Piece 6:    extended index tuple information, corresponding
**		    to Piece 5.
**	
** Inputs:
****	The list of parameters should be detailed here
**	gw_rcb			The GWF request control block
**				see gwt_open for detail.
**
** Output:
**	error->
**          err_code		One of the following error numbers.
**                              E_GW0000_OK                
**                              E_GW0001_NO_MEM
**				E_GW0004_XREL_OPEN_FAILED
**				E_GW0005_XATTR_OPEN_FAILED
**				E_GW0006_XIDX_OPEN_FAILED
**      Returns:
**		GW_TCB *	if tcb is acertained
**		NULL		cannot get proper tcb
**
** History:
**      10-May-1989 (alexh)
**          Created.
**	18-Dec-1989 (linda)
**	    Fill in cb->scf_type = SCF_CB_TYPE and
**	            cb->scf_length = sizeof(SCF_CB) before scf_call().
**	23-dec-89 (paul)
**	    There is gateway specific information associated with a TCB.
**	    Added the logic necessary to support such an object.
**	21-mar-90 (bryanp)
**	    Changed GW_TCB allocation (in gwu_gettcb and gwu_deltcb) to
**	    use ULM memory streams rather than direct SCF allocation. This
**	    is much more 'social' in that the GWF memory is now associated
**	    with GWF, rather than being server-global. Each GW_TCB gets its
**	    own memory stream. Thirdly, I restructured gwu_gettcb to have
**	    only one exit, at the bottom, and made it static since it's only
**	    called from within gwf.c.
**	2-apr-90 (bryanp)
**	    Added support for gateways without extended system catalogs. These
**	    gateways get much abbreviated GW_TCB's built. In order to add
**	    optional extended system catalog support, broke out the processing
**	    for the xrelation and xattribute catalogs into a new routine,
**	    gwu_xattr_build_info()
**	5-apr-90 (bryanp)
**	    To support GW_TCB's in their own memory streams, added memleft
**	    support - each GW_TCB's memory stream now gets its own memory
**	    limit so no individual GW_TCB can consume all of memory.
**	8-oct-92 (daveb)
**	    prototyped.  Removed unused "index" and "err_code"
**	6-oct-92 (robf)
**	    Only lookup TCB index info for index tables
**	08-sep-93 (swm)
**	    Changed session id casts from i4 to CS_SID to match recent CL
**	    interface revision.
**	06-oct-93 (swm)
**	    Bug #56443
**	    Changed tcb_dbid, rcb_dbid to PTR as gwr_database_id is now PTR.
**      08-jul-99 (schang)
**        17-mar-94 (schang)
**          provide tlist->gwt_table.tbl_storage_type value from 
**          gwrcb->gwr_table_entry->tlb_storage type
*/
static GW_TCB *
gwu_gettcb(GW_RCB *gw_rcb, DB_ERROR *err )
{
    GW_TCB	    *tlist;
    ULM_RCB	    ulm_rcb;
    GW_SESSION	    *session = (GW_SESSION *)gw_rcb->gwr_session_id;
    SIZE_TYPE	    memleft = GWF_TCBMEM_DEFAULT;
    bool	    stream_opened = FALSE;
    DB_TAB_ID	    base_id;
    PTR		    tcb_dbid;
    PTR		    rcb_dbid;
    STATUS	    cl_status;
    DB_STATUS	    status = E_DB_OK;
    char	    trbuf[GWF_MAX_MSGLEN + 1]; /* last char for `\n' */

 
    if( GWF_MAIN_MACRO(20))
    {
        gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
	    "GWU_GETTCB: Building tcb for [%d.%d]\n",
	    gw_rcb->gwr_tab_id.db_tab_base,
	    gw_rcb->gwr_tab_id.db_tab_index);

    }
    base_id.db_tab_base = gw_rcb->gwr_tab_id.db_tab_base;
    base_id.db_tab_index = 0;
        
    for(;;)
    {
	/* lock the tcb list exclusive */
	if (cl_status = CSp_semaphore(TRUE, &Gwf_facility->gwf_tcb_lock))
	{
	    (VOID) gwf_error(cl_status, GWF_INTERR, 0);
	    (VOID) gwf_error(E_GW0330_CSP_SEMAPHORE_ERROR, GWF_INTERR, 0);
	    err->err_code = E_GW0219_GWU_GETTCB_ERROR;
	    status = E_DB_ERROR;
	    break;
	}

	_VOID_ gwu_dmptcb((CS_SID)session->gws_scf_session_id);

        /*
	** Search for the requested TCB.  Key is TAB_ID.
        */
        tlist = Gwf_facility->gwf_tcb_list;

        while (tlist)
        {
	    tcb_dbid = tlist->gwt_db_id;
	    rcb_dbid = gw_rcb->gwr_database_id;

	    if ((tcb_dbid == rcb_dbid)
		&&
		(!MEcmp((PTR)&tlist->gwt_table.tbl_id,
			(PTR)&gw_rcb->gwr_tab_id, sizeof(DB_TAB_ID))))
	    {
		/* TCB was found, indicate new user. */
		tlist->gwt_use_count++;
		break;
	    }
	    tlist = tlist->gwt_next;
	}

	if (tlist)
	{
	    break;
	}
	
	/*
	** The TCB was not found, create one for this table.
	*/

	/* 
	** Open a memory stream for this TCB. All information associated with
	** this TCB will be allocated from this stream. Currently, TCB's are
	** shared across all sessions and their lifetimes are typically until
	** the server is brought down.
**ACTUALLY WE DELETE THEM ON CLOSE TO PREVENT UNBOUNDED MEMORY GROWTH.  IN THE
**FUTURE THE LIST SHOULD BE HASHED AND THE TCB'S SHOULD BE LRU'D OUT.
	**
	**	Also, there may be a rare condition with two users creating the
	**	same TCB. We allow this since TCB's contain read-only
	**	information.  This will only be a rpoblem if there is a hole
	**	where use counts can get out of date. I haven't run across one
	**	(yet).
	**
	** Each GW_TCB gets its own memory stream, and each memory stream has
	** its own limit, thus no individual GW_TCB can grow excessively large.
	*/

	STRUCT_ASSIGN_MACRO(session->gws_ulm_rcb, ulm_rcb);
	ulm_rcb.ulm_memleft = &memleft;

	if ((status = ulm_openstream(&ulm_rcb)) != E_DB_OK)
	{
	    (VOID) gwf_error(ulm_rcb.ulm_error.err_code, GWF_INTERR, 0);
	    (VOID) gwf_error(E_GW0312_ULM_OPENSTREAM_ERROR, GWF_INTERR, (i4)0);
	    err->err_code = E_GW0219_GWU_GETTCB_ERROR;
	    break;
	}
	stream_opened = TRUE;

	/*
	** A TCB's size is
	**	sizeof(GW_TCB) + sizeof(xrelation) + sizeof(sum(xattributes))
	**	+ sizeof(sum(xindexes))
	**
	** We allocate one extra attribute slot as a temporary buffer used to
	** stage extended attribute entries coming from disk.
	**
	** We allocate the information in 4 pieces:
	** 1) GW_TCB
	** 2) xrelation
	** 3) xattributes array
	** 4) xindexes array
	** This should alleviate alignment problems, although I'm not sure that
	** it helps the individual components of the xattributes and xindexes
	** pieces.
	**
	** Note that we place the handle for the open memory stream from which
	** the GW_TCB was allocated in the GW_TCB itself. Then we allocate the
	** variable sized extensions to the GW_TCB from the same stream. To
	** deallocate the GW_TCB and all of its components, all that need be
	** done is to close the stream.
	**
	** Note also that not all gateways have extended system catalogs.  If a
	** particular gateways does not have extended system catalogs, then
	** only the GW_TCB structure gets built.
	*/

	{
	    ulm_rcb.ulm_psize = sizeof(GW_TCB);
	    if ((status = ulm_palloc(&ulm_rcb)) != E_DB_OK)
	    {
		(VOID) gwf_error(ulm_rcb.ulm_error.err_code, GWF_INTERR, 0);
		(VOID) gwf_error(E_GW0314_ULM_PALLOC_ERROR, GWF_INTERR, 0);
		err->err_code = E_GW0219_GWU_GETTCB_ERROR;
		break;
	    }
	    tlist = (GW_TCB *)ulm_rcb.ulm_pptr;
	
	    /* initialize the tcb */
	    tlist->gwt_gw_id = gw_rcb->gwr_gw_id;
	    STRUCT_ASSIGN_MACRO(gw_rcb->gwr_tab_id, tlist->gwt_table.tbl_id);
	    tlist->gwt_db_id = gw_rcb->gwr_database_id;
	    tlist->gwt_table.tbl_attr_count =
		gw_rcb->gwr_table_entry->tbl_attr_count;
	    tlist->gwt_table.tbl_index_count =
		gw_rcb->gwr_table_entry->tbl_index_count;
	    tlist->gwt_use_count = 1;
	    tlist->gwt_internal_tcb = NULL;
            /*
            ** schang : provide value to tbl_storage_type in tlist
            **    it is later used in positioning
            */
            tlist->gwt_table.tbl_storage_type = 
                gw_rcb->gwr_table_entry->tbl_storage_type;

	    if (Gwf_facility->gwf_gw_info[gw_rcb->gwr_gw_id].gwf_xrel_sz > 0)
	    {
		ulm_rcb.ulm_psize =
		    Gwf_facility->gwf_gw_info[gw_rcb->gwr_gw_id].gwf_xrel_sz;
		if ((status = ulm_palloc(&ulm_rcb)) != E_DB_OK)
		{
		    (VOID) gwf_error(ulm_rcb.ulm_error.err_code, GWF_INTERR, 0);
		    (VOID) gwf_error(E_GW0314_ULM_PALLOC_ERROR, GWF_INTERR, 0);
		    err->err_code = E_GW0219_GWU_GETTCB_ERROR;
		    break;
		}
		tlist->gwt_xrel.data_address = ulm_rcb.ulm_pptr;
		tlist->gwt_xrel.data_in_size = 
		    Gwf_facility->gwf_gw_info[gw_rcb->gwr_gw_id].gwf_xrel_sz;
	    }
	    else
	    {
		tlist->gwt_xrel.data_address = (PTR)0;
		tlist->gwt_xrel.data_in_size = 0;
	    }

	    if (Gwf_facility->gwf_gw_info[gw_rcb->gwr_gw_id].gwf_xatt_sz > 0)
	    {
		status = gwu_xattr_build_info( tlist, gw_rcb, &ulm_rcb, err );
		if (status != E_DB_OK)
		{
		    switch (err->err_code)
		    {
			case    E_GW0327_DMF_DEADLOCK:
			    gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
			    break;

			default:
			    (VOID) gwf_error(err->err_code, GWF_INTERR, 0);
			    err->err_code = E_GW0219_GWU_GETTCB_ERROR;
			    break;
		    }
		    break;
		}
	    }
	    else
	    {
		tlist->gwt_attr = (GW_ATT_ENTRY *)0;
	    }
	}   
        /*
        **      Only get index info if
        **      1) There is an extended index catalog
        **      2) The TCB is an index table
        */

	if (Gwf_facility->gwf_gw_info[gw_rcb->gwr_gw_id].gwf_xidx_sz > 0 &&
            gw_rcb->gwr_tab_id.db_tab_index>0)
	{
            if( GWF_MAIN_MACRO(20))
            {
                gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
		    "GWU_GETTCB: Building index information\n");
            }
	    status = gwu_xidx_build_info( tlist, gw_rcb, &ulm_rcb, err );
	    if (status != E_DB_OK)
	    {
		switch (err->err_code)
		{
		    case    E_GW0327_DMF_DEADLOCK:
			gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
			break;

		    default:
			(VOID) gwf_error(err->err_code, GWF_INTERR, 0);
			err->err_code = E_GW0219_GWU_GETTCB_ERROR;
			break;
		}
		break;
	    }
	}
	else
	{
	    tlist->gwt_idx = (GW_IDX_ENTRY *)0;
	}

	tlist->gwt_next = Gwf_facility->gwf_tcb_list;
	Gwf_facility->gwf_tcb_list = tlist;

	_VOID_ gwu_dmptcb((CS_SID)session->gws_scf_session_id);

	/*
	** The TCB has been successfully built, so copy the ULM stream
	** information to the TCB so that it can be found when it's time to
	** release the TCB. Also, copy the current memory amount remaining in
	** the stream into the TCB and reset the ulm_rcb's memleft pointer.
	*/

	STRUCT_ASSIGN_MACRO(ulm_rcb, tlist->gwt_tcb_mstream);
	tlist->gwt_tcb_memleft = memleft;
	tlist->gwt_tcb_mstream.ulm_memleft = &tlist->gwt_tcb_memleft;

	break;
    }

    /*
    ** End of gwu_gettcb processing. If status is NOT E_DB_OK, then something
    ** went wrong, so close the ULM memory stream (if it was opened).
    */
    if (status != E_DB_OK)
    {
	if ((stream_opened == TRUE) && (ulm_closestream(&ulm_rcb) != E_DB_OK))
	{
	    (VOID) gwf_error(ulm_rcb.ulm_error.err_code, GWF_INTERR, 0);
	    (VOID) gwf_error(E_GW0313_ULM_CLOSESTREAM_ERROR, GWF_INTERR, 1,
			     sizeof(*ulm_rcb.ulm_memleft),
			     ulm_rcb.ulm_memleft);
	    err->err_code = E_GW0219_GWU_GETTCB_ERROR;
	}
	tlist = NULL;	/* tell caller there was an error */
    }

    /* release lock on the list if we have it */
    if (cl_status == OK)
    {
	if (cl_status = CSv_semaphore(&Gwf_facility->gwf_tcb_lock))
	{
	    (VOID) gwf_error(cl_status, GWF_INTERR, 0);
	    (VOID) gwf_error(E_GW0331_CSV_SEMAPHORE_ERROR, GWF_INTERR, 0);
	    err->err_code = E_GW0219_GWU_GETTCB_ERROR;
	    status = E_DB_ERROR;
	}
    }

    return(tlist);
}

/*
** Name: gwu_xattr_build_info	- add iigwXX_attribute info to GW_TCB
**
** Description:
**	This routine is called from gwu_gettcb to build the attribute
**	information for the GW_TCB.
**
**	The gateway attribute information consists of an extended relation
**	entry and extended arribute information. In turn, the attribute
**	information comes from two places: the 'standard' information comes
**	from DMF via the gw_rcb->attr_array, and the 'extended' information
**	comes from the iigwXX_attribute extended system catalog.
**
**	These sources of information are assembled into memory allocated from
**	the GW_TCB's stream and pointers to the information are set in the
**	GW_TCB.
**
** Inputs:
**	tlist	    - GW_TCB address to be updated.
**	gw_rcb	    - the control block from gwu_gettcb
**	ulm_rcb	    - the memory stream information for this GW_TCB
**
** Outputs:
**	tlist	    - the GW_TCB is updated. In particular, tlist->attr is set
**		      to the address of memory which is formatted as an array
**		      of GW_ATT_ENTRY structures. Also, the extended relation
**		      tuple is read into tlist->xrel.
**
** Returns:
**	DB_STATUS
**
** History:
**  2-apr-90 (bryanp)
**    Created out of gwu_gettcb code to make modularization possible.
**  9-sep-92 (robf)
**    Check attid on xattribute to prevent an AV if bad catalog attid.
**   8-oct-92 (daveb)
**    prototyped.  Cast db_id arg to gwu_copen; it's a PTR.
**    removed unused var err_code.
**  6-oct-92 (robf)
**    Check for E_DM0055_NONEXT on GET operation.
**  18-jan-93 (schang)
**    remove dmr.dmr_access_id = dmt.dmt_record_access_id; rhs not initalized
*/
static DB_STATUS
gwu_xattr_build_info( GW_TCB	    *tlist,
		     GW_RCB	    *gw_rcb,
		     ULM_RCB	    *ulm_rcb,
		     DB_ERROR    *err )
{
    DB_STATUS	    status = E_DB_OK;
    DMT_CB	    dmt;
    DMR_CB	    dmr;
    GW_SESSION	    *session = (GW_SESSION *)gw_rcb->gwr_session_id;
    char	    *ptr;	    /* NOT a 'PTR'! */
    DMT_ATT_ENTRY   *tmpattr;
    i4		    i;
    i4		    column_count;
    DB_TAB_ID	    base_tab_id;    /* needed for gateway secondary indexes */
    i4		    xattlen;
    DMR_ATTR_ENTRY  attkey[2];
    DMR_ATTR_ENTRY  *attkey_ptr[2];
    GW_EXTCAT_HDR   *ext_header;
    i4		    xcol;

    for (;;)
    {
	/*
	** The attribute information is kept in two arrays, an array of
	** GW_ATT_ENTRY blocks, one for each attribute, and an array or
	** extended attribute information. Each GW_ATT_ENTRY block points to
	** its extended attribute information -- this pointer is set up after
	** we allocate the extended attribute array.
	*/
	ulm_rcb->ulm_psize = (sizeof(GW_ATT_ENTRY) *
		(gw_rcb->gwr_table_entry->tbl_attr_count + 1) );
	if ((status = ulm_palloc(ulm_rcb)) != E_DB_OK)
	{
	    (VOID) gwf_error(ulm_rcb->ulm_error.err_code, GWF_INTERR, 0);
	    (VOID) gwf_error(E_GW0314_ULM_PALLOC_ERROR, GWF_INTERR, 0);
	    err->err_code = E_GW021A_GWU_XATTR_BLD_ERROR;
	    break;
	}
	tlist->gwt_attr = (GW_ATT_ENTRY *)ulm_rcb->ulm_pptr;

	/* initialize dmt_sequence */
/*	dmt.dmt_sequence = gw_rcb->gwr_gw_sequence;	*/

	STRUCT_ASSIGN_MACRO(gw_rcb->gwr_tab_id, base_tab_id);
	base_tab_id.db_tab_index = 0;

	/* Position for read from iigwXX_attribute table */
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
#if 0
	dmr.dmr_access_id = dmt.dmt_record_access_id;
#endif
	dmr.dmr_q_fcn = NULL;

	/* get extended relation catalog info - iigwX_relation */
	if ((status = gwu_copen(&dmt, &dmr, session->gws_scf_session_id, 
		(PTR)gw_rcb->gwr_database_id, 
		gw_rcb->gwr_gw_id,
		gw_rcb->gwr_xact_id,
		&gw_rcb->gwr_tab_id, 
		&session->gws_gw_info[gw_rcb->gwr_gw_id].gws_xrel_tab_name,
		DMT_A_READ, TRUE, err)) 
	    != E_DB_OK)
	{
	    switch (err->err_code)
	    {
		case    E_GW0327_DMF_DEADLOCK:
		    err->err_code = E_GW0327_DMF_DEADLOCK;
		    break;

		default:
		    (VOID) gwf_error(err->err_code, GWF_INTERR, 0);
		    err->err_code = E_GW021A_GWU_XATTR_BLD_ERROR;
		    break;
	    }
	    break;
	}

	dmr.dmr_flags_mask = DMR_NEXT;
	STRUCT_ASSIGN_MACRO(tlist->gwt_xrel, dmr.dmr_data);
	dmr.dmr_char_array.data_address = 0;

	/* search for the right tuple */
	do
	{
	    status = (*Dmf_cptr)(DMR_GET, &dmr);
	} while (status == E_DB_OK
	     &&
	     MEcmp((PTR)tlist->gwt_xrel.data_address,
		       (PTR)&gw_rcb->gwr_tab_id,
		       sizeof(DB_TAB_ID)));

        if ((status!=E_DB_OK) && (dmr.error.err_code!=E_DM0055_NONEXT))
	{
	    switch (dmt.error.err_code)
	    {
		case    E_DM0042_DEADLOCK:
		    err->err_code = E_GW0327_DMF_DEADLOCK;
		    break;

		default:
		    (VOID) gwf_error(dmr.error.err_code, GWF_INTERR, 0);
		    (VOID) gwf_error(E_GW0321_DMR_GET_ERROR, GWF_INTERR, 2,
			    (i4)4, (PTR)&gw_rcb->gwr_tab_id.db_tab_base,
			    (i4)4, (PTR)&gw_rcb->gwr_tab_id.db_tab_index);
		    err->err_code = E_GW021A_GWU_XATTR_BLD_ERROR;
		    break;
	    }
	    break;
	}

	if ((status = (*Dmf_cptr)(DMT_CLOSE, &dmt)) != E_DB_OK)
	{
	    switch (dmt.error.err_code)
	    {
		case    E_DM0042_DEADLOCK:
		    err->err_code = E_GW0327_DMF_DEADLOCK;
		    break;

		default:
		    (VOID) gwf_error(dmt.error.err_code, GWF_INTERR, 0);
		    (VOID) gwf_error(E_GW0325_DMT_CLOSE_ERROR, GWF_INTERR, 2,
				     sizeof(dmt.dmt_id.db_tab_base),
				     &dmt.dmt_id.db_tab_base,
				     sizeof(dmt.dmt_id.db_tab_index),
				     &dmt.dmt_id.db_tab_index);
		    err->err_code = E_GW021A_GWU_XATTR_BLD_ERROR;
		    break;
	    }
	    break;
	}

	/* Allocate array of attributes, then initialize attribute info */
	ulm_rcb->ulm_psize =
	    (Gwf_facility->gwf_gw_info[gw_rcb->gwr_gw_id].gwf_xatt_sz) *
	    (gw_rcb->gwr_table_entry->tbl_attr_count + 1 );
	if ((status = ulm_palloc(ulm_rcb)) != E_DB_OK)
	{
	    (VOID) gwf_error(ulm_rcb->ulm_error.err_code, GWF_INTERR, 0);
	    (VOID) gwf_error(E_GW0314_ULM_PALLOC_ERROR, GWF_INTERR, 0);
	    err->err_code = E_GW021A_GWU_XATTR_BLD_ERROR;
	    break;
	}
	ptr = ulm_rcb->ulm_pptr;

	tmpattr = (DMT_ATT_ENTRY *)gw_rcb->gwr_attr_array.ptr_address;
	xattlen = Gwf_facility->gwf_gw_info[gw_rcb->gwr_gw_id].gwf_xatt_sz;

	/*
	** We now walk both arrays at once (gw_rcb->attr_array and tlist->attr)
	** and copy the the attr_array information to the corresponding
	** gw_tcb.attr[] member. We also set up the pointer to the memory for
	** the extended attribute information.
	*/
	for (i = 0; i <= tlist->gwt_table.tbl_attr_count; i++)
	{
	    if (i < tlist->gwt_table.tbl_attr_count)
	    {
		/* Last entry is only a temporary buffer, no attribute entry */
		STRUCT_ASSIGN_MACRO(*tmpattr, tlist->gwt_attr[i].gwat_attr);
		tmpattr++;
	    }
	    tlist->gwt_attr[i].gwat_xattr.data_address = ptr;
	    tlist->gwt_attr[i].gwat_xattr.data_in_size = xattlen;
	    ptr += xattlen;
	}

	/* initialize dmt_sequence */
/*	dmt.dmt_sequence = gw_rcb->gwr_gw_sequence;	*/

	/* Position for get from iigwXX_attribute table */
	attkey[0].attr_number = 1;
	attkey[0].attr_operator = DMR_OP_EQ;
	attkey[0].attr_value = (char *)&base_tab_id.db_tab_base;
	attkey[1].attr_number = 2;
	attkey[1].attr_operator = DMR_OP_EQ;
	attkey[1].attr_value = (char *)&base_tab_id.db_tab_index;

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

	/* fetch iigwX_attribute info via DMF into xattr fields */
	/* get extended relation catalog info - iigwX_relation */
	if ((status = gwu_copen(&dmt, &dmr, session->gws_scf_session_id, 
	      (PTR)gw_rcb->gwr_database_id, gw_rcb->gwr_gw_id, 
	      gw_rcb->gwr_xact_id,
	      &gw_rcb->gwr_tab_id,
	      &session->gws_gw_info[gw_rcb->gwr_gw_id].gws_xatt_tab_name,
	      DMT_A_READ, TRUE, err)) != E_DB_OK)
	{
	    switch (err->err_code)
	    {
		case    E_GW0327_DMF_DEADLOCK:
		    err->err_code = E_GW0327_DMF_DEADLOCK;
		    break;

		default:
		    (VOID) gwf_error(err->err_code, GWF_INTERR, 0);
		    err->err_code = E_GW021A_GWU_XATTR_BLD_ERROR;
		    break;
	    }
	    break;
	}

	/* search for the right tuples */
	{
	    dmr.dmr_flags_mask = DMR_NEXT;
	    column_count = tlist->gwt_table.tbl_attr_count;
	    ext_header = (GW_EXTCAT_HDR *)
		tlist->gwt_attr[column_count].gwat_xattr.data_address;
	    STRUCT_ASSIGN_MACRO(tlist->gwt_attr[column_count].gwat_xattr,
				dmr.dmr_data);
	    dmr.dmr_char_array.data_address = 0;

	    i = 0;
	    do
	    {
		if ((status = (*Dmf_cptr)(DMR_GET, &dmr)) == E_DB_OK)
		{
		    if (!MEcmp((PTR)&ext_header->gwxt_tab_id,
			  (PTR)&base_tab_id,
			  sizeof(DB_TAB_ID)))
		    {
			/* move to its assigned slot */
			xcol = ext_header->gwxt_fld3.gwxt_xatt_id-1;
			/*
			**	Check fits in the array (could get AV
			**	otherwise).
			*/
			if( xcol<0 || xcol >=column_count)
			{
				/*
				**	This skips the attribute since
				**	we don't know where to put it.
				**	This should get an error below and
				**	fall through
				*/
				(VOID) gwf_error(E_GW0220_GWU_BAD_XATTR_OFFSET, GWF_INTERR, 2,
					    (i4)sizeof(ext_header->gwxt_fld3.gwxt_xatt_id),
					    (PTR)&ext_header->gwxt_fld3.gwxt_xatt_id,
					    (i4)sizeof(column_count),
					    (PTR)&column_count);
			}
			else
			{
				MEcopy(dmr.dmr_data.data_address,
				       dmr.dmr_data.data_in_size,
				       tlist->gwt_attr[xcol].gwat_xattr.data_address);
				i++;		
			}
		    }
		}
	    } while (status == E_DB_OK);

	    if ((status != E_DB_OK)
		&&
		(dmr.error.err_code != E_DM0055_NONEXT))
	    {
		switch (dmt.error.err_code)
		{
		    case    E_DM0042_DEADLOCK:
			err->err_code = E_GW0327_DMF_DEADLOCK;
			break;

		    default:
			(VOID) gwf_error(dmr.error.err_code, GWF_INTERR, 0);
			(VOID) gwf_error(E_GW0321_DMR_GET_ERROR, GWF_INTERR, 2,
				    (i4)4,
				    (PTR)&gw_rcb->gwr_tab_id.db_tab_base,
				    (i4)4,
				    (PTR)&gw_rcb->gwr_tab_id.db_tab_index);
			err->err_code = E_GW021A_GWU_XATTR_BLD_ERROR;
			break;
		}
		break;
	    }

	    if ((status = (*Dmf_cptr)(DMT_CLOSE, &dmt)) != E_DB_OK)
	    {
		switch (dmt.error.err_code)
		{
		    case    E_DM0042_DEADLOCK:
			err->err_code = E_GW0327_DMF_DEADLOCK;
			break;

		    default:
			(VOID) gwf_error(dmt.error.err_code, GWF_INTERR, 0);
			(VOID) gwf_error(E_GW0325_DMT_CLOSE_ERROR, GWF_INTERR,
					 2, sizeof(dmt.dmt_id.db_tab_base),
					 &dmt.dmt_id.db_tab_base,
					 sizeof(dmt.dmt_id.db_tab_index),
					 &dmt.dmt_id.db_tab_index);
			err->err_code = E_GW021A_GWU_XATTR_BLD_ERROR;
			break;
		}
		break;
	    }
	
	    if (i != column_count)
	    {
		(VOID) gwf_error(E_GW062F_WRONG_NUM_ATTRIBUTES, GWF_INTERR, 0);
		err->err_code = E_GW021A_GWU_XATTR_BLD_ERROR;
		status = E_DB_ERROR;
	        break;
	    }
	}
	/*
	** Successfully constructed the extended attribute information:
	*/
	status = E_DB_OK;
	err->err_code = E_DB_OK;
	break;
    }
    return (status);
}

/*
** Name: gwu_xidx_build_info	- add iigwXX_index info to GW_TCB
**
** Description:
**	This routine is called from gwu_gettcb to build the index information
**	for the GW_TCB.
**
**	If this TCB describes an index, then tlist->idx is set to contain a
**	pointer to the index description for the index being referenced.  This
**	field is NULL if this TCB refers to a base table (or if this gateway
**	does not use extended index information, in which case this routine is
**	not even called).
**
** Inputs:
**	tlist	    - GW_TCB address to be updated.
**	gw_rcb	    - the control block from gwu_gettcb
**	ulm_rcb	    - the memory stream information for this GW_TCB
**
** Outputs:
**	tlist	    - the GW_TCB is updated.
**
** Returns:
**	DB_STATUS
**
** History:
**	2-apr-90 (bryanp)
**	    Created out of gwu_gettcb code to make modularization possible.
**	8-oct-92 (daveb)
**	    prototyped.  Cast db_id arg to gwu_copen.  It's a PTR.
**	    removed dead var, err_code.
**  18-jan-93 (schang)
**    remove dmr.dmr_access_id = dmt.dmt_record_access_id; rhs not initalized
**	14-Jan-2004 (jenjo02)
**	    Check db_tab_index > 0 for index, not simply != 0.
*/
static DB_STATUS
gwu_xidx_build_info( GW_TCB	*tlist,
		    GW_RCB	*gw_rcb,
		    ULM_RCB	*ulm_rcb,
		    DB_ERROR	*err )
{
    DB_STATUS	    status = E_DB_OK;
    DMT_CB	    dmt;
    DMR_CB	    dmr;
    GW_SESSION	    *session = (GW_SESSION *)gw_rcb->gwr_session_id;
    char	    *ptr;	    /* NOT a 'PTR'! */
    DMT_IDX_ENTRY   *tmpidx;
    i4		    i;
    DMR_ATTR_ENTRY  attkey[2];
    DMR_ATTR_ENTRY  *attkey_ptr[2];
    bool	    have_index = FALSE;

    for (;;)
    {
	/*
	** If this is a TCB for a secondary index, initialize the secondary
	** index description.  We actually don't want all the descriptions,
	** just the one for the secondary index we are opening.  Note that all
	** gateway objects do not have to have index information.
	*/
	if (gw_rcb->gwr_tab_id.db_tab_index > 0)
	{
	    /*
	    ** Set up an index descriptor to be attached to the TCB we are
	    ** currently forming. This is an index access and we need the
	    ** description of the key fields and the key reference for RMS.
	    */
	    ulm_rcb->ulm_psize = (sizeof(GW_IDX_ENTRY));
	    if ((status = ulm_palloc(ulm_rcb)) != E_DB_OK)
	    {
		(VOID) gwf_error(ulm_rcb->ulm_error.err_code, GWF_INTERR, 0);
		(VOID) gwf_error(E_GW0314_ULM_PALLOC_ERROR, GWF_INTERR, 0);
		err->err_code = E_GW021B_GWU_XIDX_BLD_ERROR;
		break;
	    }
	    tlist->gwt_idx = (GW_IDX_ENTRY *)ulm_rcb->ulm_pptr;

	    /* Allocate index info, then initialize index info */
	    ulm_rcb->ulm_psize =
		(Gwf_facility->gwf_gw_info[gw_rcb->gwr_gw_id].gwf_xidx_sz);
	    if ((status = ulm_palloc(ulm_rcb)) != E_DB_OK)
	    {
		(VOID) gwf_error(ulm_rcb->ulm_error.err_code, GWF_INTERR, 0);
		(VOID) gwf_error(E_GW0314_ULM_PALLOC_ERROR, GWF_INTERR, 0);
		err->err_code = E_GW021B_GWU_XIDX_BLD_ERROR;
		break;
	    }
	    ptr = ulm_rcb->ulm_pptr;

	    /*
	    ** Look through the key array until find entry for this index, then
	    ** copy index information into tlist->idx.
	    */
	    tmpidx = (DMT_IDX_ENTRY *)gw_rcb->gwr_key_array.ptr_address;
	    for (i = 0; i < tlist->gwt_table.tbl_index_count; i++)
	    {
		if (!MEcmp((PTR)&gw_rcb->gwr_tab_id, (PTR)&tmpidx->idx_id,
			   sizeof(DB_TAB_ID)))
		{		
		    /* Save this index descriptor */
		    STRUCT_ASSIGN_MACRO(*tmpidx, tlist->gwt_idx->gwix_idx);
		    tlist->gwt_idx->gwix_xidx.data_address = ptr;
		    tlist->gwt_idx->gwix_xidx.data_in_size =
						ulm_rcb->ulm_psize;
		    have_index = TRUE;
		    break;
		}
		tmpidx++;
	    }

	    if (have_index == FALSE)
	    {
		/* issue error here */
		(VOID) gwf_error(E_GW0633_CANT_FIND_INDEX, GWF_INTERR, 0);
		err->err_code = E_GW021B_GWU_XIDX_BLD_ERROR;
		status = E_DB_ERROR;
		break;
	    }

	    /* initialize dmt_sequence */
/*	    dmt.dmt_sequence = gw_rcb->gwr_gw_sequence; */

	    /* Position for read from iigwXX_index table */
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
#if 0
	    dmr.dmr_access_id = dmt.dmt_record_access_id;
#endif
	    dmr.dmr_q_fcn = NULL;

	    /*
	    ** For this index entry, retrieve the index descriptor from the
	    ** iigwXX_index catalog.
	    */
	    if ((status = gwu_copen(&dmt, &dmr, session->gws_scf_session_id, 
		(PTR)gw_rcb->gwr_database_id, gw_rcb->gwr_gw_id,
		gw_rcb->gwr_xact_id,
		&gw_rcb->gwr_tab_id,
		&session->gws_gw_info[gw_rcb->gwr_gw_id].gws_xidx_tab_name,
		DMT_A_READ, TRUE, err)) != E_DB_OK)
	    {   
		switch (err->err_code)
		{
		    case    E_GW0327_DMF_DEADLOCK:
			err->err_code = E_GW0327_DMF_DEADLOCK;
			break;

		    default:
			(VOID) gwf_error(err->err_code, GWF_INTERR, 0);
			err->err_code = E_GW021B_GWU_XIDX_BLD_ERROR;
			break;
		}
		break;
	    }

	    /*
	    ** Search for the right tuple.  The right row is the one with
	    ** TAB_ID matching the one in gw_rcb.
	    */
	    {
		DB_TAB_ID	*tab_id;
		bool		have_idx = FALSE;
		DB_STATUS	close_status;

		dmr.dmr_flags_mask = DMR_NEXT;
		tab_id = (DB_TAB_ID *)&tlist->gwt_idx->gwix_idx.idx_id;
		STRUCT_ASSIGN_MACRO(tlist->gwt_idx->gwix_xidx, dmr.dmr_data);
		dmr.dmr_char_array.data_address = 0;

		status = E_DB_OK;
		while (status == E_DB_OK)
		{
		    if ((status = (*Dmf_cptr)(DMR_GET, &dmr)) == E_DB_OK)
		    {
			if (!MEcmp((PTR)tab_id,
				   (PTR)dmr.dmr_data.data_address,
				   sizeof(DB_TAB_ID)))
			{
			    /*
			    ** We have the correct tuple.
			    */
			    have_idx = TRUE;
			    break;
			}
		    }	/*  do this until no more tuples */
		}

		if ((status!=E_DB_OK) && (dmr.error.err_code!=E_DM0055_NONEXT))
		{
		    switch (dmt.error.err_code)
		    {
			case    E_DM0042_DEADLOCK:
			    err->err_code = E_GW0327_DMF_DEADLOCK;
			    break;

			default:
			    (VOID) gwf_error(dmr.error.err_code, GWF_INTERR, 0);
			    (VOID) gwf_error(E_GW0321_DMR_GET_ERROR,
				    GWF_INTERR, 2, (i4)4,
				    (PTR)&gw_rcb->gwr_tab_id.db_tab_base,
				    (i4)4,
				    (PTR)&gw_rcb->gwr_tab_id.db_tab_index);
			    err->err_code = E_GW021B_GWU_XIDX_BLD_ERROR;
			    break;
		    }
		    break;
		}
		else if (have_idx == FALSE)
		{
		    (VOID) gwf_error(E_GW0633_CANT_FIND_INDEX, GWF_INTERR, 0);
		    err->err_code = E_GW021B_GWU_XIDX_BLD_ERROR;
		    break;
		}


		if ((close_status = (*Dmf_cptr)(DMT_CLOSE, &dmt)) != E_DB_OK)
		{
		    switch (dmt.error.err_code)
		    {
			case    E_DM0042_DEADLOCK:
			    err->err_code = E_GW0327_DMF_DEADLOCK;
			    break;

			default:
			    (VOID) gwf_error(dmt.error.err_code, GWF_INTERR, 0);
			    (VOID) gwf_error(E_GW0325_DMT_CLOSE_ERROR,
					     GWF_INTERR, 2,
					     sizeof(dmt.dmt_id.db_tab_base),
					     &dmt.dmt_id.db_tab_base,
					     sizeof(dmt.dmt_id.db_tab_index),
					     &dmt.dmt_id.db_tab_index);
			    err->err_code = E_GW021B_GWU_XIDX_BLD_ERROR;
			    break;
		    }
		    if (status == E_DB_OK)
			status = close_status;
		    break;
		}
	    }
	    break;
	}   /* end of processing for 2-ary index info. */
	else
	{
	    /*
	    ** Successfully constructed the extended attribute information:
	    */
	    status = E_DB_OK;
	    err->err_code = E_DB_OK;
	    break;
	}
    }

    return (status);
}

/*
**	gwu_dmptcb - Dump the current TCB list
**
**	Only dumped if main trace point 10 is on
**
**    History:
**	8-oct-92 (daveb)
**	    prototyped
**	8-sep-93 (swm)
**	    Changed sess type from i4 to CS_SID to match recent CL
**	    interface revision.
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
*/
static VOID
gwu_dmptcb(CS_SID sess)
{
    GW_TCB  *tlist = Gwf_facility->gwf_tcb_list;

    if( !GWF_MAIN_MACRO(10))
	return;

    TRdisplay("Gateway TCB List\n");
    for (;;)
    {
	if (!tlist)
	{
	    TRdisplay("\tSession (%x) -- NULL tcb list\n", sess);
	    break;
	}
	else while (tlist)
	{
	    TRdisplay("\ttlist = %p, tab_id = (%d,%d)\n",
		tlist, tlist->gwt_table.tbl_id.db_tab_base,
		tlist->gwt_table.tbl_id.db_tab_index);
	    tlist = tlist->gwt_next;
	}
	break;
    }
}
/*{
** Name: gwu_validgw 
**
** Description:
**	This function checks if a particular gateway id is valid for
**	this server.
**
** Inputs:
**	gw_rcb			GWF request control block
**		gw_id
**
** Output:
**
**      Returns:
**          E_DB_OK             Function completed normally, gateway
**			        IS linked with current server.
**	    E_DB_WARN		Function completed normally, gateway is
**				NOT linked with current server.
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
** History:
**	18-dec-1992 (robf)
**	    Created
*/
DB_STATUS
gwu_validgw(GW_RCB *gw_rcb)
{
    DB_STATUS	status = E_DB_OK;
    GWX_RCB     gwx_rcb;
    i4  gw_id=gw_rcb->gwr_gw_id;


    for (;;)
    {
	if ((gw_rcb->gwr_gw_id <= 0) ||
	    (gw_rcb->gwr_gw_id >= GW_GW_COUNT)) 
	{
	    (VOID) gwf_error(E_GW0400_BAD_GWID, (i4)0, GWF_INTERR, 0);
	    status = E_DB_ERROR;
	    break;
	}
	
	if (Gwf_facility->gwf_gw_info[gw_rcb->gwr_gw_id].gwf_gw_exist == 0)
	{
		/*
		**	Gateway doesn't exist
		*/
		status = E_DB_WARN;
		break;
	}
	else
	{
		/*
		**	Gateway exists
		*/
		status = E_DB_OK;
		break;
	}
    }
    return(status);
}
