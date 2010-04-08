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
#include    <scf.h>
/**
**
**  Name: PSQENDSES.C - Functions used to end a parser session.
**
**  Description:
**      This file contains the functions necessary to end a parser session.
**
**          psq_end_sess - Routine to end a parser session
**
**
**  History:
**      01-oct-85 (jeff)    
**          written
**	24-feb-88 (stec)
**	    Must call pst_commit_dsql for cleanup purpose.
**	13-sep-90 (teresa)
**	    change psq_force from boolean to bitflag.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	09-Oct-1998 (jenjo02)
**	    Removed SCF semaphore functions, inlining the CS calls instead.
[@history_template@]...
**/

/*
** forward declarations
*/

/*{
** Name: psq_end_session	- End a parser session
**
**  INTERNAL PSF call format: status = psq_end_session(&psq_cb, &sess_cb);
**
**  EXTERNAL call format: status = psq_call(PSQ_END_SESSION, &psq_cb, &sess_cb);
**
** Description:
**      The psq_end_session ends a parser session, freeing up the resources 
**      used therein.  It should be used when a user stops using INGRES, or
**	when some error makes it impossible to proceed within a session.
**
** Inputs:
**      psq_cb
**	    .psq_flag		    Word containing assorted flags
**          .psq_force                  TRUE iff the session should be ended
**					despite error conditions.
**	sess_cb			    Pointer to session control block
**				    (Can be NULL)
**
** Outputs:
**      psq_cb
**          .psq_error                  Error information
**		.err_code		    What error occurred
**		    E_PS0000_OK			Success
**		    E_PS0002_INTERNAL_ERROR	Internal inconsistency in PSF
**		    E_PS0205_SRV_NOT_INIT	Server not initialized
**		    E_PS0601_CURSOR_OPEN	There was an open cursor
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Causes memory to be deallocated.
**
** History:
**	01-oct-85 (jeff)
**          written
**	24-feb-88 (stec)
**	    Must call pst_commit_dsql for cleanup purpose.
**	13-feb-90 (andre)
**	    set scf_stype to SCU_EXCLUSIVE before calling scu_swait.
**	16-nov-90 (andre)
**	    failure to acquire or to release a semaphore while ending a session
**	    will cause the server to be shut down
**	    we will finally start to initialize the scf_cb before calling
**	    scf_call().  Until now scf_call() was returning an error which we
**	    conveniently chose to disregard.  As a result, we could be running
**	    inside the critical region regardless of who else was accessing
**	    Psf_srvblk.
**	10-aug-93 (andre)
**	    fixed cause of a compiler warning
**	09-Oct-1998 (jenjo02)
**	    Removed SCF semaphore functions, inlining the CS calls instead.
**	01-jul-2002 (thaju02)
**	    ingstop -force|immediate may fail with SIGSEGV in psq_end_session.
**	    With OS threads, race condition may occur where stop server thread
**	    removes PSF memory pool before user session has run through
**	    session termination. (B102787)
*/
DB_STATUS
psq_end_session(
	register PSQ_CB    *psq_cb,
	PSS_SESBLK	   *sess_cb)
{
    i4                  originit;
    i4		err_code;
    DB_STATUS		maxerr = E_DB_OK;
    DB_STATUS		status;
    STATUS		sem_status;
    i4		sem_errno;
    bool		leave_loop = TRUE;
    extern PSF_SERVBLK	*Psf_srvblk;

    /*
    ** No error to begin with.
    */
    psq_cb->psq_error.err_code = E_PS0000_OK;

    if (!Psf_srvblk)
    {
	TRdisplay("psq_end_session: Psf_srvblk is null\n");
	return(maxerr);
    }
    /*
    ** Check for server initialized.
    */
    originit = Psf_srvblk->psf_srvinit;
    if (!Psf_srvblk->psf_srvinit)
    {
	(VOID) psf_error(E_PS0205_SRV_NOT_INIT, 0L, PSF_CALLERR, &err_code,
	    &psq_cb->psq_error, 0);
	if (! (psq_cb->psq_flag & PSQ_FORCE) )
	    return (E_DB_ERROR);
	if (maxerr < E_DB_ERROR)
	    maxerr = E_DB_ERROR;
    }

    do	    	    /* something to break out of */
    {
	if (sem_status = CSp_semaphore(1, &Psf_srvblk->psf_sem)) /* exclusive */
	{
	    sem_errno = E_PS020C_ENDSES_GETSEM_FAILURE;
	    break;
	}

	Psf_srvblk->psf_srvinit = TRUE;  /* So psq_sesfind() won't complain */

	/*
	** FIXME (maybe)
	** It would be nice to make this critical region smaller, but I don't
	** quite see a reasonable way of doing it.
	*/
	
	/*
	** Kill the session.
	*/
	if (Psf_srvblk->psf_nmsess > 0)
	{
	    /*
	    ** In case any dynamic SQL related objects in QSF are still around
	    ** do the cleanup. Ignore return status.
	    */
	    (VOID)pst_commit_dsql(psq_cb, sess_cb);

	    status = psq_killses(sess_cb, (psq_cb->psq_flag & PSQ_FORCE), 
				 &psq_cb->psq_error);
	    if (DB_FAILURE_MACRO(status))
	    {
		if (maxerr < status)
		{
		    maxerr = status;
		}

		if (~psq_cb->psq_flag & PSQ_FORCE)
		{
		    break;
		}
	    }

	    /* Uncount the session */
	    Psf_srvblk->psf_nmsess--;
	}

	/* leave_loop has already been set to TRUE */
    } while (!leave_loop);

    /* if semaphore has been successfully acquired, try to release it */
    if (sem_status == OK)
    {
	/* Restore the server initialized flag to its original state */
	Psf_srvblk->psf_srvinit = originit;

	if (sem_status = CSv_semaphore(&Psf_srvblk->psf_sem))
	    sem_errno = E_PS020D_ENDSES_RELSEM_FAILURE;
    }

    /*
    ** if an error was encountered while trying to get or to release a
    ** semaphore, set maxerr to E_DB_FATAL to bring down the server
    */
    if (sem_status != OK)
    {
	(VOID) psf_error(sem_errno, sem_status, PSF_INTERR,
	    &err_code, &psq_cb->psq_error, 0);
	maxerr = E_DB_FATAL;	    /* bring down the server */
    }

    return (maxerr);
}


