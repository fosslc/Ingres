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
# include    <lk.h>
# include    <tm.h>
# include    <di.h>
# include    <lo.h>
# include    <st.h>
# include    <ulf.h>
# include    <ulm.h>
# include    <sxf.h>
# include    <sxfint.h>
# include    <sxap.h>
# include    "sxapint.h"

/*
** Name: SXAPF.C - audit file control routines for the SXAP auditing system
**
** Description:
**      This file contains all of the control routines used by the SXF
**      low level auditing system, SXAP.
**
**          sxap_open     - open an audit file for reading or writing
**          sxap_create   - create an audit file
**          sxap_close    - close an audit file previously opened by sxap_open
**
** History:
**      2-sep-1992 (markg)
**          Initial creation as stubs.
**	10-dec-1992 (markg)
**	    Fixed memory leak problem where memory allocated in sxap_open
**	    was not getting freed in sxap_close.
**	26-jan-1993 (markg)
**	    Fix uninitialized variable bug in sxap_open().
**      10-mar-1993 (markg)
**          Fixed problem where errors returned from ulm were not getting
**          handled correctly.
**	10-may-1993 (markg)
**	    Updated some comments.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	13-aug-93 (stephenb)
**	    Integrated following change made by robf in es2dev branch:
**	    When opening "current" file make sure security auditing is enabled
**	    and issue error if not.
**	04-Aug-1997 (jenjo02)
**	    Removed sxf_incr(). It's gross overkill to semaphore-protect a
**	    simple statistic.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	19-Aug-1997 (jenjo02)
**	    Distinguish between private and shared ULM streams.
**	    Piggyback ulm_palloc() calls with ulm_openstream() where appropriate.
**	05-mar-98 (musro02)
**          In SXAP_OPEN, test length of filename instead of filebuf 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** Name: SXAP_OPEN - open an audit file
**
** Description:
**	This routine is used to open an audit file that can be used for
**	either reading or writing audit records. SXF supports the ability 
**	to have a maximum of one audit file open for writing at 
**	a time, however many audit files may be open for reading. This routine
**	returns an access identifier (actually a pointer to a SXAP_RSCB 
**	structure) that must be specified for all other operations requested 
**	on this file. If no filename is specified (i.e. filename == NULL) then
**	the current audit file will be opened.
**
** 	Overview of algorithm:-
**
**	Validate the parameters passed to the routine.
**	Open a memory stream and allocate and build a SXAP_RSCB.
**	If filename is NULL get the name of the current audit file.
**	Call the DIopen routine to perform the physical open call.
**	Validate file header page
**
**
** Inputs:
**	filename		Name of the audit file to open, if this is NULL
**				the current audit file will be opened.
**	mode			either SXF_READ of SXF_WRITE
**	sxf_rscb		the SXF_RSCB for the audit file
**
** Outputs:
**	sxf_rscb.sxf_sxap	Access identifier for audit file. 
**	filesize		The size of the audit file
**	err_code		Error code returned to the caller.
**	stamp			Stamp for the file.
**
** Returns:
**	DB_STATUS
**
** History:
**      2-sep-1992 (markg)
**	    Initial creation.
**	7-dec-1992 (robf)
**	    Added handling of file stamp as a parameter
**	10-dec-1992 (markg)
**	    Fixed memory leak problem. This routine now opens its own
**	    memory stream for building SXAP_RSCB structures. This stream
**	    is closed in sxap_close.
**	17-dec-1992 (robf)
**	    Log out-of-memory errors better.
**	26-jan-1993 (markg)
**	    Fix uninitialized variable bug where apb_status is not
**	    getting initialized correctly. 
**      10-mar-1993 (markg)
**          Fixed problem where errors returned from ulm were not getting
**          handled correctly.
**	7-jul-93 (robf)
**	    Support multiple file versions, store version in RSCB.
**	13-aug-93 (stephenb)
**	    Integrate following following change mabe by robf in es2dev branch:
**          When opening the "current" file make sure security auditing
**          is enabled, and issue appropriate error.
**	27-jul-93 (robf)
**	    When opening the "current" file make sure security auditing
**	    is enabled, and issue appropriate error.
**	26-oct-93 (robf)
**          Upated the "current" check to be audit disabled, consistent
**	    with other checks.
**	13-jan-94 (stephenb)
**          Integrate robf's fix:
**	       Add checks for open/close state.
**	4-may-94 (robf)
**          When opening a file for reading and force_flush is
**	    disabled, flush any pending records to disk first to
**	    ensure we get a consistent read.
**	05-mar-98 (musro02)
**          Change to test the length of filename instead of filebuf
**          prior to moving filename into filebuf.
*/
DB_STATUS
sxap_open(
    PTR			filename,
    i4			mode,
    SXF_RSCB		*sxf_rscb,
    i4		*filesize,
    PTR			sptr,
    i4		*err_code)
{
    DB_STATUS		status = E_DB_OK;
    STATUS		cl_status;
    CL_ERR_DESC		cl_err;
    i4		local_err;
    ULM_RCB		ulm_rcb, tmp_ulm;
    SXAP_RSCB		*sxap_rscb;
    LOCATION		loc;
    char		dev[LO_NM_LEN];
    char		fprefix[LO_NM_LEN];
    char		fsuffix[LO_NM_LEN];
    char		version[LO_NM_LEN];
    char		file[LO_NM_LEN];
    char		path[MAX_LOC + 1];
    char		filebuf[MAX_LOC + 1];
    char		*loc_out;
    bool		file_open = FALSE;
    bool		stream_open = FALSE;
    bool		tmp_stream_open = FALSE;
    SXAP_HPG		*hpg;
    SXAP_STAMP		*stamp = (SXAP_STAMP*)sptr;
    u_i4		page_size;
    bool		size_retry=FALSE;
    u_i4		tmp_page_size;

    *err_code = E_SX0000_OK;

    for (;;)
    {
	/* Validate the parameters */
	if ((mode != SXF_READ && mode != SXF_WRITE) ||
	    sxf_rscb == NULL )
	{
	    *err_code = E_SX0004_BAD_PARAMETER;
	    break;
	}

	/* 
	** Open a memory stream, and allocate and build the SXAP 
	** record control structures. If the file is being opened 
	** for write, the SXAP shared memory segment is used to 
	** buffer file access, if the file is being opened for read, 
	** allocate a page buffer from the newmemory stream.
	*/
	ulm_rcb.ulm_facility = DB_SXF_ID;
	ulm_rcb.ulm_poolid = Sxf_svcb->sxf_pool_id;
	ulm_rcb.ulm_blocksize = 0;
	ulm_rcb.ulm_memleft = &Sxf_svcb->sxf_pool_sz;
	ulm_rcb.ulm_streamid_p = &ulm_rcb.ulm_streamid;
	
	/* No other allocates are done on this stream - make it private */
	/* Open stream and allocate SXAP_RSCB with one call */
	ulm_rcb.ulm_flags = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;
	ulm_rcb.ulm_psize = sizeof (SXAP_RSCB);

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

	sxf_rscb->sxf_sxap = ulm_rcb.ulm_pptr;
	sxap_rscb = (SXAP_RSCB *) sxf_rscb->sxf_sxap;
	sxap_rscb->rs_memory = ulm_rcb.ulm_streamid;
	sxap_rscb->rs_state = SXAP_RS_INVALID;

	if (mode == SXF_WRITE)
	    sxap_rscb->rs_auditbuf = &Sxap_cb->sxap_shm->shm_apb;
	else
	{
	    ulm_rcb.ulm_psize = sizeof (SXAP_APB);
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
	    sxap_rscb->rs_auditbuf = (SXAP_APB *) ulm_rcb.ulm_pptr;
	    sxap_rscb->rs_auditbuf->apb_status = SXAP_READ;
	}

	/* Parse the filename */
	for (;;)
	{
	    *err_code = E_SX101E_INVALID_FILENAME;
	    if (filename)
	    {
	        if (STlength(filename) > MAX_LOC)
                    break;
	        STcopy(filename, filebuf);
	    }
	    else
	    {
		/*
		** Use the "current" file, only valid when security auditing
		** is enabled.
		*/
		if(!(Sxap_cb->sxap_status & SXAP_ACTIVE))
		{
		    _VOID_ ule_format(E_SX10A7_NO_CURRENT_FILE, 
			NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
		    /*
		    ** Return no-such-file external error
		    */
		    *err_code = E_SX0015_FILE_NOT_FOUND;
		    break;
		}
	        STcopy(Sxap_cb->sxap_curr_file, filebuf);
	    }
	    STcopy(filebuf, sxap_rscb->rs_filename);

	    if (LOfroms(PATH & FILENAME, filebuf, &loc) != OK)
	        break;
	    if (LOdetail(&loc, dev, path, fprefix, fsuffix, version) != OK)
	        break;
	    if (LOcompose(dev, path, "", "", "", &loc) != OK)
	        break;
	    LOtos(&loc, &loc_out);
	    STcopy(loc_out, path);
	    if (LOcompose("", "", fprefix, fsuffix, version, &loc) != OK)
	        break;
	    LOtos(&loc, &loc_out);
	    STcopy(loc_out, file);
	    *err_code = E_SX0000_OK;
	    break;
	}
	if (*err_code != E_SX0000_OK)
	{
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	/*
	** Start at current configured page size
	*/
        page_size=Sxap_cb->sxap_page_size;
	for(;;)
	{
	    /* call DIopen to perform the physical file open */
	    cl_status = DIopen(
		&sxap_rscb->rs_di_io, 
		path, 
		(u_i4) STlength(path),
		file, 
		(u_i4) STlength(file), 
		page_size,
		mode == SXF_WRITE ? DI_IO_WRITE : DI_IO_READ,
		DI_SYNC_MASK,
		&cl_err);
	    if (cl_status != OK)
	    {
	        if (cl_status == DI_BADDIR || 
	    	    cl_status == DI_DIRNOTFOUND ||
		    cl_status == DI_FILENOTFOUND)
	        {
		    *err_code = E_SX0015_FILE_NOT_FOUND;
		    break;
	        }
		/*
		** DI_BADPARAM should be returned on bad page size.
		*/
		if (cl_status == DI_BADPARAM && page_size<=SXAP_MAX_PGSIZE)
		{
		    if (size_retry==FALSE)
		    {
			size_retry=TRUE;
			page_size=SXAP_MIN_PGSIZE;
		    }
		    else
			page_size*=2;
		    /*
		    ** Try again
		    */
		    continue;
		}
	        *err_code = E_SX1018_FILE_ACCESS_ERROR;
	        _VOID_ ule_format(*err_code, &cl_err, ULE_LOG, 
		    NULL, NULL, 0L, NULL, &local_err, 0);
	        break;
	    }
	    file_open = TRUE;

	    /*
	    ** Read the file header page to ensure that the
	    ** file is of a valid type. If the file is being
	    ** opened for reading, we can use the buffer associated
	    ** with the RSCB, otherwise we will have to allocate a 
	    ** temporary buffer.
	    */
	    if (mode == SXF_READ)
	    {
	        hpg = (SXAP_HPG *) &sxap_rscb->rs_auditbuf->apb_auditpage;
	    }
	    else
	    {
	        tmp_ulm.ulm_facility = DB_SXF_ID;
	        tmp_ulm.ulm_poolid = Sxf_svcb->sxf_pool_id;
	        tmp_ulm.ulm_blocksize = 0;
	        tmp_ulm.ulm_memleft = &Sxf_svcb->sxf_pool_sz;
		tmp_ulm.ulm_streamid_p = &tmp_ulm.ulm_streamid;
		tmp_ulm.ulm_flags = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;
	        tmp_ulm.ulm_psize = page_size;
	        status = ulm_openstream(&tmp_ulm);
	        if (status != E_DB_OK)
	        {
	    	    if (tmp_ulm.ulm_error.err_code == E_UL0005_NOMEM)
		        *err_code = E_SX1002_NO_MEMORY;
	            else
		        *err_code = E_SX106B_MEMORY_ERROR;

		    _VOID_ ule_format(tmp_ulm.ulm_error.err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
		    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	            break;
	        }
	        tmp_stream_open = TRUE;

	        hpg = (SXAP_HPG *) tmp_ulm.ulm_pptr;
	    }

	    status = sxap_read_page(&sxap_rscb->rs_di_io, 
			0L, (PTR) hpg, page_size, &local_err);
	    if (status != E_DB_OK)
	    {
	        *err_code = local_err;
	        if (*err_code > E_SXF_INTERNAL)
	            _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	        break;
	    }
	    /*
	    **	Sanity check for type of file & version
	    */
	    if (hpg->hpg_header.pg_type != SXAP_HEADER)
	    {
	        *err_code = E_SX1021_BAD_PAGE_TYPE;
	        _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	        break;
	    }
	    /*
	    ** Can only handle files at our level or below
	    */
	    if (hpg->hpg_version > SXAP_VERSION)
	    {
	        *err_code = E_SX1022_BAD_FILE_VERSION;
	        _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	        break;
	    }
	    /* 
	    ** Save file version for  later use 
	    */
	    sxap_rscb->rs_version=hpg->hpg_version;	
	    /*
	    ** Check page size, introduced at version 2.1.
	    ** Earlier versions always had 2048 byte page size.
	    */
	    if(hpg->hpg_version >= SXAP_VER_21)
		tmp_page_size=hpg->hpg_pgsize;
	    else
		tmp_page_size=2048;

	    if(tmp_page_size!=page_size)
	    {
		/*
		** Page size mismatch, so correct and try again
		*/
		page_size=tmp_page_size;
	 	if (tmp_stream_open)
		{
		    _VOID_ ulm_closestream(&tmp_ulm);
		    tmp_stream_open=FALSE;
		}
	        if (file_open)
		{
		    _VOID_ DIclose(&sxap_rscb->rs_di_io, &cl_err);
		    file_open=TRUE;
		}
		continue;
	    }
	    /*
	    ** Don't use page size from the file, it may not exist
	    ** for old files under version 2.1
	    */
	    sxap_rscb->rs_pgsize=page_size;	

	    break;

	} /* End loop */
	if(*err_code!=E_SX0000_OK)
		break;
	/*
	** 	Save the stamp for the file. May need this later
	**	when checking which file to use
	*/
	if (stamp)
	{
		STRUCT_ASSIGN_MACRO(hpg->hpg_stamp,*stamp);	
	}
	/*
	** Flush out any waiting records if force_flush is disabled and
	** we are reading. This ensures we can read the records we
	** just generated.
	*/
        if(Sxap_cb->sxap_force_flush==SXAP_FF_OFF  &&
	   mode==SXF_READ &&
	   Sxap_cb->sxap_curr_rscb!= NULL)
	{
	    status = sxap_flushbuff(&local_err);
	    if(status!=E_DB_OK)
	    {
		*err_code = local_err;
		if (*err_code > E_SXF_INTERNAL)
	    	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			    NULL, NULL, 0L, NULL, &local_err, 0);
		break;
	    }
	}
	/* Find out the size of the file */
	cl_status = DIsense(
			&sxap_rscb->rs_di_io, 
			&sxap_rscb->rs_last_page,
			&cl_err);
	if (cl_status != OK)
	{
	    *err_code = E_SX1018_FILE_ACCESS_ERROR;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	if (filesize)
	    *filesize = sxap_rscb->rs_last_page * sxap_rscb->rs_pgsize / 1024;

        if (tmp_stream_open)
	{
	    status = ulm_closestream(&tmp_ulm);
	    if (status != E_DB_OK)
	    {
		*err_code = E_SX106B_MEMORY_ERROR;
	        _VOID_ ule_format(tmp_ulm.ulm_error.err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	        _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
		break;
	    }
	    tmp_stream_open = FALSE;
	}

	break;
    }

    /* Handle any errors */
    if (*err_code != E_SX0000_OK)
    {

	if (file_open)
	    _VOID_ DIclose(&sxap_rscb->rs_di_io, &cl_err);

	if (tmp_stream_open)
	    _VOID_ ulm_closestream(&tmp_ulm);

	if (stream_open)
	    _VOID_ ulm_closestream(&ulm_rcb);

	if (*err_code > E_SXF_INTERNAL)
	{
	    char   *f = filename;

	    if (f == NULL)
		f = Sxap_cb->sxap_curr_file;

	    _VOID_ ule_format(E_SX1036_AUDIT_FILE_ERROR, NULL, 
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 
			1, STlength (f), (PTR) f);

	    *err_code = E_SX1023_SXAP_OPEN;
	}

	if (status == E_DB_OK)
	    status = E_DB_ERROR;
    }
    else
    {
	    /*
	    **	File opened OK, so increment stats counter
	    */
	    if (mode == SXF_READ)
		    Sxf_svcb->sxf_stats->open_read++;
	    else
		    Sxf_svcb->sxf_stats->open_write++;
	    sxap_rscb->rs_state = SXAP_RS_OPEN;
    }
   
    return (status);
}

/*
** Name: SXAP_CREATE - create an audit file
**
** Description:
**	This routine is used to create an audit file that can be used
**	for either reading or writing audit records. When created each audit
**	file will have an initialized header page, and an initialized, empty,
**	data page.
**
** 	Overview of algorithm:
**
**	Validate the parameters passed to the routine.
**	Call the DIcreate routine to perform the physical create call.
**	Open the file and write the header page, and an empty data page.
**	Close the file.
**
**
** Inputs:
**	filename		Name of audit file to create. 
**
** Outputs:
**	err_code		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**      2-sep-1992 (markg)
**	    Initial creation.
**      10-mar-1993 (markg)
**          Fixed problem where errors returned from ulm were not getting
**          handled correctly.
*/
DB_STATUS
sxap_create(
    PTR		filename,
    i4	*err_code)
{
    DB_STATUS		status = E_DB_OK;
    STATUS		cl_status;
    CL_ERR_DESC		cl_err;
    i4		local_err;
    char		filebuf[MAX_LOC + 1];
    LOCATION		loc;
    char		dev[LO_NM_LEN];
    char		fprefix[LO_NM_LEN];
    char		fsuffix[LO_NM_LEN];
    char		version[LO_NM_LEN];
    char		file[LO_NM_LEN];
    char		path[MAX_LOC + 1];
    char		*loc_out;
    DI_IO		di_io;
    SXAP_HPG		*hpg;
    ULM_RCB		ulm_rcb;
    i4		pageno;
    bool		file_created = FALSE;
    bool		file_open = FALSE;
    bool		stream_open = FALSE;
    SXAP_SHM_SEG        *shm=Sxap_cb->sxap_shm;


    *err_code = E_SX0000_OK;
    /*
    **	Increment stats counter
    */
    Sxf_svcb->sxf_stats->create_count++;

    for (;;)
    {
	/* Validate the filename */
	for (;;)
	{
	    *err_code = E_SX101E_INVALID_FILENAME;
	    if (filename == NULL) 
	        break;
	    if (STlength((char *) filename) > MAX_LOC)
	        break;
	    STcopy((char *) filename,     filebuf);
	    if (LOfroms(PATH & FILENAME, filebuf, &loc) != OK)
	        break;
	    if (LOdetail(&loc, dev, path, fprefix, fsuffix, version) != OK)
	        break;
	    if (LOcompose(dev, path, "", "", "", &loc) != OK)
	        break;
	    LOtos(&loc, &loc_out);
	    STcopy(loc_out, path);
	    if (LOcompose("", "", fprefix, fsuffix, version, &loc) != OK)
	        break;
	    LOtos(&loc, &loc_out);
	    STcopy(loc_out, file);
	    *err_code = E_SX0000_OK;
	    break;
	}
	if (*err_code != E_SX0000_OK)
	{
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	/* call DIcreate to perform the physical file create */
	cl_status = DIcreate(
		&di_io, 
		path, 
		(u_i4) STlength(path),
		file, 
		(u_i4) STlength(file), 
		Sxap_cb->sxap_page_size,
		&cl_err);
	if (cl_status != OK)
	{
	    if (cl_status == DI_EXISTS)
	    {
		*err_code = E_SX0016_FILE_EXISTS;
		break;
	    }
	    *err_code = E_SX1018_FILE_ACCESS_ERROR;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG, 
		NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	file_created = TRUE;

	/*
	** Now open the file in write mode and initialize the file
	** header and an empty data page.
	*/
	cl_status = DIopen(
		&di_io, 
		path, 
		(u_i4) STlength(path),
		file, 
		(u_i4) STlength(file), 
		Sxap_cb->sxap_page_size,
		DI_IO_WRITE,
		DI_SYNC_MASK,
		&cl_err);
	if (cl_status != OK)
	{
	    *err_code = E_SX1018_FILE_ACCESS_ERROR;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG, 
		NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	file_open = TRUE;

	/* allocate a page buffer */
	ulm_rcb.ulm_facility = DB_SXF_ID;
	ulm_rcb.ulm_poolid = Sxf_svcb->sxf_pool_id;
	ulm_rcb.ulm_blocksize = 0;
	ulm_rcb.ulm_memleft = &Sxf_svcb->sxf_pool_sz;
	ulm_rcb.ulm_streamid_p = &ulm_rcb.ulm_streamid;
	ulm_rcb.ulm_flags = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;
	ulm_rcb.ulm_psize = Sxap_cb->sxap_page_size;
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

	hpg = (SXAP_HPG *) ulm_rcb.ulm_pptr;

	/*
	** Allocate the first two pages in the file. The first
	** will be a header page, which must be initialized
	** with the file version and current time. The second
	** will be an empty data page.
	*/

	status = sxap_alloc_page(&di_io, (PTR) hpg, &pageno, 
				Sxap_cb->sxap_page_size, &local_err);
	if (status != E_DB_OK)
	{
	    *err_code = local_err;
	    if (*err_code > E_SXF_INTERNAL)
	        _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	if (pageno != 0)
	{
	    *err_code = E_SX1024_BAD_FILE_ALLOC;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	hpg->hpg_header.pg_type = SXAP_HEADER;
	hpg->hpg_header.pg_next_byte = sizeof (SXAP_HPG) + 1;
	hpg->hpg_version = SXAP_VERSION;
	hpg->hpg_pgsize=Sxap_cb->sxap_page_size;
	/*
	**	Get the stamp for the log
	*/
	sxap_stamp_now(&hpg->hpg_stamp);

	/* Write the header page back to disk */
	status = sxap_write_page(&di_io, pageno, (PTR) hpg, TRUE, &local_err);
	if (status != E_DB_OK)
	{
	    *err_code = local_err;
	    if (*err_code > E_SXF_INTERNAL)
	        _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	/* Allocate an empty data page */
	status = sxap_alloc_page(&di_io, (PTR) hpg, &pageno, 
			Sxap_cb->sxap_page_size, &local_err);
	if (status != E_DB_OK)
	{
	    *err_code = local_err;
	    if (*err_code > E_SXF_INTERNAL)
	        _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	if (pageno != 1)
	{
	    *err_code = E_SX1024_BAD_FILE_ALLOC;
	    break;
	}

	/*
	** Close the file and free allocated buffers before
	** returning to the caller.
	*/
	if (DIclose(&di_io, &cl_err) != OK)
	{
	    *err_code = E_SX1018_FILE_ACCESS_ERROR;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG, 
		NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	file_open = FALSE;

	status = ulm_closestream(&ulm_rcb);
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

	break;
    }

    /* Handle any errors */
    if (*err_code != E_SX0000_OK)
    {
	if (file_open)
	    _VOID_ DIclose(&di_io, &cl_err);

	if (file_created)
	    _VOID_ DIdelete(&di_io, path, (u_i4) STlength(path),
		file, (u_i4) STlength(file), &cl_err);

	if (stream_open)
	    _VOID_ ulm_closestream(&ulm_rcb);

	if (*err_code > E_SXF_INTERNAL)
	{
	    _VOID_ ule_format(E_SX1036_AUDIT_FILE_ERROR, NULL, 
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 
			1, STlength (filename), (PTR) filename);

	    *err_code = E_SX1025_SXAP_CREATE;
	}

	if (status == E_DB_OK)
	    status = E_DB_ERROR;
    }


    return (status);
}

/*
** Name: SXAP_CLOSE - close an audit file
**
** Description:
**	This routine closes an audit file that has previously been opened 
**	by a call to sxap_open.
**
** 	Overview of algorithm:-
**
**	Call DIclose 
**	Close the memory stream associated with this file.
**
** Inputs:
**	sxf_rscb		The SXF_RSCB structure associated with this
**				audit file
**
** Outputs:
**	err_code		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**      2-sep-1992 (markg)
**	    Initial creation.
**	10-dec-1992 (markg)
**	    Fixed memory leak problem. This routine now closes a memory stream
**	    that was opened in sxap_open.
**      10-mar-1993 (markg)
**          Fixed problem where errors returned from ulm were not getting
**          handled correctly. Also check that sxap_rscb passed is not
**	    empty.
**	27-oct-93 (robf)
**          Flush current page if modifiedf data on it.
**	13-jan-94 (stephenb)
**	    Add checks for open/close state.
*/	    
DB_STATUS
sxap_close(
    SXF_RSCB	*rscb,
    i4	*err_code)
{
    DB_STATUS		status = E_DB_OK;
    i4		local_err;
    STATUS		cl_status;
    CL_ERR_DESC		cl_err;
    ULM_RCB		ulm_rcb;
    SXAP_RSCB		*rs = (SXAP_RSCB *) rscb->sxf_sxap;
    SXAP_SHM_SEG        *shm=Sxap_cb->sxap_shm;

    *err_code = E_SX0000_OK;

    for (;;)
    {
	if (rs == NULL || rs->rs_state != SXAP_RS_OPEN)
	{
	    *err_code = E_SX0004_BAD_PARAMETER;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	/*
	** Increment stats counter
	*/
	Sxf_svcb->sxf_stats->close_count++;

	/*
	** Make sure buffer is flushed if write file being closed
	** (note this depends on their only being a single open write
	** file at a time currently)
	*/
	if (rs->rs_auditbuf->apb_status & SXAP_WRITE)
	{
	    status = sxap_flushbuff(&local_err);
	    if (status != E_DB_OK)
	    {
		*err_code = local_err;
		if (*err_code > E_SXF_INTERNAL)
	    	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			    NULL, NULL, 0L, NULL, &local_err, 0);
		break;
	    }
	}
	cl_status = DIclose(&rs->rs_di_io, &cl_err);
	if (cl_status != OK)
	{
	    *err_code = E_SX1018_FILE_ACCESS_ERROR;
	    _VOID_ ule_format(*err_code, &cl_err, 
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code = E_SX1036_AUDIT_FILE_ERROR;
	    _VOID_ ule_format(*err_code, NULL, 
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 1, 
			STlength (rs->rs_filename), (PTR) rs->rs_filename);
	}

	/* Close the memory stream associated with the SXAP_RSCB */
	ulm_rcb.ulm_facility = DB_SXF_ID;
	ulm_rcb.ulm_streamid_p = &rs->rs_memory;
	ulm_rcb.ulm_memleft = &Sxf_svcb->sxf_pool_sz;
	status = ulm_closestream(&ulm_rcb);
	if (status != E_DB_OK)
	{
	    *err_code = E_SX106B_MEMORY_ERROR;
	    _VOID_ ule_format(ulm_rcb.ulm_error.err_code, NULL, 
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	}
	/* ULM has nullified rs_memory */

	rscb->sxf_sxap = NULL;
	break;
    }

    if (*err_code != E_SX0000_OK)
    {
	*err_code = E_SX1026_SXAP_CLOSE;
	status = E_DB_ERROR;
    }

    rs->rs_state = SXAP_RS_CLOSE;
    return (status);
}
