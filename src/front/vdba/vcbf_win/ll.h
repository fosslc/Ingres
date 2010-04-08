/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
** 
**    Source   : ll.h, header File 
**    Project  : OpenIngres Configuration Manager 
**    Author   : 
**    Purpose  :
**
** History:
**
** 06-Jun-2000: (uk$so01) 
**    (BUG #99242)
**    Cleanup code for DBCS compliant
** 05-Apr-2001 (uk$so01)
**    (SIR 103548) only allow one vcbf per installation ID
** 02-Oct-2002 (noifr01)
**    (SIR 107587) added COMP_TYPE_GW_DB2UDB definition for DB2 UDB gateway
** 20-jun-2003 (uk$so01)
**    SIR #110389 / INGDBA210
**    Add functionalities for configuring the GCF Security and
**    the security mechanisms.
** 07-Jul-2003 (schph01)
**    (bug 106858) consistently with CBF change 455479
**     Add bDisplayAllTabs in COMPONENTINFO structure.
** 25-Sep-2003 (uk$so01)
**    SIR #105781 of cbf
** 12-Mar-2004 (uk$so01)
**    SIR #110013 (Handle the DAS Server)
** 19-May-2004 (lakvi01)
**	  Added bLogFilesizesEqual1 and bLogFilesizesEqual2 to LPTRANSACTINFO to consider
**	  the rare case when Transaction Log files are not equal. 
** 08-Jun-2004 (lakvi01)
**	  BUG# 111984, added the command line parameters for VCBFllinit.
** 10-Nov-2004 (noifr01)
**    (bug 113412) added prototype of new GetTransLogCompName() and 
**    GetLocDefConfName() functions
**/

#ifndef _VCBF_LL_INCLUDED
#define _VCBF_LL_INCLUDED

//#ifdef __cplusplus
//extern "C"
//{
//#endif

BOOL VCBFllinit(char* Str_cmd);
BOOL VCBFllterminate(void);

typedef struct install_infos {
    char	*host;
    char	*ii_system;
    char	*ii_installation;
    char	*ii_system_name;
    char	*ii_installation_name;
  } SHOSTINFO, FAR *LPSHOSTINFO;

BOOL VCBFGetHostInfos(LPSHOSTINFO lphostinfo);

#define GEN_FORM_SECURE_SECURE 1
#define GEN_FORM_SECURE_C2     2
#define GEN_FORM_SECURE_GCF    3

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
#define COMP_TYPE_GW_DB2UDB        22
#define COMP_TYPE_DASVR            23



typedef struct component_infos {
    int  itype;
    char	sztype[50];
    char	szname[80];
    char	szcount[4];
    BOOL blog1;
    BOOL blog2;
    int  ispecialtype;   /* for differentiating generic forms */
    BOOL bDisplayAllTabs; /* With a Ingres client installation for "Security"*/
                          /* branch display only "System" and "Mechanisms"*/
                          /* tabs instead all the tabs*/
  } COMPONENTINFO, FAR *LPCOMPONENTINFO;

BOOL VCBFllGetFirstComp(LPCOMPONENTINFO lpcomp) ;
BOOL VCBFllGetNextComp (LPCOMPONENTINFO lpcomp) ;

#define GEN_LINE_TYPE_BOOL   1
#define GEN_LINE_TYPE_NUM    2
#define GEN_LINE_TYPE_SPIN   3
#define GEN_LINE_TYPE_STRING 4
#define GEN_LINE_TYPE_NUM_NOTNEG 5  // should refuse leading minus sign
#define GEN_LINE_TYPE_SPIN_NOTNEG 6 // should refuse leading minus sign

typedef struct generic_form_info {
    char	szname [60];
    char	szvalue[120];
    char	szunit [80];
    int iunittype;
  } GENERICLINEINFO, FAR *LPGENERICLINEINFO;

BOOL VCBFllGenLinesRead(void);
BOOL VCBFllAltLinesRead(void);
BOOL VCBFllGetFirstGenericLine(LPGENERICLINEINFO lpgeninfo) ;
BOOL VCBFllGetNextGenericLine(LPGENERICLINEINFO lpgeninfo) ;
BOOL VCBFllGetFirstGenCacheLine(LPGENERICLINEINFO lpgeninfo) ;
BOOL VCBFllGetNextGenCacheLine(LPGENERICLINEINFO lpgeninfo) ;
BOOL VCBFllGetFirstAdvancedLine(LPGENERICLINEINFO lpgeninfo); 
BOOL VCBFllGetNextAdvancedLine(LPGENERICLINEINFO lpgeninfo) ;

typedef struct audit_log_info {
    int log_n;
    char	szvalue[256];
  } AUDITLOGINFO, FAR *LPAUDITLOGINFO;

BOOL VCBFllGetFirstAuditLogLine(LPAUDITLOGINFO lpauditlog) ;
BOOL VCBFllGetNextAuditLogLine(LPAUDITLOGINFO lpauditlog) ;

typedef struct cache_line_info {
    char szcachename[80];
    char	szstatus[40];
  } CACHELINEINFO, FAR *LPCACHELINEINFO;

BOOL VCBFllGetFirstCacheLine(LPCACHELINEINFO lpcacheline) ;
BOOL VCBFllGetNextCacheLine(LPCACHELINEINFO lpcacheline) ;

typedef struct derived_line_info {
    char	szname  [60];
    char	szvalue[120];
    char	szunit  [80];
    int iunittype;
    char szprotected[30];
  } DERIVEDLINEINFO, FAR *LPDERIVEDLINEINFO;

BOOL VCBFllGetFirstDerivedLine(LPDERIVEDLINEINFO lpderivinfo) ;
BOOL VCBFllGetNextDerivedLine(LPDERIVEDLINEINFO lpderivinfo) ;

typedef enum {NET_PROTOCOL, NET_REGISTRY, DAS_PROTOCOL} protocol_type;
typedef struct netprot_line_info {
    char szprotocol[60];
    char	szstatus  [40];
    char	szlisten [120];
    protocol_type nProtocolType;
  } NETPROTLINEINFO, FAR *LPNETPROTLINEINFO;

BOOL VCBFllGetFirstNetProtLine(LPNETPROTLINEINFO lpnetprotline) ;
BOOL VCBFllGetNextNetProtLine(LPNETPROTLINEINFO lpnetprotline) ;

typedef struct bridgeprot_line_info {
    char szprotocol[60];
    char	szstatus  [40];
    char	szlisten [120];
    char	szvnode  [120];
  } BRIDGEPROTLINEINFO, FAR *LPBRIDGEPROTLINEINFO;

BOOL VCBFllGetFirstBridgeProtLine(LPBRIDGEPROTLINEINFO lpbridgeprotline) ;
BOOL VCBFllGetNextBridgeProtLine (LPBRIDGEPROTLINEINFO lpbridgeprotline) ;

#define  LG_MAX_FILE 16

typedef struct tx_info {
    char szstate  [200];

    // Primary Log Info
    //    char szpath1   [LG_MAX_FILE + 1][256];
    char szsize1   [25];
    char szenabled1[25];
    char szraw1    [25];
    char szRootPaths1    [LG_MAX_FILE + 1][256];
    char szLogFileNames1 [LG_MAX_FILE + 1][256];
    char szConfigDatLogFileName1 [512];
    int  iConfigDatLogFileSize1;
    int  iNumberOfLogFiles1;
    // Secondary Log Info
    //    char szpath2  [100];
    char szsize2   [25];
    char szenabled2[25];
    char szraw2    [25];
    char szRootPaths2    [LG_MAX_FILE + 1][256];
    char szLogFileNames2 [LG_MAX_FILE + 1][256];
    char szConfigDatLogFileName2 [512];
    int  iConfigDatLogFileSize2;
    int  iNumberOfLogFiles2;

    BOOL bEnableReformat;
    BOOL bEnableDisable;
    BOOL bEnableErase;
    BOOL bEnableDestroy;
    BOOL bEnableCreate;
    BOOL bStartOnFirstOnInit;

	BOOL bLogFilesizesEqual1;
	BOOL bLogFilesizesEqual2;

    void (*fnUpdateGraph)();
    void (*fnSetText)(LPCTSTR);

  } TRANSACTINFO, FAR *LPTRANSACTINFO;


BOOL VCBFllCanCountBeEdited(LPCOMPONENTINFO lpcomponent)    ;
BOOL VCBFllCanNameBeEdited(LPCOMPONENTINFO lpcomponent)     ;
BOOL VCBFllCanCompBeDuplicated(LPCOMPONENTINFO lpcomponent) ;
BOOL VCBFllCanCompBeDeleted(LPCOMPONENTINFO lpcomponent)    ;
BOOL VCBFllValidCount(LPCOMPONENTINFO lpcomponent, char * in_buf)  ;
BOOL VCBFllValidRename(LPCOMPONENTINFO lpcomponent, char * in_buf) ;
BOOL VCBFllValidDuplicate(LPCOMPONENTINFO lpcomponentsrc, LPCOMPONENTINFO lpcomponentdest,char * in_buf);
BOOL VCBFllValidDelete(LPCOMPONENTINFO lpcomponent);
/** ====> IMPORTANT: special management if trans.log configured  <==== **/

extern BOOL bExitRequested;
BOOL VCBFRequestExit();

BOOL VCBFllInitGenericPane( LPCOMPONENTINFO lpcomp);    // to be called when activated line has first pane of type generic
BOOL VCBFllGenPaneOnEdit( LPGENERICLINEINFO lpgenlineinfo, char * lpnewvalue);
BOOL VCBFllAuditLogOnEdit( LPAUDITLOGINFO lpauditloglineinfo, char * lpnewvalue);
BOOL VCBFllResetGenValue(LPGENERICLINEINFO lpgenlineinfo) ;
BOOL VCBFllResetAuditValue(LPAUDITLOGINFO lpauditloglineinfo);
BOOL VCBFllOnMainComponentExit(LPCOMPONENTINFO lpcomp); // to be called when deactivating line which has first pane of type generic

// for cache pane
BOOL VCBFllInitCaches();
BOOL VCBFllOnCacheEdit(LPCACHELINEINFO lpcachelineinfo, char * newvalue);

// functions for params pane for a given cache
BOOL VCBFllInitCacheParms(LPCACHELINEINFO lpcacheline);//generic cache
BOOL VCBFllCachePaneOnEdit( LPGENERICLINEINFO lpgenlineinfo, char * lpnewvalue);
BOOL VCBFllCacheResetGenValue(LPGENERICLINEINFO lpgenlineinfo) ;

BOOL VCBFllInitDependent(BOOL bCacheDep);

/******=> reload after change ****/
BOOL VCBBllOnEditDependent(LPDERIVEDLINEINFO lpderivinfo, char * newval);

BOOL VCBBllOnDependentProtectChange(LPDERIVEDLINEINFO lpderivinfo, char * newval);

/******=> reload after change ****/
BOOL VCBFllDepOnRecalc(LPDERIVEDLINEINFO lpderivinfo);

char* VCBFllLoadHist (void);
BOOL VCBFllFreeString( char* szFreeBufName);

char *VCBFll_dblist_init();
BOOL VCBFllOnDBListExit(char * db_list_buf,BOOL changed);

BOOL VCBFll_netprots_init(protocol_type nType = NET_PROTOCOL);
BOOL VCBFll_netprots_OnEditStatus(LPNETPROTLINEINFO lpnetprotline,char * in_buf, protocol_type nType);
BOOL VCBFll_netprots_OnEditListen(LPNETPROTLINEINFO lpnetprotline,char * in_buf, protocol_type nType);

BOOL VCBFll_bridgeprots_init();
BOOL VCBFll_bridgeprots_OnEditStatus(LPBRIDGEPROTLINEINFO lpbridgeprotline,char * in_buf);
BOOL VCBFll_bridgeprots_OnEditListen(LPBRIDGEPROTLINEINFO lpbridgeprotline,char * in_buf);
BOOL VCBFll_bridgeprots_OnEditVnode(LPBRIDGEPROTLINEINFO lpbridgeprotline,char * in_buf) ;


BOOL VCBFll_tx_init(LPCOMPONENTINFO lpcompinfo,LPTRANSACTINFO lptransactinfo);
BOOL VCBFll_tx_OnTimer(LPTRANSACTINFO lptransactinfo);
BOOL VCBFll_tx_OnCreate(BOOL bPrimary,LPTRANSACTINFO lptransactinfo,char *lpsizeifwasempty);
BOOL VCBFll_tx_OnReformat(BOOL bPrimary,LPTRANSACTINFO lptransactinfo);
BOOL VCBFll_tx_OnDisable(BOOL bPrimary,LPTRANSACTINFO lptransactinfo);
BOOL VCBFll_tx_OnErase(BOOL bPrimary,LPTRANSACTINFO lptransactinfo);
BOOL VCBFll_tx_OnDestroy(BOOL bPrimary,LPTRANSACTINFO lptransactinfo);
BOOL VCBFll_tx_OnExit(LPTRANSACTINFO lptransactinfo,LPCOMPONENTINFO lpLog1, LPCOMPONENTINFO lpLog2);
BOOL VCBFll_tx_OnActivatePrimaryField(LPTRANSACTINFO lptransactinfo);
BOOL VCBFll_tx_OnActivateSecondaryField(LPTRANSACTINFO lptransactinfo);

BOOL VCBFll_settings_Init(BOOL *pbSyscheck);
BOOL VCBFll_On_settings_CheckChanged(BOOL bChecked,BOOL *pbCheckedResult);
BOOL VCBFll_settings_Terminate();
BOOL VCBFll_Init_net_Form(void);
BOOL VCBFll_Init_name_Form(void);
BOOL VCBFll_Form_GetParm(int ictrl,char * buf);
BOOL VCBFll_Form_OnEditParm(int ictrl,char * in_buf, char * bufresult);

// for O.I. Desktop server.
//BOOL VCBFllInitDskServComponent( LPCOMPONENTINFO lpcomp);
//BOOL VCBFllTerminateDskServComponent( LPCOMPONENTINFO lpcomp); // please refresh line after calling this function
//BOOL VCBFllDskServOnEdit( LPGENERICLINEINFO lpgenlineinfo, char * lpnewvalue); // ok for both tabs
//BOOL VCBFllDskServResetValue(LPGENERICLINEINFO lpgenlineinfo);                 // ok for both tabs
// (to get the list of parameters for Desktop server:
//  for tab 1 ("parameters") use the VCBFllGetFirst/next/GenericLine  stuff
//  for tab 2 ("advanced")   use the VCBFllGetFirst/next/AdvancedLine stuff)

BOOL VCBFll_OnStartupCountChange(char *szOldCount);

#define NET_CTRL_INBOUND  1 /* Don't change these values:      */
#define NET_CTRL_OUTBOUND 2 /* should start with 1 and end     */
#define NET_CTRL_LOGLEVEL 3 /* with same value as NET_MAX_PARM */

#define NET_MAX_PARM 3

#define NAME_CHECK_INTERVAL    1   /* Don't change these values:       */
#define NAME_DEFAULT_SVR_CLASS 2   /* should start with 1 and end      */
#define NAME_LOCAL_VNODE       3   /* with same value as NAME_MAX_PARM */
#define NAME_SESSION_LIMIT     4

#define NAME_MAX_PARM          4


BOOL IsValidDatabaseName (LPCTSTR dbName);
void VCBFll_Security_machanism_get(CPtrList& listMech);
BOOL VCBFll_Security_machanism_edit(LPCTSTR lpszMech, LPTSTR lpszValue);
CString INGRESII_QueryInstallationID();
BOOL BRIDGE_AddVNode(char* addvnode, char* addport, char* addprotocol);
BOOL BRIDGE_DelVNode(char* addvnode, char* addport, char* addprotocol);
BOOL BRIDGE_ChangeStatusVNode(char* vnode, char* protocol, BOOL bOnOff);
//
// BRIDGE_CheckValide returns
// 0 : success
// 1 : vnode error
// 2 : protocol error
// 3 : listen address error:
int  BRIDGE_CheckValide(CString& strVNode, CString& strProtocol, CString& strListenAddress);
void BRIDGE_protocol_list(char* net_id, CStringList& listProtocols);
void BRIDGE_QueryVNode(CPtrList& listObj);

//#ifdef __cplusplus
//}
//#endif

char * GetTransLogCompName();
char * GetLocDefConfName();

#endif //_VCBF_LL_INCLUDED

