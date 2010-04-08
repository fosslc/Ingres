/*
** Copyright (c) 2008 Ingres Corporation
*/

#include	<compat.h>
#include        <er.h>
#include        <dl.h>
#include	<si.h>
#include	<st.h>
#include	<me.h>
#include	<gc.h>
#include	<ex.h>
#include        <er.h>
#include        <id.h>
#include	<cm.h>
#include	<gl.h>
#include	<sl.h>
#include	<tr.h>
#include	<iicommon.h>
#include        <sql.h>
#include        <sqlext.h>
#include        <iiodbc.h>
#include        <idmseini.h> 
#include        <odbccfg.h>
#include        <iiodbcfn.h>
#include        <fe.h>
#include        <ug.h>
#include        <uf.h>
#include        <ui.h>
#include        <pm.h>
#include        <pc.h>
#include        <cv.h>
#include        <nm.h>
#include	<iplist.h>
#include	<erst.h>
#include        <uigdata.h>
#include	<stdprmpt.h>
#include	<gca.h>
#include	<gcn.h>

/*
**  Name: iiodbcadmin -- network configuration maintenance program
**
**  Description:
**
**      The Ingres ODBC administrator program, iiodbcadmin, allows users
**      to create and modify ODBC data source definitions.  These definitions
**      may reside in the home directory's .odbc.ini file, the system-level 
**      odbc.ini file, or an alternate odbc.ini file as specified.  The
**      ODBCINI and ODBCSYSINI environment variables are recognized.
**       
**      Driver manager files, which typically reside in odbcinst.ini, are
**      not modified by the ODBC administrator.  This is done by the
**      iiodbcinst utility or iisuodbc configuration script.  However, the
**      Ingres ODBC administrator does allow users to view relevant driver
**      manager information such as the driver name and version.
**
**  Entry points:
**	This file contains no externally visible functions.
**
**  History:
**	05-jun-03 (loera01)
**	    Created.
**      22-jan-04 (loera01)
**          Added ming hints so that MINGH is not needed.
**      23-jan-04 (loera01)
**          Added test capability to main menu.
**      23-jan-04 (loera01)
**          Replaced "readonly" variable with "ronly" for VMS compiler.
**      29-jan-04 (loera01)
**          Revised getFileToken() with extra argument.
**      10-feb-04 (loera01)
**          Added content to error messages.
**      18-jul-05 (loera01) Bug 114869
**          In dsn_config_form(), added support for groups.
**	16-aug-2005 (abbjo03)
**	    Fix 18-jul change:  use dsngroup instead of group for field name.
**      20-jan-2006 (loera01) SIR 115615
**          Make "Ingres Corporation" the default vendor string.
**    25-Oct-2005 (hanje04)
**        Add prototype for copyValue() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**    16-mar-2006 (Ralph Loen) Bug 115833
**        Added support for "fill character" for failed Unicode/multibyte
**        conversions. 
**      13-Mar-2007 (Ralph Loen) SIR 117786
**        Added support for ODBC CLI connection pool time outs.
**    02-oct-2007 (Ralph Loen) Bug 119238
**        In dsn_config_form(), don't attempt to update the form if the
**        "server type" result from IIFDlpListPick() is -1.  A return of
**        -1 means the user has cancelled the list pick display.
**    02-oct-2007 (Ralph Loen) Bug 119304
**        In top_form(), don't allow invocation of test_connection() if
**        the DSN count is zero.  De-activate the Edit, Destroy, and
**        Test menu options if the DSN count is zero.
**    19-oct-2007 (Ralph Loen) Bug 119329
**        In test_connection(), use the CLI instead of the API to test
**        connections.  In get_driver_version(), used new ODBC driver
**        routine getDriverVersion() to get the driver version.
**    24-oct-2007 (Ralph Loen) Bug 119296
**        Changed "iiodbcadmn" to "iiodbcadmin" to match the Ingres
**        documentation.
**    10-dec-2007 (Ralph Loen) Bug 119329
**        In inst_drivers_form(), bracket allocHandles() and freeHandles() 
**        around the getTimeout() function.
**    24-June-2008 (Ralph Loen) Bug 120551
**        Removed support for obsolete server types such as
**        ALB, OPINGDT, etc.
**    25-June-2008 (Ralph Loen) Bug 120551
**        Re-added VANT (CA-Vantage).
**   14-Nov-2008 (Ralph Loen) SIR 121228
**      Add support for OPT_INGRESDATE configuration parameter.  If set,
**      ODBC date escape syntax is converted to ingresdate syntax, and
**      parameters converted to IIAPI_DATE_TYPE are converted instead to
**      IIAPI_DTE_TYPE.  The new attributes DSN_ATTR_INGRESDATE
**      and DSN_STR_INGRESDATE support OPT_INGRESDATE on non-Windows
**      platforms.
**   24-Nov-2009 (frima01) Bug 122490
**      Added include of cv.h and nm.h to eliminate gcc 4.3 warnings.
**   26-Feb-2009 (Ralph Loen) SIR 121702
**     Add support for OPT_FETCHWHOLELOB configuration parameter.  If set,
**     each invocation of SQLFetch() fetches the entire blob, instead of
**     the first segment.
**   29-Jan-2010 (Ralph Loen) Bug 123175
**      Rename advanced attribute "Coerce to Ingres date syntax" to
**      "Send date/time as Ingres date".
**   08-Feb-2010 (Ralph Loen) Bug 123175
**      Removed "Fetch entire long data columns", which is supported in
**      earlier code lines only.
*/
/*
PROGRAM = (PROG1PRFX)odbcadmin

NEEDLIBS = INSTALLLIB \
	   UFLIB RUNSYSLIB RUNTIMELIB FDLIB FTLIB FEDSLIB \
           UILIB LIBQSYSLIB LIBQLIB UGLIB FMTLIB AFELIB \
           APILIB LIBQGCALIB CUFLIB SQLCALIB GCFLIB ADFLIB \
	   ODBCCFGLIB SQLCALIB COMPATLIB MALLOCLIB

UNDEFS =        II_copyright
*/
 
# define MAX_TXT 80
# define SYSTEM_DRV 0
# define ALTERNATE_DRV 1
# define USER_DSN 0
# define SYSTEM_DSN 1
# define ALTERNATE_DSN 2
# define USER "USER"
# define SYSTEM "SYSTEM"
# define ALTERNATE "ALTERNATE"
# define ODBCSYSINI "/usr/local/etc"
# ifdef VMS
# define ODBCINI ODBC_INI
# else
# define ODBCINI ".odbc.ini"
# endif /* VMS */
# define MISSING_DRIVER_MSG "No Driver information. Set driver path in \"Driver\" menu option.\n"

exec sql include sqlca;
exec sql include sqlda;
 
exec sql begin declare section;
    char local_username[MAX_TXT]; 
    char *username = &local_username[0];
    char topFormName[ MAX_TXT ];
    char altDsnPathsFormName[ MAX_TXT ];
    char altDrvPathsFormName[ MAX_TXT ];
    char instDriversFormName[ MAX_TXT ];
    char tracingFormName[ MAX_TXT ];
    char timeoutFormName[ MAX_TXT ];
    char detailsFormName[ MAX_TXT ];
    char dsnConfigFormName[ MAX_TXT ];
    char drvNameFormName[ MAX_TXT ];
    char dsnNameFormName[ MAX_TXT ];
    char advancedFormName[ MAX_TXT ];
exec sql end declare section;

/*
**  Local functions...
*/

char *getDrvTypeString(i4);
char *getDsnTypeString(i4);
char *get_driver_version( char *);
char **getDefaultInfo();
STATUS display_dsn(PTR, i4 *);
STATUS display_drv(i2, char *);
void display_err(char *errMsg, STATUS status);
static void top_form();
i4 dsnpath_form(char *);
char *alt_dsnpath_form();
i4 drvpath_form(char *);
char *alt_drvpath_form();
void inst_drivers_form(i4 *, i4 *, char *);
char * sel_drivers_form(PTR);
void details_form(char *, PTR, bool, i4);
bool tracing_form(PTR);
bool timeout_form(i4 *);
bool destroy_dsn_form(char *);
bool save_dsn_form(char *);
void dsn_config_form(char *, char *, PTR, bool, i4);
bool dsn_name_form(char *, char *, PTR, PTR);
bool advanced_form(ATTR_ID **, i4, i4);
void display_message( char *text );
void test_connection (char *);
void copyValue (ATTR_ID **, i4, i4, char *);
STATUS allocHandles(i2 pathType, char * altPath, PTR *drvH, PTR *dsnH);
STATUS freeHandles( PTR drvH, PTR dsnH);

char msg[80];
char errMsg[9512];
PTR envHandle;

i4
main(i4 argc, char *argv[])
{
    EX_CONTEXT	context;
    EX		FEhandler();
    STATUS      status = OK;
    CL_ERR_DESC     err_cl;

    /* Tell EX this is an ingres tool. */
    (void) EXsetclient(EX_INGRES_TOOL);

    MEadvise( ME_INGRES_ALLOC );  /* We'll use the Ingres malloc library */

    if ( EXdeclare(FEhandler, &context) )  
    {
        EXdelete();                             
        PCexit(FAIL);
    }

    /*
    **  We'll be running in interactive mode; fire up the forms system...
    */

    if (FEforms() != OK)
    {
        SIflush(stderr);
        PCexit(FAIL);
    }

    /* Disable potentially dangerous features, and annoying "Out of Data" */
    exec frs set_frs frs ( 
        shell = 0,
        editor = 0,
        outofdatamessage = 0 );

    top_form();    /* Main processing */
	 
    FEendforms();

    EXdelete();
    PCexit(OK);
}

/*
**  top_form()
**
**  Top-level processing for the main form.
*/

