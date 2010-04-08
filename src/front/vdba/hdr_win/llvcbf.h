/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
** 
**    Source   : ll.h, header File 
**    Project  : OpenIngres Configuration Manager 
**    Author   : Vijayendra R. Lakkundi
**    Purpose  : For libbk library
**
** History:
**
** 06-Jun-2004: (lakvi01) 
**    SIR #112040, Created from ll.h of VCBF.
**/

#ifndef _VCBF_LL_INCLUDED
#define _VCBF_LL_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif

//int fnDispcallback(char* st1, char* st2, bool bYesNO);
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

#define GEN_LINE_TYPE_BOOL   1
#define GEN_LINE_TYPE_NUM    2
#define GEN_LINE_TYPE_SPIN   3
#define GEN_LINE_TYPE_STRING 4
#define GEN_LINE_TYPE_NUM_NOTNEG 5  // should refuse leading minus sign
#define GEN_LINE_TYPE_SPIN_NOTNEG 6 // should refuse leading minus sign
#define  LG_MAX_FILE 16

typedef enum {NET_PROTOCOL, NET_REGISTRY, DAS_PROTOCOL} protocol_type;

extern bool bExitRequested;
#define NET_CTRL_INBOUND  1 /* Don't change these values:      */
#define NET_CTRL_OUTBOUND 2 /* should start with 1 and end     */
#define NET_CTRL_LOGLEVEL 3 /* with same value as NET_MAX_PARM */

#define NET_MAX_PARM 3

#define NAME_CHECK_INTERVAL    1   /* Don't change these values:       */
#define NAME_DEFAULT_SVR_CLASS 2   /* should start with 1 and end      */
#define NAME_LOCAL_VNODE       3   /* with same value as NAME_MAX_PARM */
#define NAME_SESSION_LIMIT     4

#define NAME_MAX_PARM          4

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

    bool bEnableReformat;
    bool bEnableDisable;
    bool bEnableErase;
    bool bEnableDestroy;
    bool bEnableCreate;
    bool bStartOnFirstOnInit;

	bool bLogFilesizesEqual1;
	bool bLogFilesizesEqual2;
 
    void (*fnUpdateGraph)(void);
    void (*fnSetText)(char*);//lakvi01-removed LPCTSTR here!!

  } LIBBK_TRANSACTINFO, *LPLIBBK_TRANSACTINFO;

typedef struct component_infos {
    int  itype;
    char	sztype[50];
    char	szname[80];
    char	szcount[4];
    bool blog1;
    bool blog2;
    int  ispecialtype;   /* for differentiating generic forms */
    bool bDisplayAllTabs; /* With a Ingres client installation for "Security"*/
                          /* branch display only "System" and "Mechanisms"*/
                          /* tabs instead all the tabs*/
  } LIBBK_COMPONENTINFO, *LPLIBBK_COMPONENTINFO;


typedef struct install_infos {
    char	*host;
    char	*ii_system;
    char	*ii_installation;
    char	*ii_system_name;
    char	*ii_installation_name;
  } LIBBK_SHOSTINFO, *LPLIBBK_SHOSTINFO;

typedef struct generic_form_info {
    char	szname [60];
    char	szvalue[120];
    char	szunit [80];
    int iunittype;
  } LIBBK_GENERICLINEINFO, *LPLIBBK_GENERICLINEINFO;

typedef struct audit_log_info {
    int log_n;
    char	szvalue[256];
  } LIBBK_AUDITLOGINFO, *LPLIBBK_AUDITLOGINFO;

typedef struct cache_line_info {
    char szcachename[80];
    char	szstatus[40];
  } LIBBK_CACHELINEINFO, *LPLIBBK_CACHELINEINFO;

typedef struct derived_line_info {
    char	szname  [60];
    char	szvalue[120];
    char	szunit  [80];
    int iunittype;
    char szprotected[30];
  } LIBBK_DERIVEDLINEINFO,  *LPLIBBK_DERIVEDLINEINFO;

typedef struct netprot_line_info {
    char szprotocol[60];
    char	szstatus  [40];
    char	szlisten [120];
    protocol_type nProtocolType;
  } LIBBK_NETPROTLINEINFO,  *LPLIBBK_NETPROTLINEINFO;

