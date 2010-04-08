/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
**
**    Project  : Ingres Visual DBA
**
**    Source   : nodes.c
**
**    low level for list of nodes
**
** History:
**
** 10-Jul-2001 (hanje04)
**    Temporary fix for use of UNIXProcessRunning. Will always return true!
**    until it is replaced with proper GCA calls .
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 16-Apr-2002 (noifr01)
**    (sir 107587) Add DB2UDB as a new possible server class
** 02-May-2002 (noifr01)
**    (sir 106648) VDBA split project. Cleaned up functions for detecting
**    whether the installation is started, for usage by calling apps (vdbamon,
**    vdbasql, iea, iia, etc....)(rather than providing a low-level access
**    error)
** 03-Mar-2003 (noifr01)
**    (sir 109718) updated from changes done in the same file in the ivm
**    directory
** 16-May-2003 (uk$so01)
**    SIR #110269 Add Net Trafic functionality. The code lines come from the
**    gcmtool.c
** 18-Sep-2003 (uk$so01)
**    SIR #106648, Integrate the libingll.lib and cleanup codes.
** 03-Oct-2003 (schph01)
**    SIR #109864 Add functionality to manage the command line
**    option "-vnode" for ingnet utility
** 14-Nov-2003 (uk$so01)
**    BUG #110983, If GCA Protocol Level = 50 then use MIB Objects 
**    to query the server classes.
** 19-Dec-2003 (uk$so01)
**    SIR #111475, Coorperative shutdown between the DBMS Client & Server.
** 17-Feb-2004 (schph01)
**    (bug 99242)  cleanup for DBCS compliance
** 09-Mar-2004 (uk$so01)
**    SIR #110013 (Handle the DAS Server.-> exclude it from server class)
** 24-Jun-2009 (smeke01) b121675
**    Ignore innocuous MO_NO_NEXT returned from GCA_FASTSELECT for 
**    non-privileged user. Also removed unused code involving err_fatal.
** 09-Mar-2010 (drivi01) SIR 123397
**    Update NodeCheckConnection to return STATUS code instead of BOOL.
**    The error code will be used to get the error from ERlookup.
**    Add new function INGRESII_ERLookup to retrieve an error message.
*/


#define INGRES25
#ifndef MAINWIN
#ifdef WIN32
#define DESKTOP
#define NT_GENERIC
#define IMPORT_DLL_DATA
#define INGRESII
#endif
#endif

#include "nodes.h"
#include "mibobjs.h"
#ifdef MAINWIN
#include <netdb.h>
#endif

#include   <malloc.h>
#include   <assert.h>

// ingres Compat Library  headers
#include    <compat.h>
#include    <me.h>
#include    <si.h>
#include    <cm.h>
#include    <st.h>
#include    <er.h>
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

#include <tchar.h>
#include <pc.h>
#include <tr.h>
#include <gcm.h>

/* Included for #define of E_GL400C_MO_NO_NEXT */
#include    <erglf.h>

static i4	lastOpStat;
static char *hostName   = NULL; // Filled only when ingnet is launched with '-vnode=' option
static char *localvnode = NULL; // Used to add the installation password when
                                // ingnet was launch with '-vnode=' option

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

#if defined (MAINWIN)
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

static BOOL g_bOwnMutexGCA_Initiate_Terminate = FALSE;
static HANDLE g_MutexGCA_Initiate_Terminate = NULL;
void ReleaseMutexGCA_Initiate_Terminate()
{
	if (g_MutexGCA_Initiate_Terminate && g_bOwnMutexGCA_Initiate_Terminate)
	{
		ReleaseMutex(g_MutexGCA_Initiate_Terminate);
		g_bOwnMutexGCA_Initiate_Terminate = FALSE;
	}
}

FUNC_EXTERN STATUS MtIIGCa_call( i4 a1, GCA_PARMLIST* p, i4 a2, PTR ptr, i4 a3, STATUS* s)
{
	STATUS st;
	switch (a1)
	{
	case GCA_INITIATE:
		if (!g_MutexGCA_Initiate_Terminate)
		{
			g_MutexGCA_Initiate_Terminate = CreateMutex (NULL, TRUE, NULL);
			if (g_MutexGCA_Initiate_Terminate)
			{
				g_bOwnMutexGCA_Initiate_Terminate = TRUE;
				st = IIGCa_call(a1, p, a2, ptr, a3, s);
				if (st != OK)
					ReleaseMutexGCA_Initiate_Terminate();
				return st;
			}
			else
				return FAIL;
		}
		else
		{
			//
			// I suppose normally 10 mins to execute the GCA is enough, so
			// if timeout at 10 minutes I just return an error by supposing the 
			// the program hangs up !!!
			DWORD dwWait = WaitForSingleObject (g_MutexGCA_Initiate_Terminate, 10*60*1000);
			if (dwWait == WAIT_OBJECT_0)
			{
				g_bOwnMutexGCA_Initiate_Terminate = TRUE;
				st = IIGCa_call(a1, p, a2, ptr, a3, s);
				if (st != OK)
					ReleaseMutexGCA_Initiate_Terminate();
				return st;
			}
			else
				return FAIL;
		}
		break;
	case GCA_TERMINATE:
		st = IIGCa_call(a1, p, a2, ptr, a3, s);
		ReleaseMutexGCA_Initiate_Terminate();
		return st;
		break;
	default:
		return IIGCa_call(a1, p, a2, ptr, a3, s);
	}
}


static LPLISTTCHAR GetGWlistLL(LPCTSTR lpszNode,  BOOL bDetectLocalServersOnly, int* pError);
static BOOL CheckProtocolLevel_50 (LPCTSTR lpszNode, BOOL* pbProtocol50);
static LPVOID ESL_AllocMem(uisize) /* specified to fill allocated area with zeroes */
UINT uisize;
{
   LPVOID lpv;
   lpv = malloc(uisize);
   if (lpv)
      memset(lpv,'\0',uisize);
   return lpv;
}
static void   ESL_FreeMem (lpArea)
LPVOID lpArea;
{
   free(lpArea);
}

static UCHAR *fstrncpy(UCHAR *pu1,UCHAR *pu2, int n)
{	// don't use STlcopy (may split the last char)
	char * bufret = pu1;
	while ( n >= CMbytecnt(pu2) +1 ) { // copy char only if it fits including the trailing \0
		n -=CMbytecnt(pu2);
		if (*pu2 == EOS) 
			break;
		CMcpyinc(pu2,pu1);
	}
	*pu1=EOS;
	return bufret;

}

static int myerror(ierr)
int ierr;
{
   char buf[80];
   wsprintf(buf,"Internal error %d", ierr);
   MessageBox( GetFocus(), buf, NULL, MB_OK | MB_TASKMODAL);
   return RES_ERR;
}


static int CompareMonInfo1(int iobjecttype, void *pstruct1, void *pstruct2, BOOL bUpdSameObj)
{

  switch (iobjecttype) {
    case OBT_NODE:
      {
        LPNODEDATAMIN lpn1 = (LPNODEDATAMIN) pstruct1;
        LPNODEDATAMIN lpn2 = (LPNODEDATAMIN) pstruct2;
        if (lpn1->bIsLocal && lpn2->bIsLocal)
          return 0;
        if (lpn1->bIsLocal)
          return (-1);
        if (lpn2->bIsLocal)
          return (1);
        if (lpn1->bInstallPassword && lpn2->bInstallPassword)
          return 0;
        if (lpn1->bInstallPassword)
          return (-1);
        if (lpn2->bInstallPassword)
          return (1);
        return _tcsicmp(lpn1->NodeName,lpn2->NodeName);
      }
      break;
  }
  return 1;
}
static int CompareMonInfo(int iobjecttype, void *pstruct1, void *pstruct2)
{
   return CompareMonInfo1(iobjecttype, pstruct1, pstruct2, FALSE);
}

static BOOL freeNodeServerClassData(lpNodeServerClassData)
struct NodeServerClassData * lpNodeServerClassData;
{

   struct NodeServerClassData *ptemp;

   while(lpNodeServerClassData) {
      ptemp=lpNodeServerClassData->pnext;
      ESL_FreeMem(lpNodeServerClassData);  
      lpNodeServerClassData = ptemp;
   }
   return TRUE;
}

static BOOL freeNodeLoginData(lpNodeLoginData)
struct NodeLoginData * lpNodeLoginData;
{

   struct NodeLoginData *ptemp;

   while(lpNodeLoginData) {
      ptemp=lpNodeLoginData->pnext;
      ESL_FreeMem(lpNodeLoginData);  
      lpNodeLoginData = ptemp;
   }
   return TRUE;
}

