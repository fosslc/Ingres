/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project : CA/OpenIngres Visual DBA: Configuring OpenIngres
**
**    Source : ll.cpp
**    low-level functions for VCBF, derived from the .sc files
**    of the config subdirectory. 
**    adapted by : Noirot-Nerin Francois
** 
**  History:
**	20-Jul-98 noifr01
**	    reflected change for support for files greater than 2GB in
**	    size for the main log file (not the dual log file).
**	    (Replace rcp.file.size with rcp.file.kbytes ).
**	24-Nov-98 cucjo01
**	    Added multiple log file support for primary and dual logs
**	09-dec-98 (cucjo01)
**	    Added ICE Server Page support
**	    Added Gateway Pages for Oracle, Informix, Sybase, MSSQL, and ODBC
**	    * These pages specifically get put under the "Gateway" folder
**	    in new alphabetic order for the folders.
**	16-dec-98 (cucjo01)
**	    Allowed "Startup Count" for ICE to be editable.
**	18-dec-98 (cucjo01)
**	    Fixed refresh problem when starting/stopping ingres with
**	    VDBA running. Moved MessageBox statement after setting
**	    bReadOnly to avoid multiple messages sent during OnTimer() calls
**	22-dec-98 (cucjo01)
**	    Moved transaction log warning messages to logfidlg and logfsdlg
**	    classes
**	19-feb-99 (cucjo01)
**	    Added support for UNIX and NT regarding forward vs back slashes
**	    as well as typecasting CString.Format() calls to work with MAINWIN
**	30-mar-1999 (mcgem01)
**	    Microsoft now have a utils.h file so we've spelled out the path
**	    to our version.
**	15-may-1999 (cucjo01)
**	    Added VCBFll_OnStartupCountChange() to make right pane startup count
**	    editable.  It sets the changed flag and saves the old startup value
**	    in case the user wants to revert back to a previous version.
**	03-jun-1999 (mcgem01)
**	    Use the plural version of the component names for the DBMS, Star
**	    and Net servers.
**	22-jun-1999 (mcgem01)
**	    Clean up clashing message identifiers in previous integration.
**	02-aug-1999 (cucjo01)
**	    Use the plural version of the "Bridge Servers" to sync up with the
**	    other plural component names for the DBMS, Star and Net servers.
**	25-nov-1999 (cucjo01)
**	    Updated includes and Raw Log detection to work with MAINWIN.
**	24-jan-2000 (hanch04)
**	    Updated for UNIX build using mainwin, added FILENAME_SEPARATOR
**	22-feb-2000 (somsa01)
**	    Enabled RMCMD for all platforms.
**	28-feb-2000 (somsa01)
**	    li_size is now of type OFFSET_TYPE.
**	28-feb-2000 (somsa01)
**	    li_size is now of type OFFSET_TYPE.
**	07-mar-2000 (somsa01)
**	    Added putting the PID inside the config.lck . Also, if the
**	    config.lck file exists and the PID does not exist anymore,
**	    then successfully lock the file.
**	15-mar-2000 (somsa01)
**	    In mylock_config_data(), pid needs to be of type PID on UNIX.
**	09-may-2000 (somsa01)
**	    Set appropriate title for EDBC.
**	16-may-2000 (cucjo01)
**	    Added JDBC Server support.
**	25-aug-2000 (cucjo01)
**	    Added disabling of primary/dual log file before destroying 
**	    it so Ingres starts up properly.
**	05-Sep-2000 (hanch04)
**	    replace nat and longnat with i4
**	27-Sep-2000 (noifr01)
**	    (bug 99242) clean-up for DBCS compliance
**	05-Apr-2001 (uk$so01)
**	    (SIR 103548) only allow one vcbf per installation ID
**	01-nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings / errors.
**  02-Oct-2002 (noifr01)
**    (SIR 107587) have VCBF manage DB2UDB gateway (mapping it to
**    oracle gateway code until this is moved to generic management)
**	23-apr-2003 (somsa01)
**	    CRsetPMval() now takes an extra parameter.
** 26-May-2003 (schph01)
**    (bug 110305) "Logging System" component , remove some parameters
**    log_file, dual_log, raw_log_file, raw_dual_file appearing with VCBF
**    and not in CBF
**	03-Jun-2003 (uk$so01)
**	   (SIR #110344), Show the Raw Log information on the Unix Operating System.
**	11-Jun-2003 (schph01)
**	   (bug 109113) in set_resource()  allowed input 'value' to be NULL (empty). so it
**	   can handle the add and delete of the resource.
**	12-Jun-2003 (schph01)
**	   (bug 89635) in VCBFll_tx_OnCreate() function update the parameter
**	   rcp.log.log_file_parts in config.dat.
**	13-Jun-2003 (schph01)
**	   (bug 98561)The 2k page size cache can be turned off.
** 20-jun-2003 (uk$so01)
**    SIR #110389 / INGDBA210
**    Add functionalities for configuring the GCF Security and
**    the security mechanisms.
**	20-Jun-2003 (schph01)
**	   (bug 95887) Added default_page_cache_off(). If the default_page_size
**	   is updated warn the user if the respective cache size has not been
**	   enabled.
**	20-Jun-2003 (schph01)
**	   (bug 110430) in VCBFll_tx_OnDestroy() consitently with CBF only the log
**	   file is removed.
**	02-Jul-2003 (schph01)
**	   (bug 109202) consistently with CBF change 460739
**	   Don't assume that cache_name and cache_sharing are
**	   independent parameters, and fix handling of their values
**	07-Jul-2003 (schph01)
**	   (bug 106858) consistently with CBF change 455479
**	   Vcbf always display the "Security" branch.
**	   if the installation is client-only only "System" and "Mechanisms"
**	   Right panes are displayed.
** 25-Sep-2003 (uk$so01)
**    SIR #105781 of cbf
**    Also fix the problem: When changing something in vcbf and after the confirmation
**    of saving or not, and if you say no, then there is a message:
**    "The value of this setting must be an integer."
**    The variable VCBFgf_bStartupCountChange will resolve the problem.
** 31-Oct-2003 (noifr01)
**    fixed propagation errors upon massive ingres30->main codeline propagation 
** 17-Dec-2003 (schph01)
**    SIR #111462, replace the string, by the equivalence of message define
**    into the *.msg files.
**    Add function DisplayMessageWithParam(), to format and display message,
**    when then there are '%0c'. 
** 12-Mar-2004 (uk$so01)
**    SIR #110013 (Handle the DAS Server)
** 15-May-2004 (lakvi01)
**    BUG #112292, Corrected the inconsistency between CBF and VCBF in calculating the log-file size.
** 18-Jun-2004 (lakvi01)
**	  Commented out calls to mymessage in VCBFll_tx_init function as these pop-ups might
**	  be misleading in certain scenarios.
** 19-May-2004 (lakvi01)
**    BUG #111984 Porting cluster support changes of CBF (465499) to VCBF thereby
**	  enabling creation of log-file from VCBF. (after destroying it)
** 07-Sep-2004 (uk$so01)
**    BUG #112979, The gateways can be up without having DBMS server installed.
**    So, show the gateways independently on DBMS server.
** 10-Nov-2004 (noifr01)
**    (bug 113412) replaced usage of hardcoded strings (such as "(default)"
**    for a configuration name), with that of data from the message file
** 21-Feb-2007 (bonro01)
**    Remove JDBC package.
** 18-Mar-2009 (whiro01) SIR 120457, SD 133942
**    From change 492152 (and 494108) by Ralph Loen:
**     migrate the change from CBF into VCBF:
**          Added format_port_rollups() to configure new port syntax for
**          TCP/IP.
**     Renamed to VCBF_format_port_rollups()
** 02-Apr-2009 (drivi01)
**    Clean up the warnings.
*/

#include "stdafx.h"
#if defined (MAINWIN)
#define UNIX
#endif

//#define WORKGROUP_EDITION //to be #defined at the makefile or environment level

#ifndef MAINWIN
#define NT_INTEL 
#define IMPORT_DLL_DATA
#define DESKTOP 
#define DEVL 
#define INCLUDE_ALL_CL_PROTOS  
#endif

#define CL_DONT_SHIP_PROTOS   // For parameters of MEfree

#ifndef MAINWIN
#define NT_GENERIC  
#endif

#ifndef MAINWIN
#define FILENAME_SEPARATOR '\\'
#define FILENAME_SEPARATOR_STR "\\"
#else
#define FILENAME_SEPARATOR '/'
#define FILENAME_SEPARATOR_STR "/"
#include <windows.h>
#endif

#define SEP
#define SEPDEBUG

#ifdef __cplusplus

#ifdef C42
typedef int bool;
#endif // C42

#ifdef OIDSK
char *SystemLocationVariable     = "II_SYSTEM";
char *SystemLocationSubdirectory = "ingres";
//char *SystemProductName          ;
char *SystemVarPrefix            = "II";
char *SystemCfgPrefix            ="ii";
char *SystemExecName             ="ing";
#endif
extern "C"
{
#endif

# include <compat.h>
# include <gl.h>
# include <si.h>
# include <ci.h>
# include <qu.h>
# include <st.h>
# include <me.h>
# include <lo.h>
# include <ex.h>
# include <er.h>
# include <cm.h>
# include <nm.h>
# include <gl.h>
# include <sl.h>
# include <iicommon.h>
# include <gcccl.h>
# include <fe.h>
# include <ug.h>
# include <te.h>
# include <gc.h>
# include <gcccl.h>
# include <erst.h>
# include <uigdata.h>
# include <stdprmpt.h>
# include <pc.h>
# include <pm.h>
# include <pmutil.h>
# include <util.h>
# include <simacro.h>
# include <cm.h>
# include <cv.h>
# include <ug.h>
# include <id.h>
# include "cr.h"
# include "config.h"
//needed for clusters
# include <cscl.h>
# include <cx.h>

FUNC_EXTERN STATUS	CRresetcache();
FUNC_EXTERN bool	unary_constraint( CR_NODE *n, u_i4 constraint_id );
FUNC_EXTERN u_i4	CRtype( i4 idx );
#ifdef __cplusplus
}
#endif

#include "ll.h"
#include <tchar.h>


/* O.I. Desktop section */

#define SPECIAL_NONE             0
#define SPECIAL_SERVERNAME       1
#define SPECIAL_DEF_PATH_DESKTOP 2
#define SPECIAL_DEF_PATH_INGTMP  3
#define SPECIAL_MAX_OPT          4
#define SPECIAL_NUM_MINMAX       5

#ifndef DI_INCLUDED
#define DI_FILENAME_MAX 36
#endif

typedef struct oidsk_inifile_info {
    char * lpname        ;
    char * lpinifileparm ;
    char * lpunit        ;
    char * lpdefaultvalue;
    int    ispecialtype  ;
    long   lmin          ; //applies if ispecialtype== SPECIAL_NUM_MINMAX
    long   lmax          ; //applies if ispecialtype== SPECIAL_NUM_MINMAX
    char * lpsection;
  } OIDSKINIFILEINFO, FAR *LPOIDSKINIFILEINFO;

static void GetIniFileParm(LPOIDSKINIFILEINFO lp1, char * bufresu, int bufresulen);

/* end of O.I. Desktop section */
bool default_page_cache_off(char *pm_id, i4 *page_size, char *cache_share,
                             char *cache_name);
void VCBF_format_port_rollups ( char *serverType, char *instance, char *countStr);

#ifdef EDBC
static char *stdtitle = "EDBC Configuration Manager";
#else
static char *stdtitle = "Ingres Configuration Manager";
#endif /* EDBC */

static void mymessage(char *buf)
{
   MessageBox(GetFocus(),buf, NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
}
static void mymessagewithtitle(char *buf)
{
   MessageBox(GetFocus(),buf, stdtitle, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
}
static void mymessage2b(char *buf1,char * buf2)
{
   char buf[400];
   sprintf(buf,buf1,buf2);
   MessageBox(GetFocus(),buf, NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
}
/*
**
** Format and display the message, for string with '%0c' parameter
*/
static void DisplayMessageWithParam (i4 iMessageNum, TCHAR *strParam)
{
	STATUS ret_status;
	char msg_buf[ER_MAX_LEN];
	i4 language;
	i4 len;
	CL_ERR_DESC cl_err;
	ER_ARGUMENT er_args;
	int msg_len = ER_MAX_LEN;

	if( ERlangcode( NULL, &language ) != OK )
		return;

	er_args.er_size  = STlength(strParam);
	er_args.er_value = strParam;

	/* Format the message with parameter */
	ret_status = ERlookup( iMessageNum, (CL_SYS_ERR*)NULL, ER_TEXTONLY, NULL, msg_buf,
	                       msg_len, language, &len, &cl_err, 1, &er_args);
	if (ret_status!= OK)
		return;

	mymessage(msg_buf);
}

static int QuestionMessageWithParam(i4 iMessageNum, TCHAR *strParam)
{
	int result;
	STATUS ret_status;
	char msg_buf[ER_MAX_LEN];
	i4 language;
	i4 len;
	CL_ERR_DESC cl_err;
	ER_ARGUMENT er_args;
	int msg_len = ER_MAX_LEN;

	if( ERlangcode( NULL, &language ) != OK )
		return IDNO;

	er_args.er_size  = STlength(strParam);
	er_args.er_value = strParam;

	ret_status = ERlookup( iMessageNum, (CL_SYS_ERR*)NULL, ER_TEXTONLY, NULL, msg_buf,
	                       msg_len, language, &len, &cl_err, 1, &er_args);
	if (ret_status!= OK)
		return IDNO;

	result = MessageBox( GetFocus(),msg_buf,stdtitle,
	                     MB_ICONQUESTION | MB_YESNO |MB_TASKMODAL);
	return result;
}

static LOCATION	mutex_loc;
static char	mutex_locbuf[ MAX_LOC+ 1 ];
static char	*mutex_file;


/*
** Tries to lock config.dat by creating config.lck.
*/
static STATUS
mylock_config_data( char *application )
{
	PM_CONTEXT	*context, *context1;
	char		*username;
	char		buf[ 100 ]; 
	char		host[ GCC_L_NODE + 1 ];
	SYSTIME		timestamp;
#ifdef NT_GENERIC
	long		pid;
#else
	PID		pid;
#endif /* NT_GENERIC */

	/* insert lock data into PM context */

	(void) PMmInit( &context );

	PMmLowerOn( context );

	if( application != NULL )
		PMmInsert( context, ERx( "application" ), application );

	/* get user name */
	username = (char *) MEreqmem( 0, 100, FALSE, NULL );
	IDname( &username );
	PMmInsert( context, ERx( "user" ), username );
	MEfree( username );

	/* get terminal id */
	if( TEname( buf ) == OK )
		PMmInsert( context, ERx( "terminal" ), buf );

	/* get host name */
	GChostname( host, sizeof( host ) );
	PMmInsert( context, ERx( "host" ), host );

	/* get timestamp */
	TMnow( &timestamp );
	TMstr( &timestamp, buf );
	PMmInsert( context, ERx( "timestamp" ), buf );

	/* get process id */
	PCpid(&pid);
	CVla(pid, buf);
	PMmInsert( context, ERx( "pid" ), buf );


# ifdef VMS
# define MUTEX_FILE	ERx( "config.lck;1" )
# else /* VMS */
# define MUTEX_FILE	ERx( "config.lck" )
# endif /* VMS */

	/* prepare mutex file LOCATION */
	NMloc( FILES, FILENAME, MUTEX_FILE, &mutex_loc );
	LOcopy( &mutex_loc, mutex_locbuf, &mutex_loc );
	LOtos( &mutex_loc, &mutex_file );

	/*
        ** if config.lck can be opened for reading, assume that another
	** invocation of cbf has config.dat "locked".
	*/
	PMmInit( &context1 );
	PMmLowerOn( context1 );
	if( PMmLoad( context1, &mutex_loc, NULL ) == OK )
	{
		char *value;

		/* See if the PID that had this file locked still exists */
		if( PMmGet( context1, ERx( "pid" ), &value ) == OK )
		{
		    CVal(value, &pid);
		    if (!PCis_alive(pid))
			goto procdead;
		}

		mymessage( "The configuration file is locked by another application (config.lck exists)");
#if 0
		if( PMmGet( context1, ERx( "application" ), &value ) == OK )
		{
			SIprintf( "\t%-15s%s\n", ERx( "application:" ),
				value );
		}
		if( PMmGet( context1, ERx( "host" ), &value ) == OK )
			SIprintf( ERx( "\t%-15s%s\n" ), "host:", value );
		if( PMmGet( context1, ERx( "user" ), &value ) == OK )
			SIprintf( ERx( "\t%-15s%s\n" ), "user:", value );
		if( PMmGet( context1, ERx( "terminal" ), &value ) == OK )
			SIprintf( ERx( "\t%-15s%s\n" ), "terminal:", value );
		if( PMmGet( context1, ERx( "timestamp" ), &value ) == OK )
			SIprintf( ERx( "\t%-15s%s\n" ), "time:", value );
		if( PMmGet( context1, ERx( "pid" ), &value ) == OK )
			SIprintf( ERx( "\t%-15s%s\n" ), "pid:", value );
		PRINT( ERx( "\n" ) );
#endif
		PMmFree( context );
		PMmFree( context1 );
		return( FAIL );
	}

procdead:
	/*
	** This locking scheme has a small chance of failing right here 
	** if a second caller tries to open config.lck for reading at
	** this instant. 
	*/

	/* lock config.dat by writing config.lck */
	if( PMmWrite( context, &mutex_loc ) != OK ) {
                mymessage2b("Unable to write %s", mutex_file );
		VCBFRequestExit();
		return (FAIL);
        }

	PMmFree( context );
	PMmFree( context1 );
	
	return( OK );
}


/*
** Tries to lock config.dat by creating config.lck.
*/
static void
myunlock_config_data( void )
{
	LOdelete( &mutex_loc );
}

#define MAXCOMPONENTS 200
static COMPONENTINFO VCBFComponents[MAXCOMPONENTS];
static int VCBFComponentCount=-1;

#define MAXGENERICLINES 400
static GENERICLINEINFO VCBFGenLinesInfo[MAXGENERICLINES];
static GENERICLINEINFO VCBFGenCacheLinesInfo[MAXGENERICLINES];
static GENERICLINEINFO VCBFAdvLinesInfo[MAXGENERICLINES];
static int VCBFGenLines=-1;
static int VCBFGenCacheLines=-1;
static int VCBFAdvLines=-1;

#define MAXDERIVEDLINES 400
static DERIVEDLINEINFO VCBFGenDerivLinesInfo[MAXDERIVEDLINES];
static int VCBFDerivLines=-1;

#define MAX_AUDIT_LOGS 256 /* Max number of audit logs */
static AUDITLOGINFO VCBFAuditLogLinesInfo[MAX_AUDIT_LOGS];
static int VCBFAuditLogLines=-1;

#define MAX_CACHE_LINES 40 /* Max number of caches to be configured*/
static CACHELINEINFO VCBFCacheLinesInfo[MAX_CACHE_LINES];
static int VCBFCacheLines=-1;

#define MAX_NETPROT_LINES 40 
static NETPROTLINEINFO VCBFNetProtLinesInfo[MAX_NETPROT_LINES];
static int VCBFNetProtLines=-1;

#define MAX_BRIDGEPROT_LINES 40 
static BRIDGEPROTLINEINFO VCBFBridgeProtLinesInfo[MAX_BRIDGEPROT_LINES];
static int VCBFBridgeProtLines=-1;
 
GLOBALDEF char  *host;
GLOBALDEF char  *node; //in case of clusters
GLOBALDEF char  *ii_system;
GLOBALDEF char  *ii_installation;
GLOBALDEF char  *ii_system_name;
GLOBALDEF char  *ii_installation_name;
GLOBALDEF char  tty_type[ MAX_LOC+1 ];
GLOBALDEF char  key_map_filename[ MAX_LOC + 1 ];

GLOBALDEF LOCATION      config_file;    
GLOBALDEF LOCATION      protect_file;   
GLOBALDEF LOCATION      units_file;     
GLOBALDEF LOCATION      change_log_file;        
GLOBALDEF LOCATION      key_map_file;   
GLOBALDEF FILE          *change_log_fp;
GLOBALDEF char          change_log_buf[ MAX_LOC + 1 ];  
GLOBALDEF PM_CONTEXT    *config_data;
GLOBALDEF PM_CONTEXT    *protect_data;
GLOBALDEF PM_CONTEXT    *units_data;
GLOBALDEF bool          server;
GLOBALDEF char          tempdir[ MAX_LOC ];
GLOBALDEF bool          ingres_menu=TRUE;

static LOCATION         help_file;      
static PM_CONTEXT       *help_data;
static char             *syscheck_command = NULL;
static FILE             *config_fp;
static FILE             *protect_fp;
GLOBALDEF char		Affinity_Arg[16]={0};//for clusters
static char			cx_node_name_buf[CX_MAX_NODE_NAME_LEN+2];
static char			cx_host_name_buf[CX_MAX_HOST_NAME_LEN+2];
char				fileext[CX_MAX_NODE_NAME_LEN+2];

static bool             bDisplayAllSecurityTabs = TRUE;

static int icomp;
BOOL VCBFllGetFirstComp(LPCOMPONENTINFO lpcomp)
{
	icomp=-1;
	return VCBFllGetNextComp(lpcomp);
}
BOOL VCBFllGetNextComp(LPCOMPONENTINFO lpcomp)
{
	icomp++;
	if (icomp >= VCBFComponentCount)
	   return FALSE;
	*lpcomp=VCBFComponents[icomp];
	return TRUE;
}
static BOOL bGenInfoAlreadyRead= FALSE;

BOOL VCBFllGenLinesRead()
{
    return bGenInfoAlreadyRead;
}

static BOOL bAltInfoAlreadyRead= FALSE;

BOOL VCBFllAltLinesRead()
{
    return bAltInfoAlreadyRead;
}


static int igeninfo;
BOOL VCBFllGetFirstGenericLine(LPGENERICLINEINFO lpgeninfo) 
{
    bGenInfoAlreadyRead = TRUE;
	igeninfo=-1;
	return VCBFllGetNextGenericLine(lpgeninfo);
}
BOOL VCBFllGetNextGenericLine(LPGENERICLINEINFO lpgeninfo) 
{
	igeninfo++;
	if (igeninfo >= VCBFGenLines)
	   return FALSE;
	*lpgeninfo=VCBFGenLinesInfo[igeninfo];

	return TRUE;
}
static int iderivinfo;
BOOL VCBFllGetFirstDerivedLine(LPDERIVEDLINEINFO lpderivinfo) 
{
	iderivinfo=-1;
	return VCBFllGetNextDerivedLine(lpderivinfo);
}
BOOL VCBFllGetNextDerivedLine(LPDERIVEDLINEINFO lpderivinfo) 
{
	iderivinfo++;
	if (iderivinfo >= VCBFDerivLines)
	   return FALSE;
	*lpderivinfo=VCBFGenDerivLinesInfo[iderivinfo];
	return TRUE;
}
BOOL VCBFllGetFirstGenCacheLine(LPGENERICLINEINFO lpgeninfo) 
{
	igeninfo=-1;
	return VCBFllGetNextGenCacheLine(lpgeninfo);
}
BOOL VCBFllGetNextGenCacheLine(LPGENERICLINEINFO lpgeninfo) 
{
	igeninfo++;
	if (igeninfo >= VCBFGenCacheLines)
	   return FALSE;
	*lpgeninfo=VCBFGenCacheLinesInfo[igeninfo];
	return TRUE;
}
BOOL VCBFllGetFirstAdvancedLine(LPGENERICLINEINFO lpgeninfo) 
{
	bAltInfoAlreadyRead=TRUE;
	igeninfo=-1;
	return VCBFllGetNextAdvancedLine(lpgeninfo);
}

BOOL VCBFllGetNextAdvancedLine(LPGENERICLINEINFO lpgeninfo) 
{
	igeninfo++;
	if (igeninfo >= VCBFAdvLines)
	   return FALSE;
	*lpgeninfo=VCBFAdvLinesInfo[igeninfo];
	return TRUE;
}

static int iauditloginfo;
BOOL VCBFllGetFirstAuditLogLine(LPAUDITLOGINFO lpauditlog) 
{
	iauditloginfo=-1;
	return  VCBFllGetNextAuditLogLine(lpauditlog);
}
BOOL VCBFllGetNextAuditLogLine(LPAUDITLOGINFO lpauditlog) 
{
	iauditloginfo++;
	if (iauditloginfo >= VCBFAuditLogLines)
	   return FALSE;
	*lpauditlog=VCBFAuditLogLinesInfo[iauditloginfo];
	return TRUE;
}

int icacheinfo;
BOOL VCBFllGetFirstCacheLine(LPCACHELINEINFO lpcacheline) 
{
	icacheinfo=-1;
	return VCBFllGetNextCacheLine(lpcacheline);
}
BOOL VCBFllGetNextCacheLine(LPCACHELINEINFO lpcacheline) 
{
	icacheinfo++;
	if (icacheinfo>=VCBFCacheLines)
	    return FALSE;
	*lpcacheline=VCBFCacheLinesInfo[icacheinfo];
	return TRUE;
}

int inetprotinfo;
BOOL VCBFllGetFirstNetProtLine(LPNETPROTLINEINFO lpnetprotline) 
{
	inetprotinfo=-1;
	return  VCBFllGetNextNetProtLine(lpnetprotline) ;
}
BOOL VCBFllGetNextNetProtLine(LPNETPROTLINEINFO lpnetprotline)
{
	inetprotinfo++;
	if (inetprotinfo>=VCBFNetProtLines)
	    return FALSE;
	*lpnetprotline=VCBFNetProtLinesInfo[inetprotinfo];
	return TRUE;
}

int ibridgeprotinfo;
BOOL VCBFllGetFirstBridgeProtLine(LPBRIDGEPROTLINEINFO lpbridgeprotline) 
{
	ibridgeprotinfo=-1;
	return  VCBFllGetNextBridgeProtLine(lpbridgeprotline) ;
}
BOOL VCBFllGetNextBridgeProtLine(LPBRIDGEPROTLINEINFO lpbridgeprotline)
{
	ibridgeprotinfo++;
	if (ibridgeprotinfo>=VCBFBridgeProtLines)
	    return FALSE;
	*lpbridgeprotline=VCBFBridgeProtLinesInfo[ibridgeprotinfo];
	return TRUE;
}



static void myinternalerror()
{
   mymessage("internal error");
   VCBFRequestExit();
}
static void myEXdelete()
{
#ifdef VCBF_USE_EXCEPT_HDL
   EXdelete();
#endif
   return;
}

void
display_error( STATUS status, i4 nargs, ER_ARGUMENT *er_args )
{
	i4 language;
	char buf[ BIG_ENOUGH ];
	i4 len;
	CL_ERR_DESC cl_err;

	if( ERlangcode( NULL, &language ) != OK )
		return;
	if( ERlookup( status, NULL, ER_TEXTONLY, NULL, buf, sizeof( buf ),
		language, &len, &cl_err, nargs, er_args ) == OK
	)  {
		mymessage(buf );
		VCBFRequestExit();
		return;
	}
}

STATUS
copy_LO_file( LOCATION *loc1, LOCATION *loc2 )
{
	FILE *fp1, *fp2;
	char buf[ SI_MAX_TXT_REC + 1 ];
	bool loc1_ok = TRUE;

	if( SIfopen( loc1 , ERx( "r" ), SI_TXT, SI_MAX_TXT_REC, &fp1 ) != OK )
		loc1_ok = FALSE;

	if( SIfopen( loc2 , ERx( "w" ), SI_TXT, SI_MAX_TXT_REC, &fp2 ) != OK )
		return( FAIL );

	while( loc1_ok && SIgetrec( buf, sizeof( buf ), fp1 ) == OK )
		SIfprintf( fp2, ERx( "%s" ), buf ); 

	if( loc1_ok && SIclose( fp1 ) != OK )
		return( FAIL );

	if( SIclose( fp2 ) != OK )
		return( FAIL );

	return( OK );
}

STATUS
browse_file( LOCATION *loc, char *text, bool end )
{
	char    buf[ 2* (SI_MAX_TXT_REC + 1) ];
	FILE    *fp;

	if( SIfopen( loc, "r", SI_TXT, SI_MAX_TXT_REC, &fp ) != OK )
		return( FAIL );

	buf[0]=EOS;
	while( SIgetrec( buf+_tcslen(buf), SI_MAX_TXT_REC, fp ) == OK )
	{
		strcat(buf,"\n");
		if (_tcslen(buf)>=SI_MAX_TXT_REC)
		       break;
	}
	(void) SIclose(fp);
	MessageBox(GetFocus(),buf, text, MB_OK | MB_ICONSTOP | MB_TASKMODAL);

	return( OK );
}

bool
set_resource( char *name, char *value )
{
	char in_buf[ BIG_ENOUGH ]; 
	char *constraint, *temp;
#ifndef OIDSK
	char *warnmsg;
#endif
	CL_ERR_DESC cl_err;
	STATUS status;
	LOCATION valid_msg_file;
	char valid_msg_buf[ MAX_LOC + 1 ];
	FILE *valid_msg_fp;
	LOCATION backup_data_file;
	char backup_data_buf[ MAX_LOC + 1 ];
	LOCATION backup_change_log_file;
	char backup_change_log_buf[ MAX_LOC + 1 ];
	char temp_buf[ MAX_LOC + 1 ];
	i4 sym;
	PM_SCAN_REC state;
	char *regexp, *scan_name, *scan_value;
	bool syscheck_required = FALSE;
	char tmp_buf[1024];

#ifdef VCBF_FUTURE
      replace it => exec frs message :ERget( S_ST0324_validating );
#endif

	/* prepare a LOCATION for validation output */
	STcopy( ERx( "" ), temp_buf );
	LOfroms( FILENAME, temp_buf, &valid_msg_file );
	LOuniq( ERx( "cacbfv" ), NULL, &valid_msg_file );
	LOtos( &valid_msg_file, &temp );
	STcopy( tempdir, valid_msg_buf );
	STcat( valid_msg_buf, temp );
# ifdef VMS
	STcat( valid_msg_buf, ERx( ".tmp" ) );
# endif /* VMS */
	LOfroms( PATH & FILENAME, valid_msg_buf, &valid_msg_file );

	if( SIfopen( &valid_msg_file, ERx( "w" ), SI_TXT, SI_MAX_TXT_REC,
		&valid_msg_fp ) != OK )
	{
		DisplayMessageWithParam( S_ST0323_unable_to_open, valid_msg_buf);
		VCBFRequestExit();
		return FALSE;
	}

	if( CRvalidate( name, value, &constraint, valid_msg_fp ) != OK )
	{
		SIclose( valid_msg_fp );
		(void) browse_file( &valid_msg_file,
			ERget( S_ST0325_constraint_violation ), FALSE );
		return( FALSE );
	}
	else
		SIclose( valid_msg_fp );
	(void) LOdelete( &valid_msg_file );

	/* prepare a backup LOCATION for the data file */
	STcopy( ERx( "" ), temp_buf );
	LOfroms( FILENAME, temp_buf, &backup_data_file );
	LOuniq( ERx( "cacbfd2" ), NULL, &backup_data_file );
	LOtos( &backup_data_file, &temp );
	STcopy( tempdir, backup_data_buf ); 
	STcat( backup_data_buf, temp );
	LOfroms( PATH & FILENAME, backup_data_buf, &backup_data_file );
	
	/* prepare a backup LOCATION for the change log */
	STcopy( ERx( "" ), temp_buf );
	LOfroms( FILENAME, temp_buf, &backup_change_log_file );
	LOuniq( ERx( "cacbfc2" ), NULL, &backup_change_log_file );
	LOtos( &backup_change_log_file, &temp );
	STcopy( tempdir, backup_change_log_buf ); 
	STcat( backup_change_log_buf, temp );
	LOfroms( PATH & FILENAME, backup_change_log_buf,
		&backup_change_log_file );

	/* determine whether to run system resource checker */
	STprintf(tmp_buf, ERx("%s.$.cbf.syscheck_mode"), SystemCfgPrefix);
	if( syscheck_command != NULL && PMmGet( config_data,
		tmp_buf, &scan_value ) == OK &&
		ValueIsBoolTrue( scan_value ) )
	{
		/* check whether "syscheck" resources are affected */
		sym = CRidxFromPMsym( name );
		STprintf( in_buf, ERx( "%s.%s.syscheck.%%" ), 
			  SystemCfgPrefix, host );
		regexp = PMmExpToRegExp( config_data, in_buf );
		for( status = PMmScan( config_data, regexp, &state, NULL,
			&scan_name, &scan_value ); status == OK; status =
			PMmScan( config_data, NULL, &state, NULL, &scan_name,
			&scan_value ) )
		{
			i4 syscheck_sym = CRidxFromPMsym( scan_name );

			if( CRdependsOn( syscheck_sym, sym ) )
			{
				syscheck_required = TRUE;
				break;
			}
		}
	}
	
	if( syscheck_required )
	{
		/* write a backup of the data file */ 
		if( PMmWrite( config_data, &backup_data_file ) != OK )
		{
			mymessage(ERget(S_ST0326_backup_failed));
			(void) LOdelete( &backup_data_file );
			return( FALSE );
		}
	
		/* write a backup of the change log */ 
		if( copy_LO_file( &change_log_file, &backup_change_log_file )
			!= OK ) 
		{
			mymessage(ERget(S_ST0326_backup_failed));
			(void) LOdelete( &backup_change_log_file );
			return( FALSE );
		}
	}

	/* open change log file for appending */
	if( SIfopen( &change_log_file , ERx( "a" ), SI_TXT, SI_MAX_TXT_REC,
		&change_log_fp ) != OK )
	{
		DisplayMessageWithParam(S_ST0323_unable_to_open, change_log_buf);
		VCBFRequestExit();
		return FALSE;
	}

#ifdef VCBF_FUTURE
      replace it => exec frs message :ERget( S_ST0311_recalculating );
#endif

	/* set resources and write changes to config.log */
#ifdef OIDSK
	CRsetPMval( name, value, change_log_fp, FALSE, TRUE );
#else
	if (warnmsg = CRsetPMval( name, value, change_log_fp, FALSE, TRUE )) 
	    MessageBox(GetFocus(),warnmsg, stdtitle, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
#endif

	/* close change log after appending */
	(void) SIclose( change_log_fp );

	if( PMmWrite( config_data, NULL ) != OK )
	{
		DisplayMessageWithParam(S_ST0318_fatal_error, ERget( S_ST0314_bad_config_write ));
		VCBFRequestExit();
		return FALSE;
	}
	/* call system resource checker if one exists */ 
	if( syscheck_required )
	{
#ifdef VCBF_FUTURE
   replace it =>exec frs message :ERget( S_ST0316_cbf_syscheck_message );
#endif
		if( PCcmdline( NULL, syscheck_command, PC_WAIT, NULL,
			&cl_err ) != OK )
		{
			/* report syscheck failure */
		       int ret = MessageBox(GetFocus(),
					    ERget(S_ST0317_syscheck_failure),
					    NULL,
				    MB_ICONQUESTION | MB_YESNO |MB_TASKMODAL);
			if (ret == IDYES) 
			{
				PMmFree( config_data );

				(void) PMmInit( &config_data );

				PMmLowerOn( config_data );

				CRsetPMcontext( config_data );

#ifdef VCBF_FUTURE
      replace it =>             exec frs message
					:ERget( S_ST031B_restoring_settings );
#endif

				if( PMmLoad( config_data, &backup_data_file,
					NULL ) != OK )
				{
					mymessage(ERget(S_ST0315_bad_config_read));
					VCBFRequestExit();
					return FALSE;
				}
				/* rewrite config.dat */
				if( PMmWrite( config_data, NULL ) != OK )
				{
					mymessage(ERget(S_ST0314_bad_config_write));
					VCBFRequestExit();
					return FALSE;
				}

				/* delete change log */
				(void) LOdelete( &change_log_file );

				/* restore change log from backup */
				if( copy_LO_file( &backup_change_log_file,
					&change_log_file ) != OK )
				{
					(void) LOdelete(
						&backup_change_log_file );
					mymessage(ERget(S_ST0326_backup_failed));
					VCBFRequestExit();
					return FALSE;
				}

				/* delete backup of data file */
				(void) LOdelete( &backup_data_file );

				/* delete backup of change log */
				(void) LOdelete( &backup_change_log_file );

				return( FALSE );
			}
		}
		/* delete backup of data file */
		(void) LOdelete( &backup_data_file );

		/* delete backup of change log */
		(void) LOdelete( &backup_change_log_file );
	}
	return( TRUE );
}

bool
reset_resource( char *name, char **value,
	bool reset_derived )
{
//      char resource[ BIG_ENOUGH ]; 
	char *symbol;
	i4 idx, len, i;
	char *old_value;

	if( reset_derived )
	{
		/* save old value in order to reset before set_resource */
		(void) PMmGet( config_data, name, &old_value );
		old_value = STalloc( old_value );
	}

	/* delete old value in order to pick up the default */
	PMmDelete( config_data, name );

	/* get rule symbol for resource */ 
	idx = CRidxFromPMsym( name );
	if (idx== -1)
	{
	    /*
	    ** Value not in CR, probably dynamically created.
	    ** For now use the name
	    */
	    symbol=name;
	}
	else 
	{
	    symbol = CRsymbol( idx );
	}
		
	/* set defaults as required by CRcompute() */
	len = PMmNumElem( config_data, symbol );
	for( i = 3; i < len; i++ )
	{
		if( STcompare( PMmGetElem( config_data, i, symbol ),
			ERx( "$" ) ) == 0
		)
			PMmSetDefault( config_data, i, PMmGetElem(
				config_data, i, name ) );
		else
			PMmSetDefault( config_data, i, NULL );
	}

	if(idx!= -1)
	{
	    /* recompute the resource if known */
	    CRresetcache(); 
	    if( CRcompute( idx, value ) != OK )
	    {
		DisplayMessageWithParam(S_ST0318_fatal_error,ERget( S_ST032C_undefined_symbol ));
		VCBFRequestExit();
		return FALSE;
	    }

	    /* copy contents of internal CRcompute() buffer */
	    *value = STalloc( *value );
	}

	if( reset_derived )
	{
		/* reset old value so change log entry is correct */
		PMmInsert( config_data, name, old_value ); 

		/* then call set_resource to update derived */
		return( set_resource( name, *value ) );
	}
	/* otherwise, just set the resource and reload */
	PMmInsert( config_data, name, *value );
	if( PMmWrite( config_data, NULL ) != OK )
	{
		DisplayMessageWithParam(S_ST0318_fatal_error, ERget( S_ST0314_bad_config_write ));
		VCBFRequestExit();
		return FALSE;
	}
	return( TRUE );
}

bool get_help( char *param_id )
{
	char buf[ BIG_ENOUGH], *file, *title;

	/* look up help file name */
	STprintf( buf, ERx( "%s.file" ), param_id );
	if( PMmGet( help_data, buf, &file ) != OK )
		return( FALSE );

	/* look up help title */
	STprintf( buf, ERx( "%s.title" ), param_id );
	if( PMmGet( help_data, buf, &title ) != OK )
		return( FALSE );

	/* display help */
     // shouldn't be called. is there because of call from an uncalled
     // function in config.lib (could normally be cleaned up in the lib)
     // FEhelp( file, title );
	return( TRUE );
}

void
VCBFsession_timestamp( FILE *fp )
{
	char            host[ GCC_L_NODE + 1 ];
	char            *user;
	char            terminal[ 100 ]; 
	char            time[ 100 ]; 
	SYSTIME         timestamp;

	/* get user name */
	user = (char *) MEreqmem( 0, 100, FALSE, NULL );
	IDname( &user );

	/* get terminal id */
	if ( TEname( terminal ) != OK)
	terminal[0]= '\0';

	/* get host name */
	GChostname( host, sizeof( host ) );

	/* get timestamp */
	TMnow( &timestamp );
	TMstr( &timestamp, time );

	SIfprintf( fp, "\n*********************" );
	SIfprintf( fp, "\n*** BEGIN SESSION ***" );
	SIfprintf( fp, "\n*********************\n" );
	SIfprintf( fp, ERx( "\n%-15s%s" ), "host:", host );
	SIfprintf( fp, ERx( "\n%-15s%s" ), "user:", user );
	SIfprintf( fp, ERx( "\n%-15s%s" ), "terminal:", terminal );
	SIfprintf( fp, ERx( "\n%-15s%s\n" ), "time:", time );

  /* Free allocate memory */
  if (MEfree(user) == OK)
      user=NULL;

}

#ifdef VCBF_USE_EXCEPT_HDL
#ifdef __cplusplus
extern "C"
{
#endif
STATUS
cbf_handler( EX_ARGS *args )
{
       //       STATUS          FEhandler();

	myunlock_config_data();
       //       return( FEhandler( args ) );
       return OK; to be checked if handler needed in the future
}
#ifdef __cplusplus
}
#endif
#endif

static void VCBFInitComponents()
{
  int i;
  for (i=0;i<MAXCOMPONENTS;i++)
     VCBFComponents[i].sztype[0]='\0';
  VCBFComponentCount=0;
}

static int VCBFAddComponent(int itype,char *lptype,char *lpname,char *lpenabled,BOOL blog1,BOOL blog2)
{ if (VCBFComponentCount>=MAXCOMPONENTS || VCBFComponentCount<0)
    return -1;
  strcpy(VCBFComponents[VCBFComponentCount].sztype,   lptype);
  strcpy(VCBFComponents[VCBFComponentCount].szname,   lpname);
  strcpy(VCBFComponents[VCBFComponentCount].szcount,lpenabled);
  VCBFComponents[VCBFComponentCount].itype = itype;
  VCBFComponents[VCBFComponentCount].blog1 = blog1;
  VCBFComponents[VCBFComponentCount].blog2 = blog2;
  if (itype == COMP_TYPE_SECURITY)
      VCBFComponents[VCBFComponentCount].bDisplayAllTabs = bDisplayAllSecurityTabs;
  else
      VCBFComponents[VCBFComponentCount].bDisplayAllTabs = TRUE;
  VCBFComponentCount++;
  return VCBFComponentCount;
}
static void VCBFInitGenLines()
{
  int i;
  for (i=0;i<MAXGENERICLINES;i++)
     VCBFGenLinesInfo[i].szname[0]='\0';
  VCBFGenLines=0;
}
static int VCBFAddGenLine(char *lpname, char * lpvalue, char * lpunits, BOOL bIsNum)
{
  if (VCBFGenLines>=MAXGENERICLINES || VCBFGenLines<0)
    return -1;
  strcpy(VCBFGenLinesInfo[VCBFGenLines].szname,   lpname);
  strcpy(VCBFGenLinesInfo[VCBFGenLines].szvalue,  lpvalue);
  strcpy(VCBFGenLinesInfo[VCBFGenLines].szunit,  lpunits);
  VCBFGenLinesInfo[VCBFGenLines].iunittype = GEN_LINE_TYPE_STRING;
  if(!stricmp(lpunits,"boolean"))
     VCBFGenLinesInfo[VCBFGenLines].iunittype = GEN_LINE_TYPE_BOOL;
  if(bIsNum)
     VCBFGenLinesInfo[VCBFGenLines].iunittype = GEN_LINE_TYPE_NUM;

  VCBFGenLines++;
  return VCBFGenLines;
}
static void VCBFInitGenCacheLines()
{
  int i;
  for (i=0;i<MAXGENERICLINES;i++)
     VCBFGenCacheLinesInfo[i].szname[0]='\0';
  VCBFGenCacheLines=0;
}
static int VCBFAddGenCacheLine(char *lpname, char * lpvalue, char * lpunits)
{
  if (VCBFGenCacheLines>=MAXGENERICLINES || VCBFGenCacheLines<0)
    return -1;
  strcpy(VCBFGenCacheLinesInfo[VCBFGenCacheLines].szname,   lpname);
  strcpy(VCBFGenCacheLinesInfo[VCBFGenCacheLines].szvalue,  lpvalue);
  strcpy(VCBFGenCacheLinesInfo[VCBFGenCacheLines].szunit,  lpunits);
  VCBFGenCacheLinesInfo[VCBFGenCacheLines].iunittype = GEN_LINE_TYPE_STRING;
  if(!stricmp(lpunits,"boolean"))
     VCBFGenCacheLinesInfo[VCBFGenCacheLines].iunittype = GEN_LINE_TYPE_BOOL;

  VCBFGenCacheLines++;
  return VCBFGenCacheLines;
}

static void VCBFInitAdvLines()
{
  int i;
  for (i=0;i<MAXGENERICLINES;i++)
     VCBFAdvLinesInfo[i].szname[0]='\0';
  VCBFAdvLines=0;
}
static int VCBFAddAdvLine(char *lpname, char * lpvalue, char * lpunits, BOOL bIsNum)
{
  if (VCBFAdvLines>=MAXGENERICLINES || VCBFAdvLines<0)
    return -1;
  strcpy(VCBFAdvLinesInfo[VCBFAdvLines].szname,   lpname);
  strcpy(VCBFAdvLinesInfo[VCBFAdvLines].szvalue,  lpvalue);
  strcpy(VCBFAdvLinesInfo[VCBFAdvLines].szunit,  lpunits);
  VCBFAdvLinesInfo[VCBFAdvLines].iunittype = GEN_LINE_TYPE_STRING;
  if(!stricmp(lpunits,"boolean"))
     VCBFAdvLinesInfo[VCBFAdvLines].iunittype = GEN_LINE_TYPE_BOOL;
  if(bIsNum)
     VCBFAdvLinesInfo[VCBFAdvLines].iunittype = GEN_LINE_TYPE_NUM;

  VCBFAdvLines++;
  return VCBFAdvLines;
}

static void VCBFInitAuditLogLines()
{
  int i;
  for (i=0;i<MAX_AUDIT_LOGS;i++)
     VCBFAuditLogLinesInfo[i].szvalue[0]='\0';
  VCBFAuditLogLines=0;
}
static int VCBFAddAuditLogLine(int iline, char * lpvalue)
{
  if (VCBFAuditLogLines>=MAX_AUDIT_LOGS || VCBFAuditLogLines<0)
    return -1;
  VCBFAuditLogLinesInfo[VCBFAuditLogLines].log_n=iline;
  strcpy(VCBFAuditLogLinesInfo[VCBFAuditLogLines].szvalue,lpvalue);
  VCBFAuditLogLines++;
  return VCBFAuditLogLines;
}

static void VCBFInitCacheLines()
{
  int i;
  for (i=0;i<MAX_CACHE_LINES;i++)
     VCBFCacheLinesInfo[i].szcachename[0]='\0';
  VCBFCacheLines=0;
}
static int VCBFAddCacheLine(char * cachename, char * status)
{
  if (VCBFCacheLines>=MAX_CACHE_LINES || VCBFCacheLines<0)
    return -1;
  strcpy(VCBFCacheLinesInfo[VCBFCacheLines].szcachename,cachename);
  strcpy(VCBFCacheLinesInfo[VCBFCacheLines].szstatus   ,status);
  VCBFCacheLines++;
  return VCBFCacheLines;
}                      
static void VCBFInitDerivLines()
{
  int i;
  for (i=0;i<MAXDERIVEDLINES;i++)
     VCBFGenDerivLinesInfo[i].szname[0]='\0';
  VCBFDerivLines=0;
}
static int VCBFAddDerivLine(char *lpname, char * lpvalue, char * lpunits, char * lpprotect)
{
  if (VCBFDerivLines>=MAXDERIVEDLINES || VCBFDerivLines<0)
    return -1;
  strcpy(VCBFGenDerivLinesInfo[VCBFDerivLines].szname,       lpname);
  strcpy(VCBFGenDerivLinesInfo[VCBFDerivLines].szvalue,      lpvalue);
  strcpy(VCBFGenDerivLinesInfo[VCBFDerivLines].szunit,       lpunits);
  strcpy(VCBFGenDerivLinesInfo[VCBFDerivLines].szprotected,  lpprotect);
  VCBFGenDerivLinesInfo[VCBFDerivLines].iunittype = GEN_LINE_TYPE_STRING;
  if(!stricmp(lpunits,"boolean"))
     VCBFGenDerivLinesInfo[VCBFDerivLines].iunittype = GEN_LINE_TYPE_BOOL;

  VCBFDerivLines++;
  return VCBFDerivLines;
}
static void VCBFInitNetProtLines()
{
  int i;
  for (i=0;i<MAX_NETPROT_LINES;i++)
     VCBFNetProtLinesInfo[i].szprotocol[0]='\0';
  VCBFNetProtLines=0;
}
static int VCBFAddNetProtLine(char * protocol, char * status, char * listen)
{
  if (VCBFNetProtLines>=MAX_NETPROT_LINES || VCBFNetProtLines<0)
    return -1;
  strcpy(VCBFNetProtLinesInfo[VCBFNetProtLines].szprotocol,protocol);
  strcpy(VCBFNetProtLinesInfo[VCBFNetProtLines].szstatus,status);
  strcpy(VCBFNetProtLinesInfo[VCBFNetProtLines].szlisten,listen);
  VCBFNetProtLines++;
  return VCBFNetProtLines;
}                      

static void VCBFInitBridgeProtLines()
{
  int i;
  for (i=0;i<MAX_BRIDGEPROT_LINES;i++)
     VCBFBridgeProtLinesInfo[i].szprotocol[0]='\0';
  VCBFBridgeProtLines=0;
}
static int VCBFAddBridgeProtLine(char * protocol, char * status, char * listen, char * vnode)
{
  if (VCBFBridgeProtLines>=MAX_BRIDGEPROT_LINES || VCBFBridgeProtLines<0)
    return -1;
  strcpy(VCBFBridgeProtLinesInfo[VCBFBridgeProtLines].szprotocol,protocol);
  strcpy(VCBFBridgeProtLinesInfo[VCBFBridgeProtLines].szstatus,  status);
  strcpy(VCBFBridgeProtLinesInfo[VCBFBridgeProtLines].szlisten,  listen);
  strcpy(VCBFBridgeProtLinesInfo[VCBFBridgeProtLines].szvnode,   vnode);
  VCBFBridgeProtLines++;
  return VCBFBridgeProtLines;
}                      


static char szbuf1[400],szbuf2[400],szbuf3[400];
static char * dupstr1(char * p)
{
	strcpy(szbuf1,p);
	return szbuf1;
}
static char * dupstr2(char * p)
{
	strcpy(szbuf2,p);
	return szbuf2;
}
static char * dupstr3(char * p)
{
	strcpy(szbuf3,p);
	return szbuf3;
}

static STATUS do_csinstall()
{
# ifdef UNIX
	CL_ERR_DESC cl_err;
	char	cmd[80]={0};

	STprintf( cmd, ERx( "csinstall %s >/dev/null" ), Affinity_Arg );
	return ( PCcmdline((LOCATION *) NULL, cmd,
		PC_WAIT, (LOCATION *) NULL, &cl_err ) );
# else
	return OK;
# endif /* UNIX */
}

static STATUS call_rcpstat( char *arguments )
{
	CL_ERR_DESC cl_err;
	char	cmd[80+CX_MAX_NODE_NAME_LEN];
	char	*target, *arg;
	char	*redirect;

# ifdef VMS
	redirect = ERx( "" );
# else /* VMS */
# ifdef NT_GENERIC 
	redirect = ERx( ">NUL" );
# else
	redirect = ERx( ">/dev/null" );
# endif /* NT_GENERIC */
# endif /* VMS */

	target = arg = ERx( "" );

	if ( CXcluster_enabled() && STcompare( node, /*local_host*/host ) != 0 )
	{
		target = node;
		arg = ERx( "-node" );
	}

	STprintf( cmd, ERx( "rcpstat %s %s %s %s %s" ), Affinity_Arg,
		arg, target, arguments, redirect );
	return PCcmdline( (LOCATION *) NULL, cmd, 
		PC_WAIT, (LOCATION *) NULL, &cl_err );
}

static STATUS call_rcpconfig( char *arguments )
{
	CL_ERR_DESC cl_err;
	char	cmd[80+CX_MAX_NODE_NAME_LEN];
	char	*target, *arg;
	char	*redirect;

# ifdef VMS
	redirect = ERx( "" );
# else /* VMS */
# ifdef NT_GENERIC 
	redirect = ERx( ">NUL" );
# else
	redirect = ERx( ">/dev/null" );
# endif /* NT_GENERIC */
# endif /* VMS */

	target = arg = ERx( "" );

	if ( CXcluster_enabled() && STcompare( node, /*local_host*/host ) != 0 )
	{
		target = node;
		arg = ERx( "-node" );
	}

	STprintf( cmd, ERx( "rcpconfig  %s %s %s %s %s" ), Affinity_Arg,
		arg, target, arguments, redirect );
	return PCcmdline( (LOCATION *) NULL, cmd, 
		PC_WAIT, (LOCATION *) NULL, &cl_err );
}

static void do_ipcclean(void)
{
# ifdef UNIX
	CL_ERR_DESC cl_err;
	char	cmd[80]={0};

	STprintf( cmd, ERx( "ipcclean %s" ), Affinity_Arg );
	(void) PCcmdline((LOCATION *) NULL, cmd,
		PC_WAIT, (LOCATION *) NULL, &cl_err );
# endif /* UNIX */
}

static BOOL updateComponentList( void )
{
	char *name, *value;
//      char comp_type[ BIG_ENOUGH ];
//      char comp_name[ BIG_ENOUGH ];
//      char enabled_val[ BIG_ENOUGH ];
//      char in_buf[ BIG_ENOUGH ];
	char *enabled1;
	char *enabled2;
	i4 log1_row, log2_row, row = 0;
	char temp_buf[ BIG_ENOUGH ];

	PM_SCAN_REC state;
	STATUS status;
	char *regexp;
	char expbuf[ BIG_ENOUGH ];
//      char pm_id[ BIG_ENOUGH ];
	VCBFInitComponents();

	VCBFAddComponent(COMP_TYPE_NAME,
			         dupstr1(ERget( S_ST0300_name_server )),
			         dupstr2(ERget( S_ST030F_default_settings )),
			         dupstr3(ERx( "1" )),FALSE,FALSE) ;
	
	if(!ingres_menu) 
	   VCBFAddComponent(COMP_TYPE_LOCKING_SYSTEM,
			            dupstr1(ERget( S_ST0304_locking_system )),
			            dupstr2(ERget( S_ST030F_default_settings )),
			            dupstr3(ERx( "1" )),FALSE,FALSE) ;

	if( server )
	{
# ifdef UNIX
		bool csinstall = FALSE; 
# endif /* UNIX */

		/* examine transaction logs */
# ifndef JASMINE
#ifdef VCBF_FUTURE
	replace it => exec frs message :ERget( S_ST038A_examining_logs ); 
#endif

		STATUS csinstall;
		csinstall = do_csinstall();

		if( OK == call_rcpstat( ERx( "-exist -silent" )) && 
			OK == call_rcpstat( ERx( "-enable -silent" )) )
		{
			enabled1 = ERx( "1" );  
		}
		else
			enabled1 = ERx( "0" );  

		if(OK == call_rcpstat( ERx( "-exist -dual -silent" ) ) &&
			OK == call_rcpstat( ERx( "-enable -dual -silent" ) ))
		{
			enabled2 = ERx( "1" );  
		}
		else
			enabled2 = ERx( "0" );  

		if ( OK == csinstall)
		{
			do_ipcclean();
		}
# endif /* JASMINE */
	}


	/* display always the "Security" menu item but if security auditing is
	not setup only two tabs are availables "Systems" and "Mechanisms" */ 
	STprintf( temp_buf, ERx( "%s.$.c2.%%" ), SystemCfgPrefix );
	regexp = PMmExpToRegExp( config_data, temp_buf );
	if( !server || 
		PMmScan( config_data, regexp, &state, NULL, &name, &value )
		!= OK )
	{
		bDisplayAllSecurityTabs=FALSE;
	}
	else
		bDisplayAllSecurityTabs=TRUE;

    VCBFAddComponent(COMP_TYPE_SECURITY,
				     dupstr1(ERget( S_ST034E_security_menu )), 
				     dupstr2(ERget( S_ST030F_default_settings)),
				     dupstr3(ERx( "1" )),FALSE,FALSE) ;

	/* check for ICE definition */
	STprintf( expbuf, ERx( "%s.%s.%sstart.%%.icesvr" ),
                   SystemCfgPrefix, host, SystemExecName );
	regexp = PMmExpToRegExp( config_data, expbuf );
	if ( PMmScan( config_data, regexp, &state, NULL, &name, &value )
		== OK )
	{ name = PMmGetElem( config_data, 3, name );
      if( STcompare( name, ERx( "*" ) ) == 0 )
          name = dupstr1(ERget( S_ST030F_default_settings));
	  
      VCBFAddComponent(COMP_TYPE_INTERNET_COM,
	  			       ERget(S_ST02FF_internet_comm),
	  			       name,value,FALSE,FALSE);
	}

    if (/*server*/1) /*Gateway can be up without having DBMS Server !*/
    { 
      /* check for Oracle Gateway definition */
      STprintf( expbuf, ERx( "%s.%s.%sstart.%%.oracle" ), SystemCfgPrefix, host, SystemExecName );
	  regexp = PMmExpToRegExp( config_data, expbuf );
	  for( status = PMmScan( config_data, regexp, &state, NULL,
	  &name, &value ); status == OK; status =
	  PMmScan( config_data, NULL, &state, NULL, &name, &value ) )
      { name = PMmGetElem( config_data, 3, name );
	    if( STcompare( name, ERx( "*" ) ) == 0 )
	       name = dupstr1(ERget( S_ST030F_default_settings));

		VCBFAddComponent(COMP_TYPE_GW_ORACLE,
                         ERget(S_ST055C_oracle_gateway),
                         name,value,FALSE,FALSE);
	  }

      /* check for DB2UDB Gateway definition */
      STprintf( expbuf, ERx( "%s.%s.%sstart.%%.db2udb" ), SystemCfgPrefix, host, SystemExecName );
	  regexp = PMmExpToRegExp( config_data, expbuf );
	  for( status = PMmScan( config_data, regexp, &state, NULL,
	  &name, &value ); status == OK; status =
	  PMmScan( config_data, NULL, &state, NULL, &name, &value ) )
      { name = PMmGetElem( config_data, 3, name );
	    if( STcompare( name, ERx( "*" ) ) == 0 )
	       name = dupstr1(ERget( S_ST030F_default_settings));

		VCBFAddComponent(COMP_TYPE_GW_DB2UDB,
                         ERget(S_ST0677_db2udb_gateway),
                         name,value,FALSE,FALSE);
	  }

      /* check for Informix Gateway definition */
      STprintf( expbuf, ERx( "%s.%s.%sstart.%%.informix" ), SystemCfgPrefix, host, SystemExecName );
	  regexp = PMmExpToRegExp( config_data, expbuf );
	  for( status = PMmScan( config_data, regexp, &state, NULL,
	  &name, &value ); status == OK; status =
	  PMmScan( config_data, NULL, &state, NULL, &name, &value ) )
      { name = PMmGetElem( config_data, 3, name );
	    if( STcompare( name, ERx( "*" ) ) == 0 )
	       name = dupstr1(ERget( S_ST030F_default_settings));

		VCBFAddComponent(COMP_TYPE_GW_INFORMIX,
                         ERget(S_ST055F_informix_gateway),
                         name,value,FALSE,FALSE);
	  }

      /* check for Sybase Gateway definition */
      STprintf( expbuf, ERx( "%s.%s.%sstart.%%.sybase" ), SystemCfgPrefix, host, SystemExecName );
	  regexp = PMmExpToRegExp( config_data, expbuf );
	  for( status = PMmScan( config_data, regexp, &state, NULL,
	  &name, &value ); status == OK; status =
	  PMmScan( config_data, NULL, &state, NULL, &name, &value ) )
      { name = PMmGetElem( config_data, 3, name );
	    if( STcompare( name, ERx( "*" ) ) == 0 )
	       name = dupstr1(ERget( S_ST030F_default_settings));

		VCBFAddComponent(COMP_TYPE_GW_SYBASE,
                         ERget(S_ST0563_sybase_gateway),
                         name,value,FALSE,FALSE);
	  }

      /* check for MSSQL Gateway definition */
      STprintf( expbuf, ERx( "%s.%s.%sstart.%%.mssql" ), SystemCfgPrefix, host, SystemExecName );
	  regexp = PMmExpToRegExp( config_data, expbuf );
	  for( status = PMmScan( config_data, regexp, &state, NULL,
	  &name, &value ); status == OK; status =
	  PMmScan( config_data, NULL, &state, NULL, &name, &value ) )
      { name = PMmGetElem( config_data, 3, name );
	    if( STcompare( name, ERx( "*" ) ) == 0 )
	       name = dupstr1(ERget( S_ST030F_default_settings));

		VCBFAddComponent(COMP_TYPE_GW_MSSQL,
                         ERget(S_ST0566_mssql_gateway),
                         name,value,FALSE,FALSE);
	  }

      /* check for ODBC Gateway definition */
      STprintf( expbuf, ERx( "%s.%s.%sstart.%%.odbc" ), SystemCfgPrefix, host, SystemExecName );
	  regexp = PMmExpToRegExp( config_data, expbuf );
	  for( status = PMmScan( config_data, regexp, &state, NULL,
	  &name, &value ); status == OK; status =
	  PMmScan( config_data, NULL, &state, NULL, &name, &value ) )
      { name = PMmGetElem( config_data, 3, name );
	    if( STcompare( name, ERx( "*" ) ) == 0 )
	       name = dupstr1(ERget( S_ST030F_default_settings));

		VCBFAddComponent(COMP_TYPE_GW_ODBC,
                         ERget(S_ST0569_odbc_gateway),
                         name,value,FALSE,FALSE);
	  }

    }

	if (server)       /* scan Star definitions */
	{ STprintf( expbuf, ERx( "%s.%s.%sstart.%%.star" ), 
		  SystemCfgPrefix, host, SystemExecName );
      regexp = PMmExpToRegExp( config_data, expbuf );
	  for( status = PMmScan( config_data, regexp, &state, NULL,
 	       &name, &value ); status == OK; status =
		   PMmScan( config_data, NULL, &state, NULL, &name,
		   &value ) )
	  {
		name = PMmGetElem( config_data, 3, name );
		if( STcompare( name, ERx( "*" ) ) == 0 )
			name = dupstr1(ERget( S_ST030F_default_settings));

		VCBFAddComponent(COMP_TYPE_STAR,
				         ERget( S_ST063E_star_servers ),
				         name,value,FALSE,FALSE);
	  }
    }
  
	/* scan Net definitions */
	STprintf( expbuf, ERx( "%s.%s.%sstart.%%.gcc" ), 
		  SystemCfgPrefix, host, SystemExecName );
	regexp = PMmExpToRegExp( config_data, expbuf );
	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		name = PMmGetElem( config_data, 3, name );
		if( STcompare( name, ERx( "*" ) ) == 0 )
			name = dupstr1(ERget( S_ST030F_default_settings));

		VCBFAddComponent(COMP_TYPE_NET,
				         ERget( S_ST063D_net_servers ), 
				         name,value,FALSE,FALSE);
	}

	/* scan das server definitions */
	STprintf( expbuf, ERx( "%s.%s.%sstart.%%.gcd" ), 
		  SystemCfgPrefix, host, SystemExecName );
	regexp = PMmExpToRegExp( config_data, expbuf );
	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		name = PMmGetElem( config_data, 3, name );
		if( STcompare( name, ERx( "*" ) ) == 0 )
			name = dupstr1(ERget( S_ST030F_default_settings ));

		VCBFAddComponent(COMP_TYPE_DASVR,
				         ERget( S_ST069B_das_servers ), 
				         name,value,FALSE,FALSE);
	}

    if (server)       /* scan DBMS definitions */
	{ 
	   STprintf( expbuf, ERx( "%s.%s.%sstart.%%.dbms" ), SystemCfgPrefix, host, SystemExecName );
	   regexp = PMmExpToRegExp( config_data, expbuf );
	   for( status = PMmScan( config_data, regexp, &state, NULL,
            &name, &value ); status == OK; status =
		    PMmScan( config_data, NULL, &state, NULL, &name, &value ) )
	   {
	      name = PMmGetElem( config_data, 3, name );
	      if( STcompare( name, ERx( "*" ) ) == 0 )
              name = dupstr1(ERget( S_ST030F_default_settings ));

          VCBFAddComponent(COMP_TYPE_DBMS,
		                   ERget( S_ST063C_dbms_servers ), 
				           name, value,FALSE,FALSE);
	   }       
	}

	/* scan Bridge definitions */
	STprintf( expbuf, ERx( "%s.%s.%sstart.%%.gcb" ), 
		  SystemCfgPrefix, host, SystemExecName );
	regexp = PMmExpToRegExp( config_data, expbuf );
	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		name = PMmGetElem( config_data, 3, name );
		if( STcompare( name, ERx( "*" ) ) == 0 )
			name = dupstr1(ERget( S_ST030F_default_settings));

		VCBFAddComponent(COMP_TYPE_BRIDGE,
				         ERget( S_ST063F_bridge_servers ),
				         name,value,FALSE,FALSE);
	}


# ifndef JASMINE
	if( server )
	{

#ifndef OIDSK		
		/* scan Remote Command definitions */
		STprintf( expbuf, ERx( "%s.%s.%sstart.%%.rmcmd" ), 
			  SystemCfgPrefix, host, SystemExecName );
		regexp = PMmExpToRegExp( config_data, expbuf );
		status=PMmScan( config_data, regexp, &state, NULL,
			&name, &value); 
		if (status)
			value="0";
		name = dupstr1(ERget( S_ST030A_not_configurable ));
		VCBFAddComponent(COMP_TYPE_RMCMD,
					     ERget( S_ST02FE_rmcmd ),
					     name,value,FALSE,FALSE);

#endif
		VCBFAddComponent(COMP_TYPE_LOCKING_SYSTEM,
				 dupstr1(ERget( S_ST0304_locking_system )), 
				 dupstr2(ERget( S_ST030F_default_settings)),
				 dupstr3(ERx( "1" )),FALSE,FALSE) ;

		VCBFAddComponent(COMP_TYPE_LOGGING_SYSTEM,
				 dupstr1(ERget( S_ST0303_logging_system )), 
				 dupstr2(ERget( S_ST030F_default_settings)),
				 dupstr3(ERx( "1" )),FALSE,FALSE) ;

		STprintf( temp_buf, "%s_LOG_FILE", SystemVarPrefix );
		log1_row= VCBFAddComponent(COMP_TYPE_TRANSACTION_LOG,
					   ERget( S_ST0305_transaction_log ),
					   temp_buf,enabled1,TRUE,FALSE);

		STprintf( temp_buf, "%s_DUAL_LOG", SystemVarPrefix );
		log2_row= VCBFAddComponent(COMP_TYPE_TRANSACTION_LOG,
					   ERget( S_ST0305_transaction_log ),
					   temp_buf,enabled2,FALSE,TRUE);

		VCBFAddComponent(COMP_TYPE_RECOVERY,
				 dupstr1(ERget( S_ST0301_recovery_server )),
//				 dupstr2(ERget( S_ST030A_not_configurable )), // [Comment] 14-Aug-1998: UK Sotheavut (UK$SO01)
				 dupstr2(ERget( S_ST030F_default_settings)),  // [Add    ] 14-Aug-1998: UK Sotheavut (UK$SO01)
				 dupstr3(ERx( "1" )),FALSE,FALSE);

		VCBFAddComponent(COMP_TYPE_ARCHIVER,
				 dupstr1(ERget( S_ST0302_archiver_process )),
				 dupstr2(ERget( S_ST030A_not_configurable )),
				 dupstr3(ERx( "1" )),FALSE,FALSE) ;

	}
#ifdef OIDSK
	{
		char buff[200];
		OIDSKINIFILEINFO InifileInfo=
			{ "Server Name"  ,"servername"     ,"string"    ,"ingres,sqlapipe",SPECIAL_SERVERNAME,0,0,NULL};
        GetIniFileParm(&InifileInfo, buff, sizeof(buff)-20);
    	VCBFAddComponent(COMP_TYPE_OIDSK_DBMS,
	            		 "DBMS server",
                         buff,
						 dupstr3(ERx( "1" )),FALSE,FALSE) ;
	}
# endif /* OIDSK */
# endif /* JASMINE */
       return TRUE;
}

static char   config_buf[ MAX_LOC + 1 ];        
static char   protect_buf[ MAX_LOC + 1 ];       
static char   units_buf[ MAX_LOC + 1 ]; 
static char   help_buf[ MAX_LOC + 1 ];  
static char   locbuf[ BIG_ENOUGH ];

static BOOL   bUnlockOnTerminate=FALSE;

static char *cmdLineArg[BIG_ENOUGH];
static char strInput[BIG_ENOUGH]={0};

static char bufLocDefConfname[100];
static char bufTransLogCompName[100];

char * GetTransLogCompName(){ return bufTransLogCompName;}
char * GetLocDefConfName() { return  bufLocDefConfname;}

BOOL VCBFllinit(char *cmd_ln)
{
	STATUS          status;
	i4             info;
	CL_ERR_DESC     cl_err;
	char            *value;
//      char            *ii_keymap;
#ifndef OIDSK
	char            *name;
	PM_SCAN_REC     state;
#endif
	char            *temp;   
	char            tmp_buf[ BIG_ENOUGH ];
	CX_NODE_INFO    *pni;
	i4				rad;
	i4				gotcontext = 0; //cluster specific

	/* Tell EX this is an ingres tool. */
	(void) EXsetclient(EX_INGRES_TOOL);

	MEadvise( ME_INGRES_ALLOC );

	STcopy(ERget(S_ST030F_default_settings), bufLocDefConfname);
	STcopy(ERget(S_ST0305_transaction_log), bufTransLogCompName);

# ifdef JASMINE
	ingres_menu=FALSE;
# endif /* JASMINE */
# ifdef NT_GENERIC
	NMgtAt( ERx("II_TEMPORARY"), &temp );
	if( temp == NULL || *temp == EOS )
	{
	    temp = ".";
	}
	STcopy(temp, tempdir);
	STcat(tempdir, ERx("\\"));
# else 
	STcopy(TEMP_DIR, tempdir);
# endif

	if(cmd_ln)
		strcpy(strInput,cmd_ln);

	
	/* get local host name */
    /* remote configuration will be done differently */
	if(cmd_ln == NULL)
	{
		host = PMhost();
	}
	else
	{
		cmdLineArg[1]= strInput;
		host = cmdLineArg[1];
	}
	//Needed for clustered ingres support in VCBF.

	/*
	** Call CXget_context with various flags to try and
	** determine users intent.   On a NUMA cluster node,
	** we may want to set a specific RAD, since we need
	** this context for transaction log operations.
	*/

	i4 argc = 0;
	if(cmd_ln == NULL)
		argc = 1;
	else
		argc = 2;

	if ( OK == CXget_context( &argc, cmdLineArg,
		CX_NSC_CONSUME|CX_NSC_SET_CONTEXT, 
		cx_node_name_buf, CX_MAX_NODE_NAME_LEN+1 ) )

	{
		/*
		** Here on no '-rad'/'-node' & non-numa, or one of either
		** a local '-node' spec, or a '-rad'.
		**
		** We're working on a local node, RAD context is set,
		** and any -node / -rad parameter has been eaten.
		*/
		gotcontext = 1;
	}

	else if ( OK == CXget_context( &argc, cmdLineArg, 
		CX_NSC_CONSUME|CX_NSC_IGNORE_NODE|
		CX_NSC_SET_CONTEXT, NULL, 0 ) )
	{
		/*
		** Both a RAD and a NODE were specified, eat
		** the RAD, leave the node, and set NUMA context
		** based on RAD, then try another pass to
		** make sure we don't have an invalid node
		** specification.
		*/
		gotcontext = 1;
		if ( OK != CXget_context( &argc, cmdLineArg,
			CX_NSC_CONSUME|CX_NSC_NODE_GLOBAL|CX_NSC_REPORT_ERRS,
			cx_node_name_buf, CX_MAX_NODE_NAME_LEN+1 ) )
		{
			PCexit(FAIL);
		}
	}

	while (1)
	{
		rad = argc;	/* use rad as work var*/ 
		if ( OK == CXget_context( &argc, cmdLineArg,
			CX_NSC_CONSUME|CX_NSC_NODE_GLOBAL,
			tmp_buf, CX_MAX_NODE_NAME_LEN+1 ) &&
			rad != argc )
		{
			/*
			** Hmm, dicy.  -node parameter was provided,
			** but was directed against a remote node.
			** We can run on any local node, but none was
			** specified.
			*/
			STcopy( tmp_buf, cx_node_name_buf );
			if ( gotcontext )
			{
				/*
				** No sweat, user was kind enough to explicitly
				** set NUMA context with 1st rad/local node param.
				*/
				break;
			}

			/*
			** We can run on any local node, but none was
			** specified.  Search for any free.
			*/
			for ( rad = CXnuma_rad_count(); rad > 0; rad-- )
			{
				if ( CXnuma_rad_configured( /*local_host*/host, rad ) &&
					OK == CXset_context(/*local_host*/host, rad) 
#ifdef UNIX
					&& OK == CS_map_sys_segment(&cl_err)
#endif /*UNIX*/  
					)
				{
					/*
					** Found one.  Redundant CS_map_sys_segment
					** is harmless, since logic in ME_attach,
					** detects multiple attachments, and just
					** returns address previously assigned.
					*/
					gotcontext = 1;
					break;
				}
			}
			break;
		}

		/*
		** Check for old-style cbf <nodename> syntax,
		** and reparse if needed.
		*/
		if ( argc == 2 && cmdLineArg[1] != tmp_buf )
		{
			STprintf( tmp_buf, ERx( "-node=%s" ), cmdLineArg[1] );
			cmdLineArg[1]= tmp_buf;
			continue;
		}

		/* Already did reparse, or command options are hopeless*/ 
		break;
	}

	if ( !gotcontext )
	{
		/*
		** No context was provided at all, or bad or
		** multiple rad and/or node specs were specified.
		** Call once more, and yell at the user.
		*/
		(void)CXget_context( &argc, cmdLineArg,
			CX_NSC_STD_NUMA, NULL, 0 );
		PCexit(FAIL);
	}

	if ( argc > 2 )
	{
		mymessage( ERget( S_ST0427_cbf_usage ) );
		return FALSE;
	}

	/* get local host name*/
	host = node = cx_node_name_buf;
	
	/*
	** Node name, and host name are identical, except in the case
	** of a NUMA node in an Ingres cluster.   In that case, 'host'
	** is the name of the machine configured with multiple NUMA
	** nodes, and 'node' is the name of a specific node.   'host' 
	** is used to obtain most of the parameters, but parameters
	** specific to the virtual node (such as log file specs), are
	** qualified by 'node'.
	*/
	if ( CXcluster_enabled() )
	{
		pni = CXnode_info( node, 0 );
		if ( NULL == pni )
		{
			mymessage( ERget( S_ST0427_cbf_usage ) );
			return FALSE;
		}
		if ( pni->cx_host_name_l )
		{
			STlcopy( pni->cx_host_name, cx_host_name_buf,
				CX_MAX_HOST_NAME_LEN );
			host = cx_host_name_buf;
		}

		if ( CXnuma_cluster_configured() )
		{
			pni = CXnode_info( CXnode_name(NULL), 0 );
			if ( NULL == pni )
			{
				mymessage( ERget( S_ST0427_cbf_usage ) );
				return false;
			}
			STprintf( Affinity_Arg, ERx( "-rad=%d" ), pni->cx_rad_id );
		}
	}
	//end-cluster specific.

	/* prepare LOCATION for config.dat */
	NMloc( FILES, FILENAME, ERx( "config.dat" ), &config_file );
	LOcopy( &config_file, config_buf, &config_file );

	/* prepare LOCATION for protect.dat */
	NMloc( FILES, FILENAME, ERx( "protect.dat" ), &protect_file );
	LOcopy( &protect_file, protect_buf, &protect_file );

	/* prepare LOCATION for cbfunits.dat */
	NMloc( FILES, FILENAME, ERx( "cbfunits.dat" ), &units_file );
	LOcopy( &units_file, units_buf, &units_file );

	/* prepare LOCATION for cbfhelp.dat */
	NMloc( FILES, FILENAME, ERx( "cbfhelp.dat" ), &help_file );
	LOcopy( &help_file, help_buf, &help_file );

	/* prepare LOCATION for config.log */
	NMloc( FILES, FILENAME, ERx( "config.log" ), &change_log_file );
	LOcopy( &change_log_file, change_log_buf, &change_log_file );

	/* load cbfhelp.dat */
	(void) PMmInit( &help_data );
	status = PMmLoad( help_data, &help_file, NULL );
	if( status != OK )
	{
		mymessage2b(ERget( S_ST0319_unable_to_load ), help_buf );
		return FALSE;
	}

	/* load cbfunits.dat */
	(void) PMmInit( &units_data );
	status = PMmLoad( units_data, &units_file, NULL );
	if( status != OK )
	{
		mymessage2b( ERget( S_ST0319_unable_to_load ), units_buf );
		return FALSE;
	}

	/* load config.dat */
	(void) PMmInit( &config_data );
	PMmLowerOn( config_data );
	status = PMmLoad( config_data, &config_file, NULL );
	if( status != OK )
	{
		mymessage(ERget( S_ST0315_bad_config_read ));
		return FALSE;
	}

	/* load protect.dat */
	(void) PMmInit( &protect_data );
	PMmLowerOn( protect_data );
	status = PMmLoad( protect_data, &protect_file, NULL );

	/* intialize configuration rules system */
	CRinit( config_data );

	status = CRload( NULL, &info );

	switch( status )
	{
		case CR_NO_RULES_ERR:
			mymessage(ERget( S_ST0418_no_rules_found ));
			return FALSE;

		case CR_RULE_LIMIT_ERR:
			{
			   char buf[200];
			   sprintf(buf,ERget( S_ST0416_rule_limit_exceeded ), 
				       info );
			   mymessage(buf);
			}
			return FALSE;

		case CR_RECURSION_ERR:
			mymessage2b(
				ERget( S_ST0417_recursive_definition ),
				CRsymbol( info ) );
			return FALSE;
	}

	if( mylock_config_data( ERx( "vcbf" ) ) != OK )
		return FALSE;

	
/*      PCatexit( cbf_exit );  all return FALSE must now be preceeded by */
/*                             a call to unlock_config_data();           */

#ifdef VCBF_USE_EXCEPT_HDL // can be discussed: wouldn't do more than VCBFllterminate()
	if( EXdeclare( cbf_handler, &context ) != OK )
	{
		myEXdelete();
		mymessage("initialization error");
		myunlock_config_data();    
		return FALSE;
	}
#endif

	/* user must have write access to config.dat */ 
	if( SIfopen( &config_file, ERx( "a" ), SI_TXT, SI_MAX_TXT_REC,
		&config_fp ) != OK )
	{
		myEXdelete();
		DisplayMessageWithParam(S_ST0323_unable_to_open, config_buf);
		myunlock_config_data();
		return FALSE;
	   /*   IIUGerr( S_ST0323_unable_to_open, UG_ERR_FATAL, 1, */
	   /*           config_buf );                              */
	}
	(void) SIclose( config_fp );

	/* user must have write access to protect.dat */ 
	if( SIfopen( &protect_file, ERx( "a" ), SI_TXT, SI_MAX_TXT_REC,
		&protect_fp ) != OK )
	{
		if( SIfopen( &protect_file, ERx( "w" ), SI_TXT, SI_MAX_TXT_REC,
			&protect_fp ) != OK )
		{
			myEXdelete();
			DisplayMessageWithParam(S_ST0323_unable_to_open, protect_buf);
			myunlock_config_data();    
			return FALSE;
		/*      IIUGerr( S_ST0323_unable_to_open, UG_ERR_FATAL, 1, */
		/*                      protect_buf );                             */
		}
	}
	(void) SIclose( protect_fp );

	/* open change log file for appending */
	if( SIfopen( &change_log_file , ERx( "a" ), SI_TXT, SI_MAX_TXT_REC,
		&change_log_fp ) != OK )
	{
		myEXdelete();
		DisplayMessageWithParam(S_ST0323_unable_to_open, change_log_buf);
		myunlock_config_data();    
		return FALSE;
	    /*  IIUGerr( S_ST0323_unable_to_open, UG_ERR_FATAL, 1,  */
	    /*          change_log_buf );                           */
	}

	VCBFsession_timestamp( change_log_fp );

	/* close change log after appending */
	(void) SIclose( change_log_fp );

#if 0   /* no forms stuff */
=>	if (FEforms() != OK)
=>	{
=>	  myEXdelete();
=>	  SIflush(stderr);
=>	  PCexit(FAIL);
=>	}
=>
=>	/* New function key maps for CBF */
=>	NMgtAt( ERx( "II_CBF_KEYS" ), &ii_keymap);
=>	if( ii_keymap == NULL || *ii_keymap == EOS )
=>	{
=>		/* prepare LOCATION for default key files */
=>		exec frs inquire_frs frs(:tty_type = terminal);
=>		if (STbcompare(tty_type, 2, ERx("vt"), 2, TRUE) == 0) {
=>			NMloc( FILES, FILENAME, ERx( "cbf.map" ), 
=>				&key_map_file);
=>			STcopy(key_map_file.string,key_map_filename);
=>			exec frs set_frs frs (mapfile = :key_map_filename);
=>		}
=>
=>	}
=>	else
=>	{
=>		STcopy(ii_keymap,key_map_filename);
=>		exec frs set_frs frs (mapfile = :key_map_filename);
=>	}
=>	exec frs set_frs frs ( outofdatamessage = 0 );
#endif

	/* get name of syscheck program if available */
	STprintf( tmp_buf, ERx( "%s.*.cbf.syscheck_command" ),
		  SystemCfgPrefix );
	(void) PMmGet( config_data, tmp_buf, &syscheck_command );

	PMmSetDefault( config_data, 1, host );

	STprintf(tmp_buf, ERx( "%s.$.cbf.syscheck_mode" ), SystemCfgPrefix);
	if( syscheck_command != NULL && PMmGet( config_data,
		tmp_buf, &value ) == OK &&
		ValueIsBoolTrue( value ) )
	{
		/*
		** ### FIX ME
		** If the Name Server is running, syscheck should be
		** disabled and a reasonable message should be displayed
		** explaining that there is no point in checking system
		** resources, since the running system has eaten some
		** amount of them. 
		*/ 

#ifdef VCBF_FUTURE
      replace it => exec frs message :ERget( S_ST0316_cbf_syscheck_message );
#endif
		if( PCcmdline( NULL, syscheck_command, PC_WAIT, NULL,
			&cl_err ) != OK )
		{
#ifdef OIDSK
                        strcpy  ( tmp_buf, ERget( S_ST0327_syscheck_disabled ));
#else
			STprintf( tmp_buf, ERget( S_ST0327_syscheck_disabled ),
				  SystemProductName );
#endif
			mymessagewithtitle(tmp_buf);
			syscheck_command = NULL;
		}
	}

	/* determine whether this is server */
	STprintf(tmp_buf, ERx("%s.$.%sstart.%%.dbms"), SystemCfgPrefix, 
		     SystemExecName);
#ifndef OIDSK
	if( PMmScan( config_data, PMmExpToRegExp( config_data, tmp_buf ), 
		     &state, NULL, &name, &value ) == OK ) 
	{
		server = TRUE; 
	}
	else
#endif
		server = FALSE;

	PMmSetDefault( config_data, 1, NULL );

	STprintf (locbuf, "%s_INSTALLATION", SystemVarPrefix);

	ii_installation_name = STalloc (locbuf);

	ii_system_name = STalloc (SystemLocationVariable);

	/* get value of II_SYSTEM */
	NMgtAt( SystemLocationVariable, &ii_system );
	if( ii_system == NULL || *ii_system == EOS )
	{
		myEXdelete();
#ifdef OIDSK
		mymessage(ERget(S_ST0312_no_ii_system));
#else
		mymessage2b(ERget(S_ST0312_no_ii_system),
			    SystemLocationVariable );
#endif
		myunlock_config_data();    
		return FALSE;
	}
	ii_system = STalloc( ii_system );
	
	/* get value of II_INSTALLATION */
	NMgtAt( ERx( "II_INSTALLATION" ), &ii_installation );
	if( ii_installation == NULL || *ii_installation == EOS )
	{
#ifdef VMS
		/*
		** VMS system level installations do not
		** have II_INSTALLATION set, but have an 
		** internal installation code of ...
		*/
		ii_installation = ERx( "AA" );
#else
		/*
		** all other environments must have 
		** II_INSTALLATION set properly.
		*/
		myEXdelete();
#ifdef OIDSK
		mymessage( ERget( S_ST0313_no_ii_installation ));
#else
		mymessage2b( ERget( S_ST0313_no_ii_installation ), 
			     SystemVarPrefix );
#endif
		myunlock_config_data();    
		return FALSE;
#endif
	}
	else
		ii_installation = STalloc( ii_installation );

       updateComponentList();

  bUnlockOnTerminate=TRUE;
  return TRUE;
}
BOOL VCBFllterminate(void)
{
	myEXdelete();
        if (bUnlockOnTerminate) {
        	myunlock_config_data();
                bUnlockOnTerminate=FALSE;
        }
	return TRUE;
}


#ifdef __cplusplus
extern "C"    /* because these are callbacks */
{
#endif

/*
** Returns TRUE if value is a valid directory.  Prompts for
** creation if it doesn't exist.
*/
static bool
is_directory( char *value, bool *canceled )
{
	LOCATION        loc;
	i2              flag;
//        char            tmp_val[ BIG_ENOUGH ];
//        char            *ptv;
//        char            *pv;

	*canceled = FALSE;

	LOfroms( PATH, value, &loc );

	if ( LOisdir( &loc, &flag ) != OK )
	{
	       int ret = MessageBox(GetFocus(),
			       (LPTSTR)(LPCTSTR)ERget(S_ST042C_no_directory),
			       stdtitle,
			       MB_ICONQUESTION | MB_YESNO |MB_TASKMODAL);
	       if (ret == IDYES) {
		   if ( LOcreate( &loc ) != OK )
		       return ( FALSE );
	       }
	       else
	       {
		   *canceled = TRUE;
		   return ( FALSE );
	       }
	}
	else if ( !(flag & ISDIR) )
		return ( FALSE );

	return ( TRUE );
}


/*
** Returns TRUE if value is a valid file
*/
static bool
is_file( char *value )
{
	LOCATION        loc;
	i4             flag;
	LOINFORMATION   locinfo;

	LOfroms( PATH & FILENAME, value, &loc );

	flag = LO_I_TYPE;
	if( LOinfo( &loc, &flag, &locinfo ) != OK )
	{
		if ( LOcreate( &loc ) != OK )
			return ( FALSE );
	}
	else if ( !( locinfo.li_type == LO_IS_FILE ) )
		return ( FALSE );

	return ( TRUE );
}


/* return pointer to functions that verify valid pathnames */
BOOLFUNC *
CRpath_constraint(CR_NODE *n, u_i4 constraint_id )
{
//        PTR ptr;
	bool has_constraint = FALSE;

	if ( unary_constraint( n, constraint_id ) == TRUE )
	{
		if ( constraint_id == ND_DIRECTORY )
			return ((BOOLFUNC *)is_directory);
		else
			return ((BOOLFUNC *)is_file);
	}
	return NULL;
	
}
#ifdef __cplusplus
}
#endif

BOOL VCBFGetHostInfos(LPSHOSTINFO lphostinfo)
{
    lphostinfo->host                    =host;
    lphostinfo->ii_system               =ii_system;
    lphostinfo->ii_installation         =ii_installation;
    lphostinfo->ii_system_name          =ii_system_name;
    lphostinfo->ii_installation_name    =ii_installation_name;
    return TRUE;
}

void
scan_priv_cache_parm(char *instance, char *new1, char *page, 
	bool keep_old)
{
    PM_SCAN_REC state;
    STATUS      status;
    char        *last, *name, *value, *param, *regexp;
    char        resource[BIG_ENOUGH];
    char        expbuf[BIG_ENOUGH];
 
    if (page == NULL)
	STprintf( expbuf, ERx( "%s.%s.dbms.%s.%s.%%" ),
		  SystemCfgPrefix, host, ERget( S_ST0333_private ), instance );
    else
	STprintf( expbuf, ERx( "%s.%s.dbms.%s.%s.%s.%%" ),
		  SystemCfgPrefix, host, ERget( S_ST0333_private ), 
		  instance, page);

    regexp = PMmExpToRegExp( config_data, expbuf );

    for( status = PMmScan( config_data, regexp, &state,
		NULL, &name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, last, &name, &value ) )
    {
	/* set 'last' to handle deletion during scan */
	last = name;

	/* extract parameter name */
	if (page == NULL)
	    param = PMmGetElem( config_data, 5, name );
	else
	    param = PMmGetElem( config_data, 6, name );

	/* destroy old resource if 'keep_old' not TRUE */ 
	if( !keep_old && PMmDelete( config_data, name ) == OK )
	{
	    SIfprintf( change_log_fp, ERx( "DELETE %s\n" ), name );
	    SIflush( change_log_fp );
	}

	/* insert resource if 'new1' not NULL */ 
	if( new1 != NULL )
	{
	    /* compose duplicate resource name */
	    if (page == NULL)
		STprintf( resource, ERx( "%s.%s.dbms.%s.%s.%s" ), 
			  SystemCfgPrefix, host, ERget( S_ST0333_private ), 
			  new1, param );
	    else
		STprintf( resource, ERx( "%s.%s.dbms.%s.%s.%s.%s" ), 
			  SystemCfgPrefix, host, ERget( S_ST0333_private ), 
			  new1, page, param);

	    if( PMmInsert( config_data, resource, value ) == OK )
	    {
		SIfprintf( change_log_fp, ERx( "CREATE %s: (%s)\n" ),
			resource, value );
		SIflush( change_log_fp );
	    }
	}
    }
}

static BOOL
VCBFcopy_rename_nuke( LPCOMPONENTINFO lpcomponent, char *new_name, bool keep_old )
{
	char comp_type[ BIG_ENOUGH ];
	char comp_name[ BIG_ENOUGH ];
	char *new1 = new_name;
	i4 old_row = -1;

	PM_SCAN_REC state;
	STATUS status;
	char resource[ BIG_ENOUGH ];
	char expbuf[ BIG_ENOUGH ];
	char *regexp, *last, *name, *value;
	char *owner, *instance, *param;
	bool is_dbms = FALSE;
	bool is_net = FALSE;
	bool is_bridge = FALSE;
	bool private_cache = FALSE;
	char *cp;

	/* open change log file for appending */
	if( SIfopen( &change_log_file , ERx( "a" ), SI_TXT, SI_MAX_TXT_REC,
		&change_log_fp ) != OK )
	{
		DisplayMessageWithParam(S_ST0323_unable_to_open, change_log_buf);
		VCBFRequestExit();
		return FALSE;
	}

	SIfprintf( change_log_fp, ERx( "\n" ) );
	
	/* strip whitespace */
	if( new_name != NULL )
	{
		STzapblank( new_name, new_name );

		/* abort if no input */
		if( *new_name == EOS )
			return FALSE;

		/* abort if non-alphanumeric input (underbars are OK) */
		for( cp = new_name; *cp != EOS; CMnext( cp ) )
		{
			if( !CMalpha( cp ) && !CMdigit( cp ) && *cp != '_' )
			{
				mymessage(ERget( S_ST0419_invalid_name ));
				return FALSE;
			}
		}
	}

	strcpy(comp_type,lpcomponent->sztype);
	strcpy(comp_name,lpcomponent->szname);

	if( STcompare( comp_name, ERget( S_ST030F_default_settings ) ) == 0 )
		instance = ERx( "*" );
	else
		instance = comp_name;

	if( STcompare( comp_type , ERget( S_ST063C_dbms_servers ) ) == 0 )
	{
		is_dbms = TRUE; 
		owner = ERx( "dbms" );
	}

	else if( STcompare( comp_type, ERget( S_ST063D_net_servers ) ) == 0 )
	{
		is_net = TRUE;
		owner = ERx( "gcc" );
	}
	else if( STcompare( comp_type, ERget( S_ST069B_das_servers ) ) == 0 )
	{
		is_net = TRUE;
		owner = ERx( "gcd" );
	}
	else if( STcompare( comp_type ,
		ERget( S_ST063F_bridge_servers) ) == 0 )
	{
		is_bridge = TRUE;
		owner = ERx( "gcb" );
	}
	else if( STcompare( comp_type, ERget( S_ST063E_star_servers ) ) == 0 )
		owner = ERx( "star" );
	else if( STcompare( comp_type, ERget( S_ST055C_oracle_gateway ) ) == 0 )
	{ owner = ERx( "gateway" );
	  instance = ERx( "oracle" );
    }
	else if( STcompare( comp_type, ERget( S_ST0677_db2udb_gateway ) ) == 0 )
	{ owner = ERx( "gateway" );
	  instance = ERx( "db2udb" );
    }
	else if( STcompare( comp_type, ERget( S_ST055F_informix_gateway ) ) == 0 )
	{ owner = ERx( "gateway" );
	  instance = ERx( "informix" );
    }
	else if( STcompare( comp_type, ERget( S_ST0563_sybase_gateway ) ) == 0 )
	{ owner = ERx( "gateway" );
	  instance = ERx( "sybase" );
    }
	else if( STcompare( comp_type, ERget( S_ST0566_mssql_gateway ) ) == 0 )
	{ owner = ERx( "gateway" );
	  instance = ERx( "mssql" );
    }
	else if( STcompare( comp_type, ERget( S_ST0569_odbc_gateway ) ) == 0 )
	{ owner = ERx( "gateway" );
	  instance = ERx( "odbc" );
    }


	STprintf( expbuf, ERx( "%s.%s.%s.%s.%%" ), 
		      SystemCfgPrefix, host, owner, instance );

	regexp = PMmExpToRegExp( config_data, expbuf );

	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status = PMmScan(
		config_data, NULL, &state, last, &name, &value ) )
	{
		/* set 'last' to handle deletion during scan */
		last = name;

		/* extract parameter name */
		param = PMmGetElem( config_data, 4, name );

		/* destroy old resource if 'keep_old' not TRUE */ 
		if( !keep_old && PMmDelete( config_data, name ) == OK )
		{
			SIfprintf( change_log_fp, ERx( "DELETE %s\n" ),
				name );
			SIflush( change_log_fp );
		}

		/* insert resource if 'new1' not NULL */ 
		if( new1 != NULL )
		{
			/* compose duplicate resource name */
			STprintf( resource, ERx( "%s.%s.%s.%s.%s" ), 
				  SystemCfgPrefix, host, owner, new1, param );
			if( PMmInsert( config_data, resource, value ) == OK )
			{
				SIfprintf( change_log_fp,
					ERx( "CREATE %s: (%s)\n" ),
					resource, value );
				SIflush( change_log_fp );
			}
		}

		/* check for private DBMS cache */
		if( is_dbms && STcompare( ERx( "cache_sharing" ),
			param ) == 0 && !ValueIsBoolTrue( value )
		)
			private_cache = TRUE;
	}

	/* scan Net protocol resources */
	if( is_net )
	{
		char *protocol;

		STprintf( expbuf, ERx( "%s.%s.%s.%s.%%.%%" ), 
			  SystemCfgPrefix, host, owner, instance );

		regexp = PMmExpToRegExp( config_data, expbuf );

		for( status = PMmScan( config_data, regexp, &state,
			NULL, &name, &value ); status == OK; status =
			PMmScan( config_data, NULL, &state, last, &name,
			&value ) )
		{
			/* set 'last' to handle deletion during scan */
			last = name;

			/* extract protocol name */
			protocol = PMmGetElem( config_data, 4, name );

			/* extract parameter name */
			param = PMmGetElem( config_data, 5, name );

			/* destroy old resource if 'keep_old' not TRUE */ 
			if( !keep_old && PMmDelete( config_data, name )
				== OK )
			{
				SIfprintf( change_log_fp,
					ERx( "DELETE %s\n" ),
					name );
				SIflush( change_log_fp );
			}

			/* insert resource if 'new1' not NULL */ 
			if( new1 != NULL )
			{
				/* compose duplicate resource name */
				STprintf( resource,
					  ERx( "%s.%s.%s.%s.%s.%s" ),
					  SystemCfgPrefix, host, owner, new1, 
					  protocol, param );
				if( PMmInsert( config_data, resource,
					value ) == OK )
				{
					SIfprintf( change_log_fp,
						ERx( "CREATE %s: (%s)\n" ),
						resource, value );
					SIflush( change_log_fp );
				}
			}
		}
	}

	/* scan private DBMS cache resources */
	if( private_cache )
	{
		scan_priv_cache_parm(instance, new1, NULL, keep_old);
		scan_priv_cache_parm(instance, new1, ERx("cache"), keep_old);
		scan_priv_cache_parm(instance, new1, ERx("p4k"), keep_old);
		scan_priv_cache_parm(instance, new1, ERx("p8k"), keep_old);
		scan_priv_cache_parm(instance, new1, ERx("p16k"), keep_old);
		scan_priv_cache_parm(instance, new1, ERx("p32k"), keep_old);
		scan_priv_cache_parm(instance, new1, ERx("p64k"), keep_old);

		/* compose hidden dmf connect resource name */
		STprintf( resource,
			  ERx( "%s.%s.dbms.%s.%s.config.dmf_connect" ),
			  SystemCfgPrefix, host, ERget( S_ST0333_private ), 
			  instance );
	}
	else
	{
		/* compose hidden dmf connect resource name */
		STprintf( resource,
			  ERx( "%s.%s.dbms.%s.%s.config.dmf_connect" ),
			  SystemCfgPrefix, host, ERget( S_ST0332_shared ), 
			  instance );
	}

	/* delete hidden dmf connect resource */
	(void) PMmDelete( config_data, resource );

	if( !keep_old )
	{

		/* compose name of existing startup resource name */
		STprintf( resource, ERx( "%s.%s.%sstart.%s.%s" ), 
			  SystemCfgPrefix, host, SystemExecName, instance,
			  owner );      

		/* delete it */
		if( PMmDelete( config_data, resource ) == OK )
		{
			SIfprintf( change_log_fp, ERx( "DELETE %s\n" ),
				resource );
			SIflush( change_log_fp );
		}
	}

	if( new1 != NULL )
	{
		/* compose duplicate startup resource */
		STprintf( resource, ERx( "%s.%s.%sstart.%s.%s" ), 
			  SystemCfgPrefix, host, SystemExecName, new1, owner );  

		/* insert it and update tablefield if necessary */
		if( PMmInsert( config_data, resource, ERx( "0" ) ) == OK )
		{       
			SIfprintf( change_log_fp, ERx( "CREATE %s: (0)\n" ),
				resource );
			SIflush( change_log_fp );

		}
	}

	/* close change log after appending */
	(void) SIclose( change_log_fp );
	
	/* write config.dat */
	if( PMmWrite( config_data, NULL ) != OK ) {
		DisplayMessageWithParam(S_ST0318_fatal_error, ERget( S_ST0314_bad_config_write ));
		VCBFRequestExit();
		return FALSE;
	}
	return TRUE;
}



BOOL VCBFllCanCountBeEdited(LPCOMPONENTINFO lpcomponent)
{
   if (lpcomponent->itype==COMP_TYPE_OIDSK_DBMS                                 ||
       STcompare( lpcomponent->sztype, ERget( S_ST0300_name_server ) )     == 0 ||
       STcompare( lpcomponent->sztype, ERget( S_ST0303_logging_system ) )  == 0 ||
       STcompare( lpcomponent->sztype, ERget( S_ST0304_locking_system ) )  == 0 ||
       STcompare( lpcomponent->sztype, ERget( S_ST0302_archiver_process )) == 0 ||
       STcompare( lpcomponent->sztype, ERget( S_ST0305_transaction_log ) ) == 0 ||
       STcompare( lpcomponent->sztype, ERget( S_ST034E_security_menu ) )   == 0 ||
       STcompare( lpcomponent->sztype, ERget( S_ST0301_recovery_server ) ) == 0 )
	   return FALSE;
   return TRUE;
}

BOOL VCBFllCanNameBeEdited(LPCOMPONENTINFO lpcomponent)
{
   if (lpcomponent->itype==COMP_TYPE_OIDSK_DBMS  )
		 return FALSE;
   if(
	STcompare( lpcomponent->sztype, ERget( S_ST063C_dbms_servers ) ) == 0 ||
	STcompare( lpcomponent->sztype, ERget( S_ST063D_net_servers ) ) == 0 || 
	STcompare( lpcomponent->sztype, ERget( S_ST069B_das_servers ) ) == 0 || 
	STcompare( lpcomponent->sztype, ERget( S_ST063E_star_servers ) ) == 0
#ifndef OIDSK
	     ||
	STcompare( lpcomponent->sztype, ERget( S_ST063F_bridge_servers ) )
	== 0
#endif
     )
   {
	if( STcompare( lpcomponent->szname,ERget( S_ST030F_default_settings ) ) == 0 )
	   return FALSE;
	else
	   return TRUE;
   }
   return FALSE;
}

BOOL VCBFllCanCompBeDuplicated(LPCOMPONENTINFO lpcomponent)
{
   if (lpcomponent->itype==COMP_TYPE_OIDSK_DBMS  )
      return FALSE;
  
   if(
#ifndef WORKGROUP_EDITION
	STcompare( lpcomponent->sztype, ERget( S_ST063C_dbms_servers ) )
	== 0 ||
#endif
	STcompare( lpcomponent->sztype, ERget( S_ST063D_net_servers ) )
	== 0 || 
	STcompare( lpcomponent->sztype, ERget( S_ST069B_das_servers ) )
	== 0 || 
	STcompare( lpcomponent->sztype, ERget( S_ST063E_star_servers ) )
	== 0 
#ifndef OIDSK
	     ||
	STcompare( lpcomponent->sztype, ERget( S_ST063F_bridge_servers ) )
	== 0
#endif
      )
	return TRUE;
   return FALSE;
}

BOOL VCBFllCanCompBeDeleted(LPCOMPONENTINFO lpcomponent)
{
    if (lpcomponent->itype==COMP_TYPE_OIDSK_DBMS  )
	  return FALSE;
    if(
	STcompare( lpcomponent->sztype, ERget( S_ST063C_dbms_servers ) )
	== 0 ||
	STcompare( lpcomponent->sztype, ERget( S_ST063D_net_servers ) )
	== 0 || 
	STcompare( lpcomponent->sztype, ERget( S_ST069B_das_servers ) )
	== 0 || 
	STcompare( lpcomponent->sztype, ERget( S_ST063E_star_servers ) )
	== 0 
#ifndef OIDSK
	     ||
	STcompare( lpcomponent->sztype, ERget( S_ST063F_bridge_servers ) )
	== 0
#endif
      )
   {
	if( STcompare( lpcomponent->szname,
		ERget( S_ST030F_default_settings ) ) == 0 )
		return FALSE;
	else
	{
		return TRUE;
	}
   }
   return FALSE;
}

BOOL VCBFllValidCount(LPCOMPONENTINFO lpcomponent, char * in_buf)
{
	char comptype[80];
	char compname[80];
	char temp_buf[ BIG_ENOUGH ];
	BOOL bResult;
        bool is_gcc = FALSE;
        bool is_das = FALSE;
	strcpy(comptype,lpcomponent->sztype);
	strcpy(compname,lpcomponent->szname);

	while (*in_buf==' ')
	   strcpy(in_buf,in_buf+1);

	if( STcompare( lpcomponent->szname,
		ERget( S_ST030F_default_settings ) ) == 0
	)
		STcopy( ERx( "*" ), compname );

	else if ( STcompare( lpcomponent->szname,
			ERget( S_ST030A_not_configurable ) ) == 0
		) 
			STcopy( ERx( "*" ), compname );

	if( STcompare( lpcomponent->sztype, ERget( S_ST063C_dbms_servers ) ) == 0)
		STcopy( ERx( "dbms" ), comptype );
	else if( STcompare( comptype, ERget( S_ST063D_net_servers ) ) == 0)
	{
		STcopy( ERx( "gcc" ), comptype );
		is_gcc = TRUE;
	}
	else if( STcompare( comptype, ERget( S_ST069B_das_servers ) ) == 0)
	{
		STcopy( ERx( "gcd" ), comptype );
		is_das = TRUE;
	}
	else if( STcompare( comptype, ERget( S_ST063F_bridge_servers ) ) == 0)
		STcopy( ERx( "gcb" ), comptype );
	else if( STcompare( comptype, ERget( S_ST063E_star_servers ) ) == 0)
		STcopy( ERx( "star" ), comptype );
	else if( STcompare( comptype , ERget( S_ST055C_oracle_gateway ) ) == 0)
    {
		STcopy( ERx( "oracle" ), comptype );
		if (STcompare(in_buf,"1")!=0  &&  STcompare(in_buf,"0")!=0) {
		   mymessage(ERget( S_ST0448_startupcnt_zero_one));
		   return FALSE;
		}
	}
	else if( STcompare( comptype , ERget( S_ST0677_db2udb_gateway ) ) == 0)
    {
		STcopy( ERx( "db2udb" ), comptype );
		if (STcompare(in_buf,"1")!=0  &&  STcompare(in_buf,"0")!=0) {
		   mymessage(ERget( S_ST0448_startupcnt_zero_one));
		   return FALSE;
		}
	}
    else if( STcompare( comptype , ERget( S_ST055F_informix_gateway ) ) == 0)
    {
		STcopy( ERx( "informix" ), comptype );
		if (STcompare(in_buf,"1")!=0  &&  STcompare(in_buf,"0")!=0) {
		   mymessage(ERget( S_ST0448_startupcnt_zero_one));
		   return FALSE;
		}
	}
    else if( STcompare( comptype , ERget( S_ST0563_sybase_gateway ) ) == 0)
    {
		STcopy( ERx( "sybase" ), comptype );
		if (STcompare(in_buf,"1")!=0  &&  STcompare(in_buf,"0")!=0) {
		   mymessage(ERget( S_ST0448_startupcnt_zero_one));
		   return FALSE;
		}
	}
    else if( STcompare( comptype , ERget( S_ST0566_mssql_gateway ) ) == 0)
    {
		STcopy( ERx( "mssql" ), comptype );
		if (STcompare(in_buf,"1")!=0  &&  STcompare(in_buf,"0")!=0) {
		   mymessage(ERget( S_ST0448_startupcnt_zero_one));
		   return FALSE;
		}
	}
    else if( STcompare( comptype , ERget( S_ST0569_odbc_gateway ) ) == 0)
    {
		STcopy( ERx( "odbc" ), comptype );
		if (STcompare(in_buf,"1")!=0  &&  STcompare(in_buf,"0")!=0) {
		   mymessage(ERget( S_ST0448_startupcnt_zero_one));
		   return FALSE;
		}
	}

	else if( STcompare( comptype , ERget( S_ST02FE_rmcmd ) ) == 0)
		{
			STcopy( ERx( "rmcmd" ), comptype );
			if (STcompare(in_buf,"1")!=0  &&  STcompare(in_buf,"0")!=0) {
			   mymessage(ERget( S_ST0448_startupcnt_zero_one));
			   return FALSE;
			}
		}
	else if( STcompare( comptype, ERget( S_ST02FF_internet_comm ) ) == 0)
		STcopy( ERx( "icesvr" ), comptype );

	STprintf( temp_buf, ERx( "%s.%s.%sstart.%s.%s" ), SystemCfgPrefix, host,
                        SystemExecName, compname, comptype );

	bResult = set_resource( temp_buf, in_buf ) ;
        /*
        ** Indicate that port rollups are supported for DAS or GCC
        ** TCP ports with the syntax XXnn or numeric ports.  A plus 
        ** sign ("+") is appended to the port code if this format is
        ** detected and no plus sign already exists.
        **
        ** If a plus sign exists for a port, and rollups are not
        ** to be supported, the plus signs are removed.
        */
        if ( is_das || is_gcc )
            VCBF_format_port_rollups( comptype, compname, in_buf);

        /*
        ** Do this copy AFTER the port rollup check because the user
        ** COULD say "No" for a numeric port, in which case the value
        ** would revert back to one, and we need to update the display
        ** accordingly.
        */
	if (bResult==TRUE) 
	   strcpy(lpcomponent->szcount,in_buf);

	return bResult;
}

BOOL VCBFllValidRename(LPCOMPONENTINFO lpcomponent, char * in_buf)
{
   BOOL bResult =  VCBFcopy_rename_nuke( lpcomponent, in_buf, FALSE );
   if (bResult)
      strcpy(lpcomponent->szname,in_buf);
   return bResult;
}

BOOL VCBFllValidDuplicate(LPCOMPONENTINFO lpcomponentsrc, LPCOMPONENTINFO lpcomponentdest,char * in_buf)
{
   BOOL bResult=VCBFcopy_rename_nuke  ( lpcomponentsrc,in_buf, TRUE );
   if (bResult) {
     *lpcomponentdest=*lpcomponentsrc;
     strcpy(lpcomponentdest->szname,in_buf);
   }
   return bResult;
}

BOOL VCBFllValidDelete(LPCOMPONENTINFO lpcomponent)
{
     i4 n;
     CVan( lpcomponent->szcount, &n );
     if( n > 0 )  {
	mymessage(ERget( S_ST0374_startup_not_zero ));
	return FALSE;
     }
     return VCBFcopy_rename_nuke( lpcomponent, NULL, FALSE );
}
/****************************************************************/
/******************* generic form section       *****************/
/****************************************************************/


static LOCATION backup_data_file;
static char backup_data_buf[ MAX_LOC + 1 ];
static LOCATION backup_change_log_file;
static char backup_change_log_buf[ MAX_LOC + 1 ];
static LOCATION backup_protect_file;
static char backup_protect_buf[ MAX_LOC + 1 ];
static char *audit_log_list[MAX_AUDIT_LOGS];

static char *VCBFgf_pm_host;
static char bufVCBFgf_owner[80];
static char *VCBFgf_owner, VCBFgf_instance[ BIG_ENOUGH ];
static bool VCBFgf_changed = FALSE;
static char bufVCBFgf_cache_name[80];
static char *VCBFgf_cache_name = NULL;
static char bufVCBFgf_cache_sharing[80];
static char *VCBFgf_cache_sharing = NULL;
static char VCBFgf_real_pm_id[ BIG_ENOUGH ];
static char bufVCBFgf_db_list[1000];
static char *VCBFgf_db_list;
static char *VCBFgf_def_name ;
static char VCBFgf_pm_id[200];;
static bool VCBFgf_backup_mode;
static char VCBFgf_oldstartupcount[20];
static BOOL VCBFgf_bStartupCountChange = FALSE;

BOOL VCBFllInitGenericPane( LPCOMPONENTINFO lpcomp)
{
static  char comp_name[ BIG_ENOUGH ];
	char *name, *value;
//      char name_val[ BIG_ENOUGH ]; 
//      char bool_name_val[ BIG_ENOUGH ]; /*b69912*/
	char *my_name;
//      char description[ BIG_ENOUGH ];
	char *form_name;
//      char host_field[ 21 ];
//      char def_name_field[ 21 ];
	char temp_buf[ MAX_LOC + 1 ];
	i4 lognum;
//      char    fldname[33];
//	char dbms_second_menuitem[20];
	char *audlog=ERx("audit_log_");
	i4  audlen;

	PM_SCAN_REC state;
	STATUS status;
	i4 independent, dependent, len;
	char *comp_type;
	char expbuf[ BIG_ENOUGH ];
	char *regexp;
//	char derived_table_title[ BIG_ENOUGH ];
	char *p;
	bool c2_form=FALSE;
	bool doing_aud_log=FALSE;

	VCBFgf_pm_host = host; /* moved to static */
	VCBFgf_changed = FALSE;
	VCBFgf_cache_name = NULL;
	VCBFgf_cache_sharing = NULL;

    bGenInfoAlreadyRead = FALSE;

	VCBFgf_def_name= NULL;
	strcpy(comp_name,lpcomp->szname);
	switch (lpcomp->itype) {
	   case COMP_TYPE_NAME              :
	       strcpy(VCBFgf_pm_id,"gcn");
	       VCBFgf_def_name= NULL;
	       break;
	   case COMP_TYPE_DBMS              :
	       if( STcompare( comp_name,
		ERget( S_ST030F_default_settings ) ) == 0
	       )
		STcopy( ERx( "*" ), comp_name );
	       STprintf( VCBFgf_pm_id, ERx( "dbms.%s" ), comp_name );
	       VCBFgf_def_name= comp_name;
	       break;
	   case COMP_TYPE_INTERNET_COM      :
	       if( STcompare( comp_name, ERget( S_ST030F_default_settings ) ) == 0)
		       STcopy( ERx( "*" ), comp_name );
	       STprintf( VCBFgf_pm_id, ERx( "icesvr.%s" ), comp_name );
	       VCBFgf_def_name= comp_name;
	       break;
	   case COMP_TYPE_NET               :
	       if( STcompare( comp_name,
		ERget( S_ST030F_default_settings ) ) == 0
	       )
		STcopy( ERx( "*" ), comp_name );
	       STprintf( VCBFgf_pm_id, ERx( "gcc.%s" ), comp_name );
	       VCBFgf_def_name= comp_name;
	       break;
		case COMP_TYPE_DASVR            :
	       if( STcompare( comp_name,
		ERget( S_ST030F_default_settings ) ) == 0
	       )
		STcopy( ERx( "*" ), comp_name );
	       STprintf( VCBFgf_pm_id, ERx( "gcd.%s" ), comp_name );
	       VCBFgf_def_name= comp_name;
	       break;
	   case COMP_TYPE_BRIDGE            :
	       if( STcompare( comp_name,
		ERget( S_ST030F_default_settings ) ) == 0
	       )
		STcopy( ERx( "*" ), comp_name );
	       STprintf( VCBFgf_pm_id, ERx( "gcb.%s" ), comp_name );
	       VCBFgf_def_name= comp_name;
	       break;
	   case COMP_TYPE_STAR              :
	       if( STcompare( comp_name,
		ERget( S_ST030F_default_settings ) ) == 0
	       )
		STcopy( ERx( "*" ), comp_name );
	       STprintf( VCBFgf_pm_id, ERx( "star.%s" ), comp_name );
	       VCBFgf_def_name= comp_name;
	       break;
	   case COMP_TYPE_SECURITY          :
	       switch (lpcomp->ispecialtype) {
		 case GEN_FORM_SECURE_SECURE:
		    strcpy(VCBFgf_pm_id,"secure");
		    break;
		 case GEN_FORM_SECURE_C2    :
		    strcpy(VCBFgf_pm_id,"c2");
		    break;
		 case GEN_FORM_SECURE_GCF    :
		    strcpy(VCBFgf_pm_id,"gcf");
		    break;
		 default:
		    myinternalerror();
		    return FALSE;
	       }
	       VCBFgf_def_name= NULL;
	       break;
	   case COMP_TYPE_LOCKING_SYSTEM    :
	       strcpy(VCBFgf_pm_id,"rcp.lock");
	       VCBFgf_def_name= NULL;
	       break;
	   case COMP_TYPE_LOGGING_SYSTEM    :
	       strcpy(VCBFgf_pm_id,"rcp.log");
	       VCBFgf_def_name= NULL;
	       break;
	   case COMP_TYPE_SETTINGS          :
	       strcpy(VCBFgf_pm_id,"prefs");
	       VCBFgf_def_name= NULL;
	       break;
		case COMP_TYPE_RECOVERY          : // [Add    ] 14-Aug-1998: UK Sotheavut (UK$SO01)
			if( STcompare( comp_name, ERget( S_ST030F_default_settings ) ) == 0)
				STcopy( ERx( "*" ), comp_name );
			STprintf( VCBFgf_pm_id, ERx( "recovery.%s" ), comp_name );
			VCBFgf_def_name= comp_name;
			break;
	   case COMP_TYPE_GW_ORACLE         :
	       strcpy(VCBFgf_pm_id,"gateway.oracle");
	       VCBFgf_def_name= NULL;
	       break;
	   case COMP_TYPE_GW_DB2UDB         :
	       strcpy(VCBFgf_pm_id,"gateway.db2udb");
	       VCBFgf_def_name= NULL;
	       break;
	   case COMP_TYPE_GW_INFORMIX       :
	       strcpy(VCBFgf_pm_id,"gateway.informix");
	       VCBFgf_def_name= NULL;
	       break;
	   case COMP_TYPE_GW_SYBASE         :
	       strcpy(VCBFgf_pm_id,"gateway.sybase");
	       VCBFgf_def_name= NULL;
	       break;
	   case COMP_TYPE_GW_MSSQL          :
	       strcpy(VCBFgf_pm_id,"gateway.mssql");
	       VCBFgf_def_name= NULL;
	       break;
	   case COMP_TYPE_GW_ODBC           :
	       strcpy(VCBFgf_pm_id,"gateway.odbc");
	       VCBFgf_def_name= NULL;
	       break;
	   case COMP_TYPE_TRANSACTION_LOG   :
//	   case COMP_TYPE_RECOVERY          : // [Comment] 14-Aug-1998: UK Sotheavut (UK$SO01)
	   case COMP_TYPE_ARCHIVER          :
	   default:
	       myinternalerror();
	       return FALSE;

	}

//	STcopy(ERget( S_ST0354_databases_menu ),dbms_second_menuitem);

#ifdef VCBF_FUTURE
      replace it => exec frs message :ERget( S_ST0344_selecting_parameters );
#endif

	/* first component of pm_id identifies component type */
	comp_type = PMmGetElem( config_data, 0, VCBFgf_pm_id );

	/*
	** Since the forms system doesn't allow nested calls to the
	** same form, dmf_independent must be used if this function is
	** being called to drive the independent DBMS cache parameters
	** form.
	*/
	if( STcompare( comp_type, ERx( "dmf" ) ) == 0 )
	{
		form_name = ERx( "dmf_independent" );
		/* HACK: skip past "dmf." */
		p = VCBFgf_pm_id;
		CMnext( p );
		CMnext( p );
		CMnext( p );
		CMnext( p );
		STprintf( VCBFgf_real_pm_id, ERx( "dbms.%s" ), p );
	}
	else if( STcompare( comp_type, ERx( "c2" ) ) == 0 )
	{
		/*
		** C2 form is independent
		*/
		form_name = ERx("c2_independent");
		STcopy( VCBFgf_pm_id, VCBFgf_real_pm_id );
		c2_form=TRUE;
	}
	else
	{
//		if( STcompare(comp_type, ERx( "dbms" ) ) == 0 )
//		{
//		  if (!ingres_menu)
//		  *dbms_second_menuitem=EOS;
//		}
		form_name = ERx( "generic_independent" );
		STcopy( VCBFgf_pm_id, VCBFgf_real_pm_id );
	}


	/* set value of my_name */
	if (c2_form)
		my_name = NULL;
	else  if( VCBFgf_def_name == NULL )
		my_name = STalloc( ERget( S_ST030F_default_settings ) );
	else
	{
		if( STcompare( VCBFgf_def_name, ERx( "*" ) ) == 0 )
			my_name = STalloc(
				ERget( S_ST030F_default_settings ) );
		else
			my_name = STalloc( VCBFgf_def_name ); 
	}

	/* extract resource owner name from real_pm_id */
	VCBFgf_owner = PMmGetElem( config_data, 0, VCBFgf_real_pm_id );
	if (VCBFgf_owner) {
	   strcpy(bufVCBFgf_owner,VCBFgf_owner);
	   VCBFgf_owner=bufVCBFgf_owner;
	}
	len = PMmNumElem( config_data, VCBFgf_real_pm_id );
		
	/* compose "instance" */
	if( len == 2 )
	{
		/* instance name is second component */
		name = PMmGetElem( config_data, 1, VCBFgf_real_pm_id );
		if( STcompare( name,
			ERget( S_ST030F_default_settings ) ) == 0
		)
			STcopy( ERx( "*" ), VCBFgf_instance );
		else
			STcopy( name, VCBFgf_instance ); 
		STcat( VCBFgf_instance, ERx( "." ) );
	}
	else if( len > 2 )
	{
		char *p;
		/* VCBFgf_instance name is all except first */

		for( p = VCBFgf_real_pm_id; *p != '.'; CMnext( p ) );
		CMnext( p );
		STcopy( p, VCBFgf_instance );
		STcat( VCBFgf_instance, ERx( "." ) );
	}
	else
		STcopy( ERx( "" ), VCBFgf_instance );

	if (c2_form)
	{
		for(lognum=1;lognum<=MAX_AUDIT_LOGS;lognum++)
		{
			audit_log_list[lognum-1]=NULL;
		}
	}

	if( STcompare( comp_type, ERx( "c2" ) ) == 0 )
		VCBFgf_pm_host = ERx( "*" );

	/* check whether this is a DBMS cache definition */
	if( STcompare( comp_type, ERx( "dmf" ) ) == 0 )
	{
		char cache_desc[ BIG_ENOUGH ];

		/* if so, turn off backup option */
		VCBFgf_backup_mode = FALSE;

		/* and display cache_desc */
		value = PMmGetElem( config_data, 1, VCBFgf_real_pm_id );
		if( STcompare( ERget( S_ST0332_shared ), value ) == 0 ) {
		  /*    exec frs putform(                             */ 
		  /*            cache_desc = :ERget( S_ST0332_shared )*/
#ifdef FUTURE
   was that needed?
#endif
		}
		else
		{
			name = STalloc( ERget( S_ST0333_private ) );
			value = STalloc( PMmGetElem( config_data, 2,
				VCBFgf_real_pm_id ) );
			if( STcompare( value, ERx( "*" ) ) == 0 )
			{
				MEfree( value );
				value = STalloc(
					ERget( S_ST030F_default_settings ) );
			}
			STprintf( cache_desc, ERx( "%s ( %s=%s )" ), 
				name, ERget( S_ST0334_owner ), value );
			MEfree( name );
			MEfree( value );
		  /*    exec frs putform(                */ 
		  /*            cache_desc = :cache_desc */
#ifdef FUTURE
   was that needed?
#endif
		}
	}
	else
	{
		/* this is not a DBMS cache definition, so */
		VCBFgf_backup_mode = TRUE;

		/* prepare a backup LOCATION for the data file */
		STcopy( ERx( "" ), temp_buf );
		LOfroms( FILENAME, temp_buf, &backup_data_file );
		LOuniq( ERx( "cacbfd1" ), NULL, &backup_data_file );
		LOtos( &backup_data_file, &name );
		STcopy( tempdir, backup_data_buf ); 
		STcat( backup_data_buf, name );
		LOfroms( PATH & FILENAME, backup_data_buf, &backup_data_file );
	
		/* prepare a backup LOCATION for the change log */
		STcopy( ERx( "" ), temp_buf );
		LOfroms( FILENAME, temp_buf, &backup_change_log_file );
		LOuniq( ERx( "cacbfc1" ), NULL, &backup_change_log_file );
		LOtos( &backup_change_log_file, &name );
		STcopy( tempdir, backup_change_log_buf ); 
		STcat( backup_change_log_buf, name );
		LOfroms( PATH & FILENAME, backup_change_log_buf,
			&backup_change_log_file );
	
		/* prepare a backup LOCATION for the protected data file */
		STcopy( ERx( "" ), temp_buf );
		LOfroms( FILENAME, temp_buf, &backup_protect_file );
		LOuniq( ERx( "cacbfp1" ), NULL, &backup_protect_file );
		LOtos( &backup_protect_file, &name );
		STcopy( tempdir, backup_protect_buf ); 
		STcat( backup_protect_buf, name );
		LOfroms( PATH & FILENAME, backup_protect_buf,
			&backup_protect_file );
	
		/* write configuration backup */ 
		if( PMmWrite( config_data, &backup_data_file ) != OK )
		{
			mymessage(ERget(S_ST0326_backup_failed));
		     /* IIUGerr( S_ST0326_backup_failed, 0, 0 ); */
			LOdelete( &backup_data_file );
			if( my_name != NULL )
				MEfree( my_name );
			return( FALSE );
		}

		/* write change log backup */ 
		if( copy_LO_file( &change_log_file, &backup_change_log_file )
			!= OK ) 
		{
			mymessage(ERget(S_ST0326_backup_failed));
		   /*   IIUGerr( S_ST0326_backup_failed, 0, 0 );  */
			(void) LOdelete( &backup_change_log_file );
			return( FALSE );
		}

		/* write protected data backup */ 
		if( PMmWrite( protect_data, &backup_protect_file ) != OK )
		{
			mymessage(ERget(S_ST0326_backup_failed));
		   /*   IIUGerr( S_ST0326_backup_failed, 0, 0 );  */
			LOdelete( &backup_protect_file );
			if( my_name != NULL )
				MEfree( my_name );
			return( FALSE );
		}

	}

	/* prepare expbuf */
	STprintf( expbuf, ERx( "%s.%s.%s.%%" ), SystemCfgPrefix, 
		  VCBFgf_pm_host, VCBFgf_real_pm_id );

	regexp = PMmExpToRegExp( config_data, expbuf );

	/* initialize independent and dependent resource counters */ 
	independent = dependent = 0;

	/* get the name components for this set of resources */
	len = PMmNumElem( config_data, expbuf );

	VCBFInitGenLines();

	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		char *units,*short_name;

		/* extract parameter name from full resource */
		short_name = PMmGetElem( config_data, len - 1, name );

		/* save dbms cache_name value */
		if( STcompare( short_name, ERx( "cache_name" ) ) == 0 ) {
			strcpy(bufVCBFgf_cache_name,value);
			VCBFgf_cache_name = bufVCBFgf_cache_name;
		}

		/* save dbms cache_sharing value */
		if( STcompare( short_name, ERx( "cache_sharing" ) ) == 0 ) {
			strcpy(bufVCBFgf_cache_sharing,value);
			VCBFgf_cache_sharing = bufVCBFgf_cache_sharing;
		}

		/* skip derived resources */
		if( CRderived( name ) )
		{
			++dependent;
			continue;
		}
		++independent;

		/* get units */
		if( PMmGet( units_data, name, &units ) != OK )
			units = ERx( "" );

		/* use just parameter name from now on */
		name=short_name;

		/* skip db_list and assume it is only a dbms parameter */
		if( STcompare( name, ERx( "database_list" ) ) == 0 )
		{
			strcpy(bufVCBFgf_db_list,value);
			VCBFgf_db_list=bufVCBFgf_db_list;
			continue;
		}

		if( STbcompare( name, 8, ERx( "log_file" ), 8, 0 ) == 0 )
		{
			continue;
		}
		if( STbcompare( name, 8, ERx( "dual_log" ), 8, 0 ) == 0 )
		{
			continue;
		}
		if( STbcompare( name, 12, ERx( "raw_log_file" ), 12, 0 ) == 0 )
		{
			continue;
		}
		if( STbcompare( name, 12, ERx( "raw_dual_log" ), 12, 0 ) == 0 )
		{
			continue;
		}

		if (c2_form)
		{
			audlen=(i4)STlength(audlog);

			if(STbcompare(name, audlen, audlog, audlen, 1)==0)
			{
			    CVal(name+STlength(audlog), &lognum);
			    if(lognum>0 && lognum <=MAX_AUDIT_LOGS)
			    {
				audit_log_list[lognum-1]=value;
			    }
			}
			else
			{
			    VCBFAddGenLine(name, value, units, FALSE);
			}
		}
		else
		{
#ifdef OIDSK
			if (! (lpcomp->itype==COMP_TYPE_NET && !stricmp(name,"inbound_limit")))
#endif
				VCBFAddGenLine(name, value, units, FALSE);
		}
	}
	/*
	** Now we  have the audit logs load them in order.
	*/
        VCBFInitAuditLogLines();
	if (c2_form)
	{
		for(lognum=1; lognum<=MAX_AUDIT_LOGS; lognum++)
		{
			value=audit_log_list[lognum-1];
			if(!value)
				value="";
			VCBFAddAuditLogLine(lognum, value);
		}
	}
#if 0
	/* set derived_table_title */
	if( STcompare( comp_type, ERx( "dbms" ) ) == 0 )
		STcopy( ERget( S_ST036C_derived_dbms ), derived_table_title );
	else if( STcompare( comp_type, ERx( "lock" ) ) == 0 ) {
		if(!ingres_menu)
		  STcopy( ERget( S_ST0371_derived_locking ), 
		  derived_table_title );
	}
	else if( STcompare( comp_type, ERx( "dmf" ) ) == 0 )
		STcopy( ERget( S_ST036D_derived_cache ), derived_table_title );
	else if( STcompare( comp_type, ERx( "gcc" ) ) == 0 )
		STcopy( ERget( S_ST036E_derived_net ), derived_table_title );
	else if( STcompare( comp_type, ERx( "c2" ) ) == 0 )
		STcopy( ERget( S_ST0379_derived_security ), 
			derived_table_title );
	else if( STcompare( comp_type, ERx( "secure" ) ) == 0 )
		STcopy( ERget( S_ST037B_security_param_deriv ), 
			derived_table_title );
	else if( STcompare( comp_type, ERx( "star" ) ) == 0 )
		STcopy( ERget( S_ST036F_derived_star ), derived_table_title );
	else if( STcompare( VCBFgf_real_pm_id, ERx( "rcp.log" ) ) == 0 )
		STcopy( ERget( S_ST0370_derived_logging ),
			derived_table_title );
	else if( STcompare( VCBFgf_real_pm_id, ERx( "rcp.lock" ) ) == 0 )
		STcopy( ERget( S_ST0371_derived_locking ),
			derived_table_title );
#endif
	if( independent == 0 )
	{
		mymessagewithtitle("According to the current configuration rules,\n"
			  " all parameters for the selected component are derived\n"
			  " from other sources");
		return FALSE;
	}

	if( my_name != NULL )
	   MEfree( my_name );
	return TRUE;
}

