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
#define        OPV_VARNODE      TRUE
#include    <me.h>
#include    <opxlint.h>

/**
**
**  Name: OPVVARNODE.C - create a varnode
**
**  Description:
**      This routine will create a query tree var node
**
**  History:    
**      1-jul-86 (seputis)    
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
**/


/*{
** Name: opv_varnode	- make a range variable node
**
** Description:
**      This procedure allocates and initializes a query tree PST_VAR node.
**
** Inputs:
**      global                          ptr to global state variable
**      datavalue                       datatype info of node to create
**      variable                        range variable number - index
**                                      into the global range table
**      attribute                       dmf attribute of range variable to be
**					assigned to var node.
**
** Outputs:
**	Returns:
**	    ptr to PST_QNODE initialized to be a var node
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	8-apr-86 (seputis)
**          initial creation
**	9-jul-91 (seputis)
**	    remove obsolete trace point
[@history_line@]...
*/
PST_QNODE *
opv_varnode(
	OPS_STATE          *global,
	DB_DATA_VALUE      *datatype,
	OPV_IGVARS         variable,
	DB_ATT_ID	   *dmfattr)
{
    register PST_QNODE *qnode;      /* ptr used for symbol allocation */

    qnode = (PST_QNODE *) opu_memory( global, (i4) sizeof(PST_QNODE));
                                    /* allocate storage for query tree node
                                    ** to be used for creation of var node
                                    */
    qnode->pst_left = NULL;
    qnode->pst_right = NULL;
    qnode->pst_sym.pst_type = PST_VAR; /* create VAR node type */
    STRUCT_ASSIGN_MACRO(*datatype, qnode->pst_sym.pst_dataval);
    qnode->pst_sym.pst_dataval.db_data = NULL;
    qnode->pst_sym.pst_len = sizeof(PST_VAR_NODE);
    qnode->pst_sym.pst_value.pst_s_var.pst_vno = variable;
    STRUCT_ASSIGN_MACRO(*dmfattr, qnode->pst_sym.pst_value.pst_s_var.pst_atno);
    MEfill(sizeof(qnode->pst_sym.pst_value.pst_s_var.pst_atname), ' ', 
	(PTR)&qnode->pst_sym.pst_value.pst_s_var.pst_atname); /* init attribute
				    ** name */
    return( qnode );
}

/*{
** Name: opv_qlend	- create a PST_QLEND node
**
** Description:
**      This routine will create a PST_QLEND node out of the main memory stream
**
** Inputs:
**      global                          ptr to global state variable
**
** Outputs:
**	Returns:
**	    PST_QNODE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      3-dec-87 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
PST_QNODE *
opv_qlend(
	OPS_STATE          *global)
{
    PST_QNODE           *qlend;
    i4			qlend_size;

    qlend_size = sizeof(PST_QNODE) - sizeof(PST_SYMVALUE);
    qlend = (PST_QNODE *) opu_memory(global, qlend_size); /* init PST_QLEND node
                                            ** to terminate qualification list*/
    MEfill(qlend_size, (u_char)0, (PTR)qlend);
    /* qlend->pst_left = NULL; */
    /* qlend->pst_right = NULL; */
    /* qlend->pst_sym.pst_len = 0; */
    qlend->pst_sym.pst_type = PST_QLEND;
    return(qlend);
}
