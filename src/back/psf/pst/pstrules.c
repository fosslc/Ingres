/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <qu.h>
#include    <bt.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
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
** Name: pstrules.c - Functions for walking rule trees.
**
** Description:
**	This file contains routines for walking rule trees.  These are not
**	generic statement trees, and they are not recursive.  These
**	routines are very dependent on the way rule trees are built in
**	psyrules.c.
**
**          pst_ruledup - Duplicate a rule tree.
** History:
**	17-mar-89 (neil)
**	    Written.
**	18-apr-89 (neil)
**	    Added support to filter out subtrees and copy to ULM streams
**	    for cursors.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	30-jul-92 (rblumer)
**	    FIPS CONSTRAINTS:  changed uses of psr_column to psr_columns; 
**                 it is now a bitmap instead of a nat.
**	    
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp(),
**	    psl_rptqry_tblids(), psl_cons_text(), psl_gen_alter_text(),
**	    psq_tmulti_out(), psq_1rptqry_out(), psq_tout().
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 pst_ruledup(
	PSS_SESBLK *sess_cb,
	PSF_MSTREAM *mstream,
	i4 filter,
	char *col_mask,
	PST_STATEMENT *stmt_tree,
	PST_STATEMENT **stmt_dup,
	DB_ERROR *err_blk);

/*{
** Name: pst_ruledup - Duplicate a rule tree
**
** Description:
**      This routine duplicates a rule tree.  The tree is traversed in
**	the order of its statements and a copy of each node is made.
**	Currently a rule tree may contain any number of the following nodes:
**		PST_IF_TYPE
**		PST_CP_TYPE
**	With the following relationships:
**		1. An IF statement must be immediately followed by a CP
**		   statement, where the TRUE branch points at the CP statement,
**		   the "pst_link" branch points at the CP statement, and
**		   the FALSE and "pst_next" branches point at the "next"
**		   statement.
**		2. The last statement must point at null.
**	Using the above relationships there is no need to make this recursive
**	(which we try to avoid in the limited stack space we have).
**	The tree may be duplicated into QSF memory.
**
**	Tree duplication may also including the filtering out of unused rules.
**	When filtering out rules the rules are traversed in the normal way,
**	but each rule's PSC_RULE_TYPE tag is checked.  If that rule applies
**	then it's attached, otherwise it's ignored.  This functionality is
**	useful for cursors where an initial statement collected all the rules
**	and now only a particular set of rules apply.
**
** Inputs:
**	sess_cb			Session control block.
**      mstream                 PSF/QSF memory stream.
**	filter			Rule statement type used to filter out unused
**				rules from an already stored list of rules in
**				stmt_tree.  If this is 0 then no filtering is
**				done, otherwise a rule_type node is expected to
**				be present.
**	col_mask		Bit mask of current UPDATE columns.  This is
**				only checked if the rule being applied (from
**				the list) has a non-0 column number in its
**				saved rule type (originated from an UPDATE).
**      stmt_tree		Pointer to statement tree to be duplicated.
**				This may be NULL if no rule tree.
** Outputs:
**	dup_tree		Place to put duplicate statement tree.
**	err_blk.err_code	Filled in if an error happens:
**		E_PS0C06_BAD_STMT	- Corrupt/missing statement nodes.
**		Allocation errors from pst_treedup and psf_malloc.
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory
**
** History:
**	17-mar-89 (neil)
**          Written to execute prepared Dynamic SQL DML statements which
**	    already have rules applied to them.
**	18-apr-89 (neil)
**	    Modified to support cursors and rule list filtering.
**	30-jul-92 (rblumer)
**	    FIPS CONSTRAINTS:  changed uses of psr_column to psr_columns; 
**                 it is now a bitmap instead of a nat.
**	08-apr-93 (andre)
**	    removed ustream from the interface; this function will no longer be
**	    called from psy_rules() to copy all rules from QSF into ULM memory
**	    
*/