#define DSKPROTOCOL ",sqlapipe"

static OIDSKINIFILEINFO DSkMainparms[]=
 {
#if 0
    {"Server Name"            ,"servername"     ,"string"    ,"ingres,sqlapipe",SPECIAL_SERVERNAME,0,0,NULL},
#endif
    {"Cache Size"             ,"cache"          ,"2 K Pages","2000",SPECIAL_NUM_MINMAX,30,1000000,NULL},
    {"SortCache"              ,"sortcache"      ,"K"        ,"2000",SPECIAL_NUM_MINMAX,30,1000000,NULL},
    {"Lock Timeout"           ,"locktimeout"    ,"seconds"  ,"300" ,SPECIAL_NUM_MINMAX,-1,1800   ,NULL},
    {"Databases Path"         ,"dbdir"          ,"string"   ,"%II_SYSTEM%\\Desktop",SPECIAL_DEF_PATH_DESKTOP,0,0,NULL},
    {"Log Files Path"         ,"logdir"         ,"string"   ,"%II_SYSTEM%\\Desktop",SPECIAL_DEF_PATH_DESKTOP,0,0,NULL},
    {"Temp Files Path"        ,"tempdir"        ,"string"   ,"%II_SYSTEM%\\Desktop",SPECIAL_DEF_PATH_INGTMP,0,0,NULL},
//    {"Access Partitions"      ,"PARTITIONS"     ,"boolean"  ,"ON"  ,SPECIAL_NONE,0,0,NULL},
    {"Allow R.O. transactions","readonly"       ,"boolean"  ,"OFF" ,SPECIAL_NONE,0,0,NULL},
    {"Log File Prealloc"      ,"logfileprealloc","boolean"  ,"OFF" ,SPECIAL_NONE,0,0,NULL},
    {"Maximize optimization"  ,"optimizerlevel" ,"boolean"  ,"ON"  ,SPECIAL_MAX_OPT,0,0,NULL},
    {"Oracle outer join"      ,"oracleouterjoin","boolean"  ,"OFF" ,SPECIAL_NONE,0,0,NULL},
    {NULL,NULL,NULL,NULL,0,0,0,NULL},
} ;
static OIDSKINIFILEINFO DSkAdvparms[]=
 {
//    {"Character Set File"     ,"characterset"   ,"string"             ,NULL,SPECIAL_NONE,0,0,NULL},
    {"Client Check"           ,"clientcheck"    ,"boolean"            ,NULL,SPECIAL_NONE,0,0,NULL},
//    {"Display Level (0-4)"    ,"displevel"      ,"number (0-4)"       ,NULL,SPECIAL_NUM_MINMAX,0,4,NULL},
//    {"Error File"             ,"errorfile"      ,"string"             ,NULL,SPECIAL_NONE,0,0,NULL},
    {"Group Commit"           ,"groupcommit"    ,"number"             ,"2" ,SPECIAL_NUM_MINMAX,1,32767,NULL},
//    {"Locks"                  ,"locks"          ,"number (0=no limit)",NULL,SPECIAL_NUM_MINMAX,0,32767,NULL},
    {"Log File"               ,"log"            ,"string"             ,NULL,SPECIAL_NONE,0,0,NULL},
    {"ossamplerate"           ,"ossamplerate"   ,"seconds"            ,NULL,SPECIAL_NUM_MINMAX,0,255,NULL},
    {"osavgwindow"            ,"osavgwindow"    ,"number"             ,NULL,SPECIAL_NUM_MINMAX,1,255,NULL},
//    {"Time Colon Delim. only" ,"timecolononly"  ,"boolean"            ,NULL,SPECIAL_NONE,0,0,NULL},
//    {"Timeout"                ,"timeout"        ,"minutes"            ,NULL,SPECIAL_NUM_MINMAX,0,200,NULL},
    {"Timezone"               ,"timezone"       ,"-12 to +12"         ,NULL,SPECIAL_NUM_MINMAX,-12,+12,NULL},
    {"Max Connected Apps"     ,"users"          ,"number (1-800)"     ,NULL,SPECIAL_NUM_MINMAX,1,800,NULL},
//    {"Workalloc"              ,"workalloc"      ,"bytes"              ,NULL,SPECIAL_NUM_MINMAX,1000,32000,NULL},
//    {"WorkLimit"              ,"worklimit"      ,"bytes"              ,NULL,SPECIAL_NUM_MINMAX,1000,32000,NULL},
	{NULL,NULL,NULL,NULL,0,0,0,NULL},
} ;

