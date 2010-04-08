/*
** Copyright (c) 2004 Ingres Corporation
**
*/

# include    <compat.h>
# include    <gl.h>
# include    <sl.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <pc.h>
# include    <cs.h>
# include    <lk.h>
# include    <cx.h>
# include    <tm.h>
# include    <di.h>
# include    <lo.h>
# include    <me.h>
# include    <lg.h>
# include    <st.h>
# include    <nm.h>
# include    <er.h>
# include    <pm.h>
# include    <cv.h>
# include    <st.h>
# include    <tr.h>
# include    <ulf.h>
# include    <ulm.h>
# include    <sxf.h>
# include    <sxfint.h>
# include    <sxap.h>
# include    "sxapint.h"

/*
** Name: SXAPC.C - control routines for the SXAP auditing system
**
** Description:
**      This file contains all of the control routines used by the SXF
**      low level auditing system, SXAP.
**
**          sxap_startup     - initialize the auditing system
**          sxap_bgn_session - register a session with the auditing system
**          sxap_shutdown    - shutdown the auditing system
**          sxap_end_session - remove a session from the auditing system
**
** History:
**      20-aug-1992 (markg)
**          Initial creation as stubs.
**      03-Dec-1992 (markg)
**          Fixed timing bug in sxap_shutdown().
**      10-mar-1993 (markg)
**          Fixed problem where errors returned from ulm were not getting
**          handled correctly. Also cleaned up lock handling code for
**	    clusters.
**	11-mar-1993 (markg)
**	    Fixed bug in sxap_chenge_file() where if all the audit files 
**	    are full, and SXAP_STOPAUDIT is set, auditing would not get 
**	    disabled cleanly.
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	27-jul-93 (robf)
**	    Add improved error message E_SX10A6_STOPAUDIT_LOGFULL when
**	    all audit logs are used.
**	13-aug-93 (stephenb)
**	    Integrate following change made by robf in es2dev branch
**          Add improved error message E_SX10A6_STOPAUDIT_LOGFULL when
**          all audit logs are used.
**	16-aug-93 (stephenb)
**	    Removed (PTR) cast in NULL parameters in calls to PCcmdline()
**	    in order to eliminate prototype warnings. Also re-cast as
**	    (LOCATION *)
**	24-aug-93 (wolf)
**	    Fix typo in yesterday's change
**	1-oct-93 (stephenb)
**	    replaced NULL lock value parameter in call to LKrequest() when
**	    converting lock down, to a real address. Also fixed prototype
**	    warnings (again).
**	19-oct-93 (stephenb)
**	    Change PM parameters in calls to PMget() to be compatible with CBF.
**	20-oct-93 (robf)
**	    Updated sxap_restart_audit to take a requested audit log name,
**	    generally updated error handling/file processing.
**      01-jun-94 (arc)
**          Remove duplicate initialisation of SXAP semaphore and
**          'break' if shared memory buffer count incorrect in
**          sxap_startup.
**	01-May-1995 (jenjo02)
**	    Combined CSi|n_semaphore functions calls into single
**	    CSw_semaphore call.
**      12-jun-1995 (canor01)
**          semaphore protect memory allocation (MCT)
**      03-jun-1996 (canor01)
**          New ME for operating-system threads does not need external
**          semaphores.
**	23-sep-1996 (canor01)
**	    Move global data definitions to sxapdata.c.
**	18-feb-1997 (canor01)
**	    Restored the call to MEfree_pages in sxap_shutdown, because it 
**	    is required for the subsequent MEsmdestroy to work on NT.
** 	28-jul-1997 (rosga02)
**	    Fix bug 83925.	
**	04-Aug-1997 (jenjo02)
**	    Removed sxf_incr(). It's gross overkill to semaphore-protect a
**	    simple statistic.
**	10-aug-1997 (canor01)
**	    Remove the semaphore before freeing the memory.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	19-Aug-1997 (jenjo02)
**	    Distinguish between private and shared ULM streams.
**	    Piggyback ulm_palloc() calls with ulm_openstream() where appropriate.
**  12-Dec-97 (bonro01)
**      Added cleanup code for cross-process semaphores during
**      MEsmdestroy().
**	22-dec-1998 (kosma01)
**          In sxap_stamp_cmp,
**	    For sqs_ptx do a numeric compare rather than a string compare.
**	01-mar-1999 (musro02)
**          Modified above change to sxap_stamp_cmp so it is generic.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-dec-2000 (wansh01)
**	    code in line 644 status == E_DB_ERROR should be = not ==    
**	22-sep-2000 (hanch04)
**	    Change int to i4.
**	30-may-2001 (devjo01)
**	    Replace LGcluster() with CXcluster_enabled. s103715.
**	    Corrected forward declarations.
**      14-sep-2005 (horda03) Bug 115198/INGSRV3422
**          On a clustered Ingres installation, LKrequest can return
**          LK_VAL_NOTVALID. This implies that teh lock request has been
**          granted, but integrity of the lockvalue associated with the
**          lock is in doubt. It is the responsility of the caller
**          to decide whether the lockvalue should be reset.
**      01-Oct-2008 (jonj)
**          Replace CSp/v_semaphore(sxap_semaphore) calls with more robust
**          sxap_sem_lock(), sxap_sem_unlock() functions.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
[@history_template@]...
*/

/*
** Global variables owned by this module.
*/
GLOBALREF SXAP_CB  *Sxap_cb;
/*
** Forward/static declarations
*/
static DB_STATUS sxap_stop_audit(i4 *err_code); 
static DB_STATUS sxap_restart_audit(SXAP_STAMP *stamp, 
			i4 *err_code, 
			char	*filename);

