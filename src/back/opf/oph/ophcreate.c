/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

/*
NO_OPTIM =  sos_us5 i64_aix
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
#define        OPH_CREATE      TRUE
#include    <opxlint.h>

/**
**
**  Name: OPHCREATE.C - create a normalized histogram
**
**  Description:
**      routines which create a histogram
**
**  History:    
**      24-may-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	02-Jun-1995 (duursma)
**	    Changed for loop at end of routine to also initialize
**	    elements 0 and 1.  The fact that element 1 was never
**	    initialized led to NaN exceptions later, eg. in oph_sarg()
**	    in ophrelse.c
**	12-jan-96 (inkdo01)
**	    Added support for per-cell repetition factors.
**	02-jul-97 (allmi01)
**	    NO_OPTIM for sos_us5 for sigv during createdb -S iidbdb.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
OPH_HISTOGRAM *oph_create(
	OPS_SUBQUERY *subquery,
	OPH_HISTOGRAM *hp,
	OPN_PERCENT initfact,
	OPE_IEQCLS eqcls);

/*{
** Name: oph_create	- create a normalized histogram
**
** Description:
**      This routine creates a normalized histogram of the given
**      selectivity.  The previous predicates applied to the node
**      has produce a certain percentage selectivity.  This routine
**      will assume independence and apply an equal selectivity to
**      all the cells.  A cell will contain a percentage of the
**      original cell count rather than a percentage of the tuple count.
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**	02-Jun-1995 (duursma)
**	    Changed for loop at end of routine to also initialize
**	    elements 0 and 1.  The fact that element 1 was never
**	    initialized led to NaN exceptions later, eg. in oph_sarg()
**	    in ophrelse.c
**	3-oct-00 (inkdo01)
**	    Support for composite histograms - if composite histogram, avoid
**	    eqtamp call and de-ref through attribute no.
*/
OPH_HISTOGRAM *
oph_create(
	OPS_SUBQUERY       *subquery,
	OPH_HISTOGRAM      *hp,
	OPN_PERCENT        initfact,
	OPE_IEQCLS         eqcls)
{
    OPH_HISTOGRAM       *originalhp;	/* histogram element which contains
                                        ** the original cell counts for 
                                        ** the equivalence class eqcls
                                        */
    OPH_HISTOGRAM       *newhp;         /* new histogram created which will
                                        ** contain the same number of cells
                                        ** etc. as orignalhp , but the
                                        ** cell counts will be percentages
                                        ** of the respective primaryhp cells
                                        ** Thus, all cells will initially
                                        ** have the same value i.e.
                                        ** "initfact"
                                        */
    OPZ_ATTS		*ap;            /* ptr to attribute associated with
                                        ** the histogram
                                        */
    OPH_INTERVAL	*intervalp;	/* ptr to value/count part of histo
					*/
    bool		composite = FALSE; /* composite histogram indicator
					*/
    if (eqcls == OPE_NOEQCLS)
    {
	composite = TRUE;
	originalhp = hp;
	intervalp = hp->oph_composite.oph_interval;
    }
    else 
    {
	originalhp = *opn_eqtamp (subquery, &hp, eqcls, TRUE); /* find histogram 
                                        ** for the equivalence class in the
                                        ** list provided
                                        */
	ap = subquery->ops_attrs.opz_base->opz_attnums[originalhp->oph_attribute];
					/* get original attribute for this 
					** equivalence class */
	intervalp = &ap->opz_histogram;
    }

    newhp = oph_memory(subquery);	/* get a new
                                        ** histogram pointer from the list
                                        ** associated with this attribute
                                        */
    STRUCT_ASSIGN_MACRO(*originalhp, *newhp); /* make a copy of the original
                                        ** histogram*/
    newhp->oph_fcnt = oph_ccmemory(subquery, intervalp); /* allocate
                                        ** a cell count array */
    newhp->oph_creptf = oph_ccmemory(subquery, intervalp); /* allocate
                                        ** a repetition factor array */
    newhp->oph_next = NULL;             /* list is unknown at this time */
    newhp->oph_pcnt = originalhp->oph_fcnt; /* save ptr to histogram which
                                        ** contains cell counts which are
                                        ** percentages of tuples in the relation
                                        */
    newhp->oph_pnull = originalhp->oph_null; /* save NULL count */
    newhp->oph_prodarea = initfact;	/* normalized area of this histogram */
    {	/* for each cell in the histogram initialize it to current selectivity*/
	i4		        cell;   /* index into the histogram cell count
                                        ** array
                                        */
	for (cell = 0; cell < intervalp->oph_numcells; cell++)
	{
	    newhp->oph_fcnt[cell] = initfact;
	    newhp->oph_creptf[cell] = originalhp->oph_creptf[cell];
					/* copy creptf's asis */
	}
	newhp->oph_null = initfact;
    }
    return (newhp);
}
