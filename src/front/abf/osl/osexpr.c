/*
** Copyright (c) 1984, 2004, 2008 Ingres Corporation
*/

#include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
#include	<cm.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<adf.h>
#ifndef ADE_CHK_TYPE
#define ADE_CHK_TYPE(itype, type) ((itype) == (type) || (itype) == -(type))
#endif
#include	<ade.h>
#include	<afe.h>
#include	<oslconf.h>
#include	<ossym.h>
#include	<ostree.h>
#include	<oskeyw.h>
#include	<iltypes.h>
#include	<ilops.h>
#include	<oserr.h>

/**
** Name:	osexpr.c -	OSL Translator Expression Generation Module.
**
** Description:
**	Contains routines that generate IL code to evaluate OSL parser
**	expressions.  Defines:
**
**	osvalref()	return reference to expression value.
**	osvarref()	return reference to expression value in variable.
**	osevalexpr()	generate IL code to evaluate expression.
**
**	Note:  Both 'osvalref()' and 'osvarref()' handle "tkID" nodes
**	as "tkSCONST" nodes to support references for <ingres_name> at
**	this top level only, since <ingres_name> (and ID nodes) cannot
**	be part of an expression.
**
** History:
**	Initial revision  (87/10  wong)
**
**	Revision 5.1  86/11/04  14:13:02  wong
**	Removed special case for dynamic string allocation in logical
**	expression evaluation.  Modified to use keyword/operator string
**	definitions.
**
**	Revision 6.0  87/06  wong
**	Complete ADT support through ADF, ADE including Nulls.
**	88/08  wong  Fixed bug #3081 in 'push_nots()'.
**
**	Revision 6.1  88/11  wong
**	88/10  wong  Added special support for ADE_PMENCODE.
**	88/04  wong  Switch function instance IDs when applying DeMorgan's
**	rules in 'push_nots()'.
**
**	Revision 6.2  89/07  wong
**	Corrected allocation of temporaries for expression results now based on
**	type and length.  JupBug #7153.
**
**	Revision 6.3  89/08  wong
**	Corrected generation of boolean expressions to execute all non-boolean
**	sub-expressions correctly.  JupBug #7187.
**
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures (in osvalref).
**	08/17/91 (emerson)
**		Moved function declaration of punt_len to outer scope.
**
**	Revision 6.5
**	21-jul-92 (davel)
**		modifications for decimal datatype support. Also add
**		precision argument to the osdatalloc() calls; when using 
**		symbol information, pass sym_>s_scale; for nodes, use the
**		new n_prec member of OSNODE. Add special case logic in
**		instr_gen() for decimal literals (special code already exists 
**		for other types).
**	29-jul-92 (davel)
**		Add length and precision arguments to osconstalloc(),
**		as ostmpalloc() now takes both length and precision arguments.
**	 3-sep-93 (donc) Bug 52800
**	 	Add to the osevalexpr() check looking for datatype mismatches
**		(and thereby generating a temporary variable and another
**		sub-expression) a check for a decimal datatype in a result.
**		the MH routines cannot handle expression where the LHS  
**		variable is also used on the RHS (to do so causes variable
**		corruption.
**	17-nov-93 (donc) Bug 55930
**		Add NO_OPTIM for su4_u42 Ming Hint
**	19-jan-94 (donc) Bug 55930
**		Remove NO_OPTIM for su4_u42 Ming Hint
**    2-mar-94 (donc) Bug 59382
**              Exclude generation of an intermediate temporary variable
**              when coerseing a decimal to a float as per bug 52800.
**	21-mar-94 (donc) Bug 60470
**		Explicitly cast arguments to osconstalloc.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Jan-2008 (kiria01) b119806
**		Extended grammar for postfix operators beyond IS NULL
**		removes the need for th NULLOP node which is replaced
**		by the ability of the RELOP nodes to support monadic
**		operators as well as dyadic.
**	26-Aug-2009 (kschendel) b121804
**	    A couple new style function defn's for gcc 4.3.
*/

VOID	osevalexpr();

/*{
** Name:	osvalref() -	Return Reference to Expression Value.
**
** Description:
**	This routine returns a reference to an expression value.  If the
**	expression is a simple value, i.e., a variable reference or a constant,
**	a reference to that object is returned.  Otherwise, the expression is
**	evaluated into a temporary and a reference to the temporary is returned.
**
** Input:
**	expr	{OSNODE *}  The OSL parser expression tree.
**
** Returns:
**	{ILREF}	A reference to the expression value.
**
** History:
**	09/86 (jhw) -- Written.
**	04/07/91 (emerson)
**		Modifications for local procedures: changed a test
**		of s_ref to reflect addition of new bit flags.
*/