static LPOIDSKINIFILEINFO GetDskParmInfo(char * lpname)
{
    LPOIDSKINIFILEINFO lp1;
	for (lp1=DSkMainparms;lp1->lpname;lp1++) {
		if (!strcmp(lp1->lpname,lpname))
			return lp1;
	}
	for (lp1=DSkAdvparms;lp1->lpname;lp1++) {
		if (!strcmp(lp1->lpname,lpname))
			return lp1;
	}
	return NULL;
}

static char *fstrncpy(char * pu1, char * pu2,int n)
{	// don't use STlcopy (may split the last char)
	char * bufret = pu1;
	while ( n >= CMbytecnt(pu2) +1 ) { // copy char only if it fits including the trailing \0
		n -=CMbytecnt(pu2);
		if (*pu2 == EOS) 
			break;
		CMcpyinc(pu2,pu1);
	}
	*pu1=EOS;
	return bufret;

}

static BOOL GenPaneOnEditStep2(char * temp_buf,char * name_val,char * oldval, char * newval, BOOL doing_aud_log,int lognum)
{
	/* called ONLY IF THERE WAS A CHANGE*/
	bool avoid_set_resource = FALSE; /* bug #80450 */
	char in_buf[ BIG_ENOUGH ];
	char * value;
	i4     number;

	strcpy(in_buf,newval);

	/* just toggle if resource is boolean */
	if( CRtype( CRidxFromPMsym( temp_buf ) ) == CR_BOOLEAN )
	{
		if (stricmp(in_buf,"on"))/* not on =>OFF */
		    STcopy( "OFF", in_buf );
		else  {
		    char parambuf[ BIG_ENOUGH ];

		    if (!stricmp(in_buf,oldval))
			return TRUE; 
		    /*
		    ** bug #80450 - both cache_sharing and dmcm
		    ** cannot be turned on
		    */


		    STcopy( "OFF", in_buf );
#ifndef OIDSK
		    if ( STcompare(name_val,
				ERx("cache_sharing")) == 0 )
		    {
			STprintf( parambuf, ERx("%s.%s.%s."),
				SystemCfgPrefix, VCBFgf_pm_host, VCBFgf_owner );
			STcat( parambuf, VCBFgf_instance );
			STcat( parambuf, ERx("dmcm") );
			if ( PMmGet(config_data, parambuf, &value) == OK
				&& !(ValueIsBoolTrue(value)) )
			{
			    /* 
			    ** dmcm is OFF, thus cache_sharing 
			    ** can be turned ON.
			    */
			    STcopy( ERx( "ON" ), in_buf );
			}
			else
			{
			    avoid_set_resource = TRUE;
			    mymessagewithtitle(ERget( S_ST0444_dmcm_warning ) );
			}
		    }
		    else if ( STcompare(name_val,
			ERx("dmcm")) == 0 )
		    {
			STprintf( parambuf, ERx("%s.%s.%s."),
				SystemCfgPrefix, VCBFgf_pm_host, VCBFgf_owner );
			STcat( parambuf, VCBFgf_instance );
			STcat( parambuf, ERx("cache_sharing") );
			if ( PMmGet(config_data, parambuf, &value) == OK
				&& !(ValueIsBoolTrue(value)) )
			{
			    /*
			    ** cache_sharing is OFF, thus dmcm
			    ** can be turned ON.
			    */
			    STcopy( ERx( "ON" ), in_buf );
			}
			else
			{
			    avoid_set_resource = TRUE;
			    mymessagewithtitle(ERget( S_ST0444_dmcm_warning ) );
 
			}
		    }
		    else
#endif
			STcopy( "ON", in_buf );
		}
	}

	if( !avoid_set_resource)  /* bug #80450, do not call 
				  ** set_resource, since parameter 
				  ** value is not to be changed 
				  */
	{
	    if ( set_resource( temp_buf, in_buf ) )
	    {
		/* change succeeded */ 
		VCBFgf_changed = TRUE;

		if( STcompare( name_val, ERx( "cache_name" ) ) == 0 ) {
			strcpy(bufVCBFgf_cache_name,in_buf);
			VCBFgf_cache_name = bufVCBFgf_cache_name;
		 }

		if( STcompare( name_val, ERx( "cache_sharing" ) ) == 0 ) {
			strcpy(bufVCBFgf_cache_sharing,in_buf);
			VCBFgf_cache_sharing = bufVCBFgf_cache_sharing;
		}
		/* b95887 - Is DMF cache enabled for this pg size */
		if( STcompare( name_val, ERx("default_page_size")) == 0) {
			(void)CVal(in_buf, &number);
			if(default_page_cache_off(VCBFgf_pm_id, &number, VCBFgf_cache_sharing, VCBFgf_cache_name))
				mymessage (ERget(S_ST0449_no_cache_pg_default));
		}
#ifndef OIDSK
		if ((STcompare( VCBFgf_real_pm_id, ERx( "rcp.log" ) ) == 0) &&
		    (STcompare( name_val, ERx( "block_size") ) == 0))
			mymessagewithtitle(ERget(S_ST0533_reformat_log));
#endif
		/*
		** For various historical reasons we have 
		** two sets of log names. CBF reads in c2.audit_log_X
		** but SXF reads c2.log.audit_log_X, so make sure
		** they stay in sync
		*/
		if(doing_aud_log==TRUE)
		{
		     STprintf( temp_buf, 
			       ERx( "%s.%s.c2.log.audit_log_%d" ), 
			       SystemCfgPrefix, VCBFgf_pm_host,  lognum );
		      (VOID) set_resource( temp_buf, in_buf ); 
		}
		/* 
		** update tablefield. Note this works since  all 
		** tablefields have a column called "value" which 
		** holds the pmvalue
		*/


		/* load config.dat */
		if( PMmLoad( config_data, &config_file,
			display_error ) != OK )
		{
			mymessage(ERget(S_ST0315_bad_config_read));
			VCBFRequestExit();
			return FALSE;
		}

		/*
		** 08-apr-1997 (thaju02)
		** load protect.dat to get latest changes
		*/
		if( PMmLoad( protect_data, &protect_file,
			display_error ) != OK )
		{
			mymessage(ERget(S_ST0320_bad_protect_read));
			VCBFRequestExit();
			return FALSE;
		}
		strcpy(newval,in_buf);
		return TRUE;
	    }
	    return FALSE;
	}
	strcpy(newval,in_buf);
	return TRUE;
#ifdef FUTURE
   verify that in case of error exit has been done
#endif
}

