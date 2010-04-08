/*
** Copyright (c) 1993, 2009 Ingres Corporation
*/

/*
** Name: odbccfg.h
**
** Description:
**      External ODBC configuration data structures and definitions.
**
** History:
**      02-Jun-03 (loera01)
**          Created.
**	11-Jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**      17-feb-2005 (Ralph.Loen@ca.com)  Sir 113913
**          Added "DSN" string definition.
**      18-jul-2005 (loera01) Bug 114869
**          Added DSN_ATTR_GROUP and DSN_ATTR_DBMS_PWD.
**      18-jan-2006 (loera01) Bug 115615
**          Re-instituted "Computer Associates" macro defintion for backward
**          compatibility.  Added new DRV_ATTR_INGRES_CORP attribute.
**      16-mar-2006 (Ralph Loen) Bug 115833
**          Added DSN_ATTR_MULTIBYTE_FILL_CHAR.
**      13-Mar-2007 (Ralph Loen) SIR 117786
**          Added DRV_ATTR_POOL_TIMEOUT and DRV_ATTR_NEG_ONE.
**      17-June-2008 (Ralph Loen) Bug 120506
**          Renamed DRV_STR_CAIPT_TR_FILE as DRV_STR_ALT_TRACE_FILE.
**      14-Nov-2008 (Ralph Loen) SIR 121228
**          Add support for OPT_INGRESDATE configuration parameter.  If set,
**          ODBC date escape syntax is converted to ingresdate syntax, and
**          parameters converted to IIAPI_DATE_TYPE are converted instead to
**          IIAPI_DTE_TYPE.  The new attributes DSN_ATTR_INGRESDATE
**          and DSN_STR_INGRESDATE support OPT_INGRESDATE on non-Windows
**          platforms.
**     28-Apr-2009 (Ralph Loen) SIR 122007
**          Add DSN_ATTR_STRING_TRUNCATION and DSN_STR_STRING_TRUNCATION.
*/

#ifndef __II_OCFG_HDR
#define __II_OCFG_HDR

/*
** Recognized values for alternate path argument in openConfig().
*/
# define OCFG_SYS_PATH                  0  /* System (default) path */
# define OCFG_ALT_PATH                  1  /* Alternate system path */
# define OCFG_USR_PATH                  2  /* User config path */

/*
** Global defines.
*/
# define OCFG_UNKNOWN                  -1
# define OCFG_MAX_STRLEN              256

/*
** Driver manager attribute values and associated strings.
*/
# define MGR_ATTR_PATH                 0
# define MGR_STR_PATH               "DriverManagerPath"
# define MGR_ATTR_MANAGER              1
# define MGR_STR_MANAGER            "DriverManager"

# define MGR_ATTR_MAX                  3

# define MGR_VALUE_UNIX_ODBC           0
# define MGR_STR_UNIX_ODBC          "unixODBC"
# define MGR_VALUE_CAI_PT              1
# define MGR_STR_CAI_PT             "CAI/PT"
# define MGR_VALUE_FILE_NAME           2
# define MGR_STR_FILE_NAME          "odbcmgr.dat"

# define MGR_VALUE_MAX                 3

/*
** ODBC information attribute values and associated strings.
*/
# define INFO_ATTR_DRIVER_FILE_NAME    0
# define INFO_STR_DRIVER_FILE_NAME   "DriverFileName"
# define INFO_ATTR_RONLY_DRV_FNAME     1
# define INFO_STR_RONLY_DRV_FNAME    "ReadOnlyDriverFileName"
# define INFO_ATTR_DRIVER_NAME         2
# define INFO_STR_DRIVER_NAME        "DriverName"
# define INFO_ATTR_ALT_DRIVER_NAME     3
# define INFO_STR_ALT_DRIVER_NAME    "AltDriverName"

# define INFO_ATTR_MAX                 4

# define INFO_VALUE_FILE_NAME          0 
# define INFO_STR_FILE_NAME          "ocfginfo.dat"

# define INFO_VALUE_MAX                1

