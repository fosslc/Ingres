/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Project : libmon library for the Ingres Monitor ActiveX control
**
**    Source : libmon.h
**
** History
**
**	20-Nov-2001 (noifr01)
**      created
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
** 19-Nov-2004 (noifr01)
**    BUG #113500 / ISSUE #13755297 (Vdba / monitor, references such as background 
**    refresh, or the number of sessions, are not taken into account in vdbamon nor 
**    in the VDBA Monitor functionality)
********************************************************************/


#ifndef LIBMON_HEADER
#define LIBMON_HEADER /* do not change this name : it will restrict some */
                      /* definitions in dba.h and other included headers */

#include "dba.h"
#include "domdata.h"
#include "dbaginfo.h"
#include "dbaset.h"
#include "monitor.h"
#include "dbatime.h"
#include "collisio.h"

/* additional RES_XX values */

#define RES_NOSHUTDOWNSERVER        1000  /* IDS_E_SHUTDOWN_SVR */
#define RES_NOREMOVESESSION         1001  /* IDS_E_REMOVE_SESS  */
#define RES_ERR_DEL_USED_SESSION    1002  /* cannot delete session used to delete the session ... */
#define RES_SVR_NO_PROC_OPENCLOSE   1003  /* IDS_E_PROCEDURE_IMA*/
#define RES_CONNECTIONFAILURE       1004
#define RES_TOOFAST_DBEVENT         1005
#define RES_TOOMANY_REPL_SERV       1006
#define RES_CANNOT_PING_SVR         1007
#define RES_CANNOT_REGISTER_SVR     1008

#define RES_RMCMD_CMD_NOT_ACCEPTED  1010  /* IDS_REMOTECMD_NOTACCEPTED */
#define RES_RMCMD_NO_ANSWER_FRM_SVR 1011

#define RES_SVRTYPE_NOSHUTDOWN      1012  /* IDS_ERR_ONLY_ING_OR_STAR_CANBESTPD */


#define MASK_RMCMD_ERR_CLEANUP         2048 /* not used together with other 2048 values */

#define MASK_NEED_REFRESH_MONTREE      2048 /* to be checked when timer calls DBEScanDBEvents() */
#define MASK_DBEEVENTTHERE             4096 /* to be checked when timer calls DBEScanDBEvents() */

/* the next 2 definitions must have a <(-1) value because of GetRmcmdOutput() return values */
#define RES_RMCMDCLI_ERR_ACCESS_RMCMDOBJ (-2) /* must be < (-1) IDS_ERR_ACCESS_RMCMD_OBJ     */
#define RES_RMCMDCLI_ERR_OUTPUT          (-3) /* must be < (-1) IDS_ERR_RMCMD_OUTPUT_COMMAND */

/* LIBMON_RetrySvrStatusRefresh() is to be called if when getting the list pf  */
/* replic servers, the return value has the MASK_RES_ERR_REPSVRSTATUS bit set: */
/* in this case, a messagebox asking whether to continue trying to refresh     */
/* should be done. And LIBMON_RetrySvrStatusRefresh() should be called with    */
/* the corresponding value (warning, this will apply to all servers in this    */
void LIBMON_RetrySvrStatusRefresh(BOOL bRetry); 


BOOL LIBMON_IsNodeActive(TCHAR * pnode); 
void LIBMON_SetLocalNodeString(TCHAR *pLocalNodeString);
void LIBMON_SetNoneString(TCHAR *pnonestring);
TCHAR * LIBMON_getLocalNodeString();
TCHAR * LIBMON_getNoneString();
TCHAR * LIBMON_getLocalHostName();

/*preferences*/
void LIBMON_SetSessionTimeOut(long ltimeout);
void LIBMON_SetRefreshFrequency(LPFREQUENCY pfrequency);
void LIBMON_SetMaxSessions(int imaxsess);
BOOL LIBMON_CanUpdateMaxSessions(void);


int LIBMON_ExecRmcmdInBackground(TCHAR * lpVirtNode, TCHAR *lpCmd, TCHAR * InputLines);
TCHAR * LIBMON_GetFirstTraceLine(void);  // returns NULL if no more data
TCHAR * LIBMON_GetNextTraceLine(void);   // returns NULL if no more data
TCHAR * LIBMON_GetNextSignificantTraceLine();
TCHAR * LIBMON_GetTraceOutputBuf();
void LIBMON_FreeTraceOutputBuf();

int LIBMON_Mysystem(TCHAR* lpCmd);

