/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include       <compat.h>
# include       <st.h>          /* 6-x_PC_80x86 */
# include	<cm.h>
# include	<cv.h>
# include	<er.h>
# include	<ex.h>
# include	<me.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include       <fe.h>
# include       <ug.h>
# include	<erug.h>
# include	<ugexpr.h>

/**
** Name:        ugexpr.c - routines to evaluate a boolean expression.
**
** Description:
**	Recursive descent parser for boolean expressions.
**	Adapted from code listings in book "Advanced C" by Schildt,
**	1986, McGraw Hill (book gives license to use its code listings).
**	Extended to support double byte, $variables, Boolean and character
**	expressions and to allow AND/OR connectors and comparison operators
**	(e.g., "((2-1>1*2) OR ('abc'+'def'='upd'+'ate'))" ).
**	($variable support subsequently removed).
**
** This file defines:
**
**      IIUGeceEvalCharExpr	- evaluate a character expression.
**	IIUGeeExprErr		- handle errors.
**	IIUGtcTypeCompat	- check for type compatibility.
**      IIUGrefRegisterExpFns	- register callback functions.
**      ...(plus static functions)...
**
** History:
**      24-feb-1992 (pete)
**	    Initial version.
**	22-jun-1992 (pete)
**	    Remove support for $variables: caller must substitute the statement
**	    before calling. Also, remove support for assignment expressions.
**	2-sep-94    (donc) Bug 63751
**	    Check to see if the token is longer than the length allowed. 
**	    Failure to do this check and signal an exception causes
**	    heinous Seg Vios, Access Violations, and nasty flame-outs.
**	2-sep-94    (donc) Bug 63876
**	    Check to see if a quoted character string token has a trailing
**	    quote.  If it doesn't, signal an error and get out. To do otherwise
**	    would cause the token buffer to be processed on and on off to
**	    oblivion.
**	12-sep-94    (donc) Bug 63941
**	    Check numeric strings for valid characters before copying them to
**	    the output string.
**	11-oct-94   (donc)
**	    Ack! replace openingres headers.
**      25-sep-96 (mcgem01)
**          Global data moved to ufdata.c
**      18-Feb-1999 (hweho01)
**          Changed EXCONTINUE to EXCONTINUES to avoid compile error of
**          redefinition on AIX 4.3, it is used in <sys/context.h>.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */

/* GLOBALREF's */
GLOBALREF	UG_EXPR_CALLBACKS	IIUGecbExprCallBack;

/* extern's */

FUNC_EXTERN	char	*IIUGeceEvalCharExpr();
FUNC_EXTERN	VOID	IIUGeeExprErr();
FUNC_EXTERN	VOID	IIUGrefRegisterExpFns();

/* static's */
static char *IIUGprog;	/* expression to be analyzed */

static UG_EXPR_TOKEN Tok;  /* details about a token returned from get_token() */
static UG_EXPR_TOKEN Tok_save;	/* Saved info about a token which result is
				** to be assigned to.
				*/
static char	Plus[]   = "+",
		Minus[]  = "-",
		Times[]  = "*",
		Divide[] = "/",
		Equals[] = "=",
		Lparen[] = "(",
		Rparen[] = ")",
		Expon[]  = "^",
		Quote[]  = "'",
		True_str[] = "TRUE",
		False_str[] = "FALSE",
		And_str[] = "AND",
		Or_str[] = "OR",
		Ne[] = "<>",
		Ne2[] = "!=",
		Not[] = "!",
		Lt[] = "<",
		Gt[] = ">",
		Le[] = "<=",
		Ge[] = ">=",
		Empty[] = "";

/* Valid comparison operators; 2 byte ones come first. */
static UG_EXPR_OPER_INFO comp_opers[] = {
        {Ne, 2, UG_CO_NE},
        {Ne2,2, UG_CO_NE},
        {Le, 2, UG_CO_LE},
        {Ge, 2, UG_CO_GE},
        {Lt, 1, UG_CO_LT},
        {Gt, 1, UG_CO_GT},
        {Equals, 1, UG_CO_EQ}
};
#define NMBR_COMPOPERS  sizeof(comp_opers)/sizeof(UG_EXPR_OPER_INFO)

