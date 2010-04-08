/*
**Copyright (c) 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <st.h>
#include    <me.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <cs.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <gwf.h>
#include    <gwfint.h>


/**
** Name: GWFFAC.C - facility-level generic gateway functions.
**
** Description:
**      This file contains facility-level functions for the gateway facility.
**
**      This file defines the following external functions:
**    
**      gwf_init()	-   generate facility initialization
**	gwf_term()	-   terminate the facility
**	gwf_svr_info()	-   query the GWF facility for server info.
**
** History:
**	04-apr-90 (linda)
**	    Created -- broken out from gwf.c which is now obsolete.
**	5-apr-90 (bryanp)
**	    Added improved calculation of GWF memory pool. Pool size is now
**	    scaled by number of users.
**	9-apr-90 (bryanp)
**	    Updated for new error-handling scheme.  Added gwf_svr_info and
**	    changed gwf_init to support the new gwf_call interface.
**	16-apr-90 (linda)
**	    Added a parameter to gwu_deltcb() call indicating whether the
**	    tcb to be deleted must exist or not.
**	18-apr-90 (bryanp)
**	    Return proper error code when run out of memory.
**	19-apr-90 (bryanp)
**	    Fix server shutdown bug involving bad arguments to SCU_MFREE.
**	    Fix server shutdown bug involving endless loop deleting TCB's
**      18-may-92 (schang)
**          GW merge.
**	    4-feb-91 (linda)
**	        Code cleanup:  (a) update structure member names.  (b) fix unix
**	        compiler warning.
**	    4-nov-91 (rickh)
**	        Return release identifier string at server initialization time.
**	        SCF spits up this string when poked with
**	        "select dbmsinfo( '_version' )"
**	7-oct-92 (daveb)
**	    fill in gwr_scfcb_size and gwr_server at init time so SCF
**	    can treat us as a first class citizen and make the session
**	    init calls.
**	23-Oct-1992 (daveb)
**	    name semaphore.
**      21-sep-92 (schang)
**          initialize and free up server wide individual gateway specific
**          memory pointer
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      23-jul-93 (schang)
**          add st.h and me.h 
**	28-Apr-1995 (jenjo02)
**	    Combined CSi|n_semaphore functions calls into single
**	    CSw_semaphore call.
**	23-sep-1996 (canor01)
**	    Moved global data definitions to gwfdata.c.
**	05-mar-97 (toumi01)
**	    initialize the global trace flags array
**      24-jul-97 (stial01)
**          gwf_init() Set gwx_rcb.xrcb_gchdr_size before calling gateway init.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      28-jan-97 (schang)  merge 6.4 changes
**        12-aug-93 (schang)
**          initialize xrcb_xbitset
**      7-oct-2004 (thaju02)
**          Replace i4 with SIZE_TYPE for memory pool > 2Gig.
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
**	10-Mar-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
*/

/*
** Forward or global definitions.
*/

GLOBALREF	GW_FACILITY	*Gwf_facility;
GLOBALREF	DB_STATUS	((*Dmf_cptr))();


/*
** Forward declarations for static functions:
*/
static	SIZE_TYPE	    gwf_def_pool_size(void);

