/*
** Copyright (c) 1992, 2009 Ingres Corporation
*/

#include <compat.h>
#ifndef VMS
#include <systypes.h>
#endif
#include <sql.h>
#include <sqlext.h>
#include <sqlstate.h>               /* ODBC driver sqlstate codes */
#include <sqlcmd.h>

#include <cm.h>
#include <cs.h>
#include <cv.h>
#include <lo.h>
#include <me.h>
#include <nm.h>  
#include <si.h>  
#include <st.h> 
#include <tr.h>
#include <er.h>
#include <erodbc.h>

#include <iiapi.h>

#include <sqlca.h>
#include <idmsxsq.h>                /* control block typedefs */
#include <idmsutil.h>               /* utility stuff */
#include "caidoopt.h"              /* connect options */
#include "idmsoinf.h"               /* information definitions */
#include "idmsoids.h"               /* string ids */
#include <idmseini.h>               /* ini file keys */
#include "idmsodbc.h"               /* ODBC driver definitions             */
#include "idmsover.h"
#include <idmsucry.h>
# ifdef NT_GENERIC
#include "idmsodlg.h"               /* ODBC driver def's */
# endif /* NT_GENERIC */
# ifndef NT_GENERIC
#include <odbccfg.h>
# endif

/*
** Name: CONNECT.C
**
** Description:
**	Connection routines for ODBC driver.
**
** History:
**	25-nov-1992 (rosda01)
**	    Initial coding
**	14-mar-1995 (rosda01)
**	    ODBC 2.0 upgrade...
**	21-jun-1996 (rosda01)
**	    Win32 port
**	09-jan-1997 (tenje01)
**	    initializing/terminating Ingres API
**	21-jan-1997 (tenje01)
**	    fixed a bug in SQLConnect 
**	23-jan-1997 (thoda04)
**	    removed warning on TRUE, FALSE redefine 
**	25-feb-1997 (thoda04)
**	    Commit catalog transaction on disconnect
**	03-mar-1997 (tenje01)
**	    SQLDriverConnect ...
**	10-mar-1997 (tenje01)
**	    chgd LPCTSTR to LPCSTR, LRESULT CurSel for 16-bit   
**	02-apr-1997 (tenje01)
**	    checked for incomplete transaction at disconnect time
**	15-apr-1997 (tenje01)
**	    put Ingres release number in pdbc->fRelease 
**	18-apr-1997 (tenje01)
**	    bug fix error 25000 at disconnect time
**	18-apr-1997 (thoda04)
**	    Added additional server classes
**	29-apr-1997 (tenje01)
**	    buf fix release number 
**	06-may-1997 (tenje01)
**	    chgd default of INI_ACCESSIBLE to false
**	08-may-1997 (thoda04)
**	    Make sure sqb.pSqlca set up on disconnect
**	08-may-1997 (thoda04)
**	    Add IIInitIngres and IITermIngres for WIN16
**	15-may-1997 (tenje01)
**	    Make vnode a combo box for driver connect 
**	30-may-1997 (thoda04)
**	    Don't zero out szPWD; it's needed for CatConnect.
**	04-jun-1997 (thoda04)
**	    WIN16 changes to vnode processing.
**	25-jul-1997 (thoda04)
**	    Don't prompt UID/PWD if fDriverCompletion == SQL_DRIVER_NOPROMPT.
**	28-jul-1997 (thoda04)
**	    Set up fServerClass in DBC  
**	12-sep-1997 (thoda04)
**	    Change serverclass DBD to STAR
**	28-oct-1997 (tenje01)
**	    SQLDriverConnect didnot override if values were not upper case
**	30-oct-1997 (tenje01)
**	    fix bug 85857
**	07-nov-1997 (tenje01)
**	    added support of role at connection time 
**	17-nov-1997 (thoda04)
**	    Clear DB_NAME_CASE at SQLDisconnect
**	04-dec-1997 (tenje01)
**	    convert C run-time functions to CL functions 
**	10-dec-1997 (thoda04)
**	    Treat RolePWD from odbc.ini as character string
**	14-jan-1998 (thoda04)
**	    Added RMS server class  
**	05-feb-1998 (thoda04)
**	    Display return code if IIapi_initialize failure  
**	17-feb-1998 (thoda04)
**	    Added cursor SQL_CONCURRENCY (fConcurrency)
**	13-apr-1998 (thoda04)
**	    Added GROUP support.
**	07-may-1998 (Dmitriy Bubis)
**	    Fix bug 90781 (Role)
**	21-jul-1998 (thoda04)
**	    Init cBindType, cKeysetSize, cRetrieveData.
**	21-dec-1998 (thoda04)
**	    Added INGDSK, MSSQL, ODBC server classes.
**	06-jan-1999 (thoda04)
**	    Moved sqldaDefault from ENV to DBC for thread-safeness.
**	29-jan-1999 (thoda04)
**	    Added clean-up of hMSDTCTransactionCompleting event
**	03-feb-1999 (thoda04)
**	    Remove unnecessary SQLCIB, SQLPIB, SQLSID
**	09-mar-1999 (thoda04)
**	    Port to UNIX
**	29-mar-1999 (thoda04)
**	    Use SYSTEM_LOCATION_VARIABLE instead of "II_SYSTEM".
**	20-jul-1999 (Bobby Ward)
**	    Fixed Bug# 97975 Outdated Help File 
**	15-nov-1999 (thoda04)
**	    Added BLANKDATE=NULL support to connection string
**	12-jan-2000 (thoda04)
**	    Added option to return date 1582-01-01 as NULL.
**	13-mar-2000 (thoda04)
**	    Add support for Ingres Read Only.
**	31-mar-2000 (thoda04)
**	    Add szDBAname, szSystemOwnername into DBC.
**	25-apr-2000 (thoda04)
**	    Add support for SelectLoops=N connect string
**	25-apr-2000 (thoda04)
**	    Make separate catalog session optional
**	28-apr-2000 (loera01)
**	    Bug 99058: substituted 120-second login 
**	    timeout for Microsoft's default of 15 sec.
**	03-may-2000 (thoda04)
**	    Initialize pdbc field in SQLCA
**	23-may-2000 (thoda04)
**	    Display user-defined server class on dialog
**	05-jun-2000 (thoda04)
**	    Added support for numeric_overflow=ignore.
**	06-jun-2000 (thoda04)
**	    Correct role password encryption
**	12-oct-2000 (thoda04)
**	    Added CATSCHEMANULL for NULL TABLE_SCHEM
**	13-nov-2000 (lunbr01)
**	    Bug 103219: Eliminate bottleneck in     
**	    SQLDriverConnect caused by calling LockEnv.
**	    Call LockDbc instead and eliminate TEMP 
**	    registry (actually static variables).
**	11-jan-2001 (thoda04)
**	    Integrate MTS/DTC support into 2.8
**	29-may-2001 (thoda04)
**	    Upgrade to 3.5 specifications
**	05-jul-2001 (somsa01)
**	    Added define of DWL_USER to DWLP_USER for
**	    IA64. Also, corrected 64-bit compiler warnings.
**	13-aug-2001 (thoda04)
**	    Add support for LONGDISPLAY option.
**	04-sep-2001 (thoda04)
**	    Add VANT (CA-Vantage) server class; treat same as VSAM.
**	17-sep-2001 (thoda04)
**	    Hide/show role, group if other product type server.
**	29-oct-2001 (thoda04)
**	    Corrections to 64-bit compiler warnings change.
**	11-dec-2001 (thoda04)
**	    Additional 64-bit changes for overloaded attributes.
**	18-jan-2002 (thoda04)
**	    Change ServerType label to Gateway if other product.
**      01-apr-2002 (loera01)
**          Renamed DB variable to DB1 to avoid compiler error on VMS, 
**          and modified SetODBCINIlocation() to work on VMS.
**	08-apr-2002 (somsa01)
**	    Completed previous change.
**      15-may-2002 (loera01)
**          Enable the read-only driver initialization.
**      27-jun-2002 (weife01) Add DB2UDB to the supported server classes.
**      30-jul-2002 (weife01) SIR #108411
**          Made change to functions SQLGetInfo and SQLGetTypeInfo to support DBMS 
**          change to allow the VARCHAR  limit to 32k; added internal function 
**          GetSQLMaxLen.
**      25-sep-2002 (loera01) Bug 108800
**          Added support for IIAPI_VERSION_3: saved environment handle from
**          IIapi_initialize() and passed it to IIapi_connect().  Invoked
**          IIapi_releaseEnv() before calling IIapi_terminate().
**      04-Nov-2002 (loera01) Bug 108536
**          Second phase of support for SQL_C_NUMERIC.  Added envHndl field
**          in the DBC control block to store the API environment handle; 
**          storing the handle in the session control block didn't work because
**          the API connection handle overrode it later when SetConnectParam()
**          was invoked.
**      14-feb-2003 (loera01) SIR 109643
**          Minimize platform-dependency in code.  Make backward-compatible
**          with earlier Ingres versions.
**      07-mar-2003 (loera01) SIR 109789
**          Change read-only mechanism.  The getReadOnly() routine now
**          determines whether or not the driver is read-only.
**      08-mar-2003 (loera01) SIR 109643
**          Conditionally compile IIAPI_RELENVPARM according to the
**          presence of the macro definition IIAPI_VERSION_2 instead of
**          IIAPI_RELENVPARM.
**     22-apr-2003 (loera01) SIR 109643
**          Added check for system-level configuration files.  Treated
**          zero-length configuration files as if they were non-existent.
**     05-jun-2003 (loera01)
**          Fixed above change to build on win32.
**      25-Jun-2003 (loera01) Bug 110448
**          For SQLConnect_InternalCall(), insert a null string into the UID 
**          and PWD fields of the connection control block if the UID and PWD 
**          arguments are zero, instead of attempting to copy zero pointers.
**     22-sep-2003 (loera01)
**          Added II_ODBC_TRACE variable to specify internal ODBC tracing.
**     20-nov-2003 (loera01)
**          Removed unused SQL_CB_PRESERVE.  Changed SQL_CB_DELETE
**          to SQL_AUTOCOMMIT_OFF and SQL_CB_CLOSE to
**          SQL_AUTOCOMMIT_ON.
**     18-dec-2003 (loera01)
**          Fixed compile problem on Linux.  Initialized segment size in
**          the API to 4096.
**     01-jul-2003 (weife01) SIR #110511
**          Add support for group ID in DSN.
**	02-oct-2003 (somsa01)
**	    Ported to NT_AMD64.
**	23-apr-2004 (somsa01)
**	    Include systypes.h to fix compile errors on HP.
**     27-apr-2004 (loera01)
**          First phase of support for SQL_BIGINT.
**     17-May-04 (loera01) Bug 112329
**          In SQLFreeEnv() and FreeEnv(), return SQL_INVALID_HANDLE if a
**          null environment handle is supplied.
**	11-Jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	17-Jun-2004 (schka24)
**	    Safer env var handling.
**      11-Nov-2004 (Ralph.Loen@ca.com) Bug 113437
**          Create the environment handle dynamically in AllocEnv() and
**          free the handle in FreeEnv(), instead of managing a static
**          storage area.
**      15-Nov-2004 (Ralph.Loen@ca.com) Bug 113462
**          The fix for bug 113437 left an unused IIODEnv global block.
**          Replace this with a new IITRACE global block.
**      01-Dec-2004 (Ralph.Loen@ca.com) Bug 113533
**          Add support for DBMS passwords in SQLDriverConnect().  The new
**          connection attribute is DBMS_PWD.
**      18-Jul-2005 (loera01) Bug 114869
**          In SQLConnect_internalCall(), replaced "ODBC_INI" literal with
**          ODBC_INI definition macro otherwise group information cannot be
**          read on non-Windows platforms.
**      18-Jan-2006 (loera01) SIR 115615
**          Removed outmoded global references.
**      10-Mar-2006 (Ralph Loen) Bug 115833
**          Added multibyteFillChar field in the DBC control block.
**      03-Jul-2006 (Ralph Loen) SIR 116326
**          Add support for SQLBrowseConnect().
**      10-Aug-2006 (Ralph Loen) SIR 116477 
**          Initialize the API at IIAPI_VERSION_5 to enable ISO dates/times.
**      10-Jan-2006 (Ralph Loen) Bug 117459 
**          Added support for the "alternate" system path for CLI applications. 
**      26-Feb-2007 (Ralph Loen) SIR 117786
**          In FreeEnv(), ignore warnings from IIapi_terminate().
**      20-Sep-2007 (weife01) SIR 119140
**          Added support for new DSN/connection string configuration
**          option:"Convert int8 from DBMS to int4".
**      19-Oct-2007 (Ralph Loen) Bug 119329
**          Added exported function getDriverVersion().
**      01-apr-2008 (weife01) Bug 120177
**          Added DSN configuration option "Allow update when use pass through
**          query in MSAccess".
**      09-Jan-2008 (Ralph Loen) SIR 119723 
**          Initialize the API at IIAPI_VERSION_6.  In AllocConnect(),
**          initialize the new fCursorType field to SQL_FORWARD_ONLY.
**      07-Feb-2008 (Ralph Loen) SIR 119723 
**          Obsoleted cRowsetSize, cBindType, cKeysetSize from tDBC.
**      12-may-2008 (Ralph Loen) Bug 120356
**          In SQLDriverConnect_InternalCall(), added connect string 
**          DEFAULTTOCHAR to treat SQL_C_DEFAULT as SQL_C_CHAR when a Unicode 
**          string is found.
**      15-may-2008 (weife01) bug 120386
**          In connect.c, add DISABLEUNDERSCORE=Y as a connection string 
**          attribute as to the DSN option.
**      17-June-2008 (Ralph Loen) Bug 120516
**          In getAltPath(), check for ODBCSYSINI before checking anything 
**          else.  The definition of ODBCSYSINI overrides other settings.
**      24-June-2008 (Ralph Loen) Bug 120551
**          Removed support for obsolete server types such as
**          ALB, OPINGDT, etc. (comments only)
**     31-Jul-2008 (Ralph Loen) Bug 120718
**          Dump #ifdef IIAPI_VERSION_2 macro.
**     15-Aug-2008 (Ralph Loen) Bug 120777
**          Simplify API initialization to use IIAPI_VERSION.
**     19-Aug-2008 (Ralph Loen) Bug 120777
**          Reverted to the previous scheme for initializing the API
**          and added IIAPI_VERSION_6.
**     02-Oct-2008 (Ralph Loen) SIR 117537
**          Initialize new max_decprec field in AllocConnect() to 31.
**     03-Nov-2008 (Ralph Loen) SIR 121152
**         Added support for II_DECIMAL in a connection string.
**         The OPT_DECIMALPT bit now indicates that support for
**         II_DECIMAL has been specified in a DSN or connection string.
**         The value of II_DECIMAL is now stored in the connection
**         handle, and is assumed to be a comma or a period.
**     04-Apr-2009 (Ralph Loen) SIR 122007
**         Add support for "StringTruncation" connection string attribute.
**         If set to Y, string truncation errors are reported.  If set to
**         N (default), strings are truncated silently.
**     15-Oct-2009 (Ralph Loen) Bug 122675 
**         Add support for is_unicode_enabled in LPDBC in AllocConnect().
**     25-Jan-2010 (Ralph Loen) Bug 123175
**         Add support for "SendDateTimeAsIngresDateTime" connection
**         string attribute.
**     08-Jan-2010 (Ralph Loen) SIR 123266
**         In AllocEnv() added API initialization of IIAPI_VERSION_7 to 
**         support IIAPI_BOOL_TYPE.
** 
*/

/*
**  The GET_WM_COMMAND_ID macro is supposed to allow the same source
**  to be used for 16 and 32 bit compiles, but the C 7.0 compiler
**  is too old to have it, so here is a the 16 bit equivalent:
*/
#if defined(_WIN64) && !defined(GET_WM_COMMAND_ID)
#define GET_WM_COMMAND_ID(wParam,lParam) ((UINT32)((WPARAM)(wParam) & 0x00000000ffffffff))
#endif

#if defined(_WIN64) && !defined(GET_WM_COMMAND_CMD)
#define GET_WM_COMMAND_CMD(wParam,lParam) ((UINT32)((WPARAM)(wParam) >> 32))
#endif

#if defined(WIN32) && !defined(GET_WM_COMMAND_ID)
#define GET_WM_COMMAND_ID(wParam,lParam) LOWORD(wParam)
#endif

#if defined(WIN32) && !defined(GET_WM_COMMAND_CMD)
#define GET_WM_COMMAND_CMD(wParam,lParam) HIWORD(wParam)
#endif

#ifdef WIN16
II_INT2 IIInitIngres(HANDLE);
void    IITermIngres(void);
#endif

#if defined(NT_IA64) || defined(NT_AMD64)
#define DWL_USER DWLP_USER
#endif  /* NT_IA64 || NT_AMD64 */

/*
**  Exported window procedures:
*/
BOOL                 ConProcDSN (SQLHWND, UINT, WPARAM, LPARAM);
BOOL                 ConProcDriver (SQLHWND, UINT, WPARAM, LPARAM);

/*
**  Internal functions:
*/
static void                CenterDialog (SQLHWND);
static BOOL                ConProcError (SQLHWND, int, ER_MSGID);
static RETCODE             ConDriverInfo (LPDBC);
static void                ConDriverName (LPDBC);
#ifdef NT_GENERIC
static VOID    HideOrShowRole(HWND hDlg, char *szServerType);
#endif
#ifndef NT_GENERIC
char *getAltPath();
char * getFileEntry(char *p, char * szToken, bool ignoreBracket);
#endif
static BOOL                FileDoesExist(char *, BOOL);
static BOOL                GetNextVNode(int,FILE**,char *);
static void         TranslatePWD(char * name, char * pwd);
static VOID concatBrowseString(char *inputString, char *attr);

/*
**  DriverConnect attribute keywords:
*/
static const CHAR DRIVER[]    = "DRIVER=";
static const CHAR DSN[]       = "DSN=";
static const CHAR VNODE[]     = "SERVER=";
static const CHAR PWD[]       = "PWD=";
static const CHAR DBMS_PWD[]  = "DBMS_PWD=";
static const CHAR TIME[]      = "TIME=";
static const CHAR UID[]       = "UID=";
static const CHAR SERVERTYPE[]= "SERVERTYPE=";
static const CHAR DATABASE[]  = "DATABASE=";
static const CHAR ROLENAME[]  = "ROLENAME=";
static const CHAR ROLEPWD[]   = "ROLEPWD=";
static const CHAR GROUP[]     = "GROUP=";
static const CHAR BLANKDATE[] = "BLANKDATE=";
static const CHAR DATE1582[]  = "DATE1582=";
static const CHAR CATCONNECT[]= "CATCONNECT=";
static const CHAR SELECTLOOPS[]="SELECTLOOPS=";
static const CHAR NUMOVERFLOW[]="NUMERIC_OVERFLOW=";
static const CHAR CATSCHEMANULL[]="CATSCHEMANULL=";
static const CHAR CONVERTINT8TOINT4[]="CONVERTINT8TOINT4=";
static const CHAR ALLOWUPDATE[]="ALLOWUPDATE=";
static const CHAR DEFAULTTOCHAR[]="DEFAULTTOCHAR=";
static const CHAR DISABLEUNDERSCORE[]="DISABLEUNDERSCORE=";
static const CHAR SUPPORTIIDECIMAL[]="SUPPORTIIDECIMAL=";
static const CHAR STRINGTRUNCATION[]="STRING_TRUNCATION=";
static const CHAR SENDDATETIMEASINGRESDATE[]="SENDDATETIMEASINGRESDATE=";

