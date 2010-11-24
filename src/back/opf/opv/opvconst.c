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
#define        OPV_CONSTNODE      TRUE
#include    <opxlint.h>


/**
**
**  Name: OPVCONST.C - create a query tree constant node
**
**  Description:
**      This file contains a routine which will create a query tree constant 
**      node.
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
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void opv_rqinsert(
	OPS_STATE *global,
	OPS_PNUM parmno,
	PST_QNODE *qnode);
PST_QNODE *opv_constnode(
	OPS_STATE *global,
	DB_DATA_VALUE *datatype,
	i4 parmno);
PST_QNODE *opv_intconst(
	OPS_STATE *global,
	i4 intvalue);
PST_QNODE *opv_i1const(
	OPS_STATE *global,
	i4 intvalue);
void opv_uparameter(
	OPS_STATE *global,
	PST_QNODE *qnode);

/*{
** Name: OPV_RQINSERT	- insert repeat query parameter into list
**
** Description:
**      This routine will insert a constant node representing a repeat
**      query parameter into a list in descending order of repeat query
**      number.  It is also used to insert nodes which are the result of
**      simple aggregates.
**
** Inputs:
**      global                          ptr to global state variable
**      parmno                          repeat query parameter number
**      qnode                           ptr to constant node representing
**                                      the repeat query parameter
**
** Outputs:
**      global->ops_rqlist              a structure is inserted into this
**                                      list (unless one already exists with
**                                      the same repeat query number )
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-nov-86 (seputis)
**          initial creation
**      19-dec-89 (seputis)
**          fix uninitialized variable problem, b9182
**      16-oct-92 (ed)
**          - fix bug 40835 - wrong number of duplicates when union views and
**          subselects are used
[@history_template@]...
*/
VOID
opv_rqinsert(
	OPS_STATE          *global,
	OPS_PNUM	   parmno,
	PST_QNODE          *qnode)
{
    if (parmno)
    {	/* if non-zero then this is a repeat query
	** - need to create a list of these objects so that
	** OPC can easily reference them */
	OPS_RQUERY	**rqdescp;	/* used to traverse the list
					** of repeat query descriptors*/
	i4		node_parmno;	/* parameter number of the
					** node being analyzed */

	node_parmno = 0;
	for (rqdescp = &global->ops_rqlist;
	    *rqdescp;
	    rqdescp = &(*rqdescp)->ops_rqnext)
	{

	    node_parmno = (*rqdescp)->ops_rqnode->pst_sym.pst_value.
		pst_s_cnst.pst_parm_no;
	    if (node_parmno <= parmno)
		break;			/* exit since this is the
					** insertion point */
	}
	if (node_parmno != parmno)
	{   /* insert a new node into the list , otherwise
	    ** just exit since the node is already in the list */
	    OPS_RQUERY	    *new_rqnode;
	    new_rqnode = (OPS_RQUERY *) opu_memory(global, 
			    (i4)sizeof(OPS_RQUERY));
	    new_rqnode->ops_rqnext = *rqdescp;
	    *rqdescp = new_rqnode;
	    new_rqnode->ops_rqnode = qnode;
	    new_rqnode->ops_rqmask = 0;
	}
    }
}

