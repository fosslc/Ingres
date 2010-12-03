/*
** Copyright (c) 1993, 2004 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <cm.h>
#include <me.h>
#include <nm.h>
#include <pc.h>
#include <si.h>
#include <st.h>
#include <odbccfg.h>
#include <idmseini.h>

/*
** Name: iiodbcinst.c
**
** Description:
**      This file is the source for the iiodbcinst utility.  The iiodbcinst
**      utility adds, removes, or modifies the unixODBC configuration
**      file, odbcinst.ini.
**
** History:
**      02-Jun-03 (loera01)
**          Created.
**      10-Jul-03 (loera01)
**          Cleaned up code and kept up with config API revisions.
**      21-jan-04 (loera01)
**          Added ming hints to remove necessity of having a MINGH file.
**      28-jan-04 (loera01)
**          Added port to VMS.
**      29-jan-04 (loera01)
**          Added "-batch" mode for silent installs.
**      09-Feb-04 (loera01)
**          Added display_err routine for more informative error messages.
**      25-Feb-04 (loera01) Bug 111863
**          Fixed up some compiler errors on Linux and made sure
**          the driver list wasn't freed unless allocated prior.
**      27-Feb-04 (loera01)
**          Added capability to specify custom driver names.
**      01-Feb-04 (loera01) Bug 111880
**          Added help display when "-h" or "-help" argument is specified.
**      07-Apr-04 (loera01) Bug 112118
**          Display valid values if "-m" is specified with an invalid
**          driver manager entry.
**	17-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	17-Jun-2004 (schka24)
**	    Safer env variable handling.
**      18-Jan-2006 (loera01) SIR 115615
**          Default vendor is now DRV_STR_INGRES_CORP.
**      24-Oct-2007 (Ralph Loen) Bug 119353
**          On VMS, ODBC libraries are now installed with the ii_installation
**          extension if not system-level.
**      25-Oct-2007 (Ralph Loen) Bug 119353
**          Use STstrindex() to find .EXE in the file image rather than a
**          series of calls to STindex().  Don't check for an 
**          ii_installation value of "AA".
**   27-Apr-2010 (Ralph Loen) SIR 123641
**          Remove reliance on ocfginfo.dat getDefaultInfo().  The alternate
**          driver name is now "Ingres XX", where XX is the installation code.
**          Replaced numeric array descriptions with the associated constants
**          for the default attribute array.  Thus, defAttr[0] becomes
**          defAttr[INFO_ATTR_DRIVER_FILE_NAME], etc.  VMS no longer needs
**          to get the II_INSTALLATION logical to append to the file names;
**          this is now done in getDefaultInfo(). 
**          Remove prompt for custom driver name.  This is no longer necessary.
**   15-Nov-2010 (stial01) SIR 124685 Prototype Cleanup
**          Changes to eliminate compiler prototype warnings.
*/

/*
NEEDLIBS = ODBCCFGLIB APILIB COMPATLIB CUFLIB GCFLIB ADFLIB MALLOCLIB

PROGRAM = (PROG1PRFX)odbcinst

DEST = utility
*/

# define MAXLINE 80

/*
** Internal configuration routines.
*/
static bool checkArgs(char *arg, i4 argc, char **argv, char *altPath, 
	char *drvMgr);
static bool writeMgrFile(char *altPath);
static i4 line_get( char *, char *, i4, char *); 
static i4 ocfg_getrec( char *, i4);
static void display_err(char *, STATUS);
static void display_help(void);