/* We support Intersolv synonym too. */
static const CHAR DATASOURCENAME[] = "DATASOURCENAME=";
static const CHAR LOGONID[]        = "LOGONID=";
static const CHAR SERVERNAME[]     = "SERVERNAME=";
static const CHAR SRVR[]           = "SRVR=";
static const CHAR DB1[]             = "DB=";
static const CHAR OPTSG[]          = "OPTS=-G";    /* don't document these */
static const CHAR OPTIONSG[]       = "Options=-G"; /* encourage use of GROUP= */

static const CHAR ERR_TITLE[] = "SQLDriverConnect Error";
static LPCSTR ServerClass[] = KEY_SERVERCLASSES;
/* {"Ingres", "DCOM (Datacom)", "IDMS", "DB2", "IMS", "VSAM", "RDB (Rdb/VMS)", 
"STAR", "RMS", "Oracle", "Informix", "Sybase", ..., NULL} */
static LPCSTR DriverName[] = KEY_DRIVERNAMES;

/*
**  Ini file connect options:
**
**  Defaults can be set in ini file,
**  can use SQLSetConnectOption or SQLSetStmtOption to override:
*/
static const char INI_AUTOCOMMIT[]    = KEY_AUTOCOMMIT;
static const char INI_COMMIT[]        = KEY_COMMIT_BEHAVIOR;
static const char INI_DESCRIPTION[]   = KEY_DESCRIPTION;
static const char INI_READONLY[]      = KEY_READONLY;
static const char INI_SERVER[]        = KEY_SERVER;
static const char INI_SERVERS[]       = KEY_SERVERS;
static const char INI_TEMP_DSN[]      = KEY_TEMP_DSN;
static const char INI_TEMP_SERVER[]   = KEY_TEMP_SERVER;
static const char DSN_DEFAULT[]       = "Default";

static const CHAR ENVID[] = "ENV*";

static       char ODBCINIfilename[MAX_LOC+1]="";

extern IITRACE IItrace;

#define ODBC_LOGIN_TIMEOUT_DEFAULT  120UL
#define VNODE_SIZE_32               376
#define VNODE_SIZE_16               372
#define VNODE_NAME_OFFSET           44
#define VNODE_VALID_OFFSET          372
#define VNODE_EMPTY_OFFSET          0

#define LEN_GATEWAY_OPTION 255

#define USE_IINODE_FILE             1
#define USE_IILOGIN_FILE            2


/*
**  SQLAllocHandle
**
**  Allocates an environment, connection, statement, or descriptor handles.
**
**  On entry:
**
**    HandleType [Input]
**      The type of handle to be allocated by SQLAllocHandle.
**      Must be one of the following values:
**        SQL_HANDLE_ENV
**        SQL_HANDLE_DBC
**        SQL_HANDLE_STMT
**        SQL_HANDLE_DESC
**    InputHandle [Input]
**      The input handle in whose context the new handle is to be allocated.
**      If HandleType is SQL_HANDLE_ENV, this is SQL_NULL_HANDLE.
**      If HandleType is SQL_HANDLE_DBC, this must be an environment handle,
**      If HandleType is SQL_HANDLE_STMT or SQL_HANDLE_DESC, 
**         InputHandle must be a connection handle.
**    OutputHandlePtr [Output]
**      Pointer to a buffer in which to return the handle to the
**      newly allocated data structure.
**
**  Returns:
**    SQL_SUCCESS
**    SQL_SUCCESS_WITH_INFO
**    SQL_INVALID_HANDLE
**    SQL_ERROR
**
**  Notes:
**
**    SQLAllocaHandle supercedes ODBC 2.x SQLAllocConnect, SQLAllocEnv, SQLAllocStmt
**
*/
SQLRETURN  SQL_API SQLAllocHandle (
    SQLSMALLINT HandleType,
    SQLHANDLE   InputHandle,
    SQLHANDLE  *OutputHandle)
{
    LPDBC       pdbc  = NULL;
    SQLRETURN   rc    = SQL_SUCCESS;

    if (HandleType==SQL_HANDLE_ENV  ||  HandleType==SQL_HANDLE_DBC)
            LockEnv (NULL);                      /* lock the environment */
    else  
    /* (HandleType==SQL_HANDLE_STMT ||  HandleType==SQL_HANDLE_DESC) */
       if (!LockDbc (pdbc = (LPDBC)InputHandle)) /* lock the connection */
            return SQL_INVALID_HANDLE;

    switch(HandleType)
       {
        case SQL_HANDLE_ENV:                          /* ENV  */
            rc = AllocEnv((LPENV*)OutputHandle);
            break;

        case SQL_HANDLE_DBC:                          /* DBC  */
            rc = AllocConnect((LPENV)InputHandle, (LPDBC*)OutputHandle);
            break;

        case SQL_HANDLE_STMT:                         /* STMT */
            rc = AllocStmt      (pdbc, (LPSTMT*)OutputHandle);
            break;

        case SQL_HANDLE_DESC:                         /* DESC */
            rc = AllocDescriptor(pdbc, (LPDESC*)OutputHandle);
            break;

        default:      /* should never happen; should be caught by DM */
            rc = SQL_ERROR;
            break;
       }  /* end switch(HandleType) */

    if (pdbc)
        UnlockDbc (pdbc);                 /* unlock the connection */
    else
        UnlockEnv (NULL);                 /* unlock the environment */

    return(rc);
}


/*
**  SQLFreeHandle
**
**  Free an environment, connection, statement, or descriptor handle.
**
**  On entry:
**
**    HandleType [Input]
**      The type of handle to be freed.
**      Must be one of the following values:
**        SQL_HANDLE_ENV
**        SQL_HANDLE_DBC
**        SQL_HANDLE_STMT
**        SQL_HANDLE_DESC
**    Handle [Input]
**
**  Returns:
**    SQL_SUCCESS
**    SQL_SUCCESS_WITH_INFO
**    SQL_INVALID_HANDLE
**    SQL_ERROR
**
*/
SQLRETURN  SQL_API SQLFreeHandle (
    SQLSMALLINT HandleType,
    SQLHANDLE   Handle)
{
    LPDBC       pdbc  = NULL;
    LPSTMT      pstmt = NULL;
    LPDESC      pdesc = NULL;
    SQLRETURN   rc    = SQL_SUCCESS;

    switch(HandleType)
       {
        case SQL_HANDLE_ENV:                           /* ENV  */
            LockEnv (NULL);                      /* lock the environment */
            rc = FreeEnv((LPENV)Handle);
            break;

        case SQL_HANDLE_DBC:                           /* DBC  */
            if (!LockDbc ((LPDBC)Handle))    /* Ensure DBC available */
                return SQL_INVALID_HANDLE;

            LockEnv (NULL);     /* lock the env-dbc chain
                                   (while holding DBC lock) */
            rc = FreeConnect((LPDBC)Handle);
                                /* disconnect DBC from ENV-DBC chain;
                                   UnlockDbc();
                                   Free memory of DBC;
                                */
            /* leave pdbc==NULL so that UnlockEnv() will be called */
            break;  /* go UnlockEnv() */

        case SQL_HANDLE_STMT:                          /* STMT */
            if (!LockStmt (pstmt = (LPSTMT)Handle)) /* validate STMT; lock DBC */
               return SQL_INVALID_HANDLE;

            pdbc  = pstmt->pdbcOwner;   /* remember the DBC to unlock later */

            rc = FreeStmt(pstmt, SQL_DROP);  /* free the STMT */
            break;

        case SQL_HANDLE_DESC:                          /* DESC */
            if (!LockDesc (pdesc = (LPDESC)Handle)) /* validate DESC; lock DBC */
               return SQL_INVALID_HANDLE;

            pdbc  = pdesc->pdbc;        /* remember the DBC to unlock later */

            rc = FreeDescriptor(pdesc);
            break;

        default:      /* should never happen; should be caught by DM */
            rc = SQL_ERROR;
            break;
       }  /* end switch(HandleType) */

    if (pdbc)
        UnlockDbc (pdbc);                 /* unlock the connection */
    else
        UnlockEnv (NULL);                 /* unlock the environment */

    return(rc);
}


/*
**  SQLAllocConnect
**
**  Allocate a data base connection (DBC) block.
**
**  On entry: phdbc -->location for connection handle.
**
**  On exit:  *phdbc-->data base connection block.
**
**  Returns:  SQL_SUCCESS
**            SQL_ERROR
**
**  Change History
**  --------------
**
**  date        programmer          description
**
**  01/10/1997  Jean Teng           Initialize API handles  
*/
RETCODE SQL_API SQLAllocConnect(
    SQLHENV    henv,
    SQLHDBC  * phdbc)
{
    LPENV   penv = (LPENV)henv;
    RETCODE rc;

    LockEnv (penv);

    rc =  AllocConnect(penv, (LPDBC*)phdbc);

    UnlockEnv (penv);

    return (rc);
}


/*
**  SQLAllocEnv
**
**  Allocate an environment (ENV) block.
**
**  On entry: phenv -->location for environment handle.
**
**  On exit:  *phenv-->environment control block.
**
**  Returns:  SQL_SUCCESS
**            SQL_ERROR
**
**  Change History
**  --------------
**
**  date        programmer          description
**
**  01/09/1997  Jean Teng           call to IIapi_initialize
**
*/

RETCODE SQL_API SQLAllocEnv(
    SQLHENV     * phenv)
{
    RETCODE   rc;

    LockEnv (NULL);                 /* lock the environment */

    rc = AllocEnv((LPENV*)phenv);   /* allocate ENV */

    UnlockEnv (NULL);               /* unlock the environment */

    return rc;
}


/*
**  SQLConnect
**
**  Connect to database.
**
**  On entry:
**
**  On exit:
**
**  Returns:  SQL_SUCCESS
**            SQL_ERROR
**
**  Change History
**  --------------
**
**  date        programmer	description
**
**  01/21/1997  Jean Teng   pdbc->cbUID/cbPWD should depend on if cbUID/cbPWD
**							is SQL_NTS, not cbDSN. 						  
*/
RETCODE SQL_API SQLConnect(
    SQLHDBC     hdbc,
    UCHAR     * szDSN,
    SWORD       cbDSN,
    UCHAR     * szUID,
    SWORD       cbUID,
    UCHAR     * szPWD,
    SWORD       cbPWD)
{
    return(SQLConnect_InternalCall
                ((LPDBC)hdbc, szDSN, cbDSN, szUID, cbUID, szPWD, cbPWD));
}


RETCODE SQL_API SQLConnect_InternalCall(
    LPDBC       pdbc,
    UCHAR     * szDSNParameter,
    SWORD       cbDSN,
    UCHAR     * szUIDParameter,
    SWORD       cbUID,
    UCHAR     * szPWDParameter,
    SWORD       cbPWD)
{
    CHAR     * szDSN = (CHAR *)szDSNParameter;
    CHAR     * szUID = (CHAR *)szUIDParameter;
    CHAR     * szPWD = (CHAR *)szPWDParameter;
    RETCODE rc;
    LPENV   penv;
    UDWORD  dw   = 0;

    if (!LockDbc (pdbc)) return SQL_INVALID_HANDLE;

    if (IItrace.fTrace >= ODBC_EX_TRACE)
        TraceOdbc (SQL_API_SQLCONNECT, pdbc, szDSN, cbDSN, szUID, cbUID, szPWD, cbPWD);

    ErrResetDbc (pdbc);        /* clear any errors on DBC */
    penv = pdbc->penvOwner;

    /*
    **  If we've been called directly by the application:
    **
    **  1. Save the connect info as null terminated strings in the DBC.
    **  2. Set the default data source name if none passed.
    **  3. Get the driver name corresponding to this data source.
    **
    **  This is already done if we've been called by SQLDriverConnect.
    */
    if (szDSN)   /* if called by user application, not SQLDriverConnect */
    {
        ResetDbc (pdbc);
        pdbc->cbDSN = (UWORD)((cbDSN == SQL_NTS) ? STlength(szDSN) : cbDSN);
        pdbc->cbDSN = (UWORD)min (sizeof (pdbc->szDSN) - 1, pdbc->cbDSN);
        MEcopy (szDSN, pdbc->cbDSN, pdbc->szDSN);

        if (szUID)
        {
            pdbc->cbUID = (UWORD)((cbUID == SQL_NTS) ? STlength(szUID) : cbUID);
            pdbc->cbUID = (UWORD)min (sizeof (pdbc->szUID) - 1, pdbc->cbUID);
            MEcopy (szUID, pdbc->cbUID, pdbc->szUID);
        }
        else
           pdbc->szUID[0] = '\0';

        if (szPWD)
        {
            pdbc->cbPWD = (UWORD)((cbPWD == SQL_NTS) ? STlength(szPWD) : cbPWD);
            pdbc->cbPWD = (UWORD)min (sizeof (pdbc->szPWD) - 1, pdbc->cbPWD);
            MEcopy (szPWD, pdbc->cbPWD, pdbc->szPWD);
        }
        else
            pdbc->szPWD[0] = '\0';

        if (*pdbc->szDSN == EOS)
	{
	    STcopy (DSN_DEFAULT, pdbc->szDSN);
        }
	
		/*
		   if SQLConnect is called by user application, then get role name
		   and pw from odbc ini file.
		*/
		if (*pdbc->szRoleName == EOS)
			SQLGetPrivateProfileString (pdbc->szDSN,"RoleName","",pdbc->szRoleName, 
			                   sizeof(pdbc->szRoleName),ODBC_INI);
		if (pdbc->cbRolePWD == 0)
            {
			SQLGetPrivateProfileString (pdbc->szDSN,"RolePWD","",pdbc->szRolePWD, 
			                   sizeof(pdbc->szRolePWD),ODBC_INI);
            if (*pdbc->szRolePWD)
               {
                TranslatePWD(pdbc->szRoleName,pdbc->szRolePWD);
                 /* Translate character string encrypted to binary encrypted */
                pwcrypt(pdbc->szRoleName, pdbc->szRolePWD); /* decrypt */
                pdbc->cbRolePWD  = (UWORD)STlength (pdbc->szRolePWD);
                pwcrypt(pdbc->szRoleName, pdbc->szRolePWD); /* encrypt */
               }
            }  /* end if pdbc->cbRolePWD == 0 */

		pdbc->cbRoleName = (UWORD)STlength (pdbc->szRoleName);

		if (*pdbc->szGroup == 0)
			SQLGetPrivateProfileString (pdbc->szDSN,"Group","",
                        pdbc->szGroup, sizeof(pdbc->szGroup),ODBC_INI);
                pdbc->cbGroup = STlength (pdbc->szGroup);
        ConDriverName (pdbc);
        rc = ConDriverInfo (pdbc);
        if (rc != SQL_SUCCESS)
        {
            UnlockDbc (pdbc);
            return rc;
        }
    }

    /*===================================================================
    **
    **  Get connection options and statement defaults:
    **
    **  ODBC specifies that options set by SQLSetConnectOption should
    **  remain in effect until the DBC is freed by SQLFreeConnect.
    **  The Driver Manager actually implements this by remembering all
    **  the connect options and reissuing the calls to the driver (us).
    **  Unfortunately the DM does this BEFORE calling our SQLConnect,
    **  so we must be a little careful about getting default values
    **  from our ini file.
    **
    **  SQLDisconnect clears most options, so we assume that a 0 value
    **  value means the option was not set by SQLSetConnectOption.
    **  Exceptions to this are noted below...
    **
    */

    /*
    **  Tracing can always be turned on from the INI file, even if
    **  turned off using SQLSetConnectOption:
    */
/*  IItrace.fTrace = UtGetIniInt (pdbc->szDSN, INI_TRACE, IItrace.fTrace); 
    if (IItrace.fTrace) IItrace.fTrace |= TRACE_ODBCCALL;
*/

    /*
    **  Unfortunately, the DM will not call our SQLTransact if
    **  it thinks Auto Commit is enabled, which is the default, so
    **  we can't use our ini file option because we have no way of
    **  communicating it to the DM.  Methinks this is an ODBC bug,
    **  since the DM is supposed to assume Manual Commit if the
    **  driver can't support Auto Commit, but it never checks.
    **  The ODBC 1.0 DM would pass SQLTransact to the driver either
    **  way, which is the way it should be.
    */
    if (!(pdbc->fOptionsSet & OPT_AUTOCOMMIT))
/*     if (UtGetIniInt (pdbc->szDSN, INI_AUTOCOMMIT, TRUE)) */
            pdbc->fOptions |= OPT_AUTOCOMMIT;

    if (*pdbc->szDSN)  /* check DSN for LONGDISPLAY option */
       {
         char    szWithOption[LEN_GATEWAY_OPTION+1];
         SQLGetPrivateProfileString (pdbc->szDSN,"WithOption","",szWithOption, 
             	               LEN_GATEWAY_OPTION + 1,ODBC_INI);
         if (*szWithOption)
            {
             if (memcmp(szWithOption,"LONGDISPLAY",11)==0  ||
                 memcmp(szWithOption,"longdisplay",11)==0)
                {
                    /* For EDS Mapper special, return longer 
                       SQL_COLUMN_DISPLAY_SIZE and default
                       AUTOCOMMIT to OFF. Disables
                       SQLTransact because DM won't call it.
                       Do not publish this special option. */
                 pdbc->fOptions    &= ~OPT_AUTOCOMMIT;  /* AutoCommit=OFF */
                 pdbc->fOptionsSet |=  OPT_AUTOCOMMIT;  /* option has been set */
                 pdbc->fOptions    |=  OPT_LONGDISPLAY;
                }
            }  /* end if (*szWithOption)  */

       }  /* end if (pdbc->szDSN) */

    /*
    **  Set connection's transaction isolation:
    **
    **  1. Get default transaction isolation for DBMS from RC file.  This is not
    **     a connect option, but is instead an information type.  We allow the
    **     user to override the default value for the DBMS in the INI file for
    **     this data source.  If this is done we make sure that the specified
    **     value is supported by the DBMS, using the isolation options information
    **     type for the DBMS.
    **
    **  2. Set connection's isolation to default unless set by SQLSetConnectOption.
    **
    **  3. Note that we cannot honor the INI file option if it is incompatible
    **     with the access mode option set by SQLSetConnectOption in a previous
    **     session on this DBC.
    */
    GetInfoBit (pdbc, SQL_DEFAULT_TXN_ISOLATION, &pdbc->fIsolationDBMS);
    GetInfoBit (pdbc, SQL_TXN_ISOLATION_OPTION,  &dw);
/*  pdbc->fIsolationDSN  = (UDWORD)UtGetIniInt (pdbc->szDSN, INI_TXNISOLATION, 0); */
    pdbc->fIsolationDSN &= dw;
    if (pdbc->fIsolationDSN == 0)
        pdbc->fIsolationDSN = pdbc->fIsolationDBMS;

    if (pdbc->fIsolation == 0)
        pdbc->fIsolation = pdbc->fIsolationDSN;

    if (pdbc->fIsolation == SQL_TXN_READ_UNCOMMITTED)
        if (!(pdbc->fOptions & OPT_READONLY))
            pdbc->fIsolation = pdbc->fIsolationDBMS;

    /*
    **  Get accessible table name from INI file unless from SQLSetConnectOption:
    **  Note that we use this by default for IDMS, others must choose it.
    */
    if (!(pdbc->fOptionsSet & OPT_ACCESSIBLE))
/*      if (UtGetIniInt (pdbc->szDSN, INI_ACCESSIBLE, FALSE))
            pdbc->fOptions |= OPT_ACCESSIBLE;
*/

    if (!(pdbc->fOptionsSet2 & OPT_CAT_TABLE))
/*      UtGetIniString (
            pdbc->szDSN, INI_CATTABLE, "", pdbc->szCatTable, sizeof(pdbc->szCatTable)); */
        STcopy("", pdbc->szCatTable);

    /*
    **  Get max rows in bulk typle buffer (used for fetch and insert)
    **  from INI file unless from SQLSetConnectOption:
    */
    if (pdbc->crowFetchMax == 0)
/*      pdbc->crowFetchMax = UtGetIniInt (pdbc->szDSN, INI_FETCH, FETROW); */
        pdbc->crowFetchMax = FETROW;

    /*
    **  Get default commit behavior from the INI file unless set by SQLSetConnectOption:
    */
    if (!(pdbc->fOptionsSet2 & OPT_COMMIT_BEHAVIOR))
    {
        pdbc->fCommit = SQL_AUTOCOMMIT_ON;   /* default is autocommit */
    }

/*
**  Enable this later, maybe...
**    
**      Get default translation DLL stuff, if any:
**    
**      (Just uses Dsname as a work area, but its gone now...)
**    
**  SQLGetPrivateProfileString (
**      pdbc->szDSN,
**      "TranslationOption",
**      "0",
**      Dsname,
**      sizeof (Dsname),
**      ODBC_INI);
**
**  pdbc->fTransOpt = strtoul (Dsname, NULL, 0);
**
**  SQLGetPrivateProfileString (
**      pdbc->szDSN,
**      "TranslationDLL",
**      "",
**      pdbc->szTransDLL,
**      sizeof (pdbc->szTransDLL),
**      ODBC_INI);
**
**  if (pdbc->szTransDLL[0])
**  {
**      retcode = LoadTransDLL (pdbc);
**      if (retcode == SQL_ERROR)
**      {
**          UnlockDbc (pdbc);
**          return (SQL_ERROR);
**      }
**  }
*/

	if (pdbc->cbPWD > 0)
	    pwcrypt(pdbc->szUID, pdbc->szPWD); /*encrpt pw */
	
    /*
    **  CONNECT via the API:
    */
    pdbc->sqb.pSqlca = &pdbc->sqlca;
    pdbc->sqb.Options = SQB_SUSPEND;    /* request piggybacked SUSPEND */
	pdbc->sqb.pDbc = pdbc;

    SqlConnect (pdbc->psqb, pdbc->szDSN);

    rc = SQLCA_ERROR (pdbc, &pdbc->sessMain);
    if (rc != SQL_ERROR)
    {
        pdbc->sessMain.fStatus |= SESS_CONNECTED | SESS_SUSPENDED;

        /*
        **  If Ingres 2.0 and the desired transaction options are the defaults
        **  avoid sending an unnecessary SET SESSION command.  See SetTransaction
        **  in transact.c for more details.
        */
        if (pdbc->fRelease >= fReleaseRELEASE20 &&
            !(pdbc->fOptions & OPT_READONLY) &&
            pdbc->fIsolation == pdbc->fIsolationDBMS)
            pdbc->sessMain.fStatus |= SESS_SESSION_SET;
    }

    UnlockDbc (pdbc);
    return rc;
}


