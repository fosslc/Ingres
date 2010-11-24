/*
**Copyright (c) 2004, 2010 Ingres Corporation
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
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>

/**
**
**  Name: PSQVERS.C - Functions used to get the parser version number
**
**  Description:
**      This file contains the functions necessary to return the parser version 
**      number.
**
**          psq_version - Return the parser version number
**
**
**  History:    $Log-for RCS$
**      01-oct-85 (jeff)    
**          written
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	06-nov-1992 (rog)
**	    <ulf.h> must be included before <qsf.h>.
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 psq_version(
	PSQ_CB *psq_cb);

/*{
** Name: psq_version	- Get the parser version number
**
**  INTERNAL PSF call format: status = psq_version(&psq_cb);
**
**  EXTERNAL call format:     status = psq_call(PSQ_VERSION, &psq_cb, NULL);
**
** Description:
**      The psq_version function returns the version number of the parser.
**	This should be useful for compatibility; PSF will be able to tell
**	whether the parser supports certain features by the parser's version
**	number.
**
** Inputs:
**      psq_cb
**          NONE                        No Input Parameters
**	sess_cb				Ignored
**
** Outputs:
**      psq_cb
**          .psq_version                The version number of the parser
**          .psq_error                  Error information
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
**	    none
**
** History:
**	01-oct-85 (jeff)
**          written
**      02-sep-86 (seputis)
**          psq_cb.psq_version becomes psq_cb->psq_version
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/
DB_STATUS
psq_version(
	PSQ_CB             *psq_cb)
{
    extern PSF_SERVBLK  *Psf_srvblk;

    psq_cb->psq_version = Psf_srvblk->psf_version;
    return    (E_DB_OK);
}