ILREF
osvalref (OSNODE *expr)
{
	ILREF	refer = 0;
	i4	token = expr->n_token;

	if (token == VALUE || token == DOT || token == ARRAYREF)
	{
		refer = expr->n_ilref;
		ostrfree(expr);
	}
	else if (token == tkBYREF)
	{
		refer = expr->n_actual->s_ilref;
		ostrfree(expr);
	}
	else if (expr->n_token == tkNULL)
	{ /* Null constant */
		refer = (ILREF)osconstalloc((DB_DT_ID)expr->n_type, (i4)0,
			 (i2)0, (char *)NULL)->s_ilref;
		ostrfree(expr);
	}
	else if (token == tkID || tkIS_CONST_MACRO(token))
	{
		char *p = (token == tkID ? expr->n_value : expr->n_const);

		refer = IGsetConst(expr->n_type, p);
		ostrfree(expr);
	}
	else if ( token == tkPCALL )
	{ /* call a function */
		register OSSYM	*tmp;
		DB_DT_ID	type;

		if ( expr->n_type == DB_VCH_TYPE 
		  && expr->n_rlen == ADE_LEN_UNKNOWN 
		   )
			type = DB_DYC_TYPE;
		else
			type = expr->n_type;

		tmp = osdatalloc(type, expr->n_rlen, expr->n_prec);
		osgencall(expr, tmp);

		refer = (ILREF)tmp->s_ilref;
	}
	else if ( expr->n_type != DB_NODT )
	{ /* evaluate expression */
		register OSSYM	*tmp;

		osevalexpr(expr, (tmp = osdatalloc(expr->n_type, 
						   expr->n_length, 
						   expr->n_prec)));

		refer = (ILREF)tmp->s_ilref;
	}
	return refer;
}

/*{
** Name:	osvarref() -	Return Reference to Expression Value in Variable.
**
** Description:
**	This routine returns a reference to an expression value in a variable,
**	temporary or otherwise.  In particular, a temporary is allocated for
**	constant expression values into which the constant is placed.
**
**	(Forcing constants into variables is required by some IL statements
**	that can accept non-type specific references.  The type is encoded
**	in the DB data value representing the variable.)
**
** Input:
**	expr	{OSNODE *}  The OSL parser expression tree.
**
** Returns:
**	{ILREF}	A reference to the expression value.
**
** History:
**	09/86 (jhw) -- Written.
*/
ILREF
osvarref (OSNODE *expr)
{
	i4 token = expr->n_token;

	if (token != tkID && token != tkNULL && !tkIS_CONST_MACRO(token))
	{
		return osvalref(expr);
	}
	else
	{
		ILREF	refval;

		refval = (ILREF)osconstalloc( (DB_DT_ID)expr->n_type, 
					(i4)expr->n_length, (i2)expr->n_prec,
					(char *)expr->n_const)->s_ilref;
		ostrfree(expr);
		return refval;
	}
}

/*{
** Name:	osevalexpr() -	Generate IL Code to Evaluate Expression.
**
** Description:
**	This routine generates IL code to evaluate an expression into the
**	variable represented by the passed in symbol, result.  This code consist
**	of either an IL assignment or and IL expression.  Whether the expression
**	is of type boolean determines how it will be evaluated.
**
**	Logical expressions are transformed into conjunctive-normal form first.
**	Expressions consisting of simple assignment from a field or constant
**	generate code to perform the assignment.  Otherwise, the expression
**	generates IL code to evaluate the expression into the result.
**
** Input:
**	exp	{OSNODE *}  OSL parser expression tree.
**	result	{OSSYM *}  OSL symbol table entry for result.
**
** History:
**	08/86 (jhw) -- Written.
**	07/89 (jhw) -- Check length as well as type to determine if temporary
**		is necessary for expression evaluation.  JupBug #7153.
**	08/89 (jhw) -- Call new 'exprBool()' for boolean expressions to generate
**		sub-expressions in correct order (non-booleans should be
**		generated first to guarantee that they get executed through all
**		conditional paths.)  JupBug #7187.
**	11/90 (jhw)  Interpret string literals as VARCHAR or TEXT in assignments
**		so string semantics are maintained.  Bug #30893.
**	 3-sep-93 (donc) Bug 52800
**	 	Add to the osevalexpr() check looking for datatype mismatches
**		(and thereby generating a temporary variable and another
**		sub-expression) a check for a decimal datatype in a result.
**		the MH routines cannot handle expression where the LHS  
**		variable is also used on the RHS (to do so causes variable
**		corruption.
**	1-nov-93 (donc) More of bug 52800
**		Added more to osevalexpr(). If a user was tempted to assign
**		a decimal to itself (I cannot think of any sane reasoning for
**		doing this), the resulting value was incorrect. I added the
**		generation of a temp variable to be used in this case, so the
**		MH routines don't overwrite the input while generating the
**		result.
*/
static VOID	cnf();
static VOID	push_nots();
static VOID	cnorm();
static VOID	distribute();
static VOID	term_and();

static i4	sizeExpr();
static OSSYM	*evalExpr();
static VOID	evalBool();
static VOID	instr_gen();