/*
**  SQLDisconnect
**
**  Disconnect from a database.
**
**  On entry: pdbc-->connection control block.
**
**  On exit:  IDMS connection is released and all STMT's are dropped
**            unless there is an active transaction.
**
**  Returns:  SQL_SUCCESS
**            SQL_ERROR
*/

RETCODE SQL_API SQLDisconnect(
    SQLHDBC hdbc)
{
    LPDBC   pdbc = (LPDBC)hdbc;
    LPSTMT  pstmt;
    LPSTMT  pstmtNext;

    if (!LockDbc (pdbc)) return SQL_INVALID_HANDLE;

    if (IItrace.fTrace >= ODBC_EX_TRACE)
        TraceOdbc (SQL_API_SQLDISCONNECT, pdbc);

    ErrResetDbc (pdbc);        /* clear any errors on DBC */

    if (pdbc->sessMain.fStatus & SESS_COMMIT_NEEDED)
        if (!(pdbc->fOptions & OPT_LONGDISPLAY))  /* err if not EDS Mapper app */
             return ErrUnlockDbc (SQL_25000, pdbc);
	
    pdbc->sqb.pSqlca  = &pdbc->sqlca;   /* Use DBC SQLCA. */
	if (pdbc->psqb->pSession->tranHandle)
		SqlCommit(pdbc->psqb);

    #if 0                              /* maybe someday... */
    FreeTransDLL (pdbc);
    #endif

    /*
    **  Release user's session:
    */
    SqlRelease(pdbc->psqb);             /* RELEASE session. */
    if (pdbc->sqlca.SQLCODE == -6)      /* make sure its gone. */
        SqlRelease(pdbc->psqb);

    /*
    **  Release internal catalog session, if any:
    */
    if (pdbc->sessCat.fStatus & SESS_CONNECTED)
    {
        pdbc->sqb.pSession   = &pdbc->sessCat;
        if (pdbc->psqb->pSession->tranHandle)
           SqlCommit  (pdbc->psqb);
        SqlRelease (pdbc->psqb);
        if (pdbc->sqlca.SQLCODE == -6)
            SqlRelease(pdbc->psqb);
        pdbc->sqb.pSession   = &pdbc->sessMain;
        pdbc->sessCat.fStatus &= ~SESS_CONNECTED;
    }

#ifdef CATALOG_CACHE_ENABLED
    /*
    **  Free SQLTables cache, if any:
    */
    if (pdbc->fStatus & DBC_CACHE)
        CacheFree (pdbc);
#endif  /* CATALOG_CACHE_ENABLED */

    /*
    **  Free all statement blocks for this connection:
    */
    pstmt = pdbc->pstmtFirst;
    while (pstmt != NULL)
    {
        pstmt->fStatus &= ~STMT_OPEN;   /* temp... */
        pstmtNext = pstmt->pstmtNext;
        FreeStmt (pstmt, SQL_DROP);
        pstmt = pstmtNext;
    }

    /*
    **  Clear options that can be set by SQLSetConnect.
    **  The driver manager will call our SQLSetConnect function
    **  just before calling SQLConnect on this DBC to reset
    **  any connect options that the caller may have set.
    **  Also reinit connect info.
    */
    pdbc->fDriver      = 0;
    pdbc->fOptions     = 0;
    pdbc->fOptionsSet  = 0;
    pdbc->fStatus      = 0;
    pdbc->fIsolation   = 0;
    pdbc->hQuietMode   = 0;
    pdbc->cbValueMax   = 0;
    pdbc->cMaxLength   = 0;
    pdbc->crowMax      = 0;
    pdbc->crowFetchMax = 0;
    pdbc->fConcurrency = SQL_CONCUR_READ_ONLY;

    pdbc->sessMain.fStatus = 0;
    pdbc->sessCat.fStatus  = 0;

    *pdbc->szDSN    = 0;
    *pdbc->szUID    = 0;
    *pdbc->szPWD    = 0;
    *pdbc->szDBMS_PWD = 0;
    *pdbc->szDriver = 0;
    *pdbc->szRoleName = 0;
    *pdbc->szRolePWD  = 0;
    *pdbc->szGroup    = 0;
    *pdbc->szUsername = 0;
    *pdbc->szDBAname  = 0;
    *pdbc->szSystemOwnername = 0;
    pdbc->db_name_case= 0;
    pdbc->cbUID       = 0;
    pdbc->cbPWD       = 0;
    pdbc->cbDBMS_PWD  = 0;
    pdbc->cbRoleName  = 0;
    pdbc->cbRolePWD   = 0;
    pdbc->cbGroup     = 0;
    pdbc->db_table_name_length   = 0;
    pdbc->max_table_name_length  = 0;
    pdbc->max_schema_name_length = 0;
    pdbc->max_column_name_length = 0;
    pdbc->max_columns_in_table   = 0;
    pdbc->max_row_length         = 0;
    pdbc->max_char_column_length = 0; 
    pdbc->max_vchr_column_length = 0; 
    pdbc->max_byte_column_length = 0; 
    pdbc->max_vbyt_column_length = 0;
    pdbc->fRelease = 0; 

#ifdef _WIN32
    if (pdbc->hMSDTCTransactionCompleting)  /* if handle was allocated, free it  */
       { CloseHandle(pdbc->hMSDTCTransactionCompleting); /* release the handle and   */
         pdbc->hMSDTCTransactionCompleting=NULL;         /* destroy the event object */
       }
#endif

    UnlockDbc (pdbc);
    return SQL_SUCCESS;
}

/*
** Name:  SQLBrowseConnect - Perform connection attribute discovery
** 
** Description: 
**              Discover connection attributes based on input connection 
**              string.
**
** Inputs:
**              hdbc - Connection handle.
**              szConnStrIn - Attribute string.
**              cbConnStrIn - Length of attribute string.
**              cbConnStrOutMax - Maximum length of returned attribute string.
**
** Outputs: 
**              szConnStrOut - Returned attribute.
**              pcbConnStrOut - Length of returned attribute string.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_SUCCESS_WITH_INFO
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
** 
**              None.
**
** History: 
**    03-jul-06 (loera01)
**      Created.
*/ 

SQLRETURN SQL_API SQLBrowseConnect(
    SQLHDBC            hdbc,
    SQLCHAR            *szConnStrIn,
    SQLSMALLINT        cbConnStrIn,
    SQLCHAR            *szConnStrOut,
    SQLSMALLINT        cbConnStrOutMax,
    SQLSMALLINT        *pcbConnStrOut)
{
    return SQLBrowseConnect_InternalCall( hdbc,
        szConnStrIn,
        cbConnStrIn,
        szConnStrOut,
        cbConnStrOutMax,
        pcbConnStrOut);
}

SQLRETURN SQL_API SQLBrowseConnect_InternalCall(
    SQLHDBC            hdbc,
    SQLCHAR            *szConnStrIn,
    SQLSMALLINT        cbConnStrIn,
    SQLCHAR            *szConnStrOut,
    SQLSMALLINT        cbConnStrOutMax,
    SQLSMALLINT        *pcbConnStrOut)
{
    LPDBC       pdbc  = (LPDBC)hdbc;
    CHAR     *  p;
    UWORD       cb;
    char        rgbWork[800];
    BOOL        DSNFound=FALSE;
    BOOL        ServerTypeFound=FALSE;
    BOOL        ServerNameFound=FALSE;
    BOOL        DriverFound=FALSE;
    BOOL        DatabaseFound=FALSE;
    BOOL        RoleFound=FALSE;
    BOOL        GroupFound=FALSE; 
    BOOL        RPwdFound=FALSE; 
    BOOL        PWDFound=FALSE;
    BOOL        UIDFound=FALSE;
    BOOL        DBMS_PwdFound=FALSE;
    RETCODE rc = SQL_NEED_DATA, rc2;
    i4          i;

    if (!LockDbc (pdbc)) return SQL_INVALID_HANDLE;

    if (IItrace.fTrace >= ODBC_EX_TRACE)
        TraceOdbc (SQL_API_SQLBROWSECONNECT, hdbc, szConnStrIn, cbConnStrIn, szConnStrOut,
            cbConnStrOutMax, pcbConnStrOut);

    ErrResetDbc (pdbc);        /* clear any errors on DBC */
    ResetDbc (pdbc);
    
    /*
    **  Extract connect info from connect string, if any:
    */
    if (szConnStrIn)
    {
        /*
        **  Copy connect string into a work area for strtok:
        */
        cb = (UWORD)((cbConnStrIn == SQL_NTS) ? STlength ((char*)szConnStrIn) 
                                              : cbConnStrIn);
        if (cb < sizeof rgbWork)
            p = rgbWork;
        else
        {
            rc = ErrState (SQL_HY001, pdbc, F_OD0015_IDS_WORKAREA);
            UnlockDbc (pdbc);
            return rc;
        }
        MEcopy (szConnStrIn, cb, p); /* copy conn string into work area */
        p[cb] = '\0';                /* null term the work area */
        /*
        **  Parse connect string parms:
        */
        p = strtok (p, ";");

        while (p)
        {
            if (STbcompare (p, (i4)STlength(p), (char *) DSN, 
                sizeof(DSN) - 1, TRUE) == 0)
                DSNFound=TRUE;
            else
            if (STbcompare (p, (i4)STlength(p), (char *) DATASOURCENAME, 
                sizeof(DATASOURCENAME) - 1, TRUE)== 0)
                DSNFound=TRUE;
            else
            if (STbcompare (p, (i4)STlength(p), (char *) SERVERTYPE, 
                sizeof(SERVERTYPE) - 1, TRUE) == 0)
               ServerTypeFound=TRUE;
            else
            if (STbcompare (p, (i4)STlength(p), (char *) SERVERNAME, 
                sizeof(SERVERNAME) - 1, TRUE) == 0)
                ServerNameFound=TRUE;
            else
            if (STbcompare (p, (i4)STlength(p), (char *) VNODE, 
                sizeof(VNODE) - 1, TRUE) == 0)
                ServerNameFound=TRUE;
            else
            if (STbcompare (p, (i4)STlength(p), (char *) SRVR, 
                sizeof(SRVR) - 1, TRUE) == 0)
                ServerNameFound=TRUE;
            else
            if (STbcompare (p, (i4)STlength(p), (char *) DATABASE, 
                sizeof(DATABASE) - 1, TRUE) == 0)
                DatabaseFound=TRUE;
            else
            if (STbcompare (p, (i4)STlength(p), (char *) DB1, 
                sizeof(DB1) - 1, TRUE) == 0)
                DatabaseFound=TRUE;
            else
            if (STbcompare (p, (i4)STlength(p), (char *) UID, 
                sizeof(UID) - 1, TRUE) == 0)
                UIDFound=TRUE;
            else
            if (STbcompare (p, (i4)STlength(p), (char *) PWD, 
                sizeof(PWD) - 1, TRUE) == 0)
                PWDFound=TRUE;
            else
            if (STbcompare (p, (i4)STlength(p), (char *) DRIVER, 
                sizeof(DRIVER) - 1, TRUE) == 0)
                DriverFound=TRUE;
            else
            if (STbcompare (p, (i4)STlength(p), (char *) ROLENAME, 
                sizeof(ROLENAME) - 1, TRUE) == 0)
            {
                RoleFound = TRUE; 
            }
            else
            if (STbcompare (p, (i4)STlength(p), (char *) ROLEPWD, 
                sizeof(ROLEPWD) - 1, TRUE) == 0)
            {
                RPwdFound = TRUE; 
            }
            else
            if (STbcompare (p, (i4)STlength(p), (char *)GROUP, 
                sizeof(GROUP) - 1, TRUE) == 0)
            {
                GroupFound = TRUE; 
            }
            else
            if (STbcompare (p, (i4)STlength(p), (char *) DBMS_PWD, 
                sizeof(DBMS_PWD) - 1, TRUE) == 0)
                DBMS_PwdFound=TRUE;
            
            p = strtok (NULL, ";");
        }
    }

    MEcopy (szConnStrIn, cb, rgbWork); /* copy conn string into work area */
    rgbWork[cb] = '\0';                /* null term the work area */
    p = &rgbWork[cb -1];
    while (CMwhite(p)) CMprev(p, &rgbWork[STlength(rgbWork)]);
    if (*p != ';')
        STcat(rgbWork, ";");

    if (DSNFound)
    {
        if (!ServerTypeFound)
        {
            STcat(rgbWork,SERVERTYPE);
            STcat(rgbWork,"{");
            i = 0;
            while(ServerClass[i] != NULL)
            {
                STcat(rgbWork, ServerClass[i]);
                STcat(rgbWork, ",");
                i++;
            }
            p = &rgbWork[STlength(rgbWork)-1];
            *p = '}';
            STcat(rgbWork, ";");
        }
        if (!ServerNameFound)
            concatBrowseString(rgbWork, (char *)SERVERNAME);
        if (!DatabaseFound)
            concatBrowseString(rgbWork, (char *)DATABASE);
        if (!UIDFound)
            concatBrowseString(rgbWork, (char *)UID);
        if (!PWDFound)
            concatBrowseString(rgbWork, (char *)PWD);
        if (!DriverFound)
        {
            STcat(rgbWork,DRIVER);
            STcat(rgbWork,"{");
            i = 0;
            while(DriverName[i] != NULL)
            {
                STcat(rgbWork, DriverName[i]);
                STcat(rgbWork, ",");
                i++;
            }
            p = &rgbWork[STlength(rgbWork)-1];
            *p = '}';
            STcat(rgbWork, ";");
        }
        if (!RoleFound )
            concatBrowseString(rgbWork, (char *)ROLENAME);
        if (!RPwdFound )
            concatBrowseString(rgbWork, (char *)ROLEPWD);
        if (!GroupFound )
            concatBrowseString(rgbWork, (char *)GROUP);
        if (!DBMS_PwdFound)
            concatBrowseString(rgbWork, (char *)DBMS_PWD);
        if (!(DSNFound && ServerTypeFound && ServerNameFound && DriverFound 
            && DatabaseFound && RoleFound && GroupFound && RPwdFound && 
            PWDFound && UIDFound && DBMS_PwdFound))
        {
            rc2 = GetChar (NULL, rgbWork, (CHAR*)szConnStrOut, cbConnStrOutMax, 
                pcbConnStrOut);
            if (rc2 == SQL_SUCCESS_WITH_INFO)
            {
                ErrState (SQL_01004, pdbc);
                if (rc == SQL_SUCCESS) rc = rc2;
            }
            UnlockDbc (pdbc);
            return rc;
        }
    }
    UnlockDbc (pdbc);
    rc = SQLDriverConnect_InternalCall(
                hdbc,
                NULL,
                szConnStrIn,
                cbConnStrIn,
                szConnStrOut,
                cbConnStrOutMax,
                pcbConnStrOut,
                SQL_DRIVER_NOPROMPT);
    return rc;
}

