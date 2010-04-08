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
#define        OPV_COPYTREE      TRUE
#include    <me.h>
#include    <opxlint.h>


/**
**
**  Name: OPVCPYTR.C - routine to copy a query tree
**
**  Description:
**      File contains routine to copy a query tree 
**
**  History:    
**      21-jul-86 (seputis)
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	15-sep-93 (swm)
**	    Moved cs.h include above other header files which need its
**	    definition of CS_SID.
**	27-Oct-2009 (kiria01) SIR 121883
**	    Scalar sub-selects - reduced recursion copy.
[@history_template@]...
**/

/*{
** Name: opv_copynode	- copy one parse tree node
**
** Description:
**      Routine to copy a parse tree node, allocating the correct memory size
**	but not copying any of the children.
**
** Inputs:
**      global                          ptr to query state variable
**      nodepp				ptr to node to copy
**
** Outputs:
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      27-jan-94 (ed)
**          initial creation
**	15-june-00 (inkdo01)
**	    Added support for CASE expressions.
**	15-mar-02 (inkdo01)
**	    Added support for SEQOP expressions.
**	5-dec-03 (inkdo01)
**	    Added support for PST_MOP (SUBSTRING) and PST_OPERAND.
**	6-apr-06 (dougi)
**	    Add PST_GSET/GBCR/GCL nodes (for GROUP BY enhancements).
[@history_template@]...
*/
VOID
opv_copynode(
	OPS_STATE	    *global, 
	PST_QNODE	    **nodepp)
{
    i4		symsize;	/* size of symbol to allocate */
    PST_QNODE	*nodep;

    nodep = *nodepp;
    switch (nodep->pst_sym.pst_type)
    {
    case PST_AND:
    case PST_OR:
    case PST_UOP:
    case PST_BOP:
    case PST_AOP:
    case PST_COP:
    case PST_MOP:
	symsize = sizeof(PST_OP_NODE);
	break;
    case PST_CONST:
	symsize = sizeof(PST_CNST_NODE);
	break;
    case PST_RESDOM:
    case PST_BYHEAD:
	symsize = sizeof(PST_RSDM_NODE);
	break;
    case PST_VAR:
	symsize = sizeof(PST_VAR_NODE);
	break;
    case PST_RULEVAR:
	symsize = sizeof(PST_RL_NODE);
	break;
    
    case PST_AGHEAD:
    case PST_ROOT:
    case PST_SUBSEL:
	symsize = sizeof(PST_RT_NODE);
	break;
    case PST_CASEOP:
	symsize = sizeof(PST_CASE_NODE);
	break;
    case PST_SEQOP:
	symsize = sizeof(PST_SEQOP_NODE);
	break;
    case PST_GBCR:
	symsize = sizeof(PST_GROUP_NODE);
	break;
    case PST_QLEND:
    case PST_TREE:
    case PST_NOT:
    case PST_OPERAND:
    case PST_WHOP:
    case PST_WHLIST:
    case PST_GSET:
    case PST_GCL:
	symsize = 0;
	break;
    case PST_SORT:
	symsize = sizeof(PST_SRT_NODE);
	break;
    case PST_CURVAL:
	symsize = sizeof(PST_CRVAL_NODE);
	break;
    default:
	opx_error(E_OP0681_UNKNOWN_NODE);
    }
    symsize += sizeof(PST_QNODE)-sizeof(PST_SYMVALUE);

    *nodepp = (PST_QNODE *) opu_memory( global, symsize); /* get
				    ** memory for new node */
    MEcopy((PTR) nodep, symsize, (PTR) *nodepp); /* copy 
				    ** node into newly allocated space*/
}

