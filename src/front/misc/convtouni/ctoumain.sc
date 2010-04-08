/*
**  Copyright (c) 2004 Ingres Corporation
**      All rights reserved.
**
**  Name: convtouni - utility for conversion of Ingres data in character 
**		      columns into unicode.
**
**  Usage:
**	
**     convtouni dbname [param_file] | [-uuser] [-P] [-GgroupID]  
**		 [-dest=dir] [-sqlfile=filename] [-automodify] 
**		 [{-col=columns ...}] [{tables ...}]
**
**  Description:
**      This utility will allow conversion of Ingres tables to unicode in 
** 	such that all the char columns are converted to nchar and all 
**	varchar columns are converted to nvarchar.	
**	
** 	This utility will create an intermediate sql script containing
**	appropriate alter table statements to be executed. 
**	This script can be immediately executed using the automodify option 
**	or it can be verified/modified by the user for intended action and 
**	subsequently scheduled for execution at a later time. 
**
**	In automodify mode the script will be created in the temp directory
**	and it will be directly executed for the user.
**
**	The sql script contain the following statements
**	1. Alter table alter column statements.
**	2. Modify statements
**	3. Drop and Create index statements	
**	4. Drop and create constraint statements.
**	
**	The various input parameters are as follows:
**	
**	dbname		 : The data base being exported.
**	-uuser		 : Effective user for the session.
**	-P		 : Password if the session requires password.
**	-GgroupID	 : Group ID of the user.
**	-dest=dir	 : The directory where the sql file will be send to.
**	-sqlfile=filename: The name of the output sql file.
**	-automodify	 : Modify the tables immediately.
**	-tab=tables ...	 : List of tables to convert.
**	-col=columns ..  : List of columns in tablename.columnname form.
**	-param_file=filename : Reads options from a command file.
**
**  Exit Status:
**      OK      convtouni succeeded.
**      FAIL    convtouni failed.
**
**  History:
**      26-Apr-2004 (gupsh01)
**          Created.
**	24-Jun-2004 (gupsh01)
**	    Fixed username in the execution script.
**    25-Oct-2005 (hanje04)
**        Fix prototype for cu_autogen() to prevent compiler
**        errors with GCC 4.0 due to conflicting declarations.
**      28-jan-2009 (stial01)
**          Use DB_MAXNAME for database objects.
*/

# include	<compat.h>
# include	<pc.h>		
# include	<cv.h>
# include	<ex.h>
# include	<nm.h>
# include	<me.h>
# include	<st.h>
# include	<si.h>
# include	<er.h>
# include	<di.h>
# include	<lo.h>
# include	<gl.h>
# include	<sl.h>
# include	<pm.h>		
# include	<iicommon.h>
# include	<fe.h>
# include	<fedml.h>
# include	<uigdata.h>
# include	<ug.h>
# include	<ui.h>
# include       <adf.h>
# include       <afe.h>
# include	<xf.h>
# include 	"erctou.h"

/**
** Name:    ctoumain.sc - main procedure for convtouni
**
** Description:
**	This file defines:
**
**	main		main procedure for convtouni utility
**
** History:
**	26-Apr-2004 (gupsh01) 
**	    Created.
**/

# define REAL_USER_CASE         ERx("ii.$.createdb.real_user_case")
# define REAL_USER_CASE_UPPER   ERx("upper")

/* GLOBALS */
GLOBALREF bool With_distrib;
GLOBALREF PTR	Col_list;


/* Static functions */
static char * scriptname(char *prefix);
static void cu_autogen (char *dir, char *file, char *path, LOCATION *lptr);
void getiname(char *name);

