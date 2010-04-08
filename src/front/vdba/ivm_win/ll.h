/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Source   : ll.h , Header File
**  Project  : Ingres II / Visual Manager
**  Author   : Francois Noirot-Nerin (noifr01)
**
**  History (after 08-mar-2000):
** 
** 08-Mar-2000 (uk$so01)
**    SIR #100706. Provide the different context help id.
**    (Handle the generic folder of gateways).
**  18-Jan-2001 (noifr01)
**   (SIR 103727) manage JDBC server
**  21-sep-2001 (somsa01)
**	Added InitMessageSystem().
**  28-May-2002 (noifr01)
**   (sir 107879) moved the prototype of the IVM_GetFileSize() function,
**   and added isDBMS() function prototype
**  02-Oct-2002 (noifr01)
**    (SIR 107587) added COMP_TYPE_GW_DB2UDB definition for DB2 UDB gateway
**  25-Nov-2002 (noifr01)
**    (bug 109143) management of gateway messages
** 09-Mar-2004 (uk$so01)
**    SIR #110013 (Handle the DAS Server)
** 10-Nov-2004 (noifr01)
**    (bug 113412) added prototype of new GetLocDefConfName() function
**    (returning internationalized version of the "(default)" (configuration)
**    string)
*/

#ifndef _VCBF_LL_INCLUDED
#define _VCBF_LL_INCLUDED

BOOL VCBFllinit(void);
BOOL VCBFllterminate(void);

typedef struct install_infos {
    char	*host;
    char	*ii_system;
    char	*ii_installation;
    char	*ii_system_name;
    char	*ii_installation_name;
  } SHOSTINFO, FAR *LPSHOSTINFO;

BOOL VCBFGetHostInfos(LPSHOSTINFO lphostinfo);

#define COMP_TYPE_NAME              1
#define COMP_TYPE_DBMS              2
#define COMP_TYPE_INTERNET_COM      3
#define COMP_TYPE_NET               4
#define COMP_TYPE_BRIDGE            5
#define COMP_TYPE_STAR              6
#define COMP_TYPE_SECURITY          7
#define COMP_TYPE_LOCKING_SYSTEM    8
#define COMP_TYPE_LOGGING_SYSTEM    9
#define COMP_TYPE_TRANSACTION_LOG  10
#define COMP_TYPE_RECOVERY         11
#define COMP_TYPE_ARCHIVER         12
#define COMP_TYPE_RMCMD            13
#define COMP_TYPE_GW_ORACLE        14
#define COMP_TYPE_GW_INFORMIX      15
#define COMP_TYPE_GW_SYBASE        16
#define COMP_TYPE_GW_MSSQL         17
#define COMP_TYPE_GW_ODBC          18

#define COMP_TYPE_SETTINGS         19

#define COMP_TYPE_OIDSK_DBMS       20
#define COMP_TYPE_JDBC             21

#define COMP_TYPE_GW_FOLDER        22 // Use for help id only

#define COMP_TYPE_GW_DB2UDB        23
#define COMP_TYPE_DASVR            24



typedef struct component_infos {
    int  itype;
    char	sztype[50];
    char	sztypesingle[50];
    char	szname[80];
    char	szcount[4];
    BOOL blog1;
    BOOL blog2;
    int  ispecialtype;   /* for differentiating generic forms */
  } COMPONENTINFO, FAR *LPCOMPONENTINFO;

BOOL VCBFllGetFirstComp(LPCOMPONENTINFO lpcomp) ;
BOOL VCBFllGetNextComp (LPCOMPONENTINFO lpcomp) ;

BOOL VCBFllCanCompBeDuplicated(LPCOMPONENTINFO lpcomponent) ;

BOOL VCBFRequestExit();

char * GetNameSvrCompName();
char * GetICESvrCompName();
char * GetLogSysCompName();
char * GetLockSysCompName();
char * GetArchProcessCompName();
char * GetTransLogCompName();
char * GetSecurityCompName();
char * GetRecoverySvrCompName();
char * GetDBMSCompName();
char * GetNetSvrCompName();
char * GetJDBCSvrCompName();
char * GetDASVRSvrCompName();
char * GetStarServCompName();
char * GetBridgeSvrCompName();
char * GetRmcmdCompName();
char * GetLoggingSystemCompName();
char * GetLockingSystemCompName();

char * GetOracleGWName();
char * GetDB2UDBGWName();
char * GetInformixGWName();
char * GetSybaseGWName();
char * GetMSSQLGWName();
char * GetODBCGWName();

char * oneGW();
char * GWParentBranchString();

BOOL GetCompTypeAndNameFromInstanceName(char * pinstancename, // input instance string
										char *bufResType,     // buffer where to store the Comp Type
										char * bufResName);   // buffer where to store the Comp Name

void Request4SvrClassRefresh();

// functions to be called for embedded loops of GetFirstComp/GetNextComp.
// if these functions were to be called in concurrent threads, another technique should be used.
BOOL Push4GettingCompList();
BOOL Pop4GettingCompList();

BOOL IsSpecialInstance(char * pinstancename, char * bufResType,char * bufResName);
BOOL isDBMS();

extern BOOL Mysystem(char* lpCmd);

extern VOID InitMessageSystem();

long IVM_GetFileSize( char *FileName);

char *GetLocDefConfName();

#endif //_VCBF_LL_INCLUDED