BOOL VCBFllGenPaneOnEdit( LPGENERICLINEINFO lpgenlineinfo, char * lpnewvalue)
{
     char temp_buf[400];
     char in_buf[200];
     BOOL bResult;
     strcpy(in_buf,lpnewvalue);
     if (!strcmp(lpgenlineinfo->szvalue,in_buf))
        return TRUE;
     /* compose name of resource to be set */
     STprintf( temp_buf, ERx( "%s.%s.%s." ), SystemCfgPrefix,
	       VCBFgf_pm_host, VCBFgf_owner );
     STcat( temp_buf, VCBFgf_instance );
     STcat( temp_buf, lpgenlineinfo->szname );
     bResult= GenPaneOnEditStep2(temp_buf,lpgenlineinfo->szname,lpgenlineinfo->szvalue,in_buf,FALSE,0);
     if (bResult)
	strcpy(lpgenlineinfo->szvalue,in_buf);
     return bResult;
}
BOOL VCBFllAuditLogOnEdit( LPAUDITLOGINFO lpauditloglineinfo, char * lpnewvalue)
{
     char temp_buf[400];
     char in_buf[200];
     BOOL bResult;
     strcpy(in_buf,lpnewvalue);
     /* compose name of resource to be set */
     STprintf( temp_buf, ERx( "%s.%s.c2.audit_log_%d" ), 
	       SystemCfgPrefix, VCBFgf_pm_host,  lpauditloglineinfo->log_n);
     bResult = GenPaneOnEditStep2(temp_buf,"",lpauditloglineinfo->szvalue,in_buf,TRUE,lpauditloglineinfo->log_n);
     if (bResult)
	strcpy(lpauditloglineinfo->szvalue,in_buf);
     return bResult;
}

char * VCBFllGet_db_listparm() // needed for DB form
{
	return VCBFgf_db_list;
}  
char * VCBFllGet_def_name()    // needed for DB, bridge/prot &prot forms,& dep/generic
{
	return VCBFgf_def_name;
}  
BOOL VCBFllSetChanged()        // needed for DB, protocols forms 
{
    VCBFgf_changed = TRUE;
    return TRUE;
}
char * VCBFllGet_pm_id()       // needed for dependent forms
{
	return VCBFgf_pm_id;
}
BOOL VCBFllGet_backup_mode()
{
	return VCBFgf_backup_mode ;
}