/*{
** Name:	ctoumain - main procedure for convtouni
**
** Description:
**	convtouni - main program
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
**    24-Feb-2009 (coomi01) b121608
**          Test result code from IIUGdlm_ChkdlmBEobject()
**    18-May-2009 (coomi01) b116150
**          Properly present the user name to the ctouexec script.
**    16-Jun-2009 (maspa05) trac 386, b122202
**          added dbaname parameter to xfcrscript as part of adding
**            -nologging parameter to copydb, unloaddb 
**
**	MKMFIN Hints
**
PROGRAM = convtouni

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
    char	*progname = ERx("Convtouni");
    char        *filename = ERx("ctouout.sql");
    bool	portable = FALSE;
    i2		numobj;
    EX_CONTEXT	context;
    EX		FEhandler();
    i4          output_flags = 0;
    i4          output_flags2 = 0;
    char 	*owner;
    char 	tbuf[256];

    char	gendir [MAX_LOC + 1]; 
    char	genfile [MAX_LOC + 1]; 
    char	genpath [MAX_LOC + 1]; 

    bool	automodify_on = FALSE;
    bool	usergiven = FALSE;
    CL_ERR_DESC	err_code;
    LOCATION    loc_in;

    i4 		colcnt = 0;
    char	cvtabname [DB_MAXNAME +1];
    char	ctoucmdline[CMD_BUF_SIZE];    
    char        *param_file = ERx("");

    /* Tell EX this is an ingres tool. */
    (void) EXsetclient(EX_INGRES_TOOL);

    MEadvise(ME_INGRES_ALLOC);

    /* 
    ** Always create indexes in parallel and allow 
    ** journaling for recovery 
    */
    CreateParallelIndexes = TRUE;
    SetJournaling = TRUE;
    output_flags |=  XF_DROP_INCLUDE;
    output_flags2 |= XF_CONVTOUNI;

    /* Call IIUGinit to initialize character set attribute table */
    if ( IIUGinit() != OK)
	PCexit(FAIL);

    FEcopyright(progname, ERx("2004"));
 
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
	i4     	pos;

	if (FEutaopen(argc, argv, ERx("convtouni")) != OK)
	    PCexit(FAIL);

	if (FEutaget(ERx("param_file"), 0, FARG_FAIL, &rarg, &pos) == OK)
        {

            /* if we have more than one flag in input now return error.*/
            if (argc != 2)
            {
                IIUGerr(E_CT0025_Bad_paramfile, UG_ERR_FATAL, 0);
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
            if (FEutaopen(argc, argv, ERx("convtouni")) != OK)
            PCexit(FAIL);
        }

	/* database name is required */
	if (FEutaget(ERx("database"), 0, FARG_PROMPT, &rarg, &pos) != OK)
	    PCexit(FAIL);
	dbname = rarg.dat.name;

        if (FEutaget(ERx("user"), 0, FARG_FAIL, &rarg, &pos) == OK)
        {
          usergiven = TRUE;
          user = rarg.dat.name;
        }
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

        if (FEutaget(ERx("sqlfile"), 0, FARG_FAIL, &rarg, &pos) == OK)
        {
          filename = ERx(rarg.dat.name);
        }

        numobj = 0;
        while (FEutaget(ERx("objname"), numobj, FARG_FAIL, &rarg, &pos) == OK) 
        {
	  if (STcompare(rarg.dat.name, ERx(".")) == 0)
	    break;
	  /* Add to our list of objects to be unloaded. */
	  if (UI_REG_ID == IIUGdlm_ChkdlmBEobject(rarg.dat.name, NULL, TRUE))
	  {
	    char	nbuf[((2 * DB_MAXNAME) + 2 + 1)];
	    IIUGrqd_Requote_dlm(rarg.dat.name, nbuf);
	    xfaddlist(nbuf);
	  }
	  else
	  {
	    xfaddlist(rarg.dat.name);
	  }
	  numobj++;
        }

        /* Get optional arguments */

        if (FEutaget(ERx("dest"), 0, FARG_FAIL, &rarg, &pos)==OK)
          directory = rarg.dat.name;

        if (FEutaget(ERx("groupid"), 0, FARG_FAIL, &rarg, &pos) == OK)
	  groupid = rarg.dat.name;

        if (FEutaget(ERx("password"), 0, FARG_FAIL, &rarg, &pos) == OK)
        {
	  char *IIUIpassword();

	  if ((password = IIUIpassword(ERx("-P"))) == NULL)
	  {
	    FEutaerr(BADARG, 1, ERx(""));
	    PCexit(FAIL);
	  }
        }

	/* columns */
        colcnt = 0;
	while (FEutaget(ERx("colname"), colcnt, FARG_FAIL, &rarg, &pos) == OK)
        {
	  if (STcompare(rarg.dat.name, ERx(".")) == 0)
	    break;

	  /* Add to our list of objects to be unloaded. */
	  if (UI_REG_ID == IIUGdlm_ChkdlmBEobject(rarg.dat.name, NULL, TRUE))
	  {
	    char	nbuf[((2 * DB_MAXNAME) + 2 + 1)];
	    IIUGrqd_Requote_dlm(rarg.dat.name, nbuf);
	    xfaddobject(nbuf, XF_COLUMN_LIST);
	  }
	  else
	  {
	    xfaddobject(rarg.dat.name, XF_COLUMN_LIST); 
	  }

	  /* Now get the tablename from the column string */
	  if (xfcnvtabname(rarg.dat.name, cvtabname, DB_MAXNAME) == OK)
	  {
	    xfaddobject(cvtabname, XF_TABFILTER_LIST);
	    xfaddlist (cvtabname); 
	  }
	  else 
	  {
	    EXdelete();
	    PCexit(FAIL);
	  }
	  colcnt++;
        }

        /* get the other flags */
        if (FEutaget(ERx("automodify"), 0, FARG_FAIL, &rarg, &pos) == OK)
	  automodify_on = TRUE;

        FEutaclose();

    }/* end of parsing the command line flags. */

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

    if (automodify_on)
    {
      /* call autogen to obtain the working directory, 
      ** sqlfilename and full path of the executable to 
      ** run. If automodify is on then the scrip will 
      ** be generated and executed immediately.
      */
      cu_autogen (gendir, genfile, genpath, &loc_in);
      directory = ERx(gendir);
      sourcedir = ERx(gendir);
      destdir = ERx(gendir);
      filename = ERx(genfile); 
    }

    if (xfsetdirs(directory, sourcedir, destdir) != OK)
	PCexit(FAIL);

    /* open sql file */
    if ((Xf_in = xfopen(filename,0)) == NULL) 
    {
        PCexit (FAIL);
    }

    /* open script file */
    if ((Xf_unload = xfopen(scriptname(ERx("ctouexec")), 
				TH_SCRIPT)) == NULL)
    {
        PCexit(FAIL);
    }

    /* create scripts */
    /* IIUIDBdata()->suser is normalized for us already. */
    xfcrscript(IIUIdbdata()->suser, progname, (char *) NULL, portable,
                output_flags, output_flags2);

    /* generate command to run script file */
    {
       char        *unload_user;

       if (usergiven)
       {
         unload_user = user;

         getiname(unload_user);

	 /* 
	 ** CMDLINE (defined in xf.qsh, with various OP system flavours): 
	 ** Presents the username in format -u'xxxx' where the xxxx string
	 ** is contained in the unload_user variable above. The quotation 
	 ** marks for the UNIX variant being to protect the content from
	 ** Shell expansion, $ingres for example.
	 **
	 ** However, FEutaget() above will have returned the user as C-string 
	 ** -uyyyyy in the user variable, not yyyyy as one might have first
	 ** expected. This suited the connection to the database earlier on,
	 ** but we will now incorrectly produce -u'-uyyyyy' from the CMDLINE, 
	 ** whereas what we actually want is -u'yyyyy'.
	 **
	 ** A similar story is told for the other OS flavours.
	 **
	 ** So, look for and step over the -u - if it is present. 
	 */
	 if ( 0 == STncmp("-u", unload_user, 2) )
	 {
	     unload_user += 2;
	 }



         STprintf (ctoucmdline, CMDLINE, unload_user, 
		    SUFLAG, dbname, xffilename(Xf_in));
	}
	else
         STprintf (ctoucmdline, CMDLINE2, SUFLAG, 
		    dbname, xffilename(Xf_in));
    }

    /* write command to main shell-scripts */
    xfwrite(Xf_unload, ctoucmdline);

    IIUGmsg(ERget(MSG_COMPLETE3), FALSE, 1,xffilename(Xf_in));

    if (automodify_on)
    {
	/*
        **      if -u flag given then run:
        **        sql -uusername dbname < new_filename
        if (usergiven)
          STprintf(tbuf, ERx( "sql %s -s %s <%s" ),
                        user, dbname, genpath );
        else
          STprintf(tbuf, ERx( "sql -s %s <%s" ),
                                dbname, genpath );

	*/
	
        IIUGmsg(ERget(S_CT0008_Executing_script), FALSE, 1,xffilename(Xf_unload));

        if( PCcmdline((LOCATION *) NULL, xffilename(Xf_unload), PC_WAIT,
                        (LOCATION *) NULL, &err_code) != OK )
	{
	    printf("FAILED\n");
            PCexit(FAIL);
	}

        /* Delete the location */
        LOdelete(&loc_in);
        IIUGmsg(ERget(S_CT0010_Execution_completed), FALSE, 1,xffilename(Xf_unload));
   }

   /* close files */
    xfclose(Xf_in);
    xfclose(Xf_unload);

    FEing_exit();

    EXdelete();

    

    PCexit(OK);
}

