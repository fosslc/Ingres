/* 
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <st.h>
#include    <tr.h>
#include    <qu.h>
#include    <bt.h>
#include    <ex.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <adfops.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <scf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <usererror.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>

/**
**
**  Name: PSTRSLV.C - Functions used to resolve a query tree.
**
**  Description:
**      This file contains the functions necessary to resolve a query tree
**	operator node.  This means setting the conversions and the result
**	types and lengths for the node, as well as figuring out which ADF
**	function should be called.
**
**          pst_resolve - Resolve a query tree operator node
**	    pst_union_resolve - Resolve unions
**	    pst_caserslt_resolve - Resolve result expressions of case funcs
**	    pst_caseunion_resolve - Resolve source/comparand exprs of case/union
**	    pst_parm_resolve - Resolve local variables/parms
**	    pst_convlit - Convert literals to compatible type if possible
**
**
**  History:
**      03-oct-85 (jeff)    
**          written
**      13-sep_86 (seputis)
**          added initialization of fidesc ptr
**	4-mar-87 (daved)
**	    added resolution for union nodes
**	23-oct-87 (stec)
**	    General cleanup and fixes to pst_union_resolve.
**	1-feb-88 (stec)
**	    Gene changed data type precedence hierarchy.
**	18-feb-88 (thurston)
**	    Replaced the ty_better() static routine and its uses in pst_opcomp()
**	    and pst_uncomp() with the ty_rank() static routine.  This solves a
**	    bug, and is simpler code.
**	08-jun-88 (stec)
**	    Added pst_parm_resolve procedure.
**	03-nov-88 (stec)
**	    Correct bug found by lint.
**	18-jun-89 (jrb)
**	    Major changes.  Algorithm for doing resolution is now in ADF.  This
**	    routine no longer takes any knowledge about what a datatype looks
**	    like internally nor does it make any assumptions about how a
**	    datatype is resolved.  Removed pst_opcomp, pst_uncomp, and ty_rank
**	    functions.  This file is about half of its original size.
**	12-jul-89 (jrb)
**	    Made E_AD2061_BAD_OP_COUNT map to a user error rather than letting
**	    PSF treat it as an internal error.
**	15-dec-89 (jrb)
**	    Added checks for DB_ALL_TYPE in function instance descriptors (for
**	    outer join project).
**	24-oct-90 (andre)
**	    remove declarations for ty_rank(), pst_opcomp(), pst_uncomp() which
**	    have been previously removed.  Apparently, some compilers do not dig
**	    static declarations for non-existent functions.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	18-feb-93 (walt)
**		Removed (the now incorrect) prototypes for adi_fitab, adi_tycoerce,
**		and adi_tyname now that ADF is prototyped in <adf.h>.
**	23-mar-93 (smc)
**	    Cast parameter of BTtest() to match prototype.
**	29-jun-93 (andre)
**	    #included TR.H
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-mar-94 (andre)
**	    extracted code from pst_union_resolve() into
**	    pst_get_union_resdom_type()
**      1-feb-95 (inkdo01)
**          special case code to allow long byte/varchar union compatibility
**	6-feb-95 (hanch04)
**	    fixed syntax error caused by comment
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      07-Jan-2003 (hanch04)
**          Back out last change buy zhahu02.  Not compatible with new code.
**	17-jul-2003 (toumi01)
**	    Add to PSF some flattening decisions.
**	31-aug-2004 (sheco02)
**	    X-integrate change 466442 to main.
**	31-aug-2004 (sheco02)
**	    Fixed previous cross change mistake.
**	02-Sep-2004 (bonro01)
**	    Fixed x-integration of change 466442
**	3-Nov-2006 (kschendel)
**	    Remove unused case-source resolver.  Combine parts of case
**	    and union resolving since they do the same thing.
**	    Recognize a standalone NULL when case or union resolving and
**	    just adjust the type (e.g. don't max out decimal prec/scale!).
**/


GLOBALREF PSF_SERVBLK     *Psf_srvblk;  /* PSF server control block ptr */

/*
** Counters for constant folding stats - see psfmo.c
*/
i4 Psf_fold = TRUE;
#ifdef PSF_FOLD_DEBUG
i4 Psf_nops = 0;
i4 Psf_nfolded = 0;
i4 Psf_ncoerce = 0;
i4 Psf_nfoldcoerce = 0;
#endif

static DB_STATUS
pst_caserslt_resolve(
	PSS_SESBLK  *sess_cb,
	PST_QNODE   *casep,
	DB_ERROR    *error);

static DB_STATUS pst_caseunion_resolve(
	PSS_SESBLK *sess_cb,
	DB_DATA_VALUE *dv1,
	DB_DATA_VALUE *dv2,
	bool forcenull,
	DB_ERROR *error);


static DB_STATUS
pst_get_union_resdom_type(
			DB_DT_ID	dt,
			DB_DATA_VALUE *resval,
			ADF_CB	*adf_scb,
			bool		nullable,
			DB_ERROR	*error);