/* Valid delimiters; 2 byte ones come first. */
static UG_EXPR_OPER_INFO delim_opers[] = {
        {Ne, 2, UG_CO_NONE},	/* NOTE: 3rd element, comp_oper, not set */
        {Ne2,2, UG_CO_NONE},
        {Le, 2, UG_CO_NONE},
        {Ge, 2, UG_CO_NONE},
        {Lt, 1, UG_CO_NONE},
        {Gt, 1, UG_CO_NONE},
        {Equals, 1, UG_CO_NONE},
	{Plus, 1, UG_CO_NONE},
	{Minus, 1, UG_CO_NONE},
	{Times, 1, UG_CO_NONE},
	{Divide, 1, UG_CO_NONE},
	{Lparen, 1, UG_CO_NONE},
	{Rparen, 1, UG_CO_NONE},
	{Expon, 1, UG_CO_NONE}
};
#define NMBR_DELIMOPERS  sizeof(delim_opers)/sizeof(UG_EXPR_OPER_INFO)

static char *delims[] = {Plus, Minus, Times, Divide,
			 Equals, Lparen, Rparen, Expon,
			 Not, Lt, Gt
			};
#define NMBR_DELIMS	(sizeof(delims)/sizeof(char *))

static VOID Level0_andor();
static VOID Level1_boolcomp();
static VOID Level2_addsub();
static VOID Level3_multdiv();
static VOID Level4_expon();
static VOID Level5_unary();
static VOID Level6_paren();
static VOID cvtTokToRes();
static VOID arith();
static VOID unary();
static VOID get_token();
static bool isdelim();
static bool is_in();
static i4	handler();      /* EXception handler */
static VOID	addstring();	/* concatenate strings */
static UG_EXPR_OPER_INFO *isoper();
static bool	evalcond();
static bool	evalNumericCond();
static bool	evalCharCond();
static VOID	eval_andor();
static char	*longtostring();
static char	*resToString();

/*{
** Name:        IIUGeceEvalCharExpr() - Parse and evaluate integer expression.
**
** Description:
**	Recursive descent parser. Parse and evaluate an integer, boolean or
**	character expression. Expression may contain references to
**	variables, also precedence and parenthesis are supported;
**	no functions allowed (does not use ADF). Whitespace is ignored.
**	Exponentiation, through the "^" operator is supported, but will
**	not be documented (Ingres should do that via "**").
**
**	    Grammar:
**		Expression ::= <condition> {AND | OR <condition>}
**		<condition>::= (<expr> [<comp_oper> <expr>])
**		<comp_oper>::= = | <> | != | < | > | <= | >=
**		<expr>     ::= Term [+ Term] [- Term]
**		Term	   ::= Factor [* Factor] [/ Factor]
**		Factor     ::= Constant | (<expr>)
**		Constant   ::= <integer_constant> | <character_constant> |
**				<boolean_constant>
** Inputs:
**	expr		- Pointer to expression to be parsed and evaluated.
**
** Outputs:
**      Returns:
**		(char *) Result of expression evaluation or NULL if
**		error occurred and couldn't be evaluated.
**
**      Exceptions:
**              EXdeclare(handler).
**
**              NOTE that IIUGrefRegisterExpFns() must be called first, before
**		calling this routine to register memory allocation function.
**
** Side Effects:
**
** History:
**	24-feb-1992 (pete)
**	    Initial version.
**	23-jun-1992 (pete)
**	    Change so only evaluates expressions (no assignments) with
**	    no variables present. Caller must perform substitution first.
**	4-nov-1992 (pete)
**	    Make it no longer an error if an empty-string token is detected
**	    after first call to get_token() in IIUGeceEvalCharExpr().
**	    Was preventing valid statements like:
**			#define  $var ''
**	    and		#if  '$var' = ''
*/
char *
IIUGeceEvalCharExpr(expr)
char *expr;
{
        EX_CONTEXT      ex;
	UG_EXPR_RESULT result;

        /* establish an exception handler. errors unwind to here. */
        if (EXdeclare(handler, &ex) != OK)
        {
            /* cleanup & return error */

            EXdelete();
            return (char *)NULL;
        }

	MEfill((u_i2)sizeof(UG_EXPR_TOKEN), (unsigned char)0, (PTR)&Tok);
	MEfill((u_i2)sizeof(UG_EXPR_TOKEN), (unsigned char)0, (PTR)&Tok_save);

	IIUGprog = expr;

	get_token();

	Level0_andor(&result);

        EXdelete();

	return resToString(&result);
}

/* ANDs and ORs at same level */
static VOID
Level0_andor(result)
UG_EXPR_RESULT *result;
{
	UG_EXPR_RESULT hold;
	i4 tok_type;

	Level1_boolcomp(result);
	while (Tok.tok_type == UG_AND_CONNECTOR ||
		Tok.tok_type == UG_OR_CONNECTOR)
	{
	    tok_type = Tok.tok_type;
	    get_token();
	    Level1_boolcomp(&hold);
	    eval_andor(tok_type, result, &hold);
	}
}

