/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>
# include	<er.h>
# include	<me.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<ooclass.h>
# include	<fe.h>
# include	<ug.h>
# include	<ui.h>
# include	<uigdata.h>
# include	<erde.h>
# include	"doglob.h"

# define	DO_ALL_FLAG	ERx("all")
# define	DO_RPT_FLAG	ERx("report")
# define	DO_FORM_FLAG	ERx("form")
# define	DO_JD_FLAG	ERx("joindef")
# define	DO_GRAF_FLAG	ERx("graph")
# define	DO_APPL_FLAG	ERx("application")
# define	DO_QBF_FLAG	ERx("qbfname")
# define	DO_INCL_FLAG	ERx("include")
# define	DO_WILD_FLAG	ERx("wildcard")
# define	DO_QT_FLAG	ERx("silent")

/*
** DO_CRACK_CMD - crack the command line call to DELOBJ.
** The format of the command is:
**
** DELOBJ dbname	[-report|-form|-joindef|-graph|-application|-qbfname]
**			{objname} ...
**			[-wildcard] [-silent] [-include 'file']
**			[-uuser] ["-Ggroupid"] [-P]
**
**		- or -
**
** DELOBJ dbname	[-all]
**			[-silent]
**			[-uuser] ["-Ggroupid"] [-P]
**
**	where	dbname		is the database name.
**		objname		is an FE object name.
**		-all		indicates all FE objects of all types owned
**				by the effective invoker.
**		-report		indicates FE object names refer to Reports
**		-form		indicates FE object names refer to Forms
**		-joindef	indicates FE object names refer to JoinDefs
**		-graph		indicates FE object names refer to ViGraphs
**		-application	indicates FE object names refer to Applications
**		-qbfname	indicates FE object names refer to QBF Names
**		-wildcard	indicates expand wildcards.  Default is to not
**				expand.
**		-silent		stay silent.
**		-include 'file'	specifies an ASCII file 'file' containing
**				FE object names.
**		-uuser		alternate user name.
**		-Ggroupid	alternate group name.
**		-P		Password flag, used to control access to the
**				DBMS as an additional level of security.
**	The specified database is additionally opened.
**
** Parameters:
**		argc - argument count from command line.
**		argv - address of an array of addresses containing
**		       the starting points of parameter strings.
**
** Returns:
**	Nothing.
**
** Side Effects:
**	Sets global variables:
**		Dobj_wildcard,
**		Dobj_silent,
**		Dobj_database,
**		Dobj_uflag, Dobj_gidflag, Dobj_passflag,
**		Dobj_dfile.
**
** History:
**	12-jan-1993 (rdrane)
**		Written using report/rcrack.c as a model.
**	6-apr-1993 (rdrane)
**		Add checks for multiple object type specifications, and
**		simplify detection of parameters incompatible with '-all'.
**	31-aug-1993 (rdrane)
**		Correct bad parameter in STalloc() call when prompting for
**		object names.
**	28-jan-1995 (shero03)
**		Force the dbname to be entered before any options.
**		That will prevent the object from being misunderstood as the 
**		dbname.  (b66456)
**	04-Dec-1995 (mckba01)
**		Remove UG_ERR_FATAL from calls to IIUGerr, this causes
**		an VMS abort message (%SYSTEM-F-ABORT), to be displayed
**		on application exit, rplace with UG_ERR_ERROR. Also 
**		Add a call to PCexit(FAIL) after each modified call to 
**		IIUGerr, so that application still exits at this point.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-Nov-2010 (kschendel)
**	    Fix a CPP warning re extra token after #endif.
*/


