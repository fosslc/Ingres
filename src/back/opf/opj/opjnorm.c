/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <bt.h>
#include    <me.h>
#include    <iicommon.h>
#include    <cui.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <adfops.h>
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
#define OPJ_NORMALIZE TRUE
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 
/**
**
**  Name: OPJNORM.C - Functions for normalizing qualifications.
**
**  Description:
**      This file contains the functions for putting query tree qualifications
**	into conjunctive normal form.
**
**          opj_normlize - Put a qualification into conjunctive normal form
**	    opj_nnorm   - Push the "nots" as far down as possible
**	    opj_traverse- Put a tree with depressed "nots" into conj. norm. form
**	    opj_adjust  - Make sure a tree has a "PST_AND" and a "PST_QLEND" on
**			the right
**	    opj_mvands  - Move the "ands" in a tree into a linear list.
**	    opj_distributive - Apply distributive rule: 
**			A or (B and C) -> (A or B) and (A or C) 
**			for whole sub-tree
**	    opj_demorgan - Apply DeMorgan's theorem
**	    opj_mvors	- Move the "ORs" in a tree into a linear list.
**
**
**  History:    
**      03-apr-86 (jeff)    
**          Adapted from the normalization routines in 3.0
**      03-dec-87 (seputis)
**          Moved normalization to optimizer
**	26-apr-90 (stec)
**	    Added opj_mvors() for SE join qualification processing
**	    to fix bug 20526.
**      21-nov-91 (seg)
**          Comments need to be in real comments, not '#if 0' constructs!
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	24-nov-1995 (inkdo01)
**	    Added support for common conjunctive factor extraction.
**	20-aug-1996 (kch)
**	    In opj_matchbuop(), we now only call opu_dcompare() if the two
**	    datatypes are the same (opu_dcompare() DOES NOT "compare two
**	    values with different types". This change fixes bug 78326.
**	31-oct-96 (nick)
**	    Database procedure local variables ( specifically iirowcount )
**	    have pst_parm_no == 0 but aren't constants obviously. #78845
**	 3-feb-99 (hayke02)
**	    Change factors and terms from int's to f4's in calls to
**	    opj_complxest(). The int's can overflow when dealing with a large
**	    DNF query, preventing noncnf being set TRUE in opj_normalize().
**	    This in turn causes either a SIGSEGV in ope_aseqcls() (E_OP0901)
**	    or E_OP0082, depending on the value of opf_memory. This change
**	    fixes bug 95134.
**       4-Jan-2000 (hayke02)
**	    The inverse operations array invops[] has been modified so that
**	    the less than, less than or equal, greater than and greater
**	    than or equal operations (ADI_GT_OP etc.) now map correctly
**	    to their inverse. Previously, ADI_GT_OP was mapped to ADI_LE_OP
**	    (rather than ADI_LT_OP), ADI_GE_OP was mapped to ADI_LT_OP
**	    (rather than ADI_LE_OP) and so forth. This resulted in
**	    predicates being erroneously dropped during matching conjunctive
**	    factor extraction in opj_matchbuop(). This change fixes bug
**	    99854.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      21-May-2003 (hanal04) Bug 110284 INGSRV 2274
**          Prevent SIGSEGV in opj_traverse().
**      15-jun-2006 (huazh01)
**          add const_clone_array[] into OPJ_ORSTAT structure. 
**          CONST being inserted into const_array[] will also be copied 
**          into this array as well. On opj_scanfail(), we now use the
**          const_clone_array[] to perform the clean up work. 
**          b116108. 
**      17-Mar-2009 (hweho01)
**          The unwanted characters were filled into the table and owner 
**          fields in rdr_rel (DMT_TBL_ENTRY), need to limit the copying 
**          to the string length only. Since these fields are initialized,
**          so the remaining spaces would be set properly.    
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Store blank trimmed names in DMT_ATT_ENTRY
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/*
** Forward Structure Definitions:
*/
typedef struct _OPJ_ORSTAT OPJ_ORSTAT;

/* TABLE OF CONTENTS */
void opj_adjust(
	OPS_SUBQUERY *subquery,
	PST_QNODE **qual);
void opj_mvands(
	OPS_SUBQUERY *subquery,
	PST_QNODE *qual);
static void opj_distributive(
	OPS_SUBQUERY *subquery,
	PST_QNODE *tree);
static bool opj_factor(
	OPS_SUBQUERY *subquery,
	PST_QNODE **or_owner,
	PST_QNODE **cfac_grandpa,
	bool *cfac_rhs);
static bool opj_matchbuop(
	OPS_SUBQUERY *subquery,
	PST_QNODE ***twin_parent,
	PST_QNODE **twin,
	bool *twin_rhs);
static bool opj_match_inlist(
	OPS_SUBQUERY *subquery,
	PST_QNODE *twin1,
	PST_QNODE *twin2);
static void opj_traverse(
	OPS_SUBQUERY *subquery,
	PST_QNODE **qual,
	bool *noncnf);
static void opj_topscan(
	OPS_SUBQUERY *subquery,
	OPJ_ORSTAT *opj_orstatp,
	PST_QNODE **node,
	bool *success);
static void opj_orscan(
	OPS_SUBQUERY *subquery,
	OPJ_ORSTAT *opj_orstatp,
	PST_QNODE **node,
	bool *success,
	bool *failure);
static void opj_andscan(
	OPS_SUBQUERY *subquery,
	OPJ_ORSTAT *opj_orstatp,
	PST_QNODE **node,
	bool *failure,
	int *count);
static bool opj_or2scan(
	OPS_SUBQUERY *subquery,
	OPJ_ORSTAT *opj_orstatp,
	PST_QNODE **node,
	int *count);
static bool opj_bopscan(
	OPS_SUBQUERY *subquery,
	OPJ_ORSTAT *opj_orstatp,
	PST_QNODE **node,
	int *count);
static void opj_scanfail(
	OPS_SUBQUERY *subquery,
	OPJ_ORSTAT *opj_orstatp);
static void opj_inreplace(
	OPS_SUBQUERY *subquery,
	OPJ_ORSTAT *opj_orstatp);
static void opj_orreplace(
	OPS_SUBQUERY *subquery,
	OPJ_ORSTAT *opj_orstatp,
	PST_QNODE **node);
static void opj_sortor(
	OPS_SUBQUERY *subquery,
	PST_QNODE **rowarray,
	i4 *rowcount);
static void opj_tidyup(
	OPS_SUBQUERY *subquery,
	PST_QNODE *nodep);
static void opj_complxest(
	OPS_SUBQUERY *subquery,
	PST_QNODE *nodep,
	f4 *terms,
	f4 *factors,
	bool underor,
	bool *notcnf);
void opj_normalize(
	OPS_SUBQUERY *subquery,
	PST_QNODE **qual);
void opj_mvors(
	PST_QNODE *qual);

/**
**
**  Name: OPJ_ORSTAT - internal structure to house information during 
**	MS Access OR transformation analysis
**
*/

struct _OPJ_ORSTAT
{
    PST_QNODE	*var_array[OPV_MAXVAR];		/* addrs of PST_VARs under
						** consideration */
    PST_QNODE	*const_array[OPV_MAXVAR];	/* addrs of heads of PST_CONST
						** lists, corresponding to 
						** var_array entries */
    PST_QNODE	*bop_array[OPV_MAXVAR];		/* addrs of PST_BOPs containing
						** current VARs */
    PST_QNODE	*in_array[OPV_MAXVAR];		/* addrs of PST_CONSTs in "in"-
						** list in AND-set (for Focus) */

    PST_QNODE	*const_work_array[OPV_MAXVAR];	/* holds curr set of CONSTs 'til
						** we're ready to insert into 
						** CONST lists */

    PST_QNODE   *const_clone_array[OPV_MAXVAR]; /* contains a copy of CONST tree
                                                ** being added to const_array[]
                                                */ 

    PST_QNODE	*first_and;			/* address of root of first AND
						** set of this OR set. The BOPs 
						** under this AND were used to
						** generate var_array. */
    PST_VAR_NODE *in_var;			/* ptr to VAR in in-list */
    i4		const_size[OPV_MAXVAR];		/* max size of each const in
						** current OR set */
    i4		array_size;			/* count of array entries */
    i4		row_count;			/* count of CONST "rows" in 
						** current OR set */
    i4		in_count;			/* count of CONSTs in current
						** in-list */
    i4		in_col;				/* in-list "column" in const
						** array */
    OPV_BMVARS	alldups;			/* bit map to show if some VAR has
						** all same values (TRUE means 
						** all dups so far) */
    OPV_BMVARS	var_map;			/* bit map for current AND set */
    bool	first;				/* TRUE: processing first AND set
						** for current OR set */
};

/*
**  Definition of static variables and forward static functions.
*/

static const ADI_OP_ID invops[] = {ADI_NE_OP, -1, -1, -1, -1, -1, ADI_GT_OP, 
		ADI_GE_OP, ADI_EQ_OP, ADI_LT_OP, ADI_LE_OP};


/*{
** Name: adjust	- Terminate qualification with an AND and a QLEND
**
** Description:
**      This function puts a PST_AND and a PST_QLEND node at the rightmost
**	branch of a qualification tree.
**
** Inputs:
**      mstream                         Memory stream
**	qual				Qualification tree to terminate
**	err_blk				Filled in if an error happens
**
** Outputs:
**      qual                            Terminated with an AND and a QLEND
**	err_blk				Filled in if an error happens
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
**          Adapted from adjust() in 3.0
**	20-may-87 (daved)
**	    check for qlend when entering this routine
**      03-dec-87 (seputis)
**          rewrote for OPF, and eliminated recursion
**	27-apr-89 (seputis)
**	    handle NULL terminated list - b4439
[@history_line@]...
*/
VOID
opj_adjust(
	OPS_SUBQUERY	*subquery,
	PST_QNODE	**qual)
{

    while ((*qual) && ((*qual)->pst_sym.pst_type == PST_AND))
	qual = &(*qual)->pst_right;

    if (!(*qual))
	*qual = opv_qlend(subquery->ops_global);    /* get a PST_QLEND node */
    else if ((*qual)->pst_sym.pst_type != PST_QLEND)
    {
	/* Attach a PST_AND and a PST_QLEND node */
	PST_QNODE		*qlend;
	PST_QNODE		*andnode;
	qlend = opv_qlend(subquery->ops_global);    /* get a PST_QLEND node */
	andnode = opv_opnode(subquery->ops_global, PST_AND, (ADI_OP_ID)0,
	    (OPL_IOUTER)(*qual)->pst_sym.pst_value.pst_s_op.pst_joinid); 
	andnode->pst_left = *qual;
	andnode->pst_right = qlend;
	/* Make the new root the newly created AND node */
	*qual = andnode;
    }
}

/*{
** Name: opj_mvands	- Push all ANDs into a linear list
**
** Description:
**      This function pushes all the ANDs in a qualification tree into a linear
**	list.  It guarantees that all AND nodes will be on the rightmost edge
**	of the qualification tree.
**
** Inputs:
**      qual                            Pointer to the qualification tree
**
** Outputs:
**      qual                            All AND nodes moved to right edge
**	Returns:
**	    E_DB_OK			Success (Failure impossible)
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-may-86 (jeff)
**          Adapted from mvands() in 3.0.  Made it non-recursive.
**      03-dec-87 (seputis)
**          Adapted to OPF
**      26-mar-89 (seputis)
**          remove extra AND->QLEND pair
**	9-feb-93 (ed)
**	    - fix b49317, account for left operand of PST_AND to be null
[@history_line@]...
*/
VOID
opj_mvands(
	OPS_SUBQUERY	*subquery,
	PST_QNODE	*qual)
{
    register PST_QNODE  *lp;
    register PST_QNODE	*rp;

    /* A PST_QLEND node means there is nothing more on the right */
    while (qual->pst_sym.pst_type != PST_QLEND)
    {
	lp = qual->pst_left;
	rp = qual->pst_right;
	if (!lp)
	{   /* left side is null so eliminate redundant PST_AND node */
	    if (rp->pst_sym.pst_type == PST_QLEND)
	    {
		qual->pst_sym.pst_type = PST_QLEND;
		break;
	    }
	    lp = qual->pst_left = rp->pst_left;
	    qual->pst_right = (rp = rp->pst_right);
	}
	/* Move PST_ANDs on the left up to the right until there are no more */
	while (lp->pst_sym.pst_type == PST_AND)
	{
	    /* Move the PST_AND on the left up to the right */
	    if (!lp->pst_right || (lp->pst_right->pst_sym.pst_type == PST_QLEND))
		qual->pst_left = lp->pst_left; /* get rid of extra AND */
	    else
	    {
		qual->pst_left = lp->pst_right;
		qual->pst_right = lp;
		lp->pst_right = rp;
	    }

	    /* Reset the left and right ptrs, because there are new children */
	    lp = qual->pst_left;
	    rp = qual->pst_right;
	}

	/* Go down the right */
	qual = rp;
    }
}