/* evaluate a boolean comparison. */
static VOID
Level1_boolcomp(result)
UG_EXPR_RESULT *result;
{
	UG_EXPR_RESULT hold;
	UG_EXPR_OPER_INFO *p_oper;

	Level2_addsub(result);
	if ((p_oper = isoper(Tok.token, comp_opers, NMBR_COMPOPERS)) !=
                                                (UG_EXPR_OPER_INFO *)NULL)
	{
	    /* Condition has comparison operator. Must be
	    ** Boolean condition like: "1+1>2-1", or "'abc'='a'+'bc'".
	    */
	    get_token(); /*get next part of exp (token following compoper)*/
	    Level2_addsub(&hold);

	    /* Compare lhs and rhs of condition */
	    result->res.ival = evalcond(p_oper->comp_oper, result, &hold);
	    result->res_type = UG_BOOLEAN_CONSTANT;
	}
}

static VOID
Level2_addsub(result)
UG_EXPR_RESULT *result;
{
	UG_EXPR_RESULT hold;
	char ttoken[UG_MAX_BYTES_PER_CHAR];

	hold.res_type = UG_NONE;

	Level3_multdiv(result);
	while (CMcmpcase(Tok.token, Plus) == 0 ||
		CMcmpcase(Tok.token, Minus) == 0)
	{
	    CMcpychar(Tok.token, ttoken);
	    get_token();
	    Level3_multdiv(&hold);

	    if ((result->res_type & UG_CHARACTER) &&
		(hold.res_type & UG_CHARACTER) &&
		(CMcmpcase(ttoken, Plus) == 0))
		addstring(result, &hold);
	    else
		arith(ttoken, result, &hold);
	}
}

static VOID
Level3_multdiv(result)
UG_EXPR_RESULT *result;
{
	UG_EXPR_RESULT hold;
	char ttoken[UG_MAX_BYTES_PER_CHAR];

	hold.res_type = UG_NONE;

	Level4_expon(result);
	while (CMcmpcase(Tok.token, Times) == 0 ||
		CMcmpcase(Tok.token, Divide) == 0)
	{
	    CMcpychar(Tok.token, ttoken);
	    get_token();
	    Level4_expon(&hold);
	    arith(ttoken, result, &hold);
	}
}

static VOID
Level4_expon(result)
UG_EXPR_RESULT *result;
{
	UG_EXPR_RESULT hold;

	hold.res_type = UG_NONE;

	Level5_unary(result);
	if (CMcmpcase(Tok.token, Expon) == 0)	 /*  *token == '^' */
	{
	    get_token();
	    Level4_expon(&hold);
	    arith(Expon, result, &hold);
	}
}

static VOID
Level5_unary(result)
UG_EXPR_RESULT *result;
{
	char ttoken[UG_MAX_BYTES_PER_CHAR];

	ttoken[0] = EOS;
	if ((Tok.tok_type == UG_DELIMITER) &&
	    (CMcmpcase(Tok.token, Plus) == 0) ||	/*  *token=='+' */
	    (CMcmpcase(Tok.token, Minus) == 0))		/*  *token=='-' */
	{
	    CMcpychar(Tok.token, ttoken);
	    get_token();
	}

	Level6_paren(result);

	if (ttoken[0] != EOS)
	    unary(ttoken, result);
}

static VOID
Level6_paren(result)
UG_EXPR_RESULT *result;
{
	if ((CMcmpcase(Tok.token, Lparen) == 0) &&	/*  *token=='(' */
	    (Tok.tok_type == UG_DELIMITER))
	{
	    get_token();
	    Level0_andor(result);
	    if (CMcmpcase(Tok.token, Rparen) != 0) /*  *token!=')' */
	    {
	    	IIUGeeExprErr((ER_MSGID)E_UG0041_Unbalanced_Parens,
			UG_EXPR_FAIL, 0, (PTR)NULL, (PTR)NULL, (PTR)NULL);
		/*NOTREACHED*/
	    }

	    get_token();
	}
	else
	{
	    cvtTokToRes(&Tok, result);
	    get_token();
	}
}

/* convert a UG_EXPR_RESULT to a character string */
static char *
resToString(res)
UG_EXPR_RESULT	*res;
{
	char *p_res;

	if ((res->res_type & UG_NUMERIC) || (res->res_type & UG_BOOLEAN))
	{
	    p_res = longtostring(res->res.ival);
	}
	else if (res->res_type & UG_CHARACTER)
	{
	    p_res = res->res.cval;
	}
	else
	{
	    p_res = (char *)NULL;
	}

	return p_res;
}