void
get_cache_status(char *pm_id, char *page, char **stat_val)
{
//    PM_SCAN_REC       state;
    char        real_pm_id[ BIG_ENOUGH ];
    char        expbuf[ BIG_ENOUGH ];
//    char      *regexp;
    char        *p;
//    char      *name;
    char        *pm_host = host;

    p = pm_id;
    CMnext( p );
    CMnext( p );
    CMnext( p );
    CMnext( p );
    STprintf( real_pm_id, ERx( "dbms.%s.cache%s_status" ), p, page);
    STprintf( expbuf, ERx( "%s.%s.%s" ), SystemCfgPrefix, pm_host, real_pm_id );
    if ( PMmGet ( config_data, (char *)&expbuf, stat_val ) != OK )
	*stat_val = ERx( "" );
}
static char VCBFllCache_pm_id[80];
BOOL VCBFllInitCaches()
{

     char pm_id[ BIG_ENOUGH ];
     char parambuf[ BIG_ENOUGH ];
     char *cacheval;
     char *cachename;

     /* get cache_sharing and cache_name */
     STprintf( parambuf, ERx("%s.%s.%s."),
	SystemCfgPrefix, VCBFgf_pm_host, VCBFgf_owner );
     STcat( parambuf, VCBFgf_instance );
     STcat( parambuf, ERx("cache_sharing") );
     if (PMmGet(config_data, parambuf, &cacheval) != OK)
	 cacheval = NULL;
     STprintf( parambuf, ERx("%s.%s.%s."),
	SystemCfgPrefix, VCBFgf_pm_host, VCBFgf_owner );
     STcat( parambuf, VCBFgf_instance );
     STcat( parambuf, ERx("cache_name") );
     if (PMmGet(config_data, parambuf, &cachename) != OK)
	 cachename = NULL;

     if((VCBFgf_cache_name==NULL || VCBFgf_cache_sharing==NULL) && ingres_menu)
     {
	mymessage(ERget(S_ST0322_missing_cache));

	return FALSE;
     }
     if( ValueIsBoolTrue( VCBFgf_cache_sharing ) && ingres_menu )
     {
	STprintf( pm_id, ERx( "dmf.%s.%s" ),
		ERget( S_ST0332_shared ), cachename );
     }
     else
     {
	STprintf( pm_id, ERx( "dmf.%s.%s" ),
			ERget( S_ST0333_private ), VCBFgf_def_name );
     }

     if(!ingres_menu && VCBFgf_cache_name==NULL) 
       VCBFgf_cache_name = STalloc( ERget( S_ST030F_default_settings ) );
     {  
	char *def_name = VCBFgf_cache_name;
//        char  cache_type[ BIG_ENOUGH ]; 
//        char  status_flag[ BIG_ENOUGH ];
//        char  description[ BIG_ENOUGH ];
//        char  temp_buf[ MAX_LOC + 1 ];
	char    *config_cache = ERx( "config_cache" );
	char    cache_name[ BIG_ENOUGH ];
	char    *stat_val;
//        char  fldname[33];
	char    *name, *value;
	char    cache_desc[BIG_ENOUGH];
//        char  *default_cache;
  
    //    char  *p;
//        i4           len, i;
//        char  *regexp;
	char    new_pm_id[BIG_ENOUGH];
	bool    changed = FALSE;
  
	STcopy(def_name, cache_name);
	STcopy(pm_id, new_pm_id);


	/* display cache_desc */
	value = PMmGetElem( config_data, 1, pm_id );
	if( STcompare( ERget( S_ST0332_shared ), value ) == 0 )
		STcopy(ERget( S_ST0332_shared ), cache_desc);
	else
	{
		name = STalloc( ERget( S_ST0333_private ) );
		value = STalloc( PMmGetElem( config_data, 2, pm_id ) );
		if( STcompare( value, ERx( "*" ) ) == 0 )
		    value = STalloc( ERget( S_ST030F_default_settings ) );
		STprintf( cache_desc, ERx( "%s ( %s=%s )" ),
				name, ERget( S_ST0334_owner ), value );
		MEfree( name );
		MEfree( value );
	}


	VCBFInitCacheLines();
	if ( ingres_menu ) {
	  /*
	  ** now check if dmf cache for 2k pages is already setup/inuse.
	  */
	  get_cache_status( pm_id, ERget( S_ST0445_p2k ), &stat_val);
	  VCBFAddCacheLine(ERget( S_ST0431_dmf_cache_2k ),stat_val);
	}
	else {
	  /*
	  ** check if dmf cache for 2k pages is already setup/inuse
	  ** for Jasmine
	  */
#ifndef OIDSK   // doesn't apply to desktop, and didn't compile because S_ST0445_p2K didn't exist
	  get_cache_status( pm_id, ERget( S_ST0445_p2k ), &stat_val);
	  VCBFAddCacheLine(dupstr1(ERget( S_ST0431_dmf_cache_2k )),
			   stat_val);
#endif
	}
	/*
	** check if dmf cache for 4k pages is already setup/inuse
	*/
	get_cache_status( pm_id, ERget( S_ST0437_p4k ), &stat_val);
	VCBFAddCacheLine(ERget( S_ST0432_dmf_cache_4k ),stat_val);

	get_cache_status( pm_id, ERget( S_ST0438_p8k ), &stat_val);
	VCBFAddCacheLine(ERget( S_ST0433_dmf_cache_8k ),stat_val);

	get_cache_status( pm_id, ERget( S_ST0439_p16k ), &stat_val);
	VCBFAddCacheLine(ERget( S_ST0434_dmf_cache_16k ),stat_val);

	get_cache_status( pm_id, ERget( S_ST0440_p32k ), &stat_val);
	VCBFAddCacheLine(ERget( S_ST0435_dmf_cache_32k ),stat_val);

	get_cache_status( pm_id, ERget( S_ST0441_p64k ), &stat_val);
	VCBFAddCacheLine(ERget( S_ST0436_dmf_cache_64k ),stat_val);
     }
     strcpy(VCBFllCache_pm_id,pm_id);
     return TRUE;
}



bool
toggle_cache_status(char *form, char *fldname, char *pm_id, char *page,char *bufstatus_flag)
{
//    char      field[33];
    char        *formname = form;
    char        *p;
    char        real_pm_id[BIG_ENOUGH];
    char        expbuf[BIG_ENOUGH];
//    char        *regexp;
    char        *pm_host = host;
    char        tmppage[6];

    STcopy(page, tmppage);

//    exec frs inquire_frs form (:field = field);

//    exec frs getrow :formname :field (:status_flag = status);

    p = pm_id;
    CMnext( p );
    CMnext( p );
    CMnext( p );
    CMnext( p );
    STprintf( real_pm_id, ERx( "dbms.%s.cache%s_status" ), p, tmppage);
    STprintf( expbuf, ERx( "%s.%s.%s" ), SystemCfgPrefix, pm_host, real_pm_id );
    
    if (ValueIsBoolTrue(bufstatus_flag))
    {
	STcopy(ERx("OFF"), bufstatus_flag);
    }
    else
    {
	STcopy(ERx("ON"), bufstatus_flag);
    }

//    exec frs putrow :formname :field (status = :status_flag);

    if (set_resource(expbuf, bufstatus_flag))
    {
	if (PMmLoad(config_data, &config_file, display_error) != OK) {
	      mymessage(ERget(S_ST0315_bad_config_read));
	      VCBFRequestExit();
	      return FALSE;
        }

	/*
	** 08-apr-1997 (thaju02) load protect.dat to get latest changes
	*/
	if (PMmLoad(protect_data, &protect_file, display_error) != OK) {
	      mymessage(ERget(S_ST0320_bad_protect_read) );
	      VCBFRequestExit();
	      return FALSE;
	}

//      exec frs redisplay;
//      exec frs sleep 3;
    }

    return(TRUE);
}
BOOL VCBFllOnCacheEdit(LPCACHELINEINFO lpcachelineinfo, char * newvalue)
{
	char cache_type[100];
	char oldvalue[100];
	BOOL changed;
	char *config_cache = "config_cache";
	char    fldname[33];
	strcpy(cache_type,lpcachelineinfo->szcachename);
	strcpy(oldvalue,lpcachelineinfo->szstatus);
	if (!strnicmp(oldvalue,newvalue,3))
		return TRUE;

	if (STcompare(cache_type, ERget( S_ST0431_dmf_cache_2k )) == 0)
	    changed = toggle_cache_status(config_cache, fldname,
				VCBFllCache_pm_id, ERget( S_ST0445_p2k ),oldvalue);
	if (STcompare(cache_type, ERget( S_ST0432_dmf_cache_4k )) == 0)
	    changed = toggle_cache_status(config_cache, fldname,
				VCBFllCache_pm_id, ERget( S_ST0437_p4k ),oldvalue);
	if (STcompare(cache_type, ERget( S_ST0433_dmf_cache_8k )) == 0)
	    changed = toggle_cache_status(config_cache, fldname,
				VCBFllCache_pm_id, ERget( S_ST0438_p8k ),oldvalue);
	if (STcompare(cache_type, ERget( S_ST0434_dmf_cache_16k )) == 0)
	    changed = toggle_cache_status(config_cache, fldname,
				VCBFllCache_pm_id, ERget( S_ST0439_p16k ),oldvalue);
	if (STcompare(cache_type, ERget( S_ST0435_dmf_cache_32k )) == 0)
	    changed = toggle_cache_status(config_cache, fldname,
			       VCBFllCache_pm_id, ERget( S_ST0440_p32k ),oldvalue);
	if (STcompare(cache_type, ERget( S_ST0436_dmf_cache_64k )) == 0)
	    changed = toggle_cache_status(config_cache, fldname,
			       VCBFllCache_pm_id, ERget( S_ST0441_p64k ),oldvalue);
	if (changed) {
	    VCBFgf_changed = TRUE;
	    // oldvalue was changed by toggle_cache_status()
	    strcpy(lpcachelineinfo->szstatus,oldvalue);
	    return TRUE;
	}
	return FALSE;
}


BOOL VCBFllResetGenValue(LPGENERICLINEINFO lpgenlineinfo) 
{
    char temp_buf[200];
    int iret;
    char * value ="";
    STprintf( temp_buf, ERx( "%s.%s.%s." ), 
	  SystemCfgPrefix, VCBFgf_pm_host, VCBFgf_owner );
    STcat( temp_buf, VCBFgf_instance );
    STcat( temp_buf, lpgenlineinfo->szname );

    iret = QuestionMessageWithParam (S_ST0329_reset_prompt,lpgenlineinfo->szname);
    if (iret == IDYES) 
    {
	if( reset_resource( temp_buf, &value, TRUE ) ) {
	    strcpy(lpgenlineinfo->szvalue,value);
	    VCBFgf_changed = TRUE;
	    return TRUE;
	}
    }
    return FALSE;
}
BOOL VCBFllResetAuditValue(LPAUDITLOGINFO lpauditloglineinfo)
{
    char temp_buf[200],name_val[200];
    int iret;
    char * value ="";
    if (lpauditloglineinfo->szvalue[0]==EOS)
       return FALSE;
    STprintf(name_val,"audit_log_%d",lpauditloglineinfo->log_n);
    STprintf( temp_buf, "%s.%s.c2.%s", SystemCfgPrefix,
	  VCBFgf_pm_host, name_val);
    iret = QuestionMessageWithParam (S_ST0329_reset_prompt,lpauditloglineinfo->szvalue);
    if (iret == IDYES)  {
	if( reset_resource( temp_buf, &value, TRUE ) )
	{
		strcpy(lpauditloglineinfo->szvalue,value);
		VCBFgf_changed = TRUE;
		/* Reset the other name as well */
		STprintf( temp_buf, "%s.%s.c2.log.%s",
		      SystemCfgPrefix, VCBFgf_pm_host, name_val);
		(VOID) reset_resource( temp_buf, &value, TRUE ) ;
	    return TRUE;
	}
    }
    return FALSE;
}

BOOL VCBFllOnMainComponentExit(LPCOMPONENTINFO lpcomp)
{

   if (bExitRequested)
       return FALSE;
   if( VCBFgf_backup_mode && VCBFgf_changed )
   {
	int iret;
        char buf1[200];
        char *p1="Do you want to save your changes to the modified %s Component ?";
        switch (lpcomp->itype) {
           case COMP_TYPE_NAME             :
                wsprintf(buf1,p1,"Name Server");
                break;
           case COMP_TYPE_DBMS             :
                wsprintf(buf1,p1,"DBMS Server");
                break;
           case COMP_TYPE_INTERNET_COM     :
                wsprintf(buf1,p1,"Internet Communication");
                break;
           case COMP_TYPE_NET              :
                wsprintf(buf1,p1,"Net Server");
                break;
           case COMP_TYPE_DASVR             :
                wsprintf(buf1,p1,"Data Access Server");
                break;
           case COMP_TYPE_BRIDGE           :
                wsprintf(buf1,p1,"Bridge Server");
                break;
           case COMP_TYPE_STAR             :
                wsprintf(buf1,p1,"Star Server");
                break;
           case COMP_TYPE_SECURITY         :
                wsprintf(buf1,p1,"Security");
                break;
           case COMP_TYPE_LOCKING_SYSTEM   :
                wsprintf(buf1,p1,"Locking System");
                break;
           case COMP_TYPE_LOGGING_SYSTEM   :
                wsprintf(buf1,p1,"Logging System");
                break;
           case COMP_TYPE_SETTINGS         :
                strcpy(buf1,"Do you want to save the modified Preferences ?");
                break;
           case COMP_TYPE_RECOVERY          : // [Add    ] 14-Aug-1998: UK Sotheavut (UK$SO01)
                wsprintf(buf1,p1,"Recovery Server");
                break;
           case COMP_TYPE_GW_ORACLE         :
                wsprintf(buf1,p1,"Oracle Gateway");
                break;
           case COMP_TYPE_GW_DB2UDB         :
                wsprintf(buf1,p1,"DB2 UDB Gateway");
                break;
           case COMP_TYPE_GW_INFORMIX       :
                wsprintf(buf1,p1,"Informix Gateway");
                break;
           case COMP_TYPE_GW_SYBASE         :
                wsprintf(buf1,p1,"Sybase Gateway");
                break;
           case COMP_TYPE_GW_MSSQL          :
                wsprintf(buf1,p1,"MSSQL Gateway");
                break;
           case COMP_TYPE_GW_ODBC           :
                wsprintf(buf1,p1,"ODBC Gateway");
                break;
           default  :
                strcpy(buf1,"Do you want to save your changes?");
                break;
        }
	iret = MessageBox(GetFocus(), buf1, stdtitle,
		               MB_ICONQUESTION | MB_YESNO |MB_TASKMODAL);
	if (iret == IDNO) 
	{
		/* reinitialize protect.dat context */ 
		PMmFree( protect_data );
		(void) PMmInit( &protect_data );
		PMmLowerOn( protect_data );

		/* reinitialize config.dat context */ 
		PMmFree( config_data );
		(void) PMmInit( &config_data );
		PMmLowerOn( config_data );

		CRsetPMcontext( config_data );
#ifdef FUTUR
      =>   /*   exec frs message :ERget( S_ST031B_restoring_settings );*/
#endif
		/* restore protected data file */
		if( PMmLoad( protect_data,
			&backup_protect_file,
			display_error ) != OK )
		{
		      mymessage(ERget(S_ST0315_bad_config_read));

		      VCBFRequestExit();
		      return FALSE;
		}
		if (PMmWrite(protect_data, &protect_file) != 
			OK)
		{
		      mymessage(ERget(S_ST0314_bad_config_write));
		      VCBFRequestExit();
		      return FALSE;
		}

		/* restore configuration data file */
		if( PMmLoad( config_data, &backup_data_file,
			display_error ) != OK )
		{
		      mymessage(ERget(S_ST0315_bad_config_read));
		      VCBFRequestExit();
		      return FALSE;
		}
		if( PMmWrite( config_data, NULL ) != OK )
		{
		      mymessage(ERget(S_ST0314_bad_config_write));
		      VCBFRequestExit();
		      return FALSE;
		}

		/* delete change log */
		(void) LOdelete( &change_log_file );

		/* restore change log from backup */
		if( copy_LO_file( &backup_change_log_file,
			&change_log_file ) != OK )
		{
			(void) LOdelete(
				&backup_change_log_file );
		      mymessage(ERget(S_ST0326_backup_failed));
		      VCBFRequestExit();
		      return FALSE;
		}

		/* delete backup of data file */
		(void) LOdelete( &backup_data_file );

		/* delete backup of protected data file */
		(void) LOdelete( &backup_protect_file );

		/* delete backup of change log */
		(void) LOdelete( &backup_change_log_file );

		/* reset startup count */
		if (VCBFgf_bStartupCountChange && lpcomp && (strcmp(lpcomp->szcount,VCBFgf_oldstartupcount) ))
		{
			BOOL bSuccess = VCBFllValidCount(lpcomp, VCBFgf_oldstartupcount);
			VCBFgf_bStartupCountChange = FALSE;
		}
	}
   }

   if( VCBFgf_backup_mode )
   {
	/* remove backup files */
	LOdelete( &backup_data_file );
	LOdelete( &backup_protect_file );
	LOdelete( &backup_change_log_file );
   }

   return( VCBFgf_changed );
}

/****************************************************************/
/****** cache pane stuff derived from generic form section ******/
/****************************************************************/

static char *VCBFc_pm_host;
static char bufVCBFc_owner[80];
static char *VCBFc_owner, VCBFc_instance[ BIG_ENOUGH ];
//static bool VCBFc_changed = FALSE;
//static char *VCBFc_cache_name = NULL;
//static char *VCBFc_cache_sharing = NULL;
static char VCBFc_real_pm_id[ BIG_ENOUGH ];
//static char *VCBFc_db_list;
static char *VCBFc_def_name ;
static bool VCBFc_backup_mode;

static BOOL bMessage1displayed=FALSE;

static char VCBFll_cache_detail_pm_id[80];;
BOOL VCBFllInitCacheParms(LPCACHELINEINFO lpcacheline)
{
   char new_pm_id[BIG_ENOUGH];
   char cache_type[80];;
   STcopy(VCBFllCache_pm_id, new_pm_id);
   strcpy(cache_type,lpcacheline->szcachename);
   if (STcompare(cache_type, ERget( S_ST0432_dmf_cache_4k )) == 0)
   {
       STcat(new_pm_id, ERx(".p4k"));
   }
   if (STcompare(cache_type, ERget( S_ST0433_dmf_cache_8k )) == 0)
   {
       STcat(new_pm_id, ERx(".p8k"));
   }
   if (STcompare(cache_type, ERget( S_ST0434_dmf_cache_16k )) == 0)
   {
       STcat(new_pm_id, ERx(".p16k"));
   }
   if (STcompare(cache_type, ERget( S_ST0435_dmf_cache_32k )) == 0)
   {
       STcat(new_pm_id, ERx(".p32k"));
   }
   if (STcompare(cache_type, ERget( S_ST0436_dmf_cache_64k )) == 0)
   {
       STcat(new_pm_id, ERx(".p64k"));
   }

   strcpy(VCBFll_cache_detail_pm_id,new_pm_id);

   {
	char *name, *value;
//      char name_val[ BIG_ENOUGH ]; 
//      char bool_name_val[ BIG_ENOUGH ]; /*b69912*/
	char *my_name;
//      char description[ BIG_ENOUGH ];
	char *form_name;
//      char host_field[ 21 ];
//      char def_name_field[ 21 ];
	char temp_buf[ MAX_LOC + 1 ];
	i4 lognum;
//      char    fldname[33];
//	char dbms_second_menuitem[20];
	char *audlog=ERx("audit_log_");
	i4  audlen;

	PM_SCAN_REC state;
	STATUS status;
	i4 independent, dependent, len;
	char *comp_type;
	char expbuf[ BIG_ENOUGH ];
	char *regexp;
//	char derived_table_title[ BIG_ENOUGH ];
	char *p;
	bool c2_form=FALSE;
	bool doing_aud_log=FALSE;

	VCBFc_pm_host = host; /* moved to static */
//        VCBFc_cache_name = NULL;
//        VCBFc_cache_sharing = NULL;

	VCBFc_def_name= VCBFgf_cache_name;
//	STcopy(ERget( S_ST0354_databases_menu ),dbms_second_menuitem);

#ifdef VCBF_FUTURE
      replace it => exec frs message :ERget( S_ST0344_selecting_parameters );
#endif

	/* first component of pm_id identifies component type */
	comp_type = PMmGetElem( config_data, 0, VCBFll_cache_detail_pm_id );

	/*
	** Since the forms system doesn't allow nested calls to the
	** same form, dmf_independent must be used if this function is
	** being called to drive the independent DBMS cache parameters
	** form.
	*/
	if( STcompare( comp_type, ERx( "dmf" ) ) == 0 )
	{
		form_name = ERx( "dmf_independent" );
		/* HACK: skip past "dmf." */
		p = VCBFll_cache_detail_pm_id;
		CMnext( p );
		CMnext( p );
		CMnext( p );
		CMnext( p );
		STprintf( VCBFc_real_pm_id, ERx( "dbms.%s" ), p );
	}
	else if( STcompare( comp_type, ERx( "c2" ) ) == 0 )
	{
		/*
		** C2 form is independent
		*/
		form_name = ERx("c2_independent");
		STcopy( VCBFll_cache_detail_pm_id, VCBFc_real_pm_id );
		c2_form=TRUE;
	}
	else
	{
//		if( STcompare(comp_type, ERx( "dbms" ) ) == 0 )
//		{
//		  if (!ingres_menu)
//		  *dbms_second_menuitem=EOS;
//		}
		form_name = ERx( "generic_independent" );
		STcopy( VCBFll_cache_detail_pm_id, VCBFc_real_pm_id );
	}


	/* set value of my_name */
	if (c2_form)
		my_name = NULL;
	else  if( VCBFc_def_name == NULL )
		my_name = STalloc( ERget( S_ST030F_default_settings ) );
	else
	{
		if( STcompare( VCBFc_def_name, ERx( "*" ) ) == 0 )
			my_name = STalloc(
				ERget( S_ST030F_default_settings ) );
		else
			my_name = STalloc( VCBFc_def_name ); 
	}

	/* extract resource owner name from real_pm_id */
	VCBFc_owner = PMmGetElem( config_data, 0, VCBFc_real_pm_id );
	if (VCBFc_owner) {
	   strcpy(bufVCBFc_owner,VCBFc_owner) ;
	   VCBFc_owner=bufVCBFc_owner;
	}

	len = PMmNumElem( config_data, VCBFc_real_pm_id );
		
	/* compose "instance" */
	if( len == 2 )
	{
		/* instance name is second component */
		name = PMmGetElem( config_data, 1, VCBFc_real_pm_id );
		if( STcompare( name,
			ERget( S_ST030F_default_settings ) ) == 0
		)
			STcopy( ERx( "*" ), VCBFc_instance );
		else
			STcopy( name, VCBFc_instance ); 
		STcat( VCBFc_instance, ERx( "." ) );
	}
	else if( len > 2 )
	{
		char *p;
		/* VCBFc_instance name is all except first */

		for( p = VCBFc_real_pm_id; *p != '.'; CMnext( p ) );
		CMnext( p );
		STcopy( p, VCBFc_instance );
		STcat( VCBFc_instance, ERx( "." ) );
	}
	else
		STcopy( ERx( "" ), VCBFc_instance );

	if (c2_form)
	{
		for(lognum=1;lognum<=MAX_AUDIT_LOGS;lognum++)
		{
			audit_log_list[lognum-1]=NULL;
		}
	}

	if( STcompare( comp_type, ERx( "c2" ) ) == 0 )
		VCBFc_pm_host = ERx( "*" );

	/* check whether this is a DBMS cache definition */
	if( STcompare( comp_type, ERx( "dmf" ) ) == 0 )
	{
		char cache_desc[ BIG_ENOUGH ];

		/* if so, turn off backup option */
		VCBFc_backup_mode = FALSE;

		/* and display cache_desc */
		value = PMmGetElem( config_data, 1, VCBFc_real_pm_id );
		if( STcompare( ERget( S_ST0332_shared ), value ) == 0 ) {
		  /*    exec frs putform(                             */ 
		  /*            cache_desc = :ERget( S_ST0332_shared )*/
#ifdef FUTURE
   was that needed?
#endif
		}
		else
		{
			name = STalloc( ERget( S_ST0333_private ) );
			value = STalloc( PMmGetElem( config_data, 2,
				VCBFc_real_pm_id ) );
			if( STcompare( value, ERx( "*" ) ) == 0 )
			{
				MEfree( value );
				value = STalloc(
					ERget( S_ST030F_default_settings ) );
			}
			STprintf( cache_desc, ERx( "%s ( %s=%s )" ), 
				name, ERget( S_ST0334_owner ), value );
			MEfree( name );
			MEfree( value );
		  /*    exec frs putform(                */ 
		  /*            cache_desc = :cache_desc */
#ifdef FUTURE
   was that needed?
#endif
		}
	}
	else
	{
		/* this is not a DBMS cache definition, so */
		VCBFc_backup_mode = TRUE;

		/* prepare a backup LOCATION for the data file */
		STcopy( ERx( "" ), temp_buf );
		LOfroms( FILENAME, temp_buf, &backup_data_file );
		LOuniq( ERx( "cacbfd1" ), NULL, &backup_data_file );
		LOtos( &backup_data_file, &name );
		STcopy( tempdir, backup_data_buf ); 
		STcat( backup_data_buf, name );
		LOfroms( PATH & FILENAME, backup_data_buf, &backup_data_file );
	
		/* prepare a backup LOCATION for the change log */
		STcopy( ERx( "" ), temp_buf );
		LOfroms( FILENAME, temp_buf, &backup_change_log_file );
		LOuniq( ERx( "cacbfc1" ), NULL, &backup_change_log_file );
		LOtos( &backup_change_log_file, &name );
		STcopy( tempdir, backup_change_log_buf ); 
		STcat( backup_change_log_buf, name );
		LOfroms( PATH & FILENAME, backup_change_log_buf,
			&backup_change_log_file );
	
		/* prepare a backup LOCATION for the protected data file */
		STcopy( ERx( "" ), temp_buf );
		LOfroms( FILENAME, temp_buf, &backup_protect_file );
		LOuniq( ERx( "cacbfp1" ), NULL, &backup_protect_file );
		LOtos( &backup_protect_file, &name );
		STcopy( tempdir, backup_protect_buf ); 
		STcat( backup_protect_buf, name );
		LOfroms( PATH & FILENAME, backup_protect_buf,
			&backup_protect_file );
	
		/* write configuration backup */ 
		if( PMmWrite( config_data, &backup_data_file ) != OK )
		{
			mymessage(ERget(S_ST0326_backup_failed));
		     /* IIUGerr( S_ST0326_backup_failed, 0, 0 ); */
			LOdelete( &backup_data_file );
			if( my_name != NULL )
				MEfree( my_name );
			return( FALSE );
		}

		/* write change log backup */ 
		if( copy_LO_file( &change_log_file, &backup_change_log_file )
			!= OK ) 
		{
			mymessage(ERget(S_ST0326_backup_failed));
		   /*   IIUGerr( S_ST0326_backup_failed, 0, 0 );  */
			(void) LOdelete( &backup_change_log_file );
			return( FALSE );
		}

		/* write protected data backup */ 
		if( PMmWrite( protect_data, &backup_protect_file ) != OK )
		{
			mymessage(ERget(S_ST0326_backup_failed));
		   /*   IIUGerr( S_ST0326_backup_failed, 0, 0 );  */
			LOdelete( &backup_protect_file );
			if( my_name != NULL )
				MEfree( my_name );
			return( FALSE );
		}

	}

	/* prepare expbuf */
	STprintf( expbuf, ERx( "%s.%s.%s.%%" ), SystemCfgPrefix, 
		  VCBFc_pm_host, VCBFc_real_pm_id );

	regexp = PMmExpToRegExp( config_data, expbuf );

	/* initialize independent and dependent resource counters */ 
	independent = dependent = 0;

	/* get the name components for this set of resources */
	len = PMmNumElem( config_data, expbuf );

	VCBFInitGenCacheLines();

	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		char *units,*short_name;

		/* extract parameter name from full resource */
		short_name = PMmGetElem( config_data, len - 1, name );

		/* save dbms cache_name value */
		if( STcompare( short_name, ERx( "cache_name" ) ) == 0 ) {
			strcpy(bufVCBFgf_cache_name,value);
			VCBFgf_cache_name = bufVCBFgf_cache_name;
		}

		/* save dbms cache_sharing value */
		if( STcompare( short_name, ERx( "cache_sharing" ) ) == 0 ) {
			strcpy(bufVCBFgf_cache_sharing,value);
			VCBFgf_cache_sharing = bufVCBFgf_cache_sharing;
		}

		/* skip derived resources */
		if( CRderived( name ) )
		{
			++dependent;
			continue;
		}
		++independent;

		/* get units */
		if( PMmGet( units_data, name, &units ) != OK )
			units = ERx( "" );

		/* use just parameter name from now on */
		name=short_name;

		/* skip db_list and assume it is only a dbms parameter */
		if( STcompare( name, ERx( "database_list" ) ) == 0 )
		{
//                      VCBFc_db_list = value;
			continue;
		}

		if (c2_form)
		{
			audlen=(i4)STlength(audlog);

			if(STbcompare(name, audlen, audlog, audlen, 1)==0)
			{
			    CVal(name+STlength(audlog), &lognum);
			    if(lognum>0 && lognum <=MAX_AUDIT_LOGS)
			    {
				audit_log_list[lognum-1]=value;
			    }
			}
			else
			{
			    VCBFAddGenCacheLine(name, value, units);
			}
		}
		else
		{
		    VCBFAddGenCacheLine(name, value, units);
		}
	}
	/*
	** Now we  have the audit logs load them in order.
	*/
	if (c2_form)
	{
		for(lognum=1; lognum<=MAX_AUDIT_LOGS; lognum++)
		{
			value=audit_log_list[lognum-1];
			if(!value)
				value="";
			VCBFAddAuditLogLine(lognum, value);
		}
	}
#if 0
	/* set derived_table_title */
	if( STcompare( comp_type, ERx( "dbms" ) ) == 0 )
		STcopy( ERget( S_ST036C_derived_dbms ), derived_table_title );
	else if( STcompare( comp_type, ERx( "lock" ) ) == 0 ) {
		if(!ingres_menu)
		  STcopy( ERget( S_ST0371_derived_locking ), 
		  derived_table_title );
	}
	else if( STcompare( comp_type, ERx( "dmf" ) ) == 0 )
		STcopy( ERget( S_ST036D_derived_cache ), derived_table_title );
	else if( STcompare( comp_type, ERx( "gcc" ) ) == 0 )
		STcopy( ERget( S_ST036E_derived_net ), derived_table_title );
	else if( STcompare( comp_type, ERx( "c2" ) ) == 0 )
		STcopy( ERget( S_ST0379_derived_security ), 
			derived_table_title );
	else if( STcompare( comp_type, ERx( "secure" ) ) == 0 )
		STcopy( ERget( S_ST037B_security_param_deriv ), 
			derived_table_title );
	else if( STcompare( comp_type, ERx( "star" ) ) == 0 )
		STcopy( ERget( S_ST036F_derived_star ), derived_table_title );
	else if( STcompare( VCBFc_real_pm_id, ERx( "rcp.log" ) ) == 0 )
		STcopy( ERget( S_ST0370_derived_logging ),
			derived_table_title );
	else if( STcompare( VCBFc_real_pm_id, ERx( "rcp.lock" ) ) == 0 )
		STcopy( ERget( S_ST0371_derived_locking ),
			derived_table_title );
#endif
	if( independent == 0 )
	{
               if (!bMessage1displayed)
		  mymessagewithtitle("According to the current configuration rules,\n"
			  " all parameters for the selected component are derived\n"
			  " from other sources");
               bMessage1displayed=TRUE;
	}

	if( my_name != NULL )
	   MEfree( my_name );
	return TRUE;
   }
}

static BOOL CachePaneOnEditStep2(char * temp_buf,char * name_val,char * oldval, char * newval, BOOL doing_aud_log,int lognum)
{
	/* called ONLY IF THERE WAS A CHANGE*/
	bool avoid_set_resource = FALSE; /* bug #80450 */
	char in_buf[ BIG_ENOUGH ];
	char * value;

	strcpy(in_buf,newval);

	/* just toggle if resource is boolean */
	if( CRtype( CRidxFromPMsym( temp_buf ) ) == CR_BOOLEAN )
	{
		if (stricmp(in_buf,"on"))/* not on =>OFF */
		    STcopy( "OFF", in_buf );
		else  {
		    char parambuf[ BIG_ENOUGH ];

		    if (!stricmp(in_buf,oldval))
			return TRUE; 
		    /*
		    ** bug #80450 - both cache_sharing and dmcm
		    ** cannot be turned on
		    */


		    STcopy( "OFF", in_buf );
#ifndef OIDSK
		    if ( STcompare(name_val,
				ERx("cache_sharing")) == 0 )
		    {
			STprintf( parambuf, ERx("%s.%s.%s."),
				SystemCfgPrefix, VCBFc_pm_host, VCBFc_owner );
			STcat( parambuf, VCBFc_instance );
			STcat( parambuf, ERx("dmcm") );
			if ( PMmGet(config_data, parambuf, &value) == OK
				&& !(ValueIsBoolTrue(value)) )
			{
			    /* 
			    ** dmcm is OFF, thus cache_sharing 
			    ** can be turned ON.
			    */
			    STcopy( ERx( "ON" ), in_buf );
			}
			else
			{
			    avoid_set_resource = TRUE;
			    mymessagewithtitle(ERget( S_ST0444_dmcm_warning ) );
			}
		    }
		    else if ( STcompare(name_val,
			ERx("dmcm")) == 0 )
		    {
			STprintf( parambuf, ERx("%s.%s.%s."),
				SystemCfgPrefix, VCBFc_pm_host, VCBFc_owner );
			STcat( parambuf, VCBFc_instance );
			STcat( parambuf, ERx("cache_sharing") );
			if ( PMmGet(config_data, parambuf, &value) == OK
				&& !(ValueIsBoolTrue(value)) )
			{
			    /*
			    ** cache_sharing is OFF, thus dmcm
			    ** can be turned ON.
			    */
			    STcopy( ERx( "ON" ), in_buf );
			}
			else
			{
			    avoid_set_resource = TRUE;
			    mymessagewithtitle(ERget( S_ST0444_dmcm_warning ) );
 
			}
		    }
		    else
#endif
			STcopy( "ON", in_buf );
		}
	}

	if( !avoid_set_resource)  /* bug #80450, do not call 
				  ** set_resource, since parameter 
				  ** value is not to be changed 
				  */
	{
	    if ( set_resource( temp_buf, in_buf ) )
	    {
		/* change succeeded */ 
		VCBFgf_changed = TRUE;

//              if( STcompare( name_val, ERx( "cache_name" ) ) == 0 )
//                      VCBFc_cache_name = in_buf;

//              if( STcompare( name_val, ERx( "cache_sharing" ) ) == 0 )
//                      VCBFc_cache_sharing = in_buf;
#ifndef OIDSK
		if ((STcompare( VCBFc_real_pm_id, ERx( "rcp.log" ) ) == 0) &&
		    (STcompare( name_val, ERx( "block_size") ) == 0))
			mymessagewithtitle(ERget(S_ST0533_reformat_log));
#endif
		/*
		** For various historical reasons we have 
		** two sets of log names. CBF reads in c2.audit_log_X
		** but SXF reads c2.log.audit_log_X, so make sure
		** they stay in sync
		*/
		if(doing_aud_log==TRUE)
		{
		     STprintf( temp_buf, 
			       ERx( "%s.%s.c2.log.audit_log_%d" ), 
			       SystemCfgPrefix, VCBFc_pm_host,  lognum );
		      (VOID) set_resource( temp_buf, in_buf ); 
		}
		/* 
		** update tablefield. Note this works since  all 
		** tablefields have a column called "value" which 
		** holds the pmvalue
		*/


		/* load config.dat */
		if( PMmLoad( config_data, &config_file,
			display_error ) != OK )
		{
			mymessage(ERget(S_ST0315_bad_config_read));
			VCBFRequestExit();
			return FALSE;
		}

		/*
		** 08-apr-1997 (thaju02)
		** load protect.dat to get latest changes
		*/
		if( PMmLoad( protect_data, &protect_file,
			display_error ) != OK )
		{
			mymessage(ERget(S_ST0320_bad_protect_read));
			VCBFRequestExit();
			return FALSE;
		}
	    }
	}
	strcpy(newval,in_buf);
	return TRUE;
#ifdef FUTURE
   verify that in case of error exit has been done
#endif
}

