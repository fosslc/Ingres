/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<ex.h>
# include	<me.h>
# include	<st.h>
# include	<cv.h>
# include	<si.h>
# include	<er.h>
# include	<di.h>
# include	<lo.h>
# include	<gl.h>
# include	<sl.h>
# include	<pm.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<gn.h>
# include	<fedml.h>
# include       <adf.h>
# include       <afe.h>
# include	<uigdata.h>
# include	<ug.h>
# include	<ui.h>
# include	<xf.h>
# include	"erud.h"

/**
** Name:	udmain.sc - main procedure for UNLOADDB.
**
** Description:
**	This file defines:
**
**	main		main procedure for UNLOADDB utility
**
** History:
**	15-jul-87 (rdesmond) - 
**		modified for 6.0 (completely rewritten);
**	10-nov-87 (rdesmond) - 
**		fixed loop to fill array of user names.
**	17-nov-87 (rdesmond) - 
**		fixed problem for dba with no tables.
**	03-dec-87 (rdesmond) - 
**		defined CMDLINE for MVS.
** 	28-sep-88 (marian)	bug 3572
** 		Check the return status from FEingres to determine
** 		if the DBMS was connected to before continuing.  This
**		prevents later error messages stating that queries have
**		been issued outside of a DBMS session.
**	28-sep-88 (marian)
**		Change the order of the for loop so the dba scripts
**		are loaded last. This will allow the macro's in the iiud.scr
**		script to work correctly.  Force all the cp*.in scripts to
**		do a '\include <full_path_files_dir>iiud.scr' to check for
**		the existence of frontend objects before loading in new
**		frontend objects with the reload.ing script.
**	28-sep-1988 (marian)	bug 3393
**		Do a 'set autocommit on' at the start of the "copy in" script(s)
**		for 'sql' so the prompt when exiting the terminal when it is
**		not set will not be generated and the data is commited.  Without
**		this, nothing will be copied in.  Moved from xfwrscrp.
**	09-nov-88 (marian)	bug 3103
**		The +U flag is no longer needed to update extended catalogs in
**		the dbms, therefore, it can be removed from the reload.ing file.
**	25-jan-89 (marian)
**		Add new include line to reload.ing script.  This new file will
**		load the fe system catalogs only as $ingres.
**	21-feb-89 (marian)	bug 4743
**		Do a 'set autocommit on' at the start of the "copy out" 
**		scripts as well as the "copy in" scripts.
**	24-apr-89 (marian)
**		Added support for STAR.  Modified code to determine if connected
**		to STAR.  If so, only views and registrations are unloaded.  If
**		this is STAR, a direct conntect must be done to pass through
**		the QUEL statements.
**	24-apr-89 (marian)
**		The count of the number of users with objects in the database
**		is different for distributed and non-distributed.  SQL queries
**		are used to retrieve the count and user names since there
**		is no way to get the user names from multiple tables in a
**		single query in quel.
**	02-may-89 (marian)
**		Fixed a bug so the include of iiud.scr is not done for STAR.
**		STAR only unloads iiregistrations and iiviews and doesn't
**		unload objects.  This include needs to be removed because
**		macros are only allowed in quel.
**	27-may-89 (marian)
**		Call xfcapset() to check the iidbcapabilities.  Change 
**		is_distrib to a global with_distrib.
**	27-may-89 (marian)
**		Add GLOBALDEF for with_distrib, with_log_key, with_rules and
**		with_role.
**	02-oct-89 (marian)	bug 7334
**		The reload.in file will now have the DBA tables reloaded first,
**		but the system catalogs will still be reloaded last.  Save
**		the commandline to reload the cats in svcmdline to be used
**		to write out the last line in reload.ing.
**	04-oct-89 (marian)
**		Add i4  before integer constants in STpolycat and before the
**		main().
**	30-nov-89 (sandyd)
**		Removed unnecessary UNDEFS.
**      26-jan-90 (marian)
**              Fix the way unloaddb uses LOfroms().  Create locbuf[] to hold
**              the location that will eventually be passed to LOfroms().
**	16-apr-90 (marian)
**		Integrate changes from icl.  Changed call to NMloc as it
**		references a filename, which is not completely portable.
**		Replaced 'iiud.scr' by retrieval of file name from message
**		file erud.msg.  This follows the precedent of the script file 
**		in Terminal Monitor.
**	04-may-90 (billc)
**		 Major rewrite: convert to SQL, much optimization, etc.
**	31-may-90 (billc)
**		Fix us1012: quote the +U on VMS to prevent lowercasing.
**	01-jun-90 (billc)
**		Fix us1020: include iiud.scr ONLY in dba script.
**      29-aug-90 (pete)
**              Change to call FEningres, rather than FEingres and to pass
**              a NULL client name so no dictionary check will be done.
**	28-aug-1990 (Joe)
**	    Changed references to IIUIgdata to the UIDBDATA structure
**	    returned from IIUIdbdata().
**	02-oct-1990 (stevet)
**	    	Added IIUGinit call to initialize character set attribute
**		table.
**      14-may-1991 (billc)
**      	fix 37530, 37531, 37532.  All STAR problems - the DIRECT 
**      	CONNECT got lost in an earlier rewrite.
**      29-jul-1991 (billc)
**	        Fix bug 38881.  Older scripts could include newer macro scripts,
**		so must put new funtionality in new scripts.
**      19-aug-1991 (billc)
**	        Fix bug 39324.  Wasn't resetting to \quel, and the \i was 
**		leaving us in SQL.
**      24-mar-1992 (billc)
**              Check error status after DIRECT CONNECT.
**      30-feb-1992 (billc)
**		Major rewrite for FIPS in 6.5.
**    22-apr-92 (seg)
**            - EX handlers return STATUS, not EX.
**            - use same CMDLINE for OS/2, NT as PMFE.
**      27-jul-1992 (billc)
**              Rename from .qsc to .sc suffix.
**      16-nov-1992 (larrym)
**          Added CUFLIB to NEEDLIBS.  CUF is the new common utility facility.
**      10-jan-1994 (andyw)
**          If -lquel or -lsql used in the command line use IIUGerr()
**          instead of using IIUGmsg(). Correctly handles Ingres error
**          messages.
**	03/25/94 (dkh) - Added call to EXsetclient().
**	04-feb-95 (wonst02) Bug #63766
**	    For fips, need to upper-case $ingres in the unload & reload scripts
**	29-jun-95 (harpa06) Bug #69236
**	    changed the output message for UNLOADDB to inform the user that they
**	    should run CKPDB +j to enable journaling of system catalogs on 
**	    their database.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	16-nov-2000 (gupsh01)
**	    modified the xfcrscript function call for 
**	    xfcrscript, as the parameter list is
**	    changed due to new copydb flags. 
**	18-may-2001 (abbjo03)
**	    Add support for parallel index creation.
**      04-jan-2002 (stial01)
**          Added support for -with_views, -with_proc, -with_rules, -add_drop
**          -with_permits, and -with_integ
**      20-jun-2002 (stial01)
**          Unloaddb changes for 6.4 databases (b108090)
**	29-jan-03 (inkdo01)
**	    Added support of sequences.
**	30-Apr-2003 (hweho01)
**	    Bug fix 110082, Star Issue #12524976.  
**	    Unloading release 2.6 distributed database failed with error   
**          "E_US0845 Table 'iidd_tables' does not exist ...".  
**      31-Jul-2003 (hanal04) SIR 110651 INGDBA 236
**          Added new -journal option to allow fully journalled copy.in.
**	29-Apr-2004 (gupsh01)
**	    Added support for additional copydb flags.
**	27-Jan-2005 (guphs01)
**	    Add #!/bin/sh to enable using japanese and other language characters
**	    being used in the unload/reload script
**      03-May-2005 (komve01)
**          Added no_seq flag for copydb.
**      21-Oct-2005 (penga03)
**          Don't add #!/bin/sh on Windows.
**      04-jan-2008 (horda03) Bug 119681 (and 115431)
**          #!/bin/sh is only required on UNIX. On Windows and VMS this is
**          an unrecognised command. Modified penga03 change so that the
**          text is only added on Unix platforms.
**      17-Mar-2008 (hanal04) SIR 101117
**          Added -group_tab_idx to allows indexes to be created alongside
**          their parent tables.
**      02-oct-2008 (stial01)
**          Added support for -uninterrupted
**       3-Dec-2008 (hanal04) SIR 116958
**          Added -no_rep option to exclude all replicator objects.
**      28-jan-2009 (stial01)
**          Use DB_MAXNAME for database objects.
**       8-jun-2009 (maspa05) trac 386, SIR 122202
**          Added -nologging to put "set nologging" at the top of copy.in 
**          added dbaname parameter to xfcrscript so we can add "set session 
**          authorization <dbaname>"
**      18-Aug-2009 (shust01) Bug 122496
**          copy INGRES_NAME into local buffer prior to calling getingresname()
**          since we might be calling CVupper() and will crash trying to modify
**          a static string.
**/