/*
** Name: SXAP_STARTUP - startup the low level auditing system
**
** Description:
**
**	This routine initializes the default SXAP auditing system. It should
**	be called once at SXF startup time. Its purpose is to allocate
**	and initialize the resources needed for the running of the low level
**	auditing system. 
**
**	This routine creates / or maps a shared memory segment that will be 
**	used as a buffer for writing audit records. This memory segment will
**	be returned to the system at facility shutdown time by the
**	sxdef_shutdown routine. All processes that have mapped this shared
**	segment hold a shared mode audit control lock.
**
**	This routine also initializes the vector used to access all other 
**	SXAP routines, all these routines must be called using this vector.
**
** 	Algorithm:
**
**	Call sxap_init_cnf()
**	Initialize Sxap_vector.
**	Take audit control lock in shared mode.
**	Create II_AUDIT.MEM shared memory segment.
**	If (segment already exists)
**	    Map II_AUDIT.MEM segment into process space.
**	If this is the first server to start, attempt 
**	to create the current audit file
**	Call sxap_open to open current audit file.
**	Initialize / validate the audit buffer
**
** Inputs:
**	None.
**
** Outputs:
**	rscb			RSCB for the current write audit file.
**	err_code		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**      20-aug-1992 (markg)
**	    Initial creation.
**      10-mar-1993 (markg)
**          Fixed problem where errors returned from ulm were not getting
**          handled correctly. Also cleaned up lock handling code for
**	    clusters.
**	3-may-1993 (markg)
**	    Fixed bug where if there were no free audit files at startup time
**	    the server would fail to start. We now look at the value of the
**	    onlogfull PM resource, if this is set to 'stopaudit' the server
**	    is allowed to start, but with auditing disabled.
**	2-aug-93 (robf)
**	    Add sxap_alter entry point
**	    Signal stopaudit on  startup using E_DB_WARN and error code rather
**	    in sxap_init_cnf() consistent with other code
**	1-oct-93 (stephenb)
**	    It is no longer valid to pass a NULL lock value address to 
**	    LKrequest() when converting a lock down. declared a new variable,
**	    zero'd it out, and replaced NULL parameter with it's address in
**	    LKrequest calls.
**	23-dec-93 (robf)
**          Tell logical layer if auditing was stopped on startup.
**	12-jan-94 (robf)
**          Initialize sxap semaphore
**	3-apr-94 (robf)
**	    Pass write/flush requests to new queue routines.
**	5-apr-94 (stephenb)
**	    Integrate (robf) semaphore changes:
**		Initialize sxap semaphore.
**      01-jun-94 (arc)
**          Remove duplicate initialisation of SXAP semaphore and fix
**          misplaced 'break' if shared memory buffer count incorrect.
**	28-May-2008 (jonj)
**	    When connected to extant share memory, make the "current"
**	    file that which matches the share memory stamp, not the
**	    "most recent", otherwise the checksum will fail. Any 
**	    subsequent writes will detect the stamp change and will
**	    flip to the latest file.
*/
DB_STATUS
sxap_startup(
    SXF_RSCB	*rscb,
    i4 	*err_code)
{
    DB_STATUS	    status = E_DB_OK;
    i4	    local_err;
    STATUS	    cl_status;
    CL_ERR_DESC	    cl_err;
    LK_VALUE	    lk_val;
    LK_LOCK_KEY	    lk_acc_key = {LK_AUDIT, SXAP_LOCK, SXAP_ACCESS, 0, 0, 0, 0};
    LK_LOCK_KEY	    lk_shm_key = {LK_AUDIT, SXAP_LOCK, SXAP_SHMCTL, 0, 0, 0, 0};
    LK_LLID	    lk_ll = Sxf_svcb->sxf_lock_list;
    SXAP_DPG	    *pagebuf;
    SXAP_RSCB	    *rs;
    i4	    nodeid;
    i4	    l_nodeid;
    i4	    pageno;
    SIZE_TYPE	    alloc_pages;
    ULM_RCB	    ulm_rcb;
    SXAP_VECTOR	    *v;
    bool	    shm_lock = FALSE;
    bool	    access_lock = FALSE;
    bool	    shm_created = FALSE;
    bool	    new_server = TRUE;
    bool	    stream_open = FALSE;
    bool	    new_file = TRUE;
    bool	    file_open = FALSE;
    SXAP_STAMP	    stamp;
    i4		    retry;
    bool	    audit_disabled=FALSE;
    SIZE_TYPE	    shm_memsize;
    i4	    	    num_shm_qbuf;
    i4		    i, clflags;

    *err_code = E_SX0000_OK;

    for (;;)
    {
	/* Build the SXAP main control block */
	ulm_rcb.ulm_facility = DB_SXF_ID;
	ulm_rcb.ulm_streamid_p = &Sxf_svcb->sxf_svcb_mem;
	ulm_rcb.ulm_psize = sizeof (SXAP_CB);
	ulm_rcb.ulm_memleft = &Sxf_svcb->sxf_pool_sz;
	status = ulm_palloc(&ulm_rcb);
	if (status != E_DB_OK)
	{
	    if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
		*err_code = E_SX1002_NO_MEMORY;
	    else
		*err_code = E_SX106B_MEMORY_ERROR;

	    _VOID_ ule_format(ulm_rcb.ulm_error.err_code, NULL,
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	Sxap_cb = (SXAP_CB *) ulm_rcb.ulm_pptr;
	Sxap_cb->sxap_curr_rscb = NULL;
	Sxap_cb->sxap_shm = NULL;
	/*
	** Initialize sxap semaphore
	*/
        Sxap_cb->sxap_sem_owner = (CS_SID)NULL;
        Sxap_cb->sxap_sem_pins = 0;
	if (CSw_semaphore(&Sxap_cb->sxap_semaphore, CS_SEM_SINGLE,
				"SXAP sem" ) != OK)
	{
	    *err_code = E_SX1003_SEM_INIT;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	/* Build and initialize the SXAP call vector */
	ulm_rcb.ulm_psize = sizeof (SXAP_VECTOR);
	status = ulm_palloc(&ulm_rcb);
	if (status != E_DB_OK)
	{
	    if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
		*err_code = E_SX1002_NO_MEMORY;
	    else
		*err_code = E_SX106B_MEMORY_ERROR;

	    _VOID_ ule_format(ulm_rcb.ulm_error.err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	Sxap_vector = (SXAP_VECTOR *) ulm_rcb.ulm_pptr;

	v = Sxap_vector;
	v->sxap_init = sxap_startup;
	v->sxap_term = sxap_shutdown;
	v->sxap_begin = sxap_bgn_ses;
	v->sxap_end = sxap_end_ses;
	v->sxap_open = sxap_open;
	v->sxap_close = sxap_close;
	v->sxap_pos = sxap_position;
	v->sxap_read = sxap_read;
	v->sxap_write = sxap_qwrite;
	v->sxap_flush = sxap_qflush;
	v->sxap_show = sxap_show;
	v->sxap_alter = sxap_alter;


	/*
	** Get the access lock in X mode.
	*/
        cl_status = LKrequest(LK_PHYSICAL, lk_ll, &lk_acc_key, LK_X, 
			&lk_val, &Sxap_cb->sxap_acc_lockid, 0, &cl_err);
        if ( (cl_status != OK) && (cl_status != LK_VAL_NOTVALID) )
        {
            TRdisplay( "SXAPC::Bad LKrequest %d\n", __LINE__);
            _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    sxap_lkerr_reason("ACCESS,X", cl_status);
	    *err_code = E_SX1035_BAD_LOCK_REQUEST;
            _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
                	NULL, NULL, 0L, NULL, &local_err, 0);
            break;
        }
        access_lock = TRUE;

	/* Convert access lock value to stamp */
	_VOID_ sxap_lockval_to_stamp(&lk_val, &stamp);

	/*
	** If we are running on a cluster we need to initialize the
	** Shared memory control lock key to contain the node id.
	*/
	if (CXcluster_enabled())
	{
	    _VOID_ LGshow(LG_A_NODEID, (PTR)&nodeid, 
			sizeof(nodeid), &l_nodeid, &cl_err);
	    lk_shm_key.lk_key3 = (i4)nodeid;
	}				
	/*
	** Now get the shared memory control lock in S mode
	*/
        cl_status = LKrequest(LK_PHYSICAL, lk_ll, &lk_shm_key,
                        LK_S, NULL , &Sxap_cb->sxap_shm_lockid, 0, &cl_err);
        if ( (cl_status != OK) && (cl_status != LK_VAL_NOTVALID) )
        {
            TRdisplay( "SXAPC::Bad LKrequest %d\n", __LINE__);
            _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    sxap_lkerr_reason("BUFFER,S", cl_status);
	    *err_code = E_SX1035_BAD_LOCK_REQUEST;
            _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
                	NULL, NULL, 0L, NULL, &local_err, 0);
            break;
        }
        shm_lock = TRUE;

	/*
	** Attempt to convert the lock to X mode. If it fails then some other
	** server already holds the lock and is connected to the shared
	** memory segment. If the convert succeeds then this is the first
	** server to start on this node.
	*/
        cl_status = LKrequest(LK_PHYSICAL | LK_CONVERT | LK_NOWAIT, lk_ll,
		&lk_shm_key, LK_X, NULL, &Sxap_cb->sxap_shm_lockid, 0, &cl_err);
        if (cl_status == LK_BUSY)
	{
	    new_server = FALSE;
	}
	else if ((cl_status == OK) || (cl_status == LK_VAL_NOTVALID) )
	{
 	    /*
 	    ** We are no longer allowed to pass a NULL address for lock value to
 	    ** LKrequest() when converting locks down, this causes problems
 	    ** on VMS and is now treated as a bad parameter. We don't need this
 	    ** value but we must declare a variable to obtain a real address
 	    ** to pass.
 	    */
 	    LK_VALUE	lkval;
 	    MEfill(sizeof(LK_VALUE),0,(PTR)&lkval);
	    /*
	    ** Got lock in X mode, we are the first server to start on
	    ** this node. Set the new_server flag, then convert the lock
	    ** back to S.
	    */
	    new_server = TRUE;
	    cl_status = LKrequest(LK_PHYSICAL | LK_CONVERT | LK_NOWAIT, 
			lk_ll, &lk_shm_key, LK_S, &lkval, 
			&Sxap_cb->sxap_shm_lockid, 0, &cl_err);
	    if (cl_status != OK)
	    {
		sxap_lkerr_reason("BUFFER,S", cl_status);
		*err_code = E_SX1019_BAD_LOCK_CONVERT;
		_VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
			    NULL, NULL, 0L, NULL, &local_err, 0);
		break;
	    }
	}
	else
	{
            TRdisplay( "SXAPC::Bad LKrequest %d\n", __LINE__);
            _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code = E_SX1019_BAD_LOCK_CONVERT;
            _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
                	NULL, NULL, 0L, NULL, &local_err, 0);
            break;
	}


	/* Initialize the audit file configuration data */
	status = sxap_init_cnf(&local_err);
	if (status != E_DB_OK)
	{
	    *err_code = local_err;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
	    	NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	/*
	** Create the audit shared memory segment. 
	**
	** If this is the first server to start on this node 
	** (i.e. new_server == TRUE) then the shared segment should
	** not exist yet. If it does the segment could contain invalid
	** information left round after a prevoius crash, we must destroy 
	** the segment and recreate it.
        **
	** If there are other servers running on this node 
	** (i.e. new_server == FALSE) then the shared segment must already
	** exist. In this case simply attach to the existing segment.
	*/
	num_shm_qbuf=Sxap_cb->sxap_num_shm_qbuf;
	shm_memsize= ((sizeof (SXAP_SHM_SEG)+(sizeof(SXAP_QBUF)*(num_shm_qbuf-1)))
			/ ME_MPAGESIZE) + 1;

	for (retry = 0; retry < 2; retry++)
	{
	    clflags = ME_CREATE_MASK | ME_MSHARED_MASK | ME_NOTPERM_MASK;
	    if (CXnuma_user_rad())
		clflags |= ME_LOCAL_RAD;
	    cl_status = MEget_pages(clflags, shm_memsize,
			SXAP_SHMNAME, (PTR *) &Sxap_cb->sxap_shm, 
			&alloc_pages, &cl_err);

	    if (cl_status == ME_ALREADY_EXISTS)
	    {
		if (new_server == TRUE)
		{
		    /*
		    ** We are the first server to start on this node and
		    ** have found an existing audit segment. This segment
		    ** has been left behind as the result of a crash. 
		    ** Remove the segment and retry.
		    */
			CS_cp_sem_cleanup(SXAP_SHMNAME, &cl_err);
		    cl_status = MEsmdestroy(SXAP_SHMNAME, &cl_err);
		    if (cl_status != OK)
		    {
			*err_code = E_SX101B_SXAP_SHM_DESTROY;
			_VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
				    NULL, NULL, 0L, NULL, &local_err, 0);
			break;
		    }
		    continue;
		}
		else
		{
		    cl_status = MEget_pages(
				ME_MSHARED_MASK | ME_NOTPERM_MASK,
				shm_memsize,
				SXAP_SHMNAME, (PTR *) &Sxap_cb->sxap_shm, 
				&alloc_pages, &cl_err);
		}
	    }
	    else if (cl_status == OK)
	    {
		/*
		** New memory, so initialize
		*/
		Sxap_cb->sxap_shm->shm_version=SXAP_SHM_VER;
		Sxap_cb->sxap_shm->shm_num_qbuf=num_shm_qbuf;
		Sxap_cb->sxap_shm->shm_qstart.trip=0;
		Sxap_cb->sxap_shm->shm_qend.trip=0;
		Sxap_cb->sxap_shm->shm_qwrite.trip=0;
		Sxap_cb->sxap_shm->shm_qstart.buf=0;
		Sxap_cb->sxap_shm->shm_qend.buf=0;
		Sxap_cb->sxap_shm->shm_qwrite.buf=0;
		Sxap_cb->sxap_shm->shm_page_size= Sxap_cb->sxap_page_size;

		for(i=0; i<num_shm_qbuf; i++)
		{
		    MEfill(sizeof(SXAP_QBUF), 0, 
				(PTR)&Sxap_cb->sxap_shm->shm_qbuf[i]);
		    Sxap_cb->sxap_shm->shm_qbuf[i].flags=SXAP_QEMPTY;
		}
	    }
	    break;
	}

	if (cl_status != OK)
	{
	    *err_code = E_SX1034_SXAP_SHM_CREATE;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
                	NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	/*
	** Check status of shared memory, must have same version, 
	** number of buffers
	*/
	if(Sxap_cb->sxap_shm->shm_version!=SXAP_SHM_VER)
	{
	    i4  tmp=SXAP_SHM_VER;
	    status=E_DB_ERROR;
	    *err_code = E_SX10BC_SXAP_SHM_BAD_VER;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
                	NULL, NULL, 0L, NULL, &local_err, 2,
			sizeof(Sxap_cb->sxap_shm->shm_version),
			(PTR)&Sxap_cb->sxap_shm->shm_version,
			sizeof(tmp),
			(PTR)&tmp);
	    break;
	}
	if(Sxap_cb->sxap_shm->shm_page_size!=Sxap_cb->sxap_page_size)
	{
	    i4  tmp=SXAP_SHM_VER;
	    status=E_DB_ERROR;
	    *err_code = E_SX10CB_SXAP_SHM_PG_SIZE;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
                	NULL, NULL, 0L, NULL, &local_err, 2,
			sizeof(Sxap_cb->sxap_shm->shm_page_size),
			(PTR)&Sxap_cb->sxap_shm->shm_page_size,
			sizeof(Sxap_cb->sxap_page_size),
			(PTR)&Sxap_cb->sxap_page_size);
	    break;
	}
	if(Sxap_cb->sxap_shm->shm_num_qbuf!=num_shm_qbuf)
	{
	    if(Sxf_svcb->sxf_svcb_status & SXF_SNGL_USER)
	    {
		/*
		** Single user servers follow current value if
		** not set.
		*/
		Sxap_cb->sxap_num_shm_qbuf=Sxap_cb->sxap_shm->shm_num_qbuf;
	    }
	    else
	    {
	        status=E_DB_ERROR;
	        *err_code = E_SX10BD_SXAP_SHM_BUF_COUNT;
	        _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
                	NULL, NULL, 0L, NULL, &local_err, 2,
			sizeof(Sxap_cb->sxap_shm->shm_num_qbuf),
			(PTR)&Sxap_cb->sxap_shm->shm_num_qbuf,
			sizeof(num_shm_qbuf),
			(PTR)&num_shm_qbuf
			);
	        break;
	    }
	}
	if (Sxap_cb->sxap_aud_writer_start>=Sxap_cb->sxap_num_shm_qbuf)
	{
		/*
		** Make sure audit writer startup is sensible
		*/
		Sxap_cb->sxap_aud_writer_start=
			Sxap_cb->sxap_num_shm_qbuf/2;
	}
	shm_created = TRUE;

	/*
	**	Setup the "current" log file, with special handling
	**	for startup.
	**
	**	If connected to extant shared memory (!new_server)
	**	set the current file to that matching the
	**	shared memory stamp.
	*/
	status = sxap_find_curfile(SXAP_FINDCUR_INIT, 
			Sxap_cb->sxap_curr_file, 
			(new_server) 
			    ? (SXAP_STAMP*)NULL
			    : &Sxap_cb->sxap_shm->shm_stamp,
			NULL,
			err_code);
	if (status!=E_DB_OK)
	{
		/*
		**	See how to handle no log files on startup
		*/
		if (Sxap_cb->sxap_act_on_full==SXAP_STOPAUDIT)
		{
			/*
			** Make sure we log that auditing is stopped
			*/
			_VOID_ ule_format(E_SX10A6_STOPAUDIT_LOGFULL, NULL,
				ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
			/* This is a signal for caller what has happend */
			status=E_DB_WARN;
	    		*err_code=E_SX102D_SXAP_STOP_AUDIT ;
		}
		else
		{
			/*
			** If shutdown or suspend, return a fatal status
			** (suspending on startup doesn't make much sense)
			*/
			status=E_DB_FATAL;
			Sxap_cb->sxap_status |= SXAP_SHUTDOWN;
		}
	}

	/*
	**	Log the new current file if multi-user.
	*/
	if(!(Sxf_svcb->sxf_svcb_status & SXF_SNGL_USER))
	{
	    _VOID_ ule_format(I_SX1056_CHANGE_AUDIT_LOG, NULL,
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 2,
		STlength(Sxap_cb->sxap_curr_file),
		Sxap_cb->sxap_curr_file);
	}
	if (status==E_DB_WARN)
	{
	    /*
	    ** If auditing has been disabled because that are no
	    ** free audit files left we must convert the access lock
	    ** to NULL mode and return successfully to the caller.
	    */
	    if (*err_code==E_SX102D_SXAP_STOP_AUDIT)
	    {
		cl_status = LKrequest(LK_PHYSICAL | LK_CONVERT, lk_ll, 
			&lk_acc_key, LK_N, &lk_val, &Sxap_cb->sxap_acc_lockid, 
			0, &cl_err);
		if ( (cl_status != OK) && (cl_status != LK_VAL_NOTVALID) )
		{
                    TRdisplay( "SXAPC::Bad LKrequest %d\n", __LINE__);
                    _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	            sxap_lkerr_reason("ACCESS, sxap_startup", cl_status);
		    *err_code = E_SX1019_BAD_LOCK_CONVERT;
		    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
				NULL, NULL, 0L, NULL, &local_err, 0);
		    break;
		}
		status = E_DB_OK;
		*err_code = E_SX0000_OK;
		audit_disabled=TRUE;
	    }
	    else
	    {
		_VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
		status = E_DB_ERROR;
	    }
	    break;
	}
	else if (status != E_DB_OK)
	{
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
	    	NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	pagebuf = (SXAP_DPG *) &Sxap_cb->sxap_shm->shm_apb.apb_auditpage;

	/*
	** If we are the first server to start on this node, 
	** attempt to create an audit file. If the file already 
	** exists, don't wory about it - its not an error.
	*/
	if (new_server)
	{
	    status = sxap_create((PTR) &Sxap_cb->sxap_curr_file, &local_err);
	    if (status != E_DB_OK)
	    {
		if (local_err == E_SX0016_FILE_EXISTS)
		    new_file = FALSE;
		else
		{
		    *err_code = local_err;
	    	    _VOID_ ule_format(*err_code, NULL, ULE_LOG,
				NULL, NULL, 0L, NULL, &local_err, 0);
		    break;
		}
	    }
	}

	/* 
	** Open the audit file for writing.
	*/
	status = sxap_open((PTR) &Sxap_cb->sxap_curr_file, SXF_WRITE, 
			rscb, NULL, (PTR)&Sxap_cb->sxap_stamp, &local_err);
	if (status != E_DB_OK)
	{
	    *err_code = local_err;
	     _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	file_open = TRUE;

	rs = (SXAP_RSCB *) rscb->sxf_sxap;
	Sxap_cb->sxap_curr_rscb = rs;

	/*
	** If there were already servers running on this node the
	** shared audit buffer should contain valid audit data. We
	** will look at the buffered page, and validate it against the
	** corresponding page in the audit file. This is done by comparing
	** the checksum field of both the buffered page and the real page.
	** This should be identical because the checksum field in the
	** buffer will only get calculated when the buffered page gets
	** written to the file.
	**
	** But only check the checksums if the current file's stamp
	** matches the shared memory stamp. In a cluster, anything
	** can happen and the current file may have moved on before this
	** node has a chance to notice.
	**
	** On the other hand, if this is the first server to start on this
	** node we will need to initialize the audit buffer with page data
	** from the audit file. If we've just created the file we know that
	** its not been accessed by any other servers on other nodes. In 
	** this case we can use the first page in the audit file (Page 1).
	** If we are not running in a cluster environment we know that 
	** we're the only server in the installation with the audit file open
	** in this case we find the last page in the audit file, and initialize
	** the audit buffer using that page. If we are running in a cluster
	** we must allocate a new page from the audit file and use that
	** to initialize the buffer with.
	*/
	if (new_server == FALSE)
	{
	    SXAP_DPG		*local_page;

	    /* Only check if shm stamp matches "current" file stamp */
	    if ( !sxap_stamp_cmp(&Sxap_cb->sxap_shm->shm_stamp, &Sxap_cb->sxap_stamp) )
	    {
		ulm_rcb.ulm_blocksize = 0;
		ulm_rcb.ulm_poolid = Sxf_svcb->sxf_pool_id;
		ulm_rcb.ulm_streamid_p = &ulm_rcb.ulm_streamid;
		
		/* Open a private stream */
		/* Open stream and allocate memory with one call */
		ulm_rcb.ulm_flags = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;
		ulm_rcb.ulm_psize = SXAP_MAX_PGSIZE; 

		status = ulm_openstream(&ulm_rcb);
		if (status != E_DB_OK)
		{
		    if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
			*err_code = E_SX1002_NO_MEMORY;
		    else
			*err_code = E_SX106B_MEMORY_ERROR;

		    _VOID_ ule_format(ulm_rcb.ulm_error.err_code, NULL, ULE_LOG, 
			    NULL, NULL, 0L, NULL, &local_err, 0);
		    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			    NULL, NULL, 0L, NULL, &local_err, 0);
		    break;
		}
		stream_open = TRUE;

		local_page = (SXAP_DPG *) ulm_rcb.ulm_pptr;

		status = sxap_read_page(&rs->rs_di_io, 
			    (i4) pagebuf->dpg_header.pg_no, 
			    (PTR) local_page, 
			    Sxap_cb->sxap_page_size, 
			    &local_err);
		if (status != E_DB_OK)
		{
		    *err_code = local_err;
		    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
				    NULL, NULL, 0L, NULL, &local_err, 0);
		    break;
		}

		if (local_page->dpg_header.pg_chksum != 
		    pagebuf->dpg_header.pg_chksum)
		{
		    *err_code = E_SX1017_SXAP_SHM_INVALID;
		    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			       NULL, NULL, 0L, NULL, &local_err, 0);
		    break;
		}

		status = ulm_closestream (&ulm_rcb);
		if (status != E_DB_OK)
		{
		    *err_code = E_SX106B_MEMORY_ERROR;
		    _VOID_ ule_format(ulm_rcb.ulm_error.err_code, NULL, ULE_LOG, 
			       NULL, NULL, 0L, NULL, &local_err, 0);
		    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			       NULL, NULL, 0L, NULL, &local_err, 0);
		    break;
		}
		stream_open = FALSE;
	    }
	}
	else if (new_file)
	{
	    /* First server on node, and created a new file */
	    status = sxap_read_page(&rs->rs_di_io, 1L, 
			(PTR) pagebuf, 
			Sxap_cb->sxap_page_size,
			&local_err);
	    if (status != E_DB_OK)
	    {
		*err_code = local_err;
	    	_VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			    NULL, NULL, 0L, NULL, &local_err, 0);
		break;
	    }

	    Sxap_cb->sxap_shm->shm_apb.apb_last_rec.pageno = 1;
	    Sxap_cb->sxap_shm->shm_apb.apb_last_rec.offset = 0;
	    Sxap_cb->sxap_shm->shm_stamp = Sxap_cb->sxap_stamp;
	}
	else if (CXcluster_enabled() == 0)
	{
	    /*
	    **	Not on a cluster
	    */
	    status = sxap_read_page(&rs->rs_di_io, rs->rs_last_page, 
			(PTR) pagebuf, Sxap_cb->sxap_page_size,
			&local_err);
	    if (status != E_DB_OK)
	    {
		*err_code = local_err;
	    	_VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			    NULL, NULL, 0L, NULL, &local_err, 0);
		break;
	    }

	    Sxap_cb->sxap_shm->shm_apb.apb_last_rec.pageno = pageno;
	    Sxap_cb->sxap_shm->shm_apb.apb_last_rec.offset = 
		pagebuf->dpg_header.pg_next_byte;
	    Sxap_cb->sxap_shm->shm_stamp = Sxap_cb->sxap_stamp;
        }
	else
	{
	    /*
	    ** Cluster 
	    */
	    status = sxap_alloc_page(&rs->rs_di_io, 
			(PTR) pagebuf, &pageno, rs->rs_pgsize, &local_err);
	    if (status != E_DB_OK)
	    {
		*err_code = local_err;
	        _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			    NULL, NULL, 0L, NULL, &local_err, 0);
		break;
	    }

	    Sxap_cb->sxap_shm->shm_apb.apb_last_rec.pageno = pageno;
	    Sxap_cb->sxap_shm->shm_apb.apb_last_rec.offset = 0; 
	    Sxap_cb->sxap_shm->shm_stamp = Sxap_cb->sxap_stamp;
	}

	Sxap_cb->sxap_status |= SXAP_ACTIVE;
	/* 
	** Convert the access lock to null mode and put the new
	** stamp into the lock.  
	*/
	status = sxap_stamp_to_lockval(&Sxap_cb->sxap_stamp, &lk_val);
        cl_status = LKrequest(LK_PHYSICAL | LK_CONVERT, lk_ll, &lk_acc_key, 
			LK_N, &lk_val, &Sxap_cb->sxap_acc_lockid, 0, &cl_err);
	if ( (cl_status != OK) && (cl_status != LK_VAL_NOTVALID) )
	{
            TRdisplay( "SXAPC::Bad LKrequest %d\n", __LINE__);
            _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    sxap_lkerr_reason("ACCESS,X", cl_status);
	    *err_code = E_SX1019_BAD_LOCK_CONVERT;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
                	NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
        access_lock = FALSE;
	break;
    }

    /* Cleanup after any errors */
    if (*err_code != E_SX0000_OK)
    {
	if (file_open)
	    _VOID_ sxap_close(rscb, &local_err);

	if (stream_open)
	    _VOID_ ulm_closestream(&ulm_rcb);


	if (shm_created)
	{
	    _VOID_ MEfree_pages((PTR) Sxap_cb->sxap_shm,
		((sizeof (SXAP_SHM_SEG)+(sizeof(SXAP_QBUF)*(Sxap_cb->sxap_num_shm_qbuf-1)))
			/ ME_MPAGESIZE) + 1,
		    &cl_err);
	    Sxap_cb->sxap_shm = NULL;

	    if (new_server)
	    {
		CS_cp_sem_cleanup(SXAP_SHMNAME, &cl_err);
		_VOID_  MEsmdestroy(SXAP_SHMNAME, &cl_err);
	    }
	}

	if (shm_lock)
	{
	    _VOID_ LKrelease(0, lk_ll, &Sxap_cb->sxap_shm_lockid, 
			NULL, NULL, &cl_err);
	}
	
	if (access_lock)
	{
	    _VOID_ LKrelease(0, lk_ll, &Sxap_cb->sxap_acc_lockid, 
			NULL, &lk_val, &cl_err);
	}

	*err_code = E_SX101A_SXAP_STARTUP;

	/*
	** Errors during startup of the auditing system are 
	** considered to be severe errors.
	*/
	status = E_DB_SEVERE;
    }

    if(status==E_DB_OK && audit_disabled)
    {
	   /* Auditing not active */
	   Sxap_cb->sxap_status &= ~SXAP_ACTIVE;
           *err_code=E_SX102D_SXAP_STOP_AUDIT;
	   status=E_DB_WARN;
    }
    return (status);
}

