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
#define        OPV_RESDOM      TRUE
#include    <opxlint.h>

/**
**
**  Name: OPVRESDOM.C - file to create result domain
**
**  Description:
**      file to create result domain query node
**
**  History:    
**      1-jul-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
[@history_line@]...
**/



/*{
** Name: opv_resdom	- create a resdom node
**
** Description:
**      The routine will create a resdom node given the query tree fragment 
**      which will produce the attribute and the result datatype.
**
** Inputs:
**      global                          ptr to global state variable
**      function                        query tree to provide result
**      datatype                        datatype of result domain
**
** Outputs:
**	Returns:
**	    ptr to result domain query tree node
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	22-apr-86 (seputis)
**          initial creation
**	12-aug-98 (hayke02)
**	    Initialize the resdom pst_joinid if there is an outer join. This
**	    resdom maybe used in a join (if there is an ifnull in an OJ view).
**	    This prevents an incorrect or out of range ojid being used to
**	    access the outer join descriptor array (opl_ojt[]) in the PST_BOP
**	    case in opb_bfinit(). This change fixes bug 92300.
**	27-Oct-2009 (kiria01) SIR 121883
**	    Scalar sub-selects - Make sure whole resdom init'd
[@history_line@]...
*/
PST_QNODE *
opv_resdom(
	OPS_STATE          *global,
	PST_QNODE          *function,
	DB_DATA_VALUE      *datatype)
{
    PST_QNODE		*resdom;    /* result domain node being created */

    resdom = (PST_QNODE *) opu_memory(global, (i4) sizeof(PST_QNODE) );
    MEfill((i4) sizeof(PST_QNODE), (u_char)0, (PTR)resdom);
    /* resdom->pst_left = NULL; */
    resdom->pst_right = function;
    resdom->pst_sym.pst_type = PST_RESDOM;
    resdom->pst_sym.pst_len = sizeof(PST_RSDM_NODE);
    STRUCT_ASSIGN_MACRO(*datatype, resdom->pst_sym.pst_dataval);
    /* resdom->pst_sym.pst_dataval.db_data = NULL; */
    resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsflags = PST_RS_PRINT;
    resdom->pst_sym.pst_value.pst_s_rsdm.pst_ntargno =
    resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno = OPZ_NOATTR;
    /* resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsupdt = FALSE; */
    resdom->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype = PST_ATTNO;
    /* pst_rsname not initialized - FIXME assign name for debugging */
    if (global->ops_goj.opl_mask & OPL_OJFOUND)
	resdom->pst_sym.pst_value.pst_s_op.pst_joinid = OPL_NOOUTER;
    return( resdom );
}
