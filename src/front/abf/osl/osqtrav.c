/*
** Copyright (c) 1986, 2008, 2009 Ingres Corporation
*/

#include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
#include	<cm.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<oslconf.h>
#include	<oserr.h>
#include	<ossym.h>
#include	<ostree.h>
#include	<osquery.h>
#include	<oskeyw.h>
#include	"traverse.h"

/**
** Name:	osqtrav.c -	OSL Parser DML Expression Tree Traverse Module.
**
** Description:
**	Contains routines used to traverse a DML expression tree and to apply
**	the functions specified by the traverse function structure at various
**	points in the traversal.  Among other things, these functions can be
**	used to generate code for the expression tree or to count the number
**	of variables used.  Defines:
**
**	osqtrvfunc()	traverse DML expression tree.
**	ostrvqry()	traverse SQL sub-select DML expression node.
**	
**	Following function no longer called: see comment 07-jun-2006.
**	ostrvfromlist()	traverse DML comma expression as SQL from list.
**
** History:
**	Revision 5.1  86/11/04  15:11:43  wong
**	Initial revision  86/09  wong
**
**	Revision 6.0  87/06  wong
**	Added support for SQL and new QG module.  Also added support for
**	predicates in all DML WHERE clauses.
**
**	Revision 6.1  88/08  wong
**	Modified to use traversal application object.
**
**	Revision 6.4
**	03/13/91 (emerson)
**		Separate arguments in lists in queries by ", " instead of ","
**		so that the back-end won't flag a syntax error if
**		II_DECIMAL = ','.
**
**	Revision 6.5
**	15-jun-92 (davel)
**		minor modifications for decimal support.
**
**      Revision 6.6
**      29-Jan-98 (hanal04)
**              Provide for the NLIST case in osqtrvfunc(). In a CREATE
**              RULE on UPDATE (col-list) the col-list is a legitimate
**              NLIST case. (b86791). Cross integration of 433204.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-jun-2006 (smeke01) Bug 116193
**		Removed last call to function ostrvfromlist(). Left
**		function in for documentation history purposes.
**		This could be removed at a suitable later date.
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	07-Jun-2009 (kiria01) b122185
**	    Added code to reconstitue SQL CASE/IF expressions
*/

static const char	_EmptyStr[]	= ERx("");
static const char	_Dot[]		= ERx(".");
static const char	_By[]		= ERx(" by ");
static const char	_Where[]	= ERx(" where ");
static const char	_Not_[]		= ERx(" not ");
static const char	_Space[]	= ERx(" ");
static const char	_Lparen[]	= ERx("(");
static const char	_Rparen[]	= ERx(")");
static const char	_Comma[]	= ERx(", ");
static const char	_CaseStr[]	= ERx(" case ");
static const char	_ThenStr[]	= ERx(" then ");
static const char	_WhenStr[]	= ERx(" when ");
static const char	_ElseStr[]	= ERx(" else ");
static const char	_EndStr[]	= ERx(" end ");

FUNC_EXTERN     VOID    set_onclause_ref(OSNODE *node);
/* b86791 - Prototype osdblist() in order to provide evaluation of NLIST */
VOID    osdblist();
bool	may_be_field = FALSE;
bool	must_be_field = FALSE;
bool	under_select = FALSE;