i4
main (int argc, char **argv) 
{
    STATUS status=OK;
    PTR drvH=NULL;
    char **list=NULL;
    ATTR_ID **attrList=NULL;
    char **defAttr=NULL;
    i4 count=0, attrCount = 0, retcount;
    i4 i;
    bool badCmdLine = FALSE, rmPkg = FALSE, has_default = FALSE, has_alt
        = FALSE, isReadOnly = FALSE, mgrSpecified = FALSE, batch = FALSE,
	def_or_alt=FALSE;
    i4 pathType = OCFG_SYS_PATH; 
    char altPath[OCFG_MAX_STRLEN] = "\0";
    char drvMgr[OCFG_MAX_STRLEN] = "\0";
    char line[MAXLINE],prompt[MAXLINE];
    char driverPath[OCFG_MAX_STRLEN];
    char *dirPath;
    char driverName[OCFG_MAX_STRLEN];
    char *ii_installation;
    char *p = NULL;
    char custName[OCFG_MAX_STRLEN];
    char etxt[512];

    ATTR_ID driverList[6] = 
    {
        { DRV_ATTR_DRIVER,          "\0" },
        { DRV_ATTR_DRIVER_ODBC_VER, DRV_STR_VERSION },
        { DRV_ATTR_DRIVER_READONLY, DRV_STR_N },
        { DRV_ATTR_DRIVER_TYPE,     DRV_STR_INGRES },
        { DRV_ATTR_VENDOR,          DRV_STR_INGRES_CORP },
        { DRV_ATTR_DONT_DLCLOSE,    DRV_STR_ONE }
    };
    int drvCount = sizeof(driverList) / sizeof(driverList[0]);

    ATTR_ID **drvList = (ATTR_ID **)MEreqmem( (u_i2) 0, 
	(u_i4)drvCount, TRUE, (STATUS *) NULL);

    for (i = 0; i < drvCount; i++)
    {
        drvList[i] = (ATTR_ID *)MEreqmem( (u_i2) 0, 
	    (u_i4)sizeof(ATTR_ID), TRUE, (STATUS *) NULL);
        drvList[i]->id = driverList[i].id;
        drvList[i]->value = (char *)MEreqmem( (u_i2) 0, 
    	    (u_i4)OCFG_MAX_STRLEN, TRUE, (STATUS *) NULL);
        STcopy(driverList[i].value, drvList[i]->value);
    }

    if ( argc > 7 )
        badCmdLine = TRUE;

    /*
    ** Parse the command line.  Reject invalid arguments.
    */
    if ( !badCmdLine && argc > 1 )
    {
	if (checkArgs("-h", argc, argv, NULL, NULL ) )
	{
	   display_help();
           PCexit(0);	
	}
	if (checkArgs("-help", argc, argv, NULL, NULL ) )
	{
	   display_help();
           PCexit(0);	
	}
        if ( checkArgs("-batch", argc, argv, NULL, NULL ) )
            batch = TRUE;
        if ( checkArgs("-rmpkg", argc, argv, NULL, NULL ) )
            rmPkg = TRUE;
        if (checkArgs("-r", argc, argv, NULL, NULL ) )
            isReadOnly = TRUE;
        if ( checkArgs("-p", argc, argv, altPath, NULL ) )
            pathType = OCFG_ALT_PATH;
        if ( checkArgs("-m", argc, argv, NULL, drvMgr ) )
			mgrSpecified = TRUE;
    }

    /*
    ** Set driver manager according to user input.  Default is unixODBC.
    */
    if (mgrSpecified && !STbcompare( drvMgr, 0, MGR_STR_CAI_PT, 0, TRUE ) )
        driverManager = MGR_VALUE_CAI_PT;
    else
	driverManager = MGR_VALUE_UNIX_ODBC;

    /*
    ** If none of the arguments are recognized, flag an error.
    */
    if ( !batch && !rmPkg && !isReadOnly && pathType != OCFG_ALT_PATH && !mgrSpecified && argc > 1)
        badCmdLine = TRUE;

    if ( badCmdLine )
    {
        display_help();
        PCexit(FAIL);
    }
    
    if ( isReadOnly )
        STcopy(DRV_STR_Y, drvList[2]->value); 
    else
        STcopy(DRV_STR_N, drvList[2]->value); 

    if ( !writeMgrFile( altPath ) )
    {
        SIprintf("Aborting due to error writing %s/files/%s\n",
           SYSTEM_LOCATION_SUBDIRECTORY, MGR_STR_FILE_NAME); 
        PCexit(FAIL);
    }
    defAttr = getDefaultInfo(); 
    if (defAttr == NULL)
    {
        SIprintf("Aborting due to error reading %s/install/%s\n",
            SYSTEM_LOCATION_VARIABLE, INFO_STR_FILE_NAME);
        PCexit(FAIL);
    }

    /*
    ** Get the path of the driver library and create the path/libname string.
    */
    NMgtAt(SYSTEM_LOCATION_VARIABLE,&dirPath);  /* usually II_SYSTEM */
    if (dirPath != NULL && *dirPath)
        STlcopy(dirPath,driverPath,sizeof(driverPath)-20-1);
    else
    {
        SIprintf("Error--%s is not defined\n",SYSTEM_LOCATION_VARIABLE);
        PCexit(FAIL);
    }
# ifdef VMS
    STcat(driverPath,"[");
    STcat(driverPath,SYSTEM_LOCATION_SUBDIRECTORY);  /* usually "ingres"  */
    STcat(driverPath,".library]");
    if ( isReadOnly )
        STcat(driverPath,defAttr[INFO_ATTR_RONLY_DRV_FNAME]);
    else
        STcat(driverPath,defAttr[INFO_ATTR_DRIVER_FILE_NAME]);
# else
    STcat(driverPath,"/");
    STcat(driverPath,SYSTEM_LOCATION_SUBDIRECTORY);  /* usually "ingres"  */
    STcat(driverPath,"/lib/");
    if ( isReadOnly )
        STcat(driverPath,defAttr[INFO_ATTR_RONLY_DRV_FNAME]);
    else
        STcat(driverPath,defAttr[INFO_ATTR_DRIVER_FILE_NAME]);
# endif /* ifdef VMS */
    STcopy(driverPath,drvList[0]->value);
    /*
    ** Initialize the cache from the odbcinst.ini file.
    */
    openConfig(NULL, pathType, altPath, &drvH, &status);
    if (status != OK)
    {
        STprintf(etxt,"Could not open from path %s.\n",
            pathType == OCFG_ALT_PATH ? altPath : "/usr/local/etc");
        display_err(etxt,status);
        PCexit(FAIL);
    }
    if (rmPkg)
    {
        delConfigEntry( drvH, defAttr[INFO_ATTR_DRIVER_NAME], &status );
        delConfigEntry( drvH, defAttr[INFO_ATTR_ALT_DRIVER_NAME], &status );
        closeConfig(drvH, &status);
        PCexit(OK);
    }
    /*
    ** Get the driver count.
    */
    retcount = listConfig( drvH, count, list, &status );
    if (status != OK)
    {
        STprintf(etxt,"Could not list drivers.\n");
        display_err(etxt,status);
        PCexit(FAIL);
    }
    count = retcount;
    if (count)
    {
        /*
        ** Get the list of recognized drivers.
        */
	list = (char **)MEreqmem( (u_i2) 0, 
	    (u_i4)(count * sizeof(char *)), TRUE, 
	    (STATUS *) NULL);
	for (i = 0; i < count; i++)
	    list[i] =  (char *)MEreqmem( (u_i2) 0, (u_i4)OCFG_MAX_STRLEN, 
	        TRUE, (STATUS *) NULL);
        listConfig( drvH, count, list, &status );
        if (status != OK)
        {
            STprintf(etxt,"Could not list drivers.\n");
            display_err(etxt,status);
            PCexit(FAIL);
        }
    }
    for (i = 0; i < count; i++)
    {
        def_or_alt = FALSE;
	if (!has_default)
	{
            has_default = STbcompare( list[i], 0, 
                defAttr[INFO_ATTR_DRIVER_NAME], 0, TRUE ) 
                   == 0 ? TRUE : FALSE;
            if (has_default)
                def_or_alt = TRUE; 
        }
        if (!has_alt)
        {
            has_alt = STbcompare( list[i], 0, 
                defAttr[INFO_ATTR_ALT_DRIVER_NAME], 0, 
                   TRUE ) == 0 ? TRUE : FALSE;
            if (has_alt)
                def_or_alt = TRUE; 
        }
        if (def_or_alt)
        {
            if ( !batch )
	    {
                STprintf(prompt,
         "\tInstallation has pre-defined version of %s.\n\tOverwrite? (Yes/No)", 
		list[i]);
                if ( line_get( prompt, "", FALSE, line ) == EOF )
                {
                    SIprintf("ODBC driver installation safely aborted\n");
                    PCexit(OK);
                }
                STzapblank(line,line);
                if ( line[0] != 'y' && line [0] != 'Y' )
                {
                    SIprintf( "%s left untouched\n", list[i] );
		    continue; 
                }
            } /* if (!batch) */
            STcopy(list[i],driverName);
            setConfigEntry( drvH, driverName, drvCount, drvList, &status );
            if (status != OK)
            {
                STprintf(etxt,"Could not write driver entry.\n");
                display_err(etxt,status);
                PCexit(FAIL);
            }
        } /* if (has_default || has_alt) */ 
        else if ( !batch )
        {
                STprintf(prompt,
         "\tInstallation has custom version of %s.\n\tDelete? (Yes/No)", 
		list[i]);
                if ( line_get( prompt, "", FALSE, line ) == EOF )
                {
                    SIprintf("ODBC driver installation safely aborted\n");
                    PCexit(OK);
                }
                STzapblank(line,line);
                if ( line[0] != 'y' && line [0] != 'Y' )
                {
                    SIprintf( "%s left untouched\n", list[i] );
                    continue;
                }
            STcopy(list[i],driverName);
            delConfigEntry( drvH, driverName, &status );
            if (status != OK)
            {
                STprintf(etxt,"Could not write driver entry.\n");
                display_err(etxt,status);
                PCexit(FAIL);
            } 
        } /* if (!batch) && !has_default */
    } /* for (count... */
    if (!has_default) 
    {
        setConfigEntry( drvH, defAttr[INFO_ATTR_DRIVER_NAME], drvCount,
	    drvList, &status );
        if (status != OK)
        {
            STprintf(etxt,"Could not write driver entry.\n");
            display_err(etxt,status);
            PCexit(FAIL);
        }
    }
    if (!has_alt) 
    {
        setConfigEntry( drvH, defAttr[INFO_ATTR_ALT_DRIVER_NAME], drvCount,
	    drvList, &status );
        if (status != OK)
        {
            STprintf(etxt,"Could not write driver entry.\n");
            display_err(etxt,status);
            PCexit(FAIL);
        }
    }
    closeConfig(drvH, &status);
    if (status != OK)
    {
        STprintf(etxt,"Could not close driver info.\n");
        display_err(etxt,status);
        PCexit(FAIL);
    }
    if (count)
    {
        for (i = 0; i < count; i++)
	    MEfree((PTR)list[i]);
        MEfree((PTR)list);
    }
    for (i = 0; i < drvCount; i++)
    {
        MEfree((PTR)drvList[i]->value);
        MEfree((PTR)drvList[i]);
    }
    MEfree((PTR)drvList);

    PCexit(OK);
}