/*
**  SQLDriverConnect
**
**  Connect to database:
**
**      This function as its "normal" behavior is supposed to bring up a
**      dialog box if it isn't given enough information via "szConnStrIn".  If
**      it is given enough information, it's supposed to use "szConnStrIn" to
**      establish a database connection.  In either case, it returns a
**      string to the user that is the string that was eventually used to
**      establish the connection.
**
**  On entry:
**
**  On exit:
**
**  Returns:  SQL_SUCCESS
**            SQL_ERROR
*/

RETCODE SQL_API SQLDriverConnect(
    SQLHDBC     hdbc,
    SQLHWND     hwnd,
    UCHAR     * szConnStrIn,
    SWORD       cbConnStrIn,
    UCHAR     * szConnStrOut,
    SWORD       cbConnStrOutMax,
    SWORD     * pcbConnStrOut,
    UWORD       fDriverCompletion)
{
    return SQLDriverConnect_InternalCall(
                hdbc,
                hwnd,
                szConnStrIn,
                cbConnStrIn,
                szConnStrOut,
                cbConnStrOutMax,
                pcbConnStrOut,
                fDriverCompletion);
}

RETCODE SQL_API SQLDriverConnect_InternalCall(
    SQLHDBC     hdbc,
    SQLHWND     hwnd,
    UCHAR     * szConnStrIn,
    SWORD       cbConnStrIn,
    UCHAR     * szConnStrOut,
    SWORD       cbConnStrOutMax,
    SWORD     * pcbConnStrOut,
    UWORD       fDriverCompletion)
{
    LPDBC       pdbc  = (LPDBC)hdbc;
    RETCODE     rc, rc2;
    CHAR     *  p;
    CHAR     *  q;
    UWORD       cb;
    LPENV       penv;
    BOOL        GroupFound=FALSE; /* override Group Flag */
    CHAR     *  sz      = NULL;
    BOOL        fPrompt = FALSE; /*(fDriverCompletion == SQL_DRIVER_PROMPT); */
    CHAR        szPrompt[2];       
    BOOL        fPromptUIDPWD=FALSE;
    char        rgbWork[300];
    BOOL        RoleFound=FALSE; /*override Role Flag */
    BOOL        RPwdFound=FALSE; /*override Role Password flag */
    BOOL        SrvrTypeFound=FALSE;

    if (!LockDbc (pdbc)) return SQL_INVALID_HANDLE;

    if (IItrace.fTrace >= ODBC_EX_TRACE)
        TraceOdbc (SQL_API_SQLDRIVERCONNECT, pdbc, hwnd, szConnStrIn, cbConnStrIn,
            szConnStrOut, cbConnStrOutMax, pcbConnStrOut, fDriverCompletion);

    ErrResetDbc (pdbc);        /* clear any errors on DBC */
    ResetDbc (pdbc);
    penv = pdbc->penvOwner;

    /*
    **  Extract connect info from connect string, if any:
    */
    if (szConnStrIn)
    {
        /*
        **  Copy connect string into a work area for strtok:
        */
        cb = (UWORD)((cbConnStrIn == SQL_NTS) ? STlength ((char*)szConnStrIn) 
                                              : cbConnStrIn);
        if (cb < sizeof rgbWork)
            p = rgbWork;
        else
        {
            sz = MEreqmem(0, cb + 1, TRUE, NULL);
            if (sz == NULL)
            {
                rc = ErrState (SQL_HY001, pdbc, F_OD0015_IDS_WORKAREA);
                UnlockDbc (pdbc);
                return rc;
            }
            p = sz;
        }
        MEcopy (szConnStrIn, cb, p); /* copy conn string into work area */
        p[cb] = '\0';                /* null term the work area */

        /*
        **  Parse connect string parms:
        */
        p = strtok (p, ";");

        while (p)
        {
            if (STbcompare (p, (i4)STlength(p), (char *)DRIVER, sizeof DRIVER - 1, TRUE) == 0 &&
                !*pdbc->szDriver && !*pdbc->szDSN)
            {
                pdbc->fStatus |= DBC_DRIVER;
                STlcopy (p + sizeof DRIVER, pdbc->szDriver, sizeof pdbc->szDriver - 1);
                if ((p = STindex (pdbc->szDriver, "}", 0)) != NULL) 
                   *p = EOS;
            }
            else
            if (STbcompare (p, (i4)STlength(p), (char *) DSN, sizeof DSN - 1, TRUE) == 0 &&
                !*pdbc->szDSN && !*pdbc->szDriver)
                STlcopy (p + sizeof DSN - 1, pdbc->szDSN, sizeof pdbc->szDSN - 1);
            else
            if (STbcompare (p, (i4)STlength(p), (char *) DATASOURCENAME, sizeof DATASOURCENAME - 1, TRUE) == 0 &&
                !*pdbc->szDSN && !*pdbc->szDriver)
                STlcopy (p + sizeof DATASOURCENAME - 1, pdbc->szDSN, sizeof pdbc->szDSN - 1);
            else
            if (STbcompare (p, (i4)STlength(p), (char *) SERVERTYPE, sizeof SERVERTYPE - 1, TRUE) == 0 && !*pdbc->szSERVERTYPE)
               {STlcopy (p + sizeof SERVERTYPE - 1, pdbc->szSERVERTYPE, sizeof pdbc->szSERVERTYPE - 1);
                CVupper(pdbc->szSERVERTYPE);
                SrvrTypeFound=TRUE;
               }
            else
            if (STbcompare (p, (i4)STlength(p), (char *) SERVERNAME, sizeof SERVERNAME - 1, TRUE) == 0 && !*pdbc->szVNODE)
                STlcopy (p + sizeof SERVERNAME - 1, pdbc->szVNODE, sizeof pdbc->szVNODE - 1);
            else
            if (STbcompare (p, (i4)STlength(p), (char *) VNODE, sizeof VNODE - 1, TRUE) == 0 && !*pdbc->szVNODE)
                STlcopy (p + sizeof VNODE - 1, pdbc->szVNODE, sizeof pdbc->szVNODE - 1);
            else
			if (STbcompare (p, (i4)STlength(p), (char *) SRVR, sizeof SRVR - 1, TRUE) == 0 && !*pdbc->szVNODE)
                STlcopy (p + sizeof SRVR - 1, pdbc->szVNODE, sizeof pdbc->szVNODE - 1);
            else
            if (STbcompare (p, (i4)STlength(p), (char *) PWD, sizeof PWD - 1, TRUE) == 0 && !*pdbc->szPWD)
                STlcopy (p + sizeof PWD - 1, pdbc->szPWD, sizeof pdbc->szPWD - 1);
            else
            if (STbcompare (p, (i4)STlength(p), (char *) DBMS_PWD, sizeof DBMS_PWD - 1, TRUE) == 0 && !*pdbc->szDBMS_PWD)
                STlcopy (p + sizeof DBMS_PWD - 1, pdbc->szDBMS_PWD, sizeof pdbc->szDBMS_PWD - 1);
            else
            if (STbcompare (p, (i4)STlength(p), (char *) TIME, sizeof TIME - 1,TRUE) == 0 && !*pdbc->szTIME)
                STlcopy (p + sizeof TIME - 1, pdbc->szTIME, sizeof pdbc->szTIME - 1);
            else
            if (STbcompare (p, (i4)STlength(p), (char *) UID, sizeof UID - 1, TRUE) == 0 && !*pdbc->szUID)
                STlcopy (p + sizeof UID - 1, pdbc->szUID, sizeof pdbc->szUID - 1);
			else
			if (STbcompare (p, (i4)STlength(p), (char *) LOGONID, sizeof LOGONID - 1, TRUE) == 0 && !*pdbc->szUID)
                STlcopy (p + sizeof LOGONID - 1, pdbc->szUID, sizeof pdbc->szUID - 1);
			else
			if (STbcompare (p, (i4)STlength(p), (char *) DATABASE, sizeof DATABASE - 1, TRUE) == 0 && !*pdbc->szDATABASE)
                STlcopy (p + sizeof DATABASE - 1, pdbc->szDATABASE, sizeof pdbc->szDATABASE - 1);
			else
			if (STbcompare (p, (i4)STlength(p), (char *) DB1, sizeof DB1 - 1, TRUE) == 0 && !*pdbc->szDATABASE)
                STlcopy (p + sizeof DB1 - 1, pdbc->szDATABASE, sizeof pdbc->szDATABASE - 1);
			else
 			if (STbcompare (p, (i4)STlength(p), (char *) ROLENAME, sizeof ROLENAME - 1, TRUE) == 0 && !*pdbc->szRoleName)
			{
                 STlcopy (p + sizeof ROLENAME - 1, pdbc->szRoleName, sizeof pdbc->szRoleName - 1);
				RoleFound = TRUE; /*Role has been found in the connection string */
			}
			else
			if (STbcompare (p, (i4)STlength(p), (char *) ROLEPWD, sizeof ROLEPWD - 1, TRUE) == 0 && !*pdbc->szRolePWD)
			{	
                STlcopy (p + sizeof ROLEPWD - 1, pdbc->szRolePWD, sizeof pdbc->szRolePWD - 1);
                pdbc->cbRolePWD  = (UWORD)STlength (pdbc->szRolePWD);
				RPwdFound = TRUE; /*Role PWD has been found in the connection string */
			}
			else
			if (STbcompare (p, (i4)STlength(p), (char *)GROUP, sizeof GROUP - 1, TRUE) == 0     && !*pdbc->szGroup)
                        {
                             STlcopy (p + sizeof GROUP - 1,pdbc->szGroup, sizeof pdbc->szGroup - 1);
                             pdbc->cbGroup  = STlength (pdbc->szGroup);
			     GroupFound = TRUE; /* Group ID has been found in the connection string */
                        }
			else  /* support the undocumented Intersolv format -G options */
			if (STbcompare (p, (i4)STlength(p), (char *) OPTSG, sizeof OPTSG - 1, TRUE) == 0    && !*pdbc->szGroup)
                STlcopy (p + sizeof OPTSG - 1, pdbc->szGroup, sizeof pdbc->szGroup - 1);
			else
			if (STbcompare (p, (i4)STlength(p), (char *) OPTIONSG, sizeof OPTIONSG - 1, TRUE) == 0  && !*pdbc->szGroup)
                STlcopy (p + sizeof OPTIONSG - 1, pdbc->szGroup, sizeof pdbc->szGroup - 1);
			else
			if (STbcompare (p, (i4)STlength(p), (char *) BLANKDATE, sizeof BLANKDATE - 1, TRUE) == 0  &&
			    !(pdbc->fOptionsSet & OPT_BLANKDATEisNULL))
			   {  pdbc->fOptionsSet|= OPT_BLANKDATEisNULL;
			      q = p + sizeof BLANKDATE - 1;
			      if (STbcompare (q, 4, "NULL", 4, TRUE) == 0)
			         pdbc->fOptions |= OPT_BLANKDATEisNULL;
			   }
			else
			if (STbcompare (p, (i4)STlength(p), (char *) DATE1582, sizeof DATE1582 - 1, TRUE) == 0  &&
			    !(pdbc->fOptionsSet & OPT_DATE1582isNULL))
			   {  pdbc->fOptionsSet|= OPT_DATE1582isNULL;
			      q = p + sizeof DATE1582 - 1;
			      if (STbcompare (q, 4, "NULL", 4, TRUE) == 0)
			         pdbc->fOptions |= OPT_DATE1582isNULL;
			   }
			else
			if (STbcompare (p, (i4)STlength(p), (char *) CATCONNECT, sizeof CATCONNECT - 1, TRUE) == 0  &&
			    !(pdbc->fOptionsSet & OPT_CATCONNECT))
			   {  pdbc->fOptionsSet|= OPT_CATCONNECT;
			      q = p + sizeof CATCONNECT - 1;
			      if (*q == 'Y'  ||  *q == 'y')
			         pdbc->fOptions |= OPT_CATCONNECT;
			   }
			else
			if (STbcompare (p, (i4)STlength(p), (char *) SELECTLOOPS, sizeof SELECTLOOPS - 1, TRUE) == 0  &&
			    !(pdbc->fOptionsSet & OPT_CURSOR_LOOP))
			   {  pdbc->fOptionsSet|= OPT_CURSOR_LOOP;
			      q = p + sizeof SELECTLOOPS - 1;
			      if (*q == 'N'  ||  *q == 'n')
			         pdbc->fOptions |= OPT_CURSOR_LOOP;
			   }
			else
			if (STbcompare (p, (i4)STlength(p), (char *) NUMOVERFLOW, sizeof NUMOVERFLOW - 1, TRUE) == 0  &&
			    !(pdbc->fOptionsSet & OPT_NUMOVERFLOW))
			   {  pdbc->fOptionsSet|= OPT_NUMOVERFLOW;
			      q = p + sizeof NUMOVERFLOW - 1;
			      if (*q == 'I'  ||  *q == 'i')
			         pdbc->fOptions |= OPT_NUMOVERFLOW;
			   }
			else
			if (STbcompare (p, (i4)STlength(p), (char *) STRINGTRUNCATION, sizeof STRINGTRUNCATION - 1, TRUE) == 0  &&
			    !(pdbc->fOptionsSet & OPT_STRINGTRUNCATION))
                        {  
                            pdbc->fOptionsSet|= OPT_STRINGTRUNCATION;
			    q = p + sizeof STRINGTRUNCATION - 1;
			    if (*q == 'Y'  ||  *q == 'y')
			    pdbc->fOptions |= OPT_STRINGTRUNCATION;
                        }
			else
			if (STbcompare (p, (i4)STlength(p), (char *) SENDDATETIMEASINGRESDATE, sizeof SENDDATETIMEASINGRESDATE - 1, TRUE) == 0  &&
			    !(pdbc->fOptionsSet & OPT_INGRESDATE))
                        {  
                            pdbc->fOptionsSet|= OPT_INGRESDATE;
			    q = p + sizeof SENDDATETIMEASINGRESDATE - 1;
			    if (*q == 'Y'  ||  *q == 'y')
                                pdbc->fOptions |= OPT_INGRESDATE;
                        }
			else
			if (STbcompare (p, (i4)STlength(p), (char *) CATSCHEMANULL, sizeof CATSCHEMANULL - 1, TRUE) == 0  &&
			    !(pdbc->fOptionsSet & OPT_CATSCHEMA_IS_NULL))
			   {  pdbc->fOptionsSet|= OPT_CATSCHEMA_IS_NULL;
			      q = p + sizeof CATSCHEMANULL - 1;
			      if (*q == 'Y'  ||  *q == 'y')
			         pdbc->fOptions |= OPT_CATSCHEMA_IS_NULL;
			   }
		    else
			if (STbcompare (p, (i4)STlength(p), (char *) CONVERTINT8TOINT4, sizeof CONVERTINT8TOINT4 - 1, TRUE) == 0  &&
			    !(pdbc->fOptionsSet & OPT_CONVERTINT8TOINT4))
			   {  pdbc->fOptionsSet|= OPT_CONVERTINT8TOINT4;
			      q = p + sizeof CONVERTINT8TOINT4 - 1;
			      if (*q == 'Y'  ||  *q == 'y')
			         pdbc->fOptions |= OPT_CONVERTINT8TOINT4;
			   }

		    else
			if (STbcompare (p, (i4)STlength(p), (char *) DISABLEUNDERSCORE, sizeof DISABLEUNDERSCORE - 1, TRUE) == 0  &&
			    !(pdbc->fOptionsSet & OPT_DISABLEUNDERSCORE))
			   {  pdbc->fOptionsSet|= OPT_DISABLEUNDERSCORE;
			      q = p + sizeof DISABLEUNDERSCORE - 1;
			      if (*q == 'Y'  ||  *q == 'y')
			         pdbc->fOptions |= OPT_DISABLEUNDERSCORE;
			   }

		    else
			if (STbcompare (p, (i4)STlength(p), (char *) ALLOWUPDATE, sizeof ALLOWUPDATE - 1, TRUE) == 0  &&
			    !(pdbc->fOptionsSet & OPT_ALLOWUPDATE))
			   {  pdbc->fOptionsSet|= OPT_ALLOWUPDATE;
			      q = p + sizeof ALLOWUPDATE - 1;
			      if (*q == 'Y'  ||  *q == 'y')
			         pdbc->fOptions |= OPT_ALLOWUPDATE;
			   }

		    else
			if (STbcompare (p, (i4)STlength(p), (char *) DEFAULTTOCHAR, sizeof DEFAULTTOCHAR - 1, TRUE) == 0  &&
			    !(pdbc->fOptionsSet & OPT_DEFAULTTOCHAR))
			   {  pdbc->fOptionsSet|= OPT_DEFAULTTOCHAR;
			      q = p + sizeof DEFAULTTOCHAR - 1;
			      if (*q == 'Y'  ||  *q == 'y')
			         pdbc->fOptions |= OPT_DEFAULTTOCHAR;
			   }
		    else
			if (STbcompare (p, (i4)STlength(p), (char *) SUPPORTIIDECIMAL, sizeof SUPPORTIIDECIMAL - 1, TRUE) == 0  &&
			    !(pdbc->fOptionsSet & OPT_DECIMALPT))
			   {  
                  pdbc->fOptionsSet |= OPT_DECIMALPT;
			      q = p + sizeof SUPPORTIIDECIMAL - 1;
			      if (*q == 'Y'  ||  *q == 'y')
			         pdbc->fOptions |= OPT_DECIMALPT;
			   }

            p = strtok (NULL, ";");
        }
        MEfree (sz);
    }

	if (!SrvrTypeFound)
			SQLGetPrivateProfileString (pdbc->szDSN,"ServerType","",pdbc->szSERVERTYPE, 
			                   sizeof(pdbc->szSERVERTYPE),ODBC_INI);
	if (!RoleFound)
			SQLGetPrivateProfileString (pdbc->szDSN,"RoleName","",pdbc->szRoleName, 
			                   sizeof(pdbc->szRoleName),ODBC_INI);
	if (!RPwdFound)
    {
			SQLGetPrivateProfileString (pdbc->szDSN,"RolePWD","",pdbc->szRolePWD, 
			                   sizeof(pdbc->szRolePWD),ODBC_INI);
            if (*pdbc->szRolePWD)
               {
                TranslatePWD(pdbc->szRoleName,pdbc->szRolePWD);
                 /* Translate character string encrypted to binary encrypted */
                pwcrypt(pdbc->szRoleName, pdbc->szRolePWD); /*decrypt */
                pdbc->cbRolePWD  = (UWORD)STlength (pdbc->szRolePWD);
               }
	}

    if (!GroupFound)
	SQLGetPrivateProfileString (pdbc->szDSN,"Group","",pdbc->szGroup, 
			                   sizeof(pdbc->szGroup),ODBC_INI);
		
    if (!*pdbc->szDriver && !*pdbc->szDSN)  /* if no DRIVER= and no DSN= */
    {                                       /* then use DSN=Default */
        STcopy (DSN_DEFAULT, pdbc->szDSN);
    } 

    if (!*pdbc->szDriver)
        ConDriverName (pdbc);

    rc = ConDriverInfo (pdbc);
    if (rc != SQL_SUCCESS)
    {
        UnlockDbc (pdbc);
        return rc;
    }

    /*
    **  Figure out if we need to prompt user for more info.
    **  If connecting via NOPROMPT, don't worry about UID/PWD.
    */
    if (fDriverCompletion != SQL_DRIVER_NOPROMPT)
    {
       if (pdbc->fStatus & DBC_DRIVER)
           fPromptUIDPWD = TRUE;
       else
          {
           SQLGetPrivateProfileString(pdbc->szDSN,"PromptUIDPWD",
                                      DEFAULT_PROMPTUIDPWD, szPrompt, 
                                      sizeof(szPrompt),ODBC_INI);
           if (szPrompt[0] == 'Y')
               fPromptUIDPWD = TRUE;
           else
               fPromptUIDPWD = FALSE;
          }
    }  /* endif (fDriverCompletion != SQL_DRIVER_NOPROMPT) */ 

    if ((!*pdbc->szUID) && (fPromptUIDPWD))
        fPrompt = TRUE;

    if (fDriverCompletion == SQL_DRIVER_COMPLETE)
    {
        if ((!*pdbc->szPWD) && (fPromptUIDPWD))
            fPrompt = TRUE;
    }
    if (pdbc->fStatus & DBC_DRIVER)
    {
        switch (pdbc->fDriver)
        {
        case DBC_INGRES:

            if (!*pdbc->szVNODE)
                fPrompt = TRUE;
			
			if (!*pdbc->szDATABASE)
                fPrompt = TRUE;

            break;
        }
    }
    else
    {
        if (!*pdbc->szDSN)
            fPrompt = TRUE;
    }

    /*
    **  Display dialog box to prompt user for more info if needed:
    */
    if (fPrompt)
    {
#ifdef NT_GENERIC
     BOOL (         *fp)(SQLHWND hDlg, UINT wMsg, WPARAM  wParam, LPARAM lParam);
#else
        fDriverCompletion = SQL_DRIVER_NOPROMPT;
            /* if UNIX systems, always error out with IM007 */
#endif
        if (fDriverCompletion == SQL_DRIVER_NOPROMPT)
        {
            rc = ErrState (SQL_IM007, pdbc);
            UnlockDbc (pdbc);
            return rc;
        }

#ifdef NT_GENERIC
        if (pdbc->fStatus & DBC_DRIVER)
        {
            fp = ConProcDriver;

            switch (pdbc->fDriver)
            {
            case DBC_INGRES:

                pdbc->fDlg = DLG_INGRES_DRIVER;
                break;
            }
        }
        else
        {
            fp = ConProcDSN;

            switch (pdbc->fDriver)
            {
            case DBC_INGRES:

                pdbc->fDlg = DLG_INGRES_DSN;
                break;
            }
        }

        rc = (RETCODE)DialogBoxParam (
                hModule, MAKEINTRESOURCE (pdbc->fDlg), hwnd, (DLGPROC)fp, (LPARAM)pdbc);

        if (rc == -1)
        {
            UnlockDbc (pdbc);
            return SQL_ERROR;
        }
        if (rc == 0)
        {
            UnlockDbc (pdbc);
            return SQL_NO_DATA_FOUND;
        }
#endif  /* endif NT_GENERIC */
    }  /* endif (fPrompt) */

    /*
    **  Attempt to connect:
    */
    if (pdbc->fStatus & DBC_DRIVER)
    {
        STcopy (INI_TEMP_DSN, pdbc->szDSN);
    }
    
    pdbc->cbDSN      = (UWORD)STlength (pdbc->szDSN);
    pdbc->cbUID      = (UWORD)STlength (pdbc->szUID);
    pdbc->cbPWD      = (UWORD)STlength (pdbc->szPWD);
    pdbc->cbDBMS_PWD = (UWORD)STlength (pdbc->szDBMS_PWD);
    pdbc->cbRoleName = (UWORD)STlength (pdbc->szRoleName);
    pdbc->cbGroup = (UWORD)STlength (pdbc->szGroup);

    if (pdbc->cbRolePWD > 0)
		pwcrypt(pdbc->szRoleName, pdbc->szRolePWD); /*encrypt */

    rc = SQLConnect_InternalCall (pdbc, NULL, 0, NULL, 0, NULL, 0);

    /*
    **  Rebuild connect string and return it if connect was successful:
    */
    if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
    {
        sz = rgbWork;
        if (pdbc->fStatus & DBC_DRIVER)
        {
            STcopy (DRIVER, sz);
            STcat  (sz, "{");
            STcat  (sz, pdbc->szDriver);
            STcat  (sz, "}");
		}
		else
        {
            STcopy (DSN, sz);
            STcat  (sz, pdbc->szDSN);
        }

        if (*pdbc->szVNODE)
        {
            STcat (sz, ";");
            STcat (sz, (char *) VNODE);
            STcat (sz, pdbc->szVNODE);
        }
		if (*pdbc->szDATABASE)
        {
            STcat (sz, ";");
            STcat (sz, (char *) DATABASE);
            STcat (sz, pdbc->szDATABASE);
        }
		if (*pdbc->szSERVERTYPE)
        {
            STcat (sz, ";");
            STcat (sz, (char *) SERVERTYPE);
            STcat (sz, pdbc->szSERVERTYPE);
        }
		if (*pdbc->szUID)
		{
		    STcat (sz, ";");
            STcat (sz, (char *) UID);
            STcat (sz, pdbc->szUID);
		}
        if (*pdbc->szPWD)
        {
            STcat (sz, ";");
            STcat (sz, (char *) PWD);
			pwcrypt(pdbc->szUID, pdbc->szPWD); /*decrypt  */
            STcat (sz, pdbc->szPWD);
			pwcrypt(pdbc->szUID, pdbc->szPWD); /*encrypt */
        }
        if (*pdbc->szDBMS_PWD)
        {
            STcat (sz, ";");
            STcat (sz, (char *) DBMS_PWD);
            STcat (sz, pdbc->szDBMS_PWD);
        }
		if (*pdbc->szRoleName)
		{
		    STcat (sz, ";");
            STcat (sz, (char *) ROLENAME);
            STcat (sz, pdbc->szRoleName);
		}
        if (pdbc->cbRolePWD)
        {
            STcat (sz, ";");
            STcat (sz, (char *) ROLEPWD);
			pwcrypt(pdbc->szRoleName, pdbc->szRolePWD); /*decrypt  */
            STcat (sz, pdbc->szRolePWD);
			pwcrypt(pdbc->szRoleName, pdbc->szRolePWD); /*encrypt */
        }
		if (*pdbc->szGroup)
		{
		        STcat (sz, ";");
                STcat (sz, (char *) GROUP);
                STcat (sz, pdbc->szGroup);
		}
		if (pdbc->fOptions & OPT_BLANKDATEisNULL)
		{
		        STcat (sz, ";");
		        STcat (sz, (char *) BLANKDATE);
		        STcat (sz, (char *) "NULL");
		}
		if (pdbc->fOptions & OPT_DATE1582isNULL)
		{
		        STcat (sz, ";");
		        STcat (sz, (char *) DATE1582);
		        STcat (sz, (char *) "NULL");
		}
		if (pdbc->fOptions & OPT_CURSOR_LOOP)
		{
		        STcat (sz, ";");
		        STcat (sz, (char *) SELECTLOOPS);
		        STcat (sz, (char *) "N");
		}
		if (pdbc->fOptions & OPT_CATCONNECT)
		{
		        STcat (sz, ";");
		        STcat (sz, (char *) CATCONNECT);
		        STcat (sz, (char *) "Y");
		}
		if (pdbc->fOptions & OPT_NUMOVERFLOW)
		{
		        STcat (sz, ";");
		        STcat (sz, (char *) NUMOVERFLOW);
		        STcat (sz, (char *) "IGNORE");
		}
		if (pdbc->fOptions & OPT_STRINGTRUNCATION)
		{
		        STcat (sz, ";");
		        STcat (sz, (char *) STRINGTRUNCATION);
		        STcat (sz, (char *) "Y");
		}
		if (pdbc->fOptions & OPT_INGRESDATE)
		{
		        STcat (sz, ";");
		        STcat (sz, (char *) SENDDATETIMEASINGRESDATE);
		        STcat (sz, (char *) "Y");
		}
		if (pdbc->fOptions & OPT_CATSCHEMA_IS_NULL)
		{
		        STcat (sz, ";");
		        STcat (sz, (char *) CATSCHEMANULL);
		        STcat (sz, "Y");
		}

        rc2 = GetChar (NULL, sz, (CHAR*)szConnStrOut, cbConnStrOutMax, pcbConnStrOut);
        if (rc2 == SQL_SUCCESS_WITH_INFO)
        {
            ErrState (SQL_01004, pdbc);
            if (rc == SQL_SUCCESS) rc = rc2;
        }
    }

    UnlockDbc (pdbc);
    return rc;
}


