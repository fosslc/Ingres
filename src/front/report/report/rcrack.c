/*
** Copyright (c) 1981, 2009 Ingres Corporation
*/


# include	<compat.h>
# include	<cm.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include       <gl.h>
# include       <sl.h>
# include       <iicommon.h>
# include	<st.h>
# include	<fe.h>
# include	<er.h>
# include	<stype.h>
# include	<sglob.h>
# include	<errw.h>
# include	<nm.h>


/*
**   R_CRACK_CMD - crack the command line call to the report
**	formatter.  The format of the command is:
**
**	REPORT [flags] dbname repname [( {param = value} )]
**
**	where	flags	are flags to the command:
**			-b/+b no formfeeds/formfeeds by default.
**			-nofirstff
**				Suppress initial formfeed if formfeeds are
**				enabled.
**			-firstff
**				Do NOT suppress initial formfeed if formfeeds
**				are enabled.
**			-ffilename - specifies file to use for report.
**			-h  - if set, write header and footer for reports
**				even if no data is set.
**			-ln - maximum line length (default FO_DFLT).
**			-m mode of report, if default.
**			-nn - number of copies to be printed
**			-oprinter - printer to output the report to
**			-qn - maximum size of query (default MQ_DFLT).
**			-r - existing report (don't do default).
**			-s - don't print any system messages, incuding prompts.
**			-t/+t - truncate floating point columns to precision
**				if format is "F" or numeric template.
**			-uuser - -u flag, used by DBA, can change user.
**			-wn - maximum number of wrapped lines (default MW_DFLT).
**			-vn - length of page (default ...)
**			-X,-V pick up ingres pipe names for subprocess call
**			-5  - if set, integers are still treated as floats,
**				current_date default format is d" 3-feb-1901",
**				and +t is the default (compatible with 5.0)
**			-6  - if set, default reports return distinct rows
**			      (compatible with 6.0)
**			-ifilename  - read report specification from file
**				instead of getting report from database.
**			+e - batch mode, so don't prompt for any parameters  
**			+#:_= - copyright banner suppression.  This flag is NOT
**				documented to the users.  This is used 
**				INTERNALLY only.  This feature is motivated
**				by our contractual agreement with ASK.
**			-GgroupID - -G flag, used to restrict read access 
**				to the database.
**			-P - -P (password) flag, used to control access to the 
**				DBMS as an additional level of security.
**
**		repname is the report name.
**		dbname	is the database name.
**		param	is a parameter name.
**		value	is a parameter value.
**
**	(In the development version, optional trace flags are
**	also allowed.)	This routine pulls out the repname,
**	dbname, and parameters and stuffs them into the
**	proper places.
**
**      13-oct-89 (sylviap) RW is no longer supporting the -c flag.  6.0
**      architecture now requires to have the ATT structures (based on column
**      names) to build the t_commands from the r_commands.  In r_rep_load,
**      r_rco_set reads in 'n' (-cn) rcommands.  It then proceeds to build the
**      t_commands.  Now, the column names are necessary so the .QUERY is sent
**      to the DBMS (r_send_qry).  When RW tries to read in the next 'n'
**      r_commands, it gets 'in the middle of a retrieve loop' error.  A fix
**      would be to use cursors (prepare/describe) to the the column names
**      to build the ATT structure.
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
**		r_main.
**
**	Side Effects:
**		Sets global variables En_database,
**		En_report, En_fspec, 
**		En_qmax, En_lmax, En_wmax, St_silent,
**		St_repspec, St_ispec, En_gidflag, 
**		En_passflag.
**
**	Error messages:
**		0, 1, 2, 8, 9.
**
**	Trace Flags:
**		1.0, 1.2.
**
**	History:
**		6/15/81 (ps)	written.
**		10/8/81 (ps)	change dbname/repname order on command
**				line and allow flags anywhere.	Also
**				add the -s flag.
**		2/1/82 (ps)	update for version 1.3.
**		9/8/82 (jrc)	added -X flags.
**		11/5/82 (ps)	default St_prompt, change call.
**		10/28/83 (gac)	added -t/+t for truncating values.
**		12/8/83 (gac)	fixed bug 1765 -- max line default is 132.
**		2/22/84 (gac)	fixed bug 2118 -- +t before db name now does
**				not give error.
**		6/29/84 (gac)	fixed bug 2327 -- default line maximum is
**				width of terminal unless -l or -f is specified.
**		12/1/84 (rlm)	ACT reduction - added 'c' option for cache size
**				and CFEVMSTAT trace code
**		12/1/85 (prs)	Add -h flag.
**		12/14/87 (prs)	Change -a to -5 flag.
**		5/20/88 (gac)	Add -i flag for SUN report speedup.
**		6/05/89 (aem)	B6372: Could not pass parameter because
**				forgot how to recognize parens.
**		22-jun-89 (sylviap)
**			Added flags -n (number of copies to be printed) and -o
**			(direct output to printer)
**		10-jul-89 (sylviap)
**			Lowercases the report name when the -r flag is set.
**		01-sep-89 (sylviap)
**			Added -6 flag to run reports compatible w/6.*.  Default
**			is reports will return duplicate rows.  With -6 set, 
**			will run queries with DISTINCT.  
**              13-oct-89 (sylviap)
**                      Added error message that RW is no longer supporting
**                      the -c flag.  With 6.0 architecture, RW would need to
**                      put in cursors to support -c.  See above for more
**                      information.
**		11/1/89 (elein)
**			Added -d flag for continue on setup error.
**		28-dec-89 (kenl)
**			Added handling of the +c (connection/with clause) flag.
**		02-jan-90 (sylviap)
**			Added +e for batch mode.
**			Added +#:_= for copyright suppression.  This is an
**			  internal flag that is NOT documented.  Implemented
**			  to satisfy the ASK contract.
**		1/12/90 (elein)
**			Added tabular as valid report stylemode
**		06-feb-90 (sylviap)
**			Added check if -n flag is specified without the -o flag.
**			US #501
**		20-mar-90 (sylviap)
**			Initializes En_copies to 1 if -o (output) flag is used.
**              3/20/90 (elein) performance
**                      Change STcompare to STequal
**		3/21/90 (martym)
**			Added -G (group ID) and -P (password), and got rid of 
** 			-p (prompt for any missing information) which was 
**			obsolete.
**		17-may-90 (sylviap)
**			Added check for running with both the -m and -i flag
**			together. (#957)
**		5/8/90 (elein) 21553
**			Invalid flags cause errors which should be ignored.
**			We have to reset Err_count to its previous value in
**			order to ignore these errors and continue.  At this
**			point Err_count should always be 0, but we do save
**			and restore in case something changes in the future.
**		7-oct-1992 (rdrane)
**			Use new constants in (s/r)_reset invocations.
**			Converted r_error() err_num value to hex
**			to facilitate lookup in errw.msg file.
**		21-jun-1993 (rdrane)
**			Add processing for -nofirstff|-firstff flags.
**			
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      28-apr-2004 (srisu02)
**          Changes for addition of new flag "numeric_overflow=ignore|fail|warn"
**	20-Aug-2007 (kibro01) b118976
**	    Ensure that quoted parameters are accepted like normal parameters
**	14-Nov-2008 (gupsh01)
**	    Added support for II_PRINT_UTF8ALIGN environment variable
**	    and -ut8align option for report command. Both allow report
**	    to print the result on the output file with output aligned
**	    for terminals or printers capable of rendering UTF8 
**	    characters with a fixed width font.
**      05-feb-2009 (joea)
**          Rename CM_ischarsetUTF8 to CMischarset_utf8.
**	19-Aug-2009 (kschendel) 121804
**	    Need cm.h for proper CM declarations (gcc 4.3).
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

#ifdef CFEVMSTAT
	extern	i2 R_vm_trace;
#endif

static bool
these_are_parameters(
	char *arg)
{
	i4 len;
	if (arg == NULL || *arg == '\0')
		return (FALSE);
	len = STlength(arg);
	if (*arg == '"' && arg[len-1] == '"')
	{
		arg++;
		len-=2;
	}
	if (*arg == '{')
		return (TRUE);
	if (*arg == '(')
		return (TRUE);
	return (FALSE);
}

FUNC_EXTERN 	char 	*IIUIpassword();

VOID
r_crack_cmd (argc,argv)
i4		argc;
char		**argv;
{
	/* internal declarations */

