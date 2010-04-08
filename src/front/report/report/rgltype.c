/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 
# include	<er.h>


/*
**   R_G_LTYPE  --  return the logical type of a token.
**
**	Parameters:
**		token		string of operator.
**
**	Returns:
**		type of operator:
**			OP_NONE		no logical operator
**			OP_NOT		NOT operator
**			OP_AND		AND operator
**			OP_OR		OR operator
**
**	Error messages:
**		none.
**
**	Trace Flags:
**		none.
**
**	History:
**		12/19/83 (gac)	written.
*/

ADI_OP_ID
r_g_ltype(token)
char	*token;
{
	ADI_OP_ID	type = OP_NONE;

	if (STbcompare(token, 0,  ERx("not"), 0, TRUE) == 0)
	{
		type = OP_NOT;
	}
	else if (STbcompare(token, 0,  ERx("and"), 0, TRUE) == 0)
	{
		type = OP_AND;
	}
	else if (STbcompare(token, 0,  ERx("or"), 0, TRUE) == 0)
	{
		type = OP_OR;
	}

	return(type);
}
