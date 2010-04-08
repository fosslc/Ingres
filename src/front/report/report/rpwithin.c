/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include       <st.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 
# include	<cm.h>
# include	<er.h>

/*
**   R_P_WITHIN - process a .WITHIN command, which may set up some
**	number of TCMD structures.  The .WITHIN command is used to
**	set up a column environment for a set of printing commands.
**
**	The format of the command is:
**		[.BEGIN] .WIthin colname {,colname} | ALL
**			...some formatting commands ...
**		.END [WITHIN]
**
**	The block of commands is executed once for each column 
**	specified.  Within the block of commands, the implicit
**	left and right margins are those for the column, rather
**	than the report, so all .CENTER, .RIGHT, .LEFT commands
**	refer to the column by default rather than the report.
**
**	When the .WITHIN command is in effect, it automatically
**	sets the report writer in block mode, as if a .BLOCK
**	command had been entered.  Therefore, .NEWLINES within
**	the .WITHIN commands do not actually write out to the
**	report file, but rather to the internal buffer.  
**	When the .END within command is found, an automatic
**	.TOP command is executed, so the next printing is on
**	the same line in the report as it was on when the .WITHIN
**	was initially encountered.  To get it to print, a .NL
**	is required.
**
**	The keyword "all" may be specified for the column name,
**	in which case, all columns are used.
**
**	Note that a series of separate .WITHIN commands, as long
**	as no other commands are in between them, act the same as
**	the .WITHIN a,b,c type of command.
**
**	Since the .WITHIN is a block set of commands, it uses the
**	STK structures when executing.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Called by:
**		r_act_set.
**
**	Side Effects:
**		May update the Cact_tcmd pointer.
**		May update St_cwithin and St_ccoms.
**		May temporarily change the Cact_rtext ptr.
**
**	Error Messages:
**		162, 163, 164, 165.
**
**	Trace Flags:
**		100, 101.
**
**	History:
**		1/6/82 (ps)	written.
**		12/5/83 (gac)	allow unlimited-length parameters.
**		8/11/89 (elein) garnet
**			Added evaluation of $vars as column names
**      3/20/90 (elein) performance
**          Change STcompare to STequal
**		08-oct-92 (rdrane)
**			Converted r_error() err_num value to hex to facilitate
**			lookup in errw.msg file.  Allow for delimited
**			identifiers and normalize them before lookup.  Skip to next token
**			after failed parameter lookup so we don't attempt to process with
**			a potentially garbage pointer!  Handle "ALL" specially, since it's
**			a 6.5 reserved word, and so will fail unless it's specified as a
**			delimited identifier.  Note that now ALL as a regular identifier
**			represents the keyword, while "ALL" as a delimited identifier does
**			not represent the keyword!
**			Removed additional memory allocation for returned parameter values
**			since this is already accomplished as part of r_par_get().
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
r_p_within()
{
	/* internal declarations */

	ATTRIB		ordinal = 0;		/* ordinal of found attribute */
	char		*aname;			/* name of found attribute */
	char		*tmpstr;			/* parameter value */
	i4			type;			/* token type */
	bool		getstruct = FALSE;	/* true when new TCMD needed */
	bool		is_all = FALSE;	/* true if "ALL" parameter seen */
	bool		gotanything = FALSE;	/* true if we get anything */
	static char	tstring[1] = {0};	/* temp Cact_rtext */

	/* start of routine */


	if (St_cwithin!=NULL && St_ccoms!=NULL)
	{	/* currently in a .WITHIN block.  Bad nesting */
		r_error(0xA5, NONFATAL, Cact_tname, Cact_attribute, 
			Cact_command, Cact_rtext, NULL);

		/* now end the previous one */

		r_g_set(tstring);		/* avoid extra char error */
		r_p_ewithin();
		r_g_set(Cact_rtext);

		Cact_tcmd = r_new_tcmd(Cact_tcmd);	/* get a new one */
	}

	while ((type=r_g_eskip()) != TK_ENDSTRING)
	{
		switch (type)
		{
			case(TK_COMMA):
				CMnext(Tokchar);	/* ignore */
				break;

			case(TK_ALPHA):
			case(TK_QUOTE):
			case(TK_DOLLAR):
				if  ((type == TK_QUOTE) && (!St_xns_given))
				{
					r_error(0xA4, NONFATAL, Tokchar, Cact_tname, 
							Cact_attribute, Cact_command, Cact_rtext, NULL);
					return;
				}
				if  (type == TK_DOLLAR)
				{
					CMnext(Tokchar);
					tmpstr = r_g_name();
					if ((aname = r_par_get(tmpstr)) == NULL)
					{
						r_error(0x3F0, NONFATAL, tmpstr, NULL);
						return;
					}
				}
				else
				{
					aname = r_g_ident(FALSE);
				}

				/* new TCMD structure needed ? */
				if (getstruct)
				{
					Cact_tcmd = r_new_tcmd(Cact_tcmd);
				}
				getstruct = TRUE;

				if  (STbcompare(aname,0,ERx("all"),0,TRUE) == 0)
				{
					is_all = TRUE;
				}
				else
				{
					_VOID_ IIUGdlm_ChkdlmBEobject(aname,aname,FALSE);
				}

				if ((!is_all) && ((ordinal = r_mtch_att(aname)) <= 0))
				{	/* not an attribute */
					r_error(0xA3, NONFATAL, aname, Cact_tname, Cact_attribute,
							NULL);
				}
				else
				{	/* good attribute name */
					if (is_all)
					{
						ordinal = 1;
					}

					if (St_cwithin == NULL)
					{	/* put in a start block command */
						Cact_tcmd->tcmd_code = P_BLOCK;
						Cact_tcmd = r_new_tcmd(Cact_tcmd);
					}

					Cact_tcmd->tcmd_code = P_WITHIN;
					Cact_tcmd->tcmd_val.t_v_long = ordinal;

					if (St_cwithin == NULL)
					{
						St_cwithin = Cact_tcmd;
						St_ccoms = NULL;
						r_psh_be(Cact_tcmd);
					}

					if (is_all)
					{	/* keyword "ALL" specified */
						while((r_gt_att(++ordinal))!=NULL)
						{	/* next att in list */
							Cact_tcmd = r_new_tcmd(Cact_tcmd);
							Cact_tcmd->tcmd_code = P_WITHIN;
							Cact_tcmd->tcmd_val.t_v_long = ordinal;
						}
					}
				}
				break;

			default:
				/* anything else is garbage */
				r_error(0xA4, NONFATAL, Tokchar, Cact_tname, 
					Cact_attribute, Cact_command, Cact_rtext, NULL);
				return;

		}

		gotanything = TRUE;
	}

	if (!gotanything)
	{	/* nothing specified for within */
		r_error(0xA2, NONFATAL, Cact_tname, Cact_attribute, NULL);
	}

	return;
}