typedef struct bridgeprot_line_info {
    char szprotocol[60];
    char	szstatus  [40];
    char	szlisten [120];
    char	szvnode  [120];
  } LIBBK_BRIDGEPROTLINEINFO,  *LPLIBBK_BRIDGEPROTLINEINFO;
typedef struct list_element2
{
	LIBBK_BRIDGEPROTLINEINFO b1;
	struct list_element2* next1;
}item_bridge;
typedef struct list_element3
{
	LIBBK_GENERICLINEINFO* ln;
	struct list_element3* next;
}item_lineinfo;

typedef struct list_element1
{
	char* cstr;
	struct list_element1* next;
}item_string;

bool LIBBK_VCBFllGetFirstComp(LPLIBBK_COMPONENTINFO lpcomp) ;
bool LIBBK_VCBFllGetNextComp (LPLIBBK_COMPONENTINFO lpcomp) ;
bool LIBBK_VCBFllinit(char* cmdln, int (*dispcallback)(char*, char*, bool), bool (*exit_function)(void));

#define MSG_REPLY_YES	0
#define MSG_REPLY_NO	1
#define MSG_REPLY_CANCEL	2

bool LIBBK_VCBFllterminate(void);

bool LIBBK_VCBFGetHostInfos(LPLIBBK_SHOSTINFO lphostinfo);

bool LIBBK_VCBFllGenLinesRead(void);
bool LIBBK_VCBFllAltLinesRead(void);
bool LIBBK_VCBFllGetFirstGenericLine(LPLIBBK_GENERICLINEINFO lpgeninfo) ;
bool LIBBK_VCBFllGetNextGenericLine(LPLIBBK_GENERICLINEINFO lpgeninfo) ;
bool LIBBK_VCBFllGetFirstGenCacheLine(LPLIBBK_GENERICLINEINFO lpgeninfo) ;
bool LIBBK_VCBFllGetNextGenCacheLine(LPLIBBK_GENERICLINEINFO lpgeninfo) ;
bool LIBBK_VCBFllGetFirstAdvancedLine(LPLIBBK_GENERICLINEINFO lpgeninfo); 
bool LIBBK_VCBFllGetNextAdvancedLine(LPLIBBK_GENERICLINEINFO lpgeninfo) ;


bool LIBBK_VCBFllGetFirstAuditLogLine(LPLIBBK_AUDITLOGINFO lpauditlog) ;
bool LIBBK_VCBFllGetNextAuditLogLine(LPLIBBK_AUDITLOGINFO lpauditlog) ;


bool LIBBK_VCBFllGetFirstCacheLine(LPLIBBK_CACHELINEINFO lpcacheline) ;
bool LIBBK_VCBFllGetNextCacheLine(LPLIBBK_CACHELINEINFO lpcacheline) ;


bool LIBBK_VCBFllGetFirstDerivedLine(LPLIBBK_DERIVEDLINEINFO lpderivinfo) ;
bool LIBBK_VCBFllGetNextDerivedLine(LPLIBBK_DERIVEDLINEINFO lpderivinfo) ;


bool LIBBK_VCBFllGetFirstNetProtLine(LPLIBBK_NETPROTLINEINFO lpnetprotline) ;
bool LIBBK_VCBFllGetNextNetProtLine(LPLIBBK_NETPROTLINEINFO lpnetprotline) ;

bool LIBBK_VCBFllGetFirstBridgeProtLine(LPLIBBK_BRIDGEPROTLINEINFO lpbridgeprotline) ;
bool LIBBK_VCBFllGetNextBridgeProtLine (LPLIBBK_BRIDGEPROTLINEINFO lpbridgeprotline) ;