VOID
osevalexpr (exp, result)
OSNODE	*exp;
OSSYM	*result;
{
	register OSNODE *expr;
	i4		op;
	i4		size;

	if (!ADE_CHK_TYPE(exp->n_type, DB_BOO_TYPE))
		op = IL_EXPR;
	else
	{ /* transform to CNF */
		op = IL_LEXPR;
		cnf(exp);	/* transform tree */
		term_and(&exp); /* add terminating AND node */
	}

	expr = exp;
	while (expr->n_token == PARENS)
		expr = expr->n_left;

	if ((size = sizeExpr(expr)) == 0)
	{ /* simple assignment */
		if (expr->n_token == tkPCALL)
		{ /* build param structure and call procedure */
			osgencall(expr, result);
		}
		else
		{
			ADI_FI_ID	coerce;
		        OSSYM	        *res;

			coerce = oschkcoerce(expr->n_type, result->s_type);

			if (expr->n_token == VALUE 
			  || expr->n_token == DOT
			  || expr->n_token == ARRAYREF)
			{
		                if( ADE_CHK_TYPE(expr->n_type, DB_DEC_TYPE)
                                 && ADE_CHK_TYPE(result->s_type, DB_DEC_TYPE))
			        {
				    res = osdatalloc(expr->n_type, 
						 expr->n_length, 
						 expr->n_prec);
				    IGgenStmt(IL_ASSIGN, (IGSID *)NULL,
					3, res->s_ilref, 
					expr->n_ilref, coerce
				    );
				    IGgenStmt(IL_ASSIGN, (IGSID *)NULL,
					3, result->s_ilref, 
					res->s_ilref, coerce
				    );
			        }
                                else
				    IGgenStmt(IL_ASSIGN, (IGSID *)NULL,
					3, result->s_ilref, 
					expr->n_ilref, coerce
				    );
			}
			else if ( expr->n_token == tkNULL
					|| tkIS_CONST_MACRO(expr->n_token) )
			{ /* constants */
				ILREF	refval;

				if ( expr->n_type == DB_CHR_TYPE
						|| expr->n_type == DB_CHA_TYPE ) 
				{ /* string literals */
				    char	*iiIG_vstr();
				    expr->n_const = iiIG_vstr( expr->n_const,
						       STlength(expr->n_const));
				    expr->n_type =
						QUEL ? DB_TXT_TYPE :DB_VCH_TYPE;
				    expr->n_length = ((DB_TEXT_STRING *)
						expr->n_const)->db_t_count + 
						DB_CNTSIZE;
				    coerce = oschkcoerce( expr->n_type,
							result->s_type);
				}

				refval = (ILREF)osconstalloc(
						(DB_DT_ID)expr->n_type, 
						(i4)expr->n_length, 
						(i2)expr->n_prec,
						(char *)expr->n_const)->s_ilref;

				IGgenStmt(IL_ASSIGN, (IGSID *)NULL,
					3, result->s_ilref, refval, coerce
				);
			} /* constants */
			else
			{
				osuerr(OSBUG, 1, ERx("osevalexpr(unknown token)"));
			}
		}
	}
	else
	{ /* generate expression */
		OSSYM	*res = result;

		ostmpbeg();
		IGbgnExpr(0, size);

		if (ADE_CHK_TYPE(expr->n_type, DB_BOO_TYPE))
		{ /* boolean expression */
			evalBool(expr, FALSE);  /* generate sub-expressions */
			evalBool(expr, TRUE);  /* generate boolean expression */
		}
		else
		{ /* value expression */
			/* Note:  'expr->n_token' is an operator */
			if (expr->n_type != result->s_type
			  || expr->n_length != result->s_length
		          || ADE_CHK_TYPE(expr->n_type, DB_DEC_TYPE))

			{
				res = osdatalloc(expr->n_type, 
						 expr->n_length, 
						 expr->n_prec);
			}

			if (expr->n_token == UNARYOP)
			{
				/* assert:  expr->n_fiid != ADE_PMENCODE */
				if (expr->n_left == NULL)
				{ /* special case for internal ADF functions */
					instr_gen( expr->n_fiid, expr->n_length,
							1, res
					);
				}
				else
				{
					instr_gen( expr->n_fiid, expr->n_length,
						2, res, evalExpr(expr->n_left,
								(OSSYM *)NULL)
					);
				}
			}
			else
			{ /* binary op */
				if (!oschkstr(expr->n_type))
				{
					instr_gen(expr->n_fiid, expr->n_length,
						3, res,
						evalExpr(expr->n_left, (OSSYM *)NULL),
						evalExpr(expr->n_right, (OSSYM *)NULL)
					);
				}
				else
				{
					/* Note:  Order of evaluation is 
					** defined for Strings 
					*/
					OSSYM	*left, *tempsym;
		
					if (res == result
						&& (result->s_ref & OS_REFMASK))
					{
						res = osdatalloc(result->s_type,
						      (i4) result->s_length,
						      (i4) result->s_scale);
					}
					/*
					** Note: Do not pass down 'res' when 
					** evaluating expressions of type 
					** DB_CHA_TYPE.  DB_CHA_TYPE are fixed 
					** length with blank-padding upto the 
					** full length and cannot be used
					** for intermediate results.
					*/
					if ( !ADE_CHK_TYPE(res->s_type, 
							DB_CHA_TYPE) 
					  && res->s_type == expr->n_type 
					  && res->s_type == expr->n_left->n_type
					   )
					{
						tempsym = res;
					}
					else
					{
						tempsym = (OSSYM *)NULL;
					}
					left = evalExpr(expr->n_left, tempsym);

					instr_gen(expr->n_fiid, expr->n_length,
						3, res, left,
						evalExpr(expr->n_right, 
							(OSSYM *)NULL)
					);
				} /* string expression */
			} /* binary op */

			if (result != res)
			{
				instr_gen(oschkcoerce(res->s_type, result->s_type),
					expr->n_length, 2, result, res
				);
			}
		} /* value expression */

		IGgenExpr(op, (op == IL_LEXPR ? result->s_ilref :(ILREF)0));
		ostmpend();
	} /* generate expression */

	ostrfree(expr);
}

