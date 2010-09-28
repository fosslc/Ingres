/*
** Copyright (c) 1997, 2009 Ingres Corporation
*/

/*
** Name: CAIIOC32.C
**
** Description:
**	Global variables for Ingres administrator
**
** History:
**	26-jan-1994 (rosda01)
**	    Converted idmsetup.
**	01-jul-1996 (Ron Gagne)
**	    Port to Win32.
**	17-jul-1996 (rosda01)
**	    Move most INI key strings here.
**	22-jan-1997 (stoli02)
**	    Made adaptations for Ingres.
**	20-feb-1997 (thoda04)
**	    Modified memicmp to _memicmp. 
**	06-mar-1997 (tenje01)
**	    Added szPrompt.
**	18-jun-1997 (thoda04)
**	    Added szWithOption.
**	07-nov-1997 (tenje01)
**	    Added szRoleName and szRolePWD .
**	01-dec-1997 (tenje01)
**	    Added SelectLoops/CursorLoops
**	15-dec-1997 (tenje01)
**	    Added Load/FreeIngresLibraries
**	15-dec-1997 (thoda04)
**	    Added sz3PartNames
**	20-may-1998 (Dmitriy Bubis)
**	    Fixed Bug #90880 (ConfigDSN)
**	19-mar-1999 (Bobby Ward)
**	    Added code to fix Bug# 93886 (II_HOSTNAME)  
**	20-aug-1999 (padni01)
**	    Fixed bug 98452 & 98459 in AutoConfig()
**	15-nov-1999 (thoda04)
**	    Added szBlankDate
**	13-dec-1999 (thoda04)
**	    From ConfigDSN, return FALSE if canceled
**	12-jan-2000 (thoda04)
**	    Added szDate1582
**	15-mar-2000 (thoda04)
**	    Added bulletproofing if Ingres CL not installed
**	27-apr-2000 (thoda04)
**	    Added CatConnect= and SelectLoops= support
**	05-jun-2000 (thoda04)
**	    Added numeric_overflow= support
**	16-oct-2000 (loera01)
**	    Added support for II_DECIMAL (Bug 102666).
**	30-oct-2000 (thoda04)
**	    Added CatSchemaNULL support
**	25-oct-2000 (thoda04)
**	    Fixed ConfigDSN attr string scan
**	07-nov-2000 (thoda04)
**	    Convert nat to i4
**	12-jul-2001 (somsa01)
**	    Fixed 64-bit compiler warnings.
**	18-apr-2002 (weife01) Bug #106831
**          Add option in advanced tab of ODBC admin which allows the 
**          procedure update with odbc read only driver.
**      22-may-2002 (loera01) Bug 107814
**          Add "Advanced" DSN attributes to ConfigDSN().
**      14-feb-2003 (loera01) SIR 109643
**          Minimize platform-dependency in code.  Make backward-compatible
**          with earlier Ingres versions.
**      07-mar-2003 (loera01) SIR 109789
**          Add read-only option to data source DSN defintion.  Check
**          ODBCINST_INI to see if a default read-only definition was 
**          provided.
**      27-may-2003 (weife01) SIR 110311
**          Changed name <product>ODBC.DLL to <product>OD35.DLL, and
**	    <product>OCFG.DLL to <product>OC35.DLL and <product>OCFG.HLP to
**	    <product>OC35.HLP
**      01-jul-2003 (weife01) SIR #110511
**          Add support for group ID in DSN.
**	04-sep-2003 (somsa01)
**	    Removed inclusion of ctl3d.h.
**      19-apr-2004 (loera01)
**          Removed dependence on sqltypes.h.
**	11-Jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**      10-Mar-2006 (Ralph Loen) Bug 115833
**          Added support for new DSN configuration option: "Fill character 
**          for failed Unicode/multibyte conversions".
**      20-Sep-2007 (weife01) SIR 119140
**          Added support for new DSN/connection string configuration
**          option:"Convert int8 from DBMS to int4".
**      01-apr-2008 (weife01) Bug 120177
**          Added DSN configuration option "Allow update when use pass through
**          query in MSAccess".
**      12-May=2008 (Ralph Loen) Bug 120356
**          Added DSN configuration option to treat SQL_C_DEFAULT as 
**          SQL_C_CHAR for Unicode (SQLWCHAR) data types.
**	03-Nov-2008 (drivi01)
**	    Replace old windows help with html help.
**     14-Nov-2008 (Ralph Loen) SIR 121228
**         Add support for OPT_INGRESDATE configuration parameter.  If set,
**         ODBC date escape syntax is converted to ingresdate syntax, and
**         parameters converted to IIAPI_DATE_TYPE are converted instead to
**         IIAPI_DTE_TYPE.
**     28-Apr-2009 (Ralph Loen) SIR 122007
**         Add support for "StringTruncation" connection string attribute.
**         If set to Y, string truncation errors are reported.  If set to
**         N (default), strings are truncated silently.
**     09-Jun-2010 (Ralph Loen) Bug 123864
**          In ConfigDSN(), tightened code for BlankDate, Date1582,
**          IngresDate, StringTruncation, and FetchWholeLob.  Imprecisions
**          in copying buffers led to these items not being configured
**          if not specified with defaults.
*/
/*
**Jam hints
**
LIBOBJECT = caiioc32.c
**
*/
#include <compat.h>
#include <windowsx.h>
#include "resource.h"              // Resource : MS Devleoper Studio
#include <sql.h>
#include <odbcinst.h>              // Installer DLL definitions.
#include <commctrl.h>              // Common controls
#include <prsht.h>                 // Property sheets
#define _CNFG
#include <idmseini.h>
#include <idmsutil.h>               // Util definitions.
#include <htmlhelp.h>

/* CL functions */
#include <cm.h>
#include <cv.h>
#include <st.h>
#include <er.h>
#include <me.h>
#include <nm.h>
#include <erodbc.h>

#include "caiioc32.h"

//
//  Global constants and pseudo constants from string resources:
//
const char TRACE_MSG[] = "ODBC TRACE: %s\r\n";
BOOL  fDebug = FALSE;

const char  DPATCH[128] = " PATCH: "; // Global constant data patch space.

