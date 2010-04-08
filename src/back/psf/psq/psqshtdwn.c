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
#include    <scf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>

/**
**
**  Name: PSQSHTDWN.C - Functions used to shut down the parser.
**
**  Description:
**      This file contains the functions necessary to shut down the parser 
**      for a whole server.
**
**          psq_shutdown - Shut down the parser for a whole server.
**
**
**  History:
**      01-oct-85 (jeff)    
**          written
**	10-nov-87 (puree)
**	    added a call to RDF_TERMINATE to shutdown RDF properly.
**	13-sep-90 (teresa)
**	    changed psq_force from boolean to bitflag.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	06-aug-1997 (canor01)
**	    Remove semaphore before freeing memory.
**	09-Oct-1998 (jenjo02)
**	    Removed SCF semaphore functions, inlining the CS calls instead.
[@history_template@]...
**/


/*{
** Name: psq_shutdown	- Shut down the parser for a server.
**
**  INTERNAL PSF call format: status = psq_shutdown(psq_cb, NULL);
**
**  EXTERNAL call format:     status = psq_call(PSQ_SHUTDOWN, psq_cb, NULL);
**
** Description:
**      The psq_shutdown shuts down the parser facility for a server. 
**      Normally, it is only used when the whole server is being shut 
**      down.  There should be no active parser sessions when this operation
**	is called; it will return an error otherwise.  There is an option
**	telling this function to shut down the parser regardless of whether
**	there are any active sessions or not; if there are, it will end the
**	sessions first.
**
** Inputs:
**      psq_cb
**	    psq_flag			contains bitflags:
**               PSQ_FORCE		if set, shut it down regardless of
**					error conditions.
**	sess_cb				Ignored
**
** Outputs:
**      psq_cb
**	    .psq_error			Filled in with error information
**		.err_code		    What error occurred
**		    E_PS0000_OK			Success
**		    E_PS0002_INTERNAL_ERROR	Internal inconsistency in PSF
**		    E_PS0205_SRV_NOT_INIT	Server not initialized
**		    E_PS0601_CURSOR_OPEN	A cursor is still open
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can cause memory to be deallocated.
**	    The server will be marked as uninitialized.
**
** History:
**	01-oct-85 (jeff)
**          written
**	10-nov-87 (puree)
**	    added a call to RDF_TERMINATE to shutdown RDF properly.
**	14-dec-87 (stec)
**	    Fixed declaration of psf_error (should return DB_STATUS).
**	10-aug-93 (andre)
**	    fixed cause of a compiler error
**	06-aug-1997 (canor01)
**	    Remove semaphore before freeing memory.
**	09-Oct-1998 (jenjo02)
**	    Removed SCF semaphore functions, inlining the CS calls instead.
*/
DB_STATUS
psq_shutdown(
	PSQ_CB             *psq_cb)
{
    DB_STATUS		maxerr = E_DB_OK;
    DB_STATUS		status;
    i4		err_code;
    i4			originit;
    RDF_CCB		rdf_ccb;
    ULM_RCB		ulm_rcb;
    extern PSF_SERVBLK  *Psf_srvblk;

    /*
    ** No error to begin with.
    */
    psq_cb->psq_error.err_code= E_PS0000_OK;

    /*
    ** Check for server initialized.
    */
    originit = Psf_srvblk->psf_srvinit;
    if (!Psf_srvblk->psf_srvinit)
    {
	psf_error(E_PS0205_SRV_NOT_INIT, 0L, PSF_INTERR, &err_code,
	    &psq_cb->psq_error, 0);
	if (! (psq_cb->psq_flag & PSQ_FORCE))
	    return(E_DB_FATAL);
	Psf_srvblk->psf_srvinit = TRUE;	/* So psq_sesfind() won't complain */
	maxerr = E_DB_SEVERE;
    }

    /* If there are sessions open, complain */
    if (Psf_srvblk->psf_nmsess != 0)
    {
	psf_error(E_PS0501_SESSION_OPEN, 0L, PSF_CALLERR, &err_code,
	    &psq_cb->psq_error, 0);
	if ((!psq_cb->psq_flag & PSQ_FORCE))
	{
	    Psf_srvblk->psf_srvinit = originit;
	    return (E_DB_FATAL);
	}
	else
	{
	    maxerr = E_DB_WARN;
	}
    }
    /*
    ** Tell RDF to shutdown.
    */
    rdf_ccb.rdf_fcb = Psf_srvblk->psf_rdfid;
    status = rdf_call(RDF_TERMINATE, (PTR) &rdf_ccb);
    if (status != E_DB_OK)
    {
	(VOID) psf_rdf_error(RDF_TERMINATE, &rdf_ccb.rdf_error,
	    &psq_cb->psq_error);
    }
    /* remove the semaphore before releasing the memory */
    CSr_semaphore(&Psf_srvblk->psf_sem);

    /*
    ** Deallocate the memory pool.
    */
    ulm_rcb.ulm_facility = DB_PSF_ID;
    ulm_rcb.ulm_poolid = Psf_srvblk->psf_poolid;
    status = ulm_shutdown(&ulm_rcb);
    if (status == E_DB_ERROR || status == E_DB_FATAL)
    {
        if (!(psq_cb->psq_flag & PSQ_FORCE))
        {
	    Psf_srvblk->psf_srvinit = originit;
	    return (E_DB_FATAL);
	}
	else
	{
	    if (maxerr != E_DB_FATAL)
		maxerr = status;
	}
    }

    Psf_srvblk = (PSF_SERVBLK*) NULL;
    return    (maxerr);
}
