/*
** Copyright (c) 1986, 2008 Ingres Corporation
*/

#include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
#include	<me.h>
#include	<cm.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<abqual.h>
#include	<oslconf.h>
#include	<oserr.h>
#include	<ossym.h>
#include	<ostree.h>
#include	<osquery.h>
#include	<oskeyw.h>
#include	<iltypes.h>
#include	<ilops.h>
#include	"traverse.h"
#include	<eqstmt.h>
#include	<osglobs.h>

/**
** Name:	osdbgen.c -	OSL Translator DBMS Statements Generator Module.
**
** Description:
**	Contains routines used to generate IL instructions to pass values
**	evaluated for DBMS statements to the database during interpretation
**	by the OSL interpreter.  Defines:
**
**	osdbeval()	evaluate ingres reference node for dbms statement.
**	osdbqeval()	evaluate ingres variable node for dbms statement.
**	osdblist()	evaluate node of a list for dbms statement.
**	osdbtle()	generate code for dbms target list element.
**	osdbqtle()	generate code for dbms query target list element.
**	osdbsrtkey()	generate code for sort/key specification.
**	osdbstr()	generate code for db string.
**	osdbflush()	generate code to output strings in buffer to db.
**	osqtraverse()	traverse DML query expression tree generating il code.
**	osqwhere()	traverse DML where expression tree generating il code.
**	osdbqry()	evaluate sub-select query node for dbms statement.
**	osdbsqltle()	generate code for SQL target list element.
**	osdbfrom()	generate code for SQL from list.
**      IIOSgqsGenQryStmt       Generate the first IL instruction
**                              for a possible repeat query statement.
**      IIOSgqiGenQryId Generate a query id in an IL_QID statement.
**	osdbveval()	evaluate ingres var. string ref. node for dbms statement
**	osdbtle()	generate code for dbms with list element.
**	osdbwith()	generate code for dbms with list.
**
** History:
**	Revision 5.1  86/09/09  17:02:54  wong
**	Initial revision
**
**	Revision 6.0  87/06  wong
**	Added support for SQL DML code generation.
**
**	Revision 6.3  1990/02/13  Joe
**	Added IIOSgqsGenQryStmt and IIOSgqiGenQryId to fix JupBugs 10015.
**
**	Revision 6.3/03/00  90/09  wong
**	Added 'osdbveval()', 'osdbwtle()', and 'osdbwith()' for bugs #31075,
**	#31342, and #31614, and support of role, group, and user DDL statements
**	in general.
**	11/14/90 (emerson)
**		Check for global variables (OSGLOB)
**		as well as local variables (OSVAR).  (Bug 34415).
**
**	Revision 6.4
**	03/13/91 (emerson)
**		Separate arguments in lists in queries by ", " instead of ","
**		so that the back-end won't flag a syntax error if
**		II_DECIMAL = ','.
**
**	Revision 6.5
**	15-jun-92 (davel)
**		add DCONST to osdbeval() and osdbveval() for decimal support.
**      30-aug-93 (huffman)
**              add <me.h>
**  25-july-97 (rodjo04)
**      Added variable pqry_expr, a pointer to qry_expr. 
**  10-mar-2000 (rodjo04) BUG 98775.
**      Removed above change (bug 82837). 
**      Now when we come across a from_list we will evaluate the variable 
**      correctly. Added support for all datatypes. 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	17-oct-2001 (somsa01)
**	    In ospredgen(), slightly modified last "for" loop such that we
**	    would not get into a situation where we would be outside of the
**	    array bounds of listnodes.
**	23-oct-2001 (somsa01)
**	    Backed out change 453178 (rodjo04) for bug 104887.
**  27-nov-2001 (rodjo04) Bug 106438
**      Again I have removed the above change(bug 82837). I have also created
**      a new function set_onclause_ref().
**      13-feb-03 (chash01) x-integrate change#461908
**      "extern" should not be used with function body. (see CL spec).
**      remove extern from set_onclause_ref().
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
*/

VOID	osdbstr();
VOID	osdbeval();
VOID	osdbqeval();
VOID	osdbveval();
VOID	osdbpred();
VOID	osdbqtle();
VOID	ospredgen();

VOID      IIOSgqsGenQryStmt();
VOID      IIOSgqiGenQryId();