/*
** Driver attribute values and associated strings.
*/
# define DRV_ATTR_ODBC_DRIVERS          0  
# define DRV_STR_ODBC_DRIVERS        "ODBC Drivers" 
# define DRV_ATTR_INGRES                1 
# define DRV_STR_INGRES              "Ingres" 
# define DRV_ATTR_DONT_DLCLOSE          2 
# define DRV_STR_DONT_DLCLOSE        "DontDlClose" 
# define DRV_ATTR_DRIVER                3 
# define DRV_STR_DRIVER              "Driver" 
# define DRV_ATTR_DRIVER_ODBC_VER       4 
# define DRV_STR_DRIVER_ODBC_VER     "DriverODBCVer" 
# define DRV_ATTR_DRIVER_INTERNAL_VER   5 
# define DRV_STR_DRIVER_INTERNAL_VER "DriverInternalVer" 
# define DRV_ATTR_DRIVER_READONLY       6 
# define DRV_STR_DRIVER_READONLY     "DriverReadOnly" 
# define DRV_ATTR_DRIVER_TYPE           7 
# define DRV_STR_DRIVER_TYPE         "DriverType" 
# define DRV_ATTR_VENDOR                8 
# define DRV_STR_VENDOR              "Vendor" 
# define DRV_ATTR_ODBC                  9 
# define DRV_STR_ODBC                "ODBC"
# define DRV_ATTR_TRACE                10 
# define DRV_STR_TRACE              "Trace"  
# define DRV_ATTR_TRACE_FILE           11 
# define DRV_STR_TRACE_FILE         "Trace File"
# define DRV_ATTR_CAIPT_TR_FILE        12 
# define DRV_STR_ALT_TRACE_FILE      "TraceFile"
# define DRV_ATTR_POOL_TIMEOUT         13
# define DRV_STR_POOL_TIMEOUT       "PoolTimeout"

# define DRV_ATTR_MAX               14

# define DRV_VALUE_INSTALLED       0
# define DRV_STR_INSTALLED      "Installed"
# define DRV_VALUE_ZERO            1
# define DRV_STR_ZERO           "0"
# define DRV_VALUE_ONE             2
# define DRV_STR_ONE            "1"
# define DRV_VALUE_N               3 
# define DRV_STR_N              "N"
# define DRV_VALUE_Y               4
# define DRV_STR_Y              "Y"
# define DRV_VALUE_NO              5
# define DRV_STR_NO             "No"
# define DRV_VALUE_YES             6
# define DRV_STR_YES            "Yes"
# define DRV_VALUE_INGRES          7 /* DRV_STR_INGRES defined above */
# define DRV_VALUE_COMP_ASSOC      8
# define DRV_STR_COMP_ASSOC     "Computer Associates"
# define DRV_VALUE_INGRES_CORP     9
# define DRV_STR_INGRES_CORP    "Ingres Corporation"
# define DRV_VALUE_VERSION        10
# define DRV_STR_VERSION        "3.50"
# define DRV_VALUE_NEG_ONE        11 
# define DRV_STR_NEG_ONE        "-1"

# define DRV_VALUE_MAX            12

/*
** DSN attribute values and associated strings.
*/
# define DSN_ATTR_ODBC_DATA_SOURCES       0
# define DSN_STR_ODBC_DATA_SOURCES        "ODBC Data Sources"
# define DSN_ATTR_DRIVER                  1
# define DSN_STR_DRIVER                   "Driver"
# define DSN_ATTR_DESCRIPTION             2
# define DSN_STR_DESCRIPTION              "Description"
# define DSN_ATTR_VENDOR                  3
# define DSN_STR_VENDOR                   DRV_STR_VENDOR
# define DSN_ATTR_DRIVER_TYPE             4
# define DSN_STR_DRIVER_TYPE              DRV_STR_DRIVER_TYPE
# define DSN_ATTR_SERVER                  5
# define DSN_STR_SERVER                   "Server"
# define DSN_ATTR_DATABASE                6
# define DSN_STR_DATABASE                 "Database"
# define DSN_ATTR_SERVER_TYPE             7
# define DSN_STR_SERVER_TYPE              "ServerType"
# define DSN_ATTR_PROMPT_UID_PWD          8
# define DSN_STR_PROMPT_UID_PWD           "PromptUIDPWD"
# define DSN_ATTR_WITH_OPTION             9
# define DSN_STR_WITH_OPTION              "WithOption"
# define DSN_ATTR_ROLE_NAME              10
# define DSN_STR_ROLE_NAME                "RoleName"
# define DSN_ATTR_ROLE_PWD               11
# define DSN_STR_ROLE_PWD                 "RolePWD"
# define DSN_ATTR_DISABLE_CAT_UNDERSCORE 12
# define DSN_STR_DISABLE_CAT_UNDERSCORE   "DisableCatUnderscore"
# define DSN_ATTR_ALLOW_PROCEDURE_UPDATE 13
# define DSN_STR_ALLOW_PROCEDURE_UPDATE   "AllowProcedureUpdate"
# define DSN_ATTR_USE_SYS_TABLES         14
# define DSN_STR_USE_SYS_TABLES           "UseSysTables"
# define DSN_ATTR_BLANK_DATE             15
# define DSN_STR_BLANK_DATE               "BlankDate"
# define DSN_ATTR_DATE_1582              16
# define DSN_STR_DATE_1582                "Date1582"
# define DSN_ATTR_CAT_CONNECT            17
# define DSN_STR_CAT_CONNECT              "CatConnect"
# define DSN_ATTR_NUMERIC_OVERFLOW       18
# define DSN_STR_NUMERIC_OVERFLOW         "Numeric_overflow"
# define DSN_ATTR_SUPPORT_II_DECIMAL     19
# define DSN_STR_SUPPORT_II_DECIMAL       "SupportIIDECIMAL"
# define DSN_ATTR_CAT_SCHEMA_NULL        20
# define DSN_STR_CAT_SCHEMA_NULL          "CatSchemaNULL"
# define DSN_ATTR_READ_ONLY              21
# define DSN_STR_READ_ONLY                "ReadOnly"
# define DSN_ATTR_SELECT_LOOPS           22
# define DSN_STR_SELECT_LOOPS             "SelectLoops"
# define DSN_ATTR_CONVERT_3_PART         23
# define DSN_STR_CONVERT_3_PART           "ConvertThreePartNames"
# define DSN_ATTR_DBMS_PWD               24
# define DSN_STR_DBMS_PWD                 "Dbms_password"
# define DSN_ATTR_GROUP                  25
# define DSN_STR_GROUP                    "Group"
# define DSN_ATTR_MULTIBYTE_FILL_CHAR    26
# define DSN_STR_MULTIBYTE_FILL_CHAR      "MultibyteFillChar"                   
# define DSN_ATTR_INGRESDATE             27
# define DSN_STR_INGRESDATE               "IngresDate"
# define DSN_ATTR_STRING_TRUNCATION      28
# define DSN_STR_STRING_TRUNCATION        "StringTruncation"

