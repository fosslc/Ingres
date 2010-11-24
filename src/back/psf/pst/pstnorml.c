/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <bt.h>
#include    <qu.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <adfops.h>
#include    <ade.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <scf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>

/**
**
**  Name: PSTNORML.C - Functions for normalizing qualifications.
**
**  Description:
**      This file used to contain the functions for putting query tree
**	qualifications into conjunctive normal form.  That is now done
**	in OPF, and all that's left here are a couple utility routines.
**
**	    pst_node_size - Return the size of a tree node
**	    pst_treedup - Duplicate a sub-tree
**
**
**  History:
**      03-apr-86 (jeff)    
**          Adapted from the normalization routines in 3.0
**	18-feb-88 (stec)
**	    All normalization code has been `IFDEFed' out.
**	    Normalization is now done in OPF.
**	02-mar-88 (stec)
**	    pst_treedup needs to copy exact size of the node.
**	02-may-88 (stec)
**	    Also copy PST_CURVAL node.
**	10-jun-88 (stec)
**	    Modified for DB procedures.
**	08-mar-89 (neil)
**	    Support for new rule nodes.
**	02-jun-89 (andre)
**	    wrote pst_dup_viewtree()
**	12-sep-89 (andre)
**	    Collapsed pst_dup_viewtree into an all new and improved
**	    pst_treedup().
**	11-nov-91 (rblumer)
**	  merged from 6.4:  19-aug-91 (andre)
**	    changed casting from (PTR) to (char *) so that pointer arithmetic 
**	    works on all platforms.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp(),
**	    psl_rptqry_tblids(), psl_cons_text(), psl_gen_alter_text(),
**	    psq_tmulti_out(), psq_1rptqry_out(), psq_tout(), pst_treedup().
**      16-apr-2001 (stial01)
**          pst_node_size() Added PST_MOP and PST_OPERAND cases
**	22-Jul-2004 (schka24)
**	    Delete old-code, been ifdef'ed out for 16 years.
**	21-Oct-2010 (kiria01) b124629
**	    Move DOT trace code out of here into pstprmdmp.c where it belongs.
[@history_template@]...
**/

/*{
** Name: pst_node_size 	- determine exact size of the node.
**
** Description:
**      Given a node, this function willl determine length of symbol and data
**	portions thereof.
**
** Inputs:
**      sym				ptr to the symbol representing the node
**
** Outputs:
**      symsize				length of the symbol part of the node
**	datasize			length of the data part of the node
**	err_blk				Filled in if an error happened
**	Returns:
**	    E_DB_SEVERE if unknown node type is encountered;
**	    E_DB_OK otherwise.
** Side Effects:
**	    None
**
** History:
**	14-nov-89 (andre) 
**	    fix bug 8624.  if pst_len != symsize+datasize, first check if it
**	    could be a node created for a repeat query TEXT or VCHR parameter,
**	    in which case db_length would be purposely set to -1.
**	    This fix was made before the function was split of from the body of
**	    pst_treedup().
**	27-dec-89 (andre)
**	    Borrowed from pst_treedup().  This function will be used in
**	    pst_treedup() and pst_trdup(), thus assuring that changes, if any,
**	    will have to be made in exactly one spot.
**	3-sep-99 (inkdo01)
**	    Added new nodes for case function.
**	12-feb-03 (inkdo01)
**	    Belatedly added sequence operation node.
**	3-apr-06 (dougi)
**	    Added PST_GSET, PST_GBCR, PST_GCL nodes for group by enhancements.
**     16-Jun-2009 (kiria01) SIR 121883
**	    Added PST_FWDVAR for scalar sub-query support
*/
DB_STATUS
pst_node_size(
	PST_SYMBOL	    *sym,
	i4		    *symsize,
	i4		    *datasize,
	DB_ERROR	    *err_blk)
{
    switch (sym->pst_type)
    {
	case PST_AND:
	case PST_OR:
	case PST_UOP:
	case PST_BOP:
	case PST_AOP:
	case PST_COP:
	case PST_MOP:
	    *symsize = sizeof(PST_OP_NODE);
	    break;
	case PST_CONST:
	    *symsize = sizeof(PST_CNST_NODE);

	    /*
	    ** Because of special treatment accorded to repeat query parameters
	    ** of type VCHR or TEXT (widely known as VLUPs), datasize of
	    ** PST_CONST node will be determined here.
	    ** There is a little trick here if we are dealing with a VLUP:
	    **	    in pst_2adparm(), db_length was reset to ADE_LEN_UNKNOWN,
	    **	    but we can still deduce it since
	    **	    pst_len = symsize + db_length.
	    */
	    if (sym->pst_dataval.db_data != (PTR) NULL)
	    {
		*datasize =
		    (sym->pst_dataval.db_length == ADE_LEN_UNKNOWN &&
		     sym->pst_value.pst_s_cnst.pst_tparmtype == PST_RQPARAMNO)
			? sym->pst_len - *symsize
			: sym->pst_dataval.db_length;
	    }
	    else
	    {
		*datasize = 0;
	    }

	    break;
	case PST_RESDOM:
	case PST_BYHEAD:
	    *symsize = sizeof(PST_RSDM_NODE);
	    break;
	case PST_VAR:
	    *symsize = sizeof(PST_VAR_NODE);
	    break;
	case PST_FWDVAR:
	    *symsize = sizeof(PST_FWDVAR_NODE);
	    break;
	case PST_AGHEAD:
	case PST_ROOT:
	case PST_SUBSEL:
	    *symsize = sizeof(PST_RT_NODE);
	    break;
	case PST_QLEND:
	case PST_TREE:
	case PST_NOT:
	case PST_WHLIST:
	case PST_WHOP:
	case PST_OPERAND:
	case PST_GSET:
	case PST_GCL:
	    *symsize = 0;
	    break;
	case PST_CASEOP:
	    *symsize = sizeof(PST_CASE_NODE);
	    break;
	case PST_SORT:
	    *symsize = sizeof(PST_SRT_NODE);
	    break;
	case PST_CURVAL:
	    *symsize = sizeof(PST_CRVAL_NODE);
	    break;
	case PST_RULEVAR:
	    *symsize = sizeof(PST_RL_NODE);
	    break;
	case PST_SEQOP:
	    *symsize = sizeof(PST_SEQOP_NODE);
	    break;
	case PST_GBCR:
	    *symsize = sizeof(PST_GROUP_NODE);
	    break;
	default:
	{
	    i4	err_code;
	    
	    (VOID) psf_error(E_PS0C03_BAD_NODE_TYPE, 0L, PSF_INTERR,
		&err_code, err_blk, 0);
	    return (E_DB_SEVERE);
	}
    }

    /*
    ** Now find the data size.  Datasize for PST_CONST node has already been
    ** determined
    */
    if (sym->pst_type != PST_CONST)
    {
	if (sym->pst_dataval.db_data != (PTR) NULL)
	{
	    *datasize = sym->pst_dataval.db_length;
	}
	else
	{
	    *datasize = 0;
	}
    }

    return(E_DB_OK);
}

