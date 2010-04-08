/*
**	Copyright (c) 1987, 2004 Ingres Corporation
*/

# include       <compat.h>
# include       <pc.h>          /* 6-x_PC_80x86 */
# include       <st.h>
# include       <ci.h>
# include       <er.h>
# include       <ex.h>
# include       <me.h>
# include       <gl.h>
# include       <sl.h>
# include       <iicommon.h>
# include       <fe.h>
# include       <ug.h>
# include       <ooclass.h>
# include       <oocat.h>
# include       <cu.h>
# include	<lo.h>
# include	<si.h>
# include	"erie.h"
# include	"ieimpexp.h"


/**
** Name:	ieimain.c -	IIImport Main Entry Point.
**
** Description:
**	Defines
**		main--calls IIIEiii_IIImport(), which does the real work.
** Parameters:
**	argc            {int}
**	argv[]          { char *}
**
** Returns:
**	Nothing
**
** Side Effects:
**	Reads an intermediate file and populates the database.
**	Write a listing file of source files which require copying.
**
** History:
**	23-jun-1993 (daver)
**		First Written (stolen from copyapp)
**	08-sep-1993 (daver)
**		Documented, cleaned up, etc.
**	05-oct-1993 (daver)
**		Prevent the simultaneous use of both the -check AND -copysrc 
**		command-line flags.
**	02-dec-1993 (daver)
**		Fix bug 57283; don't use severe copyapp message if appl name
**		is misspelled. Changed return of value fetch_sdir, removed 
**		commented-out code, unused variables, outdated comments.
**	22-dec-1993 (daver)
**		Fix bug 57928; add two extra bytes in username buffer
**		for the "-u" that IIIEesp_ExtractStringParam() now adds.
**	04-jan-1994 (daver)
**		Added LISTFILEFLAG to our set of groovy options. 
**		Part of fix for bug 57935.
**	23-mar-1994 (daver)
**		Removed obsolete library ABFTO60LIB from NEEDLIBS.
**	03/25/94 (dkh) - Added call to EXsetclient().
**	21-apr-1994 (connie) Bug #59835
**		Changed && to & to fix appearing of E_IE003F when 
**		checking -copysrc and -check flags
**      21-aug-1995 (morayf)
**		Initialised username array start to zero, since some
**		compilers allow any old rubbish to be here if the -user=XXXX
**		option is not used. This means FEingres() call can fail.
**      07-dec-2000 (horda03)
**              Upon successfull completion of the function, terminate the
**              process via PCexit(). Bug 103429.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**      29-Sep-2004 (drivi01)
**          Updated NEEDLIBS to build iiexport dynamically on both
**          windows and unix.
*/


/*
**	MKMFIN Hints
**
PROGRAM =	iiimport

NEEDLIBS =	IMPEXPLIB COPYAPPLIB CAUTILLIB COPYUTILLIB \
		COPYFORMLIB SHINTERPLIB SHFRAMELIB SHQLIB \
		SHCOMPATLIB SHEMBEDLIB

UNDEFS = II_copyright
*/

GLOBALDEF	bool	IEDebug = FALSE;