/*{
** Name:	osqtrvfunc() -	Traverse DML Expression Tree.
**
** Description:
**	Traverse a DML expression tree applying the evaluation functions as
**	appropriate.
**
** Input:
**	func	{TRV_FUNC *}  Traversal application functions.
**	tree	{OSNODE *} DML expression tree.
**
** History:
**	09/86 (jhw) -- Modified from 5.0 OSL compiler.
**	06/87 (jhw) -- Added general predicate support and query support.
**	08/88 (jhw) -- Modified to pass application functions through
**			structure.
**	03/90 (jhw) -- Modified to output "<>" for _NotEqual in SQL.
**		JupBug #20789.
**	1/94  (donc) - Added owner.table.column support when traversing the
**		       query tree. A DOT node may now have a right leaf that 
**		       contains another DOT node. A DOT node STILL maynot
**		       contain a DOT node for its left leaf node.
**		       Bug 59076.
**  25-july-97(rodjo04) 
**           Bug 82837. Added two external TRV_FUNC pointers for       
**           switch of on_clause. If a Join..On clause is encountered         
**           qevfunc will point to the appropriate function. 
**           Also check to see if we are in a Join..on clause, if so then
**           set on_clause to SELECT_JOIN_ON.
**      29-Jan-98 (hanal04)
**              Provide for the NLIST case in osqtrvfunc(). In a CREATE
**              RULE on UPDATE (col-list) the col-list is a legitimate
**              NLIST case. (b86791). Cross integration of 433204.
**  10-mar-2000 (rodjo04) BUG 98775.
**      Removed above change ( 25-july-97 bug 82837). 
**      Now when we come across a from list we will evaluate the variable 
**      correctly. Added support for all datatypes. 
**  27-nov-2001  (rodjo04) Bug 106438
**      Again, I am removing the above change. This caused more problems
**      then it solved. I have created a new type of node. We will now check 
**      for ONCLAUSE. 
**  07-jun-2006 (smeke01) Bug 116193
**	Replaced call to function set_onclause() with the equivalent call to
**	osqtrvfunc(), thus making the call fully recursive and able to pass
**	down the appropriate TRV_FUNC pointer from the caller.
**	NOTE: I strongly suspect that the call to set_onclause_ref() can be
**	removed and handled by the single call to osqtrvfunc(). However I 
**	don't have the time to confirm this and it is not required to fix
**	this bug.
** 20-Jul-2007 (kibro01) b118760
**	Further refinement of fix for b118229 to ensure that we only
**	change previous behaviour if we are in a query - if we are, then
**	parameters are assumed to be column/table names unless on the
**	right hand side of a boolean operator (e.g. WHERE col = :val)
**  4-Sep-2007 (kibro01) b118760/b119054
**      Ensure that II_ABF_ASSUME_VALUE=Y still allows table names to
**      be unquoted, since there cannot be values there
** 21-Jan-2008 (kiria01) b119806
**	Extended grammar for postfix operators beyond IS NULL requiring
**	RELOP supporting unary form
*/

