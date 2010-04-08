/*
**	Copyright (c) 1989, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<cm.h>
# include	<er.h>
# include	<nm.h>
# include	<ug.h>
# include	<erfg.h>
# include	<ooclass.h>

# include	<metafrm.h>
# include	"framegen.h"

/**
** Name:	fgifutl.c - Utility routines evaluate/push/pop ##if statements.
**
** Description:
**	these routines are called from framegen.c
**	This file defines:
**
** IIFGif_eval()     - evaluate if a ##if statement is TRUE or FALSE.
** IIFGifdef_eval()  - evaluate if a ##ifdef statement is TRUE or FALSE.
** IIFGelse_eval()   - evaluate a ##else statement.
** IIFGpushif()      - push an IFINFO struct on the "if" stack.
** IIFGpopif()	- pop a IFINFO structure off the "if" stack.
**
** History:
**	4/1/89	(pete)	written
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
/* GLOBALDEF's */
bool	IIFGif_eval();
VOID	IIFGpushif();
i4	IIFGelse_eval();

/* extern's */
GLOBALREF i4  Ii_tos;		/* top of 'if/else' stack */
GLOBALREF IFINFO Ii_ifStack[];

FUNC_EXTERN IITOKINFO   *IIFGftFindToken(); /* find $variable (token) */

/* static's */
static	char	Dollar[]	= ERx("$");

/*{
** Name:   IIFGifdef_eval() - evaluate if an "ifdef" statement is TRUE or FALSE
**
** Description:
**	Evaluate an ifdef statement, which can contain either a $$env_var,
**	or possibly a $var, to be either TRUE or FALSE, or an error.
**
** Inputs:
**	p_varname	- pointer to string for second item on a
**			  "##ifdef" statement. (before substitution).
** Outputs:
**	Returns:
**		result of IFDEF evaluation: TRUE or FALSE
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	4/24/90 (pete)	Initial version.
**	24-jun-1992 (pete)
**	    Extend for new method of statement substitution.
*/
bool
IIFGifdef_eval(p_varname, infn, line_cnt)
char 	*p_varname;
char	*infn;		/* name of input file currently processing */
i4	line_cnt;	/* current line number in input file */
{
	bool ret = TRUE;
	char *p_orig = p_varname;

	if (CMcmpcase(p_varname, Dollar) == 0)
	{
	    /* $var or $$var */

	    CMnext(p_varname);
	    if (CMcmpcase(p_varname, Dollar) == 0)
	    {
		/* $$var */
                char *tran;

	        CMnext(p_varname);

                NMgtAt(p_varname, &tran);
                ret = (((tran == NULL) || (*tran == EOS)) ? FALSE : TRUE);
	    }
	    else
	    {
		/* $var */
                ret = (IIFGftFindToken(p_orig) != (IITOKINFO *)NULL);
	    }
	}
	else
	{
	    /* $var or $$var missing */
	    ret = FALSE;
	}

	return ret;
}

/*{
** Name:	IIFGpushif()  - push an IFINFO struct on the "if" stack.
**
** Description:
**	push information on an "##if" statement onto a stack. Stack needed
**	cause ##if/endif statements can be nested. Up to MAXIF_NEST "if"
**	statements can be put on the stack.
**
**	first if that evaluates to false sets pgm_state. pgm_state is reset
**	each time an if is poped. Example of stack:
**		TRUE	PROCESSING
**		FALSE	PROCESSING	--  & set  pgm_state=FLUSHING 
**		TRUE	FLUSHING
**		FALSE	FLUSHING
**
** Inputs:
**	if_res	pointer to IFINFO structure to be pushed on stack.
**
** Outputs:
**
**	Returns:
**		STATUS 
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	4/1/89 (pete)	written
*/
VOID
IIFGpushif(if_res, infn, line_cnt)
IFINFO *if_res;
char	*infn;		/* name of input file currently processing */
i4	line_cnt;	/* current line number in input file */
{
	if (Ii_tos >= MAXIF_NEST)
	{
	    i4  maxif = MAXIF_NEST;
	    IIFGerr(E_FG0002_IfStmt_Nesting, FGFAIL, 3,
		(PTR) &maxif, (PTR) &line_cnt, (PTR) infn);
	}

	/* don't STRUCT_ASSIGN something to itself */
	if (if_res != &Ii_ifStack[Ii_tos])
	{
	    STRUCT_ASSIGN_MACRO (*if_res, Ii_ifStack[Ii_tos]);
	}

	Ii_tos++;
}

/*{
** Name:	IIFGpopif()	- pop a IFINFO structure off the stack.
**
** Description:
** 	pop a IFINFO structure off the stack. This gets called when
**	a "##endif" statement is read in the template file.
**
** Inputs:
**	char	*infn		name of input file currently processing
**	i4	line_cnt	current line number in input file
**
** Outputs:
**
**	Returns:
**		pointer to an IFINFO structure.
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	4/1/89 (pete)	written
*/
IFINFO
*IIFGpopif(infn, line_cnt)
char	*infn;		/* name of input file currently processing */
i4	line_cnt;	/* current line number in input file */
{
	Ii_tos-- ;
	if (Ii_tos < 0)
	{
	    IIFGerr(E_FG0003_Mismatch_ElseEndif, FGFAIL, 2,
		(PTR) &line_cnt, (PTR) infn);
	}
	return &Ii_ifStack[Ii_tos];
}

/*{
** Name:	IIFGelse_eval	- evaluate an else statement.
**
** Description:
**
** Inputs:
**	NONE
**
** Outputs:
**	Returns:
**		indication of what to set global pgm_state to, or if an error
**		occurred, then indicate that.
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	4/12/89 (pete)	written
*/
i4
IIFGelse_eval(infn, line_cnt)
char	*infn;		/* name of input file currently processing */
i4	line_cnt;	/* current line number in input file */
{
    IFINFO *if_res; 
    i4  stat;

    if_res = IIFGpopif(infn, line_cnt);

    if (if_res->has_else == TRUE)
    {
	/* two ##else's in a row */
	IIFGerr(E_FG0004_Too_Many_Elses, FGFAIL, 2,
		(PTR) &line_cnt, (PTR) infn);
	/*NOTREACHED*/
    }
    else
    {	/* popped ok */
	/* now evaluate the "if" we popped, which matches this "##else" */
	if ((if_res->iftest == FALSE) && (if_res->pgm_state == PROCESSING))
	{
	    stat = PROCESSING;
	}
	else
	{
	    stat = FLUSHING;
	}

	if_res->has_else = TRUE;	/* this "if" has a "else" */	

	/* push back what we popped & tested above */
	IIFGpushif(if_res, infn, line_cnt);
    }

    return stat ;
}