bool LIBBK_VCBFllCanCountBeEdited(LPLIBBK_COMPONENTINFO lpcomponent)    ;
bool LIBBK_VCBFllCanNameBeEdited(LPLIBBK_COMPONENTINFO lpcomponent)     ;
bool LIBBK_VCBFllCanCompBeDuplicated(LPLIBBK_COMPONENTINFO lpcomponent) ;
bool LIBBK_VCBFllCanCompBeDeleted(LPLIBBK_COMPONENTINFO lpcomponent)    ;
bool LIBBK_VCBFllValidCount(LPLIBBK_COMPONENTINFO lpcomponent, char * in_buf)  ;
bool LIBBK_VCBFllValidRename(LPLIBBK_COMPONENTINFO lpcomponent, char * in_buf) ;
bool LIBBK_VCBFllValidDuplicate(LPLIBBK_COMPONENTINFO lpcomponentsrc, LPLIBBK_COMPONENTINFO lpcomponentdest,char * in_buf);
bool LIBBK_VCBFllValidDelete(LPLIBBK_COMPONENTINFO lpcomponent);
/** ====> IMPORTANT: special management if trans.log configured  <==== **/


bool LIBBK_VCBFRequestExit();

bool LIBBK_VCBFllInitGenericPane( LPLIBBK_COMPONENTINFO lpcomp);    // to be called when activated line has first pane of type generic
bool LIBBK_VCBFllGenPaneOnEdit( LPLIBBK_GENERICLINEINFO lpgenlineinfo, char * lpnewvalue);
bool LIBBK_VCBFllAuditLogOnEdit( LPLIBBK_AUDITLOGINFO lpauditloglineinfo, char * lpnewvalue);
bool LIBBK_VCBFllResetGenValue(LPLIBBK_GENERICLINEINFO lpgenlineinfo) ;
bool LIBBK_VCBFllResetAuditValue(LPLIBBK_AUDITLOGINFO lpauditloglineinfo);
bool LIBBK_VCBFllOnMainComponentExit(LPLIBBK_COMPONENTINFO lpcomp); // to be called when deactivating line which has first pane of type generic

// for cache pane
bool LIBBK_VCBFllInitCaches();
bool LIBBK_VCBFllOnCacheEdit(LPLIBBK_CACHELINEINFO lpcachelineinfo, char * newvalue);

// functions for params pane for a given cache
bool LIBBK_VCBFllInitCacheParms(LPLIBBK_CACHELINEINFO lpcacheline);//generic cache
bool LIBBK_VCBFllCachePaneOnEdit( LPLIBBK_GENERICLINEINFO lpgenlineinfo, char * lpnewvalue);
bool LIBBK_VCBFllCacheResetGenValue(LPLIBBK_GENERICLINEINFO lpgenlineinfo) ;

bool LIBBK_VCBFllInitDependent(bool bCacheDep);

/******=> reload after change ****/
bool LIBBK_VCBBllOnEditDependent(LPLIBBK_DERIVEDLINEINFO lpderivinfo, char * newval);

bool LIBBK_VCBBllOnDependentProtectChange(LPLIBBK_DERIVEDLINEINFO lpderivinfo, char * newval);

/******=> reload after change ****/
bool LIBBK_VCBFllDepOnRecalc(LPLIBBK_DERIVEDLINEINFO lpderivinfo);

char* LIBBK_VCBFllLoadHist (void);
bool LIBBK_VCBFllFreeString( char* szFreeBufName);

char *LIBBK_VCBFll_dblist_init();
bool LIBBK_VCBFllOnDBListExit(char * db_list_buf,bool changed);

bool LIBBK_VCBFll_netprots_init(protocol_type nType/* = NET_PROTOCOL*/);
bool LIBBK_VCBFll_netprots_OnEditStatus(LPLIBBK_NETPROTLINEINFO lpnetprotline,char * in_buf, protocol_type nType);
bool LIBBK_VCBFll_netprots_OnEditListen(LPLIBBK_NETPROTLINEINFO lpnetprotline,char * in_buf, protocol_type nType);

bool LIBBK_VCBFll_bridgeprots_init();
bool LIBBK_VCBFll_bridgeprots_OnEditStatus(LPLIBBK_BRIDGEPROTLINEINFO lpbridgeprotline,char * in_buf);
bool LIBBK_VCBFll_bridgeprots_OnEditListen(LPLIBBK_BRIDGEPROTLINEINFO lpbridgeprotline,char * in_buf);
bool LIBBK_VCBFll_bridgeprots_OnEditVnode(LPLIBBK_BRIDGEPROTLINEINFO lpbridgeprotline,char * in_buf) ;


