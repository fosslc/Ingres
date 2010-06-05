/*
**Copyright (c) 2007 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <bt.h>
#include    <me.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>

/**
**
**  Name: PSYTRSCAN.C - Utilities for scanning query trees
**
**  Description:
**      This file contains qrymod utilities for scanning query trees and doing
**	replacements.
**
**          psy_vfind	- Find definition for an attribute in a view tree
**	    psy_qscan	- Find a specified VAR node in a subtree
**	    psy_varset	- Scan tree and set a bit vector of variables
**	    psy_subsvars - Scan query tree and replace VAR nodes
**	    psy_vcount - Count var nodes in tree.
**
**
**  History:
**      19-jun-86 (jeff)    
**          Adapted from version 4.0 trscan.c
**	03-dec-87 (stec)
**	    Change psy_apql interface.
**	08-feb-88 (stec)
**	    Modify psy_subsvars to generate CURVAL nodes for replace cursor statement.
**	04-nov-88 (stec)
**	    Fix a view substitution bug.
**	14-dec-88 (stec)
**	    Fix correlated subqueries bug.
**	17-apr-89 (jrb)
**	    Interface change for pst_node and prec cleanup for decimal project.
**	22-may-89 (neil)
**	    Fix psy_vfind bug where ref's through null node.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	08-dec-92 (anitap)
**	    Changed the order of #includes of header files for CREATE SCHEMA.
**	15-dec-92 (wolf) 
**	    ulf.h must be included before qsf.h
**	27-jan-93 (rblumer) 
**	    replace psy_mkempty call with psl_make_default_node, to handle
**	    user-defined defaults.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to pst_treedup().
[@history_template@]...
**/

/*
** PSY_STK - stack structure to support flattening of heavily recursive
** functions. Function psy_push and psy_pop allow for the retention of the
** backtracking points. Function psy_pop_all allows for any cleanup of the
** stack should the traversal be terminated early.
** The stack is initially located on the runtime stack but extensions
** will be placed on the heap - hence the potential for cleanup needed.
** The size of 50 is unlikely to be tripped by any but the most complex
** queries and in tests optimiser memory ran out first.
*/
typedef struct _PSY_STK
{
    struct _PSY_STK *head;	/* Stack block link. */
    i4 sp;			/* 'sp' index into list[] at next free slot */
#define N_STK 50
    PTR list[N_STK];		/* Stack list */
} PSY_STK;

static VOID psy_push(PSY_STK *base, PTR val, STATUS *sts);
static PTR  psy_pop(PSY_STK *base);
static VOID psy_pop_all(PSY_STK *base);


/*{
** Name: psy_vfind	- Find definition for attribute in view tree
**
** Description:
**      This function takes a view tree and scans it for the specified RESDOM.
**	A pointer to the RESDOM node is set.
**
** Inputs:
**      vn                              The attribute number to look for
**      vtree                           The view tree to scan
**      result                          Place to put pointer to RESDOM
**
** Outputs:
**      result                          Filled with pointer to RESDOM, NULL if
**					not found.
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Failure (Not found is not considered a
**					    failure)
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	19-jun-86 (jeff)
**          Adapted from version 4.0 vfind()
**	22-may-89 (neil)
**	    Fix bug where ref's through null node.  This routine may be used for
**	    target lists w/o TREE terminating nodes. A consistency check was
**	    removed that referenced through the final value of 'v' which could
**	    have been null.  This bug caused an AV when updating a cursor which
**	    had an integrity constraint on a column that was not updated.
*/
DB_STATUS
psy_vfind(
	u_i2	    vn,
	PST_QNODE   *vtree,
	PST_QNODE   **result,
	DB_ERROR    *err_blk)
{
    i4		n;
    PST_QNODE	*v;

    n = vn;

    /* Support non-PST_TREE terminated target lists */
    for (v = vtree; v != NULL && v->pst_sym.pst_type == PST_RESDOM;
		v = v->pst_left)
    {
	if (v->pst_sym.pst_value.pst_s_rsdm.pst_rsno == n)
	{
	    /* found the correct replacement */
	    *result = v->pst_right;
	    return (E_DB_OK);
	}
    }

    *result = (PST_QNODE *) NULL;
    return (E_DB_OK);
}

/*{
** Name: psy_qscan	- Find specified VAR node in subtree
**
** Description:
**      Intended for finding a variable in a qualification, this routine just
**	scans a tree recursively looking for a VAR node with the specified
**	attribute number.
**
** Inputs:
**      root                            Pointer to root of subtree to scan
**	vn				Variable number to look for
**	an				Attribute number to look for
**
** Outputs:
**      None
**	Returns:
**	    Pointer to VAR node, NULL if not found
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	19-jun-86 (jeff)
**          written
**      11-May-2007 (kiria01) b111992
**          Flattened out stack recursion.
*/

