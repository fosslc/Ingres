/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	 <stype.h>
# include	 <sglob.h>
# include	<er.h>
# include	"ersr.h"

/*
**   S_SET_CMD - set up for a new RACTION command.
**
**	Parameters:
**		name - command name.
**
**	Returns:
**		none.
**
**	Called By:
**		s_readin, s_tok_add.
**
**	Side Effects:
**		Sets Ctype,Csection,Cattid,Ctext.
**
**	Trace Flags:
**		13.0, 13.6.
**
**	History:
**	6/1/81 (ps) - written.
**	26-aug-1992 (rdrane)
**		Converted s_error() err_num value to hex to facilitate
**		lookup in errw.msg file. Eliminate references to
**		r_syserr() and use IIUGerr() directly.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
s_set_cmd(name)
char	*name;
{
	/* start of routine */


	if (name == NULL)
	{
		IIUGerr(E_SR0011_s_set_cmd_Null_cmd,UG_ERR_FATAL,0);
	}


	/* if no report set up yet, bomb out */

	if (Cact_ren == NULL)
	{
		s_error(0x38A, FATAL, NULL);	/* ABORT!!! */
	}

	if (STlength(name) > MAXCMD)
	{
		*(name+MAXCMD) = '\0';			/* truncate it */
	}

	STcopy(name,Ccommand);
	STcopy(ERx(""),Ctext);			/* preset to NULL */

	return;
}