/*{
** Name: opj_distributive	- A or (B and C) ==> (A or B) and (A or C)
**
** Description:
**      This function takes a qualification tree that has nots only at the
**	lower ends, and places it in conjunctive normal form by repeatedly
**	applying the replacement rule: A or (B and C) ==> (A or B) and (A or C)
**
** Inputs:
**      subquery                        Pointer to OPF subquery state variable
**      qual                            Pointer to pointer to tree to normalize
**
** Outputs:
**      qual                            Set to point to pointer to normalized
**					tree
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can allocate memory
**
** History:
**	05-may-86 (jeff)
**          Adapted from 3.0 version of norm()
**      03-dec-87 (seputis)
**          Moved from PSF, improved and adapted traversal
**      6-nov-91 (seputis)
**          check for missing PST_AND operand, part of b40402
[@history_line@]...
*/
static VOID
opj_distributive(
	OPS_SUBQUERY	*subquery,
	PST_QNODE       *tree)
{
    do
    {
	if (tree->pst_sym.pst_type == PST_OR)
	{
	    PST_QNODE           *dup;
	    PST_QNODE		*or_nodep;
	    PST_QNODE		*rnodep;
	    PST_QNODE		*lnodep;

	    rnodep = tree->pst_right;
	    lnodep = tree->pst_left;
	    if (rnodep->pst_sym.pst_type == PST_AND)
	    {
		/* check to see if a degenerate PST_AND exists */
		if (!rnodep->pst_right || (rnodep->pst_right->pst_sym.pst_type == PST_QLEND))
		{
		    tree->pst_right = rnodep->pst_left;	/* remove AND node since it does
						** not have a right child */
		    continue;
		}
		if (!rnodep->pst_left || (rnodep->pst_left->pst_sym.pst_type == PST_QLEND))
		{
		    tree->pst_right = rnodep->pst_right; /* remove AND node since it does
						** not have a left child */
		    continue;
		}

		/*
		** Combine left subtree with each subtree of right subtree.
		** Use copy first so that copy is guaranteed to be the same as
		** the original.
		*/
		dup = tree->pst_left;
		opv_copytree(subquery->ops_global, &dup);/*duplicate the tree */
		or_nodep = opv_opnode(subquery->ops_global, PST_OR, 
		    (ADI_OP_ID)0, (OPL_IOUTER)dup->pst_sym.pst_value.pst_s_op.pst_joinid);
		or_nodep->pst_right = dup;
		or_nodep->pst_left = tree->pst_right->pst_left;
		/* Re-use the AND node */
		tree->pst_right->pst_sym.pst_type = PST_OR;
		tree->pst_right->pst_left = tree->pst_left;

		/* Hook up root to sub-trees and change root to an AND */
		tree->pst_left = or_nodep;
		tree->pst_sym.pst_type = PST_AND;
	    }
	    else if (lnodep->pst_sym.pst_type == PST_AND)
	    {
		/* check to see if a degenerate PST_AND exists */
		if (!lnodep->pst_right || (lnodep->pst_right->pst_sym.pst_type == PST_QLEND))
		{
		    tree->pst_left = lnodep->pst_left;	/* remove AND node since it does
						** not have a right child */
		    continue;
		}
		if (!lnodep->pst_left || (lnodep->pst_left->pst_sym.pst_type == PST_QLEND))
		{
		    tree->pst_left = lnodep->pst_right; /* remove AND node since it does
						** not have a left child */
		    continue;
		}

		/*
		** Combine right subtree with each subtree of left subtree.
		** Use copy first so that copy is guaranteed to be the same as
		** the original.
		*/
		dup = tree->pst_right;
		opv_copytree(subquery->ops_global, &dup);
		or_nodep = opv_opnode(subquery->ops_global, PST_OR, 
		    (ADI_OP_ID)0, (OPL_IOUTER)dup->pst_sym.pst_value.pst_s_op.pst_joinid);
		or_nodep->pst_right = dup;
		or_nodep->pst_left = tree->pst_left->pst_left;

		/* Re-use the AND node */
		tree->pst_left->pst_sym.pst_type = PST_OR;
		tree->pst_left->pst_left = tree->pst_right;

		/* Hook up root to sub-trees and change root to an AND */
		tree->pst_right = or_nodep;
		tree->pst_sym.pst_type = PST_AND;
	    }
	    else
		return;			    /* AND is not under OR so OK */
	}
	else if (tree->pst_sym.pst_type != PST_AND)
	    return;			    /* not a boolean operator */

	/* Now normalize left and right trees just created */
	opj_distributive(subquery, tree->pst_left); /* recurse down left side */
	tree = tree->pst_right;		/* iterate down right side */
    } while (tree);
}

/*{
** Name: opj_factor - Remove common conjunctive factors from a subtree.
**
** Description:
**      This function traverses a qualification tree from the bottom up,
**	looking for common factors to extract (e.g. (A and B) or (A and C)
**	is transformed to A and (B or C)). 
**
** Inputs:
**      subquery                        ptr to OPF subquery state variable
**      or_owner                        Pointer to pst_left/right which owns
**					PST_OR we're analyzing 
**	cfac_grandpa			Pointer to pst_left/right which owns
**					node currently being analyzed
**	cfac_rhs			Flag indicating whether current node 
**					is under a pst_right (TRUE) or left
**
** Outputs:
**      or_owner			on top of re-organized subtree with
**					"and" of common factor at top.
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-nov-1995 (inkdo01)
**	    Brand spanking new.
**	19-oct-00 (inkdo01)
**	    Removed line of code in UOP case to fix bug 102975.
**	30-sep-04 (inkdo01)
**	    Don't factor out packed IN-lists.
**	13-may-05 (inkdo01)
**	    Do factor out packed IN-lists.
**      09-apr-2008 (huazh01)
**          Invalidate the subquery's varmap after removing nodes from
**          query tree.
**          bug 120233.
[@history_template@]...
*/

static bool
opj_factor(
	OPS_SUBQUERY	*subquery,
	PST_QNODE	**or_owner,
	PST_QNODE	**cfac_grandpa,
	bool		*cfac_rhs)
{
    PST_QNODE		**cfac_parent;
    PST_QNODE		*twin;
    PST_QNODE		**twin_parent;
    bool		twin_rhs;
    bool		gotone;
    PST_QNODE		*oraddr;


    /* This function recursively descends qualification tree looking for 
    ** ANDs to recurse on, and BOPs to match on. Anything else stops
    ** the descent.                                                      */

    cfac_parent = &(*cfac_grandpa)->pst_left;
				/* start looking down lhs */
    *cfac_rhs = FALSE;
    gotone = FALSE;		/* init loop terminator */

    do 				/* loop 'til we quit */
    {
	switch ((*cfac_parent)->pst_sym.pst_type) {
	 case PST_AND:
	    if ((*cfac_parent)->pst_sym.pst_value.pst_s_op.pst_joinid !=
			(*or_owner)->pst_sym.pst_value.pst_s_op.pst_joinid)
		return(FALSE);	/* stay within same oj */
	    if (opj_factor(subquery, or_owner, cfac_parent, cfac_rhs))
		return(TRUE);	/* recurse on left */
	    *cfac_rhs = TRUE;
	    cfac_grandpa = cfac_parent;
				/* grandpa is PST_AND addr for iteration */
	    cfac_parent = &(*cfac_parent)->pst_right;
	    break;		/* then iterate on right */

	 case PST_UOP:
	    /* Only interested in IS NULL and IS NOT NULL */
	    if (!((*cfac_parent)->pst_sym.pst_value.pst_s_op.pst_opno == 
			ADI_ISNUL_OP ||
		(*cfac_parent)->pst_sym.pst_value.pst_s_op.pst_opno == 
			ADI_NONUL_OP)) return(FALSE);
	    goto matchbuop_merge;
	 case PST_BOP:
	    /* Only interested in compops (=, <>, <, <=, >, >=) */
	    if ((*cfac_parent)->pst_sym.pst_value.pst_s_op.pst_opno > ADI_GE_OP ||
		invops[(*cfac_parent)->pst_sym.pst_value.pst_s_op.pst_opno] < 0)
	     return(FALSE);
	  matchbuop_merge:
	    if ((*cfac_parent)->pst_sym.pst_value.pst_s_op.pst_joinid !=
			(*or_owner)->pst_sym.pst_value.pst_s_op.pst_joinid)
		return(FALSE);	/* stay within same oj */
	    twin = *cfac_parent;
				/* prepare for match search */
	    twin_parent = &(*or_owner)->pst_right;
	    if (!opj_matchbuop(subquery, &twin_parent, &twin, &twin_rhs)) 
		return(FALSE);
	    gotone = TRUE; 	/* show we've got a match */
	    break;

	 default: return(FALSE);
	}  	/* end of switch */
    } while (!gotone);		/* loop until success or return */


    /* If we get to here, we have a candidate factor. */

    if ((*or_owner)->pst_left == *cfac_parent || 
		(*or_owner)->pst_right == twin)
    {
	/* First handle the degenerate cases: a OR a, a OR x AND ... AND
	**  a AND ..., x AND ... AND a AND ... OR a => a. That is, a is
	** only relevant factor */
	*or_owner = twin;
    }
     else
    {
	/* At least one AND separates owning OR from common factors  
	**  on BOTH sides. Move left AND above the OR, then combine loose 
	**  BOPs on left of OR, then loose BOPs on right */

	oraddr = *or_owner;	/* save OR address */
	*or_owner = *cfac_grandpa;
				/* put AND above OR */
	if (*cfac_rhs) 
	{
	    *cfac_grandpa = (*cfac_grandpa)->pst_left;
				/* collapse "a" sibling into grandpa */
	    (*or_owner)->pst_left = oraddr;
				/* "a" on right, OR on left */
	}
	 else 
	{
	    *cfac_grandpa = (*cfac_grandpa)->pst_right;
				/* collapse "a" sibling into grandpa */
	    (*or_owner)->pst_right = oraddr;
				/* "a" on left, OR on right */
	}

	if (twin_rhs) (*twin_parent) = (*twin_parent)->pst_left;
	 else (*twin_parent) = (*twin_parent)->pst_right;
				/* just suck other side of twin up 1 level */
    }

    subquery->ops_vmflag = FALSE; 

    return (TRUE);

}	/* end of opj_factor */

/*{
** Name: opj_matchbuop - search for matching conjunctive factor.
**
** Description:
**      This function works in conjunction with opj_factor to search the 
**	subtree under the right side of a PST_OR looking for a matching 
**	PST_BOP to one found under the left side of the same OR.
**
** Inputs:
**      subquery                        ptr to OPF subquery state variable
**      twin_parent			Ptr to pointer to pst_left/right which
**					owns the node we're analyzing. On the 
**					first call from opj_factor, this is 
**					the right side of the PST_OR
**	twin				Ptr to pointer to PST_BOP to be matched
**	twin_rhs			Flag indicating whether current node 
**					is under a pst_right (TRUE) or left
**
** Outputs:
**      twin_parent			Ptr to pst_left/right owning PST_OR or
**					PST_AND owning matching BOP
**	twin				Ptr to ptr to matching PST_BOP
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-nov-1995 (inkdo01)
**	    Brand spanking new.
**	20-aug-1996 (kch)
**	    We now only call opu_dcompare() if the datatypes of the two
**	    constants are the same (opu_dcompare() DOES NOT "compare two
**	    values with different types"). This change fixes bug 78326.
**	31-oct-96 (nick)
**	    Trap null db_data before using it in opu_dcompare(). #78845
**	4-nov-97 (inkdo01)
**	    Check if caller is UOP/BOP before entering BOP/UOP case's
**	    (bug 86936).
**	29-oct-01 (hayke02)
**	    We now set the constants' type to DB_TXT_TYPE for text vars
**	    before the call to opu_dcompare(), and then restore the original
**	    types (DB_VCH_TYPE) after the compare, using twin_type. This
**	    ensures that the text strings '' and ' ' are not treated as
**	    equal. This change fixes bug 108731.
**	19-sep-03 (hayke02)
**	    More trapping of null db_data's before using them in opu_dcompare().
**	    This change fixes bug 110938.
**	30-sep-04 (inkdo01)
**	    Don't factor out packed IN-lists.
**	13-may-05 (inkdo01)
**	    Do factor out packed IN-lists.
[@history_template@]...
*/