VOID
osqtrvfunc ( func, tree )
register TRV_FUNC	*func;
register OSNODE		*tree;
{
	register VOID	(*prfunc)();
	register VOID	(*evfunc)();
	bool	prev_may, prev_must;

    VOID	ostrvqry();

    if (tree == NULL)
		osuerr(OSNULLVAL, 1, ERx("empty tree"));

        prfunc = func->print;
    	evfunc = func->eval;

	switch (tree->n_token)
	{
	  case VALUE:
		(*evfunc)(tree);
		break;
	  case ATTR:
		(*prfunc)(tree->n_name);
		(*prfunc)(_Dot);
		(*prfunc)(tree->n_attr);
		break;

	  case tkID:
		/* Note: the n_left of a tkID node is the stuff to the *right*
		** of the ID; can be confusing...
		*/
		(*prfunc)(tree->n_value == NULL ? _EmptyStr : tree->n_value);
		if (tree->n_left != NULL)
		{
			if ( tree->n_left->n_token != PARENS )
				(*prfunc)(_Space);
			osqtrvfunc( func, tree->n_left );
		}
		break;

	  case tkCASE:
		(*prfunc)(_CaseStr);
		if (tree->n_right)
		    osqtrvfunc (func, tree->n_right);
		osqtrvfunc (func, tree->n_left);
		(*prfunc)(_EndStr);
		break;
	  case tkWHEN:
		if (tree->n_left)
		    osqtrvfunc (func, tree->n_left);
		if (tree->n_right)
		{
		    if (tree->n_right->n_left)
		    {
			(*prfunc)(_WhenStr);
			osqtrvfunc (func,tree->n_right->n_left);
			(*prfunc)(_ThenStr);
			osqtrvfunc (func,tree->n_right->n_right);
		    }
		    else
		    {
			(*prfunc)(_ElseStr);
			osqtrvfunc (func,tree->n_right->n_right);
		    }
		}
		break;
	  case tkXCONST:
		(*prfunc)(ERx("X"));
	  case tkSCONST:
		(*prfunc)(osQuote);
		(*prfunc)(tree->n_const);
		(*prfunc)(osQuote);
		break;

	  case tkDCONST:
	  case tkICONST:
	  case tkFCONST:
		(*prfunc)(tree->n_const == NULL ? ERx("0") : tree->n_const);
		break;

	  case tkNULL:
		(*prfunc)(tree->n_const);
		break;

	  case AGGLIST:
	  { /* QUEL only */
		TRV_FUNC	agg_ev;

		agg_ev.print = prfunc;
		agg_ev.eval = evfunc;
		agg_ev.pred = func->pred;
		agg_ev.tle = NULL;
		agg_ev.name = NULL;

		osqtrvfunc( &agg_ev, tree->n_agexpr );
		if ( tree->n_agbylist != NULL )
		{ /* BY name list (which are <DMLexpr>s.) */
			(*prfunc)(_By);
			osqtrvfunc( &agg_ev, tree->n_agbylist );
		}
		if ( tree->n_agqual != NULL )
		{
			register OSNODE	*wnode;

			wnode = tree->n_agqual;
			if ( wnode->n_token == tkID )
			{ /* possible ONLY modifier */
				(*prfunc)(_Space);
				(*prfunc)(wnode->n_value);
				wnode = wnode->n_left;
			}
			(*prfunc)(_Where);
			if ( wnode->n_token != VALUE )
				osqtrvfunc( &agg_ev, wnode );
			else
				(*evfunc)(wnode);
		}
		break;
	  }

	  case RELOP:
		if ( SQL && tree->n_value == _NotEqual )
			tree->n_value = ERx("<>");
		/* fall through ... */
	  case OP:
		prev_must = must_be_field;
		must_be_field = FALSE;
		if (tree->n_left)
			osqtrvfunc( func, tree->n_left );
		if (CMalpha(tree->n_value))
		{ /* lexically distinguish words such as "like" and "between" */
			if (tree->n_left)
				(*prfunc)(_Space);
			(*prfunc)(tree->n_value);
			if (tree->n_right)
				(*prfunc)(_Space);
		}
		else
		{
			(*prfunc)(tree->n_value);
		}
		/* r.h.s. of a comparison operator - this is the only place
		** we assume a parameter will be a literal value rather than
		** a column or table name (kibro01) b118760
		*/
		if (tree->n_right)
		{
			prev_may = may_be_field;
			may_be_field = FALSE;
			osqtrvfunc( func, tree->n_right );
			may_be_field = prev_may;
		}
		must_be_field = prev_must;
		break;

	  case LOGOP:
		if ( osw_compare(tree->n_value, _Not) )
		{
			(*prfunc)(_Not_);
			osqtrvfunc( func, tree->n_left );
		}
		else
		{ /* AND or OR */
			osqtrvfunc( func, tree->n_left );
			(*prfunc)(_Space);
			(*prfunc)(tree->n_value);
			(*prfunc)(_Space);
			osqtrvfunc( func, tree->n_right );
		}
		break;

	  case UNARYOP:
		if (tree->n_right)
			osqtrvfunc( func, tree->n_right );
		if (CMalpha(tree->n_value))
		{ /* lexically distinguish words such as "like" and "between" */
			if (tree->n_left)
				(*prfunc)(_Space);
			(*prfunc)(tree->n_value);
			if (tree->n_right)
				(*prfunc)(_Space);
		}
		else
		{
			(*prfunc)(tree->n_value);
		}
		if (tree->n_left)
			osqtrvfunc( func, tree->n_left );
		break;

	  case PARENS:
		(*prfunc)(_Lparen);
		if ( tree->n_left != NULL )
			osqtrvfunc( func, tree->n_left );
		(*prfunc)(_Rparen);
		break;

	  case COMMA:
		osqtrvfunc( func, tree->n_left );
		(*prfunc)(_Comma);
		osqtrvfunc( func, tree->n_right );
		break;

	  case DOT:
		if ( tree->n_right->n_token != tkID 
		  && tree->n_right->n_token != DOT )
			(*evfunc)(tree);
		else if ( tree->n_right->n_token != DOT )
		{ /* ID '.' ID node */
			osqtrvfunc( func, tree->n_left );
			(*prfunc)(_Dot);
			osqtrvfunc( func, tree->n_right );
		}
		else 
		{ /* ID '.' ID '.' ID node */
			osqtrvfunc( func, tree->n_left );
			(*prfunc)(_Dot);
			osqtrvfunc( func, tree->n_right->n_left );
			(*prfunc)(_Dot);
			osqtrvfunc( func, tree->n_right->n_right );
		}
		break;

	  case BLANK:
		osqtrvfunc( func, tree->n_left );
		(*prfunc)(_Space);
		osqtrvfunc( func, tree->n_right );
		break;

	  case ASNOP:
		osqtrvfunc( func, tree->n_left );
		(*prfunc)(_Space);
		(*prfunc)(tree->n_value);
		(*prfunc)(_Space);
		osqtrvfunc( func, tree->n_right );
		break;

	  case NPRED:
		/* Note:  Only WHERE clauses should have
		**	  predicate (QUALIFICATION) nodes.
		*/
		if ( func->pred == NULL )
			oscerr(OSEPRED, 0);
		else
			(*func->pred)(tree);
		break;

	  case tkQUERY:
		if ( func->tle == NULL )
			osuerr(OSBUG, 1, ERx("osqtrvfunc(sub-query)"));
		else
		{ /* Evaluate SQL sub-select */
			ostrvqry( func, tree->n_query );
		}
		break;

          /* b86791 - the update clause of create rule may contain a */
          /*          field list which must be evaluated.            */
          case NLIST:
                if ( tree->n_left != NULL )
                   osevallist(tree, osdblist);
                else
                   osuerr(OSBUG, 1, ERx("osqtrvfunc(nlist)"));
                break;

	  case ONCLAUSE:
                if (func->state == 1)
                   set_onclause_ref(tree->n_left);
                else
                   osqtrvfunc( func, tree->n_left );
                break;
	     
	  case PREDE:
	  case DOTALL:
	  case tkBYREF:
	  case tkASSIGN:
	  case tkPCALL:
	  default:
		osuerr(OSBUG, 1, ERx("osqtrvfunc(default)"));
		/*NOTREACHED*/
	} /* end switch */
}