static void
top_form()
{
    exec sql begin declare section;
        char dsnname[MAX_TXT];
    exec sql end declare section;

    char drivername[MAX_TXT];

    PTR drvH, dsnH;
    char *odbcsysini;
    char *odbcini;
    bool sysIniDefined = FALSE;
    char altPath[OCFG_MAX_STRLEN]="\0";
    char *tmpPath = getMgrInfo(&sysIniDefined);
    char newPath[OCFG_MAX_STRLEN] = "\0";
    char vnode[OCFG_MAX_STRLEN] = "\0";
    char serverClass[OCFG_MAX_STRLEN] = "\0";
    char database[OCFG_MAX_STRLEN] = "\0";
    i4 i,j,attrCount,count;
    i4 dsnDisplayType = SYSTEM_DSN;
    i4 dsnType;
    STATUS status = OK, dsnStatus = OK, drvStatus = OK;
    bool formReturn=FALSE;
    i4 drvDisplayType;
    i4 drvType;

    if (tmpPath && *tmpPath)
        STcopy(tmpPath, altPath);
    /*
    **  Display main form...
    */
    STcopy(ERx("odmainmenu"), topFormName);
    if (IIUFgtfGetForm(IIUFlcfLocateForm(), topFormName) != OK)
        PCexit(FAIL);

    exec frs inittable  :topFormName dsndef read; 

    /*
    ** Initialize the driver info.  If ODBCSYSINI is defined, treat as
    ** the system path.  The openConfig routine gives precedence to ODBCSYSINI
    ** before odbcmgr.dat.
    */
    if (altPath && *altPath && !sysIniDefined)
    {
	drvType = dsnType = OCFG_ALT_PATH;
	drvDisplayType = ALTERNATE_DRV;
        drvStatus = allocHandles(OCFG_ALT_PATH, altPath, &drvH, &dsnH);
	dsnDisplayType = ALTERNATE_DSN;
    }
    else
    {
	drvType = dsnType = OCFG_SYS_PATH;
	drvDisplayType = SYSTEM_DRV;
        drvStatus = allocHandles(OCFG_SYS_PATH, NULL, &drvH, &dsnH);
	dsnDisplayType = SYSTEM_DSN;
    }
    /*
    **  Beginning of display loop.
    */
    exec frs display :topFormName;

    exec frs initialize;
    exec frs begin;
    {
        exec sql begin declare section;
	    char userinfo_txt[MAX_TXT];
	exec sql end declare section;

	IDname(&username);  
	STprintf(userinfo_txt, "%s: %s", 
	    ERget(S_ST0037_user_tag), username);
        exec frs putform :topFormName (userinfo=:userinfo_txt,
	         path=:ERx(getDrvTypeString(drvDisplayType)));

        if (drvStatus != OK)
        {
            STprintf(errMsg, "Cannot initialize ODBC driver info.\n");
	    display_err(errMsg, drvStatus);
            PCexit(FAIL);
        }

        /*
        ** Initialize the data source info.
        */
        if (drvStatus == OK)
        {
	    dsnStatus = display_dsn(dsnH, &count);
            if (dsnStatus != OK)
	    {
                STprintf(errMsg, "Cannot initialize ODBC DSN info.\n");
	        display_err(errMsg, dsnStatus);
    	    }
            dsnStatus = freeHandles(drvH, dsnH);
        }
    } /* exec frs begin (top_form) */
    exec frs end;
    exec frs activate menuitem :ERget( FE_Create );
    exec frs begin;
    {
	 dsnname[0] = '\0';
         drivername[0] = '\0';
         dsnStatus = allocHandles(dsnType, altPath, &drvH, &dsnH);
         if (dsnStatus == OK)
         {
    	     if (dsn_name_form(dsnname, drivername, drvH, dsnH)) 
             {
	         dsn_config_form(dsnname, drivername, dsnH, TRUE, 
                     dsnDisplayType);
	         dsnStatus = display_dsn(dsnH, &count);
                 if (dsnStatus != OK)
	         {
                     STprintf(errMsg, "Cannot initialize ODBC DSN info.\n");
	             display_err(errMsg, dsnStatus);
    	         }
                 dsnStatus = freeHandles(drvH, dsnH);
            }
        } 
    }
    exec frs end;
    exec frs activate menuitem :ERget( FE_Destroy );
    exec frs begin;
    {
	if (dsnStatus == OK && count)
	{
	    exec frs getrow :topFormName dsndef (:dsnname=dsnname);
	    formReturn = destroy_dsn_form(dsnname);
	    if (formReturn == TRUE)
	    {
                dsnStatus = allocHandles(dsnType, altPath, &drvH, &dsnH);
	        if (dsnStatus == OK)
                    delConfigEntry( dsnH, dsnname, &dsnStatus );
                if (dsnStatus == OK)
                    dsnStatus = freeHandles(drvH, dsnH);
                if (dsnStatus != OK)
	        {
                    STprintf(errMsg, "Cannot delete ODBC DSN info.\n");
	            display_err(errMsg, dsnStatus);
    	        }
		else
		{
                    dsnStatus = allocHandles(dsnType, altPath, &drvH, &dsnH);
                    if (dsnStatus == OK)
	                dsnStatus = display_dsn(dsnH, &count);
                    if (dsnStatus == OK)
                        dsnStatus = freeHandles(drvH, dsnH);
                    if (dsnStatus != OK)
	            {
                        STprintf(errMsg, "Cannot initialize ODBC DSN info.\n");
	                display_err(errMsg, dsnStatus);
    	            }
		} /* if dsnStatus != OK */
            } /* if formReturn == TRUE */
        } /* if (dsnStatus == OK && count) */
    } /* exec frs begin */
    exec frs end;
    exec frs activate menuitem :ERget( FE_Edit );
    exec frs begin;
    { 
	if (dsnStatus == OK && count)
        {
            exec frs getrow :topFormName dsndef (:dsnname=dsnname);
            dsnStatus = allocHandles(dsnType, altPath, &drvH, &dsnH);
	    if (dsnStatus == OK)
            {
	        dsn_config_form(dsnname, NULL, dsnH, FALSE, dsnDisplayType);
                dsnStatus = display_dsn(dsnH, &count);
            }
	    if (dsnStatus == OK)
                dsnStatus = freeHandles(drvH, dsnH);
            if (dsnStatus != OK)
	    {
                STprintf(errMsg, "Cannot initialize ODBC DSN info.\n");
	        display_err(errMsg, dsnStatus);
    	    }
	}
    }
    exec frs end;
    exec frs activate menuitem :ERget( FE_Test );
    exec frs begin;
	if (count)
	{
            exec frs getrow :topFormName dsndef (:dsnname=dsnname);
            test_connection(dsnname);
        }
    exec frs end;
    exec frs activate menuitem :ERx("Drivers");
    exec frs begin;
	    inst_drivers_form(&drvType, &drvDisplayType, altPath);
    exec frs end;
    exec frs activate menuitem :ERx("Files");
    exec frs begin;
    {
	dsnDisplayType =  dsnpath_form(newPath);
	switch (dsnDisplayType)
	{
            case USER_DSN:
		dsnType = OCFG_USR_PATH;
                dsnStatus = allocHandles(OCFG_USR_PATH, altPath, &drvH, &dsnH);
		if (dsnStatus == OK)
                    dsnStatus = display_dsn(dsnH, &count);
		break;

	    case SYSTEM_DSN:
		dsnType = OCFG_SYS_PATH;
                dsnStatus = allocHandles(OCFG_SYS_PATH, NULL, &drvH, &dsnH);
		if (dsnStatus == OK)
		    dsnStatus = display_dsn(dsnH, &count);
		break;

	    case ALTERNATE_DSN:
		dsnType = OCFG_ALT_PATH;
                STcopy(newPath,altPath);
                dsnStatus = allocHandles(OCFG_ALT_PATH, altPath, &drvH, &dsnH);
		if (dsnStatus == OK)
		    dsnStatus = display_dsn(dsnH, &count);
                break;

            default:
		break;
	}
        if (dsnStatus == OK)
            dsnStatus = freeHandles(drvH, dsnH);

	if (dsnStatus == OK && dsnDisplayType != -1)
        {
            exec frs putform :topFormName 
		        (path=:ERx(getDsnTypeString(dsnDisplayType)));
        }
	else if (dsnStatus != OK)
	{
	    STprintf(errMsg,"Error obtaining DSN path.\n");
            display_err(errMsg,status);
	}
    }
    exec frs end;
    exec frs activate menuitem :ERget( FE_Help ), frskey1;
    exec frs begin;
        FEhelp( ERx( "ocfgmm.hlp" ), ERx( "ODBC Administrator" ) );
    exec frs end;
    exec frs activate menuitem :ERget(FE_End), frskey3;
    exec frs begin;
        exec frs breakdisplay;
    exec frs end;

    exec frs activate before column dsndef all;
    exec frs begin;
    {
        if (!count)
        {
            exec frs set_frs menu '' (active(:ERget(FE_Destroy))=0); 
            exec frs set_frs menu '' (active(:ERget(FE_Edit))=0); 
            exec frs set_frs menu '' (active(:ERget(FE_Test))=0); 
        } 
        else
        {
            exec frs set_frs menu '' (active(:ERget(FE_Destroy))=1); 
            exec frs set_frs menu '' (active(:ERget(FE_Edit))=1); 
            exec frs set_frs menu '' (active(:ERget(FE_Test))=1); 
        }
    }
    exec frs end;
    status = freeHandles(drvH, dsnH);
}

char * sel_drivers_form(PTR drvH)
{
    char      drv_pickList[2000] = "\0";
    static char      drvEntry[MAX_TXT];
    i4        drvChoice;
    STATUS    status;
    i4        count, i;
    char **drvList;

    count = listConfig(drvH, (i4)0, NULL, &status);
    if (status != OK || !count) 
    {
	STprintf(errMsg, "Cannot get count of ODBC driver entries.\n");
	display_err(errMsg,status);
        return;
    }
    if (count)
    {
        drvList = (char **)MEreqmem( (u_i2) 0, (u_i4)(count * sizeof(char *)
            + sizeof(char)), TRUE, (STATUS *) NULL);

        for (i=0;i<count;i++)
	    drvList[i] = (char *)MEreqmem( (u_i2) 0, (u_i4)OCFG_MAX_STRLEN, 
		TRUE, (STATUS *) NULL);
        listConfig( drvH, count, drvList, &status );
        if (status != OK) 
        {
            STprintf(errMsg, 
               "Cannot list ODBC driver entries.\n");
	    display_err(errMsg,status);
            return;
        }
        for (i=0;i<count;i++)
        {
            STprintf(drvEntry,"%s\n",drvList[i]);
            STcat(drv_pickList,drvEntry);
        }
    }
    /*
    ** Prompt for path using list pick pop-up.
    */    
    drvChoice = IIFDlpListPick( "Select Driver", drv_pickList,
			   0, -1, -1, NULL, NULL, NULL, NULL );
  
    if (drvChoice < 0)
    {
        for (i = 0; i < count; i++)
            MEfree(drvList[i]);
        MEfree((PTR)drvList);
        return NULL;
    }

    STcopy(drvList[drvChoice],drvEntry);
    for (i = 0; i < count; i++)
        MEfree(drvList[i]);
    MEfree((PTR)drvList);
    return (drvEntry);
}

i4
dsnpath_form(char *altPath)
{
    char        *path_list = "USER\nSYSTEM\nALTERNATE\n";
    i4          dsnChoice;
    char        *retPath;
    /*
    ** Prompt for path using list pick pop-up.
    */    
    dsnChoice = IIFDlpListPick( "Select Data Source Path", path_list,
			   0, -1, -1, NULL, NULL, NULL, NULL );

    switch (dsnChoice)
    {
	case USER_DSN:
	    STcopy(ODBCINI, altPath);
	    break;
	case SYSTEM_DSN:
	    STcopy(ODBCSYSINI, altPath);
	    break;
	case ALTERNATE_DSN:
            retPath = alt_dsnpath_form(); 
	    if (*retPath != '\0')  
	    {
		STcopy(retPath, altPath);
		break;
	    }  /* Fall through if cancelled */
	default:
	    *altPath = '\0';
	    return -1;
    }

    return dsnChoice;
}

