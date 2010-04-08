/*
**	Copyright (c) 1992, 2004 Ingres Corporation
*/
# include       <compat.h>
# include       <st.h>          /* 6-x_PC_80x86 */
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include       <fe.h>

# include	<ugexpr.h>
# include       <erfg.h>
# include	<ooclass.h>
# include	<metafrm.h>
# include	"framegen.h"


/**
** Name:	fgdefine.c - evaluate a ##DEFINE statement.
**
** Description:
**
** This file defines:
**
**	IIFGdseDefineStmtExec	- process a ##DEFINE statement.
**
** History:
**	18-mar-1992 (pete)
**	    Initial version. Done for Amethyst/Windows4gl project for 6.5.
**	    (to keep W4gl preprocessor and Vision template lang. in sync).
**	18-jun-1992 (pete)
**	    Change syntax and semantics of ##DEFINE implemented earlier.
**	    Remove data types.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
/* GLOBALDEF's */

/* extern's */
FUNC_EXTERN	STATUS	IIUGcvnCheckVarname();
FUNC_EXTERN	bool	IIFGdvDefineVariable();

/* static's */

/*{
** Name:        IIFGdseDefineStmtExec - execute a DEFINE statement.
**
** Description:
**      Create a new $variable, or redefine the value of an existing one.
**
** Inputs:
**	varname		- Name of $variable to define.
**	stmt		- Portion of statement containing an expression. Will
**			  be evaluated to determine value to give "varname".
**      nmbr_words      - Number of words in "p_wordarray".
**      p_wordarray     - Array of pointers to each word in substituted
**			  statement buffer.
**      infn            - name of input file currently processing.
**      line_cnt        - current line number in input file.
**
** Outputs:
**      Returns:
**              TRUE if DEFINE went OK, FALSE otherwise.
**
**      Exceptions:
**              IIUGrefRegisterExpFns() must be called first, before this
**              procedure, so procedures called here-in will be known.
**
** Side Effects:
**
** History:
**      18-mar-1992 (pete)
**          Initial Version.
**	30-apr-1992 (pete)
**	    Moved CheckVarname from this file to utils!ug!fgutils.c. It's
**	    code can be shared with windows4gl.
*/
bool
IIFGdseDefineStmtExec(varname, stmt, nmbr_words, infn, line_cnt)
char	*varname;
char	*stmt;
i4	nmbr_words;
char    *infn;
i4      line_cnt;
{
	bool stat;

	if (nmbr_words < 2)
	{
	    /* incomplete statement */
	    IIFGerr(E_FG0064_Syntax_Define, FGFAIL,
		2, (PTR)&line_cnt, (PTR)infn);
	    /*NOTREACHED*/
	    stat = FALSE;
	    goto end;
	}

	/* check if legal name */
	if (IIUGcvnCheckVarname(varname) != OK)
	{
	    IIFGerr(E_FG0066_Bad_Var_Define, FGFAIL,
		3, (PTR)varname, (PTR)&line_cnt, (PTR)infn);
	    /*NOTREACHED*/
	    stat = FALSE;
	    goto end;
	}

	stat = IIFGdvDefineVariable(varname, stmt, infn, line_cnt);

end:
	return stat;
}
