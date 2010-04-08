# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<cm.h>
# include	<er.h>
# include	"monitor.h"
# include	"ermo.h"

/*
** Copyright (c) 2004 Ingres Corporation
**
**  EXPR -- evaluate expression
**
**	This module evaluates an expression in somewhat standard
**	infix notation.	 Several restrictions apply.  There are
**	no variables, since this can be simulated with the macro
**	processor.  No numeric overflow is checked.  There may be
**	no spaces, tabs, or newlines in the expression.
**
**	The text of the expression is read from 'macgetch', so
**	that must be initialized before calling this routine.
**
**	Operators accepted are + - * / < > >= <= = != % ( )
**	& |.
**	Operands may be signed integers.
**	Standard precedence applies.
**
**	An expression can be viewed as a sequence of operands,
**	and operators.	If the terminator is considered to be
**	an operator, then the sequence must be composed
**	of n matched pairs of operators and operands.  NOT and
**	Negation are considered to be part of the operand and
**	are treated as such.  Thus to evaluate an expression,
**	n pairs are read until the terminator is found as the
**	last operator.
**
**	Parameters:
**		none
**
**	Returns:
**		value of the expression.  Undetermined value
**		on error.
**
**	Side Effects:
**		Macro processing can occur.
**
**	Defines:
**		expr -- the main entry point.
**		valof -- the guts of the expression processor.
**		exprfind -- finds and chews on operands.
**		opfind -- finds and chews on operations.
**		exp_op -- performs operations.
**		numberget -- converts numbers.
**		exprgch -- to get characters.
**		pushop, popop, pushnum, popnum -- stack operations.
**		ExprNstack, ExprNptr, ExprOstack, ExprOptr --
**			stacks + associated data.
**		ExprError -- an error flag.
**		ExprPrec -- the precedence tables.
**		ExprPeek -- the lookbehind character for exprgch.
**
**	Requires:
**		macgetch -- to get the next character.
**
**	Required By:
**		branch.c
**
**	Files:
**		none
**
**	Trace Flags:
**		none
**
**	History:
**		2/27/79 -- written by John Woodfill, with some
**			cleanup by Eric.
**		85.11.21 (bml) -- bug fix #7002 -- changed order of included headers; ch.h must follow compat.h since
**				it depends on the definition of extern as globalref.
**				Also changed some storage allocations to globaldef, as necessary.
**              15-jan-1996 (toumi01; from 1.1 axp_osf port)
**                   Added kchin's changes (from 6.4) for axp_osf
**                   10/15/93 (kchin)
**                   Cast 2nd argument to PTR when calling putprintf(), this
**                   is due to the change of datatype of variable parameters
**                   in putprintf() routine itself.  The change is made in
**                   exprfind() and opfind().
**	28-Aug-2009 (kschendel) 121804
**	    Need monitor.h to satisfy gcc 4.3.
*/



# define	EXPSTACKSIZE	   50
# define	RIGHTP		21
# define	END		22
# define	SEPERATOR	0
# define	OR		1
# define	AND		2
# define	EQUALS		3
# define	NEQ		4
# define	LESS		5
# define	LEQ		6
# define	GREATER		7
# define	GEQ		8
# define	ADD		9
# define	SUBTRACT	10
# define	MULTIPLY	11
# define	DIVIDE		12
# define	MOD		13


GLOBALDEF	i4	ExprPrec[] =			/* Precedence table */
{
	0,	/* filler */
	1,	/* 1 -- OR */
	2,	/* 2 -- AND */
	3,	/* 3 -- EQUALS */
	3,	/* 4 -- NEQ */
	4,	/* 5 -- LESS */
	4,	/* 6 -- LEQ */
	4,	/* 7 -- GREATER */
	4,	/* 8 -- GEQ */
	5,	/* 9 -- ADD */
	5,	/* 10 -- SUBTRACT */
	6,	/* 11 -- MULTIPLY */
	6,	/* 12 -- DIVIDE */
	6	/* 13 -- MOD */
};


GLOBALDEF	i4	ExprNstack[EXPSTACKSIZE]	ZERO_FILL;
GLOBALDEF	i4	*ExprNptr			= 0;
GLOBALDEF	i4	ExprOstack[EXPSTACKSIZE]	ZERO_FILL;
GLOBALDEF	i4	*ExprOptr			= 0;
GLOBALDEF	i4	ExprError			= 0;
GLOBALDEF	i2	ExprPeek			= ' ';

i4	pushop(),popop(),pushnum(),popnum(),exprfind();
i4	opfind(),exp_op(),numberget(),exprgch();


