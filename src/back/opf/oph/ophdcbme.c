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
#define             OPH_DCBMEMORY       TRUE
#include    <opxlint.h>

/**
**
**  Name: OPHDCBME.C - deallocate cell boundary memory
**
**  Description:
**      routines to deallocate cell boundary arrays
**
**  History:    
**      13-jun-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void oph_dcbmemory(
	OPS_SUBQUERY *subquery,
	OPH_CELLS cellptr);

/*{
** Name: opn_dcbmemory	- deallocate cell boundary memory
**
** Description:
**      This routine will deallocate the temporary cell boundary memory array. 
**      It does not do anything now but is put here for consistency when the
**      memory management routines have deallocation.
**
** Inputs:
**      subquery                        ptr to subquery being analzyed
**      cbptr                           ptr to cell boundary element to
**                                      deallocate
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
[@history_line@]...
*/
VOID
oph_dcbmemory(
	OPS_SUBQUERY       *subquery,
	OPH_CELLS          cellptr)
{
}