/*{
** Name: pst_treedup	- Duplicate a view tree
**
** Description:
**	This function is a rewrite of the enhanced version of the original
**	pst_treedup. This rewrite removes the recursion requirements and generally
**	speeds things up abit.
**	In addition to copying the tree passed to it, it may perform requested
**	services for the caller.  For the time being it
**	"understands" 5 opcodes and the meanings of the masks are as follows:
**	PSS_1DUP_SEEK_AGHEAD --  while duplicating a tree, search for
**				 occurrences of AGHEAD with BYHEAD as its left
**				 child. If found, set an apprpriate bit in
**				 dup_cb->pss_tree_info.  Whenever ROOT or SUBSEL
**				 is duplicated reset tree_info to point at
**				 pst_mask1 in the newly duplicated node, as we
**				 want this information stored in the node.  In
**				 addition to that, if such occurrence is found
**				 in the outermost SUBSELECT (i.e. no SUBSEL
**				 nodes between the node where this occurrence
**				 was found and PST_ROOT), we want to
**				 communicate this knowledge to the caller,
**				 hence, if the bit was set in PST_ROOT's
**				 pst_mask1, it will be set in the mask passed
**				 by the caller inside dup_cb.  Doing so will
**				 allow us to determine if one or more members of
**				 the union had a AGHEAD/BYHEAD combination in
**				 their outer SUBSELECTs as well as pass the info
**				 to the caller of pst_treedup().
**				 This will not be done for the case when the
**				 bit is set inside SUBSEL's pst_mask1, as this
**				 in this case, information is only of interest
**				 while processing the corresponding SUBSELECT.
**	PSS_2DUP_RESET_JOINID -- while duplicating a tree, search for
**				 occurrences of operator nodes (PST_AND, PST_OR,
**				 PST_AOP, PST_BOP, PST_UOP, PST_COP), and if
**				 pst_joinid is != PST_NOJOIN, increment it by
**				 dup_cb->pst_numjoins.
**	PSS_3DUP_RENUMBER_BYLIST if set - renumber the bylist resdoms.
**	PSS_MAP_VAR_TO_RULEVAR --'replace' VAR node with copy of subtree in
**				 .pss_1ptr
**	PSS_5DUP_ATTACH_AGHEAD --if set, add AGHEAD addresses to YYAGG_NODE_PTR
**				 list, the head of which is in .pss_1ptr
**
**	To produce a copy of the original tree, it traverses the tree,
**	allocating space for each node based on the length in the node's
**	header, and copies the contents.
**
** Inputs:
**	dup_rb		    ptr to dup. request block
**	    pss_op_mask		    requested_services mask
**	    pss_numjoins	    number of joins found in the query so far
**	    pss_tree_info	    ptr to a mask used to communicate all sorts
**				    of useful info to the caller
**	    pss_mstream		    Memory stream
**	    pss_tree		    Pointer to tree to be duplicated
**	    pss_dup		    Place to put pointer to duplicate
**	    pss_err_blk		    Filled in if an error happens
**
** Outputs:
**	dup_rb		    ptr to dup. request block
**      *pss_dup			    Filled in with pointer to copy
**	*pss_tree_info		    may be set if a condition for which we
**				    are looking has occured
**	    PSS_GROUP_BY_IN_TREE    found an AGHEAD whose left child is a BYHEAD
**	pss_err_blk		    Filled in if an error happened
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic error
**	    E_DB_FATAL			Catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory
**
** History:
**	06-may-86 (jeff)
**          Adapted from treedup() in 3.0
**	02-mar-88 (stec)
**	    pst_treedup needs to copy exact size of the node.
**	02-may-88 (stec)
**	    Also copy PST_CURVAL node.
**	10-jun-88 (stec)
**	    Modified for DB procedures. Make adjustments for
**	    the new resdom and constant node sizes, i.e., make
**	    more robust for the case when node sizes change and
**	    we're retrieving old trees from catalogs.
**	08-mar-89 (neil)
**	    Support for new rule nodes.
**	11-sep-89 (andre)
**	    make changes to handle group IDs in the operator nodes.  To this end
**	    we will accept a number_of_joins_in_the_tree and increment
**	    meaningful (not PST_NOJOIN) group ids by the number of group ids
**	    in the rest of the query.
**	    Also added changes to allow searching for specific conditions (e.g.
**	    AGHEAD/BYHEAD combinations.)
**	1-oct-89 (andre)
**	    Propagate fix made in 7.0 line.  2 new masks will be "understood" by
**	    pst_treedup():
**	    PSS_3DUP_RENUMBER_BYLIST	--  renumber elements of BY list;
**	    PSS_4DUP_COUNT_RSDMS	--  after a tree rooted in RESDOM has
**					    been copied, increment
**					    dup_rb->pss_rsdm_no and use the
**					    result as a new resdom number.
**	22-jan-90 (andre)
**	    If there is a datavalue to copy, make sure that the destination
**	    db_data points at the newly allocated memory.  Until now, it would
**	    contain the value of the source db_data pointer, and so the source
**	    datavalue would be copied unto itself.
**	23-jan-90 (andre)
**	    make use of the newly defined pst_node_size().
**	27-mar-90 (andre)
**	    propagate fix for bug 20499.  1 new mask will be recognized by
**	    pst_treedup():
**	    PSS_5DUP_ATTACH_AGHEAD	-- attach all newly copied PST_AGHEAD
**					   nodes to the list of agheads whose
**					   address has been passed by the caller
**	10-jul-90 (andre)
**	    If PSS_3DUP_RENUMBER_BYLIST is set, renumber resdom nodes in the
**	    BY-list AFTER the BYHEAD node and its subtrees are processed.
**	    Previously, we would set PSS_4DUP_COUNT_RSDMS and use pss_num_rsdms
**	    to keep track of resdom numbers as they are being renumbered.  This
**	    approach fails since it relies on an assumption that there may be no
**	    BYHEAD nodes having another BYHEAD as a descendant, which is easily
**	    violated (e.g. retrieve (a=sum((t1.i) by avg((t1.i) by t1.i))).)
**	30-sep-92 (andre)
**	    pss_tree_info will no longer point at PST_RT_NODE.pst_mask1, new
**	    masks have been defined over it
**	01-oct-92 (andre)
**	    remember whether a tree being copied involved singleton subselects
**	    only when instructed to do so by the caller
**	    (dup_rb->pss_op_mask & PSS_4SEEK_SINGLETON_SUBSEL)
**	31-dec-92 (andre)
**	    pst_treedup() will no longer be asked to look for singleton
**	    subselects inside the view tree - this information will be stored
**	    inside the view tree header
**	04-jan-93 (andre)
**	    handle new opcode:
**		PSS_MAP_VAR_TO_RULEVAR - when set, every for every occurrence of
**		a PST_VAR node we will create a copy of a PST_RULEVAR node
**		pointed to by pss_1ptr and overwrite PST_RL_NODE.pst_rltno with
**		PST_VAR_NODE.pst_atno.db_att_id
**      10-Nov-2006 (kschendel)
**          Revise length-mismatch error message, treedup originals aren't
**          necessarily from rdf!
**	30-oct-07 (kibro01) b119378
**	    Do not recurse down RESDOM nodes - there could be hundreds of them
**	    and recursion is unnecessary.
**	22-Jan-2009 (kibro01) b120460
**	    Don't recurse down CONSTS either, like with RESDOM nodes.
**	    IN-clauses with many parameters end up with this.
**	30-Jun-2010 (kiria01) b124000
**	    Rewrite of function to remove the recursive nature. The recursion
**	    is simply replaced by a stack of 'to be done' pointers and a suitable
**	    algorithmn to handle the depth first needs of the original code.
**	    The stack used is initially in the from of the caller but if needed
**	    can expand into session heap.
*/