/*{
** Name: gwf_init - initialize the gateway facility
**
** Description:
**	This function performs general gateway initialization.  Facility global
**	structures are allocated and initialized.
**
**	A ULM memory stream is set up for GWF for allocating the various GWF
**	data structures.
**
**	Gateway initialization exits are called to initialize the gateway.  The
**	identity of the initialization exits are obtained from Gwf_itab.
**
** Inputs:
**	gw_rcb->		Standard GWF control block
**	    gwr_dmf_cptr	The address of function "dmf_call()", so that
**				we can call back to DMF for, e.g., extended
**				catalog access.
**
** Output:
**	gw_rcb->		Standard GWF control block
**	    gwr_out_vdata1	Release id of Gateway.
**	    gwr_scfcb_size	size of CB for SCF to allocate per session.
**	    gwr_server		set to the GwF_facility, for SCF to know.
**	error->
**          err_code		One of the following error numbers.
**				E_GW0200_GWF_INIT_ERROR
**				E_GW0600_NO_MEM
**
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Cannot allocate Gwf_facility.
**	    E_DB_WARN		Success, informational status sent back to DMF
**				in the error.err_code field (either that there
**				is no gateway initialized, or that none of the
**				gateways needs transaction notification).
**
** History:
**      21-Apr-1989 (alexh)
**          Created.
**	14-Dec-1989 (linda)
**	    Extended catalog table names were being filled in incorrectly; see
**	    comments below.
**	23-dec-89 (paul)
**	    Changed memory allocation strategy for the gateway.  See comments
**	    embedded in code.
**	26-mar-90 (linda)
**	    Changed error handling.  Changed to have one return point.
**	5-apr-90 (bryanp)
**	    Added improved calculation of GWF memory pool. Pool size is now
**	    scaled by number of users.
**	9-apr-90 (bryanp)
**	    This function is now called via gwf_call(), and takes a gw_rcb.
**	18-apr-90 (bryanp)
**	    If SCF says not enough memory, return proper error code.
**	27-sep-90 (linda)
**	    Set up pointer to tidp tuple for extended attribute tables, in
**	    support of gateway secondary indexes.
**	5-dec-90 (linda)
**	    Initialize the tcb semaphore.  We were using it for locking the tcb
**	    list -- but it hadn't been initialized so locking was not working.
**	4-nov-91 (rickh)
**	    Return release identifier string at server initialization time.
**	    SCF spits up this string when poked with
**	    "select dbmsinfo( '_version' )"
**	7-oct-92 (daveb)
**	    fill in gwr_scfcb_size and gwr_server at init time so SCF
**	    can treat us as a first class citizen and make the session
**	    init calls.  Prototyped.
**	23-Oct-1992 (daveb)
**	    name semaphore.
**      21-sep-92 (schang)
**          initialize individual gateway specific server wide memory pointer
**	05-mar-97 (toumi01)
**	    initialize the global trace flags array
**      24-jul-97 (stial01)
**          gwf_init() Set gwx_rcb.xrcb_gchdr_size before calling gateway init.
*/
DB_STATUS
gwf_init( GW_RCB *gw_rcb )
{
    i4		i;
    SCF_CB	scf_cb;
    DB_STATUS	status;
    STATUS	cl_status;

    /* zero out the release id descriptor */

    MEfill(sizeof( DM_DATA ), 0, (PTR)&gw_rcb->gwr_out_vdata1 );

    for (;;)	/*  Something to break out of...    */
    {
	/* allocate Gwf_facility */
	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_length = sizeof(SCF_CB);
	scf_cb.scf_session = DB_NOSESSION;
	scf_cb.scf_facility = DB_GWF_ID;
	scf_cb.scf_scm.scm_functions = 0;
	scf_cb.scf_scm.scm_in_pages = (sizeof(GW_FACILITY)/SCU_MPAGESIZE+1);
	if ((status = scf_call(SCU_MALLOC, &scf_cb)) != E_DB_OK)
	{
	    gwf_error(scf_cb.scf_error.err_code, GWF_INTERR, 0);
	    gwf_error(E_GW0300_SCU_MALLOC_ERROR, GWF_INTERR, 1,
		      sizeof(scf_cb.scf_scm.scm_in_pages),
		      &scf_cb.scf_scm.scm_in_pages);

	    switch (scf_cb.scf_error.err_code)
	    {
		case E_SC0004_NO_MORE_MEMORY:
		case E_SC0005_LESS_THAN_REQUESTED:
		case E_SC0107_BAD_SIZE_EXPAND:
		    gw_rcb->gwr_error.err_code = E_GW0600_NO_MEM;
		    break;

		default:
		    gw_rcb->gwr_error.err_code = E_GW0200_GWF_INIT_ERROR;
		    break;
	    }
	    break;
	}
	Gwf_facility = (GW_FACILITY *)scf_cb.scf_scm.scm_addr;
	Gwf_facility->gwf_tcb_list = NULL;
	cl_status = CSw_semaphore(&Gwf_facility->gwf_tcb_lock, CS_SEM_SINGLE,
					"GWF TCB sem" );
	if (cl_status != OK)
	{
	    gwf_error(cl_status, GWF_INTERR, 0);
	    gw_rcb->gwr_error.err_code = E_GW0200_GWF_INIT_ERROR;
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Initialize memory allocation scheme for GWF. We have the following
	** memory allocation scheme.
	**
	**	1. TCB
	**		Allocated directly by SCF. Allocation and deallocation
	**		is controlled directly by GWF. It loks like TCB's are
	**		held until they are no longer valid (due to a DROP or
	**		REGISTER INDEX) or until the server shuts down. It's
	**		not clear this is the best allocation strategy.
	**
	**	2. SCB
	**		The session control block is allocated within its own
	**		ULM memory stream. Since there is no other information
	**		that lives for the entire session, this is the only
	**		information handled by this memory stream. The stream
	**		id is stored in the SCB.
	**
	**	3. RSB
	**		The record control blocks containing information for a
	**		particular access to a gateway table are allocated from
	**		a separate stream initialized at the time the table is
	**		"opened" for access and deleted at the time the table
	**		is "closed".  The stream id is stored in the RSB.
	**
	**	4. Temporary Memory
	**		Memory needed for a single operation such as
	**		registering a table is allocated from a ULF memory
	**		stream. Such srteams must be opened and closed within a
	**		single invocation of the GWF.
	**
	** At this time we initialize the pool from which ULM streams will be
	** allocated.
	*/
	Gwf_facility->gwf_ulm_rcb.ulm_facility = DB_GWF_ID;
	Gwf_facility->gwf_ulm_rcb.ulm_blocksize = SCU_MPAGESIZE;
	Gwf_facility->gwf_ulm_rcb.ulm_sizepool = gwf_def_pool_size();
	status = ulm_startup(&Gwf_facility->gwf_ulm_rcb);
	if (status != E_DB_OK)
	{
	    gwf_error(Gwf_facility->gwf_ulm_rcb.ulm_error.err_code,
		      GWF_INTERR, 0);
	    gwf_error(E_GW0310_ULM_STARTUP_ERROR, GWF_INTERR, 1,
		      sizeof(Gwf_facility->gwf_ulm_rcb.ulm_sizepool),
		      &Gwf_facility->gwf_ulm_rcb.ulm_sizepool);
	    if (Gwf_facility->gwf_ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
		gw_rcb->gwr_error.err_code = E_GW0600_NO_MEM;
	    else
		gw_rcb->gwr_error.err_code = E_GW0200_GWF_INIT_ERROR;
	    break;
	}

	Gwf_facility->gwf_gw_active = 0;    /* assume no gateways. */
	Gwf_facility->gwf_gw_xacts  = 0;    /* and no transaction handling */

	/* initialize the global trace flags array */
	MEfill(sizeof(Gwf_facility->gwf_trace), 0,
	    (PTR)Gwf_facility->gwf_trace);

	/* initialize each gateway's exit vector */
	for (i=0; i < GW_GW_COUNT; ++i)
	{
	    GWX_RCB	gwx_rcb;

	    gwx_rcb.xrcb_gwf_version = GWX_VERSION;
	    gwx_rcb.xrcb_exit_table =
		(GWX_VECTOR *)&Gwf_facility->gwf_gw_info[i].gwf_gw_exits[0];
	    gwx_rcb.xrcb_dmf_cptr = gw_rcb->gwr_dmf_cptr;
	    gwx_rcb.xrcb_gca_cb = gw_rcb->gwr_gca_cb;

            /*
            ** schang: init new field xrcb_xhandle, this field passes
            ** individual gateway specific, server wide memory
            ** pointer (sep-21-1992)
            ** initialize xrcb_xbitset (aug-12-93)
            */
            gwx_rcb.xrcb_xhandle = NULL;
            gwx_rcb.xrcb_xbitset = 0;

	    MEfill(sizeof( DM_DATA ), 0, (PTR)&gwx_rcb.xrcb_var_data1 );

	    /* refer to Gwf_itab to decide which initializations are required */
	    if (Gwf_itab[i] == NULL)
	    {
		Gwf_facility->gwf_gw_info[i].gwf_gw_exist = 0;
	    }
	    else if ((status = (*Gwf_itab[i])(&gwx_rcb)) == E_DB_OK)
	    {
               /* schang : new memory pointer initialized */
                Gwf_facility->gwf_gw_info[i].gwf_xhandle =
                                    gwx_rcb.xrcb_xhandle;
                Gwf_facility->gwf_gw_info[i].gwf_xbitset =
                                    gwx_rcb.xrcb_xbitset;
		Gwf_facility->gwf_gw_info[i].gwf_rsb_sz =
				    gwx_rcb.xrcb_exit_cb_size;
		Gwf_facility->gwf_gw_info[i].gwf_xrel_sz =
				    gwx_rcb.xrcb_xrelation_sz;
		Gwf_facility->gwf_gw_info[i].gwf_xatt_sz =
				    gwx_rcb.xrcb_xattribute_sz;
		Gwf_facility->gwf_gw_info[i].gwf_xidx_sz =
				    gwx_rcb.xrcb_xindex_sz;
		Gwf_facility->gwf_gw_info[i].gwf_gw_exist = 1;
		/* initialize extended catalog names */
		STprintf((char *)&Gwf_facility->gwf_gw_info[i].gwf_xrel_tab_name,
			"iigw%02d_relation", i);
		STprintf((char *)&Gwf_facility->gwf_gw_info[i].gwf_xatt_tab_name,
			"iigw%02d_attribute", i);
		STprintf((char *)&Gwf_facility->gwf_gw_info[i].gwf_xidx_tab_name,
			"iigw%02d_index", i);

		/* pass the release identifier up to SCF */

		if ( gwx_rcb.xrcb_var_data1.data_address != 0 )
		{
		    MEcopy( (PTR)&gwx_rcb.xrcb_var_data1, sizeof( DM_DATA ),
			(PTR)&gw_rcb->gwr_out_vdata1 );
		}

		/*
		** Now set up pointer to tidp tuple for this gateway's extended
		** attribute catalog, to support gateway secondary indexes.
		*/
		Gwf_facility->gwf_gw_info[i].gwf_xatt_tidp =
					    gwx_rcb.xrcb_xatt_tidp;
		/*
		** Note, if >1 gateway is initialized, then if any gateway needs
		** transaction notification, DMF will always notify.  Also note,
		** we check error.err_code here even though status is E_DB_OK.
		** Not great, but I can't think of a better way...
		*/
		if (gwx_rcb.xrcb_error.err_code == E_GW0500_GW_TRANSACTIONS)
		    Gwf_facility->gwf_gw_xacts = 1;

		Gwf_facility->gwf_gw_active = 1;
	    }
	    else	/* status != E_DB_OK */
	    {
		gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR, 0);
		gw_rcb->gwr_error.err_code = E_GW0200_GWF_INIT_ERROR;
		break;
	    }
	}

	gw_rcb->gwr_scfcb_size = sizeof(GW_SESSION);
	gw_rcb->gwr_server = (PTR)Gwf_facility;

	if (status != E_DB_OK)
	    break;		/* gateway exit failed */

	/*
	** Now that we're ready to go, assign global Dmf_cptr its value (== the
	** address of function dmf_call()).  We need to do this to remove
	** explicit  calls to the DMF facility, resolving circular references of
	** shareable libraries when building.
	*/
	Dmf_cptr = gw_rcb->gwr_dmf_cptr;

	break;
    }

    if (status != E_DB_OK)
    {
	return(status);
    }
    else
    {
	gw_rcb->gwr_error.err_code = E_DB_OK;
	return(E_DB_OK);
    }
}

/*{
** Name: gwf_term - terminate the gateway facility
**
** Description:
**	This function performs gateway facility termination.  Facility global
**	structures are deallocated, and all active gateway servers are shut
**	down.
**
** Inputs:
**	gw_rcb->		Standard GWF control block
**	    error		error structure for error status
**
** Output:
**	gw_rcb->		Standard GWF control block
**	    error.
**		err_code
**				E_GW0007_GWX_VTERM_ERROR
**
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
** History:
**      21-Apr-1989 (alexh)
**          Created.
**	27-dec-89 (paul)
**	    Added support for calling termination entry with an RCB.  This will
**	    allow termination arguments if needed for future GW's.
**	26-mar-90 (linda)
**	    Added error handling.
**	9-apr-90 (bryanp)
**	    Updated error handling to new GWF scheme.  Changed to take gw_rcb
**	    structure.
**	19-apr-90 (bryanp)
**	    Fix server shutdown bug involving bad arguments to SCU_MFREE.
**	    Fix server shutdown bug involving endless loop deleting TCB's
**	7-oct-92 (daveb)
**	    prototyped.  removed dead variable err.
**      21-sep-92 (schang)
**          free individual gateway specific  server wide memory before
**          server shutdown
*/
DB_STATUS
gwf_term( GW_RCB *gw_rcb )
{
    DB_STATUS	status;
    bool	badstat = FALSE;
    i4		i;
    SCF_CB	scf_cb;
    GWX_RCB	gwx_rcb;
    
    status = E_DB_OK;
    
    /* terminate each initiated gateway by calling its termination entry. */
    for (i=0; i < GW_GW_COUNT; ++i)
    {
	if (Gwf_facility->gwf_gw_info[i].gwf_gw_exist)
	{
            /* schang : terminates individual gateway specific memory */
            gwx_rcb.xrcb_xhandle = Gwf_facility->gwf_gw_info[i].gwf_xhandle;
	    status =
	      (*Gwf_facility->gwf_gw_info[i].gwf_gw_exits[GWX_VTERM])(&gwx_rcb);
	    if (status != E_DB_OK)
	    {
		/* On error report, log and continue shutting down gateways */
		gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR, 0);
		gwf_error(E_GW0201_GWF_TERM_ERROR, GWF_INTERR, 1,
			  sizeof(i), &i);
		badstat = TRUE;
		continue;
	    }
	}
    }

#if 0
    /*
    ** Never mind, we will use a special routine (later) to do this.  For now
    ** the gwu_deltcb() routine needs to get a gw_rcb struct, so that it can
    ** tell whether the database id's match as well as table id's.  Obviously
    ** we don't have a gw_rcb here...and we want to delete for all databases.
    */
    /* free all tcb's */
    while (Gwf_facility->gwf_tcb_list)
    {
	status = gwu_deltcb(&Gwf_facility->gwf_tcb_list->gwt_table.tbl_id,
			    GWU_TCB_MANDATORY, &err);
	if (status != E_DB_OK)
	{
	    gwf_error(err.err_code, GWF_INTERR, 0);
	    badstat = TRUE;
	    break;
	}
    }
#endif

    /* get rid of the memory pool */
    if ((status = ulm_shutdown(&Gwf_facility->gwf_ulm_rcb)) != E_DB_OK)
    {
	gwf_error(Gwf_facility->gwf_ulm_rcb.ulm_error.err_code, GWF_INTERR, 0);
	gwf_error(E_GW0311_ULM_SHUTDOWN_ERROR, GWF_INTERR, 0);
	badstat = TRUE;
    }
    
    /* deallocate Gwf_facility memory */
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_facility = DB_GWF_ID;
    scf_cb.scf_scm.scm_in_pages = (sizeof(GW_FACILITY)/SCU_MPAGESIZE+1);
    scf_cb.scf_scm.scm_addr = (char *)Gwf_facility;
    status = scf_call(SCU_MFREE, &scf_cb);
    if (status != E_DB_OK)
    {
	gwf_error(scf_cb.scf_error.err_code, GWF_INTERR, 0);
	gwf_error(E_GW0301_SCU_MFREE_ERROR, GWF_INTERR, 0);
	badstat = TRUE;
    }

    Gwf_facility = NULL;

    if (badstat == TRUE)
    {
	status = E_DB_ERROR;
	gw_rcb->gwr_error.err_code = E_GW0201_GWF_TERM_ERROR;
    }

    return(status);
}

