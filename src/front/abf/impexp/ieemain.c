/*
**	Copyright (c) 1993, 2004 Ingres Corporation
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
# include       <lo.h>
# include       <si.h>
# include       "erie.h"
# include       "ieimpexp.h"

/**
** Name:        ieemain.c -     IIExport Main Entry Point.
**
** Defines:
**      main()          - calls IIIEiie_IIExport(), the true main routine
**
** Input:
**      argc            {int}
**      argv[]          { char **}
**
** History:
**      23-jun-1993 (daver)
**              First Written (stolen from copyapp)
**      7-sep-1993 (daver)
**              Added -listing support, err message, comment cleanup, etc
**      22-dec-1993 (daver)
**              Gave username some space rather than a pointer, added
**              params to IIIEesp_ExtractStringParam(). Fixes bug 57924.
**      19-jan-1994 (daver)
**              Fixed Bug 57685. Added error handling to remove intfile
**              when no components specified on the command line exist.
**      15-feb-1994 (daver)
**              Fixed bug 59703; initialized the listfile variable.
**      23-mar-1994 (daver)
**              Removed obsolete library ABFTO60LIB from NEEDLIBS.
**      03/25/94 (dkh) - Added call to EXsetclient().
**      26-oct-1994 (slawrence) #60687
**              Remove the user specified (optional) .list file if non-OK 
**              is returned from IIIEiie_IIExport ().  
**      27-oct-1994 (slawrence) #60689
**              intfile and listing file names cannot be identical        
**      21-aug-1995 (morayf)
**		Initialised username array start to zero, since some
**		compilers allow any old rubbish to be here if the -user=XXXX
**		option is not used. This means FEingres() can fail.
**	18-jun-97 (hayke02)
**	    Add dummy calls to force inclusion of libruntime.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to build iiexport dynamically on both
**	    windows and unix.
*/

/*
**      MKMFIN Hints
**
PROGRAM =       iiexport

NEEDLIBS =      IMPEXPLIB COPYAPPLIB CAUTILLIB \
		COPYUTILLIB COPYFORMLIB SHINTERPLIB \
		SHFRAMELIB SHQLIB SHCOMPATLIB SHEMBEDLIB

UNDEFS = II_copyright
*/


FUNC_EXTERN OO_OBJECT **        IIAMgobGetIIamObjects();      

FUNC_EXTERN     STATUS  IIIEiie_IIExport();
FUNC_EXTERN     VOID    IIIEeum_ExportUsageMsg();
FUNC_EXTERN     VOID    IIIEepv_ExtractParamValue();
FUNC_EXTERN     VOID    IIIEesp_ExtractStringParam();


GLOBALDEF       bool    IEDebug = FALSE;

