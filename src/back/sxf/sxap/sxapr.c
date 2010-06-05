/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
NO_OPTIM = nc4_us5
*/

# include    <compat.h>
# include    <gl.h>
# include    <pc.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <dudbms.h>
# include    <cs.h>
# include    <lk.h>
# include    <tm.h>
# include    <di.h>
# include    <lo.h>
# include    <me.h>
# include    <st.h>
# include    <ulf.h>
# include    <sxf.h>
# include    <sxfint.h>
# include    <sxap.h>
# include    "sxapint.h"

/*
** Name: SXAPR.C - record access routines for the SXAP auditing system
**
** Description:
**      This file contains all of the record access routines used by the SXF
**      low level auditing system, SXAP.
**
**          sxap_position   - position a record stream for reading
**          sxap_read	    _ read an audit record from a record stream
**          sxap_write	    - write an audit record to the audit buffer
**          sxap_flush	    - flush records for a session to disk
**	    sxap_read_page  - read a page from an audit file
**	    sxap_write_page - write a page to an audit file
**	    sxap_alloc_page - allocate the next free page from an audit file.
**	    sxap_audit_writer- Security audit queue writer.
**
**	Static routines included in this file:
**	    check_sum	    - calculate the checksum of an audit page
**	    compress record - compress an audit record
**	    uncompress_record - uncompress an audit record
**	    comp_field_len  - find the length of a compressed field
**	    write_shared_page- Write the shared page to disk
**	    flush_shared_page- Flush the shared page to disk.
**	    start_audit_writer-Start audit writer if necessary.
**
** History:
**      20-sep-1992 (markg)
**          Initial creation.
**	01-dec-1992 (markg)
**	    Added code to compress audit records by removing trailing
**	    spaces from character fields.
**	10-feb-1993 (markg)
**	    Removed unnecessary include of ulm.h.
**	14-mar-1993 (markg)
**	    Fixed bugs in sxap_write and sxap_flush, which caused AVs
**	    if the routines were called when auditing had been disabled.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	3-aug-93 (stephenb)
**	    fixed bug in sxap_write where routine would return on error
**	    E_SX102D_SXAP_STOP_AUDIT witout releasing lock.
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
**    27-oct-93 (stephenb)
**        Fix bug in sxap_flush() where loop would break on error and
**        not check wether the lock was held, thus returning with the lock.
**    	8-jul-94 (robf/stephenb)
**        Error handling in several places could cause a potential AV
**        by trying to do STlength(r->rs_filename) when r/filename may be
**        NULL. Updated error handling to only try and traceback the filename
**        if its available.
**      20-nov-1994 (andyw)
**          Added NO_OPTIM due to Solaris 2.4/Compiler 3.00 errors
**      1-june-1995 (hanch04)
**          Removed NO_OPTIM su4_us5, installed sun patch
**      15-jun-95 (popri01)
**          Add NO_OPTIM for nc4_us5 (NCR C Development Toolkit - 2.03.01)
**	    to eliminate  a security audit file access error indicated by
**	    messages E_GW4064 (sxa read error) and E_SX1065 (bad record 
**	    length), which appeared in c2secure sep test c2log02.
**	04-Aug-1997 (jenjo02)
**	    Removed sxf_incr(). It's gross overkill to semaphore-protect a
**	    simple statistic.
**	27-Aug-1997 (jenjo02)
**	    Use sxap_semaphore to guard sxap_status & SXAP_AUDIT_WRITER_BUSY.
**	    We were CSresume-ing the audit writer thread while it was becoming
**	    active and making a lock request, leading LKrequest() to fail
**	    with CL103E.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      14-sep-2005 (horda03) Bug 115198/INGSRV3422
**          On a clustered Ingres installation, LKrequest can return
**          LK_VAL_NOTVALID. This implies that teh lock request has been
**          granted, but integrity of the lockvalue associated with the
**          lock is in doubt. It is the responsility of the caller
**          to decide whether the lockvalue should be reset.
**	01-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameters that will be passed to LKrequest
**	    to avoid random implicit lock conversions.
**      01-Oct-2008 (jonj)
**          Replace CSp/v_semaphore(sxap_semaphore) calls with more robust
**          sxap_sem_lock(), sxap_sem_unlock() functions.
**      05-dec-2008 (horda03) Bug 121332
**         Before updating the sxf_sxap hold the 
**         sxap_semaphore to ensure the sxap_curr_rscb
**         has a valid value. Prevents an ACCVIO or E_SX102C.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/

/*
** Forward function references.
*/
static DB_STATUS write_shared_page(
    SXF_SCB		*scb,
    SXAP_RSCB		*rscb,
    SXF_AUDIT_REC	*audit_rec,
    LK_LKID		*lk_id,
    LK_VALUE		*lk_val,
    bool		*page_written,
    i4		*err_code);

static DB_STATUS flush_shared_page (
	SXF_SCB 	*scb,
	bool		lock_access,
	bool		force_flush,
	i4		*err_code
);
static VOID sxap_dump_audit_rec(SXF_AUDIT_REC  *rec);

static  i4  check_sum(
	PTR    		page,
	u_i4		page_size);

static  DB_STATUS  compress_record(
	SXF_AUDIT_REC	*rec,
	char		*buf,
	i4		*length,
	i4		*err_code);

static  DB_STATUS  uncompress_record(
	char		*buf,
	SXF_AUDIT_REC	*rec,
	i4		*length,
	i4		version,
	i4		*err_code);

static  i4   comp_field_len(
	char		*field,
	i4		length);

static VOID start_audit_writer(bool force_start);

