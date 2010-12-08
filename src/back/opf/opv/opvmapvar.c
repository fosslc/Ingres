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
#define        OPV_MAPVAR      TRUE
#include    <bt.h>
#include    <opxlint.h>


/**
**
**  Name: OPVMAPVAR.C - map the query tree fragment
**
**  Description:
**      Contains routine which will update the varmap of a query tree fragment
**
**  History:    
**      3-jul-86 (seputis)    
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void opv_mapvar(
	PST_QNODE *nodep,
	OPV_GBMVARS *map);

/*{
** Name: opv_mapvar	- update varmap of a query tree fragment
**
** Description:
**      This routine will recursively descent the query tree and update the 
**      varmap i.e. bitmap of all range variables referenced in this query tree
**      given to this routine.
**
** Inputs:
**      nodep                           ptr to current query tree node being
**                                      analyzed
**       map                            ptr to bitmap of vars to be updated
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
**	3-jul-86 (seputis)
**          initial creation
**     28-jan-04 (wanfr01)
**        Bug 36414, INGSRV582
**        Constant node doesn't need it's children recursed;
**        all children will also be constants.
[@history_line@]...
*/
VOID
opv_mapvar(
	PST_QNODE          *nodep,
	OPV_GBMVARS        *map)
{
    while (nodep)
    {
	if (nodep->pst_sym.pst_type == PST_VAR)
	{
	    /* var node found so set bit map */
	    BTset( (i4)nodep->pst_sym.pst_value.pst_s_var.pst_vno,
		   (char *)map );
	    return;
	}
	if ((nodep->pst_left) && (nodep->pst_sym.pst_type != PST_CONST))
	    opv_mapvar( nodep->pst_left, map); /* recurse down left */
	nodep = nodep->pst_right;	/* iterate down right side */
    };
}