/*
**  SQLFreeConnect
**
**  Free all connection resources:
**
**  On entry: pdbc-->connection control block.
**
**  On exit:  All storage for connection is freed.
**
**  Returns:  SQL_SUCCESS
*/

RETCODE SQL_API SQLFreeConnect(
    SQLHDBC  hdbc)
{
    LPDBC    pdbc = (LPDBC)hdbc;
    RETCODE  rc;

    if (IItrace.fTrace >= ODBC_EX_TRACE)
        TraceOdbc (SQL_API_SQLFREECONNECT,pdbc);

    if (!LockDbc (pdbc))          
       return SQL_INVALID_HANDLE;
    
    rc = FreeConnect(pdbc);  /* free DBC resources and release semaphore */

    return rc;
}

/*
**  FreeConnect
**
**  Free all connection resources:
**
**  On entry: pdbc-->connection control block.  (Locked)
**
**  On exit:  All storage for connection is freed.
**
**  Returns:  SQL_SUCCESS
*/

RETCODE  FreeConnect(
    LPDBC   pdbc)
{
    LPDBC   pdbcPrior;
    LPDBC   pdbcCurrent;
    LPENV   penv = pdbc->penvOwner;

    /*
    **  Unlink DBC from ENV-->DBC chain:
    */
    if (penv->pdbcFirst == pdbc)
        penv->pdbcFirst = pdbc->pdbcNext;
    else
    {
        pdbcPrior = penv->pdbcFirst;
        while ((pdbcCurrent = pdbcPrior->pdbcNext) != pdbc)
        {
            if (pdbcCurrent == NULL)  /* should not happen */
               {
                UnlockDbc (pdbc);           /* unlock the dbc */
                return SQL_INVALID_HANDLE;
               }

            pdbcPrior = pdbcCurrent;
        }
        pdbcPrior->pdbcNext = pdbc->pdbcNext;
    }

    UnlockDbc (pdbc);               /* unlock the dbc */

    ODBCDeleteCriticalSection (&pdbc->csDbc);

    MEfree ((PTR)pdbc);

    return SQL_SUCCESS;
}


/*
**  SQLFreeEnv
**
**  Free all environment resources:
**
**  On entry:  penv-->Environment control block.
**
**  On exit:   All storage remaining is freed.
**
**  Returns:   SQL_SUCCESS.
**
**  Change History
**  --------------
**
**  date        programmer          description
**
**  01/09/1997  Jean Teng           call to IIapi_terminate
**
*/

RETCODE SQL_API SQLFreeEnv(
    SQLHENV henv)
{
    LPENV           penv = (LPENV)henv;
    SQLRETURN       rc;

    LockEnv (NULL);

    if (!penv)
        return SQL_INVALID_HANDLE;

    if ( IItrace.fTrace >= ODBC_EX_TRACE)
        TraceOdbc (SQL_API_SQLFREEENV, penv);

    rc = FreeEnv(penv);

    UnlockEnv (NULL);
    return SQL_SUCCESS;
}


/*
**  FreeEnv
**
**  Free all environment resources:
**
**  On entry:  penv-->Environment control block.
**
**  On exit:   All storage remaining is freed.
**
**  Returns:   SQL_SUCCESS.
**
**  Change History
**  --------------
**
**  date        programmer          description
**
**  01/09/1997  Jean Teng           call to IIapi_terminate
**
*/

RETCODE FreeEnv(
    LPENV penv)
{
    IIAPI_RELENVPARM relEnvParm;
    IIAPI_TERMPARM  termParm;

    if (!penv)
        return SQL_INVALID_HANDLE;

    /*
    Terminates Ingres/API
    */
    relEnvParm.re_envHandle = penv->envHndl;
    IIapi_releaseEnv(&relEnvParm);
    IIapi_terminate (&termParm);
    if (termParm.tm_status != IIAPI_ST_SUCCESS && termParm.tm_status !=
        IIAPI_ST_WARNING)
    {
        return SQL_ERROR;
    }


    MEfree ((PTR)penv);
    TermTrace();

    return SQL_SUCCESS;
}


/*
**  AllocConnect
**
**  Allocate a data base connection (DBC) block.
**
**  On entry: penv  -->ENV block
**            ppdbc -->location for connection block.
**
**  On exit:  *ppdbc-->data base connection block.
**
**  Returns:  SQL_SUCCESS
**            SQL_ERROR
**
**  Change History
**  --------------
**
**  date        programmer          description
**
**  01/10/1997  Jean Teng           Initialize API handles  
*/
RETCODE AllocConnect(LPENV  penv, LPDBC  *ppdbc)
{
    RETCODE rc;
    LPDBC   pdbc = NULL;
    LPDBC   pdbcPrior;

    ErrResetEnv (penv);

    /*
    **  Allocate DBC:
    */
    pdbc = (LPDBC)MEreqmem(0, sizeof(DBC), TRUE, NULL);
    *ppdbc = pdbc;   /* return pdbc */

    if (pdbc == NULL)
    {
        rc = ErrState (SQL_HY001, penv, F_OD0004_IDS_CONNECTION);
        return rc;
    }

    STcopy ("DBC*",     pdbc->szEyeCatcher);
    STcopy ("00000",    pdbc->szSqlState);
    STcopy ("MAINSESS*",pdbc->sessMain.szEyeCatcher);
    STcopy ("CATSESS*", pdbc->sessCat.szEyeCatcher);

    pdbc->fConcurrency = SQL_CONCUR_READ_ONLY;
    pdbc->envHndl = pdbc->sessMain.connHandle = penv->envHndl;
    /*
    **  Initialize DBC critical section in Win32:
    */
    ODBCInitializeCriticalSection (&pdbc->csDbc);

    /*
    **  Link DBC into ENV-->DBC chain:
    */
    pdbc->penvOwner = penv;

    if (penv->pdbcFirst == NULL)
        penv->pdbcFirst = pdbc;
    else
    {
        pdbcPrior = penv->pdbcFirst;
        while (pdbcPrior->pdbcNext != NULL)
            pdbcPrior = pdbcPrior->pdbcNext;
        pdbcPrior->pdbcNext = pdbc;
    }

    /*
    **  Set up blocks:
    */
    pdbc->psqb        = &pdbc->sqb;
    pdbc->sqb.pSession= &pdbc->sessMain; /* sess status, connHandle, tranH */
    pdbc->sqb.pSqlca  = &pdbc->sqlca;
    pdbc->sqb.pStmt      = NULL;

    MEcopy("SQLCA   ", 8, pdbc->sqlca.SQLCAID);
    pdbc->sqlca.pdbc =  pdbc;  /* init dbc owner of SQLCA */

    /*
    **  The following are white lies to make "JET" the ODBC engine
    **  used by ACCESS, VISUAL BASIC, and maybe even the ODBC cursor
    **  library work properly.
    */
    pdbc->cLoginTimeout = ODBC_LOGIN_TIMEOUT_DEFAULT;
    pdbc->cQueryTimeout = 15;  /* apparently not SQL_QUERY_TIMEOUT_DEFAULT */
    pdbc->cRetrieveData = SQL_RD_DEFAULT;
    pdbc->fServerClass  = fServerClassINGRES;  /* Default to Ingres server class*/
    pdbc->is_unicode_enabled = FALSE;
    pdbc->multibyteFillChar = '\0';
    STcopy(".", pdbc->szDecimal);
    pdbc->fCursorType = SQL_CURSOR_FORWARD_ONLY;

    /*
    ** Max_decprec is overridded by SQL_MAX_DECIMAL_PRECISION at connect time
    ** if this exists in iidbcapabilities.
    */
    pdbc->max_decprec = 31;

    return (SQL_SUCCESS);
}


/*
**  AllocEnv
**
**  Allocate an environment (ENV) block.
**
**  On entry: phenv -->location for environment handle.
**
**  On exit:  *phenv-->environment control block.
**
**  Returns:  SQL_SUCCESS
**            SQL_ERROR
**
**  Change History
**  --------------
**
**  date        programmer          description
**
**  01/09/1997  Jean Teng           call to IIapi_initialize
**
*/

RETCODE AllocEnv(
    LPENV * ppenv)
{
    LPENV           penv;
    WORD            trace = TRUE;
    IIAPI_INITPARM  initParm;
    II_LONG paramvalue = 4096;
    IIAPI_SETENVPRMPARM se;

    /*
    Initialize Ingres API.
    */
    initParm.in_timeout = -1;

# if defined(IIAPI_VERSION_7)
    initParm.in_version = IIAPI_VERSION_7;
# elif defined(IIAPI_VERSION_6)
    initParm.in_version = IIAPI_VERSION_6;
# elif defined(IIAPI_VERSION_5)
    initParm.in_version = IIAPI_VERSION_5;
# elif defined(IIAPI_VERSION_4)
    initParm.in_version = IIAPI_VERSION_4;
# elif defined(IIAPI_VERSION_3)
    initParm.in_version = IIAPI_VERSION_3;
# elif defined(IIAPI_VERSION_2) && !defined(IIAPI_VERSION_3)
    initParm.in_version = IIAPI_VERSION_2;
# else
    initParm.in_version = IIAPI_VERSION_1;
# endif
    IIapi_initialize (&initParm);
    if (initParm.in_status != IIAPI_ST_SUCCESS)
    {   char s[80];
        int  rc=(int)initParm.in_status;
        STprintf(s,"IIapi_initialize() return code = %d ",rc);
        if (rc==IIAPI_ST_OUT_OF_MEMORY)
           STcat(s,"(OUT OF MEMORY)"); 
#ifdef NT_GENERIC
        MessageBox(NULL, s, "ODBC/OpenApi Initialization Error", MB_ICONSTOP|MB_OK);
#endif
        *ppenv = SQL_NULL_HENV;
        return (SQL_ERROR);
    }

    /*
    **  Allocate ENV:
    */

    penv = (LPENV)MEreqmem(0, sizeof(ENV), TRUE, NULL);
    if (penv == NULL)
    {
        *((SQLINTEGER *)penv) = SQL_NULL_HENV;
        return SQL_ERROR;
    }

    STcopy (ENVID,  penv->szEyeCatcher);
    STcopy ("00000",penv->szSqlState);

    se.se_envHandle = initParm.in_envHandle;
    se.se_paramID = IIAPI_EP_MAX_SEGMENT_LEN;
    se.se_paramValue = (II_LONG *)(&paramvalue);

    IIapi_setEnvParam( &se );
    penv->envHndl = initParm.in_envHandle;
   
    InitTrace();
    *ppenv = penv;
    return SQL_SUCCESS;
}


