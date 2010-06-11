/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Project  : Ingres Visual DBA
**
**    Source   : nodes.c
**
**    low level for list of nodes
**  History:
**	15-aug-2001 (somsa01)
**	    For MAINWIN, do not typedef UCHAR.
**  02-May-2002 (noifr01)
**    (sir 106648) VDBA split project. Cleaned up functions for detecting
**    whether the installation is started, for usage by calling apps (vdbamon,
**    vdbasql, iea, iia, etc....)(rather than providing a low-level access
**    error)
** 04-Mar-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
** 19-May-2003 (uk$so01)
**    SIR #110269, Add Network trafic monitoring.
** 18-Sep-2003 (uk$so01)
**    SIR #106648, Integrate the libingll.lib and cleanup codes.
** 03-Oct-2003 (schph01)
**    SIR #109864 Add Prototype for NodeLL_FillHostName() and 
**    NodeLL_IsVnodeOptionDefined()
** 03-Nov-2003 (noifr01)
**  upon checks after massive ingres30->main code propagation, added
**  trailing CR for avoiding potential future propagation problems
** 14-Nov-2003 (uk$so01)
**    BUG #110983, If GCA Protocol Level = 50 then use MIB Objects 
**    to query the server classes.
** 19-Dec-2003 (uk$so01)
**    SIR #111475, Coorperative shutdown between the DBMS Client & Server.
**    Created.
** 09-Mar-2010 (drivi01) SIR 123397
**    Update NodeCheckConnection to return STATUS code instead of BOOL.
**    The error code will be used to get the error from ERlookup.
**    Add new function INGRESII_ERLookup to retrieve an error message.
**  25-May-2010 (drivi01) Bug 123817
**    Fix the object length to allow for long IDs.
**    Remove hard coded constants, use DB_MAXNAME instead.

