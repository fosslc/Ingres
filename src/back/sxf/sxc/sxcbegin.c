/*
**Copyright (c) 2004 Ingres Corporation
*/

# include    <compat.h>
# include    <gl.h>
# include    <pc.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <cs.h>
# include    <cv.h>
# include    <pc.h>
# include    <lk.h>
# include    <me.h>
# include    <st.h>
# include    <tm.h>
# include    <scf.h>
# include    <ulf.h>
# include    <ulm.h>
# include    <sxf.h>
# include    <sxfint.h>

/*
** Name: SXCBEGIN.C - SXF begin session routines
**
** Description:
**	This file contains the routines that are used to begin a 
**	SXF session.
**
**	sxc_bgn_session - begin a SXF session.
**	sxf_build_scb   - build a SXF session control block.
**
** History:
**	11-aug-1992 (markg)
**	    Initial creation.
**	26-oct-1992 (markg)
**	    Updated error handling.
**      10-mar-1993 (markg)
**          Fixed problem where errors returned from ulm were not getting
**          handled correctly.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-nov-93 (robf)
**          ingres_low/high now pure labels, change code accordingly.
**	05-Aug-1997 (jenjo02)
**	    Move closestream after referencing sxf_scb for the last time.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	19-Aug-1997 (jenjo02)
**	    Distinguish between private and shared ULM streams.
**	    Piggyback ulm_palloc() calls with ulm_openstream() where appropriate.
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LKcreate_list() prototype.
**	15-Dec-1999 (jenjo02)
**	    Removed DB_DIS_TRAN_ID parm from LKcreate_list() prototype.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

/*
** Name: SXC_BGN_SESSION - begin a SXF session
**
** Description:
**
** 	This routine is used to begin a SXF session, it is called at
**	session startup time via the sxf_call(SXC_BGN_SESSION) request. 
**	All requests made to the SXF must be made by sessions that have
**	previously been registered with SXF via this routine. The exceptions
**	to this are the facility startup and shutdown requests.
**
**	Overview of algorithm:-
**
**	Validate the parameters passed in the SXF_RCB.
**	Build the SXF_SCB for this session.
**	Call sxac_bgn_session() to register session with the auditing system.
**	Link SXF_SCB onto the list in the server control block
**
** Inputs:
**	rcb
**	    .sxf_server		Pointer to SXF server control block.
**	    .sxf_scb		Pointer to the session control block.
**	    .sxf_sess_id	The session identifier for this session.
**
** Outputs:
**	rcb
**	    .sxf_scb		Pointer to the session control block.
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
**      10-mar-1993 (markg)
**          Fixed problem where errors returned from ulm were not getting
**          handled correctly.
**	5-jul-93 (robf)
**	    Initialize per-session trace vectors
**      29-jul-1993 (stephenb)
**          Fixed problem where SXF would spuriosly dissalow new sessions
**          with E_SX000A after max sessions had realy been exeeded but session
**          count was subsequently decreased
*/
DB_STATUS
sxc_bgn_session(
    SXF_RCB	*rcb)
{
    DB_STATUS		status = E_DB_OK;
    i4		err_code = E_SX0000_OK;
    i4		local_err;
    SXF_SCB		*scb;
    bool		scb_built = FALSE;

    for (;;)
    {
	/*
	** Check that the session quota is not exceeded and that
	** the parameters look OK.
	*/
        /*
	** 29-jun-93 (stephenb): moved incriment of sxf_active_ses after test
	** for exeeded session limit to avoid problem where: when session limit
	** was exeeded SXF would continue to incriment number of sessions and
	** then refuse the connection.
	*/
	CSp_semaphore(TRUE, &Sxf_svcb->sxf_svcb_sem);
	if (Sxf_svcb->sxf_active_ses > Sxf_svcb->sxf_max_ses)
	{
	    CSv_semaphore(&Sxf_svcb->sxf_svcb_sem);
	    err_code = E_SX000A_QUOTA_EXCEEDED;
	    break;
	}
	Sxf_svcb->sxf_active_ses++;
	CSv_semaphore(&Sxf_svcb->sxf_svcb_sem);

	if ((Sxf_svcb->sxf_svcb_status & SXF_SNGL_USER) == 0 &&
	   (rcb->sxf_scb == NULL || rcb->sxf_sess_id == (CS_SID)0))
	{
	    err_code = E_SX0004_BAD_PARAMETER;
	    break;
	}
	    
	/* Get an SCB and register the session with the audit system */
	status = sxc_build_scb(rcb, &scb, &local_err);
	if (status != E_DB_OK)
	{
	    err_code = local_err;
	    if (err_code > E_SXF_INTERNAL)
	        _VOID_ ule_format(err_code, NULL,
		        ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	scb_built = TRUE;

	status = sxac_bgn_session(scb, &local_err);
	if (status != E_DB_OK)
	{
	    err_code = local_err;
	    if (err_code > E_SXF_INTERNAL)
	        _VOID_ ule_format(err_code, NULL,
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	CSp_semaphore(TRUE, &Sxf_svcb->sxf_svcb_sem);
	if (Sxf_svcb->sxf_scb_list != NULL)
	    Sxf_svcb->sxf_scb_list->sxf_prev = scb;
	scb->sxf_next = Sxf_svcb->sxf_scb_list;
	Sxf_svcb->sxf_scb_list = scb;
	CSv_semaphore(&Sxf_svcb->sxf_svcb_sem);

	/* If in single user mode return the scb to the caller */
	if (Sxf_svcb->sxf_svcb_status & SXF_SNGL_USER)
	    rcb->sxf_scb = (PTR) scb;

	break;
    }

    /* Clean up after any errors */
    if (err_code != E_SX0000_OK)
    {	
	CSp_semaphore(TRUE, &Sxf_svcb->sxf_svcb_sem);
	Sxf_svcb->sxf_active_ses--;
	CSv_semaphore(&Sxf_svcb->sxf_svcb_sem);

	if (scb_built)
	    _VOID_ sxc_destroy_scb(scb, &local_err);

	if (err_code > E_SXF_INTERNAL)
	    err_code = E_SX0009_BEGIN_SESSION;

	rcb->sxf_error.err_code = err_code;
	if (status == E_DB_OK)
	    status = E_DB_ERROR;	
    }

    return (status);
}

/*
** Name: SXC_BUILD_SCB - build a SXF session control block
**
** Description:
**	This routine is used to build a session control block for SXF. 
**	It will be called at session startup time. If the facility is 
**	running in single user mode this routine will be required to
**	allocate the memory for the SXF_SCB, if the facility is running in
**	multi-user mode the memory will already have been allocated by SCF.
**
**	Overview of algorithm:
**
**	Validate the parameters passed to the routine
**	Open a memory stream from the SXF pool
**	If SXF is multi-user mode
**	    Call SCF to get user and database information
**	    Initialize SCB
**	Else
**	    Allocate SCB from memory stream
**	    Initialize SCB from parameters in RCB
**	Create a lock list
**	    
** Inputs:
**	rcb			the SXF_RCB associated with this request
**
** Outputs:
**	scb			pointer to the session control block
**	err_code		pointer to error code
**
** Returns:
**	DB_STATUS
**
** History:
**	23-sept-92 (markg)
**	    Initial creation.
**	26-oct-1992 (markg)
**	    Updated error handling.
**	10-may-1993 (robf)
**	    Lookup/Allocate database cb (SXF_DBCB) if necessary.
**	    Also copy over the session security label if a B1 system.
**	    Allocate session trace block
**	    Save session id for auditing.
**	08-sep-93 (swm)
**	    Cast rcb->sxf_sess_id to SCF_SESSION rather than i4 since
**	    SCF_SESSION is no longer defined to be i4.
**	30-sep-93 (robf)
**          For single user servers, only copy over security label if
**	    provided.
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for assignment to sxf_owner which has changed type
**	    to PTR.
**	5-apr-94 (robf)
**          Improved error handling, some intermediate errors could  be
**	    lost with only the  SX1006 getting logged.
**	5-jul-94 (robf)
**	    Make lock list non-interruptable.
**	05-Aug-1997 (jenjo02)
**	    Move closestream after referencing sxf_scb for the last time.
**	2-jul-2004 (stephenb)
**	    Fix up SCI array size for SCF call.
*/
DB_STATUS
sxc_build_scb(
    SXF_RCB	*rcb,
    SXF_SCB	**scb,
    i4	*err_code)
{
    DB_STATUS		status = E_DB_OK;
    i4		sxf_stat = Sxf_svcb->sxf_svcb_status;
    i4		local_err;
    ULM_RCB		ulm_rcb;
    LK_UNIQUE		lk_unique;
    CL_ERR_DESC		cl_err;
    bool		stream_open = FALSE;
    bool		lock_list_created = FALSE;
    bool		got_dbcb = FALSE;
    SXF_SCB		*sxf_scb;
    SXF_DBCB		*sxf_dbcb;
    DB_DB_NAME		db_name;
    i4			result;

    *scb = NULL;
    *err_code = E_SX0000_OK;

    for (;;)
    {
	/* 
	** Check that the parameters are valid. If the SXF_SNGL_USER flag is
	** not set rcb->sxf_scb must contain the valid address ofa SCB. If 
	** the SXF_SNGL_USER flag is set, the sxf_user, sxf_ruser, sxf_ustat 
	** and sxf_db_name fields in the rcb must be set.
	**
	** NOTE: since all i4 values are valid for a user status mask,
	** we are unable to validate the contents of rcb->sxf_ustat.
	*/
	if (((sxf_stat & SXF_SNGL_USER) == 0 && rcb->sxf_scb == 0) || 
	   ((sxf_stat & SXF_SNGL_USER) && (rcb->sxf_user == 0 || 
	   rcb->sxf_ruser == 0 || rcb->sxf_db_name == 0)))
	{
	    *err_code = E_SX0004_BAD_PARAMETER;
	    break;
	}

	/*
	** Open a memory stream from the SXF pool and build the SXF_SCB.
	** If the facility is running in multi-user mode we have to get 
	** the user and database information from SCF. If the facility
	** is running in single user mode then allocate a SXF_SCB from
	** the memory pool and take the user informatiom from the SXF_RCB.
	*/
	ulm_rcb.ulm_facility = DB_SXF_ID;
	ulm_rcb.ulm_poolid = Sxf_svcb->sxf_pool_id;
	ulm_rcb.ulm_blocksize = 0;
	ulm_rcb.ulm_memleft = &Sxf_svcb->sxf_pool_sz;
	ulm_rcb.ulm_streamid_p = &ulm_rcb.ulm_streamid;
	/* This stream will be exposed only to this session */
	ulm_rcb.ulm_flags = ULM_PRIVATE_STREAM;
	if (sxf_stat & SXF_SNGL_USER)
	{
	    ulm_rcb.ulm_flags |= ULM_OPEN_AND_PALLOC;
	    ulm_rcb.ulm_psize = sizeof (SXF_SCB);
	}
	status = ulm_openstream(&ulm_rcb);
	if (status != E_DB_OK)
	{
	    _VOID_ ule_format(ulm_rcb.ulm_error.err_code, NULL,
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
		*err_code = E_SX1002_NO_MEMORY;
	    else
		*err_code = E_SX106B_MEMORY_ERROR;

	    break;
	}
	stream_open = TRUE;


	if ((sxf_stat & SXF_SNGL_USER) == 0)
	{
	    /*
	    ** Multi-user, lookup in SCF
	    */
	    SCF_CB		scf_cb;
	    SCF_SCI		sci_list[3];
	 
	    sxf_scb = (SXF_SCB *) rcb->sxf_scb;
	    MEfill(SXF_SESSID_LEN, ' ', (PTR)&sxf_scb->sxf_sessid);

 	    scf_cb.scf_length = sizeof(SCF_CB);
	    scf_cb.scf_type = SCF_CB_TYPE;
	    scf_cb.scf_facility = DB_SXF_ID;
	    scf_cb.scf_session = (SCF_SESSION) rcb->sxf_sess_id;
	    scf_cb.scf_ptr_union.scf_sci = (SCI_LIST *) sci_list;
	    scf_cb.scf_len_union.scf_ilength = 3;
	    sci_list[0].sci_length  = sizeof(DB_DB_NAME);
	    sci_list[0].sci_code    = SCI_DBNAME;
	    sci_list[0].sci_aresult = (char *) &db_name;
	    sci_list[0].sci_rlength = NULL;
	    sci_list[1].sci_length  = sizeof(DB_OWN_NAME);
	    sci_list[1].sci_code    = SCI_RUSERNAME;
	    sci_list[1].sci_aresult = (char *) &sxf_scb->sxf_ruser;
	    sci_list[1].sci_rlength = NULL;
	    sci_list[2].sci_length  = SXF_SESSID_LEN;
	    sci_list[2].sci_code    = SCI_SESSID;
	    sci_list[2].sci_aresult = (char *) &(sxf_scb->sxf_sessid);
	    sci_list[2].sci_rlength = NULL;
	    status = scf_call(SCU_INFORMATION, &scf_cb);
	    if (status != E_DB_OK)
	    {
		*err_code = scf_cb.scf_error.err_code;
		_VOID_ ule_format(*err_code, NULL,
		    ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
		*err_code = E_SX1005_SESSION_INFO;
		break;
	    }

	    sxf_scb->sxf_ustat = 0;
	    MEfill(sizeof (DB_OWN_NAME), ' ', (PTR)&sxf_scb->sxf_user);
	}
	else 
	{
	    /* Single user */
	    char id[CV_HEX_PTR_SIZE+1];
	    i4 i, len;

	    if(Sxf_svcb->sxf_pool_sz < Sxf_svcb->sxf_stats->mem_low_avail)
		Sxf_svcb->sxf_stats->mem_low_avail=Sxf_svcb->sxf_pool_sz;
	    sxf_scb = (SXF_SCB *) ulm_rcb.ulm_pptr;
	    sxf_scb->sxf_lock_list = Sxf_svcb->sxf_lock_list;
	    sxf_scb->sxf_ustat = rcb->sxf_ustat;
	    STRUCT_ASSIGN_MACRO(*rcb->sxf_db_name, db_name);
	    STRUCT_ASSIGN_MACRO(*rcb->sxf_ruser, sxf_scb->sxf_ruser);
	    STRUCT_ASSIGN_MACRO(*rcb->sxf_user, sxf_scb->sxf_user);
	    MEfill(SXF_SESSID_LEN, ' ', (PTR)&sxf_scb->sxf_sessid);
	    /* Safely copy hex'ed SCB addr as session ID */
	    CVptrax((PTR)sxf_scb, &id[0]);
	    len = STlength(id);
	    i = len - SXF_SESSID_LEN;
	    if (i < 0)
		i = 0;
	    if (len > SXF_SESSID_LEN)
		len = SXF_SESSID_LEN;
	    MEcopy(&id[i], len, &sxf_scb->sxf_sessid[0]);
	}

	/* Make  sure db info is initialized to NULL, otherwise
	** may point to random value causing lower levels to fail
	*/
	sxf_scb->sxf_db_info=NULL;

	/* initialize the remainder of the SCB */
	sxf_scb->sxf_next = NULL;
	sxf_scb->sxf_prev = NULL;
	sxf_scb->sxf_cb_type = SXFSCB_CB;
	sxf_scb->sxf_owner = (PTR)DB_SXF_ID;
	sxf_scb->sxf_ascii_id = SXFSCB_ASCII_ID;
	sxf_scb->sxf_scb_mem = ulm_rcb.ulm_streamid;
	sxf_scb->sxf_sxap_scb = NULL;
        sxf_scb->sxf_flags_mask=0;

	/* Initialize per-session trace macro */
        ult_init_macro(&sxf_scb->sxf_trace, 
		SXS_TBIT_COUNT, SXS_TVALUE_COUNT, SXS_TVALUE_COUNT);

	/* Create per-session lock list */
	if (LKcreate_list((LK_NONPROTECT|LK_ASSIGN|LK_NOINTERRUPT), 0, 
	    &lk_unique, &sxf_scb->sxf_lock_list, 10, &cl_err) != OK)
	{
	    *err_code = E_SX1004_LOCK_CREATE;
	    break;
	}
	lock_list_created = TRUE;
	/*
	** Initialize the DBCB, we must do this AFTER the session lock list
	** is created.
	*/
	sxf_dbcb=sxc_getdbcb(sxf_scb, &db_name, TRUE, err_code );
	if (!sxf_dbcb)
	{
		/*
		** Unable to find/allocate a database control block
		*/
		/*
		** Log lower level error
		*/
	        _VOID_ ule_format(*err_code, &cl_err,
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
		
	        *err_code = E_SX1077_SXC_GETDBCB;
		status=E_DB_ERROR;
		break;
	}
	/*
	** Connect the DBCB to the SCB
	*/
	sxf_scb->sxf_db_info=sxf_dbcb;
	/* Increment number of sessions using this db, need semaphore for this*/
	CSp_semaphore(TRUE, &Sxf_svcb->sxf_db_sem);
	sxf_dbcb->sxf_usage_cnt++;	
	CSv_semaphore( &Sxf_svcb->sxf_db_sem);

	got_dbcb=TRUE;

	*scb = sxf_scb;
	break;
    }

    /* Clean up after any errors */
    if (*err_code != E_SX0000_OK)
    {	
	if (got_dbcb)
	{
		CSp_semaphore(TRUE, &Sxf_svcb->sxf_db_sem);
		sxf_dbcb->sxf_usage_cnt--;	
		CSv_semaphore( &Sxf_svcb->sxf_db_sem);
		sxf_scb->sxf_db_info=NULL;
	}

	if (lock_list_created)
	    _VOID_ LKrelease(LK_ALL, sxf_scb->sxf_lock_list, 
			NULL, NULL, NULL, &cl_err);

	if (stream_open)
	    _VOID_ ulm_closestream(&ulm_rcb);

	if (*err_code > E_SXF_INTERNAL)
	{
	   _VOID_ ule_format(*err_code, NULL,
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code = E_SX1006_SCB_BUILD;
	}

	if (status == E_DB_OK)
	    status = E_DB_ERROR;	
    }

    return (status);
}