#ifdef NT_GENERIC
/*
**  CenterDialog
**
**  Center the dialog over the parent window
**
**  On entry:   hdlg  =  dialog window handle
**
**  Returns:    Nothing.
*/

static void             CenterDialog (SQLHWND hdlg)
{
    RECT    rcParent;                   /* Parent window client rect */
    RECT    rcDlg;                      /* Dialog window rect */
    int     nLeft, nTop;                /* Top-left coordinates */
    int     cWidth, cHeight;            /* Width and height */

    /*
    ** Get frame window client rectangle in screen coordinates:
    */
    GetWindowRect (GetParent(hdlg), &rcParent);

    /*
    ** Determine the top-left point for the dialog to be centered:
    */
    GetWindowRect (hdlg, &rcDlg);
    cWidth  = rcDlg.right  - rcDlg.left;
    cHeight = rcDlg.bottom - rcDlg.top;
    nLeft   = rcParent.left +
              (((rcParent.right  - rcParent.left) - cWidth ) / 2);
    nTop    = rcParent.top  +
              (((rcParent.bottom - rcParent.top ) - cHeight) / 2);
    if (nLeft < 0) nLeft = 0;
    if (nTop  < 0) nTop  = 0;

    /*
    ** Place the dialog:
    */
    MoveWindow (hdlg, nLeft, nTop, cWidth, cHeight, TRUE);

    return;
}
#endif /* endif NT_GENERIC */


/*
**  ConDriverInfo
**
**  Get driver information.
**
**  Figure out which driver we are impersonating,
**
**  On entry:  pdbc-->DBC block with pdbc->szDriver = driver name
**
**  On exit:   pdbc->fDriver   = driver type resource id
**             pdbc->fInfo     = SQLGetInfo resource id
**             pdbc->fInfoType = SQLGetTypeInfo resource id
**             pdbc->fCatalog  = catalog functions resource id
**
**  Returns:   SQL_SUCCESS
**             SQL_ERROR    (unknown driver name)
*/
static RETCODE             ConDriverInfo (LPDBC pdbc)
{
    char szDriver[32];

    TRACE_INTL (pdbc, "ConDriverInfo");

    STcopy(pdbc->szDriver, szDriver);   /* work with driver name in upper case*/
    CVupper(szDriver);

    if (GetReadOnly(pdbc->szDSN))
    {
        pdbc->fStatus     |= DBC_READ_ONLY;
        pdbc->fOptions    |= OPT_READONLY;
        pdbc->fOptionsSet |= OPT_READONLY;
    }

    pdbc->fDriver = DBC_INGRES;
    pdbc->fInfo     = DBC_INGRES;
    pdbc->fInfoType = DBC_INGRES;
    pdbc->fCatalog  = DBC_INGRES;

    return SQL_SUCCESS;
}


/*
**  ConDriverName
**
**  Get driver name for data source from ODBC ini files:
**
**  1. First search [ODBC Data Sources] section of ODBC.INI.
**  2. If not found and data source is the default,
**     search for it in ODBCINST.INI (ODBC 1.0 might have put it there).
**
**  On entry:  pdbc-->DBC block with pdbc->szDsn = data source name
**
**  On exit:   pdbc->szDriver = driver name if found
**
**  Returns:   nothing
*/
static void             ConDriverName (LPDBC pdbc)
{
    TRACE_INTL (pdbc, "ConDriverName");

    SQLGetPrivateProfileString (
        ODBC_DSN, pdbc->szDSN, "",
        pdbc->szDriver, sizeof (pdbc->szDriver), ODBC_INI);

    return;
}

#ifdef NT_GENERIC
/*
**  ConProcDriver
**
**  Dialog Proc for SQLDriverConnect when DRIVER is specified.
**
**  On entry: hdlg   =  dialog window handle
**            wMsg   =  message id
**            wParam =  control id
**            lParam =  -->connection block
**
**  On exit:  connection block info filled in, if possible.
**
**  Returns:  TRUE
**            FALSE
*/
BOOL                 ConProcDriver(
    SQLHWND hDlg,
    UINT    wMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
    UINT    uTime = 0;
    BOOL    fTime = -1;
    LPDBC   pdbc;
    DECLTEMP (rgbTemp[LEN_TEMP])    /* Win32 only */
	LPCSTR *lpTmp;
	char   *lpsz;
	char   *szServerType;
    LRESULT CurSel;
    UINT    cb_findstringxxx;
	BOOL    bRC = TRUE;
	CHAR    szNodeName[LEN_SERVER+1];
	FILE *  fileNode = NULL;
	char   *szLOCAL = "(LOCAL)";
    int     wmId, wmEvent;

	switch (wMsg)
    {
    case WM_INITDIALOG:

//        CenterDialog (hDlg);
        SetWindowLongPtr (hDlg, GWLP_USERDATA, (LONG)lParam);
        pdbc = (LPDBC)lParam;
        szServerType = pdbc->szSERVERTYPE;
        
		SetDlgItemText (hDlg, IDS_DLG_DATABASE,    ERget(F_OD0080_IDS_DLG_DATABASE));
        SetDlgItemText (hDlg, IDS_DLG_USERID,      ERget(F_OD0082_IDS_DLG_USERID));
        SetDlgItemText (hDlg, IDS_DLG_PASSWORD,    ERget(F_OD0083_IDS_DLG_PASSWORD));
        SetDlgItemText (hDlg, IDS_DLG_ROLENAME,    ERget(F_OD0084_IDS_DLG_ROLENAME));
        SetDlgItemText (hDlg, IDS_DLG_ROLEPWD,     ERget(F_OD0085_IDS_DLG_ROLEPWD));
        SetDlgItemText (hDlg, IDS_DLG_OK,          ERget(F_OD0086_IDS_DLG_OK));
        SetDlgItemText (hDlg, IDS_DLG_CANCEL,      ERget(F_OD0087_IDS_DLG_CANCEL));
        SetDlgItemText (hDlg, IDS_DLG_HELP,        ERget(F_OD0088_IDS_DLG_HELP));
		SetDlgItemText (hDlg, IDS_DLG_SERVERNAME,  ERget(F_OD0089_IDS_DLG_SERVERNAME));
		SetDlgItemText (hDlg, IDS_DLG_SERVERTYPE,  ERget(F_OD0090_IDS_DLG_SERVERTYPE));

        /*
        **  Set initial values in dialog box controls:
        */
        STprintf (rgbTemp, ERget(F_OD0019_IDS_DRIVERCONNECT), pdbc->szDriver);
        SetWindowText (hDlg, rgbTemp);

        SetDlgItemText (hDlg, IDC_DICT, pdbc->szDATABASE);
        SendDlgItemMessage(hDlg, IDC_DICT, EM_LIMITTEXT, sizeof pdbc->szDATABASE - 1, 0L);

        SetDlgItemText (hDlg, IDC_UID, pdbc->szUID);
        SendDlgItemMessage(hDlg, IDC_UID, EM_LIMITTEXT, sizeof pdbc->szUID - 1, 0L);

        SetDlgItemText (hDlg, IDC_PWD, pdbc->szPWD);
        SendDlgItemMessage(hDlg, IDC_PWD, EM_LIMITTEXT, sizeof pdbc->szPWD - 1, 0L);

        if (pdbc->fDriver == DBC_INGRES)
        {
			SetDlgItemText (hDlg, IDC_GROUP, pdbc->szGroup);
			SendDlgItemMessage(hDlg, IDC_GROUP, EM_LIMITTEXT, sizeof pdbc->szGroup - 1, 0L);

			SetDlgItemText (hDlg, IDC_ROLENAME, pdbc->szRoleName);
			SendDlgItemMessage(hDlg, IDC_ROLENAME, EM_LIMITTEXT, sizeof pdbc->szRoleName - 1, 0L);

			SetDlgItemText (hDlg, IDC_ROLEPWD, pdbc->szRolePWD);
			SendDlgItemMessage(hDlg, IDC_ROLEPWD, EM_LIMITTEXT, sizeof pdbc->szRolePWD - 1, 0L);

			SetDlgItemText (hDlg, IDC_DBMS_PWD, pdbc->szDBMS_PWD);
			SendDlgItemMessage(hDlg, IDC_DBMS_PWD, EM_LIMITTEXT, sizeof pdbc->szDBMS_PWD - 1, 0L);

            SendDlgItemMessage(hDlg, IDC_NODE, EM_LIMITTEXT, sizeof pdbc->szVNODE - 1, 0L);

            SendDlgItemMessage(hDlg, IDC_TASK, EM_LIMITTEXT, sizeof pdbc->szSERVERTYPE - 1, 0L);
			/* fill in the vnode combo box */
			SendDlgItemMessage(hDlg, IDC_NODE, CB_ADDSTRING, (WPARAM) 0, (LPARAM) szLOCAL); 
			/* get vnode names from iinode file */
			while (bRC)
			{
				bRC = GetNextVNode(USE_IINODE_FILE, &fileNode, szNodeName);
				if (bRC)
				{
					if (szNodeName[0] != 0)
					{
		            CurSel = SendDlgItemMessage(hDlg, IDC_NODE, CB_FINDSTRINGEXACT,
			                 (WPARAM) -1, (LPARAM) (LPCSTR) szNodeName);
					if (CurSel == CB_ERR)
			            SendDlgItemMessage(hDlg, IDC_NODE, CB_ADDSTRING, (WPARAM) 0, (LPARAM) szNodeName); 
					}
				}
			}
			/* get vnode names from iilogin file */
			bRC = TRUE;
			fileNode = NULL;
			while (bRC)
			{
				bRC = GetNextVNode(USE_IILOGIN_FILE, &fileNode, szNodeName);
				if (bRC)
				{
					if (szNodeName[0] != 0)
					{
		            CurSel = SendDlgItemMessage(hDlg, IDC_NODE, CB_FINDSTRINGEXACT,
			                 (WPARAM) -1, (LPARAM) (LPCSTR) szNodeName);
					if (CurSel == CB_ERR)
			            SendDlgItemMessage(hDlg, IDC_NODE, CB_ADDSTRING, (WPARAM) 0, (LPARAM) szNodeName); 
					}
				}
			}
			CurSel = SendDlgItemMessage(hDlg, IDC_NODE, CB_FINDSTRINGEXACT,
				(WPARAM) -1, (LPARAM) (LPCSTR) pdbc->szVNODE);
			if (CurSel == CB_ERR)
				CurSel = 0;
			SendDlgItemMessage(hDlg, IDC_NODE, CB_SETCURSEL, (WPARAM) CurSel, (LPARAM) 0);

			/* fill the server type combo box */
			lpTmp = ServerClass;
		    while (*lpTmp)
		    {
			    SendDlgItemMessage(hDlg, IDC_SERVERTYPE, CB_ADDSTRING, (WPARAM) 0, (LPARAM) *lpTmp); 
			    lpTmp++;
		    }
		    if (STcompare(szServerType, "ALB")     == 0  ||  /* if one of the server */
		        STcompare(szServerType, "DCOM")    == 0  ||  /* in the list with     */
		        STcompare(szServerType, "INGDSK")  == 0  ||  /* a comment following  */
		        STcompare(szServerType, "OPINGDT") == 0  ||  /* then search using    */
		        STcompare(szServerType, "MSSQL")   == 0  ||  /* prefix as a special  */
		        STcompare(szServerType, "RDB")     == 0  ||  /* case.                */
		        STcompare(szServerType, "VANT")    == 0   )
		           cb_findstringxxx = CB_FINDSTRING;       /* search using prefix */
		    else   cb_findstringxxx = CB_FINDSTRINGEXACT;  /* search using exact  */
		    CurSel = SendDlgItemMessage(hDlg, IDC_SERVERTYPE, cb_findstringxxx,
		                  (WPARAM) -1, (LPARAM) (LPCSTR) szServerType);
		    if (CurSel == CB_ERR)
		       if (*szServerType == '\0')  /* if no server class then default*/
		        CurSel = SendDlgItemMessage(hDlg, IDC_SERVERTYPE, CB_FINDSTRINGEXACT,
		                  (WPARAM) -1, (LPARAM) (LPCSTR) SERVER_TYPE_DEFAULT);
		       else  /* add user defined class like INGBATCH */
		        CurSel = SendDlgItemMessage(hDlg, IDC_SERVERTYPE, CB_ADDSTRING,
		                  (WPARAM) 0, (LPARAM) szServerType);
		    if (CurSel != CB_ERR)
		        SendDlgItemMessage(hDlg, IDC_SERVERTYPE, CB_SETCURSEL,
		                  (WPARAM) CurSel, (LPARAM) 0);

			/* FIXME future implementation 
            uTime = (UINT)atoi (pdbc->szTIME);
            if (uTime < 1 || uTime > 255) uTime = 10;
            SetDlgItemInt (hDlg, IDC_TIME, uTime, FALSE);
            SendDlgItemMessage(hDlg, IDC_TIME, EM_LIMITTEXT, 3, 0L);
			*/
        }

        HideOrShowRole(hDlg, szServerType);
           /* hide or show rolename control based on server class */

        if (*pdbc->szDATABASE != '\0')   /* if database name is filled in */
           {
            HWND hwnd = GetDlgItem(hDlg, IDC_UID);  /* set input focus to UID */
            SetFocus(hwnd);
            return FALSE;
           }

        return TRUE;

    case WM_COMMAND:

        wmId    = GET_WM_COMMAND_ID (wParam, lParam);
        wmEvent = GET_WM_COMMAND_CMD(wParam, lParam);

        switch (wmId)
        {
        case IDC_SERVERTYPE :

            switch (wmEvent)
            {
            char szServerType[LEN_SERVERTYPE + 1] = "";

            case CBN_SELCHANGE:   /* user has made his selection */
            case CBN_EDITCHANGE:  /* user has altered text */
                if (wmEvent == CBN_SELCHANGE)
                   {LRESULT  CurSel;
                    CurSel = SendDlgItemMessage(hDlg, IDC_SERVERTYPE,
                                CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                    if (CurSel != CB_ERR)
                             SendDlgItemMessage(hDlg, IDC_SERVERTYPE,
                                CB_GETLBTEXT, (WPARAM) CurSel, (LPARAM) szServerType);
                   }
                else /* wmEvent == CBN_EDITCHANGE */
                    GetDlgItemText (hDlg, IDC_SERVERTYPE, szServerType, LEN_SERVERTYPE + 1);
                HideOrShowRole(hDlg, szServerType);

                return FALSE;
            }
            break;

        case IDOK:

            pdbc = (LPDBC)GetWindowLongPtr(hDlg, GWLP_USERDATA);

            GetDlgItemText (hDlg, IDC_DICT, pdbc->szDATABASE, sizeof pdbc->szDATABASE);
            GetDlgItemText (hDlg, IDC_UID, pdbc->szUID, sizeof pdbc->szUID);
            GetDlgItemText (hDlg, IDC_PWD, pdbc->szPWD, sizeof pdbc->szPWD);

            if (pdbc->fDriver == DBC_INGRES)
            {
				GetDlgItemText (hDlg, IDC_GROUP,    pdbc->szGroup,    sizeof pdbc->szGroup);
				GetDlgItemText (hDlg, IDC_ROLENAME, pdbc->szRoleName, sizeof pdbc->szRoleName);
				GetDlgItemText (hDlg, IDC_ROLEPWD,  pdbc->szRolePWD,  sizeof pdbc->szRolePWD);
                pdbc->cbRolePWD  = (UWORD)STlength (pdbc->szRolePWD);
				GetDlgItemText (hDlg, IDC_DBMS_PWD,  pdbc->szDBMS_PWD,  sizeof pdbc->szDBMS_PWD);
                pdbc->cbDBMS_PWD  = (UWORD)STlength (pdbc->szDBMS_PWD);
                GetDlgItemText (hDlg, IDC_NODE, pdbc->szVNODE, sizeof pdbc->szVNODE);
                GetDlgItemText (hDlg, IDC_SERVERTYPE, pdbc->szSERVERTYPE, sizeof pdbc->szSERVERTYPE);
                CVupper(pdbc->szSERVERTYPE);

                if ((lpsz = STindex(pdbc->szSERVERTYPE," ",0)) != NULL)
                   	CMcpychar("\0",lpsz);  /* strip any doc from servertype */

				/* FIXME future implementation 
                uTime = GetDlgItemInt (hDlg, IDC_TIME, &fTime, FALSE);
				*/
            }

            /*
            **  Make a stab at validation:
            */
		    if (!*pdbc->szDATABASE)
                 return ConProcError (hDlg, IDC_DICT, F_OD0020_IDS_DRV_DATABASE);

            switch (pdbc->fDriver)
            {
            case DBC_INGRES:

                if (!*pdbc->szVNODE)
                    return ConProcError (hDlg, IDC_NODE, F_OD0023_IDS_DRV_NODE);
				/* FIXME
                if (!fTime || uTime > 255)
                    return ConProcError (hDlg, IDC_TIME, IDS_DRV_TIMEOUT);
				*/
            }
			/*
            if (!*pdbc->szUID)
                return ConProcError (hDlg, IDC_UID, IDS_DRV_USERID);
			*/
        case IDCANCEL:

            EndDialog (hDlg, GET_WM_COMMAND_ID(wParam, lParam) == IDOK);
            WinHelp (hDlg, szIdmsHelp, HELP_QUIT, 0L);
            return (TRUE);

        case IDC_HELP:

            pdbc = (LPDBC)GetWindowLongPtr(hDlg, GWLP_USERDATA);
            WinHelp (hDlg, szIdmsHelp, HELP_CONTENTS, (DWORD)0);
            return (TRUE);
        }
    }
    return (FALSE);
}


/*
**  ConProcDSN
**
**  Dialog Proc for SQLDriverConnect when DRIVER is not specified.
**
**  On entry: hdlg   =  dialog window handle
**            wMsg   =  message id
**            wParam =  control id
**            lParam =  -->connection block
**
**  On exit:  connection block info filled in, if possible.
**
**  Returns:  TRUE
**            FALSE
*/
BOOL                 ConProcDSN(
    SQLHWND hDlg,
    UINT    wMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
    LPDBC   pdbc;
    DECLTEMP (rgbTemp[LEN_TEMP])    /* Win32 only */
    
    switch (wMsg)
    {
    case WM_INITDIALOG:

        if (InSendMessage())
            SetSysModalWindow (hDlg);

//        CenterDialog (hDlg);
        SetWindowLongPtr (hDlg, GWLP_USERDATA, (LONG)lParam);
        pdbc = (LPDBC)lParam;

        SetDlgItemText (hDlg, IDS_DLG_DATASOURCE,    ERget(F_OD0081_IDS_DLG_DATASOURCE));
        SetDlgItemText (hDlg, IDS_DLG_USERID,        ERget(F_OD0082_IDS_DLG_USERID));
        SetDlgItemText (hDlg, IDS_DLG_PASSWORD,      ERget(F_OD0083_IDS_DLG_PASSWORD));
        SetDlgItemText (hDlg, IDS_DLG_ROLENAME,      ERget(F_OD0084_IDS_DLG_ROLENAME));
        SetDlgItemText (hDlg, IDS_DLG_ROLEPWD,       ERget(F_OD0085_IDS_DLG_ROLEPWD));
        SetDlgItemText (hDlg, IDS_DLG_OK,            ERget(F_OD0086_IDS_DLG_OK));
        SetDlgItemText (hDlg, IDS_DLG_CANCEL,        ERget(F_OD0087_IDS_DLG_CANCEL));
        SetDlgItemText (hDlg, IDS_DLG_HELP,          ERget(F_OD0088_IDS_DLG_HELP));

		/*
        **  Set initial values in dialog box controls:
        */
        STprintf (rgbTemp, ERget(F_OD0019_IDS_DRIVERCONNECT), pdbc->szDriver);
        SetWindowText (hDlg, rgbTemp);

        SetDlgItemText (hDlg, IDC_DSN, pdbc->szDSN);

        SetDlgItemText (hDlg, IDC_UID, pdbc->szUID);
        SendDlgItemMessage(hDlg, IDC_UID, EM_LIMITTEXT, sizeof (pdbc->szUID) - 1, 0L);

        SetDlgItemText (hDlg, IDC_PWD, pdbc->szPWD);
        SendDlgItemMessage(hDlg, IDC_PWD, EM_LIMITTEXT, sizeof (pdbc->szPWD) - 1, 0L);

        SetDlgItemText (hDlg, IDC_ROLENAME, pdbc->szRoleName);
        SendDlgItemMessage(hDlg, IDC_ROLENAME, EM_LIMITTEXT, sizeof (pdbc->szRoleName) - 1, 0L);

        SetDlgItemText (hDlg, IDC_ROLEPWD, pdbc->szRolePWD);
        SendDlgItemMessage(hDlg, IDC_ROLEPWD, EM_LIMITTEXT, (sizeof(pdbc->szRolePWD) - 1)/2, 0L);

        SetDlgItemText (hDlg, IDC_GROUP, pdbc->szGroup);
        SendDlgItemMessage(hDlg, IDC_GROUP, EM_LIMITTEXT, sizeof (pdbc->szGroup) - 1, 0L);

        HideOrShowRole(hDlg, pdbc->szSERVERTYPE);
           /* hide or show rolename control based on server class */

        if (*pdbc->szDSN != '\0')   /* if DSN is filled in */
           {
            HWND hwnd = GetDlgItem(hDlg, IDC_UID);  /* set input focus to UID */
            SetFocus(hwnd);
            return FALSE;
           }

        return TRUE;

    case WM_COMMAND:

        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDOK:

            pdbc = (LPDBC)GetWindowLongPtr(hDlg, GWLP_USERDATA);

            GetDlgItemText (hDlg, IDC_DSN, pdbc->szDSN, sizeof pdbc->szDSN);
            GetDlgItemText (hDlg, IDC_UID, pdbc->szUID, sizeof pdbc->szUID);
            GetDlgItemText (hDlg, IDC_PWD, pdbc->szPWD, sizeof pdbc->szPWD);
            GetDlgItemText (hDlg, IDC_ROLENAME, pdbc->szRoleName, sizeof pdbc->szRoleName);
            GetDlgItemText (hDlg, IDC_ROLEPWD,  pdbc->szRolePWD,  sizeof pdbc->szRolePWD);
            pdbc->cbRolePWD  = (UWORD)STlength (pdbc->szRolePWD);
            GetDlgItemText (hDlg, IDC_GROUP,    pdbc->szGroup,    sizeof pdbc->szGroup);
            GetDlgItemText (hDlg, IDC_DBMS_PWD,    pdbc->szDBMS_PWD,    sizeof pdbc->szDBMS_PWD);

            if (!*pdbc->szDSN)
                return ConProcError (hDlg, IDC_DSN, F_OD0021_IDS_DRV_DATASOURCE);
			/*
            if (!*pdbc->szUID)
                return ConProcError (hDlg, IDC_UID, IDS_DRV_USERID);
			*/
        case IDCANCEL:

            EndDialog (hDlg, GET_WM_COMMAND_ID(wParam, lParam) == IDOK);
            WinHelp (hDlg, szIdmsHelp, HELP_QUIT, 0L);
            return (TRUE);

        case IDC_HELP:

            pdbc = (LPDBC)GetWindowLongPtr(hDlg, GWLP_USERDATA);
            WinHelp (hDlg, szIdmsHelp, HELP_CONTENTS, (DWORD)0);
            return (TRUE);
        }
    }
    return (FALSE);
}
#endif  /* endif NT_GENERIC */


