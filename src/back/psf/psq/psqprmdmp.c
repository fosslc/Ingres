/*
**Copyright (c) 2004, 2010 Ingres Corporation
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
**  Name: PSQPRMDMP.C - Functions used to dump the parameters in a control
**		    block.
**
**  Description:
**      This file contains the functions necessary to dump the parameters 
**      contained in a PSQ_CB.
**
**          psq_prmddump - Dump the parameters in a control block.
**
**
**  History:
**      02-oct-85 (jeff)    
**          written
**	13-sep-90 (teresa)
**	    changed to dump PSQ_FLAG fields.  Flag pqs_force has changed to
**	    be a bitflag instead of a boolean.  Also, many new flags have been
**	    added and some of those are in psq_flag.  Now all psq_flag bits 
**	    will be dumped.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	04-aug-93  (shailaja)
**	    Fixed QAC warnings.
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	17-dec-93 (rblumer)
**	    removed all FIPS_MODE flag references.
**	11-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in psq_prmdump().
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 psq_prmdump(
	register PSQ_CB *psq_cb);

/*{
** Name: psq_prmdump	- Dump the parameters in a control block.
**
**  INTERNAL PSF call format: status = psq_prmdump(&psq_cb)
**
**  EXTERNAL call format:     status = psq_call(PSQ_PRMDUMP, &psq_cb, NULL);
**
** Description:
**	The psq_prmdump function formats and prints the given control block.
**	The output will go to the output terminal and/or file named by the
**	user in the "SET TRACE TERMINAL" and "SET TRACE OUTPUT" commands.
**
** Inputs:
**      psq_cb                          Everything in the control block gets
**					printed.
**	sess_cb				Ignored.
**
** Outputs:
**	NONE				Nothing in the control block will be
**					altered by this command, even if there
**					is an error.
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Sends output to terminal and/or log file
**
** History:
**	02-oct-85 (jeff)
**          written
**	13-sep-90 (teresa)
**	    changed to dump PSQ_FLAG fields.  Flag pqs_force has changed to
**	    be a bitflag instead of a boolean.  Also, many new flags have been
**	    added and some of those are in psq_flag.  Now all psq_flag bits 
**	    will be dumped.
**	08-sep-93 (swm)
**	    Changed cast of session id parameter to psq_siddmp() function
**	    to CS_SID.
**	17-dec-93 (rblumer)
**	    "FIPS mode" no longer exists.  It was replaced some time ago by
**	    several feature-specific flags (e.g. flatten_nosingleton and
**	    direct_cursor_mode).  So I removed all FIPS_MODE flags.
**	11-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
*/
DB_STATUS
psq_prmdump(
	register PSQ_CB     *psq_cb)
{
    DB_STATUS		  status;

    if (TRdisplay("Contents of PSQ_CB at 0x%p:\n", psq_cb) != TR_OK)
	return (E_DB_ERROR);
    /* Print common header for all cb's */
    if ((status = psq_headdmp((PSQ_CBHEAD *) psq_cb)) != E_DB_OK)
	return (status);
    if (TRdisplay("\n\tpsq_sessid:\t") != TR_OK)
	return (E_DB_ERROR);
    /* Print session id */
    if ((status = psq_siddmp((CS_SID *) &psq_cb->psq_sessid)) != E_DB_OK)
	return (status);
    if (TRdisplay("\n\tpsq_qlang:\t") != TR_OK)
	return (E_DB_ERROR);
    /* Print query language id */
    if ((status = psq_lngdmp(psq_cb->psq_qlang)) != E_DB_OK)
	return (status);
    if (TRdisplay("\n\tpsq_decimal:\t") != TR_OK)
	return (E_DB_ERROR);
    /* Print decimal specification */
    if ((status = psq_decdmp(&psq_cb->psq_decimal)) != E_DB_OK)
	return (status);
    if (TRdisplay("\n\tpsq_distrib:\t") != TR_OK)
	return (E_DB_ERROR);
    /* Print distributed specifier */
    if ((status = psq_dstdmp(psq_cb->psq_distrib)) != E_DB_OK)
	return (status);

    /* display bitflags in psq_flags */
    if (TRdisplay("\n\tpsq_flag.PSQ_FORCE:\t") != TR_OK)
	return (E_DB_ERROR);
    if ((status = psq_booldmp(psq_cb->psq_flag & PSQ_FORCE)) != E_DB_OK)
	return (status);
    if (TRdisplay("\n\tpsq_flag.PSQ_IIQRYTEXT:\t") != TR_OK)
	return (E_DB_ERROR);
    if ((status = psq_booldmp(psq_cb->psq_flag & PSQ_IIQRYTEXT)) != E_DB_OK)
	return (status);
    if (TRdisplay("\n\tpsq_flag.PSQ_ALIAS_SET:\t") != TR_OK)
	return (E_DB_ERROR);
    if ((status = psq_booldmp(psq_cb->psq_flag & PSQ_ALIAS_SET)) != E_DB_OK)
	return (status);
    if (TRdisplay("\n\tpsq_flag.PSQ_CATUPD:\t") != TR_OK)
	return (E_DB_ERROR);
    if ((status = psq_booldmp(psq_cb->psq_flag & PSQ_CATUPD)) != E_DB_OK)
	return (status);
    if (TRdisplay("\n\tpsq_flag.PSQ_WARNINGS:\t") != TR_OK)
	return (E_DB_ERROR);
    if ((status = psq_booldmp(psq_cb->psq_flag & PSQ_WARNINGS)) != E_DB_OK)
	return (status);
    if (TRdisplay("\n\tpsq_flag.PSQ_ALLDELUPD:\t") != TR_OK)
	return (E_DB_ERROR);
    if ((status = psq_booldmp(psq_cb->psq_flag & PSQ_ALLDELUPD)) != E_DB_OK)
	return (status);
    if (TRdisplay("\n\tpsq_flag.PSQ_DBA_DROP_ALL:\t") != TR_OK)
	return (E_DB_ERROR);
    if ((status = psq_booldmp(psq_cb->psq_flag & PSQ_DBA_DROP_ALL)) != E_DB_OK)
	return (status);

    if (TRdisplay("\n\tpsq_version:\t%d", psq_cb->psq_version) != TR_OK)
	return (E_DB_ERROR);
    if (TRdisplay("\n\tpsq_mode:\t") != TR_OK)
	return (E_DB_ERROR);
    /* Print the query mode */
    if ((status = psq_modedmp(psq_cb->psq_mode)) != E_DB_OK)
	return (status);
/*
**    if (TRdisplay("\n\tpsq_qryid:\t") != TR_OK)
**	  return (E_DB_ERROR);
** {Print the query id} 
**    if ((status = psq_qiddmp(&psq_cb->psq_qryid)) != E_DB_OK)
**        return (status);
**
*/
    if (TRdisplay("\n\tpsq_table:\t") != TR_OK)
	return (E_DB_ERROR);
    /* Print the id of the given table */
    if ((status = psq_tbiddmp(&psq_cb->psq_table)) != E_DB_OK)
	return (status);
    if (TRdisplay("\n\tpsq_cursid:\t") != TR_OK)
	return (E_DB_ERROR);
    /* Print the cursor id */
    if ((status = psq_ciddmp(&psq_cb->psq_cursid)) != E_DB_OK)
	return (status);
/*
**    if (TRdisplay("\n\tpsq_result:\t") != TR_OK)
**        return (E_DB_ERROR);
**    {Print the id for the parse result }
**    if ((status = psq_qiddmp(&psq_cb->psq_result)) != E_DB_OK)
**        return (status);
**
*/
    if (TRdisplay("\n\tpsq_error:\t:") != TR_OK)
	return (E_DB_ERROR);
    /* Print the error block */
    if ((status = psq_errdmp(&psq_cb->psq_error)) != E_DB_OK)
	return (status);

    return    (E_DB_OK);
}