static BOOL freeNodeConnectionData(lpNodeConnectionData)
struct NodeConnectionData * lpNodeConnectionData;
{

   struct NodeConnectionData *ptemp;

   while(lpNodeConnectionData) {
      ptemp=lpNodeConnectionData->pnext;
      ESL_FreeMem(lpNodeConnectionData);  
      lpNodeConnectionData = ptemp;
   }
   return TRUE;
}

static BOOL freeNodeAttributeData(lpNodeAttributeData)
struct NodeAttributeData * lpNodeAttributeData;
{

   struct NodeAttributeData *ptemp;

   while(lpNodeAttributeData) {
      ptemp=lpNodeAttributeData->pnext;
      ESL_FreeMem(lpNodeAttributeData);  
      lpNodeAttributeData = ptemp;
   }
   return TRUE;
}

static BOOL freeWindowData(lpWindowData)
struct WindowData * lpWindowData;
{

   struct WindowData *ptemp;

   while(lpWindowData) {
      ptemp=lpWindowData->pnext;
      ESL_FreeMem(lpWindowData);  
      lpWindowData = ptemp;
   }
   return TRUE;
}

static BOOL freeMonNodeData(lpMonNodeData)
struct MonNodeData * lpMonNodeData;
{

   struct MonNodeData *ptemp;

   while(lpMonNodeData) {
      ptemp=lpMonNodeData->pnext;
      freeNodeServerClassData(lpMonNodeData->lpServerClassData);
      freeWindowData(lpMonNodeData->lpWindowData);
      freeNodeLoginData(lpMonNodeData->lpNodeLoginData);
      freeNodeConnectionData(lpMonNodeData->lpNodeConnectionData);
	  freeNodeAttributeData (lpMonNodeData->lpNodeAttributeData);
      ESL_FreeMem(lpMonNodeData);  
      lpMonNodeData = ptemp;
   }
   return TRUE;
}