/*
** Name:	cnf() -	Convert Logical Expression to Conjunctive-Normal Form.
**
** Description:
**	This routine applies various transformation rules to convert an
**	OSL parser logical expression to conjunctive-normal form.  An
**	expression tree is in <conjunctive normal form> (CNF) when it is
**	either a literal, or a disjunction of literals, or a conjunction
**	whose conjuncts are either literals or disjunctions of literals.
**
**	These transformations are:
**
**		(1) Apply DeMorgan's Rules and the double-negation rule
**			to push NOTs as far down the expression tree as possible.
**		(2) Distribute OR nodes over AND nodes. (Expression is now CNF.)
**
** Input:
**	expr	{OSNODE *}  OSL parser expression tree to transform.
**
** History:
**	08/86 (jhw) -- Written.
*/
static VOID
cnf (expr)
OSNODE	*expr;
{
	push_nots(expr, FALSE);	/* step 1:  push NOTs down tree */
	cnorm(expr);		/* step 2:  normalize ORs over ANDs */
}

/*
** Name:	push_nots() -	Push NOTs Down to (Logical) Leaf Nodes.
**
** Description:
**	This routine pushes NOT nodes down a tree to the leaf nodes as far as
**	possible (a logical leaf node is a comparison node.)  It does so by
**	applying DeMorgan's Rules and the double-negation rule whenever a NOT
**	node is found in the tree in a pre-order walk of the tree.
**
**	This routine pushes a NOT node by deleting the node (accomplished by
**	copying the child node to the node and freeing the child) and applying
**	the NOT to the child node as it continues the push.  Further pushing
**	is required for only logical operators and their children.
**
**	The 'negation' parameter indicates when a NOT is being pushed and should
**	be applied to the current node.  This will be application of the
**	double-negation rule if the node is a NOT node, DeMorgan's Rules if
**	the node is either an AND or an OR, or complementing the operator
**	if the node represents a relational operator.
**
**		Double-Negation:
**			not ( not P ) ==> P
**
**		DeMorgan's Rules:
**			not (A or B) ==> not A and not B
**			not (A and B) ==> not A or not B
**
** Input:
**	expr	{OSNODE *}  OSL parser expression tree to transform.
**	negation {bool}  Flags whether a NOT should be applied to the current
**			 node or possibly any children (DeMorgan's Rules.)
**
** History:
**	08/86 (jhw) -- Written.
**	02/87 (jhw) -- Now uses ADF to find complement (via 'oschkcmplmnt()'.)
**	08/88 (jhw) -- When applying DeMorgan's Rules and switching logical
**			operator must also switch 'n_fiid'.
**	09/90 (jhw) -- For special ESCAPE handling, apply negation to LIKE
**		child.  Bug #32956.
*/
static VOID
push_nots ( expr, negation )
register OSNODE	*expr;
bool		negation;
{
	if ( negation )
	{ /* apply a NOT node */
		if ( expr->n_token == LOGOP )
		{
			if ( osw_compare(expr->n_value, _Not) )
			{ /* apply double-negation rule (ie, delete node) */
				OSNODE	*child = expr->n_left;

				MEcopy((PTR)child, sizeof(*child), (PTR)expr);
				ostrnfree(child);
				negation = FALSE;
			}
			/* apply DeMorgan's Rules */
			else if ( osw_compare(expr->n_value, _Or) )
			{ /* switch to AND */
				expr->n_value = _And;
				expr->n_fiid = ADE_AND;
			}
			else if ( osw_compare(expr->n_value, _And) )
			{ /* switch to OR */
				expr->n_value = _Or;
				expr->n_fiid = ADE_OR;
			}
		}
		else if ( expr->n_token == RELOP )
		{ /* apply NOT by complementing operator */
			if ( expr->n_token == RELOP
					&& osw_compare(expr->n_value, _Escape) )
			{ /* apply to LIKE node, which is left child */
				expr = expr->n_left;
			}
			if ( (expr->n_fiid = oschkcmplmnt(expr->n_fiid))
					== ADI_NOFI )
			{
				osuerr(OSBUG, 1,ERx("osvalref(no complement)"));
			}
		}
		/*
		** Note:  At present, no other cases can occur, since NOTs can
		** only be applied to logical or relational nodes (guaranteed by
		** the syntax.)  However as an example, if boolean variables are
		** allowed, then code will have to be added to insert a NOT node
		** when applied to a leaf representing a boolean variable.
		*/
	}
	/*
	** Continue pushing down tree for logical nodes.
	*/
	if (expr->n_token == LOGOP)
	{ /* continue pushing down tree ... */
		if (osw_compare(expr->n_value, _Not))
		{ /* push not (ie, delete node and apply to child) */
			OSNODE	*child = expr->n_left;

			MEcopy((PTR)child, sizeof(*child), (PTR)expr);
			ostrnfree(child);

			push_nots(expr, TRUE);
		}
		else /* "or" or "and" */
		{ /* check children, applying NOTs as part of DeMorgan's Rules */
			push_nots(expr->n_left, negation);
			push_nots(expr->n_right, negation);
		}
	}
}

/*
** Name:	cnorm() -	Perform Conjunctive Normalization of Tree.
**
** Description:
**	This routine transforms a tree into conjunctive-normal form by applying
**	the distributive rule to any nodes it finds with OR over ANDs.  This is
**	done as part of a post-order walk of tree.  (The tree should have had
**	all NOTs pushed prior to calling this routine.)
**
** Input:
**	tree	{OSNODE *}  OSL parser expression tree to transform.
**
** History:
**	08/86 (jhw) -- Written.
*/
static VOID
cnorm (tree)
register OSNODE	*tree;
{
	if ( tree->n_token == LOGOP
			&& ( osw_compare(tree->n_value, _And)
				|| osw_compare(tree->n_value, _Or) ) )
	{ /* normalize only AND or OR nodes (others already CNF) */
		cnorm(tree->n_left);
		cnorm(tree->n_right);
		if (osw_compare(tree->n_value, _Or)  &&
			((tree->n_left->n_token == LOGOP &&
				osw_compare(tree->n_left->n_value, _And)) ||
			(tree->n_right->n_token == LOGOP &&
				osw_compare(tree->n_right->n_value, _And))))
			distribute(tree);	/* apply distributive rule */
	}
}