# define DSN_ATTR_MAX                    29

# define DSN_VALUE_N            0
# define DSN_STR_N              DRV_STR_N 
# define DSN_VALUE_Y            1
# define DSN_STR_Y              DRV_STR_Y 
# define DSN_VALUE_NO           2
# define DSN_STR_IGNORE         "IGNORE"
# define DSN_VALUE_FAIL         3
# define DSN_STR_FAIL           "FAIL"
# define DSN_VALUE_SELECT       4
# define DSN_STR_SELECT         "SELECT"
# define DSN_VALUE_CURSOR       5
# define DSN_STR_CURSOR         "CURSOR"
# define DSN_VALUE_INGRES       6
# define DSN_STR_INGRES         DRV_STR_INGRES
# define DSN_VALUE_COMP_ASSOC   7
# define DSN_STR_COMP_ASSOC     DRV_STR_COMP_ASSOC
# define DSN_VALUE_INGRES_CORP  8
# define DSN_STR_INGRES_CORP    DRV_STR_INGRES_CORP
# define DSN_VALUE_LOCAL        9
# define DSN_STR_LOCAL          "(local)"
# define DSN_VALUE_EMPTY_STR    10
# define DSN_STR_EMPTY_STR      "\0"

# define DSN_VALUE_MAX          11

# define DSN_STR_DSN            "DSN"

/*
** Name: ATTR_ID - Formatted ODBC configuration entries
**
** Description:
**      This structure defines the formatted attribute list for the
**      unixODBC configuration routines.
**
** History:
**      02-Jun-03 (loera01)
**          Created.
*/

typedef struct _ATTR_ID
{
    i4 id;                     /* Formatted attribute ID */
    char *value;               /* Value of attribute ID */
} ATTR_ID;

/*
** Name: ATTR_NAME - Unformatted ODBC configuration entries
**
** Description:
**      This structure defines the unformatted attribute list for the
**      unixODBC configuration routines.
**
** History:
**      02-Jun-03 (loera01)
**          Created.
*/

typedef struct _ATTR_NAME
{ 
    char *name;              /* Unformatted attribute name */
    char *value;             /* Value of unformatted attribute */
} ATTR_NAME;

/*
** Global reference for driver manager listed in ingres/files/odbcmgr.dat.
*/
GLOBALREF i2 driverManager;
/*
** External templates for ODBC configuration routines.
*/
void openConfig( PTR inputHandle, i4 type, char * path, 
    PTR * outputHandle, STATUS *status );

void closeConfig( PTR handle, STATUS * status );

i4 listConfig( PTR handle, i4 count, char **list, STATUS * status );

i4 getConfigEntry( PTR handle, char * name, i4 count, 
    ATTR_ID **attrlist, STATUS * status );

void setConfigEntry( PTR handle, char * name, i4 count, 
    ATTR_ID **attrlist, STATUS *status );

void delConfigEntry( PTR handle, char * name, STATUS * status );

i4 getEntryAttribute( PTR handle, char *name, i4 count, 
    ATTR_NAME **attrlist, STATUS *status );

bool getTrace( PTR handle, char *path, STATUS *status );
void setTrace( PTR handle, bool trace_on, char *path, STATUS *status );
i4 getTimeout( PTR handle );
void setTimeout( PTR handle, i4 timeout, STATUS *status );
#endif /* __II_OCFG_HDR */
