/*
**Copyright (c) 2004 Ingres Corporation
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
#define        OPV_OPNODE      TRUE
#include    <me.h>
#include    <opxlint.h>


/**
**
**  Name: OPVOPNODE.C - create a PST_OP_NODE
**
**  Description:
**      This routine will create a query tree PST_OP_NODE node
**
**  History:
**      1-jul-86 (seputis)    
**          initial creation
**	17-apr-89 (jrb)
**	    Properly initialized prec field(s).
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
**/


/*{
** Name: opv_opnode	- make a PST_OP_NODE node
**
** Description:
**      This procedure allocates and initializes a query tree PST_OP_NODE node.
**      Routine is used specifically to allocate a PST_AND node or a PST_BOP
**      node for a "=" in which the pst_dataval component is not defined.
**
** Inputs:
**      global                          ptr to global state variable
**      type                            type of PST_OP_NODE to be created
**      operator                        ADF operator id for node
**
** Outputs:
**	Returns:
**	    ptr to PST_QNODE initialized to be a PST_OP_NODE node
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	8-apr-86 (seputis)
**          initial creation
**	25-Jun-2008 (kiria01) SIR120473
**	    Move pst_isescape into new u_i2 pst_pat_flags.
[@history_line@]...
*/
PST_QNODE *
opv_opnode(
	OPS_STATE          *global,
	i4                type,
	ADI_OP_ID	   operator,
	OPL_IOUTER	   ojid)
{
    register PST_QNODE *qnode;      /* ptr used for symbol allocation */
    i4		       op_size;

    op_size = sizeof(PST_QNODE) - sizeof(PST_SYMVALUE) + sizeof(PST_OP_NODE);
    qnode = (PST_QNODE *) opu_memory(global, op_size); /* init PST_AND node
                                            ** to terminate qualification list*/
    MEfill(op_size, (u_char)0, (PTR)qnode);
    /* qnode->pst_left = NULL; */
    /* qnode->pst_right = NULL; */
    qnode->pst_sym.pst_type = type; /* create PST_OP_NODE node type */
    /* pst_dataval not defined for relational operators, AND nodes */
    /* qnode->pst_sym.pst_dataval.db_datatype = DB_NODT; */
    /* qnode->pst_sym.pst_dataval.db_prec = 0; */
    /* qnode->pst_sym.pst_dataval.db_length = 0; */
    /* qnode->pst_sym.pst_dataval.db_data = NULL; */
    /* qnode->pst_sym.pst_value.pst_s_op.pst_flags = 0; */
    qnode->pst_sym.pst_len = sizeof(PST_OP_NODE);
    qnode->pst_sym.pst_value.pst_s_op.pst_opno = operator;
    qnode->pst_sym.pst_value.pst_s_op.pst_opinst = 
    qnode->pst_sym.pst_value.pst_s_op.pst_oplcnvrtid = 
    qnode->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid = ADI_NOFI;
    qnode->pst_sym.pst_value.pst_s_op.pst_joinid = ojid;
    qnode->pst_sym.pst_value.pst_s_op.pst_pat_flags = AD_PAT_DOESNT_APPLY;
    return( qnode );
}
