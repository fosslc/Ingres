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
#define             OPH_DCCMEMORY       TRUE
#include    <opxlint.h>
/**
**
**  Name: OPHDCCME.C - routine to deallocate a cell count array
**
**  Description:
**      Routine to deallocate a cell count array
**
**  History:    
**      16-jun-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
**/

/*{
** Name: oph_dccmemory	- deallocate cell count array
**
** Description:
**      Routine to deallocate a cell count array
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      intervalp                       ptr to histogram component of an
**                                      attribute describing characteristics
**                                      of the histogram count array
**                                      oph_fnct component
**	counts                          memory to OPH_COUNT cell count 
**                                      component of this histogram structure.
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
**	16-jun-86 (seputis)
**          initial creation
**	17-mar-06 (dougi)
**	    Only place arrays onto free list if they are big enough to
**	    support the OPH_COERCE overlay. This should fix a CPU loop in
**	    oph_ccmemory() chasing the free chain.
[@history_line@]...
*/
VOID
oph_dccmemory(
	OPS_SUBQUERY       *subquery,
	OPH_INTERVAL       *intervalp,
	OPH_COUNTS	   counts)
{
    OPH_COERCE		*coercep;   /* coercion use to traverse linked list*/
    i4			size;

    if (sizeof(OPH_COERCE) > (size = intervalp->oph_numcells *
						sizeof(OPN_PERCENT)))
	return;

    coercep = (OPH_COERCE *) counts;
    coercep->oph_size = size;
    coercep->oph_next = 
	(OPH_COERCE *)subquery->ops_global->ops_estate.opn_ccfree;
    subquery->ops_global->ops_estate.opn_ccfree = (OPH_COUNTS) coercep;
}