/*{
** Name: opv_constnode	- create a query tree constant node
**
** Description:
**      This routine will create a query tree parameter constant node 
**      to be used with substituting the result of a simple aggregate into 
**      the query.  The constant node will be assigned a parameter constant
**      number.
**
** Inputs:
**      global                          ptr to global state variable
**      datatype                        ptr to datatype for constant
**      parmno                          unique repeat query parm number
**                                      or 0
**
** Outputs:
**	Returns:
**	    ptr to PST_CONSTANT node with parameter constant allocated
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	1-jul-86 (seputis)
**          initial creation
**	9-nov-89 (seputis)
**	    used local variables instead of repeat parameters for procedures
[@history_line@]...
*/
PST_QNODE *
opv_constnode(
	OPS_STATE          *global,
	DB_DATA_VALUE      *datatype,
	i4                parmno)
{
    register PST_QNODE *qnode;      /* ptr used for symbol allocation */

    qnode = (PST_QNODE *) opu_memory( global, (i4) sizeof(PST_QNODE));
                                    /* allocate storage for query tree node
                                    ** to be used for creation of const node
                                    */
    qnode->pst_left = NULL;
    qnode->pst_right = NULL;
    qnode->pst_sym.pst_type = PST_CONST; /* create constant node type */
    STRUCT_ASSIGN_MACRO(*datatype, qnode->pst_sym.pst_dataval);
    qnode->pst_sym.pst_len = sizeof(PST_CNST_NODE);
    qnode->pst_sym.pst_value.pst_s_cnst.pst_parm_no = parmno; 
				    /* allocate a parameter number for the
                                    ** result of this aggregate */
    qnode->pst_sym.pst_value.pst_s_cnst.pst_pmspec = PST_PMNOTUSED; /* no
                                    ** pattern matching for simple aggregates */
    qnode->pst_sym.pst_value.pst_s_cnst.pst_cqlang 
	= global->ops_adfcb->adf_qlang; /* FIXME - this should really be the
                                    ** language ID of the root node containing
                                    ** the aggregate, but it doesn't matter
                                    ** since pattern matching should not be
                                    ** be used */

    qnode->pst_sym.pst_value.pst_s_cnst.pst_origtxt = NULL; /* no text for 
					** repeat query constant */
    if (parmno)
    {
	if (global->ops_procedure->pst_isdbp)
	    /* OPC cannot deal with repeat query parameters and local dbp variables
	    ** so only use local dbp variables if inside a procedure */
	    qnode->pst_sym.pst_value.pst_s_cnst.pst_tparmtype = PST_LOCALVARNO;
	else
	    qnode->pst_sym.pst_value.pst_s_cnst.pst_tparmtype = PST_RQPARAMNO;
	qnode->pst_sym.pst_dataval.db_data = NULL; /* if a repeat query
				    ** parm is being create then the
				    ** data value is not valid */
	opv_rqinsert(global, parmno, qnode);
    }
    else
	qnode->pst_sym.pst_value.pst_s_cnst.pst_tparmtype = PST_USER;
    return( qnode );
}

/*{
** Name: opv_intconst	- create integer constant node with value
**
** Description:
**      This routine will create an i2 integer constant with the given value 
[@comment_line@]...
**
** Inputs:
**      global                          ptr to global state variable
**      intvalue                        value constant should take
**
** Outputs:
**	Returns:
**	    ptr to constant node
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      9-sep-88 (seputis)
**          initial creation
**	31-Oct-2007 (kschendel) b122118
**	    Keep junk out of collID's.
[@history_line@]...
[@history_template@]...
*/
PST_QNODE *
opv_intconst(
	OPS_STATE          *global,
	i4                intvalue)
{
    i2		*int_const;
    DB_DATA_VALUE	datavalue;

    datavalue.db_datatype = DB_INT_TYPE;
    datavalue.db_prec = 0;
    datavalue.db_collID = DB_NOCOLLATION;
    datavalue.db_length = sizeof(*int_const);
    datavalue.db_data = (PTR)opu_memory(global, (i4)sizeof(*int_const));
    int_const = (i2 *) datavalue.db_data;
    *int_const = intvalue;
    return(opv_constnode(global, &datavalue, 0));
}

/*{
** Name: opv_i1const	- create i1 constant node with value
**
** Description:
**      This routine will create an i1 integer constant with the given value 
[@comment_line@]...
**
** Inputs:
**      global                          ptr to global state variable
**      intvalue                        value constant should take
**
** Outputs:
**	Returns:
**	    ptr to constant node
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      9-sep-89 (seputis)
**          initial creation
**	21-May-2009 (kiria01) b122051
**	    Reduce uninit'd collID
[@history_line@]...
[@history_template@]...
*/
PST_QNODE *
opv_i1const(
	OPS_STATE          *global,
	i4                intvalue)
{
    i1		*int_const;
    DB_DATA_VALUE	datavalue;

    datavalue.db_datatype = DB_INT_TYPE;
    datavalue.db_prec = 0;
    datavalue.db_collID = DB_NOCOLLATION;
    datavalue.db_length = sizeof(*int_const);
    datavalue.db_data = (PTR)opu_memory(global, (i4)sizeof(*int_const));
    int_const = (i1 *) datavalue.db_data;
    *int_const = intvalue;
    return(opv_constnode(global, &datavalue, 0));
}

/*{
** Name: OPV_UPARAMETER - assign parameter number to this user parameter
**
** Description:
**	This routine will assign a parameter number to a constant node
**      and insert it into the repeat query list.
**
** Inputs:
**      global                          ptr to global state variable
**      qnode                           ptr to constant node containing
**                                      the query parameter
**
** Outputs:
**      global->ops_rqlist              a structure is inserted into this
**                                      list (unless one already exists with
**                                      the same repeat query number )
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      13-feb-89 (seputis)
**          initial creation
[@history_template@]...
*/
VOID
opv_uparameter(
	OPS_STATE          *global,
	PST_QNODE          *qnode)
{
    OPS_PNUM	    parm_no;

    parm_no =
    qnode->pst_sym.pst_value.pst_s_cnst.pst_parm_no = ++global->ops_parmtotal;
    opv_rqinsert(global, parm_no, qnode);
}