BOOL LIBMON_NeedBkRefresh (int hnodestruct);

int  LIBMON_OpenNodeStruct  (LPUCHAR lpVirtNode);
BOOL LIBMON_CloseNodeStruct (int hnodestruct);


/* use instead of CheckConnection, because this one has more than 2 possible return values */
int LIBMON_CheckConnection(LPUCHAR lpVirtNode);

/************************ REPLICATION *****************************************/
typedef struct error_parameters
{
	int iErrorID; //error return by the low level, at the moment only tblinteg.sc fill this field.
	int iParam1;
	int iParam2;
	TCHAR tcParam1[400];
	TCHAR tcParam2[400];
} ERRORPARAMETERS , FAR *LPERRORPARAMETERS;

// Tblinteg.sc
typedef struct _tblstringformat
{
	LPTSTR lpCheckUniqueKeys;         // IDS_E_CHECK_UNIQUE_KEYS_DBMS_ERROR "CHECK_UNIQUE_KEYS:  DBMS error -%d looking for table '%s.%s'"
	LPTSTR lpLookingUPColumns;        // IDS_E_LOOKING_UP_COLUMNS           "CHECK_UNIQUE_KEYS:  Error %d looking up columns for table '%s.%s'"
	LPTSTR lpIngresReplic;            // IDS_I_INGRES_REPLIC                "Ingres Replicator"
	LPTSTR lpTblIntegReport;          // IDS_E_TBL_INTEG_REPORT             "Table Integrity Report"
	LPTSTR lpForTbl;                  // IDS_I_FOR_TBL                      "For table '%s.%s' in '%s'"
	LPTSTR lpIandtbl;                 // IDS_I_AND_TBL                      "And table '%s.%s' in '%s'"
	LPTSTR lpIRegKeyColumns;          // IDS_I_REG_KEY_COLUMNS              "Registered key columns do not force the table to be"
	LPTSTR lpIUnique;                 // IDS_I_UNIQUE                       "unique in %s." ID Inexistant Actuelement
	LPTSTR lpRowOnlyIn;               // IDS_ROW_ONLY_IN                    "Row only in %s, not in %s"
	LPTSTR lpTblIntegRowDifferences;  // IDS_TBL_INTEG_ROW_DIFFERENCES      "Row is different in %s than in %s"
	LPTSTR lpTblIntegSourceDB;        // IDS_TBL_INTEG_SOURCE_DB            "Source DB:  %d, Transaction ID:  %d, Sequence no:  %d.\r\n."
	LPTSTR lpTblIntegDBTransac;       // IDS_TBL_INTEG_DB_TRANSAC           "database no:  %d, transaction id:  %d, sequence no:  %d"
	LPTSTR lpCheckIndexesOpenCursor;  // IDS_E_CHECK_INDEXES_OPEN_CURSOR     "CHECK_INDEXES:  Open cursor failed with error -%d."
	LPTSTR lpFetchIndex;              // IDS_E_FETCH_INDEX "CHECK_INDEXES:  Fetch of index names for table"
	                                                     //" '%s.%s' failed with error -%d."
	LPTSTR lpResTblIntegError;        // IDS_E_TBL_INTEG_ERROR "TABLE_INTEGRITY: Error %d retrieving database name for database %d."
	LPTSTR lpInvalidDB;               // IDS_E_INVALID_DB "invalid database name %s or node %s for database number %d"
	LPTSTR lpTblIntegTargetType;      // IDS_E_TBL_INTEG_TARGET_TYPE "TABLE_INTEGRITY: Error %d retrieving target_type for database %d and cdds %d."
	LPTSTR lpDbUnprotreadonly;        // IDS_E_DB_UNPROTREADONLY "Cannot run integrity report on URO target.\n"
	                                                           //"Database '%s' in CDDS %d is an Unprotected Read Only target. The Integrity"
	                                                           //"Report can only be run on Full Peer and Protected Read Only targets."
	LPTSTR lpMaxNumConnection;        // IDS_E_MAX_NUM_CONNECTION  "Maximum number of connections has been reached - Cannot display integrity report."
	LPTSTR lpConnectDb;               // IDS_E_CONNECT_DB "Error -%d connecting to database '%s'."
	LPTSTR lpReplicObjectInDB;        // IDS_E_REPLIC_OBJECT_IN_DB
	LPTSTR lpInformationMissing;      // IDS_E_INFO_MISSING "Column information for '%s.%s' is missing.  Record deleted?\r\n"
	LPTSTR lpUnBoundedDta;            // IDS_E_UNBOUNDED_DTA "**Unbounded Data Type**"
	LPTSTR lpColumnInfo;              // IDS_F_COLUMN_INFO
	LPTSTR lpTblIntegNoReplicCol;     // IDS_E_TBL_INTEG_NO_REPLIC_COL
	LPTSTR lpKeyColFound;             // IDS_E_KEY_COL_FOUND
	LPTSTR lpTblIntegProbConfig;      // IDS_E_TBL_INTEG_PROB_CONFIG
	LPTSTR lpDescribeStatement;       // IDS_E_DESCRIBE_STATEMENT
	LPTSTR lpTblIntegOpenCursor;      // IDS_E_TBL_INTEG_OPEN_CURSOR
	LPTSTR lpTblIntegFetch;           // IDS_TBL_INTEG_FETCH_INTEG
	LPTSTR lpTblIntegExecImm;         // IDS_E_TBL_INTEG_EXEC_IMM
	LPTSTR lpTblIntegTblDef;          // IDS_TBL_INTEG_TBL_DEF
	LPTSTR lpTblIntegNoDifferences;   // IDS_TBL_INTEG_NO_DIFFERENCES
	LPTSTR lpReplicObjectExist;       // IDS_E_REPLIC_OBJECT_EXIST
} TBLINPUTSTRING , FAR *LPTBLINPUTSTRING;