static const char	_Space[]	= ERx(" ");
static const char	_Comma[]	= ERx(", ");
static const char	_Period[]   = ERx(".");
static const char	_LBrace[]   = ERx("[");
static const char	_RBrace[]   = ERx("]");

static i4	predcomp();
extern bool	may_be_field;
extern bool	must_be_field;
extern bool	under_select;

/*
** Reference (Name) Node Evaluation Object.
*/
static const
	TRV_FUNC	ref_node = {
					osdbstr,
					osdbeval,
					NULL,
					NULL,
					NULL,
					1
};

/*
** Variable String Reference (Name) Node Evaluation Object.
*/
static const
	TRV_FUNC	vref_node = {
					osdbstr,
					osdbveval,
					NULL,
					NULL,
					NULL,
					1
};

/*
** Query Expression Evaluation Object.
*/
static const
	TRV_FUNC	qry_expr = {
					osdbstr,
					osdbqeval,
					osdbpred,
					osdbqtle,
					osdbeval,
					1
};



/*{
** Name:	osdbeval() -	Evaluate Ingres Reference Node for DBMS Statement.
**
** Description:
**	This routine evaluates a node that is an Ingres reference by generating
**	an IL statement to send the node value to the database.  (An Ingres
**	reference is an OSL field or table field value [of any string type],
**	a constant identifier, or a DBMS attribute specification
**	["relation . attribute" including "all"].)
**
** Input:
**	node	{OSNODE *}  The Ingres reference node.
**
** History:
**	09/86 (jhw) -- Written.
**  25-july-97 (rodjo04) 
**         bug 82837. Now we sould check to see if we are in a sub select 
**         of Join..on clause. If we are then set on_clause to SUBSELECT_JOIN_ON.
**  10-mar-2000 (rodjo04) BUG 98775.
**      Removed above change (bug 82837).
**      Now when we come across a from_list we will evaluate the variable 
**      correctly. Added support for all datatypes.
**  27-nov-2001 (rodjo04) Bug 106438
**      Removed above fix. 
**  11-jun-2001 (rodjo04) Bug 104887
**      Added case for datatypes DB_CHA_TYPE and DB_VCH_TYPE, instead of 
**      using default so that these datatypes will be evaluated correctly.
*/

VOID
osdbeval (expr)
register OSNODE	*expr;
{
	switch (expr->n_token)
	{
	  case VALUE:
		/* send value */
		osdbflush();
		IGgenStmt(IL_DBVAL, (IGSID *)NULL, 1, expr->n_ilref);
		break;
	  case ATTR:
		osdbstr(expr->n_name);
		osdbstr(_Period);
		osdbstr(expr->n_attr);
		break;
	  case ASSOCOP:
		osdbeval(expr->n_left);
		osdbstr(_Space);
		osdbstr(expr->n_value);
		osdbstr(_Space);
		osdbeval(expr->n_right);
		break;
	  case ARRAYREF:
		osdbeval(expr->n_left);
		osdbstr(_LBrace);
		osdbeval(expr->n_right);
		osdbstr(_RBrace);
		break;
	  case DOT:
		if ( expr->n_right->n_token != tkID )
		{ /* a complex-object value */
			osdbflush();
			IGgenStmt(IL_DBVAL, (IGSID *)NULL, 1, expr->n_ilref);
		}
		else
		{
			osdbeval(expr->n_left);
			osdbstr(_Period);
			osdbeval(expr->n_right);
		}
		break;
	  case BLANK:
		osdbeval(expr->n_left);
		osdbstr(_Space);
		osdbeval(expr->n_right);
		break;
	  case tkID:
      	osdbstr(expr->n_value);
		if (expr->n_left != (OSNODE *) NULL)
		{
			/*
			** Special case for column-list in the UPDATE clause of
			** a GRANT statement.  
			** XXX This could be generalized using a BLANK node.
			*/

			if (expr->n_left->n_token != PARENS)
				osdbstr(_Space);
			osdbeval(expr->n_left);
		}
		break;
	  case tkSCONST:
		osdbstr(osQuote);
		osdbstr(expr->n_const);
		osdbstr(osQuote);
		break;
	  case tkXCONST:
	  case tkICONST:
	  case tkFCONST:
	  case tkDCONST:
	  case tkNULL:
		osdbstr(expr->n_const);
		break;
	  default:
	  /* Note:  This should include comma lists
	  ** and special operators (such as ':') only.
	  */
		osqtrvfunc( &ref_node, expr );
		break;
	} /* end switch */

	ostrfree(expr);
}
/*{
** Name:	osdbqeval() -   Evaluate Ingres Variable Node for DBMS Statement.
**
** Description:
**	Evaluates a node that is an Ingres variable by generating an IL
**	statement to send the variable to the database.  (An Ingres variable is
**	an OSL field or table field, a constant identifier, or a DB attribute
**	specification ["relation . attribute" including "all"].  Note that the
**	last two are sent by value, as is done by 'osdbeval()' above.)
**
** Input:
**	node	{OSNODE *}  The Ingres variable node.
**
** History:
**	04/87 (jhw) -- Copied from 'osdbeval()'.
**	12/89 (jhw) -- Added support for DOT nodes.
**	02-May-2007 (kibro01) b118229
**	    As a result of the fix for bug 116193, qeval rather than eval
**	    is run for FROM lists.  This change checks if we are in a FROM list
**	    and uses IL_DBVAL instead of IL_DBVAR to avoid quotes being put
**	    around table names.
**	20-Jul-2007 (kibro01) b118760
**	    Further refinement of fix for b118229 to ensure that we only
**	    change previous behaviour if we are in a query - if we are, then
**	    parameters are assumed to be column/table names unless on the
**	    right hand side of a boolean operator (e.g. WHERE col = :val)
**	4-Sep-2007 (kibro01) b118760/b119054
**	    Ensure that II_ABF_ASSUME_VALUE=Y still allows table names to
**	    be unquoted, since there cannot be values there
*/

