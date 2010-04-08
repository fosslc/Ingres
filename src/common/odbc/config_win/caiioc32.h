//
//  (C) Copyright 1993,2009 by Ingres Corporation
//  CAIIOC32.H
//
//  32 bit Ingres Configuration Utility header file.
//
//
/*  Change History
//  --------------
//
//  date        programmer          description
//
//   5/11/1993  Dave Ross          Initial coding
//   7/29/1993  Dave Ross          RPB "standard" stuff
//   9/07/1993  Julie Kessler      Added ENABLEd and COMMITBEHAVIOR
//   9/23/1993  Julie Kessler      Added DLG_SETUP
//   1/04/1994  Dave Ross          Added version 3 stuff
//   1/26/1994  Dave Ross          Create from idmsetup.h
//  11/29/1994  Dave Ross          Moved all user strings to resource file
//   7/17/1996  Dave Ross          Copied from caidcnfg.h
//  03/06/1997  Jean Teng          Added szPrompt
//  05/08/1997  Jean Teng          Added MAX_OBJECT_NAME
//  11/07/1997  Jean Teng          Added szRoleName and szRolePWD
//  12/01/1997  Jean Teng          Added AdvancedProc and szSelectLoops
//  12/15/1997  Dave Thole         Added sz3PartNames
//  03/19/1999  Bobby Ward         Added NMgtat to help II_HOSTNAME change
//  11/15/1999  Dave Thole         Added szBlankDate
//  01/12/2000  Dave Thole         Added szDate1582
//  04/27/2000  Dave Thole         Added szCatConnect and szSelectLoops
//  06/05/2000  Dave Thole         Added numeric_overflow= support
//  10/16/2000  Ralph Loen         Added support for II_DECIMAL (Bug 102666).
//  01/16/2002  Dave Thole         Add support for DisableCatUnderscore search char.
//  04/18/2002  Fei Wei            Bug #106831: add option in advanced tab 
//                                 of ODBC admin which allows the procedure 
//                                 update with odbc read only driver. 
//  05/22/2002  Ralph Loen         Bug 107814: Put DSN configuration constants
//                                 into this file, rather than duplicate
//                                 definitions in caiioc32.c and confgdsn.c. 
**      14-feb-2003 (loera01) SIR 109643
**          Minimize platform-dependency in code.  Make backward-compatible
**          with earlier Ingres versions.
**      07-mar-2003 (loera01) SIR 109789
**          Add DSN_OPTIONS_READONLY and DSN_OPTIONS_DRIVER_READONLY.
**      01-jul-2003  (weife01) SIR #110511
**          Add support for group ID in DSN.
**      10-Mar-2006 (Ralph Loen) Bug 115833
**          Added support for new DSN configuration option: "Fill character
**          for failed Unicode/multibyte conversions".
**      20-Sep-2007 (weife01) SIR 119140
**          Added support for new DSN/connection string configuration
**          option:"Convert int8 from DBMS to int4".
**      01-apr-2008 (weife01) Bug 120177
**          Added DSN configuration option "Allow update when use pass through
**          query in MSAccess".
**      14-Nov-2008 (Ralph Loen) SIR 121228
**          Added szIngresDate and DSN_OPTIONS_INGRESDATE global variables.
**     28-Apr-2009 (Ralph Loen) SIR 122007
**          Added DSN_OPTIONS_STRINGTRUNCATION and szStringTruncation.
*/
#ifndef _INC_CAIIOC32
#define _INC_CAIIOC32

#include <windows.h>
#include <commdlg.h>               // Common dialog box functions.

#ifndef _WINDOWS
#define _WINDOWS
#endif
#define _WINDLL

#define LEN_MESSAGE         512
#define LEN_WORK            (MAX_PATH * 2)

#define MAX_OBJECT_NAME     (32 + 1)

#define RC_ERROR            -1
#define RC_OK               0
#define RC_CANCEL           1
#define RC_DELETE           2

//
//  Icon resource ids:
//
//  (not used)
//