char *alt_dsnpath_form()
{
    exec sql begin declare section;
        static char altp[OCFG_MAX_STRLEN] = "\0";
    exec sql end declare section;
    STATUS status; 

    /*
    **  Display altdsnpath form...
    */
    STcopy(ERx("altdsnpath"), altDsnPathsFormName);
    if (IIUFgtfGetForm(IIUFlcfLocateForm(), altDsnPathsFormName) != OK)
        return ("\0");

    /*
    **  Beginning of display loop.
    */
    exec frs display :altDsnPathsFormName with style = popup;

    exec frs activate menuitem :ERget( FE_Save ), frskey4;
	exec frs begin;
            exec frs getform :altDsnPathsFormName ( :altp = altdsnpath );
            exec frs breakdisplay;
	exec frs end;
    exec frs activate menuitem :ERget( FE_Cancel ), frskey9;
	exec frs begin;
	    STcopy("\0",altp);
            exec frs breakdisplay;
	exec frs end;
    exec frs activate menuitem :ERget( FE_Help ), frskey1;
    exec frs begin;
	FEhelp( ERx( "ocaltdsnp.hlp" ), ERx( "ODBC Administrator" ) );
    exec frs end;
    return (altp);
}


i4 drvpath_form(char *altPath)
{
    char        *path_list = "SYSTEM\nALTERNATE\n";
    i4          choice;
    char        *retPath;

    /*
    ** Prompt for path using list pick pop-up.
    */    
    choice = IIFDlpListPick( "Select Driver Path", path_list,
			   0, -1, -1, NULL, NULL, NULL, NULL );

    switch (choice)
    {
	case SYSTEM_DRV:
	    STcopy(ODBCSYSINI, altPath);
	    break;
	case ALTERNATE_DRV:
            retPath = alt_drvpath_form(); 
	    if (*retPath != '\0')  
	    {
		STcopy(retPath, altPath);
		break;
	    }  /* Fall through if cancelled */
	default:
	    *altPath = '\0';
	    return -1;
    }
    return( choice );
}

char *alt_drvpath_form()
{
    exec sql begin declare section;
        static char altp[MAX_TXT] = "\0";
    exec sql end declare section;
    STATUS status; 

    /*
    **  Display altdrvpath form...
    */
    STcopy(ERx("altdrvpath"), altDrvPathsFormName);
    if (IIUFgtfGetForm(IIUFlcfLocateForm(), altDrvPathsFormName) != OK)
        return ("\0"); 

    /*
    **  Beginning of display loop.
    */
    exec frs display :altDrvPathsFormName with style = popup;

    exec frs initialize;
    exec frs activate menuitem :ERget( FE_Save ), frskey4;
	exec frs begin;
        exec frs getform :altDrvPathsFormName ( :altp = altdrvpath );
            exec frs breakdisplay;
	exec frs end;
    exec frs activate menuitem :ERget( FE_Cancel ), frskey9;
	exec frs begin;
	    STcopy("\0",altp);
            exec frs breakdisplay;
	exec frs end;
    exec frs activate menuitem :ERget( FE_Help ), frskey1;
    exec frs begin;
	FEhelp( ERx( "ocaltdrvp.hlp" ), ERx( "ODBC Administrator" ) );
    exec frs end;
    return (altp);
}

void inst_drivers_form(i4 *type, i4 *drvDisplayType, char *altPath)
{
    exec sql begin declare section;
	char drivername[MAX_TXT];
	char driverversion[MAX_TXT];
	char filename[MAX_TXT];
	char tracing[4];
	char tracepath[OCFG_MAX_STRLEN];
    exec sql end declare section;

    PTR handle;
    PTR drvH, dsnH;
    i4 count, attrCount, i, j;
    char **drvList;
    ATTR_ID **attrList;
    STATUS status; 
    bool traceOn=FALSE;
    char newPath[OCFG_MAX_STRLEN] = "\0";
    STATUS drvStatus;
    i4 timeout;

    /*
    **  Display instdrivers form...
    */
    STcopy(ERx("instdrivers"), instDriversFormName);
    if (IIUFgtfGetForm(IIUFlcfLocateForm(), instDriversFormName) != OK)
        return;

    exec frs inittable  :instDriversFormName inst_drvrs read; 

    /*
    **  Beginning of display loop.
    */
    exec frs display :instDriversFormName;

    exec frs initialize;

    exec frs begin;
        if (*drvDisplayType == OCFG_ALT_PATH)
            status = display_drv(OCFG_ALT_PATH, altPath);
        else if (*drvDisplayType == OCFG_SYS_PATH)
            status = display_drv(OCFG_SYS_PATH, NULL);

        status = allocHandles(*drvDisplayType, *drvDisplayType ==
            OCFG_ALT_PATH ? altPath : NULL, &drvH, &dsnH);
        if (driverManager == MGR_VALUE_CAI_PT )
            handle = dsnH;
        else
            handle = drvH;
	traceOn = getTrace(handle, tracepath, &status);
	if (status != OK)
	{
	    STprintf(errMsg, "Cannot get tracing info.\n");
	    display_err(errMsg,status);
	}
	if (traceOn)
	    STcopy("ON", tracing);
        else
	    STcopy("OFF", tracing);

        exec frs putform :instDriversFormName ( tracing = :tracing,
	      tracepath = :tracepath, 
	      drvpath = :ERx(getDrvTypeString(*drvDisplayType)));
        status = freeHandles(drvH, dsnH);
    exec frs end;
    exec frs activate menuitem :ERx("Tracing");
    exec frs begin;
        status = allocHandles(*drvDisplayType, *drvDisplayType ==
            OCFG_ALT_PATH ? altPath : NULL, &drvH, &dsnH);
        if (driverManager == MGR_VALUE_CAI_PT )
            handle = dsnH;
        else
            handle = drvH;
        tracing_form(handle);
        traceOn = getTrace(handle, tracepath, &status);
        if (status != OK)
        {
            STprintf(errMsg, "Cannot get tracing info.\n");
            display_err(errMsg,status);
        }
        if (traceOn)
            STcopy("ON", tracing);
        else
            STcopy("OFF", tracing);
        exec frs putform :instDriversFormName ( tracing = :tracing,
	        tracepath = :tracepath);
        status = freeHandles(drvH, dsnH);
    exec frs end;
    exec frs activate menuitem :ERx("Files");
    exec frs begin;
        *drvDisplayType = drvpath_form(newPath);
        switch (*drvDisplayType)
        {
            case SYSTEM_DRV:
	        drvStatus = display_drv(OCFG_SYS_PATH, NULL);
                if (drvStatus == OK)
		    *type = OCFG_SYS_PATH;
		break;
            case ALTERNATE_DRV:
                drvStatus = display_drv(OCFG_ALT_PATH, altPath);
                if (drvStatus == OK)
                    *type = OCFG_ALT_PATH;
                break;
            default:                  /* Cancelled */
		drvStatus = OK;
		break;
	} /* end switch drvDisplaytype */

	if (drvStatus == OK && *drvDisplayType != -1)
	{
            exec frs putform :instDriversFormName 
		        (drvpath=:ERx(getDrvTypeString(*drvDisplayType)));
        } 
	else if (drvStatus != OK)
	{
            STprintf(errMsg, "Error obtaining path.\n");
            display_err(errMsg,drvStatus);
	}
    exec frs end;
    exec frs activate menuitem :ERx("Details");
    exec frs begin;
        exec frs getrow :instDriversFormName inst_drvrs 
		     (:drivername=drivername);
        if (*drvDisplayType == OCFG_ALT_PATH)
            status = allocHandles(OCFG_ALT_PATH, altPath, &drvH, &dsnH);
        else
            status = allocHandles(OCFG_SYS_PATH, NULL, &drvH, &dsnH);
        if (status == OK)
        {
	    details_form(drivername, drvH, TRUE, *drvDisplayType);
            status = freeHandles(drvH, dsnH);
        }
    exec frs end;
    exec frs activate menuitem :ERx("Configuration Options");
    exec frs begin;
        status = allocHandles(*drvDisplayType, *drvDisplayType ==
            OCFG_ALT_PATH ? altPath : NULL, &drvH, &dsnH);
        if (driverManager == MGR_VALUE_CAI_PT )
            handle = dsnH;
        else
            handle = drvH;
        timeout = getTimeout(handle);
        if (status == OK)
        {
            if (timeout_form(&timeout))
            {
                if ( CONFCH_YES == IIUFccConfirmChoice(CONF_END, 
                    "time out value", "Configuration Options", 
                    ERx("bogus"), ERx("")) )
                setTimeout(handle, timeout, &status);
            }
        }
        if (status != OK)
	{
            STprintf(errMsg,"Cannot set timeout info.\n");
	    display_err(errMsg,status);
	}
        status = freeHandles(drvH, dsnH);
    exec frs end;
    exec frs activate menuitem :ERget( FE_Help ), frskey1;
    exec frs begin;
	FEhelp( ERx( "ocinstdrvr.hlp" ), ERx( "ODBC Administrator" ) );
    exec frs end;
    exec frs activate menuitem :ERget(FE_End), frskey3;
    exec frs begin;
        exec frs breakdisplay;
    exec frs end;

    return;
}

bool tracing_form(PTR handle)
{
    exec sql begin declare section;
        char trace[4];
	char tracepath[OCFG_MAX_STRLEN];
	char *dmgr;
        char fieldname[MAX_TXT];
    exec sql end declare section;

    STATUS status; 
    bool traceEnabled=FALSE;

    /*
    **  Display tracing form...
    */
    STcopy(ERx("tracing"), tracingFormName);
    if (IIUFgtfGetForm(IIUFlcfLocateForm(), tracingFormName) != OK)
        return;

    /*
    **  Beginning of display loop.
    */
    exec frs display :tracingFormName;

    exec frs initialize;
    exec frs begin;
	traceEnabled = getTrace(handle, tracepath, &status);
	if (status != OK)
	{
            STprintf(errMsg,"Cannot get tracing info.\n");
	    display_err(errMsg,status);
	}
	if (traceEnabled)
	    STcopy("ON", trace);
    else
	    STcopy("OFF", trace);

    dmgr = driverManager == MGR_VALUE_UNIX_ODBC ? 
		     MGR_STR_UNIX_ODBC : MGR_STR_CAI_PT;
    exec frs putform :tracingFormName ( odbctracing = :trace,
		     tracepath = :tracepath,
		     drivermanager = :dmgr);
    exec frs end;
    exec frs activate menuitem :ERx("Set Tracing");
    exec frs begin;
    { 
	if (!STbcompare(trace, 0, "OFF", 0, FALSE ) )
	{
	    STcopy("ON", trace);
            traceEnabled = TRUE;
        }
        else
        {
             STcopy("OFF", trace);
             traceEnabled = FALSE;
        }
        exec frs putform :tracingFormName ( odbctracing = :trace );
    }
    exec frs end;
    exec frs activate menuitem :ERget( FE_Help ), frskey1;
	exec frs begin;
	FEhelp( ERx( "octrace.hlp" ), ERx( "ODBC Administrator" ) );
    exec frs end;
    exec frs activate menuitem :ERget( FE_Save ), frskey4;
    exec frs begin;
	exec frs getform :tracingFormName ( :tracepath = tracepath );
            setTrace(handle, traceEnabled, tracepath, &status);
	if (status != OK)
	{
            STprintf(errMsg,"Cannot set tracing info.\n");
	        display_err(errMsg,status);
	}
    exec frs end;
    exec frs activate menuitem :ERget( FE_End ), frskey3;
    exec frs begin;
        exec frs breakdisplay;
    exec frs end;
    
    return;
}