# define REAL_USER_CASE		ERx("ii.$.createdb.real_user_case")
# define REAL_USER_CASE_UPPER	ERx("upper")

GLOBALREF bool	With_distrib;
GLOBALREF bool  With_20_catalogs;
GLOBALREF bool  Unload_byuser;

/* For linked list of usernames.  Avoids up-front allocation. */
typedef struct _uname_node
{
	char	*name;
	struct _uname_node	*next;
} UNAME;
static UNAME *_get_uname();
FUNC_EXTERN STATUS	FEhandler();
FUNC_EXTERN char	*IIUGdmlStr();

static char *scriptname(char *prefix);
void getingresname(char *name);

/*{
** Name:	udmain - main procedure for unloaddb
**
** Description:
**	Unloaddb - main program
**	-----------------------
**
** Input params:
**
** Output params:
**
** Returns:
**
** Exceptions:
**
** Side Effects:
**
** History:
/*
**	MKMFIN Hints
**
PROGRAM =	unloaddb

NEEDLIBS =	XFERDBLIB \
		GNLIB UFLIB RUNSYSLIB RUNTIMELIB FDLIB FTLIB FEDSLIB \
		UILIB LIBQSYSLIB LIBQLIB UGLIB FMTLIB AFELIB \
		LIBQGCALIB CUFLIB GCFLIB ADFLIB \
		COMPATLIB MALLOCLIB

UNDEFS =	II_copyright
*/