/*
** Name:	distribute() -		Distribute ORs Over ANDs.
**
** Description:
**	This routine distributes an OR node over any AND nodes it might have
**	as children.  When both children are AND nodes, the distribution is
**	accomplished by making a one-level recursive call on this routine
**	for each of the resultant children of the first application of the
**	distributive rule (making essentially a second-pass.)
**
**	The distributive rules applied are:
**
**		(A and B) or C ==> (A or C) and (B or C)
**		A or (B and C) ==> (A or B) and (A or C)
**	or
**		(A and B) or (C and D) ==> (A or (C and D)) and (B or (C and D))
**			<second-pass>  ==> (A or C) and (A or D) and
**							(B or C) and (B or D)
**
**	Note:  This routine is not recursive so the distributive rule must have
**	been applied to any children prior to applying it to the parent node.
**
** Input:
**	node	{OSNODE *} The OR node to be distributed over it's AND children.
**
** History:
**	08/86 (jhw) -- Written.
*/
static OSNODE	*copy_expr();

static VOID
distribute (node)
register OSNODE	*node;
{
	register OSNODE	*andn;	/* the AND node being distributed over */
	register OSNODE	*othern;/* the other node */
	register OSNODE	**linkp;/* ref. to other node link */
	struct {
		bool	left;	/* dist. over resultant left child */
		bool	right;	/* dist. over resultant right child */
	} redistribute;

	if (node->n_left->n_token == LOGOP 
	  && osw_compare(node->n_left->n_value, _And))
	{
		andn = node->n_left;
		othern = node->n_right;
		linkp = &node->n_right;
	}
	else
	{
		andn = node->n_right;
		othern = node->n_left;
		linkp = &node->n_left;
	}

	/*
	** Distribute OR over AND by changing OR node to AND, attaching a new OR
	** node with the right child of the AND node and the other node as its
	** children (in place of what was the other node), and re-using the AND
	** node as another new OR node with a its left child intact and a copy
	** of the other node as its children.
	**
	**	      OR		             AND
	**	    /	 \		            /	\
	**	   /	  \		 ==>	   /	 \
	**	  /	   \			  /	  \
	**	AND	 other			 OR	  OR
	**     /   \				/  \	 /  \
	**  left right			    left other right copy-of-other
	**
	** If any of the children (left, right, or other) is also an AND node,
	** then the distribute rule must be applied again to each resultant OR
	** child that has as a child one of the these AND node children to
	** complete the distribution.  This is accomplished by recursive calls
	** to this routine and will recurse as many times as necessary to
	** distribute the original OR over all the ANDs in its child nodes.
	*/

	/* Check for redistribution over resultant children */
	redistribute.left = redistribute.right = FALSE;
	if (othern->n_token == LOGOP && osw_compare(othern->n_value, _And))
		redistribute.left = redistribute.right = TRUE;
	else
	{
		if (andn->n_left->n_token == LOGOP 
		  && osw_compare(andn->n_left->n_value, _And))
		{
			redistribute.left = TRUE;
		}
		if (andn->n_right->n_token == LOGOP 
		  && osw_compare(andn->n_right->n_value, _And))
		{
			redistribute.right = TRUE;
		}
	}
	/*
	** NOTE:  The following implementation is dependent on creation of the
	** new OR node occurring before re-use of the AND node as an OR node!
	*/
	/* make new OR node by copying current OR node
	** (and attach in place of other node)
	*/
	*linkp = oscpnode(node, andn->n_right, copy_expr(othern));
	/* make current OR node into AND node */
	node->n_value = _And;
	node->n_fiid = andn->n_fiid;	/* AND function instance ID */
	/* re-use AND child node as OR node */
	andn->n_value = _Or;
	andn->n_fiid = (*linkp)->n_fiid;	/* OR function instance ID */
	andn->n_right = othern;

	/* Check for follow-up application of distribute */
	if (redistribute.left)
		distribute(andn);
	if (redistribute.right)
		distribute(*linkp);
}

/*
** Name:	copy_expr() -	Create Copy of Expression Tree.
**
** Description:
**	This routine duplicates an expression tree (or sub-tree) by creating a
**	new node for each node of the tree as part of a walk of the tree.  Note
**	that only logical operators are duplicated so that only relation nodes
**	need to be evaluated more than once in the resulting DAG.
**
** Input:
**	tree	{OSNODE *}  OSL parser expression tree to copy.
**
** History:
**	08/86 (jhw) -- Written.
*/
static OSNODE *
copy_expr (tree)
register OSNODE	*tree;
{
	if (tree == NULL)
		return NULL;

	if (tree->n_token != LOGOP)
		return tree;

	/* duplicate logical node */

	return oscpnode(tree, copy_expr(tree->n_left), copy_expr(tree->n_right));
}