VOID
osdbqeval (expr)
register OSNODE	*expr;
{
	extern bool osl_assume_value();

	if ( expr->n_token == VALUE || 
		( expr->n_token == DOT && expr->n_left->n_token != tkID ) )
	{ /* send variable */
		osdbflush();
		/* Normally IL_DBVAR (quoted string) unless we are in a
		** query and not in rhs of operator (kibro01) b118760/b119054
		*/
		IGgenStmt((must_be_field ||
		    (under_select && may_be_field && !osl_assume_value())) ?
			IL_DBVAL : IL_DBVAR,
			(IGSID *)NULL, 1, expr->n_ilref);
	}
	else if ( expr->n_token == ATTR )
	{
		osdbstr(expr->n_name);
		osdbstr(_Period);
		osdbstr(expr->n_attr);
	}
	else if ( expr->n_token == DOT )
		osdbeval(expr);
	else if ( expr->n_token == tkID )
		osdbstr(expr->n_value);
	else
		osdbstr(expr->n_const);

	ostrfree(expr);
}

/*{
** Name:	osdbpred() -	Generate Query Predicate (Qualification.)
**
** Description:
**	Generate query predicate IL instructions for a predicate node
**	derived from a qualifaction element.  Qualification elements are
**	recognized as predicates in DML conditional expressions by the
**	OSL parser.
**
** Input:
**	node	{OSNODE *}  Predicate node.
**
** Generates:
**		IL_QRYPRED type num
**		TL2ELM ...
**
** History:
**	10/86 (jhw) -- Written (as 'where_pred()' in "osquery.c".)
**	1/91 (Mike S) Allow qualification in tablefields.
**		      Abstracted almost all into ospredgen.
*/
VOID
osdbpred ( node )
OSNODE	*node;
{
	ospredgen(node, FALSE);
}

