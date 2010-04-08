/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<ci.h>
#include	<me.h>
#include	<lo.h>
#include	<ex.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include 	<st.h>
#include	<abfcnsts.h>
#include	<abfcompl.h>
#include    	<abclass.h>
#include	"erai.h"

/**
** Name:	abimain.c -	Application Image Builder Main Entry Point.
**
** Description:
**	The ImageApp utility is an ABF entry point that can be used to compile
**	and link an application in batch mode.  After getting all arguments, it
**	reads the application in and then compiles the files.  Finally, it links
**	the application.  The real work is done by routines in the ABF library.
**
** History:
**	Revision 6.0  9-jun-1987 (Joe)
**	Initial Version
**
**	Revision 6.3  89/09  wong
**	Added support for RunTime INGRES using new compile options.
**	29-dec-89 (kenl)
**		Add handling of +c flag.
**	Revision 6.3/03/00  90/03  wong
**	Modified to be 'IIabfImage()' entry point called from ABF main.
**	14-aug-91 (blaise)
**		Added support for -nocompile flag (don't compile, just link
**		object file).
**	19-aug-91 (blaise)
**		Just for symmetry, added -compile flag. This is the default,
**		so the flag does nothing, unless -nocompile and -compile are
**		issued on the same command line, in which case -compile takes
**		precedence.
**	30-aug-91 (leighb) Revived ImageBld.
**	16-dec-92 (essi)
**		Added support for flag -constants_file=fn. This flag is used
**		to read and override global constants from a file.
**	02-feb-93 (essi)
**		Added call to IIUGvfVerifyFile to check the validity of the
**		constants_file.
**      01-feb-00 (rodjo04) bug 100247
**              Added buffer for -R role support. 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**/

GLOBALREF bool	IIabVision;
GLOBALREF bool	IIabAbf;
GLOBALREF char	**IIar_Dbname;
GLOBALREF bool	IIabOpen;
GLOBALREF bool	IIabWopt;
GLOBALREF bool	IIabC50;
GLOBALREF char	*IIabGroupID;
GLOBALREF char	*IIabRoleID;
GLOBALREF char	*IIabPassword;
GLOBALREF bool	IIabfmFormsMode;
GLOBALREF i4    IIOIimageBld;		 
GLOBALREF bool  IIabKME;


/*{
** Name:	main() - ABF Image Builder Main Entry Point and Argument Parser.
**
** Description:
**	The ImageApp program.  Usage is:
**
**		imageapp dbname application [-uuser] [-f] [-oexecutable]
**				[-P] [-GgroupID] [-RroleID]
**				[-c] [-Xflag] [-nocompile] [-constants_file=fn]
**
**		where:
**
**		dbname		is a database name.  If not specified,
**				the user will be prompted.
**		application	is the name of the application to
**				build an image for.
**		-f		the "force" argument.  If set, then
**				all files in the application should be
**				compiled.
**		-oexecutable	The name of the executable to create.
**				If not given, it will be defaulted.
**
**		-c		the "compileonly" argument.  If set, then do
**				not link an executable for the application.
**		
**		-P		Prompt for user password.
**
**		-GgroupID	The group ID to be used to connect to the DBMS.
**
**		-RroleID	The role ID to be used for the application; the
**				user will be prompted for the password.
**
**		-nocompile	Don't compile, just link object files.
**
**		-compile	The reverse of nocompile (this is the default)
**
**		-constants_file=fn 
**				Use 'fn' to override the global constants
**				during image build.
**
**	In order to use the FEuta routines to parse the command line,
**	the following entry has been put into the UTEXE.DEF file:
**
**imageapp        imageapp        PROCESS -+      dbname application [-uuser] 
**		[-f] [-w] [+wopen] [-5.0] [-oexecutable] [-P] [-GgroupID] 
**		[-RroleID] [-constants_file=fn]
**        database        '%S'    0       Database?       %S
**        application     '%S'    0       Application?    %S
**        force           -f      0       ?               -f
**        executable      -o%S    0       ?               -o%S
**        compileonly     -c      0       ?               -c
**        open            +wopen
**        warning         -w
**        5.0             -5.0
**        user            -u'%S'  0       ?               -u%-
**        equel           -X%S    0       ?               -X%-
**        roleid          -R'%S'  0       Role?           -R%-
**        groupid         -G'%S'  0       Group?          -G%-
**        password        -P      0
**        connect         +c%S
**        nocompile       -nocompile
**        compile         -compile
**		
**
** Returns:
**	Return to operating system.
**
**	OK if everything ok.
**	status, usually FAIL, if anything goes wrong.
**
** History:
**	9-jun-1987 (Joe)
**		Initial Version
**     22-mar-94 (donc) Bug 59981
**		Replace CI_PART_RUNTIME in preference for CI_ABF_IMAGE
*/