/* evaluate the result of two expressions connected with a boolean AND or OR.*/
static VOID
eval_andor(tok_type, lhs, rhs)
i4  tok_type;
UG_EXPR_RESULT *lhs;
UG_EXPR_RESULT *rhs;
{
	/* check for valid types that can do arithmetic on */
	if ((lhs->res_type & UG_CHARACTER) || (rhs->res_type & UG_CHARACTER))
	{
	    /* can't use character expression with AND/OR */
            IIUGeeExprErr((ER_MSGID)E_UG0045_Cant_AND_OR_Expr, UG_EXPR_FAIL, 0,
		(PTR)NULL, (PTR)NULL, (PTR)NULL);
            /*NOTREACHED*/
	}
	else if ((lhs->res_type == UG_AND_CONNECTOR) ||
		 (rhs->res_type == UG_OR_CONNECTOR))
	{
	    /* bad use of AND or OR */
            IIUGeeExprErr((ER_MSGID)E_UG0048_Bad_AND_OR, UG_EXPR_FAIL, 0,
		(PTR)NULL, (PTR)NULL, (PTR)NULL);
            /*NOTREACHED*/
	}

	if (tok_type == UG_AND_CONNECTOR)
	{
	    lhs->res.ival = (lhs->res.ival != 0 && rhs->res.ival != 0);
	}
	else if (tok_type == UG_OR_CONNECTOR)
	{
	    lhs->res.ival = (lhs->res.ival != 0 || rhs->res.ival != 0);
	}
}

/* convert a TOKEN to a UG_EXPR_RESULT */
static VOID
cvtTokToRes(tok, result)
UG_EXPR_TOKEN *tok;
UG_EXPR_RESULT *result;
{
	char *p_tmp;

	switch (tok->tok_type) {
	case UG_NUMERIC_CONSTANT:
	    result->res_type = tok->tok_type;
	    _VOID_ CVal(tok->token, &(result->res.ival));
	    break;
	case UG_BOOLEAN_CONSTANT:
	    result->res_type = tok->tok_type;
	    if (STbcompare(tok->token,0, True_str,0, TRUE) == 0)
		result->res.ival = TRUE;
	    else if (STbcompare(tok->token,0, False_str,0, TRUE) == 0)
		result->res.ival = FALSE;
	    break;
	case UG_CHARACTER_CONSTANT:
	    result->res_type = tok->tok_type;
	    if ((p_tmp=(char *)(*IIUGecbExprCallBack.getmem)
				(STlength(tok->token)+1, FALSE)) != NULL)
	    {
		STcopy(tok->token, p_tmp);
		result->res.cval = p_tmp;
	    }
	    break;
	case UG_AND_CONNECTOR:
	case UG_OR_CONNECTOR:
	    /* incorrect use of AND or OR. */
	    IIUGeeExprErr((ER_MSGID)E_UG0048_Bad_AND_OR, UG_EXPR_FAIL, 0,
		(PTR)NULL, (PTR)NULL, (PTR)NULL);
	    /*NOTREACHED*/
	default:
	    /* UG_DELIMITER or UG_NONE */
	    IIUGeeExprErr((ER_MSGID)E_UG0040_Syntax_Error, UG_EXPR_FAIL, 0,
		(PTR)NULL, (PTR)NULL, (PTR)NULL);    /* syntax error */
	    /*NOTREACHED*/
	}
}

/* Add two strings. write result to second argument (lhs).
** Allocate enough memory.
*/
static VOID
addstring(lhs, rhs)
UG_EXPR_RESULT *lhs;
UG_EXPR_RESULT *rhs;
{
	char *p_tmp;

	if ((p_tmp=(char *)(*IIUGecbExprCallBack.getmem)
	    (STlength(lhs->res.cval)+STlength(rhs->res.cval)+1, FALSE)) != NULL)
	{
	    STpolycat(2, lhs->res.cval, rhs->res.cval, p_tmp);
	    lhs->res.cval = p_tmp;
	}
}

