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
#define        OPH_MEMORY      TRUE
#include    <opxlint.h>


/**
**
**  Name: OPHMEMORY.C - get memory for a histogram element
**
**  Description:
**      routines to get memory for a histogram element
**
**  History:    
**      24-may-86 (seputis)    
{@history_line@}...
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**/

/*{
** Name: oph_memory	- get new histogram element
**
** Description:
**      This routine is the "memory manager" for histogram structures 
**      which will be replace by the free space routines once they are
**      integrated into the memory manager.
**
**      If a free element is not available then this routine will allocate
**      a new histogram element 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**
** Outputs:
**	Returns:
**	    OPH_HISTOGRAM element 
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-may-86 (seputis)
**          initial creation
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
*/
OPH_HISTOGRAM *
oph_memory(
	OPS_SUBQUERY       *subquery)
{
    OPH_HISTOGRAM          **freehpp;	    /* ptr to ptr to next free element
					    ** on the histogram free list */
    OPH_HISTOGRAM          *freehp;         /* ptr to free histogram element */

    if (*(freehpp = &subquery->ops_global->ops_estate.opn_hfree))
    {	/* an element exists in the free list of histograms so remove it */
	freehp = *freehpp;		    /* save histogram element */
	*freehpp = freehp->oph_next;	    /* remove it from the list */
    }
    else
    {	/* need to allocate a new histogram element */
	freehp = (OPH_HISTOGRAM *) opn_memory( subquery->ops_global, 
	    (i4) sizeof(OPH_HISTOGRAM) );  /* allocate memory for histogram 
					    ** element */
    }

    return (freehp);
}