/*{
** Name: pst_resolve	- Resolve a query tree operator node
**
** Description:
**      The pst_resolve function resolves a query tree operator node.  This
**	means that it sets result type and length for the node.  It also
**	figures out which ADF function should be called for the node.  This
**	operation should be called whenever a new operator node	is constructed. 
**	It will not be necessary to re-resolve an operator node should it get
**	new children; however, one should be sure that the function that the
**	operator represents can take the new children as arguments (it might
**	be impossible to convert the children to the desired types before
**	performing the function).  Any query tree that comes from the parser
**	is guaranteed to have already been resolved.
**
**	This function will work on the following node types.  They MUST have the
**	children required for the particular node type:
**
**	    Node Type	    Left Child	    Right Child
**
**	    PST_UOP	    Yes		    No
**	    PST_BOP	    Yes		    Yes
**	    PST_AOP	    Yes		    No
**	    PST_COP	    No		    No
**	    PST_MOP         Yes             No
**
**	NOTE: This function does not set the conversion ids for an operator
**	node.  That it because the operator node could receive new children
**	of different types.  The conversions must be set before the expression
**	is used.
**
**	NOTE: Some operators (cast, for example) set the result type by their
**	very definition in the parser. If the result type determined here is
**	the same as the datatype in the operator node, it is assumed that the 
**	parser already set the data type for the operator and it is retained.
**
** Inputs:
**	sess_cb				session control block. MAY BE NULL.
**	adf_scb				adf session control block
**      opnode				The operator node to resolve
**	lang				query language
**      error                           Error block
**
** Outputs:
**      opnode				The resolved operator node
**	error				Standard error block
**	    .err_code			    What error occurred
**		E_PS0000_OK			Success
**		E_PS0002_INTERNAL_ERROR		Internal inconsistency in PSF
**		2908L				No function found by this name
**						that can take these arguments.
**		2910L				Ambiguous function or operator
**		E_PS0C02_NULL_PTR		opnode = NULL
**		E_PS0C03_BAD_NODE_TYPE		opnode is not an operator node
**		E_PS0C04_BAD_TREE		Something is wrong with the
**						query tree.
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	03-oct-85 (jeff)
**          written
**      13-sep-86 (seputis)
**          added ptr to ADI_FI_DESC to node to be resolved
**	14-may-87 (stec)
**	    added code to handle COUNT(*) case in SQL (no children).
**	09-jul-87 (stec)
**	    fixed bug resulting in computation of bad result length.
**	12-aug-87 (thurston)
**	    fixed bug that didn't calculate the result length properly for
**	    aggregates that have the nullability of the result different than
**	    that of the input, AND did not need to coerce the input.
**	18-jun-89 (jrb)
**	    Changed this routine to use ADF to do datatype resolution.  Also,
**	    fixed a bug where the function instance id's for the coercions in an
**	    operator node where being set incorrectly (they're not currently
**	    being used anyway).
**	12-jul-89 (jrb)
**	    Made E_AD2061_BAD_OP_COUNT map to a user error rather than letting
**	    PSF treat it as an internal error.
**	18-jul-89 (jrb)
**	    Set precision field for all db_data_values contained in here.
**	25-jul-89 (jrb)
**	    Made sure db_data field was filled in for db_data_value's used in
**	    calclen calls.
**	15-dec-89 (jrb)
**	    Added checks for DB_ALL_TYPE in function instance descriptors.
**	10-aug-93 (Andre)
**	    removed declaration of scf_call()
**	09-Mar-1999 (shero03)
**	    support MOP nodes.
**	7-sep-99 (inkdo01)
**	    Added calls to pst_caserslt_resolve/pst_casesrce_resolve for case 
**	    function resolution.
**	4-july-01 (inkdo01)
**	    Fix error message for wrong number of function parms.
**      09-dec-2005 (huazh01)
**          if the aggregate is under a PST_BOP node, do not 
**          flatten such query too. 
**          b115593, INGSRV3530.
**	16-jul-03 (hayke02)
**	    Call adi_resolve() with varchar_precedence TRUE if either ps202
**	    (PSS_VARCHAR_PRECEDENCE) is set or psf_vch_prec is ON. This change
**	    fixes bug 109132.
**	22-dec-04 (inkdo01)
**	    Add logic to resolve binop's involving explicit collations.
**	2-feb-06 (dougi)
**	    Make cast functions in select list work.
**	8-mar-06 (dougi)
**	    Slight adjustment to avoid problems from ii_iftrue().
**      17-apr-2006 (huazh01)
**          some nodes in a dynamic SQL query might not be resolved in
**          pst_node(), which will leave the node's dbtype as 0. If
**          the dbtype of a child node does not seem to be valid,
**          try to resolve such child node before we continue our work
**          with the invalid dbtype.
**          bug 115963.
**	25-Apr-2006 (kschendel)
**	    Fix to not check args to varargs fns that are in the optional
**	    part of the param list (>= fi's numargs).
**	    Fix old bug in MOP checking, followed wrong link.
**	05-sep-2006 (gupsh01)
**	    Added call to adi_dtfamily_resolve(), if function instance 
**	    selected is generic to the datatype family of the operands.
**	25-oct-2006 (dougi)
**	    Check for null sess_cb before dereferencing it.
**	03-Nov-2007 (kiria01) b119410
**	    When determining the length of parameters ensure that the
**	    datatype is correct if a coercion has been applied.
**	13-Apr-2008 (kiria01) b119410
**	    Apply string-numeric compares using a normalise operator
**	    that we add to the tree at the appropriate place.
**	10-Jul-2008 (kiria01) b119410
**	    Drive the numericnorm insertion based on the NUMERIC
**	    compares instead of BYTE-BYTE as we retain ADF interface
**	    boundaries better as only the dbms (parser) needs the fixup.
**	25-Jun-2008 (kiria01) SIR120473
**	    Move pst_isescape into new u_i2 pst_pat_flags.
**	22-Sep-2008 (kiria01) b120926
**	    Extend numericnorm insertion to detect compressed '=' tree
**	    sometimes used to build IN value lists.
**	12-Oct-2008 (kiria01) SIR121012
**	    Detect and pre-evaluate constant actions applying them back
**	    into the parse tree. This is applied on two levels - coercion
**	    operators on constant operands and on constant functions
**	    of constant operands. The former are very frequent where
**	    often a varchar constant is used to initialise another
**	    datatype.
**	15-Oct-2008 (kiria01) b121061
**	    Increase the number of parameters supported in the calls
**	    to adf_func using the more general vector form.
**	20-Oct-2008 (kiria01) b121098
**	    Properly establish the pst_oplcnvrtid & pst_oprcnvrtid
**	    fields to reliably indicate whether an operand requires
**	    a coercion and if so which one. Well, almost - see comment
**	    below regarding PST_OPERAND and lack of its pst_value.
**	12-Oct-2008 (kiria01) SIR121012
**	    Spread the net wider to catch more constant coercions.
**	    This involves the introduction of ADI_F16384_CHKVARFI and
**	    state information from ADF - .adf_const_const
**	    Also draw in the constants whose matching datatypes would
**	    have excluded them from folding - these could still have
**	    length or precision changes that need applying. This might
**	    look like it will add extra un-needed coercions but we do
**	    this just to literal constants and then these will fold
**	    away.
**	15-Nov-2008 (kiria01) SIR120473
**	    Explicitly handle nil-coercions in the constant folding
**	    where a datatype has minimal support.
**	31-Dec-2008 (kiria01) SIR121012
**	    Leave IN value list constants intact and don't attempt to
**	    fold them.
**	09-Feb-2009 (kiria01) b121546
**	    Propagate the resolved type of the injected numericnorm
**	    node through subselect nodes.
**	06-Mar-2009 (kiria01) b119410
**	    Support efficient folding of string to numeric if string
**	    constant expression offered. This will now allow for key
**	    support in these situations which apparently happen more
**	    often than expected.
**	11-Mar-2009 (kiria01) b121781
**	    Calculate prec & scale instead of using the numeric side
**	    values from a string numeric compare. Otherwise we will
**	    likely loose precision with decimal.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	21-May-2009 (kiria01) b122051
**	    Tidy collID handling
**	24-Aug-2009 (kiria01) b122522
**	    Sanity check before allowing string to int coercion, there
**	    might be a decimal point in the string that default coercion
**	    would be relaxed about.
**	23-Sep-2009 (kiria01) b122578
**	    Initialise the ADF_FN_BLK .adf_fi_desc and adf_dv_n members.
**	    Update the parse tree node indication a coercion is no longer
**	    required due to folding.
**	11-Nov-2009 (kiria01) SIR 121883
**	    With PST_INLIST, check for PST_CONST on RHS as the flag
**	    may be applied to PST_SUBSEL to aid servers to distinguish
**	    simple = from IN and <> from NOT IN.
**	27-Nov-2009 (kiria01) b122966
**	    Fully ringfence exceptions in adf_func as they will be
**	    re-raised in context later.
**	02-May-2010: (kiria01) b123672
**	    Split length check from datatype to avoid the inevitable but
**	    overlooked clash between BYTE(64) and I8 etc!
**      19-May-2010 (hanal04) Bug 123764
**          IFNULL aggregate handling was taking place without checking for
**          and earlier failure in adi_resolve(). This lead to a SIGSEGV
**          instead of the expected error report. Move the IFNULL aggregate
**          down below the error check.
**      7-jun-2010 (stephenb)
**          correct error handling for bad return of adi_dtfamilty_resolve,
**          it may produce various type and operand errors that need to be 
**          returned as user errors. (Bug 123884)
**	14-Jul-2010 (kschendel) b123104
**	    Don't fold true/false generating operators back to constants.
**	3-Aug-2010 (kschendel) b124170
**	    Use joinid from node being resolved when inserting a coercion.
**	4-aug-2010 (stephenb)
**	    ADF calls may also return E_AD2009_NOCOERCION when no coercion
**	    is available, treat as user error. Also fix error handling
**	    for other ADF calls. (Bug 124209)
**	12-Aug-2010 (kschendel) b124234
**	    Don't attempt to constant-fold LONG data.  Even if it works,
**	    the resulting value described by the coupon is unlikely to
**	    last until execution time.
**	24-Sep-2010 (smeke01) b123993
**	    For SQL, the flag value for pmspec for the replacement constant
**	    must always be PST_PMNOTUSED. For QUEL, base the flag value on the
**	    value(s) seen in the original operand constants.
*/
DB_STATUS
pst_resolve(
	PSS_SESBLK	    *sess_cb,
	ADF_CB		    *adf_scb,
	register PST_QNODE  *opnode,
	DB_LANG 	    lang,
	DB_ERROR	    *error)
{

    i4			children;
    i4			dtbits;
    i4			i;
    i4			npars;
    DB_STATUS		status;
    DB_DATA_VALUE	ops[ADI_MAX_OPERANDS];
    DB_DATA_VALUE	*pop[ADI_MAX_OPERANDS];
    DB_DATA_VALUE	tempop1;
    ADI_FI_DESC		*fi_desc;
    ADI_FI_DESC		*best_fidesc;
    ADI_FI_DESC		*base_fidesc;
    ADI_RSLV_BLK	adi_rslv_blk;
    ADI_FI_ID		*pcnvrtid;
    PST_QNODE  		*lopnode;
    PST_QNODE	        *lqnode;
    PST_QNODE           *resqnode;
    i1		        leftvisited = FALSE;
    i1			const_cand = Psf_fold/*TRUE*/;
    i4		val1;
    i4		val2;
    PST_PMSPEC		pmspec = PST_PMNOTUSED;
#ifdef    xDEBUG
    SCF_CB		scf_cb;
    SCF_SCI		sci_list[1];
    PTR			scf_scb;
#endif

    /* Make sure there is a node to resolve */
    if (opnode == (PST_QNODE *) NULL)
    {
	error->err_code = E_PS0C02_NULL_PTR;
	return (E_DB_SEVERE);
    }

    /* If it's a "case" function, call the special case resolution functions. */
    if (opnode->pst_sym.pst_type == PST_CASEOP)
	return pst_caserslt_resolve(sess_cb, opnode, error);

    /* Make sure it's an operator node */
    if (opnode->pst_sym.pst_type != PST_UOP &&
        opnode->pst_sym.pst_type != PST_BOP &&
        opnode->pst_sym.pst_type != PST_COP &&
	opnode->pst_sym.pst_type != PST_AOP &&
	opnode->pst_sym.pst_type != PST_MOP)
    {
	error->err_code = E_PS0C03_BAD_NODE_TYPE;
	return (E_DB_SEVERE);
    }

    for (i = 0; i < ADI_MAX_OPERANDS; i++)
    {	
    	pop[i] = (DB_DATA_VALUE *)NULL;
	adi_rslv_blk.adi_dt[i] = DB_NODT;
    }

    /* Figure out the number of children */
    if ((opnode->pst_sym.pst_type == PST_COP) ||
	((opnode->pst_sym.pst_type == PST_AOP) &&
	 (opnode->pst_sym.pst_value.pst_s_op.pst_opno == ADI_CNTAL_OP))
       )
	children = 0;
    else if (opnode->pst_sym.pst_type == PST_BOP)
	children = 2;
    else
	children = 1;

    if (opnode->pst_sym.pst_type == PST_AOP &&
	opnode->pst_right != (PST_QNODE *) NULL) children = 2;

    if (opnode->pst_sym.pst_type == PST_MOP)
    {
	lopnode = opnode;
	children = 0;
	while (lopnode)
	{	
	   if (lopnode->pst_right)     
	   {	
              adi_rslv_blk.adi_dt[children] =
	    	    lopnode->pst_right->pst_sym.pst_dataval.db_datatype;
    	      children++;
	   }
	   lopnode = lopnode->pst_left;
	}
    }		
    else if (children >= 1)
    {
	if (opnode->pst_left == (PST_QNODE *) NULL)
	{
             error->err_code = E_PS0C04_BAD_TREE;
	     return (E_DB_SEVERE);
	}
        adi_rslv_blk.adi_dt[0] = 
		opnode->pst_left->pst_sym.pst_dataval.db_datatype;

        /* b115963 */
        if (adi_rslv_blk.adi_dt[0] == 0) 
        {
           status = pst_resolve(sess_cb, adf_scb, opnode->pst_left, lang, error);

           if (status != E_DB_OK) return (E_DB_SEVERE);

           adi_rslv_blk.adi_dt[0] =
                opnode->pst_left->pst_sym.pst_dataval.db_datatype;
        }

	if (children == 2)
	{
	    if (opnode->pst_right == (PST_QNODE *) NULL)
	    {
        	error->err_code = E_PS0C04_BAD_TREE;
		return (E_DB_SEVERE);
	    }
            adi_rslv_blk.adi_dt[1] = 
		opnode->pst_right->pst_sym.pst_dataval.db_datatype;

            /* b115963 */
            if (adi_rslv_blk.adi_dt[1] == 0)
            {
               status = pst_resolve(sess_cb, adf_scb, opnode->pst_right,
                                      lang, error);

               if (status != E_DB_OK) return (E_DB_SEVERE);

               adi_rslv_blk.adi_dt[1] =
                   opnode->pst_right->pst_sym.pst_dataval.db_datatype;
            }

	}
    }

#ifdef	xDEBUG

    /* Don't do tracing if couldn't get session control block */
    if (sess_cb != (PSS_SESBLK *) NULL)
    {
	if (ult_check_macro(&sess_cb->pss_trace, 21, &val1, &val2))
	{
	    TRdisplay("Resolving operator with %d children\n\n", children);
	}
    }
#endif

    /* set up to call ADF to resolve this node */
    adi_rslv_blk.adi_op_id   = opnode->pst_sym.pst_value.pst_s_op.pst_opno;
    adi_rslv_blk.adi_num_dts = children;
    tempop1 = opnode->pst_sym.pst_dataval;

    if ((Psf_srvblk->psf_vch_prec == TRUE) ||
	(sess_cb && ult_check_macro(&sess_cb->pss_trace,
	PSS_VARCHAR_PRECEDENCE, &val1, &val2)))
	status = adi_resolve(adf_scb, &adi_rslv_blk, TRUE);
    else
	status = adi_resolve(adf_scb, &adi_rslv_blk, FALSE);

    if (status != E_DB_OK)
    {
	/* Complain if no applicable function found */
	if (	adf_scb->adf_errcb.ad_errcode == E_AD2062_NO_FUNC_FOUND ||
		adf_scb->adf_errcb.ad_errcode == E_AD2009_NOCOERCION)
	{
	    if (children == 1)
		error->err_code = 2907L;
	    else
		error->err_code = 2908L;
	    return (E_DB_ERROR);
	}

	/* Complain if parameter count is wrong. */
	if (  	adf_scb->adf_errcb.ad_errcode == E_AD2061_BAD_OP_COUNT)
	{
	    error->err_code = 2903L;
	    return(E_DB_ERROR);
	}

	/* Complain if ambiguous function found */
	if (adf_scb->adf_errcb.ad_errcode == E_AD2063_FUNC_AMBIGUOUS)
	{
	    if (children == 1)
		error->err_code = 2909L;
	    else
		error->err_code = 2910L;
	    return (E_DB_ERROR);
	}

	error->err_code = E_PS0C05_BAD_ADF_STATUS;
	return (E_DB_ERROR);
    }

    /*
    ** for flattening we care if we have one or multiple ifnulls with
    ** aggregates, so use two flags as a two-bit counter indicating that
    ** we have none, one, or more than one
    */
    if ( sess_cb && adi_rslv_blk.adi_op_id == ADI_IFNUL_OP )
    {
	if ( (opnode->pst_left->pst_sym.pst_type == PST_AGHEAD )
             ||
             /* b115593, INGSRV3530 
             ** do not flatten the query if the PST_AGHEAD is under
             ** a PST_BOP node.
             */
             (
              (opnode->pst_left->pst_sym.pst_type == PST_BOP) &&
              (
                 (opnode->pst_left->pst_left->pst_sym.pst_type ==
                   PST_AGHEAD) 
                 ||
                 (opnode->pst_left->pst_right->pst_sym.pst_type ==
                   PST_AGHEAD)
              )
             )
           )
	{
	    if (sess_cb->pss_flattening_flags & PSS_IFNULL_AGHEAD)
                sess_cb->pss_flattening_flags |= PSS_IFNULL_AGHEAD_MULTI;
	    sess_cb->pss_flattening_flags |= PSS_IFNULL_AGHEAD;
	}
    }

    /* we now have the "best" fi descriptor */
    best_fidesc = base_fidesc = adi_rslv_blk.adi_fidesc;
    
    if (base_fidesc)
    {
	i2	need_fmly_resolve = 0;

	status = adi_need_dtfamily_resolve( adf_scb, base_fidesc, 
					    &adi_rslv_blk, 
					    &need_fmly_resolve);
	if (status)
	{
    	  error->err_code = E_PS0C05_BAD_ADF_STATUS;
	  return (E_DB_SEVERE);
	}

	if (sess_cb && need_fmly_resolve == 1) 
	{
	    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
		sizeof(ADI_FI_DESC), &best_fidesc, error);
            if (status != E_DB_OK)
            {
              return (status);
            }
 	    if  (best_fidesc == NULL)
	    {
    	      error->err_code = E_PS0C05_BAD_ADF_STATUS;
	      return (E_DB_SEVERE);
	    }

	    status = adi_dtfamily_resolve(adf_scb, base_fidesc, 
					  best_fidesc, &adi_rslv_blk);
	    if (status != E_DB_OK)
	    {
		/* Complain if no applicable function found */
		if (	adf_scb->adf_errcb.ad_errcode == E_AD2062_NO_FUNC_FOUND
			|| adf_scb->adf_errcb.ad_errcode == E_AD2009_NOCOERCION)
		{
		    if (children == 1)
			error->err_code = 2907L;
		    else
			error->err_code = 2908L;
		    return (E_DB_ERROR);
		}

		/* Complain if parameter count is wrong. */
		if (  	adf_scb->adf_errcb.ad_errcode == E_AD2061_BAD_OP_COUNT)
		{
		    error->err_code = 2903L;
		    return(E_DB_ERROR);
		}

		/* Complain if ambiguous function found */
		if (adf_scb->adf_errcb.ad_errcode == E_AD2063_FUNC_AMBIGUOUS)
		{
		    if (children == 1)
			error->err_code = 2909L;
		    else
			error->err_code = 2910L;
		    return (E_DB_ERROR);
		}

		error->err_code = E_PS0C05_BAD_ADF_STATUS;
		return (E_DB_ERROR);
	    }
	}
    }
    if (sess_cb &&
	lang == DB_SQL &&
	best_fidesc->adi_fitype == ADI_COMPARISON &&
	adi_rslv_blk.adi_num_dts == 2 &&
	best_fidesc->adi_dt[0] == DB_ALL_TYPE &&
	best_fidesc->adi_dt[1] == DB_ALL_TYPE &&
	(opnode->pst_sym.pst_value.pst_s_op.pst_flags & PST_INLIST) == 0)
    {
	ADI_RSLV_BLK tmp_adi_rslv_blk;
	bool handled = FALSE;
	/*
	** adi_resolve has spotted a string-numeric compare and
	** diverted the FI to one of the NUMERIC comparisons. While
	** this will work it will be inefficient for keyed lookup.
	** In the general case this could be a full join with arbitary
	** expressions. There is a way for this to be handled reasonably
	** but there are some cases that can be done more directly.
	** These relate to the case where we may have a constant and
	** this could be either the the LHS or RHS. If the constant is
	** a number then there is not much we can do as the string expr
	** could have all manner of spacing or justification. However,
	** if we have a string constant then we directly attempt the
	** coercion to the other side. If this works then we can fix up
	** the comparison to the actual type and drop down for constant
	** folding. In all other cases we do the general form.
	**
	** Do we have a string constant?
	*/
	{
	    PST_QNODE **s = &opnode->pst_left;
	    PST_QNODE **n = &opnode->pst_right;
	    PST_QNODE *res_node = NULL;
	    DB_DT_ID n_dt = (*n)->pst_sym.pst_dataval.db_datatype;

	    status = 0;
	    switch (abs((*s)->pst_sym.pst_dataval.db_datatype))
	    {
	    default:
		/* A none s-n - just ignore it */
		break;
	    case DB_INT_TYPE:
	    case DB_DEC_TYPE:
	    case DB_FLT_TYPE:
		/* Swap nodes & fall through */
		s = &opnode->pst_right;
		n = &opnode->pst_left;
		n_dt = (*n)->pst_sym.pst_dataval.db_datatype;
		/*FALLTHROUGH*/
	    case DB_TXT_TYPE:
	    case DB_CHR_TYPE:
	    case DB_CHA_TYPE:
	    case DB_VCH_TYPE:
	    case DB_LVCH_TYPE:
	    case DB_NCHR_TYPE:
	    case DB_NVCHR_TYPE:
	    case DB_LNVCHR_TYPE:
	    case DB_LTXT_TYPE:
		if ((*s)->pst_sym.pst_type == PST_CONST &&
		    (*s)->pst_sym.pst_value.pst_s_cnst.pst_tparmtype == PST_USER &&
		    (*s)->pst_sym.pst_value.pst_s_cnst.pst_parm_no == 0)
		{
		    PST_OP_NODE op;
		    i2 ps = (*n)->pst_sym.pst_dataval.db_prec;
		    i4 len = (*n)->pst_sym.pst_dataval.db_length;
		    /* We do seem to have a string constant so
		    ** lets try to directly coerce to the datatype/size
		    ** of the other side */
		    op.pst_opmeta = PST_NOMETA;
		    op.pst_pat_flags = AD_PAT_DOESNT_APPLY;
		    op.pst_joinid = opnode->pst_sym.pst_value.pst_s_op.pst_joinid;
		    /* The darn i4 casts are because sizeof is technically
		    ** a size_t, which is 64-bit on LP64, and gcc complains. */
		    switch (abs(n_dt))
		    {
		    case DB_INT_TYPE:
		    case DB_FLT_TYPE:
			switch (n_dt*8+len)
			{
			case DB_INT_TYPE*8+(i4)sizeof(i1):
			case-DB_INT_TYPE*8+(i4)sizeof(i1)+1: /*Nullable*/
			    op.pst_opno = ADI_I1_OP;
			    break;
			case DB_INT_TYPE*8+(i4)sizeof(i2):
			case-DB_INT_TYPE*8+(i4)sizeof(i2)+1: /*Nullable*/
			    op.pst_opno = ADI_I2_OP;
			    break;
			case DB_INT_TYPE*8+(i4)sizeof(i4):
			case-DB_INT_TYPE*8+(i4)sizeof(i4)+1: /*Nullable*/
			    op.pst_opno = ADI_I4_OP;
			    break;
			case DB_INT_TYPE*8+(i4)sizeof(i8):
			case-DB_INT_TYPE*8+(i4)sizeof(i8)+1: /*Nullable*/
			    op.pst_opno = ADI_I8_OP;
			    break;
			case DB_FLT_TYPE*8+(i4)sizeof(f4):
			case-DB_FLT_TYPE*8+(i4)sizeof(f4)+1: /*Nullable*/
			    op.pst_opno = ADI_F4_OP;
			    break;
			case DB_FLT_TYPE*8+(i4)sizeof(f8):
			case-DB_FLT_TYPE*8+(i4)sizeof(f8)+1: /*Nullable*/
			    op.pst_opno = ADI_F8_OP;
			    break;
			}
			break;
		    case DB_DEC_TYPE:
			{
			    /* Decimal is a little more awkward */
			    PST_CNST_NODE cconst;
			    cconst.pst_tparmtype = PST_USER;
			    cconst.pst_parm_no = 0;
			    cconst.pst_pmspec  = PST_PMNOTUSED;
			    cconst.pst_cqlang = DB_SQL;
			    cconst.pst_origtxt = (char *) NULL;

			    /* Find out best prec/scale to use */
			    if (status = adu_decprec(adf_scb, &(*s)->pst_sym.pst_dataval, &ps))
				break;
			    status = pst_node(sess_cb, &sess_cb->pss_ostream, (PST_QNODE *)NULL,
				(PST_QNODE *)NULL, PST_CONST, (char *)&cconst,
				sizeof(cconst), DB_INT_TYPE, (i2) 0, (i4)sizeof(ps),
				(DB_ANYTYPE *)&ps, &res_node, error, 0);
			    /* Adjust the length to match the prec */
			    len = DB_PREC_TO_LEN_MACRO(DB_P_DECODE_MACRO(ps));
			    op.pst_opno = ADI_DEC_OP;
			}
			break;
		    default:
			status = -1;
		    }
		    if (!status && abs(n_dt) == DB_INT_TYPE)
		    {
			/* Extra check for integer. Normal string coercion to int
			** will stop at the decimal point without error. We use 'is integer'
			** to see if there would be a problem & if not, we go for it. */
			i4 cmp = 0;
			if (!(status = adu_isinteger(adf_scb, &(*s)->pst_sym.pst_dataval, &cmp)) &&
				cmp == 0)
			    status = -1;
		    }
		    if (!status &&
			!pst_node(sess_cb, &sess_cb->pss_ostream,
			*s, res_node, res_node ? PST_BOP : PST_UOP,
			    (char *)&op, sizeof(op), n_dt, ps, len,
			    (DB_ANYTYPE *)NULL, &res_node, error, PSS_JOINID_PRESET) &&
			res_node &&
			res_node->pst_sym.pst_type == PST_CONST)
		    {
			/* We sucessfully converted the string to the
			** number and now we need to redo the resolve
			** for the new datatype.
			*/
			tmp_adi_rslv_blk = adi_rslv_blk;
			tmp_adi_rslv_blk.adi_dt[0] = tmp_adi_rslv_blk.adi_dt[1] = abs(n_dt);
			if (!adi_resolve(adf_scb, &tmp_adi_rslv_blk, FALSE))
			{
			    /* Resolver happy, use the new fidesc and set the new
			    ** node into the tree */
			    adi_rslv_blk = tmp_adi_rslv_blk;
			    best_fidesc = adi_rslv_blk.adi_fidesc;
			    *s = res_node;
			    handled = TRUE;
			}
		    }
		}
	    }
	}
	/*
	** What we do is swap the NUMERIC compare for a BYTE-BYTE
	** compare and insert calls to numericnorm()
	** Note: IN list processing is presently not optimised in
	** this manner. To do so would require expanding out the
	** compacted list as the short form uses the pst_left field
	** as the constant list pointer and this would conflict with
	** more general expression code.
	*/
	tmp_adi_rslv_blk = adi_rslv_blk;
	tmp_adi_rslv_blk.adi_dt[0] = tmp_adi_rslv_blk.adi_dt[1] = DB_BYTE_TYPE;
	if (!handled && !adi_resolve(adf_scb, &tmp_adi_rslv_blk, FALSE))
	{
	    PST_OP_NODE op;
	    PST_QNODE **p;
	    PST_QNODE *t;
	    i = 0;
	    /* Use the BYTE-BYTE fidesc we just looked up */
	    adi_rslv_blk = tmp_adi_rslv_blk;
            best_fidesc = adi_rslv_blk.adi_fidesc;

	    op.pst_opno = ADI_NUMNORM_OP;
	    op.pst_opmeta = PST_NOMETA;
	    op.pst_pat_flags = AD_PAT_DOESNT_APPLY;
	    op.pst_joinid = opnode->pst_sym.pst_value.pst_s_op.pst_joinid;

	    /* String numeric compare spotted - slot
	    ** in normalisation functions */
	    p = &opnode->pst_left;
	    do /* once for each sub-tree */
	    {
		PST_QNODE **p2 = p;
		/* We need to find where to stick the normalisation
		** function - usually it will be:
		**	x = y
		** becomes:
		**	n(x) = n(y)
		** However, sometimes the value of interest is buried -
		**	x = (SELECT y FROM ... )
		** must become:
		**	n(x) = (SELECT n(y) FROM ... )
		** Sneeky eh? This means there will be an intervening
		** PST_SUBSEL and PST_RESDOM node to step over. We
		** will allow for stepping over multiple instances
		** in anticipation of select list sub-selects.
		*/
		while ((t = *p) &&
			t->pst_sym.pst_type == PST_SUBSEL &&
			(t = t->pst_left) &&
			t->pst_sym.pst_type == PST_RESDOM)
		{
		    p = &t->pst_right;
		}
		t = *p;
		*p = NULL;
		if (status = pst_node(sess_cb, &sess_cb->pss_ostream,
			t, (PST_QNODE *)NULL, PST_UOP,
			(char *)&op, sizeof(op), DB_NODT, (i2)0, (i4)0,
			(DB_ANYTYPE *)NULL, p, error, PSS_JOINID_PRESET))
		    return (status);
		/* Having injected the node we need to propagete
		** the full resolved datatype to any intermediate
		** SUBSEL/RESDOM nodes */
		while ((t = *p2) &&
			t->pst_sym.pst_type == PST_SUBSEL &&
			(t = t->pst_left) &&
			t->pst_sym.pst_type == PST_RESDOM)
		{
		    /* If subsel here we need to propagate the type up
		    ** from the operator we're about to add.
		    */
		    t->pst_sym.pst_dataval =
			(*p2)->pst_sym.pst_dataval = (*p)->pst_sym.pst_dataval;
		    p2 = &t->pst_right;
		}
		p = &opnode->pst_right;
	    } while (i++ == 0);
	}
    }
    status = adi_res_type(adf_scb, best_fidesc,
    		adi_rslv_blk.adi_num_dts, adi_rslv_blk.adi_dt,
		&opnode->pst_sym.pst_dataval.db_datatype);
		
    if (status != E_DB_OK)
    {
    	error->err_code = E_PS0C05_BAD_ADF_STATUS;
	return (E_DB_SEVERE);
    }
    opnode->pst_sym.pst_value.pst_s_op.pst_opinst = best_fidesc->adi_finstid;
    opnode->pst_sym.pst_value.pst_s_op.pst_fdesc = best_fidesc;
    if (opnode->pst_sym.pst_dataval.db_datatype < 0 &&
	tempop1.db_datatype > 0)
    {
	tempop1.db_length++;
	tempop1.db_datatype = -tempop1.db_datatype;
    }

    /*
    ** Figure out the result length.  First we must get the lengths of the
    ** operands after conversion, then we must use these lengths to figure
    ** out the length of the result.
    */

    npars = 0;
    if (opnode->pst_sym.pst_type == PST_MOP)
    {
	/* MOP operands are chained thru the left link, but the first
	** operand is the mop's right link.
	*/
    	resqnode = opnode;
	lqnode = resqnode->pst_right;
	pcnvrtid = &resqnode->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid;
    }
    else
    {
    	lqnode = opnode->pst_left;
	pcnvrtid = &opnode->pst_sym.pst_value.pst_s_op.pst_oplcnvrtid;
	if (lqnode == NULL)
	{
	   lqnode = opnode->pst_right;
	   pcnvrtid = &opnode->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid;
	   leftvisited = TRUE;
	}
    }
    
    if (best_fidesc->adi_fiflags & ADI_F1_VARFI)
	const_cand = FALSE;

    /* Don't constant fold constant true/false generating operators either.
    ** We want these to stay operators (to preserve joinid).
    ** Normally one wouldn't hit iitrue/iifalse during resolve, but it
    ** can happen when re-processing a tree for e.g. parameter substitution.
    */
    if (opnode->pst_sym.pst_value.pst_s_op.pst_opno == ADI_IITRUE_OP
      || opnode->pst_sym.pst_value.pst_s_op.pst_opno == ADI_IIFALSE_OP)
	const_cand = FALSE;

    while (lqnode)
    {
	DB_DATA_VALUE tempop;
	bool const_coerce = TRUE;

	/* Assume no coercion for now */
	tempop = lqnode->pst_sym.pst_dataval;
	ops[npars] = tempop;

	/* If this operand is a constant then we are going to pass this
	** down the coercion path as even if the DTs match the lengths/sizes
	** or precision may still need a coercion that we will otherwise miss
	** and they'll end up added in later by optimiser anyway */
	if (!Psf_fold ||
		!sess_cb ||
		lqnode->pst_sym.pst_type != PST_CONST ||
		lqnode->pst_sym.pst_value.pst_s_cnst.pst_tparmtype != PST_USER ||
		lqnode->pst_sym.pst_value.pst_s_cnst.pst_parm_no != 0)
	    const_coerce = FALSE;

	/* Don't run a coercion that results in a LONG type. */
	if (const_coerce)
	{
	    status = adi_dtinfo(adf_scb, best_fidesc->adi_dt[npars], &dtbits);
	    if (status == E_DB_OK && dtbits & AD_PERIPHERAL)
		const_coerce = FALSE;
	}

	/* Get coercion id for this child, if needed */
	if (npars < best_fidesc->adi_numargs &&
	    best_fidesc->adi_dt[npars] != DB_ALL_TYPE &&
	    (best_fidesc->adi_dt[npars] != 
		abs(lqnode->pst_sym.pst_dataval.db_datatype) ||
	    const_coerce &&	/* This to catch elidable constant coercions */
		best_fidesc->adi_dt[npars] == base_fidesc->adi_dt[npars]))
	{
#	    ifdef PSF_FOLD_DEBUG
	    Psf_ncoerce++;
#	    endif
	    status = adi_ficoerce(adf_scb, 
	   	lqnode->pst_sym.pst_dataval.db_datatype,
		best_fidesc->adi_dt[npars],
		pcnvrtid);
	    if (status == E_DB_OK)
		status = adi_fidesc(adf_scb, *pcnvrtid, &fi_desc);
	    if (status != E_DB_OK)
	    {
		if (!const_coerce ||
		    best_fidesc->adi_dt[npars] != base_fidesc->adi_dt[npars])
		{
		    /* should return user error for no coercion */
		    if (	adf_scb->adf_errcb.ad_errcode == E_AD2062_NO_FUNC_FOUND
			    || adf_scb->adf_errcb.ad_errcode == E_AD2009_NOCOERCION)
		    {
			if (children == 1)
			    error->err_code = 2907L;
			else
			    error->err_code = 2908L;
			return (E_DB_ERROR);
		    }

		    /* Complain if parameter count is wrong. */
		    if (  	adf_scb->adf_errcb.ad_errcode == E_AD2061_BAD_OP_COUNT)
		    {
			error->err_code = 2903L;
			return(E_DB_ERROR);
		    }

		    /* Complain if ambiguous function found */
		    if (adf_scb->adf_errcb.ad_errcode == E_AD2063_FUNC_AMBIGUOUS)
		    {
			if (children == 1)
			    error->err_code = 2909L;
			else
			    error->err_code = 2910L;
			return (E_DB_ERROR);
		    }
		    
		    error->err_code = E_PS0C05_BAD_ADF_STATUS;
		    return (E_DB_SEVERE);
		}
		/* Although no coersion operator exists for this we only went
		** down this path to action coersions that matched datatypes
		** but could have length or precision actions required so we
		** ignore the error and saving the NIL coercion FI to
		** use later. */
		*pcnvrtid = ADI_NILCOERCE;
	    }
	    else if (fi_desc->adi_fitype == ADI_AGG_FUNC ||
		    (opnode->pst_sym.pst_value.pst_s_op.pst_flags & PST_INLIST) &&
			opnode->pst_right->pst_sym.pst_type == PST_CONST)
	    {
		/* Don't fold aggregates or IN lists */
		const_cand = FALSE;
		ops[npars] = tempop;
	    }
	    else
	    {
		{
		    /* Get length after coercion */
		    DB_DATA_VALUE *opptrs = &ops[npars];
		    /* Caller needs to setup the result datatype and the
		    ** coercion will probably have changed it!
		    */
		    tempop.db_datatype = lqnode->pst_sym.pst_dataval.db_datatype < 0
			? -best_fidesc->adi_dt[npars]
			: best_fidesc->adi_dt[npars];
		    status = adi_0calclen(adf_scb, &fi_desc->adi_lenspec, 1,
				&opptrs, &tempop);
		}
		if (status != E_DB_OK 
		    && adf_scb->adf_errcb.ad_errcode != E_AD2022_UNKNOWN_LEN)
		{
		    error->err_code = E_PS0C05_BAD_ADF_STATUS;
		    return (E_DB_SEVERE);
		}

		/* Not all coercions are constant! Not at first obvious
		** but due to things like decimal points and currency
		** symbols, some must be done at runtime and can't be
		** optimised :-(
		** So we look for potential constant function invocations
		** based on the VARFI flag being clear (even for coercions)
		** and only when all operands are constant will we fold. */
		if (!const_coerce ||
		    (fi_desc->adi_fiflags & ADI_F1_VARFI) != 0)
		{
		    const_cand = FALSE;
		    ops[npars] = tempop;
		}
		else
		{
		    /* Constant fold the coercion.
		    ** Apply the coercion to the constant now and replace
		    ** the constant node. We do this regardless of whether the
		    ** parent can be folded - the coercion can be and that will
		    ** help.
		    */
		    ADF_FN_BLK func_blk;
		    DB_ERROR err_blk;
		    /*
		    ** Set up function block to convert the constant argument.
		    ** Then call ADF to execute it
		    */
		    /* Set up the 1st (and only) parameter */
		    func_blk.adf_dv[1] = lqnode->pst_sym.pst_dataval;
		    func_blk.adf_dv_n = 1;
		    /* Set up the result value */
		    func_blk.adf_dv[0] = tempop;
		    /* Allocate space for the destination data */
		    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
					func_blk.adf_dv[0].db_length,
					(PTR *)&func_blk.adf_dv[0].db_data, &err_blk);
		    if (!status)
		    {
			/* To control math exceptions, we tell the PSF exception handler
			** to call the ADF exception handler to handle exceptions and
			** we arrange to have these just to continue with a warning */
			i2 save_exc_state = adf_scb->adf_exmathopt;

			sess_cb->pss_stmt_flags |= PSS_CALL_ADF_EXCEPTION;
			adf_scb->adf_exmathopt = ADX_IGNALL_MATHEX;
			adf_scb->adf_errcb.ad_errcode = 0;

			/* Set up the function id and execute it */
			func_blk.adf_fi_id = *pcnvrtid;
			func_blk.adf_pat_flags = AD_PAT_DOESNT_APPLY;
			func_blk.adf_fi_desc = NULL;
			if (fi_desc->adi_fiflags & ADI_F16384_CHKVARFI)
			{
			    u_i1 save = adf_scb->adf_const_const;
			    adf_scb->adf_const_const = 0;
			    status = adf_func(adf_scb, &func_blk);
			    /* If any flags set (AD_CNST_DSUPLMT or AD_CNST_USUPLMT
			    ** then this was not constant - stop it folding. */
			    if (adf_scb->adf_const_const)
				status = -1;
			    adf_scb->adf_const_const = save;
			}
			else
			    status = adf_func(adf_scb, &func_blk);

			/* Reset the exception flag and state */
			sess_cb->pss_stmt_flags &= ~PSS_CALL_ADF_EXCEPTION;
			adf_scb->adf_exmathopt = save_exc_state;
		    }

		    if (status || adf_scb->adf_errcb.ad_errcode)
		    {
			/* If we got problems trying this, treat the parent
			** as non-constant and continue */
			const_cand = FALSE;
			ops[npars] = tempop;
		    }
		    else
		    {
#			ifdef PSF_FOLD_DEBUG
			Psf_nfoldcoerce++;
#			endif
			ops[npars] = func_blk.adf_dv[0];
			/* We need to update the parse tree with the
			** new value. It will already be a PST_CONST
			** node but its length will reflect the prior
			** (pre-coerced) value. */
			lqnode->pst_left = NULL;
			lqnode->pst_right = NULL;
			lqnode->pst_sym.pst_len -= lqnode->pst_sym.pst_dataval.db_length;
			lqnode->pst_sym.pst_dataval = func_blk.adf_dv[0];
			lqnode->pst_sym.pst_len += lqnode->pst_sym.pst_dataval.db_length;
			/* Update the parse tree noting the action done */
			*pcnvrtid = ADI_NILCOERCE;
		    }
		}
	    }
	}
	else
	{
	    /* No coercion needed. This is as determined by datatype as
	    ** above but this could be with a dtfamily fixed up pst_fdesc
	    ** which will not always persist - certainly not through RDF
	    ** if used in a view. As we have saved the coercion FI to
	    ** use later, we now save the fact that no coercion is needed
	    ** for this one even if the datatypes differ! */
	    *pcnvrtid = ADI_NILCOERCE;

	    if (const_cand &&
			!(lqnode->pst_sym.pst_type == PST_CONST &&
			lqnode->pst_sym.pst_value.pst_s_cnst.pst_tparmtype == PST_USER &&
			lqnode->pst_sym.pst_value.pst_s_cnst.pst_parm_no == 0))
		/* Although type matches - the node is not a constant
		** so the parent node can't be either. */
		const_cand = FALSE;
	}

	pop[npars] = &ops[npars];

	/*
	** (QUEL) If we do end up folding the constant operand(s) into a single
	** constant to replace the operator, we need to fill in a value for
	** the flag pst_pmspec on the replacement constant. The flag can
	** take the values (in logical though not integer order) PST_PMNOTUSED,
	** PST_PMMAYBE, PST_PMUSED. We keep a high-water mark in this order
	** as the values are found in the constant operand(s).
	*/
	if (lang == DB_QUEL && const_cand && lqnode->pst_sym.pst_type == PST_CONST)
	{
	    PST_PMSPEC cnst_pmspec = lqnode->pst_sym.pst_value.pst_s_cnst.pst_pmspec;

	    if (cnst_pmspec != PST_PMNOTUSED && cnst_pmspec != pmspec &&
		pmspec != PST_PMUSED)
	    {
		pmspec = cnst_pmspec;
	    }
	}

	npars++;
	if (opnode->pst_sym.pst_type == PST_MOP)
	{
	    /* Chain on the left, actual args on the right */
	    resqnode = resqnode->pst_left;
	    if (!resqnode)
	    {
		/* See comment regarding problem with PST_OPERAND below */
		opnode->pst_sym.pst_value.pst_s_op.pst_oplcnvrtid = (ADI_FI_ID)0;
		break;
	    }
	    lqnode = resqnode->pst_right;
	    /* At this point we hit a problem. Ideally we want to say:
	    **	pcnvrtid = &resqnode->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid;
	    ** But we can't (yet). The reason being that resqnode is pointing
	    ** at a PST_OPERAND which presently has no pst_value present and hence
	    ** the reference is invalid. Until such time as PST_OPERAND gets its
	    ** own PST_OP_NODE body, we set pcnvrtid to point as follows to the left
	    ** (yes left, the OPERAND chain) of the opnode and clear it before
	    ** returning. This is safe as it will only affect MOP nodes (very few)
	    ** and non deal with dtfamily types at present.
	    */
	    pcnvrtid = &opnode->pst_sym.pst_value.pst_s_op.pst_oplcnvrtid;
	}		
	else
	{
	    if (leftvisited)
		break;
	    leftvisited = TRUE;
	    lqnode = opnode->pst_right;    	    
	    pcnvrtid = &opnode->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid;
	}
    }

    /* Now figure out the result length of the operator being resolved */
    status = adi_0calclen(adf_scb, &best_fidesc->adi_lenspec, children, pop,
			&opnode->pst_sym.pst_dataval);
	
    /* Determine result db_collID. Chooses left operand collation ID
    ** unless right collID >= 0 and left was not set (not sure about this -
    ** it should actually be an error if they are both explicitly 
    ** declared, >0 and not equal). */
    if (tempop1.db_datatype == best_fidesc->adi_dtresult ||
	tempop1.db_datatype == -best_fidesc->adi_dtresult)
	opnode->pst_sym.pst_dataval = tempop1;
    else if (opnode->pst_left)
    {
	opnode->pst_sym.pst_dataval.db_collID = 
			opnode->pst_left->pst_sym.pst_dataval.db_collID;
	if (opnode->pst_right)
	{
	    if (!DB_COLL_UNSET(opnode->pst_right->pst_sym.pst_dataval) &&
		DB_COLL_UNSET(opnode->pst_sym.pst_dataval))
		opnode->pst_sym.pst_dataval.db_collID =
			opnode->pst_right->pst_sym.pst_dataval.db_collID;
	}
    }

    if (status != E_DB_OK	
	&& adf_scb->adf_errcb.ad_errcode != E_AD2022_UNKNOWN_LEN)
    {
	error->err_code = E_PS0C05_BAD_ADF_STATUS;
	return (E_DB_SEVERE);
    }