/*
**  ConProcError
**
**  Display error message and postion cursor to control in error.
**
**  On entry: hdlg   =  dialog window handle
**            iMsg   =  message id
**            iCtl   =  control id
**
**  Returns:  TRUE
*/
static BOOL             ConProcError (
    SQLHWND hDlg,
    int     iCtl,
    ER_MSGID iMsg)
{
	CHAR     * lpTitle = ERget(F_OD0094_IDS_DRVRCONNECT_ERROR); 

#ifdef NT_GENERIC
    MessageBox (hDlg, ERget(iMsg), lpTitle, MB_OK | MB_ICONSTOP);
    SetFocus (GetDlgItem (hDlg, iCtl));
#endif
    return TRUE;
}


/*
**  ResetDbc
**
**  Initialize selected fields in DBC:
**
**  On entry:  pdbc-->Data Base Connection block
**
**  Returns:   nothing.
*/

VOID ResetDbc (LPDBC pdbc)
{
    TRACE_INTL (pdbc, "ResetDbc");

    pdbc->fStatus = 0;
    pdbc->sessMain.fStatus = 0;
    pdbc->sessCat.fStatus  = 0;
    pdbc->fServerClass = fServerClassINGRES;  /* Default back to Ingres server class*/

    MEfill (sizeof pdbc->szDriver, 0, pdbc->szDriver);
    MEfill (sizeof pdbc->szDSN, 0, pdbc->szDSN);
    MEfill (sizeof pdbc->szUID, 0, pdbc->szUID);
    MEfill (sizeof pdbc->szPWD, 0, pdbc->szPWD);

    MEfill (sizeof pdbc->szTIME, 0, pdbc->szTIME);
    MEfill (sizeof pdbc->szVNODE, 0, pdbc->szVNODE);
    MEfill (sizeof pdbc->szDATABASE, 0, pdbc->szDATABASE);
    MEfill (sizeof pdbc->szSERVERTYPE, 0, pdbc->szSERVERTYPE);

    MEfill (sizeof pdbc->szRoleName, 0, pdbc->szRoleName);
    MEfill (sizeof pdbc->szRolePWD, 0, pdbc->szRolePWD);
    MEfill (sizeof pdbc->szGroup,   0, pdbc->szGroup);
    return;
}
/*--------------------------------------------------------------*/
static BOOL             FixComputerName(char * szPath)
{
char * lpChar = szPath;
char * lpSave = NULL; 

	if (lpChar)
	{
		/*
		lpChar = lpChar + STlength(szPath) - 1;
		while (*lpChar != '\\')
		{
			if (*lpChar == ' ')
				*lpChar = '_';
			lpChar--;
		}
		*/
		while(*lpChar != EOS)       /* find the last occurrence of back slash */
		{
			if (! CMcmpcase(lpChar,"\\") )
				lpSave = lpChar;
			CMnext(lpChar);
		}
		if (lpSave)
        {
			lpChar = lpSave;
			while(*lpChar != EOS)   /* replace space by underscore */       
		    {
				if (CMspace(lpChar))
					CMcpychar("_",lpChar); 
				CMnext(lpChar);
			}
        }
	}
	return(TRUE);
}

/*
**  FileDoesExist
**
**  Check for the existence of a file, and, optionally, whether or not the file
**      has a size greater than zero. 
**
**  On entry: szPath -->location for environment handle.
**            check_size -->whether or not to check the size.
**
**  Returns:  TRUE if the file exists, FALSE otherwise.
**
**  History:
**
**  22-Apr-2003 (loera01) SIR 109643
**      Added description and history.  Added check for file size.      
**
*/
static
BOOL FileDoesExist(char * szPath, BOOL check_size)
{
    LOCATION      loc;
    LOINFORMATION locinfo;
    i4            flags = 0;

    if ( check_size)
        flags |= LO_I_TYPE | LO_I_SIZE;
    else
        flags |= LO_I_TYPE;

    if (LOfroms(PATH & FILENAME, szPath, &loc) == OK)
       if (LOinfo(&loc, &flags, &locinfo) == OK)
           if ((flags & LO_I_TYPE)  &&  locinfo.li_type == LO_IS_FILE)
           {
               if (!check_size)
                   return(TRUE);
               else if ((flags & LO_I_SIZE) && locinfo.li_size)
                   return(TRUE);
           }
    return(FALSE);
}

/*--------------------------------------------------------------*/
static BOOL             GetNextVNode(int      NodeLogin, 
                                     FILE * * ppFile,
                                     char *   pNodeName)
{ 
char *   lpIngDir;
char     szPath[MAX_LOC];
FILE *   pfile;
LOCATION loc;
STATUS   status;
i4       actual_count;
char     locbuf[MAX_LOC + 1];
 
#ifdef WIN32
#define IINODE  "IINODE_"
#define IILOGIN "IILOGIN_"
char   buff[VNODE_SIZE_32]; /* size of each vnode is 376 in Ingres */
char   szComputerName[MAX_COMPUTERNAME_LENGTH+1];
DWORD  dwSize=MAX_COMPUTERNAME_LENGTH+1;
#else
#define IINODE  "iinode"
#define IILOGIN "iilogin"
char   szComputerName[1]="";
char   buff[VNODE_SIZE_16]; /* size of each vnode is 128 in ingres */
#endif

     MEfill(LEN_SERVER+1, 0, pNodeName);
 
     pfile = *ppFile; 

     if (pfile == NULL)    /* if the first time, open file */
     {
	 STcopy("C:", szPath);
         NMgtAt(SYSTEM_LOCATION_VARIABLE,&lpIngDir);  /* usually II_SYSTEM */
         if (lpIngDir != NULL)
             STlcopy(lpIngDir,szPath,sizeof(szPath)-25-1);
         STcat(szPath,"\\");
         STcat(szPath,SYSTEM_LOCATION_SUBDIRECTORY);  /* usually "ingres"  */
         STcat(szPath,"\\files\\name\\");
#ifdef WIN32
         if (!GetComputerName(szComputerName,&dwSize))
             return(FALSE);
#else 
         STcat(szPath,"\\files\\name\\");   
#endif
         if (NodeLogin == USE_IINODE_FILE)
            {
             STcat(szPath,IINODE);
            }
         else  /* NodeLogin == USE_IILOGIN_FILE */
            {
             STcat(szPath,IILOGIN);
            }
         STcat(szPath,szComputerName);

         STcopy(szPath, locbuf);    /* set up LOCATION block for file */
         if (LOfroms(PATH & FILENAME, locbuf, &loc) != OK)
            return(FALSE);

#ifdef WIN32 
         if (!FileDoesExist(szPath, FALSE))
		 {
			 /* 
			 if computer name contains space, Ingres 2.0 changes the
			 space to '_'. Prior to 2.0, it would just leave the space 
			 alone. So, if open fails, we will replace space by '_', then 
			 try again. 
			 */
			 FixComputerName(szPath);
             STcopy(szPath, locbuf);    /* set up LOCATION block for file */
             if (LOfroms(PATH & FILENAME, locbuf, &loc) != OK)
                return(FALSE);
		 }
#endif
         if ((status = SIfopen(&loc, ERx("r"), (i4)SI_BIN,  
                                     sizeof(buff), &pfile)) != OK)
             return(FALSE);
 
		 *ppFile = pfile;    /* return file handle to caller for next read */
     }  /* endif first time, opening file */
	 
     if (SIread (pfile, (i4)sizeof(buff), &actual_count, buff) != OK)
	 {
         SIclose(pfile);
          /*  Note: if this SIclose GPFs on WIN32, the reason is that different
              MSVCRT.DLL and MSVCRTD.DLL are being mixed up.  The OICLFNT.DLL
              is probably calling fopen() in MSVCRT.DLL and this program is
              probably calling fclose() in MSVCRTD.DLL since SIclose is #defined
              to fclose.  The release version of fopen is not compatible with
              the debug version of fclose.  The internal structures are different.
              If compiling this under Visual C, always use MSVCRT.DLL (even under
              Debug builds):
                project/settings/"C/C++"/Code Generation
                  Use run-time library: Multithread DLL */
         return(FALSE);       
	 }

#ifdef WIN32
     if ((buff[VNODE_VALID_OFFSET] == EOS) &&  /* vnode is valid, not destroyed */
         (buff[VNODE_EMPTY_OFFSET] == EOS))    /* this record is valid, not an empty slot. */
     {
          STcopy(&buff[VNODE_NAME_OFFSET],pNodeName);
     }
#else 
	      STcopy(&buff[VNODE_NAME_OFFSET],pNodeName);
#endif

     return(TRUE);
}

/*-----------------------------------------------------------------*/ 
/*
**  SetServerClass
**
**  Initialize fServerClass in DBC with right bit setting:
**
**  On entry:  pdbc-->Data Base Connection block
**
**  Returns:   nothing.
*/

void  SetServerClass (LPDBC pdbc)
{UWORD   class       = fServerClassINGRES;
 CHAR *  pServerType = pdbc->szSERVERTYPE;
 
 if      (STcompare(pServerType,"INGRES")==0)    class=fServerClassINGRES;
 else if (STcompare(pServerType,"INGDSK")==0  ||
          STcompare(pServerType,"OPINGDT")==0)   class=fServerClassOPINGDT;
 else if (STcompare(pServerType,"DCOM")==0)      class=fServerClassDCOM;
 else if (STcompare(pServerType,"IDMS")==0)      class=fServerClassIDMS;
 else if (STcompare(pServerType,"DB2")==0)       class=fServerClassDB2;
 else if (STcompare(pServerType,"IMS")==0)       class=fServerClassIMS;
 else if (STcompare(pServerType,"VANT")==0    ||
          STcompare(pServerType,"VSAM")==0)      class=fServerClassVSAM;
 else if (STcompare(pServerType,"RDB")==0)       class=fServerClassRDB;
 else if (STcompare(pServerType,"RMS")==0)       class=fServerClassRMS;
 else if (STcompare(pServerType,"STAR")==0)      class=fServerClassSTAR;
 else if (STcompare(pServerType,"ORACLE")==0)    class=fServerClassORACLE;
 else if (STcompare(pServerType,"INFORMIX")==0)  class=fServerClassINFORMIX;
 else if (STcompare(pServerType,"SYBASE")==0  ||
          STcompare(pServerType,"MSSQL") ==0  ||
          STcompare(pServerType,"ODBC")  ==0)    class=fServerClassSYBASE;
 else if (STcompare(pServerType,"ALLBASE")==0)   class=fServerClassALLBASE;
 else if (STcompare(pServerType,"DB2UDB")==0)    class=fServerClassDB2UDB;
 pdbc->fServerClass = class;

}

/*--------------------------------------------------------------*/
static int      DecryptTrans(int c)
{
  if (c>='0'  &&  c<='9')
     return(0x00 + (c-'0'));

  if (c>='A'  &&  c<='F')
     return(0x0a + (c-'A'));

  if (c>='a'  &&  c<='f')
     return(0x0a + (c-'a'));

  return(0);
}

/*--------------------------------------------------------------
**
**  DecryptPWD
**
**  Decrypt the password from a character string.
**
**  On entry: name-->user name
**            pwd -->Encrypted password character string
**
**  Returns:  *pwd is overlaid with the encrypted binary string.
*/
static void      TranslatePWD(char * name, char * pwd)
{ char * s;
  char * t;
  char   newpwd[LEN_ROLEPWD] = "";
  int i;
  OFFSET_TYPE len = (OFFSET_TYPE)(strlen(pwd)/2);

  for (s=pwd, t=newpwd; *s; s++)  /* convert char string to binary*/
      { i = DecryptTrans(*s)*16;
        if (*(++s))
           i += DecryptTrans(*s);
        else s--;    /* shouldn't happen; '\0' in trailing char */
        *(t++)=(char)i;
      }  /* end for loop */

  memcpy(pwd,newpwd,32); /* copy new encrypted binary string pwd to pwd */

  if (len > 32)   /* safety check */
      len = 32;
                     /* current ver is always 32 but old ver was 8, */
                     /* for upward compatibility put in rest of pwd */
  pwcrypt(name,pwd); /*decrypt the pwd */
  if (len >= 8  && len < 32)
     {strcpy(pwd+8, newpwd+8);
      pwd[len] = '\0';
     }
  pwcrypt(name,pwd); /*encrypt the pwd */
  
/*              leave PWD encrypted in binary string encryption */
}