/*
** Name: line_get
**
** Description:
**      This function displays a prompt and accepts input from the 
**      command line.  Adapted from gcn_line_get() in netu.
**
** Return value:
**      status              Error number or OK.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      prompt             Display string.
**      def_line           Default value.
**      wild_flag          If true, accepts null input. 
**      in_line            Returned input line.
**
** History:
**      03-Jun-03 (loera01)
**          Created.
*/

static i4
line_get( char *prompt, 
	      char *def_line, i4  wild_flag, char *in_line ) 
{
	char    line[MAXLINE];
	char	ret = '\n';
	char	*p;
	i4	status = EOF;

	for(;;)
	{
		STATUS s;

		SIprintf(prompt);
		SIprintf( *def_line ? "(%s): " : ": ", def_line );

/* This is somewhat unusuall but does force the SIflush to work */
		SIflush(stdout);

	        s = ocfg_getrec( line, (i4)MAXLINE );

		if( s != OK || line[0] == '\033' )
			break;

		if (p = STindex(line, &ret, MAXLINE))
			*p = EOS;

		if( *def_line && !*line )
		{
			STcopy( def_line, in_line );
			SIprintf("\t\tDefault value: %s\n",in_line);
		}
		else
		{
			STcopy(line,in_line);
		}

	        STzapblank(in_line,in_line);

		if( !*in_line )
		{
			SIprintf("\t\tValue required, enter <ESC> to exit.\n");
			continue;
		}

		if(!STbcompare( in_line, 0, "*",0, TRUE ) )
		{
			if( !wild_flag )
			{
				SIprintf("\t\tValue required, enter <ESC> to exit.\n");
				continue;
			}
			in_line[0] = EOS;
		}

		status = OK;
		break;		/* There is valid data */
	}	/* end of FOR loop */
	return status;
}