#ifdef CFEVMSTAT
	char label [600];
	i2 ii;
#endif

	char		**targv;
	i4		targc;	   /* arg counter needed by CMS (scl) */
	char		*c;			/* ptr to char buffer */
	char		*with_ptr = ERx("");
	bool		gotdb = FALSE;
	bool		no_m_flag = TRUE;
	i2		save_err_count;
	bool		isUTF8 = FALSE;		
	char		*ptr;

	isUTF8 = CMischarset_utf8(); 

	/* start of routine */

#ifdef CFEVMSTAT
	R_vm_trace = 0;
	STcopy (argv[0],label);
	for (ii=1; ii < argc; ++ii)
	{
		STcat (label,ERx(" "));
		STcat (label,argv[ii]);
	}
	STcat (label,ERx(" : "));
	TMcvtsecs (TMsecs(),label + STlength(label));
#endif
	if (isUTF8)
	{
	  En_need_utf8_adj = TRUE;
	  En_utf8_adj_cnt = 0;
	}

	save_err_count = Err_count;
	/* set up global variables */
	r_reset(RP_RESET_CALL,RP_RESET_LIST_END);

	/* first pull out any flags. */

/*
**	Had to add 'targc' into the loop control, since CMS doesn't
**	tack a NULL pointer onto the end of the parmlist; it expects
**	you to use argc instead. (scl)
*/
	for(targv = argv, targv++ , targc=2;
	   *targv != NULL && targc <= argc;
	   targv++, targc++)
	{	/* next parameter */
		if (**targv == '-')
		{	/* flag found. Set and clear out */
if (STlength(*targv+1) > 17 && STbcompare(*targv+1, 17,ERx("numeric_overflow="),17,TRUE) == 0 )
{
  if (STcompare(ERx("fail"), *targv+18 ) == 0)
  {
    En_num_oflag = NUM_O_FAIL;
  }
  else if (STcompare(ERx("warn"), *targv+18) == 0)
  {
    En_num_oflag = NUM_O_WARN;
  }
  else if (STcompare(ERx("ignore"), *targv+18) == 0)
  {
    En_num_oflag = NUM_O_IGNORE;
  }
  else
  {
    SIprintf(ERget(E_RW1418_));
    En_num_oflag = NUM_O_FAIL;
  }
}
else
			switch(*(*targv+1))
			{
				case('5'):
					St_compatible = TRUE;
					break;
				case('6'):
					St_compatible60 = TRUE;
					break;

				case('b'):
				case('B'):
					St_ff_on = FALSE;
					St_ff_set = TRUE;
					break;

				case('c'):
				case('C'):
					/* the -c flag is no longer supported */
					r_error(0x3FA, NONFATAL, NULL);
					break;

				case('d'):
				case('D'):
					St_setup_continue = TRUE;
					break;
				case('f'):
				case('F'):
                                	if  (STbcompare(((*targv) + 1),0,
						RW_1STFF_FLAG,0,TRUE) == 0)
                                	{
						/*
						** We allow multiple
						** instances so long as
						** they don't flip-flop!
						*/
                                        	if  ((St_no1stff_set) &&
                                        	     (St_no1stff_on))
						{
	        					r_error(0x00,FATAL,
								RW_1STFF_FLAG,
								NULL);
							break;
						}
                                        	St_no1stff_on = FALSE;
                                        	St_no1stff_set = TRUE;
					}
					else
					{
						En_fspec = *targv+2;
					}
					break;

				case('G'):
					/* -G flag set */
					En_gidflag = *targv;
					break;

				case('h'):
				case('H'):
					St_hflag_on = TRUE;
					break;

				case('i'):
	    				if (!no_m_flag)
	    				{
	        				r_error(0x40B, FATAL, NULL);
						break;
	    				}
					St_ispec = TRUE;
					En_sfile = *targv+2;
					if (En_amax != ACH_DFLT)
					{
					    /* -c was specified; ignore it */
					    r_error(0x0E, NONFATAL, NULL);
					    En_amax = ACH_DFLT;
					}
					break;

				case('l'):
				case('L'):
					r_g_set(*targv+2);
					if ((r_g_long(&En_lmax) < 0) || (En_lmax<1))
					{
						r_error(0x08, NONFATAL, *targv, NULL);
						En_lmax = FO_DFLT;
					}
					else
					{	/* set default r.m. to this value */
						if (En_lmax < St_r_rm)
						{
							St_r_rm = En_lmax;
						}
					}
					St_lspec = TRUE;
					break;

				case('m'):
				case('M'):
	    				if (St_ispec)
	    				{
	        				r_error(0x40B, FATAL, NULL);
						break;
	    				}
					no_m_flag = FALSE;
					if (STequal(*targv+2,ERx("default"))
					  ||STequal(*targv+2,ERx("")) )
					{
						St_style = RS_DEFAULT;
						break;
					}
					if (STequal(*targv+2,ERx("tabular")) )
					{
						St_style = RS_COLUMN;
						break;
					}
					if (STequal(*targv+2,ERx("column")) )
					{
						St_style = RS_COLUMN;
						break;
					}
					if (STequal(*targv+2,ERx("wrap")) )
					{
						St_style = RS_WRAP;
						break;
					}
					if (STequal(*targv+2,ERx("block")) )
					{
						St_style = RS_BLOCK;
						break;
					}
					r_error(0x09, NONFATAL, *targv+2, NULL);
					St_style = RS_DEFAULT;
					break;

				case('n'):
				case('N'):
					if  (CVan((*targv+2),&En_copies) != OK)
					{
						if  (STbcompare(((*targv) + 1),
							0,RW_NO1STFF_FLAG,0,
							TRUE) == 0)
						{
							/*
							** We allow multiple
							** instances so long as
							** they don't flip-flop!
							*/
                                        		if  ((St_no1stff_set) &&
							     (!St_no1stff_on))
							{
	        						r_error(0x00,
									FATAL,
								RW_NO1STFF_FLAG,
									NULL);
								break;
							}
							St_no1stff_on = TRUE;
							St_no1stff_set = TRUE;
						}
						else if  ((STbcompare(((*targv) + 1),
                                                        0,RW_UTF8_NOALIGNFLAG,0,
                                                        TRUE) == 0) && isUTF8)
						{
					  		En_need_utf8_adj = FALSE;
						}
						else
						{
							r_error(0x00,
								FATAL,
								*targv,NULL);
						}
						break;
					}
					if  (En_copies < 1)
					{
						r_error(0x08,NONFATAL,*targv,
							NULL);
						En_copies = 1;
					}
					break;
				case('o'):
				case('O'):
					St_print = TRUE;
					/* 
					** set number of copies to default to
					** 1 if not already set
					*/
					if (En_copies == 0)
						En_copies = 1;
					if (!(STequal(*targv+2,ERx(""))) )
						En_printer = *targv+2;
					break;

				case('P'): /* -P (password) flag is set: */
					En_passflag = *targv;
					break;

				case('q'):
				case('Q'):
					r_g_set(*targv+2);
					if ((r_g_long(&En_qmax) < 0) || (En_qmax<1))
					{
						r_error(0x08, NONFATAL, *targv, NULL);
						En_qmax = MQ_DFLT;
					}
					break;

				case('r'):
					St_repspec = TRUE;
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
					St_truncate = FALSE;
					St_tflag_set = TRUE;
					break;

				case('u'):
				case('U'):
					/* -u flag set */
					  En_uflag = *targv;
					break;

				case('v'):
				case('V'):
					r_g_set(*targv+2);
					if ((r_g_long(&St_p_length) < 0) || (St_p_length < 0))
					{
						r_error(0x0D, NONFATAL, *targv, NULL);
						St_p_length = PL_DFLT;
					}
					else
					{
						St_pl_set = TRUE;
					}
					break;

				case('w'):
				case('W'):
					r_g_set(*targv+2);
					if ((r_g_long(&En_wmax) < 0) || (En_wmax<1))
					{
						r_error(0x08, NONFATAL, *targv, NULL);
						En_wmax = MW_DFLT;
					}
					break;

				case('X'):
					r_xpipe = (*targv);
					St_to_term = TRUE;
					break;

				default:
					r_error(0x00, FATAL, *targv, NULL);
			}
		}
		else if (**targv == '+')
		{
			switch(*(*targv+1))
			{
				case('b'):
				case('B'):
					St_ff_on = TRUE;
					St_ff_set = TRUE;
					break;

				case('c'):	/* connection parameters */
				case('C'):
					/* set up WITH clause for CONNECT */
					with_ptr = STalloc(*targv + 2);
					IIUIswc_SetWithClause(with_ptr);
					break;

				case('e'):
				case('E'):
					/* 
					** Set batch boolean.  Cannot prompt
					** for variables, so also set St_silent.
					*/
					St_batch = TRUE;
					St_silent = TRUE;
					break;

				case('t'):
				case('T'):
					St_truncate = TRUE;
					St_tflag_set = TRUE;
					break;

				case ('#'):
					/* 
					** Special internal flag used to 
					** suppress the copyright banner.
					*/
					if (STequal(*targv+2,ERx(":_=")))
						St_copyright = TRUE;
					else
						r_error(0x00, FATAL, *targv, NULL);
					break;
				default:
					r_error(0x00, FATAL, *targv, NULL);
			}
		}
#ifdef CFEVMSTAT
		else if (**targv == '@')
		{
			switch (*(*targv+1))
			{
			case 'f':
				R_vm_trace = 1;
				FEvminit (*targv+2,label);
				break;
			case 'p':
				CVan (*targv+2,&En_program);
				break;
			case 's':
				St_called = TRUE;
				break;
			default:
				SIprintf (ERx("debug option %c?\n"),*(*targv+1));
				break;
			}
		}
#endif
	}

