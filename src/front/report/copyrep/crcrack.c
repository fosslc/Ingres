/*
**	 Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<me.h>
# include	<stype.h>
# include	<sglob.h>
# include	<st.h>
# include	<ui.h>
# include	<cm.h>
# include	"errc.h"
# include	<errw.h>
# include	<er.h>

/*
**   CRCRACK_CMD - crack the command line call to COPYREP.
**	The format of the command is:
**
**	COPYREP [-s] [-f] [-uuser] [-cacts] [-P] [-GgroupID] dbname 
**		filename {repname}
**
**		dbname	is the database name.
**		repname	 is a list of reports.
**		filename is the name of the file containing format cmds.
**		-s	stay silent.
**		-f	copy out as done by Filereport in RBF.
**		-uuser	alternate user name.
**		*** -c no longer supported ***
**		-cacts	action cache size.
**		-P - -P (password) flag, used to control access to the
**			DBMS as an additional level of security.
**		-GgroupID - -G flag, used to restrict read access to 
**			the database.
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
**		copyrep.
**
**	Side Effects:
**		Sets global variables En_database,
**		En_sfile, St_silent, Ptr_ren_top,
**		En_uflag, St_fflag, En_gidflag, 
**		En_passflag.
**
**	Error messages:
**		E_RW1320_, E_RW1321_, E_RW13FA_.
**
**	History:
**		9/12/82 (ps)	written.
**		8/15/83 (ps)	changed for Release 2.1.
**	       11/3/83 (nml)	End of list of reports to copy out
**				is now a RETURN instead of ctrl-Z.
**		12/1/84 (rlm)	ACT reduction - cache size option added
**		3/6/87 (KY)	change CH.h to CM.h for Double Bytes character 
**			        handling
**		18-mar-1987 (peter)
**			Add static forward declaration for routine c_numopt.
**              13-oct-89 (sylviap)
**                      Added error message that copyrep is no longer supporting
**                      the -c flag.  With 6.0 architecture, would need to
**                      put in cursors to support -c.  See rcrack.c in the
**			report directory for more information.
**		28-dec-89 (kenl)
**			Added handling of the +c (connection/with clause) flag.
**		3/21/90 (martym)
**			Added -G (group ID) and -P (password), and got rid of
**			all references to -p (prompt for any missing inform-
**			ation) which was obsolete.
**		12-aug-1992 (rdrane)
**			Use new constants in (s/r)_reset invocations.
**			Removed obsolete CFEVMSTAT references.
**			IIUIpassword() now declared in ui.h.  Explicitly
**			indicate that function returns nothing.  STtrmwhite()
**			returns resultant length, thus no need for separate
**			STlength().  Deleted unreferenced c_numopt() static
**			routine.  Converted r_error() err_num value to hex
**			to facilitate lookup in errw.msg file.
**		17-nov-1992 (rdrane)
**			Restore initialization of En_fspec.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* external declarations */

	extern	bool	St_fflag;	/* TRUE to make Filerep copy */


