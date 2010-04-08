/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ui.h>
# include	<cm.h>
# include	 <stype.h> 
# include	 <sglob.h> 

/*
**   S_BRK_SET - set the input file to header or footer text for a break.
**		 This is called in response to the .HEADER or .FOOTER command
**		 in the input file.
**
**	The format of the command is:
**		.HEADER attname|page|report
**	or
**		.FOOTER attname|page|report
**
**	Parameters:
**		brk_type - type of break text, either B_HEADER or B_FOOTER.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none, directly.
**
**	Called by:
**		s_readin.
**
**	History:
**		6/1/81 (ps) - written.
**      	8/10/89 (elein) garnet
**      		Allow $parameters to be passed through to report
**              04-oct-89 (sylviap)
**                      B8209: Compares 'name' to key words 'report' and 'page'.
**                      If fails test, then assumes it must be a column name, 
**			so it does an IIUGlbo to preserve DBMS case sensitivity.
**              11-oct-90 (sylviap)
**                      Added new paramter to s_g_skip.
**		17-sep-1992 (rdrane)
**			Add support for delimited identifiers.  Converted 
**			s_error() err_num value to hex to facilitate lookup
**			in errw.msg file.  Call s_g_name() for suspected
**			parameters, not s_g_token() - every one seems to...
**			Rename type parameter to brk_type to avoid any future
**			language extension confusion.
**
**      20-sep-93 (huffman)
**              Readonly memory is passed to CVlower function, causing
**              Access Violation on AXP VMS.
**
**		8-dec-1993 (rdrane)
**			Rework delimited identifier support such that
**			the attribute name is stored normalized (b54950).
**      02-feb-1995 (sarjo01) Bug 62630
**         Make id normalizing call to IIUGdbo_dlmBEobject()
**         only if id is not keywords 'page' or 'report'
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
s_brk_set(brk_type)
char	*brk_type;
{
	/* internal declarations */

	char		name[(FE_UNRML_MAXNAME+1)];
	i4		tok_type;
	i4             rtn_char;               /* dummy variable for sgskip */

	/* start of routine */

	tok_type = s_g_skip(TRUE, &rtn_char);
	switch(tok_type)
	{
		case TK_ALPHA:
		case TK_NUMBER:
		case TK_QUOTE:
		case TK_DOLLAR:
			if  (tok_type == TK_ALPHA)
			{
				STcopy(s_g_ident(FALSE,&rtn_char,FALSE),name);
				if  (rtn_char != UI_REG_ID)
				{
					s_error(0x38D,NONFATAL,
						name,NULL);
					break;
				}
				/* B8209: For gateways, we may fail the 
				** comparison against REPORT or PAGE due to 
				** case sensitiviy.  Find out if we've got a 
				** reserved word and use the constant name 
				** instead.  If it is not PAGE or REPORT, 
				** then it is a column name.
				*/
				if (STbcompare(name,0,NAM_REPORT,
					       0,TRUE) == 0)
				{
                                        STcopy(NAM_REPORT,name);
				}
				else if (STbcompare(name,0,NAM_PAGE,
						    0,TRUE) == 0 )
				{
					STcopy(NAM_PAGE, name);
				}
                                else
				       IIUGdbo_dlmBEobject(name,FALSE);
			}
			else if (tok_type == TK_QUOTE)	/* Delim ID	*/
			{
				if  (!St_xns_given)
				{
					s_error(0x38D,NONFATAL,
						name,NULL);
					break;
				}
				STcopy(s_g_ident(FALSE,&rtn_char,FALSE),name);
				if  (rtn_char != UI_DELIM_ID)
				{
					s_error(0x38D,NONFATAL,
						name,NULL);
					break;
				}
				_VOID_ IIUGdlm_ChkdlmBEobject(name,name,
							FALSE);
			}
			else
			{
				STcopy(s_g_name(FALSE),name);
				IIUGdbo_dlmBEobject(name,FALSE);
			}

			/* check for valid break */

			if (En_n_breaks > 0)
			{
				if (s_sbr_set(name, brk_type) == NULL)
				{
					s_error(0x39C, NONFATAL, name, NULL);
				}
			}
			else if (s_rso_set(name, brk_type) == NULL)
			{
				if (En_n_sorted > 0)
				{
					s_error(0x38C, NONFATAL, name, NULL);
				}
				else
				{
					s_error(0x39B, NONFATAL, name, NULL);
				}
			}
			return;

		default:
			s_error(0x38D, NONFATAL, NULL);

	}

	return;
}