#   ifdef PSF_FOLD_DEBUG
    Psf_nops++;
#   endif
    /* Constant folding of opnode:
    ** Assuming the node is a candidate for folding, we will need
    ** to update the parse tree with the new value. It will not be
    ** a PST_CONST node but its length will need to be large enough
    ** to be melded to such. From the checks at the head of this
    ** routine, we know it will be an operator node and so long
    ** as sizeof(PST_CNST_NODE) <= sizeof(PST_OP_NODE) there
    ** will be space to update inplace.*/

    /* Don't attempt to constant-fold a LONG datatype.  The resulting
    ** value (a coupon) doesn't survive through to the query plan,
    ** since it's in a temporary LONG holding tank, and execution
    ** time blows up.
    */
    status = adi_dtinfo(adf_scb, best_fidesc->adi_dtresult, &dtbits);
    if (status == E_DB_OK && dtbits & AD_PERIPHERAL)
	const_cand = FALSE;

    if (Psf_fold &&
	sess_cb &&
	const_cand &&
	best_fidesc->adi_fitype != ADI_AGG_FUNC &&
	sizeof(PST_CNST_NODE) <= sizeof(PST_OP_NODE))
    {
	ADF_FN_BLK func_blk;
	DB_ERROR err_blk;
	/*
	** Set up function block to convert the constant argument.
	** Then call ADF to execute it
	*/
	/* Set up the parameters */
	func_blk.adf_dv_n = npars;
	for (i = 0; i < npars; i++)
	    func_blk.adf_dv[i+1] = ops[i];

	/* Set up the result value */
	func_blk.adf_dv[0] = opnode->pst_sym.pst_dataval;
	/* Allocate space for the destination data */
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
			    func_blk.adf_dv[0].db_length,
			    (PTR *)&func_blk.adf_dv[0].db_data, &err_blk);
	if (!status)
	{
	    /* To control math exceptions, we tell the PSF exception handler
	    ** to call the ADF exception handler to handle exceptions and
	    ** we arrange to have these just to continue with a warning */
	    i2 save_exc_state = adf_scb->adf_exmathopt;

	    sess_cb->pss_stmt_flags |= PSS_CALL_ADF_EXCEPTION;
	    adf_scb->adf_exmathopt = ADX_IGNALL_MATHEX;
	    adf_scb->adf_errcb.ad_errcode = 0;

	    /* Set up the function id etc. and execute it */
	    func_blk.adf_fi_desc = best_fidesc;
	    func_blk.adf_fi_id = opnode->pst_sym.pst_value.pst_s_op.pst_opinst;
	    func_blk.adf_pat_flags = opnode->pst_sym.pst_value.pst_s_op.pst_pat_flags;
	    func_blk.adf_escape = opnode->pst_sym.pst_value.pst_s_op.pst_escape;

	    if (best_fidesc->adi_fiflags & ADI_F16384_CHKVARFI)
	    {
		u_i1 save = adf_scb->adf_const_const;
		adf_scb->adf_const_const = 0;
		status = adf_func(adf_scb, &func_blk);
		/* If any flags set (AD_CNST_DSUPLMT or AD_CNST_USUPLMT
		** then this was not constant - stop it folding. */
		if (adf_scb->adf_const_const)
		    status = -1;
		adf_scb->adf_const_const = save;
	    }
	    else
		status = adf_func(adf_scb, &func_blk);

	    /* Reset the exception flag and state */
	    sess_cb->pss_stmt_flags &= ~PSS_CALL_ADF_EXCEPTION;
	    adf_scb->adf_exmathopt = save_exc_state;
	}

	if (!status && !adf_scb->adf_errcb.ad_errcode)
	{
#	    ifdef PSF_FOLD_DEBUG
	    Psf_nfolded++;
#	    endif
	    /* Update the parse tree with the new value:
	    ** Clear left and right sub-trees */
	    opnode->pst_left = NULL;
	    opnode->pst_right = NULL;
	    /* Copy data descriptor contents */
	    opnode->pst_sym.pst_dataval = func_blk.adf_dv[0];
	    /* Make into a constant node and adjust length */
	    opnode->pst_sym.pst_type = PST_CONST;
	    opnode->pst_sym.pst_len = sizeof(PST_CNST_NODE) + 
			    opnode->pst_sym.pst_dataval.db_length;
	    opnode->pst_sym.pst_value.pst_s_cnst.pst_tparmtype = PST_USER;
	    opnode->pst_sym.pst_value.pst_s_cnst.pst_parm_no = 0;
	    opnode->pst_sym.pst_value.pst_s_cnst.pst_pmspec = pmspec;
	    opnode->pst_sym.pst_value.pst_s_cnst.pst_cqlang = adf_scb->adf_qlang;
	    opnode->pst_sym.pst_value.pst_s_cnst.pst_origtxt = NULL;
	}
    }
    return    (E_DB_OK);
}