static bool debug_trace=0;
/*
** Name: SXAP_POSITION - position an audit file for reading
**
** Description:
**	This routine positions an audit file reading. The caller must
**	specify a SXF_RSCB for a file that has previously been opened 
**	by a sxap_open call. Once a file has been opened for read it 
**	may be positioned multiple times. All audit files must be positioned 
**	using this routine before attempting to read records from the file
**	
**	NOTE:
**	The current version of SXAP does not support keyed record access. For 
**	the moment all sxap_position calls must specify the SXF_SCAN type.
**
** 	Overview of algorithm:-
**
**	Validate the input parameters.
**	Get the audit access lock.
**	Call sxap_read_page to read the first data page into the buffer.
**	Release the audit access lock.
**
**
** Inputs:
**	scb			SXF session control block.
**	rscb			The SXF_RSCB for this audit file
**	pos_type		Type of position operation to be performed,
**				currently this must be SXF_SCAN.
**	pos_key			Key to use for position operation, should
**				be NULL for SXF_SCAN.
**
** Outputs:
**	err_code		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**      20-sep-1992 (markg)
**	    Initial creation.
**	25-apr-94 (robf)
**          Take the access lock in shared mode.
*/
DB_STATUS
sxap_position(	
    SXF_SCB		*scb,
    SXF_RSCB		*rscb,
    i4			pos_type,
    PTR			pos_key,
    i4		*err_code)
{
    DB_STATUS		status = E_DB_OK;
    i4		local_err;
    STATUS		cl_status;
    CL_ERR_DESC		cl_err;
    LK_LKID		lk_id;
    LK_VALUE		lk_val;
    LK_LOCK_KEY		lk_key = {LK_AUDIT, SXAP_LOCK, SXAP_ACCESS, 0, 0, 0, 0};
    LK_LLID		lk_ll = scb->sxf_lock_list;
    SXAP_RSCB		*r = (SXAP_RSCB *) rscb->sxf_sxap;
    SXAP_APB		*auditbuf = (SXAP_APB *) r->rs_auditbuf;
    SXAP_DPG		*p = (SXAP_DPG *) &auditbuf->apb_auditpage;
    bool		locked = FALSE;

    MEfill(sizeof(LK_LKID), 0, &lk_id);
    *err_code = E_SX0000_OK;

    for (;;)
    {
	/*
	** Check that the position arguments are valid and
	** that the file has been opened for writing.
	*/
	if (pos_type != SXF_SCAN || pos_key != NULL)
	{
	    *err_code = E_SX0014_BAD_POSITION_TYPE;
	    break;
	}

	/* Check that we're nor reading from the write buffer */
	if ((auditbuf->apb_status & SXAP_WRITE) == SXAP_WRITE)
	{
	    *err_code = E_SX1027_BAD_FILE_MODE;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	/*
	** Before reading the page from the audit file, the
	** audit access lock must be held. This will ensure
	** that we don't read a page that is in the process of
	** being written.
	*/
	cl_status = LKrequest(LK_PHYSICAL, lk_ll, &lk_key,
			LK_S, &lk_val, &lk_id, 0L, &cl_err);
	if ( (cl_status != OK) && (cl_status != LK_VAL_NOTVALID) )
	{
            TRdisplay( "SXAPR::Bad LKrequest %d\n", __LINE__);
            _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    sxap_lkerr_reason("ACCESS,X", cl_status);
	    *err_code = E_SX1035_BAD_LOCK_REQUEST;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG, 
		NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	locked = TRUE;
	/* 
	** Read the first page from the audit file into
	** the page buffer. 
	*/
	status = sxap_read_page(&r->rs_di_io, 1L, (PTR) p, 
				r->rs_pgsize, &local_err);
	if (status != E_DB_OK)
	{
	    *err_code = local_err;
	    if (*err_code > E_SXF_INTERNAL)
	        _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	cl_status = LKrelease(0, lk_ll, &lk_id, NULL, &lk_val, &cl_err);
	if (cl_status != OK)
	{
	    *err_code = E_SX1008_BAD_LOCK_RELEASE;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG, 
		NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	locked = FALSE;

	/*
	** Initialize the record pointer in the buffer to 
	** point to the first audit page.
	*/
	auditbuf->apb_last_rec.pageno = 1;
	auditbuf->apb_last_rec.offset = CL_OFFSETOF(SXAP_DPG, dpg_tuples);

	break;
    }

    /* Handle any errors */
    if (*err_code != E_SX0000_OK)
    {
	if (locked)
	    _VOID_ LKrelease(0, lk_ll, &lk_id, NULL, &lk_val, &cl_err);

	if (*err_code > E_SXF_INTERNAL)
	{
	    if(r && r->rs_filename)
	    {
	        _VOID_ ule_format(E_SX1036_AUDIT_FILE_ERROR, NULL, 
		    ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 1, 
		    STlength (r->rs_filename), (PTR) r->rs_filename);
	    }
	    *err_code = E_SX1028_SXAP_POSITION;
	}

	if (status == E_DB_OK)
	    status = E_DB_ERROR;
    }

    return (status);
}

/*
** Name: SXAP_READ - read the next record from an audit file.
**
** Description:
**	This routine gets the next record from an audit file and returns it
**	to the caller. The audit file must have previously been opened by
**	a sxap_open (SXR_READ) call, it must also have been positioned for
**	read using a sxap_position call. If no more audit records exist in 
**	the audit file a warning will be returned to the caller.
**
** 	Overview of algorithm:-
**
**	Validate parameters passed to the routine.
**	Check that the file has been positioned for reading.
**	If (no more records on page)
**		Get the audit access lock.
**		Read the next page into buffer.
**		Release the audit access lock.
**		If (no more pages)
**			Return warning to caller
**	Read the next record from buffered page.
**		
**
** Inputs:
**	scb			SXF session control block.
**	rscb			The SXF_RSCB for the audit file 
**
** Outputs:
**	audit_rec		The audit record to be returned to the caller
**	err_code		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**      20-sep-1992 (markg)
**	    Initial creation.
**	14-dec-1992 (robf)
**	    Updated to handle syncronization of audit log stamp locks/buffers.
**	25-apr-94 (robf)
**          Take the access lock in shared mode.
*/
DB_STATUS
sxap_read( 
    SXF_SCB		*scb,
    SXF_RSCB		*rscb,
    SXF_AUDIT_REC       *audit_rec,
    i4		*err_code)
{
    DB_STATUS		status = E_DB_OK;
    i4		local_err;
    STATUS		cl_status;
    CL_ERR_DESC		cl_err;
    LK_LKID		lk_id;
    LK_VALUE		lk_val;
    LK_LOCK_KEY		lk_key = {LK_AUDIT, SXAP_LOCK, SXAP_ACCESS, 0, 0, 0, 0};
    LK_LLID		lk_ll = scb->sxf_lock_list;
    SXAP_RSCB		*r = (SXAP_RSCB *) rscb->sxf_sxap;
    SXAP_APB		*auditbuf = (SXAP_APB *) r->rs_auditbuf;
    SXAP_DPG		*p = (SXAP_DPG *) &auditbuf->apb_auditpage;
    i4		next_page;
    i4		rec_length;
    char		*char_ptr;
    bool		locked = FALSE;

    MEfill(sizeof(LK_LKID), 0, &lk_id);
    *err_code = E_SX0000_OK;
    /*
    **	Logical (buffered) read
    */
    Sxf_svcb->sxf_stats->read_buff++;

    for (;;)
    {
	if (p->dpg_header.pg_no != auditbuf->apb_last_rec.pageno)
	{
	    *err_code = E_SX1029_INVALID_PAGENO;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	while (auditbuf->apb_last_rec.offset >= p->dpg_header.pg_next_byte)
	{
	    /* 
	    ** There are no more records on this data page - go
	    ** and get the next one.
	    */
	    next_page = p->dpg_header.pg_no + 1;
	    if (next_page > r->rs_last_page)
	    {
		*err_code = E_SX0007_NO_ROWS;
		status = E_DB_WARN;
		break;
	    }

	    cl_status = LKrequest(LK_PHYSICAL, lk_ll, &lk_key, 
				LK_S, &lk_val, &lk_id, 0, &cl_err);
	    if ( (cl_status != OK) && (cl_status != LK_VAL_NOTVALID) )
	    {
                TRdisplay( "SXAPR::Bad LKrequest %d\n", __LINE__);
                _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	        sxap_lkerr_reason("ACCESS,S", cl_status);
	        *err_code = E_SX1035_BAD_LOCK_REQUEST;
		_VOID_ ule_format(*err_code, &cl_err, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
		break;
	    }
	    locked = TRUE;
	
	    status = sxap_read_page(&r->rs_di_io, 
			next_page, (PTR) p, 
			r->rs_pgsize, &local_err);
	    if (status != E_DB_OK)
	    {
		*err_code = local_err;
		if (*err_code > E_SXF_INTERNAL)
	    	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
		break;
	    }

	    cl_status = LKrelease(0, lk_ll, &lk_id, NULL, &lk_val, &cl_err);
	    if (cl_status != OK)
	    {
		*err_code = E_SX1008_BAD_LOCK_RELEASE;
		_VOID_ ule_format(*err_code, &cl_err, ULE_LOG,	
			NULL, NULL, 0L, NULL, &local_err, 0);
		break;
	    }
	    locked = FALSE;

	    /*
	    ** Initialize the record pointer in the buffer to 
	    ** point to the first audit page.
	    */
	    auditbuf->apb_last_rec.pageno = next_page;
	    auditbuf->apb_last_rec.offset = CL_OFFSETOF(SXAP_DPG, dpg_tuples);
	}
	if (*err_code != E_SX0000_OK)
	    break;

	/*
	** The record pointer in the audit buffer is now pointing
	** at a valid audit record. Copy the record into the buffer
	** supplied by the caller and increment the record pointer.
	*/
	char_ptr = (char *) p;

	status = uncompress_record(
		(char *) char_ptr + auditbuf->apb_last_rec.offset,
		audit_rec,
		&rec_length,
		r->rs_version,
		&local_err);
	if (status != E_DB_OK)
	{
	    *err_code = local_err;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
		    NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	auditbuf->apb_last_rec.offset += rec_length;

	break;
    }

    /* Handle any errors */
    if (*err_code != E_SX0000_OK)
    {
	if (locked)
	    _VOID_ LKrelease(0, lk_ll, &lk_id, NULL, &lk_val, &cl_err);

	if (*err_code > E_SXF_INTERNAL)
	{
	    if(r && r->rs_filename)
	    {
	        _VOID_ ule_format(E_SX1036_AUDIT_FILE_ERROR, NULL, 
		    ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 1, 
		    STlength (r->rs_filename), (PTR) r->rs_filename);
	    }
	    *err_code = E_SX102B_SXAP_READ;
	}

	if (status == E_DB_OK)
	    status = E_DB_ERROR;
    }

    return (status);
}

/*
** Name: SXAP_QWRITE - write a record into an audit queue
**
** Description:
**	This routine queues an audit record for writing.
**
** Algorithm:
**	Take QUEUE lock in X mode
**	while queue is full
**	    Release QUEUE lock
**          Flush queue
**	    Take QUEUE lock in X mode
**      put record on QUEUE
**      Release QUEUE lock
**
** Inputs:
**	scb			SXF session control block.
**	rscb			Record stream control block for current
**				audit file.
**	audit_rec		The audit record to be written to the file.
**
** Outputs:
**	err_code		Error code returned to the caller.
**				Special values:
**				E_SX10AE_SXAP_AUDIT_SUSPEND - suspend audit
**				E_SX102D_SXAP_STOP_AUDIT - stop audit
**
** Returns:
**	E_DB_OK	  - succeeded
**	E_DB_WARN - some exceptional condition happened
**	other	  - error
**
** History:
**      2-apr-94 (robf)
**	      Created
**	27-apr-94 (robf)
**            Copy out full detail text buffers
**      05-dec-2008 (horda03) Bug 121332
**         Before updating the sxf_sxap hold the 
**         sxap_semaphore to ensure the sxap_curr_rscb
**         has a valid value. Prevents an ACCVIO or E_SX102C.
*/
DB_STATUS
sxap_qwrite(
    SXF_SCB		*scb,
    SXF_RSCB		*rscb,
    SXF_AUDIT_REC	*audit_rec,
    i4		*err_code)
{
    DB_STATUS		local_status = E_DB_OK;
    DB_STATUS		status = E_DB_OK;
    i4		local_err;
    bool		loop=FALSE;
    LK_LKID		lk_id;
    LK_VALUE		lk_val;
    SXAP_SHM_SEG        *shm=Sxap_cb->sxap_shm;
    SXAP_QBUF		*qbuf;
    bool		q_locked=FALSE;
    SXAP_SCB 	        *sxap_scb = (SXAP_SCB*)scb->sxf_sxap_scb;
    i4			force_flush;

    MEfill(sizeof(LK_LKID), 0, &lk_id);

    /*
    ** Degenerate to old algorithm if no shared queue
    */
    if (Sxap_cb->sxap_num_shm_qbuf<1)
    {
	status=sxap_write(scb, rscb, audit_rec, err_code);
	if( (status == E_DB_OK) && (rscb->sxf_sxap!=(PTR)Sxap_cb->sxap_curr_rscb))
        {
             /* Current rscb has changed, but protect from a
             ** NULL sxap_curr_rscb then re-check.
             */
             if ( ! (status = sxap_sem_lock(err_code) ) )
             {
                if(rscb->sxf_sxap!=(PTR)Sxap_cb->sxap_curr_rscb)
                {
    	           rscb->sxf_sxap=(PTR)Sxap_cb->sxap_curr_rscb;
                }

                status = sxap_sem_unlock(err_code);
              }

        }
	return status;
    }

    Sxf_svcb->sxf_stats->write_queue++;
    do
    {

	status=sxap_q_lock( scb, LK_X, &lk_id, &lk_val, err_code);
	if(status!=E_DB_OK)
		break;

	q_locked=TRUE;
        if(SXS_SESS_TRACE(SXS_TAUD_SHM) || debug_trace)
	{
		TRdisplay("SXAP_QWRITE: %p: start=%d, %d end=%d, %d write=%d, %d\n",
			scb,
			shm->shm_qstart.trip, shm->shm_qstart.buf,
			shm->shm_qend.trip, shm->shm_qend.buf,
			shm->shm_qwrite.trip, shm->shm_qwrite.buf);
	}
	/*
	** Loop until queue has space.
	*/
	while (shm->shm_qstart.buf == shm->shm_qend.buf &&
	       shm->shm_qstart.trip < shm->shm_qend.trip )
	{
		if(SXS_SESS_TRACE(SXS_TAUD_SHM) || debug_trace)
		{
			TRdisplay("SXAP_QWRITE: %p: queue is full, flushing\n",
					scb);
		}
		/* start the audit writer to help out */
		start_audit_writer(TRUE);

        	Sxf_svcb->sxf_stats->flush_qfull++;
    		sxap_scb->flags|= SXAP_SESS_MODIFY;
		sxap_scb->sess_lastq=shm->shm_qstart;

		if(Sxap_cb->sxap_num_shm_qbuf<10)
		{
			sxap_qloc_incr(&sxap_scb->sess_lastq);
		}
		else
		{
			i4 i;
			/*
			** Write several buffers out to make space.
			*/
			for(i=0; i<10; i++)
				sxap_qloc_incr(&sxap_scb->sess_lastq);
		}

		/*
		** flush to make space.
		*/

		status=sxap_q_unlock(scb, &lk_id, &lk_val, err_code);
		if(status!=E_DB_OK)
			break;
		q_locked=FALSE;

		/*
		** We turn on FORCE_FLUSH otherwise the Q-flush
		** may not free up any space.
		*/
		force_flush=Sxap_cb->sxap_force_flush;

		Sxap_cb->sxap_force_flush=SXAP_FF_SYSTEM;

		status=sxap_qflush(scb, err_code);

		if(force_flush!=SXAP_FF_SYSTEM)
			Sxap_cb->sxap_force_flush=force_flush;

		if(status!=E_DB_OK)
			break;
		
		status=sxap_q_lock( scb, LK_X, &lk_id, &lk_val, err_code);
		if(status!=E_DB_OK)
			break;
		q_locked=TRUE;
	}
	if(status!=E_DB_OK)
		break;

	/* At this point we have the queue locked and space on it for
	** the new record, so save it
	*/
        if(SXS_SESS_TRACE(SXS_TAUD_SHM) || debug_trace)
	{
		TRdisplay("SXAP_QWRITE: %p: Saving record in buffer %d\n",
				scb, shm->shm_qend.buf);
	}
	qbuf= &shm->shm_qbuf[shm->shm_qend.buf];
	qbuf->auditrec= *audit_rec;
	qbuf->flags=SXAP_QUSED;
	if(audit_rec->sxf_detail_txt && audit_rec->sxf_detail_len>0 &&
			audit_rec->sxf_detail_len <= SXF_DETAIL_TXT_LEN)
	{
		qbuf->flags|=SXAP_QDETAIL;
		MEcopy( (PTR)audit_rec->sxf_detail_txt,
			audit_rec->sxf_detail_len,
			(PTR)&qbuf->detail_txt);
	}
	/* Remember how far we have queued records for this session */

	sxap_scb->sess_lastq=shm->shm_qend; 
	/* Move end of the queue */
	sxap_qloc_incr(&shm->shm_qend);

	/* Mark session as having queued data */
	sxap_scb->flags|=SXAP_SESS_MODIFY;


    } while(loop);
    if(q_locked)
    {
	local_status=sxap_q_unlock(scb, &lk_id, &lk_val, &local_err);
        if(local_status!=E_DB_OK)
	{
		status=local_status;
		*err_code=local_err;
	}
    }
    /*
    ** If current write rscb changed under us make the rscb here keep in
    ** sync. 
    */
    if( (status == E_DB_OK) && rscb->sxf_sxap!=(PTR)Sxap_cb->sxap_curr_rscb)
    {
        /* Current rscb has changed, but protect from a
        ** NULL sxap_curr_rscb then re-check.
        */

        if ( ! (status = sxap_sem_lock(err_code) ) )
        {
           if(rscb->sxf_sxap!=(PTR)Sxap_cb->sxap_curr_rscb)
           {
              rscb->sxf_sxap=(PTR)Sxap_cb->sxap_curr_rscb;
           }

           status = sxap_sem_unlock(err_code);
        }
    }

    /* 
    ** start the audit writer to help write the record
    */
    start_audit_writer(FALSE);

    if(status!=E_DB_OK && status!=E_DB_WARN)
    {
	if (*err_code > E_SXF_INTERNAL)
	{
	    _VOID_ ule_format(*err_code, NULL,  ULE_LOG, 
		NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code=E_SX10C0_SXAP_QWRITE_ERR;
	}
    }
    return status;
}
	    
/*
** Name: SXAP_WRITE - write a record to an audit file
**
** Description:
**	This routine is used to write an audit record to an audit file.
**	Audit records written by this routine will be written into a buffer, 
**	a subsequent call to sxap_flush will ensure they are flushed to disk.
**	
** Inputs:
**      scb    		- SXF scb
**	rscb   		- Write file RSCB
** 	audit_rec 	- Audit record being written
**
** Returns:
**	E_DB_OK    - Success
**
**	E_DB_ERROR - Failure
**
** 	E_DB_WARN  - Auditing should be stopped/suspended
**
** History:
**	2-apr-94(robf)
**	    Initial creation.
*/
DB_STATUS
sxap_write(
    SXF_SCB		*scb,
    SXF_RSCB		*rscb,
    SXF_AUDIT_REC	*audit_rec,
    i4		*err_code)
{
    DB_STATUS 	status=E_DB_OK;
    LK_LKID	lk_id;
    LK_VALUE	lk_val;
    bool	locked=FALSE;

    MEfill(sizeof(LK_LKID), 0, &lk_id);

    for(;;)
    {
	/*
	** To prevent RSCB changing under us take the ACCESS lock
	** first
	*/
	status=sxap_ac_lock(scb, LK_X, &lk_id, &lk_val, err_code);
	if(status!=E_DB_OK)
		break;
	locked=TRUE;
	status= write_shared_page(scb, (SXAP_RSCB*)rscb->sxf_sxap,
					audit_rec, &lk_id, &lk_val, 
					NULL, err_code);
	if(status!=E_DB_OK)
		break;
	status=sxap_ac_unlock(scb, &lk_id, &lk_val, err_code);
	if(status!=E_DB_OK)
		break;
	locked=FALSE;
	break;
    }
    if(locked)
    {
	(VOID)sxap_ac_unlock(scb, &lk_id, &lk_val, err_code);
    }
    return status;
}

/*
** Name: write_shared_page - write a shared buffer page to an audit file
**
** Description:
**	This routine is used to write an audit record to an audit file.
**	Audit records written by this routine will be written into a buffer, 
**	a subsequent call to sxap_flush will ensure they are flushed to disk.
**
** 	This should be called only when the access lock is taken.
**
** 	Overview of algorithm:-
**
**	Validate the parameters passed to the routine.
**	Get the audit access lock.
**	Call sxap_chk_cnf
**	If (no room on page)
**	    Write current page to disk
**	    Attempt to allocate a new page
**	    If (file size >= max file size)
**		Call sxap_change_file()
**	Copy record into audit buffer
**	Relese audit access lock
**
** Inputs:
**      scb    		- SXF scb
**	rscb   		- SXAP Write RSCB
** 	audit_rec 	- Audit record being written
**	lk_id/lk_val    - Pointers to ACCESS lock values, may be altered by
**		          this routine.
**	page_written	- Set to TRUE if we forced the page to disk.
**
** Returns:
**	E_DB_OK    - Success
**
**	E_DB_ERROR - Failure
**
** 	E_DB_WARN  - Auditing should be stopped/suspended
**
** History:
**      20-sep-1992 (markg)
**	    Initial creation.
**	09-dec-1992 (robf)
**	    Add handling checking/switching of log files.
**	14-mar-1993 (markg)
**	    Fixed bug which caused an AV if this routine was called
**	    when auditing had been disabled.
**	3-aug-93 (stephenb)
**	    Fixed bug where routine would return after recieving 
**	    E_SX102D_SXAP_STOP_AUDIT without releasing lock.
**	3-aug-93 (robf)
**	    Updated handling of LOGFULL operation:
**          - STOP/SUSPEND audit condition is passed back to the logical
**	      layer to implement.
**	    - Check added to not switch logs if the current session has
**	      MAINTAIN_AUDIT privilege and switching  would cause a log
**	      suspend.
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
**	13-jan-94 (stephenb)
**	    Integrate (robf) fixes:
**          - Get the audit access lock before checking args, since another
**          session could be updating the rscb info under the access lock.
**          - Only report audit file if this is known (prevents AV)
**          - If r is empty due to auditing being stopped then quietly
**          return. This allows for the case where another session stopped
**          auditing between the SXA layer check and getting here.
**          - Make lock request non-interruptable to stop users interrupting
**            the request and possibly causing auditing to fail
**	30-mar-94 (robf)
**          Rework to add support for shared queue.
**	    Seperated out from sxap_write, added lk_id/val parameters.
**	10-may-94 (robf)
**          Changing logs may invalidate the rscb, so check and reassign
**	    the current rscb after checking for log change.
**	27-may-94 (robf)
**          Add same change as 10-may-94 in other places where log changes.
**	8-jul-94 (robf/stephenb)
**          When returning audit disabled turn off the active flag,
**          this prevents later problems during stop audit processing
**          when the current audit file is already closed.
**	29-Aug-2008 (jonj)
**	    Take sxap_semaphore before fetching sxap_curr_file,
**	    check for NULL.
*/
static DB_STATUS
write_shared_page(
    SXF_SCB		*scb,
    SXAP_RSCB		*rscb,
    SXF_AUDIT_REC	*audit_rec,
    LK_LKID		*lk_id,
    LK_VALUE		*lk_val,
    bool		*page_written,
    i4		*err_code)
{
    DB_STATUS		status = E_DB_OK;
    i4		local_err;
    STATUS		cl_status;
    CL_ERR_DESC		cl_err;
    LK_LOCK_KEY		lk_key = {LK_AUDIT, SXAP_LOCK, SXAP_ACCESS, 0, 0, 0, 0};
    LK_LLID		lk_ll = scb->sxf_lock_list;
    SXAP_SCB	        *sxap_scb=(SXAP_SCB*)scb->sxf_sxap_scb;
    SXAP_RECID		*scb_last_rec = &sxap_scb->sess_lastrec;
    SXAP_RSCB		*r;
    SXAP_APB		*auditbuf;
    SXAP_DPG		*p;
    char		*char_ptr;
    i4		pageno;
    i4			length;
    i4			retry;
    bool		locked = FALSE;
    SXAP_STAMP		stamp;
    i4			lock_retry=0;
    bool		log_change=FALSE;

    *err_code = E_SX0000_OK;

    for (;;)
    {
        /*
        **	Logical (buffered) write
        */
        Sxf_svcb->sxf_stats->write_buff++;

	if(page_written)
		*page_written=FALSE;
	r =  rscb;
	if(r==NULL)
	{
	    if(!(Sxap_cb->sxap_status & SXAP_ACTIVE))
	    {
		/*
		** Security auditing stopped so quietly 
		** stop.
		*/
		break;
	    }
	    *err_code = E_SX102C_BAD_FILE_CONTEXT;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	auditbuf = (SXAP_APB *) r->rs_auditbuf;
	p = (SXAP_DPG *) &auditbuf->apb_auditpage;

	/*
	** Check that the RSCB we have been given is for the
	** current audit file.
	*/
	if (r != Sxap_cb->sxap_curr_rscb || r->rs_state != SXAP_RS_OPEN ||
			r->rs_pgsize==0)
	{
	    *err_code = E_SX102C_BAD_FILE_CONTEXT;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	/*
	**  Check if configuration changed. This may also
	**  change the "current" log file if appropriate.
	*/
	status = sxap_chk_cnf( lk_val, &local_err);
	if (status != E_DB_OK)
	{
	    *err_code = local_err;
	    if (local_err == E_SX102D_SXAP_STOP_AUDIT ||
		local_err ==  E_SX10AE_SXAP_AUDIT_SUSPEND)
	    {
		return (E_DB_WARN);
	    }
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	/*
	** Current rscb may have changed due to file switch, so
	** check to make sure
	*/
	if (r != Sxap_cb->sxap_curr_rscb )
	{
	    /* Watch for NULL sxap_curr_rscb */
            if ( status = sxap_sem_lock(err_code) )
                break;
	    r=Sxap_cb->sxap_curr_rscb;
            if ( status = sxap_sem_unlock(err_code) )
                break;

	    if ( !r || r->rs_state != SXAP_RS_OPEN  )
	    {
	        *err_code = E_SX102C_BAD_FILE_CONTEXT;
	        _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	        break;
	    }
	    auditbuf = (SXAP_APB *) r->rs_auditbuf;
	    p = (SXAP_DPG *) &auditbuf->apb_auditpage;
	}

	for (retry = 0; retry < 2; retry++)
	{
            char_ptr = (char *) p;
	    length = r->rs_pgsize - p->dpg_header.pg_next_byte;

	    /* 
	    ** Compress the audit record and attempt to write the output
	    ** to the current buffered audit page. If there's not enough 
	    ** room on the current page, get a new page and retry.
	    */
	    status = compress_record(
		audit_rec,
		(char *) char_ptr + p->dpg_header.pg_next_byte,
		&length,
		&local_err);
	    if (status == E_DB_OK)
	    {
		/*
		** Update the audit buffer and the SCB to reflect the
		** successful write of the audit record.
		*/
		p->dpg_header.pg_next_byte += length;
		auditbuf->apb_status |= SXAP_MODIFY;
		scb_last_rec->pageno = p->dpg_header.pg_no;
		scb_last_rec->offset = p->dpg_header.pg_next_byte;
		break;
	    }
	    else
	    {
		if (local_err != E_SX1066_BUF_TOO_SHORT || retry != 0)
		{
		    *err_code = local_err;
	    	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			    NULL, NULL, 0L, NULL, &local_err, 0);
		    break;
		}
	    }

	    /* 
	    ** If we get here then there's no room on the current 
	    ** page - write this page out, then go and allocate 
	    ** a new one.
	    */
	    Sxf_svcb->sxf_stats->write_full++; /* Write-full */

	    status = sxap_flushbuff(&local_err);
	    if (status != E_DB_OK)
	    {
		*err_code = local_err;
		if (*err_code > E_SXF_INTERNAL)
	    	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			    NULL, NULL, 0L, NULL, &local_err, 0);
		break;
	    }
	    if(page_written)	
		*page_written=TRUE;

	    status = sxap_alloc_page(&r->rs_di_io, 
			(PTR)p, &pageno, r->rs_pgsize, &local_err);
	    if (status != E_DB_OK)
	    {
		*err_code = local_err;
		if (*err_code > E_SXF_INTERNAL)
	    	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			    NULL, NULL, 0L, NULL, &local_err, 0);
		break;
	    }

	    /* 
	    ** Check to see if the allocation of the new page
	    ** has made us exceed the maximum specified size 
	    ** for an audit file. If it has we need to change 
	    ** files.
	    */
	    if ((pageno + 1) * rscb->rs_pgsize / 1024 > 
		Sxap_cb->sxap_max_file &&
		Sxap_cb->sxap_max_file != 0L)
	    {
		/*
		** The following allows a SUSPEND to be processed at the 
		** logical level. We leave  the physical file  open so another
		** session with MAINTAIN_AUDIT privilege can continue to
		** write records while suspended.
		*/
		log_change=TRUE;

		if(Sxap_cb->sxap_act_on_full==SXAP_SUSPEND)
		{
			/*
			** Check if any more logs available
			*/
			status = sxap_find_curfile(SXAP_FINDCUR_NEXT, 
				NULL, NULL, NULL, &local_err);
			if(status==E_DB_WARN)
			{
	    		    if(!(scb->sxf_ustat & DU_UALTER_AUDIT))
			    {
				/*
				** No more logs, no special privs, so stay 
				** with the same log, and indicate to caller 
				** logical suspend occurs.
				*/
				_VOID_ ule_format(local_err, NULL, ULE_LOG, 
					NULL, NULL, 0L, NULL, &local_err, 0);
			        *err_code = E_SX10AE_SXAP_AUDIT_SUSPEND;
				return E_DB_WARN;
			    }
			    else
			    {
				/* 
				** MAINTAIN_AUDIT user continue using same
				** audit log
				*/
				log_change=FALSE;
			    }
			}
		}
	    }
	    if(log_change)
	    {
		/*
		** Need to change logs
		*/
		status = sxap_change_file(SXAP_CHANGE_NEXT, 
					&stamp, NULL, &local_err);
		if (status != E_DB_OK)
		{
		    /*
		    **	If auditing stops break
		    */
		    *err_code = local_err;
		    if (local_err == E_SX102D_SXAP_STOP_AUDIT ||
		        local_err ==  E_SX10AE_SXAP_AUDIT_SUSPEND)
		    {
			/* 
			** Mark auditing as inactive on STOP since no 
			** current audit file. 
			*/
			if(local_err == E_SX102D_SXAP_STOP_AUDIT)
			{
				Sxap_cb->sxap_status &= ~SXAP_ACTIVE;
			}
			return (E_DB_WARN);
		    }
		    if (*err_code > E_SXF_INTERNAL)
	    	        _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			        NULL, NULL, 0L, NULL, &local_err, 0);
		    break;
		}
	        /*
	        **	Convert the new log stamp to a lock value.
	        **	This will cause the lock stamp to be updated when
	        **	we release the lock below. 
	        */
	        _VOID_ sxap_stamp_to_lockval(&stamp, lk_val);
	        /*
	        ** Update the shared buffer manager stamp value. At this
	        ** point the node will be sync. Note we don't do this
	        ** in sxap_change_file since here we can guarantee we
	        ** have the access lock.
	        */
	        STRUCT_ASSIGN_MACRO(stamp,Sxap_cb->sxap_shm->shm_stamp);
		/*
		** Current rscb may have changed due to file switch, so
		** check to make sure
		*/
		if (r != Sxap_cb->sxap_curr_rscb )
		{
		    /* Watch for NULL sxap_curr_rscb */
                    if ( status = sxap_sem_lock(err_code) )
                       break;
		    r=Sxap_cb->sxap_curr_rscb;
                    if ( status = sxap_sem_unlock(err_code) )
                       break;
		    
		    if ( !r || r->rs_state != SXAP_RS_OPEN  )
		    {
			*err_code = E_SX102C_BAD_FILE_CONTEXT;
			_VOID_ ule_format(*err_code, NULL, ULE_LOG, 
				NULL, NULL, 0L, NULL, &local_err, 0);
			break;
		    }
		    auditbuf = (SXAP_APB *) r->rs_auditbuf;
		    p = (SXAP_DPG *) &auditbuf->apb_auditpage;
		}
	    } /* if log_change */
	} /* retry */
	if (*err_code != E_SX0000_OK)
	    break;

	break;
    }

    /* Handle any errors */
    if (*err_code != E_SX0000_OK)
    {

	if (*err_code > E_SXF_INTERNAL)
	{
	    if (r && r->rs_filename)
	    {
	    	_VOID_ ule_format(E_SX1036_AUDIT_FILE_ERROR, NULL, 
		    ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 1, 
		    STlength (r->rs_filename), (PTR) r->rs_filename);
	    }
	    *err_code = E_SX102E_SXAP_WRITE;
	}

	/* 
	** Any failure to write an audit record is 
	** considered a severe error.
	*/
	status = E_DB_SEVERE;
    }

    return (status);
}

/*
** Name: SXAP_FLUSH - flush all audit records for this session to disk
**
** Description:
**	This routine is used to flush any buffered audit records previously
**	written by this session to disk.
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
**      20-sep-1992 (markg)
**	    Initial creation.
**	14-mar-1993 (markg)
**	    Fixed bug which caused an AV if this routine was called
**	    when auditing had been disabled.
**    27-oct-93 (stephenb)
**          Sometimes the "for" loop breaks on error without releasing the lock,
**          check at the end of the routine for lock held and release if
**          necesary.
**	13-jan-94 (stephenb)
**	    Integrate (robf) fixes:
**	12-jan-94 (robf)
**          Reorder code to avoid failures during state transitions,
**	    and check if auditing is disabled or not. (we need this
**	    here since state may have changed in SXAP after  the check in
**	    SXA)
**	    Make lock request non-interruptable to stop users interrupting
**          the request and possibly causing auditing to fail
**	2-apr-94 (robf)
**          Made wrapper for flush_shared_page for compatibility
*/
DB_STATUS
sxap_flush(
    SXF_SCB		*scb,
    i4		*err_code)
{

    return flush_shared_page(scb, TRUE, FALSE, err_code);
}

/*
** Name: flush_shared_page - flush audit records for this
**	 session from the shared audit page to disk.
**
** Description:
**	This routine is used to flush any buffered audit records previously
**	written by this session to disk.
**
** 	Overview of algorithm:-
**
**	If (last record written to disk is prior to 
**	   last record written by session)
**		Get the audit access lock
**		Call sxap_write_page()
**		Release the audit access lock.
**
** Inputs:
**	scb			SXF session control block.
**
**	lock_access	        if TRUE take the access lock if necessary
**	                        if FALSE caller has already taken lock
**
**	force_flush		if TRUE forces a flush, 
**				if FALSE only flushes if thinks data is written
**
** Outputs:
**	err_code		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**      20-sep-1992 (markg)
**	    Initial creation.
**	14-mar-1993 (markg)
**	    Fixed bug which caused an AV if this routine was called
**	    when auditing had been disabled.
**    27-oct-93 (stephenb)
**          Sometimes the "for" loop breaks on error without releasing the lock,
**          check at the end of the routine for lock held and release if
**          necesary.
**	13-jan-94 (stephenb)
**	    Integrate (robf) fixes:
**	12-jan-94 (robf)
**          Reorder code to avoid failures during state transitions,
**	    and check if auditing is disabled or not. (we need this
**	    here since state may have changed in SXAP after  the check in
**	    SXA)
**	    Make lock request non-interruptable to stop users interrupting
**          the request and possibly causing auditing to fail
**	2-apr-94 (robf)
**         Renamed from sxap_flush now this is a lower level call,
**         added lock_access parameter to let caller previously take the
**         access lock
**	29-Aug-2008 (jonj)
**	    Take sxap_semaphore before fetching sxap_curr_file,
**	    check for NULL.
*/
static DB_STATUS
flush_shared_page (
	SXF_SCB 	*scb,
	bool		lock_access,
	bool		force_flush,
	i4		*err_code
)
{
    DB_STATUS		status = E_DB_OK;
    i4		local_err;
    STATUS		cl_status;
    CL_ERR_DESC		cl_err;
    LK_LKID		lk_id;
    LK_VALUE		lk_val;
    LK_LOCK_KEY		lk_key = {LK_AUDIT, SXAP_LOCK, SXAP_ACCESS, 0, 0, 0, 0};
    LK_LLID		lk_ll = scb->sxf_lock_list;
    SXAP_SCB		*sxap_scb = (SXAP_SCB *) scb->sxf_sxap_scb;
    SXAP_RECID		*scb_last_rec =  &sxap_scb->sess_lastrec;
    SXAP_RSCB		*r;
    SXAP_APB		*auditbuf;
    SXAP_DPG		*p;
    bool		locked = FALSE;
    bool		got_semaphore = FALSE;
    i4			retry;

    MEfill(sizeof(LK_LKID), 0, &lk_id);
    *err_code = E_SX0000_OK;

    for (;;)
    {
	/*
	** Get access to the SXA semaphore before
	** accessing the sxap_cb, in case it changes underneath us.
	*/
  	Sxf_svcb->sxf_stats->flush_count++;
  
        if (scb_last_rec->pageno == 0 &&
  	    scb_last_rec->offset == 0 &&
	    !force_flush)
  	{
  	    /*
  	    **	Nothing to flush, so break
  	    */
  	    Sxf_svcb->sxf_stats->flush_empty++;
  	    break;
  	}
  
        if ( status = sxap_sem_lock(err_code) )
            break;
	got_semaphore=TRUE;

	if(!(Sxap_cb->sxap_status & SXAP_ACTIVE))
	{
		/*
		** If auditing not active then no need to flush.
		*/
		scb_last_rec->pageno = 0;
		scb_last_rec->offset = 0;
		break;
	}

	r = Sxap_cb->sxap_curr_rscb;

	if (!r || r->rs_state!=SXAP_RS_OPEN)
	{
	    /*
	    ** Invalid CB
	    */
	    *err_code = E_SX102C_BAD_FILE_CONTEXT;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	auditbuf = (SXAP_APB *) r->rs_auditbuf;
	p = (SXAP_DPG *) &auditbuf->apb_auditpage;

	/*
	** If the last record written by this session has not yet
	** been written to the audit file, we write out the whole
	** current page of the file.
	*/
	if ((scb_last_rec->pageno > auditbuf->apb_last_rec.pageno) ||
	    (scb_last_rec->pageno == auditbuf->apb_last_rec.pageno  &&
	    scb_last_rec->offset >  auditbuf->apb_last_rec.offset) ||
	    force_flush)
	{
	    if(lock_access)
	    {
		/* Release sem while waiting for lock */
                if ( status = sxap_sem_unlock(err_code) )
                    break;
		got_semaphore=FALSE;
	   	status=sxap_ac_lock(scb, LK_X, &lk_id, &lk_val, err_code);
	   	if(status!=E_DB_OK)
	   		break;
	        locked = TRUE;
		/* Retake sem */
                if ( status = sxap_sem_lock(err_code) )
                    break;
		got_semaphore=TRUE;
	    }
	    /*
	    ** Check if file closed or auditing stopped
	    ** while we were waiting.
	    */
	    r = Sxap_cb->sxap_curr_rscb;

	    if(!r || r->rs_state==SXAP_RS_CLOSE ||
		!(Sxap_cb->sxap_status & SXAP_ACTIVE))
	    {
		/*
		** If auditing not active, nothing to do
		*/
		scb_last_rec->pageno = 0;
		scb_last_rec->offset = 0;
		break;
	    }

	    /*
	    ** Release semaphore prior to writing the page.
	    */
	    if(got_semaphore)
	    {
                if ( status = sxap_sem_unlock(err_code) )
                    break;
		got_semaphore=FALSE;
	    }

	    status = sxap_write_page(&r->rs_di_io, 
			 p->dpg_header.pg_no, (PTR)p, TRUE, &local_err);
	    if (status != E_DB_OK)
	    {
		*err_code = local_err;
		if (*err_code > E_SXF_INTERNAL)
	            _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
		break;
	    }
	    auditbuf->apb_last_rec.pageno = p->dpg_header.pg_no;
	    auditbuf->apb_last_rec.offset = p->dpg_header.pg_next_byte;
	    auditbuf->apb_status &= ~(SXAP_MODIFY);

	    if(lock_access)
	    {
	        status=sxap_ac_unlock(scb, &lk_id, &lk_val, err_code);
	        if(status!=E_DB_OK)
			break;
		locked=FALSE;
	    }
	}
	else
	{
	    /*
	    ** Already flushed - done by some other session
	    */
	    Sxf_svcb->sxf_stats->flush_predone++;
	}
	scb_last_rec->pageno = 0;
	scb_last_rec->offset = 0;

	break;
    }
    /*
    ** Make sure we have released the lock.
    */
    if (locked)
    {
	cl_status = LKrelease(0, lk_ll, &lk_id, NULL, &lk_val, &cl_err);
	if (cl_status != OK)
	{
	    *err_code = E_SX1008_BAD_LOCK_RELEASE;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
		NULL, NULL, 0L, NULL, &local_err, 0);
	}
    }

    if (got_semaphore)
    {
       cl_status =  sxap_sem_unlock(&local_err);              
                                                             
       if ( (status == E_DB_OK) &&                             
            (cl_status != E_DB_OK) )                          
       {                                                       
          status = cl_status;                                 
          *err_code = local_err;                               
       }                                                       
    }

    /* Handle any errors */
    if (*err_code != E_SX0000_OK)
    {
	if (*err_code > E_SXF_INTERNAL)
	{
	    if(r && r->rs_filename)
	    {
	        _VOID_ ule_format(E_SX1036_AUDIT_FILE_ERROR, NULL, 
		    ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 1, 
		    STlength (r->rs_filename), (PTR) r->rs_filename);
	    }
	    *err_code = E_SX102F_SXAP_FLUSH;
	}

	if (status == E_DB_OK)
	    status = E_DB_ERROR;
    }

    return (status);
}
	
/*
** Name: SXAP_WRITE_PAGE - write an audit file page
**
** Description:
**	This routine is used to write a buffered audit file page to an
**	audit file. The page is written to the file and then flushed to
**	disk.
**
** 	Overview of algorithm:
**
**	Calculate the checksum for the page.
**	Call DIwrite to write the page.
**	Call DIforce to force the page to disk.
**
** Inputs:
**	di_io			DI context of audit file.
**	pageno			Page number to write.
**	page_buf		Pointer to audit page to write
**	force			if TRUE force the page.
**
** Outputs:
**	err_code		Error code.
**
** Returns:
**	DB_STATUS.
**
** History:
**      20-sep-1992 (markg)
**	    Initial creation.
**	26-oct-93 (robf)
**          Add trace output when DIwrite fails.
**	15-apr-94 (robf)
**          Add force flag to indicate whether or no
**	    to force the page.
*/
DB_STATUS
sxap_write_page(
    DI_IO		*di_io,
    i4		pageno,
    PTR			page_buf,
    bool		force,
    i4		*err_code)
{
    DB_STATUS		status = E_DB_OK;
    i4		local_err;
    STATUS		cl_status;
    CL_ERR_DESC		cl_err;
    i4			write_pages;
    SXAP_PGHDR		*pghdr = (SXAP_PGHDR *) page_buf;

    *err_code = E_SX0000_OK;
    /*
    **	Physical (direct) write
    */
    Sxf_svcb->sxf_stats->write_direct++;

    for(;;)
    {
	if (pghdr->pg_no != (SXAP_PGNO) pageno)
	{
	    *err_code = E_SX1029_INVALID_PAGENO;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	pghdr->pg_chksum = check_sum(page_buf, Sxap_cb->sxap_page_size);

	write_pages = 1;
	cl_status = DIwrite(di_io, &write_pages, pageno, 
			   (char *) page_buf, &cl_err);
	if (cl_status != OK)
	{
	     /* Log CL error status */
	    _VOID_ ule_format(cl_status, &cl_err, ULE_LOG, 
		NULL, NULL, 0L, NULL, &local_err, 0);

	    *err_code = E_SX1018_FILE_ACCESS_ERROR;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
		NULL, NULL, 0L, NULL, &local_err, 0);

	    TRdisplay("SXAP_WRITE_PAGE, DIwrite() failed:\n");
	    TRdisplay("cl_status=%d, di_io=%x, pageno=%d, page_buf=%x\n",
			cl_status,
			di_io,
			pageno,
			page_buf);
	    break;
	}

	/*
	** Only force if necessary
	*/
        if(force)
	{
	    cl_status = DIforce(di_io, &cl_err);
	    if (cl_status != OK)
	    {
	        *err_code = E_SX1018_FILE_ACCESS_ERROR;
	        _VOID_ ule_format(*err_code, &cl_err, ULE_LOG, 
		    NULL, NULL, 0L, NULL, &local_err, 0);
	        break;
	    }
	}

	break;
    }

    /* Handle any errors */
    if (*err_code != E_SX0000_OK)
    {
	if (*err_code > E_SXF_INTERNAL)
	    *err_code = E_SX1030_SXAP_WRITE_PAGE;

	status = E_DB_ERROR;
    }

    return (status);
}

/*
** Name: SXAP_READ_PAGE - read an audit file page
**
** Description:
**	The routine will read a given page from an audit file, and return
**	the page contents to the caller.
**
**	Overview of algorithm:-
**
**	Call DIread to read the specified page
**	Checksum validate the read page
**
** Inputs:
**	di_io			DI context structure of audit file
**	pageno			page number to read
**	page_buf		Where to put the read page.
**	page_size		Page size, in bytes.
**
** Outputs:
**	page_buf		Buffer containing read page.
**	err_code		Error code.
**
** Returns:
**	DB_STATUS
**
** History:
**      20-sep-1992 (markg)
**	    Initial creation.
**	15-may-94 (robf)
**          Add page_size parameter, pass onto check_sum()
*/
DB_STATUS
sxap_read_page(
    DI_IO		*di_io,
    i4		pageno,
    PTR			page_buf,
    i4		page_size,
    i4		*err_code)
{
    DB_STATUS		status = E_DB_OK;
    i4		local_err;
    STATUS		cl_status;
    CL_ERR_DESC		cl_err;
    i4			read_pages;
    SXAP_PGHDR		*pghdr = (SXAP_PGHDR *) page_buf;

    *err_code = E_DB_OK;

    /*
    **	Physical (direct) read
    */
    Sxf_svcb->sxf_stats->read_direct++;

    for (;;)
    {
	read_pages = 1;
	cl_status = DIread(di_io, &read_pages, pageno, 
			  (char *) page_buf, &cl_err);
	if (cl_status != OK)
	{
	    *err_code = E_SX1018_FILE_ACCESS_ERROR;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG, 
		NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	if (pghdr->pg_chksum != check_sum(page_buf, page_size))
	{
	    *err_code = E_SX1031_BAD_CHECKSUM;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	break;
    }

    /* Handle any errors */
    if (*err_code != E_SX0000_OK)
    {
	if (*err_code > E_SXF_INTERNAL)
	    *err_code = E_SX1032_SXAP_READ_PAGE;

	status = E_DB_ERROR;
    }

    return (status);
}

/*
** Name: SXAP_ALLOC_PAGE - allocate and initialize an audit page
**
** Description:
**	This routine allocates and initializes a data page in an audit file. 
**	The next free page is allocated from the file and initialized. The 
**	initialized page is written back to the file before the file header 
**	is flushed. 
**
**	This routine will only allocate data pages in the audit file. Callers
**	who wish to allocate header pages may use this routine, however they 
**	must change the allocated data page to a header page and write it
**	back to disk themselves. In the event of a failure during this
**	operation, it is the callers responsibility to perform any necessary
**	cleanup.
**
**	Overview of algorithm:-
**
**	Call DIalloc to allocate page in audit file
**	Initialize values on page
**	Call sxap_write_page to write page to disk
**	Call DIflush to flush the file header to disk
**
** Inputs:
**	di_io			The DI context structure for the audit file
**	page_buf		An audit file page buffer
**
** Outputs:
**	page_buf		The initialized page in the audit file
**	pageno			The page number of the allocated page
**	page_size		Size of page, bytes
**	err_code		Error code.
**
** Returns:
**	DB_STATUS
**
** History:
**      20-sep-1992 (markg)
**	    Initial creation.
**	15-apr-94 (robf)
**	    Pass page_size parameter.
**	10-may-94 (robf)
**          Log DIflush error as Ingres error.
*/
DB_STATUS
sxap_alloc_page(
    DI_IO		*di_io,
    PTR			page_buf,
    i4		*pageno,
    i4		page_size,
    i4		*err_code)
{
    DB_STATUS		status = E_DB_OK;
    i4		local_err;
    STATUS		cl_status;
    CL_ERR_DESC		cl_err;
    SXAP_PGHDR		*pghdr = (SXAP_PGHDR *) page_buf;
    SXAP_SHM_SEG        *shm=Sxap_cb->sxap_shm;

    *err_code = E_SX0000_OK;

    for (;;)
    {
        Sxf_svcb->sxf_stats->extend_count++;


	cl_status = DIalloc(di_io, 1, pageno, &cl_err);
	if (cl_status != OK)
	{
	    _VOID_ ule_format(cl_status, &cl_err, ULE_LOG, 
		NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code = E_SX1018_FILE_ACCESS_ERROR;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
		NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	MEfill(page_size , '\0', page_buf);
	/*
	** Initialize the page and write it to disk before 
	** flushing the file header.
	*/
	pghdr->pg_no = (SXAP_PGNO) *pageno;
	pghdr->pg_type = SXAP_DATA;
	pghdr->pg_next_byte = 
		(SXAP_OFFSET) CL_OFFSETOF(SXAP_DPG, dpg_tuples);

	status = sxap_write_page(di_io, *pageno, 
			page_buf, FALSE, &local_err);
	if (status != E_DB_OK)
	{
	    	*err_code = local_err;
	    	if (*err_code > E_SXF_INTERNAL)
	        	_VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    	break;
	}

        if(status!=E_DB_OK)
		break;

	cl_status = DIflush(di_io, &cl_err);
	if (cl_status != OK)
	{
	    _VOID_ ule_format(cl_status, &cl_err, ULE_LOG, 
		NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code = E_SX1018_FILE_ACCESS_ERROR;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
		NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	/* First page */
        pghdr->pg_no = (SXAP_PGNO) *pageno;

	break;
    }

    /* Handle any errors */
    if (*err_code != E_SX0000_OK)
    {
	if (*err_code > E_SXF_INTERNAL)
	    *err_code = E_SX1033_SXAP_ALLOC_PAGE;

	if (status == E_DB_OK)
	    status = E_DB_ERROR;
    }

    return (status);
}

/*
** Name: CHECK_SUM - calculate the checksum of a page
**
** Description:
**	This routine is used to calculate the value of the checksum of an
**	audit page. The checksum is defined as being the exclusive or of
**	all i4 on the audit page with the page checksum field set to zero.
**
** Inputs:
**	page			pointer to the audit page to checksum;
**	page_size		page size in bytes
**
** Outputs:
**	None.
**
** Returns:
**	The checksum of the page.
**
** History:
**      20-sep-1992 (markg)
**	    Initial creation.
**	15-may-94 (robf)
**	    Added page_size parameter.
*/
static i4
check_sum(
    PTR		page,
    u_i4	page_size)
{
    i4		*p = (i4 *) page;
    i4	i;
    i4		sum = 0;

    for (i = 0; i < page_size / (sizeof(i4) * 16); p += 16, i++)
	sum ^= p[0] ^ p[1] ^ p[2] ^ p[3] ^ p[4] ^ p[5] ^ p[6] ^ p[7] ^
	       p[8] ^ p[9] ^ p[10] ^ p[11] ^ p[12] ^ p[13] ^ p[14] ^ p[15];

    /*
    ** Ignore the value of the checksum field in the page.
    */
    sum ^= ((SXAP_PGHDR *) page)->pg_chksum;

    return (sum);
}

/*
** Name: compress_record - compress an audit record
**
** Description:
**	This routine compresses an audit record by removing trailing spaces
**	from character fields. The resulting record consists of a two byte
**	length indicator as the first element of the record, followed by the
**	compressed record. Only fields that contain character strings are
**	compressed. Each compressed field contains a one byte field length
**	indicator as the first byte of the field, followed by the character
**	string with any trailing spaces removed.
**
** Inputs:
**	rec		the audit record to be compressed.
**	buf		the output buffer to use.
**	length		the lenght of the above buffer.
**
** Outputs:
**	length		the length of the compressed record.
**	err_code	error code.
**
** Returns
**	DB_STATUS.
**
** History:
**	1-Dec-1992 (markg)
**	    Initial creation.
**	11-jan-1993 (robf)
**          Add handling of label and detail fields.
**	17-jun-1993 (robf)
**	    Increase length indicator of detail text to 2 bytes.
**	    Add security label handling
**	    Allow for file version
**	10-mar-94 (robf)
**          Write security label length as an unsigned value, since
**	    on systems with long labels (like HP) this may overflow 
**	    a char.
*/
static DB_STATUS
compress_record(
    SXF_AUDIT_REC	*rec,
    char		*buf,
    i4			*length,
    i4		*err_code)
{

    DB_STATUS		status = E_DB_OK;
    i4			l;
    u_char		*p, *orig;
    u_i2		tmp_i2;

    orig = (u_char*)buf;
    p = orig;

    for (;;)
    {
	*err_code = E_SX1066_BUF_TOO_SHORT;

	/*
	** The first two bytes of the output record show the total
	** length of the compressed record. Initialize them to zero 
	** for now, we'll fill them in once the compression is complete.
	*/
	if (p - orig + sizeof(u_i2) > *length)
	    break;
	tmp_i2 = 0;
	MECOPY_CONST_MACRO(
	    (PTR) &tmp_i2,
	    sizeof (u_i2),
	    (PTR) p);
	p += sizeof(u_i2);

	/* now copy each of the fields into the output buffer */

	if (p - orig + sizeof(SXF_EVENT) > *length)
	    break;
	MECOPY_CONST_MACRO(
		(PTR)&rec->sxf_eventtype, 
		sizeof(SXF_EVENT), 
		(PTR)p);
	p += sizeof(SXF_EVENT);
	
	if (p - orig + sizeof(SYSTIME) > *length)
	    break;
	MECOPY_CONST_MACRO(
		(PTR)&rec->sxf_tm, 
		sizeof(SYSTIME), 
		(PTR)p);
	p += sizeof(SYSTIME);

	l = comp_field_len(rec->sxf_ruserid.db_own_name, DB_OWN_MAXNAME);
	if (p - orig + l + 1 > *length)
	    break;
	*(p++) = (char)l;
	MEcopy((PTR)rec->sxf_ruserid.db_own_name, l, (PTR)p);
	p += l;

	l = comp_field_len(rec->sxf_euserid.db_own_name, DB_OWN_MAXNAME);
	if (p - orig + l + 1> *length)
	    break;
	*(p++) = (char)l;
	MEcopy((PTR)rec->sxf_euserid.db_own_name, l, (PTR)p);
	p += l;

	l = comp_field_len(rec->sxf_dbname.db_db_name, DB_DB_MAXNAME);
	if (p - orig + l + 1> *length)
	    break;
	*(p++) = (char)l;
	MEcopy((PTR)rec->sxf_dbname.db_db_name, l, (PTR)p);
	p += l;

	if (p - orig + sizeof(i4) > *length)
	    break;
	MECOPY_CONST_MACRO(
		(PTR)&rec->sxf_messid, 
		sizeof(i4), 
		(PTR)p);
	p += sizeof(i4);

	if (p - orig + sizeof(STATUS) > *length)
	    break;
	MECOPY_CONST_MACRO(
		(PTR)&rec->sxf_auditstatus, 
		sizeof(STATUS), 
		(PTR)p);
	p += sizeof(STATUS);

	if (p - orig + sizeof(i4) > *length)
	    break;
	MECOPY_CONST_MACRO(
		(PTR) &rec->sxf_userpriv, 
		sizeof(i4), 
		(PTR)p);
	p += sizeof(i4);

	if (p - orig + sizeof(i4) > *length)
	    break;
	MECOPY_CONST_MACRO(
		(PTR)&rec->sxf_objpriv, 
		sizeof(i4), 
		(PTR)p);
	p += sizeof(i4);

	if (p - orig + sizeof(SXF_ACCESS) > *length)
	    break;
	MECOPY_CONST_MACRO(
		(PTR)&rec->sxf_accesstype, 
		sizeof(SXF_ACCESS), 
		(PTR)p);
	p += sizeof(SXF_ACCESS);

	l = comp_field_len(rec->sxf_objectowner.db_own_name, DB_OWN_MAXNAME);
	if (p - orig + l + 1> *length)
	    break;
	*(p++) = (char)l;
	MEcopy((PTR)rec->sxf_objectowner.db_own_name, l, (PTR)p);
	p += l;

	l = comp_field_len(rec->sxf_object, DB_OBJ_MAXNAME);
	if (p - orig + l + 1 > *length)
	    break;
	*(p++) = (char)l;
	MEcopy((PTR)rec->sxf_object, l, (PTR)p);
	p += l;
	/*
	**	detail text
	*/
	if (rec->sxf_detail_txt && rec->sxf_detail_len > 0)
		l = comp_field_len(rec->sxf_detail_txt, rec->sxf_detail_len);
	else
		l = 0;
  	if (p - orig + 1 +l > *length)
		break;
	/*
	** detail length, then text
	*/
	tmp_i2=l;
	MEcopy((PTR)&tmp_i2, sizeof(tmp_i2), (PTR)p);
	p += sizeof(tmp_i2);

	MEcopy((PTR)rec->sxf_detail_txt, l, (PTR)p);
	p += l;
	/*
	**	detail integer
	*/
	if (p - orig + sizeof(rec->sxf_detail_int) > *length)
	    break;
	MECOPY_CONST_MACRO(
		(PTR)&rec->sxf_detail_int, 
		sizeof(rec->sxf_detail_int), 
		(PTR)p);
	p += sizeof(rec->sxf_detail_int);
	*(p++) = (char)0;	

	/*
	** Session id
	*/
	l = comp_field_len(rec->sxf_sessid, SXF_SESSID_LEN);
	if (p - orig + l + 1> *length)
	    break;
	*(p++) = (char)l;
	MEcopy((PTR)rec->sxf_sessid, l, (PTR)p);
	p += l;

	*length = p - orig;
	tmp_i2 = *length;
	MECOPY_CONST_MACRO(
	    (PTR) &tmp_i2,
	    sizeof (u_i2),
	    (PTR) orig);

	*err_code = E_SX0000_OK;
	break;
    }

    if (*err_code != E_SX0000_OK)
	status = E_DB_ERROR;

    return (status);
}

/*
** Name: UNCOMPRESS_RECORD - uncompress an audit record.
**
** Description:
**	This routine takes a compressed audit record and uncompresses it
**	by inserting any trailing spaces that were removed when the record
**	was compressed.
**
** Inputs:
**	buf		buffer conyaining the compressed audit record.
**	rec		pointer to an SXF_AUDIT_REC structutre where the
**			uncompressed record should be placed.
**	version		File version
**
** Outputs:
**	length		length of the compressed record.
**	err_code	error code.
**
** Returns
**	DB_STATUS
**
** History:
**	1-dec-92 (markg)
**	    Initial creation.
**	11-jan-93 (robf)
**	    Added label, detail fields.
**	17-jun-93 (robf)
**	    Use 2-byte detail text length value
**	    Check version for data elements processed
**	4-jan-94 (robf)
**          For systems with no security label support, use the size of
**	    the (dummy) security label  type rather than DB_MAXNAME since 
**	    they could be different. 
*/
static DB_STATUS
uncompress_record(
    char		*buf,
    SXF_AUDIT_REC	*rec,
    i4			*length,
    i4			version,
    i4		*err_code)
{
    DB_STATUS		status = E_DB_OK;
    u_i2		record_length;
    u_i2		field_length;
    u_char		*p, *orig;
    u_i2		tmp_i2;

    *err_code = E_SX0000_OK;

    orig = (u_char*) buf;
    p=orig;

    for (;;)
    {
	MECOPY_CONST_MACRO(
	    (PTR) p,
	    sizeof (u_i2),
	    (PTR) &record_length);
	p += sizeof (u_i2);

	MECOPY_CONST_MACRO(
	    (PTR) p, 
	    sizeof (SXF_EVENT), 
	    (PTR) &rec->sxf_eventtype);
	p += sizeof (SXF_EVENT);

	MECOPY_CONST_MACRO(
	    (PTR) p, 
	    sizeof (SYSTIME), 
	    (PTR) &rec->sxf_tm);
	p += sizeof (SYSTIME);

	field_length = (u_i2) *p++;
	MEmove(field_length, (PTR) p, ' ', 
	    DB_OWN_MAXNAME, (PTR) rec->sxf_ruserid.db_own_name);
	p += field_length;

	field_length = (u_i2) *p++;
	MEmove(field_length, (PTR) p, ' ', 
	    DB_OWN_MAXNAME, (PTR) rec->sxf_euserid.db_own_name);
	p += field_length;

	field_length = (u_i2) *p++;
	MEmove(field_length, (PTR) p, ' ', 
	    DB_DB_MAXNAME, (PTR) rec->sxf_dbname.db_db_name);
	p += field_length;

	MECOPY_CONST_MACRO(
	    (PTR) p, 
	    sizeof (i4), 
	    (PTR) &rec->sxf_messid);
	p += sizeof (i4);

	MECOPY_CONST_MACRO(
	    (PTR) p, 
	    sizeof (STATUS), 
	    (PTR) &rec->sxf_auditstatus);
	p += sizeof (STATUS);

	MECOPY_CONST_MACRO(
	    (PTR) p, 
	    sizeof (i4), 
	    (PTR) &rec->sxf_userpriv);
	p += sizeof (i4);

	MECOPY_CONST_MACRO(
	    (PTR) p, 
	    sizeof (i4), 
	    (PTR) &rec->sxf_objpriv);
	p += sizeof (i4);

	MECOPY_CONST_MACRO(
	    (PTR) p, 
	    sizeof (SXF_ACCESS), 
	    (PTR) &rec->sxf_accesstype);
	p += sizeof (SXF_ACCESS);

	field_length = (u_i2) *p++;
	MEmove(field_length, (PTR) p, ' ', 
	    DB_OWN_MAXNAME, (PTR) rec->sxf_objectowner.db_own_name);
	p += field_length;

	field_length = (u_i2) *p++;
	MEmove(field_length, (PTR) p, ' ', 
	    DB_OBJ_MAXNAME, (PTR) rec->sxf_object);
	p += field_length;

	/*
	**	Detail text, may be empty.
	*/

	if(version<SXAP_VER_2)
	{
		/*
		** Before version 2 only one byte was stored, and that was
		** just a place holder
		*/
		p++;
	}
	else
	{
		MEcopy((PTR)p, sizeof(tmp_i2), (PTR)&tmp_i2);
		field_length = tmp_i2;
		p += sizeof(tmp_i2);

		if (rec->sxf_detail_txt)
		{
			MEmove(field_length, 
			    (PTR) p, 
			    ' ', 
			    SXF_DETAIL_TXT_LEN, 
			    (PTR) rec->sxf_detail_txt);

			rec->sxf_detail_len=field_length;
		}
		else
		{
			rec->sxf_detail_len=0;
		}
		p += field_length;
	}
	MECOPY_CONST_MACRO(
	    (PTR) p, 
	    sizeof (rec->sxf_detail_int), 
	    (PTR) &rec->sxf_detail_int);
	p += sizeof (rec->sxf_detail_int);

	field_length = (u_i2) *p++;
	p += field_length;

	if(version>SXAP_VER_1)
	{
		/*
		** Session Id only logged for version 2 and above
		*/
		field_length = (u_i2) *p++;
		MEmove(field_length, 
		    (PTR) p, 
		    ' ', 
		    SXF_SESSID_LEN, 
		    (PTR) rec->sxf_sessid);
		p += field_length;
	}
	if ((*length = (p - orig)) != record_length)
	{
	    *err_code = E_SX1065_BAD_REC_LENGTH;
	    break;
	}

	break;
    }

    if (*err_code != E_SX0000_OK)
	status = E_DB_ERROR;

    return (status);
}

/*
** Name: COMP_FIELD_LEN - return the compressed length of a field
**
** Description:
**	This routine returns the the length of an audit record field. The
**	compressed length is the length of the field not including trailing
**	spaces.
**
** Inputs:
**	field		pointer to field in audit record.
**	length		length of field including trailing spaces.
**
** Outputs:
**
** Returns:
**	The length of the field not including trailing spaces.
**
** History:
**	1-dec-92 (markg)
**	    Initial creaion.
*/
static i4
comp_field_len(
    char	*field,
    i4		length)
{
    char	*p;

    for (p = field + length; *(p - 1) == ' ' && p > field; p--)
	;

    return (p - field);
}

/*
** Name: SXAP_FLUSHBUFF - Flush the write buffer (if neccessay)
**
** Description:
**	This routine is used to flush the shared write buffer to disk
**
**
** Returns:
**	DB_STATUS
**
** History:
**      7-dec-1992 (robf)
**	    Initial creation.
**	29-Aug-2008 (jonj)
**	    Take sxap_semaphore before fetching sxap_curr_file,
**	    check for NULL.
**	26-Sep-2008 (jonj)
**	    Check that sxap_semaphore may already be held by thread,
**	    if so, don't release it.
*/
DB_STATUS 
sxap_flushbuff(i4 *err_code)
{
    DB_STATUS 		status=E_DB_OK;
    i4		local_err;
    SXAP_DPG		*pg;
    SXAP_RSCB		*r ;
    SXAP_APB 		*auditbuf;
    
    /* Watch for NULL sxap_curr_rscb */
    if ( status = sxap_sem_lock(err_code) )
        return(status);
    r = Sxap_cb->sxap_curr_rscb;
    if ( status = sxap_sem_unlock(err_code) )
       return(status);;

    if ( !r || r->rs_state != SXAP_RS_OPEN )
    {
	status = E_DB_ERROR;
	*err_code = E_SX102C_BAD_FILE_CONTEXT;
	_VOID_ ule_format(*err_code, NULL, ULE_LOG, 
		NULL, NULL, 0L, NULL, &local_err, 0);
    }
    else
    {
	auditbuf = (SXAP_APB *) r->rs_auditbuf;
	pg = (SXAP_DPG *) &auditbuf->apb_auditpage;

	if (auditbuf->apb_status & SXAP_MODIFY)
	{
	    status = sxap_write_page(&r->rs_di_io, 
		     pg->dpg_header.pg_no, (PTR)pg, TRUE, &local_err);
	    if (status != E_DB_OK)
	    {
		*err_code = local_err;
		if (*err_code > E_SXF_INTERNAL)
		    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    }
	    auditbuf->apb_last_rec.pageno = pg->dpg_header.pg_no;
	    auditbuf->apb_last_rec.offset = pg->dpg_header.pg_next_byte;
	}
    }
    return status;
}

/*
** Name:  sxap_dump_audit_rec
**
** Description: Dumps an audit record
**
** Inputs:
**	rec  Audit  record
**
** Outputs:
**	None
**
** History:
**	3-feb-94 (robf)
**          Created
*/
static VOID
sxap_dump_audit_rec(SXF_AUDIT_REC  *rec)
{
	TRdisplay("*** Dump of SXF security audit record info***\n");
	TRdisplay("  Object Name : %32.32s\n", rec->sxf_object);
	TRdisplay("  Object Owner: %32.32s\n", &rec->sxf_objectowner);
	TRdisplay("  Event type  : %d\n",rec->sxf_eventtype);
	TRdisplay("  Access type : %d (X%x)\n", 
				rec->sxf_accesstype,
				rec->sxf_accesstype);
	TRdisplay("  Status      : %d \n", rec->sxf_auditstatus);
	TRdisplay("  Message id  : %d (X%x)\n", 
				rec->sxf_messid,
				rec->sxf_messid);
}

/*
** Name: sxap_qloc_compare 
**
** Description: Compare two qloc values
**
** Inputs:
**	qloc values to compare
**
** Returns:
**	-1 if loc1 less than loc2
**	 0 if equal
**	 1 if loc1 greater than loc2
**
** History:
**	2-apr-94 (robf)
**        Created
*/
i4
sxap_qloc_compare (SXAP_QLOC *loc1, SXAP_QLOC *loc2)
{
	if(loc1->trip < loc2->trip)
		return -1;
	else if (loc1->trip > loc2->trip)
		return 1;
	else if (loc1->buf > loc2->buf)
		return 1;
	else if (loc1->buf < loc2->buf)
		return -1;
	else
		return 0;
}

/* 
** Name: sxap_qloc_incr
**
** Description:
**	Increments a qloc
**
** Inputs:
**	qloc - Value to increment
**
** Outputs:
**	qloc - incremented value
**
** History:
**	2-apr-94 (robf)
**	    Created
*/
VOID
sxap_qloc_incr (SXAP_QLOC *qloc)
{
	qloc->buf++;
	if(qloc->buf>=Sxap_cb->sxap_num_shm_qbuf)
	{
		/*
		** Wrap buffers, increase trip counter
		*/
		qloc->buf=0;
		qloc->trip++;
	}
}

/*
** Name: SXAP_QFLUSH - flush all audit records for this session to disk
**	               from the queue buffers
**
** Description:
**	This routine is used to flush any buffered audit records previously
**	written by this session to disk.
**
** Algorithm:
**        Take QUEUE lock in X mode
**        While qstart < session_qlast
**	       If qwrite < session_qlast
**                   Increment qwrite
**                   Lock ACCESS in X mode
**                   Unlock QUEUE lock
**                   Write old qwrite
**                   Unlock ACCESS
**                   Take QUEUE lock in X mode
**             else
**                   Lock ACCESS in X mode
**                   Force records to disk
**                   Set qstart to qwrite
**                   Unlock ACCESS
**        Endwhile
**        Unlock QUEUE lock
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
**      2-apr-94 (robf)
**	    Created
**	27-may-94 (robf)
**          If a warning is returned while writing a page then don't
**	    reset record flag, since we may be called again to try and
**	    flush the records once the condition is corrected.
**      23-mar-04 (wanfr01)
**          Bug 110789, INGSRV 2486
**          security audit buffers should be flushed if
**          sxap_force_flush==SXAP_FF_ON, or the information may be lost
**          if a shutdown occurs.
*/
DB_STATUS
sxap_qflush(
    SXF_SCB		*scb,
    i4		*err_code)
{
    DB_STATUS		local_status = E_DB_OK;
    DB_STATUS		status = E_DB_OK;
    i4		local_err;
    bool		loop=FALSE;
    LK_LKID		lk_ac_id;
    LK_VALUE		lk_ac_val;
    LK_LLID		lk_ll = scb->sxf_lock_list;
    LK_LKID		lk_id;
    LK_VALUE		lk_val;
    SXAP_SHM_SEG        *shm=Sxap_cb->sxap_shm;
    SXAP_QBUF		*qbuf;
    bool		q_locked=FALSE;
    bool		locked=FALSE;
    SXAP_QLOC		*sess_last;
    SXAP_QLOC		qwrite_last;
    SXF_AUDIT_REC       audit_rec;
    SXAP_SCB 	        *sxap_scb = (SXAP_SCB*)scb->sxf_sxap_scb;
    char		detail_txt[SXF_DETAIL_TXT_LEN];
    i4			buff_write;
    bool		page_written=FALSE;

    MEfill(sizeof(LK_LKID), 0, &lk_id);

    /*
    ** Degenerate to old algorithm if no shared queue
    */
    if (Sxap_cb->sxap_num_shm_qbuf<1)
	return sxap_flush(scb, err_code);

    Sxf_svcb->sxf_stats->flush_queue++;
    do {
	/* If no records written for this session then we are done */
	if (!(sxap_scb->flags&SXAP_SESS_MODIFY))
	{
            if(SXS_SESS_TRACE(SXS_TAUD_SHM) || debug_trace)
		TRdisplay("SXAP_QFLUSH: %p: No records queued for this session so returning.\n",
				scb);
	    Sxf_svcb->sxf_stats->flush_qempty++;
	    break;
	}

	sess_last= &sxap_scb->sess_lastq;

	status=sxap_q_lock( scb, LK_X, &lk_id, &lk_val, err_code);
	if(status!=E_DB_OK)
		break;
	q_locked=TRUE;
	qwrite_last=shm->shm_qstart;

        if(SXS_SESS_TRACE(SXS_TAUD_SHM) || debug_trace)
		TRdisplay("SXAP_QFLUSH: %p: start=%d, %d end=%d, %d write=%d, %d session=%d, %d\n",
			scb,
			shm->shm_qstart.trip, shm->shm_qstart.buf,
			shm->shm_qend.trip, shm->shm_qend.buf,
			shm->shm_qwrite.trip, shm->shm_qwrite.buf,
			sess_last->trip, sess_last->buf);
	/*
	** Loop while either the start hasn't reached out last write point
	** or start is at the write point and we havn't written that 
	** record yet.
	*/
	while (sxap_qloc_compare(&shm->shm_qstart, sess_last)<0 ||
	       (sxap_qloc_compare(&shm->shm_qwrite, &shm->shm_qstart)==0 &&
	       sxap_qloc_compare( sess_last, &shm->shm_qstart)==0)
	)
	{
	    /*
	    ** If we havn't written the record then write it
	    */
	    if(sxap_qloc_compare(&shm->shm_qwrite, sess_last) <=0 )
	    {
		qbuf= &shm->shm_qbuf[shm->shm_qwrite.buf];
		buff_write=shm->shm_qwrite.buf;

		audit_rec=qbuf->auditrec;
		if(!(qbuf->flags&SXAP_QUSED))
		{
			status=E_DB_ERROR;
			*err_code=E_SX10C2_SXAP_QFLUSH_BUF_FREE;
			sxap_shm_dump();
			/*
			** Try to cleanup for next try.
			*/
			shm->shm_qwrite=shm->shm_qend;
			shm->shm_qstart=shm->shm_qend;
			break;
		}
		if(qbuf->flags&SXAP_QDETAIL)
		{
			MEcopy((PTR)&qbuf->detail_txt,
				SXF_DETAIL_TXT_LEN,
				(PTR)&detail_txt);
			audit_rec.sxf_detail_txt=detail_txt;
		}
		else
			audit_rec.sxf_detail_txt=NULL;
		
		qbuf->flags=SXAP_QEMPTY;
		sxap_qloc_incr(&shm->shm_qwrite);

		MEfill(sizeof(LK_LKID), 0, &lk_ac_id);

		status=sxap_ac_lock(scb, LK_X, &lk_ac_id, &lk_ac_val, err_code);
		if(status!=E_DB_OK)
			break;
		locked=TRUE;
        	if(SXS_SESS_TRACE(SXS_TAUD_SHM) || debug_trace)
			TRdisplay("SXAP_QFLUSH: %p: Writing buffer %d\n",
				scb, buff_write);


		qwrite_last=shm->shm_qwrite;

		status=sxap_q_unlock(scb, &lk_id, &lk_val, err_code);
		if(status!=E_DB_OK)
			break;
		q_locked=FALSE;

		page_written=FALSE;
		status=write_shared_page(scb, Sxap_cb->sxap_curr_rscb, 
				&audit_rec, &lk_ac_id, &lk_ac_val, 
				&page_written, err_code);
		if(status!=E_DB_OK)
			break;
		
		if (Sxap_cb->sxap_force_flush==SXAP_FF_ON)
		{
		   status=flush_shared_page(scb, FALSE, TRUE, err_code);
                   if(status!=E_DB_OK)
                        break;
                   page_written = TRUE;
		}
		status=sxap_ac_unlock(scb, &lk_ac_id, &lk_ac_val, err_code);
		if(status!=E_DB_OK)
			break;
		locked=FALSE;

		status=sxap_q_lock( scb, LK_X, &lk_id, &lk_val, err_code);
		if(status!=E_DB_OK)
			break;
		q_locked=TRUE;
		if(page_written==TRUE)
		{
			/*
			** We know we can move the start point to
			** the last page written since write_shared_page
			** needed to force it
			*/
	                if(sxap_qloc_compare(&shm->shm_qstart, &qwrite_last)<0)
			{
				if(SXS_SESS_TRACE(SXS_TAUD_SHM) || debug_trace)
				     TRdisplay("SXAP_QFLUSH: %p: Moving start from %d, %d to %d, %d \n",
					scb,
					shm->shm_qstart.trip, shm->shm_qstart.buf,
					qwrite_last.trip, qwrite_last.buf
					);
				shm->shm_qstart=qwrite_last;
			}
		}
	}
	else if(Sxap_cb->sxap_force_flush==SXAP_FF_OFF)
	{
		if(SXS_SESS_TRACE(SXS_TAUD_SHM) || debug_trace)
			TRdisplay("SXAP_QFLUSH: %p: Breaking, flush_force is off\n",
				scb);
		break;
	}
	else
	{
		SXAP_QLOC tmpqloc;
		qwrite_last=shm->shm_qwrite;

		status=sxap_q_unlock(scb, &lk_id, &lk_val, err_code);
		if(status!=E_DB_OK)
			break;
		q_locked=FALSE;

		status=sxap_ac_lock(scb, LK_X, &lk_ac_id, &lk_ac_val, err_code);
		if(status!=E_DB_OK)
			break;
		locked=TRUE;
		/*
		** Flush current page to disk
		*/
		if(SXS_SESS_TRACE(SXS_TAUD_SHM) || debug_trace)
			TRdisplay("SXAP_QFLUSH: %p: Flushing shared page to disk\n",
					scb);
		status=flush_shared_page(scb, FALSE, TRUE,  err_code);
		if(status!=E_DB_OK)
			break;

		status=sxap_ac_unlock(scb, &lk_ac_id, &lk_ac_val, err_code);
		if(status!=E_DB_OK)
			break;
		locked=FALSE;

		/* Take lock to update current values */
		status=sxap_q_lock( scb, LK_X, &lk_id, &lk_val, err_code);
		if(status!=E_DB_OK)
			break;
		q_locked=TRUE;
		/* 
		** If start hasn't moved past our known write point 
		** then move the start to the write point, since everywhere
		** up to there has been written
		*/
		if (sxap_qloc_compare(&shm->shm_qstart, &qwrite_last)<0)
		{
			if(SXS_SESS_TRACE(SXS_TAUD_SHM) || debug_trace)
			     TRdisplay("SXAP_QFLUSH: %p: Moving start from %d, %d to %d, %d \n",
				scb,
				shm->shm_qstart.trip, shm->shm_qstart.buf,
				qwrite_last.trip, qwrite_last.buf
				);
			shm->shm_qstart=qwrite_last;
		}
		break;
	  }

	  if(status!=E_DB_OK)
		break;
	}
    } while(loop);

    /* Reset modify counter, unless a warning */
    if (status!=E_DB_WARN)
	    sxap_scb->flags&= ~SXAP_SESS_MODIFY;

    if(SXS_SESS_TRACE(SXS_TAUD_SHM) || debug_trace)
	     TRdisplay("SXAP_QFLUSH: %p: Done\n", scb);

    if(locked)
    {
	local_status=sxap_ac_unlock(scb, &lk_ac_id, &lk_ac_val, &local_err);
	if(local_status!=E_DB_OK)
	{
		status=local_status;
		*err_code=local_err;
	}
    }
    if(q_locked)
    {
	local_status=sxap_q_unlock(scb, &lk_id, &lk_val, &local_err);
	if(local_status!=E_DB_OK)
	{
		status=local_status;
		*err_code=local_err;
	}
    }
    if(status!=E_DB_OK && status!=E_DB_WARN)
    {
	if (*err_code > E_SXF_INTERNAL)
	{
	    _VOID_ ule_format(*err_code, NULL,  ULE_LOG, 
		NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code=E_SX10C1_SXAP_QFLUSH_ERR;
	}
    }
    return status;
}

/*
** Name: SXAP_QALLFLUSH - flush all audit records 
**	               from the queue buffers to disk
**
** Description:
**	This routine is used to flush any buffered audit records previously
**	queued to disk
**
**	It assumes the caller has taken the QUEUE and QUEUE WRITE locks 
**	to prevent new records being queued up while we are doing the 
**	flush.
**
** Algorithm:
**        While qstart < qend
**	       If qwrite < qend
**                   Increment qwrite
**                   Lock ACCESS in X mode
**                   Write old qwrite
**                   Unlock ACCESS
**             else
**                   Lock ACCESS in X mode
**                   Force records to disk
**                   Set qstart to qwrite
**                   Unlock ACCESS
**        Endwhile
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
**      13-apr-94 (robf)
**	    Created
*/
DB_STATUS
sxap_qallflush(
    SXF_SCB		*scb,
    i4		*err_code)
{
    DB_STATUS		local_status = E_DB_OK;
    DB_STATUS		status = E_DB_OK;
    i4		local_err;
    bool		loop=FALSE;
    LK_LLID		lk_ll = scb->sxf_lock_list;
    LK_LKID		lk_ac_id;
    LK_VALUE		lk_ac_val;
    SXAP_SHM_SEG        *shm=Sxap_cb->sxap_shm;
    SXAP_QBUF		*qbuf;
    bool		locked=FALSE;
    SXF_AUDIT_REC       audit_rec;
    SXAP_SCB 	        *sxap_scb = (SXAP_SCB*)scb->sxf_sxap_scb;
    char		detail_txt[SXF_DETAIL_TXT_LEN];
    i4			buff_write;

    Sxf_svcb->sxf_stats->flush_qall++;
    /*
    ** Degenerate to old algorithm if no shared queue
    */
    if (Sxap_cb->sxap_num_shm_qbuf<1)
	return sxap_flush(scb, err_code);

    do {

        if(SXS_SESS_TRACE(SXS_TAUD_SHM) || debug_trace)
		TRdisplay("SXAP_QALLFLUSH: %p: start=%d, %d end=%d, %d write=%d, %d \n",
			scb,
			shm->shm_qstart.trip, shm->shm_qstart.buf,
			shm->shm_qend.trip, shm->shm_qend.buf,
			shm->shm_qwrite.trip, shm->shm_qwrite.buf);

	/* Start the audit  writer to help out */
	start_audit_writer(TRUE);
	/*
	** Loop until no more queued records
	*/
	while (sxap_qloc_compare(&shm->shm_qstart, &shm->shm_qend)<0)
	{
	    /*
	    ** If not all written, write the next
	    */
	    if(sxap_qloc_compare(&shm->shm_qwrite, &shm->shm_qend)<0)
	    {
		qbuf= &shm->shm_qbuf[shm->shm_qwrite.buf];
		buff_write=shm->shm_qwrite.buf;

		audit_rec=qbuf->auditrec;
		if(!(qbuf->flags&SXAP_QUSED))
		{
			status=E_DB_ERROR;
			*err_code=E_SX10C4_SXAP_QALLFLUSH_BUF;
			sxap_shm_dump();
			/*
			** Try to cleanup for next try.
			*/
			shm->shm_qwrite=shm->shm_qend;
			shm->shm_qstart=shm->shm_qend;
			break;
		}
		if(qbuf->flags&SXAP_QDETAIL)
		{
			MEcopy((PTR)&qbuf->detail_txt,
				SXF_DETAIL_TXT_LEN,
				(PTR)&detail_txt);
			audit_rec.sxf_detail_txt=detail_txt;
		}
		else
			audit_rec.sxf_detail_txt=NULL;
		
		qbuf->flags=SXAP_QEMPTY;
		sxap_qloc_incr(&shm->shm_qwrite);

		status=sxap_ac_lock(scb, LK_X, &lk_ac_id, &lk_ac_val, err_code);
		if(status!=E_DB_OK)
			break;
		locked=TRUE;
        	if(SXS_SESS_TRACE(SXS_TAUD_SHM) || debug_trace)
			TRdisplay("SXAP_QALLFLUSH: %p: Writing buffer %d\n",
				scb, buff_write);

		status=write_shared_page(scb, Sxap_cb->sxap_curr_rscb, 
				&audit_rec, &lk_ac_id, &lk_ac_val, 
				NULL, err_code);
		if(status!=E_DB_OK)
			break;
		
		status=sxap_ac_unlock(scb, &lk_ac_id, &lk_ac_val, err_code);
		if(status!=E_DB_OK)
			break;
		locked=FALSE;

	}
	else
	{
		status=sxap_ac_lock(scb, LK_X, &lk_ac_id, &lk_ac_val, err_code);
		if(status!=E_DB_OK)
			break;
		locked=TRUE;
		/*
		** Flush current page to disk
		*/
		status=flush_shared_page(scb, FALSE, TRUE, err_code);
		if(status!=E_DB_OK)
			break;

		status=sxap_ac_unlock(scb, &lk_ac_id, &lk_ac_val, err_code);
		if(status!=E_DB_OK)
			break;
		locked=FALSE;
		shm->shm_qstart.trip=shm->shm_qwrite.trip;
		shm->shm_qstart.buf=shm->shm_qwrite.buf;
	    }
	    if(status!=E_DB_OK)
		break;
	}
    } while(loop);

    if(locked)
    {
	local_status=sxap_ac_unlock(scb, &lk_ac_id, &lk_ac_val, &local_err);
	if(local_status!=E_DB_OK)
	{
		status=local_status;
		*err_code=local_err;
	}
    }
    if(status!=E_DB_OK && status!=E_DB_WARN)
    {
	if (*err_code > E_SXF_INTERNAL)
	{
	    _VOID_ ule_format(*err_code, NULL,  ULE_LOG, 
		NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code=E_SX10C3_SXAP_QALLFLUSH_ERR;
	}
    }
    return status;
}

/*
** Name: start_audit_writer - start the audit  writer thread
**
** Description:
**	This routine may resume the security  audit writer thread if
**	necessary.
**
**	The resume is done if:
**	1: the audit writer thread exists
**	2: Either force_start is TRUE, or the current queue is full 
**	   past its limit
**
** Inputs:
**	force_start - If TRUE forces the writer to start
**
** History:
**	14-apr-94 (robf)
**	    Created
**	3-jun-94 (robf)
**          Check audit writer thread in Sxap_cb now, not Ascb.
**	27-Aug-1997 (jenjo02)
**	    Use sxap_semaphore to guard sxap_status & SXAP_AUDIT_WRITER_BUSY.
**	    We were CSresume-ing the audit writer thread while it was becoming
**	    active and making a lock request, leading LKrequest() to fail
**	    with CL103E.
*/
static VOID
start_audit_writer(bool force_start)
{
        SXF_ASCB	*ascb = Sxf_svcb->sxf_aud_state;
        SXAP_SHM_SEG    *shm  = Sxap_cb->sxap_shm;
	i4	        used_records;
        i4              err_code;

	/* If no audit  writer then return */
	if(!Sxap_cb->sxap_aud_writer)
		return;

	/* If never start thread then return */
	if (Sxap_cb->sxap_aud_writer_start<=0)
		return;

	/* Audit writer already busy so don't resume */
    	if (Sxap_cb->sxap_status & SXAP_AUDIT_WRITER_BUSY)
		return;

	if (shm->shm_qwrite.trip < shm->shm_qend.trip)
	{
		used_records = Sxap_cb->sxap_num_shm_qbuf-
			(shm->shm_qwrite.buf-shm->shm_qend.buf);
	}
	else
	{
		used_records = shm->shm_qend.buf-shm->shm_qwrite.buf;
	}

	if (force_start || used_records > Sxap_cb->sxap_aud_writer_start)
	{
	    /* Protect sxap_status with sxap_semaphore */
            if ( sxap_sem_lock(&err_code) )
                return;

	    /* Check status again after waiting for semaphore */
	    if((Sxap_cb->sxap_status & SXAP_AUDIT_WRITER_BUSY) == 0)
	    {
		if(SXS_SESS_TRACE(SXS_TAUD_SHM) || debug_trace)
		{
		    TRdisplay("START_AUDIT_WRITER: Resuming Audit Writer Thread...\n");
		}
		Sxap_cb->sxap_status |= SXAP_AUDIT_WRITER_BUSY;
		CSresume(Sxap_cb->sxap_aud_writer);
	    }
            sxap_sem_unlock(&err_code);
	}
	return;
}

/*
** Name: sxap_audit_writer - audit writer thread
**
** Description:  
**	This routine implements the low-level  audit writer mechanism.
**	It writes out any queued up audit records until the queue is
**	empty then returns. Note that it doesn't do flushes (like
**	sxap_qflush() since these are expensive and are delayed until
**	the session associated with the records needs to do the flush).
**
** Inputs:
**	scb	- SCB
**
** Outputs:
**	err_code - Error status
**
** Returns:
**	E_DB_STATUS
**
** History:
**	14-apr-94 (robf)
**	    Created
**	3-jun-94 (robf)
**	    Initialize Sxap_cb->sxap_aud_writer here now.
**	27-Aug-1997 (jenjo02)
**	    Protect sxap_status with sxap_semaphore.
*/
DB_STATUS 
sxap_audit_writer(
SXF_SCB		*scb,
i4		*err_code)
{
    DB_STATUS		local_status = E_DB_OK;
    DB_STATUS		status = E_DB_OK;
    i4		local_err;
    bool		loop = FALSE;
    LK_LLID		lk_ll = scb->sxf_lock_list;
    LK_LKID		lk_ac_id;
    LK_VALUE		lk_ac_val;
    LK_LKID		lk_id;
    LK_VALUE		lk_val;
    SXAP_SHM_SEG        *shm = Sxap_cb->sxap_shm;
    SXAP_QBUF		*qbuf;
    bool		locked = FALSE;
    bool		q_locked = FALSE;
    SXF_AUDIT_REC       audit_rec;
    SXAP_SCB 	        *sxap_scb = (SXAP_SCB*)scb->sxf_sxap_scb;
    char		detail_txt[SXF_DETAIL_TXT_LEN];
    i4			buff_write;
    bool		page_written = FALSE;
    SXAP_QLOC		qwrite_last;


    MEfill(sizeof(LK_LKID), 0, &lk_id);

    if (SXS_SESS_TRACE(SXS_TAUD_SHM) || debug_trace)
	TRdisplay("SXAP_AUDIT_WRITER: %p: Starting \n", scb);

    if ( status = sxap_sem_lock(err_code) )
        return(status);
    Sxap_cb->sxap_status |= SXAP_AUDIT_WRITER_BUSY;

    /* Initialize audit writer here if not done already */
    if (Sxap_cb->sxap_aud_writer == 0)
    {
	CSget_sid(&Sxap_cb->sxap_aud_writer);
    }
    if ( status = sxap_sem_unlock(err_code) )
        return(status);

    Sxf_svcb->sxf_stats->audit_writer_wakeup++;
    /*
    ** Degenerate to old algorithm if no shared queue
    */
    if (Sxap_cb->sxap_num_shm_qbuf < 1)
    {
	status = sxap_flush(scb, err_code);
        if ( local_status = sxap_sem_lock(err_code) )
            return(local_status);
        Sxap_cb->sxap_status &= ~SXAP_AUDIT_WRITER_BUSY;
        if ( local_status = sxap_sem_unlock(err_code) )
            return(local_status);
	return status;
    }

    do {
	status = sxap_q_lock( scb, LK_X, &lk_id, &lk_val, err_code);
	if (status != E_DB_OK)
	    break;
	q_locked = TRUE;

        if (SXS_SESS_TRACE(SXS_TAUD_SHM) || debug_trace)
	    TRdisplay("SXAP_AUDIT_WRITER: %p: start=%d, %d end=%d, %d write=%d, %d \n",
		scb, shm->shm_qstart.trip, shm->shm_qstart.buf,
		shm->shm_qend.trip, shm->shm_qend.buf,
		shm->shm_qwrite.trip, shm->shm_qwrite.buf);

	/*
	** Loop until no more queued records
	*/
	while (sxap_qloc_compare(&shm->shm_qwrite, &shm->shm_qend)<0)
	{
	    qbuf = &shm->shm_qbuf[shm->shm_qwrite.buf];
	    buff_write = shm->shm_qwrite.buf;
	    qwrite_last = shm->shm_qwrite;

	    audit_rec = qbuf->auditrec;
	    if (!(qbuf->flags&SXAP_QUSED))
	    {
		status = E_DB_ERROR;
		*err_code = E_SX10C8_AUDIT_WRITE_BUF_FREE;
		sxap_shm_dump();
		/*
		** Try to cleanup for next try.
		*/
		shm->shm_qwrite = shm->shm_qend;
		shm->shm_qstart = shm->shm_qend;
		break;
	    }
	    if (qbuf->flags & SXAP_QDETAIL)
	    {
		MEcopy((PTR)&qbuf->detail_txt, SXF_DETAIL_TXT_LEN,
				(PTR)&detail_txt);
		audit_rec.sxf_detail_txt=detail_txt;
	    }
	    else
		audit_rec.sxf_detail_txt = NULL;
		
	    qbuf->flags = SXAP_QEMPTY;
	    sxap_qloc_incr(&shm->shm_qwrite);

	    status = sxap_ac_lock(scb, LK_X, &lk_ac_id, &lk_ac_val, err_code);
	    if (status != E_DB_OK)
		break;
	    locked = TRUE;

	    status = sxap_q_unlock(scb, &lk_id, &lk_val, err_code);
	    if (status != E_DB_OK)
		break;
	    q_locked=FALSE;


            if (SXS_SESS_TRACE(SXS_TAUD_SHM) || debug_trace)
		TRdisplay("SXAP_AUDIT_WRITER: %p: Writing buffer %d into shared page\n",
				scb, buff_write );
	    page_written = FALSE;
	    status = write_shared_page(scb, Sxap_cb->sxap_curr_rscb, 
				&audit_rec, &lk_ac_id, &lk_ac_val, 
				&page_written, err_code);
	    if (status != E_DB_OK)
		break;
		
	    status = sxap_ac_unlock(scb, &lk_ac_id, &lk_ac_val, err_code);
	    if (status != E_DB_OK)
		break;
	    locked = FALSE;

	    status = sxap_q_lock( scb, LK_X, &lk_id, &lk_val, err_code);
	    if (status != E_DB_OK)
		break;
	    q_locked = TRUE;
	    if (page_written == TRUE)
	    {
		/*
		** We know we can move the start point to
		** the last page written since write_shared_page
		** needed to force it
		*/
	        if (sxap_qloc_compare(&shm->shm_qstart, &qwrite_last) < 0)
		{
		    if (SXS_SESS_TRACE(SXS_TAUD_SHM) || debug_trace)
			TRdisplay("SXAP_AUDIT_WRITER: %p: Moving start from %d, %d to %d, %d \n",
				scb,
				shm->shm_qstart.trip, shm->shm_qstart.buf,
				qwrite_last.trip, qwrite_last.buf
				);
		    shm->shm_qstart = qwrite_last;
		}
	    }

	}
    } while(loop);

    if (locked)
    {
	local_status = sxap_ac_unlock(scb, &lk_ac_id, &lk_ac_val, &local_err);
	if (local_status != E_DB_OK)
	{
	    status = local_status;
	    *err_code = local_err;
	}
    }
    if (q_locked)
    {
	local_status = sxap_q_unlock(scb, &lk_id, &lk_val, &local_err);
	if (local_status != E_DB_OK)
	{
	    status = local_status;
	    *err_code = local_err;
	}
    }
    if (status != E_DB_OK && status != E_DB_WARN)
    {
	if (*err_code > E_SXF_INTERNAL)
	{
	    _VOID_ ule_format(*err_code, NULL,  ULE_LOG, 
		NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code = E_SX10C7_AUDIT_WRITE_ERROR;
	}
    }
    if (SXS_SESS_TRACE(SXS_TAUD_SHM) || debug_trace)
    {
	TRdisplay("SXAP_AUDIT_WRITER: %p: Done\n", scb);
    }

    if ( local_status = sxap_sem_lock(err_code) )
         return(local_status);
    Sxap_cb->sxap_status &= ~SXAP_AUDIT_WRITER_BUSY;
    if ( local_status = sxap_sem_unlock(err_code) )
         return(local_status);

    return status;
}