/* call only numeric types (those with a UG_EXPR_RESULT.res.ival) */
static VOID
arith(tok, lhs, rhs)
char *tok;
UG_EXPR_RESULT *lhs;
UG_EXPR_RESULT *rhs;
{
	UG_EXPR_RESULT t, ex;

	/* check for valid types that can do arithmetic on */
	if ((lhs->res_type & UG_CHARACTER) || (rhs->res_type & UG_CHARACTER))
	{
	    /* can't do arithmetic on character types */
            IIUGeeExprErr((ER_MSGID)E_UG004B_Char_Arith, UG_EXPR_FAIL, 0,
		(PTR)NULL, (PTR)NULL, (PTR)NULL);
            /*NOTREACHED*/
	}
	else if ((lhs->res_type == UG_AND_CONNECTOR) ||
		 (rhs->res_type == UG_OR_CONNECTOR))
	{
	    /* bad use of AND or OR */
            IIUGeeExprErr((ER_MSGID)E_UG0048_Bad_AND_OR, UG_EXPR_FAIL, 0,
		(PTR)NULL, (PTR)NULL, (PTR)NULL);
            /*NOTREACHED*/
	}

	if (CMcmpcase(tok, Minus) == 0)
	    lhs->res.ival = lhs->res.ival - rhs->res.ival;
	else if (CMcmpcase(tok, Plus) == 0)
	    lhs->res.ival = lhs->res.ival + rhs->res.ival;
	else if (CMcmpcase(tok, Times) == 0)
	    lhs->res.ival = (lhs->res.ival) * (rhs->res.ival);
	else if (CMcmpcase(tok, Divide) == 0) {
	    if (rhs->res.ival == 0) {
		lhs->res.ival = 0;
		IIUGeeExprErr((ER_MSGID)E_UG0043_Divide_By_Zero, UG_EXPR_FAIL,
			0, (PTR)NULL, (PTR)NULL, (PTR)NULL);
		/*NOTREACHED*/
	    }
	    else
		lhs->res.ival = (lhs->res.ival)/(rhs->res.ival);
	}
	else if (CMcmpcase(tok, Expon) == 0) {
	    ex.res.ival = lhs->res.ival;
	    if (rhs->res.ival ==0) {
		lhs->res.ival = 1;
	    }
	    else
		for (t.res.ival = rhs->res.ival -1; t.res.ival > 0;
							--(t.res.ival))
		    lhs->res.ival = (lhs->res.ival) * (ex.res.ival);
	}
}

static VOID
unary(o,r)
char *o;
UG_EXPR_RESULT *r;
{
	/* check for valid type that can do Unary on */
	if (r->res_type & UG_CHARACTER)
	{
	    /* can't do unary of character type */
            IIUGeeExprErr((ER_MSGID)E_UG004B_Char_Arith, UG_EXPR_FAIL, 0,
		(PTR)NULL, (PTR)NULL, (PTR)NULL);
            /*NOTREACHED*/
	}
	else if ((r->res_type == UG_AND_CONNECTOR) ||
		 (r->res_type == UG_OR_CONNECTOR))
	{
	    /* bad use of AND or OR (e.g. -AND) */
            IIUGeeExprErr((ER_MSGID)E_UG0048_Bad_AND_OR, UG_EXPR_FAIL, 0,
		(PTR)NULL, (PTR)NULL, (PTR)NULL);
            /*NOTREACHED*/
	}

	if (CMcmpcase(o, Minus) == 0)
	    r->res.ival = -(r->res.ival);
}

static VOID
get_token()
{
	char *temp;
	i4  maxtoklen = UG_MAX_TOKSIZE;
	i4 i;
	UG_EXPR_OPER_INFO *p_oper;

	Tok.tok_type = UG_NONE;
	Tok.token[0] = EOS;
	temp=Tok.token;

	while (CMwhite(IIUGprog))	   /* skip white space */
	    CMnext(IIUGprog);

	if ((p_oper = isoper(IIUGprog, delim_opers, NMBR_DELIMOPERS)) != NULL)
	{
	    Tok.tok_type = UG_DELIMITER;
	    /* advance beyond operator (could be >1 character long) */
	    for (i=0; i < p_oper->oper_len; i++)
		CMcpyinc(IIUGprog, temp);
	}
	else if ( STlength(IIUGprog) > UG_MAX_TOKSIZE ) {
            IIUGeeExprErr((ER_MSGID)E_UG004F_TokenTooLong, UG_EXPR_FAIL, 1,
		 (PTR)&maxtoklen, (PTR)NULL, (PTR)NULL );
	    return;
	}
	else if (CMdigit(IIUGprog))
	{
	    Tok.tok_type = UG_NUMERIC_CONSTANT;
	    while (!isdelim(IIUGprog)) 
            {
		if ( CMdigit(IIUGprog) )
	            CMcpyinc(IIUGprog, temp);
		else
	            IIUGeeExprErr((ER_MSGID)E_UG0040_Syntax_Error, UG_EXPR_FAIL,
		    0,(PTR)NULL,(PTR)NULL,(PTR)NULL);    /* syntax error */
	    }
	}
	else if (CMcmpcase(IIUGprog, Quote) == 0)
	{
	    Tok.tok_type = UG_CHARACTER_CONSTANT;

	    /* quoted string; skip over quote. */
	    CMnext(IIUGprog);

	    /* Check to see if a trailing quote exists, if not
	       issue an error */
	    if ( STindex( IIUGprog, Quote, 0 ) == NULL )
                IIUGeeExprErr((ER_MSGID)E_UG0050_MissingQuote, UG_EXPR_FAIL, 1,
		   Quote, (PTR)NULL, (PTR)NULL );
	    else 
	    {
	        while (CMcmpcase(IIUGprog, Quote) != 0)
		    CMcpyinc(IIUGprog, temp);

	        CMnext(IIUGprog); /* skip over trailing quote in input buf */
	        *temp=EOS;		/* terminate token */
	    }

	}
	else
	{
	    /* collect next token; it's a Boolean_constant or AND/OR */
	    while (!isdelim(IIUGprog))
	        CMcpyinc(IIUGprog, temp);

	    *temp=EOS;

	    if ((*IIUGprog == EOS) && (Tok.token[0] == EOS))
		/* we're done; no more tokens. */
		Tok.tok_type = UG_NONE;
	    else if ((STbcompare(Tok.token,0, True_str,0, TRUE) == 0) ||
		     (STbcompare(Tok.token,0, False_str,0, TRUE) == 0))
	    {
		/* it's a boolean constant: TRUE or FALSE */
		Tok.tok_type = UG_BOOLEAN_CONSTANT;
	    }
	    else if (STbcompare(Tok.token,0, And_str,0, TRUE) == 0)
	    {
		Tok.tok_type = UG_AND_CONNECTOR;	/* boolean connector */
	    }
	    else if (STbcompare(Tok.token,0, Or_str,0, TRUE) == 0)
	    {
		Tok.tok_type = UG_OR_CONNECTOR;		/* boolean connector */
	    }
	    else
	    {
		/* invalid token */
		IIUGeeExprErr((ER_MSGID)E_UG0044_Invalid_Token, UG_EXPR_FAIL, 1,
			(PTR)&(Tok.token[0]), (PTR)NULL, (PTR)NULL);
		/*NOTREACHED*/
	    }
	}

	*temp=EOS;	/* NULL terminate Tok.token */
}