bool timeout_form(i4 *timeout)
{
    exec sql begin declare section;
        i4 poolTimeOut;
        bool changed;
    exec sql end declare section;
    char msg[80];
    bool ret = FALSE;

    /*
    **  Display timeout form...
    */
    STcopy(ERx("pooltimeout"), timeoutFormName);
    if (IIUFgtfGetForm(IIUFlcfLocateForm(), timeoutFormName) != OK)
        return;

    /*
    **  Beginning of display loop.
    */
    exec frs display :timeoutFormName;

    exec frs initialize;
    exec frs begin;
    poolTimeOut = *timeout;
    exec frs putform :timeoutFormName ( timeout = :poolTimeOut);
    exec frs end;
    exec frs activate menuitem :ERget( FE_Help ), frskey1;
	exec frs begin;
	FEhelp( ERx( "octimeout.hlp" ), ERx( "ODBC Administrator" ) );
    exec frs end;
    exec frs activate menuitem :ERget( FE_Save ), frskey4;
    exec frs begin;
        exec frs inquire_frs form (:changed = change(:timeoutFormName));
        if (changed)
        {
	    exec frs getform :timeoutFormName ( :poolTimeOut = timeout );
            if (poolTimeOut == 0 || poolTimeOut < -1 )
            {
                STprintf(msg, 
                    "Time out value must be -1 or range 1 to 2,147,483,647\n");
                display_message(msg);
                exec frs resume field timeout;
            }
            *timeout = poolTimeOut;
            ret = TRUE;
        }
    exec frs end;
    exec frs activate menuitem :ERget( FE_End ), frskey3;
    exec frs begin;
        exec frs breakdisplay;
    exec frs end;
    
    return ret;
}

void details_form(char *name, PTR handle, bool isDriver, 
    i4 displayType)
{
    STATUS status; 
    exec sql begin declare section;
	char name_title[MAX_TXT];
	char attribute[MAX_TXT];
	char value[MAX_TXT];
    exec sql end declare section;
    ATTR_NAME **attrName;
    i4 count, j;

    if (isDriver)
	STprintf(name_title,"Driver name: %s",name);
    else
	STprintf(name_title,"Data Source Name: %s",name);
    /*
    **  Display details form...
    */
    STcopy(ERx("details"), detailsFormName);
    if (IIUFgtfGetForm(IIUFlcfLocateForm(), detailsFormName) != OK)
        return;

    /*
    **  Beginning of display loop.
    */
    exec frs display :detailsFormName;

    exec frs initialize;
    exec frs begin;
        count = getEntryAttribute( handle, name, 0, NULL, &status );
        if (status != OK)
        {
	    STprintf(errMsg, "Cannot get entry count.\n");
	    display_err(errMsg,status);
            return;
        }
	    if (count)
	    {
            attrName = (ATTR_NAME **)MEreqmem( (u_i2) 0, 
	            (u_i4)(count * sizeof(ATTR_NAME *)), TRUE, (STATUS *) NULL);
            for (j = 0; j < count; j++)
    	    {
                attrName[j] = (ATTR_NAME *)MEreqmem( (u_i2) 0, 
    	            (u_i4)(sizeof(ATTR_NAME)), TRUE, (STATUS *) NULL);
    	        attrName[j]->name = (char *) MEreqmem( (u_i2) 0, 
    	            (u_i4)MAX_TXT, TRUE, (STATUS *) NULL);
    	        attrName[j]->value = (char *) MEreqmem( (u_i2) 0, 
    	            (u_i4)OCFG_MAX_STRLEN, TRUE, (STATUS *) NULL);
    	    }
            getEntryAttribute( handle, name, count, attrName, &status );
            if (status != OK)
            {
    	        STprintf(errMsg, "Cannot get configuration entries.\n");
    	        display_err(errMsg,status);
                return;
            }
        }
        exec frs inittable  :detailsFormName detailstbl read; 

        for (j = 0; j < count; j++)
	    {
	        STcopy(attrName[j]->name,attribute);
                STcopy(attrName[j]->value,value);
	        exec frs loadtable :detailsFormName detailstbl 
		        (attribute = :attribute, value = :value);
        }
	    if (isDriver)
            exec frs putform :detailsFormName (heading= 
                :ERx("Driver Configuration Details"),
	    	path=:ERx(getDrvTypeString(displayType)));
	    else
            exec frs putform :detailsFormName (heading= 
                :ERx("Data Source Configuration Details"),
		path=:ERx(getDsnTypeString(displayType)));

    exec frs end;

    exec frs activate menuitem :ERget( FE_Help ), frskey1;
    exec frs begin;
	FEhelp( ERx( isDriver ? "ocdrvdetails.hlp" : "ocdsndetails.hlp" ), 
	ERx( "ODBC Administrator" ) );
    exec frs end;
    exec frs activate menuitem :ERget(FE_End), frskey3;
    exec frs begin;
        exec frs breakdisplay;
    exec frs end;
    for (j = 0; j < count; j++)
    {
	    MEfree((PTR)attrName[j]->name);
	    MEfree((PTR)attrName[j]->value);
	    MEfree((PTR)attrName[j]);
    }
    MEfree((PTR)attrName);
    return;
}

bool
save_dsn_form( char *dsnname )
{
    char msg[MAX_TXT] = "data for Data Source";

    /* Prompt for confirmation. */

    if ( CONFCH_YES == IIUFccConfirmChoice(CONF_END,
        dsnname, msg, ERx("bogus"), ERx("")) )
        return TRUE;
    return( FALSE );
}

bool
destroy_dsn_form( char *dsnname )
{
    char msg[MAX_TXT] = "data for Data Source";

    /* Prompt for confirmation. */

    if ( CONFCH_YES == IIUFccConfirmChoice(CONF_DESTROY,
        dsnname, msg, ERx(""), ERx("")) )
        return TRUE;
    return( FALSE );
}

