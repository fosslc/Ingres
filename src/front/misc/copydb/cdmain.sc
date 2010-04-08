/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<cv.h>
# include	<ex.h>
# include	<me.h>
# include	<st.h>
# include	<si.h>
# include	<lo.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include 	<er.h>
# include	<fe.h>
# include	<fedml.h>
# include	<uigdata.h>
# include	<ug.h>
# include	<ui.h>
# include       <adf.h>
# include       <afe.h>
# include	<xf.h>
# include 	"ercd.h"

/**
** Name:    cdmain.sc - main procedure for COPYDB.
**
** Description:
**	This file defines:
**
**	main		main procedure for COPYDB utility
**
** History:
**	15-jul-87 (rdesmond) - modified for 6.0 (completely rewritten);
**	03/16/88 (dkh) - Added EXdelete call to conform to coding spec.
** 	28-sep-88 (marian)	bug 3594
** 		Check the return status from FEingres to determine
** 		if the DBMS was connected to before continuing.  This
**		prevents later error messages stating that queries have
**		been issued outside of a DBMS session.
**	28-sep-1988 (marian)	bug 3393
**		Do a 'set autocommit on' at the start of the "copy in" script(s)
**		for 'sql' so the prompt when exiting the terminal when it is
**		not set will not be generated and the data is commited.  Without
**		this, nothing will be copied in.  Moved from xfwrscrp.c.
**	14-nov-88 (marian)	bug 3969
**		Changed = to == in if statement.
**	21-feb-89 (marian)	bug 4743
**		Do a 'set autocommit on' at the start of the "copy out" 
**		scripts as well as the "copy in" scripts.
**	21-apr-89 (marian)
**		ADD support for STAR.  Check to see if distributed and perform
**		the unload of registered objects and views.  Calls xfchkcap()
**		to do a 'direct connect' to send the QUEL statements through
**		and to check to see if this is an INGRES dbms.  If this is
**		star, a direct disconnect will be done at the end.
**	27-may-89 (marian)
**		Call xfcapset() to determine the iidbcapabilities.  Changed
**		is_distrib to with_distrib.  Make with_distrib a global.
**	27-may-89 (marian)
**		Add GLOBALDEF for with_log_key, with_rules, with_role.
**	04-oct-89 (marian)
**		Add i4  for PC group.
**	30-nov-89 (sandyd)
**		Removed unnecessary UNDEFS.
**	26-jan-90 (marian)
**		Fix the way unloaddb uses LOfroms().  Create locbuf[] to hold
**		the location that will eventually be passed to LOfroms().
**      05-mar-1990 (mgw)
**              Changed #include <cd.h> to #include "cd.h" since this is
**              a local file and some platforms need to make the destinction.
**	04-may-90 (billc)
**		Major rewrite.  Convert to SQL.  Other optimization and cleanup.
**      29-aug-90 (pete)
**              Change to call FEningres, rather than FEingres and to pass
**              a NULL client name so no dictionary check will be done.
**	28-aug-1990 (Joe)
**	    Changed references to IIUIgdata to the UIDBDATA structure
**	    returned from IIUIdbdata().
**	02-oct-1990 (stevet)
**		Added IIUGinit call to initialize character set attribute 
**              table.
**	14-may-1991 (billc)
**		fix 37530, 37531, 37532.  All STAR problems - the DIRECT 
**		CONNECT got lost in an earlier rewrite.
**      24-mar-1992 (billc)
**              Check error status after DIRECT CONNECT.
**	30-feb-1992 (billc)
**		Major rewrite to support FIPS in 6.5.
**      27-jul-1992 (billc)
**              Rename from .qsc to .sc suffix.
**      16-nov-1992 (larrym)
**          Added CUFLIB to NEEDLIBS.  CUF is the new common utility facility.
**      10-jan-1994 (andyw)
**          If -lquel or -lsql used in the command line use IIUGerr()
**          instead of using IIUGmsg(). Correctly handles Ingres error
**          messages.
**	03/25/94 (dkh) - Added call to EXsetclient().
**	28-apr-94 (robf)
**          Add support for -labels
**      05-jan-1995 (chech02)  bug#64214
**          Pass IIUIdbdata()->suser as a username to xfcrscript().
**	01-jun-1995 (harpa06)
**	    Allowed the use of delimited identifiers be relocating the fix for
**	    bug #3594
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	16-nov-2000 (gupsh01)
**	    Added new flags for the copydb utility in order to create only the 
**	    individual statements, like createonly, modifyonly etc.
**	18-may-2001 (abbjo03)
**	    Add support for parallel index creation.
**	15-jun-2001 (gupsh01)
**	    Added support for -no_persist flag, this will not write any 
**	    create index statements to the copy.in file for indexes which 
**	    are created with persistence. This is needed for usermod. 
**	    (Bug 104883).  
**	29-Oct-2001 (hanch04)
**	    Call IIUGdlm_ChkdlmBEobject to check the names of the objects
**	    that have been passed in. 
**	01-nov-2001 (gupsh01)
**	    Added new flag -param_file to allow input of parameters through
**	    a parameter file. 
**	25-jun-2002 (gupsh01)
**	    Added -no_repmod option which will exclude modify of any 
**	    replicator catalogs.
**	29-jan-03 (inkdo01)
**	    Added -sequencesonly for sequence support.
**	14-mar-2003 (devjo01) b109764
**	    Add XF_PRINT_TOTAL to 'outputflags' if '-add_drop' specified,
**	    since this implicitly enables processing of tables.
**	07-apr-2003 (devjo01) b110000
**	    -add_drop cleanup Correct prev 109764 fix plus others tweaks).
**      29-Jul-2003 (hanal04) Bug 76483 INGSRV 2432
**          Dynamically allocate nbuf to ensure we do not overrun the
**          buffer when storing command line parameters that may be
**          longer than expected.
**      31-Jul-2003 (hanal04) SIR 110651 INGDBA 236
**          Added new -journal option to allow fully journalled copy.in.
**	23-Apr-2004 (gupsh01)
**	    Added support for additional flags for copydb.
**	16-feb-2005 (thaju02)
**	    Added concurrent_updates for usermod.
**	03-May-2005 (komve01)
**	    Added no_seq(no sequences) option for copydb.
**	31-Oct-2006 (kiria01) b116944
**	    Added processing for XF_NODEPENDENCY_CHECK
**      17-Mar-2008 (hanal04) SIR 101117
**          Added -group_tab_idx to allows indexes to be created alongside
**          their parent tables.
**       3-Dec-2008 (hanal04) SIR 116958
**          Added -no_rep option to exclude all replicator objects.
**      28-jan-2009 (stial01)
**          Use DB_MAXNAME for database objects.
**       8-jun-2009 (maspa05) trac 386, SIR 122202
**          Added -nologging to put "set nologging" at the top of copy.in 
**          checks that the user is the dba of the database since one needs
**          to be the dba to run "set nologging". 
**          Added E_CD0027_Nologging_NotDBA error for this case
**      30-Sep-2009 (hanal04) Bug 122647
**          Add E_CD0028_Completed message to COPYDB output to remind
**          user to copy the data after runnig copydb.
**       2-Oct-2009 (hanal04) Bug 122647
**          Add -no_warn flag used bu usermod to skip the reminder to run
**          the copy.out.
**/

