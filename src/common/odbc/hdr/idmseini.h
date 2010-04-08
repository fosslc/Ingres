/*
**  IDMSEINI
**
**  Module Name:    idmseini.h
**
**  Copyright (c) 2009 Ingres Corporation
**
**  description:    CAIDMS.INI Setup definitions used in ODBC, QCLI, DTS et al
**
**   Change History
**   --------------
**
**   date        programmer          description
**
**   05/28/1993  Dave Ross           Initial coding.
**   08/26/1993  Dave Ross           Added options for ACCESS.
**   10/13/1993  Dave Ross           Added options for DLCI.
**   03/03/1997  Jean Teng           Changed LEN_NODE from 8 to 32
**   03/06/1997  Jean Teng           Added LEN_PROMPT
**   11/07/1997  Jean Teng           Added LEN_ROLENAME and LEN_ROLEPWD
**   12/01/1997  Jean Teng           Added LEN_SELECTLOOPS
**   12/09/1997  Dave Thole          Changed LEN_ROLEPWD from 32 to 64 for string write
**	 05/29/1998	 Dmitriy Bubis		 Added valid keynames for ini file section
**   12/21/1998  Dave Thole          Added KEY_SERVERCLASSES
**   03/29/1999  Dave Thole          Added SYSTEM_LOCATION_VARIABLE
**   03/30/1999  Dave Thole          Added other product support
**   01/19/2001  Dave Thole          Increased LEN_NODE to 128 for nodeless connect
**   08/17/2001  Dave Thole          Added VANT (CA-Vantage) server class
**   05/22/2002  Ralph Loen          Bug 107814: Add "advanced" DSN keys.
**   06/11/2002  Ralph Loen          Fixed C++-style comments.
**   06/27/2002  Fei Wei             Add DB2UDB to the server classes.
**   03/14/2003  Ralph Loen          Added common definition for INI files.
**   06/02/2003  Ralph Loen          Moved ScanPastSpaces(), GetOdbcIniToken(),
**                                   and getDriverVersion() templates 
**                                   to this file.
**   28-jan-2004 (loera01)
**       Added boolean argument to getFileToken() for VMS.
**   11-Jun-2004 (somsa01)
**	 Cleaned up code for Open Source.
**   18-jan-2006 (loera01) SIR 115615
**       Removed outmoded registry references.
**   10-Mar-2006 (Ralph Loen) Bug 115833
**       Added support for new DSN configuration option: "Fill character
**       for failed Unicode/multibyte conversions".
**   03-Jul-2006 (Ralph Loen) SIR 116326
**       Added KEY_DRIVERNAMES to support SQLBrowseConnect().
**   30-Jan-2007 (Ralph Loen) Bug 117569
**       Rename the driver name "Ingres 3.0" to "Ingres 2006".
**   19-Oct-2007 (Ralph Loen) Bug 119329
**       GetDriverVersion() now has a "path" input argument.
**   01-apr-2008 (weife01) Bug 120177
**       Added DSN configuration option "Allow update when use pass through
**       query in MSAccess".
**   12-May-2008 (Ralph Loen) Bug 120356
**       Add KEY_DEFAULTTOCHAR and LEN_DEFAULTTOCHAR.
**   24-June-2008 (Ralph Loen) Bug 120551
**       Removed support for obsolete server types such as
**       ALB, OPINGDT, etc.
**   25-June-2008 (Ralph Loen) Bug 120551
**       Re-added VANT (CA-Vantage).
**   15-Sept-2008 (Ralph Loen) Bug 120518
**       Rename the driver name "Ingres 2006" to "Ingres 9.3".
**     14-Nov-2008 (Ralph Loen) SIR 121228
**       Added support KEY_INGRESDATE and LEN_INGRESDATE.  Removed
**       obsolete key definitions.
**   23-Mar-2009 (Ralph Loen) Bug 121838
**       Added SQL_API_MIN_LEVEL_3_VALUE, SQL_API_MAX_LEVEL_3_VALUE,
**       and SQL_API_MAX_LEVEL_2_VALUE for SQLGetFunctions().
**     28-Apr-2009 (Ralph Loen) SIR 122007
**         Add KEY_STRING_TRUNCATION.
**   08-Jun-2009 (Ralph Loen) Bug 122142
**       Rename Ingres driver name as "Ingres 9.4".
**   04-Aug-2009 (Ralph Loen) Bug 122268
**       Rename Ingres driver name as "Ingres 10.0".
**   29-Jan-2010 (Ralph Loen) Bug 123175
**       Add KEY_SENDDATETIMEASINGRESDATE.
*/