void dsn_config_form(char *dsnname, char *drvname, PTR dsnH, 
    bool new, i4 dsnDisplayType)
{
    exec sql begin declare section;
        char fieldname[OCFG_MAX_STRLEN] = "\0";
	char datasource[OCFG_MAX_STRLEN];
	char description[OCFG_MAX_STRLEN];
	char drivername[OCFG_MAX_STRLEN];
	char withoptions[OCFG_MAX_STRLEN];
	char rolename[OCFG_MAX_STRLEN];
	char rolepwd[OCFG_MAX_STRLEN];
	char vnode[OCFG_MAX_STRLEN];
	char servertype[OCFG_MAX_STRLEN];
	char database[OCFG_MAX_STRLEN];
	char ronly[OCFG_MAX_STRLEN];
	char dsnpath[OCFG_MAX_STRLEN];
	char group[OCFG_MAX_STRLEN];
        bool changed;
    exec sql end declare section;
    STATUS status; 
    i4 srvtype,attrCount;
    i4 i,j;
    bool viewDetails = new ? FALSE : TRUE, formReturn = FALSE;
    char **defAttr = getDefaultInfo();
    char *dirPath;
    char fullPath[OCFG_MAX_STRLEN];
    bool missing = TRUE;

    i4 attrID[] = 
        {
            DSN_ATTR_DRIVER,
            DSN_ATTR_DESCRIPTION,
            DSN_ATTR_VENDOR,
            DSN_ATTR_DRIVER_TYPE,
            DSN_ATTR_SERVER,
            DSN_ATTR_DATABASE,
            DSN_ATTR_SERVER_TYPE,
            DSN_ATTR_PROMPT_UID_PWD,
            DSN_ATTR_WITH_OPTION,
            DSN_ATTR_ROLE_NAME,
            DSN_ATTR_ROLE_PWD,
            DSN_ATTR_DISABLE_CAT_UNDERSCORE,
            DSN_ATTR_ALLOW_PROCEDURE_UPDATE,
            DSN_ATTR_USE_SYS_TABLES,
            DSN_ATTR_BLANK_DATE,
            DSN_ATTR_DATE_1582,
            DSN_ATTR_CAT_CONNECT,
            DSN_ATTR_NUMERIC_OVERFLOW,
            DSN_ATTR_SUPPORT_II_DECIMAL,
            DSN_ATTR_CAT_SCHEMA_NULL,
            DSN_ATTR_READ_ONLY,
            DSN_ATTR_SELECT_LOOPS,
            DSN_ATTR_CONVERT_3_PART,
            DSN_ATTR_DBMS_PWD,
            DSN_ATTR_GROUP,
            DSN_ATTR_MULTIBYTE_FILL_CHAR,
            DSN_ATTR_INGRESDATE,
            DSN_ATTR_STRING_TRUNCATION
        };		 
    i4 count = sizeof(attrID) / sizeof(attrID[0]);
    char server_types[512] = "\0";
    char *serverList[] = 
    {    "Ingres", "DCOM", "IDMS", "DB2", "DB2UDB", "IMS", "VSAM", "RDB", 
         "STAR", "RMS", "Oracle", "Informix", "Sybase", "MSSQL", "VANT"
    };
    char serverEntry[MAX_TXT];

    ATTR_ID **attrList = (ATTR_ID **)MEreqmem( (u_i2) 0, (u_i4)(count * 
        sizeof(ATTR_ID *)), TRUE, (STATUS *) NULL);

    group[0] = '\0';
    STcopy(dsnname, datasource);
    for (j = 0; j < count; j++)
    {
        attrList[j] = (ATTR_ID *)MEreqmem( (u_i2) 0, 
	    (u_i4)(sizeof(ATTR_ID)), TRUE, (STATUS *) NULL);
        attrList[j]->value = (char *)MEreqmem( (u_i2) 0, 
	    (u_i4)OCFG_MAX_STRLEN, TRUE, (STATUS *) NULL);
    }

    for (j = 0; j < (sizeof(serverList) 
        / sizeof(serverList[0])); j++)
    {
        STprintf(serverEntry,"%s\n",serverList[j]);
        STcat(server_types, serverEntry);
    }
    /*
    **  Display dsnconfig form...
    */
    STcopy(ERx("dsnconfig"), dsnConfigFormName);
    if (IIUFgtfGetForm(IIUFlcfLocateForm(), dsnConfigFormName) != OK)
        return;

    /*
    **  Beginning of display loop.
    */
    exec frs display :dsnConfigFormName;

    exec frs initialize;

    exec frs begin; 
	{
        if (!new) 
        {
            attrCount = getConfigEntry( dsnH, dsnname, 0, NULL, &status );
            if (status != OK)
            {
	        STprintf(errMsg,"Cannot get DSN entry count.\n");
	        display_err(errMsg,status);
                return;
            }
	    if (attrCount)
	    {
                getConfigEntry( dsnH, dsnname, attrCount, attrList, &status );
                if (status != OK)
                {
	            STprintf(errMsg,"Cannot get DSN entries.\n");
	            display_err(errMsg,status);
                    return;
                }
                for (j=0;j<attrCount;j++)
                {
                    if (attrList[j]->id == DSN_ATTR_DESCRIPTION)
                        STcopy(attrList[j]->value,description);
                    if (attrList[j]->id == DSN_ATTR_DRIVER)
                        STcopy(attrList[j]->value,drivername);
                    if (attrList[j]->id == DSN_ATTR_WITH_OPTION)
                        STcopy(attrList[j]->value,withoptions);
                    if (attrList[j]->id == DSN_ATTR_ROLE_NAME)
                        STcopy(attrList[j]->value,rolename);
                    if (attrList[j]->id == DSN_ATTR_ROLE_PWD)
                        STcopy(attrList[j]->value,rolepwd);
                    if (attrList[j]->id == DSN_ATTR_SERVER)
                        STcopy(attrList[j]->value,vnode);
                    if (attrList[j]->id == DSN_ATTR_SERVER_TYPE)
                        STcopy(attrList[j]->value,servertype);
                    if (attrList[j]->id == DSN_ATTR_DATABASE)
                        STcopy(attrList[j]->value,database);
                    if (attrList[j]->id == DSN_ATTR_READ_ONLY)
                        STcopy(attrList[j]->value,ronly);
                    if (attrList[j]->id == DSN_ATTR_GROUP)
                        STcopy(attrList[j]->value,group);
                }
                /*
                ** Look for missing attributes and add default values.
                */
                for (i=0;i<count;i++)
                {
                    for (j = 0; j < attrCount; j++)
                    {
                        missing = TRUE;
                        if (attrID[i] == attrList[j]->id)
                        {
                            missing = FALSE;
                            break;
                        }
                    }
                    if (missing)
                    {
                        attrList[j]->id = attrID[i];
                        attrCount++;
                    }
                }
            }  /* if (attrCount) */
        } /* if (!new) */
        else
        {
            for (j = 0; j < count; j++)
	        {
                    attrList[j]->id = attrID[j];
                    if (attrList[j]->id == DSN_ATTR_DESCRIPTION)
                        attrList[j]->value[0] = '\0';
                    if (attrList[j]->id == DSN_ATTR_DRIVER)
                    {
                        if (driverManager == MGR_VALUE_CAI_PT)
                        {
                            /*
                            ** Get the path of the driver file.
                            */
                            NMgtAt(SYSTEM_LOCATION_VARIABLE,&dirPath);
                            if (dirPath != NULL && *dirPath)
                                STcopy(dirPath,fullPath);
                            else
                            {			     
                                STprintf(msg, "Cannot determine value of %s\n",
                                    SYSTEM_LOCATION_VARIABLE);
                                display_message(msg);
                                return;
                            }
# ifdef VMS
                            STcat(fullPath,"[");
                            STcat(fullPath,SYSTEM_LOCATION_SUBDIRECTORY); 
                            STcat(fullPath,".library]");
# else
                            STcat(fullPath,"/");
                            STcat(fullPath,SYSTEM_LOCATION_SUBDIRECTORY);
                            STcat(fullPath,"/lib/");
# endif /* VMS */
                            STcat(fullPath,defAttr[0]);
			    STcopy(fullPath,attrList[j]->value);
                        }
                        else                        
                            STcopy(drvname,attrList[j]->value);
                    }			 
                    if (attrList[j]->id == DSN_ATTR_WITH_OPTION)
                        attrList[j]->value[0] = '\0';
                    if (attrList[j]->id == DSN_ATTR_ROLE_NAME)
                        attrList[j]->value[0] = '\0';
                    if (attrList[j]->id == DSN_ATTR_ROLE_PWD)
                        attrList[j]->value[0] = '\0';
                    if (attrList[j]->id == DSN_ATTR_SERVER)
                        STcopy(DSN_STR_LOCAL, attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_SERVER_TYPE)
                        STcopy(DSN_STR_INGRES, attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_DATABASE)
                        attrList[j]->value[0] = '\0';
                    if (attrList[j]->id == DSN_ATTR_READ_ONLY)
                        STcopy(DSN_STR_N, attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_GROUP)
                        attrList[j]->value[0] = '\0';
                    if (attrList[j]->id == DSN_ATTR_VENDOR)
                        STcopy(DSN_STR_INGRES_CORP,attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_DRIVER_TYPE)
                        STcopy(DSN_STR_INGRES,attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_DISABLE_CAT_UNDERSCORE)
                        STcopy(DSN_STR_N, attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_ALLOW_PROCEDURE_UPDATE)
                        STcopy(DSN_STR_N, attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_USE_SYS_TABLES)
                        STcopy(DSN_STR_N, attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_BLANK_DATE)
                        STcopy(DSN_STR_N, attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_DATE_1582)
                        STcopy(DSN_STR_N, attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_CAT_CONNECT)
                        STcopy(DSN_STR_N, attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_NUMERIC_OVERFLOW)
                        STcopy(DSN_STR_N, attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_SUPPORT_II_DECIMAL)
                        STcopy(DSN_STR_N, attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_CAT_SCHEMA_NULL)
                        STcopy(DSN_STR_N, attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_SELECT_LOOPS)
                        STcopy(DSN_STR_N, attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_CONVERT_3_PART)
                        STcopy(DSN_STR_N, attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_MULTIBYTE_FILL_CHAR)
                        STcopy(DSN_STR_EMPTY_STR, attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_INGRESDATE)
                        STcopy(DSN_STR_N, attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_STRING_TRUNCATION)
                        STcopy(DSN_STR_N, attrList[j]->value);
	        }
            for (j=0;j<count;j++)
            {
                if (attrList[j]->id == DSN_ATTR_DESCRIPTION)
                    STcopy(attrList[j]->value,description);
                if (attrList[j]->id == DSN_ATTR_DRIVER)
                    STcopy(attrList[j]->value,drivername);
                if (attrList[j]->id == DSN_ATTR_WITH_OPTION)
                    STcopy(attrList[j]->value,withoptions);
                if (attrList[j]->id == DSN_ATTR_ROLE_NAME)
                    STcopy(attrList[j]->value,rolename);
                if (attrList[j]->id == DSN_ATTR_ROLE_PWD)
                    STcopy(attrList[j]->value,rolepwd);
                if (attrList[j]->id == DSN_ATTR_SERVER)
                    STcopy(attrList[j]->value,vnode);
                if (attrList[j]->id == DSN_ATTR_SERVER_TYPE)
                    STcopy(attrList[j]->value,servertype);
                if (attrList[j]->id == DSN_ATTR_DATABASE)
                    STcopy(attrList[j]->value,database);
                if (attrList[j]->id == DSN_ATTR_READ_ONLY)
                    STcopy(attrList[j]->value,ronly);
                if (attrList[j]->id == DSN_ATTR_GROUP)
                    STcopy(attrList[j]->value,group);
                }
        }
        exec frs putform :dsnConfigFormName 
            (dsnpath = :ERx(getDsnTypeString(dsnDisplayType)),
	     datasource = :datasource,
	     description = :description,
	     drivername = :drivername,
	     withoptions = :withoptions,
	     rolename = :rolename,
	     rolepwd = :rolepwd,
	     vnode = :vnode,
	     servertype = :servertype,
	     database = :database,
	     readonly = :ronly,
	     dsngroup = :group);
	}
    exec frs end;
    exec frs activate menuitem :ERx("Advanced");
	exec frs begin;
	    if (advanced_form(attrList, (new ? count : attrCount), 
		dsnDisplayType))
            {
                setConfigEntry( dsnH, dsnname, (new ? count : attrCount ), 
	           attrList, &status );
                if (status != OK)
                {
                    STprintf(errMsg,"Error writing DSN entries.\n");
	            display_err(errMsg,status);
                }    
            }
	exec frs end;
    exec frs activate menuitem :ERx("Details");
	exec frs begin;
	    if (viewDetails)
	        details_form(dsnname, dsnH, FALSE, dsnDisplayType);
	exec frs end;
    exec frs activate menuitem :ERget(F_ST0001_list_choices) (activate=0);
    exec frs begin;
    {
	if (!STcompare(fieldname, ERx("servertype")))
	    srvtype = IIFDlpListPick( "Select ServerType", server_types,
		   0, -1, -1, NULL, NULL, NULL, NULL );
        if (srvtype != -1)
        {
            STcopy(serverList[srvtype],servertype);
            exec frs putform :dsnConfigFormName ( servertype = :servertype );
            changed = TRUE;
            exec frs set_frs form (change(:dsnConfigFormName) = :changed);
        }
    }
    exec frs end;
    exec frs activate menuitem :ERget ( FE_Save ), frskey4;
    exec frs begin;
    {
        exec frs inquire_frs form (:changed = change(:dsnConfigFormName));
        if (changed)
        {
            exec frs getform :dsnConfigFormName ( 
                :datasource = datasource,
                :description = description,
                :drivername = drivername,
                :withoptions = withoptions,
                :rolename = rolename,
                :rolepwd = rolepwd,
                :vnode = vnode,
                :servertype = servertype,
                :database = database,
                :ronly = readonly,
                :group = dsngroup);
             
            if (!STlength(vnode))
            {
                STprintf(msg, "Blank vnode name not allowed. Enter \"(local)\" for local databases\n");
                display_message(msg);
                formReturn = FALSE;
                exec frs resume field vnode;
            }
            else if (!STlength(database))
            {
                STprintf(msg, "Blank database name not allowed.\n");
                display_message(msg);
                formReturn = FALSE;
                exec frs resume field database;
            }
            else if (!STlength(servertype))
            {
                STprintf(msg, "Blank ServerType not allowed.  See ListChoices menu option\n");
                display_message(msg);
                formReturn = FALSE;
                exec frs resume field servertype;
            }
            else if (STbcompare(ronly, 0, DSN_STR_N, 0, TRUE) 
    		 && STbcompare(ronly, 0, DSN_STR_Y, 0, TRUE))
            {
                STprintf(msg, 
                    "Only Y or N allowed for Read Only entry");
                display_message(msg);
                formReturn = FALSE;
                exec frs resume field readonly;
            }
            else
                formReturn = save_dsn_form(dsnname);

            if (formReturn == TRUE)
            {
                for (j=0;j< (new ? count : attrCount);j++)
                {
                    if (attrList[j]->id == DSN_ATTR_DESCRIPTION)
                        STcopy(description, attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_DRIVER)
                        STcopy(drivername,attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_WITH_OPTION)
                        STcopy(withoptions, attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_ROLE_NAME)
                        STcopy(rolename, attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_ROLE_PWD)
                        STcopy(rolepwd,attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_SERVER)
                        STcopy(vnode, attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_SERVER_TYPE)
                        STcopy(servertype,attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_DATABASE)
                        STcopy(database, attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_READ_ONLY)
                        STcopy(ronly,attrList[j]->value);
                    if (attrList[j]->id == DSN_ATTR_GROUP)
                        STcopy(group,attrList[j]->value);
                }
                setConfigEntry( dsnH, dsnname, (new ? count : attrCount ), 
    	        attrList, &status );
                if (status != OK)
                {
                    STprintf(errMsg,"Error writing DSN entries.\n");
                    display_err(errMsg,status);
                }
                else
                    viewDetails = TRUE;
            } /* if (formReturn == TRUE) */
            exec frs breakdisplay;
        } /* if (changed) */
        else if (!STlength(database))
        {
            STprintf(msg, "Blank database name not allowed.\n");
            display_message(msg);
            formReturn = FALSE;
            exec frs resume field database;
        }
    } /* exec frs begin */
    exec frs end;
    exec frs activate menuitem :ERget( FE_Help ), frskey1;
    exec frs begin;
	    FEhelp( ERx( "ocdsncfg.hlp" ), ERx( "ODBC Administrator" ) );
    exec frs end;
    exec frs activate menuitem :ERget(FE_End), frskey3;
    exec frs begin;
        exec frs enddisplay;
    exec frs end;

    exec frs activate before field all;
    exec frs begin;
    {
        exec frs inquire_frs form (:fieldname = field);
        if (!STcompare (fieldname,ERx("servertype")))
        {
            exec frs set_frs menu '' (active(:ERget(F_ST0001_list_choices))=1); 
        } 
        else
        {
            exec frs set_frs menu '' (active(:ERget(F_ST0001_list_choices))=0); 
        }
    }
    exec frs end;

    exec frs finalize;

    for (j = 0; j < (new ? count : attrCount); j++)
	    MEfree(attrList[j]->value);

    return;
}

