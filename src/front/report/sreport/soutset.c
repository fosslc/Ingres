/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <stype.h> 
# include	 <sglob.h> 
# include	<cm.h>
# include	<er.h>

/*
**   S_OUT_SET - called in response to the .OUTPUT command to set up the
**	default output file for the report.
**	Format of OUTPUT command is:
**		.OUTPUT filename|"directory:filename"
**	No check is made to see if the file name is good.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Sets St_o_given.  Temporarily changes Command fields.
**
**	Called by:
**		s_readin.
**
**	Error Messages:
**		906, 912, 922.
**
**	Trace Flags:
**		13.0, 13.10.
**
**	History:
**		6/1/81 (ps)	written.
**		12/22/81 (ps)	modified for two table version.
**		08/06/86 (drh)	Added TK_SQUOTE support
**		25-mar-1987 (yamamoto)
**			Modified for double byte characters.
**		7/30/89 (elein) garnet
**			Allow passing of $vars for output name
**              2/20/90 (martym)
**                      Casting all call(s) to s_w_row() to VOID. Since it
**                      returns a STATUS now and we're not interested here.
**		08-oct-90 (sylviap)
**			Preserves quotes about the output filename if 
**			  specified. (#33314)
**			Does not CVlower the filename.  Preserves case. (#33778)
**              11-oct-90 (sylviap)
**                      Added new paramter to s_g_skip.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


FUNC_EXTERN STATUS s_w_row();

VOID
s_out_set()
{
	/* internal declarations */

	char		*name = NULL;		/* hold name of table */
	i4             rtn_char;               /* dummy variable for sgskip */

	/* start of routine */


	if (Cact_ren == NULL)
	{
		s_error(906, FATAL, NULL);
	}

	if (St_o_given)
	{	/* already specified */
		s_error(922, NONFATAL, Cact_ren->ren_report, NULL);
	}

	save_em();		/* temp store of fields */

	/* Set up fields in Command */

	STcopy(NAM_OUTPUT,Ctype);
	Csequence = 0;
	STcopy(ERx(" "), Csection);
	STcopy(ERx(" "), Cattid);
	STcopy(ERx(" "), Ccommand);

	switch(s_g_skip(TRUE, &rtn_char))
	{
		case(TK_ALPHA):
		case(TK_NUMBER):
		case(TK_DOLLAR):
			/* should be ok */
			name = s_g_token(FALSE);
			break;

		case(TK_QUOTE):
			/* Keep the quotes #33314 */
			name = s_g_string(FALSE, (i4) TK_QUOTE );
			break;

		case(TK_SQUOTE):
			/* Keep the quotes #33314 */
			name = s_g_string(FALSE, (i4) TK_SQUOTE );
			break;
		default:
			s_error(912, NONFATAL, NULL);
			break;

	}

	if (name != NULL)
	{
		STcopy(name,Ctext);
		_VOID_ s_w_row();

	}

	get_em();

	St_o_given = TRUE;

	return;
}