#ifndef _INC_IDMSEINI
#define _INC_IDMSEINI

#define ODBC_INI "odbc.ini"
#define ODBCINST_INI "odbcinst.ini"
#define ODBC_DSN "ODBC Data Sources"

/*
**  Ini file lengths:
*/
#define LEN_ATOE            256
#define LEN_DBNAME          32
#define LEN_DESCRIPTION     255
#define LEN_DICTIONARY      16
#define LEN_DRIVER_NAME     32
#define LEN_SERVER          32
#define LEN_DSN             32
#define LEN_SERVERTYPE      32
#define LEN_ROLENAME        32
#define LEN_ROLEPWD         64
#define LEN_GROUP           32
#define LEN_PROMPT          1
#define LEN_SELECTLOOPS     1
#define LEN_LOGFILE         255
#define LEN_NODE            128
#define LEN_PRODUCT_NAME    40
#define LEN_RESOURCE        8
#define LEN_SYSTEMID        8
#define LEN_TASK            8
#define LEN_VIEW            37
#define LEN_VERSION_STRING  16
#define LEN_XREFILE         255
#define LEN_IDMS_NAME       LEN_DRIVER_NAME + 1
#define LEN_DSNSECT         LEN_DSN + 1
#define LEN_NODESECT        LEN_NODE + 5
#define LEN_RESOURCESECT    LEN_DRIVER_NAME + 9
#define LEN_DRIVERSECT       LEN_DRIVER_NAME + 9
#define LEN_DRIVER_TYPE      LEN_DRIVER_NAME
#define LEN_IIDECIMAL       1
#define LEN_CONVERTINT8TOINT4 1
#define LEN_ALLOWUPDATE 1
#define LEN_DEFAULTTOCHAR 1
#define LEN_INGRESDATE 1

/*
**  Ini file section and key names:
*/
#define KEY_DSN				"DSN"
#define KEY_DESCRIPTION_UP	"DESCRIPTION"
#define KEY_SERVER_UP       "SERVER"
#define KEY_SERVERTYPE_UP   "SERVERTYPE"
#define KEY_DATABASE_UP     "DATABASE"
#define KEY_ROLENAME        "ROLENAME"
#define KEY_ROLEPWD	        "ROLEPWD"
#define KEY_GROUP           "GROUP"
#define KEY_PROMPT	        "PROMPT"
#define KEY_WITHOPTION      "WITHOPTION"
#define KEY_PROMPTUIDPWD    "PROMPTUIDPWD"
#define KEY_DATASOURCENAME	"DATASOURCENAME"
#define KEY_SERVERNAME      "SERVERNAME"
#define KEY_SRVR	        "SRVR"
#define KEY_DB		        "DB"
#define KEY_DEFAULTS        "Defaults"

#define KEY_OPTIONS         "Options"

#define KEY_SERVER          "Server"
#define KEY_SERVERTYPE      "ServerType"
#define KEY_DESCRIPTION     "Description"
#define KEY_DATABASE        "Database"

#define KEY_ACCESSIBLE      "AccessibleTables"
#define KEY_ACCESSINFO      "AccessInfo"
#define KEY_ACCESSTYPE      "AccessType"
#define KEY_ACCT_PROMPT     "AccountPrompt"
#define KEY_ATOE            "AsciiEbcdicTables"
#define KEY_AUTOCOMMIT      "AutoCommit"
#define KEY_CACHE_PROC      "CacheProcAddresses"
#define KEY_CACHE_TABLES    "CacheSQLTables"
#define KEY_CLASS           "ClassOfService"
#define KEY_COLUMNS         "CatalogColumns"
#define KEY_COMMIT_BEHAVIOR "CommitBehavior"
#define KEY_CUSTOM          "Custom"
#define KEY_DATACOM         "DATACOM"
#define KEY_DATASOURCES     "Data Sources"
#define KEY_DBNAME          "Dbname"
#define KEY_DBNAMES         "Dbnames"
#define KEY_DEFAULT         "Default"

