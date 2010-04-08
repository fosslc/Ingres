#define INGRES25 
// #define DONTUSEFASTGWDETECTION

/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Project  : Ingres Visual DBA
**
**  Source   : nodes.c
**
**  low level for list of nodes
**
**  History:  
**  29-Jun-1999 (noifr01) 
**    (bug 97679): extended the criteria for showing the "(local") node
**    to the presence of a running gateway. 
**  18-Jan-2000 (noifr01) 
**    (bug 100058) : added ADABAS to the list
**     of MVS possible gateways (should avoid a future patch of VDBA when 
**     this gateway will be available)
**	20-Jul-2000 (hanch04)
**	   Use ping_iigcn for UNIX.  This should be used by all platforms.
**  05-Sep-2000 (hanch04)
**     replace nat and longnat with i4
**  25-Oct-2000 (noifr01)
**     (bug 102276) removed the DESKTOP,NT_GENERIC and IMPORT_DLL_DATA 
**     preprocessor definitions that were hardcoded for NT platforms specifically,
**     given that this is now done at the .dsp project files level
**  20-Dec-2000 (noifr01)
**   (SIR 103548) use new functions for determining whether certain processes
**   are running 
**  26-Mar-2001 (noifr01)
**   (sir 104270) removal of code for managing Ingres/Desktop
**  10-oct-2001 (noifr01)
**    (sir 105989) Add VANT for Vantage as a new possible server class
**  16-Apr-2002 (noifr01)
**    (sir 107587) Add DB2UDB as a new possible server class
**  22-Sep-2002 (schph01)
**    bug 108645 Replace the call of CGhostname() by the GetLocalVnodeName()
**    function.( XX.XXXXX.gcn.local_vnode parameter in config.dat)
** 14-Nov-2003 (uk$so01)
**    BUG #110983, If GCA Protocol Level = 50 then use MIB Objects 
**    to query the server classes.
** 09-Mar-2004 (uk$so01)
**    SIR #110013 (Handle the DAS Server.-> exclude it from server class)
** 10-Nov-2004 (schph01)
**    SIR 113426 Add new function IsIceServerLaunched() for verified if the
**    Ice Server was launched on the current vnode.
** 24-Jun-2009 (smeke01) b121675
**    Ignore innocuous MO_NO_NEXT returned from GCA_FASTSELECT for 
**    non-privileged user. Also removed unused code involving err_fatal.
** 09-Mar-2010 (drivi01) SIR 123397
**    Update NodeCheckConnection to return STATUS code instead of BOOL.
**    The error code will be used to get the error from ERlookup.
**    Add new function INGRESII_ERLookup to retrieve an error message.
********************************************************************/

#include   "dba.h"
#include   "esltools.h"
#include   "main.h"    // hInst
#include   "domdloca.h"
#include   "monitor.h"
#include   "dbaginfo.h"
#include   "msghandl.h"
#include   "domdata.h"
#include   "dll.h"
#include   "extccall.h"
#include   "resource.h"
#include  "prodname.h"
#include   <malloc.h>
#include   <tchar.h>

#include    <compat.h>
#include	   <me.h>
#include    <si.h>
#include    <st.h>
#include    <er.h>
//#include    <erst.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <gc.h>
#include    <gca.h>
#include    <gcn.h>

#ifdef INGRES25
#include    <gcu.h>
#endif

#include    <qu.h>
#include    <tm.h>
#include    <nm.h>
#include    <gcnint.h>
#include    <id.h>
#include    <pm.h>
#include    <gcm.h>

/* Included for #define of E_GL400C_MO_NO_NEXT */
#include    <erglf.h>

typedef struct tagLISTTCHAR
{
	TCHAR* lptchItem;
	struct tagLISTTCHAR* next;
} LISTTCHAR, FAR* LPLISTTCHAR;
LPLISTTCHAR ListTchar_Add  (LPLISTTCHAR oldList, LPCTSTR lpszString);
LPLISTTCHAR ListTchar_Done (LPLISTTCHAR oldList);

typedef struct tagMIBOBJECT
{
	TCHAR tchszMIBCHASS[256]; // Example:  "exp.gcf.gcn.server.class"
	LPLISTTCHAR listValues;   // Example:   if not null, the list of strings represent the server classes.
	struct tagMIBOBJECT* next;
} MIBOBJECT, FAR* LPMIBOBJECT;
LPMIBOBJECT MibObject_Add  (LPMIBOBJECT oldList, LPCTSTR lpszMibClass);
LPMIBOBJECT MibObject_Done (LPMIBOBJECT oldList);

typedef struct tagMIBREQUEST
{
	LPMIBOBJECT listMIB;
	BOOL bError;
	BOOL bEnd;
} MIBREQUEST, FAR* LPMIBREQUEST;
extern MIBREQUEST g_mibRequest;
BOOL QueryMIBObjects(LPMIBREQUEST pRequest, LPCTSTR lpszTarget, int nMode);
LPLISTTCHAR GetListServerClass(LPCTSTR lpszNode);
//LPLISTTCHAR GetGWlist(LPCTSTR lpszNode);

#ifndef INGRES25                    /* this section is for demo only on a 2.0 install*/
#define GCN_NET_FLAG  GCN_DEF_FLAG  /*(values are different but not accepted by older installs. for demo only */
#define gcu_encode(a,b,c) gcn_encrypt(a,b,c)
#define gcu_get_int(a,b)  gcn_get_int(a,b)
#define gcu_get_str(a,b)  gcn_get_str(a,b)
#define gcu_words( a, b, c, d, e ) gcn_words( a, b, c, d, e)
#endif

static i4	lastOpStat;
static char	*hostName = NULL;

static LPNODELOGINDATA     lplllogindata     = NULL;
static LPNODECONNECTDATA   lpllconnectdata   = NULL;
static LPNODEATTRIBUTEDATA lpllattributedata = NULL;

static LPMONNODEDATA     lpllnodedata         = NULL;
static LPNODELOGINDATA   lpcurlogindata       = NULL;
static LPNODECONNECTDATA lpcurconnectdata     = NULL;
static LPNODEATTRIBUTEDATA lpcurattributedata = NULL;
static BOOL bpriv;
static int  ilConnectEntry;
static int  inodesLLlevel = 0;
static BOOL bErrInit = FALSE;
static char username[80];

// forward references
       i4	n_gcn_response();
static i4	gcn_respsh();
static STATUS nv_api_call(i4 flag, char *obj, i4 opcode, char *value, char *type);

static i4 gcn_err_func(char *msg) { return 0 ; }

LPMONNODEDATA GetFirstLLNode()
{
   return lpllnodedata;
}

LPNODECONNECTDATA GetFirstLLConnectData()
{
   return lpllconnectdata;
}

LPNODELOGINDATA   GetFirstLLLoginData()
{
   return lplllogindata;
}

LPNODEATTRIBUTEDATA   GetFirstLLAttributeData()
{
   return lpllattributedata;
}

#ifndef WIN32
BOOL IsQueryBlockingDefined()
{
  char	*env = 0;

  NMgtAt( "II_QUERY_BLOCKING", &env);

  if (0) {  // temporary code
      if (env && (*env=='t' || *env == 'T'))
         return TRUE;
      return FALSE;
  }

  if (env && (*env=='f' || *env == 'F'))
    return FALSE;

  return TRUE;
}
#endif

static BOOL bMEAdviseInit = FALSE;

int NodeLLInit(void)
{
    STATUS rtn;
    STATUS iret;
    char * user;

    inodesLLlevel++;
    if (inodesLLlevel>1)
      return RES_SUCCESS;


//    DBACloseNodeSessions("");

    if (!bMEAdviseInit) {
       iret=MEadvise( ME_INGRES_ALLOC );  /* We'll use the Ingres malloc library */
//       if (iret != OK) { // removed because may have been called by Isprocessrunning()
//         bErrInit = TRUE;
//         return RES_ERR;
//       }
       SIeqinit();
       bMEAdviseInit = TRUE;
    }

    if (OK != (rtn = IIGCn_call( GCN_INITIATE, NULL ))) {
       bErrInit = TRUE;
	    return RES_ERR;
    }

#ifndef WIN32
    if ( (rtn= IIgcn_check()) != OK ) {
	   IIGCn_call( GCN_TERMINATE, NULL );
      bErrInit = TRUE;
  	   return RES_ERR;
    }
#endif
    gcn_seterr_func( gcn_err_func );
    user = username;
    IDname(&user);
    bErrInit = FALSE;
    return RES_SUCCESS;
}


int NodeLLTerminate(void)
{
    inodesLLlevel--;
    if (inodesLLlevel>0)
      return RES_SUCCESS;

    freeNodeLoginData      (lplllogindata  );
    freeNodeConnectionData (lpllconnectdata);
    freeNodeAttributeData  (lpllattributedata);
    freeMonNodeData        (lpllnodedata);
    lplllogindata     = NULL;
    lpllconnectdata   = NULL;
	lpllattributedata = NULL;
    lpllnodedata      = NULL;
    if (!bErrInit)
      IIGCn_call(GCN_TERMINATE, NULL);
    PostCloseIfNeeded();
    return 1;
}

STATUS NodeCheckConnection(LPUCHAR host)
{

   char target[GCN_EXTNAME_MAX_LEN + 1]; 
   STATUS rtn;

   STpolycat( 2, host, ERx("::/IINMSVR"), target );
   NodeLLInit();
   rtn = gcn_testaddr( target, 0 , NULL );
   NodeLLTerminate();
   return rtn;

}

static	i4	msg_max;
static	char	*msg_buff;