/*
** Name: ocfg_getrec
**
** Description:
**      This function reads the standard input.  Adapted from gcn_getrec()
**      in netu.
**
** Return value:
**      status              Error number or OK.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      buf                 Returned input line.
**      len                 Requested length.
**
** History:
**      03-Jun-03 (loera01)
**          Created.
*/

static i4
ocfg_getrec( char *buf, i4  len )
{
        return SIgetrec( buf, (i4)len, stdin );
}

/*
** Name: checkArgs
**
** Description:
**      This function validates and formats command line arguments.
**
** Return value:
**      isValid            True if valid argument, false otherwise.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      arg               Argument to check.
**      argc              Number of command-line arguments.
**      argv              Array in command-line arguments.
**      altPath           Returned alternate path for "-p" argument.
**
** History:
**      03-Jun-03 (loera01)
**          Created.
*/

static bool
checkArgs (char *arg, i4 argc, char **argv, char *altPath, 
    char * drvMgr)
{
    i4 i;
    
    for ( i = 1; i < argc; i++ )
    {
        if ( !STbcompare( arg, 0, argv[i], 0, TRUE ) && 
             !STbcompare( arg, 0, "-p", 0, TRUE ) && i < (argc - 1) )
        {
	    i++;
            STcopy( argv[i], altPath );
            return TRUE; 
        }
        if ( !STbcompare( arg, 0, argv[i], 0, TRUE ) && 
             !STbcompare( arg, 0, "-m", 0, TRUE ) && i < (argc - 1) )
        {
	    i++;
            if ( !STbcompare( MGR_STR_UNIX_ODBC, 0, argv[i], 0, TRUE ) )
            {				  
                STcopy( argv[i], drvMgr );
                return TRUE; 
            }
            else if ( !STbcompare( MGR_STR_CAI_PT, 0, argv[i], 0, TRUE ) )
            {				  
                STcopy( argv[i], drvMgr );
                return TRUE; 
            }
            else
            {
                display_help();
		PCexit(FAIL);
            }
        }
        if ( !STbcompare( arg, 0, argv[i], 0, TRUE ) &&
            !STbcompare ( arg, 0, "-r", 0, TRUE ) )
            return TRUE;
        if ( !STbcompare( arg, 0, argv[i], 0, TRUE ) &&
            !STbcompare ( arg, 0, "-rmpkg", 0, TRUE ) )
            return TRUE;
        if ( !STbcompare( arg, 0, argv[i], 0, TRUE ) &&
            !STbcompare ( arg, 0, "-batch", 0, TRUE ) )
            return TRUE;
        if ( !STbcompare( arg, 0, argv[i], 0, TRUE ) &&
            !STbcompare ( arg, 0, "-h", 0, TRUE ) )
            return TRUE;
        if ( !STbcompare( arg, 0, argv[i], 0, TRUE ) &&
            !STbcompare ( arg, 0, "-help", 0, TRUE ) )
            return TRUE;
    }
    return FALSE;
}

