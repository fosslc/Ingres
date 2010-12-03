/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
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
#define        OPO_FORDERING	TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>

/**
**
**  Name: OPOFORDR.C - {@comment_text@}
**
**  Description:
**
**
**
**  History:    
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	15-sep-93 (swm)
**	    Moved cs.h include above other header files which need its
**	    definition of CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
OPO_ISORT opo_fordering(
	OPS_SUBQUERY *subquery,
	OPE_BMEQCLS *eqcmap);

/*{
** Name: opo_fordering	- find or create multi-attribute ordering
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
*/
OPO_ISORT
opo_fordering(
	OPS_SUBQUERY       *subquery,
	OPE_BMEQCLS        *eqcmap)
{
    OPO_ISORT           ordering;	/* number used to represent
					** multi-attribute ordering */
    OPO_SORT		*orderp;        /* ptr to multi-attribute
					** ordering descriptor */
    OPE_IEQCLS		maxeqcls;

    maxeqcls = subquery->ops_eclass.ope_ev;
    ordering = BTnext( (i4)-1, (char *)eqcmap, (i4)maxeqcls);
    if (ordering < 0)
	return(OPE_NOEQCLS);
    if ( BTnext( (i4)ordering, (char *)eqcmap, (i4)maxeqcls) < 0)
	return(ordering);		/* only one equivalence class
					** so use it as the ordering */
    
    if (!subquery->ops_msort.opo_base)
	opo_iobase(subquery);
    {	/* search existing list for the multi-attribute ordering */
	i4		maxorder;

	maxorder = subquery->ops_msort.opo_sv;
	for( ordering = 0; ordering < maxorder; ordering++)
	{
	    orderp = subquery->ops_msort.opo_base->opo_stable[ordering];
	    if ( orderp->opo_stype == OPO_SINEXACT)
	    {
		if (BTsubset((char *)eqcmap, (char *)orderp->opo_bmeqcls,
			(i4)maxeqcls)
		    &&
		    BTsubset((char *)orderp->opo_bmeqcls, (char *)eqcmap,
			(i4)maxeqcls))
		    return((OPO_ISORT)(ordering+maxeqcls)); /* correct ordering found 
						** - FIXME need a bitwise
						** equality */
	    }
	}
    }
    /* create new ordering since current one was not found */
    ordering = opo_gnumber(subquery);	/* get the next multi-attribute
					** ordering number */
    orderp = (OPO_SORT *)opn_memory(subquery->ops_global, (i4)sizeof(*orderp));
    orderp->opo_stype = OPO_SINEXACT;
    orderp->opo_bmeqcls = (OPE_BMEQCLS *)opn_memory(subquery->ops_global, (i4)sizeof(OPE_BMEQCLS));
    MEcopy((PTR)eqcmap, sizeof(OPE_BMEQCLS), (PTR)orderp->opo_bmeqcls);
    orderp->opo_eqlist = NULL;
    subquery->ops_msort.opo_base->opo_stable[ordering] = orderp;
    return((OPO_ISORT)(ordering+maxeqcls));
}