bool dsn_name_form(char *dsnname, char *drivername, PTR drvH,
    PTR dsnH)
{
    STATUS status; 
    bool ret = FALSE;
    exec sql begin declare section;
	char dsn[MAX_TXT];
        bool changed;
    exec sql end declare section;

    char *dn;
    i4 i;
    char **dsnList = NULL;
    i4 count = listConfig(dsnH, (i4)0, NULL, &status);

    if (status != OK)
        return FALSE;

    if (count)
    {
        dsnList = (char **)MEreqmem( (u_i2) 0, 
            (u_i4)(count * sizeof(char *)), TRUE, (STATUS *) NULL);
        for (i=0;i<count;i++)
	    dsnList[i] = (char *)MEreqmem( (u_i2) 0, (u_i4)OCFG_MAX_STRLEN, 
	    TRUE, (STATUS *) NULL);
        listConfig( dsnH, count, dsnList, &status );
        if (status != OK)
            return FALSE;
    }

/*
**  Display dsnname form...
*/
    STcopy(ERx("dsnname"), dsnNameFormName);
    if (IIUFgtfGetForm(IIUFlcfLocateForm(), dsnNameFormName) != OK)
        return FALSE;

/*
**  Beginning of display loop.
*/
    exec frs display :dsnNameFormName;

    exec frs initialize;

    exec frs activate menuitem :ERget ( FE_Save ), frskey4;
    exec frs begin;
	exec frs getform :dsnNameFormName ( :dsn = dsn );
        if (!STlength(dsn))
        {
           STprintf(msg, "Blank data source name is not allowed\n"); 
           display_message(msg);
        }
        else
        {
            exec frs inquire_frs form (:changed = change(:dsnNameFormName));
            if (changed)
            {            
                for (i = 0; i < count; i++)
                {
                    if (!STbcompare( dsn, 0, dsnList[i], 0, TRUE ))
                    {
                        STprintf(msg, 
                            "Duplicate definition of %s is not allowed\n", dsn);
                        display_message(msg);
                        ret = FALSE;
                        break;
                    }
                }
                if (i >= count)
                {
                    STcopy(dsn,dsnname);
                    ret = TRUE;
                    dn = sel_drivers_form(drvH);
                    if (dn)
                    {
                        STcopy(dn, drivername);
    	            exec frs breakdisplay;
                    }
                }
    	    } /* if (changed) */
        } /* if (STlength(dsn) */
    exec frs end;
    exec frs activate menuitem :ERget ( FE_Cancel ), frskey9;
    exec frs begin;
	ret = FALSE;
	exec frs breakdisplay;
    exec frs end;
    exec frs activate menuitem :ERget( FE_Help ), frskey1;
    exec frs begin;
	FEhelp( ERx( "ocdsnname.hlp" ), ERx( "ODBC Administrator" ) );
	ret = FALSE;
    exec frs end;

    for (i=0;i<count;i++)
        MEfree(dsnList[i]);
    if (dsnList)
        MEfree((PTR)dsnList);

    return ret;
}

bool advanced_form(ATTR_ID **dsnAttrList, i4 attrCount, i4 dsnDisplayType)
{
    exec sql begin declare section;
         char attribute[MAX_TXT];
         char value[MAX_TXT];
         i4 rowno;
         bool changed;
         char mbfillchar[2];
         char path[OCFG_MAX_STRLEN];
    exec sql end declare section;

    STATUS status; 
    bool ret = FALSE;
    i4 j;
    enum adv_form_attr
    {
        LOOP, SCHEMA, SYS, EMPTYSTRING, NULLDATE, SESSION,
        ARITHMETIC, IIDECIMAL, NULLSCHEMA, WILDCARD, PROCUPDATE,
        INGRESDATE, STRINGTRUNCATION
    };
    i2 table_size = sizeof(enum adv_form_attr) / sizeof(LOOP);
    char *attrDisplay[13] = 
    {
        "Cursor or Select Loops",
        "Convert 3-part owner.table.column to 2-part table.column",
        "Include \"SYS\" or \"sys\" SQLTables in result set",
        "Return empty string date values as NULL",
        "Return date values \"1582-01-01\" as NULL",
        "Force separate session for ODBC catalog functions",
        "Ignore arithmetic errors overflow, underflow, divide by zero",
        "Support II_DECIMAL",
        "Return NULL for SCHEMA columns in result sets",
        "Disable underscore wild-card pattern in search patterns",
        "Allow update in database procedures (Read-only driver)",
        "Send date/time as Ingres date",
        "Force string truncation errors"
    };
    char valueDisplay[13][6] =
    {
        "SELECT", DSN_STR_N, DSN_STR_N, DSN_STR_N, DSN_STR_N, "FAIL", 
	DSN_STR_N, DSN_STR_N, DSN_STR_N, DSN_STR_N, DSN_STR_N, DSN_STR_N,
        DSN_STR_N
    };
    /*
    **  Display advanced form...
    */
    STcopy(ERx("advanced"), advancedFormName);
    if (IIUFgtfGetForm(IIUFlcfLocateForm(), advancedFormName) != OK)
        return FALSE;

    exec frs inittable  :advancedFormName advanced read; 

    /*
    **  Beginning of display loop.
    */
    exec frs display :advancedFormName;

    exec frs initialize;
    
    exec frs begin;
    {
        for (j = 0; j < attrCount; j++)
        {
            if (dsnAttrList[j]->id == DSN_ATTR_SELECT_LOOPS)
	    {
	        if ( !STbcompare(dsnAttrList[j]->value, 0, DSN_STR_Y, 0, 
		    FALSE ) )
                    STcopy(DSN_STR_SELECT,valueDisplay[LOOP]);
	        else
		    STcopy(DSN_STR_CURSOR,valueDisplay[LOOP]);
	    }
            if (dsnAttrList[j]->id == DSN_ATTR_CONVERT_3_PART)
                STcopy(dsnAttrList[j]->value,valueDisplay[SCHEMA]);
            if (dsnAttrList[j]->id == DSN_ATTR_USE_SYS_TABLES)
                STcopy(dsnAttrList[j]->value,valueDisplay[SYS]);
            if (dsnAttrList[j]->id == DSN_ATTR_BLANK_DATE)
                STcopy(dsnAttrList[j]->value,valueDisplay[EMPTYSTRING]);
            if (dsnAttrList[j]->id == DSN_ATTR_DATE_1582)
                STcopy(dsnAttrList[j]->value,valueDisplay[NULLDATE]);
            if (dsnAttrList[j]->id == DSN_ATTR_CAT_CONNECT)
                STcopy(dsnAttrList[j]->value,valueDisplay[SESSION]);
            if (dsnAttrList[j]->id == DSN_ATTR_NUMERIC_OVERFLOW)
            {
                if ( !STbcompare(dsnAttrList[j]->value, 0, DSN_STR_Y, 
		    0, FALSE ) )
                    STcopy(DSN_STR_IGNORE,valueDisplay[ARITHMETIC]);
	        else
	            STcopy(DSN_STR_FAIL,valueDisplay[ARITHMETIC]);
	    }
            if (dsnAttrList[j]->id == DSN_ATTR_SUPPORT_II_DECIMAL)
                STcopy(dsnAttrList[j]->value,valueDisplay[IIDECIMAL]);
            if (dsnAttrList[j]->id == DSN_ATTR_CAT_SCHEMA_NULL)
                STcopy(dsnAttrList[j]->value,valueDisplay[NULLSCHEMA]);
            if (dsnAttrList[j]->id == DSN_ATTR_DISABLE_CAT_UNDERSCORE)
                STcopy(dsnAttrList[j]->value,valueDisplay[WILDCARD]);
            if (dsnAttrList[j]->id == DSN_ATTR_ALLOW_PROCEDURE_UPDATE)
                STcopy(dsnAttrList[j]->value,valueDisplay[PROCUPDATE]); 
            if (dsnAttrList[j]->id == DSN_ATTR_MULTIBYTE_FILL_CHAR)
                STcopy(dsnAttrList[j]->value,mbfillchar); 
            if (dsnAttrList[j]->id == DSN_ATTR_INGRESDATE)
                STcopy(dsnAttrList[j]->value,valueDisplay[INGRESDATE]); 
            if (dsnAttrList[j]->id == DSN_ATTR_STRING_TRUNCATION)
                STcopy(dsnAttrList[j]->value,valueDisplay[STRINGTRUNCATION]); 
        } 
	for (j = 0; j < 13; j++)
	{
	    STcopy(attrDisplay[j],attribute);
            STcopy(valueDisplay[j],value);
            exec frs loadtable :advancedFormName advanced ( attribute =
                :attribute, value = :value); 
        }
        STcopy(getDsnTypeString(dsnDisplayType),path);
        exec frs putform :advancedFormName ( path = :path, mbfillchar = :mbfillchar);
    }
    exec frs end;
    exec frs activate menuitem :ERget ( FE_Edit );
    exec frs begin;
    {
	exec frs getrow :advancedFormName advanced (:attribute=attribute,
	    :value = value);
        CVupper(value);
        exec frs inquire_frs table :advancedFormName (:rowno = rowno);
            switch (rowno - 1)
	    {
                case LOOP:
	            if ( !STbcompare(value, 0, DSN_STR_SELECT, 0, FALSE ) )
                        STcopy(DSN_STR_CURSOR,value);
	            else
                        STcopy(DSN_STR_SELECT,value);
                    break;

                case ARITHMETIC:
                    if ( !STbcompare(value, 0, DSN_STR_IGNORE, 0, FALSE ) )
                        STcopy(DSN_STR_FAIL,value);
	            else
                        STcopy(DSN_STR_IGNORE,value);
                    break;

                default:
                    if ( !STbcompare(value, 0, DSN_STR_Y, 0, FALSE ) )
                        STcopy(DSN_STR_N,value);
	            else
                        STcopy(DSN_STR_Y,value);
                    break;

            } /* End switch (rowno) */
	exec frs putrow :advancedFormName advanced (attribute=:attribute,
	    value = :value);
    }
    exec frs end;
    exec frs activate menuitem :ERget ( FE_Save ), frskey4;
    exec frs begin;
    {
        if (save_dsn_form("advanced"))
        {
            exec frs getform :advancedFormName ( :path = path,
	        :mbfillchar = mbfillchar);
            copyValue (dsnAttrList, attrCount, 
	        DSN_ATTR_MULTIBYTE_FILL_CHAR, mbfillchar);
            exec frs unloadtable :advancedFormName advanced ( :attribute =
                attribute, :value = value); 
            exec frs begin;
            {
                if (!STbcompare(attribute, 0, attrDisplay[LOOP],
		    0, FALSE))
                {
		    for (j = 0; j < attrCount; j++)
                    {
                        if (dsnAttrList[j]->id == DSN_ATTR_SELECT_LOOPS)
                        {
	                    if ( !STbcompare(value, 0, DSN_STR_SELECT, 
				0, FALSE ) )
                                STcopy(DSN_STR_Y,dsnAttrList[j]->value);
	                    else
                                STcopy(DSN_STR_N,dsnAttrList[j]->value);
                            break;
                        }
                    }  /* End for (j = 0; j < attrCount; j++) */
                }
                else if (!STbcompare(attribute, 0, 
		    attrDisplay[ARITHMETIC], 0, FALSE))
                {
		    for (j = 0; j < attrCount; j++)
                    {
                        if (dsnAttrList[j]->id == DSN_ATTR_NUMERIC_OVERFLOW)
                        {
                            if ( !STbcompare(value, 0, "IGNORE", 0, FALSE ) )
                                STcopy(DSN_STR_Y,dsnAttrList[j]->value);
	                    else
                                STcopy(DSN_STR_N,dsnAttrList[j]->value);
                            break;
                        }
                    }  /* End for (j = 0; j < attrCount; j++) */
                 }
                 else if (!STbcompare(attribute, 0, 
		     attrDisplay[SCHEMA], 0, FALSE))
                     copyValue (dsnAttrList, attrCount, 
			 DSN_ATTR_CONVERT_3_PART, value);
                 else if (!STbcompare(attribute, 0, 
		     attrDisplay[SYS], 0, FALSE))
                     copyValue (dsnAttrList, attrCount, 
			 DSN_ATTR_USE_SYS_TABLES, value);
                 else if (!STbcompare(attribute, 0, 
		     attrDisplay[EMPTYSTRING], 0, FALSE))
                     copyValue (dsnAttrList, attrCount, 
			 DSN_ATTR_BLANK_DATE, value);
                 else if (!STbcompare(attribute, 0, 
		     attrDisplay[NULLDATE], 0, FALSE))
                     copyValue (dsnAttrList, attrCount, 
			 DSN_ATTR_DATE_1582, value);
                 else if (!STbcompare(attribute, 0, 
		     attrDisplay[SESSION], 0, FALSE))
                     copyValue (dsnAttrList, attrCount, 
			 DSN_ATTR_CAT_CONNECT, value);
                 else if (!STbcompare(attribute, 0, 
		     attrDisplay[IIDECIMAL], 0, FALSE))
                     copyValue (dsnAttrList, attrCount, 
			 DSN_ATTR_SUPPORT_II_DECIMAL, value);
                 else if (!STbcompare(attribute, 0, 
		     attrDisplay[NULLSCHEMA], 0, FALSE))
                     copyValue (dsnAttrList, attrCount, 
			 DSN_ATTR_CAT_SCHEMA_NULL, value);
                 else if (!STbcompare(attribute, 0, 
		     attrDisplay[WILDCARD], 0, FALSE))
                     copyValue (dsnAttrList, attrCount, 
			 DSN_ATTR_DISABLE_CAT_UNDERSCORE, value);
                 else if (!STbcompare(attribute, 0, 
		     attrDisplay[PROCUPDATE], 0, FALSE))
                     copyValue (dsnAttrList, attrCount, 
			 DSN_ATTR_ALLOW_PROCEDURE_UPDATE, value);
                 else if (!STbcompare(attribute, 0, 
		     attrDisplay[INGRESDATE], 0, FALSE))
                     copyValue (dsnAttrList, attrCount, 
			 DSN_ATTR_INGRESDATE, value);
                 else if (!STbcompare(attribute, 0, 
		     attrDisplay[STRINGTRUNCATION], 0, FALSE))
                     copyValue (dsnAttrList, attrCount, 
			 DSN_ATTR_STRING_TRUNCATION, value);
            }
            exec frs end; /* unloadtable display loop */
        } /* if (save_dsn_form("advanced")) */
        ret = TRUE;
        exec frs breakdisplay;
    }
    exec frs end; /* Save display loop */
    exec frs activate menuitem :ERget ( FE_End ), frskey3;
    exec frs begin;
    {
	exec frs breakdisplay;
    }
    exec frs end;
    exec frs activate menuitem :ERget( FE_Help ), frskey1;
    exec frs begin;
    {
	FEhelp( ERx( "ocadvname.hlp" ), ERx( "ODBC Administrator" ) );
	ret = FALSE;
    }
    exec frs end;
    return ret;
}

