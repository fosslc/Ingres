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
#define        OPU_DEALLOCATE      TRUE
#include    <opxlint.h>

/**
**
**  Name: OPUDEALLOCATE.C - deallocate a stream
**
**  Description:
**      routine to deallocate a stream
**
**  History:    
**      16-jun-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void opu_deallocate(
	OPS_STATE *global,
	PTR *streamid);

/*{
** Name: opu_deallocate	- deallocate a memory stream
**
** Description:
**      This routine will deallocate a memory stream.
**
** Inputs:
**      global                          ptr to global state variable
**      streamid                        ptr to streamid to deallocate
**
** Outputs:
**      streamid                        deallocate streamid will be set to NULL
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-jun-86 (seputis)
**          initial creation
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
[@history_line@]...
*/
VOID
opu_deallocate(
	OPS_STATE          *global,
	PTR		   *streamid)
{
    DB_STATUS      ulmstatus;		    /* return status from ULM */
    global->ops_mstate.ops_ulmrcb.ulm_streamid_p = streamid; /*deallocate proper streamid */
    if ((ulmstatus = ulm_closestream(&global->ops_mstate.ops_ulmrcb)) != E_DB_OK)
# ifdef E_OP0080_ULM_CLOSESTREAM
	opx_verror( ulmstatus, E_OP0080_ULM_CLOSESTREAM,
	    global->ops_mstate.ops_ulmrcb.ulm_error.err_code) /* report error if found */
# endif
	;
    /* ULM has nullified *streamid */
}