const char DSN[]			= "DSN=";
const char DESCRIPTION[]	= "DESCRIPTION=";
const char VNODE[]			= "SERVER=";
const char SERVERTYPE[]		= "SERVERTYPE=";
const char DATABASE[]		= "DATABASE=";
const char ROLENAME[]		= "ROLENAME=";
const char ROLEPWD[]		= "ROLEPWD=";
const char GROUP[]              = "GROUP=";
const char PROMPT[]             = "PROMPT=";
const char SELECTLOOPS[]        = "SELECTLOOPS=";
const char PROMPTUIDPWD[]       = "PROMPTUIDPWD=";
const char DSNREADONLY[]        = "READONLY=";
const char WITHOPTION[]         = "WITHOPTION=";
const char DISABLECATUNDERSCORE[]  = "DISABLECATUNDERSCORE=";
const char CATUNDERSCORE[]    = "DISABLECATUNDERSCORE=";
const char ALLOWPROCEDUREUPDATE[]  = "ALLOWPROCEDUREUPDATE=";
const char MULTIBYTEFILLCHAR[] = "MULTIBYTEFILLCHAR=";
const char CONVERTTHREEPARTNAMES[] = "CONVERTTHREEPARTNAMES=";
const char USESYSTABLES[]          = "USESYSTABLES=";
const char BLANKDATE[]             = "BLANKDATE=";
const char DATE1582[]              = "DATE1582=";
const char CATCONNECT[]            = "CATCONNECT=";
const char NUMERIC_OVERFLOW[]      = "NUMERIC_OVERFLOW=";
const char SUPPORTIIDECIMAL[]      = "SUPPORTIIDECIMAL=";
const char CATSCHEMANULL[]         = "CATSCHEMANULL=";
const char CONVERTINT8TOINT4[]     = "CONVERTINT8TOINT4=";
const char ALLOWUPDATE[]           = "ALLOWUPDATE=";
const char DEFAULTTOCHAR[]         = "DEFAULTTOCHAR=";
const char INGRESDATE[]            = "INGRESDATE=";
const char STRING_TRUNCATION[]      = "STRINGTRUNCATION=";

/* We support Intersolv synonym too. */
const char DATASOURCENAME[] = "DATASOURCENAME=";
const char SERVERNAME[]     = "SERVERNAME=";
const char SRVR[]           = "SRVR=";
const char DB[]             = "DB=";

char MSG_REQUIRED[80];
char MSG_DESCRIPTION[80];

//
//  Global ini file constants:
//

const char INI_FALSE[] = "0";
const char INI_TRUE[]  = "1";

const char INI_DSN[]				= KEY_DSN;
const char INI_DESCRIPTION[]		= KEY_DESCRIPTION_UP;
const char INI_SERVER[]				= KEY_SERVER_UP;
const char INI_SERVERTYPE[]			= KEY_SERVERTYPE_UP;
const char INI_DATABASE[]			= KEY_DATABASE_UP;
const char INI_ROLENAME[]			= KEY_ROLENAME;
const char INI_ROLEPWD[]			= KEY_ROLEPWD;
const char INI_GROUP[]                          = KEY_GROUP;
const char INI_WITHOPTION[]			= KEY_WITHOPTION;
const char INI_DATASOURCENAME[]		= KEY_DATASOURCENAME;
const char INI_SERVERNAME[]			= KEY_SERVERNAME;
const char INI_SRVR[]				= KEY_SRVR;
const char INI_DB[]					= KEY_DB;
const char INI_SELECTLOOPS[]	         = KEY_SELECTLOOPS;
const char INI_PROMPTUIDPWD[]	         = KEY_PROMPTUIDPWD;
const char INI_READONLY[]	         = KEY_READONLY;
const char INI_DISABLECATUNDERSCORE[]    = KEY_DISABLECATUNDERSCORE;
const char INI_ALLOWPROCEDUREUPDATE[]    = KEY_ALLOWPROCEDUREUPDATE;
const char INI_CONVERTTHREEPARTNAMES[]   = KEY_CONVERTTHREEPARTNAMES;
const char INI_USESYSTABLES[]  	         = KEY_USESYSTABLES;
const char INI_BLANKDATE[]	         = KEY_BLANKDATE;
const char INI_DATE1582[]		 = KEY_DATE1582;
const char INI_CATCONNECT[]	         = KEY_CATCONNECT;
const char INI_NUMERIC_OVERFLOW[]	 = KEY_NUMERIC_OVERFLOW;
const char INI_SUPPORTIIDECIMAL[]	 = KEY_SUPPORTIIDECIMAL;
const char INI_CATSCHEMANULL[]	         = KEY_CATSCHEMANULL;
const char INI_CONVERTINT8TOINT4[]   = KEY_CONVERTINT8TOINT4;
const char INI_MULTIBYTEFILLCHAR[]   = KEY_MULTIBYTEFILLCHAR;
const char INI_ALLOWUPDATE[]         = KEY_ALLOWUPDATE;
const char INI_DEFAULTTOCHAR[]         = KEY_DEFAULTTOCHAR;
const char INI_INGRESDATE[]            = KEY_INGRESDATE;
const char INI_STRINGTRUNCATION[]      = KEY_STRING_TRUNCATION;

//
//  Global variables:
//
HINSTANCE           hInstance;      // actually used as a module handle
HINSTANCE   hClfModule=NULL;        /* Saved module instance handle of CL */
HINSTANCE   hClfDataModule=NULL;    
CRITICAL_SECTION    csCNF;          // just one caller at a time per process
OSVERSIONINFO       os;             // operating system version