static bool
opj_matchbuop(
	OPS_SUBQUERY	*subquery,
	PST_QNODE 	***twin_parent,
	PST_QNODE	**twin,
	bool		*twin_rhs)
{
    PST_QNODE		**local_parent;
    PST_QNODE		*locleft;
    PST_QNODE		*locright;
    bool		sameord, matched;
    DB_DT_ID		twin_type;

    /* This function recursively descends the qualification subtree under 
    ** the right hand side of the OR, looking for a BOP or UOP which matches
    ** twin. When found, it and its owner node pointer are returned  */

    local_parent = *twin_parent;

    do				/* loop 'til we return */
    {
	switch ((*local_parent)->pst_sym.pst_type) {

	 case PST_AND:
	    if ((*local_parent)->pst_sym.pst_value.pst_s_op.pst_joinid !=
			(*twin)->pst_sym.pst_value.pst_s_op.pst_joinid)
		return(FALSE);	/* must be in same oj as twin */
	    /* Look under AND - recursing on both AND operands */
	    local_parent = &(**twin_parent)->pst_left;
				/* prepare to recurse on lhs */
	    *twin_rhs = FALSE;
	    if (opj_matchbuop(subquery, &local_parent, twin, twin_rhs))
	    {			/* got a match */
		if ((*twin_rhs) && (*local_parent)->pst_right == *twin 
			|| (*local_parent)->pst_left == *twin)
		  		*twin_parent = local_parent;
		return(TRUE);
	    }

	    local_parent = &(**twin_parent)->pst_right;
				/* prepare to recurse on rhs */
	    *twin_rhs = TRUE;
	    if (opj_matchbuop(subquery, &local_parent, twin, twin_rhs))
	    {			/* got a match */
		if ((*twin_rhs) && (*local_parent)->pst_right == *twin 
			|| (*local_parent)->pst_left == *twin)
		  		*twin_parent = local_parent;
		return(TRUE);
	    }

	    return(FALSE);	/* no other escape from here */

	 case PST_UOP:
	    if ((*twin)->pst_sym.pst_type != PST_UOP) return(FALSE);
				/* better be another UOP */
	    if ((*local_parent)->pst_sym.pst_value.pst_s_op.pst_joinid !=
			(*twin)->pst_sym.pst_value.pst_s_op.pst_joinid)
		return(FALSE);	/* must be in same oj as twin */
	    /* See if operators match - *twin must be IS [N0T] NULL */
	    if ((*twin)->pst_sym.pst_value.pst_s_op.pst_opno ==
		   (*local_parent)->pst_sym.pst_value.pst_s_op.pst_opno &&
		(*twin)->pst_left->pst_sym.pst_type ==
		   (*local_parent)->pst_left->pst_sym.pst_type)

	    {		/* structural match - now check operand contents */
		switch ((*twin)->pst_left->pst_sym.pst_type) {

		 case PST_VAR:
		    if ((*twin)->pst_left->pst_sym.pst_value.pst_s_var.
						pst_atno.db_att_id !=
		        (*local_parent)->pst_left->pst_sym.pst_value.pst_s_var.
						pst_atno.db_att_id)
			return(FALSE);
		    if ((*twin)->pst_left->pst_sym.pst_value.pst_s_var.
						pst_vno !=
		        (*local_parent)->pst_left->pst_sym.pst_value.pst_s_var.
						pst_vno) return(FALSE);
		    break;

		 case PST_CONST:
	    	    if ((*twin)->pst_left->pst_sym.pst_value.pst_s_cnst.
				pst_parm_no !=
		   	(*local_parent)->pst_left->pst_sym.pst_value.pst_s_cnst.
				pst_parm_no) 
			return(FALSE);

		    if ((*twin)->pst_left->pst_sym.pst_dataval.db_datatype
			!= (*local_parent)->pst_left->pst_sym.pst_dataval.db_datatype)
			return(FALSE);

	    	    if (((*twin)->pst_left->pst_sym.pst_value.pst_s_cnst.
				pst_parm_no == 0) && 
			((*twin)->pst_left->pst_sym.pst_dataval.db_data 
				!= NULL) &&
			opu_dcompare(subquery->ops_global, 
			    &(*twin)->pst_left->pst_sym.pst_dataval, 
			    &(*local_parent)->pst_left->pst_sym.pst_dataval))
			return(FALSE);
	    	    break;

		 default: return(FALSE);
		}	/* end of uop operand switch */

		*twin = *local_parent;	/* remarkably, we have a match */
		return(TRUE); 		/* notify the world */
	    }	/* end of long "if" after PST_UOP */
	     else return(FALSE);	/* PST_UOP, but no go */

	 case PST_BOP:
	    if ((*twin)->pst_sym.pst_type != PST_BOP) 
		return(FALSE);	/* better be another BOP */
	    if (((*twin)->pst_sym.pst_value.pst_s_op.pst_flags & PST_INLIST) !=
	        ((*local_parent)->pst_sym.pst_value.pst_s_op.pst_flags 
			& PST_INLIST))
		return(FALSE);	/* both or neither must be packed IN-lists */
	    if ((*local_parent)->pst_sym.pst_value.pst_s_op.pst_joinid !=
			(*twin)->pst_sym.pst_value.pst_s_op.pst_joinid)
		return(FALSE);	/* must be in same oj as twin */
	    /* See if operators match or are inverse (e.g., > is inverse of <=)
	    ** Note that *twin is guaranteed to be a comparative operator */
	    if (!((sameord = (*twin)->pst_sym.pst_value.pst_s_op.pst_opno ==
		   (*local_parent)->pst_sym.pst_value.pst_s_op.pst_opno) ||
		invops[(*twin)->pst_sym.pst_value.pst_s_op.pst_opno] ==
		   (*local_parent)->pst_sym.pst_value.pst_s_op.pst_opno))
		return(FALSE);	/* opno's don't match */

	    if (((*twin)->pst_sym.pst_value.pst_s_op.pst_opno == ADI_EQ_OP ||
		(*twin)->pst_sym.pst_value.pst_s_op.pst_opno == ADI_NE_OP) &&
		(*local_parent)->pst_left->pst_sym.pst_type ==
		    (*twin)->pst_left->pst_sym.pst_type || sameord)
	               	/* if operation is EQ or NE and operands are same
			** type, or if operation is simply same */
	    {
		locleft = (*local_parent)->pst_left;
		locright = (*local_parent)->pst_right;
	    }
	     else	/* otherwise operations may be inverse */
	    {
		locleft = (*local_parent)->pst_right;
		locright = (*local_parent)->pst_left;
	    }

	    matched = FALSE; 

	    if ((*twin)->pst_left->pst_sym.pst_type ==
		   locleft->pst_sym.pst_type &&
	        (*twin)->pst_right->pst_sym.pst_type ==
		   locright->pst_sym.pst_type)

	    {		/* structural match - now check operand contents */
		if ((*twin)->pst_sym.pst_value.pst_s_op.pst_flags & PST_INLIST)
		{
		    if (opj_match_inlist(subquery, (*twin), (*local_parent)))
		    {
			*twin = *local_parent;
			return(TRUE);	/* matching IN-lists */
		    }
		    else return(FALSE);	/* non-matching IN-lists */
		}

		while (!matched)	/* "while" is for EQ/NE inversion */
		{
		    switch ((*twin)->pst_left->pst_sym.pst_type) {

		     case PST_VAR:
			if ((*twin)->pst_left->pst_sym.pst_value.pst_s_var.
						pst_atno.db_att_id !=
		        	locleft->pst_sym.pst_value.pst_s_var.
						pst_atno.db_att_id)
			 break;
			if ((*twin)->pst_left->pst_sym.pst_value.pst_s_var.
						pst_vno ==
		        	locleft->pst_sym.pst_value.pst_s_var.
						pst_vno) matched = TRUE;
			break;

		     case PST_CONST:
	    		if ((*twin)->pst_left->pst_sym.pst_value.pst_s_cnst.
						pst_parm_no !=
		   		locleft->pst_sym.pst_value.pst_s_cnst.pst_parm_no) 
			 break;
			if ((*twin)->pst_left->pst_sym.pst_dataval.db_datatype
			    != locleft->pst_sym.pst_dataval.db_datatype)
			    return(FALSE);
			twin_type =
			    (*twin)->pst_left->pst_sym.pst_dataval.db_datatype;
			if ((*twin)->pst_right->pst_sym.pst_type == PST_VAR &&
			abs((*twin)->pst_right->pst_sym.pst_dataval.db_datatype)
								== DB_TXT_TYPE)
			{
			    (*twin)->pst_left->pst_sym.pst_dataval.db_datatype
								= DB_TXT_TYPE;
			    locleft->pst_sym.pst_dataval.db_datatype
								= DB_TXT_TYPE;
			}
	    		if (((*twin)->pst_left->pst_sym.pst_value.pst_s_cnst.
					pst_parm_no == 0) && 
			    ((*twin)->pst_left->pst_sym.pst_dataval.db_data
					!= NULL) &&
			    (locleft->pst_sym.pst_dataval.db_data != NULL) &&
				!opu_dcompare(subquery->ops_global, 
					&(*twin)->pst_left->pst_sym.pst_dataval, 
					&locleft->pst_sym.pst_dataval))
			 matched = TRUE;
			(*twin)->pst_left->pst_sym.pst_dataval.db_datatype
								    = twin_type;
			locleft->pst_sym.pst_dataval.db_datatype = twin_type;
	    		break;

		     default: return(FALSE);
		    }	/* end of lhs switch */

		    if (matched) break;
		    if (locleft == (*local_parent)->pst_right ||
			(*twin)->pst_sym.pst_value.pst_s_op.pst_opno 
							!= ADI_EQ_OP &&
			(*twin)->pst_sym.pst_value.pst_s_op.pst_opno 
							!= ADI_NE_OP)
		     return(FALSE);
				/* no match, and not invertable */
		     else
		    {
			locleft = (*local_parent)->pst_right;
			locright = (*local_parent)->pst_left;
		    }
		}	/* end of loop to force EQ/NE inversion */

		/* If we get here, left operands of BOP match. Only the 
		** right operands to check. */
		
		switch ((*twin)->pst_right->pst_sym.pst_type) {

		 case PST_VAR:
		    if ((*twin)->pst_right->pst_sym.pst_value.pst_s_var.
						pst_atno.db_att_id !=
		        locright->pst_sym.pst_value.pst_s_var.
						pst_atno.db_att_id)
			return(FALSE);
		    if ((*twin)->pst_right->pst_sym.pst_value.pst_s_var.
						pst_vno !=
		        locright->pst_sym.pst_value.pst_s_var.
						pst_vno) return(FALSE);
		    break;

		 case PST_CONST:
	    	    if ((*twin)->pst_right->pst_sym.pst_value.pst_s_cnst.
				pst_parm_no !=
		   	locright->pst_sym.pst_value.pst_s_cnst.
				pst_parm_no) return(FALSE);
		    if ((*twin)->pst_right->pst_sym.pst_dataval.db_datatype
			!= locright->pst_sym.pst_dataval.db_datatype)
			return(FALSE);
		    twin_type =
			(*twin)->pst_right->pst_sym.pst_dataval.db_datatype;
		    if (abs((*twin)->pst_left->pst_sym.pst_dataval.db_datatype)
			==
			DB_TXT_TYPE)
		    {
			(*twin)->pst_right->pst_sym.pst_dataval.db_datatype =
								DB_TXT_TYPE;
			locright->pst_sym.pst_dataval.db_datatype = DB_TXT_TYPE;
		    }
		    if (((*twin)->pst_right->pst_sym.pst_value.pst_s_cnst.
				pst_parm_no == 0) &&
			((*twin)->pst_right->pst_sym.pst_dataval.db_data
				!= NULL) &&
			(locright->pst_sym.pst_dataval.db_data != NULL) &&
			opu_dcompare(subquery->ops_global, 
				&(*twin)->pst_right->pst_sym.pst_dataval, 
				&locright->pst_sym.pst_dataval))
		    {
			(*twin)->pst_right->pst_sym.pst_dataval.db_datatype
								    = twin_type;
			locright->pst_sym.pst_dataval.db_datatype = twin_type;
			return(FALSE);
		    }
		    (*twin)->pst_right->pst_sym.pst_dataval.db_datatype
								= twin_type;
		    locright->pst_sym.pst_dataval.db_datatype = twin_type;
	    	    break;

		 default: return(FALSE);
		}	/* end of rhs switch */

		*twin = *local_parent;	/* remarkably, we have a match */
		return(TRUE); 		/* notify the world */
	    }	/* end of long "if" after PST_BOP */
	     else return(FALSE);	/* PST_BOP, but no go */

	 default: return(FALSE);	/* nothing else is interesting */

	}	/* end of local_parent switch */

    } while(TRUE);	/* end of iteration loop - we will ALWAYS 
			** return out of this loop */

}	/* end of opj_matchbuop */

/*{
** Name: opj_match_inlist - check for matching packed IN-listsr.
**
** Description:
**      This function compares 2 packed IN-list predicates to verify 
**	that they are equivalent. Only "a in (..." are acceptible for now,
**	NOT "a not in (...".
**
** Inputs:
**      subquery                        ptr to OPF subquery state variable
**      twin1				Ptr to 1st IN-list PST_BOP.
**	twin2				Ptr to 2nd IN-list PST_BOP.
**
** Outputs:
**	Returns:			TRUE - if the BOPs are equivalent
**					FALSE - otherwise
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	13-may-05 (inkdo01)
**	    Newbee.
[@history_template@]...
*/

static bool
opj_match_inlist(
	OPS_SUBQUERY	*subquery,
	PST_QNODE 	*twin1,
	PST_QNODE	*twin2)