/*
** Name:	term_and() -	Add Terminating AND Node to Logical Expression.
**
** Description:
**	This routine adds a terminating AND node to a logical expression in the
**	right-most position.  The right child of this AND node is NULL.  This
**	allows short-circuit evalation of logical expressions.
**
** Input:
**	tree	{OSNODE **}  Address of the expression tree.
**
** History:
**	08/86 (jhw) -- Written.
*/
static VOID
term_and (tree)
OSNODE	**tree;
{
	register OSNODE	*node = *tree;

	if (node->n_token == LOGOP && osw_compare(node->n_value, _And))
		term_and(&node->n_right);
	else
	{ /* add terminating AND node */
		U_ARG	arg[3];

		arg[0].u_cp = _And;
		arg[1].u_nodep = node;
		arg[2].u_nodep = node;
		*tree = osmknode(LOGOP, &arg[0], &arg[1], &arg[2]);
		(*tree)->n_right = NULL;
	}
}

/*
** Name:	sizeExpr() -	Determine IL Code Size for Expression.
**
** Description:
**	This routine determines the size of the IL code that will be
**	generated to evaluate an OSL parser expression tree.
**
**	The size of the IL code for each operator node of the tree is
**	one for the operand, plus one for the number of operators, plus
**	one for each operand, plus the size for each operand node.
**
** Input:
**	expr	{OSNODE *}  OSL parser expression tree.
**
** History:
**	09/86 (jhw) -- Written.
*/
static i4
sizeExpr (expr)
register OSNODE	*expr;
{
	switch (expr->n_token)
	{
	  case tkSCONST:
	  case tkXCONST:
	  case tkICONST:
	  case tkFCONST:
	  case tkDCONST:
	  case tkNULL:
	  case VALUE:
	  case tkPCALL:
		return 0;	/* accounted for by operator node */

	  case PARENS:
		return sizeExpr(expr->n_left);

	  case DOT:
		/* the leaf node has the size, assuming no errors. */
		return sizeExpr(expr->n_right);

	  case ARRAYREF:
		return sizeExpr(expr->n_left);

	  case OP:
		return 1 + 1 + 2 + 1 + sizeExpr(expr->n_left) + sizeExpr(expr->n_right);
		/* operator + no. of operands + 2 operands + result + ... */

	  case UNARYOP:
		/* Note:  Certain ADF functions such as 'username()', 'time()',
		** etc., which are mapped to UNARYOP nodes, have no arguments.
		*/
		return 1 + 1 + 1 + 1 +
			(expr->n_left != NULL ? sizeExpr(expr->n_left) : 0);
		/* operator + no. of operands + 1 operand + result + ... */

	  case RELOP:
		if (!expr->n_right)
		    /* operator + 1 operands + ... */
		    return 1 + 1 + 1 + sizeExpr(expr->n_left);

		/* operator + 2 operands + ... */
		return 1 + 1 + 2 + sizeExpr(expr->n_left) + sizeExpr(expr->n_right);

	  case LOGOP:
		{
			i4 size;

			size = sizeExpr(expr->n_left) + 2;
			if (osw_compare(expr->n_value, _Or) 
			  || (osw_compare(expr->n_value, _And) 
			      && expr->n_right != NULL))
			{ /* "or" or "and" but not terminating "and" */
				size += sizeExpr(expr->n_right);
			}
			return size;
			/* operator + ... */
		}

	  default:
			osuerr(OSBUG, 1, ERx("sizeExpr()"));
			/*NOTREACHED*/
	} /* end switch */
}

/*
** Name:	evalBool() -	Evaluate Boolean Expression Into IL Code.
**
** Description:
**	Generates IL code to evaluate an OSL parser boolean expression into
**	the input result.  It will be called twice; once to generate any
**	non-boolean sub-expressions, which guarantees that they will be
**	executed no matter what path the boolean evaluation follows; and the
**	second and last time, to generate the boolean expression, itself.
**
** Input:
**	expr	{OSNODE *}  OSL parser expression tree.
**	result	{bool}  Generate a boolean expression, otherwise generate
**			non-boolean sub-expressions.
**
** History:
**	08/89 (jhw) -- Written.  (For JupBug #7187.)
**	01/90 (jhw) -- Removed special hack for ESCAPE operator operand length
**		(see fix below in 'instr_gen()'.)  Was causing JupBug #9796.
**	06/90 (jhw) -- Corrected RELOP and NULLOP instruction generation to only
**		be when 'result' is TRUE.  (Sub-expressions will be generated
**		otherwise if the temporaries are NULL.)
*/
static VOID
evalBool ( expr, result )
register OSNODE	*expr;
bool		result;
{
	switch (expr->n_token)
	{
	  case PARENS:
		evalBool(expr->n_left, result);
		break;

	  case RELOP:
		if ( osw_compare(expr->n_value, _Escape) )
		{ /* Hack attack: special case for ESCAPE */
			if ( result )
			{ /* generate ESCAPE first when evaluating */
				OSSYM *tmp;

				/* 
				** Right child is a string constant:  the
				** escape character.  We have to generate this
				** instruction BEFORE generating the LIKE one.
				*/

				tmp = evalExpr(expr->n_right, (OSSYM *)NULL);

				instr_gen(expr->n_fiid, 0, 1, tmp);
			}
			/* Left child is the LIKE statement.  Generate it. */
			evalBool(expr->n_left, result);
		}
		else
		{
			/* Note:  All RELOPs are binary - well were once.
			** Postfix operators, of which IS NULL is one, are unary */
			if ( !result )
			{
				/* evaluate sub-expressions */
				if ( expr->n_lhstemp == NULL && expr->n_left)
					expr->n_lhstemp =
					  evalExpr(expr->n_left, (OSSYM *)NULL);
				if ( expr->n_rhstemp == NULL && expr->n_right)
					expr->n_rhstemp =
					  evalExpr(expr->n_right, (OSSYM*)NULL);
			}
			else
			{
				if (expr->n_right)
					instr_gen( expr->n_fiid, 0,
						2, expr->n_lhstemp, expr->n_rhstemp);
				else
					instr_gen( expr->n_fiid, 0,
						1, expr->n_lhstemp, (OSSYM *)NULL);
			}
		}
		break;

	  case LOGOP:
		evalBool(expr->n_left, result);
		if ( result )
			instr_gen(expr->n_fiid, 0, 0);
		if ( osw_compare(expr->n_value, _Or) 
			|| ( osw_compare(expr->n_value, _And)
				&& expr->n_right != NULL ) )
		{ /* "or" or "and" but not terminating "and" */
			evalBool(expr->n_right, result);
		}
		break;

	  default:
		osuerr(OSBUG, 1, ERx("evalBool()"));
		/*NOTREACHED*/
	} /* end switch */
}