GLOBALREF bool With_distrib;

/*{
** Name:	cdmain - main procedure for copydb
**
** Description:
**	Copydb - main program
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
PROGRAM =	copydb

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
    char	*dbname = ERx("");
    char	*user = ERx("");
    char	*directory = ERx("");
    char	*sourcedir = NULL;
    char	*destdir = NULL;
    char        *password = ERx("");
    char        *groupid = ERx("");
    char	*progname;
    bool	portable = FALSE;
    i4		numobj;
    EX_CONTEXT	context;
    EX		FEhandler();
    char	*filenamein = ERx("copy.in");
    char	*filenameout = ERx("copy.out");
    char	*param_file = ERx("");
    i4 		output_flags = 0;
    i4 		output_flags2 = 0; /* For additional copydb flags */
    char        *nbuf = NULL;
    i4          size_of_nbuf = 0;
    i4		arg_len;
    STATUS	me_status;
    UIDBDATA    *uidbdata;
    bool	Warning = TRUE;

    /* Tell EX this is an ingres tool. */
    (void) EXsetclient(EX_INGRES_TOOL);

    MEadvise(ME_INGRES_ALLOC);

    /* Call IIUGinit to initialize character set attribute table */
    if ( IIUGinit() != OK)
	PCexit(FAIL);

    progname = ERget(F_CD0001_COPYDB);
    FEcopyright(progname, ERx("1987"));
    CreateParallelIndexes = FALSE;
    concurrent_updates = FALSE;
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

	if (FEutaopen(argc, argv, ERx("copydb")) != OK)
	    PCexit(FAIL);

	/* 
	** The first parameter scanned should be -param_file=filename 
	** if this is true we need to open FEuta from parameter list
	** we have created
	*/ 
	
	if (FEutaget(ERx("param_file"), 0, FARG_FAIL, &rarg, &pos) == OK)
	{

	    /* if we have more than one flag in input now return error.*/
	    if (argc != 2) 
	    { 
		IIUGerr(E_CD0025_Bad_paramfile, UG_ERR_FATAL, 0); 
		PCexit(FAIL);
	    }
	    
	    /* get parameter file name */
	    param_file = ERx(rarg.dat.name);

	    /* close FE */
	    FEutaclose();
	
	    /* parse the parameters into arglist */
	    if (xffileinput(param_file, &argv, &argc) != OK)	
	    /* error already reported so exit */
	    PCexit(FAIL);

            /* open the FE again */
            if (FEutaopen(argc, argv, ERx("copydb")) != OK)
            PCexit(FAIL); 
	}

	/* database name is required */
	if (FEutaget(ERx("database"), 0, FARG_PROMPT, &rarg, &pos) != OK)
	    PCexit(FAIL);
	dbname = rarg.dat.name;

        if (FEutaget(ERx("user"), 0, FARG_FAIL, &rarg, &pos) == OK)
            user = rarg.dat.name;

        /*
        ** bug 3594
        ** Check the return status from FEningres to determine
        ** if the DBMS was connected to before continuing.
        */
        if (FEningres(NULL, (i4) 0, dbname, user, ERx(""), password,
            groupid, (char *) NULL) != OK)
        {
            PCexit(FAIL);
        }

	if (FEutaget(ERx("createonly"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags |= XF_TAB_CREATEONLY;
	if (FEutaget(ERx("modifyonly"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags |= XF_TAB_MODONLY;
	if (FEutaget(ERx("copyonly"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags |= XF_TAB_COPYONLY;
	if (FEutaget(ERx("tableall"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags |= XF_PRINT_TOTAL;
	if (FEutaget(ERx("orderccm"), 0, FARG_FAIL, &rarg, &pos) == OK)
	{
		output_flags |= XF_ORDER_CCM;
		output_flags |= XF_PRINT_TOTAL;
        }	
	if (FEutaget(ERx("indexonly"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags |= XF_INDEXES_ONLY;
	if (FEutaget(ERx("constraintonly"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags |= XF_CONSTRAINTS_ONLY;
	if (FEutaget(ERx("viewonly"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags |= XF_VIEW_ONLY;
	if (FEutaget(ERx("synonymonly"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags |= XF_SYNONYM_ONLY;
	if (FEutaget(ERx("eventonly"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags |= XF_EVENT_ONLY;
	if (FEutaget(ERx("procedureonly"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags |= XF_PROCEDURE_ONLY;
	if (FEutaget(ERx("registrationonly"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags |= XF_REGISTRATION_ONLY;
	if (FEutaget(ERx("rulesonly"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags |= XF_RULES_ONLY;
	if (FEutaget(ERx("secalarmsonly"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags |= XF_SECALARM_ONLY;
	if (FEutaget(ERx("sequencesonly"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags |= XF_SEQUENCE_ONLY;
	if (FEutaget(ERx("nosequences"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags2 |= XF_NO_SEQUENCES;
	if (FEutaget(ERx("commentsonly"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags |= XF_COMMENT_ONLY;
	if (FEutaget(ERx("rolesonly"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags |= XF_ROLE_ONLY;
	if (FEutaget(ERx("dropincluded"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags |= XF_DROP_INCLUDE;
	if (FEutaget(ERx("relpath"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags |= XF_RELATIVE_PATH;
	if (FEutaget(ERx("nolocations"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags |= XF_NO_LOCATIONS;
	if (FEutaget(ERx("nopermits"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags |= XF_NO_PERMITS;
	if (FEutaget(ERx("uninterrupted"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags |= XF_UNINTURRUPTED;
	if (FEutaget(ERx("nopersist"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags |= XF_CHECK_PERSIST;
	if (FEutaget(ERx("nodependencycheck"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags2 |= XF_NODEPENDENCY_CHECK;

	if (FEutaget(ERx("infile"), 0, FARG_FAIL, &rarg, &pos) == OK)
	{
	    output_flags |= XF_USER_FILEOUT;
	    filenamein = ERx(rarg.dat.name);
	}
	if (FEutaget(ERx("outfile"), 0, FARG_FAIL, &rarg, &pos) == OK)
	{
	    output_flags |= XF_USER_FILEOUT;
	    filenameout = ERx(rarg.dat.name);
	}

	if (FEutaget(ERx("permitsonly"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags |= XF_PERMITS_ONLY;

	if (FEutaget(ERx("norepmod"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags |= XF_NO_REPMODIFY;

	if (FEutaget(ERx("norep"), 0, FARG_FAIL, &rarg, &pos) == OK)
	output_flags2 |= XF_NOREP;

	numobj = 0;
	while (FEutaget(ERx("objname"), numobj, FARG_FAIL, &rarg, &pos) == OK) 
	{
	    if (STcompare(rarg.dat.name, ERx(".")) == 0)
		break;

	    /* Add to our list of objects to be unloaded. */
	    if (IIUGdlm_ChkdlmBEobject(rarg.dat.name, NULL, TRUE))
	    {
                arg_len = STlength(rarg.dat.name);
                if(size_of_nbuf < arg_len)
                {
                    if(nbuf != NULL)
                    {
                        MEfree(nbuf);
                        nbuf = NULL;
                    }
                    size_of_nbuf = (arg_len > (2 * DB_MAXNAME)) ?
					(arg_len + 2 + 1) :
					((2 * DB_MAXNAME) + 2 + 1);
                    nbuf = (char *) MEreqmem( (u_i2) 0, (u_i4)size_of_nbuf, 
					FALSE, &me_status);
                    if(me_status != OK)
                    {
		        IIUGerr(E_CD0026_Out_of_memory, UG_ERR_FATAL, 0); 
                        PCexit(FAIL);
                    }
                }
                MEfill(size_of_nbuf, (unsigned char) '\0', nbuf);

	     	IIUGrqd_Requote_dlm(rarg.dat.name, nbuf);
	        xfaddlist(nbuf);
	    }
	    else
	    {
	        xfaddlist(rarg.dat.name);
	    }
	    numobj++;
	}

        if(nbuf != NULL)
        {
            MEfree(nbuf);
        }

	/* If no explicit object scope is set, turn on table creation. */
	if ( !( output_flags & 
              (XF_TAB_CREATEONLY | XF_TAB_MODONLY | XF_TAB_COPYONLY |
               XF_INDEXES_ONLY | XF_CONSTRAINTS_ONLY | XF_VIEW_ONLY |
               XF_SYNONYM_ONLY | XF_EVENT_ONLY | XF_PROCEDURE_ONLY |
               XF_REGISTRATION_ONLY | XF_RULES_ONLY | XF_SECALARM_ONLY |
               XF_SEQUENCE_ONLY | XF_COMMENT_ONLY | XF_PERMITS_ONLY) ) )
	{
	    output_flags |= XF_PRINT_TOTAL;
	}
 
	/* Get optional arguments */

	if (FEutaget(ERx("portable"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    portable = TRUE;

        if (FEutaget(ERx("directory"), 0, FARG_FAIL, &rarg, &pos)==OK)
	    directory = rarg.dat.name;

        if (FEutaget(ERx("source"), 0, FARG_FAIL, &rarg, &pos)==OK)
	    sourcedir = rarg.dat.name;

        if (FEutaget(ERx("dest"), 0, FARG_FAIL, &rarg, &pos)==OK)
	    destdir = rarg.dat.name;

	/* We now ignore the language parameter. */
	if (FEutaget(ERx("language"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    IIUGerr(E_CD0015_Flag_Ignored, UG_ERR_FATAL, 0);

        if (FEutaget(ERx("groupid"), 0, FARG_FAIL, &rarg, &pos) == OK)
            groupid = rarg.dat.name;

        if (FEutaget(ERx("group_tab_idx"), 0, FARG_FAIL, &rarg, &pos) == OK)
            GroupTabIdx = TRUE;

        if (FEutaget(ERx("no_warn"), 0, FARG_FAIL, &rarg, &pos) == OK)
            Warning = FALSE;

        /*
        ** -nologging option 
        */

        if (FEutaget(ERx("nologging"), 0, FARG_FAIL, &rarg, &pos) == OK)
        {

            uidbdata = IIUIdbdata();

            /* check that user is dba of database */
            if (! uidbdata->isdba)
            {
		IIUGerr(E_CD0027_Nologging_Not_DBA, UG_ERR_FATAL, 1, 
				uidbdata->user); 
		PCexit(FAIL);
            }

            NoLogging = TRUE;
	}

        if (FEutaget(ERx("parallel"), 0, FARG_FAIL, &rarg, &pos) == OK)
            CreateParallelIndexes = TRUE;

	if (FEutaget(ERx("journal"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    SetJournaling = TRUE;

	if (FEutaget(ERx("concurrent_updates"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    concurrent_updates = TRUE;

        if (FEutaget(ERx("password"), 0, FARG_FAIL, &rarg, &pos) == OK)
        {
            char *IIUIpassword();

            if ((password = IIUIpassword(ERx("-P"))) == NULL)
            {
                FEutaerr(BADARG, 1, ERx(""));
                PCexit(FAIL);
            }
        }

	FEutaclose();
    }

    if (EXdeclare(FEhandler, &context) != OK)
    {
	EXdelete();
	PCexit(FAIL);
    }

    xfcapset();

    if (!With_distrib)
    {
        EXEC SQL SET LOCKMODE SESSION WHERE READLOCK = NOLOCK;
    }

    if (xfsetdirs(directory, sourcedir, destdir) != OK)
	PCexit(FAIL);

    /* open files */
    if ((Xf_in = xfopen(filenamein, 0)) == NULL
	|| (Xf_out = xfopen(filenameout, 0)) == NULL
        || (Xf_both = xfduplex(Xf_in, Xf_out, 0)) == NULL)
    {
        PCexit (FAIL);
    }

    /* create scripts */
    /* IIUIDBdata()->suser is normalized for us already. */
    xfcrscript(IIUIdbdata()->suser, progname, (char *) NULL, portable, output_flags,
	output_flags2);

    /* close files */
    xfclose(Xf_both);
    xfclose(Xf_in);
    xfclose(Xf_out);

    if(Warning)
    {
        IIUGmsg(ERget(E_CD0028_Completed), FALSE, 1, dbname);
    }

    FEing_exit();

    EXdelete();

    PCexit(OK);
    /* NOTREACHED */
}