PST_QNODE *
psy_qscan(
	PST_QNODE   *tree,
	i4	    vn,
	u_i2	    an)
{
    PSY_STK stk = {0, 0, 0};
    STATUS sts;
    while (tree)
    {
	/* check to see if this node qualifies */
	if (tree->pst_sym.pst_type == PST_VAR &&
		tree->pst_sym.pst_value.pst_s_var.pst_vno == vn &&
		tree->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id == an)
	{
	    /* We are truncating the traverse so ensure recursion stack clean */
	    psy_pop_all(&stk);

	    break; /* with tree set ready for return */
	}

	if (tree->pst_right)
	    /* Save right tree for later traversal */
	    psy_push(&stk, (PTR)tree->pst_right, &sts);
	
	/* Switch to left node if any, otherwise grab deffered */
	if (!(tree = tree->pst_left))
            tree = (PST_QNODE*)psy_pop(&stk);
    }
    return tree;
}

/*{
** Name: psy_varset	- Scan tree and set a bit vector of variables
**
** Description:
**      The return value is a bit vector representing the set of variables
**	used in the given subtree.
**
** Inputs:
**      root                            Pointer to root of subtree to scan
**	bitmap		bitmap address (bitmap does not have to be initialized).
**
** Outputs:
**	bitmap		bit map of variables used.
**
**	Returns:
**	    None.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	19-jun-86 (jeff)
**          written
**	28-may-87 (daved)
**	    use the from lists
**	11-jan-88 (stec)
**	    change i/f.
**      11-May-2007 (kiria01) b111992
**          Flattened out stack recursion.
*/

