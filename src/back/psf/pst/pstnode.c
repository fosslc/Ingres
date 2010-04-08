/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <bt.h>
#include    <me.h>
#include    <mh.h>
#include    <st.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <adfops.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <scf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>

/**
**
**  Name: PSTNODE.C - Functions for making query tree nodes
**
**  Description:
**      This file contains functions for making query tree nodes.
**
**          pst_node - Make a query tree node, fill it in, and link up
**			its children.
**
**
**  History:
**      20-mar-86 (jeff)    
**          adapted from tree.c in the pre-Jupiter parser
**      03-sep-86 (seputis)
**          fixed SCU_INFORMATION call to pass length of output array
**	16-jan-87 (daved)
**	    on append/replace resdoms, check for conversion
**	22-feb-87 (stec)
**	    For RESDOMs fix datatypes only if right node is not null.
**	30-mar-87 (stec)
**	    Added datatype resolution for nullable data types.
**	21-apr-87 (puree)
**	    Do not check data type while preparing a dynamic SQL statement.
**	18-feb-88 (stec)
**	    Change pst_len initialization.
**	17-apr-89 (jrb)
**	    Changed interface to pst_node.
**	03-aug-89 (andre)
**	    Modified pst_node() to handle dummy node types introduced in
**	    conjunction with having target_list parsed before the FROM list.
**	22-sep-89 (andre)
**	    Modified pst_node() to accept a mask.  This will allow us to do away
**	    with the dummy datatypes described above.
**	25-oct-90 (andre)
**	    set pst_project to FALSE in PST_AGHEAD, PST_ROOT, PST_SUBSEL nodes
**	    regardless of language (bug 33371)
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	28-sep-92 (rblumer)
**	    FIPS CONSTRAINTS.
**	    Added new flag to pst_node function to denote type resolution for
**	    referential constraints.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**      07-oct-94 (mikem)
**	    Assert the PST_4IGNORE_DUPERR bit in pst_mask1 if the ps201
**          tracepoint has been executed in this session.  This traceflag
**          will be used by the CA-MASTERPIECE product so that duplicate
**          key errors will not shut down all open cursors (whenever the
**          "keep cursors open on errors" project is integrated this change
**          will no longer be necessary).
**	13-apr-95 (abowler)
**	    Clear the PST_4IGNORE_DUPERR bit in pst_mask1 if the ps201 
**	    tracepoint is off. This was causing the changed Masterpiece
**	    functionality to kick in when building with a Solaris V3
**	    compiler, as the bit was uninitialised.
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp().
[@history_template@]...
**	18-Feb-05 (srisu02)
**	    Fixed an AIX compilation error by typecasting the return value of 
**          a call to ME_ALIGN_MACRO as i4 
**	21-May-2009 (kiria01) b122051
**	    Reduce uninit'ed collID
**      03-nov-2009 (joea)
**          Replace sizeof(bool) by DB_BOO_LEN when dealing with DB_BOO_TYPE.
**	18-Mar-2010 (kiria01) b123438
**	    Initialize the data in the node if not passed in.
**/

/*
** Forward Function References
*/
bool
pst_is_const_bool(PST_QNODE *node, bool *bval);

/*
[@#defines_of_other_constants@]
[@type_definitions@]
*/

/*
** Global Variable Definitions
*/

/*
** Counters for constant folding stats - see psfmo.c
*/
#ifdef PSF_FOLD_DEBUG
i4 Psf_nbexpr = 0;
i4 Psf_nfoldbexpr = 0;
#endif

/*
[@static_variable_or_function_definitions@]
*/