/*{
** Name: pst_union_resolve	- Resolve a query tree union
**
** Description:
**      The pst_union_resolve function resolves a query tree union.  This
**	means that it sets result type and length for the nodes.  It also
**	figures out which ADF function should be called for the nodes.  This
**	operation should be called whenever a union is added to the select. 
**	Any query tree that comes from the parser is guaranteed to have already
**	been resolved.
**
**	NOTE: This function does not set the conversion ids for an operator
**	node.  That it because the operator node could receive new children
**	of different types.  The conversions must be set before the expression
**	is used.
**
** Inputs:
**	sess_cb				session control block
**      rootnode			The root node pointing to the union
**      error                           Error block
**
** Outputs:
**      opnode				The resolved operator node
**	error				Standard error block
**	    .err_code			    What error occurred
**		E_PS0000_OK			Success
**		E_PS0002_INTERNAL_ERROR		Internal inconsistency in PSF
**		E_PS0C02_NULL_PTR		rootnode = NULL or rootnode
**						-->pst_union == NULL
**		E_PS0C03_BAD_NODE_TYPE		rootnode is not a root node
**		E_PS0C04_BAD_TREE		Something is wrong with the
**						query tree.
**		2917				Unequal number of resdoms
**						in union.
**		2918				resdom types not compatible
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	4/3/87 (daved)
**	    written
**	23-oct-87 (stec)
**	    Cleanup, do resolution for n subselects, not just two.
**	18-jun-89 (jrb)
**	    This routine now calls ADF to do datatype resolution instead of
**	    taking knowledge of how it should be done.
**	12-jul-89 (jrb)
**	    Made E_AD2061_BAD_OP_COUNT map to a user error rather than letting
**	    PSF treat it as an internal error.
**	01-aug-89 (jrb)
**	    Set precision field for all db_data_values contained in here.  Also,
**	    added rules for doing UNIONs on decimal resdoms.
**	18-aug-89 (jrb)
**	    Fixed problem where error printing code was AVing because it was
**	    using the wrong fi pointer to get the dt name from ADF.
**	30-oct-89 (jrb)
**	    Can't initialize structs on the stack on Unix although it works on
**	    VMS.  Fixed this.
**	15-dec-89 (jrb)
**	    Added checks for DB_ALL_TYPE in function instance descriptors.
**	14-mar-94 (andre)
**	    moved code propagating type information of RESDOM nodes in the first
**	    element of UNION into RESDOM nodes of the subsequent elements
**	15-apr-05 (inkdo01)
**	    Add collation support.
**      09-sep-05 (zhahu02)
**          Updated to be compatible with long nvarchar (INGSRV3411/b115179).
**	15-Oct-2008 (kiria01) b121061
**	    Increase the number of parameters supported in the calls
**	    to adf_func using the more general vector form.
**	19-Jan-2009 (kiria01) b121515
**	    Generate user error if an attempt to UNION NULLs would result in
**	    a typeless NULL.
*/
DB_STATUS
pst_union_resolve(
	PSS_SESBLK  *sess_cb,
	PST_QNODE   *rootnode,
	DB_ERROR    *error)
{
    ADF_CB		*adf_scb = (ADF_CB *)sess_cb->pss_adfcb;
    DB_STATUS		status;
    register PST_QNODE	*res1;
    register PST_QNODE	*res2;
    register PST_QNODE	*ss;
    i4		err_code;
    bool		forcenull;
    DB_DATA_VALUE	*dv;

    /* Make sure there is a node to resolve */
    if (rootnode == (PST_QNODE *) NULL || 
	rootnode->pst_sym.pst_value.pst_s_root.pst_union.pst_next
						     == (PST_QNODE*) NULL)
    {
	error->err_code = E_PS0C02_NULL_PTR;
	return (E_DB_SEVERE);
    }

    /* Make sure it's a root node */
    if (rootnode->pst_sym.pst_type != PST_ROOT)
    {
	error->err_code = E_PS0C03_BAD_NODE_TYPE;
	return (E_DB_SEVERE);
    }

    /*
    ** In general case, a query expression may have n subselects. We need to
    ** look at data types of the resdoms across subselects (i.e., resdom no. =
    ** const), and determine if they can be coerced into a common datatype, and
    ** if there is more than one, choose the best one. Rules for choosing the
    ** best one are same as in case of other data type coercions. Upon return
    ** the target list of the first subselect shall contain the appropriate
    ** datatype info.  The algorithm is to process two target lists at a time.
    ** In the first run it will be done for the target list of the first and
    ** second subselects, then first and nth subselect, where n = 3, 4, ...
    */

    /* traverse each subselect */
    for (ss = rootnode->pst_sym.pst_value.pst_s_root.pst_union.pst_next; ss;
	 ss = ss->pst_sym.pst_value.pst_s_root.pst_union.pst_next
	)
    {
	/* walk down list of resdoms for 1st and nth subselect */
	for (res1 = rootnode->pst_left, res2 = ss->pst_left;
	     res1 || res2;
	     res1 = res1->pst_left, res2 = res2->pst_left
	    )
	{
	    /* if not both resdoms or tree ends, then error */
	    if (!res1 || !res2 || 
		(res1->pst_sym.pst_type != res2->pst_sym.pst_type)
	       )
	    {
		(VOID) psf_error(2917L, 0L, PSF_USERERR, &err_code, error, 1,
		    sizeof (sess_cb->pss_lineno), &sess_cb->pss_lineno);
		return (E_DB_ERROR);
	    }

	    if (res1->pst_sym.pst_type == PST_TREE)
		break;

	    /* Make sure it's a resdom node */
	    if (res1->pst_sym.pst_type != PST_RESDOM)
	    {
		error->err_code = E_PS0C03_BAD_NODE_TYPE;
		return (E_DB_SEVERE);
	    }

	    /* Coalesce the collation IDs (if any). If nth is > 0,
	    ** copy to 1st. Later, 1st will be propagated to the rest.
	    ** NOTE: 2 collations > 0 but different should probably be 
	    ** an error, but not just yet since they're probably unicode
	    ** and unicode_case_insensitive which we tolerate. */
	    if (!DB_COLL_UNSET(res2->pst_sym.pst_dataval) &&
		res2->pst_sym.pst_dataval.db_collID > DB_NOCOLLATION &&
		res1->pst_sym.pst_dataval.db_collID != DB_UNICODE_CASEINSENSITIVE_COLL)
		res1->pst_sym.pst_dataval.db_collID = 
		    res2->pst_sym.pst_dataval.db_collID;

	    /* Check for select resdoms being constant NULL */
	    forcenull = FALSE;
	    if (res1->pst_right->pst_sym.pst_type == PST_CONST
	      && (dv = &res1->pst_right->pst_sym.pst_dataval)->db_datatype == -DB_LTXT_TYPE
	      && dv->db_data != NULL
	      && *(u_char *)(dv->db_data + dv->db_length - 1) != 0)
	    {
		forcenull = TRUE;
	    }
	    if (!forcenull
	      && res2->pst_right->pst_sym.pst_type == PST_CONST
              && (dv = &res2->pst_right->pst_sym.pst_dataval)->db_datatype == -DB_LTXT_TYPE
              && dv->db_data != NULL
	      && *(u_char *)(dv->db_data + dv->db_length - 1) != 0)
            {
		forcenull = TRUE;
            }

	    status = pst_caseunion_resolve(sess_cb,
			&res1->pst_sym.pst_dataval,
			&res2->pst_sym.pst_dataval,
			forcenull, error);
	    if (status != E_DB_OK)
		return (status);
	        
	} /* for each resdom in both analyzed target lists */
    } /* for each subselect in union loop */

    /* Propagate the resdom data type to the subselect being added. */
    for (ss = rootnode->pst_sym.pst_value.pst_s_root.pst_union.pst_next;
         ss;
	 ss = ss->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
    {
	for (res1 = rootnode->pst_left, res2 = ss->pst_left; 
	    res1->pst_sym.pst_type == PST_RESDOM;
	    res1 = res1->pst_left, res2 = res2->pst_left)
	{
	    if ((res2->pst_sym.pst_dataval.db_datatype 
		= res1->pst_sym.pst_dataval.db_datatype) == -DB_LTXT_TYPE)
	    {
		(VOID) psf_error(2183L, 0L, PSF_USERERR, &err_code, error, 1,
		    sizeof (sess_cb->pss_lineno), &sess_cb->pss_lineno);
		return (E_DB_ERROR);
	    }
	    res2->pst_sym.pst_dataval.db_prec
		= res1->pst_sym.pst_dataval.db_prec;
	    res2->pst_sym.pst_dataval.db_length
		= res1->pst_sym.pst_dataval.db_length;
	    res2->pst_sym.pst_dataval.db_collID
		= res1->pst_sym.pst_dataval.db_collID;
	}
    }

    return    (E_DB_OK);
}

/*{
** Name: pst_caserslt_resolve	- Resolve the set of result expressions
**				in a case function
**
** Description:
**      The pst_caserslt_resolve function resolves the result expressions of
**	a case function. That is, it determines the common data type for
**	all result expressions and sets it into their respective subtrees
**	(and the case node itself).
**
** Inputs:
**	sess_cb				session control block
**      casep				The case function root node
**      error                           Error block
**
** Outputs:
**      opnode				The resolved operator node
**	error				Standard error block
**	    .err_code			    What error occurred
**		E_PS0000_OK			Success
**		E_PS0002_INTERNAL_ERROR		Internal inconsistency in PSF
**		E_PS0C02_NULL_PTR		rootnode = NULL or rootnode
**						-->pst_union == NULL
**		E_PS0C03_BAD_NODE_TYPE		rootnode is not a root node
**		E_PS0C04_BAD_TREE		Something is wrong with the
**						query tree.
**		2933				Result expressions not coercible
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	3-sep-99 (inkdo01)
**	    Cloned from pst_union_resolve for case function implementation.
**      23-May-2002 (zhahu02)
**          Modified with the decimal type for case function implementation
**          (b107866/INGSRV1786).
**	07-Jan-2003 (hanch04)
**	    Back out last change.  Not compatible with new code.
**	8-nov-2006 (dougi)
**	    Assign result type to "then" results, too, to assure correct
**	    coercions.
**	13-nov-2006 (dougi)
**	    Oops - that one broke more than it fixed - now removed.
**	22-Mar-07 (wanfr01)
**	    Bug 117979
**	    Need third parameter for adi_resolve
**	3-apr-2007 (dougi)
**	    Permit NULL result from THEN or ELSE.
*/
static DB_STATUS
pst_caserslt_resolve(
	PSS_SESBLK  *sess_cb,
	PST_QNODE   *casep,
	DB_ERROR    *error)
{
    ADF_CB		*adf_scb = (ADF_CB *)sess_cb->pss_adfcb;
    DB_STATUS		status;
    register PST_QNODE	*res;
    PST_QNODE		*whlistp, *whop1p, *whop2p;
    DB_DATA_VALUE	*dv;
    i4			err_code;
    bool		forcenull, else_seen;

    /* Make sure there is a node to resolve */
    if (casep == (PST_QNODE *) NULL ||
	(whlistp = casep->pst_left) == (PST_QNODE *)NULL ||
	(whop1p = whlistp->pst_right) == (PST_QNODE *)NULL)
    {
	error->err_code = E_PS0C02_NULL_PTR;
	return (E_DB_SEVERE);
    }

    /* Make sure it's a case function. */
    if (casep->pst_sym.pst_type != PST_CASEOP ||
	whlistp->pst_sym.pst_type != PST_WHLIST ||
	whop1p->pst_sym.pst_type != PST_WHOP)
    {
	error->err_code = E_PS0C03_BAD_NODE_TYPE;
	return (E_DB_SEVERE);
    }

    /* Start by copying first result expression type to "case" node. */
    res = whop1p->pst_right;
    casep->pst_sym.pst_dataval.db_datatype = 
	    res->pst_sym.pst_dataval.db_datatype;
    casep->pst_sym.pst_dataval.db_prec =
	    res->pst_sym.pst_dataval.db_prec;
    casep->pst_sym.pst_dataval.db_collID =
	    res->pst_sym.pst_dataval.db_collID;
    casep->pst_sym.pst_dataval.db_length =
	    res->pst_sym.pst_dataval.db_length;

    /* Check for stupid "case" with one "when" and no "else". */
    if (whlistp->pst_left == (PST_QNODE *)NULL)
    {
	/* Since there's no else, by definition it has to be nullable! */
	if (casep->pst_sym.pst_dataval.db_datatype > 0)
	{
	    casep->pst_sym.pst_dataval.db_datatype =
			- casep->pst_sym.pst_dataval.db_datatype;
	    ++ casep->pst_sym.pst_dataval.db_length;
	}
	return(E_DB_OK);
    }

    /* Check for first result expr being constant NULL */
    forcenull = FALSE;
    if (res->pst_sym.pst_type == PST_CONST
      && (dv = &res->pst_sym.pst_dataval)->db_datatype == -DB_LTXT_TYPE
      && dv->db_data != NULL
      && *(u_char *)(dv->db_data + dv->db_length - 1) != 0)
    {
	forcenull = TRUE;
    }

    /*
    ** The algorithm is simply to resolve the first result expression
    ** (copied to the case node) in turn with each remaining result
    ** expression. This will produce the final result type in the case node.
    */

    /* Walk down when-list to process remaining result exprs. */
    else_seen = FALSE;
    for (whlistp = whlistp->pst_left;
	whlistp && whlistp->pst_sym.pst_type == PST_WHLIST &&
	(whop2p = whlistp->pst_right) && (res = whop2p->pst_right);
	whlistp = whlistp->pst_left) 
    {
	/* Check for next result expr being constant NULL */
	if (!forcenull
	  && res->pst_sym.pst_type == PST_CONST
	  && (dv = &res->pst_sym.pst_dataval)->db_datatype == -DB_LTXT_TYPE
	  && dv->db_data != NULL
	  && *(u_char *)(dv->db_data + dv->db_length - 1) != 0)
	{
	    forcenull = TRUE;
	}

	status = pst_caseunion_resolve(sess_cb,
			&casep->pst_sym.pst_dataval,
			&res->pst_sym.pst_dataval,
			forcenull, error);
	if (status != E_DB_OK)
	    return (status);

	else_seen = (whop2p->pst_left == NULL);
	forcenull = FALSE;		/* For next trip thru */
    } /* for each "when" in case function */

    /* If no ELSE, make sure we ended up nullable */
    if (!else_seen && casep->pst_sym.pst_dataval.db_datatype > 0)
    {
	casep->pst_sym.pst_dataval.db_datatype = 
			-casep->pst_sym.pst_dataval.db_datatype;
	casep->pst_sym.pst_dataval.db_length++;
    }

    return    (E_DB_OK);
}

/*
** Name: pst_caseunion_resolve - Resolve types for CASE or UNION
**
** Description:
**
**	Resolve two data values into the "highest" data type,
**	storing the resulting type into the first one.
**
**	If one or the other of the inputs is explicitly the constant
**	NULL, all we need to do is take the other value's type info,
** 	as nullable.  Since there are a few different ways this can
**	occur (e.g. a CASE with no ELSE), we'll let the caller figure
**	it out and flag it.
**
** Inputs:
**	sess_cb			PSS_SESBLK parser session control block
**	dv1			The best-so-far data value
**	dv2			The other data value to resolve
**	forcenull		TRUE if just make nullable
**	error			Returned error info if errors
**
** Outputs:
**	May update dv1 with new type info
**	Returns E_DB_OK or error
**
** History:
**	3-Nov-2006 (kschendel)
**	    Extract from union and CASE resolvers, in an attempt to fix
**	    up problems with "case when p then decimal() else null".
**	22-Mar-07 (wanfr01)
**	    Bug 117979
**	    Need third parameter for adi_resolve
**	21-Mar-2008 (kiria01) b120144
**	    Apply dtfamily processing to resolve ANSI date coersion.
*/

static DB_STATUS
pst_caseunion_resolve(PSS_SESBLK *sess_cb,
	DB_DATA_VALUE *dv1, DB_DATA_VALUE *dv2,
	bool forcenull, DB_ERROR *error)

{

    ADF_CB *adf_scb = (ADF_CB *)sess_cb->pss_adfcb;
    ADI_FI_DESC *best_fidesc;
    ADI_FI_DESC *new_fidesc = NULL;
    ADI_RSLV_BLK adi_rslv_blk;
    bool nullable;
    DB_DT_ID temp_dt;
    DB_STATUS status;
    i4 err_code;

    /* Coalesce the collation IDs (if any). If nth is > 0,
    ** copy to 1st. Later, 1st will be propagated to the rest.
    ** NOTE: 2 collations > 0 but different should probably be 
    ** an error, but not just yet since they're probably unicode
    ** and unicode_case_insensitive which we tolerate. */
    if (!DB_COLL_UNSET(*dv2) &&
	dv2->db_collID > DB_NOCOLLATION &&
	dv1->db_collID != DB_UNICODE_CASEINSENSITIVE_COLL)
	dv1->db_collID = dv2->db_collID;

    /* If we just need to force nullability, do so.  Assume that we'll use
    ** the datatype of the first operand unless it's internal-text, in which
    ** case use the second operand.
    */
    if (forcenull)
    {
	if (abs(dv1->db_datatype) == DB_LTXT_TYPE)
	{
	    dv1->db_datatype = dv2->db_datatype;
	    dv1->db_prec = dv2->db_prec;
	    dv1->db_length = dv2->db_length;
	    dv1->db_collID = dv2->db_collID;
	}
	if (dv1->db_datatype > 0)
	{
	    dv1->db_datatype = -dv1->db_datatype;
	    ++ dv1->db_length;
	}
	return (E_DB_OK);
    }

    /* ask ADF to find a common datatype for the current resdoms */
    adi_rslv_blk.adi_op_id   = ADI_FIND_COMMON;
    adi_rslv_blk.adi_num_dts = 2;
    adi_rslv_blk.adi_dt[0]     = dv1->db_datatype;
    adi_rslv_blk.adi_dt[1]     = dv2->db_datatype;

    status = adi_resolve(adf_scb, &adi_rslv_blk, FALSE);

    if (status != E_DB_OK)
    {
	ADI_DT_NAME leftparm, rightparm;	/* For error reporting */

	/* Complain if no applicable function found */
	if (	adf_scb->adf_errcb.ad_errcode == E_AD2062_NO_FUNC_FOUND
	    ||	adf_scb->adf_errcb.ad_errcode == E_AD2061_BAD_OP_COUNT
	    ||	adf_scb->adf_errcb.ad_errcode == E_AD2009_NOCOERCION
	    ||  adf_scb->adf_errcb.ad_errcode == E_AD2063_FUNC_AMBIGUOUS)
	{
	    i4 err = 2918;

	    if (adf_scb->adf_errcb.ad_errcode == E_AD2063_FUNC_AMBIGUOUS)
		err = 2919;

	    status = adi_tyname(adf_scb,
				    adi_rslv_blk.adi_dt[0],
				    &leftparm);
	    if (status != E_DB_OK)
	    {
		if (adf_scb->adf_errcb.ad_errclass == ADF_USER_ERROR)
		{
		    (VOID) psf_error(E_PS0C04_BAD_TREE,
			adf_scb->adf_errcb.ad_usererr, PSF_INTERR,
			&err_code, error, 0);
		}
		else
		{
		    (VOID) psf_error(E_PS0C04_BAD_TREE,
			adf_scb->adf_errcb.ad_errcode, PSF_INTERR,
			&err_code, error, 0);
		}
		return (E_DB_SEVERE);
	    }
	    status = adi_tyname(adf_scb,
				    adi_rslv_blk.adi_dt[1],
				    &rightparm);
	    if (status != E_DB_OK)
	    {
		if (adf_scb->adf_errcb.ad_errclass == ADF_USER_ERROR)
		{
		    (VOID) psf_error(E_PS0C04_BAD_TREE,
			adf_scb->adf_errcb.ad_usererr, PSF_INTERR,
			&err_code, error, 0);
		}
		else
		{
		    (VOID) psf_error(E_PS0C04_BAD_TREE,
			adf_scb->adf_errcb.ad_errcode, PSF_INTERR,
			&err_code, error, 0);
		}
		return (E_DB_SEVERE);
	    }
	    (VOID) psf_error(err, 0L, PSF_USERERR, 
		&err_code, error, 3, 
		sizeof (sess_cb->pss_lineno), &sess_cb->pss_lineno,
		psf_trmwhite(sizeof (ADI_DT_NAME), (char *) &leftparm), 
		&leftparm, 
		psf_trmwhite(sizeof (ADI_DT_NAME), (char *) &rightparm),
		&rightparm);

	    return (E_DB_ERROR);
	}

	error->err_code = E_PS0C05_BAD_ADF_STATUS;
	return (E_DB_ERROR);
    }

    /* we now have the "best" fi descriptor */
    if (best_fidesc = adi_rslv_blk.adi_fidesc)
    {
	i2	need_fmly_resolve = 0;

	status = adi_need_dtfamily_resolve( adf_scb, best_fidesc, 
					    &adi_rslv_blk, 
					    &need_fmly_resolve);
	if (status)
	{
	    error->err_code = E_PS0C05_BAD_ADF_STATUS;
	    return (E_DB_SEVERE);
	}

	if (sess_cb && need_fmly_resolve == 1) 
	{
	    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
		sizeof(ADI_FI_DESC), &new_fidesc, error);
	    if (status != E_DB_OK)
	    {
		return (status);
	    }
	    if  (new_fidesc == NULL)
	    {
		error->err_code = E_PS0C05_BAD_ADF_STATUS;
		return (E_DB_SEVERE);
	    }

	    status = adi_dtfamily_resolve_union(adf_scb, best_fidesc, 
					    new_fidesc, &adi_rslv_blk);
	    if (status != E_DB_OK)
	    {
		return (status);
	    }

	    best_fidesc = new_fidesc; 
	}
    }

    /*
    ** In some cases coercion per se may not be needed, but
    ** data type may have to be changed to nullable.
    */

    status = adi_res_type(adf_scb, best_fidesc,
		adi_rslv_blk.adi_num_dts, adi_rslv_blk.adi_dt,
		&temp_dt);
		
    nullable = (bool) (temp_dt < 0);

    /* update type info of left node, if needed */
    status = pst_get_union_resdom_type(best_fidesc->adi_dt[0],
		dv1, adf_scb, nullable, error);
    if (DB_FAILURE_MACRO(status))
	return(status);
    
    /* if either type is long byte/varchar, must short circuit the
    ** resolution process here. Since long's have no "=" ops,
    ** adi_resolve (way back there) produced the actual coercion 
    ** fi. Since there's only one operand for coercions, the
    ** rest of this stuff needs to be bypassed */

    if (best_fidesc->adi_dt[0] != DB_LVCH_TYPE && 
	  best_fidesc->adi_dt[0] != DB_LBYTE_TYPE &&
	  best_fidesc->adi_dt[0] != DB_LNVCHR_TYPE)
    {                             /* not "long" - keep going */
	DB_DATA_VALUE res_dv;
	
	/* update type info of right node, if needed;
	** do into temp, keep dv2 untouched.
	*/
	STRUCT_ASSIGN_MACRO(*dv2, res_dv);
	status = pst_get_union_resdom_type(best_fidesc->adi_dt[1],
		&res_dv, adf_scb, nullable, error);
	if (DB_FAILURE_MACRO(status))
	    return(status);

       /*
	** For decimal datatype we use the ADI_DECBLEND lenspec to get the
	** result precision, scale, and length; for other types, just
	** take the maximum length.  The resulting precision/scale and
	** length will be placed in dv1 which will end up with the values
	** that everything else will need to be coerced into.
	*/
	if (abs(dv1->db_datatype) == DB_DEC_TYPE)
	{
	    DB_DATA_VALUE	    *dv[2];
	    DB_DATA_VALUE	    tmp;
	    ADI_LENSPEC	    lenspec;

	    tmp.db_datatype = DB_DEC_TYPE;
	    tmp.db_data = NULL;

	    lenspec.adi_lncompute	= ADI_DECBLEND;
	    lenspec.adi_fixedsize	= 0;
	    lenspec.adi_psfixed	= 0;
	    dv[0] = dv1;
	    dv[1] = &res_dv;
	
	    status = adi_0calclen(adf_scb, &lenspec, 2, dv, &tmp);
	    if (status != 0)
	    {
		error->err_code = E_PS0C05_BAD_ADF_STATUS;
		return (E_DB_SEVERE);
	    }

	    dv[0]->db_prec = tmp.db_prec;
	    dv[0]->db_length = tmp.db_length;
	    dv[0]->db_collID = DB_UNSET_COLL;

	    if (nullable)
		dv[0]->db_length++;
	}
	else
	{
	    if (dv1->db_length < res_dv.db_length)
	    {
		dv1->db_length = res_dv.db_length;
	    }
	}
    }

    return (E_DB_OK);
} /* pst_caseunion_resolve */