i4
expr()
{
	i4	valof();
	ExprNptr	= ExprNstack;
	ExprOptr	= ExprOstack;
	ExprError	= FALSE;
	ExprPeek	= -1;

	return(valof((i4)END));
}
 /*
**  VALOF -- compute value of expression
**
**	This is the real expression processor.	It handles sequencing
**	and precedence.
**
**	Parameters:
**		terminator -- the symbol which should terminate
**			the expression.
**
**	Returns:
**		The value of the expression.
**
**	Side Effects:
**		Gobbles input.
**
**	Requires:
**		exprfind -- to read operands.
**		opfind -- to read operators.
**		exp_op -- to perform operations.
**
**	Called By:
**		expr
**
**	Diagnostics:
**		Extra Parenthesis found: assumed typo
**			An unmatched right parenthesis was read.
**			It was thrown away.
**		Insufficient parenthesis found: assumed zero.
**			An unmatched left parenthesis was left
**			in the operator stack at the end of the
**			expression.  The value zero was taken
**			for the expression.
**
**	Syserrs:
**		none
*/

i4
valof(termoper)
i4	termoper;
{
	register i4	number;
	register i4	operator;

	pushop((i4)SEPERATOR);		/* initialize the stack */

	for(;;)
	{
		number = exprfind();
		if (ExprError)
			return(0);
		operator = opfind();
		if (ExprError)
			return(0);

		if (operator == RIGHTP || operator == END)
			break;

		/* Do all previous operations with a higher precedence */
		while (ExprPrec[operator] <= ExprPrec[ExprOptr[-1]])
			number = exp_op(popop(), popnum(), number);
		pushop(operator);
		pushnum(number);
	}
	if (operator != termoper)		/* ExprError in operators */
		if (operator == RIGHTP)
			putprintf(ERget(F_MO0010_Extra_paren_found));
		else
		{
			ExprError = TRUE;
			putprintf(ERget(F_MO0011_Insufficient_parens));
			return(0);
		}
	/* Empty stack for this call of valof */
	while ((operator = popop()) != SEPERATOR)
		number = exp_op(operator, popnum(), number);

	return(number);
}
 /*
**  EXPRFIND -- find and chomp operand
**
**	This routine reads the next operand.  It generally just
**	reads numbers, except it also knows about unary operators
**	! and - (where it calls itself recursively), and paren-
**	theses (where it calls valof recursively).
**
**	Parameters:
**		none
**
**	Returns:
**		value of operand.
**
**	Side Effects:
**		Gobbles input.
**
**	Requires:
**		numberget -- to read numbers.
**		exprgch.
**
**	Called By:
**		valof
**		exprfind (recursively)
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		Expression expected: end of expression found.
**			Nothing was found.  Zero is returned.
**		Expression expected: %c found; assumed zero.
**			A syntax error -- nothing was found
**			which was acceptable.
*/



i4
exprfind()
{
	register i4	result;
	register i4	c;
	i4		retval;

	c = exprgch();

	switch(c)
	{

	  case '0':
	  case '1':
	  case '2':
	  case '3':
	  case '4':
	  case '5':
	  case '6':
	  case '7':
	  case '8':
	  case '9':
		retval = numberget((char) c);
		break;

	  case '!':
		result = exprfind();
		retval = (ExprError ? 0 : (result <= 0));
		break;

	  case '-':
		result = exprfind();
		retval = (ExprError ? 0 : -result);
		break;

	  case '(':
		retval = valof((i4)RIGHTP);
		break;

	  case ' ':
	  case '\n':
	  case '\t':
	  case '\0':
		putprintf(ERget(F_MO0012_Expression_expected));
		ExprError = TRUE;
		retval = 0;
		break;

	  default:
		putprintf(ERget(F_MO0013_Expression_expected__), (PTR)c);
		ExprError = TRUE;
		retval = 0;
		break;
	}

	return (retval);
}

/*
**  OPFIND -- find and translate operator
**
**	This reads the next operator from the input stream and
**	returns the internal code for it.
**
**	Parameters:
**		none
**
**	Returns:
**		The code for the next operator.
**		Zero on error.
**
**	Side Effects:
**		Gobbles input.
**
**	Requires:
**		exprgch.
**
**	Called By:
**		valof
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		Operator expected: '%c' found.
**			Gibberish in input.
*/

i4
opfind()
{
	register i4	c;
	i4		retval;

	c = exprgch();

	switch(c)
	{

	  case '/':
		retval = DIVIDE;
		break;

	  case '=':
		retval = EQUALS;
		break;

	  case	'&':
		retval = AND;
		break;

	  case '|':
		retval = OR;
		break;

	  case '+':
		retval = ADD;
		break;

	  case '-':
		retval = SUBTRACT;
		break;

	  case '*':
		retval = MULTIPLY;
		break;

	  case '<':
		c = exprgch();
		if (c == '=')
		{
			retval = LEQ;
		}
		else
		{
			ExprPeek = c;
			retval = LESS;
		}
		break;

	  case '>':
		c = exprgch();
		if (c == '=')
		{
			retval = GEQ;
		}
		else
		{
			ExprPeek = c;
			retval = GREATER;
		}
		break;

	  case '%':
		retval = MOD;
		break;

	  case '!':
		c = exprgch();
		if (c == '=')
		{
			retval = NEQ;
		}
		else
		{
			putprintf(ERget(F_MO0014_Operator_expected), (PTR)c);
			ExprError = TRUE;
			retval = 0;
		}
		break;

	  case ')':
		retval = RIGHTP;
		break;

	  case ' ':
	  case '\t':
	  case '\n':
	  case '\0':
		retval = END;
		break;

	  default:
		putprintf(ERget(F_MO0015_Oper_expected_found_c), (PTR)c);
		ExprError = TRUE;
		retval = 0;
		break;

	}

	return (retval);
}

