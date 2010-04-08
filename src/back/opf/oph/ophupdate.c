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
#define             OPH_UPDATE          TRUE
#include    <opxlint.h>
/**
**
**  Name: OPHUPDATE.C - update the histogram with new selectivity
**
**  Description:
**      Routines used to update and normalize a list of histograms associated
**      with a boolean factor
**
**  History:    
**      25-may-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
**/

/*{
** Name: oph_normalize	- normalize a histogram
**
** Description:
**	Normalize the histogram to have an area of newarea
**
**   we know that:
**
**	   sum pi = 1
**   and   sum pi*fi = oldarea
**
**    so    sum pi((1-newarea)/(1-oldarea) * fi + (newarea-oldarea)/(1-oldarea))
**        = (1-newarea)/(1-oldarea) * sum pi*fi +   
**					(newarea-oldarea)/(1-oldarea) * sum pi
**        = (1-newarea)/(1-oldarea) * oldarea   + (newarea-oldarea)/(1-oldarea)
**        = (oldarea-newarea*oldarea+newarea-oldarea)/(1-oldarea)
**        = newarea*(1-oldarea)/(1-oldarea)
**        = newarea
**   
**    similarly:
**   
**   	sum pi*(newarea/oldarea * fi) 
**        = newarea/oldarea * sum pi*fi
**        = newarea/oldarea * oldarea
**        = newarea
**
**	These formulas are used to solve the 2 cases existant when this
**	is called: new area > oldarea ==> an OR has happened, in which case
**	all cells now have some selectivity (because column values previously
**	excluded because of restrictions implemented through this histogram
**	may now be included because of ORed preds on independent columns), or
**	newarea < oldarea ==> an AND has happened in which case cells which
**	previously had no selectivity will continue to have no selectivity.
**	This is the reason for the separate multiplicative and additive 
**	factors used to update the fcnt's, and why there are separate 
**	calculations used depending on the relative values of oldarea and
**	newarea. I do hope this helps.
**
**
**	Called by:	oph_update
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      histp                           ptr to histogram to be updated
**      newarea                         new area for histogram
**
** Outputs:
**      histp                           histogram is updated to new selectivity
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-may-86 (seputis)
**          initial creation
**	17-apr-97 (inkdo01)
**	    Updated the hopeless comments so that future readers won't pull
**	    their hair out like I did, trying to figure out what's going on.
**	20-oct-00 (inkdo01)
**	    Added support for composite histograms.
[@history_line@]...
*/
VOID
oph_normalize(
	OPS_SUBQUERY       *subquery,
	OPH_HISTOGRAM      *histp,
	OPN_PERCENT        newarea)
{
    OPN_PERCENT         oldarea;	/* area (selectivity) prior to applying
                                        ** predicate which resulted in
                                        ** selectivity equal to newarea */
    OPN_PERCENT         multfactor;     /* each cell is multiplied by this 
                                        ** factor */
    OPN_PERCENT         addfactor;      /* each cell has this factor added to
                                        ** it */
    i4                  cell;		/* current cell being adjusted */

    oldarea = histp->oph_prodarea;	/* current "old area" of histogram */
    if (newarea > OPN_100PC)            /* new selectivity should be between */
	newarea = OPN_100PC;            /*   100 percent  OR ... */
    else if (newarea < OPN_MINPC)
        newarea = OPN_MINPC;		/* the smallest non-zero percentage*/

    /* 
    ** let f be current value of a histogram cell
    **
    ** if new area (newarea) is greater than old area (oldarea) then we
    ** have:
    ** newf = (newarea-oldarea)/(1-oldarea) * (1-f) + f
    ** if oldarea > newarea then:
    ** newf = newarea/oldarea * f
    ** futzing around with these equations gives the multfactor and addfactor
    ** factors below
    */
    if (newarea > oldarea)
    {
	multfactor = (OPN_D100PC - newarea) / (OPN_D100PC - oldarea);
	addfactor = (newarea - oldarea) / (OPN_D100PC - oldarea);
    }
    else
    {
	multfactor = newarea / oldarea;
	addfactor = OPN_0PC;
    }
    histp->oph_prodarea = newarea;	/* reset to the newarea value */

    cell = (histp->oph_mask & OPH_COMPOSITE) ? 
		histp->oph_composite.oph_interval->oph_numcells :
		subquery->ops_attrs.opz_base->opz_attnums[histp->oph_attribute]
	->opz_histogram.oph_numcells;
    while( cell-- > 1 )			/* for each cell */
	histp->oph_fcnt[cell] = histp->oph_fcnt[cell] * multfactor + addfactor;
					/* normalize it */
}