/*
** Name:    pst_get_union_resdom_type - determine type info of a resdom node
**
** Description:
**	Using result of adi_resolve() and adi_res_type(), determine type info of
**	a resdom node of a UNION select
**
** Input:
**	dt		result of resolving datatypes of left and right RESDOM
**			nodes
**	resval		ptr to DB_DATA_VALUE of RESDOM node whose type info needs
**			to be computed
**	adf_scb		ADF session CB
**	nullable	indicates whether the common datatype should be nullable
**
** Output:
**	error
**	    err_code	filled in if an error occurred
**
** Returns:
**	E_DB_{OK,STATUS}
**
** History:
**	14-mar-94 (andre)
**	    extracted from the body of pst_union_resolve()
**	8-sep-99 (inkdo01)
**	    Changed 2nd parm to ptr to pst_dataval from ptr to node (to make 
**	    case funcs a bit easier to code)
*/
static DB_STATUS
pst_get_union_resdom_type(
			  DB_DT_ID	dt,
			  DB_DATA_VALUE	*resval,
			  ADF_CB	*adf_scb,
			  bool		nullable,
			  DB_ERROR	*error)
{
    DB_STATUS	    status;
    ADI_FI_ID	    convid;
    ADI_FI_DESC	    *fi_desc;
    DB_DATA_VALUE   resop;
    DB_DATA_VALUE   *opptrs[ADI_MAX_OPERANDS];

    /* Get coercion id for the resdom, if needed */
    if (dt != abs(resval->db_datatype) && dt != DB_ALL_TYPE)
    {
	status = adi_ficoerce(adf_scb, resval->db_datatype,
	    dt, &convid);
	if (status != E_DB_OK)
	{
	    error->err_code = E_PS0C05_BAD_ADF_STATUS;
	    return(E_DB_SEVERE);
	}

	/* Look up coercion in function instance table */
	status = adi_fidesc(adf_scb, convid, &fi_desc);
	if (status != E_DB_OK)
	{
	    error->err_code = E_PS0C05_BAD_ADF_STATUS;
	    return (E_DB_SEVERE);
	}

	resop.db_datatype = (nullable) ? -dt : dt;

	opptrs[0] = resval;
	/* Get length after coercion */
	status = adi_0calclen(adf_scb, &fi_desc->adi_lenspec, 1,
	    opptrs, &resop);
	if (   status != E_DB_OK
	    && adf_scb->adf_errcb.ad_errcode != E_AD2022_UNKNOWN_LEN)
	{
	    error->err_code = E_PS0C05_BAD_ADF_STATUS;
	    return(E_DB_SEVERE);
	}

	/* Remember the result length of the coercion */
	resval->db_prec = resop.db_prec;
	resval->db_collID = DB_UNSET_COLL;
	resval->db_length = resop.db_length;
	resval->db_datatype = resop.db_datatype;
    }
    /* Convert from non-nullable to nullable */
    else if (resval->db_datatype > 0 && nullable)
    {
	resval->db_datatype = -resval->db_datatype;
	resval->db_length++;
    }

    return(E_DB_OK);
}

