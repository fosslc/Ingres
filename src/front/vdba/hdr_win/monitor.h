/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Project : Visual DBA
**
**  Source : monitor.h
**
**  misc definitions fro VDBA Monitor Windows functionalities
**
**  Author : Francois Noirot-Nerin
**
**  History after 01-01-2000:
**
**	24-Jan-2000 (noifr01)
**   (SIR 100118) added the session description in the SessionDataMax
**   structure (used for storing the detail information on a session)
** 26-Jan-2000 (noifr01)
**  (bug 100155) added the server_pid in a structure where it was 
**  missing (it needs to be used in conjunction with the session_id
**  for fixing the bug) and a boolean for debug purpose
** 03-Feb-2000 (noifr01)
**  (SIR 100331) added RMCMD (monitor) server type
** 06-Dec-2000 (schph01)
**   SIR 102822 Add the variables members in LogSummaryDataMin structure.
** 26-Mar-2001 (noifr01)
**   (sir 104270) removal of code for managing Ingres/Desktop
** 27-Jul-2001 (noifr01)
**  (bug 102895) added a "capabilities" parameter in the prototype of
**  the FindClassServerType() function
** 01-Aug-2001 (noifr01)
**  (sir 105275) instantiate the JDBC server type, and changed the order
**  in which the servers will appear
** 23-Nov-2001 (noifr01)
**   added #if[n]def's for the libmon project
** 17-Sep-2003 (schph01)
**  (sir 110921) Increase the session_query buffer in SESSIONDATAMIN structure
**  with the same size plus one that the column session_query define in
**  ima_server_sessions table.
** 06-Feb-2004 (schph01)
**  (sir 111752) Added prototype of INGRESII_IsDBEventsEnabled() function
** 15-Mar-2004 (schph01)
**  (SIR #110013) Handle the DAS Server
** 24-Mar-2004 (shaha03)
**	(sir 112040) Following functions are added to make the LibMon Library thread safe
**					LIBMON_GetFirstMonInfo()
**					LIBMON_EmbdGetFirstMonInfo()
**					LIBMON_GetNextMonInfo()
**					LIBMON_ReleaseMonInfoHdl()
**  16-Apr-2004 (noifr01)
**   (sir 112040) #ifdef'ed modified prototypes of push4mong() and pop4mong()
**   so that winmain project still builds (using old version of these functions)
**  30-Apr-2004 (schph01)
**   (BUG 112243) Add prototype for functions IsServerTypeShutdown() and
**   IsInternalSession()
** 24-Sep-2004 (uk$so01)
**    BUG #113124, Convert the 64-bits Session ID to 64 bits Hexa value
*********************************************************************/

#ifndef _MONITOR_H
#define _MONITOR_H

#ifndef LIBMON_HEADER
#include "dba.h"
void UpdateMonDisplay(int hnodestruct, int iobjecttypeParent,
                      void *pstructParent, int   iobjecttypeReq);
void UpdateMonDisplay_MT(int hnodestruct, int iobjecttypeParent,
                      void *pstructParent, int   iobjecttypeReq);
void UpdateDBEDisplay(int hnodestruct, char * dbname);
void UpdateDBEDisplay_MT(int hnodestruct, char * dbname);
void UpdateNodeDisplay(void);
void UpdateNodeDisplay_MT(void);
extern BOOL InstallPasswordExists();
extern int MonOpenWindowsCount(int hNode);
#endif

// these values reflect the values from IMA
#define RESTYPE_ALL             0
#define RESTYPE_DATABASE        1
#define RESTYPE_TABLE           2
#define RESTYPE_PAGE            3
#define RESTYPE_EXTEND_FILE     4
#define RESTYPE_BM_PAGE         5
#define RESTYPE_CREATE_TABLE    6
#define RESTYPE_OWNER_ID        7
#define RESTYPE_CONFIG          8
#define RESTYPE_DB_TEMP_ID      9
#define RESTYPE_SV_DATABASE    10
#define RESTYPE_SV_TABLE       11
#define RESTYPE_SS_EVENT       12
#define RESTYPE_TBL_CONTROL    13
#define RESTYPE_JOURNAL        14
#define RESTYPE_OPEN_DB        15
#define RESTYPE_CKP_DB         16
#define RESTYPE_CKP_CLUSTER    17
#define RESTYPE_BM_LOCK        18
#define RESTYPE_BM_DATABASE    19
#define RESTYPE_BM_TABLE       20
#define RESTYPE_CONTROL        21
#define RESTYPE_EVCONNECT      22
#define RESTYPE_AUDIT          23
#define RESTYPE_ROW            24

// New types for ICE as of Sep. 10, 98
#define RESTYPE_ICE_CONNECTED_USER      25
#define RESTYPE_ICE_TRANSACTION         26
#define RESTYPE_ICE_CURSOR              27
#define RESTYPE_ICE_ACTIVE_USER         28
#define RESTYPE_ICE_FILEINFO            29
#define RESTYPE_ICE_FILEINFO_ALL        30
#define RESTYPE_ICE_FILEINFO_USER       31
#define RESTYPE_ICE_FILEINFO_FILENAME   32
#define RESTYPE_ICE_FILEINFO_LOCATION   33
#define RESTYPE_ICE_HTTP_CONNECTION     34
#define RESTYPE_ICE_DB_CONNECTION       35

#define SHUTDOWN_HARD           1
#define SHUTDOWN_SOFT           2

/*
** Log Process Status - comes from lgdef.h
*/
#define LPB_ACTIVE          0x000001
#define LPB_MASTER          0x000002
#define LPB_ARCHIVER        0x000004
#define LPB_FCT             0x000008
#define LPB_RUNAWAY         0x000010
#define LPB_SLAVE           0x000020
#define LPB_CKPDB           0x000040
#define LPB_VOID            0x000080
#define LPB_SHARED_BUFMGR   0x000100
#define LPB_IDLE            0x000200
#define LPB_DEAD            0x000400
#define LPB_DYING           0x000800
#define LPB_FOREIGN_RUNDOWN 0x001000
#define LPB_CPAGENT         0x002000


// type of lock
typedef struct type_lock {
  int iResource;
  char *szStringLock;
} STYPELOCK, FAR *LPSTYPELOCK;

extern STYPELOCK Type_Lock [];

// filter structure
typedef struct filter {
  BOOL bNullResources;
  BOOL bInternalSessions;
  BOOL bSystemLockLists;
  BOOL bInactiveTransactions;
  int ResourceType;
  BOOL bWithSystem;
  char lpBaseOwner[MAXOBJECTNAME];  // filter on database owner - "" means no filter
  char lpOtherOwner[MAXOBJECTNAME]; // filter on other objects owner - "" means no filter

  // Special for dom, if hint is FILTER_DOM_BKREFRESH
  int  UpdateType;  // must contain a OT_Xyz type, which will be tested in the dompage in order to know whether we need to call GetFirst/Next and update the display

  // Special for dom, if hint is FILTER_DOM_BKREFRESH_DETAIL:
  // Reflect input parameters to RefreshPropWindows
  BOOL bOnLoad;
  time_t refreshtime;

  } SFILTER, FAR *LPSFILTER;



// guaranteed maximum length of object name
#define MAXMONINFONAME  100

// Functions


/*Available only if BLD_MULTI is not defined */
#ifndef BLD_MULTI
int GetFirstMonInfo(int       hnodestruct,
                    int       iobjecttypeParent,
                    void     *pstructParent,
                    int       iobjecttypeReq,
                    void     *pstructReq,
                    LPSFILTER pFilter);
int GetNextMonInfo(void *pstructReq);
#endif

/*Following Function are added for thread safe Libmon */
int LIBMON_GetFirstMonInfo(int       hnodestruct,
                    int       iobjecttypeParent,
                    void     *pstructParent,
                    int       iobjecttypeReq,
                    void     *pstructReq,
                    LPSFILTER pFilter,
					int		*thdl);
int LIBMON_EmbdGetFirstMonInfo(int       hnodestruct,
                    int       iobjecttypeParent,
                    void     *pstructParent,
                    int       iobjecttypeReq,
                    void     *pstructReq,
                    LPSFILTER pFilter,
					int		thdl);
int LIBMON_GetNextMonInfo(int hdl,void *pstructReq);
int LIBMON_ReleaseMonInfoHdl(int thdl);

int	CompareMonInfo (int iobjecttype, void *pstruct1, void *pstruct2);
int LIBMON_CompareMonInfo (int iobjecttype, void *pstruct1, void *pstruct2, int thdl);
int CompareMonInfo1(int iobjecttype, void *pstruct1, void *pstruct2, BOOL bUpdSameObj);
int LIBMON_CompareMonInfo1(int iobjecttype, void *pstruct1, void *pstruct2, BOOL bUpdSameObj, int thdl);
char *GetMonInfoName(int hdl, int iobjecttype, void *pstruct, char *buf,int bufsize);
char *LIBMON_GetMonInfoName(int hdl, int iobjecttype, void *pstruct, char *buf,int bufsize, int thdl);
int  GetMonInfoStructSize(int iobjecttype);

int MonCompleteStruct(int hnode, void *pstruct, int* poldtype, int *pnewtype);
int MonGetDetailInfoLL(int hnode, void *pbigstruct, int oldtype, int newtype);
int MonGetDetailInfo(int hnode, void *pbigstruct, int oldtype, int newtype);
int MonGetRelatedInfo(int hnodestruct, int iobjecttypeSrc, void *pstructSrc,
                      int iobjecttypeDest, void *pstructDest);

void UpdateMonInfo(int hnodestruct,int iobjecttypeParent, void *pstructParent,
                   int iobjecttypeReq);

BOOL IsMonTrueParent(int iobjecttypeparent, int iobjecttypeobj);
int *MonGetDependentTypeList(int iobjecttype);
BOOL ContinueExpand(int iobjecttype);
BOOL MonFitWithFilter(int iobjtype,void * pstructReq,LPSFILTER pfilter);
void FillResStructFromDB(void *pstructReq,LPUCHAR buf);
BOOL IsInternalSession(void * pstructReq);
BOOL IsShutdownServerType(void * pstructReq);

char * FindResourceString(int iTypeRes);
char *FindServerTypeString(int svrtype);
int   FindClassServerType(char *ServerClass, char * capabilities);

int decodeStatusVal(int val,char *buf);

extern LPTSTR FormatHexa64(LPCTSTR lpszString,  LPTSTR lpszBuffer);
extern char * FormatHexaS(char * lpValStr,char * lpBufRes);
extern char * FormatHexaL(long lpVal,char * lpBufRes);

extern BOOL isMonSpecialState(int hnode);
extern BOOL SetMonNormalState(int hnode);
extern int LocationInLocGroup(int hnode,char * locationname, char * locationgroupname);


// dummy structure - TEMPORAIRE
struct GeneralMonInfo {
  char name[MAXMONINFONAME];
  int  bidon;
};
  
#define SVR_TYPE_GCN      1
#define SVR_TYPE_NET      2
#define SVR_TYPE_DBMS     3
#define SVR_TYPE_RECOVERY 4
#define SVR_TYPE_STAR     5 
#define SVR_TYPE_ICE      6
#define SVR_TYPE_JDBC     7
#define SVR_TYPE_DASVR    8
#define SVR_TYPE_RMCMD    9
#define SVR_TYPE_OTHER    10

#define LOCK_TYPE_OTHER  0
#define LOCK_TYPE_DB     1
#define LOCK_TYPE_TABLE  2
#define LOCK_TYPE_PAGE   3

typedef struct GeneralMonInfo
 MONSESSIONMIN, FAR *LPMONSESSIONMIN;

/*---------------------------------------------------------*/

// Structures
typedef struct NodeServerClassDataMin { 
   UCHAR NodeName   [MAXOBJECTNAME];
   BOOL bIsLocal;
   BOOL isSimplifiedModeOK;

   UCHAR ServerClass[MAXOBJECTNAME];
   
} NODESVRCLASSDATAMIN, FAR * LPNODESVRCLASSDATAMIN;

typedef struct NodeUserDataMin { 
   UCHAR NodeName   [MAXOBJECTNAME];
   BOOL  bIsLocal;
   BOOL  isSimplifiedModeOK;

   UCHAR ServerClass[MAXOBJECTNAME];

   UCHAR User[MAXOBJECTNAME];
   
} NODEUSERDATAMIN, FAR * LPNODEUSERDATAMIN;

typedef struct NodeLoginDataMin { 
   UCHAR NodeName   [MAXOBJECTNAME];

   BOOL bPrivate;

   UCHAR Login      [MAXOBJECTNAME];
   UCHAR Passwd     [MAXOBJECTNAME];

} NODELOGINDATAMIN, FAR * LPNODELOGINDATAMIN;

typedef struct NodeConnectDataMin { 
   UCHAR NodeName   [MAXOBJECTNAME];

   BOOL bPrivate;

   UCHAR Address    [MAXOBJECTNAME];
   UCHAR Protocol   [MAXOBJECTNAME];
   UCHAR Listen     [MAXOBJECTNAME];

   int   ino;    // for sort (this branch will not be sorted on the
                 // nodename, but on this number, in order to reflect
                 // the order returned by low-level if it impacts on
                 // the priority among Connection data

} NODECONNECTDATAMIN, FAR * LPNODECONNECTDATAMIN;

typedef struct NodeAttributeDataMin { 
   UCHAR NodeName   [MAXOBJECTNAME];

   BOOL bPrivate;

   UCHAR AttributeName [MAXOBJECTNAME];
   UCHAR AttributeValue[MAXOBJECTNAME];

} NODEATTRIBUTEDATAMIN, FAR * LPNODEATTRIBUTEDATAMIN;

typedef struct NodeDataMin { 
   UCHAR NodeName   [MAXOBJECTNAME];
   BOOL bIsLocal;
   BOOL isSimplifiedModeOK;

   BOOL bWrongLocalName; // true if local node defined differently that with user * (only)
   BOOL bInstallPassword;
   BOOL bTooMuchInfoInInstallPassword;

   int inbConnectData;
   NODECONNECTDATAMIN ConnectDta;

   int inbLoginData;
   NODELOGINDATAMIN   LoginDta;

} NODEDATAMIN, FAR * LPNODEDATAMIN;

typedef struct WindowDataMin { 
   HWND hwnd;
   UCHAR Title[200];
   UCHAR ServerClass[200];
   UCHAR ConnectUserName[MAXOBJECTNAME];
} WINDOWDATAMIN, FAR * LPWINDOWDATAMIN;



typedef struct ServerDataMin { 
  char  name_server[65];
  char  listen_address[65];
  char  server_class[33];
  char  server_dblist[33];

  char listen_state[33];
//   long  maxSessions;
//   long  numSessions;
  BOOL bHasServerPid;
  long  serverPid;


  BOOL bMultipleWithSameName;

  int servertype;   //SVR_TYPE_xx

} SERVERDATAMIN, FAR * LPSERVERDATAMIN;

typedef struct ServerDataMax { 
  
  SERVERDATAMIN serverdatamin; 
  long  current_connections;       // number session list 
  long  current_max_connections;   //
  long  active_current_sessions;   // session computable
  long  active_max_sessions;       // max_connections in the table 
                                   // ima_dbms_server_parameter
} SERVERDATAMAX, FAR * LPSERVERDATAMAX;



typedef struct SessionDataMin{
  char  session_name[33];
  char  session_id[33];
  char  db_lock[10];
  char  db_name[33];
  char  server_facility[11];
  char  session_query[1001];
  char  session_terminal[13];
  char  client_pid[21];
  long  server_pid;
  char  effective_user[33];
  char  real_user[33];
  long  session_state_num;
  char  session_state[33];
  char  session_wait_reason[33];
  long  application_code;

  long  locklist_wait_id;  // if not 0, session is blocked and the value comes from the structure of the blocked locklist

  SERVERDATAMIN sesssvrdata;

  BOOL bMultipleWithSameName;

} SESSIONDATAMIN, FAR * LPSESSIONDATAMIN;

typedef struct SessionDataMax{

  SESSIONDATAMIN sessiondatamin;

  char  session_mask[33]; 
  char  db_owner[33];
  char  session_group[9];
  char  session_role[9];
  char  session_activity[81];
  char  activity_detail[81];
  char  session_full_query[1001];
  char  client_host[20];
  char  client_pid [20];
  char  client_terminal[20];
  char  client_user[32];
  char  client_connect_string[64];
  char  session_description[256+1];

} SESSIONDATAMAX, FAR * LPSESSIONDATAMAX;

typedef struct LockListDataMin {
  char  vnode[65];
  long  locklist_id;
  long  locklist_logical_count;
  char  locklist_status[51];
  long  locklist_lock_count;
  long  locklist_max_locks;
  long  locklist_tx_high;     // Transaction id high.0 if no transaction  
  long  locklist_tx_low;      // Transaction id low. 
  long  locklist_server_pid;
  char  locklist_session_id[33];
  long  locklist_wait_id;
  long  locklist_related_llb;

  BOOL bIs4AllLockLists;

  /*** available ONLY after a MonGetDetailInfo() call;  ***/
  char  locklist_transaction_id[33]; //available ONLY after MonGetDetailInfo();

} LOCKLISTDATAMIN, FAR * LPLOCKLISTDATAMIN;

typedef struct LockListDataMax {

  LOCKLISTDATAMIN locklistdatamin;

  long  locklist_related_llb_id_id;
  long  locklist_related_count;
  char  WaitingResource [40];

} LOCKLISTDATAMAX, FAR * LPLOCKLISTDATAMAX;

typedef struct LockDataMin {
//  char  vnode[65];
  long  lock_id;                     // from LL
  char  lock_request_mode[4];        // from LL
  char  lock_grant_mode[4];
  char  lock_state[21];              // from LL
  char  lock_attributes[31];
  long  resource_id;                 // from LL
  long  locklist_id;                 // from LL

  char  resource_key[61];
  long  resource_type;                 // from LL
  long  resource_database_id;          // from LL
  char  res_database_name[33];         
  long  resource_table_id;             // from LL
  char  res_table_name[80];             // Filled by MonGetDetailInfo()
           // size because of possible [tblname]idxname format display
  long  resource_index_id;             // from LL
  long  resource_page_number;          // from LL

  int hdl ;         // used only for tables

  int   locktype;      // LOCK_TYPE_XX

} LOCKDATAMIN, FAR * LPLOCKDATAMIN;

typedef struct LockDataMax {

  LOCKDATAMIN lockdatamin; 

} LOCKDATAMAX, FAR * LPLOCKDATAMAX;


typedef struct ResourceDataMin {
  
  char  resource_key[61];
  long  resource_id;
  char  resource_grant_mode[4];
  char  resource_convert_mode[4];
  long  resource_type;
  long  resource_database_id;
  long  resource_table_id;
  long  resource_index_id;
  long  resource_page_number;
  long  resource_row_id;

  int	dbtype;  // DBTYPE_REGULAR , DBTYPE_DISTRIBUTED , DBTYPE_COORDINATOR or DBTYPE_ERROR (for the icon)
  
  int hdl ;         // used only for tables

  char  res_database_name[33];       
  /*** available only after MonGetDetailInfo() call ***/
  char  res_table_name[2 * MAXOBJECTNAME +2]; // Filled by MonGetDetailInfo()
           // size because of possible [tblname]idxname format display
  BOOL m_bRefresh; // Internal used by the dialog (special optimization to call OnUpdate data only one).
} RESOURCEDATAMIN, FAR * LPRESOURCEDATAMIN;

typedef struct ResourceDataMax {

  RESOURCEDATAMIN resourcedatamin;
  int number_locks;

} RESOURCEDATAMAX, FAR * LPRESOURCEDATAMAX;


typedef struct LogProcessDataMin {

  UCHAR LogProcessName[MAXOBJECTNAME]; // keep this name

  long  log_process_id;
  long  process_pid;
  long  process_status_num;
  char  process_status [201]; 
  long  process_count;
  long  process_writes;
  long  process_log_forces;
  long  process_waits;
  long  process_tx_begins;
  long  process_tx_ends;

   // should include  RESOURCE, even if not displayed

} LOGPROCESSDATAMIN, FAR * LPLOGPROCESSDATAMIN;

typedef struct LogProcessDataMax {

  LOGPROCESSDATAMIN logprocessdatmin;

} LOGPROCESSDATAMAX, FAR * LPLOGPROCESSDATAMAX;


typedef struct LogDBDataMin {

  char  db_name[33];
  char  db_status[101];
  long  db_tx_count;
  long  db_tx_begins;
  long  db_tx_ends;
  long  db_reads;
  long  db_writes;

  int	dbtype;  // DBTYPE_REGULAR , DBTYPE_DISTRIBUTED , DBTYPE_COORDINATOR or DBTYPE_ERROR (for the icon)


} LOGDBDATAMIN, FAR * LPLOGDBDATAMIN;


typedef struct LogTransactDataMin {

  char tx_transaction_id[33];
  long tx_id_id;
  long tx_id_instance;
  char tx_db_name [33];
  char tx_user_name [33];
  char tx_status [201];
  char tx_session_id [33];
  long tx_transaction_high;
  long tx_transaction_low;
  long tx_state_split;
  long tx_state_write;
  long tx_server_pid;

} LOGTRANSACTDATAMIN, FAR * LPLOGTRANSACTDATAMIN;

typedef struct LogTransactDataMax {

  LOGTRANSACTDATAMIN logtxdatamin;
  char tx_first_log_address [33];
  char tx_last_log_address [33];
  char tx_cp_log_address [33];
  long tx_state_force;
  long tx_state_wait;
  long tx_internal_pid;      // tx_pr_id_id 
  long tx_external_pid;      // tx_server_pid

} LOGTRANSACTDATAMAX, FAR * LPLOGTRANSACTDATAMAX;

typedef struct LockSummaryDataMin {
  
  // Lock list information
  long sumlck_lklist_created_list;        /* lock lists created  */
  long sumlck_lklist_released_all;        /* lock lists released */
  long sumlck_lklist_inuse;               /* lock lists in use   */
  long sumlck_lklist_remaining;           /* lock lists remaining */
  long sumlck_total_lock_lists_available; /* total lock lists available */

  // Hash Table sizes
  long sumlck_rsh_size;          /* size of resource has table */
  long sumlck_lkh_size;          /* size of lock hash table */

  // Lock information
  long sumlck_lkd_request_new;   /* locks requested */
  long sumlck_lkd_request_cvt;   /* locks re-requested */
  long sumlck_lkd_convert;       /* locks converted */
  long sumlck_lkd_release;       /* locks released */
  long sumlck_lkd_cancel;        /* locks cancelled */
  long sumlck_lkd_rlse_partial;  /* locks escalated */
  long sumlck_lkd_inuse;         /* locks in use */
  long sumlck_locks_remaining;   /* locks remaining */
  long sumlck_locks_available;   /* total locks available */

  long sumlck_lkd_dlock_srch;    /* Deadlock Search */
  long sumlck_lkd_cvt_deadlock;  /* Convert Deadlock */
  long sumlck_lkd_deadlock;      /* deadlock */

  long sumlck_lkd_convert_wait;  /* convert wait */
  long sumlck_lkd_wait;          /* lock wait */

  long sumlck_lkd_max_lkb;       /* locks per transaction */

  long sumlck_lkd_rsb_inuse;     /* Resource Blocks in use */

} LOCKSUMMARYDATAMIN, FAR * LPLOCKSUMMARYDATAMIN;

typedef struct LogSummaryDataMin {

    long lgd_add;            // Database adds 
    long lgd_remove;
    long lgd_begin;          // transact begin
    long lgd_end;
    long lgd_write;          // Log Writes
    long lgd_writeio;
    long lgd_readio;
    long lgd_wait;           // Log Waits
    long lgd_split;
    long lgd_free_wait;
    long lgd_stall_wait;
    long lgd_force;          // log Forces
    long lgd_group_force;    // log group commit
    long lgd_group_count;    // log group count
    long lgd_pgyback_check;  // timer
    long lgd_pgyback_write;
    long lgd_kbytes;         // Kbytes Written
    long lgd_inconsist_db;   // Inconsistent DB
    long lgd_Present_end;     // store the current lgd_end
    long lgd_Present_writeio; // store the current lgd_writeio
    long lgd_Commit_Average;
    long lgd_Commit_Current;
    long lgd_Commit_Max;
    long lgd_Commit_Min;
    long lgd_Write_Average;
    long lgd_Write_Current;
    long lgd_Write_Max;
    long lgd_Write_Min;

} LOGSUMMARYDATAMIN, FAR * LPLOGSUMMARYDATAMIN;

typedef struct LogHeaderDataMin {

  char  status[101];
  long  block_count;
  long  block_size;
  long  buf_count;
  long  logfull_interval;
  long  abort_interval;
  long  cp_interval;
  char  begin_addr[31];
  long  begin_block;
  char  end_addr[40];
  long  end_block;
  char  cp_addr[40];
  long  cp_block;
  long  next_cp_block;
  long  reserved_blocks;
  long  inuse_blocks;
  long  avail_blocks;

} LOGHEADERDATAMIN, FAR * LPLOGHEADERDATAMIN;

typedef struct LocationDataMin {

  UCHAR LocationName[4 * MAXOBJECTNAME]; // wider because of special management
                                         // for OT_LOCATIONNODOUBLE
  UCHAR LocationArea[256];

  UCHAR LocationUsages[5];
} LOCATIONDATAMIN, FAR * LPLOCATIONDATAMIN;

#define DBEHDL_NOTASSIGNED   (-1)
#define DBEHDL_ERROR         (-2)
#define DBEHDL_ERROR_ALLWAYS (-3)

#define REPSVR_UNKNOWNSTATUS 0     
#define REPSVR_STARTED       1
#define REPSVR_STOPPED       2
#define REPSVR_ERROR         3

typedef struct ReplicServerDataMin {
    UCHAR ParentDataBaseName[MAXOBJECTNAME];

    UCHAR RunNode[MAXOBJECTNAME];   
    UCHAR LocalDBNode[MAXOBJECTNAME];
    UCHAR LocalDBName[MAXOBJECTNAME];
	int	  localdb;
    int   serverno;
    int   startstatus;    // REPSVR_???
	UCHAR FullStatus[200];
	int   DBEhdl;                   // handle in session cache for getting raised events (-1 => not connected)
    BOOL m_bRefresh; // Internal used by the dialog (special optimization to call OnUpdate data only one).
#ifdef LIBMON_HEADER
	BOOL  bMultipleWithDiffLocalDB;
#endif
} REPLICSERVERDATAMIN, FAR * LPREPLICSERVERDATAMIN;

typedef struct ReplicCddsAssignDataMin {
    int Database_No;
    UCHAR szDBName[MAXOBJECTNAME];
    UCHAR szVnodeName[MAXOBJECTNAME];
    int Cdds_No;
    UCHAR szCddsName[MAXOBJECTNAME];
    UCHAR szTargetType[MAXOBJECTNAME];
} REPLICCDDSASSIGNDATAMIN, FAR * LPREPLICCDDSASSIGNDATAMIN;

//
// Extra information needed by the MonGetFirst ()
// This structure is used to update the page of the table control: ex Session Page.
typedef struct tagIPMUPDATEPARAMS
{
    int         nType;      // The 2nd  parameter of MonGetFirst
    void*       pStruct;    // The 3rd  parameter of MonGetFirst
    LPSFILTER   pSFilter;   // The last parameter of MonGetFirst
    int         nIpmHint;   // What is the filter that causes the change ?, 0 = Ignore.
} IPMUPDATEPARAMS, * LPIPMUPDATEPARAMS;



extern int MonitorGetServerInfo              (     LPSERVERDATAMAX pServerDataMax );
extern int MonitorGetAllServerInfo           (     LPSERVERDATAMAX pServerDataMax );
extern int MonitorGetDetailLockSummary       (LPLOCKSUMMARYDATAMIN pLockSummary   );
extern int MonitorGetDetailSessionInfo       (    LPSESSIONDATAMAX pSessionDataMax);
extern int MonitorGetDetailLockListInfo      (   LPLOCKLISTDATAMAX plocklistdta   );
extern int MonitorGetDetailLogSummary        ( LPLOGSUMMARYDATAMIN pLogSummary    );
extern int MonitorGetDetailLogHeader         (  LPLOGHEADERDATAMIN pLogHeader     );
extern int MonitorGetDetailTransactionInfo   (LPLOGTRANSACTDATAMAX pTransactionDataMax);
extern int MonitorFindCurrentSessionId  ( char * CurID );

extern char * GetLockTableNameStr(LPLOCKDATAMIN lplock, char * bufres, int bufsize);
extern char * GetLockPageNameStr (LPLOCKDATAMIN lplock, char * bufres);
extern char * GetResTableNameStr(LPRESOURCEDATAMIN lpres, char * bufres, int bufsize);
extern char * GetResPageNameStr (LPRESOURCEDATAMIN lpres, char * bufres);
extern char * GetTransactionString(int hdl, long tx_high, long tx_low, char * bufres, int bufsize);

extern int MonShutDownSvr  (int hnode, LPSERVERDATAMIN pServer, BOOL bSoft);
extern int MonRemoveSession(int hnode, LPSESSIONDATAMIN pSession)        ;
extern int DBA_ShutdownServer         (LPSERVERDATAMIN pServer   , int ShutdownType);
extern int DBA_RemoveSession          (LPSESSIONDATAMIN pSession , char *ServerName );
extern int MonOpenCloseSvr (int hnode, LPSERVERDATAMIN pServer, BOOL bOpen);
extern int DBA_OpenCloseServer        (LPSERVERDATAMIN pServer ,BOOL bOpen);


extern char * FormatStateString ( LPSESSIONDATAMIN pSess , char * lpBufRes);
extern char * FormatServerSession(long lValue, char * lpBufRes);
extern void FillMonResourceDBName(LPRESOURCEDATAMIN lpres);
extern void LIBMON_FillMonResourceDBName(LPRESOURCEDATAMIN lpres, int thdl) ;
extern void FillMonLockDBName    (LPLOCKDATAMIN     lplock);
extern void LIBMON_FillMonLockDBName    (LPLOCKDATAMIN     lplock, int thdl) ;

extern int	FilledSpecialRunNode( long hdl,  LPRESOURCEDATAMIN pResDtaMin,
								  LPREPLICSERVERDATAMIN pReplSvrDtaBeforeRunNode,
								  LPREPLICSERVERDATAMIN pReplSvrDtaAfterRunNode);

int MonGetLockSummaryInfo(int hnode,
                          LPLOCKSUMMARYDATAMIN presult,
                          LPLOCKSUMMARYDATAMIN pstart,
                          BOOL bSinceStartUp,
                          BOOL bNowAction);
int MonGetLogSummaryInfo(int hnode,
                          LPLOGSUMMARYDATAMIN presult,
                          LPLOGSUMMARYDATAMIN pstart,
                          BOOL bSinceStartUp,
                          BOOL bNowAction);

#if defined(LIBMON_HEADER) || defined(BLD_MULTI)
void push4mong(int thdl);
void pop4mong(int thdl);
#else
void push4mong();
void pop4mong();
#endif

extern int GetPathLoc( char * LocArea, char ** BaseDir);

extern BOOL INGRESII_IsDBEventsEnabled (LPNODESVRCLASSDATAMIN lpInfo);

// New structures for ICE as of Sep. 10, 98

typedef struct IceConnectedUserDataMin {
   UCHAR name	[32];
   UCHAR inguser[32];
   long req_count;
   long timeout;

	SERVERDATAMIN svrdata;

} ICE_CONNECTED_USERDATAMIN, FAR * LPICE_CONNECTED_USERDATAMIN;

typedef struct IceConnectedUserDataMax {
  ICE_CONNECTED_USERDATAMIN iceconnecteduserdatamin;
} ICE_CONNECTED_USERDATAMAX, FAR * LPICE_CONNECTED_USERDATAMAX;

typedef struct IceActiveUserDataMin {
	UCHAR name	[32];
    UCHAR host  [32];
	UCHAR query [64];
	long count;
} ICE_ACTIVE_USERDATAMIN, FAR * LPICE_ACTIVE_USERDATAMIN;

typedef struct IceActiveUserDataMax {
	ICE_ACTIVE_USERDATAMIN iceactiveuserdatamin;
} ICE_ACTIVE_USERDATAMAX, FAR * LPICE_ACTIVE_USERDATAMAX;

typedef struct IceTransactionDataMin {
	UCHAR trans_key [32];
    UCHAR name      [32];
	UCHAR connect   [32];
	UCHAR owner     [32];  // connected user

	SERVERDATAMIN svrdata;

} ICE_TRANSACTIONDATAMIN, FAR * LPICE_TRANSACTIONDATAMIN;

typedef struct IceTransactionDataMax {
	ICE_TRANSACTIONDATAMIN icetransactiondatamin;
} ICE_TRANSACTIONDATAMAX, FAR * LPICE_TRANSACTIONDATAMAX;

typedef struct IceCursorDataMin {
	UCHAR curs_key  [32];
	UCHAR name      [32];
	UCHAR query     [64];
	UCHAR owner     [32];  // transaction key??
} ICE_CURSORDATAMIN, FAR * LPICE_CURSORDATAMIN;

typedef struct IceCursorDataMax {
	ICE_CURSORDATAMIN icecursordatamin;
} ICE_CURSORDATAMAX, FAR * LPICE_CURSORDATAMAX;

typedef struct IceCacheDataMin {
    UCHAR cache_index [32];
	UCHAR name        [32];  // page name
	UCHAR loc_name    [32];  // location name
	UCHAR status      [32];
	int counter;
	int iexist;
	UCHAR owner		  [32];  // connected user
	int itimeout;
	int in_use;
	int req_count;

	SERVERDATAMIN svrdata;

} ICE_CACHEINFODATAMIN, FAR * LPICE_CACHEINFODATAMIN;

typedef struct IceCacheDataMax {
	ICE_CACHEINFODATAMIN icecacheinfodatamin;
} ICE_CACHEINFODATAMAX, FAR * LPICE_CACHEINFODATAMAX;

typedef struct IceDbConnectionDataMin {
    UCHAR sessionkey  [32];
	int   driver;
	UCHAR dbname	  [32];
	int   iused;
	int	itimeout;
} ICE_DB_CONNECTIONDATAMIN, FAR * LPICE_DB_CONNECTIONDATAMIN;

typedef struct IceDbConnectionDataMax {
	ICE_DB_CONNECTIONDATAMIN icedbconnectiondatamin;
} ICE_DB_CONNECTIONDATAMAX, FAR * LPICE_DB_CONNECTIONDATAMAX;

typedef union unionDatamin {
     NODESVRCLASSDATAMIN SvrClassDta;
     NODEUSERDATAMIN     UsrDta;
     NODELOGINDATAMIN    LoginDta;
     NODECONNECTDATAMIN  ConnectDta;
     NODEDATAMIN         NodeDta;
     WINDOWDATAMIN       WindowDta;
     SERVERDATAMIN       SvrDta;
     SESSIONDATAMIN      SessionDta;
     LOCKLISTDATAMIN     LockListDta;
     LOCKDATAMIN         LockDta;
     RESOURCEDATAMIN     ResourceDta;
     LOGPROCESSDATAMIN   LogProcessDta;
     LOGDBDATAMIN        LogDBDta;
     LOGTRANSACTDATAMIN  LogTransacDta;
     REPLICSERVERDATAMIN ReplicServerDta;
     ICE_CONNECTED_USERDATAMIN IceConnectedUserDta;
     ICE_ACTIVE_USERDATAMIN    IceActiveUserDta;
     ICE_TRANSACTIONDATAMIN    IceTransactionDta;
     ICE_CURSORDATAMIN         IceCursorDta;
     ICE_CACHEINFODATAMIN      IceCacheInfoDta;
     ICE_DB_CONNECTIONDATAMIN  IceDbConnectionDta;

} UNIONDATAMIN, FAR *LPUNIONDATAMIN;

#endif   // _MONITOR_H