//
//  Driver attributes flag bits
//
#define DAF_DICTIONARY       0x0001
#define DAF_BE_TRACE         0x0002
#define DAF_CECP_USED        0x0004
#define DAF_DBCS_USED        0x0008
#define DAF_CADB_OPTIONS     0x0010
#define DAF_REP_READ         0x0020
#define DAF_PRESERVE_CURSOR  0x0040
#define DAF_REAL_AS_DOUBLE   0x0080
#define DAF_ACCT_PROMPT      0x0100
#define DAF_ACC_TABLES       0x0200

#ifndef _CNFG
//
//  Global constants:
//
extern const char DSN[];
extern const char INI_DRIVERS[];

extern const char ERR_EXEC[];
extern const char ERR_ADDSTRING[];
extern const char ERR_ALLOC[];
extern const char ERR_SELECT[];

extern const char MSG_SELECT[];
extern const char MSG_REQUIRED[];
extern const char MSG_CONFIRM[];
extern const char MSG_DSN[];
extern const char MSG_DBNAME[];
extern const char MSG_KEEP[];
extern const char MSG_SERVER[];
extern const char MSG_SERVERTYPE[];
extern const char MSG_CONNECTFAILED[];
extern const char MSG_CONNECTOK[];

extern const char INI_FALSE[];
extern const char INI_TRUE[];

extern const char INI_ACCESSIBLE[];
extern const char INI_ACCT_PROMPT[];
extern const char INI_CACHE_TABLES[];
extern const char INI_CECP[];
extern const char INI_COMMIT_BEHAVIOR[];
extern const char INI_DBCS_PATH[];
extern const char INI_DBCS_TYPE[];
extern const char INI_DBCS_TYPES[];
extern const char INI_DEFAULTS[];
extern const char INI_DESCRIPTION[];
extern const char INI_DICTIONARY[];
extern const char INI_DOUBLE[];
extern const char INI_ENABLE_ENSURE[];
extern const char INI_INPUT_ENABLE[];
extern const char INI_KATAKANA[];
extern const char INI_LOG_FILE[];
extern const char INI_LOG_OPTIONS[];
extern const char INI_OPTIONS[];
extern const char INI_NODE[];
extern const char INI_PATH[];
extern const char INI_RESOURCE[];
extern const char INI_READONLY[];
extern const char INI_ROWS[];
extern const char INI_VNODE[];
extern const char INI_SERVERS[];
extern const char INI_TASK[];
extern const char INI_TIMEOUT[];
extern const char INI_TXNISOLATION[];
extern const char INI_VIEW[];

//
//  Global variables:
//
extern HINSTANCE            hInstance;
extern CRITICAL_SECTION     csCNF;
extern OSVERSIONINFO        os;

extern const char szErrTitle[];
extern const char szHelp[MAX_PATH];
// extern const char szName[MAX_PATH];
extern const char szPath[MAX_PATH];         
extern const char szMsgTitle[];

extern char szDBName[];
extern char szDescription[];
extern char szDSN[];
extern char szMessage[LEN_MESSAGE];
extern char szServer[];
extern char szServerType[];
extern char szVnodeSect[];
extern char szDriver[];
extern char szProduct[];
extern char szProductVersion[];
// extern char szTitle[];
extern char szWork[LEN_WORK];
extern char szPrompt[];
extern char szReadOnly[];
extern char szDriverReadOnly[];
extern char szWithOption[];
extern char szRoleName[];
extern char szRolePWD[];
extern char szGroup[];
extern char szSelectLoops[];
extern char sz3PartNames[6];
extern char szSYSTABLES[6];
extern char szBlankDate[20];
extern char szDate1582[20];
extern char szCatConnect[6];
extern char szNumOverflow[20];
extern char szIIDECIMAL[6];
extern char szCatSchemaNULL[6];
extern char szCatUnderscoreDisabled[4];
extern char szAllowProcedureUpdate[4];
extern char szMultibyteFillChar[4];
extern char szConvertInt8ToInt4[4];
extern char szAllowUpdate[4];
extern char szDefaultToChar[4];
extern char szIngresDate[4];
extern char szStringTruncation[4];


extern BOOL fDSN;
extern BOOL fIDMS;
extern BOOL fUNIX;
extern BOOL fCADB;
extern BOOL fGetServer;
extern UINT fDriver;
extern UINT fDriverAttrib;
extern UINT fRequest;
extern UINT idDialogLog;
extern UINT idDialogOptions;
extern UINT idDialogServer;
extern UINT cbDictLen;