/*{
** Name: pst_parm_resolve   - Resolve a resdom node representing
**			      local var/parm.
**
** Description:
**      The function resolves datatypes for a resdom node representing
**	a local variable or a parameter. The datatype of the resdom node
**	must not actually change, it's the right child that needs adjustment.
**
** Inputs:
**
** Outputs:
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-jun-88 (stec)
**	    written
**	03-nov-88 (stec)
**	    Correct bug found by lint.
*/
DB_STATUS
pst_parm_resolve(
	PSS_SESBLK  *sess_cb,
	PSQ_CB	    *psq_cb,
	PST_QNODE   *resdom)
{
    ADF_CB	    *adf_scb = (ADF_CB *)sess_cb->pss_adfcb;
    ADI_DT_BITMASK  mask;
    DB_STATUS	    status;
    i4	    err_code;
    DB_DT_ID	    rdt, cdt;
    ADI_DT_NAME	    rdtname, cdtname;
    ADI_FI_ID	    convid;
    

    /* Make sure there is a node to resolve */
    if (resdom == (PST_QNODE *) NULL || 
	resdom->pst_right == (PST_QNODE*) NULL
       )
    {
	return (E_DB_OK);
    }

    /* Make sure it's a resdom node */
    if (resdom->pst_sym.pst_type != PST_RESDOM)
    {
	(VOID) psf_error(E_PS0C03_BAD_NODE_TYPE, 0L,
	    PSF_INTERR, &err_code, &psq_cb->psq_error, 0);
	return (E_DB_SEVERE);
    }

    rdt = resdom->pst_sym.pst_dataval.db_datatype;
    cdt = resdom->pst_right->pst_sym.pst_dataval.db_datatype;

    /* See what the right child is coercible into. */
    status = adi_tycoerce(adf_scb, abs(cdt), &mask);

    if (DB_FAILURE_MACRO(status))
    {
	if (adf_scb->adf_errcb.ad_errclass == ADF_USER_ERROR)
	{
	    (VOID) psf_error(E_PS0C05_BAD_ADF_STATUS,
		adf_scb->adf_errcb.ad_usererr, PSF_INTERR,
		&err_code, &psq_cb->psq_error, 0);
	}
	else
	{
	    (VOID) psf_error(E_PS0C05_BAD_ADF_STATUS,
		adf_scb->adf_errcb.ad_errcode, PSF_INTERR,
		&err_code, &psq_cb->psq_error, 0);
	}
	return(status);
    }

    /* See what the resdom data type is one of the options. */
    if (BTtest(ADI_DT_MAP_MACRO(abs(rdt)), (char *)&mask) == FALSE)
    {
	/* Conversion impossible. */

	status = adi_tyname(adf_scb, rdt, &rdtname);
	if (DB_FAILURE_MACRO(status))
	{
	    if (adf_scb->adf_errcb.ad_errclass == ADF_USER_ERROR)
	    {
		(VOID) psf_error(E_PS0C04_BAD_TREE,
		    adf_scb->adf_errcb.ad_usererr, PSF_INTERR,
		    &err_code, &psq_cb->psq_error, 0);
	    }
	    else
	    {
		(VOID) psf_error(E_PS0C04_BAD_TREE,
		    adf_scb->adf_errcb.ad_errcode, PSF_INTERR,
		    &err_code, &psq_cb->psq_error, 0);
	    }
	    return (E_DB_SEVERE);
	}

	status = adi_tyname(adf_scb, cdt, &cdtname);
	if (DB_FAILURE_MACRO(status))
	{
	    if (adf_scb->adf_errcb.ad_errclass == ADF_USER_ERROR)
	    {
		(VOID) psf_error(E_PS0C04_BAD_TREE,
		    adf_scb->adf_errcb.ad_usererr, PSF_INTERR,
		    &err_code, &psq_cb->psq_error, 0);
	    }
	    else
	    {
		(VOID) psf_error(E_PS0C04_BAD_TREE,
		    adf_scb->adf_errcb.ad_errcode, PSF_INTERR,
		    &err_code, &psq_cb->psq_error, 0);
	    }
	    return (E_DB_SEVERE);
	}

	(VOID) psf_error(2417L, 0L, PSF_USERERR, &err_code, 
	    &psq_cb->psq_error, 2,
	    psf_trmwhite(sizeof(ADI_DT_NAME), (char *) &cdtname), &cdtname,
	    psf_trmwhite(sizeof(ADI_DT_NAME), (char *) &rdtname), &rdtname); 

	return (E_DB_ERROR);
    }

    return    (E_DB_OK);
}