BOOL VCBFllCachePaneOnEdit( LPGENERICLINEINFO lpgenlineinfo, char * lpnewvalue)
{
     char temp_buf[400];
     char in_buf[200];
     BOOL bResult;
     strcpy(in_buf,lpnewvalue);
     /* compose name of resource to be set */
     STprintf( temp_buf, ERx( "%s.%s.%s." ), SystemCfgPrefix,
	       VCBFc_pm_host, VCBFc_owner );
     STcat( temp_buf, VCBFc_instance );
     STcat( temp_buf, lpgenlineinfo->szname);
     bResult= CachePaneOnEditStep2(temp_buf,lpgenlineinfo->szname,lpgenlineinfo->szvalue,in_buf,FALSE,0);
     if (bResult)
	strcpy(lpgenlineinfo->szvalue,in_buf);
     return bResult;
}

char * VCBFllGet_cachepane_pm_id()       // needed for dependent forms
{
	return VCBFll_cache_detail_pm_id;
}
BOOL VCBFllGet_cachepane_backup_mode()
{
	return VCBFc_backup_mode ;
}

BOOL VCBFllCacheResetGenValue(LPGENERICLINEINFO lpgenlineinfo) 
{
    char temp_buf[200];
    int iret;
    char * value = "";
    STprintf( temp_buf, ERx( "%s.%s.%s." ), 
	  SystemCfgPrefix, VCBFc_pm_host, VCBFc_owner );
    STcat( temp_buf, VCBFc_instance );
    STcat( temp_buf, lpgenlineinfo->szname );

    iret = QuestionMessageWithParam (S_ST0329_reset_prompt,lpgenlineinfo->szname);
    if (iret == IDYES) 
    {
	if( reset_resource( temp_buf, &value, TRUE ) ) {
	    strcpy(lpgenlineinfo->szvalue,value);
	    VCBFgf_changed = TRUE;
	    return TRUE;
	}
    }
    return FALSE;
}

/****************************************************************/
/************** generic dependent form functions*****************/
/****************************************************************/
static char * VCBFll_dep_pm_id;
static char bufVCBFdep_owner[80];
static char *VCBFdep_owner, VCBFdep_instance[ BIG_ENOUGH ];
static char *VCBFdep_pm_host;
static char VCBFdep_expbuf[ BIG_ENOUGH ];
static i4 VCBFdep_len;

BOOL VCBFllInitDependent(BOOL bCacheDep)
{
   char *def_name  =bCacheDep? VCBFc_def_name    :VCBFgf_def_name;
   bool backup_mode=bCacheDep? VCBFgf_backup_mode:VCBFc_backup_mode;
   bool handle_save= FALSE;
   char *name, *value;
//   char in_buf[ BIG_ENOUGH ];
//   char protect_val[ BIG_ENOUGH ];
//   char name_val[ BIG_ENOUGH ];
   char *my_name = def_name;
//   char temp_val[ BIG_ENOUGH ];
   char *protect;
//   char description[ BIG_ENOUGH ];
   char *form_name;
//   char temp_buf[ BIG_ENOUGH ];

   PM_SCAN_REC state;
   STATUS status;
   char *regexp;
   bool no_protect = FALSE;
//   i4  i;
   bool changed = FALSE;
   char *comp_type;
   char *p;
   char real_pm_id[ BIG_ENOUGH ];
   bool c2_form=FALSE;

   VCBFll_dep_pm_id=bCacheDep?    VCBFll_cache_detail_pm_id :VCBFgf_pm_id;
   VCBFdep_pm_host = host;

   /* first component of pm_id identifies component type */
   comp_type = PMmGetElem( config_data, 0, VCBFll_dep_pm_id );

#ifdef FUTURE
   => exec frs message :ERget( S_ST0345_selecting_derived );
#endif

   if( STcompare( comp_type, ERx( "dmf" ) ) == 0 )
   {
	form_name = ERx( "dmf_dependent" );
	/* HACK: skip past "dmf." */
	p = VCBFll_dep_pm_id;
	CMnext( p );
	CMnext( p );
	CMnext( p );
	CMnext( p );
	STprintf( real_pm_id, ERx( "dbms.%s" ), p );
	/*
	** variable page size project: supporting multiple caches
	** append page size to table title for clarification
	*/
//      if (PMmNumElem(config_data, VCBFll_dep_pm_id) == 4)
//      {
//                      if (STcompare(PMmGetElem(config_data, 3, VCBFll_dep_pm_id), 
//                              ERx("p4k")) == 0)
//                      STcat(full_title, ERx(" for 4k"));
//                      if (STcompare(PMmGetElem(config_data, 3, VCBFll_dep_pm_id), 
//                              ERx("p8k")) == 0)
//                      STcat(full_title, ERx(" for 8k"));
//                      if (STcompare(PMmGetElem(config_data, 3, VCBFll_dep_pm_id), 
//                              ERx("p16k")) == 0)
//                      STcat(full_title, ERx(" for 16k"));
//                      if (STcompare(PMmGetElem(config_data, 3, VCBFll_dep_pm_id), 
//                              ERx("p32k")) == 0)
//                      STcat(full_title, ERx(" for 32k"));
//                      if (STcompare(PMmGetElem(config_data, 3, VCBFll_dep_pm_id), 
//                              ERx("p64k")) == 0)
//                      STcat(full_title, ERx(" for 64k"));
//      }
//      else
//              STcat(full_title, ERx(" for 2k"));

   }
   else
   {
	form_name = ERx( "generic_dependent" );
	STcopy( VCBFll_dep_pm_id, real_pm_id );
   }

   /* set value of my_name */
   if( STcompare( comp_type, ERx( "c2" ) ) == 0 )
	c2_form=TRUE;
   else
	c2_form=FALSE;
   if(c2_form)
	my_name=NULL;
   else if( def_name == NULL )
	my_name = STalloc( ERget( S_ST030F_default_settings ) );
   else
   {
	if( STcompare( def_name, ERx( "*" ) ) == 0 )
		my_name = STalloc(
			ERget( S_ST030F_default_settings ) );
	else
		my_name = STalloc( def_name ); 
   }

   /* extract resource owner name from real_pm_id */
   VCBFdep_owner = PMmGetElem( config_data, 0, real_pm_id );
   if (VCBFdep_owner){
      strcpy(bufVCBFdep_owner,VCBFdep_owner);
      VCBFdep_owner=bufVCBFdep_owner;
   }


   VCBFdep_len = PMmNumElem( config_data, real_pm_id );
	
   /* compose "instance" */
   if( VCBFdep_len == 2 )
   {
	/* instance name is second component */
	name = PMmGetElem( config_data, 1, real_pm_id );
	if( STcompare( name,
		ERget( S_ST030F_default_settings ) ) == 0
	)
		STcopy( ERx( "*" ), VCBFdep_instance );
	else
		STcopy( name, VCBFdep_instance ); 
	STcat( VCBFdep_instance, ERx( "." ) );
   }
   else if( VCBFdep_len > 2 )
   {
	char *p;
	/* instance name is all except first */

	for( p = real_pm_id; *p != '.'; CMnext( p ) );
	CMnext( p );
	STcopy( p, VCBFdep_instance );
	STcat( VCBFdep_instance, ERx( "." ) );
   }
   else
	STcopy( ERx( "" ), VCBFdep_instance );


   /*
   ** 08-apr-1997 (thaju02)
   ** load protect.dat to get latest changes.
   */
   PMmFree( protect_data );
   (void) PMmInit( &protect_data );
   PMmLowerOn( protect_data );
   if (PMmLoad(protect_data, &protect_file, display_error) != OK) {
       mymessage(ERget(S_ST0320_bad_protect_read));
       VCBFRequestExit();
       return FALSE;
   }
   if (c2_form)
	VCBFdep_pm_host = ERx( "*" );


   /* display cache_desc if this is a cache definition */
   if( STcompare( comp_type, ERx( "dmf" ) ) == 0 )
   {
	char cache_desc[ BIG_ENOUGH ];

	value = PMmGetElem( config_data, 1, real_pm_id );
	if( STcompare( ERget( S_ST0332_shared ), value ) == 0 ) {
#ifdef FUTURE
	 =>     exec frs putform(
	 =>             cache_desc = :ERget( S_ST0332_shared )
	 =>     );
#endif
	}
	else
	{
		name = STalloc( ERget( S_ST0333_private ) );
		value = STalloc( PMmGetElem( config_data, 2,
			real_pm_id ) );
		if( STcompare( value, ERx( "*" ) ) == 0 )
			value = STalloc(
				ERget( S_ST030F_default_settings ) );
		STprintf( cache_desc, ERx( "%s (%s=%s)" ), 
			name, ERget( S_ST0334_owner ), value );
		MEfree( name );
		MEfree( value );
#ifdef FUTURE
	 =>     exec frs putform(
	 =>             cache_desc = :cache_desc
	 =>     );
#endif
	}
   }

   /* construct expbuf contents */
   STprintf( VCBFdep_expbuf, ERx( "%s.%s.%s.%%" ), SystemCfgPrefix,
	  VCBFdep_pm_host, real_pm_id );

   VCBFdep_len = PMmNumElem( config_data, VCBFdep_expbuf );

   regexp = PMmExpToRegExp( config_data, VCBFdep_expbuf );

   VCBFInitDerivLines();
   for( status = PMmScan( config_data, regexp, &state, NULL,
	&name, &value ); status == OK; status =
	PMmScan( config_data, NULL, &state, NULL, &name,
	&value ) )
   {
	char *units;

	PM_SCAN_REC state;
	char *full_name;

	/* skip non-derived resources */
	if( ! CRderived( name ) )
		continue;

	/* get units */
	if( PMmGet( units_data, name, &units ) != OK )
		units = ERx( "" );

	/* save full resource name */
	full_name = name;

	/* extract parameter from full resource name */
	name = PMmGetElem( config_data, VCBFdep_len - 1, name );

	/* get value of column "protected" */
	regexp = PMmExpToRegExp( protect_data, full_name );
	if( PMmScan( protect_data, regexp, &state, NULL,
		NULL, &value ) == OK
	)
		protect = dupstr1(ERget( S_ST0330_yes ));
	else
		protect = dupstr1(ERget( S_ST0331_no ));

	VCBFAddDerivLine(name, value, units, protect);
   }
//   if( my_name != NULL )      // dangerous if passed as a parm!
//      MEfree( my_name );      
   return TRUE;
}

/******=> reload after change ****/
BOOL VCBBllOnEditDependent(LPDERIVEDLINEINFO lpderivinfo, char * newval)
{
   bool protected1 = FALSE;
   char *protect;
   char *name, *value;
   char in_buf[400];
   char temp_buf[ BIG_ENOUGH ];
   char *regexp;
   STATUS status;
   PM_SCAN_REC state;
   bool no_protect = FALSE; // unmodified in original code
   
   strcpy(in_buf,newval);

   /* save dbms cache_name value */
   if( STcompare( lpderivinfo->szname, ERx( "cache_name" ) ) == 0 ) {
      if (strcmp(lpderivinfo->szvalue,newval)!=0) {
          strcpy(bufVCBFgf_cache_name,newval);
          VCBFgf_cache_name = bufVCBFgf_cache_name;
      }
   }

   /* save dbms cache_sharing value */
   if( STcompare( lpderivinfo->szname, ERx( "cache_sharing" ) ) == 0 ) {
      if (strcmp(lpderivinfo->szvalue,newval)!=0) {
          strcpy(bufVCBFgf_cache_sharing,newval);
          VCBFgf_cache_sharing = bufVCBFgf_cache_sharing;
      }
   }

   STprintf( temp_buf, ERx( "%s.%s.%s." ), SystemCfgPrefix,
     VCBFdep_pm_host, VCBFdep_owner );
   STcat( temp_buf, VCBFdep_instance );
   STcat( temp_buf, lpderivinfo->szname);
   if (!strcmp(lpderivinfo->szvalue,newval))
      return TRUE;
   if (CRtype( CRidxFromPMsym( temp_buf ) ) == CR_BOOLEAN )
   {
       if ( strnicmp(in_buf,"ON",2))
	   STcopy( ERx( "OFF"), in_buf );
       else
	   STcopy( ERx("ON"), in_buf );
   }

   /* remove from protected resource to change */
   if( PMmDelete( protect_data, temp_buf ) == OK )
	protected1 = TRUE;

   if( set_resource( temp_buf, in_buf ) )
   {
	/* change succeeded */ 
	VCBFgf_changed = TRUE;
	strcpy(lpderivinfo->szvalue,in_buf);
	if( protected1 )
	{
		/* reinsert value into protect data set */
		(void) PMmInsert( protect_data, temp_buf,
			in_buf );
	}

	/* load config.dat */
	if( PMmLoad( config_data, &config_file,
		display_error ) != OK )
	{
	   mymessage( ERget(S_ST0315_bad_config_read));
	      VCBFRequestExit();
	      return FALSE;
	}

	/*
	** 08-apr-1997 (thaju02)
	** load protect.dat to get latest changes.
	*/
	if( PMmLoad( protect_data, &protect_file,
		display_error ) != OK )
	{
	   mymessage(ERget(S_ST0320_bad_protect_read));
	      VCBFRequestExit();
	      return FALSE;
	}

	regexp = PMmExpToRegExp( config_data, VCBFdep_expbuf );

	/* reload tablefield - contents may have changed */
	   VCBFInitDerivLines();
	for( status = PMmScan( config_data, regexp,
		&state, NULL, &name, &value ); status == OK;
		status = PMmScan( config_data, NULL, &state,
		NULL, &name, &value ) )
	{
		char *units;

		PM_SCAN_REC state;
		char *full_name;

		/* skip non-derived resources */
		if( ! CRderived( name ) )
			continue;

		/* get units */
		if( PMmGet( units_data, name, &units ) != OK )
			units = ERx( "" );

		/* save full resource name */
		full_name = name;
   
		/* extract parameter from full resource name */
		name = PMmGetElem( config_data, VCBFdep_len - 1,
			name );

		/* get value of column "protected1" */
		if( no_protect )
			protect = ERget( S_ST0331_no );
		else
		{
			regexp = PMmExpToRegExp( protect_data,
				full_name );
			if( PMmScan( protect_data, regexp,
				&state, NULL, NULL, &value )
				== OK
			)
				protect =
					dupstr1(ERget( S_ST0330_yes ));
			else
				protect =
					dupstr1(ERget( S_ST0331_no ));
		}

		   VCBFAddDerivLine(name, value, units, protect);
	}
   }
   else
	(void) PMmInsert( protect_data, temp_buf, in_buf );
   return TRUE;
}

BOOL VCBBllOnDependentProtectChange(LPDERIVEDLINEINFO lpderivinfo, char * newval)
{
   char temp_buf[ BIG_ENOUGH ];
   /* 
   ** 08-apr-1997 (thaju02)
   ** load protect.dat to get latest changes.
      PMmFree( protect_data );
      (void) PMmInit( &protect_data );
      PMmLowerOn( protect_data );
   if (PMmLoad(protect_data, &protect_file, display_error) != OK)
       IIUGerr( S_ST0320_bad_protect_read,UG_ERR_FATAL, 0 );
   */


   STprintf( temp_buf, ERx( "%s.%s.%s." ), SystemCfgPrefix,
	  VCBFdep_pm_host, VCBFdep_owner );
   STcat( temp_buf, VCBFdep_instance );
   STcat( temp_buf, lpderivinfo->szname);

   if (!strcmp(lpderivinfo->szprotected,newval))
      return TRUE;
   if( STequal( lpderivinfo->szprotected, ERget( S_ST0330_yes ) ) != 0 )
   {
	(void) PMmDelete( protect_data, temp_buf );
	strcpy(lpderivinfo->szprotected,ERget( S_ST0331_no ));
   }
   else
   {
	(void) PMmInsert( protect_data, temp_buf, lpderivinfo->szvalue );
	strcpy(lpderivinfo->szprotected,ERget( S_ST0330_yes ));
   }

   VCBFgf_changed = TRUE;

   if( PMmWrite( protect_data, &protect_file ) != OK ) {
       mymessage(ERget(S_ST0314_bad_config_write));
       VCBFRequestExit();
       return FALSE;
   }
   return TRUE;
}

/******=> reload after change ****/
BOOL VCBFllDepOnRecalc(LPDERIVEDLINEINFO lpderivinfo)
{
   int iret;
   char temp_buf[ BIG_ENOUGH ];
   char *name, *value="";
   char *regexp;
   char *protect;
   PM_SCAN_REC state;
   bool no_protect = FALSE; // unmodified in original code
   STATUS status;
   if( STequal( lpderivinfo->szprotected, ERget( S_ST0330_yes ) ) != 0 )
   {
	   mymessagewithtitle(ERget( S_ST032B_resource_protected ));
	   return FALSE;
   }
   iret = QuestionMessageWithParam (S_ST032A_recalculate_prompt,lpderivinfo->szname);
   if (iret == IDNO) 
      return FALSE;

   STprintf( temp_buf, ERx( "%s.%s.%s." ), SystemCfgPrefix,
	  VCBFdep_pm_host, VCBFdep_owner );
   STcat( temp_buf, VCBFdep_instance );
   STcat( temp_buf, lpderivinfo->szname);

   if( reset_resource( temp_buf, &value, TRUE ) )
   {
	VCBFgf_changed = TRUE;

	regexp = PMmExpToRegExp( config_data, VCBFdep_expbuf );

	/* reload tablefield - contents may have changed */
	   VCBFInitDerivLines();
	for( status = PMmScan( config_data, regexp,
		&state, NULL, &name, &value ); status == OK;
		status = PMmScan( config_data, NULL, &state,
		NULL, &name, &value ) )
	{
	    char *units;

	    PM_SCAN_REC state;
	    char *full_name;

	    /* skip non-derived resources */
	    if( ! CRderived( name ) )
		continue;

	    /* get units */
	    if( PMmGet( units_data, name, &units ) != OK )
		units = ERx( "" );

	    /* save full resource name */
	    full_name = name;
   
	    /* extract parameter from full resource name */
	    name = PMmGetElem( config_data, VCBFdep_len - 1,
		name );

	    /* get value of column "protected" */
	    if( no_protect )
		protect = ERget( S_ST0331_no );
	    else
	    {
		regexp = PMmExpToRegExp( protect_data,
			full_name );
		if( PMmScan( protect_data, regexp,
			&state, NULL, NULL, &value )
			== OK
		)
			protect =
				dupstr1(ERget( S_ST0330_yes ));
		else
			protect =
				dupstr1(ERget( S_ST0331_no ));
	    }
	    VCBFAddDerivLine(name, value, units, protect);
	}
        return TRUE;
   }
   return FALSE;
}


///////////////// PS FUNCTIONS ////////////////////

# define MAX_COL        78

char *VCBFllLoadHist ( )
{
    LOINFORMATION loinf;
    i4 flag;
    UINT uiresult;
    long iSize;
    char* szBufferName;
    HFILE hf;
    
    if ( &change_log_file == NULL)
	return NULL;

    hf = _lopen (change_log_file.path, OF_READ);
    if (hf == HFILE_ERROR)
	return NULL;

    // Get size of file
    flag = LO_I_SIZE;
    if (LOinfo(&change_log_file,&flag,&loinf) != OK)
	return NULL;
    iSize = (long)(loinf.li_size+1);

    // Allocate buffer
    szBufferName = (char * ) malloc((size_t)iSize);
    if (szBufferName == NULL)
	return NULL;
    memset (szBufferName,'\0',(size_t)iSize);

    // read File
    uiresult =_lread(hf, szBufferName, (size_t)(iSize-1));
    if (uiresult==HFILE_ERROR)  {
	free(szBufferName);
	szBufferName = NULL;
	//return szBufferName;
    }

    hf =_lclose(hf);
    if (hf == HFILE_ERROR)  {
	free(szBufferName);
	szBufferName = NULL;
	return szBufferName;
    }
    return szBufferName;
}


BOOL VCBFllFreeString( char* szFreeBufName)
{
    free(szFreeBufName);
    szFreeBufName = NULL;
    return TRUE;
}


char *VCBFll_dblist_init()
{
#ifdef FUTURE
# ifdef VMS
	/* 
	** on VMS, convert non-whitespace to whitespace.
	** nick> this is only here now to convert any
	** 'old' parenthesised entries to the current
	** format ; it can be removed at some stage.
	*/
	for (p = db_list_copy; *p != EOS; p++)
	{
	    if (*p == '(' || *p == ',' || *p == ')')
	    {
		*p = ' ';
	  ??    changed = TRUE;
	    }
		}
# endif /* VMS    */
# endif /* FUTURE */
	return  VCBFgf_db_list;
}

BOOL VCBFllOnDBListExit(char * db_list_buf,BOOL changed)
{
     char resource[ BIG_ENOUGH ];
     bool init = TRUE;

     if( changed )
     {
	/* set the new db_list resource */
	STprintf( resource, ERx( "%s.%s.dbms.%s.database_list"),
		  SystemCfgPrefix, host, VCBFgf_def_name );
	PMmDelete( config_data, resource );
	PMmInsert( config_data, resource, db_list_buf );

	/* write config.dat */
	if( PMmWrite( config_data, &config_file ) != OK )
	{
		mymessage(ERget(S_ST0314_bad_config_write));
		VCBFRequestExit();
		return FALSE;
	}

	/*
	** duplicate and return the modified db_list -
	** this will cause a minor memory leak.
	*/

	strcpy(bufVCBFgf_db_list,db_list_buf);
	VCBFgf_db_list=bufVCBFgf_db_list;
	VCBFgf_changed = TRUE;
     }
     return TRUE;
}

BOOL VCBFll_netprots_init(protocol_type nType)
{
	char *name, *value;
	char expbuf[ BIG_ENOUGH ];
	char  *protocol_status;

	STATUS status;
	PM_SCAN_REC state;
	char *regexp;
	char request[ BIG_ENOUGH ]; 

	/* construct expbuf for gcc port id scan */
	switch (nType)
	{
	case NET_REGISTRY:
		STprintf( expbuf, ERx( "%s.%s.%s.%%.port" ), SystemCfgPrefix, host, ERx("registry") );
		break;
	case NET_PROTOCOL:
		STprintf( expbuf, ERx( "%s.%s.gcc.%s.%%.port" ), SystemCfgPrefix, host, VCBFgf_def_name );
		break;
	default:
		STprintf( expbuf, ERx( "%s.%s.gcd.%s.%%.port" ), SystemCfgPrefix, host, VCBFgf_def_name );
		break;
	}

	regexp = PMmExpToRegExp( config_data, expbuf );

	VCBFInitNetProtLines();
	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		char *protocol;

		/* extract protocol name */
		protocol = PMmGetElem( config_data, (nType==NET_REGISTRY)? 3 : 4, name ); 

		/* get status of scanned protocol */ 
		switch (nType)
		{
		case NET_REGISTRY:
			STprintf( request, ERx( "%s.%s.registry.%s.status" ), SystemCfgPrefix, host, protocol);
			break;
		case NET_PROTOCOL:
			STprintf( request, ERx( "%s.%s.gcc.%s.%s.status" ), SystemCfgPrefix, host, VCBFgf_def_name, protocol );
			break;
		default:
			STprintf( request, ERx( "%s.%s.gcd.%s.%s.status" ), SystemCfgPrefix, host, VCBFgf_def_name, protocol );
			break;
		}
		if( PMmGet( config_data, request, &protocol_status ) != OK)
			continue;

		VCBFAddNetProtLine(protocol, protocol_status, value);
	}
	return TRUE;
}

BOOL VCBFll_netprots_OnEditStatus(LPNETPROTLINEINFO lpnetprotline,char * in_buf, protocol_type nType)
{
	char request[ BIG_ENOUGH ]; 

	/* compose resource request for insertion */
	switch (nType)
	{
	case NET_REGISTRY:
		STprintf( request, ERx( "%s.%s.registry.%s.status" ), SystemCfgPrefix, host, lpnetprotline->szprotocol );
		break;
	case NET_PROTOCOL:
		STprintf( request, ERx( "%s.%s.gcc.%s.%s.status" ), SystemCfgPrefix, host, VCBFgf_def_name, lpnetprotline->szprotocol );
		break;
	default:
		STprintf( request, ERx( "%s.%s.gcd.%s.%s.status" ), SystemCfgPrefix, host, VCBFgf_def_name, lpnetprotline->szprotocol );
		break;
	}

	if( set_resource( request, in_buf ) )   {
		strcpy(lpnetprotline->szstatus,in_buf);
		VCBFgf_changed = TRUE;
		return TRUE;
	}
	return FALSE;
}


BOOL VCBFll_netprots_OnEditListen(LPNETPROTLINEINFO lpnetprotline,char * in_buf, protocol_type nType)
{
	char request[ BIG_ENOUGH ]; 

	/* compose resource request for insertion */
	switch (nType)
	{
	case NET_REGISTRY:
		STprintf( request, ERx( "%s.%s.registry.%s.port" ), SystemCfgPrefix, host, lpnetprotline->szprotocol );
		break;
	case NET_PROTOCOL:
		STprintf( request, ERx( "%s.%s.gcc.%s.%s.port" ),  SystemCfgPrefix, host, VCBFgf_def_name, lpnetprotline->szprotocol );
		break;
	default:
		STprintf( request, ERx( "%s.%s.gcd.%s.%s.port" ),  SystemCfgPrefix, host, VCBFgf_def_name, lpnetprotline->szprotocol );
		break;
	}

	if( set_resource( request, in_buf ) )
	{
		strcpy(lpnetprotline->szlisten,in_buf);
		VCBFgf_changed = TRUE;
		return TRUE;
	}
	return FALSE;
}

static  char VCBFtx_state[ 100 ];
static  bool VCBFtx_read_only = FALSE;
static  bool VCBFtx_transactions = FALSE;
static  u_i2 VCBFtx_log_status = 0;
static  char *VCBFtx_rcpstat_online;
// static  char *VCBFtx_dir1, *VCBFtx_dir2, *VCBFtx_file1, *VCBFtx_file2;
static  char *VCBFtx_dir1[LG_MAX_FILE + 1], *VCBFtx_dir2[LG_MAX_FILE + 1];
static  char *VCBFtx_file1[LG_MAX_FILE + 1], *VCBFtx_file2[LG_MAX_FILE + 1];
	
//static  LOCATION VCBFtx_loc1;
static  LOCATION VCBFtx_loc1[LG_MAX_FILE + 1];
//static  LOCATION VCBFtx_loc2;
static  LOCATION VCBFtx_loc2[LG_MAX_FILE + 1];

#ifdef __cplusplus
extern "C"        /* because these are callbacks */
{
#endif
static void
init_graph( bool create, i4 size )
{
}
static void
update_graph( void )
{ // MessageBeep(MB_ICONEXCLAMATION);
}
static void
display_message( char *text )
{ if (text)
    MessageBox(NULL, text, "Configuration Manager", MB_OK);
}
#ifdef __cplusplus
}
#endif

static char loc1_buf[ LG_MAX_FILE + 1][ MAX_LOC + 1 ];
static char loc2_buf[ LG_MAX_FILE + 1][ MAX_LOC + 1 ];

static char temporary[ LG_MAX_FILE + 1][ MAX_LOC + 1 ];
static void
format_log_name( char *outbuf, char *basename, i4 partno, char *fileext )
{
	i4	choppos;
	char	svch = '\0';

	choppos = DI_FILENAME_MAX - ( STlength(basename) + 4 );

	if ( choppos < CX_MAX_NODE_NAME_LEN+1 )
	{
	    svch = *(fileext + choppos);
	    *(fileext + choppos) = '\0';
	}

	STprintf( outbuf, "%s.l%02d%s", basename, partno, fileext );

	if ( '\0' != svch )
	{
	    *(fileext + choppos) = svch;
	}
}