static bool
isdelim(c)
char *c;
{
	if (is_in(c, delims, NMBR_DELIMS) || CMwhite(c) || *c == EOS)
	    return TRUE;

	return FALSE;
}

static bool
is_in(ch, s, cnt)
char *ch;
char *s[];
i4   cnt;
{
	i4 i;

	for (i=0; i < cnt; i++)
	    if (CMcmpcase(ch, s[i]) == 0)
		return TRUE;
	return FALSE;
}

/* See if character is in array of UG_EXPR_OPER_INFO */
static UG_EXPR_OPER_INFO
*isoper(buf, opers, numopers)
char    *buf;		/* does not have to be null terminated */
UG_EXPR_OPER_INFO *opers;
i4	numopers;
{
        i4  i;

        for (i=0; i < numopers; i++, opers++)
            if (STscompare(buf, opers->oper_len,
			   opers->str_oper, opers->oper_len) == 0)
                return opers;

        return (UG_EXPR_OPER_INFO *)NULL;
}

/*{
** Name:	IIUGeeExprErr - process expression evaluation error.
**
** Description:
**	Print message and then issue exception which will either cause
**	expression evaluation to continue where it left off (before the
**	error), or to return FAIL from the top-level routine:
**	IIUGeceEvalCharExpr().
**
** Inputs:
**	ER_MSGID        msgid		Message id for the error to print.
**	i4             severity	[ UG_EXPR_FAIL | UG_EXPR_CONTINUE ]
**	i4             parcount	Number of parameters that follow (max=3)
**	PTR             a1 - a3
**
** Outputs:
**	NONE
**
**	Returns:
**		VOID
**
**	Exceptions:
**		EXVFLJMP or EXVALJMP
**
** Side Effects:
**	NONE
**
** History:
**	10-mar-1992 (pete)
**	    Initial Version
*/
VOID
IIUGeeExprErr(msgid, severity, parcount, a1, a2, a3)
ER_MSGID        msgid;
i4              severity;
i4              parcount;
PTR             a1, a2, a3;	/* maximum of 3 args */
{
	IIUGerr(msgid, UG_ERR_ERROR,
		  parcount, a1, a2, a3);

	/*
	** Now, either continue processing where we were before error occurred,
	** or run the EXdeclare block in IIUGeceEvalCharExpr, based on the value
	** of 'severity'.
	*/
	if (severity & UG_EXPR_FAIL)
	    EXsignal (EXVFLJMP,  0);  /* run EXdeclare block & return FAIL */
	else
	    EXsignal (EXVALJMP,  0); /* warning message. continue processing*/
}