static STATUS
gca_release( i4 assoc_id )
{
    GCA_SD_PARMS	sd_parms;
    STATUS		status;
    STATUS		call_status;
    char		*ptr = msg_buff;

    ptr += gcu_put_int( ptr, 0 );

    MEfill( sizeof( sd_parms ), 0, (PTR)&sd_parms );
    sd_parms.gca_assoc_id = assoc_id;
    sd_parms.gca_message_type = GCA_RELEASE;
    sd_parms.gca_buffer = msg_buff;
    sd_parms.gca_msg_length = ptr - msg_buff;
    sd_parms.gca_end_of_data = TRUE;

    call_status = IIGCa_call( GCA_SEND, (GCA_PARMLIST *)&sd_parms,
			      GCA_SYNC_FLAG, NULL, -1, &status );

    if ( call_status == OK  &&  sd_parms.gca_status != OK )
	status = sd_parms.gca_status, call_status = FAIL;

    return( call_status );
}

static STATUS
gca_request( char *target, i4 *assoc_id ,char * username, char * password)
{
    GCA_RQ_PARMS	rq_parms;
    STATUS		status;
    STATUS		call_status;

    MEfill( sizeof( rq_parms ), 0, (PTR)&rq_parms );
    rq_parms.gca_peer_protocol = GCA_PROTOCOL_LEVEL_62;
    rq_parms.gca_partner_name = target;

    if ( *username  &&  *password )
    {
	rq_parms.gca_modifiers |= GCA_RQ_REMPW;
	rq_parms.gca_rem_username = username;
	rq_parms.gca_rem_password = password;
    }

    call_status = IIGCa_call( GCA_REQUEST, (GCA_PARMLIST *)&rq_parms,
			      GCA_SYNC_FLAG, NULL, -1, &status );

    *assoc_id = rq_parms.gca_assoc_id;

    if ( call_status == OK  &&  rq_parms.gca_status != OK )
		status = rq_parms.gca_status, call_status = FAIL;

    if ( call_status == OK ) {
		if ( rq_parms.gca_peer_protocol < GCA_PROTOCOL_LEVEL_50 ) {
			gca_release( *assoc_id );
			call_status = FAIL;
		}
	}

	return( call_status );
}


static STATUS
gca_receive( i4 assoc_id, i4 *msg_type, i4 *msg_len, char *msg_buff )
{
    GCA_RV_PARMS	rv_parms;
    STATUS		status;
    STATUS		call_status;

    MEfill( sizeof( rv_parms ), 0, (PTR)&rv_parms );
    rv_parms.gca_assoc_id = assoc_id;
    rv_parms.gca_b_length = *msg_len;
    rv_parms.gca_buffer = msg_buff;

    call_status = IIGCa_call( GCA_RECEIVE, (GCA_PARMLIST *)&rv_parms,
			      GCA_SYNC_FLAG, NULL, -1, &status );

    if ( call_status == OK  &&  rv_parms.gca_status != OK )
	status = rv_parms.gca_status, call_status = FAIL;

    if ( call_status != OK ) {
		// the value of call_status will be returned
	}
    else  if ( ! rv_parms.gca_end_of_data )
    {
		call_status = FAIL;
    }
    else
    {
	*msg_type = rv_parms.gca_message_type;
	*msg_len = rv_parms.gca_d_length;
    }

    return( call_status );
}

static STATUS
gca_send( i4 assoc_id, i4 msg_type, i4 msg_len, char *msg_buff )
{
    GCA_SD_PARMS	sd_parms;
    STATUS		status;
    STATUS		call_status;

    MEfill( sizeof( sd_parms ), 0, (PTR)&sd_parms );
    sd_parms.gca_association_id = assoc_id;
    sd_parms.gca_message_type = msg_type;
    sd_parms.gca_msg_length = msg_len;
    sd_parms.gca_buffer = msg_buff;
    sd_parms.gca_end_of_data = TRUE;

    call_status = IIGCa_call( GCA_SEND, (GCA_PARMLIST *)&sd_parms,
			      GCA_SYNC_FLAG, NULL, -1, &status );

    if ( call_status == OK  &&  sd_parms.gca_status != OK )
		status = sd_parms.gca_status, call_status = FAIL;

    if ( call_status != OK ) {
		// the value of call_status will be returned
	}

    return( call_status );
}


static STATUS
gca_disassoc( i4 assoc_id )
{
    GCA_DA_PARMS	da_parms;
    STATUS		status;
    STATUS		call_status;

    MEfill( sizeof( da_parms ), 0, (PTR)&da_parms );
    da_parms.gca_association_id = assoc_id;

    call_status = IIGCa_call( GCA_DISASSOC, (GCA_PARMLIST *)&da_parms,
			      GCA_SYNC_FLAG, NULL, -1, &status );

    if ( call_status == OK  &&  da_parms.gca_status != OK )
		status = da_parms.gca_status, call_status = FAIL;

    if ( call_status != OK ) {
		// the value of call_status will be returned
	}
	return( call_status );
}
static STATUS
svr_load( i4 assoc_id, char *pclass, BOOL * pbresult)
{
    STATUS	status;
    char	*ptr = msg_buff;
    i4		msg_type;
    i4		msg_len = msg_max;
	* pbresult = FALSE;

    /*
    ** Build and send GCN_NS_OPER request.
    */
    ptr += gcu_put_int( ptr, GCN_PUB_FLAG );
    ptr += gcu_put_int( ptr, GCN_RET );
    ptr += gcu_put_str( ptr, "" );
    ptr += gcu_put_int( ptr, 1 );
    ptr += gcu_put_str( ptr, pclass );
    ptr += gcu_put_str( ptr, "" );
    ptr += gcu_put_str( ptr, "" );
    ptr += gcu_put_str( ptr, "" );

    status = gca_send( assoc_id, GCN_NS_OPER, ptr - msg_buff, msg_buff );

    /*
    ** Receive response.
    */
    if ( status == OK )
	status = gca_receive( assoc_id, &msg_type, &msg_len, msg_buff );

    if ( status == OK )
	switch( msg_type )
	{
	    case GCA_ERROR :
	    {
		i4	ele_cnt, prm_cnt, val;

		ptr = msg_buff;
		ptr += gcu_get_int( ptr, &ele_cnt );

		while( ele_cnt-- )
		{
		    ptr += gcu_get_int( ptr, &status );
		    ptr += gcu_get_int( ptr, &val ); /* skip id_server */
		    ptr += gcu_get_int( ptr, &val ); /* skip server_type */
		    ptr += gcu_get_int( ptr, &val ); /* skip severity */
		    ptr += gcu_get_int( ptr, &val ); /* skip local_error */
		    ptr += gcu_get_int( ptr, &prm_cnt );

		    while( prm_cnt-- )
		    {
			ptr += gcu_get_int( ptr, &val );
			ptr += gcu_get_int( ptr, &val );
			ptr += gcu_get_int( ptr, &val );
			//SIprintf("\nERROR 0x%x: %.*s\n", status, val, ptr);
			ptr += val;
		    }
		}

		status = OK;
	    }
	    break;
		
	    case GCN_RESULT :
	    {
		char		*val;
		i4		count;

		ptr = msg_buff;
		ptr += gcu_get_int( ptr, &count );	/* skip gcn_op */
		ptr += gcu_get_int( ptr, &count );

		while( count-- > 0 )
		{

		    ptr += gcu_get_str( ptr, &val );
			if (!x_stricmp(val,pclass))
				* pbresult = TRUE;

//		    data->class = STalloc( val );
		    ptr += gcu_get_str( ptr, &val );	/* skip uid */
		    ptr += gcu_get_str( ptr, &val );	
//		    data->obj = STalloc( val );
		    ptr += gcu_get_str( ptr, &val );
//		    data->addr = STalloc( val );

		}
	    }
	    break;

	    default :
		 //SIprintf( "\nInvalid GCA message type: %d\n", msg_type );
		break;
	}

    return( status );
}

static LPUCHAR  PossibleLocalGW[]= {
	                              "INFORMIX",
	                              "ORACLE"  ,
#ifndef MAINWIN
								  "MSSQL"   ,
#endif
	                              "SYBASE"  ,
	                              "DB2UDB"  ,
	                              "OPINGDT" ,
                                  NULL,
};

static LPUCHAR  VMSPossibleGW[]= {
	                              "DB2"  ,
	                              "DCOM" ,
	                              "IDMS" ,
	                              "IMS"  ,
	                              "VANT"    ,
	                              "VSAM"    ,
								  "ADABAS",
	                              NULL};

static LPUCHAR  NonVMSPossibleGW[]= {
                                  "ALB"  ,
	                              "INFORMIX",
	                              "INGRES",
	                              "MSSQL"   ,
	                              "ODBC"    ,
	                              "ORACLE"  ,
	                              "RDB"     ,
	                              "RMS"     ,
	                              "SYBASE"  ,
	                              "DB2UDB"  ,
                                 NULL};

//static LPUCHAR IngresGW = "INGRES";

#define MAX_SERVER_CLASS 128
static UCHAR BufferServerClass[MAX_SERVER_CLASS + 1][36];
static LPUCHAR FoundGW[MAX_SERVER_CLASS + 1];
static char bufCmdParmsGW[80];

static LPUCHAR FoundCmdParmsGW[2] = {
										bufCmdParmsGW,
										NULL
};

LPUCHAR * GetGWlist(LPUCHAR host)
{
	BOOL b1 = GetContextDrivenServerClassName(bufCmdParmsGW);
	if (b1)
		return FoundCmdParmsGW;

	return Task_GetGWList(host);
}