/*{
** Name:	ostrvqry() -	Traverse SQL Sub-select DML Expression Node.
**
** Description:
**	Traverse an SQL select expression tree applying the input functions
**	where appropriate.
**
** Input:
**	qry	{OSQRY *}  Query node.
**	func	{TRV_FUNC *}  Traversal application object.
**
** History:
**	05/87 (jhw) -- Written.
** 
**  25-july-97 (rodjo04)
**           bug 82837. Added external variable on_clause. See oslconf.h
**           for details. Also added use of Reset_on macro(). 
**  27-nov-2001 (rodjo04) Bug 106438
**            on_clause and Reset_on macro were removed with bug fix 98775.
**  07-jun-2006 (smeke01) Bug 116193
**	Replaced call to function ostrvfromlist() with call to  
**	osqtrvfunc(), thus making the call fully recursive and able to pass
**	down the appropriate TRV_FUNC pointer from the caller.
**  02-May-2007 (kibro01) b118229
**	As a result of the fix for bug 116193, qeval rather than eval
**	is run for FROM lists.  This change allows us to check if we are in
**	a FROM list and uses QS_VALUE/IL_DBVAL instead of QS_QRYVAL/IL_DBVAR
**	to avoid quotes being put around table names.
** 20-Jul-2007 (kibro01) b118760
**	Further refinement of fix for b118229 to ensure that we only
**	change previous behaviour if we are in a query - if we are, then
**	parameters are assumed to be column/table names unless on the
**	right hand side of a boolean operator (e.g. WHERE col = :val)
**  4-Sep-2007 (kibro01) b118760/b119054
**      Ensure that II_ABF_ASSUME_VALUE=Y still allows table names to
**      be unquoted, since there cannot be values there
** 21-Aug-2008 (kibro01) b120792/b119076/b118760
**	Ensure thet II_ABF_ASSUME_VALUE=Y does not affect "WHERE :a"
*/

