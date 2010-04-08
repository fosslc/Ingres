/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <ex.h>
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
#include    <scf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>

/**
**
**  Name: PSFSESSCB.C - get the session control block
**
**  Description:
**      The routines in this file are used to get the PSF session control block
**  from SCF. 
**
**          psf_sesscb - get the session control block.
**
**
**  History:    $Log-for RCS$
**      8-oct-86 (daved)
**          written
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	10-Jan-2001 (jenjo02)
**	    Use (new) GET_PSS_SESBLK macro instead of
**	    scf_call(SCU_INFORMATION) to return session's
**	    PSS_SESBLK*.
[@history_line@]...
[@history_template@]...
**/


/*{
** Name: PSF_SESSCB	- get session control block.
**
** Description:
**      Get the session control block from SCF. 
**
** Inputs:
**	none
**
** Outputs:
**	Returns:
**          sess_cb                     session control block ptr
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      8-oct-86 (daved)
**          written
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-aug-93 (andre)
**	    removed declaration of scf_call()
**	08-sep-93 (swm)
**	    Changed sizeof(DB_SESSID) to sizeof(CS_SID) to reflect recent CL
**	    interface revision.
**	10-Jan-2001 (jenjo02)
**	    Use (new) GET_PSS_SESBLK macro instead of
**	    scf_call(SCU_INFORMATION) to return session's
**	    PSS_SESBLK*.
[@history_line@]...
[@history_template@]...
*/
PSS_SESBLK*
psf_sesscb(void)
{
    CS_SID	sid;

    CSget_sid(&sid);
    return(GET_PSS_SESBLK(sid));
}