/*
** Name: SXAP_SHUTDOWN - shutdown the low level auditing system
**
** Description:
**	This routine is used to shutdown the low level auditing system. It
**	should be called only once at facility shutdown time. The function
**	of the routine is to free any resourses allocated in the low level
**	auditing system.
**
**	The shared memory segment used to buffer audit writes needs to be
**	freed at this time. In order to determine if the memory should be
**	destroyed or unmapped this function accepts to escalate the control
**	lock granted in sxap_init to exclusive mode. If it is successful
**	then no other processes are using the segment and it should be
**	destroyed. If the lock request is unsuccessful the segment should
**	only be unmapped from the process.
**
** 	Algorithm:
**
**	Attempt to escalate shared memory control lock to exclusive mode
**	If (successful)
**		Destroy shared memory segment
**	else
**		Unmap shared memory segment from process space
**
** Inputs:
**	None.
**
** Outputs:
**	err_code		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**      20-aug-1992 (markg)
**	    Initial creation.
**	03-dec-1992 (markg)
**	    Modified to take the audit ACCESS lock in X mode while manipulating
**	    the shared memory segment at at shutdown time. This could have
**	    possibly caused timing related problems.
**      10-mar-1993 (markg)
**          Fixed problem where errors returned from ulm were not getting
**          handled correctly. Also cleaned up lock handling code for
**	    clusters.
**	10-may-1993 (markg)
**	    Commented out the call to MEfree_pages, because it failed
**	    continually. I think that this is due to a bug in the UNIX CL.
**	    As the server is shutting down this should not cause a problem.
**	18-feb-1997 (canor01)
**	    Restored the call to MEfree_pages because it is required for
**	    the subsequent MEsmdestroy to work on NT.
**	24-jul-1997 (nanpr01)
**	    Added multi-thread flag to lock request and release calls.
**	10-aug-1997 (canor01)
**	    Remove the semaphore before freeing the memory.
**	28-aug-1997 (nanpr01)
**	    Back out change 431377 which added multi-thread flag 
**	    to lock request and release calls.
*/
DB_STATUS
sxap_shutdown( 
    i4 	*err_code)
{
    DB_STATUS	    status = E_DB_OK;
    STATUS	    cl_status;
    i4	    local_err;
    CL_ERR_DESC	    cl_err;
    LK_VALUE	    lk_val;
    LK_LOCK_KEY	    lk_acc_key = {LK_AUDIT, SXAP_LOCK, SXAP_ACCESS, 0, 0, 0, 0};
    LK_LOCK_KEY	    lk_shm_key = {LK_AUDIT, SXAP_LOCK, SXAP_SHMCTL, 0, 0, 0, 0};
    LK_LLID	    lk_ll = Sxf_svcb->sxf_lock_list;
    i4	    nodeid, l_nodeid;

    *err_code = E_SX0000_OK;

    /* Request the ACCESS lock in X mode */
    cl_status = LKrequest(LK_PHYSICAL | LK_CONVERT, lk_ll, 
		    &lk_acc_key, LK_X, &lk_val,
		    &Sxap_cb->sxap_acc_lockid, 0, &cl_err);
    if ( (cl_status != OK) && (cl_status != LK_VAL_NOTVALID) )
    {
        TRdisplay( "SXAPC::Bad LKrequest %d\n", __LINE__);
        _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	*err_code = E_SX1035_BAD_LOCK_REQUEST;
        _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
                	NULL, NULL, 0L, NULL, &local_err, 0);
    }

    cl_status = MEfree_pages((PTR) Sxap_cb->sxap_shm,
                    ((sizeof (SXAP_SHM_SEG)+
                     (sizeof(SXAP_QBUF)*(Sxap_cb->sxap_num_shm_qbuf-1)))
			/ ME_MPAGESIZE) + 1,
		    &cl_err);
    if (cl_status != OK)
    {
        *err_code = E_SX1016_SXAP_MEM_FREE;
        _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
    	    NULL, NULL, 0L, NULL, &local_err, 0);
    }

    Sxap_cb->sxap_shm = NULL;

    /*
    ** If we are running on a cluster we need to initialize the
    ** Shared memory control lock key to contain the node id.
    */
    if (CXcluster_enabled())
    {
	_VOID_ LGshow(LG_A_NODEID, (PTR)&nodeid, 
		sizeof(nodeid), &l_nodeid, &cl_err);
	lk_shm_key.lk_key3 = (i4)nodeid;
    }

    /* Now attempt to escalate the SHMCTL lock to X mode */
    cl_status = LKrequest(LK_PHYSICAL | LK_CONVERT | LK_NOWAIT, 
		    lk_ll, &lk_shm_key, LK_X, NULL, 
		    &Sxap_cb->sxap_shm_lockid, 0, &cl_err);
    if (cl_status == LK_BUSY) 
	/* 
	** If we were not granted the lock it means that other
	** servers were attached to the shared segment. In this case
	** we do nothing.
	*/
	;
    else if ( (cl_status == OK) || (cl_status == LK_VAL_NOTVALID) )
    {
	/*
	** We are the only server using the shared segment. Destroy
	** the segment.
	*/
	CS_cp_sem_cleanup(SXAP_SHMNAME, &cl_err);
	cl_status = MEsmdestroy(SXAP_SHMNAME, &cl_err);
	if (cl_status != OK)
	{
	    *err_code = E_SX101B_SXAP_SHM_DESTROY;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	}
    }
    else
    {
        TRdisplay( "SXAPC::Bad LKrequest %d\n", __LINE__);
        _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	*err_code = E_SX1035_BAD_LOCK_REQUEST;
	_VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
                NULL, NULL, 0L, NULL, &local_err, 0);
    }

    /* Release the SHMCTL lock */
    cl_status = LKrelease(0, lk_ll, &Sxap_cb->sxap_shm_lockid, 
			NULL, NULL, &cl_err);
    if (cl_status != OK)
    {
	sxap_lkerr_reason("SHMCTL, sxap_shutdown", cl_status);
	*err_code = E_SX1008_BAD_LOCK_RELEASE;
	_VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
		NULL, NULL, 0L, NULL, &local_err, 0);
    }

    Sxap_cb->sxap_status &= ~SXAP_ACTIVE;

    /* Release the ACCESS lock */
    cl_status = LKrelease(0, lk_ll, &Sxap_cb->sxap_acc_lockid, 
			NULL, &lk_val, &cl_err);
    if (cl_status != OK)
    {
	sxap_lkerr_reason("ACCESS, sxap_shutdown", cl_status);
	*err_code = E_SX1008_BAD_LOCK_RELEASE;
	_VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
		NULL, NULL, 0L, NULL, &local_err, 0);
    }


    if (*err_code != E_SX0000_OK)
    {
	*err_code = E_SX101C_SXAP_SHUTDOWN;
	status = E_DB_ERROR;
    }

    CSr_semaphore(&Sxap_cb->sxap_semaphore);

    return (status);
}