char szDSN[LEN_DSN + 1];
char szHelp[256];
char szPath[MAX_PATH];         
char szMessage[LEN_MESSAGE];
char szMsgTitle[80];
char szServer[LEN_SERVER + 1];
char szServerSect[LEN_RESOURCESECT + 1];
char szServerType[LEN_SERVERTYPE + 1];
char szDriver[LEN_DRIVER_NAME + 1];
char szDBName[LEN_DBNAME + 1];
char szDescription[LEN_DESCRIPTION + 1];
char szProduct[LEN_PRODUCT_NAME + 1];
char szProductVersion[LEN_VERSION_STRING + 1];
char szConfigVersion[LEN_VERSION_STRING + 1];
char szErrTitle[80];
char szWork[LEN_WORK];
char szPrompt[LEN_PROMPT + 1];
char szReadOnly[2];
char szDriverReadOnly[2];
char szWithOption[256];
char szRoleName[LEN_ROLENAME + 1];
char szRolePWD[LEN_ROLEPWD + 1];
char szGroup[LEN_GROUP + 1];
char szSelectLoops[LEN_SELECTLOOPS + 1];
char sz3PartNames[6];
char szSYSTABLES[6];
char szBlankDate[20];
char szDate1582[20];
char szCatConnect[6];
char szNumOverflow[20];
char szIIDECIMAL[6];
char szCatSchemaNULL[6];
char szCatUnderscoreDisabled[4];
char szAllowProcedureUpdate[4];
char szMultibyteFillChar[4];
char szConvertInt8ToInt4[4];
char szAllowUpdate[4];
char szDefaultToChar[4];
char szIngresDate[4];
char szStringTruncation[4];

BOOL fDSN;
BOOL fUNIX;
BOOL fIDMS;
BOOL fCADB;
BOOL fGetServer;
BOOL fDriverReadOnly = FALSE;
UINT fRequest;
UINT fDriver;
UINT fDriverAttrib;
UINT idDialogLog;
UINT idDialogOptions;
UINT idDialogServer;
UINT cbDictLen;

//
//  AutoConfig Keyname Table:
//
#define SEC_DATASOURCE      1
#define SEC_SERVER          2
#define SEC_OPTIONS         3


static const struct
{
    UINT    fSection;           // Registry sub key flag (ini section name)
    LPCSTR  szKey;              // Registry value name   (ini key name)
}
KEYTAB[] =
{
    { SEC_DATASOURCE, INI_DESCRIPTION     },
    { SEC_DATASOURCE, INI_SERVER	      },
    { SEC_DATASOURCE, INI_SERVERTYPE      },
    { SEC_DATASOURCE, INI_DATABASE	      },
    { SEC_DATASOURCE, INI_ROLENAME	      },
    { SEC_DATASOURCE, INI_ROLEPWD	      },
    { SEC_DATASOURCE, INI_WITHOPTION      },
    { SEC_DATASOURCE, INI_SERVERNAME      },
    { SEC_DATASOURCE, INI_SRVR		      },
    { SEC_DATASOURCE, INI_DB		      },
    { SEC_DATASOURCE, INI_SELECTLOOPS },
    { SEC_DATASOURCE, INI_PROMPTUIDPWD },
    { SEC_DATASOURCE, INI_GROUP  	      },
    { SEC_DATASOURCE, INI_READONLY },
    { SEC_DATASOURCE, INI_DISABLECATUNDERSCORE },
    { SEC_DATASOURCE, INI_ALLOWPROCEDUREUPDATE },
    { SEC_DATASOURCE, INI_MULTIBYTEFILLCHAR },
    { SEC_DATASOURCE, INI_CONVERTTHREEPARTNAMES },
    { SEC_DATASOURCE, INI_USESYSTABLES },
    { SEC_DATASOURCE, INI_BLANKDATE },
    { SEC_DATASOURCE, INI_DATE1582 },
    { SEC_DATASOURCE, INI_CATCONNECT },
    { SEC_DATASOURCE, INI_NUMERIC_OVERFLOW },
    { SEC_DATASOURCE, INI_SUPPORTIIDECIMAL },
    { SEC_DATASOURCE, INI_CATSCHEMANULL },
    { SEC_DATASOURCE, INI_CONVERTINT8TOINT4 },
    { SEC_DATASOURCE, INI_ALLOWUPDATE },
    { SEC_DATASOURCE, INI_DEFAULTTOCHAR },
    { SEC_DATASOURCE, INI_INGRESDATE },
    { SEC_DATASOURCE, INI_STRINGTRUNCATION }
};
#define KEYTABMAX   sizeof KEYTAB / sizeof KEYTAB[0]

//
//  Internal variables:
//
static char szTabCaption[80];
static char szTabTitle[7][40];

static PROPSHEETHEADER psh;
static PROPSHEETPAGE   psp[3];

//
//  Internal function prototypes:
//
extern void EncryptPWD(char * name, char * pwd);
BOOL AutoConfig (UINT, LPCSTR);
int  CreatePropertySheet (HWND);
VOID CreatePropertyTab (int i);
BOOL SetDriverInfo (LPCSTR driver);

static void localNMgtAt(char *name, char **ret_val);

//
//  DllMain
//
//  Win32 DLL entry and exit routines.
//  Replaces Libmain and WEP used in 16 bit Windows.
//
//  Input:   hmodule     = Module handle (for 16 bit DLL's this was the
//                         instance handle, but it is used the same way).
//
//           fdwReason   = DLL_PROCESS_ATTACH
//                         DLL_THREAD_ATTACH
//                         DLL_THREAD_DETACH
//                         DLL_PROCESS_DETACH
//
//           lpvReserved = just what it says.
//
//  Returns: TRUE
//
//
BOOL APIENTRY DllMain (
    HMODULE hmodule,
    DWORD   fdwReason,
    LPVOID  lpReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        //
        //  The DLL is being mapped into the process's address space.
        //
        hInstance   = hmodule;

        InitializeCriticalSection (&csCNF);
        //
        //  Get OS version:
        //
        os.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
        if (!GetVersionEx (&os))
            return FALSE;

        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:

        break;

    case DLL_PROCESS_DETACH:
        /*
        **  The DLL is being unmapped from the process's address space.
        */
        DeleteCriticalSection (&csCNF);
        break;
    }
    return TRUE;
}

//
//  ConfigDSN
//
//  Configure Data Source Name.
//
//  Entry point function for an ODBC administration application.
//
//  On entry: hwnd           = parent window handle.
//            fReq           = type of request (ADD, CONFIG, REMOVE).
//            lpszDriver    -->driver type.
//            lpszAttributes-->attribute string.
//
//  Returns:  SQL_SUCCESS
//            SQL_ERROR
//