DB_STATUS
pst_ruledup(
	PSS_SESBLK  	*sess_cb,
	PSF_MSTREAM     *mstream,
	i4		filter,
	char		*col_mask,
	PST_STATEMENT	*stmt_tree,
	PST_STATEMENT	**stmt_dup,
	DB_ERROR	*err_blk)
{
    PST_STATEMENT	*curs;		/* Current stmt to walk through list */
    PST_STATEMENT	*ifs;		/* IF statement */
    PST_STATEMENT	*cps;		/* CP statement */
    PST_STATEMENT	*prev_if;	/* Previous IF and CP statements to  */
    PST_STATEMENT	*prev_cp;	/*     patch dangling pointers       */
    PST_STATEMENT	*cur_rule;	/* PTR to CP node or IF-CP node pair */
    PST_STATEMENT	*result;	/* Return value - start of list */
    PSC_RULE_TYPE	*rtype;		/* Filter rule type */
    DB_COLUMN_BITMAP	tst_mask;       /* test mask for update columns */
    DB_STATUS           status;
    PSS_DUPRB		dup_rb;
    i4		err_code;

    *stmt_dup = result = NULL;
    if ((curs = stmt_tree) == NULL)	/* Nothing doing */
	return (E_DB_OK);

    prev_if = NULL;
    prev_cp = NULL;

    /* initialize fields in dup_rb */
    dup_rb.pss_op_mask = 0;
    dup_rb.pss_num_joins = PST_NOJOIN;
    dup_rb.pss_tree_info = (i4 *) NULL;
    dup_rb.pss_mstream = mstream;
    dup_rb.pss_err_blk = err_blk;
    
    while (curs != NULL)
    {
	/* 
	** Filter out current rule if not applicable.  Check statement type of
	** caller and rule, and column mask.
	*/
	if (filter != 0)
	{
	    if ((rtype = (PSC_RULE_TYPE *)curs->pst_opf) == NULL)
	    {
		psf_error(E_PS0C06_BAD_STMT, 0L, PSF_INTERR, 
			  &err_code, err_blk, 1,
			  sizeof("PSC_RULE_TYPE.pst_opf")-1,
			  "PSC_RULE_TYPE.pst_opf");
		return (E_DB_ERROR);
	    }

	    /* copy the bitmap of the rule to an overwriteable test mask,
	    ** and test that against the bitmap of columns being updated
	    */
	    MEcopy((PTR) &rtype->psr_columns,
		   sizeof(DB_COLUMN_BITMAP), (PTR) &tst_mask);
	    BTand((i4) DB_COL_BITS, 
		  (char *) col_mask, (char *) tst_mask.db_domset);

	    /* Check statement type of caller and rule,
	    ** and check result of column bitmap test. 
	    */
	    if (   ((rtype->psr_statement & filter) == 0)
		|| (BTcount((char *) tst_mask.db_domset, DB_COL_BITS) == 0)
	       )
	    {
		curs = curs->pst_next;		/* Skip this one */
		continue;
	    }
	}
	ifs = cps = NULL;		/* Current ones start out nulled */

	/* If found IF, allocate node, patch pointers & continue with CP node */
	if (curs->pst_type == PST_IF_TYPE)
	{
	    status = psf_malloc(sess_cb, mstream, sizeof(*ifs), (PTR*)&ifs, err_blk);

	    if (status != E_DB_OK)
		return (status);
		
	    STRUCT_ASSIGN_MACRO(*curs, *ifs);

	    if (filter != 0)		/* Detach for safety when filtering */
		ifs->pst_opf = NULL;

	    /* Patch/copy IF condition pointer and confirm it exists*/
	    dup_rb.pss_tree = curs->pst_specific.pst_if.pst_condition;
	    dup_rb.pss_dup  = &ifs->pst_specific.pst_if.pst_condition;
	    status = pst_treedup(sess_cb, &dup_rb);

	    if (status != E_DB_OK)
		return (status);
		
	    if (ifs->pst_specific.pst_if.pst_condition == NULL)
	    {
		psf_error(E_PS0C06_BAD_STMT, 0L, PSF_INTERR, 
			  &err_code, err_blk, 1,
			  sizeof("PST_IF.pst_condition") - 1,
			  "PST_IF.pst_condition");
		return (E_DB_ERROR);
	    }
	    /* True and False links done later */

	    curs = ifs->pst_link;	/* Better be a CP statement node */
	} /* If found an IF node */

	if (curs->pst_type != PST_CP_TYPE)
	{
	    psf_error(E_PS0C06_BAD_STMT, 0L, PSF_INTERR, 
		      &err_code, err_blk, 1,
		      sizeof("PST_CP") - 1, "PST_CP");
	    return (E_DB_ERROR);
	}

	/* Allocate and initialize CP node */
	status = psf_malloc(sess_cb, mstream, sizeof(*cps), (PTR *)&cps, err_blk);

	if (status != E_DB_OK)
	    return (status);
	    
	STRUCT_ASSIGN_MACRO(*curs, *cps);

	if (filter != 0)		/* Detach for safety when filtering */
	    cps->pst_opf = NULL;

	/* If there are any arguments patch/copy arguments list pointer */
	if (curs->pst_specific.pst_callproc.pst_arglist != NULL)
	{
	    dup_rb.pss_tree = curs->pst_specific.pst_callproc.pst_arglist;
	    dup_rb.pss_dup  = &cps->pst_specific.pst_callproc.pst_arglist;
	    status = pst_treedup(sess_cb, &dup_rb);

	    if (status != E_DB_OK)
		return (status);
	}

	/* Adjust pointers of [IF node and] CP node */
	if (ifs != NULL)
	{
	    cur_rule = ifs;			/* Rule pair is IF->CP */
	    ifs->pst_link = cps;
	    ifs->pst_specific.pst_if.pst_true = cps;
	}
	else
	{
	    cur_rule = cps;			/* Rule is just CP */
	}

	if (result == NULL)			/* 1st rule - no patch needed */
	{
	    result = cur_rule;		
	}
	else       /* Patch dangling pointers of previous [IF node &] CP node */
	{
	    if (prev_if != NULL)
	    {
		prev_if->pst_next = cur_rule;
		prev_if->pst_specific.pst_if.pst_false = cur_rule;
	    }
	    prev_cp->pst_next = cur_rule;
	    prev_cp->pst_link = cur_rule;
	}

	/* Move on to next statement */
	prev_if = ifs;
	prev_cp = cps;
	curs = curs->pst_next;
    } /* While there are more statements */

    /* Clear dangling pointers of last [IF node and] CP node */
    if (prev_if != NULL)
    {
	prev_if->pst_next = NULL;
	prev_if->pst_specific.pst_if.pst_false = NULL;
    }
    if (prev_cp != NULL)	/* Could be null if all were filtered out */
    {
	prev_cp->pst_next = NULL;
	prev_cp->pst_link = NULL;
    }
    *stmt_dup = result;
    return (E_DB_OK);
} /* pst_ruledup */