/*{
** Name: psq_killses	- Deallocate everything in a session control block.
**
** Description:
**      This function deallocates all memory streams inside a session control
**	block.
**
** Inputs:
**      sess_cb                         Pointer to the session control block
**					to be deleted
**      force                           TRUE means delete it even if there are
**					errors along the way
**      err_blk                         Pointer to an error block
**
** Outputs:
**      err_blk                         Filled in with any errors that happen
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
**	Exceptions:
**	    none
**
** Side Effects:
**	    Deallocates memory
**
** History:
**	06-jan-86 (jeff)
**          written
*/
DB_STATUS
psq_killses(
	PSS_SESBLK         *sess_cb,
	i4		   force,
	DB_ERROR   	   *err_blk)
{
    register PSC_CURBLK	*cur_cb;
    register PSC_CURBLK	*next_cur_cb;
    DB_STATUS		maxerr = E_DB_OK;
    register i4	i;
    register DB_STATUS	status;
    register i4	curfound = FALSE;
    i4		err_code;
    ULM_RCB		ulm_rcb;
    extern PSF_SERVBLK	    *Psf_srvblk;
        

    /* Close the symbol table memory stream */
    ulm_rcb.ulm_facility = DB_PSF_ID;
    ulm_rcb.ulm_poolid	= Psf_srvblk->psf_poolid;
    ulm_rcb.ulm_streamid_p = &sess_cb->pss_symstr;
    ulm_rcb.ulm_memleft = &sess_cb->pss_memleft;
    status = ulm_closestream(&ulm_rcb);
    if (status != E_DB_OK)
    {
	status = E_DB_ERROR;
	if (ulm_rcb.ulm_error.err_code == E_UL0004_CORRUPT)
	{
	    (VOID) psf_error(E_PS0602_CORRUPT_MEM, ulm_rcb.ulm_error.err_code,
		PSF_INTERR, &err_code, err_blk, 0);
	    status = E_DB_FATAL;
	}
    }
    if (status != E_DB_OK)
    {
	if (force)
	{
	    if (maxerr != E_DB_FATAL)
	    {
		maxerr = status;
	    }
	}
	else
	{
	    return (status);
	}
    }
    else
    {
	/* Only unlink the cb if the deallocation succeeded */
	/* ULM has nullified pss_symstr */
	sess_cb->pss_symtab = (PSS_SYMBLK*) NULL;
	sess_cb->pss_symnext = (u_char *) NULL;
	sess_cb->pss_symblk = (PSS_SYMBLK *) NULL;
    }

    for (i = 0; i < PSS_CURTABSIZE; i++)
    {
	for (cur_cb = sess_cb->pss_curstab.pss_curque[i]; cur_cb;
	    cur_cb = next_cur_cb)
	{
	    /*
	    ** It's an error to try to end a session with open cursors.
	    ** Only report it the first time it happens.
	    */
	    if (!curfound && cur_cb->psc_open)
	    {
		(VOID) psf_error(E_PS0601_CURSOR_OPEN, 0L, PSF_CALLERR,
		    &err_code, err_blk, 0);
		curfound = TRUE;
	    }

	    /*
	    ** If the caller has not asked for a forced operation, then just
	    ** return the error.  On a forced operation, get rid of the cursor.
	    */
	    if (!force)
	    {
		return (E_DB_ERROR);
	    }

	    /* Get next control block before deallocating current */
	    next_cur_cb = cur_cb->psc_next;
	    status = psq_delcursor(cur_cb, &sess_cb->pss_curstab,
		&sess_cb->pss_memleft, err_blk);
	    if ((status == E_DB_ERROR || status == E_DB_FATAL) &&
		maxerr != E_DB_FATAL)
	    {
		    maxerr = status;
	    }
	}
    }

    return (maxerr);
}