static BOOL pingGW(LPUCHAR host,LPUCHAR lpGWname)
{
	char target[GCN_EXTNAME_MAX_LEN + 1]; 
	STATUS rtn;
	STpolycat( 3, host, ERx("::/"),lpGWname,target );
	rtn = gcn_testaddr( target, 0 , NULL );
	if (rtn==OK) 
		return TRUE;
	else
		return FALSE;
}


static int NodeLLFillConnectionList()
{ 
    char value[GCN_VAL_MAX_LEN+1];
    char name[GCN_VAL_MAX_LEN+1];
    STATUS ires;
    x_strcpy(value,",,");
    x_strcpy(name,"");     // the buffer may not be mandatory

    freeNodeConnectionData (lpllconnectdata);

    lpllconnectdata=(LPNODECONNECTDATA)ESL_AllocMem(sizeof(NODECONNECTDATA));
    if (!lpllconnectdata) 
       return RES_ERR;
    lpcurconnectdata = lpllconnectdata;
    ilConnectEntry=0;

    bpriv=TRUE;
    ires=nv_api_call(GCN_NET_FLAG, name, GCN_RET, value, GCN_NODE);
    if (ires!=OK)
      return RES_ERR;
    bpriv=FALSE;
    ires=nv_api_call(GCN_NET_FLAG|GCN_PUB_FLAG, name, GCN_RET, value, GCN_NODE);
    if (ires!=OK)
      return RES_ERR;
    return RES_SUCCESS;
}

static int NodeLLFillLoginList()
{
    char value[GCN_VAL_MAX_LEN+1];
    char vnode[GCN_VAL_MAX_LEN+1];
    STATUS ires;
    x_strcpy(value,",");
    x_strcpy(vnode,"");

    freeNodeLoginData(lplllogindata);

    lplllogindata=(LPNODELOGINDATA)ESL_AllocMem(sizeof(NODELOGINDATA));
    if (!lplllogindata) 
       return RES_ERR;
    lpcurlogindata = lplllogindata;

    bpriv=TRUE;
    ires= nv_api_call(GCN_NET_FLAG, vnode, GCN_RET, value, GCN_LOGIN);
    if (ires!=OK)
      return RES_ERR;
    bpriv=FALSE;
    ires= nv_api_call(GCN_NET_FLAG|GCN_PUB_FLAG, vnode, GCN_RET, value, GCN_LOGIN);
    if (ires!=OK)
      return RES_ERR;
    return RES_SUCCESS;
}
static int NodeLLFillAttributeList()
{
#ifdef INGRES25
    char value[GCN_VAL_MAX_LEN+1];
    char vnode[GCN_VAL_MAX_LEN+1];
    STATUS ires;
    x_strcpy(value,",");
    x_strcpy(vnode,"");

    freeNodeAttributeData(lpllattributedata);

    lpllattributedata=(LPNODEATTRIBUTEDATA)ESL_AllocMem(sizeof(NODEATTRIBUTEDATA));
    if (!lpllattributedata) 
       return RES_ERR;
    lpcurattributedata = lpllattributedata;

    bpriv=TRUE;
    ires= nv_api_call(GCN_NET_FLAG, vnode, GCN_RET, value, GCN_ATTR);
    if (ires!=OK)
      return RES_ERR;
    bpriv=FALSE;
    ires= nv_api_call(GCN_NET_FLAG|GCN_PUB_FLAG, vnode, GCN_RET, value, GCN_ATTR);
    if (ires!=OK)
      return RES_ERR;
#else
    freeNodeAttributeData(lpllattributedata);

    lpllattributedata=(LPNODEATTRIBUTEDATA)ESL_AllocMem(sizeof(NODEATTRIBUTEDATA));
    if (!lpllattributedata) 
       return RES_ERR;
    lpcurattributedata = lpllattributedata;
#endif
    return RES_SUCCESS;
}
int NodeLLFillNodeLists(void) 
{
//	UpdPossibleLocalDBMSList(); /* must be in main thread for an unknown reason related to PM functions ? */
	                            /* therefore not included in the animation */
	return Task_GetNodesList();
//	return 	NodeLLFillNodeListsLL();
}

int NodeLLFillNodeListsLL(void)
{
   int ires;
   BOOL blocalnode;
   LPUCHAR * pLocalServers;
   LPMONNODEDATA       pnodedata,pnodedata2;
   LPNODELOGINDATA     plogdata     ;
   LPNODECONNECTDATA   pconnectdata ;
   LPNODEATTRIBUTEDATA pattributedata;
   char LocalHostName[MAXOBJECTNAME];

   //GChostname (LocalHostName,MAXOBJECTNAME);
   GetLocalVnodeName (LocalHostName, MAXOBJECTNAME);

   ires = NodeLLFillConnectionList();
   if (ires !=RES_SUCCESS)
      return ires;
   ires = NodeLLFillLoginList();
   if (ires !=RES_SUCCESS)
      return ires;
   ires = NodeLLFillAttributeList();
   if (ires !=RES_SUCCESS)
      return ires;

    lpllnodedata=(LPMONNODEDATA)ESL_AllocMem(sizeof(MONNODEDATA));
    if (!lpllnodedata) 
       return RES_ERR;
    pnodedata=lpllnodedata;
    for (pconnectdata=lpllconnectdata;pconnectdata;pconnectdata=pconnectdata->pnext) {
       BOOL bExist=FALSE;
       for (pnodedata2=lpllnodedata;pnodedata2;pnodedata2=pnodedata2->pnext){
          if (!x_strcmp(pnodedata2->MonNodeDta.NodeName,
                      pconnectdata->NodeConnectionDta.NodeName)) {
             bExist=TRUE;
             break;
          }
       }
       if (!bExist) {
          pnodedata->MonNodeDta.ConnectDta = pconnectdata->NodeConnectionDta;
          pnodedata->MonNodeDta.inbConnectData = 1;
          pnodedata->MonNodeDta.inbLoginData   = 0;
		  pnodedata->MonNodeDta.LoginDta.Login[0]='\0';
          pnodedata->MonNodeDta.bIsLocal       = FALSE;
          fstrncpy(pnodedata->MonNodeDta.NodeName,
                   pconnectdata->NodeConnectionDta.NodeName,
                   sizeof(pnodedata->MonNodeDta.NodeName));
          pnodedata=GetNextMonNode(pnodedata);
          if (!pnodedata)
             return RES_ERR;
       }
       else {
          pnodedata2->MonNodeDta.inbConnectData++;
       }
    }
    for (plogdata=lplllogindata;plogdata;plogdata=plogdata->pnext) {
       BOOL bExist=FALSE;
       for (pnodedata2=lpllnodedata;pnodedata2;pnodedata2=pnodedata2->pnext){
          if (!x_strcmp(pnodedata2->MonNodeDta.NodeName,
                      plogdata->NodeLoginDta.NodeName)) {
             bExist=TRUE;
             break;
          }
       }
       if (!bExist) {
          pnodedata->MonNodeDta.LoginDta = plogdata->NodeLoginDta;
          pnodedata->MonNodeDta.inbLoginData = 1;
          pnodedata->MonNodeDta.bIsLocal     = FALSE;
          fstrncpy(pnodedata->MonNodeDta.NodeName,
                   plogdata->NodeLoginDta.NodeName,
                   sizeof(pnodedata->MonNodeDta.NodeName));
          pnodedata=GetNextMonNode(pnodedata);
          if (!pnodedata)
             return RES_ERR;
       }
       else {
          pnodedata2->MonNodeDta.LoginDta = plogdata->NodeLoginDta;
          pnodedata2->MonNodeDta.inbLoginData++;
       }
    }

    for (pattributedata=lpllattributedata;pattributedata;pattributedata=pattributedata->pnext) {
       BOOL bExist=FALSE;
       for (pnodedata2=lpllnodedata;pnodedata2;pnodedata2=pnodedata2->pnext){
          if (!x_strcmp(pnodedata2->MonNodeDta.NodeName,
                      pattributedata->NodeAttributeDta.NodeName)) {
             bExist=TRUE;
             break;
          }
       }
       if (!bExist) {
       //   pnodedata->MonNodeDta.LoginDta = plogdata->NodeLoginDta;
          pnodedata->MonNodeDta.inbLoginData = 0;
          pnodedata->MonNodeDta.bIsLocal     = FALSE;
          fstrncpy(pnodedata->MonNodeDta.NodeName,
                   pattributedata->NodeAttributeDta.NodeName,
                   sizeof(pnodedata->MonNodeDta.NodeName));
          pnodedata=GetNextMonNode(pnodedata);
          if (!pnodedata)
             return RES_ERR;
       }
    }

    // update misc flags for each node
    for (pnodedata2=lpllnodedata;pnodedata2;pnodedata2=pnodedata2->pnext){
       if ( /*(pnodedata2->MonNodeDta.ConnectDta.bPrivate ==
             pnodedata2->MonNodeDta.LoginDta.bPrivate) 
            && */ pnodedata2->MonNodeDta.inbConnectData == 1
            && pnodedata2->MonNodeDta.inbLoginData   == 1
          )
            pnodedata2->MonNodeDta.isSimplifiedModeOK=TRUE;
       else 
            pnodedata2->MonNodeDta.isSimplifiedModeOK=FALSE;
		pnodedata2->MonNodeDta.bWrongLocalName = FALSE;
		pnodedata2->MonNodeDta.bInstallPassword = FALSE;
		pnodedata2->MonNodeDta.bTooMuchInfoInInstallPassword = FALSE;

		if (!x_stricmp(pnodedata2->MonNodeDta.NodeName,LocalHostName)) {
			for (plogdata=lplllogindata;plogdata;plogdata=plogdata->pnext) {
				if (!x_stricmp(plogdata->NodeLoginDta.Login,""))
					continue;
				if (!x_strcmp(pnodedata2->MonNodeDta.NodeName,
						plogdata->NodeLoginDta.NodeName)) {
					if (!x_stricmp(plogdata->NodeLoginDta.Login,"*"))
						pnodedata2->MonNodeDta.bInstallPassword = TRUE;
					else 
						pnodedata2->MonNodeDta.bTooMuchInfoInInstallPassword = TRUE;
				}
			}
			for (pconnectdata=lpllconnectdata;pconnectdata;pconnectdata=pconnectdata->pnext) {
				if (!x_stricmp(pconnectdata->NodeConnectionDta.NodeName,""))
					continue;
				if (!x_strcmp(pnodedata2->MonNodeDta.NodeName,
					pconnectdata->NodeConnectionDta.NodeName)) {
					pnodedata2->MonNodeDta.bTooMuchInfoInInstallPassword = TRUE;
				}
			}
			for (pattributedata=lpllattributedata;pattributedata;pattributedata=pattributedata->pnext) {
				if (!x_strcmp(pattributedata->NodeAttributeDta.NodeName,""))
					continue; 
				if (!x_strcmp(pnodedata2->MonNodeDta.NodeName,
					pattributedata->NodeAttributeDta.NodeName)) {
					pnodedata2->MonNodeDta.bTooMuchInfoInInstallPassword = TRUE;
				}
			}
			if (!pnodedata2->MonNodeDta.bInstallPassword) {
				pnodedata2->MonNodeDta.bWrongLocalName = TRUE;
				pnodedata2->MonNodeDta.bTooMuchInfoInInstallPassword = FALSE;
			}
		}
	}

    // add "(local)" node if applies

	blocalnode = FALSE;
	pLocalServers = GetGWlistLL("",TRUE);
	if (pLocalServers) {
		while (*pLocalServers) {
				blocalnode = TRUE;
			pLocalServers++;
		}
	}

    if (blocalnode) {
       pnodedata->MonNodeDta.bIsLocal     = TRUE;
       if (!LoadString (hResource, (UINT)IDS_I_LOCALNODE,
                              pnodedata->MonNodeDta.NodeName,
                              sizeof (pnodedata->MonNodeDta.NodeName)))
           x_strcpy (pnodedata->MonNodeDta.NodeName, "(local)");
    }

   // sort the linked list 
   {
      LPMONNODEDATA     pn0,pn1,pn2;
      BOOL bDone=FALSE;
      while (!bDone) {
         pn0 = lpllnodedata;
         if (!pn0)
            break;
         pn1=pn0->pnext;
         if (!pn1)
            break;
         pn2=pn1->pnext;
         if (CompareMonInfo(OT_NODE, &(pn0->MonNodeDta), &(pn1->MonNodeDta))>0){
            lpllnodedata       = pn1;
            lpllnodedata->pnext= pn0;
            pn0->pnext=pn2;
            continue;
         }
         if (!pn2)
            break;
         while (!bDone) {
            if (!pn2) {
               bDone=TRUE;
               break;
            }
            if (CompareMonInfo(OT_NODE, &(pn1->MonNodeDta), &(pn2->MonNodeDta))>0) {
               pn0->pnext=pn2;
               pn1->pnext=pn2->pnext;
               pn2->pnext=pn1;
               break;
            }
            pn0=pn1;
            pn1=pn0->pnext;
            if (!pn1) {
               bDone=TRUE;
               break;
            }
            pn2=pn1->pnext;
         }  
      }
   }
   // end of sort/
   return RES_SUCCESS;
}

