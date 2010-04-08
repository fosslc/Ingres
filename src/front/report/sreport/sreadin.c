/*
** Copyright (c) 2004 Ingres Corporation
*/

/*	static char	Sccsid[] = "@(#)sreadin.c	30.1	11/14/84";	*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <stype.h> 
# include	 <sglob.h> 
# include	 <er.h> 
# include	"ersr.h"

/*
**   S_READIN - read in the text file and process the commands.  This
**	calls s_process to do the work.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Called by:
**		s_main.
**
**	Side Effects:
**		Opens the text file.
**
**	History:
**		6/1/81 (ps) - written.
**		11/10/83 (gac)	broke off loop to create s_process for if
**				statement.
**		07/24/89 (martym)
**			Fixed bug #6883(#6879), by modifying error 902 calls
**			to send in a string of blanks if En_sfile happens to
**			be null.
**		8/21/89 (elein) garnet
**			Added parameter of file name for recursive calls for
**			include
**		1/25/90 (elein)
**			ensure file_name has appropriate scope for LOCation
**
**		26-aug-1992 (rdrane)
**			Use new constants in s_reset() invocations.
**			Converted r_error() err_num value to hex to facilitate
**			lookup in errw.msg file.  r_fopen() returns STATUS, so
**			look for OK.  Explicitly declare function as returning
**			VOID.
**		05-Apr-93 (fredb)
**			Removed references to "St_txt_open", "Fl_input" will
**			be used instead.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-Aug-2004 (lazro01)
**	    BUG 112819
**	    Changed datatype of savelineno to i4 to accommodate value more 
**	    than 32K.
*/

static i4	nestcount=0;

VOID
s_readin(fname)
char *fname;
{
	char	*savename;
	i4	savelineno;
	i4	numerrors;
	FILE	*savefp;
	char	use_file_name[MAX_LOC+1];


	/* check and open the text file */
	if( nestcount == 0)
	{
		s_reset(RP_RESET_SRC_FILE,RP_RESET_LIST_END);
	}

	/* Save file name, line number and file pointer of previous call */
	savelineno = Line_num;
	savename   = En_sfile;
	savefp     = Fl_input;
	numerrors  = Err_count;

	/* Check and open rw source file */
	STcopy(fname, use_file_name);
	En_sfile = use_file_name;
	En_sfile = s_chk_file(En_sfile);
	if (r_fopen(En_sfile, &Fl_input) != OK)
	{	/* bad file */
		if (En_sfile == NULL)
		{
			r_error(0x386, FATAL, " ", NULL);	/* ABORT!!!*/
		}
		else
		{
			r_error(0x386, FATAL, En_sfile, NULL);	/* ABORT!!!*/
		}
	}
	Line_num = 0;
	if (!St_silent && (En_program != PRG_RBF) && nestcount != 0)
	{
		/* Indent */
		SIprintf("%*s",nestcount*3,ERx(" "));
		SIprintf(ERget(S_SR0014_including_file),En_sfile);
		SIflush(stdout);
	}
	nestcount++;

	/* Now do actual processing of commands */
	s_process(FALSE);

	/* Close the file, except if it was the first one */
	nestcount--;
	if( nestcount != 0 )
	{
		r_fclose(Fl_input);
		if (!St_silent && (En_program != PRG_RBF))
		{
			/* Indent */
			SIprintf("%*s",nestcount*3,ERx(" "));
			if( numerrors == Err_count)
			{
				SIprintf(ERget(S_SR0015_good_include),En_sfile);
			}
			else
			{
				SIprintf(ERget(S_SR0016_error_include),En_sfile,
					Err_count-numerrors);
			}
			SIflush(stdout);
		}
	}
	En_sfile = savename;	/* restore previous file name */
	Line_num = savelineno;	/* restore line number */
	Fl_input = savefp;	/* restore file pointer */
	return;
}