/*
** Name:	evalExpr() -	Evaluate Expression Into IL Code.
**
** Description:
**	This routine generates IL code to evaluate an OSL parser expression
**	into the input result.  It assumes that the expression does not consist
**	of a simple field or constant.
**
** Input:
**	expr	{OSNODE *}  OSL parser expression tree.
**	result	{OSNODE *}  OSL symbol table entry for result.
**
** Returns:
**	{OSSYM *}  Symbol table reference for result of node.
**
** History:
**	08/86 (jhw) -- Written.
*/
static OSSYM *
evalExpr (expr, result)
register OSNODE	*expr;
OSSYM		*result;
{
	register OSSYM  *tmpsym;

	switch (expr->n_token)
	{
	  case tkSCONST:
	  case tkXCONST:
	  case tkICONST:
	  case tkFCONST:
	  case tkDCONST:
	  case tkNULL:
		return osconstalloc( (DB_DT_ID)expr->n_type, 
				     (i4)expr->n_length, (i2)expr->n_prec,
				     (char *)expr->n_const);

	  case VALUE:
		if (result == NULL || result->s_type == expr->n_type)
			return expr->n_sym;
		else
		{ /* string */
			instr_gen(oschkcoerce(expr->n_type, result->s_type),
				(i4) expr->n_sym->s_length, 2, 
				result, expr->n_sym
			);
			return result;
		}

	  case PARENS:
		return evalExpr(expr->n_left, result);

	  case ARRAYREF:
	  case DOT:
		if (result == NULL || result->s_type == expr->n_type)
		{
			/* this is where we stashed the temp that we
			** dereference a record or array into.
			*/
			return expr->n_lhstemp;
		}
		else
		{ 
			instr_gen(oschkcoerce(expr->n_type, result->s_type),
				(i4) expr->n_lhstemp->s_length, 2, 
				result, expr->n_lhstemp
			);
			return result;
		}

	  case OP:
		if (oschkstr(expr->n_type) && osw_compare(expr->n_value, _Plus))
		{
			/* Note:  Order of evaluation matters for String 
			** concatenation 
			*/

			OSSYM	*left;

			if (result == NULL)
				result = osdatalloc(expr->n_type, 
						    expr->n_length, 
						    expr->n_prec);
			/*
			** Note: Do not pass down 'res' when evaluating 
			** expressions of type DB_CHA_TYPE.  DB_CHA_TYPE are 
			** fixed length with blank-padding upto the full 
			** length and cannot be used for intermediate results.
			*/
			left = evalExpr(expr->n_left, (
				!ADE_CHK_TYPE(result->s_type, DB_CHA_TYPE) &&
				result->s_type == expr->n_left->n_type 
						      ) ? result : NULL
			       );
			instr_gen(expr->n_fiid, expr->n_length, 3, result, left,
				evalExpr(expr->n_right, (OSSYM *)NULL)
			);
			tmpsym = result;
		}
		else
		{
			tmpsym = osdatalloc(expr->n_type, 
					    expr->n_length, 
					    expr->n_prec);
			instr_gen(expr->n_fiid, expr->n_length, 3, tmpsym,
				evalExpr(expr->n_left, (OSSYM *)NULL),
				evalExpr(expr->n_right, (OSSYM *)NULL)
			);
		}
		return tmpsym;

	  case UNARYOP:
		if ( expr->n_fiid == ADE_PMENCODE )
		{
			tmpsym = evalExpr(expr->n_left, (OSSYM *)NULL);
			instr_gen( expr->n_fiid, expr->n_length, 1, tmpsym); 
		}
		else
		{
			tmpsym = osdatalloc(expr->n_type, 
					    expr->n_length, 
					    expr->n_prec);
			if (expr->n_left == NULL)
			{ /* special case internal ADF functions (e.g.,'left')*/
				instr_gen( expr->n_fiid, expr->n_length,
						1, tmpsym
				);
			}
			else
			{
				instr_gen( expr->n_fiid, expr->n_length,
					   2, tmpsym,
					   evalExpr(expr->n_left, (OSSYM *)NULL)
				);
			}
		}
		return tmpsym;

	  case tkPCALL:
		tmpsym = osdatalloc(
					(expr->n_type == DB_VCH_TYPE 
					&& expr->n_rlen == ADE_LEN_UNKNOWN)
						? DB_DYC_TYPE : expr->n_type,
					expr->n_rlen, expr->n_prec
				   );
		ostmpbeg();

		/* build param structure and call procedure */
		osgencall(expr, tmpsym);	
		ostmpend();
		return tmpsym;

	  default:
		osuerr(OSBUG, 1, ERx("evalExpr()"));
		/*NOTREACHED*/
	} /* end switch */
}