BOOL VCBFll_tx_init(LPCOMPONENTINFO lpcompinfo,LPTRANSACTINFO lptransactinfo)
{
	char *log_sym=lpcompinfo->szname;
	char *tmp1, *tmp2;
	char size1[ 25 ], size2[ 25 ];
	char enabled1[ 25 ], enabled2[ 25 ];
	char raw1[ 25 ], raw2[ 25 ];
	i4 LOinfo_flag;
	char tmp_buf[ BIG_ENOUGH ];
    u_i2 loop = 0;
    char configStr[ MAX_LOC ];		/* config str to look up */

#ifdef UNIX
	char cmd[ BIG_ENOUGH ];
#endif
	LOINFORMATION loc_info;
	bool changed = FALSE;
# ifdef VMS
	char *clusterIDStr = NULL;              /* cluster ID string */
	char configStr[ MAX_LOC ];              /* config str to look up */
	i4  clusterID;                         /* cluster ID */
	bool clustered = FALSE;                 /* clustered? */
	VCBFtx_read_only = FALSE;
	VCBFtx_transactions = FALSE;
	VCBFtx_log_status = 0;
	//VCBFtx_rcpstat_online = ERx( "rcpstat -online -silent" );
/*# else  VMS 

# ifdef NT_GENERIC 
	VCBFtx_rcpstat_online = ERx( "rcpstat -online -silent >NUL" );
# else
	VCBFtx_rcpstat_online = ERx( "rcpstat -online -silent >/dev/null" );
# endif /*NT_GENERIC */

# endif /* VMS */

#ifdef FUTURE
	=> exec frs message :ERget( S_ST038A_examining_logs ); 
#endif

# ifdef UNIX
	if( OK == do_csinstall())
	{
#ifdef FUTURE
	=>      exec frs message :ERget( S_ST038A_examining_logs ); 
#endif
	}
# endif /* UNIX */

	/*
	** ### CHECK FOR ACTIVE LOCAL OR REMOTE CLUSTER SERVER PROCESS.
	*/
  	VCBFtx_read_only = FALSE;
    VCBFtx_transactions = FALSE;

	if( OK == call_rcpstat( ERx( "-online -silent" ) ))
	{
#ifdef OIDSK   // parameter added with 2.0
		strcpy( tmp_buf, ERget( S_ST039A_log_ops_disabled ));
#else
		STprintf( tmp_buf, ERget( S_ST039A_log_ops_disabled ),
			  SystemProductName );
#endif
		mymessagewithtitle(tmp_buf);
		VCBFtx_read_only = TRUE;
		STcopy( ERget( S_ST0424_on_line ), VCBFtx_state ); 
	}
	else if( OK == call_rcpstat( ERx( "-transactions -silent" ) ))
	{
		STcopy( ERget( S_ST0426_off_line_transactions ), VCBFtx_state ); 
		VCBFtx_transactions = TRUE;
	}
	else
		STcopy( ERget( S_ST0425_off_line ), VCBFtx_state ); 

#ifdef FUTURE
#ifndef UNIX
	/* make 'raw' column in both tablefields invisible unless UNIX */
       =>       exec frs set_frs column transaction_logs primary_log (
       =>               invisible( raw ) = 1
       =>       );
       =>
       =>       exec frs set_frs column transaction_logs secondary_log (
       =>               invisible( raw ) = 1
       =>       );

# endif /* UNIX */
       =>       exec frs set_frs frs ( timeout = 30 );
#endif

		/*
		** if this is a clustered installation, we need to
		** append the host name to the log file...
		*/
	   	fileext[0] = '\0';
		if ( CXcluster_configured() )
		{
		    CXdecorate_file_name( fileext, NULL );
		}	
	   // Get primary log file name
        STprintf( configStr, "%s.%s.rcp.log.log_file_name",	SystemCfgPrefix, host );

		if( OK == PMmGet( config_data, configStr, &tmp2))
		{ tmp2 = STalloc( tmp2 );
          sprintf(lptransactinfo->szConfigDatLogFileName1, tmp2);
        }
		else
		{ tmp2 = STalloc(ERx("INGRES_LOG"));
          sprintf(lptransactinfo->szConfigDatLogFileName1, "INGRES_LOG");
        }

        // Get primary log file size
        char *value;
        i4 logsize;
        STprintf( configStr, "%s.%s.rcp.file.kbytes", SystemCfgPrefix, host );
        if( OK == PMmGet( config_data, configStr, &value ))
        { CVal( value, &logsize );
          lptransactinfo->iConfigDatLogFileSize1 = logsize / 1024L;
        }
        else
          lptransactinfo->iConfigDatLogFileSize1 = 16;
     
     
# ifdef VMS
		/* 
		** if this is a clustered installation, we need to
		** append the host name to the log file...
		*/
		STprintf( configStr, "%s.%s.config.cluster.id", 
			  SystemCfgPrefix, host );
		PMmSetDefault( config_data, 1, host );
		if( OK == PMmGet( config_data, configStr, &clusterIDStr ) ) {
		    /*
		    ** clusterID will either not exist, which means
		    ** we're not clustered, or will have one of two 
		    ** values:  a node number, or the word 'false' 
		    ** (which is functionally equivalent to it not existing).
		    ** We only want to fixup the log file name iff we're 
		    ** clustered.  We also check to make sure CVan can convert
		    ** clusterIDStr into an honest nat.  If not, we must have 
		    ** a bogus clusterIDStr.
		    */
		    if ( 0 != STcompare( clusterIDStr, "false" ) 
			&& 0 != STcompare( clusterIDStr, "FALSE" ) ) {
			if ( OK == CVan( clusterIDStr, &clusterID ) ) {
			    clustered = TRUE;
			    if ( NULL != tmp2 && EOS != *tmp2 )
				STcat( tmp2, host );
			}
		    }
		}
# else /* VMS */

        lptransactinfo->iNumberOfLogFiles1=0;
		// Get primary transaction log information
        for  (loop = 0; loop < LG_MAX_FILE; loop++)
		{
            STprintf( configStr, "%s.%s.rcp.log.log_file_%d",
			SystemCfgPrefix, node, loop + 1);
		    PMmSetDefault( config_data, 1, node );
		    if( OK == PMmGet( config_data, configStr, &tmp1))
               tmp1 = STalloc( tmp1 );
		    else
               tmp1 = NULL;

            if (tmp1)
            { lptransactinfo->iNumberOfLogFiles1++;
              format_log_name(lptransactinfo->szLogFileNames1[loop], tmp2, loop+1, fileext);
              strcpy(lptransactinfo->szRootPaths1[loop], tmp1);

              STcopy( tmp1, loc1_buf[loop] );
			  LOfroms( PATH, loc1_buf[loop], &VCBFtx_loc1[loop] );
              
              LOfaddpath( &VCBFtx_loc1[loop], SystemLocationSubdirectory, &VCBFtx_loc1[loop] );
			  LOfaddpath( &VCBFtx_loc1[loop], ERx( "log" ), &VCBFtx_loc1[loop] );
			  LOtos( &VCBFtx_loc1[loop], &VCBFtx_dir1[loop] );
			  VCBFtx_dir1[loop] = STalloc( VCBFtx_dir1[loop] );
              format_log_name(tmp_buf, tmp2, loop+1, fileext);
              VCBFtx_file1[loop] = STalloc(tmp_buf);
			  LOfstfile(VCBFtx_file1[loop], &VCBFtx_loc1[loop] );
            }
            else
			{ strcpy(lptransactinfo->szRootPaths1[loop], "");
              sprintf(lptransactinfo->szLogFileNames1[loop], "");
            }
           
            MEfree( tmp1 );


        } // for...        

# endif /* VMS */

  /// Make sure all are gone or all exist - otherwise *error*
  //  for  (loop = 0; loop < LG_MAX_FILE; loop++)
  //	if (GetFileAttributes(lptransactinfo->szRootPaths1[loop]) == 0xffffffff)
  //       strcpy(lptransactinfo->szRootPaths1[loop], "");

        if( *lptransactinfo->szRootPaths1[0] != EOS && LOexist( &VCBFtx_loc1[0] ) == OK )
		{
			/* primary transaction log exists */

			LOinfo_flag = LO_I_SIZE;
			if (LOinfo(&VCBFtx_loc1[0], &LOinfo_flag, &loc_info) != OK || loc_info.li_size == 0) 
			{
				char *value;
				bool havevalue = FALSE;

				/* use ii.$.rcp.file.size */
				PMmSetDefault( config_data, 1, host );
				STprintf( tmp_buf, "%s.$.rcp.file.kbytes", SystemCfgPrefix );
				if ( PMmGet( config_data, tmp_buf, &value ) == OK )
				{
#ifdef NT_GENERIC
				    long ivalue;
#else
				    int ivalue;
#endif /* NT_GENERIC */

				    havevalue = TRUE;
				    CVal( value, &ivalue );
				    loc_info.li_size = ivalue;
				    STprintf( size1, ERx( "%ld" ), loc_info.li_size / 1024L );
				}
				
                if ( havevalue == FALSE )
				{
				    STprintf( tmp_buf, "%s.$.rcp.file.size", SystemCfgPrefix );
				    if( PMmGet( config_data, tmp_buf, &value ) == OK )
				    {
#ifdef NT_GENERIC
					long ivalue;
#else
					int ivalue;
#endif /* NT_GENERIC */

					havevalue = TRUE;
					CVal( value, &ivalue );
					loc_info.li_size = ivalue;
					STprintf( size1, ERx( "%ld" ),
						  loc_info.li_size / 1048576L );
				    }
				}
				
                if ( havevalue == FALSE )
				{
					mymessagewithtitle(ERget(S_ST038B_no_primary_size ));
					STprintf( size1, ERx( "???????" ) );
				}
				
                PMmSetDefault( config_data, 1, NULL );
			}
			else
			{
				STprintf( size1, ERx( "%ld" ), loc_info.li_size / 1048576L );
			}

			/* determine whether primary log is enabled */
			if( OK == call_rcpstat( ERx( "-enable -silent" ) ))
			{
				STcopy( ERget( S_ST0330_yes ), enabled1 );
				VCBFtx_log_status |= PRIMARY_LOG_ENABLED;
			}
			else
				STcopy( ERget( S_ST0331_no ), enabled1 );
	

			STcopy( ERget( S_ST0331_no ), raw1 );   
# ifdef UNIX
			/* determine whether primary log is raw */
			STprintf( cmd, ERx( "iichkraw %s" ), lptransactinfo->szRootPaths1[0] );
			if( PCcmdline( (LOCATION *) NULL, cmd, PC_WAIT,
				(LOCATION *) NULL,
				&cl_err ) == OK )
			{
				STcopy( ERget( S_ST0330_yes ), raw1 );  
			}
# endif /* UNIX */
		}
		else
		{
			*size1 = EOS;
			*enabled1 = EOS;
			*raw1 = EOS;
		}
	
		OFFSET_TYPE lsize1 = 0;
		OFFSET_TYPE tmp_file_size1 = 0;
		lptransactinfo->bLogFilesizesEqual1=TRUE;
		for  (loop = 0; loop < (lptransactinfo->iNumberOfLogFiles1)&&(*lptransactinfo->szRootPaths1[loop] != EOS); loop++)
		{
			if(LOexist( &VCBFtx_loc1[loop] ) == OK )
			{
				/*Primary Transaction log exists*/
				LOinfo_flag = LO_I_SIZE;
				if (LOinfo(&VCBFtx_loc1[loop], &LOinfo_flag, &loc_info) == OK || loc_info.li_size != 0) 
				{
					if(tmp_file_size1 == 0)
						tmp_file_size1 = loc_info.li_size;
					else if(loc_info.li_size != tmp_file_size1)
					{
						mymessagewithtitle(ERget(S_ST038B_no_primary_size ));
						lptransactinfo->bLogFilesizesEqual1 = FALSE;
						break;
					}
					lsize1 += loc_info.li_size;
					STprintf( size1, ERx( "%ld" ),
						(i4)(lsize1 / 1048576L ));
				}
			}
			else
			{
				//mymessagewithtitle(ERget(S_ST038B_no_primary_size ));
				lptransactinfo->bLogFilesizesEqual1 = FALSE;
				break;
			}

		} //end of 'for' loop. which runs for all primary log-files

		strcpy(lptransactinfo->szsize1   , size1);
		strcpy(lptransactinfo->szenabled1, enabled1);
		strcpy(lptransactinfo->szraw1    , raw1);
       
        MEfree( tmp2 );

        /* get secondary transaction log information */

		// Get dual log file name
        STprintf( configStr, "%s.%s.rcp.log.dual_log_name",	SystemCfgPrefix, node );

		if( OK == PMmGet( config_data, configStr, &tmp2))
		{ tmp2 = STalloc( tmp2 );
          sprintf(lptransactinfo->szConfigDatLogFileName2, tmp2);
        }
		else
		{ tmp2 = STalloc(ERx("DUAL_LOG"));
          sprintf(lptransactinfo->szConfigDatLogFileName2, "DUAL_LOG");
        }

        // Get dual log file size
        STprintf( configStr, "%s.%s.rcp.file.kbytes", SystemCfgPrefix, host );
        if( OK == PMmGet( config_data, configStr, &value ))
        { CVal( value, &logsize );
          lptransactinfo->iConfigDatLogFileSize2 = logsize / 1024L;
        }
        else
          lptransactinfo->iConfigDatLogFileSize2 = 16;

# ifdef VMS
		/* 
		** If this is a clustered VMS installation, we need to
		** append the host name to the secondary log file name.
		*/
		if ( clustered ) {
		    if ( NULL != tmp2 && EOS != *tmp2 )
			STcat( tmp2, host );
		}
# endif /* VMS */

        lptransactinfo->iNumberOfLogFiles2 = 0;

		// Get secondary transaction log information
        for  (loop = 0; loop < LG_MAX_FILE; loop++)
		{
            STprintf( configStr, "%s.%s.rcp.log.dual_log_%d",
			SystemCfgPrefix, node, loop + 1);
		    PMmSetDefault( config_data, 1, host );
		    if( OK == PMmGet( config_data, configStr, &tmp1))
               tmp1 = STalloc( tmp1 );
		    else
               tmp1 = NULL;

            if (tmp1)
            { lptransactinfo->iNumberOfLogFiles2++;

              format_log_name(lptransactinfo->szLogFileNames2[loop], tmp2, loop+1, fileext);
              strcpy(lptransactinfo->szRootPaths2[loop], tmp1);

              STcopy( tmp1, loc2_buf[loop]) ;
			  LOfroms( PATH, loc2_buf[loop], &VCBFtx_loc2[loop] );
              
              LOfaddpath( &VCBFtx_loc2[loop], SystemLocationSubdirectory, &VCBFtx_loc2[loop] );
			  LOfaddpath( &VCBFtx_loc2[loop], ERx( "log" ), &VCBFtx_loc2[loop] );
			  LOtos( &VCBFtx_loc2[loop], &VCBFtx_dir2[loop] );
			  VCBFtx_dir2[loop] = STalloc( VCBFtx_dir2[loop] );
              format_log_name(tmp_buf, tmp2, loop+1, fileext);
              VCBFtx_file2[loop] = STalloc(tmp_buf);
			  LOfstfile(VCBFtx_file2[loop], &VCBFtx_loc2[loop] );
            }
            else
			{ strcpy(lptransactinfo->szRootPaths2[loop], "");
              sprintf(lptransactinfo->szLogFileNames2[loop], "");
            }
           
            MEfree( tmp1 );
        } // for...        

		if( *lptransactinfo->szRootPaths2[0] != EOS && LOexist( &VCBFtx_loc2[0] ) == OK )
		{
			/* secondary transaction log exists */

			LOinfo_flag = LO_I_SIZE;
			if( LOinfo( &VCBFtx_loc2[0], &LOinfo_flag, &loc_info ) != OK || loc_info.li_size == 0 )
			{
				char *value;

				PMmSetDefault( config_data, 1, host );
				STprintf( tmp_buf, "%s.$.rcp.file.size",
					  SystemCfgPrefix );
				if( PMmGet( config_data,
					    tmp_buf,
					    &value ) != OK )
				{
					mymessagewithtitle(ERget(S_ST038C_no_secondary_size ) );
					STprintf( size2, ERx( "???????" ) );
				}
				else
				{
#ifdef NT_GENERIC
					long ivalue;
#else
					int ivalue;
#endif /* NT_GENERIC */

					CVal( value, &ivalue );
					loc_info.li_size = ivalue;
					STprintf( size2, ERx( "%ld" ),
						loc_info.li_size / 1048576L );
				}
				PMmSetDefault( config_data, 1, NULL );
			}
			else
			{
				STprintf( size2, ERx( "%ld" ),
					loc_info.li_size / 1048576L );
			}
	
			/* determine whether secondary log is enabled */
			if( OK == call_rcpstat( ERx( "-enable -dual -silent" ))) 
			{
				STcopy( ERget( S_ST0330_yes ), enabled2 );
				VCBFtx_log_status |= SECONDARY_LOG_ENABLED;
			}
			else
				STcopy( ERget( S_ST0331_no ), enabled2 );

			STcopy( ERget( S_ST0331_no ), raw2 );   
# ifdef UNIX
			/* determine whether secondary log is raw */
			STprintf( cmd, ERx( "iichkraw %s" ), lptransactinfo->szRootPaths2[0] );
			if( PCcmdline( (LOCATION *) NULL, cmd, PC_WAIT,
				(LOCATION *) NULL,
				&cl_err ) == OK )
			{
				STcopy( ERget( S_ST0330_yes ), raw2 );  
			}
# endif /* UNIX */
		}
		else
		{
			*size2 = EOS;
			*enabled2 = EOS;
			*raw2 = EOS;
		}

		OFFSET_TYPE lsize2 = 0;
		OFFSET_TYPE tmp_file_size2 = 0;
		lptransactinfo->bLogFilesizesEqual2=TRUE;
		for  (loop = 0; loop < (lptransactinfo->iNumberOfLogFiles2)&&(*lptransactinfo->szRootPaths2[loop] != EOS); loop++)
		{
			if(LOexist( &VCBFtx_loc2[loop] ) == OK )
			{
				/*Dual Transaction log exists*/
				LOinfo_flag = LO_I_SIZE;
				if (LOinfo(&VCBFtx_loc2[loop], &LOinfo_flag, &loc_info) == OK || loc_info.li_size != 0) 
				{
					if(tmp_file_size2 == 0)
						tmp_file_size2 = loc_info.li_size;
					else if(loc_info.li_size != tmp_file_size2)
					{
						mymessagewithtitle(ERget(S_ST038C_no_secondary_size));
						lptransactinfo->bLogFilesizesEqual2 = FALSE;
						break;
					}
					lsize2 += loc_info.li_size;
					STprintf( size2, ERx( "%ld" ),
						(i4)(lsize2 / 1048576L ));
				}
			}
			else
			{
				//mymessagewithtitle(ERget(S_ST038C_no_secondary_size));
				lptransactinfo->bLogFilesizesEqual2 = FALSE;
				break;
			}

		} //end of 'for' loop. which runs for all dual log-files

		strcpy(lptransactinfo->szsize2   ,size2   );
		strcpy(lptransactinfo->szenabled2,enabled2);
		strcpy(lptransactinfo->szraw2    ,raw2    );


		if( STcompare( log_sym, ERx( "II_LOG_FILE" ) ) == 0 )
			lptransactinfo->bStartOnFirstOnInit=TRUE;
		else
			lptransactinfo->bStartOnFirstOnInit=FALSE;

		if (VCBFtx_read_only)
        {
			lptransactinfo->bEnableCreate     =FALSE;
			lptransactinfo->bEnableReformat   =FALSE;
			lptransactinfo->bEnableErase      =FALSE;
			lptransactinfo->bEnableDisable    =FALSE;
			lptransactinfo->bEnableDestroy    =FALSE;
		}
		else
        {
			lptransactinfo->bEnableCreate     =TRUE;
			lptransactinfo->bEnableReformat   =TRUE;
			lptransactinfo->bEnableErase      =TRUE;
			lptransactinfo->bEnableDisable    =TRUE;
			lptransactinfo->bEnableDestroy    =TRUE;
		}
		
        strcpy(lptransactinfo->szstate,VCBFtx_state);
		return TRUE;
}

BOOL VCBFll_tx_OnTimer(LPTRANSACTINFO lptransactinfo)
{
	
   char tmp_buf[ BIG_ENOUGH ];
   /*
   ** Check whether the logging system has gone on-line
   ** at each timeout (if form is not in read_only mode),
   ** and make it read-only if it is on-line. 
   */
   if( !VCBFtx_read_only && OK == call_rcpstat( ERx( "-online -silent" ) ))
   {
#ifdef OIDSK
       strcpy  ( tmp_buf, ERget( S_ST039A_log_ops_disabled ));
#else
       STprintf( tmp_buf, ERget( S_ST039A_log_ops_disabled ),
	  SystemProductName );
#endif
       VCBFtx_read_only = TRUE;
       mymessagewithtitle(tmp_buf);

       STcopy( ERget( S_ST0424_on_line ), VCBFtx_state ); 

       strcpy(lptransactinfo->szstate ,VCBFtx_state);   
       lptransactinfo->bEnableCreate     =FALSE;
       lptransactinfo->bEnableReformat   =FALSE;
       lptransactinfo->bEnableErase      =FALSE;
       lptransactinfo->bEnableDisable    =FALSE;
       lptransactinfo->bEnableDestroy    =FALSE;
       return TRUE;
   }
   return FALSE;
}

static BOOL VCBFtx_reformat(BOOL bPrimary,LPTRANSACTINFO lptransactinfo)
{
	char cmd[ BIG_ENOUGH ]={0};
    if( bPrimary) 
       STcopy( ERx( "-force_init_log -silent" ), cmd );
    else
	STcopy( ERx( "-force_init_dual -silent"), cmd );

#ifdef FUTURE
	=> exec frs message :ERget( S_ST038E_formatting_log );
#endif



    if( OK != call_rcpconfig(cmd))
    {
	mymessage(ERget( S_ST038F_log_format_failed ));
    }
    else
    {
       if (bPrimary)
	     strcpy(lptransactinfo->szenabled1,"yes");  // ERget( S_ST0330_yes )
	   else
	     strcpy(lptransactinfo->szenabled2,"yes");

	/* update log_status mask */
	if( bPrimary)
		VCBFtx_log_status |= PRIMARY_LOG_ENABLED;
	else
		VCBFtx_log_status |= SECONDARY_LOG_ENABLED;

	    lptransactinfo->bEnableDisable=TRUE;
    }

    return TRUE;
}

BOOL VCBFll_tx_OnCreate(BOOL bPrimary,LPTRANSACTINFO lptransactinfo,char *lpsizeifwasempty)
{               
    char size[ BIG_ENOUGH ];
    char value[ BIG_ENOUGH ];
    char tmp_buf[ BIG_ENOUGH ];
    char **log_dir, **log_file;
    i4 kbytes;
    bool format = FALSE;
    int loop;

    if (bPrimary)
       lptransactinfo->iNumberOfLogFiles1 = 0;
    else
       lptransactinfo->iNumberOfLogFiles2 = 0;
    /*
    ** If no log exists, prompt for new size (in megabytes),
    ** enforce 8 megabytes.  SIR: It would be better to
    ** refer to the rules for the minimum megabytes.
    */

    for (loop=0; loop < LG_MAX_FILE; loop++)
    {   
       if (bPrimary)
       { if (!strcmp(lptransactinfo->szRootPaths1[loop], ""))
         { // No path exists
           VCBFtx_dir1[loop]  = 0;
           VCBFtx_file1[loop] = 0;

           STprintf( tmp_buf, "%s.%s.rcp.log.log_file_%d", SystemCfgPrefix, node, loop+1);
           (void) PMmDelete( config_data, tmp_buf );
         }
         else
         { // Path exists
           STprintf( tmp_buf, ERx( "%s%cingres%clog" ), lptransactinfo->szRootPaths1[loop],FILENAME_SEPARATOR,FILENAME_SEPARATOR); 
	       VCBFtx_dir1[loop]  = STalloc( tmp_buf );
           VCBFtx_file1[loop] = STalloc( lptransactinfo->szLogFileNames1[loop] );

           STprintf( tmp_buf, "%s.%s.rcp.log.log_file_%d", SystemCfgPrefix, node, loop+1);
 	       (void) PMmDelete( config_data, tmp_buf );
           (void) PMmInsert( config_data, tmp_buf, lptransactinfo->szRootPaths1[loop] );
           lptransactinfo->iNumberOfLogFiles1++ ;
         }
       }
       else  // Dual Log
       { if (!strcmp(lptransactinfo->szRootPaths2[loop], ""))
         { // No path exists
           VCBFtx_dir2[loop]  = 0;
           VCBFtx_file2[loop] = 0;

           STprintf( tmp_buf, "%s.%s.rcp.log.dual_log_%d", SystemCfgPrefix, node, loop+1);
           (void) PMmDelete( config_data, tmp_buf );
         }
         else
         { // Path exists
           STprintf( tmp_buf, ERx( "%s%cingres%clog" ), lptransactinfo->szRootPaths2[loop],FILENAME_SEPARATOR,FILENAME_SEPARATOR); 
	       VCBFtx_dir2[loop]  = STalloc( tmp_buf );
           VCBFtx_file2[loop] = STalloc( lptransactinfo->szLogFileNames2[loop] );

           STprintf( tmp_buf, "%s.%s.rcp.log.dual_log_%d", SystemCfgPrefix, node, loop+1);
 	       (void) PMmDelete( config_data, tmp_buf );
           (void) PMmInsert( config_data, tmp_buf, lptransactinfo->szRootPaths2[loop] );
           lptransactinfo->iNumberOfLogFiles2++;
         }
       }
    } // for...

    if (bPrimary)
    { STprintf( tmp_buf, "%s.%s.rcp.log.log_file_name", SystemCfgPrefix, node );
      STprintf( value, ERx( "%s" ), lptransactinfo->szConfigDatLogFileName1 );
      (void) PMmDelete( config_data, tmp_buf );
      (void) PMmInsert( config_data, tmp_buf, value );
    }
    else
    {
      STprintf( tmp_buf, "%s.%s.rcp.log.dual_log_name", SystemCfgPrefix, node );
	  STprintf( value, ERx( "%s" ), lptransactinfo->szConfigDatLogFileName2 );
      (void) PMmDelete( config_data, tmp_buf );
      (void) PMmInsert( config_data, tmp_buf, value );
    }  

    STprintf( tmp_buf, "%s.%s.rcp.log.log_file_parts",
        SystemCfgPrefix, host );
    STprintf( value, ERx( "%d" ), lptransactinfo->iNumberOfLogFiles1+lptransactinfo->iNumberOfLogFiles2);
    (void) PMmDelete( config_data, tmp_buf );
    (void) PMmInsert( config_data, tmp_buf, value );

    PMmSetDefault( config_data, 1, NULL );
    if( PMmWrite( config_data, NULL ) != OK )
	{
		mymessage(ERget(S_ST0314_bad_config_write));
		    VCBFRequestExit();
		    return FALSE;
	}


    if (bPrimary)
    {
      log_dir = VCBFtx_dir1;
      log_file = VCBFtx_file1;  //(char **)(lptransactinfo->szLogFileNames1); 
	
      strcpy(size, lptransactinfo->szsize2);
    }
    else
    {
	  log_dir = VCBFtx_dir2;
	  log_file = VCBFtx_file2;
	
      strcpy(size, lptransactinfo->szsize1);
    }

    if( *size == EOS )   // There is no other tx log to force as value
    { i4 mbytes;
	  
      strcpy(size,lpsizeifwasempty);

	  if (size[0]=='\0')
      { mymessage("size is empty");
	    return FALSE;
	  }
	
      if( CVal( size, &mbytes ) != OK )
      {	mymessage(ERget( S_ST039D_integer_expected ));
		return FALSE;
	  }

	  if( mbytes < 16 )
	  { mymessage(ERget( S_ST039E_log_too_small ) );
		return FALSE;
	  }
	  
	  PMmSetDefault( config_data, 1, host );
      STprintf( tmp_buf, "%s.$.rcp.file.kbytes", SystemCfgPrefix );
      (void) PMmDelete( config_data, tmp_buf );

	  STprintf( value, ERx( "%ld" ), mbytes * 1024L );
      (void) PMmInsert( config_data, tmp_buf, value );

      PMmSetDefault( config_data, 1, NULL );
      if( PMmWrite( config_data, NULL ) != OK )
	  {
	     mymessage(ERget(S_ST0314_bad_config_write));
	     VCBFRequestExit();
	     return FALSE;
	  }
    }
    else  // The is a size from an existing size in the other log file
    {  
       // We force the user to put the same size value in the "Create" dialog
       #if 0
        int ret = MessageBox(GetFocus(),
		"This operation will create a transaction log of the same size as the "
		"existing (backup) log.  If you want to change the transaction log size "
		"for this installation, you must first destroy the other "
		"transaction log file.\n\nDo you want to continue this operation?", stdtitle,
		 MB_ICONQUESTION | MB_YESNO | MB_TASKMODAL);
       
        if (ret != IDYES) 
           return FALSE;
        #endif
    }

if((OK == PMmSetDefault( config_data, 0, SystemCfgPrefix ))&& (OK == PMmSetDefault( config_data, 1, node )))
{
	if( (kbytes = write_transaction_log( TRUE,
	config_data, log_dir, log_file,
	display_message, init_graph, 63,
	lptransactinfo->fnUpdateGraph )) == 0) // update_graph )) == 0 )
    {
	   mymessage(ERget( S_ST038D_log_creation_failed ));
    }
    else
    {
	   char *no_str = ERget( S_ST0331_no );

	   STprintf( size, ERx( "%ld" ), kbytes / 1024L );

	   if (bPrimary)
       {
	      strcpy(lptransactinfo->szsize1,size);
	      strcpy(lptransactinfo->szenabled1,no_str);
	      strcpy(lptransactinfo->szraw1,no_str);
       }
       else
       {
	      strcpy(lptransactinfo->szsize2,size);
	      strcpy(lptransactinfo->szenabled2,no_str);
	      strcpy(lptransactinfo->szraw2,no_str);
	   }

	   /* update log_status mask */
	   if( bPrimary)
       {
  	      u_i2 mod_status = VCBFtx_log_status |
		  PRIMARY_LOG_ENABLED;
		  VCBFtx_log_status = mod_status -
		  PRIMARY_LOG_ENABLED;
	   }
	   else
       {
          u_i2 mod_status = VCBFtx_log_status |
		  SECONDARY_LOG_ENABLED;
		  VCBFtx_log_status = mod_status -
		  SECONDARY_LOG_ENABLED;
	   }

	   lptransactinfo->bEnableCreate  =FALSE;
	   lptransactinfo->bEnableReformat=TRUE;
	   lptransactinfo->bEnableErase   =TRUE;
	   lptransactinfo->bEnableDestroy =TRUE;

       if (lptransactinfo->fnSetText)
       { if (bPrimary)
           lptransactinfo->fnSetText("Primary Transaction Log Created.  Reformatting Log File...");
         else
           lptransactinfo->fnSetText("Dual Transaction Log Created.  Reformatting Log File...");
       }
	
       return VCBFtx_reformat(bPrimary,lptransactinfo);
    }
}

    return TRUE;
}  // OnCreate()

BOOL VCBFll_tx_OnReformat(BOOL bPrimary,LPTRANSACTINFO lptransactinfo)
{
    // Moved messages to logfidlg and logfsdlg classes
    // int iret;
    // if( VCBFtx_transactions )
    // {
	//    mymessage(ERget( S_ST0389_transaction_warning ));
	//    iret = MessageBox(GetFocus(),
	// 		  "Do you want to throw away any uncommitted transactions contained in the "
	// 		  "selected transaction log by reformatting? ",
	// 		  stdtitle,
	// 		  MB_ICONQUESTION | MB_YESNO |MB_TASKMODAL);
	//    if (iret != IDYES) 
    //      return FALSE;
    // }
    
    return VCBFtx_reformat(bPrimary,lptransactinfo);
}

BOOL VCBFll_tx_OnDisable(BOOL bPrimary,LPTRANSACTINFO lptransactinfo)
{
    
    char other_enabled[ BIG_ENOUGH ];
    char other_tblfld[ BIG_ENOUGH ];
	char cmd[ BIG_ENOUGH ] = {0};

    if( bPrimary)
    {
	STcopy( ERx( "-disable_log -silent" ),cmd );
	strcpy(other_enabled,lptransactinfo->szenabled2);
	STcopy( ERx( "secondary_log" ), other_tblfld );
    }
    else
    {
	STcopy( ERx( "-disable_dual -silent" ), cmd );
	strcpy(other_enabled,lptransactinfo->szenabled1);
	STcopy( ERx( "primary_log" ), other_tblfld );
    }

    if( *other_enabled == EOS )
    {
	mymessage(ERget( S_ST0390_cant_disable_log ));
	return FALSE;
    }

#ifdef FUTURE
    => exec frs message :ERget( S_ST0391_disabling_log ); 
#endif

    if( OK != call_rcpconfig(cmd))
    {
       mymessage(ERget( S_ST0392_log_disable_failed ));
    }
    else
    {
	if (bPrimary)
	     strcpy(lptransactinfo->szenabled1,ERget( S_ST0331_no ));
	else
	     strcpy(lptransactinfo->szenabled2,ERget( S_ST0331_no ));

	/* update log_status mask */
	if( bPrimary)
	{
		u_i2 mod_status = VCBFtx_log_status | 
			PRIMARY_LOG_ENABLED;
		VCBFtx_log_status = mod_status -
			PRIMARY_LOG_ENABLED;
	}
	else
	{
		u_i2 mod_status = VCBFtx_log_status |
			SECONDARY_LOG_ENABLED;
		VCBFtx_log_status = mod_status -
			SECONDARY_LOG_ENABLED;
	}

	lptransactinfo->bEnableDisable=FALSE;

	if( *other_enabled != EOS && STcompare( other_enabled,
		ERget( S_ST0331_no ) ) == 0 )
	{
		if (bPrimary)
		    strcpy(lptransactinfo->szenabled2,ERget( S_ST0330_yes ));
		else
		    strcpy(lptransactinfo->szenabled1,ERget( S_ST0330_yes ));

		/* update log_status mask */
		if( STcompare( other_tblfld,
			ERx( "primary_log" ) ) == 0 )
		{
			VCBFtx_log_status |= PRIMARY_LOG_ENABLED;
		}
		else
			VCBFtx_log_status |= SECONDARY_LOG_ENABLED;

		mymessagewithtitle(ERget( S_ST0393_other_log_enabled ));
	}
    }
    return TRUE;
}

BOOL VCBFll_tx_OnErase(BOOL bPrimary, LPTRANSACTINFO lptransactinfo)
{
	char **log_dir, **log_file;
	bool format = FALSE;

	if( bPrimary)
	{
		log_dir = VCBFtx_dir1;
		log_file = VCBFtx_file1;
    }
	else
	{
		log_dir = VCBFtx_dir2;
		log_file = VCBFtx_file2;
	}

	if( write_transaction_log( FALSE, config_data,
		log_dir, log_file, display_message,
    	init_graph, 63, update_graph ) == 0 )
    {
		mymessage(ERget( S_ST0394_log_erase_failed ));
	}
	else   
	    return VCBFtx_reformat(bPrimary,lptransactinfo);

	return FALSE;
}

BOOL VCBFll_tx_OnDestroy(BOOL bPrimary,LPTRANSACTINFO lptransactinfo)
{
	
	char *empty;
    char szPath[BIG_ENOUGH];
    char other_enabled[ BIG_ENOUGH ];
	LOCATION *loc;
    int iret;
	
	// Moved message to logfidlg and logfsdlg classes
    // if( VCBFtx_transactions )
	//   mymessagewithtitle(ERget( S_ST0389_transaction_warning ));
	
    MessageBeep(MB_ICONEXCLAMATION);
    iret = MessageBox(GetFocus(),
			  "Are you sure you want to destroy the transaction log?", stdtitle,
			  MB_ICONQUESTION | MB_YESNO |MB_TASKMODAL);
	
    if (iret != IDYES) 
	    return FALSE;

	if( bPrimary)
		loc = &VCBFtx_loc1[0];
	else
		loc = &VCBFtx_loc2[0];

	if(bPrimary)
		strcpy(other_enabled,lptransactinfo->szenabled2);
	else
		strcpy(other_enabled,lptransactinfo->szenabled1);

	if( *other_enabled != EOS )
		BOOL bDontCare = VCBFll_tx_OnDisable(bPrimary, lptransactinfo);

#ifdef FUTURE
	=>exec frs message :ERget( S_ST039B_destroying_log );
#endif
    
    for  (int loop = 0; loop < LG_MAX_FILE; loop++)
	{ char *path1;
      LOtos( &loc[loop], &path1 );  // path1 == lptransactinfo->szRootPaths1[loop];
      
	  if (path1 && strcmp(path1, "")) 
      {
        if( LOdelete( &loc[loop] ) != OK )
        { 
	      if (bPrimary)
             sprintf(szPath, "%s%cingres%clog%c%s", lptransactinfo->szRootPaths1[loop],FILENAME_SEPARATOR,FILENAME_SEPARATOR,FILENAME_SEPARATOR, lptransactinfo->szLogFileNames1[loop]);
          else
             sprintf(szPath, "%s%cingres%clog%c%s", lptransactinfo->szRootPaths2[loop],FILENAME_SEPARATOR,FILENAME_SEPARATOR,FILENAME_SEPARATOR, lptransactinfo->szLogFileNames2[loop]);

          if (GetFileAttributes(szPath) != 0xffffffff)
          { char szBuffer[1024];
            //mymessagewithtitle(ERget( S_ST0399_log_destroy_failed ));
            MessageBeep(MB_ICONEXCLAMATION);
            sprintf(szBuffer, "Error:  Cannot destroy the log file '%s'.  The file may be in use or read only.  You must manually delete the file to continue.", szPath);
            mymessagewithtitle(szBuffer);
            return FALSE;
          }
        }

      }
    } // for...

    empty = ERx( "" );

	if (bPrimary)  {
		strcpy(lptransactinfo->szsize1,   empty);
		strcpy(lptransactinfo->szenabled1,empty);
		strcpy(lptransactinfo->szraw1,    empty);
	}
	else {
		strcpy(lptransactinfo->szsize2,   empty);
		strcpy(lptransactinfo->szenabled2,empty);
		strcpy(lptransactinfo->szraw2,    empty);
	}

	lptransactinfo->bEnableCreate  =TRUE;
	lptransactinfo->bEnableReformat=FALSE;
	lptransactinfo->bEnableErase   =FALSE;
	lptransactinfo->bEnableDisable =FALSE;
	lptransactinfo->bEnableDestroy =FALSE;

	/* update log_status mask */
	if( bPrimary)
	{
		u_i2 mod_status = VCBFtx_log_status | PRIMARY_LOG_ENABLED;
		VCBFtx_log_status = mod_status - PRIMARY_LOG_ENABLED;
	}
	else
	{
		u_i2 mod_status = VCBFtx_log_status | SECONDARY_LOG_ENABLED;
		VCBFtx_log_status = mod_status - SECONDARY_LOG_ENABLED;
	}
	
    return TRUE;
}

BOOL VCBFll_tx_OnExit(LPTRANSACTINFO lptransactinfo,LPCOMPONENTINFO lpLog1, LPCOMPONENTINFO lpLog2)
{
# ifdef UNIX
		if( OK != call_rcpstat(ERx(" -online -silent")))
		{
#ifdef FUTURE
		  =>    exec frs message :ERget( S_ST0395_cleaning_up_memory ); 
#endif
			do_ipcclean();
		}
# endif /* UNIX */

	if( VCBFtx_log_status & PRIMARY_LOG_ENABLED )
	      strcpy(lpLog1->szcount,ERx( "1" ));
	else
	      strcpy(lpLog1->szcount,ERx( "0" ));

	if( VCBFtx_log_status & SECONDARY_LOG_ENABLED )
	      strcpy(lpLog2->szcount,ERx( "1" ));
	else
	      strcpy(lpLog2->szcount,ERx( "0" ));
	return TRUE;
}

BOOL VCBFll_tx_OnActivatePrimaryField(LPTRANSACTINFO lptransactinfo)
{
      if( VCBFtx_read_only )
	    return TRUE;

      if( lptransactinfo->szRootPaths1[0] == EOS )   /* symbols missing */
      {	
	    lptransactinfo->bEnableCreate=  FALSE;
	    lptransactinfo->bEnableReformat=FALSE;
	    lptransactinfo->bEnableErase=   FALSE;
	    lptransactinfo->bEnableDisable= FALSE;
	    lptransactinfo->bEnableDestroy= FALSE;
      }
      else  /* symbols are set */
      {  
	    if( lptransactinfo->szsize1[0] == EOS )  /* transaction log doesn't exist */
	    { 
		  lptransactinfo->bEnableCreate=  TRUE;
		  lptransactinfo->bEnableReformat=FALSE;
		  lptransactinfo->bEnableErase=   FALSE;
		  lptransactinfo->bEnableDisable= FALSE;
		  lptransactinfo->bEnableDestroy= FALSE;
	    }
	    else  /* transaction log exists */
       {  
	 	  lptransactinfo->bEnableCreate=  FALSE;
		  lptransactinfo->bEnableReformat=TRUE;
		  lptransactinfo->bEnableErase=   TRUE;

		  if( STcompare( lptransactinfo->szenabled1, ERget( S_ST0330_yes ) ) == 0 )
			lptransactinfo->bEnableDisable= TRUE;
		  else
			lptransactinfo->bEnableDisable= FALSE;

		  lptransactinfo->bEnableDestroy = TRUE;
	    }
      }
      
      return TRUE;
}


BOOL VCBFll_tx_OnActivateSecondaryField(LPTRANSACTINFO lptransactinfo)
{
	
	if( VCBFtx_read_only )
		return FALSE;

	if( lptransactinfo->szRootPaths2[0] == EOS )
	{
		/* symbols missing */
		lptransactinfo->bEnableCreate=  FALSE;
		lptransactinfo->bEnableReformat=FALSE;
		lptransactinfo->bEnableErase=   FALSE;
		lptransactinfo->bEnableDisable= FALSE;
		lptransactinfo->bEnableDestroy= FALSE;
	}
	else
	{
		/* symbols are set */

		if( lptransactinfo->szsize2[0] == EOS )
		{
			/* transaction log doesn't exist */
			lptransactinfo->bEnableCreate=  TRUE;
			lptransactinfo->bEnableReformat=FALSE;
			lptransactinfo->bEnableErase=   FALSE;
			lptransactinfo->bEnableDisable= FALSE;
			lptransactinfo->bEnableDestroy= FALSE;
		}
		else
		{
			/* transaction log exists */
			lptransactinfo->bEnableCreate=  FALSE;
			lptransactinfo->bEnableReformat=TRUE;
			lptransactinfo->bEnableErase=   TRUE;

			if( STcompare( lptransactinfo->szenabled2, ERget( S_ST0330_yes ) )
				== 0 )
				lptransactinfo->bEnableDisable= TRUE;
			else
			{
				lptransactinfo->bEnableDisable= FALSE;
			}
			lptransactinfo->bEnableDestroy= TRUE;
		}
	}
        return TRUE;
}