/*{
** Name:        handler - local exception handler.
**
** Description:
**      This routine is called when an exception occurs (Note that all errors
**      that occur during expression evaluation raise an exception, but other
**	things can raise an exception too).
**
** Inputs:
**      EX_ARGS *ex_args
**
** Outputs:
**      NONE
**
**      Returns:
**              EXDECLARE or EXCONTINUES.
**
**      Exceptions:
**              NONE.
**
** Side Effects:
**
** History:
**	10-mar-1992 (pete)
**	    Initial version.
*/
static i4
handler(ex_args)
EX_ARGS *ex_args;
{
        if (ex_args->exarg_num == EXVALJMP)
        {
            return ( EXCONTINUES );
        }
        else if ((ex_args->exarg_num == EXSEGVIO)
             || (ex_args->exarg_num  == EXBUSERR))
        {
            IIUGerr(E_UG0046_AV_SegViol, UG_ERR_ERROR, 0);
        }

        return ((i4) EXDECLARE);   /* any other exception: run
                                        ** declare section
                                        */
}

/*{
** Name:	IIUGtcTypeCompat - check for type compatibility.
**
** Description:
**	Compare two types, given as arguments, for compatibility.
**	Character is incompatible with Num or Bool.
**	Nothing is compatible with UG_NONE or UG_DELIMITER.
**
** Inputs:
**	ltype	- token or result type.
**	rtype	- token or result type.
**
** Outputs:
**
**	Returns:
**		TRUE for "compatible", FALSE otherwise.
**
**	Exceptions:
**		NONE
**
** History:
**	11-mar-1992 (pete)
**	    Initial version.
*/
bool
IIUGtcTypeCompat(ltype, rtype)
i4  ltype;
i4  rtype;
{
	if (
	  (((ltype & UG_BOOLEAN) || (ltype & UG_NUMERIC)) &&
	    (rtype & UG_CHARACTER))
	   ||
	  ((ltype & UG_CHARACTER) &&
	  ((rtype & UG_BOOLEAN) || (rtype & UG_NUMERIC)))
	   ||
	  ((ltype == UG_DELIMITER) || (rtype == UG_DELIMITER))
	   ||
	  ((ltype == UG_AND_CONNECTOR) || (rtype == UG_AND_CONNECTOR) ||
	   (ltype == UG_OR_CONNECTOR) || (rtype == UG_OR_CONNECTOR))
	   ||
	  ((ltype == UG_NONE) || (rtype == UG_NONE))
	   )
	    return FALSE;	/* incompatible types */
	else
	    return TRUE;	/* compatible */
}

/*{
** Name:        evalcond - evaluate a boolean condition.
**
** Description:
**      evaluate a boolean condition. Return TRUE or FALSE.
**
** Inputs:
**	oper	- symbolic for the operator (>, <, etc)
**	lhs	- result for left hand side of conditional
**	rhs	- result for right hand side of conditional
**
** Outputs:
**
**      Returns:
**              Whatever condition evaluates to: TRUE or FALSE.
**
**      Exceptions:
**
** History:
**      12-mar-1992 (pete)
**          Initial version.
*/
static bool
evalcond(oper, lhs, rhs)
i4  oper;
UG_EXPR_RESULT *lhs;
UG_EXPR_RESULT *rhs;
{
	bool retval;

	/* validate that types can be compared. */
/* BUG: what if RHS is empty? will this return error ? */
	if (!IIUGtcTypeCompat(lhs->res_type, rhs->res_type))
	{
	    IIUGeeExprErr((ER_MSGID)E_UG0047_Mixed_Type_Compare,
		UG_EXPR_FAIL, 0, (PTR)NULL, (PTR)NULL, (PTR)NULL);
	    /*NOTREACHED*/
	}

        if  ((lhs->res_type & UG_BOOLEAN) ||
             (lhs->res_type & UG_NUMERIC))
        {
            /*printf("numeric or boolean expression; "); */
            retval = evalNumericCond(oper, lhs, rhs);
        }
        else if (lhs->res_type & UG_CHARACTER)
        {
            /*printf("character expression; "); */
            retval = evalCharCond(oper, lhs, rhs);
        }
        else
            retval = FALSE ;

	return retval;
}