DB_STATUS
pst_treedup(
	PSS_SESBLK	   *sess_cb,
	PSS_DUPRB	   *dup_rb)
{
    DB_STATUS status = E_DB_OK;
    i4 err_code;
    PST_STK stk;
    PST_QNODE **nodep;
    bool descend = TRUE;
    PST_QNODE *subsel = NULL;

    if (dup_rb->pss_op_mask & PSS_1DUP_SEEK_AGHEAD)
    {
	if (dup_rb->pss_tree_info)
	    *dup_rb->pss_tree_info = 0;
	else
	{
	    psf_error(E_PS0002_INTERNAL_ERROR, 0L, PSF_INTERR,
		      &err_code, dup_rb->pss_err_blk, 0);
	    return E_DB_ERROR;
	}
    }
    if (!(*dup_rb->pss_dup = dup_rb->pss_tree))
	return status;

    PST_STK_INIT(stk, sess_cb);

    /* We will Descend the source tree via the address of a pointer to the tree.
    ** Allocations will be done in place and full copies taken of the current node
    ** and the allocated address written back to the address of a pointer now to
    ** the top of the new tree. (Immediatly after allocation the left and right
    ** pointer in the new block will refer to the OLD respective subtrees) */
    nodep = dup_rb->pss_dup;
    while (nodep)
    {
	PST_QNODE *node = *nodep;
	/* Copy the node insitu unless this is a non-terminal that we've already
	** copied, in which case descend will be FALSE */
	if (descend || !node->pst_left && !node->pst_right)
	{
	    PST_QNODE *onode = node, *ovar = NULL;
	    i4 nsize, datasize;
	    /* 'descend' will be set if we are seeing a non-terminal for the first
	    ** time or a leaf node. Either way we allocate the duplicate and might
	    ** stack it if needed. */

	    if (onode->pst_sym.pst_type == PST_VAR &&
		dup_rb->pss_op_mask & PSS_MAP_VAR_TO_RULEVAR)
	    {
		/* We were asked to map VAR node into RULEVAR - temporarily set onode
		** and nodep to refer to the RULEVAR node which will be replicated.
		** As nodep roves over the pointers in the new copy, we may assign
		** the rulevar tree into the current. (ovar refers to real VAR) */
		ovar = onode;
		onode = *nodep = (PST_QNODE *)dup_rb->pss_1ptr;
		/* Assert nodetype */
		if (onode->pst_sym.pst_type != PST_RULEVAR)
		{
		    psf_error(E_PS0002_INTERNAL_ERROR, 0L, PSF_INTERR,
			      &err_code, dup_rb->pss_err_blk, 0);
		    status = E_DB_ERROR;
		    goto cleanup_exit;
		}
	    }
	    if (status = pst_node_size(&onode->pst_sym, &nsize, &datasize,
			   dup_rb->pss_err_blk))
		goto cleanup_exit;

	    /* Allocate enough space */
	    nsize += sizeof(PST_QNODE) - sizeof(PST_SYMVALUE);
	    if (status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
			    nsize + datasize, nodep, dup_rb->pss_err_blk))
		goto cleanup_exit;

	    node = *nodep; /* The new memory */
	    /* Copy the old node to the new node */
	    MEcopy((PTR)onode, nsize, (PTR)node);
	    if (datasize)
	    {
		/* NOTE:
		** Of the allocated memory, the initial part was for the node itself
		** and the remaining datasize bytes will be used for the datavalue */
		node->pst_sym.pst_dataval.db_data = (char*)node + nsize;
		MEcopy((PTR)onode->pst_sym.pst_dataval.db_data,
			datasize, (PTR) node->pst_sym.pst_dataval.db_data);
	    }

	    if (ovar)
	    {
		/* Nodetype Asserted above */
		node->pst_sym.pst_value.pst_s_rule.pst_rltno =
		    ovar->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
	    }
	}

	/****** Start reduced recursion, depth first tree scan ******/
	if (descend && (node->pst_left || node->pst_right))
	{
	    switch (node->pst_sym.pst_type)
	    {
	    case PST_ROOT:
	    case PST_SUBSEL:
		if (node->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
		{
		    pst_push_item(&stk,
			(PTR)&node->pst_sym.pst_value.pst_s_root.pst_union.pst_next);
		    /* Mark that the top node needs descending (and allocating) */
		    pst_push_item(&stk, (PTR)PST_DESCEND_MARK);
		}
		subsel = node; /* Track enclosing root/subsel */
		/* Clear GB flag & let be set if seen (via subsel) */
		subsel->pst_sym.pst_value.pst_s_root.pst_mask1 &= ~PST_3GROUP_BY;
		break;
	    }
	    /* Delay node evaluation */
	    pst_push_item(&stk, (PTR)nodep);
	    if (node->pst_left)
	    {
		if (node->pst_right)
		{
		    /* Delay RHS */
		    pst_push_item(&stk, (PTR)&node->pst_right);
		    if (node->pst_right->pst_right ||
				node->pst_right->pst_left)
			/* Mark that the top node needs descending */
			pst_push_item(&stk, (PTR)PST_DESCEND_MARK);
		}
		nodep = &node->pst_left;
		continue;
	    }
	    else if (node->pst_right)
	    {
		nodep = &node->pst_right;
		continue;
	    }
	}

	/*** Depth first traverse ***/
	switch (node->pst_sym.pst_type)
	{
	case PST_UOP:
	case PST_BOP:
	case PST_MOP:
	case PST_AOP:
	case PST_COP:
	case PST_AND:
	case PST_OR:
	    if (dup_rb->pss_op_mask & PSS_2DUP_RESET_JOINID &&
		dup_rb->pss_num_joins != PST_NOJOIN &&
		node->pst_sym.pst_value.pst_s_op.pst_joinid != PST_NOJOIN)
	    {
		/* Add max as offset to get new number */
		node->pst_sym.pst_value.pst_s_op.pst_joinid += dup_rb->pss_num_joins;

		if (node->pst_sym.pst_value.pst_s_op.pst_joinid > PST_NUMVARS - 1)
		{
		    /* Exceeded joinids */
		    psf_error(3100L, 0L, PSF_USERERR, &err_code, dup_rb->pss_err_blk, 0);
		    status = E_DB_ERROR;
		    goto cleanup_exit;
		}
	    }
	    break;
	case PST_AGHEAD:
	    if (dup_rb->pss_op_mask & PSS_5DUP_ATTACH_AGHEAD)
	    {
		YYAGG_NODE_PTR *new_agg_node;

		/* Save location of new agg head */
		if (status = psf_malloc(sess_cb, dup_rb->pss_mstream,
				    (i4) sizeof(YYAGG_NODE_PTR), 
				    (PTR *)&new_agg_node, dup_rb->pss_err_blk))
		    goto cleanup_exit;
		new_agg_node->agg_next = *((YYAGG_NODE_PTR **)dup_rb->pss_1ptr);
		new_agg_node->agg_node = node;
		*((YYAGG_NODE_PTR **)dup_rb->pss_1ptr) = new_agg_node;
	    }
	    /* The check for PSS_1DUP_SEEK_AGHEAD and action of setting of
	    ** PSS_GROUP_BY_IN_TREE is handled in BYHEAD directly
	    ** and the outer setting of PSS_GROUP_BY_IN_TREE to return
	    ** the .pss_tree_info is done in the top PST_ROOT nodes */
	    break;
	case PST_ROOT:
	    if (dup_rb->pss_op_mask & PSS_1DUP_SEEK_AGHEAD)
	    {
		/* For root neet to update real caller .pss_tree_info */
		if (node->pst_sym.pst_value.pst_s_root.pst_mask1 & PST_3GROUP_BY)
		    *dup_rb->pss_tree_info |= PSS_GROUP_BY_IN_TREE;
	    }
	    break;
	case PST_SUBSEL:
	    /* Nothing much to do now - the PSS_1DUP_SEEK_AGHEAD check for
	    ** PSS_GROUP_BY_IN_TREE is done directly in the BYHEAD by accessing
	    ** the ansestor nodes appropriatly. */
	    /* Reset 'active subsel' */
	    subsel = pst_antecedant_by_2types(&stk, NULL, PST_SUBSEL, PST_ROOT);
	    break;
	case PST_BYHEAD:
	    if (dup_rb->pss_op_mask & PSS_1DUP_SEEK_AGHEAD)
	    {
		PST_QNODE *rt_node = pst_antecedant_by_1type(&stk, NULL, PST_AGHEAD);
		/* For us to find a BYHEAD implies an explicit, user specified, GROUP BY
		** as opposed to one we've added. If this flag is noted in the subsel code
		** on uncorrelated, we will leave the subselect unflattend */
		if (subsel)
		    subsel->pst_sym.pst_value.pst_s_root.pst_mask1 |= PST_3GROUP_BY;
		if (rt_node)
		    rt_node->pst_sym.pst_value.pst_s_root.pst_mask1 |= PST_3GROUP_BY;
	    }
	    if (dup_rb->pss_op_mask & PSS_3DUP_RENUMBER_BYLIST)
	    {
		register PST_QNODE *bylist_elem = node->pst_left;
		i4 renumber = 0;
		/* If this is the top of the BY list, count the RESDOM nodes
		** and use to renumber. */
		while(bylist_elem->pst_sym.pst_type == PST_RESDOM)
		{
		    renumber++;
		    bylist_elem = bylist_elem->pst_left;
		}
		bylist_elem = node->pst_left;
		while (bylist_elem->pst_sym.pst_type == PST_RESDOM)
		{
		    bylist_elem->pst_sym.pst_value.pst_s_rsdm.pst_rsno = renumber--;
		    bylist_elem = bylist_elem->pst_left;
		}
	    }
	    break;
	}
	nodep = (PST_QNODE**)pst_pop_item(&stk);
	if (descend = (nodep == PST_DESCEND_MARK))
	    nodep = (PST_QNODE**)pst_pop_item(&stk);
    }
cleanup_exit:
    pst_pop_all(&stk);
    return status;
}

/*{
** Name: pst_push_item -     Push item on recursion stack
**
** Description:
**	Hybrid recursion stack, initial allocation on stack
**	with heap overflow.
**
** Inputs:
**	cb		Parser context for memory
**	base		Stack base block
**	item		Item to be stored
**
** Outputs:
**	*base		Structure updated
**
**	Returns:
**	    sts		Status return
**	Exceptions:
**	    None
**
** Side Effects:
**	May allocate session memory.
**
** Return:
**	E_DB_OK		Everything fine
**	E_DB_WARN	A NULL was attempted to be pushed but has been
**			ignored as this would be retrieved as an
**			end-of-stack condition.
**
** History:
**	27-Oct-2009 (kiria01) SIR 121883
**	    Created for flattened out stack recursion
*/

