/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rglog.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 

/*
**   R_G_LOGOP  - get a logical operator from RTEXT.
**
**	Parameters:
**		none.
**
**	Returns:
**		type of operator:
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
**		12/19/83 (gac)	written.
*/



ADI_OP_ID
r_g_logop()
{
	/* internal declarations */

	ADI_OP_ID	type = OP_NONE;
	char		*save_Tokchar;
	char		*token;
	ADI_OP_ID	r_g_ltype();

	if (r_g_eskip() == TK_ALPHA)
	{
		save_Tokchar = Tokchar;
		token = r_g_name();
		type = r_g_ltype(token);
		if (type == OP_NONE)	/* Backup to start of word */
		{
			Tokchar = save_Tokchar;
		}
	}

	return(type);
}