BOOL INSTAPI ConfigDSN(
    HWND    hwnd,
    WORD    fReq,
    LPCSTR  lpszDriver,
    LPCSTR  lpszAttr)
{
    char *  sz;
    BOOL    rc = FALSE;
	

    if (!lpszDriver)
        return FALSE;
    
    EnterCriticalSection (&csCNF);
    
    //
    //  Setup global driver information
    //
    fIDMS			= FALSE;
    fRequest		= fReq;
    *szDSN			= 0;
    *szDriver		= 0;
	*szDescription	= 0;
	*szServerType	= 0;
	*szServer		= 0;
	*szDBName		= 0;
	*szRoleName		= 0;
	*szRolePWD		= 0;
    *szGroup        = 0;
	*szPrompt		= 0;
	*szReadOnly		= 0;
	*szDriverReadOnly = 0;
	*szWithOption	= 0;
	*szBlankDate 	= 0;
	*szDate1582 	= 0;
	*szCatConnect	= 0;
	*szSelectLoops	= 0;
	*szNumOverflow	= 0;
	*szCatSchemaNULL= 0;
	*szCatUnderscoreDisabled= 0;
	*szAllowProcedureUpdate=0;
	*szMultibyteFillChar=0;
	*szConvertInt8ToInt4=0;
	*szAllowUpdate = 0;
	*szDefaultToChar = 0;
	*szIngresDate = 0;
        *szStringTruncation = 0;

	STcopy(lpszDriver,szDriver);
    
    if (!SetDriverInfo(lpszDriver))
        return FALSE;
    
    /*
    **  Extract DSN from attribute string, if any.
    */
    for (sz = (char *)lpszAttr; sz && *sz; sz += STlength(sz) + 1)
    {
        if (STbcompare((char *)DSN, 4, sz, 4, TRUE) == 0 && !*szDSN)
            STlcopy(sz + sizeof DSN - 1, szDSN, sizeof szDSN - 1);
        else
        if (STbcompare((char *)DATASOURCENAME, 15, sz, 15, TRUE) == 0 
            && !*szDSN)
            STlcopy(sz + sizeof DATASOURCENAME - 1, szDSN, sizeof szDSN - 1);
        else
        if (STbcompare((char *)DESCRIPTION, 12, sz, 12, TRUE) == 0 
            && !*szDescription)
            STlcopy(sz + sizeof DESCRIPTION - 1, szDescription, 
                sizeof szDescription - 1);
        else
        if (STbcompare((char *)SERVERTYPE, 11, sz, 11, TRUE) == 0 
            && !*szServerType)
            STlcopy(sz + sizeof SERVERTYPE - 1, szServerType, 
                sizeof szServerType - 1);
        else
        if (STbcompare((char *)SERVERNAME, 11, sz, 11, TRUE) == 0 
            && !*szServer)
            STlcopy(sz + sizeof SERVERNAME - 1, szServer, 
                sizeof szServer - 1);
        else
        if (STbcompare((char *)VNODE, 6, sz, 6, TRUE) == 0 && !*szServer)
            STlcopy(sz + sizeof VNODE - 1, szServer, sizeof szServer - 1);
        else
        if (STbcompare((char *)SRVR, 5, sz, 5, TRUE) == 0 && !*szServer)
            STlcopy(sz + sizeof SRVR - 1, szServer, sizeof szServer - 1);
        else
        if (STbcompare((char *)DATABASE, 9, sz, 9, TRUE) == 0 && !*szDBName)
            STlcopy(sz + sizeof DATABASE - 1, szDBName, sizeof szDBName - 1);
        else
        if (STbcompare((char *)DB, 3, sz, 3, TRUE) == 0 && !*szDBName)
            STlcopy(sz + sizeof DB - 1, szDBName, sizeof szDBName - 1);
        else
        if (STbcompare((char *)ROLENAME, 9, sz, 9, TRUE) == 0 && !*szRoleName)
            STlcopy(sz + sizeof ROLENAME - 1, szRoleName, 
                sizeof szRoleName - 1);
        else
        if (STbcompare((char *)ROLEPWD, 8, sz, 8, TRUE) == 0 && !*szRolePWD)
            STlcopy(sz + sizeof ROLEPWD - 1, szRolePWD, sizeof szRolePWD - 1);
        else
        if (STbcompare((char *)PROMPTUIDPWD, 13, sz, 13, TRUE) == 0 
            && !*szPrompt)
        {
            STlcopy(sz + sizeof PROMPTUIDPWD - 1, szPrompt, 
                sizeof szPrompt - 1);
            if (*szPrompt == 'Y'  ||  *szPrompt == 'y')
                STcopy("Y", szPrompt);  /* normalize the value */
            else 
                STcopy("N", szPrompt);
        }
        else
        if (STbcompare((char *)GROUP, 6, sz, 6, TRUE) == 0 && !*szGroup)
            STlcopy (sz + sizeof GROUP - 1, szGroup, sizeof szGroup - 1);
        else
        if (STbcompare((char *)DSNREADONLY, 9, sz, 9, TRUE) == 0 
            && !*szReadOnly)
        {
            STlcopy(sz + sizeof DSNREADONLY - 1, szReadOnly, 
                sizeof szReadOnly - 1);
            if (*szReadOnly == 'Y'  ||  *szReadOnly == 'y')
                STcopy("Y", szReadOnly);  /* normalize the value */
            else 
                STcopy("N", szReadOnly);
        }
        else
        if (STbcompare((char *)WITHOPTION, 11, sz, 11, TRUE) == 0 
            && !*szWithOption)
            STlcopy(sz + sizeof WITHOPTION - 1, szWithOption, 
                sizeof szWithOption - 1);
        else
        if (STbcompare((char *)BLANKDATE, 10, sz, 10, TRUE) == 0 
            && !*szBlankDate)
        {
            STlcopy(sz + sizeof BLANKDATE - 1, szBlankDate, 
                sizeof szBlankDate - 1);
            if (*szBlankDate == 'N'  ||  *szBlankDate == 'n')
                 STcopy("NULL", szBlankDate);  /* normalize the value */
            else 
                STcopy(" ", szBlankDate); /* user supplied not NULL value*/
        }
        else
        if (STbcompare((char *)DATE1582, 9, sz, 9, TRUE) == 0 && !*szDate1582)
        {
            STlcopy(sz + sizeof DATE1582 - 1, szDate1582, 
                sizeof szDate1582 - 1);
            if (*szDate1582 == 'N'  ||  *szDate1582 == 'n')
                STcopy("NULL", szDate1582);  /* normalize the value */
             else 
                STcopy(" ", szDate1582);     /* user supplied not NULL value*/
        }
        else
        if (STbcompare((char *)CATCONNECT,11, sz,11, TRUE) == 0 
            && !*szCatConnect)
        {
            STlcopy(sz + sizeof CATCONNECT - 1, szCatConnect, 
                sizeof szCatConnect - 1);
            if (*szCatConnect == 'Y'  ||  *szCatConnect == 'y')
                STcopy("Y",szCatConnect);  /* normalize the value */
            else 
                STcopy("N",szCatConnect);
        }
        else
        if (STbcompare((char *)SELECTLOOPS,12, sz,12, TRUE) == 0 
            && !*szSelectLoops)
        {
            STlcopy(sz + sizeof SELECTLOOPS - 1, szSelectLoops, 
                sizeof szSelectLoops - 1);
            if (*szSelectLoops == 'Y'  ||  *szSelectLoops == 'y')
                STcopy("Y",szSelectLoops);  /* normalize the value */
            else 
                STcopy("N",szSelectLoops);
        }
        else
        if (STbcompare((char *)NUMERIC_OVERFLOW,17, sz,17, TRUE) == 0 
            && !*szNumOverflow)
        {
            STlcopy(sz + sizeof NUMERIC_OVERFLOW - 1, szNumOverflow, 
                sizeof szNumOverflow - 1);
            if (*szNumOverflow == 'I'  ||  *szNumOverflow == 'i')
                STcopy("Ignore",szNumOverflow);  /* normalize the value */
            else 
                STcopy("Fatal", szNumOverflow);
        }
        else
        if (STbcompare((char *)CATSCHEMANULL,14, sz, 14, TRUE) == 0 
            && !*szCatSchemaNULL)
        {
            STlcopy(sz + sizeof CATSCHEMANULL - 1, szCatSchemaNULL, 
                sizeof szCatSchemaNULL - 1);
            if (*szCatSchemaNULL == 'Y'  ||  *szCatSchemaNULL == 'y')
                STcopy("Y",szCatSchemaNULL);  /* normalize the value */
            else 
                STcopy("N",szCatSchemaNULL);
        }
        else
        if (STbcompare((char *)CATUNDERSCORE,14, sz, 14, TRUE) == 0 
            && !*szCatUnderscoreDisabled)
        {
            STlcopy(sz + sizeof CATUNDERSCORE - 1, szCatUnderscoreDisabled, 
                sizeof szCatUnderscoreDisabled - 1);
            if (*szCatUnderscoreDisabled == 'Y' ||  
                *szCatUnderscoreDisabled == 'y')
                STcopy("Y",szCatUnderscoreDisabled);  /* normalize the value */
            else 
                STcopy("N",szCatUnderscoreDisabled);
        }
        else
        if (STbcompare((char *)ALLOWPROCEDUREUPDATE,21, sz, 21, TRUE) == 0 
            && !*szAllowProcedureUpdate)
        {
            STlcopy(sz + sizeof ALLOWPROCEDUREUPDATE - 1, 
                szAllowProcedureUpdate, sizeof szAllowProcedureUpdate - 1);
            if (*szAllowProcedureUpdate == 'Y'  ||  
                *szAllowProcedureUpdate == 'y')
                STcopy("Y",szAllowProcedureUpdate);  /* normalize the value */
            else 
                STcopy("N",szAllowProcedureUpdate);
        }
        else
        if (STbcompare((char *)CONVERTTHREEPARTNAMES,22, sz, 22, TRUE) == 0 
            && !*sz3PartNames)
        {
            STlcopy(sz + sizeof CONVERTTHREEPARTNAMES - 1, sz3PartNames, 
                sizeof sz3PartNames - 1);
            if (*sz3PartNames == 'Y'  ||  *sz3PartNames == 'y')
                STcopy("Y",sz3PartNames);  /* normalize the value */
            else 
                STcopy("N",sz3PartNames);
        }
        else
        if (STbcompare((char *)SUPPORTIIDECIMAL,17, sz, 17, TRUE) == 0 
            && !*szIIDECIMAL)
        {
            STlcopy(sz + sizeof SUPPORTIIDECIMAL - 1, szIIDECIMAL, 
                sizeof szIIDECIMAL - 1);
            if (*szIIDECIMAL == 'Y'  ||  *szIIDECIMAL == 'y')
                STcopy("Y",szIIDECIMAL);  /* normalize the value */
            else 
                STcopy("N",szIIDECIMAL);
        }
        else
        if (STbcompare((char *)USESYSTABLES,13, sz, 13, TRUE) == 0 
            && !*szSYSTABLES)
        {
            STlcopy(sz + sizeof USESYSTABLES - 1, szSYSTABLES, 
                sizeof szSYSTABLES - 1);
            if (*szSYSTABLES == 'Y' ||  *szSYSTABLES == 'y')
                STcopy("Y",szSYSTABLES);  /* normalize the value */
            else 
                STcopy("N",szSYSTABLES);
        }
        else
        if (STbcompare((char *)CONVERTINT8TOINT4,18, sz, 18, TRUE) == 0 
            && !*szConvertInt8ToInt4)
        {
            STlcopy(sz + sizeof CONVERTINT8TOINT4 - 1, szConvertInt8ToInt4, 
                sizeof szConvertInt8ToInt4 - 1);
            if (*szConvertInt8ToInt4 == 'Y'  ||  *szConvertInt8ToInt4 == 'y')
                STcopy("Y",szConvertInt8ToInt4);  /* normalize the value */
            else 
                STcopy("N",szConvertInt8ToInt4);
        }
        else
        if (STbcompare((char *)MULTIBYTEFILLCHAR,17, sz, 17, TRUE) == 0 
            && !*szMultibyteFillChar)
        {
            STlcopy(sz + sizeof MULTIBYTEFILLCHAR - 1, szMultibyteFillChar, 
                sizeof (char));
        }
        else
        if (STbcompare((char *)ALLOWUPDATE,11, sz, 11, TRUE) == 0 
            && !*szAllowUpdate)
        {
            STlcopy(sz + sizeof ALLOWUPDATE - 1, szAllowUpdate, 
                sizeof szAllowUpdate - 1);
            if (*szAllowUpdate == 'Y'  ||  *szAllowUpdate == 'y')
                STcopy("Y",szAllowUpdate);  /* normalize the value */
            else 
                STcopy("N",szAllowUpdate);
        }
        else
        if (STbcompare((char *)DEFAULTTOCHAR,15, sz, 15, TRUE) == 0 
            && !*szDefaultToChar)
        {
            STlcopy(sz + sizeof(DEFAULTTOCHAR) - 1, szDefaultToChar, 
                sizeof(szDefaultToChar) - 1);
            if (*szDefaultToChar == 'Y'  ||  *szAllowUpdate == 'y')
                STcopy("Y",szDefaultToChar);  /* normalize the value */
            else 
                STcopy("N",szDefaultToChar);
        }
        else
        if (STbcompare((char *)INGRESDATE,10, sz, 10, TRUE) == 0 
            && !*szIngresDate)
        {
            STlcopy(sz + sizeof(INGRESDATE) - 1, szIngresDate, 
                sizeof(szIngresDate) - 1);
            if (*szIngresDate == 'Y'  ||  *szIngresDate == 'y')
                STcopy("Y",szIngresDate);  /* normalize the value */
            else 
                STcopy("N",szIngresDate);
        }
        else
        if (STbcompare((char *)STRING_TRUNCATION, 18, sz, 18, TRUE) == 0 
            && !*szStringTruncation)
        {
            STlcopy(sz + sizeof(STRING_TRUNCATION) - 1, szStringTruncation, 
                sizeof(szStringTruncation) - 1);
            if (*szStringTruncation == 'Y'  ||  *szStringTruncation == 'y')
                STcopy("Y",szStringTruncation);  /* normalize the value */
            else 
                STcopy("N",szStringTruncation);
        }
    }
        
    //
    //  If no parent window, try to do configuration without dialogs:
    //
    if (!hwnd)
    {
        rc = AutoConfig (fRequest, lpszAttr);
        LeaveCriticalSection (&csCNF);
        return rc;
    }
    
    //
    //  Process request:
    //
    switch (fRequest)
    {
    case ODBC_ADD_DSN:

        rc = CreatePropertySheet (hwnd);
        break;

    case ODBC_CONFIG_DSN:
    
        if (*szDSN != EOS)
        {
            rc = CreatePropertySheet (hwnd);
        }
        break;

    case ODBC_REMOVE_DSN:

        if (*szDSN != EOS)
            rc = SQLRemoveDSNFromIni (szDSN);
        break;
    }

    HtmlHelp(hwnd, szHelp, HH_CLOSE_ALL, 0);
    LeaveCriticalSection (&csCNF);
    
    return rc;
}