/*{
** Name: gwf_svr_info	    - return information about GW's in this server
**
** Description:
**	This routine is used by DMF to query the GW status of this server.
**	DMF needs to know whether this server supports any gateways, and, if
**	so, whether any of those gateway's support transaction handling.
**
** Inputs:
**	gw_rcb->		Standard GWF control block
**
** Outputs:
**	gw_rcb->		Standard GWF control block
**	    gwr_gw_active	non-zero if any gateways are active
**	    gwr_gw_xacts	non-zero if any gateways support xacts
**	    error.err_code	One of the following error numbers:
**				E_GW021C_GWF_SVR_INFO_ERROR
**
** Returns:
**	E_DB_OK			Function completed normally
**	E_DB_ERROR		Funtion failed.
**
** History:
**	9-apr-90 (bryanp)
**	    Created.
**	7-oct-92 (daveb)
**	    prototyped.
*/
DB_STATUS
gwf_svr_info( GW_RCB *gw_rcb )
{
    if (Gwf_facility == 0)
    {
	gw_rcb->gwr_error.err_code = E_GW021C_GWF_SVR_INFO_ERROR;
	return (E_DB_ERROR);
    }

    gw_rcb->gwr_gw_active = Gwf_facility->gwf_gw_active;
    gw_rcb->gwr_gw_xacts  = Gwf_facility->gwf_gw_xacts;

    return (E_DB_OK);
}