/*
** 	ospredgen -- Generate query predicate IL instructions for a predicate
**	node derived from a qualifaction element.
*/
VOID 
ospredgen(node, is_select)
OSNODE *node;
bool is_select;	/* TRUE if called from where_pred in osquery.c.
		** FALSE if called from osdbpred above.
		*/
{
	register OSNODE	*list;
	register i4	n = 0;
	i4		i;
	char		*nptr;
	OSNODE		**listnodes;

	if (!is_select)
	{
		if ( node->n_token != NPRED )
			osuerr(OSBUG, 1, ERx("ospredgen()"));

		osdbflush();
	}
	for (list = node->n_plist ; list != NULL ; list = list->n_next)
		++n;
	IGgenStmt(IL_QRYPRED, (IGSID *)NULL, 2, (i2)ABQ_FLD, (i2)n);

	if (n > 0)
	{
		/* 
		** Sort the list to group tablefields together.
		*/
		listnodes = (OSNODE **)
				MEreqmem((u_i4)0, 
				(SIZE_TYPE)(n * sizeof(OSNODE *)),
			     	FALSE, (STATUS *)NULL);
		if (listnodes == (OSNODE **)NULL)
			IIUGbmaBadMemoryAllocation(ERx("osdbpred()"));
		for (list = node->n_plist, i = 0;
		     list != NULL ; 
		     list = list->n_next, i++)
		{
			listnodes[i] = list;
		}
		IIUGqsort((char *)listnodes, n, sizeof(OSNODE *), predcomp);

		for (i = 0; i < n; i++)
		{
			register OSNODE	*np = listnodes[i]->n_ele;
			register OSSYM	*fld = listnodes[i]->n_ele->n_pfld;
			char		buf[FE_MAXNAME*2 + 1 + 1];
			char		buf2[FE_MAXNAME*2 + 1 + 1];

			if ( np->n_token != PREDE ||
			     (fld->s_kind != OSFIELD &&
			      fld->s_kind != OSGLOB &&
			      fld->s_kind != OSVAR &&
			      fld->s_kind != OSTABLE &&
			      fld->s_kind != OSACONST &&
			      fld->s_kind != OSCONSTANT  &&
			      fld->s_kind != OSUNDEF  &&
			      fld->s_kind != OSCOLUMN ))
				osuerr(OSBUG, 1, ERx("ospredgen(element)"));

			if (fld->s_kind == OSCOLUMN)
				nptr = STprintf(buf2, ERx("%s.%s"), 
						fld->s_parent->s_name,
						fld->s_name);
			else
				nptr = fld->s_name;

			IGgenStmt(IL_TL2ELM, (IGSID *)NULL, 2,
				IGsetConst(DB_CHA_TYPE, nptr),
				IGsetConst(DB_CHA_TYPE, 
					STprintf(buf, np->n_dbattr != NULL
						? ERx("%s.%s") : ERx("%s"),
					np->n_dbname, np->n_dbattr)
				)
			);
		}
		_VOID_ MEfree((PTR)listnodes);
	}
	IGgenStmt(IL_ENDLIST, (IGSID *)NULL, 0);
	ostrfree(node->n_plist);
}

/*{
** Name:	osdblist() -	Evaluate Node of a List for DB Statement.
**
** Description:
**	This routine evaluates an Ingres reference node that is part of a
**	list by generating an IL statement to send the node value to the
**	database.  A comma is sent after the node value, if it is not the
**	last element of the list.
**
** Input:
**	node	{OSNODE *}  The Ingres reference node.
**	comma	{bool}		TRUE if not last element in list.
**
** History:
**	09/86 (jhw) -- Written.
*/

VOID
osdblist ( expr, comma )
OSNODE	*expr;
bool	comma;
{
	osdbeval(expr);
	if ( comma )
		osdbstr(_Comma);
}

/*{
** Name:	osdbtle() -	Generate IL Code for DBMS Target List Element.
**
** Description:
**	Generates IL code to output a "simple" target list element to the
**	DBMS.  (A simple target list is a simple name association, not
**	an assignment, e.g., copy target list or create target list, etc.)
**
**	Note that this target list element contains an equals, "=", sign.
**	Use 'osdbsqltle()' if the target list element is to contain a space.
**
** History:
**	09/86 (jhw) -- Written.
*/
VOID
osdbtle (ele, value, opt, comma)
OSNODE	*ele;
OSNODE	*value;
OSNODE	*opt;
bool	comma;
{
	osdbeval(ele);
	if ( value != NULL )
	{
		osdbstr(_Equal);	/* note:  possibly QUEL specific */
		osdbeval(value);
		if ( opt != NULL )
		{
			osdbstr(_Space);
			osdbeval(opt);
		}
	}
	if ( comma )
		osdbstr(_Comma);
}