//
//  AutoConfig
//
//  Add, change, or delete a data source definition without dialogs.
//
//  On entry: fReq   = type of request (ADD, CONFIG, REMOVE)
//            szAttr-->attribute list (attr=value)
//
//  Returns:  TRUE if success
//            FALSE otherwise
//
BOOL AutoConfig (
    UINT    fReq,
    LPCSTR  szAttr)
{
    char    szKey[80];
	char * p;
    int     i;
    char    tszDBName[LEN_DBNAME + 1];

    switch (fReq)
    {
    case ODBC_ADD_DSN:

        if (*szServer == EOS)
            return FALSE;

        if (!SQLWriteDSNToIni (szDSN, szDriver))
            return FALSE;
// falls through to case ODBC_CONFIG_DSN 

    case ODBC_CONFIG_DSN:

// if case ODBC_CONFIG_DSN and ODBC.INI does not contain dbname for this DSN,
// assume DSN does not exist
    if (fReq == ODBC_CONFIG_DSN)
    {
        *tszDBName = 0;
        SQLGetPrivateProfileString(szDSN, DSN_OPTIONS_DBNAME, "", 
            tszDBName, LEN_DBNAME + 1, ODBC_INI);
        if (!*tszDBName)
            return FALSE;
    }
    if (*szServer)
    {
        wsprintf (szServerSect, "Vnode %s", szServer);
    }

    //
    //  Parse attribute string for valid key names,
    //  and add them to the correct registry section:
    //
    for (; *szAttr != EOS; szAttr += STlength(szAttr), CMnext(szAttr) )
    {
        if (p = (char *) STindex(szAttr, "=" ,0))
        {
            MEfill(sizeof szKey, 0, szKey);
            MEcopy(szAttr, min (p - szAttr, sizeof szKey - 1), szKey );
            szAttr = p;
            CMnext(szAttr);

            for (p=szKey; *p!=EOS; CMnext(p) )  /* conv key to upper case */
                CMtoupper (p, p); 

            for (i = 0; i < KEYTABMAX; i++)
            {
                if (STcompare(szKey, KEYTAB[i].szKey) == 0)
                {
                    switch (KEYTAB[i].fSection)
                    {
                    case SEC_SERVER:
                    if (*szServer)
                        break;
                    
                    break;
                    
                    case SEC_DATASOURCE:
                        if (STcompare(szKey,KEY_DESCRIPTION_UP) ==0)
                            SQLWritePrivateProfileString(szDSN, 
                                DSN_OPTIONS_DESCRIPTION, szAttr, ODBC_INI);
                        else if (STcompare(szKey,KEY_SERVER_UP) ==0 || 
                            STcompare(szKey,KEY_SERVERNAME) ==0 ||
                            STcompare(szKey,KEY_SRVR) ==0)
                                SQLWritePrivateProfileString(szDSN, 
                                    DSN_OPTIONS_SERVER, szAttr, ODBC_INI);
                        else if (STcompare(szKey,KEY_DATABASE_UP)==0 || 
                            strcmp(szKey,KEY_DB)==0)
                                SQLWritePrivateProfileString(szDSN, 
                                    DSN_OPTIONS_DBNAME, szAttr, ODBC_INI);
                        else if (STcompare(szKey,KEY_SERVERTYPE_UP)==0)
                            SQLWritePrivateProfileString(szDSN, 
                                DSN_OPTIONS_SERVERTYPE, szAttr, ODBC_INI);
                        else if (STcompare(szKey,KEY_WITHOPTION)==0)
                            SQLWritePrivateProfileString(szDSN, 
                                DSN_OPTIONS_WITHOPTION, szAttr, ODBC_INI);
                        else if (STcompare(szKey,KEY_ROLENAME)==0)
                            SQLWritePrivateProfileString(szDSN, 
                                DSN_OPTIONS_ROLENAME, szAttr, ODBC_INI);
                        else if (STcompare(szKey,KEY_ROLEPWD)==0)
                        {
                            if (STlength(szRolePWD) > 0)
                                EncryptPWD(szRoleName, szRolePWD); /* encrypts 
                                                                      the pw */
                                SQLWritePrivateProfileString(szDSN, 
                                    DSN_OPTIONS_ROLEPWD,  szRolePWD, ODBC_INI);
                        }
                        else if (STcompare(szKey,KEY_PROMPTUIDPWD)==0)
                            SQLWritePrivateProfileString(szDSN, 
                                DSN_OPTIONS_PROMPT, szPrompt, ODBC_INI);
                        else if (STcompare(szKey,KEY_GROUP)==0)
                            SQLWritePrivateProfileString(szDSN, 
                                DSN_OPTIONS_GROUP, szGroup, ODBC_INI);
                        else if (STcompare(szKey,KEY_READONLY)==0)
                            SQLWritePrivateProfileString(szDSN, 
                                DSN_OPTIONS_READONLY, 
                                    szReadOnly, ODBC_INI);
                        // Begin "Advanced" options
                        else if (STcompare(szKey,KEY_SELECTLOOPS)==0)
                            SQLWritePrivateProfileString(szDSN, 
                                DSN_OPTIONS_SELECTLOOPS, szSelectLoops, 
                                    ODBC_INI);
                        else if (STcompare(szKey,KEY_DISABLECATUNDERSCORE)==0)
                            SQLWritePrivateProfileString(szDSN, 
                                DSN_OPTIONS_CATUNDERSCORE,
                                    szCatUnderscoreDisabled, ODBC_INI);
                        else if (STcompare(szKey,KEY_CONVERTTHREEPARTNAMES)==0)
                            SQLWritePrivateProfileString(szDSN, 
                                DSN_OPTIONS_3_PART_NAMES, 
                                    sz3PartNames, ODBC_INI);
                        else if (STcompare(szKey,KEY_USESYSTABLES)==0)
                            SQLWritePrivateProfileString(szDSN, 
                                DSN_OPTIONS_SYSTABLES, szSYSTABLES, ODBC_INI);
                        else if (STcompare(szKey,KEY_BLANKDATE)==0)
                            SQLWritePrivateProfileString(szDSN, 
                                DSN_OPTIONS_BLANKDATE, szBlankDate, ODBC_INI);
                        else if (STcompare(szKey,KEY_DATE1582)==0)
                            SQLWritePrivateProfileString(szDSN, 
                                DSN_OPTIONS_DATE1582, szDate1582, ODBC_INI);
                        else if (STcompare(szKey,KEY_CATCONNECT)==0)
                            SQLWritePrivateProfileString(szDSN, 
                                DSN_OPTIONS_CATCONNECT, szCatConnect, ODBC_INI);
                        else if (STcompare(szKey,KEY_NUMERIC_OVERFLOW)==0)
                            SQLWritePrivateProfileString(szDSN, 
                                DSN_OPTIONS_NUMOVERFLOW, szNumOverflow, 
                                    ODBC_INI);
                        else if (STcompare(szKey,KEY_SUPPORTIIDECIMAL)==0)
                            SQLWritePrivateProfileString(szDSN, 
                                DSN_OPTIONS_IIDECIMAL, szIIDECIMAL, ODBC_INI);
                        else if (STcompare(szKey,KEY_CATSCHEMANULL)==0)
                            SQLWritePrivateProfileString(szDSN, 
                                DSN_OPTIONS_CATSCHEMANULL, szCatSchemaNULL, 
                                    ODBC_INI);
                        else if (STcompare(szKey,KEY_ALLOWPROCEDUREUPDATE)==0)
                            SQLWritePrivateProfileString(szDSN, 
                                DSN_OPTIONS_ALLOWPROCEDUREUPDATE,
                                    szAllowProcedureUpdate, ODBC_INI);
                        else if (STcompare(szKey,KEY_MULTIBYTEFILLCHAR)==0)
                            SQLWritePrivateProfileString(szDSN, 
                                DSN_OPTIONS_MULTIBYTEFILLCHAR, 
                                    szMultibyteFillChar, ODBC_INI);
                        else if (STcompare(szKey,KEY_CONVERTINT8TOINT4)==0)
                            SQLWritePrivateProfileString(szDSN, 
                                DSN_OPTIONS_CONVERTINT8TOINT4,
                                    szConvertInt8ToInt4, ODBC_INI);
                        else if (STcompare(szKey,KEY_ALLOWUPDATE)==0)
                            SQLWritePrivateProfileString(szDSN, 
                                DSN_OPTIONS_ALLOWUPDATE, szAllowUpdate, 
                                    ODBC_INI);
                        else if (!STcompare(szKey,KEY_DEFAULTTOCHAR))
                            SQLWritePrivateProfileString(szDSN,
                                DSN_OPTIONS_DEFAULTTOCHAR, 
                                    szDefaultToChar, ODBC_INI);
                        else if (!STcompare(szKey,KEY_INGRESDATE))
                            SQLWritePrivateProfileString(szDSN,
                                DSN_OPTIONS_INGRESDATE, szIngresDate, ODBC_INI);
                        else if (!STcompare(szKey,KEY_STRING_TRUNCATION))
                            SQLWritePrivateProfileString(szDSN,
                                DSN_OPTIONS_STRINGTRUNCATION, 
                                    szStringTruncation, ODBC_INI);
                                    
                        break;

                    case SEC_OPTIONS:

                        break;
                    }
                    break;
                }
            }
        }
    }
    return TRUE;

    case ODBC_REMOVE_DSN:

        if (*szDSN != EOS)
            return SQLRemoveDSNFromIni (szDSN);

        if (*szServer != EOS)
            wsprintf (szServerSect, "Vnode %s", szServer);

    default:

        return FALSE;
    }
}

