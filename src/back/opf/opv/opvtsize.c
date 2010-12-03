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
#define        OPV_TUPLESIZE      TRUE
#include    <opxlint.h>


/**
**
**  Name: OPVTSIZE.C - get size of tuple
**
**  Description:
**      Routine to get the size of a tuple
**
**  History:    
**      8-apr-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**      06-mar-96 (nanpr01)
**          Changed the type to i4 from i4  to make it consistent
**          with function definition.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 opv_tuplesize(
	OPS_STATE *global,
	PST_QNODE *qnode);

/*{
** Name: opv_tuplesize	- find width in bytes of tuple represented by query tree
**
** Description:
**      This routine will traverse the left branches of the query tree... the
**      assumption is that this is a target list of the projection tree; i.e.
**      is a "resdom list" or a "byhead list" ...and add the size of individual 
**      elements in the list (as they would appear in a tuple) and return 
**      the size of the projected tuple.
**
** Inputs:
**      qnode                           ptr to query tree node to be analyzed
**
** Outputs:
**	Returns:
**	    Size of projected tuple in bytes.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	8-apr-86 (seputis)
**          initial creation
**      6-mar-96 (nanpr01)
**          Changed the type to i4 from i4  to make it consistent
**          with function definition.
[@history_line@]...
*/
i4
opv_tuplesize(
	OPS_STATE          *global,
	PST_QNODE          *qnode)
{
    register PST_QNODE  *node;       /* register accelerated ptr to query
                                     ** tree node
                                     */
    i4             tuple_width; /* cumulative size of projected tuple */

# ifdef E_OP0680_TARGETLIST
    /* validate input parameter */
    if (!qnode || !qnode->pst_left)
        opx_error(E_OP0680_TARGETLIST);
# endif
    tuple_width = 0;
    for (node = qnode->pst_left;     /* start at beginning of target list */
         node && node->pst_sym.pst_type != PST_TREE; /* check if end of list 
                                     ** is found*/
         node = node->pst_left)      /* get next element of target list */
    {
# ifdef E_OP0680_TARGETLIST
         /* validate result domain node type */
         if (!node || node->pst_sym.pst_type != PST_RESDOM)
             opx_error(E_OP0680_TARGETLIST);
# endif
         /* This assumes that only resdoms are hanging down left side of tree */
         tuple_width += node->pst_sym.pst_dataval.db_length; /*add datatype 
					** length to total */
    }

    return (tuple_width);
}
