/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>

/**
**
**  Name: PSFMEM.C - Parser memory allocator.
**
**  Description:
**      This file contains the functions for allocation memory within the 
**      parser facility.
**
**          psf_mopen	- Open a memory stream.
**          psf_malloc	- Allocate memory from a stream.
**          psf_mclose	- Close a memory stream.
**	    psf_mroot	- Set the root for an stream.
**	    psf_mchtyp	- Change object type for memory stream.
**	    psf_mlock	- lock the memory stream.
**	    psf_munlock	- unlock the memory stream.
**	    psf_symall	- Allocates next symbol block.
**          psf_umalloc	- Allocate memory from  ULM stream.
**
**
**  History:
**      27-dec-85 (jeff)    
**          written
**	18-mar-87 (stec)
**	    Add psf_mchtyp() call.
**	03-apr-87 (puree)
**	    Add psf_mlock().
**	25-nov-87 (stec)
**	    Wrote psf_symall.
**	19-apr-89 (neil)
**	    Wrote psf_umalloc.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-aug-93 (andre)
**	    removed declaration for qsf_call()
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**      29-nov-93 (rblumer)
**          Added psf_munlock routine.
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for qsf_owner which has changed type to PTR.
**	31-jan-97 (pchang)
**	    psf_symall(), when allocating memory for a symbol table block,
**	    failed to take into account the space needed for the next block
**	    pointer 'pss_sbnext' which later caused memory overrun in the data
**	    area 'pss_symdata'.  In some cases, the corruption is neutralized
**	    when the user of the violated adjacent memory space overlays it
**	    with new data.  Whereas, if the violated memory space is occupied
**	    by a less volatile entity such as a pointer for a linked list,
**	    what follows could be a random SEGV.  (B75105?)
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp().
**	    Changed psf_sesscb() prototype to return PSS_SESBLK* instead
**	    of DB_STATUS. Supply scf_session to SCF when it's known.
**	    Use QSF_RCB embedded in PSS_SESBLK, initialized once when
**	    session began.
**	28-Jan-2004 (schka24)
**	    Don't lie about what memory pool is exhausted.  Who knows
**	    how many support hours have been wasted increasing psf-memory
**	    when it's really QSF memory that ran out...
[@history_template@]...
**/

/*{
** Name: psf_mopen	- Open a memory stream.
**
** Description:
**      All memory allocation within the parser is done by means of streams. 
**      A memory stream must be opened before it is used.  To open it, one 
**	specifies the type of object to be allocated (query text, query tree,
**	or query plan), and gives a place to put the stream id and the lock
**	id.  The stream id and lock id are used for all future references to
**	the memory stream.
**
** Inputs:
**	sess_cb				Ptr to session's CB.
**      objtype                         The type of memory to be allocated.
**	mstream				Pointer to structure to contain stream
**					id and lock id, which will uniquely
**					identify object in future.
**	err_blk				Place to put the error information.
**	    E_PS0A05_BADMEMREQ	    Bad memory request
**
** Outputs:
**	mstream				Filled in with stream id and lock id
**	err_blk				Filled in with error number and text.
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s);
**					error status in psq_cb.psq_error chain
**					of error blocks.
**	    E_DB_ERROR			Function failed due to error by caller;
**					error status in psq_cb.psq_error chain
**					of error blocks.
**	    E_DB_FATAL			Function failed due to some internal
**					problem; error status in
**					psq_cb.psq_error chain of error blocks.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory.
**
** History:
**	27-dec-85 (jeff)
**          written
*/
DB_STATUS
psf_mopen(
	PSS_SESBLK	   *sess_cb,
	i4		   objtype,
	PSF_MSTREAM	   *mstream,
	DB_ERROR	   *err_blk)
{
    i4			 err_code;
    DB_STATUS			 status;

    sess_cb->pss_qsf_rcb.qsf_obj_id.qso_type = objtype;
    sess_cb->pss_qsf_rcb.qsf_obj_id.qso_lname = 0;
    status = qsf_call(QSO_CREATE, &sess_cb->pss_qsf_rcb);
    if (status != E_DB_OK)
    {
	i4	    qerr;

	qerr = sess_cb->pss_qsf_rcb.qsf_error.err_code;

	if (qerr == E_QS0001_NOMEM)
	    (VOID) psf_error(qerr, qerr, PSF_CALLERR, &err_code,
		err_blk, 0);
	else 
	    (VOID) psf_error(E_PS0A05_BADMEMREQ, qerr, PSF_INTERR, &err_code,
		err_blk, 0);
	mstream->psf_mstream.qso_handle = (PTR) NULL;
	mstream->psf_mlock = 0;
	return (status);
    }

    mstream->psf_mstream.qso_handle = sess_cb->pss_qsf_rcb.qsf_obj_id.qso_handle;
    mstream->psf_mlock = sess_cb->pss_qsf_rcb.qsf_lk_id;

    return (E_DB_OK);
}