/*
** Name: SXAP_BGN_SES - begin a session
**
** Description:
**	This routine should be called at session startup time to initialize
**	any audit specific data structures associated with the session. It
**	should be called only once for each session.
**
**	The only session specific data structures that are needed in the
**	default auditing system is a record identifier that shows the
**	record id of the last audit record written. This structure is
**	allocated from the session'm memory stream and is attached to the 
**	SCB
**
** 	Overview of functions performed in this routine:-
**
**	Allocate SXAP_REC_ID structure from sessions memory stream
**	Attach to SCB
**
** Inputs:
**	scb			SXF session control block.
**
** Outputs:
**	err_code		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**      20-aug-1992 (markg)
**	    Initial creation.
**      10-mar-1993 (markg)
**          Fixed problem where errors returned from ulm were not getting
**          handled correctly.
**	2-apr-94 (robf)
**	    Allocate a true SCB rather than a  last_rec value for SXAP
**	    now, although it still uses the same pointer.
*/
DB_STATUS
sxap_bgn_ses(
    SXF_SCB	*scb,
    i4	*err_code)
{
    DB_STATUS	status = E_DB_OK;
    i4	local_err;
    ULM_RCB	ulm_rcb;
    SXAP_SCB	*sxap_scb;

    *err_code = E_SX0000_OK;

    ulm_rcb.ulm_facility = DB_SXF_ID;
    ulm_rcb.ulm_streamid_p = &scb->sxf_scb_mem;
    ulm_rcb.ulm_psize = sizeof (SXAP_SCB);
    ulm_rcb.ulm_memleft = &Sxf_svcb->sxf_pool_sz;
    status = ulm_palloc(&ulm_rcb);
    if (status != E_DB_OK)
    {
	if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
	    *err_code = E_SX1002_NO_MEMORY;
	else
	    *err_code = E_SX106B_MEMORY_ERROR;

	_VOID_ ule_format(ulm_rcb.ulm_error.err_code, NULL,
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
    }
    else
    {
	scb->sxf_sxap_scb = ulm_rcb.ulm_pptr;
	sxap_scb=(SXAP_SCB*)ulm_rcb.ulm_pptr;
	sxap_scb->flags=0;
	sxap_scb->sess_lastq.trip=0;
	sxap_scb->sess_lastq.buf=0;
    }

	
    /* Handle any errors */
    if (*err_code != E_SX0000_OK)
    {
	_VOID_ ule_format(*err_code, NULL,
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	*err_code = E_SX101D_SXAP_BGN_SESSION;
	status = E_DB_ERROR;
    }

    return (status);
}      

/*
** Name: SXAP_END_SES - end a session
**
** Description:
**	This routine should be called at session end time to release
**	any audit specific data structures associated with the session. It
**	should be called only once for each session.
**
**	In the SXAP low level auditing system, there is no work to be 
**	performed by this routine. This routine is just a stub.
**
** Inputs:
**	scb			SXF session control block.
**
** Outputs:
**	err_code		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**      20-aug-1992 (markg)
**	    Initial creation.
**	27-apr-94 (robf)
**	    Flush to disk if not-force is turned on.
*/
DB_STATUS
sxap_end_ses(
    SXF_SCB	*scb,
    i4	*err_code)
{
    DB_STATUS status=E_DB_OK;
    i4   local_err;

    *err_code = E_SX0000_OK;
    /*
    ** Make sure data is flushed at end of session in case
    ** not flushed previously.
    */
    if(Sxap_cb->sxap_force_flush==SXAP_FF_OFF &&
       Sxap_cb->sxap_curr_rscb!= NULL)
    {
    	status = sxap_flushbuff(err_code);
    	if (status != E_DB_OK)
    	{
		if (*err_code > E_SXF_INTERNAL)
		    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
		    NULL, NULL, 0L, NULL, &local_err, 0);
	}
    }
    return(E_DB_OK);
}

/*
** Name: SXAP_INIT_CNF - initialize the SXAP configuration
**
** Description:
**	This routine initializes the SXAP configuration from the PM
**	configuration file.
**
** Inputs:
**	None.
**
** Outputs:
**	err_code			Error code returned to caller.
**
** Returns:
**	E_DB_OK		- Initialization succeeded
**
**	E_DB_WARN	- Succeeded, but auditing is disabled
**
**	E_DB_ERROR	- something went wrong
**
** History:
**      20-aug-1992 (markg)
**	    Initial creation.
**	04-dec-1992 (robf)
**	    Initialize options fully. Now finds/sets the "current" file..
**      10-mar-1993 (markg)
**          Fixed problem where errors returned from ulm were not getting
**          handled correctly. 
**	27-jul-93 (robf)
**	    Write E_SX10A6_STOPAUDIT_LOGFULL to the error log if no
**	    audit files are available  at startup so we know security 
**	    auditing is disabled.
**	13-aug-93 (stephenb)
**	    Integrate following change made by robf in es2dev branch:
**          Write E_SX10A6_STOPAUDIT_LOGFULL to the error log if no
**          audit files are available  at startup so we know security
**          auditing is disabled.
**	3-aug-93 (robf)
**	    Signal all log files used with E_DB_WARN and an explicit
**	    error code, rather than indirectly. This is now consistent
**	    with the rest of the code.
**	8-oct-93 (robf)
**	    Only log the 'I_SX..Audit log set' message for multi-user
**	    servers.
**	19-oct-93 (stephenb)
**	    Change PM parameters in calls to PMget() to be compatible with CBF.
**	23-dec-93 (robf)
**          Don't mark auditing disabled  here, this should be done in 
**	    the logical layer.
**	30-mar-94 (robf)
**          Look up number of shared memory buffers,
**	    Also update comments to match actual PM values.
**	    Take out initialization, now done elsewhere.
**	13-apr-94 (robf)
**          Use ERoptlog to write options to startup log.
**	11-may-94 (robf)
**          Only write startup options if not single user server.
**	27-may-94 (robf)
**          The c2.security_audit startup option is already written to the
**	    log by SCF processing, so don't write it here, so avoiding a duplicate
**	    message.
*/
DB_STATUS
sxap_init_cnf(
	i4		*err_code)
{
    DB_STATUS		status = E_DB_OK;
    STATUS		clstatus;
    i4		local_err;
    ULM_RCB		ulm_rcb;
    char		*fptr;
    char		*pmvalue;
    char		*pmfile;
    i4			i;

    *err_code = E_SX0000_OK;

    for (;;)
    {
	/*
	**	Look up info PM info.
	**	This includes
	**	II.*.c2.security_auditing	
	**	II.*.c2.logfile.N
	**	II.*.c2.max_log_size
	**	II.*.c2.on_log_full
	**	II.*.c2.on_switch_log
	**	II.*.c2.on_error
	**	II.*.c2.shared_buffer_count
	*/

# ifdef xDEBUG
	/*
	**	Allow private override if necessary
	*/
	NMgtAt("II_SXF_PMFILE",&pmfile);
	if(pmfile && *pmfile)
	{
		LOCATION pmloc;
		TRdisplay("Loading SXF-PM file '%s'\n",pmfile);
	    	LOfroms(PATH & FILENAME, pmfile, &pmloc);
		if(PMload(&pmloc,NULL)!=OK)
			TRdisplay("Error loading PMfile '%s'\n",pmfile);
	}
# endif

	/*
	**	Auditing status
	*/
	Sxap_cb->sxap_status=0L;

	if (PMget(SX_C2_MODE,&pmvalue) == OK)
	{
    		if (!STbcompare(pmvalue, 0, SX_C2_MODE_ON, 0, TRUE)) 
		{
			/*
			**	Auditing is on
			*/
			Sxap_cb->sxap_status=SXAP_ACTIVE;
		}
    		else if (STbcompare(pmvalue, 0, SX_C2_MODE_OFF, 0, TRUE)!=0) 
		{
			/*
			**	Invalid mode specified, must be "on" or  "off"
			*/
			*err_code=E_SX1061_SXAP_BAD_MODE;
			break;
		}
	}
	else
	{
		/*
		**	No value found, error.
		*/
		*err_code=E_SX1061_SXAP_BAD_MODE;
		break;
	}
	/*
	**	Maximum size of audit files
	*/
	if (PMget("II.*.C2.MAX_LOG_SIZE",&pmvalue) == OK)
	{
	        if(!(Sxf_svcb->sxf_svcb_status & SXF_SNGL_USER))
			ERoptlog("max_log_size", pmvalue);
		clstatus= CVal(pmvalue, &Sxap_cb->sxap_max_file );

		/*
		**	Check if valid (Anything less than 100K could cause
		**	thrashing)
		*/
		if(Sxap_cb->sxap_max_file< SXAP_MIN_MAXFILE || clstatus!=OK)
		{
			*err_code=E_SX104A_SXAP_BAD_MAXFILE;
			break;
		}
	}
	else
	{
		/*
		**	No value found, set a default
		*/
		Sxap_cb->sxap_max_file= SXAP_DEF_MAXFILE;
		TRdisplay("SXAP: Max file size not found, setting default to %d\n",
				SXAP_DEF_MAXFILE);
	}
	/*
	**	ONLOGFULL - can STOPAUDIT, SUSPEND or SHUTDOWN
	*/
	if (PMget("II.*.C2.ON_LOG_FULL",&pmvalue) == OK)
	{
	        if(!(Sxf_svcb->sxf_svcb_status & SXF_SNGL_USER))
			ERoptlog("on_log_full", pmvalue);
    		if (!STcasecmp(pmvalue, "STOPAUDIT" )) 
		{
			Sxap_cb->sxap_act_on_full=SXAP_STOPAUDIT;
		}
    		else if (!STcasecmp(pmvalue, "SHUTDOWN" )) 
		{
			Sxap_cb->sxap_act_on_full=SXAP_SHUTDOWN;
		}
    		else if (!STcasecmp(pmvalue, "SUSPEND" )) 
		{
			Sxap_cb->sxap_act_on_full=SXAP_SUSPEND;
		}
		else
		{
			/*
			**	Invalid behaviour specified.
			*/
			*err_code=E_SX104D_SXAP_BAD_ONLOGFULL;
			break;
		}
	}
	else
	{
		/*
		**	No value found, error 
		*/
		*err_code=E_SX104D_SXAP_BAD_ONLOGFULL;
		break;
	}
	/*
	**	ONERROR - can STOPAUDIT or SHUTDOWN
	*/
	if (PMget("II.*.C2.ON_ERROR",&pmvalue) == OK)
	{
	        if(!(Sxf_svcb->sxf_svcb_status & SXF_SNGL_USER))
			ERoptlog("on_error", pmvalue);
    		if (!STcasecmp(pmvalue, "STOPAUDIT" )) 
		{
			Sxf_svcb->sxf_act_on_err=SXF_ERR_STOPAUDIT;
		}
    		else if (!STcasecmp(pmvalue, "SHUTDOWN" )) 
		{
			Sxf_svcb->sxf_act_on_err=SXF_ERR_SHUTDOWN;
		}
		else
		{
			/*
			**	Invalid behaviour specified.
			*/
			*err_code=E_SX1060_SXAP_BAD_ONERROR;
			break;
		}
	}
	else
	{
		/*
		**	No value found, error 
		*/
		*err_code=E_SX1060_SXAP_BAD_ONERROR;
		break;
	}
	/*
	**	Number of shared memory buffers
	*/
	if (PMget("II.*.C2.SHARED_BUFFER_COUNT",&pmvalue) == OK)
	{
	        if(!(Sxf_svcb->sxf_svcb_status & SXF_SNGL_USER))
			ERoptlog("shared_buffer_count", pmvalue);
		clstatus= CVal(pmvalue, &Sxap_cb->sxap_num_shm_qbuf );
		if(clstatus!=OK || Sxap_cb->sxap_num_shm_qbuf<0)
		{
			Sxap_cb->sxap_num_shm_qbuf=SXAP_SHM_DEF_BUFFERS;
			TRdisplay("SXAP: Number shared queue buffers invalid, setting default to %d\n",
				SXAP_SHM_DEF_BUFFERS);
		}

	}
	else 
	{
		/*
		**	No value found, set a default
		*/
		Sxap_cb->sxap_num_shm_qbuf= SXAP_SHM_DEF_BUFFERS;
	}
	/*
	**	Audit writer startup
	*/
	if (PMget("II.*.C2.AUDIT_WRITER_START",&pmvalue) == OK)
	{
	        if(!(Sxf_svcb->sxf_svcb_status & SXF_SNGL_USER))
			ERoptlog("audit_writer_start", pmvalue);
		clstatus= CVal(pmvalue, &Sxap_cb->sxap_aud_writer_start );
		if(clstatus!=OK || Sxap_cb->sxap_aud_writer_start<0)
		{
			Sxap_cb->sxap_num_shm_qbuf=SXAP_DEF_AUD_WRITER_START;
			TRdisplay("SXAP: Audit  writer start invalid, setting default to %d\n",
				SXAP_DEF_AUD_WRITER_START);
		}

	}
	else 
	{
		/*
		**	No value found, set a default
		*/
		Sxap_cb->sxap_aud_writer_start= SXAP_DEF_AUD_WRITER_START;
	}
	/*
	**	ONSWITCHLOG - optional program to run when changing
	**	logfiles.
	*/
	if (PMget("II.*.C2.ON_SWITCH_LOG",&pmvalue) == OK)
	{
	        if(!(Sxf_svcb->sxf_svcb_status & SXF_SNGL_USER))
			ERoptlog("on_switch_log", pmvalue);
		STcopy(pmvalue,Sxap_cb->sxap_switch_prog);
	}
	else
	{
		/*
		**	No value found, set default
		*/
		STcopy(SXAP_DEF_SWITCHPROG,Sxap_cb->sxap_switch_prog);
	}
	if (STlength(Sxap_cb->sxap_switch_prog)>0)
		Sxap_cb->sxap_status|=SXAP_SWITCHPROG;
	/*
	**	LOG_PAGE_SIZE - Number of KB per page, values
	**	are 2, 4, 8, 16, 32
	*/
	if (PMget("II.*.C2.LOG_PAGE_SIZE",&pmvalue) == OK)
	{
	        if(!(Sxf_svcb->sxf_svcb_status & SXF_SNGL_USER))
			ERoptlog("log_page_size", pmvalue);
		clstatus= CVal(pmvalue, &Sxap_cb->sxap_page_size );
		if(clstatus!=OK || Sxap_cb->sxap_page_size<2)
		{
			Sxap_cb->sxap_page_size=SXAP_DEF_PGSIZE;
		}
		else
		{	
			/*
			** Convert KB to bytes.
			*/
			Sxap_cb->sxap_page_size*=1024;
		}
	}
	else
	{
		/*
		**	No value found, set default
		*/
		Sxap_cb->sxap_page_size=SXAP_DEF_PGSIZE;
	}
	/*
	**	FLUSH_FORCE - Boolean to flush or not.
	*/
	if (PMget("II.*.C2.FLUSH_FORCE",&pmvalue) == OK)
	{
	        if(!(Sxf_svcb->sxf_svcb_status & SXF_SNGL_USER))
			ERoptlog("flush_force", pmvalue);
		if(!STcasecmp(pmvalue, "OFF"))
			Sxap_cb->sxap_force_flush=SXAP_FF_OFF;
		else
			Sxap_cb->sxap_force_flush=SXAP_FF_ON;
	}
	else
	{
		/*
		**	No value found, set default
		*/
		Sxap_cb->sxap_force_flush=SXAP_FF_ON;
	}
	/*
	**	End  of processing loop
	*/
	break;
    }
    /* Handle any errors */
    if (*err_code != E_SX0000_OK && *err_code!=E_SX102D_SXAP_STOP_AUDIT)
    {
	_VOID_ ule_format(*err_code, NULL,
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	*err_code = E_SX1020_SXAP_INIT_CNF;
	status = E_DB_ERROR;
    }

    return (status);
}      

/*
** Name: SXAP_SHOW - return data about the SXAP configuration
**
** Description:
**	This routine is used to return data about the configuration of the
**	SXAP configuration. Currently he only information returned is
**	The name of the current audit file and the maximum file size.
**
** Inputs:
**	filename		pointer to buffer to hold nameof audit file.
**
** Outputs:
**	filename		name of the current audit file.
**	max_file		the maximum size of an audit file.
**	er_code			error code for the call.
**
** Returns:
**	DB_STATUS
**
** History:
**      20-aug-1992 (markg)
**	    Initial creation.
**	26-oct-93 (robf)
**          Only return the current file name if there is one.
**	12-jan-94 (robf)
**          Get sxap semaphore to make sure we get good information from
**	    the CB
**	5-apr-94 (stephenb)
**	    Integrate (robf) semaphore changes:
**		Get sxap semaphore to make sure we get good information from
**		the CB
*/
DB_STATUS
sxap_show(
    PTR		filename,
    i4	*max_file,
    i4	*err_code)
{
    DB_STATUS		status = E_DB_OK;
    i4		local_err;
    char		*file;
    LOCATION		loc;
    char		dev[LO_NM_LEN];
    char		fprefix[LO_NM_LEN];
    char		fsuffix[LO_NM_LEN];
    char		version[LO_NM_LEN];
    char		path[MAX_LOC + 1];
    char		filebuf[MAX_LOC + 1];
    bool                got_semaphore=FALSE;


    *err_code = E_SX0000_OK;

    for (;;)
    {
	/*
	** Get SXAP semaphore so data doesn't change under us
	*/
        if ( status = sxap_sem_lock(err_code) )
            break;
	got_semaphore=TRUE;

	if(!(Sxap_cb->sxap_status & SXAP_ACTIVE))
	{
	    /*
	    ** Auditing not active, so return empty name
	    */
	    if(filename)
		filename[0]='\0';
	}
	else if (filename)
	{
	    *err_code = E_SX101E_INVALID_FILENAME;
	    STcopy((char *)Sxap_cb->sxap_curr_file, filebuf);
	    if (LOfroms(PATH & FILENAME, filebuf, &loc) != OK)
		break;
	    if (LOdetail(&loc, dev, path, fprefix, fsuffix, version) != OK)
		break;
	    if (LOcompose(NULL, NULL, fprefix, fsuffix, version, &loc) != OK)
		break;
	    LOtos(&loc, &file);
	    STcopy(file, (char *) filename);
	    *err_code = E_SX0000_OK;
	}

	*max_file = Sxap_cb->sxap_max_file;

	break;
    }

    if (got_semaphore)
    {
        status = sxap_sem_unlock(&local_err);

        if ( (status != E_DB_OK) && 
             (*err_code == E_SX0000_OK) )
           *err_code = local_err;
    }

        
    if (*err_code != E_SX0000_OK)
    {
	_VOID_ ule_format(*err_code, NULL,
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	*err_code = E_SX101F_SXAP_SHOW;
	status = E_DB_ERROR;
    }

    return (status);
}

/*
** Name: sxap_chk_cnf, check current configuration.
**
** Description: Checks the configuration of the
**	shared system to see if log switch is required.
** 	As a side effect if the lock value has changed may also
** 	switch to a new file.
**
** Inputs:
**	lk_val	- current access lock value, aka audit log stamp
**
** Outputs:
**	err_code - error code
**
** History:
**	20-aug-1992 (markg)
** 		Written as stub.
**	09-dec-1992 (robf)
**		Coded operational.
**	10-mar-94 (robf)
**              Added traceback message E_SX10BB_SXAP_CHK_PROTO_INFO
**	20-Jun-2007 (jonj)
**	    In a cluster, do what the comments say, checking server
**	    stamp vs buffer stamp, not current stamp vs server stamp.
**	22-May-2008 (jonj)
**	    After switching to node-changed "stamp" file,
**	    sync shared memory buffer stamp to match.
**	29-May-2008 (jonj)
**	    In a cluster with multiple servers per node, this audit
**	    file may have flipped several times before this server's
**	    first write. As it will still have the server stamp it
**	    started with at startup, it'll be hopelessly out of
**	    sync with both the shared memory stamp and current
**	    value of the access lock, so just switch to the
**	    file represented by "stamp".
*/
DB_STATUS
sxap_chk_cnf(
    LK_VALUE	*lk_val,
    i4	*err_code)
{
    SXAP_STAMP 	stamp;
    SXAP_STAMP  *buff_stamp;
    SXAP_STAMP  *svr_stamp;
    DB_STATUS 	status=E_DB_OK;
    i4	local_err;

    for(;;)
    {
	    /*
	    **	At this point lock value should have the current stamp.
	    **	If 0 something went rather wrong with locking.
	    */
	    if (lk_val->lk_low==0 && lk_val->lk_high==0)
	    {
		/*
		**	Empty lock value, indicates serious problem
		*/
		*err_code=E_SX1064_CHK_CNF_NULL_LKVAL;
		status=E_DB_SEVERE;
		break;

	    }
	    /*
	    **	Important stamp values, buffer & server
	    */
	    buff_stamp= &Sxap_cb->sxap_shm->shm_stamp;
	    svr_stamp=  &Sxap_cb->sxap_stamp;

	    /*
	    **	Convert input lock value to a stamp
	    */
	    status=sxap_lockval_to_stamp(lk_val,&stamp);
	    if (status!=E_DB_OK)
	    {
		/*
		**	Should never happen, indicates serious problem
		*/
		break;
	    }
	    /*
	    **	Compare stamp to current value in server
	    */
	    if (!sxap_stamp_cmp(&stamp, svr_stamp))
	    {
		/*
		**	No change, so nothing to do.
		*/
		status=E_DB_OK;
		break;
	    }
	    /*
	    **	Stamp values differ. Need to check action here
	    */

	    /*
	    **	Check if new stamp matches the shared buffer stamp
	    */
	    if (!sxap_stamp_cmp(&stamp, buff_stamp))
	    {
		/*
		** New stamp is the same as the buffer, so shared memory
		** on this node is already updated. In this case simply
		** change this server to the new file.
		*/
		status=sxap_change_file(SXAP_CHANGE_STAMP,
			&stamp,
			NULL,
			err_code);
		/*
		**	Done processing here.
		*/
		break;
	    }
	    /*
	    **	At this point the new stamp does NOT match the shared
	    **  buffer stamp. This is only legal  in a cluster, so error
	    **	if not a cluster. This is a "protocol" error.
	    **
	    **  In a cluster, this server's stamp may well not match
	    **  the shared buffer stamp if there are multiple servers
	    **  in this node, another of which may have flopped to 
	    **  another audit file and updated the buffer stamp to match.
	    **  So just switch to the new "stamp" file.
	    */
	    if ( !(CXcluster_enabled()) )
	    {
		_VOID_ ule_format(E_SX10BB_SXAP_CHK_PROTO_INFO,
			NULL,
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 6,
			4, svr_stamp, 4, &(svr_stamp->stamp[4]),
			4, buff_stamp, 4, &(buff_stamp->stamp[4]),
			4, &stamp, 4, &(stamp.stamp[4])
			);
		*err_code=E_SX105C_SXAP_CHK_PROTOCOL;
		status=E_DB_SEVERE;
		break;
	    }
	    /*
	    ** Flush buffers first since this node may need flushing.
	    */
	    status= sxap_flushbuff(err_code);
	    if (status!=E_DB_OK)
	    {
		/*
		**	Couldn't flush the buffer, so break.
		*/
		break;
	    }
	    /*
	    ** Switch to the specified file
	    **
	    ** Note that this updates the server stamp to "stamp"
	    */
	    status=sxap_change_file(SXAP_CHANGE_STAMP,
			&stamp,
			NULL,
			err_code);

	    /* All sync'd up, update the buffer stamp */
	    if ( status == E_DB_OK )
		STRUCT_ASSIGN_MACRO(stamp, Sxap_cb->sxap_shm->shm_stamp);
	    break;
    }
    if (status!=E_DB_OK)
    {
	/*
	** Log any errors
	*/
        if (*err_code >E_SXF_INTERNAL &&
	    *err_code!=E_SX102D_SXAP_STOP_AUDIT &&
	    *err_code!=E_SX10AE_SXAP_AUDIT_SUSPEND) 
	{
		_VOID_ ule_format(*err_code, NULL,
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
		*err_code=E_SX105B_SXAP_CHK_CONF_ERR;
	}
    }
    return status;
}

/*
** sxap_change_file - change to a new audit file.
** This is called when the caller has detected the current file is full.
** It:
** 	- Closes out the current file.
**	- Run the user-exit for on-switch-log
** 	- Find the next file name.
** 	- Create/open it ready for writing.
**	- Update the lock value for the file.
**
** Inputs:
**	Operation:
**	   SXAP_CHANGE_NEXT - Change to the "next" file in the sequence.
**	   SXAP_CHANGE_STAMP- Change to file identified by "stamp".
**	   SXAP_CHANGE_NAME - Change to file named by "filename".
**
** Outputs:
**	E_DB_OK	  - File  changed OK
**	E_DB_WARN - Auditing should stop
**	E_DB_ERROR- Something went wrong.
**	E_DB_SEVERE - Serious internal problem
**	E_DB_FATAL  - Fatal problem. Server needs to shutdown
**
** History:
**	04-dec-1992 (robf)
**		Created.
**	11-mar-1993 (markg)
**	    Fixed bug where if all the audit files are full, and 
**	    SXAP_STOPAUDIT is set auditing would not get disabled
**	    cleanly.
**	2-aug-93 (robf)
**	    Add handling for SXAP_CHANGE_NAME to let caller specify
**	    the new filename.
**	    Move STOPAUDIT and SUSPEND processing to logical layer, we
**	    now return a E_DB_WARN status and let the caller figure out
**	    what to do.
**	12-jan-94 (robf)
**         Protect changing the audit log by the SXAP semaphore. This
**	   prevents any sessions that peek at the information in the
**	   CB getting invalid data while the change is going on.
**	5-apr-94 (stephenb)
**	    Integrate (robf) semaphore changes:
**		Protect changing the audit log by the SXAP semaphore. This
**		prevents any sessions that peek at the information in the
**		CB getting invalid data while the change is going on.
**	4-may-9 (robf)
**         Set curr_rscb explicitly to NULL during log switch in
**	   case it gets checked (e.g. in sxap_open of the next log when
**	   flushing records.)
**	29-May-2008 (jonj)
**	   When CHANGE_STAMP, allocate/read new page if stamp of 
**	   new file isn't the same as the shared memory stamp.
*/
DB_STATUS
sxap_change_file(
    i4		oper,
    SXAP_STAMP *stamp,
    PTR	        filename,
    i4	*err_code
)
{
    DB_STATUS 	status=E_DB_OK;
    i4 	local_err;
    SXAP_STAMP	newstamp;
    SXF_RSCB 	*rscb = Sxf_svcb->sxf_write_rscb;
    i4		find_oper;
    SXAP_RSCB	*rs;
    bool        got_semaphore=FALSE;
    i4		i;
    i4          loc_status;

    /*
    ** Increment change counter
    */
    Sxf_svcb->sxf_stats->logswitch_count++;

    for(;;)
    {
	    /*
	    ** Check the input mode.
	    ** SXAP_CHANGE_NEXT, or SXAP_CHANGE_STAMP.
	    */
	    switch(oper)
	    {
	    case SXAP_CHANGE_NEXT:
		find_oper=SXAP_FINDCUR_NEXT;
		break;
	    case SXAP_CHANGE_STAMP:
		find_oper=SXAP_FINDCUR_STAMP;
		break;
	    case SXAP_CHANGE_NAME:
		find_oper=SXAP_FINDCUR_NAME;
		break;
	    default:
		*err_code=E_SX105A_SXAP_CHANGE_PARAM;
		status=E_DB_ERROR;
		break;
	    }
	    if (status!=E_DB_OK)
		break;
	
	    /*
	    ** Special case for change by name, we check this is valid
	    ** first to allow failure without being left with no log
	    ** (since file name is user specified and could be invalid)
	    */

	    if (oper==SXAP_CHANGE_NAME)
	    {
		    status=sxap_find_curfile(find_oper,
			NULL, 
			stamp,
			filename,
			err_code);

		    if(status!=E_DB_OK)
			break;
	    }
	    /*
	    **	Close out the old file, take the SXAP semaphore first
	    **  so sessions don't try to access the rscb until its
	    **  legal again.
	    */
            if ( status = sxap_sem_lock(err_code) )
                break;
	    got_semaphore=TRUE;

	    status=sxap_close(rscb, &local_err);
	    if (status!=E_DB_OK)
	    {
		/*
		**	Unable to close the file. This should never happen,
		**	so is treated as a severe error.
		*/
		_VOID_ ule_format(local_err, NULL,
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
		*err_code=E_SX1054_SXAP_CHANGE_CLOSE;
		status=E_DB_SEVERE;
		break;
	    }
	    Sxap_cb->sxap_curr_rscb = NULL;
	    /*
	    **	Run the user-exit for onswitchlog. Only do this when 
	    **  looking for the "next" log or by name. This is the old filename.
	    */
	    if (oper==SXAP_CHANGE_NEXT || oper==SXAP_CHANGE_NAME)
	    {
		    sxap_run_onswitchlog(Sxap_cb->sxap_curr_file);
	    }
	    /*
	    ** We copy the file name over *after* running the on switch prog
	    */
	    if (oper==SXAP_CHANGE_NAME)
	    {
		STcopy(filename, Sxap_cb->sxap_curr_file);
	    }

	    /*
	    ** Find the requested file, already done this when changing
	    ** by name.
	    */
	    if(oper!=SXAP_CHANGE_NAME)
	    {
		status=sxap_find_curfile(find_oper,
			Sxap_cb->sxap_curr_file, 
			stamp,
			NULL,
			err_code);
		/*
		**	Check to see if we found a file or not
		*/
		if (status==E_DB_WARN)
		{
		
			/*
			** No file found, so make  sure we  return
			** appropriate status to caller.
			*/
			if(Sxap_cb->sxap_act_on_full==SXAP_STOPAUDIT)
			{
				_VOID_ ule_format(*err_code, NULL,
					ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);

				/* Log message saying auditing is stopped */
				_VOID_ ule_format(E_SX10A6_STOPAUDIT_LOGFULL, NULL,
					ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
				/* This error is understood by higher level code */
				*err_code=E_SX102D_SXAP_STOP_AUDIT;
				break;
			}
			else if(Sxap_cb->sxap_act_on_full==SXAP_SUSPEND)
			{
				_VOID_ ule_format(*err_code, NULL,
					ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
				/* This error is understood  by higher level code*/
				*err_code=E_SX10AE_SXAP_AUDIT_SUSPEND;
				break;
			}
			else
			{
				/*
				**	Need to shutdown the server on logfull.
				**	First log this, then die.
				*/
				*err_code=E_SX1052_SHUTDOWN_LOGFULL;
				_VOID_ ule_format(*err_code, NULL,
					ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
				_VOID_ sxap_imm_shutdown();
			}
				
		}
		else if (status!=E_DB_OK)
		{
			/*
			** Something else went wrong, treat this as a
			** fatal error.
			*/
			_VOID_ ule_format(*err_code, NULL,
				ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
			*err_code=E_SX1057_SXAP_CHANGE_NEXT;
			status=E_DB_SEVERE;
			break;
		}
	    }
	    /*
	    ** If getting the "next" file or by name create it. 
	    ** When changing to a file by stamp the file should already 
	    ** have been created, so we skip this phase.
	    */
	    if (oper==SXAP_CHANGE_NEXT || oper==SXAP_CHANGE_NAME)
	    {
		    status = sxap_create((PTR) &Sxap_cb->sxap_curr_file, 
			&local_err);
		    if (status!=E_DB_OK)
		    {
			_VOID_ ule_format(local_err, NULL,
				ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
			/*
			** Log file name now since generic code later
			** doesn't have parameters.
			*/
			_VOID_ ule_format(E_SX1059_SXAP_CHANGE_CREATE, NULL,
				ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 2,
					sizeof(Sxap_cb->sxap_curr_file),
					Sxap_cb->sxap_curr_file);
			status=E_DB_SEVERE;
			*err_code=E_SX1058_SXAP_CHANGE_LOG_ERR;
			break;
		    }
	    }
	    /*
	    **	Open the new file and set up ready to write.
	    */
	    status = sxap_open((PTR) &Sxap_cb->sxap_curr_file, 
			SXF_WRITE, rscb, NULL, (PTR)&newstamp, &local_err);
	    if (status!=E_DB_OK)
	    {
		/*
		**	This should never happen, since create should always
		**	return a valid, writable file. This is a fatal error.
		**	We log the file name now, since later generic code
		**	doesn't take parameters.
		*/
		_VOID_ ule_format(local_err, NULL,
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
		_VOID_ ule_format(E_SX1055_SXAP_CHANGE_OPEN, NULL,
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 2,
				sizeof(Sxap_cb->sxap_curr_file),
				Sxap_cb->sxap_curr_file);
		*err_code=E_SX1058_SXAP_CHANGE_LOG_ERR;
		status=E_DB_SEVERE;
		break;
	    }

	    rs = (SXAP_RSCB *) rscb->sxf_sxap;
	    if (oper==SXAP_CHANGE_NEXT ||
		oper==SXAP_CHANGE_NAME)
	    {
		/*
		**	Read page 1 into buffer
		*/
		status=sxap_read_page(&rs->rs_di_io,
			(i4)1,
			(PTR) rs->rs_auditbuf->apb_auditpage,
			Sxap_cb->sxap_page_size,
			&local_err);
		if (status!=E_DB_OK)
		{
			*err_code=local_err;
			break;
		}
	    }
	    else if (oper==SXAP_CHANGE_STAMP)
	    {
		/*
		** Allocate/read new page into buffer if
		** new file stamp isn't the same as shared memory stamp
		*/
		if (sxap_stamp_cmp(&newstamp, &Sxap_cb->sxap_shm->shm_stamp))
		{
			i4	newpage;
			/*
			**	Allocate / read new page into buffer
			*/
			status=sxap_alloc_page(&rs->rs_di_io,
			     (PTR) rs->rs_auditbuf->apb_auditpage,
			     &newpage, 
			     rs->rs_pgsize, 
			     &local_err);
			if (status!=E_DB_OK)
			{
				*err_code=local_err;
				break;
			}
		}
	    }
	    /*
	    **	If "stamp" was specified, fill in the new value
	    */
	    if(stamp!=NULL)
		STRUCT_ASSIGN_MACRO(newstamp,*stamp);
	    /*
	    **	Save the internal "current" file for writing
	    */
	    Sxap_cb->sxap_curr_rscb = (SXAP_RSCB*) rscb->sxf_sxap;
	    /*
	    **	Update the internal server stamp value
	    **	so things stay in sync. At this point, this server
	    **	is consistent
	    */
	   
	    STRUCT_ASSIGN_MACRO(newstamp,Sxap_cb->sxap_stamp);

	    break;

   }
   if(got_semaphore)
   {
     loc_status =  sxap_sem_unlock(&local_err);

     if ( (status == E_DB_OK) &&
          (loc_status != E_DB_OK) )
     {
        status = loc_status;
        *err_code = local_err;
     }
   }

   if (status!=E_DB_OK)
   {
	/*
	**	Log any errors
	*/
        if (*err_code >E_SXF_INTERNAL && 
	    *err_code!=E_SX1058_SXAP_CHANGE_LOG_ERR &&
	    *err_code!=E_SX102D_SXAP_STOP_AUDIT &&
	    *err_code!=E_SX10AE_SXAP_AUDIT_SUSPEND)
	{
		_VOID_ ule_format(*err_code, NULL,
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
		*err_code=E_SX1058_SXAP_CHANGE_LOG_ERR;
	}
   }
   else
   {
	   /*
	   **	Log the fact we are switching logs
	   */
	  _VOID_ ule_format(I_SX1056_CHANGE_AUDIT_LOG, NULL,
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 2,
			STlength(Sxap_cb->sxap_curr_file), 
				Sxap_cb->sxap_curr_file);
   }
   return (status);
}

/*
** sxap_find_curfile - finds the next current file, handling
** various cases appropriately.
**
** Inputs:
**	oper, INIT, NEXT, NAME or by STAMP.
**
**	If by STAMP is specified, then the "stamp" parameter must not be
**	null.
**
** 	If by NAME is specified then the "filename" parameter must not be 
**	null. When looking up by name we only check for non-existent files,
**	since switching to a file which already has audit records in could
**	have unexpected side effects
**
** Outputs: 
**	Status:
**		E_DB_ERROR:	Something went wrong
**		E_DB_WARN:	No file could be found.
**		E_DB_OK:        Next file  found.
**
**	Fills in fileloc with the  new filename, if found.
**	
** History:
**	04-dec-1992 (robf)
**		Created.
**	02-aug-93 (robf)
**	     Added handling for lookup by NAME (SXAP_FINDCUR_NAME)
**	19-oct-93 (stephenb)
**	    Change PM parameters in calls to PMget() to be compatible with CBF.
**	5-may-94 (robf)
**          When looking for a new file, only consider those which
**	    have the same page size as currently in use.
**	28-May-2008 (jonj)
**	    INIT may pass a shm stamp value. If so, find it first before
**	    resorting to "most recent" file.
*/
FUNC_EXTERN DB_STATUS 
sxap_find_curfile(
    i4			oper,
    char		*fileloc,
    SXAP_STAMP		*stamp,
    PTR			filename,
    i4		*err_code
) 
{
	DB_STATUS 	status=E_DB_OK;
	i4 	local_err;
	i4 	filesize;
	STATUS  	pmstat;
	char 		*pmvalue;
	char 		pmname[40];
	i4 		ndx;
	i4		use_ndx=0;
	i4		use_filesize=0;
	SXAP_STAMP 	use_stamp;
	bool		rscb_built=FALSE;
        SXF_RSCB	*rscb;
    	SXAP_RSCB	*rs;
	SXAP_STAMP	logstamp;
	i4		maxfilesize=0;

	if (oper!=SXAP_FINDCUR_INIT &&
	    oper!=SXAP_FINDCUR_NEXT &&
	    oper!=SXAP_FINDCUR_NAME &&
	    oper!=SXAP_FINDCUR_STAMP )
	{
		/*
		**	Bad operation.
		*/
		*err_code=E_SX104F_SXAP_FINDCUR_OPER;
		return E_DB_ERROR;
	}
	if (oper==SXAP_FINDCUR_STAMP && stamp==NULL)
	{
		/*
		**	Bad operation - need a stamp when looking by stamp.
		*/
		*err_code=E_SX104F_SXAP_FINDCUR_OPER;
		return E_DB_ERROR;
	}
	if (oper==SXAP_FINDCUR_NAME && filename==NULL)
	{
		/*
		**	Bad operation - need a filename when looking by name.
		*/
		*err_code=E_SX104F_SXAP_FINDCUR_OPER;
		return E_DB_ERROR;
	}
	*err_code=E_SX0000_OK;
	/*
	** Maximum size of a file, in KB.
	*/
	maxfilesize= Sxap_cb->sxap_max_file;

	for(;;) 	/* Something to break out of */
	{

		/*
		**    We loop round each logfile in turn looking for files. 
		**    When INIT:
		**	Pick lowest which either doesn't exist or isn't full
		**    When NEXT:
		**	Pick lowest which doesn't exist.
		**    When STAMP:
		**	Pick one which matches stamp.
		**    When NAME:
		**	Pick one which matches name
		*/
		for(ndx=1; ndx<=SXAP_MAX_LOGFILES; ndx++)
		{
			STprintf(pmname,"II.*.C2.LOG.AUDIT_LOG_%d", ndx);
			pmstat=PMget(pmname,&pmvalue);
			if (pmstat!=OK)
			{
				/*
				**	Not found, so stop search
				*/
				break;
			}
			/*
			** If looking up by NAME, check if this is the
			** file requested
			*/
			if(oper==SXAP_FINDCUR_NAME)
			{
				if(STcompare(filename,pmvalue))
					continue;
			}
			/*
			**	Found a name, lets see if it exists,
			**	first need to a build a RSCB for the file
			*/
			if (rscb_built==FALSE)
			{
				status = sxau_build_rscb(&rscb, &local_err);
				if (status != E_DB_OK)
				{
				    *err_code = local_err;
				    if (*err_code >E_SXF_INTERNAL)
					_VOID_ ule_format(*err_code, NULL,
						ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
				    break;
				}
				rscb->sxf_scb_ptr = NULL;
				rscb->sxf_rscb_status |= SXRSCB_READ;
				rscb_built = TRUE;
			}

			status = sxap_open(
				pmvalue, SXF_READ, rscb, &filesize, (PTR)&logstamp, 
						&local_err);
			if (status != E_DB_OK)
			{
			    status=E_DB_OK;
			    if (local_err==E_SX0015_FILE_NOT_FOUND)
			    {
				/*
				**	No such file, so we can use this if 
				**	looking for NEXT or NAME
				*/
				if(oper==SXAP_FINDCUR_NEXT ||
				   oper==SXAP_FINDCUR_NAME)
				{
					use_ndx=ndx;
					use_filesize=0;	/* Must be empty */
					break;
				}
				else if(oper==SXAP_FINDCUR_STAMP)
				{
					/*
					** Ignore for STAMP.
					*/
					continue;
			        }
				else
				{
					/*
					** For INIT, remember this with 
					** a zero size. then continue
					*/  
					if (!use_ndx)
					{
						use_filesize=0;
						use_ndx=ndx;
					}
					continue;
				}
			    }
			    else
			    {
				/*
				**	Something else went wrong opening the
				**	file. Go on to next (semi-fault 
				**	tolerant). Log a message to be friendly.
				*/
				_VOID_ ule_format(local_err, NULL, ULE_LOG, 
					    NULL, NULL, 0L, NULL, err_code, 0);
				_VOID_ ule_format(E_SX1050_SXAP_FINDCUR_LOGCHK,
						NULL,ULE_LOG, NULL, NULL, 0L,
						NULL, err_code, 2,
							STlength(pmname),
							pmname);
				continue;
			    }
			}
			rscb->sxf_rscb_status |= SXRSCB_OPENED;
			/*
			** Check page size, must match current page size
			** for writing. Skip any files which don't match
			*/
			rs = (SXAP_RSCB *) rscb->sxf_sxap;
			if(rs->rs_pgsize != Sxap_cb->sxap_page_size )
			{
				/* No match so close out the file */
		       		_VOID_ sxap_close(rscb, &local_err);
				continue;
			}
			/*
			**	Got a possible file. 
			**	Check the stamp for INIT, NAME or  STAMP to see
			**	if we should use it.
			*/
			if (oper==SXAP_FINDCUR_INIT)
			{
				/*
				** If seeking a specific stamp and this is it,
				** we're done.
				*/
				if ( stamp && !sxap_stamp_cmp(&logstamp, stamp) )
				{
				    use_ndx = ndx;
				    _VOID_ sxap_close(rscb, &local_err);
				    break;
				}
				/*
				** If got a previous possibility, then
				** see if new file is newer.
				*/
				if (filesize <  maxfilesize )
				{
					/*
					** File small enough to use, check if
					** latest.
					*/
					if (!use_ndx || !use_filesize || 
					    sxap_stamp_cmp(&logstamp, &use_stamp)>0) 
					{
						use_ndx=ndx;
						use_filesize=filesize;
						STRUCT_ASSIGN_MACRO(logstamp,use_stamp);
					}
				}
			}
			else if (oper==SXAP_FINDCUR_STAMP)
			{
				if (!sxap_stamp_cmp(stamp,&logstamp))
				{
					/*
					**	Found it, so save and quit.
					*/
					use_ndx=ndx;
				       _VOID_ sxap_close(rscb, &local_err);
				       break;
				}
			}
			else if (oper==SXAP_FINDCUR_NAME)
			{
				/*
				** Found by name and opened, this can't be
				** used.
				*/
			        _VOID_ sxap_close(rscb, &local_err);
				use_ndx= 0;
				*err_code=E_SX0026_USER_LOG_EXISTS;
			        break;
			}
			/*
			**	Done so close the file
			*/
		       _VOID_ sxap_close(rscb, &local_err);

		} /* End loop over all files */
		/*
		**	If we built an rscb clean it up
		*/
		if (rscb_built==TRUE)
			_VOID_ sxau_destroy_rscb(rscb, &local_err);
		/*
		**	See if found any files to use
		*/
		if (use_ndx==0)
		{
			/*
			**	No files found! See if because no names
			**	specified or all full/used.
			*/
			if (ndx==1)
				*err_code=E_SX104B_SXAP_NO_LOGNAMES;
			else if(oper==SXAP_FINDCUR_NAME)
			{
				if(*err_code==E_SX0000_OK)
					*err_code=E_SX0027_USER_LOG_UNKNOWN;
			}
			else
				*err_code=E_SX104C_SXAP_NEED_AUDITLOGS;
			status=E_DB_WARN;
			break;
		}
		/*
		**	Found a file to use OK
		*/
		status=E_DB_OK;
		/*
		**	Fill in the filename if wanted.
		*/
		if (fileloc!=NULL)
		{
			STprintf(pmname,"II.*.C2.LOG.AUDIT_LOG_%d", use_ndx);
			pmstat = PMget(pmname, &pmvalue);
			if (pmstat!=OK)
			{
				/*
				**	Should never happen - the file name
				**	appears to have "vanished"
				*/
				status=E_DB_ERROR;
				*err_code=E_SX1051_SXAP_FINDCUR_BADGET;
				break;
			}
			STcopy(pmvalue,fileloc);
		}
		break;
	}
	return status;
}

/*
** Name: sxap_imm_shutdown - shutdown right now.
**
** Description:	This is used to terminate a server on a SHUTDOWN audit 
**	operation. It ensures users cannot continue processing without 
**	any audit records being written.
**
** History:
**	10-dec-1992 (robf)
**		Initial Creation.
*/
VOID
sxap_imm_shutdown(void)
{
	i4	active;
	i4 local_err;
	/*
	**	Log that we are shutting down
	*/
	_VOID_ ule_format(E_SX1053_SHUTDOWN_NOW, NULL,
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	/*
	**	Call CS to terminate us
	*/
	_VOID_ CSterminate(CS_KILL,&active);
	/*
	**	In case CS decided to return (shouldn't), really exit
	*/
	PCexit(0);
}
/*
** Name: sxap_stamp_cmp.
**
** Description: Compares two audit stamps.
**
** Inputs:
**	Pointers to two stamps
**
** Outputs:
**	0 if stamps the same
**	>0 if s1 > s2
**	<0 if s1 < s2
**
** History:
**	07-dec-1992 (robf)
**		Initial Creation.
**	22-dec-1998 (kosma01)
**		For sqs_ptx do a numeric compare rather than a string compare.
**              On this, a "little endian" machine numbers are stored with 
**              their least significant byte first.  Therefore a simple string
**              compare will sometimes give false results.  Specifically, when
**              tests c2_aud46 and c2_aud47 are successfully run, a ckpdb fails 
**              due to attempting to open the wrong audit file. 
**	01-mar-1999 (musro02)
**          Modified above change to sxap_stamp_cmp so it is generic.
*/
i4 
sxap_stamp_cmp(
	SXAP_STAMP *s1,
	SXAP_STAMP *s2
) {
   i4 *is1, *is2;
   is1 = (i4 *)s1;
   is2 = (i4 *)s2;
   return (*is1 - *is2);
}

/*
** Name: sxap_stamp_to_lockval - converts a file STAMP to a LK_VALUE
** 	 sxap_lockval_to_stamp - converts a LK_VALUE to a STAMP
**
** History:
**	07-dec-1992 (robf)
**		Created
**	5-apr-94 (stephenb)
**	    stamp offset should be 4, not 3.
*/
DB_STATUS
sxap_stamp_to_lockval (
	SXAP_STAMP *stamp,
	LK_VALUE   *lockval 

) {
	if(stamp==NULL || lockval==NULL)
		return E_DB_ERROR;
	
	MEcopy((PTR)&stamp->stamp,4,(PTR)&(lockval->lk_low));
	MEcopy((PTR)&stamp->stamp[4],4,(PTR)&(lockval->lk_high));
	return E_DB_OK;

}
DB_STATUS
sxap_lockval_to_stamp (
	LK_VALUE   *lockval,
	SXAP_STAMP *stamp

) {
	if(stamp==NULL || lockval==NULL)
		return E_DB_ERROR;

	MEcopy((PTR)&(lockval->lk_low), 4, (PTR)&stamp->stamp);
	MEcopy((PTR)&(lockval->lk_high), 4, (PTR)&stamp->stamp[4]);
	return E_DB_OK;
}
/*
** Name: sxap_stamp_now
**
** Description: 
**	Fills stamp with the date/time "now". This currently only uses the
**	first 4 bytes of the stamp.
**
** Outputs:
**	- Log file stamp.
**
** History:
**	07-dec-1992 (robf)
**		Created.
*/
VOID
sxap_stamp_now (
	SXAP_STAMP *stamp
) {
	i4 t;
	i4 len;

	if (stamp==NULL)
		return;
	/*
	**	Blank out the stamp 
	*/
	MEfill(sizeof(SXAP_STAMP),0,(PTR)stamp);
	/*
	**	Get the time "now"
	**	NOTE: this assumes  the binary representation of 
	**	TMsecs is always increasing. If not, the sequencing will
	**	have to be re-visited
	*/
	t=TMsecs();
	if (sizeof(t)<sizeof(*stamp))
		len=sizeof(t);
	else
		len=sizeof(*stamp);
		    
	MEcopy((PTR)&t, len, (PTR)&(stamp->stamp));
}
/*
** Name: sxap_run_onswitchlog.
**
** Description:
**	Run the user exit handler associated with ONSWITCHLOG. 
**	Exit is run with NOWAIT so processing can continue in the
**	server while it is running.
**
**	This routine should get called exactly once per installation
**	log file switch.
**
** Inputs:
**	- Name of the OLD audit log.
**
** History:
**	 7-dec-1992 (robf)
**		 Created
**	2-mar-94 (robf)
**           Suppress E_US0001 errors being logged when PCcmdline simply
**	     returns FAIL. We still log a valid status return, but suppress 
**	     the FAIL case.
**	     Also changed the ifdef VMS to the actual #define which should
**	     be somewhat more portable.
**	26-apr-94 (robf)
**           Log name is NULL-terminated, so format that way
**	22-May-2008 (jonj)
**	     Delete obsolete #ifdef PC_NOWAIT
*/
VOID
sxap_run_onswitchlog(
	char	*filename	/* Name of the logfile that is full */
)
{
	char 		*switchprog;
	char 		command_buf[512];
	i4		local_err;
	CL_ERR_DESC 	clerr;
	STATUS     	stat;
static  i4	        num_tries=0;

	if (Sxap_cb->sxap_status&SXAP_SWITCHPROG)
	{
		/*
		**	Log a message the program is being run
		*/
		_VOID_ ule_format(I_SX105D_RUN_ONSWITCHPROG, NULL,
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 2,
			STlength(Sxap_cb->sxap_switch_prog),
			Sxap_cb->sxap_switch_prog);
	
		switchprog=Sxap_cb->sxap_switch_prog;
		STprintf(command_buf,"%s %s",switchprog,filename);

		stat=PCcmdline((LOCATION *)NULL, command_buf, PC_NO_WAIT, 
				 (LOCATION *)NULL , &clerr);

		if (stat!=OK)
		{
			/*
			** Only log status if meaningful, 'fail' is logged
			** as "E_US0001 Number of users has exceeded limit", 
			** which isn't very helpful.
			*/
			if(stat!=FAIL)
			{
			    ule_format(stat, &clerr, ULE_LOG, NULL,
				(char*)0,  (i4)0, (i4 *)0, &local_err, 0);
			}	
			if (num_tries++>5)
			{
				/*
				**	Too many failures to run so
				**	disable attempting to run program
				*/
				local_err=E_SX105E_RUN_SWITCH_TOOMANY;
				Sxap_cb->sxap_status  &= ~SXAP_SWITCHPROG;
			}
			else
				local_err=E_SX105F_RUN_SWITCH_ERROR;
			/*
			** Log the failure
			*/
			_VOID_ ule_format (local_err, NULL,
				ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
		}
		else
		{
			/*
			**	On success reset try counter
			*/
			num_tries=0;
		}
	}
	return;
}

/*
** Name: sxap_alter.
**
** Description: 
**	Alter the SXAP state, including  current audit file. This uses
**	the physical layer access lock to make sure  things do not change
**	underneath  us.
**
** Inputs:
**	filename  - New audit file name (optional)
**
**	flags	  - Operation flags
**
** Outputs:
**	err_code  - Error code
**
** Returns
**	DB_STATUS
**
** History:
**	2-aug-93 (robf)
**		Created.
**	13-jan-94 (robf)
**          Take SXAP semaphore prior to stopping audit. 
**	14-apr-94 (robf)
**          Add support for audit writer, SXAP_C_AUDIT_WRITER
**	01-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameters that will be passed to LKrequest
**	    to avoid random implicit lock conversions.
*/
DB_STATUS 
sxap_alter (
	SXF_SCB	*scb,
	PTR 	filename,
	i4	flags,
	i4 *err_code
)
{
    DB_STATUS status=E_DB_OK;
    i4	local_err;
    bool	locked = FALSE;
    STATUS	cl_status;
    CL_ERR_DESC	cl_err;
    LK_LKID	lk_id;
    LK_VALUE	lk_val;
    LK_LKID	lk_qid;
    LK_VALUE	lk_qval;
    LK_LLID	lk_ll = scb->sxf_lock_list;
    SXAP_STAMP	stamp;
    SXF_RSCB 	*rscb = Sxf_svcb->sxf_write_rscb;
    bool	got_semaphore=FALSE;
    bool	q_locked=FALSE;

    MEfill(sizeof(LK_LKID), 0, &lk_id);
    MEfill(sizeof(LK_LKID), 0, &lk_qid);

    for(;;)
    {
	if (flags&SXAP_C_AUDIT_WRITER)
	{
	    /*
	    ** This is the audit writer thread, we go and flush audit
	    ** records as necessary until no more.
	    */
	    status=sxap_audit_writer(scb, err_code);
	    break;
	    break;
	}
	/*
	** Take the audit queue lock so no new audit records can be
	** queued up.
	*/

	status=sxap_q_lock(scb, LK_X, &lk_qid, &lk_qval, err_code);
	if(status!=E_DB_OK)
		break;
	q_locked=TRUE;
	/*
	** Now we have things locked, we need to drain the queue.
	*/
	status=sxap_qallflush(scb, err_code);
	if(status!=E_DB_OK)
		break;

	/*
	** Take the audit access lock so things don't change underneath us
	*/
	status=sxap_ac_lock(scb, LK_X, &lk_id, &lk_val, err_code);
	if(status!=E_DB_OK)
		break;
	locked=TRUE;
	/*
	** Check on flags for operation to perform. 
	*/
	if (flags&SXAP_C_STOP_AUDIT)
	{
                if ( status = sxap_sem_lock(err_code) )
                    break;
		got_semaphore=TRUE;
		/*
		** Stop auditing. This marks the physical layer as
		** inactive.
		*/
		if(!(Sxap_cb->sxap_status & SXAP_ACTIVE))
			break;
		status=sxap_stop_audit(err_code);
	        if(status!=E_DB_OK)
			break;
		Sxap_cb->sxap_status &= ~SXAP_ACTIVE;
	}
	else if (flags&SXAP_C_RESTART_AUDIT)
	{
		/*
		** Restart auditing. This marks the physical layer as
		** active, and also checks the configuration to do any 
		** required log switches etc.
		*/
		if(Sxap_cb->sxap_status & SXAP_ACTIVE)
			break;
		_VOID_ sxap_lockval_to_stamp(&lk_val,&stamp);
		status=sxap_restart_audit(&stamp, err_code, filename);
	        if(status!=E_DB_OK)
			break;
		_VOID_ sxap_stamp_to_lockval(&stamp,&lk_val);
		Sxap_cb->sxap_status |= SXAP_ACTIVE;
	}
	else if (flags&SXAP_C_CHANGE_FILE)
	{
		if(!filename)
		{
			/* Internal error, no file name passed in */
			*err_code=E_SX0004_BAD_PARAMETER;
			status=E_DB_SEVERE;
			break;
		}
		status=sxap_change_file(SXAP_CHANGE_NAME,
			&stamp,
			filename,
			err_code);

		if(status!=E_DB_OK)
		{
	    	    _VOID_ ule_format(E_SX1036_AUDIT_FILE_ERROR, NULL, 
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 1, 
			STlength (filename), (PTR) filename);
			break;
		}
		/*
		**	Convert the new log stamp to a lock value.
		**	This will cause the lock stamp to be updated when
		**	we release the lock below. 
		*/
		_VOID_ sxap_stamp_to_lockval(&stamp,&lk_val);
		/*
		** Update the shared buffer manager stamp value. At this
		** point the node will be sync. Note we don't do this
		** in sxap_change_file since here we can guarantee we
		** have the access lock.
		*/
		STRUCT_ASSIGN_MACRO(stamp,Sxap_cb->sxap_shm->shm_stamp);
	}

	/*
	** Done so release access lock
	*/
	status=sxap_ac_unlock(scb, &lk_id, &lk_val, err_code);
	if(status!=E_DB_OK)
		break;
	locked = FALSE;
	break;
    }
    if(locked)
    {
	(VOID)sxap_ac_unlock(scb, &lk_id, &lk_val, err_code);
    }
    if(q_locked)
    {
	(VOID)sxap_q_unlock(scb, &lk_qid, &lk_qval, err_code);
    }
    if(got_semaphore)
    {
       cl_status = sxap_sem_unlock(&local_err);
       if ( (status == E_DB_OK) &&
            (cl_status != E_DB_OK) )
       {
          status = cl_status;
          *err_code = local_err;
       }
    }
    return status;
}

/*
** Name: sxap_stop_audit
**
** Description:
**	This routine implements the SXAP_C_STOP_AUDIT request and
**	stops auditing at the physical layer.
**
**	It assumes that the audit access lock has been taken in X mode
**	prior to being called so no other servers change at the same time
**
** Inputs:
**	None
**
** Outputs:
**	err_code:	Error status
**
** Returns:
**	DB_STATUS
**
** History:
**	6-aug-93 (robf)
**	    Created
**/
static DB_STATUS
sxap_stop_audit( i4 *err_code )
{
    DB_STATUS 	status=E_DB_OK;
    SXF_RSCB 	*rscb = Sxf_svcb->sxf_write_rscb;
    i4	local_err;
    CL_ERR_DESC	cl_err;
    i4		i;

    for(;;)
    {
	/* 
	** Close the physical file, if active. 
	** Note: This depends on rscb->sxf_sxap being cleaned up
	** properly by sxap_close()
	*/
	if(rscb->sxf_sxap)
	{
		status=sxap_close(rscb, &local_err);
		if(status)
		{
			_VOID_ ule_format(local_err, &cl_err, ULE_LOG,	
				NULL, NULL, 0L, NULL, &local_err, 0);
			*err_code=E_SX10AF_SXAP_STOP_CLOSE_ERR;
			break;
			
		}
	        Sxap_cb->sxap_curr_rscb = NULL;
	}
	break;
    }
    return status;
}

/*
** Name: sxap_restart_audit
**
** Description:
**	This routine implements the SXAP_C_RESTART_AUDIT request and
**	restarts auditing at the physical layer.
**
**	It assumes that the audit access lock has been taken in X mode
**	prior to being called so no other servers change at the same time.
**
** Inputs:
**	lk_stamp	Current lock stamp
**
**	filename	New audit file, NULL indicates used default.
**
** Outputs:
**	err_code:	Error status
**
** Returns:
**	DB_STATUS
**
** History:
**	6-aug-93 (robf)
**	    Created
**	5-may-94 (robf)
**          Re-initialize the buffer page on restart.
**/
static DB_STATUS
sxap_restart_audit( SXAP_STAMP *lk_stamp, i4 *err_code, char *filename )
{
    DB_STATUS 	status=E_DB_OK;
    SXF_RSCB 	*rscb = Sxf_svcb->sxf_write_rscb;
    i4	local_err;
    SXAP_STAMP  *buff_stamp;
    SXAP_STAMP  *svr_stamp;
    SXAP_STAMP	newstamp;
    SXAP_RSCB   *rs;
    CL_ERR_DESC	cl_err;
    bool	created=FALSE;
    i4	newpage;

    for(;;)
    {
	/*
	** Check if the current lock value is valid for a
	** current audit log, or requested file if specified
	*/
	if(filename)
	{
	    status=sxap_find_curfile( 
		SXAP_FINDCUR_NAME, 
		Sxap_cb->sxap_curr_file, 
		lk_stamp, filename,
		&local_err);
	    /*
	    ** If we can't find the requested file, give up
	    */
	    if (status!=E_DB_OK)
	    {
		*err_code=local_err;
	    	_VOID_ ule_format(E_SX1036_AUDIT_FILE_ERROR, NULL, 
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 1, 
			STlength (filename), (PTR) filename);
		status=E_DB_ERROR;
		break;
	    }
	}
	else
	{
	    status=sxap_find_curfile( 
		SXAP_FINDCUR_STAMP, 
		Sxap_cb->sxap_curr_file, 
		lk_stamp, NULL,
		&local_err);
	}
	if(status==E_DB_WARN)
	{
		/*
		** Lock stamp is not valid for any current audit file,
		** so we need to find a new file
		*/
		status=sxap_find_curfile(
			SXAP_FINDCUR_INIT,
			Sxap_cb->sxap_curr_file, 
			(SXAP_STAMP*)NULL, NULL,
			&local_err);

		if(status!=E_DB_OK)
		{
			/*
			** Check for all logfiles used,  this is just
			** a warning to the caller.
			*/
			if(local_err==E_SX104C_SXAP_NEED_AUDITLOGS)
			{
				status=E_DB_WARN;
			        *err_code=local_err;
			}
			else
			{
			    _VOID_ ule_format(local_err, 0, ULE_LOG,	
				NULL, NULL, 0L, NULL, &local_err, 0);
			    *err_code=E_SX10B0_SXAP_RESTART_INIT_ERR;
			}
		        break;
		}
	}
	else if(status!=E_DB_OK)
	{
		*err_code=local_err;
		break;
	}
	created=FALSE;
	/*
	** At this point we should have a valid file name in
	** Sxap_cb->sxap_curr_file, so try to open it. Check for file
	** not existing on open, and if so create it.
	*/
	for(;;)
	{
	    status = sxap_open((PTR) &Sxap_cb->sxap_curr_file, 
		SXF_WRITE, rscb, NULL, (PTR)&newstamp, &local_err);
	    if(status!=E_DB_OK)
	    {
		if(local_err==E_SX0015_FILE_NOT_FOUND)
		{
			/*
			** No such file, so try to create
			*/
		        status = sxap_create((PTR) &Sxap_cb->sxap_curr_file, 
				&local_err);
			if(status!=E_DB_OK)
			{
				_VOID_ ule_format(local_err, &cl_err, ULE_LOG,	
					NULL, NULL, 0L, NULL, &local_err, 0);
				*err_code=E_SX10B1_SXAP_RESTART_CREATE;
				break;
			}
			else
			{
				/*
				** Now we have created the file, lets
				** go and open it
				*/
				created=TRUE;
				continue;
			}
		}
		else
		{
			/*
			** Open failed for some other reason
			*/
			_VOID_ ule_format(local_err, &cl_err, ULE_LOG,	
				NULL, NULL, 0L, NULL, &local_err, 0);
			*err_code=E_SX10B2_SXAP_RESTART_OPEN;
			break;
		}
	    }
	    break;
	}
	if(status!=E_DB_OK)
		break;
	/*
	** If we get here, the audit log has been opened, so 
	** set the buffer and server log stamps to the new value.
	** Also pass back the new stamp so the caller can update
	** the access lock id
	*/
	Sxap_cb->sxap_stamp=newstamp;
        Sxap_cb->sxap_shm->shm_stamp = newstamp;
	*lk_stamp=newstamp;
	/*
	**	Read next page into shared buffer
	*/
        rs = (SXAP_RSCB *) rscb->sxf_sxap;
	if(created)
	{
	    /*
	    ** Read page 1 for new file.
	    */
	    status=sxap_read_page(&rs->rs_di_io,
		(i4)1,
		(PTR) rs->rs_auditbuf->apb_auditpage,
		Sxap_cb->sxap_page_size,
		&local_err);
	}
	else
	{
	    /*
	    ** Allocate/read next page for existing file.
	    */
	    status=sxap_alloc_page(&rs->rs_di_io,
	    	(PTR) rs->rs_auditbuf->apb_auditpage,
	    	&newpage, 
	    	rs->rs_pgsize, 
	    	&local_err);
	}
	if (status!=E_DB_OK)
	{
		*err_code=local_err;
		break;
	}
	/*
	**	Save the internal "current" file for writing
	*/
	Sxap_cb->sxap_curr_rscb = (SXAP_RSCB*) rscb->sxf_sxap;
	/*
	** Log new audit file
	*/
	_VOID_ ule_format(I_SX1056_CHANGE_AUDIT_LOG, NULL,
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 2,
		STlength(Sxap_cb->sxap_curr_file),
		Sxap_cb->sxap_curr_file);
	break;
    }
    return status;
}
