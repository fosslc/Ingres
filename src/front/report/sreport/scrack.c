/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<st.h>
# include	 <stype.h>
# include	 <sglob.h>
# include	"ersr.h"

/*
**   S_CRACK_CMD - crack the command line call to the report
**	specifier.  The format of the command is:
**
**	SREPORT [flags] dbname filename
**
**	where	flags	are flags to the command:
**			-s - don't print any system messages, incuding prompts.
**			-t - save the temporary files.
**			-uname - used by the DBA, act as another user.
**			-X-V
**			-GgroupID - -G flag, used to restrict read access to 
**				the database.
**			-P - -P (password) flag, used to control access to the
**				DBMS as an additional level of security.
**		dbname	is the database name.
**		filename is the name of the file containing format cmds.
**
**	(In the development version, optional trace flags are
**	also allowed.)	This routine pulls out the repname,
**	filename, and flags and stuffs them into the
**	proper places.
**
**	Parameters:
**		argc - argument count from command line.
**		argv - address of an array of addresses containing
**			the starting points of parameter strings.
**
**	Returns:
**		none.
**
**	Called by:
**		s_main.
**
**	Side Effects:
**		Sets global variables En_database,
**		En_sfile, St_silent, St_keep_files,
**		En_uflag, En_gidflag, En_passflag.
**
**	Error messages:
**		900, 901.
**
**	Trace Flags:
**		11.0, 11.2.
**
**	History:
**		6/15/81 (ps)	written.
**		10/8/81 (ps)	change dbname/filename order on command
**				line and allow flags anywhere.	Also
**				add the -s flag and -t flag.
**		9/8/82	(jrc)	Added the -X flags.
**		12/29/89 (kenl)	Added handling of the +c 
**				(connection/with clause) flag.
**		3/22/90 (martym) Added -G (group ID) and -P (password), 
**				and got rid of -p (prompt for any missing 
**				information) which was obsolete.
**		20-jan-1992 (rdrane)
**			Use new constants in (s/r)_reset invocations.
**			Converted r_error() err_num value to hex
**			to facilitate lookup in errw.msg file.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

FUNC_EXTERN     char    *IIUIpassword();

VOID
s_crack_cmd (argc,argv)
i4		argc;
char		*argv[];
{
	/* internal declarations */

	char		*c;			/* ptr to char buffer */
 	char		*with_ptr;
	i4		i;			/* used as counter */

	/* start of routine */


	r_reset(RP_RESET_CALL,RP_RESET_LIST_END); /* reset for call */
	s_reset(RP_RESET_PGM,RP_RESET_LIST_END);

	/* first pull out any flags. */

	i = St_called ? 0 : 1;

	for(; (i<argc); i++)
	{	/* next parameter */
		if (*(argv[i]) == '-')
		{	/* flag found. Set and clear out */
			switch(*(argv[i]+1))
			{
				case('G'): /* -G flag set */
					En_gidflag = argv[i];
					break;

				case('P'): /* -P (password) flag is set: */
					En_passflag = argv[i];
					break;


				case('R'):
					/* ignore debugging flags */
					break;

				case('s'):
				case('S'):
					St_silent = TRUE;
					break;

				case('t'):
				case('T'):
					St_keep_files = TRUE;
					break;

				case('u'):
				case('U'):
					En_uflag = argv[i];
					break;

				case('X'):
					r_xpipe = (argv[i]);
					break;

				default:
					r_error(0x384, FATAL, argv[i], NULL);
			}
			argv[i] = NULL;		/* clear out the flag */
		}
		else if (*(argv[i]) == '+')
		{
			switch(*(argv[i]+1))
			{
				case('c'):	/* connection parameters */
				case('C'):
					/* set up WITH clause for CONNECT */
					with_ptr = STalloc(argv[i] + 2);
					IIUIswc_SetWithClause(with_ptr);
					break;
			}
			argv[i] = NULL;		/* clear out the flag */
		}
	}


	/*
	** If the '-P' flag has been set, prompt for the password:
	*/
	if (En_passflag != NULL)
	{
		char *SavFlag = En_passflag;
		/*
		** If there is something wrong with the '-P' flag,
		** such as the user has specified the password on the
		** command line, IIUIpassword() will return NULL.
		** In such cases bail out:
		*/
		if ((En_passflag = IIUIpassword(En_passflag)) == NULL)
			r_error(0x384, FATAL, SavFlag, NULL);
	}


	/* now check to see that the database name was specified.
	**  If not, ask for one: */

	i = St_called ? 0 : 1;

	for(; (i<argc); i++)
	{	/* check next parameter */
		if (argv[i]==NULL)
		{
			continue;
		}
		En_database = argv[i++];
		break;
	}

	if (!St_called && (STlength(En_database)<1))
	{	/* no database name. Ask for one: */
		c = r_prompt(ERget(FE_Database), TRUE);
		En_database = (char *) STalloc(c);
	}

	/* See if the file name specified.  If not, 
	** ask for it:			*/

	for(; (i<argc); i++)
	{	/* check next parameter */
		if (argv[i]==NULL)
		{
			continue;
		}
		En_sfile = argv[i++];
		break;
	}

	if (STlength(En_sfile) < 1)
	{	/* no file name. Ask for one: */
		c = r_prompt(ERget(S_SR0001_Report_File), TRUE);
		En_sfile = (char *) STalloc(c);
	}

	return;
}