#ifdef NT_GENERIC
/*
**  HideOrShowRole
**
**  On entry: hDlg          =  dialog handle
**            szServerType  -> to be added
**
**  Returns:  nothing
**
**  Notes:    locates an entry in list of
**               a flag byte that can be used by the caller
**               a character string of the name
**               a null terminator ('\0')
*/
static VOID HideOrShowRole(HWND hDlg, char *szServerTypeParm)
{
    char szServerType[LEN_SERVERTYPE + 1];
    char *lpsz;
    HWND  hwnd;
    int   nCmdShow;
    int   nCurShow = IsWindowVisible(hwnd=GetDlgItem (hDlg, IDC_ROLENAME))?
                         SW_SHOWNORMAL : SW_HIDE;  /* current state of control */

    strcpy(szServerType, szServerTypeParm);  /* make a work copy */
    if ((lpsz=strchr(szServerType,' ')) != NULL)
       *lpsz = '\0';    /* strip any doc from servertype */

    if (strcmp(szServerType, "DB2")  == 0     ||
        strcmp(szServerType, "DCOM") == 0     ||
        strcmp(szServerType, "IDMS") == 0     ||
        strcmp(szServerType, "IMS")  == 0     ||
        strcmp(szServerType, "VANT") == 0     ||
        strcmp(szServerType, "VSAM") == 0)
            nCmdShow = SW_HIDE;
    else  /* szServerType == "INGRES" or others */
            nCmdShow = SW_SHOWNORMAL;

/*  if (nCmdShow == nCurShow)   ** if target visibility already there then return */
/*      return; */

    if (nCmdShow == SW_HIDE)   /* if target visibility == HIDE */
       {SetDlgItemText (hDlg, IDC_ROLENAME, "");   /* clear out the text so that */
        SetDlgItemText (hDlg, IDC_ROLEPWD,  "");   /* is no "bleed-through" from an */
        SetDlgItemText (hDlg, IDC_GROUP, "");      /* invisible control to a visible one*/
       }

    ShowWindow(GetDlgItem (hDlg, IDC_ROLENAME),       nCmdShow); /* hide or show  */
    ShowWindow(GetDlgItem (hDlg, IDS_DLG_ROLENAME),   nCmdShow); /*  the controls */
    ShowWindow(GetDlgItem (hDlg, IDC_ROLEPWD),        nCmdShow);
    ShowWindow(GetDlgItem (hDlg, IDS_DLG_ROLEPWD),    nCmdShow);
    ShowWindow(GetDlgItem (hDlg, IDC_GROUP),          nCmdShow);
    ShowWindow(GetDlgItem (hDlg, IDC_GROUP_TEXT),     nCmdShow);

    return;
}
#endif /* NT_GENERIC */

#ifndef NT_GENERIC

/*--------------------------------------------------------------*/

/*
**  GetOdbcIniToken
**
**  Build token from odbc.ini file:
**
**  On entry: p       -->current character in record
**            szToken -->where to build token into
**
**  Returns:  -->new current character in record
**
*/
char * GetOdbcIniToken(char *p, char * szToken)
{
    char *   pToken;
    i4       lenToken;

    pToken = p = ScanPastSpaces (p);  /* ptoken -> start of token */
    for(;  *p  &&  CMcmpcase(p,ERx("]")) != 0
               &&  CMcmpcase(p,ERx("=")) != 0
               &&  CMcmpcase(p,ERx("\n"))!= 0
               &&  CMcmpcase(p,ERx(";")) != 0; /* stop scan if comment */  
           CMnext(p));  /* scan the token */
    lenToken = p - pToken;             /* lenToken = length of token */
    if (lenToken > 255)                /* safety check */
        lenToken = 255;

    if (lenToken)
       MEcopy(pToken, lenToken, szToken); /* copy token to token buf */
    szToken[lenToken] = EOS;           /* null terminate the token*/

    STtrmwhite(szToken);            /* trim trailing white space but
                                       leave any embedded white*/
    return(p);
}


/*
**  FindEntry
**
**  Find entry in ini file:
**
**  On entry: szSection-->ini file [<section>]
**            szEntry  -->ini section key=
**            szFile   -->ini file name
**
**  Returns:  -->entry value if found
**            NULL otherwise
**
*/
static char * FindEntry(
    LPCSTR  szSection,
    LPCSTR  szEntry,
    LPCSTR  szFile,
    char  * szWork)
{
    char *   pToken = szWork;
    char     buf[256];                  /* ini file buffer  */
    LOCATION loc;
    char     locbuf[MAX_LOC + 1];
    FILE *   pfile = NULL;
    char *   p;
    char *   pVal = NULL;
    STATUS   status;
    i4       lenSection = STlength(szSection);
    i4       lenEntry   = STlength(szEntry);
    i4       lenToken;

#ifdef WIN32
    if (STequal(szSection, ODBC_DSN))  /* a little different on WIN32 */
       szSection = "ODBC 32 bit Data Sources";
    lenSection = STlength(szSection);
#endif

    if (*szFile == ' ')        /* if odbc.ini file name could not be found */
       return(NULL);           /*    return NULL right away */

    STcopy(szFile, locbuf);    /* set up LOCATION block for file */
    if (LOfroms(PATH & FILENAME, locbuf, &loc) != OK)
       return(NULL);

    /*
    **  Read the ini file and search for the entry:
    */
    if ((status = SIfopen(&loc, ERx("r"), (i4)SI_TXT, 256, &pfile)) == OK)
    {
        while (SIgetrec (buf, (i4)sizeof(buf), pfile) == OK)
        {
            p = ScanPastSpaces (buf);

            if (CMcmpcase(p,ERx("[")) == 0) /* if starting '[' */
            {                               /* find the right section */
                CMnext(p);
                p = GetOdbcIniToken(p, pToken); /* get entry value */
                lenToken = STlength(pToken);    /* length of token */

                if (*p  &&  lenToken==lenSection  
                        &&  STbcompare(pToken, lenToken, 
                             (char*)szSection, lenSection, TRUE) == 0)
                {     /* found right section! */
                    while (SIgetrec (buf, (i4)sizeof(buf), pfile) == OK)
                    {                           /* process entries in the section */
                        p = ScanPastSpaces (buf);   /* skip leading whitespace */
                        if (CMcmpcase(p,ERx("[")) == 0) /* if starting new section */
                           break;                       /*   return not found*/

                        p = GetOdbcIniToken(p, pToken); /* get entry keyword */
                        if (*pToken == EOS)      /* if missing entry keyword */
                               continue;         /* then skip the record     */

                        p = ScanPastSpaces (p);  /* skip leading whitespace   */
                        if (CMcmpcase(p,ERx("=")) != 0)
                           continue; /* if missing '=' in key = val
                                        then skip record          */
                        CMnext(p);   /* skip the '='              */

                        lenToken = STlength(pToken); /* length of keyword token */

                        if (*p  &&  lenToken==lenEntry  
                                &&  STbcompare(pToken,  lenToken, 
                                        (char*)szEntry, lenEntry, TRUE) == 0)
                        {     /* found right entry! */
                            p = GetOdbcIniToken(p, pToken); /* get entry value */
                            pVal = pToken;  /* return pointer to entry value */
                            break;          /* return success and entry value! */
                        }
                   }   /* end while loop processing entries */
                   break;
                }
            }
        }   /* end while loop processing sections */
        SIclose (pfile);
          /*  Note: if this SIclose GPFs on WIN32, the reason is that different
              MSVCRT.DLL and MSVCRTD.DLL are being mixed up.  The OICLFNT.DLL
              is probably calling fopen() in MSVCRT.DLL and this program is
              probably calling fclose() in MSVCRTD.DLL since SIclose is #defined
              to fclose.  The release version of fopen is not compatible with
              the debug version of fclose.  The internal structures are different.*/
    }
    return pVal;
}

#ifdef INCLUDE_IIxxxPrivateProfileString
/*
**  GetPrivateProfileString
**
**  Get string value from ini file.
**
**  On entry: szSection-->ini file [<section>]
**            szEntry  -->ini section key=
**            szDefault-->default value
**            szBuffer -->return buffer
**            cbBuffer  = size of return buffer
**            szFile   -->ini file name
**
**  Returns:  length of value returned in szBuffer
**
*/
DWORD IIGetPrivateProfileString(
    LPCSTR  szSection,
    LPCSTR  szEntry,
    LPCSTR  szDefault,
    LPSTR   szBuffer,
    DWORD     cbBuffer,
    LPCSTR  szFile)
{
    LPCSTR  p;
    DWORD   len;
    char    szWork[256];       /* returned FindEntry entry value held here */

    p = FindEntry (szSection, szEntry, szFile, szWork);
    if (!p) 
         p = szDefault;
    len = STlength(p);
    if (len > (cbBuffer - 1))
        len = (cbBuffer - 1);
    if (len)
       MEcopy(p, len, szBuffer);
    *(szBuffer + len) = 0;
    return STlength (szBuffer);
}

/*
**  IIWritePrivateProfileString
**
**  Write string value to ini file.
**
**  On entry: szSection-->ini file [<section>]
**            szEntry  -->ini section key=
**            szString -->string to be written
**            szFile   -->ini file name
**
**  Returns:  length of value returned in szBuffer
**
*/
BOOL IIWritePrivateProfileString(
    LPCSTR  szSection,
    LPCSTR  szEntry,
    LPCSTR  szString,
    LPCSTR  szFile)
{
    return TRUE;   /* WritePrivateProfileString stubbed out */
}
#endif /* INCLUDE_IIxxxPrivateProfileString */

/*
**  SetODBCINIlocation
**
**  Set the file location of the ODBC.INI file.
**  Look for $ODBCINI first, then $HOME/.odbc.ini file.
**  This follows the same search order as 
**  unixODBC and Intersolv driver managers.
**
*/
static void 
SetODBCINIlocation()
{
    char * lpDir;
    char szPath[MAX_LOC+1] = "";
    bool sysIniDefined = FALSE;
    char *tmpPath = getAltPath();

    if (*ODBCINIfilename)
       return;

    NMgtAt("ODBCINI",&lpDir);   /* try $ODBCINI */
    if (lpDir  && *lpDir)
    {
         STlcopy(lpDir,szPath,sizeof(szPath)-1);
         if (FileDoesExist(szPath, TRUE))
         { /* found it */
              STcopy(szPath, ODBCINIfilename);
              return;
         }
    }

# ifdef VMS
    NMgtAt("SYS$LOGIN",&lpDir);     /* try SYS$LOGIN:odbc.ini */
# else
    NMgtAt("HOME",&lpDir);     /* try $HOME/.odbc.ini */
# endif /* VMS */
    if (lpDir  && *lpDir)
    {
         STlcopy(lpDir,szPath,sizeof(szPath)-10-1);
# ifdef VMS
         STcat(szPath, "odbc.ini");
# else
         STcat(szPath, "/.odbc.ini");
# endif /* VMS */
         if (FileDoesExist(szPath, TRUE))
         { /* found it */
              STcopy(szPath, ODBCINIfilename);
              return;
         }
    }
    /*
    **  If no user-level definition, look for system level.
    */
    NMgtAt("ODBCSYSINI",&lpDir);  
    if (lpDir  && *lpDir)
    {
         STlcopy(lpDir,szPath,sizeof(szPath)-10-1);
         STcat(szPath, "/odbc.ini");
         if (FileDoesExist(szPath, TRUE))
         { /* found it */
              STcopy(szPath, ODBCINIfilename);
              return;
         }
    }
# ifdef VMS
    NMgtAt("SYS$SHARE",&lpDir);
# else
    lpDir = tmpPath;
# endif
    if (lpDir  && *lpDir)
    {
         STlcopy(lpDir,szPath,sizeof(szPath)-10-1);
# ifndef VMS
         STcat(szPath, "/");
# endif
         STcat(szPath, "odbc.ini");
         if (FileDoesExist(szPath, TRUE))
         { /* found it */
              STcopy(szPath, ODBCINIfilename);
              return;
         }
    }

    STcopy(" ", ODBCINIfilename);  /* make that odbc.ini is unknown */

    return;
}

/*
**  SQLGetPrivateProfileString
**
**  Get string value from ini file.
**
**  On entry: szSection-->ini file [<section>]
**            szEntry  -->ini section key=
**            szDefault-->default value
**            szBuffer -->return buffer
**            cbBuffer  = size of return buffer
**            szFile   -->ini file name
**
**  Returns:  length of value returned in szBuffer
**
*/
int INSTAPI 
SQLGetPrivateProfileString(
    LPCSTR  szSection,
    LPCSTR  szEntry,
    LPCSTR  szDefault,
    LPSTR   szBuffer,
    int     cbBuffer,
    LPCSTR  szFile)
{
    LPCSTR  p;
    i4      len;
    char    szWork[256];       /* returned FindEntry entry value held here */
    char    systemPath[256];
    bool    sysIniDefined=FALSE;
    char *tmpPath = getAltPath();

    if (STequal(szFile, ODBC_INI))  /* if odbc.ini (not odbcinst.ini) */
       {
         SetODBCINIlocation();         /* find the odbc.ini path/fname, if needed */
         if (*ODBCINIfilename)         /* if fully qualified name was found then */
            szFile = ODBCINIfilename;  /* repoint szFile to full odbc.ini name */
       }

    p = FindEntry (szSection, szEntry, szFile, szWork);
    if (!p) 
    {
# ifdef VMS
        STprintf(systemPath, "%sodbc.ini",tmpPath); 
# else
        STprintf(systemPath, "%s/odbc.ini",tmpPath); 
# endif
        szFile = systemPath;
        p = FindEntry (szSection, szEntry, szFile, szWork);
    }
    if (!p)
         p = szDefault;
    len = STlength(p);
    if (len > (cbBuffer - 1))
        len = (cbBuffer - 1);
    if (len)
       MEcopy(p, len, szBuffer);
    *(szBuffer + len) = 0;
    return STlength (szBuffer);
}
/*
** Name: getAltPath
**
** Description:
**      This function retrieves the alternate path info, if it exists,
**      from the $II_CONFIG/odbcmgr.dat file.
**
** Return value:
**      defaultInfo        Array of default configuration values.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      none.
**
** History:
**      06-Sep-03 (loera01)
**          Created.
**      17-June-2008 (Ralph Loen) Bug 120516
**          Check for ODBCSYSINI before checking anything else.  The 
**          definition of ODBCSYSINI overrides other settings.
*/

char *getAltPath()
{
    char *dirPath;
    char fullPath[OCFG_MAX_STRLEN];
    char buf[OCFG_MAX_STRLEN];           
    FILE *pfile;
    LOCATION loc;
    static bool pathDefined = FALSE;
    static char *path = NULL;
    i4 i;
    char Token[OCFG_MAX_STRLEN], *pToken = &Token[0];
    char *p;
  
    if (pathDefined)
        return path;

    /*
    ** ODBCSYSINI definition trumps all.
    */
    NMgtAt("ODBCSYSINI",&dirPath);  
    if (dirPath != NULL && *dirPath)
    { 
        pathDefined = TRUE;
        path = &dirPath[0];
        return path;
    }

    /*
    ** Get the path of the odbcmgr.dat file and create the asscociated string.
    */
    NMgtAt(SYSTEM_LOCATION_VARIABLE,&dirPath);  /* usually II_SYSTEM */
    if (dirPath != NULL && *dirPath)
        STlcopy(dirPath,fullPath,sizeof(fullPath)-20-1);
    else
        return NULL;

# ifdef VMS
    STcat(fullPath,"[");
    STcat(fullPath,SYSTEM_LOCATION_SUBDIRECTORY);  /* usually "ingres"  */
    STcat(fullPath,".files]");
    STcat(fullPath,MGR_STR_FILE_NAME);
# else
    STcat(fullPath,"/");
    STcat(fullPath,SYSTEM_LOCATION_SUBDIRECTORY);  /* usually "ingres"  */
    STcat(fullPath,"/files/");
    STcat(fullPath,MGR_STR_FILE_NAME);
# endif /* VMS */

    if ( LOfroms(PATH & FILENAME, fullPath, &loc) != OK )
    {
       /*
       ** if odbcmgr.dat cannot be located, return the default for unixODBC.
       */
#ifdef VMS
       path = "SYS$SHARE:";
#else
       path = "/usr/local/etc/";
#endif
       return path;
    }

    if (SIfopen(&loc, "r", (i4)SI_TXT, OCFG_MAX_STRLEN, &pfile) != OK)
    {
        pathDefined = TRUE;
#ifdef VMS
       path = "SYS$SHARE:";
#else
       path = "/usr/local/etc/";
#endif
       return path;
    }

    while (SIgetrec (buf, (i4)sizeof(buf), pfile) == OK)
    {
        p = buf;
        while (CMwhite(p)) CMnext(p);/* skip leading whitespace */
        p = getFileEntry(p, pToken,FALSE); /* get entry keyword */
        if (*pToken == EOS)      /* if missing entry keyword */
            continue;            /* then skip the record     */
        while (CMwhite(p)) CMnext(p);/* skip leading whitespace */
        if (CMcmpcase(p,ERx("=")) != 0)
        {
            continue; /* if missing '=' in key = val, skip record */
        }

        /*
        ** Check for "DriverManagerPath".
        */
	if ( !STbcompare( pToken, 0, MGR_STR_PATH, 0, TRUE ) && 
            ! pathDefined )
        {
	    CMnext(p);   /* skip the '='              */
#ifdef VMS
	    p = getFileEntry(p, pToken,TRUE); /* get entry value */
#else
	    p = getFileEntry(p, pToken,FALSE); /* get entry value */
#endif /* VMS */
            if (*pToken == EOS)      /* if missing entry keyword */
                continue;            /* then skip the record     */
	    path = STalloc(pToken);
            break;
        }
        else
        {
#ifdef VMS
            path = "SYS$SHARE:";   
#else
            path = "/usr/local/etc/";
#endif
            break;
         }

    }   /* end while loop processing entries */

    if (path)
        STtrmwhite(path);

    pathDefined = TRUE;
    SIclose(pfile);
    return path;
}

char * getFileEntry(char *p, char * szToken, bool ignoreBracket)
{
    char *   pToken;
    i4       lenToken;

    while (CMspace(p)) CMnext(p);  /* Ignore leading spaces */
    pToken = p;
# ifdef VMS
    if (ignoreBracket)
        for(;  *p  &&  CMcmpcase(p,ERx("=")) != 0
               &&  CMcmpcase(p,ERx("\n"))!= 0
               &&  CMcmpcase(p,ERx(";")) != 0; /* stop scan if comment */  
           CMnext(p));  /* scan the token */
    else
# endif /* VMS */
        for(;  *p  &&  CMcmpcase(p,ERx("]")) != 0
               &&  CMcmpcase(p,ERx("=")) != 0
               &&  CMcmpcase(p,ERx("\n"))!= 0
               &&  CMcmpcase(p,ERx(";")) != 0; /* stop scan if comment */  
           CMnext(p));  /* scan the token */
    lenToken = p - pToken;             /* lenToken = length of token */
    if (lenToken > 255)                /* safety check */
        lenToken = 255;
    if (lenToken)
    {
       MEcopy(pToken, lenToken, szToken); /* copy token to token buf */
    }
    szToken[lenToken] = EOS;           /* null terminate the token*/
    STtrmwhite(szToken);            /* trim trailing white space but
                                       leave any embedded white*/
    return(p);
}
#endif /*not NT_GENERIC */
static VOID concatBrowseString(char *inputString, char *attr)
{
    STcat(inputString, attr);
    STcat(inputString, "?;");
}

/*
** Name: getDriverVersion
**
** Description:
**      This exported function retuns the current value string for the
**      ODBC driver.
**
** Input:                  None.
**
** Output:                 Version string.
**
** Return value:           Void.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** History:
**   19-oct-2007 (Ralph Loen) Bug 119329
**      Created.
*/

void getDriverVersion  ( char *version )
{
    STcopy(VER_FILEVERSION_STR, version);
    return;
}