# ifdef DGC_AOS
	if (*with_ptr == EOS)
	{
		with_ptr = STalloc(ERx("dgc_mode='reading'");
		IIUIswc_SetWithClause(with_ptr);
	}
# endif

	if (St_repspec)
	{
		St_style = RS_NULL;
	}

	if (isUTF8)
	{
	  NMgtAt(ERx("II_PRINT_UTF8ALIGN"),&ptr);
          if  ((ptr != NULL) && (*ptr != EOS))
          {
            CVlower(ptr);
            if  (STcompare(ptr,ERx("off")) == 0)
            {
		En_need_utf8_adj = FALSE;
            }
            else if  (STcompare(ptr,ERx("files")) == 0)
            {
		En_need_utf8_adj_frs = TRUE;
            }
          }

	  if (St_to_term && 
	      En_need_utf8_adj && 
	      !(En_need_utf8_adj_frs))
	  {
		En_need_utf8_adj = FALSE;
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
			r_error(0x01, FATAL, SavFlag, NULL);
	}


	/* 
	** Now check to see that the database name was specified if standalone.
	** If not, ask for one:
	*/

	if (St_called)
	{	/* if called, DB is open already */
		gotdb = TRUE;		/* use from first param */
	}
	else
	{
/*
**	Had to add 'targc' into the loop control, since CMS doesn't
**	tack a NULL pointer onto the end of the parmlist; it expects
**	you to use argc instead. (scl)
*/
		for(targv = argv, targv++ , targc=2;
		   *targv != NULL && targc <= argc;
		   targv++, targc++)
		{	/* check next parameter */
			if (**targv=='-' || **targv=='+')
			{
				continue;
			}
#ifdef CFEVMSTAT
			if (**targv == '@')
				continue;
#endif
			if (**targv=='(')
			{
				break;
			}

			En_database = *targv++;
			targc++; /* keep targv and targc in sync (scl) */
			break;
		}

		if (STlength(En_database) == 0)
		{	/* no database name. Ask for one: */
			c = r_prompt(ERget(FE_Database),TRUE);
			En_database = (char *) STalloc(c);
			STcopy(c, En_database);
		}
	}

	/* 
	** See if the report name specified.  If not, 
	** ask for it:
	*/

	if (gotdb)
	      { targv = argv; targc = 1; } /* keep targv and targc in sync (scl) */
/*
**	Had to add 'targc' into the loop control, since CMS doesn't
**	tack a NULL pointer onto the end of the parmlist; it expects
**	you to use argc instead. (scl)
*/
	for(;*targv != NULL && targc <= argc; targv++, targc++)
	{	/* check next parameter */
		if (**targv=='-' || **targv=='+')
		{
			continue;
		}
		/* B6372: added back in paren recognition + b118976 (kibro01) */
		if (these_are_parameters(*targv))
		{
			break;
		}
		En_report = *targv++;

		/*  Lower case report name if -r flag is set */
		if (St_repspec)
			CVlower(En_report);
		targc++;     /* keep targv and targc in sync (scl) */
		break;
	}

	if (STlength(En_report) > 0)
	{
	    if (St_ispec)
	    {
	        r_error(0x0F, FATAL, NULL);
	    }
	}
	else
	{	/* no report name. Ask for one: */
	    if (!St_ispec)
	    {
		c = r_prompt(ERget(S_RW000F_Report_or_Table),TRUE);
		En_report = (char *) STalloc(c);
		STcopy(c, En_report);
                /*  Lower case report name if -r flag is set */
		if (St_repspec)
			CVlower(En_report);
	    }
	}


	/* now pull off the parameter list (if specified) and
	** stuff it into linebuf for use by other routines  */

/*
**	Had to add 'targc' into the loop control, since CMS doesn't
**	tack a NULL pointer onto the end of the parmlist; it expects
**	you to use argc instead. (scl)
*/
	for(targv = argv, targv++ , targc=2;
	   *targv != NULL && targc <= argc;
	   targv++, targc++)
	{	/* found param list */
		/* B6372: added back in paren recognition + b118976 (kibro01) */
		if (these_are_parameters(*targv))
		{
#ifdef CMS
			/*
			**     The Whitesmiths C compiler under CMS
			**     terminates each "word" in the command
			**     line with a null; this defeats the design
			**     of r_crk_par, which expects to find a
			**     null only after the very last parm. As a
			**     fix we will re-construct the remaining
			**     portion of the command line into one
			**     null-terminated string, with each "word"
			**     separated by blanks.
			*/
			char   cline[512];
			char   **tmpv;
			i4	tmpc;
			cline[0] = NULL;
			for (tmpv = targv, tmpc = targc;
			     tmpc <= argc;
			     tmpv++, tmpc++)
			    {
			    STcat(cline,*tmpv);
			    STcat(cline,ERx(" "));
			    }
			if ((c = r_crk_par(cline)) != NULL)
#else
			if ((c = r_crk_par(*targv)) != NULL)
#endif
			{	/* error found in parameter list */
				r_error(0x01, FATAL, c, NULL);
			}
			break;
		}
	}
	if ((!St_print) && (En_copies != 0))
		/* Print error if specified -n flag without the -o flag */
		r_error(0x401, FATAL, NULL);
	Err_count = save_err_count;
	return;

}
