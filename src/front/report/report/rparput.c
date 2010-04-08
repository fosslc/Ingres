/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rparput.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	<ade.h>
# include	 <rtype.h> 
# include	 <rglob.h> 
# include	<cm.h>
# include	<ug.h>
# include	<er.h>

/*
**   R_PAR_PUT - put a string value in the PAR linked list for a parameter.
**		If the parameter is not in the list, add an entry for it.
**
**	Parameters:
**		name		parameter name to search for.
**		value		string value of parameter.
**		datatype	datatype of declared variable.
**		prompt_string	prompt string for declared variable.
**
**	Returns:
**		pointer to parameter in list.
**
**	Called by:
**		r_par_get, r_p_tparam.
**
**
**	Trace Flags:
**		none.
**
**	History:
**		7/1/81 (ps)	written.
**		1/3/84 (gac)	rewrote for expressions.
**		1/8/85 (rlm)	tagged memory allocation.
**		19-dec-88 (sylviap)
**			Changed memory allocation to use FEreqmem.
**		23-oct-1992 (rdrane)
**			Ensure set/propagate precision in DB_DATA_VALUE
**			structures.  Converted r_error() err_num values to hex
**			to facilitate lookup in errw.msg file.
**		26-may-1993 (rdrane)
**			Add support for hexadecimal string literals.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-Jun-2007 (kibro01) b118486/b117033/b117376
**	    Add par_set value to indicate that the variable has been set
**	    through the command-line interface already.  Add in flag to say
**	    if this is setting through the CLI.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/



PAR	*
r_par_put(name, value, datatype, prompt_string, set_through_cli)
	char		*name;
	char		*value;
	DB_DATA_VALUE	*datatype;
	char		*prompt_string;
	bool		set_through_cli;
{
	PAR	*par;
	PAR	*lastpar;
	i4	hex_chars;
	char	*save_Tokchar;


	r_par_clip(name);
	if (r_par_find(name, &par))
	{
		if  ((par->par_declared) && ((DB_DATA_VALUE *)datatype != NULL))
		{
			r_error(0x37, NONFATAL, name, Cact_command, Cact_rtext,
				NULL);
		}
	}
	else
	{
		lastpar = par;

        	if ((par = (PAR *)FEreqmem ((u_i4)Rst4_tag,
			(u_i4)(sizeof(PAR)), TRUE, (STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("r_par_put"));
		}
							 
		par->par_name = name;
		par->par_string = NULL;
		par->par_declared = FALSE;
		par->par_set = FALSE;
		par->par_value.db_datatype = DB_NODT;
		par->par_value.db_length = ADE_LEN_UNKNOWN;
		par->par_value.db_prec = 0;
		par->par_value.db_data = NULL;
		par->par_prompt = NULL;
		
		if (lastpar == NULL)
		{	/* first parameter */
			Ptr_par_top = par;
		}
		else
		{
			lastpar->par_below = par;
		}
	}

	if  (datatype != (DB_DATA_VALUE *)NULL)
	{
		par->par_declared = TRUE;
		par->par_prompt = prompt_string;
		par->par_value.db_datatype = datatype->db_datatype;
		par->par_value.db_length = datatype->db_length;
		par->par_value.db_prec = datatype->db_prec;
	}

	if (par->par_string == NULL)
	{
		par->par_string = value;
		par->par_set = set_through_cli;
		/*
		** If it's a hexadecimal string literal, then we
		** need to resolve its value.  If we had a previous
		** datatype specification, then it had better have
		** been VARCHAR, or embedded ASCII NULs will fail.
		** If we were called for a WITH PROMPT, then there
		** is no value to look at, so skip all this.
		** Note tha "real" strings will begin with a quote!
		*/ 
		if  ((value != NULL) &&
		     ((*value == 'x') || (*value == 'X')))
		{
			if  ((datatype != (DB_DATA_VALUE *)NULL) &&
			     (abs(datatype->db_datatype) != DB_VCH_TYPE))
			{
				/*
				** We should display an error here ...
				*/
				return(par);
			}
			save_Tokchar = Tokchar;
			Tokchar = value;
			CMnext(Tokchar);
			hex_chars = r_g_hexlit((DB_TEXT_STRING **)&value);
			Tokchar = save_Tokchar;
			if  (hex_chars == 0)
			{
				/*
				** We should display an error here ...
				*/
				return(par);
			}
			par->par_string =
			   (char *)(&(((DB_TEXT_STRING *)value)->db_t_text[0]));
		}
	}

	return(par);
}