VOID
ostrvqry ( func, qry )
register TRV_FUNC	*func;
register OSQRY		*qry;
{
	register VOID	(*prfunc)() = func->print;
	bool prev_under, prev_may;

	if ( qry->qn_type == tkQUERY )
	{
		(*prfunc)(ERx("select "));
		if ( qry->qn_unique != NULL && *qry->qn_unique != EOS )
		{
			(*prfunc)(qry->qn_unique);
			(*prfunc)(_Space);
		}
		if ( func->tle == NULL )
			osuerr(OSBUG, 1, ERx("ostrvqry(target list)"));
		ostltrvfunc( qry->qn_tlist, func->tle );

		prev_under = under_select;
		prev_may = may_be_field;
		may_be_field = TRUE;
		under_select = TRUE;

		if ( qry->qn_from != NULL )
		{ /* from list */
			(*prfunc)(ERx(" from "));
			must_be_field = TRUE;
			osqtrvfunc(func, qry->qn_from);
			must_be_field = FALSE;
			if ( qry->qn_where != NULL )
			{
				(*prfunc)(_Where);
				if ( qry->qn_where->n_token != VALUE )
				{
					osqtrvfunc( func, qry->qn_where );
				}
				else
				{ /* where :var must evaluate as `name'
						not value */
					must_be_field = TRUE;
					(*func->name)(qry->qn_where);
					must_be_field = FALSE;
				}
			}
			must_be_field = TRUE;
			if ( qry->qn_groupby != NULL )
			{
				TRV_FUNC	names;

				names.print = prfunc;
				names.eval = func->name;
				names.pred = NULL;
				names.tle = NULL;
				names.name = NULL;
				(*prfunc)(ERx(" group by "));
				osqtrvfunc( &names, qry->qn_groupby );
			}
			if ( qry->qn_having != NULL )
			{
				(*prfunc)(ERx(" having "));
				osqtrvfunc( func, qry->qn_having );
			}
			must_be_field = FALSE;
		} /* end `from' clause */
		may_be_field = prev_may;
		under_select = prev_under;
	}
	else if (qry->qn_type == PARENS)
	{
		(*prfunc)(_Lparen);
		ostrvqry( func, qry->qn_left );
		(*prfunc)(_Rparen);
	}
	else if (qry->qn_type == OP)
	{
		ostrvqry( func, qry->qn_left );
		(*prfunc)(_Space);
		(*prfunc)(qry->qn_op);	/* UNION or UNION ALL, etc. */
		(*prfunc)(_Space);
		ostrvqry( func, qry->qn_right );
	}
	else
	{
		char	buf[32];

		osuerr(OSBUG, 1,
			STprintf(buf, ERx("ostrvqry(type=%d)"), qry->qn_type)
		);
	}
}

/*{
** Name:	ostrvfromlist() -	Traverse DML Comma Expression as From List.
**
** Description:
**	Traverse a DML comma expression list for an SQL from list.
**
**	Note that ID nodes with left elements specify table correlations and
**	are handled specially.  (The correlation name is the value with the
**	table name being the left element even though specified as
**	"table correlation".)
**
** Input:
**	list	{OSNODE *}  DML comma expression list.
**	prfunc	{nat (*)()} Address of output function.
**	evfunc	{nat (*)()} Address of evaluation function.
**
** History:
**	05/87 (jhw) -- Written.
**      Now when we come across a from list we will evaluate the variable 
**      correctly. Added support for all datatypes. 
**
**  07-jun-2006 (smeke01) Bug 116193
**	Removed last call to function ostrvfromlist(). Left function in 
**	for documentation history purposes. This could be removed at a 
**	suitable later date.
*/

ostrvfromlist(list, prfunc, evfunc)
register OSNODE	*list;
register VOID	(*prfunc)();
register VOID	(*evfunc)();
{
	
	if (list->n_token == tkID)
	{ /* table correlations */
		if (list->n_left != NULL)
		{
			(*evfunc)(list->n_left);
			(*prfunc)(_Space);
		}
		(*prfunc)(list->n_value);
	}
	else if (list->n_token != COMMA)
		(*evfunc)(list);
	else
	{ /* comma node */
		ostrvfromlist(list->n_left, prfunc, evfunc);
		(*prfunc)(_Comma);
		ostrvfromlist(list->n_right, prfunc, evfunc);
	}
	
}