VOID
crcrack_cmd (argc,argv)
i4		argc;
char		*argv[];
{
	char		*c;			/* ptr to char buffer */
	char		*with_ptr = ERx("");
	i4		i;			/* used as counter */
	REN		*ren;
	bool		tmp_St_silent;

	/* start of routine */

	/*
	** Reset for call
	*/
	r_reset(RP_RESET_CALL,RP_RESET_LIST_END);
	s_reset(RP_RESET_PGM,RP_RESET_LIST_END);
	St_fflag = FALSE;		/* TRUE if Filereport copy */

	/* first pull out any flags. */
	i = St_called ? 0 : 1;
	for(; (i<argc); i++)
	{	/* next parameter */
		if (*(argv[i]) == '-')
		{	/* flag found. Set and clear out */
			switch(*(argv[i]+1))
			{
				case('c'):
				case('C'):
					/* -c flag is no longer supported */
                                        r_error(0x3FA,NONFATAL,NULL);
					break;

				case('f'):
				case('F'):
					St_fflag = TRUE;
					break;

				case('G'):  /* -G flag set */
					En_gidflag = argv[i];	
					break;

				case('P'):  /* -P (password) flag is set: */
					En_passflag = argv[i];	
					break;

				case('R'):
					/* ignore debugging flags */
					break;

				case('s'):
				case('S'):
					St_silent = TRUE;
					break;

				case('u'):
				case('U'):
					En_uflag = argv[i];
					break;

				case('V'):
					r_vpipe = (argv[i] + 2);
					break;

				case('X'):
					r_xpipe = (argv[i] + 2);
					break;

				default:
					r_error(0x320,FATAL,0);
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

				default:
					r_error(0x320,FATAL,0);
			}
			argv[i] = NULL;		/* clear out the flag */
		}
	}

# ifdef DGC_AOS
	if (*with_ptr == EOS)
	{
		with_ptr = STalloc(ERx("dgc_mode='reading'");
		IIUIswc_SetWithClause(with_ptr);
	}
# endif


	/*
	** If the '-P' flag has been set, prompt for the password:
	*/
	if (En_passflag != NULL)
	{
                /*
                ** If there is something wrong with the '-P' flag,
                ** such as the user has specified the password on the
                ** command line, IIUIpassword() will return NULL.
                ** In such cases bail out:
                */
		if ((En_passflag = IIUIpassword(En_passflag)) == NULL)
			r_error(0x320,FATAL,NULL);
	}


	/*
	** Now check to see that the database name was specified.
	** If not, ask for one.
	*/
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

	if  ((!St_called) && (STlength(En_database) <= 0))
	{	/* no database name. Ask for one: */
		c = r_prompt(ERget(S_RC0001_Database),TRUE);
		En_database = STalloc(c);
	}

	/* The next parameter is the file name */

	En_fspec = ERx("");
	for(; (i<argc); i++)
	{	/* check next parameter */
		if (argv[i]==NULL)
		{
			continue;
		}
		En_fspec = argv[i++];
		break;
	}

	if ((!St_called) && (STlength(En_fspec) <= 0))
	{	/* no file name. Ask for one: */
		c = r_prompt(ERget(S_RC0002_Filename),TRUE);
		En_fspec = STalloc(c);
	}

	/* The rest of the parameters will be report names. */
	for(; (i<argc); i++)
	{	/* check next parameter */
		if (argv[i]==NULL)
		{
			continue;
		}
		ren = (REN *) MEreqmem(0,sizeof(REN),TRUE,NULL);
		if (Ptr_ren_top == NULL)
		{
			Ptr_ren_top = ren;
		}
		else
		{
			Cact_ren->ren_below = ren;
		}
		Cact_ren = ren;
		ren->ren_report = STalloc(argv[i]);
	}

	if (Ptr_ren_top != NULL)
		return;

	/* No report names. Ask for one: */

	/* fix for bug 5640 */
	tmp_St_silent = St_silent;
	St_silent = FALSE;

	/*
	** Prompt for report name missing from command line
	** invocation.  Continue until an empty line is seen.
	*/
	while (TRUE)
	{
		c = r_prompt(ERget(S_RC0003_Report_name),FALSE);
		if (STtrmwhite(c) == 0)
		{
			St_silent = tmp_St_silent;
			break;
		}
		ren = (REN *)MEreqmem(0,sizeof(REN),TRUE,NULL);
		if (Ptr_ren_top == NULL)
		{
			Ptr_ren_top = ren;
		}
		else
		{
			Cact_ren->ren_below = ren;
		}
		Cact_ren = ren;
		ren->ren_report = STalloc(c);
	}

	if (Ptr_ren_top == NULL)
	{
		r_error(0x321,FATAL,0);
	}

	return;
}

