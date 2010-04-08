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
#define             OPV_DEASSIGN        TRUE
#include    <opxlint.h>
/**
**
**  Name: OPVDEASS.C - deassign a global range variable
**
**  Description:
**      File contains routine which will deassign a global range variable
**
**  History:    
**      1-jul-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
**/

/*{
** Name: opv_deassign	- deassign a global range variable
**
** Description:
**      This routine will deassign a global range variable which was assigned 
**      to an aggregate temporary but is not longer needed because the aggregate
**      will be evaluated together with another aggregate.
**
** Inputs:
**      global                          ptr to global state variable
**      varno                           global range variable number to be
**                                      deassigned.
**
** Outputs:
**      global->ops_range_tab           varno will be deassigned
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	1-jul-86 (seputis)
**          initial creation
[@history_line@]...
*/
VOID
opv_deassign(
	OPS_STATE          *global,
	OPV_IGVARS         varno)
{
    global->ops_rangetab.opv_base->opv_grv[varno] = NULL; /* deallocate the
						** temporary by destroying the
                                                ** the global range table entry
                                                */
    /* FIXME - memory is lost here */
}