// add/alter/drop section

static int LLManageConnect(LPNODECONNECTDATAMIN lpNCData,i4 gcnx)
{
    char value[GCN_VAL_MAX_LEN+1];
    u_i4 flag;
    STATUS stat;
    x_strcpy(value, lpNCData->Address);
    x_strcat(value, ",");
    x_strcat(value, lpNCData->Protocol);
    x_strcat(value, ",");
    x_strcat(value, lpNCData->Listen);

    flag = GCN_NET_FLAG;
    if (!lpNCData->bPrivate)
        flag |= GCN_PUB_FLAG;
//    if (is_setuid)
//        flag |= GCN_UID_FLAG;
//    if (mflag)

    flag |= GCN_MRG_FLAG;

    stat = nv_api_call(flag, lpNCData->NodeName, gcnx, value, GCN_NODE);
    if (stat==OK)
      return RES_SUCCESS;
    else
      return RES_ERR;
}

static int LLManageLogin(LPNODELOGINDATAMIN lpNLData, i4 gcnx)
{
    char encrp_pwd[128];
    char value[GCN_VAL_MAX_LEN+1];
    u_i4 flag;
    STATUS stat;

    if(lpNLData->Passwd[0] != EOS)
    	gcu_encode(lpNLData->Login,lpNLData->Passwd,encrp_pwd);
    else
    	encrp_pwd[0] = EOS;

    x_strcpy(value, lpNLData->Login);
    x_strcat(value, ",");
    if (gcnx!=GCN_DEL)
       x_strcat(value, encrp_pwd);

    flag = GCN_NET_FLAG;
    if (!lpNLData->bPrivate)
        flag |= GCN_PUB_FLAG;
//    if (is_setuid)
//        flag |= GCN_UID_FLAG;

    stat = nv_api_call(flag, lpNLData->NodeName, gcnx, value, GCN_LOGIN);
    if (stat==OK)
      return RES_SUCCESS;
    else
      return RES_ERR;
}

static int LLManageAttribute(LPNODEATTRIBUTEDATAMIN lpAttrData,i4 gcnx)
{
#ifdef INGRES25
    char value[GCN_VAL_MAX_LEN+1];
    u_i4 flag;
    STATUS stat;
    x_strcpy(value, lpAttrData->AttributeName);
    x_strcat(value, ",");
    x_strcat(value, lpAttrData->AttributeValue);

    flag = GCN_NET_FLAG;
    if (!lpAttrData->bPrivate)
        flag |= GCN_PUB_FLAG;
//    if (is_setuid)
//        flag |= GCN_UID_FLAG;
//    if (mflag)

    flag |= GCN_MRG_FLAG;

    stat = nv_api_call(flag, lpAttrData->NodeName, gcnx, value, GCN_ATTR);
    if (stat==OK)
      return RES_SUCCESS;
    else
      return RES_ERR;
#else
      return RES_ERR;
#endif
}

static int checkConnectData(LPNODECONNECTDATAMIN lp1)
{
   if (!lp1->NodeName[0] || !lp1->Address[0] ||
       !lp1->Protocol[0] || !lp1->Listen[0]    )
      return RES_ERR;
   return RES_SUCCESS;
}
static int checkLoginData(LPNODELOGINDATAMIN lp1)
{
   if (!lp1->NodeName[0] || !lp1->Login[0] || !lp1->Passwd[0])
      return RES_ERR;
   return RES_SUCCESS;
}

static int CleanCacheAndSession(LPUCHAR lpnodename)
{
   int hnode = OpenNodeStruct  (lpnodename);
   CloseNodeStruct(hnode,TRUE);
   FreeNodeStruct(hnode,TRUE);
   PostNodeClose(lpnodename);
   return RES_SUCCESS;
}

// FULL NODES ADD/ALTER/DROP

int LLAddFullNode(LPNODEDATAMIN lpNData)
{
    if (!x_stricmp(lpNData->LoginDta.Login,"*") && !x_strlen(lpNData->ConnectDta.Address)) {
	  if ( checkLoginData  (&(lpNData->LoginDta))  !=RES_SUCCESS   )
		  return RES_ERR;
	   if (LLAddNodeLoginData  (&(lpNData->LoginDta))  !=RES_SUCCESS   )
		  return RES_ERR;
	   return RES_SUCCESS;
	}
    
   if (checkConnectData(&(lpNData->ConnectDta))!=RES_SUCCESS ||
       checkLoginData  (&(lpNData->LoginDta))  !=RES_SUCCESS   )
      return RES_ERR;
   if (LLAddNodeConnectData(&(lpNData->ConnectDta))!=RES_SUCCESS ||
       LLAddNodeLoginData  (&(lpNData->LoginDta))  !=RES_SUCCESS   )
      return RES_ERR;
   return RES_SUCCESS;
}
int LLDropFullNode(LPNODEDATAMIN lpNData)
{
   int ires;
   NODELOGINDATAMIN      logindta;
   NODECONNECTDATAMIN    connectdta;
   NODEATTRIBUTEDATAMIN  attributedta;

   if (CleanCacheAndSession(lpNData->NodeName)!=RES_SUCCESS)
      return RES_ERR;

   // hnode unused for node definitions. 
   ires=GetFirstMonInfo(0,OT_NODE,lpNData,OT_NODE_LOGINPSW,&logindta,NULL);
   while (ires==RES_SUCCESS) {
     if (LLDropNodeLoginData(&logindta) !=RES_SUCCESS)
        return RES_ERR;
     ires=GetNextMonInfo(&logindta);
   }
   if (ires!=RES_ENDOFDATA)
      return RES_ERR;

   ires=GetFirstMonInfo(0,OT_NODE,lpNData,OT_NODE_ATTRIBUTE,&attributedta,NULL);
   while (ires==RES_SUCCESS) {
     if (LLDropNodeAttributeData(&attributedta) !=RES_SUCCESS)
        return RES_ERR;
     ires=GetNextMonInfo(&attributedta);
   }
   if (ires!=RES_ENDOFDATA)
      return RES_ERR;

   ires=GetFirstMonInfo(0,OT_NODE,lpNData,OT_NODE_CONNECTIONDATA,&connectdta,NULL);
   while (ires==RES_SUCCESS) {
     if (LLDropNodeConnectData(&connectdta) !=RES_SUCCESS)
        return RES_ERR;
     ires=GetNextMonInfo(&connectdta);
   }
   if (ires!=RES_ENDOFDATA)
      return RES_ERR;
   
   return RES_SUCCESS;

//   if (LLDropNodeConnectData(&(lpNData->ConnectDta))!=RES_SUCCESS ||
//       LLDropNodeLoginData  (&(lpNData->LoginDta))  !=RES_SUCCESS   )
//      return RES_ERR;
//   return RES_SUCCESS;
}
int LLAlterFullNode(LPNODEDATAMIN lpNDataold,LPNODEDATAMIN lpNDatanew)
{
   if (CleanCacheAndSession(lpNDataold->NodeName)!=RES_SUCCESS)
      return RES_ERR;
   if (checkConnectData(&(lpNDatanew->ConnectDta))!=RES_SUCCESS ||
       checkLoginData  (&(lpNDatanew->LoginDta))  !=RES_SUCCESS   )
      return RES_ERR;
   if (LLAlterNodeConnectData(&(lpNDataold->ConnectDta),&(lpNDatanew->ConnectDta))!=RES_SUCCESS ||
       LLAlterNodeLoginData  (&(lpNDataold->LoginDta)  ,&(lpNDatanew->LoginDta)  )!=RES_SUCCESS   )
      return RES_ERR;
   return RES_SUCCESS;
}