STATUS
pst_push_item(PST_STK *base, PTR item)
{
    STATUS sts = E_DB_OK;
    /* Point to true list head block */
    struct PST_STK1 *stk = base->stk.link;
    if (!item)
	return E_DB_WARN;
    if (!stk)
	stk = &base->stk;

    /* Is there space */
    if (stk->sp >= PST_N_STK)
    {
	DB_ERROR err_blk;
	/* No space - one on free? */
	if (base->stk.link = base->cb->pss_stk_freelist)
	    base->cb->pss_stk_freelist = base->stk.link->link;
	/* .. one from heap */
	else if (sts = psf_malloc(base->cb, &base->cb->pss_ostream, (i4) sizeof(struct PST_STK1),
		(PTR *) &base->stk.link, &err_blk))
		return sts;;

	/* Link in new block */
	base->stk.link->link = stk;
        stk = base->stk.link;
	/* Init sp */
        stk->sp = 0;
    }
    /* push block */
    stk->list[stk->sp++] = item;
    return sts;
}

/*{
** Name: pst_pop_item	- Retrieve top item off recursion stack
**
** Description:
**	Hybrid recursion stack pop item
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
**	27-Oct-2009 (kiria01) SIR 121883
**	    Created for flattened out stack recursion
*/

PTR
pst_pop_item(PST_STK *base)
{
    /* Point to true list head block */
    struct PST_STK1 *stk = base->stk.link;
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
	stk->link = base->cb->pss_stk_freelist;
	base->cb->pss_stk_freelist = stk;
	/* Point to next block*/
        stk = base->stk.link;
    }
    /* Pop actual item */
    return stk->list[--stk->sp];
}

/*{
** Name: pst_pop_all    - Release resources from the recursion stack
**
** Description:
**	Hybrid recursion stack pop all item
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
**	27-Oct-2009 (kiria01) SIR 121883
**	    Created for flattened out stack recursion
*/

VOID
pst_pop_all(PST_STK *base)
{
    if (base->stk.link)
    {
	struct PST_STK1 *stk;
	for (stk = base->stk.link; stk != &base->stk; stk = base->stk.link)
	{
	    base->stk.link = stk->link;
	    stk->link = base->cb->pss_stk_freelist;
	    base->cb->pss_stk_freelist = stk;
	}
    }
    base->stk.sp = 0;
}

/*{
** Name: pst_parent_node	- Find parent off recursion stack
**
** Description:
**	Nondestructive traverse of Hybrid recursion stack looking for
**	parent node.
**	The tree walking algorithms using this mechanism touch each
**	leaf node once and everything else twice - allowing scope to be
**	statically modelled and ancestors identified easily without needing
**	to carry data in stack parameters in recursion.
**	Given:			(ignoring TREE) traversal will be: 
**                      ROOT                      ROOT    AGH 2   AND 2
**                     /    \                     RSDa    RSDb2   ROOT 2
**                  RSDa      \                   RSDb    VAR
**                 /    \       \                 AGH     RSDa2
**              RSDb     VAR     AND              BYH     AND
**             /   \            /   \             RSDc    BOP
**          TREE    AGH       BOP    \            VAR     VAR
**                 /   \     /   \    BOP         RSDc2   VAR
**              BYH   qual  VAR VAR  /   \        AOP     BOP 2
**             /   \                VAR const     VAR     BOP
**          RSDc    AOP                           AOP 2   VAR
**         /   \    /                             BYH 2   const
**       TREE VAR  VAR                            qual -> BOP 2 ->
**
**	As can be seen, a non-terminal node is seen at beginning of scope
**	when we also push it onto the stack (a true parent). However, we also
**	have to push on the stack an entry for any descendant sub-tree of
**	this non-terminal that we are not immediately descending - so we can
**	come back to it. Clearly, pushing this entry on the stack, it must
**	be distinguished from a parent node with children are being processed.
**	To do this we add a marker on the stack that tells us that the next
**	node popped from the stack must be decended (properly processed). Such
**	a node will be pushed back on the stack, this time unmarked - a true
**	parent. Thus for example, if we are processing the AOP having come to
**	it on its second pass (closing scope), the stack will look like:
**
**      ROOT AND * RSDa VAR RSDb AGH qual * BYH (AOP just popped off)
**
**	From this, following back up the list skipping descend marked entries
**	gives us:
**
**	ROOT RSDa RSDb AGH BYH
**
**	and these are the true antecedants.
**	NOTE that there is a special case when dealing with terminal nodes
**	such as VAR. With these no descend mark is placed and instead the
**	leafness aspect is noted by inspection. If this were not the case,
**	it would be necessary to place a NULL address on the stack an in this
**	implemetation, that is not possible as such a NULL would be
**	indistinquishable from the end of stack condition. For tre walking
**	this is not a limitation as these routines are designed for working
**	withthe extra level of indirection to trivially suppport modification
**	of the tree.
**
**
** Inputs:
**	base		Stack base block
**	child		child whose parent to find. If passed as NULL
**			the top 'node' of stack is returned. This is not
**			necessarily the same as what pst_pop_item would
**			destructivly return.
**
** Outputs:
**
**	Returns:
**	    parent or NULL if not present
**
**	Exceptions:
**	    None
**
** Side Effects:
**	None - stack left intact.
**
** History:
**	27-Oct-2009 (kiria01) SIR 121883
**	    Created for flattened out stack recursion
**	25-Mar-2010 (kiria01) b123535
**	    Cater for the potential that a nodes parent
**	    may not be in the stack due to tree additions and
**	    also arrange to skip items that were marked for
**	    'descend' as these will not be on the ancestry line
**	    but will be merely siblings or aunts/uncles.
**	07-Apr-2010 (kiria01) b123535
**	    Also skip termninals - as these cannot be descended
**	    they carry no PST_DESCEND_MARK.
*/

PST_QNODE *
pst_parent_node(PST_STK *base, PST_QNODE *child)
{
    i4 i;
    bool skip = FALSE;
    /* Point to true list head block */
    struct PST_STK1 *stk = base->stk.link;
    if (!child)
    {
	for(;;)
	{
	    if (!stk)
		stk = &base->stk;
	    for (i = stk->sp; i > 0;)
	    {
		PST_QNODE **p = (PST_QNODE **)(stk->list[--i]);
		if (skip)
		    skip = FALSE;
		else if (p == PST_DESCEND_MARK)
		    skip = TRUE;
		else if (p && (!*p ||	/* Also skip terminals */
			(*p)->pst_left ||(*p)->pst_right))
		    return *p;
	    }
	    if (stk == &base->stk)
		return NULL;
	    /* Point to next block*/
	    stk = stk->link;
	}
    }
    else
    {
	PST_QNODE *prev = NULL;
	for(;;)
	{
	    if (!stk)
		stk = &base->stk;
	    i = stk->sp;
	    while (i > 0)
	    {
		PST_QNODE **p = (PST_QNODE **)(stk->list[--i]);
		if (skip)
		    skip = FALSE;
		else if (p == PST_DESCEND_MARK)
		    skip = TRUE;
		else if (p && (!*p ||	/* Also skip terminals */
			(*p)->pst_left ||(*p)->pst_right))
		{
		    PST_QNODE *parent = *p;
		    if (prev == child ||
			parent->pst_left == child ||
			parent->pst_right == child)
			return parent;
		    prev = parent;
		}
	    }
	    if (stk == &base->stk)
		return NULL;

	    /* Point to next block*/
	    stk = stk->link;
	}
    }
}

/*{
** Name: pst_antecedant_by_1type - Find antecedant off recursion stack
**
** Description:
**	Nondestructive traverse of Hybrid recursion stack looking for
**	parent node.
**
** Inputs:
**	base		Stack base block
**	child		child whose antecedant to find. 
**	t1...		Types to look for
**
** Outputs:
**
**	Returns:
**	    node or NULL if not present
**
**	Exceptions:
**	    None
**
** Side Effects:
**	None - stack left intact.
**
** History:
**	27-Oct-2009 (kiria01) SIR 121883
**	    Created for flattened out stack recursion
*/

PST_QNODE *
pst_antecedant_by_1type(
	PST_STK *base,
	PST_QNODE *child,
	PST_TYPE t1)
{
    while (child = pst_parent_node(base, child))
    {
	if (child->pst_sym.pst_type == t1)
	    return child;
    }
    return child;
}

