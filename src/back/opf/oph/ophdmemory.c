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
#define             OPH_DMEMORY         TRUE
#include    <opxlint.h>
/**
**
**  Name: OPHDMEMORY.C - deallocate memory for a histogram
**
**  Description:
**      routine to deallocate memory for a histogram
**
**  History:    
**      13-jun-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
**/

/*{
** Name: oph_dmemory	- deallocate a histogram element
**
** Description:
**      This routine will return a histogram element back to the free list
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      histp                           ptr to histogram element to deallocate
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
**	13-jun-86 (seputis)
**          initial creation
**	9-oct-01 (inkdo01)
**	    Unbelievably - fix 2nd line of code to add current hist to free 
**	    chain. Ever since written, this function essentially did nothing
**	    because of this bug.
[@history_line@]...
*/
VOID
oph_dmemory(
	OPS_SUBQUERY       *subquery,
	OPH_HISTOGRAM      *histp)
{
    /* place deallocated memory into histogram free list */
    histp->oph_next = subquery->ops_global->ops_estate.opn_hfree;
    subquery->ops_global->ops_estate.opn_hfree = histp;
}