// CONNECT DATA ADD/ALTER/DROP
int LLAddNodeConnectData(LPNODECONNECTDATAMIN lpNCData)
{
   if (checkConnectData(lpNCData)!=RES_SUCCESS)
      return RES_ERR;
   if (CleanCacheAndSession(lpNCData->NodeName)!=RES_SUCCESS)
      return RES_ERR;
   return LLManageConnect(lpNCData,GCN_ADD);
}

int LLAlterNodeConnectData(LPNODECONNECTDATAMIN lpNCDataold,LPNODECONNECTDATAMIN lpNCDatanew)
{
   if (CleanCacheAndSession(lpNCDataold->NodeName)!=RES_SUCCESS)
      return RES_ERR;
   if (LLManageConnect(lpNCDataold,GCN_DEL)!=RES_SUCCESS)
      return RES_ERR;
   return LLManageConnect(lpNCDatanew,GCN_ADD);
}

int LLDropNodeConnectData(LPNODECONNECTDATAMIN lpNCData)
{
   if (CleanCacheAndSession(lpNCData->NodeName)!=RES_SUCCESS)
      return RES_ERR;
   return LLManageConnect(lpNCData,GCN_DEL);
}

// LOGIN  DATA ADD/ALTER/DROP
int LLAddNodeLoginData(LPNODELOGINDATAMIN lpNLData)
{
   if (CleanCacheAndSession(lpNLData->NodeName)!=RES_SUCCESS)
      return RES_ERR;
   if (checkLoginData(lpNLData)!=RES_SUCCESS)
      return RES_ERR;
   return LLManageLogin(lpNLData,GCN_ADD);
}

int LLAlterNodeLoginData(LPNODELOGINDATAMIN lpNLDataold,LPNODELOGINDATAMIN lpNLDatanew)
{
   if (CleanCacheAndSession(lpNLDataold->NodeName)!=RES_SUCCESS)
      return RES_ERR;
   if (LLManageLogin(lpNLDataold,GCN_DEL)!=RES_SUCCESS)
      return RES_ERR;
   return LLManageLogin(lpNLDatanew,GCN_ADD);
}
int LLDropNodeLoginData(LPNODELOGINDATAMIN lpNLData)
{
   if (CleanCacheAndSession(lpNLData->NodeName)!=RES_SUCCESS)
      return RES_ERR;
   return LLManageLogin(lpNLData,GCN_DEL);
}

// ATTRIBUTE DATA ADD/ALTER/DROP
int LLAddNodeAttributeData(LPNODEATTRIBUTEDATAMIN lpAttributeData)
{
   if (CleanCacheAndSession(lpAttributeData->NodeName)!=RES_SUCCESS)
      return RES_ERR;
   return LLManageAttribute(lpAttributeData,GCN_ADD);
}

int LLAlterNodeAttributeData(LPNODEATTRIBUTEDATAMIN lpAttributeDataold,LPNODEATTRIBUTEDATAMIN lpAttributeDatanew)
{
   if (CleanCacheAndSession(lpAttributeDataold->NodeName)!=RES_SUCCESS)
      return RES_ERR;
   if (LLManageAttribute(lpAttributeDataold,GCN_DEL)!=RES_SUCCESS)
      return RES_ERR;
   return LLManageAttribute(lpAttributeDatanew,GCN_ADD);
}

int LLDropNodeAttributeData(LPNODEATTRIBUTEDATAMIN lpAttributeData)
{
   if (CleanCacheAndSession(lpAttributeData->NodeName)!=RES_SUCCESS)
      return RES_ERR;
   return LLManageAttribute(lpAttributeData,GCN_DEL);
}

/*** the following functions come from netutil  ***/

/*
** Name: gcn_resperr() - decodes a GCA_ERROR from the name server.
** If error code(s) have been placed in the GCA_ER_DATA use the first
** error code as the lastOpStat value to indicate the status of the request.
**
*/

static VOID
gcn_resperr(char *buf, i4 len)
{
    GCA_ER_DATA	    *ptr = (GCA_ER_DATA*) buf;

    if ( ptr->gca_l_e_element > 0 )
        lastOpStat = ptr->gca_e_element[0].gca_id_error; 
}

/***************************************************************************/

/*
**  gcn_response -- handle a response from the name server.
**
**  This function is called by the name server to handle the response to
**  any function.  It parses out the text and calls gcn_respsh for each
**  line.
*/

i4 n_gcn_response(i4 msg_type, char *buf, i4 len)
{
        i4 row_count, op = 0;

        /* Handle the message responses from name server. */
        switch( msg_type )
        {
            case GCN_RESULT:
                /* Pull off the results from the operation */
                buf += gcu_get_int( buf, &op );
                if ( op == GCN_RET )
                {
	                buf += gcu_get_int( buf, &row_count );
                   while( row_count-- )
	                   buf += gcn_respsh( buf );
                }
                lastOpStat = OK;
                break;
 
            case GCA_ERROR:
                /* The name server has flunked the request. */ 
                gcn_resperr(buf, len);
                break;
 
            default:
                break;
        }
        return op;
}

/*
** Name: gcn_respsh() - decode one row of show command output.  Figures
** out what needs to be stashed away based on what sort of command is
** being processed, and calls the text-list handler to save away the
** relevant information.
**
** Returns the number of bytes of the buffer which were processed by this
** call.
*/

static i4
gcn_respsh(char *buf)
{
	char		*type, *uid, *obj, *val;
	char		*pv[5];
	char		*obuf = buf;

	/* get type, tuple parts */

	buf += gcu_get_str( buf, &type );
	buf += gcu_get_str( buf, &uid );
	buf += gcu_get_str( buf, &obj );
	buf += gcu_get_str( buf, &val );

	if ( !STbcompare( type, 0, GCN_NODE, 0, TRUE ) )
	{
	   gcu_words( val, (char *)NULL, pv, ',', 3 ) ;

      x_strcpy (lpcurconnectdata->NodeConnectionDta.NodeName, obj);
      x_strcpy (lpcurconnectdata->NodeConnectionDta.Address , pv[0]);
      x_strcpy (lpcurconnectdata->NodeConnectionDta.Protocol, pv[1]);
      x_strcpy (lpcurconnectdata->NodeConnectionDta.Listen  , pv[2]);
      lpcurconnectdata->NodeConnectionDta.bPrivate = bpriv;
      lpcurconnectdata->NodeConnectionDta.ino      = ilConnectEntry++;
      lpcurconnectdata=GetNextNodeConnection(lpcurconnectdata);
      if (!lpcurconnectdata)
         goto ne;
	} 
#ifdef INGRES25
	else if ( !STbcompare( type, 0, GCN_ATTR, 0, TRUE ) )
	{
	   _VOID_ gcu_words( val, (char *)NULL, pv, ',', 3 );
      x_strcpy(lpcurattributedata->NodeAttributeDta.NodeName,obj);
      x_strcpy(lpcurattributedata->NodeAttributeDta.AttributeName ,pv[0]);
      x_strcpy(lpcurattributedata->NodeAttributeDta.AttributeValue,pv[1]);
      lpcurattributedata->NodeAttributeDta.bPrivate=bpriv;
      lpcurattributedata=GetNextNodeAttribute(lpcurattributedata);
      if (!lpcurattributedata)
         goto ne;
	}
#endif
	else if ( !STbcompare( type, 0, GCN_LOGIN, 0, TRUE ) )
	{
	   _VOID_ gcu_words( val, (char *)NULL, pv, ',', 3 );
      x_strcpy(lpcurlogindata->NodeLoginDta.NodeName,obj);
      x_strcpy(lpcurlogindata->NodeLoginDta.Login   ,val);
      x_strcpy(lpcurlogindata->NodeLoginDta.Passwd  ,"");
      lpcurlogindata->NodeLoginDta.bPrivate=bpriv;
      lpcurlogindata=GetNextNodeLogin(lpcurlogindata);
      if (!lpcurlogindata)
         goto ne;
	}

	else if ( !STbcompare( type, 0, GCN_COMSVR, 0, TRUE ) )
	{
	}
ne:
	return buf - obuf;
}

