/*
**Copyright (c) 2004 Ingres Corporation
*/

# include    <compat.h>
# include    <gl.h>
# include    <pc.h>
# include    <sl.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <cs.h>
# include    <pc.h>
# include    <lk.h>
# include    <st.h>
# include    <tm.h>
# include    <adf.h>
# include    <ulf.h>
# include    <sxf.h>
# include    <sxfint.h>
# include    <ulm.h>

/*
** Name: SXCSTART.C - SXF facility startup routines.
**
** Description:
**	This file contains the routines that are used to startup the SXF
**	facility.
**	
**	    sxc_startup - startup SXF
**
** History:
**	11-aug-1992 (markg)
**	    Initial creation.
**	23-Oct-1992 (daveb)
**	    name the semaphore
**	26-oct-1992 (markg)
**	    Updated error handling.
**      7-dec-1992 (markg)
**          Fix AV caused by passing incorrect number of parameters
**          to ule_format.
**	1-feb-1993 (markg)
**	    Return error if user specifies a pool size of less than
**	    SXF_BYTES_PER_SERVER.
**	10-mar-1993 (markg)
**	    Fixed problem where errors returned from ulm were not getting
**	    handled correctly.
**	10-may-1993 (robf)
**	    Initialize startup variables from PM
**	    Add ADF external SOP pointer.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	13-oct-93 (robf)
**          Add st.h
**	10-nov-93 (robf)
**          ingres_low/high now pure labels, change code accordingly.
**	11-may-94 (robf)
**          Don't log parameters for single user servers.
**	01-May-1995 (jenjo02)
**	    Combined CSi|n_semaphore functions calls into single
**	    CSw_semaphore call.
**	10-oct-1996 (canor01)
**	    Changed INGRES_HIGH/LOW to CA_HIGH/LOW.  Changed 'II'
**	    prefix in config file to SystemCfgPrefix.
**	19-Aug-1997 (jenjo02)
**	    Distinguish between private and shared ULM streams.
**	    Piggyback ulm_palloc() calls with ulm_openstream() where appropriate.
**      6-mar-97(rosga02 SEVMS)
**          Check error status that can be from B1secure checks
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LKcreate_list() prototype.
**	15-Dec-1999 (jenjo02)
**	    Removed DB_DIS_TRAN_ID parm from LKcreate_list() prototype.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-Aug-2001 (hanje04)
**	    Bug 105709
**	    Initialize Sxf_svcb->sxf_db_info = NULL to stop SEGV in sxc_getdbcb
**	    on Linux.
**	    
[@history_template@]...
*/

