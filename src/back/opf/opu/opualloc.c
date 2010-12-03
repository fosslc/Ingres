/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
/* beginning of optimizer header files */
#include    <opglobal.h>
#include    <opdstrib.h>
#include    <opfcb.h>
#include    <opgcb.h>
#include    <opscb.h>
#include    <ophisto.h>
#include    <opboolfact.h>
#include    <oplouter.h>
#include    <opeqclass.h>
#include    <opcotree.h>
#include    <opvariable.h>
#include    <opattr.h>
#include    <openum.h>
#include    <opagg.h>
#include    <opmflat.h>
#include    <opcsubqry.h>
#include    <opsubquery.h>
#include    <opcstate.h>
#include    <opstate.h>

/* external routine declarations definitions */
#define        OPU_ALLOCATE      TRUE
#include    <opxlint.h>

/**
**
**  Name: OPUALLOC.C - allocate a new memory stream
**
**  Description:
**      Routine to allocate a new memory stream
**
**  History:    
**      16-jun-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	15-Aug-1997 (jenjo02)
** 	    New field, ulm_flags, added to ULM_RCB in which to specify the
**	    type of stream to be opened, shared or private.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
PTR opu_allocate(
	OPS_STATE *global);

/*{
** Name: opu_allocate	- allocate a new private memory stream
**
** Description:
**      This routine will allocate a new private memory stream from the ULM
**
** Inputs:
**      global                          ptr to global state variable
**
** Outputs:
**	Returns:
**	    PTR which represents the new memory stream
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-jun-86 (seputis)
**          initial creation
**	11-oct-2006 (hayke02)
**	    Send E_OP0002_NOMEMORY to errlog.log. This change fixes bug 116309.
[@history_line@]...
*/
PTR
opu_allocate(
	OPS_STATE          *global)
{
    DB_STATUS       ulmstatus;		/* return status from ulm */

    /* Tell ULM to return streamid into ulm_streamid */
    global->ops_mstate.ops_ulmrcb.ulm_streamid_p =
	&global->ops_mstate.ops_ulmrcb.ulm_streamid;
    ulmstatus = ulm_openstream(&global->ops_mstate.ops_ulmrcb);
    if (DB_FAILURE_MACRO(ulmstatus))
    {
	opx_lerror(E_OP0002_NOMEMORY, 0, 0, 0, 0, 0);
	opx_verror(ulmstatus, E_OP0002_NOMEMORY, 
	    global->ops_mstate.ops_ulmrcb.ulm_error.err_code);
    }
    return ( global->ops_mstate.ops_ulmrcb.ulm_streamid );
}