/*{
** Name: psf_malloc	- Allocate memory from a stream.
**
** Description:
**      This function allocates memory from a memory stream.  If there is room 
**      in the current block, it will use it.  If not, it will allocate another 
**      block and use that.
**
** Inputs:
**	sess_cb				Ptr to session's CB.
**      stream                          Pointer to the stream to allocate from.
**      size                            Number of bytes to allocate
**      result                          Place to put pointer to allocated memory
**	err_blk				Place to put error information.
**
** Outputs:
**      result                          Filled in with pointer to alloc. memory
**	err_blk				Filled in with error information.
**	    E_PS0A05_BADMEMREQ		    Bad memory request
**
**	Returns:
**	    E_DB_OK			Operation succeeded.
**	    E_DB_ERROR			Error by caller (e.g. bad parameter).
**	    E_DB_FATAL			Operation failed (e.g. out of memory)
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can allocate memory.
**
** History:
**	27-dec-85 (jeff)
**          written
*/
DB_STATUS
psf_malloc(
	PSS_SESBLK	   *sess_cb,
	PSF_MSTREAM	   *stream,
	i4                size,
	void		   *result,
	DB_ERROR	   *err_blk)
{
    i4		err_code;
    DB_STATUS		status;

    /*
    ** Caller error if non-positive request.
    */
    if (size <= 0)
    {
	psf_error(E_PS0A05_BADMEMREQ, 0L, PSF_INTERR, &err_code, err_blk, 0);
	*(void **) result = NULL;
	return (E_DB_SEVERE);
    }

    /* Allocate the object from QSF */
    sess_cb->pss_qsf_rcb.qsf_obj_id.qso_handle = stream->psf_mstream.qso_handle;
    sess_cb->pss_qsf_rcb.qsf_lk_id = stream->psf_mlock;
    sess_cb->pss_qsf_rcb.qsf_sz_piece = size;
    status = qsf_call(QSO_PALLOC, &sess_cb->pss_qsf_rcb);
    if (status != E_DB_OK)
    {
	i4		qerr = sess_cb->pss_qsf_rcb.qsf_error.err_code;

	if (qerr == E_QS0001_NOMEM)
	    (VOID) psf_error(qerr, qerr, PSF_CALLERR, &err_code,
		err_blk, 0);
	else 
	    (VOID) psf_error(E_PS0A05_BADMEMREQ, qerr, PSF_INTERR, &err_code,
		err_blk, 0);
	*(void **) result = NULL;
	return (E_DB_ERROR);
    }

    *(void **) result = sess_cb->pss_qsf_rcb.qsf_piece;
    return (E_DB_OK);
}

