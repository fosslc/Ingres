/*
**Copyright (c) 2004 Ingres Corporation
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
#define             OPN_DRMEMORY        TRUE
#include    <opxlint.h>
/**
**
**  Name: OPNDRMEM.C - return OPN_RLS structure to memory manager
**
**  Description:
**      routine to return OPN_RLS structure to memory manager
**
**  History:    
**      29-may-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
**/

/*{
** Name: opn_drmemory	- return OPN_RLS structure to memory manager
**
** Description:
**      The routine will free an OPN_RLS structure
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      rlsp                            ptr to OPN_RLS structure to return
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	28-may-86 (seputis)
**          initial creation
**	19-nov-01 (inkdo01)
**	    Tidy up the histogram chain ptr.
**	21-feb-02 (inkdo01)
**	    Clear down ptr to OPN_EQS (for safety).
[@history_line@]...
*/
VOID
opn_drmemory(
	OPS_SUBQUERY       *subquery,
	OPN_RLS		   *rlsp)
{
    if (rlsp)
    {
 	rlsp->opn_histogram = NULL;
	rlsp->opn_rlnext = subquery->ops_global->ops_estate.opn_rlsfree;
	subquery->ops_global->ops_estate.opn_rlsfree = rlsp;
	rlsp->opn_eqp = NULL;
    }
}