/***************************************************************************/

/* Generic API interfaces */


/*  nv_api_call -- generic call to the name server interface.
**
**  Inputs:
**
**    flag    -- Flag parameter for GCN_CALL_PARMS.
**                 GCN_PUB_FLAG - for global operations
**                 GCN_UID_FLAG - if impersonating another user.
**    obj     -- the thing being operated on.
**    opcode  -- GCN_ADD, GCN_DEL, etc., specifying the operation to be done.
**    value   -- the value being changed to or added.
**    type    -- GCN_NODE or GCN_LOGIN depending on what we're operating on.
*/

static STATUS
nv_api_call(i4 flag, char *obj, i4 opcode, char *value, char *type)
{
    GCN_CALL_PARMS gcn_parm;
    STATUS status;

    gcn_parm.gcn_response_func = n_gcn_response;  /* Handle response locally. */

    gcn_parm.gcn_uid = username;
    gcn_parm.gcn_flag = flag;
    gcn_parm.gcn_obj = obj;
    gcn_parm.gcn_type = type;
    gcn_parm.gcn_value = value;

    NMgtAt(ERx("II_INSTALLATION"),&gcn_parm.gcn_install);

    gcn_parm.gcn_host = hostName;

    /* Assume GCA request will fail, response handling may update this value */
//    lastOpStat = E_ST0050_gca_error;

    status = IIGCn_call(opcode,&gcn_parm);

    if (status != OK)
        return status;
    else 
        return lastOpStat;
}

static char szIIPromptAll[] = "IIPROMPTALL";
static char szIIPrompt1[]   = "IIPROMPT1";

int GetPathLoc( char * LocArea, char ** BaseDir)
{
if ( x_strcmp (LocArea,"II_DATABASE") == 0 )   {
   NMgtAt( "II_DATABASE", BaseDir);
   } else
      if ( x_strcmp (LocArea,"II_CHECKPOINT") == 0 )   {
         NMgtAt( "II_CHECKPOINT", BaseDir );
      } else
         if (x_strcmp (LocArea,"II_JOURNAL") == 0 )   {
            NMgtAt( "II_JOURNAL", BaseDir);
         } else
            if ( x_strcmp (LocArea,"II_CHECKPOINT") == 0 )   {
               NMgtAt( "II_CHECKPOINT", BaseDir);
            } else
               if (x_strcmp (LocArea,"II_DUMP" ) == 0 )   {
                  NMgtAt( "II_DUMP", BaseDir);
               } else
                  if (x_strcmp (LocArea,"II_WORK" ) == 0 )   {
                     NMgtAt( "II_WORK", BaseDir);
                  } else
                     return FALSE;

   if ( *BaseDir == NULL || (*BaseDir)[0] == '\0') 
      return FALSE;
   return TRUE;
}

/*
** Ping name server to see if it's up.
*/
BOOL 
ping_iigcn( void )
{
	GCA_PARMLIST	parms;
	STATUS		status;
	CL_ERR_DESC	cl_err;
	i4		assoc_no;
	STATUS		tmp_stat;
	char		gcn_id[ GCN_VAL_MAX_LEN ];

	MEfill( sizeof(parms), 0, (PTR) &parms );

	(void) IIGCa_call( GCA_INITIATE, &parms, GCA_SYNC_FLAG, 0,
		-1L, &status);
			
	if( status != OK )
		return FALSE;
	if(( status = parms.gca_in_parms.gca_status ) != OK ){
//		IIGCa_call(   GCA_TERMINATE,
//                      &parms,
//                      (i4) GCA_SYNC_FLAG,    /* Synchronous */
//                      (PTR) 0,                /* async id */
//                      (i4) -1,           /* no timeout */
//                      &status);
		return( FALSE );
	}

	status = GCnsid( GC_FIND_NSID, gcn_id, GCN_VAL_MAX_LEN, &cl_err );

	if( status == OK )
	{
		/* make GCA_REQUEST call */
		MEfill( sizeof(parms), 0, (PTR) &parms);
		parms.gca_rq_parms.gca_partner_name = gcn_id;
		parms.gca_rq_parms.gca_modifiers = GCA_CS_BEDCHECK |
			GCA_NO_XLATE;
		(void) IIGCa_call( GCA_REQUEST, &parms, GCA_SYNC_FLAG, 
			0, GCN_RESTART_TIME, &status );
		if( status == OK )
			status = parms.gca_rq_parms.gca_status;

		/* make GCA_DISASSOC call */
		assoc_no = parms.gca_rq_parms.gca_assoc_id;
		MEfill( sizeof( parms ), 0, (PTR) &parms);
		parms.gca_da_parms.gca_association_id = assoc_no;
		(void) IIGCa_call( GCA_DISASSOC, &parms, GCA_SYNC_FLAG, 
			0, -1L, &tmp_stat );
		/* existing name servers answer OK, new ones NO_PEER */
		if( status == E_GC0000_OK || status == E_GC0032_NO_PEER ){
			IIGCa_call(   GCA_TERMINATE,
              &parms,
              (i4) GCA_SYNC_FLAG,    /* Synchronous */
              (PTR) 0,                /* async id */
              (i4) -1,           /* no timeout */
              &status);
			return( TRUE );
		}
	}
    IIGCa_call(   GCA_TERMINATE,
                  &parms,
                  (i4) GCA_SYNC_FLAG,    /* Synchronous */
                  (PTR) 0,                /* async id */
                  (i4) -1,           /* no timeout */
                  &status);
	return( FALSE );
}



/*
** Query MIB Objects  !!! Begin !!!
** ************************************************************************************************
*/
FUNC_EXTERN i4  gcm_put_str();
FUNC_EXTERN i4  gcm_put_int();
FUNC_EXTERN i4  gcm_get_str();
FUNC_EXTERN i4  gcm_get_int();
FUNC_EXTERN i4  gcm_get_long();

static char target[255];
static char response[400];
static i4   message_type = GCM_GET;
static char work_buff[4120];
static PTR  save_data_area;
static char obj_class[16][255];
static char obj_instance[16][255];
static char obj_value[16][255];
static i4   obj_perms[16];
static i4   element_count;
static i4   row_count = 1;
static i4   get_async();
static i4   parm_index = 0;
static i4   parm_req[16];
static GCA_PARMLIST    parms[16];

static i4
get_async(id)
i4 id;
{
	if (id)
		return id;
	if (++parm_index > 15)
		parm_index = 1;
	return parm_index;
}

LPLISTTCHAR ListTchar_Add  (LPLISTTCHAR oldList, LPCTSTR lpszString)
{
	LPTSTR lpszText;
	LPLISTTCHAR lpStruct, ls;

	if (!lpszString)
		return oldList;
	lpszText = (LPTSTR) malloc (_tcslen (lpszString) +1);
	lpStruct = (LPLISTTCHAR)malloc (sizeof (LISTTCHAR));
	if (!(lpszText && lpStruct))
	{
		//myerror (ERR_ALLOCMEM); 
		return oldList;
	}

	_tcscpy (lpszText, lpszString);
	lpStruct->next      = NULL;
	lpStruct->lptchItem = lpszText;
	if (!oldList)
	{
		return lpStruct;
	}

	ls = oldList;
	while (ls)
	{
		if (!ls->next)
		{
			ls->next = lpStruct; 
			break;
		}
		ls = ls->next;
	}
	return oldList;
}

LPLISTTCHAR ListTchar_Done (LPLISTTCHAR oldList)
{
	LPLISTTCHAR ls, lpTemp;
	ls = oldList;
	while (ls)
	{
		lpTemp = ls;
		ls = ls->next;
		free ((void*)(lpTemp->lptchItem));
		free ((void*)lpTemp);
	}
	return NULL;
}

LPMIBOBJECT MibObject_Add  (LPMIBOBJECT oldList, LPCTSTR lpszMibClass)
{
	LPMIBOBJECT lpStruct, ls;

	if (!lpszMibClass)
		return oldList;
	lpStruct = (LPMIBOBJECT)malloc (sizeof (MIBOBJECT));
	if (!lpStruct)
	{
		//myerror (ERR_ALLOCMEM); 
		return oldList;
	}

	_tcscpy (lpStruct->tchszMIBCHASS, lpszMibClass);
	lpStruct->next      = NULL;
	lpStruct->listValues= NULL;
	if (!oldList)
	{
		return lpStruct;
	}

	ls = oldList;
	while (ls)
	{
		if (!ls->next)
		{
			ls->next = lpStruct; 
			break;
		}
		ls = ls->next;
	}
	return oldList;
}

static LPMIBOBJECT MibObject_Search (LPMIBOBJECT oldList, LPCTSTR lpszMibClass)
{
	LPMIBOBJECT ls = oldList;
	while (ls)
	{
		if (_tcsicmp(ls->tchszMIBCHASS, lpszMibClass) == 0)
			return ls;
		ls = ls->next;
	}
	return NULL;
}

LPMIBOBJECT MibObject_Done (LPMIBOBJECT oldList)
{
	LPMIBOBJECT ls, lpTemp;
	ls = oldList;
	while (ls)
	{
		lpTemp = ls;
		ls = ls->next;
		lpTemp->listValues = ListTchar_Done (lpTemp->listValues);
		free ((void*)lpTemp);
	}
	return NULL;
}