/*{
** Name:	osdbqtle() -	Generate IL Code for
**					DBMS Query Target List Element.
** Description:
**	Generates IL code to output a "query" target list element to the DBMS.
**	(A query target list is an assignment from a DBMS expression, e.g., an
**	append or retrieve/select target list, etc.)
**
**	Note that this target list element contains an equals, "=", sign (if
**	there is a 'value'.)
**
**	A target list element can have no 'value' when it is just a name, value,
**	or attribute, or an expression in SQL sub-selects.  In the later case,
**	it must be output as an expression using 'osqtrvfunc()'.
**
** History:
**	09/86 (jhw) -- Written.
**	03/90 (jhw) -- Corrected so that element VALUE nodes are also
**		processed as expressions.  JupBug #20003.
*/
/*ARGSUSED*/
VOID
osdbqtle ( ele, value, dim, comma )
OSNODE	*ele;
OSNODE	*value;
OSNODE	*dim;
bool	comma;
{
	VOID	osdbstr();
	VOID	osdbeval();

	if ( value == NULL && ( ele->n_token != tkSCONST
					&& ele->n_token != ATTR) )
	{ /* i.e., value as expression */
		osqtrvfunc( &qry_expr, ele );
	}
	else
	{ /* name `=' value */
		osdbeval(ele);
		if ( value != NULL )
		{
			osdbstr(_Equal);
			osqtrvfunc( &qry_expr, value );
		}
	}
	if ( comma )
		osdbstr(_Comma);
}

/*{
** Name:	osdbsrtkey() -	Generate Code for Sort/Key Specification.
**
** Description:
**	This routine generates code for a sort/key specification.
**
** Input:
**	name		{OSNODE *}  Node representing the sort name.
**	dir		{OSNODE *}  Node representing the sort direction.
**	separator	{char *}  DML dependent separator for name/direction.
**	comma		{bool}  Comma
**
** History:
**	09/86 (jhw) -- Written.
**	06/87 (jhw) -- Added separator parameter for SQL support.
*/
VOID
osdbsrtkey ( name, dir, separator, comma )
OSNODE	*name;
OSNODE	*dir;
char	*separator;
bool	comma;
{
	osdbeval(name);
	if ( dir != NULL )
	{
		osdbstr(separator);
		osdbeval(dir);
	}
	if ( comma )
		osdbstr(_Comma);
}

/*
** OSL Translator DB Statements Generator String Module.
**
** Description:
**	These routines generate IL code to output strings to the database.
**	These strings are part of a DB statement recognized by the OSL
**	translator.
**
**	Strings are concatenated into larger strings for efficiency, and are
**	flushed when they reach a certain size, or when code for output of a
**	variable to the DB is generated, or on completion of the code
**	generation for a DB statement.
*/

static struct {
	char	*bp;		/* buffer pointer */
	char	buf[OSBUFSIZE+1];	/* buffer */
} buf = {buf.buf, {EOS}};

/*{
** Name:	osdbstr() -	Generate IL Code for DB String.
**
** Description:
**	This routine concatenates the input string for output to the DB with
**	other strings for output upto a specified size (of the storage buffer.)
**	If these size is exceeded, this routine generates IL code to output
**	the other (previous) strings concatenated in the buffer.
**
** Input:
**	str	{char *}  The string to be output.
**
** History:
**	09/86 (jhw) -- Written.
*/
VOID
osdbstr (str)
char	*str;
{
	if ( str != NULL && *str != EOS )
	{
		register i4	s_len;

		s_len = STlength(str);
		if ( buf.bp - buf.buf + s_len >= sizeof(buf.buf) )
		{
			IGgenStmt(IL_DBCONST, (IGSID*)NULL,
				1, IGsetConst(DB_CHA_TYPE,buf.buf)
			);
			buf.bp = buf.buf;
		}
		STcopy(str, buf.bp); buf.bp += s_len;
	}
}

/*{
** Name:	osdbflush() -	Generate IL Code for Output of Strings to DB.
**
** Description:
**	Generates IL code that outputs the concatenated strings (through
**	'osdbstr()') to the DB.  This reinitializes the storage buffer.
**
** History:
**	09/86 (jhw) -- Written.
*/
osdbflush ()
{
	if ( buf.bp > buf.buf )
		IGgenStmt( IL_DBCONST, (IGSID*)NULL,
				1, IGsetConst(DB_CHA_TYPE,buf.buf)
		);
	buf.bp = buf.buf;
}