/*{
** Name: pst_handler	- Exception handler for local coercion errors
**
** Description:
**      This function is the exception handler just for coercion errors
**	locally, since the standard PSF exception handler simply shuts down
**	the session, while our errors here are handleable.
**
** Inputs:
**      exargs                          The exception handler args, as defined
**					by the CL spec.
**
** Outputs:
**      None
** Returns:
**	EXDECLARE
** Exceptions:
**	None
**
** Side Effects:
**	Can set the adf_cb errclass
**
** History:
**	26-sep-07 (kibro01)
**          written
*/
static STATUS
pst_handler(
	EX_ARGS            *exargs)
{
    STATUS		ret_val = EXRESIGNAL;
    PSS_SESBLK		*sess_cb;
    ADF_CB		*adf_cb = NULL;

    if (exargs->exarg_num == EXINTOVF || exargs->exarg_num == EXFLTOVF)
    {
	sess_cb = psf_sesscb();
	if (sess_cb)
	    adf_cb = sess_cb->pss_adfcb;
	/* Note that this is an error which can be reported back */
	if (adf_cb)
	{
	    adf_cb->adf_errcb.ad_errclass = ADF_USER_ERROR;
	    ret_val = EXCONTINUES;
	}
    }
    return(ret_val);
}

/*{
** Name: pst_convlit	- Convert constant literal between string & 
**				numeric
**
** Description:
**      In INSERT/UPDATE statements and binary expressions, if context
**	dictates that type mismatch can be overcome by converting a 
**	constant literal to the target type, we'll give it a whack.
**	This is integral to the Rpath request to support integer literals
**	in character columns and vice versa.
**
** Inputs:
**	sess_cb		- ptr to parser control block
**	stream		- ptr to memory stream for constant allocation
**	targdv		- ptr to target type description
**	srcenode	- ptr to source node
**
** Outputs:
**	Returns:
**	    FALSE			Conversion couldn't be done
**	    TRUE			Conversion was successful
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	13-sep-2006 (dougi)
**	    Written.
**	26-sep-2007 (kibro01) b119191
**	    Added exception so that if one of the coercion functions
**	    throws an exception we catch it here.
*/
bool
pst_convlit(
	PSS_SESBLK  *sess_cb,
	PSF_MSTREAM *stream,
	DB_DATA_VALUE *targdv,
	PST_QNODE   *srcenode)