{
    PST_QNODE	*left1, *right1, *left2, *right2, *const1, *const2;
    bool	nomatch;

    /* First perform cursory tests - both in or not in, both on same PST_VARs,
    ** both comparing lists of constants. */
    if (twin1->pst_sym.pst_value.pst_s_op.pst_opno != 
	twin2->pst_sym.pst_value.pst_s_op.pst_opno)
	return(FALSE);

    left1 = twin1->pst_left;
    left2 = twin2->pst_left;
    if (left1->pst_sym.pst_type != PST_VAR ||
	left2->pst_sym.pst_type != PST_VAR)
	return(FALSE);
    if (left1->pst_sym.pst_value.pst_s_var.pst_vno !=
	left2->pst_sym.pst_value.pst_s_var.pst_vno ||
	left1->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id !=
	left2->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
	return(FALSE);

    right1 = twin1->pst_right;
    right2 = twin2->pst_right;
    if (right1->pst_sym.pst_type != PST_CONST ||
	right2->pst_sym.pst_type != PST_CONST)
	return(FALSE);

    /* Now verify that IN-lists are all PST_CONSTs and that they cover
    ** each other. This requires 2 sets of nested loops - one to find
    ** matches for all twin1 values in the twin2 list and one to find 
    ** matches for all twin2 values in the twin1 list. */
    for (const1 = right1; const1; const1 = const1->pst_left)
    {
	nomatch = TRUE;
	if (const1->pst_sym.pst_type != PST_CONST)
	    return(FALSE);
	for (const2 = right2; const2 && nomatch; const2 = const2->pst_left)
	{
	    if (const2->pst_sym.pst_type != PST_CONST)
		return(FALSE);
	    if (const1->pst_sym.pst_value.pst_s_cnst.pst_parm_no !=
		const2->pst_sym.pst_value.pst_s_cnst.pst_parm_no)
		continue;
	    if (const1->pst_sym.pst_dataval.db_datatype !=
		const2->pst_sym.pst_dataval.db_datatype)
		continue;
	    if (const1->pst_sym.pst_value.pst_s_cnst.pst_parm_no == 0 &&
		const1->pst_sym.pst_dataval.db_data &&
		const2->pst_sym.pst_dataval.db_data && 
		opu_dcompare(subquery->ops_global, 
			&const1->pst_sym.pst_dataval,
			&const2->pst_sym.pst_dataval) == 0)
		nomatch = FALSE;
	}
	if (nomatch)
	    return(FALSE);	/* no match for this guy */
    }	/* end of "twin2 covers twin1" loops */

    for (const2 = right2; const2; const2 = const2->pst_left)
    {
	nomatch = TRUE;
	if (const2->pst_sym.pst_type != PST_CONST)
	    return(FALSE);
	for (const1 = right1; const1 && nomatch; const1 = const1->pst_left)
	{
	    if (const2->pst_sym.pst_value.pst_s_cnst.pst_parm_no !=
		const1->pst_sym.pst_value.pst_s_cnst.pst_parm_no)
		continue;
	    if (const2->pst_sym.pst_dataval.db_datatype !=
		const1->pst_sym.pst_dataval.db_datatype)
		continue;
	    if (const2->pst_sym.pst_value.pst_s_cnst.pst_parm_no == 0 &&
		const2->pst_sym.pst_dataval.db_data &&
		const1->pst_sym.pst_dataval.db_data && 
		opu_dcompare(subquery->ops_global, 
			&const2->pst_sym.pst_dataval,
			&const1->pst_sym.pst_dataval) == 0)
		nomatch = FALSE;
	}
	if (nomatch)
	    return(FALSE);	/* no match for this guy */
    }	/* end of "twin1 covers twin2" loops */

    return(TRUE);		/* success, if we get this far */
}


/*{
** Name: opj_traverse	- Put a tree with depressed "nots" into CNF
**
** Description:
**      This function traverses a qualification tree from the bottom up, so
**	that the conversion of ORs of ANDs can take place at the innermost
**	clauses first.  That is, it normalizes before replication rather than
**	after replication.
**
** Inputs:
**      subquery                        ptr to OPF subquery state variable
**      qual                            Pointer to qualification tree to
**					traverse (must have nots depressed)
**
** Outputs:
**      qual                            Put into conjunctive normal form
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	05-may-86 (jeff)
**          adapted from travers() in 3.0
**      03-dec-87 (seputis)
**          adapted from PSF
**	28-nov-95 (inkdo01)
**	    Added support to extract common conjunctive factors from ORs.
**      21-May-2003 (hanal04) Bug 110284 INGSRV 2274
**          opj_factor() may alter qual so the assumption that *qual
**          references a PST_OR node which has non-null pst_left
**          and pst_right children does not always hold. Prevent a
**          SIGSEGV by breaking out of the while loop if *qual is
**          about to be set to NULL.
[@history_template@]...
*/
static VOID
opj_traverse(
	OPS_SUBQUERY	    *subquery,
	PST_QNODE	    **qual,
	bool		    *noncnf)
{

    /* Traverse any subtrees that could be normalized */
    if ((*qual)->pst_right &&
	((*qual)->pst_right->pst_sym.pst_type == PST_AND ||
	 (*qual)->pst_right->pst_sym.pst_type == PST_OR))
	opj_traverse(subquery, &(*qual)->pst_right, noncnf);

    if ((*qual)->pst_left &&
	((*qual)->pst_left->pst_sym.pst_type == PST_AND ||
	 (*qual)->pst_left->pst_sym.pst_type == PST_OR))
	opj_traverse(subquery, &(*qual)->pst_left, noncnf);

    /* For the time being, if we find a non-zero pst_opmeta in a BOP
    ** node, we'll do the CNF transform. Otherwise, SEjoin compilation
    ** can fail. */
    if ((*qual)->pst_left && (*qual)->pst_left->pst_sym.pst_type == PST_BOP
	&& (*qual)->pst_left->pst_sym.pst_value.pst_s_op.pst_opmeta !=
	PST_NOMETA ||
	(*qual)->pst_right && (*qual)->pst_right->pst_sym.pst_type == PST_BOP
	&& (*qual)->pst_right->pst_sym.pst_value.pst_s_op.pst_opmeta !=
	PST_NOMETA) *noncnf = FALSE;

    /* After descending the qualification tree, the normalization is
    ** performed on the way back up the call stack. First we do the
    ** common conjunctive factor extraction, then the re-distribution
    ** of remaining conjunctive factors across the ORs. */

    /* Attempt common factor extraction from under ORs. This is done
    ** iteratively until no more common factors are found. */
    if ((*qual)->pst_sym.pst_type == PST_OR)
    {
    	bool	cfac_rhs;
	while ((*qual)->pst_sym.pst_type == PST_OR)
	{
	    if (!opj_factor(subquery, qual, qual, &cfac_rhs)) break;
	    else if (cfac_rhs) 
	       {
		   if((*qual)->pst_left)
		      qual = &(*qual)->pst_left;
                   else
		      break;
               }
	       else 
	       {
		   if((*qual)->pst_right)
	               qual = &(*qual)->pst_right;
		   else
		       break;
	       }
	}
    }

    /* Normalize ANDs under ORs (if any left) */
    if (!(*noncnf) && (*qual)->pst_sym.pst_type == PST_OR) 
		opj_distributive(subquery, *qual);
}


/*{
** Name: OPJ_TOPSCAN	- analyze where clause (parse tree) looking for 
**	the MS Access "special" pattern.
**
** Description:
**      This is the root function in a set of recursively descending functions
**	which look for a class of where clause built by (among possible others)
**	MS Access. This class of where clause contains ANDs nested within ORs,
**	and ordinarily triggers the CNF conversion of Ingres. They are built
**	typically to implement joins between a local table and a remote Ingres
**	table (in the absence of true distributed processing) and also to 
**	handle retrievals performed for possible updates. Because queries
**	of this nature can contain large numbers of such OR/AND combinations
**	there is potential to encounter numerous Ingres internal limitations
**	(memory, Boolean factors) during the transform. 
**	
**	The characteristic query of this class has a where clause which looks
**	like:
**	... where col1 = val1 and col2 = val2 and ... and (coli = vi1 and
**	  colj = vj1 and ... or coli = vi2 and colj = vj2 and ... or ...)
**	The col1 and col2 comparisons need not be there (i.e. the coli, colj,
**	... subexpression may be all that is in the where clause), but the 
**	key component is a series of ORs, in which is nested a set of ANDed 
**	"=" comparisons with the exact same set of columns with various 
**	constant variables. 
**
**	These functions attempt to recognize such patterns. The constant sets
**	for each "AND set" are assembled into rows in an internally generated
**	temporary table and the where clause is re-written to replace the
**	entire OR substructure with a single set of ANDed "=" predicates 
**	between the original column set and the new temporary table "columns". 
**	The example above would transform to:
**	... where col1 = val1 and col2 = val2 and ... and coli = ttab.coli
**	and colj = ttab.colj and ...
**
**	This will eliminate a significant portion of where clause which would
**	otherwise be subject to a potentially problematic CNF conversion.
**
** Inputs: subquery 	- pointer to OPF subquery state variable
**	opj_orstatp	- ptr to OR transformation state variable
**	node		- ptr to current where clause node ptr
**	success		- ptr to switch indicating successful transform
**
** Outputs:
**      None
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      4-feb-97 (inkdo01)
**          written
[@history_template@]...
*/
static VOID 
opj_topscan(
	OPS_SUBQUERY	*subquery,
	OPJ_ORSTAT	*opj_orstatp,
	PST_QNODE	**node,
	bool		*success)

{
bool		failure;

    /* This function examines the top level of the qualification tree. It 
    ** recursively descends its way through ANDs, in its search for ORs. 
    ** The first OR results in a call to the OR analysis function. If the
    ** first set of ORs doesn't yield a potential transformation, it 
    ** continues examining the nested ANDs for another possibly useful
    ** OR structure. */

    if (subquery->ops_global->ops_rangetab.opv_gv >= OPV_MAXVAR) return;
				/* can't even start if there's no room 
				** for another table */

    /* Nothing more than a switch, to look at the various possibilities. */

    if (*node) switch ((*node)->pst_sym.pst_type) {

     case PST_AND:

	if ((*node)->pst_sym.pst_value.pst_s_op.pst_joinid != PST_NOJOIN)
		return;		/* no ON clauses for now */
	opj_topscan(subquery, opj_orstatp, &(*node)->pst_left, success);
				/* recurse down left ... */
	if (*success) return;
	opj_topscan(subquery, opj_orstatp, &(*node)->pst_right, success);
				/* then recurse down right */
	return;

     case PST_OR:

	if ((*node)->pst_sym.pst_value.pst_s_op.pst_joinid != PST_NOJOIN)
		return;		/* no ON clauses for now */
	/* Init stuff in the status structure, then call OR analyzer. */
	opj_orstatp->first = TRUE;
	opj_orstatp->array_size = 0;
	opj_orstatp->row_count  = 0;
	MEfill(sizeof(opj_orstatp->alldups), (u_char)255,
	    (PTR)&opj_orstatp->alldups);
	failure = FALSE;	/* this controls orscan flow */

	opj_orscan(subquery, opj_orstatp, node, success, &failure);

	if (*success)
	 opj_orreplace(subquery, opj_orstatp, node);
				/* if we got one, transform is done here */
	return;

     case PST_BOP:		/* BOP ought to be the only other possibility */
     case PST_UOP:		/* but we'll trap the lot, anyway */
     default:
	return;
    }		/* end of node type switch */

}	/* end of opj_topscan */

/*{
** Name: OPJ_ORSCAN 	- analyze OR structure in where clause, looking for 
**	transformable pattern (part of the MS Access where clause transform).
**
** Description:
**      This function recursively descends OR nodes in the where clause, looking
**	for ANDs to further analyze. Anything else results in failure down this
**	branch of the expression tree.
**
** Inputs: subquery 	- pointer to OPF subquery state variable
**	opj_orstatp	- ptr to OR transformation state variable
**	node		- ptr to current OR node ptr
**	success		- ptr to switch indicating successful transform
**	failure		- ptr to switch indicating failure in current OR set
**
** Outputs:
**      None
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      4-feb-97 (inkdo01)
**          written
**	14-mar-97 (inkdo01)
**	    Added support for the Focus in-list extension to the MS access 
**	    transform
[@history_template@]...
*/
static VOID 
opj_orscan(
	OPS_SUBQUERY	*subquery,
	OPJ_ORSTAT	*opj_orstatp,
	PST_QNODE	**node,
	bool		*success,
	bool		*failure)

{
    i4		count, i;


    /* Recursively descend ORs. ANDs trigger call to next level function
    ** (we're getting close to the action). Anything else means this branch
    ** of the where doesn't qualify. */

    /* Do the big switch. */

    if (*node) switch ((*node)->pst_sym.pst_type) {

     case PST_OR:
	opj_orscan(subquery, opj_orstatp, &(*node)->pst_left, success, failure);
				/* recurse down the left ... */
	if (*failure) 
	{
	    *success = FALSE;			/* don't deceive caller */
	    return;
	}
	opj_orscan(subquery, opj_orstatp, &(*node)->pst_right, success, failure);
	if (!(*failure)) *success = TRUE;	/* so far, so good */
	 else *success = FALSE;
	return;

     case PST_AND:
	count = 0;		/* init a bit */
	MEfill(sizeof(opj_orstatp->var_map), (u_char)0, 
	    (PTR)&opj_orstatp->var_map);
	opj_orstatp->first_and = *node;
	opj_orstatp->row_count++;	/* another "row" */
	opj_orstatp->in_var = NULL;
	opj_orstatp->in_count = 0;

	opj_andscan(subquery, opj_orstatp, node, failure, &count);

	if (!(*failure) && opj_orstatp->array_size == count)
	{
	    /* this set of ANDs seems to be consistent with previous (or 
	    ** it's the first) */
	    for (i = 1; i < count; i++)
		opj_orstatp->const_array[i-1]->pst_right =
		    opj_orstatp->const_array[i];
						/* chain "row" together */
	    opj_orstatp->first = FALSE;		/* reset for the rest */
	    if (opj_orstatp->in_count) opj_inreplace(subquery, opj_orstatp);
						/* if AND-set had in-list, 
						** replicate AND-set */
	    return;
	}

	/* else, drop to failure */

     default:
	if (!(*failure))			/* if not yet failed */
	    opj_scanfail(subquery, opj_orstatp);    /* clean up */
	*failure = TRUE;
	return;
    }		/* end of switch */

}	/* end of opj_orscan */

/*{
** Name: OPJ_ANDSCAN	- analyze AND structure under OR in where clause, looking
**	for transformable pattern (part of the MS Access where clause transform).
**
** Description:
**      This function recursively descends AND nodes under OR nodes in the where
**	clause. Only ANDs and BOPs are of interest to it - anything else results
**	in failure of this branch of the tree. BOPs are further analyzed.
**	Changes have now been made to accept ORs, as well, in the event that
**	they may be the result of an expanded IN-list predicate. Focus can
**	sometimes stick these into an AND-set as an apparent "optimization".
**
** Inputs: subquery 	- pointer to OPF subquery state variable
**	opj_orstatp	- ptr to OR transformation state variable
**	node		- ptr to current AND node ptr
**	failure		- ptr to switch indicating failure in this OR set
**	count		- ptr to running count of BOPs in this AND set
**
** Outputs:
**      None
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      4-feb-97 (inkdo01)
**          written
**	14-mar-97 (inkdo01)
**	    Added support for the Focus in-list extension to the MS access 
**	    transform
[@history_template@]...
*/
static VOID 
opj_andscan(
	OPS_SUBQUERY	*subquery,
	OPJ_ORSTAT	*opj_orstatp,
	PST_QNODE	**node,
	bool		*failure,
	int		*count)

{

    /* This function recursively descends AND structures looking for BOPs.
    ** More specifically, it looks for BOPs which are "=" operations between 
    ** a PST_VAR and a PST_CONST (order is immaterial). These are passed to
    ** the next (and last!) routine for further analysis. Anything else
    ** triggers failure in this branch of the where. */

    /* Big switch on node type. */

    if (*node) switch ((*node)->pst_sym.pst_type) {

     case PST_AND:
	opj_andscan(subquery, opj_orstatp, &(*node)->pst_left, failure, count);
				/* recurse down the left ... */
	if (*failure) return;
	opj_andscan(subquery, opj_orstatp, &(*node)->pst_right, failure, count);
				/* then recurse down the right */
	return;

     case PST_OR:	/* this is only acceptable if it is part of an in-list */
	if (opj_orstatp->in_var ||
		!opj_or2scan(subquery, opj_orstatp, node, count))
	{
	    *failure = TRUE;
	    opj_scanfail(subquery, opj_orstatp);
	}
	return;

     case PST_BOP:
	/* Check for VAR = CONST or CONST = VAR, then analyze further. */
	if ((*node)->pst_sym.pst_value.pst_s_op.pst_opno == ADI_EQ_OP &&
	    ((*node)->pst_left->pst_sym.pst_type == PST_VAR &&
	     (*node)->pst_right->pst_sym.pst_type == PST_CONST ||
	     (*node)->pst_left->pst_sym.pst_type == PST_CONST &&
	     (*node)->pst_right->pst_sym.pst_type == PST_VAR) &&
	    opj_bopscan(subquery, opj_orstatp, node, count))
	  return;		/* if (all that stuff up there) */

	/* else, fall through to failure */

     default:		/* everything else, including OR at this level
			** (nested under an AND nested under another OR) */
	opj_scanfail(subquery, opj_orstatp);
	*failure = TRUE;		/* tell our stackmates */
	return;
    }		/* end of switch */	

}	/* end of opj_andscan */

/*{
** Name: OPJ_OR2SCAN	- process sequence of ORs (likely generated by "in-list"
**	in an AND-set (Focus extension).
**
** Description:
**      This function analyzes a series of OR'ed "=" predicates inside an
**	AND-set being processed for the MS Access transform. This might be an
**	in-list generated by Focus. The OR'ed predicates must all be "="
**	preds between the same VAR and various CONSTs. The VAR must be one of
**	the VARs in the high level OR-set. Given the presence of an acceptable
**	in-list, the other "=" predicates in the current AND-set will eventually
**	be replicated as many times as there are entries in the in-list, with 
**	one of the in-list predicates being added to each replication. This 
**	effectively turns: col1 = val1 and col2 in (val2, val3, ...) into:
**	col1 = val1 and col2 = val2 or col1 = val1 and col2 = val3 or ...
**	which is an acceptable pattern for the MS Access transformation.
**
** Inputs: subquery 	- pointer to OPF subquery state variable
**	opj_orstatp	- ptr to OR transformation state variable
**	node		- ptr to current BOP node ptr
**	count		- ptr to running count of BOPs in this AND set
**
** Outputs:
**      None
**	Returns:
**	    TRUE	- if subtree anchored in node satisfies in-list
**			  constraints
**	    FALSE	- otherwise
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-mar-97 (inkdo01)
**          written
[@history_template@]...
*/
static bool
opj_or2scan(
	OPS_SUBQUERY	*subquery,
	OPJ_ORSTAT	*opj_orstatp,
	PST_QNODE	**node,
	int		*count)

{
    PST_QNODE	*var, *cnst;

    /* This function just switches on node type, recursing, rejecting or
    ** precessing, depending on what is found. */

    switch ((*node)->pst_sym.pst_type) {

     case PST_OR:	/* just recurse down the in-list */
	if (!opj_or2scan(subquery, opj_orstatp, &(*node)->pst_left, count))
	 return(FALSE);
	return(opj_or2scan(subquery, opj_orstatp, &(*node)->pst_right, count));

     case PST_BOP:	/* this is the interesting case */
	/* Some simple checks to see if we're even in the game. */
	if ((*node)->pst_sym.pst_value.pst_s_op.pst_opno != ADI_EQ_OP ||
	    !((*node)->pst_left->pst_sym.pst_type == PST_VAR &&
	      (*node)->pst_right->pst_sym.pst_type == PST_CONST ||
	      (*node)->pst_left->pst_sym.pst_type == PST_CONST &&
	      (*node)->pst_right->pst_sym.pst_type == PST_VAR))
	 return(FALSE);		/* must be "=" of VAR and CONST */

	/* Next, lets find the VAR and the CONST. */
	if ((*node)->pst_left->pst_sym.pst_type == PST_VAR)
	{		/* var = cnst */
	    var = (*node)->pst_left;
	    cnst = (*node)->pst_right;
	}
	else
	{		/* cnst = var */
	    var = (*node)->pst_right;
	    cnst = (*node)->pst_left;
	}

	/* Slightly closer scrutinizing of the CONST node. */
	if (!(cnst->pst_sym.pst_dataval.db_data) ||
		cnst->pst_sym.pst_value.pst_s_cnst.pst_tparmtype !=
		PST_USER) return(FALSE);
				/* must have value and be PST_USER */

	/* If first BOP in in-list, BOPSCAN does some shortcutting. */
	if (opj_orstatp->in_count++ == 0)
	{
	    if (!opj_bopscan(subquery, opj_orstatp, node, count))
		return(FALSE);
	    opj_orstatp->in_var = &var->pst_sym.pst_value.pst_s_var;
	    /* Determine IN-list VAR's column in const_array. */
	    if (opj_orstatp->first) 
		opj_orstatp->in_col = opj_orstatp->array_size-1;
	    else
	    {
		PST_QNODE	*var1;
		i4		i;
		/* Find our VAR in the var_array. */
		for (i = 0; i < opj_orstatp->array_size; i++)
	 	 if ((var1 = opj_orstatp->var_array[i])->
				pst_sym.pst_value.pst_s_var.pst_vno == 
			var->pst_sym.pst_value.pst_s_var.pst_vno &&
	      		var1->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id ==
			var->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
		break;
		opj_orstatp->in_col = i;
	    }
	    return(TRUE);
	}

	/* Check that this VAR is same as others in "in-list" and that 
	** constant types match, then add CONST */
	if (opj_orstatp->in_count >= OPV_MAXVAR) return(FALSE);
	if (var->pst_sym.pst_value.pst_s_var.pst_vno !=
		opj_orstatp->in_var->pst_vno ||
	    var->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id !=
		opj_orstatp->in_var->pst_atno.db_att_id ||
	    opj_orstatp->const_array[opj_orstatp->in_col]->
						pst_sym.pst_dataval.db_datatype
		!= cnst->pst_sym.pst_dataval.db_datatype) return(FALSE);
					/* must be same data type */
	opj_orstatp->in_array[opj_orstatp->in_count-1] = cnst;
	if (opj_orstatp->const_size[opj_orstatp->in_col] < 
			cnst->pst_sym.pst_dataval.db_length)
		opj_orstatp->const_size[opj_orstatp->in_col] = 
				cnst->pst_sym.pst_dataval.db_length;
					/* maintain max length per "column" */
	return(TRUE);

     default:		/* all else is verboten! */
	return(FALSE);
    }		/* end of switch */

}	/* end of opj_or2scan */

/*{
** Name: OPJ_BOPSCAN	- scrutinize the "=" operation to assure we still fit
**	the pattern for doing the MS Access transformation. 
**
** Description:
**      This function analyzes a "=" operation to assure that it is consistent 
**	with the current set of ANDs under ORs being considered for transform. 
**	The VAR must appear exactly once in the current AND set, and must match
**	a VAR in the original AND set in the current OR set.
**
** Inputs: subquery 	- pointer to OPF subquery state variable
**	opj_orstatp	- ptr to OR transformation state variable
**	node		- ptr to current BOP node ptr
**	count		- ptr to running count of BOPs in this AND setorm
**
** Outputs:
**      None
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      4-feb-97 (inkdo01)
**          written
**      13-nov-03 (wanfr01)
**          INGSRV 2532, Bug 110981
**          The Access transformation doesn't work for tid columns. 
**      15-jun-2006 (huazh01)
**          the fix to b36414 packs constants in IN clause into a
**          linked list under one PST_CONST node. opj_bopscan() add the 
**          constant node into const_array[], but if opj_scanfail() is 
**          called, re-NULL the left chain for constants nodes in 
**          const_array[] results in losing constants specified 
**          in the IN clause, and eventually causes wrong result. 
**          We now clone the PST_CONST tree into const_clone_array[]
**          and use this array to do the 'clean up' work in 
**          opj_scanfail(). b116108.
**      15-jul-2009 (huazh01)
**          put constants in IN-list into OPJ_ORSTAT's in_array[]. 
**          b122313. 
**      17-aug-2010 (huazh01) 
**          stop copying the query tree into in_array[]. Let
**          in_array[] and const_array[] references the same query 
**          tree. (b124212)
[@history_template@]...
*/
static bool
opj_bopscan(
	OPS_SUBQUERY	*subquery,
	OPJ_ORSTAT	*opj_orstatp,
	PST_QNODE	**node,
	int		*count)

{
    PST_QNODE	*var, *cnst, *dup, *ptr;
    int		i;

    /* First we perform some simple tests (hard to imagine a query with 
    ** > 32 BOPs in an AND set!). */

    (*count)++;		/* increment count in current AND set */
    if (opj_orstatp->first != TRUE && *count > opj_orstatp->array_size
	    || *count > OPV_MAXVAR) return(FALSE);

    /* Next, lets find the VAR and the CONST. */
    if ((*node)->pst_left->pst_sym.pst_type == PST_VAR)
    {		/* var = const */
	var = (*node)->pst_left;
	cnst = (*node)->pst_right;
    }
     else
    {		/* const = var */
	var = (*node)->pst_right;
	cnst = (*node)->pst_left;
    }


    /* The MS Access transformation does not work for tid columns */
    if (var->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id == DB_IMTID) 
    {
        return FALSE;
    }

    /* Slightly closer scrutinizing of the CONST node. */
    if (!(cnst->pst_sym.pst_dataval.db_data) ||
	cnst->pst_sym.pst_value.pst_s_cnst.pst_tparmtype != PST_USER) 
	    return(FALSE);	 /* must have value and be PST_USER */

    /* If we're working on the first AND set in this OR set, life is
    ** a bit easier. */
    if (opj_orstatp->first)
    {
	/* See if for some strange reason, the VAR is already here (strange,
	** because it means a where with "a = x ... and a = y" in same OR) */
	for (i = 0; i < opj_orstatp->array_size; i++)
	 if (opj_orstatp->var_array[i]->pst_sym.pst_value.pst_s_var.pst_vno
		== var->pst_sym.pst_value.pst_s_var.pst_vno && 
	     opj_orstatp->var_array[i]->pst_sym.pst_value.pst_s_var.pst_atno.
								db_att_id
		== var->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
	  return(FALSE);

	opj_orstatp->var_array[(*count)-1] = var;  /* stuff nodes in arrays */
	opj_orstatp->const_array[(*count)-1] = cnst;
	opj_orstatp->bop_array[(*count)-1] = (*node);
	opj_orstatp->const_size[(*count)-1] = 
			cnst->pst_sym.pst_dataval.db_length;
	opj_orstatp->array_size++;		/* increment count */

        /* b116108, duplicate the constant tree */
        dup = cnst; 
        opv_copytree(subquery->ops_global, &dup);
        opj_orstatp->const_clone_array[(*count)-1] = dup;

        /* b122313: put constants in IN-list into in_array[] */
        if (cnst->pst_left &&
            cnst->pst_left->pst_sym.pst_type == PST_CONST)
        {
            dup = cnst->pst_left; 

            if (!opj_orstatp->in_count) 
               opj_orstatp->in_count++; 

            opj_orstatp->in_var = &var->pst_sym.pst_value.pst_s_var;
            opj_orstatp->in_col = opj_orstatp->array_size - 1; 
            opj_orstatp->in_array[opj_orstatp->in_count] = dup;
            opj_orstatp->in_count++;
        }

	(*node)->pst_left = var;		/* this harmless (harmless, */
	(*node)->pst_right = cnst;		/* since op is "=") change  
						** eases life later on */
	return(TRUE);
    }

    /* For subsequent AND sets, it's a bit more difficult. VAR must already
    ** be present from first AND set, though again it can only be here once. 
    ** CONST is then hooked into the chain anchored in the array. CONSTs 
    ** are linked together using their pst_left pointers. Since CONST nodes
    ** are ALWAYS leaf nodes in parse trees, pst_left is ALWAYS available
    ** for this purpose. */
    {
	PST_QNODE	*var1;

	/* Find our VAR in the var_array. */
	for (i = 0; i < opj_orstatp->array_size; i++)
	 if ((var1 = opj_orstatp->var_array[i])->pst_sym.pst_value.pst_s_var.
		pst_vno == var->pst_sym.pst_value.pst_s_var.pst_vno &&
	      var1->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id ==
		var->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
	{	/* found a matching VAR */
	    /* Was VAR already found in this AND set? */
	    if (BTtest((i4)i, (char *)&opj_orstatp->var_map))
		return(FALSE);		/* later, check CONST type match, too */
	    BTset((i4)i, (char *)&opj_orstatp->var_map);
					/* show VAR is in this AND set */
	    if (opj_orstatp->const_array[i]->pst_sym.pst_dataval.db_datatype
		!= cnst->pst_sym.pst_dataval.db_datatype) return(FALSE);
					/* must be same data type */
	    if (MEcmp((PTR)cnst->pst_sym.pst_dataval.db_data, 
		(PTR)opj_orstatp->const_array[i]->pst_sym.pst_dataval.db_data,
		cnst->pst_sym.pst_dataval.db_length) != 0)
		  BTclear((i4)i, (PTR)&opj_orstatp->alldups);
					/* show not all values are the same */

            /* b122313: put constants in IN-list into in_array[] */
            if (cnst->pst_left &&
                cnst->pst_left->pst_sym.pst_type == PST_CONST)
            {
                dup = cnst->pst_left;

                if (!opj_orstatp->in_count)
                   opj_orstatp->in_count++;

                opj_orstatp->in_var = &var1->pst_sym.pst_value.pst_s_var;
                opj_orstatp->in_col = i;
                opj_orstatp->in_array[opj_orstatp->in_count] = dup;
                opj_orstatp->in_count++;
            }

            /* loop to the end of constant chain, then link it
            ** with const_array[i].
            */
            ptr = cnst; 
            while (ptr && 
                   ptr->pst_sym.pst_type == PST_CONST &&
                   ptr->pst_left)
                  ptr = ptr->pst_left; 

	    ptr->pst_left = opj_orstatp->const_array[i];

	    opj_orstatp->const_array[i] = cnst;
					/* chain new CONST to head of list */
	    if (opj_orstatp->const_size[i] < 
				cnst->pst_sym.pst_dataval.db_length)
		opj_orstatp->const_size[i] = 
				cnst->pst_sym.pst_dataval.db_length;
					/* maintain max length per "column" */
	    return(TRUE);		/* return the good news */
	}

	/* Dropping off the for-loop means we didn't find a VAR match. */
	return(FALSE);

    }

}	/* end of opj_bopscan */

/*{
** Name: OPJ_SCANFAIL	- this routine cleans up the where clause and the 
**	status structure when we fail in the midst of an analysis.
**
** Description:
**      This function just re-NULLs the pst_left chain of all CONSTs
**	anchored in the const_array. Nothing else is needed, since the
**	status structure is otherwise initialized elsewhere.
**
** Inputs: subquery 	- pointer to OPF subquery state variable
**	opj_orstatp	- ptr to OR transformation state variable
**
** Outputs:
**      None
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      4-feb-97 (inkdo01)
**          written
**      15-jun-2006 (huazh01)
**          instead of re-null the left/right chain of the const node, 
**          we now just restore it using nodes saved in const_clone_array[]. 
**          b116108. 
**          
[@history_template@]...
*/
static VOID
opj_scanfail(
	OPS_SUBQUERY	*subquery,
	OPJ_ORSTAT	*opj_orstatp)

{
    int		i;

    for (i = 0; i < opj_orstatp->array_size; i++)
    {
        opj_orstatp->const_array[i] = 
               opj_orstatp->const_clone_array[i]; 
    }
    return; 

}	/* end of opj_scanfail */

/*{
** Name: OPJ_INREPLACE	- merge IN-list with current AND-set, according to 
**	Focus extension to MS Access transformation.
**
** Description:
**      This function updates the opj_orstat structure by merging the current 
**	AND-set with IN-list values encountered during its processing. This
**	is for AND-sets like: col1 = val1 and col2 in (val2, val3, ...) and
**	coln = valn. This is equivalent to: col1 = val1 and col2 = val2 and
**	coln = valn or col1 = val1 and col2 = val3 and coln = valn or ...
**	The purpose of this routine is to add the IN-list constants to the
**	appropriate const_array list and to replicate the 1st entry in all
**	other const_array lists (i.e., the values from the current AND-set)
**	as many times as there are entries in the IN-list. Then hook them
**	all back together.
**
** Inputs: subquery 	- pointer to OPF subquery state variable
**	opj_orstatp	- ptr to OR transformation state variable
**
** Outputs:
**      None
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-mar-97 (inkdo01)
**          written
**	22-may-97 (inkdo01)
**	    Minor glitch allowed bogus "row" into constant matrix.
**	23-jun-97 (inkdo01)
**	    Clear dups flag for in-list cols, too.
**      15-jul-2009 (huazh01)
**          opj_orstatp->in_array[] could have a linked list of 
**          constants now, scan them and put them into constant 
**          array. (b122313)
**      17-aug-2009 (huazh01)
**          in_array[] and const_array[] now references to the same 
**          query tree. This means we don't need to add PST_QNODEs in
**          in_array[] into const_array[]. For unprocessed nodes in 
**          in_array[], whose pst_right is NULL, we just need to set up 
**          the link for its adjacent column. (b124212)
**          
[@history_template@]...
*/
static VOID
opj_inreplace(
	OPS_SUBQUERY	*subquery,
	OPJ_ORSTAT	*opj_orstatp)

{
    PST_QNODE	*cnst, *this, *next;
    i4		invar, i, j;

    /* This function consists of a pair of nested loops. The outer loops
    ** over the IN-list constants, and the inner loops over the lists in
    ** the const_array. */

    invar = opj_orstatp->in_col;		/* for ease of reference */

    for (i = 1; i < opj_orstatp->in_count; i++)
    {
      this = opj_orstatp->in_array[i];

      while (this != (PST_QNODE *)NULL && 
             this->pst_sym.pst_type == PST_CONST &&
             this->pst_right == (PST_QNODE*)NULL)
      {
	/* Insert current IN-list constant into corresponding list in
	** const_array. */
	if (MEcmp((PTR)opj_orstatp->const_array[invar]->pst_sym.pst_dataval.
		db_data, (PTR)this->pst_sym.pst_dataval.db_data, 
                this->pst_sym.pst_dataval.db_length) != 0) 
	 BTclear((i4)invar, (PTR)&opj_orstatp->alldups);
					/* first reset dups flag, if ok */

        next = this->pst_left; 
	opj_orstatp->row_count++;

	for (j = 0; j < opj_orstatp->array_size; j++)
	{
	    /* Short-circuit if this is IN-list's column. */
	    if (j == invar)
	    {
                cnst = opj_orstatp->const_array[invar]->pst_right;
                this->pst_right = cnst; 

		if (j > 0)
		 opj_orstatp->const_array[j-1]->pst_right = this;
					/* link 'em sideways, but only
					** if IN-list col isn't first */
		continue;
	    }
	    /* Replicate current constant & do the linkages. */
	    cnst = opj_orstatp->const_array[j];
	    opv_copynode(subquery->ops_global, &cnst);
	    cnst->pst_left = opj_orstatp->const_array[j];
	    opj_orstatp->const_array[j] = cnst;
					/* link 'em up/down */
	    if (j > 0)
		opj_orstatp->const_array[j-1]->pst_right = cnst;
					/* link 'em sideways */
	    cnst->pst_right = NULL;	/* be tidy! */
	}	/* end of inner loop */

        this = next; 

      }         /* end of while loop */

    }		/* end of outer loop */

}	/* end of opj_inreplace */

/*{
** Name: OPJ_ORREPLACE	- This routine performs the transformation of an
**	eligible OR substructure in a where clause into an equivalent join
**	to the internally generated temporary table. This is the final step
**	in transforming where clauses which follow the MS Access pattern.
**
** Description:
**	Several steps are required to complete this transformation:
**	  1. search for VARs which are compared to the same value in all
**	 AND sets (i.e., comparisons which are common factors). One of the
**	 corresponding BOPs is elevated above the OR structure, making it
**	 a distinct Boolean factor in the original expression. 
**	  2. use the CONSTs of the remaining VARs to build the temporary
**	 table layout. A RDR_INFO structure to describe the temp table is 
**	 constructed, along with range table structures to make the temp 
**	 table appear as if it were in the original query.
**	  3. modify the expression tree so that the entire OR substructure 
**	 is replaced by one instance of a contained AND set in which the
**	 PST_CONSTs are replaced by PST_VARs representing the temp table 
**	 columns. This effectively transforms the sets of "col = constant"
**	 predicates into join predicates with the temp table constructed from 
**	 the constants.
**	  
**	The eventual effect of all this is to turn a where which looks like:
**	"... where col1 = val1 and (col2 = v21 and col3 = v31 and ...
**	   or col2 = v22 and col3 = v32 and ...
**	   or col2 = v23 and col3 = v33 and ...", into one which looks like:
**	"... where col1 = val1 and col2 = temptab.col2 
**	   and col3 = temptab.col3 ...". 
**
** Inputs: subquery 	- pointer to OPF subquery state variable
**	opj_orstatp	- ptr to OR transformation state variable
**	node		- ptr to ptr to root of OR structure to be transformed
**
** Outputs:
**      None
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      5-feb-97 (inkdo01)
**          written
[@history_template@]...
*/
static VOID
opj_orreplace(
	OPS_SUBQUERY	*subquery,
	OPJ_ORSTAT	*opj_orstatp,
	PST_QNODE	**node)

{
    PST_QNODE	*andnode, *varnode, *bopnode, *constnode;
    i4		i, j, ttabcols, ttabrowsize, attoff;
    OPV_IGVARS	grvno;
    DB_ATT_ID	attno = {0};
    OPS_STATE	*global = subquery->ops_global;

    RDR_INFO	*rdr;
    PST_RNGENTRY **rngp;
    char	*nextname;
    i4		dmt_mem_size;
    DMT_ATT_ENTRY	*att_array;
    DMT_ATT_ENTRY	*attrp;
    i4			n;
    i4			attr_nametot;

 
    /* Start by checking for VARs which compared to same constant in
    ** every AND set. These are common factors which can be extracted 
    ** outside the OR structure without being included in the temporary
    ** table. If all VARs are like this, there doesn't even need to be
    ** a temp table! */

    /* At very first, though, replace whole OR substructure with QLE. 
    ** subsequent splicing operations leaves it at bottom of replacement
    ** subtree. */

    (*node) = opv_qlend(global); 
    ttabcols = ttabrowsize = 0;
    j = 0;

    for (i = 0; i < opj_orstatp->array_size; i++)
     if (BTtest((i4)i, (char *)&opj_orstatp->alldups))
     {		/* process a common factor */
	andnode = opv_opnode(global, PST_AND, (ADI_OP_ID)0, PST_NOJOIN);
				/* make a new AND */
	bopnode = opj_orstatp->bop_array[i];
	andnode->pst_left = bopnode;
				/* hang common factor BOP under new AND */
	andnode->pst_right = (*node);
	(*node) = andnode;	/* splice AND into modified OR structure */
	bopnode->pst_right->pst_left = NULL;
				/* clean up CONST linkage */
     }
     else
     {		/* future temp table column */
	PST_QNODE	*const1, *const2;

	/* First see if status vectors need to be shifted becuase of 
	** preceding "alldups" */
	if (i != j)
	{
	    opj_orstatp->var_array[j] = opj_orstatp->var_array[i];
	    opj_orstatp->const_array[j] = opj_orstatp->const_array[i];
	    opj_orstatp->bop_array[j] = opj_orstatp->bop_array[i];
	    opj_orstatp->const_size[j] = opj_orstatp->const_size[i];
	}

	/* If immediately preceding was alldups, must re-link const's 
	** pst_right's. */
	if (j >= 1 && BTtest((i4)(i-1), (char *)&opj_orstatp->alldups))
	 for (const1 = opj_orstatp->const_array[j-1], 
		const2 = opj_orstatp->const_array[j]; const1;
		const1 = const1->pst_left, const2 = const2->pst_left)
					/* loop down "rows" of c1, c2 */
	    const1->pst_right = const2;

	ttabcols++;
	ttabrowsize += opj_orstatp->const_size[j];
	j++;
     }

    if (ttabcols == 0) return;		/* all VARs were common factors */

    if (BTtest((i4)(i-1), (char *)&opj_orstatp->alldups))
     for (constnode = opj_orstatp->const_array[ttabcols-1]; constnode; 
	constnode = constnode->pst_left)
     constnode->pst_right = NULL;   /* clear right link for last col if
				** last VAR was a common factor */

    /* Compute blank stripped length of attribute names */
    for (i = 0, attr_nametot = 0; i < ttabcols; i++)
    {
	varnode = opj_orstatp->var_array[i];

	/* Compute blank stripped length of attribute names */
	n = cui_trmwhite(DB_ATT_MAXNAME,
		(char *)&varnode->pst_sym.pst_value.pst_s_var.pst_atname); 
	attr_nametot += (n+1);
    }

    /* Create new range table entry to contain temporary table definition. */
    grvno = global->ops_rangetab.opv_gv;	/* grab next number */
    (VOID)opv_parser(global, grvno, OPS_MAIN, FALSE, FALSE, FALSE);
				/* create global rt entry */
    rdr = (RDR_INFO *) opu_memory(global, sizeof(RDR_INFO));
				/* allocate/format RDF stuff */
    MEfill(sizeof(RDR_INFO), (u_char)0, (char *)rdr);
    rdr->rdr_rel = (DMT_TBL_ENTRY *) opu_memory(global, 
		sizeof(DMT_TBL_ENTRY));
    MEfill(sizeof(DMT_TBL_ENTRY), (u_char)0, (char *)rdr->rdr_rel);

    dmt_mem_size = ((ttabcols+1) * sizeof(DMT_ATT_ENTRY *))
	+ ((ttabcols+1) * sizeof(DMT_ATT_ENTRY))
	+ attr_nametot;

    rdr->rdr_attr = (DMT_ATT_ENTRY **) opu_memory(global, dmt_mem_size);
    rdr->rdr_attr[0] = NULL;	/* first is empty (for TID) */
    att_array = (DMT_ATT_ENTRY *)((char *)rdr->rdr_attr + 
		    ((ttabcols+1) * sizeof(DMT_ATT_ENTRY *)));
    nextname = (char *)att_array + ((ttabcols+1) * sizeof(DMT_ATT_ENTRY));

    global->ops_rangetab.opv_base->opv_grv[grvno]->opv_relation = rdr;
    global->ops_rangetab.opv_base->opv_grv[grvno]->opv_gmask 
		|= OPV_MCONSTTAB;  /* indicate wacky temptab */
    MEcopy("$memory_res_table ", 18, (char *)&rdr->rdr_rel->tbl_name); 
    MEcopy("$internal ", 10, (char *)&rdr->rdr_rel->tbl_owner);  
    rdr->rdr_no_attr = ttabcols;
    rdr->rdr_rel->tbl_attr_nametot = attr_nametot;
    rdr->rdr_rel->tbl_attr_count = ttabcols;
    rdr->rdr_rel->tbl_width = ttabrowsize;
    rdr->rdr_rel->tbl_storage_type = DMT_HEAP_TYPE;
    rdr->rdr_rel->tbl_status_mask = DMT_RD_ONLY;
    rdr->rdr_rel->tbl_record_count = opj_orstatp->row_count;
    rdr->rdr_rel->tbl_pgsize = 2048;
    rdr->rdr_rel->tbl_temporary = 1;
    
    BTset((i4)grvno, 
	(char *)&subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm);
				/* show temptab reference in parse tree.
				** This triggers local rt entry creation */

    /* Create a new parse tree range table entry, too */
    rngp = (PST_RNGENTRY **) opu_memory(global, 
	(grvno+1)*sizeof(PST_RNGENTRY *));
    MEcopy((PTR)global->ops_qheader->pst_rangetab, 
	(grvno)*sizeof(PST_RNGENTRY *), (PTR)rngp);
				/* copy old rangetab ptr array */
    rngp[grvno] = (PST_RNGENTRY *) opu_memory(global, sizeof(PST_RNGENTRY));
    MEfill(sizeof(PST_RNGENTRY), (u_char)0, (PTR)rngp[grvno]);
    rngp[grvno]->pst_rgtype = PST_TABLE;
    rngp[grvno]->pst_rngvar.db_tab_base = -1;
    rngp[grvno]->pst_rngvar.db_tab_index = -1;
    MEcopy((PTR)&rdr->rdr_rel->tbl_name, sizeof(DB_TAB_NAME),
	(PTR)&rngp[grvno]->pst_corr_name);
    global->ops_qheader->pst_rangetab = rngp;
				/* replace original */

    /* Build vector of "row" addresses, and anchor it in OPV_GRV */

    global->ops_rangetab.opv_base->opv_grv[grvno]->opv_orrows =
	(PST_QNODE **) opu_memory(global, opj_orstatp->row_count *
		sizeof(PST_QNODE *));
    for (constnode = opj_orstatp->const_array[0], i = 0; constnode; 
		constnode = constnode->pst_left)
	global->ops_rangetab.opv_base->opv_grv[grvno]->opv_orrows[i++]
	    = constnode;	/* fill the "row" ptr array */

    /* Sort constant rows here, to remove duplicates (to prevent equijoin
    ** semantics from screwing up results) */
    opj_sortor(subquery, global->ops_rangetab.opv_base->opv_grv[grvno]->
	opv_orrows, &rdr->rdr_rel->tbl_record_count);

    /* Loop over "columns", building both join predicates to splice into
    ** where clause, and DMT_ATT_ENTRYs to define "column". */

    attoff = 0;			/* for offset computation */
    for (i = 0, attrp = att_array; i < ttabcols; i++, attrp++)
     {		/* non-common factors are what we're after here */
	DB_DATA_VALUE	*dataval;

	/* first, fiddle the parse tree */
	attno.db_att_id++;	/* incr attno */
	andnode = opv_opnode(global, PST_AND, (ADI_OP_ID)0, PST_NOJOIN);
				/* get another AND */
	bopnode = opj_orstatp->bop_array[i];
				/* and the BOP to modify */
	varnode = opj_orstatp->var_array[i];
				/* and the VAR for later */
	dataval = &opj_orstatp->const_array[i]->pst_sym.pst_dataval;
	bopnode->pst_right = opv_varnode(global, dataval, grvno, &attno);
				/* make VAR for ttab join column */
	bopnode->pst_right->pst_sym.pst_dataval.db_length = 
		opj_orstatp->const_size[i];
				/* store the correct (max) length */
	bopnode->pst_right->pst_sym.pst_value.pst_s_var.pst_atname =
		    varnode->pst_sym.pst_value.pst_s_var.pst_atname;
	andnode->pst_left = bopnode;	/* and sew 'em all together */
	andnode->pst_right = (*node);
	(*node) = andnode;

	/* Now build the attribute descriptor */
	rdr->rdr_attr[attno.db_att_id] = attrp;
	MEfill(sizeof(DMT_ATT_ENTRY), (u_char)0, (char *)attrp);

	/* Compute blank stripped length of attribute names */
	n = cui_trmwhite(DB_ATT_MAXNAME,
		(char *)&varnode->pst_sym.pst_value.pst_s_var.pst_atname); 

	attrp->att_nmstr = nextname;
	attrp->att_nmlen = n;
	cui_move(n, varnode->pst_sym.pst_value.pst_s_var.pst_atname.db_att_name,
		    '\0', n + 1, nextname); 
	nextname += (n + 1);

	attrp->att_number = attno.db_att_id;
	attrp->att_offset = attoff;
	attrp->att_type   = dataval->db_datatype;
	attrp->att_width  = opj_orstatp->const_size[i];
	attrp->att_prec   = dataval->db_prec;
	attoff += attrp->att_width;

     }

}	/* end of opj_orreplace */

/*{
** Name: OPJ_SORTOR - This function sorts the "rows" of a MS Access OR 
**	transformed in-memory constant table. In so doing, it also drops
**	duplicate rows. This is essential to prevent join semantics from
**	possibly producing incorrect results in the transformed query.
**
** Description:
**	The function uses a plain old bubble sort to reorder the "rows",
**	comparing pairs of values in the PST_CONST nodes which define the
**	"table" at this point in time. 
**
** Inputs: subquery 	- pointer to OPF subquery state variable
**	rowarray	- ptr to array of CONST node ptrs which anchor 
**			each "row"
**	rowcount	- ptr to count of rows in table
**
** Outputs:
**      rowcount	- value is updated if any rows are dropped
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-feb-97 (inkdo01)
**          written
**	24-feb-97 (inkdo01)
**	    Minor tweak to sot out highest value, too (stopped 1 too soon).
**      05-oct-98 (johco01)
**          Modified for loop condition which is used to delete 
**          duplicate rows if they exist. Bug 92927.
**
[@history_template@]...
*/
static VOID
opj_sortor(
	OPS_SUBQUERY	*subquery,
	PST_QNODE	**rowarray,
	i4		*rowcount)

{
    OPS_STATE	*global = subquery->ops_global;
    i4		i, j;

    /* Plain old every day bubble sort. Only trick is that values to
    ** sort are embedded in PST_QNODE structures and that adjacent columns
    ** in a row are linked via their pst_right values. */

    for (i = 0; i < (*rowcount)-1; i++)
     for (j = (*rowcount)-1; j > i; j--)
     {
	PST_QNODE	*const1, *const2;
	i4		result = 0;

	/* Loop across all columns, 'til a diff is found */
	for (const1 = rowarray[j-1], const2 = rowarray[j]; 
		const1 && result == 0; const1 = const1->pst_right,
		const2 = const2->pst_right)
	 result = opu_dcompare(global, &const1->pst_sym.pst_dataval,
		&const2->pst_sym.pst_dataval);
				/* ADF performs the comparison */
	if (result == 0)
	{
	    i4	k;

	    /* If loop exits with result == 0, we have a duplicate row.
	    ** Suck all the following rows over top (just their addrs, 
	    ** actually) and decrement the row count. 
	    ** [Bug 92927] Modified loop condition, because when one
	    ** duplicate row existed loop was not entered to remove
	    ** it. */
	    for (k = j; k < (*rowcount)-1; k++)
	     rowarray[k] = rowarray[k+1];
	    (*rowcount)--;
	}
	else if (result > 0)
	{
	    /* Row[j-1] > row[j] - swap 'em, says the bubble sort ! */
	    const1 = rowarray[j-1];
	    rowarray[j-1] = rowarray[j];
	    rowarray[j] = const1;
	}
     }		/* end of nested sorting loops */

}	/* end of opj_sortor */

/*{
** Name: OPJ_TIDYUP	- clean up a where clause parse tree chunk so that each 
**	level of AND or OR nesting is in a predictable format.
**
** Description:
**	This function iterates over like conjunctions (meaning AND or OR)
**	and recurses on unlike conjunctions, reorganizing a parse tree fragment 
**	so that at each nesting level of a non-CNF transformed boolean 
**	expression, the conjunctions are strung down the pst_right side of the
**	tree, and the nesting is done on the pst_left side of the tree. This
**	makes life easier in subsequent routines.
**
** Inputs: subquery 	- pointer to OPF subquery state variable
**	nodep		- ptr to root of subtree being tidied
**
** Outputs:
**	Returns:
**			Tidied up parse tree
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      23-apr-97 (inkdo01)
**          written
[@history_template@]...
*/
static VOID
opj_tidyup( 
	OPS_SUBQUERY	*subquery,
	PST_QNODE	*nodep)

{
    PST_QNODE	*leftp;
    i4		lefttype, righttype, nodetype;

    /* This function simply iterates down the left side of the parse tree 
    ** fragment looking for node types the same as the current. In this case, 
    ** it splices the left node on top of the right node and moves the left
    ** node's old right node to the current node's left node. A picture may
    ** help:
    **
    **		OR				OR
    **	       /  \          becomes           /  \
    **	      OR  OR                          Y   OR
    **       / \  / \                            /  \
    **      X  Y P   Q                          X    OR
    **						    /  \
    **						   P    Q
    **
    ** If a different conjunction type is found, a nesting has been started
    ** and we recurse to tidy the nested expression, too. 
    */

    nodetype = nodep->pst_sym.pst_type;

    while(TRUE)		/* loop 'til we're done */
    {
	leftp = nodep->pst_left;
	if (leftp)
	 if ((lefttype = leftp->pst_sym.pst_type) == nodetype)
	{
	    PST_QNODE	*tempp;
	
	    /* Matching conjunct on left side. Here's where we swap. */
	    tempp = nodep->pst_right;
	    nodep->pst_right = leftp;
	    nodep->pst_left = leftp->pst_right;
	    leftp->pst_right = tempp;

	    continue;		/* stay in loop with "new" nodep->pst_left */
	}
	 else if (lefttype == PST_AND || lefttype == PST_OR)
		opj_tidyup(subquery, leftp);
				/* recurse for nested conjunction */

	/* Once we're here, we've tidied up the left side. Now iterate 
	** down the right side. */
	if (nodep->pst_right)
	 if ((righttype = nodep->pst_right->pst_sym.pst_type) == nodetype)
	{
	    nodep = nodep->pst_right;	
				/* if same conjunct, just stay in loop */
	    continue;
	}
	 else if (righttype == PST_OR || righttype == PST_AND)
	{
	    PST_QNODE	*tempp;
	
	    opj_tidyup(subquery, nodep->pst_right);
				/* if nesting, recurse */
	    tempp = nodep->pst_left;
	    nodep->pst_left = nodep->pst_right;
	    nodep->pst_right = tempp;
				/* when nesting, do it to left */
	}
	break;			/* however we got here, we're at the end 
				** of the line */
    }

}	/* end of opj_tidyup */

/*{
** Name: OPJ_COMPLXEST	- estimate complexity of boolean expression.
**
** Description:
**	This function recursively analyzes the boolean expression, 
**	estimating the number of boolean factors and predicate terms 
**	which will result from its conjunctive normal form transformation.
**	These estimates are used to determine whether or not to perform
**	the transformation.
**
** Inputs: subquery 	- pointer to OPF subquery state variable
**	nodep		- ptr to root of parse tree fragment currently 
**			being examined
**	terms		- ptr to running total of terms contained in 
**			transformed subtree
**	factors		- ptr to running total of factors contained in
**			transformed subtree
**	underor		- flag indicating whether current node is under
**			an OR in the parse tree fragment
**
** Outputs:
**      notcnf		- TRUE indicates that there is at least one 
**			instance of an AND nested under an OR in the 
**			boolean expression being analyzed
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-may-97 (inkdo01)
**          written
[@history_template@]...
*/
static VOID
opj_complxest(
	OPS_SUBQUERY	*subquery,
	PST_QNODE	*nodep,
	f4		*terms,
	f4		*factors,
	bool		underor,
	bool		*notcnf)

{
    f4		leftfactors, rightfactors;
    f4		leftterms, rightterms;


    /* This function is quite simple. It is a switch statement which 
    ** recursively descends AND and OR operators, then computes factors
    ** and terms based on the factor/term counts passed up from the 
    ** subtrees. It also sets the "notcnf" flag as soon as an AND is
    ** detected under an OR. */

    *factors = *terms = 0.0;
					/* init. */
    if (!nodep) return;

    switch (nodep->pst_sym.pst_type) {

     case PST_AND:
	if (underor) *notcnf = TRUE;	/* that's all there is to that! */
	opj_complxest(subquery, nodep->pst_left, &leftterms, &leftfactors, 
		underor, notcnf);	/* do left subtree */
	opj_complxest(subquery, nodep->pst_right, &rightterms, &rightfactors,
		underor, notcnf);	/* do right subtree */

	/* Magic AND formulas */
	*factors = leftfactors + rightfactors;
	*terms = leftterms + rightterms;
	return;

     case PST_OR:
	opj_complxest(subquery, nodep->pst_left, &leftterms, &leftfactors,
		TRUE, notcnf);		/* do left subtree -
					** NOTE: underor = TRUE */
	opj_complxest(subquery, nodep->pst_right, &rightterms, &rightfactors,
		TRUE, notcnf);

	/* Magic OR formulas */
	*factors = leftfactors * rightfactors;
	*terms = (leftterms/leftfactors + rightterms/rightfactors)
		* (*factors);
	return;

     case PST_QLEND:
	return;

     default:		/* simple terms */
	*factors = 1.0;
	*terms = 1.0;
	return;

    }		/* end of switch */

}	/* end of opj_complxest */

/*{
** Name: opj_normalize	- Normalize a qualification
**
** Description:
**      This function normalizes a query tree qualification, putting it in
**	conjunctive normal form.  It does this by rearranging the tree so
**	that the "not" nodes are as close to the leaves as possible, then
**	repeatedly applying the rule: A or (B and C) ==> (A or B) and (A or C)
**
** Inputs:
**      mstream                         Pointer to memory stream
**      qual                            Pointer to pointer to qualification tree
**      err_blk                         Place to put error info
**
** Outputs:
**      qual                            Rearranged into conjunctive normal form
**      err_blk                         Filled in if error happens
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
**	03-apr-86 (jeff)
**          adapted from norml function in 3.0 parser
**      03-dec-87 (seputis)
**          moved from PSF to OPF, as part of long term solution to several
**          problems
**	6-feb-97 (inkdo01)
**	    Add call to MS Access OR clause transformation logic.
**	5-june-97 (inkdo01)
**	    No Star queries allowed in nonCNF code or Access transform.
**	5-sep-97 (inkdo01)
**	    Fix noncnf test following MS Access transform attempt.
**	17-nov-97 (inkdo01)
**	    Introduce ops_cnffact as complexity evaluation parameter.
**	24-may-02 (inkdo01)
**	    Code can only support one MS Access-style transform per query. Check 
**	    for it and don't try for 2nd.
**	13-feb-03 (toumi01) bug 109609
**	    Make CNF transform decision based on new OPF_MAXBF_CNF rather
**	    than the (higher) OPB_MAXBF value. That is - we only exceed
**	    OPB_MAXBF_CNF for queries that include more boolean factors
**	    WITHOUT our doing a CNF transformation.
**	22-mar-10 (smeke01) b123457
**	    Added trace point op214 call to opu_qtprint.
[@history_template@]...
*/
VOID
opj_normalize(
	OPS_SUBQUERY	*subquery,
	PST_QNODE	**qual)
{
    OPJ_ORSTAT	opj_orstat;		/* status structure for MS Access 
					** transformation analysis */
    f4		terms, factors;
    bool	success = FALSE;
    bool	noncnf = FALSE;
    bool	notcnf = FALSE;

    if (subquery->ops_normal)
	return;				/* return if already normalized */
    if (!(*qual))
	/* create PST_QLEND node since qualification is NULL */
	*qual = opv_qlend(subquery->ops_global);
    else
    {
	/* Push "nots" as far down in tree as possible */
	/* opj_nnorm(subquery, qual); Demorgan's laws now applied in
        ** opa_generate on the initial traversal of the query tree */

	/* As part of the removal of complex conjunctive normal form 
	** transformations, evaluate the complexity of the where clause.
	** If it is exceedingly complex or if op183 is set, don't do
	** the transform. If config.dat requests the old processing, or
	** if op182 is set, do the transform. op183 overrides everything
	** (as a debugging tool). */

	opj_complxest(subquery, *qual, &terms, &factors, FALSE, &notcnf);
	if ((f4) subquery->ops_global->ops_cb->ops_alter.ops_cnffact*factors > 
		(f4) OPB_MAXBF_CNF) noncnf = TRUE;
	if (subquery->ops_global->ops_cb->ops_server->opg_smask & OPF_OLDCNF ||
		subquery->ops_global->ops_cb->ops_check &&
		opt_strace(subquery->ops_global->ops_cb, OPT_F054_OLDCNF) ||
		subquery->ops_global->ops_cb->ops_smask & OPS_MDISTRIBUTED)

	 noncnf = FALSE;
	if (subquery->ops_global->ops_cb->ops_check &&
		opt_strace(subquery->ops_global->ops_cb, OPT_F055_NOCNF))
	 noncnf = TRUE;

	if (subquery->ops_root == NULL) noncnf = FALSE;
					/* probably a rule compile - no
					** need to worry about CNF woes */
	 else if (noncnf && 
		!(subquery->ops_global->ops_cb->ops_smask & OPS_MDISTRIBUTED) &&
		!(subquery->ops_global->ops_gmask & OPS_MEMTAB_XFORM))
	{   /* Perform MS Access OR analysis/transformation */
	    opj_topscan(subquery, &opj_orstat, qual, &success);
	    if (success)
		subquery->ops_global->ops_gmask |= OPS_MEMTAB_XFORM;
					/* flag transform - we can only do
					** one per query. */

	    if (success)
	    {
		/* The MS Access transform will usually drastically reduce the 
		** complexity of the where. So check it out again. */
		opj_complxest(subquery, *qual, &terms, &factors, FALSE, 
								&notcnf);
		if (subquery->ops_global->ops_cb->ops_alter.ops_cnffact*factors 
		    <= OPB_MAXBF_CNF && (!subquery->ops_global->ops_cb->ops_check ||
		    !opt_strace(subquery->ops_global->ops_cb, OPT_F055_NOCNF)))
	 	noncnf = FALSE;
	    }
	}
	if (noncnf) subquery->ops_mask |= OPS_NONCNF;

	/* Make tree into conjunctive normal form */
	opj_traverse(subquery, qual, &noncnf);

	/* If non-CNF, tidy up the parse tree */
	if (noncnf && ((*qual)->pst_sym.pst_type == PST_AND ||
		(*qual)->pst_sym.pst_type == PST_OR) && FALSE)
	    opj_tidyup(subquery, *qual);

	/* Terminate the list on the rightmost branch */
	opj_adjust(subquery, qual);

	/* Make "ands" into a linear list */
	opj_mvands(subquery, *qual);

# ifdef OPT_F086_DUMP_QTREE2
        if ( subquery->ops_global->ops_cb->ops_check &&
	    opt_strace(subquery->ops_global->ops_cb, OPT_F086_DUMP_QTREE2)) 
	{	/* op214 - dump parse tree in op146 style */
	    opu_qtprint(subquery->ops_global, subquery->ops_root, "after norm");
	}
# endif
        if (/* success && */ subquery->ops_global->ops_cb->ops_check &&
               opt_strace(subquery->ops_global->ops_cb, OPT_F042_PTREEDUMP) &&
		subquery->ops_root)
	{         /* op170 - dump parse tree */
	    opt_pt_entry(subquery->ops_global, NULL, subquery->ops_root->pst_right,
		"after norm");
	}
    }
    subquery->ops_normal = TRUE;	/* subquery is now normalized */
}

/*{
** Name: opj_mvors	- Push all ORs into a linear list
**
** Description:
**      This function pushes all the ORs in a qualification tree into a linear
**	list.  It guarantees that all OR nodes will be on the rightmost edge
**	of the qualification tree. It is assummed that the tree is already
**	normalized and all AND nodes are above OR nodes.
**
** Inputs:
**      qual                            Pointer to the qualification tree
**
** Outputs:
**      qual                            All OR nodes moved to right edge
**	Returns:
**	    E_DB_OK			Success (Failure impossible)
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-apr-90 (stec)
**	    Created.
[@history_template@]...
*/
VOID
opj_mvors(
	PST_QNODE *qual)
{
    register PST_QNODE  *lp;
    register PST_QNODE	*rp;
    register PST_QNODE	*q;

    /*
    ** Traverse the rightmost "AND" branch
    ** of the normalized qualification tree.
    */
    while (qual && qual->pst_sym.pst_type != PST_QLEND)
    {
	q = qual->pst_left;

	/*
	** Traverse the rightmost branch of the 
	** subtree; we are interested in ORs only.
	*/
	while (q && q->pst_sym.pst_type == PST_OR)
	{
	    rp = q->pst_right;
	    lp = q->pst_left;

	    /*
	    ** Move PST_ORs on the left up to 
	    ** the right until there are no more.
	    */
	    while (lp && lp->pst_sym.pst_type == PST_OR)
	    {
		/* Move the PST_OR on the left up to the right */
		q->pst_left = lp->pst_right;
		q->pst_right = lp;
		lp->pst_right = rp;

		/* Reset the left and right ptrs, because there are new children */
		lp = q->pst_left;
		rp = q->pst_right;
	    }

	    /* Go down the right */
	    q = rp;
	}

	qual = qual->pst_right;
    }
}
