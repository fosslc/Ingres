/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)sglog.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <stype.h> 
# include	 <sglob.h> 

/*
**   S_G_LOGOP  - get a logical operator from RTEXT.
**
**	Parameters:
**		token		pointer to operator string returned
**				by this routine.
**
**	Returns:
**		operator id:
**			OP_NONE		no logical operator
**			OP_NOT		NOT operator
**			OP_AND		AND operator
**			OP_OR		OR operator
**
**	Side effects:
**		Tokchar points after the word if it is a logical operator.
**
**	Error messages:
**		none.
**
**	Trace Flags:
**		none.
**
**	History:
**		11/29/83 (gac)	written.
**              11-oct-90 (sylviap)
**                      Added new paramter to s_g_skip.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



ADI_OP_ID
s_g_logop(token)
char	**token;
{
	/* internal declarations */

	ADI_OP_ID	op = OP_NONE;
	char		*save_Tokchar;
	i4             rtn_char;               /* dummy variable for sgskip */

	if (s_g_skip(FALSE, &rtn_char) == TK_ALPHA)
	{
		save_Tokchar = Tokchar;
		*token = s_g_name(TRUE);
		op = r_g_ltype(*token);
		if (op == OP_NONE)	/* Backup to start of word */
		{
			Tokchar = save_Tokchar;
		}
	}

	return(op);
}
