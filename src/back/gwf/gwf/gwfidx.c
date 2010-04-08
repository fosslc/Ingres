/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <pc.h>
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
#include    <gwf.h>
#include    <gwfint.h>
#include    <gwftrace.h>

#if defined(hp3_us5)
#pragma OPT_LEVEL 1
#endif /* hp3_us5 */

/**
** Name: GWFIDX.C - generic gateway index functions.
**
** Description:
**	This file contains the index functions for the Gateway facility.
**
**      This file defines the following external functions:
**    
**	gwi_register() - register index to extended catalog
**	gwi_remove() - remove table registration from extended catalog
**
**      This file defines the following static functions:
**    
**	gwi_map_idx_rel()   -	create entry in iigwX_relation for the index
**	gwi_map_idx_att()   -	create entries in iigwX_attribute for the index
**
** History:
**  09-apr-90 (linda)
**	Created -- broken out from gwf.c, which is now obsolete.
**  16-apr-90 (linda)
**	Added a parameter in call to gwu_deltcb() indicating whether the tcb
**	to be removed must exist or not.
**	18-apr-90 (bryanp)
**	    Added code to re-map exit errors to generic GWF errors. Also
**	    updated the memory allocation code to properly return NO_MEM when
**	    out of memory.
**      18-may-92 (schang)
**          GW merge.
**	    4-feb-91 (linda)
**	     (1) Code cleanup:  reflect new structure member names; correct code
**	     to address unix compiler warnings; (2) deadlock handling; (3)
**	     add support for gateway secondary indexes, including new routines
**	     gwi_map_idx_rel() and gwi_map_idx_att(); (4) keyed access to
**	     extended system catalogs to minimize contention.
**	    17-jun-91 (johnr)
**	     C1 internal compiler error in gwi_map_idx_att.  Compile with 
**	     pragma OPT_LEVEL 1.
**    8-oct-92 (daveb)
**        prototyped
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      23-jul-93 (schang)
**          add me.h
**	27-jul-93 (ralph)
**	    DELIM_IDENT:
**	    Use gws_xrel_tab_name instead of gwf_xrel_tab_name.
**	    Use gws_xatt_tab_name instead of gwf_xatt_tab_name.
**	    Use gws_xidx_tab_name instead of gwf_xidx_tab_name.
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
*/
/* forward refs */
static
DB_STATUS gwi_map_idx_rel( GW_RCB       *gw_rcb,
                          DMT_CB        *dmt,
                          DMR_CB        *dmr,
                          ULM_RCB       *ulm_rcb );


static
DB_STATUS gwi_map_idx_att(GW_RCB        *gw_rcb,
                          ULM_RCB       *ulm_rcb );