/*{
** Name: gwf_def_pool_size  - calculate GWF default memory pool size
**
** Description:
**	This function calculates the default GWF memory pool size. The default
**	size is set to
**
**	    GWF_FACMEM_DEFAULT + (num_users * GWF_SESMEM_DEFAULT)
**
**	where GWF_FACMEM_DEFAULT and GWF_SESMEM_DEFAULT are defined in
**	<gwfint.h> and stand for the default amount of memory for the facility
**	as a whole and for each particular session, respectively.
**
**	Eventually we may also add server startup parameters to allow
**	individual servers more fine-grained control over these values.
**
** Inputs:
**	None
**
** Outputs:
**	None
**
** Returns:
**	pool_size		the default pool size to use for this server
**				0 indicates that we had trouble determining
**				the number of users in this server; this is
**				probably a server-fatal condition.
**
** History:
**	3-apr-90 (bryanp)
**	    Created.
**	8-oct-92 (daveb)
**	    prototyped
**	17-aug-2004 (thaju02)
**	    For now, limit pool size to 2Gig. 
*/
static SIZE_TYPE
gwf_def_pool_size(void)
{
    DB_STATUS	status;
    SIZE_TYPE	pool_size = 0;
    i4		num_users;
    SCF_CB	scf_cb;
    SCF_SCI	sci_list[1];

    /* Get the number of users in this server. */

    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_facility = DB_GWF_ID;
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_ptr_union.scf_sci = (SCI_LIST *)sci_list;
    scf_cb.scf_len_union.scf_ilength = 1;
    sci_list[0].sci_length = sizeof(i4);
    sci_list[0].sci_code = SCI_NOUSERS;
    sci_list[0].sci_aresult = (PTR)(&num_users);
    sci_list[0].sci_rlength = NULL;

    status = scf_call(SCU_INFORMATION, &scf_cb);

    if (status != E_DB_OK)
    {
	gwf_error(scf_cb.scf_error.err_code, GWF_INTERR, 0);
	gwf_error(E_GW0302_SCU_INFORMATION_ERROR, GWF_INTERR, 0);

	pool_size = 0;	/* indicate error to caller */
    }
    else
    {
	pool_size = GWF_FACMEM_DEFAULT + (num_users * GWF_SESMEM_DEFAULT);
	if (pool_size < 0)
	{
	    TRdisplay("gwf_def_pool_size: overflow, capping to MAXI4\n");
	    pool_size = MAXI4;
	}
    }	

    return(pool_size);
}