//
//  Create Property Sheet
//
//  Create property sheet containing tab dialogs for configuration options.
//
//  On entry: globals contain driver name and request type
//
//  Returns:  return code from PropertySheet function which calls dialog procedures
//
int CreatePropertySheet (HWND hwnd)
{
    int     i = 0;
	UCHAR  * lpM;

    //
    //  Set up propety sheet tabs:
    //

	lpM = (UCHAR  *) ERget(F_OD0114_IDS_TAB_DATASOURCE);
	STcopy(lpM,szTabTitle[i]);
    psp[i].dwFlags     = PSP_USETITLE | PSP_HASHELP;
    psp[i].pszTemplate = MAKEINTRESOURCE(DLG_DATASOURCE);
    psp[i].pfnDlgProc  = (DLGPROC)DataSourceProc;
    CreatePropertyTab (i++);

	lpM = (UCHAR  *) ERget(F_OD0116_IDS_TAB_ADVANCED);
	STcopy(lpM,szTabTitle[i]);
    psp[i].dwFlags     = PSP_USETITLE | PSP_HASHELP;

    psp[i].pszTemplate = MAKEINTRESOURCE(DLG_ADVANCED);
    psp[i].pfnDlgProc  = (DLGPROC)AdvancedProc; 

    CreatePropertyTab (i++);

	lpM = (UCHAR  *) ERget(F_OD0115_IDS_TAB_ABOUT);
	STcopy(lpM,szTabTitle[i]);
    psp[i].dwFlags     = PSP_USETITLE;
    psp[i].pszTemplate = MAKEINTRESOURCE(DLG_ABOUT);
    psp[i].pfnDlgProc  = (DLGPROC)AboutProc;
    CreatePropertyTab (i++);

    //
    //  Set up property sheet header:
    //
	lpM = (UCHAR  *) ERget(F_OD0113_IDS_TAB_CAPTION);
    wsprintf (szTabCaption, lpM, szDriver);
    psh.dwSize     = sizeof (PROPSHEETHEADER);
    psh.dwFlags    = PSP_USETITLE | PSH_HASHELP;
    psh.hwndParent = hwnd;
    psh.hInstance  = hInstance;
    psh.pszIcon    = NULL;
    psh.pszCaption = szTabCaption; 
    psh.nPages     = i;
    psh.ppsp       = (LPCPROPSHEETPAGE) &psp;

    return (PropertySheet (&psh) > 0) ? TRUE : FALSE;
}

