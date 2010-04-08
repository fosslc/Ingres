/*
**	Copyright (c) 1992, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<ex.h>
# include	<cm.h>
# include	<me.h>
# include	<si.h>
# include	<pc.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<ooclass.h>
# include	<abclass.h>
# include	<metafrm.h>
# include	<erfg.h>
# include	<erug.h>
# include	<ugexpr.h>

# include	"framegen.h"

/**
** Name:	fgvars.c - routines that work with $variables.
**
** Description:
**
** This file defines:
**
**	IIFGftFindToken	- find a $variable.
**	IIFGcvCreateVariable - create new $variable. ##DEFINE stmt.
**	IIFGdvDefineVariable - define $variable. ##DEFINE stmt.
**	IIFGrdvReDefineVariable - redefine existing $variable. ##DEFINE stmt.
**	IIFGuvUndefVariable - delete a $variable. ##UNDEF stmt.
**
** History:
**	18-mar-1992 (pete) (20th wedding anniversary)
**	    Initial version. Moved IIFGftFindToken here from framegen.c
**      24-sep-96 (hanch04)
**          Global data moved to data.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

GLOBALREF DECLARED_IITOKS *IIFGdcl_iitoks ;	/* DEFINEd $vars */

FUNC_EXTERN     PTR     IIFGgm_getmem();
FUNC_EXTERN     bool    IIUGtcTypeCompat();
FUNC_EXTERN	VOID	IIUGeeExprErr();
FUNC_EXTERN	char	*IIUGeceEvalCharExpr();
FUNC_EXTERN     IITOKINFO	*IIFGftFindToken();
FUNC_EXTERN     bool	IIFGcvCreateVariable();
FUNC_EXTERN     bool	IIFGrdvReDefineVariable();
FUNC_EXTERN bool        IIFGbseBoolStmtEval();

/* extern's */
GLOBALREF IITOKINFO Iitokarray[]; 	/* defined in fginit.c */
GLOBALREF i4  Nmbriitoks;

/* statics */
static IITOKINFO *find_declared_iitok();
static DECLARED_IITOKS *find_last_iitok();

static char Empty[] = "";

/*{
** Name:	IIFGftFindToken()	- find definition of a $variable.
**
** Description:
**	Find definition of a $variable by name in the global Iitokarray.
**	Performs a binary search through Iitokarray, which is in alphabetical
**	order by token name.
**
** Inputs:
**	string pointer to token whose definition needs to be found.
**
** Outputs:
**
**	Returns:
**		pointer to either translation string if one was found,
**		or NULL ptr if $token not defined.
**
**	Exceptions:
**		NONE
**
** Possible Performance Improvements:
**	Make token list a hash list, rather than sorted array.
**
** Side Effects:
**
** History:
**	4/1/89 (pete)	written
**	28-feb-1992 (pete)
**	    Change return type from (char *) to (IITOKINFO *). Change routine
**	    name from findiitok to IIFGftFindToken and make globally visible.
**	    Move to new fgvars.c file (used to live in framegen.c).
*/
IITOKINFO *
IIFGftFindToken(token)
char *token;
{
    i4  low, high, mid, stat;
    bool foundit = FALSE;

    low  = 0;
    high = Nmbriitoks -1;

    while (low <= high)
    {
	mid = (low + high)/2;	
		/* STcompare below ignores case */
	stat = STbcompare(token,0, Iitokarray[mid].tokname,0, TRUE);
	if (stat<0)	 high = mid  -1;
	else if (stat>0) low = mid +1;
	else 
	{
	    foundit = TRUE;	/* got hit at position '[mid]' */
	    break;
	}
    }

    if (foundit)
    {
    	return &Iitokarray[mid];	/* we got a hit on an active token */

	/* Otherwise, we got a hit on a token that user UNDEF'd. So, fall thru
	** and continue searching to see if got reDEFINEd again later...
	*/
    }

    /* search user DEFINEd $variables */
    return find_declared_iitok(token);
}

