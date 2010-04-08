/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**   Project : CA/OpenIngres Visual DBA
**
**   Source : dbaginfo.h
**   Low level read for cache
**
**  History after 01-01-2000
**
**  28-01-2000 (noifr01)
**   (bug  100196) added prototypes of functions for managing the
**  "special state" of the nodes list in the cache when VDBA is invoked
**  "in the context"
**  21-Mar-2000 (noifr01)
**   (bug 100951) added definition of SESSION_KEEP_DEFAULTISOLATION,
**   bit to be set when getting a session in the cache if the session
**   needs to keep its default isolation level
**  18-Dec-2000 (noifr01)
**   (bug 103515) added new SESSION_CHECKING_CONNECTION flag for GetSession(),
**   to be used when checking the connection, so that GetSession() doesn't 
**   rely on certain flags that are set only after the level of the DBMS
**   is known
**  20-Dec-2000 (noifr01)
**   (SIR 103548)
**   changed prototypes of functions that detect whether certain processes 
**   are running.
**  26-Mar-2001 (noifr01)
**   (sir 104270) removal of code for managing Ingres/Desktop
**  23-Nov-2001 (noifr01)
**   added #if[n]def's for the libmon project
**   24-Mar-2004 (shaha03)
**   (sir 112040) added LIBMON_DBAGetFirstObject() to suuport the multi-threaded context with the same purpose of DBAGetFirstObject()
**   (sir 112040) added LIBMON_DBAGetNextObject()to support the multi-threaded context for the same purpose of DBAGetNextObject()
**  09-Mar-2010 (drivi01)
**   SIR 123397
**   Update NodeCheckConnection to return STATUS code instead of BOOL.
**   The error code will be used to get the error from ERlookup.
**   Add new function INGRESII_ERLookup to retrieve an error message.
*********************************************************************/

#ifndef DBAGINFO_HEADER
#define DBAGINFO_HEADER


#define SESSION_TYPE_INDEPENDENT     1
#define SESSION_TYPE_CACHEREADLOCK   2
#define SESSION_TYPE_CACHENOREADLOCK 3

#define SESSION_KEEP_DEFAULTISOLATION 8 /* must be power of 2 and > biggest SESSION_TYPE_XXX definition*/
#define SESSION_CHECKING_CONNECTION  16 /* must be power of 2 and > biggest SESSION_TYPE_XXX definition*/


#define RELEASE_COMMIT   1
#define RELEASE_ROLLBACK 2
#define RELEASE_NONE     3


typedef struct ReplicDBE {
	int EventType;
	UCHAR DBName[MAXOBJECTNAME];
	UCHAR TableName[MAXOBJECTNAME];
#ifdef LIBMON_HEADER
   int ihdl;
#endif
} RAISEDREPLICDBE, FAR * LPRAISEDREPLICDBE;

typedef struct RaisedDBE {
   UCHAR StrTimeStamp[60];
   UCHAR DBEventName[MAXOBJECTNAME];
   UCHAR DBEventOwner[MAXOBJECTNAME];
   UCHAR DBEventText[256];
} RAISEDDBE, FAR * LPRAISEDDBE;

#ifdef LIBMON_HEADER
	#ifndef BLD_MULTI  /*To make available in Non multi-thread context only */
		int DBEScanDBEvents(void *pDoc, LPRAISEDREPLICDBE praiseddbe);/* function to be called every x seconds, each time as */                                                 /* many times as dbevents are found                    */
	#endif
#else
	#ifndef BLD_MULTI  /*To make available in Non multi-thread context only */
		int DBEScanDBEvents(void);  // function to be called every x seconds
	#endif
int DBETraceDisplayRaisedDBE(HWND hwnd, LPRAISEDDBE pStructRaisedDbe);
int  DBAGetFirstObjectLL(LPUCHAR lpVirtNode,
                         int iobjecttype,
                         int level,
                         LPUCHAR *parentstrings,
                         BOOL bWithSystem,
                         LPUCHAR lpobjectname,
                         LPUCHAR lpownername,
                         LPUCHAR lpextradata);
