/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Project  : Web DBA tools
**
**    Source   : lbbknode.h
**
** 06-May-2004 (shaha03)
**    SIR #112040, file created based on nodes.h of LibIngll.lib
**		   Added LIBBK_GetCurrentUserName(),
**				 LIBBK_ImpersonateUser(),
**		         LIBBK_RevertBackTheUser() to support impersonation
**		   Added LIBBK_setUser(),
**				 LIBBK_setBackOrgUser() to set userid variable, to access 
**			     different users Node info.
**		   Moved all the structure definitions from nodes.c of libingll to this file
**	29-June-2004 (shaha03)
**	  SIR #112040, changed the proto type for LIBBK_NodeLLInit()
**					to pass call back function pointer to display error.
** 25-May-2010 (drivi01) Bug 123817
**    Fix the object length to allow for long IDs.
**    Remove hard coded constants, use DB_MAXNAME instead.
*/

#if !defined (_LIBBK_NODES_C_HEADER_FILE_)
#define _LIBBK_NODES_C_HEADER_FILE_
#if defined (__cplusplus)
extern "C"
{
#endif
#include <compat.h>
#include <windows.h>
#include <string.h>
#include <time.h>
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
   bool bSyncOnParent;
   bool bSync4SameType;
   bool bOnLoad;
} FREQUENCY, FAR * LPFREQUENCY;

typedef struct DomRefreshParams {
   time_t LastRefreshTime;
   long   LastDOMUpdver;
   bool   bUseLocal;
   int    iobjecttype;
   FREQUENCY frequency;
} DOMREFRESHPARAMS, FAR * LPDOMREFRESHPARAMS;

typedef struct NodeServerClassDataMin { 
   UCHAR NodeName   [MAXOBJECTNAME];
   bool bIsLocal;
   bool isSimplifiedModeOK;

   UCHAR ServerClass[MAXOBJECTNAME];
   
} NODESVRCLASSDATAMIN, FAR * LPNODESVRCLASSDATAMIN;

typedef struct NodeUserDataMin { 
   UCHAR NodeName   [MAXOBJECTNAME];
   bool  bIsLocal;
   bool  isSimplifiedModeOK;

   UCHAR ServerClass[MAXOBJECTNAME];

   UCHAR User[MAXOBJECTNAME];
   
} NODEUSERDATAMIN, FAR * LPNODEUSERDATAMIN;

typedef struct NodeLoginDataMin { 
   UCHAR NodeName   [MAXOBJECTNAME];

   bool bPrivate;

   UCHAR Login      [MAXOBJECTNAME];
   UCHAR Passwd     [MAXOBJECTNAME];

#ifdef	EDBC
   bool bSave;
#endif

} NODELOGINDATAMIN, FAR * LPNODELOGINDATAMIN;

typedef struct NodeConnectDataMin { 
   UCHAR NodeName   [MAXOBJECTNAME];

   bool bPrivate;

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

   bool bPrivate;

   UCHAR AttributeName [MAXOBJECTNAME];
   UCHAR AttributeValue[MAXOBJECTNAME];

} NODEATTRIBUTEDATAMIN, FAR * LPNODEATTRIBUTEDATAMIN;

typedef struct NodeDataMin { 
   UCHAR NodeName   [MAXOBJECTNAME];
   bool bIsLocal;
   bool isSimplifiedModeOK;

   bool bWrongLocalName; // true if local node defined differently that with user * (only)
   bool bInstallPassword;
   bool bTooMuchInfoInInstallPassword;

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
   bool HasSystemServerClasses;
   int nbserverclasses;
   int serverclasseserrcode;
   struct NodeServerClassData * lpServerClassData;

   DOMREFRESHPARAMS WindowRefreshParms;
   bool HasSystemWindows;
   int nbwindows;
   int windowserrcode;
   struct WindowData  * lpWindowData;

   DOMREFRESHPARAMS LoginRefreshParms;
   bool HasSystemLogins;
   int nblogins;
   int loginerrcode;
   struct NodeLoginData  * lpNodeLoginData;

   DOMREFRESHPARAMS NodeConnectionRefreshParms;
   bool HasSystemNodeConnections;
   int nbNodeConnections;
   int NodeConnectionerrcode;
   struct NodeConnectionData  * lpNodeConnectionData;

   DOMREFRESHPARAMS NodeAttributeRefreshParms;
   bool HasSystemNodeAttributes;
   int nbNodeAttributes;
   int NodeAttributeerrcode;
   struct NodeAttributeData  * lpNodeAttributeData;

   struct MonNodeData * ptemp;

   struct MonNodeData * pnext;
} MONNODEDATA, FAR * LPMONNODEDATA;

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

	bool bError;

} NETTRAFIC;