/*{
** Name:	IIFGdvDefineVariable - define/redefine $var.
**
** Description:
**	Create new or change definition of existing $variable.
**
** Inputs:
**	varname		- name of $variable to create. assumed valid.
**	expr		- value expression to give variable.
**			  (*expr==EOS) is valid and means no expression
**			  was given for the $var in the ##DEFINE stmt.
**      infn            - name of input file currently processing.
**      line_cnt        - current line number in input file.
**
** Outputs:
**
**	Returns:
**		TRUE if all went ok. FALSE otherwise.
**
** History:
**	22-jun-1992 (pete)
**	    Initial version. Done as part of Windows4gl Amethyst project
**	    for 6.5 release.
*/
bool
IIFGdvDefineVariable(varname, expr, infn, line_cnt)
char	*varname;
char	*expr;
char    *infn;
i4      line_cnt;
{
	bool stat = TRUE;
	IITOKINFO *p_tok;

	if ((p_tok = IIFGftFindToken(varname)) != NULL)
	{
            /* $var exists. */
            if (p_tok->tokclass == FG_TOK_BUILTIN)
            {
                /* trying to re-DEFINE a builtin $variable */
                IIFGerr(E_FG0063_Duplicate_Var, FGFAIL,
                    3, (PTR)varname, (PTR)&line_cnt, (PTR)infn);
                /*NOTREACHED*/
                stat = FALSE;
                goto end;
            }
            else if ((stat = IIFGrdvReDefineVariable(p_tok, expr)) != TRUE)
            {
                /* unable to redefine existing $var */
                IIFGerr(E_FG0065_Bad_Type_Define, FGFAIL,
                    3, (PTR)varname, (PTR)&line_cnt, (PTR)infn);
                /*NOTREACHED*/
                goto end;
            }
	}
	else if ((stat = IIFGcvCreateVariable(varname, expr,
						infn, line_cnt)) != TRUE)
	{
            /* unable to create new $var */
            IIFGerr(E_FG0065_Bad_Type_Define, FGFAIL,
                3, (PTR)varname, (PTR)&line_cnt, (PTR)infn);
            /*NOTREACHED*/
            goto end;
	}

end:
	return stat;
}

/*{
** Name:	IIFGcvCreateVariable - create new $variable.
**
** Description:
**	Create a new $variable.
**	It is assumed that this $variable name is valid and does not already
**	exist in Iitokinfo[] (caller has done IIUGcvnCheckVarname() &
**	IIFGftFindToken()).
**
** Inputs:
**	varname		- name of $variable to create. assumed valid.
**	expr		- value expression to give variable.
**			  (*expr==EOS) is valid and means no expression
**			  was given for the $var in the ##DEFINE stmt.
**      infn            - name of input file currently processing.
**      line_cnt        - current line number in input file.
**
** Outputs:
**
**	Returns:
**		TRUE if create went ok. FALSE otherwise.
**
** History:
**	18-mar-1992 (pete) (20th wedding anniversary)
**	    Initial version. Done as part of Windows4gl Amethyst project
**	    for 6.5 release.
**	19-jun-1992 (pete)
**	    Change syntax of ##statement defined earlier, as result of
**	    feedback from others. Make syntax more like cpp.
*/
bool
IIFGcvCreateVariable(varname, expr, infn, line_cnt)
char	*varname;
char	*expr;
char    *infn;
i4      line_cnt;
{
	DECLARED_IITOKS *p_new, *p_last;
	bool stat = TRUE;

	if ((p_new = (DECLARED_IITOKS *)IIFGgm_getmem(
	    (u_i4)sizeof(DECLARED_IITOKS), TRUE)) != (DECLARED_IITOKS *)NULL)
	{
	    /* initialize the new $variable (DECLARED_IITOKS entry) */

	    /* set $variable's name */
	    p_new->tok.tokname = (char *)IIFGgm_getmem(
					(u_i4)STlength(varname)+1, FALSE);
	    STcopy(varname, p_new->tok.tokname);

	    p_new->tok.tokclass = FG_TOK_DEFINED;

	    if (*expr == EOS)
		/* stmt was: "##DEFINE $var" (with no expression) */
		p_new->tok.toktran = Empty;
	    else
	    {
		/* ##DEFINE has an expression to evaluate */
	        if ((p_new->tok.toktran = IIUGeceEvalCharExpr(expr))
			== (char *)NULL)
	        {
		    /* error evaluating expression */
		    IIFGerr(E_FG0069_Bad_Expression, FGFAIL,
		        2, (PTR)&line_cnt, (PTR)infn);
		    /*NOTREACHED*/
		    stat = FALSE;
		    goto end;
	        }
	    }

	    /* place this new $variable at end of list */
	    p_last = find_last_iitok();
	    if (p_last == (DECLARED_IITOKS *)NULL)
		IIFGdcl_iitoks = p_new;		/* first one in list */
	    else
		p_last->next = p_new;		/* add to end of list */
	}
	else
	    stat = FALSE;

end:
	return stat;

}