/*{
** Name:	osqtraverse() -	Traverse DML Query Expression Tree
**					Generating IL Code.
** Description:
**	Traverse an expression in a DML query context (i.e., a where clause
**	or a target list assignment expression.)
**
** Input:
**	tree	{OSNODE *}  An expression tree representing the DML query expr.
*/
VOID
osqtraverse (tree)
OSNODE	*tree;
{
	osqtrvfunc( &qry_expr, tree );
	ostrfree(tree);
}

/*{
** Name:	osqwhere() -	Traverse DML Where Expression Tree
**					Generating IL Code.
** Description:
**	Traverse an expression in a DML where clause.
**
** Input:
**	tree	{OSNODE *}  An expression tree representing the DML where expr.
*/
VOID
osqwhere (tree)
OSNODE	*tree;
{
	osdbstr(ERx(" where "));
	if (tree->n_token != VALUE)
		osqtrvfunc( &qry_expr, tree );
	else
	{
		osdbflush();
		IGgenStmt(IL_DBVAL, (IGSID *)NULL, 1, tree->n_sym->s_ilref);
	}
	ostrfree(tree);
}

/*{
** Name:	osdbqry() -	Evaluate Sub-select Query Node for DB Statement.
**
** Description:
**	This routine evaluates a query node representing an (SQL) sub-select
**	generating IL statements to send the sub-select to the DBMS.
**	
** Input:
**	qry	{OSQRY *}  Sub-select query node.
**
** History:
**	05/87 (jhw) -- Written.
*/

VOID
osdbqry (qry)
OSQRY	*qry;
{
	ostrvqry( &qry_expr, qry );
	osqryfree(qry);
}

/*{
** Name:	osdbsqltle() -	Generate IL Code for SQL DB Target List Element.
**
** Description:
**	Generates IL code to output a "simple" SQL target list element to the
**	DBMS.  (A simple SQL target list is a simple name association for
**	a create target list.)
**
** History:
**	05/87 (jhw) -- Written.
*/
VOID
osdbsqltle ( ele, value, opt, comma )
OSNODE	*ele;
OSNODE	*value;
OSNODE	*opt;
bool	comma;
{
	osdbeval(ele);
	if ( value != NULL )
	{
		osdbstr(_Space);
		osdbeval(value);
		if ( opt != NULL )
		{
			osdbstr(_Space);
			osdbeval(opt);
		}
	}
	if ( comma )
		osdbstr(_Comma);
}

/*{
** Name:	osdbfrom() -	Generate IL Code for SQL From List.
**
** Description:
**	Generates IL code for an SQL from list by calling 'ostrvfromlist()'
**	with the DB output functions.
**
** History:
**	06/87 (jhw) -- Written.
**	08-sep-92 (davel)
**		Changed to use osqtrvfunc for outer join support.
*/
VOID
osdbfrom (fromlist)
OSNODE	*fromlist;
{
	osqtrvfunc( &qry_expr, fromlist );
	ostrfree(fromlist);
}

/*{
** Name:	IIOSgqsGenQryStmt - Generate topmost IL for possible repeat qry.
**
** Description:
**	This generates the topmost IL for a possible repeated query.  A
**	possible repeat query statement looks like:
**
**	IL_XXX flagIsRepeated
**	[IL_QID strvalQueryName intvalQryId1 intvalQryId2]
**
**	The IL_QID is only generated if the query is a repeat query.
**
** Inputs:
**	operator	This is the IL_XXX value for the query statement
**			being generated.  This must be a repeatable
**			query statement.  The current repeatable query
**			IL operators are:
**			    IL_APPEND, IL_DELETE, IL_REPLACE, IL_DELETEFRM,
**			    IL_INSERT, and IL_UPDATE.
**			NOTE: THIS ROUTINE DOES NOT CHECK THAT OPERATOR IS
**			ONE OF THESE. THE CALLER MUST ENSURE THAT.
**
**	isrepeated	This is bool that should be set TRUE if the statement
**			is to be the repeat variant, FALSE otherwise.
**
** Side Effects:
**	Generates IL statements using IGgenStmt.
**
** History:
**	12-feb-1990
**	    First Written
*/
VOID
IIOSgqsGenQryStmt(operator, isrepeated)
IL	operator;
bool	isrepeated;
{
    IGgenStmt(operator, (IGSID *) NULL, 1, isrepeated);
    if (isrepeated)
	IIOSgqiGenQryId(osFrm);
}