int DBETraceInit(int hnode,LPUCHAR lpDBName,HWND hwnd,int *pires);
int DBETraceRegister(int hdl, LPUCHAR DBEventName, LPUCHAR DBEventOwner);
int DBETraceUnRegister(int hdl, LPUCHAR DBEventName, LPUCHAR DBEventOwner);
int DBETraceTerminate(int hdl);
BOOL GetDBNameFromObjType (int ObjType,  char * buffResult, char* dbName);
#endif

int  DBAGetFirstObject  (LPUCHAR lpVirtNode,
                         int iobjecttype,
                         int level,
                         LPUCHAR *parentstrings,
                         BOOL bWithSystem,
                         LPUCHAR lpobjectname,
                         LPUCHAR lpownername,
                         LPUCHAR lpextradata);

int  LIBMON_DBAGetFirstObject  (LPUCHAR lpVirtNode,
                         int iobjecttype,
                         int level,
                         LPUCHAR *parentstrings,
                         BOOL bWithSystem,
                         LPUCHAR lpobjectname,
                         LPUCHAR lpownername,
                         LPUCHAR lpextradata,
						 int thdl);/* thdl--Thread handle*/

int  DBAGetNextObject   (LPUCHAR lpobjectname, 
                         LPUCHAR lpownername,
                         LPUCHAR lpextradata);

int  LIBMON_DBAGetNextObject   (LPUCHAR lpobjectname, 
                         LPUCHAR lpownername,
                         LPUCHAR lpextradata,
						 int	thdl); /* thdl--Thread handle*/

void DBAginfoInit(void);
void DBAginfoFree(void);

#define LPCLASSPREFIXINNODENAME " (/"
#define LPCLASSSUFFIXINNODENAME ")"

#define LPUSERPREFIXINNODENAME " (user:"
#define LPUSERSUFFIXINNODENAME ")"

BOOL DBAGetUserName (const unsigned char* VirtNodeName, unsigned char* UserName);

#ifndef BLD_MULTI /*To make available in Non multi-thread context only */
int DBAThreadDisableCurrentSession();
int DBAThreadEnableCurrentSession();
#endif

 
int ReleaseSession(int hdl, int releasetype);
int ActivateSession(int hdl);
int CommitSession(int hdl);
int GetImaDBsession(UCHAR *lpnodename, int sessiontype, int *phdl);
BOOL SetSessionUserIdentifier(LPUCHAR username);
BOOL ResetSessionUserIdentifier();
int Getsession( UCHAR *lpsessionname,int sessiontype,int * phdl);
long sessionnumber(int hdl);
LPUCHAR GetSessionName(int hdl);
int OpenSession   ( LPUCHAR nodename);
int CloseSessions ( LPUCHAR nodename); //if null pointer, closes all sessions
int DBACloseNodeSessions(UCHAR * lpNodeName,BOOL bAllGWNodes, BOOL bAllUsers);

#ifndef LIBMON_HEADER
BOOL IsNT();
BOOL ping_iigcn( void );
STATUS INGRESII_ERLookup(int msgid, char **buf, int buf_len);
#endif

BOOL isLastErr50(void);

LPUCHAR GlobalInfoDBName();
int PostCloseIfNeeded(void);
int PostNodeClose(LPUCHAR nodename);
int GetNodeAndDB(LPUCHAR NodeName,LPUCHAR DBName,LPUCHAR SessionName);



#ifndef LIBMON_HEADER
int NodeLLInit(void)                 ;
int NodeLLTerminate(void)            ;
int NodeLLFillNodeLists(void)        ;
int NodeLLFillNodeListsLL(void)      ;
STATUS NodeCheckConnection(LPUCHAR host);
LPUCHAR * GetGWlist(LPUCHAR host);
LPUCHAR * GetGWlistLL(LPUCHAR host, BOOL bDetectLocalServersOnly);
#endif