#define KEY_MAXTIMEOUT      "MaxTimeOut"
#define KEY_NAME            "Name"
#define KEY_NODE            "Node"
#define KEY_NODES           "Nodes"
#define KEY_READONLY        "ReadOnly"
#define KEY_SERVERS         "Servers"
#define KEY_SUBSCHEMA       "Subschema"
#define KEY_TEMP_DSN        "DriverConnectTemp"
#define KEY_TEMP_SERVER     "Server DriverConnectTemp"
#define KEY_TXNISOLATION    "TxnIsolation"
#define KEY_TRACE_ODBC      "OdbcTrace"

/* "Advanced" options */

#define KEY_SELECTLOOPS "SELECTLOOPS"
#define KEY_DISABLECATUNDERSCORE "DISABLECATUNDERSCORE"
#define KEY_ALLOWPROCEDUREUPDATE "ALLOWPROCEDUREUPDATE"
#define KEY_MULTIBYTEFILLCHAR "MULTIBYTEFILLCHAR"
#define KEY_CONVERTTHREEPARTNAMES "CONVERTTHREEPARTNAMES"
#define KEY_USESYSTABLES "USESYSTABLES"
#define KEY_BLANKDATE "BLANKDATE"
#define KEY_DATE1582 "DATE1582"
#define KEY_CATCONNECT "CATCONNECT"
#define KEY_NUMERIC_OVERFLOW "NUMERIC_OVERFLOW"
#define KEY_SUPPORTIIDECIMAL "SUPPORTIIDECIMAL"
#define KEY_CATSCHEMANULL "CATSCHEMANULL"
#define KEY_CONVERTINT8TOINT4 "CONVERTINT8TOINT4"
#define KEY_ALLOWUPDATE "ALLOWUPDATE"
#define KEY_DEFAULTTOCHAR "DEFAULTTOCHAR"
#define KEY_INGRESDATE "INGRESDATE"
#define KEY_STRING_TRUNCATION "STRING_TRUNCATION"
#define KEY_SENDDATETIMEASINGRESDATE "SENDDATETIMEASINGRESDATE"

#define KEY_LOG_FILE        "LogFile"
#define KEY_LOG_OPTIONS     "LogOptions"

#define KEY_SERVERCLASSES   { \
    "Ingres", "DCOM (Datacom)", "IDMS","DB2", "IMS", "VSAM", \
    "RDB (Rdb/VMS)", "STAR", "RMS", "Oracle", "Informix", \
    "Sybase", "MSSQL (MS SQL Server)", "VANT (CA-Vantage)", "DB2UDB", NULL }

#define KEY_DRIVERNAMES { \
    "Ingres", "Ingres 10.0", NULL}
#ifndef SYSTEM_LOCATION_VARIABLE   /* normally defined in gl.h */
# define SYSTEM_LOCATION_VARIABLE      "II_SYSTEM"
# define SYSTEM_LOCATION_SUBDIRECTORY  "ingres"
# define SYSTEM_LOCATION_HOSTNAME      "II_HOSTNAME"
#endif /* SYSTEM_LOCATION_VARIABLE */

# define SERVER_TYPE_DEFAULT "INGRES"

# define SQL_API_MIN_LEVEL_3_VALUE 1000
# define SQL_API_MAX_LEVEL_3_VALUE 1050
# define SQL_API_MAX_LEVEL_2_VALUE 100

/*
** External routines for reading INI files.
*/
char * ScanPastSpaces(char *p);
char * GetOdbcIniToken(char *p, char * szToken);
void getDriverVersion( char *p);

/*
** The next two routines are the equivalent of ScanPastSpaces() and
** GetOdbcIniToken(). They are being maintained indepenently until
** a common odbc utility library is created for the driver,
** configuration, and administrator modules.
*/
char * skipSpaces (char *);
char * getFileToken (char *, char *, bool);

char **getDefaultInfo();
char *getMgrInfo(bool *sysIniDefined);

#endif   /* _INC_IDMSEINI */