/* Name: cu_autogen - generates the directory and temp file names
** in case of automodify.
**
** History:
**	26-Apr-2004 (gupsh01)
**	    Created.
*/
static void
cu_autogen (directory, sqlfile, fullpath, lptr)
char    	*directory; 
char    	*sqlfile; 
char    	*fullpath;
LOCATION	*lptr;
{
    LOCATION        tloc;
    LOCATION        loc_in;
    char            loc_buf[MAX_LOC + 1];
    char            tmp_buf[MAX_LOC + 1];

    char            dev[MAX_LOC + 1];
    char            path[MAX_LOC + 1];
    char            fprefix[MAX_LOC + 1];
    char            fsuffix[MAX_LOC + 1];
    char            version[MAX_LOC + 1];

    char            *new_filename;
    char            *temploc;

    if (lptr)
      tloc = *lptr;
    /* Get the temporary path location */
    NMloc(TEMP, PATH, NULL, &tloc);
    LOcopy(&tloc, tmp_buf, &loc_in);
    LOtos(&loc_in, &temploc);

    /* Get the new filename for path location */
    LOuniq(ERx("convtouni"), ERx("sql"), &tloc);
    LOcopy(&tloc, loc_buf, &loc_in);
    LOtos(&loc_in, &new_filename);

    /* Get just the filename for copydb code   */
    LOdetail(&tloc, dev, path, fprefix, fsuffix, version);
    if ( *fsuffix !=  '\0' )
    {
      STcat(fprefix, ".");
      STcat(fprefix, fsuffix);
    }

    STcopy (temploc, directory); 
    STcopy (fprefix, sqlfile); 
    STcopy (new_filename, fullpath); 
}

/*
**      getiname - Get the owner name in the right form.
**
**      Get the FIPS-related parameter from config.dat:
**      if real_user_case is upper, upper-case the owner name.
*/
void
getiname(char *name)
{
    char        *usercase;

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

/*
** scriptname - takes a filename and attaches a system-dependent suffix
**              appropriate for an executable script.
**              The name does not persist since it's formatted into one
**              static global buffer.
**
**              This routine assumes a "well behaved" caller, and doesn't
**              check for length.
*/

static char Fname[ MAX_LOC + 1 ];

static char *
scriptname(char *prefix)
{
    STprintf(Fname, ERx("%s.%s"), prefix, SCRIPT_SUFFIX);
    return (Fname);
}