/*{
** Name: pst_node	- Make a query tree node.
**
** Description:
**      This function makes a query tree node of the appropriate type,
**	and fills in the node values.  This can include operator ids,
**	data values, etc.  It links up the left and right children to
**	the newly created node.
**
**	NOTE: This function currently uses a memory stream interface in PSF.
**	When QSF becomes available, it will use the QSF equivalent of a
**	memory stream.
**
** Inputs:
**	sess_cb
**	stream				Stream to allocate from
**      left                            Pointer to the left child
**	right				Pointer to the right child
**	type				Type of node
**	value				Pointer to the value field
**					(NULL if none)
**	vallen				Length of value field in bytes
**	datatype			Datatype id for data value
**	dataprec			Precision/Scale of data value
**	datalen				Length of data value
**	datavalue			Pointer to data value
**	newnode				Place to put pointer to new node
**	err_blk				Place to put error code
**	flags				Mask to convey special info to
**					pst_node().
**		    PSS_NORES	       - do not do type resolution on opnodes.
**		    PSS_NOALLOC	       - Space for the node has already been
**					 allocated
**	 	    PSS_TYPERES_ONLY   - Perform only type resolution
**		    PSS_NOTYPERES_ERR  - Don't report type resolution errors.
**		    PSS_REF_TYPERES    - if error, report special error 
** Outputs:
**      newnode                         Filled in with pointer to new node
**	err_code			Filled in with error number on error
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Minor error or user error
**	    E_DB_FATAL			Catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory out of the stream.
**
** History:
**	20-mar-86 (jeff)
**          adapted from tree() in pre-Jupiter parser
**	30-mar-87 (stec)
**	    Added datatype resolution for nullable data types.
**	10-apr-87 (stec)
**	    Removed test for intrinsic datatypes for RESDOM nodes.
**	    This has been requested by Gene Thurston.
**	21-apr-87 (puree)
**	    Do not check data type while preparing a dynamic SQL statement.
**	18-feb-88 (stec)
**	    Change pst_len initialization to show length of meaningful
**	    data only.
**	29-dec-88 (andre)
**	    For SQL, if a node type is one of PST_ROOT, PST_SUBSEL, or
**	    PST_AGHEAD, set pst_qlang to DB_SQL (i4urlich) and pst_project to
**	    FALSE.
**	27-mar-89 (jrb)
**	    Added dataprec to parameter list for decimal project.
**	03-aug-89 (andre)
**	    Made changes to handle dummy node types introduced in conjunction
**	    with having target list parsed before the FROM list.  In particular,
**	    PST_1[ABU]OP is used to indicate that type resolution is not to be
**	    performed (because the characteristics of the operand(s) is not
**	    known.)  PST_2[ABU]OP indicate that the node has already been
**	    allocated, and we just want to perform type resolution.  PST_2CONST
**	    indicates that enough space for a PST_CONST node has been allocated,
**	    but we need to properly set fields inside the node (usually this
**	    would happen if we allocated PST_VAR node when processing the FROM
**	    list, but now we realized that it was supposed to be a PST_CONST;
**	    presently this would occur when processing a target list inside a
**	    dbproc.)  PST_2VAR indicates that we have already allocated space
**	    for PST_VAR node, and now we are ready to initialize the fields.
**	22-sep-89 (andre)
**	    Get rid of the "dummy" datatypes so painstakingly described above.
**	    Instead, we will have pst_node receive a mask with bits
**	    appropriately set to convey all sorts of wonderful info:
**	    PSS_NORES		--    do not do type resolution on opnodes.
**	    PSS_NOALLOC		--  Space for the node has already been
**				    allocated
**	    PSS_TYPERES_ONLY	-- Perform only type resolution
**	27-sep-89 (andre)
**	    Now pst_node will "understand" one more mask:
**	    PSS_NOTYPERES_ERR	--  Don't report type resolution errors.
**	27-apr-90 (andre)
**	    If we have a unary minus on a constant with db_data==NULL
**	    (this would occur if the constant is one of dbproc built-ins), do
**	    not call pst_negate(), since a node should be created. (bug 21429)
**	25-oct-90 (andre)
**	    set pst_project to FALSE in PST_AGHEAD, PST_ROOT, PST_SUBSEL nodes
**	    regardless of language (bug 33371)
**	04-feb-91 (andre)
**	    if a unary minus was encountered in conjunction with a repeat query
**	    parameter, we should build a node representing the unary minus
**	    rather than replace the constant with -1*constant.  Otherwise, QEP
**	    is built for repeat_parameter, rather than -1*repeat parameter, and
**	    this causes all sorts of unpleasant results.  This fixes bug 35605
**	05-nov-91 (andre)
**	    - don't take the unary plus shortcut unless the operand is of
**	      numeric type (i.e. INT, FLOAT, or DECIMAL)
**	    - apply the unary minus shortcut to DECIMAL operand
**	28-sep-92 (rblumer)
**	    FIPS CONSTRAINTS.
**	    Added new flag PSS_REF_TYPERES to denote type resolution for
**	    referential constraints.  If set, it will generate a special
**	    error message instead of generic type res errors.
**	22-apr-93 (ed)
**	    initialize pst_op_node.pst_flags
**	10-aug-93 (andre)
**	    removed declaration of scf_call()
**      07-oct-94 (mikem)
**	    Assert the PST_4IGNORE_DUPERR bit in pst_mask1 if the ps201
**          tracepoint has been executed in this session.  This traceflag
**          will be used by the CA-MASTERPIECE product so that duplicate
**          key errors will not shut down all open cursors (whenever the
**          "keep cursors open on errors" project is integrated this change
**          will no longer be necessary).
**	09-Mar-1999 (shero03)
**	    support MOP node.
**	7-sep-99 (inkdo01)
**	    Add PST_CASEOP to list of operator nodes which trigger type 
**	    resolution.
**	4-july-01 (inkdo01)
**	    Fix message for funcs with wrong number of parms.
**	1-may-02 (inkdo01)
**	    Minor change to re-direct case operand resolution errors.
**      13-aug-04 (wanfr01)
**          Bug 112828, INGSRV2799
**          Use pss_targparm if PSS_PARSING_TARGET_LIST is set when
**          determining whether to compute datatypes.
**	17-dec-04 (inkdo01)
**	    Init db_collID's to -1 or value in left/right.
**	07-Feb-2004 (bonro01)
**	    Fix alignment problem caused by addition of db_collID
**	04-nov-05 (toumi01)
**	    If PSS_XFORM_AVG is set that tells us to adjust the node's
**	    precision/scale, since the precision of avg(x) is one greater
**	    than the scale of sum(x). This is for the avg(x) to
**	    sum(x)/count(x) transformation only.
**	7-apr-06 (dougi)
**	    Sneak init of pst_lvrm/rvrm in here. They look untidy in
**	    debug displays otherwise.
**	13-sep-2006 (dougi)
**	    Add call to pst_convlit() to selectively permit auto coercion
**	    between string/numeric.
**      10-Nov-2006 (kschendel)
**          Clean up awkwardness in dataval copying.
**      03-jun-2005 (gnagn01)
**          Set joinid to PST_NOJOIN case operator and
**          set PSS_CASE_AGHEAD flag if there is case and aghead node in 
**          the query.Bug 113574,INGSRV3073.
**	13-Apr-2008 (kiria01) b119410
**	    Allow pst_joinid to be preset so that it is available to
**	    parser routines that need to manipulate the parse tree. Uses
**	    flag PSS_JOINID_PRESET.
**	03-Nov-2008 (kiria01) SIR121012
**	    Collapse at source boolean and case related nodes and
**	    moved fn psl_is_const_node to pst_is_const_node and put
**	    in here.
**	05-Dec-2008 (kiria01) b121333
**	    Allow pst_flags to be preset so that it is available to parser
**	    routines that need to manipulate the parse tree. Uses flag
**	    PSS_FLAGS_PRESET.
**	06-May-2009 (kiria01) b121012
**	    When folding WHLIST & CASEOP, check for NORES clear instead of
**	    TYPERES_ONLY and take care not to return unallocated node.
**	11-Jan-2010 (kiria01) SIR 121883
**	    Arrange for subtraction nodes to be treated as adds of negatives
**	    to allow for simpler tree comparisons and normalisations. Thus
**	    allow -(-(x)) to disappear and for numeric a-(-b) -> a+b and
**	    a-b -> a+(-b) allowing for normnalising across addition operators.
**      30-Mar-2010 (horda03) b123416
**          Only fold out a SUBTREE if the subtree does not contain any
**          VAR nodes. If the subtree contains VAR nodes these could be
**          correlated, and dropping the SUBTREE could cause E_OP0A91.
*/
DB_STATUS
pst_node(
	PSS_SESBLK	   *cb,
	PSF_MSTREAM        *stream,
	PST_QNODE	   *left,
	PST_QNODE	   *right,
	i4		   type,
	char		   *value,
	i4		   vallen,
	DB_DT_ID	   datatype,
	i2		   dataprec,
	i4		   datalen,
	DB_ANYTYPE	   *datavalue,
	PST_QNODE	   **newnode,
	DB_ERROR	   *err_blk,
	i4		    flags)
{
    DB_STATUS           status = E_DB_OK;
    ADI_DT_BITMASK	typeset;
    DB_DT_ID		db_right;
    DB_DT_ID		adb_right;
    DB_DT_ID		adatatype;
    PST_QNODE		*node;
    register PST_SYMBOL *symbol;
    PST_RT_NODE		*rnode;
    PTR			data_val = NULL;
    i4		err_code;
    i4		val1;
    i4		val2;
    bool 	bval;
    PST_QNODE   *folded_node = NULL;

    /*
    ** Note that if PSS_TYPERES_ONLY is set, node has been already allocated,
    ** and the fields in it need not be reset.  It is, however an opnode, and
    ** type resolution must be performed.
    */
    if (flags & PSS_TYPERES_ONLY)
    {
	/* *newnode is pointing at the previously allocated node */
	node = *newnode;
	symbol = &node->pst_sym;
	/*
	** All fields have already been set, so just fall through to type
	** resolution part
	*/
	if (!value)
	    value = (char*)&symbol->pst_value;
    }
    /*
    ** if a unary plus or minus is applied to a numeric (i.e. INT, DECIMAL, or
    ** FLOAT) constant, we can take a shortcut: for '+', we simply return ptr to
    ** the operand, for '-', we can negate  the value in place (subject to some
    ** limitations).
    */
    if (   type == PST_UOP
	&& left != (PST_QNODE *) NULL && left->pst_sym.pst_type == PST_CONST
	&& (left->pst_sym.pst_dataval.db_datatype == DB_INT_TYPE ||
	    left->pst_sym.pst_dataval.db_datatype == DB_FLT_TYPE ||
	    left->pst_sym.pst_dataval.db_datatype == DB_DEC_TYPE)
       )
    {
	if (((PST_OP_NODE *) value)->pst_opno == ADI_PLUS_OP)
	{
	    *newnode = left;
#ifdef	xDEBUG
	    /* Don't do tracing if couldn't get session control block */
	    if (cb != (PSS_SESBLK *) NULL)
	    {
		if (ult_check_macro(&cb->pss_trace, 18, &val1, &val2))
		{
		    TRdisplay("Unary + on a constant node.\n");
		    TRdisplay("Just return the sub-tree:\n\n\n");
		    status = pst_prmdump(*newnode, (PST_QTREE *) NULL,
			err_blk, DB_PSF_ID);
		}
	    }
#endif
	    return (E_DB_OK);
	}
	/*
	** If we have a unary minus on a numeric (i.e. int, float, or decimal)
	** constant, negate the constant and return a pointer to it.
	** 4/27/90 (andre)
	** Make exception for dbproc built-ins: since db_data is set to NULL, it
	** would be foolhardy to call pst_negate() which tries to manipulate
	** *db_data. (bug 21429)
	** 2/4/91 (andre)
	** pst_negate() should not be used with repeat query parameters, as this
	** results in incorrect QEPs being built (this fixes bug #35605)
	*/
	else if (   ((PST_OP_NODE *) value)->pst_opno == ADI_MINUS_OP
		 && left->pst_sym.pst_dataval.db_data != NULL
		 && left->pst_sym.pst_value.pst_s_cnst.pst_tparmtype !=
		         PST_RQPARAMNO
		)
	{
	    pst_negate(&left->pst_sym.pst_dataval);
	    *newnode = left;
#ifdef	xDEBUG
	    /* Don't do tracing if couldn't get session control block */
	    if (cb != (PSS_SESBLK *) NULL)
	    {
		if (ult_check_macro(&cb->pss_trace, 18, &val1, &val2))
		{
		    TRdisplay("Unary - on a constant node.\n");
		    TRdisplay("Just return the negated sub-tree:\n\n\n");
		    status = pst_prmdump(*newnode, (PST_QTREE *) NULL,
			err_blk, DB_PSF_ID);
		}
	    }
#endif
	    return (E_DB_OK);
	}
	else if (((PST_OP_NODE *)value)->pst_opno == ADI_MINUS_OP &&
		left->pst_sym.pst_type == PST_UOP &&
		left->pst_sym.pst_value.pst_s_op.pst_opno == ADI_MINUS_OP)
	{
	    *newnode = left->pst_left;
	    return E_DB_OK;
	}
    }

    /* Constant fold on AND/OR/NOT where these appear as nodes */
    switch (type)
    {
    case PST_OR:
	if (pst_is_const_bool(left, &bval))
        {
            if (bval)
            {
               *newnode = left;
               folded_node = right;
               break;
            }
            else
            {
               *newnode = right;
               folded_node = left;
            }
        }
        else if (pst_is_const_bool(right, &bval))
        {
            if (bval)
            {
               *newnode = right;
               folded_node = left;
               break;
            }
            else
            {
               *newnode = left;
               folded_node = right;
            }
        }
        break;
    case PST_AND:
	if (pst_is_const_bool(left, &bval))
        {
            if (bval)
            {
               *newnode = right;
               folded_node = left;
            }
            else
            {
               *newnode = left;
               folded_node = right;
            }
        }
        else if (pst_is_const_bool(right, &bval))
        {
            if (bval)
            {
               *newnode = left;
               folded_node = right;
            }
            else
            {
               *newnode = right;
               folded_node = left;
            }
        }
        break;
    case PST_NOT:
	if (pst_is_const_bool(left, &bval))
	{
	    *(bool*)left->pst_sym.pst_dataval.db_data = bval ? 0 : 1;
	    *newnode = left;
	}
	else if (left && left->pst_sym.pst_type == PST_NOT)
	    *newnode = left->pst_left;
	else
	    break;
	return E_DB_OK;
    case PST_WHLIST:
	if (right && right->pst_sym.pst_type == PST_WHOP &&
		right->pst_left &&
		(flags & PSS_NORES) == 0 &&
		pst_is_const_bool(right->pst_left, &bval))
	{
	    if (bval)
	    {
		/* make this the else and drop the rest */
		right->pst_left = NULL;
		left = NULL;
		/* As we changed left we need to make sure
		** that it is reflected in the node as in
		** TYPERES_ONLY it won't be. */
		if (flags & PSS_TYPERES_ONLY)
		    (*newnode)->pst_left = left;
		break; /* To pick up normal allocation */
	    }
	    else
		*newnode = left;
	}
	else
	    break;
	return E_DB_OK;
    case PST_CASEOP:
	if (left && left->pst_sym.pst_type == PST_WHLIST &&
		!left->pst_left &&
		left->pst_right &&
		left->pst_right->pst_sym.pst_type == PST_WHOP &&
		!left->pst_right->pst_left &&
		(flags & PSS_NORES) == 0)
	{
	    *newnode = left->pst_right->pst_right;
	}
	else
	    break;
	return E_DB_OK;
    case PST_BOP:
	if (((PST_OP_NODE *)value)->pst_opno == ADI_SUB_OP && (
		abs(left->pst_sym.pst_dataval.db_datatype) == DB_INT_TYPE ||
		abs(left->pst_sym.pst_dataval.db_datatype) == DB_FLT_TYPE ||
		abs(left->pst_sym.pst_dataval.db_datatype) == DB_DEC_TYPE) && (
		abs(right->pst_sym.pst_dataval.db_datatype) == DB_INT_TYPE ||
		abs(right->pst_sym.pst_dataval.db_datatype) == DB_FLT_TYPE ||
		abs(right->pst_sym.pst_dataval.db_datatype) == DB_DEC_TYPE)
	    )
	{
	    ((PST_OP_NODE *)value)->pst_opno = ADI_ADD_OP;
	    if (right &&
		right->pst_sym.pst_type == PST_UOP &&
		right->pst_sym.pst_value.pst_s_op.pst_opno == ADI_MINUS_OP)
	    {
		/* x - (-y) -> x+y */
		right = right->pst_left;
	    }
	    else
	    {
		PST_OP_NODE opnode;
		PST_QNODE *tmp = NULL;
		/* x - y -> x+ (-y) */
		MEfill(sizeof(opnode), 0, (PTR)&opnode);
		opnode.pst_opno = ADI_MINUS_OP;
		opnode.pst_opmeta = PST_NOMETA;
		opnode.pst_pat_flags = AD_PAT_DOESNT_APPLY;
		opnode.pst_joinid = PST_NOJOIN;
		if (status = pst_node(cb, stream, right, NULL,
			PST_UOP, (char *)&opnode, sizeof(opnode), DB_NODT, 0, 0,
			NULL, &tmp, err_blk, flags & ~PSS_TYPERES_ONLY))
		    return status;
		right = tmp;
	    }
	    if (flags & PSS_TYPERES_ONLY)
	    {
		/* Update the existing node */
		node->pst_right = right;
	    }
	}
	break;
    }

    /* If folded_node set, then need to verify that the subtree being elided
    ** does not contain a VAR node. If it does, then can't do the fold.
    */
    if (folded_node)
    {
       if (! psl_find_node( folded_node, NULL, PST_VAR) )
       {
          /* No VAR_NODEs in folded sub-tree so allow fold. */

          return E_DB_OK;
       }
    }
    
    if (!(flags & PSS_TYPERES_ONLY))
    {
	if (~flags & PSS_NOALLOC)
	{
	    /*
	    ** Allocate the node.  Get just enough for the common part of
	    ** the node and the part of the PST_SYMVALUE we'll actually
	    ** use.  Some are longer than others, and there's not much
	    ** point to allocating more than we need.  Also allocate space
	    ** for the data value, if any.
	    ** Align malloc'd area so that data portion is aligned to
	    ** DB_ALIGN_SZ, so that data that requires alignment such as float8
	    ** can be referenced.
	    */
	    status = psf_malloc(cb, stream,
		DB_ALIGN_MACRO(sizeof(PST_QNODE) - sizeof(PST_SYMVALUE) +
		vallen) + datalen,
		(PTR *) newnode, err_blk);
	}
	else if (datalen > 0 && datavalue != (DB_ANYTYPE *) NULL)
	{
	    /* only need to allocate space for data value */
	    status = psf_malloc(cb, stream, datalen, &data_val, err_blk);
	}
	
	if (status != E_DB_OK)
	{
	    return (status);
	}

	node = *newnode;
	symbol = &node->pst_sym;

	/* Hook up the left and right children */
	node->pst_left = left;
	node->pst_right = right;

	/*
	** Store away the node type, its length, its parameter number, and
	** the actual value. Length stored represents length of meaningful
	** data in the node.
	*/

	symbol->pst_len = vallen;
	if (datavalue != NULL)
	    symbol->pst_len += datalen;

	if (vallen)
	{
	    if (value)
		MEcopy(value, vallen, (char *) &symbol->pst_value);
	    else
		MEfill(vallen, 0, (char *)&symbol->pst_value);
	}

	switch (type)
	{
	    case PST_AND: case PST_OR:  
	    {
		symbol->pst_value.pst_s_op.pst_fdesc = NULL;
		/* fall thru */
	    }
	    case PST_UOP: case PST_BOP: 
	    case PST_AOP: case PST_COP:
	    case PST_MOP: 
	    {
		/* set joinid to PST_NOJOIN for all operator nodes */
		if (!(flags & PSS_JOINID_PRESET))
		    symbol->pst_value.pst_s_op.pst_joinid = PST_NOJOIN;
		/* initialize mask field */
		if (!(flags & PSS_FLAGS_PRESET))
		    symbol->pst_value.pst_s_op.pst_flags = 0;

		/* fall through to set the node type */

	    default:	    symbol->pst_type = type;
                /*check if it is a case operator and PST_AGHEAD flag is set*/
                if((type == PST_CASEOP) && (cb->pss_flattening_flags & PSS_AGHEAD))
                      cb->pss_flattening_flags |= PSS_CASE_AGHEAD; /* set 
                                  PSS_CASE_AGHEAD flag which is used to 
                                  set flattening off when there is case and aggregate in a query*/
	    }
                  break;
	}

	/*
	** For PST_SUBSEL, PST_ROOT, and PST_AGHEAD  set pst_project to FALSE
	** + set the language.
	*/
	if (type == PST_ROOT || type == PST_AGHEAD || type == PST_SUBSEL)
	{
	    rnode = &symbol->pst_value.pst_s_root;
	    rnode->pst_qlang   = cb->pss_lang;
	    rnode->pst_project = FALSE;
	    if (ult_check_macro(&cb->pss_trace, 
		PSS_ENABLE_NOERR_ON_DUPS, &val1, &val2))
	    {
		/* Will be used to ignore the duplicate key error on inserts/
		** updates from dmf (ie. the same way QUEL already does by
		** default).
		*/
		rnode->pst_mask1 |= PST_4IGNORE_DUPERR;
	    }
            else
            {
                rnode->pst_mask1 &= ~PST_4IGNORE_DUPERR;
            }
	}

	/*
	** Store away the data type, precision, length, and value (if any).
	*/
	symbol->pst_dataval.db_datatype = datatype;
	symbol->pst_dataval.db_prec = dataprec;
	symbol->pst_dataval.db_length = datalen;
	if (left && !DB_COLL_UNSET(left->pst_sym.pst_dataval))
	    symbol->pst_dataval.db_collID = left->pst_sym.pst_dataval.db_collID;
	else if (right && !DB_COLL_UNSET(right->pst_sym.pst_dataval))
	    symbol->pst_dataval.db_collID = right->pst_sym.pst_dataval.db_collID;
	else
	    symbol->pst_dataval.db_collID = DB_UNSET_COLL;
	if (datavalue != (DB_ANYTYPE *) NULL && datalen > 0)
	{
	    /*
	    ** Store the data value directly after the symbol; if data_val
	    ** is not NULL, space for data value had to be allocated
	    ** separately
	    ** Align db_data in case we are dealing with data that may need
	    ** to be aligned such as float8 values.
	    */
	    if (data_val == NULL)
		data_val = ME_ALIGN_MACRO(((char *) &symbol->pst_value) + vallen,DB_ALIGN_SZ);
	    symbol->pst_dataval.db_data = data_val;
	    MEcopy((char *) datavalue, datalen, data_val);
	}
	else
	{
	    symbol->pst_dataval.db_data = (PTR) NULL;
	}

#ifdef	xDEBUG
	/* Don't do tracing if couldn't get session control block */
	if (cb != (PSS_SESBLK *) NULL)
	{
	    if (ult_check_macro(&cb->pss_trace, 18, &val1, &val2))
	    {
		TRdisplay("Just allocated top node of:\n\n\n");
		status = pst_prmdump(*newnode, (PST_QTREE *) NULL,
		    err_blk, DB_PSF_ID);
	    }
	}
#endif
    }

    /* Perform type resolution on opnodes unless user explicitly asked not to */
    if ((type == PST_UOP || type == PST_AOP ||
         type == PST_BOP || type == PST_COP || 
	 type == PST_MOP || type == PST_CASEOP)	&&
	~flags & PSS_NORES			&&
	(cb->pss_defqry != PSQ_PREPARE ||
	 (!(cb->pss_stmt_flags & PSS_PARSING_TARGET_LIST) &&
	    cb->pss_highparm == -1) ||
	 ( (cb->pss_stmt_flags & PSS_PARSING_TARGET_LIST) &&
	    cb->pss_targparm == -1)
	)
    )
    {
	if (type == PST_UOP &&
	    symbol->pst_value.pst_s_op.pst_opno == ADI_EXIST_OP
	   )
	{
	    symbol->pst_dataval.db_datatype = DB_BOO_TYPE;
	    symbol->pst_dataval.db_prec = 0;
	    symbol->pst_dataval.db_length = DB_BOO_LEN;
	    symbol->pst_dataval.db_collID = DB_UNSET_COLL;
	    symbol->pst_value.pst_s_op.pst_opinst = (ADI_FI_ID) ADI_NOFI;
	    symbol->pst_value.pst_s_op.pst_fdesc = (ADI_FI_DESC *) NULL;
	}
	else
	{
	    /* Now resolve the operator node */
	    status = pst_resolve(cb, (ADF_CB*) cb->pss_adfcb, node, 
		cb->pss_lang, err_blk);
	    if (status == E_DB_OK)
	    {
	        if ((flags & PSS_XFORM_AVG) &&
		    abs(symbol->pst_dataval.db_datatype) == DB_DEC_TYPE &&
		    symbol->pst_value.pst_s_op.pst_opno == ADI_SUM_OP)
		{
		    int p1 = DB_P_DECODE_MACRO(symbol->pst_dataval.db_prec);
		    int s1 = DB_S_DECODE_MACRO(symbol->pst_dataval.db_prec);
		    s1 = min(DB_MAX_DECPREC, s1+1);
		    symbol->pst_dataval.db_prec = DB_PS_ENCODE_MACRO(p1, s1);
		}
	    }
	    else
	    {
		/*
		** If error indicates that there was no applicable function or
		** an ambiguous function was found, and the user requested that
		** such errors not be shown to the user, just return; otherwise
		** display error to the user.
		*/

		if (flags & PSS_NOTYPERES_ERR	    &&
		    (err_blk->err_code == 2903 ||
		     err_blk->err_code == 2907 ||
		     err_blk->err_code == 2908 ||
		     err_blk->err_code == 2909 ||
		     err_blk->err_code == 2910
		    )
		   )
		{
		    /*
		    ** Turn off PSS_NOTYPERES_ERR bit to indicate that an error
		    ** of the typem expected by the user has been encountered
		    ** and return E_DB_ERROR
		    */
		    flags ^= PSS_NOTYPERES_ERR;
		    err_blk->err_code = 0L;	/* reset error code */
		    return(E_DB_ERROR);
		}
		else if (flags & PSS_REF_TYPERES	    
			 && (err_blk->err_code == 2903 ||
			     err_blk->err_code == 2907 ||
			     err_blk->err_code == 2908 ||
			     err_blk->err_code == 2909 ||
			     err_blk->err_code == 2910
			     )
			 )
		{
		    /*
		    ** If error indicates that there was no applicable function,
		    ** & the user wants a special REFERENTIAL CONSTRAINT error,
		    ** display that special error to the user.
		    */
		    _VOID_ psf_error(E_PS0485_REF_TYPE_RES, 0L, PSF_USERERR,
			     &err_code, err_blk, 2,
			     psf_trmwhite(sizeof(DB_ATT_NAME),
				       (char *) &left->pst_sym.pst_value
					                 .pst_s_var.pst_atname),
			     &left->pst_sym.pst_value.pst_s_var.pst_atname,
			     psf_trmwhite(sizeof(DB_ATT_NAME),
				       (char *) &right->pst_sym.pst_value
					                 .pst_s_var.pst_atname),
			     &right->pst_sym.pst_value.pst_s_var.pst_atname);
		    return(E_DB_ERROR);
		}
		else if (err_blk->err_code == E_PS0001_USER_ERROR && 
		    (type == PST_CASEOP || type == PST_WHOP || type == PST_WHLIST))
		    return(E_DB_ERROR);		/* case-specific error */
		else
		{
		    return (pst_rserror(cb->pss_lineno, node, err_blk));
		}
	    }
	}
    }

    if ((type == PST_RESDOM) && (right != (PST_QNODE *) NULL) &&
	(cb->pss_defqry != PSQ_PREPARE ||
	 (!(cb->pss_stmt_flags & PSS_PARSING_TARGET_LIST) &&
	    cb->pss_highparm == -1) ||
	 ( (cb->pss_stmt_flags & PSS_PARSING_TARGET_LIST) &&
	    cb->pss_targparm == -1)
	)
    )
    {
	db_right = right->pst_sym.pst_dataval.db_datatype;
	adb_right = abs(db_right);
	adatatype = abs(datatype);
	if (adatatype != adb_right)
	{
	    /* can only convert intrinsics */
	    if (
		/*
		** Following code has been commented out 
		** as requested by Gene Thurston 4/10/87
		(adb_right != DB_INT_TYPE
	      && adb_right != DB_FLT_TYPE
	      && adb_right != DB_CHR_TYPE
	      && adb_right != DB_TXT_TYPE
	      && adb_right != DB_VCH_TYPE
	      && adb_right != DB_CHA_TYPE
	      && adb_right != DB_LTXT_TYPE
	       )
	     ||
		*/
	     ((status = adi_tycoerce((ADF_CB *) cb->pss_adfcb, db_right,
		&typeset))
	     ||
	     !BTtest((i4)ADI_DT_MAP_MACRO(adatatype), (char*) &typeset))
	     &&
	     !pst_convlit(cb, stream, &node->pst_sym.pst_dataval, right)
	    )
	    {
		/* error 2913 - incompatible type coercion */
		ADI_DT_NAME	    right_name;
		ADI_DT_NAME	    res_name;
	    
		/* Get the names of the left & right operands, if any */
		STmove("<none>", ' ', sizeof (ADI_DT_NAME),
		     (char *) &right_name);
		STmove("<none>", ' ', sizeof (ADI_DT_NAME), 
		    (char *) &res_name);

		status = adi_tyname((ADF_CB *) cb->pss_adfcb, db_right,
		    &right_name);
		status = adi_tyname((ADF_CB *) cb->pss_adfcb, datatype,
		    &res_name);

		/* Now report the error */
		(VOID) psf_error(2913, 0L, PSF_USERERR, &err_code, err_blk, 3,
		    sizeof(cb->pss_lineno), &cb->pss_lineno, 
		    (i4) psf_trmwhite(sizeof (ADI_DT_NAME), 
			(char *) &right_name), 
		    &right_name, 
		    (i4) psf_trmwhite(sizeof (ADI_DT_NAME), 
			(char *) &res_name),
		    &res_name);
		return (E_DB_ERROR);
	    }
	}
    }

    /* map the from list for quel queries. sql handled elsewhere.
    ** Also init pst_lvrm, pst_rvrm (they're annoying when not
    ** initialized). */
    if ((type == PST_ROOT || type == PST_AGHEAD || type == PST_SUBSEL) && 
	(rnode = &symbol->pst_value.pst_s_root))
    {
	MEfill(sizeof(PST_J_MASK), 0, (char *)&rnode->pst_lvrm);
	rnode->pst_lvrc = 0;
	MEfill(sizeof(PST_J_MASK), 0, (char *)&rnode->pst_rvrm);
	if (rnode->pst_qlang == DB_QUEL)
	{
	    MEfill(sizeof(PST_J_MASK), 0, (char *)&rnode->pst_tvrm);
	    pst_map(node->pst_right, &rnode->pst_tvrm);
	    pst_map(node->pst_left, &rnode->pst_tvrm);
	    rnode->pst_tvrc = BTcount((char*) &rnode->pst_tvrm, 
			    BITS_IN(rnode->pst_tvrm));
	}
    }
    return (E_DB_OK);
}