i4
main(argc, argv)
i4	argc;
char	**argv;
{
EXEC SQL BEGIN DECLARE SECTION;
    char    uname[DB_MAXNAME + 1];
    char    dbaname[DB_MAXNAME + 1];
EXEC SQL END DECLARE SECTION;
    char	*dbname = ERx("");
    char	*user = ERx("");
    char	*directory = ERx("");
    char	*sourcedir = NULL;
    char	*destdir = NULL;
    char	*password = ERx("");
    char	*groupid = ERx("");
    char	*progname;
    bool	portable = FALSE;
    UIDBDATA	*uidbdata;
    char	uncmdline[CMD_BUF_SIZE];
    char	recmdline[CMD_BUF_SIZE];
    EX_CONTEXT	context;
    i4		output_flags = 0;
    i4		output_flags2 = 0;
    char	*curr_user = NULL;
    i4		usercnt;
    UNAME	*namelist = NULL;
    UNAME	*np;

    /* Tell EX this is an ingres tool. */
    (void) EXsetclient(EX_INGRES_TOOL);

    MEadvise(ME_INGRES_ALLOC);

    /* Call IIUGinit to initialize character set attribute table */
    if ( IIUGinit() != OK)
	PCexit(FAIL);

    progname = ERget(F_UD0001_UNLOADDB);
    FEcopyright(progname, ERx("1987"));
    CreateParallelIndexes = FALSE;
    SetJournaling = FALSE;

    /*
    ** Get arguments from command line
    **
    ** This block of code retrieves the parameters from the command
    ** line.  The required parameters are retrieved first and the
    ** procedure returns if they are not.
    ** The optional parameters are then retrieved.
    */
    {
	ARGRET	rarg;
	i4	pos;

	if (FEutaopen(argc, argv, ERx("unloaddb")) != OK)
		PCexit(FAIL);

	/* database name is required */
	if (FEutaget(ERx("database"), 0, FARG_PROMPT, &rarg, &pos) != OK)
	    PCexit(FAIL);
	dbname = rarg.dat.name;

	/* Get optional arguments */

	if (FEutaget(ERx("portable"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    portable = TRUE;

	if (FEutaget(ERx("user"), 0, FARG_FAIL, &rarg, &pos) == OK) 
	    user = rarg.dat.name;

	if (FEutaget(ERx("directory"), 0, FARG_FAIL, &rarg, &pos)==OK)
	    directory = rarg.dat.name;

	if (FEutaget(ERx("source"), 0, FARG_FAIL, &rarg, &pos)==OK)
	    sourcedir = rarg.dat.name;

	if (FEutaget(ERx("dest"), 0, FARG_FAIL, &rarg, &pos)==OK)
	    destdir = rarg.dat.name;

	if (FEutaget(ERx("groupid"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    groupid = rarg.dat.name;

	if (FEutaget(ERx("parallel"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    CreateParallelIndexes = TRUE;

        if (FEutaget(ERx("group_tab_idx"), 0, FARG_FAIL, &rarg, &pos) == OK)
            GroupTabIdx = TRUE;

        if (FEutaget(ERx("nologging"), 0, FARG_FAIL, &rarg, &pos) == OK)
            NoLogging = TRUE;

        if (FEutaget(ERx("journal"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    SetJournaling = TRUE;

	/* We now ignore the language parameter. */
	if (FEutaget(ERx("language"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    IIUGerr(E_UD0015_Flag_Ignored, UG_ERR_FATAL, 0);

	if (FEutaget(ERx("password"), 0, FARG_FAIL, &rarg, &pos) == OK)
	{
	    char *IIUIpassword();

	    if ((password = IIUIpassword(ERx("-P"))) == NULL)
	    {
		FEutaerr(BADARG, 1, ERx(""));
		PCexit(FAIL);
	    }
	}

	if (FEutaget(ERx("viewonly"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    output_flags |= XF_VIEW_ONLY;

	if (FEutaget(ERx("procedureonly"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    output_flags |= XF_PROCEDURE_ONLY;

	if (FEutaget(ERx("rulesonly"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    output_flags |= XF_RULES_ONLY;

	if (FEutaget(ERx("sequencesonly"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    output_flags |= XF_SEQUENCE_ONLY;

	if (FEutaget(ERx("nosequences"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    output_flags2 |= XF_NO_SEQUENCES;

	if (FEutaget(ERx("dropincluded"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    output_flags |= XF_DROP_INCLUDE;

	if (FEutaget(ERx("integonly"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    output_flags |= XF_INTEG_ONLY;

	if (FEutaget(ERx("permitsonly"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    output_flags |= XF_PERMITS_ONLY;

        if (FEutaget(ERx("norep"), 0, FARG_FAIL, &rarg, &pos) == OK)
            output_flags2 |= XF_NOREP;

	if (FEutaget(ERx("uninterrupted"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    output_flags |= XF_UNINTURRUPTED;

	FEutaclose();
    }

    if (EXdeclare(FEhandler, &context) != OK)
    {
	EXdelete();
	PCexit(FAIL);
    }

    /*
    ** bug 3572
    ** Check the return status from FEningres to determine
    ** if the DBMS was connected to before continuing.
    */
    if (FEningres(NULL, (i4) 0, dbname, user, ERx(""), password,
				    groupid, (char *) NULL) != OK)
    {
	PCexit(FAIL);
    }

    uidbdata = IIUIdbdata();

    /* check that user is dba of database */
    if (!uidbdata->isdba)
	IIUGerr(E_UD0002_You_are_not_the_dba, UG_ERR_FATAL, 1, uidbdata->user);

    /* Check database capabilities, setting globals. */
    xfcapset();

    if (!With_distrib)
    {
	EXEC SQL SET LOCKMODE SESSION WHERE READLOCK = NOLOCK;
    }


    if (xfsetdirs(directory, sourcedir, destdir) != OK)
	PCexit(FAIL);

    /* open unload and reload shell scripts */
    if ((Xf_unload = xfopen(scriptname(ERx("unload")), TH_SCRIPT)) == NULL 
	|| (Xf_reload = xfopen(scriptname(ERx("reload")), TH_SCRIPT)) == NULL)
    {
	PCexit(FAIL);
    }

    /* With_20_catalogs can't be used if it is a distributed database. */      
    if ( (!With_20_catalogs) && (!With_distrib) )
	Unload_byuser = TRUE;

    /* get name of dba */

    EXEC SQL SELECT dbmsinfo ('dba')
	      INTO :dbaname;

    if (!Unload_byuser)
    {
	/* open unload/reload SQL script files */
	if ((Xf_in = xfopen(ERx("copy.in"), 0)) == NULL
	    || (Xf_out = xfopen(ERx("copy.out"), 0)) == NULL
	    || (Xf_both = xfduplex(Xf_in, Xf_out, 0)) == NULL)
	{
	    PCexit (FAIL);
	}
    }
    else
    {
	char    ingresname[DB_MAXNAME + 1];
	STcopy(INGRES_NAME, ingresname);


	/* get list of usernames */
	usercnt = 0;
	getingresname(ingresname);
	if (With_distrib)
	{
	    EXEC SQL SELECT table_owner
		    INTO :uname
		    FROM iidd_tables
		UNION
		    SELECT object_owner 
		    FROM iidd_registrations;
	    EXEC SQL BEGIN;
	    {
		if (STtrmwhite(uname) > 0 && !STequal(uname, ingresname)
		  && !STequal(uname, dbaname))
		{
		    namelist = _get_uname(namelist, uname);
		    usercnt++;
		}
	    }
	    EXEC SQL END;
	}
	else
	{
	    /* 
	    ** this check is required because iiregistrations is not
	    ** in all databases at this time.
	    */
	    if (FErelexists(ERx("iiregistrations")) == OK)
	    {
		EXEC SQL SELECT table_owner 
			INTO :uname
			FROM iitables
		    UNION
			SELECT procedure_owner 
			FROM iiprocedures
		    UNION
			SELECT object_owner 
			FROM iiregistrations;
		EXEC SQL BEGIN;
		{
		    if (STtrmwhite(uname) > 0 && !STequal(uname, ingresname)
		      && !STequal(uname, dbaname))
		    {
			namelist = _get_uname(namelist, uname);
			usercnt++;
		    }
		}
		EXEC SQL END;
	    }
	    else
	    {
		EXEC SQL SELECT table_owner 
			INTO :uname
			FROM iitables
		    UNION
			SELECT procedure_owner 
			FROM iiprocedures;
		EXEC SQL BEGIN;
		{
		    if (STtrmwhite(uname) > 0 && !STequal(uname, ingresname)
		      && !STequal(uname, dbaname))
		    {
			namelist = _get_uname(namelist, uname);
			usercnt++;
		    }
		}
		EXEC SQL END;
	    }
	}
	namelist = _get_uname(namelist, dbaname);
	usercnt++;
	/* check -- the DBA might be $ingres, so don't put it on the list twice. */
	if (!STequal(ingresname, dbaname))
	{
	    namelist = _get_uname(namelist, ingresname);
	    usercnt++;
	}
	np = namelist;
    }

    if (IIGNssStartSpace(0, ERx("cp"), DB_MAXNAME + 2, DB_MAXNAME + 2, 
		GN_LOWER, GNA_XUNDER, NULL) != OK)
	IIUGerr(E_UD0007_Cannot_start_name_gen, UG_ERR_FATAL, 0);

    for ( ; ; )
    {
	if (Unload_byuser)
	{
	    char    *filehead;
	    char    namein[DI_FILENAME_MAX];
	    char    nameout[DI_FILENAME_MAX];

	    /* open unload/reload SQL script files */
	    if (!np)
		break;
	    curr_user = np->name;
	    /* generate name for users script files */
	    if (IIGNgnGenName(0, np->name, &filehead) != OK)
		PCexit(FAIL);
	    STpolycat(2, filehead, ERx(".in"), namein);
	    STpolycat(2, filehead, ERx(".out"), nameout);
	    if ((Xf_in = xfopen(ERx(namein), 0)) == NULL
		|| (Xf_out = xfopen(ERx(nameout),0)) == NULL
		|| (Xf_both = xfduplex(Xf_in, Xf_out, 0)) == NULL)
	    {
		PCexit (FAIL);
	    }
	}

	/* 
	** Create scripts.  xfcrscript does all the real work of UNLOADDB/COPYDB. 
	*/
	xfcrscript(curr_user, progname, dbaname, portable, 
                     output_flags, output_flags2);

	/* generate command to run script file */
	/*
	** bug 3103
	** Remove +U flag from the command line call to the terminal monitor.
	**
	** I didn't understand the 3103 description, and we nee +U in order
	**	to destroy and recreate FE catalogs.  So, +U goes back in. 
	**	(billc 5/90).
	*/
	/*
	** Get the FIPS-related parameter from config.dat:
	**	if real_user_case is upper, upper-case the $ingres.
	*/
	{
	    char         ingresname[DB_MAXNAME + 1];
	    char	*unload_user;

	    if (Unload_byuser)
		unload_user = curr_user;
	    else
            {
	        STcopy(INGRES_NAME, ingresname);
		unload_user = ingresname;
            }		
	    getingresname(unload_user);
	    STprintf(recmdline, CMDLINE, unload_user, SUFLAG,
			dbname, xffilename(Xf_in));
	    STprintf(uncmdline, CMDLINE, unload_user,  
			ERx(""), dbname, xffilename(Xf_out));
	}

#ifdef UNIX 
	xfwrite(Xf_unload, "#!/bin/sh\n");
#endif
	/* write command to main shell-scripts */
	xfwrite(Xf_unload, uncmdline);
#ifdef UNIX
	xfwrite(Xf_reload, "#!/bin/sh\n");
#endif
	xfwrite(Xf_reload, recmdline);

	/* close files */
	xfclose(Xf_both);
	xfclose(Xf_in);
	xfclose(Xf_out);
	if (Unload_byuser)
	{
	    np = np->next;
	    if (!np)
		break;
	}
	else
	{
	    break;
	}
    }

  
    xfclose(Xf_unload);
    xfclose(Xf_reload);

    IIUGmsg(ERget(MSG_COMPLETE1), FALSE, 0);
    IIUGmsg(ERget(MSG_COMPLETE2), FALSE, 1,dbname);

    FEing_exit();

    EXdelete();

    PCexit(OK);
    /* NOTREACHED */
}

/*
** scriptname - takes a filename and attaches a system-dependent suffix
**		appropriate for an executable script.
**		The name does not persist since it's formatted into one
**		static global buffer.
**
**		This routine assumes a "well behaved" caller, and doesn't
**		check for length.
*/

static char Fname[ MAX_LOC + 1 ];

static char *
scriptname(char *prefix)
{
    STprintf(Fname, ERx("%s.%s"), prefix, SCRIPT_SUFFIX);
    return (Fname);
}

/*
**	getingresname - Get the owner name in the right form.
**
**	Get the FIPS-related parameter from config.dat:
**	if real_user_case is upper, upper-case the owner name.
*/
void
getingresname(char *name)
{
    char	*usercase;

    PMinit();
    if (PMload( (LOCATION *) NULL,  (PM_ERR_FUNC *) NULL) == OK)
    {
	if( PMsetDefault(1, PMhost() ) == OK 
	 && PMget(REAL_USER_CASE, &usercase) == OK
	 && STbcompare(usercase, 0, REAL_USER_CASE_UPPER, 0, TRUE) == 0)
	    CVupper(name);
    }
    PMfree();
}


static UNAME *_Ufree = NULL;
#define UFR_SIZE	16
static UNAME *
_get_uname(list, name)
UNAME	*list;
char	*name;
{
    UNAME *tmp;
    if (_Ufree == NULL)
    {
	i4  i;
	_Ufree = (UNAME *)MEreqmem((u_i4)0, (sizeof(UNAME) * UFR_SIZE),
			FALSE, (STATUS*)NULL);
	if (_Ufree == NULL)
	{
	    PCexit(FAIL);
	}
	/* set up the 'next' pointers. */
	for (i = 0; i < (UFR_SIZE - 1); i++)
	{
	    _Ufree[i].name = NULL;
	    _Ufree[i].next = &_Ufree[i+1];
	}
	_Ufree[UFR_SIZE - 1].next = NULL;
    }
    tmp = _Ufree;
    _Ufree = _Ufree->next;
    tmp->name = STalloc(name);
    tmp->next = list;
    return tmp;
}