static struct NodeServerClassData * GetNextServerClass(pServerClassdata)
struct NodeServerClassData * pServerClassdata;
{
   if (!pServerClassdata) {
      myerror(ERR_LIST);
      return pServerClassdata;
   }
   if (! pServerClassdata->pnext) {
      pServerClassdata->pnext=ESL_AllocMem(sizeof(struct NodeServerClassData));
      if (!pServerClassdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pServerClassdata->pnext;
}

static struct NodeLoginData * GetNextNodeLogin(pNodeLogindata)
struct NodeLoginData * pNodeLogindata;
{
   if (!pNodeLogindata) {
      myerror(ERR_LIST);
      return pNodeLogindata;
   }
   if (! pNodeLogindata->pnext) {
      pNodeLogindata->pnext=ESL_AllocMem(sizeof(struct NodeLoginData));
      if (!pNodeLogindata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pNodeLogindata->pnext;
}

static struct NodeConnectionData * GetNextNodeConnection(pNodeConnectiondata)
struct NodeConnectionData * pNodeConnectiondata;
{
   if (!pNodeConnectiondata) {
      myerror(ERR_LIST);
      return pNodeConnectiondata;
   }
   if (! pNodeConnectiondata->pnext) {
      pNodeConnectiondata->pnext=ESL_AllocMem(sizeof(struct NodeConnectionData));
      if (!pNodeConnectiondata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pNodeConnectiondata->pnext;
}

static struct NodeAttributeData  * GetNextNodeAttribute(pNodeAttributeData)
struct NodeAttributeData  * pNodeAttributeData;
{
   if (!pNodeAttributeData) {
      myerror(ERR_LIST);
      return pNodeAttributeData;
   }
   if (! pNodeAttributeData->pnext) {
      pNodeAttributeData->pnext=ESL_AllocMem(sizeof(struct NodeAttributeData));
      if (!pNodeAttributeData->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pNodeAttributeData->pnext;
}

static struct MonNodeData * GetNextMonNode(pMonNodedata)
struct MonNodeData * pMonNodedata;
{
   if (!pMonNodedata) {
      myerror(ERR_LIST);
      return pMonNodedata;
   }
   if (! pMonNodedata->pnext) {
      pMonNodedata->pnext=ESL_AllocMem(sizeof(struct MonNodeData));
      if (!pMonNodedata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pMonNodedata->pnext;
}


static struct WindowData * GetNextNodeWindow(pWindowdata)
struct WindowData * pWindowdata;
{
   if (!pWindowdata) {
      myerror(ERR_LIST);
      return pWindowdata;
   }
   if (! pWindowdata->pnext) {
      pWindowdata->pnext=ESL_AllocMem(sizeof(struct WindowData));
      if (!pWindowdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pWindowdata->pnext;
}

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


static BOOL bMEAdviseInit = FALSE;

BOOL NodeLL_IsVnodeOptionDefined()
{
    if (hostName && *hostName != EOS)
        return TRUE;
    else
        return FALSE;
}

BOOL NodeLL_FillHostName(LPCTSTR stNodeName)
{
    int iLen;
    iLen = _tcslen(stNodeName);
    if (iLen<=0)
        return FALSE;
    if (!hostName) {
        if ( !(hostName = (char *) ESL_AllocMem(iLen*sizeof(TCHAR)+sizeof(TCHAR)))) {
            myerror(ERR_ALLOCMEM);
            return FALSE;
        }
    }
    _tcscpy(hostName, stNodeName);
    return TRUE;
}

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
//       if (iret != OK) {
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
    if ( hostName ) {
      ESL_FreeMem(hostName);
      hostName = NULL;
    }
    if ( localvnode ) {
      ESL_FreeMem(localvnode);
      localvnode = NULL;
    }
    return 1;
}

STATUS NodeCheckConnection(LPUCHAR host)
{

   char target[GCN_EXTNAME_MAX_LEN + 1]; 
   STATUS rtn;
#ifdef	EDBC
   int	ret;
#endif

#ifdef	EDBC
   STpolycat( 2, host, ERx("::iidbdb"), target );
#else
   STpolycat( 2, host, ERx("::/IINMSVR"), target );
#endif

   NodeLLInit();
#ifdef	EDBC
   ret = LLManageSaveLogin(host, TRUE);
#endif
   rtn = gcn_testaddr( target, 0 , NULL );
#ifdef	EDBC
   ret = LLManageSaveLogin(host, FALSE);
#endif
   NodeLLTerminate();
#ifdef EDBC
   return ret;
#endif
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
    sd_parms.gca_msg_length = (i4)(ptr - msg_buff);
    sd_parms.gca_end_of_data = TRUE;

    call_status = MtIIGCa_call( GCA_SEND, (GCA_PARMLIST *)&sd_parms,
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

    call_status = MtIIGCa_call( GCA_REQUEST, (GCA_PARMLIST *)&rq_parms,
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
gca_receive( i4 assoc_id, i4 *msg_type, i4 *msg_len, char *msgbuff )
{
    GCA_RV_PARMS	rv_parms;
    STATUS		status;
    STATUS		call_status;

    MEfill( sizeof( rv_parms ), 0, (PTR)&rv_parms );
    rv_parms.gca_assoc_id = assoc_id;
    rv_parms.gca_b_length = *msg_len;
    rv_parms.gca_buffer = msgbuff;

    call_status = MtIIGCa_call( GCA_RECEIVE, (GCA_PARMLIST *)&rv_parms,
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
gca_send( i4 assoc_id, i4 msg_type, i4 msg_len, char *msgbuff )
{
    GCA_SD_PARMS	sd_parms;
    STATUS		status;
    STATUS		call_status;

    MEfill( sizeof( sd_parms ), 0, (PTR)&sd_parms );
    sd_parms.gca_association_id = assoc_id;
    sd_parms.gca_message_type = msg_type;
    sd_parms.gca_msg_length = msg_len;
    sd_parms.gca_buffer = msgbuff;
    sd_parms.gca_end_of_data = TRUE;

    call_status = MtIIGCa_call( GCA_SEND, (GCA_PARMLIST *)&sd_parms,
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

    call_status = MtIIGCa_call( GCA_DISASSOC, (GCA_PARMLIST *)&da_parms,
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

    status = gca_send( assoc_id, GCN_NS_OPER, (i4)(ptr - msg_buff), msg_buff );

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
			if (!stricmp(val,pclass))
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

static BOOL bMute = FALSE;
void MuteLLRefresh() {bMute=TRUE;}
void UnMuteLLRefresh() {bMute=FALSE;}

static int NodeLLFillConnectionList()
{ 
    char value[GCN_VAL_MAX_LEN+1];
    char name[GCN_VAL_MAX_LEN+1];
    STATUS ires;
    strcpy(value,",,");
    strcpy(name,"");     // the buffer may not be mandatory

    if (bMute) {
        if (lpllconnectdata)
            return RES_SUCCESS;
        else
            return RES_ERR;
    }

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
    strcpy(value,",");
    strcpy(vnode,"");

    if (bMute) {
        if (lplllogindata)
            return RES_SUCCESS;
        else
            return RES_ERR;
    }

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
    strcpy(value,",");
    strcpy(vnode,"");

    if (bMute) {
        if (lpllattributedata)
            return RES_SUCCESS;
        else
            return RES_ERR;
    }

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

static int NodeLLFillNodeLists_1(BOOL bIncludeLocalIfAny)
{
   int ires;
   BOOL blocalnode;
   LPMONNODEDATA       pnodedata,pnodedata2;
   LPNODELOGINDATA     plogdata     ;
   LPNODECONNECTDATA   pconnectdata ;
   LPNODEATTRIBUTEDATA pattributedata;
   char LocalHostName[MAXOBJECTNAME];

   if ( NodeLL_IsVnodeOptionDefined() )
   {
      NETLOCALVNODENAME pLocNode;
      memset (&pLocNode, 0, sizeof(NETLOCALVNODENAME));

      if ( NetQueryLocalVnode( &pLocNode ) ) {
         _tcscpy(LocalHostName,pLocNode.tcLocalVnode);
      }
   }
   else
      GChostname (LocalHostName,MAXOBJECTNAME);
   ires = NodeLLFillConnectionList();
   if (ires !=RES_SUCCESS)
      return ires;
   ires = NodeLLFillLoginList();
   if (ires !=RES_SUCCESS)
      return ires;
   ires = NodeLLFillAttributeList();
   if (ires !=RES_SUCCESS)
      return ires;
	freeMonNodeData        (lpllnodedata);
    lpllnodedata=(LPMONNODEDATA)ESL_AllocMem(sizeof(MONNODEDATA));
    if (!lpllnodedata) 
       return RES_ERR;
    pnodedata=lpllnodedata;
    for (pconnectdata=lpllconnectdata;pconnectdata;pconnectdata=pconnectdata->pnext) {
       BOOL bExist=FALSE;
       for (pnodedata2=lpllnodedata;pnodedata2;pnodedata2=pnodedata2->pnext){
          if (!strcmp(pnodedata2->MonNodeDta.NodeName,
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
          if (!strcmp(pnodedata2->MonNodeDta.NodeName,
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
          if (!strcmp(pnodedata2->MonNodeDta.NodeName,
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

		if (!_tcsicmp(pnodedata2->MonNodeDta.NodeName,LocalHostName)) {
			for (plogdata=lplllogindata;plogdata;plogdata=plogdata->pnext) {
				if (!stricmp(plogdata->NodeLoginDta.Login,""))
					continue;
				if (!strcmp(pnodedata2->MonNodeDta.NodeName,
						plogdata->NodeLoginDta.NodeName)) {
					if (!stricmp(plogdata->NodeLoginDta.Login,"*"))
						pnodedata2->MonNodeDta.bInstallPassword = TRUE;
					else 
						pnodedata2->MonNodeDta.bTooMuchInfoInInstallPassword = TRUE;
				}
			}
			for (pconnectdata=lpllconnectdata;pconnectdata;pconnectdata=pconnectdata->pnext) {
				if (!stricmp(pconnectdata->NodeConnectionDta.NodeName,""))
					continue;
				if (!strcmp(pnodedata2->MonNodeDta.NodeName,
					pconnectdata->NodeConnectionDta.NodeName)) {
					pnodedata2->MonNodeDta.bTooMuchInfoInInstallPassword = TRUE;
				}
			}
			for (pattributedata=lpllattributedata;pattributedata;pattributedata=pattributedata->pnext) {
				if (!strcmp(pattributedata->NodeAttributeDta.NodeName,""))
					continue; 
				if (!strcmp(pnodedata2->MonNodeDta.NodeName,
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
	if (bIncludeLocalIfAny) {
		int nError = 0;
		LPLISTTCHAR pLocalServers = GetGWlistLL(_T(""), TRUE, &nError);
		if (pLocalServers) {
			blocalnode = TRUE;
			pLocalServers = ListTchar_Done (pLocalServers);
		}
		if (blocalnode) {
			pnodedata->MonNodeDta.bIsLocal     = TRUE;
			strcpy (pnodedata->MonNodeDta.NodeName, "(local)");
		}
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
         if (CompareMonInfo(OBT_NODE, &(pn0->MonNodeDta), &(pn1->MonNodeDta))>0){
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
            if (CompareMonInfo(OBT_NODE, &(pn1->MonNodeDta), &(pn2->MonNodeDta))>0) {
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

int NodeLLFillNodeLists(void)
{
	return NodeLLFillNodeLists_1(TRUE);
}
int NodeLLFillNodeLists_NoLocal(void)
{
	return NodeLLFillNodeLists_1(FALSE);
}

// add/alter/drop section

static int LLManageConnect(LPNODECONNECTDATAMIN lpNCData,i4 gcnx)
{
    char value[GCN_VAL_MAX_LEN+1];
    u_i4 flag;
    STATUS stat;
    strcpy(value, lpNCData->Address);
    strcat(value, ",");
    strcat(value, lpNCData->Protocol);
    strcat(value, ",");
    strcat(value, lpNCData->Listen);

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

#ifdef	EDBC
	/*
	** Do not save the user name and password if bSave is FALSE.
	*/
	if (lpNLData->bSave)
		strcpy(value, lpNLData->Login);
	else
		strcpy(value,"\0");
    strcat(value, ",");
    if (gcnx!=GCN_DEL)
		if (lpNLData->bSave)	
			strcat(value, encrp_pwd);
#else
	strcpy(value, lpNLData->Login);
    strcat(value, ",");
    if (gcnx!=GCN_DEL)
       strcat(value, encrp_pwd);
#endif

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

#ifdef	EDBC
/* SIR 104751
** Ideally, we should not write anything to the Name Server Database
** but overwriting the username and password during connection times.
** Since we are using gcn_testaddr() in this program, there is no
** way to do that.
**
** The following routine is a workaround because rearchitecting 
** GCA/GCA communication is beyond the scope of this SIR.
**
** The login information has to be saved in the name server in order for
** the normal operations (e.g. Test Server) to work. This function tries
** to manage that. If bSave is TRUE, it saves the login information
** to the name save database for the immediated operation. If bSave is 
** FALSE, it saves NULLs to the name server database.
*/
int LLManageSaveLogin(LPUCHAR ServerName, BOOL bSave)
{
    char encrp_pwd[128];
    char value[GCN_VAL_MAX_LEN+1];
    u_i4 flag;
    STATUS stat;
	LPNODELOGINDATA lpNLData;

	lpNLData = GetFirstLLLoginData();
	while (lpNLData)
	{
		if (lstrcmpi ((LPCTSTR)ServerName, (LPCTSTR)lpNLData->NodeLoginDta.NodeName) == 0)
		{
			break;
		}
		lpNLData = lpNLData->pnext;
	}

	if (!lpNLData)
		return RES_ERR;

	/*
	** Do not need to do anything if the login is saved.
	*/
	if (lpNLData->NodeLoginDta.bSave)
		return RES_SUCCESS;


    if(lpNLData->NodeLoginDta.Passwd[0] != EOS)
    	gcu_encode(lpNLData->NodeLoginDta.Login,lpNLData->NodeLoginDta.Passwd,encrp_pwd);
    else
    	encrp_pwd[0] = EOS;

	if (bSave)
	{
		/*
		** Delete the Vnode with the NULL login first.
		*/
		strcpy(value, "\0");
		strcat(value, ",");
		flag = GCN_NET_FLAG;
		if (!lpNLData->NodeLoginDta.bPrivate)
			flag |= GCN_PUB_FLAG;

		stat = nv_api_call(flag, lpNLData->NodeLoginDta.NodeName, GCN_DEL, value, GCN_LOGIN);
		if (stat!=OK)
			return RES_ERR;

		/*
		** Add the a vnode with entered login.
		*/
		strcpy(value, lpNLData->NodeLoginDta.Login);
		strcat(value, ",");
		strcat(value, encrp_pwd);
		flag = GCN_NET_FLAG;
		if (!lpNLData->NodeLoginDta.bPrivate)
			flag |= GCN_PUB_FLAG;

		stat = nv_api_call(flag, lpNLData->NodeLoginDta.NodeName, GCN_ADD, value, GCN_LOGIN);
		if (stat==OK)
			return RES_SUCCESS;
		else
			return RES_ERR;
	}
	else
	{
		/*
		** Delete the Vnode with the entered login first.
		*/
		strcpy(value, lpNLData->NodeLoginDta.Login);
		strcat(value, ",");
		flag = GCN_NET_FLAG;
		if (!lpNLData->NodeLoginDta.bPrivate)
			flag |= GCN_PUB_FLAG;

		stat = nv_api_call(flag, lpNLData->NodeLoginDta.NodeName, GCN_DEL, value, GCN_LOGIN);
		if (stat!=OK)
			return RES_ERR;

		/*
		** Add the a vnode with NULL login.
		*/
		strcpy(value, "\0");
		strcat(value, ",");
		flag = GCN_NET_FLAG;
		if (!lpNLData->NodeLoginDta.bPrivate)
			flag |= GCN_PUB_FLAG;

		stat = nv_api_call(flag, lpNLData->NodeLoginDta.NodeName, GCN_ADD, value, GCN_LOGIN);
		if (stat==OK)
			return RES_SUCCESS;
		else
			return RES_ERR;
	}


}





/*
** Return TRUE and put the connection string in the output
** parameter ConnStr in the following format:
**		@<host>,<protocol>,<port>[<user>,<pwd>]
** if the login and password are not saved, return FALSE 
** otherwise.
*/
BOOL EDBC_GetConnectionString(LPCTSTR ServerName, char * ConnStr)
{
	LPNODELOGINDATA lpNLData;
	LPNODECONNECTDATA lpNCData;

	lpNLData = GetFirstLLLoginData();
	while (lpNLData)
	{
		if (lstrcmpi (ServerName, lpNLData->NodeLoginDta.NodeName) == 0)
		{
			break;
		}
		lpNLData = lpNLData->pnext;
	}

	if (!lpNLData || lpNLData->NodeLoginDta.bSave)
		return FALSE;

	lpNCData = GetFirstLLConnectData();
	while (lpNCData)
	{
		if (lstrcmpi (ServerName, lpNCData->NodeConnectionDta.NodeName) == 0)
		{
			break;
		}
		lpNCData = lpNCData->pnext;
	}

	if (!lpNCData)
		return FALSE;

	/*
	** Put together the connection string for vnodeless connections.
	*/
	strcpy(ConnStr, "@");
	strcat(ConnStr, lpNCData->NodeConnectionDta.Address);
	strcat(ConnStr, ",");
	strcat(ConnStr, lpNCData->NodeConnectionDta.Protocol);
	strcat(ConnStr, ",");
	strcat(ConnStr, lpNCData->NodeConnectionDta.Listen);
	strcat(ConnStr, "[");
	strcat(ConnStr, lpNLData->NodeLoginDta.Login);
	strcat(ConnStr, ",");
	strcat(ConnStr, lpNLData->NodeLoginDta.Passwd);
	strcat(ConnStr, "]");

	return TRUE;
}
#endif


static int LLManageAttribute(LPNODEATTRIBUTEDATAMIN lpAttrData,i4 gcnx)
{
#ifdef INGRES25
    char value[GCN_VAL_MAX_LEN+1];
    u_i4 flag;
    STATUS stat;
    strcpy(value, lpAttrData->AttributeName);
    strcat(value, ",");
    strcat(value, lpAttrData->AttributeValue);

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
#ifdef	EDBC
   if (lp1->bSave)
   {
   if (!lp1->NodeName[0] || !lp1->Login[0] || !lp1->Passwd[0])
      return RES_ERR;
   return RES_SUCCESS;
   }
   else
   {
   if (!lp1->NodeName[0])
      return RES_ERR;
   return RES_SUCCESS;
   }
#else
   if (!lp1->NodeName[0] || !lp1->Login[0] || !lp1->Passwd[0])
      return RES_ERR;
   return RES_SUCCESS;
#endif
}


BOOL NodeExists(LPUCHAR NodeName)
{
	LPMONNODEDATA  p1= GetFirstLLNode();
	if (_tcslen(NodeName)==0) 
		return FALSE;
	while (p1) {
		if (!_tcsicmp(p1->MonNodeDta.NodeName,NodeName))
			return TRUE;
		p1=p1->pnext;
	}
	return FALSE;
}

// FULL NODES ADD/ALTER/DROP

int CompleteNodeStruct(LPNODEDATAMIN lpnodedata)
{
	LPMONNODEDATA  p1= GetFirstLLNode();
	while (p1) {
		if (!_tcsicmp(p1->MonNodeDta.NodeName,lpnodedata->NodeName)) {
			if (!p1->MonNodeDta.isSimplifiedModeOK)
				return RES_NOT_SIMPLIFIEDM0DE;
			*lpnodedata=p1->MonNodeDta;
			return RES_SUCCESS;
		}
		p1=p1->pnext;
	}
	return RES_ENDOFDATA;

}

int LLAddFullNode(LPNODEDATAMIN lpNData)
{
    if (!stricmp(lpNData->LoginDta.Login,"*") && !_tcslen(lpNData->ConnectDta.Address)) {
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

	 LPNODELOGINDATA     plogdatall ;
     LPNODECONNECTDATA   pconnectdatall ;
	 LPNODEATTRIBUTEDATA pattributedataall;

     LPUCHAR NodeName = lpNData->NodeName;

     for (plogdatall=GetFirstLLLoginData(); plogdatall;
          plogdatall=plogdatall->pnext) {
        if (!strcmp(plogdatall->NodeLoginDta.NodeName,NodeName)) {
			 if (LLDropNodeLoginData(& (plogdatall->NodeLoginDta)) !=RES_SUCCESS)
				return RES_ERR;
        }
     }

     for (pattributedataall=GetFirstLLAttributeData(); pattributedataall;
          pattributedataall=pattributedataall->pnext) {
        if (!strcmp(pattributedataall->NodeAttributeDta.NodeName,NodeName)) {
		    if (LLDropNodeAttributeData(&(pattributedataall->NodeAttributeDta)) !=RES_SUCCESS)
				return RES_ERR;
	    }
     }

     for (pconnectdatall=GetFirstLLConnectData(); pconnectdatall;
          pconnectdatall=pconnectdatall->pnext) {
        if (!strcmp(pconnectdatall->NodeConnectionDta.NodeName, NodeName)) {
			 if (LLDropNodeConnectData(&(pconnectdatall->NodeConnectionDta)) !=RES_SUCCESS)
				return RES_ERR;
        }
     }
	 return RES_SUCCESS;

}

int LLAlterFullNode(LPNODEDATAMIN lpNDataold,LPNODEDATAMIN lpNDatanew)
{
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
   return LLManageConnect(lpNCData,GCN_ADD);
}

int LLAlterNodeConnectData(LPNODECONNECTDATAMIN lpNCDataold,LPNODECONNECTDATAMIN lpNCDatanew)
{
   if (LLManageConnect(lpNCDataold,GCN_DEL)!=RES_SUCCESS)
      return RES_ERR;
   return LLManageConnect(lpNCDatanew,GCN_ADD);
}

int LLDropNodeConnectData(LPNODECONNECTDATAMIN lpNCData)
{
   return LLManageConnect(lpNCData,GCN_DEL);
}

// LOGIN  DATA ADD/ALTER/DROP
int LLAddNodeLoginData(LPNODELOGINDATAMIN lpNLData)
{
   if (checkLoginData(lpNLData)!=RES_SUCCESS)
      return RES_ERR;
   return LLManageLogin(lpNLData,GCN_ADD);
}

int LLAlterNodeLoginData(LPNODELOGINDATAMIN lpNLDataold,LPNODELOGINDATAMIN lpNLDatanew)
{
   if (LLManageLogin(lpNLDataold,GCN_DEL)!=RES_SUCCESS)
      return RES_ERR;
   return LLManageLogin(lpNLDatanew,GCN_ADD);
}
int LLDropNodeLoginData(LPNODELOGINDATAMIN lpNLData)
{
   return LLManageLogin(lpNLData,GCN_DEL);
}

// ATTRIBUTE DATA ADD/ALTER/DROP
int LLAddNodeAttributeData(LPNODEATTRIBUTEDATAMIN lpAttributeData)
{
   return LLManageAttribute(lpAttributeData,GCN_ADD);
}

int LLAlterNodeAttributeData(LPNODEATTRIBUTEDATAMIN lpAttributeDataold,LPNODEATTRIBUTEDATAMIN lpAttributeDatanew)
{
   if (LLManageAttribute(lpAttributeDataold,GCN_DEL)!=RES_SUCCESS)
      return RES_ERR;
   return LLManageAttribute(lpAttributeDatanew,GCN_ADD);
}

int LLDropNodeAttributeData(LPNODEATTRIBUTEDATAMIN lpAttributeData)
{
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

      strcpy (lpcurconnectdata->NodeConnectionDta.NodeName, obj);
      strcpy (lpcurconnectdata->NodeConnectionDta.Address , pv[0]);
      strcpy (lpcurconnectdata->NodeConnectionDta.Protocol, pv[1]);
      strcpy (lpcurconnectdata->NodeConnectionDta.Listen  , pv[2]);
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
      strcpy(lpcurattributedata->NodeAttributeDta.NodeName,obj);
      strcpy(lpcurattributedata->NodeAttributeDta.AttributeName ,pv[0]);
      strcpy(lpcurattributedata->NodeAttributeDta.AttributeValue,pv[1]);
      lpcurattributedata->NodeAttributeDta.bPrivate=bpriv;
      lpcurattributedata=GetNextNodeAttribute(lpcurattributedata);
      if (!lpcurattributedata)
         goto ne;
	}
#endif
	else if ( !STbcompare( type, 0, GCN_LOGIN, 0, TRUE ) )
	{
	   _VOID_ gcu_words( val, (char *)NULL, pv, ',', 3 );
      strcpy(lpcurlogindata->NodeLoginDta.NodeName,obj);
      strcpy(lpcurlogindata->NodeLoginDta.Login   ,val);
      strcpy(lpcurlogindata->NodeLoginDta.Passwd  ,"");
      lpcurlogindata->NodeLoginDta.bPrivate=bpriv;
#ifdef	EDBC
	  /*
	  ** Set up the bSave flag.
	  */
	  if (val[0] == '\0')
		lpcurlogindata->NodeLoginDta.bSave = FALSE;
	  else
		lpcurlogindata->NodeLoginDta.bSave = TRUE;
#endif
      lpcurlogindata=GetNextNodeLogin(lpcurlogindata);
      if (!lpcurlogindata)
         goto ne;
	}

	else if ( !STbcompare( type, 0, GCN_COMSVR, 0, TRUE ) )
	{
	}
ne:
	return (i4)(buf - obuf);
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


static char *GetMonInfoName(int iobjecttype, void *pstruct, char *buf,int bufsize)
{
  switch (iobjecttype) {
    case OBT_NODE:
      {
        LPNODEDATAMIN lpn = (LPNODEDATAMIN) pstruct;
		if (lpn->bInstallPassword) 
			wsprintf(buf,"<installation password node>[%s]",lpn->NodeName);
		else
			fstrncpy((LPUCHAR)buf,lpn->NodeName,bufsize);
      }
      break;
    case OBT_NODE_SERVERCLASS:
      {
        LPNODESVRCLASSDATAMIN lpn = (LPNODESVRCLASSDATAMIN) pstruct;
        fstrncpy((LPUCHAR)buf,lpn->ServerClass,bufsize);
      }
      break;
 
    case OBT_NODE_LOGINPSW:
      {
        LPNODELOGINDATAMIN lp = (LPNODELOGINDATAMIN) pstruct;
        fstrncpy((LPUCHAR)buf,(LPUCHAR)lp->Login,bufsize);
      }
      break;
    case OBT_NODE_CONNECTIONDATA:
      {
        LPNODECONNECTDATAMIN lp = (LPNODECONNECTDATAMIN) pstruct;
        fstrncpy((LPUCHAR)buf,(LPUCHAR)lp->Address,bufsize);
      }
      break;
    case OBT_NODE_ATTRIBUTE:
      {
        LPNODEATTRIBUTEDATAMIN lp = (LPNODEATTRIBUTEDATAMIN) pstruct;
		wsprintf(buf,"%s : %s" ,lp->AttributeName,lp->AttributeValue);
      }
      break;
    default:
      fstrncpy((LPUCHAR)buf,(LPUCHAR)pstruct,bufsize);
      break;
  }
  return buf;
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

	(void) MtIIGCa_call( GCA_INITIATE, &parms, GCA_SYNC_FLAG, 0,-1L, &status);
			
	if( status != OK )
		return FALSE;
	if(( status = parms.gca_in_parms.gca_status ) != OK ){
		ReleaseMutexGCA_Initiate_Terminate();
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
		(void) MtIIGCa_call( GCA_REQUEST, &parms, GCA_SYNC_FLAG, 
			0, GCN_RESTART_TIME, &status );
		if( status == OK )
			status = parms.gca_rq_parms.gca_status;

		/* make GCA_DISASSOC call */
		assoc_no = parms.gca_rq_parms.gca_assoc_id;
		MEfill( sizeof( parms ), 0, (PTR) &parms);
		parms.gca_da_parms.gca_association_id = assoc_no;
		(void) MtIIGCa_call( GCA_DISASSOC, &parms, GCA_SYNC_FLAG, 
			0, -1L, &tmp_stat );
		/* existing name servers answer OK, new ones NO_PEER */
		if( status == E_GC0000_OK || status == E_GC0032_NO_PEER ){
			MtIIGCa_call(   GCA_TERMINATE,
              &parms,
              (i4) GCA_SYNC_FLAG,    /* Synchronous */
              (PTR) 0,                /* async id */
              (i4) -1,           /* no timeout */
              &status);
			return( TRUE );
		}
	}
    MtIIGCa_call(   GCA_TERMINATE,
                  &parms,
                  (i4) GCA_SYNC_FLAG,    /* Synchronous */
                  (PTR) 0,                /* async id */
                  (i4) -1,           /* no timeout */
                  &status);
	return( FALSE );
}


BOOL TestInstallationThroughNet(char * puser, char * ppassword)
{
    STATUS		status;
    STATUS		call_status;
    GCA_IN_PARMS	in_parms;
    GCA_TE_PARMS	te_parms;

	BOOL bResult = FALSE;

	if (!lstrcmpi(puser,"*") || !lstrcmpi(ppassword,"*")) {
		return FALSE;
	}

    /*
    ** CL initialization.
    */
    if (!bMEAdviseInit) {
       MEadvise( ME_INGRES_ALLOC );  /* We'll use the Ingres malloc library */
       SIeqinit();
       bMEAdviseInit = TRUE;
    }

    /*
    ** Initialize communications.
    */
    MEfill(sizeof(in_parms), (u_char)0, (PTR)&in_parms);

    in_parms.gca_modifiers = GCA_API_VERSION_SPECD;
    in_parms.gca_api_version = GCA_API_LEVEL_5;
    in_parms.gca_local_protocol = GCA_PROTOCOL_LEVEL_62;

    call_status = MtIIGCa_call( GCA_INITIATE, (GCA_PARMLIST *)&in_parms,
                              GCA_SYNC_FLAG, NULL, -1, &status );

	if ( call_status == OK  &&  in_parms.gca_status != OK )
		status = in_parms.gca_status, call_status = FAIL;

	if ( call_status != OK ) 
	{
		ReleaseMutexGCA_Initiate_Terminate();
		return FALSE;
	}

	msg_max = GCA_FS_MAX_DATA + in_parms.gca_header_length;

	if ( (msg_buff = (char *)ESL_AllocMem(msg_max)) !=NULL ) {
		char target[ 64 ];
		char LocalHostName[MAXOBJECTNAME];
		i4  assoc_id;

		GChostname (LocalHostName,MAXOBJECTNAME);
		// Check if registry is active.
		// Build target specification for master Name Server:
		// ( @host::/iinmsvr for remote , which we test although the "node" is the local one)
#ifdef MAINWIN
	        /*
		** FIXME
		** Shouldn't be using registry to check if localhost is
		** active so for UNIX we don't (as it doesn't work).
		** Just connect to local name server.
		*/
		STprintf( target, "@::/iinmsvr" );
	        bResult = TRUE;
#else
		STprintf( target, "@%s::/iinmsvr", LocalHostName );

		/*
		** Connect to master Name Server.
		*/
		if ( (status = gca_request( target, &assoc_id, "*", "*")) == OK ) {
			gca_release( assoc_id );
			bResult = TRUE;
		}
		else
			bResult = FALSE;
		gca_disassoc( assoc_id );
#endif

		
		if ( bResult) { // ping has worked, test with the the real password
			if ( (status = gca_request( target, &assoc_id, puser, ppassword)) == OK ) {
				gca_release( assoc_id );
				bResult = TRUE;
			}
			else
				bResult = FALSE;
			gca_disassoc( assoc_id );
		}
		ESL_FreeMem(msg_buff);
	}


    /*
    ** Shutdown communications.
    */
    call_status = MtIIGCa_call( GCA_TERMINATE, (GCA_PARMLIST *)&te_parms,
                              GCA_SYNC_FLAG, NULL, -1, &status );

    if ( call_status == OK  &&  te_parms.gca_status != OK )
	status = te_parms.gca_status, call_status = FAIL;

    if ( call_status != OK )
		bResult = FALSE;
	return bResult;
}

#if defined (EDBC)
BOOL EDBC_ModifyLogin(LPCTSTR lpszVNodeName, LPCTSTR lpszUserName, LPCTSTR lpszPassword)
{
	BOOL bOK = FALSE;
	LPNODELOGINDATA pFirst = GetFirstLLLoginData();
	while (pFirst)
	{
		if (lstrcmpi (lpszVNodeName, (LPCTSTR)pFirst->NodeLoginDta.NodeName) == 0)
		{
			lstrcpy ((LPTSTR)pFirst->NodeLoginDta.Login,  lpszUserName);
			lstrcpy ((LPTSTR)pFirst->NodeLoginDta.Passwd, lpszPassword);
			bOK  = TRUE;
			break;
		}
		pFirst = pFirst->pnext;
	}
	return bOK;
}
#endif


/*
** Net trafic section  !!! begin !!!
** ************************************************************************************************
*/
FUNC_EXTERN i4  gcm_put_str();
FUNC_EXTERN i4  gcm_put_int();
FUNC_EXTERN i4  gcm_get_str();
FUNC_EXTERN i4  gcm_get_int();
FUNC_EXTERN i4  gcm_get_long();

static i4   gcm_message();
static VOID gcm_complete();
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
static STATUS gcm_fastselect();
static i4   get_async();
static i4   parm_index = 0;
static i4   parm_req[16];
static GCA_PARMLIST    parms[16];

#define nMAX_MIP 8
static PTR Mib2Fetch[nMAX_MIP]={
	GCC_MIB_IB_CONN_COUNT,
	GCC_MIB_IB_MAX,
	GCC_MIB_OB_CONN_COUNT,
	GCC_MIB_OB_MAX,
	GCC_MIB_MSGS_IN,
	GCC_MIB_DATA_IN,
	GCC_MIB_MSGS_OUT,
	GCC_MIB_DATA_OUT
};
static NETTRAFIC traficdata;


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

static i4
gcm_display(PTR start, NETTRAFIC* pData)
{
	i4 error_status;
	i4 error_index;
	char* r = start;
	i4 i;

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

	if( error_status )
	{
		traficdata.bError = TRUE;
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

		if (_tcsicmp(obj_class[0], GCC_MIB_IB_CONN_COUNT) == 0)
			pData->nInbound = _ttol(obj_value[0]);
		else
		if (_tcsicmp(obj_class[0], GCC_MIB_IB_MAX) == 0)
			pData->nInboundMax = _ttol(obj_value[0]);
		else
		if (_tcsicmp(obj_class[0], GCC_MIB_OB_CONN_COUNT) == 0)
			pData->nOutbound = _ttol(obj_value[0]);
		else
		if (_tcsicmp(obj_class[0], GCC_MIB_OB_MAX) == 0)
			pData->nOutboundMax = _ttol(obj_value[0]);
		else
		if (_tcsicmp(obj_class[0], GCC_MIB_MSGS_IN) == 0)
			pData->nPacketIn = _ttol(obj_value[0]);
		else
		if (_tcsicmp(obj_class[0], GCC_MIB_DATA_IN) == 0)
			pData->nDataIn = _ttol(obj_value[0]);
		else
		if (_tcsicmp(obj_class[0], GCC_MIB_MSGS_OUT) == 0)
			pData->nPacketOut = _ttol(obj_value[0]);
		else
		if (_tcsicmp(obj_class[0], GCC_MIB_DATA_OUT) == 0)
			pData->nDataOut = _ttol(obj_value[0]);
	}
	return r - start;
}

static VOID
gcm_complete(async_id)
i4 async_id;
{
	GCA_FS_PARMS* fs_parms;
	memset (&traficdata, 0, sizeof (traficdata));

	switch (parm_req[async_id])
	{
	case GCA_FASTSELECT:
		fs_parms = &parms[async_id].gca_fs_parms;

		if (fs_parms->gca_status == E_GCFFFE_INCOMPLETE)
		{
			gcm_fastselect(async_id);
			return;
		}

		if (fs_parms->gca_status != OK)
		{
			traficdata.bError = TRUE;
			if (fs_parms->gca_status != E_GC0032_NO_PEER)
			{
				GCshut();
			}
			return;
		}

		gcm_display( save_data_area, &traficdata);
		GCshut();
		break;
	default:
		break;
	}

	parm_req[async_id] = 0;
	return;
}

static STATUS
gcm_fastselect(id)
i4 id;
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
	if ( ! id )  MEfill( sizeof( *fs_parms ), 0, (PTR)fs_parms );
	fs_parms->gca_peer_protocol = GCA_PROTOCOL_LEVEL_62;
	fs_parms->gca_partner_name = target;
	fs_parms->gca_modifiers = GCA_RQ_GCM;
	fs_parms->gca_buffer = work_buff;
	fs_parms->gca_b_length = sizeof work_buff;

	if (!resume)
	{
		fs_parms->gca_msg_length = gcm_message( save_data_area );
		fs_parms->gca_message_type = message_type;
	}

	call_status = MtIIGCa_call(
		GCA_FASTSELECT,
		(GCA_PARMLIST *)fs_parms,
		GCA_ASYNC_FLAG | resume,
		(PTR) async_id,
		(i4) -1,
		&status);

	if (call_status != OK)
		return call_status;

	return OK;
}



static i4
gcm_message( start )
PTR	start;
{
	i4   i;
	char* q = start;

	for (i=0; i<nMAX_MIP; i++)
	{
		STcopy(Mib2Fetch[i], obj_class[i]);
		STcopy("0" , obj_instance[i]);
		STcopy("", obj_value[i]);
	}
	element_count = nMAX_MIP;

	q += sizeof(i4);               /* bump past error_status */
	q += sizeof(i4);               /* bump past error_index */
	q += sizeof(i4);               /* bump past future[0] */
	q += sizeof(i4);               /* bump past future[1] */
	q += gcm_put_int(q, -1);       /* set client_perms */
	q += gcm_put_int(q, row_count);/* set row_count */

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


BOOL NetQueryTraficInfo(LPTSTR lpszNode, LPTSTR lpszListenAddress, NETTRAFIC* pTraficInfo)
{
	STATUS status, call_status;
	GCA_IN_PARMS in_parms;
	GCA_FO_PARMS fo_parms;
	GCA_TE_PARMS te_parms;

	/* Issue initialization call to ME */
	if (!bMEAdviseInit) 
	{
		MEadvise( ME_INGRES_ALLOC );
		SIeqinit();
		bMEAdviseInit = TRUE;
	}
	/*
	** GCA initiate
	*/
	in_parms.gca_normal_completion_exit = gcm_complete;
	in_parms.gca_expedited_completion_exit = gcm_complete;

	in_parms.gca_alloc_mgr = NULL;
	in_parms.gca_dealloc_mgr = NULL;
	in_parms.gca_p_semaphore = NULL;
	in_parms.gca_v_semaphore = NULL;

	in_parms.gca_modifiers = GCA_API_VERSION_SPECD;
	in_parms.gca_api_version = GCA_API_LEVEL_4;
	in_parms.gca_local_protocol = GCA_PROTOCOL_LEVEL_62;

	call_status = MtIIGCa_call(
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
	fo_parms.gca_buffer = work_buff;
	fo_parms.gca_b_length = sizeof work_buff;

	call_status = MtIIGCa_call(
		GCA_FORMAT,
		(GCA_PARMLIST *)&fo_parms,
		(i4) GCA_SYNC_FLAG,
		(PTR) 0,
		(i4) -1,
		&status);

	if (call_status == OK)
	{
		save_data_area = fo_parms.gca_data_area;
		if (_tcsicmp(lpszNode, _T(""))==0 || _tcsicmp(lpszNode, _T("(local)")) == 0 )
			STpolycat( 2, ERx("/@"), lpszListenAddress, target);
		else
			STpolycat( 3, lpszNode, ERx("::/@"),lpszListenAddress, target );

		/*
		** Invoke first asyncronous service 
		*/
		gcm_fastselect(0);
		GCexec();
	}
	else
		traficdata.bError = TRUE; // will cause to return FALSE
	/*
	** Invoke GCA_TERMINATE
	*/
	call_status = MtIIGCa_call(
		GCA_TERMINATE,
		(GCA_PARMLIST *)&te_parms,
		(i4) GCA_SYNC_FLAG,
		(PTR) 0,
		(i4) -1,
		&status);

	if (call_status != OK)
		return FALSE;
	if (traficdata.bError)
		return FALSE;
	memcpy (pTraficInfo, &traficdata, sizeof(traficdata));
	return TRUE;
}

/*
** Net trafic section  !!! end !!!
** ************************************************************************************************
*/

/*
** Net Local Vnode  !!! Begin !!!
** ************************************************************************************************
*/

static i4     gcm_NLV_message();
static VOID   gcm_NLV_complete();
static STATUS gcm_NLV_fastselect();

static NETLOCALVNODENAME localvnodedata;

static i4
gcm_NLV_display(PTR start, NETLOCALVNODENAME* pData)
{
	i4 error_status;
	i4 error_index;
	char* r = start;
	//i4 i;

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

	if( error_status )
	{
		localvnodedata.bError = TRUE;
	}

//	for( i = 0; i < element_count; i++ )
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

		if (_tcsicmp(obj_class[0], GCN_MIB_LOCAL_VNODE) == 0)
			_tcscpy(pData->tcLocalVnode,obj_value[0]);
	}
	return r - start;
}

static VOID
gcm_NLV_complete(async_id)
i4 async_id;
{
	GCA_FS_PARMS* fs_parms;
	memset (&localvnodedata, 0, sizeof (localvnodedata));

	switch (parm_req[async_id])
	{
	case GCA_FASTSELECT:
		fs_parms = &parms[async_id].gca_fs_parms;

		if (fs_parms->gca_status == E_GCFFFE_INCOMPLETE)
		{
			gcm_fastselect(async_id);
			return;
		}

		if (fs_parms->gca_status != OK)
		{
			localvnodedata.bError = TRUE;
			if (fs_parms->gca_status != E_GC0032_NO_PEER)
			{
				GCshut();
			}
			return;
		}

		gcm_NLV_display( save_data_area, &localvnodedata);
		GCshut();
		break;
	default:
		break;
	}

	parm_req[async_id] = 0;
	return;
}


static STATUS
gcm_NLV_fastselect(id)
i4 id;
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
	if ( ! id )  MEfill( sizeof( *fs_parms ), 0, (PTR)fs_parms );
	fs_parms->gca_peer_protocol = GCA_PROTOCOL_LEVEL_62;
	fs_parms->gca_partner_name = target;
	fs_parms->gca_modifiers = GCA_RQ_GCM;
	fs_parms->gca_buffer = work_buff;
	fs_parms->gca_b_length = sizeof work_buff;

	if (!resume)
	{
		fs_parms->gca_msg_length = gcm_NLV_message( save_data_area );
		fs_parms->gca_message_type = message_type;
	}

	call_status = MtIIGCa_call(
		GCA_FASTSELECT,
		(GCA_PARMLIST *)fs_parms,
		GCA_ASYNC_FLAG | resume,
		(PTR) async_id,
		(i4) -1,
		&status);

	if (call_status != OK)
		return call_status;

	return OK;
}

static i4
gcm_NLV_message( start )
PTR	start;
{
	i4   i;
	char* q = start;

	STcopy(GCN_MIB_LOCAL_VNODE, obj_class[0]);
	STcopy("0" , obj_instance[0]);
	STcopy("", obj_value[0]);

	element_count = 1;

	q += sizeof(i4);               /* bump past error_status */
	q += sizeof(i4);               /* bump past error_index */
	q += sizeof(i4);               /* bump past future[0] */
	q += sizeof(i4);               /* bump past future[1] */
	q += gcm_put_int(q, -1);       /* set client_perms */
	q += gcm_put_int(q, row_count);/* set row_count */

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

BOOL NetQueryLocalVnode( NETLOCALVNODENAME *pLocalVnode )
{
	STATUS status, call_status;
	GCA_IN_PARMS in_parms;
	GCA_FO_PARMS fo_parms;
	GCA_TE_PARMS te_parms;
	int iLen;

	if (!hostName || *hostName == EOS)
		return FALSE;  /* it's necessary to fill hostname variable */
		               /* with NodeLL_FillHostName() function before */
		               /* used this function */

	if (localvnode && *localvnode != EOS)
	{
		_tcscpy(pLocalVnode->tcLocalVnode, localvnode);
		localvnodedata.bError = FALSE;
		return TRUE;
	}

	/* Issue initialization call to ME */
	if (!bMEAdviseInit) 
	{
		MEadvise( ME_INGRES_ALLOC );
		SIeqinit();
		bMEAdviseInit = TRUE;
	}
	/*
	** GCA initiate
	*/
	in_parms.gca_normal_completion_exit = gcm_NLV_complete;
	in_parms.gca_expedited_completion_exit = gcm_NLV_complete;

	in_parms.gca_alloc_mgr = NULL;
	in_parms.gca_dealloc_mgr = NULL;
	in_parms.gca_p_semaphore = NULL;
	in_parms.gca_v_semaphore = NULL;

	in_parms.gca_modifiers = GCA_API_VERSION_SPECD;
	in_parms.gca_api_version = GCA_API_LEVEL_4;
	in_parms.gca_local_protocol = GCA_PROTOCOL_LEVEL_62;

	call_status = MtIIGCa_call(
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
	fo_parms.gca_buffer = work_buff;
	fo_parms.gca_b_length = sizeof work_buff;

	call_status = MtIIGCa_call(
		GCA_FORMAT,
		(GCA_PARMLIST *)&fo_parms,
		(i4) GCA_SYNC_FLAG,
		(PTR) 0,
		(i4) -1,
		&status);

	if (call_status == OK)
	{
		save_data_area = fo_parms.gca_data_area;
		STpolycat( 2, hostName, ERx("::/iinmsvr"), target );

		/*
		** Invoke first asyncronous service 
		*/
		gcm_NLV_fastselect(0);
		GCexec();
	}
	else
		localvnodedata.bError = TRUE; // will cause to return FALSE;
	/*
	** Invoke GCA_TERMINATE
	*/
	call_status = MtIIGCa_call(
		GCA_TERMINATE,
		(GCA_PARMLIST *)&te_parms,
		(i4) GCA_SYNC_FLAG,
		(PTR) 0,
		(i4) -1,
		&status);

	if (call_status != OK)
		return FALSE;
	if (localvnodedata.bError)
		return FALSE;

	memcpy (pLocalVnode, &localvnodedata, sizeof(localvnodedata));

	iLen = _tcslen(pLocalVnode->tcLocalVnode);
	if ( !(localvnode = (char *) ESL_AllocMem(iLen*sizeof(TCHAR)+sizeof(TCHAR)))) {
		myerror(ERR_ALLOCMEM);
		return TRUE;
	}
	_tcscpy(localvnode, pLocalVnode->tcLocalVnode);

	return TRUE;
}

/*
** Net Local Vnode  !!! End !!!
** ************************************************************************************************
*/


/*
** Query MIB Objects  !!! Begin !!!
** ************************************************************************************************
*/
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

int ListTchar_Count (LPLISTTCHAR oldList)
{
	LPLISTTCHAR ls = oldList;
	int nCount = 0;
	while (ls)
	{
		nCount++;
		ls = ls->next;
	}
	return nCount;
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

	_tcscpy (lpStruct->tchszMIBCLASS, lpszMibClass);
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

LPMIBOBJECT MibObject_Search (LPMIBOBJECT oldList, LPCTSTR lpszMibClass)
{
	LPMIBOBJECT ls = oldList;
	while (ls)
	{
		if (_tcsicmp(ls->tchszMIBCLASS, lpszMibClass) == 0)
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


BOOL Mib_AddValue (LPMIBREQUEST pRequest, LPCTSTR lpszMibClass, LPCTSTR lpszMibValue)
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

		CheckEndSelect (pRequest, obj_class[0]);
		if (pRequest->bEnd)
			break;
		if (pRequest->pfnUserFetch)
			pRequest->pfnUserFetch (pRequest, obj_class[0], obj_instance[0], obj_value[0]);
		else
			Mib_AddValue (pRequest, obj_class[0], obj_value[0]);
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
			STcopy(ls->tchszMIBCLASS, obj_class[nCount]);
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
	q += gcm_put_int(q, row_count);/* set row_count */

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

	call_status = MtIIGCa_call(
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

static HANDLE g_hMutexQueryMIB = NULL;
BOOL QueryMIBObjects(LPMIBREQUEST pRequest, LPCTSTR lpszTarget, int nMode)
{
	STATUS status, call_status;
	GCA_IN_PARMS in_parms;
	GCA_FO_PARMS fo_parms;
	GCA_TE_PARMS te_parms;
	DWORD dwWait;
	row_count = 20;

	if (!g_hMutexQueryMIB)
		g_hMutexQueryMIB = CreateMutex(NULL, FALSE, NULL);
	if (!g_hMutexQueryMIB)
		return FALSE;
	dwWait = WaitForSingleObject(g_hMutexQueryMIB, 10*1000);
	switch (dwWait)
	{
	case WAIT_OBJECT_0:
		break;
	default:
		return FALSE;
	}
	memcpy (&g_mibRequest, pRequest, sizeof (MIBREQUEST));

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

	call_status = MtIIGCa_call(
		GCA_INITIATE,
		(GCA_PARMLIST *)&in_parms,
		(i4) GCA_SYNC_FLAG,
		(PTR) 0,
		(i4) -1,
		&status);

	if (call_status != OK)
	{
		ReleaseMutex (g_hMutexQueryMIB);
		return FALSE;
	}
	/*
	** Invoke GCA_FORMAT
	*/
	fo_parms.gca_buffer   = work_buff;
	fo_parms.gca_b_length = sizeof (work_buff);

	call_status = MtIIGCa_call(
		GCA_FORMAT,
		(GCA_PARMLIST *)&fo_parms,
		(i4) GCA_SYNC_FLAG,
		(PTR) 0,
		(i4) -1,
		&status);

	if (call_status != OK)
	{
		ReleaseMutex (g_hMutexQueryMIB);
		ReleaseMutexGCA_Initiate_Terminate();
		return FALSE;
	}

	save_data_area = fo_parms.gca_data_area;
	STcopy(lpszTarget, target);
	/*
	** Invoke first asyncronous service 
	*/
	bMibFirstSelect = TRUE;
	while (!g_mibRequest.bEnd && !g_mibRequest.bError)
	{
		gcm_mib_fastselect(0, &g_mibRequest);
		GCexec();
		bMibFirstSelect = FALSE;
	}

	/*
	** Invoke GCA_TERMINATE
	*/
	call_status = MtIIGCa_call(
		GCA_TERMINATE,
		(GCA_PARMLIST *)&te_parms,
		(i4) GCA_SYNC_FLAG,
		(PTR) 0,
		(i4) -1,
		&status);

	if (call_status != OK || g_mibRequest.bError)
	{
		ReleaseMutex (g_hMutexQueryMIB);
		return FALSE;
	}

	memcpy (pRequest, &g_mibRequest, sizeof (MIBREQUEST));
	memset (&g_mibRequest, 0, sizeof(g_mibRequest));
	ReleaseMutex (g_hMutexQueryMIB);
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

	call_status = MtIIGCa_call( GCA_INITIATE, (GCA_PARMLIST *)&in_parms, GCA_SYNC_FLAG, NULL, -1, &status );
	if ( call_status == OK  &&  in_parms.gca_status != OK )
		status = in_parms.gca_status, call_status = FAIL;

	if ( call_status != OK ) 
		bOK = FALSE;

	if (bOK)
	{
		msg_max = GCA_FS_MAX_DATA + in_parms.gca_header_length;
		if ( ! (msg_buff = (char *) ESL_AllocMem(msg_max))) 
		{
			call_status = MtIIGCa_call( GCA_TERMINATE, (GCA_PARMLIST *)&te_parms, GCA_SYNC_FLAG, NULL, -1, &status );
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

		call_status = MtIIGCa_call( GCA_TERMINATE, (GCA_PARMLIST *)&te_parms, GCA_SYNC_FLAG, NULL, -1, &status );
		if ( call_status == OK  &&  te_parms.gca_status != OK )
			status = te_parms.gca_status, call_status = FAIL;

		ESL_FreeMem(msg_buff);
		if ( call_status != OK ) 
			bOK = FALSE;
	}
	else
	{
		ReleaseMutexGCA_Initiate_Terminate();
	}
	return bOK;
}

LPLISTTCHAR GetGWlistLL(LPCTSTR lpszNode,  BOOL bDetectLocalServersOnly, int* pError)
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
	if (pError)
		*pError = 0;
	NodeLLInit();

	if (!CheckProtocolLevel_50 (lpszNode, &bGCA_PROTOCOL_LEVEL_50))
	{
		NodeLLTerminate();
		if (pError)
			*pError = 1;
		return ppresult;
	}
	if (bGCA_PROTOCOL_LEVEL_50)
	{
#ifdef EDBC
		ret = LLManageSaveLogin((LPUCHAR)lpszNode, TRUE);
#endif
		ppresult = GetListServerClass (lpszNode);
#ifdef EDBC
		ret = LLManageSaveLogin((LPUCHAR)lpszNode, FALSE);
#endif
	}
	else
	{
		// can be MVS or old ingres installs
		// use older code
		BOOL bVMSGwFound = FALSE;
		if (stricmp(lpszNode,""))  {
			if (NodeCheckConnection((LPUCHAR)lpszNode)!=OK)
			{
				if (pError)
					*pError = 1;
				return ppresult;
			}
		}
#ifdef	EDBC
		ret = LLManageSaveLogin((LPUCHAR)lpszNode, TRUE);
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
		ret = LLManageSaveLogin((LPUCHAR)lpszNode, FALSE);
#endif
	}
	NodeLLTerminate();

	if (bHasError)
	{
		if (pError)
			*pError = 1;
		return NULL;
	}
	return ppresult;
}

LPLISTTCHAR GetGWlist(LPCTSTR lpszNode, int* pError)
{
	if (pError)
		*pError = 0;
	return GetGWlistLL(lpszNode,  FALSE, pError);
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

#define NOT_SVRM  9
LPLISTTCHAR GetListServerClass(LPCTSTR lpszNode)
{
	MIBREQUEST mibRequest;
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

	memset (&mibRequest, 0, sizeof(mibRequest));
	tchszTarget[0] = _T('\0');
	mibRequest.listMIB = MibObject_Add (NULL, _T("exp.gcf.gcn.server.class"));
	if (!lpszNode || (lpszNode && ( _tcslen(lpszNode) == 0 || _tcsicmp (lpszNode, _T("(local)")) == 0)))
		_tcscpy(tchszTarget, _T("/iinmsvr"));
	else
	{
		_stprintf(tchszTarget, _T("%s::/iinmsvr"), lpszNode);
	}

	bOK = QueryMIBObjects(&mibRequest, tchszTarget, GCM_GETNEXT);
	if (bOK)
	{
		LPMIBOBJECT pMibSvrclass = MibObject_Search (mibRequest.listMIB, _T("exp.gcf.gcn.server.class"));
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
	mibRequest.listMIB = MibObject_Done(mibRequest.listMIB);
	return listSvr;
}

void CALLBACK userFetchMIBObject (void* pData, LPCTSTR lpszClass, LPCTSTR lpszInstance, LPCTSTR lpszValue)
{
	MIBREQUEST* pRequest = (MIBREQUEST*)pData;
	if (_tcsicmp (lpszClass, _T("exp.gcf.gcc.conn.inbound")) == 0 && _tcsicmp (lpszValue, _T("N")) == 0)
	{
		Mib_AddValue (pRequest, lpszClass, lpszInstance);
	}
	else
	{
		//
		// Check if the class name is among the specified objects:
		LPMIBOBJECT pMib = MibObject_Search (pRequest->listMIB, lpszClass);
		if (!pMib)
			return;
		//
		// Search for the instance (the instance that its value is 'N'):
		pMib = MibObject_Search (pRequest->listMIB, _T("exp.gcf.gcc.conn.inbound"));
		if (pMib)
		{
			BOOL bFound = FALSE;
			LPLISTTCHAR ls =  pMib->listValues;
			while (ls)
			{
				if (_tcsicmp(ls->lptchItem, lpszInstance) == 0)
				{
					bFound = TRUE;
					break;
				}
				ls = ls->next;
			}
			if (bFound)
				Mib_AddValue (pRequest, lpszClass, lpszValue);
		}
	}
}