/*
** Name: display_message
**
** Description:
**      Display a message prompt.
**
** Input:
**	text         Message text.
**
** Output:
**      none.
**
** Returns:
**	void.
**
** History:
**	 21-jan-04 (loera01)
**	    Created.
*/
void display_message( char *text )
{
    exec sql begin declare section;
    char *msg = text;
    exec sql end declare section;
    exec frs message :msg with style = popup;
}

/*
** Name: display_dsn
**
** Description:
**      Display data source information.
**
** Input:
**	drvH         Driver handle.
**      type         Path type.
**      altPath      Alternate path.
**
** Output:
**      dsnH         Data source handle.
**      count        Count of data source entries.
**
** Returns:
**	OK if successful, otherwise error code.
**
** History:
**	 21-jan-04 (loera01)
**	    Created.
*/
STATUS display_dsn(PTR dsnH, i4 *count)
{
    exec sql begin declare section;
        char dsnname[OCFG_MAX_STRLEN];
        char dsndescript[OCFG_MAX_STRLEN];
        char drvname[OCFG_MAX_STRLEN];
    exec sql end declare section;

    i4 i,j,attrCount;
    char **dsnList;
    ATTR_ID dsnAttrList[DSN_ATTR_MAX];
    ATTR_ID *attrList[DSN_ATTR_MAX];
    STATUS status = OK, drvStatus;
    STATUS dsnStatus;
    
    exec frs clear field dsndef;

    *count = listConfig(dsnH, (i4)0, NULL, &status);
    if (status != OK) 
        return status;

    if (*count)
    {
        dsnList = (char **)MEreqmem( (u_i2) 0, 
            (u_i4)(*count * sizeof(char *)), TRUE, (STATUS *) NULL);
        for (i=0;i<*count;i++)
	    dsnList[i] = (char *)MEreqmem( (u_i2) 0, (u_i4)OCFG_MAX_STRLEN, 
	    TRUE, (STATUS *) NULL);
       listConfig( dsnH, *count, dsnList, &status );
       if (status != OK) 
           return status;

        for (i=0;i<*count;i++)
        {
            STcopy(dsnList[i],dsnname);
            attrCount = getConfigEntry( dsnH, dsnname, 0, NULL, &status );
            if (status != OK)
                return status;
	    if (attrCount)
	    {
                for (j = 0; j < attrCount; j++)
	        {
                    attrList[j] = &dsnAttrList[j];
                    dsnAttrList[j].value = (char *)MEreqmem( (u_i2) 0, 
	                   (u_i4)OCFG_MAX_STRLEN, TRUE, (STATUS *) NULL);
	        }
                getConfigEntry( dsnH, dsnList[i], attrCount, attrList, 
                   &status );
                if (status != OK)
                    return status;

                for (j=0;j<attrCount;j++)
                {
	            if (dsnAttrList[j].id == DSN_ATTR_DESCRIPTION)
                        STcopy(dsnAttrList[j].value,dsndescript);
	            if (dsnAttrList[j].id == DSN_ATTR_DRIVER)
                        STcopy(dsnAttrList[j].value,drvname);
                    MEfree((PTR)dsnAttrList[j].value);
                }
	    }
            exec frs loadtable :topFormName dsndef ( dsnname = :dsnname, 
				    dsndescript = :dsndescript, 
				    drvname = :drvname );
        } 
    }
    for (i = 0; i < *count; i++)
	MEfree((PTR)dsnList[i]);
    if (*count)
        MEfree((PTR)dsnList);
    return status;
}
/*
** Name: display_drv
**
** Description:
**      Display driver information.
**
** Input:
**	drvH         Driver handle.
**
** Output:
**	none.
**
** Returns:
**	OK if successful, otherwise error code.
**
** History:
**	 21-jan-04 (loera01)
**	    Created.
*/
STATUS display_drv(i2 pathType, char *altPath)
{
    exec sql begin declare section;
    char drvpath[MAX_TXT];
	char drivername[MAX_TXT];
	char driverversion[MAX_TXT];
	char filename[MAX_TXT];
	char tracing[4];
	char tracepath[OCFG_MAX_STRLEN];
    exec sql end declare section;

    i4 count, attrCount, i, j;
    char **drvList;
    ATTR_ID **attrList;
    STATUS status; 
    bool traceOn=FALSE;
    i4 displayType;
    char newPath[OCFG_MAX_STRLEN] = "\0";
    PTR drvH, dsnH;
    char *p = NULL;
    
    exec frs clear field inst_drvrs;

    if (pathType == OCFG_SYS_PATH)
        status = allocHandles(OCFG_SYS_PATH, NULL, &drvH, &dsnH);
    else if (pathType == OCFG_ALT_PATH)
        status = allocHandles(OCFG_ALT_PATH, altPath, &drvH, &dsnH);
    if (status == OK)
        count = listConfig(drvH, (i4)0, NULL, &status);
    if (status != OK) 
    {
	STprintf(errMsg, "Cannot get count of ODBC driver entries.\n");
	display_err(errMsg,status);
        return;
    }
    if (count)
    {
        drvList = (char **)MEreqmem( (u_i2) 0, (u_i4)(count * sizeof(char *)),
		  TRUE, (STATUS *) NULL);

        for (i=0;i<count;i++)
	    drvList[i] = (char *)MEreqmem( (u_i2) 0, (u_i4)OCFG_MAX_STRLEN, 
		TRUE, (STATUS *) NULL);
        listConfig( drvH, count, drvList, &status );
        if (status != OK) 
        {
	    STprintf(errMsg, "Cannot list ODBC driver entries.\n");
	        display_err(errMsg,status);
            return;
        }
        for (i=0;i<count;i++)
        {
            STcopy(drvList[i],drivername);
            attrCount = getConfigEntry( drvH, drivername, 0, NULL, &status );
            if (status != OK)
            { 
                STprintf(errMsg, "Cannot get driver entry count.\n");
                display_err(errMsg,status);
                return;
            }
	    if (attrCount)
	    {
                attrList = (ATTR_ID **)MEreqmem( (u_i2) 0, 
	            (u_i4)(attrCount * sizeof(ATTR_ID *)), 
	            TRUE, (STATUS *) NULL);
                for (j = 0; j < attrCount; j++)
	        {
                    attrList[j] = (ATTR_ID *)MEreqmem( (u_i2) 0, 
	                    (u_i4)(sizeof(ATTR_ID)), TRUE, (STATUS *) NULL);
                    attrList[j]->value = (char *)MEreqmem( (u_i2) 0, 
	                    (u_i4)OCFG_MAX_STRLEN, TRUE, (STATUS *) NULL);
	        }
                getConfigEntry( drvH, drvList[i], attrCount, attrList, 
		            &status );
                if (status != OK)
                {
                    STprintf(errMsg, "Cannot get driver entries.\n");
	            display_err(errMsg,status);
                    return;
                }

                STcopy(drvList[i],drivername);
                for (j=0;j<attrCount;j++)
	        {
	            if (attrList[j]->id == DRV_ATTR_DRIVER)
                    {
                        STcopy(attrList[j]->value,filename); 
                        p = get_driver_version(filename);
                        if (p)
                        STcopy(p, driverversion);
                        else
                        STcopy("DEBUG",driverversion); 
                    } 
		    MEfree((PTR)attrList[j]->value);
		    MEfree((PTR)attrList[j]);
                }
		MEfree((PTR)attrList);
	    } /* If attrCount */
            exec frs loadtable :instDriversFormName inst_drvrs (
				    drivername = :drivername, 
				    driverversion = :driverversion, 
				    filename = :filename ); 
        } 
    }	  
    status = freeHandles(drvH, dsnH);
    for (i = 0; i < count; i++)
	    MEfree((PTR)drvList[i]);
    if (count) 
		MEfree((PTR)drvList);
    return status;
}
/*
** Name: getDrvTypeString
**
** Description:
**      Return a string that corresponds to the driver display value.
**
** Input:
**	drvDisplayType  Display type.
**
** Output:
**	none.
**
** Returns:
**	The string that corresponds to the display value.
**
** History:
**	 21-jan-04 (loera01)
**	    Created.
*/
char *getDrvTypeString(i4 drvDisplayType)
{
    switch (drvDisplayType)
    {
	case SYSTEM_DRV:
            return SYSTEM;
	case ALTERNATE_DRV:
            return ALTERNATE;
	default:
	    return "UNKNOWN";
    }
}
/*
** Name: getDsnTypeString
**
** Description:
**      Return a string that corresponds to the DSN display value.
**
** Input:
**	dsnDisplayType  Display type.
**
** Output:
**	none.
**
** Returns:
**	The string that corresponds to the display value.
**
** History:
**	 21-jan-04 (loera01)
**	    Created.
*/
char *getDsnTypeString(i4 dsnDisplayType)
{
    switch (dsnDisplayType)
    {
        case USER_DSN:
            return USER;
	case SYSTEM_DSN:
            return SYSTEM;
	case ALTERNATE_DSN:
            return ALTERNATE;
	default:
	    return "UNKNOWN";
    }
}

