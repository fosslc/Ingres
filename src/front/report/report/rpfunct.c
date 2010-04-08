/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rpfunct.c	30.1	11/14/84"; */

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
# include	<afe.h>
# include	<cm.h>

/*
**	R_P_FUNCT -- set up an item pointing to a function and its arguments.
**
**	Parameters:
**		name		name of the function.
**		item		pointer to item returned by this procedure.
**		funct_type	datatype of function result.
**
**	Returns:
**		status:
**			GOOD_EXP	if function exists.
**			NO_EXP		if function name doesn't exist.
**			BAD_EXP		error.
**
**	Called by:
**		r_p_param.
**
**	Side Effects:
**		Updates the Tokchar.
**
**	Error Messages:
**		504.
**
**	Trace Flags:
**		none.
**
**	History:
**
**		1/24/84 (gac)	written.
**      3/20/90 (elein) performance
**          Change STcompare to STequal
**		08-oct-92 (rdrane)
**			Converted r_error() err_num value to hex to facilitate
**			lookup in errw.msg file.  Allow for delimited
**			identifiers and normalize them before lookup.  Remove
**			function declarations since they're now in the hdr
**			files.  Ensure db_prec is initialized/propagated for
**			proper DECIMAL support.
**		11-nov-1992 (rdrane)
**			Fix-up parameterization of r_g_ident().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



i4
r_p_funct(name, item, funct_type)
char		*name;
ITEM		*item;
DB_DATA_VALUE	*funct_type;
{
	/* internal declarations */

	ADI_OP_ID	op_code;
	DB_DATA_VALUE	exp_type;
	EXP	*exp;
	BRK	*brk;
	ATTRIB	ordinal;
	ATTRIB	ord;
	char	*attname;
	AFE_OPERS	ops;
	DB_DATA_VALUE	*aops[MAX_OPANDS];
	DB_DATA_VALUE	opval[MAX_OPANDS];
	AFE_OPERS	cops;
	DB_DATA_VALUE	*caops[MAX_OPANDS];
	DB_DATA_VALUE	copval[MAX_OPANDS];
	ADI_FI_DESC	fdesc;
	bool		cancoerce;
	STATUS		status;
	i4		i;
	bool		nodt_found;
	i4		token_type;

	/* start of routine */

	op_code = r_gt_funct(name);

	if (op_code != OP_NONE)
	{
		if (r_g_eskip() == TK_OPAREN)
		{
			CMnext(Tokchar);
			if (op_code == OP_BREAK)
			{
				token_type = r_g_eskip();
				if ((token_type == TK_ALPHA) ||
					((token_type == TK_QUOTE) && (St_xns_given)))
				{
					attname = r_g_ident(FALSE);
					_VOID_ IIUGdlm_ChkdlmBEobject(attname,attname,FALSE);
					ordinal = r_mtch_att(attname);
					if (ordinal > 0 || ordinal == A_GROUP)
					{
						if (!r_grp_set(ordinal))
						{
							r_error(0xB8, NONFATAL, Cact_tname, Cact_attribute,
								Cact_command, Cact_rtext, NULL);
							return(BAD_EXP);
						}
	
						while ((ord = r_grp_nxt()) > 0)
						{
							for (brk = Ptr_brk_top; brk != NULL;
								 brk = brk->brk_below)
							{
								if (ord == brk->brk_attribute)
								{
									break;
								}
							}
	
							if (brk == NULL)
							{	/* not a break attribute */
								r_error(0x72, NONFATAL, attname,
									Cact_tname, Cact_attribute,
									Cact_command, Cact_rtext, NULL);
								return(BAD_EXP);
							}
						}

						item->item_type = I_ATT;
						item->item_val.i_v_att = ordinal;
					}
					else if (STequal(attname, NAM_REPORT) )
					{
						item->item_type = I_ATT;
						item->item_val.i_v_att = A_REPORT;
					}
					else
					{
						r_error(0x82, NONFATAL, attname, 
							Cact_tname, Cact_attribute, 
							Cact_command, Cact_rtext, NULL);
						return(BAD_EXP);
					}
				}
				else
				{
					r_error(0x74, NONFATAL, Cact_tname, Cact_attribute, 
						Cact_command, Cact_rtext, NULL);
					return(BAD_EXP);
				}

				funct_type->db_datatype = DB_BOO_TYPE;
				funct_type->db_length = 1;
				funct_type->db_prec = 0;
				r_mak_tree(op_code, item);
			}
			else 
			{
			    item->item_type = I_NONE;
			    exp = r_mak_tree(op_code, item);

			    /* set up operand array for afe_fdesc */

			    ops.afe_ops = aops;
			    cops.afe_ops = caops;
			    ops.afe_opcnt = 0;
			    cops.afe_opcnt = 0;

			    /* get parameters if there are any */

			    nodt_found = FALSE;
			    for (i = 0; r_g_eskip() != TK_CPAREN &&
					i < MAX_OPANDS; i++)
			    {
				status = r_g_expr(&exp->exp_operand[i],
						  &exp_type);
				if (status == BAD_EXP)
				{
				    return BAD_EXP;
				}
				else if (status == GOOD_EXP &&
					 exp_type.db_datatype == DB_NODT)
				{   /* perhaps a runtime parameter */
				    nodt_found = TRUE;
				}

			        /* set up operand array for afe_fdesc */

			        ops.afe_opcnt++;
			        cops.afe_opcnt++;
			        aops[i] = &opval[i];
			        caops[i] = &copval[i];
			        opval[i].db_datatype = exp_type.db_datatype;
			        opval[i].db_length = exp_type.db_length;
			        opval[i].db_prec = exp_type.db_prec;

				token_type = r_g_eskip();
				if (token_type == TK_COMMA)
				{
				    CMnext(Tokchar);
				    if (r_g_eskip() == TK_CPAREN)
				    {
					/* argument expected */
					r_error(0x7D, NONFATAL, Cact_tname,
						Cact_attribute, Cact_command,
						Cact_rtext, NULL);
					return BAD_EXP;
				    }
				}
				else if (token_type != TK_CPAREN)
				{
				    /* , or ) expected */
				    r_error(0x7C, NONFATAL, Cact_tname,
					    Cact_attribute, Cact_command,
					    Cact_rtext, NULL);
				    return BAD_EXP;
				}
			    }

			    if (r_g_eskip() != TK_CPAREN)
			    {
				/* too many arguments or no ) */
				r_error(0x7E, NONFATAL, Cact_tname,
					Cact_attribute, Cact_command,
					Cact_rtext, NULL);
				return BAD_EXP;
			    }

			    if (!nodt_found)
			    {
				status = afe_fdesc(Adf_scb, op_code, &ops,
						   &cops, funct_type, &fdesc);
				if (status != OK)
				{
				    FEafeerr(Adf_scb);
				    r_error(0x1F9, NONFATAL, Cact_tname,
					    Cact_attribute, Cact_command,
					    Cact_rtext, NULL);
				    return BAD_EXP;
				}

				/*
				** now go through actual arguments, checking
				** that they are coercible to the datatype of
				** the corresponding formal argument for the
				** function.
				*/

				for (i = 0; i < ops.afe_opcnt; i++)
				{
				    afe_cancoerce(Adf_scb, &opval[i], &copval[i],
						  &cancoerce);
				    if (!cancoerce)
				    {
					r_error(0x1F8, NONFATAL, Cact_tname,
						Cact_attribute, Cact_command,
						Cact_rtext, NULL);
					return BAD_EXP;
				    }
				}
			    }
			    else
			    {
				funct_type->db_datatype = DB_NODT;
			    }
			}

			if (r_g_eskip() == TK_CPAREN)
			{
				CMnext(Tokchar);
			}
			else
			{
				/* closing parenthesis expected */
				r_error(0x71, NONFATAL, Cact_tname,
					Cact_attribute, Cact_command,
					Cact_rtext, NULL);
				return BAD_EXP;
			}
		}
		else
		{
			/* opening parenthesis expected */
			r_error(0x7F, NONFATAL, Cact_tname,
				Cact_attribute, Cact_command,
				Cact_rtext, NULL);
			return BAD_EXP;
		}

		return(GOOD_EXP);
	}
	else
	{
		return(NO_EXP);
	}
}