/*
** Name: pst_negate	- Negate a float or int in a DB_DATA_VALUE
**
** Description:
**      This function negates a float or int constant in a DB_DATA_VALUE.
**	It is useful for collapsing unary minuses when building query trees,
**	so that the optimizer will recognize a negated constant as a constant.
**
**	WARNING: This function assumes that the type must be integer or float,
**	and that integers come in 1, 2, 4, and 8 bytes while floats come only
**	in 4 and 8 bytes.
**
** Inputs:
**      dataval                         Pointer to the DB_DATA_VALUE to negate
**
** Outputs:
**      dataval.db_data                 Negates the data value
**	Returns:
**	    NONE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	27-mar-86 (jeff)
**          written
**	05-nov-91 (andre)
**	    add code to handle negation of DECIMAL constants
**	14-May-2004 (schka24)
**	    Handle i8's.
*/
VOID
pst_negate(
	register DB_DATA_VALUE *dataval)
{
    /* Fix up the pointer casting in here eventually, for the dumb compilers */
    if (dataval->db_datatype == DB_INT_TYPE)
    {
	if (dataval->db_length == 8)
	{
	    *((i8 *) dataval->db_data) = - *((i8 *) dataval->db_data);
	}
	else if (dataval->db_length == 4)
	{
	    *((i4 *) dataval->db_data) = - *((i4 *) dataval->db_data);
	}
	else if (dataval->db_length == 2)
	{
	    *((i2 *) dataval->db_data) = - *((i2 *) dataval->db_data);
	}
	else
	{
	    *((i1 *) dataval->db_data) = - *((i1 *) dataval->db_data);
	}
    }
    else if (dataval->db_datatype == DB_DEC_TYPE)
    {
	u_char	    pk[DB_MAX_DECLEN];

	/*
	** MHpkneg() uses MEcopy(), so we will place the negated value into pk
	** and then copy it back into dataval
	*/

	MHpkneg(dataval->db_data, DB_P_DECODE_MACRO(dataval->db_prec),
	    DB_S_DECODE_MACRO(dataval->db_prec), (PTR) pk);
	
	MEcopy((PTR) pk, dataval->db_length, dataval->db_data);
    }
    else /* Must be float */
    {
	if (dataval->db_length == 8)
	{
	    *((f8 *) dataval->db_data) = - *((f8 *) dataval->db_data);
	}
	else
	{
	    *((f4 *) dataval->db_data) = - *((f4 *) dataval->db_data);
	}
    }
}