PST_QNODE *
pst_antecedant_by_2types(
	PST_STK *base,
	PST_QNODE *child,
	PST_TYPE t1,
	PST_TYPE t2)
{
    while (child = pst_parent_node(base, child))
    {
	if (child->pst_sym.pst_type == t1 ||
	    child->pst_sym.pst_type == t2)
	    return child;
    }
    return child;
}

PST_QNODE *
pst_antecedant_by_3types(
	PST_STK *base,
	PST_QNODE *child,
	PST_TYPE t1,
	PST_TYPE t2,
	PST_TYPE t3)
{
    while (child = pst_parent_node(base, child))
    {
	if (child->pst_sym.pst_type == t1 ||
	    child->pst_sym.pst_type == t2 ||
	    child->pst_sym.pst_type == t3)
	    return child;
    }
    return child;
}

PST_QNODE *
pst_antecedant_by_4types(
	PST_STK *base,
	PST_QNODE *child,
	PST_TYPE t1,
	PST_TYPE t2,
	PST_TYPE t3,
	PST_TYPE t4)
{
    while (child = pst_parent_node(base, child))
    {
	if (child->pst_sym.pst_type == t1 ||
	    child->pst_sym.pst_type == t2 ||
	    child->pst_sym.pst_type == t3 ||
	    child->pst_sym.pst_type == t4)
	    return child;
    }
    return child;
}

/*{
** Name: pst_qtree_compare* - Compare two query trees
**
** Description:
**	Compares two query trees and returns an ordering.
**	pst_qtree_compare_norm returns a biased ordering for
**	normalisation.
**
** Inputs:
**	ctx		Control block:
**	 .stk		Recursion stack - must be initialised
**	 .cb		PSS_SESBLK Parser context
**	 .rngtabs	PSS_RNGTAB Range table vector
**	 .n_covno	Number of valid VAR vno mapping
**	 .co_vnos[]	VAR vno mapping pairs vector
**	a		Tree 1 (earlier)
**	b		Tree 2 (later)
**	fixup		Bool as to whether vnos be fixed up
**
** Outputs:
**	ctx
**	 .same		TRUE if trees match
**	 .reversed	Usually [a] expected to be 'earlier' than [b]
**			but if not .reversed will be set.
**	 .inconsistant	Error flag set if [a] & [b] not strictly
**			ordered.
**
**	Returns:
**	    cmp		Difference result.
**
**	Exceptions:
**	    None
**
** Side Effects:
**	None - stack left intact.
**
** History:
**	
**	27-Oct-2009 (kiria01) SIR 121883
**	    Created for flattened out stack recursion
**	03-Dec-2009 (kiria01) SIR 121883
**	    Extended for group by and order by requirements.
*/