static BOOL AddValue (LPMIBREQUEST pRequest, LPCTSTR lpszMibClass, LPCTSTR lpszMibValue)
{
	LPMIBOBJECT lpMib = MibObject_Search (pRequest->listMIB, lpszMibClass);
	if (lpMib)
	{
		lpMib->listValues = ListTchar_Add(lpMib->listValues, lpszMibValue);
		return TRUE;
	}

	return FALSE;
}

static void CheckEndSelect (LPMIBREQUEST pRequest, LPCTSTR lpszCurrentMibClass)
{
	LPMIBOBJECT lpMib = MibObject_Search (pRequest->listMIB, lpszCurrentMibClass);
	if (!lpMib)
	{
		pRequest->bEnd = TRUE;
	}
}



MIBREQUEST g_mibRequest;
static i4 gMib_message_type = GCM_GETNEXT;
static BOOL bMibFirstSelect = TRUE;
static STATUS gcm_mib_fastselect(i4 id, LPMIBREQUEST pRequest);
static i4 gcm_mib_display(PTR start, LPMIBREQUEST pRequest)
{
	i4 error_status;
	i4 error_index, i;
	char* r = start;

	r += gcm_get_long(r, &error_status);/* get error_status */
	r += gcm_get_int(r, &error_index);  /* get error_index */

	r += sizeof(i4); /* bump past future[0] */
	r += sizeof(i4); /* bump past future[1] */
	r += sizeof(i4); /* bump past client_perms */
	r += sizeof(i4); /* bump past row_count */

	r += gcm_get_int(r, &element_count); /* get element_count */

	STcopy("", obj_class[0]);
	STcopy("", obj_instance[0]);
	STcopy("", obj_value[0]);

	switch ( error_status )
	{
		case 0:
			break;
		/* MO_NO_NEXT should be ignored (b121675) */
		case E_GL400C_MO_NO_NEXT:
			error_status = 0;
			error_index = 0;
			break;
		default:
			pRequest->bError = TRUE;
			break;
	}

	for( i = 0; i < element_count; i++ )
	{
		char* c;
		i4 l;

		r += gcm_get_str(r, &c, &l); /* get classid */
		MEcopy(c, l, obj_class[0]);
		obj_class[0][l] = '\0';
		r += gcm_get_str(r, &c, &l);/* get instance */
		MEcopy(c, l, obj_instance[0]);
		obj_instance[0][l] = '\0';
		r += gcm_get_str(r, &c, &l);/* get value */
		MEcopy(c, l, obj_value[0]);
		obj_value[0][l] = '\0';
		r += gcm_get_int(r, &obj_perms[0]); /* get perms */

		AddValue (pRequest, obj_class[0], obj_value[0]);
		CheckEndSelect (pRequest, obj_class[0]);
		if (pRequest->bEnd)
			break;
	}
	return r - start;
}

static VOID gcm_mib_complete(i4 async_id)
{
	GCA_FS_PARMS* fs_parms;
	switch (parm_req[async_id])
	{
	case GCA_FASTSELECT:
		fs_parms = &parms[async_id].gca_fs_parms;

		if (fs_parms->gca_status == E_GCFFFE_INCOMPLETE)
		{
			gcm_mib_fastselect(async_id, &g_mibRequest);
			return;
		}

		if (fs_parms->gca_status != OK)
		{
			g_mibRequest.bError = TRUE; 
			if (fs_parms->gca_status != E_GC0032_NO_PEER)
			{
				GCshut();
			}
			return;
		}

		gcm_mib_display( save_data_area, &g_mibRequest); 
		GCshut();
		break;
	default:
		break;
	}

	parm_req[async_id] = 0;
	return;
}

static i4 gcm_mib_message( PTR start, LPMIBREQUEST pMibs )
{
	i4   i;
	char* q = start;
	int nCount = 0;
	LPMIBOBJECT ls = pMibs->listMIB;
	while (ls != NULL)
	{
		if (bMibFirstSelect)
		{
			STcopy(ls->tchszMIBCHASS, obj_class[nCount]);
			if (gMib_message_type == GCM_GET)
				STcopy("0" , obj_instance[nCount]);
			else
				STcopy("" , obj_instance[nCount]);
			STcopy("",  obj_value[nCount]);
		}

		nCount++;
		ls = ls->next;
	}
	element_count = nCount;
	q += sizeof(i4);               /* bump past error_status */
	q += sizeof(i4);               /* bump past error_index */
	q += sizeof(i4);               /* bump past future[0] */
	q += sizeof(i4);               /* bump past future[1] */
	q += gcm_put_int(q, -1);       /* set client_perms */
	q += gcm_put_int(q,  24);      /* set row_count */

	q += gcm_put_int(q, element_count); /* set element_count */

	for( i = 0; i < element_count; i++ )
	{
		q += gcm_put_str(q, obj_class[i]);      /* set input classid */
		q += gcm_put_str(q, obj_instance[i]);   /* set input instance */
		q += gcm_put_str(q, obj_value[i]);      /* set value */
		q += gcm_put_int(q, 0);                 /* set perms */
	}
	return q - start;
}

static STATUS gcm_mib_fastselect(i4 id, LPMIBREQUEST pRequest)
{
	GCA_FS_PARMS* fs_parms;
	STATUS        status;
	STATUS        call_status;
	i4            async_id;
	i4            resume = 0;

	async_id = get_async(id);
	if (id)
		resume = GCA_RESUME;

	parm_req[async_id] = GCA_FASTSELECT;
	fs_parms = &parms[async_id].gca_fs_parms;
	if ( ! id )
		MEfill( sizeof( *fs_parms ), 0, (PTR)fs_parms );
	fs_parms->gca_peer_protocol = GCA_PROTOCOL_LEVEL_62;
	fs_parms->gca_partner_name = target;
	fs_parms->gca_modifiers = GCA_RQ_GCM;
	fs_parms->gca_buffer = work_buff;
	fs_parms->gca_b_length = sizeof (work_buff);

	if (!resume)
	{
		fs_parms->gca_msg_length = gcm_mib_message( save_data_area, pRequest);
		fs_parms->gca_message_type = gMib_message_type; 
	}

	call_status = IIGCa_call(
		GCA_FASTSELECT,
		(GCA_PARMLIST *)fs_parms,
		GCA_ASYNC_FLAG | resume,
		(PTR) async_id,
		(i4) -1,
		&status);

	if (call_status != OK)
	{
		pRequest->bError = TRUE;
		return call_status;
	}

	return OK;
}


BOOL QueryMIBObjects(LPMIBREQUEST pRequest, LPCTSTR lpszTarget, int nMode)
{
	STATUS status, call_status;
	GCA_IN_PARMS in_parms;
	GCA_FO_PARMS fo_parms;
	GCA_TE_PARMS te_parms;

	if (nMode != GCM_GETNEXT && nMode != GCM_GET)
		nMode = GCM_GETNEXT;

	gMib_message_type = nMode;

	/*
	**Issue initialization call to ME
	*/
	if (!bMEAdviseInit) 
	{
		MEadvise( ME_INGRES_ALLOC );
		SIeqinit();
		bMEAdviseInit = TRUE;
	}
	/*
	** GCA initiate
	*/
	in_parms.gca_normal_completion_exit    = gcm_mib_complete;
	in_parms.gca_expedited_completion_exit = gcm_mib_complete;

	in_parms.gca_alloc_mgr   = NULL;
	in_parms.gca_dealloc_mgr = NULL;
	in_parms.gca_p_semaphore = NULL;
	in_parms.gca_v_semaphore = NULL;

	in_parms.gca_modifiers      = GCA_API_VERSION_SPECD;
	in_parms.gca_api_version    = GCA_API_LEVEL_4;
	in_parms.gca_local_protocol = GCA_PROTOCOL_LEVEL_62;

	call_status = IIGCa_call(
		GCA_INITIATE,
		(GCA_PARMLIST *)&in_parms,
		(i4) GCA_SYNC_FLAG,
		(PTR) 0,
		(i4) -1,
		&status);

	if (call_status != OK)
		return FALSE;
	/*
	** Invoke GCA_FORMAT
	*/
	fo_parms.gca_buffer   = work_buff;
	fo_parms.gca_b_length = sizeof (work_buff);

	call_status = IIGCa_call(
		GCA_FORMAT,
		(GCA_PARMLIST *)&fo_parms,
		(i4) GCA_SYNC_FLAG,
		(PTR) 0,
		(i4) -1,
		&status);

	if (call_status != OK)
		return FALSE;

	save_data_area = fo_parms.gca_data_area;
	STcopy(lpszTarget, target);
	/*
	** Invoke first asyncronous service 
	*/
	bMibFirstSelect = TRUE;
	while (!pRequest->bEnd && !pRequest->bError)
	{
		gcm_mib_fastselect(0, pRequest);
		GCexec();
		bMibFirstSelect = FALSE;
	}

	/*
	** Invoke GCA_TERMINATE
	*/
	call_status = IIGCa_call(
		GCA_TERMINATE,
		(GCA_PARMLIST *)&te_parms,
		(i4) GCA_SYNC_FLAG,
		(PTR) 0,
		(i4) -1,
		&status);

	if (call_status != OK)
		return FALSE;
	if (pRequest->bError)
		return FALSE;
	return TRUE;
}

