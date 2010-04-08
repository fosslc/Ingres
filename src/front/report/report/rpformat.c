/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rpformat.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 
# include	<cm.h>

/*
**   R_P_FORMAT - process a FORMAT or TFORMAT command, which can set
**	up some number of TCMD strucutures.  The FORMAT command
**	sets up a new default format for an attribute, and the TFORMAT
**	command sets up a temporary format to be used only on the
**	next printing of the attribute value.
**	The format of the command is:
**		FORMAT colname {,colname} (format) {,colname {,colname} (format) }
**
**	Parameters:
**		code - the code for type of command (P_FORMAT or P_TFORMAT)
**
**	Returns:
**		none.
**
**	Called by:
**		r_act_set.
**
**	Side Effects:
**		Updates the Cact_tcmd, maybe.
**
**	Error Messages:
**		105, 106, 107, 130, 131, 184.
**
**	Trace Flags:
**		3.0, 3.9.
**
**	History:
**		6/6/81 (ps)	written.
**		7/5/82 (ps)	modified to accept parameters.
**		12/5/83 (gac)	allow unlimited-length parameters.
**		2/1/84 (gac)	added date type.
**		23-mar-1987 (yamamoto)
**			Modified for double byte characters.
**		8/16/89 (elein) garnet
**			Changed error message # for bad parameter value.
**		08-oct-92 (rdrane)
**			Converted r_error() err_num value to hex to facilitate
**			lookup in errw.msg file.  Allow for delimited
**			identifiers and normalize them before lookup.  Remove
**			function declrations since they're now in the hdr
**			files.
**		11-nov-1992 (rdrane)
**			Fix-up parameterization of r_g_ident().
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
r_p_format(code)
ACTION	code;
{
	/* internal declarations */

	ATTRIB		ordinal = 0;		/* ordinal of found attribute */
	ATT		*attribute;		/* att struct of found att */
	char		*aname;			/* name of found att */
	FMT		*fmt;			/* FMT structure found */
	i4		type;			/* token type */
	TCMD		*ftcmd;			/* first TCMD structure in list
						** if more than one with same
						** format given */
	bool		getstruct = FALSE;	/* TRUE when new TCMD needed */
	char		*pval;			/* return from param match */
	STATUS		status;
	bool		valid;

	/* start of routine */



	ftcmd = Cact_tcmd;

	while ((type = r_g_eskip()) != TK_ENDSTRING)
	{
		switch (type)
		{
			case(TK_COMMA):
				CMnext(Tokchar);	/* ignore it */
				break;

			case(TK_ALPHA):
			case(TK_DOLLAR):
			case(TK_QUOTE):
				/* new TCMD structure needed ? */
				if (getstruct == TRUE)
				{
					Cact_tcmd = r_new_tcmd(Cact_tcmd);
					if (ordinal == 0)
					{	/* first att for next format */
						ftcmd = Cact_tcmd;
					}
				}
				getstruct = TRUE;

				if (type == TK_DOLLAR)
				{	/* parameter specified.  convert it */
					CMnext(Tokchar);	/* skip $ */
					aname = r_g_name();
					if ((pval = r_par_get(aname)) == NULL)
					{	/* Couldn't find parameter */
						r_error(0x3F0, NONFATAL, aname, NULL);
						return;
					}
					else
					{	/* good one */
						aname = (char *) MEreqmem(0,
							STlength(pval)+1,
							TRUE,NULL);
						STcopy(pval, aname);
					}
				}
				else
				{
					aname = r_g_ident(FALSE);
				}
				_VOID_ IIUGdlm_ChkdlmBEobject(aname,aname,FALSE);


				if ((ordinal = r_mtch_att(aname)) == A_ERROR)
				{	/* not an attribute */
					r_error(0x82, NONFATAL, aname, Cact_tname, 
							Cact_attribute, Cact_command, Cact_rtext, NULL);
					return;
				}
				Cact_tcmd->tcmd_val.t_v_ap = (AP *) 
						MEreqmem( (u_i4) 0,
							(u_i4) sizeof(AP),TRUE,
							(STATUS *) NULL);
				(Cact_tcmd->tcmd_val.t_v_ap)->ap_ordinal = ordinal;

				break;

			case(TK_OPAREN):
				/* make sure at least one att specified */

				if ((ordinal == 0) || (ordinal == A_ERROR))
				{
					r_error(0x83, NONFATAL, Tokchar, Cact_tname, Cact_attribute,
							Cact_command, Cact_rtext, NULL);
					return;
				}

				/* format found for these attributes */

				CMnext(Tokchar);
				if ((fmt=r_g_format()) != NULL)
				{
					if (r_g_eskip() == TK_CPAREN)
					{	/* good format */
						CMnext(Tokchar);


						for(; ftcmd!=NULL; ftcmd=ftcmd->tcmd_below)
						{	/* add to each TCMD */
							ordinal = (ftcmd->tcmd_val.t_v_ap)->ap_ordinal;

							if(!r_grp_set(ordinal))
							{
								r_error(0xB8, NONFATAL, Cact_tname,
										Cact_attribute, 
										Cact_command, Cact_rtext, NULL);
								return;
							}

							while((ordinal=r_grp_nxt())>0)
							{
								attribute = r_gt_att(ordinal);
								aname = attribute->att_name;

								/* match type with format */

								status = fmt_isvalid(Adf_scb, fmt,
													 &(attribute->att_value),
													 &valid);
								if (status != OK)
								{
									FEafeerr(Adf_scb);
								}

								if (!valid)
								{	/* bad match */
									r_error(0x6A, NONFATAL, aname, Cact_tname,
										Cact_attribute, Cact_command,
										Cact_rtext, NULL);
									return;
								}

								/* all is ok */
								ftcmd->tcmd_code = code;
								(ftcmd->tcmd_val.t_v_ap)->ap_format = fmt;
							}
						}
						ordinal = 0;
						break;

					}
				}
				/* fall through on error */

			default:
				r_error(0x69, NONFATAL, aname, Cact_tname, Cact_attribute,
						Cact_command, Cact_rtext, NULL);
				return;
		}

	}

	return;
}