/* Parameter Description */

static ARG_DESC
	args[] = {
		/* Required */
		{ERx("database"),	DB_CHR_TYPE,	FARG_PROMPT,	NULL},
		{ERx("application"),	DB_CHR_TYPE,	FARG_PROMPT,	NULL},
		/* Internal optional */
		{ERx("equel"),		DB_CHR_TYPE,	FARG_FAIL,	NULL},
		/* Optional */
		{ERx("user"),		DB_CHR_TYPE,	FARG_FAIL,	NULL},
		{ERx("open"),		DB_BOO_TYPE,	FARG_FAIL,	NULL},
		{ERx("warning"),	DB_BOO_TYPE,	FARG_FAIL,	NULL},
		{ERx("5.0"),		DB_BOO_TYPE,	FARG_FAIL,	NULL},
		{ERx("groupid"),	DB_CHR_TYPE,	FARG_FAIL,	NULL},
		{ERx("roleid"),		DB_CHR_TYPE,	FARG_FAIL,	NULL},
		{ERx("password"),	DB_BOO_TYPE,	FARG_FAIL,	NULL},
		{ERx("force"),		DB_BOO_TYPE,	FARG_FAIL,	NULL},
		{ERx("executable"),	DB_CHR_TYPE,	FARG_FAIL,	NULL},
		{ERx("connect"),	DB_CHR_TYPE,	FARG_FAIL,	NULL},
		{ERx("constants_file"),	DB_CHR_TYPE,	FARG_FAIL,	NULL},
		/* Undocumented */
		{ERx("nocompile"),	DB_BOO_TYPE,	FARG_FAIL,	NULL},
		{ERx("compile"),	DB_BOO_TYPE,	FARG_FAIL,	NULL},
		{ERx("compileonly"),	DB_BOO_TYPE,	FARG_FAIL,	NULL},
		NULL
};

