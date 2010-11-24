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
#define             OPN_COST            TRUE
#include    <fp.h>
#include    <opxlint.h>
/**
**
**  Name: OPNCOST.C - evaluate cost of tree
**
**  Description:
**      This routine will evaluate the cost of a tree as a function
**      of the cpu and disk i/o
**
**  History:    
**      29-may-86 (seputis)    
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
OPO_COST opn_cost(
	OPS_SUBQUERY *subquery,
	OPO_BLOCKS dio,
	OPO_CPU cpu);

/*{
** Name: opn_cost	- cost of tree
**
** Description:
**      This routine will evaluate the cost of a tree as a function
**      of the cpu and disk i/o
**
** Inputs:
**      dio                             disk i/o required to produce the tree
**      cpu                             cpu activity required to produce tree
**
** Outputs:
**	Returns:
**	    relative "cost" of the tree
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	29-may-86 (seputis)
**          initial creation
**	29-feb-2008 (dougi)
**	    Verify that the resulting value is ok - if not, return a big number.
*/
OPO_COST
opn_cost(
	OPS_SUBQUERY       *subquery,
	OPO_BLOCKS         dio,
	OPO_CPU            cpu)
{
    OPO_COST	cost = dio + (cpu / subquery->ops_global->ops_cb->ops_alter.
						ops_cpufactor);

    if (!FPffinite(&cost))
	return(1e36);
    else return(cost);
}
