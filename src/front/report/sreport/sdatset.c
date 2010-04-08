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
# include	 <stype.h> 
# include	 <sglob.h> 
# include	<er.h>

/*
**   S_DAT_SET - called in response to the .DATA command to set up the
**	data table for the report.
**	Format of DATA command is:
**		.DATA tabname
**	tabname may be a regular or a delimited identifier, may have an
**	owner qualification, and may be a parameter.  However, if an
**	owner.tablename construct is used, then a parameter may not
**	replace a component, i.e.,
**		$owner.["]tablename["]
**		["]owner["].$tablename
**		$owner.$tablename
**	are all invalid.
**	No check is made to see if the table exists and/or is accessable.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Sets ren_data field in Cact_ren.
**		Also sets up the Command fields.
**
**	Called by:
**		s_readin.
**
**	History:
**		6/1/81 (ps)	written.
**		12/22/81 (ps)	modified for two table version.
**		7/30/89 (elein) garnet
**			Allow passing of $vars as table name
**              2/20/90 (martym)
**                      Casting all call(s) to s_w_row() to VOID. Since it
**                      returns a STATUS now and we're not interested here.
**              11-oct-90 (sylviap)
**                      Added new paramter to s_g_skip.
**		16-sep-1992 (rdrane)
**			Add support for owner.tablename and/or delimited
**			identifiers.  Converted s_error() err_num value to
**			hex to facilitate lookup in errw.msg file.  Don't
**			write to s_w_row() if there were errors ...
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
s_dat_set()
{
	char		*name;			/* hold name of table */
	i4		tok_type;		/* type of this token */
	i4		rtn_char;		/*
						** dummy variable for s_g_skip()
						** and s_g_ident().
						*/


	save_em();		/* store old versions of Command fields */

	/* Set up fields for .DATA command */

	STcopy(NAM_TABLE,Ctype);
	Csequence = 0;
	STcopy(ERx(" "),Csection);
	STcopy(ERx(" "),Cattid);
	STcopy(ERx(" "),Ccommand);

	if (Cact_ren == NULL)
	{
		s_error(0x38A, FATAL, NULL);
	}

	if (St_q_started == TRUE)
	{	/* already specified .query or .data */
		s_error(0x3BC, NONFATAL, NULL);
	}
	St_q_started = TRUE;

	tok_type = s_g_skip(TRUE, &rtn_char);
	switch(tok_type)
	{
	case(TK_ALPHA):
	case(TK_QUOTE):
	case(TK_DOLLAR):
		if  (tok_type == TK_DOLLAR)
		{
			name = s_g_name(FALSE);
			IIUGdbo_dlmBEobject(name,FALSE);
		}
		else
		{
			name = s_g_tbl(FALSE,&rtn_char);
			if  (!rtn_char)
			{
			    break;
			}
		}
		STcopy(name,Ctext);
		break;
	default:
		s_error(0x38F, NONFATAL, NULL);
		break;
	}

	_VOID_ s_w_row();

	get_em();		/* restore the previous variables */

	return;
}