/*
** Name: gwi_register - register a gateway index
**
** Description:
**	This function registers extended gateway  index catalog information.  A
**	DMF external interface is used to update, namely put, the extended
**	gateway catalog,  iigwX_index.  This request  must be made before
**	gateway index registration can be considered complete.
**
**	The GWF tcb cache for the base table is discarded since it is no longer
**	current.
**
** Inputs:
**	gw_rcb->		gateway request control block
**		session_id	an established session id
**		dmu->
**		  dmu_db_id	database id
**		  dmu_tran_id	transaction id
**		  dmu_tbl_id	base table id
**		  dmu_idx_id	INGRES index id
**		gw_id		gateway id, derived from DBMS type
**		in_vdata2	source of the gateway index.
**
** Output:
**	gw_rcb->
**		error.err_code	One of the following error numbers.
**                              E_GW0600_NO_MEM
**				E_GW0400_BAD_GWID
**				E_GW0208_GWI_REGISTER_ERROR
**				E_GW0006_XIDX_OPEN_FAILED
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
** History:
**      28-Apr-1989 (alexh)
**          Created.
**	2-apr-90 (bryanp)
**	    If gateway has no extended system catalog, nothing to do.
**	09-apr-90 (linda)
**	    Add error handling and single return point.
**	18-apr-90 (bryanp)
**	    Return E_GW0600_NO_MEM when out of ULM memory.
**	1-jul-91 (rickh)
**	    If the gateway exit returns an error, don't clobber the status
**	    word when we close the stream!  Also, if the exit has already
**	    reported the problem, spare us the uninformative yet alarming
**	    error-babble of higher facilities.
**	7-oct-92 (robf)
**	    Still create Xrelation & Xattribute entries even if no Xindex
**	    table - a gateway may need rel/attr entires but not the
**	    index one
**      7-oct-92 (daveb)
**          Hand the exit the session pointer, in case it wants to add
**          an exit-private SCB.  Prototyped.  Removed dead vars xrel_sz
**          and sav_idx_id.  Cast db_id arg to gwu_copen; it's a PTR.
**	17-nov-92 (robf)
**	    Changed calls to gwf_display() to use array rather than
**	    arg list.
*/
DB_STATUS
gwi_register(GW_RCB *gw_rcb)
{
    GWX_RCB	gwx;
    ULM_RCB	ulm_rcb;
    GW_SESSION	*session = (GW_SESSION *)gw_rcb->gwr_session_id;
    DMT_CB	dmt;
    DMR_CB	dmr;
    i4     gw_id = gw_rcb->gwr_gw_id;
    i4          xidx_sz = Gwf_facility->gwf_gw_info[gw_id].gwf_xidx_sz;
    DB_STATUS	status;
    DB_STATUS	ulm_status;
    DB_ERROR	err;
    char	trbuf[GWF_MAX_MSGLEN + 1]; /* last char for `\n' */

    if(GWF_MAIN_MACRO(12))
    {
	gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
	    "GWI_REGISTER: REGISTER INDEX entry\n");
    }
    for (;;)
    {
	if ((gw_id <= 0) || (gw_id >= GW_GW_COUNT) ||
	    (Gwf_facility->gwf_gw_info[gw_id].gwf_gw_exist == 0))
	{
	    (VOID) gwf_error(E_GW0400_BAD_GWID, (i4)0, GWF_INTERR, 1,
			     sizeof(gw_id), &gw_id);
	    gw_rcb->gwr_error.err_code = E_GW0208_GWI_REGISTER_ERROR;
	    status = E_DB_ERROR;
	    break;
	}
	/*
	** Allocate a memory stream for the life of this operation.  Used as
	** storage for temporary structures.  Note that it is allocated from
	** the session's GWF memory stream.
	*/
	STRUCT_ASSIGN_MACRO(session->gws_ulm_rcb, ulm_rcb);
	ulm_rcb.ulm_blocksize = xidx_sz;
	if ((status = ulm_openstream(&ulm_rcb)) != E_DB_OK)
	{
		(VOID) gwf_error(ulm_rcb.ulm_error.err_code, GWF_INTERR, 0);
		(VOID) gwf_error(E_GW0312_ULM_OPENSTREAM_ERROR, GWF_INTERR, 3,
			 sizeof(*ulm_rcb.ulm_memleft),
			 ulm_rcb.ulm_memleft,
			 sizeof(ulm_rcb.ulm_sizepool),
			 &ulm_rcb.ulm_sizepool,
			 sizeof(ulm_rcb.ulm_blocksize),
			 &ulm_rcb.ulm_blocksize);
		if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
			gw_rcb->gwr_error.err_code = E_GW0600_NO_MEM;
		else
			gw_rcb->gwr_error.err_code = E_GW0208_GWI_REGISTER_ERROR;
		break;
	}
	/*
	**	Only process extended index info if this gateway has an
	**	extended index catalog
	*/
	if (Gwf_facility->gwf_gw_info[gw_id].gwf_xidx_sz > 0)
	{

	    gwx.xrcb_tab_id = &gw_rcb->gwr_tab_id;
/*	    STRUCT_ASSIGN_MACRO(gw_rcb->gwr_tab_id, gwx.xrcb_tab_id);    */
	    gwx.xrcb_gw_id = gw_id;
	    STRUCT_ASSIGN_MACRO(gw_rcb->gwr_in_vdata2, gwx.xrcb_var_data2);


	    /* allocate iigwX_index tuple buffer */
	    gwx.xrcb_var_data1.data_in_size =
	        ulm_rcb.ulm_psize = ulm_rcb.ulm_blocksize; 
	    if ((status = ulm_palloc(&ulm_rcb)) != E_DB_OK)
	    {
	        (VOID) gwf_error(ulm_rcb.ulm_error.err_code, GWF_INTERR, 0);
	        (VOID) gwf_error(E_GW0314_ULM_PALLOC_ERROR, GWF_INTERR, 4,
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
	    	gw_rcb->gwr_error.err_code = E_GW0208_GWI_REGISTER_ERROR;
	        break;
	    }
	    gwx.xrcb_var_data1.data_address = ulm_rcb.ulm_pptr;

	    /* request exit format */
	    if ((status =
	    	(*Gwf_facility->gwf_gw_info[gw_id].gwf_gw_exits	[GWX_VIDXF])
	    	    (&gwx)) !=  E_DB_OK)
	    {
	        if ( gwx.xrcb_error.err_code == E_GW050E_ALREADY_REPORTED )
	        {
			gw_rcb->gwr_error.err_code = E_GW050E_ALREADY_REPORTED;
	        }
	        else
	        {
			(VOID) gwf_error(gwx.xrcb_error.err_code, GWF_INTERR, 0);
			gw_rcb->gwr_error.err_code = E_GW0208_GWI_REGISTER_ERROR;
	        }

	        if ((ulm_status = ulm_closestream(&ulm_rcb)) != E_DB_OK)
	        {
			(VOID) gwf_error(ulm_rcb.ulm_error.err_code, GWF_INTERR, 0);
			(VOID) gwf_error(E_GW0313_ULM_CLOSESTREAM_ERROR, GWF_INTERR, 1,
					 sizeof(*ulm_rcb.ulm_memleft),
					 ulm_rcb.ulm_memleft);
			break;
	        }

	        break;
	    }
	    else
	    {
	        /* initialize dmt_sequence */
	        dmt.dmt_sequence = gw_rcb->gwr_gw_sequence;

	        /* request DMF to append tuples */
	        status = gwu_copen(&dmt, &dmr, session->gws_scf_session_id, 
	            (PTR) gw_rcb->gwr_database_id, gw_id, gw_rcb->gwr_xact_id,
	            &gw_rcb->gwr_tab_id,
		    &session->gws_gw_info[gw_id].gws_xidx_tab_name,
	            DMT_A_WRITE, FALSE, &err);

	        /* put the iigwX_index tuple */
	        if ( status == E_DB_OK)
	        {
	    	STRUCT_ASSIGN_MACRO(gwx.xrcb_var_data1, dmr.dmr_data);
	    	if ((status = (*Dmf_cptr)(DMR_PUT, &dmr)) == E_DB_OK)
	    	{
	    	    status = (*Dmf_cptr)(DMT_CLOSE, &dmt);
	    	    if (status != E_DB_OK)
	    	    {
	    		/* Handle error from close */
	    		(VOID) gwf_error(dmt.error.err_code, GWF_INTERR, 0);
	    		gw_rcb->gwr_error.err_code =
	    			    E_GW0208_GWI_REGISTER_ERROR;
	    		break;
	    	    }
	    	}
	    	else
	    	{
	    	    /* Handle error from put */
	    	    switch (dmt.error.err_code)
	    	    {
	    		case    E_DM0042_DEADLOCK:
	    		    gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
	    		    break;

	    		default:
	    		    (VOID) gwf_error(dmt.error.err_code, GWF_INTERR, 0);
	    		    gw_rcb->gwr_error.err_code =
	    				E_GW0208_GWI_REGISTER_ERROR;
	    		    break;
	    	    }
	    	    break;
	    	}
	        }
	        else
	        {
			/* Handle error from gwu_copen() */
			(VOID) gwf_error(err.err_code, GWF_INTERR, 0);
			gw_rcb->gwr_error.err_code = E_GW0208_GWI_REGISTER_ERROR;
			break;
	        }

	        /* 
	        **	delete tcb from GWF cache since its index info cannot be
	        **	current.
	        **
	        **** Not clear that we care about this. We probably can't remove
	        **** an index if it is being used and it is only referenced in its
	        **** own tcb.
	        */
#if 0
	        /*
	        ** Save the index's table id, set db_tab_index to 0 for the call.
	        */
	        _VOID_ MEcopy((PTR)&gw_rcb->gwr_tab_id, sizeof(DB_TAB_ID),
	    		  (PTR)&sav_idx_id);
	        gw_rcb->gwr_tab_id.db_tab_index = 0;

	        if ((status = gwu_deltcb(gw_rcb, GWU_TCB_OPTIONAL, &err))
	    	    != E_DB_OK)
	        {
			(VOID) gwf_error(err.err_code, GWF_INTERR, 0);
			gw_rcb->gwr_error.err_code = E_GW0208_GWI_REGISTER_ERROR;
			_VOID_ MEcopy((PTR)&sav_idx_id, sizeof(DB_TAB_ID),
	    		      (PTR)&gw_rcb->gwr_tab_id);
			break;
	        }
	        _VOID_ MEcopy((PTR)&sav_idx_id, sizeof(DB_TAB_ID),
	    		  (PTR)&gw_rcb->gwr_tab_id);
#endif	    /* 0 */
	    }
	} /* End if Xindex processing */

	/*
	** Append a tuple to iigwX_relation catalog for this index.
	*/
	if ((status = gwi_map_idx_rel(gw_rcb, &dmt, &dmr, &ulm_rcb)) != E_DB_OK)
	{
	    _VOID_ gwf_error(gw_rcb->gwr_error.err_code, GWF_INTERR, 0);
	    gw_rcb->gwr_error.err_code = E_GW0208_GWI_REGISTER_ERROR;
	    break;
	}

	/*
	** Now append iigwX_attribute tuples.
	*/

	if ((status = gwi_map_idx_att(gw_rcb, &ulm_rcb)) != E_DB_OK)
	{
	    _VOID_ gwf_error(gw_rcb->gwr_error.err_code, GWF_INTERR, 0);
	    gw_rcb->gwr_error.err_code = E_GW0208_GWI_REGISTER_ERROR;
	    break;
	}

	/* close the temporary stream */
	if ((status = ulm_closestream(&ulm_rcb)) != E_DB_OK)
	{
	    (VOID) gwf_error(ulm_rcb.ulm_error.err_code, GWF_INTERR, 0);
	    (VOID) gwf_error(E_GW0313_ULM_CLOSESTREAM_ERROR, GWF_INTERR, 1,
			     sizeof(*ulm_rcb.ulm_memleft),
			     ulm_rcb.ulm_memleft);
	    gw_rcb->gwr_error.err_code = E_GW0208_GWI_REGISTER_ERROR;
	    break;
	}

	/* If we get here everything is okay. */
	break;
    }

    if (status == E_DB_OK)
	gw_rcb->gwr_error.err_code = E_DB_OK;

    return(status);
}