/*{
** Name: psf_mclose	- Close a memory stream.
**
** Description:
**      This function closes a memory stream, which amounts to deallocating 
**      all of the memory held by it.
**
** Inputs:
**	sess_cb				Ptr to session's CB.
**      stream                          Pointer to the stream to be closed.
**      err_blk                         Place to put error information.
**
** Outputs:
**      err_blk                         Filled in with error information.
**	    E_PS0006_CANTFREE		    Error when freeing memory.
**
**	Returns:
**	    E_DB_OK			Operation succeeded.
**	    E_DB_ERROR			Error by caller (e.g. bad parameter).
**	    E_DB_FATAL			Operation failed (e.g. out of memory)
**	Exceptions:
**	    none
**
** Side Effects:
**	    Deallocates memory.
**
** History:
**	27-dec-85 (jeff)
**          written
**	6-may-87 (daved)
**	    lock memory stream if not already locked.
**	    This condition can happen if, for example, a syntax error
**	    is discovered after psq_tout has been called and we now
**	    want to delete the memory stream.
*/
DB_STATUS
psf_mclose(
	PSS_SESBLK	   *sess_cb,
	PSF_MSTREAM        *stream,
	DB_ERROR	    *err_blk)
{
    i4		err_code;
    DB_STATUS		status;

    if (stream->psf_mlock == 0)
    {
	status = psf_mlock(sess_cb, stream, err_blk);
	if (status != E_DB_OK)
	    return (status);
    }
    sess_cb->pss_qsf_rcb.qsf_obj_id.qso_handle = stream->psf_mstream.qso_handle;
    sess_cb->pss_qsf_rcb.qsf_lk_id = stream->psf_mlock;
    status = qsf_call(QSO_DESTROY, &sess_cb->pss_qsf_rcb);
    if (status != E_DB_OK)
    {
	(VOID) psf_error(E_PS0A06_CANTFREE, sess_cb->pss_qsf_rcb.qsf_error.err_code, 
	    PSF_INTERR, &err_code, err_blk, 0);
	return (status);
    }

    return (E_DB_OK);
}

/*{
** Name: psf_mroot	- Set the root for a memory stream
**
** Description:
**      This function sets the root for a memory stream.  When the next
**	facility looks at the object allocated by the stream, it can ask
**	for the root.
**
** Inputs:
**	sess_cb				Ptr to session's CB.
**      mstream                         Pointer to the memory stream
**      root                            Pointer to the root of the object
**
** Outputs:
**      None
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	30-may-86 (jeff)
**          written
*/
DB_STATUS
psf_mroot(
	PSS_SESBLK	   *sess_cb,
	PSF_MSTREAM        *mstream,
	PTR		   root,
	DB_ERROR	   *err_blk)
{
    DB_STATUS		status;
    i4		err_code;

    sess_cb->pss_qsf_rcb.qsf_obj_id.qso_handle = mstream->psf_mstream.qso_handle;
    sess_cb->pss_qsf_rcb.qsf_root = root;
    sess_cb->pss_qsf_rcb.qsf_lk_id = mstream->psf_mlock;
    status = qsf_call(QSO_SETROOT, &sess_cb->pss_qsf_rcb);
    if (status != E_DB_OK)
    {
	(VOID) psf_error(E_PS0A05_BADMEMREQ, sess_cb->pss_qsf_rcb.qsf_error.err_code, 
	    PSF_INTERR, &err_code, err_blk, 0);
	return (status);
    }

    return (E_DB_OK);
}


/*{
** Name: psf_mchtyp - Change object type for a memory stream.
**
** Description:
**      This function changes the object type for a memory stream.
**
** Inputs:
**	sess_cb				Ptr to session's CB.
**      mstream                         Pointer to the memory stream
**      newtype				New type for the object
**	err_block			Pointer to error control block
**
** Outputs:
**      err_blk                         Filled in with error information.
**	    E_PS0A07_CANTCHANGE		Error when changing object type.
**
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	18-mar-87 (stec)
**          Written.
*/
DB_STATUS
psf_mchtyp(
	PSS_SESBLK	   *sess_cb,
	PSF_MSTREAM     *mstream,
	i4		objtype,
	DB_ERROR	*err_blk)
{
    DB_STATUS		status;
    i4		err_code;

    sess_cb->pss_qsf_rcb.qsf_obj_id.qso_handle = mstream->psf_mstream.qso_handle;
    sess_cb->pss_qsf_rcb.qsf_obj_id.qso_type = objtype;
    sess_cb->pss_qsf_rcb.qsf_obj_id.qso_lname = 0;
    sess_cb->pss_qsf_rcb.qsf_lk_id = mstream->psf_mlock;
    status = qsf_call(QSO_CHTYPE, &sess_cb->pss_qsf_rcb);
    if (status != E_DB_OK)
    {
	(VOID) psf_error(E_PS0A07_CANTCHANGE, sess_cb->pss_qsf_rcb.qsf_error.err_code, 
	    PSF_INTERR, &err_code, err_blk, 0);
	return (status);
    }

    return (E_DB_OK);
}