/*{
** Name: opv_copytree	- copy a query tree
**
** Description:
**      This routine will copy a query tree in the optimizer's memory space
**
** Inputs:
**      global                          ptr to global state variable
**      nodepp                          ptr to ptr to root node to copy
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-jul-86 (seputis)
**          initial creation
**	27-jan-94 (ed)
**	    extracted single node creation routine
**	17-aug-06 (toumi01)
**	    make function aware of logically null opa_suckrestrict node markers
**	31-Oct-2006 (kschendel)
**	    VAR nodes don't have children, but opa sometimes makes it
**	    look like they do.  Don't recurse under VAR's.
[@history_template@]...
*/
VOID
opv_copytree(
	OPS_STATE          *global,
	PST_QNODE          **nodepp)
{
    OPV_STKDECL
    PST_QNODE           *nodep;

    OPV_STKINIT
    nodep = *nodepp;			/* get ptr to node to copy */
    while (nodep && nodep != (PST_QNODE *) -1)	/* skip opa_suckrestrict markers */
    {	
	opv_copynode(global, nodepp);
	/* duplicate a union */
	if ((nodep->pst_sym.pst_type == PST_ROOT || 
	    nodep->pst_sym.pst_type == PST_AGHEAD ||
	    nodep->pst_sym.pst_type == PST_SUBSEL)
	    &&
	    nodep->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
	{
	    OPV_STKPUSH(&(*nodepp)->pst_sym.pst_value.pst_s_root.pst_union.pst_next);
	}
	/* VAR nodes can be tricked out, don't fall for it */
	if (nodep->pst_sym.pst_type == PST_VAR)
	{
	    if (!(nodepp = (PST_QNODE**)OPV_STKPOP()))
		break;
	}
	else if (nodep->pst_left &&
	    nodep->pst_left != (PST_QNODE *) -1)
	{
	    if (nodep->pst_right &&
		nodep->pst_right != (PST_QNODE *) -1)
		OPV_STKPUSH(&(*nodepp)->pst_right);
	    nodepp = &(*nodepp)->pst_left;
	}
	else if (nodep->pst_right &&
		nodep->pst_right != (PST_QNODE *) -1)
	{
	    nodepp = &(*nodepp)->pst_right;
	}
	else if (!(nodepp = (PST_QNODE**)OPV_STKPOP()))
	    break;
	nodep = *nodepp;
    }
}

/*{
** Name: opv_push_ptr -     Push item on recursion stack
**
** Description:
**
** Inputs:
**	base		Stack base block
**	item		Item to be stored
**
** Outputs:
**	*base		Structure updated
**
**	Returns:
**	    None
**	Exceptions:
**	    None
**
** Side Effects:
**	May allocate memory.
**
** History:
**	02-Sep-2009 (kiria01)
**	    Created for flattened out stack recursion
*/

VOID
opv_push_item(OPS_STATE *global, OPV_STK *base, PTR item)
{
    /* Point to true list head block */
    struct OPV_STK1 *stk = base->stk.link;
    if (!item)
	return;
    if (!stk)
	stk = &base->stk;

    /* Is there space */
    if (stk->sp >= OPV_N_STK)
    {
	/* No space - one on free? */
	if (base->stk.link = base->free)
	    base->free = base->stk.link->link;
	/* .. one from heap */
	else
	{
	    base->stk.link = (struct OPV_STK1*)opu_memory(global, (i4)sizeof(struct OPV_STK1));
	    if (!base->stk.link)
		return;
	}
	/* Link in new block */
	base->stk.link->link = stk;
        stk = base->stk.link;
	/* Init sp */
        stk->sp = 0;
    }
    /* push block */
    stk->list[stk->sp++] = item;
}

/*{
** Name: opv_pop_item	- Retrieve top item off recursion stack
**
** Description:
**
** Inputs:
**	base		Stack base block
**
** Outputs:
**
**	Returns:
**	    item from stack or NULL if end-of-stack.
**
**	Exceptions:
**	    None
**
** Side Effects:
**	May drop memory from the stack.
**
** History:
**	02-Sep-2009 (kiria01)
**	    Created for flattened out stack recursion
*/

PTR
opv_pop_item(OPV_STK *base)
{
    /* Point to true list head block */
    struct OPV_STK1 *stk = base->stk.link;
    if (!stk)
	stk = &base->stk;

    if (!stk->sp)
    {
        if (stk == &base->stk)
            /*stack underflow*/
            return NULL;

	/* Unlink blk */
        base->stk.link = stk->link;
	/* Add to free list */
	stk->link = base->free;
	base->free = stk;
	/* Point to next block*/
        stk = base->stk.link;
    }
    /* Pop actual item */
    return stk->list[--stk->sp];
}

/*{
** Name: opv_pop_all    - Release resources from the recursion stack
**
** Description:
**
** Inputs:
**	base            Stack base block
**
** Outputs:
**	None
**
**	Returns:
**	    None.
**	Exceptions:
**	    None
**
** Side Effects:
**	May free memory
**
** History:
**	02-Sep-2009 (kiria01)
**	    Created for flattened out stack recursion
*/

VOID
opv_pop_all(OPV_STK *base)
{
    if (base->stk.link)
    {
	struct OPV_STK1 *stk;
	for (stk = base->stk.link; stk != &base->stk; stk = base->stk.link)
	{
	    base->stk.link = stk->link;
	    stk->link = base->free;
	    base->free = stk;
	}
    }
    base->stk.sp = 0;
}