/*{
** Name: gwi_remove - remove a gateway index
**
** Description:
**	This function removes extended gateway index catalog information.  A
**	DMF external interface is used to update, namely delete, the extended
**	gateway index catalog, iigwX_index. This request must be made before
**	gateway index removal can be considered complete.
**
**	The GWF tcb cache for the base table is discarded since it is no longer
**	current.
**
** Inputs:
**	gw_rcb->		gateway request control block
**		session_id	an established session id
**		dmu->
**		  dmu_db_id	database id
**		  dmu_tran_id	transaction id
**		  dmu_tbl_id	base table id
**		  dmu_idx_id	INGRES index id
**		gw_id		gateway id, derived from DBMS type
**
** Output:
**	gw_rcb->
**		error.err_code	One of the following error numbers.
**                              E_GW0600_NO_MEM
**				E_GW0400_BAD_GWID
**				E_GW0006_XIDX_OPEN_FAILED
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
** History:
**      28-Apr-1989 (alexh)
**          Created.
**	2-apr-90 (bryanp)
**	    If this gateway has no extended system catalogs, nothing to do.
**	09-apr-90 (linda)
**	    Added error handling, single return point.
**	18-apr-90 (bryanp)
**	    Return E_GW0600_NO_MEM when out of ULM memory.
**	28-mar-91 (linda)
**	    Fixed 2 problems:  (a) error on tuple delete from dmf was logged
**	    but did not cause this routine to return a bad status; (b) call to
**	    DMR_DELETE required DMR_CURRENT_POS in flags mask.
**      7-oct-92 (daveb)
**          prototyped.  Cast arg to gwu_copen (db_id is PTR).
*/
DB_STATUS
gwi_remove(GW_RCB *gw_rcb)
{
    GW_SESSION	    *session = (GW_SESSION *)gw_rcb->gwr_session_id;
    DMT_CB	    dmt;
    DMR_CB	    dmr;
    ULM_RCB	    ulm_rcb;
    DM_DATA	    xidx_buff;
    i4	    gw_id = gw_rcb->gwr_gw_id;
    i4		    xidx_sz = Gwf_facility->gwf_gw_info[gw_id].gwf_xidx_sz;
    DMR_ATTR_ENTRY  attkey[2];
    DMR_ATTR_ENTRY  *attkey_ptr[2];
    DB_TAB_ID	    sav_idx_id;
    DB_ERROR	    err;
    DB_STATUS	    status;
    DB_STATUS	    sav_status;

    for (;;)
    {
	if ((gw_id <= 0) || (gw_id >= GW_GW_COUNT) ||
	    (Gwf_facility->gwf_gw_info[gw_id].gwf_gw_exist == 0))
	{
	    (VOID) gwf_error(E_GW0400_BAD_GWID, (i4)0, GWF_INTERR, 1,
			     sizeof(gw_rcb->gwr_gw_id), &gw_rcb->gwr_gw_id);
	    gw_rcb->gwr_error.err_code = E_GW0208_GWI_REGISTER_ERROR;
	    status = E_DB_ERROR;
	    break;
	}

	if (xidx_sz == 0)
	{
	    status = E_DB_OK;
	    break;
	}

	/* copy ulm_rcb */
	STRUCT_ASSIGN_MACRO(session->gws_ulm_rcb, ulm_rcb);
	xidx_buff.data_in_size = xidx_sz;
	ulm_rcb.ulm_blocksize = xidx_buff.data_in_size;

	/* allocate a iigwX_index buffer from a temporary  memory stream */
	if ((status = ulm_openstream(&ulm_rcb)) != E_DB_OK)
	{
	    (VOID) gwf_error(ulm_rcb.ulm_error.err_code, GWF_INTERR, 0);
	    (VOID) gwf_error(E_GW0312_ULM_OPENSTREAM_ERROR, GWF_INTERR, 3,
			     sizeof(*ulm_rcb.ulm_memleft),
			     ulm_rcb.ulm_memleft,
			     sizeof(ulm_rcb.ulm_sizepool),
			     &ulm_rcb.ulm_sizepool,
			     sizeof(ulm_rcb.ulm_blocksize),
			     &ulm_rcb.ulm_blocksize);
	    if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
		gw_rcb->gwr_error.err_code = E_GW0600_NO_MEM;
	    else
		gw_rcb->gwr_error.err_code = E_GW0209_GWI_REMOVE_ERROR;
	    break;
	}

	ulm_rcb.ulm_psize = ulm_rcb.ulm_blocksize;
	if ((status = ulm_palloc(&ulm_rcb)) != E_DB_OK)
	{
	    (VOID) gwf_error(ulm_rcb.ulm_error.err_code, GWF_INTERR, 0);
	    (VOID) gwf_error(E_GW0314_ULM_PALLOC_ERROR, GWF_INTERR, 4,
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
		gw_rcb->gwr_error.err_code = E_GW0209_GWI_REMOVE_ERROR;

	    if ((status = ulm_closestream(&ulm_rcb)) != E_DB_OK)
	    {
		(VOID) gwf_error(ulm_rcb.ulm_error.err_code, GWF_INTERR, 0);
		(VOID) gwf_error(E_GW0313_ULM_CLOSESTREAM_ERROR, GWF_INTERR, 1,
				 sizeof(*ulm_rcb.ulm_memleft),
				 ulm_rcb.ulm_memleft);
	    }
	    status = E_DB_ERROR;    /* reset after closestream call */
	    break;
	}

	xidx_buff.data_address = ulm_rcb.ulm_pptr;

	/* initialize dmt_sequence */
	dmt.dmt_sequence = gw_rcb->gwr_gw_sequence;

	/* set up position for iigwXX_index catalog*/
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

	/* open the iigwX_index catalog */
	if ((status = gwu_copen(&dmt, &dmr, session->gws_scf_session_id, 
		(PTR)gw_rcb->gwr_database_id, gw_id, gw_rcb->gwr_xact_id,
		&gw_rcb->gwr_tab_id,
		&session->gws_gw_info[gw_id].gws_xidx_tab_name,
		DMT_A_WRITE, TRUE, &err)) != E_DB_OK)
	{
	    switch (dmt.error.err_code)
	    {
		case    E_DM0042_DEADLOCK:
		    gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
		    break;

		default:
		    (VOID) gwf_error(err.err_code, GWF_INTERR, 0);
		    gw_rcb->gwr_error.err_code = E_GW0209_GWI_REMOVE_ERROR;
		    break;
	    }

	    if ((status = ulm_closestream(&ulm_rcb)) != E_DB_OK)
	    {
		(VOID) gwf_error(ulm_rcb.ulm_error.err_code, GWF_INTERR, 0);
		(VOID) gwf_error(E_GW0313_ULM_CLOSESTREAM_ERROR, GWF_INTERR, 1,
				 sizeof(*ulm_rcb.ulm_memleft),
				 ulm_rcb.ulm_memleft);
		gw_rcb->gwr_error.err_code = E_GW0209_GWI_REMOVE_ERROR;
	    }
	    break;
	}

	dmr.dmr_flags_mask = DMR_NEXT;
	STRUCT_ASSIGN_MACRO(xidx_buff, dmr.dmr_data);
	dmr.dmr_char_array.data_address = 0;

	/*
	**	Delete the index entry with index and base table id
	*/
	do
	{
	    status = (*Dmf_cptr)(DMR_GET, &dmr);
	}
	while (status == E_DB_OK &&
		MEcmp((PTR)dmr.dmr_data.data_address,
		      (PTR)&gw_rcb->gwr_tab_id,
		      sizeof(DB_TAB_ID))
		);

	if (status == E_DB_OK)
	{
	    dmr.dmr_flags_mask |= DMR_CURRENT_POS;
	    if ((status = (*Dmf_cptr)(DMR_DELETE, &dmr)) != E_DB_OK)
	    {
		switch (dmt.error.err_code)
		{
		    case    E_DM0042_DEADLOCK:
			gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
			break;

		    default:
			(VOID) gwf_error(dmr.error.err_code, GWF_INTERR, 0);
			(VOID) gwf_error(E_GW0322_DMR_DELETE_ERROR, GWF_INTERR,
				     2, sizeof(gw_rcb->gwr_tab_id.db_tab_base),
				     &gw_rcb->gwr_tab_id.db_tab_base,
				     sizeof(gw_rcb->gwr_tab_id.db_tab_index),
				     &gw_rcb->gwr_tab_id.db_tab_index);
			gw_rcb->gwr_error.err_code = E_GW0209_GWI_REMOVE_ERROR;
			break;
		}
		if (dmt.error.err_code == E_DM0042_DEADLOCK)
			break;
		/* (Else no break; we still want to close index catalog.) */
	    }
	}
	else	/* status != E_DB_OK */
	{
	    switch (dmt.error.err_code)
	    {
		case    E_DM0042_DEADLOCK:
		    gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
		    break;

		default:
		    (VOID) gwf_error(dmr.error.err_code, GWF_INTERR, 0);
		    (VOID) gwf_error(E_GW0321_DMR_GET_ERROR, GWF_INTERR, 2,
				     sizeof(gw_rcb->gwr_tab_id.db_tab_base),
				     &gw_rcb->gwr_tab_id.db_tab_base,
				     sizeof(gw_rcb->gwr_tab_id.db_tab_index),
				     &gw_rcb->gwr_tab_id.db_tab_index);
		    gw_rcb->gwr_error.err_code = E_GW0209_GWI_REMOVE_ERROR;
		    break;
	    }
	    if (dmt.error.err_code == E_DM0042_DEADLOCK)
		    break;
	    /* (Else no break; we still want to close index catalog.) */
	}

	sav_status = status;	/* in case of non-deadlock error above */

	if ((status = (*Dmf_cptr)(DMT_CLOSE, &dmt)) != E_DB_OK)
	{
	    switch (dmt.error.err_code)
	    {
		case    E_DM0042_DEADLOCK:
		    gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
		    break;

		default:
		    (VOID) gwf_error(dmt.error.err_code, GWF_INTERR, 0);
		    (VOID) gwf_error(E_GW0325_DMT_CLOSE_ERROR, GWF_INTERR, 2,
				     sizeof(dmt.dmt_id.db_tab_base),
				     &dmt.dmt_id.db_tab_base,
				     sizeof(dmt.dmt_id.db_tab_index),
				     &dmt.dmt_id.db_tab_index);
		    gw_rcb->gwr_error.err_code = E_GW0209_GWI_REMOVE_ERROR;
		    break;
	    }
	}

	/* 
	**  delete tcb from GWF cache since its index info cannot be current.
	*/
	/*
	** First save index's tab_id, set base to 0 for call.
	*/
	_VOID_ MEcopy((PTR)&gw_rcb->gwr_tab_id, sizeof(DB_TAB_ID),
		      (PTR)&sav_idx_id);
	gw_rcb->gwr_tab_id.db_tab_index = 0;

	if ((status = gwu_deltcb(gw_rcb, GWU_TCB_OPTIONAL, &err)) != E_DB_OK)
	{
	    (VOID) gwf_error(err.err_code, GWF_INTERR, 0);
	    gw_rcb->gwr_error.err_code = E_GW0209_GWI_REMOVE_ERROR;
	    _VOID_ MEcopy((PTR)&sav_idx_id, sizeof(DB_TAB_ID),
			  (PTR)&gw_rcb->gwr_tab_id);
	    break;
	}
	_VOID_ MEcopy((PTR)&sav_idx_id, sizeof(DB_TAB_ID),
		      (PTR)&gw_rcb->gwr_tab_id);

	if ((status = ulm_closestream(&ulm_rcb)) != E_DB_OK)
	{
	    (VOID) gwf_error(ulm_rcb.ulm_error.err_code, GWF_INTERR, 0);
	    (VOID) gwf_error(E_GW0313_ULM_CLOSESTREAM_ERROR, GWF_INTERR, 1,
			     sizeof(ulm_rcb.ulm_error.err_code),
			     &ulm_rcb.ulm_error.err_code);
	    gw_rcb->gwr_error.err_code = E_GW0209_GWI_REMOVE_ERROR;
	    break;
	}

	/*
	** If we get here, things went okay after the tuple delete.  Recheck
	** status from that call.
	*/
	if (sav_status == E_DB_OK)
	{
	    gw_rcb->gwr_error.err_code = E_DB_OK;
	}
	else
	{
	    status = sav_status;
	    gw_rcb->gwr_error.err_code = E_GW0209_GWI_REMOVE_ERROR;
	}
	break;
    }

    return(status);
}

/*{
** Name: gwi_map_idx_rel    -	create a tuple entry in the iigwX_relation
**				table for this gateway secondary index
**
** Description:
**	This function creates a tuple in the extended relation catalog for this
**	gateway secondary index.  The tuple is identical to that for its base
**	table, except that it has a non-zero xreltidx field.
**
** Inputs:
**	gw_rcb->		gateway request control block
**	    gwr_tab_id		table id for this secondary index
**	dmt_cb			DMF table request control block, passed in from
**				parent routine to avoid re-allocating
**	dmr_cb			DMF record request control block passed in from
**				parent routine to avoid re-allocating
**	ulm_rcb			memory request control block, passed in from
**				parent routine as it is in the same stream, we
**				want to deallocate all related memory from there
**
** Output:
**	gw_rcb->
**		error.err_code	One of the following error numbers.
**				E_GW0600_NO_MEM
**
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
** History:
**	24-sep-90 (linda)
**	    Created.
**	07-oct-92 (robf)
**	    If we can't find the base tuple, don't try to save an (incorrect)
**	    index tuple.
**	    Look up the base tuple, not index tuple, from relation (the
**	    index tuple doesn't exist yet, so we wouldn't expect to find it)
**	    If opening the iigwXX_relation catalog fails, break rather
**	    than trying to access an unopened table.
**	    Save errors on return so higher level knows what happened.
**	    (symptom was spurious ULEFORMAT errors on error #0)
**      8-oct-92 (daveb)
**          prototyped.  Cast db_id arg to gwu_copen.  It's a PTR.
**          removed dead var gwx.
*/
static
DB_STATUS
gwi_map_idx_rel( GW_RCB *gw_rcb,
                DMT_CB  *dmt,
                DMR_CB  *dmr,
                ULM_RCB *ulm_rcb )
{
    GW_SESSION	    *session = (GW_SESSION *)gw_rcb->gwr_session_id;
    DB_TAB_ID	    base_tab_id;
    DB_TAB_ID	    *tab_id;
    i4	    gw_id = gw_rcb->gwr_gw_id;
    i4		    xrel_sz = Gwf_facility->gwf_gw_info[gw_id].gwf_xrel_sz;
    DB_STATUS	    status = E_DB_OK;
    DB_STATUS	    ret_code=E_DB_OK;
    DMR_ATTR_ENTRY  attkey[2];
    DMR_ATTR_ENTRY  *attkey_ptr[2];
    DB_ERROR	    err;

    for (;;)	/* just to break out of on error... */
    {
        /*
        ** First create the iigwXX_relation row buffer
        */
        ulm_rcb->ulm_psize = xrel_sz;

        if ((status = ulm_palloc(ulm_rcb)) != E_DB_OK)
        {
            gwf_error(ulm_rcb->ulm_error.err_code, GWF_INTERR, 0);
            gwf_error(E_GW0314_ULM_PALLOC_ERROR, GWF_INTERR, 4,
                      sizeof(*ulm_rcb->ulm_memleft),
                      ulm_rcb->ulm_memleft,
                      sizeof(ulm_rcb->ulm_sizepool),
                      &ulm_rcb->ulm_sizepool,
                      sizeof(ulm_rcb->ulm_blocksize),
                      &ulm_rcb->ulm_blocksize,
                      sizeof(ulm_rcb->ulm_psize),
                      &ulm_rcb->ulm_psize);
            if (ulm_rcb->ulm_error.err_code == E_UL0005_NOMEM)
                gw_rcb->gwr_error.err_code = E_GW0600_NO_MEM;
            else
                gw_rcb->gwr_error.err_code = E_GW021D_GWU_MAP_IDX_REL_ERROR;
            break;
        }

	dmr->dmr_data.data_in_size = xrel_sz;
	dmr->dmr_data.data_address = ulm_rcb->ulm_pptr;
     
	/*
	** NOTE we are looking for the base table entry.
	*/
	STRUCT_ASSIGN_MACRO(gw_rcb->gwr_tab_id, base_tab_id);
	base_tab_id.db_tab_index = 0;


	/* position to get if this is a DMT_A_READ */
	attkey[0].attr_number = 1;
	attkey[0].attr_operator = DMR_OP_EQ;
	attkey[0].attr_value = (char *)&gw_rcb->gwr_tab_id.db_tab_base;
	attkey[1].attr_number = 2;
	attkey[1].attr_operator = DMR_OP_EQ;
	/*
	attkey[1].attr_value = (char *)&gw_rcb->gwr_tab_id.db_tab_index;
	*/
	attkey[1].attr_value = (char*) &base_tab_id.db_tab_index;

	attkey_ptr[0] = &attkey[0];
	attkey_ptr[1] = &attkey[1];

	dmr->type = DMR_RECORD_CB;
	dmr->length = sizeof(DMR_CB);
	dmr->dmr_flags_mask = 0;
	dmr->dmr_position_type = DMR_QUAL;
	dmr->dmr_attr_desc.ptr_in_count = 2;
	dmr->dmr_attr_desc.ptr_size = sizeof(DMR_ATTR_ENTRY);
	dmr->dmr_attr_desc.ptr_address = (PTR)&attkey_ptr[0];
	dmr->dmr_access_id = dmt->dmt_record_access_id;
	dmr->dmr_q_fcn = NULL;


	/* get extended relation catalog info - iigwX_relation */
	if ((status = gwu_copen(dmt, dmr, session->gws_scf_session_id, 
		(PTR)gw_rcb->gwr_database_id, gw_id, gw_rcb->gwr_xact_id,
		&gw_rcb->gwr_tab_id, 
		&session->gws_gw_info[gw_id].gws_xrel_tab_name,
		DMT_A_WRITE, TRUE, &err)) 
	    != E_DB_OK)
	{
	    switch (dmt->error.err_code)
	    {
		case    E_DM0042_DEADLOCK:
		    gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
		    break;

		default:
		    (VOID) gwf_error(err.err_code, GWF_INTERR, 0);
		    gw_rcb->gwr_error.err_code = E_GW021D_GWU_MAP_IDX_REL_ERROR;
		    break;
	    }
	    break;
	}

	dmr->dmr_flags_mask = DMR_NEXT;
	tab_id = (DB_TAB_ID *)dmr->dmr_data.data_address;
	STRUCT_ASSIGN_MACRO(base_tab_id, *tab_id);
	dmr->dmr_char_array.data_address = 0;

	/* search for the right tuple */
	do
	{
	    status = (*Dmf_cptr)(DMR_GET, dmr);
	} while (status == E_DB_OK
	     &&
	     MEcmp((PTR)dmr->dmr_data.data_address,
		   (PTR)&base_tab_id,
		   sizeof(DB_TAB_ID)));

	if (status != E_DB_OK)	/* cannot get table info */
	{
	    switch (dmt->error.err_code)
	    {
		case    E_DM0042_DEADLOCK:
		    err.err_code = E_GW0327_DMF_DEADLOCK;
		    break;

		default:
		    (VOID) gwf_error(dmr->error.err_code, GWF_INTERR, 0);
		    (VOID) gwf_error(E_GW0321_DMR_GET_ERROR, GWF_INTERR, 2,
			    (i4)4, (PTR)&gw_rcb->gwr_tab_id.db_tab_base,
			    (i4)4, (PTR)&gw_rcb->gwr_tab_id.db_tab_index);
		    gw_rcb->gwr_error.err_code = E_GW021D_GWU_MAP_IDX_REL_ERROR;
		    break;
	    }
	}
	/*
	**	Only save the index tuple if we found base tuple
	*/
	if (status == E_DB_OK)
	{
		/*
		** Now that we've found the tuple for the base table, 
		** update it to be that of the index.  This code assumes that 
		** the tab_id fields are the first fields in the extended 
		** relation catalog.
		*/
		tab_id = (DB_TAB_ID *)dmr->dmr_data.data_address;
		STRUCT_ASSIGN_MACRO(gw_rcb->gwr_tab_id, *tab_id);

		/* put the iigwX_relation tuple */
		/*
		** Initialize the DMR control block with the information 
		** required for an insert.
		*/
		dmr->type = DMR_RECORD_CB;
		dmr->length = sizeof(*dmr);
		dmr->dmr_data.data_in_size = xrel_sz;

		if ((status = (*Dmf_cptr)(DMR_PUT, dmr)) != E_DB_OK)
		{
		    switch (dmt->error.err_code)
		    {
			case    E_DM0042_DEADLOCK:
			    err.err_code = E_GW0327_DMF_DEADLOCK;
			    break;

			default:
			    /***note need args here to specify which catalog***/
			    gwf_error(dmr->error.err_code, GWF_INTERR, 0);
			    gwf_error(E_GW0320_DMR_PUT_ERROR, GWF_INTERR, 2,
				      sizeof(gw_rcb->gwr_tab_id.db_tab_base),
				      &gw_rcb->gwr_tab_id.db_tab_base,
				      sizeof(gw_rcb->gwr_tab_id.db_tab_index),
				      &gw_rcb->gwr_tab_id.db_tab_index);
			    gwf_error(E_GW0623_XCAT_PUT_FAILED, GWF_INTERR, 0);
			    gw_rcb->gwr_error.err_code = E_GW021D_GWU_MAP_IDX_REL_ERROR;
			    break;
		    }
		}
	}
	/*
	**	Preserve previous status in case of earlier error and
	**	OK close loosing error status.
	*/
	ret_code=status;

	if ((status = (*Dmf_cptr)(DMT_CLOSE, dmt)) != E_DB_OK)
	{
	    switch (dmt->error.err_code)
	    {
		case    E_DM0042_DEADLOCK:
		    err.err_code = E_GW0327_DMF_DEADLOCK;
		    break;

		default:
		    (VOID) gwf_error(dmt->error.err_code, GWF_INTERR, 0);
		    (VOID) gwf_error(E_GW0325_DMT_CLOSE_ERROR, GWF_INTERR, 2,
				     sizeof(dmt->dmt_id.db_tab_base),
				     &dmt->dmt_id.db_tab_base,
				     sizeof(dmt->dmt_id.db_tab_index),
				     &dmt->dmt_id.db_tab_index);
		    gw_rcb->gwr_error.err_code = E_GW021D_GWU_MAP_IDX_REL_ERROR;
		    break;
	    }
	}
	/*
	**	Restore any failure from before
	*/
	if (status==E_DB_OK)
		status=ret_code;

	break;
    }
    return(status);
}

/*{
** Name: gwi_map_idx_att    -	create tuple entries in the iigwX_attribute
**				table for this gateway secondary index
**
** Description:
**	This function creates tuples in the extended attribute catalog for this
**	gateway secondary index.  The tuples are those defined for the index,
**	plus a tidp column.
**
**	In order to create the tuple entries, the following steps are performed:
**
**	(1) select all rows from iiattribute for this index, and save them.
**	(2) For each row saved:
**	    (a)	if it's 'tidp', drop down to (3).
**	    (b)	select the row from iiattribute for the base table which has
**		the same name and save its attid; we will use this to get
**		extended attribute info.
**	    (c)	now select the appropriate row from iigwX_attribute.
**	    (d)	update the xrelidx and xattid fields -- xrelidx is non-zero for
**		this index, xattid is that for the index not the base table.
**	    (e)	append the row.
**	(3) finished with "real" attributes.  Check that we found 1 or more
**	    other attributes -- else error.  Now create the row for the tidp
**	    domain and append it.  Use integer gw datatype, length 4, offset 0;
**	    we don't really use this attribute and it doesn't exist in the
**	    gateway record.  Need to have a gateway-specific static definition
**	    for this row.
**
** Inputs:
**	gw_rcb->		gateway request control block
**	    gwr_tab_id		table id for this secondary index
**	ulm_rcb			memory request control block, passed in from
**				parent routine as it is in the same stream, we
**				want to deallocate all related memory from there
**
** Output:
**	gw_rcb->
**		error.err_code	One of the following error numbers.
**				E_GW0600_NO_MEM
**
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
** History:
**	24-sep-90 (linda)
**	    Created.
**	07-oct-92 (robf)
**	    If open of iiattribute fails, don't try to continue processing.
**      8-oct-92 (daveb)
**          cast an assignment.
**      8-oct-92 (daveb)
**          prototyped.  Cast db_id arg to gwu_copen; it's a PTR.
**	30-May-2006 (jenjo02)
**	    Max attributes in an index is now DB_MAXIXATTS, not DB_MAXKEYS.
*/
static
DB_STATUS
gwi_map_idx_att(GW_RCB  *gw_rcb,
                ULM_RCB *ulm_rcb )
{
    GW_SESSION	    *session = (GW_SESSION *)gw_rcb->gwr_session_id;
    DMT_CB	    dmt;
    DMR_CB	    dmr;
    char	    *dmp_atts[DB_MAXIXATTS+1];
    GW_DMP_ATTR	    *dmp_att1;
    GW_DMP_ATTR	    *dmp_att2;
    GW_DMP_ATTR	    iiatt_buf;
    DB_TAB_ID	    base_tab_id;
    DB_TAB_ID	    *tab_id;
    i4		    i;
    i4		    j;
    i2		    *attid_ptr;
    DMR_ATTR_ENTRY  attkey[2];
    DMR_ATTR_ENTRY  *attkey_ptr[2];
    i4	    gw_id = gw_rcb->gwr_gw_id;
    i4		    xatt_sz = Gwf_facility->gwf_gw_info[gw_id].gwf_xatt_sz;
    char	    stage_buf[DB_MAXTUP];
    char	    *xatt_rows[DB_MAXIXATTS+1];
    DB_STATUS	    status = E_DB_OK;

    /*
    ** Set up the base table id for later use.
    */
    STRUCT_ASSIGN_MACRO(gw_rcb->gwr_tab_id, base_tab_id);
    base_tab_id.db_tab_index = 0;

    for (;;)	/* just to break out of on error... */
    {
	/*
	** First set up dmt and open iiattribute table.
	*/
	dmt.type = DMT_TABLE_CB;
	dmt.length = sizeof(DMT_CB);
	dmt.dmt_id.db_tab_base = 3;    /* tab_id for iiattribute */
	dmt.dmt_id.db_tab_index = 0;   /* "    "	"   "	"   " */
/**
*** gwr_gw_sequence == 0, which causes dmt_open to fail.  For now set it to 1.
***	dmt.dmt_sequence = gw_rcb->gwr_gw_sequence;
**/
	dmt.dmt_sequence = 1;
	dmt.dmt_db_id = (PTR)gw_rcb->gwr_database_id;
	dmt.dmt_tran_id = gw_rcb->gwr_xact_id;
	dmt.dmt_flags_mask = 0;
	dmt.dmt_access_mode = DMT_A_READ;
	dmt.dmt_lock_mode = DMT_IS;
	dmt.dmt_update_mode = DMT_U_DEFERRED;
	dmt.dmt_char_array.data_address = NULL;
	dmt.dmt_char_array.data_in_size = 0;

	if ((status = (*Dmf_cptr)(DMT_OPEN, &dmt)) != E_DB_OK)
	{
	    switch (dmt.error.err_code)
	    {
		case    E_DM0042_DEADLOCK:
		    gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
		    break;

		default:
		    (VOID) gwf_error(dmt.error.err_code, GWF_INTERR, 0);
		    (VOID) gwf_error(E_GW0324_DMT_OPEN_ERROR, GWF_INTERR, 2,
				    (i4)4, (PTR)&dmt.dmt_id.db_tab_base,
				    (i4)4, (PTR)&dmt.dmt_id.db_tab_index);
		    gw_rcb->gwr_error.err_code = E_GW021E_GWU_MAP_IDX_ATT_ERROR;
		    break;
	    }
	    break;
	}

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
	if ((status = (*Dmf_cptr)(DMR_POSITION, &dmr)) != E_DB_OK)
	{
	    switch (dmt.error.err_code)
	    {
		case    E_DM0042_DEADLOCK:
		    gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
		    break;

		default:
		    (VOID) gwf_error(dmr.error.err_code, GWF_INTERR, 0);
		    (VOID) gwf_error(E_GW0323_DMR_POSITION_ERROR, GWF_INTERR, 2,
				    (i4)4, (PTR)&dmt.dmt_id.db_tab_base,
				    (i4)4, (PTR)&dmt.dmt_id.db_tab_index);
		    gw_rcb->gwr_error.err_code = E_GW0213_GWU_COPEN_ERROR;
		    break;
	    }
	}

	/*
	** We'll allocate room for a GW_DMP_ATTR struct plus an extra 2
	** bytes for the column's base table attid, which we'll use to get
	** the right row from iigwX_attribute.
	*/
	ulm_rcb->ulm_psize = sizeof(GW_DMP_ATTR) + 2;

	dmr.dmr_data.data_in_size = sizeof(GW_DMP_ATTR) + 2;
	dmr.dmr_flags_mask = DMR_NEXT;
	dmr.dmr_data.data_address = &stage_buf[0];
	dmr.dmr_char_array.data_address = 0;

	for (i=0; i < gw_rcb->gwr_column_cnt; i++)
	{
	    do
	    {
	        status = (*Dmf_cptr)(DMR_GET, &dmr);
	    }
	    while ((status == E_DB_OK) &&
		   (MEcmp((PTR)dmr.dmr_data.data_address,
			  (PTR)&gw_rcb->gwr_tab_id,
			  sizeof(DB_TAB_ID)))
		  );

	    if (status == E_DB_OK)
	    {
		if ((status = ulm_palloc(ulm_rcb)) != E_DB_OK)
		{
		    gwf_error(ulm_rcb->ulm_error.err_code, GWF_INTERR, 0);
		    gwf_error(E_GW0314_ULM_PALLOC_ERROR, GWF_INTERR, 4,
			      sizeof(*ulm_rcb->ulm_memleft),
			      ulm_rcb->ulm_memleft,
			      sizeof(ulm_rcb->ulm_sizepool),
			      &ulm_rcb->ulm_sizepool,
			      sizeof(ulm_rcb->ulm_blocksize),
			      &ulm_rcb->ulm_blocksize,
			      sizeof(ulm_rcb->ulm_psize),
			      &ulm_rcb->ulm_psize);
		    if (ulm_rcb->ulm_error.err_code == E_UL0005_NOMEM)
			gw_rcb->gwr_error.err_code = E_GW0600_NO_MEM;
		    else
		    {
			switch (dmt.error.err_code)
			{
			    case    E_DM0042_DEADLOCK:
				gw_rcb->gwr_error.err_code =
					    E_GW0327_DMF_DEADLOCK;
				break;

			    default:
				gw_rcb->gwr_error.err_code =
					E_GW021E_GWU_MAP_IDX_ATT_ERROR;
				break;
			}
		    }
		    break;
		}
		MEcopy((PTR)&stage_buf[0], sizeof(GW_DMP_ATTR)+2,
		       ulm_rcb->ulm_pptr);
		dmp_atts[i] = (char *)ulm_rcb->ulm_pptr;
		continue;   /* get next attribute for the index */
	    }
	    else if (dmr.error.err_code == E_DM0055_NONEXT)
	    {
/***FIXME*** -- at this point we've allocated an extra buffer?  look at -- ***/
		status = E_DB_OK;
		dmr.error.err_code = E_DB_OK;
		break;	    /* no more attributes for this index */
	    }
	    else
	    {
		switch (dmt.error.err_code)
		{
		    case    E_DM0042_DEADLOCK:
			gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
			break;

		    default:
			(VOID) gwf_error(dmr.error.err_code, GWF_INTERR, 0);
			(VOID) gwf_error(E_GW0323_DMR_POSITION_ERROR,
					 GWF_INTERR, 2, (i4)4,
					 (PTR)&dmt.dmt_id.db_tab_base,
					 (i4)4,
					 (PTR)&dmt.dmt_id.db_tab_index);
			gw_rcb->gwr_error.err_code =
				    E_GW021E_GWU_MAP_IDX_ATT_ERROR;
			break;
		}
	    }

	    break;
	}

	if (status != E_DB_OK)
	    break;  /* error already reported. */

	/*
	** Now we get the base table rows from iiattribute, so we can store the
	** attid from the base to get the proper base table row from
	** iigwX_attribute.
	*/
	attkey[0].attr_value = (char *)&base_tab_id.db_tab_base;
	attkey[1].attr_value = (char *)&base_tab_id.db_tab_index;

	if ((status = (*Dmf_cptr)(DMR_POSITION, &dmr)) != E_DB_OK)
	{
	    switch (dmt.error.err_code)
	    {
		case    E_DM0042_DEADLOCK:
		    gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
		    break;

		default:
		    (VOID) gwf_error(dmr.error.err_code, GWF_INTERR, 0);
		    (VOID) gwf_error(E_GW0323_DMR_POSITION_ERROR, GWF_INTERR, 2,
				    (i4)4, (PTR)&dmt.dmt_id.db_tab_base,
				    (i4)4, (PTR)&dmt.dmt_id.db_tab_index);
		    gw_rcb->gwr_error.err_code = E_GW021E_GWU_MAP_IDX_ATT_ERROR;
		    break;
	    }
	    break;
	}

	/*
	** We don't know how many columns are in the base table; just need to
	** find matches for those in the index table.  The tidp column will be
	** handled separately.  Only need one buffer here, we store base table
	** attid with previously-allocated rows for index table.
	*/

	dmr.dmr_data.data_in_size = sizeof(GW_DMP_ATTR);
	dmr.dmr_data.data_address = (PTR)&iiatt_buf;
	dmr.dmr_char_array.data_address = 0;

	i = 0;	/* keep a count of matches found */

	for (;;)    /* this gives us a place to continue to. */
	{
	    do
	    {
		status = (*Dmf_cptr)(DMR_GET, &dmr);
	    }
	    while ((status == E_DB_OK) &&
		   (MEcmp((PTR)dmr.dmr_data.data_address,
			  (PTR)&base_tab_id,
			  sizeof(DB_TAB_ID)))
		  );

	    if (status == E_DB_OK)
	    {
		dmp_att1 = (GW_DMP_ATTR *)dmr.dmr_data.data_address;
		for (j=0; j < gw_rcb->gwr_column_cnt; j++)
		{
		    dmp_att2 = (GW_DMP_ATTR *)dmp_atts[j];
		    if
		    (!MEcmp((PTR)&dmp_att1->attname, (PTR)&dmp_att2->attname,
			    sizeof(DB_ATT_NAME)))
		    {
			attid_ptr =
			    (i2 *)(dmp_atts[j] + sizeof(GW_DMP_ATTR));
			*attid_ptr = dmp_att1->attid;
			i++;
			break;	/* out of inner loop */
		    }
		}
	    }
	    else if (dmr.error.err_code == E_DM0055_NONEXT)
	    {
		status = E_DB_OK;
		dmr.error.err_code = E_DB_OK;
		break;	    /* no more attributes for this index */
	    }
	    else
	    {
		switch (dmt.error.err_code)
		{
		    case    E_DM0042_DEADLOCK:
			gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
			break;

		    default:
			(VOID) gwf_error(dmr.error.err_code, GWF_INTERR, 0);
			(VOID) gwf_error(E_GW0323_DMR_POSITION_ERROR,
					 GWF_INTERR, 2, (i4)4,
					 (PTR)&dmt.dmt_id.db_tab_base,
					 (i4)4,
					 (PTR)&dmt.dmt_id.db_tab_index);
			gw_rcb->gwr_error.err_code =
				    E_GW021E_GWU_MAP_IDX_ATT_ERROR;
			break;
		}
		break;
	    }

	    if (i == gw_rcb->gwr_column_cnt - 1)
		break;	/* we've found all the matching rows */
	}

	/* Always close iiattribute table. */
	if ((status = (*Dmf_cptr)(DMT_CLOSE, &dmt)) != E_DB_OK)
	{
	    switch (dmt.error.err_code)
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
		    gw_rcb->gwr_error.err_code = E_GW021E_GWU_MAP_IDX_ATT_ERROR;
		    break;
	    }
	    break;
	}

	if (status != E_DB_OK)	/* whether from previous code, or close */
	    break;  /* error already reported */

	/*
	** We should have found matches for all but one index column (tidp).
	*/
	if (i != gw_rcb->gwr_column_cnt - 1)
	{
/***generate error here***/
	    gw_rcb->gwr_error.err_code = E_GW021E_GWU_MAP_IDX_ATT_ERROR;
	    break;
	}


	/* position on base table id */
	dmr.dmr_position_type = DMR_QUAL;
	dmr.dmr_attr_desc.ptr_in_count = 2;
	dmr.dmr_attr_desc.ptr_size = sizeof(DMR_ATTR_ENTRY);
	dmr.dmr_attr_desc.ptr_address = (PTR)&attkey_ptr[0];


	/* now we get the rows from the extended attribute catalog */
	if ((status = gwu_copen(&dmt, &dmr, session->gws_scf_session_id, 
		(PTR)gw_rcb->gwr_database_id, gw_id, gw_rcb->gwr_xact_id,
		&gw_rcb->gwr_tab_id, 
		&session->gws_gw_info[gw_id].gws_xatt_tab_name,
		DMT_A_WRITE, TRUE, &gw_rcb->gwr_error)) 
	    != E_DB_OK)
	{
	    switch (dmt.error.err_code)
	    {
		case    E_DM0042_DEADLOCK:
		    gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
		    break;

		default:
		    (VOID) gwf_error(gw_rcb->gwr_error.err_code, GWF_INTERR, 0);
		    gw_rcb->gwr_error.err_code = E_GW021E_GWU_MAP_IDX_ATT_ERROR;
		    break;
	    }
	}

	/*
	** Set up staging area for tuples returned from extended attribute
	** catalog.  We'll allocate space when we find a match, so we can
	** insert them later.
	*/
	dmr.dmr_flags_mask = DMR_NEXT;
	dmr.dmr_data.data_in_size = xatt_sz;
	dmr.dmr_data.data_address = (PTR)&stage_buf[0];
	dmr.dmr_char_array.data_address = 0;

	/*
	** Size to be allocated is one extended attribute row.
	*/
	ulm_rcb->ulm_psize = xatt_sz;

	i = 0;

	/* search for the right tuples */
	for (;;)
	{
	    do
	    {
		status = (*Dmf_cptr)(DMR_GET, &dmr);
	    } while (status == E_DB_OK
		 &&
		 MEcmp((PTR)dmr.dmr_data.data_address,
		       (PTR)&base_tab_id,
		       sizeof(DB_TAB_ID)));

	    if (status == E_DB_OK)
	    {

		/*
		** When we find a tuple from iigwX_attribute which matches one
		** of the index tuples, update the values to represent the
		** index and then store the tuple.  NOTE that we assume that
		** tab_id and attid fields are the first fields in the extended
		** catalog.  We can't know the internal structure of extended
		** attribute catalogs for particular gateways beyond the 1st
		** three cols.
		*/

		dmp_att1 = (GW_DMP_ATTR *)dmr.dmr_data.data_address;

		for (j=0; j < gw_rcb->gwr_column_cnt - 1; j++)
		{
		    if (dmp_att1->attid ==
			*(i2 *)(dmp_atts[j] + sizeof(GW_DMP_ATTR)))
		    {
			if ((status = ulm_palloc(ulm_rcb)) != E_DB_OK)
			{
			    gwf_error(ulm_rcb->ulm_error.err_code, GWF_INTERR,
				      0);
			    gwf_error(E_GW0314_ULM_PALLOC_ERROR, GWF_INTERR, 4,
				      sizeof(*ulm_rcb->ulm_memleft),
				      ulm_rcb->ulm_memleft,
				      sizeof(ulm_rcb->ulm_sizepool),
				      &ulm_rcb->ulm_sizepool,
				      sizeof(ulm_rcb->ulm_blocksize),
				      &ulm_rcb->ulm_blocksize,
				      sizeof(ulm_rcb->ulm_psize),
				      &ulm_rcb->ulm_psize);
			    if (ulm_rcb->ulm_error.err_code == E_UL0005_NOMEM)
				gw_rcb->gwr_error.err_code = E_GW0600_NO_MEM;
			    else
				gw_rcb->gwr_error.err_code =
					E_GW021E_GWU_MAP_IDX_ATT_ERROR;
			    break;
			}

			dmp_att1->attid = i + 1;
			xatt_rows[i] = ulm_rcb->ulm_pptr;
			MEcopy(dmr.dmr_data.data_address,
			       xatt_sz, xatt_rows[i]);
			tab_id = (DB_TAB_ID *)xatt_rows[i];
			STRUCT_ASSIGN_MACRO(gw_rcb->gwr_tab_id, *tab_id);
			i++;
			break;
		    }
		}
		if (i == gw_rcb->gwr_column_cnt - 1)
		    break;
	    }
	    else	/* cannot get table info */
	    {
		if (dmr.error.err_code == E_DM0055_NONEXT)
		{
		    status = E_DB_OK;
		    dmr.error.err_code = E_DB_OK;
		    break;	    /* no more attributes for this index */
		}
		else
		{
		    switch (dmt.error.err_code)
		    {
			case    E_DM0042_DEADLOCK:
			    gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
			    break;

			default:
			    (VOID) gwf_error(dmr.error.err_code, GWF_INTERR, 0);
			    (VOID) gwf_error(E_GW0321_DMR_GET_ERROR,
					GWF_INTERR, 2, (i4)4,
					(PTR)&gw_rcb->gwr_tab_id.db_tab_base,
					(i4)4,
					(PTR)&gw_rcb->gwr_tab_id.db_tab_index);
			    gw_rcb->gwr_error.err_code =
					E_GW021E_GWU_MAP_IDX_ATT_ERROR;
			    break;
		    }
		    break;
		}
	    }

	    continue;   /* get next attribute for the base */
	}

	/*
	** Okay, now construct the row for tidp.  Reuse stage_buf for this.
	*/
	MEcopy(Gwf_facility->gwf_gw_info[gw_id].gwf_xatt_tidp, xatt_sz,
	       (PTR)&stage_buf[0]);
	tab_id = (DB_TAB_ID *)&stage_buf[0];
	STRUCT_ASSIGN_MACRO(gw_rcb->gwr_tab_id, *tab_id);
	attid_ptr = (i2 *)(&stage_buf[0] + sizeof(DB_TAB_ID));
	*attid_ptr = i + 1;
	xatt_rows[i] = &stage_buf[0];

	/*
	** Finally, put the iigwX_attribute tuples for the gateway secondary
	** index.
	*/

	dmr.dmr_data.data_in_size = xatt_sz;

	for (j=0; j<=i; j++)
	{
	    dmr.dmr_data.data_address = (PTR)xatt_rows[j];

	    if ((status = (*Dmf_cptr)(DMR_PUT, &dmr)) != E_DB_OK)
	    {
		switch (dmt.error.err_code)
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
			gw_rcb->gwr_error.err_code =
					E_GW021E_GWU_MAP_IDX_ATT_ERROR;
			break;
		}
		break;
	    }
	}

	if ((status = (*Dmf_cptr)(DMT_CLOSE, &dmt)) != E_DB_OK)
	{
	    switch (dmt.error.err_code)
	    {
		case    E_DM0042_DEADLOCK:
		    gw_rcb->gwr_error.err_code = E_GW0327_DMF_DEADLOCK;
		    break;

		default:
		    (VOID) gwf_error(dmt.error.err_code, GWF_INTERR, 0);
		    (VOID) gwf_error(E_GW0325_DMT_CLOSE_ERROR, GWF_INTERR, 2,
				     sizeof(dmt.dmt_id.db_tab_base),
				     &dmt.dmt_id.db_tab_base,
				     sizeof(dmt.dmt_id.db_tab_index),
				     &dmt.dmt_id.db_tab_index);
		    gw_rcb->gwr_error.err_code = E_GW021E_GWU_MAP_IDX_ATT_ERROR;
		    break;
	    }
	    break;
	}

	/* if we get here everything's OK... */
	break;
    }

    return(status);
}