/*{
** Name: psf_mlock - Lock an QSO object stream
**
** Description:
**      This function locks the specified QSO object.
**
** Inputs:
**	sess_cb				Ptr to session's CB.
**      mstream                         Pointer to the memory stream
**	    .psf_mstream.qso_handle	Object handle
**	err_block			Pointer to error control block
**
** Outputs:
**      mstream                         Pointer to the memory stream
**	    .psf_mlock			Lock id returned by QSF
**      err_blk                         Filled in with error information.
**	    E_PS0A08_CANTLOCK		Error when locking object type.
**
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	03-apr-87 (puree)
**          Written.
*/
DB_STATUS
psf_mlock(
	PSS_SESBLK	   *sess_cb,
	PSF_MSTREAM     *mstream,
	DB_ERROR	*err_blk)
{
    DB_STATUS		status;
    i4		err_code;

    sess_cb->pss_qsf_rcb.qsf_obj_id.qso_handle = mstream->psf_mstream.qso_handle;
    sess_cb->pss_qsf_rcb.qsf_lk_state = QSO_EXLOCK;
    sess_cb->pss_qsf_rcb.qsf_obj_id.qso_lname = 0;
    status = qsf_call(QSO_LOCK, &sess_cb->pss_qsf_rcb);
    if (status != E_DB_OK)
    {
	(VOID) psf_error(E_PS0A08_CANTLOCK, sess_cb->pss_qsf_rcb.qsf_error.err_code, 
	    PSF_INTERR, &err_code, err_blk, 0);
	return (status);
    }

    mstream->psf_mlock = sess_cb->pss_qsf_rcb.qsf_lk_id;
    return (E_DB_OK);

}  /* end psf_mlock */

/*{
** Name: psf_munlock - Unlock a QSO object stream
**
** Description:
**      This function unlocks the specified QSO object.
**
** Inputs:
**	sess_cb				Ptr to session's CB.
**      mstream                         Pointer to the memory stream
**	    .psf_mstream.qso_handle	Object handle
**	err_block			Pointer to error control block
**
** Outputs:
**      mstream                         Pointer to the memory stream
**	    .psf_mlock			Lock id returned by QSF
**      err_blk                         Filled in with error information.
**	    E_PS0B05_CANT_UNLOCK	Error when unlocking object type.
**
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	29-nov-93 (rblumer)
**          Written.
*/
DB_STATUS
psf_munlock(
	PSS_SESBLK	   *sess_cb,
	PSF_MSTREAM     *mstream,
	DB_ERROR	*err_blk)
{
    DB_STATUS		status;
    i4		err_code;

    sess_cb->pss_qsf_rcb.qsf_lk_id	 = mstream->psf_mlock;
    STRUCT_ASSIGN_MACRO(mstream->psf_mstream, sess_cb->pss_qsf_rcb.qsf_obj_id);

    status = qsf_call(QSO_UNLOCK, &sess_cb->pss_qsf_rcb);
    if (status != E_DB_OK)
    {
	(VOID) psf_error(E_PS0B05_CANT_UNLOCK, sess_cb->pss_qsf_rcb.qsf_error.err_code, 
	    PSF_INTERR, &err_code, err_blk, 0);
	return (status);
    }

    mstream->psf_mlock = 0;
    return (E_DB_OK);

}  /* end psf_munlock */