i4
pst_qtree_compare(
    PST_STK_CMP *ctx,
    PST_QNODE **a,
    PST_QNODE **b,
    bool fixup)
{
    register i4	res = 0;
    i4		depth = 0;
    i4		whlist_count = 0;
    bool	descend = TRUE;

    ctx->same = TRUE;
    if (!fixup)
	ctx->reversed = ctx->inconsistant = FALSE;

    while (a || b)
    {
	PST_SYMVALUE *av, *bv;
	if (!a)
	{
	    res = -1;
	    goto different;
	}
	if (!b)
	{
	    res = 1;
	    goto different;
	}
	if (res = (*a)->pst_sym.pst_type - (*b)->pst_sym.pst_type)
	{
	    /* Arrange that CONSTs drift left and QLEND right */
	    if ((*a)->pst_sym.pst_type == PST_CONST ||
		(*a)->pst_sym.pst_type == PST_QLEND)
		res = 1;
	    else if ((*b)->pst_sym.pst_type == PST_CONST ||
		     (*b)->pst_sym.pst_type == PST_QLEND)
		res = -1;
	    goto different;
	}

	/****** Start reduced recursion, depth first tree scan ******/
	if (descend && ((*a)->pst_left || (*a)->pst_right ||
			(*b)->pst_left || (*b)->pst_right))
	{
	    /* Delay node evaluation (types match) */
	    pst_push_item(&ctx->stk, (PTR)b);
	    pst_push_item(&ctx->stk, (PTR)a);
	    /* Increment depth counters for children */
	    switch ((*a)->pst_sym.pst_type)
	    {
	    case PST_WHLIST: whlist_count++; break;
	    case PST_ROOT:
	    case PST_SUBSEL:
		depth++;
		if ((*a)->pst_sym.pst_value.pst_s_root.pst_union.pst_next &&
		    (*b)->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
		{
		    pst_push_item(&ctx->stk, (PTR)&(*b)->pst_sym.pst_value.pst_s_root.pst_union.pst_next);
		    pst_push_item(&ctx->stk, (PTR)&(*a)->pst_sym.pst_value.pst_s_root.pst_union.pst_next);
		    pst_push_item(&ctx->stk, (PTR)PST_DESCEND_MARK);
		    if ((res = (*a)->pst_sym.pst_value.pst_s_root.pst_union.pst_setop -
			(*b)->pst_sym.pst_value.pst_s_root.pst_union.pst_setop) ||
			(res = (*a)->pst_sym.pst_value.pst_s_root.pst_union.pst_dups -
			(*b)->pst_sym.pst_value.pst_s_root.pst_union.pst_dups) ||
			(res = (*a)->pst_sym.pst_value.pst_s_root.pst_union.pst_nest -
			(*b)->pst_sym.pst_value.pst_s_root.pst_union.pst_nest))
		    {
			goto different;
		    }
		}
		else if ((*a)->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
		{
		    res = -1;
		    goto different;
		}
		else if ((*b)->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
		{
		    res = 1;
		    goto different;
		}
		break;
	    }
	    if ((*a)->pst_left)
	    {
		if (!(*b)->pst_left)
		{
		    res = -1;
		    goto different;
		}

		if ((*a)->pst_right)
		{
		    if (!(*b)->pst_right)
		    {
			res = -1;
			goto different;
		    }
		    if (res = (*a)->pst_right->pst_sym.pst_type -
			    (*b)->pst_right->pst_sym.pst_type)
		    {
			if ((*a)->pst_right->pst_sym.pst_type == PST_CONST ||
			    (*a)->pst_right->pst_sym.pst_type == PST_QLEND)
			    res = 1;
			else
			if ((*b)->pst_right->pst_sym.pst_type == PST_CONST ||
			    (*b)->pst_right->pst_sym.pst_type == PST_QLEND)
			    res = -1;
			goto different;
		    }

		    /* Delay RHS (types match) */
		    pst_push_item(&ctx->stk, (PTR)&(*b)->pst_right);
		    pst_push_item(&ctx->stk, (PTR)&(*a)->pst_right);
		    if ((*a)->pst_right->pst_right ||
				(*a)->pst_right->pst_left)
			/* Mark that the top nodes need descending */
			pst_push_item(&ctx->stk, (PTR)PST_DESCEND_MARK);
		}
		a = &(*a)->pst_left;
		b = &(*b)->pst_left;
		continue;
	    }
	    else if ((*a)->pst_right)
	    {
		if ((*b)->pst_left || !(*b)->pst_right)
		{
		    res = 1;
		    goto different;
		}
		a = &(*a)->pst_right;
		b = &(*b)->pst_right;
		continue;
	    }
	    else
	    {
		res = 1;
		goto different;
	    }
	}

	/*** Depth first traverse ***/
	av = &(*a)->pst_sym.pst_value;
	bv = &(*b)->pst_sym.pst_value;
	switch ((*a)->pst_sym.pst_type)
	{
	    
	case PST_UOP:
	case PST_BOP:
	case PST_AOP:
	case PST_COP:
	case PST_MOP:
	    if ((res = av->pst_s_op.pst_opno - bv->pst_s_op.pst_opno) ||
		(res = av->pst_s_op.pst_opinst - bv->pst_s_op.pst_opinst) ||
		(res = av->pst_s_op.pst_distinct - bv->pst_s_op.pst_distinct) ||
		(res = av->pst_s_op.pst_opmeta - bv->pst_s_op.pst_opmeta) ||
		(res = av->pst_s_op.pst_escape - bv->pst_s_op.pst_escape) ||
		(res = av->pst_s_op.pst_pat_flags - bv->pst_s_op.pst_pat_flags) ||
		(res = av->pst_s_op.pst_flags - bv->pst_s_op.pst_flags))
	    {
		goto different;
	    }
	    if (av->pst_s_op.pst_joinid != bv->pst_s_op.pst_joinid)
	    {
	    }
	    break;
	case PST_CONST:
	    /* Single out real constants */
	    if ((res = (av->pst_s_cnst.pst_tparmtype == PST_USER &&
			!av->pst_s_cnst.pst_parm_no) -
		       (bv->pst_s_cnst.pst_tparmtype == PST_USER &&
			!bv->pst_s_cnst.pst_parm_no)))
		goto different;
	    if ((res = av->pst_s_cnst.pst_tparmtype - bv->pst_s_cnst.pst_tparmtype) ||
		(res = av->pst_s_cnst.pst_parm_no - bv->pst_s_cnst.pst_parm_no) ||
		(res = av->pst_s_cnst.pst_pmspec - bv->pst_s_cnst.pst_pmspec) ||
		(res = av->pst_s_cnst.pst_cqlang - bv->pst_s_cnst.pst_cqlang))
	    {
		goto different;
	    }
	    if ((res = (*a)->pst_sym.pst_dataval.db_datatype - (*b)->pst_sym.pst_dataval.db_datatype) ||
		(res = (*a)->pst_sym.pst_dataval.db_length - (*b)->pst_sym.pst_dataval.db_length) ||
		(res = (*a)->pst_sym.pst_dataval.db_prec - (*b)->pst_sym.pst_dataval.db_prec))
	    {
		goto different;
	    }
	    if ((*a)->pst_sym.pst_dataval.db_data && (*b)->pst_sym.pst_dataval.db_data)
	    {
		if (res = MEcmp((*a)->pst_sym.pst_dataval.db_data, (*b)->pst_sym.pst_dataval.db_data,
			    (*a)->pst_sym.pst_dataval.db_length))
		    goto different;
	    }
	    else if ((*a)->pst_sym.pst_dataval.db_data || (*b)->pst_sym.pst_dataval.db_data)
	    {
		if ((*a)->pst_sym.pst_dataval.db_data)
		    res = -1;
		else
		    res = 1;
		goto different;
	    }
	    break;
	case PST_RESDOM:
	case PST_BYHEAD:
	    if (res = av->pst_s_rsdm.pst_ttargtype - bv->pst_s_rsdm.pst_ttargtype)
	    {
		goto different;
	    }
	    break;
	case PST_FWDVAR:
	    res = 0;
	    goto different;
	case PST_VAR:
	    if ((res = av->pst_s_var.pst_atno.db_att_id -
					bv->pst_s_var.pst_atno.db_att_id) ||
		(res = MEcmp(&av->pst_s_var.pst_atname, &bv->pst_s_var.pst_atname,
			sizeof(av->pst_s_var.pst_atname))) ||
		!ctx->rngtabs &&
		    (res = av->pst_s_var.pst_vno - bv->pst_s_var.pst_vno))
	    {
		goto different;
	    }
	    res = av->pst_s_var.pst_vno - bv->pst_s_var.pst_vno;
	    if (res && ctx->rngtabs)
	    {
		PSS_RNGTAB *at = ctx->rngtabs[av->pst_s_var.pst_vno];
		PSS_RNGTAB *bt = ctx->rngtabs[bv->pst_s_var.pst_vno];
		u_i4 i;
		/* vno values differ - if in a sub-select context, see
		** if they refer to the same tables and if so, we will
		** tentatively accept as the same as long as all the
		** subsequent vno match consistently. 
		** We will also return the result. */
		if (!depth ||
		    (res = at->pss_rgtype - bt->pss_rgtype) ||
		    (res = at->pss_rgparent - bt->pss_rgparent) ||
		    (res = at->pss_tabid.db_tab_base - bt->pss_tabid.db_tab_base) ||
		    (res = at->pss_tabid.db_tab_index - bt->pss_tabid.db_tab_index))
		{
		    goto different;
		}
		for (i = 0; i < ctx->n_covno; i++)
		{
		    if (ctx->co_vnos[i].a == av->pst_s_var.pst_vno)
		    {
			if (ctx->co_vnos[i].b == bv->pst_s_var.pst_vno)
			{
			    if (fixup)
			    {
				i4 lose = ctx->reversed
					? ctx->co_vnos[i].a
					: ctx->co_vnos[i].b;
				i4 keep = ctx->reversed
					? ctx->co_vnos[i].b
					: ctx->co_vnos[i].a;
				if (ctx->rngtabs[keep]->pss_used)
				{
				    PSS_RNGTAB *rv = ctx->rngtabs[lose];
				    av->pst_s_var.pst_vno = keep;
				    bv->pst_s_var.pst_vno = keep;
				    if (rv->pss_used)
				    {
					PST_QNODE *rt = pst_antecedant_by_2types(
					    &ctx->stk, ctx->reversed
					    ?*a:*b, PST_SUBSEL, PST_AGHEAD);
					BTset(keep, (char*)&rt->pst_sym.pst_value.pst_s_root.pst_tvrm);
					BTclear(lose, (char*)&rt->pst_sym.pst_value.pst_s_root.pst_tvrm);
					rv->pss_used = FALSE;
					rv->pss_rgno = -1;
					/* Clear the inner and the outer relation mask
					** It is unlikely that these will be set. */
					MEfill(sizeof(PST_J_MASK), 0, (PTR)&rv->pss_inner_rel);
					MEfill(sizeof(PST_J_MASK), 0, (PTR)&rv->pss_outer_rel);
				    }
				}
			    }
			    break;
			}
			/* vno not consistant */
			goto different;
		    }
		    else if (ctx->co_vnos[i].b == bv->pst_s_var.pst_vno)
			/* vno not consistant */
			goto different;
		}
		if (i == ctx->n_covno)
		{
		    /* new vno - accept for now */
		    ctx->co_vnos[i].a = av->pst_s_var.pst_vno;
		    ctx->co_vnos[i].b = bv->pst_s_var.pst_vno;
		    if (ctx->co_vnos[i].a > ctx->co_vnos[i].b)
			if (!i)
			    ctx->reversed = TRUE;
			else if (!ctx->reversed)
			    ctx->inconsistant = TRUE;
		    ctx->n_covno++;
		}
	    }
	    break;
	case PST_ROOT:
	case PST_SUBSEL:
	    depth--;
	    /*FALLTHROUGH*/
	case PST_AGHEAD:
	    if ((res = av->pst_s_root.pst_dups - bv->pst_s_root.pst_dups) ||
		(res = av->pst_s_root.pst_qlang - bv->pst_s_root.pst_qlang) ||
		(res = av->pst_s_root.pst_mask1 - bv->pst_s_root.pst_mask1) ||
		(res = av->pst_s_root.pst_project - bv->pst_s_root.pst_project))
	    {
		goto different;
	    }
	    break;
	case PST_SORT:
	    if ((res = av->pst_s_sort.pst_srvno - bv->pst_s_sort.pst_srvno) ||
		(res = av->pst_s_sort.pst_srasc - bv->pst_s_sort.pst_srasc))
	    {
		goto different;
	    }
	    break;
	case PST_CURVAL:
	    if (res = MEcmp(&av->pst_s_curval, &bv->pst_s_curval,
			sizeof(av->pst_s_curval)))
	    {
		goto different;
	    }
	    break;
	case PST_RULEVAR:
	    if ((res = av->pst_s_rule.pst_rltype - bv->pst_s_rule.pst_rltype) ||
		(res = av->pst_s_rule.pst_rltargtype - bv->pst_s_rule.pst_rltargtype) ||
		(res = av->pst_s_rule.pst_rltno - bv->pst_s_rule.pst_rltno))
	    {
		goto different;
	    }
	    break;
	case PST_TAB:
	    if (res = MEcmp(&av->pst_s_tab, &bv->pst_s_tab,
			sizeof(av->pst_s_tab)))
	    {
		goto different;
	    }
	    break;
	case PST_CASEOP:
	    if ((res = av->pst_s_case.pst_caselen - bv->pst_s_case.pst_caselen) ||
		(res = av->pst_s_case.pst_casedt - bv->pst_s_case.pst_casedt) ||
		(res = av->pst_s_case.pst_caseprec - bv->pst_s_case.pst_caseprec) ||
		(res = av->pst_s_case.pst_casetype - bv->pst_s_case.pst_casetype))
	    {
		goto different;
	    }
	    break;
	case PST_SEQOP:
	    if (res = MEcmp(&av->pst_s_seqop, &bv->pst_s_seqop, sizeof (av->pst_s_seqop)))
	    {
		goto different;
	    }
	    break;
	case PST_GBCR:
	    if (res = av->pst_s_group.pst_groupmask - bv->pst_s_group.pst_groupmask)
	    {
		goto different;
	    }
	    break;

	case PST_AND:
	case PST_OR:
	case PST_NOT:
	case PST_QLEND:
	case PST_TREE:
	case PST_OPERAND:
	case PST_WHLIST:
	case PST_WHOP:
	case PST_GCL:
	case PST_GSET:
	    break;
	}
	a = (PST_QNODE **)pst_pop_item(&ctx->stk);
	if (descend = (a == PST_DESCEND_MARK))
	    a = (PST_QNODE **)pst_pop_item(&ctx->stk);
	b = (PST_QNODE **)pst_pop_item(&ctx->stk);
	continue;
different:
	ctx->same = FALSE;
	break;
    }
    pst_pop_all(&ctx->stk);
    return res;
}

i4
pst_qtree_compare_norm(
    PST_STK_CMP *ctx,
    PST_QNODE **a,
    PST_QNODE **b)
{
    u_i4 asize, bsize;

    ctx->same = FALSE;
    if (!a && !b)
    {
	ctx->same = TRUE;
	return 0;
    }
    if (!a)
	return 1;
    if (!b)
	return -1;
    /* Make minus unweighted to ensure that sub-trees that
    ** are simple negatives have the same weight */
    while (*a &&
	    (*a)->pst_sym.pst_type == PST_UOP &&
	    (*a)->pst_sym.pst_value.pst_s_op.pst_opno == ADI_MINUS_OP)
	a = &(*a)->pst_left;
    while (*b &&
	    (*b)->pst_sym.pst_type == PST_UOP &&
	    (*b)->pst_sym.pst_value.pst_s_op.pst_opno == ADI_MINUS_OP)
	b = &(*b)->pst_left;

    if (!*a && !*b)
    {
	ctx->same = TRUE;
	return 0;
    }
    if (!*a)
	return 1;
    if (!*b)
	return -1;
    if ((*a)->pst_sym.pst_type != (*b)->pst_sym.pst_type)
    {
	/* Arrange that CONSTs and QLENDs drift right */
	if ((*a)->pst_sym.pst_type == PST_CONST ||
	    (*a)->pst_sym.pst_type == PST_QLEND)
	    return 1;
	if ((*b)->pst_sym.pst_type == PST_CONST ||
	    (*b)->pst_sym.pst_type == PST_QLEND)
	    return -1;
    }
    asize = pst_qtree_size(&ctx->stk, *a);
    bsize = pst_qtree_size(&ctx->stk, *b);
    if (asize < bsize)
	return -1;
    if (asize > bsize)
	return 1;
    return pst_qtree_compare(ctx, a, b, FALSE);
}

/*{
** Name: pst_non_const_core - Return 'interesting bit'
**
** Description:
**	This routine descends the parse tree until it find a non-constant
**	item. In group by processing, 'outer' constants are irrelevant to
**	the grouping. Thus:
**	    GROUP BY a+b+100*:c
**	is the same as:
**	    GROUP BY a+b
**	Equally a target list expression that has just constant embellishments
**	such as:
**	    SELECT 'xxx'||VARCHAR(a || b)
**	will have the same grouping requierments as:
**	    GROUP BY a||b
**
**	NOTE: This cannot only be done with one-one mappings thus the following
**	are NOT equivalent group bys!
**	    ABS(a)	    and		a	(condenses range)
**	    CHAR(a)	    and		a	(normalisation not o->o)
**	    
**
** Inputs:
**	node		The parse tree segment
**
** Outputs:
**	none
**
**	Returns:
**	    node	non-const 'interesting' portion.
**
**	Exceptions:
**	    None
**
** Side Effects:
**	None - stack left intact.
**
** History:
**	
**	27-Nov-2009 (kiria01) SIR 121883
**	    Created for scalar subqueries.
*/

PST_QNODE *
pst_non_const_core(
    PST_QNODE *node)
{
    while (node)
    {
	PST_SYMVALUE *s = &node->pst_sym.pst_value;
	if (node->pst_sym.pst_type == PST_BOP)
	{
	    if (!s->pst_s_op.pst_fdesc ||
		    s->pst_s_op.pst_fdesc->adi_fiflags & (
			ADI_F16384_CHKVARFI|ADI_F1_VARFI) ||
			(s->pst_s_op.pst_opno != ADI_MUL_OP &&
			s->pst_s_op.pst_opno != ADI_DIV_OP &&
			s->pst_s_op.pst_opno != ADI_ADD_OP &&
			s->pst_s_op.pst_opno != ADI_SUB_OP &&
			s->pst_s_op.pst_opno != ADI_CNCAT_OP))
	    {
		break;
	    }
	    if (node->pst_left->pst_sym.pst_type == PST_CONST)
		node = node->pst_right;
	    else if (node->pst_right->pst_sym.pst_type == PST_CONST)
		node = node->pst_left;
	    else
		break;
	}
	else if (node->pst_sym.pst_type == PST_UOP)
	{
	    if (!s->pst_s_op.pst_fdesc ||
		    s->pst_s_op.pst_fdesc->adi_fiflags & (
			ADI_F16384_CHKVARFI|ADI_F1_VARFI) ||
			(s->pst_s_op.pst_opno != ADI_LOG_OP &&
			s->pst_s_op.pst_opno != ADI_SQRT_OP &&
			s->pst_s_op.pst_opno != ADI_PLUS_OP &&
			s->pst_s_op.pst_opno != ADI_MINUS_OP &&
			s->pst_s_op.pst_opno != ADI_HEX_OP))
		break;
	    node = node->pst_left;
	}
	else
	    break;
    }
    return node;
}

/*
** Name: pst_qtree_norm - normalse 1 tree for compares.
**
** Description:
**	Does a flattened, depth first traversal of the parse tree looking
**
** Input:
**	nodep		Address of pointer to global tree to flatten
**	tlist		Address of head target list resdom.
**	yyvarsp		Address of PSS_YYVARS context
**	psq_cb		Query parse control block
**	stk		Shared stack context
**
** Output:
**	nodep		The tree may be updated.
**
** Side effects:
**	Will re-order '+'s, '*', ANDs and ORs to normalised form. In the
**      case of arithmetic expressions with associative operators, the tree
**	will be left weighted with constants folded.
**
** History:
**	02-Nov-2009 (kiria01) 
**	    Created to normalise an expression so that equivalents
**	    are more cheaply found.
**	15-Jun-2010 (kschendel) b123921
**	    Don't reference past the end of *node when copying to fake.
**	    Most parse tree nodes aren't as large as the full PST_QNODE.
*/

DB_STATUS
pst_qtree_norm(
	PST_STK		*stk,
	PST_QNODE	**nodep,
	PSQ_CB		*psq_cb)
{
    DB_STATUS		status = E_DB_OK;
    PST_STK_CMP		cmp_ctx;
    i4			opno;
    i4			dt, ldt, rdt;
    PST_TYPE		ty;
    PST_QNODE		*node;
    PST_QNODE		fake;
    bool		chg;

    PST_STK_CMP_INIT(cmp_ctx, stk->cb, NULL);

    while (nodep)
    {
	node = *nodep;

	/*** Top-down traverse ***/
	switch (ty = node->pst_sym.pst_type)
	{
	case PST_BOP:
	    opno = node->pst_sym.pst_value.pst_s_op.pst_opno;
	    dt = abs(node->pst_sym.pst_dataval.db_datatype);
	    ldt = abs(node->pst_left->pst_sym.pst_dataval.db_datatype);
	    rdt = abs(node->pst_right->pst_sym.pst_dataval.db_datatype);
	    if (opno == ADI_ADD_OP && (/* ensure not really a CONCAT */
			ldt == DB_INT_TYPE ||
			ldt == DB_FLT_TYPE ||
			rdt == DB_DEC_TYPE) && (
			rdt == DB_INT_TYPE ||
			rdt == DB_FLT_TYPE ||
			rdt == DB_DEC_TYPE) ||
		opno == ADI_MUL_OP)
	    {
		PST_QNODE *l, *c, **p = nodep;
		bool reresolve = FALSE;

		/* *Not* fake = **nodep since the node will be allocated
		** exact-size, and we mustn't copy past the end of *nodep.
		*/
		MEcopy((PTR) (*nodep),
			sizeof(PST_QNODE) - sizeof(PST_SYMVALUE) + sizeof(PST_OP_NODE),
			(PTR) &fake);
		if ((*p)->pst_sym.pst_value.pst_s_op.pst_fdesc)
		{
		    (*p)->pst_sym.pst_value.pst_s_op.pst_fdesc = NULL;
		    reresolve = TRUE;
		}
		do
		{
		    while ((l = (*p)->pst_left) &&
			l->pst_sym.pst_type == PST_BOP &&
			l->pst_sym.pst_value.pst_s_op.pst_opno == opno &&
			abs(l->pst_sym.pst_dataval.db_datatype) == dt)
		    {
			if (l->pst_sym.pst_value.pst_s_op.pst_fdesc)
			{
			    l->pst_sym.pst_value.pst_s_op.pst_fdesc = NULL;
			    reresolve = TRUE;
			}
			(*p)->pst_left = l->pst_right;
			l->pst_right = *p;
			*p = l;
		    }
		    p = &(*p)->pst_right;
		}
		while ((*p)->pst_sym.pst_type == PST_BOP &&
			(*p)->pst_sym.pst_value.pst_s_op.pst_opno == opno &&
			abs((*p)->pst_sym.pst_dataval.db_datatype) == dt);
		fake.pst_left = *p;
		fake.pst_right = NULL;
		*p = &fake;
		/* At this point the common logical operators are a RHS list
		** and (*p) points to the fake tail which we shall detach
		** eventually. The constants will have moved to the end and
		** the list which will look like:
		**     +		This arrangement is the wrong way
		**    / \		round to what we really want which is
		**   e1  +		a LHS order but shortly we will rotate
		**	/ \		the tree as we resolve the operators.
		**     e2  +
		**	  / \		With constants drifted to the right
		**	 c1  F		we can cause these to be folded and
		**	    /		once the tree is rotated the constants
		**	   c2		will be in the right place for being
		**			ignored as non-core constants.
		*/
		l = *nodep;
		while (c = l->pst_right)
		{
		    do
		    {
			if (pst_qtree_compare_norm(&cmp_ctx,
					&l->pst_left,
					&c->pst_left) > 0)
			{
			    PST_QNODE *t = c->pst_left;
			    c->pst_left = l->pst_left;
			    l->pst_left = t;
			}
		    } while (c = c->pst_right);
		    l = l->pst_right;
		}
		/* Find the first of any constants */
		p = nodep;
		c = NULL;
		while ((*p)->pst_right && (c = (*p)->pst_left))
		{
		    if (c->pst_sym.pst_type == PST_CONST &&
			c->pst_sym.pst_value.pst_s_cnst.pst_tparmtype == PST_USER &&
			!c->pst_sym.pst_value.pst_s_cnst.pst_parm_no)
		    {
			/* Found first of at least 2 constants (as (*p)->pst_right != 0)
			** Arrange that they be folded. */
			while ((*p)->pst_right) /* *p != &fake */
			{
			    PST_QNODE **q = p;
			    while ((*q)->pst_right != &fake)
				q = &(*q)->pst_right;
			    /* q refers now to the training pair of constants.
			    ** Fiddle tail for folding to this and call resolver
			    **	  +		to do the folding. As we are
			    **	 / \		only dealing with true constants
			    **	e1  +		we can resolve the types now
			    **	   / \		regardless of outer parse state.
			    **	  e2  F
			    **	     /
			    **	    +
			    **	   / \
			    **	  c1  c2
			    */
			    c = *q;
			    *q = &fake;
			    c->pst_right = fake.pst_left;
			    fake.pst_left = c;
			    if (status = pst_node(stk->cb, &stk->cb->pss_ostream,
						c->pst_left, c->pst_right,
						c->pst_sym.pst_type, NULL, 0, DB_NODT, 0, 0, 
						NULL, &fake.pst_left, &psq_cb->psq_error, PSS_TYPERES_ONLY))
				return status;
			}
			break;
		    }
		    p = &(*p)->pst_right;
		}
		/* drop fake */
		*p = fake.pst_left;
		if (p != nodep)
		{
		    for(;;)
		    {  
			node = *nodep;
			if (node->pst_right == fake.pst_left)
			    break;
			*nodep = (*nodep)->pst_right;
			node->pst_right = (*nodep)->pst_left;
			if (reresolve &&
			    (status = pst_node(stk->cb, &stk->cb->pss_ostream,
					    node->pst_left, node->pst_right,
					    node->pst_sym.pst_type, NULL, 0, DB_NODT, 0, 0, 
					    NULL, &node, &psq_cb->psq_error, PSS_TYPERES_ONLY)))
			    return status;
			(*nodep)->pst_left = node;
		    }
		}
		if (reresolve &&
		    (status = pst_node(stk->cb, &stk->cb->pss_ostream,
			(*nodep)->pst_left, (*nodep)->pst_right,
			node->pst_sym.pst_type, (PTR)NULL, (i4)0, DB_NODT, (i2)0, (i4)0, 
			(DB_ANYTYPE *)NULL, nodep, &psq_cb->psq_error,
			PSS_TYPERES_ONLY)))
		    return status;

		do {
		    pst_push_item(stk, (PTR)&(*nodep)->pst_right);
		    p = nodep;
		    nodep = &(*nodep)->pst_left;
		} while ((l = *nodep) &&
		    l->pst_sym.pst_type == PST_BOP &&
		    l->pst_sym.pst_value.pst_s_op.pst_opno == opno);
		nodep = &(*p)->pst_left;
		continue;
	    }
	    break;
	case PST_AND:
	case PST_OR:
	    {
		PST_QNODE *l, *c, **p = nodep;

		/* *Not* fake = **nodep since the node will be allocated
		** exact-size, and we mustn't copy past the end of *nodep.
		*/
		MEcopy((PTR) (*nodep),
			sizeof(PST_QNODE) - sizeof(PST_SYMVALUE) + sizeof(PST_OP_NODE),
			(PTR) &fake);
		do
		{
		    while ((l = (*p)->pst_left) &&
			l->pst_sym.pst_type == ty)
		    {
			(*p)->pst_left = l->pst_right;
			l->pst_right = *p;
			*p = l;
		    }
		    p = &(*p)->pst_right;
		} while ((*p)->pst_sym.pst_type == ty);
		
		fake.pst_left = *p;
		fake.pst_right = NULL;
		*p = &fake;
		/* At this point the common logical operators are a RHS list
		** and (*p) points to the odd tail which we shall detach
		** and use as the comparitor down the list which will look
		** like:		  A
		**			 / \
		**		        e1  A
		**		           / \
		**			  e2  A
		**			     / \
		**			    e3  F
		**			       /
		**			      e4
		*/
		l = *nodep;
		while (c = l->pst_right)
		{
		    do
		    {
			if (pst_qtree_compare_norm(&cmp_ctx,
					&l->pst_left,
					&c->pst_left) > 0)
			{
			    PST_QNODE *t = c->pst_left;
			    c->pst_left = l->pst_left;
			    l->pst_left = t;
			}
		    } while (c = c->pst_right);
		    l = l->pst_right;
		}
		*p = NULL;
		while (*nodep)
		{
		    pst_push_item(stk, (PTR)&(*nodep)->pst_left);
		    nodep = &(*nodep)->pst_right;
		}
		*nodep = fake.pst_left;
		continue;
	    }
	    break;
	}
	if (node->pst_left)
	{
	    if (node->pst_right)
		pst_push_item(stk, (PTR)&node->pst_right);
	    nodep = &node->pst_left;
	}
	else if (node->pst_right)
	    nodep = &node->pst_right;
	else
	    nodep = (PST_QNODE**)pst_pop_item(stk);
    }

    return status;
}

/*
** Name: pst_qtree_size - size the tree
**
** Description:
**	Does a flattened parse tree count ignoring minuses.
**
** Input:
**	stk		Shared stack context
**	node		Address of tree to size
**
** Output:
**	none
**
** Side effects:
**	None
**
** History:
**	02-Nov-2009 (kiria01) 
**	    Created to quickly determine tree size
*/

u_i4
pst_qtree_size(
	PST_STK		*stk,
	PST_QNODE	*node)
{
    i4			tot = 0;

    while (node)
    {
	if (node->pst_sym.pst_type != PST_UOP ||
		node->pst_sym.pst_value.pst_s_op.pst_opno != ADI_MINUS_OP)
	    tot++;
	if (node->pst_left)
	{
	    if (node->pst_right)
		pst_push_item(stk, (PTR)node->pst_right);
	    node = node->pst_left;
	}
	else if (!(node = node->pst_right))
	    node = (PST_QNODE*)pst_pop_item(stk);
    }

    return tot;
}

