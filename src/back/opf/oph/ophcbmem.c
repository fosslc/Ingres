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
#define        OPH_CBMEMORY      TRUE
#include    <opxlint.h>

/**
**
**  Name: OPHCBMEM.C - get memory for cell boundary array
**
**  Description:
**      Contains routines to get memory for a cell boundary array
**
**  History:    
**      13-jun-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
OPH_CELLS oph_cbmemory(
	OPS_SUBQUERY *subquery,
	i4 arraysize);

/*{
** Name: oph_cbmemory	- get memory for a cell boundary array
**
** Description:
**      This routine will obtain and manage the memory for a cell 
**      boundary array.  The assumption is that only one such element 
**      will ever need to be created by the enumeration phase of the optimizer.
**      There will be a streamid associated with this memory so that it
**      can be deallocated if a larger block of memory is required.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      cbsize                          number of bytes to be allocated
**                                      for the cell boundary array
**
** Outputs:
**	Returns:
**	    ptr to a OPH_CELLS array of the appropriate size
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	13-jun-86 (seputis)
**          initial creation
[@history_line@]...
*/
OPH_CELLS 
oph_cbmemory(
	OPS_SUBQUERY       *subquery,
	i4	   	   arraysize)
{
    OPS_STATE           *global;            /* ptr to global state variable */

    global = subquery->ops_global;          /* ptr to global state variable */

    if (global->ops_estate.opn_cbsize >= arraysize)
	return (global->ops_estate.opn_cbptr); /* current memory area is large
                                            ** enough */

    /* not enough memory available so allocate more for this element */
    opu_deallocate(global, &global->ops_estate.opn_cbstreamid);
    global->ops_estate.opn_cbstreamid = opu_allocate(global);
    /* FIXME - replace with ulm_reinitstream when it is implemented */

    {	
        ULM_RCB                *ulm_rcb;    /* ptr to control block used to
                                            ** manage cell boundary memory */

    	ulm_rcb = &global->ops_mstate.ops_ulmrcb;      /* ptr to ULM_RCB for session */
	ulm_rcb->ulm_psize = global->ops_estate.opn_cbsize = arraysize; /* 
                                            ** get size of array required */
	if (ulm_palloc( ulm_rcb ) != E_DB_OK)
	    opx_verror(E_DB_ERROR, E_OP0400_MEMORY, global->
		ops_mstate.ops_ulmrcb.ulm_error.err_code);
        global->ops_estate.opn_cbptr = ulm_rcb->ulm_pptr; /* save base of 
                                            ** memory allocated for cell
                                            ** boundary temporary */
    	return ((OPH_CELLS) ulm_rcb->ulm_pptr); /* enough memory is
					    ** available */
    }
}