/*
** Name: SXC_STARTUP - startup the SXF facility
**
** Description:
** 	This routine is used to startup the SXF facility, it is called at
**	server startup time via sxf_call(SXC_STARTUP) request. This routine
**	must only be called once during the lifetime of the facility. 
**
**	Overview of algorithm:-
**
**	Validate the parameters passed in the SXF_RCB.
**	Initialize the facility memory pool.
**	Allocate and initialize the SXF_SVCB.
**	Call sxac_startup() to initialize the auditing system.
**	Mark the facility as being online.
**	
** Inputs:
**	rcb
**	    .sxf_mode		Shows if the facility is to started in
**				single or multi-user mode.
**	    .sxf_pool_sz	Number of bytes to allocate for the SXF
**				memory pool.
**	    .sxf_max_active_ses	Maximum number of active sessions allowed
**				in the facility, used to calculate the
**				size of memory pool to allocate, if 
**				sxf_pool_sz is not specified.
**
** Outputs:
**	rcb
**	    .sxf_server		pointer to the server control block (SXF_SVCB)
**	    .sxf_scb_sz		the size of a SXF session control block	
**	    .sxf_error		error code returned to the caller
**
** Returns:
**	DB_STATUS
**
** History:
**	11-aug-1992 (markg)
**	    Initial creation.
**	26-oct-1992 (markg)
**	    Updated error handling.
**	14-dec-1992 (robf)
**	    Allocate SXF statistics block.
**	17-dec-1992 (robf)
**	    Better memory sizing, now include constant overhead.
**      7-dec-1992 (markg)
**          Fix AV caused by passing incorrect number of parameters
**          to ule_format.
**	1-feb-1993 (markg)
**	    Return error if user specifies a pool size of less than
**	    SXF_BYTES_PER_SERVER.
**	10-mar-1993 (markg)
**	    Fixed problem where errors returned from ulm were not getting
**	    handled correctly.
**	23-Oct-1992 (daveb)
**	    name the semaphore
**	10-may-1993 (robf)
**	    Initialize/name the DBCB semaphore
**	    Initialize startup variables from PM
**	    Check the SXF_B1_SERVER parameter to make sure the facility is
**	    configured as B1 properly.
**	    Initialize label cache number from PM
**	30-sep-93 (robf)
**          Always initialize dmf_cptr, even in single user servers
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for assignment to sxf_owner which has changed type
**	    to PTR.
**	9-feb-94 (robf)
**          Attach to MO.
**	25-feb-94 (robf)
**          Label cache PM value should have additional class value.
**	12-may-94 (robf)
**          Change single-server check to rcb since the svcb hasn't been
**	    allocated yet.
**	5-jul-94 (robf)
**	    Make lock list non-interruptable.
**      30-May-2006 (hanal04) bug 116149
**          Initialise Sxf_svcb->sxf_db_info to prevent SIGSEGV in
**          sxc_getdb().
**	11-Jun-2010 (kiria01) b123908
**	    Init ulm_streamid_p for ulm_openstream to fix potential segvs.
*/
DB_STATUS
sxc_startup(
    SXF_RCB	*rcb)
{
    DB_STATUS		status = E_DB_OK;
    ULM_RCB		ulm_rcb;
    SXF_SCB		*sxf_scb;
    i4		err_code = E_SX0000_OK;
    i4		local_err;
    LK_UNIQUE		lk_unique;
    CL_ERR_DESC		cl_err;
    char		*pmval;
    i4		lab_cache_num;
    i4		pool_size;
    char		cfgstring[1024];

    rcb->sxf_server = NULL;
    rcb->sxf_scb_sz = 0;

    for (;;)
    {
	/*
	** Check that the facility is not already running and that the 
	** parameter values passed in the request control block look OK.
	*/
	if (Sxf_svcb)
	{
	    err_code = E_SX0006_FACILITY_ACTIVE;
	    break;
	}

	if ((rcb->sxf_mode != SXF_MULTI_USER   &&
	    rcb->sxf_mode  != SXF_SINGLE_USER) ||
	    rcb->sxf_max_active_ses <= 0)
	{
	    err_code = E_SX0004_BAD_PARAMETER;
	    break;
	}

	if (rcb->sxf_pool_sz < SXF_BYTES_PER_SERVER &&
	    rcb->sxf_pool_sz != 0)
	{
	    _VOID_ ule_format(E_SX106A_BAD_POOL_SIZE, NULL, 
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    err_code = E_SX0004_BAD_PARAMETER;
	    break;
	}

	/*
	** Call ULM to initialize the facility memory pool. If the size
	** of the pool has been specified in the RCB we use this value,
	** is the size hasn't been specified (or it contains a negative 
	** value) the pool size is calculated from the maximum allowable
	** number of active sessions specified. 
	*/
	ulm_rcb.ulm_facility = DB_SXF_ID;
	if (rcb->sxf_pool_sz>0)
	{
		ulm_rcb.ulm_sizepool=rcb->sxf_pool_sz;
	}
	else
	{
		ulm_rcb.ulm_sizepool= SXF_BYTES_PER_SERVER  +
	    		(rcb->sxf_max_active_ses * SXF_BYTES_PER_SES);
	}
	pool_size=ulm_rcb.ulm_sizepool;
	ulm_rcb.ulm_blocksize = 0;
	status = ulm_startup(&ulm_rcb);
	if (status != E_DB_OK)
	{
	    err_code = E_SX106B_MEMORY_ERROR;
	    _VOID_ ule_format(ulm_rcb.ulm_error.err_code, NULL, 
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    _VOID_ ule_format(err_code, NULL, 
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	/* Build the server control block */
        ulm_rcb.ulm_memleft = &ulm_rcb.ulm_sizepool;

	/* Open a shared stream and allocate SXF_SVCB with one call */
	ulm_rcb.ulm_flags = ULM_SHARED_STREAM | ULM_OPEN_AND_PALLOC;
	ulm_rcb.ulm_psize = sizeof (SXF_SVCB);
	ulm_rcb.ulm_streamid_p = NULL;

	status = ulm_openstream(&ulm_rcb);		
	if (status != E_DB_OK)
	{
	    if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
		err_code = E_SX1002_NO_MEMORY;
	    else
		err_code = E_SX106B_MEMORY_ERROR;

	    _VOID_ ule_format(ulm_rcb.ulm_error.err_code, NULL, 
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    _VOID_ ule_format(err_code, NULL, 
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	Sxf_svcb = (SXF_SVCB *) ulm_rcb.ulm_pptr;
	Sxf_svcb->sxf_next = NULL;
	Sxf_svcb->sxf_prev = NULL;
	Sxf_svcb->sxf_db_info = (SXF_DBCB*)0;
	Sxf_svcb->sxf_length = sizeof (SXF_SVCB);
	Sxf_svcb->sxf_cb_type = SXFSVCB_CB;
	Sxf_svcb->sxf_owner = (PTR)DB_SXF_ID;
	Sxf_svcb->sxf_ascii_id = SXFSVCB_ASCII_ID;
	Sxf_svcb->sxf_svcb_status = 0;
	Sxf_svcb->sxf_svcb_status |= SXF_STARTUP;
	Sxf_svcb->sxf_db_info = NULL;
	if (rcb->sxf_mode == SXF_SINGLE_USER)
	    Sxf_svcb->sxf_svcb_status |= SXF_SNGL_USER;
	if ((rcb->sxf_status & 
	    (SXF_C2_SERVER|SXF_STAR_SERVER)) == SXF_C2_SERVER)
	{
	    /* There is no support for C2 in STAR servers */
	    Sxf_svcb->sxf_svcb_status |= SXF_C2_SECURE;
	}

	PCpid(&(Sxf_svcb->sxf_pid));

	ulm_rcb.ulm_psize = sizeof (SXF_STATS);
	status = ulm_palloc(&ulm_rcb);
	if (status != E_DB_OK)
	{
	    if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
		err_code = E_SX1002_NO_MEMORY;
	    else
		err_code = E_SX106B_MEMORY_ERROR;

	    _VOID_ ule_format(ulm_rcb.ulm_error.err_code, NULL, 
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    _VOID_ ule_format(err_code, NULL, 
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	Sxf_svcb->sxf_stats = (SXF_STATS *) ulm_rcb.ulm_pptr;
	Sxf_svcb->sxf_stats->mem_at_startup = pool_size; /* Startup pool size */
	Sxf_svcb->sxf_stats->mem_low_avail = pool_size; /* Startup pool size */
	Sxf_svcb->sxf_pool_id = ulm_rcb.ulm_poolid;
	Sxf_svcb->sxf_pool_sz = *ulm_rcb.ulm_memleft;
	Sxf_svcb->sxf_svcb_mem = ulm_rcb.ulm_streamid;
	Sxf_svcb->sxf_scb_list = NULL;
	Sxf_svcb->sxf_rscb_list = NULL;
	Sxf_svcb->sxf_active_ses = 0;
	Sxf_svcb->sxf_max_ses = 
	    (rcb->sxf_mode & SXF_SINGLE_USER) ? 1 : rcb->sxf_max_active_ses;
	Sxf_svcb->sxf_aud_state = NULL;
	Sxf_svcb->sxf_write_rscb = NULL;
	Sxf_svcb->sxf_sxap_cb = NULL;
	Sxf_svcb->sxf_act_on_err = SXF_ERR_SHUTDOWN;
        Sxf_svcb->sxf_dmf_cptr = rcb->sxf_dmf_cptr;

	if (CSw_semaphore(&Sxf_svcb->sxf_svcb_sem, CS_SEM_SINGLE,
				"SXF sem" ) != OK)
	{
	    err_code = E_SX1003_SEM_INIT;
	    _VOID_ ule_format(err_code, NULL, 
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	if (CSw_semaphore(&Sxf_svcb->sxf_db_sem, CS_SEM_SINGLE,
				"SXF DBCB sem" ) != OK)
	{
	    err_code = E_SX1003_SEM_INIT;
	    _VOID_ ule_format(err_code, NULL, 
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	if (LKcreate_list((LK_NONPROTECT|LK_ASSIGN|LK_NOINTERRUPT), 0, 
	    &lk_unique, &Sxf_svcb->sxf_lock_list, 10, &cl_err) != OK)
	{
	    err_code = E_SX1004_LOCK_CREATE;
	    _VOID_ ule_format(err_code, &cl_err, 
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	/* Startup the SXF auditing subsystem */
	status = sxac_startup(&local_err);
	if (status != E_DB_OK)
	{
	    err_code = local_err;
	    if (err_code > E_SXF_INTERNAL)
	        _VOID_ ule_format(err_code, NULL, 
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	/* Attach to MO. If this fails, continue processing */
	(VOID) sxc_mo_attach();

	/* Mark the facility as being online and set up the output values */
	Sxf_svcb->sxf_svcb_status |= SXF_ACTIVE;
	Sxf_svcb->sxf_svcb_status &= ~(SXF_STARTUP);
	rcb->sxf_server = (PTR) Sxf_svcb;
	rcb->sxf_scb_sz = sizeof (SXF_SCB);
	break;
    }

    /* Handle any errors */
    if (err_code != E_SX0000_OK)
    {
	if (err_code > E_SXF_INTERNAL)
	    err_code = E_SX0008_BAD_STARTUP;

	rcb->sxf_error.err_code = err_code;
	if (status == E_DB_OK)
	    status = E_DB_ERROR;
    }

    return (status);
}