#define DAF_IDMS          DAF_ACC_TABLES           | \
                          DAF_CECP_USED            | \
                          DAF_DBCS_USED            | \
                          DAF_DICTIONARY           | \
                          DAF_PRESERVE_CURSOR      | \
                          DAF_REAL_AS_DOUBLE       | \
                          DAF_ACCT_PROMPT

#define DAF_IDMSUNIX      DAF_CADB_OPTIONS         | \
                          DAF_BE_TRACE             | \
                          DAF_REP_READ

#define DAF_CADBUNIX      DAF_CADB_OPTIONS         | \
                          DAF_BE_TRACE             | \
                          DAF_REP_READ

#define DAF_DATACOM       DAF_CADB_OPTIONS         | \
                          DAF_BE_TRACE             | \
                          DAF_REP_READ

#endif // not _CNFG

//
//  Function prototypes:
//
BOOL    ExecApp (HWND, UINT, LPSTR);
void    MessageString (HWND, int, UINT);
BOOL    MsgConfirm (HWND, LPCSTR, LPSTR);
LRESULT SetDlgListString (HWND, int, UINT, int);
void    SetDlgTextString (HWND, int, int);
void    ClearDSNCacheCapabilities(char * szDSN);
void    WriteDSNCacheCapabilities(char * szDSN, char * ServerName, char * ServerType);

//
//  Windows procedures:
//
BOOL CALLBACK AboutProc (HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK AtoeProc (HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DataSourceProc (HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK AdvancedProc (HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DefaultsProc (HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK LogOptionsProc (HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK OptionsProc (HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK ServerProc (HWND, UINT, WPARAM, LPARAM);


//
//  DSN Configuration constants
//
static const char DSN_OPTIONS_DESCRIPTION[]	= "Description";
static const char DSN_OPTIONS_SERVER[]	= "Server";
static const char DSN_OPTIONS_SERVERTYPE[]	= "ServerType";
static const char DSN_OPTIONS_DBNAME[]	= "Database";
static const char DSN_OPTIONS_PROMPT[]  = "PromptUIDPWD";
static const char DSN_OPTIONS_GROUP[]  = "Group";
static const char DSN_OPTIONS_READONLY[]  = "ReadOnly";
static const char DSN_OPTIONS_DRIVER_READONLY[]  = "DriverReadOnly";
static const char DSN_OPTIONS_WITHOPTION[] = "WithOption";
static const char DSN_OPTIONS_ROLENAME[]   = "RoleName";
static const char DSN_OPTIONS_ROLEPWD[]    = "RolePWD";
static const char DSN_OPTIONS_SELECTLOOPS[]= "SelectLoops";
static const char DSN_OPTIONS_3_PART_NAMES[]="ConvertThreePartNames";
static const char DSN_OPTIONS_SYSTABLES[]="UseSysTables";
static const char DSN_OPTIONS_BLANKDATE[]   ="BlankDate";
static const char DSN_OPTIONS_DATE1582[]    ="Date1582";
static const char DSN_OPTIONS_CATCONNECT[]  ="CatConnect";
static const char DSN_OPTIONS_NUMOVERFLOW[] ="Numeric_overflow";
static const char DSN_OPTIONS_IIDECIMAL[]="SupportIIDECIMAL";
static const char DSN_OPTIONS_CATSCHEMANULL[]="CatSchemaNULL";
static const char DSN_OPTIONS_CATUNDERSCORE[]="DisableCatUnderscore";
static const char DSN_OPTIONS_ALLOWPROCEDUREUPDATE[]="AllowProcedureUpdate";
static const char DSN_OPTIONS_MULTIBYTEFILLCHAR[]="MultibyteFillChar";
static const char DSN_OPTIONS_CONVERTINT8TOINT4[]="ConvertInt8ToInt4";
static const char DSN_OPTIONS_ALLOWUPDATE[]="AllowUpdate";
static const char DSN_OPTIONS_DEFAULTTOCHAR[]="DefaultToChar";
static const char DSN_OPTIONS_INGRESDATE[]="IngresDate";
static const char DSN_OPTIONS_STRINGTRUNCATION[]="StringTruncation";
#endif  // _INC_CAIIOC32
