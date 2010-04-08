/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<afe.h>
# include	<fmt.h>
# include	 <rtype.h>
# include	 <rglob.h>
# include	<errw.h>
# include       <er.h>

/*
**   R_PAR_TYPE - set a parameter to the given datatype if the latter is more
**		preferable to the parameter's current datatype.
**		Its previous type and the given type are checked
**		for compatiblity.
**		If the types are not compatible, give an error.
**
**	Parameters:
**		name	parameter name.
**		type	datatype to change to if it is more preferable
**			to its previous datatype.
**
**	Returns:
**		none.
**
**	Called by:
**		r_par_get, r_g_expr.
**
**	Side Effects:
**		none.
**
**	Error Messages:
**		107.
**
**	Trace Flags:
**		none.
**
**	History:
**		7/1/81 (ps)	written.
**		1/3/84 (gac)	rewrote for expressions.
**		9/16/85 (marian)	bug #5905
**			The wrong error message was being sent to r_error.
**			Changed from 515 to 107.
**		2/13/90 (elein) b8377
**			Don't just set undeclared dbv to the length
**			of the given type (which is usually the other
**			operator in an expression).  If this is an
**			undeclared variable, we haven't gotten the
**			value yet and the length of the variable might
**			be longer than the value of this type.  Use the
**			adc_minmaxdv to set the length to the maximum.
**			If the type is CHR, rfixparl will shrink the length
**			after it prompts for the value.  
**		8/27/90 (elein) b32654
**			adc_minmaxdv doesn't handle nullable datatypes.
**			Check and make nullable explicitly
**
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	15-apr-97 (dacri01)
**		Bug 81567: If new datatype is DB_FLT_TYPE, inherit the same
**		length as source data item.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_par_type(name, type)
char		*name;
DB_DATA_VALUE	*type;
{
    PAR		*par;
    bool		cancoerce;
    STATUS	status;

    i4	nullable;

    if( type->db_datatype < 0)
	nullable = TRUE;
    else
	nullable = FALSE;

    if (r_par_find(name, &par))
    {	/* found parameter */
	if (type->db_datatype != DB_NODT)
	{
	    if (par->par_value.db_datatype == DB_NODT)
	    {
		par->par_value.db_datatype = abs(type->db_datatype);
		/* Dont shortcut by just using type->db_length,
		** use maximum for this type of undeclared value
		*/
		status = adc_minmaxdv(Adf_scb, NULL, &(par->par_value));
		if (status != OK)
		{
			FEafeerr(Adf_scb);
		}
		if( nullable )
		{
			AFE_MKNULL(&(par->par_value));
		}
	    }
	    else
	    {
		afe_cancoerce(Adf_scb, &(par->par_value), type, &cancoerce);
		if (cancoerce)
		{
		    if (!par->par_declared &&
			afe_preftype(type->db_datatype,
				     par->par_value.db_datatype))
		    {	/* given type is preferable to current type */
			par->par_value.db_datatype = abs(type->db_datatype);
			/* Dont shortcut by just using type->db_length,
			** use maximum for this type of undeclared value
			*/
			if (par->par_value.db_datatype == DB_FLT_TYPE)
			    par->par_value.db_length = type->db_length;
			else
			{
				status = adc_minmaxdv(Adf_scb, NULL, &(par->par_value));
				if (status != OK)
				{
					FEafeerr(Adf_scb);
				}
			}
			if( nullable )
			{
				AFE_MKNULL(&(par->par_value));
			}

		    }
		}
		else
		{
		    r_error(107, NONFATAL, name, Cact_tname, Cact_attribute,
			    Cact_command, Cact_rtext, NULL);
		}
	    }
	}
    }
    else
    {
	r_syserr(E_RW0039_r_par_type__parameter);
    }
}