/*
** Name: copyValue
**
** Description:
**      Copies the attribute value into the attribute member with a matching
**      attribute ID.
**
** Input:
**	attrList       An array of attributes.
**      attrCount      Attribute count.
**      id             Attribute ID.
**      value          Attribute value.
**
** Output:
**	none.
**
** Returns:
**	The ODBC version if successful, otherwise "3.50.0000".
**
** History:
**	 21-jan-04 (loera01)
**	    Created.
*/
void copyValue (ATTR_ID **attrList, i4 attrCount, i4 id, char *value)
{
    i2 j;

    for (j = 0; j < attrCount; j++)
    {
        if (attrList[j]->id == id)
        {
            STcopy(value, attrList[j]->value);
            break;
        }
    }
}

/*
** Name: get_driver_version
**
** Description:
**      Attempts to get the ODBC version from the shared library binary file.
**	Reads the library until a pattern match is found that corresponds 
**      to the driver version.
**
** Input:
**	path         Path and file name of the ODBC driver.
**
** Output:
**	none.
**
** Returns:
**	The ODBC version if successful, otherwise "3.50.0000".
**
** Side effects:
**      Dynamically loads the ODBC driver from the path/file name provided.
**
** History:
**     21-jan-04 (loera01) 
**	    Created.
**     19-oct-2007 (Ralph Loen) Bug 119329
**	    Revised to utilize getDriverVersion() instead of trying to read
**          the driver image.
*/
char *get_driver_version(char *path)
{
    i4 i;
    char *dirPath;
    char buf[MAX_LOC+1], buf1[MAX_LOC+1];           
    PTR dlHandle;
    i2 bufSize;
    FILE *pfile;
    LOCATION loc;
    bool done = FALSE;
    char *p, *sp;
    CL_ERR_DESC clerr;
    char *driver_syms[] = { "getDriverVersion", NULL };
    STATUS status;
    IIODBC_DRIVER_FN IIGetDriverVersion;
    char version[20];

    STcopy(path, buf);

    status = LOfroms(PATH & FILENAME, buf, &loc);
    if (status != OK)
       goto err;

    p = buf;
    /*
    ** Extract the module name of the ODBC driver from the path/filename.
    */
#ifdef VMS
    while(STindex(p, "]", 0)) CMnext(p);
    if (!p)
        while(STindex(p, ":", 0)) CMnext(p);
#else
    while(STindex(p, "/", 0)) CMnext(p);
#endif /* VMS */
    STcopy(p, buf1);

    /*
    ** Load the driver with only the getDriverVersion() routine mapped.
    */
    status = DLload( &loc, buf1, driver_syms, &dlHandle, &clerr );
    if (status != OK)
       goto err;

    status = DLbind(dlHandle, driver_syms[0], (PTR *)&IIGetDriverVersion,
        &clerr);
    if (status != OK)
        goto err;

    /*
    ** Execute getDriverVersion().
    */
    IIGetDriverVersion(version);
    return version;
err:
    return ("3.50.0000");
}

/*
** Name: test_connection
**
** Description:
**      Test data source connection information.  
**
** Input:
**	dsnname        ODBC DSN definiiton name.
**
** Output:
**	none.
**
** Returns:
**	void.
**
** History:
**	 21-jan-04 (loera01)
**	    Created.
*/
VOID test_connection( char *dsnname )
{
    HENV                henv;
    HDBC                hdbc;
    RETCODE             rc;
    SQLINTEGER          handleType;
    SQLHANDLE           handle;
    SQLINTEGER          NativeError;
    SQLCHAR             Msg[512];

    for (;;)
    {
        rc = SQLAllocEnv(&henv);
        if (rc != SQL_SUCCESS)
            break;
        handleType = SQL_HANDLE_ENV;
        handle = henv;
        rc = SQLAllocConnect(henv, &hdbc);
        if (rc != SQL_SUCCESS)
            break;
        handleType = SQL_HANDLE_DBC;
        handle = hdbc;
        rc = SQLConnect(hdbc, (SQLCHAR *)dsnname, SQL_NTS, NULL, SQL_NTS, 
            NULL, SQL_NTS);
        if (rc != SQL_SUCCESS)
            break;
        rc = SQLDisconnect(hdbc);
        if (rc != SQL_SUCCESS)
            break;
        rc = SQLFreeConnect(hdbc);
        if (rc != SQL_SUCCESS)
            break;
        handleType = SQL_HANDLE_ENV;
        handle = henv;
        rc = SQLFreeEnv(henv);
        if (rc != SQL_SUCCESS)
            break;
        display_message("Connection was successful");
        break;
    }
    if (!SQL_SUCCEEDED(rc))
    {
        if (SQL_SUCCEEDED(SQLGetDiagRec(handleType, handle, 1,
            NULL, &NativeError, Msg, sizeof(Msg), NULL)) )
	    display_message((char *)Msg);
        else
            display_message("unspecified ODBC error");
    }
}
/*
** Name: display_err
**
** Description:
**      Display error message text corresponding to the error number.
**
** Input:
**	errMsg        Error message text.
**      status        error number.
**
** Output:
**	none.
**
** Returns:
**	void.
**
** History:
**	 10-feb-04 (loera01)
**	    Created.
*/
void display_err(char *errMsg, STATUS status)
{
    if (OK != ERreport(status, &errMsg[STlength(errMsg)]))
        STprintf(&errMsg[STlength(errMsg)],
        ERget(S_ST0036_reason_unknown), status);
    display_message(errMsg);
}

/*
** Name: allocHandles
**
** Description:
**      Allocate driver and DSN handles
**
** Input:
**	pathType      Configuration path type:
**                        OCFG_USR_PATH, OCFG_ALT_PATH, OCFG_SYS_PATH.
**      altPath       Alternate path for OCFG_ALT_PATH.
**
** Output:
**	drvH          Driver handle.
**      dsnH          DSN handle. 
**
** Returns:
**	status        error status.
**
** History:
**	 19-oct-2007 (Ralph Loen) Bug 119329
**	    Created.
*/

STATUS allocHandles(i2 pathType, char * altPath, PTR *drvH, PTR *dsnH)
{
    STATUS status = OK;

    if (altPath && *altPath)
        while (CMwhite(altPath)) CMnext(altPath);

    /*
    ** Initialize the driver source info.
    */
    if (pathType == OCFG_USR_PATH)
    {
        if (altPath && *altPath)
            openConfig(NULL, OCFG_ALT_PATH, altPath, drvH, &status); 
        else
            openConfig(NULL, OCFG_SYS_PATH, NULL, drvH, &status); 
    }
    else if (pathType == OCFG_SYS_PATH)
        openConfig(NULL, pathType, NULL, drvH, &status); 
    else
        openConfig(NULL, pathType, altPath, drvH, &status); 

    /*
    ** Initialize the data source info.
    */
    if (status == OK)
        openConfig(*drvH, pathType, altPath, dsnH, &status);
    if (status != OK)
    {
        STprintf(errMsg, "Cannot allocate config handles\n");
        display_err(errMsg, status);
    }
    return status;
}

/*
** Name: freeHandles
**
** Description:
**      De-allocate driver and DSN handles
**
** Input:
**	drvH          Driver handle.
**      dsnH          DSN handle. 
**
** Output:            None.
**
** Returns:
**	status        error status.
**
** History:
**	 19-oct-2007 (Ralph Loen) Bug 119329
**	    Created.
*/
STATUS freeHandles( PTR drvH, PTR dsnH)
{
    STATUS status=OK;

    closeConfig(drvH, &status);
    if (status == OK)
        closeConfig(dsnH, &status);
    if (status != OK)
    {
        STprintf(errMsg, "Cannot free config handles\n");
        display_err(errMsg, status);
    }
    return status;
}