bool LIBBK_VCBFll_tx_init(LPLIBBK_COMPONENTINFO lpcompinfo,LPLIBBK_TRANSACTINFO lptransactinfo);
bool LIBBK_VCBFll_tx_OnTimer(LPLIBBK_TRANSACTINFO lptransactinfo);
bool LIBBK_VCBFll_tx_OnCreate(bool bPrimary,LPLIBBK_TRANSACTINFO lptransactinfo,char *lpsizeifwasempty);
bool LIBBK_VCBFll_tx_OnReformat(bool bPrimary,LPLIBBK_TRANSACTINFO lptransactinfo);
bool LIBBK_VCBFll_tx_OnDisable(bool bPrimary,LPLIBBK_TRANSACTINFO lptransactinfo);
bool LIBBK_VCBFll_tx_OnErase(bool bPrimary,LPLIBBK_TRANSACTINFO lptransactinfo);
bool LIBBK_VCBFll_tx_OnDestroy(bool bPrimary,LPLIBBK_TRANSACTINFO lptransactinfo);
bool LIBBK_VCBFll_tx_OnExit(LPLIBBK_TRANSACTINFO lptransactinfo,LPLIBBK_COMPONENTINFO lpLog1, LPLIBBK_COMPONENTINFO lpLog2);
bool LIBBK_VCBFll_tx_OnActivatePrimaryField(LPLIBBK_TRANSACTINFO lptransactinfo);
bool LIBBK_VCBFll_tx_OnActivateSecondaryField(LPLIBBK_TRANSACTINFO lptransactinfo);

bool LIBBK_VCBFll_settings_Init(bool *pbSyscheck);
bool LIBBK_VCBFll_On_settings_CheckChanged(bool bChecked,bool *pbCheckedResult);
bool LIBBK_VCBFll_settings_Terminate();
bool LIBBK_VCBFll_Init_net_Form();
bool LIBBK_VCBFll_Init_name_Form();
bool LIBBK_VCBFll_Form_GetParm(int ictrl,char * buf);
bool LIBBK_VCBFll_Form_OnEditParm(int ictrl,char * in_buf, char * bufresult);

// for O.I. Desktop server.
//bool VCBFllInitDskServComponent( LPCOMPONENTINFO lpcomp);
//bool VCBFllTerminateDskServComponent( LPCOMPONENTINFO lpcomp); // please refresh line after calling this function
//bool VCBFllDskServOnEdit( LPGENERICLINEINFO lpgenlineinfo, char * lpnewvalue); // ok for both tabs
//bool VCBFllDskServResetValue(LPGENERICLINEINFO lpgenlineinfo);                 // ok for both tabs
// (to get the list of parameters for Desktop server:
//  for tab 1 ("parameters") use the VCBFllGetFirst/next/GenericLine  stuff
//  for tab 2 ("advanced")   use the VCBFllGetFirst/next/AdvancedLine stuff)

bool LIBBK_VCBFll_OnStartupCountChange(char *szOldCount);



bool LIBBK_IsValidDatabaseName (char* dbName);
void LIBBK_VCBFll_Security_machanism_get(/*CPtrList& listMech*/item_lineinfo** headL);
bool LIBBK_VCBFll_Security_machanism_edit(char* lpszMech, char* lpszValue);
void LIBBK_INGRESII_QueryInstallationID(char *);
bool LIBBK_BRIDGE_AddVNode(char* addvnode, char* addport, char* addprotocol);
bool LIBBK_BRIDGE_DelVNode(char* addvnode, char* addport, char* addprotocol);
bool LIBBK_BRIDGE_ChangeStatusVNode(char* vnode, char* protocol, bool bOnOff);
//
// BRIDGE_CheckValide returns
// 0 : success
// 1 : vnode error
// 2 : protocol error
// 3 : listen address error:
int  LIBBK_BRIDGE_CheckValide(char* strVNode, char* strProtocol, char* strListenAddress);

void LIBBK_BRIDGE_protocol_list(char* net_id, item_string** item_St/*CStringList& listProtocols*/);

void LIBBK_BRIDGE_QueryVNode(/*CPtrList& listObj*/ item_bridge** h1);

#ifdef __cplusplus
}
#endif

#endif //_VCBF_LL_INCLUDED