{
    ADF_CB	*adf_cb = sess_cb->pss_adfcb;
    DB_STATUS	status;
    DB_ERROR	errblk;
    DB_DATA_VALUE convdv;
    char	*convptr;
    DB_DT_ID	targtype, srcetype;
    EX_CONTEXT	ex_context;


    /* Verify that source node is a PST_CONST literal. */
    if (srcenode->pst_sym.pst_type != PST_CONST ||
	srcenode->pst_sym.pst_value.pst_s_cnst.pst_tparmtype != PST_USER)
	return(FALSE);

    /* We have a literal - now see if it is coercible. Verify the 
    ** permissable combinations. */
    srcetype = abs(srcenode->pst_sym.pst_dataval.db_datatype);
    targtype = abs(targdv->db_datatype);

    switch (targtype) {
      case DB_INT_TYPE:
      case DB_FLT_TYPE:
      case DB_DEC_TYPE:
	if (srcetype != DB_CHA_TYPE && srcetype != DB_VCH_TYPE)
	    return(FALSE);
	break;

      case DB_CHA_TYPE:
      case DB_VCH_TYPE:
	if (srcetype != DB_INT_TYPE && srcetype != DB_FLT_TYPE &&
		srcetype != DB_DEC_TYPE)
	    return(FALSE);
	break;

      default:	/* All the rest won't work. */
	return(FALSE);
    }

    /* Passed the easy tests - allocate space for the converted literal. */
    status = psf_malloc(sess_cb, stream, targdv->db_length, 
	&convptr, &errblk);
    if (status != E_DB_OK)
	return(FALSE);

    STRUCT_ASSIGN_MACRO(*targdv, convdv);
    if (convdv.db_datatype < 0)
    {
	/* Convert to not null so's not to offend the coercion functions. */
	convdv.db_datatype = -convdv.db_datatype;
	convdv.db_length--;
    }

    convdv.db_data = convptr;

    if (EXdeclare(pst_handler, &ex_context) != OK)
    {
	EXdelete();
	return (FALSE);
    }

    /* Use switch to direct coercion to correct function. */
    switch(targtype) {
      case DB_INT_TYPE:
	status = adu_1int_coerce(adf_cb, &srcenode->pst_sym.pst_dataval,
						&convdv);
	break;

      case DB_FLT_TYPE:
	status = adu_1flt_coerce(adf_cb, &srcenode->pst_sym.pst_dataval,
						&convdv);
	break;

      case DB_DEC_TYPE:
	status = adu_1dec_coerce(adf_cb, &srcenode->pst_sym.pst_dataval,
						&convdv);
	break;

      case DB_CHA_TYPE:
      case DB_VCH_TYPE:
	status = adu_ascii(adf_cb, &srcenode->pst_sym.pst_dataval,
						&convdv);
	break;

      default:	/* Shouldn't get here - but just in case */
	EXdelete();
	return(FALSE);
    }

    EXdelete();

    if (adf_cb->adf_errcb.ad_errclass == ADF_USER_ERROR)
    {
	/* We found an overflow or similar failure through an exception, so 
	** unset the error class and return FALSE (kibro01) b119191
	*/
	adf_cb->adf_errcb.ad_errclass = 0;
	return(FALSE);
    }

    /* If coercion failed, just return FALSE. */
    if (status != E_DB_OK)
	return(FALSE);

    /* Copy the resulting DB_DATA_VALUE and return with success. */
    STRUCT_ASSIGN_MACRO(convdv, srcenode->pst_sym.pst_dataval);
    return(TRUE);		/* Ta da! */

}