/*{
** Name:	IIFGrdvReDefineVariable - redefine existing $var.
**
** Description:
**	Change definition of existing $variable.
**
** Inputs:
**	var		- details about $variable to redefine.
**	expr		- value expression to give variable.
**      infn            - name of input file currently processing.
**      line_cnt        - current line number in input file.
**
** Outputs:
**
**	Returns:
**		TRUE if all went ok. FALSE otherwise.
**
** History:
**	22-jun-1992 (pete)
**	    Initial version. Done as part of Windows4gl Amethyst project
**	    for 6.5 release.
*/
bool
IIFGrdvReDefineVariable(var, expr, infn, line_cnt)
IITOKINFO	*var;
char		*expr;
char		*infn;
i4		line_cnt;
{
	bool stat = TRUE;

	if (*expr == EOS)
	    /* stmt was: "##DEFINE $var" (with no expression) */
	    var->toktran = Empty;
	else
	{
	    /* ##DEFINE has an expression to evaluate */
	    if ((var->toktran = IIUGeceEvalCharExpr(expr)) == (char *)NULL)
	    {
		/* error evaluating expression */
		IIFGerr(E_FG0069_Bad_Expression, FGFAIL,
		    2, (PTR)&line_cnt, (PTR)infn);
		/*NOTREACHED*/
		stat = FALSE;
		goto end;
	    }
	}

	var->tokclass = FG_TOK_DEFINED;

end:
	return stat;
}

/*{
** Name:	IIFGuvUndefVariable - Delete (UNDEFine) a $variable.
**
** Description:
**	Delete a $variable.
**
** Inputs:
**	varname	- name of $variable to delete.
**
** Outputs:
**
**	Returns:
**		TRUE if $variable deleted ok. FALSE if $variable does
**		not exist.
**
** History:
**	23-mar-1992 (pete)
**	    Initial version.
*/
bool
IIFGuvUndefVariable(varname)
char *varname;
{
	IITOKINFO *p_tok;
	bool stat = TRUE;

	if ((p_tok = IIFGftFindToken(varname)) != (IITOKINFO *)NULL)
	{
	    if (p_tok->tokclass == FG_TOK_DEFINED)
		p_tok->tokclass = FG_TOK_DELETED;	/* mark token deleted */
	    else
		/* error. trying to ##UNDEF a built-in $variable */
		stat = FALSE;
	}
	else
	    stat = FALSE;

	return stat;
}

/* look for a $variable in the list of DEFINEd $variables */
static IITOKINFO *
find_declared_iitok(token)
char *token;
{
	DECLARED_IITOKS *p_tok = IIFGdcl_iitoks;

	while (p_tok != NULL)
	{
	    if ((STbcompare(token,0, p_tok->tok.tokname,0, TRUE) == 0) &&
		(p_tok->tok.tokclass != FG_TOK_DELETED))
		break;	/* found it and it hasn't been UNDEFined. */

	    p_tok = p_tok->next;	/* last one will be NULL */
	}

	return (p_tok == (DECLARED_IITOKS *)NULL)
		      ? (IITOKINFO *)NULL
		      : &(p_tok->tok);
}

/* find last $variable in list of DEFINEd $variables */
static DECLARED_IITOKS *
find_last_iitok()
{
	DECLARED_IITOKS *p_tok = IIFGdcl_iitoks;

	while (p_tok != NULL)
	{
	    if (p_tok->next == (DECLARED_IITOKS *)NULL)
		break;	/* last one */

	    p_tok = p_tok->next;	/* last one will be NULL */
	}

	return p_tok;
}

