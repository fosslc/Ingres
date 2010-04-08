/* Copyright (c) 1995, 2005 Ingres Corporation
**
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <adf.h>
#include    <ade.h>
#include    <dudbms.h>
#include    <dmf.h> 
#include    <dmmcb.h>
#include    <dmucb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <ex.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <scf.h>
#include    <qefmain.h>
#include    <qefscb.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>
#include    <sxf.h>
#include    <qefprotos.h>

/**
**
**  Name: QECDBG.C - debugging routines
**
**  Description:
**      The routines in this file are used to print various structures
**  in the QEF.
**
**          qec_pss  - print session control block
**          qec_psr  - print server control block
**          qec_pdsh - print DSH 
**          qec_pqep - print query execution plan
**	    qec_pqp  - print QP
**
**  History:
**      30-may-86 (daved)    
**          written
**	18-apr-89 (paul)
**	    Updated to support cursor update and delete as regular query plans.
**	09-may-89 (neil)
**	    Added qec_scctrace as interface to SCC_TRACE for session tracing.
**	31-may-89 (neil)
**	    Removed qec_scctrace as at same time qec_tprintf was written.
**	08-dec-92 (anitap)
**	    Added #include <qsf.h> and <psfparse.h>.
**	08-jan-93 (davebf)
**	    Converted to function prototypes  
**	    qec_psr() not prototyped.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	29-jun-93 (rickh)
**	    Added TR.H.
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	11-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in qec_pss(), qec_psr(),
**	    qec_pdsh() and qec_pqep().
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**	3-Jun-2005 (schka24)
**	    Fix old-style decls to quiet the C compiler.
[@history_line@]...
**/


/*{
** Name: qec_pss       - print session control block
**
** Description:
**      Print the elements of the session control block 
**
** Inputs:
**      qef_cb                          the session control block
**
** Outputs:
**      none.
**
**      Returns:
**          E_DB_OK
**      Exceptions:
**          none
**
** Side Effects:
**          Trace information is printed
**
** History:
**      30-may-86 (daved)
**          written
**	11-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
[@history_line@]...
*/
VOID
qec_pss(
QEF_CB         *qef_cb )
{
    char       *name;

    TRdisplay("The QEF_CB at address %p\n", qef_cb);
    TRdisplay("session id %x, transaction id %x\n", qef_cb->qef_ses_id,
        qef_cb->qef_tran_id);
    switch (qef_cb->qef_stat)
    {
        case QEF_NOTRAN:
            name = "no transaction";
            break;
        case QEF_SSTRAN:
            name = "single statement transaction";
            break;
        case QEF_MSTRAN:
            name = "multi statement transaction";
            break;
        case QEF_ITRAN:
            name = "internal transaction";
            break;
    }
    TRdisplay("Transaction type is %s\n", name);
}

/*{
** Name: qec_psr       - print server control block
**
** Description:
**      The contents of the QEF server control block are printed. 
**
** Inputs:
**      qef_s_cb                        the server control block
**
** Outputs:
**      none
**
**      Returns:
**          E_DB_OK
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      30-may-86 (daved)
**          written
**	11-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
[@history_line@]...
*/
VOID
qec_psr(qef_s_cb)
QEF_S_CB       *qef_s_cb;  
{
    TRdisplay("QEF session control block at %p\n", qef_s_cb);
}

/*{
** Name: qec_pdsh   - print the DSH struct
**
** Description:
**      Print the runtime environment for a query plan 
**
** Inputs:
**      qee_dsh                         the runtime environment header
**
** Outputs:
**      none
**
**      Returns:
**          E_DB_OK
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      30-may-86 (daved)
**          written
[@history_line@]...
*/
VOID
qec_pdsh(
QEE_DSH        *qee_dsh )
{
    TRdisplay("QEF DSH struct %p\n", qee_dsh);
}

/*{
** Name: qec_pqep       - print a query execution plan
**
** Description:
**      Print a query execution plan 
**
** Inputs:
**      qen_node                query execution plan
**
** Outputs:
**      none
**
**      Returns:
**          E_DB_OK
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      30-may-86 (daved)
**          written
[@history_line@]...
*/
FUNC_EXTERN VOID
qec_pqep(
QEN_NODE       *qen_node )
{
    TRdisplay("QEF query execution plan. %p\n", qen_node);
}

