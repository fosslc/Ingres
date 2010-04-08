/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<pc.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ui.h>
# include	<ug.h>
# include	<st.h>
# include	<cv.h>
# include       "rbftype.h"
# include       "rbfglob.h"

/*
**   RFCRACK_CMD - crack the command line call to RBF.
**	The format of the command is:
**
**	RBF [flags] dbname rpt_object
**
**	where	flags	are flags to the command:
**			-e empty catalog -- don't load names into catalogs.
**			-m mode of report, if default.
**			-ln - maximum line length.
**			-s - don't print any system messages, incuding prompts.
**			-r - existing report specified.
**			-table - make default report out of named table	
**			-edit_restrict - called for restricted editing
**			-uuser - -u flag, used by DBA, can change user.
**			-Ztestfile
**			-Ddumpfile
**			-Ikeystrokefile
**			-GgroupID - -G flag, used to restrict read access to 
**				the database.
**			-P - -P (password) flag, used to control access to 
**				the DBMS as an additional level of security.
**		dbname	is the database name.
**		repname is the report name.
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
**		rFrbf.
**
**	Side Effects:
**		Sets global variables:
**		En_database, En_report, En_amax, En_lmax, St_silent,
**		En_gidflag, En_passflag.
**
**	Error messages:
**		708, 709.
**
**	History:
**		2/10/82 (ps)	written.
**		2/7/84	(gac)	refixed bug #966 while fixing bug #2059.
**		4/5/84	(gac)	added -Z test i/o filenames.
**		6/29/84 (gac)	fixed bug 2327 -- default line maximum is
**				width of terminal unless -l or -f is specified.
**		12/1/84 (rlm)	-c option for ACT reduction
**		10/1/85 (peter) Change prompt for db.
**		14-sep-1987 (rdesmond)
**			completely rewritten to use FEuta.
**		05-jan-1988 (rdesmond)
**			added 'table' parameter so RBF can be called from 
**			utexe with arguments specified by keyword.
**		12-jan-1988 (rdesmond)
**			changed 'report' parameter to include "r <reportname>"
**			as per user documentation.
**		25-aug-1988 (sylviap)
**			Changed RBF so it will not lowercase the database name 
**			as it parses.
**		12-jun-1989 (martym)
**			changed the 'report' parameter '-r' to detect 
**			unspecified report names as an error and not to
**			include "-r <reportname>" to match the documentation.
**      	9/22/89 (elein) UG changes ingresug change #90045
**			changed <rbftype.h> & <rbfglob.h> to
**			"rbftype.h" & "rbfglob.h"
**		12/27/89 (elein)
**			set  Rbf_from_ABF if -X flag and report name included
**		28-dec-89 (kenl)
**			Added handling of +c (connection/with clause) flag.
**		10-jan-90 (sylviap)
**			Took out any reference to numact.  RBF does not support
**			the -c flag.
**		1/12/90 (elein)
**			Added recognition of all -m modes  which are valid for
**			rbf when creating a report based on tables.  Added
**			tabular, indented and labels
**		3/21/90 (martym)
** 			Added -G and -P flags.
**		12-apr-90 (sylviap)
**			Added error to catch the situation when called from
**			3/4GL with: 
**			## call rbf (database = dbname, table = tblname,
**			##		report = rep_name)
**                      jupbug #20393.
**		26-dec-91 (pearl)  bug 41647
**			Set Rbf_from_ABF flag to TRUE only if the edit_restrict
**			flag is set.  Rbf_from_ABF should mean that 
**			Rbf is being called as part of an Abf edit, as opposed
**			to a 4gl CALL RBF statement.  Rbf_from_ABF used to be
**			set whenever the -X flag was present, and it couldn't
**			distinguish between the 2 cases.  The former case is
**			restrictive, and the latter case should act as though
**			it were called with the same arguments from the command
**			line.
**		21-jan-92 (pearl) bug 41791
**			Add support for table specification.
**			Substitute 'rpt_object' for 'table' parameter.  
**			Introduce new -table flag.  Create new booleans
**			GotRpt_object and GotTable.  Initialize St_style to
**			RS_NULL at the beginning of the routine.
**		9-oct-1992 (rdrane)
**			Use new constants in (s/r)_reset invocations.
**		4-nov-1992 (rdrane)
**			Include ui.h and delete the declaration of
**			IIUIpassword().  Don't unconditionally lowercase
**			a known table name (-table parameter).
**		19-feb-93 (leighb) DeskTop Porting Change:
**			Call PCexit(1)if no database name is given after
**			calling FEutaget() to prompt for it.
**		19-jun-95 (harpa06)
**			Kill the conversion to lowercase of a tablename that
**			has been provided on the command line until we know
**			later on what kind of conversion needs to be made.
**			During command line processing, there is no way
**			to determine whether or not the user is using a FIPS
**			installation until they are connected to the database.
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
rFcrack_cmd (argc,argv)
i4		argc;
char		*argv[];
{
    r_reset(RP_RESET_CALL,RP_RESET_LIST_END);
    s_reset(RP_RESET_PGM,RP_RESET_LIST_END);

    /* parse command line */
    {
	ARGRET	rarg;
	i4	pos;
	char	*ifile;		/* -I parameter */
	char	*dfile;		/* -D parameter */
	char	*zfile;		/* -Z parameter */
	bool    GotTable;	/* Got a table name */
	bool    GotReport;	/* Got a report name */
	bool    GotRpt_object;	/* Got a rpt_object name */
	char	*with_ptr = ERx("");    /* pointer to with clause */


	GotTable = FALSE;
	GotReport = FALSE;
	GotRpt_object = FALSE;
	St_style = RS_NULL;
	if (FEutaopen(argc, argv, ERx("rbf")) != OK)
	    PCexit(FAIL);

	if (FEutaget(ERx("emptycat"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    Rbf_noload = TRUE;

	if (FEutaget(ERx("mxline"), 0, FARG_FAIL, &rarg, &pos) == OK)
	{
	    r_g_set(rarg.dat.name);
	    if ((r_g_long(&En_lmax) < 0) || (En_lmax < 1))
	    {
		r_error(708, FATAL, rarg.dat.name, 0);
	    }
	    else
	    {	/* set default r.m. to this value */
		if (En_lmax < St_r_rm)
		    St_r_rm = En_lmax;
	    }
	    St_lspec = TRUE;
	}


	if (FEutaget(ERx("mode"), 0, FARG_FAIL, &rarg, &pos) == OK)
	{
	    if (STcompare(rarg.dat.name, ERget(F_RF0018_default)) == 0
	      || STcompare(rarg.dat.name, ERx("")) == 0)
		St_style = RS_DEFAULT;
	    else if (STcompare(rarg.dat.name, ERget(F_RF0019_column)) == 0)
		St_style = RS_COLUMN;
	    else if (STcompare(rarg.dat.name, ERget(F_RF005D_tabular)) == 0)
		St_style = RS_COLUMN;
	    else if (STcompare(rarg.dat.name, ERget(F_RF001A_wrap)) == 0)
		St_style = RS_WRAP;
	    else if (STcompare(rarg.dat.name, ERget(F_RF001B_block)) == 0)
		St_style = RS_BLOCK;
	    else if (STcompare(rarg.dat.name, ERget(F_RF005E_indented)) == 0)
		St_style = RS_INDENTED;
	    else if (STcompare(rarg.dat.name, ERget(F_RF005F_label)) == 0)
		St_style = RS_LABELS;
	    else
	        /* invalid mode specified */
		r_error(709, FATAL, rarg.dat.name, 0);
	}

	if (FEutaget(ERx("silent"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    St_silent = TRUE;

	if (FEutaget(ERx("user"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    En_uflag = rarg.dat.name;

	if (FEutaget(ERx("groupid"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    En_gidflag = rarg.dat.name;

	if (FEutaget(ERx("password"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    En_passflag = ERx("-P");

	if (FEutaget(ERx("equel"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    r_xpipe = rarg.dat.name;

	/* get database name */
	if  (FEutaget(ERx("database"), 0, FARG_PROMPT, &rarg, &pos) != OK)
		PCexit(1);
	
	/*
	** Do not lowercase database name.  User will need to enter the correct
	** case - upper, lower or mixed.
	*/
	STtrmwhite(rarg.dat.name);
	En_database = STalloc(rarg.dat.name);

	/* get rpt_object name, if specified */
	if (FEutaget(ERx("rpt_object"), 0, FARG_FAIL, &rarg, &pos) == OK)
	{
	    STtrmwhite(rarg.dat.name);
	    En_report = STalloc(rarg.dat.name);
	    GotRpt_object = TRUE;
	}

	/* Is this from ABF edit? */
	if (FEutaget(ERx("edit_restrict"), 0, FARG_FAIL, &rarg, &pos) == OK)
	{
		Rbf_from_ABF = TRUE;
		STcopy( En_report, Rbf_orname);
	}

	if (FEutaget(ERx("table"), 0, FARG_FAIL, &rarg, &pos) == OK)
	{
	    /* Just the -table specified w/no name */
	    if (STequal(rarg.dat.name, ERx("")))
	    {
	        if (!GotRpt_object)
	        {
			/*
			** No report name or table name specified:
			*/ 
	    		IIUGerr(E_RF004D_No_Name, UG_ERR_FATAL, 0);
	        }
	     }
	     /* The -table specified w/a table name */
	     else
	     {
		if (GotRpt_object)
		{
			/*
			** Both report name or table name specified:
			*/
			IIUGerr(E_RF0088_too_many, UG_ERR_FATAL, 0);
		}
		else
		{
			/*
			**  We're really doing this to get the string
			** cased and properly trimmed.  We'll still pass
			** it along unnormalized.  Maybe we don't have to do
			** anything but trim the string at this point ...
			*/

			STtrmwhite(rarg.dat.name);
			switch(IIUGdlm_ChkdlmBEobject(rarg.dat.name,NULL,
						      FALSE))
			{
			case UI_REG_ID:
				IIUGdbo_dlmBEobject(rarg.dat.name,FALSE);
				break;
			case UI_DELIM_ID:
				IIUGdbo_dlmBEobject(rarg.dat.name,TRUE);
				break;
			case UI_BOGUS_ID:
			default:
				/*
				** If it's bogus, just leave it alone and
				** let it fail further down the line ...
				*/
				break;
			}
			En_report = STalloc(rarg.dat.name);
		}
	     }
	     /*
	     **	Don't override if already set by -m flag; as long as it
	     ** is non-null, rFcatalog knows it's a table, not a report.
	     ** 
	     */
	     if (St_style == RS_NULL)
	     {
	     	 St_style = RS_DEFAULT;
	     }
	     GotTable = TRUE;
	}

	/* get report name, if specified */
	if (FEutaget(ERx("report"), 0, FARG_FAIL, &rarg, &pos) == OK)
	{
	    if (GotTable)
	    {
	    	IIUGerr(E_RF0088_too_many, UG_ERR_FATAL, 0);
	    }
	    /* Just the -r specified w/no name */
	    if (STequal(rarg.dat.name, ERx("")))
	    {
	        if (GotRpt_object)
	        {
	    		St_repspec = TRUE;
	        }
	        else
	        {
			/*
			** No report name or table name specified:
			*/ 
	    		IIUGerr(E_RF004D_No_Name, UG_ERR_FATAL, 0);
	        }
	     }
	     /* The -r specified w/a report name */
	     else
	     {
		if (GotRpt_object)
		{
			/*
			** Both report name or table name specified:
			*/
			IIUGerr(E_RF0088_too_many, UG_ERR_FATAL, 0);
		}
		else
		{
			St_repspec = TRUE;
			STtrmwhite(rarg.dat.name);
			CVlower(rarg.dat.name);
			En_report = STalloc(rarg.dat.name);
		}
	     }
	}

	/*
	** If the '-P' flag is specified, prompt for the password:
	*/
	if (En_passflag != NULL)
	{
		En_passflag = IIUIpassword(En_passflag);
	}


	/*
	** Internal testing options (-D, -I, -Z flags) for forms programs.
	** All front end programs which use the forms system should put
	** in the call to FEtstbegin/FEtstend with the options given
	** from the following flags.
	*/

	dfile = NULL;		/* No -I, -Z, -D files */
	ifile = NULL;
	zfile = NULL;

	if (FEutaget(ERx("dump"), 0, FARG_FAIL, &rarg, &pos) == OK)
	{	/* -D flag */
		dfile = rarg.dat.name;
	}

	if (FEutaget(ERx("keystroke"), 0, FARG_FAIL, &rarg, &pos) == OK)
	{	/* -I flag */
		ifile = rarg.dat.name;
	}

	if (FEutaget(ERx("test"), 0, FARG_FAIL, &rarg, &pos) == OK)
	{	/* -Z flag */
		zfile = rarg.dat.name;
	}

	/*
	** The following call takes the three flags for forms
	** testing and initializes the testing files, etc.
	** You should call this routine whether or not the
	** -I, -D and  -Z flags are specified, in case you
	** want to check on the status of testing through
	** calls to FEtstmake and FEtstrun.
	*/

	if (FEtstbegin(dfile, ifile, zfile) != OK)
	    IIUGerr(E_RF0048_Testing_can_not_begin, UG_ERR_FATAL, 0);

	if (FEutaget(ERx("connect"), 0, FARG_FAIL, &rarg, &pos) == OK)
	{
		with_ptr = STalloc(rarg.dat.name);
		IIUIswc_SetWithClause(with_ptr);
	}

	FEutaclose();

    }
    return;
}
