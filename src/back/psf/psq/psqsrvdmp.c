/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <tr.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>

/**
**
**  Name: PSQSRVDMP.C - Functions used to dump the server control block.
**
**  Description:
**      This file contains the functions necessary to dump the server control 
**      block.
**
**          psq_srvdump - Dump the server control block.
**
**
**  History:    $Log-for RCS$
**      02-oct-85 (jeff)    
**          written
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	11-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in psq_srvdump().
[@history_template@]...
**/

/*{
** Name: psq_srvdump	- Dump a server control block.
**
**  INTERNAL PSF call format:	 status = psq_srvdump(&psq_cb);
**
**  EXTERNAL call format:	 status = psq_call(PSQ_SRVDUMP, &psq_cb, NULL);
**
** Description:
**      The psq_srvdump function formats and prints the server control block
**	for the parser facility.  The output will go to the output terminal
**	and/or  file named by the user in the "SET TRACE TERMINAL" and "SET
**	TRACE OUTPUT" commands.
**
** Inputs:
**      NONE                            No input parameters
**
** Outputs:
**      psq_cb
**          .psq_error                  Filled in if an error happens
**		.err_code		    What error occurred
**		    E_PS0000_OK			Success
**		    E_PS0002_INTERNAL_ERROR	Internal inconsistency in PSF
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Writes to output file and/or terminal as specified in the "set trace
**	    output" and "set trace terminal" commands.
**
** History:
**	02-oct-85 (jeff)
**          written
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	11-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
*/
DB_STATUS
psq_srvdump(
	register PSQ_CB    *psq_cb)
{
    STATUS              status;
    DB_STATUS           dbstatus;
    i4		err_code;
    extern PSF_SERVBLK	*Psf_srvblk;

    if ((status = TRdisplay("Server Control Block at 0x%p:\n", Psf_srvblk))
	!= TR_OK)
    {
	return (E_DB_ERROR);
    }

    if ((dbstatus = psq_headdmp((PSQ_CBHEAD *) Psf_srvblk)) != E_DB_OK)
    {
	return (E_DB_ERROR);
    }

    if ((status = TRdisplay("\n\tpsf_srvinit:\t")) != TR_OK)
    {
	return (E_DB_ERROR);
    }

    if ((dbstatus = psq_booldmp(Psf_srvblk->psf_srvinit)) != E_DB_OK)
    {
	return (E_DB_ERROR);
    }

    if ((status = TRdisplay("\n\tpsf_version:\t%d", Psf_srvblk->psf_version)) !=
	 TR_OK)
    {
	return (E_DB_ERROR);
    }

    if ((status = TRdisplay("\n\tpsf_nmsess:\t%d", Psf_srvblk->psf_nmsess)) !=
	TR_OK)
    {
	return (E_DB_ERROR);
    }

    if ((status = TRdisplay("\n\tpsf_mxsess:\t%d", Psf_srvblk->psf_mxsess)) !=
	TR_OK)
    {
	return (E_DB_ERROR);
    }

    if ((status = TRdisplay("\n\tpsf_memleft:\t%d", Psf_srvblk->psf_memleft)) !=
	TR_OK)
    {
	return (E_DB_ERROR);
    }

    if ((status = TRdisplay("\n\tpsf_poolid:\t%d", Psf_srvblk->psf_poolid)) !=
	TR_OK)
    {
	return (E_DB_ERROR);
    }

    if ((status = TRdisplay("\n\tpsf_rdfid:\t%d", Psf_srvblk->psf_rdfid)) !=
	TR_OK)
    {
	return (E_DB_ERROR);
    }

    if ((status = TRdisplay("\n\tpsf_lang_allowed:\t")) != TR_OK)
    {
	return (E_DB_ERROR);
    }

    if ((status = TRdisplay("%v", "QUEL,SQL", Psf_srvblk->psf_lang_allowed)) !=
	TR_OK)
    {
	return (E_DB_ERROR);
    }

    return    (E_DB_OK);
}