VOID
psy_varset(
	PST_QNODE	*tree,
	PST_VRMAP	*bitmap)
{
    PSY_STK stk = {0, 0, 0};
    STATUS sts;
    (VOID)MEfill(sizeof(PST_VRMAP), (u_char) 0, (PTR) bitmap);

    while (tree)
    {
	/* check out this node */
	switch (tree->pst_sym.pst_type)
	{
	case PST_ROOT:
	    /* check union nodes */
	    if (tree->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
	    {
		/* Save union tree for later traversal */
                psy_push(&stk,
                    (PTR)tree->pst_sym.pst_value.pst_s_root.pst_union.pst_next,
		    &sts);
	    }
	    /*FALLTHROUGH*/
	case PST_SUBSEL:
	case PST_AGHEAD:
	    /* add this from list */
	    BTor((i4) BITS_IN(PST_VRMAP),
		(char *)&tree->pst_sym.pst_value.pst_s_root.pst_tvrm, 
		(char *)bitmap);
            break;

	}
	if (tree->pst_right)
	    /* Save right tree for later traversal */
	    psy_push(&stk, (PTR)tree->pst_right, &sts);
	
	if (!(tree = tree->pst_left))
	   tree = (PST_QNODE*)psy_pop(&stk);
    }
}

/*{
** Name: psy_subsvars	- Scan query tree and replace VAR nodes
**
** Description:
**	Scans a tree and finds all VAR nodes for this variable.
**	These nodes are looked up in the translation tree and
**	replaced by the value found there.  If this is for a
**	view, the corresponding node must exist in the translation
**	tree.  If not for a view, a 'zero' node (of a type appropriate based
**	on the context) is created and inserted, or, if for a "replace
**	cursor" command, a CURVAL node is substituted for the VAR
**	node.
**
**	This routine is one half of the guts of the whole view
**	algorithm.
**
**	VAR nodes are detached and replaced with the replacement
**	as defined by the view.  Note that there can never be any
**	problems here with update anomalies, since VAR nodes are
**	only used in retrieve contexts.
**
**	It does some extra processing with RESDOM nodes with
**	resno = 0.  These nodes specify a 'tid' domain, and are
**	included by the parser on REPLACE and DELETE commands
**	Subsvars will allow this construct iff the right hand pointer is a
**	VAR node with attno = 0.  In pre-Jupiter versions, this used
**	to update the variable number in these tid RESDOMs; in Jupiter,
**	however, it was changed to keep the same result variable number
**	throughout the qrymod process, so this should be unnecessary.
**	This is because the resvar is the variable number of the one and
**	only underlying base relation of the view on an update
**	(which is presumably the only case where this can come
**	up).  Psy_vrscan has already insured that there can only be
**	a single base relation in this case.
**
**	This whole messy thing is only done with view substitutions.
**	NOTE: THIS IS NOT TRUE!  IT IS ALSO DONE FOR INTEGRITY SUBSTITUTIONS!
**	I DON'T KNOW WHY THIS COMMENT IS HERE.
**
**	In order to fix the handling of aggregates over views, subsvars
**	calls psy_apql at the appropriate place to append a
**	view qualification (if any).  It is done here to handle nested
**	aggregates correctly since after psy_subsvars we no longer know
**	which nested aggregates actually contained the view variable.
**	The view qual will be appended if and only if the view var
**	appears explicitly within the immediate scope of a root node
**	(NOT in a nested aggregate.)
**
**	If at any scope we encounter a var node, we add the qualification
**	to that scope. Once a var node has been found in a scope (and
**	the qualifaction added), for example, a nested aggregate, the
**	qualification is not added to an outer scope unless a var node
**	in the view or integ has been found in that outer scope.
**
**  
** Inputs:
**	proot				Pointer to pointer to root of tree
**					to be updated
**	rngvar				view variable range table entry
**	transtree			Pointer to the target list of the
**					translation tree
**	vmode				PSQ_VIEW if called from view processor,
**					PSQ_APPEND is called from the integrity
**					processor with an APPEND command, else
**					something else.  Mostly, changes
**					handling of tid nodes, and forces an
**					error on a view if the VAR node in the
**					scanned tree does not exist in the
**					vtree.
**	vqual				View qualification to be appended,
**					if any.
**	resvar				Range table entry for result variable
**					in query being modified.
**	from_list			from_list from view getting added.
**	qmode				Query mode of user query.
**	cursid				Cursor id of current cursor, if any
**	result				Pointer to indicator for result.
**	dup_rb				Ptr to dup. request block
**	    pss_op_mask	    --		0
**	    pss_num_joins   --		PST_NOJOIN
**	    pss_tree_info   --		NULL
**	    pss_mstream	    --		ptr to memory stream to be used
**	    pss_err_blk	    --		ptr to error block
**
** Outputs:
**      proot                           User query tree can be updated
**      result                          Filled in with TRUE if view variable
**					was found, FALSE if not. Valid only
**					if E_DB_OK returned.
**	dup_rb
**	    pss_err_blk			Filled in if an error happens.
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can allocate memory
**
** History:
**	19-jun-86 (jeff)
**          written
**	1-oct-86 (daved)
**	    set return to TRUE if a VAR node is found.
**	23-dec-86 (daved)
**	    copy qualification before appending it. this avoids graphs in
**	    the tree as well as, and more importantly, the re-use of the
**	    memory used by the qualification before its time. That is,
**	    if vqual is in temporary memory and gets deleted but the
**	    proot tree thinks the memory is still around, bad things happen.
**	12-aug-87 (stec)
**	    Removed test for special 'tid' attribute case, which, according
**	    to Jeff is no longer needed.
**	    Check for special 'tid' resdom now includes open cursor stmt.
**	15-oct-87 (stec)
**	    Added the removed test for special 'tid' attribute case;
**	    it's necessary for checking cases like "retrieve (viewname.tid)".
**	03-dec-87 (stec)
**	    Change psy_apql interface.
**	31-dec-87 (stec)
**	    Cleanup.
**	08-feb-88 (stec)
**	    Modify psy_subsvars to generate CURVAL nodes for replace cursor statement.
**	04-nov-88 (stec)
**	    Fix a view substitution bug. When visiting an AGHEAD node it may happen
**	    that count(*), or count(const) were defined, in which case there are
**	    no VAR nodes and the applicability of the view has to be determined from
**	    the relation bitmap in the node. This anomaly exists only in SQL.
**	14-dec-88 (stec)
**	    Fix correlated subqueries bug.
**	05-apr-89 (andre)
**	    simplify the test for when reference to the view in pst_tvrm is to
**	    be replaced with the tables used to define the view.  We no longer
**	    care if there were any variables found below the root node, instead,
**	    we do it for every root node which has a bit corresponding to the
**	    view set in pst_tvrm.
**	    As a part of the fix, qualification of the view will be appended to
**	    the tree whenever the view is found in the pst_tvrm of the root
**	    node.
**	    Besides allowing us to get rid of calling recursive psy_fixmap() in
**	    psy_view, but it also fixes bugs such as:
**	    "select const from view"  returning more rows than there are in the
**	    view.
**	04-may-89 (andre)
**	    for the time being, set pst_maks1 in all ROOT-type nodes to 0.
**	01-jun-89 (andre)
**	    The preceding fix was not perfect.
**	    "create view v as select * from t where <qual>;
**	     select <aggregate> from v\g"
**	     would result in as many rows containing result of applying
**	     <aggregate> as there are rows in v.  This happens only in SQL.  The
**	     problem is that <qual> gets appended to both AGGHEAD node and the
**	     ROOT node.  The solution is as follows:
**	         For every node N s.t. N is of type ROOT or SUBSELECT, remember
**		 if <qual> has been applied to an AGGHEAD node in the left
**		 subtree of N (in SQL you can not have AGGHEADs in the
**		 "where-clause").  If <qual> has been applied to AGGHEAD(s) in
**		 the left subtree of N, do not append it to the right subtree of
**		 N.
**	22-jun-89 (andre)
**	    And yet another fix for the previous bug fix.  I have incorrectly
**	    assumed that there may be no AGGHEADs in the right subtrre of
**	    ROOT/SUBSEL (select ... having agg(col)).  Before setting *mask to
**	    indicate that an AGGHEAD has been seen, make sure that we are in the
**	    left subtree of the immediate ROOT/SUBSEL parent
**	    (mask != (i4 *) NULL).  If mask is NULL, we must be in the right
**	    subtree, and the fact that we saw an AGGHEAD is of no importance
**	    (or shall I add "or so I believe"?)
**	13-sep-89 (andre)
**	    receive ptr to PSS_DUPRB which will point at memopry stream and
**	    error block + it will be used when calling pst_treedup().  The
**	    fields in dup_rb must be set as follows:
**	    pss_op_mask	    -- 0
**	    pss_num_joins   -- PST_NOJOIN
**	    pss_tree_info   -- NULL
**	    pss_mstream	    -- ptr to memory stream to be used
**	    pss_err_blk	    -- ptr to error block
**	14-sep-92 (andre)
**	    do not zero out pst_mask1 in PST_ROOT and PST_SUBSEL node
**	    (fix for bug 45238)
**	11-feb-93 (andre)
**	    if a query tree involved a reference to a TID attribute of a view V,
**	    replace it with a reference to TID attribute of V's underlying table
**	    or view; this is accomplished by replacing variable number found in
**	    the PST_VAR node with the variable number of the view's underlying
**	    table/view (which can be found by looking for the first set bit in
**	    from_list)
**	27-nov-02 (inkdo01)
**	    Range table expansion (i4 changed to PSAT_J_MASK).
**	13-Jun-2006 (kschendel)
**	    Barf if we translate a var node to a seqop default in an INSERT.
**	    This only happens if we're translating an integrity where-clause
**	    tree, and a var in that where-clause isn't mentioned in the
**	    values list, so we stick the default in instead.  Seqops aren't
**	    allowed in where clauses.  (It would imply that the insert
**	    integrity-permission depends on the sequence value, which is
**	    silly at best.)
**	15-May-2007 (kiria01) b111992
**	    Flatten out much of the recursion of this function to reduce
**	    runtime stack usage - especially bad with views containing
**	    massive IN clauses (>5K elements). 
**	28-nov-2007 (dougi)
**	    Add PSQ_REPDYN to PSQ_DEFCURS test for cached dynamic qs.
**	05-Nov-2009 (kiria01) b122841
**	    Use psl_mk_const_similar to cast default values directly.
**	12-Nov-2009 (kiria01) b122841
**	    Corrected psl_mk_const_similar parameters with explicit
**	    mstream.
**       5-Feb-2010 (hanal04) Bug 123209
**          psy_integ() calls psy_subsvars() to subsitute VARs in the
**          integrity tree with the corresponding nodes from the
**          user query. When a VAR is replaced with a CONST cast the
**          CONST to the VAR's datatype. This stops the substitution from
**          breaking ADE_COMPAREN & ADE_NCOMPAREN processing if the
**          VAR was part of an IN LIST. 
**	18-May-2010 (kiria01) b123442
**	    Force psl_mk_const_similar to generate coercion to cleanly 
**	    enable correct datatype to be represented when substituting
**	    default values.
*/
DB_STATUS
psy_subsvars(
	PSS_SESBLK	*cb,
	PST_QNODE	**proot,
	PSS_RNGTAB	*rngvar,
	PST_QNODE	*transtree,
	i4		vmode,
	PST_QNODE	*vqual,
	PSS_RNGTAB	*resvar,
	PST_J_MASK	*from_list,
	i4		qmode,
	DB_CURSOR_ID	*cursid,
	i4		*mask,
	PSS_DUPRB	*dup_rb)

{
    PSY_STK	stk = {0, 0, 0};/* Backtrack stack */
    PST_QNODE	*t;		/* Temporary for *proot */
    i4		vn = rngvar
		    ? rngvar->pss_rgno : -1; /* can be NULL on replace cursor statements */
    i4		err_code;
    DB_STATUS	status = E_DB_OK;

    while(proot && (t = *proot))
    {
	/* These 3 mask variables are only used for ROOT, SUBSEL and AGHEAD */
	i4 newmask;	/* For receiving result from recursive call */
	i4 *l_mask;	/* For propagating state to recursive caller */
	i4 *r_mask;	/*  .. */

	/*
	** The recursive nature of this function has been restructured to
	** allow for most of the processing to be achieved in an iterative
	** manner. Unlike with the other functions in this module, the
	** flattening could not be complete due to the 'mask' output parameter
	** which requires local storage for certain node types: ROOT, SUBSEL
	** and AGHEAD. If we have one of these node types we recurse 1 level
	** to process the left and right sub-trees with the correct scoping of
	** the 'mask' variable.
	*/
	switch (t->pst_sym.pst_type)
	{
	case PST_ROOT:
	    /* Handle any unions */
	    if (t->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
	    {
		/*
		** Defer the tree representing the next subselect in the UNION
		** status = psy_subsvars(cb, 
		**    &t->pst_sym.pst_value.pst_s_root.pst_union.pst_next, rngvar, transtree,
		**    vmode, vqual, resvar, from_list, qmode, cursid, (i4 *) NULL,
		**    dup_rb);
		*/
		psy_push(&stk, (PTR)&t->pst_sym.pst_value.pst_s_root.pst_union.pst_next,
		      &status);
		if (DB_FAILURE_MACRO(status))
		{
		    proot = NULL; /* Exiting to return error */
		    break;
		}
	    }
	    /*FALLTHROUGH*/
	case PST_SUBSEL:
	    /*
	    ** The following applies when language is SQL:
	    **	    if this is a ROOT or a SUBSELECT node, we want to know if the left
	    **	    subtree contains AGGHEAD nodes to which view qualification have been
	    **	    applied; we are not concerned with AGGHEAD nodes in the
	    **	    qualification (there shouldn't be any, anyway) or in other members
	    **	    of the union.
	    */

	    if (cb->pss_lang == DB_SQL)
	    {
		newmask = 0;
		l_mask = &newmask;

		/* we don't care about the right subtree */
		r_mask = (i4 *) NULL;	
	    }
	    /*FALLTHROUGH*/
	case PST_AGHEAD:
	    /*
	    ** The following applies when language is SQL:
	    **	    If this is an AGGHEAD node, set a bit in 'mask' to remember that
	    **	    we saw it.
	    */
	    if (t->pst_sym.pst_type == PST_AGHEAD)
	    {
		if (cb->pss_lang == DB_SQL)
		{
		    if (l_mask = r_mask = mask)
			/*
			** If we are in the right subtree of the immediate ROOT/SUBSELECT
			** parent, mask will be NULL, since we are not concerned with
			** AGHEADs in the right subtrees.
			*/
			*mask |= PSS_1SAW_AGG;
		}
		/*
		** pst_mask1 in PST_AGHEAD node is neither used nor set; I think it
		** would be a good idea to set it, but at this point there is not a heck
		** of a lot that we can do.  Here we will zero out PST_AGHEAD.pst_mask1
		** purely for esthetic reasons.
		*/
		t->pst_sym.pst_value.pst_s_root.pst_mask1 = 0;
	    }

	    /*
	    ** Recurse 1 level to process the left & right subtrees completly
	    ** so that we can complete the processing of this node
	    */
	    status = psy_subsvars(cb, &t->pst_left, rngvar, transtree, vmode, 
		vqual, resvar, from_list, qmode, cursid, l_mask, dup_rb);
	    if (DB_FAILURE_MACRO(status))
	    {
		proot = NULL; /* Exiting to return error */
		break;
	    }

	    /* Process the right branch */
	    status = psy_subsvars(cb, &t->pst_right, rngvar, transtree, vmode, 
		vqual, resvar, from_list, qmode, cursid, r_mask, dup_rb);
	    if (DB_FAILURE_MACRO(status))
	    {
		proot = NULL; /* Exiting to return error */
		break;
	    }

	    /* Add `from' list to bitmap, remove entry for replaced var */
	    if (BTtest(vn, (char*)&t->pst_sym.pst_value.pst_s_root.pst_tvrm))
	    {
		BTclear(vn, (char*)&t->pst_sym.pst_value.pst_s_root.pst_tvrm);
		BTor(PST_NUMVARS, (char *)from_list, 
			(char *)&t->pst_sym.pst_value.pst_s_root.pst_tvrm);
		t->pst_sym.pst_value.pst_s_root.pst_tvrc = 
		    BTcount((char*)&t->pst_sym.pst_value.pst_s_root.pst_tvrm,
			    BITS_IN(t->pst_sym.pst_value.pst_s_root.pst_tvrm));

		/*
		** We will append qualification (if there is one) if the
		** following holds:
		** 1) This is not an SQL (must be QUEL) query    OR
		** 2) if node is ROOT or SUBSEL (i.e. not an AGHEAD) then there
		**    were no AGHEADs found in its left subtree
		**
		**  Let QUAL <==> there is a qualification,
		**      SQL  <==> language is SQL
		**	ROOT <==> node type is ROOT
		**	SUBSEL <==> node type is SUBSEL
		**	AGG    <==> node type is AGHEAD
		**	SAW_AGG <==> mask & PSS_1SAW_AGG.  Then
		**	
		** (Do not apply qualification) <==>
		**  !QUAL + SQL * (ROOT + SUBSEL) * SAW_AGG -->
		**  (Apply qualification) <==>
		**  !(!QUAL + SQL * (ROOT + SUBSEL) * SAW_AGG) <==>
		**  QUAL * !(SQL * (ROOT + SUBSEL) * SAW_AGG)  <==>
		**  QUAL * (!SQL + !((ROOT + SUBSEL) * SAW_AGG)) <==>
		**  QUAL * (!SQL + !(ROOT + SUBSEL) + !SAW_AGG)  <==>
		**  QUAL * (!SQL + AGG + !SAW_AGG)
		*/
		if (vqual &&
		    (cb->pss_lang != DB_SQL	||
		    t->pst_sym.pst_type == PST_AGHEAD ||
		     (*l_mask & PSS_1SAW_AGG) == 0))
		{
		    PST_QNODE *vqual_copy;
		    dup_rb->pss_tree = vqual;
		    dup_rb->pss_dup  = &vqual_copy;
		    status = pst_treedup(cb, dup_rb);
		    dup_rb->pss_tree = (PST_QNODE *)NULL;
		    dup_rb->pss_dup  = (PST_QNODE **)NULL;
        		    
		    if (DB_FAILURE_MACRO(status))
		    {
			proot = NULL; /* Exiting to return error */
			break;
		    }
		    /* append view qualification */
		    status = psy_apql(cb, dup_rb->pss_mstream, vqual_copy, t,
				      dup_rb->pss_err_blk);
		    if (DB_FAILURE_MACRO(status))
		    {
			proot = NULL; /* Exiting to return error */
			break;
		    }
		}
	    }
	    /* left & right have been processed */
	    break;

	case PST_VAR:
	    /*
	    ** This is a terminal node - the expectation is that left & right are 0
	    */
            
	    /*
	    ** Check for a VAR node but of a different variable than the one
	    ** we are substituting for. REPLACE CURSOR (quel version) is an
	    ** exception because the substitution variable (resvar) is not
	    ** defined, in that case we do not want to execute the code
	    ** below, but want to continue the translation process.
	    */
	    if (vn != -1 && t->pst_sym.pst_value.pst_s_var.pst_vno != vn)
		break;
	    /*
	    ** if this is a reference to a TID attribute of a view (which is not
	    ** a "real" attribute), it needs to be translated into a reference
	    ** to the TID attribute of the view's underlying table or view
	    */
	    if (t->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id == 0 &&
		vmode == PSQ_VIEW)
	    {
		t->pst_sym.pst_value.pst_s_var.pst_vno =
		    BTnext(-1, (char *) from_list, sizeof(*from_list));
	    }
	    else
	    {
		PST_QNODE *v;

		/* find var in vtree */
		status = psy_vfind((u_i2)t->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id, 
		    transtree, &v, dup_rb->pss_err_blk);
		if (DB_FAILURE_MACRO(status))
		{
		    proot = NULL; /* Exiting to return error */
		    break;
		}

		if (v == (PST_QNODE *)NULL)
		{
		    /* attribute not defined in view */
		    if (vmode == PSQ_VIEW)
		    {
			psf_error(E_PS0D03_ATT_NOT_FOUND, 0L, PSF_INTERR, &err_code,
			    dup_rb->pss_err_blk, 0);
			status = E_DB_SEVERE;
			proot = NULL; /* Exiting to return error */
			break;
		    }
		    /* append defaults for integrity. Integrity might exist on a value
		    ** we are appending by default. I.e., the attribute was not mentioned
		    ** in the target list. We replace the var node in the integrity with
		    ** a default value so that the integrity will read 'default value' ?
		    ** value.
		    */
		    else if (vmode == PSQ_APPEND)
		    {
			status = psl_make_default_node(cb, dup_rb->pss_mstream, resvar,
						       t->pst_sym.pst_value
							     .pst_s_var.pst_atno.db_att_id,
						       &v, dup_rb->pss_err_blk);
			if (DB_FAILURE_MACRO(status))
			{
			    proot = NULL; /* Exiting to return error */
			    break;
			}
			/* Try to cast to column type */
			status = psl_mk_const_similar(cb, dup_rb->pss_mstream,
						&t->pst_sym.pst_dataval,
						&v, dup_rb->pss_err_blk, NULL);
			if (DB_FAILURE_MACRO(status))
			    return(status);

			/* If we ended up with a sequence default, fail. This is an
			** unreasonable situation, integrity where tests should not apply
			** to sequence defaults.
			*/
			if (v->pst_sym.pst_type == PST_SEQOP)
			{
			    psf_error(6319, 0, PSF_USERERR, &err_code,
				    dup_rb->pss_err_blk, 0);
			    status = E_DB_ERROR;
			    proot = NULL; /* Exiting to return error */
			    break;
			}
		    }
		    /* we would like to delete the qualification for this node since the
		    ** value is not changing and thus we don't need to worry about integrity
		    ** constaints on it. However, we don't do that. Instead we have the
		    ** integrity refer to the value in the current row that reflects the
		    ** value for the attribute. In the replace statement (not replace
		    ** cursor) we just perform the retrieve, the qualification is unneeded
		    ** but doesn't hurt anything. We want to avoid causing a retrieve for
		    ** each update cursor; therefore, we change the varnode to refer to the
		    ** current value (ie the retrieve has already been done).
		    */
		    else if (vmode == PSQ_REPCURS)
		    {
			PST_CRVAL_NODE curval;
			/* Create a CURVAL node for the corresponding column */
			curval.pst_curcol.db_att_id =
			    t->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
			STRUCT_ASSIGN_MACRO(*cursid, curval.pst_cursor);
			status = pst_node(cb, dup_rb->pss_mstream, (PST_QNODE *)NULL,
				(PST_QNODE *)NULL, PST_CURVAL, (PTR)&curval, sizeof(curval),
				t->pst_sym.pst_dataval.db_datatype,
				t->pst_sym.pst_dataval.db_prec,
				t->pst_sym.pst_dataval.db_length,
				(DB_ANYTYPE *)t->pst_sym.pst_dataval.db_data, &v,
				dup_rb->pss_err_blk, (i4) 0);
			if (DB_FAILURE_MACRO(status))
			{
			    proot = NULL; /* Exiting to return error */
			    break;
			}
		    }
		}
		else
		{
		    dup_rb->pss_tree = v;
		    dup_rb->pss_dup  = &v;
		    status = pst_treedup(cb, dup_rb);
		    dup_rb->pss_tree = (PST_QNODE *)NULL;
		    dup_rb->pss_dup  = (PST_QNODE **)NULL;
                	
		    if (DB_FAILURE_MACRO(status))
		    {
			proot = NULL; /* Exiting to return error */
			break;
		    }

                    /* When called from psy_integ() v will be found but
                    ** may still need constants to be cast.
                    */
                    if ((v != (PST_QNODE *)NULL) &&
                        (v->pst_sym.pst_type == PST_CONST))
                    {
			bool handled;

                        /* Try to cast to column type */
                        status = psl_mk_const_similar(cb, dup_rb->pss_mstream,
                                                &t->pst_sym.pst_dataval,
                                                &v, dup_rb->pss_err_blk, 
                                                &handled);
                        if (DB_FAILURE_MACRO(status))
                            return(status);
                    }
		}

		/* replace VAR node */
		if (v != (PST_QNODE *)NULL)
		{
		    *proot = v;
		}
	    }
	    /* left and right should have been null as we are on a terminal */
	    break;

	case PST_RESDOM:
	    /* Process `TID' resdom used by DELETE, REPLACE and OPEN CURSOR */
	    if (t->pst_sym.pst_value.pst_s_rsdm.pst_rsno == 0 &&
		(qmode == PSQ_DELETE || qmode == PSQ_REPLACE || 
			qmode == PSQ_DEFCURS || qmode == PSQ_REPDYN))
	    {
		/*
		** If resvar not specified, or if not resvar, ignore leaf.
		*/
		if (resvar && vn == resvar->pss_rgno)
		{
		    /* t->right better be VAR node, attno 0 */
		    t = t->pst_right;
		    if (t->pst_sym.pst_type != PST_VAR ||
			t->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id != 0 ||
			t->pst_sym.pst_value.pst_s_var.pst_vno != vn)
		    {			
			(VOID) psf_error(E_PS0D02_BAD_TID_NODE, 0L, PSF_INTERR, &err_code,
			    dup_rb->pss_err_blk, 0);
			status = E_DB_SEVERE;
			proot = NULL; /* Exiting to return error */
			break;
		    }
		}
		else if (t->pst_right)
		    /* Process the right branch */
		    psy_push(&stk, (PTR)&t->pst_right, &status);
	    }
	    else if (t->pst_right)
		/* Process the right branch */
		psy_push(&stk, (PTR)&t->pst_right, &status);

	    if (DB_FAILURE_MACRO(status))
	    {
		proot = NULL; /* Exiting to return error */
		break;
	    }
	    /* Process left branch */
	    if (t->pst_left)
		psy_push(&stk, (PTR)&t->pst_left, &status);
	    if (DB_FAILURE_MACRO(status))
	    {
		proot = NULL; /* Exiting to return error */
		break;
	    }
	    break;

	default:
	    /*
	    ** Just ensure that we traverse the tree
	    */
	    if (t->pst_right)
		psy_push(&stk, (PTR)&t->pst_right, &status);
	    if (DB_FAILURE_MACRO(status))
	    {
		proot = NULL; /* Exiting to return error */
		break;
	    }

	    if (t->pst_left)
		psy_push(&stk, (PTR)&t->pst_left, &status);
	    if (DB_FAILURE_MACRO(status))
	    {
		proot = NULL; /* Exiting to return error */
		break;
	    }
	    break;
	}
	if (!proot)    /* We're breaking out early to exit */
	    break;

	/*
	** Get next deferred node
	*/
	proot = (PST_QNODE**)psy_pop(&stk);
    }

    /* Release any outstanding entries */
    psy_pop_all(&stk);

    return status;
}

/*{
** Name: psy_vcount 	- Scan query tree and map variables
**
** Description:
**  
** Inputs:
**	tree		input query tree.
**	bitmap		bitmap address (bitmap does not have to be initialized).
**
** Outputs:
**	bitmap		bit map of variables used.
**
**	Returns:
**	    None.
**	Exceptions:
**	    None
**
** Side Effects:
**	None.
**
** History:
**	16-june-87 (daved)
**	    written
**	30-dec-87 (stec)
**	    Change interface.
**      11-May-2007 (kiria01) b111992
**          Flattened out stack recursion
*/

VOID
psy_vcount(
	PST_QNODE	*tree,
	PST_VRMAP	*bitmap)
{
    PSY_STK stk = {0, 0, 0};
    STATUS sts;

    (VOID)MEfill(sizeof(PST_VRMAP), (u_char)0, (PTR)bitmap);

    while (tree)
    {
	/* If this is a VAR node supplement the bitmap */
	switch (tree->pst_sym.pst_type)
	{
	case PST_VAR:
	    (VOID)BTset((i4)tree->pst_sym.pst_value.pst_s_var.pst_vno,
		(char *)bitmap);
            break;

        case PST_ROOT:
	    /* check union nodes */
	    if (tree->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
		psy_push(&stk,
			(PTR)tree->pst_sym.pst_value.pst_s_root.pst_union.pst_next,
			&sts);
	    break;
	}
	/* scan left and right branches if any */
	if (tree->pst_right)
	    psy_push(&stk, (PTR)tree->pst_right, &sts);
	if (!(tree = tree->pst_left))
	    tree = (PST_QNODE*)psy_pop(&stk);
    }
}


/*{
** Name: psy_push -	Push item on recursion stack
**
** Description:
**  
** Inputs:
**	base		Stack base block
**	val		Item to be stored
**
** Outputs:
**	sts		Status return
**
**	Returns:
**	    None.
**	Exceptions:
**	    None
**
** Side Effects:
**	May allocate memory.
**
** History:
**      11-May-2007 (kiria01) b111992
**          Created for flattened out stack recursion
*/

static VOID
psy_push(PSY_STK *base, PTR val, STATUS *sts)
{
    /* Point to true list head block */
    PSY_STK *stk = base->head ? base->head : base;

    if (stk->sp >= N_STK)
    {
        base->head = (PSY_STK*)MEreqmem(0, sizeof(PSY_STK), FALSE, sts);
        if (!base->head || DB_FAILURE_MACRO(*sts))
	   return;
        base->head->head = stk;
        stk = base->head;
        stk->sp = 0;
   }
   stk->list[stk->sp++] = val;
}


/*{
** Name: psy_pop 	- Retrieve top item off recursion stack
**
** Description:
**  
** Inputs:
**	base		Stack base block
**
** Outputs:
**
**	Returns:
**	    value from stack or NULL if end-of-stack.
**
**	Exceptions:
**	    None
**
** Side Effects:
**	May free memory from the stack.
**
** History:
**      11-May-2007 (kiria01) b111992
**          Created for flattened out stack recursion
*/

static PTR
psy_pop(PSY_STK *base)
{
    /* Point to true list head block */
    PSY_STK *stk = base->head ? base->head : base;
    
    if (!stk->sp)
    {
        if (base == stk)
        {
            /*stack underflow*/
            return NULL;
        }
        base->head = stk->head;

        MEfree((PTR)stk);
        stk = base->head;
   }
   return stk->list[--stk->sp];
}


/*{
** Name: psy_pop_all 	- Release resources from the recursion stack
**
** Description:
**  
** Inputs:
**	base		Stack base block
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
**      11-May-2007 (kiria01) b111992
**          Created for flattened out stack recursion
*/

static VOID
psy_pop_all(PSY_STK *base)
{
    PSY_STK *stk;
    if (base->head)
    {
	for (stk = base->head; base != stk; stk = base->head)
	{
	    base->head = stk->head;
	    MEfree((PTR)stk);
	}
    }
}