BOOL IsIngresIISession();



#define REPLIC_DBE_NONE               0

#define REPLIC_DBE_OUT_INSERT         1
#define REPLIC_DBE_OUT_UPDATE         2
#define REPLIC_DBE_OUT_DELETE         3
#define REPLIC_DBE_OUT_TRANSACTION    4
#define REPLIC_DBE_IN_INSERT          5
#define REPLIC_DBE_IN_UPDATE          6
#define REPLIC_DBE_IN_DELETE          7
#define REPLIC_DBE_IN_TRANSACTION     8

/*To make available in Non multi-thread context only */
#ifndef BLD_MULTI /*To make available in Non multi-thread context only */
	#ifdef LIBMON_HEADER
		int DBEReplMonInit(int hnode,LPUCHAR lpDBName,int *pires, void *pDoc);
		int DBEReplMonTerminate(int hdl,void *pDoc);
		int DBEReplReplReleaseEntry(int hdl,void *pDoc);
		int DBEReplGetEntry(LPUCHAR lpNodeName,LPUCHAR lpDBName,int *pires, int serverno,void *pDoc);
	#else
		int DBEReplMonInit(int hnode,LPUCHAR lpDBName,int *pires);
		int DBEReplMonTerminate(int hdl);
		int DBEReplReplReleaseEntry(int hdl);
		int DBEReplGetEntry(LPUCHAR lpNodeName,LPUCHAR lpDBName,int *pires, int serverno);
	#endif
#endif 

int DBECacheEntriesUsedByReplMon(int hnode);

BOOL isNodeWithUserUsedByReplMon(LPUCHAR lpNodeName);

int DBEReplMonDisplayRaisedDBE(int hdl, LPRAISEDREPLICDBE pStructRaisedReplicDbe);

BOOL IsInvokedInContextWithOneWinExactly();
int OneWinExactlyGetWinType();

int ReplMonGetQueues(int hnode,LPUCHAR lpDBName, int * pinput, int * pdistrib);

BOOL GetGWClassNameFromString(LPUCHAR lpstring, LPUCHAR lpdestbuffer);
void RemoveGWNameFromString(LPUCHAR lpstring);
BOOL GetConnectUserFromString(LPUCHAR lpstring, LPUCHAR lpdestbuffer);
void RemoveConnectUserFromString(LPUCHAR lpstring);
// Manage DD_SERVER_SPECIAL
int CreateDD_SERVER_SPECIALIfNotExist();
int DeleteRowsInDD_SERVER_SPECIAL();
int DropInDD_SERVER_SPECIAL();

// Context-driven acceleration functions
// NOTE : buffer assumed to be of MAXOBJECTNAME size
BOOL GetContextDrivenNodeName(char* buffer);
BOOL GetContextDrivenServerClassName(char* buffer);
BOOL VDBA_InvokedInContextOneDocExactlyGetDocName(char* buf, int bufsize);

/* management of the special state of the nodes list when VDBA is invoked in the context */
extern void InitContextNodeListStatus();
extern void RefreshNodesListIfNeededUponNextFetch();
extern BOOL isNodesListInSpecialState();

/* 27-Mar-2001 : moved from oidttool.h, because applies also to non-oidt functionalities */

/*----------------------------------------------------------- ConcateStrings -*/
/* The function that concates strings (ioBuffer, aString, ..)                 */
/*                                                                            */
/* Parameter:                                                                 */
/*     1) ioBuffer: In Out Buffer                                             */
/*     2) aString : String to be concated.                                    */
/*     3) ...     : Ellipse for other strings...with NULL terminated (char*)0 */
/* Return:                                                                    */
/*     return  1 if Successful.                                               */   
/*     return  0 otherwise.                                                   */   
/*----------------------------------------------------------------------------*/
int
ConcateStrings (LPSTR* ioBuffer, LPSTR aString, ...);

#endif /* #define DBAGINFO_HEADER */