/*
** Name: writeMgrFile
**
** Description:
**      This function writes "alternate path" configuration file, 
**      $II_SYSTEM/ingres/files/odbcpath.dat.
**
** Return value:
**      isOK               True if successfully written, otherwise false.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      altPath            Path of the odbcpath.dat file.
**
** History:
**      03-Jun-03 (loera01)
**          Created.
*/

static bool
writeMgrFile( char *altPath )
{
    char *dirPath;
    char fullPath[OCFG_MAX_STRLEN];
    FILE *pfile;
    LOCATION loc;
    STATUS status;

# ifdef VMS
    NMgtAt(SYSTEM_LOCATION_VARIABLE,&dirPath);  /* usually II_SYSTEM */
    if (dirPath != NULL && *dirPath)
        STlcopy(dirPath,fullPath,sizeof(fullPath)-20-1);
    else
        return (FALSE);
    STcat(fullPath,"[");
    STcat(fullPath,SYSTEM_LOCATION_SUBDIRECTORY);  /* usually "ingres"  */
    STcat(fullPath,".files]");
    STcat(fullPath,MGR_STR_FILE_NAME);
# else
    NMgtAt(SYSTEM_LOCATION_VARIABLE,&dirPath);  /* usually II_SYSTEM */
    if (dirPath != NULL && *dirPath)
        STlcopy(dirPath,fullPath,sizeof(fullPath)-20-1);
    else
        return (FALSE);
    STcat(fullPath,"/");
    STcat(fullPath,SYSTEM_LOCATION_SUBDIRECTORY);  /* usually "ingres"  */
    STcat(fullPath,"/files/");
    STcat(fullPath,MGR_STR_FILE_NAME);
# endif /* ifdef VMS */

    if ( LOfroms(PATH & FILENAME, fullPath, &loc) != OK )
       return FALSE;

    if (SIfopen(&loc, "w", (i4)SI_TXT, OCFG_MAX_STRLEN, &pfile) != OK)
        return FALSE;

    if (altPath && *altPath)
        SIfprintf(pfile,"%s=%s\n",MGR_STR_PATH,altPath);
    SIfprintf(pfile,"%s=%s\n",MGR_STR_MANAGER,
	driverManager == MGR_VALUE_UNIX_ODBC ? 
        MGR_STR_UNIX_ODBC : MGR_STR_CAI_PT);
    return TRUE;
}
/*
** Name: display_err
**
** Description:
**      This function displays the error message associated with the
**      status code, if known.
**
** Return value:
**      void.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      etxt            Error message string.
**      status          error number.
**
** History:
**      09-Feb-04 (loera01)
**          Created.
*/