/*{
** Name:	IIOSgqiGenQryId	- Generate an IL_QID statement.
**
** Description:
**	Generates an IL_QID statement.  An IL_QID statement is an IL
**	statement that contains a query id.  A query id is a 3 part identifier
**	used to identify repeat queries and cursors to libq.  The 3 parts
**	are a name, and 2 integers.   A query id must uniquely identify a
**	query to a server across all sessions of the server.  This uses
**	the exact same technique and routine that the embedded languages
**	do to generate a query id.
**
** Inputs:
**	name		The name of the object this query is contained in.
**			Normally this should be the frame or procedure
**			name.
**			Some part (or all) of this name will be used as
**			part of query id's name.
**
** Side Effects:
**	Generates IL using IGgenStmt.
**
** History:
**	12-feb-1990
**	    First Written
*/
VOID
IIOSgqiGenQryId(name)
char 	*name;
{
    EDB_QUERY_ID	qry_id;
    char		qid1[20];	/* Big enough for an i4 */
    char		qid2[20];	/* ditto */

    edb_unique(&qry_id, name);
    /*
    ** To get a constant into the system we have to have a string,
    ** Therefore we convert the integers to strings.
    */
    _VOID_ CVla(qry_id.edb_qry_id[0], qid1);
    _VOID_ CVla(qry_id.edb_qry_id[1], qid2);
    IGgenStmt(IL_QID, (IGSID *) NULL, 3,
		IGsetConst(DB_CHA_TYPE, qry_id.edb_qry_name),
		IGsetConst(DB_INT_TYPE, qid1), 
		IGsetConst(DB_INT_TYPE, qid2)
    );
}

/*{
** Name:	osdbveval() -	Evaluate INGRES Variable String Reference Node
**					for DBMS Statement.
** Description:
**	Evaluates a node that is optionally an INGRES variable string reference
**	by generating an IL statement to send the node value to the database as
**	a variable string.  (An INGRES variable string reference differs from
**	other INGRES references in that variables and identifiers are to be sent
**	to the DBMS as a quoted string.)
**
** Input:
**	node	{OSNODE *}  The INGRES variable string reference node.
**
** History:
**	09/90 (jhw) -- Written.
*/
VOID
osdbveval ( expr )
register OSNODE	*expr;
{
	switch (expr->n_token)
	{
	  case VALUE:
		/* send value */
		osdbflush();
		IGgenStmt(IL_DBVSTR, (IGSID *)NULL, 1, expr->n_ilref);
		break;
	  case DOT:
		if ( expr->n_right->n_token != tkID )
		{ /* a complex-object value */
			osdbflush();
			IGgenStmt(IL_DBVSTR, (IGSID *)NULL, 1, expr->n_ilref);
			break;
		}
		/* else fall through ... */
	  case ATTR:
	  case ASSOCOP:
	  case ARRAYREF:
	  case BLANK:
		osdbeval(expr);
		return;
	  case tkID:
		if ( expr->n_left == (OSNODE *)NULL )
		{
			osdbstr(osQuote);
			osdbstr(expr->n_value);
			osdbstr(osQuote);
		}
		else
		{
			osdbstr(expr->n_value);
			/*
			** Special case for column-list in the UPDATE clause of
			** a GRANT statement.  
			** XXX This could be generalized using a BLANK node.
			*/

			if (expr->n_left->n_token != PARENS)
				osdbstr(_Space);
			osdbeval(expr->n_left);
		}
		break;
	  case tkSCONST:
		osdbstr(osQuote);
		osdbstr(expr->n_const);
		osdbstr(osQuote);
		break;
	  case tkXCONST:
	  case tkICONST:
	  case tkFCONST:
	  case tkDCONST:
	  case tkNULL:
		osdbstr(expr->n_const);
		break;
	  default:
	  /* Note:  This should include comma lists
	  ** and special operators (such as ':') only.
	  */
		osqtrvfunc( &vref_node, expr );
		break;
	} /* end switch */

	ostrfree(expr);
}