int
main(argc, argv)
i4	argc;
char	*argv[];
{
	/* interrupt handler for front-ends */
	EX_CONTEXT	context;
	EX	FEhandler();	
	
	char	*dbname = NULL;			/* name of the database */
	char	*appname = NULL;                /* name of the applicaton */
	char	applowner[FE_MAXNAME + 1];	/* owner of the application */
	char	username[FE_MAXNAME + 3];	/* user for -u flag + "-u" */
	char	newowner[FE_MAXNAME + 1];	/* defines new ownership */
	OOID	applid;
	
	LOCATION	loc;
	IEPARAMS	*srclist = NULL;/* objects that might have src code */

	char	intfile[MAX_LOC + 1];	/* intermediate file name */
	char	listfile[MAX_LOC + 1];
	char	newsrcdir[MAX_LOC + 1];	/* source code directory paths */
	char	oldsrcdir[MAX_LOC + 1];
	
	i4	allflags = 0;		/* bit mask those bool flags!*/
	
	i4	i;			/* ubiquitous for loop index */
	i4	xactnum;		/* transaction processing */
	STATUS	rval;			/* ubiquitous return value */
	
	/* Tell EX this is an ingres tool. */
	(void) EXsetclient(EX_INGRES_TOOL);

	/* Use the ingres allocator instead of malloc/free default (daveb) */
	MEadvise( ME_INGRES_ALLOC );

	/* Add IIUGinit call to initialize character set attribute table */

	if ( IIUGinit() != OK)
	{
	        PCexit(FAIL);
	}

	if ( EXdeclare(FEhandler, &context) != OK )
	{
		EXdelete();
		IIUGerr(E_IE000C_EXCEPTION, UG_ERR_ERROR, 1, ERx("IIIMPORT"));
		PCexit(FAIL);
	}

	FEcopyright(ERx("IIIMPORT"), ERx("1993"));

	/*
	** Now do the real work.
	**
	** Command looks like:
	**
	**    iiimport dbname appname [<flags>]
	**
	** where <flags> is:
	**
	**      -check | copysrc                      ! copyapp in -p,q,r
	**                                            ! OR copyapp in -s -a
	**      -dropfile                             ! copyapp in -c
	**      -user=username                        ! copyapp in -u
	**      -listing=filename                     ! copyapp in -l
	**      -intfile=filename                     ! copyapp in <db><ifile>
	**
	** note: -check and -copysrc are mutually exclusive; 
	**       -owner=newowner not implemented yet
	**
	** so lets pull out the dbname and appname, then loop thru flags
	*/

	/* iiimport just requires dbname and appname (argc > 2) */
	if (argc <= 2)
	{
		/* NEED TO produce better message */
		IIUGerr(E_IE000D_Need_2_Args, UG_ERR_ERROR, 0);
		IIIEium_ImportUsageMsg();
	}

	/* make sure first two args, dbname/appname, don't start with - */
	if (argv[1][0] == '-' || argv[2][0] == '-')
	{
		IIUGerr(E_IE000E_Flags_Before_DB_App, UG_ERR_ERROR, 0);
		IIIEium_ImportUsageMsg();
	}

	/* dbname is in argv[1]; check validity? */

	dbname = argv[1];

	/* app name is in argv[2]; check validity? */

	appname = argv[2];

	/* default file name is iiexport.tmp */
	STcopy(ERx("iiexport.tmp"), intfile);
	*listfile = EOS;

	/* Ensure no rubbish gets passed to FEingres if user not set */
	username[0] = '\0';

	/*
	** flip through flags. at this point, everything should start
	** with the "-" thang. if it don't, do that usage thang
	*/

	for (i = 3; i < argc; i++)
	{
	    if (argv[i][0] != '-')
	    {
		IIUGerr(E_IE000F_Args_have_dash, UG_ERR_ERROR, 1, argv[i]);
		IIIEium_ImportUsageMsg();
	    }
	    else
	    {
		switch (argv[i][1])
		{
		  case 'c':
			/*
			** check and copysrc are our two flags. but they
			** really are mutually exclusive, since it
			** doesn't make sence to transfer files if all you
			** wanted to do was make sure all the components
			** actually existed in the target application.
			**
			** Prevent the use of both flags here.
			*/
			if (STcompare(argv[i], ERx("-check")) == 0)
			{
			    if (allflags & COPYSRCFLAG)
			    {
				IIUGerr(E_IE003F_Both_Check_Copysrc,
					UG_ERR_ERROR, 0);
			    	IIIEium_ImportUsageMsg();
			    }

			    allflags = allflags | CHECKFLAG;
			}
			else if (STcompare(argv[i], ERx("-copysrc")) == 0)
			{
			    if (allflags & CHECKFLAG)
			    {
				IIUGerr(E_IE003F_Both_Check_Copysrc,
					UG_ERR_ERROR, 0);
			    	IIIEium_ImportUsageMsg();
			    }

			    allflags = allflags | COPYSRCFLAG;
			}
			else
			{
			    IIUGerr(E_IE0010_Illegal_flag, UG_ERR_ERROR, 
						2, argv[i], ERx("IIIMPORT"));
			    IIIEium_ImportUsageMsg();
			}
			break;
		  case 'd':
			if (STcompare(argv[i], ERx("-dropfile")) == 0)
			{
				allflags = allflags | DROPFILEFLAG;
			}
			else if (STcompare(argv[i], ERx("-debug")) == 0)
			{
				IEDebug = TRUE;
			}
			else
			{
			    IIUGerr(E_IE0010_Illegal_flag, UG_ERR_ERROR, 
						2, argv[i], ERx("IIIMPORT"));
			    IIIEium_ImportUsageMsg();
			}
			break;
		  case 'i':
			IIIEesp_ExtractStringParam(ERx("intfile"), argv[i],
							intfile, FALSE);
			break;
		  case 'l':
		        IIIEesp_ExtractStringParam(ERx("listing"), argv[i],
							listfile, FALSE);
			allflags = allflags | LISTFILEFLAG;
			if (IEDebug)
				SIprintf("\nLISTING is %s\n", listfile);
			break;
		  case 'u':
			IIIEesp_ExtractStringParam(ERx("user"), argv[i], 
							username, FALSE);
			break;
		  case 'o':
			IIIEesp_ExtractStringParam(ERx("owner"), argv[i], 
							newowner, FALSE);
			break;
		  default:
			IIUGerr(E_IE0010_Illegal_flag, UG_ERR_ERROR, 
						2, argv[i], ERx("IIIMPORT"));
			IIIEium_ImportUsageMsg();
			break;
		} /* end switch on 2nd char of each param */

	    } /* end else 1st char is a '-' and might be a valid flag */

	} /* end for loop, flipping though each argv */

	/* start processing the command line */

	/*
	** The "" 3rd param is the Xpipe, which doesn't seem to get used.
	** I imagine this would change if we provided a forms based interface
	** and execution capabilities from within Vision. IF so, gang, here's
	** where you'd add the Xpipe shme; look at abf!copyapp!copyappl.c
	** for details on processing the Xpipe off the command line.
	*/
	if ( FEingres(dbname, username, "", (char *)NULL) != OK )
	{
		IIUGerr(E_UG000F_NoDBconnect, UG_ERR_ERROR, 1, dbname );
	        IIUGerr(E_IE0005_Exitmsg, UG_ERR_FATAL, 0);
	}

	/*
	** create one big transaction for the whole copyapp operation.
	*/
	IIUIlxbLabelXactBegin(&xactnum);


	/*
	** fetch_sdir does more than just get the source directory of
	** the application in question. It also returns us the object id of
	** the application, used as "applid" in ii_abfobjects, and will
	** check to see if the application exists as well. All in one fell
	** swoop! If the application "appname" doesn't exist, it issues an
	** error and exits. But wait there's more! It now also returns
	** the application owner as well. Who could ask for more! 
	** 
	*/
	if (fetch_sdir(appname, newsrcdir, &applid, applowner) != OK)
	{
		IIUGerr(E_IE0013_No_Such_App, UG_ERR_ERROR, 1, (PTR) appname);
	        IIUGerr(E_IE0005_Exitmsg, UG_ERR_FATAL, 0);
	}

	/* 
	** we found an application "appname" in the database, and now
	** know its id and owner
	** 
	** If -copysrc was specified, then we'll be interested in the
	** source code directory of the application components being
	** copied. This directory is written in the intermediate file!
	*/
	rval = IIIEiii_IIImport(applid, applowner, intfile, 
					allflags, oldsrcdir, &srclist);

	/*
	** this will either commit the one big transaction,
	** or abort it again to make sure if anything inside
	** it aborted.
	*/
	IIUIlxeLabelXactEnd(xactnum);
	if (rval == OK)
	{
		IICApfdPatchFormDates(applid);
		
		if (allflags & DROPFILEFLAG)
		{
			LOfroms(FILENAME, intfile, &loc);
			LOdelete(&loc);
			IIUGmsg(ERget(S_IE0019_Deleting_File), FALSE, 1,
					intfile);
		}

		/*
		** open up another transaction, since we'll be using
		** cursor selects. the database changes are atomic in
		** themselves; we're just doing some file copying and
		** corresponding updates to catalogs. if this fails, they
		** can still copy over files by hand
		*/
		IIUIlxbLabelXactBegin(&xactnum);

		/*
		** Don't bother with all of this unless we're copying source
		** or writing a list of files affected!
		*/
		if ( (allflags & COPYSRCFLAG) || (*listfile != EOS) ) 
		{
			IIIEcsf_CopySourceFiles(oldsrcdir, newsrcdir, applid, 
						(allflags & COPYSRCFLAG), 
						listfile, srclist);
		}

		IIUIlxeLabelXactEnd(xactnum);
	}
	else if (rval == CUQUIT)
	{
		IIUGerr(E_IE0005_Exitmsg, UG_ERR_FATAL, 0);
	}

	FEing_exit();

        EXdelete();

        PCexit(OK);

	return -1; /* Should never be reached */
}