static void
display_err(char *etxt, STATUS status)
{
    if (OK != ERreport(status, &etxt[STlength(etxt)]))
        STprintf(&etxt[STlength(etxt)],
	    "(Reason unknown)", status);
    SIprintf("%s\n",etxt);
}

/*
** Name: display_help
**
** Description:
**      This function displays "help" information.
**
** Return value:
**      void.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      None.
**
** History:
**      01-Mar-04 (loera01)
**          Created.
*/
static void
display_help()
{
    SIprintf("%sodbcinst help\n---------------\n", 
	SYSTEM_CFG_PREFIX);
    SIprintf("Usage: %sodbcinst [-batch ] [ -m drvMgr ] [ -p altPath ] [ -rmpkg ] [ -r ]\n", SYSTEM_CFG_PREFIX); 
    SIprintf("Arguments:\n");
    SIprintf("-batch\t\tRun silently, without prompts\n");
    SIprintf("-m driverMgr\tDriver Manager.  Must be either \"unixODBC\" or \"CAI/PT\"\n");
    SIprintf("-p altPath\tAlternate installation path for odbcinst.ini\n");
    SIprintf("-rmpkg\t\tRemove Ingres driver definition from odbcinst.ini\n");
    SIprintf("-r\t\tSpecify read-only driver as default\n");
    SIprintf("-h or -help\tDisplay this help page\n");
}