/*
**  EXP_OP -- perform operation
**
**	Performs an operation between two values.
**
**	Parameters:
**		op -- the operation to perform.
**		lv -- the left operand.
**		rv -- the right operand.
**
**	Returns:
**		The value of the operation.
**
**	Side Effects:
**		none
**
**	Requires:
**		none
**
**	Called By:
**		valof.
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		none
*/

i4
exp_op(op, lv, rv)
i4	op;
i4	lv;
i4	rv;
{
	register i4	retval;

	switch(op)
	{

	  case OR:
		retval = ((lv > 0) || (rv > 0));
		break;

	  case AND:
		retval = ((lv > 0) && (rv > 0));
		break;

	  case EQUALS:
		retval = (lv == rv);
		break;

	  case NEQ:
		retval = (lv != rv);
		break;

	  case LESS:
		retval = (lv < rv);
		break;

	  case LEQ:
		retval = (lv <= rv);
		break;

	  case GREATER:
		retval = (lv > rv);
		break;

	  case GEQ:
		retval = (lv >= rv);
		break;

	  case ADD:
		retval = (lv + rv);
		break;

	  case SUBTRACT:
		retval = (lv - rv);
		break;

	  case MULTIPLY:
		retval = (lv * rv);
		break;

	  case DIVIDE:
		if (rv == 0)
		{
			putprintf(ERget(F_MO0016_Divide_by_zero));
			retval = 0;
		}
		else
		{
			retval = (lv / rv);
		}
		break;

	  case MOD:
		retval = (lv % rv);
		break;

	  default:
		/* exp_op: bad op %d */
		ipanic(E_MO004D_1500600, op);

	}

	return (retval);
}
 /*
**  NUMBERGET -- read and convert a number
**
**	Reads and converts a signed integer.
**
**	Parameters:
**		none
**
**	Returns:
**		The next number in the input stream.
**
**	Side Effects:
**		Gobbles input.
**
**	Requires:
**		exprgch.
**
**	Called By:
**		exprfind.
*/

i4
numberget(cx)
char	cx;
{
	register i4	result;
	char		c;


	c	= cx;

	result	= 0;

	do
	{
		result	= result * 10 + c - '0';
		c	= exprgch();
	} while (CMdigit(&c));

	ExprPeek = c;

	return(result);
}
 /*
**  EXPRGCH -- expression character get
**
**	Gets the next character from the expression input.  Takes
**	a character out of ExprPeek first.  Also maps spaces, tabs,
**	and newlines into zero bytes.
**
**	Parameters:
**		none
**
**	Returns:
**		Next character.
**
**	Side Effects:
**		Gobbles input.
**		Clears ExprPeek if set.
**
**	Requires:
**		ExprPeek -- the peek character.
**		macgetch -- to get the next character if ExprPeek
**			is not set.
*/

i4
exprgch()
{
	register i4	c;


	if (ExprPeek == -1)
		c = macgetch();
	else
		c = ExprPeek;

	ExprPeek = -1;

	if (c == ' ' || c == '\n' || c == '\t')
		c = 0;

	return (c);
}
 /*
**  Stack operations.
*/


/* Popop returns the top of the operator stack and decrements this stack. */
i4
popop()
{
	if (ExprOptr <= ExprOstack)
		/* popop: underflow */
		ipanic(E_MO004E_1500601);
	/*! BEGIN COMPILER */
	/* return(*--ExprOptr); */
	ExprOptr--;
	return(*ExprOptr);
	/*! END COMPILER */
}



/* Pushop increments the stack pointer and pushes op on the stack. */
i4
pushop(op)
i4	op;
{
	*ExprOptr++ = op;
}



/* Popnum returns the top of the number stack and decrements the stack pointer. */
i4
popnum()
{
	if (ExprNptr <= ExprNstack)
		/* popnum: underflow */
		ipanic(E_MO004F_1500602);
	/*! BEGIN COMPILER */
	/* return(*--ExprNptr); */
	ExprNptr--;
	return(*ExprNptr);
	/*! END COMPILER */
}




/* Pushnum increments the stack pointer and pushes num onto the stack */
i4
pushnum(num)
i4	num;
{
	*ExprNptr++ = num;
}