main(argc, argv)
i4      argc;
char    *argv[];
{
        EX_CONTEXT      context;
        STATUS          retstat;        /* Return status */

        EX      FEhandler();    /* interrupt handler for front-ends */

        char    *dbname = NULL;                 /* name of the database */
        char    *appname = NULL;                /* name of the applicaton */
        char    username[FE_MAXNAME + 3];       /* user for -u flag + "-u"*/

        /* 
        ** holds all params for passing. the param_list is an array of 
        ** pointers; the index correponds to the object type. if that
        ** object type (e.g., frame) is specified, than each name gets its
        ** own element in the list (e.g., -frame=a,b,c would mean that at
        ** array index EXP_FRAME there would be a list of 3 elements, "a",
        ** "b", and "c"). The complete array looks like:
        **      param_list[EXP_FRAME = 0] = "-frame=framename,otherframe,..."
        **      param_list[EXP_PROC = 1] = "-procedure=procname,otherproc,..."
        **      param_list[EXP_GLOB = 2] = "-global=globname,otherglob,..."
        **      param_list[EXP_CONST = 3] = "-const=constname,otherconst,..."
        **      param_list[EXP_REC = 4] = "-record=recname,otherrec,..."
        **      
        */
        IEPARAMS        *param_list[EXP_OBJECTS];

        i4              allflags = 0;           /* bit map shme */

        char            intfile[MAX_LOC + 1];   /* intermediate file name */
        FILE            *intfptr;
        LOCATION        intloc;
        char            *tmpintl;       /* 27-oct-1994 (SL)  #60689     */

        FILE            *listfptr = NULL;       /* initialize file pointer */
        char            listfile[MAX_LOC+1];    /* buffer for listfile loc */
        LOCATION        listloc;
        char            *tmplistl;      /* 27-oct-1994 (SL)  #60689     */

        i4              i;                      /* ubiquitous for-loop index */


        /* Tell EX this is an ingres tool. */
        (void) EXsetclient(EX_INGRES_TOOL);

        IEDebug = FALSE;
        STcopy( ERx("iiexport.tmp"), intfile );
        *listfile = EOS;

        /* first do all the standard setup stuff (stolen from copyapp) */

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
                IIUGerr(E_IE000C_EXCEPTION, UG_ERR_ERROR, 1, ERx("IIEXPORT"));
                PCexit(FAIL);
        }

        FEcopyright(ERx("IIEXPORT"), ERx("1993"));

        /*
        ** Now do the real work. 
        ** 
        ** Command looks like:
        ** 
        ** iiexport <dbname> <appname> <flags>
        **
        ** so lets pull out the dbname and appname, then loop thru flags
        */

        /*
        ** check the argc's. gotta have iiexport <db> <app> <something>
        ** so argc's gotta be at least 4. if its not, print the usage
        ** message and exit
        */
        if (argc <= 3)
        {
            IIUGerr(E_IE0012_Need_3_Args, UG_ERR_ERROR, 0);
            IIIEeum_ExportUsageMsg();
        }

        /* make sure first two args, dbname/appname, don't start with - */
        if (argv[1][0] == '-' || argv[2][0] == '-')
        {
            IIUGerr(E_IE000E_Flags_Before_DB_App, UG_ERR_ERROR, 0);
            IIIEeum_ExportUsageMsg();
        }

        /* dbname and appname will be found in there place on the cmd line */
        dbname = argv[1];
        appname = argv[2];

        /* before we flip through the flags, null out the param array */
        for (i = 0; i < EXP_OBJECTS; i++)
                param_list[i] = NULL;

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
                IIIEeum_ExportUsageMsg();
            }
            else
            {
                switch (argv[i][1])
                {
                  case 'a':
                        if (STcompare(argv[i], ERx("-allframes")) == 0)
                                allflags = allflags | ALLFRAMES;
                        else if (STcompare(argv[i], ERx("-allprocs")) == 0)
                                allflags = allflags | ALLPROCS;
                        else if (STcompare(argv[i], ERx("-allrecords")) == 0)
                                allflags = allflags | ALLRECS;
                        else if (STcompare(argv[i], ERx("-allglobals")) == 0)
                                allflags = allflags | ALLGLOBS;
                        else if (STcompare(argv[i], ERx("-allconstants")) == 0)
                                allflags = allflags | ALLCONSTS;
                        else
                        {
                                IIUGerr(E_IE0010_Illegal_flag, UG_ERR_ERROR,
                                        2, argv[i], ERx("IIEXPORT"));
                                IIIEeum_ExportUsageMsg();
                        }
                        break;
                    /*
                    ** anything other than the allshme flags has a specific
                    ** value attatched, like -frame=name,name or -user=uname
                    ** call IIIEepv_ExtractParamValue to place the value 
                    ** in the appropriate buffer (filling the param_arrary) 
                    ** for later processing
                    */
                    case 'f':
                        IIIEepv_ExtractParamValue(ERx("frame"), argv[i], 
                                                &param_list[EXP_FRAME]);
                        break;
                    case 'p':
                        IIIEepv_ExtractParamValue(ERx("proc"), argv[i], 
                                                &param_list[EXP_PROC]);
                        break;
                    case 'g':
                        IIIEepv_ExtractParamValue(ERx("global"), argv[i], 
                                                &param_list[EXP_GLOB]);
                        break;
                    case 'c':
                        IIIEepv_ExtractParamValue(ERx("constant"), argv[i], 
                                                &param_list[EXP_CONST]);
                        break;
                    case 'r':
                        IIIEepv_ExtractParamValue(ERx("record"), argv[i], 
                                                        &param_list[EXP_REC]);
                        break;
                    case 'i':
                        IIIEesp_ExtractStringParam(ERx("intfile"), argv[i], 
                                                        intfile, TRUE);
                        break;
                    case 'l':
                        IIIEesp_ExtractStringParam(ERx("listing"), argv[i], 
                                                        listfile, TRUE);
                        break;
                    case 'u':
                        IIIEesp_ExtractStringParam(ERx("user"), argv[i], 
                                                        username, TRUE);
                        break;
                    case 'd':
                        if (STcompare(ERx("-debug"), argv[i]) == 0)
                        {
                                /* set the global Debug flag */
                                IEDebug = TRUE;
                                break;
                        }
                    default:
                        IIUGerr(E_IE0010_Illegal_flag, UG_ERR_ERROR,
                                                2, argv[i], ERx("IIEXPORT"));
                        IIIEeum_ExportUsageMsg();
                        break;

                } /* end switch on 2nd char of each param */

            } /* end else 1st char is a '-' and might be a valid flag */

        } /* end for loop, flipping though each argv */

        /*
        ** copyapp didn't check the status return either; we'll get an error 
        ** from SIfopen if something went wrong, and issue the error then
        */
        LOfroms(PATH&FILENAME, intfile, &intloc);
                
        if ( SIfopen(&intloc, ERx("w"), SI_TXT, 512, &intfptr) != OK )
        {
                IIIEfoe_FileOpenError(&intloc, ERx("intermediate"), 
                                                TRUE, ERx("IIEXPORT"));
                IIUGerr(E_IE0005_Exitmsg, UG_ERR_FATAL, 0);
        }

        /* only do the list file if user specified one ... */
        if (*listfile != EOS)
        {
                LOfroms(PATH&FILENAME, listfile, &listloc);
/*      27-oct-1994  (SL)  #600689                                      */
                LOtos (&listloc, &tmplistl);
                LOtos (&intloc,  &tmpintl);
                if ( STcompare(tmplistl, tmpintl) == 0) 
                {
                        IIUGerr(E_IE0044_Dupl_File_Name, UG_ERR_ERROR, 0);
                        IIUGerr(E_IE0005_Exitmsg, UG_ERR_FATAL, 0);
                }
/*      27-oct-1994  (SL)  #600689                                      */
                if ( SIfopen(&listloc, ERx("w"), SI_TXT, 512, &listfptr) != OK )
                {
                        IIIEfoe_FileOpenError(&listloc, ERx("listing"), 
                                                TRUE, ERx("IIEXPORT"));
                        IIUGerr(E_IE0005_Exitmsg, UG_ERR_FATAL, 0);
                }
        }

        /* Crank up Ingres.  "" is used for the obsolete Xpipe flag */
        if ( FEingres(dbname, username, "", (char *)NULL) != OK )
        {
                IIUGerr( E_UG000F_NoDBconnect, UG_ERR_ERROR, 1, dbname );
                IIUGerr(E_IE0005_Exitmsg, UG_ERR_FATAL, 0);
        }

        /* initialize OO because CopyApp does */
        IIOOinit( IIAMgobGetIIamObjects() );             

        /* "Writing components from application `%0c' to file `%1c'" */
        IIUGmsg(ERget(S_IE0003_Writing_file), FALSE, 2, (PTR)appname,
                                                        (PTR)intfile);
        /*
        ** IIIEiie_IIExport is the routine which does the real work; 
        ** save the status for our exit status
        */
        retstat = IIIEiie_IIExport(appname, allflags, param_list, 
                                                        intfptr, listfptr);
        SIclose(intfptr);

        if (*listfile != EOS)
/* 26-oct-1994  SIclose(listfptr);      (SL) #60687                     */ 
/* 26-oct-1994  (SL)    #60687                                          */
        {
                SIclose(listfptr);                 
            if (retstat != OK)
            {
                LOfroms(FILENAME, listfile, &listloc);
                LOdelete(&listloc);
                IIUGmsg(ERget(S_IE0019_Deleting_File), FALSE, 1, listfile);
            }
        }
/* 26-oct-1994  (SL)    #60687                                          */
        
        /*
        ** remove the file if errors occured, or if nothing valid was
        ** specified on the command line
        */
        if (retstat != OK)
        {
                LOfroms(FILENAME, intfile, &intloc);
                LOdelete(&intloc);
                IIUGmsg(ERget(S_IE0019_Deleting_File), FALSE, 1, intfile);
        }

        FEing_exit();

        EXdelete();
        PCexit( retstat );
}

/* this function should not be called. Its purpose is to    */
/* force some linkers that can't handle circular references */
/* between libraries to include these functions so they     */
/* won't show up as undefined                               */
dummy1_to_force_inclusion()
{
    IItscroll();
    IItbsmode();
}
