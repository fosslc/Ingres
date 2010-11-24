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
#define             OPA_PRIME           TRUE
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 

/**
**
**  Name: OPAPRIME.C - routines for determining whether a node is prime
**
**  Description:
**      This file contains routines which determine whether an aggregate
**      is prime  .
**
**  History:    
**      13-apr-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
bool opa_prime(
	OPS_SUBQUERY *subquery);

/*{
** Name: opa_prime	- traverse list to discover any prime aggregates
**
** Description:
**	Determine if an aggregate contains any
**	prime aggregates. Note that there might
**	be more than one aggregate.
**
** Inputs:
**      subquery                        ptr to first aggregate subquery
**					in list of aggregates to be
**                                      evaluated together
**
** Outputs:
**	Returns:
**	    TRUE - if a prime aggregate is found in the list
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	13-apr-86 (seputis)
**          initial creation
[@history_line@]...
*/
bool
opa_prime(
    OPS_SUBQUERY       *subquery)
{
    for( ; subquery ; subquery = subquery->ops_agg.opa_concurrent)
    {
        if (subquery->ops_agg.opa_aop->pst_sym.pst_value.pst_s_op.pst_distinct
	    == 
	    PST_DISTINCT)
	    return (TRUE);
    }
    return(FALSE);
}
