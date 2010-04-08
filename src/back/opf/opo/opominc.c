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
#define        OPO_MINCOCOST      TRUE
#include    <opxlint.h>


/**
**
**  Name: OPOMINC.C - find minimum cost ordering  cost
**
**  Description:
**      Routine to find the minimum cost ordering cost in a cost ordering set
**
**  History:    
**      6-jun-86 (seputis)    
**          initial creation from mincocost.c
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
**/

/*{
** Name: opo_mincocost	- minimum cost of a cost ordering set
**
** Description:
**	Find minimum cost co in this list, ignore co's that have
**	costs greater than subquery->ops_cost, are originals or are marked
**	to be deleted.
**	If two nodes have equal cost, but one node is a sort node and other is
**      not, make the sort node more expensive.
**
**	Called by:	opn_calcost, opn_prrjoin
**
** Inputs:
**      subtp                           ptr to cost ordering set
**
** Outputs:
**      mincost                         minimum cost of this cost ordering set
**	Returns:
**	    ptr to minimum cost co from this cost ordering set
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	6-jun-86 (seputis)
**          initial creation from mincocost
[@history_line@]...
*/
OPO_CO *
opo_mincocost(
	OPS_SUBQUERY       *subquery,
	OPN_SUBTREE	   *subtp,
	OPO_COST           *mcost)
{
    OPO_COST            mincost;		/* minimum cost so far */
    OPO_CO              *mincop;                /* ptr to minimum cost ordering
                                                ** element found so far */
    OPO_CO              *cop;                   /* current cost ordering element
                                                ** of cost ordering set being
                                                ** analyzed */
    mincost = *mcost;
    mincop = NULL;

    for (cop = (OPO_CO *)subtp->opn_coforw; 
	 cop != (OPO_CO *)subtp; 
	 cop = cop->opo_coforw)
    {
	OPO_COST               copcost;		/* cost of current cost ordering
                                                ** being analyzed */
	if ((	/* cost is minimum so far or avoids a sort node */
		(   (copcost = opn_cost(subquery, cop->opo_cost.opo_dio, 
				     cop->opo_cost.opo_cpu)
		    )
		    < 
		    mincost
		)
		||
		(
		    (cop->opo_sjpr != DB_RSORT)
		    &&
		    (copcost == mincost)
		)
	    )
	    &&
	    (cop->opo_sjpr != DB_ORIG)
	    &&
	    !cop->opo_deleteflag
	)
	{   /* save info on number minimum cost member of set */
	    mincost	= copcost;
	    mincop	= cop;
	}
    }

    *mcost	= mincost;

    return (mincop);
}
