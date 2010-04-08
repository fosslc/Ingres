/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<st.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h>
# include	 <rglob.h>
# include	<errw.h>

/*
**   R_PAR_REQ - if -p flag has been set, request the user to enter the
**		value of the parameter by prompting.
**
**	Parameters:
**		name	parameter name to get value for.
**		prompt	if not NULL, uses this prompt instead of the standard.
**
**	Returns:
**		Ptr to value associated with that name.
**		If -p not set, return NULL.
**
**	Called by:
**		r_par_get.
**
**	History:
**		7/1/81 (ps)	written.
**		1/3/84 (gac)	rewrote for expressions.
**		02-jan-90 (sylviap)
**			Added check for running in batch mode.  Does not
**			prompt for parameter values, so prints error.
**		3/22/90 (martym)
**			Took out reference to St_prompt.
*/



char	*
r_par_req(name, prompt)
register char	*name;
register char	*prompt;
{
	/* internal declarations */

	char		pbuf[MAXPNAME+20];	/* prompt buffer */
	char		*value = NULL;		/* value of parameter */
	char		*reply;

	/* start of routine */

	if (St_batch)
	{
		/* 
		** Running in batch/background mode, so cannot
		** prompt for any variables.  Print error, and terminate
		** report.
		*/
		r_error (1020, NONFATAL, name, NULL);
		value = NULL;
	}
	else
	{
		if (prompt != NULL)
		{
			reply = r_prompt(prompt, FALSE);
		}
		else
		{
			STprintf(pbuf, ERget(S_RW0038_Enter), name);
			reply = r_prompt(pbuf, FALSE);
		}
		value = (char *) STalloc(reply);
	}

	return(value);
}