/*{
** Name:	osdbwtle() -	Generate IL Code for DBMS With List Element.
**
** Description:
**	Generates IL code to output a "with" list element to the DBMS.  (Similar
**	to a "simple" target list element except the elements LOG, PASSWORD,
**	USERS, and GROUP have their values generated as variable string
**	references.)
**
**	Note that this target list element contains an equals (=) sign.
**
** History:
**	09/90 (jhw) -- Written.
**      20-apr-91 (rudyw)
**	    Change assignment statement for setting vstr. ODT compiler choked
**	    on assignment statement to boolean (said it was too complex).
**	    The complex statement was acceptable as part of a conditional.
**	19-nov-93 (robf)
**          Upated list of variable string references, should  include
**	    audit_log, expire_date, description and security_label.
**	    Took out group & users from the above list since they can't be 
**	    quoted for 6.5 (should be a delimeted identifier)
*/
VOID
osdbwtle ( ele, value, opt, comma )
OSNODE	*ele;
OSNODE	*value;
OSNODE	*opt;
bool	comma;
{
	bool	vstr;

	vstr = FALSE;
	if ( ele->n_token == tkID &&
			( osw_compare(ele->n_value, "log") ||
				osw_compare(ele->n_value, "password") ||
				osw_compare(ele->n_value, "old_password") ||
				osw_compare(ele->n_value, "oldpassword") ||
				osw_compare(ele->n_value, "expire_date") ||
				osw_compare(ele->n_value, "audit_log") ||
				osw_compare(ele->n_value, "description") 
			))
	{
		vstr = TRUE;
	}
	if ( ele->n_token == tkID &&
	     osw_compare(ele->n_value, "security_label"))
	{
	    /*
	    ** We quote the security label unless its the constant
	    ** INITIAL_LABEL
	    */
	    if(ele->n_token == tkID &&
	       osw_compare(value->n_value,"initial_label"))
		vstr=FALSE;
	    else
		vstr=TRUE;
	}
	osdbeval(ele);
	if ( value != NULL )
	{
		osdbstr(_Equal);	/* note:  possibly QUEL specific */
		if ( vstr )
		{ /* generate variable strings . . . */
			osdbveval(value);
		}
		else
		{
			osdbeval(value);
		}

		if ( opt != NULL )
		{
			osdbstr(_Space);
			osdbeval(opt);
		}
	}
	if ( comma )
		osdbstr(_Comma);
}

/*{
** Name:	osdbwith() -	Generate IL Code for DBMS WITH List.
**
** Description:
**	Generates IL code for an optional WITH list by calling 'osevaltlist()'
**	with the with list element generation routine, 'osdbwtle()'.
**
** Input:
**	tlist	{OSTLIST *}  The optional WITH list.
**
** History:
**	09/90 (jhw) -- Written.
*/
VOID
osdbwith ( tlist )
OSTLIST	*tlist;
{
	if ( tlist != NULL )
	{
		osdbstr(" with ");
		osevaltlist(tlist, osdbwtle);
	}
}

/*
** Sort routine for sorting PREDE nodes.  Sort so that tablefield qualifications
** come first, with columns from one table grouped together.
*/
static i4  
predcomp(pnode1, pnode2)
char *pnode1;
char *pnode2;
{
	OSSYM *sym1 = (*((OSNODE **)pnode1))->n_ele->n_pfld;
	OSSYM *sym2 = (*((OSNODE **)pnode2))->n_ele->n_pfld;
	bool is1col = sym1->s_kind == OSCOLUMN;
	bool is2col = sym2->s_kind == OSCOLUMN;

	/*
	** Columns precede simple fields.  Otherwise, sort by name.
	*/
	if (is1col && !is2col)
	{
		return -1; /* Columns go first */
	}
	else if (!is1col && is2col)
	{
		return 1;
	}
	else if (is1col && is2col)
	{
		i4 retval;
		
		/* Compare table name */
		retval = STcompare(sym1->s_parent->s_name, 
				   sym2->s_parent->s_name);
		if (retval != 0)
			return retval;	/* Columns in different tables */
	}
	return STcompare(sym1->s_name, sym2->s_name); 	/* Sort by name */
}


VOID
set_onclause_ref( node)
OSNODE	*node;
{
	 osqtrvfunc(&vref_node, node);
}