typedef struct tagNETLOCALVNODENAME
{
	TCHAR tcLocalVnode[MAXOBJECTLEN*4];

	bool bError;

} NETLOCALVNODENAME;



LPMONNODEDATA       LIBBK_GetFirstLLNode         (void);
LPNODECONNECTDATA   LIBBK_GetFirstLLConnectData  (void);
LPNODELOGINDATA     LIBBK_GetFirstLLLoginData    (void);
LPNODEATTRIBUTEDATA LIBBK_GetFirstLLAttributeData(void);

bool LIBBK_NodeLL_FillHostName(char *stNodeName);
bool LIBBK_NodeLL_IsVnodeOptionDefined(void);

void LIBBK_MuteLLRefresh();
void LIBBK_UnMuteLLRefresh();

int LIBBK_NodeLLInit(void (*callBackFn)(char*, char*, bool));
int LIBBK_NodeLLTerminate(void)            ;
int LIBBK_NodeLLFillNodeLists(void);

int LIBBK_LLAddFullNode           (LPNODEDATAMIN lpNData);
int LIBBK_LLAddNodeConnectData    (LPNODECONNECTDATAMIN lpNCData);
int LIBBK_LLAddNodeLoginData      (LPNODELOGINDATAMIN lpNLData);
int LIBBK_LLAddNodeAttributeData  (LPNODEATTRIBUTEDATAMIN lpAttributeData);
int LIBBK_LLDropFullNode          (LPNODEDATAMIN lpNData);
int LIBBK_LLDropNodeConnectData   (LPNODECONNECTDATAMIN lpNCData);
int LIBBK_LLDropNodeLoginData     (LPNODELOGINDATAMIN lpNLData);
int LIBBK_LLDropNodeAttributeData (LPNODEATTRIBUTEDATAMIN lpAttributeData);
int LIBBK_LLAlterFullNode         (LPNODEDATAMIN lpNDataold,LPNODEDATAMIN lpNDatanew);
int LIBBK_LLAlterNodeConnectData  (LPNODECONNECTDATAMIN lpNCDataold,LPNODECONNECTDATAMIN lpNCDatanew);
int LIBBK_LLAlterNodeLoginData    (LPNODELOGINDATAMIN lpNLDataold,LPNODELOGINDATAMIN lpNLDatanew);
int LIBBK_LLAlterNodeAttributeData(LPNODEATTRIBUTEDATAMIN lpAttributeDataold,LPNODEATTRIBUTEDATAMIN lpAttributeDatanew);
#ifdef	EDBC
int LIBBK_LLManageSaveLogin(LPUCHAR ServerName, bool bSave);
int LIBBK_GetConnectionString(char *ServerName, char * ConnStr);
#endif

bool LIBBK_NodeExists(LPUCHAR NodeName);

// LIBBK_CompleteNodeStruct returns:
//  RES_SUCCESS : OK
//  RES_ENDOFDATA: node was not found
//  RES_NOT_SIMPLIFIEDM0DE: not simplified mode (cannot be altered)

int LIBBK_CompleteNodeStruct(LPNODEDATAMIN lpnodedata);

bool LIBBK_NodeCheckConnection(LPUCHAR host);

char *LIBBK_GetMonInfoName(int iobjecttype, void *pstruct, char *buf,int bufsize);
int LIBBK_NodeLLFillNodeLists_NoLocal(void);



bool LIBBK_NetQueryTraficInfo(LPTSTR lpszNode, LPTSTR lpszListenAddress, NETTRAFIC* pTraficInfo);

bool LIBBK_NetQueryLocalVnode(NETLOCALVNODENAME *pLocalVnode);
void LIBBK_ReleaseMutexGCA_Initiate_Terminate();

bool LIBBK_GetCurrentUserName(char *, int *);

/*In case of impersonation */
bool LIBBK_ImpersonateUser(char *user
#ifndef UNIX
					 ,char *passwd
					,char *domain
#endif
					);
bool LIBBK_RevertBackTheUser();
bool LIBBK_setUser(char *);
bool LIBBK_setBackOrgUser();

#if defined (__cplusplus)
}
#endif


#endif //_NODES_C_HEADER_FILE_