//chkdistr.sc
typedef struct _chkstringformat
{
	LPTSTR lpUnexpectedRow;          // IDS_E_UNEXPECTED_ROW
	LPTSTR lpGetDbName;              // IDS_E_GET_DB_NAME
	LPTSTR lpIngresReplic;           // IDS_I_INGRES_REPLIC
	LPTSTR lpDistribConfig;          // IDS_I_DISTRIB_CONFIG
	LPTSTR lpLocDb;                  // IDS_I_LOC_DB
	LPTSTR lpDistribDDpathTbl;       // IDS_E_DISTRIB_DD_PATH_TBL
	LPTSTR lpPropagationPath;        // IDS_E_PROPAGATION_PATH
	LPTSTR lpOpenLocalCursor;        // IDS_E_OPEN_LOCAL_CURSOR
	LPTSTR lpFetchingDB;             // IDS_E_FETCHING_DB
	LPTSTR lpCloseLocCursor;         // IDS_E_CLOSE_LOC_CURSOR
	LPTSTR lpLocalRollback;          // IDS_E_LOCAL_ROLLBACK
	LPTSTR lpLocalCommit;            // IDS_E_LOCAL_COMMIT
	LPTSTR lpReleaseSession;         // IDS_ERR_RELEASE_SESSION
	LPTSTR lpCloseTempFile;          // IDS_ERR_CLOSE_TEMP_FILE
	LPTSTR lpReportDB;               // IDS_I_REPORT_DB
	LPTSTR lpMaxConnection;          // IDS_E_MAX_CONNECTION
	LPTSTR lpProhibits;              // IDS_E_PROHIBITS
	LPTSTR lpNoProblem;              // IDS_I_NO_PROBLEM
	LPTSTR lpNoRelease;              // IDS_ERR_RELEASE_SESSION
	LPTSTR lpErrorConnecting;        // IDS_E_CONNECTING
	LPTSTR lpDBAName;                // IDS_F_DBA_NAME
	LPTSTR lpReplicObjectInDB;       // IDS_E_REPLIC_OBJECT_IN_DB
	LPTSTR lpReplicObjectExist;      // IDS_E_REPLIC_OBJECT_EXIST
	LPTSTR lpOpenCursor;             // IDS_E_OPEN_CURSOR
	LPTSTR lpFetching;               // IDS_E_FETCHING
	LPTSTR lpCheckingDDDB;           // IDS_E_CHECKING_DD_DB
	LPTSTR DDDatabasesError;         // IDS_E_DD_DATABASES_ERROR
	LPTSTR lpRecordDB;               // IDS_E_RECORD_DB
	LPTSTR lpDbName;                 // IDS_F_DB_NAME
	LPTSTR lpDbNoAndVnode;           // IDS_E_DBNO_AND_VNODE
	LPTSTR lpDbNoAndDBMS;            // IDS_E_DBNO_AND_DBMS
	LPTSTR lpCloseCursorDDDB;        // IDS_E_CLOSE_CURSOR_DD_DB
	LPTSTR lpOpenCurDDreg;           // IDS_E_OPEN_CUR_DD_REG
	LPTSTR lpFetchDDReg;             // IDS_E_FETCH_DD_REG
	LPTSTR lpCheckDDReg;             // IDS_E_CHECK_DD_REG
	LPTSTR lpDDRegTblError;          // IDS_E_DD_REG_TBL_ERROR
	LPTSTR lpRecordNoNotFound;       // IDS_E_RECORD_NO_NOT_FOUND
	LPTSTR lpSupportTblNotCreate;    // IDS_E_SUPPORT_TBL_NOT_CREATE
	LPTSTR lpColumnRegistred;        // IDS_E_COLUMN_REGISTRED
	LPTSTR lpTblNameError;           // IDS_E_TBL_NAME_ERROR
	LPTSTR lpTblOwnerError;          // IDS_E_TBL_OWNER_ERROR
	LPTSTR lpDBCddsOpenCursor;       // IDS_E_DB_CDDS_OPEN_CURSOR
	LPTSTR lpDDRegistCloseCursor;    // IDS_E_DD_REGIST_CLOSE_CURSOR
	LPTSTR lpDDRegistColOpenCursor;  // IDS_E_DD_REGIST_COL_OPEN_CURSOR
	LPTSTR lpDDRegistColFetchCursor; // IDS_E_DD_REGIST_COL_FETCH_CURSOR
	LPTSTR lpDDRegistColChecking;    // IDS_E_DD_REGIST_COL_CHECKING
	LPTSTR lpDDRegistColError;       // IDS_E_DD_REGIST_COL_ERROR
	LPTSTR lpRecordTable;            // IDS_E_RECORD_TABLE
	LPTSTR lpColumnSequence;         // IDS_E_COLUMN_SEQUENCE
	LPTSTR lpRegistColCloseCursor;   // IDS_E_REGIST_COL_CLOSE_CURSOR
	LPTSTR lpKeySequenceError;       // IDS_E_KEY_SEQUENCE_ERROR
	LPTSTR lpCddsOpenCursor;         // IDS_E_CDDS_OPEN_CURSOR
	LPTSTR lpCddsFetchCursor;        // IDS_E_CDDS_FETCH_CURSOR
	LPTSTR lpCddsChecking;           // IDS_E_CDDS_CHECKING
	LPTSTR lpCddsCheckError;         // IDS_E_CDDS_CHECK_ERROR
	LPTSTR lpCddsNoFound;            // IDS_E_CDDS_NO_FOUND
	LPTSTR lpCddsName;               // IDS_E_CDDS_NAME
	LPTSTR lpCddsCollision;          // IDS_E_CDDS_COLLISION
	LPTSTR lpCddsErrormode;          // IDS_E_CDDS_ERRORMODE
	LPTSTR lpDDCddsCloseCursor;      // IDS_E_DD_CDDS_CLOSE_CURSOR
	LPTSTR lpCheckDBCddsOpenCursor;  // IDS_E_CHECK_DB_CDDS_OPEN_CURSOR
	LPTSTR lpCddsFetchingCursor;     // IDS_E_CDDS_FETCHING_CURSOR
	LPTSTR lpCddsCheckingDBno;       // IDS_E_CDDS_CHECKING_DB_NO
	LPTSTR lpCddsError;              // IDS_E_CDDS_ERROR
	LPTSTR lpCddsDBno;               // IDS_E_CDDS_DB_NO
	LPTSTR lpCddsTargetType;         // IDS_E_CDDS_TARGET_TYPE
	LPTSTR lpCddsCloseCursor;        // IDS_E_CDDS_CLOSE_CURSOR
	LPTSTR lpPathOpenCursor;         // IDS_E_PATH_OPEN_CURSOR
	LPTSTR lpPathFetchingCursor;     // IDS_E_PATH_FETCHING_CURSOR
	LPTSTR lpDDPathTbl;              // IDS_E_DD_PATH_TBL
	LPTSTR lpDDPathError;            // IDS_E_DD_PATH_ERROR
	LPTSTR lpRecordCddsNo;           // IDS_E_RECORD_CDDS_NO
	LPTSTR lpCddsLocalNo;            // IDS_E_CDDS_LOCAL_NO
	LPTSTR lpClosingCursor;          // IDS_E_CLOSING_CURSOR
	LPTSTR lpSettingSession;         // IDS_E_SETTING_SESSION

}CHKINPUTSTRING , FAR *LPCHKINPUTSTRING;