static COMPONENTINFO VCBFsettings_comp;
static BOOL VCBFsettings_checked;
BOOL VCBFll_settings_Init(BOOL *pbSyscheck)
{
	BOOL bResult;
	GENERICLINEINFO lineinfo;
	VCBFsettings_comp.itype=COMP_TYPE_SETTINGS ;
	bResult=VCBFllInitGenericPane(&VCBFsettings_comp);
	if (!bResult)
	   return FALSE;
 
	bResult=VCBFllGetFirstGenericLine(&lineinfo);
	while (bResult) {
	    if (!strcmp(lineinfo.szname,"ingstart_syscheck")) {
		if (!stricmp(lineinfo.szvalue,"ON")) 
		     VCBFsettings_checked = TRUE;
		else  
		     VCBFsettings_checked = FALSE;
		*pbSyscheck=VCBFsettings_checked;
		return TRUE;
	    }
	    bResult=VCBFllGetNextGenericLine(&lineinfo) ;
	}
	mymessage("Preferences parameters unavailable");
	return FALSE;
 
}

BOOL VCBFll_On_settings_CheckChanged(BOOL bChecked,BOOL *pbCheckedResult)
{
    GENERICLINEINFO lineinfo;
    char in_buf[100];
    if ((bChecked && VCBFsettings_checked) ||
	(!bChecked && !VCBFsettings_checked)
       )  {
	    *pbCheckedResult=bChecked;
	    return TRUE;
    }
    strcpy(lineinfo.szname,"ingstart_syscheck");
    if (VCBFsettings_checked)
	    strcpy(lineinfo.szvalue,"ON");
    else
	    strcpy(lineinfo.szvalue,"OFF");
    strcpy(lineinfo.szunit,"boolean");
    if (bChecked)
	    strcpy(in_buf,"ON");
    else
	    strcpy(in_buf,"OFF");
    lineinfo.iunittype=GEN_LINE_TYPE_BOOL;
    VCBFllGenPaneOnEdit(&lineinfo,in_buf);
    if (!stricmp(lineinfo.szvalue,"ON"))
	    *pbCheckedResult=TRUE;
    else
	    *pbCheckedResult=FALSE;

    return TRUE;
}
BOOL VCBFll_settings_Terminate()
{
    return VCBFllOnMainComponentExit(&VCBFsettings_comp);
} 
		   
static char VCBFForm_parms[10][100];
typedef struct s1parms {
  int type;
  char * lptype;
  BOOL bFound;
} S1PARMS;
S1PARMS netparams[] = {
     {NET_CTRL_INBOUND ,"inbound_limit" , FALSE},
     {NET_CTRL_OUTBOUND,"outbound_limit", FALSE},
     {NET_CTRL_LOGLEVEL,"log_level"     , FALSE},
     {0                ,""              , FALSE}},
 nameparams[] = {
     {NAME_CHECK_INTERVAL   , "check_interval"      , FALSE},
     {NAME_DEFAULT_SVR_CLASS, "default_server_class", FALSE},
     {NAME_LOCAL_VNODE      , "local_vnode"         , FALSE},
     {NAME_SESSION_LIMIT    , "session_limit"       , FALSE},
     {0                     , ""                    , FALSE}};

static BOOL VCBFisNameForm;
static BOOL VCBFll_InitFormParms(BOOL bName)
{
    BOOL bResult;
    GENERICLINEINFO lineinfo;
    int i;
    S1PARMS * s1= bName?nameparams:netparams;
    int imax    = bName?NAME_MAX_PARM:NET_MAX_PARM;
    VCBFisNameForm=bName;
    for (i=0;i<imax;i++){
      strcpy(VCBFForm_parms[i],"");
      s1[i].bFound=FALSE;
    }

    bResult=VCBFllGetFirstGenericLine(&lineinfo);
    while (bResult) {
	for (i=0;i<imax;i++) {
	    if (!strcmp(lineinfo.szname,s1[i].lptype)) {
		strcpy(VCBFForm_parms[s1[i].type],lineinfo.szvalue);
		s1[i].bFound=TRUE;
	    }
	}
	bResult=VCBFllGetNextGenericLine(&lineinfo) ;
    }
    for (i=0;i<imax;i++) {
      if (!s1[i].bFound) {
	char buf1[100];
	sprintf(buf1,"%s parameter unavailable",s1[i].lptype);
	mymessage(buf1);
	return FALSE;
      }
    }
    return TRUE;
}
BOOL VCBFll_Init_net_Form()
{
  return VCBFll_InitFormParms(FALSE);
}
BOOL VCBFll_Init_name_Form()
{
  return VCBFll_InitFormParms(TRUE);
}

BOOL VCBFll_Form_GetParm(int ictrl,char * buf)
{
    strcpy(buf,VCBFForm_parms[ictrl]);
    return TRUE;
}

BOOL VCBFll_Form_OnEditParm(int ictrl,char * in_buf, char * bufresult)
{
    S1PARMS * s1= VCBFisNameForm?nameparams:netparams;
    int imax    = VCBFisNameForm?NAME_MAX_PARM:NET_MAX_PARM;
    int i;
    if (ictrl<1 || ictrl>imax) {
	mymessage("internal error");
	return FALSE;
    }
    for(i=0;i<imax;i++) {
	if (s1[i].type==ictrl) {
	    GENERICLINEINFO lineinfo;
	    BOOL bResult;
	    strcpy(lineinfo.szname,s1[i].lptype);
	    strcpy(lineinfo.szvalue,VCBFForm_parms[ictrl]);
	    strcpy(lineinfo.szunit,"");
	    bResult= VCBFllGenPaneOnEdit(&lineinfo,in_buf);
	    strcpy(bufresult,lineinfo.szvalue);
	    strcpy(VCBFForm_parms[ictrl],lineinfo.szvalue);
	    return bResult;
	}
    }
    mymessage("internal error");
    return FALSE;
}


static BOOL bConfigSaved=FALSE;

static char App_backup_data_buf[ MAX_LOC + 1 ];
static char App_backup_protect_buf[ MAX_LOC + 1 ];
static char App_backup_change_log_buf [MAX_LOC + 1];
static LOCATION App_backup_data_file;
static LOCATION App_backup_protect_file;
static LOCATION App_backup_change_log_file;

static BOOL SaveConfig()
{
    char temp_buf[ MAX_LOC + 1 ];
    char *name;

    /* prepare a backup LOCATION for the data file */
    STcopy( ERx( "" ), temp_buf );
    LOfroms( FILENAME, temp_buf, &App_backup_data_file );
    LOuniq( ERx( "c0cbfd1" ), NULL, &App_backup_data_file );
    LOtos( &App_backup_data_file, &name );
    STcopy( tempdir, App_backup_data_buf ); 
    STcat( App_backup_data_buf, name );
    LOfroms( PATH & FILENAME, App_backup_data_buf, &App_backup_data_file );
    
    /* prepare a backup LOCATION for the change log */
    STcopy( ERx( "" ), temp_buf );
    LOfroms( FILENAME, temp_buf, &App_backup_change_log_file );
    LOuniq( ERx( "c0cbfc1" ), NULL, &App_backup_change_log_file );
    LOtos( &App_backup_change_log_file, &name );
    STcopy( tempdir, App_backup_change_log_buf ); 
    STcat( App_backup_change_log_buf, name );
    LOfroms( PATH & FILENAME, App_backup_change_log_buf,
	&App_backup_change_log_file );
    
    /* prepare a backup LOCATION for the protected data file */
    STcopy( ERx( "" ), temp_buf );
    LOfroms( FILENAME, temp_buf, &App_backup_protect_file );
    LOuniq( ERx( "c0cbfp1" ), NULL, &App_backup_protect_file );
    LOtos( &App_backup_protect_file, &name );
    STcopy( tempdir, App_backup_protect_buf ); 
    STcat( App_backup_protect_buf, name );
    LOfroms( PATH & FILENAME, App_backup_protect_buf,
	&App_backup_protect_file );
    
    /* write configuration backup */ 
    if( PMmWrite( config_data, &App_backup_data_file ) != OK )
    {
	mymessage(ERget(S_ST0326_backup_failed));
	LOdelete( &App_backup_data_file );
	return( FALSE );
    }

    /* write change log backup */ 
    if( copy_LO_file( &change_log_file, &App_backup_change_log_file )
	!= OK ) 
    {
	    mymessage(ERget(S_ST0326_backup_failed));
       /*       IIUGerr( S_ST0326_backup_failed, 0, 0 );  */
	(void) LOdelete( &App_backup_change_log_file );
	return( FALSE );
    }

    /* write protected data backup */ 
    if( PMmWrite( protect_data, &App_backup_protect_file ) != OK )
    {
	mymessage(ERget(S_ST0326_backup_failed));
	LOdelete( &App_backup_protect_file );
	return( FALSE );
    }
    bConfigSaved=TRUE;
    return TRUE;
}

BOOL VCBFll_bridgeprots_init()
{
	char *name, *value;
	char expbuf[ BIG_ENOUGH ];
	char *protocol_status;

	STATUS status;
	PM_SCAN_REC state;
	bool changed = FALSE;
	char *regexp;
	char request[ BIG_ENOUGH ]; 


	/* construct expbuf for gcb port id scan */
	STprintf( expbuf, ERx( "%s.%s.gcb.%s.%%.port" ), 
		  SystemCfgPrefix, host, VCBFgf_def_name );

	regexp = PMmExpToRegExp( config_data, expbuf );

	VCBFInitBridgeProtLines();
	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		char *protocol;
		char dummy_vnode[2];

		/* extract protocol name */
		protocol = PMmGetElem( config_data, 4, name ); 

                /* copy SPACE to vnode, if it is not defined already*/

		STcopy( " " , dummy_vnode );


		/* get status of scanned protocol */ 
		STprintf( request, ERx( "%s.%s.gcb.%s.%s.status" ),
			  SystemCfgPrefix, host, VCBFgf_def_name, protocol );

		if( PMmGet( config_data, request, &protocol_status )
			!= OK
		)	
			continue;

		VCBFAddBridgeProtLine(protocol, protocol_status, value,dummy_vnode);
	}

	/* construct expbuf for gcb port id scan */
	STprintf( expbuf, ERx( "%s.%s.gcb.%s.%%.port.%%" ), 
		  SystemCfgPrefix, host, VCBFgf_def_name );

	regexp = PMmExpToRegExp( config_data, expbuf );

	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		char *vnode;
		char *protocol;

		/* extract protocol name */
		protocol = PMmGetElem( config_data, 4, name ); 

		/* extract vnode name */
		if( PMnumElem( name ) == 7 )
		   vnode = PMmGetElem( config_data, 6, name );

		/* get status of scanned protocol */ 
		STprintf( request, ERx( "%s.%s.gcb.%s.%s.status" ),
			  SystemCfgPrefix, host, VCBFgf_def_name, protocol );

		if( PMmGet( config_data, request, &protocol_status )
			!= OK
		)	
			continue;

		VCBFAddBridgeProtLine(protocol, protocol_status, value,vnode);

	}
        return TRUE;
}


BOOL VCBFll_bridgeprots_OnEditStatus(LPBRIDGEPROTLINEINFO lpbridgeprotline,char * in_buf)
{
	char request[ BIG_ENOUGH ]; 

	/* compose resource request for insertion */
	STprintf( request, ERx( "%s.%s.gcb.%s.%s.status" ),
		  SystemCfgPrefix, host, VCBFgf_def_name, lpbridgeprotline->szprotocol);

	if( set_resource( request, in_buf ) )
	{
        	strcpy(lpbridgeprotline->szstatus,in_buf);
	        VCBFgf_changed = TRUE;
	        return TRUE;
        }
        return FALSE;
}

BOOL VCBFll_bridgeprots_OnEditListen(LPBRIDGEPROTLINEINFO lpbridgeprotline,char * in_buf)
{
       char request[ BIG_ENOUGH ]; 
		
       if( STzapblank( lpbridgeprotline->szvnode , lpbridgeprotline->szvnode ) == 0 )
       {
           /* compose resource request for insertion */
           STprintf( request, ERx( "%s.%s.gcb.%s.%s.port" ), 
       	      SystemCfgPrefix, host, VCBFgf_def_name, lpbridgeprotline->szprotocol);

           if( set_resource( request, in_buf ) )
           {
	        strcpy(lpbridgeprotline->szlisten,in_buf);
	        VCBFgf_changed = TRUE;
	        return TRUE;
           }
       }
       else
       {
           /* compose resource request for insertion */
           STprintf( request, ERx( "%s.%s.gcb.%s.%s.port.%s" ), 
       	      SystemCfgPrefix, host, VCBFgf_def_name, lpbridgeprotline->szprotocol, 
       	      lpbridgeprotline->szvnode );

           if( set_resource( request, in_buf ) )
           {
	        strcpy(lpbridgeprotline->szlisten,in_buf);
	        VCBFgf_changed = TRUE;
	        return TRUE;
           }
       }
       return FALSE;
}

BOOL VCBFll_bridgeprots_OnEditVnode(LPBRIDGEPROTLINEINFO lpbridgeprotline,char * in_buf)
{
   char request[ BIG_ENOUGH ]; 
   char tmp_buf[ BIG_ENOUGH ];
   char vnode_name[200];
   strcpy(vnode_name, lpbridgeprotline->szvnode);
   if( ( STlength( vnode_name ) != 0 ) &&
   			( STlength( in_buf ) != 0 ) )
   {
       /* compose resource request for deletion */
       STprintf( request, ERx( "%s.%s.gcb.%s.%s.port.%s" ), 
   	      SystemCfgPrefix, host,
   	      VCBFgf_def_name, lpbridgeprotline->szprotocol , vnode_name);
       PMmDelete( config_data, request );
   } 

   if( STzapblank( in_buf, tmp_buf ) == 0 )
   {
       STprintf( request, ERx( "%s.%s.gcb.%s.%s.port" ), 
   	      SystemCfgPrefix, host, VCBFgf_def_name, lpbridgeprotline->szprotocol );
   }
   else
      
       /* compose resource request for insertion */
       STprintf( request, ERx( "%s.%s.gcb.%s.%s.port.%s" ), 
   	      SystemCfgPrefix, host, VCBFgf_def_name, lpbridgeprotline->szprotocol, 
   	      in_buf );

   if( set_resource( request, lpbridgeprotline->szlisten) )
   {
      if( STlength( in_buf ) == 0 ) {
      /* Don't update tablefield */
      }
      else
      {
      /* update tablefield */
           strcpy(lpbridgeprotline->szvnode,in_buf);
           VCBFgf_changed = TRUE;
           return TRUE;
      }
   }
   return TRUE;
}

static void VCBFll_Security_machanism_init(CStringList& listMech)
{
	char expbuf[ BIG_ENOUGH ];
	char* regexp;
	char* name;
	char* value;
	char* ele;
	STATUS status;
	PM_SCAN_REC state;

	/*
	** Build list of mechanism for PM config symbols.
	*/
	STprintf( expbuf, ERx("%s.%s.gcf.mech.%%.%%"), SystemCfgPrefix, host );
	regexp = PMmExpToRegExp( config_data, expbuf );

	for( 
	 status = PMmScan( config_data, regexp, &state, NULL, &name, &value );
	 status == OK;
	 status = PMmScan( config_data, NULL, &state, NULL, &name, &value )
	   )
	{
		ele = PMmGetElem( config_data, 4, name );
		CString strElement = ele;
		strElement.MakeLower(); 
		if (listMech.Find(strElement) == NULL)
			listMech.AddTail(strElement);
	}
}

void VCBFll_Security_machanism_get(CPtrList& listMech)
{
	char expbuf[ BIG_ENOUGH ];
	char* regexp;
	char* name;
	char* value;
	STATUS status;
	PM_SCAN_REC state;

	CStringList ls;
	VCBFll_Security_machanism_init(ls);
	POSITION pos = ls.GetHeadPosition();
	while (pos != NULL)
	{
		CString s = ls.GetNext(pos);

		STprintf( expbuf, ERx( "%s.%s.gcf.mech.%s.%%" ), SystemCfgPrefix,  host, (LPTSTR)(LPCTSTR)s );
		regexp = PMmExpToRegExp( config_data, expbuf );
		for( 
			 status = PMmScan(config_data, regexp, &state, NULL, &name, &value);
			 status == OK; 
			 status = PMmScan( config_data, NULL, &state, NULL, &name, &value ) 
		    )
		{
			char* units;

			if ( PMmGet( units_data, name, &units ) != OK )  
				units = ERx("");
			name = PMmGetElem( config_data, 5, name );

			GENERICLINEINFO* pObj = new GENERICLINEINFO;
			_tcscpy (pObj->szname, s);
			_tcscpy (pObj->szunit, units);
			_tcscpy (pObj->szvalue,value);
			listMech.AddTail(pObj);
		}
	}
	ls.RemoveAll();
}

BOOL VCBFll_Security_machanism_edit(LPCTSTR lpszMech, LPTSTR lpszValue)
{
	char temp_buf[ MAX_LOC + 1 ];
	STprintf( temp_buf, ERx( "%s.%s.gcf.mech.%s.enabled" ), SystemCfgPrefix, host, lpszMech);
	if ( set_resource( temp_buf, lpszValue ) )
	{
		VCBFgf_changed = TRUE;
		return TRUE;
	}
	return FALSE;
}


BOOL IsValidDatabaseName (LPCTSTR dbName)
{
  // Must not contain a point
  if (_tcschr(dbName, '.'))
  	return FALSE;

  // First character must not be Underscore, 0 through 9, #, @ and $
  if (_tcschr("_0123456789#@$", dbName[0]))
  	return FALSE;

  // acceptable name
  return TRUE;
}

#if 0  // unused because II_CHARSET dependent in this context?;
       // also problem with leading underscore
BOOL IsValidDatabaseName (LPCTSTR dbName)
{
  if (!(*dbName))
      return FALSE;
  if (CMnmstart(dbName)==FALSE)
          return FALSE;
  dbName++;
  while (*dbName) {
     if (CMnmchar(dbName)==FALSE)
       return FALSE;
     CMnext(dbName);
  }
  return TRUE;
}
#endif

BOOL VCBFll_OnStartupCountChange(char *szOldCount)
{ 
  VCBFgf_changed = TRUE;
  strcpy(VCBFgf_oldstartupcount, szOldCount);
  VCBFgf_bStartupCountChange = TRUE;
  return TRUE;
}

CString INGRESII_QueryInstallationID()
{
	char *ii_installation;
	CString strInstallationID = _T(" [<error>]");
#ifdef EDBC
	NMgtAt( ERx( "ED_INSTALLATION" ), &ii_installation );
#else
	NMgtAt( ERx( "II_INSTALLATION" ), &ii_installation );
#endif
	if( ii_installation == NULL || *ii_installation == EOS )
		strInstallationID = _T(" [<error>]");
	else
		strInstallationID.Format(_T(" [%s]"),ii_installation);
	return strInstallationID;
}

/* default_page_cache_off() returns TRUE if the respective DMF cache
** has not been enabled.
*/
bool
default_page_cache_off( char *pm_id, i4 *page_size, char *cache_share,
                        char *cache_name)
{
    char        real_pm_id[ BIG_ENOUGH ];
    char        expbuf[ BIG_ENOUGH ];
    char        szPrivate[ BIG_ENOUGH ];
    char        *p;
    char        *stat_val;
    char        *pm_host = host;
    bool        shared;
    char        *cbf_srv_name = ERx("dbms"); // This function is only called with "dbms" in this variable.

    if(STcompare(cache_share, ERx("ON")) == 0)
    {
       shared = TRUE;
       STprintf(szPrivate, ERget( S_ST0332_shared ));
    }
    else
    {
       STprintf(szPrivate, ERget( S_ST0333_private ));
       shared = FALSE;
    }

    /* If shared cache the config.dat entry has cache_name else we use
    ** the config identity.
    */
    if(shared)
    {
       STcat(szPrivate, ERx("."));
       STcat(szPrivate, cache_name);
    }
    else
    {
       /* pm_id of format dbms.* or dbms.ident move point to '.' */
       p = pm_id;
       CMnext( p );
       CMnext( p );
       CMnext( p );
       CMnext( p );
       STcat(szPrivate, p);
    }
    p = szPrivate;

    if (cbf_srv_name != NULL)
    {
       switch(*page_size)
       {
         case P2K:
               STprintf( real_pm_id, ERx( "%s.%s.cache%s_status" ), 
                         cbf_srv_name, p, ERget( S_ST0445_p2k ) );
               break;
         case P4K:
               STprintf( real_pm_id, ERx( "%s.%s.cache%s_status" ),
                         cbf_srv_name, p, ERget( S_ST0437_p4k ));
               break;
         case P8K:
               STprintf( real_pm_id, ERx( "%s.%s.cache%s_status" ),
                         cbf_srv_name, p, ERget( S_ST0438_p8k ));
               break;
         case P16K:
               STprintf( real_pm_id, ERx( "%s.%s.cache%s_status" ),
                         cbf_srv_name, p, ERget( S_ST0439_p16k ));
               break;
         case P32K:
               STprintf( real_pm_id, ERx( "%s.%s.cache%s_status" ),
                         cbf_srv_name, p, ERget( S_ST0440_p32k ));
               break;
         case P64K:
               STprintf( real_pm_id, ERx( "%s.%s.cache%s_status" ),
                         cbf_srv_name, p, ERget( S_ST0441_p64k ));
               break;
       }
    }
    STprintf( expbuf, ERx( "%s.%s.%s" ), SystemCfgPrefix,VCBFgf_pm_host, real_pm_id );
    if ( PMmGet ( config_data, (char *)&expbuf, &stat_val ) != OK )
        stat_val = ERx( "" );

    if(STcompare(stat_val, ERx("OFF")) == 0)
       return(TRUE);
    else
       return(FALSE);
}

static BOOL VCBF_pm_initialized = FALSE;
int  BRIDGE_CheckValide(CString& strVNode, CString& strProtocol, CString& strListenAddress)
{
	char* pm_val;
	char request[ BIG_ENOUGH ]; 
	strVNode.TrimRight();
	strProtocol.TrimRight();
	strListenAddress.TrimRight();

	if (!VCBF_pm_initialized)
	{
		PMinit();
		if (PMload(NULL, (PM_ERR_FUNC *)NULL) == OK)
			VCBF_pm_initialized = TRUE;
	}
	if (strProtocol.IsEmpty())
	{
		AfxMessageBox(ERget(S_ST031F_edit_protocol_prompt));
		return 2;
	}
	if (strListenAddress.IsEmpty())
	{
		AfxMessageBox(ERget(E_ST000E_listen_req));
		return 3;
	}
	if (strVNode.IsEmpty())
	{
		AfxMessageBox(ERget(S_ST0530_vnode_prompt));
		return 1;
	}

	STprintf( request, ERx( "%s.%s.gcb.%s.%s.status" ), SystemCfgPrefix, host, "*", (LPTSTR)(LPCTSTR)strProtocol );
	if ( PMget( request, &pm_val ) != OK ) 
	{
		AfxMessageBox(ERget(S_ST031F_edit_protocol_prompt));
		return 2;
	}
	return 0;
}

BOOL BRIDGE_AddVNode(char* vnode, char* port, char* protocol)
{
	char request[ BIG_ENOUGH ]; 

	STprintf( request, ERx( "%s.%s.gcb.%s.%s.port.%s" ), SystemCfgPrefix, host, "*", protocol, vnode );
	BOOL bOK1 = set_resource( request, port);
	/*  default the status to ON  */ 
	STprintf( request, ERx( "%s.%s.gcb.%s.%s.status.%s" ), SystemCfgPrefix, host, "*", protocol, vnode);
	BOOL bOK2 = set_resource( request, ERx( "ON" ) ); 
	if (bOK1 && bOK2)
		VCBFgf_changed = TRUE;
	else
		return FALSE;
	return TRUE;
}

BOOL BRIDGE_DelVNode(char* vnode, char* port, char* protocol)
{
	char request[ BIG_ENOUGH ]; 
	STprintf( request, ERx( "%s.%s.gcb.%s.%s.port.%s" ), SystemCfgPrefix, host, "*", protocol, vnode );
	BOOL bOK1 = set_resource( request, NULL);
	/*  default the status to ON  */ 
	STprintf( request, ERx( "%s.%s.gcb.%s.%s.status.%s" ), SystemCfgPrefix, host, "*", protocol, vnode);
	BOOL bOK2 = set_resource( request, NULL); 
	if (bOK1 && bOK2)
		VCBFgf_changed = TRUE;
	else
		return FALSE;
	return TRUE;
}

BOOL BRIDGE_ChangeStatusVNode(char* vnode, char* protocol, BOOL bOnOff)
{
	char request[ BIG_ENOUGH ]; 
	BOOL bOK = FALSE;
	STprintf( request, ERx( "%s.%s.gcb.%s.%s.status.%s" ), SystemCfgPrefix, host, "*", protocol, vnode);
	if (bOnOff)
		bOK = set_resource( request, ERx("ON"));
	else
		bOK = set_resource( request, ERx("OFF"));
	if (bOK)
		VCBFgf_changed = TRUE;
	else
		return FALSE;
	return TRUE;
}

void BRIDGE_protocol_list(char* net_id, CStringList& listProtocols)
{
	char *protocol;
	char* name;
	char* value;
	PM_SCAN_REC state;
	STATUS status;
	char* regexp;
	char  expbuf[ BIG_ENOUGH ];

	/* construct expbuf for gcb port id scan */
	STprintf( expbuf, ERx( "%s.%s.gcb.%s.%%.status" ), SystemCfgPrefix, host, net_id);
	regexp = PMmExpToRegExp( config_data, expbuf );
	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		/* extract protocol name */
		protocol = PMmGetElem( config_data, 4, name ); 
		if (protocol)
			listProtocols.AddTail(protocol);
	}
}

void BRIDGE_QueryVNode(CPtrList& listObj)
{
	char* name;
	char* value;
	char* protocol_status;
	char* regexp;
	char  expbuf[ BIG_ENOUGH ];
	char request[ BIG_ENOUGH ]; 

	STATUS status;
	PM_SCAN_REC state;

	/* construct expbuf for gcb port id scan */
	STprintf( expbuf, ERx( "%s.%s.gcb.%s.%%.port.%%" ), SystemCfgPrefix, host, "*" );
	regexp = PMmExpToRegExp( config_data, expbuf );

	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		char* vnode;
		char* protocol;

		/* extract protocol name */
		protocol = PMmGetElem( config_data, 4, name ); 
		/* extract vnode name */
		if( PMnumElem( name ) == 7 )
			vnode = PMmGetElem( config_data, 6, name );
		/* get status of scanned protocol and vnode*/ 
		STprintf( request, ERx( "%s.%s.gcb.%s.%s.status.%s" ), SystemCfgPrefix, host, "*", protocol, vnode );
		if( PMmGet( config_data, request, &protocol_status ) != OK)
		{
			/* default to the status of scanned protocol */ 
			STprintf( request, ERx( "%s.%s.gcb.%s.%s.status" ), SystemCfgPrefix, host, "*", protocol );
			if( PMmGet( config_data, request, &protocol_status ))
				continue;
		}

		BRIDGEPROTLINEINFO* lData = new BRIDGEPROTLINEINFO;
		_tcscpy (lData->szlisten, value);
		_tcscpy (lData->szprotocol, protocol);
		_tcscpy (lData->szstatus, protocol_status);
		_tcscpy (lData->szvnode, vnode);
		listObj.AddTail(lData);
	}
}

void
VCBF_format_port_rollups ( char *serverType, char *instance, char *countStr) 
{
    char expbuf[ BIG_ENOUGH ];
    char *regexp, *name, *value;
    char *startCntBuff;
    char *gcd_warnmsg = "Setting startup count to one";
    char *protocol;
    char *type;
    STATUS status;
    char newPort[GCC_L_PORT];
    u_i2 offset = 0;
    u_i2 portNum = 0;
    u_i2 numericPort = 0;
    u_i2 startCount = 0;
    u_i2 startOffset = 0;
    PM_SCAN_REC state;
    typedef struct
    {
        QUEUE port_q;
        char *port;
        char *protocol;
    }  PORT_QUEUE;
    PORT_QUEUE pq, *pp;
    QUEUE *q;
    
    startCntBuff = countStr;
    QUinit ( &pq.port_q );

    STprintf( expbuf, ERx( "%s.%s.%s.%s.%%.port" ),
        SystemCfgPrefix, host, serverType, instance );

    regexp = PMmExpToRegExp( config_data, expbuf );

    /*
    ** Search for items pertaining to the present server type and
    ** instance with a TCP protocol and port.
    */
    for( status = PMmScan( config_data, regexp, &state, NULL,
        &name, &value ); status == OK; status =
        PMmScan( config_data, NULL, &state, NULL, &name,
        &value ) )
    {   
        /* extract protocol name */
        protocol = PMmGetElem( config_data, 4, name );

        /* extract port */
        type = PMmGetElem( config_data, 5, name );

        if ( !STcompare(protocol, "tcp_ip" ) ||
             !STcompare(protocol, "wintcp" ) ||
             !STcompare(protocol, "tcp_wol" ) ||
             !STcompare(protocol, "tcp_dec" ))
        {
            pp = (PORT_QUEUE *)MEreqmem( 0, 
                sizeof(PORT_QUEUE), TRUE, NULL );
            pp->port = STalloc( value );
            pp->protocol = STalloc( protocol );
            QUinsert( (QUEUE *)pp, &pq.port_q );
            
            /*
            ** Check for a numeric port.
            **
            ** If input is not strictly numeric, input is 
            ** ignored.
            **
            ** Extract the numeric port and check for expected
            ** syntax: n{n}+.
            */
            numericPort = 0;
            if ( portNum == 0 )
                for( offset = 0; CMdigit( &value[offset] ); offset++ )
                    numericPort = (numericPort * 10) + (value[offset] - '0');
            /*
            ** Extract the startup count.
            */
            for( startOffset = 0, startCount = 0; 
                CMdigit( &startCntBuff[startOffset] ); startOffset++ )
            {
                startCount = (startCount * 10) + 
                    (startCntBuff[startOffset] - '0');
            }
                             
            /*
            ** If the port is explicitly numeric, the user may 
            ** not want the port number to increment.  Confirm 
            ** before proceeding.
            */
            if ( ! value[ offset ] && numericPort && startCount > 1)
            {
#ifdef OLD_CODE
                if  (CONFCH_NO == 
                    IIUFccConfirmChoice( CONF_GENERIC,
                        NULL, value, NULL, NULL,
                        S_ST06BF_multiple_das, 
                        S_ST06C0_allow_multiple,
                        S_ST06C1_disallow_multiple, NULL, 
                        TRUE ) ) 
#else
                if (QuestionMessageWithParam(S_ST06BF_multiple_das, value) == IDNO)
#endif
                {
#ifdef OLD_CODE
=>                  exec frs message :gcd_warnmsg with 
=>                      style=popup;
#else
                    mymessage(gcd_warnmsg);
#endif
                    STprintf( expbuf, 
                        ERx( "%s.%s.%sstart.%s.%s" ),
                            SystemCfgPrefix, host, 
                            SystemExecName, instance, 
                            serverType );
                    startCntBuff[0] = '1';
                    startCntBuff[1] = '\0';
                    if( set_resource( expbuf, startCntBuff ) )
                    {
#ifdef FUTURE
=>                      exec frs putrow startup_config 
=>                          components ( enabled = 
=>                          :startCntBuff);
#endif
                    }
                    startCount = 1;
                }
            }
        } /* if (!STcompare("tcp_ip"... */
    } /* for( status = PMmScan( config_data... */

    /*
    ** Go through the queue and re-write the ports
    ** with the rollup indicator if the startup count is greater
    ** than one and no indicator exists.  If the startup count
    ** is less than 2, remove the indicator if it exists.
    */
    for (q = pq.port_q.q_prev; q != &pq.port_q; 
        q = q->q_prev)
    {
        pp = (PORT_QUEUE *)q;
        for( portNum = 0, offset = 0; CMalpha( &pp->port[0] ) 
           && (CMalpha( &pp->port[1] ) || 
              CMdigit( &pp->port[1] )); )
        {
            /*
            ** A two-character symbolic port permits rollup
            ** without special formatting.
            */
            offset = 2;
            if ( ! pp->port[offset] )
                break;
    
            /*
            ** A one or two-digit base subport may be 
            ** specified.
            */
            if ( CMdigit( &pp->port[offset] ) )
            {
                portNum = (portNum * 10) + (pp->port[offset++] - '0');
    
                if ( CMdigit( &pp->port[offset] ) )
                {
                    portNum = (portNum * 10) + (pp->port[offset++] - '0');
    
                    /*
                    ** An explicit base subport must be in 
                    ** the range [0,15].  A minimum of
                    ** 14 is required to support
                    ** rollup.
                    */
                    if ( portNum > 14 )  
                    {
                        offset = 0;
                        break;
                    }
                }
            } /* if ( CMdigit ( &pp->port[offset] ) */
  
            /* 
            ** Unconditionally break from this loop.
            */
            break;

        } /* for( ; CMalpha( &pp->port[0] ) ... */
        
        /*
        ** Subport values greater than 14 cannot roll up.  Ignore them.
        */
        if ( portNum > 14 )
           continue;

        /*
        ** Check for a numeric port.
        **
        ** If input is not strictly numeric, input is 
        ** ignored.
        **
        ** Extract the numeric port and check for expected
        ** syntax: n{n}+.
        */
        numericPort = 0;
        if ( portNum == 0 )
        {
            for( offset = 0; CMdigit( &pp->port[offset] ); offset++ )
                numericPort = (numericPort * 10) + (pp->port[offset] - '0');
        }
        
        /*
        ** Ports with the format "XX" are ignored.
        */
        if ( offset < 3 && !numericPort )
            continue;

        /*
        ** Existing ports with rollups are ignored if the startup count
        ** is greater the one.
        */
        if ( startCount > 1 && ( pp->port[ offset ] == '+' ) )
            continue;

        /*
        ** If no rollup indicator is present, and no multiple startups
        ** have been specified, ignore.
        */ 
        if ( startCount < 2 && ! pp->port[ offset ] )
            continue;

        /*
        ** If multiple startups are not specified, remove the rollup
        ** indicator if it exists.
        **
        ** After this, all exceptions should have been handled and
        ** the port is re-written with the plus indicator.
        */
        if ( startCount < 2 && pp->port[ offset ] == '+' )
        {
            pp->port[ offset ] = '\0';
            STprintf( newPort, "%s", pp->port );
        }
        else 
            STprintf( newPort, "%s+", pp->port );

        STprintf( expbuf, ERx( "%s.%s.%s.%s.%s.%s" ),
            SystemCfgPrefix, host, serverType, 
                instance, pp->protocol, type );
        CRsetPMval( expbuf, newPort, change_log_fp, 
            FALSE, TRUE );
        PMmWrite ( config_data, NULL );

    } /* for ( q = pq.port... */
    
    /*
    ** De-allocate any items in the port queue.
    */
    for (q = pq.port_q.q_prev; q != &pq.port_q; 
        q = pq.port_q.q_prev)
    {
        pp = (PORT_QUEUE *)q;
        MEfree((PTR)pp->port);
        MEfree((PTR)pp->protocol);
        QUremove (q);
        MEfree((PTR)q);
    }
} 