/*{
** Name: psf_symall	Allocates memory for a symbol block.
**
** Description:
**      Symbol blocks are used mainly by scanners to pass additional data
**	to parsers. They also hold emmitted text. This routine allocates
**	a new block and takes care of initializing appropriate fields in the
**	session control block.
**
** Inputs:
**      pss_cb				session control block pointer.
**	psq_cb				query control block.
**	size				amount of memory to be allocated.
**
** Outputs:
**	pss_cb				session control block with
**					appropriate fields initialized.
**	psq_cb				query control block with psq_error
**					field initialized, if error.
**	Returns:
**	    E_DB_OK
**	    status			return by ULM
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory.
**
** History:
**	25-nov-87 (stec)
**          written
**	31-jan-97 (pchang)
**	    psf_symall(), when allocating memory for a symbol table block,
**	    failed to take into account the space needed for the next block
**	    pointer 'pss_sbnext' which later caused memory overrun in the data
**	    area 'pss_symdata'.  In some cases, the corruption is neutralized
**	    when the user of the violated adjacent memory space overlays it
**	    with new data.  Whereas, if the violated memory space is occupied
**	    by a less volatile entity such as a pointer for a linked list,
**	    what follows could be a random SEGV.  (B75105?)
*/
DB_STATUS
psf_symall(
	PSS_SESBLK  *pss_cb,
	PSQ_CB	    *psq_cb,
	i4	    size)
{
    DB_STATUS	status = E_DB_OK;
    ULM_RCB	ulm_rcb;
    i4	err_code;

    /* If no more blocks, allocate another */
    if (pss_cb->pss_symblk->pss_sbnext==(PSS_SYMBLK *) NULL)
    {
	/* If out of room, allocate some more */
	ulm_rcb.ulm_facility = DB_PSF_ID;
	ulm_rcb.ulm_streamid_p = &pss_cb->pss_symstr;
	ulm_rcb.ulm_psize = size + sizeof(PTR);    /* add space for fwd ptr */
	ulm_rcb.ulm_memleft = &pss_cb->pss_memleft;
	status = ulm_palloc(&ulm_rcb);

	if (DB_FAILURE_MACRO(status))
	{
	    if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
	    {
		psf_error(E_PS0F02_MEMORY_FULL, 0L, PSF_CALLERR,
		    &err_code, &psq_cb->psq_error, 0);
	    }
	    else
	    {
		psf_error(2704L, 0L, PSF_USERERR,
		    &err_code, &psq_cb->psq_error, 0);
	    }
	    return (status);
	}
	pss_cb->pss_symblk->pss_sbnext = (PSS_SYMBLK*) ulm_rcb.ulm_pptr;
	pss_cb->pss_symblk->pss_sbnext->pss_sbnext = NULL;
    }

    /* Set current block and next character pointer */
    pss_cb->pss_symblk = pss_cb->pss_symblk->pss_sbnext;
    pss_cb->pss_symnext = pss_cb->pss_symblk->pss_symdata;

    return (E_DB_OK);
}

/*{
** Name: psf_umalloc	- Allocate memory from ULM.
**
** Description:
**      General purpose interface into allocating from ULM for the current
**	session.
**
** Inputs:
**      sess_cb				Session control block pointer.
**	mstream				ULM stream id.
**	msize				Size of memory.
**
** Outputs:
**	sess_cb				Session control block:
**		.pss_memleft		Memory left in current session.
**	mresult				Pointer to result object.
**	err_blk.err_code		Error returned
**		E_PS0F02_MEMORY_FULL	No more memory for session
**		E_PS0A02_BADALLOC	Some other ULM error
**
**	Returns:
**	    status			return by ULM
**	    E_DB_FATAL			If memory corrupted.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory.
**
** History:
**	19-apr-89 (neil)
**          Written
*/
DB_STATUS
psf_umalloc(
	PSS_SESBLK  	*sess_cb,
	PTR		mstream,
	i4	    	msize,
	PTR		*mresult,
	DB_ERROR	*err_blk)
{
    DB_STATUS	status;
    ULM_RCB	ulm_rcb;
    i4	err_code;

    ulm_rcb.ulm_facility = DB_PSF_ID;
    ulm_rcb.ulm_streamid_p = &mstream;
    ulm_rcb.ulm_psize = msize;
    ulm_rcb.ulm_memleft = &sess_cb->pss_memleft;
    status = ulm_palloc(&ulm_rcb);

    if (status != E_DB_OK)
    {
	if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
	{
	    _VOID_ psf_error(E_PS0F02_MEMORY_FULL, 0L, PSF_CALLERR, &err_code,
			     err_blk, 0);
	}
	else
	{
	    _VOID_ psf_error(E_PS0A02_BADALLOC, 0L, PSF_INTERR, &err_code,
			     err_blk, 0);
	}
	if (ulm_rcb.ulm_error.err_code == E_UL0004_CORRUPT)
	    status = E_DB_FATAL;
	*mresult = NULL;
    }
    else
    {
	*mresult = ulm_rcb.ulm_pptr;
    }
    return (status);
} /* psf_umalloc */