// structure needed to return collisions
typedef struct _storevisualinfo
{
	int iNumVisualColumns;
	VISUALSEQUENCE VisualSeq; // define in collisio.h
	VISUALCOL      *VisualCol;// define in collisio.h
} STOREVISUALINFO, FAR *LPSTOREVISUALINFO;

typedef struct _retvisualinfo
{
	int iNumVisualSequence;
	LPSTOREVISUALINFO lpRetVisualSeq;
} RETVISUALINFO, FAR *LPRETVISUALINFO;

typedef struct _cddsinfo
{
	int   CddsNumber;
	TCHAR tcDisplayInfo[200];
}CDDSINFO ,FAR *LPCDDSINFO;

typedef struct _retrievecddsnum
{
	int iNumCdds; // number of structre allocated
	LPCDDSINFO lpCddsInfo;
} RETRIEVECDDSNUM , FAR *LPRETRIEVECDDSNUM;

// Collisio.sc
#define RES_E_COUNT_CONNECTIONS       1050  // IDS_ERR_COUNT_CONNECTIONS 
#define RES_E_CANNOT_ALLOCATE_MEMORY  1051  // IDS_E_CANNOT_ALLOCATE_MEMORY
#define RES_E_OPENING_CURSOR          1052  // IDS_ERR_OPENING_CURSOR
#define RES_E_MAX_CONNECTIONS         1053  // IDS_ERR_MAX_CONNECTIONS
#define RES_E_OPENING_SESSION         1054  // IDS_F_OPENING_SESSION       (tcParam1)
#define RES_E_FETCHING_TARGET_DB      1055  // IDS_ERR_FETCHING_TARGET_DB
#define RES_E_SESSION_NUMBER          1056  // IDS_ERR_SESSION_NUMBER
#define RES_E_IMMEDIATE_EXECUTION     1057  // IDS_ERR_IMMEDIATE_EXECUTION (iParam1)
#define RES_E_EXECUT_IMMEDIAT         1058  // IDS_F_EXECUT_IMMEDIAT       (iParam1)
#define RES_E_RELEASE_SESSION         1059  // IDS_ERR_RELEASE_SESSION
#define RES_E_SELECT_FAILED           1060  // IDS_ERR_SELECT_FAILED
#define RES_E_UPDATE_DD_DISTRIB_QUEUE 1061  // IDS_ERR_UPDATE_DD_DISTRIB_QUEUE
#define RES_E_F_SHADOW_TBL            1062  // IDS_F_SHADOW_TBL            (tcParam1)
#define RES_E_BLANK_TABLE_NAME        1063  // IDS_ERR_BLANK_TABLE_NAME
#define RES_E_F_TBL_NUMBER            1064  // IDS_F_TBL_NUMBER            (iParam1)
#define RES_E_F_INVALID_OBJECT        1065  // IDS_F_INVALID_OBJECT        (iParam1)
#define RES_E_F_FETCHING_DATA         1066  // IDS_F_FETCHING_DATA         (iParam1)
#define RES_E_F_GETTING_COLUMNS       1067  // IDS_F_GETTING_COLUMNS       (iParam1,tcParam1,tcParam2)
#define RES_E_F_REGISTRED_COL         1068  // IDS_F_REGISTRED_COL         (tcParam1,tcParam2)
#define RES_E_GENERATE_SHADOW         1069  // IDS_ERR_GENERATE_SHADOW
#define RES_E_TBL_NUMBER              1070  // IDS_ERR_TBL_NUMBER          (iParam1)
#define RES_E_F_RELATIONSHIP          1071  // IDS_F_RELATIONSHIP          (iParam1)
#define RES_E_NO_COLUMN_REGISTRATION  1072  // IDS_F_NO_COLUMN_REGISTRATION (tcParam1,tcParam2)
#define RES_E_FLOAT_EXEC_IMMEDIATE    1073  // IDS_F_FLOAT_EXEC_IMMEDIATE  (iParam1)
#define RES_E_CHAR_EXECUTE_IMMEDIATE  1074  // IDS_ERR_CHAR_EXECUTE_IMMEDIATE (iParam1)
#define RES_E_INCONSISTENT_COUNT      1075  // IDS_ERR_INCONSISTENT_COUNT  (tcParam1)
#define RES_E_SQL                     1076  // IDS_ERR_SQLERR
#define RES_E_F_COLUMN_INFO           1077  // IDS_F_COLUMN_INFO
#define RES_E_OPEN_REPORT             1078  // IDS_F_OPEN_REPORT (tcParam1)
#define RES_E_ACTIV_SESSION           1079  // IDS_E_ACTIV_SESSION
#define RES_E_INVALID_DB              1080  // IDS_E_INVALID_DB
#define RES_E_MAX_CONN_SEND_EVENT     1081  // IDS_F_MAX_CONN_SEND_EVENT
#define RES_E_DBA_OWNER_EVENT         1082  // IDS_F_DBA_OWNER_EVENT
#define RES_E_SEND_EXECUTE_IMMEDIAT   1083  // IDS_E_SEND_EXECUTE_IMMEDIAT (iParam1)
#define RES_E_NO_INFORMATION_FOUND    1084  // IDS_NO_INFORMATION_FOUND NEW
#define RES_E_CONNECTION_FAILED       1085  // IDS_E_GET_SESSION