/*
** Query MIB Objects  !!! End !!!
** ************************************************************************************************
*/
static BOOL CheckProtocolLevel_50 (LPCTSTR lpszNode, BOOL* pbProtocol50)
{
	STATUS status;
	STATUS call_status;
	GCA_IN_PARMS in_parms;
	GCA_TE_PARMS te_parms;
	char target[128];
	i4   assoc_id;
	BOOL bOK = TRUE;
	if (pbProtocol50)
		*pbProtocol50 = FALSE;
	/*
	** Initialize communications.
	*/
	MEfill(sizeof(in_parms), (u_char)0, (PTR)&in_parms);
	in_parms.gca_modifiers = GCA_API_VERSION_SPECD;
	in_parms.gca_api_version = GCA_API_LEVEL_5;
	in_parms.gca_local_protocol = GCA_PROTOCOL_LEVEL_62;

	call_status = IIGCa_call( GCA_INITIATE, (GCA_PARMLIST *)&in_parms, GCA_SYNC_FLAG, NULL, -1, &status );
	if ( call_status == OK  &&  in_parms.gca_status != OK )
		status = in_parms.gca_status, call_status = FAIL;

	if ( call_status != OK ) 
		bOK = FALSE;

	if (bOK)
	{
		msg_max = GCA_FS_MAX_DATA + in_parms.gca_header_length;
		if ( ! (msg_buff = (char *) ESL_AllocMem(msg_max))) 
		{
			call_status = IIGCa_call( GCA_TERMINATE, (GCA_PARMLIST *)&te_parms, GCA_SYNC_FLAG, NULL, -1, &status );
			return FALSE;
		}

		/*
		** Build target specification for Name Server:
		** @::/@addr for local, @host::/@addr for remote.
		*/
		if (lpszNode && *lpszNode )
			STprintf( target, "%s::/iinmsvr", lpszNode );
		else
			STprintf( target, "/iinmsvr" );

		if ( (status = gca_request( target, &assoc_id ,"","")) == OK ) 
		{
			if (pbProtocol50)
				*pbProtocol50 = TRUE;
			gca_release( assoc_id );
		}
		gca_disassoc( assoc_id );

		call_status = IIGCa_call( GCA_TERMINATE, (GCA_PARMLIST *)&te_parms, GCA_SYNC_FLAG, NULL, -1, &status );
		if ( call_status == OK  &&  te_parms.gca_status != OK )
			status = te_parms.gca_status, call_status = FAIL;

		ESL_FreeMem(msg_buff);
		if ( call_status != OK ) 
			bOK = FALSE;
	}
	return bOK;
}

LPLISTTCHAR GetGWlistLLNew(LPCTSTR lpszNode,  BOOL bDetectLocalServersOnly)
{
	char szLocal[MAXOBJECTNAME];  
	LPLISTTCHAR ppresult = NULL;
	LPUCHAR * pp1;
	BOOL bResult, bHasError;
	BOOL bGCA_PROTOCOL_LEVEL_50 ;
#ifdef	EDBC
	i4 ret;
#endif
	bHasError = FALSE;
	strcpy(szLocal, "(local)");
	if (bDetectLocalServersOnly || !stricmp(szLocal, lpszNode)) 
		lpszNode="";

	NodeLLInit();

	if (!CheckProtocolLevel_50 (lpszNode, &bGCA_PROTOCOL_LEVEL_50))
	{
		NodeLLTerminate();
		return ppresult;
	}
	if (bGCA_PROTOCOL_LEVEL_50)
	{
#ifdef EDBC
		ret = LLManageSaveLogin(lpszNode, TRUE);
#endif
		ppresult = GetListServerClass (lpszNode);
#ifdef EDBC
		ret = LLManageSaveLogin(lpszNode, FALSE);
#endif
	}
	else
	{
		// can be MVS or old ingres installs
		// use older code
		BOOL bVMSGwFound = FALSE;
		if (stricmp(lpszNode,""))  {
			if (NodeCheckConnection((LPUCHAR)lpszNode)!=OK)
				return ppresult;
		}
#ifdef	EDBC
		ret = LLManageSaveLogin(lpszNode, TRUE);
#endif
		// try VMS gateways
		for (pp1=VMSPossibleGW;*pp1 && !bDetectLocalServersOnly;pp1++) {
			bResult = pingGW((LPUCHAR)lpszNode,*pp1);
			if (bResult) {
				ppresult = ListTchar_Add (ppresult, (LPCTSTR)*pp1);
				bVMSGwFound = TRUE;
			}
		}
		if (!bVMSGwFound) {  // try non_VMS and non INGRES gateways
			if (!bDetectLocalServersOnly)
			{
				for (pp1=NonVMSPossibleGW;*pp1;pp1++) {
					bResult = pingGW((LPUCHAR)lpszNode,*pp1);
					if (bResult) 
						ppresult = ListTchar_Add (ppresult, (LPCTSTR)*pp1);
				}
			}
			else
			{
				assert(FALSE); // should not be happened here programming error ?
				bHasError = TRUE;
			}
		}
#ifdef	EDBC
		ret = LLManageSaveLogin(lpszNode, FALSE);
#endif
	}
	NodeLLTerminate();

	if (bHasError)
		return NULL;
	return ppresult;
}

BOOL IsIceServerLaunched(LPCTSTR lpszNode)
{
	TCHAR tchszTarget[128];
	BOOL bFound = FALSE;
	BOOL bOK = FALSE;
	int i = 0;
	TCHAR tchIceSvr[10] = _T("ICESVR");
	memset (&g_mibRequest, 0, sizeof(g_mibRequest));
	tchszTarget[0] = _T('\0');
	g_mibRequest.listMIB = MibObject_Add (NULL, _T("exp.gcf.gcn.server.class"));
	if (!lpszNode || (lpszNode && ( _tcslen(lpszNode) == 0 || _tcsicmp (lpszNode, _T("(local)")) == 0)))
		_tcscpy(tchszTarget, _T("/iinmsvr"));
	else
	{
		_stprintf(tchszTarget, _T("%s::/iinmsvr"), lpszNode);
	}

	bOK = QueryMIBObjects(&g_mibRequest, tchszTarget, GCM_GETNEXT);
	if (bOK)
	{
		LPMIBOBJECT pMibSvrclass = MibObject_Search (g_mibRequest.listMIB, _T("exp.gcf.gcn.server.class"));
		if (pMibSvrclass)
		{
			LPLISTTCHAR ls = pMibSvrclass->listValues;
			while (ls)
			{
				if (_tcsicmp (ls->lptchItem, tchIceSvr) == 0)
				{
					bFound = TRUE;
					break;
				}
				ls = ls->next;
			}
		}
	}
	g_mibRequest.listMIB = MibObject_Done(g_mibRequest.listMIB);
	return bFound;
}

#define NOT_SVRM  9
LPLISTTCHAR GetListServerClass(LPCTSTR lpszNode)
{
	TCHAR tchszTarget[128];
	LPLISTTCHAR listSvr = NULL;
	BOOL bOK = FALSE;
	int i = 0;
	TCHAR tchNonSvr[NOT_SVRM][16]=
	{
		_T("IINMSVR"),
		_T("NMSVR"),
		_T("IUSVR"),
		_T("ICESVR"),
		_T("COMSVR"),
		_T("JDBC"),
		_T("DASVR"),
		_T("STAR"),
		_T("RMCMD")
	};
	memset (&g_mibRequest, 0, sizeof(g_mibRequest));
	tchszTarget[0] = _T('\0');
	g_mibRequest.listMIB = MibObject_Add (NULL, _T("exp.gcf.gcn.server.class"));
	if (!lpszNode || (lpszNode && ( _tcslen(lpszNode) == 0 || _tcsicmp (lpszNode, _T("(local)")) == 0)))
		_tcscpy(tchszTarget, _T("/iinmsvr"));
	else
	{
		_stprintf(tchszTarget, _T("%s::/iinmsvr"), lpszNode);
	}

	bOK = QueryMIBObjects(&g_mibRequest, tchszTarget, GCM_GETNEXT);
	if (bOK)
	{
		LPMIBOBJECT pMibSvrclass = MibObject_Search (g_mibRequest.listMIB, _T("exp.gcf.gcn.server.class"));
		if (pMibSvrclass)
		{
			LPLISTTCHAR ls = pMibSvrclass->listValues;
			while (ls)
			{
				BOOL bFound = FALSE;
				for (i=0; i<NOT_SVRM; i++)
				{
					if (_tcsicmp (ls->lptchItem, tchNonSvr[i]) == 0)
					{
						bFound = TRUE;
						break;
					}
				}
				if (!bFound)
					listSvr = ListTchar_Add  (listSvr, ls->lptchItem);
				ls = ls->next;
			}
		}
	}
	g_mibRequest.listMIB = MibObject_Done(g_mibRequest.listMIB);
	return listSvr;
}

LPUCHAR* GetGWlistLL(LPUCHAR host, BOOL bDetectLocalServersOnly)
{
	LPLISTTCHAR ls = GetGWlistLLNew((LPCTSTR)host,  bDetectLocalServersOnly);
	LPUCHAR* pRest = FoundGW;
	LPLISTTCHAR l = ls;
	int count = 0;
	while (l && count < MAX_SERVER_CLASS)
	{
		_tcscpy (BufferServerClass[count], l->lptchItem);
		pRest[count] = BufferServerClass[count];

		l = l->next;
		count++;
	}
	pRest[count] = NULL;
	ListTchar_Done (ls);

	return pRest;
}


STATUS INGRESII_ERLookup(int msgid, char** buf, int buf_len)
{
	STATUS ret_status;
	char* tmp_buf = *buf;

	i4 len;
	CL_ERR_DESC     er_clerr;

	*tmp_buf='\0';
	ret_status = ERlookup(msgid, NULL, 0, NULL, tmp_buf, buf_len, 1, &len, &er_clerr, 0, NULL);

	return ret_status;
}