/*
** Name:	instr_gen() -   Generate an ADE Instruction for an IL Expression.
**
** Description:
**	Generate an ADE instruction as part of an IL expression from an operator
**	name and OSL parser symbol references.  This routine sets up an ADE
**	operand array from the symbol references and calls ILG to generate the
**	ADE instruction (assuming 'IGbgnExpr()' has been called) for the named
**	operator.
**
** Input:
**	oper	{i2}   fiid of instruction.
**	len	{nat}  Length of the operation result.
**	nops	{nat}  Number of operands.
**	op1..op2 {OSSYM *}  OSL symbol references for each operands.
**
** Side Effects:
**	Calls 'IGgenInstr()', which generates the ADE instruction as
**	part of an IL expression.
**
** History:
**	04/87 (jhw) -- Written.
**	01/90 (jhw) -- Added special check when setting
**				ADE_ESC_CHAR operator length.  JupBug #9796.
*/
static i4  punt_len();

/*VARARGS4*/
static VOID
instr_gen (oper, len, nops, op1, op2, op3)
i2	oper;
i4	len;
i4	nops;
OSSYM	*op1,
	*op2,
	*op3;
{
	ADE_OPERAND	ops[3];

	if (nops > 0)
	{
		ops[0].opr_dt = ( op1->s_type == DB_DYC_TYPE )
					? DB_VCH_TYPE : op1->s_type;
		ops[0].opr_prec = op1->s_scale;
		if ( (ops[0].opr_len = op1->s_length) == 0
				|| len == ADE_LEN_UNKNOWN )
		{
			/* Special case:  ADF expects the length to be
			** one for the special ADE_ESC_CHAR operator.
			*/
			ops[0].opr_len = ( oper == ADE_ESC_CHAR )
						? 1 : ADE_LEN_UNKNOWN;
		}
		ops[0].opr_base = op1->s_ilref;
		if (nops > 1)
		{
			ops[1].opr_dt = ( op2->s_type == DB_DYC_TYPE )
						? DB_VCH_TYPE : op2->s_type;
			ops[1].opr_prec = op2->s_scale;
			if ((ops[1].opr_len = op2->s_length) == 0)
			{ /* constants of type int, flt, or a string type */
				if (op2->s_type == DB_INT_TYPE)
					ops[1].opr_len = sizeof(i4);
				else if (op2->s_type == DB_FLT_TYPE)
					ops[1].opr_len = sizeof(f8);
				else if (op2->s_type == DB_DEC_TYPE)
				{
					/* supply a valid length and precision
					** so ADE doesn't reject.  Currently,
					** this length/precision doesn't have
					** to reflect reality.  Good thing,
					** as it's not convenient to get at
					** the constant's len/precision given
					** just the OSSYM.  Now if we had
					** the OSNODE it would be easier...
					*/
					ops[1].opr_len = DB_MAX_DECLEN;
					ops[1].opr_prec = 
						DB_PS_ENCODE_MACRO(
							DB_MAX_DECPREC, 0);
				}
				else
				{ /* a string type constant */
					if ( op2->s_type == op1->s_type
						     || len == ADE_LEN_UNKNOWN )
					{
						ops[1].opr_len = ops[0].opr_len;
					}
					else
					{/* size irrelevant but must be legal */
						ops[1].opr_len =
							punt_len(op2->s_type);
					}
				}
			}
			ops[1].opr_base = op2->s_ilref;
			if (nops > 2)
			{
				ops[2].opr_dt = ( op3->s_type == DB_DYC_TYPE )
						    ? DB_VCH_TYPE : op3->s_type;
				ops[2].opr_prec = op3->s_scale;
				if ((ops[2].opr_len = op3->s_length) == 0)
				{ /* cons. of type int, flt, or a string type */
					if (op3->s_type == DB_INT_TYPE)
						ops[2].opr_len = sizeof(i4);
					else if (op3->s_type == DB_FLT_TYPE)
						ops[2].opr_len = sizeof(f8);
					else if (op3->s_type == DB_DEC_TYPE)
					{
						/* See DB_DEC_TYPE special
						** handling comment above
						*/
						ops[2].opr_len = DB_MAX_DECLEN;
						ops[2].opr_prec = 
							DB_PS_ENCODE_MACRO(
								DB_MAX_DECPREC,
								0);
					}
					else
					{ /* a string type constant */
						if ( op3->s_type == op1->s_type
						      || len ==ADE_LEN_UNKNOWN )
						{
							ops[2].opr_len =
								ops[0].opr_len;
						}
						else
						{ /* size irrelevant but ...
								must be legal */
							ops[2].opr_len =
							  punt_len(op3->s_type);
						}
					}
				}
				ops[2].opr_base = op3->s_ilref;
			}
		}
	}
	IGgenInstr(oper, nops, ops);
}

/* a legal size for a string constant or temporary */
static i4
punt_len ( type )
DB_DT_ID	type;
{
	register i4	ret = DB_CNTSIZE;

	if (AFE_NULLABLE(type))
		++ret;
	return ret;
}
