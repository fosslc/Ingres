/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rparfind.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include       <st.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 

/*
**   R_PAR_FIND - get a parameter value from the PAR linked list.
**
**	Parameters:
**		name	parameter name to search for.
**		par	pointer to pointer to parameter in parameter list
**			returned by this routine if parameter is found.
**			If the parameter is not found, this points to
**			the last parameter in the list.
**
**	Returns:
**		TRUE	if parameter found.
**		FALSE	otherwise.
**
**	Called by:
**		r_par_get, r_p_tparam, r_par_put, r_par_type.
**
**
**	Error Messages:
**		none.
**
**	Trace Flags:
**		none.
**
**	History:
**		7/1/81 (ps)	written.
**		1/3/84 (gac)	rewrote for expressions.
**              3/20/90 (elein) performance
**                      Change STcompare to STequal
*/



bool
r_par_find(name, par)
register char	*name;
register PAR	**par;
{
	PAR	*npar;
	PAR	*opar = NULL;

	for (npar = Ptr_par_top; npar != NULL; opar = npar, npar = opar->par_below)
	{
		if (STequal(name, npar->par_name) )
		{
			*par = npar;
			return(TRUE);
		}
	}

	*par = opar;
	return(FALSE);
}