/*{
** Name: QEC_PQP	- print query plan
**
** Description:
**      The query plan, its actions, and its QEPs are printed 
**
** Inputs:
**      qef_qp                          query plan
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	25-jun-86 (daved)
**          written
**	28-sep-87 (rogerk)
**	    Look for QEA_LOAD type.
[@history_line@]...
*/
VOID
qec_pqp(
QEF_QP_CB          *qef_qp )
{
    i4		number;
    i4		i;
    QEF_AHD	*ahd_ptr;
    QEN_NODE	*node_ptr;

    TRdisplay("\nQuery Plan name : %t\n", DB_MAXNAME, 
		qef_qp->qp_id.db_cur_name);
    TRdisplay("Update : %d, Delete : %d, Repeat %d\n", 
	qef_qp->qp_status & QEQP_UPDATE,
	qef_qp->qp_status & QEQP_DELETE,
	qef_qp->qp_status & QEQP_RPT);
    TRdisplay("Space required for Keys : %d\n", qef_qp->qp_key_sz);
    TRdisplay("Space required for result tuple : %d\n", qef_qp->qp_res_row_sz);
    TRdisplay("Number of row buffers required : %d\n", qef_qp->qp_row_cnt);
    TRdisplay("Number of status blocks : %d\n", qef_qp->qp_stat_cnt);
    TRdisplay("Number of control blocks : %d\n", qef_qp->qp_cb_cnt);
    TRdisplay("Number of actions required in query : %d\n", qef_qp->qp_ahd_cnt);
    TRdisplay("The actions are:\n\n");
    for (i = 0, ahd_ptr = qef_qp->qp_ahd; i < qef_qp->qp_ahd_cnt; i++)
    {
	TRdisplay("Action type is %w\n", 
	    ", QEA_DMU, QEA_APP, QEA_SAGG, QEA_PROJ, \
QEA_AGGF, QEA_UPD, QEA_DEL, QEA_GET, QEA_UTL, QEA_RUP, QEA_RDEL, QEA_RAGGF, \
QEA_LOAD",
	    ahd_ptr->ahd_atype);
	switch(ahd_ptr->ahd_atype)
	{
	    case QEA_SAGG:
	    case QEA_APP:
	    case QEA_LOAD:
	    case QEA_PROJ:
	    case QEA_AGGF:
	    case QEA_UPD:
	    case QEA_DEL:
	    case QEA_GET:
		if (node_ptr = ahd_ptr->qhd_obj.qhd_qep.ahd_qep)
		{
		    TRdisplay("The QEP is:\n");
		    number = 0;
		    for (; node_ptr; node_ptr = node_ptr->qen_postfix)
			qec_prnode(node_ptr, &number);
		}
		else
		{
		    TRdisplay("No QEP\n");
		}
	    case QEA_RUP:
	    case QEA_RDEL:
		TRdisplay("DMF control block containing update row : %d\n",
		    ahd_ptr->qhd_obj.qhd_qep.ahd_odmr_cb);
	    default:
		/* do nothing*/ ;
	}	    /* end case */
    }		    /* for each action */
}   

/*{
** Name: qec_prnode	- print QEP node
**
** Description:
**	Dump the contents of a QEP node
**
** Inputs:
**      tree                            QEP
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-jun-86 (daved)
**          written
[@history_line@]...
*/
void
qec_prnode(
QEN_NODE	   *node,
i4		   *number )
{
    TRdisplay("*******************\n\n");
    TRdisplay("Node Number : %d\n", *number++);
    switch (node->qen_type)
    {
	case QE_ORIG:
	    TRdisplay("Node Type : QE_ORIG\n");
	    /* print the attributes */
	    pratts(node->qen_natts, node->qen_atts);
	    TRdisplay("Row Number : %d\n", node->qen_row);
	    TRdisplay("Row Size : %d\n", node->qen_rsz);
	    /* print the rows used to produce the tuple at this node */
	    TRdisplay("The rows necessary to produce the tuple at \
this node:\n");
	    TRdisplay("DMF record control block is : %d\n", 
		node->node_qen.qen_orig.orig_get);
	    TRdisplay("The rows necessary for qualifying the tuple are:\n");
	    TRdisplay("The keys for this node are in control block \
number : %d\n", node->node_qen.qen_orig.orig_keys);
	    break;
	case QE_TJOIN:
	    TRdisplay("Node Type : QE_TJOIN\n");
	    /* print the attributes */
	    pratts(node->qen_natts, node->qen_atts);
	    TRdisplay("Row Number : %d\n", node->qen_row);
	    TRdisplay("Row Size : %d\n", node->qen_rsz);
	    /* print the rows used to produce the tuple at this node */
	    TRdisplay("The rows necessary to produce the tuple at \
this node:\n");
	    TRdisplay("DMF record control block is : %d\n", 
		node->node_qen.qen_tjoin.tjoin_get);
	    TRdisplay("The rows necessary for qualifying the tuple are:\n");
	    break;
	default:
	    ;
    }
}

/*{
** Name: pratts	- print the attributes for a node
**
** Description:
**
** Inputs:
**      num_atts                        number of attributes
**      atts                            attribute array
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-jun-86 (daved)
**          written
[@history_line@]...
*/
void
pratts(
i4		num_atts,
DMT_ATT_ENTRY	**atts )
{
    i4	    i;

    for (i = 0; i < num_atts; i++)
    {
	TRdisplay("Attribute number : %d\n", atts[i]->att_number);
	TRdisplay("Attribute offset : %d\n", atts[i]->att_offset);
	TRdisplay("Attribute type : %d\n", atts[i]->att_type);
	TRdisplay("Attribute width : %d\n", atts[i]->att_width);
	TRdisplay("\n");
    }
}
