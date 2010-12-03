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
#define        OPH_CCMEMORY      TRUE
#include    <opxlint.h>


/**
**
**  Name: OPHCCMEM.C - get memory for a histogram cell count array
**
**  Description:
**      routines to get memory for a histogram cell count array
**
**  History:    
**      24-may-86 (seputis)    
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**       7-oct-94 (inkdo01)
**          fix for bug 63550. Need to get minimum of sizeof(OPH_COERCE) 
**          bytes so that freelist template can overlay the acquired stge.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
OPH_COUNTS oph_ccmemory(
	OPS_SUBQUERY *subquery,
	OPH_INTERVAL *intervalp);

/*{
** Name: oph_memory	- get new histogram cell count array element
**
** Description:
**      This routine is the "memory manager" for histogram cell count structures
**      which will be replaced by the free space routines once they are
**      integrated into the memory manager.
**
**      If a free element is not available then this routine will allocate
**      a new OPH_COUNT array.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      intervalp                       ptr to histogram component of an
**                                      attribute describing characteristics
**                                      of the histogram oph_fnct component
**
** Outputs:
**	Returns:
**	    memory to OPH_COUNT cell count component of this histogram 
**          structure.
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
**	2-may-95 (wadag01)
**	    Removed unnecessary braces from the second block of code
**	    (caused error - 'return' : incompatible types on odt_us5).
[@history_line@]...
*/
OPH_COUNTS
oph_ccmemory(
	OPS_SUBQUERY       *subquery,
	OPH_INTERVAL	   *intervalp)
{
    i4			required;   /* amount of memory to allocate */
    OPH_COUNTS          cellp;      /* ptr to cell count array */
    OPH_COERCE		**coercepp; /* coercion use to traverse linked list*/

    required = intervalp->oph_numcells * sizeof( OPN_PERCENT ) ;
    if (required < (i4)sizeof(OPH_COERCE)) required = sizeof(OPH_COERCE);
					     /* fix for bug 63550 */
    for (coercepp =(OPH_COERCE **)&subquery->ops_global->ops_estate.opn_ccfree; 
	*coercepp; 
	coercepp = &(*coercepp)->oph_next)
    {	/* attempt to obtain memory from the free list */
	if ((*coercepp)->oph_size == required)
	{   /* correct size found so remove from list and return */
	    cellp = (OPH_COUNTS) *coercepp;
	    *coercepp = (*coercepp)->oph_next;
	    return(cellp);
	}
    }

    /* element does not exist on free list so allocate a new one */
	cellp = (OPH_COUNTS) opn_memory( subquery->ops_global, (i4)required);/*
 				    ** allocate memory for cell count array */
	return (cellp);
}