//
//  CreatePropTab
//
//  Initialize property sheet tab common stuff.
//
//  On entry: i = psp index
//
//  Returns:  Nothing
//
VOID CreatePropertyTab (int i)
{
    psp[i].dwSize      = sizeof (PROPSHEETPAGE);
    psp[i].hInstance   = hInstance;
    psp[i].pszIcon     = NULL;
    psp[i].pszTitle    = szTabTitle[i];
    psp[i].lParam      = 0;
    return;
}

void MessageString (HWND hDlg, int ids, UINT flag)
{
	
	UCHAR  * lpM;

    lpM = (UCHAR  *) ERget(ids);
	MessageBox (hDlg, lpM, szMsgTitle, flag);

    return;
}

//
//  MsgConfirm
//
//  Display confirmation message box.
//
//  On entry: hdlg    = dialog window handle.
//            pszType-->type of object to delete.
//            pszName-->name of object to delete.
//
//  Returns:  TRUE to confirm.
//            FALSE to cancel.
//
BOOL MsgConfirm (
    HWND    hDlg,
    LPCSTR  lpszType,
    LPSTR   lpszName)
{
	UCHAR  * lpM; 
	
    lpM = (UCHAR  *) ERget(F_OD0124_IDS_MSG_CONFIRM);
    wsprintf (szMessage, lpM, lpszType, lpszName);
    return (MessageBox (hDlg, szMessage, szMsgTitle, MB_OKCANCEL) == IDOK);
}