/*{
** Name: pst_map	- Map the 'from' list equivalent for QUEL trees
**
** Description:
**	A tree is scanned for VAR nodes; when found, the
**	mapping defined by "map" is applied.  This is done so that
**	varno's as defined in trees in the 'tree' catalog will be
**	unique with respect to varno's in the user's query tree.  For
**
** Inputs:
**      tree                            Root of tree to do mapping in
**
** Outputs:
**	map				Array of range variable numbers;
**					the number in position n tells the
**					new number for range variable n.
**	err_blk				Filled in if an error happens
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	2-feb-87 (daved)
**	    written
**	10-jul-89 (andre)
**	    Do not make a special case out of BYHEAD: previously, pst_map()
**	    would be invoked recursively on the left child of a BYHEAD and the
**	    map retuned by it was disregarded.  This would cause a problem in a
**	    query like "retrieve tab.col where 1=any(1 by tab1.col1, tab.col1)".
**	    Now left subtree of a BYHEAD is mapped iteratively, just like that
**	    of the most of the other nodes
**	27-nov-02 (inkdo01)
**	    Range table expansion (change i4 to PST_J_MASK).
*/
VOID
pst_map(
	PST_QNODE   *tree,
	PST_J_MASK  *map)
{
    register PST_QNODE	*t;
    register i4	type;

    t	= tree;

    while (t != (PST_QNODE *) NULL)
    {
	type = t->pst_sym.pst_type;
	if (type == PST_ROOT || type == PST_AGHEAD || type == PST_SUBSEL)
	{
	    MEfill(sizeof(PST_J_MASK), 0, 
			(char *)&t->pst_sym.pst_value.pst_s_root.pst_tvrm);
	    pst_map(t->pst_right, &t->pst_sym.pst_value.pst_s_root.pst_tvrm);
	    pst_map(t->pst_left, &t->pst_sym.pst_value.pst_s_root.pst_tvrm);
	    t->pst_sym.pst_value.pst_s_root.pst_tvrc = 
		BTcount((char*) &t->pst_sym.pst_value.pst_s_root.pst_tvrm,
		BITS_IN(t->pst_sym.pst_value.pst_s_root.pst_tvrm));
	    break;
	}
	/* map right subtree */
	pst_map(t->pst_right, map);

	/* check this node */
	if (type == PST_VAR)
	{
	    BTset(t->pst_sym.pst_value.pst_s_var.pst_vno, (char*) map);
	}

	/* map left subtree (iteratively) */
	t = t->pst_left;
    }
}
/*
** Name: pst_is_const_bool - Is the node a constant bool type?
**
** Description:
**	This function determines if a given node has been folded to a constant
**	boolean value and if so, retrieves its value.
**
** Input:
**	node		ptr to PST_QNODE
**			
** Output:
**	*bval		the value of the boolean - undefined if function
**			returns FALSE
**
** Returns:
**	TRUE or FALSE
**
** History:
**	30-Sep-2008 (kiria01) b121012
**	    Created to aid constant folding
**	07-Jan-2009 (kiria01) b121012
**	    Don't forget NULLability. Also Treat any NULL
**	    bools as not TRUE (FALSE).
**	12-Jan-2009 (kiria01) b121012
**	    Windows has a different size for bool - use sizeof.
**	06-May-2009 (kiria01) b121012
**	    Remove the char access to bool data - again for Windows.
*/
bool
pst_is_const_bool(
    PST_QNODE *node,
    bool *bval)
{

#   ifdef PSF_FOLD_DEBUG

    Psf_nbexpr++;
#   endif
    if (node &&
	node->pst_sym.pst_type == PST_CONST &&
	node->pst_sym.pst_value.pst_s_cnst.pst_tparmtype == PST_USER &&
	node->pst_sym.pst_value.pst_s_cnst.pst_parm_no == 0 &&
	node->pst_sym.pst_dataval.db_data)
    {

	switch(node->pst_sym.pst_dataval.db_datatype)
	{
	case -DB_BOO_TYPE:
	    if (node->pst_sym.pst_dataval.db_length != DB_BOO_LEN+1)
		break;
	    *bval = (((char*)node->pst_sym.pst_dataval.db_data)[DB_BOO_LEN] & ADF_NVL_BIT) != 0
			? FALSE
			: *(bool*)node->pst_sym.pst_dataval.db_data;
#	    ifdef PSF_FOLD_DEBUG
	     Psf_nfoldbexpr++;
#	    endif
	    return TRUE;
	case DB_BOO_TYPE:
	    if (node->pst_sym.pst_dataval.db_length != DB_BOO_LEN)
		break;
	    *bval = *(bool*)node->pst_sym.pst_dataval.db_data;
#	    ifdef PSF_FOLD_DEBUG
	     Psf_nfoldbexpr++;
#	    endif
	    return TRUE;
	}
    }
    return FALSE;
}

