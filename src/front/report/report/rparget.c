/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<st.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 

/*
**   R_PAR_GET - get a parameter value from the PAR linked list.
**	If it is already there, return it.  If it isn't, and the
**	-p flag has been set, prompt for it, and set its type to string.
**
**	Parameters:
**		name	parameter name to search for.
**
**	Returns:
**		Ptr to string associated with that name.
**		If -p not set, and can't find it, return NULL.
**
**	Called by:
**		r_qur_set.
**
**	Side Effects:
**		May set Ptr_par_top.
**
**	Error Messages:
**		none.
**
**	Trace Flags:
**		2.0, 2.12.
**
**	History:
**		7/1/81 (ps)	written.
**		1/3/84 (gac)	rewrote for expressions.
**		9/6/85 (marian) bug # 6202
**				don't lowercase prompt value
**		13-Jun-2007 (kibro01) b118486
**			Note this isn't the CLI in setting parameter
**		20-Sep-2007 (kibro01) b119155
**			If value is provided, mark that the parameter is set
*/



char	*
r_par_get(name)
register char	*name;
{
	/* internal declarations */

	PAR	*par;
	char	*value;
	char	*r_par_req();
	char	*newspot;
	char	lcasename[MAXPNAME +2];		/* lowercase name - bug # 6202 */

	/* start of routine */

	St_parms = TRUE;	/* this report does indeed have parameters */
	STcopy(name, lcasename);
	r_par_clip(lcasename);

	if (r_par_find(lcasename, &par))
	{
		value = par->par_string;
	}
	else
	{	/* no parameter found */
		
		/* don't force lowercase prompt -- bug # 6202 */
		value = r_par_req(name, NULL);	/* prompt user for value */

		/* use lowercase in table */
		newspot = (char *) STalloc(lcasename);
		/* If parameter value has been given, mark that it is now set
		** (kibro01) b119155
		*/
		r_par_put(newspot, value, NULL, NULL,
			(value != NULL) ? TRUE : FALSE );
	}

	return(value);
}