#define MASK_RES_E_ACTIVATE_SESSION     2048  //  IDS_ERR_ACTIVATE_SESSION
#define MASK_RES_E_RELEASE_SESSION      4096  //  IDS_ERR_RELEASE_SESSION
#define MASK_RES_E_READ_TEMPORARY_FILE  8192  //  IDS_ERR_READ_TEMPORARY_FILE

// Public functions to retrieve informations
int LIBMON_VdbaColl_Retrieve_Collision ( int CurrentNodeHdl ,char *dbname ,
                                         LPRETVISUALINFO lpRetCol,
                                         LPERRORPARAMETERS lpError);
// free memory allocated by LIBMON_VdbaColl_Retrieve_Collision() function
void LIBMON_VdbaCollFreeMemory( LPRETVISUALINFO lpCurVisual );

//  call in vrepColf.cpp
int LIBMON_UpdateTable4ResolveCollision( int nStatusPriority ,
                                         int nSrcDB ,
                                         int nTransactionID ,
                                         int nSequenceNo ,
                                         char *pTblName,
                                         int nTblNo,
                                         LPERRORPARAMETERS lpError);

TCHAR *LIBMON_table_integrity(int CurrentNodeHdl,TCHAR *CurDbName,short db1,
                              short db2,long table_no, short cdds_no,
                              TCHAR *begin_time,TCHAR *end_time,
                              TCHAR *order_clause,LPTBLINPUTSTRING lpStringFormat,
                              LPERRORPARAMETERS lpErrorParameters);