*/
#if !defined (_NODES_C_HEADER_FILE_)
#define _NODES_C_HEADER_FILE_
#if defined (__cplusplus)
extern "C"
{
#endif


#include <windows.h>
#include <string.h>
#include <time.h>
#include <compat.h>
#include <iicommon.h>

typedef HWND DISPWND;
#ifndef MAINWIN
typedef unsigned char UCHAR;   
#endif  /* MAINWIN */
typedef unsigned char * LPUCHAR;

#define MAXOBJECTNAME   (DB_MAXNAME*2 + 3)
#define RES_ERR                 0
#define RES_SUCCESS             1
#define RES_TIMEOUT             2
#define RES_NOGRANT             3
#define RES_ENDOFDATA           4
#define RES_NOT_SIMPLIFIEDM0DE  1100

#define GWTYPE_NONE         0
#define GWTYPE_OIDT         1

#define OBT_NODE                    1051
#define OBT_NODE_OPENWIN            1052
#define OBT_NODE_LOGINPSW           1053
#define OBT_NODE_CONNECTIONDATA     1054
#define OBT_NODE_SERVERCLASS        1055
#define OBT_NODE_ATTRIBUTE          1056

#define ERR_ALLOCMEM         5
#define ERR_LIST            13
#define ERR_GW              26

typedef struct dbafrequency {
   int  count;
   int  unit;
   BOOL bSyncOnParent;
   BOOL bSync4SameType;
   BOOL bOnLoad;
} FREQUENCY, FAR * LPFREQUENCY;

typedef struct DomRefreshParams {
   time_t LastRefreshTime;
   long   LastDOMUpdver;
   BOOL   bUseLocal;
   int    iobjecttype;
   FREQUENCY frequency;
} DOMREFRESHPARAMS, FAR * LPDOMREFRESHPARAMS;

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

#ifdef	EDBC
   BOOL bSave;
#endif

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


typedef struct WindowData  {

   WINDOWDATAMIN WindowDta;

   struct WindowData  * pnext;
} NODEWINDOWDATA, FAR * LPNODEWINDOWDATA;

typedef struct NodeServerClassData  {

   NODESVRCLASSDATAMIN NodeSvrClassDta;

   struct NodeServerClassData  * pnext;
}  NODESERVERCLASSDATA, FAR * LPNODESERVERCLASSDATA;

typedef struct NodeLoginData  {

   NODELOGINDATAMIN NodeLoginDta;

   struct NodeLoginData  * pnext;
} NODELOGINDATA, FAR * LPNODELOGINDATA;

typedef struct NodeConnectionData {

   NODECONNECTDATAMIN NodeConnectionDta;

   struct NodeConnectionData * pnext;
} NODECONNECTDATA, FAR * LPNODECONNECTDATA;

typedef struct NodeAttributeData {

   NODEATTRIBUTEDATAMIN NodeAttributeDta;

   struct NodeAttributeData * pnext;
} NODEATTRIBUTEDATA, FAR * LPNODEATTRIBUTEDATA;

typedef struct MonNodeData {

   NODEDATAMIN MonNodeDta;

   DOMREFRESHPARAMS ServerClassRefreshParms;
   BOOL HasSystemServerClasses;
   int nbserverclasses;
   int serverclasseserrcode;
   struct NodeServerClassData * lpServerClassData;

   DOMREFRESHPARAMS WindowRefreshParms;
   BOOL HasSystemWindows;
   int nbwindows;
   int windowserrcode;
   struct WindowData  * lpWindowData;

   DOMREFRESHPARAMS LoginRefreshParms;
   BOOL HasSystemLogins;
   int nblogins;
   int loginerrcode;
   struct NodeLoginData  * lpNodeLoginData;

   DOMREFRESHPARAMS NodeConnectionRefreshParms;
   BOOL HasSystemNodeConnections;
   int nbNodeConnections;
   int NodeConnectionerrcode;
   struct NodeConnectionData  * lpNodeConnectionData;

   DOMREFRESHPARAMS NodeAttributeRefreshParms;
   BOOL HasSystemNodeAttributes;
   int nbNodeAttributes;
   int NodeAttributeerrcode;
   struct NodeAttributeData  * lpNodeAttributeData;

   struct MonNodeData * ptemp;

   struct MonNodeData * pnext;
} MONNODEDATA, FAR * LPMONNODEDATA;


LPMONNODEDATA       GetFirstLLNode         (void);
LPNODECONNECTDATA   GetFirstLLConnectData  (void);
LPNODELOGINDATA     GetFirstLLLoginData    (void);
LPNODEATTRIBUTEDATA GetFirstLLAttributeData(void);

BOOL NodeLL_FillHostName(LPCTSTR stNodeName);
BOOL NodeLL_IsVnodeOptionDefined(void);

void MuteLLRefresh();
void UnMuteLLRefresh();

int NodeLLInit(void)                 ;
int NodeLLTerminate(void)            ;
int NodeLLFillNodeLists(void);

int LLAddFullNode           (LPNODEDATAMIN lpNData);
int LLAddNodeConnectData    (LPNODECONNECTDATAMIN lpNCData);
int LLAddNodeLoginData      (LPNODELOGINDATAMIN lpNLData);
int LLAddNodeAttributeData  (LPNODEATTRIBUTEDATAMIN lpAttributeData);
int LLDropFullNode          (LPNODEDATAMIN lpNData);
int LLDropNodeConnectData   (LPNODECONNECTDATAMIN lpNCData);
int LLDropNodeLoginData     (LPNODELOGINDATAMIN lpNLData);
int LLDropNodeAttributeData (LPNODEATTRIBUTEDATAMIN lpAttributeData);
int LLAlterFullNode         (LPNODEDATAMIN lpNDataold,LPNODEDATAMIN lpNDatanew);
int LLAlterNodeConnectData  (LPNODECONNECTDATAMIN lpNCDataold,LPNODECONNECTDATAMIN lpNCDatanew);
int LLAlterNodeLoginData    (LPNODELOGINDATAMIN lpNLDataold,LPNODELOGINDATAMIN lpNLDatanew);
int LLAlterNodeAttributeData(LPNODEATTRIBUTEDATAMIN lpAttributeDataold,LPNODEATTRIBUTEDATAMIN lpAttributeDatanew);
#ifdef	EDBC
int LLManageSaveLogin(LPUCHAR ServerName, BOOL bSave);
int GetConnectionString(LPCTSTR ServerName, char * ConnStr);
#endif

BOOL NodeExists(LPUCHAR NodeName);

// CompleteNodeStruct returns:
//  RES_SUCCESS : OK
//  RES_ENDOFDATA: node was not found
//  RES_NOT_SIMPLIFIEDM0DE: not simplified mode (cannot be altered)

int CompleteNodeStruct(LPNODEDATAMIN lpnodedata);
STATUS NodeCheckConnection(LPUCHAR host);

char *GetMonInfoName(int iobjecttype, void *pstruct, char *buf,int bufsize);
int NodeLLFillNodeLists_NoLocal(void);


typedef struct tagNETTRAFIC
{
	long nInboundMax;
	long nInbound;
	long nOutboundMax;
	long nOutbound;

	long nPacketIn;
	long nPacketOut;
	long nDataIn;
	long nDataOut;

	BOOL bError;

} NETTRAFIC;
BOOL NetQueryTraficInfo(LPTSTR lpszNode, LPTSTR lpszListenAddress, NETTRAFIC* pTraficInfo);

typedef struct tagNETLOCALVNODENAME
{
	TCHAR tcLocalVnode[256];

	BOOL bError;

} NETLOCALVNODENAME;

BOOL NetQueryLocalVnode(NETLOCALVNODENAME *pLocalVnode);
void ReleaseMutexGCA_Initiate_Terminate();


#if defined (__cplusplus)
}
#endif
#endif //_NODES_C_HEADER_FILE_
