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
#define        OPZ_FMAKE      TRUE
#include    <me.h>
#include    <opxlint.h>


/**
**
**  Name: OPZFMAKE.C - make a function attribute element
**
**  Description:
**      create a new function attribute element
**
**  History:    
**      24-jun-86 (seputis)    
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
OPZ_FATTS *opz_fmake(
	OPS_SUBQUERY *subquery,
	PST_QNODE *qnode,
	OPZ_FACLASS class);

/*{
** Name: opz_fmake	- allocate a new function attribute element
**
** Description:
**      This routine will allocate a new function attribute element
**      and initialize it.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      qnode                           ptr to query tree node representing
**                                      function attribute expression
**      class                           type of function attribute to create
**
** Outputs:
**	Returns:
**	    ptr to allocate function attribute node
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-jun-86 (seputis)
**          initial creation
[@history_line@]...
*/
OPZ_FATTS *
opz_fmake(
	OPS_SUBQUERY       *subquery,
	PST_QNODE          *qnode,
	OPZ_FACLASS        class)
{
    OPZ_FATTS           *funcp;		/* ptr to newly allocated function
                                        ** attribute element */

    funcp = (OPZ_FATTS *) opu_memory( subquery->ops_global, 
				      (i4)sizeof(OPZ_FATTS)); /* get new
                                        ** function attribute element */
    MEfill( sizeof(OPZ_FATTS), (u_char)0, (PTR)funcp);
    funcp->opz_type = class;
    funcp->opz_function = qnode;
    if ((funcp->opz_fnum = subquery->ops_funcs.opz_fv++) >= (OPZ_MAXATT-1))
	opx_error( E_OP0304_FUNCATTR_OVERFLOW);
    subquery->ops_funcs.opz_fbase->opz_fatts[funcp->opz_fnum] = funcp;
    return (funcp);
}
