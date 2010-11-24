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
#define             OPN_COINSERT        TRUE
#include    <opxlint.h>
/**
**
**  Name: OPNCOINS.C - insert co struct into subtree list
**
**  Description:
**      routine to insert CO struct into subtree list
**
**  History:    
**      29-may-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void opn_coinsert(
	OPN_SUBTREE *sbtp,
	OPO_CO *cop);

/*{
** Name: opn_coinsert	- insert co struct into subtree list
**
** Description:
**      Insert cost ordering structure into the cost ordering set
**
** Inputs:
**      sbtp                            subtree representing cost ordering set
**      cop                             ptr to cost ordering
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
**	29-may-86 (seputis)
**          initial creation
{@history_line@}
[@history_line@]...
*/
VOID
opn_coinsert(
	OPN_SUBTREE        *sbtp,
	OPO_CO             *cop)
{
    cop->opo_coforw = sbtp->opn_coforw;
    sbtp->opn_coforw->opo_coback = cop;
    cop->opo_coback = (OPO_CO *) sbtp;
    sbtp->opn_coforw = cop;
}