//  call in rsdistri.cpp
TCHAR *LIBMON_check_distrib_config( int CurrentNodeHdl, char *dbname ,
                                    LPCHKINPUTSTRING    lpStringFormat,
                                    LPERRORPARAMETERS   lpErrorParameters);

void LIBMON_Vdba_FreeMemory(TCHAR **tmp);


int LIBMON_SendEvent( char *vnode, char *dbname,char *evt_name,char *flag_name,
                      char *svr_no, LPERRORPARAMETERS lpError);

int LIBMON_GetCddsNumInLookupTbl(int hdl, TCHAR *lpDbName, TCHAR *cdds_lookup_tbl,
                                 LPRETRIEVECDDSNUM lpInfo);

int LIBMON_ReplMonGetQueues(int hnode,LPUCHAR lpDBName, int * pinput, int * pdistrib);

int LIBMON_GetCddsName (int hdl ,TCHAR *lpDbName, int cdds_num , TCHAR *lpcddsname);

int LIBMON_FilledSpecialRunNode( long hdl,  LPRESOURCEDATAMIN pResDtaMin,
                                 LPREPLICSERVERDATAMIN pReplSvrDtaBeforeRunNode,
                                 LPREPLICSERVERDATAMIN pReplSvrDtaAfterRunNode);

int LIBMON_MonitorReplicatorStopSvr ( int hdl, int svrNo ,LPUCHAR lpDbName,
                                      LPERRORPARAMETERS lpError );


void LIBMON_SetSessionStart(long nStart);
void LIBMON_SetSessionDescription(LPCTSTR lpszDescription);
extern HANDLE hgEventDataChange; // Global valriable
HANDLE LIBMON_CreateEventDataChange();

int LIBMON_SaveCache(void ** ppbuf, int * plen);
void LIBMON_FreeSaveCacheBuffer(void * pbuf);
int LIBMON_LoadCache(void *pbuf, int ilen);

VOID   LIBMON_SetTemporaryFileName(LPTSTR pFileName);
LPTSTR LIBMON_GetTemporaryFileName();
void LIBMON_EnableSqlErrorManagmt();
void LIBMON_DisableSqlErrorManagmt();

#endif //#ifndef LIBMON_HEADER