/*{
** Name:        evalNumericCond - Evaluate a numeric or boolean condition.
**
** Description:
**
** Inputs:
**	oper	- symbolic for the operator (>, <, etc)
**	lhs	- result for left hand side of conditional
**	rhs	- result for right hand side of conditional
**
** Outputs:
**      Returns:
**              Whatever condition evaluates to: TRUE or FALSE.
**
**      Exceptions:
**
** History:
**      12-mar-1992 (pete)
**          Initial version.
*/
static bool
evalNumericCond(oper, lhs, rhs)
i4  oper;
UG_EXPR_RESULT *lhs;
UG_EXPR_RESULT *rhs;
{
	bool eval;

        /* compare lhs with rhs */
        switch(oper)
        {
            case (UG_CO_NONE):
                eval = (lhs->res.ival != 0) ? TRUE : FALSE ;
                break;
            case (UG_CO_EQ):
                eval = (lhs->res.ival == rhs->res.ival);
                break;
            case (UG_CO_NE):
                eval = (lhs->res.ival != rhs->res.ival);
                break;
            case (UG_CO_GT):
                eval = (lhs->res.ival > rhs->res.ival);
                break;
            case (UG_CO_GE):
                eval = (lhs->res.ival >= rhs->res.ival);
                break;
            case (UG_CO_LT):
                eval = (lhs->res.ival < rhs->res.ival);
                break;
            case (UG_CO_LE):
                eval = (lhs->res.ival <= rhs->res.ival);
                break;
            default:
		eval = FALSE;
                break;
                /* bad comparison operator
                ** IIUGeeExprErr((ER_MSGID)E_UG0049_Bad_Bool_Oper,
		**	UG_EXPR_FAIL, 0, (PTR)NULL, (PTR)NULL, (PTR)NULL);
		*/
        }

        return eval;
}

/*{
** Name:        evalCharCond - Evaluate a character condition.
**
** Description:
**
** Inputs:
**	oper	- symbolic for the operator (>, <, etc)
**	lhs	- result for left hand side of conditional
**	rhs	- result for right hand side of conditional
**
** Outputs:
**      Returns:
**              Whatever condition evaluates to: TRUE or FALSE.
**
**      Exceptions:
**
** History:
**      12-mar-1992 (pete)
**          Initial version.
*/
static bool
evalCharCond(oper, lhs, rhs)
i4  oper;
UG_EXPR_RESULT *lhs;
UG_EXPR_RESULT *rhs;
{
	bool eval;
	i4 diff;

        /* Something may not have been initialized above; would cause STcompare
        ** to crash.
        */
        if (lhs->res.cval == (char *)NULL)
            lhs->res.cval = Empty;

        if (rhs->res.cval == (char *)NULL)
            rhs->res.cval = Empty;

        diff = STcompare(lhs->res.cval, rhs->res.cval);

        /* compare lhs with rhs */
	switch(oper)
	{
            case (UG_CO_EQ):
                eval = (diff == 0);
                break;
            case (UG_CO_NE):
                eval = (diff != 0);
                break;
            case (UG_CO_GT):
                eval = (diff > 0);
                break;
            case (UG_CO_GE):
                eval = (diff >= 0);
                break;
            case (UG_CO_LT):
                eval = (diff < 0);
                break;
            case (UG_CO_LE):
                eval = (diff <= 0);
                break;
            case (UG_CO_NONE):
            default:
                /* missing or bad comparison operator */
                IIUGeeExprErr((ER_MSGID)E_UG004A_Bad_Condition, UG_EXPR_FAIL, 0,
			(PTR)NULL, (PTR)NULL, (PTR)NULL);
                /*NOTREACHED*/
                eval = FALSE;
        }

        return eval;
}

/*{
** Name:	longtostring - convert i4 to string.
**
** Description: make string version of a i4 value. Calls CVla() to do
**	the conversion.
**
** Inputs:
**	l		- i4 value to convert to string.
**
** Outputs:
**
**	Returns:
**		(char *) equivalent of i4 value, or NULL if couldn't
**		allocate memory for string.
**
** Side Effects:
**	allocates memory for string.
**
** History:
**	23-jun-1992 (pete)
**	    Initial version.
*/
static char *
longtostring(l)
i4	l;
{
        char tmp[FE_MAXNAME+1];
        char *p_tmp;

        /* make string version of token's integer value (must have both) */
        CVla(l, tmp);
        if ((p_tmp =
	        (char *)(*IIUGecbExprCallBack.getmem)((u_i4)(STlength(tmp) +1),
                FALSE)) != NULL)
            STcopy(tmp, p_tmp);

        return p_tmp;
}

/*{
** Name:        IIUGrefRegisterExpFns() - Register callback functions.
**
** Description:
**
** Inputs:
**      (*fp4)()         - Function to allocate memory. Passed two args:
**                         1 - Number of bytes to allocate.
**                         2 - TRUE if want allocated memory to be initialized
**                              with zeros, FALSE otherwise.
**                         returns PTR.
** Outputs:
**      NONE
**
** History:
**      6-mar-1992 (pete)
**          Initial version.
**      23-jun-1992 (pete)
**	    Removed (*fp1)(), fp2, fp3. No longer needed.
*/
VOID
IIUGrefRegisterExpFns(fp4)
PTR (*fp4)();           /* get memory */
{
        IIUGecbExprCallBack.getmem = fp4;
}