//
//  SetDriverInfo
//
//  On entry: szDriver = driver type
//            (global parameter)
//
//  Returns:  TRUE if known driver type
//            FALSE otherwise
//
BOOL SetDriverInfo(LPCSTR driver)
{
    OFFSET_TYPE    cb;
    LPSTR   lpszHelp;
    CHAR    *lpIngDir;
    UCHAR  * lpM;
    char    szDriverUPP[32];
	char	buf[MAX_PATH];
	int		len;
    //
    //  Set message titles
    //
    STcopy(szDriver,szMsgTitle);
    cb = STlength(szMsgTitle);
    lpM = (UCHAR  *) ERget(F_OD0128_IDS_MSG_TITLE);
    STcopy(lpM,szMsgTitle + cb);

    STcopy(szDriver,szErrTitle);
    cb = STlength(szErrTitle);
    lpM = (UCHAR  *) ERget(F_OD0117_IDS_ERR_TITLE);
    STcopy(lpM,szErrTitle + cb);

    STcopy(szDriver,szDriverUPP);   /* work with driver name in upper case*/
    CVupper(szDriverUPP);

    lpszHelp = "\\caiiocfg.chm";

	len = SQLGetPrivateProfileString(driver, "Driver", "", buf, sizeof(buf), ODBCINST_INI);
	if (len>0)
	{
		lpIngDir=&(buf[len]);
		while (strncmp(lpIngDir, "\\", 1) != 0)
		{
			*lpIngDir='\0';
			lpIngDir--;
		}
		*lpIngDir='\0';
		lpIngDir=&buf;
	}

    if (lpIngDir != NULL)
        STcopy(lpIngDir,szPath);

    STcopy(szPath,szHelp);           /* help path */
    STcat(szHelp, lpszHelp);

    fDriver = IDU_IDMS;                      /* driver type */

    STcopy(szDriver,szProduct);              /* product name */
    STcat(szProduct, " ODBC Driver");

    STcopy("Version 3.5",szProductVersion);  /* product name */


    cbDictLen = LEN_DBNAME;

    return TRUE;
}