STATUS
IIabfImage ( argc, argv )
i4	argc;
char	**argv;
{
	char		*applname;	/* application name */
	char		exebuf[MAX_LOC+1];/* Buffer for Name of executable */
	char		*executable;    /* executable to use */
	char		*xflag;		/* Pipe names, with -X flag.
					** NOTE:  These must be set to an empty
					** string or a value from the command
					** line.  It must not be NULL in the
					** call to 'FEingres()'.
					*/
	char		*uname;		/* Username, with -u flag.
					** NOTE:  These must be set to an empty
					** string or a value from the command
					** line.  It must not be NULL in the
					** call to 'FEingres()'.
					*/
	char		*connect;	/* connection/with clause parameters */
	bool		force;		/* TRUE if -f flag set */
	bool		compileonly;	/* TRUE if -c flag set */
	bool		password;	/* TRUE if -P flag set */
	bool		nocompile;	/* TRUE if -nocompile flag set */
	bool		compile;	/* TRUE if -compile flag set */
	bool		var_ingres;	/* TRUE if !CI_ABF && CI_ABF_IMAGE */
	STATUS		retstat;	/* Return status */
	EX_CONTEXT	context;	/* Used for stack context */
	char		*cons_file;	/* Global Constants File */
	char        	*role;      	/* Default role for application */

	EX	IIARexchand();	/* interrupt handler */
	char    *IIABgrGtRole();
	char	*IIUIpassword();
	char	*IIUIroleID();

	/*
	** If site can run ABF or VISION, or are a VAR copy, they can run
	** ImageApp.  'retstat' will be set to the status of a failed capability
	** check that was supposed to succeed.
	*/
	IIabVision = TRUE ;
        var_ingres = TRUE ;
        IIabAbf = TRUE ;
        var_ingres = TRUE ;
	if ( var_ingres && ( IIabAbf || IIabVision ))
	    var_ingres = FALSE;

	IIAMsivSetIIab_Vision(IIabVision);		 
	if ( !IIabAbf && !IIabVision && !var_ingres )
	{ /* Print an error if not allowed and exit. */
		char	errbuf[ER_MAX_LEN+1];

		ERreport(retstat, errbuf);
		FEmsg(errbuf, FALSE);
		return FAIL;
	}

	password = FALSE;
	force = FALSE;
	compileonly = FALSE;
	nocompile = FALSE;
	compile = FALSE;
	executable = NULL;
	*exebuf = EOS;
	xflag = uname = connect = ERx("");
	cons_file = NULL;
	IIabfmFormsMode = FALSE;  /* We're doing a non-forms system build */

	/*
	** Get the parameters.  (See 'args[]' above.)
	*/

	args[0]._ref = (PTR)IIar_Dbname;
	args[1]._ref = (PTR)&applname;
	args[2]._ref = (PTR)&xflag;
	args[3]._ref = (PTR)&uname;
	args[4]._ref = (PTR)&IIabOpen;
	args[5]._ref = (PTR)&IIabWopt;
	args[6]._ref = (PTR)&IIabC50;
	args[7]._ref = (PTR)&IIabGroupID;
	args[8]._ref = (PTR)&IIabRoleID;
	args[9]._ref = (PTR)&password;
	args[10]._ref = (PTR)&force;
	args[11]._ref = (PTR)&executable;
	args[12]._ref = (PTR)&connect;
	args[13]._ref = (PTR)&cons_file;
	args[14]._ref = (PTR)&compileonly;
	args[15]._ref = (PTR)&nocompile;
	args[16]._ref = (PTR)&compile;
	if (IIUGuapParse(argc, argv, *argv, args) != OK)
		return FAIL;

	/*
	** Make cons_file avaible to other modules if -constants_file
	** flag was specified on command line.
	*/

	if (cons_file != NULL)
	{
		/*
		** Move the pointer to one past the `=` sign in
		** constants_file=filename.
		** The "constants_file=" is 15 characters long so
		** we move the pointer to position 16.
		*/
		cons_file += 16;
		_VOID_ IIUGvfVerifyFile ( cons_file, UG_IsExistingFile,
			FALSE, TRUE, UG_ERR_FATAL );
		_VOID_ IIABscfSetConstantFile(cons_file);
	}
	/*
	** If the user doesn't give an executable to use, then set
	** the pointer to point to the empty buffer exebuf.  It will
	** be filled in by the linking routine with the default name
	** of the executable.
	*/
	if (executable == NULL || *executable == EOS)
	    executable = exebuf;

	/* Start execution */

	if (EXdeclare(IIARexchand, &context) != OK)
	{
		EXdelete();
		return FAIL;
	}

	if (IIOIimageBld)							 
	     FEcopyright( ERx("APPLICATION/IMAGE-BUILD"), ERx("1991") );	 
	else FEcopyright( ERx("APPLICATION/IMAGE"), ERx("1988") );		 

	if (FEchkenv(FENV_TWRITE) != OK)
	{ /* Check user environment.  */
		IIUGerr( ABTMPDIR, UG_ERR_ERROR, 0 );
		EXdelete();
		return FAIL;
	}

	if ( password )
	{ /* check and prompt for password */
		char	*p;

		if ( (p = IIUIpassword(ERx("-P"))) != NULL )
			IIabPassword = p;
		else
		{
			FEutaerr(BADARG, 1, IIabPassword);
			EXdelete();
			return FAIL;
		}
	}

/* bug 54568 */

    if ( *IIabRoleID == EOS )
    { /* check for default role */
        if ((retstat = FEingres(*IIar_Dbname, xflag, uname,
                    (char *)NULL)) != OK)
            return retstat;
        role = ERx("");
        role = IIABgrGtRole (applname);
        FEing_exit();
 
        if ( *role != EOS )
        {
          char role_buf[40];
            
            role_buf[0] = '\0';
            IIabRoleID = ERx("-R");
            STcat(role_buf, IIabRoleID);
            STcat(role_buf, role);
            IIabRoleID = (PTR) &role_buf;

        }
    }

	if ( *IIabRoleID != EOS )
	{ /* check for role ID and prompt for password */
		char	*p;

		if ( (p = IIUIroleID(IIabRoleID)) != NULL )
			IIabRoleID = p;
		else
		{
			FEutaerr(BADARG, 1, IIabRoleID);
			EXdelete();
			return FAIL;
		}
	}

	if (*connect != EOS)
	{
		IIUIswc_SetWithClause(connect);
	}
	if ( FEingres( *IIar_Dbname, xflag, uname, IIabRoleID,
			IIabPassword, IIabGroupID, (char *)NULL) != OK )
	{
		IIUGerr( NOSUCHDB, UG_ERR_ERROR, 1, (PTR)*IIar_Dbname );
		retstat = FAIL;
	}
	else
	{
	    i4	options;	/* compilation options */

	    options = ABF_COMPILE|ABF_NOERRDISP;
	    if ( force )
		options |= ABF_FORCE;

	    if ( var_ingres )
		options |= ABF_OBJONLY;

	    if (nocompile && !compile)
		options |= ABF_NOCOMPILE;

	    IIAMsetDBsession();	/* set lock mode for DBMS session */
	    if (retstat = abfimage(applname, options, compileonly, executable))
	    {
		IIUGerr(E_AI0002_EXE_NOT_MADE, UG_ERR_ERROR, 0);
	    }
	    else
	    {
		IIUGmsg(ERget(S_AI0001_EXE_MADE), FALSE, 1, (PTR) executable);
	    }
	    IIAMclrDBsession();	/* clear lock mode for DBMS session */
	    FEing_exit();		/* Close database */
	}

	EXdelete();			/* Get rid of exception handler */
	return retstat;
}