VOID
do_crack_cmd(i4 argc, char **argv)
{
	i4	i;
	i4	argc_remain;
	i4	obj_params;
	bool	rn_params;
	bool	bogus_param;
	bool	all_param;
	OOID	del_obj_type;
	STATUS	c;
	char	*with_ptr;
	char	*user_flag;
	char	*group_flag;
	char	*pwd_flag;
	char	*tmp_ptr;
	FE_DEL	*fd;
	FE_DEL	*prev_fd;
	char	pr_buf[256];		/* Holds any current prompt */


	/*
	** Default to no wildcard expansion and no specified object type.
	** Indicate no names seen yet on command line, and no bogus
	** parameters.  Indicate no object type specifications (there
	** should never be more than one).
	** Track remaining command line parameters.
	*/
	Dobj_wildcard = FALSE;
	del_obj_type = OC_UNDEFINED;
	obj_params = 0;
	rn_params = FALSE;
	bogus_param = FALSE;
	all_param = FALSE;
	argc_remain = argc - 1;	/* Bypass argv[0] - the program name!	*/
	Dobj_database = NULL;   /* assume no database name b66456       */

	i = 1;			/* First pull out any flags	*/
	while (i < argc)
	{
		if  (*(argv[i]) == '-')	/* Flag found. Set and clear out */
		{
			switch(*(argv[i] + 1))
			{
			case('r'):
			case('R'):
				CVlower((argv[i] + 1));
				if  (STcompare((argv[i] + 1),DO_RPT_FLAG) == 0)
				{
					del_obj_type = OC_REPORT;
					Dobj_pr_str = ERget(F_DE0008_Rep);
					obj_params++;
				}
				else
				{
					bogus_param = TRUE;
				}
				break;
			case('f'):
			case('F'):
				CVlower((argv[i] + 1));
				if  (STcompare((argv[i] + 1),DO_FORM_FLAG) == 0)
				{
					del_obj_type = OC_FORM;
					Dobj_pr_str = ERget(F_DE0004_Form);
					obj_params++;
				}
				else
				{
					bogus_param = TRUE;
				}
				break;
			case('j'):
			case('J'):
				CVlower((argv[i] + 1));
				if  (STcompare((argv[i] + 1),DO_JD_FLAG) == 0)
				{
					del_obj_type = OC_JOINDEF;
					Dobj_pr_str = ERget(F_DE0006_Jdef);
					obj_params++;
				}
				else
				{
					bogus_param = TRUE;
				}
				break;
			case('g'):
			case('G'):
				tmp_ptr = STalloc(argv[i]);
				CVlower((tmp_ptr + 1));
				if  (STcompare((tmp_ptr + 1),DO_GRAF_FLAG) == 0)
				{
					del_obj_type = OC_GRAPH;
					Dobj_pr_str = ERget(F_DE0005_Graph);
					obj_params++;
				}
				else if (*(argv[i] + 1) == 'G')
				{
					Dobj_gidflag = argv[i];	
				}
				else
				{
					bogus_param = TRUE;
				}
				MEfree((PTR)tmp_ptr);
				break;
			case('a'):
			case('A'):
				CVlower((argv[i] + 1));
				if  (STcompare((argv[i] + 1),DO_APPL_FLAG) == 0)
				{
					del_obj_type = OC_APPL;
					Dobj_pr_str = ERget(F_DE0003_App);
					obj_params++;
				}
				else if  (STcompare((argv[i] + 1),
						    DO_ALL_FLAG) == 0)
				{
					del_obj_type = OC_OBJECT;
					/*
					** No prompt string for -all
					*/
					Dobj_pr_str = ERget(F_DE0009_Dobj_O);
					all_param = TRUE;
				}
				else
				{
					bogus_param = TRUE;
				}
				break;
			case('q'):
			case('Q'):
				CVlower((argv[i] + 1));
				if  (STcompare((argv[i] + 1),DO_QBF_FLAG) == 0)
				{
					del_obj_type = OC_QBFNAME;
					Dobj_pr_str = ERget(F_DE0007_Qbfnm);
					obj_params++;
				}
				else
				{
					bogus_param = TRUE;
				}
				break;
			case('i'):
			case('I'):
				CVlower((argv[i] + 1));
				if  (STcompare((argv[i] + 1),DO_INCL_FLAG) == 0)
				{
					/*
					** Clear out the flag so we don't see
					** it when we look for DB and FE object
					** names.
					*/
					argv[i] = NULL;
					i++;
					if  (i >= argc)
					{
						IIUGerr(E_DE000B_Dobj_no_file,
							UG_ERR_ERROR,0);
						FEexits(ERx(""));
						PCexit(FAIL);
						break;
					}
					Dobj_dfile = argv[i];
				}
				else
				{
					bogus_param = TRUE;
				}
				break;
			case('s'):
			case('S'):
				CVlower((argv[i] + 1));
				if  (STcompare((argv[i] + 1),DO_QT_FLAG) == 0)
				{
					Dobj_silent = TRUE;
				}
				else
				{
					bogus_param = TRUE;
				}
				break;
			case('u'):
			case('U'):
				Dobj_uflag = argv[i];
				break;
			case('w'):
			case('W'):
				CVlower((argv[i] + 1));
				if  (STcompare((argv[i] + 1),DO_WILD_FLAG) == 0)
				{
					Dobj_wildcard = TRUE;
				}
				else
				{
					bogus_param = TRUE;
				}
				break;
			case('P'):	/* -P (password) flag set	*/
				Dobj_passflag = argv[i];	
				break;
			default:
				bogus_param = TRUE;
				break;
			}
			if  (bogus_param)
			{
				IIUGerr(E_DE000A_Dobj_bad_flag,
					UG_ERR_ERROR,2,argv[i],
					ERget(E_DE0014_Dobj_syntax));
				FEexits(ERx(""));
				PCexit(FAIL);
			}
			/*
			** Clear out the flag so we don't see it when
			** we look for DB and FE object names.
			*/
			argv[i] = NULL;
			argc_remain--;
		}
		i++;
	}


# ifdef DGC_AOS
	with_ptr = STalloc(ERx("dgc_mode='reading'");
	IIUIswc_SetWithClause(with_ptr);
# else
	with_ptr = ERx("");
# endif

	/*
	** If -all was specified, then we can't have -wildcard, -include,
	** object type specifications, or objects names as well.  argc_remain
	** should at most reflect a database name.
	*/
	if  ((all_param) &&
	     ((Dobj_wildcard) || (Dobj_dfile != NULL) ||
	      (obj_params > 0) || (argc_remain > 1)))
	{
		IIUGerr(E_DE0012_Dobj_badall,UG_ERR_ERROR,1,
			ERget(E_DE0014_Dobj_syntax));
		FEexits(ERx(""));
		PCexit(FAIL);
	}

	/*
	** If multiple object types were specified, then abort!
	*/
	if  (obj_params > 1)
	{
		IIUGerr(E_DE0015_Dobj_multype,UG_ERR_ERROR,1,
			ERget(E_DE0014_Dobj_syntax));
		FEexits(ERx(""));
		PCexit(FAIL);
	}
	/*
	** If no object type was specified, then abort!
	*/
	if  (del_obj_type == OC_UNDEFINED)
	{
		IIUGerr(E_DE0013_Dobj_notype,UG_ERR_ERROR,1,
			ERget(E_DE0014_Dobj_syntax));
		FEexits(ERx(""));
		PCexit(FAIL);
	}

	/*
	** Check to see that the database name was specified.
	** If so, it will be the first non-flag parameter.
	** If not, ask for one:
	*/
	i = 1;
	while (i < argc)
	{
		if  (argv[i] == NULL)
		{
		/*
		** If we reached a used option,
		**   then the database name is missing.  b66456
		*/
			break;
			/* i++; */
			/* continue; */
		}

		/*
		** If we reached an option then, no dbname  b66456
		*/
		if (*argv[i] == '-')
		  break;

		Dobj_database = argv[i++];
		break;
	}

	if  (Dobj_database == NULL)
	{
		c = FEprompt(ERget(FE_Database),TRUE,(sizeof(pr_buf) - 1),
			     &pr_buf[0]);
		Dobj_database = STalloc(&pr_buf[0]);
	}

	/*
	** If the '-P' flag has been set, prompt for the password
	*/
	if  (Dobj_passflag != NULL)
	{
		/*
		** If there is something wrong with the '-P' flag, such as the
		** user has specified the password on the command line,
		** IIUIpassword() will return NULL.  In such cases bail out.
		*/
		if  ((Dobj_passflag = IIUIpassword(Dobj_passflag)) == NULL)
		{
			IIUGerr(E_DE000A_Dobj_bad_flag,UG_ERR_ERROR,2,
				ERx("-P"),ERget(E_DE0014_Dobj_syntax));
			FEexits(ERx(""));
			PCexit(FAIL);
		}
	}

	/*
	** Open the database, will abort on FAIL.  We do it here because we
	** need to establish the invoking user name before we process the FE
	** object names.  We use "user" since owners of FE objects are always
	** stored in the FE catalogs as lower case.  This will have to change
	** to suser when we fully support authorization identifiers as
	** delimited identifiers, which can be other than lower case
	** (specifically FIPS which is UI_MIXED_CASE).
	*/
	user_flag = ERx("");
	group_flag = ERx("");
	pwd_flag = ERx("");
	if  (Dobj_uflag != NULL)
	{
		user_flag = Dobj_uflag;
	}
	if  (Dobj_gidflag != NULL)
	{
		group_flag = Dobj_gidflag;
	}
	if  (Dobj_passflag != NULL)
	{
		pwd_flag = Dobj_passflag;
	}
	if  (FEingres(Dobj_database,user_flag,group_flag,pwd_flag,NULL) != OK)
	{
		IIUGerr(E_DE0004_NoOpen,UG_ERR_ERROR,0);
		FEexits(ERx(""));
		PCexit(FAIL);
	}

	/*
	** If -all was specified, force a wildcard expansion of "%".
	** do_expand_name() will recognize del_obj_type == OC_OBJECT
	** as being special.
	*/
	if  (del_obj_type == OC_OBJECT)
	{
		Dobj_wildcard = TRUE;
		fd = (FE_DEL *)MEreqmem(0,sizeof(FE_DEL),TRUE,NULL);
		Ptr_fd_top = fd;
		prev_fd = (FE_DEL *)NULL;
		Cact_fd = fd;
		Cact_fd->fd_name_info = (FE_RSLV_NAME *)MEreqmem(0,
						sizeof(FE_RSLV_NAME),TRUE,NULL);
		Cact_fd->fd_name_info->name = STalloc(ERx("%"));
		Cact_fd->fd_below = (FE_DEL *)NULL;
		Cact_fd = do_expand_name(Cact_fd,&prev_fd,del_obj_type);
		return;
	}

	/*
	** The rest of the parameters will be FE object names.
	*/
	while (i < argc)
	{
		if  (argv[i] == NULL)
		{
			i++;
			continue;
		}
		rn_params = TRUE;
		fd = (FE_DEL *)MEreqmem(0,sizeof(FE_DEL),TRUE,NULL);
		if  (Ptr_fd_top == (FE_DEL *)NULL)
		{
			Ptr_fd_top = fd;
			prev_fd = (FE_DEL *)NULL;
		}
		else
		{
			Cact_fd->fd_below = fd;
			prev_fd = Cact_fd;
		}
		Cact_fd = fd;
		Cact_fd->fd_name_info = (FE_RSLV_NAME *)MEreqmem(0,
						sizeof(FE_RSLV_NAME),TRUE,NULL);
		Cact_fd->fd_name_info->name = STalloc(argv[i]);
		Cact_fd->fd_below = (FE_DEL *)NULL;
		Cact_fd = do_expand_name(Cact_fd,&prev_fd,del_obj_type);
		/*
		** Returns ptr to current end of FE_DEL chain, which may be
		** NULL if (still) 1st and no match.  If no match, the FE_DEL
		** and its name will have been de-allocated, and prev_fd and
		** Ptr_fd_top will have been be altered as required.
		*/
		i++;
	}

	if  (Dobj_dfile != NULL)
	{
		do_ifile_parse(Dobj_dfile,del_obj_type);
	}	

	/*
	** If we found anything, then we're all done
	*/
	if  (Ptr_fd_top != (FE_DEL *)NULL)
	{
		return;
	}

	/*
	** If we had FE object names either on the command line or in a
	** file but they all failed expansion/resolution, then don't prompt
	** for any names!
	*/
	if  ((rn_params) || (Dobj_dfile != NULL))
	{
		IIUGerr(E_DE000C_Dobj_no_input,UG_ERR_ERROR,1,
			Dobj_pr_str);
		FEexits(ERx(""));
		PCexit(FAIL);
	}

	/*
	** Prompt for no FE object name - either on
	** the command line or in a -list file.  Note that
	** we ignore any Dobj_silent in this instance.
	*/
	while (TRUE)
	{
		c = FEprompt(Dobj_pr_str,FALSE,(sizeof(pr_buf) - 1),&pr_buf[0]);
		if  ((c != OK) || (STtrmwhite(&pr_buf[0]) == 0))
		{
			/*
			** When NULL string entered (or error), all done
			*/
			break;
		}
		fd = (FE_DEL *)MEreqmem(0,sizeof(FE_DEL),TRUE,NULL);
		if  (Ptr_fd_top == (FE_DEL *)NULL)
		{
			Ptr_fd_top = fd;
			prev_fd = (FE_DEL *)NULL;
		}
		else
		{
			Cact_fd->fd_below = fd;
			prev_fd = Cact_fd;
		}
		Cact_fd = fd;
		Cact_fd->fd_name_info = (FE_RSLV_NAME *)MEreqmem(0,
						sizeof(FE_RSLV_NAME),TRUE,NULL);
		Cact_fd->fd_name_info->name = STalloc(&pr_buf[0]);
		Cact_fd->fd_below = (FE_DEL *)NULL;
		Cact_fd = do_expand_name(Cact_fd,&prev_fd,del_obj_type);
	}

	if  (Ptr_fd_top == (FE_DEL *)NULL)
	{
		IIUGerr(E_DE000C_Dobj_no_input,UG_ERR_ERROR,1,Dobj_pr_str);
		FEexits(ERx(""));
		PCexit(FAIL);
	}

	return;
}
