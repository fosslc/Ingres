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
#define             OPH_PDF             TRUE
#include    <opxlint.h>
/**
**
**  Name: OPHPDF.C - make histogram integrate to one
**
**  Description:
**      Routine to make histogram integrate to one
**
**  History:    
**      27-may-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
**/

/*{
** Name: oph_histpdf	- make histogram a probability density function
**
** Description:
**      Make the histogram a probability density function (which integrates
**      to one).
**
**      i.e. This routine will transform the histogram cells to be a 
**      percentage of the number of tuples in the relation rather than a 
**      percentage of the respective cell counts.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      histp                           ptr to histogram to be transformed
**                                      into probability density function
**
** Outputs:
**      histp->oph_fnct                 now contains probability density
**                                      function
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	27-may-86 (seputis)
**          initial creation
**	20-oct-00 (inkdo01)
**	    Support for composite histograms.
[@history_line@]...
*/
VOID
oph_pdf(
	OPS_SUBQUERY       *subquery,
	OPH_HISTOGRAM      *histp)
{
    i4                  numcells;	/* number of cells in histogram */
    OPN_PERCENT         totalarea;      /* percentage of relation selected
                                        ** by this histogram */
    i4                  cell;           /* current cell being analyzed */

    numcells = (histp->oph_mask & OPH_COMPOSITE) ? 
		histp->oph_composite.oph_interval->oph_numcells :
		subquery->ops_attrs.opz_base->opz_attnums[histp->oph_attribute]->
	opz_histogram.oph_numcells;
    totalarea = OPN_0PC;                /* start with 0 percent selected */

    for (cell = numcells; cell-- > 1;)
    {
	/*
	**	Changed due to SUN and 3B20 compiler bugs:
	**	totalarea += (histp->oph_fcnt[cell] *= histp->oph_pcnt[cell]);
	*/
	histp->oph_fcnt[cell] *= histp->oph_pcnt[cell];
	totalarea += histp->oph_fcnt[cell];
    }
    histp->oph_null *= histp->oph_pnull;    /* add contribution for NULLs */
    totalarea += histp->oph_null;

    if (totalarea > 0.0)
    {
	f4                    factor;	    /* factor to increase each cell */

	factor = 1.0 / totalarea;	    /* multiply each cell be this amount
                                            ** so that histogram integrates to
                                            ** one */
	for (cell = numcells; cell-- > 1;)
	    histp->oph_fcnt[cell] *= factor;
	histp->oph_null *= factor;
    }
}
